/*	$NetBSD: rwlock.h,v 1.2.26.1 2008/01/09 01:49:34 matt Exp $	*/

/*-
 * Copyright (c) 2002, 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe and Andrew Doran.
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

#ifndef _VAX_RWLOCK_H_
#define	_VAX_RWLOCK_H_

struct krwlock {
	volatile uintptr_t	rw_owner;
	__cpu_simple_lock_t	rw_lock;
	unsigned int	 	rw_dodebug : 24;
};

#ifdef __RWLOCK_PRIVATE

#define	RW_RECEIVE(rw)			/* nothing */
#define	RW_GIVE(rw)			/* nothing */

bool _rw_cas(krwlock_t *rw, uintptr_t old, uintptr_t new);

/*
 *	RW_ACQUIRE(rw, old, new)
 *		Perform an atomic "compare and swap" operation and
 *		evaluate to true or false according to the success
 *		of the operation, such that:
 *			if (rw->rw_owner == old) {
 *				rw->rw_owner = new;
 *				return 1;
 *			} else
 *				return 0;
 *		Must be MP/interrupt atomic.
 */
#define	RW_ACQUIRE(rw, old, new)	_rw_cas(rw, old, new)

/*
 *
 *	RW_RELEASE(rw, old, new)
 *		As above, but for releasing the lock.  Must be
 *		MP/interrupt atomic.
 */
#define	RW_RELEASE(rw, old, new)	_rw_cas(rw, old, new)

/*
 *	RW_SET_WAITERS(rw, need_wait, set_wait)
 *		Set the has-waiters indication.  If the needs-waiting
 *		condition becomes false, abort the operation.  Must
 *		be MP/interrupt atomic.
 */
static inline bool
RW_SET_WAITERS(krwlock_t *rw, uintptr_t need, uintptr_t set)
{
	uintptr_t old = rw->rw_owner;
	if (old & need)
		return _rw_cas(rw, old, old | set);
	return false;
}

/*
 *	RW_SETID(rw, id)
 *		Set the debugging ID for the lock, an integer.  Only
 *		used in the LOCKDEBUG case.
 */
static inline void
RW_SETDEBUG(krwlock_t *rw, bool dodebug)
{
	rw->rw_dodebug = dodebug;
}

/*
 *	RW_GETID(rw)
 *		Get the debugging ID for the lock, an integer.  Only
 *		used in the LOCKDEBUG case.
 */
static inline bool
RW_DEBUG_P(krwlock_t *rw)
{
	return rw->rw_dodebug;
}

#endif	/* __RWLOCK_PRIVATE */

#endif /* _VAX_RWLOCK_H_ */
