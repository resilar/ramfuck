#define _DEFAULT_SOURCE /* for snprintf(3) */
#include "ast.h"
#include "ramfuck.h"

#include "eval.h"
#include "opt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Static functions */
static void ast_leaf_delete(struct ast *this);
static void ast_binop_delete(struct ast *this);
static void ast_unop_delete(struct ast *this);

/* Vtables for AST nodes */
static const struct ast_operations vtables[AST_TYPES] = {
    /* free, evaluate */

    /* AST_INT   */
    {ast_leaf_delete, ast_int_evaluate},
    /* AST_UINT  */
    {ast_leaf_delete, ast_uint_evaluate},
    /* AST_FLOAT */
    {ast_leaf_delete, ast_float_evaluate},
    /* AST_VALUE */
    {ast_leaf_delete, ast_value_evaluate},
    /* AST_VAR   */
    {ast_leaf_delete, ast_var_evaluate},

    /* AST_ADD */
    {ast_binop_delete, ast_add_evaluate},
    /* AST_SUB */
    {ast_binop_delete, ast_sub_evaluate},
    /* AST_MUL */
    {ast_binop_delete, ast_mul_evaluate},
    /* AST_DIV */
    {ast_binop_delete, ast_div_evaluate},
    /* AST_MOD */
    {ast_binop_delete, ast_mod_evaluate},

    /* AST_AND */
    {ast_binop_delete, ast_and_evaluate},
    /* AST_XOR */
    {ast_binop_delete, ast_xor_evaluate},
    /* AST_OR  */
    {ast_binop_delete, ast_or_evaluate},
    /* AST_SHL */
    {ast_binop_delete, ast_shl_evaluate},
    /* AST_SHR */
    {ast_binop_delete, ast_shr_evaluate},

    /* AST_CAST  */
    {ast_unop_delete, ast_cast_evaluate},
    /* AST_UADD  */
    {ast_unop_delete, ast_uadd_evaluate},
    /* AST_USUB  */
    {ast_unop_delete, ast_usub_evaluate},
    /* AST_NOT   */
    {ast_unop_delete, ast_not_evaluate},
    /* AST_COMPL */
    {ast_unop_delete, ast_compl_evaluate},

    /* AST_EQ  */
    {ast_binop_delete, ast_eq_evaluate},
    /* AST_NEQ */
    {ast_binop_delete, ast_neq_evaluate},
    /* AST_LT  */
    {ast_binop_delete, ast_lt_evaluate},
    /* AST_GT  */
    {ast_binop_delete, ast_gt_evaluate},
    /* AST_LE  */
    {ast_binop_delete, ast_le_evaluate},
    /* AST_GE  */
    {ast_binop_delete, ast_ge_evaluate},

    /* AST_AND_COND */
    {ast_binop_delete, ast_and_cond_evaluate},
    /* AST_OR_COND  */
    {ast_binop_delete, ast_or_cond_evaluate}
};

int ast_is_type(struct ast *ast, enum ast_type node_type)
{
    if (node_type >= AST_TYPES)
        return 0;
    return ast->vtable == &vtables[node_type];
}

/*
 * AST allocation and initialization functions.
 */
struct ast *ast_int_new(intmax_t value)
{
    struct ast_int *n;
    if ((n = malloc(sizeof(struct ast_int)))) {
        n->root.node_type = AST_INT;
        n->root.vtable = &vtables[n->root.node_type];
        n->value = value;
    }
    return (struct ast *)n;
}

struct ast *ast_uint_new(uintmax_t value)
{
    struct ast_uint *n;
    if ((n = malloc(sizeof(struct ast_uint)))) {
        n->root.node_type = AST_UINT;
        n->root.vtable = &vtables[n->root.node_type];
        n->value = value;
    }
    return (struct ast *)n;
}


struct ast *ast_float_new(double value)
{
    struct ast_float *n;
    if ((n = malloc(sizeof(struct ast_float)))) {
        n->root.node_type = AST_FLOAT;
        n->root.vtable = &vtables[n->root.node_type];
        n->value = value;
    }
    return (struct ast *)n;
}

struct ast *ast_value_new(struct value *value)
{
    struct ast_value *n;
    if ((n = malloc(sizeof(struct ast_value)))) {
        n->root.node_type = AST_VALUE;
        n->root.vtable = &vtables[n->root.node_type];
        n->root.value_type = value->type;
        n->value = *value;
    }
    return (struct ast *)n;
}

struct ast *ast_var_new(const char *identifier, struct value *value)
{
    struct ast_var *n;
    size_t len = strlen(identifier);
    if ((n = malloc(sizeof(struct ast_var) + len + 1))) {
        n->root.node_type = AST_VAR;
        n->root.vtable = &vtables[n->root.node_type];
        strcpy((char *)n + sizeof(struct ast_var), identifier);
        n->identifier = (const char *)n + sizeof(struct ast_var);
        n->value = value;
    }
    return (struct ast *)n;
}


struct ast *ast_binop_new(enum ast_type node_type,
                          struct ast *left, struct ast *right)
{
    struct ast_binop *n;
    if ((n = malloc(sizeof(struct ast_binop)))) {
        n->root.node_type = node_type;
        n->root.vtable = &vtables[node_type];
        n->left = left;
        n->right = right;
    }
    return (struct ast *)n;
}

struct ast *ast_cast_new(enum value_type value_type, struct ast *child)
{
    struct ast_unop *n;
    if ((n = malloc(sizeof(struct ast_cast)))) {
        n->root.node_type = AST_CAST;
        n->root.value_type = value_type;
        n->root.vtable = &vtables[AST_CAST];
        n->child = child;
    }
    return (struct ast *)n;
}

struct ast *ast_unop_new(enum ast_type node_type, struct ast *child)
{
    struct ast_unop *n;
    if ((n = malloc(sizeof(struct ast_unop)))) {
        n->root.node_type = node_type;
        n->root.vtable = &vtables[node_type];
        n->child = child;
    }
    return (struct ast *)n;
}

struct ast *ast_rel_new(enum ast_type node_type,
        struct ast *left, struct ast *right)
{
    return ast_binop_new(node_type, left, right);
}

struct ast *ast_cond_new(enum ast_type node_type,
        struct ast *left, struct ast *right)
{
    return ast_binop_new(node_type, left, right);
}

/*
 * Delete.
 */
static void ast_leaf_delete(struct ast *this)
{
    free(this);
}

static void ast_binop_delete(struct ast *this)
{
    struct ast *left, *right;
    left = ((struct ast_binop *)this)->left;
    right = ((struct ast_binop *)this)->right;
    left->vtable->delete(left);
    right->vtable->delete(right);
    free(this);
}

static void ast_unop_delete(struct ast *this)
{
    struct ast *child;
    child = ((struct ast_unop *)this)->child;
    child->vtable->delete(child);
    free(this);
}

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
            if (ast_snprint(ast, p, len+1) <= len) {
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

static size_t ast_int_snprint(struct ast *this, char *out, size_t size)
{
    return snprintf(out, size, "%ld", (long)((struct ast_int *)this)->value);
}

static size_t ast_uint_snprint(struct ast *this, char *out, size_t size)
{
    return snprintf(out, size, "%lu",
                    (unsigned long)((struct ast_uint *)this)->value);
}

static size_t ast_float_snprint(struct ast *this, char *out, size_t size)
{
    return snprintf(out, size, "%f", ((struct ast_float *)this)->value);
}

static size_t ast_value_snprint(struct ast *this, char *out, size_t size)
{
    size_t len = 0;
    struct value *value = &((struct ast_value *)this)->value;

    if (size && len < size-1)
        len += snprintf(out+len, size-len, "(");
    else len += snprintf(NULL, 0, "(");

    if (size && len < size-1)
        len += value_type_to_string_r(this->value_type, out+len, size-len);
    else len += value_type_to_string_r(this->value_type, NULL, 0);

    if (size && len < size-1)
        len += snprintf(out+len, size-len, ")");
    else len += snprintf(NULL, 0, ")");

    if (size && len < size-1)
        len += value_to_string_r(value, out+len, size-len);
    else len += value_to_string_r(value, NULL, 0);

    return len;
}

static size_t ast_var_snprint(struct ast *this, char *out, size_t size)
{
    return snprintf(out, size, "%s", ((struct ast_var *)this)->identifier);
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

static size_t ast_cast_snprint(struct ast *this, char *out, size_t size)
{
    size_t len = ast_unop_snprint(this, "(", out, size);

    if (size && len < size-1)
        len += value_type_to_string_r(this->value_type, out+len, size-len);
    else len += value_type_to_string_r(this->value_type, NULL, 0);

    if (size && len < size-1)
        len += snprintf(out+len, size-len, ")");
    else len += snprintf(NULL, 0, ")");

    return len;
}

static size_t ast_uadd_snprint(struct ast *this, char *out, size_t size)
{
    return ast_unop_snprint(this, "u+", out, size);
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

size_t (*ast_snprint_funcs[AST_TYPES])(struct ast *, char *out, size_t size) = {
    /* AST_INT   */ ast_int_snprint,
    /* AST_UINT  */ ast_uint_snprint,
    /* AST_FLOAT */ ast_float_snprint,
    /* AST_VALUE */ ast_value_snprint,
    /* AST_VAR   */ ast_var_snprint,

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

    /* AST_CAST  */ ast_cast_snprint,
    /* AST_UADD  */ ast_uadd_snprint,
    /* AST_USUB  */ ast_usub_snprint,
    /* AST_NOT   */ ast_not_snprint,
    /* AST_COMPL */ ast_compl_snprint,

    /* AST_EQ  */ ast_eq_snprint,
    /* AST_NEQ */ ast_neq_snprint,
    /* AST_LT  */ ast_lt_snprint,
    /* AST_GT  */ ast_gt_snprint,
    /* AST_LE  */ ast_le_snprint,
    /* AST_GE  */ ast_ge_snprint,

    /* AST_AND_COND */ ast_and_cond_snprint,
    /* AST_OR_COND  */ ast_or_cond_snprint
};
