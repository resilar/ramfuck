#include "ramfuck.h"
#include "cli.h"
#include "ptrace.h"

#include <stdarg.h>
#include <stdio.h>
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

void ramfuck_context_init(struct ramfuck_context *ctx)
{
    ctx->running = 1;
    ctx->pid = 0;
}

void ramfuck_context_destroy(struct ramfuck_context *ctx)
{
    ctx->running = 0;
    if (ctx->pid)
        ptrace_detach(ctx->pid);
}

int main(int argc, char *argv[])
{
    int rc = cli_run(stdin);
    return rc;
}
