#ifndef AST_H_INCLUDED
#define AST_H_INCLUDED

#include "value.h"

#include <stddef.h>

/*
 * Abstract syntax tree node types.
 * The order must match lex token types defined in lex.h.
 */
#define lex_to_ast_type(lex_token_type) ((lex_token_type) + AST_ADD-LEX_ADD)
enum ast_type {
    AST_INT=0, AST_UINT, AST_FLOAT, AST_VALUE, AST_VAR,

    /* Binary operators */
    AST_ADD, AST_SUB, AST_MUL, AST_DIV, AST_MOD,
    
    /* Bitwise operators */
    AST_AND, AST_XOR, AST_OR, AST_SHL, AST_SHR,

    /* Unary opertators */
    AST_CAST, AST_UADD, AST_USUB, AST_NOT, AST_COMPL,

    /* Relational operators */
    AST_EQ, AST_NEQ, AST_LT, AST_GT, AST_LE, AST_GE,

    /* Conditional operators */
    AST_AND_COND, AST_OR_COND,

    AST_TYPES
};

/*
 * AST structures.
 */
struct ast {
    enum ast_type node_type;
    enum value_type value_type; /* Filled by parser. */
};

#define ast_is_constant(t) ((t)->node_type < AST_VAR)

struct ast_binop {
    struct ast root;
    struct ast *left, *right;
};

struct ast_unop {
    struct ast root;
    struct ast *child;
};

/*
 * AST nodes.
 */
struct ast_int   { struct ast root; intmax_t value; };
struct ast_uint  { struct ast root; uintmax_t value; };
struct ast_float { struct ast root; double value; };
struct ast_value { struct ast root; struct value value; };
struct ast_var {
    struct ast root;
    struct value *value;
    const char *identifier;
};

struct ast_add   { struct ast_binop root; };
struct ast_sub   { struct ast_binop root; };
struct ast_mul   { struct ast_binop root; };
struct ast_div   { struct ast_binop root; };

struct ast_and   { struct ast_binop root; };
struct ast_xor   { struct ast_binop root; };
struct ast_or    { struct ast_binop root; };
struct ast_shl   { struct ast_binop root; };
struct ast_shr   { struct ast_binop root; };

struct ast_cast  { struct ast_unop root; };
struct ast_uadd  { struct ast_unop root; };
struct ast_usub  { struct ast_unop root; };
struct ast_not   { struct ast_unop root; };
struct ast_compl { struct ast_unop root; };

struct ast_eq    { struct ast_binop root; };
struct ast_neq   { struct ast_binop root; };
struct ast_lt    { struct ast_binop root; };
struct ast_gt    { struct ast_binop root; };
struct ast_le    { struct ast_binop root; };
struct ast_ge    { struct ast_binop root; };

struct ast_and_cond { struct ast_binop root; };
struct ast_or_cond  { struct ast_binop root; };

/*
 * Routines for allocating & initializing AST nodes.
 */
struct ast *ast_int_new(intmax_t value);
struct ast *ast_uint_new(uintmax_t value);
struct ast *ast_float_new(double value);
struct ast *ast_value_new(struct value *value);
struct ast *ast_var_new(const char *identifier, struct value *value);

struct ast *ast_binop_new(enum ast_type node_type,
                         struct ast *left, struct ast *right);
#define ast_add_new(l, r) ast_binop_new(AST_ADD, (l), (r))
#define ast_sub_new(l, r) ast_binop_new(AST_SUB, (l), (r))
#define ast_mul_new(l, r) ast_binop_new(AST_MUL, (l), (r))
#define ast_div_new(l, r) ast_binop_new(AST_DIV, (l), (r))
#define ast_and_new(l, r) ast_binop_new(AST_AND, (l), (r))
#define ast_xor_new(l, r) ast_binop_new(AST_XOR, (l), (r))
#define ast_or_new(l, r)  ast_binop_new(AST_OR, (l), (r))
#define ast_shl_new(l, r) ast_binop_new(AST_SHL, (l), (r))
#define ast_shr_new(l, r) ast_binop_new(AST_SHR, (l), (r))

struct ast *ast_cast_new(enum value_type value_type, struct ast *child);

struct ast *ast_unop_new(enum ast_type node_type, struct ast *child);
#define ast_uadd_new(c)  ast_unop_new(AST_UADD, (c))
#define ast_usub_new(c)  ast_unop_new(AST_USUB, (c))
#define ast_not_new(c)   ast_unop_new(AST_NOT, (c))
#define ast_compl_new(c) ast_unop_new(AST_COMPL, (c))

struct ast *ast_rel_new(enum ast_type node_type,
        struct ast *left, struct ast *right);
#define ast_eq_new(l, r)  ast_rel_new(AST_EQ, (l), (r))
#define ast_neq_new(l, r) ast_rel_new(AST_NEQ, (l), (r))
#define ast_lt_new(l, r)  ast_rel_new(AST_LT, (l), (r))
#define ast_gt_new(l, r)  ast_rel_new(AST_GT, (l), (r))
#define ast_le_new(l, r)  ast_rel_new(AST_LE, (l), (r))
#define ast_ge_new(l, r)  ast_rel_new(AST_GE, (l), (r))

struct ast *ast_cond_new(enum ast_type node_type,
        struct ast *left, struct ast *right);
#define ast_and_cond_new(l, r) ast_cond_new(AST_AND_COND, (l), (r))
#define ast_or_cond_new(l, r)  ast_cond_new(AST_OR_COND, (l), (r))

/*
 * Delete an AST node and its children.
 */
extern void (*ast_delete_funcs[AST_TYPES])(struct ast *);
#define ast_delete(ast) (ast_delete_funcs[(ast)->node_type]((ast)))

/*
 * Write a textual representation of an AST node to a string buffer.
 *
 * Behaves similarly to snprintf(3), i.e., truncates the output to `size` bytes
 * and returns the number of characters needed to write the entire text.
 */
extern size_t (*ast_snprint_funcs[AST_TYPES])(struct ast *, char *, size_t);
#define ast_snprint(ast, out, size) \
    (ast_snprint_funcs[(ast)->node_type]((ast), (out), (size)))

/*
 * Print a text of an AST node to standard output.
 */
void ast_print(struct ast *ast);

#endif
