#ifndef VALUE_H_INCLUDED
#define VALUE_H_INCLUDED

#include <stdint.h>
#include <stddef.h>

/*
 * Value types are defined so that t1 < t2 implies rank(t1) < rank(t2).
 * Rank is used in (implicit) value type conversions.
 */
enum value_type {
    SINT = 0x000100,
    S8   = 0x000101, SBYTE  = S8,
    S16  = 0x020102, SWORD  = S16,
    S32  = 0x040104, SDWORD = S32,
    S64  = 0x060108, SQWORD = S64,

    UINT = 0x000200,
    U8   = 0x010201, BYTE  = U8,
    U16  = 0x030202, WORD  = U16,
    U32  = 0x050204, DWORD = U32,
    U64  = 0x070208, QWORD = U64,
 
    INT = UINT | SINT,

    FPU = 0x000400,
    F32 = 0x080400 | sizeof(float),
    F64 = 0x090400 | sizeof(double),

    PTR = 0x100000,
    ARR = 0x200000
};

#define HIGHER_TYPE(t1, t2) (((t1) < (t2)) ? (t2) : (t1))

#define value_type_index(t) ((t) >> 16)
#define value_type_sizeof(t) ((t) & 0xFF)

union value_data {
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    uintmax_t uint;

    int8_t s8;
    int16_t s16;
    int32_t s32;
    int64_t s64;
    intmax_t sint;

    float f32;
    double f64;
};

struct value {
    enum value_type type;
    union value_data data;
};

#define value_sizeof(v) (((v)->type) & 0xFF)

extern const struct value_operations *value_vtables[];
#define value_vtable(v) (value_vtables[(v)->type >> 16])
#define value_type_vtable(t) (value_vtables[(t) >> 16])

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
    int (*cast_to_f32)(struct value *this, struct value *out);
    int (*cast_to_f64)(struct value *this, struct value *out);

    /* this = (typeof(this))src; */
    int (*assign)(struct value *this, struct value *src);

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

    int (*usub)(struct value *op1, struct value *out);
    int (*not)(struct value *op1, struct value *out);
    int (*compl)(struct value *op1, struct value *out);

    int (*eq)(struct value *op1, struct value *op2, struct value *out);
    int (*neq)(struct value *op1, struct value *op2, struct value *out);
    int (*lt)(struct value *op1, struct value *op2, struct value *out);
    int (*gt)(struct value *op1, struct value *op2, struct value *out);
    int (*le)(struct value *op1, struct value *op2, struct value *out);
    int (*ge)(struct value *op1, struct value *op2, struct value *out);
};

/*
 * Initialize value structure.
 * The caller does not have to free/destroy initialized values.
 */
int value_init(struct value *dest, enum value_type type, void *pvalue);
int value_init_s8(struct value *dest, int8_t value);
int value_init_u8(struct value *dest, uint8_t value);
int value_init_s16(struct value *dest, int16_t value);
int value_init_u16(struct value *dest, uint16_t value);
int value_init_s32(struct value *dest, int32_t value);
int value_init_u32(struct value *dest, uint32_t value);
int value_init_s64(struct value *dest, int64_t value);
int value_init_u64(struct value *dest, uint64_t value);
int value_init_f32(struct value *dest, float value);
int value_init_f64(struct value *dest, double value);

/*
 * Check whether value equals to (non-)zero.
 */
int value_is_zero(const struct value *dest);
#define value_is_nonzero(dest) (!value_is_zero((dest)))

/*
 * Return a constant string representing a value type.
 */
const char *value_type_to_string(enum value_type type);

/*
 * Produce a string representation of a value.
 *
 * Returns a pointer to a static buffer or fills a buffer pointed by out. The
 * buffer must be at least 32 bytes.
 */
size_t value_to_string(const struct value *value, char *out, size_t size);

/*
 * Inverse of value_type_to_string.
 *
 * Returns value type or 0 on an error.
 */
enum value_type value_type_from_string(const char *str);
enum value_type value_type_from_substring(const char *str, size_t len);

#endif
