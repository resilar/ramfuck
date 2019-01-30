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

static struct ast *ast_unary_optimize(struct ast *this)
{
    struct ast *child = ast_optimize(((struct ast_unary *)this)->child);
    struct ast *ast = ast_unary_new(this->node_type, child);
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

static struct ast *ast_binary_optimize(struct ast *this)
{
    struct ast *left = ast_optimize(((struct ast_binary *)this)->left);
    struct ast *right = ast_optimize(((struct ast_binary *)this)->right);
    struct ast *ast = ast_binary_new(this->node_type, left, right);
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
        struct ast *child = ast_optimize(((struct ast_unary *)this)->child);
        return ast_cast_new(this->value_type, child);
    }
    return ast_unary_optimize(this);
}

static struct ast *ast_deref_optimize(struct ast *this)
{
    struct target *target = ((struct ast_deref *)this)->target;
    struct ast *child = ast_optimize(((struct ast_unary *)this)->child);
    return ast_deref_new(child, this->value_type, target);
}

struct ast *(*ast_optimize_funcs[AST_TYPES])(struct ast *) = {
    /* AST_VALUE */ ast_value_optimize,
    /* AST_VAR   */ ast_var_optimize,

    /* AST_CAST  */ ast_cast_optimize,
    /* AST_DEREF */ ast_deref_optimize,
    /* AST_USUB  */ ast_unary_optimize,
    /* AST_NOT   */ ast_unary_optimize,
    /* AST_COMPL */ ast_unary_optimize,

    /* AST_ADD */ ast_binary_optimize,
    /* AST_SUB */ ast_binary_optimize,
    /* AST_MUL */ ast_binary_optimize,
    /* AST_DIV */ ast_binary_optimize,
    /* AST_MOD */ ast_binary_optimize,

    /* AST_AND */ ast_binary_optimize,
    /* AST_XOR */ ast_binary_optimize,
    /* AST_OR  */ ast_binary_optimize,
    /* AST_SHL */ ast_binary_optimize,
    /* AST_SHR */ ast_binary_optimize,

    /* AST_EQ  */ ast_binary_optimize,
    /* AST_NEQ */ ast_binary_optimize,
    /* AST_LT  */ ast_binary_optimize,
    /* AST_GT  */ ast_binary_optimize,
    /* AST_LE  */ ast_binary_optimize,
    /* AST_GE  */ ast_binary_optimize,

    /* AST_AND_COND */ ast_binary_optimize,
    /* AST_OR_COND  */ ast_binary_optimize
};
