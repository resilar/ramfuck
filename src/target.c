#define _DEFAULT_SOURCE /* for pread(3) */
#include "target.h"
#include "ramfuck.h"
#include "ptrace.h"

#include <ctype.h>
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

/*
 * /proc/pid/maps format:
 * address           perms offset  dev   inode   pathname
 * 00400000-00580000 r-xp 00000000 fe:01 4858009 /usr/lib/nethack/nethack
 */
static struct region *process_region_iter_next(struct region *it)
{
    if (it) {
        char range[256];
        addr_t start, end;
        if (fscanf(((struct process_region_iter *)it)->fd, "%255[^ ]", range)) {
            char *p = strchr(range, '-');
            if (p && strlen(p+1 + strspn(p+1, "0")) * 4 <= ADDR_BITS) {
                char perms[4];
                *((struct process_region_iter *)it)->pathbuf = '\0';
                if (sscanf(range, "%" SCNaddr "-%" SCNaddr, &start, &end) == 2
                 && fscanf(((struct process_region_iter *)it)->fd,
                           " %c%c%c%c %*[^ ] %*[^ ] %*[^ ]%*[ ]%4095[^\n]",
                           &perms[0], &perms[1], &perms[2], &perms[3],
                           ((struct process_region_iter *)it)->pathbuf) >= 4) {
                    it->start = start;
                    it->size = end - start;
                    it->prot = 0;
                    if (perms[0] == 'r') it->prot |= MEM_READ;
                    if (perms[1] == 'w') it->prot |= MEM_WRITE;
                    if (perms[2] == 'x') it->prot |= MEM_EXECUTE;
                    it->path = ((struct process_region_iter *)it)->pathbuf;
                    it->path += isspace(*it->path);
                    if (!*it->path)
                        it->path = NULL;
                    return it;
                } else {
                    errf("target: invalid /proc/pid/maps format");
                }
            } else if (p) {
                warnf("target: process memory addresses exceed supported "
                      "%u bits", (unsigned int)ADDR_BITS);
            }
        }
        fclose(((struct process_region_iter *)it)->fd);
        free(it);
        it = NULL;
    }
    return NULL;
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

static struct region *process_region_at(struct target *target, addr_t addr)
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

static int process_ptrace_read(struct target_process *process,
                               addr_t addr, void *buf, size_t len)
{

    return (uintptr_t)addr == addr
        && ptrace_read(process->pid, (const void *)(uintptr_t)addr, buf, len);
}

static int process_pread_read(struct target_process *process,
                              addr_t addr, void *buf, size_t len)
{
    if (process->mem_fd != -1) {
        int errnold = errno;
        int errors = 0;
        errno = 0;
        while (len > 0) {
            ssize_t ret = pread(process->mem_fd, buf, len, (off_t)addr);
            if (ret <= 0) {
                if (errno != EINTR || ++errors == 3)
                    break;
            } else {
                buf = (char *)buf + ret;
                len -= ret;
                addr += ret;
            }
        }
        errno = errnold;
    }
    return !len;
}

static int process_read(struct target *target,
                        addr_t addr, void *buf, size_t len)
{
    struct target_process *process = (struct target_process *)target;
    if (len <= sizeof(long)) {
        return process_ptrace_read(process, addr, buf, len)
            || process_pread_read(process, addr, buf, len);
    }
    return process_pread_read(process, addr, buf, len)
        || process_ptrace_read(process, addr, buf, len);
}

static int process_ptrace_write(struct target_process *process,
                                addr_t addr, void *buf, size_t len)
{
    return (uintptr_t)addr == addr
        && ptrace_write(process->pid, (void *)(uintptr_t)addr, buf, len);
}

static int process_pwrite_write(struct target_process *process,
                                addr_t addr, void *buf, size_t len)
{
    if (process->mem_fd != -1) {
        int errnold = errno;
        int errors = 0;
        errno = 0;
        while (len > 0) {
            ssize_t ret = pwrite(process->mem_fd, buf, len, (off_t)addr);
            if (ret <= 0) {
                if ((errno != EAGAIN && errno != EINTR) || ++errors == 3)
                    break;
            } else {
                buf = (char *)buf + ret;
                len -= ret;
                addr += ret;
            }
        }
        errno = errnold;
    }
    return !len;
}

static int process_write(struct target *target, addr_t addr, void *buf,
                         size_t len)
{
    struct target_process *process = (struct target_process *)target;
    if (len <= sizeof(long)) {
        return process_ptrace_write(process, addr, buf, len)
            || process_pwrite_write(process, addr, buf, len);
    }
    return process_pwrite_write(process, addr, buf, len)
        || process_ptrace_write(process, addr, buf, len);
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
            if ((process->mem_fd = open(mem_path, O_RDWR)) == -1) {
                if ((process->mem_fd = open(mem_path, O_RDONLY)) == -1)
                    warnf("target: open(%s) failed", mem_path);
            }
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
        if (!size || !(dest->path = malloc(size))) {
            errf("target: allocating path buffer for copied region failed");
            return 0;
        }
        memcpy(dest->path, source->path, size);
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
    return snprintf(out, size,
                   "0x%08" PRIaddr "-0x%08" PRIaddr " %3u%c %c%c%c %s",
                    mr->start, mr->start + mr->size,
                    (unsigned int)hsize, suffix,
                    (mr->prot & MEM_READ) ? 'r' : '-',
                    (mr->prot & MEM_WRITE) ? 'w' : '-',
                    (mr->prot & MEM_EXECUTE) ? 'e' : '-',
                    mr->path ? mr->path : "");
}

void region_destroy(struct region *region)
{
    free(region->path);
    memset(region, 0, sizeof(struct region));
}
