#ifndef MEM_H_INCLUDED
#define MEM_H_INCLUDED

#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

struct mem_region;

struct mem_io {
    /* Attach/detach target */
    int (*attach_pid)(struct mem_io *, pid_t);
    int (*detach)(struct mem_io *);
    int (*attached)(struct mem_io *);

    /* Target break and continue */
    int (*target_break)(struct mem_io *);
    int (*target_continue)(struct mem_io *);

    /* Iterate memory regions */
    struct mem_region *(*region_first)(struct mem_io *);
    struct mem_region *(*region_next)(struct mem_region *);
	struct mem_region *(*region_at)(struct mem_io *, uintptr_t addr);

    /* For freeing regions returned by the functions above */
    void (*region_put)(struct mem_region *);

    /* Read/write target memory */
    int (*read)(struct mem_io *, uintptr_t addr, void *buf, size_t len);
    int (*write)(struct mem_io *, uintptr_t addr, void *buf, size_t len);
};

/* Get mem_io instance */
struct mem_io *mem_io_get();

/* Destroy mem_io instance */
void mem_io_put(struct mem_io *mem);

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

int mem_region_copy(struct mem_region *dest, const struct mem_region *source);

size_t mem_region_snprint(const struct mem_region *mr, char *out, size_t size);

void mem_region_destroy(struct mem_region *region);

#endif
