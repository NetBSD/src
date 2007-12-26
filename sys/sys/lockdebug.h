/*	$NetBSD: lockdebug.h,v 1.6.2.1 2007/12/26 19:57:54 ad Exp $	*/

/*-
 * Copyright (c) 2006, 2007 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _KERNEL
#error "Sorry, nothing of interest to user level programs here."
#endif

typedef	struct lockops {
	const char	*lo_name;
	int		lo_sleeplock;
	void		(*lo_dump)(volatile void *);
} lockops_t;

#define	LOCKDEBUG_ABORT(l, o, f, m)	lockdebug_abort(l, o, f, m)

void	lockdebug_abort(volatile void *, lockops_t *,
			const char *, const char *);

void	lockdebug_lock_print(void *, void (*)(const char *, ...));

#ifdef LOCKDEBUG

bool	lockdebug_alloc(volatile void *, lockops_t *, uintptr_t);
void	lockdebug_free(volatile void *);
void	lockdebug_wantlock(volatile void *, uintptr_t, int);
void	lockdebug_locked(volatile void *, uintptr_t, int);
void	lockdebug_unlocked(volatile void *, uintptr_t, int);
void	lockdebug_barrier(volatile void *, int);
void	lockdebug_mem_check(const char *, void *, size_t);

#define	LOCKDEBUG_ALLOC(lock, ops, addr)	lockdebug_alloc(lock, ops, addr)
#define	LOCKDEBUG_FREE(dodebug, lock) \
    if (dodebug) lockdebug_free(lock)
#define	LOCKDEBUG_WANTLOCK(dodebug, lock, where, s) \
    if (dodebug) lockdebug_wantlock(lock, where, s)
#define	LOCKDEBUG_LOCKED(dodebug, lock, where, s) \
    if (dodebug) lockdebug_locked(lock, where, s)
#define	LOCKDEBUG_UNLOCKED(dodebug, lock, where, s) \
    if (dodebug) lockdebug_unlocked(lock, where, s)
#define	LOCKDEBUG_BARRIER(lock, slp)		lockdebug_barrier(lock, slp)
#define	LOCKDEBUG_MEM_CHECK(base, sz)	\
    lockdebug_mem_check(__func__, base, sz)

#else	/* LOCKDEBUG */

#define	LOCKDEBUG_ALLOC(lock, ops, addr)		false
#define	LOCKDEBUG_FREE(dodebug, lock)			/* nothing */
#define	LOCKDEBUG_WANTLOCK(dodebug, lock, where, s)	/* nothing */
#define	LOCKDEBUG_LOCKED(dodebug, lock, where, s)	/* nothing */
#define	LOCKDEBUG_UNLOCKED(dodebug, lock, where, s)	/* nothing */
#define	LOCKDEBUG_BARRIER(lock, slp)			/* nothing */
#define	LOCKDEBUG_MEM_CHECK(base, sz)			/* nothing */

#endif	/* LOCKDEBUG */

#endif	/* __SYS_LOCKDEBUG_H__ */
