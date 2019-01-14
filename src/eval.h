#ifndef EVAL_H_INCLUDED
#define EVAL_H_INCLUDED

#include "parse.h"
#include "value.h"

#include <stdint.h>

/**
 * Evaluate (interpret) AST and store result to *out.
 */
/*int ast_evaluate(struct ast *ast, struct value *out);*/
#define ast_evaluate(ast, out) ((ast)->vtable->evaluate((ast), (out)))

/**
 * Do not call these functions directly. Use the ast_evaluate macro.
 */

int ast_int_evaluate(struct ast *this, struct value *out);
int ast_uint_evaluate(struct ast *this, struct value *out);
int ast_float_evaluate(struct ast *this, struct value *out);
int ast_value_evaluate(struct ast *this, struct value *out);
int ast_var_evaluate(struct ast *this, struct value *out);

int ast_add_evaluate(struct ast *this, struct value *out);
int ast_sub_evaluate(struct ast *this, struct value *out);
int ast_mul_evaluate(struct ast *this, struct value *out);
int ast_div_evaluate(struct ast *this, struct value *out);
int ast_mod_evaluate(struct ast *this, struct value *out);

int ast_and_evaluate(struct ast *this, struct value *out);
int ast_xor_evaluate(struct ast *this, struct value *out);
int ast_or_evaluate(struct ast *this, struct value *out);
int ast_shl_evaluate(struct ast *this, struct value *out);
int ast_shr_evaluate(struct ast *this, struct value *out);

int ast_cast_evaluate(struct ast *this, struct value *out);
int ast_uadd_evaluate(struct ast *this, struct value *out);
int ast_usub_evaluate(struct ast *this, struct value *out);
int ast_not_evaluate(struct ast *this, struct value *out);
int ast_compl_evaluate(struct ast *this, struct value *out);

int ast_eq_evaluate(struct ast *this, struct value *out);
int ast_neq_evaluate(struct ast *this, struct value *out);
int ast_lt_evaluate(struct ast *this, struct value *out);
int ast_gt_evaluate(struct ast *this, struct value *out);
int ast_le_evaluate(struct ast *this, struct value *out);
int ast_ge_evaluate(struct ast *this, struct value *out);

int ast_and_cond_evaluate(struct ast *this, struct value *out);
int ast_or_cond_evaluate(struct ast *this, struct value *out);

#endif
