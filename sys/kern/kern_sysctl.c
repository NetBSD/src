/*	$NetBSD: kern_sysctl.c,v 1.95 2001/09/24 06:01:13 chs Exp $	*/

/*-
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Karels at Berkeley Software Design, Inc.
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
 *	@(#)kern_sysctl.c	8.9 (Berkeley) 5/20/95
 */

/*
 * sysctl system call.
 */

#include "opt_ddb.h"
#include "opt_insecure.h"
#include "opt_defcorename.h"
#include "opt_new_pipe.h"
#include "opt_sysv.h"
#include "pty.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/dkstat.h>
#include <sys/exec.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/syscallargs.h>
#include <sys/tty.h>
#include <sys/unistd.h>
#include <sys/vnode.h>
#include <sys/socketvar.h>
#define	__SYSCTL_PRIVATE
#include <sys/sysctl.h>
#include <sys/lock.h>
#include <sys/namei.h>

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

#include <dev/cons.h>

#if defined(DDB)
#include <ddb/ddbvar.h>
#endif

#ifdef NEW_PIPE
#include <sys/pipe.h>
#endif

#define PTRTOINT64(foo)	((u_int64_t)(uintptr_t)(foo))

static int sysctl_file(void *, size_t *);
#if defined(SYSVMSG) || defined(SYSVSEM) || defined(SYSVSHM)
static int sysctl_sysvipc(int *, u_int, void *, size_t *);
#endif
static int sysctl_msgbuf(void *, size_t *);
static int sysctl_doeproc(int *, u_int, void *, size_t *);
#ifdef MULTIPROCESSOR
static int sysctl_docptime(void *, size_t *, void *);
static int sysctl_ncpus(void);
#endif
static void fill_kproc2(struct proc *, struct kinfo_proc2 *);
static int sysctl_procargs(int *, u_int, void *, size_t *, struct proc *);
#if NPTY > 0
static int sysctl_pty(void *, size_t *, void *, size_t);
#endif

/*
 * The `sysctl_memlock' is intended to keep too many processes from
 * locking down memory by doing sysctls at once.  Whether or not this
 * is really a good idea to worry about it probably a subject of some
 * debate.
 */
struct lock sysctl_memlock;

void
sysctl_init(void)
{

	lockinit(&sysctl_memlock, PRIBIO|PCATCH, "sysctl", 0, 0);
}

int
sys___sysctl(struct proc *p, void *v, register_t *retval)
{
	struct sys___sysctl_args /* {
		syscallarg(int *) name;
		syscallarg(u_int) namelen;
		syscallarg(void *) old;
		syscallarg(size_t *) oldlenp;
		syscallarg(void *) new;
		syscallarg(size_t) newlen;
	} */ *uap = v;
	int error;
	size_t savelen = 0, oldlen = 0;
	sysctlfn *fn;
	int name[CTL_MAXNAME];
	size_t *oldlenp;

	/*
	 * all top-level sysctl names are non-terminal
	 */
	if (SCARG(uap, namelen) > CTL_MAXNAME || SCARG(uap, namelen) < 2)
		return (EINVAL);
	error = copyin(SCARG(uap, name), &name,
		       SCARG(uap, namelen) * sizeof(int));
	if (error)
		return (error);

	/*
	 * For all but CTL_PROC, must be root to change a value.
	 * For CTL_PROC, must be root, or owner of the proc (and not suid),
	 * this is checked in proc_sysctl() (once we know the targer proc).
	 */
	if (SCARG(uap, new) != NULL && name[0] != CTL_PROC &&
		    (error = suser(p->p_ucred, &p->p_acflag)))
			return error;

	switch (name[0]) {
	case CTL_KERN:
		fn = kern_sysctl;
		break;
	case CTL_HW:
		fn = hw_sysctl;
		break;
	case CTL_VM:
		fn = uvm_sysctl;
		break;
	case CTL_NET:
		fn = net_sysctl;
		break;
	case CTL_VFS:
		fn = vfs_sysctl;
		break;
	case CTL_MACHDEP:
		fn = cpu_sysctl;
		break;
#ifdef DEBUG
	case CTL_DEBUG:
		fn = debug_sysctl;
		break;
#endif
#ifdef DDB
	case CTL_DDB:
		fn = ddb_sysctl;
		break;
#endif
	case CTL_PROC:
		fn = proc_sysctl;
		break;
	default:
		return (EOPNOTSUPP);
	}

	/*
	 * XXX Hey, we wire `old', but what about `new'?
	 */

	oldlenp = SCARG(uap, oldlenp);
	if (oldlenp) {
		if ((error = copyin(oldlenp, &oldlen, sizeof(oldlen))))
			return (error);
		oldlenp = &oldlen;
	}
	if (SCARG(uap, old) != NULL) {
		error = lockmgr(&sysctl_memlock, LK_EXCLUSIVE, NULL);
		if (error)
			return (error);
		error = uvm_vslock(p, SCARG(uap, old), oldlen,
		    VM_PROT_READ|VM_PROT_WRITE);
		if (error) {
			(void) lockmgr(&sysctl_memlock, LK_RELEASE, NULL);
			return error;
		}
		savelen = oldlen;
	}
	error = (*fn)(name + 1, SCARG(uap, namelen) - 1, SCARG(uap, old),
	    oldlenp, SCARG(uap, new), SCARG(uap, newlen), p);
	if (SCARG(uap, old) != NULL) {
		uvm_vsunlock(p, SCARG(uap, old), savelen);
		(void) lockmgr(&sysctl_memlock, LK_RELEASE, NULL);
	}
	if (error)
		return (error);
	if (SCARG(uap, oldlenp))
		error = copyout(&oldlen, SCARG(uap, oldlenp), sizeof(oldlen));
	return (error);
}

/*
 * Attributes stored in the kernel.
 */
char hostname[MAXHOSTNAMELEN];
int hostnamelen;

char domainname[MAXHOSTNAMELEN];
int domainnamelen;

long hostid;

#ifdef INSECURE
int securelevel = -1;
#else
int securelevel = 0;
#endif

#ifndef DEFCORENAME
#define	DEFCORENAME	"%n.core"
#endif
char defcorename[MAXPATHLEN] = DEFCORENAME;
int defcorenamelen = sizeof(DEFCORENAME);

extern	int	kern_logsigexit;
extern	fixpt_t	ccpu;

#ifndef MULTIPROCESSOR
#define sysctl_ncpus() 1
#endif

#ifdef MULTIPROCESSOR

#ifndef CPU_INFO_FOREACH
#define CPU_INFO_ITERATOR int
#define CPU_INFO_FOREACH(cii, ci) cii = 0, ci = curcpu(); ci != NULL; ci = NULL
#endif

static int
sysctl_docptime(void *oldp, size_t *oldlenp, void *newp)
{
	u_int64_t cp_time[CPUSTATES];
	int i;
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;

	for (i=0; i<CPUSTATES; i++)
		cp_time[i] = 0;

	for (CPU_INFO_FOREACH(cii, ci)) {
		for (i=0; i<CPUSTATES; i++)
			cp_time[i] += ci->ci_schedstate.spc_cp_time[i];
	}
	return (sysctl_rdstruct(oldp, oldlenp, newp,
	    cp_time, sizeof(cp_time)));
}

static int
sysctl_ncpus(void)
{
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;

	int ncpus = 0;
	for (CPU_INFO_FOREACH(cii, ci))
		ncpus++;
	return ncpus;
}

#endif

/*
 * kernel related system variables.
 */
int
kern_sysctl(int *name, u_int namelen, void *oldp, size_t *oldlenp,
    void *newp, size_t newlen, struct proc *p)
{
	int error, level, inthostid;
	int old_autonicetime;
	int old_vnodes;
	dev_t consdev;

	/* All sysctl names at this level, except for a few, are terminal. */
	switch (name[0]) {
	case KERN_PROC:
	case KERN_PROC2:
	case KERN_PROF:
	case KERN_MBUF:
	case KERN_PROC_ARGS:
	case KERN_SYSVIPC_INFO:
	case KERN_PIPE:
		/* Not terminal. */
		break;
	default:
		if (namelen != 1)
			return (ENOTDIR);	/* overloaded */
	}

	switch (name[0]) {
	case KERN_OSTYPE:
		return (sysctl_rdstring(oldp, oldlenp, newp, ostype));
	case KERN_OSRELEASE:
		return (sysctl_rdstring(oldp, oldlenp, newp, osrelease));
	case KERN_OSREV:
		return (sysctl_rdint(oldp, oldlenp, newp, __NetBSD_Version__));
	case KERN_VERSION:
		return (sysctl_rdstring(oldp, oldlenp, newp, version));
	case KERN_MAXVNODES:
		old_vnodes = desiredvnodes;
		error = sysctl_int(oldp, oldlenp, newp, newlen, &desiredvnodes);
		if (old_vnodes > desiredvnodes) {
		        desiredvnodes = old_vnodes;
			return (EINVAL);
		}
		if (error == 0) {
			vfs_reinit();
			nchreinit();
		}
		return (error);
	case KERN_MAXPROC:
		return (sysctl_int(oldp, oldlenp, newp, newlen, &maxproc));
	case KERN_MAXFILES:
		return (sysctl_int(oldp, oldlenp, newp, newlen, &maxfiles));
	case KERN_ARGMAX:
		return (sysctl_rdint(oldp, oldlenp, newp, ARG_MAX));
	case KERN_SECURELVL:
		level = securelevel;
		if ((error = sysctl_int(oldp, oldlenp, newp, newlen, &level)) ||
		    newp == NULL)
			return (error);
		if (level < securelevel && p->p_pid != 1)
			return (EPERM);
		securelevel = level;
		return (0);
	case KERN_HOSTNAME:
		error = sysctl_string(oldp, oldlenp, newp, newlen,
		    hostname, sizeof(hostname));
		if (newp && !error)
			hostnamelen = newlen;
		return (error);
	case KERN_DOMAINNAME:
		error = sysctl_string(oldp, oldlenp, newp, newlen,
		    domainname, sizeof(domainname));
		if (newp && !error)
			domainnamelen = newlen;
		return (error);
	case KERN_HOSTID:
		inthostid = hostid;  /* XXX assumes sizeof long <= sizeof int */
		error =  sysctl_int(oldp, oldlenp, newp, newlen, &inthostid);
		hostid = inthostid;
		return (error);
	case KERN_CLOCKRATE:
		return (sysctl_clockrate(oldp, oldlenp));
	case KERN_BOOTTIME:
		return (sysctl_rdstruct(oldp, oldlenp, newp, &boottime,
		    sizeof(struct timeval)));
	case KERN_VNODE:
		return (sysctl_vnode(oldp, oldlenp, p));
	case KERN_PROC:
	case KERN_PROC2:
		return (sysctl_doeproc(name, namelen, oldp, oldlenp));
	case KERN_PROC_ARGS:
		return (sysctl_procargs(name + 1, namelen - 1,
		    oldp, oldlenp, p));
	case KERN_FILE:
		return (sysctl_file(oldp, oldlenp));
#ifdef GPROF
	case KERN_PROF:
		return (sysctl_doprof(name + 1, namelen - 1, oldp, oldlenp,
		    newp, newlen));
#endif
	case KERN_POSIX1:
		return (sysctl_rdint(oldp, oldlenp, newp, _POSIX_VERSION));
	case KERN_NGROUPS:
		return (sysctl_rdint(oldp, oldlenp, newp, NGROUPS_MAX));
	case KERN_JOB_CONTROL:
		return (sysctl_rdint(oldp, oldlenp, newp, 1));
	case KERN_SAVED_IDS:
#ifdef _POSIX_SAVED_IDS
		return (sysctl_rdint(oldp, oldlenp, newp, 1));
#else
		return (sysctl_rdint(oldp, oldlenp, newp, 0));
#endif
	case KERN_MAXPARTITIONS:
		return (sysctl_rdint(oldp, oldlenp, newp, MAXPARTITIONS));
	case KERN_RAWPARTITION:
		return (sysctl_rdint(oldp, oldlenp, newp, RAW_PART));
#ifdef NTP
	case KERN_NTPTIME:
		return (sysctl_ntptime(oldp, oldlenp));
#endif
	case KERN_AUTONICETIME:
	        old_autonicetime = autonicetime;
	        error = sysctl_int(oldp, oldlenp, newp, newlen, &autonicetime);
		if (autonicetime < 0)
 		        autonicetime = old_autonicetime;
		return (error);
	case KERN_AUTONICEVAL:
		error = sysctl_int(oldp, oldlenp, newp, newlen, &autoniceval);
		if (autoniceval < PRIO_MIN)
			autoniceval = PRIO_MIN;
		if (autoniceval > PRIO_MAX)
			autoniceval = PRIO_MAX;
		return (error);
	case KERN_RTC_OFFSET:
		return (sysctl_rdint(oldp, oldlenp, newp, rtc_offset));
	case KERN_ROOT_DEVICE:
		return (sysctl_rdstring(oldp, oldlenp, newp,
		    root_device->dv_xname));
	case KERN_MSGBUFSIZE:
		/*
		 * deal with cases where the message buffer has
		 * become corrupted.
		 */
		if (!msgbufenabled || msgbufp->msg_magic != MSG_MAGIC) {
			msgbufenabled = 0;
			return (ENXIO);
		}
		return (sysctl_rdint(oldp, oldlenp, newp, msgbufp->msg_bufs));
	case KERN_FSYNC:
		return (sysctl_rdint(oldp, oldlenp, newp, 1));
	case KERN_SYSVMSG:
#ifdef SYSVMSG
		return (sysctl_rdint(oldp, oldlenp, newp, 1));
#else
		return (sysctl_rdint(oldp, oldlenp, newp, 0));
#endif
	case KERN_SYSVSEM:
#ifdef SYSVSEM
		return (sysctl_rdint(oldp, oldlenp, newp, 1));
#else
		return (sysctl_rdint(oldp, oldlenp, newp, 0));
#endif
	case KERN_SYSVSHM:
#ifdef SYSVSHM
		return (sysctl_rdint(oldp, oldlenp, newp, 1));
#else
		return (sysctl_rdint(oldp, oldlenp, newp, 0));
#endif
 	case KERN_DEFCORENAME:
		if (newp && newlen < 1)
			return (EINVAL);
		error = sysctl_string(oldp, oldlenp, newp, newlen,
		    defcorename, sizeof(defcorename));
		if (newp && !error)
			defcorenamelen = newlen;
		return (error);
	case KERN_SYNCHRONIZED_IO:
		return (sysctl_rdint(oldp, oldlenp, newp, 1));
	case KERN_IOV_MAX:
		return (sysctl_rdint(oldp, oldlenp, newp, IOV_MAX));
	case KERN_MBUF:
		return (sysctl_dombuf(name + 1, namelen - 1, oldp, oldlenp,
		    newp, newlen));
	case KERN_MAPPED_FILES:
		return (sysctl_rdint(oldp, oldlenp, newp, 1));
	case KERN_MEMLOCK:
		return (sysctl_rdint(oldp, oldlenp, newp, 1));
	case KERN_MEMLOCK_RANGE:
		return (sysctl_rdint(oldp, oldlenp, newp, 1));
	case KERN_MEMORY_PROTECTION:
		return (sysctl_rdint(oldp, oldlenp, newp, 1));
	case KERN_LOGIN_NAME_MAX:
		return (sysctl_rdint(oldp, oldlenp, newp, LOGIN_NAME_MAX));
	case KERN_LOGSIGEXIT:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
		    &kern_logsigexit));
	case KERN_FSCALE:
		return (sysctl_rdint(oldp, oldlenp, newp, FSCALE));
	case KERN_CCPU:
		return (sysctl_rdint(oldp, oldlenp, newp, ccpu));
	case KERN_CP_TIME:
#ifndef MULTIPROCESSOR
		return (sysctl_rdstruct(oldp, oldlenp, newp,
		    curcpu()->ci_schedstate.spc_cp_time,
		    sizeof(curcpu()->ci_schedstate.spc_cp_time)));
#else
		return (sysctl_docptime(oldp, oldlenp, newp));
#endif
#if defined(SYSVMSG) || defined(SYSVSEM) || defined(SYSVSHM)
	case KERN_SYSVIPC_INFO:
		return (sysctl_sysvipc(name + 1, namelen - 1, oldp, oldlenp));
#endif
	case KERN_MSGBUF:
		return (sysctl_msgbuf(oldp, oldlenp));
	case KERN_CONSDEV:
		if (cn_tab != NULL)
			consdev = cn_tab->cn_dev;
		else
			consdev = NODEV;
		return (sysctl_rdstruct(oldp, oldlenp, newp, &consdev,
		    sizeof consdev));
#if NPTY > 0
	case KERN_MAXPTYS:
		return sysctl_pty(oldp, oldlenp, newp, newlen);
#endif
#ifdef NEW_PIPE
	case KERN_PIPE:
		return (sysctl_dopipe(name + 1, namelen - 1, oldp, oldlenp,
		    newp, newlen));
#endif
	case KERN_MAXPHYS:
		return (sysctl_rdint(oldp, oldlenp, newp, MAXPHYS));
	case KERN_SBMAX:
	    {
		int new_sbmax = sb_max;

		error = sysctl_int(oldp, oldlenp, newp, newlen, &new_sbmax);
		if (error == 0) {
			if (new_sbmax < (16 * 1024)) /* sanity */
				return (EINVAL);
			sb_max = new_sbmax;
		}
		return (error);
	    }
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

/*
 * hardware related system variables.
 */
int
hw_sysctl(int *name, u_int namelen, void *oldp, size_t *oldlenp,
    void *newp, size_t newlen, struct proc *p)
{

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case HW_MACHINE:
		return (sysctl_rdstring(oldp, oldlenp, newp, machine));
	case HW_MACHINE_ARCH:
		return (sysctl_rdstring(oldp, oldlenp, newp, machine_arch));
	case HW_MODEL:
		return (sysctl_rdstring(oldp, oldlenp, newp, cpu_model));
	case HW_NCPU:
		return (sysctl_rdint(oldp, oldlenp, newp, sysctl_ncpus()));
	case HW_BYTEORDER:
		return (sysctl_rdint(oldp, oldlenp, newp, BYTE_ORDER));
	case HW_PHYSMEM:
		return (sysctl_rdint(oldp, oldlenp, newp, ctob(physmem)));
	case HW_USERMEM:
		return (sysctl_rdint(oldp, oldlenp, newp,
		    ctob(physmem - uvmexp.wired)));
	case HW_PAGESIZE:
		return (sysctl_rdint(oldp, oldlenp, newp, PAGE_SIZE));
	case HW_ALIGNBYTES:
		return (sysctl_rdint(oldp, oldlenp, newp, ALIGNBYTES));
	case HW_CNMAGIC: {
		char magic[CNS_LEN];
		int error;

		if (oldp)
			cn_get_magic(magic, CNS_LEN);
		error = sysctl_string(oldp, oldlenp, newp, newlen,
		    magic, sizeof(magic));
		if (newp && !error) {
			error = cn_set_magic(magic);
		}
		return (error);
	}
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

#ifdef DEBUG
/*
 * Debugging related system variables.
 */
struct ctldebug debug0, debug1, debug2, debug3, debug4;
struct ctldebug debug5, debug6, debug7, debug8, debug9;
struct ctldebug debug10, debug11, debug12, debug13, debug14;
struct ctldebug debug15, debug16, debug17, debug18, debug19;
static struct ctldebug *debugvars[CTL_DEBUG_MAXID] = {
	&debug0, &debug1, &debug2, &debug3, &debug4,
	&debug5, &debug6, &debug7, &debug8, &debug9,
	&debug10, &debug11, &debug12, &debug13, &debug14,
	&debug15, &debug16, &debug17, &debug18, &debug19,
};

int
debug_sysctl(int *name, u_int namelen, void *oldp, size_t *oldlenp,
    void *newp, size_t newlen, struct proc *p)
{
	struct ctldebug *cdp;

	/* all sysctl names at this level are name and field */
	if (namelen != 2)
		return (ENOTDIR);		/* overloaded */
	cdp = debugvars[name[0]];
	if (name[0] >= CTL_DEBUG_MAXID || cdp->debugname == 0)
		return (EOPNOTSUPP);
	switch (name[1]) {
	case CTL_DEBUG_NAME:
		return (sysctl_rdstring(oldp, oldlenp, newp, cdp->debugname));
	case CTL_DEBUG_VALUE:
		return (sysctl_int(oldp, oldlenp, newp, newlen, cdp->debugvar));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}
#endif /* DEBUG */

int
proc_sysctl(int *name, u_int namelen, void *oldp, size_t *oldlenp,
    void *newp, size_t newlen, struct proc *p)
{
	struct proc *ptmp = NULL;
	const struct proclist_desc *pd;
	int error = 0;
	struct rlimit alim;
	struct plimit *newplim;
	char *tmps = NULL;
	int i, curlen, len;

	if (namelen < 2)
		return EINVAL;

	if (name[0] == PROC_CURPROC) {
		ptmp = p;
	} else {
		proclist_lock_read();
		for (pd = proclists; pd->pd_list != NULL; pd++) {
			for (ptmp = LIST_FIRST(pd->pd_list); ptmp != NULL;
			    ptmp = LIST_NEXT(ptmp, p_list)) {
				/* Skip embryonic processes. */
				if (ptmp->p_stat == SIDL)
					continue;
				if (ptmp->p_pid == (pid_t)name[0])
					break;
			}
			if (ptmp != NULL)
				break;
		}
		proclist_unlock_read();
		if (ptmp == NULL)
			return(ESRCH);
		if (p->p_ucred->cr_uid != 0) {
			if(p->p_cred->p_ruid != ptmp->p_cred->p_ruid ||
			    p->p_cred->p_ruid != ptmp->p_cred->p_svuid)
				return EPERM;
			if (ptmp->p_cred->p_rgid != ptmp->p_cred->p_svgid)
				return EPERM; /* sgid proc */
			for (i = 0; i < p->p_ucred->cr_ngroups; i++) {
				if (p->p_ucred->cr_groups[i] ==
				    ptmp->p_cred->p_rgid)
					break;
			}
			if (i == p->p_ucred->cr_ngroups)
				return EPERM;
		}
	}
	if (name[1] == PROC_PID_CORENAME) {
		if (namelen != 2)
			return EINVAL;
		/*
		 * Can't use sysctl_string() here because we may malloc a new
		 * area during the process, so we have to do it by hand.
		 */
		curlen = strlen(ptmp->p_limit->pl_corename) + 1;
		if (oldlenp  && *oldlenp < curlen) {
			if (!oldp)
				*oldlenp = curlen;
			return (ENOMEM);
		}
		if (newp) {
			if (securelevel > 2)
				return EPERM;
			if (newlen > MAXPATHLEN)
				return ENAMETOOLONG;
			tmps = malloc(newlen + 1, M_TEMP, M_WAITOK);
			if (tmps == NULL)
				return ENOMEM;
			error = copyin(newp, tmps, newlen + 1);
			tmps[newlen] = '\0';
			if (error)
				goto cleanup;
			/* Enforce to be either 'core' for end with '.core' */
			if (newlen < 4)  { /* c.o.r.e */
				error = EINVAL;
				goto cleanup;
			}
			len = newlen - 4;
			if (len > 0) {
				if (tmps[len - 1] != '.' &&
				    tmps[len - 1] != '/') {
					error = EINVAL;
					goto cleanup;
				}
			}
			if (strcmp(&tmps[len], "core") != 0) {
				error = EINVAL;
				goto cleanup;
			}
		}
		if (oldp && oldlenp) {
			*oldlenp = curlen;
			error = copyout(ptmp->p_limit->pl_corename, oldp,
			    curlen);
		}
		if (newp && error == 0) {
			/* if the 2 strings are identical, don't limcopy() */
			if (strcmp(tmps, ptmp->p_limit->pl_corename) == 0) {
				error = 0;
				goto cleanup;
			}
			if (ptmp->p_limit->p_refcnt > 1 &&
			    (ptmp->p_limit->p_lflags & PL_SHAREMOD) == 0) {
				newplim = limcopy(ptmp->p_limit);
				limfree(ptmp->p_limit);
				ptmp->p_limit = newplim;
			} else if (ptmp->p_limit->pl_corename != defcorename) {
				free(ptmp->p_limit->pl_corename, M_TEMP);
			}
			ptmp->p_limit->pl_corename = tmps;
			return (0);
		}
cleanup:
		if (tmps)
			free(tmps, M_TEMP);
		return (error);
	}
	if (name[1] == PROC_PID_LIMIT) {
		if (namelen != 4 || name[2] >= PROC_PID_LIMIT_MAXID)
			return EINVAL;
		memcpy(&alim, &ptmp->p_rlimit[name[2] - 1], sizeof(alim));
		if (name[3] == PROC_PID_LIMIT_TYPE_HARD)
			error = sysctl_quad(oldp, oldlenp, newp, newlen,
			    &alim.rlim_max);
		else if (name[3] == PROC_PID_LIMIT_TYPE_SOFT)
			error = sysctl_quad(oldp, oldlenp, newp, newlen,
			    &alim.rlim_cur);
		else 
			error = EINVAL;

		if (error)
			return error;

		if (newp)
			error = dosetrlimit(ptmp, p->p_cred,
			    name[2] - 1, &alim);
		return error;
	}
	return (EINVAL);
}

/*
 * Convenience macros.
 */

#define SYSCTL_SCALAR_CORE_LEN(oldp, oldlenp, valp, len) 		\
	if (oldlenp) {							\
		if (!oldp)						\
			*oldlenp = len;					\
		else {							\
			if (*oldlenp < len)				\
				return(ENOMEM);				\
			*oldlenp = len;					\
			error = copyout((caddr_t)valp, oldp, len);	\
		}							\
	}

#define SYSCTL_SCALAR_CORE_TYP(oldp, oldlenp, valp, typ) \
	SYSCTL_SCALAR_CORE_LEN(oldp, oldlenp, valp, sizeof(typ))

#define SYSCTL_SCALAR_NEWPCHECK_LEN(newp, newlen, len)	\
	if (newp && newlen != len)			\
		return (EINVAL);

#define SYSCTL_SCALAR_NEWPCHECK_TYP(newp, newlen, typ)	\
	SYSCTL_SCALAR_NEWPCHECK_LEN(newp, newlen, sizeof(typ))

#define SYSCTL_SCALAR_NEWPCOP_LEN(newp, valp, len)	\
	if (error == 0 && newp)				\
		error = copyin(newp, valp, len);

#define SYSCTL_SCALAR_NEWPCOP_TYP(newp, valp, typ)      \
	SYSCTL_SCALAR_NEWPCOP_LEN(newp, valp, sizeof(typ))

#define SYSCTL_STRING_CORE(oldp, oldlenp, str)		\
	if (oldlenp) {					\
		len = strlen(str) + 1;			\
		if (!oldp)				\
			*oldlenp = len;			\
		else {					\
			if (*oldlenp < len) {		\
				err2 = ENOMEM;		\
				len = *oldlenp;		\
			} else				\
				*oldlenp = len;		\
			error = copyout(str, oldp, len);\
			if (error == 0)			\
				error = err2;		\
		}					\
	}

/*
 * Validate parameters and get old / set new parameters
 * for an integer-valued sysctl function.
 */
int
sysctl_int(void *oldp, size_t *oldlenp, void *newp, size_t newlen, int *valp)
{
	int error = 0;

	SYSCTL_SCALAR_NEWPCHECK_TYP(newp, newlen, int)
	SYSCTL_SCALAR_CORE_TYP(oldp, oldlenp, valp, int)
	SYSCTL_SCALAR_NEWPCOP_TYP(newp, valp, int)

	return (error);
}


/*
 * As above, but read-only.
 */
int
sysctl_rdint(void *oldp, size_t *oldlenp, void *newp, int val)
{
	int error = 0;

	if (newp)
		return (EPERM);

	SYSCTL_SCALAR_CORE_TYP(oldp, oldlenp, &val, int)

	return (error);
}

/*
 * Validate parameters and get old / set new parameters
 * for an quad-valued sysctl function.
 */
int
sysctl_quad(void *oldp, size_t *oldlenp, void *newp, size_t newlen,
    quad_t *valp)
{
	int error = 0;

	SYSCTL_SCALAR_NEWPCHECK_TYP(newp, newlen, quad_t)
	SYSCTL_SCALAR_CORE_TYP(oldp, oldlenp, valp, quad_t)
	SYSCTL_SCALAR_NEWPCOP_TYP(newp, valp, quad_t)

	return (error);
}

/*
 * As above, but read-only.
 */
int
sysctl_rdquad(void *oldp, size_t *oldlenp, void *newp, quad_t val)
{
	int error = 0;

	if (newp)
		return (EPERM);

	SYSCTL_SCALAR_CORE_TYP(oldp, oldlenp, &val, quad_t)

	return (error);
}

/*
 * Validate parameters and get old / set new parameters
 * for a string-valued sysctl function.
 */
int
sysctl_string(void *oldp, size_t *oldlenp, void *newp, size_t newlen, char *str,
    int maxlen)
{
	int len, error = 0, err2 = 0;

	if (newp && newlen >= maxlen)
		return (EINVAL);

	SYSCTL_STRING_CORE(oldp, oldlenp, str);

	if (error == 0 && newp) {
		error = copyin(newp, str, newlen);
		str[newlen] = 0;
	}
	return (error);
}

/*
 * As above, but read-only.
 */
int
sysctl_rdstring(void *oldp, size_t *oldlenp, void *newp, const char *str)
{
	int len, error = 0, err2 = 0;

	if (newp)
		return (EPERM);

	SYSCTL_STRING_CORE(oldp, oldlenp, str);

	return (error);
}

/*
 * Validate parameters and get old / set new parameters
 * for a structure oriented sysctl function.
 */
int
sysctl_struct(void *oldp, size_t *oldlenp, void *newp, size_t newlen, void *sp,
    int len)
{
	int error = 0;

	SYSCTL_SCALAR_NEWPCHECK_LEN(newp, newlen, len)
	SYSCTL_SCALAR_CORE_LEN(oldp, oldlenp, sp, len)
	SYSCTL_SCALAR_NEWPCOP_LEN(newp, sp, len)

	return (error);
}

/*
 * Validate parameters and get old parameters
 * for a structure oriented sysctl function.
 */
int
sysctl_rdstruct(void *oldp, size_t *oldlenp, void *newp, const void *sp,
    int len)
{
	int error = 0;

	if (newp)
		return (EPERM);

	SYSCTL_SCALAR_CORE_LEN(oldp, oldlenp, sp, len)

	return (error);
}

/*
 * As above, but can return a truncated result.
 */
int
sysctl_rdminstruct(void *oldp, size_t *oldlenp, void *newp, const void *sp,
    int len)
{
	int error = 0;

	if (newp)
		return (EPERM);

	len = min(*oldlenp, len);
	SYSCTL_SCALAR_CORE_LEN(oldp, oldlenp, sp, len)

	return (error);
}

/*
 * Get file structures.
 */
static int
sysctl_file(void *vwhere, size_t *sizep)
{
	int buflen, error;
	struct file *fp;
	char *start, *where;

	start = where = vwhere;
	buflen = *sizep;
	if (where == NULL) {
		/*
		 * overestimate by 10 files
		 */
		*sizep = sizeof(filehead) + (nfiles + 10) * sizeof(struct file);
		return (0);
	}

	/*
	 * first copyout filehead
	 */
	if (buflen < sizeof(filehead)) {
		*sizep = 0;
		return (0);
	}
	error = copyout((caddr_t)&filehead, where, sizeof(filehead));
	if (error)
		return (error);
	buflen -= sizeof(filehead);
	where += sizeof(filehead);

	/*
	 * followed by an array of file structures
	 */
	for (fp = filehead.lh_first; fp != 0; fp = fp->f_list.le_next) {
		if (buflen < sizeof(struct file)) {
			*sizep = where - start;
			return (ENOMEM);
		}
		error = copyout((caddr_t)fp, where, sizeof(struct file));
		if (error)
			return (error);
		buflen -= sizeof(struct file);
		where += sizeof(struct file);
	}
	*sizep = where - start;
	return (0);
}

#if defined(SYSVMSG) || defined(SYSVSEM) || defined(SYSVSHM)
#define	FILL_PERM(src, dst) do { \
		(dst)._key = (src)._key; \
		(dst).uid = (src).uid; \
		(dst).gid = (src).gid; \
		(dst).cuid = (src).cuid; \
		(dst).cgid = (src).cgid; \
		(dst).mode = (src).mode; \
		(dst)._seq = (src)._seq; \
	} while (0);
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
	} while (0)
#define	FILL_SEM(src, dst) do { \
	FILL_PERM((src).sem_perm, (dst).sem_perm); \
	(dst).sem_nsems = (src).sem_nsems; \
	(dst).sem_otime = (src).sem_otime; \
	(dst).sem_ctime = (src).sem_ctime; \
	} while (0)
#define	FILL_SHM(src, dst) do { \
	FILL_PERM((src).shm_perm, (dst).shm_perm); \
	(dst).shm_segsz = (src).shm_segsz; \
	(dst).shm_lpid = (src).shm_lpid; \
	(dst).shm_cpid = (src).shm_cpid; \
	(dst).shm_atime = (src).shm_atime; \
	(dst).shm_dtime = (src).shm_dtime; \
	(dst).shm_ctime = (src).shm_ctime; \
	(dst).shm_nattch = (src).shm_nattch; \
	} while (0)

static int
sysctl_sysvipc(int *name, u_int namelen, void *where, size_t *sizep)
{
#ifdef SYSVMSG
	struct msg_sysctl_info *msgsi;
#endif
#ifdef SYSVSEM
	struct sem_sysctl_info *semsi;
#endif
#ifdef SYSVSHM
	struct shm_sysctl_info *shmsi;
#endif
	size_t infosize, dssize, tsize, buflen;
	void *buf = NULL, *buf2;
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
	buf = malloc(min(tsize, buflen), M_TEMP, M_WAITOK);
	memset(buf, 0, min(tsize, buflen));

	switch (*name) { 
#ifdef SYSVMSG
	case KERN_SYSVIPC_MSG_INFO:
		msgsi = (struct msg_sysctl_info *)buf;
		buf2 = &msgsi->msgids[0];
		msgsi->msginfo = msginfo;
		break;
#endif
#ifdef SYSVSEM
	case KERN_SYSVIPC_SEM_INFO:
		semsi = (struct sem_sysctl_info *)buf;
		buf2 = &semsi->semids[0];
		semsi->seminfo = seminfo;
		break;
#endif
#ifdef SYSVSHM
	case KERN_SYSVIPC_SHM_INFO:
		shmsi = (struct shm_sysctl_info *)buf;
		buf2 = &shmsi->shmids[0];
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
	error = copyout(buf, start, *sizep);
	/* If copyout succeeded, use return code set earlier. */
	if (error == 0)
		error = ret;
	if (buf)
		free(buf, M_TEMP);
	return (error);
}
#endif /* SYSVMSG || SYSVSEM || SYSVSHM */

static int
sysctl_msgbuf(void *vwhere, size_t *sizep)
{
	char *where = vwhere;
	size_t len, maxlen = *sizep;
	long beg, end;
	int error;

	/*
	 * deal with cases where the message buffer has
	 * become corrupted.
	 */
	if (!msgbufenabled || msgbufp->msg_magic != MSG_MAGIC) {
		msgbufenabled = 0;
		return (ENXIO);
	}

	if (where == NULL) {
		/* always return full buffer size */
		*sizep = msgbufp->msg_bufs;
		return (0);
	}

	error = 0;
	maxlen = min(msgbufp->msg_bufs, maxlen);

	/*
	 * First, copy from the write pointer to the end of
	 * message buffer.
	 */
	beg = msgbufp->msg_bufx;
	end = msgbufp->msg_bufs;
	while (maxlen > 0) {
		len = min(end - beg, maxlen);
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
 * try over estimating by 5 procs
 */
#define KERN_PROCSLOP	(5 * sizeof(struct kinfo_proc))

static int
sysctl_doeproc(int *name, u_int namelen, void *vwhere, size_t *sizep)
{
	struct eproc eproc;
	struct kinfo_proc2 kproc2;
	struct kinfo_proc *dp;
	struct proc *p;
	const struct proclist_desc *pd;
	char *where, *dp2;
	int type, op, arg, elem_size, elem_count;
	int buflen, needed, error;

	dp = vwhere;
	dp2 = where = vwhere;
	buflen = where != NULL ? *sizep : 0;
	error = needed = 0;
	type = name[0];

	if (type == KERN_PROC) {
		if (namelen != 3 && !(namelen == 2 && name[1] == KERN_PROC_ALL))
			return (EINVAL);
		op = name[1];
		if (op != KERN_PROC_ALL)
			arg = name[2];
	} else {
		if (namelen != 5)
			return (EINVAL);
		op = name[1];
		arg = name[2];
		elem_size = name[3];
		elem_count = name[4];
	}

	proclist_lock_read();

	pd = proclists;
again:
	for (p = LIST_FIRST(pd->pd_list); p != NULL; p = LIST_NEXT(p, p_list)) {
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
			if (arg == KERN_PROC_TTY_REVOKE) {
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
				error = copyout((caddr_t)p, &dp->kp_proc,
						sizeof(struct proc));
				if (error)
					goto cleanup;
				error = copyout((caddr_t)&eproc, &dp->kp_eproc,
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
			*sizep = (caddr_t)dp - where;
		else
			*sizep = dp2 - where;
		if (needed > *sizep)
			return (ENOMEM);
	} else {
		needed += KERN_PROCSLOP;
		*sizep = needed;
	}
	return (0);
 cleanup:
	proclist_unlock_read();
	return (error);
}

/*
 * Fill in an eproc structure for the specified process.
 */
void
fill_eproc(struct proc *p, struct eproc *ep)
{
	struct tty *tp;

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
		ep->e_tpgid = tp->t_pgrp ? tp->t_pgrp->pg_id : NO_PID;
		ep->e_tsess = tp->t_session;
	} else
		ep->e_tdev = NODEV;
	if (p->p_wmesg)
		strncpy(ep->e_wmesg, p->p_wmesg, WMESGLEN);
	ep->e_xsize = ep->e_xrssize = 0;
	ep->e_xccount = ep->e_xswrss = 0;
	ep->e_flag = ep->e_sess->s_ttyvp ? EPROC_CTTY : 0;
	if (SESS_LEADER(p))
		ep->e_flag |= EPROC_SLEADER;
	strncpy(ep->e_login, ep->e_sess->s_login, MAXLOGNAME);
}

/*
 * Fill in an eproc structure for the specified process.
 */
static void
fill_kproc2(struct proc *p, struct kinfo_proc2 *ki)
{
	struct tty *tp;

	memset(ki, 0, sizeof(*ki));

	ki->p_forw = PTRTOINT64(p->p_forw);
	ki->p_back = PTRTOINT64(p->p_back);
	ki->p_paddr = PTRTOINT64(p);

	ki->p_addr = PTRTOINT64(p->p_addr);
	ki->p_fd = PTRTOINT64(p->p_fd);
	ki->p_cwdi = PTRTOINT64(p->p_cwdi);
	ki->p_stats = PTRTOINT64(p->p_stats);
	ki->p_limit = PTRTOINT64(p->p_limit);
	ki->p_vmspace = PTRTOINT64(p->p_vmspace);
	ki->p_sigacts = PTRTOINT64(p->p_sigacts);
	ki->p_sess = PTRTOINT64(p->p_session);
	ki->p_tsess = 0;	/* may be changed if controlling tty below */
	ki->p_ru = PTRTOINT64(p->p_ru);

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

	ki->p_tpgid = NO_PID;	/* may be changed if controlling tty below */

	ki->p_uid = p->p_ucred->cr_uid;
	ki->p_ruid = p->p_cred->p_ruid;
	ki->p_gid = p->p_ucred->cr_gid;
	ki->p_rgid = p->p_cred->p_rgid;

	memcpy(ki->p_groups, p->p_cred->pc_ucred->cr_groups,
	    min(sizeof(ki->p_groups), sizeof(p->p_cred->pc_ucred->cr_groups)));
	ki->p_ngroups = p->p_cred->pc_ucred->cr_ngroups;

	ki->p_jobc = p->p_pgrp->pg_jobc;
	if ((p->p_flag & P_CONTROLT) && (tp = p->p_session->s_ttyp)) {
		ki->p_tdev = tp->t_dev;
		ki->p_tpgid = tp->t_pgrp ? tp->t_pgrp->pg_id : NO_PID;
		ki->p_tsess = PTRTOINT64(tp->t_session);
	} else {
		ki->p_tdev = NODEV;
	}

	ki->p_estcpu = p->p_estcpu;
	ki->p_rtime_sec = p->p_rtime.tv_sec;
	ki->p_rtime_usec = p->p_rtime.tv_usec;
	ki->p_cpticks = p->p_cpticks;
	ki->p_pctcpu = p->p_pctcpu;
	ki->p_swtime = p->p_swtime;
	ki->p_slptime = p->p_slptime;
	if (p->p_stat == SONPROC) {
		KDASSERT(p->p_cpu != NULL);
		ki->p_schedflags = p->p_cpu->ci_schedstate.spc_flags;
	} else
		ki->p_schedflags = 0;

	ki->p_uticks = p->p_uticks;
	ki->p_sticks = p->p_sticks;
	ki->p_iticks = p->p_iticks;

	ki->p_tracep = PTRTOINT64(p->p_tracep);
	ki->p_traceflag = p->p_traceflag;

	ki->p_holdcnt = p->p_holdcnt;

	memcpy(&ki->p_siglist, &p->p_sigctx.ps_siglist, sizeof(ki_sigset_t));
	memcpy(&ki->p_sigmask, &p->p_sigctx.ps_sigmask, sizeof(ki_sigset_t));
	memcpy(&ki->p_sigignore, &p->p_sigctx.ps_sigignore,sizeof(ki_sigset_t));
	memcpy(&ki->p_sigcatch, &p->p_sigctx.ps_sigcatch, sizeof(ki_sigset_t));

	ki->p_stat = p->p_stat;
	ki->p_priority = p->p_priority;
	ki->p_usrpri = p->p_usrpri;
	ki->p_nice = p->p_nice;

	ki->p_xstat = p->p_xstat;
	ki->p_acflag = p->p_acflag;

	strncpy(ki->p_comm, p->p_comm,
	    min(sizeof(ki->p_comm), sizeof(p->p_comm)));

	if (p->p_wmesg)
		strncpy(ki->p_wmesg, p->p_wmesg, sizeof(ki->p_wmesg));
	ki->p_wchan = PTRTOINT64(p->p_wchan);

	strncpy(ki->p_login, p->p_session->s_login, sizeof(ki->p_login));

	if (p->p_stat == SIDL || P_ZOMBIE(p)) {
		ki->p_vm_rssize = 0;
		ki->p_vm_tsize = 0;
		ki->p_vm_dsize = 0;
		ki->p_vm_ssize = 0;
	} else {
		struct vmspace *vm = p->p_vmspace;

		ki->p_vm_rssize = vm_resident_count(vm);
		ki->p_vm_tsize = vm->vm_tsize;
		ki->p_vm_dsize = vm->vm_dsize;
		ki->p_vm_ssize = vm->vm_ssize;
	}

	if (p->p_session->s_ttyvp)
		ki->p_eflag |= EPROC_CTTY;
	if (SESS_LEADER(p))
		ki->p_eflag |= EPROC_SLEADER;

	/* XXX Is this double check necessary? */
	if ((p->p_flag & P_INMEM) == 0 || P_ZOMBIE(p)) {
		ki->p_uvalid = 0;
	} else {
		ki->p_uvalid = 1;

		ki->p_ustart_sec = p->p_stats->p_start.tv_sec;
		ki->p_ustart_usec = p->p_stats->p_start.tv_usec;

		ki->p_uutime_sec = p->p_stats->p_ru.ru_utime.tv_sec;
		ki->p_uutime_usec = p->p_stats->p_ru.ru_utime.tv_usec;
		ki->p_ustime_sec = p->p_stats->p_ru.ru_stime.tv_sec;
		ki->p_ustime_usec = p->p_stats->p_ru.ru_stime.tv_usec;

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

		ki->p_uctime_sec = p->p_stats->p_cru.ru_utime.tv_sec +
		    p->p_stats->p_cru.ru_stime.tv_sec;
		ki->p_uctime_usec = p->p_stats->p_cru.ru_utime.tv_usec +
		    p->p_stats->p_cru.ru_stime.tv_usec;
	}
#ifdef MULTIPROCESSOR
	if (p->p_cpu != NULL)
		ki->p_cpuid = p->p_cpu->ci_cpuid;
	else
#endif
		ki->p_cpuid = KI_NOCPU;
}

int
sysctl_procargs(int *name, u_int namelen, void *where, size_t *sizep,
    struct proc *up)
{
	struct ps_strings pss;
	struct proc *p;
	size_t len, upper_bound, xlen;
	struct uio auio;
	struct iovec aiov;
	vaddr_t argv;
	pid_t pid;
	int nargv, type, error, i;
	char *arg;
	char *tmp;

	if (namelen != 2)
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

	if (sizep != NULL && where == NULL) {
		if (type == KERN_PROC_NARGV || type == KERN_PROC_NENV)
			*sizep = sizeof (int);
		else
			*sizep = ARG_MAX;	/* XXX XXX XXX */
		return (0);
	}
	if (where == NULL || sizep == NULL)
		return (EINVAL);

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
		error = copyout(&nargv, where, sizeof(nargv));
		*sizep = sizeof(nargv);
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
	auio.uio_offset = (off_t)(long)tmp;
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
	upper_bound = *sizep;
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

		/* make sure we don't copyout past the end of the user's buffer */
		if (len + i > upper_bound)
			i = upper_bound - len;

		error = copyout(arg, (char *)where + len, i);
		if (error)
			break;

		if (nargv == 0) {
			len += i;
			break;
		}
	}
	*sizep = len;

done:
	uvmspace_free(p->p_vmspace);

	free(arg, M_TEMP);
	return (error);
}

#if NPTY > 0
int pty_maxptys(int, int);		/* defined in kern/tty_pty.c */

/*
 * Validate parameters and get old / set new parameters
 * for pty sysctl function.
 */
static int
sysctl_pty(void *oldp, size_t *oldlenp, void *newp, size_t newlen)
{
	int error = 0;
	int oldmax = 0, newmax = 0;

	/* get current value of maxptys */
	oldmax = pty_maxptys(0, 0);

	SYSCTL_SCALAR_CORE_TYP(oldp, oldlenp, &oldmax, int)

	if (!error && newp) {
		SYSCTL_SCALAR_NEWPCHECK_TYP(newp, newlen, int)
		SYSCTL_SCALAR_NEWPCOP_TYP(newp, &newmax, int)

		if (newmax != pty_maxptys(newmax, (newp != NULL)))
			return (EINVAL);

	}

	return (error);
}
#endif /* NPTY > 0 */
