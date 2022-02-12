/*	$NetBSD: kern_proc.c,v 1.263 2022/02/12 15:51:29 thorpej Exp $	*/

/*-
 * Copyright (c) 1999, 2006, 2007, 2008, 2020 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: kern_proc.c,v 1.263 2022/02/12 15:51:29 thorpej Exp $");

#ifdef _KERNEL_OPT
#include "opt_kstack.h"
#include "opt_maxuprc.h"
#include "opt_dtrace.h"
#include "opt_compat_netbsd32.h"
#include "opt_kaslr.h"
#endif

#if defined(__HAVE_COMPAT_NETBSD32) && !defined(COMPAT_NETBSD32) \
    && !defined(_RUMPKERNEL)
#define COMPAT_NETBSD32
#endif

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
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/signalvar.h>
#include <sys/ras.h>
#include <sys/filedesc.h>
#include <sys/syscall_stats.h>
#include <sys/kauth.h>
#include <sys/sleepq.h>
#include <sys/atomic.h>
#include <sys/kmem.h>
#include <sys/namei.h>
#include <sys/dtrace_bsd.h>
#include <sys/sysctl.h>
#include <sys/exec.h>
#include <sys/cpu.h>
#include <sys/compat_stub.h>
#include <sys/futex.h>
#include <sys/pserialize.h>

#include <uvm/uvm_extern.h>

/*
 * Process lists.
 */

struct proclist		allproc		__cacheline_aligned;
struct proclist		zombproc	__cacheline_aligned;

 kmutex_t		proc_lock	__cacheline_aligned;
static pserialize_t	proc_psz;

/*
 * pid to lwp/proc lookup is done by indexing the pid_table array.
 * Since pid numbers are only allocated when an empty slot
 * has been found, there is no need to search any lists ever.
 * (an orphaned pgrp will lock the slot, a session will lock
 * the pgrp with the same number.)
 * If the table is too small it is reallocated with twice the
 * previous size and the entries 'unzipped' into the two halves.
 * A linked list of free entries is passed through the pt_lwp
 * field of 'free' items - set odd to be an invalid ptr.  Two
 * additional bits are also used to indicate if the slot is
 * currently occupied by a proc or lwp, and if the PID is
 * hidden from certain kinds of lookups.  We thus require a
 * minimum alignment for proc and lwp structures (LWPs are
 * at least 32-byte aligned).
 */

struct pid_table {
	uintptr_t	pt_slot;
	struct pgrp	*pt_pgrp;
	pid_t		pt_pid;
};

#define	PT_F_FREE		((uintptr_t)__BIT(0))
#define	PT_F_LWP		0	/* pseudo-flag */
#define	PT_F_PROC		((uintptr_t)__BIT(1))

#define	PT_F_TYPEBITS		(PT_F_FREE|PT_F_PROC)
#define	PT_F_ALLBITS		(PT_F_FREE|PT_F_PROC)

#define	PT_VALID(s)		(((s) & PT_F_FREE) == 0)
#define	PT_RESERVED(s)		((s) == 0)
#define	PT_NEXT(s)		((u_int)(s) >> 1)
#define	PT_SET_FREE(pid)	(((pid) << 1) | PT_F_FREE)
#define	PT_SET_LWP(l)		((uintptr_t)(l))
#define	PT_SET_PROC(p)		(((uintptr_t)(p)) | PT_F_PROC)
#define	PT_SET_RESERVED		0
#define	PT_GET_LWP(s)		((struct lwp *)((s) & ~PT_F_ALLBITS))
#define	PT_GET_PROC(s)		((struct proc *)((s) & ~PT_F_ALLBITS))
#define	PT_GET_TYPE(s)		((s) & PT_F_TYPEBITS)
#define	PT_IS_LWP(s)		(PT_GET_TYPE(s) == PT_F_LWP && (s) != 0)
#define	PT_IS_PROC(s)		(PT_GET_TYPE(s) == PT_F_PROC)

#define	MIN_PROC_ALIGNMENT	(PT_F_ALLBITS + 1)

/*
 * Table of process IDs (PIDs).
 */
static struct pid_table *pid_table	__read_mostly;

#define	INITIAL_PID_TABLE_SIZE		(1 << 5)

/* Table mask, threshold for growing and number of allocated PIDs. */
static u_int		pid_tbl_mask	__read_mostly;
static u_int		pid_alloc_lim	__read_mostly;
static u_int		pid_alloc_cnt	__cacheline_aligned;

/* Next free, last free and maximum PIDs. */
static u_int		next_free_pt	__cacheline_aligned;
static u_int		last_free_pt	__cacheline_aligned;
static pid_t		pid_max		__read_mostly;

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
	.cwdi_cmask = CMASK,
	.cwdi_refcnt = 1,
};
struct plimit limit0;
struct pstats pstat0;
struct vmspace vmspace0;
struct sigacts sigacts0;
struct proc proc0 = {
	.p_lwps = LIST_HEAD_INITIALIZER(&proc0.p_lwps),
	.p_sigwaiters = LIST_HEAD_INITIALIZER(&proc0.p_sigwaiters),
	.p_nlwps = 1,
	.p_nrlwps = 1,
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
#ifdef PROC0_MD_INITIALIZERS
	PROC0_MD_INITIALIZERS
#endif
};
kauth_cred_t cred0;

static const int	nofile	= NOFILE;
static const int	maxuprc	= MAXUPRC;

static int sysctl_doeproc(SYSCTLFN_PROTO);
static int sysctl_kern_proc_args(SYSCTLFN_PROTO);
static int sysctl_security_expose_address(SYSCTLFN_PROTO);

#ifdef KASLR
static int kern_expose_address = 0;
#else
static int kern_expose_address = 1;
#endif
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

static kauth_listener_t proc_listener;

static void fill_proc(const struct proc *, struct proc *, bool);
static int fill_pathname(struct lwp *, pid_t, void *, size_t *);
static int fill_cwd(struct lwp *, pid_t, void *, size_t *);

static int
proc_listener_cb(kauth_cred_t cred, kauth_action_t action, void *cookie,
    void *arg0, void *arg1, void *arg2, void *arg3)
{
	struct proc *p;
	int result;

	result = KAUTH_RESULT_DEFER;
	p = arg0;

	switch (action) {
	case KAUTH_PROCESS_CANSEE: {
		enum kauth_process_req req;

		req = (enum kauth_process_req)(uintptr_t)arg1;

		switch (req) {
		case KAUTH_REQ_PROCESS_CANSEE_ARGS:
		case KAUTH_REQ_PROCESS_CANSEE_ENTRY:
		case KAUTH_REQ_PROCESS_CANSEE_OPENFILES:
		case KAUTH_REQ_PROCESS_CANSEE_EPROC:
			result = KAUTH_RESULT_ALLOW;
			break;

		case KAUTH_REQ_PROCESS_CANSEE_ENV:
			if (kauth_cred_getuid(cred) !=
			    kauth_cred_getuid(p->p_cred) ||
			    kauth_cred_getuid(cred) !=
			    kauth_cred_getsvuid(p->p_cred))
				break;

			result = KAUTH_RESULT_ALLOW;

			break;

		case KAUTH_REQ_PROCESS_CANSEE_KPTR:
			if (!kern_expose_address)
				break;

			if (kern_expose_address == 1 && !(p->p_flag & PK_KMEM))
				break;

			result = KAUTH_RESULT_ALLOW;

			break;

		default:
			break;
		}

		break;
		}

	case KAUTH_PROCESS_FORK: {
		int lnprocs = (int)(unsigned long)arg2;

		/*
		 * Don't allow a nonprivileged user to use the last few
		 * processes. The variable lnprocs is the current number of
		 * processes, maxproc is the limit.
		 */
		if (__predict_false((lnprocs >= maxproc - 5)))
			break;

		result = KAUTH_RESULT_ALLOW;

		break;
		}

	case KAUTH_PROCESS_CORENAME:
	case KAUTH_PROCESS_STOPFLAG:
		if (proc_uidmatch(cred, p->p_cred) == 0)
			result = KAUTH_RESULT_ALLOW;

		break;

	default:
		break;
	}

	return result;
}

static int
proc_ctor(void *arg __unused, void *obj, int flags __unused)
{
	struct proc *p = obj;

	memset(p, 0, sizeof(*p));
	klist_init(&p->p_klist);

	/*
	 * There is no need for a proc_dtor() to do a klist_fini(),
	 * since knote_proc_exit() ensures that p->p_klist is empty
	 * when a process exits.
	 */

	return 0;
}

static pid_t proc_alloc_pid_slot(struct proc *, uintptr_t);

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

	mutex_init(&proc_lock, MUTEX_DEFAULT, IPL_NONE);

	proc_psz = pserialize_create();

	pid_table = kmem_alloc(INITIAL_PID_TABLE_SIZE
	    * sizeof(struct pid_table), KM_SLEEP);
	pid_tbl_mask = INITIAL_PID_TABLE_SIZE - 1;
	pid_max = PID_MAX;

	/* Set free list running through table...
	   Preset 'use count' above PID_MAX so we allocate pid 1 next. */
	for (i = 0; i <= pid_tbl_mask; i++) {
		pid_table[i].pt_slot = PT_SET_FREE(LINK_EMPTY + i + 1);
		pid_table[i].pt_pgrp = 0;
		pid_table[i].pt_pid = 0;
	}
	/* slot 0 is just grabbed */
	next_free_pt = 1;
	/* Need to fix last entry. */
	last_free_pt = pid_tbl_mask;
	pid_table[last_free_pt].pt_slot = PT_SET_FREE(LINK_EMPTY);
	/* point at which we grow table - to avoid reusing pids too often */
	pid_alloc_lim = pid_tbl_mask - 1;
#undef LINK_EMPTY

	/* Reserve PID 1 for init(8). */	/* XXX slightly gross */
	mutex_enter(&proc_lock);
	if (proc_alloc_pid_slot(&proc0, PT_SET_RESERVED) != 1)
		panic("failed to reserve PID 1 for init(8)");
	mutex_exit(&proc_lock);

	proc_specificdata_domain = specificdata_domain_create();
	KASSERT(proc_specificdata_domain != NULL);

	size_t proc_alignment = coherency_unit;
	if (proc_alignment < MIN_PROC_ALIGNMENT)
		proc_alignment = MIN_PROC_ALIGNMENT;

	proc_cache = pool_cache_init(sizeof(struct proc), proc_alignment, 0, 0,
	    "procpl", NULL, IPL_NONE, proc_ctor, NULL, NULL);

	proc_listener = kauth_listen_scope(KAUTH_SCOPE_PROCESS,
	    proc_listener_cb, NULL);
}

void
procinit_sysctl(void)
{
	static struct sysctllog *clog;

	sysctl_createv(&clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "expose_address",
		       SYSCTL_DESCR("Enable exposing kernel addresses"),
		       sysctl_security_expose_address, 0,
		       &kern_expose_address, 0, CTL_KERN, CTL_CREATE, CTL_EOL);
	sysctl_createv(&clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "proc",
		       SYSCTL_DESCR("System-wide process information"),
		       sysctl_doeproc, 0, NULL, 0,
		       CTL_KERN, KERN_PROC, CTL_EOL);
	sysctl_createv(&clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "proc2",
		       SYSCTL_DESCR("Machine-independent process information"),
		       sysctl_doeproc, 0, NULL, 0,
		       CTL_KERN, KERN_PROC2, CTL_EOL);
	sysctl_createv(&clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "proc_args",
		       SYSCTL_DESCR("Process argument information"),
		       sysctl_kern_proc_args, 0, NULL, 0,
		       CTL_KERN, KERN_PROC_ARGS, CTL_EOL);

	/*
	  "nodes" under these:

	  KERN_PROC_ALL
	  KERN_PROC_PID pid
	  KERN_PROC_PGRP pgrp
	  KERN_PROC_SESSION sess
	  KERN_PROC_TTY tty
	  KERN_PROC_UID uid
	  KERN_PROC_RUID uid
	  KERN_PROC_GID gid
	  KERN_PROC_RGID gid

	  all in all, probably not worth the effort...
	*/
}

/*
 * Initialize process 0.
 */
void
proc0_init(void)
{
	struct proc *p;
	struct pgrp *pg;
	struct rlimit *rlim;
	rlim_t lim;
	int i;

	p = &proc0;
	pg = &pgrp0;

	mutex_init(&p->p_stmutex, MUTEX_DEFAULT, IPL_HIGH);
	mutex_init(&p->p_auxlock, MUTEX_DEFAULT, IPL_NONE);
	p->p_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_NONE);

	rw_init(&p->p_reflock);
	cv_init(&p->p_waitcv, "wait");
	cv_init(&p->p_lwpcv, "lwpwait");

	LIST_INSERT_HEAD(&p->p_lwps, &lwp0, l_sibling);

	KASSERT(lwp0.l_lid == 0);
	pid_table[lwp0.l_lid].pt_slot = PT_SET_LWP(&lwp0);
	LIST_INSERT_HEAD(&allproc, p, p_list);

	pid_table[lwp0.l_lid].pt_pgrp = pg;
	LIST_INSERT_HEAD(&pg->pg_members, p, p_pglist);

#ifdef __HAVE_SYSCALL_INTERN
	(*p->p_emul->e_syscall_intern)(p);
#endif

	/* Create credentials. */
	cred0 = kauth_cred_alloc();
	p->p_cred = cred0;

	/* Create the CWD info. */
	rw_init(&cwdi0.cwdi_lock);

	/* Create the limits structures. */
	mutex_init(&limit0.pl_lock, MUTEX_DEFAULT, IPL_NONE);

	rlim = limit0.pl_rlimit;
	for (i = 0; i < __arraycount(limit0.pl_rlimit); i++) {
		rlim[i].rlim_cur = RLIM_INFINITY;
		rlim[i].rlim_max = RLIM_INFINITY;
	}

	rlim[RLIMIT_NOFILE].rlim_max = maxfiles;
	rlim[RLIMIT_NOFILE].rlim_cur = maxfiles < nofile ? maxfiles : nofile;

	rlim[RLIMIT_NPROC].rlim_max = maxproc;
	rlim[RLIMIT_NPROC].rlim_cur = maxproc < maxuprc ? maxproc : maxuprc;

	lim = MIN(VM_MAXUSER_ADDRESS, ctob((rlim_t)uvm_availmem(false)));
	rlim[RLIMIT_RSS].rlim_max = lim;
	rlim[RLIMIT_MEMLOCK].rlim_max = lim;
	rlim[RLIMIT_MEMLOCK].rlim_cur = lim / 3;

	rlim[RLIMIT_NTHR].rlim_max = maxlwp;
	rlim[RLIMIT_NTHR].rlim_cur = maxlwp < maxuprc ? maxlwp : maxuprc;

	/* Note that default core name has zero length. */
	limit0.pl_corename = defcorename;
	limit0.pl_cnlen = 0;
	limit0.pl_refcnt = 1;
	limit0.pl_writeable = false;
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
	    trunc_page(VM_MAXUSER_ADDRESS),
#ifdef __USE_TOPDOWN_VM
	    true
#else
	    false
#endif
	    );

	/* Initialize signal state for proc0. XXX IPL_SCHED */
	mutex_init(&p->p_sigacts->sa_mutex, MUTEX_DEFAULT, IPL_SCHED);
	siginit(p);

	proc_initspecific(p);
	kdtrace_proc_ctor(NULL, p);
}

/*
 * Session reference counting.
 */

void
proc_sesshold(struct session *ss)
{

	KASSERT(mutex_owned(&proc_lock));
	ss->s_count++;
}

void
proc_sessrele(struct session *ss)
{
	struct pgrp *pg;

	KASSERT(mutex_owned(&proc_lock));
	KASSERT(ss->s_count > 0);

	/*
	 * We keep the pgrp with the same id as the session in order to
	 * stop a process being given the same pid.  Since the pgrp holds
	 * a reference to the session, it must be a 'zombie' pgrp by now.
	 */
	if (--ss->s_count == 0) {
		pg = pg_remove(ss->s_sid);
	} else {
		pg = NULL;
		ss = NULL;
	}

	mutex_exit(&proc_lock);

	if (pg)
		kmem_free(pg, sizeof(struct pgrp));
	if (ss)
		kmem_free(ss, sizeof(struct session));
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

	if (pg_id == INT_MIN)
		return EINVAL;

	mutex_enter(&proc_lock);
	if (pg_id < 0) {
		struct proc *p1 = proc_find(-pg_id);
		if (p1 == NULL) {
			error = EINVAL;
			goto fail;
		}
		pgrp = p1->p_pgrp;
	} else {
		pgrp = pgrp_find(pg_id);
		if (pgrp == NULL) {
			error = EINVAL;
			goto fail;
		}
	}
	session = pgrp->pg_session;
	error = (session != p->p_pgrp->pg_session) ? EPERM : 0;
fail:
	mutex_exit(&proc_lock);
	return error;
}

/*
 * p_inferior: is p an inferior of q?
 */
static inline bool
p_inferior(struct proc *p, struct proc *q)
{

	KASSERT(mutex_owned(&proc_lock));

	for (; p != q; p = p->p_pptr)
		if (p->p_pid == 0)
			return false;
	return true;
}

/*
 * proc_find_lwp: locate an lwp in said proc by the ID.
 *
 * => Must be called with p::p_lock held.
 * => LSIDL lwps are not returned because they are only partially
 *    constructed while occupying the slot.
 * => Callers need to be careful about lwp::l_stat of the returned
 *    lwp.
 */
struct lwp *
proc_find_lwp(proc_t *p, pid_t pid)
{
	struct pid_table *pt;
	struct lwp *l = NULL;
	uintptr_t slot;
	int s;

	KASSERT(mutex_owned(p->p_lock));

	/*
	 * Look in the pid_table.  This is done unlocked inside a pserialize
	 * read section covering pid_table's memory allocation only, so take
	 * care to read the slot atomically and only once.  This issues a
	 * memory barrier for dependent loads on alpha.
	 */
	s = pserialize_read_enter();
	pt = &pid_table[pid & pid_tbl_mask];
	slot = atomic_load_consume(&pt->pt_slot);
	if (__predict_false(!PT_IS_LWP(slot))) {
		pserialize_read_exit(s);
		return NULL;
	}

	/*
	 * Check to see if the LWP is from the correct process.  We won't
	 * see entries in pid_table from a prior process that also used "p",
	 * by virtue of the fact that allocating "p" means all prior updates
	 * to dependant data structures are visible to this thread.
	 */
	l = PT_GET_LWP(slot);
	if (__predict_false(atomic_load_relaxed(&l->l_proc) != p)) {
		pserialize_read_exit(s);
		return NULL;
	}

	/*
	 * We now know that p->p_lock holds this LWP stable.
	 *
	 * If the status is not LSIDL, it means the LWP is intended to be
	 * findable by LID and l_lid cannot change behind us.
	 *
	 * No need to acquire the LWP's lock to check for LSIDL, as
	 * p->p_lock must be held to transition in and out of LSIDL.
	 * Any other observed state of is no particular interest.
	 */
	pserialize_read_exit(s);
	return l->l_stat != LSIDL && l->l_lid == pid ? l : NULL;
}

/*
 * proc_find_lwp_unlocked: locate an lwp in said proc by the ID.
 *
 * => Called in a pserialize read section with no locks held.
 * => LSIDL lwps are not returned because they are only partially
 *    constructed while occupying the slot.
 * => Callers need to be careful about lwp::l_stat of the returned
 *    lwp.
 * => If an LWP is found, it's returned locked.
 */
struct lwp *
proc_find_lwp_unlocked(proc_t *p, pid_t pid)
{
	struct pid_table *pt;
	struct lwp *l = NULL;
	uintptr_t slot;

	KASSERT(pserialize_in_read_section());

	/*
	 * Look in the pid_table.  This is done unlocked inside a pserialize
	 * read section covering pid_table's memory allocation only, so take
	 * care to read the slot atomically and only once.  This issues a
	 * memory barrier for dependent loads on alpha.
	 */
	pt = &pid_table[pid & pid_tbl_mask];
	slot = atomic_load_consume(&pt->pt_slot);
	if (__predict_false(!PT_IS_LWP(slot))) {
		return NULL;
	}

	/*
	 * Lock the LWP we found to get it stable.  If it's embryonic or
	 * reaped (LSIDL) then none of the other fields can safely be
	 * checked.
	 */
	l = PT_GET_LWP(slot);
	lwp_lock(l);
	if (__predict_false(l->l_stat == LSIDL)) {
		lwp_unlock(l);
		return NULL;
	}

	/*
	 * l_proc and l_lid are now known stable because the LWP is not
	 * LSIDL, so check those fields too to make sure we found the
	 * right thing.
	 */
	if (__predict_false(l->l_proc != p || l->l_lid != pid)) {
		lwp_unlock(l);
		return NULL;
	}

	/* Everything checks out, return it locked. */
	return l;
}

/*
 * proc_find_lwp_acquire_proc: locate an lwp and acquire a lock
 * on its containing proc.
 *
 * => Similar to proc_find_lwp(), but does not require you to have
 *    the proc a priori.
 * => Also returns proc * to caller, with p::p_lock held.
 * => Same caveats apply.
 */
struct lwp *
proc_find_lwp_acquire_proc(pid_t pid, struct proc **pp)
{
	struct pid_table *pt;
	struct proc *p = NULL;
	struct lwp *l = NULL;
	uintptr_t slot;

	KASSERT(pp != NULL);
	mutex_enter(&proc_lock);
	pt = &pid_table[pid & pid_tbl_mask];

	slot = pt->pt_slot;
	if (__predict_true(PT_IS_LWP(slot) && pt->pt_pid == pid)) {
		l = PT_GET_LWP(slot);
		p = l->l_proc;
		mutex_enter(p->p_lock);
		if (__predict_false(l->l_stat == LSIDL)) {
			mutex_exit(p->p_lock);
			l = NULL;
			p = NULL;
		}
	}
	mutex_exit(&proc_lock);

	KASSERT(p == NULL || mutex_owned(p->p_lock));
	*pp = p;
	return l;
}

/*
 * proc_find_raw_pid_table_locked: locate a process by the ID.
 *
 * => Must be called with proc_lock held.
 */
static proc_t *
proc_find_raw_pid_table_locked(pid_t pid, bool any_lwpid)
{
	struct pid_table *pt;
	proc_t *p = NULL;
	uintptr_t slot;

	/* No - used by DDB.  KASSERT(mutex_owned(&proc_lock)); */
	pt = &pid_table[pid & pid_tbl_mask];

	slot = pt->pt_slot;
	if (__predict_true(PT_IS_LWP(slot) && pt->pt_pid == pid)) {
		/*
		 * When looking up processes, require a direct match
		 * on the PID assigned to the proc, not just one of
		 * its LWPs.
		 *
		 * N.B. We require lwp::l_proc of LSIDL LWPs to be
		 * valid here.
		 */
		p = PT_GET_LWP(slot)->l_proc;
		if (__predict_false(p->p_pid != pid && !any_lwpid))
			p = NULL;
	} else if (PT_IS_PROC(slot) && pt->pt_pid == pid) {
		p = PT_GET_PROC(slot);
	}
	return p;
}

proc_t *
proc_find_raw(pid_t pid)
{

	return proc_find_raw_pid_table_locked(pid, false);
}

static proc_t *
proc_find_internal(pid_t pid, bool any_lwpid)
{
	proc_t *p;

	KASSERT(mutex_owned(&proc_lock));

	p = proc_find_raw_pid_table_locked(pid, any_lwpid);
	if (__predict_false(p == NULL)) {
		return NULL;
	}

	/*
	 * Only allow live processes to be found by PID.
	 * XXX: p_stat might change, since proc unlocked.
	 */
	if (__predict_true(p->p_stat == SACTIVE || p->p_stat == SSTOP)) {
		return p;
	}
	return NULL;
}

proc_t *
proc_find(pid_t pid)
{
	return proc_find_internal(pid, false);
}

proc_t *
proc_find_lwpid(pid_t pid)
{
	return proc_find_internal(pid, true);
}

/*
 * pgrp_find: locate a process group by the ID.
 *
 * => Must be called with proc_lock held.
 */
struct pgrp *
pgrp_find(pid_t pgid)
{
	struct pgrp *pg;

	KASSERT(mutex_owned(&proc_lock));

	pg = pid_table[pgid & pid_tbl_mask].pt_pgrp;

	/*
	 * Cannot look up a process group that only exists because the
	 * session has not died yet (traditional).
	 */
	if (pg == NULL || pg->pg_id != pgid || LIST_EMPTY(&pg->pg_members)) {
		return NULL;
	}
	return pg;
}

static void
expand_pid_table(void)
{
	size_t pt_size, tsz;
	struct pid_table *n_pt, *new_pt;
	uintptr_t slot;
	struct pgrp *pgrp;
	pid_t pid, rpid;
	u_int i;
	uint new_pt_mask;

	KASSERT(mutex_owned(&proc_lock));

	/* Unlock the pid_table briefly to allocate memory. */
	pt_size = pid_tbl_mask + 1;
	mutex_exit(&proc_lock);

	tsz = pt_size * 2 * sizeof(struct pid_table);
	new_pt = kmem_alloc(tsz, KM_SLEEP);
	new_pt_mask = pt_size * 2 - 1;

	/* XXX For now.  The pratical limit is much lower anyway. */
	KASSERT(new_pt_mask <= FUTEX_TID_MASK);

	mutex_enter(&proc_lock);
	if (pt_size != pid_tbl_mask + 1) {
		/* Another process beat us to it... */
		mutex_exit(&proc_lock);
		kmem_free(new_pt, tsz);
		goto out;
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
	 * of issuing pid numbers that increase with time.
	 */
	i = pt_size - 1;
	n_pt = new_pt + i;
	for (; ; i--, n_pt--) {
		slot = pid_table[i].pt_slot;
		pgrp = pid_table[i].pt_pgrp;
		if (!PT_VALID(slot)) {
			/* Up 'use count' so that link is valid */
			pid = (PT_NEXT(slot) + pt_size) & ~pt_size;
			rpid = 0;
			slot = PT_SET_FREE(pid);
			if (pgrp)
				pid = pgrp->pg_id;
		} else {
			pid = pid_table[i].pt_pid;
			rpid = pid;
		}

		/* Save entry in appropriate half of table */
		n_pt[pid & pt_size].pt_slot = slot;
		n_pt[pid & pt_size].pt_pgrp = pgrp;
		n_pt[pid & pt_size].pt_pid = rpid;

		/* Put other piece on start of free list */
		pid = (pid ^ pt_size) & ~pid_tbl_mask;
		n_pt[pid & pt_size].pt_slot =
			PT_SET_FREE((pid & ~pt_size) | next_free_pt);
		n_pt[pid & pt_size].pt_pgrp = 0;
		n_pt[pid & pt_size].pt_pid = 0;

		next_free_pt = i | (pid & pt_size);
		if (i == 0)
			break;
	}

	/* Save old table size and switch tables */
	tsz = pt_size * sizeof(struct pid_table);
	n_pt = pid_table;
	pid_table = new_pt;
	pid_tbl_mask = new_pt_mask;

	/*
	 * pid_max starts as PID_MAX (= 30000), once we have 16384
	 * allocated pids we need it to be larger!
	 */
	if (pid_tbl_mask > PID_MAX) {
		pid_max = pid_tbl_mask * 2 + 1;
		pid_alloc_lim |= pid_alloc_lim << 1;
	} else
		pid_alloc_lim <<= 1;	/* doubles number of free slots... */

	mutex_exit(&proc_lock);

	/*
	 * Make sure that unlocked access to the old pid_table is complete
	 * and then free it.
	 */
	pserialize_perform(proc_psz);
	kmem_free(n_pt, tsz);

 out:	/* Return with proc_lock held again. */
	mutex_enter(&proc_lock);
}

struct proc *
proc_alloc(void)
{
	struct proc *p;

	p = pool_cache_get(proc_cache, PR_WAITOK);
	p->p_stat = SIDL;			/* protect against others */
	proc_initspecific(p);
	kdtrace_proc_ctor(NULL, p);

	/*
	 * Allocate a placeholder in the pid_table.  When we create the
	 * first LWP for this process, it will take ownership of the
	 * slot.
	 */
	if (__predict_false(proc_alloc_pid(p) == -1)) {
		/* Allocating the PID failed; unwind. */
		proc_finispecific(p);
		proc_free_mem(p);
		p = NULL;
	}
	return p;
}

/*
 * proc_alloc_pid_slot: allocate PID and record the occcupant so that
 * proc_find_raw() can find it by the PID.
 */
static pid_t __noinline
proc_alloc_pid_slot(struct proc *p, uintptr_t slot)
{
	struct pid_table *pt;
	pid_t pid;
	int nxt;

	KASSERT(mutex_owned(&proc_lock));

	for (;;expand_pid_table()) {
		if (__predict_false(pid_alloc_cnt >= pid_alloc_lim)) {
			/* ensure pids cycle through 2000+ values */
			continue;
		}
		/*
		 * The first user process *must* be given PID 1.
		 * it has already been reserved for us.  This
		 * will be coming in from the proc_alloc() call
		 * above, and the entry will be usurped later when
		 * the first user LWP is created.
		 * XXX this is slightly gross.
		 */
		if (__predict_false(PT_RESERVED(pid_table[1].pt_slot) &&
				    p != &proc0)) {
			KASSERT(PT_IS_PROC(slot));
			pt = &pid_table[1];
			pt->pt_slot = slot;
			return 1;
		}
		pt = &pid_table[next_free_pt];
#ifdef DIAGNOSTIC
		if (__predict_false(PT_VALID(pt->pt_slot) || pt->pt_pgrp))
			panic("proc_alloc: slot busy");
#endif
		nxt = PT_NEXT(pt->pt_slot);
		if (nxt & pid_tbl_mask)
			break;
		/* Table full - expand (NB last entry not used....) */
	}

	/* pid is 'saved use count' + 'size' + entry */
	pid = (nxt & ~pid_tbl_mask) + pid_tbl_mask + 1 + next_free_pt;
	if ((uint)pid > (uint)pid_max)
		pid &= pid_tbl_mask;
	next_free_pt = nxt & pid_tbl_mask;

	/* XXX For now.  The pratical limit is much lower anyway. */
	KASSERT(pid <= FUTEX_TID_MASK);

	/* Grab table slot */
	pt->pt_slot = slot;

	KASSERT(pt->pt_pid == 0);
	pt->pt_pid = pid;
	pid_alloc_cnt++;

	return pid;
}

pid_t
proc_alloc_pid(struct proc *p)
{
	pid_t pid;

	KASSERT((((uintptr_t)p) & PT_F_ALLBITS) == 0);
	KASSERT(p->p_stat == SIDL);

	mutex_enter(&proc_lock);
	pid = proc_alloc_pid_slot(p, PT_SET_PROC(p));
	if (pid != -1)
		p->p_pid = pid;
	mutex_exit(&proc_lock);

	return pid;
}

pid_t
proc_alloc_lwpid(struct proc *p, struct lwp *l)
{
	struct pid_table *pt;
	pid_t pid;

	KASSERT((((uintptr_t)l) & PT_F_ALLBITS) == 0);
	KASSERT(l->l_proc == p);
	KASSERT(l->l_stat == LSIDL);

	/*
	 * For unlocked lookup in proc_find_lwp(), make sure l->l_proc
	 * is globally visible before the LWP becomes visible via the
	 * pid_table.
	 */
#ifndef __HAVE_ATOMIC_AS_MEMBAR
	membar_producer();
#endif

	/*
	 * If the slot for p->p_pid currently points to the proc,
	 * then we should usurp this ID for the LWP.  This happens
	 * at least once per process (for the first LWP), and can
	 * happen again if the first LWP for a process exits and
	 * before the process creates another.
	 */
	mutex_enter(&proc_lock);
	pid = p->p_pid;
	pt = &pid_table[pid & pid_tbl_mask];
	KASSERT(pt->pt_pid == pid);
	if (PT_IS_PROC(pt->pt_slot)) {
		KASSERT(PT_GET_PROC(pt->pt_slot) == p);
		l->l_lid = pid;
		pt->pt_slot = PT_SET_LWP(l);
	} else {
		/* Need to allocate a new slot. */
		pid = proc_alloc_pid_slot(p, PT_SET_LWP(l));
		if (pid != -1)
			l->l_lid = pid;
	}
	mutex_exit(&proc_lock);

	return pid;
}

static void __noinline
proc_free_pid_internal(pid_t pid, uintptr_t type __diagused)
{
	struct pid_table *pt;

	pt = &pid_table[pid & pid_tbl_mask];

	KASSERT(PT_GET_TYPE(pt->pt_slot) == type);
	KASSERT(pt->pt_pid == pid);

	/* save pid use count in slot */
	pt->pt_slot = PT_SET_FREE(pid & ~pid_tbl_mask);
	pt->pt_pid = 0;

	if (pt->pt_pgrp == NULL) {
		/* link last freed entry onto ours */
		pid &= pid_tbl_mask;
		pt = &pid_table[last_free_pt];
		pt->pt_slot = PT_SET_FREE(PT_NEXT(pt->pt_slot) | pid);
		pt->pt_pid = 0;
		last_free_pt = pid;
		pid_alloc_cnt--;
	}
}

/*
 * Free a process id - called from proc_free (in kern_exit.c)
 *
 * Called with the proc_lock held.
 */
void
proc_free_pid(pid_t pid)
{

	KASSERT(mutex_owned(&proc_lock));
	proc_free_pid_internal(pid, PT_F_PROC);
}

/*
 * Free a process id used by an LWP.  If this was the process's
 * first LWP, we convert the slot to point to the process; the
 * entry will get cleaned up later when the process finishes exiting.
 *
 * If not, then it's the same as proc_free_pid().
 */
void
proc_free_lwpid(struct proc *p, pid_t pid)
{

	KASSERT(mutex_owned(&proc_lock));

	if (__predict_true(p->p_pid == pid)) {
		struct pid_table *pt;

		pt = &pid_table[pid & pid_tbl_mask];

		KASSERT(pt->pt_pid == pid);
		KASSERT(PT_IS_LWP(pt->pt_slot));
		KASSERT(PT_GET_LWP(pt->pt_slot)->l_proc == p);

		pt->pt_slot = PT_SET_PROC(p);
		return;
	}
	proc_free_pid_internal(pid, PT_F_LWP);
}

void
proc_free_mem(struct proc *p)
{

	kdtrace_proc_dtor(NULL, p);
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
 * Only called from sys_setsid, sys_setpgid and posix_spawn/spawn_return.
 */
int
proc_enterpgrp(struct proc *curp, pid_t pid, pid_t pgid, bool mksess)
{
	struct pgrp *new_pgrp, *pgrp;
	struct session *sess;
	struct proc *p;
	int rval;
	pid_t pg_id = NO_PGID;

	/* Allocate data areas we might need before doing any validity checks */
	sess = mksess ? kmem_alloc(sizeof(*sess), KM_SLEEP) : NULL;
	new_pgrp = kmem_alloc(sizeof(*new_pgrp), KM_SLEEP);

	mutex_enter(&proc_lock);
	rval = EPERM;	/* most common error (to save typing) */

	/* Check pgrp exists or can be created */
	pgrp = pid_table[pgid & pid_tbl_mask].pt_pgrp;
	if (pgrp != NULL && pgrp->pg_id != pgid)
		goto done;

	/* Can only set another process under restricted circumstances. */
	if (pid != curp->p_pid) {
		/* Must exist and be one of our children... */
		p = proc_find_internal(pid, false);
		if (p == NULL || !p_inferior(p, curp)) {
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
		    pgrp_find(curp->p_pid)))
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
		mutex_exit(&proc_lock);
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

	KASSERT(mutex_owned(&proc_lock));

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
		mutex_exit(&proc_lock);
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

	KASSERT(mutex_owned(&proc_lock));

	pt = &pid_table[pg_id & pid_tbl_mask];
	pgrp = pt->pt_pgrp;

	KASSERT(pgrp != NULL);
	KASSERT(pgrp->pg_id == pg_id);
	KASSERT(LIST_EMPTY(&pgrp->pg_members));

	pt->pt_pgrp = NULL;

	if (!PT_VALID(pt->pt_slot)) {
		/* Orphaned pgrp, put slot onto free list. */
		KASSERT((PT_NEXT(pt->pt_slot) & pid_tbl_mask) == 0);
		pg_id &= pid_tbl_mask;
		pt = &pid_table[last_free_pt];
		pt->pt_slot = PT_SET_FREE(PT_NEXT(pt->pt_slot) | pg_id);
		KASSERT(pt->pt_pid == 0);
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

	KASSERT(mutex_owned(&proc_lock));

	pg = pid_table[pg_id & pid_tbl_mask].pt_pgrp;
	if (pg == NULL || pg->pg_id != pg_id || !LIST_EMPTY(&pg->pg_members)) {
		mutex_exit(&proc_lock);
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
	 * if last reference.  It will also release the locks.
	 */
	pg = (ss->s_sid != pg->pg_id) ? pg_remove(pg_id) : NULL;
	proc_sessrele(ss);

	if (pg != NULL) {
		/* Free it, if was not done above. */
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

	KASSERT(mutex_owned(&proc_lock));

	/*
	 * Check p's parent to see whether p qualifies its own process
	 * group; if so, adjust count for p's process group.
	 */
	hispgrp = p->p_pptr->p_pgrp;
	if (hispgrp != pgrp && hispgrp->pg_session == mysession) {
		if (entering) {
			pgrp->pg_jobc++;
			p->p_lflag &= ~PL_ORPHANPG;
		} else {
			/* KASSERT(pgrp->pg_jobc > 0); */
			if (--pgrp->pg_jobc == 0)
				orphanpg(pgrp);
		}
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
			} else {
				KASSERT(hispgrp->pg_jobc > 0);
				if (--hispgrp->pg_jobc == 0)
					orphanpg(hispgrp);
			}
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

	KASSERT(mutex_owned(&proc_lock));

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
	uintptr_t slot;
	int id;

	db_printf("pid table %p size %x, next %x, last %x\n",
		pid_table, pid_tbl_mask+1,
		next_free_pt, last_free_pt);
	for (pt = pid_table, id = 0; id <= pid_tbl_mask; id++, pt++) {
		slot = pt->pt_slot;
		if (!PT_VALID(slot) && !pt->pt_pgrp)
			continue;
		if (PT_IS_LWP(slot)) {
			p = PT_GET_LWP(slot)->l_proc;
		} else if (PT_IS_PROC(slot)) {
			p = PT_GET_PROC(slot);
		} else {
			p = NULL;
		}
		db_printf("  id %x: ", id);
		if (p != NULL)
			db_printf("slotpid %d proc %p id %d (0x%x) %s\n",
				pt->pt_pid, p, p->p_pid, p->p_pid, p->p_comm);
		else
			db_printf("next %x use %x\n",
				PT_NEXT(slot) & pid_tbl_mask,
				PT_NEXT(slot) & ~pid_tbl_mask);
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
	int ret = 0;

	marker.p_flag = PK_MARKER;
	mutex_enter(&proc_lock);
	for (p = LIST_FIRST(list); ret == 0 && p != NULL;) {
		if (p->p_flag & PK_MARKER) {
			p = LIST_NEXT(p, p_list);
			continue;
		}
		LIST_INSERT_AFTER(p, &marker, p_list);
		ret = (*callback)(p, arg);
		KASSERT(mutex_owned(&proc_lock));
		p = LIST_NEXT(&marker, p_list);
		LIST_REMOVE(&marker, p_list);
	}
	mutex_exit(&proc_lock);

	return ret;
}

int
proc_vmspace_getref(struct proc *p, struct vmspace **vm)
{

	/* XXXCDC: how should locking work here? */

	/* curproc exception is for coredump. */

	if ((p != curproc && (p->p_sflag & PS_WEXIT) != 0) ||
	    (p->p_vmspace->vm_refcnt < 1)) {
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
	kauth_cred_t oc;

	/* Reset what needs to be reset in plimit. */
	if (p->p_limit->pl_corename != defcorename) {
		lim_setcorename(p, defcorename, 0);
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
	int error __diagused;

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

int
proc_uidmatch(kauth_cred_t cred, kauth_cred_t target)
{
	int r = 0;

	if (kauth_cred_getuid(cred) != kauth_cred_getuid(target) ||
	    kauth_cred_getuid(cred) != kauth_cred_getsvuid(target)) {
		/*
		 * suid proc of ours or proc not ours
		 */
		r = EPERM;
	} else if (kauth_cred_getgid(target) != kauth_cred_getsvgid(target)) {
		/*
		 * sgid proc has sgid back to us temporarily
		 */
		r = EPERM;
	} else {
		/*
		 * our rgid must be in target's group list (ie,
		 * sub-processes started by a sgid process)
		 */
		int ismember = 0;

		if (kauth_cred_ismember_gid(cred,
		    kauth_cred_getgid(target), &ismember) != 0 ||
		    !ismember)
			r = EPERM;
	}

	return (r);
}

/*
 * sysctl stuff
 */

#define KERN_PROCSLOP	(5 * sizeof(struct kinfo_proc))

static const u_int sysctl_flagmap[] = {
	PK_ADVLOCK, P_ADVLOCK,
	PK_EXEC, P_EXEC,
	PK_NOCLDWAIT, P_NOCLDWAIT,
	PK_32, P_32,
	PK_CLDSIGIGN, P_CLDSIGIGN,
	PK_SUGID, P_SUGID,
	0
};

static const u_int sysctl_sflagmap[] = {
	PS_NOCLDSTOP, P_NOCLDSTOP,
	PS_WEXIT, P_WEXIT,
	PS_STOPFORK, P_STOPFORK,
	PS_STOPEXEC, P_STOPEXEC,
	PS_STOPEXIT, P_STOPEXIT,
	0
};

static const u_int sysctl_slflagmap[] = {
	PSL_TRACED, P_TRACED,
	PSL_CHTRACED, P_CHTRACED,
	PSL_SYSCALL, P_SYSCALL,
	0
};

static const u_int sysctl_lflagmap[] = {
	PL_CONTROLT, P_CONTROLT,
	PL_PPWAIT, P_PPWAIT,
	0
};

static const u_int sysctl_stflagmap[] = {
	PST_PROFIL, P_PROFIL,
	0

};

/* used by kern_lwp also */
const u_int sysctl_lwpflagmap[] = {
	LW_SINTR, L_SINTR,
	LW_SYSTEM, L_SYSTEM,
	0
};

/*
 * Find the most ``active'' lwp of a process and return it for ps display
 * purposes
 */
static struct lwp *
proc_active_lwp(struct proc *p)
{
	static const int ostat[] = {
		0,
		2,	/* LSIDL */
		6,	/* LSRUN */
		5,	/* LSSLEEP */
		4,	/* LSSTOP */
		0,	/* LSZOMB */
		1,	/* LSDEAD */
		7,	/* LSONPROC */
		3	/* LSSUSPENDED */
	};

	struct lwp *l, *lp = NULL;
	LIST_FOREACH(l, &p->p_lwps, l_sibling) {
		KASSERT(l->l_stat >= 0 && l->l_stat < __arraycount(ostat));
		if (lp == NULL ||
		    ostat[l->l_stat] > ostat[lp->l_stat] ||
		    (ostat[l->l_stat] == ostat[lp->l_stat] &&
		    l->l_cpticks > lp->l_cpticks)) {
			lp = l;
			continue;
		}
	}
	return lp;
}

static int
sysctl_doeproc(SYSCTLFN_ARGS)
{
	union {
		struct kinfo_proc kproc;
		struct kinfo_proc2 kproc2;
	} *kbuf;
	struct proc *p, *next, *marker;
	char *where, *dp;
	int type, op, arg, error;
	u_int elem_size, kelem_size, elem_count;
	size_t buflen, needed;
	bool match, zombie, mmmbrains;
	const bool allowaddr = get_expose_address(curproc);

	if (namelen == 1 && name[0] == CTL_QUERY)
		return (sysctl_query(SYSCTLFN_CALL(rnode)));

	dp = where = oldp;
	buflen = where != NULL ? *oldlenp : 0;
	error = 0;
	needed = 0;
	type = rnode->sysctl_num;

	if (type == KERN_PROC) {
		if (namelen == 0)
			return EINVAL;
		switch (op = name[0]) {
		case KERN_PROC_ALL:
			if (namelen != 1)
				return EINVAL;
			arg = 0;
			break;
		default:
			if (namelen != 2)
				return EINVAL;
			arg = name[1];
			break;
		}
		elem_count = 0;	/* Hush little compiler, don't you cry */
		kelem_size = elem_size = sizeof(kbuf->kproc);
	} else {
		if (namelen != 4)
			return EINVAL;
		op = name[0];
		arg = name[1];
		elem_size = name[2];
		elem_count = name[3];
		kelem_size = sizeof(kbuf->kproc2);
	}

	sysctl_unlock();

	kbuf = kmem_zalloc(sizeof(*kbuf), KM_SLEEP);
	marker = kmem_alloc(sizeof(*marker), KM_SLEEP);
	marker->p_flag = PK_MARKER;

	mutex_enter(&proc_lock);
	/*
	 * Start with zombies to prevent reporting processes twice, in case they
	 * are dying and being moved from the list of alive processes to zombies.
	 */
	mmmbrains = true;
	for (p = LIST_FIRST(&zombproc);; p = next) {
		if (p == NULL) {
			if (mmmbrains) {
				p = LIST_FIRST(&allproc);
				mmmbrains = false;
			}
			if (p == NULL)
				break;
		}
		next = LIST_NEXT(p, p_list);
		if ((p->p_flag & PK_MARKER) != 0)
			continue;

		/*
		 * Skip embryonic processes.
		 */
		if (p->p_stat == SIDL)
			continue;

		mutex_enter(p->p_lock);
		error = kauth_authorize_process(l->l_cred,
		    KAUTH_PROCESS_CANSEE, p,
		    KAUTH_ARG(KAUTH_REQ_PROCESS_CANSEE_EPROC), NULL, NULL);
		if (error != 0) {
			mutex_exit(p->p_lock);
			continue;
		}

		/*
		 * Hande all the operations in one switch on the cost of
		 * algorithm complexity is on purpose. The win splitting this
		 * function into several similar copies makes maintenance burden
		 * burden, code grow and boost is neglible in practical systems.
		 */
		switch (op) {
		case KERN_PROC_PID:
			match = (p->p_pid == (pid_t)arg);
			break;

		case KERN_PROC_PGRP:
			match = (p->p_pgrp->pg_id == (pid_t)arg);
			break;

		case KERN_PROC_SESSION:
			match = (p->p_session->s_sid == (pid_t)arg);
			break;

		case KERN_PROC_TTY:
			match = true;
			if (arg == (int) KERN_PROC_TTY_REVOKE) {
				if ((p->p_lflag & PL_CONTROLT) == 0 ||
				    p->p_session->s_ttyp == NULL ||
				    p->p_session->s_ttyvp != NULL) {
				    	match = false;
				}
			} else if ((p->p_lflag & PL_CONTROLT) == 0 ||
			    p->p_session->s_ttyp == NULL) {
				if ((dev_t)arg != KERN_PROC_TTY_NODEV) {
					match = false;
				}
			} else if (p->p_session->s_ttyp->t_dev != (dev_t)arg) {
				match = false;
			}
			break;

		case KERN_PROC_UID:
			match = (kauth_cred_geteuid(p->p_cred) == (uid_t)arg);
			break;

		case KERN_PROC_RUID:
			match = (kauth_cred_getuid(p->p_cred) == (uid_t)arg);
			break;

		case KERN_PROC_GID:
			match = (kauth_cred_getegid(p->p_cred) == (uid_t)arg);
			break;

		case KERN_PROC_RGID:
			match = (kauth_cred_getgid(p->p_cred) == (uid_t)arg);
			break;

		case KERN_PROC_ALL:
			match = true;
			/* allow everything */
			break;

		default:
			error = EINVAL;
			mutex_exit(p->p_lock);
			goto cleanup;
		}
		if (!match) {
			mutex_exit(p->p_lock);
			continue;
		}

		/*
		 * Grab a hold on the process.
		 */
		if (mmmbrains) {
			zombie = true;
		} else {
			zombie = !rw_tryenter(&p->p_reflock, RW_READER);
		}
		if (zombie) {
			LIST_INSERT_AFTER(p, marker, p_list);
		}

		if (buflen >= elem_size &&
		    (type == KERN_PROC || elem_count > 0)) {
			ruspace(p);	/* Update process vm resource use */

			if (type == KERN_PROC) {
				fill_proc(p, &kbuf->kproc.kp_proc, allowaddr);
				fill_eproc(p, &kbuf->kproc.kp_eproc, zombie,
				    allowaddr);
			} else {
				fill_kproc2(p, &kbuf->kproc2, zombie,
				    allowaddr);
				elem_count--;
			}
			mutex_exit(p->p_lock);
			mutex_exit(&proc_lock);
			/*
			 * Copy out elem_size, but not larger than kelem_size
			 */
			error = sysctl_copyout(l, kbuf, dp,
			    uimin(kelem_size, elem_size));
			mutex_enter(&proc_lock);
			if (error) {
				goto bah;
			}
			dp += elem_size;
			buflen -= elem_size;
		} else {
			mutex_exit(p->p_lock);
		}
		needed += elem_size;

		/*
		 * Release reference to process.
		 */
	 	if (zombie) {
			next = LIST_NEXT(marker, p_list);
 			LIST_REMOVE(marker, p_list);
		} else {
			rw_exit(&p->p_reflock);
			next = LIST_NEXT(p, p_list);
		}

		/*
		 * Short-circuit break quickly!
		 */
		if (op == KERN_PROC_PID)
                	break;
	}
	mutex_exit(&proc_lock);

	if (where != NULL) {
		*oldlenp = dp - where;
		if (needed > *oldlenp) {
			error = ENOMEM;
			goto out;
		}
	} else {
		needed += KERN_PROCSLOP;
		*oldlenp = needed;
	}
	kmem_free(kbuf, sizeof(*kbuf));
	kmem_free(marker, sizeof(*marker));
	sysctl_relock();
	return 0;
 bah:
 	if (zombie)
 		LIST_REMOVE(marker, p_list);
	else
		rw_exit(&p->p_reflock);
 cleanup:
	mutex_exit(&proc_lock);
 out:
	kmem_free(kbuf, sizeof(*kbuf));
	kmem_free(marker, sizeof(*marker));
	sysctl_relock();
	return error;
}

int
copyin_psstrings(struct proc *p, struct ps_strings *arginfo)
{
#if !defined(_RUMPKERNEL)
	int retval;

	if (p->p_flag & PK_32) {
		MODULE_HOOK_CALL(kern_proc32_copyin_hook, (p, arginfo),
		    enosys(), retval);
		return retval;
	}
#endif /* !defined(_RUMPKERNEL) */

	return copyin_proc(p, (void *)p->p_psstrp, arginfo, sizeof(*arginfo));
}

static int
copy_procargs_sysctl_cb(void *cookie_, const void *src, size_t off, size_t len)
{
	void **cookie = cookie_;
	struct lwp *l = cookie[0];
	char *dst = cookie[1];

	return sysctl_copyout(l, src, dst + off, len);
}

/*
 * sysctl helper routine for kern.proc_args pseudo-subtree.
 */
static int
sysctl_kern_proc_args(SYSCTLFN_ARGS)
{
	struct ps_strings pss;
	struct proc *p;
	pid_t pid;
	int type, error;
	void *cookie[2];

	if (namelen == 1 && name[0] == CTL_QUERY)
		return (sysctl_query(SYSCTLFN_CALL(rnode)));

	if (newp != NULL || namelen != 2)
		return (EINVAL);
	pid = name[0];
	type = name[1];

	switch (type) {
	case KERN_PROC_PATHNAME:
		sysctl_unlock();
		error = fill_pathname(l, pid, oldp, oldlenp);
		sysctl_relock();
		return error;

	case KERN_PROC_CWD:
		sysctl_unlock();
		error = fill_cwd(l, pid, oldp, oldlenp);
		sysctl_relock();
		return error;

	case KERN_PROC_ARGV:
	case KERN_PROC_NARGV:
	case KERN_PROC_ENV:
	case KERN_PROC_NENV:
		/* ok */
		break;
	default:
		return (EINVAL);
	}

	sysctl_unlock();

	/* check pid */
	mutex_enter(&proc_lock);
	if ((p = proc_find(pid)) == NULL) {
		error = EINVAL;
		goto out_locked;
	}
	mutex_enter(p->p_lock);

	/* Check permission. */
	if (type == KERN_PROC_ARGV || type == KERN_PROC_NARGV)
		error = kauth_authorize_process(l->l_cred, KAUTH_PROCESS_CANSEE,
		    p, KAUTH_ARG(KAUTH_REQ_PROCESS_CANSEE_ARGS), NULL, NULL);
	else if (type == KERN_PROC_ENV || type == KERN_PROC_NENV)
		error = kauth_authorize_process(l->l_cred, KAUTH_PROCESS_CANSEE,
		    p, KAUTH_ARG(KAUTH_REQ_PROCESS_CANSEE_ENV), NULL, NULL);
	else
		error = EINVAL; /* XXXGCC */
	if (error) {
		mutex_exit(p->p_lock);
		goto out_locked;
	}

	if (oldp == NULL) {
		if (type == KERN_PROC_NARGV || type == KERN_PROC_NENV)
			*oldlenp = sizeof (int);
		else
			*oldlenp = ARG_MAX;	/* XXX XXX XXX */
		error = 0;
		mutex_exit(p->p_lock);
		goto out_locked;
	}

	/*
	 * Zombies don't have a stack, so we can't read their psstrings.
	 * System processes also don't have a user stack.
	 */
	if (P_ZOMBIE(p) || (p->p_flag & PK_SYSTEM) != 0) {
		error = EINVAL;
		mutex_exit(p->p_lock);
		goto out_locked;
	}

	error = rw_tryenter(&p->p_reflock, RW_READER) ? 0 : EBUSY;
	mutex_exit(p->p_lock);
	if (error) {
		goto out_locked;
	}
	mutex_exit(&proc_lock);

	if (type == KERN_PROC_NARGV || type == KERN_PROC_NENV) {
		int value;
		if ((error = copyin_psstrings(p, &pss)) == 0) {
			if (type == KERN_PROC_NARGV)
				value = pss.ps_nargvstr;
			else
				value = pss.ps_nenvstr;
			error = sysctl_copyout(l, &value, oldp, sizeof(value));
			*oldlenp = sizeof(value);
		}
	} else {
		cookie[0] = l;
		cookie[1] = oldp;
		error = copy_procargs(p, type, oldlenp,
		    copy_procargs_sysctl_cb, cookie);
	}
	rw_exit(&p->p_reflock);
	sysctl_relock();
	return error;

out_locked:
	mutex_exit(&proc_lock);
	sysctl_relock();
	return error;
}

int
copy_procargs(struct proc *p, int oid, size_t *limit,
    int (*cb)(void *, const void *, size_t, size_t), void *cookie)
{
	struct ps_strings pss;
	size_t len, i, loaded, entry_len;
	struct uio auio;
	struct iovec aiov;
	int error, argvlen;
	char *arg;
	char **argv;
	vaddr_t user_argv;
	struct vmspace *vmspace;

	/*
	 * Allocate a temporary buffer to hold the argument vector and
	 * the arguments themselve.
	 */
	arg = kmem_alloc(PAGE_SIZE, KM_SLEEP);
	argv = kmem_alloc(PAGE_SIZE, KM_SLEEP);

	/*
	 * Lock the process down in memory.
	 */
	vmspace = p->p_vmspace;
	uvmspace_addref(vmspace);

	/*
	 * Read in the ps_strings structure.
	 */
	if ((error = copyin_psstrings(p, &pss)) != 0)
		goto done;

	/*
	 * Now read the address of the argument vector.
	 */
	switch (oid) {
	case KERN_PROC_ARGV:
		user_argv = (uintptr_t)pss.ps_argvstr;
		argvlen = pss.ps_nargvstr;
		break;
	case KERN_PROC_ENV:
		user_argv = (uintptr_t)pss.ps_envstr;
		argvlen = pss.ps_nenvstr;
		break;
	default:
		error = EINVAL;
		goto done;
	}

	if (argvlen < 0) {
		error = EIO;
		goto done;
	}


	/*
	 * Now copy each string.
	 */
	len = 0; /* bytes written to user buffer */
	loaded = 0; /* bytes from argv already processed */
	i = 0; /* To make compiler happy */
	entry_len = PROC_PTRSZ(p);

	for (; argvlen; --argvlen) {
		int finished = 0;
		vaddr_t base;
		size_t xlen;
		int j;

		if (loaded == 0) {
			size_t rem = entry_len * argvlen;
			loaded = MIN(rem, PAGE_SIZE);
			error = copyin_vmspace(vmspace,
			    (const void *)user_argv, argv, loaded);
			if (error)
				break;
			user_argv += loaded;
			i = 0;
		}

#if !defined(_RUMPKERNEL)
		if (p->p_flag & PK_32)
			MODULE_HOOK_CALL(kern_proc32_base_hook,
			    (argv, i++), 0, base);
		else
#endif /* !defined(_RUMPKERNEL) */
			base = (vaddr_t)argv[i++];
		loaded -= entry_len;

		/*
		 * The program has messed around with its arguments,
		 * possibly deleting some, and replacing them with
		 * NULL's. Treat this as the last argument and not
		 * a failure.
		 */
		if (base == 0)
			break;

		while (!finished) {
			xlen = PAGE_SIZE - (base & PAGE_MASK);

			aiov.iov_base = arg;
			aiov.iov_len = PAGE_SIZE;
			auio.uio_iov = &aiov;
			auio.uio_iovcnt = 1;
			auio.uio_offset = base;
			auio.uio_resid = xlen;
			auio.uio_rw = UIO_READ;
			UIO_SETUP_SYSSPACE(&auio);
			error = uvm_io(&vmspace->vm_map, &auio, 0);
			if (error)
				goto done;

			/* Look for the end of the string */
			for (j = 0; j < xlen; j++) {
				if (arg[j] == '\0') {
					xlen = j + 1;
					finished = 1;
					break;
				}
			}

			/* Check for user buffer overflow */
			if (len + xlen > *limit) {
				finished = 1;
				if (len > *limit)
					xlen = 0;
				else
					xlen = *limit - len;
			}

			/* Copyout the page */
			error = (*cb)(cookie, arg, len, xlen);
			if (error)
				goto done;

			len += xlen;
			base += xlen;
		}
	}
	*limit = len;

done:
	kmem_free(argv, PAGE_SIZE);
	kmem_free(arg, PAGE_SIZE);
	uvmspace_free(vmspace);
	return error;
}

/*
 * Fill in a proc structure for the specified process.
 */
static void
fill_proc(const struct proc *psrc, struct proc *p, bool allowaddr)
{
	COND_SET_STRUCT(p->p_list, psrc->p_list, allowaddr);
	memset(&p->p_auxlock, 0, sizeof(p->p_auxlock));
	COND_SET_STRUCT(p->p_lock, psrc->p_lock, allowaddr);
	memset(&p->p_stmutex, 0, sizeof(p->p_stmutex));
	memset(&p->p_reflock, 0, sizeof(p->p_reflock));
	COND_SET_STRUCT(p->p_waitcv, psrc->p_waitcv, allowaddr);
	COND_SET_STRUCT(p->p_lwpcv, psrc->p_lwpcv, allowaddr);
	COND_SET_PTR(p->p_cred, psrc->p_cred, allowaddr);
	COND_SET_PTR(p->p_fd, psrc->p_fd, allowaddr);
	COND_SET_PTR(p->p_cwdi, psrc->p_cwdi, allowaddr);
	COND_SET_PTR(p->p_stats, psrc->p_stats, allowaddr);
	COND_SET_PTR(p->p_limit, psrc->p_limit, allowaddr);
	COND_SET_PTR(p->p_vmspace, psrc->p_vmspace, allowaddr);
	COND_SET_PTR(p->p_sigacts, psrc->p_sigacts, allowaddr);
	COND_SET_PTR(p->p_aio, psrc->p_aio, allowaddr);
	p->p_mqueue_cnt = psrc->p_mqueue_cnt;
	memset(&p->p_specdataref, 0, sizeof(p->p_specdataref));
	p->p_exitsig = psrc->p_exitsig;
	p->p_flag = psrc->p_flag;
	p->p_sflag = psrc->p_sflag;
	p->p_slflag = psrc->p_slflag;
	p->p_lflag = psrc->p_lflag;
	p->p_stflag = psrc->p_stflag;
	p->p_stat = psrc->p_stat;
	p->p_trace_enabled = psrc->p_trace_enabled;
	p->p_pid = psrc->p_pid;
	COND_SET_STRUCT(p->p_pglist, psrc->p_pglist, allowaddr);
	COND_SET_PTR(p->p_pptr, psrc->p_pptr, allowaddr);
	COND_SET_STRUCT(p->p_sibling, psrc->p_sibling, allowaddr);
	COND_SET_STRUCT(p->p_children, psrc->p_children, allowaddr);
	COND_SET_STRUCT(p->p_lwps, psrc->p_lwps, allowaddr);
	COND_SET_PTR(p->p_raslist, psrc->p_raslist, allowaddr);
	p->p_nlwps = psrc->p_nlwps;
	p->p_nzlwps = psrc->p_nzlwps;
	p->p_nrlwps = psrc->p_nrlwps;
	p->p_nlwpwait = psrc->p_nlwpwait;
	p->p_ndlwps = psrc->p_ndlwps;
	p->p_nstopchild = psrc->p_nstopchild;
	p->p_waited = psrc->p_waited;
	COND_SET_PTR(p->p_zomblwp, psrc->p_zomblwp, allowaddr);
	COND_SET_PTR(p->p_vforklwp, psrc->p_vforklwp, allowaddr);
	COND_SET_PTR(p->p_sched_info, psrc->p_sched_info, allowaddr);
	p->p_estcpu = psrc->p_estcpu;
	p->p_estcpu_inherited = psrc->p_estcpu_inherited;
	p->p_forktime = psrc->p_forktime;
	p->p_pctcpu = psrc->p_pctcpu;
	COND_SET_PTR(p->p_opptr, psrc->p_opptr, allowaddr);
	COND_SET_PTR(p->p_timers, psrc->p_timers, allowaddr);
	p->p_rtime = psrc->p_rtime;
	p->p_uticks = psrc->p_uticks;
	p->p_sticks = psrc->p_sticks;
	p->p_iticks = psrc->p_iticks;
	p->p_xutime = psrc->p_xutime;
	p->p_xstime = psrc->p_xstime;
	p->p_traceflag = psrc->p_traceflag;
	COND_SET_PTR(p->p_tracep, psrc->p_tracep, allowaddr);
	COND_SET_PTR(p->p_textvp, psrc->p_textvp, allowaddr);
	COND_SET_PTR(p->p_emul, psrc->p_emul, allowaddr);
	COND_SET_PTR(p->p_emuldata, psrc->p_emuldata, allowaddr);
	COND_SET_CPTR(p->p_execsw, psrc->p_execsw, allowaddr);
	COND_SET_STRUCT(p->p_klist, psrc->p_klist, allowaddr);
	COND_SET_STRUCT(p->p_sigwaiters, psrc->p_sigwaiters, allowaddr);
	COND_SET_STRUCT(p->p_sigpend.sp_info, psrc->p_sigpend.sp_info,
	    allowaddr);
	p->p_sigpend.sp_set = psrc->p_sigpend.sp_set;
	COND_SET_PTR(p->p_lwpctl, psrc->p_lwpctl, allowaddr);
	p->p_ppid = psrc->p_ppid;
	p->p_oppid = psrc->p_oppid;
	COND_SET_PTR(p->p_path, psrc->p_path, allowaddr);
	p->p_sigctx = psrc->p_sigctx;
	p->p_nice = psrc->p_nice;
	memcpy(p->p_comm, psrc->p_comm, sizeof(p->p_comm));
	COND_SET_PTR(p->p_pgrp, psrc->p_pgrp, allowaddr);
	COND_SET_VALUE(p->p_psstrp, psrc->p_psstrp, allowaddr);
	p->p_pax = psrc->p_pax;
	p->p_xexit = psrc->p_xexit;
	p->p_xsig = psrc->p_xsig;
	p->p_acflag = psrc->p_acflag;
	COND_SET_STRUCT(p->p_md, psrc->p_md, allowaddr);
	p->p_stackbase = psrc->p_stackbase;
	COND_SET_PTR(p->p_dtrace, psrc->p_dtrace, allowaddr);
}

/*
 * Fill in an eproc structure for the specified process.
 */
void
fill_eproc(struct proc *p, struct eproc *ep, bool zombie, bool allowaddr)
{
	struct tty *tp;
	struct lwp *l;

	KASSERT(mutex_owned(&proc_lock));
	KASSERT(mutex_owned(p->p_lock));

	COND_SET_PTR(ep->e_paddr, p, allowaddr);
	COND_SET_PTR(ep->e_sess, p->p_session, allowaddr);
	if (p->p_cred) {
		kauth_cred_topcred(p->p_cred, &ep->e_pcred);
		kauth_cred_toucred(p->p_cred, &ep->e_ucred);
	}
	if (p->p_stat != SIDL && !P_ZOMBIE(p) && !zombie) {
		struct vmspace *vm = p->p_vmspace;

		ep->e_vm.vm_rssize = vm_resident_count(vm);
		ep->e_vm.vm_tsize = vm->vm_tsize;
		ep->e_vm.vm_dsize = vm->vm_dsize;
		ep->e_vm.vm_ssize = vm->vm_ssize;
		ep->e_vm.vm_map.size = vm->vm_map.size;

		/* Pick the primary (first) LWP */
		l = proc_active_lwp(p);
		KASSERT(l != NULL);
		lwp_lock(l);
		if (l->l_wchan)
			strncpy(ep->e_wmesg, l->l_wmesg, WMESGLEN);
		lwp_unlock(l);
	}
	ep->e_ppid = p->p_ppid;
	if (p->p_pgrp && p->p_session) {
		ep->e_pgid = p->p_pgrp->pg_id;
		ep->e_jobc = p->p_pgrp->pg_jobc;
		ep->e_sid = p->p_session->s_sid;
		if ((p->p_lflag & PL_CONTROLT) &&
		    (tp = p->p_session->s_ttyp)) {
			ep->e_tdev = tp->t_dev;
			ep->e_tpgid = tp->t_pgrp ? tp->t_pgrp->pg_id : NO_PGID;
			COND_SET_PTR(ep->e_tsess, tp->t_session, allowaddr);
		} else
			ep->e_tdev = (uint32_t)NODEV;
		ep->e_flag = p->p_session->s_ttyvp ? EPROC_CTTY : 0;
		if (SESS_LEADER(p))
			ep->e_flag |= EPROC_SLEADER;
		strncpy(ep->e_login, p->p_session->s_login, MAXLOGNAME);
	}
	ep->e_xsize = ep->e_xrssize = 0;
	ep->e_xccount = ep->e_xswrss = 0;
}

/*
 * Fill in a kinfo_proc2 structure for the specified process.
 */
void
fill_kproc2(struct proc *p, struct kinfo_proc2 *ki, bool zombie, bool allowaddr)
{
	struct tty *tp;
	struct lwp *l, *l2;
	struct timeval ut, st, rt;
	sigset_t ss1, ss2;
	struct rusage ru;
	struct vmspace *vm;

	KASSERT(mutex_owned(&proc_lock));
	KASSERT(mutex_owned(p->p_lock));

	sigemptyset(&ss1);
	sigemptyset(&ss2);

	COND_SET_VALUE(ki->p_paddr, PTRTOUINT64(p), allowaddr);
	COND_SET_VALUE(ki->p_fd, PTRTOUINT64(p->p_fd), allowaddr);
	COND_SET_VALUE(ki->p_cwdi, PTRTOUINT64(p->p_cwdi), allowaddr);
	COND_SET_VALUE(ki->p_stats, PTRTOUINT64(p->p_stats), allowaddr);
	COND_SET_VALUE(ki->p_limit, PTRTOUINT64(p->p_limit), allowaddr);
	COND_SET_VALUE(ki->p_vmspace, PTRTOUINT64(p->p_vmspace), allowaddr);
	COND_SET_VALUE(ki->p_sigacts, PTRTOUINT64(p->p_sigacts), allowaddr);
	COND_SET_VALUE(ki->p_sess, PTRTOUINT64(p->p_session), allowaddr);
	ki->p_tsess = 0;	/* may be changed if controlling tty below */
	COND_SET_VALUE(ki->p_ru, PTRTOUINT64(&p->p_stats->p_ru), allowaddr);
	ki->p_eflag = 0;
	ki->p_exitsig = p->p_exitsig;
	ki->p_flag = L_INMEM;   /* Process never swapped out */
	ki->p_flag |= sysctl_map_flags(sysctl_flagmap, p->p_flag);
	ki->p_flag |= sysctl_map_flags(sysctl_sflagmap, p->p_sflag);
	ki->p_flag |= sysctl_map_flags(sysctl_slflagmap, p->p_slflag);
	ki->p_flag |= sysctl_map_flags(sysctl_lflagmap, p->p_lflag);
	ki->p_flag |= sysctl_map_flags(sysctl_stflagmap, p->p_stflag);
	ki->p_pid = p->p_pid;
	ki->p_ppid = p->p_ppid;
	ki->p_uid = kauth_cred_geteuid(p->p_cred);
	ki->p_ruid = kauth_cred_getuid(p->p_cred);
	ki->p_gid = kauth_cred_getegid(p->p_cred);
	ki->p_rgid = kauth_cred_getgid(p->p_cred);
	ki->p_svuid = kauth_cred_getsvuid(p->p_cred);
	ki->p_svgid = kauth_cred_getsvgid(p->p_cred);
	ki->p_ngroups = kauth_cred_ngroups(p->p_cred);
	kauth_cred_getgroups(p->p_cred, ki->p_groups,
	    uimin(ki->p_ngroups, sizeof(ki->p_groups) / sizeof(ki->p_groups[0])),
	    UIO_SYSSPACE);

	ki->p_uticks = p->p_uticks;
	ki->p_sticks = p->p_sticks;
	ki->p_iticks = p->p_iticks;
	ki->p_tpgid = NO_PGID;	/* may be changed if controlling tty below */
	COND_SET_VALUE(ki->p_tracep, PTRTOUINT64(p->p_tracep), allowaddr);
	ki->p_traceflag = p->p_traceflag;

	memcpy(&ki->p_sigignore, &p->p_sigctx.ps_sigignore,sizeof(ki_sigset_t));
	memcpy(&ki->p_sigcatch, &p->p_sigctx.ps_sigcatch, sizeof(ki_sigset_t));

	ki->p_cpticks = 0;
	ki->p_pctcpu = p->p_pctcpu;
	ki->p_estcpu = 0;
	ki->p_stat = p->p_stat; /* Will likely be overridden by LWP status */
	ki->p_realstat = p->p_stat;
	ki->p_nice = p->p_nice;
	ki->p_xstat = P_WAITSTATUS(p);
	ki->p_acflag = p->p_acflag;

	strncpy(ki->p_comm, p->p_comm,
	    uimin(sizeof(ki->p_comm), sizeof(p->p_comm)));
	strncpy(ki->p_ename, p->p_emul->e_name, sizeof(ki->p_ename));

	ki->p_nlwps = p->p_nlwps;
	ki->p_realflag = ki->p_flag;

	if (p->p_stat != SIDL && !P_ZOMBIE(p) && !zombie) {
		vm = p->p_vmspace;
		ki->p_vm_rssize = vm_resident_count(vm);
		ki->p_vm_tsize = vm->vm_tsize;
		ki->p_vm_dsize = vm->vm_dsize;
		ki->p_vm_ssize = vm->vm_ssize;
		ki->p_vm_vsize = atop(vm->vm_map.size);
		/*
		 * Since the stack is initially mapped mostly with
		 * PROT_NONE and grown as needed, adjust the "mapped size"
		 * to skip the unused stack portion.
		 */
		ki->p_vm_msize =
		    atop(vm->vm_map.size) - vm->vm_issize + vm->vm_ssize;

		/* Pick the primary (first) LWP */
		l = proc_active_lwp(p);
		KASSERT(l != NULL);
		lwp_lock(l);
		ki->p_nrlwps = p->p_nrlwps;
		ki->p_forw = 0;
		ki->p_back = 0;
		COND_SET_VALUE(ki->p_addr, PTRTOUINT64(l->l_addr), allowaddr);
		ki->p_stat = l->l_stat;
		ki->p_flag |= sysctl_map_flags(sysctl_lwpflagmap, l->l_flag);
		ki->p_swtime = l->l_swtime;
		ki->p_slptime = l->l_slptime;
		if (l->l_stat == LSONPROC)
			ki->p_schedflags = l->l_cpu->ci_schedstate.spc_flags;
		else
			ki->p_schedflags = 0;
		ki->p_priority = lwp_eprio(l);
		ki->p_usrpri = l->l_priority;
		if (l->l_wchan)
			strncpy(ki->p_wmesg, l->l_wmesg, sizeof(ki->p_wmesg));
		COND_SET_VALUE(ki->p_wchan, PTRTOUINT64(l->l_wchan), allowaddr);
		ki->p_cpuid = cpu_index(l->l_cpu);
		lwp_unlock(l);
		LIST_FOREACH(l, &p->p_lwps, l_sibling) {
			/* This is hardly correct, but... */
			sigplusset(&l->l_sigpend.sp_set, &ss1);
			sigplusset(&l->l_sigmask, &ss2);
			ki->p_cpticks += l->l_cpticks;
			ki->p_pctcpu += l->l_pctcpu;
			ki->p_estcpu += l->l_estcpu;
		}
	}
	sigplusset(&p->p_sigpend.sp_set, &ss1);
	memcpy(&ki->p_siglist, &ss1, sizeof(ki_sigset_t));
	memcpy(&ki->p_sigmask, &ss2, sizeof(ki_sigset_t));

	if (p->p_session != NULL) {
		ki->p_sid = p->p_session->s_sid;
		ki->p__pgid = p->p_pgrp->pg_id;
		if (p->p_session->s_ttyvp)
			ki->p_eflag |= EPROC_CTTY;
		if (SESS_LEADER(p))
			ki->p_eflag |= EPROC_SLEADER;
		strncpy(ki->p_login, p->p_session->s_login,
		    uimin(sizeof ki->p_login - 1, sizeof p->p_session->s_login));
		ki->p_jobc = p->p_pgrp->pg_jobc;
		if ((p->p_lflag & PL_CONTROLT) && (tp = p->p_session->s_ttyp)) {
			ki->p_tdev = tp->t_dev;
			ki->p_tpgid = tp->t_pgrp ? tp->t_pgrp->pg_id : NO_PGID;
			COND_SET_VALUE(ki->p_tsess, PTRTOUINT64(tp->t_session),
			    allowaddr);
		} else {
			ki->p_tdev = (int32_t)NODEV;
		}
	}

	if (!P_ZOMBIE(p) && !zombie) {
		ki->p_uvalid = 1;
		ki->p_ustart_sec = p->p_stats->p_start.tv_sec;
		ki->p_ustart_usec = p->p_stats->p_start.tv_usec;

		calcru(p, &ut, &st, NULL, &rt);
		ki->p_rtime_sec = rt.tv_sec;
		ki->p_rtime_usec = rt.tv_usec;
		ki->p_uutime_sec = ut.tv_sec;
		ki->p_uutime_usec = ut.tv_usec;
		ki->p_ustime_sec = st.tv_sec;
		ki->p_ustime_usec = st.tv_usec;

		memcpy(&ru, &p->p_stats->p_ru, sizeof(ru));
		ki->p_uru_nvcsw = 0;
		ki->p_uru_nivcsw = 0;
		LIST_FOREACH(l2, &p->p_lwps, l_sibling) {
			ki->p_uru_nvcsw += (l2->l_ncsw - l2->l_nivcsw);
			ki->p_uru_nivcsw += l2->l_nivcsw;
			ruadd(&ru, &l2->l_ru);
		}
		ki->p_uru_maxrss = ru.ru_maxrss;
		ki->p_uru_ixrss = ru.ru_ixrss;
		ki->p_uru_idrss = ru.ru_idrss;
		ki->p_uru_isrss = ru.ru_isrss;
		ki->p_uru_minflt = ru.ru_minflt;
		ki->p_uru_majflt = ru.ru_majflt;
		ki->p_uru_nswap = ru.ru_nswap;
		ki->p_uru_inblock = ru.ru_inblock;
		ki->p_uru_oublock = ru.ru_oublock;
		ki->p_uru_msgsnd = ru.ru_msgsnd;
		ki->p_uru_msgrcv = ru.ru_msgrcv;
		ki->p_uru_nsignals = ru.ru_nsignals;

		timeradd(&p->p_stats->p_cru.ru_utime,
			 &p->p_stats->p_cru.ru_stime, &ut);
		ki->p_uctime_sec = ut.tv_sec;
		ki->p_uctime_usec = ut.tv_usec;
	}
}


int
proc_find_locked(struct lwp *l, struct proc **p, pid_t pid)
{
	int error;

	mutex_enter(&proc_lock);
	if (pid == -1)
		*p = l->l_proc;
	else
		*p = proc_find(pid);

	if (*p == NULL) {
		if (pid != -1)
			mutex_exit(&proc_lock);
		return ESRCH;
	}
	if (pid != -1)
		mutex_enter((*p)->p_lock);
	mutex_exit(&proc_lock);

	error = kauth_authorize_process(l->l_cred,
	    KAUTH_PROCESS_CANSEE, *p,
	    KAUTH_ARG(KAUTH_REQ_PROCESS_CANSEE_ENTRY), NULL, NULL);
	if (error) {
		if (pid != -1)
			mutex_exit((*p)->p_lock);
	}
	return error;
}

static int
fill_pathname(struct lwp *l, pid_t pid, void *oldp, size_t *oldlenp)
{
	int error;
	struct proc *p;

	if ((error = proc_find_locked(l, &p, pid)) != 0)
		return error;

	if (p->p_path == NULL) {
		if (pid != -1)
			mutex_exit(p->p_lock);
		return ENOENT;
	}

	size_t len = strlen(p->p_path) + 1;
	if (oldp != NULL) {
		size_t copylen = uimin(len, *oldlenp);
		error = sysctl_copyout(l, p->p_path, oldp, copylen);
		if (error == 0 && *oldlenp < len)
			error = ENOSPC;
	}
	*oldlenp = len;
	if (pid != -1)
		mutex_exit(p->p_lock);
	return error;
}

static int
fill_cwd(struct lwp *l, pid_t pid, void *oldp, size_t *oldlenp)
{
	int error;
	struct proc *p;
	char *path;
	char *bp, *bend;
	struct cwdinfo *cwdi;
	struct vnode *vp;
	size_t len, lenused;

	if ((error = proc_find_locked(l, &p, pid)) != 0)
		return error;

	len = MAXPATHLEN * 4;

	path = kmem_alloc(len, KM_SLEEP);

	bp = &path[len];
	bend = bp;
	*(--bp) = '\0';

	cwdi = p->p_cwdi;
	rw_enter(&cwdi->cwdi_lock, RW_READER);
	vp = cwdi->cwdi_cdir;
	error = getcwd_common(vp, NULL, &bp, path, len/2, 0, l);
	rw_exit(&cwdi->cwdi_lock);

	if (error)
		goto out;

	lenused = bend - bp;

	if (oldp != NULL) {
		size_t copylen = uimin(lenused, *oldlenp);
		error = sysctl_copyout(l, bp, oldp, copylen);
		if (error == 0 && *oldlenp < lenused)
			error = ENOSPC;
	}
	*oldlenp = lenused;
out:
	if (pid != -1)
		mutex_exit(p->p_lock);
	kmem_free(path, len);
	return error;
}

int
proc_getauxv(struct proc *p, void **buf, size_t *len)
{
	struct ps_strings pss;
	int error;
	void *uauxv, *kauxv;
	size_t size;

	if ((error = copyin_psstrings(p, &pss)) != 0)
		return error;
	if (pss.ps_envstr == NULL)
		return EIO;

	size = p->p_execsw->es_arglen;
	if (size == 0)
		return EIO;

	size_t ptrsz = PROC_PTRSZ(p);
	uauxv = (void *)((char *)pss.ps_envstr + (pss.ps_nenvstr + 1) * ptrsz);

	kauxv = kmem_alloc(size, KM_SLEEP);

	error = copyin_proc(p, uauxv, kauxv, size);
	if (error) {
		kmem_free(kauxv, size);
		return error;
	}

	*buf = kauxv;
	*len = size;

	return 0;
}


static int
sysctl_security_expose_address(SYSCTLFN_ARGS)
{
	int expose_address, error;
	struct sysctlnode node;

	node = *rnode;
	node.sysctl_data = &expose_address;
	expose_address = *(int *)rnode->sysctl_data;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_KERNADDR,
	    0, NULL, NULL, NULL))
		return EPERM;

	switch (expose_address) {
	case 0:
	case 1:
	case 2:
		break;
	default:
		return EINVAL;
	}

	*(int *)rnode->sysctl_data = expose_address;

	return 0;
}

bool
get_expose_address(struct proc *p)
{
	/* allow only if sysctl variable is set or privileged */
	return kauth_authorize_process(kauth_cred_get(), KAUTH_PROCESS_CANSEE,
	    p, KAUTH_ARG(KAUTH_REQ_PROCESS_CANSEE_KPTR), NULL, NULL) == 0;
}
