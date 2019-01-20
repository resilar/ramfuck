#define _DEFAULT_SOURCE
#include "ramfuck.h"
#include "cli.h"
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
}

void ramfuck_destroy(struct ramfuck *ctx)
{
    if (!ramfuck_dead(ctx)) {
        ctx->state = DEAD;
        if (ctx->linereader)
            linereader_close(ctx->linereader);
        if (ctx->mem) {
            if (ctx->mem->attached(ctx->mem))
                ctx->mem->detach(ctx->mem);
            mem_io_put(ctx->mem);
        }
    }
}

void ramfuck_stop(struct ramfuck *ctx)
{
    ctx->state = STOPPING;
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
        long int hits = 0;
        size_t len = snprintf(NULL, 0, "%ld> ", hits);
        if (0 < len && len < SIZE_MAX) {
            if (len + 1 > sizeof(buf)) {
                if (!(prompt = malloc(len + 1)))
                    errf("ramfuck: out-of-memory for get_line prompt");
            } else {
                prompt = buf;
            }
        }
        if (prompt)
            snprintf(prompt, len + 1, "%ld> ", hits);
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

int main(int argc, char *argv[])
{
    int rc = cli_run(stdin);
    return rc;
}
