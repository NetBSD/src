/*	$NetBSD: linux_sysctl.c,v 1.4 2002/04/02 20:23:44 jdolecek Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_sysctl.c,v 1.4 2002/04/02 20:23:44 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/sysctl.h>
#include <sys/syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>

#include <compat/linux/linux_syscallargs.h>
#include <compat/linux/common/linux_sysctl.h>
#include <compat/linux/common/linux_exec.h>

int linux_kern_sysctl(int *, u_int, void *, size_t *, void *, size_t,
    struct proc *);
int linux_vm_sysctl(int *, u_int, void *, size_t *, void *, size_t,
    struct proc *);
int linux_net_sysctl(int *, u_int, void *, size_t *, void *, size_t,
    struct proc *);
int linux_proc_sysctl(int *, u_int, void *, size_t *, void *, size_t,
    struct proc *);
int linux_fs_sysctl(int *, u_int, void *, size_t *, void *, size_t,
    struct proc *);
#ifdef DEBUG
int linux_debug_sysctl(int *, u_int, void *, size_t *, void *, size_t,
    struct proc *);
#endif
int linux_dev_sysctl(int *, u_int, void *, size_t *, void *, size_t,
    struct proc *);
int linux_bus_sysctl(int *, u_int, void *, size_t *, void *, size_t,
    struct proc *);

int
linux_sys___sysctl(struct proc *p, void *v, register_t *retval)
{
	struct linux_sys___sysctl_args /* {
		syscallarg(struct linux___sysctl *) lsp;
	} */ *uap = v;
	struct linux___sysctl ls;
	int error;
	size_t savelen = 0, oldlen = 0;
	sysctlfn *fn;
	int name[CTL_MAXNAME];
	size_t *oldlenp;


	if ((error = copyin(SCARG(uap, lsp), &ls, sizeof ls)))
		return error;
	/*
	 * all top-level sysctl names are non-terminal
	 */
	if (ls.nlen > CTL_MAXNAME || ls.nlen < 2)
		return (EINVAL);
	error = copyin(ls.name, &name, ls.nlen * sizeof(int));
	if (error)
		return (error);

	/*
	 * For all but CTL_PROC, must be root to change a value.
	 * For CTL_PROC, must be root, or owner of the proc (and not suid),
	 * this is checked in proc_sysctl() (once we know the targer proc).
	 */
	if (ls.newval != NULL && name[0] != CTL_PROC &&
		    (error = suser(p->p_ucred, &p->p_acflag)))
			return error;

	switch (name[0]) {
	case LINUX_CTL_KERN:
		fn = linux_kern_sysctl;
		break;
	case LINUX_CTL_VM:
		fn = linux_vm_sysctl;
		break;
	case LINUX_CTL_NET:
		fn = linux_net_sysctl;
		break;
	case LINUX_CTL_PROC:
		fn = linux_proc_sysctl;
		break;
	case LINUX_CTL_FS:
		fn = linux_fs_sysctl;
		break;
#ifdef DEBUG
	case LINUX_CTL_DEBUG:
		fn = linux_debug_sysctl;
		break;
#endif
	case LINUX_CTL_DEV:
		fn = linux_dev_sysctl;
		break;
	case LINUX_CTL_BUS:
		fn = linux_bus_sysctl;
		break;
	default:
		return (EOPNOTSUPP);
	}

	/*
	 * XXX Hey, we wire `oldval', but what about `newval'?
	 */
	oldlenp = ls.oldlenp;
	if (oldlenp) {
		if ((error = copyin(oldlenp, &oldlen, sizeof(oldlen))))
			return (error);
		oldlenp = &oldlen;
	}
	if (ls.oldval != NULL) {
		error = uvm_vslock(p, ls.oldval, oldlen,
		    VM_PROT_READ|VM_PROT_WRITE);
		savelen = oldlen;
	}
	error = (*fn)(name + 1, ls.nlen - 1, ls.oldval, oldlenp, ls.newval,
	    ls.newlen, p);
	if (ls.oldval != NULL)
		uvm_vsunlock(p, ls.oldval, savelen);
	if (error)
		return (error);
	if (ls.oldlenp)
		error = copyout(&oldlen, ls.oldlenp, sizeof(oldlen));
	return (error);
}

/*
 * NOTE: DO NOT CHANGE THIS
 * Linux makes assumptions about specific features being present with
 * more recent kernels. Specifically, LinuxThreads use RT queued
 * signals if the kernel release is bigger. Since we don't support them
 * yet, the version needs to stay this way until we'd have the RT queued
 * signals implemented.
 */
char linux_sysname[128] = "Linux";
char linux_release[128] = "2.0.38";
char linux_version[128] = "#0 Sun Nov 11 11:11:11 MET 2000";

/*
 * kernel related system variables.
 */
int
linux_kern_sysctl(int *name, u_int nlen, void *oldp, size_t *oldlenp,
    void *newp, size_t newlen, struct proc *p)
{
	/*
	 * Note that we allow writing into this, so that userland
	 * programs can setup things as they see fit. This is suboptimal.
	 */
	switch (name[0]) {
	case LINUX_KERN_OSTYPE:
		return sysctl_string(oldp, oldlenp, newp, newlen,
		    linux_sysname, sizeof(linux_sysname));
	case LINUX_KERN_OSRELEASE:
		return sysctl_string(oldp, oldlenp, newp, newlen,
		    linux_release, sizeof(linux_release));
	case LINUX_KERN_VERSION:
		return sysctl_string(oldp, oldlenp, newp, newlen,
		    linux_version, sizeof(linux_version));
	default:
		return EOPNOTSUPP;
	}
}

/*
 * kernel related system variables.
 */
int
linux_sysctl(int *name, u_int nlen, void *oldp, size_t *oldlenp,
    void *newp, size_t newlen, struct proc *p)
{
	if (nlen != 2 || name[0] != EMUL_LINUX_KERN)
		return EOPNOTSUPP;

	/*
	 * Note that we allow writing into this, so that userland
	 * programs can setup things as they see fit. This is suboptimal.
	 */
	switch (name[1]) {
	case EMUL_LINUX_KERN_OSTYPE:
		return sysctl_string(oldp, oldlenp, newp, newlen,
		    linux_sysname, sizeof(linux_sysname));
	case EMUL_LINUX_KERN_OSRELEASE:
		return sysctl_string(oldp, oldlenp, newp, newlen,
		    linux_release, sizeof(linux_release));
	case EMUL_LINUX_KERN_VERSION:
		return sysctl_string(oldp, oldlenp, newp, newlen,
		    linux_version, sizeof(linux_version));
	default:
		return EOPNOTSUPP;
	}
}

/*
 * hardware related system variables.
 */
int
linux_vm_sysctl(int *name, u_int nlen, void *oldp, size_t *oldlenp,
    void *newp, size_t newlen, struct proc *p)
{
	return (EOPNOTSUPP);
}

int
linux_net_sysctl(int *name, u_int nlen, void *oldp, size_t *oldlenp,
    void *newp, size_t newlen, struct proc *p)
{
	return (EOPNOTSUPP);
}

int
linux_proc_sysctl(int *name, u_int nlen, void *oldp, size_t *oldlenp,
    void *newp, size_t newlen, struct proc *p)
{
	return (EOPNOTSUPP);
}

int
linux_fs_sysctl(int *name, u_int nlen, void *oldp, size_t *oldlenp,
    void *newp, size_t newlen, struct proc *p)
{
	return (EOPNOTSUPP);
}
#ifdef DEBUG
int
linux_debug_sysctl(int *name, u_int nlen, void *oldp, size_t *oldlenp,
    void *newp, size_t newlen, struct proc *p)
{
	return (EOPNOTSUPP);
}
#endif /* DEBUG */

int
linux_dev_sysctl(int *name, u_int nlen, void *oldp, size_t *oldlenp,
    void *newp, size_t newlen, struct proc *p)
{
	return (EOPNOTSUPP);
}

int
linux_bus_sysctl(int *name, u_int nlen, void *oldp, size_t *oldlenp,
    void *newp, size_t newlen, struct proc *p)
{
	return (EOPNOTSUPP);
}
