/*	$NetBSD: darwin_sysctl.c,v 1.1 2002/11/23 02:18:54 manu Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: darwin_sysctl.c,v 1.1 2002/11/23 02:18:54 manu Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/mount.h> 
#include <sys/proc.h> 
#include <sys/sysctl.h> 

#include <sys/syscallargs.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_vm.h>

#include <compat/freebsd/freebsd_syscallargs.h>

#include <compat/darwin/darwin_sysctl.h>
#include <compat/darwin/darwin_syscallargs.h>

int darwin_kern_sysctl
    __P((int *, u_int, void *, size_t *, void *, size_t, struct proc *));
int darwin_vm_sysctl
    __P((int *, u_int, void *, size_t *, void *, size_t, struct proc *));
int darwin_vfs_sysctl
    __P((int *, u_int, void *, size_t *, void *, size_t, struct proc *));
int darwin_net_sysctl
    __P((int *, u_int, void *, size_t *, void *, size_t, struct proc *));
int darwin_debug_sysctl
    __P((int *, u_int, void *, size_t *, void *, size_t, struct proc *));
int darwin_hw_sysctl
    __P((int *, u_int, void *, size_t *, void *, size_t, struct proc *));
int darwin_machdep_sysctl
    __P((int *, u_int, void *, size_t *, void *, size_t, struct proc *));
int darwin_user_sysctl
    __P((int *, u_int, void *, size_t *, void *, size_t, struct proc *));

int
darwin_sys___sysctl(struct proc *p, void *v, register_t *retval)
{
	struct darwin_sys___sysctl_args /* {
		syscallarg(int *) name;
		syscallarg(u_int) namelen;
		syscallarg(void *) oldp;
		syscallarg(size_t *) oldlenp;
		syscallarg(void *) newp;
		syscallarg(size_t) newlen;
	} */ *uap = v;
	int error;
	sysctlfn *fn;
	int name[CTL_MAXNAME];
	size_t oldlen;
	size_t savelen;
	int *namep = SCARG(uap, name);
	int namelen = SCARG(uap, namelen);
	void *oldp = SCARG(uap, oldp);
	size_t *oldlenp = SCARG(uap, oldlenp);
	void *newp = SCARG(uap, newp);
	size_t newlen = SCARG(uap, newlen);

	/*
	 * all top-level sysctl names are non-terminal
	 */
	if (namelen > CTL_MAXNAME || namelen < 2)
		return EINVAL;
	if ((error = copyin(namep, &name, namelen * sizeof(int))) != 0)
		return error;

#ifdef DEBUG_DARWIN
	{
		int i;

		DPRINTF(("darwin_sys___sysctl: name = [ "));
		for (i = 0; i < namelen; i++)
			DPRINTF(("%d, ", name[i]));
		DPRINTF(("]\n"));
	}
#endif /* DEBUG_DARWIN */
	/* 
	 * Need to be root to change a value
	 */
	if ((newp != NULL) && (error = suser(p->p_ucred, &p->p_acflag)))
		return error;

	switch (name[0]) {
	case DARWIN_CTL_KERN:
		fn = darwin_kern_sysctl;
		break;
	case DARWIN_CTL_VM:
		fn = darwin_vm_sysctl;
		break;
	case DARWIN_CTL_VFS:
		fn = darwin_vfs_sysctl;
		break;
	case DARWIN_CTL_NET:
		fn = darwin_net_sysctl;
		break;
	case DARWIN_CTL_DEBUG:
		fn = darwin_debug_sysctl;
		break;
	case DARWIN_CTL_HW:
		fn = darwin_hw_sysctl;
		break;
	case DARWIN_CTL_MACHDEP:
		fn = darwin_machdep_sysctl;
		break;
	case DARWIN_CTL_USER:
		fn = darwin_user_sysctl;
		break;
	default:
		return EOPNOTSUPP;
	}

	if (oldlenp) {
		if ((error = copyin(oldlenp, &oldlen, sizeof(oldlen))) != 0)
			return error;
		oldlenp = &oldlen;
	}

	if (oldp) {
		error = uvm_vslock(p, oldp, oldlen, 
		    VM_PROT_READ|VM_PROT_WRITE);
		savelen = oldlen;
	}

	error = (*fn)(name + 1, namelen - 1, oldp, oldlenp, newp, newlen, p);

	if (oldp)
		uvm_vsunlock(p, oldp, savelen);

	if (error)
		return error;

	if (oldlenp)
		error = copyout(&oldlen, SCARG(uap, oldlenp), sizeof(oldlen));

	return error;
}


int
darwin_kern_sysctl(name, nlen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int nlen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	switch (name[0]) {
	default:
		return EOPNOTSUPP;
	}

	/* NOTREACHED */
	return 0;
}

int
darwin_vm_sysctl(name, nlen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int nlen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	switch (name[0]) {
	default:
		return EOPNOTSUPP;
	}

	/* NOTREACHED */
	return 0;
}

int
darwin_vfs_sysctl(name, nlen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int nlen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	switch (name[0]) {
	default:
		return EOPNOTSUPP;
	}

	/* NOTREACHED */
	return 0;
}

int
darwin_net_sysctl(name, nlen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int nlen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	switch (name[0]) {
	default:
		return EOPNOTSUPP;
	}

	/* NOTREACHED */
	return 0;
}

int
darwin_debug_sysctl(name, nlen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int nlen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	switch (name[0]) {
	default:
		return EOPNOTSUPP;
	}

	/* NOTREACHED */
	return 0;
}

int
darwin_hw_sysctl(name, nlen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int nlen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	switch (name[0]) {
	case DARWIN_HW_NCPU:
		name[0] = HW_NCPU;
		return hw_sysctl(name, 1, oldp, oldlenp, newp, newlen, p);
		break;
	case DARWIN_HW_VECTORUNIT:
		return sysctl_rdint(oldp, oldlenp, newp, 0);
		break;
	default:
		return EOPNOTSUPP;
	}

	/* NOTREACHED */
	return 0;
}

int
darwin_machdep_sysctl(name, nlen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int nlen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	switch (name[0]) {
	default:
		return EOPNOTSUPP;
	}

	/* NOTREACHED */
	return 0;
}

int
darwin_user_sysctl(name, nlen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int nlen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	switch (name[0]) {
	default:
		return EOPNOTSUPP;
	}

	/* NOTREACHED */
	return 0;
}

