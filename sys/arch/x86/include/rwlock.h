/*	$NetBSD: rwlock.h,v 1.1.2.2 2006/10/20 19:28:11 ad Exp $	*/

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

#ifndef _X86_RWLOCK_H_
#define	_X86_RWLOCK_H_

struct krwlock {
	volatile uintptr_t	rw_owner;
	uint32_t		rw_id;
};

#ifdef __RWLOCK_PRIVATE

#define	__HAVE_RW_ENTER			1
#define	__HAVE_RW_EXIT			1

#define	RW_ACQUIRE(rw, old, new)					\
	_lock_cas((uintptr_t *)(rw), (old), (new))
#define	RW_RELEASE(rw, old, new)					\
	_lock_cas((uintptr_t *)(rw), (old), (new))
#define	RW_SET_WAITERS(rw, need_wait, set_wait)				\
	_lock_set_waiters((uintptr_t *)(rw), (need_wait), (set_wait))
#define	RW_RECEIVE(rw)							\
	__insn_barrier()

#define	RW_SETID(rw, id)		((rw)->rw_id = id)
#define	RW_GETID(rw)			((rw)->rw_id)

#ifdef _KERNEL
int	_lock_cas(volatile uintptr_t *, uintptr_t, uintptr_t);
int	_lock_set_waiters(volatile uintptr_t *, uintptr_t, uintptr_t);
#endif /* _KERNEL */

#endif	/* __RWLOCK_PRIVATE */

#endif /* _X86_RWLOCK_H_ */
