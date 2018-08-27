/*	$NetBSD: rcupdate.h,v 1.4 2018/08/27 07:34:23 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef _LINUX_RCUPDATE_H_
#define _LINUX_RCUPDATE_H_

#include <sys/atomic.h>
#include <sys/null.h>
#include <sys/xcall.h>

#define	__rcu

#define	rcu_assign_pointer(P, V) do {					      \
	membar_producer();						      \
	(P) = (V);							      \
} while (0)


#define	rcu_dereference(P)	rcu_dereference_protected((P), 1)

#define	rcu_dereference_protected(P, C) ({				      \
	WARN_ON(!(C));							      \
	typeof(*(P)) *__rcu_dereference_protected_tmp = (P);		      \
	membar_datadep_consumer();					      \
	__rcu_dereference_protected_tmp;				      \
})

static inline void
rcu_read_lock(void)
{

	kpreempt_disable();
	__insn_barrier();
}

static inline void
rcu_read_unlock(void)
{

	__insn_barrier();
	kpreempt_enable();
}

static inline void
synchronize_rcu_xc(void *a, void *b)
{
}

static inline void
synchronize_rcu(void)
{
	xc_wait(xc_broadcast(0, &synchronize_rcu_xc, NULL, NULL));
}

#endif  /* _LINUX_RCUPDATE_H_ */
