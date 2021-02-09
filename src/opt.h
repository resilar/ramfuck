/*
 * Functions performing simple optimization on AST nodes.
 * For the time being, only simple constant folding is implemented.
 */

#ifndef OPTIMIZE_H_INCLUDED
#define OPTIMIZE_H_INCLUDED

#include "ast.h"

/*
 * Optimize an AST.
 *
 * Returns an optimized copy of the passed in AST. ast_delete() must be called
 * for the returned optimized AST as well as the original passed in AST.
 */
extern struct ast *(*ast_optimize_funcs[AST_TYPES])(struct ast *);
#define ast_optimize(ast) (ast_optimize_funcs[(ast)->node_type]((ast)))

#endif
