/*	$NetBSD: rumpuser_port.h,v 1.2 2012/08/25 18:00:06 pooka Exp $	*/

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

#if defined(__linux__)
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

#ifndef __RCSID
#define __RCSID(a)
#endif

#ifndef INFTIM
#define INFTIM (-1)
#endif

#ifndef _DIAGASSERT
#define _DIAGASSERT(_p_)
#endif

#ifdef __linux__
#define SA_SETLEN(a,b)
#else /* BSD */
#define SA_SETLEN(_sa_, _len_) ((struct sockaddr *)_sa_)->sa_len = _len_
#endif

#ifndef __predict_true
#define __predict_true(a) a
#define __predict_false(a) a
#endif

#ifndef __dead
#define __dead
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

#ifdef __linux__
#define arc4random() random()
#endif

#ifndef __NetBSD_Prereq__
#define __NetBSD_Prereq__(a,b,c) 0
#endif

#if !defined(__CMSG_ALIGN)
#include <sys/socket.h>
#ifdef CMSG_ALIGN
#define __CMSG_ALIGN(a) CMSG_ALIGN(a)
#endif
#endif

#endif /* _LIB_LIBRUMPUSER_RUMPUSER_PORT_H_ */
