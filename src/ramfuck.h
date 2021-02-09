#ifndef RAMFUCK_H_INCLUDED
#define RAMFUCK_H_INCLUDED

#include <stddef.h>
#include <stdint.h>

void infof(const char *format, ...);
void warnf(const char *format, ...);
void dbgf(const char *format, ...);
void errf(const char *format, ...);
void dief(const char *format, ...);

struct ramfuck {
    enum {
        DEAD = 0,
        RUNNING,
        QUITTING
    } state;
    int rc;

    struct config *config;
    struct linereader *linereader;

    struct target *target;
    int breaks;
    int addr_type;
    struct hits *hits;
    struct hits *undo;
    struct hits *redo;
};

#define ramfuck_dead(ctx) ((ctx)->state == DEAD)
#define ramfuck_running(ctx) ((ctx)->state == RUNNING)
#define ramfuck_attached(ctx) ((ctx)->pid != 0)

int ramfuck_init(struct ramfuck *ctx);
void ramfuck_destroy(struct ramfuck *ctx);
void ramfuck_quit(struct ramfuck *ctx);

char *ramfuck_get_line(struct ramfuck *ctx);
void ramfuck_free_line(struct ramfuck *ctx, char *line);

int ramfuck_break(struct ramfuck *ctx);
int ramfuck_continue(struct ramfuck *ctx);

void ramfuck_set_hits(struct ramfuck *ctx, struct hits *hits);
int ramfuck_undo(struct ramfuck *ctx);
int ramfuck_redo(struct ramfuck *ctx);

#endif
