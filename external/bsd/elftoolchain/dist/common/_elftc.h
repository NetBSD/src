/*	$NetBSD: _elftc.h,v 1.9 2022/05/03 16:47:15 jkoshy Exp $	*/

/*-
 * Copyright (c) 2009 Joseph Koshy
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Id: _elftc.h 3975 2022-04-30 20:10:58Z jkoshy
 */

/**
 ** Miscellaneous definitions needed by multiple components.
 **/

#ifndef	_ELFTC_H
#define	_ELFTC_H

#ifndef	NULL
#define NULL 	((void *) 0)
#endif

#ifndef	offsetof
#define	offsetof(T, M)		((int) &((T*) 0) -> M)
#endif

/* --QUEUE-MACROS-- [[ */

/*
 * Supply macros missing from <sys/queue.h>
 */

/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef	LIST_FOREACH_SAFE
#define	LIST_FOREACH_SAFE(var, head, field, tvar)		\
	for ((var) = LIST_FIRST((head));			\
	    (var) && ((tvar) = LIST_NEXT((var), field), 1);	\
	    (var) = (tvar))
#endif

#ifndef	SLIST_FOREACH_SAFE
#define	SLIST_FOREACH_SAFE(var, head, field, tvar)		\
	for ((var) = SLIST_FIRST((head));			\
	    (var) && ((tvar) = SLIST_NEXT((var), field), 1);	\
	    (var) = (tvar))
#endif

#ifndef	STAILQ_CONCAT
#define	STAILQ_CONCAT(head1, head2) do {			\
	if (!STAILQ_EMPTY((head2))) {				\
		*(head1)->stqh_last = (head2)->stqh_first;	\
		(head1)->stqh_last = (head2)->stqh_last;	\
		STAILQ_INIT((head2));				\
	}							\
} while (/*CONSTCOND*/0)
#endif

#ifndef	STAILQ_EMPTY
#define	STAILQ_EMPTY(head)	((head)->stqh_first == NULL)
#endif

#ifndef	STAILQ_ENTRY
#define	STAILQ_ENTRY(type)					\
struct {							\
	struct type *stqe_next;	/* next element */		\
}
#endif

#ifndef	STAILQ_FIRST
#define	STAILQ_FIRST(head)	((head)->stqh_first)
#endif

#ifndef	STAILQ_HEAD
#define	STAILQ_HEAD(name, type)					\
struct name {							\
	struct type *stqh_first; /* first element */		\
	struct type **stqh_last; /* addr of last next element */ \
}
#endif

#ifndef	STAILQ_HEAD_INITIALIZER
#define	STAILQ_HEAD_INITIALIZER(head)				\
	{ NULL, &(head).stqh_first }
#endif

#ifndef	STAILQ_FOREACH
#define	STAILQ_FOREACH(var, head, field)			\
	for ((var) = ((head)->stqh_first);			\
		(var);						\
		(var) = ((var)->field.stqe_next))
#endif

#ifndef	STAILQ_FOREACH_SAFE
#define STAILQ_FOREACH_SAFE(var, head, field, tvar)		\
       for ((var) = STAILQ_FIRST((head));			\
	    (var) && ((tvar) = STAILQ_NEXT((var), field), 1);	\
	    (var) = (tvar))
#endif

#ifndef	STAILQ_INIT
#define	STAILQ_INIT(head) do {					\
	(head)->stqh_first = NULL;				\
	(head)->stqh_last = &(head)->stqh_first;		\
} while (/*CONSTCOND*/0)
#endif

#ifndef	STAILQ_INSERT_HEAD
#define	STAILQ_INSERT_HEAD(head, elm, field) do {			\
	if (((elm)->field.stqe_next = (head)->stqh_first) == NULL)	\
		(head)->stqh_last = &(elm)->field.stqe_next;		\
	(head)->stqh_first = (elm);					\
} while (/*CONSTCOND*/0)
#endif

#ifndef	STAILQ_INSERT_TAIL
#define	STAILQ_INSERT_TAIL(head, elm, field) do {			\
	(elm)->field.stqe_next = NULL;					\
	*(head)->stqh_last = (elm);					\
	(head)->stqh_last = &(elm)->field.stqe_next;			\
} while (/*CONSTCOND*/0)
#endif

#ifndef	STAILQ_INSERT_AFTER
#define	STAILQ_INSERT_AFTER(head, listelm, elm, field) do {		\
	if (((elm)->field.stqe_next = (listelm)->field.stqe_next) == NULL)\
		(head)->stqh_last = &(elm)->field.stqe_next;		\
	(listelm)->field.stqe_next = (elm);				\
} while (/*CONSTCOND*/0)
#endif

#ifndef	STAILQ_LAST
#define STAILQ_LAST(head, type, field)					\
	(STAILQ_EMPTY((head)) ?					\
	    NULL : ((struct type *)(void *)				\
	    ((char *)((head)->stqh_last) - offsetof(struct type, field))))
#endif

#ifndef	STAILQ_NEXT
#define	STAILQ_NEXT(elm, field)	((elm)->field.stqe_next)
#endif

#ifndef	STAILQ_REMOVE
#define	STAILQ_REMOVE(head, elm, type, field) do {			\
	if ((head)->stqh_first == (elm)) {				\
		STAILQ_REMOVE_HEAD((head), field);			\
	} else {							\
		struct type *curelm = (head)->stqh_first;		\
		while (curelm->field.stqe_next != (elm))		\
			curelm = curelm->field.stqe_next;		\
		if ((curelm->field.stqe_next =				\
			curelm->field.stqe_next->field.stqe_next) == NULL) \
			    (head)->stqh_last = &(curelm)->field.stqe_next; \
	}								\
} while (/*CONSTCOND*/0)
#endif

#ifndef	STAILQ_REMOVE_HEAD
#define	STAILQ_REMOVE_HEAD(head, field) do {				\
	if (((head)->stqh_first = (head)->stqh_first->field.stqe_next) == \
	    NULL)							\
		(head)->stqh_last = &(head)->stqh_first;		\
} while (/*CONSTCOND*/0)
#endif

/*
 * The STAILQ_SORT macro is adapted from Simon Tatham's O(n*log(n))
 * mergesort algorithm.
 */
#ifndef	STAILQ_SORT
#define	STAILQ_SORT(head, type, field, cmp) do {			\
	STAILQ_HEAD(, type) _la, _lb;					\
	struct type *_p, *_q, *_e;					\
	int _i, _sz, _nmerges, _psz, _qsz;				\
									\
	_sz = 1;							\
	do {								\
		_nmerges = 0;						\
		STAILQ_INIT(&_lb);					\
		while (!STAILQ_EMPTY((head))) {				\
			_nmerges++;					\
			STAILQ_INIT(&_la);				\
			_psz = 0;					\
			for (_i = 0; _i < _sz && !STAILQ_EMPTY((head));	\
			     _i++) {					\
				_e = STAILQ_FIRST((head));		\
				if (_e == NULL)				\
					break;				\
				_psz++;					\
				STAILQ_REMOVE_HEAD((head), field);	\
				STAILQ_INSERT_TAIL(&_la, _e, field);	\
			}						\
			_p = STAILQ_FIRST(&_la);			\
			_qsz = _sz;					\
			_q = STAILQ_FIRST((head));			\
			while (_psz > 0 || (_qsz > 0 && _q != NULL)) {	\
				if (_psz == 0) {			\
					_e = _q;			\
					_q = STAILQ_NEXT(_q, field);	\
					STAILQ_REMOVE_HEAD((head),	\
					    field);			\
					_qsz--;				\
				} else if (_qsz == 0 || _q == NULL) {	\
					_e = _p;			\
					_p = STAILQ_NEXT(_p, field);	\
					STAILQ_REMOVE_HEAD(&_la, field);\
					_psz--;				\
				} else if (cmp(_p, _q) <= 0) {		\
					_e = _p;			\
					_p = STAILQ_NEXT(_p, field);	\
					STAILQ_REMOVE_HEAD(&_la, field);\
					_psz--;				\
				} else {				\
					_e = _q;			\
					_q = STAILQ_NEXT(_q, field);	\
					STAILQ_REMOVE_HEAD((head),	\
					    field);			\
					_qsz--;				\
				}					\
				STAILQ_INSERT_TAIL(&_lb, _e, field);	\
			}						\
		}							\
		(head)->stqh_first = _lb.stqh_first;			\
		(head)->stqh_last = _lb.stqh_last;			\
		_sz *= 2;						\
	} while (_nmerges > 1);						\
} while (/*CONSTCOND*/0)
#endif

#ifndef	TAILQ_FOREACH_SAFE
#define TAILQ_FOREACH_SAFE(var, head, field, tvar)                      \
	for ((var) = TAILQ_FIRST((head));                               \
	    (var) && ((tvar) = TAILQ_NEXT((var), field), 1);            \
	    (var) = (tvar))
#endif

/* ]] --QUEUE-MACROS-- */

/*
 * VCS Ids.
 *
 * The place holder below is intended to be replaced with a project-specific
 * definition of the ELFTC_VCSID macro.
 */

#ifndef	ELFTC_VCSID
#define	ELFTC_VCSID(ID)		/**/
#endif

#ifndef	ELFTC_VCSID

#if defined(__DragonFly__) || defined(__NetBSD__)

#define	ELFTC_VCSID(ID)		__RCSID(ID)

#elif defined(__FreeBSD__)

#define	ELFTC_VCSID(ID)		__FBSDID(ID)

#elif defined(__APPLE__) || defined(__OpenBSD__) || defined(__GLIBC__) || \
    defined(__GNU__) || defined(__linux__) || defined(__minix)

#if defined(__GNUC__)
#define	ELFTC_VCSID(ID)		__asm__(".ident\t\"" ID "\"")
#else
#define	ELFTC_VCSID(ID)		/**/
#endif

#endif

#endif	/* ELFTC_VCSID */

/*
 * The place holder below is meant to be replaced by a declaration
 * of the downstream project's revision control macro.
 *
 * E.g. on NetBSD, this place holder would be replaced by:
 *
 *   #if !defined(__RCSID)
 *   #define __RCSID(ID)
 *   #endif
 */
#if !defined(__RCSID)
#define __RCSID(ID) /**/
#endif  /* !defined(__RCSID) */

/*
 * Provide an equivalent for getprogname(3).
 */

#ifndef	ELFTC_GETPROGNAME

#if defined(__APPLE__) || defined(__DragonFly__) || defined(__FreeBSD__) || \
    defined(__minix) || defined(__NetBSD__)

#include <stdlib.h>

#define	ELFTC_GETPROGNAME()	getprogname()

#endif	/* __APPLE__ || __DragonFly__ || __FreeBSD__ || __minix || __NetBSD__ */


#if defined(__GLIBC__) || defined(__linux__)
#ifndef _GNU_SOURCE
/*
 * GLIBC based systems have a global 'char *' pointer referencing
 * the executable's name.
 */
extern const char *program_invocation_short_name;
#endif	/* !_GNU_SOURCE */

#define	ELFTC_GETPROGNAME()	program_invocation_short_name

#endif	/* __GLIBC__ || __linux__ */


#if defined(__OpenBSD__)

extern const char *__progname;

#define	ELFTC_GETPROGNAME()	__progname

#endif	/* __OpenBSD__ */

#endif	/* ELFTC_GETPROGNAME */


/*
 * Per-OS configuration.
 *
 * The following symbols are supported by this configuration fragment,
 * although not all the OSes so referenced are fully supported.
 *
 * Cross-compilation:
 *
 * HAVE_NBTOOL_CONFIG_H : cross-compiling NetBSD tools on various OSes.
 *
 * Native compilation:
 *
 * __APPLE__     : compiling under Mac OS X.
 * __DragonFly__ : compiling under DragonFlyBSD.
 * __GLIBC__     : compiling under GNU based systems, such as GNU/kFreeBSD.
 * __linux__     : compiling under GNU/Linux systems.
 * __FreeBSD__   : compiling under FreeBSD.
 * __minix       : compiling under Minix3.
 * __NetBSD__    : compiling (native) under NetBSD.
 * __OpenBSD__   : compiling under OpenBSD.
 */

#if defined(HAVE_NBTOOL_CONFIG_H)

#include <sys/param.h>
#include <sys/endian.h>

#ifndef	roundup2
#define	roundup2	roundup
#endif

#define	ELFTC_BYTE_ORDER			_BYTE_ORDER
#define	ELFTC_BYTE_ORDER_LITTLE_ENDIAN		_LITTLE_ENDIAN
#define	ELFTC_BYTE_ORDER_BIG_ENDIAN		_BIG_ENDIAN

#define	ELFTC_HAVE_MMAP				1
#define	ELFTC_HAVE_STRMODE			1

#elif defined(__APPLE__)

#include <libkern/OSByteOrder.h>
#define	htobe32(x)	OSSwapHostToBigInt32(x)
#define	roundup2	roundup

#define	ELFTC_BYTE_ORDER			_BYTE_ORDER
#define	ELFTC_BYTE_ORDER_LITTLE_ENDIAN		_LITTLE_ENDIAN
#define	ELFTC_BYTE_ORDER_BIG_ENDIAN		_BIG_ENDIAN

#define	ELFTC_HAVE_MMAP				1
#define	ELFTC_HAVE_STRMODE			1

#define ELFTC_NEED_BYTEORDER_EXTENSIONS		1

#elif defined(__DragonFly__)

#include <osreldate.h>
#include <sys/endian.h>

#define	ELFTC_BYTE_ORDER			_BYTE_ORDER
#define	ELFTC_BYTE_ORDER_LITTLE_ENDIAN		_LITTLE_ENDIAN
#define	ELFTC_BYTE_ORDER_BIG_ENDIAN		_BIG_ENDIAN

#define	ELFTC_HAVE_MMAP				1

#elif defined(__GLIBC__) || defined(__linux__)

#include <endian.h>

#define	ELFTC_BYTE_ORDER			__BYTE_ORDER
#define	ELFTC_BYTE_ORDER_LITTLE_ENDIAN		__LITTLE_ENDIAN
#define	ELFTC_BYTE_ORDER_BIG_ENDIAN		__BIG_ENDIAN

#define	ELFTC_HAVE_MMAP				1

/*
 * Debian GNU/Linux and Debian GNU/kFreeBSD do not have strmode(3).
 */
#define	ELFTC_HAVE_STRMODE			0

/* Whether we need to supply {be,le}32dec. */
#define ELFTC_NEED_BYTEORDER_EXTENSIONS		1

#define	roundup2	roundup

#elif defined(__FreeBSD__)

#include <osreldate.h>
#include <sys/endian.h>

#define	ELFTC_BYTE_ORDER			_BYTE_ORDER
#define	ELFTC_BYTE_ORDER_LITTLE_ENDIAN		_LITTLE_ENDIAN
#define	ELFTC_BYTE_ORDER_BIG_ENDIAN		_BIG_ENDIAN

#define	ELFTC_HAVE_MMAP				1
#define	ELFTC_HAVE_STRMODE			1
#if __FreeBSD_version <= 900000
#define	ELFTC_BROKEN_YY_NO_INPUT		1
#endif

#elif defined(__minix)
#define	ELFTC_HAVE_MMAP				0

#elif defined(__NetBSD__)

#include <sys/param.h>
#include <sys/endian.h>

#define	ELFTC_BYTE_ORDER			_BYTE_ORDER
#define	ELFTC_BYTE_ORDER_LITTLE_ENDIAN		_LITTLE_ENDIAN
#define	ELFTC_BYTE_ORDER_BIG_ENDIAN		_BIG_ENDIAN

#define	ELFTC_HAVE_MMAP				1
#define	ELFTC_HAVE_STRMODE			1
#if __NetBSD_Version__ <= 599002100
/* from src/doc/CHANGES: flex(1): Import flex-2.5.35 [christos 20091025] */
/* and 5.99.21 was from Wed Oct 21 21:28:36 2009 UTC */
#  define ELFTC_BROKEN_YY_NO_INPUT		1
#endif

#elif defined(__OpenBSD__)

#include <sys/param.h>
#include <sys/endian.h>

#define	ELFTC_BYTE_ORDER			_BYTE_ORDER
#define	ELFTC_BYTE_ORDER_LITTLE_ENDIAN		_LITTLE_ENDIAN
#define	ELFTC_BYTE_ORDER_BIG_ENDIAN		_BIG_ENDIAN

#define	ELFTC_HAVE_MMAP				1
#define	ELFTC_HAVE_STRMODE			1

#define	ELFTC_NEED_BYTEORDER_EXTENSIONS		1
#define	roundup2	roundup

#endif	/* __OpenBSD__ */

#endif	/* _ELFTC_H */
