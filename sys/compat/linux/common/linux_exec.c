/*	$NetBSD: linux_exec.c,v 1.118.2.1 2018/05/21 04:36:03 pgoyette Exp $	*/

/*-
 * Copyright (c) 1994, 1995, 1998, 2000, 2007, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas, Frank van der Linden, Eric Haszlakiewicz and
 * Thor Lancelot Simon.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_exec.c,v 1.118.2.1 2018/05/21 04:36:03 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

#include <sys/mman.h>
#include <sys/syscallargs.h>

#include <sys/ptrace.h>	/* For proc_reparent() */

#include <uvm/uvm_extern.h>

#include <sys/cpu.h>
#include <machine/reg.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_sched.h>
#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/common/linux_exec.h>
#include <compat/linux/common/linux_futex.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>

#include <compat/linux/linux_syscallargs.h>
#include <compat/linux/linux_syscall.h>
#include <compat/linux/common/linux_misc.h>
#include <compat/linux/common/linux_errno.h>
#include <compat/linux/common/linux_emuldata.h>

extern struct sysent linux_sysent[];
extern const char * const linux_syscallnames[];
extern char linux_sigcode[], linux_esigcode[];

/*
 * Emulation switch.
 */

struct uvm_object *emul_linux_object;

struct emul emul_linux = {
	.e_name =		"linux",
	.e_path =		"/emul/linux",
#ifndef __HAVE_MINIMAL_EMUL
	.e_flags =		0,
	.e_errno =		native_to_linux_errno,
	.e_nosys =		LINUX_SYS_syscall,
	.e_nsysent =		LINUX_SYS_NSYSENT,
#endif
	.e_sysent =		linux_sysent,
	.e_syscallnames =	linux_syscallnames,
	.e_sendsig =		linux_sendsig,
	.e_trapsignal =		linux_trapsignal,
	.e_sigcode =		linux_sigcode,
	.e_esigcode =		linux_esigcode,
	.e_sigobject =		&emul_linux_object,
	.e_setregs =		linux_setregs,
	.e_proc_exec =		linux_e_proc_exec,
	.e_proc_fork =		linux_e_proc_fork,
	.e_proc_exit =		linux_e_proc_exit,
	.e_lwp_fork =		linux_e_lwp_fork,
	.e_lwp_exit =		linux_e_lwp_exit,
#ifdef __HAVE_SYSCALL_INTERN
	.e_syscall_intern =	linux_syscall_intern,
#else
#error Implement __HAVE_SYSCALL_INTERN for this platform
#endif
	.e_sysctlovly =		NULL,
	.e_vm_default_addr =	uvm_default_mapaddr,
	.e_usertrap =		linux_usertrap,
	.e_ucsize =		0,
	.e_startlwp =		NULL
};

void
linux_e_proc_exec(struct proc *p, struct exec_package *epp)
{
	struct lwp *l;

	l = LIST_FIRST(&p->p_lwps);
	if (l->l_emuldata == NULL) {
		l->l_emuldata = kmem_zalloc(sizeof(struct linux_emuldata), KM_SLEEP);
	} else {
		memset(l->l_emuldata, 0, sizeof (struct linux_emuldata));
	}

	KASSERT(p->p_nlwps == 1);
	l = LIST_FIRST(&p->p_lwps);
	mutex_enter(p->p_lock);
	l->l_lid = p->p_pid;
	mutex_exit(p->p_lock);
}

void
linux_e_proc_exit(struct proc *p)
{
	struct lwp *l;

	KASSERT(p->p_nlwps == 1);
	l = LIST_FIRST(&p->p_lwps);
	linux_e_lwp_exit(l);
}

void
linux_e_proc_fork(struct proc *p2, struct lwp *l1, int flags)
{
	struct linux_emuldata *led1, *led2;
	struct lwp *l2;

	KASSERT(p2->p_nlwps == 1);
	l2 = LIST_FIRST(&p2->p_lwps);
	l2->l_lid = p2->p_pid;
	led1 = l1->l_emuldata;
	led2 = l2->l_emuldata;
	led2->led_child_tidptr = led1->led_child_tidptr;
}

void
linux_e_lwp_fork(struct lwp *l1, struct lwp *l2)
{
	struct linux_emuldata *led2;

	led2 = kmem_zalloc(sizeof(*led2), KM_SLEEP);
	l2->l_emuldata = led2;
}

void
linux_e_lwp_exit(struct lwp *l)
{
	struct linux_emuldata *led;
	struct linux_sys_futex_args cup;
	register_t retval;
	int error, zero = 0;

	led = l->l_emuldata;
	if (led->led_clear_tid == NULL) {
		return;
	}

	/* Emulate LINUX_CLONE_CHILD_CLEARTID */
	error = copyout(&zero, led->led_clear_tid, sizeof(zero));
#ifdef DEBUG_LINUX
	if (error != 0)
		printf("%s: cannot clear TID\n", __func__);
#endif

	SCARG(&cup, uaddr) = led->led_clear_tid;
	SCARG(&cup, op) = LINUX_FUTEX_WAKE;
	SCARG(&cup, val) = 0x7fffffff; /* Awake everyone */
	SCARG(&cup, timeout) = NULL;
	SCARG(&cup, uaddr2) = NULL;
	SCARG(&cup, val3) = 0;
	if ((error = linux_sys_futex(curlwp, &cup, &retval)) != 0)
		printf("%s: linux_sys_futex failed\n", __func__);

	release_futexes(l);

	led = l->l_emuldata;
	l->l_emuldata = NULL;
	kmem_free(led, sizeof(*led));
}
