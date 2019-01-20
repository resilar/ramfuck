#ifndef MEM_H_INCLUDED
#define MEM_H_INCLUDED

#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

struct mem_region;

struct mem_io {
    int (*attach_pid)(struct mem_io *, pid_t);
    int (*detach)(struct mem_io *);
    int (*attached)(struct mem_io *);

    struct mem_region *(*region_first)(struct mem_io *);
    struct mem_region *(*region_next)(struct mem_region *);
	struct mem_region *(*region_find_addr)(struct mem_io *, uintptr_t addr);
    void (*region_put)(struct mem_region *);

    int (*read)(struct mem_io *, uintptr_t addr, void *buf, size_t len);
};

enum mem_prot {
    MEM_EXECUTE = 1,
    MEM_WRITE = 2,
    MEM_READ = 4
};

struct mem_region {
    uintptr_t start;
    size_t size;
    enum mem_prot prot;
    char *path;
};

/* Get mem_io instance */
struct mem_io *mem_io_get();

/* Destroy mem_io instance */
void mem_io_put(struct mem_io *mem);

#endif
