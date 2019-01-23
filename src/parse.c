#define _DEFAULT_SOURCE /* vfprintf(3) */
#include "parse.h"
#include "ramfuck.h"
#include "value.h"

#include <memory.h>
#include <stdarg.h>
#include <stdlib.h>

void parser_init(struct parser *target)
{
    memset(target, 0, sizeof(struct parser));
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
    if (!lexer(&p->in, p->symbol)) {
        do { lexer(&p->in, p->symbol); } while (p->symbol->type != LEX_EOL);
        return 0;
    }
    return 1;
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

struct ast *parse_expression(struct parser *p, const char *in)
{
    struct ast *out;
    int errors = p->errors;

    p->in = in;
    p->symbol = &p->tokens[0];
    p->accepted = &p->tokens[1];
    memset(p->tokens, 0, sizeof(p->tokens));
    next_symbol(p);
    if ((out = expression(p))) {
        if (p->symbol->type != LEX_EOL) {
            parse_error(p, "EOL expected");
            do { next_symbol(p); } while (p->symbol->type != LEX_EOL);
        }
        if (p->errors > errors) {
            ast_delete(out);
            out = NULL;
        }
    } else if (p->errors == errors) {
        parse_error(p, "empty input");
    }

    return out;
}

static struct ast *ast_binop_try_new(struct parser *p, enum ast_type node_type,
                                     struct ast *left, struct ast *right,
                                     enum value_type mask, enum value_type type)
{
    const char *errfmt;
    if (!left || !right)
        goto delete_operands;

    if ((left->value_type & mask) && (right->value_type & mask)) {
        if (left->value_type < right->value_type)
            left = ast_cast_new(right->value_type, left);
        if (right->value_type < left->value_type)
            right = ast_cast_new(left->value_type, right);
        if (left && left->value_type < S32)
            left = ast_cast_new(S32, left);
        if (right && right->value_type < S32)
            right = ast_cast_new(S32, right);
        if (left && right) {
            struct ast *root;
            if ((root = ast_binop_new(node_type, left, right))) {
                root->value_type = type ? type : left->value_type;
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

    parse_error(p, errfmt, lex_token_type_string[ast_to_lex_type(node_type)]);

delete_operands:
    if (left) ast_delete(left);
    if (right) ast_delete(right);
    return NULL;
}

/*
 * Productions. See doc/grammar.txt
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
        root = ast_binop_try_new(p, type, left, right, INT|FPU, S32);
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
        root = ast_binop_try_new(p, type, left, right, INT|FPU, S32);
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
        root = ast_binop_try_new(p, type, left, right, INT|FPU, S32);
    }

    return root;
}

static struct ast *or_expression(struct parser *p)
{
    struct ast *root = xor_expression(p);

    while (root && accept(p, LEX_OR)) {
        struct ast *left = root;
        struct ast *right = xor_expression(p);
        root = ast_binop_try_new(p, AST_OR, left, right, INT, 0);
    }

    return root;
}
static struct ast *xor_expression(struct parser *p)
{
    struct ast *root = and_expression(p);

    while (root && accept(p, LEX_XOR)) {
        struct ast *left = root;
        struct ast *right = and_expression(p);
        root = ast_binop_try_new(p, AST_XOR, left, right, INT, 0);
    }

    return root;
}
static struct ast *and_expression(struct parser *p)
{
    struct ast *root = shift_expression(p);

    while (root && accept(p, LEX_AND)) {
        struct ast *left = root;
        struct ast *right = shift_expression(p);
        root = ast_binop_try_new(p, AST_AND, left, right, INT, 0);
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
        root = ast_binop_try_new(p, type, left, right, INT, 0);
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
        root = ast_binop_try_new(p, type, left, right, INT|FPU, 0);
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
        enum value_type valid_types = (type == AST_MOD) ? INT : (INT|FPU);
        root = ast_binop_try_new(p, type, left, right, valid_types, 0);
    }

    return root;
}

static struct ast *cast_expression(struct parser *p)
{
    if (p->symbol->type == LEX_LEFT_PARENTHESE) {
        struct lex_token peek[2];
        const char *pin = p->in;
        if (lexer(&pin, &peek[0]) && lexer(&pin, &peek[1])
                && peek[0].type == LEX_IDENTIFIER
                && peek[1].type == LEX_RIGHT_PARENTHESE) {
            char buf[16];
            enum value_type type;
            size_t len = peek[0].value.identifier.len;
            const char *name = peek[0].value.identifier.name;
            if (len < sizeof(buf)-1) {
                buf[len] = '\0';
                type = value_type_from_string(memcpy(buf, name, len));
                if (type && accept(p, LEX_LEFT_PARENTHESE)
                         && accept(p, LEX_IDENTIFIER)
                         && accept(p, LEX_RIGHT_PARENTHESE)) {
                    struct ast *child = cast_expression(p);
                    return child ? ast_cast_new(type, child) : NULL;
                }
            }
        }
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

    if (accept(p, LEX_SUB) || accept(p, LEX_NOT) || accept(p, LEX_COMPL)) {
        enum ast_type type = lex_to_ast_type(p->accepted->type);
        struct ast *child = cast_expression(p);
        root = NULL;
        if (child) {
            const char *errfmt, *op;
            enum value_type valid_types;

            if (type == AST_SUB) {
                type = AST_NEG;
                valid_types = INT|FPU;
            } else {
                valid_types = INT;
            }

            if (child->value_type & valid_types) {
                if (child->value_type < S32)
                    child = ast_cast_new(S32, child);
                if (child) {
                    if ((root = ast_unop_new(type, child))) {
                        root->value_type = child->value_type;
                        return root;
                    }
                    errfmt = "out-of-memory for AST node '%s'";
                } else {
                    errfmt = "out-of-memory for implicit typecast for '%s'";
                }
            } else {
                errfmt = "invalid operand types for '%s'";
            }

            op = lex_token_type_string[ast_to_lex_type(type)];
            parse_error(p, errfmt, op);
            ast_delete(child);
        }
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
            root = ast_var_new(p->symtab, sym);
        } else {
            parse_error(p, "unknown identifier '%.*s'", (int)len, name);
            root = NULL;
        }
    } else if (accept(p, LEX_INTEGER)) {
        struct value value;
        intmax_t sint = p->accepted->value.integer;
        if ((uintmax_t)sint != (uint32_t)sint) {
            value_init_s64(&value, sint);
        } else {
            value_init_s32(&value, sint);
        }
        root = ast_value_new(&value);
    } else if (accept(p, LEX_UINTEGER)) {
        struct value value;
        uintmax_t uint = (uintmax_t)p->accepted->value.integer;
        if (uint != (uint32_t)uint) {
            value_init_u64(&value, uint);
        } else {
            value_init_u32(&value, uint);
        }
        root = ast_value_new(&value);
    } else if (accept(p, LEX_FLOATING_POINT)) {
        struct value value;
        value_init_f64(&value, p->accepted->value.fp);
        root = ast_value_new(&value);
    } else if (accept(p, LEX_LEFT_PARENTHESE)) {
        root = expression(p);
        expect(p, LEX_RIGHT_PARENTHESE);
    } else {
        if (p->symbol->type != LEX_EOL) {
            const char *token = lex_token_type_string[p->symbol->type];
            parse_error(p, "expected a factor but got '%s'", token);
        } else {
            parse_error(p, "expected a factor");
        }
        root = NULL;
    }

    return root;
}
