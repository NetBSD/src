/*	$NetBSD: kern_proc.c,v 1.63 2003/03/19 16:47:36 christos Exp $	*/

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
 *	@(#)kern_proc.c	8.7 (Berkeley) 2/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_proc.c,v 1.63 2003/03/19 16:47:36 christos Exp $");

#include "opt_kstack.h"

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

static void pg_delete(pid_t);

/*
 * Structure associated with user cacheing.
 */
struct uidinfo {
	LIST_ENTRY(uidinfo) ui_hash;
	uid_t	ui_uid;
	long	ui_proccnt;
};
#define	UIHASH(uid)	(&uihashtbl[(uid) & uihash])
LIST_HEAD(uihashhead, uidinfo) *uihashtbl;
u_long uihash;		/* size of hash table - 1 */

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
 * List of processes that has called exit, but need to be reaped.
 * Locking of this proclist is special; it's accessed in a
 * critical section of process exit, and thus locking it can't
 * modify interrupt state.
 * We use a simple spin lock for this proclist.
 * Processes on this proclist are also on zombproc.
 */
struct simplelock deadproc_slock;
struct deadprocs deadprocs = SLIST_HEAD_INITIALIZER(deadprocs);

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
static __inline uint p2u(struct proc *p) { return (uint)(uintptr_t)p; };
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

struct pool proc_pool;
struct pool lwp_pool;
struct pool lwp_uc_pool;
struct pool pcred_pool;
struct pool plimit_pool;
struct pool pstats_pool;
struct pool pgrp_pool;
struct pool rusage_pool;
struct pool ras_pool;
struct pool sadata_pool;
struct pool saupcall_pool;
struct pool ptimer_pool;

MALLOC_DEFINE(M_EMULDATA, "emuldata", "Per-process emulation data");
MALLOC_DEFINE(M_PROC, "proc", "Proc structures");
MALLOC_DEFINE(M_SESSION, "session", "session header");
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

static void orphanpg __P((struct pgrp *));
#ifdef DEBUG
void pgrpdump __P((void));
#endif

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

	simple_lock_init(&deadproc_slock);

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
	LIST_INIT(&deadlwp);
	LIST_INIT(&zomblwp);

	uihashtbl =
	    hashinit(maxproc / 16, HASH_LIST, M_PROC, M_WAITOK, &uihash);

	pool_init(&proc_pool, sizeof(struct proc), 0, 0, 0, "procpl",
	    &pool_allocator_nointr);
	pool_init(&lwp_pool, sizeof(struct lwp), 0, 0, 0, "lwppl",
	    &pool_allocator_nointr);
	pool_init(&lwp_uc_pool, sizeof(ucontext_t), 0, 0, 0, "lwpucpl",
	    &pool_allocator_nointr);
	pool_init(&pgrp_pool, sizeof(struct pgrp), 0, 0, 0, "pgrppl",
	    &pool_allocator_nointr);
	pool_init(&pcred_pool, sizeof(struct pcred), 0, 0, 0, "pcredpl",
	    &pool_allocator_nointr);
	pool_init(&plimit_pool, sizeof(struct plimit), 0, 0, 0, "plimitpl",
	    &pool_allocator_nointr);
	pool_init(&pstats_pool, sizeof(struct pstats), 0, 0, 0, "pstatspl",
	    &pool_allocator_nointr);
	pool_init(&rusage_pool, sizeof(struct rusage), 0, 0, 0, "rusgepl",
	    &pool_allocator_nointr);
	pool_init(&ras_pool, sizeof(struct ras), 0, 0, 0, "raspl",
	    &pool_allocator_nointr);
	pool_init(&sadata_pool, sizeof(struct sadata), 0, 0, 0, "sadatapl",
	    &pool_allocator_nointr);
	pool_init(&saupcall_pool, sizeof(struct sadata_upcall), 0, 0, 0, 
	    "saupcpl",
	    &pool_allocator_nointr);
	pool_init(&ptimer_pool, sizeof(struct ptimer), 0, 0, 0, "ptimerpl",
	    &pool_allocator_nointr);
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
	return (s);
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
 * Change the count associated with number of processes
 * a given user is using.
 */
int
chgproccnt(uid_t uid, int diff)
{
	struct uidinfo *uip;
	struct uihashhead *uipp;

	uipp = UIHASH(uid);

	LIST_FOREACH(uip, uipp, ui_hash)
		if (uip->ui_uid == uid)
			break;

	if (uip) {
		uip->ui_proccnt += diff;
		if (uip->ui_proccnt > 0)
			return (uip->ui_proccnt);
		if (uip->ui_proccnt < 0)
			panic("chgproccnt: procs < 0");
		LIST_REMOVE(uip, ui_hash);
		FREE(uip, M_PROC);
		return (0);
	}
	if (diff <= 0) {
		if (diff == 0)
			return(0);
		panic("chgproccnt: lost user");
	}
	MALLOC(uip, struct uidinfo *, sizeof(*uip), M_PROC, M_WAITOK);
	LIST_INSERT_HEAD(uipp, uip, ui_hash);
	uip->ui_uid = uid;
	uip->ui_proccnt = diff;
	return (diff);
}

/*
 * Check that the specifies process group in in the session of the
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
 */
int
inferior(struct proc *p, struct proc *q)
{

	for (; p != q; p = p->p_pptr)
		if (p->p_pid == 0)
			return (0);
	return (1);
}

/*
 * Locate a process by number
 */
struct proc *
pfind(pid_t pid)
{
	struct proc *p;

	proclist_lock_read();
	p = pid_table[pid & pid_tbl_mask].pt_proc;
	/* Only allow live processes to be found by pid. */
	if (!P_VALID(p) || p->p_pid != pid ||
	    !((1 << SACTIVE | 1 << SSTOP) & 1 << p->p_stat))
		p = 0;

	/* XXX MP - need to have a reference count... */
	proclist_unlock_read();
	return p;
}


/*
 * Locate a process group by number
 */
struct pgrp *
pgfind(pid_t pgid)
{
	struct pgrp *pgrp;

	proclist_lock_read();
	pgrp = pid_table[pgid & pid_tbl_mask].pt_pgrp;
	/*
	 * Can't look up a pgrp that only exists because the session
	 * hasn't died yet (traditional)
	 */
	if (pgrp == NULL || pgrp->pg_id != pgid
	    || LIST_EMPTY(&pgrp->pg_members))
		pgrp = 0;

	/* XXX MP - need to have a reference count... */
	proclist_unlock_read();
	return pgrp;
}

/*
 * Set entry for process 0
 */
void
proc0_insert(struct proc *p, struct lwp *l, struct pgrp *pgrp,
	struct session *sess)
{
	int s;

	LIST_INIT(&p->p_lwps);
	LIST_INSERT_HEAD(&p->p_lwps, l, l_sibling);
	p->p_nlwps = 1;

	s = proclist_lock_write();

	pid_table[0].pt_proc = p;
	LIST_INSERT_HEAD(&allproc, p, p_list);
	LIST_INSERT_HEAD(&alllwp, l, l_list);

	p->p_pgrp = pgrp;
	pid_table[0].pt_pgrp = pgrp;
	LIST_INIT(&pgrp->pg_members);
	LIST_INSERT_HEAD(&pgrp->pg_members, p, p_pglist);

	pgrp->pg_session = sess;
	sess->s_count = 1;
	sess->s_sid = 0;
	sess->s_leader = p;

	proclist_unlock_write(s);
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
	 * Processing the table backwards maintians a semblance
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
	int s;
	int nxt;
	pid_t pid;
	struct pid_table *pt;

	p = pool_get(&proc_pool, PR_WAITOK);
	p->p_stat = SIDL;			/* protect against others */

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
 * the calling processes pid.
 * If is only valid to enter a process group that is in the session
 * of the process.
 * Also mksess should only be set if we are creating a process group
 *
 * Only called from sys_setsid, sys_setpgid/sys_setprp and the
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
		MALLOC(sess, struct session *, sizeof(struct session),
			    M_SESSION, M_WAITOK);
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
		free(sess, M_SESSION);
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
 * remove process from process group
 */
int
leavepgrp(struct proc *p)
{
	int s = proclist_lock_write();
	struct pgrp *pgrp;
	pid_t pg_id;

	pgrp = p->p_pgrp;
	LIST_REMOVE(p, p_pglist);
	p->p_pgrp = 0;
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
	int s;

	s = proclist_lock_write();
	pgrp = pid_table[pg_id & pid_tbl_mask].pt_pgrp;
	if (pgrp == NULL || pgrp->pg_id != pg_id ||
	     !LIST_EMPTY(&pgrp->pg_members)) {
		proclist_unlock_write(s);
		return;
	}

	/* Remove reference (if any) from tty to this process group */
	ttyp = pgrp->pg_session->s_ttyp;
	if (ttyp != NULL && ttyp->t_pgrp == pgrp)
		ttyp->t_pgrp = NULL;

	ss = pgrp->pg_session;

	if (ss->s_sid == pgrp->pg_id) {
		proclist_unlock_write(s);
		SESSRELE(ss);
		/* pgrp freed by sessdelete() if last reference */
		return;
	}

	proclist_unlock_write(s);
	SESSRELE(ss);
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

	FREE(ss, M_SESSION);
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
 */
void
fixjobc(struct proc *p, struct pgrp *pgrp, int entering)
{
	struct pgrp *hispgrp;
	struct session *mysession = pgrp->pg_session;

	/*
	 * Check p's parent to see whether p qualifies its own process
	 * group; if so, adjust count for p's process group.
	 */
	if ((hispgrp = p->p_pptr->p_pgrp) != pgrp &&
	    hispgrp->pg_session == mysession) {
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
	LIST_FOREACH(p, &p->p_children, p_sibling) {
		if ((hispgrp = p->p_pgrp) != pgrp &&
		    hispgrp->pg_session == mysession &&
		    P_ZOMBIE(p) == 0) {
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
	struct plimit *newlim;

	p->p_flag |= P_SUGID;
	/* reset what needs to be reset in plimit */
	if (p->p_limit->pl_corename != defcorename) {
		if (p->p_limit->p_refcnt > 1 &&
		    (p->p_limit->p_lflags & PL_SHAREMOD) == 0) {
			newlim = limcopy(p->p_limit);
			limfree(p->p_limit);
			p->p_limit = newlim;
		}
		free(p->p_limit->pl_corename, M_TEMP);
		p->p_limit->pl_corename = defcorename;
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
	u_int32_t *ip;
	u_int32_t const *end;

	KASSERT(l != NULL);
	KASSERT(l != &lwp0);

	/*
	 * fill all the stack with magic number
	 * so that later modification on it can be detected.
	 */
	ip = (u_int32_t *)KSTACK_LOWEST_ADDR(l);
	end = (u_int32_t *)((caddr_t)KSTACK_LOWEST_ADDR(l) + KSTACK_SIZE); 
	for (; ip < end; ip++) {
		*ip = KSTACK_MAGIC;
	}
}

void
kstack_check_magic(const struct lwp *l)
{
	u_int32_t const *ip, *end;
	int stackleft;

	KASSERT(l != NULL);

	/* don't check proc0 */ /*XXX*/
	if (l == &lwp0)
		return;

#ifdef __MACHINE_STACK_GROWS_UP
	/* stack grows upwards (eg. hppa) */
	ip = (u_int32_t *)((caddr_t)KSTACK_LOWEST_ADDR(l) + KSTACK_SIZE); 
	end = (u_int32_t *)KSTACK_LOWEST_ADDR(l);
	for (ip--; ip >= end; ip--)
		if (*ip != KSTACK_MAGIC)
			break;
		
	stackleft = (caddr_t)KSTACK_LOWEST_ADDR(l) + KSTACK_SIZE - (caddr_t)ip;
#else /* __MACHINE_STACK_GROWS_UP */
	/* stack grows downwards (eg. i386) */
	ip = (u_int32_t *)KSTACK_LOWEST_ADDR(l);
	end = (u_int32_t *)((caddr_t)KSTACK_LOWEST_ADDR(l) + KSTACK_SIZE); 
	for (; ip < end; ip++)
		if (*ip != KSTACK_MAGIC)
			break;

	stackleft = (caddr_t)ip - KSTACK_LOWEST_ADDR(l);
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
