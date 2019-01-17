#ifndef RAMFUCK_H_INCLUDED
#define RAMFUCK_H_INCLUDED

#include <stdint.h>
#include <sys/types.h>

void infof(const char *format, ...);
void warnf(const char *format, ...);
void dbgf(const char *format, ...);
void errf(const char *format, ...);
void dief(const char *format, ...);

struct ramfuck_context {
    int running;
    pid_t pid;
};

void ramfuck_context_init(struct ramfuck_context *ctx);
void ramfuck_context_destroy(struct ramfuck_context *ctx);

int main(int argc, char *argv[]);

#endif
