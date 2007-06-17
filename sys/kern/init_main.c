/*	$NetBSD: init_main.c,v 1.299.2.11 2007/06/17 21:31:18 ad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: init_main.c,v 1.299.2.11 2007/06/17 21:31:18 ad Exp $");

#include "opt_ipsec.h"
#include "opt_kcont.h"
#include "opt_multiprocessor.h"
#include "opt_ntp.h"
#include "opt_pipe.h"
#include "opt_posix.h"
#include "opt_syscall_debug.h"
#include "opt_sysv.h"
#include "opt_systrace.h"
#include "opt_fileassoc.h"
#include "opt_ktrace.h"
#include "opt_pax.h"

#include "rnd.h"
#include "veriexec.h"

#include <sys/param.h>
#include <sys/acct.h>
#include <sys/filedesc.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/callout.h>
#include <sys/cpu.h>
#include <sys/kernel.h>
#include <sys/kcont.h>
#include <sys/kmem.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/resourcevar.h>
#include <sys/signalvar.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/fstrans.h>
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
#include <sys/event.h>
#include <sys/mbuf.h>
#include <sys/sched.h>
#include <sys/sleepq.h>
#include <sys/iostat.h>
#include <sys/uuid.h>
#include <sys/extent.h>
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
#ifdef SYSTRACE
#include <sys/systrace.h>
#endif
#ifdef P1003_1B_SEMAPHORE
#include <sys/ksem.h>
#endif
#include <sys/domain.h>
#include <sys/namei.h>
#if NRND > 0
#include <sys/rnd.h>
#endif
#include <sys/pipe.h>
#ifdef LKM
#include <sys/lkm.h>
#endif
#if NVERIEXEC > 0
#include <sys/verified_exec.h>
#endif /* NVERIEXEC > 0 */
#ifdef KTRACE
#include <sys/ktrace.h>
#endif
#include <sys/debug.h>
#include <sys/kauth.h>
#include <net80211/ieee80211_netbsd.h>

#include <sys/syscall.h>
#include <sys/syscallargs.h>

#if defined(PAX_MPROTECT) || defined(PAX_SEGVGUARD)
#include <sys/pax.h>
#endif /* PAX_MPROTECT || PAX_SEGVGUARD */
#include <ufs/ufs/quota.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/syncfs/syncfs.h>

#include <machine/cpu.h>

#include <uvm/uvm.h>

#include <dev/cons.h>

#include <net/if.h>
#include <net/raw_cb.h>

#include <secmodel/secmodel.h>

extern struct proc proc0;
extern struct lwp lwp0;
extern struct cwdinfo cwdi0;

#ifndef curlwp
struct	lwp *curlwp = &lwp0;
#endif
struct	proc *initproc;

struct	vnode *rootvp, *swapdev_vp;
int	boothowto;
int	cold = 1;			/* still working on startup */
struct timeval boottime;	        /* time at system startup - will only follow settime deltas */
time_t	rootfstime;			/* recorded root fs time, if known */
int	ncpu = 0;			/* number of CPUs configured, assume 1 */

volatile int start_init_exec;		/* semaphore for start_init() */

static void check_console(struct lwp *l);
static void start_init(void *);
void main(void);

#if defined(__SSP__) || defined(__SSP_ALL__)
long __stack_chk_guard[8] = {0, 0, 0, 0, 0, 0, 0, 0};
void __stack_chk_fail(void);

void
__stack_chk_fail(void)
{
	panic("stack overflow detected; terminated");
}
#endif

void __secmodel_none(void);
__weak_alias(secmodel_start,__secmodel_none);
void
__secmodel_none(void)
{
	return;
}

/*
 * System startup; initialize the world, create process 0, mount root
 * filesystem, and fork to create init and pagedaemon.  Most of the
 * hard work is done in the lower-level initialization routines including
 * startup(), which does memory initialization and autoconfiguration.
 */
void
main(void)
{
#ifdef __HAVE_TIMECOUNTER
	struct timeval time;
#endif
	struct lwp *l;
	struct proc *p;
	struct pdevinit *pdev;
	int s, error;
	extern struct pdevinit pdevinit[];
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

	KERNEL_LOCK_INIT();

	uvm_init();

#ifdef DEBUG
	debug_init();
#endif

	kmem_init();

	/* Initialize the extent manager. */
	extent_init();

	/* Do machine-dependent initialization. */
	cpu_startup();

	/* Initialize callouts. */
	callout_startup();

	/*
	 * Initialize the kernel authorization subsystem and start the
	 * default security model, if any. We need to do this early
	 * enough so that subsystems relying on any of the aforementioned
	 * can work properly. Since the security model may dictate the
	 * credential inheritance policy, it is needed at least before
	 * any process is created, specifically proc0.
	 */
	kauth_init();
	secmodel_start();

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
#if NRND > 0
	rnd_init();		/* initialize RNG */
#endif

	/* Initialize process and pgrp structures. */
	procinit();
	lwpinit();

	/* Initialize signal-related data structures. */
	signal_init();

	/* Create process 0 (the swapper). */
	proc0_init();

	/* Initialize the UID hash table. */
	uid_init();

	/* Charge root for one process. */
	(void)chgproccnt(0, 1);

	/* Initialize the run queues, turnstiles and sleep queues. */
	sched_rqinit();
	turnstile_init();
	sleeptab_init(&sleeptab);

	/* MI initialization of the boot cpu */
	error = mi_cpu_attach(curcpu());
	KASSERT(error == 0);

	/* Initialize the sysctl subsystem. */
	sysctl_init();

	/* Initialize I/O statistics. */
	iostat_init();

	/* Initialize the file systems. */
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

	/* Initialize fstrans. */
	fstrans_init();

	/* Initialize the file descriptor system. */
	filedesc_init();

	/* Initialize the select()/poll() system calls. */
	selsysinit();

	/* Initialize asynchronous I/O. */
	aio_sysinit();

#ifdef __HAVE_TIMECOUNTER
	inittimecounter();
	ntp_init();
#endif /* __HAVE_TIMECOUNTER */

	/* Initialize the device switch tables. */
	devsw_init();

	/* Configure the system hardware.  This will enable interrupts. */
	configure();

	/* Initialize the buffer cache, part 2 */
	bufinit2();

#if defined(__SSP__) || defined(__SSP_ALL__)
	{
#ifdef DIAGNOSTIC
		printf("Initializing SSP:");
#endif
		/*
		 * We initialize ssp here carefully:
		 *	1. after we got some entropy
		 *	2. without calling a function
		 */
		size_t i;
		long guard[__arraycount(__stack_chk_guard)];

		arc4randbytes(guard, sizeof(guard));
		for (i = 0; i < __arraycount(guard); i++)
			__stack_chk_guard[i] = guard[i];
#ifdef DIAGNOSTIC
		for (i = 0; i < __arraycount(guard); i++)
			printf("%lx ", guard[i]);
		printf("\n");
#endif
	}
#endif
	ubc_init();		/* must be after autoconfig */

	/* Lock the kernel on behalf of proc0. */
	KERNEL_LOCK(1, l);

#ifdef SYSTRACE
	systrace_init();
#endif

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

#if NVERIEXEC > 0
	/*
	 * Initialise the Veriexec subsystem.
	 */
	veriexec_init();
#endif /* NVERIEXEC > 0 */

#if defined(PAX_MPROTECT) || defined(PAX_SEGVGUARD)
	pax_init();
#endif /* PAX_MPROTECT || PAX_SEGVGUARD */

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

#ifndef PIPE_SOCKETPAIR
	/* Initialize pipes. */
	pipe_init();
#endif

	/* Setup the scheduler */
	sched_setup();

#ifdef KTRACE
	/* Initialize ktrace. */
	ktrinit();
#endif

	/* Initialize the UUID system calls. */
	uuid_init();

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
	 * Now that device driver threads have been created, wait for
	 * them to finish any deferred autoconfiguration.  Note we don't
	 * need to lock this semaphore, since we haven't booted any
	 * secondary processors, yet.
	 */
	while (config_pending)
		(void) tsleep(&config_pending, PWAIT, "cfpend", 0);

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

	/*
	 * Initialise the time-of-day clock, passing the time recorded
	 * in the root filesystem (if any) for use by systems that
	 * don't have a non-volatile time-of-day device.
	 */
	inittodr(rootfstime);

	CIRCLEQ_FIRST(&mountlist)->mnt_flag |= MNT_ROOTFS;
	CIRCLEQ_FIRST(&mountlist)->mnt_op->vfs_refcount++;

	/*
	 * Get the vnode for '/'.  Set filedesc0.fd_fd.fd_cdir to
	 * reference it.
	 */
	error = VFS_ROOT(CIRCLEQ_FIRST(&mountlist), &rootvnode);
	if (error)
		panic("cannot find root vnode, error=%d", error);
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
	 * from the file system.  Reset l->l_rtime as it may have been
	 * munched in mi_switch() after the time got set.
	 */
#ifdef __HAVE_TIMECOUNTER
	getmicrotime(&time);
#else
	mono_time = time;
#endif
	boottime = time;
	mutex_enter(&proclist_lock);
	LIST_FOREACH(p, &allproc, p_list) {
		KASSERT((p->p_flag & PK_MARKER) == 0);
		mutex_enter(&p->p_smutex);
		p->p_stats->p_start = time;
		LIST_FOREACH(l, &p->p_lwps, l_sibling) {
			lwp_lock(l);
			l->l_cpu->ci_schedstate.spc_runtime = time;
			l->l_rtime.tv_sec = l->l_rtime.tv_usec = 0;
			lwp_unlock(l);
		}
		mutex_exit(&p->p_smutex);
	}
	mutex_exit(&proclist_lock);

	/* Create the pageout daemon kernel thread. */
	uvm_swap_init();
	if (kthread_create(PVM, KTHREAD_MPSAFE, NULL, uvm_pageout,
	    NULL, NULL, "pgdaemon"))
		panic("fork pagedaemon");

	/* Create the filesystem syncer kernel thread. */
	if (kthread_create(PINOD, 0, NULL, sched_sync, NULL, NULL, "ioflush"))
		panic("fork syncer");

	/* Create the aiodone daemon kernel thread. */
	if (workqueue_create(&uvm.aiodone_queue, "aiodoned",
	    uvm_aiodone_worker, NULL, PVM, IPL_BIO, 0))
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
	wakeup(&start_init_exec);

	/* The scheduler is an infinite loop. */
	uvm_scheduler();
	/* NOTREACHED */
}

void
setrootfstime(time_t t)
{
	rootfstime = t;
}

static void
check_console(struct lwp *l)
{
	struct nameidata nd;
	int error;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE, "/dev/console", l);
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
		(void) tsleep(&start_init_exec, PWAIT, "initexec", 0);

	/*
	 * This is not the right way to do this.  We really should
	 * hand-craft a descriptor onto /dev/console to hand to init,
	 * but that's a _lot_ more work, and the benefit from this easy
	 * hack makes up for the "good is the enemy of the best" effect.
	 */
	check_console(l);

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
	p->p_vmspace->vm_maxsaddr = (void *)STACK_MAX(addr, PAGE_SIZE);

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
			(void)copyout((void *)flags, arg1, i);
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
		(void)copyout(path, arg0, i);

		/*
		 * Move out the arg pointers.
		 */
		ucp = (void *)STACK_ALIGN(ucp, ALIGNBYTES);
		uap = (char **)STACK_ALLOC(ucp, sizeof(char *) * 3);
		SCARG(&args, path) = arg0;
		SCARG(&args, argp) = uap;
		SCARG(&args, envp) = NULL;
		slash = strrchr(path, '/');
		if (slash)
			(void)suword((void *)uap++,
			    (long)arg0 + (slash + 1 - path));
		else
			(void)suword((void *)uap++, (long)arg0);
		if (options != 0)
			(void)suword((void *)uap++, (long)arg1);
		(void)suword((void *)uap++, 0);	/* terminator */

		/*
		 * Now try to exec the program.  If can't for any reason
		 * other than it doesn't exist, complain.
		 */
		error = sys_execve(l, &args, retval);
		if (error == 0 || error == EJUSTRETURN) {
			KERNEL_UNLOCK_LAST(l);
			return;
		}
		printf("exec %s: error %d\n", path, error);
	}
	printf("init: not found\n");
	panic("no init");
}
