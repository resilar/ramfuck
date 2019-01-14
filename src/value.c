#include "value.h"

#include <memory.h>
#include <stdio.h>

/*
 * Value operation declarations & definitions.
 *
 * Brace yourself, this file is ugly as fuck.
 */

/* Do nothing */
static int dummy(struct value *op1, struct value *op2, struct value *out);
static int udummy(struct value *op1, struct value *out);

/* Promote to s32 and do the operation */
static int dummy_add(struct value *op1, struct value *op2, struct value *out);
static int dummy_sub(struct value *op1, struct value *op2, struct value *out);
static int dummy_mul(struct value *op1, struct value *op2, struct value *out);
static int dummy_div(struct value *op1, struct value *op2, struct value *out);
static int dummy_mod(struct value *op1, struct value *op2, struct value *out);
static int dummy_and(struct value *op1, struct value *op2, struct value *out);
static int dummy_xor(struct value *op1, struct value *op2, struct value *out);
static int dummy_or(struct value *op1, struct value *op2, struct value *out);
static int dummy_shl(struct value *op1, struct value *op2, struct value *out);
static int dummy_shr(struct value *op1, struct value *op2, struct value *out);
static int dummy_usub(struct value *op1, struct value *out);
static int dummy_not(struct value *op1, struct value *out);
static int dummy_compl(struct value *op1, struct value *out);
static int dummy_eq(struct value *op1, struct value *op2, struct value *out);
static int dummy_neq(struct value *op1, struct value *op2, struct value *out);
static int dummy_lt(struct value *op1, struct value *op2, struct value *out);
static int dummy_gt(struct value *op1, struct value *op2, struct value *out);
static int dummy_le(struct value *op1, struct value *op2, struct value *out);
static int dummy_ge(struct value *op1, struct value *op2, struct value *out);

static int s8_to_s8(struct value *this, struct value *out);
static int s8_to_u8(struct value *this, struct value *out);
static int s8_to_s16(struct value *this, struct value *out);
static int s8_to_u16(struct value *this, struct value *out);
static int s8_to_s32(struct value *this, struct value *out);
static int s8_to_u32(struct value *this, struct value *out);
static int s8_to_s64(struct value *this, struct value *out);
static int s8_to_u64(struct value *this, struct value *out);
static int s8_to_float(struct value *this, struct value *out);
static int s8_to_double(struct value *this, struct value *out);
static int s8_assign(struct value *this, struct value *src);

static int u8_to_s8(struct value *this, struct value *out);
static int u8_to_u8(struct value *this, struct value *out);
static int u8_to_s16(struct value *this, struct value *out);
static int u8_to_u16(struct value *this, struct value *out);
static int u8_to_s32(struct value *this, struct value *out);
static int u8_to_u32(struct value *this, struct value *out);
static int u8_to_s64(struct value *this, struct value *out);
static int u8_to_u64(struct value *this, struct value *out);
static int u8_to_float(struct value *this, struct value *out);
static int u8_to_double(struct value *this, struct value *out);
static int u8_assign(struct value *this, struct value *src);

static int s16_to_s8(struct value *this, struct value *out);
static int s16_to_u8(struct value *this, struct value *out);
static int s16_to_s16(struct value *this, struct value *out);
static int s16_to_u16(struct value *this, struct value *out);
static int s16_to_s32(struct value *this, struct value *out);
static int s16_to_u32(struct value *this, struct value *out);
static int s16_to_s64(struct value *this, struct value *out);
static int s16_to_u64(struct value *this, struct value *out);
static int s16_to_float(struct value *this, struct value *out);
static int s16_to_double(struct value *this, struct value *out);
static int s16_assign(struct value *this, struct value *src);

static int u16_to_s8(struct value *this, struct value *out);
static int u16_to_u8(struct value *this, struct value *out);
static int u16_to_s16(struct value *this, struct value *out);
static int u16_to_u16(struct value *this, struct value *out);
static int u16_to_s32(struct value *this, struct value *out);
static int u16_to_u32(struct value *this, struct value *out);
static int u16_to_s64(struct value *this, struct value *out);
static int u16_to_u64(struct value *this, struct value *out);
static int u16_to_float(struct value *this, struct value *out);
static int u16_to_double(struct value *this, struct value *out);
static int u16_assign(struct value *this, struct value *src);

static int s32_to_s8(struct value *this, struct value *out);
static int s32_to_u8(struct value *this, struct value *out);
static int s32_to_s16(struct value *this, struct value *out);
static int s32_to_u16(struct value *this, struct value *out);
static int s32_to_s32(struct value *this, struct value *out);
static int s32_to_u32(struct value *this, struct value *out);
static int s32_to_s64(struct value *this, struct value *out);
static int s32_to_u64(struct value *this, struct value *out);
static int s32_to_float(struct value *this, struct value *out);
static int s32_to_double(struct value *this, struct value *out);
static int s32_assign(struct value *this, struct value *src);
static int s32_add(struct value *op1, struct value *op2, struct value *out);
static int s32_sub(struct value *op1, struct value *op2, struct value *out);
static int s32_mul(struct value *op1, struct value *op2, struct value *out);
static int s32_div(struct value *op1, struct value *op2, struct value *out);
static int s32_mod(struct value *op1, struct value *op2, struct value *out);
static int s32_and(struct value *op1, struct value *op2, struct value *out);
static int s32_xor(struct value *op1, struct value *op2, struct value *out);
static int s32_or(struct value *op1, struct value *op2, struct value *out);
static int s32_shl(struct value *op1, struct value *op2, struct value *out);
static int s32_shr(struct value *op1, struct value *op2, struct value *out);
static int s32_usub(struct value *op1, struct value *out);
static int s32_not(struct value *op1, struct value *out);
static int s32_compl(struct value *op1, struct value *out);
static int s32_eq(struct value *op1, struct value *op2, struct value *out);
static int s32_neq(struct value *op1, struct value *op2, struct value *out);
static int s32_lt(struct value *op1, struct value *op2, struct value *out);
static int s32_gt(struct value *op1, struct value *op2, struct value *out);
static int s32_le(struct value *op1, struct value *op2, struct value *out);
static int s32_ge(struct value *op1, struct value *op2, struct value *out);

static int u32_to_s8(struct value *this, struct value *out);
static int u32_to_u8(struct value *this, struct value *out);
static int u32_to_s16(struct value *this, struct value *out);
static int u32_to_u16(struct value *this, struct value *out);
static int u32_to_s32(struct value *this, struct value *out);
static int u32_to_u32(struct value *this, struct value *out);
static int u32_to_s64(struct value *this, struct value *out);
static int u32_to_u64(struct value *this, struct value *out);
static int u32_to_float(struct value *this, struct value *out);
static int u32_to_double(struct value *this, struct value *out);
static int u32_assign(struct value *this, struct value *src);
static int u32_add(struct value *op1, struct value *op2, struct value *out);
static int u32_sub(struct value *op1, struct value *op2, struct value *out);
static int u32_mul(struct value *op1, struct value *op2, struct value *out);
static int u32_div(struct value *op1, struct value *op2, struct value *out);
static int u32_mod(struct value *op1, struct value *op2, struct value *out);
static int u32_and(struct value *op1, struct value *op2, struct value *out);
static int u32_xor(struct value *op1, struct value *op2, struct value *out);
static int u32_or(struct value *op1, struct value *op2, struct value *out);
static int u32_shl(struct value *op1, struct value *op2, struct value *out);
static int u32_shr(struct value *op1, struct value *op2, struct value *out);
static int u32_usub(struct value *op1, struct value *out);
static int u32_not(struct value *op1, struct value *out);
static int u32_compl(struct value *op1, struct value *out);
static int u32_eq(struct value *op1, struct value *op2, struct value *out);
static int u32_neq(struct value *op1, struct value *op2, struct value *out);
static int u32_lt(struct value *op1, struct value *op2, struct value *out);
static int u32_gt(struct value *op1, struct value *op2, struct value *out);
static int u32_le(struct value *op1, struct value *op2, struct value *out);
static int u32_ge(struct value *op1, struct value *op2, struct value *out);

static int s64_to_s8(struct value *this, struct value *out);
static int s64_to_u8(struct value *this, struct value *out);
static int s64_to_s16(struct value *this, struct value *out);
static int s64_to_u16(struct value *this, struct value *out);
static int s64_to_s32(struct value *this, struct value *out);
static int s64_to_u32(struct value *this, struct value *out);
static int s64_to_s64(struct value *this, struct value *out);
static int s64_to_u64(struct value *this, struct value *out);
static int s64_to_float(struct value *this, struct value *out);
static int s64_to_double(struct value *this, struct value *out);
static int s64_assign(struct value *this, struct value *src);
static int s64_add(struct value *op1, struct value *op2, struct value *out);
static int s64_sub(struct value *op1, struct value *op2, struct value *out);
static int s64_mul(struct value *op1, struct value *op2, struct value *out);
static int s64_div(struct value *op1, struct value *op2, struct value *out);
static int s64_mod(struct value *op1, struct value *op2, struct value *out);
static int s64_and(struct value *op1, struct value *op2, struct value *out);
static int s64_xor(struct value *op1, struct value *op2, struct value *out);
static int s64_or(struct value *op1, struct value *op2, struct value *out);
static int s64_shl(struct value *op1, struct value *op2, struct value *out);
static int s64_shr(struct value *op1, struct value *op2, struct value *out);
static int s64_usub(struct value *op1, struct value *out);
static int s64_not(struct value *op1, struct value *out);
static int s64_compl(struct value *op1, struct value *out);
static int s64_eq(struct value *op1, struct value *op2, struct value *out);
static int s64_neq(struct value *op1, struct value *op2, struct value *out);
static int s64_lt(struct value *op1, struct value *op2, struct value *out);
static int s64_gt(struct value *op1, struct value *op2, struct value *out);
static int s64_le(struct value *op1, struct value *op2, struct value *out);
static int s64_ge(struct value *op1, struct value *op2, struct value *out);

static int u64_to_s8(struct value *this, struct value *out);
static int u64_to_u8(struct value *this, struct value *out);
static int u64_to_s16(struct value *this, struct value *out);
static int u64_to_u16(struct value *this, struct value *out);
static int u64_to_s32(struct value *this, struct value *out);
static int u64_to_u32(struct value *this, struct value *out);
static int u64_to_s64(struct value *this, struct value *out);
static int u64_to_u64(struct value *this, struct value *out);
static int u64_to_float(struct value *this, struct value *out);
static int u64_to_double(struct value *this, struct value *out);
static int u64_assign(struct value *this, struct value *src);
static int u64_add(struct value *op1, struct value *op2, struct value *out);
static int u64_sub(struct value *op1, struct value *op2, struct value *out);
static int u64_mul(struct value *op1, struct value *op2, struct value *out);
static int u64_div(struct value *op1, struct value *op2, struct value *out);
static int u64_mod(struct value *op1, struct value *op2, struct value *out);
static int u64_and(struct value *op1, struct value *op2, struct value *out);
static int u64_xor(struct value *op1, struct value *op2, struct value *out);
static int u64_or(struct value *op1, struct value *op2, struct value *out);
static int u64_shl(struct value *op1, struct value *op2, struct value *out);
static int u64_shr(struct value *op1, struct value *op2, struct value *out);
static int u64_usub(struct value *op1, struct value *out);
static int u64_not(struct value *op1, struct value *out);
static int u64_compl(struct value *op1, struct value *out);
static int u64_eq(struct value *op1, struct value *op2, struct value *out);
static int u64_neq(struct value *op1, struct value *op2, struct value *out);
static int u64_lt(struct value *op1, struct value *op2, struct value *out);
static int u64_gt(struct value *op1, struct value *op2, struct value *out);
static int u64_le(struct value *op1, struct value *op2, struct value *out);
static int u64_ge(struct value *op1, struct value *op2, struct value *out);


static int float_to_s8(struct value *this, struct value *out);
static int float_to_u8(struct value *this, struct value *out);
static int float_to_s16(struct value *this, struct value *out);
static int float_to_u16(struct value *this, struct value *out);
static int float_to_s32(struct value *this, struct value *out);
static int float_to_u32(struct value *this, struct value *out);
static int float_to_s64(struct value *this, struct value *out);
static int float_to_u64(struct value *this, struct value *out);
static int float_to_float(struct value *this, struct value *out);
static int float_to_double(struct value *this, struct value *out);
static int float_assign(struct value *this, struct value *src);
static int float_add(struct value *op1, struct value *op2, struct value *out);
static int float_sub(struct value *op1, struct value *op2, struct value *out);
static int float_mul(struct value *op1, struct value *op2, struct value *out);
static int float_div(struct value *op1, struct value *op2, struct value *out);
static int float_usub(struct value *op1, struct value *out);
static int float_not(struct value *op1, struct value *out);
static int float_eq(struct value *op1, struct value *op2, struct value *out);
static int float_neq(struct value *op1, struct value *op2, struct value *out);
static int float_lt(struct value *op1, struct value *op2, struct value *out);
static int float_gt(struct value *op1, struct value *op2, struct value *out);
static int float_le(struct value *op1, struct value *op2, struct value *out);
static int float_ge(struct value *op1, struct value *op2, struct value *out);

static int double_to_s8(struct value *this, struct value *out);
static int double_to_u8(struct value *this, struct value *out);
static int double_to_s16(struct value *this, struct value *out);
static int double_to_u16(struct value *this, struct value *out);
static int double_to_s32(struct value *this, struct value *out);
static int double_to_u32(struct value *this, struct value *out);
static int double_to_s64(struct value *this, struct value *out);
static int double_to_u64(struct value *this, struct value *out);
static int double_to_float(struct value *this, struct value *out);
static int double_to_double(struct value *this, struct value *out);
static int double_assign(struct value *this, struct value *src);
static int double_add(struct value *op1, struct value *op2, struct value *out);
static int double_sub(struct value *op1, struct value *op2, struct value *out);
static int double_mul(struct value *op1, struct value *op2, struct value *out);
static int double_div(struct value *op1, struct value *op2, struct value *out);
static int double_usub(struct value *op1, struct value *out);
static int double_not(struct value *op1, struct value *out);
static int double_eq(struct value *op1, struct value *op2, struct value *out);
static int double_neq(struct value *op1, struct value *op2, struct value *out);
static int double_lt(struct value *op1, struct value *op2, struct value *out);
static int double_gt(struct value *op1, struct value *op2, struct value *out);
static int double_le(struct value *op1, struct value *op2, struct value *out);
static int double_ge(struct value *op1, struct value *op2, struct value *out);

static const struct value_operations s32_vtable = {
    /*.cast_to_s8 =    */ s32_to_s8,
    /*.cast_to_u8 =    */ s32_to_u8,
    /*.cast_to_s16 =   */ s32_to_s16,
    /*.cast_to_u16 =   */ s32_to_u16,
    /*.cast_to_s32 =   */ s32_to_s32,
    /*.cast_to_u32 =   */ s32_to_u32,
    /*.cast_to_s64 =   */ s32_to_s64,
    /*.cast_to_u64 =   */ s32_to_u64,
    /*.cast_to_float = */ s32_to_float,
    /*.cast_to_double =*/ s32_to_double,
    /*.assign =        */ s32_assign,

    /*.add =*/ s32_add,
    /*.sub =*/ s32_sub,
    /*.mul =*/ s32_mul,
    /*.div =*/ s32_div,
    /*.mod =*/ s32_mod,

    /*.and =*/ s32_and,
    /*.xor =*/ s32_xor,
    /*.or = */ s32_or,
    /*.shl =*/ s32_shl,
    /*.shr =*/ s32_shr,

    /*.usub = */ s32_usub,
    /*.not =  */ s32_not,
    /*.compl =*/ s32_compl,

    /*.eq = */ s32_eq,
    /*.neq =*/ s32_neq,
    /*.lt = */ s32_lt,
    /*.gt = */ s32_gt,
    /*.le = */ s32_le,
    /*.ge = */ s32_ge
};

static const struct value_operations u32_vtable = {
    u32_to_s8, u32_to_u8, u32_to_s16, u32_to_u16,
    u32_to_s32, u32_to_u32, u32_to_s64, u32_to_u64,
    u32_to_float, u32_to_double,
    u32_assign,

    u32_add, u32_sub, u32_mul, u32_div, u32_mod,
    u32_and, u32_xor, u32_or, u32_shl, u32_shr,
    u32_usub, u32_not, u32_compl,
    u32_eq, u32_neq, u32_lt, u32_gt, u32_le, u32_ge
};

static const struct value_operations s64_vtable = {
    s64_to_s8, s64_to_u8, s64_to_s16, s64_to_u16,
    s64_to_s32, s64_to_u32, s64_to_s64, s64_to_u64,
    s64_to_float, s64_to_double,
    s64_assign,

    s64_add, s64_sub, s64_mul, s64_div, s64_mod,
    s64_and, s64_xor, s64_or, s64_shl, s64_shr,
    s64_usub, s64_not, s64_compl,
    s64_eq, s64_neq, s64_lt, s64_gt, s64_le, s64_ge
};

static const struct value_operations u64_vtable = {
    u64_to_s8, u64_to_u8, u64_to_s16, u64_to_u16,
    u64_to_s32, u64_to_u32, u64_to_s64, u64_to_u64,
    u64_to_float, u64_to_double,
    u64_assign,

    u64_add, u64_sub, u64_mul, u64_div, u64_mod,
    u64_and, u64_xor, u64_or, u64_shl, u64_shr,
    u64_usub, u64_not, u64_compl,
    u64_eq, u64_neq, u64_lt, u64_gt, u64_le, u64_ge
};

static const struct value_operations s8_vtable = {
    s8_to_s8, s8_to_u8, s8_to_s16, s8_to_u16,
    s8_to_s32, s8_to_u32, s8_to_s64, s8_to_u64,
    s8_to_float, s8_to_double,
    s8_assign,

    dummy_add, dummy_sub, dummy_mul, dummy_div, dummy_mod,
    dummy_and, dummy_xor, dummy_or, dummy_shl, dummy_shr,
    dummy_usub, dummy_not, dummy_compl,
    dummy_eq, dummy_neq, dummy_lt, dummy_gt, dummy_le, dummy_ge
};

static const struct value_operations u8_vtable = {
    u8_to_s8, u8_to_u8, u8_to_s16, u8_to_u16,
    u8_to_s32, u8_to_u32, u8_to_s64, u8_to_u64,
    u8_to_float, u8_to_double,
    u8_assign,

    dummy_add, dummy_sub, dummy_mul, dummy_div, dummy_mod,
    dummy_and, dummy_xor, dummy_or, dummy_shl, dummy_shr,
    dummy_usub, dummy_not, dummy_compl,
    dummy_eq, dummy_neq, dummy_lt, dummy_gt, dummy_le, dummy_ge
};

static const struct value_operations s16_vtable = {
    s16_to_s8, s16_to_u8, s16_to_s16, s16_to_u16,
    s16_to_s32, s16_to_u32, s16_to_s64, s16_to_u64,
    s16_to_float, s16_to_double,
    s16_assign,

    dummy_add, dummy_sub, dummy_mul, dummy_div, dummy_mod,
    dummy_and, dummy_xor, dummy_or, dummy_shl, dummy_shr,
    dummy_usub, dummy_not, dummy_compl,
    dummy_eq, dummy_neq, dummy_lt, dummy_gt, dummy_le, dummy_ge
};

static const struct value_operations u16_vtable = {
    u16_to_s8, u16_to_u8, u16_to_s16, u16_to_u16,
    u16_to_s32, u16_to_u32, u16_to_s64, u16_to_u64,
    u16_to_float, u16_to_double,
    u16_assign,

    dummy_add, dummy_sub, dummy_mul, dummy_div, dummy_mod,
    dummy_and, dummy_xor, dummy_or, dummy_shl, dummy_shr,
    dummy_usub, dummy_not, dummy_compl,
    dummy_eq, dummy_neq, dummy_lt, dummy_gt, dummy_le, dummy_ge
};

static const struct value_operations float_vtable = {
    float_to_s8, float_to_u8, float_to_s16, float_to_u16,
    float_to_s32, float_to_u32, float_to_s64, float_to_u64,
    float_to_float, float_to_double,
    float_assign,

    float_add, float_sub, float_mul, float_div, dummy_mod,
    dummy, dummy, dummy, dummy, dummy,
    float_usub, float_not, udummy,
    float_eq, float_neq, float_lt, float_gt, float_le, float_ge
};

static const struct value_operations double_vtable = {
    double_to_s8, double_to_u8, double_to_s16, double_to_u16,
    double_to_s32, double_to_u32, double_to_s64, double_to_u64,
    double_to_float, double_to_double,
    double_assign,

    double_add, double_sub, double_mul, double_div, dummy_mod,
    dummy, dummy, dummy, dummy, dummy,
    double_usub, double_not, udummy,
    double_eq, double_neq, double_lt, double_gt, double_le, double_ge
};

const struct value_operations *value_vtables[] = {
    &s8_vtable, &u8_vtable,
    &s16_vtable, &u16_vtable,
    &s32_vtable, &u32_vtable,
    &s64_vtable, &u64_vtable,
    &float_vtable, &double_vtable
};

/*
 * Initialization functions.
 */
int value_init_s8(struct value *dest, int8_t value)
{
    dest->type = S8;
    dest->data.s8 = value;
    return 1;
}

int value_init_u8(struct value *dest, uint8_t value)
{
    dest->type = U8;
    dest->data.u8 = value;
    return 1;
}

int value_init_s16(struct value *dest, int16_t value)
{
    dest->type = S16;
    dest->data.s16 = value;
    return 1;
}

int value_init_u16(struct value *dest, uint16_t value)
{
    dest->type = U16;
    dest->data.u16 = value;
    return 1;
}

int value_init_s32(struct value *dest, int32_t value)
{
    dest->type = S32;
    dest->data.s32 = value;
    return 1;
}

int value_init_u32(struct value *dest, uint32_t value)
{
    dest->type = U32;
    dest->data.u32 = value;
    return 1;
}

int value_init_s64(struct value *dest, int64_t value)
{
    dest->type = S64;
    dest->data.s64 = value;
    return 1;
}

int value_init_u64(struct value *dest, uint64_t value)
{
    dest->type = U64;
    dest->data.u64 = value;
    return 1;
}

int value_init_float(struct value *dest, float value)
{
    dest->type = FLOAT;
    dest->data.float_val = value;
    return 1;
}

int value_init_double(struct value *dest, double value)
{
    dest->type = DOUBLE;
    dest->data.double_val = value;
    return 1;
}

int value_init(struct value *dest, enum value_type type)
{
    dest->type = type;
    return 1;
}

int value_set_data(struct value *dest, void *data)
{
    memcpy(&dest->data, data, value_sizeof(dest));
    return 1;
}

int value_is_zero(struct value *dest)
{
    int i, j;
    for (i = 0, j = value_sizeof(dest); i < j; i++) {
        if (((char *)&dest->data)[i] != 0)
            return 0;
    }
    return 1;
}

char *value_type_to_string(enum value_type type)
{
    static char buf[32];
    return value_type_to_string_r(type, buf);
}

char *value_type_to_string_r(enum value_type type, char *out)
{
    switch (type)
    {
    case SINT: sprintf(out, "sint"); break;
    case S8:   sprintf(out, "s8");   break;
    case S16:  sprintf(out, "s16");  break;
    case S32:  sprintf(out, "s32");  break;
    case S64:  sprintf(out, "s64");  break;

    case UINT: sprintf(out, "uint"); break;
    case U8:   sprintf(out, "u8");   break;
    case U16:  sprintf(out, "u16");  break;
    case U32:  sprintf(out, "u32");  break;
    case U64:  sprintf(out, "u64");  break;

    case FPU: sprintf(out, "fpu"); break;
    case FLOAT:  sprintf(out, "f32"); break;
    case DOUBLE: sprintf(out, "f64"); break;

    default:
        sprintf(out, "???");
        break;
    }

    return out;
}

char *value_to_string(struct value *value)
{
    static char buf[256];
    return value_to_string_r(value, buf);
}

char *value_to_string_r(struct value *value, char *out)
{
    switch (value->type)
    {
    case S8:  sprintf(out, "%ld", (long)value->data.s8);  break;
    case S16: sprintf(out, "%ld", (long)value->data.s16); break;
    case S32: sprintf(out, "%ld", (long)value->data.s32); break;
    case S64: sprintf(out, "%ld", (long)value->data.s64); break;

    case U8:  sprintf(out, "%lu", (unsigned long)value->data.u8);  break;
    case U16: sprintf(out, "%lu", (unsigned long)value->data.u16); break;
    case U32: sprintf(out, "%lu", (unsigned long)value->data.u32); break;
    case U64: sprintf(out, "%lu", (unsigned long)value->data.u64); break;

    case FLOAT:
        sprintf(out, "%g", value->data.float_val);
        break;
    case DOUBLE:
        sprintf(out, "%g", value->data.double_val);
        break;

    default:
        sprintf(out, "???");
        break;
    }

    return out;
}

enum value_type value_type_from_string(const char *type)
{
    if (!strcmp(type, "s8")) {
        return S8;
    } else if (!strcmp(type, "s16")) {
        return S16;
    } else if (!strcmp(type, "s32")) {
        return S32;
    } else if (!strcmp(type, "s64")) {
        return S64;
    } else if (!strcmp(type, "u8")) {
        return U8;
    } else if (!strcmp(type, "u16")) {
        return U16;
    } else if (!strcmp(type, "u32")) {
        return U32;
    } else if (!strcmp(type, "u64")) {
        return U64;
    } else if (!strcmp(type, "float")) {
        return FLOAT;
    } else if (!strcmp(type, "double")) {
        return DOUBLE;
    }

    return 0;
}

#define DO_TYPE_CASTS(value_type, operation, op1, op2)                  \
    if ((op1)->type < (op2)->type) {                                    \
        struct value tmp_op;                                            \
        value_init(&tmp_op, (op2)->type);                               \
        value_vtable(&tmp_op)->assign(&tmp_op, (op1));                  \
        return value_vtable(&tmp_op)->operation(&tmp_op, (op2), out);   \
    } else if ((op1)->type > (op2)->type) {                             \
        value_vtable(op2)->cast_to_ ## value_type ((op2), (op2));       \
    }

static int dummy(struct value *op1, struct value *op2, struct value *out)
{
    return 0; /* fail. */
}

static int udummy(struct value *op1, struct value *out)
{
    return 0; /* fail. */
}

/*
 * Dummy promotion routines.
 */
static int dummy_add(struct value *op1, struct value *op2, struct value *out)
{
    value_vtable(op1)->cast_to_s32(op1, out);
    return value_vtable(out)->add(out, op2, out);
}

static int dummy_sub(struct value *op1, struct value *op2, struct value *out)
{
    value_vtable(op1)->cast_to_s32(op1, out);
    return value_vtable(out)->sub(out, op2, out);
}

static int dummy_mul(struct value *op1, struct value *op2, struct value *out)
{
    value_vtable(op1)->cast_to_s32(op1, out);
    return value_vtable(out)->mul(out, op2, out);
}

static int dummy_div(struct value *op1, struct value *op2, struct value *out)
{
    value_vtable(op1)->cast_to_s32(op1, out);
    return value_vtable(out)->div(out, op2, out);
}

static int dummy_mod(struct value *op1, struct value *op2, struct value *out)
{
    value_vtable(op1)->cast_to_s32(op1, out);
    return value_vtable(out)->mod(out, op2, out);
}

static int dummy_and(struct value *op1, struct value *op2, struct value *out)
{
    value_vtable(op1)->cast_to_s32(op1, out);
    return value_vtable(out)->and(out, op2, out);
}
static int dummy_xor(struct value *op1, struct value *op2, struct value *out)
{
    value_vtable(op1)->cast_to_s32(op1, out);
    return value_vtable(out)->xor(out, op2, out);
}

static int dummy_or(struct value *op1, struct value *op2, struct value *out)
{
    value_vtable(op1)->cast_to_s32(op1, out);
    return value_vtable(out)->or(out, op2, out);
}

static int dummy_shl(struct value *op1, struct value *op2, struct value *out)
{
    value_vtable(op1)->cast_to_s32(op1, out);
    return value_vtable(out)->shl(out, op2, out);
}

static int dummy_shr(struct value *op1, struct value *op2, struct value *out)
{
    value_vtable(op1)->cast_to_s32(op1, out);
    return value_vtable(out)->shr(out, op2, out);
}

static int dummy_usub(struct value *op1, struct value *out)
{
    value_vtable(op1)->cast_to_s32(op1, out);
    return value_vtable(out)->usub(out, out);
}

static int dummy_not(struct value *op1, struct value *out)
{
    value_vtable(op1)->cast_to_s32(op1, out);
    return value_vtable(out)->not(out, out);
}

static int dummy_compl(struct value *op1, struct value *out)
{
    value_vtable(op1)->cast_to_s32(op1, out);
    return value_vtable(out)->compl(out, out);
}

static int dummy_eq(struct value *op1, struct value *op2, struct value *out)
{
    value_vtable(op1)->cast_to_s32(op1, out);
    return value_vtable(out)->eq(out, op2, out);
}

static int dummy_neq(struct value *op1, struct value *op2, struct value *out)
{
    value_vtable(op1)->cast_to_s32(op1, out);
    return value_vtable(out)->neq(out, op2, out);
}

static int dummy_lt(struct value *op1, struct value *op2, struct value *out)
{
    value_vtable(op1)->cast_to_s32(op1, out);
    return value_vtable(out)->lt(out, op2, out);
}

static int dummy_gt(struct value *op1, struct value *op2, struct value *out)
{
    value_vtable(op1)->cast_to_s32(op1, out);
    return value_vtable(out)->gt(out, op2, out);
}

static int dummy_le(struct value *op1, struct value *op2, struct value *out)
{
    value_vtable(op1)->cast_to_s32(op1, out);
    return value_vtable(out)->le(out, op2, out);
}

static int dummy_ge(struct value *op1, struct value *op2, struct value *out)
{
    value_vtable(op1)->cast_to_s32(op1, out);
    return value_vtable(out)->ge(out, op2, out);
}

/*
 * s8 routines.
 */
static int s8_to_s8(struct value *this, struct value *out)
{
    return value_init_s8(out, this->data.s8);
}

static int s8_to_u8(struct value *this, struct value *out)
{
    return value_init_u8(out, this->data.s8);
}

static int s8_to_s16(struct value *this, struct value *out)
{
    return value_init_s16(out, this->data.s8);
}

static int s8_to_u16(struct value *this, struct value *out)
{
    return value_init_u16(out, this->data.s8);
}

static int s8_to_s32(struct value *this, struct value *out)
{
    return value_init_s32(out, this->data.s8);
}

static int s8_to_u32(struct value *this, struct value *out)
{
    return value_init_u32(out, this->data.s8);
}

static int s8_to_s64(struct value *this, struct value *out)
{
    return value_init_s64(out, this->data.s8);
}

static int s8_to_u64(struct value *this, struct value *out)
{
    return value_init_u64(out, this->data.s8);
}

static int s8_to_float(struct value *this, struct value *out)
{
    return value_init_float(out, this->data.s8);
}

static int s8_to_double(struct value *this, struct value *out)
{
    return value_init_double(out, this->data.s8);
}

static int s8_assign(struct value *this, struct value *src)
{
    return value_vtable(src)->cast_to_s8(src, this);
}

/*
 * u8 routines.
 */
static int u8_to_s8(struct value *this, struct value *out)
{
    return value_init_s8(out, this->data.u8);
}

static int u8_to_u8(struct value *this, struct value *out)
{
    return value_init_u8(out, this->data.u8);
}

static int u8_to_s16(struct value *this, struct value *out)
{
    return value_init_s16(out, this->data.u8);
}

static int u8_to_u16(struct value *this, struct value *out)
{
    return value_init_u16(out, this->data.u8);
}

static int u8_to_s32(struct value *this, struct value *out)
{
    return value_init_s32(out, this->data.u8);
}

static int u8_to_u32(struct value *this, struct value *out)
{
    return value_init_u32(out, this->data.u8);
}

static int u8_to_s64(struct value *this, struct value *out)
{
    return value_init_s64(out, this->data.u8);
}

static int u8_to_u64(struct value *this, struct value *out)
{
    return value_init_u64(out, this->data.u8);
}

static int u8_to_float(struct value *this, struct value *out)
{
    return value_init_float(out, this->data.u8);
}

static int u8_to_double(struct value *this, struct value *out)
{
    return value_init_double(out, this->data.u8);
}

static int u8_assign(struct value *this, struct value *src)
{
    return value_vtable(src)->cast_to_u8(src, this);
}

/*
 * s16 routines.
 */
static int s16_to_s8(struct value *this, struct value *out)
{
    return value_init_s8(out, this->data.s16);
}

static int s16_to_u8(struct value *this, struct value *out)
{
    return value_init_u8(out, this->data.s16);
}

static int s16_to_s16(struct value *this, struct value *out)
{
    return value_init_s16(out, this->data.s16);
}

static int s16_to_u16(struct value *this, struct value *out)
{
    return value_init_u16(out, this->data.s16);
}

static int s16_to_s32(struct value *this, struct value *out)
{
    return value_init_s32(out, this->data.s16);
}

static int s16_to_u32(struct value *this, struct value *out)
{
    return value_init_u32(out, this->data.s16);
}

static int s16_to_s64(struct value *this, struct value *out)
{
    return value_init_s64(out, this->data.s16);
}

static int s16_to_u64(struct value *this, struct value *out)
{
    return value_init_u64(out, this->data.s16);
}

static int s16_to_float(struct value *this, struct value *out)
{
    return value_init_float(out, this->data.s16);
}

static int s16_to_double(struct value *this, struct value *out)
{
    return value_init_double(out, this->data.s16);
}

static int s16_assign(struct value *this, struct value *src)
{
    return value_vtable(src)->cast_to_s16(src, this);
}

/*
 * u16 routines.
 */
static int u16_to_s8(struct value *this, struct value *out)
{
    return value_init_s8(out, this->data.u16);
}

static int u16_to_u8(struct value *this, struct value *out)
{
    return value_init_u8(out, this->data.u16);
}

static int u16_to_s16(struct value *this, struct value *out)
{
    return value_init_s16(out, this->data.u16);
}

static int u16_to_u16(struct value *this, struct value *out)
{
    return value_init_u16(out, this->data.u16);
}

static int u16_to_s32(struct value *this, struct value *out)
{
    return value_init_s32(out, this->data.u16);
}

static int u16_to_u32(struct value *this, struct value *out)
{
    return value_init_u32(out, this->data.u16);
}

static int u16_to_s64(struct value *this, struct value *out)
{
    return value_init_s64(out, this->data.u16);
}

static int u16_to_u64(struct value *this, struct value *out)
{
    return value_init_u64(out, this->data.u16);
}

static int u16_to_float(struct value *this, struct value *out)
{
    return value_init_float(out, this->data.u16);
}
    
static int u16_to_double(struct value *this, struct value *out)
{
    return value_init_double(out, this->data.u16);
}

static int u16_assign(struct value *this, struct value *src)
{
    return value_vtable(src)->cast_to_u16(src, this);
}

/*
 * s32 routines.
 */
static int s32_to_s8(struct value *this, struct value *out)
{
    return value_init_s8(out, this->data.s32);
}

static int s32_to_u8(struct value *this, struct value *out)
{
    return value_init_u8(out, this->data.s32);
}

static int s32_to_s16(struct value *this, struct value *out)
{
    return value_init_s16(out, this->data.s32);
}

static int s32_to_u16(struct value *this, struct value *out)
{
    return value_init_u16(out, this->data.s32);
}

static int s32_to_s32(struct value *this, struct value *out)
{
    return value_init_s32(out, this->data.s32);
}

static int s32_to_u32(struct value *this, struct value *out)
{
    return value_init_u32(out, this->data.s32);
}

static int s32_to_s64(struct value *this, struct value *out)
{
    return value_init_s64(out, this->data.s32);
}

static int s32_to_u64(struct value *this, struct value *out)
{
    return value_init_u64(out, this->data.s32);
}

static int s32_to_float(struct value *this, struct value *out)
{
    return value_init_float(out, this->data.s32);
}

static int s32_to_double(struct value *this, struct value *out)
{
    return value_init_double(out, this->data.s32);
}

static int s32_assign(struct value *this, struct value *src)
{
    return value_vtable(src)->cast_to_s32(src, this);
}

static int s32_add(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s32, add, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.s32 + op2->data.s32;
    return 1;
}

static int s32_sub(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s32, sub, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.s32 - op2->data.s32;
    return 1;
}

static int s32_mul(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s32, mul, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.s32 * op2->data.s32;
    return 1;
}

static int s32_div(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s32, div, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.s32 / op2->data.s32;
    return 1;
}

static int s32_mod(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s32, mod, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.s32 % op2->data.s32;
    return 1;
}

static int s32_and(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s32, and, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.s32 & op2->data.s32;
    return 1;
}

static int s32_xor(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s32, xor, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.s32 ^ op2->data.s32;
    return 1;
}

static int s32_or(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s32, or, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.s32 | op2->data.s32;
    return 1;
}

static int s32_shl(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s32, shl, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.s32 << op2->data.s32;
    return 1;
}

static int s32_shr(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s32, shr, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.s32 >> op2->data.s32;
    return 1;
}

static int s32_usub(struct value *op1, struct value *out)
{
    out->type = S32;
    out->data.s32 = -op1->data.s32;
    return 1;
}

static int s32_not(struct value *op1, struct value *out)
{
    out->type = S32;
    out->data.s32 = !op1->data.s32;
    return 1;
}

static int s32_compl(struct value *op1, struct value *out)
{
    out->type = S32;
    out->data.s32 = ~op1->data.s32;
    return 1;
}

static int s32_eq(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s32, eq, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.s32 == op2->data.s32;
    return 1;
}

static int s32_neq(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s32, neq, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.s32 != op2->data.s32;
    return 1;
}

static int s32_lt(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s32, lt, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.s32 < op2->data.s32;
    return 1;
}

static int s32_gt(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s32, gt, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.s32 > op2->data.s32;
    return 1;
}

static int s32_le(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s32, le, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.s32 <= op2->data.s32;
    return 1;
}

static int s32_ge(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s32, ge, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.s32 >= op2->data.s32;
    return 1;
}

/*
 * u32 routines.
 */
static int u32_to_s8(struct value *this, struct value *out)
{
    return value_init_s8(out, this->data.u32);
}

static int u32_to_u8(struct value *this, struct value *out)
{
    return value_init_u8(out, this->data.u32);
}

static int u32_to_s16(struct value *this, struct value *out)
{
    return value_init_s16(out, this->data.u32);
}

static int u32_to_u16(struct value *this, struct value *out)
{
    return value_init_u16(out, this->data.u32);
}

static int u32_to_s32(struct value *this, struct value *out)
{
    return value_init_s32(out, this->data.u32);
}

static int u32_to_u32(struct value *this, struct value *out)
{
    return value_init_u32(out, this->data.u32);
}

static int u32_to_s64(struct value *this, struct value *out)
{
    return value_init_s64(out, this->data.u32);
}

static int u32_to_u64(struct value *this, struct value *out)
{
    return value_init_u64(out, this->data.u32);
}

static int u32_to_float(struct value *this, struct value *out)
{
    return value_init_float(out, this->data.u32);
}

static int u32_to_double(struct value *this, struct value *out)
{
    return value_init_double(out, this->data.u32);
}

static int u32_assign(struct value *this, struct value *src)
{
    return value_vtable(src)->cast_to_u32(src, this);
}

static int u32_add(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u32, add, op1, op2);
    out->type = U32;
    out->data.u32 = op1->data.u32 + op2->data.u32;
    return 1;
}

static int u32_sub(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u32, sub, op1, op2);
    out->type = U32;
    out->data.u32 = op1->data.u32 - op2->data.u32;
    return 1;
}

static int u32_mul(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u32, mul, op1, op2);
    out->type = U32;
    out->data.u32 = op1->data.u32 * op2->data.u32;
    return 1;
}

static int u32_div(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u32, div, op1, op2);
    out->type = U32;
    out->data.u32 = op1->data.u32 / op2->data.u32;
    return 1;
}

static int u32_mod(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u32, mod, op1, op2);
    out->type = U32;
    out->data.u32 = op1->data.u32 % op2->data.u32;
    return 1;
}

static int u32_and(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u32, and, op1, op2);
    out->type = U32;
    out->data.s32 = op1->data.s32 & op2->data.s32;
    return 1;
}

static int u32_xor(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u32, xor, op1, op2);
    out->type = U32;
    out->data.s32 = op1->data.s32 ^ op2->data.s32;
    return 1;
}

static int u32_or(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u32, or, op1, op2);
    out->type = U32;
    out->data.s32 = op1->data.s32 | op2->data.s32;
    return 1;
}

static int u32_shl(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u32, shl, op1, op2);
    out->type = U32;
    out->data.u32 = op1->data.u32 << op2->data.u32;
    return 1;
}

static int u32_shr(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u32, shr, op1, op2);
    out->type = U32;
    out->data.u32 = op1->data.u32 >> op2->data.u32;
    return 1;
}

static int u32_usub(struct value *op1, struct value *out)
{
    out->type = U32;
    out->data.u32 = -op1->data.u32;
    return 1;
}

static int u32_not(struct value *op1, struct value *out)
{
    out->type = S32;
    out->data.s32 = !op1->data.u32;
    return 1;
}

static int u32_compl(struct value *op1, struct value *out)
{
    out->type = U32;
    out->data.u32 = ~op1->data.u32;
    return 1;
}

static int u32_eq(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u32, eq, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.u32 == op2->data.u32;
    return 1;
}

static int u32_neq(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u32, neq, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.u32 != op2->data.u32;
    return 1;
}

static int u32_lt(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u32, lt, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.u32 < op2->data.u32;
    return 1;
}

static int u32_gt(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u32, gt, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.u32 > op2->data.u32;
    return 1;
}

static int u32_le(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u32, le, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.u32 <= op2->data.u32;
    return 1;
}

static int u32_ge(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u32, ge, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.u32 >= op2->data.u32;
    return 1;
}

/*
 * s64 routines.
 */
static int s64_to_s8(struct value *this, struct value *out)
{
    return value_init_s8(out, this->data.s64);
}

static int s64_to_u8(struct value *this, struct value *out)
{
    return value_init_u8(out, this->data.s64);
}

static int s64_to_s16(struct value *this, struct value *out)
{
    return value_init_s16(out, this->data.s64);
}

static int s64_to_u16(struct value *this, struct value *out)
{
    return value_init_u16(out, this->data.s64);
}

static int s64_to_s32(struct value *this, struct value *out)
{
    return value_init_s32(out, this->data.s64);
}

static int s64_to_u32(struct value *this, struct value *out)
{
    return value_init_u32(out, this->data.s64);
}

static int s64_to_s64(struct value *this, struct value *out)
{
    return value_init_s64(out, this->data.s64);
}

static int s64_to_u64(struct value *this, struct value *out)
{
    return value_init_u64(out, this->data.s64);
}

static int s64_to_float(struct value *this, struct value *out)
{
    return value_init_float(out, this->data.s64);
}

static int s64_to_double(struct value *this, struct value *out)
{
    return value_init_double(out, this->data.s64);
}

static int s64_assign(struct value *this, struct value *src)
{
    return value_vtable(src)->cast_to_s64(src, this);
}

static int s64_add(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s64, add, op1, op2);
    out->type = S64;
    out->data.s64 = op1->data.s64 + op2->data.s64;
    return 1;
}

static int s64_sub(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s64, sub, op1, op2);
    out->type = S64;
    out->data.s64 = op1->data.s64 - op2->data.s64;
    return 1;
}

static int s64_mul(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s64, mul, op1, op2);
    out->type = S64;
    out->data.s64 = op1->data.s64 * op2->data.s64;
    return 1;
}

static int s64_div(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s64, div, op1, op2);
    out->type = S64;
    out->data.s64 = op1->data.s64 / op2->data.s64;
    return 1;
}

static int s64_mod(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s64, mod, op1, op2);
    out->type = S64;
    out->data.s64 = op1->data.s64 % op2->data.s64;
    return 1;
}

static int s64_and(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s64, and, op1, op2);
    out->type = S64;
    out->data.s64 = op1->data.s64 & op2->data.s64;
    return 1;
}

static int s64_xor(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s64, xor, op1, op2);
    out->type = S64;
    out->data.s64 = op1->data.s64 ^ op2->data.s64;
    return 1;
}

static int s64_or(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s64, or, op1, op2);
    out->type = S64;
    out->data.s64 = op1->data.s64 | op2->data.s64;
    return 1;
}

static int s64_shl(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s64, shl, op1, op2);
    out->type = S64;
    out->data.s64 = op1->data.s64 << op2->data.s64;
    return 1;
}

static int s64_shr(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s64, shr, op1, op2);
    out->type = S64;
    out->data.s64 = op1->data.s64 >> op2->data.s64;
    return 1;
}

static int s64_usub(struct value *op1, struct value *out)
{
    out->type = S64;
    out->data.s64 = -op1->data.s64;
    return 1;
}

static int s64_not(struct value *op1, struct value *out)
{
    out->type = S64;
    out->data.s64 = !op1->data.s64;
    return 1;
}

static int s64_compl(struct value *op1, struct value *out)
{
    out->type = S64;
    out->data.s64 = ~op1->data.s64;
    return 1;
}

static int s64_eq(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s64, eq, op1, op2);
    out->type = S64;
    out->data.s64 = op1->data.s64 == op2->data.s64;
    return 1;
}

static int s64_neq(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s64, neq, op1, op2);
    out->type = S64;
    out->data.s64 = op1->data.s64 != op2->data.s64;
    return 1;
}

static int s64_lt(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s64, lt, op1, op2);
    out->type = S64;
    out->data.s64 = op1->data.s64 < op2->data.s64;
    return 1;
}

static int s64_gt(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s64, gt, op1, op2);
    out->type = S64;
    out->data.s64 = op1->data.s64 > op2->data.s64;
    return 1;
}

static int s64_le(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s64, le, op1, op2);
    out->type = S64;
    out->data.s64 = op1->data.s64 <= op2->data.s64;
    return 1;
}

static int s64_ge(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(s64, ge, op1, op2);
    out->type = S64;
    out->data.s64 = op1->data.s64 >= op2->data.s64;
    return 1;
}

/*
 * u64 routines.
 */
static int u64_to_s8(struct value *this, struct value *out)
{
    return value_init_s8(out, this->data.u64);
}

static int u64_to_u8(struct value *this, struct value *out)
{
    return value_init_u8(out, this->data.u64);
}

static int u64_to_s16(struct value *this, struct value *out)
{
    return value_init_s16(out, this->data.u64);
}

static int u64_to_u16(struct value *this, struct value *out)
{
    return value_init_u16(out, this->data.u64);
}

static int u64_to_s32(struct value *this, struct value *out)
{
    return value_init_s32(out, this->data.u64);
}

static int u64_to_u32(struct value *this, struct value *out)
{
    return value_init_u32(out, this->data.u64);
}

static int u64_to_s64(struct value *this, struct value *out)
{
    return value_init_s64(out, this->data.u64);
}

static int u64_to_u64(struct value *this, struct value *out)
{
    return value_init_u64(out, this->data.u64);
}

static int u64_to_float(struct value *this, struct value *out)
{
    return value_init_float(out, this->data.u64);
}

static int u64_to_double(struct value *this, struct value *out)
{
    return value_init_double(out, this->data.u64);
}

static int u64_assign(struct value *this, struct value *src)
{
    return value_vtable(src)->cast_to_u64(src, this);
}

static int u64_add(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u64, add, op1, op2);
    out->type = U64;
    out->data.u64 = op1->data.u64 + op2->data.u64;
    return 1;
}

static int u64_sub(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u64, sub, op1, op2);
    out->type = U64;
    out->data.u64 = op1->data.u64 - op2->data.u64;
    return 1;
}

static int u64_mul(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u64, mul, op1, op2);
    out->type = U64;
    out->data.u64 = op1->data.u64 * op2->data.u64;
    return 1;
}

static int u64_div(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u64, div, op1, op2);
    out->type = U64;
    out->data.u64 = op1->data.u64 / op2->data.u64;
    return 1;
}

static int u64_mod(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u64, mod, op1, op2);
    out->type = U64;
    out->data.u64 = op1->data.u64 % op2->data.u64;
    return 1;
}

static int u64_and(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u64, and, op1, op2);
    out->type = U64;
    out->data.s32 = op1->data.s32 & op2->data.s32;
    return 1;
}

static int u64_xor(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u64, xor, op1, op2);
    out->type = U64;
    out->data.s32 = op1->data.s32 ^ op2->data.s32;
    return 1;
}

static int u64_or(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u64, or, op1, op2);
    out->type = U64;
    out->data.s32 = op1->data.s32 | op2->data.s32;
    return 1;
}

static int u64_shl(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u64, shl, op1, op2);
    out->type = U64;
    out->data.u64 = op1->data.u64 << op2->data.u64;
    return 1;
}

static int u64_shr(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u64, shr, op1, op2);
    out->type = U64;
    out->data.u64 = op1->data.u64 >> op2->data.u64;
    return 1;
}

static int u64_usub(struct value *op1, struct value *out)
{
    out->type = U64;
    out->data.u64 = -op1->data.u64;
    return 1;
}

static int u64_not(struct value *op1, struct value *out)
{
    out->type = S32;
    out->data.s32 = !op1->data.u64;
    return 1;
}

static int u64_compl(struct value *op1, struct value *out)
{
    out->type = U64;
    out->data.u64 = ~op1->data.u64;
    return 1;
}

static int u64_eq(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u64, eq, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.u64 == op2->data.u64;
    return 1;
}

static int u64_neq(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u64, neq, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.u64 != op2->data.u64;
    return 1;
}

static int u64_lt(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u64, lt, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.u64 < op2->data.u64;
    return 1;
}

static int u64_gt(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u64, gt, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.u64 > op2->data.u64;
    return 1;
}

static int u64_le(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u64, le, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.u64 <= op2->data.u64;
    return 1;
}

static int u64_ge(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(u64, ge, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.u64 >= op2->data.u64;
    return 1;
}

/*
 * float routines.
 */
static int float_to_s8(struct value *this, struct value *out)
{
    return value_init_s8(out, this->data.float_val);
}

static int float_to_u8(struct value *this, struct value *out)
{
    return value_init_u8(out, this->data.float_val);
}

static int float_to_s16(struct value *this, struct value *out)
{
    return value_init_s16(out, this->data.float_val);
}

static int float_to_u16(struct value *this, struct value *out)
{
    return value_init_u16(out, this->data.float_val);
}

static int float_to_s32(struct value *this, struct value *out)
{
    return value_init_s32(out, this->data.float_val);
}

static int float_to_u32(struct value *this, struct value *out)
{
    return value_init_u32(out, this->data.float_val);
}

static int float_to_s64(struct value *this, struct value *out)
{
    return value_init_s64(out, this->data.float_val);
}

static int float_to_u64(struct value *this, struct value *out)
{
    return value_init_u64(out, this->data.float_val);
}

static int float_to_float(struct value *this, struct value *out)
{
    return value_init_float(out, this->data.float_val);
}

static int float_to_double(struct value *this, struct value *out)
{
    return value_init_double(out, this->data.float_val);
}

static int float_assign(struct value *this, struct value *src)
{
    return value_vtable(src)->cast_to_float(src, this);
}

static int float_add(struct value *op1, struct value *op2, struct value *out)
{
    value_vtable(op1)->cast_to_double(op1, out);
    return value_vtable(out)->add(out, op2, out);
}

static int float_sub(struct value *op1, struct value *op2, struct value *out)
{
    value_vtable(op1)->cast_to_double(op1, out);
    return value_vtable(out)->sub(out, op2, out);
}

static int float_mul(struct value *op1, struct value *op2, struct value *out)
{
    value_vtable(op1)->cast_to_double(op1, out);
    return value_vtable(out)->mul(out, op2, out);
}

static int float_div(struct value *op1, struct value *op2, struct value *out)
{
    value_vtable(op1)->cast_to_double(op1, out);
    return value_vtable(out)->div(out, op2, out);
}

static int float_usub(struct value *op1, struct value *out)
{
    value_vtable(op1)->cast_to_double(op1, out);
    return value_vtable(out)->usub(out, out);
}

static int float_not(struct value *op1, struct value *out)
{
    value_vtable(op1)->cast_to_double(op1, out);
    return value_vtable(out)->not(out, out);
}

static int float_eq(struct value *op1, struct value *op2, struct value *out)
{
    value_vtable(op1)->cast_to_double(op1, out);
    return value_vtable(out)->eq(out, op2, out);
}

static int float_neq(struct value *op1, struct value *op2, struct value *out)
{
    value_vtable(op1)->cast_to_double(op1, out);
    return value_vtable(out)->neq(out, op2, out);
}

static int float_lt(struct value *op1, struct value *op2, struct value *out)
{
    value_vtable(op1)->cast_to_double(op1, out);
    return value_vtable(out)->lt(out, op2, out);
}

static int float_gt(struct value *op1, struct value *op2, struct value *out)
{
    value_vtable(op1)->cast_to_double(op1, out);
    return value_vtable(out)->gt(out, op2, out);
}

static int float_le(struct value *op1, struct value *op2, struct value *out)
{
    value_vtable(op1)->cast_to_double(op1, out);
    return value_vtable(out)->le(out, op2, out);
}

static int float_ge(struct value *op1, struct value *op2, struct value *out)
{
    value_vtable(op1)->cast_to_double(op1, out);
    return value_vtable(out)->ge(out, op2, out);
}


/*
 * double routines.
 */
static int double_to_s8(struct value *this, struct value *out)
{
    return value_init_s8(out, this->data.double_val);
}

static int double_to_u8(struct value *this, struct value *out)
{
    return value_init_u8(out, this->data.double_val);
}

static int double_to_s16(struct value *this, struct value *out)
{
    return value_init_s16(out, this->data.double_val);
}

static int double_to_u16(struct value *this, struct value *out)
{
    return value_init_u16(out, this->data.double_val);
}

static int double_to_s32(struct value *this, struct value *out)
{
    return value_init_s32(out, this->data.double_val);
}

static int double_to_u32(struct value *this, struct value *out)
{
    return value_init_u32(out, this->data.double_val);
}

static int double_to_s64(struct value *this, struct value *out)
{
    return value_init_s64(out, this->data.double_val);
}

static int double_to_u64(struct value *this, struct value *out)
{
    return value_init_u64(out, this->data.double_val);
}

static int double_to_float(struct value *this, struct value *out)
{
    return value_init_float(out, this->data.double_val);
}

static int double_to_double(struct value *this, struct value *out)
{
    return value_init_double(out, this->data.double_val);
}

static int double_assign(struct value *this, struct value *src)
{
    return value_vtable(src)->cast_to_double(src, this);
}

static int double_add(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(double, add, op1, op2);
    out->type = DOUBLE;
    out->data.double_val = op1->data.double_val + op2->data.double_val;
    return 1;
}

static int double_sub(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(double, sub, op1, op2);
    out->type = DOUBLE;
    out->data.double_val = op1->data.double_val - op2->data.double_val;
    return 1;
}

static int double_mul(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(double, mul, op1, op2);
    out->type = DOUBLE;
    out->data.double_val = op1->data.double_val * op2->data.double_val;
    return 1;
}

static int double_div(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(double, div, op1, op2);
    out->type = DOUBLE;
    out->data.double_val = op1->data.double_val / op2->data.double_val;
    return 1;
}

static int double_usub(struct value *op1, struct value *out)
{
    out->type = DOUBLE;
    out->data.double_val = -op1->data.double_val;
    return 1;
}

static int double_not(struct value *op1, struct value *out)
{
    out->type = S32;
    out->data.s32 = !op1->data.double_val;
    return 1;
}

static int double_eq(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(double, eq, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.double_val == op2->data.double_val;
    return 1;
}

static int double_neq(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(double, neq, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.double_val != op2->data.double_val;
    return 1;
}

static int double_lt(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(double, lt, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.double_val < op2->data.double_val;
    return 1;
}

static int double_gt(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(double, gt, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.double_val > op2->data.double_val;
    return 1;
}

static int double_le(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(double, le, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.double_val <= op2->data.double_val;
    return 1;
}

static int double_ge(struct value *op1, struct value *op2, struct value *out)
{
    DO_TYPE_CASTS(double, ge, op1, op2);
    out->type = S32;
    out->data.s32 = op1->data.double_val >= op2->data.double_val;
    return 1;
}

#undef DO_TYPE_CASTS
