/*	$NetBSD: netbsd32_sysctl.c,v 1.3 2001/05/30 11:37:29 mrg Exp $	*/

/*
 * Copyright (c) 1998, 2001 Matthew R. Green
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(_KERNEL_OPT)
#include "opt_ddb.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/syscallargs.h>
#include <sys/proc.h>
#define	__SYSCTL_PRIVATE
#include <sys/sysctl.h>

#include <uvm/uvm_extern.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

#if defined(DDB)
#include <ddb/ddbvar.h>
#endif

int uvm_sysctl32(int *, u_int, void *, size_t *, void *, size_t, struct proc *);
int kern_sysctl32(int *, u_int, void *, size_t *, void *, size_t, struct proc *);

/*
 * uvm_sysctl32: sysctl hook into UVM system, handling special 32-bit
 * sensitive calls.
 */
int
uvm_sysctl32(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	struct netbsd32_loadavg av32;

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case VM_LOADAVG:
		netbsd32_from_loadavg(&av32, &averunnable);
		return (sysctl_rdstruct(oldp, oldlenp, newp, &av32,
		    sizeof(av32)));

	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

/*
 * kern_sysctl32: sysctl hook into KERN system, handling special 32-bit
 * sensitive calls.
 */
int
kern_sysctl32(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	struct netbsd32_timeval bt32;

	/* All sysctl names at this level, except for a few, are terminal. */
	switch (name[0]) {
#if 0
	case KERN_PROC:
	case KERN_PROC2:
	case KERN_PROF:
	case KERN_MBUF:
	case KERN_PROC_ARGS:
	case KERN_SYSVIPC_INFO:
		/* Not terminal. */
		break;
#endif
	default:
		if (namelen != 1)
			return (ENOTDIR);	/* overloaded */
	}

	switch (name[0]) {
	case KERN_BOOTTIME:
		netbsd32_from_timeval(&boottime, &bt32);
		return (sysctl_rdstruct(oldp, oldlenp, newp, &bt32,
		    sizeof(struct netbsd32_timeval)));

	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

int
netbsd32___sysctl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32___sysctl_args /* {
		syscallarg(netbsd32_intp) name;
		syscallarg(u_int) namelen;
		syscallarg(netbsd32_voidp) old;
		syscallarg(netbsd32_size_tp) oldlenp;
		syscallarg(netbsd32_voidp) new;
		syscallarg(netbsd32_size_t) newlen;
	} */ *uap = v;
	int error;
	netbsd32_size_t savelen = 0;
	size_t oldlen = 0;
	sysctlfn *fn;
	int name[CTL_MAXNAME];

/*
 * Some of these sysctl functions do their own copyin/copyout.
 * We need to disable or emulate the ones that need their
 * arguments converted.
 */

	if (SCARG(uap, new) != NULL &&
	    (error = suser(p->p_ucred, &p->p_acflag)))
		return (error);
	/*
	 * all top-level sysctl names are non-terminal
	 */
	if (SCARG(uap, namelen) > CTL_MAXNAME || SCARG(uap, namelen) < 2)
		return (EINVAL);
	error = copyin((caddr_t)(u_long)SCARG(uap, name), &name,
		       SCARG(uap, namelen) * sizeof(int));
	if (error)
		return (error);

	switch (name[0]) {
	case CTL_KERN:
		switch (name[1]) {
#if 0
		case KERN_FILE:
		case KERN_NTPTIME:
		case KERN_SYSVIPC_INFO:
#endif
		case KERN_BOOTTIME:
			fn = kern_sysctl32;
			break;
		default:
			fn = kern_sysctl;
			break;
		}
		break;
	case CTL_HW:
		fn = hw_sysctl;
		break;
	case CTL_VM:
		switch (name[1]) {
		case VM_LOADAVG:
			fn = uvm_sysctl32;	/* need to convert a `long' */
			break;
		default:
			fn = uvm_sysctl;
			break;
		}
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

	if (SCARG(uap, oldlenp) &&
	    (error = copyin((caddr_t)(u_long)SCARG(uap, oldlenp), &savelen,
	     sizeof(savelen))))
		return (error);
	if (SCARG(uap, old) != NULL) {
		error = lockmgr(&sysctl_memlock, LK_EXCLUSIVE, NULL);
		if (error)
			return (error);
		error = uvm_vslock(p, (void *)(u_long)SCARG(uap, old), savelen,
		    VM_PROT_READ|VM_PROT_WRITE);
		if (error) {
			(void) lockmgr(&sysctl_memlock, LK_RELEASE, NULL);
			return error;
		}
		oldlen = savelen;
	}
	error = (*fn)(name + 1, SCARG(uap, namelen) - 1, 
		      (void *)(u_long)SCARG(uap, old), &oldlen, 
		      (void *)(u_long)SCARG(uap, new), SCARG(uap, newlen), p);
	if (SCARG(uap, old) != NULL) {
		uvm_vsunlock(p, (void *)(u_long)SCARG(uap, old), savelen);
		(void) lockmgr(&sysctl_memlock, LK_RELEASE, NULL);
	}
	savelen = oldlen;
	if (error)
		return (error);
	if (SCARG(uap, oldlenp))
		error = copyout(&savelen,
		    (caddr_t)(u_long)SCARG(uap, oldlenp), sizeof(savelen));
	return (error);
}
