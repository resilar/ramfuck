#ifndef TARGET_H_INCLUDED
#define TARGET_H_INCLUDED

#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

struct region;

struct target {
    /* Detach target */
    int (*detach)(struct target *);

    /* Target break and continue */
    int (*stop)(struct target *);
    int (*run)(struct target *);

    /* Iterate memory regions */
    struct region *(*region_first)(struct target *);
    struct region *(*region_next)(struct region *);
	struct region *(*region_at)(struct target *, uintptr_t addr);

    /* For freeing regions returned by the functions above */
    void (*region_put)(struct region *);

    /* Read/write target memory */
    int (*read)(struct target *, uintptr_t addr, void *buf, size_t len);
    int (*write)(struct target *, uintptr_t addr, void *buf, size_t len);
};

/* Get target instance */
struct target *target_attach_pid(pid_t pid);

/* Destroy target instance */
void target_detach(struct target *mem);

enum mem_protection {
    MEM_EXECUTE = 1,
    MEM_WRITE = 2,
    MEM_READ = 4
};

struct region {
    uintptr_t start;
    size_t size;
    enum mem_protection prot;
    char *path;
};

int region_copy(struct region *dest, const struct region *source);

size_t region_snprint(const struct region *mr, char *out, size_t size);

void region_destroy(struct region *region);

#endif
