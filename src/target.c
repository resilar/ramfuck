#define _DEFAULT_SOURCE /* for pread(3) */
#include "target.h"
#include "ramfuck.h"
#include "ptrace.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>

struct target_process {
    struct target target;
    pid_t pid;
    int mem_fd;
};

static int process_detach(struct target *target)
{
    struct target_process *process = (struct target_process *)target;
    int rc = 1;
    if (process->pid) {
        process->pid = 0;
    } else rc = 0;
    if (process->mem_fd != -1) {
        close(process->mem_fd);
        process->mem_fd = -1;
    } else rc = 0;
    free(process);
    return rc;
}

int process_stop(struct target *target)
{
    struct target_process *process = (struct target_process *)target;
    return ptrace_attach(process->pid);
}

int process_run(struct target *target)
{
    struct target_process *process = (struct target_process *)target;
    return ptrace_detach(process->pid);
}

struct process_region_iter {
    struct region region;
    char pathbuf[4096];
    FILE* fd;
};

static struct region *process_region_iter_next(struct region *it)
{
    if (it) {
        int n;
        void *start, *end;
        char perms[4];
        it->path[0] = '\0';
        n = fscanf(((struct process_region_iter *)it)->fd,
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
            if (n > 0) errf("target: invalid /proc/pid/maps format");
            fclose(((struct process_region_iter *)it)->fd);
            free(it);
            it = NULL;
        }
    }
    return it;
}

static struct region *process_region_iter_first(struct target *target)
{
    struct process_region_iter *it;
    struct target_process *process = (struct target_process *)target;
    if ((it = malloc(sizeof(struct process_region_iter)))) {
        char filename[128];
        it->region.path = it->pathbuf;

        if (process->pid > 0) {
            sprintf(filename, "/proc/%lu/maps", (unsigned long)process->pid);
        } else {
            memcpy(filename, "/proc/self/maps", sizeof("/proc/self/maps"));
        }

        if (!(it->fd = fopen(filename, "r"))) {
            errf("target: error opening %s", filename);
            free(it);
            it = NULL;
        }
    }
    return it ? process_region_iter_next((struct region *)it) : NULL;
}

static struct region *process_region_at(struct target *target, uintptr_t addr)
{
    struct region *it = process_region_iter_first(target);
    while ((it = process_region_iter_next(it))) {
        if (it->start <= addr && addr < it->start + it->size) {
            fclose(((struct process_region_iter *)it)->fd);
            it = realloc(it, sizeof(struct region)
                             + (*it->path ? strlen(it->path)+1 : 0));
            break;
        }
    }
    return (struct region *)it;
}

static void process_region_put(struct region *region)
{
    free(region->path);
    free(region);
}

static int process_read(struct target *target,
                        uintptr_t addr, void *buf, size_t len)
{
    struct target_process *process = (struct target_process *)target;
    int errors = 0;

    if (process->mem_fd != -1 && len > sizeof(uint64_t)) {
        do {
            ssize_t ret = pread(process->mem_fd, buf, len, (off_t)addr);
            if (ret == -1) {
                if (errno != EINTR || ++errors == 3)
                    break;
            } else {
                buf = (char *)buf + ret;
                len -= ret;
                addr += ret;
            }
        } while (len > 0);
    }

    return !len || ptrace_read(process->pid, (const void *)addr, buf, len);
}

static int process_write(struct target *target, uintptr_t addr, void *buf,
                         size_t len)
{
    struct target_process *process = (struct target_process *)target;
    return ptrace_write(process->pid, (void *)addr, buf, len);
}

struct target *target_attach_pid(pid_t pid)
{
    static const struct target process_init = {
        process_detach,
        process_stop,
        process_run,
        process_region_iter_first,
        process_region_iter_next,
        process_region_at,
        process_region_put,
        process_read,
        process_write
    };

    struct target_process *process;
    if ((process = malloc(sizeof(*process)))) {
        if (ptrace_attach(pid) && ptrace_detach(pid)) {
            char mem_path[128];
            memcpy(&process->target, &process_init, sizeof(struct target));
            process->pid = pid;
            sprintf(mem_path, "/proc/%lu/mem", (unsigned long)pid);
            if ((process->mem_fd = open(mem_path, O_RDONLY)) == -1)
                warnf("target: open(%s) failed", mem_path);
        } else {
            free(process);
            process = NULL;
        }
    } else {
        errf("target: out-of-memory for target instance");
    }

    return (struct target *)process;
}

void target_detach(struct target *target)
{
    target->detach(target);
}

int region_copy(struct region *dest, const struct region *source)
{
    memcpy(dest, source, sizeof(struct region));
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

size_t region_snprint(const struct region *mr, char *out, size_t size)
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

void region_destroy(struct region *region)
{
    free(region->path);
    memset(region, 0, sizeof(struct region));
}
