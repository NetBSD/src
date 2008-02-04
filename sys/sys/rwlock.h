/*	$NetBSD: rwlock.h,v 1.1.18.3 2008/02/04 09:25:00 yamt Exp $	*/

/*-
 * Copyright (c) 2002, 2006, 2007, 2008 The NetBSD Foundation, Inc.
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

#ifndef _SYS_RWLOCK_H_
#define	_SYS_RWLOCK_H_

/*
 * The rwlock provides exclusive access when held as a "writer",
 * and shared access when held as a "reader".
 *
 * Machine dependent code must provide the following:
 *
 *	struct krwlock
 *		The actual rwlock structure.  This structure is mostly
 *		opaque to machine-independent code; most access are done
 *		through macros.  However, machine-independent code must
 *		be able to access the following member:
 *
 *			volatile uintptr_t	rw_owner
 *
 *		The rw_owner field is laid out like so:
 *
 *		 N                    4       3        2        1      0
 *		+-------------------------------------------------------+
 *		| owner or read count | unused | wrlock | wrwant | wait |
 *		+-------------------------------------------------------+
 *
 *	RW_RECEIVE(rw)
 *		Receive the lock from the giver (direct-handoff).  This
 *		is the place to do a load fence.
 *
 *	RW_GIVE(rw)
 *		This is the place to do a load/store fence before the
 *		lock is released.
 *
 *
 * If an architecture can be considered 'simple' (no interlock required in
 * the MP case, or no MP) it need only define __HAVE_SIMPLE_RW_LOCKS and
 * provide the following:
 *
 * 	RW_CAS(ptr, old, new)
 *		Perform an atomic "compare and swap" operation and
 *		evaluate to true or false according to the success
 *
 * Otherwise, the following must be defined:
 *
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
 *
 *	RW_RELEASE(rw, old, new)
 *		As above, but for releasing the lock.  Must be
 *		MP/interrupt atomic.
 *
 *	RW_SET_WAITERS(rw, need_wait, set_wait)
 *		Set the has-waiters indication.  If the needs-waiting
 *		condition becomes false, abort the operation.  Must
 *		be MP/interrupt atomic.
 *
 *	RW_SETDEBUG(rw, dodebug)
 *		Set the debugging boolean flag.  The flag is only used in
 *		the LOCKDEBUG case.  Used by rw_init to initialize a lock.
 *
 *	RW_DEBUG_P(rw)
 *		Evaluate to true if the lock is initialized by RW_SETDEBUG
 *		with dodebug==true.  Only used in the LOCKDEBUG case.
 *
 * Architectures may optionally provide stubs for the following functions to
 * implement the easy (unlocked, no waiters) cases.  If these stubs are
 * provided, __HAVE_RW_STUBS should be defined.
 *
 *	rw_enter()
 *	rw_exit()
 *	rw_tryenter()
 */

#if defined(_KERNEL_OPT)
#include "opt_lockdebug.h"
#endif

#if !defined(_KERNEL)
#include <sys/types.h>
#include <sys/inttypes.h>
#endif

typedef enum krw_t {
	RW_READER = 0,
	RW_WRITER = 1
} krw_t;

typedef struct krwlock krwlock_t;

#ifdef __RWLOCK_PRIVATE
/*
 * Bits in the owner field of the lock that indicate lock state.  If the
 * WRITE_LOCKED bit is clear, then the owner field is actually a count of
 * the number of readers.
 */
#define	RW_HAS_WAITERS		0x01UL	/* lock has waiters */
#define	RW_WRITE_WANTED		0x02UL	/* >= 1 waiter is a writer */
#define	RW_WRITE_LOCKED		0x04UL	/* lock is currently write locked */
#define	RW_DEBUG		0x08UL	/* LOCKDEBUG enabled */

#define	RW_READ_COUNT_SHIFT	4
#define	RW_READ_INCR		(1UL << RW_READ_COUNT_SHIFT)
#define	RW_THREAD		((uintptr_t)-RW_READ_INCR)
#define	RW_OWNER(rw)		((rw)->rw_owner & RW_THREAD)
#define	RW_COUNT(rw)		((rw)->rw_owner & RW_THREAD)
#define	RW_FLAGS(rw)		((rw)->rw_owner & ~RW_THREAD)

void	rw_vector_enter(krwlock_t *, const krw_t);
void	rw_vector_exit(krwlock_t *);
int	rw_vector_tryenter(krwlock_t *, const krw_t);
#endif	/* __RWLOCK_PRIVATE */

#include <machine/rwlock.h>

#ifdef _KERNEL

void	rw_init(krwlock_t *);
void	rw_destroy(krwlock_t *);

int	rw_tryenter(krwlock_t *, const krw_t);
int	rw_tryupgrade(krwlock_t *);
void	rw_downgrade(krwlock_t *);

int	rw_read_held(krwlock_t *);
int	rw_write_held(krwlock_t *);
int	rw_lock_held(krwlock_t *);

void	rw_enter(krwlock_t *, const krw_t);
void	rw_exit(krwlock_t *);

#endif	/* _KERNEL */

#endif /* _SYS_RWLOCK_H_ */

