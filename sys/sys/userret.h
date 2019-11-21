/*	$NetBSD: userret.h,v 1.29 2019/11/21 19:24:02 ad Exp $	*/

/*-
 * Copyright (c) 1998, 2000, 2003, 2006, 2008, 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum, and Andrew Doran.
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

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef _SYS_USERRET_H_
#define	_SYS_USERRET_H_

#include <sys/lockdebug.h>
#include <sys/intr.h>
#include <sys/psref.h>

/*
 * Define the MI code needed before returning to user mode, for trap and
 * syscall.
 *
 * We handle "exceptional" events: pending signals, stop/exit actions, etc. 
 * Note that the event must be flagged BEFORE any AST is posted as we are
 * reading unlocked.
 */
static __inline void
mi_userret(struct lwp *l)
{
	struct cpu_info *ci;

	KPREEMPT_DISABLE(l);
	ci = l->l_cpu;
	KASSERT(l->l_blcnt == 0);
	KASSERT(ci->ci_biglock_count == 0);
	if (__predict_false(ci->ci_want_resched)) {
		preempt();
		ci = l->l_cpu;
	}
#ifdef __HAVE_FAST_SOFTINTS
	if (__predict_false(l->l_flag & LW_USERRET)) {
#else
	if (((l->l_flag & LW_USERRET) | ci->ci_data.cpu_softints) != 0) {
#endif
		KPREEMPT_ENABLE(l);
		lwp_userret(l);
		KPREEMPT_DISABLE(l);
		ci = l->l_cpu;
	}
	l->l_kpriority = false;
	ci->ci_schedstate.spc_curpriority = l->l_priority;
	KPREEMPT_ENABLE(l);

	LOCKDEBUG_BARRIER(NULL, 0);
	KASSERT(l->l_nopreempt == 0);
	PSREF_DEBUG_BARRIER();
	KASSERT(l->l_psrefs == 0);
}

#endif	/* !_SYS_USERRET_H_ */
