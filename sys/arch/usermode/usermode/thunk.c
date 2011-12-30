/* $NetBSD: thunk.c,v 1.61 2011/12/30 11:32:57 reinoud Exp $ */

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
__RCSID("$NetBSD: thunk.c,v 1.61 2011/12/30 11:32:57 reinoud Exp $");
#endif

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/reboot.h>
#include <sys/poll.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <sys/audioio.h>
#include <sys/shm.h>
#include <machine/vmparam.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_tap.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <aio.h>
#include <assert.h>
#include <ctype.h>
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

#define RFB_DEBUG

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

pid_t
thunk_getpid(void)
{
	return getpid();
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
	struct sigaction sa;

	sa.sa_flags = SA_RESTART | SA_ONSTACK;
	sa.sa_sigaction = (void (*)(int, siginfo_t *, void *))func;
	sigemptyset(&sa.sa_mask);
	sigaction(sig, &sa, NULL);
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

int
thunk_audio_open(const char *path)
{
	return open(path, O_RDWR);
}

int
thunk_audio_close(int fd)
{
	return close(fd);
}

int
thunk_audio_drain(int fd)
{
	return ioctl(fd, AUDIO_DRAIN, 0);
}

int
thunk_audio_config(int fd, const thunk_audio_config_t *pconf,
    const thunk_audio_config_t *rconf)
{
	struct audio_info info;
	int error;

	AUDIO_INITINFO(&info);
	info.play.sample_rate = pconf->sample_rate;
	info.play.channels = pconf->channels;
	info.play.precision = pconf->precision;
	info.play.encoding = AUDIO_ENCODING_SLINEAR_LE;
	info.record.sample_rate = rconf->sample_rate;
	info.record.channels = rconf->channels;
	info.record.precision = rconf->precision;
	info.record.encoding = AUDIO_ENCODING_SLINEAR_LE;
	info.mode = AUMODE_PLAY_ALL|AUMODE_RECORD;

	error = ioctl(fd, AUDIO_SETINFO, &info);
	if (error)
		printf("AUDIO_SETINFO failed: %s\n", strerror(errno));

	return error;
}

int
thunk_audio_pollout(int fd)
{
	struct audio_info info;
	int error;

	AUDIO_INITINFO(&info);
	error = ioctl(fd, AUDIO_GETBUFINFO, &info);
	if (error)
		return -1;

	return info.play.buffer_size - info.play.seek;
}

int
thunk_audio_pollin(int fd)
{
	struct audio_info info;
	int error;

	AUDIO_INITINFO(&info);
	error = ioctl(fd, AUDIO_GETBUFINFO, &info);
	if (error)
		return -1;

	return info.record.seek;
}

int
thunk_audio_write(int fd, const void *buf, size_t buflen)
{
	return write(fd, buf, buflen);
}

int
thunk_audio_read(int fd, void *buf, size_t buflen)
{
	return read(fd, buf, buflen);
}

int
thunk_rfb_open(thunk_rfb_t *rfb, uint16_t port)
{
	struct sockaddr_in sin;

	rfb->clientfd = -1;
	rfb->connected = false;

	/* create socket */
	rfb->sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (rfb->sockfd == -1) {
		printf("rfb: couldn't create socket: %s\n", strerror(errno));
		return errno;
	}
	/* bind to requested port */
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(port);
	if (bind(rfb->sockfd, (struct sockaddr *)&sin, sizeof(sin)) != 0) {
		printf("rfb: couldn't bind port %d: %s\n", port,
		    strerror(errno));
		close(rfb->sockfd);
		return errno;
	}
	/* listen for connections */
	if (listen(rfb->sockfd, 1) != 0) {
		printf("rfb: couldn't listen on socket: %s\n", strerror(errno));
		close(rfb->sockfd);
		return errno;
	}

	return 0;
}

static int
safe_send(int s, const void *msg, size_t len)
{
	const uint8_t *p;
	int sent_len;

	p = msg;
	while (len) {
		assert(len >= 0);
		sent_len = send(s, p, len, MSG_NOSIGNAL);
		if (sent_len < 0) 
			return -1;
	
		p   += sent_len;
		len -= sent_len;
	}
	return 0;
}

static int
thunk_rfb_server_init(thunk_rfb_t *rfb)
{
	char msgbuf[80];
	char *p = msgbuf;
	uint32_t namelen = strlen(rfb->name);

	*(uint16_t *)p = htons(rfb->width);	p += 2;
	*(uint16_t *)p = htons(rfb->height);	p += 2;
	*(uint8_t *)p = rfb->depth;		p += 1;
	*(uint8_t *)p = rfb->depth;		p += 1;
	*(uint8_t *)p = 0;			p += 1;	/* endian */
	*(uint8_t *)p = 1;			p += 1; /* true color */
	*(uint16_t *)p = htons(0xff);		p += 2;	/* red max */
	*(uint16_t *)p = htons(0xff);		p += 2;	/* green max */
	*(uint16_t *)p = htons(0xff);		p += 2;	/* blue max */
	*(uint8_t *)p = 0;			p += 1;	/* red shift */
	*(uint8_t *)p = 8;			p += 1;	/* green shift */
	*(uint8_t *)p = 16;			p += 1;	/* blue shift */
	*(uint8_t *)p = 0;			p += 1;	/* padding x3 */
	*(uint8_t *)p = 0;			p += 1;
	*(uint8_t *)p = 0;			p += 1;
	*(uint32_t *)p = htonl(namelen);	p += 4;	/* name length */
	memcpy(p, rfb->name, namelen);		p += namelen;

	return safe_send(rfb->clientfd, msgbuf, p - msgbuf);
}

static int
thunk_rfb_handshake(thunk_rfb_t *rfb)
{
	ssize_t len;
	const char *protover = "RFB 003.003\n";
	uint32_t security_type;
	uint8_t shared_flag;
	char dummy;

	/* send server protocol version */
	len = safe_send(rfb->clientfd, protover, strlen(protover));
	if (len < 0)
		return errno;

	/* receive client protocol version */
	do {
		len = recv(rfb->clientfd, &dummy, sizeof(dummy), MSG_NOSIGNAL);
		if (len < 0)
			return errno;
		if (len == 0)
			return EIO;
	} while (dummy != '\n');

	/* send security capabilities */
	security_type = htonl(1);	/* no security */
	len = safe_send(rfb->clientfd, &security_type, sizeof(security_type));
	if (len < 0)
		return errno;

	/* receive client init message */
	len = recv(rfb->clientfd, &shared_flag, sizeof(shared_flag),
		MSG_NOSIGNAL);
	if (len <= 0)
		return errno;

	/* send server init message */
	len = thunk_rfb_server_init(rfb);
	if (len < 0)
		return errno;

	return 0;
}

static void
thunk_rfb_send_pending(thunk_rfb_t *rfb)
{
	thunk_rfb_update_t *update;
	uint8_t rfb_update[16];
	uint8_t *p;
	unsigned int n;
	unsigned int bytes_per_pixel;
	ssize_t stride, line_len, len;

	if (rfb->connected == false || rfb->nupdates == 0)
		return;

	/* If we have too many updates queued, just send a single update */
	if (rfb->nupdates >= __arraycount(rfb->update)) {
		rfb->nupdates = 1;
		rfb->update[0].x = 0;
		rfb->update[0].y = 0;
		rfb->update[0].w = rfb->width;
		rfb->update[0].h = rfb->height;
	}

#ifdef RFB_DEBUG
	fprintf(stdout, "rfb: sending %d updates\n", rfb->nupdates);
#endif

	p = rfb_update;
	*(uint8_t *)p = 0;		p += 1;		/* FramebufferUpdate */
	*(uint8_t *)p = 0;		p += 1;		/* padding */
	*(uint16_t *)p = htons(rfb->nupdates);	p += 2;	/* # rects */

	len = safe_send(rfb->clientfd, rfb_update, 4);
	if (len < 0)
		goto disco;

	bytes_per_pixel = rfb->depth / 8;
	stride = rfb->width * bytes_per_pixel;
	for (n = 0; n < rfb->nupdates; n++) {
		p = rfb_update;
		update = &rfb->update[n];
		*(uint16_t *)p = htons(update->x);	p += 2;
		*(uint16_t *)p = htons(update->y);	p += 2;
		*(uint16_t *)p = htons(update->w);	p += 2;
		*(uint16_t *)p = htons(update->h);	p += 2;
		*(uint32_t *)p = htonl(0);		p += 4;	/* Raw enc */

#ifdef RFB_DEBUG
		fprintf(stdout, "rfb:   [%u] x=%d y=%d w=%d h=%d\n",
		    n, update->x, update->y, update->w, update->h);
#endif

		len = safe_send(rfb->clientfd, rfb_update, 12);
		if (len < 0)
			goto disco;

		p = rfb->framebuf + (update->y * stride) + (update->x * bytes_per_pixel);
		line_len = update->w * bytes_per_pixel;
		while (update->h-- > 0) {
			len = safe_send(rfb->clientfd, p, line_len);
			if (len < 0)
				goto disco;
			p += stride;
		}
	}

	rfb->nupdates = 0;
	rfb->first_mergable = 0;

	return;

disco:
	fprintf(stdout, "rfb: client disconnected: %s\n", strerror(errno));
	close(rfb->clientfd);
	rfb->clientfd = -1;
	rfb->connected = false;
}

int
thunk_rfb_poll(thunk_rfb_t *rfb, thunk_rfb_event_t *event)
{
	int error, len, msg_len;
	uint8_t set_pixel_format[19];
	uint8_t set_encodings[3];
	uint8_t framebuffer_update_request[9];
	uint8_t key_event[7];
	uint8_t pointer_event[5];
	uint8_t client_cut_text[7];
	uint8_t ch;

	if (rfb->clientfd == -1) {
		struct sockaddr_in sin;
		struct pollfd fds[1];
		socklen_t sinlen;
		int flags;

		/* poll for connections */
		fds[0].fd = rfb->sockfd;
		fds[0].events = POLLIN;
		fds[0].revents = 0;
		if (poll(fds, __arraycount(fds), 0) != 1)
			return -1;

		sinlen = sizeof(sin);
		rfb->clientfd = accept(rfb->sockfd, (struct sockaddr *)&sin,
		    &sinlen);
		if (rfb->clientfd == -1)
			return -1;

		fprintf(stdout, "rfb: connection from %s:%d\n",
		    inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));

		/* rfb handshake */
		if (thunk_rfb_handshake(rfb) != 0) {
			fprintf(stdout, "rfb: handshake failed\n");
			close(rfb->clientfd);
			rfb->clientfd = -1;
			return -1;
		}

		rfb->connected = true;

		/* enable sigio on input */
		flags = fcntl(rfb->clientfd, F_GETFL, 0);
		fcntl(rfb->clientfd, F_SETFL, flags | O_ASYNC);
		error = fcntl(rfb->clientfd, F_SETOWN, getpid());
		if (error) {
			fprintf(stdout, "rfb: setown failed: %s\n",
			    strerror(errno));
			close(rfb->clientfd);
			rfb->clientfd = -1;
			return -1;
		}

		rfb->nupdates = 0;
		rfb->first_mergable = 0;
		thunk_rfb_update(rfb, 0, 0, rfb->width, rfb->height);
	}

	thunk_rfb_send_pending(rfb);
	if (rfb->clientfd == -1)
		return -1;

	error = ioctl(rfb->clientfd, FIONREAD, &len);
	if (error) {
		//printf("rfb: FIONREAD failed: %s\n", strerror(errno));
		close(rfb->clientfd);
		rfb->clientfd = -1;
		return -1;
	}
	if (len == 0)
		return 0;

	recv(rfb->clientfd, &ch, sizeof(ch), MSG_NOSIGNAL);

	event->message_type = ch;
	switch (ch) {
	case THUNK_RFB_SET_PIXEL_FORMAT:
		msg_len = sizeof(set_pixel_format);
		break;
	case THUNK_RFB_SET_ENCODINGS:
		recv(rfb->clientfd, set_encodings, sizeof(set_encodings),
		    MSG_NOSIGNAL);
		msg_len = 4 * ntohs(*(uint16_t *)&set_encodings[1]);
		break;
	case THUNK_RFB_FRAMEBUFFER_UPDATE_REQUEST:
		msg_len = sizeof(framebuffer_update_request);
		break;
	case THUNK_RFB_KEY_EVENT:
		recv(rfb->clientfd, key_event, sizeof(key_event), MSG_NOSIGNAL);
		event->data.key_event.down_flag = key_event[0];
		event->data.key_event.keysym =
		    ntohl(*(uint32_t *)&key_event[3]);
		msg_len = 0;
		break;
	case THUNK_RFB_POINTER_EVENT:
		msg_len = sizeof(pointer_event);
		break;
	case THUNK_RFB_CLIENT_CUT_TEXT:
		recv(rfb->clientfd, client_cut_text, sizeof(client_cut_text),
		    MSG_NOSIGNAL);
		msg_len = ntohl(*(uint32_t *)&client_cut_text[3]);
		break;
	default:
		fprintf(stdout, "rfb: unknown message type %d\n", ch);
		close(rfb->clientfd);
		rfb->clientfd = -1;
		return -1;
	}

	/* discard any remaining bytes */
	while (msg_len-- > 0) {
		recv(rfb->clientfd, &ch, sizeof(ch), MSG_NOSIGNAL);
	}

	return 1;
}

void
thunk_rfb_update(thunk_rfb_t *rfb, int x, int y, int w, int h)
{
	thunk_rfb_update_t *update = NULL;
	unsigned int n;

	/* if the queue is full, just return */
	if (rfb->nupdates >= __arraycount(rfb->update))
		return;

	/* no sense in queueing duplicate updates */
	for (n = rfb->first_mergable; n < rfb->nupdates; n++) {
		if (rfb->update[n].x == x && rfb->update[n].y == y &&
		    rfb->update[n].w == w && rfb->update[n].h == h)
			return;
	}

#ifdef RFB_DEBUG
	fprintf(stdout, "rfb: queue slot %d, x=%d y=%d w=%d h=%d\n",
	    rfb->nupdates, x, y, w, h);
#endif

	/* add the update request to the queue */
	update = &rfb->update[rfb->nupdates++];
	update->type = THUNK_RFB_TYPE_UPDATE;
	update->x = x;
	update->y = y;
	update->w = w;
	update->h = h;
}
