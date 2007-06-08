/*	$NetBSD: kern_resource.c,v 1.116.2.3 2007/06/08 14:17:21 ad Exp $	*/

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
 *	@(#)kern_resource.c	8.8 (Berkeley) 2/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_resource.c,v 1.116.2.3 2007/06/08 14:17:21 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/resourcevar.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <sys/kauth.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <uvm/uvm_extern.h>

/*
 * Maximum process data and stack limits.
 * They are variables so they are patchable.
 */
rlim_t maxdmap = MAXDSIZ;
rlim_t maxsmap = MAXSSIZ;

struct uihashhead *uihashtbl;
u_long uihash;		/* size of hash table - 1 */
kmutex_t uihashtbl_lock;

/*
 * Resource controls and accounting.
 */

int
sys_getpriority(struct lwp *l, void *v, register_t *retval)
{
	struct sys_getpriority_args /* {
		syscallarg(int) which;
		syscallarg(id_t) who;
	} */ *uap = v;
	struct proc *curp = l->l_proc, *p;
	int low = NZERO + PRIO_MAX + 1;
	int who = SCARG(uap, who);

	mutex_enter(&proclist_lock);
	switch (SCARG(uap, which)) {
	case PRIO_PROCESS:
		if (who == 0)
			p = curp;
		else
			p = p_find(who, PFIND_LOCKED);
		if (p != NULL)
			low = p->p_nice;
		break;

	case PRIO_PGRP: {
		struct pgrp *pg;

		if (who == 0)
			pg = curp->p_pgrp;
		else if ((pg = pg_find(who, PFIND_LOCKED)) == NULL)
			break;
		LIST_FOREACH(p, &pg->pg_members, p_pglist) {
			if (p->p_nice < low)
				low = p->p_nice;
		}
		break;
	}

	case PRIO_USER:
		if (who == 0)
			who = (int)kauth_cred_geteuid(l->l_cred);
		PROCLIST_FOREACH(p, &allproc) {
			mutex_enter(&p->p_mutex);
			if (kauth_cred_geteuid(p->p_cred) ==
			    (uid_t)who && p->p_nice < low)
				low = p->p_nice;
			mutex_exit(&p->p_mutex);
		}
		break;

	default:
		mutex_exit(&proclist_lock);
		return (EINVAL);
	}
	mutex_exit(&proclist_lock);

	if (low == NZERO + PRIO_MAX + 1)
		return (ESRCH);
	*retval = low - NZERO;
	return (0);
}

/* ARGSUSED */
int
sys_setpriority(struct lwp *l, void *v, register_t *retval)
{
	struct sys_setpriority_args /* {
		syscallarg(int) which;
		syscallarg(id_t) who;
		syscallarg(int) prio;
	} */ *uap = v;
	struct proc *curp = l->l_proc, *p;
	int found = 0, error = 0;
	int who = SCARG(uap, who);

	mutex_enter(&proclist_lock);
	switch (SCARG(uap, which)) {
	case PRIO_PROCESS:
		if (who == 0)
			p = curp;
		else
			p = p_find(who, PFIND_LOCKED);
		if (p != 0) {
			mutex_enter(&p->p_mutex);
			error = donice(l, p, SCARG(uap, prio));
			mutex_exit(&p->p_mutex);
		}
		found++;
		break;

	case PRIO_PGRP: {
		struct pgrp *pg;

		if (who == 0)
			pg = curp->p_pgrp;
		else if ((pg = pg_find(who, PFIND_LOCKED)) == NULL)
			break;
		LIST_FOREACH(p, &pg->pg_members, p_pglist) {
			mutex_enter(&p->p_mutex);
			error = donice(l, p, SCARG(uap, prio));
			mutex_exit(&p->p_mutex);
			found++;
		}
		break;
	}

	case PRIO_USER:
		if (who == 0)
			who = (int)kauth_cred_geteuid(l->l_cred);
		PROCLIST_FOREACH(p, &allproc) {
			mutex_enter(&p->p_mutex);
			if (kauth_cred_geteuid(p->p_cred) ==
			    (uid_t)SCARG(uap, who)) {
				error = donice(l, p, SCARG(uap, prio));
				found++;
			}
			mutex_exit(&p->p_mutex);
		}
		break;

	default:
		error = EINVAL;
		break;
	}
	mutex_exit(&proclist_lock);
	if (found == 0)
		return (ESRCH);
	return (error);
}

/*
 * Renice a process.
 *
 * Call with the target process' credentials locked.
 */
int
donice(struct lwp *l, struct proc *chgp, int n)
{
	kauth_cred_t cred = l->l_cred;
	int onice;

	KASSERT(mutex_owned(&chgp->p_mutex));

	if (n > PRIO_MAX)
		n = PRIO_MAX;
	if (n < PRIO_MIN)
		n = PRIO_MIN;
	n += NZERO;
	onice = chgp->p_nice;
	onice = chgp->p_nice;

  again:
	if (kauth_authorize_process(cred, KAUTH_PROCESS_NICE, chgp,
	    KAUTH_ARG(n), NULL, NULL))
		return (EACCES);
	mutex_spin_enter(&chgp->p_stmutex);
	if (onice != chgp->p_nice) {
		mutex_spin_exit(&chgp->p_stmutex);
		goto again;
	}
	sched_nice(chgp, n);
	mutex_spin_exit(&chgp->p_stmutex);
	return (0);
}

/* ARGSUSED */
int
sys_setrlimit(struct lwp *l, void *v, register_t *retval)
{
	struct sys_setrlimit_args /* {
		syscallarg(int) which;
		syscallarg(const struct rlimit *) rlp;
	} */ *uap = v;
	int which = SCARG(uap, which);
	struct rlimit alim;
	int error;

	error = copyin(SCARG(uap, rlp), &alim, sizeof(struct rlimit));
	if (error)
		return (error);
	return (dosetrlimit(l, l->l_proc, which, &alim));
}

int
dosetrlimit(struct lwp *l, struct proc *p, int which, struct rlimit *limp)
{
	struct rlimit *alimp;
	struct plimit *oldplim;
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
	error = kauth_authorize_process(l->l_cred, KAUTH_PROCESS_RLIMIT,
	    p, limp, KAUTH_ARG(which), NULL);
	if (error)
			return (error);

	mutex_enter(&p->p_mutex);
	if (p->p_limit->p_refcnt > 1 &&
	    (p->p_limit->p_lflags & PL_SHAREMOD) == 0) {
	    	oldplim = p->p_limit;
		p->p_limit = limcopy(p);
		limfree(oldplim);
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
		    || limp->rlim_max < p->p_vmspace->vm_ssize * PAGE_SIZE) {
			mutex_exit(&p->p_mutex);
			return (EINVAL);
		}

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
				prot = VM_PROT_READ | VM_PROT_WRITE;
				size = limp->rlim_cur - alimp->rlim_cur;
				addr = (vaddr_t)p->p_vmspace->vm_minsaddr -
				    limp->rlim_cur;
			} else {
				prot = VM_PROT_NONE;
				size = alimp->rlim_cur - limp->rlim_cur;
				addr = (vaddr_t)p->p_vmspace->vm_minsaddr -
				     alimp->rlim_cur;
			}
			(void) uvm_map_protect(&p->p_vmspace->vm_map,
			    addr, addr+size, prot, false);
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
	mutex_exit(&p->p_mutex);
	return (0);
}

/* ARGSUSED */
int
sys_getrlimit(struct lwp *l, void *v, register_t *retval)
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
 *
 * Should be called with p->p_smutex held unless called from exit1().
 */
void
calcru(struct proc *p, struct timeval *up, struct timeval *sp,
    struct timeval *ip, struct timeval *rp)
{
	u_quad_t u, st, ut, it, tot;
	unsigned long sec;
	long usec;
 	struct timeval tv;
	struct lwp *l;

	mutex_spin_enter(&p->p_stmutex);
	st = p->p_sticks;
	ut = p->p_uticks;
	it = p->p_iticks;
	mutex_spin_exit(&p->p_stmutex);

	sec = p->p_rtime.tv_sec;
	usec = p->p_rtime.tv_usec;

	LIST_FOREACH(l, &p->p_lwps, l_sibling) {
		lwp_lock(l);
		sec += l->l_rtime.tv_sec;
		if ((usec += l->l_rtime.tv_usec) >= 1000000) {
			sec++;
			usec -= 1000000;
		}
		if (l->l_cpu == curcpu()) {
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
			if (usec >= 1000000) {
				sec++;
				usec -= 1000000;
			}
		}
		lwp_unlock(l);
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
	if (sp != NULL) {
		sp->tv_sec = st / 1000000;
		sp->tv_usec = st % 1000000;
	}
	if (up != NULL) {
		up->tv_sec = ut / 1000000;
		up->tv_usec = ut % 1000000;
	}
	if (ip != NULL) {
		if (it != 0)
			it = (u * it) / tot;
		ip->tv_sec = it / 1000000;
		ip->tv_usec = it % 1000000;
	}
	if (rp != NULL) {
		rp->tv_sec = sec;
		rp->tv_usec = usec;
	}
}

/* ARGSUSED */
int
sys_getrusage(struct lwp *l, void *v, register_t *retval)
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
	return (copyout(rup, SCARG(uap, rusage), sizeof(struct rusage)));
}

void
ruadd(struct rusage *ru, struct rusage *ru2)
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
 *
 * XXXSMP This is atrocious, need to simplify.
 */
struct plimit *
limcopy(struct proc *p)
{
	struct plimit *lim, *newlim;
	char *corename;
	size_t l;

	KASSERT(mutex_owned(&p->p_mutex));

	mutex_exit(&p->p_mutex);
	newlim = pool_get(&plimit_pool, PR_WAITOK);
	mutex_init(&newlim->p_lock, MUTEX_DEFAULT, IPL_NONE);
	newlim->p_lflags = 0;
	newlim->p_refcnt = 1;
	mutex_enter(&p->p_mutex);

	for (;;) {
		lim = p->p_limit;
		mutex_enter(&lim->p_lock);
		if (lim->pl_corename != defcorename) {
			l = strlen(lim->pl_corename) + 1;

			mutex_exit(&lim->p_lock);
			mutex_exit(&p->p_mutex);
			corename = malloc(l, M_TEMP, M_WAITOK);
			mutex_enter(&p->p_mutex);
			mutex_enter(&lim->p_lock);

			if (l != strlen(lim->pl_corename) + 1) {
				mutex_exit(&lim->p_lock);
				mutex_exit(&p->p_mutex);
				free(corename, M_TEMP);
				mutex_enter(&p->p_mutex);
				continue;
			}
		} else
			l = 0;
			
		memcpy(newlim->pl_rlimit, lim->pl_rlimit,
		    sizeof(struct rlimit) * RLIM_NLIMITS);
		if (l != 0)
			strlcpy(newlim->pl_corename, lim->pl_corename, l);
		else
			newlim->pl_corename = defcorename;
		mutex_exit(&lim->p_lock);
		break;
	}

	return (newlim);
}

void
limfree(struct plimit *lim)
{
	int n;

	mutex_enter(&lim->p_lock);
	n = --lim->p_refcnt;
	mutex_exit(&lim->p_lock);
	if (n > 0)
		return;
#ifdef DIAGNOSTIC
	if (n < 0)
		panic("limfree");
#endif
	if (lim->pl_corename != defcorename)
		free(lim->pl_corename, M_TEMP);
	mutex_destroy(&lim->p_lock);
	pool_put(&plimit_pool, lim);
}

struct pstats *
pstatscopy(struct pstats *ps)
{

	struct pstats *newps;

	newps = pool_get(&pstats_pool, PR_WAITOK);

	memset(&newps->pstat_startzero, 0,
	(unsigned) ((char *)&newps->pstat_endzero -
		    (char *)&newps->pstat_startzero));
	memcpy(&newps->pstat_startcopy, &ps->pstat_startcopy,
	((char *)&newps->pstat_endcopy -
	 (char *)&newps->pstat_startcopy));

	return (newps);

}

void
pstatsfree(struct pstats *ps)
{

	pool_put(&pstats_pool, ps);
}

/*
 * sysctl interface in five parts
 */

/*
 * a routine for sysctl proc subtree helpers that need to pick a valid
 * process by pid.
 */
static int
sysctl_proc_findproc(struct lwp *l, struct proc **p2, pid_t pid)
{
	struct proc *ptmp;
	int error = 0;

	if (pid == PROC_CURPROC)
		ptmp = l->l_proc;
	else if ((ptmp = pfind(pid)) == NULL)
		error = ESRCH;

	*p2 = ptmp;
	return (error);
}

/*
 * sysctl helper routine for setting a process's specific corefile
 * name.  picks the process based on the given pid and checks the
 * correctness of the new value.
 */
static int
sysctl_proc_corename(SYSCTLFN_ARGS)
{
	struct proc *ptmp;
	struct plimit *lim;
	int error = 0, len;
	char *cname;
	char *tmp;
	struct sysctlnode node;

	/*
	 * is this all correct?
	 */
	if (namelen != 0)
		return (EINVAL);
	if (name[-1] != PROC_PID_CORENAME)
		return (EINVAL);

	/*
	 * whom are we tweaking?
	 */
	error = sysctl_proc_findproc(l, &ptmp, (pid_t)name[-2]);
	if (error)
		return (error);

	/* XXX this should be in p_find() */
	error = kauth_authorize_process(l->l_cred, KAUTH_PROCESS_CANSEE,
	    ptmp, NULL, NULL, NULL);
	if (error)
		return (error);

	cname = PNBUF_GET();
	/*
	 * let them modify a temporary copy of the core name
	 */
	node = *rnode;
	strlcpy(cname, ptmp->p_limit->pl_corename, MAXPATHLEN);
	node.sysctl_data = cname;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	/*
	 * if that failed, or they have nothing new to say, or we've
	 * heard it before...
	 */
	if (error || newp == NULL ||
	    strcmp(cname, ptmp->p_limit->pl_corename) == 0) {
		goto done;
	}

	error = kauth_authorize_process(l->l_cred, KAUTH_PROCESS_CORENAME,
	    ptmp, cname, NULL, NULL);
	if (error)
		return (error);

	/*
	 * no error yet and cname now has the new core name in it.
	 * let's see if it looks acceptable.  it must be either "core"
	 * or end in ".core" or "/core".
	 */
	len = strlen(cname);
	if (len < 4) {
		error = EINVAL;
	} else if (strcmp(cname + len - 4, "core") != 0) {
		error = EINVAL;
	} else if (len > 4 && cname[len - 5] != '/' && cname[len - 5] != '.') {
		error = EINVAL;
	}
	if (error != 0) {
		goto done;
	}

	/*
	 * hmm...looks good.  now...where do we put it?
	 */
	tmp = malloc(len + 1, M_TEMP, M_WAITOK|M_CANFAIL);
	if (tmp == NULL) {
		error = ENOMEM;
		goto done;
	}
	strlcpy(tmp, cname, len + 1);

	mutex_enter(&ptmp->p_mutex);
	lim = ptmp->p_limit;
	if (lim->p_refcnt > 1 && (lim->p_lflags & PL_SHAREMOD) == 0) {
		ptmp->p_limit = limcopy(ptmp);
		limfree(lim);
		lim = ptmp->p_limit;
	}
	if (lim->pl_corename != defcorename)
		free(lim->pl_corename, M_TEMP);
	lim->pl_corename = tmp;
	mutex_exit(&ptmp->p_mutex);
done:
	PNBUF_PUT(cname);
	return error;
}

/*
 * sysctl helper routine for checking/setting a process's stop flags,
 * one for fork and one for exec.
 */
static int
sysctl_proc_stop(SYSCTLFN_ARGS)
{
	struct proc *ptmp;
	int i, f, error = 0;
	struct sysctlnode node;

	if (namelen != 0)
		return (EINVAL);

	error = sysctl_proc_findproc(l, &ptmp, (pid_t)name[-2]);
	if (error)
		return (error);

	/* XXX this should be in p_find() */
	error = kauth_authorize_process(l->l_cred, KAUTH_PROCESS_CANSEE,
	    ptmp, NULL, NULL, NULL);
	if (error)
		return (error);

	switch (rnode->sysctl_num) {
	case PROC_PID_STOPFORK:
		f = PS_STOPFORK;
		break;
	case PROC_PID_STOPEXEC:
		f = PS_STOPEXEC;
		break;
	case PROC_PID_STOPEXIT:
		f = PS_STOPEXIT;
		break;
	default:
		return (EINVAL);
	}

	i = (ptmp->p_flag & f) ? 1 : 0;
	node = *rnode;
	node.sysctl_data = &i;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	mutex_enter(&ptmp->p_smutex);
	error = kauth_authorize_process(l->l_cred, KAUTH_PROCESS_STOPFLAG,
	    ptmp, KAUTH_ARG(f), NULL, NULL);
	if (error)
		return (error);
	if (i)
		ptmp->p_sflag |= f;
	else
		ptmp->p_sflag &= ~f;
	mutex_exit(&ptmp->p_smutex);

	return (0);
}

/*
 * sysctl helper routine for a process's rlimits as exposed by sysctl.
 */
static int
sysctl_proc_plimit(SYSCTLFN_ARGS)
{
	struct proc *ptmp;
	u_int limitno;
	int which, error = 0;
        struct rlimit alim;
	struct sysctlnode node;

	if (namelen != 0)
		return (EINVAL);

	which = name[-1];
	if (which != PROC_PID_LIMIT_TYPE_SOFT &&
	    which != PROC_PID_LIMIT_TYPE_HARD)
		return (EINVAL);

	limitno = name[-2] - 1;
	if (limitno >= RLIM_NLIMITS)
		return (EINVAL);

	if (name[-3] != PROC_PID_LIMIT)
		return (EINVAL);

	error = sysctl_proc_findproc(l, &ptmp, (pid_t)name[-4]);
	if (error)
		return (error);

	/* XXX this should be in p_find() */
	error = kauth_authorize_process(l->l_cred, KAUTH_PROCESS_CANSEE,
	    ptmp, NULL, NULL, NULL);
	if (error)
		return (error);

	node = *rnode;
	memcpy(&alim, &ptmp->p_rlimit[limitno], sizeof(alim));
	if (which == PROC_PID_LIMIT_TYPE_HARD)
		node.sysctl_data = &alim.rlim_max;
	else
		node.sysctl_data = &alim.rlim_cur;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	return (dosetrlimit(l, ptmp, limitno, &alim));
}

/*
 * and finally, the actually glue that sticks it to the tree
 */
SYSCTL_SETUP(sysctl_proc_setup, "sysctl proc subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "proc", NULL,
		       NULL, 0, NULL, 0,
		       CTL_PROC, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_ANYNUMBER,
		       CTLTYPE_NODE, "curproc",
		       SYSCTL_DESCR("Per-process settings"),
		       NULL, 0, NULL, 0,
		       CTL_PROC, PROC_CURPROC, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE|CTLFLAG_ANYWRITE,
		       CTLTYPE_STRING, "corename",
		       SYSCTL_DESCR("Core file name"),
		       sysctl_proc_corename, 0, NULL, MAXPATHLEN,
		       CTL_PROC, PROC_CURPROC, PROC_PID_CORENAME, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "rlimit",
		       SYSCTL_DESCR("Process limits"),
		       NULL, 0, NULL, 0,
		       CTL_PROC, PROC_CURPROC, PROC_PID_LIMIT, CTL_EOL);

#define create_proc_plimit(s, n) do {					\
	sysctl_createv(clog, 0, NULL, NULL,				\
		       CTLFLAG_PERMANENT,				\
		       CTLTYPE_NODE, s,					\
		       SYSCTL_DESCR("Process " s " limits"),		\
		       NULL, 0, NULL, 0,				\
		       CTL_PROC, PROC_CURPROC, PROC_PID_LIMIT, n,	\
		       CTL_EOL);					\
	sysctl_createv(clog, 0, NULL, NULL,				\
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE|CTLFLAG_ANYWRITE, \
		       CTLTYPE_QUAD, "soft",				\
		       SYSCTL_DESCR("Process soft " s " limit"),	\
		       sysctl_proc_plimit, 0, NULL, 0,			\
		       CTL_PROC, PROC_CURPROC, PROC_PID_LIMIT, n,	\
		       PROC_PID_LIMIT_TYPE_SOFT, CTL_EOL);		\
	sysctl_createv(clog, 0, NULL, NULL,				\
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE|CTLFLAG_ANYWRITE, \
		       CTLTYPE_QUAD, "hard",				\
		       SYSCTL_DESCR("Process hard " s " limit"),	\
		       sysctl_proc_plimit, 0, NULL, 0,			\
		       CTL_PROC, PROC_CURPROC, PROC_PID_LIMIT, n,	\
		       PROC_PID_LIMIT_TYPE_HARD, CTL_EOL);		\
	} while (0/*CONSTCOND*/)

	create_proc_plimit("cputime",		PROC_PID_LIMIT_CPU);
	create_proc_plimit("filesize",		PROC_PID_LIMIT_FSIZE);
	create_proc_plimit("datasize",		PROC_PID_LIMIT_DATA);
	create_proc_plimit("stacksize",		PROC_PID_LIMIT_STACK);
	create_proc_plimit("coredumpsize",	PROC_PID_LIMIT_CORE);
	create_proc_plimit("memoryuse",		PROC_PID_LIMIT_RSS);
	create_proc_plimit("memorylocked",	PROC_PID_LIMIT_MEMLOCK);
	create_proc_plimit("maxproc",		PROC_PID_LIMIT_NPROC);
	create_proc_plimit("descriptors",	PROC_PID_LIMIT_NOFILE);
	create_proc_plimit("sbsize",		PROC_PID_LIMIT_SBSIZE);

#undef create_proc_plimit

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE|CTLFLAG_ANYWRITE,
		       CTLTYPE_INT, "stopfork",
		       SYSCTL_DESCR("Stop process at fork(2)"),
		       sysctl_proc_stop, 0, NULL, 0,
		       CTL_PROC, PROC_CURPROC, PROC_PID_STOPFORK, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE|CTLFLAG_ANYWRITE,
		       CTLTYPE_INT, "stopexec",
		       SYSCTL_DESCR("Stop process at execve(2)"),
		       sysctl_proc_stop, 0, NULL, 0,
		       CTL_PROC, PROC_CURPROC, PROC_PID_STOPEXEC, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE|CTLFLAG_ANYWRITE,
		       CTLTYPE_INT, "stopexit",
		       SYSCTL_DESCR("Stop process before completing exit"),
		       sysctl_proc_stop, 0, NULL, 0,
		       CTL_PROC, PROC_CURPROC, PROC_PID_STOPEXIT, CTL_EOL);
}

void
uid_init(void)
{

	/*
	 * XXXSMP This could be at IPL_SOFTNET, but for now we want
	 * to to be deadlock free, so it must be at IPL_VM.
	 */
	mutex_init(&uihashtbl_lock, MUTEX_DRIVER, IPL_VM);

	/*
	 * Ensure that uid 0 is always in the user hash table, as
	 * sbreserve() expects it available from interrupt context.
	 */
	(void)uid_find(0);
}

struct uidinfo *
uid_find(uid_t uid)
{
	struct uidinfo *uip;
	struct uidinfo *newuip = NULL;
	struct uihashhead *uipp;

	uipp = UIHASH(uid);

again:
	mutex_enter(&uihashtbl_lock);
	LIST_FOREACH(uip, uipp, ui_hash)
		if (uip->ui_uid == uid) {
			mutex_exit(&uihashtbl_lock);
			if (newuip) {
				free(newuip, M_PROC);
				mutex_destroy(&newuip->ui_lock);
			}
			return uip;
		}
	if (newuip == NULL) {
		mutex_exit(&uihashtbl_lock);
		/* Must not be called from interrupt context. */
		newuip = malloc(sizeof(*uip), M_PROC, M_WAITOK | M_ZERO);
		mutex_init(&newuip->ui_lock, MUTEX_DRIVER, IPL_SOFTNET);
		goto again;
	}
	uip = newuip;

	LIST_INSERT_HEAD(uipp, uip, ui_hash);
	uip->ui_uid = uid;
	mutex_exit(&uihashtbl_lock);

	return uip;
}

/*
 * Change the count associated with number of processes
 * a given user is using.
 */
int
chgproccnt(uid_t uid, int diff)
{
	struct uidinfo *uip;

	if (diff == 0)
		return 0;

	uip = uid_find(uid);
	mutex_enter(&uip->ui_lock);
	uip->ui_proccnt += diff;
	KASSERT(uip->ui_proccnt >= 0);
	mutex_exit(&uip->ui_lock);
	return uip->ui_proccnt;
}

int
chgsbsize(struct uidinfo *uip, u_long *hiwat, u_long to, rlim_t xmax)
{
	rlim_t nsb;

	mutex_enter(&uip->ui_lock);
	nsb = uip->ui_sbsize + to - *hiwat;
	if (to > *hiwat && nsb > xmax) {
		mutex_exit(&uip->ui_lock);
		return 0;
	}
	*hiwat = to;
	uip->ui_sbsize = nsb;
	KASSERT(uip->ui_sbsize >= 0);
	mutex_exit(&uip->ui_lock);
	return 1;
}
