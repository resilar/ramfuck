#define _DEFAULT_SOURCE /* for pread(3) */
#include "mem.h"
#include "ramfuck.h"
#include "ptrace.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>

struct mem_process {
    struct mem_io io;
    pid_t pid;
    int mem_fd;
};

static int mem_attach_pid(struct mem_io *io, pid_t pid)
{
    struct mem_process *mem = (struct mem_process *)io;
    if (ptrace_attach(pid)) {
        char mem_path[128];
        mem->pid = pid;
        sprintf(mem_path, "/proc/%lu/mem", (unsigned long)pid);
        if ((mem->mem_fd = open(mem_path, O_RDONLY)) == -1)
            warnf("mem: open(%s) failed", mem_path);
        return 1;
    }
    return 0;
}

static int mem_detach(struct mem_io *io)
{
    struct mem_process *mem = (struct mem_process *)io;
    int rc = 1;
    if (mem->pid && ptrace_detach(mem->pid)) {
        mem->pid = 0;
    } else rc = 0;
    if (mem->mem_fd != -1) {
        close(mem->mem_fd);
        mem->mem_fd = -1;
    } else rc = 0;
    return rc;
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

static struct mem_region *mem_region_iter_first(struct mem_io *io)
{
    struct mem_region_iter *it;
    struct mem_process *mem = (struct mem_process *)io;
    if ((it = malloc(sizeof(struct mem_region_iter)))) {
        char filename[128];
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

static struct mem_region *mem_region_at(struct mem_io *mem, uintptr_t addr)
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

static void mem_region_put(struct mem_region *region)
{
    free(region->path);
    free(region);
}

static int mem_read(struct mem_io *io, uintptr_t addr, void *buf, size_t len)
{
    struct mem_process *mem = (struct mem_process *)io;
    int errors = 0;

    if (mem->mem_fd != -1 && len > sizeof(uint64_t)) {
        do {
            ssize_t ret = pread(mem->mem_fd, buf, len, (off_t)addr);
            if (ret == -1) {
                if (errno != EINTR || ++errors == 3)
                    return 0;
            } else {
                buf = (char *)buf + ret;
                len -= ret;
                addr += ret;
            }
        } while (len > 0);
    }

    return !len || !ptrace_read(mem->pid, (const void *)addr, buf, len);
}

static int mem_write(struct mem_io *io, uintptr_t addr, void *buf, size_t len)
{
    struct mem_process *mem = (struct mem_process *)io;
    return !ptrace_write(mem->pid, (void *)addr, buf, len);
}

struct mem_io *mem_io_get()
{
    static struct mem_io mem_io_init = {
        mem_attach_pid,
        mem_detach,
        mem_attached,
        mem_region_iter_first,
        mem_region_iter_next,
        mem_region_at,
        mem_region_put,
        mem_read,
        mem_write
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

int mem_region_copy(struct mem_region *dest, const struct mem_region *source)
{
    memcpy(dest, source, sizeof(struct mem_region));
    if (source->path) {
        size_t size = strlen(source->path) + 1;
        if ((dest->path = malloc(size))) {
            memcpy(dest->path, source->path, size);
        } else {
            /* return 0; */ /* ? */
        }
    }
    return 1;
}

size_t mem_region_snprint(const struct mem_region *mr, char *out, size_t size)
{
    char suffix;
    size_t hsize = mr->size;

    /*
     * Get human-readable size.
     *
     * We cheat a little and round up values 1000-1023 to the next unit
     * so that the human-readable representation is at most 3 digits.
     */
    if (hsize < 1000) { /* round up a little to get 3 digits */
        suffix = 'B';
    } else if ((hsize /= 1024) < 1000) {
        suffix = 'K';
    } else if ((hsize /= 1024) < 1000) {
        suffix = 'M';
    } else if ((hsize /= 1024) < 1000) {
        suffix = 'G';
    } else if ((hsize /= 1024) < 1000) {
        suffix = 'T';
    } else if ((hsize /= 1024) < 1000) {
        suffix = 'P';
    } else if ((hsize /= 1024) < 1000) {
        suffix = 'E';
    } else if ((hsize /= 1024) < 1000) {
        suffix = 'Z';
    } else {
        suffix = '?';
    }
    return snprintf(out, size, "%p-%p %3d%c %c%c%c %s",
                    (void *)mr->start, (void *)(mr->start + mr->size),
                    hsize ? (int)hsize : 1, suffix,
                    (mr->prot & MEM_READ) ? 'r' : '-',
                    (mr->prot & MEM_WRITE) ? 'w' : '-',
                    (mr->prot & MEM_EXECUTE) ? 'e' : '-',
                    mr->path);
}

void mem_region_destroy(struct mem_region *region)
{
    free(region->path);
    memset(region, 0, sizeof(struct mem_region));
}
