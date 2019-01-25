#define _DEFAULT_SOURCE /* vsnprintf(3) */
#include "cli.h"
#include "ramfuck.h"

#include "config.h"
#include "eval.h"
#include "hits.h"
#include "lex.h"
#include "line.h"
#include "opt.h"
#include "parse.h"
#include "ptrace.h"
#include "search.h"
#include "symbol.h"
#include "target.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * Utility functions for parsing the CLI input.
 */
static int eol(const char *p)
{
    return !*p;
}

static void skip_spaces(const char **pin)
{
    while (**pin && isspace(**pin)) (*pin)++;
}

static int accept(const char **pin, const char *what)
{
    size_t i;
    for (i = 0; what[i] && what[i] == (*pin)[i]; i++);
    if (!what[i] && (!(*pin)[i] || isspace((*pin)[i]))) {
        *pin += i;
        skip_spaces(pin);
        return 1;
    }
    return 0;
}

static size_t fprint_value(FILE *stream, const struct value *value, int typed)
{
    char buf[256], *p;
    size_t len = value_to_string(value, (p = buf), sizeof(buf));
    if (len > sizeof(buf)-1) {
        if (len < SIZE_MAX && (p = malloc(len + 1))) {
            value_to_string(value, p, len + 1);
        } else {
            errf("cli: out-of-memory for %lu-byte value string", (unsigned long)len);
            p = buf;
        }
    }
    if (typed)
        len += fprintf(stream, "(%s)", value_type_to_string(value->type));
    fputs(p, stream);
    if (p != buf)
        free(p);
    return len;
}

/*
 * Attach to a process specified by PID.
 * Usage: attach <pid>
 */
static int do_attach(struct ramfuck *ctx, const char *in)
{
    char *end;
    unsigned long pid;

    if (eol(in)) {
        errf("attach: missing pid");
        return 1;
    }

    if (ctx->target) {
        errf("attach: already attached (detach first)");
        return 2;
    }

    pid = strtoul(in, &end, 10);
    if (*end || pid != (pid_t)pid) {
        errf("attach: invalid pid");
        return 3;
    }
    in = end;

    if (!eol(in)) {
        errf("attach: trailing characters after pid");
        return 4;
    }

    if (!(ctx->target = target_attach_pid((pid_t)pid))) {
        errf("attach: attaching to pid=%lu failed", (unsigned long)pid);
        return 5;
    }
    ctx->breaks = 0;

    infof("attached to process %lu", pid);
    return 0;
}

/*
 * Break (stop) target.
 * Usage: break
 */
static int do_break(struct ramfuck *ctx, const char *in)
{
    if (!eol(in)) {
        errf("break: trailing characters");
        return 1;
    }

    if (!ctx->target) {
        errf("break: not attached to any target");
        return 2;
    }

    if (ctx->breaks > 0) {
        errf("break: target is already stopped");
        return 3;
    }

    if (!ramfuck_break(ctx)) {
        errf("break: stopping failed");
        return 3;
    }

    infof("target stopped");
    return 0;
}

/*
 * Clear hits.
 * Usage: hits
 */
static int do_clear(struct ramfuck *ctx, const char *in)
{
    if (!eol(in)) {
        errf("clear: trailing characters");
        return 1;
    }

    ramfuck_set_hits(ctx, NULL);
    return 0;
}

/*
 * Get or set config.
 * Usage: config
 *        config <item>
 *        config <item> = <value>
 */
static int do_config(struct ramfuck *ctx, const char *in)
{
    return !config_process_line(ctx->config, in);
}

/*
 * Continue target.
 * Usage: continue
 */
static int do_continue(struct ramfuck *ctx, const char *in)
{
    int breaks;

    if (!eol(in)) {
        errf("continue: trailing characters");
        return 1;
    }

    if (!ctx->target) {
        errf("continue: not attached to any target");
        return 2;
    }

    if (!ctx->breaks) {
        errf("continue: target is already running");
        return 3;
    }

    breaks = ctx->breaks;
    ctx->breaks = 1;
    if (!ramfuck_continue(ctx)) {
        ctx->breaks = breaks;
        errf("continue: continuing failed");
        return 4;
    }

    infof("target continued");
    return 0;
}

/*
 * Detach from target.
 * Usage: detach
 */
static int do_detach(struct ramfuck *ctx, const char *in)
{
    if (!eol(in)) {
        errf("detach: trailing characters");
        return 1;
    }
    if (!ctx->target) {
        errf("detach: not attached to any target");
        return 1;
    }

    if (!ctx->breaks && !ramfuck_break(ctx))
        warnf("detach: stopping target failed");

    ctx->target->detach(ctx->target);
    ctx->target = NULL;
    infof("detached");
    return 0;
}

/*
 * Explain expression, i.e., print in Reverse Polish Notation (RPN).
 * Usage: explain <expr>
 */
static int do_explain(struct ramfuck *ctx, const char *in)
{
    int rc;
    struct symbol_table *symtab;

    if (eol(in)) {
        errf("explain: expression expected");
        return 1;
    }

    /* Isn't it pretty? Like a Down's child */
    if ((symtab = symbol_table_new(ctx))) {
        struct parser parser;
        struct value value;
        struct ast *ast;
        value_init_s32(&value, 42);
        symbol_table_add(symtab, "value", S32, &value.data);
        parser_init(&parser);
        parser.symtab = symtab;

        if ((ast = parse_expression(&parser, in))) {
            struct value out;
            printf("rpn: ");
            ast_print(ast);
            printf("\n");

            if (ast_evaluate(ast, &out)) {
                struct ast *ast_opt;
                if ((ast_opt = ast_optimize(ast))) {
                    struct value out_opt;
                    printf("opt: ");
                    ast_print(ast_opt);
                    printf("\n");

                    if (ast_evaluate(ast_opt, &out_opt)) {
                        struct ast *ast_check;
                        struct ast *l = ast_value_new(&out);
                        struct ast *r = ast_value_new(&out_opt);
                        if (l && l->value_type < S32)
                            l = ast_cast_new(S32, l);
                        if (r && r->value_type < S32)
                            r = ast_cast_new(S32, r);
                        if (l && r && (ast_check = ast_eq_new(l, r))) {
                            struct value out_check;
                            if (ast_evaluate(ast_check, &out_check)) {
                                if (value_is_nonzero(&out_check)) {
                                    fprint_value(stdout, &out, 1);
                                    fputc('\n', stdout);
                                    rc = 0;
                                } else {
                                    errf("explain: invalid optimization");
                                    rc = 9;
                                }
                            } else {
                                errf("explain: evaluation of check AST failed");
                                rc = 8;
                            }
                            ast_delete(ast_check);
                        } else {
                            ast_delete(l);
                            ast_delete(r);
                            errf("explain: creation of check AST failed");
                            rc = 7;
                        }
                    } else {
                        errf("explain: evaluation of optimized AST failed");
                        rc = 6;
                    }
                    ast_delete(ast_opt);
                } else {
                    errf("explain: optimization of AST faild");
                    rc = 5;
                }
            } else {
                errf("explain: evaluation of AST failed");
                rc = 4;
            }
            ast_delete(ast);
        } else {
            errf("explain: %d parse errors", parser.errors);
            rc = 3;
        }
        symbol_table_delete(symtab);
    } else {
        errf("explain: creating a symbol table failed");
        rc = 2;
    }

    return rc;
}

/*
 * Filter current hits.
 * Usage: filter <expression>
 */
static int do_filter(struct ramfuck *ctx, const char *in)
{
    struct hits *hits;

    if (eol(in)) {
        errf("filter: expression expected");
        return 1;
    }

    if (!ctx->hits || !ctx->hits->size) {
        infof("filter: zero hits");
        return 2;
    }

    hits = filter(ctx, ctx->hits, in);
    if (hits == ctx->hits)
        return 3;

    ramfuck_set_hits(ctx, hits);
    return 0;
}

/*
 * List current hits.
 * Usage: list
 */
static int do_list(struct ramfuck *ctx, const char *in)
{
    size_t i;
    struct target *tg;

    if (!eol(in)) {
        errf("list: trailing characters");
        return 1;
    }

    if (!ctx->hits || !ctx->hits->size) {
        infof("list: zero hits");
        return 0;
    }

    tg = ctx->target;
    ramfuck_break(ctx);
    for (i = 0; i < ctx->hits->size; i++) {
        struct value value;
        struct hit *hit = &ctx->hits->items[i];
        value.type = hit->type;
        fprintf(stdout, "%lu. *(%s *)%p = ", (unsigned long)(i+1),
                value_type_to_string(hit->type), (void *)hit->addr);

        if (tg && tg->read(tg, hit->addr, &value.data, value_sizeof(&value))) {
            fprint_value(stdout, &value, 0);
        } else {
            fprintf(stdout, "???");
        }
        fputc('\n', stdout);
    }
    ramfuck_continue(ctx);

    return 0;
}

/*
 * Show memory maps of target.
 * Usage: maps
 */
static int do_maps(struct ramfuck *ctx, const char *in)
{
    char *buf;
    size_t size;
    struct target *target;
    struct region *mr;

    if (!eol(in)) {
        errf("maps: trailing characters");
        return 1;
    }

    if (!ctx->target) {
        errf("maps: attach first");
        return 2;
    }

    if (!(buf = malloc((size = 128)))) {
        errf("maps: cannot allocate line buffer");
        return 3;
    }

    target = ctx->target;
    for (mr = target->region_first(target); mr; mr = target->region_next(mr)) {
        size_t len = region_snprint(mr, buf, size);
        if (len + 1 > size) {
            char *new = realloc(buf, len + 1);
            if (new) {
                buf = new;
                size = len + 1;
                if (region_snprint(mr, buf, size) != len)
                    warnf("maps: inconsistent memory region line formatting");
            } else {
                errf("maps: error reallocating line buffer (will truncate)");
            }
        }
        fprintf(stdout, "%s\n", buf);
    }

    free(buf);
    return 0;
}

/*
 * Peek address.
 * Usage: peek <type> <addr>
 *        peek <hit_index>
 */
static int do_peek(struct ramfuck *ctx, const char *in)
{
    const char *p;
    enum value_type type;
    struct parser parser;
    struct ast *ast, *cast;
    uint64_t addr;
    int64_t index;
    struct value out;
    int ret;

    if (eol(in)) {
        errf("peek: type & addr or index expected");
        return 1;
    }

    type = 0;
    skip_spaces(&in);
    for (p = in; *p && !isspace(*p); p++);
    if ((type = value_type_from_substring(in, (size_t)(p - in)))) {
        skip_spaces(&p);
        in = p;
    }
    if (eol(in)) {
        errf("peek: address expression expected");
        return 2;
    }

    parser_init(&parser);
    if (!(ast = parse_expression(&parser, in))) {
        errf("peek: %d parse errors", parser.errors);
        return 3;
    }
    if (!(cast = ast_cast_new(type ? U64 : S64, ast))) {
        errf("peek: out-of-memory for typecast AST");
        ast_delete(ast);
        return 4;
    }
    ast = cast;

    ret = ast_evaluate(ast, &out);
    ast_delete(ast);
    if (!ret) {
        errf("peek: evaluating %s failed", (type ? "address" : "hit index"));
        return 5;
    }

    if (type) {
        addr = out.data.u64;
        index = -1;
    } else {
        if (!ctx->hits || !ctx->hits->size) {
            errf("peek: bad index %ld (0 hits)", out.data.s64);
            return 6;
        }
        index = out.data.s64;
        index = (index < 0) ? index + ctx->hits->size : index - 1;
        if (0 <= index && index < ctx->hits->size) {
            addr = ctx->hits->items[index].addr;
            type = ctx->hits->items[index].type;
        } else {
            errf("peek: bad index %ld not in 1..%lu",
                 (long)out.data.s64, (unsigned long)ctx->hits->size);
            return 6;
        }
    }

    out.type = type;
    if (index != -1)
        fprintf(stdout, "%ld. ", (long)(index + 1));
    fprintf(stdout, "*(%s *)%p = ", value_type_to_string(type), (void *)addr);
    if (ramfuck_read(ctx, addr, &out.data, value_sizeof(&out))) {
        fprint_value(stdout, &out, 0);
    } else {
        fprintf(stdout, "%s", "???");
    }
    fputc('\n', stdout);
    return 0;
}

/*
 * Poke value.
 * Usage: poke <type> <addr> <value>
 *        poke <hit_index> <value>
 */
static int do_poke(struct ramfuck *ctx, const char *in)
{
    const char *p;
    enum value_type type;
    long index;
    uintptr_t addr;
    uint32_t addr32;
    uint64_t addr64;
    struct symbol_table *symtab;
    struct parser parser;
    struct ast *ast, *cast;
    struct value out;
    int ret;

    if (eol(in)) {
        errf("poke: type & addr & value or index & value expected");
        return 1;
    }

    if (!ctx->target) {
        errf("poke: attach to target first");
        return 2;
    }

    type = 0;
    skip_spaces(&in);
    for (p = in; *p && !isspace(*p); p++);
    if ((type = value_type_from_substring(in, (size_t)(p - in)))) {
        struct lex_token token;
        skip_spaces(&p);
        if (eol(p)) {
            errf("poke: address expected");
            return 3;
        }
        if (!lexer(&p, &token) || (token.type != LEX_INTEGER
                                && token.type != LEX_UINTEGER)) {
            errf("poke: invalid address");
            return 4;
        }
        addr = (uintptr_t)token.value.integer;
        in = p;
    }

    if (type) {
        index = -1;
    } else {
        long index1;
        char *end;
        if (!isdigit(*(in + (*in == '-')))) {
            errf("poke: invalid hit index (not a number)");
            return 5;
        }
        if (!ctx->hits || !ctx->hits->size) {
            errf("poke: bad index %ld (0 hits)");
            return 6;
        }
        index1 = strtol(in, &end, 0);
        index = (index1 < 0) ? index1 + ctx->hits->size : index1 - 1;
        if (!(0 <= index && index < ctx->hits->size)) {
            errf("poke: bad index %ld not in 1..%lu",
                 index1, (unsigned long)ctx->hits->size);
            return 6;
        }
        addr = ctx->hits->items[index].addr;
        type = ctx->hits->items[index].type;
        in = end;
    }

    skip_spaces(&in);
    if (eol(in)) {
        errf("poke: value expression expected");
        return 7;
    }

    parser_init(&parser);
    if (strstr(in, "addr") || strstr(in, "value") /* hack ... */) {
        if (!(symtab = symbol_table_new(ctx))) {
            errf("poke: out-of-memory for symbol table");
            return 9;
        }
        if (strstr(in, "addr")) {
            if (addr <= UINT32_MAX) {
                addr32 = (uint32_t)addr;
                symbol_table_add(symtab, "addr", U32, (void *)&addr32);
            } else {
                addr64 = (uint64_t)addr;
                symbol_table_add(symtab, "addr", U64, (void *)&addr64);
            }
        }
        if (strstr(in, "value")) {
            out.type = type;
            if (ramfuck_read(ctx, addr, &out.data, value_sizeof(&out))) {
                symbol_table_add(symtab, "value", out.type, &out.data);
            } else {
                errf("poke: error reading %lu-byte value from address %p",
                     (unsigned long)value_sizeof(&out), (void *)addr);
                return 10;
            }
        }
        parser.symtab = symtab;
    } else {
        symtab = NULL;
    }

    if (!(ast = parse_expression(&parser, in))) {
        errf("poke: %d parse errors", parser.errors);
        return 11;
    }
    if (!(cast = ast_cast_new(type, ast))) {
        errf("poke: out-of-memory for typecast AST");
        if (symtab) symbol_table_delete(symtab);
        ast_delete(ast);
        return 12;
    }
    ast = cast;

    ret = ast_evaluate(ast, &out);
    if (symtab) symbol_table_delete(symtab);
    ast_delete(ast);
    if (!ret) {
        errf("poke: evaluating value expression failed");
        return 13;
    }

    if (!ramfuck_write(ctx, addr, &out.data, value_sizeof(&out))) {
        errf("poke: error writing %lu bytes to address %p",
             (unsigned long)value_sizeof(&out), (void *)addr);
        return 14;
    }

    if (index != -1)
        fprintf(stdout, "%ld. ", index + 1);
    fprintf(stdout, "*(%s *)%p = ", value_type_to_string(type), (void *)addr);
    fprint_value(stdout, &out, 0);
    fputc('\n', stdout);
    return 0;
}

/*
 * Undo undo.
 * Usage: redo
 */
static int do_redo(struct ramfuck *ctx, const char *in)
{
    if (!eol(in)) {
        errf("redo: trailing characters");
        return 1;
    }

    if (!ramfuck_redo(ctx)) {
        errf("redo: redo unavailable");
        return 2;
    }

    return 0;
}

/*
 * Initial search.
 * Usage: search <expression>
 *        search <type> <expression>
 * where 'type' is one of: s8, u8, s16, u16, s32, u32, s64, u64, f32, f64.
 */
static int do_search(struct ramfuck *ctx, const char *in)
{
    const char *p;
    enum value_type type;
    struct hits *hits;

    if (eol(in)) {
        errf("search: missing type");
        return 1;
    }

    if (!ctx->target) {
        errf("search: attach to target first");
        return 2;
    }

    skip_spaces(&in);
    for (p = in; *p && !isspace(*p); p++);
    if (isspace(*p) && (type = value_type_from_substring(in, (size_t)(p - in))))
        hits = search(ctx, type, p);
    else hits = search(ctx, S32, in);

    if (!hits)
        return 3;

    ramfuck_set_hits(ctx, hits);
    return 0;
}

/*
 * Measure running time of a command.
 * Usage: time <command>
 */
static int do_time(struct ramfuck *ctx, const char *in)
{
    int rc;
    clock_t start, end;

    start = clock();
    rc = cli_execute_line(ctx, in);
    end = clock();

    printf("%gs\n", (double)(end - start) / CLOCKS_PER_SEC);
    return rc;
}

/*
 * Undo.
 * Usage: undo
 */
static int do_undo(struct ramfuck *ctx, const char *in)
{
    if (!eol(in)) {
        errf("undo: trailing characters");
        return 1;
    }

    if (!ramfuck_undo(ctx)) {
        errf("undo: undo unavailable");
        return 2;
    }

    return 0;
}

/*
 * Evaluate expression and print its value.
 * Usage: eval <expr>
 */
static int do_eval(struct ramfuck *ctx, const char *in)
{
    struct parser parser;
    struct ast *ast;
    parser_init(&parser);
    parser.quiet = 1;
    if ((ast = parse_expression(&parser, in))) {
        struct value out;
        if (ast_evaluate(ast, &out)) {
            fprint_value(stdout, &out, 0);
            fputc('\n', stdout);
        }
        ast_delete(ast);
    }
    return parser.errors;
}

int cli_execute_line(struct ramfuck *ctx, const char *in)
{
    int rc = 0;
    skip_spaces(&in);
    if (accept(&in, "and")) {
        rc = ctx->rc ? ctx->rc : cli_execute_line(ctx, in);
    } else if (accept(&in, "attach")) {
        rc = do_attach(ctx, in);
    } else if (accept(&in, "break")) {
        rc = do_break(ctx, in);
    } else if (accept(&in, "clear")) {
        rc = do_clear(ctx, in);
    } else if (accept(&in, "config")) {
        rc = do_config(ctx, in);
    } else if (accept(&in, "continue")) {
        rc = do_continue(ctx, in);
    } else if (accept(&in, "detach")) {
        rc = do_detach(ctx, in);
    } else if (accept(&in, "explain")) {
        rc = do_explain(ctx, in);
    } else if (accept(&in, "filter") || accept(&in, "next")) {
        rc = do_filter(ctx, in);
    } else if (accept(&in, "or")) {
        rc = ctx->rc ? cli_execute_line(ctx, in) : ctx->rc;
    } else if (accept(&in, "ls") || accept(&in, "list")) {
        rc = do_list(ctx, in);
    } else if (accept(&in, "m") || accept(&in, "maps") || accept(&in, "mem")) {
        rc = do_maps(ctx, in);
    } else if (accept(&in, "peek")) {
        rc = do_peek(ctx, in);
    } else if (accept(&in, "poke")) {
        rc = do_poke(ctx, in);
    } else if (accept(&in, "quit") || accept(&in, "q") || accept(&in, "exit")) {
        ramfuck_quit(ctx);
        rc = ctx->rc;
    } else if (accept(&in, "redo")) {
        rc = do_redo(ctx, in);
    } else if (accept(&in, "search")) {
        rc = do_search(ctx, in);
    } else if (accept(&in, "time")) {
        rc = do_time(ctx, in);
    } else if (accept(&in, "undo")) {
        rc = do_undo(ctx, in);
    } else if (!eol(in) && do_eval(ctx, in) != 0) {
        size_t i;
        for (i = 0; i < INT_MAX && in[i] && !isspace(in[i]); i++);
        errf("cli: unknown command '%.*s'", (int)i, in);
        rc = 1;
    }

    return (ctx->rc = rc);
}

int cli_execute(struct ramfuck *ctx, char *in)
{
    int rc = 0;
    char *start, *end, *comment;
    for (start = in; start; start = end ? end + 1 : NULL) {
        if ((end = strchr(start, '\n')))
            *end = '\0';
        if ((comment = strchr(start, '#')))
            *comment = '\0';
        rc = cli_execute_line(ctx, start);
        if (comment) *comment = '#';
        if (end) *end = '\n';
    }
    return rc;
}

int cli_execute_format(struct ramfuck *ctx, const char *format, ...)
{
    va_list args;
    char buf[BUFSIZ];
    size_t len;
    int rc;

    va_start(args, format);
    len = vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    if (len < sizeof(buf)-1) {
        rc = cli_execute(ctx, buf);
    } else {
        int ok;
        char *p;
        if ((p = malloc(len + 1))) {
            va_start(args, format);
            ok = vsnprintf(p, len + 1, format, args) == len;
            va_end(args);
            if (ok) {
                rc = cli_execute(ctx, p);
            } else {
                errf("cli: inconsistent command formatting");
                rc = 2;
            }
            free(p);
        } else {
            errf("cli: out-of-memory for formatted command");
            rc = 1;
        }
    }

    return rc;
}

int cli_main_loop(struct ramfuck *ctx)
{
    while (ramfuck_running(ctx)) {
        char *line = ramfuck_get_line(ctx);
        if (line) {
            cli_execute(ctx, line);
            ramfuck_free_line(ctx, line);
        } else {
            ramfuck_quit(ctx);
        }
    }
    return ctx->rc;
}
