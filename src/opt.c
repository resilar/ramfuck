#include "opt.h"
#include "eval.h"

static struct ast *ast_var_optimize(struct ast *this)
{
    struct ast_var *var = (struct ast_var *)this;
    struct ast *opt = ast_var_new(var->symtab, var->sym, var->size);
    if (opt) opt->value_type = this->value_type;
    return opt;
}

static struct ast *ast_value_optimize(struct ast *this)
{
    struct ast *ast;
    if ((ast = ast_value_new(&((struct ast_value *)this)->value)))
        ast->value_type = this->value_type;
    return ast;
}

static struct ast *ast_unop_optimize(struct ast *this)
{
    struct ast *child = ast_optimize(((struct ast_unop *)this)->child);
    struct ast *ast = ast_unop_new(this->node_type, child);
    if (ast) {
        ast->value_type = this->value_type;
        if (ast_is_constant(child)) {
            struct value value;
            if (ast_evaluate(ast, &value)) {
                ast_delete(ast);
                ast = ast_value_new(&value);
            }
        }
    }
    return ast;
}

static struct ast *ast_binop_optimize(struct ast *this)
{
    struct ast *left = ast_optimize(((struct ast_binop *)this)->left);
    struct ast *right = ast_optimize(((struct ast_binop *)this)->right);
    struct ast *ast = ast_binop_new(this->node_type, left, right);
    if (ast) {
        ast->value_type = this->value_type;
        if (ast_is_constant(left) && ast_is_constant(right)) {
            struct value value;
            if (ast_evaluate(ast, &value)) {
                ast_delete(ast);
                ast = ast_value_new(&value);
            }
        }
    }
    return ast;
}

static struct ast *ast_cast_optimize(struct ast *this)
{
    if ((this->value_type & PTR)) {
        struct ast *child = ast_optimize(((struct ast_unop *)this)->child);
        return ast_cast_new(this->value_type, child);
    }
    return ast_unop_optimize(this);
}

static struct ast *ast_deref_optimize(struct ast *this)
{
    struct target *target = ((struct ast_deref *)this)->target;
    struct ast *child = ast_optimize(((struct ast_unop *)this)->child);
    return ast_deref_new(child, this->value_type, target);
}

struct ast *(*ast_optimize_funcs[AST_TYPES])(struct ast *) = {
    /* AST_VALUE */ ast_value_optimize,
    /* AST_VAR   */ ast_var_optimize,

    /* AST_CAST  */ ast_cast_optimize,
    /* AST_DEREF */ ast_deref_optimize,
    /* AST_USUB  */ ast_unop_optimize,
    /* AST_NOT   */ ast_unop_optimize,
    /* AST_COMPL */ ast_unop_optimize,

    /* AST_ADD */ ast_binop_optimize,
    /* AST_SUB */ ast_binop_optimize,
    /* AST_MUL */ ast_binop_optimize,
    /* AST_DIV */ ast_binop_optimize,
    /* AST_MOD */ ast_binop_optimize,

    /* AST_AND */ ast_binop_optimize,
    /* AST_XOR */ ast_binop_optimize,
    /* AST_OR  */ ast_binop_optimize,
    /* AST_SHL */ ast_binop_optimize,
    /* AST_SHR */ ast_binop_optimize,

    /* AST_EQ  */ ast_binop_optimize,
    /* AST_NEQ */ ast_binop_optimize,
    /* AST_LT  */ ast_binop_optimize,
    /* AST_GT  */ ast_binop_optimize,
    /* AST_LE  */ ast_binop_optimize,
    /* AST_GE  */ ast_binop_optimize,

    /* AST_AND_COND */ ast_binop_optimize,
    /* AST_OR_COND  */ ast_binop_optimize
};
