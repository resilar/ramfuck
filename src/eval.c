#include "eval.h"

#include <memory.h>
#include <stdlib.h>

int ast_int_evaluate(struct ast *this, struct value *out)
{
    intmax_t value = ((struct ast_int *)this)->value;
    if (value != (int32_t)value) {
        value_init_s64(out, value);
    } else {
        value_init_s32(out, value);
    }
    return 1;
}

int ast_uint_evaluate(struct ast *this, struct value *out)
{
    uintmax_t value = ((struct ast_uint *)this)->value;
    if (value != (uint32_t)value) {
        value_init_u64(out, value);
    } else {
        value_init_u32(out, value);
    }
    return 1;
}

int ast_float_evaluate(struct ast *this, struct value *out)
{
    value_init_double(out, ((struct ast_float *)this)->value);
    return 1;
}

int ast_value_evaluate(struct ast *this, struct value *out)
{
    *out = ((struct ast_value *)this)->value;
    return 1;
}

int ast_var_evaluate(struct ast *this, struct value *out)
{
    struct value *value;
    value = ((struct ast_var *)this)->value;
    *out = *value;
    /*value_vtable(out)->assign(out, value);*/
    return 1;
}

int ast_add_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binop *)this)->left, &left)
        && ast_evaluate(((struct ast_binop *)this)->right, &right)
        && value_vtable(&left)->add(&left, &right, out);
}

int ast_sub_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binop *)this)->left, &left)
        && ast_evaluate(((struct ast_binop *)this)->right, &right)
        && value_vtable(&left)->sub(&left, &right, out);
}

int ast_mul_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binop *)this)->left, &left)
        && ast_evaluate(((struct ast_binop *)this)->right, &right)
        && value_vtable(&left)->mul(&left, &right, out);
}

int ast_div_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binop *)this)->left, &left)
        && ast_evaluate(((struct ast_binop *)this)->right, &right)
        && value_vtable(&left)->div(&left, &right, out);
}

int ast_mod_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binop *)this)->left, &left)
        && ast_evaluate(((struct ast_binop *)this)->right, &right)
        && value_vtable(&left)->mod(&left, &right, out);
}

int ast_and_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binop *)this)->left, &left)
        && ast_evaluate(((struct ast_binop *)this)->right, &right)
        && value_vtable(&left)->and(&left, &right, out);
}

int ast_xor_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binop *)this)->left, &left)
        && ast_evaluate(((struct ast_binop *)this)->right, &right)
        && value_vtable(&left)->xor(&left, &right, out);
}

int ast_or_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binop *)this)->left, &left)
        && ast_evaluate(((struct ast_binop *)this)->right, &right)
        && value_vtable(&left)->or(&left, &right, out);
}

int ast_shl_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binop *)this)->left, &left)
        && ast_evaluate(((struct ast_binop *)this)->right, &right)
        && value_vtable(&left)->shl(&left, &right, out);
}

int ast_shr_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binop *)this)->left, &left)
        && ast_evaluate(((struct ast_binop *)this)->right, &right)
        && value_vtable(&left)->shr(&left, &right, out);
}

int ast_cast_evaluate(struct ast *this, struct value *out)
{
    struct value value;
    return ast_evaluate(((struct ast_unop *)this)->child, &value)
        && value_type_vtable(this->value_type)->assign(out, &value);
}

int ast_uadd_evaluate(struct ast *this, struct value *out)
{
    return ast_evaluate(((struct ast_unop *)this)->child, out);
}

int ast_usub_evaluate(struct ast *this, struct value *out)
{
    struct value value;
    return ast_evaluate(((struct ast_unop *)this)->child, &value)
        && value_vtable(&value)->usub(&value, out);
}

int ast_not_evaluate(struct ast *this, struct value *out)
{
    struct value value;
    return ast_evaluate(((struct ast_unop *)this)->child, &value)
        && value_vtable(&value)->not(&value, out);
}

int ast_compl_evaluate(struct ast *this, struct value *out)
{
    struct value value;
    return ast_evaluate(((struct ast_unop *)this)->child, &value)
        && value_vtable(&value)->compl(&value, out);
}

int ast_eq_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binop *)this)->left, &left)
        && ast_evaluate(((struct ast_binop *)this)->right, &right)
        && value_vtable(&left)->eq(&left, &right, out);
}

int ast_neq_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binop *)this)->left, &left)
        && ast_evaluate(((struct ast_binop *)this)->right, &right)
        && value_vtable(&left)->neq(&left, &right, out);
}

int ast_lt_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binop *)this)->left, &left)
        && ast_evaluate(((struct ast_binop *)this)->right, &right)
        && value_vtable(&left)->lt(&left, &right, out);
}

int ast_gt_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binop *)this)->left, &left)
        && ast_evaluate(((struct ast_binop *)this)->right, &right)
        && value_vtable(&left)->gt(&left, &right, out);
}

int ast_le_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binop *)this)->left, &left)
        && ast_evaluate(((struct ast_binop *)this)->right, &right)
        && value_vtable(&left)->le(&left, &right, out);
}

int ast_ge_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binop *)this)->left, &left)
        && ast_evaluate(((struct ast_binop *)this)->right, &right)
        && value_vtable(&left)->ge(&left, &right, out);
}

int ast_and_cond_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;

    if (!ast_evaluate(((struct ast_binop *)this)->left, &left))
        return 0;
    if (value_is_zero(&left)) {
        value_init_s32(out, 0);
        return 1;
    }

    if (!ast_evaluate(((struct ast_binop *)this)->right, &right))
        return 0;
    if (value_is_zero(&right)) {
        value_init_s32(out, 0);
        return 1;
    }

    value_init_s32(out, 1);
    return 1;
}

int ast_or_cond_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;

    if (!ast_evaluate(((struct ast_binop *)this)->left, &left))
        return 0;
    if (!value_is_zero(&left)) {
        value_init_s32(out, 1);
        return 1;
    }

    if (!ast_evaluate(((struct ast_binop *)this)->right, &right))
        return 0;
    if (!value_is_zero(&right)) {
        value_init_s32(out, 1);
        return 1;
    }

    value_init_s32(out, 0);
    return 1;
}
