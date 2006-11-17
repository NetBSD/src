/*	$NetBSD: rwlockmi.h,v 1.1.2.1 2006/11/17 16:34:40 ad Exp $	*/

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

/*
 * Slow, bare bones lock constructs for architectures that do not provide
 * them.
 */

#ifndef _SYS_RWLOCKMI_H_
#define	_SYS_RWLOCKMI_H_

struct krwlock {
	volatile uintptr_t	rw_owner;
	uint32_t		rw_id;
};

#ifdef __RWLOCK_PRIVATE

#define	RW_ACQUIRE(rw, old, new)					\
	_rwlock_cas((uintptr_t *)(rw), (old), (new))
#define	RW_RELEASE(rw, old, new)					\
	_rwlock_cas((uintptr_t *)(rw), (old), (new))
#define	RW_SET_WAITERS(rw, need_wait, set_wait)				\
	_rwlock_set_waiters((uintptr_t *)(rw), (need_wait), (set_wait))
#define	RW_RECEIVE(rw)							\
	__insn_barrier()

#define	RW_SETID(rw, id)		((rw)->rw_id = id)
#define	RW_GETID(rw)			((rw)->rw_id)

static inline u_int
_rwlock_cas(uintptr_t *p, uintptr_t old, uintptr_t new)
{
	int s = splsched();
	if (*p != old) {
		splx(s);
		return 0;
	}
	*p = new;
	splx(s);
	return 1;
}

static inline u_int
_rwlock_set_waiters(uintptr_t *p, uintptr_t need, uintptr_t set)
{
	int s = splsched();
	if ((*p & need) == 0) {
		splx(s);
		return 0;
	}
	*p |= set;
	splx(s);
	return 1;
}

#endif	/* __RWLOCK_PRIVATE */

#endif /* _SYS_RWLOCKMI_H_ */
