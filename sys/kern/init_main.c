/*	$NetBSD: init_main.c,v 1.157.2.4 2000/12/13 15:50:18 bouyer Exp $	*/

/*
 * Copyright (c) 1995 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1982, 1986, 1989, 1991, 1992, 1993
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
 *	@(#)init_main.c	8.16 (Berkeley) 5/14/95
 */

#include "fs_nfs.h"
#include "opt_nfsserver.h"
#include "opt_sysv.h"
#include "opt_maxuprc.h"
#include "opt_multiprocessor.h"
#include "opt_syscall_debug.h"

#include "rnd.h"

#include <sys/param.h>
#include <sys/acct.h>
#include <sys/filedesc.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/resourcevar.h>
#include <sys/signalvar.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/disklabel.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/exec.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/reboot.h>
#include <sys/user.h>
#include <sys/sysctl.h>
#ifdef SYSVSHM
#include <sys/shm.h>
#endif
#ifdef SYSVSEM
#include <sys/sem.h>
#endif
#ifdef SYSVMSG
#include <sys/msg.h>
#endif
#include <sys/domain.h>
#include <sys/mbuf.h>
#include <sys/namei.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <sys/syscall.h>
#include <sys/syscallargs.h>

#include <ufs/ufs/quota.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/syncfs/syncfs.h>

#include <machine/cpu.h>

#include <uvm/uvm.h>

#include <net/if.h>
#include <net/raw_cb.h>

const char copyright[] = "\
Copyright (c) 1996, 1997, 1998, 1999, 2000
    The NetBSD Foundation, Inc.  All rights reserved.
Copyright (c) 1982, 1986, 1989, 1991, 1993
    The Regents of the University of California.  All rights reserved.

";

/* Components of the first process -- never freed. */
struct	session session0;
struct	pgrp pgrp0;
struct	proc proc0;
struct	pcred cred0;
struct	filedesc0 filedesc0;
struct	cwdinfo cwdi0;
struct	plimit limit0;
struct	vmspace vmspace0;
struct	sigacts sigacts0;
#ifndef curproc
struct	proc *curproc = &proc0;
#endif
struct	proc *initproc;

int	cmask = CMASK;
extern	struct user *proc0paddr;

struct	vnode *rootvp, *swapdev_vp;
int	boothowto;
int	cold = 1;			/* still working on startup */
struct	timeval boottime;

__volatile int start_init_exec;		/* semaphore for start_init() */

static void check_console(struct proc *p);
static void start_init(void *);
void main(void);

extern const struct emul emul_netbsd;	/* defined in kern_exec.c */

/*
 * System startup; initialize the world, create process 0, mount root
 * filesystem, and fork to create init and pagedaemon.  Most of the
 * hard work is done in the lower-level initialization routines including
 * startup(), which does memory initialization and autoconfiguration.
 */
void
main(void)
{
	struct proc *p;
	struct pdevinit *pdev;
	int i, s, error;
	rlim_t lim;
	extern struct pdevinit pdevinit[];
	extern void schedcpu(void *);
	extern void disk_init(void);
#if defined(NFSSERVER) || defined(NFS)
	extern void nfs_init(void);
#endif
#ifdef NVNODE_IMPLICIT
	int usevnodes;
#endif

	/*
	 * Initialize the current process pointer (curproc) before
	 * any possible traps/probes to simplify trap processing.
	 */
	p = &proc0;
	curproc = p;
	p->p_cpu = curcpu();
	/*
	 * Attempt to find console and initialize
	 * in case of early panic or other messages.
	 */
	consinit();
	printf("%s", copyright);

	KERNEL_LOCK_INIT();

	uvm_init();

	/* Do machine-dependent initialization. */
	cpu_startup();

	/* Initialize callouts. */
	callout_startup();

	/*
	 * Initialize mbuf's.  Do this now because we might attempt to
	 * allocate mbufs or mbuf clusters during autoconfiguration.
	 */
	mbinit();

	/* Initialize sockets. */
	soinit();

	/*
	 * The following 3 things must be done before autoconfiguration.
	 */
	disk_init();		/* initialize disk list */
	tty_init();		/* initialize tty list */
#if NRND > 0
	rnd_init();		/* initialize RNG */
#endif

	/* Initialize the sysctl subsystem. */
	sysctl_init();

	/*
	 * Initialize process and pgrp structures.
	 */
	procinit();

	/*
	 * Create process 0 (the swapper).
	 */
	s = proclist_lock_write();
	LIST_INSERT_HEAD(&allproc, p, p_list);
	LIST_INSERT_HEAD(PIDHASH(p->p_pid), p, p_hash);
	proclist_unlock_write(s);

	p->p_pgrp = &pgrp0;
	LIST_INSERT_HEAD(PGRPHASH(0), &pgrp0, pg_hash);
	LIST_INIT(&pgrp0.pg_members);
	LIST_INSERT_HEAD(&pgrp0.pg_members, p, p_pglist);

	pgrp0.pg_session = &session0;
	session0.s_count = 1;
	session0.s_sid = p->p_pid;
	session0.s_leader = p;

	/*
	 * Set P_NOCLDWAIT so that kernel threads are reparented to
	 * init(8) when they exit.  init(8) can easily wait them out
	 * for us.
	 */
	p->p_flag = P_INMEM | P_SYSTEM | P_NOCLDWAIT;
	p->p_stat = SONPROC;
	p->p_nice = NZERO;
	p->p_emul = &emul_netbsd;
#ifdef __HAVE_SYSCALL_INTERN
	(*p->p_emul->e_syscall_intern)(p);
#endif
	strncpy(p->p_comm, "swapper", MAXCOMLEN);

	callout_init(&p->p_realit_ch);
	callout_init(&p->p_tsleep_ch);

	/* Create credentials. */
	cred0.p_refcnt = 1;
	p->p_cred = &cred0;
	p->p_ucred = crget();
	p->p_ucred->cr_ngroups = 1;	/* group 0 */

	/* Create the file descriptor table. */
	finit();
	p->p_fd = &filedesc0.fd_fd;
	fdinit1(&filedesc0);

	/* Create the CWD info. */
	p->p_cwdi = &cwdi0;
	cwdi0.cwdi_cmask = cmask;
	cwdi0.cwdi_refcnt = 1;

	/* Create the limits structures. */
	p->p_limit = &limit0;
	for (i = 0; i < sizeof(p->p_rlimit)/sizeof(p->p_rlimit[0]); i++)
		limit0.pl_rlimit[i].rlim_cur =
		    limit0.pl_rlimit[i].rlim_max = RLIM_INFINITY;

	limit0.pl_rlimit[RLIMIT_NOFILE].rlim_max = maxfiles;
	limit0.pl_rlimit[RLIMIT_NOFILE].rlim_cur = 
	    maxfiles < NOFILE ? maxfiles : NOFILE;

	limit0.pl_rlimit[RLIMIT_NPROC].rlim_max = maxproc;
	limit0.pl_rlimit[RLIMIT_NPROC].rlim_cur =
	    maxproc < MAXUPRC ? maxproc : MAXUPRC;

	lim = ptoa(uvmexp.free);
	limit0.pl_rlimit[RLIMIT_RSS].rlim_max = lim;
	limit0.pl_rlimit[RLIMIT_MEMLOCK].rlim_max = lim;
	limit0.pl_rlimit[RLIMIT_MEMLOCK].rlim_cur = lim / 3;
	limit0.pl_corename = defcorename;
	limit0.p_refcnt = 1;

	/*
	 * Initialize proc0's vmspace, which uses the kernel pmap.
	 * All kernel processes (which never have user space mappings)
	 * share proc0's vmspace, and thus, the kernel pmap.
	 */
	uvmspace_init(&vmspace0, pmap_kernel(), round_page(VM_MIN_ADDRESS),
	    trunc_page(VM_MAX_ADDRESS), TRUE);
	p->p_vmspace = &vmspace0;

	p->p_addr = proc0paddr;				/* XXX */

	/*
	 * We continue to place resource usage info in the
	 * user struct so they're pageable.
	 */
	p->p_stats = &p->p_addr->u_stats;

	/*
	 * Charge root for one process.
	 */
	(void)chgproccnt(0, 1);

	rqinit();

	/* Configure virtual memory system, set vm rlimits. */
	uvm_init_limits(p);

	/* Initialize the file systems. */
#if defined(NFSSERVER) || defined(NFS)
	nfs_init();			/* initialize server/shared data */
#endif
	vfsinit();

	/* Configure the system hardware.  This will enable interrupts. */
	configure();

	ubc_init();		/* must be after autoconfig */

	/* Lock the kernel on behalf of proc0. */
	KERNEL_PROC_LOCK(p);

#ifdef SYSVSHM
	/* Initialize System V style shared memory. */
	shminit();
#endif

#ifdef SYSVSEM
	/* Initialize System V style semaphores. */
	seminit();
#endif

#ifdef SYSVMSG
	/* Initialize System V style message queues. */
	msginit();
#endif

	/* Attach pseudo-devices. */
	for (pdev = pdevinit; pdev->pdev_attach != NULL; pdev++)
		(*pdev->pdev_attach)(pdev->pdev_count);

	/*
	 * Initialize protocols.  Block reception of incoming packets
	 * until everything is ready.
	 */
	s = splimp();
	ifinit();
	domaininit();
	splx(s);

#ifdef GPROF
	/* Initialize kernel profiling. */
	kmstartup();
#endif

	/* Initialize system accouting. */
	acct_init();

	/*
	 * Initialize signal-related data structures, and signal state
	 * for proc0.
	 */
	signal_init();
	p->p_sigacts = &sigacts0;
	siginit(p);

	/* Kick off timeout driven events by calling first time. */
	schedcpu(NULL);

	/*
	 * Create process 1 (init(8)).  We do this now, as Unix has
	 * historically had init be process 1, and changing this would
	 * probably upset a lot of people.
	 *
	 * Note that process 1 won't immediately exec init(8), but will
	 * wait for us to inform it that the root file system has been
	 * mounted.
	 */
	if (fork1(p, 0, SIGCHLD, NULL, 0, start_init, NULL, NULL, &initproc))
		panic("fork init");

	/*
	 * Create any kernel threads who's creation was deferred because
	 * initproc had not yet been created.
	 */
	kthread_run_deferred_queue();

	/*
	 * Now that device driver threads have been created, wait for
	 * them to finish any deferred autoconfiguration.  Note we don't
	 * need to lock this semaphore, since we haven't booted any
	 * secondary processors, yet.
	 */
	while (config_pending)
		(void) tsleep((void *)&config_pending, PWAIT, "cfpend", 0);

	/*
	 * Now that autoconfiguration has completed, we can determine
	 * the root and dump devices.
	 */
	cpu_rootconf();
	cpu_dumpconf();

	/* Mount the root file system. */
	do {
		domountroothook();
		if ((error = vfs_mountroot())) {
			printf("cannot mount root, error = %d\n", error);
			boothowto |= RB_ASKNAME;
			setroot(root_device,
			    (rootdev != NODEV) ? DISKPART(rootdev) : 0);
		}
	} while (error != 0);
	mountroothook_destroy();

	mountlist.cqh_first->mnt_flag |= MNT_ROOTFS;
	mountlist.cqh_first->mnt_op->vfs_refcount++;

	/*
	 * Get the vnode for '/'.  Set filedesc0.fd_fd.fd_cdir to
	 * reference it.
	 */
	if (VFS_ROOT(mountlist.cqh_first, &rootvnode))
		panic("cannot find root vnode");
	cwdi0.cwdi_cdir = rootvnode;
	VREF(cwdi0.cwdi_cdir);
	VOP_UNLOCK(rootvnode, 0);
	cwdi0.cwdi_rdir = NULL;

	/*
	 * Now that root is mounted, we can fixup initproc's CWD
	 * info.  All other processes are kthreads, which merely
	 * share proc0's CWD info.
	 */
	initproc->p_cwdi->cwdi_cdir = rootvnode;
	VREF(initproc->p_cwdi->cwdi_cdir);
	initproc->p_cwdi->cwdi_rdir = NULL;

	/*
	 * Now can look at time, having had a chance to verify the time
	 * from the file system.  Reset p->p_rtime as it may have been
	 * munched in mi_switch() after the time got set.
	 */
	proclist_lock_read();
	s = splsched();
	for (p = LIST_FIRST(&allproc); p != NULL;
	     p = LIST_NEXT(p, p_list)) {
		p->p_stats->p_start = mono_time = boottime = time;
		if (p->p_cpu != NULL)
			p->p_cpu->ci_schedstate.spc_runtime = time;
		p->p_rtime.tv_sec = p->p_rtime.tv_usec = 0;
	}
	splx(s);
	proclist_unlock_read();

	/* Create the pageout daemon kernel thread. */
	uvm_swap_init();
	if (kthread_create1(uvm_pageout, NULL, NULL, "pagedaemon"))
		panic("fork pagedaemon");

	/* Create the process reaper kernel thread. */
	if (kthread_create1(reaper, NULL, NULL, "reaper"))
		panic("fork reaper");

	/* Create the filesystem syncer kernel thread. */
	if (kthread_create1(sched_sync, NULL, NULL, "ioflush"))
		panic("fork syncer");

	/* Create the aiodone daemon kernel thread. */
	if (kthread_create1(uvm_aiodone_daemon, NULL, NULL, "aiodoned"))
		panic("fork aiodoned");

#if defined(MULTIPROCESSOR)
	/* Boot the secondary processors. */
	cpu_boot_secondary_processors();
#endif

	/* Initialize exec structures */
	exec_init(1);

	/*
	 * Okay, now we can let init(8) exec!  It's off to userland!
	 */
	start_init_exec = 1;
	wakeup((void *)&start_init_exec);

#ifdef NVNODE_IMPLICIT
	/*
	 * If maximum number of vnodes in namei vnode cache is not explicitly
	 * defined in kernel config, adjust the number such as we use roughly
	 * 0.5% of memory for vnode cache (but not less than NVNODE vnodes).
	 */
	usevnodes = (ptoa(physmem) / 200) / sizeof(struct vnode);
	if (usevnodes > desiredvnodes) 
		desiredvnodes = usevnodes;
#endif

	/* The scheduler is an infinite loop. */
	uvm_scheduler();
	/* NOTREACHED */
}

static void
check_console(struct proc *p)
{
	struct nameidata nd;
	int error;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE, "/dev/console", p);
	error = namei(&nd);
	if (error == 0)
		vrele(nd.ni_vp);
	else if (error == ENOENT)
		printf("warning: no /dev/console\n");
	else
		printf("warning: lookup /dev/console: error %d\n", error);
}

/*
 * List of paths to try when searching for "init".
 */
static const char *initpaths[] = {
	"/sbin/init",
	"/sbin/oinit",
	"/sbin/init.bak",
	NULL,
};

/*
 * Start the initial user process; try exec'ing each pathname in "initpaths".
 * The program is invoked with one argument containing the boot flags.
 */
static void
start_init(void *arg)
{
	struct proc *p = arg;
	vaddr_t addr;
	struct sys_execve_args /* {
		syscallarg(const char *) path;
		syscallarg(char * const *) argp;
		syscallarg(char * const *) envp;
	} */ args;
	int options, i, error;
	register_t retval[2];
	char flags[4], *flagsp;
	const char **pathp, *path, *slash;
	char *ucp, **uap, *arg0, *arg1 = NULL;

	/*
	 * Now in process 1.
	 */
	strncpy(p->p_comm, "init", MAXCOMLEN);

	/*
	 * Wait for main() to tell us that it's safe to exec.
	 */
	while (start_init_exec == 0)
		(void) tsleep((void *)&start_init_exec, PWAIT, "initexec", 0);

	/*
	 * This is not the right way to do this.  We really should
	 * hand-craft a descriptor onto /dev/console to hand to init,
	 * but that's a _lot_ more work, and the benefit from this easy
	 * hack makes up for the "good is the enemy of the best" effect.
	 */
	check_console(p);

	/*
	 * Need just enough stack to hold the faked-up "execve()" arguments.
	 */
	addr = USRSTACK - PAGE_SIZE;
	if (uvm_map(&p->p_vmspace->vm_map, &addr, PAGE_SIZE, 
                    NULL, UVM_UNKNOWN_OFFSET, 0,
                    UVM_MAPFLAG(UVM_PROT_ALL, UVM_PROT_ALL, UVM_INH_COPY,
		    UVM_ADV_NORMAL,
                    UVM_FLAG_FIXED|UVM_FLAG_OVERLAY|UVM_FLAG_COPYONW))
		!= KERN_SUCCESS)
		panic("init: couldn't allocate argument space");
	p->p_vmspace->vm_maxsaddr = (caddr_t)addr;

	for (pathp = &initpaths[0]; (path = *pathp) != NULL; pathp++) {
		ucp = (char *)(addr + PAGE_SIZE);

		/*
		 * Construct the boot flag argument.
		 */
		flagsp = flags;
		*flagsp++ = '-';
		options = 0;

		if (boothowto & RB_SINGLE) {
			*flagsp++ = 's';
			options = 1;
		}
#ifdef notyet
		if (boothowto & RB_FASTBOOT) {
			*flagsp++ = 'f';
			options = 1;
		}
#endif

		/*
		 * Move out the flags (arg 1), if necessary.
		 */
		if (options != 0) {
			*flagsp++ = '\0';
			i = flagsp - flags;
#ifdef DEBUG
			printf("init: copying out flags `%s' %d\n", flags, i);
#endif
			(void)copyout((caddr_t)flags, (caddr_t)(ucp -= i), i);
			arg1 = ucp;
		}

		/*
		 * Move out the file name (also arg 0).
		 */
		i = strlen(path) + 1;
#ifdef DEBUG
		printf("init: copying out path `%s' %d\n", path, i);
#endif
		(void)copyout((caddr_t)path, (caddr_t)(ucp -= i), i);
		arg0 = ucp;

		/*
		 * Move out the arg pointers.
		 */
		uap = (char **)((long)ucp & ~ALIGNBYTES);
		(void)suword((caddr_t)--uap, 0);	/* terminator */
		if (options != 0)
			(void)suword((caddr_t)--uap, (long)arg1);
		slash = strrchr(path, '/');
		if (slash)
			(void)suword((caddr_t)--uap,
			    (long)arg0 + (slash + 1 - path));
		else
			(void)suword((caddr_t)--uap, (long)arg0);

		/*
		 * Point at the arguments.
		 */
		SCARG(&args, path) = arg0;
		SCARG(&args, argp) = uap;
		SCARG(&args, envp) = NULL;

		/*
		 * Now try to exec the program.  If can't for any reason
		 * other than it doesn't exist, complain.
		 */
		error = sys_execve(p, &args, retval);
		if (error == 0 || error == EJUSTRETURN) {
			KERNEL_PROC_UNLOCK(p);
			return;
		}
		if (error != ENOENT)
			printf("exec %s: error %d\n", path, error);
	}
	printf("init: not found\n");
	panic("no init");
}
