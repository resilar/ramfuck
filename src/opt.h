#ifndef OPTIMIZE_H_INCLUDED
#define OPTIMIZE_H_INCLUDED

#include "ast.h"

/*
 * Functions performing simple optimization on AST nodes.
 * For the time being, only simple constant folding is implemented.
 */
extern struct ast *(*ast_optimize_funcs[AST_TYPES])(struct ast *this); 

/*
 * Optimize an AST.
 * Returns an optimized copy of the passed in AST. Delete the returned AST with
 * ast_delete().
 */
#define ast_optimize(t) (ast_optimize_funcs[(t)->node_type]((t)))

#endif
