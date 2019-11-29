/*	$NetBSD: mutex.h,v 1.5 2019/11/29 20:06:34 riastradh Exp $	*/

/*-
 * Copyright (c) 2002, 2007 The NetBSD Foundation, Inc.
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

#ifndef _SPARC64_MUTEX_H_
#define	_SPARC64_MUTEX_H_

struct kmutex {
	union {
		volatile uintptr_t	mtxa_owner;
#ifdef __MUTEX_PRIVATE
		struct {
			uint8_t			mtxs_unused;
                        __cpu_simple_lock_t	mtxs_lock;
			ipl_cookie_t		mtxs_ipl;
			uint8_t			mtxs_dummy;
		} s;
#endif
	} u;
};

#ifdef __MUTEX_PRIVATE

#define	mtx_owner 			u.mtxa_owner
#define	mtx_ipl 			u.s.mtxs_ipl
#define	mtx_lock			u.s.mtxs_lock

#define __HAVE_MUTEX_STUBS		1
#define	__HAVE_SIMPLE_MUTEXES		1

/*
 * XXX Should this be LoadLoad|LoadStore, or does the assumption of a
 * preceding atomic r/m/w operation obviate the need for that?
 */
#define	MUTEX_RECEIVE(mtx) __asm __volatile("membar #LoadLoad" : : : "memory")

/*
 * MUTEX_GIVE: no memory barrier required, as _lock_cas() will take care of it.
 */
#define	MUTEX_GIVE(mtx)			__insn_barrier()

#define	MUTEX_CAS(p, o, n)		_lock_cas((p), (o), (n))

int	_lock_cas(volatile uintptr_t *, uintptr_t, uintptr_t);

#endif	/* __MUTEX_PRIVATE */

#endif /* _SPARC64_MUTEX_H_ */
