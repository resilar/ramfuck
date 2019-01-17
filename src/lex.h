#ifndef LEX_H_INCLUDED
#define LEX_H_INCLUDED

#include <stddef.h>
#include <stdint.h>

enum lex_token_type {
    LEX_NIL = 0,

    LEX_EOL, LEX_LEFT_PARENTHESE, LEX_RIGHT_PARENTHESE,

    LEX_INTEGER, LEX_UINTEGER, LEX_FLOATING_POINT, LEX_IDENTIFIER,

    /*STRING,
    WIDE_STRING,*/

    /* The order must match ast_types in ast.h */
    LEX_ADD, LEX_SUB, LEX_MUL, LEX_DIV, LEX_MOD,

    LEX_AND, LEX_XOR, LEX_OR, LEX_SHL, LEX_SHR,
    
    LEX_CAST, LEX_UADD, LEX_USUB, /* reserved */
    LEX_NOT, LEX_COMPL,

    LEX_EQ, LEX_NEQ, LEX_LT, LEX_GT, LEX_LE, LEX_GE,

    LEX_AND_COND, LEX_OR_COND,

    LEX_TYPES
};

struct lex_token {
    enum lex_token_type type;

    union {
        intmax_t integer;
        double fp;
        struct {
            const char *name;
            size_t len;
        } identifier;

        /*char string[4096];
        wchar_t wstring[2048];*/ /* Not supported (yet?) */
    } value;
};

/**
 * Read the next token from string `*pin` and advance the pointer `*pin`.
 *
 * On success, returns the given `out` pointer with a filled token structure.
 * Return NULL if EOF is reached.
 */
struct lex_token *lexer(const char **pin, struct lex_token *out);

extern const char *lex_token_type_string[LEX_TYPES];
size_t lex_token_to_string(struct lex_token *t, char *out, size_t size);

#endif
