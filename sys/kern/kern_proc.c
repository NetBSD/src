/*	$NetBSD: kern_proc.c,v 1.147.2.1 2009/05/13 17:21:56 jym Exp $	*/

/*-
 * Copyright (c) 1999, 2006, 2007, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, and by Andrew Doran.
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

/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)kern_proc.c	8.7 (Berkeley) 2/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_proc.c,v 1.147.2.1 2009/05/13 17:21:56 jym Exp $");

#include "opt_kstack.h"
#include "opt_maxuprc.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/buf.h>
#include <sys/acct.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <ufs/ufs/quota.h>
#include <sys/uio.h>
#include <sys/pool.h>
#include <sys/pset.h>
#include <sys/mbuf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/signalvar.h>
#include <sys/ras.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/filedesc.h>
#include "sys/syscall_stats.h"
#include <sys/kauth.h>
#include <sys/sleepq.h>
#include <sys/atomic.h>
#include <sys/kmem.h>

#include <uvm/uvm.h>
#include <uvm/uvm_extern.h>

/*
 * Other process lists
 */

struct proclist allproc;
struct proclist zombproc;	/* resources have been freed */

kmutex_t	*proc_lock;

/*
 * pid to proc lookup is done by indexing the pid_table array.
 * Since pid numbers are only allocated when an empty slot
 * has been found, there is no need to search any lists ever.
 * (an orphaned pgrp will lock the slot, a session will lock
 * the pgrp with the same number.)
 * If the table is too small it is reallocated with twice the
 * previous size and the entries 'unzipped' into the two halves.
 * A linked list of free entries is passed through the pt_proc
 * field of 'free' items - set odd to be an invalid ptr.
 */

struct pid_table {
	struct proc	*pt_proc;
	struct pgrp	*pt_pgrp;
};
#if 1	/* strongly typed cast - should be a noop */
static inline uint p2u(struct proc *p) { return (uint)(uintptr_t)p; }
#else
#define p2u(p) ((uint)p)
#endif
#define P_VALID(p) (!(p2u(p) & 1))
#define P_NEXT(p) (p2u(p) >> 1)
#define P_FREE(pid) ((struct proc *)(uintptr_t)((pid) << 1 | 1))

#define INITIAL_PID_TABLE_SIZE	(1 << 5)
static struct pid_table *pid_table;
static uint pid_tbl_mask = INITIAL_PID_TABLE_SIZE - 1;
static uint pid_alloc_lim;	/* max we allocate before growing table */
static uint pid_alloc_cnt;	/* number of allocated pids */

/* links through free slots - never empty! */
static uint next_free_pt, last_free_pt;
static pid_t pid_max = PID_MAX;		/* largest value we allocate */

/* Components of the first process -- never freed. */

extern struct emul emul_netbsd;	/* defined in kern_exec.c */

struct session session0 = {
	.s_count = 1,
	.s_sid = 0,
};
struct pgrp pgrp0 = {
	.pg_members = LIST_HEAD_INITIALIZER(&pgrp0.pg_members),
	.pg_session = &session0,
};
filedesc_t filedesc0;
struct cwdinfo cwdi0 = {
	.cwdi_cmask = CMASK,		/* see cmask below */
	.cwdi_refcnt = 1,
};
struct plimit limit0;
struct pstats pstat0;
struct vmspace vmspace0;
struct sigacts sigacts0;
struct turnstile turnstile0;
struct proc proc0 = {
	.p_lwps = LIST_HEAD_INITIALIZER(&proc0.p_lwps),
	.p_sigwaiters = LIST_HEAD_INITIALIZER(&proc0.p_sigwaiters),
	.p_nlwps = 1,
	.p_nrlwps = 1,
	.p_nlwpid = 1,		/* must match lwp0.l_lid */
	.p_pgrp = &pgrp0,
	.p_comm = "system",
	/*
	 * Set P_NOCLDWAIT so that kernel threads are reparented to init(8)
	 * when they exit.  init(8) can easily wait them out for us.
	 */
	.p_flag = PK_SYSTEM | PK_NOCLDWAIT,
	.p_stat = SACTIVE,
	.p_nice = NZERO,
	.p_emul = &emul_netbsd,
	.p_cwdi = &cwdi0,
	.p_limit = &limit0,
	.p_fd = &filedesc0,
	.p_vmspace = &vmspace0,
	.p_stats = &pstat0,
	.p_sigacts = &sigacts0,
};
struct lwp lwp0 __aligned(MIN_LWP_ALIGNMENT) = {
#ifdef LWP0_CPU_INFO
	.l_cpu = LWP0_CPU_INFO,
#endif
	.l_proc = &proc0,
	.l_lid = 1,
	.l_flag = LW_INMEM | LW_SYSTEM,
	.l_stat = LSONPROC,
	.l_ts = &turnstile0,
	.l_syncobj = &sched_syncobj,
	.l_refcnt = 1,
	.l_priority = PRI_USER + NPRI_USER - 1,
	.l_inheritedprio = -1,
	.l_class = SCHED_OTHER,
	.l_psid = PS_NONE,
	.l_pi_lenders = SLIST_HEAD_INITIALIZER(&lwp0.l_pi_lenders),
	.l_name = __UNCONST("swapper"),
};
kauth_cred_t cred0;

extern struct user *proc0paddr;

int nofile = NOFILE;
int maxuprc = MAXUPRC;
int cmask = CMASK;

MALLOC_DEFINE(M_EMULDATA, "emuldata", "Per-process emulation data");
MALLOC_DEFINE(M_SUBPROC, "subproc", "Proc sub-structures");

/*
 * The process list descriptors, used during pid allocation and
 * by sysctl.  No locking on this data structure is needed since
 * it is completely static.
 */
const struct proclist_desc proclists[] = {
	{ &allproc	},
	{ &zombproc	},
	{ NULL		},
};

static struct pgrp *	pg_remove(pid_t);
static void		pg_delete(pid_t);
static void		orphanpg(struct pgrp *);

static specificdata_domain_t proc_specificdata_domain;

static pool_cache_t proc_cache;

/*
 * Initialize global process hashing structures.
 */
void
procinit(void)
{
	const struct proclist_desc *pd;
	u_int i;
#define	LINK_EMPTY ((PID_MAX + INITIAL_PID_TABLE_SIZE) & ~(INITIAL_PID_TABLE_SIZE - 1))

	for (pd = proclists; pd->pd_list != NULL; pd++)
		LIST_INIT(pd->pd_list);

	proc_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_NONE);
	pid_table = kmem_alloc(INITIAL_PID_TABLE_SIZE
	    * sizeof(struct pid_table), KM_SLEEP);

	/* Set free list running through table...
	   Preset 'use count' above PID_MAX so we allocate pid 1 next. */
	for (i = 0; i <= pid_tbl_mask; i++) {
		pid_table[i].pt_proc = P_FREE(LINK_EMPTY + i + 1);
		pid_table[i].pt_pgrp = 0;
	}
	/* slot 0 is just grabbed */
	next_free_pt = 1;
	/* Need to fix last entry. */
	last_free_pt = pid_tbl_mask;
	pid_table[last_free_pt].pt_proc = P_FREE(LINK_EMPTY);
	/* point at which we grow table - to avoid reusing pids too often */
	pid_alloc_lim = pid_tbl_mask - 1;
#undef LINK_EMPTY

	proc_specificdata_domain = specificdata_domain_create();
	KASSERT(proc_specificdata_domain != NULL);

	proc_cache = pool_cache_init(sizeof(struct proc), 0, 0, 0,
	    "procpl", NULL, IPL_NONE, NULL, NULL, NULL);
}

/*
 * Initialize process 0.
 */
void
proc0_init(void)
{
	struct proc *p;
	struct pgrp *pg;
	struct session *sess;
	struct lwp *l;
	rlim_t lim;
	int i;

	p = &proc0;
	pg = &pgrp0;
	sess = &session0;
	l = &lwp0;

	KASSERT(l->l_lid == p->p_nlwpid);

	mutex_init(&p->p_stmutex, MUTEX_DEFAULT, IPL_HIGH);
	mutex_init(&p->p_auxlock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&l->l_swaplock, MUTEX_DEFAULT, IPL_NONE);
	p->p_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_NONE);

	rw_init(&p->p_reflock);
	cv_init(&p->p_waitcv, "wait");
	cv_init(&p->p_lwpcv, "lwpwait");

	LIST_INSERT_HEAD(&p->p_lwps, l, l_sibling);

	pid_table[0].pt_proc = p;
	LIST_INSERT_HEAD(&allproc, p, p_list);
	LIST_INSERT_HEAD(&alllwp, l, l_list);

	pid_table[0].pt_pgrp = pg;
	LIST_INSERT_HEAD(&pg->pg_members, p, p_pglist);

#ifdef __HAVE_SYSCALL_INTERN
	(*p->p_emul->e_syscall_intern)(p);
#endif

	callout_init(&l->l_timeout_ch, CALLOUT_MPSAFE);
	callout_setfunc(&l->l_timeout_ch, sleepq_timeout, l);
	cv_init(&l->l_sigcv, "sigwait");

	/* Create credentials. */
	cred0 = kauth_cred_alloc();
	p->p_cred = cred0;
	kauth_cred_hold(cred0);
	l->l_cred = cred0;

	/* Create the CWD info. */
	rw_init(&cwdi0.cwdi_lock);

	/* Create the limits structures. */
	mutex_init(&limit0.pl_lock, MUTEX_DEFAULT, IPL_NONE);
	for (i = 0; i < __arraycount(limit0.pl_rlimit); i++)
		limit0.pl_rlimit[i].rlim_cur =	 
		    limit0.pl_rlimit[i].rlim_max = RLIM_INFINITY;

	limit0.pl_rlimit[RLIMIT_NOFILE].rlim_max = maxfiles;
	limit0.pl_rlimit[RLIMIT_NOFILE].rlim_cur =
	    maxfiles < nofile ? maxfiles : nofile;

	limit0.pl_rlimit[RLIMIT_NPROC].rlim_max = maxproc;
	limit0.pl_rlimit[RLIMIT_NPROC].rlim_cur =
	    maxproc < maxuprc ? maxproc : maxuprc;

	lim = ptoa(uvmexp.free);
	limit0.pl_rlimit[RLIMIT_RSS].rlim_max = lim;
	limit0.pl_rlimit[RLIMIT_MEMLOCK].rlim_max = lim;
	limit0.pl_rlimit[RLIMIT_MEMLOCK].rlim_cur = lim / 3;
	limit0.pl_corename = defcorename;	 
	limit0.pl_refcnt = 1;	 
	limit0.pl_sv_limit = NULL;

	/* Configure virtual memory system, set vm rlimits. */
	uvm_init_limits(p);

	/* Initialize file descriptor table for proc0. */
	fd_init(&filedesc0);

	/*
	 * Initialize proc0's vmspace, which uses the kernel pmap.
	 * All kernel processes (which never have user space mappings)
	 * share proc0's vmspace, and thus, the kernel pmap.
	 */
	uvmspace_init(&vmspace0, pmap_kernel(), round_page(VM_MIN_ADDRESS),
	    trunc_page(VM_MAX_ADDRESS));

	l->l_addr = proc0paddr;				/* XXX */

	/* Initialize signal state for proc0. XXX IPL_SCHED */
	mutex_init(&p->p_sigacts->sa_mutex, MUTEX_DEFAULT, IPL_SCHED);
	siginit(p);

	proc_initspecific(p);
	lwp_initspecific(l);

	SYSCALL_TIME_LWP_INIT(l);
}

/*
 * Session reference counting.
 */

void
proc_sesshold(struct session *ss)
{

	KASSERT(mutex_owned(proc_lock));
	ss->s_count++;
}

void
proc_sessrele(struct session *ss)
{

	KASSERT(mutex_owned(proc_lock));
	/*
	 * We keep the pgrp with the same id as the session in order to
	 * stop a process being given the same pid.  Since the pgrp holds
	 * a reference to the session, it must be a 'zombie' pgrp by now.
	 */
	if (--ss->s_count == 0) {
		struct pgrp *pg;

		pg = pg_remove(ss->s_sid);
		mutex_exit(proc_lock);

		kmem_free(pg, sizeof(struct pgrp));
		kmem_free(ss, sizeof(struct session));
	} else {
		mutex_exit(proc_lock);
	}
}

/*
 * Check that the specified process group is in the session of the
 * specified process.
 * Treats -ve ids as process ids.
 * Used to validate TIOCSPGRP requests.
 */
int
pgid_in_session(struct proc *p, pid_t pg_id)
{
	struct pgrp *pgrp;
	struct session *session;
	int error;

	mutex_enter(proc_lock);
	if (pg_id < 0) {
		struct proc *p1 = p_find(-pg_id, PFIND_LOCKED | PFIND_UNLOCK_FAIL);
		if (p1 == NULL)
			return EINVAL;
		pgrp = p1->p_pgrp;
	} else {
		pgrp = pg_find(pg_id, PFIND_LOCKED | PFIND_UNLOCK_FAIL);
		if (pgrp == NULL)
			return EINVAL;
	}
	session = pgrp->pg_session;
	if (session != p->p_pgrp->pg_session)
		error = EPERM;
	else
		error = 0;
	mutex_exit(proc_lock);

	return error;
}

/*
 * p_inferior: is p an inferior of q?
 */
static inline bool
p_inferior(struct proc *p, struct proc *q)
{

	KASSERT(mutex_owned(proc_lock));

	for (; p != q; p = p->p_pptr)
		if (p->p_pid == 0)
			return false;
	return true;
}

/*
 * Locate a process by number
 */
struct proc *
p_find(pid_t pid, uint flags)
{
	struct proc *p;
	char stat;

	if (!(flags & PFIND_LOCKED))
		mutex_enter(proc_lock);

	p = pid_table[pid & pid_tbl_mask].pt_proc;

	/* Only allow live processes to be found by pid. */
	/* XXXSMP p_stat */
	if (P_VALID(p) && p->p_pid == pid && ((stat = p->p_stat) == SACTIVE ||
	    stat == SSTOP || ((flags & PFIND_ZOMBIE) &&
	    (stat == SZOMB || stat == SDEAD || stat == SDYING)))) {
		if (flags & PFIND_UNLOCK_OK)
			 mutex_exit(proc_lock);
		return p;
	}
	if (flags & PFIND_UNLOCK_FAIL)
		mutex_exit(proc_lock);
	return NULL;
}


/*
 * Locate a process group by number
 */
struct pgrp *
pg_find(pid_t pgid, uint flags)
{
	struct pgrp *pg;

	if (!(flags & PFIND_LOCKED))
		mutex_enter(proc_lock);
	pg = pid_table[pgid & pid_tbl_mask].pt_pgrp;
	/*
	 * Can't look up a pgrp that only exists because the session
	 * hasn't died yet (traditional)
	 */
	if (pg == NULL || pg->pg_id != pgid || LIST_EMPTY(&pg->pg_members)) {
		if (flags & PFIND_UNLOCK_FAIL)
			 mutex_exit(proc_lock);
		return NULL;
	}

	if (flags & PFIND_UNLOCK_OK)
		mutex_exit(proc_lock);
	return pg;
}

static void
expand_pid_table(void)
{
	size_t pt_size, tsz;
	struct pid_table *n_pt, *new_pt;
	struct proc *proc;
	struct pgrp *pgrp;
	pid_t pid;
	u_int i;

	pt_size = pid_tbl_mask + 1;
	tsz = pt_size * 2 * sizeof(struct pid_table);
	new_pt = kmem_alloc(tsz, KM_SLEEP);

	mutex_enter(proc_lock);
	if (pt_size != pid_tbl_mask + 1) {
		/* Another process beat us to it... */
		mutex_exit(proc_lock);
		kmem_free(new_pt, tsz);
		return;
	}

	/*
	 * Copy entries from old table into new one.
	 * If 'pid' is 'odd' we need to place in the upper half,
	 * even pid's to the lower half.
	 * Free items stay in the low half so we don't have to
	 * fixup the reference to them.
	 * We stuff free items on the front of the freelist
	 * because we can't write to unmodified entries.
	 * Processing the table backwards maintains a semblance
	 * of issueing pid numbers that increase with time.
	 */
	i = pt_size - 1;
	n_pt = new_pt + i;
	for (; ; i--, n_pt--) {
		proc = pid_table[i].pt_proc;
		pgrp = pid_table[i].pt_pgrp;
		if (!P_VALID(proc)) {
			/* Up 'use count' so that link is valid */
			pid = (P_NEXT(proc) + pt_size) & ~pt_size;
			proc = P_FREE(pid);
			if (pgrp)
				pid = pgrp->pg_id;
		} else
			pid = proc->p_pid;

		/* Save entry in appropriate half of table */
		n_pt[pid & pt_size].pt_proc = proc;
		n_pt[pid & pt_size].pt_pgrp = pgrp;

		/* Put other piece on start of free list */
		pid = (pid ^ pt_size) & ~pid_tbl_mask;
		n_pt[pid & pt_size].pt_proc =
				    P_FREE((pid & ~pt_size) | next_free_pt);
		n_pt[pid & pt_size].pt_pgrp = 0;
		next_free_pt = i | (pid & pt_size);
		if (i == 0)
			break;
	}

	/* Save old table size and switch tables */
	tsz = pt_size * sizeof(struct pid_table);
	n_pt = pid_table;
	pid_table = new_pt;
	pid_tbl_mask = pt_size * 2 - 1;

	/*
	 * pid_max starts as PID_MAX (= 30000), once we have 16384
	 * allocated pids we need it to be larger!
	 */
	if (pid_tbl_mask > PID_MAX) {
		pid_max = pid_tbl_mask * 2 + 1;
		pid_alloc_lim |= pid_alloc_lim << 1;
	} else
		pid_alloc_lim <<= 1;	/* doubles number of free slots... */

	mutex_exit(proc_lock);
	kmem_free(n_pt, tsz);
}

struct proc *
proc_alloc(void)
{
	struct proc *p;
	int nxt;
	pid_t pid;
	struct pid_table *pt;

	p = pool_cache_get(proc_cache, PR_WAITOK);
	p->p_stat = SIDL;			/* protect against others */

	proc_initspecific(p);
	/* allocate next free pid */

	for (;;expand_pid_table()) {
		if (__predict_false(pid_alloc_cnt >= pid_alloc_lim))
			/* ensure pids cycle through 2000+ values */
			continue;
		mutex_enter(proc_lock);
		pt = &pid_table[next_free_pt];
#ifdef DIAGNOSTIC
		if (__predict_false(P_VALID(pt->pt_proc) || pt->pt_pgrp))
			panic("proc_alloc: slot busy");
#endif
		nxt = P_NEXT(pt->pt_proc);
		if (nxt & pid_tbl_mask)
			break;
		/* Table full - expand (NB last entry not used....) */
		mutex_exit(proc_lock);
	}

	/* pid is 'saved use count' + 'size' + entry */
	pid = (nxt & ~pid_tbl_mask) + pid_tbl_mask + 1 + next_free_pt;
	if ((uint)pid > (uint)pid_max)
		pid &= pid_tbl_mask;
	p->p_pid = pid;
	next_free_pt = nxt & pid_tbl_mask;

	/* Grab table slot */
	pt->pt_proc = p;
	pid_alloc_cnt++;

	mutex_exit(proc_lock);

	return p;
}

/*
 * Free a process id - called from proc_free (in kern_exit.c)
 *
 * Called with the proc_lock held.
 */
void
proc_free_pid(struct proc *p)
{
	pid_t pid = p->p_pid;
	struct pid_table *pt;

	KASSERT(mutex_owned(proc_lock));

	pt = &pid_table[pid & pid_tbl_mask];
#ifdef DIAGNOSTIC
	if (__predict_false(pt->pt_proc != p))
		panic("proc_free: pid_table mismatch, pid %x, proc %p",
			pid, p);
#endif
	/* save pid use count in slot */
	pt->pt_proc = P_FREE(pid & ~pid_tbl_mask);

	if (pt->pt_pgrp == NULL) {
		/* link last freed entry onto ours */
		pid &= pid_tbl_mask;
		pt = &pid_table[last_free_pt];
		pt->pt_proc = P_FREE(P_NEXT(pt->pt_proc) | pid);
		last_free_pt = pid;
		pid_alloc_cnt--;
	}

	atomic_dec_uint(&nprocs);
}

void
proc_free_mem(struct proc *p)
{

	pool_cache_put(proc_cache, p);
}

/*
 * proc_enterpgrp: move p to a new or existing process group (and session).
 *
 * If we are creating a new pgrp, the pgid should equal
 * the calling process' pid.
 * If is only valid to enter a process group that is in the session
 * of the process.
 * Also mksess should only be set if we are creating a process group
 *
 * Only called from sys_setsid and sys_setpgid.
 */
int
proc_enterpgrp(struct proc *curp, pid_t pid, pid_t pgid, bool mksess)
{
	struct pgrp *new_pgrp, *pgrp;
	struct session *sess;
	struct proc *p;
	int rval;
	pid_t pg_id = NO_PGID;

	sess = mksess ? kmem_alloc(sizeof(*sess), KM_SLEEP) : NULL;

	/* Allocate data areas we might need before doing any validity checks */
	mutex_enter(proc_lock);		/* Because pid_table might change */
	if (pid_table[pgid & pid_tbl_mask].pt_pgrp == 0) {
		mutex_exit(proc_lock);
		new_pgrp = kmem_alloc(sizeof(*new_pgrp), KM_SLEEP);
		mutex_enter(proc_lock);
	} else
		new_pgrp = NULL;
	rval = EPERM;	/* most common error (to save typing) */

	/* Check pgrp exists or can be created */
	pgrp = pid_table[pgid & pid_tbl_mask].pt_pgrp;
	if (pgrp != NULL && pgrp->pg_id != pgid)
		goto done;

	/* Can only set another process under restricted circumstances. */
	if (pid != curp->p_pid) {
		/* must exist and be one of our children... */
		if ((p = p_find(pid, PFIND_LOCKED)) == NULL ||
		    !p_inferior(p, curp)) {
			rval = ESRCH;
			goto done;
		}
		/* ... in the same session... */
		if (sess != NULL || p->p_session != curp->p_session)
			goto done;
		/* ... existing pgid must be in same session ... */
		if (pgrp != NULL && pgrp->pg_session != p->p_session)
			goto done;
		/* ... and not done an exec. */
		if (p->p_flag & PK_EXEC) {
			rval = EACCES;
			goto done;
		}
	} else {
		/* ... setsid() cannot re-enter a pgrp */
		if (mksess && (curp->p_pgid == curp->p_pid ||
		    pg_find(curp->p_pid, PFIND_LOCKED)))
			goto done;
		p = curp;
	}

	/* Changing the process group/session of a session
	   leader is definitely off limits. */
	if (SESS_LEADER(p)) {
		if (sess == NULL && p->p_pgrp == pgrp)
			/* unless it's a definite noop */
			rval = 0;
		goto done;
	}

	/* Can only create a process group with id of process */
	if (pgrp == NULL && pgid != pid)
		goto done;

	/* Can only create a session if creating pgrp */
	if (sess != NULL && pgrp != NULL)
		goto done;

	/* Check we allocated memory for a pgrp... */
	if (pgrp == NULL && new_pgrp == NULL)
		goto done;

	/* Don't attach to 'zombie' pgrp */
	if (pgrp != NULL && LIST_EMPTY(&pgrp->pg_members))
		goto done;

	/* Expect to succeed now */
	rval = 0;

	if (pgrp == p->p_pgrp)
		/* nothing to do */
		goto done;

	/* Ok all setup, link up required structures */

	if (pgrp == NULL) {
		pgrp = new_pgrp;
		new_pgrp = NULL;
		if (sess != NULL) {
			sess->s_sid = p->p_pid;
			sess->s_leader = p;
			sess->s_count = 1;
			sess->s_ttyvp = NULL;
			sess->s_ttyp = NULL;
			sess->s_flags = p->p_session->s_flags & ~S_LOGIN_SET;
			memcpy(sess->s_login, p->p_session->s_login,
			    sizeof(sess->s_login));
			p->p_lflag &= ~PL_CONTROLT;
		} else {
			sess = p->p_pgrp->pg_session;
			proc_sesshold(sess);
		}
		pgrp->pg_session = sess;
		sess = NULL;

		pgrp->pg_id = pgid;
		LIST_INIT(&pgrp->pg_members);
#ifdef DIAGNOSTIC
		if (__predict_false(pid_table[pgid & pid_tbl_mask].pt_pgrp))
			panic("enterpgrp: pgrp table slot in use");
		if (__predict_false(mksess && p != curp))
			panic("enterpgrp: mksession and p != curproc");
#endif
		pid_table[pgid & pid_tbl_mask].pt_pgrp = pgrp;
		pgrp->pg_jobc = 0;
	}

	/*
	 * Adjust eligibility of affected pgrps to participate in job control.
	 * Increment eligibility counts before decrementing, otherwise we
	 * could reach 0 spuriously during the first call.
	 */
	fixjobc(p, pgrp, 1);
	fixjobc(p, p->p_pgrp, 0);

	/* Interlock with ttread(). */
	mutex_spin_enter(&tty_lock);

	/* Move process to requested group. */
	LIST_REMOVE(p, p_pglist);
	if (LIST_EMPTY(&p->p_pgrp->pg_members))
		/* defer delete until we've dumped the lock */
		pg_id = p->p_pgrp->pg_id;
	p->p_pgrp = pgrp;
	LIST_INSERT_HEAD(&pgrp->pg_members, p, p_pglist);

	/* Done with the swap; we can release the tty mutex. */
	mutex_spin_exit(&tty_lock);

    done:
	if (pg_id != NO_PGID) {
		/* Releases proc_lock. */
		pg_delete(pg_id);
	} else {
		mutex_exit(proc_lock);
	}
	if (sess != NULL)
		kmem_free(sess, sizeof(*sess));
	if (new_pgrp != NULL)
		kmem_free(new_pgrp, sizeof(*new_pgrp));
#ifdef DEBUG_PGRP
	if (__predict_false(rval))
		printf("enterpgrp(%d,%d,%d), curproc %d, rval %d\n",
			pid, pgid, mksess, curp->p_pid, rval);
#endif
	return rval;
}

/*
 * proc_leavepgrp: remove a process from its process group.
 *  => must be called with the proc_lock held, which will be released;
 */
void
proc_leavepgrp(struct proc *p)
{
	struct pgrp *pgrp;

	KASSERT(mutex_owned(proc_lock));

	/* Interlock with ttread() */
	mutex_spin_enter(&tty_lock);
	pgrp = p->p_pgrp;
	LIST_REMOVE(p, p_pglist);
	p->p_pgrp = NULL;
	mutex_spin_exit(&tty_lock);

	if (LIST_EMPTY(&pgrp->pg_members)) {
		/* Releases proc_lock. */
		pg_delete(pgrp->pg_id);
	} else {
		mutex_exit(proc_lock);
	}
}

/*
 * pg_remove: remove a process group from the table.
 *  => must be called with the proc_lock held;
 *  => returns process group to free;
 */
static struct pgrp *
pg_remove(pid_t pg_id)
{
	struct pgrp *pgrp;
	struct pid_table *pt;

	KASSERT(mutex_owned(proc_lock));

	pt = &pid_table[pg_id & pid_tbl_mask];
	pgrp = pt->pt_pgrp;

	KASSERT(pgrp != NULL);
	KASSERT(pgrp->pg_id == pg_id);
	KASSERT(LIST_EMPTY(&pgrp->pg_members));

	pt->pt_pgrp = NULL;

	if (!P_VALID(pt->pt_proc)) {
		/* Orphaned pgrp, put slot onto free list. */
		KASSERT((P_NEXT(pt->pt_proc) & pid_tbl_mask) == 0);
		pg_id &= pid_tbl_mask;
		pt = &pid_table[last_free_pt];
		pt->pt_proc = P_FREE(P_NEXT(pt->pt_proc) | pg_id);
		last_free_pt = pg_id;
		pid_alloc_cnt--;
	}
	return pgrp;
}

/*
 * pg_delete: delete and free a process group.
 *  => must be called with the proc_lock held, which will be released.
 */
static void
pg_delete(pid_t pg_id)
{
	struct pgrp *pg;
	struct tty *ttyp;
	struct session *ss;

	KASSERT(mutex_owned(proc_lock));

	pg = pid_table[pg_id & pid_tbl_mask].pt_pgrp;
	if (pg == NULL || pg->pg_id != pg_id || !LIST_EMPTY(&pg->pg_members)) {
		mutex_exit(proc_lock);
		return;
	}

	ss = pg->pg_session;

	/* Remove reference (if any) from tty to this process group */
	mutex_spin_enter(&tty_lock);
	ttyp = ss->s_ttyp;
	if (ttyp != NULL && ttyp->t_pgrp == pg) {
		ttyp->t_pgrp = NULL;
		KASSERT(ttyp->t_session == ss);
	}
	mutex_spin_exit(&tty_lock);

	/*
	 * The leading process group in a session is freed by proc_sessrele(),
	 * if last reference.  Note: proc_sessrele() releases proc_lock.
	 */
	pg = (ss->s_sid != pg->pg_id) ? pg_remove(pg_id) : NULL;
	proc_sessrele(ss);

	if (pg != NULL) {
		/* Free it, if was not done by proc_sessrele(). */
		kmem_free(pg, sizeof(struct pgrp));
	}
}

/*
 * Adjust pgrp jobc counters when specified process changes process group.
 * We count the number of processes in each process group that "qualify"
 * the group for terminal job control (those with a parent in a different
 * process group of the same session).  If that count reaches zero, the
 * process group becomes orphaned.  Check both the specified process'
 * process group and that of its children.
 * entering == 0 => p is leaving specified group.
 * entering == 1 => p is entering specified group.
 *
 * Call with proc_lock held.
 */
void
fixjobc(struct proc *p, struct pgrp *pgrp, int entering)
{
	struct pgrp *hispgrp;
	struct session *mysession = pgrp->pg_session;
	struct proc *child;

	KASSERT(mutex_owned(proc_lock));

	/*
	 * Check p's parent to see whether p qualifies its own process
	 * group; if so, adjust count for p's process group.
	 */
	hispgrp = p->p_pptr->p_pgrp;
	if (hispgrp != pgrp && hispgrp->pg_session == mysession) {
		if (entering) {
			pgrp->pg_jobc++;
			p->p_lflag &= ~PL_ORPHANPG;
		} else if (--pgrp->pg_jobc == 0)
			orphanpg(pgrp);
	}

	/*
	 * Check this process' children to see whether they qualify
	 * their process groups; if so, adjust counts for children's
	 * process groups.
	 */
	LIST_FOREACH(child, &p->p_children, p_sibling) {
		hispgrp = child->p_pgrp;
		if (hispgrp != pgrp && hispgrp->pg_session == mysession &&
		    !P_ZOMBIE(child)) {
			if (entering) {
				child->p_lflag &= ~PL_ORPHANPG;
				hispgrp->pg_jobc++;
			} else if (--hispgrp->pg_jobc == 0)
				orphanpg(hispgrp);
		}
	}
}

/*
 * A process group has become orphaned;
 * if there are any stopped processes in the group,
 * hang-up all process in that group.
 *
 * Call with proc_lock held.
 */
static void
orphanpg(struct pgrp *pg)
{
	struct proc *p;
	int doit;

	KASSERT(mutex_owned(proc_lock));

	doit = 0;

	LIST_FOREACH(p, &pg->pg_members, p_pglist) {
		if (p->p_stat == SSTOP) {
			p->p_lflag |= PL_ORPHANPG;
			psignal(p, SIGHUP);
			psignal(p, SIGCONT);
		}
	}
}

#ifdef DDB
#include <ddb/db_output.h>
void pidtbl_dump(void);
void
pidtbl_dump(void)
{
	struct pid_table *pt;
	struct proc *p;
	struct pgrp *pgrp;
	int id;

	db_printf("pid table %p size %x, next %x, last %x\n",
		pid_table, pid_tbl_mask+1,
		next_free_pt, last_free_pt);
	for (pt = pid_table, id = 0; id <= pid_tbl_mask; id++, pt++) {
		p = pt->pt_proc;
		if (!P_VALID(p) && !pt->pt_pgrp)
			continue;
		db_printf("  id %x: ", id);
		if (P_VALID(p))
			db_printf("proc %p id %d (0x%x) %s\n",
				p, p->p_pid, p->p_pid, p->p_comm);
		else
			db_printf("next %x use %x\n",
				P_NEXT(p) & pid_tbl_mask,
				P_NEXT(p) & ~pid_tbl_mask);
		if ((pgrp = pt->pt_pgrp)) {
			db_printf("\tsession %p, sid %d, count %d, login %s\n",
			    pgrp->pg_session, pgrp->pg_session->s_sid,
			    pgrp->pg_session->s_count,
			    pgrp->pg_session->s_login);
			db_printf("\tpgrp %p, pg_id %d, pg_jobc %d, members %p\n",
			    pgrp, pgrp->pg_id, pgrp->pg_jobc,
			    LIST_FIRST(&pgrp->pg_members));
			LIST_FOREACH(p, &pgrp->pg_members, p_pglist) {
				db_printf("\t\tpid %d addr %p pgrp %p %s\n",
				    p->p_pid, p, p->p_pgrp, p->p_comm);
			}
		}
	}
}
#endif /* DDB */

#ifdef KSTACK_CHECK_MAGIC
#include <sys/user.h>

#define	KSTACK_MAGIC	0xdeadbeaf

/* XXX should be per process basis? */
static int	kstackleftmin = KSTACK_SIZE;
static int	kstackleftthres = KSTACK_SIZE / 8;

void
kstack_setup_magic(const struct lwp *l)
{
	uint32_t *ip;
	uint32_t const *end;

	KASSERT(l != NULL);
	KASSERT(l != &lwp0);

	/*
	 * fill all the stack with magic number
	 * so that later modification on it can be detected.
	 */
	ip = (uint32_t *)KSTACK_LOWEST_ADDR(l);
	end = (uint32_t *)((char *)KSTACK_LOWEST_ADDR(l) + KSTACK_SIZE);
	for (; ip < end; ip++) {
		*ip = KSTACK_MAGIC;
	}
}

void
kstack_check_magic(const struct lwp *l)
{
	uint32_t const *ip, *end;
	int stackleft;

	KASSERT(l != NULL);

	/* don't check proc0 */ /*XXX*/
	if (l == &lwp0)
		return;

#ifdef __MACHINE_STACK_GROWS_UP
	/* stack grows upwards (eg. hppa) */
	ip = (uint32_t *)((void *)KSTACK_LOWEST_ADDR(l) + KSTACK_SIZE);
	end = (uint32_t *)KSTACK_LOWEST_ADDR(l);
	for (ip--; ip >= end; ip--)
		if (*ip != KSTACK_MAGIC)
			break;

	stackleft = (void *)KSTACK_LOWEST_ADDR(l) + KSTACK_SIZE - (void *)ip;
#else /* __MACHINE_STACK_GROWS_UP */
	/* stack grows downwards (eg. i386) */
	ip = (uint32_t *)KSTACK_LOWEST_ADDR(l);
	end = (uint32_t *)((char *)KSTACK_LOWEST_ADDR(l) + KSTACK_SIZE);
	for (; ip < end; ip++)
		if (*ip != KSTACK_MAGIC)
			break;

	stackleft = ((const char *)ip) - (const char *)KSTACK_LOWEST_ADDR(l);
#endif /* __MACHINE_STACK_GROWS_UP */

	if (kstackleftmin > stackleft) {
		kstackleftmin = stackleft;
		if (stackleft < kstackleftthres)
			printf("warning: kernel stack left %d bytes"
			    "(pid %u:lid %u)\n", stackleft,
			    (u_int)l->l_proc->p_pid, (u_int)l->l_lid);
	}

	if (stackleft <= 0) {
		panic("magic on the top of kernel stack changed for "
		    "pid %u, lid %u: maybe kernel stack overflow",
		    (u_int)l->l_proc->p_pid, (u_int)l->l_lid);
	}
}
#endif /* KSTACK_CHECK_MAGIC */

int
proclist_foreach_call(struct proclist *list,
    int (*callback)(struct proc *, void *arg), void *arg)
{
	struct proc marker;
	struct proc *p;
	struct lwp * const l = curlwp;
	int ret = 0;

	marker.p_flag = PK_MARKER;
	uvm_lwp_hold(l);
	mutex_enter(proc_lock);
	for (p = LIST_FIRST(list); ret == 0 && p != NULL;) {
		if (p->p_flag & PK_MARKER) {
			p = LIST_NEXT(p, p_list);
			continue;
		}
		LIST_INSERT_AFTER(p, &marker, p_list);
		ret = (*callback)(p, arg);
		KASSERT(mutex_owned(proc_lock));
		p = LIST_NEXT(&marker, p_list);
		LIST_REMOVE(&marker, p_list);
	}
	mutex_exit(proc_lock);
	uvm_lwp_rele(l);

	return ret;
}

int
proc_vmspace_getref(struct proc *p, struct vmspace **vm)
{

	/* XXXCDC: how should locking work here? */

	/* curproc exception is for coredump. */

	if ((p != curproc && (p->p_sflag & PS_WEXIT) != 0) ||
	    (p->p_vmspace->vm_refcnt < 1)) { /* XXX */
		return EFAULT;
	}

	uvmspace_addref(p->p_vmspace);
	*vm = p->p_vmspace;

	return 0;
}

/*
 * Acquire a write lock on the process credential.
 */
void 
proc_crmod_enter(void)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct plimit *lim;
	kauth_cred_t oc;
	char *cn;

	/* Reset what needs to be reset in plimit. */
	if (p->p_limit->pl_corename != defcorename) {
		lim_privatise(p, false);
		lim = p->p_limit;
		mutex_enter(&lim->pl_lock);
		cn = lim->pl_corename;
		lim->pl_corename = defcorename;
		mutex_exit(&lim->pl_lock);
		if (cn != defcorename)
			free(cn, M_TEMP);
	}

	mutex_enter(p->p_lock);

	/* Ensure the LWP cached credentials are up to date. */
	if ((oc = l->l_cred) != p->p_cred) {
		kauth_cred_hold(p->p_cred);
		l->l_cred = p->p_cred;
		kauth_cred_free(oc);
	}

}

/*
 * Set in a new process credential, and drop the write lock.  The credential
 * must have a reference already.  Optionally, free a no-longer required
 * credential.  The scheduler also needs to inspect p_cred, so we also
 * briefly acquire the sched state mutex.
 */
void
proc_crmod_leave(kauth_cred_t scred, kauth_cred_t fcred, bool sugid)
{
	struct lwp *l = curlwp, *l2;
	struct proc *p = l->l_proc;
	kauth_cred_t oc;

	KASSERT(mutex_owned(p->p_lock));

	/* Is there a new credential to set in? */
	if (scred != NULL) {
		p->p_cred = scred;
		LIST_FOREACH(l2, &p->p_lwps, l_sibling) {
			if (l2 != l)
				l2->l_prflag |= LPR_CRMOD;
		}

		/* Ensure the LWP cached credentials are up to date. */
		if ((oc = l->l_cred) != scred) {
			kauth_cred_hold(scred);
			l->l_cred = scred;
		}
	} else
		oc = NULL;	/* XXXgcc */

	if (sugid) {
		/*
		 * Mark process as having changed credentials, stops
		 * tracing etc.
		 */
		p->p_flag |= PK_SUGID;
	}

	mutex_exit(p->p_lock);

	/* If there is a credential to be released, free it now. */
	if (fcred != NULL) {
		KASSERT(scred != NULL);
		kauth_cred_free(fcred);
		if (oc != scred)
			kauth_cred_free(oc);
	}
}

/*
 * proc_specific_key_create --
 *	Create a key for subsystem proc-specific data.
 */
int
proc_specific_key_create(specificdata_key_t *keyp, specificdata_dtor_t dtor)
{

	return (specificdata_key_create(proc_specificdata_domain, keyp, dtor));
}

/*
 * proc_specific_key_delete --
 *	Delete a key for subsystem proc-specific data.
 */
void
proc_specific_key_delete(specificdata_key_t key)
{

	specificdata_key_delete(proc_specificdata_domain, key);
}

/*
 * proc_initspecific --
 *	Initialize a proc's specificdata container.
 */
void
proc_initspecific(struct proc *p)
{
	int error;

	error = specificdata_init(proc_specificdata_domain, &p->p_specdataref);
	KASSERT(error == 0);
}

/*
 * proc_finispecific --
 *	Finalize a proc's specificdata container.
 */
void
proc_finispecific(struct proc *p)
{

	specificdata_fini(proc_specificdata_domain, &p->p_specdataref);
}

/*
 * proc_getspecific --
 *	Return proc-specific data corresponding to the specified key.
 */
void *
proc_getspecific(struct proc *p, specificdata_key_t key)
{

	return (specificdata_getspecific(proc_specificdata_domain,
					 &p->p_specdataref, key));
}

/*
 * proc_setspecific --
 *	Set proc-specific data corresponding to the specified key.
 */
void
proc_setspecific(struct proc *p, specificdata_key_t key, void *data)
{

	specificdata_setspecific(proc_specificdata_domain,
				 &p->p_specdataref, key, data);
}
