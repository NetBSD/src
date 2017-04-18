/*	$NetBSD: svc_fdset.c,v 1.16 2017/04/18 11:35:34 maya Exp $	*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__RCSID("$NetBSD: svc_fdset.c,v 1.16 2017/04/18 11:35:34 maya Exp $");


#include "reentrant.h"

#include <sys/fd_set.h>

#include <rpc/rpc.h>

#ifdef FDSET_DEBUG
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <lwp.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <poll.h>

#include "svc_fdset.h"

#undef svc_fdset
#undef svc_maxfd
#ifdef _LIBC
extern __fd_set_256 svc_fdset;
#endif
extern int svc_maxfd;
int __svc_flags;

struct svc_fdset {
	/* select */
	fd_set *fdset;
	int	fdmax;
	int	fdsize;
	/* poll */
	struct pollfd *fdp;
	int	fdnum;
	int	fdused;
};

/* The single threaded, one global fd_set version */
static struct svc_fdset __svc_fdset;

static thread_key_t fdsetkey = -2;

#ifdef FDSET_DEBUG

static void  __printflike(3, 0)
svc_header(const char *func, size_t line, const char *fmt, va_list ap)
{
	fprintf(stderr, "%s[%d.%d]: %s, %zu: ", getprogname(), (int)getpid(),
	    (int)_lwp_self(), func, line);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

static void __printflike(4, 5)
svc_fdset_print(const char *func, size_t line, struct svc_fdset *fds, 
    const char *fmt, ...)
{
	va_list ap;
	const char *did = "";

	va_start(ap, fmt);
	svc_header(func, line, fmt, ap);
	va_end(ap);

	fprintf(stderr, "%p[%d] fd_set<", fds->fdset, fds->fdmax);
	for (int i = 0; i <= fds->fdmax; i++) {
		if (!FD_ISSET(i, fds->fdset))
			continue;
		fprintf(stderr, "%s%d", did, i);
		did = ", ";
	}
	did = "";
	fprintf(stderr, "> poll<");
	for (int i = 0; i < fds->fdused; i++) {
		int fd = fds->fdp[i].fd;
		if (fd == -1)
			continue;
		fprintf(stderr, "%s%d", did, fd);
		did = ", ";
	}
	fprintf(stderr, ">\n");
}

static void __printflike(3, 4)
svc_print(const char *func, size_t line, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	svc_header(func, line, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

#define DPRINTF(...)		svc_print(__func__, __LINE__, __VA_ARGS__)
#define DPRINTF_FDSET(...)	svc_fdset_print(__func__, __LINE__, __VA_ARGS__)

#else

#define DPRINTF(...)
#define DPRINTF_FDSET(...)

#endif


static inline void
svc_fdset_sanitize(struct svc_fdset *fds)
{
	while (fds->fdmax >= 0 && !FD_ISSET(fds->fdmax, fds->fdset))
		fds->fdmax--;
#ifdef _LIBC
	/* Compat update */
	if (fds == &__svc_fdset) {
		svc_fdset = *(__fd_set_256 *)(void *)__svc_fdset.fdset;
		svc_maxfd = __svc_fdset.fdmax;
	}
#endif
}

static void
svc_fdset_free(void *v)
{
	struct svc_fdset *fds = v;
	DPRINTF_FDSET(fds, "free");

	free(fds->fdp);
	free(fds->fdset);
	free(fds);
}

static void
svc_pollfd_init(struct pollfd *pfd, int nfd)
{
	for (int i = 0; i < nfd; i++) {
		pfd[i].fd = -1;
		pfd[i].events = POLLIN | POLLPRI | POLLRDNORM | POLLRDBAND;
		pfd[i].revents = 0;
	}
}

static struct pollfd *
svc_pollfd_alloc(struct svc_fdset *fds)
{
	if (fds->fdp != NULL)
		return fds->fdp;
		
	fds->fdnum = FD_SETSIZE;
	fds->fdp = calloc(fds->fdnum, sizeof(*fds->fdp));
	if (fds->fdp == NULL)
		return NULL;
	svc_pollfd_init(fds->fdp, fds->fdnum);
	return fds->fdp;
}


static struct svc_fdset *
svc_pollfd_add(int fd, struct svc_fdset *fds)
{
	struct pollfd *pfd;

	if ((pfd = svc_pollfd_alloc(fds)) == NULL)
		return NULL;

	for (int i = 0; i < fds->fdnum; i++)
		if (pfd[i].fd == -1) {
			if (i >= fds->fdused)
				fds->fdused = i + 1;
			DPRINTF("add fd=%d slot=%d fdused=%d",
			    fd, i, fds->fdused);
			pfd[i].fd = fd;
			return fds;
		}

	pfd = realloc(fds->fdp, (fds->fdnum + FD_SETSIZE) * sizeof(*fds->fdp));
	if (pfd == NULL)
		return NULL;

	svc_pollfd_init(pfd + fds->fdnum, FD_SETSIZE);
	pfd[fds->fdnum].fd = fd;
	fds->fdused = fds->fdnum + 1;
	DPRINTF("add fd=%d slot=%d fdused=%d", fd, fds->fdnum, fds->fdused);
	fds->fdnum += FD_SETSIZE;
	fds->fdp = pfd;
	return fds;
}

static struct svc_fdset *
svc_pollfd_del(int fd, struct svc_fdset *fds)
{
	struct pollfd *pfd;

	if ((pfd = svc_pollfd_alloc(fds)) == NULL)
		return NULL;

	for (int i = 0; i < fds->fdnum; i++) {
		if (pfd[i].fd != fd)
			continue;

		pfd[i].fd = -1;
		DPRINTF("del fd=%d slot=%d", fd, fds->fdused);
		if (i != fds->fdused - 1)
			return fds;

		do
			if (pfd[i].fd != -1) 
				break;
		while (--i >= 0);

		fds->fdused = i + 1;
		DPRINTF("del fd=%d fdused=%d", fd, fds->fdused);
		return fds;
	}
	DPRINTF("del fd=%d not found", fd);
	return NULL;
}

static struct svc_fdset *
svc_fdset_resize(int fd, struct svc_fdset *fds)
{
	if (fds->fdset && fd < fds->fdsize) {
		DPRINTF_FDSET(fds, "keeping %d < %d", fd, fds->fdsize);
		return fds;
	}

	fd += FD_SETSIZE; 

	char *newfdset = realloc(fds->fdset, __NFD_BYTES(fd));
	if (newfdset == NULL)
		return NULL;

	memset(newfdset + __NFD_BYTES(fds->fdsize), 0,
	    __NFD_BYTES(fd) - __NFD_BYTES(fds->fdsize));


	fds->fdset = (void *)newfdset;
	DPRINTF_FDSET(fds, "resize %d > %d", fd, fds->fdsize);
	fds->fdsize = fd;

	return fds;
}

static struct svc_fdset *
svc_fdset_alloc(int fd)
{
	struct svc_fdset *fds;

	if (!__isthreaded || fdsetkey == -2)
		return svc_fdset_resize(fd, &__svc_fdset);

	if (fdsetkey == -1)
		thr_keycreate(&fdsetkey, svc_fdset_free);

	if ((fds = thr_getspecific(fdsetkey)) == NULL) {

		fds = calloc(1, sizeof(*fds));
		if (fds == NULL)
			return NULL;

		(void)thr_setspecific(fdsetkey, fds);

		if (__svc_fdset.fdsize != 0) {
			*fds = __svc_fdset;
			DPRINTF("switching to %p", fds->fdset);
		} else {
			DPRINTF("first thread time %p", fds->fdset);
		}
	} else {
		DPRINTF("again for %p", fds->fdset);
		if (fd < fds->fdsize)
			return fds;
	}

	return svc_fdset_resize(fd, fds);
}

/* allow each thread to have their own copy */
void
svc_fdset_init(int flags)
{
	DPRINTF("%x", flags);
	__svc_flags = flags;
	if ((flags & SVC_FDSET_MT) && fdsetkey == -2)
		fdsetkey = -1;
}

void
svc_fdset_zero(void)
{
	DPRINTF("zero");

	struct svc_fdset *fds = svc_fdset_alloc(0);
	if (fds == NULL)
		return;
	memset(fds->fdset, 0, fds->fdsize);
	fds->fdmax = -1;

	free(fds->fdp);
	fds->fdp = NULL;
	fds->fdnum = fds->fdused = 0;
}

int
svc_fdset_set(int fd)
{
	struct svc_fdset *fds = svc_fdset_alloc(fd);

	if (fds == NULL)
		return -1;

	FD_SET(fd, fds->fdset);
	if (fd > fds->fdmax)
		fds->fdmax = fd;

	int rv = svc_pollfd_add(fd, fds) ? 0 : -1;
	DPRINTF_FDSET(fds, "%d", fd);

	svc_fdset_sanitize(fds);
	return rv;
}

int
svc_fdset_isset(int fd)
{
	struct svc_fdset *fds = svc_fdset_alloc(fd);

	if (fds == NULL)
		return -1;

	DPRINTF_FDSET(fds, "%d", fd);

	return FD_ISSET(fd, fds->fdset) != 0;
}

int
svc_fdset_clr(int fd)
{
	struct svc_fdset *fds = svc_fdset_alloc(fd);

	if (fds == NULL)
		return -1;

	FD_CLR(fd, fds->fdset);

	int rv = svc_pollfd_del(fd, fds) ? 0 : -1;
	DPRINTF_FDSET(fds, "%d", fd);

	svc_fdset_sanitize(fds);
	return rv;
}

fd_set *
svc_fdset_copy(const fd_set *orig)
{
	int size = svc_fdset_getsize(0);
	if (size == -1)
		return NULL;
	fd_set *copy = calloc(1, __NFD_BYTES(size));
	if (copy == NULL)
		return NULL;
	if (orig)
		memcpy(copy, orig, __NFD_BYTES(size));
	return copy;
}

fd_set *
svc_fdset_get(void)
{
	struct svc_fdset *fds = svc_fdset_alloc(0);

	if (fds == NULL)
		return NULL;

	DPRINTF_FDSET(fds, "get");
	svc_fdset_sanitize(fds);
	return fds->fdset;
}

int *
svc_fdset_getmax(void)
{
	struct svc_fdset *fds = svc_fdset_alloc(0);

	if (fds == NULL)
		return NULL;

	DPRINTF_FDSET(fds, "getmax");
	svc_fdset_sanitize(fds);
	return &fds->fdmax;
}

int
svc_fdset_getsize(int fd)
{
	struct svc_fdset *fds = svc_fdset_alloc(fd);

	if (fds == NULL)
		return -1;

	DPRINTF_FDSET(fds, "getsize");
	return fds->fdsize;
}

struct pollfd *
svc_pollfd_copy(const struct pollfd *orig)
{
	int size = svc_fdset_getsize(0);
	if (size == -1)
		return NULL;
	struct pollfd *copy = calloc(size, sizeof(*orig));
	if (copy == NULL)
		return NULL;
	if (orig)
		memcpy(copy, orig, size * sizeof(*orig));
	return copy;
}

struct pollfd *
svc_pollfd_get(void)
{
	struct svc_fdset *fds = svc_fdset_alloc(0);

	if (fds == NULL)
		return NULL;

	DPRINTF_FDSET(fds, "getpoll");
	return fds->fdp;
}

int *
svc_pollfd_getmax(void)
{
	struct svc_fdset *fds = svc_fdset_alloc(0);

	if (fds == NULL)
		return NULL;
	return &fds->fdused;
}

int
svc_pollfd_getsize(int fd)
{
	struct svc_fdset *fds = svc_fdset_alloc(fd);

	if (fds == NULL)
		return -1;

	DPRINTF_FDSET(fds, "getsize");
	return fds->fdnum;
}
