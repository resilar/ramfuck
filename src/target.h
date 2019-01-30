#ifndef TARGET_H_INCLUDED
#define TARGET_H_INCLUDED

#include "defines.h"
#include <sys/types.h>

struct region;

struct target {
    /* Detach target */
    int (*detach)(struct target *);

    /* Target break and continue */
    int (*stop)(struct target *);
    int (*run)(struct target *);

    /* Iterate memory regions (iterate till the end to prevent memory leaks!) */
    struct region *(*region_first)(struct target *);
    struct region *(*region_next)(struct region *);

    /* Read/write target memory */
    int (*read)(struct target *, addr_t addr, void *buf, size_t len);
    int (*write)(struct target *, addr_t addr, void *buf, size_t len);
};


/* Create target instance for URI */
struct target *target_attach(const char *uri);

/* Destroy target instance */
void target_detach(struct target *target);

/* Memory region */
struct region {
    addr_t start;
    addr_t size;
    enum mem_protection {
        MEM_EXECUTE = 1,
        MEM_WRITE = 2,
        MEM_READ = 4
    } prot;
    char *path;
};

/* Represent region as a line of string */
size_t region_snprint(const struct region *mr, char *out, size_t size);

/* Copying and freeing regions iterated via target->region_{first,next}() */
int region_copy(struct region *dest, const struct region *source);
void region_destroy(struct region *region);

#endif
