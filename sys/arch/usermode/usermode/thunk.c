/* $NetBSD: thunk.c,v 1.53 2011/12/26 14:50:27 jmcneill Exp $ */

/*-
 * Copyright (c) 2011 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifdef __NetBSD__
__RCSID("$NetBSD: thunk.c,v 1.53 2011/12/26 14:50:27 jmcneill Exp $");
#endif

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/reboot.h>
#include <sys/poll.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <machine/vmparam.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_tap.h>

#include <aio.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <sched.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <ucontext.h>
#include <unistd.h>

#include "../include/thunk.h"

#ifndef __arraycount
#define __arraycount(x)	(sizeof((x)) / sizeof((x)[0]))
#endif

#ifndef MAP_ANON
#define MAP_ANON MAP_ANONYMOUS
#endif

extern int boothowto;

void
dprintf_debug(const char *fmt, ...)
{
        if (boothowto & AB_DEBUG) {
                va_list ap;

                va_start(ap, fmt);
                vfprintf(stderr, fmt, ap);
                va_end(ap);
        }
}

static void
thunk_to_timeval(const struct thunk_timeval *ttv, struct timeval *tv)
{
	tv->tv_sec = ttv->tv_sec;
	tv->tv_usec = ttv->tv_usec;
}

static void
thunk_from_timeval(const struct timeval *tv, struct thunk_timeval *ttv)
{
	ttv->tv_sec = tv->tv_sec;
	ttv->tv_usec = tv->tv_usec;
}

static void
thunk_to_itimerval(const struct thunk_itimerval *tit, struct itimerval *it)
{
	thunk_to_timeval(&tit->it_interval, &it->it_interval);
	thunk_to_timeval(&tit->it_value, &it->it_value);
}

static void
thunk_from_itimerval(const struct itimerval *it, struct thunk_itimerval *tit)
{
	thunk_from_timeval(&it->it_interval, &tit->it_interval);
	thunk_from_timeval(&it->it_value, &tit->it_value);
}

static void
thunk_to_termios(const struct thunk_termios *tt, struct termios *t)
{
	int i;

	t->c_iflag = tt->c_iflag;
	t->c_oflag = tt->c_oflag;
	t->c_cflag = tt->c_cflag;
	t->c_lflag = tt->c_lflag;
	for (i = 0; i < __arraycount(t->c_cc); i++)
		t->c_cc[i] = tt->c_cc[i];
	t->c_ispeed = tt->c_ispeed;
	t->c_ospeed= tt->c_ospeed;
}

static void
thunk_from_termios(const struct termios *t, struct thunk_termios *tt)
{
	int i;

	tt->c_iflag = t->c_iflag;
	tt->c_oflag = t->c_oflag;
	tt->c_cflag = t->c_cflag;
	tt->c_lflag = t->c_lflag;
	for (i = 0; i < __arraycount(tt->c_cc); i++)
		tt->c_cc[i] = t->c_cc[i];
	tt->c_ispeed = t->c_ispeed;
	tt->c_ospeed= t->c_ospeed;
}

static int
thunk_to_native_prot(int prot)
{
	int nprot = PROT_NONE;

	if (prot & THUNK_PROT_READ)
		nprot |= PROT_READ;
	if (prot & THUNK_PROT_WRITE)
		nprot |= PROT_WRITE;
	if (prot & THUNK_PROT_EXEC)
		nprot |= PROT_EXEC;

	return nprot;
}

static int
thunk_to_native_mapflags(int flags)
{
	int nflags = 0;

	if (flags & THUNK_MAP_ANON)
		nflags |= MAP_ANON;
	if (flags & THUNK_MAP_FIXED)
		nflags |= MAP_FIXED;
	if (flags & THUNK_MAP_FILE)
		nflags |= MAP_FILE;
	if (flags & THUNK_MAP_SHARED)
		nflags |= MAP_SHARED;
	if (flags & THUNK_MAP_PRIVATE)
		nflags |= MAP_PRIVATE;
#ifndef MAP_NOSYSCALLS
#define MAP_NOSYSCALLS (1<<16)		/* XXX alias for now XXX */
#endif
#ifdef MAP_NOSYSCALLS
	if (flags & THUNK_MAP_NOSYSCALLS)
		nflags |= MAP_NOSYSCALLS;
#endif

	return nflags;
}

int
thunk_setitimer(int which, const struct thunk_itimerval *value,
    struct thunk_itimerval *ovalue)
{
	struct itimerval it, oit;
	int error;

	thunk_to_itimerval(value, &it);
	error = setitimer(which, &it, &oit);
	if (error)
		return error;
	if (ovalue)
		thunk_from_itimerval(&oit, ovalue);

	return 0;
}

int
thunk_gettimeofday(struct thunk_timeval *tp, void *tzp)
{
	struct timeval tv;
	int error;

	error = gettimeofday(&tv, tzp);
	if (error)
		return error;

	thunk_from_timeval(&tv, tp);

	return 0;
}

unsigned int
thunk_getcounter(void)
{
	struct timespec ts;
	int error;

	error = clock_gettime(CLOCK_MONOTONIC, &ts);
	if (error) {
		perror("clock_gettime CLOCK_MONOTONIC");
		abort();
	}

	return (unsigned int)(ts.tv_nsec % 1000000000ULL);
}

long
thunk_clock_getres_monotonic(void)
{
	struct timespec res;
	int error;

	error = clock_getres(CLOCK_MONOTONIC, &res);
	if (error)
		return -1;

	return (long)(res.tv_sec * 1000000000ULL + res.tv_nsec);
}

timer_t
thunk_timer_attach(void)
{
	timer_t timerid;
	int error;

	error = timer_create(CLOCK_MONOTONIC, NULL, &timerid);
	if (error) {
		perror("timer_create CLOCK_MONOTONIC");
		abort();
	}

	return timerid;
}

int
thunk_timer_start(timer_t timerid, int freq)
{
	struct itimerspec tim;

	tim.it_interval.tv_sec = 0;
	tim.it_interval.tv_nsec = 1000000000 / freq;
	tim.it_value = tim.it_interval;

	return timer_settime(timerid, TIMER_RELTIME, &tim, NULL);
}

int
thunk_timer_getoverrun(timer_t timerid)
{
	return timer_getoverrun(timerid);
}

int
thunk_usleep(useconds_t microseconds)
{
	return usleep(microseconds);
}

void
thunk_exit(int status)
{
	return exit(status);
}

void
thunk_abort(void)
{
	abort();
}

int
thunk_geterrno(void)
{
	return errno;
}

void
thunk_seterrno(int nerrno)
{
	errno = nerrno;
}

int
thunk_getcontext(ucontext_t *ucp)
{
	return getcontext(ucp);
}

int
thunk_setcontext(const ucontext_t *ucp)
{
	return setcontext(ucp);
}

void
thunk_makecontext(ucontext_t *ucp, void (*func)(void), 
    int nargs, void *arg1, void *arg2, void *arg3)
{
	switch (nargs) {
	case 0:
		makecontext(ucp, func, 0);
		break;
	case 1:
		makecontext(ucp, func, 1, arg1);
		break;
	case 2:
		makecontext(ucp, func, 2, arg1, arg2);
		break;
	case 3:
		makecontext(ucp, func, 3, arg1, arg2, arg3);
		break;
	default:
		printf("%s: nargs (%d) too big\n", __func__, nargs);
		abort();
	}
}

int
thunk_swapcontext(ucontext_t *oucp, ucontext_t *ucp)
{
	return swapcontext(oucp, ucp);
}

int
thunk_tcgetattr(int fd, struct thunk_termios *tt)
{
	struct termios t;
	int error;

	error = tcgetattr(fd, &t);
	if (error)
		return error;
	thunk_from_termios(&t, tt);
	return 0;
}

int
thunk_tcsetattr(int fd, int action, const struct thunk_termios *tt)
{
	struct termios t;

	thunk_to_termios(tt, &t);
	return tcsetattr(fd, action, &t);
}

int
thunk_set_stdin_sigio(int onoff)
{
	int flags;

	flags = fcntl(STDIN_FILENO, F_GETFL, 0);

	if (onoff)
		flags |= O_ASYNC;
	else
		flags &= ~O_ASYNC;

	return fcntl(STDIN_FILENO, F_SETFL, flags);
}

int
thunk_pollchar(void)
{
	struct pollfd fds[1];
	uint8_t c;

	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;
	fds[0].revents = 0;

	if (poll(fds, __arraycount(fds), 0) > 0) {
		if (fds[0].revents & POLLIN) {
			if (read(STDIN_FILENO, &c, 1) != 1)
				return EOF;
			return c;
		}
	}

	return EOF;
}

int
thunk_getchar(void)
{
	return getchar();
}

void
thunk_putchar(int c)
{
	char wc = (char) c;
	write(1, &wc, 1);
}

int
thunk_execv(const char *path, char * const argv[])
{
	return execv(path, argv);
}

int
thunk_open(const char *path, int flags, mode_t mode)
{
	return open(path, flags, mode);
}

int
thunk_fstat_getsize(int fd, ssize_t *size, ssize_t *blksize)
{
	struct stat st;
	int error;

	error = fstat(fd, &st);
	if (error)
		return -1;

	if (size)
		*size = st.st_size;
	if (blksize)
		*blksize = st.st_blksize;

	return 0;
}

ssize_t
thunk_pread(int d, void *buf, size_t nbytes, off_t offset)
{
	return pread(d, buf, nbytes, offset);
}

ssize_t
thunk_pwrite(int d, const void *buf, size_t nbytes, off_t offset)
{
	return pwrite(d, buf, nbytes, offset);
}

ssize_t
thunk_read(int d, void *buf, size_t nbytes)
{
	return read(d, buf, nbytes);
}

ssize_t
thunk_write(int d, const void *buf, size_t nbytes)
{
	return write(d, buf, nbytes);
}

int
thunk_fsync(int fd)
{
	return fsync(fd);
}

int
thunk_mkstemp(char *template)
{
	return mkstemp(template);
}

int
thunk_unlink(const char *path)
{
	return unlink(path);
}

int
thunk_sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
	return sigaction(sig, act, oact);
}

int
thunk_sigaltstack(const stack_t *ss, stack_t *oss)
{
	return sigaltstack(ss, oss);
}

void
thunk_signal(int sig, void (*func)(int))
{
	signal(sig, func);
}

int
thunk_sigblock(int sig)
{
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, sig);
	return sigprocmask(SIG_BLOCK, &set, NULL);
}

int
thunk_sigunblock(int sig)
{
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, sig);
	return sigprocmask(SIG_UNBLOCK, &set, NULL);
}

int
thunk_sigemptyset(sigset_t *sa_mask)
{
	return sigemptyset(sa_mask);
}


void
thunk_sigaddset(sigset_t *sa_mask, int sig)
{
	int retval;
	retval = sigaddset(sa_mask, sig);
	if (retval < 0) {
		perror("%s: bad signal added");
		abort();
	}
}

int
thunk_sigprocmask(int how, const sigset_t * set, sigset_t *oset)
{
	return sigprocmask(how, set, oset);
}

int
thunk_atexit(void (*function)(void))
{
	return atexit(function);
}

int
thunk_aio_read(struct aiocb *aiocbp)
{
	return aio_read(aiocbp);
}

int
thunk_aio_write(struct aiocb *aiocbp)
{
	return aio_write(aiocbp);
}

int
thunk_aio_error(const struct aiocb *aiocbp)
{
	return aio_error(aiocbp);
}

int
thunk_aio_return(struct aiocb *aiocbp)
{
	return aio_return(aiocbp);
}

void *
thunk_malloc(size_t len)
{
	return malloc(len);
}

void
thunk_free(void *addr)
{
	free(addr);
}

void *
thunk_sbrk(intptr_t len)
{
	return sbrk(len);
}

void *
thunk_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
	int nflags, nprot;
	void *a;

	nprot = thunk_to_native_prot(prot);
	nflags = thunk_to_native_mapflags(flags);

	a = mmap(addr, len, nprot, nflags, fd, offset);
	if (a == (void *)-1)
		perror("mmap");
	return a;
}

int
thunk_munmap(void *addr, size_t len)
{
	return munmap(addr, len);
}

int
thunk_mprotect(void *addr, size_t len, int prot)
{
	int nprot;

	nprot = thunk_to_native_prot(prot);

	return mprotect(addr, len, nprot);
}

int
thunk_posix_memalign(void **ptr, size_t alignment, size_t size)
{
	return posix_memalign(ptr, alignment, size);
}

char *
thunk_getenv(const char *name)
{
	return getenv(name);
}

vaddr_t
thunk_get_vm_min_address(void)
{
	return VM_MIN_ADDRESS;
}

int
thunk_idle(void)
{
	sigset_t sigmask;

	sigemptyset(&sigmask);

	return sigsuspend(&sigmask);
}

int
thunk_getcpuinfo(char *cp, int *len)
{
	ssize_t rlen;
	int fd;

	fd = open("/proc/cpuinfo", O_RDONLY);
	if (fd == -1)
		return -1;
	rlen = read(fd, cp, *len - 1);
	close(fd);

	if (rlen == -1)
		return -1;

	*len = rlen;
	return 0;
}

int
thunk_getmachine(char *machine, size_t machinelen,
    char *machine_arch, size_t machine_archlen)
{
	size_t len;

	memset(machine, 0, machinelen);
	len = machinelen - 1;
	if (sysctlbyname("hw.machine", machine, &len, NULL, 0) != 0) {
		perror("sysctlbyname hw.machine failed");
		abort();
	}

	memset(machine_arch, 0, machine_archlen);
	len = machine_archlen - 1;
	if (sysctlbyname("hw.machine_arch", machine_arch, &len, NULL, 0) != 0) {
		perror("sysctlbyname hw.machine_arch failed");
		abort();
	}

	return 0;
}

int
thunk_setown(int fd)
{
	return fcntl(fd, F_SETOWN, getpid());
}

int
thunk_open_tap(const char *device)
{
	int fd, error, enable;

	/* open tap device */
	fd = open(device, O_RDWR);
	if (fd == -1)
		return -1;

	/* set async mode */
	enable = 1;
	error = ioctl(fd, FIOASYNC, &enable);
	if (error)
		return -1;

	return fd;
}

int
thunk_pollin_tap(int fd, int timeout)
{
#if 0
	struct pollfd fds[1];

	fds[0].fd = fd;
	fds[0].events = POLLIN|POLLRDNORM;
	fds[0].revents = 0;

	return poll(fds, __arraycount(fds), timeout);
#else
	int error, len;

	error = ioctl(fd, FIONREAD, &len);
	if (error)
		return 0;

	return len;
#endif
}

int
thunk_pollout_tap(int fd, int timeout)
{
	struct pollfd fds[1];

	fds[0].fd = fd;
	fds[0].events = POLLOUT|POLLWRNORM;
	fds[0].revents = 0;

	return poll(fds, __arraycount(fds), timeout);
}
