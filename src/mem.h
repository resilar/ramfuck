#ifndef MEM_H_INCLUDED
#define MEM_H_INCLUDED

#include "ramfuck.h"

#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

struct mem_region;

struct mem_io {
    struct ramfuck *ctx;

    struct mem_region *(*mem_region_first)();
    struct mem_region *(*mem_region_next)(struct mem_region *);
    void (*mem_region_put)(struct mem_region *);
};

enum mem_prot {
    MEM_EXEC = 1,
    MEM_WRITE = 2,
    MEM_READ = 4
};

struct mem_region {
    uintptr_t start;
    size_t size;
    enum mem_prot prot;
    char *path;
};

/* Iteration of PID memory regions */
struct mem_region *mem_region_iter_first(pid_t pid);
struct mem_region *mem_region_iter_next(struct mem_region *it);

/* Find memory region based on address or pathname */
struct mem_region *mem_region_find(pid_t pid, uintptr_t addr, char *path);

/* Dump memory region from process memory */
void *mem_region_dump(pid_t pid, struct mem_region *region);


#endif
