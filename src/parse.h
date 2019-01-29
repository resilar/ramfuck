/* Recursive descent expression parser for ramfuck. */
#ifndef PARSE_H_INCLUDED
#define PARSE_H_INCLUDED

#include "ast.h"
#include "lex.h"
#include "symbol.h"
#include "value.h"

struct parser {
    const char *in;
    const char *end;
    struct symbol_table *symtab;
    int quiet;
    int errors;
    enum value_type addr_type;
    struct target *target;
    int has_deref;

    /* Internal */
    struct lex_token *symbol;   /* symbol being processed */
    struct lex_token *accepted; /* last accepted symbol */
    struct lex_token tokens[2]; /* symbol and accepted fields point here */
};

/*
 * Initialize parser.
 *
 * The initialized parser does not require destroying.
 */
void parser_init(struct parser *parser);

/*
 * Parse an expression using symbol table to produce an abstract syntax tree.
 *
 * Returns the number of errors, i.e., zero if successful. Upon return `*pout`,
 * if non-NULL, receives a pointer to the parsed AST (or NULL on parse error).
 * The received AST pointer `*pout` must be deleted with ast_delete().
 */
struct ast *parse_expression(struct parser *p, const char *in);

#endif
