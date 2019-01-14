/* Recursive descent expression parser for ramfuck. */
#ifndef PARSE_H_INCLUDED
#define PARSE_H_INCLUDED

#include "ast.h"
#include "lex.h"
#include "symbol.h"

struct parser {
    const char *in;

    struct lex_token *symbol;   /* Symbol being processed. */
    struct lex_token *accepted; /* Last accepted symbol. */
    struct lex_token tokens[2]; /* symbol and accepted fields point here. */

    struct symbol_table *symtab;

    int errors;
};

/*
 * Initialize a parser.
 *
 * Parameter symtab can be NULL.
 *
 * The initialized parser does not have to be freed (caller should handle
 * deallocating the symbol table and closing the input stream if needed).
 */
void parser_init(struct parser *p, struct symbol_table *symtab, const char *in);

/*
 * Parse an expression using parser p.
 *
 * The return value is the resulting abstract syntax tree or NULL if a parse
 * error occurs (p->errors > 0). The returned pointer should be freed with
 * ast_delete().
 */
struct ast *parse_expression(struct parser *p);

#endif
