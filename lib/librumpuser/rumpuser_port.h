/*	$NetBSD: rumpuser_port.h,v 1.6 2012/11/22 21:23:08 pooka Exp $	*/

/*
 * Portability header for non-NetBSD platforms.
 * Quick & dirty.
 * Maybe should try to use the infrastructure in tools/compat instead?
 */

/*
 * XXX:
 * There is currently no errno translation for the error values reported
 * by the hypercall layer.
 */

#ifndef _LIB_LIBRUMPUSER_RUMPUSER_PORT_H_
#define _LIB_LIBRUMPUSER_RUMPUSER_PORT_H_

#ifdef __NetBSD__
#include <sys/cdefs.h>
#include <sys/param.h>

#define PLATFORM_HAS_KQUEUE
#define PLATFORM_HAS_CHFLAGS
#define PLATFORM_HAS_NBMOUNT
#define PLATFORM_HAS_NFSSVC
#define PLATFORM_HAS_FSYNC_RANGE
#define PLATFORM_HAS_NBSYSCTL
#define PLATFORM_HAS_NBFILEHANDLE

#if __NetBSD_Prereq__(5,99,48)
#define PLATFORM_HAS_NBQUOTA
#endif

/*
 * This includes also statvfs1() and fstatvfs1().  They could be
 * reasonably easily emulated on other platforms.
 */
#define PLATFORM_HAS_NBVFSSTAT
#endif

#ifdef __linux__
#define _XOPEN_SOURCE 600
#define _BSD_SOURCE
#define _FILE_OFFSET_BITS 64
#define _GNU_SOURCE
#include <features.h>
#endif

#include <sys/types.h>
#include <sys/param.h>

#if defined(__linux__) || defined(__sun__)
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* this is inline simply to make this header self-contained */
static inline int 
getenv_r(const char *name, char *buf, size_t buflen)
{
	char *tmp;

	if ((tmp = getenv(name)) != NULL) {
		if (strlen(tmp) >= buflen) {
			errno = ERANGE;
			return -1;
		}
		strcpy(buf, tmp);
		return 0;
	} else {
		errno = ENOENT;
		return -1;
	}
}
#endif

/* Solarisa 10 has memalign() but no posix_memalign() */
#if defined(__sun__)
#include <stdlib.h>

static inline int
posix_memalign(void **ptr, size_t align, size_t size)
{

	*ptr = memalign(align, size);
	if (*ptr == NULL)
		return ENOMEM;
	return 0;
}
#endif /* __sun__ */

#ifndef __RCSID
#define __RCSID(a)
#endif

#ifndef INFTIM
#define INFTIM (-1)
#endif

#ifndef _DIAGASSERT
#define _DIAGASSERT(_p_)
#endif

#if defined(__linux__) || defined(__sun__)
#define SA_SETLEN(a,b)
#else /* BSD */
#define SA_SETLEN(_sa_, _len_) ((struct sockaddr *)_sa_)->sa_len = _len_
#endif

#ifndef __predict_true
#define __predict_true(a) a
#define __predict_false(a) a
#endif

#ifndef __dead
#define __dead __attribute__((__noreturn__))
#endif

#ifndef __printflike
#define __printflike(a,b)
#endif

#ifndef __noinline
#ifdef __GNUC__
#define __noinline __attribute__((__noinline__))
#else
#define __noinline
#endif
#endif

#ifndef __arraycount
#define __arraycount(_ar_) (sizeof(_ar_)/sizeof(_ar_[0]))
#endif

#ifndef __UNCONST
#define __UNCONST(_a_) ((void *)(unsigned long)(const void *)(_a_))
#endif

#if defined(__linux__) || defined(__sun__)
#define arc4random() random()
#endif

#ifndef __NetBSD_Prereq__
#define __NetBSD_Prereq__(a,b,c) 0
#endif

#include <sys/socket.h>

#if !defined(__CMSG_ALIGN)
#ifdef CMSG_ALIGN
#define __CMSG_ALIGN(a) CMSG_ALIGN(a)
#endif
#endif

#ifndef PF_LOCAL
#define PF_LOCAL PF_UNIX
#endif
#ifndef AF_LOCAL
#define AF_LOCAL AF_UNIX
#endif

/* pfft, but what are you going to do? */
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#if defined(__sun__) && !defined(RUMP_REGISTER_T)
#define RUMP_REGISTER_T long
typedef RUMP_REGISTER_T register_t;
#endif

#endif /* _LIB_LIBRUMPUSER_RUMPUSER_PORT_H_ */
