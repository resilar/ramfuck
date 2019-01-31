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
    INT = 0x000100,
    FPU = 0x000200,

    PTR = 0x100000,
    ARR = 0x200000,

    S8  = 0x000101, S8PTR  = S8  | PTR,
    U8  = 0x010101, U8PTR  = U8  | PTR,

    S16 = 0x020102, S16PTR = S16 | PTR,
    U16 = 0x030102, U16PTR = U16 | PTR,

    S32 = 0x040104, S32PTR = S32 | PTR,
    U32 = 0x050104, U32PTR = U32 | PTR,

    S64 = 0x060108, S64PTR = S64 | PTR,
    U64 = 0x070108, U64PTR = U64 | PTR,

    #ifndef NO_FLOAT_VALUES
    F32 = 0x080200 | sizeof(float),  F32PTR = F32 | PTR,
    F64 = 0x090200 | sizeof(double), F64PTR = F64 | PTR,
    #endif

    VALUE_TYPES = 4 + 4 + 2
    #ifdef NO_FLOAT_VALUES
                            - 2
    #endif
};

#define HIGHER_TYPE(t1, t2) (((t1) < (t2)) ? (t2) : (t1))

#define value_type_index(t) ((t) >> 16)
#define value_type_sizeof(t) ((t) & 0xFF)

union value_data {
    int8_t s8; uint8_t u8;
    int16_t s16; uint16_t u16;
    int32_t s32; uint32_t u32;
    int64_t s64; uint64_t u64;
    #ifndef NO_FLOAT_VALUES
    float f32; double f64;
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

#define value_sizeof(v) (((v)->type) & 0xFF)

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
    int (*cast_to_s64)(struct value *this, struct value *out);
    int (*cast_to_u64)(struct value *this, struct value *out);
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
#define value_ops(v) (&value_ops[(v)->type >> 16])
#define value_type_ops(t) (&value_ops[(t) >> 16])

/* Initialize value structure (destroying not needed) */
int value_init_s8(struct value *dest, int8_t value);
int value_init_u8(struct value *dest, uint8_t value);
int value_init_s16(struct value *dest, int16_t value);
int value_init_u16(struct value *dest, uint16_t value);
int value_init_s32(struct value *dest, int32_t value);
int value_init_u32(struct value *dest, uint32_t value);
int value_init_s64(struct value *dest, int64_t value);
int value_init_u64(struct value *dest, uint64_t value);
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
 * Inverse of value_type_to_string.
 *
 * Returns value type (or 0 on an error).
 */
enum value_type value_type_from_string(const char *str);
enum value_type value_type_from_substring(const char *str, size_t len);

#endif
