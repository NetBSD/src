/*	$NetBSD: init_sysctl.c,v 1.47 2005/07/16 22:47:18 christos Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Brown.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: init_sysctl.c,v 1.47 2005/07/16 22:47:18 christos Exp $");

#include "opt_sysv.h"
#include "opt_multiprocessor.h"
#include "opt_posix.h"
#include "opt_verified_exec.h"
#include "pty.h"
#include "rnd.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/errno.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/unistd.h>
#include <sys/disklabel.h>
#include <sys/rnd.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/msgbuf.h>
#include <dev/cons.h>
#include <sys/socketvar.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/tty.h>
#include <sys/malloc.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/exec.h>
#include <sys/conf.h>
#include <sys/device.h>
#define	VERIEXEC_NEED_NODE
#include <sys/verified_exec.h>

#if defined(SYSVMSG) || defined(SYSVSEM) || defined(SYSVSHM)
#include <sys/ipc.h>
#endif
#ifdef SYSVMSG
#include <sys/msg.h>
#endif
#ifdef SYSVSEM
#include <sys/sem.h>
#endif
#ifdef SYSVSHM
#include <sys/shm.h>
#endif

#include <machine/cpu.h>

/*
 * try over estimating by 5 procs/lwps
 */
#define KERN_PROCSLOP	(5 * sizeof(struct kinfo_proc))
#define KERN_LWPSLOP	(5 * sizeof(struct kinfo_lwp))

#ifndef MULTIPROCESSOR
#define	sysctl_ncpus()	(1)
#else /* MULTIPROCESSOR */
#ifndef CPU_INFO_FOREACH
#define CPU_INFO_ITERATOR int
#define CPU_INFO_FOREACH(cii, ci) cii = 0, ci = curcpu(); ci != NULL; ci = NULL
#endif
static int
sysctl_ncpus(void)
{
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;

	int ncpus = 0;
	for (CPU_INFO_FOREACH(cii, ci))
		ncpus++;
	return (ncpus);
}
#endif /* MULTIPROCESSOR */

static int sysctl_kern_maxvnodes(SYSCTLFN_PROTO);
static int sysctl_kern_rtc_offset(SYSCTLFN_PROTO);
static int sysctl_kern_maxproc(SYSCTLFN_PROTO);
static int sysctl_kern_securelevel(SYSCTLFN_PROTO);
static int sysctl_kern_hostid(SYSCTLFN_PROTO);
static int sysctl_setlen(SYSCTLFN_PROTO);
static int sysctl_kern_clockrate(SYSCTLFN_PROTO);
static int sysctl_kern_file(SYSCTLFN_PROTO);
static int sysctl_kern_autonice(SYSCTLFN_PROTO);
static int sysctl_msgbuf(SYSCTLFN_PROTO);
static int sysctl_kern_defcorename(SYSCTLFN_PROTO);
static int sysctl_kern_cptime(SYSCTLFN_PROTO);
#if defined(SYSVMSG) || defined(SYSVSEM) || defined(SYSVSHM)
static int sysctl_kern_sysvipc(SYSCTLFN_PROTO);
#endif /* defined(SYSVMSG) || defined(SYSVSEM) || defined(SYSVSHM) */
#if NPTY > 0
static int sysctl_kern_maxptys(SYSCTLFN_PROTO);
#endif /* NPTY > 0 */
static int sysctl_kern_sbmax(SYSCTLFN_PROTO);
static int sysctl_kern_urnd(SYSCTLFN_PROTO);
static int sysctl_kern_lwp(SYSCTLFN_PROTO);
static int sysctl_kern_forkfsleep(SYSCTLFN_PROTO);
static int sysctl_kern_root_partition(SYSCTLFN_PROTO);
static int sysctl_kern_drivers(SYSCTLFN_PROTO);
static int sysctl_kern_file2(SYSCTLFN_PROTO);
#ifdef VERIFIED_EXEC
static int sysctl_kern_veriexec(SYSCTLFN_PROTO);
#endif
static int sysctl_kern_cpid(SYSCTLFN_PROTO);
static int sysctl_doeproc(SYSCTLFN_PROTO);
static int sysctl_kern_proc_args(SYSCTLFN_PROTO);
static int sysctl_hw_usermem(SYSCTLFN_PROTO);
static int sysctl_hw_cnmagic(SYSCTLFN_PROTO);
static int sysctl_hw_ncpu(SYSCTLFN_PROTO);

static void fill_kproc2(struct proc *, struct kinfo_proc2 *);
static void fill_lwp(struct lwp *l, struct kinfo_lwp *kl);
static void fill_file(struct kinfo_file *, const struct file *, struct proc *,
		      int);

/*
 * ********************************************************************
 * section 1: setup routines
 * ********************************************************************
 * these functions are stuffed into a link set for sysctl setup
 * functions.  they're never called or referenced from anywhere else.
 * ********************************************************************
 */

/*
 * sets up the base nodes...
 */
SYSCTL_SETUP(sysctl_root_setup, "sysctl base setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "kern",
		       SYSCTL_DESCR("High kernel"),
		       NULL, 0, NULL, 0,
		       CTL_KERN, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "vm",
		       SYSCTL_DESCR("Virtual memory"),
		       NULL, 0, NULL, 0,
		       CTL_VM, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "vfs",
		       SYSCTL_DESCR("Filesystem"),
		       NULL, 0, NULL, 0,
		       CTL_VFS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "net",
		       SYSCTL_DESCR("Networking"),
		       NULL, 0, NULL, 0,
		       CTL_NET, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "debug",
		       SYSCTL_DESCR("Debugging"),
		       NULL, 0, NULL, 0,
		       CTL_DEBUG, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "hw",
		       SYSCTL_DESCR("Generic CPU, I/O"),
		       NULL, 0, NULL, 0,
		       CTL_HW, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep",
		       SYSCTL_DESCR("Machine dependent"),
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);
	/*
	 * this node is inserted so that the sysctl nodes in libc can
	 * operate.
	 */
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "user",
		       SYSCTL_DESCR("User-level"),
		       NULL, 0, NULL, 0,
		       CTL_USER, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "ddb",
		       SYSCTL_DESCR("In-kernel debugger"),
		       NULL, 0, NULL, 0,
		       CTL_DDB, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "proc",
		       SYSCTL_DESCR("Per-process"),
		       NULL, 0, NULL, 0,
		       CTL_PROC, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_NODE, "vendor",
		       SYSCTL_DESCR("Vendor specific"),
		       NULL, 0, NULL, 0,
		       CTL_VENDOR, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "emul",
		       SYSCTL_DESCR("Emulation settings"),
		       NULL, 0, NULL, 0,
		       CTL_EMUL, CTL_EOL);
}

/*
 * this setup routine is a replacement for kern_sysctl()
 */
SYSCTL_SETUP(sysctl_kern_setup, "sysctl kern subtree setup")
{
	extern int kern_logsigexit;	/* defined in kern/kern_sig.c */
	extern fixpt_t ccpu;		/* defined in kern/kern_synch.c */
	extern int dumponpanic;		/* defined in kern/subr_prf.c */

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "kern", NULL,
		       NULL, 0, NULL, 0,
		       CTL_KERN, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "ostype",
		       SYSCTL_DESCR("Operating system type"),
		       NULL, 0, &ostype, 0,
		       CTL_KERN, KERN_OSTYPE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "osrelease",
		       SYSCTL_DESCR("Operating system release"),
		       NULL, 0, &osrelease, 0,
		       CTL_KERN, KERN_OSRELEASE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "osrevision",
		       SYSCTL_DESCR("Operating system revision"),
		       NULL, __NetBSD_Version__, NULL, 0,
		       CTL_KERN, KERN_OSREV, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "version",
		       SYSCTL_DESCR("Kernel version"),
		       NULL, 0, &version, 0,
		       CTL_KERN, KERN_VERSION, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "maxvnodes",
		       SYSCTL_DESCR("Maximum number of vnodes"),
		       sysctl_kern_maxvnodes, 0, NULL, 0,
		       CTL_KERN, KERN_MAXVNODES, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "maxproc",
		       SYSCTL_DESCR("Maximum number of simultaneous processes"),
		       sysctl_kern_maxproc, 0, NULL, 0,
		       CTL_KERN, KERN_MAXPROC, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "maxfiles",
		       SYSCTL_DESCR("Maximum number of open files"),
		       NULL, 0, &maxfiles, 0,
		       CTL_KERN, KERN_MAXFILES, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "argmax",
		       SYSCTL_DESCR("Maximum number of bytes of arguments to "
				    "execve(2)"),
		       NULL, ARG_MAX, NULL, 0,
		       CTL_KERN, KERN_ARGMAX, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "securelevel",
		       SYSCTL_DESCR("System security level"),
		       sysctl_kern_securelevel, 0, &securelevel, 0,
		       CTL_KERN, KERN_SECURELVL, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "hostname",
		       SYSCTL_DESCR("System hostname"),
		       sysctl_setlen, 0, &hostname, MAXHOSTNAMELEN,
		       CTL_KERN, KERN_HOSTNAME, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE|CTLFLAG_HEX,
		       CTLTYPE_INT, "hostid",
		       SYSCTL_DESCR("System host ID number"),
		       sysctl_kern_hostid, 0, NULL, 0,
		       CTL_KERN, KERN_HOSTID, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "clockrate",
		       SYSCTL_DESCR("Kernel clock rates"),
		       sysctl_kern_clockrate, 0, NULL,
		       sizeof(struct clockinfo),
		       CTL_KERN, KERN_CLOCKRATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "vnode",
		       SYSCTL_DESCR("System vnode table"),
		       sysctl_kern_vnode, 0, NULL, 0,
		       CTL_KERN, KERN_VNODE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "file",
		       SYSCTL_DESCR("System open file table"),
		       sysctl_kern_file, 0, NULL, 0,
		       CTL_KERN, KERN_FILE, CTL_EOL);
#ifndef GPROF
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "profiling",
		       SYSCTL_DESCR("Profiling information (not available)"),
		       sysctl_notavail, 0, NULL, 0,
		       CTL_KERN, KERN_PROF, CTL_EOL);
#endif
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "posix1version",
		       SYSCTL_DESCR("Version of ISO/IEC 9945 (POSIX 1003.1) "
				    "with which the operating system attempts "
				    "to comply"),
		       NULL, _POSIX_VERSION, NULL, 0,
		       CTL_KERN, KERN_POSIX1, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "ngroups",
		       SYSCTL_DESCR("Maximum number of supplemental groups"),
		       NULL, NGROUPS_MAX, NULL, 0,
		       CTL_KERN, KERN_NGROUPS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "job_control",
		       SYSCTL_DESCR("Whether job control is available"),
		       NULL, 1, NULL, 0,
		       CTL_KERN, KERN_JOB_CONTROL, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "saved_ids",
		       SYSCTL_DESCR("Whether POSIX saved set-group/user ID is "
				    "available"), NULL,
#ifdef _POSIX_SAVED_IDS
		       1,
#else /* _POSIX_SAVED_IDS */
		       0,
#endif /* _POSIX_SAVED_IDS */
		       NULL, 0, CTL_KERN, KERN_SAVED_IDS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "boottime",
		       SYSCTL_DESCR("System boot time"),
		       NULL, 0, &boottime, sizeof(boottime),
		       CTL_KERN, KERN_BOOTTIME, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "domainname",
		       SYSCTL_DESCR("YP domain name"),
		       sysctl_setlen, 0, &domainname, MAXHOSTNAMELEN,
		       CTL_KERN, KERN_DOMAINNAME, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "maxpartitions",
		       SYSCTL_DESCR("Maximum number of partitions allowed per "
				    "disk"),
		       NULL, MAXPARTITIONS, NULL, 0,
		       CTL_KERN, KERN_MAXPARTITIONS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "rawpartition",
		       SYSCTL_DESCR("Raw partition of a disk"),
		       NULL, RAW_PART, NULL, 0,
		       CTL_KERN, KERN_RAWPARTITION, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "timex", NULL,
		       sysctl_notavail, 0, NULL, 0,
		       CTL_KERN, KERN_TIMEX, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "autonicetime",
		       SYSCTL_DESCR("CPU clock seconds before non-root "
				    "process priority is lowered"),
		       sysctl_kern_autonice, 0, &autonicetime, 0,
		       CTL_KERN, KERN_AUTONICETIME, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "autoniceval",
		       SYSCTL_DESCR("Automatic reniced non-root process "
				    "priority"),
		       sysctl_kern_autonice, 0, &autoniceval, 0,
		       CTL_KERN, KERN_AUTONICEVAL, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "rtc_offset",
		       SYSCTL_DESCR("Offset of real time clock from UTC in "
				    "minutes"),
		       sysctl_kern_rtc_offset, 0, &rtc_offset, 0,
		       CTL_KERN, KERN_RTC_OFFSET, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "root_device",
		       SYSCTL_DESCR("Name of the root device"),
		       sysctl_root_device, 0, NULL, 0,
		       CTL_KERN, KERN_ROOT_DEVICE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "msgbufsize",
		       SYSCTL_DESCR("Size of the kernel message buffer"),
		       sysctl_msgbuf, 0, NULL, 0,
		       CTL_KERN, KERN_MSGBUFSIZE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "fsync",
		       SYSCTL_DESCR("Whether the POSIX 1003.1b File "
				    "Synchronization Option is available on "
				    "this system"),
		       NULL, 1, NULL, 0,
		       CTL_KERN, KERN_FSYNC, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "sysvmsg",
		       SYSCTL_DESCR("System V style message support available"),
		       NULL,
#ifdef SYSVMSG
		       1,
#else /* SYSVMSG */
		       0,
#endif /* SYSVMSG */
		       NULL, 0, CTL_KERN, KERN_SYSVMSG, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "sysvsem",
		       SYSCTL_DESCR("System V style semaphore support "
				    "available"), NULL,
#ifdef SYSVSEM
		       1,
#else /* SYSVSEM */
		       0,
#endif /* SYSVSEM */
		       NULL, 0, CTL_KERN, KERN_SYSVSEM, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "sysvshm",
		       SYSCTL_DESCR("System V style shared memory support "
				    "available"), NULL,
#ifdef SYSVSHM
		       1,
#else /* SYSVSHM */
		       0,
#endif /* SYSVSHM */
		       NULL, 0, CTL_KERN, KERN_SYSVSHM, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "synchronized_io",
		       SYSCTL_DESCR("Whether the POSIX 1003.1b Synchronized "
				    "I/O Option is available on this system"),
		       NULL, 1, NULL, 0,
		       CTL_KERN, KERN_SYNCHRONIZED_IO, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "iov_max",
		       SYSCTL_DESCR("Maximum number of iovec structures per "
				    "process"),
		       NULL, IOV_MAX, NULL, 0,
		       CTL_KERN, KERN_IOV_MAX, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "mapped_files",
		       SYSCTL_DESCR("Whether the POSIX 1003.1b Memory Mapped "
				    "Files Option is available on this system"),
		       NULL, 1, NULL, 0,
		       CTL_KERN, KERN_MAPPED_FILES, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "memlock",
		       SYSCTL_DESCR("Whether the POSIX 1003.1b Process Memory "
				    "Locking Option is available on this "
				    "system"),
		       NULL, 1, NULL, 0,
		       CTL_KERN, KERN_MEMLOCK, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "memlock_range",
		       SYSCTL_DESCR("Whether the POSIX 1003.1b Range Memory "
				    "Locking Option is available on this "
				    "system"),
		       NULL, 1, NULL, 0,
		       CTL_KERN, KERN_MEMLOCK_RANGE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "memory_protection",
		       SYSCTL_DESCR("Whether the POSIX 1003.1b Memory "
				    "Protection Option is available on this "
				    "system"),
		       NULL, 1, NULL, 0,
		       CTL_KERN, KERN_MEMORY_PROTECTION, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "login_name_max",
		       SYSCTL_DESCR("Maximum login name length"),
		       NULL, LOGIN_NAME_MAX, NULL, 0,
		       CTL_KERN, KERN_LOGIN_NAME_MAX, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "defcorename",
		       SYSCTL_DESCR("Default core file name"),
		       sysctl_kern_defcorename, 0, defcorename, MAXPATHLEN,
		       CTL_KERN, KERN_DEFCORENAME, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "logsigexit",
		       SYSCTL_DESCR("Log process exit when caused by signals"),
		       NULL, 0, &kern_logsigexit, 0,
		       CTL_KERN, KERN_LOGSIGEXIT, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "fscale",
		       SYSCTL_DESCR("Kernel fixed-point scale factor"),
		       NULL, FSCALE, NULL, 0,
		       CTL_KERN, KERN_FSCALE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "ccpu",
		       SYSCTL_DESCR("Scheduler exponential decay value"),
		       NULL, 0, &ccpu, 0,
		       CTL_KERN, KERN_CCPU, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "cp_time",
		       SYSCTL_DESCR("Clock ticks spent in different CPU states"),
		       sysctl_kern_cptime, 0, NULL, 0,
		       CTL_KERN, KERN_CP_TIME, CTL_EOL);
#if defined(SYSVMSG) || defined(SYSVSEM) || defined(SYSVSHM)
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "sysvipc_info",
		       SYSCTL_DESCR("System V style IPC information"),
		       sysctl_kern_sysvipc, 0, NULL, 0,
		       CTL_KERN, KERN_SYSVIPC_INFO, CTL_EOL);
#endif /* SYSVMSG || SYSVSEM || SYSVSHM */
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "msgbuf",
		       SYSCTL_DESCR("Kernel message buffer"),
		       sysctl_msgbuf, 0, NULL, 0,
		       CTL_KERN, KERN_MSGBUF, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "consdev",
		       SYSCTL_DESCR("Console device"),
		       sysctl_consdev, 0, NULL, sizeof(dev_t),
		       CTL_KERN, KERN_CONSDEV, CTL_EOL);
#if NPTY > 0
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "maxptys",
		       SYSCTL_DESCR("Maximum number of pseudo-ttys"),
		       sysctl_kern_maxptys, 0, NULL, 0,
		       CTL_KERN, KERN_MAXPTYS, CTL_EOL);
#endif /* NPTY > 0 */
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "maxphys",
		       SYSCTL_DESCR("Maximum raw I/O transfer size"),
		       NULL, MAXPHYS, NULL, 0,
		       CTL_KERN, KERN_MAXPHYS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "sbmax",
		       SYSCTL_DESCR("Maximum socket buffer size"),
		       sysctl_kern_sbmax, 0, NULL, 0,
		       CTL_KERN, KERN_SBMAX, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "monotonic_clock",
		       SYSCTL_DESCR("Implementation version of the POSIX "
				    "1003.1b Monotonic Clock Option"),
		       /* XXX _POSIX_VERSION */
		       NULL, _POSIX_MONOTONIC_CLOCK, NULL, 0,
		       CTL_KERN, KERN_MONOTONIC_CLOCK, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "urandom",
		       SYSCTL_DESCR("Random integer value"),
		       sysctl_kern_urnd, 0, NULL, 0,
		       CTL_KERN, KERN_URND, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "labelsector",
		       SYSCTL_DESCR("Sector number containing the disklabel"),
		       NULL, LABELSECTOR, NULL, 0,
		       CTL_KERN, KERN_LABELSECTOR, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "labeloffset",
		       SYSCTL_DESCR("Offset of the disklabel within the "
				    "sector"),
		       NULL, LABELOFFSET, NULL, 0,
		       CTL_KERN, KERN_LABELOFFSET, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "lwp",
		       SYSCTL_DESCR("System-wide LWP information"),
		       sysctl_kern_lwp, 0, NULL, 0,
		       CTL_KERN, KERN_LWP, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "forkfsleep",
		       SYSCTL_DESCR("Milliseconds to sleep on fork failure due "
				    "to process limits"),
		       sysctl_kern_forkfsleep, 0, NULL, 0,
		       CTL_KERN, KERN_FORKFSLEEP, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "posix_threads",
		       SYSCTL_DESCR("Version of IEEE Std 1003.1 and its "
				    "Threads option to which the system "
				    "attempts to conform"),
		       /* XXX _POSIX_VERSION */
		       NULL, _POSIX_THREADS, NULL, 0,
		       CTL_KERN, KERN_POSIX_THREADS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "posix_semaphores",
		       SYSCTL_DESCR("Version of IEEE Std 1003.1 and its "
				    "Semaphores option to which the system "
				    "attempts to conform"), NULL,
#ifdef P1003_1B_SEMAPHORE
		       200112,
#else /* P1003_1B_SEMAPHORE */
		       0,
#endif /* P1003_1B_SEMAPHORE */
		       NULL, 0, CTL_KERN, KERN_POSIX_SEMAPHORES, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "posix_barriers",
		       SYSCTL_DESCR("Version of IEEE Std 1003.1 and its "
				    "Barriers option to which the system "
				    "attempts to conform"),
		       /* XXX _POSIX_VERSION */
		       NULL, _POSIX_BARRIERS, NULL, 0,
		       CTL_KERN, KERN_POSIX_BARRIERS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "posix_timers",
		       SYSCTL_DESCR("Version of IEEE Std 1003.1 and its "
				    "Timers option to which the system "
				    "attempts to conform"),
		       /* XXX _POSIX_VERSION */
		       NULL, _POSIX_TIMERS, NULL, 0,
		       CTL_KERN, KERN_POSIX_TIMERS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "posix_spin_locks",
		       SYSCTL_DESCR("Version of IEEE Std 1003.1 and its Spin "
				    "Locks option to which the system attempts "
				    "to conform"),
		       /* XXX _POSIX_VERSION */
		       NULL, _POSIX_SPIN_LOCKS, NULL, 0,
		       CTL_KERN, KERN_POSIX_SPIN_LOCKS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "posix_reader_writer_locks",
		       SYSCTL_DESCR("Version of IEEE Std 1003.1 and its "
				    "Read-Write Locks option to which the "
				    "system attempts to conform"),
		       /* XXX _POSIX_VERSION */
		       NULL, _POSIX_READER_WRITER_LOCKS, NULL, 0,
		       CTL_KERN, KERN_POSIX_READER_WRITER_LOCKS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "dump_on_panic",
		       SYSCTL_DESCR("Perform a crash dump on system panic"),
		       NULL, 0, &dumponpanic, 0,
		       CTL_KERN, KERN_DUMP_ON_PANIC, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "root_partition",
		       SYSCTL_DESCR("Root partition on the root device"),
		       sysctl_kern_root_partition, 0, NULL, 0,
		       CTL_KERN, KERN_ROOT_PARTITION, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "drivers",
		       SYSCTL_DESCR("List of all drivers with block and "
				    "character device numbers"),
		       sysctl_kern_drivers, 0, NULL, 0,
		       CTL_KERN, KERN_DRIVERS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "file2",
		       SYSCTL_DESCR("System open file table"),
		       sysctl_kern_file2, 0, NULL, 0,
		       CTL_KERN, KERN_FILE2, CTL_EOL);
#ifdef VERIFIED_EXEC
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "veriexec",
		       SYSCTL_DESCR("Verified Exec"),
		       NULL, 0, NULL, 0,
		       CTL_KERN, KERN_VERIEXEC, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "verbose",
		       SYSCTL_DESCR("Verified Exec verbose level"),
		       NULL, 0, &veriexec_verbose, 0,
		       CTL_KERN, KERN_VERIEXEC, VERIEXEC_VERBOSE,
		       CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "strict",
		       SYSCTL_DESCR("Verified Exec strict level"),
		       sysctl_kern_veriexec, 0, NULL, 0,
		       CTL_KERN, KERN_VERIEXEC, VERIEXEC_STRICT, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "algorithms",
		       SYSCTL_DESCR("Verified Exec supported hashing "
				    "algorithms"),
		       sysctl_kern_veriexec, 0, NULL, 0,
		       CTL_KERN, KERN_VERIEXEC, VERIEXEC_ALGORITHMS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, &veriexec_count_node,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "count",
		       SYSCTL_DESCR("Number of fingerprints on device(s)"),
		       NULL, 0, NULL, 0,
		       CTL_KERN, KERN_VERIEXEC, VERIEXEC_COUNT, CTL_EOL);
#endif /* VERIFIED_EXEC */
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "cp_id",
		       SYSCTL_DESCR("Mapping of CPU number to CPU id"),
		       sysctl_kern_cpid, 0, NULL, 0,
		       CTL_KERN, KERN_CP_ID, CTL_EOL);
}

SYSCTL_SETUP(sysctl_kern_proc_setup,
	     "sysctl kern.proc/proc2/proc_args subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "kern", NULL,
		       NULL, 0, NULL, 0,
		       CTL_KERN, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "proc",
		       SYSCTL_DESCR("System-wide process information"),
		       sysctl_doeproc, 0, NULL, 0,
		       CTL_KERN, KERN_PROC, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "proc2",
		       SYSCTL_DESCR("Machine-independent process information"),
		       sysctl_doeproc, 0, NULL, 0,
		       CTL_KERN, KERN_PROC2, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
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

SYSCTL_SETUP(sysctl_hw_setup, "sysctl hw subtree setup")
{
	u_int u;
	u_quad_t q;

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "hw", NULL,
		       NULL, 0, NULL, 0,
		       CTL_HW, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "machine",
		       SYSCTL_DESCR("Machine class"),
		       NULL, 0, machine, 0,
		       CTL_HW, HW_MACHINE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "model",
		       SYSCTL_DESCR("Machine model"),
		       NULL, 0, cpu_model, 0,
		       CTL_HW, HW_MODEL, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "ncpu",
		       SYSCTL_DESCR("Number of active CPUs"),
		       sysctl_hw_ncpu, 0, NULL, 0,
		       CTL_HW, HW_NCPU, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "byteorder",
		       SYSCTL_DESCR("System byte order"),
		       NULL, BYTE_ORDER, NULL, 0,
		       CTL_HW, HW_BYTEORDER, CTL_EOL);
	u = ((u_int)physmem > (UINT_MAX / PAGE_SIZE)) ?
		UINT_MAX : physmem * PAGE_SIZE;
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "physmem",
		       SYSCTL_DESCR("Bytes of physical memory"),
		       NULL, u, NULL, 0,
		       CTL_HW, HW_PHYSMEM, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "usermem",
		       SYSCTL_DESCR("Bytes of non-kernel memory"),
		       sysctl_hw_usermem, 0, NULL, 0,
		       CTL_HW, HW_USERMEM, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "pagesize",
		       SYSCTL_DESCR("Software page size"),
		       NULL, PAGE_SIZE, NULL, 0,
		       CTL_HW, HW_PAGESIZE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "disknames",
		       SYSCTL_DESCR("List of disk devices present"),
		       sysctl_hw_disknames, 0, NULL, 0,
		       CTL_HW, HW_DISKNAMES, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "diskstats",
		       SYSCTL_DESCR("Statistics on disk operation"),
		       sysctl_hw_diskstats, 0, NULL, 0,
		       CTL_HW, HW_DISKSTATS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "machine_arch",
		       SYSCTL_DESCR("Machine CPU class"),
		       NULL, 0, machine_arch, 0,
		       CTL_HW, HW_MACHINE_ARCH, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "alignbytes",
		       SYSCTL_DESCR("Alignment constraint for all possible "
				    "data types"),
		       NULL, ALIGNBYTES, NULL, 0,
		       CTL_HW, HW_ALIGNBYTES, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE|CTLFLAG_HEX,
		       CTLTYPE_STRING, "cnmagic",
		       SYSCTL_DESCR("Console magic key sequence"),
		       sysctl_hw_cnmagic, 0, NULL, CNS_LEN,
		       CTL_HW, HW_CNMAGIC, CTL_EOL);
	q = (u_quad_t)physmem * PAGE_SIZE;
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_QUAD, "physmem64",
		       SYSCTL_DESCR("Bytes of physical memory"),
		       NULL, q, NULL, 0,
		       CTL_HW, HW_PHYSMEM64, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_QUAD, "usermem64",
		       SYSCTL_DESCR("Bytes of non-kernel memory"),
		       sysctl_hw_usermem, 0, NULL, 0,
		       CTL_HW, HW_USERMEM64, CTL_EOL);
}

#ifdef DEBUG
/*
 * Debugging related system variables.
 */
struct ctldebug /* debug0, */ /* debug1, */ debug2, debug3, debug4;
struct ctldebug debug5, debug6, debug7, debug8, debug9;
struct ctldebug debug10, debug11, debug12, debug13, debug14;
struct ctldebug debug15, debug16, debug17, debug18, debug19;
static struct ctldebug *debugvars[CTL_DEBUG_MAXID] = {
	&debug0, &debug1, &debug2, &debug3, &debug4,
	&debug5, &debug6, &debug7, &debug8, &debug9,
	&debug10, &debug11, &debug12, &debug13, &debug14,
	&debug15, &debug16, &debug17, &debug18, &debug19,
};

/*
 * this setup routine is a replacement for debug_sysctl()
 *
 * note that it creates several nodes per defined debug variable
 */
SYSCTL_SETUP(sysctl_debug_setup, "sysctl debug subtree setup")
{
	struct ctldebug *cdp;
	char nodename[20];
	int i;

	/*
	 * two ways here:
	 *
	 * the "old" way (debug.name -> value) which was emulated by
	 * the sysctl(8) binary
	 *
	 * the new way, which the sysctl(8) binary was actually using

	 node	debug
	 node	debug.0
	 string	debug.0.name
	 int	debug.0.value
	 int	debug.name

	 */

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "debug", NULL,
		       NULL, 0, NULL, 0,
		       CTL_DEBUG, CTL_EOL);

	for (i = 0; i < CTL_DEBUG_MAXID; i++) {
		cdp = debugvars[i];
		if (cdp->debugname == NULL || cdp->debugvar == NULL)
			continue;

		snprintf(nodename, sizeof(nodename), "debug%d", i);
		sysctl_createv(clog, 0, NULL, NULL,
			       CTLFLAG_PERMANENT|CTLFLAG_HIDDEN,
			       CTLTYPE_NODE, nodename, NULL,
			       NULL, 0, NULL, 0,
			       CTL_DEBUG, i, CTL_EOL);
		sysctl_createv(clog, 0, NULL, NULL,
			       CTLFLAG_PERMANENT|CTLFLAG_HIDDEN,
			       CTLTYPE_STRING, "name", NULL,
			       /*XXXUNCONST*/
			       NULL, 0, __UNCONST(cdp->debugname), 0,
			       CTL_DEBUG, i, CTL_DEBUG_NAME, CTL_EOL);
		sysctl_createv(clog, 0, NULL, NULL,
			       CTLFLAG_PERMANENT|CTLFLAG_HIDDEN,
			       CTLTYPE_INT, "value", NULL,
			       NULL, 0, cdp->debugvar, 0,
			       CTL_DEBUG, i, CTL_DEBUG_VALUE, CTL_EOL);
		sysctl_createv(clog, 0, NULL, NULL,
			       CTLFLAG_PERMANENT,
			       CTLTYPE_INT, cdp->debugname, NULL,
			       NULL, 0, cdp->debugvar, 0,
			       CTL_DEBUG, CTL_CREATE, CTL_EOL);
	}
}
#endif /* DEBUG */

/*
 * ********************************************************************
 * section 2: private node-specific helper routines.
 * ********************************************************************
 */

/*
 * sysctl helper routine for kern.maxvnodes.  drain vnodes if
 * new value is lower than desiredvnodes and then calls reinit
 * routines that needs to adjust to the new value.
 */
static int
sysctl_kern_maxvnodes(SYSCTLFN_ARGS)
{
	int error, new_vnodes, old_vnodes;
	struct sysctlnode node;

	new_vnodes = desiredvnodes;
	node = *rnode;
	node.sysctl_data = &new_vnodes;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	old_vnodes = desiredvnodes;
	desiredvnodes = new_vnodes;
	if (new_vnodes < old_vnodes) {
		error = vfs_drainvnodes(new_vnodes, l->l_proc);
		if (error) {
			desiredvnodes = old_vnodes;
			return (error);
		}
	}
	vfs_reinit();
	nchreinit();

	return (0);
}

/*
 * sysctl helper routine for rtc_offset - set time after changes
 */
static int
sysctl_kern_rtc_offset(SYSCTLFN_ARGS)
{
	struct timeval tv, delta;
	int s, error, new_rtc_offset;
	struct sysctlnode node;

	new_rtc_offset = rtc_offset;
	node = *rnode;
	node.sysctl_data = &new_rtc_offset;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	if (securelevel > 0)
		return (EPERM);
	if (rtc_offset == new_rtc_offset)
		return (0);

	/* if we change the offset, adjust the time */
	s = splclock();
	tv = time;
	splx(s);
	delta.tv_sec = 60*(new_rtc_offset - rtc_offset);
	delta.tv_usec = 0;
	timeradd(&tv, &delta, &tv);
	rtc_offset = new_rtc_offset;
	settime(&tv);

	return (0);
}

/*
 * sysctl helper routine for kern.maxvnodes.  ensures that the new
 * values are not too low or too high.
 */
static int
sysctl_kern_maxproc(SYSCTLFN_ARGS)
{
	int error, nmaxproc;
	struct sysctlnode node;

	nmaxproc = maxproc;
	node = *rnode;
	node.sysctl_data = &nmaxproc;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	if (nmaxproc < 0 || nmaxproc >= PID_MAX)
		return (EINVAL);
#ifdef __HAVE_CPU_MAXPROC
	if (nmaxproc > cpu_maxproc())
		return (EINVAL);
#endif
	maxproc = nmaxproc;

	return (0);
}

/*
 * sysctl helper routine for kern.securelevel.  ensures that the value
 * only rises unless the caller has pid 1 (assumed to be init).
 */
static int
sysctl_kern_securelevel(SYSCTLFN_ARGS)
{
	int newsecurelevel, error;
	struct sysctlnode node;

	newsecurelevel = securelevel;
	node = *rnode;
	node.sysctl_data = &newsecurelevel;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	if (newsecurelevel < securelevel && l && l->l_proc->p_pid != 1)
		return (EPERM);
	securelevel = newsecurelevel;

	return (error);
}

/*
 * sysctl helper function for kern.hostid.  the hostid is a long, but
 * we export it as an int, so we need to give it a little help.
 */
static int
sysctl_kern_hostid(SYSCTLFN_ARGS)
{
	int error, inthostid;
	struct sysctlnode node;

	inthostid = hostid;  /* XXX assumes sizeof int <= sizeof long */
	node = *rnode;
	node.sysctl_data = &inthostid;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	hostid = (unsigned)inthostid;

	return (0);
}

/*
 * sysctl helper function for kern.hostname and kern.domainnname.
 * resets the relevant recorded length when the underlying name is
 * changed.
 */
static int
sysctl_setlen(SYSCTLFN_ARGS)
{
	int error;

	error = sysctl_lookup(SYSCTLFN_CALL(rnode));
	if (error || newp == NULL)
		return (error);

	switch (rnode->sysctl_num) {
	case KERN_HOSTNAME:
		hostnamelen = strlen((const char*)rnode->sysctl_data);
		break;
	case KERN_DOMAINNAME:
		domainnamelen = strlen((const char*)rnode->sysctl_data);
		break;
	}

	return (0);
}

/*
 * sysctl helper routine for kern.clockrate.  assembles a struct on
 * the fly to be returned to the caller.
 */
static int
sysctl_kern_clockrate(SYSCTLFN_ARGS)
{
	struct clockinfo clkinfo;
	struct sysctlnode node;

	clkinfo.tick = tick;
	clkinfo.tickadj = tickadj;
	clkinfo.hz = hz;
	clkinfo.profhz = profhz;
	clkinfo.stathz = stathz ? stathz : hz;

	node = *rnode;
	node.sysctl_data = &clkinfo;
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}


/*
 * sysctl helper routine for kern.file pseudo-subtree.
 */
static int
sysctl_kern_file(SYSCTLFN_ARGS)
{
	int error;
	size_t buflen;
	struct file *fp;
	char *start, *where;

	start = where = oldp;
	buflen = *oldlenp;
	if (where == NULL) {
		/*
		 * overestimate by 10 files
		 */
		*oldlenp = sizeof(filehead) + (nfiles + 10) * sizeof(struct file);
		return (0);
	}

	/*
	 * first copyout filehead
	 */
	if (buflen < sizeof(filehead)) {
		*oldlenp = 0;
		return (0);
	}
	error = copyout(&filehead, where, sizeof(filehead));
	if (error)
		return (error);
	buflen -= sizeof(filehead);
	where += sizeof(filehead);

	/*
	 * followed by an array of file structures
	 */
	LIST_FOREACH(fp, &filehead, f_list) {
		if (buflen < sizeof(struct file)) {
			*oldlenp = where - start;
			return (ENOMEM);
		}
		error = copyout(fp, where, sizeof(struct file));
		if (error)
			return (error);
		buflen -= sizeof(struct file);
		where += sizeof(struct file);
	}
	*oldlenp = where - start;
	return (0);
}

/*
 * sysctl helper routine for kern.autonicetime and kern.autoniceval.
 * asserts that the assigned value is in the correct range.
 */
static int
sysctl_kern_autonice(SYSCTLFN_ARGS)
{
	int error, t = 0;
	struct sysctlnode node;

	node = *rnode;
	t = *(int*)node.sysctl_data;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	switch (node.sysctl_num) {
	case KERN_AUTONICETIME:
		if (t >= 0)
			autonicetime = t;
		break;
	case KERN_AUTONICEVAL:
		if (t < PRIO_MIN)
			t = PRIO_MIN;
		else if (t > PRIO_MAX)
			t = PRIO_MAX;
		autoniceval = t;
		break;
	}

	return (0);
}

/*
 * sysctl helper routine for kern.msgbufsize and kern.msgbuf.  for the
 * former it merely checks the message buffer is set up.  for the latter,
 * it also copies out the data if necessary.
 */
static int
sysctl_msgbuf(SYSCTLFN_ARGS)
{
	char *where = oldp;
	size_t len, maxlen;
	long beg, end;
	int error;

	if (!msgbufenabled || msgbufp->msg_magic != MSG_MAGIC) {
		msgbufenabled = 0;
		return (ENXIO);
	}

	switch (rnode->sysctl_num) {
	case KERN_MSGBUFSIZE: {
		struct sysctlnode node = *rnode;
		int msg_bufs = (int)msgbufp->msg_bufs;
		node.sysctl_data = &msg_bufs;
		return (sysctl_lookup(SYSCTLFN_CALL(&node)));
	}
	case KERN_MSGBUF:
		break;
	default:
		return (EOPNOTSUPP);
	}

	if (newp != NULL)
		return (EPERM);

        if (oldp == NULL) {
		/* always return full buffer size */
		*oldlenp = msgbufp->msg_bufs;
		return (0);
        }

	error = 0;
	maxlen = MIN(msgbufp->msg_bufs, *oldlenp);

	/*
	 * First, copy from the write pointer to the end of
	 * message buffer.
	 */
	beg = msgbufp->msg_bufx;
	end = msgbufp->msg_bufs;
	while (maxlen > 0) {
		len = MIN(end - beg, maxlen);
		if (len == 0)
			break;
		error = copyout(&msgbufp->msg_bufc[beg], where, len);
		if (error)
			break;
		where += len;
		maxlen -= len;

		/*
		 * ... then, copy from the beginning of message buffer to
		 * the write pointer.
		 */
		beg = 0;
		end = msgbufp->msg_bufx;
	}

	return (error);
}

/*
 * sysctl helper routine for kern.defcorename.  in the case of a new
 * string being assigned, check that it's not a zero-length string.
 * (XXX the check in -current doesn't work, but do we really care?)
 */
static int
sysctl_kern_defcorename(SYSCTLFN_ARGS)
{
	int error;
	char newcorename[MAXPATHLEN];
	struct sysctlnode node;

	node = *rnode;
	node.sysctl_data = &newcorename[0];
	memcpy(node.sysctl_data, rnode->sysctl_data, MAXPATHLEN);
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	/*
	 * when sysctl_lookup() deals with a string, it's guaranteed
	 * to come back nul terminated.  so there.  :)
	 */
	if (strlen(newcorename) == 0)
		return (EINVAL);

	memcpy(rnode->sysctl_data, node.sysctl_data, MAXPATHLEN);

	return (0);
}

/*
 * sysctl helper routine for kern.cp_time node.  adds up cpu time
 * across all cpus.
 */
static int
sysctl_kern_cptime(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;

#ifndef MULTIPROCESSOR

	if (namelen == 1) {
		if (name[0] != 0)
			return (ENOENT);
		/*
		 * you're allowed to ask for the zero'th processor
		 */
		name++;
		namelen--;
	}
	node.sysctl_data = curcpu()->ci_schedstate.spc_cp_time;
	node.sysctl_size = sizeof(curcpu()->ci_schedstate.spc_cp_time);
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));

#else /* MULTIPROCESSOR */

	u_int64_t *cp_time = NULL;
	int error, n = sysctl_ncpus(), i;
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;

	/*
	 * if you specifically pass a buffer that is the size of the
	 * sum, or if you are probing for the size, you get the "sum"
	 * of cp_time (and the size thereof) across all processors.
	 *
	 * alternately, you can pass an additional mib number and get
	 * cp_time for that particular processor.
	 */
	switch (namelen) {
	case 0:
	    	if (*oldlenp == sizeof(u_int64_t) * CPUSTATES || oldp == NULL) {
			node.sysctl_size = sizeof(u_int64_t) * CPUSTATES;
			n = -1; /* SUM */
		}
		else {
			node.sysctl_size = n * sizeof(u_int64_t) * CPUSTATES;
			n = -2; /* ALL */
		}
		break;
	case 1:
		if (name[0] < 0 || name[0] >= n)
			return (ENOENT); /* ENOSUCHPROCESSOR */
		node.sysctl_size = sizeof(u_int64_t) * CPUSTATES;
		n = name[0];
		/*
		 * adjust these so that sysctl_lookup() will be happy
		 */
		name++;
		namelen--;
		break;
	default:
		return (EINVAL);
	}

	cp_time = malloc(node.sysctl_size, M_TEMP, M_WAITOK|M_CANFAIL);
	if (cp_time == NULL)
		return (ENOMEM);
	node.sysctl_data = cp_time;
	memset(cp_time, 0, node.sysctl_size);

	for (CPU_INFO_FOREACH(cii, ci)) {
		if (n <= 0)
			for (i = 0; i < CPUSTATES; i++)
				cp_time[i] += ci->ci_schedstate.spc_cp_time[i];
		/*
		 * if a specific processor was requested and we just
		 * did it, we're done here
		 */
		if (n == 0)
			break;
		/*
		 * if doing "all", skip to next cp_time set for next processor
		 */
		if (n == -2)
			cp_time += CPUSTATES;
		/*
		 * if we're doing a specific processor, we're one
		 * processor closer
		 */
		if (n > 0)
			n--;
	}

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	free(node.sysctl_data, M_TEMP);
	return (error);

#endif /* MULTIPROCESSOR */
}

#if defined(SYSVMSG) || defined(SYSVSEM) || defined(SYSVSHM)
/*
 * sysctl helper routine for kern.sysvipc_info subtree.
 */

#define	FILL_PERM(src, dst) do { \
	(dst)._key = (src)._key; \
	(dst).uid = (src).uid; \
	(dst).gid = (src).gid; \
	(dst).cuid = (src).cuid; \
	(dst).cgid = (src).cgid; \
	(dst).mode = (src).mode; \
	(dst)._seq = (src)._seq; \
} while (/*CONSTCOND*/ 0);
#define	FILL_MSG(src, dst) do { \
	FILL_PERM((src).msg_perm, (dst).msg_perm); \
	(dst).msg_qnum = (src).msg_qnum; \
	(dst).msg_qbytes = (src).msg_qbytes; \
	(dst)._msg_cbytes = (src)._msg_cbytes; \
	(dst).msg_lspid = (src).msg_lspid; \
	(dst).msg_lrpid = (src).msg_lrpid; \
	(dst).msg_stime = (src).msg_stime; \
	(dst).msg_rtime = (src).msg_rtime; \
	(dst).msg_ctime = (src).msg_ctime; \
} while (/*CONSTCOND*/ 0)
#define	FILL_SEM(src, dst) do { \
	FILL_PERM((src).sem_perm, (dst).sem_perm); \
	(dst).sem_nsems = (src).sem_nsems; \
	(dst).sem_otime = (src).sem_otime; \
	(dst).sem_ctime = (src).sem_ctime; \
} while (/*CONSTCOND*/ 0)
#define	FILL_SHM(src, dst) do { \
	FILL_PERM((src).shm_perm, (dst).shm_perm); \
	(dst).shm_segsz = (src).shm_segsz; \
	(dst).shm_lpid = (src).shm_lpid; \
	(dst).shm_cpid = (src).shm_cpid; \
	(dst).shm_atime = (src).shm_atime; \
	(dst).shm_dtime = (src).shm_dtime; \
	(dst).shm_ctime = (src).shm_ctime; \
	(dst).shm_nattch = (src).shm_nattch; \
} while (/*CONSTCOND*/ 0)

static int
sysctl_kern_sysvipc(SYSCTLFN_ARGS)
{
	void *where = oldp;
	size_t *sizep = oldlenp;
#ifdef SYSVMSG
	struct msg_sysctl_info *msgsi = NULL;
#endif
#ifdef SYSVSEM
	struct sem_sysctl_info *semsi = NULL;
#endif
#ifdef SYSVSHM
	struct shm_sysctl_info *shmsi = NULL;
#endif
	size_t infosize, dssize, tsize, buflen;
	void *bf = NULL;
	char *start;
	int32_t nds;
	int i, error, ret;

	if (namelen != 1)
		return (EINVAL);

	start = where;
	buflen = *sizep;

	switch (*name) {
	case KERN_SYSVIPC_MSG_INFO:
#ifdef SYSVMSG
		infosize = sizeof(msgsi->msginfo);
		nds = msginfo.msgmni;
		dssize = sizeof(msgsi->msgids[0]);
		break;
#else
		return (EINVAL);
#endif
	case KERN_SYSVIPC_SEM_INFO:
#ifdef SYSVSEM
		infosize = sizeof(semsi->seminfo);
		nds = seminfo.semmni;
		dssize = sizeof(semsi->semids[0]);
		break;
#else
		return (EINVAL);
#endif
	case KERN_SYSVIPC_SHM_INFO:
#ifdef SYSVSHM
		infosize = sizeof(shmsi->shminfo);
		nds = shminfo.shmmni;
		dssize = sizeof(shmsi->shmids[0]);
		break;
#else
		return (EINVAL);
#endif
	default:
		return (EINVAL);
	}
	/*
	 * Round infosize to 64 bit boundary if requesting more than just
	 * the info structure or getting the total data size.
	 */
	if (where == NULL || *sizep > infosize)
		infosize = ((infosize + 7) / 8) * 8;
	tsize = infosize + nds * dssize;

	/* Return just the total size required. */
	if (where == NULL) {
		*sizep = tsize;
		return (0);
	}

	/* Not enough room for even the info struct. */
	if (buflen < infosize) {
		*sizep = 0;
		return (ENOMEM);
	}
	bf = malloc(min(tsize, buflen), M_TEMP, M_WAITOK);
	memset(bf, 0, min(tsize, buflen));

	switch (*name) {
#ifdef SYSVMSG
	case KERN_SYSVIPC_MSG_INFO:
		msgsi = (struct msg_sysctl_info *)bf;
		msgsi->msginfo = msginfo;
		break;
#endif
#ifdef SYSVSEM
	case KERN_SYSVIPC_SEM_INFO:
		semsi = (struct sem_sysctl_info *)bf;
		semsi->seminfo = seminfo;
		break;
#endif
#ifdef SYSVSHM
	case KERN_SYSVIPC_SHM_INFO:
		shmsi = (struct shm_sysctl_info *)bf;
		shmsi->shminfo = shminfo;
		break;
#endif
	}
	buflen -= infosize;

	ret = 0;
	if (buflen > 0) {
		/* Fill in the IPC data structures.  */
		for (i = 0; i < nds; i++) {
			if (buflen < dssize) {
				ret = ENOMEM;
				break;
			}
			switch (*name) {
#ifdef SYSVMSG
			case KERN_SYSVIPC_MSG_INFO:
				FILL_MSG(msqids[i], msgsi->msgids[i]);
				break;
#endif
#ifdef SYSVSEM
			case KERN_SYSVIPC_SEM_INFO:
				FILL_SEM(sema[i], semsi->semids[i]);
				break;
#endif
#ifdef SYSVSHM
			case KERN_SYSVIPC_SHM_INFO:
				FILL_SHM(shmsegs[i], shmsi->shmids[i]);
				break;
#endif
			}
			buflen -= dssize;
		}
	}
	*sizep -= buflen;
	error = copyout(bf, start, *sizep);
	/* If copyout succeeded, use return code set earlier. */
	if (error == 0)
		error = ret;
	if (bf)
		free(bf, M_TEMP);
	return (error);
}

#undef FILL_PERM
#undef FILL_MSG
#undef FILL_SEM
#undef FILL_SHM

#endif /* defined(SYSVMSG) || defined(SYSVSEM) || defined(SYSVSHM) */

#if NPTY > 0
/*
 * sysctl helper routine for kern.maxptys.  ensures that any new value
 * is acceptable to the pty subsystem.
 */
static int
sysctl_kern_maxptys(SYSCTLFN_ARGS)
{
	int pty_maxptys(int, int);		/* defined in kern/tty_pty.c */
	int error, xmax;
	struct sysctlnode node;

	/* get current value of maxptys */
	xmax = pty_maxptys(0, 0);

	node = *rnode;
	node.sysctl_data = &xmax;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	if (xmax != pty_maxptys(xmax, 1))
		return (EINVAL);

	return (0);
}
#endif /* NPTY > 0 */

/*
 * sysctl helper routine for kern.sbmax.  basically just ensures that
 * any new value is not too small.
 */
static int
sysctl_kern_sbmax(SYSCTLFN_ARGS)
{
	int error, new_sbmax;
	struct sysctlnode node;

	new_sbmax = sb_max;
	node = *rnode;
	node.sysctl_data = &new_sbmax;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	error = sb_max_set(new_sbmax);

	return (error);
}

/*
 * sysctl helper routine for kern.urandom node.  picks a random number
 * for you.
 */
static int
sysctl_kern_urnd(SYSCTLFN_ARGS)
{
#if NRND > 0
	int v;

	if (rnd_extract_data(&v, sizeof(v), RND_EXTRACT_ANY) == sizeof(v)) {
		struct sysctlnode node = *rnode;
		node.sysctl_data = &v;
		return (sysctl_lookup(SYSCTLFN_CALL(&node)));
	}
	else
		return (EIO);	/*XXX*/
#else
	return (EOPNOTSUPP);
#endif
}

/*
 * sysctl helper routine to do kern.lwp.* work.
 */
static int
sysctl_kern_lwp(SYSCTLFN_ARGS)
{
	struct kinfo_lwp klwp;
	struct proc *p;
	struct lwp *l2;
	char *where, *dp;
	int pid, elem_size, elem_count;
	int buflen, needed, error;

	if (namelen == 1 && name[0] == CTL_QUERY)
		return (sysctl_query(SYSCTLFN_CALL(rnode)));

	dp = where = oldp;
	buflen = where != NULL ? *oldlenp : 0;
	error = needed = 0;

	if (newp != NULL || namelen != 3)
		return (EINVAL);
	pid = name[0];
	elem_size = name[1];
	elem_count = name[2];

	p = pfind(pid);
	if (p == NULL)
		return (ESRCH);
	LIST_FOREACH(l2, &p->p_lwps, l_sibling) {
		if (buflen >= elem_size && elem_count > 0) {
			fill_lwp(l2, &klwp);
			/*
			 * Copy out elem_size, but not larger than
			 * the size of a struct kinfo_proc2.
			 */
			error = copyout(&klwp, dp,
			    min(sizeof(klwp), elem_size));
			if (error)
				goto cleanup;
			dp += elem_size;
			buflen -= elem_size;
			elem_count--;
		}
		needed += elem_size;
	}

	if (where != NULL) {
		*oldlenp = dp - where;
		if (needed > *oldlenp)
			return (ENOMEM);
	} else {
		needed += KERN_LWPSLOP;
		*oldlenp = needed;
	}
	return (0);
 cleanup:
	return (error);
}

/*
 * sysctl helper routine for kern.forkfsleep node.  ensures that the
 * given value is not too large or two small, and is at least one
 * timer tick if not zero.
 */
static int
sysctl_kern_forkfsleep(SYSCTLFN_ARGS)
{
	/* userland sees value in ms, internally is in ticks */
	extern int forkfsleep;		/* defined in kern/kern_fork.c */
	int error, timo, lsleep;
	struct sysctlnode node;

	lsleep = forkfsleep * 1000 / hz;
	node = *rnode;
	node.sysctl_data = &lsleep;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	/* refuse negative values, and overly 'long time' */
	if (lsleep < 0 || lsleep > MAXSLP * 1000)
		return (EINVAL);

	timo = mstohz(lsleep);

	/* if the interval is >0 ms && <1 tick, use 1 tick */
	if (lsleep != 0 && timo == 0)
		forkfsleep = 1;
	else
		forkfsleep = timo;

	return (0);
}

/*
 * sysctl helper routine for kern.root_partition
 */
static int
sysctl_kern_root_partition(SYSCTLFN_ARGS)
{
	int rootpart = DISKPART(rootdev);
	struct sysctlnode node = *rnode;

	node.sysctl_data = &rootpart;
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

/*
 * sysctl helper function for kern.drivers
 */
static int
sysctl_kern_drivers(SYSCTLFN_ARGS)
{
	int error;
	size_t buflen;
	struct kinfo_drivers kd;
	char *start, *where;
	const char *dname;
	int i;
	extern struct devsw_conv *devsw_conv;
	extern int max_devsw_convs;

	if (newp != NULL || namelen != 0)
		return (EINVAL);

	start = where = oldp;
	buflen = *oldlenp;
	if (where == NULL) {
		*oldlenp = max_devsw_convs * sizeof kd;
		return 0;
	}

	/*
	 * An array of kinfo_drivers structures
	 */
	error = 0;
	for (i = 0; i < max_devsw_convs; i++) {
		dname = devsw_conv[i].d_name;
		if (dname == NULL)
			continue;
		if (buflen < sizeof kd) {
			error = ENOMEM;
			break;
		}
		memset(&kd, 0, sizeof(kd));
		kd.d_bmajor = devsw_conv[i].d_bmajor;
		kd.d_cmajor = devsw_conv[i].d_cmajor;
		strlcpy(kd.d_name, dname, sizeof kd.d_name);
		error = copyout(&kd, where, sizeof kd);
		if (error != 0)
			break;
		buflen -= sizeof kd;
		where += sizeof kd;
	}
	*oldlenp = where - start;
	return error;
}

/*
 * sysctl helper function for kern.file2
 */
static int
sysctl_kern_file2(SYSCTLFN_ARGS)
{
	struct proc *p;
	struct file *fp;
	struct filedesc *fd;
	struct kinfo_file kf;
	char *dp;
	u_int i, op;
	size_t len, needed, elem_size, out_size;
	int error, arg, elem_count;

	if (namelen == 1 && name[0] == CTL_QUERY)
		return (sysctl_query(SYSCTLFN_CALL(rnode)));

	if (namelen != 4)
		return (EINVAL);

	error = 0;
	dp = oldp;
	len = (oldp != NULL) ? *oldlenp : 0;
	op = name[0];
	arg = name[1];
	elem_size = name[2];
	elem_count = name[3];
	out_size = MIN(sizeof(kf), elem_size);
	needed = 0;

	if (elem_size < 1 || elem_count < 0)
		return (EINVAL);

	switch (op) {
	case KERN_FILE_BYFILE:
		/*
		 * doesn't use arg so it must be zero
		 */
		if (arg != 0)
			return (EINVAL);
		LIST_FOREACH(fp, &filehead, f_list) {
			if (len >= elem_size && elem_count > 0) {
				fill_file(&kf, fp, NULL, 0);
				error = copyout(&kf, dp, out_size);
				if (error)
					break;
				dp += elem_size;
				len -= elem_size;
			}
			if (elem_count > 0) {
				needed += elem_size;
				if (elem_count != INT_MAX)
					elem_count--;
			}
		}
		break;
	    case KERN_FILE_BYPID:
		if (arg < -1)
			/* -1 means all processes */
			return (EINVAL);
		proclist_lock_read();
		PROCLIST_FOREACH(p, &allproc) {
			if (p->p_stat == SIDL)
				/* skip embryonic processes */
				continue;
			if (arg > 0 && p->p_pid != arg)
				/* pick only the one we want */
				/* XXX want 0 to mean "kernel files" */
				continue;
			fd = p->p_fd;
			for (i = 0; i < fd->fd_nfiles; i++) {
				fp = fd->fd_ofiles[i];
				if (fp == NULL || !FILE_IS_USABLE(fp))
					continue;
				if (len >= elem_size && elem_count > 0) {
					fill_file(&kf, fd->fd_ofiles[i],
						  p, i);
					error = copyout(&kf, dp, out_size);
					if (error)
						break;
					dp += elem_size;
					len -= elem_size;
				}
				if (elem_count > 0) {
					needed += elem_size;
					if (elem_count != INT_MAX)
						elem_count--;
				}
			}
		}
		proclist_unlock_read();
		break;
	default:
		return (EINVAL);
	}

	if (oldp == NULL)
		needed += KERN_FILESLOP * elem_size;
	*oldlenp = needed;

	return (error);
}

static void
fill_file(struct kinfo_file *kp, const struct file *fp, struct proc *p, int i)
{

	memset(kp, 0, sizeof(*kp));

	kp->ki_fileaddr =	PTRTOUINT64(fp);
	kp->ki_flag =		fp->f_flag;
	kp->ki_iflags =		fp->f_iflags;
	kp->ki_ftype =		fp->f_type;
	kp->ki_count =		fp->f_count;
	kp->ki_msgcount =	fp->f_msgcount;
	kp->ki_usecount =	fp->f_usecount;
	kp->ki_fucred =		PTRTOUINT64(fp->f_cred);
	kp->ki_fuid =		fp->f_cred->cr_uid;
	kp->ki_fgid =		fp->f_cred->cr_gid;
	kp->ki_fops =		PTRTOUINT64(fp->f_ops);
	kp->ki_foffset =	fp->f_offset;
	kp->ki_fdata =		PTRTOUINT64(fp->f_data);

	/* vnode information to glue this file to something */
	if (fp->f_type == DTYPE_VNODE) {
		struct vnode *vp = (struct vnode *)fp->f_data;

		kp->ki_vun =	PTRTOUINT64(vp->v_un.vu_socket);
		kp->ki_vsize =	vp->v_size;
		kp->ki_vtype =	vp->v_type;
		kp->ki_vtag =	vp->v_tag;
		kp->ki_vdata =	PTRTOUINT64(vp->v_data);
	}

        /* process information when retrieved via KERN_FILE_BYPID */
	if (p) {
		kp->ki_pid =		p->p_pid;
		kp->ki_fd =		i;
		kp->ki_ofileflags =	p->p_fd->fd_ofileflags[i];
	}
}

static int
sysctl_doeproc(SYSCTLFN_ARGS)
{
	struct eproc eproc;
	struct kinfo_proc2 kproc2;
	struct kinfo_proc *dp;
	struct proc *p;
	const struct proclist_desc *pd;
	char *where, *dp2;
	int type, op, arg;
	u_int elem_size, elem_count;
	size_t buflen, needed;
	int error;

	if (namelen == 1 && name[0] == CTL_QUERY)
		return (sysctl_query(SYSCTLFN_CALL(rnode)));

	dp = oldp;
	dp2 = where = oldp;
	buflen = where != NULL ? *oldlenp : 0;
	error = 0;
	needed = 0;
	type = rnode->sysctl_num;

	if (type == KERN_PROC) {
		if (namelen != 2 && !(namelen == 1 && name[0] == KERN_PROC_ALL))
			return (EINVAL);
		op = name[0];
		if (op != KERN_PROC_ALL)
			arg = name[1];
		else
			arg = 0;		/* Quell compiler warning */
		elem_size = elem_count = 0;	/* Ditto */
	} else {
		if (namelen != 4)
			return (EINVAL);
		op = name[0];
		arg = name[1];
		elem_size = name[2];
		elem_count = name[3];
	}

	proclist_lock_read();

	pd = proclists;
again:
	PROCLIST_FOREACH(p, pd->pd_list) {
		/*
		 * Skip embryonic processes.
		 */
		if (p->p_stat == SIDL)
			continue;
		/*
		 * TODO - make more efficient (see notes below).
		 * do by session.
		 */
		switch (op) {

		case KERN_PROC_PID:
			/* could do this with just a lookup */
			if (p->p_pid != (pid_t)arg)
				continue;
			break;

		case KERN_PROC_PGRP:
			/* could do this by traversing pgrp */
			if (p->p_pgrp->pg_id != (pid_t)arg)
				continue;
			break;

		case KERN_PROC_SESSION:
			if (p->p_session->s_sid != (pid_t)arg)
				continue;
			break;

		case KERN_PROC_TTY:
			if (arg == (int) KERN_PROC_TTY_REVOKE) {
				if ((p->p_flag & P_CONTROLT) == 0 ||
				    p->p_session->s_ttyp == NULL ||
				    p->p_session->s_ttyvp != NULL)
					continue;
			} else if ((p->p_flag & P_CONTROLT) == 0 ||
			    p->p_session->s_ttyp == NULL) {
				if ((dev_t)arg != KERN_PROC_TTY_NODEV)
					continue;
			} else if (p->p_session->s_ttyp->t_dev != (dev_t)arg)
				continue;
			break;

		case KERN_PROC_UID:
			if (p->p_ucred->cr_uid != (uid_t)arg)
				continue;
			break;

		case KERN_PROC_RUID:
			if (p->p_cred->p_ruid != (uid_t)arg)
				continue;
			break;

		case KERN_PROC_GID:
			if (p->p_ucred->cr_gid != (uid_t)arg)
				continue;
			break;

		case KERN_PROC_RGID:
			if (p->p_cred->p_rgid != (uid_t)arg)
				continue;
			break;

		case KERN_PROC_ALL:
			/* allow everything */
			break;

		default:
			error = EINVAL;
			goto cleanup;
		}
		if (type == KERN_PROC) {
			if (buflen >= sizeof(struct kinfo_proc)) {
				fill_eproc(p, &eproc);
				error = copyout(p, &dp->kp_proc,
				    sizeof(struct proc));
				if (error)
					goto cleanup;
				error = copyout(&eproc, &dp->kp_eproc,
				    sizeof(eproc));
				if (error)
					goto cleanup;
				dp++;
				buflen -= sizeof(struct kinfo_proc);
			}
			needed += sizeof(struct kinfo_proc);
		} else { /* KERN_PROC2 */
			if (buflen >= elem_size && elem_count > 0) {
				fill_kproc2(p, &kproc2);
				/*
				 * Copy out elem_size, but not larger than
				 * the size of a struct kinfo_proc2.
				 */
				error = copyout(&kproc2, dp2,
				    min(sizeof(kproc2), elem_size));
				if (error)
					goto cleanup;
				dp2 += elem_size;
				buflen -= elem_size;
				elem_count--;
			}
			needed += elem_size;
		}
	}
	pd++;
	if (pd->pd_list != NULL)
		goto again;
	proclist_unlock_read();

	if (where != NULL) {
		if (type == KERN_PROC)
			*oldlenp = (char *)dp - where;
		else
			*oldlenp = dp2 - where;
		if (needed > *oldlenp)
			return (ENOMEM);
	} else {
		needed += KERN_PROCSLOP;
		*oldlenp = needed;
	}
	return (0);
 cleanup:
	proclist_unlock_read();
	return (error);
}

/*
 * sysctl helper routine for kern.proc_args pseudo-subtree.
 */
static int
sysctl_kern_proc_args(SYSCTLFN_ARGS)
{
	struct ps_strings pss;
	struct proc *p, *up = l->l_proc;
	size_t len, upper_bound, xlen, i;
	struct uio auio;
	struct iovec aiov;
	vaddr_t argv;
	pid_t pid;
	int nargv, type, error;
	char *arg;
	char *tmp;

	if (namelen == 1 && name[0] == CTL_QUERY)
		return (sysctl_query(SYSCTLFN_CALL(rnode)));

	if (newp != NULL || namelen != 2)
		return (EINVAL);
	pid = name[0];
	type = name[1];

	switch (type) {
	case KERN_PROC_ARGV:
	case KERN_PROC_NARGV:
	case KERN_PROC_ENV:
	case KERN_PROC_NENV:
		/* ok */
		break;
	default:
		return (EINVAL);
	}

	/* check pid */
	if ((p = pfind(pid)) == NULL)
		return (EINVAL);

	/* only root or same user change look at the environment */
	if (type == KERN_PROC_ENV || type == KERN_PROC_NENV) {
		if (up->p_ucred->cr_uid != 0) {
			if (up->p_cred->p_ruid != p->p_cred->p_ruid ||
			    up->p_cred->p_ruid != p->p_cred->p_svuid)
				return (EPERM);
		}
	}

	if (oldp == NULL) {
		if (type == KERN_PROC_NARGV || type == KERN_PROC_NENV)
			*oldlenp = sizeof (int);
		else
			*oldlenp = ARG_MAX;	/* XXX XXX XXX */
		return (0);
	}

	/*
	 * Zombies don't have a stack, so we can't read their psstrings.
	 * System processes also don't have a user stack.
	 */
	if (P_ZOMBIE(p) || (p->p_flag & P_SYSTEM) != 0)
		return (EINVAL);

	/*
	 * Lock the process down in memory.
	 */
	/* XXXCDC: how should locking work here? */
	if ((p->p_flag & P_WEXIT) || (p->p_vmspace->vm_refcnt < 1))
		return (EFAULT);

	p->p_vmspace->vm_refcnt++;	/* XXX */

	/*
	 * Allocate a temporary buffer to hold the arguments.
	 */
	arg = malloc(PAGE_SIZE, M_TEMP, M_WAITOK);

	/*
	 * Read in the ps_strings structure.
	 */
	aiov.iov_base = &pss;
	aiov.iov_len = sizeof(pss);
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = (vaddr_t)p->p_psstr;
	auio.uio_resid = sizeof(pss);
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_rw = UIO_READ;
	auio.uio_procp = NULL;
	error = uvm_io(&p->p_vmspace->vm_map, &auio);
	if (error)
		goto done;

	if (type == KERN_PROC_ARGV || type == KERN_PROC_NARGV)
		memcpy(&nargv, (char *)&pss + p->p_psnargv, sizeof(nargv));
	else
		memcpy(&nargv, (char *)&pss + p->p_psnenv, sizeof(nargv));
	if (type == KERN_PROC_NARGV || type == KERN_PROC_NENV) {
		error = copyout(&nargv, oldp, sizeof(nargv));
		*oldlenp = sizeof(nargv);
		goto done;
	}
	/*
	 * Now read the address of the argument vector.
	 */
	switch (type) {
	case KERN_PROC_ARGV:
		/* XXX compat32 stuff here */
		memcpy(&tmp, (char *)&pss + p->p_psargv, sizeof(tmp));
		break;
	case KERN_PROC_ENV:
		memcpy(&tmp, (char *)&pss + p->p_psenv, sizeof(tmp));
		break;
	default:
		return (EINVAL);
	}
	auio.uio_offset = (off_t)(unsigned long)tmp;
	aiov.iov_base = &argv;
	aiov.iov_len = sizeof(argv);
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_resid = sizeof(argv);
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_rw = UIO_READ;
	auio.uio_procp = NULL;
	error = uvm_io(&p->p_vmspace->vm_map, &auio);
	if (error)
		goto done;

	/*
	 * Now copy in the actual argument vector, one page at a time,
	 * since we don't know how long the vector is (though, we do
	 * know how many NUL-terminated strings are in the vector).
	 */
	len = 0;
	upper_bound = *oldlenp;
	for (; nargv != 0 && len < upper_bound; len += xlen) {
		aiov.iov_base = arg;
		aiov.iov_len = PAGE_SIZE;
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_offset = argv + len;
		xlen = PAGE_SIZE - ((argv + len) & PAGE_MASK);
		auio.uio_resid = xlen;
		auio.uio_segflg = UIO_SYSSPACE;
		auio.uio_rw = UIO_READ;
		auio.uio_procp = NULL;
		error = uvm_io(&p->p_vmspace->vm_map, &auio);
		if (error)
			goto done;

		for (i = 0; i < xlen && nargv != 0; i++) {
			if (arg[i] == '\0')
				nargv--;	/* one full string */
		}

		/*
		 * Make sure we don't copyout past the end of the user's
		 * buffer.
		 */
		if (len + i > upper_bound)
			i = upper_bound - len;

		error = copyout(arg, (char *)oldp + len, i);
		if (error)
			break;

		if (nargv == 0) {
			len += i;
			break;
		}
	}
	*oldlenp = len;

done:
	uvmspace_free(p->p_vmspace);

	free(arg, M_TEMP);
	return (error);
}

/*
 * Sysctl helper routine for Verified Exec.
 */
#ifdef VERIFIED_EXEC
static int
sysctl_kern_veriexec(SYSCTLFN_ARGS)
{
	int newval, error;
	int *var = NULL, raise_only = 0;
	struct sysctlnode node;

	node = *rnode;

	switch (rnode->sysctl_num) {
	case VERIEXEC_STRICT:
		raise_only = 1;
		var = &veriexec_strict;
		break;
	case VERIEXEC_ALGORITHMS:
		node.sysctl_data = veriexec_fp_names;
		node.sysctl_size = strlen(veriexec_fp_names) + 1;
		return (sysctl_lookup(SYSCTLFN_CALL(&node)));
	default:
		return (EINVAL);
	}

	newval = *var;

	node.sysctl_data = &newval;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL) {
		return (error);
	}

	if (raise_only && (newval < *var))
		return (EPERM);

	*var = newval;

	return (error);
}
#endif /* VERIFIED_EXEC */

/*
 * sysctl helper routine for kern.cp_id node.  maps cpus to their
 * cpuids.
 */
static int
sysctl_kern_cpid(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;

#ifndef MULTIPROCESSOR
	u_int64_t id;

	if (namelen == 1) {
		if (name[0] != 0)
			return (ENOENT);
		/*
		 * you're allowed to ask for the zero'th processor
		 */
		name++;
		namelen--;
	}
	node.sysctl_data = &id;
	node.sysctl_size = sizeof(id);
	id = cpu_number();
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));

#else /* MULTIPROCESSOR */
	u_int64_t *cp_id = NULL;
	int error, n = sysctl_ncpus();
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;

	/*
	 * here you may either retrieve a single cpu id or the whole
	 * set.  the size you get back when probing depends on what
	 * you ask for.
	 */
	switch (namelen) {
	case 0:
		node.sysctl_size = n * sizeof(u_int64_t);
		n = -2; /* ALL */
		break;
	case 1:
		if (name[0] < 0 || name[0] >= n)
			return (ENOENT); /* ENOSUCHPROCESSOR */
		node.sysctl_size = sizeof(u_int64_t);
		n = name[0];
		/*
		 * adjust these so that sysctl_lookup() will be happy
		 */
		name++;
		namelen--;
		break;
	default:
		return (EINVAL);
	}

	cp_id = malloc(node.sysctl_size, M_TEMP, M_WAITOK|M_CANFAIL);
	if (cp_id == NULL)
		return (ENOMEM);
	node.sysctl_data = cp_id;
	memset(cp_id, 0, node.sysctl_size);

	for (CPU_INFO_FOREACH(cii, ci)) {
		if (n <= 0)
			cp_id[0] = ci->ci_cpuid;
		/*
		 * if a specific processor was requested and we just
		 * did it, we're done here
		 */
		if (n == 0)
			break;
		/*
		 * if doing "all", skip to next cp_id slot for next processor
		 */
		if (n == -2)
			cp_id++;
		/*
		 * if we're doing a specific processor, we're one
		 * processor closer
		 */
		if (n > 0)
			n--;
	}

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	free(node.sysctl_data, M_TEMP);
	return (error);

#endif /* MULTIPROCESSOR */
}

/*
 * sysctl helper routine for hw.usermem and hw.usermem64.  values are
 * calculate on the fly taking into account integer overflow and the
 * current wired count.
 */
static int
sysctl_hw_usermem(SYSCTLFN_ARGS)
{
	u_int ui;
	u_quad_t uq;
	struct sysctlnode node;

	node = *rnode;
	switch (rnode->sysctl_num) {
	    case HW_USERMEM:
		if ((ui = physmem - uvmexp.wired) > (UINT_MAX / PAGE_SIZE))
			ui = UINT_MAX;
		else
			ui *= PAGE_SIZE;
		node.sysctl_data = &ui;
		break;
	case HW_USERMEM64:
		uq = (u_quad_t)(physmem - uvmexp.wired) * PAGE_SIZE;
		node.sysctl_data = &uq;
		break;
	default:
		return (EINVAL);
	}

	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

/*
 * sysctl helper routine for kern.cnmagic node.  pulls the old value
 * out, encoded, and stuffs the new value in for decoding.
 */
static int
sysctl_hw_cnmagic(SYSCTLFN_ARGS)
{
	char magic[CNS_LEN];
	int error;
	struct sysctlnode node;

	if (oldp)
		cn_get_magic(magic, CNS_LEN);
	node = *rnode;
	node.sysctl_data = &magic[0];
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	return (cn_set_magic(magic));
}

static int
sysctl_hw_ncpu(SYSCTLFN_ARGS)
{
	int ncpu;
	struct sysctlnode node;

	ncpu = sysctl_ncpus();
	node = *rnode;
	node.sysctl_data = &ncpu;

	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}


/*
 * ********************************************************************
 * section 3: public helper routines that are used for more than one
 * node
 * ********************************************************************
 */

/*
 * sysctl helper routine for the kern.root_device node and some ports'
 * machdep.root_device nodes.
 */
int
sysctl_root_device(SYSCTLFN_ARGS)
{
	struct sysctlnode node;

	node = *rnode;
	node.sysctl_data = root_device->dv_xname;
	node.sysctl_size = strlen(root_device->dv_xname) + 1;
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

/*
 * sysctl helper routine for kern.consdev, dependent on the current
 * state of the console.  also used for machdep.console_device on some
 * ports.
 */
int
sysctl_consdev(SYSCTLFN_ARGS)
{
	dev_t consdev;
	struct sysctlnode node;

	if (cn_tab != NULL)
		consdev = cn_tab->cn_dev;
	else
		consdev = NODEV;
	node = *rnode;
	node.sysctl_data = &consdev;
	node.sysctl_size = sizeof(consdev);
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

/*
 * ********************************************************************
 * section 4: support for some helpers
 * ********************************************************************
 */

/*
 * Fill in a kinfo_proc2 structure for the specified process.
 */
static void
fill_kproc2(struct proc *p, struct kinfo_proc2 *ki)
{
	struct tty *tp;
	struct lwp *l;
	struct timeval ut, st;

	memset(ki, 0, sizeof(*ki));

	ki->p_paddr = PTRTOUINT64(p);
	ki->p_fd = PTRTOUINT64(p->p_fd);
	ki->p_cwdi = PTRTOUINT64(p->p_cwdi);
	ki->p_stats = PTRTOUINT64(p->p_stats);
	ki->p_limit = PTRTOUINT64(p->p_limit);
	ki->p_vmspace = PTRTOUINT64(p->p_vmspace);
	ki->p_sigacts = PTRTOUINT64(p->p_sigacts);
	ki->p_sess = PTRTOUINT64(p->p_session);
	ki->p_tsess = 0;	/* may be changed if controlling tty below */
	ki->p_ru = PTRTOUINT64(p->p_ru);

	ki->p_eflag = 0;
	ki->p_exitsig = p->p_exitsig;
	ki->p_flag = p->p_flag;

	ki->p_pid = p->p_pid;
	if (p->p_pptr)
		ki->p_ppid = p->p_pptr->p_pid;
	else
		ki->p_ppid = 0;
	ki->p_sid = p->p_session->s_sid;
	ki->p__pgid = p->p_pgrp->pg_id;

	ki->p_tpgid = NO_PGID;	/* may be changed if controlling tty below */

	ki->p_uid = p->p_ucred->cr_uid;
	ki->p_ruid = p->p_cred->p_ruid;
	ki->p_gid = p->p_ucred->cr_gid;
	ki->p_rgid = p->p_cred->p_rgid;
	ki->p_svuid = p->p_cred->p_svuid;
	ki->p_svgid = p->p_cred->p_svgid;

	memcpy(ki->p_groups, p->p_cred->pc_ucred->cr_groups,
	    min(sizeof(ki->p_groups), sizeof(p->p_cred->pc_ucred->cr_groups)));
	ki->p_ngroups = p->p_cred->pc_ucred->cr_ngroups;

	ki->p_jobc = p->p_pgrp->pg_jobc;
	if ((p->p_flag & P_CONTROLT) && (tp = p->p_session->s_ttyp)) {
		ki->p_tdev = tp->t_dev;
		ki->p_tpgid = tp->t_pgrp ? tp->t_pgrp->pg_id : NO_PGID;
		ki->p_tsess = PTRTOUINT64(tp->t_session);
	} else {
		ki->p_tdev = NODEV;
	}

	ki->p_estcpu = p->p_estcpu;
	ki->p_rtime_sec = p->p_rtime.tv_sec;
	ki->p_rtime_usec = p->p_rtime.tv_usec;
	ki->p_cpticks = p->p_cpticks;
	ki->p_pctcpu = p->p_pctcpu;

	ki->p_uticks = p->p_uticks;
	ki->p_sticks = p->p_sticks;
	ki->p_iticks = p->p_iticks;

	ki->p_tracep = PTRTOUINT64(p->p_tracep);
	ki->p_traceflag = p->p_traceflag;


	memcpy(&ki->p_siglist, &p->p_sigctx.ps_siglist, sizeof(ki_sigset_t));
	memcpy(&ki->p_sigmask, &p->p_sigctx.ps_sigmask, sizeof(ki_sigset_t));
	memcpy(&ki->p_sigignore, &p->p_sigctx.ps_sigignore,sizeof(ki_sigset_t));
	memcpy(&ki->p_sigcatch, &p->p_sigctx.ps_sigcatch, sizeof(ki_sigset_t));

	ki->p_stat = p->p_stat; /* Will likely be overridden by LWP status */
	ki->p_realstat = p->p_stat;
	ki->p_nice = p->p_nice;

	ki->p_xstat = p->p_xstat;
	ki->p_acflag = p->p_acflag;

	strncpy(ki->p_comm, p->p_comm,
	    min(sizeof(ki->p_comm), sizeof(p->p_comm)));

	strncpy(ki->p_login, p->p_session->s_login,
	    min(sizeof ki->p_login - 1, sizeof p->p_session->s_login));

	ki->p_nlwps = p->p_nlwps;
	ki->p_nrlwps = p->p_nrlwps;
	ki->p_realflag = p->p_flag;

	if (p->p_stat == SIDL || P_ZOMBIE(p)) {
		ki->p_vm_rssize = 0;
		ki->p_vm_tsize = 0;
		ki->p_vm_dsize = 0;
		ki->p_vm_ssize = 0;
		l = NULL;
	} else {
		struct vmspace *vm = p->p_vmspace;

		ki->p_vm_rssize = vm_resident_count(vm);
		ki->p_vm_tsize = vm->vm_tsize;
		ki->p_vm_dsize = vm->vm_dsize;
		ki->p_vm_ssize = vm->vm_ssize;

		/* Pick a "representative" LWP */
		l = proc_representative_lwp(p);
		ki->p_forw = PTRTOUINT64(l->l_forw);
		ki->p_back = PTRTOUINT64(l->l_back);
		ki->p_addr = PTRTOUINT64(l->l_addr);
		ki->p_stat = l->l_stat;
		ki->p_flag |= l->l_flag;
		ki->p_swtime = l->l_swtime;
		ki->p_slptime = l->l_slptime;
		if (l->l_stat == LSONPROC) {
			KDASSERT(l->l_cpu != NULL);
			ki->p_schedflags = l->l_cpu->ci_schedstate.spc_flags;
		} else
			ki->p_schedflags = 0;
		ki->p_holdcnt = l->l_holdcnt;
		ki->p_priority = l->l_priority;
		ki->p_usrpri = l->l_usrpri;
		if (l->l_wmesg)
			strncpy(ki->p_wmesg, l->l_wmesg, sizeof(ki->p_wmesg));
		ki->p_wchan = PTRTOUINT64(l->l_wchan);

	}

	if (p->p_session->s_ttyvp)
		ki->p_eflag |= EPROC_CTTY;
	if (SESS_LEADER(p))
		ki->p_eflag |= EPROC_SLEADER;

	/* XXX Is this double check necessary? */
	if (P_ZOMBIE(p)) {
		ki->p_uvalid = 0;
	} else {
		ki->p_uvalid = 1;

		ki->p_ustart_sec = p->p_stats->p_start.tv_sec;
		ki->p_ustart_usec = p->p_stats->p_start.tv_usec;

		calcru(p, &ut, &st, 0);
		ki->p_uutime_sec = ut.tv_sec;
		ki->p_uutime_usec = ut.tv_usec;
		ki->p_ustime_sec = st.tv_sec;
		ki->p_ustime_usec = st.tv_usec;

		ki->p_uru_maxrss = p->p_stats->p_ru.ru_maxrss;
		ki->p_uru_ixrss = p->p_stats->p_ru.ru_ixrss;
		ki->p_uru_idrss = p->p_stats->p_ru.ru_idrss;
		ki->p_uru_isrss = p->p_stats->p_ru.ru_isrss;
		ki->p_uru_minflt = p->p_stats->p_ru.ru_minflt;
		ki->p_uru_majflt = p->p_stats->p_ru.ru_majflt;
		ki->p_uru_nswap = p->p_stats->p_ru.ru_nswap;
		ki->p_uru_inblock = p->p_stats->p_ru.ru_inblock;
		ki->p_uru_oublock = p->p_stats->p_ru.ru_oublock;
		ki->p_uru_msgsnd = p->p_stats->p_ru.ru_msgsnd;
		ki->p_uru_msgrcv = p->p_stats->p_ru.ru_msgrcv;
		ki->p_uru_nsignals = p->p_stats->p_ru.ru_nsignals;
		ki->p_uru_nvcsw = p->p_stats->p_ru.ru_nvcsw;
		ki->p_uru_nivcsw = p->p_stats->p_ru.ru_nivcsw;

		timeradd(&p->p_stats->p_cru.ru_utime,
			 &p->p_stats->p_cru.ru_stime, &ut);
		ki->p_uctime_sec = ut.tv_sec;
		ki->p_uctime_usec = ut.tv_usec;
	}
#ifdef MULTIPROCESSOR
	if (l && l->l_cpu != NULL)
		ki->p_cpuid = l->l_cpu->ci_cpuid;
	else
#endif
		ki->p_cpuid = KI_NOCPU;
}

/*
 * Fill in a kinfo_lwp structure for the specified lwp.
 */
static void
fill_lwp(struct lwp *l, struct kinfo_lwp *kl)
{

	kl->l_forw = PTRTOUINT64(l->l_forw);
	kl->l_back = PTRTOUINT64(l->l_back);
	kl->l_laddr = PTRTOUINT64(l);
	kl->l_addr = PTRTOUINT64(l->l_addr);
	kl->l_stat = l->l_stat;
	kl->l_lid = l->l_lid;
	kl->l_flag = l->l_flag;

	kl->l_swtime = l->l_swtime;
	kl->l_slptime = l->l_slptime;
	if (l->l_stat == LSONPROC) {
		KDASSERT(l->l_cpu != NULL);
		kl->l_schedflags = l->l_cpu->ci_schedstate.spc_flags;
	} else
		kl->l_schedflags = 0;
	kl->l_holdcnt = l->l_holdcnt;
	kl->l_priority = l->l_priority;
	kl->l_usrpri = l->l_usrpri;
	if (l->l_wmesg)
		strncpy(kl->l_wmesg, l->l_wmesg, sizeof(kl->l_wmesg));
	kl->l_wchan = PTRTOUINT64(l->l_wchan);
#ifdef MULTIPROCESSOR
	if (l->l_cpu != NULL)
		kl->l_cpuid = l->l_cpu->ci_cpuid;
	else
#endif
		kl->l_cpuid = KI_NOCPU;
}

/*
 * Fill in an eproc structure for the specified process.
 */
void
fill_eproc(struct proc *p, struct eproc *ep)
{
	struct tty *tp;
	struct lwp *l;

	ep->e_paddr = p;
	ep->e_sess = p->p_session;
	ep->e_pcred = *p->p_cred;
	ep->e_ucred = *p->p_ucred;
	if (p->p_stat == SIDL || P_ZOMBIE(p)) {
		ep->e_vm.vm_rssize = 0;
		ep->e_vm.vm_tsize = 0;
		ep->e_vm.vm_dsize = 0;
		ep->e_vm.vm_ssize = 0;
		/* ep->e_vm.vm_pmap = XXX; */
	} else {
		struct vmspace *vm = p->p_vmspace;

		ep->e_vm.vm_rssize = vm_resident_count(vm);
		ep->e_vm.vm_tsize = vm->vm_tsize;
		ep->e_vm.vm_dsize = vm->vm_dsize;
		ep->e_vm.vm_ssize = vm->vm_ssize;

		/* Pick a "representative" LWP */
		l = proc_representative_lwp(p);

		if (l->l_wmesg)
			strncpy(ep->e_wmesg, l->l_wmesg, WMESGLEN);
	}
	if (p->p_pptr)
		ep->e_ppid = p->p_pptr->p_pid;
	else
		ep->e_ppid = 0;
	ep->e_pgid = p->p_pgrp->pg_id;
	ep->e_sid = ep->e_sess->s_sid;
	ep->e_jobc = p->p_pgrp->pg_jobc;
	if ((p->p_flag & P_CONTROLT) &&
	    (tp = ep->e_sess->s_ttyp)) {
		ep->e_tdev = tp->t_dev;
		ep->e_tpgid = tp->t_pgrp ? tp->t_pgrp->pg_id : NO_PGID;
		ep->e_tsess = tp->t_session;
	} else
		ep->e_tdev = NODEV;

	ep->e_xsize = ep->e_xrssize = 0;
	ep->e_xccount = ep->e_xswrss = 0;
	ep->e_flag = ep->e_sess->s_ttyvp ? EPROC_CTTY : 0;
	if (SESS_LEADER(p))
		ep->e_flag |= EPROC_SLEADER;
	strncpy(ep->e_login, ep->e_sess->s_login, MAXLOGNAME);
}
