#define _DEFAULT_SOURCE /* for snprintf(3) */
#include "lex.h"
#include "ramfuck.h"

#include <ctype.h>
#include <math.h>
#include <memory.h>
#include <stdio.h>

static const char get(const char **pin)
{
    (*pin)++;
    return (*pin)[-1];
}

static const char peek(const char **pin)
{
    return **pin;
}

static const char unget(const char **pin)
{
    (*pin)--;
    return **pin;
}

/* Returns 0 on an invalid number. */
/* TODO: error on too big numbers */
static int scan_number(const char **pin, struct lex_token *out)
{
    intmax_t val, fraction, exponent;
    double dval;
    int neg_exponent, base, has_point, has_exponent;
    int digits, fraction_digits, exponent_digits;
    int is_unsigned_int;
    char c;
    has_point = has_exponent = 0;
    neg_exponent = 0;
    is_unsigned_int = 0;

    /* Determine base. */
    c = get(pin);
    if (c == '0') {
        if (peek(pin) == 'x') {
            get(pin);
            c = get(pin);
            base = 16;
        } else if (isdigit(peek(pin))) { 
            base = 8;
        } else base = 10;
    } else if (isdigit(c)) {
        base = 10;
    } else if (c == '.') {
        base = 10;
        has_point = 1;
        c = get(pin);
    } else return 0;

    /* Scan the number */
    dval = val = fraction = exponent = 0;
    digits = fraction_digits = exponent_digits = 0;
    do {
        /* Process character c */
        if (!has_point && !has_exponent) {
            if (base == 8) {
                if (c >= '0' && c <= '8') {
                    val = val * 8 + c - '0';
                } else return 0;
                digits++;
            } else if (base == 16) {
                if (c >= '0' && c <= '9') {
                    val = val * 16 + c - '0';
                } else if (c >= 'A' && c <= 'F') {
                    val = val * 16 + 10 + c - 'A';
                } else if (c >= 'a' && c <= 'f') {
                    val = val * 16 + 10 + c - 'a';
                } else return 0;
                digits++;
            } else if (base == 10) {
                if (c >= '0' && c <= '9') {
                    val = val * 10 + c - '0';
                    digits++;
                } else if (c == '.') {
                    dval = val;
                    has_point = 1;
                } else if (c == 'e' || c == 'E') {
                    char next = peek(pin);
                    dval = val;
                    has_exponent = 1;
                    if (next == '-' || next == '+') {
                        get(pin);
                        neg_exponent = (next == '-');
                        next = peek(pin);
                    }
                    if (!isdigit(next)) {
                        return 0;
                    }
                } else return 0;
            }
        } else if (has_point && !has_exponent) {
            if (c >= '0' && c <= '9') {
                fraction = fraction * 10 + c - '0';
                fraction_digits++;
            } else if ((digits || fraction_digits) && (c == 'e' || c == 'E')) {
                has_exponent = 1;
                if (peek(pin) == '-') {
                    get(pin);
                    neg_exponent = 1;
                }
            } else return 0;
        } else if (has_exponent) {
            if (c >= '0' && c <= '9') {
                exponent = exponent * 10 + c - '0';
                exponent_digits++;
            } else return 0;
        }

    } while ((c = get(pin)) != '\0' && strchr("0123456789abcdefABCDEF.", c));
    unget(pin);

    /* Trailing characters / suffix */
    if (isalpha(c)) {
        if ((c == 'u' || c == 'U') && !has_point && !has_exponent) {
            get(pin);
            is_unsigned_int = 1;
        } else return 0;
    }

    /* Fill token */
    if (has_point || has_exponent) {
        out->type = LEX_FLOATING_POINT;
        if (neg_exponent) exponent = -exponent;
        out->value.fp = (dval + fraction*pow(10, -fraction_digits))
            * pow(10, exponent);
    } else {
        out->type = (is_unsigned_int) ? LEX_UINTEGER : LEX_INTEGER;
        out->value.integer = val;
    }
    return 1;
}

struct lex_token *lexer(const char **pin, struct lex_token *out)
{
    char c;

    memset(out, 0, sizeof(struct lex_token));
    while (isspace(peek(pin))) get(pin);

    c = get(pin);
    if (c == '(') {
        out->type = LEX_LEFT_PARENTHESE;
    } else if (c == ')') {
        out->type = LEX_RIGHT_PARENTHESE;
    } else if (c == '.' || isdigit(c)) {
        unget(pin);
        if (!scan_number(pin, out)) {
            errf("lex: invalid number");
            return NULL;
        }
    } else if (c == '+') {
        out->type = LEX_ADD;
    } else if (c == '-') {
        out->type = LEX_SUB;
    } else if (c == '*') {
        out->type = LEX_MUL;
    } else if (c == '/') {
        out->type = LEX_DIV;
    } else if (c == '%') {
        out->type = LEX_MOD;
    } else if (c == '!') {
        if (peek(pin) == '=') {
            get(pin);
            out->type = LEX_NEQ;
        } else out->type = LEX_NOT;
    } else if (c == '~') {
        out->type = LEX_COMPL;
    } else if (c == '^') {
        out->type = LEX_XOR;
    } else if (c == '|') {
        if (peek(pin) == '|') {
            get(pin);
            out->type = LEX_OR_COND;
        } else {
            out->type = LEX_OR;
        }
    } else if (c == '&') {
        if (peek(pin) == '&') {
            get(pin);
            out->type = LEX_AND_COND;
        } else {
            out->type = LEX_AND;
        }
    } else if (c == '=') {
        c = get(pin);
        if (c != '=') {
            errf("lex: expected '=' after '='");
            return NULL;
        }
        out->type = LEX_EQ;
    } else if (c == '<') {
        if (peek(pin) == '=') {
            get(pin);
            out->type = LEX_LE;
        } else if (peek(pin) == '<') {
            get(pin);
            out->type = LEX_SHL;
        } else {
            out->type = LEX_LT;
        }
    } else if (c == '>') {
        if (peek(pin) == '=') {
            get(pin);
            out->type = LEX_GE;
        } else if (peek(pin) == '>') {
            get(pin);
            out->type = LEX_SHR;
        } else {
            out->type = LEX_GT;
        }
    } else if (c == '\0' || c == '\n' || c == '\r') {
        out->type = LEX_EOL;
    } else if (isalpha(c)) {
        out->type = LEX_IDENTIFIER;
        out->value.identifier.name = (*pin)-1;
        while (isalnum(peek(pin)) || **pin == '_') (*pin)++;
        out->value.identifier.len = *pin - out->value.identifier.name;
    } else {
        errf("lex: unexpected character '%c'", c);
        return NULL;
    }

    return out;
}

size_t lex_token_to_string(struct lex_token *t, char *out, size_t size)
{
    switch (t->type) {
    case LEX_EOL: return snprintf(out, size, "EOL");
    case LEX_LEFT_PARENTHESE: return snprintf(out, size, "(");
    case LEX_RIGHT_PARENTHESE: return snprintf(out, size, ")");

    case LEX_INTEGER:
        return snprintf(out, size, "%ld", (long int)t->value.integer);
    case LEX_UINTEGER:
        return snprintf(out, size, "%luu", (unsigned long)t->value.integer);
    case LEX_FLOATING_POINT:
        return snprintf(out, size, "%g", t->value.fp);

    case LEX_ADD: return snprintf(out, size, "+");
    case LEX_SUB: return snprintf(out, size, "-");
    case LEX_MUL: return snprintf(out, size, "*");
    case LEX_DIV: return snprintf(out, size, "/");
    case LEX_MOD: return snprintf(out, size, "%%");

    case LEX_AND: return snprintf(out, size, "&");
    case LEX_XOR: return snprintf(out, size, "^");
    case LEX_OR:  return snprintf(out, size, "|");
    case LEX_SHL: return snprintf(out, size, "<<");
    case LEX_SHR: return snprintf(out, size, ">>");

    case LEX_NOT: return snprintf(out, size, "!");
    case LEX_COMPL: return snprintf(out, size, "~");
    case LEX_CAST: return snprintf(out, size, "(T)");

    case LEX_EQ: return snprintf(out, size, "==");
    case LEX_NEQ: return snprintf(out, size, "!=");
    case LEX_LT: return snprintf(out, size, "<");
    case LEX_GT: return snprintf(out, size, ">");
    case LEX_LE: return snprintf(out, size, "<=");
    case LEX_GE: return snprintf(out, size, ">=");

    case LEX_AND_COND: return snprintf(out, size, "&&");
    case LEX_OR_COND: return snprintf(out, size, "||");

    default:
        break;
    }

    return snprintf(out, size, "???");
}
