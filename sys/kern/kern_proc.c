/*	$NetBSD: kern_proc.c,v 1.95 2006/10/08 04:28:44 thorpej Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: kern_proc.c,v 1.95 2006/10/08 04:28:44 thorpej Exp $");

#include "opt_kstack.h"
#include "opt_maxuprc.h"
#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"

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
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/mbuf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/signalvar.h>
#include <sys/ras.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/filedesc.h>
#include <sys/kauth.h>

#include <uvm/uvm.h>
#include <uvm/uvm_extern.h>

/*
 * Other process lists
 */

struct proclist allproc;
struct proclist zombproc;	/* resources have been freed */


/*
 * Process list locking:
 *
 * We have two types of locks on the proclists: read locks and write
 * locks.  Read locks can be used in interrupt context, so while we
 * hold the write lock, we must also block clock interrupts to
 * lock out any scheduling changes that may happen in interrupt
 * context.
 *
 * The proclist lock locks the following structures:
 *
 *	allproc
 *	zombproc
 *	pid_table
 */
struct lock proclist_lock;

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
struct session session0;
struct pgrp pgrp0;
struct proc proc0;
struct lwp lwp0;
kauth_cred_t cred0;
struct filedesc0 filedesc0;
struct cwdinfo cwdi0;
struct plimit limit0;
struct pstats pstat0;
struct vmspace vmspace0;
struct sigacts sigacts0;

extern struct user *proc0paddr;

extern const struct emul emul_netbsd;	/* defined in kern_exec.c */

int nofile = NOFILE;
int maxuprc = MAXUPRC;
int cmask = CMASK;

POOL_INIT(proc_pool, sizeof(struct proc), 0, 0, 0, "procpl",
    &pool_allocator_nointr);
POOL_INIT(pgrp_pool, sizeof(struct pgrp), 0, 0, 0, "pgrppl",
    &pool_allocator_nointr);
POOL_INIT(plimit_pool, sizeof(struct plimit), 0, 0, 0, "plimitpl",
    &pool_allocator_nointr);
POOL_INIT(pstats_pool, sizeof(struct pstats), 0, 0, 0, "pstatspl",
    &pool_allocator_nointr);
POOL_INIT(rusage_pool, sizeof(struct rusage), 0, 0, 0, "rusgepl",
    &pool_allocator_nointr);
POOL_INIT(session_pool, sizeof(struct session), 0, 0, 0, "sessionpl",
    &pool_allocator_nointr);

MALLOC_DEFINE(M_EMULDATA, "emuldata", "Per-process emulation data");
MALLOC_DEFINE(M_PROC, "proc", "Proc structures");
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

static void orphanpg(struct pgrp *);
static void pg_delete(pid_t);

static specificdata_domain_t proc_specificdata_domain;

/*
 * Initialize global process hashing structures.
 */
void
procinit(void)
{
	const struct proclist_desc *pd;
	int i;
#define	LINK_EMPTY ((PID_MAX + INITIAL_PID_TABLE_SIZE) & ~(INITIAL_PID_TABLE_SIZE - 1))

	for (pd = proclists; pd->pd_list != NULL; pd++)
		LIST_INIT(pd->pd_list);

	spinlockinit(&proclist_lock, "proclk", 0);

	pid_table = malloc(INITIAL_PID_TABLE_SIZE * sizeof *pid_table,
			    M_PROC, M_WAITOK);
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

	LIST_INIT(&alllwp);

	uihashtbl =
	    hashinit(maxproc / 16, HASH_LIST, M_PROC, M_WAITOK, &uihash);

	proc_specificdata_domain = specificdata_domain_create();
	KASSERT(proc_specificdata_domain != NULL);
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
	int s;
	u_int i;
	rlim_t lim;

	p = &proc0;
	pg = &pgrp0;
	sess = &session0;
	l = &lwp0;

	simple_lock_init(&p->p_lock);
	LIST_INIT(&p->p_lwps);
	LIST_INSERT_HEAD(&p->p_lwps, l, l_sibling);
	p->p_nlwps = 1;
	simple_lock_init(&p->p_sigctx.ps_silock);
	CIRCLEQ_INIT(&p->p_sigctx.ps_siginfo);

	s = proclist_lock_write();

	pid_table[0].pt_proc = p;
	LIST_INSERT_HEAD(&allproc, p, p_list);
	LIST_INSERT_HEAD(&alllwp, l, l_list);

	p->p_pgrp = pg;
	pid_table[0].pt_pgrp = pg;
	LIST_INIT(&pg->pg_members);
	LIST_INSERT_HEAD(&pg->pg_members, p, p_pglist);

	pg->pg_session = sess;
	sess->s_count = 1;
	sess->s_sid = 0;
	sess->s_leader = p;

	proclist_unlock_write(s);

	/*
	 * Set P_NOCLDWAIT so that kernel threads are reparented to
	 * init(8) when they exit.  init(8) can easily wait them out
	 * for us.
	 */
	p->p_flag = P_SYSTEM | P_NOCLDWAIT;
	p->p_stat = SACTIVE;
	p->p_nice = NZERO;
	p->p_emul = &emul_netbsd;
#ifdef __HAVE_SYSCALL_INTERN
	(*p->p_emul->e_syscall_intern)(p);
#endif
	strncpy(p->p_comm, "swapper", MAXCOMLEN);

	l->l_flag = L_INMEM;
	l->l_stat = LSONPROC;
	p->p_nrlwps = 1;

	callout_init(&l->l_tsleep_ch);

	/* Create credentials. */
	cred0 = kauth_cred_alloc();
	p->p_cred = cred0;
	lwp_update_creds(l);

	/* Create the CWD info. */
	p->p_cwdi = &cwdi0;
	cwdi0.cwdi_cmask = cmask;
	cwdi0.cwdi_refcnt = 1;
	simple_lock_init(&cwdi0.cwdi_slock);

	/* Create the limits structures. */
	p->p_limit = &limit0;
	simple_lock_init(&limit0.p_slock);
	for (i = 0; i < sizeof(p->p_rlimit)/sizeof(p->p_rlimit[0]); i++)
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
	limit0.p_refcnt = 1;

	/* Configure virtual memory system, set vm rlimits. */
	uvm_init_limits(p);

	/* Initialize file descriptor table for proc0. */
	p->p_fd = &filedesc0.fd_fd;
	fdinit1(&filedesc0);

	/*
	 * Initialize proc0's vmspace, which uses the kernel pmap.
	 * All kernel processes (which never have user space mappings)
	 * share proc0's vmspace, and thus, the kernel pmap.
	 */
	uvmspace_init(&vmspace0, pmap_kernel(), round_page(VM_MIN_ADDRESS),
	    trunc_page(VM_MAX_ADDRESS));
	p->p_vmspace = &vmspace0;

	l->l_addr = proc0paddr;				/* XXX */

	p->p_stats = &pstat0;

	/* Initialize signal state for proc0. */
	p->p_sigacts = &sigacts0;
	siginit(p);
}

/*
 * Acquire a read lock on the proclist.
 */
void
proclist_lock_read(void)
{
	int error;

	error = spinlockmgr(&proclist_lock, LK_SHARED, NULL);
#ifdef DIAGNOSTIC
	if (__predict_false(error != 0))
		panic("proclist_lock_read: failed to acquire lock");
#endif
}

/*
 * Release a read lock on the proclist.
 */
void
proclist_unlock_read(void)
{

	(void) spinlockmgr(&proclist_lock, LK_RELEASE, NULL);
}

/*
 * Acquire a write lock on the proclist.
 */
int
proclist_lock_write(void)
{
	int s, error;

	s = splclock();
	error = spinlockmgr(&proclist_lock, LK_EXCLUSIVE, NULL);
#ifdef DIAGNOSTIC
	if (__predict_false(error != 0))
		panic("proclist_lock: failed to acquire lock");
#endif
	return s;
}

/*
 * Release a write lock on the proclist.
 */
void
proclist_unlock_write(int s)
{

	(void) spinlockmgr(&proclist_lock, LK_RELEASE, NULL);
	splx(s);
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

	if (pg_id < 0) {
		struct proc *p1 = pfind(-pg_id);
		if (p1 == NULL)
			return EINVAL;
		pgrp = p1->p_pgrp;
	} else {
		pgrp = pgfind(pg_id);
		if (pgrp == NULL)
			return EINVAL;
	}
	if (pgrp->pg_session != p->p_pgrp->pg_session)
		return EPERM;
	return 0;
}

/*
 * Is p an inferior of q?
 *
 * Call with the proclist_lock held.
 */
int
inferior(struct proc *p, struct proc *q)
{

	for (; p != q; p = p->p_pptr)
		if (p->p_pid == 0)
			return 0;
	return 1;
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
		proclist_lock_read();
	p = pid_table[pid & pid_tbl_mask].pt_proc;
	/* Only allow live processes to be found by pid. */
	if (P_VALID(p) && p->p_pid == pid &&
	    ((stat = p->p_stat) == SACTIVE || stat == SSTOP
		    || (stat == SZOMB && (flags & PFIND_ZOMBIE)))) {
		if (flags & PFIND_UNLOCK_OK)
			 proclist_unlock_read();
		return p;
	}
	if (flags & PFIND_UNLOCK_FAIL)
		 proclist_unlock_read();
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
		proclist_lock_read();
	pg = pid_table[pgid & pid_tbl_mask].pt_pgrp;
	/*
	 * Can't look up a pgrp that only exists because the session
	 * hasn't died yet (traditional)
	 */
	if (pg == NULL || pg->pg_id != pgid || LIST_EMPTY(&pg->pg_members)) {
		if (flags & PFIND_UNLOCK_FAIL)
			 proclist_unlock_read();
		return NULL;
	}

	if (flags & PFIND_UNLOCK_OK)
		proclist_unlock_read();
	return pg;
}

static void
expand_pid_table(void)
{
	uint pt_size = pid_tbl_mask + 1;
	struct pid_table *n_pt, *new_pt;
	struct proc *proc;
	struct pgrp *pgrp;
	int i;
	int s;
	pid_t pid;

	new_pt = malloc(pt_size * 2 * sizeof *new_pt, M_PROC, M_WAITOK);

	s = proclist_lock_write();
	if (pt_size != pid_tbl_mask + 1) {
		/* Another process beat us to it... */
		proclist_unlock_write(s);
		FREE(new_pt, M_PROC);
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

	/* Switch tables */
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

	proclist_unlock_write(s);
	FREE(n_pt, M_PROC);
}

struct proc *
proc_alloc(void)
{
	struct proc *p;
	int s, nxt, error;
	pid_t pid;
	struct pid_table *pt;

	p = pool_get(&proc_pool, PR_WAITOK);
	p->p_stat = SIDL;			/* protect against others */

	error = specificdata_init(proc_specificdata_domain, &p->p_specdataref);
	KASSERT(error == 0);

	/* allocate next free pid */

	for (;;expand_pid_table()) {
		if (__predict_false(pid_alloc_cnt >= pid_alloc_lim))
			/* ensure pids cycle through 2000+ values */
			continue;
		s = proclist_lock_write();
		pt = &pid_table[next_free_pt];
#ifdef DIAGNOSTIC
		if (__predict_false(P_VALID(pt->pt_proc) || pt->pt_pgrp))
			panic("proc_alloc: slot busy");
#endif
		nxt = P_NEXT(pt->pt_proc);
		if (nxt & pid_tbl_mask)
			break;
		/* Table full - expand (NB last entry not used....) */
		proclist_unlock_write(s);
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

	proclist_unlock_write(s);

	return p;
}

/*
 * Free last resources of a process - called from proc_free (in kern_exit.c)
 */
void
proc_free_mem(struct proc *p)
{
	int s;
	pid_t pid = p->p_pid;
	struct pid_table *pt;

	specificdata_fini(proc_specificdata_domain, &p->p_specdataref);

	s = proclist_lock_write();

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

	nprocs--;
	proclist_unlock_write(s);

	pool_put(&proc_pool, p);
}

/*
 * Move p to a new or existing process group (and session)
 *
 * If we are creating a new pgrp, the pgid should equal
 * the calling process' pid.
 * If is only valid to enter a process group that is in the session
 * of the process.
 * Also mksess should only be set if we are creating a process group
 *
 * Only called from sys_setsid, sys_setpgid/sys_setpgrp and the
 * SYSV setpgrp support for hpux == enterpgrp(curproc, curproc->p_pid)
 */
int
enterpgrp(struct proc *p, pid_t pgid, int mksess)
{
	struct pgrp *new_pgrp, *pgrp;
	struct session *sess;
	struct proc *curp = curproc;
	pid_t pid = p->p_pid;
	int rval;
	int s;
	pid_t pg_id = NO_PGID;

	/* Allocate data areas we might need before doing any validity checks */
	proclist_lock_read();		/* Because pid_table might change */
	if (pid_table[pgid & pid_tbl_mask].pt_pgrp == 0) {
		proclist_unlock_read();
		new_pgrp = pool_get(&pgrp_pool, PR_WAITOK);
	} else {
		proclist_unlock_read();
		new_pgrp = NULL;
	}
	if (mksess)
		sess = pool_get(&session_pool, M_WAITOK);
	else
		sess = NULL;

	s = proclist_lock_write();
	rval = EPERM;	/* most common error (to save typing) */

	/* Check pgrp exists or can be created */
	pgrp = pid_table[pgid & pid_tbl_mask].pt_pgrp;
	if (pgrp != NULL && pgrp->pg_id != pgid)
		goto done;

	/* Can only set another process under restricted circumstances. */
	if (p != curp) {
		/* must exist and be one of our children... */
		if (p != pid_table[pid & pid_tbl_mask].pt_proc
		    || !inferior(p, curp)) {
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
		if (p->p_flag & P_EXEC) {
			rval = EACCES;
			goto done;
		}
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
		new_pgrp = 0;
		if (sess != NULL) {
			sess->s_sid = p->p_pid;
			sess->s_leader = p;
			sess->s_count = 1;
			sess->s_ttyvp = NULL;
			sess->s_ttyp = NULL;
			sess->s_flags = p->p_session->s_flags & ~S_LOGIN_SET;
			memcpy(sess->s_login, p->p_session->s_login,
			    sizeof(sess->s_login));
			p->p_flag &= ~P_CONTROLT;
		} else {
			sess = p->p_pgrp->pg_session;
			SESSHOLD(sess);
		}
		pgrp->pg_session = sess;
		sess = 0;

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

	/* Move process to requested group */
	LIST_REMOVE(p, p_pglist);
	if (LIST_EMPTY(&p->p_pgrp->pg_members))
		/* defer delete until we've dumped the lock */
		pg_id = p->p_pgrp->pg_id;
	p->p_pgrp = pgrp;
	LIST_INSERT_HEAD(&pgrp->pg_members, p, p_pglist);

    done:
	proclist_unlock_write(s);
	if (sess != NULL)
		pool_put(&session_pool, sess);
	if (new_pgrp != NULL)
		pool_put(&pgrp_pool, new_pgrp);
	if (pg_id != NO_PGID)
		pg_delete(pg_id);
#ifdef DEBUG_PGRP
	if (__predict_false(rval))
		printf("enterpgrp(%d,%d,%d), curproc %d, rval %d\n",
			pid, pgid, mksess, curp->p_pid, rval);
#endif
	return rval;
}

/*
 * Remove a process from its process group.
 */
int
leavepgrp(struct proc *p)
{
	int s;
	struct pgrp *pgrp;
	pid_t pg_id;

	s = proclist_lock_write();
	pgrp = p->p_pgrp;
	LIST_REMOVE(p, p_pglist);
	p->p_pgrp = NULL;
	pg_id = LIST_EMPTY(&pgrp->pg_members) ? pgrp->pg_id : NO_PGID;
	proclist_unlock_write(s);

	if (pg_id != NO_PGID)
		pg_delete(pg_id);
	return 0;
}

static void
pg_free(pid_t pg_id)
{
	struct pgrp *pgrp;
	struct pid_table *pt;
	int s;

	s = proclist_lock_write();
	pt = &pid_table[pg_id & pid_tbl_mask];
	pgrp = pt->pt_pgrp;
#ifdef DIAGNOSTIC
	if (__predict_false(!pgrp || pgrp->pg_id != pg_id
	    || !LIST_EMPTY(&pgrp->pg_members)))
		panic("pg_free: process group absent or has members");
#endif
	pt->pt_pgrp = 0;

	if (!P_VALID(pt->pt_proc)) {
		/* orphaned pgrp, put slot onto free list */
#ifdef DIAGNOSTIC
		if (__predict_false(P_NEXT(pt->pt_proc) & pid_tbl_mask))
			panic("pg_free: process slot on free list");
#endif

		pg_id &= pid_tbl_mask;
		pt = &pid_table[last_free_pt];
		pt->pt_proc = P_FREE(P_NEXT(pt->pt_proc) | pg_id);
		last_free_pt = pg_id;
		pid_alloc_cnt--;
	}
	proclist_unlock_write(s);

	pool_put(&pgrp_pool, pgrp);
}

/*
 * delete a process group
 */
static void
pg_delete(pid_t pg_id)
{
	struct pgrp *pgrp;
	struct tty *ttyp;
	struct session *ss;
	int s, is_pgrp_leader;

	s = proclist_lock_write();
	pgrp = pid_table[pg_id & pid_tbl_mask].pt_pgrp;
	if (pgrp == NULL || pgrp->pg_id != pg_id ||
	    !LIST_EMPTY(&pgrp->pg_members)) {
		proclist_unlock_write(s);
		return;
	}

	ss = pgrp->pg_session;

	/* Remove reference (if any) from tty to this process group */
	ttyp = ss->s_ttyp;
	if (ttyp != NULL && ttyp->t_pgrp == pgrp) {
		ttyp->t_pgrp = NULL;
#ifdef DIAGNOSTIC
		if (ttyp->t_session != ss)
			panic("pg_delete: wrong session on terminal");
#endif
	}

	/*
	 * The leading process group in a session is freed
	 * by sessdelete() if last reference.
	 */
	is_pgrp_leader = (ss->s_sid == pgrp->pg_id);
	proclist_unlock_write(s);
	SESSRELE(ss);

	if (is_pgrp_leader)
		return;

	pg_free(pg_id);
}

/*
 * Delete session - called from SESSRELE when s_count becomes zero.
 */
void
sessdelete(struct session *ss)
{
	/*
	 * We keep the pgrp with the same id as the session in
	 * order to stop a process being given the same pid.
	 * Since the pgrp holds a reference to the session, it
	 * must be a 'zombie' pgrp by now.
	 */

	pg_free(ss->s_sid);

	pool_put(&session_pool, ss);
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
 * Call with proclist_lock held.
 */
void
fixjobc(struct proc *p, struct pgrp *pgrp, int entering)
{
	struct pgrp *hispgrp;
	struct session *mysession = pgrp->pg_session;
	struct proc *child;

	/*
	 * Check p's parent to see whether p qualifies its own process
	 * group; if so, adjust count for p's process group.
	 */
	hispgrp = p->p_pptr->p_pgrp;
	if (hispgrp != pgrp && hispgrp->pg_session == mysession) {
		if (entering)
			pgrp->pg_jobc++;
		else if (--pgrp->pg_jobc == 0)
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
			if (entering)
				hispgrp->pg_jobc++;
			else if (--hispgrp->pg_jobc == 0)
				orphanpg(hispgrp);
		}
	}
}

/*
 * A process group has become orphaned;
 * if there are any stopped processes in the group,
 * hang-up all process in that group.
 *
 * Call with proclist_lock held.
 */
static void
orphanpg(struct pgrp *pg)
{
	struct proc *p;

	LIST_FOREACH(p, &pg->pg_members, p_pglist) {
		if (p->p_stat == SSTOP) {
			LIST_FOREACH(p, &pg->pg_members, p_pglist) {
				psignal(p, SIGHUP);
				psignal(p, SIGCONT);
			}
			return;
		}
	}
}

/* mark process as suid/sgid, reset some values to defaults */
void
p_sugid(struct proc *p)
{
	struct plimit *lim;
	char *cn;

	p->p_flag |= P_SUGID;
	/* reset what needs to be reset in plimit */
	lim = p->p_limit;
	if (lim->pl_corename != defcorename) {
		if (lim->p_refcnt > 1 &&
		    (lim->p_lflags & PL_SHAREMOD) == 0) {
			p->p_limit = limcopy(lim);
			limfree(lim);
			lim = p->p_limit;
		}
		simple_lock(&lim->p_slock);
		cn = lim->pl_corename;
		lim->pl_corename = defcorename;
		simple_unlock(&lim->p_slock);
		if (cn != defcorename)
			free(cn, M_TEMP);
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
			    pgrp->pg_members.lh_first);
			for (p = pgrp->pg_members.lh_first; p != 0;
			    p = p->p_pglist.le_next) {
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
int kstackleftmin = KSTACK_SIZE;
int kstackleftthres = KSTACK_SIZE / 8; /* warn if remaining stack is
					  less than this */

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
	end = (uint32_t *)((caddr_t)KSTACK_LOWEST_ADDR(l) + KSTACK_SIZE);
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
	ip = (uint32_t *)((caddr_t)KSTACK_LOWEST_ADDR(l) + KSTACK_SIZE);
	end = (uint32_t *)KSTACK_LOWEST_ADDR(l);
	for (ip--; ip >= end; ip--)
		if (*ip != KSTACK_MAGIC)
			break;

	stackleft = (caddr_t)KSTACK_LOWEST_ADDR(l) + KSTACK_SIZE - (caddr_t)ip;
#else /* __MACHINE_STACK_GROWS_UP */
	/* stack grows downwards (eg. i386) */
	ip = (uint32_t *)KSTACK_LOWEST_ADDR(l);
	end = (uint32_t *)((caddr_t)KSTACK_LOWEST_ADDR(l) + KSTACK_SIZE);
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

/* XXX shouldn't be here */
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
#define	PROCLIST_ASSERT_LOCKED_READ()	\
	KASSERT(lockstatus(&proclist_lock) == LK_SHARED)
#else
#define	PROCLIST_ASSERT_LOCKED_READ()	/* nothing */
#endif

int
proclist_foreach_call(struct proclist *list,
    int (*callback)(struct proc *, void *arg), void *arg)
{
	struct proc marker;
	struct proc *p;
	struct lwp * const l = curlwp;
	int ret = 0;

	marker.p_flag = P_MARKER;
	PHOLD(l);
	proclist_lock_read();
	for (p = LIST_FIRST(list); ret == 0 && p != NULL;) {
		if (p->p_flag & P_MARKER) {
			p = LIST_NEXT(p, p_list);
			continue;
		}
		LIST_INSERT_AFTER(p, &marker, p_list);
		ret = (*callback)(p, arg);
		PROCLIST_ASSERT_LOCKED_READ();
		p = LIST_NEXT(&marker, p_list);
		LIST_REMOVE(&marker, p_list);
	}
	proclist_unlock_read();
	PRELE(l);

	return ret;
}

int
proc_vmspace_getref(struct proc *p, struct vmspace **vm)
{

	/* XXXCDC: how should locking work here? */

	/* curproc exception is for coredump. */

	if ((p != curproc && (p->p_flag & P_WEXIT) != 0) ||
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
proc_crmod_enter(struct proc *p)
{

	/*
	 * XXXSMP This should be a lightweight sleep lock.  'struct lock' is
	 * too large.
	 */
	simple_lock(&p->p_lock);
	while ((p->p_flag & P_CRLOCK) != 0)
		ltsleep(&p->p_cred, PLOCK, "crlock", 0, &p->p_lock);
	p->p_flag |= P_CRLOCK;
	simple_unlock(&p->p_lock);
}

/*
 * Block out readers, set in a new process credential, and drop the write
 * lock.  The credential must have a reference already.  Optionally, free a
 * no-longer required credential.
 */
void
proc_crmod_leave(struct proc *p, kauth_cred_t scred, kauth_cred_t fcred)
{

	KDASSERT((p->p_flag & P_CRLOCK) != 0);
	simple_lock(&p->p_lock);
	p->p_cred = scred;
	p->p_flag &= ~P_CRLOCK;
	simple_unlock(&p->p_lock);
	wakeup(&p->p_cred);
	if (fcred != NULL)
		kauth_cred_free(fcred);
}

/*
 * proc_specific_key_create --
 *	Create a key for subsystem proc-specific data.
 */
int
proc_specific_key_create(specificdata_key_t *keyp, specificdata_dtor_t dtor)
{

	return (specificdata_key_create(proc_specificdata_domain,
					keyp, dtor));
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
