#ifndef VALUE_H_INCLUDED
#define VALUE_H_INCLUDED

#include <stdint.h>

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

    FPU    = 0x000400,
    FLOAT  = 0x080400 | sizeof(float),
    DOUBLE = 0x090400 | sizeof(double),

    PTR = 0x100000,
    ARR = 0x200000
};

#define HIGHER_TYPE(t1, t2) (((t1) < (t2)) ? (t2) : (t1))

#define value_type_index(t) ((t) >> 16)
#define value_type_sizeof(t) ((t) & 0xFF)
#define value_type_is_uint(v) ((v) & UINT)
#define value_type_is_sint(v) ((v) & SINT)
#define value_type_is_int(v) ((v) & INT)
#define value_type_is_fp(v) ((v) & FPU)

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

    float float_val;
    double double_val;
};

struct value {
    enum value_type type;
    union value_data data;
};

#define value_sizeof(v) (((v)->type) & 0xFF)
#define value_is_uint(v) (((v)->type) & UINT)
#define value_is_sint(v) (((v)->type) & SINT)
#define value_is_int(v) (((v)->type) & INT)
#define value_is_fp(v) (((v)->type) & FPU)

extern const struct value_operations *value_vtables[];
#define value_vtable(v) (value_vtables[(v)->type >> 16])
#define value_type_vtable(t) (value_vtables[(t) >> 16])

/*
 * Vtables for values.
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
    int (*cast_to_float)(struct value *this, struct value *out);
    int (*cast_to_double)(struct value *this, struct value *out);

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
int value_init_s8(struct value *dest, int8_t value);
int value_init_u8(struct value *dest, uint8_t value);
int value_init_s16(struct value *dest, int16_t value);
int value_init_u16(struct value *dest, uint16_t value);
int value_init_s32(struct value *dest, int32_t value);
int value_init_u32(struct value *dest, uint32_t value);
int value_init_s64(struct value *dest, int64_t value);
int value_init_u64(struct value *dest, uint64_t value);
int value_init_float(struct value *dest, float value);
int value_init_double(struct value *dest, double value);

/*
 * Initialize a value of type 'type'.
 *
 * If the type is invalid, horrible things happens! Value/data can be set with
 * vtable->assign() or value_set_data().
 */
int value_init(struct value *dest, enum value_type type);

/*
 * Assign data to a value.
 */
int value_set_data(struct value *dest, void *data);

/*
 * Check whether value equals to zero.
 */
int value_is_zero(struct value *dest);

/*
 * Produce a string representation of a type.
 * 
 * Returns a pointer to a static buffer or fills a buffer pointed by out. The
 * buffer must be at least 32 bytes.
 */
char *value_type_to_string(enum value_type type);
char *value_type_to_string_r(enum value_type type, char *out);

/*
 * Produce a string representation of a value.
 *
 * Returns a pointer to a static buffer or fills a buffer pointed by out. The
 * buffer must be at least 32 bytes.
 */
char *value_to_string(struct value *value);
char *value_to_string_r(struct value *value, char *out);

/*
 * Inverse of value_type_to_string.
 *
 * Returns value type or 0 on an error.
 */
enum value_type value_type_from_string(const char *type);

#endif
