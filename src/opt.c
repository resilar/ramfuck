#include "opt.h"
#include "eval.h"

#include <string.h>

static struct ast *ast_binop_optimize(enum ast_type type, struct ast *this)
{
    struct ast *left = ast_optimize(((struct ast_binop *)this)->left);
    struct ast *right = ast_optimize(((struct ast_binop *)this)->right);
    struct ast *ast = ast_binop_new(type, left, right);
    ast->value_type = this->value_type;
    if (ast_is_constant(left) && ast_is_constant(right)) {
        struct value value;
        ast_evaluate(ast, &value);
        ast_delete(ast);
        ast = ast_value_new(&value);
    }
    return ast;
}

static struct ast *ast_unop_optimize(enum ast_type type, struct ast *this)
{
    struct ast *child = ast_optimize(((struct ast_unop *)this)->child);
    struct ast *ast = ast_unop_new(type, child);
    ast->value_type = this->value_type;
    if (ast_is_constant(child)) {
        struct value value;
        ast_evaluate(ast, &value);
        ast_delete(ast);
        ast = ast_value_new(&value);
    }
    return ast;
}

static struct ast *ast_int_optimize(struct ast *this)
{
    struct ast *ast = ast_int_new(((struct ast_int *)this)->value);
    ast->value_type = this->value_type;
    return ast;
}

static struct ast *ast_uint_optimize(struct ast *this)
{
    struct ast *ast = ast_uint_new(((struct ast_uint *)this)->value);
    ast->value_type = this->value_type;
    return ast;
}

static struct ast *ast_float_optimize(struct ast *this)
{
    struct ast *ast = ast_float_new(((struct ast_float *)this)->value);
    ast->value_type = this->value_type;
    return ast;
}

static struct ast *ast_value_optimize(struct ast *this)
{
    struct ast *ast = ast_value_new(&((struct ast_value *)this)->value);
    ast->value_type = this->value_type;
    return ast;
}

static struct ast *ast_var_optimize(struct ast *this)
{
    struct ast *ast = ast_var_new(((struct ast_var *)this)->identifier,
                                  ((struct ast_var *)this)->value);
    ast->value_type = this->value_type;
    return ast;
}

static struct ast *ast_add_optimize(struct ast *this)
{
    return ast_binop_optimize(AST_ADD, this);
}

static struct ast *ast_sub_optimize(struct ast *this)
{
    return ast_binop_optimize(AST_SUB, this);
}

static struct ast *ast_mul_optimize(struct ast *this)
{
    return ast_binop_optimize(AST_MUL, this);
}

static struct ast *ast_div_optimize(struct ast *this)
{
    return ast_binop_optimize(AST_DIV, this);
}

static struct ast *ast_mod_optimize(struct ast *this)
{
    return ast_binop_optimize(AST_MOD, this);
}

static struct ast *ast_and_optimize(struct ast *this)
{
    return ast_binop_optimize(AST_AND, this);
}

static struct ast *ast_xor_optimize(struct ast *this)
{
    struct ast *ast;
    ast = ast_binop_optimize(AST_XOR, this);
    return ast;
}

static struct ast *ast_or_optimize(struct ast *this)
{
    return ast_binop_optimize(AST_OR, this);
}

static struct ast *ast_shl_optimize(struct ast *this)
{
    return ast_binop_optimize(AST_SHL, this);
}

static struct ast *ast_shr_optimize(struct ast *this)
{
    return ast_binop_optimize(AST_SHR, this);
}

static struct ast *ast_cast_optimize(struct ast *this)
{
    return ast_unop_optimize(AST_CAST, this);
}

static struct ast *ast_uadd_optimize(struct ast *this)
{
    return ast_unop_optimize(AST_UADD, this);
}

static struct ast *ast_usub_optimize(struct ast *this)
{
    return ast_unop_optimize(AST_USUB, this);
}

static struct ast *ast_not_optimize(struct ast *this)
{
    return ast_unop_optimize(AST_NOT, this);
}

static struct ast *ast_compl_optimize(struct ast *this)
{
    return ast_unop_optimize(AST_COMPL, this);
}

static struct ast *ast_eq_optimize(struct ast *this)
{
    return ast_binop_optimize(AST_EQ, this);
}

static struct ast *ast_neq_optimize(struct ast *this)
{
    return ast_binop_optimize(AST_NEQ, this);
}

static struct ast *ast_lt_optimize(struct ast *this)
{
    return ast_binop_optimize(AST_LT, this);
}

static struct ast *ast_gt_optimize(struct ast *this)
{
    return ast_binop_optimize(AST_GT, this);
}

static struct ast *ast_le_optimize(struct ast *this)
{
    return ast_binop_optimize(AST_LE, this);
}

static struct ast *ast_ge_optimize(struct ast *this)
{
    return ast_binop_optimize(AST_GE, this);
}

static struct ast *ast_and_cond_optimize(struct ast *this)
{
    return ast_binop_optimize(AST_AND_COND, this);
}

static struct ast *ast_or_cond_optimize(struct ast *this)
{
    return ast_binop_optimize(AST_OR_COND, this);
}

struct ast *(*ast_optimize_funcs[AST_TYPES])(struct ast *this) = {
    /* AST_INT   */ ast_int_optimize,
    /* AST_UINT  */ ast_uint_optimize,
    /* AST_FLOAT */ ast_float_optimize,
    /* AST_VALUE */ ast_value_optimize,
    /* AST_VAR   */ ast_var_optimize,

    /* AST_ADD */ ast_add_optimize,
    /* AST_SUB */ ast_sub_optimize,
    /* AST_MUL */ ast_mul_optimize,
    /* AST_DIV */ ast_div_optimize,
    /* AST_MOD */ ast_mod_optimize,

    /* AST_AND */ ast_and_optimize,
    /* AST_XOR */ ast_xor_optimize,
    /* AST_OR  */ ast_or_optimize,
    /* AST_SHL */ ast_shl_optimize,
    /* AST_SHR */ ast_shr_optimize,

    /* AST_CAST  */ ast_cast_optimize,
    /* AST_UADD  */ ast_uadd_optimize,
    /* AST_USUB  */ ast_usub_optimize,
    /* AST_NOT   */ ast_not_optimize,
    /* AST_COMPL */ ast_compl_optimize,

    /* AST_EQ  */ ast_eq_optimize,
    /* AST_NEQ */ ast_neq_optimize,
    /* AST_LT  */ ast_lt_optimize,
    /* AST_GT  */ ast_gt_optimize,
    /* AST_LE  */ ast_le_optimize,
    /* AST_GE  */ ast_ge_optimize,

    /* AST_AND_COND */ ast_and_cond_optimize,
    /* AST_OR_COND  */ ast_or_cond_optimize
};
