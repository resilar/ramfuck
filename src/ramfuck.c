#define _DEFAULT_SOURCE /* snprintf(3) vfprintf(3) */
#include "ramfuck.h"
#include "config.h"
#include "cli.h"
#include "hits.h"
#include "line.h"
#include "ptrace.h"
#include "target.h"

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

int ramfuck_init(struct ramfuck *ctx)
{
    ctx->state = RUNNING;
    ctx->rc = 0;
    if (!(ctx->config = config_new()))
        return 0;
    ctx->linereader = NULL;
    ctx->target = NULL;
    ctx->breaks = 0;
    ctx->hits = NULL;
    ctx->undo = NULL;
    ctx->redo = NULL;
    return 1;
}

void ramfuck_destroy(struct ramfuck *ctx)
{
    if (!ramfuck_dead(ctx)) {
        ctx->state = DEAD;
        ctx->rc = 0;
        if (ctx->config) {
            config_delete(ctx->config);
            ctx->config = NULL;
        }
        if (ctx->linereader) {
            linereader_close(ctx->linereader);
            ctx->linereader = NULL;
        }
        if (ctx->target) {
            if (!ctx->breaks)
                ctx->target->stop(ctx->target);
            target_detach(ctx->target);
            ctx->target = NULL;
        }
        ctx->breaks = 0;
        if (ctx->hits) {
            hits_delete(ctx->hits);
            ctx->hits = NULL;
        }
        if (ctx->undo) {
            hits_delete(ctx->undo);
            ctx->undo = NULL;
        }
        if (ctx->redo) {
            hits_delete(ctx->redo);
            ctx->redo = NULL;
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
    if (ctx->target) {
        ramfuck_break(ctx);
        ret = ctx->target->read(ctx->target, addr, buf, len);
        ramfuck_continue(ctx);
    } else {
        ret = 0;
    }
    return ret;
}

int ramfuck_write(struct ramfuck *ctx, uintptr_t addr, void *buf, size_t len)
{
    int ret;
    if (ctx->target) {
        ramfuck_break(ctx);
        ret = ctx->target->write(ctx->target, addr, buf, len);
        ramfuck_continue(ctx);
    } else {
        ret = 0;
    }
    return ret;
}

int ramfuck_break(struct ramfuck *ctx)
{
    if (ctx->target && (ctx->breaks > 0 || ctx->target->stop(ctx->target))) {
        ctx->breaks++;
        return 1;
    }
    return 0;
}

int ramfuck_continue(struct ramfuck *ctx)
{
    if (ctx->target && ctx->breaks > 0) {
        if (ctx->breaks > 1 || ctx->target->run(ctx->target))
            ctx->breaks--;
        return 1;
    }
    return 0;
}

void ramfuck_set_hits(struct ramfuck *ctx, struct hits *hits)
{
    if (ctx->hits != hits) {
        if (ctx->undo)
            hits_delete(ctx->undo);
        ctx->undo = ctx->hits;
        ctx->hits = hits;
        if (ctx->redo) {
            hits_delete(ctx->redo);
            ctx->redo = NULL;
        }
    }
}

int ramfuck_undo(struct ramfuck *ctx)
{
    if (ctx->undo) {
        if (ctx->redo)
            hits_delete(ctx->redo);
        ctx->redo = ctx->hits;
        ctx->hits = ctx->undo;
        ctx->undo = NULL;
        return 1;
    }
    return 0;
}

int ramfuck_redo(struct ramfuck *ctx)
{
    if (ctx->redo) {
        if (ctx->undo)
            hits_delete(ctx->undo);
        ctx->undo = ctx->hits;
        ctx->hits = ctx->redo;
        ctx->redo = NULL;
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    struct ramfuck ctx;
    int rc = 0;

    if (!ramfuck_init(&ctx)) {
        errf("main: out-of-memory for ramfuck context");
        return 1;
    }

    if (argc == 2) {
       char *end;
       unsigned long pidlu = strtoul(argv[1], &end, 10);
       if (*argv[1] && !*end) {
           rc = cli_execute_format(&ctx, "attach %lu", pidlu);
       } else {
           errf("main: bad pid '%s'", argv[1]);
           rc = 1;
       }
    }

    if (!rc) {
        ramfuck_set_input_stream(&ctx, stdin);
        rc = cli_main_loop(&ctx);
    }

    ramfuck_destroy(&ctx);
    return rc;
}
