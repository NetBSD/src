/*	$NetBSD: svr4_resource.c,v 1.1 1998/11/28 21:53:02 christos Exp $	 */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>

#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_resource.h>
#include <compat/svr4/svr4_signal.h>
#include <compat/svr4/svr4_syscallargs.h>
#include <compat/svr4/svr4_util.h>

static __inline int svr4_to_native_rl __P((int));

static __inline int
svr4_to_native_rl(rl)
	int rl;
{
	switch (rl) {
	case SVR4_RLIMIT_CPU:
		return RLIMIT_CPU;
	case SVR4_RLIMIT_FSIZE:
		return RLIMIT_FSIZE;
	case SVR4_RLIMIT_DATA:
		return RLIMIT_DATA;
	case SVR4_RLIMIT_STACK:
		return RLIMIT_STACK;
	case SVR4_RLIMIT_CORE:
		return RLIMIT_CORE;
	case SVR4_RLIMIT_NOFILE:
		return RLIMIT_NOFILE;
	case SVR4_RLIMIT_VMEM:
		return RLIMIT_RSS;
	default:
		return -1;
	}
}

#define OKLIMIT(l) ((l) > 0 && (l) < SVR4_RLIM_INFINITY && \
	((svr4_rlim_t)(l)) != SVR4_RLIM_INFINITY && \
	((svr4_rlim_t)(l)) != SVR4_RLIM_SAVED_CUR && \
	((svr4_rlim_t)(l)) != SVR4_RLIM_SAVED_MAX)

#define OKLIMIT64(l) ((l) > 0 && (l) < RLIM_INFINITY && \
	((svr4_rlim64_t)(l)) != SVR4_RLIM64_INFINITY && \
	((svr4_rlim64_t)(l)) != SVR4_RLIM64_SAVED_CUR && \
	((svr4_rlim64_t)(l)) != SVR4_RLIM64_SAVED_MAX)

int
svr4_sys_getrlimit(p, v, retval)
	register struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_getrlimit_args *uap = v;
	int rl = svr4_to_native_rl(SCARG(uap, which));
	struct rlimit *limp;
	struct svr4_rlimit lim;

	if (rl == -1)
		return EINVAL;

	limp = &p->p_rlimit[rl];

	/*
	 * If the limit can be be represented, it is returned.
	 * Otherwise, if rlim_cur == rlim_max, return RLIM_SAVED_MAX
	 * else return RLIM_SAVED_CUR
	 */
	if (limp->rlim_max == RLIM_INFINITY)
		lim.rlim_max = SVR4_RLIM_INFINITY;
	else if (OKLIMIT(limp->rlim_max))
		lim.rlim_max = (svr4_rlim_t) limp->rlim_max;
	else
		lim.rlim_max = SVR4_RLIM_SAVED_MAX;

	if (limp->rlim_cur == RLIM_INFINITY)
		lim.rlim_cur = SVR4_RLIM_INFINITY;
	else if (OKLIMIT(limp->rlim_cur))
		lim.rlim_cur = (svr4_rlim_t) limp->rlim_cur;
	else if (limp->rlim_max == limp->rlim_cur)
		lim.rlim_cur = SVR4_RLIM_SAVED_MAX;
	else
		lim.rlim_cur = SVR4_RLIM_SAVED_CUR;

	return copyout(&lim, SCARG(uap, rlp), sizeof(*SCARG(uap, rlp)));
}


int
svr4_sys_setrlimit(p, v, retval)
	register struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_setrlimit_args *uap = v;
	int rl = svr4_to_native_rl(SCARG(uap, which));
	struct rlimit alim, *limp;
	struct svr4_rlimit lim;
	int error;

	if (rl == -1)
		return EINVAL;

	limp = &p->p_rlimit[rl];

	if ((error = copyin(SCARG(uap, rlp), &lim, sizeof(lim))) != 0)
		return error;

	/*
	 * if the limit is SVR4_RLIM_INFINITY, then we set it to our
	 * unlimited.
	 * We should also: If it is SVR4_RLIM_SAVED_MAX, we should set the
	 * new limit to the corresponding saved hard limit, and if
	 * it is equal to SVR4_RLIM_SAVED_CUR, we should set it to the
	 * corresponding saved soft limit.
	 *
	 */
	if (lim.rlim_max == SVR4_RLIM64_INFINITY)
		alim.rlim_max = RLIM_INFINITY;
	else if (OKLIMIT(lim.rlim_max))
		alim.rlim_max = (rlim_t) lim.rlim_max;
	else if (lim.rlim_max == SVR4_RLIM_SAVED_MAX)
		alim.rlim_max = limp->rlim_max;
	else if (lim.rlim_max == SVR4_RLIM_SAVED_CUR)
		alim.rlim_max = limp->rlim_cur;

	if (lim.rlim_cur == SVR4_RLIM_INFINITY)
		alim.rlim_cur = RLIM_INFINITY;
	else if (OKLIMIT(lim.rlim_cur))
		alim.rlim_cur = (rlim_t) lim.rlim_cur;
	else if (lim.rlim_cur == SVR4_RLIM_SAVED_MAX)
		alim.rlim_cur = limp->rlim_max;
	else if (lim.rlim_cur == SVR4_RLIM_SAVED_CUR)
		alim.rlim_cur = limp->rlim_cur;

	*retval = 0;
	return dosetrlimit(p, rl, &alim);
}


int
svr4_sys_getrlimit64(p, v, retval)
	register struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_getrlimit64_args *uap = v;
	int rl = svr4_to_native_rl(SCARG(uap, which));
	struct rlimit *limp;
	struct svr4_rlimit64 lim;

	if (rl == -1)
		return EINVAL;

	limp = &p->p_rlimit[rl];

	/*
	 * If the limit can be be represented, it is returned.
	 * Otherwise, if rlim_cur == rlim_max, return SVR4_RLIM_SAVED_MAX
	 * else return SVR4_RLIM_SAVED_CUR
	 */
	if (limp->rlim_max == RLIM_INFINITY)
		lim.rlim_max = SVR4_RLIM64_INFINITY;
	else if (OKLIMIT64(limp->rlim_max))
		lim.rlim_max = (svr4_rlim64_t) limp->rlim_max;
	else
		lim.rlim_max = SVR4_RLIM64_SAVED_MAX;

	if (limp->rlim_cur == RLIM_INFINITY)
		lim.rlim_cur = SVR4_RLIM64_INFINITY;
	else if (OKLIMIT64(limp->rlim_cur))
		lim.rlim_cur = (svr4_rlim64_t) limp->rlim_cur;
	else if (limp->rlim_max == limp->rlim_cur)
		lim.rlim_cur = SVR4_RLIM64_SAVED_MAX;
	else
		lim.rlim_cur = SVR4_RLIM64_SAVED_CUR;

	return copyout(&lim, SCARG(uap, rlp), sizeof(*SCARG(uap, rlp)));
}


int
svr4_sys_setrlimit64(p, v, retval)
	register struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_setrlimit64_args *uap = v;
	int rl = svr4_to_native_rl(SCARG(uap, which));
	struct rlimit alim, *limp;
	struct svr4_rlimit64 lim;
	int error;

	if (rl == -1)
		return EINVAL;

	limp = &p->p_rlimit[rl];

	if ((error = copyin(SCARG(uap, rlp), &lim, sizeof(lim))) != 0)
		return error;

	/*
	 * if the limit is SVR4_RLIM64_INFINITY, then we set it to our
	 * unlimited.
	 * We should also: If it is SVR4_RLIM64_SAVED_MAX, we should set the
	 * new limit to the corresponding saved hard limit, and if
	 * it is equal to SVR4_RLIM64_SAVED_CUR, we should set it to the
	 * corresponding saved soft limit.
	 *
	 */
	if (lim.rlim_max == SVR4_RLIM_INFINITY)
		alim.rlim_max = RLIM_INFINITY;
	else if (OKLIMIT64(lim.rlim_max))
		alim.rlim_max = (rlim_t) lim.rlim_max;
	else if (lim.rlim_max == SVR4_RLIM64_SAVED_MAX)
		alim.rlim_max = limp->rlim_max;
	else if (lim.rlim_max == SVR4_RLIM64_SAVED_CUR)
		alim.rlim_max = limp->rlim_cur;

	if (lim.rlim_cur == SVR4_RLIM64_INFINITY)
		alim.rlim_cur = RLIM_INFINITY;
	else if (OKLIMIT64(lim.rlim_cur))
		alim.rlim_cur = (rlim_t) lim.rlim_cur;
	else if (lim.rlim_cur == SVR4_RLIM64_SAVED_MAX)
		alim.rlim_cur = limp->rlim_max;
	else if (lim.rlim_cur == SVR4_RLIM64_SAVED_CUR)
		alim.rlim_cur = limp->rlim_cur;

	*retval = 0;
	return dosetrlimit(p, rl, &alim);
}
