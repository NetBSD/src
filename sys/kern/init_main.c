/*	$NetBSD: init_main.c,v 1.237 2004/05/06 22:20:30 pk Exp $	*/

/*
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
 *	@(#)init_main.c	8.16 (Berkeley) 5/14/95
 */

/*
 * Copyright (c) 1995 Christopher G. Demetriou.  All rights reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: init_main.c,v 1.237 2004/05/06 22:20:30 pk Exp $");

#include "fs_nfs.h"
#include "opt_nfsserver.h"
#include "opt_ipsec.h"
#include "opt_sysv.h"
#include "opt_maxuprc.h"
#include "opt_multiprocessor.h"
#include "opt_pipe.h"
#include "opt_syscall_debug.h"
#include "opt_systrace.h"
#include "opt_posix.h"
#include "opt_kcont.h"

#include "opencrypto.h"
#include "rnd.h"

#include <sys/param.h>
#include <sys/acct.h>
#include <sys/filedesc.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/kcont.h>
#include <sys/mount.h>
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
#include <sys/disk.h>
#include <sys/exec.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/reboot.h>
#include <sys/user.h>
#include <sys/sysctl.h>
#include <sys/event.h>
#include <sys/mbuf.h>
#ifdef FAST_IPSEC
#include <netipsec/ipsec.h>
#endif
#ifdef SYSVSHM
#include <sys/shm.h>
#endif
#ifdef SYSVSEM
#include <sys/sem.h>
#endif
#ifdef SYSVMSG
#include <sys/msg.h>
#endif
#ifdef P1003_1B_SEMAPHORE
#include <sys/ksem.h>
#endif
#ifdef SYSTRACE
#include <sys/systrace.h>
#endif
#include <sys/domain.h>
#include <sys/namei.h>
#if NOPENCRYPTO > 0
#include <opencrypto/cryptodev.h>	/* XXX really the framework */
#endif
#if NRND > 0
#include <sys/rnd.h>
#endif
#ifndef PIPE_SOCKETPAIR
#include <sys/pipe.h>
#endif
#ifdef LKM
#include <sys/lkm.h>
#endif

#include <sys/syscall.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>

#include <ufs/ufs/quota.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/syncfs/syncfs.h>

#include <machine/cpu.h>

#include <uvm/uvm.h>

#include <dev/cons.h>

#include <net/if.h>
#include <net/raw_cb.h>

/* Components of the first process -- never freed. */
struct	session session0;
struct	pgrp pgrp0;
struct	proc proc0;
struct	lwp lwp0;
struct	pcred cred0;
struct	filedesc0 filedesc0;
struct	cwdinfo cwdi0;
struct	plimit limit0;
struct	pstats pstat0;
struct	vmspace vmspace0;
struct	sigacts sigacts0;
#ifndef curlwp
struct	lwp *curlwp = &lwp0;
#endif
struct	proc *initproc;

int	nofile = NOFILE;
int	maxuprc = MAXUPRC;
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
	struct lwp *l;
	struct proc *p;
	struct pdevinit *pdev;
	int s, error;
	u_int i;
	rlim_t lim;
	extern struct pdevinit pdevinit[];
	extern void schedcpu(void *);
#if defined(NFSSERVER) || defined(NFS)
	extern void nfs_init(void);
#endif
#ifdef NVNODE_IMPLICIT
	int usevnodes;
#endif

	/*
	 * Initialize the current LWP pointer (curlwp) before
	 * any possible traps/probes to simplify trap processing.
	 */
	l = &lwp0;
	curlwp = l;
	l->l_cpu = curcpu();
	l->l_proc = &proc0;
	l->l_lid = 1;

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

	/* Initialise pools. */
	link_pool_init();

	/* Initialize callouts. */
	callout_startup();

	/* Initialize the buffer cache */
	bufinit();

	/*
	 * Initialize mbuf's.  Do this now because we might attempt to
	 * allocate mbufs or mbuf clusters during autoconfiguration.
	 */
	mbinit();

	/* Initialize sockets. */
	soinit();

#ifdef KCONT
	/* Initialize kcont. */
        kcont_init();
#endif

	/*
	 * The following things must be done before autoconfiguration.
	 */
	evcnt_init();		/* initialize event counters */
	disk_init();		/* initialize disk list */
	tty_init();		/* initialize tty list */
#if NRND > 0
	rnd_init();		/* initialize RNG */
#endif
#if NOPENCRYPTO > 0
	/* Initialize crypto subsystem before configuring crypto hardware. */
	(void)crypto_init();
#endif
	/* Initialize the sysctl subsystem. */
	sysctl_init();

	/* Initialize process and pgrp structures. */
	procinit();

#ifdef LKM
	/* Initialize the LKM system. */
	lkm_init();
#endif

	/*
	 * Create process 0 (the swapper).
	 */
	p = &proc0;
	proc0_insert(p, l, &pgrp0, &session0);

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
	cred0.p_refcnt = 1;
	p->p_cred = &cred0;
	p->p_ucred = crget();
	p->p_ucred->cr_ngroups = 1;	/* group 0 */

	/* Create the file descriptor table. */
	p->p_fd = &filedesc0.fd_fd;
	fdinit1(&filedesc0);

	/* Create the CWD info. */
	p->p_cwdi = &cwdi0;
	cwdi0.cwdi_cmask = cmask;
	cwdi0.cwdi_refcnt = 1;

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
#ifdef NVNODE_IMPLICIT
	/*
	 * If maximum number of vnodes in namei vnode cache is not explicitly
	 * defined in kernel config, adjust the number such as we use roughly
	 * 1.0% of memory for vnode cache (but not less than NVNODE vnodes).
	 */
	usevnodes = (ptoa((unsigned)physmem) / 100) / sizeof(struct vnode);
	if (usevnodes > desiredvnodes)
		desiredvnodes = usevnodes;
#endif
	vfsinit();

	/* Configure the system hardware.  This will enable interrupts. */
	configure();

	ubc_init();		/* must be after autoconfig */

	/* Lock the kernel on behalf of proc0. */
	KERNEL_PROC_LOCK(l);

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

#ifdef P1003_1B_SEMAPHORE
	/* Initialize posix semaphores */
	ksem_init();
#endif
	/* Attach pseudo-devices. */
	for (pdev = pdevinit; pdev->pdev_attach != NULL; pdev++)
		(*pdev->pdev_attach)(pdev->pdev_count);

#ifdef	FAST_IPSEC
	/* Attach network crypto subsystem */
	ipsec_attach();
#endif

	/*
	 * Initialize protocols.  Block reception of incoming packets
	 * until everything is ready.
	 */
	s = splnet();
	ifinit();
	domaininit();
	if_attachdomain();
	splx(s);

#ifdef GPROF
	/* Initialize kernel profiling. */
	kmstartup();
#endif

	/* Initialize system accouting. */
	acct_init();

#ifdef SYSTRACE
	systrace_init();
#endif
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
	if (fork1(l, 0, SIGCHLD, NULL, 0, start_init, NULL, NULL, &initproc))
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
	 * Finalize configuration now that all real devices have been
	 * found.  This needs to be done before the root device is
	 * selected, since finalization may create the root device.
	 */
	config_finalize();

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

	CIRCLEQ_FIRST(&mountlist)->mnt_flag |= MNT_ROOTFS;
	CIRCLEQ_FIRST(&mountlist)->mnt_op->vfs_refcount++;

	/*
	 * Get the vnode for '/'.  Set filedesc0.fd_fd.fd_cdir to
	 * reference it.
	 */
	if (VFS_ROOT(CIRCLEQ_FIRST(&mountlist), &rootvnode))
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
	LIST_FOREACH(p, &allproc, p_list) {
		p->p_stats->p_start = mono_time = boottime = time;
		LIST_FOREACH(l, &p->p_lwps, l_sibling) {
			if (l->l_cpu != NULL)
				l->l_cpu->ci_schedstate.spc_runtime = time;
		}
		p->p_rtime.tv_sec = p->p_rtime.tv_usec = 0;
	}
	splx(s);
	proclist_unlock_read();

	/* Create the pageout daemon kernel thread. */
	uvm_swap_init();
	if (kthread_create1(uvm_pageout, NULL, NULL, "pagedaemon"))
		panic("fork pagedaemon");

	/* Create the filesystem syncer kernel thread. */
	if (kthread_create1(sched_sync, NULL, NULL, "ioflush"))
		panic("fork syncer");

	/* Create the aiodone daemon kernel thread. */
	if (kthread_create1(uvm_aiodone_daemon, NULL, &uvm.aiodoned_proc,
	    "aiodoned"))
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
	struct lwp *l = arg;
	struct proc *p = l->l_proc;
	vaddr_t addr;
	struct sys_execve_args /* {
		syscallarg(const char *) path;
		syscallarg(char * const *) argp;
		syscallarg(char * const *) envp;
	} */ args;
	int options, i, error;
	register_t retval[2];
	char flags[4], *flagsp;
	const char *path, *slash;
	char *ucp, **uap, *arg0, *arg1 = NULL;
	char ipath[129];
	int ipx, len;

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
	addr = (vaddr_t)STACK_ALLOC(USRSTACK, PAGE_SIZE);
	if (uvm_map(&p->p_vmspace->vm_map, &addr, PAGE_SIZE,
                    NULL, UVM_UNKNOWN_OFFSET, 0,
                    UVM_MAPFLAG(UVM_PROT_ALL, UVM_PROT_ALL, UVM_INH_COPY,
		    UVM_ADV_NORMAL,
                    UVM_FLAG_FIXED|UVM_FLAG_OVERLAY|UVM_FLAG_COPYONW)) != 0)
		panic("init: couldn't allocate argument space");
	p->p_vmspace->vm_maxsaddr = (caddr_t)STACK_MAX(addr, PAGE_SIZE);

	ipx = 0;
	while (1) {
		if (boothowto & RB_ASKNAME) {
			printf("init path");
			if (initpaths[ipx])
				printf(" (default %s)", initpaths[ipx]);
			printf(": ");
			len = cngetsn(ipath, sizeof(ipath)-1);
			if (len == 0) {
				if (initpaths[ipx])
					path = initpaths[ipx++];
				else
					continue;
			} else {
				ipath[len] = '\0';
				path = ipath;
			}
		} else {
			if ((path = initpaths[ipx++]) == NULL)
				break;
		}

		ucp = (char *)USRSTACK;

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
			arg1 = STACK_ALLOC(ucp, i);
			ucp = STACK_MAX(arg1, i);
			(void)copyout((caddr_t)flags, arg1, i);
		}

		/*
		 * Move out the file name (also arg 0).
		 */
		i = strlen(path) + 1;
#ifdef DEBUG
		printf("init: copying out path `%s' %d\n", path, i);
#else
		if (boothowto & RB_ASKNAME || path != initpaths[0])
			printf("init: trying %s\n", path);
#endif
		arg0 = STACK_ALLOC(ucp, i);
		ucp = STACK_MAX(arg0, i);
		(void)copyout((caddr_t)path, arg0, i);

		/*
		 * Move out the arg pointers.
		 */
		ucp = (caddr_t)STACK_ALIGN(ucp, ALIGNBYTES);
		uap = (char **)STACK_ALLOC(ucp, sizeof(char *) * 3);
		SCARG(&args, path) = arg0;
		SCARG(&args, argp) = uap;
		SCARG(&args, envp) = NULL;
		slash = strrchr(path, '/');
		if (slash)
			(void)suword((caddr_t)uap++,
			    (long)arg0 + (slash + 1 - path));
		else
			(void)suword((caddr_t)uap++, (long)arg0);
		if (options != 0)
			(void)suword((caddr_t)uap++, (long)arg1);
		(void)suword((caddr_t)uap++, 0);	/* terminator */

		/*
		 * Now try to exec the program.  If can't for any reason
		 * other than it doesn't exist, complain.
		 */
		error = sys_execve(LIST_FIRST(&p->p_lwps), &args, retval);
		if (error == 0 || error == EJUSTRETURN) {
			KERNEL_PROC_UNLOCK(l);
			return;
		}
		printf("exec %s: error %d\n", path, error);
	}
	printf("init: not found\n");
	panic("no init");
}
