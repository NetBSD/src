/*	$NetBSD: userret.h,v 1.35 2024/01/28 10:06:19 skrll Exp $	*/

/*-
 * Copyright (c) 1998, 2000, 2003, 2006, 2008, 2019, 2020, 2023
 *     The NetBSD Foundation, Inc.
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
	int exception;

	KPREEMPT_DISABLE(l);
	KASSERTMSG(l->l_cpu->ci_biglock_count == 0, "kernel_lock leaked");
	KASSERT(l->l_blcnt == 0);
	exception = l->l_cpu->ci_want_resched | (l->l_flag & LW_USERRET);
	KPREEMPT_ENABLE(l);
	if (__predict_false(exception)) {
		lwp_userret(l);
	}

	LOCKDEBUG_BARRIER(NULL, 0);
	KASSERT(l->l_nopreempt == 0);
	PSREF_DEBUG_BARRIER();
	KASSERT(l->l_psrefs == 0);
}

#endif	/* !_SYS_USERRET_H_ */
