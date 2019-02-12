#define _DEFAULT_SOURCE /* for snprintf(3) */
#include "lex.h"
#include "ramfuck.h"

#include <ctype.h>
#include <limits.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#define get(pin) ((++*pin)[-1])
#define unget(pin) ((*pin)--)
#define peek(pin) (**pin)

static int accept(const char **pin, int c)
{
    if (**pin == c) {
        (*pin)++;
        return 1;
    }
    return 0;
}

static int acceptf(const char **pin, int (*is)(int c))
{
    if (is(**pin)) {
        (*pin)++;
        return 1;
    }
    return 0;
}

enum scan_error_id {
    SUCCESS = 0,
    EMPTY_NUMBER,
    INVALID_BASE,
    INVALID_EXPONENT,
    INVALID_FRACTION,
    INVALID_OCT,
    INVALID_DEC,
    INVALID_HEX,
    #ifdef NO_FLOAT_VALUES
    UNSUPPORTED_FLOAT,
    #endif
    OVERFLOW_INTEGER,
    OVERFLOW_FRACTION,
    OVERFLOW_EXPONENT,
    SCAN_ERRORS
};

#if SCAN_ERRORS > UCHAR_MAX
# error "Way too many scan errors, wtf? Or broken CHAR_MAX dude!"
#endif

/* Macro to combine scan_error_id with optional character */
#define ERRC(id, c) (((unsigned char)(c) << CHAR_BIT) | (unsigned char)(id))
#define scan_error_to_char(err) ((char)(((err) >> CHAR_BIT)))
#define scan_error_to_id(err) ((enum scan_error_id)(unsigned char)(err))
typedef int scan_error_t;

void scan_error_errf(scan_error_t err)
{
    char c = scan_error_to_char(err);
    switch (scan_error_to_id(err)) {
    case SUCCESS:
        infof("lex: success (no error)");
        break;

    case EMPTY_NUMBER:
        errf("lex: empty number (got 0 digits)");
        break;

    case INVALID_BASE:
        errf("lex: number with invalid base (%c)", c);
        break;
    case INVALID_EXPONENT:
        errf("lex: floating-point number with invalid exponent digit '%c'", c);
        break;
    case INVALID_FRACTION:
        errf("lex: floating-point number with invalid fraction digit '%c'", c);
        break;
    case INVALID_OCT:
        errf("lex: invalid octal number digit '%c'", c);
        break;
    case INVALID_DEC:
        errf("lex: invalid decimal number digit '%c'", c);
        break;
    case INVALID_HEX:
        errf("lex: invalid hexadecimal number digit '%c'", c);
        break;

#ifdef NO_FLOAT_VALUES
    case UNSUPPORTED_FLOAT:
        errf("lex: floating point numbers unsupported");
        break;
#endif

    case OVERFLOW_INTEGER:
        errf("lex: numeric constant too large");
        break;
    case OVERFLOW_FRACTION:
        errf("lex: numeric fraction too large");
        break;
    case OVERFLOW_EXPONENT:
        errf("lex: numeric exponent too large");
        break;

    default: break;
    }
    return;
}

#ifndef NO_64BIT_VALUES
# define UMAX_MAX UINT64_MAX
#else
# define UMAX_MAX UINT32_MAX
#endif

static scan_error_t scan_number(const char **pin, struct lex_token *out)
{
    uintmax_t value, fraction, exponent;
    int digits, fraction_digits, exponent_digits;
    int base, has_exponent, neg_exponent, is_float;
    #ifndef NO_FLOAT_VALUES
    const char *in0 = *pin;
    #endif
    value = fraction = exponent = 0;
    digits = fraction_digits = exponent_digits = 0;
    base = has_exponent = neg_exponent = is_float = 0;

    /* Determine base */
    if (accept(pin, '0')) {
        if (accept(pin, 'x')) {
            base = 16;
        } else if (accept(pin, '.')) {
            base = 10;
            is_float = 1;
        } else {
            base = 8;
            digits++;
        }
    } else if (isdigit(peek(pin))) {
        base = 10;
    } else if (accept(pin, '.')) {
        base = 10;
        is_float = 1;
    }
    if (base == 0)
        return ERRC(INVALID_BASE, peek(pin));

    /* Scan the number */
    while (isxdigit(peek(pin)) || peek(pin) == '.') {
        /* Process character c */
        char c = get(pin);
        if (!is_float) {
            int val;
            if (base == 8) {
                if (c >= '0' && c <= '7') {
                    val = c - '0';
                } else return ERRC(INVALID_OCT, c);
                digits++;
            } else if (base == 16) {
                if (c >= '0' && c <= '9') {
                    val = c - '0';
                } else if (c >= 'A' && c <= 'F') {
                    val = c - 'A' + 10;
                } else if (c >= 'a' && c <= 'f') {
                    val = c - 'a' + 10;
                } else return ERRC(INVALID_HEX, c);
                digits++;
            } else /*if (base == 10)*/ {
                if (c >= '0' && c <= '9') {
                    val = c - '0';
                    digits++;
                } else if (c == '.') {
                    is_float = 1;
                } else if (c == 'e' || c == 'E') {
                    has_exponent = 2*(accept(pin, '+') || !accept(pin, '-'))-1;
                    if (!isdigit(peek(pin)))
                        return ERRC(INVALID_EXPONENT, c);
                    is_float = 1;
                } else return ERRC(INVALID_DEC, c);
            }
            if (!is_float) {
                if (value > (UMAX_MAX - val) / base)
                    return OVERFLOW_INTEGER;
                value = value * base + val;
            }
        } else if (!has_exponent) {
            if (c >= '0' && c <= '9') {
                if (fraction > (UMAX_MAX - (c - '0')) / 10)
                    return OVERFLOW_FRACTION;
                fraction = fraction * 10 + (c - '0');
                fraction_digits++;
            } else if ((digits || fraction_digits) && (c == 'e' || c == 'E')) {
                has_exponent = 2*(accept(pin, '+') || !accept(pin, '-'))-1;
            } else return ERRC(INVALID_FRACTION, c);
        } else /*if (has_exponent)*/ {
            if (c >= '0' && c <= '9') {
                if (exponent > (UMAX_MAX - (c - '0')) / 10)
                    return OVERFLOW_FRACTION;
                exponent = exponent * 10 + (c - '0');
                exponent_digits++;
            } else return ERRC(INVALID_EXPONENT, c);
        }
    }

    if (!digits && !fraction_digits && !exponent_digits)
        return EMPTY_NUMBER;

    /* Fill token */
    if (is_float) {
        #ifdef NO_FLOAT_VALUES
        return UNSUPPORTED_FLOAT;
        #else
        char *end;
        out->type = LEX_FLOATING_POINT;
        out->value.fp = strtod(in0, &end);
        *pin = end;
        #endif
    } else {
        if (accept(pin, 'u') || accept(pin, 'U')
                || (INT32_MAX < value && value <= UINT32_MAX)
                #ifndef NO_64BIT_VALUES
                || (INT64_MAX < value && value <= UINT64_MAX)
                #endif
                )
            out->type = LEX_UINTEGER;
        else out->type = LEX_INTEGER;
        out->value.integer = value;
    }
    return SUCCESS;
}

int lexer(const char **pin, struct lex_token *out)
{
    scan_error_t err;

    while (acceptf(pin, isspace));
    memset(out, 0, sizeof(struct lex_token));

    switch (get(pin)) {
    case '(': out->type = LEX_LEFT_PARENTHESE; break;
    case ')': out->type = LEX_RIGHT_PARENTHESE; break;

    case '.':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        unget(pin);
        if ((err = scan_number(pin, out))) {
            scan_error_errf(err);
            return 0;
        }
        break;

    case '+': out->type = LEX_ADD; break;
    case '-': out->type = LEX_SUB; break;
    case '*': out->type = LEX_MUL; break;
    case '/': out->type = LEX_DIV; break;
    case '%': out->type = LEX_MOD; break;
    case '!': out->type = accept(pin, '=') ? LEX_NEQ : LEX_NOT; break;
    case '~': out->type = LEX_COMPL; break;
    case '^': out->type = LEX_XOR; break;
    case '|': out->type = accept(pin, '|') ? LEX_OR_COND : LEX_OR; break;
    case '&': out->type = accept(pin, '&') ? LEX_AND_COND : LEX_AND; break;

    case '=':
        if (!accept(pin, '=')) {
            errf("lex: expected '=' after '='");
            return 0;
        }
        out->type = LEX_EQ;
        break;

    case '<':
        if (accept(pin, '=')) {
            out->type = LEX_LE;
        } else if (accept(pin, '<')) {
            out->type = LEX_SHL;
        } else {
            out->type = LEX_LT;
        }
        break;

    case '>':
        if (accept(pin, '=')) {
            out->type = LEX_GE;
        } else if (accept(pin, '>')) {
            out->type = LEX_SHR;
        } else {
            out->type = LEX_GT;
        }
        break;

    case '\r':
        if (!accept(pin, '\n')) {
            errf("lex: expected newline after '\r'");
            return 0;
        }
        /* fall-through */
    case '\n':
    case '\0':
        out->type = LEX_EOL;
        break;

    default:
        unget(pin);
        if (isalpha(peek(pin)) || peek(pin) == '_') {
            out->value.identifier.name = *pin;
            while (acceptf(pin, isalnum) || accept(pin, '_'));
            out->value.identifier.len = *pin - out->value.identifier.name;
            out->type = LEX_IDENTIFIER;
        } else if (isprint(peek(pin))) {
            errf("lex: unexpected character '%c'", get(pin));
            return 0;
        } else {
            errf("lex: unexpected character '\\x%02X'", get(pin));
            return 0;
        }
    }

    return 1;
}

const char *lex_token_type_string[LEX_TYPES] = {
    "NIL",  /* LEX_NIL */

    "EOL",  /* LEX_EOL */
    "(",    /* LEX_LEFT_PARENTHESE */
    ")",    /* LEX_RIGHT_PARENTHESE */

    "sint", /* LEX_INTEGER */
    "uint", /* LEX_UINTEGER */
#ifndef NO_FLOAT_VALUES
    "fp",   /* LEX_FLOATING_POINT */
#endif
    "var",  /* LEX_IDENTIFIER */

    "(T)",  /* LEX_CAST */
    "u*",   /* LEX_DEREF */
    "u-",   /* LEX_NEG */
    "!",    /* LEX_NOT */
    "~",    /* LEX_COMPL */

    "+",    /* LEX_ADD */
    "-",    /* LEX_SUB */
    "*",    /* LEX_MUL */
    "/",    /* LEX_DIV */
    "%",    /* LEX_MOD */

    "&",    /* LEX_AND */
    "^",    /* LEX_XOR */
    "|",    /* LEX_OR */
    "<<",   /* LEX_SHL */
    ">>",   /* LEX_SHR */

    "==",   /* LEX_EQ */
    "!=",   /* LEX_NEQ */
    "<",    /* LEX_LT */
    ">",    /* LEX_GT */
    "<=",   /* LEX_LE */
    ">=",   /* LEX_GE */

    "&&",   /* LEX_AND_COND */
    "||"    /* LEX_OR_COND */
};

size_t lex_token_to_string(struct lex_token *t, char *out, size_t size)
{
    switch (t->type) {
    case LEX_INTEGER:
        return snprintf(out, size, "%ld", (long int)t->value.integer);
    case LEX_UINTEGER:
        return snprintf(out, size, "%luu", (unsigned long)t->value.integer);
#ifndef NO_FLOAT_VALUES
    case LEX_FLOATING_POINT:
        return snprintf(out, size, "%g", t->value.fp);
#endif
    case LEX_IDENTIFIER: {
        const char *name = t->value.identifier.name;
        size_t len = t->value.identifier.len;
        return snprintf(out, size, "%.*s", (int)len, name);
    }

    default:
        if (t->type >= 0 && t->type < LEX_TYPES)
            return snprintf(out, size, "%s", lex_token_type_string[t->type]);
        break;
    }

    return snprintf(out, size, "%s", "???");
}
