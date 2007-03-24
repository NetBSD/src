/*	$NetBSD: netbsd32_wait.c,v 1.11.2.2 2007/03/24 14:55:16 yamt Exp $	*/

/*
 * Copyright (c) 1998, 2001 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_wait.c,v 1.11.2.2 2007/03/24 14:55:16 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/resourcevar.h>
#include <sys/vnode.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/dirent.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

int
netbsd32_wait4(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_wait4_args /* {
		syscallarg(int) pid;
		syscallarg(netbsd32_intp) status;
		syscallarg(int) options;
		syscallarg(netbsd32_rusagep_t) rusage;
	} */ *uap = v;
	struct sys_wait4_args ua;
	void *sg;
	struct rusage *ruup = NULL;
	int error;

	if (SCARG_P32(uap, rusage)) {
		sg = stackgap_init(l->l_proc, sizeof(*ruup));
		ruup = (struct rusage *)stackgap_alloc(l->l_proc, &sg,
		    sizeof(*ruup));
		if (ruup == NULL)
			return ENOMEM;
	}

	NETBSD32TO64_UAP(pid);
	NETBSD32TOP_UAP(status, int);
	NETBSD32TO64_UAP(options);
	SCARG(&ua, rusage) = ruup;

	error = sys_wait4(l, &ua, retval);
	if (error)
		return error;

	if (ruup != NULL) {
		struct netbsd32_rusage ru32;
		struct rusage rus;

		error = copyin(ruup, &rus, sizeof(rus));
		if (error)
			return error;
		netbsd32_from_rusage(&rus, &ru32);
		error = copyout(&ru32, SCARG_P32(uap, rusage), sizeof(ru32));
	}

	return error;
}

int
netbsd32_getrusage(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_getrusage_args /* {
		syscallarg(int) who;
		syscallarg(netbsd32_rusagep_t) rusage;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct rusage *rup;
	struct netbsd32_rusage ru;

	switch (SCARG(uap, who)) {

	case RUSAGE_SELF:
		rup = &p->p_stats->p_ru;
		mutex_enter(&p->p_smutex);
		calcru(p, &rup->ru_utime, &rup->ru_stime, NULL, NULL);
		mutex_exit(&p->p_smutex);
		break;

	case RUSAGE_CHILDREN:
		rup = &p->p_stats->p_cru;
		break;

	default:
		return (EINVAL);
	}
	netbsd32_from_rusage(rup, &ru);
	return copyout(&ru, SCARG_P32(uap, rusage), sizeof(ru));
}
