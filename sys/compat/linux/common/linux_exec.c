/*	$NetBSD: linux_exec.c,v 1.77 2005/06/22 15:10:51 manu Exp $	*/

/*-
 * Copyright (c) 1994, 1995, 1998, 2000 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: linux_exec.c,v 1.77 2005/06/22 15:10:51 manu Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

#include <sys/mman.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/reg.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_exec.h>
#include <compat/linux/common/linux_machdep.h>

#include <compat/linux/linux_syscallargs.h>
#include <compat/linux/linux_syscall.h>
#include <compat/linux/common/linux_misc.h>
#include <compat/linux/common/linux_errno.h>
#include <compat/linux/common/linux_emuldata.h>

extern struct sysent linux_sysent[];
extern const char * const linux_syscallnames[];
extern char linux_sigcode[], linux_esigcode[];

static void linux_e_proc_exec __P((struct proc *, struct exec_package *));
static void linux_e_proc_fork __P((struct proc *, struct proc *, int));
static void linux_e_proc_exit __P((struct proc *));
static void linux_e_proc_init __P((struct proc *, struct proc *, int));

static void linux_userret_settid __P((struct lwp *, void *));

/*
 * Execve(2). Just check the alternate emulation path, and pass it on
 * to the NetBSD execve().
 */
int
linux_sys_execve(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_execve_args /* {
		syscallarg(const char *) path;
		syscallarg(char **) argv;
		syscallarg(char **) envp;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct sys_execve_args ap;
	caddr_t sg;

	sg = stackgap_init(p, 0);
	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	SCARG(&ap, path) = SCARG(uap, path);
	SCARG(&ap, argp) = SCARG(uap, argp);
	SCARG(&ap, envp) = SCARG(uap, envp);

	return sys_execve(l, &ap, retval);
}

/*
 * Emulation switch.
 */

struct uvm_object *emul_linux_object;

const struct emul emul_linux = {
	"linux",
	"/emul/linux",
#ifndef __HAVE_MINIMAL_EMUL
	0,
	(const int *)native_to_linux_errno,
	LINUX_SYS_syscall,
	LINUX_SYS_NSYSENT,
#endif
	linux_sysent,
	linux_syscallnames,
	linux_sendsig,
	linux_trapsignal,
	NULL,
	linux_sigcode,
	linux_esigcode,
	&emul_linux_object,
	linux_setregs,
	linux_e_proc_exec,
	linux_e_proc_fork,
	linux_e_proc_exit,
	NULL,
	NULL,
#ifdef __HAVE_SYSCALL_INTERN
	linux_syscall_intern,
#else
#error Implement __HAVE_SYSCALL_INTERN for this platform
#endif
	NULL,
	NULL,

	uvm_default_mapaddr,

	linux_usertrap,
};

static void
linux_e_proc_init(p, parent, forkflags)
	struct proc *p, *parent;
	int forkflags;
{
	struct linux_emuldata *e = p->p_emuldata;
	struct linux_emuldata_shared *s;
	struct linux_emuldata *ep = NULL;

	if (!e) {
		/* allocate new Linux emuldata */
		MALLOC(e, void *, sizeof(struct linux_emuldata),
			M_EMULDATA, M_WAITOK);
	} else  {
		e->s->refs--;
		if (e->s->refs == 0)
			FREE(e->s, M_EMULDATA);
	}

	memset(e, '\0', sizeof(struct linux_emuldata));

	if (parent)
		ep = parent->p_emuldata;

	if (forkflags & FORK_SHAREVM) {
#ifdef DIAGNOSTIC
		if (ep == NULL) {
			killproc(p, "FORK_SHAREVM while emuldata is NULL\n");
			return;
		}
#endif
		s = ep->s;
		s->refs++;
	} else {
		struct vmspace *vm;

		MALLOC(s, void *, sizeof(struct linux_emuldata_shared),
			M_EMULDATA, M_WAITOK);
		s->refs = 1;

		/*
		 * Set the process idea of the break to the real value.
		 * For fork, we use parent's vmspace since our's
		 * is not setup at the time of this call and is going
		 * to be copy of parent's anyway. For exec, just
		 * use our own vmspace.
		 */
		vm = (parent) ? parent->p_vmspace : p->p_vmspace;
		s->p_break = vm->vm_daddr + ctob(vm->vm_dsize);

		/*
		 * Linux threads are emulated as NetBSD processes (not lwp)
		 * We use native PID for Linux TID. The Linux TID is the
		 * PID of the first process in the group. It is stored
		 * here
		 */
		s->group_pid = p->p_pid;
	}

	e->s = s;

	/* 
	 * initialize TID pointers. ep->child_clear_tid and 
	 * ep->child_set_tid will not be used beyond this point.
	 */
	e->child_clear_tid = NULL;
	e->child_set_tid = NULL;
	if (ep != NULL) {
		e->clear_tid = ep->child_clear_tid;
		e->set_tid = ep->child_set_tid;
		ep->child_clear_tid = NULL;
		ep->child_set_tid = NULL;
	} else {
		e->clear_tid = NULL;
		e->set_tid = NULL;
	}

	p->p_emuldata = e;
}

/*
 * Allocate new per-process structures. Called when executing Linux
 * process. We can reuse the old emuldata - if it's not null,
 * the executed process is of same emulation as original forked one.
 */
static void
linux_e_proc_exec(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	/* exec, use our vmspace */
	linux_e_proc_init(p, NULL, 0);
}

/*
 * Emulation per-process exit hook.
 */
static void
linux_e_proc_exit(p)
	struct proc *p;
{
	struct linux_emuldata *e = p->p_emuldata;

	/* Emulate LINUX_CLONE_CHILD_CLEARTID */
	if (e->clear_tid != NULL) {
		int error;
		int null = 0;

		if ((error = copyout(&null, 
		    e->clear_tid, 
		    sizeof(null))) != 0)
			printf("linux_e_proc_exit: cannot clear TID\n");

#ifdef notyet /* Not yet implemented */
		if ((error = linux_sys_futex(e->clear_tid, 
		    LINUX_FUTEX_WAKE, 1, NULL, NULL, 0)) != 0)
			printf("linux_e_proc_exit: linux_sys_futex failed\n");
#endif
	}

	/* free Linux emuldata and set the pointer to null */
	e->s->refs--;
	if (e->s->refs == 0)
		FREE(e->s, M_EMULDATA);
	FREE(e, M_EMULDATA);
	p->p_emuldata = NULL;
}

/*
 * Emulation fork hook.
 */
static void
linux_e_proc_fork(p, parent, forkflags)
	struct proc *p, *parent;
	int forkflags;
{
	struct linux_emuldata *e;

	/*
	 * The new process might share some vmspace-related stuff
	 * with parent, depending on fork flags (CLONE_VM et.al).
	 * Force allocation of new base emuldata, and share the
	 * VM-related parts only if necessary.
	 */
	p->p_emuldata = NULL;
	linux_e_proc_init(p, parent, forkflags);

	/* 
	 * Emulate LINUX_CLONE_CHILD_SETTID: This cannot be done  
	 * right now because the child VM is not set up. We will
	 * do it at userret time.
	 */
	e = p->p_emuldata;
	if (e->set_tid != NULL) 
		p->p_userret = (*linux_userret_settid);

	return;
}

static void
linux_userret_settid(l, arg)
	struct lwp *l;
	void *arg;
{
	struct proc *p = l->l_proc;
	struct linux_emuldata *led = p->p_emuldata;
	int error;

	p->p_userret = NULL;

	/* Emulate LINUX_CLONE_CHILD_SETTID  */
	if (led->set_tid != NULL) {
		if ((error = copyout(&p->p_pid,
		    led->set_tid, sizeof(p->p_pid))) != 0)
			printf("linux_userret_settid: cannot set TID\n");
	}

	return;	
}
