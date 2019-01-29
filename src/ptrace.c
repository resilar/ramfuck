#define _DEFAULT_SOURCE /* kill(3) */
#include "ptrace.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>

int ptrace_attach(pid_t pid)
{
    int status;

    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1) {
        perror("ptrace(ATTACH)");
        return 0;
    }
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid(ATTACH)");
        return 0;
    }

    return !!WIFSTOPPED(status);
}

int ptrace_detach(pid_t pid)
{
    if (ptrace(PTRACE_DETACH, pid, NULL, NULL) == -1) {
        perror("ptrace(DETACH)");
        return 0;
    }
    return 1;
}

int ptrace_break(pid_t pid)
{
    int status;
    if (kill(pid, SIGSTOP) == -1) {
        perror("ptrace(BREAK)");
        return 0;
    }
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid(BREAK))");
        return 0;
    }
    return !!WIFSTOPPED(status);
}

int ptrace_continue(pid_t pid)
{
    if (ptrace(PTRACE_CONT, pid, NULL, NULL) == -1) {
        perror("ptrace(CONT)");
        return 0;
    }
    return 1;
}

int ptrace_read(pid_t pid, const void *addr, void *buf, size_t len)
{
    int errnold = errno;
    errno = 0;
    while (!errno && len > 0) {
        size_t i, j;
        if ((i = ((size_t)addr % sizeof(long))) || len < sizeof(long)) {
            union {
                long value;
                char buf[sizeof(long)];
            } data;
            data.value = ptrace(PTRACE_PEEKDATA, pid, (char *)addr - i, NULL);
            if (!errno) {
                for (j = i; j < sizeof(long) && j-i < len; j++)
                    ((char *)buf)[j-i] = data.buf[j];
                addr = (char *)addr + (j-i);
                buf = (char *)buf + (j-i);
                len -= j-i;
            }
        } else {
            for (i = 0, j = len/sizeof(long); i < j; i++) {
                *(long *)buf = ptrace(PTRACE_PEEKDATA, pid, addr, NULL);
                if (errno) break;
                addr = (char *)addr + sizeof(long);
                buf = (char *)buf + sizeof(long);
                len -= sizeof(long);
            }
        }
    }
    if (!errno)
        errno = errnold;
    return !len;
}

size_t ptrace_write(pid_t pid, void *addr, void *buf, size_t len)
{
    size_t n = len;
    int errnold = errno;
    errno = 0;
    while (!errno && len > 0) {
        size_t i, j;
        if ((i = ((size_t)addr % sizeof(long))) || len < sizeof(long)) {
            union {
                long value;
                char buf[sizeof(long)];
            } data;
            data.value = ptrace(PTRACE_PEEKDATA, pid, (char *)addr-i, 0);
            if (!errno) {
                for (j = i; j < sizeof(long) && j-i < len; j++)
                    data.buf[j] = ((char *)buf)[j-i];
                if (!ptrace(PTRACE_POKEDATA, pid, (char *)addr-i, data.value)) {
                    addr = (char *)addr + (j-i);
                    buf = (char *)buf + (j-i);
                    len -= j-i;
                }
            }
        } else {
            for (i = 0, j = len/sizeof(long); i < j; i++) {
                if (ptrace(PTRACE_POKEDATA, pid, addr, *(long *)buf) == -1)
                    break;
                addr = (char *)addr + sizeof(long);
                buf = (char *)buf + sizeof(long);
                len -= sizeof(long);
            }
        }
    }
    if (!errno)
        errno = errnold;
    return n - len;
}
