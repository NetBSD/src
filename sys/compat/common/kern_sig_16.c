/*	$NetBSD: kern_sig_16.c,v 1.2.38.2 2018/04/17 07:24:55 pgoyette Exp $	*/

/*-
 * Copyright (c) 2006, 2007, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
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
 *	@(#)kern_sig.c	8.14 (Berkeley) 5/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_sig_16.c,v 1.2.38.2 2018/04/17 07:24:55 pgoyette Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/rwlock.h>
#include <sys/signalvar.h>
#include <sys/proc.h>
#include <sys/pool.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#include <sys/syscallargs.h>
#include <sys/kauth.h>
#include <sys/wait.h>
#include <sys/kmem.h>

#include <uvm/uvm_object.h>
#include <uvm/uvm_prot.h>
#include <uvm/uvm_pager.h>

#include <compat/common/compat_mod.h>

extern krwlock_t exec_lock;

#if !defined(__amd64__) || defined(COMPAT_NETBSD32)
#define COMPAT_SIGCONTEXT
extern char sigcode[], esigcode[];
struct uvm_object *emul_netbsd_object;
#endif

static const struct syscall_package kern_sig_16_syscalls[] = {
#ifdef COMPAT_SIGCONTEXT
	{ SYS_compat_16___sigaction14, 0,
	    (sy_call_t *)compat_16_sys___sigaction14 },
	{ SYS_compat_16___sigreturn14, 0,
	    (sy_call_t *)compat_16_sys___sigreturn14 },
#endif
	{ 0, 0, NULL }
};

int
compat_16_sys___sigaction14(struct lwp *l,
    const struct compat_16_sys___sigaction14_args *uap, register_t *retval)
{
	/* {
		syscallarg(int)				signum;
		syscallarg(const struct sigaction *)	nsa;
		syscallarg(struct sigaction *)		osa;
	} */
	struct sigaction	nsa, osa;
	int			error;

	if (SCARG(uap, nsa)) {
		error = copyin(SCARG(uap, nsa), &nsa, sizeof(nsa));
		if (error)
			return (error);
	}
	error = sigaction1(l, SCARG(uap, signum),
	    SCARG(uap, nsa) ? &nsa : 0, SCARG(uap, osa) ? &osa : 0,
	    NULL, 0);
	if (error)
		return (error);
	if (SCARG(uap, osa)) {
		error = copyout(&osa, SCARG(uap, osa), sizeof(osa));
		if (error)
			return (error);
	}
	return (0);
}

int
kern_sig_16_init(void)
{
	int error;

	error = syscall_establish(NULL, kern_sig_16_syscalls);
	if (error)
		return error;
#if defined(COMPAT_SIGCONTEXT)
	KASSERT(emul_netbsd.e_sigobject == NULL);
	rw_enter(&exec_lock, RW_WRITER);
	emul_netbsd.e_sigcode = sigcode;
	emul_netbsd.e_esigcode = esigcode;
	emul_netbsd.e_sigobject = &emul_netbsd_object;
	rw_exit(&exec_lock);
	KASSERT(sendsig_sigcontext_vec == NULL);
	sendsig_sigcontext_vec = sendsig_sigcontext;
#endif

	return 0;
}

int
kern_sig_16_fini(void)
{
	proc_t *p;
	int error;

	error = syscall_disestablish(NULL, kern_sig_16_syscalls);
	if (error)
		return error;
	/*
	 * Ensure sendsig_sigcontext() is not being used.
	 * module_lock prevents the flag being set on any
	 * further processes while we are here.  See
	 * sigaction1() for the opposing half.
	 */
	mutex_enter(proc_lock);
	PROCLIST_FOREACH(p, &allproc) {
		if ((p->p_lflag & PL_SIGCOMPAT) != 0) {
			break;
		}
	}
	mutex_exit(proc_lock);
	if (p != NULL) {
		syscall_establish(NULL, kern_sig_16_syscalls);
		return EBUSY;
	}
	sendsig_sigcontext_vec = NULL;

#if defined(COMPAT_SIGCONTEXT)
	/*
	 * The sigobject may persist if still in use, but
	 * is reference counted so will die eventually.
	 */
	rw_enter(&exec_lock, RW_WRITER);
	if (emul_netbsd_object != NULL) {
		(*emul_netbsd_object->pgops->pgo_detach)(emul_netbsd_object);
	}
	emul_netbsd_object = NULL;
	emul_netbsd.e_sigcode = NULL;
	emul_netbsd.e_esigcode = NULL;
	emul_netbsd.e_sigobject = NULL;
	rw_exit(&exec_lock);
#endif
	return 0;
}

