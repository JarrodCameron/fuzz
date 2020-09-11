#ifndef _SAFE_H_
#define _SAFE_H_

/* Here is a wrapper around pretty much every function call. If the function
 * fails then we are fucked so we might as well abort. */

#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "utils.h"

/* Forward declerations */
struct stat;
typedef void (*sighandler_t)(int);

/* mmap() wrapper */
void *smmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);

/* signal() wrapper */
sighandler_t ssignal(int signum, sighandler_t handler);

/* open() wrapper */
int sopen(const char *pathname, int flags, ...);

/* close() wrapper */
int sclose(int fd);

/* fstat() wrapper */
int sfstat(int fd, struct stat *statbuf);

/* write() wrapper */
ssize_t swrite(int fd, const void *buf, size_t count);

/* fork() wrapper */
pid_t sfork(void);

/* dup2() wrapper */
int sdup2(int oldfd, int newfd);

/* waitpid() wrapper */
pid_t swaitpid(pid_t pid, int *wstatus, int options);

/* execve() wrapper */
NORETURN void sexecve(const char *pathname, char *const argv[], char *const envp[]);

/* lseek() wrapper */
off_t slseek(int fd, off_t offset, int whence);

/* malloc() wrapper */
void *smalloc(size_t size);

/* ftruncate() wrapper */
int sftruncate(int fd, off_t length);

/* read() wrapper */
ssize_t sread(int fd, void *buf, size_t count);

/* pipe() wrapper */
int spipe(int pipefd[2]);

/* fopen() wrapper */
FILE *sfopen(const char *pathname, const char *mode);

/* unlink() wrapper */
int sunlink(const char *pathname);

/* strdup() wrapper */
char *sstrdup(const char *s);

/* mkstemp() wrapper */
int smkstemp(char *template);

/* time() wrapper */
time_t stime(time_t *tloc);

#endif /* _SAFE_H_ */
