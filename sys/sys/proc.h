/*	$NetBSD: proc.h,v 1.222 2006/05/16 00:08:25 elad Exp $	*/

/*-
 * Copyright (c) 1986, 1989, 1991, 1993
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
 *	@(#)proc.h	8.15 (Berkeley) 5/19/95
 */

#ifndef _SYS_PROC_H_
#define	_SYS_PROC_H_

#if defined(_KERNEL_OPT)
#include "opt_multiprocessor.h"
#include "opt_kstack.h"
#endif

#include <machine/proc.h>		/* Machine-dependent proc substruct */
#include <sys/lock.h>
#include <sys/lwp.h>
#include <sys/queue.h>
#include <sys/callout.h>
#include <sys/signalvar.h>
#include <sys/siginfo.h>
#include <sys/event.h>

#ifndef _KERNEL
#include <sys/time.h>
#include <sys/resource.h>
#endif

/*
 * One structure allocated per session.
 */
struct session {
	int		s_count;	/* Ref cnt; pgrps in session */
	u_int		s_flags;
#define	S_LOGIN_SET	1		/* s_login set in this session */
	struct proc	*s_leader;	/* Session leader */
	struct vnode	*s_ttyvp;	/* Vnode of controlling terminal */
	struct tty	*s_ttyp;	/* Controlling terminal */
	char		s_login[MAXLOGNAME]; /* Setlogin() name */
	pid_t		s_sid;		/* Session ID (pid of leader) */
};

/*
 * One structure allocated per process group.
 */
struct pgrp {
	LIST_HEAD(, proc) pg_members;	/* Pointer to pgrp members */
	struct session	*pg_session;	/* Pointer to session */
	pid_t		pg_id;		/* Pgrp id */
	int		pg_jobc;	/*
					 * Number of processes qualifying
					 * pgrp for job control
					 */
};

/*
 * One structure allocated per emulation.
 */
struct exec_package;
struct ps_strings;
struct ras;
struct sa_emul;
struct kauth_cred;

struct emul {
	const char	*e_name;	/* Symbolic name */
	const char	*e_path;	/* Extra emulation path (NULL if none)*/
#ifndef __HAVE_MINIMAL_EMUL
	int		e_flags;	/* Miscellaneous flags, see above */
					/* Syscall handling function */
	const int	*e_errno;	/* Errno array */
	int		e_nosys;	/* Offset of the nosys() syscall */
	int		e_nsysent;	/* Number of system call entries */
#endif
	const struct sysent *e_sysent;	/* System call array */
	const char * const *e_syscallnames; /* System call name array */
					/* Signal sending function */
	void		(*e_sendsig)(const struct ksiginfo *,
					  const sigset_t *);
	void		(*e_trapsignal)(struct lwp *, const struct ksiginfo *);
	int		(*e_tracesig)(struct proc *, int);
	char		*e_sigcode;	/* Start of sigcode */
	char		*e_esigcode;	/* End of sigcode */
					/* Set registers before execution */
	struct uvm_object **e_sigobject;/* shared sigcode object */
	void		(*e_setregs)(struct lwp *, struct exec_package *,
					  u_long);

					/* Per-process hooks */
	void		(*e_proc_exec)(struct proc *, struct exec_package *);
	void		(*e_proc_fork)(struct proc *, struct proc *, int);
	void		(*e_proc_exit)(struct proc *);
	void		(*e_lwp_fork)(struct lwp *, struct lwp *);
	void		(*e_lwp_exit)(struct lwp *);

#ifdef __HAVE_SYSCALL_INTERN
	void		(*e_syscall_intern)(struct proc *);
#else
	void		(*e_syscall)(void);
#endif
					/* Emulation specific sysctl data */
	struct sysctlnode *e_sysctlovly;
	int		(*e_fault)(struct proc *, vaddr_t, int);

	vaddr_t		(*e_vm_default_addr)(struct proc *, vaddr_t, vsize_t);

	/* Emulation-specific hook for userspace page faults */
	int		(*e_usertrap)(struct lwp *, vaddr_t, void *);

	/* SA-related information */
	const struct sa_emul *e_sa;
};

/*
 * Emulation miscelaneous flags
 */
#define	EMUL_HAS_SYS___syscall	0x001	/* Has SYS___syscall */

/*
 * Description of a process.
 *
 * This structure contains the information needed to manage a thread of
 * control, known in UN*X as a process; it has references to substructures
 * containing descriptions of things that the process uses, but may share
 * with related processes.  The process structure and the substructures
 * are always addressible except for those marked "(PROC ONLY)" below,
 * which might be addressible only on a processor on which the process
 * is running.
 *
 * Fields marked 'p:' are protected by the process's own p_lock.
 * Fields marked 'l:' are protected by the proclist_lock
 * Fields marked 's:' are protected by the SCHED_LOCK.
 */
struct proc {
	LIST_ENTRY(proc) p_list;	/* List of all processes */

	/* Substructures: */
	struct kauth_cred *p_cred;	/* Credentials */
	struct filedesc	*p_fd;		/* Ptr to open files structure */
	struct cwdinfo	*p_cwdi;	/* cdir/rdir/cmask info */
	struct pstats	*p_stats;	/* Accounting/statistics (PROC ONLY) */
	struct plimit	*p_limit;	/* Process limits */
	struct vmspace	*p_vmspace;	/* Address space */
	struct sigacts	*p_sigacts;	/* Process sigactions (state is below)*/

	void		*p_ksems;	/* p1003.1b semaphores */
#define	p_rlimit	p_limit->pl_rlimit

	int		p_exitsig;	/* signal to send to parent on exit */
	int		p_flag;		/* P_* flags. */
	char		p_stat;		/* S* process status. */
	char		p_pad1[3];

	pid_t		p_pid;		/* Process identifier. */
	LIST_ENTRY(proc) p_pglist;	/* l: List of processes in pgrp. */
	struct proc 	*p_pptr;	/* l: Pointer to parent process. */
	LIST_ENTRY(proc) p_sibling;	/* l: List of sibling processes. */
	LIST_HEAD(, proc) p_children;	/* l: Pointer to list of children. */

	struct simplelock p_lock;	/* Lock on proc state (p:) */

	/* XXX dsl: locking of LWP info is suspect in schedcpu and kpsignal2 */
	LIST_HEAD(, lwp) p_lwps;	/* p: Pointer to list of LWPs. */

	LIST_HEAD(, ras) p_raslist;	/* p: Pointer to RAS queue */

/* The following fields are all zeroed upon creation in fork. */
#define	p_startzero	p_nlwps

	int 		p_nlwps;	/* p: Number of LWPs */
	int 		p_nrlwps;	/* s: Number of running LWPs */
	int 		p_nzlwps;	/* p: Number of zombie LWPs */
	int 		p_nlwpid;	/* p: Next LWP ID */

	u_int		p_nstopchild;	/* l: Count of stopped/dead children */

	struct sadata 	*p_sa;		/* Scheduler activation information */

	/* scheduling */
	fixpt_t		p_estcpu;	/* Time averaged value of p_cpticks XXX belongs in p_startcopy section */
	fixpt_t		p_estcpu_inherited;
	unsigned int	p_forktime;
	int		p_cpticks;	/* Ticks of CPU time */
	fixpt_t		p_pctcpu;	/* %cpu for this process during p_swtime */

	struct proc	*p_opptr;	/* Save parent during ptrace. */
	struct ptimers	*p_timers;	/* Timers: real, virtual, profiling */
	struct timeval 	p_rtime;	/* Real time */
	u_quad_t 	p_uticks;	/* Statclock hits in user mode */
	u_quad_t 	p_sticks;	/* Statclock hits in system mode */
	u_quad_t 	p_iticks;	/* Statclock hits processing intr */

	int		p_traceflag;	/* Kernel trace points */
	void		*p_tracep;	/* Trace private data */
	void		*p_systrace;	/* Back pointer to systrace */

	struct vnode 	*p_textvp;	/* Vnode of executable */

	const struct emul *p_emul;	/* Emulation information */
	void		*p_emuldata;	/* Per-process emulation data, or NULL.
					 * Malloc type M_EMULDATA
					 */

	void 		(*p_userret)(struct lwp *, void *);
					/* Function to call at userret(). */
	void		*p_userret_arg;

	const struct execsw *p_execsw;	/* Exec package information */
	struct klist	p_klist;	/* Knotes attached to this process */

/*
 * End area that is zeroed on creation
 */
#define	p_endzero	p_startcopy

/*
 * The following fields are all copied upon creation in fork.
 */
#define	p_startcopy	p_sigctx.ps_startcopy

	struct sigctx 	p_sigctx;	/* Signal state */

	u_char		p_nice;		/* Process "nice" value */
	char		p_comm[MAXCOMLEN+1];	/* basename of last exec file */

	struct pgrp 	*p_pgrp;	/* Pointer to process group */

	struct ps_strings *p_psstr;	/* address of process's ps_strings */
	size_t 		p_psargv;	/* offset of ps_argvstr in above */
	size_t 		p_psnargv;	/* offset of ps_nargvstr in above */
	size_t 		p_psenv;	/* offset of ps_envstr in above */
	size_t 		p_psnenv;	/* offset of ps_nenvstr in above */

/*
 * End area that is copied on creation
 */
#define	p_endcopy	p_xstat

	u_short		p_xstat;	/* Exit status for wait; also stop signal */
	u_short		p_acflag;	/* Accounting flags */
	struct rusage 	*p_ru;		/* Exit information. XXX */

	struct mdproc	p_md;		/* Any machine-dependent fields */
};

#define	p_session	p_pgrp->pg_session
#define	p_pgid		p_pgrp->pg_id

/*
 * Status values.
 *
 */
#define	SIDL		1		/* Process being created by fork */
#define	SACTIVE		2		/* Process is not stopped */
#define	SSTOP		4		/* Process debugging or suspension */
#define	SZOMB		5		/* Awaiting collection by parent */

#define	P_ZOMBIE(p)	((p)->p_stat == SZOMB)

/* These flags are kept in p_flag. */
#define	P_ADVLOCK	0x00000001 /* Process may hold a POSIX advisory lock */
#define	P_CONTROLT	0x00000002 /* Has a controlling terminal */
#define	P_INMEM	     /* 0x00000004 */	L_INMEM
#define	P_NOCLDSTOP	0x00000008 /* No SIGCHLD when children stop */
#define	P_PPWAIT	0x00000010 /* Parent is waiting for child exec/exit */
#define	P_PROFIL	0x00000020 /* Has started profiling */
#define	P_SELECT     /* 0x00000040 */	L_SELECT
#define	P_SINTR	     /* 0x00000080 */	L_SINTR
#define	P_SUGID		0x00000100 /* Had set id privileges since last exec */
#define	P_SYSTEM	0x00000200 /* System proc: no sigs, stats or swapping */
#define	P_SA	     /* 0x00000400 */	L_SA
#define	P_TRACED	0x00000800 /* Debugged process being traced */
#define	P_WAITED	0x00001000 /* Debugging process has waited for child */
#define	P_WEXIT		0x00002000 /* Working on exiting */
#define	P_EXEC		0x00004000 /* Process called exec */
#define	P_OWEUPC	0x00008000 /* Owe process an addupc() at next ast */
#define	P_FSTRACE	0x00010000 /* Debugger process being traced by procfs */
#define	P_NOCLDWAIT	0x00020000 /* No zombies if child dies */
#define	P_32		0x00040000 /* 32-bit process (used on 64-bit kernels) */
#define	P_CLDSIGIGN	0x00080000 /* Process is ignoring SIGCHLD */
#define	P_INEXEC	0x00100000 /* Process is exec'ing and can't be traced */
#define	P_SYSTRACE	0x00200000 /* Process system call tracing active */
#define	P_CHTRACED	0x00400000 /* Child has been traced & reparented */
#define	P_STOPFORK	0x00800000 /* Child will be stopped on fork(2) */
#define	P_STOPEXEC	0x01000000 /* Will be stopped on exec(2) */
#define	P_STOPEXIT	0x02000000 /* Will be stopped at process exit */
#define	P_SYSCALL	0x04000000 /* process has PT_SYSCALL enabled */
#define	P_PAXMPROTECT  	0x08000000 /* Explicitly enable PaX MPROTECT */
#define	P_PAXNOMPROTECT	0x10000000 /* Explicitly disable PaX MPROTECT */
#define	P_UNUSED2	0x20000000
#define	P_UNUSED1	0x40000000
#define	P_MARKER	0x80000000 /* Is a dummy marker process */

#define	P_SHARED	(L_INMEM|L_SELECT|L_SINTR|L_SA)

/*
 * Macro to compute the exit signal to be delivered.
 */
#define	P_EXITSIG(p)	(((p)->p_flag & (P_TRACED|P_FSTRACE)) ? SIGCHLD : \
			 p->p_exitsig)

/*
 * MOVE TO ucred.h?
 *
 * Shareable process credentials (always resident).  This includes a reference
 * to the current user credentials as well as real and saved ids that may be
 * used to change ids.
 */
struct pcred {
	struct ucred	*pc_ucred;	/* Current credentials */
	uid_t		p_ruid;		/* Real user id */
	uid_t		p_svuid;	/* Saved effective user id */
	gid_t		p_rgid;		/* Real group id */
	gid_t		p_svgid;	/* Saved effective group id */
	int		p_refcnt;	/* Number of references */
};

LIST_HEAD(proclist, proc);		/* A list of processes */

/*
 * This structure associates a proclist with its lock.
 */
struct proclist_desc {
	struct proclist	*pd_list;	/* The list */
	/*
	 * XXX Add a pointer to the proclist's lock eventually.
	 */
};

#ifdef _KERNEL
#include <sys/mallocvar.h>
MALLOC_DECLARE(M_EMULDATA);
MALLOC_DECLARE(M_PROC);
MALLOC_DECLARE(M_SESSION);
MALLOC_DECLARE(M_SUBPROC);	/* XXX - only used by sparc/sparc64 */

/*
 * We use process IDs <= PID_MAX until there are > 16k processes.
 * NO_PGID is used to represent "no process group" for a tty.
 */
#define	PID_MAX		30000
#define	NO_PGID		((pid_t)-1)

#define	SESS_LEADER(p)	((p)->p_session->s_leader == (p))
#define	SESSHOLD(s)	((s)->s_count++)
#define	SESSRELE(s)							\
do {									\
	if (--(s)->s_count == 0)					\
		sessdelete(s);						\
} while (/* CONSTCOND */ 0)


/*
 * Flags passed to fork1().
 */
#define	FORK_PPWAIT	0x01		/* Block parent until child exit */
#define	FORK_SHAREVM	0x02		/* Share vmspace with parent */
#define	FORK_SHARECWD	0x04		/* Share cdir/rdir/cmask */
#define	FORK_SHAREFILES	0x08		/* Share file descriptors */
#define	FORK_SHARESIGS	0x10		/* Share signal actions */
#define	FORK_NOWAIT	0x20		/* Make init the parent of the child */
#define	FORK_CLEANFILES	0x40		/* Start with a clean descriptor set */

/*
 * Allow machine-dependent code to override curproc in <machine/cpu.h> for
 * its own convenience.  Otherwise, we declare it as appropriate.
 */
#if !defined(curlwp)
#if defined(MULTIPROCESSOR)
#define	curlwp		curcpu()->ci_curlwp	/* Current running LWP */
#else
extern struct lwp	*curlwp;		/* Current running LWP */
#endif /* MULTIPROCESSOR */
#endif /* ! curproc */

static struct proc *__curproc(void);

static __inline struct proc *
__curproc()
{
	struct lwp *l = curlwp;

	if (l == NULL)
		return NULL;
	return l->l_proc;
}
#define	curproc	__curproc()

extern struct proc	proc0;		/* Process slot for swapper */
extern int		nprocs, maxproc; /* Current and max number of procs */
#define	vmspace_kernel()	(proc0.p_vmspace)

/* Process list lock; see kern_proc.c for locking protocol details */
extern struct lock	proclist_lock;

extern struct proclist	allproc;	/* List of all processes */
extern struct proclist	zombproc;	/* List of zombie processes */

extern SLIST_HEAD(deadprocs, proc) deadprocs;	/* List of dead processes */
extern struct simplelock deadproc_slock;

extern struct proc	*initproc;	/* Process slots for init, pager */

extern const struct proclist_desc proclists[];

extern struct pool	pcred_pool;	/* Memory pool for pcreds */
extern struct pool	plimit_pool;	/* Memory pool for plimits */
extern struct pool 	pstats_pool;	/* memory pool for pstats */
extern struct pool	rusage_pool;	/* Memory pool for rusages */
extern struct pool	ptimer_pool;	/* Memory pool for ptimers */

struct proc *p_find(pid_t, uint);	/* Find process by id */
struct pgrp *pg_find(pid_t, uint);	/* Find process group by id */
/* Flags values for p_find() and pg_find(). */
#define PFIND_ZOMBIE		1	/* look for zombies as well */
#define PFIND_LOCKED		2	/* proclist locked on entry */
#define PFIND_UNLOCK_FAIL	4	/* unlock proclist on failure */
#define PFIND_UNLOCK_OK		8	/* unlock proclist on success */
#define PFIND_UNLOCK		(PFIND_UNLOCK_OK | PFIND_UNLOCK_FAIL)
/* For source compatibility. but UNLOCK_OK gives a stale answer... */
#define pfind(pid) p_find((pid), PFIND_UNLOCK)
#define pgfind(pgid) pg_find((pgid), PFIND_UNLOCK)

struct simplelock;
int	enterpgrp(struct proc *, pid_t, int);
void	fixjobc(struct proc *, struct pgrp *, int);
int	inferior(struct proc *, struct proc *);
int	leavepgrp(struct proc *);
void	sessdelete(struct session *);
void	yield(void);
struct lwp *chooselwp(void);
void	pgdelete(struct pgrp *);
void	procinit(void);
void	resetprocpriority(struct proc *);
void	suspendsched(void);
int	ltsleep(volatile const void *, int, const char *, int,
	    volatile struct simplelock *);
void	wakeup(volatile const void *);
void	wakeup_one(volatile const void *);
void	exit1(struct lwp *, int);
int	find_stopped_child(struct proc *, pid_t, int, struct proc **);
struct proc *proc_alloc(void);
void	proc0_init(void);
void	proc_free(struct proc *);
void	proc_free_mem(struct proc *);
void	exit_lwps(struct lwp *l);
int	fork1(struct lwp *, int, int, void *, size_t,
	    void (*)(void *), void *, register_t *, struct proc **);
void	rqinit(void);
int	pgid_in_session(struct proc *, pid_t);
#ifndef cpu_idle
void	cpu_idle(void);
#endif
void	cpu_exit(struct lwp *);
void	cpu_lwp_fork(struct lwp *, struct lwp *, void *, size_t,
	    void (*)(void *), void *);
#ifndef cpu_lwp_free
void	cpu_lwp_free(struct lwp *, int);
#endif

#ifdef __HAVE_SYSCALL_INTERN
void	syscall_intern(struct proc *);
#endif

void	child_return(void *);

int	proc_isunder(struct proc *, struct lwp *);
void	proc_stop(struct proc *, int);

void	proclist_lock_read(void);
void	proclist_unlock_read(void);
int	proclist_lock_write(void);
void	proclist_unlock_write(int);
void	p_sugid(struct proc *);

int	proc_vmspace_getref(struct proc *, struct vmspace **);

int	proclist_foreach_call(struct proclist *,
    int (*)(struct proc *, void *arg), void *);
static __inline struct proc *_proclist_skipmarker(struct proc *);

static __inline struct proc *
_proclist_skipmarker(struct proc *p0)
{
	struct proc *p = p0;

	while (p != NULL && p->p_flag & P_MARKER)
		p = LIST_NEXT(p, p_list);

	return p;
}
#define	PROCLIST_FOREACH(var, head)					\
	for ((var) = LIST_FIRST(head);					\
		((var) = _proclist_skipmarker(var)) != NULL;		\
		(var) = LIST_NEXT(var, p_list))

/* Compatibility with old, non-interlocked tsleep call */
#define	tsleep(chan, pri, wmesg, timo)					\
	ltsleep(chan, pri, wmesg, timo, NULL)

#if defined(MULTIPROCESSOR)
void	proc_trampoline_mp(void);	/* XXX */
#endif

#ifdef KSTACK_CHECK_MAGIC
void kstack_setup_magic(const struct lwp *);
void kstack_check_magic(const struct lwp *);
#endif

/*
 * kernel stack paramaters
 * XXX require sizeof(struct user)
 */
/* the lowest address of kernel stack */
#ifndef KSTACK_LOWEST_ADDR
#define	KSTACK_LOWEST_ADDR(l)	((caddr_t)ALIGN((l)->l_addr + 1))
#endif
/* size of kernel stack */
#ifndef KSTACK_SIZE
#define	KSTACK_SIZE	(USPACE - ALIGN(sizeof(struct user)))
#endif

#endif	/* _KERNEL */
#endif	/* !_SYS_PROC_H_ */
