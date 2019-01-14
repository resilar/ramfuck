#ifndef MEM_H_INCLUDED
#define MEM_H_INCLUDED

#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

enum memory_protection {
    MEM_EXEC = 1,
    MEM_WRITE = 2,
    MEM_READ = 4
};

struct memory_region {
    uintptr_t start;
    size_t size;
    enum memory_protection prot;
    char *path;
};

/* Iteration of PID memory regions */
struct memory_region *memory_region_iter_first(pid_t pid);
struct memory_region *memory_region_iter_next(struct memory_region *it);

/* Find memory region based on address or pathname */
struct memory_region *memory_region_find(pid_t pid, uintptr_t addr, char *path);

/* Dump memory region from process memory */
void *memory_region_dump(pid_t pid, struct memory_region *region);


#endif
