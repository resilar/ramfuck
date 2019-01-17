#include "mem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <limits.h>

struct mem_region_iter {
    struct mem_region region;
    char pathbuf[4096];
    FILE* fd;
};

struct mem_region *mem_region_iter_first(struct mem_io *mem)
{
    struct mem_region_iter *it = malloc(sizeof(struct mem_region_iter));
    if (it) {
        it->region.path = it->pathbuf;

        if (mem->ctx->pid > 0) {
            char filename[64];
            sprintf(filename, "/proc/%lu/maps", (unsigned long)mem->ctx->pid);
            it->fd = fopen(filename, "r");
        } else {
            it->fd = fopen("/proc/self/maps", "r");
        }

        if (!it->fd) {
            free(it);
            it = NULL;
        }
    }
    return (struct mem_region *)it;
}

struct mem_region *mem_region_iter_next(struct mem_region *it)
{
    if (it) {
        void *start, *end;
        char perms[4];
        if (fscanf(((struct mem_region_iter *)it)->fd,
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
            fclose(((struct mem_region_iter *)it)->fd);
            free(it);
            it = NULL;
        }
    }
    return it;
}

struct mem_region *mem_region_find(struct mem_io *mem, uintptr_t addr,
                                   char *path)
{
    struct mem_region *it = mem_region_iter_first(mem);
    while ((it = mem_region_iter_next(it))) {
        if (path && strcmp(it->path, path) != 0)
            continue;
        if (addr && !(it->start <= addr && addr < it->start + it->size))
            continue;
        fclose(((struct mem_region_iter *)it)->fd);
        it = realloc(it, sizeof(struct mem_region)
                         + (*it->path ? strlen(it->path)+1 : 0));
        break;
    }
    return (struct mem_region *)it;
}

void *mem_region_dump(struct mem_io *mem, struct mem_region *region)
{
    char mem_file[128];
    int fd;
    unsigned char *buf;
    size_t count, ret;
    buf = 0;
    sprintf(mem_file, "/proc/%lu/mem", (unsigned long)mem->ctx->pid);

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
