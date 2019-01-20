#include "cli.h"
#include "ramfuck.h"

#include "eval.h"
#include "line.h"
#include "mem.h"
#include "opt.h"
#include "parse.h"
#include "ptrace.h"
#include "symbol.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Utility function for parsing the CLI input.
 */
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

static int eol(const char *p)
{
    return *p == '\0';
}

static size_t fprint_value(FILE *stream, const struct value *value, int typed)
{
    char buf[256], *p;
    size_t len = value_to_string_r(value, (p = buf), sizeof(buf));
    if (len > sizeof(buf)-1) {
        if (len < SIZE_MAX && (p = malloc(len + 1))) {
            value_to_string_r(value, p, len + 1);
        } else {
            errf("p: out-of-memory for %lu-byte value string", (unsigned long)len);;
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
    dbgf("ptrace(%s) attach", in);

    if (eol(in)) {
        errf("attach: missing pid");
        return 1;
    }

    pid = strtoul(in, &end, 10);
    if (*end != '\0' || pid != (pid_t)pid) {
        errf("attach: invalid pid");
        return 1;
    }
    in = end;

    if (!eol(in)) {
        errf("attach: trailing characters after pid");
        return 1;
    }

    /* Confirm that we can attach to the process with PTRACE_ATTACH */
    if (ptrace_attach((pid_t)pid)) {
        infof("attached to process %lu", pid);
        ctx->pid = pid;
    }
    return 0;
}

/*
 * Detach from the target process.
 * Usage: detach
 */
static int do_detach(struct ramfuck *ctx, const char *in)
{
    if (!eol(in)) {
        errf("detach: trailing characters");
        return 1;
    }
    if (!ctx->pid) {
        errf("detach: not attached to any process");
        return 1;
    }

    ptrace_detach(ctx->pid);
    infof("detach: detached from %lu", (unsigned long)ctx->pid);
    ctx->pid = 0;
    return 0;
}

/*
 * Explain expression, i.e., print in Reverse Polish Notation RPN).
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
        int errors;
        struct value value;
        struct ast *ast;
        value_init_s32(&value, 42);
        symbol_table_insert(symtab, symbol_new("value", &value));

        errors = parse_expression(in, symtab, 0, &ast);
        if (errors == 0) {
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
                        if (l && r && (ast_check = ast_eq_new(l, r))) {
                            struct value out_check;
                            if (ast_evaluate(ast_check, &out_check)) {
                                if (value_is_nonzero(&out_check)) {
                                    printf("(%s)%s\n",
                                           value_type_to_string(out.type),
                                           value_to_string(&out));
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
            errf("explain: %d parse errors", errors);
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
 * Show memory maps of the attached process.
 * Usage: maps
 */
static int do_maps(struct ramfuck *ctx, const char *in)
{
    struct mem_io *mem;
    struct mem_region *mr;

    if (!eol(in)) {
        errf("maps: trailing characters");
        return 1;
    }

    if (!ctx->pid) {
        errf("maps: attach to process first (pid=0)");
        return 2;
    }

    mem = ctx->mem;
    for (mr = mem->region_first(mem); mr; mr = mem->region_next(mr)) {
        fprintf(stdout, "%p-%p %c%c%c",
                (void *)mr->start, (void *)(mr->start + mr->size),
                (mr->prot & MEM_READ) ? 'r' : '-',
                (mr->prot & MEM_WRITE) ? 'w' : '-',
                (mr->prot & MEM_EXECUTE) ? 'x' : '-');
        if (mr->path) {
            fputc(' ', stdout);
            fprintf(stdout, "%s", mr->path);
        }
        fputc('\n', stdout);
    }

    return 0;
}

/*
 * Print expression and its value.
 * Usage: p <expr>
 */
static int do_p(struct ramfuck *ctx, const char *in)
{
    int rc, errors;
    struct ast *ast;
    struct value out;

    if (eol(in)) {
        errf("p: expression expected");
        return 1;
    }

    errors = parse_expression(in, NULL, 0, &ast);
    if (errors > 0) {
        errf("p: %d parse errors", errors);
        return 2;
    }

    printf("rpn: ");
    ast_print(ast);
    printf("\n");

    rc = 0;
    if (ast_evaluate(ast, &out)) {
        fprint_value(stdout, &out, 1);
        fputc('\n', stdout);
    } else {
        errf("p: evaluation failed");
        rc = 3;
    }
    ast_delete(ast);

    return rc;
}

/*
 * Evaluate expression and print its value.
 * Usage: eval <expr>
 */
static int do_eval(struct ramfuck *ctx, const char *in)
{
    struct ast *ast;
    int errors = parse_expression(in, NULL, 1, &ast);
    if (errors == 0) {
        struct value out;
        if (ast_evaluate(ast, &out)) {
            fprint_value(stdout, &out, 0);
            fputc('\n', stdout);
        }
        ast_delete(ast);
    }
    return errors;
}

static int cli_execute(struct ramfuck *ctx, const char *in)
{
    int rc = 0;
    skip_spaces(&in);
    if (accept(&in, "attach")) {
        rc = do_attach(ctx, in);
    } else if (accept(&in, "detach")) {
        rc = do_detach(ctx, in);
    } else if (accept(&in, "explain")) {
        rc = do_explain(ctx, in);
    } else if (accept(&in, "exit") || accept(&in, "quit") || accept(&in, "q")) {
        ramfuck_stop(ctx);
        rc = 0;
    } else if (accept(&in, "m") || accept(&in, "maps") || accept(&in, "mem")) {
        rc = do_maps(ctx, in);
    } else if (accept(&in, "p")) {
        rc = do_p(ctx, in);
    } else if (!eol(in) && do_eval(ctx, in) != 0) {
        size_t i;
        for (i = 0; i < INT_MAX && in[i] && !isspace(in[i]); i++);
        errf("cli: unknown command '%.*s'", (int)i, in);
    }
    return rc;
}

int cli_run(FILE *in)
{
    struct ramfuck ctx;
    int rc = 0;
    ramfuck_init(&ctx);
    ramfuck_set_input_stream(&ctx, in);
    while (ramfuck_running(&ctx)) {
        char *line = ramfuck_get_line(&ctx);
        if (line) {
            rc = cli_execute(&ctx, line);
            ramfuck_free_line(&ctx, line);
        } else {
            ramfuck_stop(&ctx);
        }
    }
    ramfuck_destroy(&ctx);
    return rc;
}
