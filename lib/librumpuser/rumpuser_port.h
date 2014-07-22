/*	$NetBSD: rumpuser_port.h,v 1.34 2014/07/22 22:41:58 justin Exp $	*/

/*
 * Portability header for non-NetBSD platforms.
 * Quick & dirty.
 * Maybe should try to use the infrastructure in tools/compat instead?
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
#ifndef HAVE_PTHREAD_SETNAME_3
#define HAVE_PTHREAD_SETNAME_3
#endif

#define PLATFORM_HAS_STRSUFTOLL
#define PLATFORM_HAS_SETGETPROGNAME

#if __NetBSD_Prereq__(5,99,48)
#define PLATFORM_HAS_NBQUOTA
#endif

#if __NetBSD_Prereq__(6,99,16)
#define HAVE_CLOCK_NANOSLEEP
#endif

/*
 * This includes also statvfs1() and fstatvfs1().  They could be
 * reasonably easily emulated on other platforms.
 */
#define PLATFORM_HAS_NBVFSSTAT
#endif /* __NetBSD__ */

#ifndef MIN
#define MIN(a,b)        ((/*CONSTCOND*/(a)<(b))?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b)        ((/*CONSTCOND*/(a)>(b))?(a):(b))
#endif

/* might not be 100% accurate, maybe need to revisit later */
#if (defined(__linux__) && !defined(__ANDROID__)) || defined(__sun__)
#define HAVE_CLOCK_NANOSLEEP
#endif

#ifdef __linux__
#define _XOPEN_SOURCE 600
#define _BSD_SOURCE
#define _GNU_SOURCE
#endif

#ifdef __ANDROID__
#include <stdint.h>
typedef uint16_t in_port_t;
#include <sys/select.h>
#define atomic_inc_uint(x)  __sync_fetch_and_add(x, 1)
#define atomic_dec_uint(x)  __sync_fetch_and_sub(x, 1)
static inline int getsubopt(char **optionp, char * const *tokens, char **valuep);
static inline int
getsubopt(char **optionp, char * const *tokens, char **valuep)
{

	/* TODO make a definition */
	return -1;
}
#endif

#if defined(__sun__)
#  if defined(RUMPUSER_NO_FILE_OFFSET_BITS)
#    undef _FILE_OFFSET_BITS
#  endif
#endif

#if defined(__APPLE__)
#define	__dead		__attribute__((noreturn))
#include <sys/cdefs.h>

#include <libkern/OSAtomic.h>
#define	atomic_inc_uint(x)	OSAtomicIncrement32((volatile int32_t *)(x))
#define	atomic_dec_uint(x)	OSAtomicDecrement32((volatile int32_t *)(x))

#include <sys/time.h>

#define	CLOCK_REALTIME	0
typedef int clockid_t;

static inline int
clock_gettime(clockid_t clk, struct timespec *ts)
{
	struct timeval tv;

	if (gettimeofday(&tv, 0) == 0) {
		ts->tv_sec = tv.tv_sec;
		ts->tv_nsec = tv.tv_usec * 1000;
	}
	return -1;
}

#endif

#include <sys/types.h>
#include <sys/param.h>

/* NetBSD is the only(?) platform with getenv_r() */
#if !defined(__NetBSD__)
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

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

#if defined(__sun__)
#include <sys/sysmacros.h>

#if !defined(HAVE_POSIX_MEMALIGN)
/* Solarisa 10 has memalign() but no posix_memalign() */
#include <stdlib.h>

static inline int
posix_memalign(void **ptr, size_t align, size_t size)
{

	*ptr = memalign(align, size);
	if (*ptr == NULL)
		return ENOMEM;
	return 0;
}
#endif /* !HAVE_POSIX_MEMALIGN */
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

#if defined(__linux__) || defined(__sun__) || defined(__CYGWIN__)
#define SIN_SETLEN(a,b)
#else /* BSD */
#define SIN_SETLEN(_sin_, _len_) _sin_.sin_len = _len_
#endif

#ifndef __predict_true
#define __predict_true(a) a
#define __predict_false(a) a
#endif

#ifndef __dead
#define __dead __attribute__((__noreturn__))
#endif

#ifndef __printflike
#ifdef __GNUC__
#define __printflike(a,b) __attribute__((__format__ (__printf__,a,b)))
#else
#define __printflike(a,b)
#endif
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

#ifndef __CONCAT
#define __CONCAT(x,y)	x ## y
#endif

#ifndef __STRING
#define __STRING(x)	#x
#endif

#if (defined(__NetBSD__) && __NetBSD_Version__ > 600000000) || \
  defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
#define PLATFORM_HAS_ARC4RANDOM_BUF
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

#if (defined(__sun__) || defined(__ANDROID__)) && !defined(RUMP_REGISTER_T)
#define RUMP_REGISTER_T long
typedef RUMP_REGISTER_T register_t;
#endif

#include <sys/time.h>

#ifndef TIMEVAL_TO_TIMESPEC
#define TIMEVAL_TO_TIMESPEC(tv, ts)		\
do {						\
	(ts)->tv_sec  = (tv)->tv_sec;		\
	(ts)->tv_nsec = (tv)->tv_usec * 1000;	\
} while (/*CONSTCOND*/0)
#endif

#ifndef PLATFORM_HAS_SETGETPROGNAME
#define setprogname(a)
#endif

#endif /* _LIB_LIBRUMPUSER_RUMPUSER_PORT_H_ */
