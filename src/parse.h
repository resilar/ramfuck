/* Recursive descent expression parser for ramfuck. */
#ifndef PARSE_H_INCLUDED
#define PARSE_H_INCLUDED

#include "ast.h"
#include "symbol.h"

/*
 * Parse an expression using symbol table to produce an abstract syntax tree.
 *
 * Returns the number of errors, i.e., zero if successful. Upon return `*pout`,
 * if non-NULL, receives a pointer to the parsed AST (or NULL on parse error).
 * The received AST pointer `*pout` must be deleted with ast_delete().
 */
int parse_expression(const char *in, struct symbol_table *symtab,
                     int quiet, struct ast **pout);

#endif
