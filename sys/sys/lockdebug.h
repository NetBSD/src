/*	$NetBSD: lockdebug.h,v 1.13.8.2 2017/12/03 11:39:20 jdolecek Exp $	*/

/*-
 * Copyright (c) 2006, 2007, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

#ifndef __SYS_LOCKDEBUG_H__
#define	__SYS_LOCKDEBUG_H__

#ifdef _KERNEL_OPT
#include "opt_lockdebug.h"
#endif

#if !defined(_KERNEL) && !defined(_KMEMUSER)
#error "Sorry, nothing of interest to user level programs here."
#endif

#define	LOCKOPS_SLEEP	0
#define	LOCKOPS_SPIN	1
#define	LOCKOPS_CV	2

typedef	struct lockops {
	const char	*lo_name;
	int		lo_type;
	void		(*lo_dump)(const volatile void *);
} lockops_t;

#define	LOCKDEBUG_ABORT(f, ln, l, o, m) \
    lockdebug_abort(f, ln, l, o, m)

void	lockdebug_abort(const char *, size_t, const volatile void *,
    lockops_t *, const char *);

void	lockdebug_lock_print(void *, void (*)(const char *, ...)
    __printflike(1, 2));

#ifdef LOCKDEBUG

bool	lockdebug_alloc(const char *, size_t, volatile void *, lockops_t *,
    uintptr_t);
void	lockdebug_free(const char *, size_t, volatile void *);
void	lockdebug_wantlock(const char *, size_t, const volatile void *,
    uintptr_t, int);
void	lockdebug_locked(const char *, size_t, volatile void *, void *,
    uintptr_t, int);
void	lockdebug_unlocked(const char *, size_t, volatile void *,
    uintptr_t, int);
void	lockdebug_barrier(const char *, size_t, volatile void *, int);
void	lockdebug_mem_check(const char *, size_t, void *, size_t);
void	lockdebug_wakeup(const char *, size_t, volatile void *, uintptr_t);

#define	LOCKDEBUG_ALLOC(lock, ops, addr) \
    lockdebug_alloc(__func__, __LINE__, lock, ops, addr)
#define	LOCKDEBUG_FREE(dodebug, lock) \
    if (dodebug) lockdebug_free(__func__, __LINE__, lock)
#define	LOCKDEBUG_WANTLOCK(dodebug, lock, where, s) \
    if (dodebug) lockdebug_wantlock(__func__, __LINE__, lock, where, s)
#define	LOCKDEBUG_LOCKED(dodebug, lock, al, where, s) \
    if (dodebug) lockdebug_locked(__func__, __LINE__, lock, al, where, s)
#define	LOCKDEBUG_UNLOCKED(dodebug, lock, where, s) \
    if (dodebug) lockdebug_unlocked(__func__, __LINE__, lock, where, s)
#define	LOCKDEBUG_BARRIER(lock, slp) \
    lockdebug_barrier(__func__, __LINE__, lock, slp)
#define	LOCKDEBUG_MEM_CHECK(base, sz)	\
    lockdebug_mem_check(__func__, __LINE__, base, sz)
#define	LOCKDEBUG_WAKEUP(dodebug, lock, where)	\
    if (dodebug) lockdebug_wakeup(__func__, __LINE__, lock, where)

#else	/* LOCKDEBUG */

#define	LOCKDEBUG_ALLOC(lock, ops, addr)		false
#define	LOCKDEBUG_FREE(dodebug, lock)			/* nothing */
#define	LOCKDEBUG_WANTLOCK(dodebug, lock, where, s)	/* nothing */
#define	LOCKDEBUG_LOCKED(dodebug, lock, al, where, s)	/* nothing */
#define	LOCKDEBUG_UNLOCKED(dodebug, lock, where, s)	/* nothing */
#define	LOCKDEBUG_BARRIER(lock, slp)			/* nothing */
#define	LOCKDEBUG_MEM_CHECK(base, sz)			/* nothing */
#define	LOCKDEBUG_WAKEUP(dodebug, lock, where)		/* nothing */

#endif	/* LOCKDEBUG */

#endif	/* __SYS_LOCKDEBUG_H__ */
