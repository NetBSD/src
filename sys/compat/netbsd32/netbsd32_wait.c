/*	$NetBSD: netbsd32_wait.c,v 1.6 2003/02/14 10:19:14 dsl Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_wait.c,v 1.6 2003/02/14 10:19:14 dsl Exp $");

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
	struct proc *parent = l->l_proc;
	struct netbsd32_rusage ru32;
	struct proc *child;
	int status, error;

	if (SCARG(uap, pid) == 0)
		SCARG(uap, pid) = -parent->p_pgid;
	if (SCARG(uap, options) &~ (WUNTRACED|WNOHANG))
		return (EINVAL);

	error =  find_stopped_child(parent, SCARG(uap,pid), SCARG(uap,options),
					&child);
	if (error != 0)
		return error;
	if (child == NULL) {
		retval[0] = 0;
		return 0;
	}

	retval[0] = child->p_pid;
	if (child->p_stat == SZOMB) {
		if (SCARG(uap, status)) {
			status = child->p_xstat;	/* convert to int */
			error = copyout((caddr_t)&status,
				    (caddr_t)NETBSD32PTR64(SCARG(uap, status)),
				    sizeof(status));
			if (error)
				return error;
		}
		if (SCARG(uap, rusage)) {
			netbsd32_from_rusage(child->p_ru, &ru32);
			error = copyout((caddr_t)&ru32,
				    (caddr_t)NETBSD32PTR64(SCARG(uap, rusage)),
				    sizeof(struct netbsd32_rusage));
			if (error)
				return error;
		}
		proc_free(child);
		return 0;
	}

	if (SCARG(uap, status)) {
		status = W_STOPCODE(child->p_xstat);
		return copyout((caddr_t)&status,
			    (caddr_t)NETBSD32PTR64(SCARG(uap, status)),
			    sizeof(status));
	}
	return 0;
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
		calcru(p, &rup->ru_utime, &rup->ru_stime, NULL);
		break;

	case RUSAGE_CHILDREN:
		rup = &p->p_stats->p_cru;
		break;

	default:
		return (EINVAL);
	}
	netbsd32_from_rusage(rup, &ru);
	return (copyout(&ru, (caddr_t)NETBSD32PTR64(SCARG(uap, rusage)),
	    sizeof(ru)));
}
