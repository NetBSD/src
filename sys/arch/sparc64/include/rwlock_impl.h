/*	$NetBSD: rwlock_impl.h,v 1.1.2.2 2002/03/22 03:33:31 thorpej Exp $	*/

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

#ifndef _SPARC64_RWLOCK_IMPL_H_
#define	_SPARC64_RWLOCK_IMPL_H_

struct rwlock {
	__volatile unsigned long rwl_owner;
#ifndef __arch64__
	unsigned long rwl_rsvd0;
#endif
#if defined(RWLOCK_DEBUG)
	struct rwlock_debug_info rwl_debug;
#endif
};

#define	RWLOCK_INITIALIZER						\
	{ .rwl_owner = 0 }

#ifdef __arch64__
#define	RWLOCK_THREAD		0xfffffffffffffff0UL
#else
#define	RWLOCK_THREAD		0xfffffff0UL
#endif

#define	RWLOCK_INIT(rwl)						\
do {									\
	(rwl)->rwl_owner = 0;						\
	__asm __volatile("membar #MemIssue" ::: "memory");		\
} while (/*CONSTCOND*/0)

#define	RWLOCK_ACQUIRE(rwl, old, new)					\
	_rwlock_cas((rwl), (old), (new))

#define	RWLOCK_RELEASE(rwl, old, new)					\
	_rwlock_cas((rwl), (old), (new))

#define	RWLOCK_SET_WAITERS(rwl, need_wait, set_wait)			\
	_rwlock_set_waiters((rwl), (need_wait), (set_wait))

#define	RWLOCK_GIVE(rwl, new)						\
do {									\
	__asm __volatile("membar #MemIssue" ::: "memory");		\
	(rwl)->rwl_owner = new;						\
} while (/*CONSTCOND*/0)

#define	RWLOCK_RECEIVE(rwl)						\
do {									\
	__asm __volatile("membar #MemIssue" ::: "memory");		\
} while (/*CONSTCOND*/0)

#ifdef _KERNEL
int	_rwlock_cas(krwlock_t *, unsigned long, unsigned long);
void	_rwlock_set_waiters(krwlock_t *, unsigned long, unsigned long);
#endif /* _KERNEL */

#endif /* _SPARC64_RWLOCK_IMPL_H_ */
