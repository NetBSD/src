/*	$NetBSD: netbsd32_rlimit.c,v 1.2 2021/01/19 03:20:13 simonb Exp $	*/

/*
 * Copyright (c) 1998, 2001, 2008, 2018 Matthew R. Green
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: NetBSD: netbsd32_netbsd.c,v 1.218 2018/08/10 21:44:58 pgoyette Exp
 */

/* rlimit netbsd32 related code */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_rlimit.c,v 1.2 2021/01/19 03:20:13 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/resource.h>
#include <sys/exec.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

#define LIMITCHECK(a, b) ((a) != RLIM_INFINITY && (a) > (b))

static void
fixlimit(int which, struct rlimit *alim)
{
	switch (which) {
	case RLIMIT_DATA:
		if (LIMITCHECK(alim->rlim_cur, MAXDSIZ32))
			alim->rlim_cur = MAXDSIZ32;
		if (LIMITCHECK(alim->rlim_max, MAXDSIZ32))
			alim->rlim_max = MAXDSIZ32;
		return;
	case RLIMIT_STACK:
		if (LIMITCHECK(alim->rlim_cur, MAXSSIZ32))
			alim->rlim_cur = MAXSSIZ32;
		if (LIMITCHECK(alim->rlim_max, MAXSSIZ32))
			alim->rlim_max = MAXSSIZ32;
		return;
	default:
		return;
	}
}

int
netbsd32_getrlimit(struct lwp *l, const struct netbsd32_getrlimit_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) which;
		syscallarg(netbsd32_rlimitp_t) rlp;
	} */
	int which = SCARG(uap, which);
	struct rlimit alim;

	if ((u_int)which >= RLIM_NLIMITS)
		return EINVAL;

	alim = l->l_proc->p_rlimit[which];

	fixlimit(which, &alim);

	return copyout(&alim, SCARG_P32(uap, rlp), sizeof(alim));
}

int
netbsd32_setrlimit(struct lwp *l, const struct netbsd32_setrlimit_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) which;
		syscallarg(const netbsd32_rlimitp_t) rlp;
	} */
		int which = SCARG(uap, which);
	struct rlimit alim;
	int error;

	if ((u_int)which >= RLIM_NLIMITS)
		return EINVAL;

	error = copyin(SCARG_P32(uap, rlp), &alim, sizeof(struct rlimit));
	if (error)
		return error;

	fixlimit(which, &alim);

	return dosetrlimit(l, l->l_proc, which, &alim);
}

void
netbsd32_adjust_limits(struct proc *p)
{
	static const struct {
		int id;
		rlim_t lim;
	} lm[] = {
		{ RLIMIT_DATA,	MAXDSIZ32 },
		{ RLIMIT_STACK, MAXSSIZ32 },
	};
	size_t i;
	struct plimit *lim;
	struct rlimit *rlim;

	/*
	 * We can only reduce the current limits, we cannot stop external
	 * processes from changing them (eg via sysctl) later on.
	 * So there is no point trying to lock out such changes here.
	 *
	 * If we assume that rlim_cur/max are accessed using atomic
	 * operations, we don't need to lock against any other updates
	 * that might happen if the plimit structure is shared writable
	 * between multiple processes.
	 */

	/* Scan to determine is any limits are out of range */
	lim = p->p_limit;
	for (i = 0; ; i++) {
		if (i >= __arraycount(lm))
			/* All in range */
			return;
		rlim = lim->pl_rlimit + lm[i].id;
		if (LIMITCHECK(rlim->rlim_cur, lm[i].lim))
			break;
		if (LIMITCHECK(rlim->rlim_max, lm[i].lim))
			break;
	}

	lim_privatise(p);

	lim = p->p_limit;
	for (i = 0; i < __arraycount(lm); i++) {
		rlim = lim->pl_rlimit + lm[i].id;
		if (LIMITCHECK(rlim->rlim_cur, lm[i].lim))
			rlim->rlim_cur = lm[i].lim;
		if (LIMITCHECK(rlim->rlim_max, lm[i].lim))
			rlim->rlim_max = lm[i].lim;
	}
}
