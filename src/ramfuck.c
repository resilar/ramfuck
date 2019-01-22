#define _DEFAULT_SOURCE /* snprintf(3) vfprintf(3) */
#include "ramfuck.h"
#include "cli.h"
#include "hits.h"
#include "line.h"
#include "mem.h"
#include "ptrace.h"

#include <stdarg.h>
#include <stdlib.h>

void infof(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    fputc('\n', stdout);
    va_end(args);
}

void warnf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    fputc('\n', stderr);
    va_end(args);
}

void dbgf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    fputc('\n', stdout);
    va_end(args);
}

void errf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    fputc('\n', stderr);
    va_end(args);
}

void dief(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    fputc('\n', stderr);
    va_end(args);
    abort();
}

void ramfuck_init(struct ramfuck *ctx)
{
    ctx->state = RUNNING;
    ctx->linereader = NULL;
    ctx->mem = mem_io_get(ctx);
    ctx->breaks = 0;
    ctx->hits = NULL;
}

void ramfuck_destroy(struct ramfuck *ctx)
{
    if (!ramfuck_dead(ctx)) {
        ctx->state = DEAD;
        if (ctx->linereader)
            linereader_close(ctx->linereader);
        if (ctx->mem) {
            if (ctx->mem->attached(ctx->mem)) {
                if (!ctx->breaks)
                    ctx->mem->target_break(ctx->mem);
                ctx->mem->detach(ctx->mem);
            }
            mem_io_put(ctx->mem);
        }
        ctx->breaks = 0;
        if (ctx->hits) {
            hits_delete(ctx->hits);
        }
    }
}

void ramfuck_quit(struct ramfuck *ctx)
{
    ctx->state = QUITTING;
}

void ramfuck_set_input_stream(struct ramfuck *ctx, FILE *in)
{
    if (ctx->linereader)
        linereader_close(ctx->linereader);
    ctx->linereader = linereader_get(in);
}

void ramfuck_close_input_stream(struct ramfuck *ctx)
{
    linereader_close(ctx->linereader);
}

char *ramfuck_get_line(struct ramfuck *ctx)
{
    char buf[64], *line, *prompt = NULL;

    if (isatty(STDOUT_FILENO)) {
        unsigned long hits = (unsigned long)(ctx->hits ? ctx->hits->size : 0);
        size_t len = snprintf(NULL, 0, "%lu> ", hits);
        if (0 < len && len < SIZE_MAX) {
            if (len + 1 > sizeof(buf)) {
                if (!(prompt = malloc(len + 1)))
                    errf("ramfuck: out-of-memory for get_line prompt");
            } else {
                prompt = buf;
            }
        }
        if (prompt)
            snprintf(prompt, len + 1, "%lu> ", hits);
    }

    line = linereader_get_line(ctx->linereader, prompt);

    if (prompt && prompt != buf)
        free(prompt);
    return line;
}

void ramfuck_free_line(struct ramfuck *ctx, char *line)
{
    linereader_free_line(ctx->linereader, line);
}

int ramfuck_read(struct ramfuck *ctx, uintptr_t addr, void *buf, size_t len)
{
    int ret;
    ramfuck_break(ctx);
    ret = ctx->mem->read(ctx->mem, addr, buf, len);
    ramfuck_continue(ctx);
    return ret;
}

int ramfuck_write(struct ramfuck *ctx, uintptr_t addr, void *buf, size_t len)
{
    int ret;
    ramfuck_break(ctx);
    ret = ctx->mem->write(ctx->mem, addr, buf, len);
    ramfuck_continue(ctx);
    return ret;
}

int ramfuck_break(struct ramfuck *ctx)
{
    if (ctx->breaks > 0 || ctx->mem->target_break(ctx->mem)) {
        ctx->breaks++;
        return 1;
    }
    return 0;
}

int ramfuck_continue(struct ramfuck *ctx)
{
    if (ctx->breaks > 0) {
        if (ctx->breaks > 1 || ctx->mem->target_continue(ctx->mem))
            ctx->breaks--;
        return 1;
    }
    return 0;
}

void ramfuck_set_hits(struct ramfuck *ctx, struct hits *hits)
{
    if (ctx->hits)
        hits_delete(ctx->hits);
    ctx->hits = hits;
}

int main(int argc, char *argv[])
{
    struct ramfuck ctx;
    int rc = 0;

    ramfuck_init(&ctx);
    ramfuck_set_input_stream(&ctx, stdin);

    if (argc == 2) {
       char *end;
       unsigned long pidlu = strtoul(argv[1], &end, 10);
       if (*argv[1] && !*end) {
           if ((rc = cli_execute_format(&ctx, "attach %lu", pidlu))) {
               errf("main: error attaching to pid '%s'", argv[1]);
           }
       } else {
           errf("main: bad pid '%s'", argv[1]);
           rc = 1;
       }
    }

    rc = rc ? rc : cli_main_loop(&ctx);

    ramfuck_destroy(&ctx);
    return rc;
}
