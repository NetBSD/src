/*	$NetBSD: kern_sysctl.c,v 1.53 1999/11/03 09:12:15 jdolecek Exp $	*/

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
#include "opt_sysv.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/unistd.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/disklabel.h>
#include <sys/device.h>
#include <vm/vm.h>
#include <sys/sysctl.h>
#include <sys/msgbuf.h>

#include <uvm/uvm_extern.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>


#if defined(DDB)
#include <ddb/ddbvar.h>
#endif

/*
 * Locking and stats
 */
static struct sysctl_lock {
	int	sl_lock;
	int	sl_want;
	int	sl_locked;
} memlock;

int
sys___sysctl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	register struct sys___sysctl_args /* {
		syscallarg(int *) name;
		syscallarg(u_int) namelen;
		syscallarg(void *) old;
		syscallarg(size_t *) oldlenp;
		syscallarg(void *) new;
		syscallarg(size_t) newlen;
	} */ *uap = v;
	int error, dolock = 1;
	size_t savelen = 0, oldlen = 0;
	sysctlfn *fn;
	int name[CTL_MAXNAME];

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
		if (name[2] != KERN_VNODE)	/* XXX */
			dolock = 0;
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

	if (SCARG(uap, oldlenp) &&
	    (error = copyin(SCARG(uap, oldlenp), &oldlen, sizeof(oldlen))))
		return (error);
	if (SCARG(uap, old) != NULL) {
		if (!uvm_useracc(SCARG(uap, old), oldlen, B_WRITE))
			return (EFAULT);
		while (memlock.sl_lock) {
			memlock.sl_want = 1;
			sleep((caddr_t)&memlock, PRIBIO+1);
			memlock.sl_locked++;
		}
		memlock.sl_lock = 1;
		if (dolock) {
			/*
			 * XXX Um, this is kind of evil.  What should we
			 * XXX be passing here?
			 */
			if (uvm_vslock(p, SCARG(uap, old), oldlen,
			    VM_PROT_NONE) != KERN_SUCCESS) {
				memlock.sl_lock = 0;
				if (memlock.sl_want) {
					memlock.sl_want = 0;
					wakeup((caddr_t)&memlock);
					return (EFAULT);
				}
			}
		}
		savelen = oldlen;
	}
	error = (*fn)(name + 1, SCARG(uap, namelen) - 1, SCARG(uap, old),
	    &oldlen, SCARG(uap, new), SCARG(uap, newlen), p);
	if (SCARG(uap, old) != NULL) {
		if (dolock)
			uvm_vsunlock(p, SCARG(uap, old), savelen);
		memlock.sl_lock = 0;
		if (memlock.sl_want) {
			memlock.sl_want = 0;
			wakeup((caddr_t)&memlock);
		}
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
#ifdef DEFCORENAME
char defcorename[MAXPATHLEN] = DEFCORENAME;
int defcorenamelen = sizeof(DEFCORENAME);
#else
char defcorename[MAXPATHLEN] = "%n.core";
int defcorenamelen = sizeof("%n.core");
#endif

/*
 * kernel related system variables.
 */
int
kern_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	int error, level, inthostid;
	int old_autonicetime;
	int old_vnodes;
	extern char ostype[], osrelease[], version[];

	/* All sysctl names at this level, except for a few, are terminal. */
	switch (name[0]) {
	case KERN_PROC:
	case KERN_PROF:
	case KERN_MBUF:
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
		return (sysctl_rdint(oldp, oldlenp, newp, NetBSD));
	case KERN_VERSION:
		return (sysctl_rdstring(oldp, oldlenp, newp, version));
	case KERN_MAXVNODES:
		old_vnodes = desiredvnodes;
		error = sysctl_int(oldp, oldlenp, newp, newlen, &desiredvnodes);
		if (old_vnodes > desiredvnodes) {
		        desiredvnodes = old_vnodes;
			return (EINVAL);
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
		return (sysctl_doeproc(name + 1, namelen - 1, oldp, oldlenp));
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
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

/*
 * hardware related system variables.
 */
int
hw_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	extern char machine[], machine_arch[], cpu_model[];

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
		return (sysctl_rdint(oldp, oldlenp, newp, 1));	/* XXX */
	case HW_BYTEORDER:
		return (sysctl_rdint(oldp, oldlenp, newp, BYTE_ORDER));
	case HW_PHYSMEM:
		return (sysctl_rdint(oldp, oldlenp, newp, ctob(physmem)));
	case HW_USERMEM:
		return (sysctl_rdint(oldp, oldlenp, newp,
		    ctob(physmem - uvmexp.wired)));
	case HW_PAGESIZE:
		return (sysctl_rdint(oldp, oldlenp, newp, PAGE_SIZE));
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
debug_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
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
proc_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	struct proc *ptmp=NULL;
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
		if (oldp && *oldlenp < curlen)
			return (ENOMEM);
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
		if (oldp) {
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
 * Validate parameters and get old / set new parameters
 * for an integer-valued sysctl function.
 */
int
sysctl_int(oldp, oldlenp, newp, newlen, valp)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	int *valp;
{
	int error = 0;

	if (oldp && *oldlenp < sizeof(int))
		return (ENOMEM);
	if (newp && newlen != sizeof(int))
		return (EINVAL);
	*oldlenp = sizeof(int);
	if (oldp)
		error = copyout(valp, oldp, sizeof(int));
	if (error == 0 && newp)
		error = copyin(newp, valp, sizeof(int));
	return (error);
}

/*
 * As above, but read-only.
 */
int
sysctl_rdint(oldp, oldlenp, newp, val)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	int val;
{
	int error = 0;

	if (oldp && *oldlenp < sizeof(int))
		return (ENOMEM);
	if (newp)
		return (EPERM);
	*oldlenp = sizeof(int);
	if (oldp)
		error = copyout((caddr_t)&val, oldp, sizeof(int));
	return (error);
}

/*
 * Validate parameters and get old / set new parameters
 * for an quad-valued sysctl function.
 */
int
sysctl_quad(oldp, oldlenp, newp, newlen, valp)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	quad_t *valp;
{
	int error = 0;

	if (oldp && *oldlenp < sizeof(quad_t))
		return (ENOMEM);
	if (newp && newlen != sizeof(quad_t))
		return (EINVAL);
	*oldlenp = sizeof(quad_t);
	if (oldp)
		error = copyout(valp, oldp, sizeof(quad_t));
	if (error == 0 && newp)
		error = copyin(newp, valp, sizeof(quad_t));
	return (error);
}

/*
 * As above, but read-only.
 */
int
sysctl_rdquad(oldp, oldlenp, newp, val)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	quad_t val;
{
	int error = 0;

	if (oldp && *oldlenp < sizeof(quad_t))
		return (ENOMEM);
	if (newp)
		return (EPERM);
	*oldlenp = sizeof(quad_t);
	if (oldp)
		error = copyout((caddr_t)&val, oldp, sizeof(quad_t));
	return (error);
}


/*
 * Validate parameters and get old / set new parameters
 * for a string-valued sysctl function.
 */
int
sysctl_string(oldp, oldlenp, newp, newlen, str, maxlen)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	char *str;
	int maxlen;
{
	int len, error = 0;

	len = strlen(str) + 1;
	if (oldp && *oldlenp < len)
		return (ENOMEM);
	if (newp && newlen >= maxlen)
		return (EINVAL);
	if (oldp) {
		*oldlenp = len;
		error = copyout(str, oldp, len);
	}
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
sysctl_rdstring(oldp, oldlenp, newp, str)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	char *str;
{
	int len, error = 0;

	len = strlen(str) + 1;
	if (oldp && *oldlenp < len)
		return (ENOMEM);
	if (newp)
		return (EPERM);
	*oldlenp = len;
	if (oldp)
		error = copyout(str, oldp, len);
	return (error);
}

/*
 * Validate parameters and get old / set new parameters
 * for a structure oriented sysctl function.
 */
int
sysctl_struct(oldp, oldlenp, newp, newlen, sp, len)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	void *sp;
	int len;
{
	int error = 0;

	if (oldp && *oldlenp < len)
		return (ENOMEM);
	if (newp && newlen > len)
		return (EINVAL);
	if (oldp) {
		*oldlenp = len;
		error = copyout(sp, oldp, len);
	}
	if (error == 0 && newp)
		error = copyin(newp, sp, len);
	return (error);
}

/*
 * Validate parameters and get old parameters
 * for a structure oriented sysctl function.
 */
int
sysctl_rdstruct(oldp, oldlenp, newp, sp, len)
	void *oldp;
	size_t *oldlenp;
	void *newp, *sp;
	int len;
{
	int error = 0;

	if (oldp && *oldlenp < len)
		return (ENOMEM);
	if (newp)
		return (EPERM);
	*oldlenp = len;
	if (oldp)
		error = copyout(sp, oldp, len);
	return (error);
}

/*
 * Get file structures.
 */
int
sysctl_file(where, sizep)
	char *where;
	size_t *sizep;
{
	int buflen, error;
	struct file *fp;
	char *start = where;

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

/*
 * try over estimating by 5 procs
 */
#define KERN_PROCSLOP	(5 * sizeof(struct kinfo_proc))

int
sysctl_doeproc(name, namelen, where, sizep)
	int *name;
	u_int namelen;
	char *where;
	size_t *sizep;
{
	register struct proc *p;
	register struct kinfo_proc *dp = (struct kinfo_proc *)where;
	register int needed = 0;
	int buflen = where != NULL ? *sizep : 0;
	const struct proclist_desc *pd;
	struct eproc eproc;
	int error = 0;

	if (namelen != 2 && !(namelen == 1 && name[0] == KERN_PROC_ALL))
		return (EINVAL);

	proclist_lock_read();

	pd = proclists;
again:
	for (p = LIST_FIRST(pd->pd_list); p != NULL;
	     p = LIST_NEXT(p, p_list)) {
		/*
		 * Skip embryonic processes.
		 */
		if (p->p_stat == SIDL)
			continue;
		/*
		 * TODO - make more efficient (see notes below).
		 * do by session.
		 */
		switch (name[0]) {

		case KERN_PROC_PID:
			/* could do this with just a lookup */
			if (p->p_pid != (pid_t)name[1])
				continue;
			break;

		case KERN_PROC_PGRP:
			/* could do this by traversing pgrp */
			if (p->p_pgrp->pg_id != (pid_t)name[1])
				continue;
			break;

		case KERN_PROC_TTY:
			if ((p->p_flag & P_CONTROLT) == 0 ||
			    p->p_session->s_ttyp == NULL ||
			    p->p_session->s_ttyp->t_dev != (dev_t)name[1])
				continue;
			break;

		case KERN_PROC_UID:
			if (p->p_ucred->cr_uid != (uid_t)name[1])
				continue;
			break;

		case KERN_PROC_RUID:
			if (p->p_cred->p_ruid != (uid_t)name[1])
				continue;
			break;
		}
		if (buflen >= sizeof(struct kinfo_proc)) {
			fill_eproc(p, &eproc);
			error = copyout((caddr_t)p, &dp->kp_proc,
					sizeof(struct proc));
			if (error)
				return (error);
			error = copyout((caddr_t)&eproc, &dp->kp_eproc,
					sizeof(eproc));
			if (error)
				return (error);
			dp++;
			buflen -= sizeof(struct kinfo_proc);
		}
		needed += sizeof(struct kinfo_proc);
	}
	pd++;
	if (pd->pd_list != NULL)
		goto again;
	proclist_unlock_read();

	if (where != NULL) {
		*sizep = (caddr_t)dp - where;
		if (needed > *sizep)
			return (ENOMEM);
	} else {
		needed += KERN_PROCSLOP;
		*sizep = needed;
	}
	return (0);
}

/*
 * Fill in an eproc structure for the specified process.
 */
void
fill_eproc(p, ep)
	register struct proc *p;
	register struct eproc *ep;
{
	register struct tty *tp;

	ep->e_paddr = p;
	ep->e_sess = p->p_pgrp->pg_session;
	ep->e_pcred = *p->p_cred;
	ep->e_ucred = *p->p_ucred;
	if (p->p_stat == SIDL || P_ZOMBIE(p)) {
		ep->e_vm.vm_rssize = 0;
		ep->e_vm.vm_tsize = 0;
		ep->e_vm.vm_dsize = 0;
		ep->e_vm.vm_ssize = 0;
		/* ep->e_vm.vm_pmap = XXX; */
	} else {
		register struct vmspace *vm = p->p_vmspace;

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
