#include "mem.h"
#include "ramfuck.h"
#include "ptrace.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <limits.h>

struct mem_process {
    struct mem_io io;
    pid_t pid;
};

static int mem_attach_pid(struct mem_io *io, pid_t pid)
{
    struct mem_process *mem = (struct mem_process *)io;
    if (ptrace_attach(pid)) {
        mem->pid = pid;
        return 1;
    }
    return 0;
}

static int mem_detach(struct mem_io *io)
{
    struct mem_process *mem = (struct mem_process *)io;
    if (mem->pid && ptrace_detach(mem->pid)) {
        mem->pid = 0;
        return 1;
    }
    return 0;
}

static int mem_attached(struct mem_io *io)
{
    struct mem_process *mem = (struct mem_process *)io;
    return mem->pid != 0;
}

struct mem_region_iter {
    struct mem_region region;
    char pathbuf[4096];
    FILE* fd;
};

static struct mem_region *mem_region_iter_next(struct mem_region *it);

static struct mem_region *mem_region_iter_first(struct mem_io *io)
{
    struct mem_region_iter *it;
    struct mem_process *mem = (struct mem_process *)io;
    if ((it = malloc(sizeof(struct mem_region_iter)))) {
        char filename[64];
        it->region.path = it->pathbuf;

        if (mem->pid > 0) {
            sprintf(filename, "/proc/%lu/maps", (unsigned long)mem->pid);
        } else {
            memcpy(filename, "/proc/self/maps", sizeof("/proc/self/maps"));
        }

        if (!(it->fd = fopen(filename, "r"))) {
            errf("mem: error opening %s", filename);
            free(it);
            it = NULL;
        }
    }
    return it ? mem_region_iter_next((struct mem_region *)it) : NULL;
}

static struct mem_region *mem_region_iter_next(struct mem_region *it)
{
    if (it) {
        int n;
        void *start, *end;
        char perms[4];
        it->path[0] = '\0';
        n = fscanf(((struct mem_region_iter *)it)->fd,
                   "%p-%p %c%c%c%c %*[^ ] %*[^ ] %*[^ ]%*[ ]%4095[^\n]",
                   &start, &end, &perms[0], &perms[1], &perms[2], &perms[3],
                   it->path);
        if (n >= 6) {
            it->start = (uintptr_t)start;
            it->size = (size_t)((uintptr_t)end - (uintptr_t)start);
            it->prot = 0;
            if (perms[0] == 'r') it->prot |= MEM_READ;
            if (perms[1] == 'w') it->prot |= MEM_WRITE;
            if (perms[2] == 'x') it->prot |= MEM_EXECUTE;
        } else {
            if (n > 0) errf("mem: invalid /proc/pid/maps format");
            fclose(((struct mem_region_iter *)it)->fd);
            free(it);
            it = NULL;
        }
    }
    return it;
}

static struct mem_region *mem_region_find_addr(struct mem_io *mem,
                                               uintptr_t addr)
{
    struct mem_region *it = mem_region_iter_first(mem);
    while ((it = mem_region_iter_next(it))) {
        if (it->start <= addr && addr < it->start + it->size) {
            fclose(((struct mem_region_iter *)it)->fd);
            it = realloc(it, sizeof(struct mem_region)
                             + (*it->path ? strlen(it->path)+1 : 0));
            break;
        }
    }
    return (struct mem_region *)it;
}

static void *mem_region_dump(struct mem_io *io, struct mem_region *region)
{
    int fd;
    size_t count, ret;
    unsigned char *buf;
    char procmem_path[128];
    struct mem_process *mem = (struct mem_process *)io;

    sprintf(procmem_path, "/proc/%lu/mem", (unsigned long)mem->pid);
    if (!(fd = open(procmem_path, O_RDONLY))) {
        errf("mem: cannot open '%s' for reading", procmem_path);
        return NULL;
    }

    if (!(buf = malloc(region->size))) {
        errf("mem: %lu-byte allocation failed", (unsigned long)region->size);
        goto fail;
    }

    if (lseek(fd, region->start, SEEK_SET) == -1) {
        errf("mem: seeking to a memory region failed");
        goto fail;
    }

    for (count = 0; count < region->size; count += ret) {
        ret = read(fd, &buf[count], region->size - count);
        if (ret == 0) {
            errf("mem: unexpected end of memory region");
            goto fail;
        } else if (ret == -1) {
            /*
             * This is a common error when trying to read mapped (?) regions.
             *
             perror("mem: memory read failed");
             */
            goto fail;
        }
    }

    /* Finish */
    close(fd);
    return buf;

fail:
    free(buf);
    close(fd);
    return NULL;
}

static void mem_region_put(struct mem_region *region)
{
    free(region);
}

struct mem_io *mem_io_get()
{
    static struct mem_io mem_io_init = {
        mem_attach_pid,
        mem_detach,
        mem_attached,
        mem_region_iter_first,
        mem_region_iter_next,
        mem_region_find_addr,
        mem_region_dump,
        mem_region_put,
    };

    struct mem_process *mem;
    if ((mem = malloc(sizeof(*mem)))) {
        memcpy(&mem->io, &mem_io_init, sizeof(struct mem_io));
        mem->pid = 0;
    } else {
        errf("mem: out-of-memory for mem_io");
    }

    return (struct mem_io *)mem;
}

void mem_io_put(struct mem_io *mem)
{
    free(mem);
}
