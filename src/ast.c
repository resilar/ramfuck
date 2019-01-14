#include "ast.h"
#include "eval.h"
#include "opt.h"

#include <stdlib.h>
#include <string.h>

/* Static functions */
static void ast_leaf_delete(struct ast *this);
static void ast_binop_delete(struct ast *this);
static void ast_unop_delete(struct ast *this);

static void ast_int_print(struct ast *this, FILE *out);
static void ast_uint_print(struct ast *this, FILE *out);
static void ast_float_print(struct ast *this, FILE *out);
static void ast_value_print(struct ast *this, FILE *out);
static void ast_var_print(struct ast *this, FILE *out);

static void ast_add_print(struct ast *this, FILE *out);
static void ast_sub_print(struct ast *this, FILE *out);
static void ast_mul_print(struct ast *this, FILE *out);
static void ast_div_print(struct ast *this, FILE *out);
static void ast_mod_print(struct ast *this, FILE *out);

static void ast_and_print(struct ast *this, FILE *out);
static void ast_xor_print(struct ast *this, FILE *out);
static void ast_or_print(struct ast *this, FILE *out);
static void ast_shl_print(struct ast *this, FILE *out);
static void ast_shr_print(struct ast *this, FILE *out);

static void ast_cast_print(struct ast *this, FILE *out);
static void ast_uadd_print(struct ast *this, FILE *out);
static void ast_usub_print(struct ast *this, FILE *out);
static void ast_not_print(struct ast *this, FILE *out);
static void ast_compl_print(struct ast *this, FILE *out);

static void ast_eq_print(struct ast *this, FILE *out);
static void ast_neq_print(struct ast *this, FILE *out);
static void ast_lt_print(struct ast *this, FILE *out);
static void ast_gt_print(struct ast *this, FILE *out);
static void ast_le_print(struct ast *this, FILE *out);
static void ast_ge_print(struct ast *this, FILE *out);

static void ast_and_cond_print(struct ast *this, FILE *out);
static void ast_or_cond_print(struct ast *this, FILE *out);

/* Vtables for AST nodes */
static const struct ast_operations vtables[AST_TYPES] = {
    /* free, print, evaluate, optimize */

    /* AST_INT   */
    {ast_leaf_delete, ast_int_print, ast_int_evaluate},
    /* AST_UINT  */
    {ast_leaf_delete, ast_uint_print, ast_uint_evaluate},
    /* AST_FLOAT */
    {ast_leaf_delete, ast_float_print, ast_float_evaluate},
    /* AST_VALUE */
    {ast_leaf_delete, ast_value_print, ast_value_evaluate},
    /* AST_VAR   */
    {ast_leaf_delete, ast_var_print, ast_var_evaluate},

    /* AST_ADD */
    {ast_binop_delete, ast_add_print, ast_add_evaluate},
    /* AST_SUB */
    {ast_binop_delete, ast_sub_print, ast_sub_evaluate},
    /* AST_MUL */
    {ast_binop_delete, ast_mul_print, ast_mul_evaluate},
    /* AST_DIV */
    {ast_binop_delete, ast_div_print, ast_div_evaluate},
    /* AST_MOD */
    {ast_binop_delete, ast_mod_print, ast_mod_evaluate},

    /* AST_AND */
    {ast_binop_delete, ast_and_print, ast_and_evaluate},
    /* AST_XOR */
    {ast_binop_delete, ast_xor_print, ast_xor_evaluate},
    /* AST_OR  */
    {ast_binop_delete, ast_or_print, ast_or_evaluate},
    /* AST_SHL */
    {ast_binop_delete, ast_shl_print, ast_shl_evaluate},
    /* AST_SHR */
    {ast_binop_delete, ast_shr_print, ast_shr_evaluate},

    /* AST_CAST  */
    {ast_unop_delete, ast_cast_print, ast_cast_evaluate},
    /* AST_UADD  */
    {ast_unop_delete, ast_uadd_print, ast_uadd_evaluate},
    /* AST_USUB  */
    {ast_unop_delete, ast_usub_print, ast_usub_evaluate},
    /* AST_NOT   */
    {ast_unop_delete, ast_not_print, ast_not_evaluate},
    /* AST_COMPL */
    {ast_unop_delete, ast_compl_print, ast_compl_evaluate},

    /* AST_EQ  */
    {ast_binop_delete, ast_eq_print, ast_eq_evaluate},
    /* AST_NEQ */
    {ast_binop_delete, ast_neq_print, ast_neq_evaluate},
    /* AST_LT  */
    {ast_binop_delete, ast_lt_print, ast_lt_evaluate},
    /* AST_GT  */
    {ast_binop_delete, ast_gt_print, ast_gt_evaluate},
    /* AST_LE  */
    {ast_binop_delete, ast_le_print, ast_le_evaluate},
    /* AST_GE  */
    {ast_binop_delete, ast_ge_print, ast_ge_evaluate},

    /* AST_AND_COND */
    {ast_binop_delete, ast_and_cond_print, ast_and_cond_evaluate},
    /* AST_OR_COND  */
    {ast_binop_delete, ast_or_cond_print, ast_or_cond_evaluate}
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
 * Print.
 */
static void ast_binop_print(struct ast *this, FILE *out)
{
    struct ast *left, *right;
    left = ((struct ast_binop *)this)->left;
    right = ((struct ast_binop *)this)->right;
    left->vtable->print(left, out);
    fputc(' ', out);
    right->vtable->print(right, out);
    fputc(' ', out);
}

static void ast_unop_print(struct ast *this, FILE *out)
{
    struct ast *child;
    child = ((struct ast_unop *)this)->child;
    child->vtable->print(child, out);
    fputc(' ', out);
}

static void ast_int_print(struct ast *this, FILE *out)
{
    fprintf(out, "%ld", (long int)((struct ast_int *)this)->value);
}

static void ast_uint_print(struct ast *this, FILE *out)
{
    fprintf(out, "%lu", (unsigned long int)((struct ast_uint *)this)->value);
}

static void ast_float_print(struct ast *this, FILE *out)
{
    fprintf(out, "%f", ((struct ast_float *)this)->value);
}

static void ast_value_print(struct ast *this, FILE *out)
{
    char node_type[32], value[32];
    value_type_to_string_r(this->value_type, node_type);
    value_to_string_r(&((struct ast_value *)this)->value, value);
    fprintf(out, "(%s)%s", node_type, value);
}

static void ast_var_print(struct ast *this, FILE *out)
{
    fprintf(out, "%s", ((struct ast_var *)this)->identifier);
}

static void ast_add_print(struct ast *this, FILE *out)
{
    ast_binop_print(this, out);
    fputc('+', out);
}

static void ast_sub_print(struct ast *this, FILE *out)
{
    ast_binop_print(this, out);
    fputc('-', out);
}

static void ast_mul_print(struct ast *this, FILE *out)
{
    ast_binop_print(this, out);
    fputc('*', out);
}

static void ast_div_print(struct ast *this, FILE *out)
{
    ast_binop_print(this, out);
    fputc('/', out);
}

static void ast_mod_print(struct ast *this, FILE *out)
{
    ast_binop_print(this, out);
    fputc('%', out);
}

static void ast_and_print(struct ast *this, FILE *out)
{
    ast_binop_print(this, out);
    fprintf(out, "&");
}

static void ast_xor_print(struct ast *this, FILE *out)
{
    ast_binop_print(this, out);
    fprintf(out, "^");
}

static void ast_or_print(struct ast *this, FILE *out)
{
    ast_binop_print(this, out);
    fprintf(out, "|");
}

static void ast_shl_print(struct ast *this, FILE *out)
{
    ast_binop_print(this, out);
    fprintf(out, "<<");
}

static void ast_shr_print(struct ast *this, FILE *out)
{
    ast_binop_print(this, out);
    fprintf(out, ">>");
}

static void ast_cast_print(struct ast *this, FILE *out)
{
    ast_unop_print(this, out);
    fprintf(out, "(T)");
}

static void ast_uadd_print(struct ast *this, FILE *out)
{
    ast_unop_print(this, out);
    fprintf(out, "u+");
}

static void ast_usub_print(struct ast *this, FILE *out)
{
    ast_unop_print(this, out);
    fprintf(out, "u-");
}

static void ast_not_print(struct ast *this, FILE *out)
{
    ast_unop_print(this, out);
    fputc('!', out);
}

static void ast_compl_print(struct ast *this, FILE *out)
{
    ast_unop_print(this, out);
    fputc('~', out);
}

static void ast_eq_print(struct ast *this, FILE *out)
{
    ast_binop_print(this, out);
    fprintf(out, "==");
}

static void ast_neq_print(struct ast *this, FILE *out)
{
    ast_binop_print(this, out);
    fprintf(out, "!=");
}

static void ast_lt_print(struct ast *this, FILE *out)
{
    ast_binop_print(this, out);
    fputc('<', out);
}

static void ast_gt_print(struct ast *this, FILE *out)
{
    ast_binop_print(this, out);
    fputc('>', out);
}

static void ast_le_print(struct ast *this, FILE *out)
{
    ast_binop_print(this, out);
    fprintf(out, "<=");
}

static void ast_ge_print(struct ast *this, FILE *out)
{
    ast_binop_print(this, out);
    fprintf(out, ">=");
}

static void ast_and_cond_print(struct ast *this, FILE *out)
{
    ast_binop_print(this, out);
    fprintf(out, "&&");
}

static void ast_or_cond_print(struct ast *this, FILE *out)
{
    ast_binop_print(this, out);
    fprintf(out, "||");
}
