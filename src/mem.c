#include "mem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <limits.h>

struct memory_region_iter {
    struct memory_region region;
    char pathbuf[4096];
    FILE* fd;
};

struct memory_region *memory_region_iter_first(pid_t pid)
{
    struct memory_region_iter *it = malloc(sizeof(struct memory_region_iter));
    if (it) {
        it->region.path = it->pathbuf;

        if (pid > 0) {
            char filename[64];
            sprintf(filename, "/proc/%ld/maps", (long)pid);
            it->fd = fopen(filename, "r");
        } else {
            it->fd = fopen("/proc/self/maps", "r");
        }

        if (!it->fd) {
            free(it);
            it = NULL;
        }
    }
    return (struct memory_region *)it;
}

struct memory_region *memory_region_iter_next(struct memory_region *it)
{
    if (it) {
        void *start, *end;
        char perms[4];
        if (fscanf(((struct memory_region_iter *)it)->fd,
                   "%p-%p %c%c%c%c %*[^ ] %*[^ ] %*[^ ]%*[ ]%4095[^\n]",
                   &start, &end, &perms[0], &perms[1], &perms[2], &perms[3],
                   it->path) >= 6) {
            it->start = (uintptr_t)start;
            it->size = (size_t)((uintptr_t)end - (uintptr_t)start);
            it->prot = 0;
            if (perms[0] == 'r') it->prot |= MEM_READ;
            if (perms[1] == 'w') it->prot |= MEM_WRITE;
            if (perms[2] == 'x') it->prot |= MEM_EXEC;
        } else {
            fclose(((struct memory_region_iter *)it)->fd);
            free(it);
            it = NULL;
        }
    }
    return it;
}

struct memory_region *memory_region_find(pid_t pid, uintptr_t addr, char *path)
{
    struct memory_region *it = memory_region_iter_first(pid);
    while ((it = memory_region_iter_next(it))) {
        void *tmp;
        if (path && strcmp(it->path, path) != 0)
            continue;
        if (addr && !(it->start <= addr && addr < it->start + it->size))
            continue;
        tmp = realloc(it, sizeof(struct memory_region)
                          + (*it->path ? strlen(it->path)+1 : 0));
        if (tmp) it = tmp;
        fclose(((struct memory_region_iter *)it)->fd);
        break;
    }
    return (struct memory_region *)it;
}

void *memory_region_dump(pid_t pid, struct memory_region *region)
{
    char mem_file[128];
    int fd;
    unsigned char *buf;
    size_t count, ret;
    buf = 0;
    sprintf(mem_file, "/proc/%lu/mem", (unsigned long)pid);

    if (!(fd = open(mem_file, O_RDONLY))) {
        free(buf);
        fprintf(stderr, "mem: cannot open '%s' for reading\n", mem_file);
        return NULL;
    }

    if (!(buf = malloc(region->size))) {
        fprintf(stderr, "mem: memory allocation of %lu bytes failed\n",
                (unsigned long)region->size);
        return NULL;
    }

    if (lseek(fd, region->start, SEEK_SET) == -1) {
        fprintf(stderr, "mem: seeking to a memory region failed");
        goto fail;
    }

    count = 0;
    do {
        ret = read(fd, &buf[count], region->size - count);
        if (ret == 0) {
            fprintf(stderr, "mem: unexpected end of memory region\n");
            goto fail;
        } else if (ret == -1) {
            /*
             * This is a common error when trying to read mapped (?) regions.
             *
             perror("mem: memory read failed");
             */
            goto fail;
        }
        count += ret;
    } while (count < region->size);

    /* Finish */
    close(fd);
    return buf;

fail:
    if (buf) free(buf);
    if (fd != -1) close(fd);
    return NULL;
}
