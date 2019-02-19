#ifndef VALUE_H_INCLUDED
#define VALUE_H_INCLUDED

#include "defines.h"

#include <stdint.h>
#include <stddef.h>

/*
 * Value types are defined so that t1 < t2 implies rank(t1) < rank(t2).
 * Rank is used in (implicit) value type conversions.
 */
enum value_type {

    PTR = 0x10000000,
    ARR = 0x20000000,

    S8  = 0x00000001, S8PTR  = S8  | PTR,
    U8  = 0x00010001, U8PTR  = U8  | PTR,
    S16 = 0x00020002, S16PTR = S16 | PTR,
    U16 = 0x00030002, U16PTR = U16 | PTR,
    S32 = 0x00040004, S32PTR = S32 | PTR,
    U32 = 0x00050004, U32PTR = U32 | PTR,

    #ifndef NO_64BIT_VALUES
    S64 = 0x00060008, S64PTR = S64 | PTR,
    U64 = 0x00070008, U64PTR = U64 | PTR,
    SMAX = S64, UMAX = U64,
    #else
    SMAX = S32, UMAX = U32,
    #endif

    #ifndef NO_FLOAT_VALUES
    F32 = 0x00080004, F32PTR = F32 | PTR,
    F64 = 0x00090008, F64PTR = F64 | PTR,
    FMAX = F64,
    #endif

    #if ADDR_BITS == 64
    ADDR_TYPE = U64,
    #else
    ADDR_TYPE = U32,
    #endif

    VALUE_TYPES = 6
    #ifndef NO_64BIT_VALUES
                    + 2
    #endif
    #ifndef NO_FLOAT_VALUES
                        + 2
    #endif
};

#define HIGHER_TYPE(t1, t2) (((t1) < (t2)) ? (t2) : (t1))

#define value_type_index(t) ((t) >> 16)
#define value_type_sizeof(t) ((t) & 0xFF)
#define value_type_is_int(t) ((t) <= UMAX)
#ifndef NO_FLOAT_VALUES
#define value_type_is_fpu(t) ((t) > UMAX && (t) <= FMAX)
#else
#define value_type_is_fpu(t) 0
#endif

union value_data {
    int8_t s8; uint8_t u8;
    int16_t s16; uint16_t u16;
    int32_t s32; uint32_t u32;
    #ifndef NO_64BIT_VALUES
    int64_t s64; uint64_t u64;
    #endif
    #ifndef NO_FLOAT_VALUES
    float f32; double f64;
    #endif

    #ifndef NO_64BIT_VALUES
    int64_t smax;
    uint64_t umax;
    #else
    int32_t smax;
    uint32_t umax;
    #endif

    #if ADDR_BITS == 64
    uint64_t addr;
    #else
    uint32_t addr;
    #endif
};

struct value {
    enum value_type type;
    union value_data data;
};

#define value_index(v) value_type_index((v)->type)
#define value_sizeof(v) value_type_sizeof((v)->type)

/*
 * Value operations.
 */
struct value_operations {
    int (*cast_to_s8)(struct value *this, struct value *out);
    int (*cast_to_u8)(struct value *this, struct value *out);
    int (*cast_to_s16)(struct value *this, struct value *out);
    int (*cast_to_u16)(struct value *this, struct value *out);
    int (*cast_to_s32)(struct value *this, struct value *out);
    int (*cast_to_u32)(struct value *this, struct value *out);
    #ifndef NO_64BIT_VALUES
    int (*cast_to_s64)(struct value *this, struct value *out);
    int (*cast_to_u64)(struct value *this, struct value *out);
    #endif
    #ifndef NO_FLOAT_VALUES
    int (*cast_to_f32)(struct value *this, struct value *out);
    int (*cast_to_f64)(struct value *this, struct value *out);
    #endif

    /* this = (typeof(this))src; */
    int (*assign)(struct value *this, struct value *src);

    int (*neg)(struct value *op1, struct value *out);
    int (*not)(struct value *op1, struct value *out);
    int (*compl)(struct value *op1, struct value *out);

    int (*add)(struct value *op1, struct value *op2, struct value *out);
    int (*sub)(struct value *op1, struct value *op2, struct value *out);
    int (*mul)(struct value *op1, struct value *op2, struct value *out);
    int (*div)(struct value *op1, struct value *op2, struct value *out);
    int (*mod)(struct value *op1, struct value *op2, struct value *out);

    int (*and)(struct value *op1, struct value *op2, struct value *out);
    int (*xor)(struct value *op1, struct value *op2, struct value *out);
    int (*or)(struct value *op1, struct value *op2, struct value *out);
    int (*shl)(struct value *op1, struct value *op2, struct value *out);
    int (*shr)(struct value *op1, struct value *op2, struct value *out);

    int (*eq)(struct value *op1, struct value *op2, struct value *out);
    int (*neq)(struct value *op1, struct value *op2, struct value *out);
    int (*lt)(struct value *op1, struct value *op2, struct value *out);
    int (*gt)(struct value *op1, struct value *op2, struct value *out);
    int (*le)(struct value *op1, struct value *op2, struct value *out);
    int (*ge)(struct value *op1, struct value *op2, struct value *out);
};

const struct value_operations value_ops[VALUE_TYPES];
#define value_ops(v) (&value_ops[value_index((v))])
#define value_type_ops(t) (&value_ops[value_type_index((t))])

/* Initialize value structure (destroying not needed) */
int value_init_s8(struct value *dest, int8_t value);
int value_init_u8(struct value *dest, uint8_t value);
int value_init_s16(struct value *dest, int16_t value);
int value_init_u16(struct value *dest, uint16_t value);
int value_init_s32(struct value *dest, int32_t value);
int value_init_u32(struct value *dest, uint32_t value);
#ifndef NO_64BIT_VALUES
int value_init_s64(struct value *dest, int64_t value);
int value_init_u64(struct value *dest, uint64_t value);
#endif
#ifndef NO_FLOAT_VALUES
int value_init_f32(struct value *dest, float value);
int value_init_f64(struct value *dest, double value);
#endif

/* Check whether value is (non-)zero */
int value_is_zero(const struct value *dest);
#define value_is_nonzero(dest) (!value_is_zero((dest)))

/*
 * Return a constant string representing a value type.
 */
const char *value_type_to_string(enum value_type type);

/*
 * Produce a (hex)string representation of a value.
 */
size_t value_to_string(const struct value *value, char *out, size_t size);
size_t value_to_hexstring(const struct value *value, char *out, size_t size);

/*
 * Inverse of value_type_to_string().
 *
 * Returns value type (or 0 on an error).
 */
enum value_type value_type_from_substring(const char *str, size_t len);

#endif
