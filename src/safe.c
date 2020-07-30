#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils.h"
#include "safe.h"

void *
smmap
(
	void *addr,
	size_t length,
	int prot,
	int flags,
	int fd,
	off_t offset
)
{
	void *ret = mmap(addr, length, prot, flags, fd, offset);
	if (ret == MAP_FAILED)
		panic("Failed: mmap(%p, %llu, %d, %d, %d, %llu)\n",
			addr, length, prot, flags, fd, offset
		);
	return ret;
}

sighandler_t
ssignal(int signum, sighandler_t handler)
{
	sighandler_t ret = signal(signum, handler);
	if (ret == SIG_ERR)
		panic("Failed: signal(%d, %p)\n", signum, handler);
	return ret;
}

int
sopen(const char *pathname, int flags, ...)
{
	va_list ap;
	int ret;
	mode_t mode = 0;

	if (flags & O_CREAT) {
		va_start(ap, flags);
		mode = va_arg(ap, mode_t);
		va_end(ap);
	}

	ret = open(pathname, flags, mode);
	if (ret < 0)
		panic("Error: open(\"%s\", %d, %o)\n", pathname, flags, mode);

	return ret;
}

int
sclose(int fd)
{
	int ret = close(fd);
	if (ret < 0)
		panic("Error: close(%d)\n", fd);
	return ret;
}

int
sfstat(int fd, struct stat *statbuf)
{
	int ret = fstat(fd, statbuf);
	if (ret < 0)
		panic("Error: fstat(%d, %p)\n", fd, statbuf);
	return ret;
}

ssize_t
swrite(int fd, const void *buf, size_t count)
{
	ssize_t ret = write(fd, buf, count);
	if (ret < 0)
		panic("Error: write(%d, %p, %llu)\n", fd, buf, count);
	return ret;
}

pid_t
sfork(void)
{
	pid_t ret = fork();
	if (ret < 0)
		panic("Error: fork()\n");
	return ret;
}

int
sdup2(int oldfd, int newfd)
{
	int ret = dup2(oldfd, newfd);
	if (ret < 0)
		panic("Error: dup2(%d, %d)\n", oldfd, newfd);
	return ret;
}

pid_t
swaitpid(pid_t pid, int *wstatus, int options)
{
	pid_t ret = waitpid(pid, wstatus, options);
	if (ret < 0)
		panic("Error: waitpid(%d, %p, %d)\n", pid, wstatus, options);
	return ret;
}

void
sexecve(const char *pathname, char *const argv[], char *const envp[])
{
	execve(pathname, argv, envp);
	panic("Error: execve(\"%s\", %p, %p)\n", pathname, argv, envp);
}

off_t
slseek(int fd, off_t offset, int whence)
{
	off_t ret = lseek(fd, offset, whence);
	if (ret == (off_t) -1)
		panic("Error: lseek(%d, %llu, %d)\n", fd, offset, whence);
	return ret;
}

int
srename(const char *oldpath, const char *newpath)
{
	int ret = rename(oldpath, newpath);
	if (ret < 0)
		panic("Error: rename(\"%s\", \"%s\")\n", oldpath, newpath);
	return ret;
}

void *
smalloc(size_t size)
{
	/* Use calloc to zero out the buf */
	void *ret = calloc(1, size);
	if (!ret)
		panic("Error: malloc(%llu)\n", size);
	return ret;
}

int
sftruncate(int fd, off_t length)
{
	int ret = ftruncate(fd, length);
	if (ret < 0)
		panic("Error: ftruncate(%d, %lu)\n", fd, length);
	return ret;
}

ssize_t
sread(int fd, void *buf, size_t count)
{
	ssize_t ret = read(fd, buf, count);
	if (ret < 0)
		panic("Error: read(%d, %p, %lu)\n", fd, buf, count);
	return ret;
}

int
spipe(int pipefd[2])
{
	int ret = pipe(pipefd);
	if (ret < 0)
		panic("Error: pipe[%d, %d]\n", pipefd[0], pipefd[1]);
	return ret;
}

FILE *
sfopen(const char *pathname, const char *mode)
{
	FILE *ret = fopen(pathname, mode);
	if (ret == NULL)
		panic("Error: fopen(\"%s\", \"%s\")\n", pathname, mode);
	return ret;
}
