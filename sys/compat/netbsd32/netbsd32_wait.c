/*	$NetBSD: netbsd32_wait.c,v 1.1 2001/02/08 13:19:34 mrg Exp $	*/

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
netbsd32_wait4(q, v, retval)
	struct proc *q;
	void *v;
	register_t *retval;
{
	struct netbsd32_wait4_args /* {
		syscallarg(int) pid;
		syscallarg(netbsd32_intp) status;
		syscallarg(int) options;
		syscallarg(netbsd32_rusagep_t) rusage;
	} */ *uap = v;
	struct netbsd32_rusage ru32;
	int nfound;
	struct proc *p, *t;
	int status, error;

	if (SCARG(uap, pid) == 0)
		SCARG(uap, pid) = -q->p_pgid;
	if (SCARG(uap, options) &~ (WUNTRACED|WNOHANG))
		return (EINVAL);

loop:
	nfound = 0;
	for (p = q->p_children.lh_first; p != 0; p = p->p_sibling.le_next) {
		if (SCARG(uap, pid) != WAIT_ANY &&
		    p->p_pid != SCARG(uap, pid) &&
		    p->p_pgid != -SCARG(uap, pid))
			continue;
		nfound++;
		if (p->p_stat == SZOMB) {
			retval[0] = p->p_pid;

			if (SCARG(uap, status)) {
				status = p->p_xstat;	/* convert to int */
				error = copyout((caddr_t)&status,
						(caddr_t)(u_long)SCARG(uap, status),
						sizeof(status));
				if (error)
					return (error);
			}
			if (SCARG(uap, rusage)) {
				netbsd32_from_rusage(p->p_ru, &ru32);
				if ((error = copyout((caddr_t)&ru32,
						     (caddr_t)(u_long)SCARG(uap, rusage),
						     sizeof(struct netbsd32_rusage))))
					return (error);
			}
			/*
			 * If we got the child via ptrace(2) or procfs, and
			 * the parent is different (meaning the process was
			 * attached, rather than run as a child), then we need
			 * to give it back to the old parent, and send the
			 * parent a SIGCHLD.  The rest of the cleanup will be
			 * done when the old parent waits on the child.
			 */
			if ((p->p_flag & P_TRACED) &&
			    p->p_oppid != p->p_pptr->p_pid) {
				t = pfind(p->p_oppid);
				proc_reparent(p, t ? t : initproc);
				p->p_oppid = 0;
				p->p_flag &= ~(P_TRACED|P_WAITED|P_FSTRACE);
				psignal(p->p_pptr, SIGCHLD);
				wakeup((caddr_t)p->p_pptr);
				return (0);
			}
			p->p_xstat = 0;
			ruadd(&q->p_stats->p_cru, p->p_ru);
			pool_put(&rusage_pool, p->p_ru);

			/*
			 * Finally finished with old proc entry.
			 * Unlink it from its process group and free it.
			 */
			leavepgrp(p);

			LIST_REMOVE(p, p_list);	/* off zombproc */

			LIST_REMOVE(p, p_sibling);

			/*
			 * Decrement the count of procs running with this uid.
			 */
			(void)chgproccnt(p->p_cred->p_ruid, -1);

			/*
			 * Free up credentials.
			 */
			if (--p->p_cred->p_refcnt == 0) {
				crfree(p->p_cred->pc_ucred);
				pool_put(&pcred_pool, p->p_cred);
			}

			/*
			 * Release reference to text vnode
			 */
			if (p->p_textvp)
				vrele(p->p_textvp);

			pool_put(&proc_pool, p);
			nprocs--;
			return (0);
		}
		if (p->p_stat == SSTOP && (p->p_flag & P_WAITED) == 0 &&
		    (p->p_flag & P_TRACED || SCARG(uap, options) & WUNTRACED)) {
			p->p_flag |= P_WAITED;
			retval[0] = p->p_pid;

			if (SCARG(uap, status)) {
				status = W_STOPCODE(p->p_xstat);
				error = copyout((caddr_t)&status,
				    (caddr_t)(u_long)SCARG(uap, status),
				    sizeof(status));
			} else
				error = 0;
			return (error);
		}
	}
	if (nfound == 0)
		return (ECHILD);
	if (SCARG(uap, options) & WNOHANG) {
		retval[0] = 0;
		return (0);
	}
	if ((error = tsleep((caddr_t)q, PWAIT | PCATCH, "wait", 0)) != 0)
		return (error);
	goto loop;
}

int
netbsd32_getrusage(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_getrusage_args /* {
		syscallarg(int) who;
		syscallarg(netbsd32_rusagep_t) rusage;
	} */ *uap = v;
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
	return (copyout(&ru, (caddr_t)(u_long)SCARG(uap, rusage), sizeof(ru)));
}
