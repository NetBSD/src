/*	$NetBSD: kern_resource.c,v 1.131 2008/01/23 15:04:39 elad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: kern_resource.c,v 1.131 2008/01/23 15:04:39 elad Exp $");

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
#include <sys/timevar.h>
#include <sys/kauth.h>
#include <sys/atomic.h>
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

static pool_cache_t plimit_cache;
static pool_cache_t pstats_cache;

void
resource_init(void)
{

	plimit_cache = pool_cache_init(sizeof(struct plimit), 0, 0, 0,
	    "plimitpl", NULL, IPL_NONE, NULL, NULL, NULL);
	pstats_cache = pool_cache_init(sizeof(struct pstats), 0, 0, 0,
	    "pstatspl", NULL, IPL_NONE, NULL, NULL, NULL);
}

/*
 * Resource controls and accounting.
 */

int
sys_getpriority(struct lwp *l, const struct sys_getpriority_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) which;
		syscallarg(id_t) who;
	} */
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
sys_setpriority(struct lwp *l, const struct sys_setpriority_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) which;
		syscallarg(id_t) who;
		syscallarg(int) prio;
	} */
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
	mutex_spin_enter(&chgp->p_smutex);
	if (onice != chgp->p_nice) {
		mutex_spin_exit(&chgp->p_smutex);
		goto again;
	}
	sched_nice(chgp, n);
	mutex_spin_exit(&chgp->p_smutex);
	return (0);
}

/* ARGSUSED */
int
sys_setrlimit(struct lwp *l, const struct sys_setrlimit_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) which;
		syscallarg(const struct rlimit *) rlp;
	} */
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
	int error;

	if ((u_int)which >= RLIM_NLIMITS)
		return (EINVAL);

	if (limp->rlim_cur < 0 || limp->rlim_max < 0)
		return (EINVAL);

	if (limp->rlim_cur > limp->rlim_max) {
		/*
		 * This is programming error. According to SUSv2, we should
		 * return error in this case.
		 */
		return (EINVAL);
	}

	alimp = &p->p_rlimit[which];
	/* if we don't change the value, no need to limcopy() */
	if (limp->rlim_cur == alimp->rlim_cur &&
	    limp->rlim_max == alimp->rlim_max)
		return 0;

	error = kauth_authorize_process(l->l_cred, KAUTH_PROCESS_RLIMIT,
	    p, KAUTH_ARG(KAUTH_REQ_PROCESS_RLIMIT_SET), limp, KAUTH_ARG(which));
	if (error)
		return (error);

	lim_privatise(p, false);
	/* p->p_limit is now unchangeable */
	alimp = &p->p_rlimit[which];

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

	mutex_enter(&p->p_limit->pl_lock);
	*alimp = *limp;
	mutex_exit(&p->p_limit->pl_lock);
	return (0);
}

/* ARGSUSED */
int
sys_getrlimit(struct lwp *l, const struct sys_getrlimit_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) which;
		syscallarg(struct rlimit *) rlp;
	} */
	struct proc *p = l->l_proc;
	int which = SCARG(uap, which);
	struct rlimit rl;

	if ((u_int)which >= RLIM_NLIMITS)
		return (EINVAL);

	mutex_enter(&p->p_mutex);
	memcpy(&rl, &p->p_rlimit[which], sizeof(rl));
	mutex_exit(&p->p_mutex);

	return copyout(&rl, SCARG(uap, rlp), sizeof(rl));
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
	uint64_t u, st, ut, it, tot;
	struct lwp *l;
	struct bintime tm;
	struct timeval tv;

	mutex_spin_enter(&p->p_stmutex);
	st = p->p_sticks;
	ut = p->p_uticks;
	it = p->p_iticks;
	mutex_spin_exit(&p->p_stmutex);

	tm = p->p_rtime;

	LIST_FOREACH(l, &p->p_lwps, l_sibling) {
		lwp_lock(l);
		bintime_add(&tm, &l->l_rtime);
		if ((l->l_flag & LW_RUNNING) != 0) {
			struct bintime diff;
			/*
			 * Adjust for the current time slice.  This is
			 * actually fairly important since the error
			 * here is on the order of a time quantum,
			 * which is much greater than the sampling
			 * error.
			 */
			binuptime(&diff);
			bintime_sub(&diff, &l->l_stime);
			bintime_add(&tm, &diff);
		}
		lwp_unlock(l);
	}

	tot = st + ut + it;
	bintime2timeval(&tm, &tv);
	u = (uint64_t)tv.tv_sec * 1000000ul + tv.tv_usec;

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
		*rp = tv;
	}
}

/* ARGSUSED */
int
sys_getrusage(struct lwp *l, const struct sys_getrusage_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) who;
		syscallarg(struct rusage *) rusage;
	} */
	struct rusage ru;
	struct proc *p = l->l_proc;

	switch (SCARG(uap, who)) {
	case RUSAGE_SELF:
		mutex_enter(&p->p_smutex);
		memcpy(&ru, &p->p_stats->p_ru, sizeof(ru));
		calcru(p, &ru.ru_utime, &ru.ru_stime, NULL, NULL);
		mutex_exit(&p->p_smutex);
		break;

	case RUSAGE_CHILDREN:
		mutex_enter(&p->p_smutex);
		memcpy(&ru, &p->p_stats->p_cru, sizeof(ru));
		mutex_exit(&p->p_smutex);
		break;

	default:
		return EINVAL;
	}

	return copyout(&ru, SCARG(uap, rusage), sizeof(ru));
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
 * Unfortunately (due to PL_SHAREMOD) it is possibly for the structure
 * we are copying to change beneath our feet!
 */
struct plimit *
lim_copy(struct plimit *lim)
{
	struct plimit *newlim;
	char *corename;
	size_t alen, len;

	newlim = pool_cache_get(plimit_cache, PR_WAITOK);
	mutex_init(&newlim->pl_lock, MUTEX_DEFAULT, IPL_NONE);
	newlim->pl_flags = 0;
	newlim->pl_refcnt = 1;
	newlim->pl_sv_limit = NULL;

	mutex_enter(&lim->pl_lock);
	memcpy(newlim->pl_rlimit, lim->pl_rlimit,
	    sizeof(struct rlimit) * RLIM_NLIMITS);

	alen = 0;
	corename = NULL;
	for (;;) {
		if (lim->pl_corename == defcorename) {
			newlim->pl_corename = defcorename;
			break;
		}
		len = strlen(lim->pl_corename) + 1;
		if (len <= alen) {
			newlim->pl_corename = corename;
			memcpy(corename, lim->pl_corename, len);
			corename = NULL;
			break;
		}
		mutex_exit(&lim->pl_lock);
		if (corename != NULL)
			free(corename, M_TEMP);
		alen = len;
		corename = malloc(alen, M_TEMP, M_WAITOK);
		mutex_enter(&lim->pl_lock);
	}
	mutex_exit(&lim->pl_lock);
	if (corename != NULL)
		free(corename, M_TEMP);
	return newlim;
}

void
lim_addref(struct plimit *lim)
{
	atomic_inc_uint(&lim->pl_refcnt);
}

/*
 * Give a process it's own private plimit structure.
 * This will only be shared (in fork) if modifications are to be shared.
 */
void
lim_privatise(struct proc *p, bool set_shared)
{
	struct plimit *lim, *newlim;

	lim = p->p_limit;
	if (lim->pl_flags & PL_WRITEABLE) {
		if (set_shared)
			lim->pl_flags |= PL_SHAREMOD;
		return;
	}

	if (set_shared && lim->pl_flags & PL_SHAREMOD)
		return;

	newlim = lim_copy(lim);

	mutex_enter(&p->p_mutex);
	if (p->p_limit->pl_flags & PL_WRITEABLE) {
		/* Someone crept in while we were busy */
		mutex_exit(&p->p_mutex);
		limfree(newlim);
		if (set_shared)
			p->p_limit->pl_flags |= PL_SHAREMOD;
		return;
	}

	/*
	 * Since most accesses to p->p_limit aren't locked, we must not
	 * delete the old limit structure yet.
	 */
	newlim->pl_sv_limit = p->p_limit;
	newlim->pl_flags |= PL_WRITEABLE;
	if (set_shared)
		newlim->pl_flags |= PL_SHAREMOD;
	p->p_limit = newlim;
	mutex_exit(&p->p_mutex);
}

void
limfree(struct plimit *lim)
{
	struct plimit *sv_lim;

	do {
		if (atomic_dec_uint_nv(&lim->pl_refcnt) > 0)
			return;
		if (lim->pl_corename != defcorename)
			free(lim->pl_corename, M_TEMP);
		sv_lim = lim->pl_sv_limit;
		mutex_destroy(&lim->pl_lock);
		pool_cache_put(plimit_cache, lim);
	} while ((lim = sv_lim) != NULL);
}

struct pstats *
pstatscopy(struct pstats *ps)
{

	struct pstats *newps;

	newps = pool_cache_get(pstats_cache, PR_WAITOK);

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

	pool_cache_put(pstats_cache, ps);
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
	char *ocore;
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

	/* XXX-elad */
	error = kauth_authorize_process(l->l_cred, KAUTH_PROCESS_CANSEE, ptmp,
	    KAUTH_ARG(KAUTH_REQ_PROCESS_CANSEE_ENTRY), NULL, NULL);
	if (error)
		return (error);

	if (newp == NULL) {
		error = kauth_authorize_process(l->l_cred,
		    KAUTH_PROCESS_CORENAME, ptmp,
		    KAUTH_ARG(KAUTH_REQ_PROCESS_CORENAME_GET), NULL, NULL);
		if (error)
			return (error);
	}

	/*
	 * let them modify a temporary copy of the core name
	 */
	cname = PNBUF_GET();
	lim = ptmp->p_limit;
	mutex_enter(&lim->pl_lock);
	strlcpy(cname, lim->pl_corename, MAXPATHLEN);
	mutex_exit(&lim->pl_lock);

	node = *rnode;
	node.sysctl_data = cname;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	/*
	 * if that failed, or they have nothing new to say, or we've
	 * heard it before...
	 */
	if (error || newp == NULL)
		goto done;
	lim = ptmp->p_limit;
	mutex_enter(&lim->pl_lock);
	error = strcmp(cname, lim->pl_corename);
	mutex_exit(&lim->pl_lock);
	if (error == 0)
		/* Unchanged */
		goto done;

	error = kauth_authorize_process(l->l_cred, KAUTH_PROCESS_CORENAME,
	    ptmp, KAUTH_ARG(KAUTH_REQ_PROCESS_CORENAME_SET), cname, NULL);
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
	memcpy(tmp, cname, len + 1);

	lim_privatise(ptmp, false);
	lim = ptmp->p_limit;
	mutex_enter(&lim->pl_lock);
	ocore = lim->pl_corename;
	lim->pl_corename = tmp;
	mutex_exit(&lim->pl_lock);
	if (ocore != defcorename)
		free(ocore, M_TEMP);

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

	/* XXX-elad */
	error = kauth_authorize_process(l->l_cred, KAUTH_PROCESS_CANSEE, ptmp,
	    KAUTH_ARG(KAUTH_REQ_PROCESS_CANSEE_ENTRY), NULL, NULL);
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

	/* XXX-elad */
	error = kauth_authorize_process(l->l_cred, KAUTH_PROCESS_CANSEE, ptmp,
	    KAUTH_ARG(KAUTH_REQ_PROCESS_CANSEE_ENTRY), NULL, NULL);
	if (error)
		return (error);

	/* Check if we can view limits. */
	if (newp == NULL) {
		error = kauth_authorize_process(l->l_cred, KAUTH_PROCESS_RLIMIT,
		    ptmp, KAUTH_ARG(KAUTH_REQ_PROCESS_RLIMIT_GET), &alim,
		    KAUTH_ARG(which));
		if (error)
			return (error);
	}

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
	mutex_init(&uihashtbl_lock, MUTEX_DEFAULT, IPL_VM);

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
				mutex_destroy(&newuip->ui_lock);
				free(newuip, M_PROC);
			}
			return uip;
		}
	if (newuip == NULL) {
		mutex_exit(&uihashtbl_lock);
		/* Must not be called from interrupt context. */
		newuip = malloc(sizeof(*uip), M_PROC, M_WAITOK | M_ZERO);
		/* XXX this could be IPL_SOFTNET */
		mutex_init(&newuip->ui_lock, MUTEX_DEFAULT, IPL_VM);
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
