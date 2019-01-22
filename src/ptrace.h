#ifndef PTRACE_H_INCLUDED_
#define PTRACE_H_INCLUDED_

#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * Attach/detach process specified by pid.
 */
int ptrace_attach(pid_t pid);
int ptrace_detach(pid_t pid);

/*
 * Break/continue process.
 */
int ptrace_break(pid_t pid);
int ptrace_continue(pid_t pid);

/*
 * Read & write data.
 */
int ptrace_read(pid_t pid, const void *addr, void *buf, size_t len);
size_t ptrace_write(pid_t pid, void *addr, void *buf, size_t len);

#endif
