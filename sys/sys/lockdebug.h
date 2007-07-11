/*	$NetBSD: lockdebug.h,v 1.3.4.1 2007/07/11 20:12:32 mjf Exp $	*/

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

#define	LOCKDEBUG_ABORT(id, l, o, f, m)	lockdebug_abort(id, l, o, f, m)

void	lockdebug_abort(u_int, volatile void *, lockops_t *,
			const char *, const char *);

void	lockdebug_lock_print(void *, void (*)(const char *, ...));

#ifdef LOCKDEBUG

u_int	lockdebug_alloc(volatile void *, lockops_t *);
void	lockdebug_free(volatile void *, u_int);
void	lockdebug_wantlock(u_int, uintptr_t, int);
void	lockdebug_locked(u_int, uintptr_t, int);
void	lockdebug_unlocked(u_int, uintptr_t, int);
void	lockdebug_barrier(volatile void *, int);

#define	LOCKDEBUG_ALLOC(lock, ops)		lockdebug_alloc(lock, ops)
#define	LOCKDEBUG_FREE(lock, id)		lockdebug_free(lock, id)
#define	LOCKDEBUG_WANTLOCK(id, where, s)	lockdebug_wantlock(id, where, s)
#define	LOCKDEBUG_LOCKED(id, where, s)		lockdebug_locked(id, where, s)
#define	LOCKDEBUG_UNLOCKED(id, where, s)	lockdebug_unlocked(id, where, s)
#define	LOCKDEBUG_BARRIER(lk, slp)		lockdebug_barrier(lk, slp)

#else	/* LOCKDEBUG */

#define	LOCKDEBUG_ALLOC(lock, ops)		0
#define	LOCKDEBUG_FREE(lock, id)		/* nothing */
#define	LOCKDEBUG_WANTLOCK(id, where, s)	/* nothing */
#define	LOCKDEBUG_LOCKED(id, where, s)		/* nothing */
#define	LOCKDEBUG_UNLOCKED(id, where, s)	/* nothing */
#define	LOCKDEBUG_BARRIER(lk, slp)		/* nothing */

#endif	/* LOCKDEBUG */

#endif	/* __SYS_LOCKDEBUG_H__ */
