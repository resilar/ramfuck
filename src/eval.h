#ifndef EVAL_H_INCLUDED
#define EVAL_H_INCLUDED

#include "ast.h"
#include "value.h"

/*
 * Evaluate (interpret) AST and store result to pointed value.
 */
extern int (*ast_evaluate_funcs[AST_TYPES])(struct ast *, struct value *);
#define ast_evaluate(ast, out) \
    (ast_evaluate_funcs[(ast)->node_type]((ast), (out)))

#endif
