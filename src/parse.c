#define _DEFAULT_SOURCE /* vfprintf(3) */
#include "parse.h"
#include "ramfuck.h"
#include "value.h"

#include <memory.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void parser_init(struct parser *target)
{
    memset(target, 0, sizeof(struct parser));
    target->addr_type = U32;
    target->symbol = &target->tokens[0];
    target->accepted = &target->tokens[1];
}

static struct ast *expression(struct parser *p);
static struct ast *conditional_expression(struct parser *p);
static struct ast *equality_expression(struct parser *p);
static struct ast *relational_expression(struct parser *p);
static struct ast *or_expression(struct parser *p);
static struct ast *xor_expression(struct parser *p);
static struct ast *and_expression(struct parser *p);
static struct ast *shift_expression(struct parser *p);
static struct ast *addsub_expression(struct parser *p);
static struct ast *muldiv_expression(struct parser *p);
static struct ast *cast_expression(struct parser *p);
static struct ast *unary_expression(struct parser *p);
static struct ast *factor(struct parser *p);

static void parse_error(struct parser *p, const char *format, ...)
{
    if (!p->quiet) {
        va_list args;
        va_start(args, format);
        fputs("parse: ", stderr);
        vfprintf(stderr, format, args);
        fputc('\n', stderr);
        va_end(args);
    }
    p->errors++;
}

static int next_symbol(struct parser *p)
{
    int err = 0;
    do {
        err = !lexer(&p->in, p->symbol) || err;
        if (p->end && p->in > p->end)
            p->symbol->type = LEX_EOL;
    } while (err && p->symbol->type != LEX_EOL);
    return !err;
}

static int accept(struct parser *p, enum lex_token_type sym)
{
    if (p->symbol->type != sym)
        return 0;

    /* Swap p->accepted and p->symbol */
    p->accepted = p->symbol;
    if (p->symbol == &p->tokens[0]) {
        p->symbol = &p->tokens[1];
    } else { /* p->symbol == &p->tokens[1] */
        p->symbol = &p->tokens[0];
    }

    return next_symbol(p);
}

static int expect(struct parser *p, enum lex_token_type sym)
{
    if (!accept(p, sym)) {
        char symstr[64];
        lex_token_to_string(p->symbol, symstr, sizeof(symstr));
        parse_error(p, "unexpected symbol '%s'", symstr);
        return 0;
    }
    return 1;
}

static int accept_typecast(struct parser *p, enum value_type *type, int *ptrs)
{
    const char *pin = p->in;
    if (accept(p, LEX_LEFT_PARENTHESIS)) {
        if (accept(p, LEX_IDENTIFIER)) {
            size_t len = p->accepted->value.identifier.len;
            const char *name = p->accepted->value.identifier.name;
            if ((*type = value_type_from_substring(name, len))) {
                for (*ptrs = 0; accept(p, LEX_MUL); (*ptrs)++);
                if (accept(p, LEX_RIGHT_PARENTHESIS))
                    return 1;
            }
        }
        /* Restore old parser state */
        p->symbol->type = LEX_LEFT_PARENTHESIS;
        p->in = pin;
    }
    return 0;
}

struct ast *parse_expression(struct parser *p, const char *in)
{
    struct ast *out;
    int errors = p->errors;

    p->in = in;
    p->symbol = &p->tokens[0];
    p->accepted = &p->tokens[1];
    memset(p->tokens, 0, sizeof(p->tokens));
    next_symbol(p);
    if (accept(p, LEX_EOL)) {
        parse_error(p, "empty input");
        out = NULL;
    } else if ((out = expression(p))) {
        if (p->symbol->type != LEX_EOL) {
            char tokenstr[32];
            lex_token_to_string(p->symbol, tokenstr, sizeof(tokenstr)-1);
            parse_error(p, "EOL expected before '%s'", tokenstr);
            do { next_symbol(p); } while (p->symbol->type != LEX_EOL);
        }
        if (p->errors > errors) {
            ast_delete(out);
            out = NULL;
        }
    }

    return out;
}

#define INT UMAX
#ifndef NO_FLOAT_VALUES
#define INTFPU FMAX
#else
#define INTFPU INT
#endif
static struct ast *ast_binary_try_new(struct parser *p, enum ast_type node_type,
                                      struct ast *left, struct ast *right,
                                      enum value_type max_type)
{
    const char *errfmt;
    struct ast_cast *cast, **pcast;
    struct ast **pother;
    if (!left || !right)
        goto delete_operands;

    if ((left->value_type & PTR) && (right->value_type & PTR)) {
        if ((node_type == AST_SUB && left->value_type == right->value_type)
                || ast_type_is_compare(node_type)) {
            struct ast *root;
            struct ast *l = ((struct ast_unary *)left)->child;
            struct ast *r = ((struct ast_unary *)right)->child;
            if ((root = ast_binary_new(node_type, l, r))) {
                if (ast_is_compare(root)) {
                    root->value_type = S32;
                } else {
                    root->value_type = p->addr_type ^ (S8 ^ U8);
                }
                free(left);
                free(right);
                return root;
            }
            errfmt = "out-of-memory for '%s' with pointer operands";
        } else {
            errfmt = "two pointer operands unsupported by '%s'";
        }
        goto print_error;
    } else if (left->value_type & PTR) {
        cast = (struct ast_cast *)left;
        pcast = (struct ast_cast **)&left;
        pother = (struct ast **)&right;
    } else if (right->value_type & PTR) {
        cast = (struct ast_cast *)right;
        pcast = (struct ast_cast **)&right;
        pother = (struct ast **)&left;
    } else {
        pcast = NULL;
        cast = NULL;
    }

    if ((left->value_type <= max_type || left->value_type & PTR)
            && (right->value_type <= max_type || right->value_type & PTR)) {
        if (cast) {
            if (((struct ast *)cast)->node_type != AST_CAST) {
                errfmt = "unexpected pointer from non-cast node in '%s'";
                goto print_error;
            }
            if (!ast_type_is_conditional(node_type)) {
                if ((*pother)->value_type < p->addr_type)
                    *pother = ast_cast_new(p->addr_type, *pother);
            }
            *(struct ast **)pcast = cast->tree.child;
        } else {
            if (left->value_type < right->value_type)
                left = ast_cast_new(right->value_type, left);
            if (right->value_type < left->value_type)
                right = ast_cast_new(left->value_type, right);
            if (left) {
                if (value_type_is_int(left->value_type)) {
                    if (left->value_type < S32)
                        left = ast_cast_new(S32, left);
                }
                #ifndef NO_FLOAT_VALUES
                else if (value_type_is_fpu(left->value_type)) {
                    if (left->value_type < F64)
                        left = ast_cast_new(F64, left);
                }
                #endif
            }
            if (right) {
                if (value_type_is_int(right->value_type)) {
                    if (right->value_type < S32)
                        right = ast_cast_new(S32, right);
                }
                #ifndef NO_FLOAT_VALUES
                else if (value_type_is_fpu(right->value_type))  {
                    if (right->value_type < F64)
                        right = ast_cast_new(F64, right);
                }
                #endif
            }
        }

        if (left && right) {
            struct ast *root;
            if ((root = ast_binary_new(node_type, left, right))) {
                if (ast_is_compare(root)) {
                    root->value_type = S32;
                } else {
                    root->value_type = left->value_type;
                    if (cast) {
                        cast->tree.child = root;
                        root = (struct ast *)cast;
                    }
                }
                return root;
            } else {
                errfmt = "out-of-memory for AST node '%s'";
            }
        } else {
            errfmt = "out-of-memory for promoted operands for '%s'";
        }
    } else {
        errfmt = "invalid operand types for '%s'";
    }

print_error:
    parse_error(p, errfmt, lex_token_type_string[ast_to_lex_type(node_type)]);

delete_operands:
    if (left) ast_delete(left);
    if (right) ast_delete(right);
    return NULL;
}

/*
 * Productions
 */
static struct ast *expression(struct parser *p)
{
    return (p->symbol->type == LEX_EOL) ? NULL : conditional_expression(p);
}

static struct ast *conditional_expression(struct parser *p)
{
    struct ast *root = equality_expression(p);

    while (root && (accept(p, LEX_AND_COND) || accept(p, LEX_OR_COND))) {
        enum ast_type type = lex_to_ast_type(p->accepted->type);
        struct ast *left = root;
        struct ast *right = equality_expression(p);
        root = ast_binary_try_new(p, type, left, right, INTFPU);
    }

    return root;
}

static struct ast *equality_expression(struct parser *p)
{
    struct ast *root = relational_expression(p);

    if (root && (accept(p, LEX_EQ) || accept(p, LEX_NEQ))) {
        enum ast_type type = lex_to_ast_type(p->accepted->type);
        struct ast *left = root;
        struct ast *right = relational_expression(p);
        root = ast_binary_try_new(p, type, left, right, INTFPU);
    }

    return root;
}

static struct ast *relational_expression(struct parser *p)
{
    struct ast *root = or_expression(p);

    if (root && (accept(p, LEX_LT) || accept(p, LEX_GT)
              || accept(p, LEX_LE) || accept(p, LEX_GE))) {
        enum ast_type type = lex_to_ast_type(p->accepted->type);
        struct ast *left = root;
        struct ast *right = or_expression(p);
        root = ast_binary_try_new(p, type, left, right, INTFPU);
    }

    return root;
}

static struct ast *or_expression(struct parser *p)
{
    struct ast *root = xor_expression(p);

    while (root && accept(p, LEX_OR)) {
        struct ast *left = root;
        struct ast *right = xor_expression(p);
        root = ast_binary_try_new(p, AST_OR, left, right, INT);
    }

    return root;
}
static struct ast *xor_expression(struct parser *p)
{
    struct ast *root = and_expression(p);

    while (root && accept(p, LEX_XOR)) {
        struct ast *left = root;
        struct ast *right = and_expression(p);
        root = ast_binary_try_new(p, AST_XOR, left, right, INT);
    }

    return root;
}
static struct ast *and_expression(struct parser *p)
{
    struct ast *root = shift_expression(p);

    while (root && accept(p, LEX_AND)) {
        struct ast *left = root;
        struct ast *right = shift_expression(p);
        root = ast_binary_try_new(p, AST_AND, left, right, INT);
    }

    return root;
}

static struct ast *shift_expression(struct parser *p)
{
    struct ast *root = addsub_expression(p);

    while (root && (accept(p, LEX_SHL) || accept(p, LEX_SHR))) {
        enum ast_type type = lex_to_ast_type(p->accepted->type);
        struct ast *left = root;
        struct ast *right = addsub_expression(p);
        root = ast_binary_try_new(p, type, left, right, INT);
    }

    return root;
}

static struct ast *addsub_expression(struct parser *p)
{
    struct ast *root = muldiv_expression(p);

    while (root && (accept(p, LEX_ADD) || accept(p, LEX_SUB))) {
        enum ast_type type = lex_to_ast_type(p->accepted->type);
        struct ast *left = root;
        struct ast *right = muldiv_expression(p);
        root = ast_binary_try_new(p, type, left, right, INTFPU);
    }

    return root;
}

static struct ast *muldiv_expression(struct parser *p)
{
    struct ast *root = cast_expression(p);

    while (root && (accept(p, LEX_MUL)
                 || accept(p, LEX_DIV)
                 || accept(p, LEX_MOD))) {
        enum ast_type type = lex_to_ast_type(p->accepted->type);
        struct ast *left = root;
        struct ast *right = cast_expression(p);
        enum value_type valid_types = (type == AST_MOD) ? INT : INTFPU;
        root = ast_binary_try_new(p, type, left, right, valid_types);
    }

    return root;
}

static struct ast *cast_expression(struct parser *p)
{
    int ptrs;
    enum value_type type;
    if (accept_typecast(p, &type, &ptrs)) {
        struct ast *child;
        if (ptrs > 1) {
            parse_error(p, "multi-level pointers unimplemented");
            return NULL;
        }
        if (!(child = cast_expression(p)))
            return NULL;
        if (child->value_type & PTR) {
            struct ast *gchild = ((struct ast_unary *)child)->child;
            free(child);
            child = gchild;
        }
        if (!ptrs)
            return ast_cast_new(type, child);
        if (child->value_type < p->addr_type) {
            struct ast *cast;
            if (!(cast = ast_cast_new(p->addr_type, child))) {
                parse_error(p, "out-of-memory for pointer cast");
                ast_delete(child);
                return NULL;
            }
            child = cast;
        }
        return ast_cast_new(type|PTR, child);
    }
    return unary_expression(p);
}

static struct ast *unary_expression(struct parser *p)
{
    struct ast *root;

    if (accept(p, LEX_ADD)) {
        /* Unary add is no-op */
        return cast_expression(p);
    }

    if (accept(p, LEX_MUL)) {
        struct ast *child;
        if (!(child = cast_expression(p)))
            return NULL;
        if (child->value_type & PTR) {
            if (p->target) {
                enum value_type type = child->value_type ^ PTR;
                struct ast *gchild = ((struct ast_unary *)child)->child;
                if ((root = ast_deref_new(gchild, type, p->target))) {
                    root->value_type = child->value_type & ~PTR;
                    p->has_deref |= 1;
                    free(child);
                    return root;
                }
                parse_error(p, "out-of-memory for AST_DEREF node");
            } else {
                parse_error(p, "cannot dereference pointers without target");
            }
        } else {
            parse_error(p, "invalid non-pointer operand type for unary '*'");
        }
        ast_delete(child);
        return NULL;
    }

    if (accept(p, LEX_SUB) || accept(p, LEX_NOT) || accept(p, LEX_COMPL)) {
        const char *errfmt, *op;
        enum ast_type type = lex_to_ast_type(p->accepted->type);
        struct ast *child = cast_expression(p);
        if (!child) return NULL;

        if (type == AST_SUB)
            type = AST_NEG;

        if (child->value_type & PTR) {
            if (child->node_type == AST_CAST) {
                struct ast **pgchild = &((struct ast_unary *)child)->child;
                if ((root = ast_unary_new(type, *pgchild))) {
                    root->value_type = (*pgchild)->value_type;
                    if (type != AST_NOT) {
                        *pgchild = root;
                        root = child;
                    } else {
                        free(child);
                    }
                    return root;
                }
                parse_error(p, "out-of-memory for implicit cast for '!'");
            } else {
                parse_error(p, "unexpected pointer from non-cast node");
                root = NULL;
            }
            ast_delete(child);
            return root;
        }

        if (child->value_type & ((type == AST_NEG) ? INTFPU : INT)) {
            if (child->value_type < S32)
                child = ast_cast_new(S32, child);
            if (child) {
                if ((root = ast_unary_new(type, child))) {
                    root->value_type = child->value_type;
                    return root;
                }
                errfmt = "out-of-memory for AST node '%s'";
            } else {
                errfmt = "out-of-memory for implicit cast for '%s'";
            }
        } else {
            errfmt = "invalid operand type for '%s'";
        }

        op = lex_token_type_string[ast_to_lex_type(type)];
        parse_error(p, errfmt, op);
        ast_delete(child);
        return NULL;
    } else {
        root = factor(p);
    }

    return root;
}

static struct ast *factor(struct parser *p)
{
    struct ast *root;

    if (accept(p, LEX_IDENTIFIER)) {
        size_t sym;
        const char *name = p->accepted->value.identifier.name;
        size_t len = p->accepted->value.identifier.len;
        if (p->symtab && (sym = symbol_table_lookup(p->symtab, name, len))) {
            struct symbol *symbol = p->symtab->symbols[sym];
            if (symbol->type & PTR) {
                size_t size = value_type_sizeof(p->addr_type);
                root = ast_var_new(p->symtab, sym, size);
                if (root) {
                    struct ast *cast;
                    root->value_type = p->addr_type;
                    if ((cast = ast_cast_new(symbol->type, root))) {
                        root = cast;
                    } else {
                        ast_delete(root);
                        root = NULL;
                    }
                }
            } else {
                root = ast_var_new(p->symtab, sym, 0);
            }
            if (!root)
                parse_error(p, "out-of-memory for AST variable node");
        } else {
            parse_error(p, "unknown identifier '%.*s'", (int)len, name);
            root = NULL;
        }
    } else if (accept(p, LEX_INTEGER)) {
        struct value value;
        #ifndef NO_64BIT_VALUES
        intmax_t sint = p->accepted->value.integer;
        if ((uintmax_t)sint != (uint32_t)sint) {
            value_init_s64(&value, sint);
        } else {
            value_init_s32(&value, sint);
        }
        #else
        value_init_s32(&value, (int32_t)p->accepted->value.integer);
        #endif
        root = ast_value_new(&value);
    } else if (accept(p, LEX_UINTEGER)) {
        struct value value;
        #ifndef NO_64BIT_VALUES
        uintmax_t uint = (uintmax_t)p->accepted->value.integer;
        if (uint != (uint32_t)uint) {
            value_init_u64(&value, uint);
        } else {
            value_init_u32(&value, uint);
        }
        #else
        value_init_u32(&value, (uint32_t)p->accepted->value.integer);
        #endif
        root = ast_value_new(&value);
    #ifndef NO_FLOAT_VALUES
    } else if (accept(p, LEX_FLOATING_POINT)) {
        struct value value;
        value_init_f64(&value, p->accepted->value.fp);
        root = ast_value_new(&value);
    #endif
    } else if (accept(p, LEX_LEFT_PARENTHESIS)) {
        root = expression(p);
        expect(p, LEX_RIGHT_PARENTHESIS);
    } else {
        if (p->symbol->type != LEX_EOL) {
            const char *token = lex_token_type_string[p->symbol->type];
            parse_error(p, "expected a factor but got '%s'", token);
        } else {
            parse_error(p, "expected a factor before EOL");
        }
        root = NULL;
    }

    return root;
}
