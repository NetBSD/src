/*	$NetBSD: kern_resource.c,v 1.70 2003/03/14 21:38:26 dsl Exp $	*/

/*-
 * Copyright (c) 1982, 1986, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)kern_resource.c	8.8 (Berkeley) 2/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_resource.c,v 1.70 2003/03/14 21:38:26 dsl Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/resourcevar.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/proc.h>

#include <sys/mount.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>

#include <uvm/uvm_extern.h>

/*
 * Maximum process data and stack limits.
 * They are variables so they are patchable.
 *
 * XXXX Do we really need them to be patchable?
 */
rlim_t maxdmap = MAXDSIZ;
rlim_t maxsmap = MAXSSIZ;

/*
 * Resource controls and accounting.
 */

int
sys_getpriority(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_getpriority_args /* {
		syscallarg(int) which;
		syscallarg(int) who;
	} */ *uap = v;
	struct proc *curp = l->l_proc, *p;
	int low = NZERO + PRIO_MAX + 1;

	switch (SCARG(uap, which)) {

	case PRIO_PROCESS:
		if (SCARG(uap, who) == 0)
			p = curp;
		else
			p = pfind(SCARG(uap, who));
		if (p == 0)
			break;
		low = p->p_nice;
		break;

	case PRIO_PGRP: {
		struct pgrp *pg;

		if (SCARG(uap, who) == 0)
			pg = curp->p_pgrp;
		else if ((pg = pgfind(SCARG(uap, who))) == NULL)
			break;
		LIST_FOREACH(p, &pg->pg_members, p_pglist) {
			if (p->p_nice < low)
				low = p->p_nice;
		}
		break;
	}

	case PRIO_USER:
		if (SCARG(uap, who) == 0)
			SCARG(uap, who) = curp->p_ucred->cr_uid;
		proclist_lock_read();
		LIST_FOREACH(p, &allproc, p_list) {
			if (p->p_ucred->cr_uid == (uid_t) SCARG(uap, who) &&
			    p->p_nice < low)
				low = p->p_nice;
		}
		proclist_unlock_read();
		break;

	default:
		return (EINVAL);
	}
	if (low == NZERO + PRIO_MAX + 1)
		return (ESRCH);
	*retval = low - NZERO;
	return (0);
}

/* ARGSUSED */
int
sys_setpriority(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_setpriority_args /* {
		syscallarg(int) which;
		syscallarg(int) who;
		syscallarg(int) prio;
	} */ *uap = v;
	struct proc *curp = l->l_proc, *p;
	int found = 0, error = 0;

	switch (SCARG(uap, which)) {

	case PRIO_PROCESS:
		if (SCARG(uap, who) == 0)
			p = curp;
		else
			p = pfind(SCARG(uap, who));
		if (p == 0)
			break;
		error = donice(curp, p, SCARG(uap, prio));
		found++;
		break;

	case PRIO_PGRP: {
		struct pgrp *pg;
		 
		if (SCARG(uap, who) == 0)
			pg = curp->p_pgrp;
		else if ((pg = pgfind(SCARG(uap, who))) == NULL)
			break;
		LIST_FOREACH(p, &pg->pg_members, p_pglist) {
			error = donice(curp, p, SCARG(uap, prio));
			found++;
		}
		break;
	}

	case PRIO_USER:
		if (SCARG(uap, who) == 0)
			SCARG(uap, who) = curp->p_ucred->cr_uid;
		proclist_lock_read();
		LIST_FOREACH(p, &allproc, p_list) {
			if (p->p_ucred->cr_uid == (uid_t) SCARG(uap, who)) {
				error = donice(curp, p, SCARG(uap, prio));
				found++;
			}
		}
		proclist_unlock_read();
		break;

	default:
		return (EINVAL);
	}
	if (found == 0)
		return (ESRCH);
	return (error);
}

int
donice(curp, chgp, n)
	struct proc *curp, *chgp;
	int n;
{
	struct pcred *pcred = curp->p_cred;
	int s;

	if (pcred->pc_ucred->cr_uid && pcred->p_ruid &&
	    pcred->pc_ucred->cr_uid != chgp->p_ucred->cr_uid &&
	    pcred->p_ruid != chgp->p_ucred->cr_uid)
		return (EPERM);
	if (n > PRIO_MAX)
		n = PRIO_MAX;
	if (n < PRIO_MIN)
		n = PRIO_MIN;
	n += NZERO;
	if (n < chgp->p_nice && suser(pcred->pc_ucred, &curp->p_acflag))
		return (EACCES);
	chgp->p_nice = n;
	SCHED_LOCK(s);
	(void)resetprocpriority(chgp);
	SCHED_UNLOCK(s);
	return (0);
}

/* ARGSUSED */
int
sys_setrlimit(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_setrlimit_args /* {
		syscallarg(int) which;
		syscallarg(const struct rlimit *) rlp;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int which = SCARG(uap, which);
	struct rlimit alim;
	int error;

	error = copyin(SCARG(uap, rlp), &alim, sizeof(struct rlimit));
	if (error)
		return (error);
	return (dosetrlimit(p, p->p_cred, which, &alim));
}

int
dosetrlimit(p, cred, which, limp)
	struct proc *p;
	struct  pcred *cred;
	int which;
	struct rlimit *limp;
{
	struct rlimit *alimp;
	struct plimit *newplim;
	int error;

	if ((u_int)which >= RLIM_NLIMITS)
		return (EINVAL);

	if (limp->rlim_cur < 0 || limp->rlim_max < 0)
		return (EINVAL);

	alimp = &p->p_rlimit[which];
	/* if we don't change the value, no need to limcopy() */
	if (limp->rlim_cur == alimp->rlim_cur &&
	    limp->rlim_max == alimp->rlim_max)
		return 0;

	if (limp->rlim_cur > limp->rlim_max) {
		/*
		 * This is programming error. According to SUSv2, we should
		 * return error in this case.
		 */
		return (EINVAL);
	}
	if (limp->rlim_max > alimp->rlim_max
	    && (error = suser(cred->pc_ucred, &p->p_acflag)) != 0)
			return (error);

	if (p->p_limit->p_refcnt > 1 &&
	    (p->p_limit->p_lflags & PL_SHAREMOD) == 0) {
		newplim = limcopy(p->p_limit);
		limfree(p->p_limit);
		p->p_limit = newplim;
		alimp = &p->p_rlimit[which];
	}

	switch (which) {

	case RLIMIT_DATA:
		if (limp->rlim_cur > maxdmap)
			limp->rlim_cur = maxdmap;
		if (limp->rlim_max > maxdmap)
			limp->rlim_max = maxdmap;
		break;

	case RLIMIT_STACK:
		if (limp->rlim_cur > maxsmap)
			limp->rlim_cur = maxsmap;
		if (limp->rlim_max > maxsmap)
			limp->rlim_max = maxsmap;

		/*
		 * Return EINVAL if the new stack size limit is lower than
		 * current usage. Otherwise, the process would get SIGSEGV the
		 * moment it would try to access anything on it's current stack.
		 * This conforms to SUSv2.
		 */
		if (limp->rlim_cur < p->p_vmspace->vm_ssize * PAGE_SIZE
		    || limp->rlim_max < p->p_vmspace->vm_ssize * PAGE_SIZE)
			return (EINVAL);

		/*
		 * Stack is allocated to the max at exec time with
		 * only "rlim_cur" bytes accessible (In other words,
		 * allocates stack dividing two contiguous regions at
		 * "rlim_cur" bytes boundary).
		 *
		 * Since allocation is done in terms of page, roundup
		 * "rlim_cur" (otherwise, contiguous regions
		 * overlap).  If stack limit is going up make more
		 * accessible, if going down make inaccessible.
		 */
		limp->rlim_cur = round_page(limp->rlim_cur);
		if (limp->rlim_cur != alimp->rlim_cur) {
			vaddr_t addr;
			vsize_t size;
			vm_prot_t prot;

			if (limp->rlim_cur > alimp->rlim_cur) {
				prot = VM_PROT_ALL;
				size = limp->rlim_cur - alimp->rlim_cur;
				addr = USRSTACK - limp->rlim_cur;
			} else {
				prot = VM_PROT_NONE;
				size = alimp->rlim_cur - limp->rlim_cur;
				addr = USRSTACK - alimp->rlim_cur;
			}
			(void) uvm_map_protect(&p->p_vmspace->vm_map,
					      addr, addr+size, prot, FALSE);
		}
		break;

	case RLIMIT_NOFILE:
		if (limp->rlim_cur > maxfiles)
			limp->rlim_cur = maxfiles;
		if (limp->rlim_max > maxfiles)
			limp->rlim_max = maxfiles;
		break;

	case RLIMIT_NPROC:
		if (limp->rlim_cur > maxproc)
			limp->rlim_cur = maxproc;
		if (limp->rlim_max > maxproc)
			limp->rlim_max = maxproc;
		break;
	}
	*alimp = *limp;
	return (0);
}

/* ARGSUSED */
int
sys_getrlimit(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_getrlimit_args /* {
		syscallarg(int) which;
		syscallarg(struct rlimit *) rlp;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int which = SCARG(uap, which);

	if ((u_int)which >= RLIM_NLIMITS)
		return (EINVAL);
	return (copyout(&p->p_rlimit[which], SCARG(uap, rlp),
	    sizeof(struct rlimit)));
}

/*
 * Transform the running time and tick information in proc p into user,
 * system, and interrupt time usage.
 */
void
calcru(p, up, sp, ip)
	struct proc *p;
	struct timeval *up;
	struct timeval *sp;
	struct timeval *ip;
{
	u_quad_t u, st, ut, it, tot;
	unsigned long sec;
	long usec;
	int s;
	struct timeval tv;
	struct lwp *l;

	s = splstatclock();
	st = p->p_sticks;
	ut = p->p_uticks;
	it = p->p_iticks;
	splx(s);

	sec = p->p_rtime.tv_sec;
	usec = p->p_rtime.tv_usec;
	LIST_FOREACH(l, &p->p_lwps, l_sibling) {
		if (l->l_stat == LSONPROC) {
			struct schedstate_percpu *spc;
			
			KDASSERT(l->l_cpu != NULL);
			spc = &l->l_cpu->ci_schedstate;
			
			/*
			 * Adjust for the current time slice.  This is
			 * actually fairly important since the error
			 * here is on the order of a time quantum,
			 * which is much greater than the sampling
			 * error.  
			 */
			microtime(&tv);
			sec += tv.tv_sec - spc->spc_runtime.tv_sec;
			usec += tv.tv_usec - spc->spc_runtime.tv_usec;
		}
	}

	tot = st + ut + it;
	u = sec * 1000000ull + usec;

	if (tot == 0) {
		/* No ticks, so can't use to share time out, split 50-50 */
		st = ut = u / 2;
	} else {
		st = (u * st) / tot;
		ut = (u * ut) / tot;
	}
	sp->tv_sec = st / 1000000;
	sp->tv_usec = st % 1000000;
	up->tv_sec = ut / 1000000;
	up->tv_usec = ut % 1000000;
	if (ip != NULL) {
		if (it != 0)
			it = (u * it) / tot;
		ip->tv_sec = it / 1000000;
		ip->tv_usec = it % 1000000;
	}
}

/* ARGSUSED */
int
sys_getrusage(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_getrusage_args /* {
		syscallarg(int) who;
		syscallarg(struct rusage *) rusage;
	} */ *uap = v;
	struct rusage *rup;
	struct proc *p = l->l_proc;

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
	return (copyout(rup, SCARG(uap, rusage), sizeof(struct rusage)));
}

void
ruadd(ru, ru2)
	struct rusage *ru, *ru2;
{
	long *ip, *ip2;
	int i;

	timeradd(&ru->ru_utime, &ru2->ru_utime, &ru->ru_utime);
	timeradd(&ru->ru_stime, &ru2->ru_stime, &ru->ru_stime);
	if (ru->ru_maxrss < ru2->ru_maxrss)
		ru->ru_maxrss = ru2->ru_maxrss;
	ip = &ru->ru_first; ip2 = &ru2->ru_first;
	for (i = &ru->ru_last - &ru->ru_first; i >= 0; i--)
		*ip++ += *ip2++;
}

/*
 * Make a copy of the plimit structure.
 * We share these structures copy-on-write after fork,
 * and copy when a limit is changed.
 */
struct plimit *
limcopy(lim)
	struct plimit *lim;
{
	struct plimit *newlim;

	newlim = pool_get(&plimit_pool, PR_WAITOK);
	memcpy(newlim->pl_rlimit, lim->pl_rlimit,
	    sizeof(struct rlimit) * RLIM_NLIMITS);
	if (lim->pl_corename == defcorename) {
		newlim->pl_corename = defcorename;
	} else {
		newlim->pl_corename = malloc(strlen(lim->pl_corename)+1,
		    M_TEMP, M_WAITOK);
		strcpy(newlim->pl_corename, lim->pl_corename);
	}
	newlim->p_lflags = 0;
	newlim->p_refcnt = 1;
	return (newlim);
}

void
limfree(lim)
	struct plimit *lim;
{

	if (--lim->p_refcnt > 0)
		return;
#ifdef DIAGNOSTIC
	if (lim->p_refcnt < 0)
		panic("limfree");
#endif
	if (lim->pl_corename != defcorename)
		free(lim->pl_corename, M_TEMP);
	pool_put(&plimit_pool, lim);
}

struct pstats *
pstatscopy(ps)
	struct pstats *ps;
{
	
	struct pstats *newps;

	newps = pool_get(&pstats_pool, PR_WAITOK);

	memset(&newps->pstat_startzero, 0,
	(unsigned) ((caddr_t)&newps->pstat_endzero -
		    (caddr_t)&newps->pstat_startzero));
	memcpy(&newps->pstat_startcopy, &ps->pstat_startcopy,
	((caddr_t)&newps->pstat_endcopy -
	 (caddr_t)&newps->pstat_startcopy));

	return (newps);

}

void
pstatsfree(ps)
	struct pstats *ps;
{

	pool_put(&pstats_pool, ps);
}
