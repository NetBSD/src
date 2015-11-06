/*	$NetBSD: svc_fdset.c,v 1.2 2015/11/06 19:34:13 christos Exp $	*/

#include <sys/cdefs.h>
__RCSID("$NetBSD: svc_fdset.c,v 1.2 2015/11/06 19:34:13 christos Exp $");


#include "reentrant.h"

#include <sys/fd_set.h>

#include <rpc/rpc.h>

#include <stdlib.h>
#include <string.h>

struct svc_fdset {
	fd_set *fdset;
	int	fdmax;
	int	fdsize;
};


/* The single threaded, one global fd_set version */
static fd_set *__svc_fdset;
static int svc_fdsize = 0;

/*
 * Update the old global svc_fdset if needed for binary compatibility
 */
#define COMPAT_UPDATE(a)				\
	do						\
		if ((a) == __svc_fdset)			\
			svc_fdset = *__svc_fdset;	\
	while (/*CONSTCOND*/0)

static thread_key_t fdsetkey = -2;

#ifdef FDSET_DEBUG
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <lwp.h>

static void  __printflike(3, 0)
svc_header(const char *func, size_t line, const char *fmt, va_list ap)
{
	fprintf(stderr, "%s[%d.%d]: %s, %zu: ", getprogname(), (int)getpid(),
	    (int)_lwp_self(), func, line);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

static void __printflike(5, 6)
svc_fdset_print(const char *func, size_t line, const fd_set *fds, int fdmax,
    const char *fmt, ...)
{
	va_list ap;
	const char *did = "";

	va_start(ap, fmt);
	svc_header(func, line, fmt, ap);
	va_end(ap);

	if (fdmax == 0)
		fdmax = FD_SETSIZE;

	fprintf(stderr, "%p[%d] <", fds, fdmax);
	for (int i = 0; i <= fdmax; i++) {
		if (!FD_ISSET(i, fds))
			continue;
		fprintf(stderr, "%s%d", did, i);
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

#define DPRINTF_FDSET(...)	svc_fdset_print(__func__, __LINE__, __VA_ARGS__)
#define DPRINTF(...)		svc_print(__func__, __LINE__, __VA_ARGS__)
#else
#define DPRINTF_FDSET(...)
#define DPRINTF(...)
#endif


static void
svc_fdset_free(void *v)
{
	struct svc_fdset *rv = v;
	DPRINTF_FDSET(rv->fdset, 0, "free");

	free(rv->fdset);
	free(rv);
}

static fd_set *
svc_fdset_resize(int fd, fd_set **fdset, int *fdsize)
{
	if (*fdset && fd < *fdsize) {
		DPRINTF_FDSET(*fdset, 0, "keeping %d < %d",
		    fd, *fdsize);
		return *fdset;
	}

	fd += FD_SETSIZE; 
	if (fd == 517)
		abort();

	char *newfdset = realloc(*fdset, __NFD_BYTES(fd));
	if (newfdset == NULL)
		return NULL;

	memset(newfdset + __NFD_BYTES(*fdsize), 0,
	    __NFD_BYTES(fd) - __NFD_BYTES(*fdsize));


	*fdset = (void *)newfdset;
	DPRINTF_FDSET(*fdset, 0, "resize %d > %d", fd, *fdsize);
	*fdsize = fd;

	COMPAT_UPDATE(*fdset);

	return *fdset;
}

static struct svc_fdset *
svc_fdset_alloc(int fd)
{
	struct svc_fdset *rv;

	if (fdsetkey == -1)
		thr_keycreate(&fdsetkey, svc_fdset_free);

	if ((rv = thr_getspecific(fdsetkey)) == NULL) {

		rv = calloc(1, sizeof(*rv));
		if (rv == NULL)
			return NULL;

		(void)thr_setspecific(fdsetkey, rv);

		if (svc_fdsize != 0) {
			rv->fdset = __svc_fdset;
			DPRINTF("switching to %p", rv->fdset);
			rv->fdmax = svc_maxfd;
			rv->fdsize = svc_fdsize;

			svc_fdsize = 0;
		} else {
			DPRINTF("first thread time %p", rv->fdset);
		}
	} else {
		DPRINTF("again for %p", rv->fdset);
		if (fd < rv->fdsize)
			return rv;
	}
	if (svc_fdset_resize(fd, &rv->fdset, &rv->fdsize) == NULL)
		return NULL;
	return rv;
}

static fd_set *
svc_fdset_get_internal(int fd)
{
	struct svc_fdset *rv;

	if (!__isthreaded || fdsetkey == -2)
		return svc_fdset_resize(fd, &__svc_fdset, &svc_fdsize);

	rv = svc_fdset_alloc(fd);
	if (rv == NULL)
		return NULL;
	return rv->fdset;
}


/* allow each thread to have their own copy */
void
svc_fdset_init(int flags)
{
	DPRINTF("%x", flags);
	if ((flags & SVC_FDSET_MT) && fdsetkey == -2)
		fdsetkey = -1;
}

void
svc_fdset_zero(void)
{
	DPRINTF("zero");
	fd_set *fds = svc_fdset_get_internal(0);
	int size = svc_fdset_getsize(0);
	memset(fds, 0, __NFD_BYTES(size));
	*svc_fdset_getmax() = 0;

	COMPAT_UPDATE(fds);

}

void
svc_fdset_set(int fd)
{
	fd_set *fds = svc_fdset_get_internal(fd);
	int *fdmax = svc_fdset_getmax();
	FD_SET(fd, fds);
	if (fd > *fdmax)
		*fdmax = fd;
	DPRINTF_FDSET(fds, *fdmax, "%d", fd);

	COMPAT_UPDATE(fds);
}

int
svc_fdset_isset(int fd)
{
	fd_set *fds = svc_fdset_get_internal(fd);
	DPRINTF_FDSET(fds, 0, "%d", fd);
	return FD_ISSET(fd, fds);
}

void
svc_fdset_clr(int fd)
{
	fd_set *fds = svc_fdset_get_internal(fd);
	FD_CLR(fd, fds);
	/* XXX: update fdmax? */
	DPRINTF_FDSET(fds, 0, "%d", fd);

	COMPAT_UPDATE(fds);
}

fd_set *
svc_fdset_copy(const fd_set *orig)
{
	int len, fdmax;
	fd_set *fds;
	
	len = 0;
	fds = 0;
	fdmax = *svc_fdset_getmax();

	DPRINTF_FDSET(orig, 0, "[orig]");
	fds = svc_fdset_resize(fdmax, &fds, &len);
	if (fds == NULL)
		return NULL;

	if (orig)
		memcpy(fds, orig, __NFD_BYTES(fdmax));
	DPRINTF_FDSET(fds, 0, "[copy]");
	return fds;
}

fd_set *
svc_fdset_get(void)
{
	fd_set *fds = svc_fdset_get_internal(0);
	DPRINTF_FDSET(fds, 0, "get");
	return fds;
}

int *
svc_fdset_getmax(void)
{
	struct svc_fdset *rv;

	if (!__isthreaded || fdsetkey == -2)
		return &svc_maxfd;
		
	rv = svc_fdset_alloc(0);
	if (rv == NULL)
		return NULL;
	return &rv->fdmax;
}

int
svc_fdset_getsize(int fd)
{
	struct svc_fdset *rv;

	if (!__isthreaded || fdsetkey == -2) {
		if (svc_fdset_resize(fd, &__svc_fdset, &svc_fdsize) == NULL)
			return -1;
		else
			return svc_fdsize;
	}

	rv = svc_fdset_alloc(fd);
	if (rv == NULL)
		return -1;
	return rv->fdsize;
}
