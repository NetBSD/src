/*	$NetBSD: mutex.h,v 1.7 2022/04/06 22:47:57 riastradh Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Takayoshi Kochi.
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

#ifndef _IA64_MUTEX_H_
#define	_IA64_MUTEX_H_

#ifndef __MUTEX_PRIVATE

struct kmutex {
	uintptr_t	mtx_pad1;
	uint32_t	mtx_pad2[2];
};

#else

struct kmutex {
	volatile uintptr_t	mtx_owner;
	ipl_cookie_t		mtx_ipl;
	__cpu_simple_lock_t	mtx_lock;
};

/* XXX when we implement mutex_enter()/mutex_exit(), uncomment this
#define __HAVE_MUTEX_STUBS		1
*/
/* XXX when we implement mutex_spin_enter()/mutex_spin_exit(), uncomment this
#define __HAVE_SPIN_MUTEX_STUBS		1
*/
#define	__HAVE_SIMPLE_MUTEXES		1

#endif	/* __MUTEX_PRIVATE */

#endif	/* _IA64_MUTEX_H_ */
