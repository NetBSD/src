/*	$NetBSD: smp.h,v 1.4.4.1 2024/10/04 11:40:49 martin Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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

#ifndef	_LINUX_SMP_H_
#define	_LINUX_SMP_H_

#include <sys/cpu.h>
#include <sys/systm.h>
#include <sys/xcall.h>

#define	smp_processor_id()	cpu_number()

static inline int
get_cpu(void)
{

	kpreempt_disable();
	return cpu_index(curcpu());
}

static inline void
put_cpu(void)
{

	kpreempt_enable();
}

static inline void
on_each_cpu_xc(void *a, void *b)
{
	void (**fp)(void *) = a;
	void *cookie = b;

	(**fp)(cookie);
}

static inline void
on_each_cpu(void (*f)(void *), void *cookie, int wait)
{
	uint64_t ticket;

	ticket = xc_broadcast(0, &on_each_cpu_xc, &f, cookie);
	if (wait)
		xc_wait(ticket);
}

#endif	/* _LINUX_SMP_H_ */
