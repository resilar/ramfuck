#define _DEFAULT_SOURCE /* for snprintf(3) */
#include "ast.h"
#include "ramfuck.h"
#include "symbol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * AST allocation and initialization functions.
 */
struct ast *ast_value_new(struct value *value)
{
    struct ast_value *n;
    if ((n = malloc(sizeof(struct ast_value)))) {
        n->root.node_type = AST_VALUE;
        n->root.value_type = value->type;
        n->value = *value;
    }
    return (struct ast *)n;
}

struct ast *ast_var_new(struct symbol_table *symtab, size_t sym)
{
    struct ast_var *n;
    if ((n = malloc(sizeof(struct ast_var)))) {
        n->root.node_type = AST_VAR;
        n->root.value_type = symtab->symbols[sym]->type;
        n->symtab = symtab;
        n->sym = sym;
    }
    return (struct ast *)n;
}

struct ast *ast_cast_new(enum value_type value_type, struct ast *child)
{
    struct ast_unop *n;
    if ((n = malloc(sizeof(struct ast_cast)))) {
        n->root.node_type = AST_CAST;
        n->root.value_type = value_type;
        n->child = child;
    }
    return (struct ast *)n;
}

struct ast *ast_unop_new(enum ast_type node_type, struct ast *child)
{
    struct ast_unop *n;
    if ((n = malloc(sizeof(struct ast_unop)))) {
        n->root.node_type = node_type;
        n->child = child;
    }
    return (struct ast *)n;
}

struct ast *ast_binop_new(enum ast_type node_type,
                          struct ast *left, struct ast *right)
{
    struct ast_binop *n;
    if ((n = malloc(sizeof(struct ast_binop)))) {
        n->root.node_type = node_type;
        n->left = left;
        n->right = right;
    }
    return (struct ast *)n;
}

/*
 * Delete.
 */
static void ast_leaf_delete(struct ast *this)
{
    free(this);
}

static void ast_unop_delete(struct ast *this)
{
    ast_delete(((struct ast_unop *)this)->child);
    free(this);
}

static void ast_binop_delete(struct ast *this)
{
    ast_delete(((struct ast_binop *)this)->left);
    ast_delete(((struct ast_binop *)this)->right);
    free(this);
}

void (*ast_delete_funcs[AST_TYPES])(struct ast *) = {
    /* AST_VALUE */ ast_leaf_delete,
    /* AST_VAR   */ ast_leaf_delete,

    /* AST_CAST  */ ast_unop_delete,
    /* AST_USUB  */ ast_unop_delete,
    /* AST_NOT   */ ast_unop_delete,
    /* AST_COMPL */ ast_unop_delete,

    /* AST_ADD */ ast_binop_delete,
    /* AST_SUB */ ast_binop_delete,
    /* AST_MUL */ ast_binop_delete,
    /* AST_DIV */ ast_binop_delete,
    /* AST_MOD */ ast_binop_delete,

    /* AST_AND */ ast_binop_delete,
    /* AST_XOR */ ast_binop_delete,
    /* AST_OR  */ ast_binop_delete,
    /* AST_SHL */ ast_binop_delete,
    /* AST_SHR */ ast_binop_delete,

    /* AST_EQ  */ ast_binop_delete,
    /* AST_NEQ */ ast_binop_delete,
    /* AST_LT  */ ast_binop_delete,
    /* AST_GT  */ ast_binop_delete,
    /* AST_LE  */ ast_binop_delete,
    /* AST_GE  */ ast_binop_delete,

    /* AST_AND_COND */ ast_binop_delete,
    /* AST_OR_COND  */ ast_binop_delete
};

/*
 * AST printing.
 */
void ast_print(struct ast *ast)
{
    char buf[BUFSIZ];
    size_t len = ast_snprint(ast, buf, sizeof(buf));
    if (len < sizeof(buf)) {
        fwrite(buf, sizeof(char), len, stdout);
    } else {
        char *p = malloc(len+1);
        if (p) {
            if (ast_snprint(ast, p, len+1) == len) {
                fwrite(p, sizeof(char), len, stdout);
            } else {
                errf("ast: inconsistent ast_snprint() results");
            }
            free(p);
        } else {
            errf("ast: out-of-memory");
        }
    }
}

static size_t ast_value_snprint(struct ast *this, char *out, size_t size)
{
    size_t len = 0;
    struct value *value = &((struct ast_value *)this)->value;
    const char *type = value_type_to_string(this->value_type);

    if (size && len < size-1)
        len += snprintf(out+len, size-len, "(%s)", type);
    else len += snprintf(NULL, 0, "(%s)", type);

    if (size && len < size-1)
        len += value_to_string(value, out+len, size-len);
    else len += value_to_string(value, NULL, 0);

    return len;
}

static size_t ast_var_snprint(struct ast *this, char *out, size_t size)
{
    struct ast_var *var = (struct ast_var *)this;
    const char *name = symbol_name(var->symtab->symbols[var->sym]);
    return snprintf(out, size, "%s", name);
}

static size_t ast_cast_snprint(struct ast *this, char *out, size_t size)
{
    const char *type;
    struct ast *child = ((struct ast_unop *)this)->child;
    size_t len = 0;
    if (size && len < size-1) {
        len += ast_snprint(child, out, size);
    } else len += ast_snprint(child, NULL, 0);

    type = value_type_to_string(this->value_type);
    if (size && len < size-1)
        len += snprintf(out+len, size-len, " (%s)", type);
    else len += snprintf(NULL, 0, " (%s)", type);

    return len;
}

static size_t ast_unop_snprint(struct ast *this, const char *op,
                               char *out, size_t size)
{
    size_t len = 0;
    struct ast *child = ((struct ast_unop *)this)->child;
    if (size && len < size-1) {
        len += ast_snprint(child, out, size);
    } else len += ast_snprint(child, NULL, 0);
    if (size && len < size-1)
        len += snprintf(out+len, size-len, " %s", op);
    else len += snprintf(NULL, 0, " %s", op);
    return len;
}

static size_t ast_binop_snprint(struct ast *this, const char *op,
                                char *out, size_t size)
{
    size_t len = 0;
    struct ast *left = ((struct ast_binop *)this)->left;
    struct ast *right = ((struct ast_binop *)this)->right;

    if (size && len < size-1)
        len += ast_snprint(left, out, size);
    else len += ast_snprint(left, NULL, 0);

    if (size && len < size-1)
        len += snprintf(out+len, size-len, " ");
    else len += snprintf(NULL, 0, " ");

    if (size && len < size-1)
        len += ast_snprint(right, out+len, size-len);
    else len += ast_snprint(right, NULL, 0);

    if (size && len < size-1)
        len += snprintf(out+len, size-len, " %s", op);
    else len += snprintf(NULL, 0, " %s", op);

    return len;
}

static size_t ast_usub_snprint(struct ast *this, char *out, size_t size)
{
    return ast_unop_snprint(this, "u-", out, size);
}

static size_t ast_not_snprint(struct ast *this, char *out, size_t size)
{
    return ast_unop_snprint(this, "!", out, size);
}

static size_t ast_compl_snprint(struct ast *this, char *out, size_t size)
{
    return ast_unop_snprint(this, "~", out, size);
}

static size_t ast_add_snprint(struct ast *this, char *out, size_t size)
{
    return ast_binop_snprint(this, "+", out, size);
}

static size_t ast_sub_snprint(struct ast *this, char *out, size_t size)
{
    return ast_binop_snprint(this, "-", out, size);
}

static size_t ast_mul_snprint(struct ast *this, char *out, size_t size)
{
    return ast_binop_snprint(this, "*", out, size);
}

static size_t ast_div_snprint(struct ast *this, char *out, size_t size)
{
    return ast_binop_snprint(this, "/", out, size);
}

static size_t ast_mod_snprint(struct ast *this, char *out, size_t size)
{
    return ast_binop_snprint(this, "%", out, size);
}

static size_t ast_and_snprint(struct ast *this, char *out, size_t size)
{
    return ast_binop_snprint(this, "&", out, size);
}

static size_t ast_xor_snprint(struct ast *this, char *out, size_t size)
{
    return ast_binop_snprint(this, "^", out, size);
}

static size_t ast_or_snprint(struct ast *this, char *out, size_t size)
{
    return ast_binop_snprint(this, "|", out, size);
}

static size_t ast_shl_snprint(struct ast *this, char *out, size_t size)
{
    return ast_binop_snprint(this, "<<", out, size);
}

static size_t ast_shr_snprint(struct ast *this, char *out, size_t size)
{
    return ast_binop_snprint(this, ">>", out, size);
}

static size_t ast_eq_snprint(struct ast *this, char *out, size_t size)
{
    return ast_binop_snprint(this, "==", out, size);
}

static size_t ast_neq_snprint(struct ast *this, char *out, size_t size)
{
    return ast_binop_snprint(this, "!=", out, size);
}

static size_t ast_lt_snprint(struct ast *this, char *out, size_t size)
{
    return ast_binop_snprint(this, "<", out, size);
}

static size_t ast_gt_snprint(struct ast *this, char *out, size_t size)
{
    return ast_binop_snprint(this, ">", out, size);
}

static size_t ast_le_snprint(struct ast *this, char *out, size_t size)
{
    return ast_binop_snprint(this, "<=", out, size);
}

static size_t ast_ge_snprint(struct ast *this, char *out, size_t size)
{
    return ast_binop_snprint(this, ">=", out, size);
}

static size_t ast_and_cond_snprint(struct ast *this, char *out, size_t size)
{
    return ast_binop_snprint(this, "&&", out, size);
}

static size_t ast_or_cond_snprint(struct ast *this, char *out, size_t size)
{
    return ast_binop_snprint(this, "||", out, size);
}

size_t (*ast_snprint_funcs[AST_TYPES])(struct ast *, char *, size_t) = {
    /* AST_VALUE */ ast_value_snprint,
    /* AST_VAR   */ ast_var_snprint,

    /* AST_CAST  */ ast_cast_snprint,
    /* AST_USUB  */ ast_usub_snprint,
    /* AST_NOT   */ ast_not_snprint,
    /* AST_COMPL */ ast_compl_snprint,

    /* AST_ADD */ ast_add_snprint,
    /* AST_SUB */ ast_sub_snprint,
    /* AST_MUL */ ast_mul_snprint,
    /* AST_DIV */ ast_div_snprint,
    /* AST_MOD */ ast_mod_snprint,

    /* AST_AND */ ast_and_snprint,
    /* AST_XOR */ ast_xor_snprint,
    /* AST_OR  */ ast_or_snprint,
    /* AST_SHL */ ast_shl_snprint,
    /* AST_SHR */ ast_shr_snprint,

    /* AST_EQ  */ ast_eq_snprint,
    /* AST_NEQ */ ast_neq_snprint,
    /* AST_LT  */ ast_lt_snprint,
    /* AST_GT  */ ast_gt_snprint,
    /* AST_LE  */ ast_le_snprint,
    /* AST_GE  */ ast_ge_snprint,

    /* AST_AND_COND */ ast_and_cond_snprint,
    /* AST_OR_COND  */ ast_or_cond_snprint
};
