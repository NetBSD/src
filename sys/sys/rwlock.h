/*	$NetBSD: rwlock.h,v 1.1.2.5 2002/03/22 03:26:59 thorpej Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	struct rwlock
 *		The actual rwlock structure.  This structure is mostly
 *		opaque to machine-independent code; most access are done
 *		through macros.  However, machine-independent code must
 *		be able to access the following member:
 *
 *			__volatile unsigned long rwl_owner
 *
 *		The rwl_owner field is laid out like so:
 *
 *		 N                   4 3    2        1       0
 *		+------------------------------------------------+
 *		| owner or read count | | wrlock | wrwant | wait |
 *		+------------------------------------------------+
 *
 *	RWLOCK_INITIALIZER
 *		Static initializer for rwlocks.
 *
 *	RWLOCK_INIT(rwl)
 *		Initialize the rwlock.
 *
 *	RWLOCK_ACQUIRE(rwl, old, new)
 *		Perform an atomic "compare and swap" operation and
 *		evaluate to true or false according to the success
 *		of the operation, such that:
 *			if (rwl->rwl_owner == old) {
 *				rwl->rwl_owner = new;
 *				return 1;
 *			} else
 *				return 0;
 *
 *	RWLOCK_RELEASE(rwl, old, new, actual)
 *		As above, but for releasing the lock.
 *
 *	RWLOCK_SET_WAITERS(rwl, need_wait, set_wait)
 *		Set the has-waiters indication.  If the needs-waiting
 *		condition becomes false, abort the operation.
 *
 *	RWLOCK_GIVE(rwl, new)
 *		Give the lock to the receiver (direct-handoff).
 *
 *	RWLOCK_RECEIVE(rwl)
 *		Receive the lock from the giver (direct-handoff).
 *
 *	RWLOCK_THREAD
 *		Mask of valid thread/count bits.
 */

#if defined(_KERNEL_OPT)
#include "opt_lockdebug.h"
#endif

struct rwlock_debug_info {
	vaddr_t		rwl_locked;	/* PC where mutex was locked */
	vaddr_t		rwl_unlocked;	/* PC where mutex was unlocked */
};      

typedef int krw_t;

#define	RW_WRITER		0
#define	RW_READER		1

typedef struct rwlock krwlock_t;

/*
 * Bits in the owner field of the lock that indicate various bits
 * of information.  If the WRITE_LOCKED bit is set, then the owner
 * field is actually a count of the number of readers.
 */
#define	RWLOCK_HAS_WAITERS	0x01	/* lock has waiters */
#define	RWLOCK_WRITE_WANTED	0x02	/* at least one waiter is a writer */
#define	RWLOCK_WRITE_LOCKED	0x04	/* lock is currently write locked */

#define	RWLOCK_READ_COUNT_SHIFT	4
#define	RWLOCK_READ_INCR	(1 << RWLOCK_READ_COUNT_SHIFT)

#define	RWLOCK_OWNER(rwl)						\
	((struct proc *)((rwl)->rwl_owner & RWLOCK_THREAD))

#define	RWLOCK_COUNT(rwl)						\
	((rwl)->rwl_owner & RWLOCK_THREAD)

#ifdef _KERNEL
void	rw_init(krwlock_t *);
void	rw_destroy(krwlock_t *);

void	rw_enter(krwlock_t *, krw_t);
int	rw_tryenter(krwlock_t *, krw_t);
void	rw_exit(krwlock_t *);

void	rw_downgrade(krwlock_t *);
int	rw_tryupgrade(krwlock_t *);

int	rw_read_held(krwlock_t *);
int	rw_write_held(krwlock_t *);

int	rw_read_locked(krwlock_t *);
int	rw_write_locked(krwlock_t *);

struct proc *rw_owner(krwlock_t *);
#endif /* _KERNEL */

#include <machine/rwlock_impl.h>

#endif /* _SYS_RWLOCK_H_ */
