#ifndef RAMFUCK_H_INCLUDED
#define RAMFUCK_H_INCLUDED

#include <stdio.h>
#include <sys/types.h>

void infof(const char *format, ...);
void warnf(const char *format, ...);
void dbgf(const char *format, ...);
void errf(const char *format, ...);
void dief(const char *format, ...);

enum ramfuck_state {
    DEAD = 0,
    RUNNING,
    STOPPING
};

struct ramfuck {
    enum ramfuck_state state;

    struct linereader *linereader;
    struct mem_io *mem;
    
    struct hits *hits;
};

#define ramfuck_dead(ctx) ((ctx)->state == DEAD)
#define ramfuck_running(ctx) ((ctx)->state == RUNNING)
#define ramfuck_attached(ctx) ((ctx)->pid != 0)

void ramfuck_init(struct ramfuck *ctx);
void ramfuck_destroy(struct ramfuck *ctx);
void ramfuck_stop(struct ramfuck *ctx);

void ramfuck_set_input_stream(struct ramfuck *ctx, FILE *in);
char *ramfuck_get_line(struct ramfuck *ctx);
void ramfuck_free_line(struct ramfuck *ctx, char *line);

void ramfuck_set_hits(struct ramfuck *ctx, struct hits *hits);

int main(int argc, char *argv[]);

#endif
