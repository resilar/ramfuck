#include "cli.h"
#include "ramfuck.h"

#include "eval.h"
#include "line.h"
#include "opt.h"
#include "parse.h"
#include "ptrace.h"
#include "symbol.h"

#include <ctype.h>
#include <errno.h>
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

/*
 * Attach to a process.
 * Usage: attach pid
 */
static int do_attach(struct ramfuck_context *ctx, const char *in)
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
static int do_detach(struct ramfuck_context *ctx, const char *in)
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

static int do_p(struct ramfuck_context *ctx, const char *in)
{
    struct parser p;
    struct symbol_table *symtab;
    struct ast *ast, *ast_opt;
    struct value value, out;

    if (eol(in)) {
        errf("p: expression expected");
        return 1;
    }

    symtab = symbol_table_new();
    value_init_s32(&value, 42);
    symbol_table_insert(symtab, symbol_new("value", &value));

    parser_init(&p, symtab, in);
    ast = parse_expression(&p);
    if (p.errors > 0 || !ast) {
        printf("got %d parse errors\n", p.errors);
        return p.errors + !ast;
    }

    printf("rpn: ");
    ast_print(ast);
    printf("\n");

    if (ast_evaluate(ast, &out)) {
        printf("ret: (%s)%s\n", value_type_to_string(out.type),
                value_to_string(&out));
    } else printf("evaluation failed\n");

    printf("-------------------\n");
    printf("rpn: ");
    ast_opt = ast_optimize(ast);
    ast_print(ast_opt);
    printf(" [opt]\n");

    if (ast_evaluate(ast_opt, &out)) {
        printf("ret: (%s)%s [opt]\n", value_type_to_string(out.type),
                value_to_string(&out));
    } else printf("opt evaluation failed\n");

    ast_delete(ast_opt);
    ast_delete(ast);
    symbol_table_delete(symtab);

    return 0;
}

static int cli_execute(struct ramfuck_context *ctx, const char *in)
{
    /*const char *cmd;*/
    int rc = 0;
    skip_spaces(&in);
    dbgf("execute(%s)", in);
    if (accept(&in, "attach")) {
        rc = do_attach(ctx, in);
    } else if (accept(&in, "detach")) {
        rc = do_detach(ctx, in);
    } else if (accept(&in, "p")) {
        rc = do_p(ctx, in);
    } else if (accept(&in, "exit") || accept(&in, "quit") || accept(&in, "q")) {
        ctx->running = 0;
    }
    return rc;
}

int cli_run(FILE *in)
{
    char *line;
    struct ramfuck_context ctx;
    struct linereader *reader = linereader_get();
    int rc = 0;
    ramfuck_context_init(&ctx);
    while (ctx.running) {
        /* Prompt */
        printf("%d> ", rc);
        fflush(stdout);

        /* Read line */
        line = reader->get_line(reader, in);
        if (!line) break;

        /* Execute */
        rc = cli_execute(&ctx, line);
        reader->add_history(reader, line);
    }
    linereader_put(reader);
    ramfuck_context_destroy(&ctx);
    return rc;
}
