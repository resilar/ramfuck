#include "eval.h"
#include "symbol.h"
#include "target.h"

#include <string.h>

static int ast_value_evaluate(struct ast *this, struct value *out)
{
    *out = ((struct ast_value *)this)->value;
    return 1;
}

static int ast_var_evaluate(struct ast *this, struct value *out)
{
    struct ast_var *var = (struct ast_var *)this;
    out->type = this->value_type;
    memcpy(&out->data, var->symtab->symbols[var->sym]->pdata, var->size);
    return 1;
}

static int ast_cast_evaluate(struct ast *this, struct value *out)
{
    struct value value;
    if ((this->value_type & PTR)) {
        struct ast_cast *cast = (struct ast_cast *)this;
        if (ast_evaluate(cast->tree.child, &value)) {
            enum value_type addr_type = cast->tree.child->value_type;
            if (value_type_ops(addr_type)->assign(out, &value)) {
                out->type = this->value_type;
                return 1;
            }
        }
        return 0;
    }
    return ast_evaluate(((struct ast_unary *)this)->child, &value)
        && value_type_ops(this->value_type)->assign(out, &value);
}

static int ast_deref_evaluate(struct ast *this, struct value *out)
{
    struct value value;
    if (ast_evaluate(((struct ast_unary *)this)->child, &value)) {
        struct target *target = ((struct ast_deref *)this)->target;
#if ADDR_BITS == 64
        addr_t addr = (value.type == U64) ? value.data.u64 : value.data.u32;
#else
        addr_t addr = value.data.u32;
#endif
        out->type = this->value_type;
        return target->read(target, addr, &out->data, value_sizeof(out));
    }
    return 0;
}

static int ast_neg_evaluate(struct ast *this, struct value *out)
{
    struct value value;
    return ast_evaluate(((struct ast_unary *)this)->child, &value)
        && value_ops(&value)->neg(&value, out);
}

static int ast_not_evaluate(struct ast *this, struct value *out)
{
    struct value value;
    return ast_evaluate(((struct ast_unary *)this)->child, &value)
        && value_ops(&value)->not(&value, out);
}

static int ast_compl_evaluate(struct ast *this, struct value *out)
{
    struct value value;
    return ast_evaluate(((struct ast_unary *)this)->child, &value)
        && value_ops(&value)->compl(&value, out);
}

static int ast_add_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binary *)this)->left, &left)
        && ast_evaluate(((struct ast_binary *)this)->right, &right)
        && value_ops(&left)->add(&left, &right, out);
}

static int ast_sub_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binary *)this)->left, &left)
        && ast_evaluate(((struct ast_binary *)this)->right, &right)
        && value_ops(&left)->sub(&left, &right, out);
}

static int ast_mul_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binary *)this)->left, &left)
        && ast_evaluate(((struct ast_binary *)this)->right, &right)
        && value_ops(&left)->mul(&left, &right, out);
}

static int ast_div_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binary *)this)->left, &left)
        && ast_evaluate(((struct ast_binary *)this)->right, &right)
        && value_ops(&left)->div(&left, &right, out);
}

static int ast_mod_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binary *)this)->left, &left)
        && ast_evaluate(((struct ast_binary *)this)->right, &right)
        && value_ops(&left)->mod(&left, &right, out);
}

static int ast_and_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binary *)this)->left, &left)
        && ast_evaluate(((struct ast_binary *)this)->right, &right)
        && value_ops(&left)->and(&left, &right, out);
}

static int ast_xor_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binary *)this)->left, &left)
        && ast_evaluate(((struct ast_binary *)this)->right, &right)
        && value_ops(&left)->xor(&left, &right, out);
}

static int ast_or_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binary *)this)->left, &left)
        && ast_evaluate(((struct ast_binary *)this)->right, &right)
        && value_ops(&left)->or(&left, &right, out);
}

static int ast_shl_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binary *)this)->left, &left)
        && ast_evaluate(((struct ast_binary *)this)->right, &right)
        && value_ops(&left)->shl(&left, &right, out);
}

static int ast_shr_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binary *)this)->left, &left)
        && ast_evaluate(((struct ast_binary *)this)->right, &right)
        && value_ops(&left)->shr(&left, &right, out);
}

static int ast_eq_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binary *)this)->left, &left)
        && ast_evaluate(((struct ast_binary *)this)->right, &right)
        && value_ops(&left)->eq(&left, &right, out);
}

static int ast_neq_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binary *)this)->left, &left)
        && ast_evaluate(((struct ast_binary *)this)->right, &right)
        && value_ops(&left)->neq(&left, &right, out);
}

static int ast_lt_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binary *)this)->left, &left)
        && ast_evaluate(((struct ast_binary *)this)->right, &right)
        && value_ops(&left)->lt(&left, &right, out);
}

static int ast_gt_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binary *)this)->left, &left)
        && ast_evaluate(((struct ast_binary *)this)->right, &right)
        && value_ops(&left)->gt(&left, &right, out);
}

static int ast_le_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binary *)this)->left, &left)
        && ast_evaluate(((struct ast_binary *)this)->right, &right)
        && value_ops(&left)->le(&left, &right, out);
}

static int ast_ge_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;
    return ast_evaluate(((struct ast_binary *)this)->left, &left)
        && ast_evaluate(((struct ast_binary *)this)->right, &right)
        && value_ops(&left)->ge(&left, &right, out);
}

static int ast_and_cond_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;

    if (!ast_evaluate(((struct ast_binary *)this)->left, &left))
        return 0;
    if (value_is_zero(&left)) {
        value_init_s32(out, 0);
        return 1;
    }

    if (!ast_evaluate(((struct ast_binary *)this)->right, &right))
        return 0;
    if (value_is_zero(&right)) {
        value_init_s32(out, 0);
        return 1;
    }

    value_init_s32(out, 1);
    return 1;
}

static int ast_or_cond_evaluate(struct ast *this, struct value *out)
{
    struct value left, right;

    if (!ast_evaluate(((struct ast_binary *)this)->left, &left))
        return 0;
    if (value_is_nonzero(&left)) {
        value_init_s32(out, 1);
        return 1;
    }

    if (!ast_evaluate(((struct ast_binary *)this)->right, &right))
        return 0;
    if (value_is_nonzero(&right)) {
        value_init_s32(out, 1);
        return 1;
    }

    value_init_s32(out, 0);
    return 1;
}

int (*ast_evaluate_funcs[AST_TYPES])(struct ast *, struct value *) = {
    /* AST_VALUE */ ast_value_evaluate,
    /* AST_VAR   */ ast_var_evaluate,

    /* AST_CAST  */ ast_cast_evaluate,
    /* AST_DEREF */ ast_deref_evaluate,
    /* AST_NEG   */ ast_neg_evaluate,
    /* AST_NOT   */ ast_not_evaluate,
    /* AST_COMPL */ ast_compl_evaluate,

    /* AST_ADD */ ast_add_evaluate,
    /* AST_SUB */ ast_sub_evaluate,
    /* AST_MUL */ ast_mul_evaluate,
    /* AST_DIV */ ast_div_evaluate,
    /* AST_MOD */ ast_mod_evaluate,

    /* AST_AND */ ast_and_evaluate,
    /* AST_XOR */ ast_xor_evaluate,
    /* AST_OR  */ ast_or_evaluate,
    /* AST_SHL */ ast_shl_evaluate,
    /* AST_SHR */ ast_shr_evaluate,

    /* AST_EQ  */ ast_eq_evaluate,
    /* AST_NEQ */ ast_neq_evaluate,
    /* AST_LT  */ ast_lt_evaluate,
    /* AST_GT  */ ast_gt_evaluate,
    /* AST_LE  */ ast_le_evaluate,
    /* AST_GE  */ ast_ge_evaluate,

    /* AST_AND_COND */ ast_and_cond_evaluate,
    /* AST_OR_COND  */ ast_or_cond_evaluate
};
