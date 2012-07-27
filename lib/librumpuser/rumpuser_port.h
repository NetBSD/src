/*	$NetBSD: rumpuser_port.h,v 1.1 2012/07/27 09:09:05 pooka Exp $	*/

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

#ifndef __arraycount
#define __arraycount(_ar_) (sizeof(_ar_)/sizeof(_ar_[0]))
#endif

#ifndef __UNCONST
#define __UNCONST(_a_) ((void *)(unsigned long)(const void *)(_a_))
#endif

#ifdef __linux__
#define arc4random() random()
#endif

#endif /* _LIB_LIBRUMPUSER_RUMPUSER_PORT_H_ */
