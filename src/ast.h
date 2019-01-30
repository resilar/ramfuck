/*
 * Abstract Syntax Tree (AST).
 */

#ifndef AST_H_INCLUDED
#define AST_H_INCLUDED

#include "value.h"

#include <stddef.h>

/*
 * AST node types.
 *
 * The ordering must match lex token types defined in lex.h file.
 */
enum ast_type {
    AST_VALUE=0, AST_VAR,

    /* Unary operators */
    AST_CAST, AST_DEREF, AST_NEG, AST_NOT, AST_COMPL,

    /* Binary operators */
    AST_ADD, AST_SUB, AST_MUL, AST_DIV, AST_MOD,
    
    /* Bitwise operators */
    AST_AND, AST_XOR, AST_OR, AST_SHL, AST_SHR,

    /* Relational operators */
    AST_EQ, AST_NEQ, AST_LT, AST_GT, AST_LE, AST_GE,

    /* Conditional operators */
    AST_AND_COND, AST_OR_COND,

    AST_TYPES
};
#define lex_to_ast_type(lex_token_type) ((lex_token_type) + AST_ADD-LEX_ADD)
#define ast_to_lex_type(ast_token_type) ((ast_token_type) + LEX_ADD-AST_ADD)

/*
 * AST structures.
 */
struct ast {
    enum ast_type node_type;
    enum value_type value_type;
};

#define ast_is_constant(ast) ((ast)->node_type < AST_VAR)
#define ast_is_compare(ast) ((ast)->node_type >= AST_EQ)
#define ast_type_is_compare(type) ((type) >= AST_EQ)
#define ast_type_is_conditional(type) ((type) >= AST_AND_COND)

struct ast_binary {
    struct ast tree;
    struct ast *left, *right;
};

struct ast_unary {
    struct ast tree;
    struct ast *child;
};

/*
 * AST nodes.
 */
struct ast_value {
    struct ast tree;
    struct value value;
};

struct ast_var {
    struct ast tree;
    struct symbol_table *symtab;
    size_t sym;
    size_t size;
};

struct ast_cast  { struct ast_unary tree; };

struct ast_deref {
    struct ast_unary tree;
    struct target *target;
};

struct ast_neg   { struct ast_unary tree; };
struct ast_not   { struct ast_unary tree; };
struct ast_compl { struct ast_unary tree; };

struct ast_add { struct ast_binary tree; };
struct ast_sub { struct ast_binary tree; };
struct ast_mul { struct ast_binary tree; };
struct ast_div { struct ast_binary tree; };

struct ast_and { struct ast_binary tree; };
struct ast_xor { struct ast_binary tree; };
struct ast_or  { struct ast_binary tree; };
struct ast_shl { struct ast_binary tree; };
struct ast_shr { struct ast_binary tree; };

struct ast_eq  { struct ast_binary tree; };
struct ast_neq { struct ast_binary tree; };
struct ast_lt  { struct ast_binary tree; };
struct ast_gt  { struct ast_binary tree; };
struct ast_le  { struct ast_binary tree; };
struct ast_ge  { struct ast_binary tree; };

struct ast_and_cond { struct ast_binary tree; };
struct ast_or_cond  { struct ast_binary tree; };

/*
 * Routines and macros for allocating & initializing AST nodes.
 */
struct ast *ast_value_new(struct value *value);
struct ast *ast_var_new(struct symbol_table *symtab, size_t sym, size_t size);

struct ast *ast_cast_new(enum value_type value_type, struct ast *child);
struct ast *ast_deref_new(struct ast *child, enum value_type value_type,
                          struct target *target);

struct ast *ast_unary_new(enum ast_type node_type, struct ast *child);
#define ast_neg_new(c)   ast_unary_new(AST_NEG, (c))
#define ast_not_new(c)   ast_unary_new(AST_NOT, (c))
#define ast_compl_new(c) ast_unary_new(AST_COMPL, (c))

struct ast *ast_binary_new(enum ast_type node_type,
                           struct ast *left, struct ast *right);
#define ast_add_new(l, r) ast_binary_new(AST_ADD, (l), (r))
#define ast_sub_new(l, r) ast_binary_new(AST_SUB, (l), (r))
#define ast_mul_new(l, r) ast_binary_new(AST_MUL, (l), (r))
#define ast_div_new(l, r) ast_binary_new(AST_DIV, (l), (r))
#define ast_and_new(l, r) ast_binary_new(AST_AND, (l), (r))
#define ast_xor_new(l, r) ast_binary_new(AST_XOR, (l), (r))
#define ast_or_new(l, r)  ast_binary_new(AST_OR, (l), (r))
#define ast_shl_new(l, r) ast_binary_new(AST_SHL, (l), (r))
#define ast_shr_new(l, r) ast_binary_new(AST_SHR, (l), (r))

#define ast_eq_new(l, r)  ast_binary_new(AST_EQ, (l), (r))
#define ast_neq_new(l, r) ast_binary_new(AST_NEQ, (l), (r))
#define ast_lt_new(l, r)  ast_binary_new(AST_LT, (l), (r))
#define ast_gt_new(l, r)  ast_binary_new(AST_GT, (l), (r))
#define ast_le_new(l, r)  ast_binary_new(AST_LE, (l), (r))
#define ast_ge_new(l, r)  ast_binary_new(AST_GE, (l), (r))

#define ast_and_cond_new(l, r) ast_binary_new(AST_AND_COND, (l), (r))
#define ast_or_cond_new(l, r)  ast_binary_new(AST_OR_COND, (l), (r))

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
