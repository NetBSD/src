/*	$NetBSD: linux_exec.c,v 1.98.2.2 2007/12/26 21:38:59 ad Exp $	*/

/*-
 * Copyright (c) 1994, 1995, 1998, 2000, 2007 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: linux_exec.c,v 1.98.2.2 2007/12/26 21:38:59 ad Exp $");

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

static void linux_e_proc_exec(struct proc *, struct exec_package *);
static void linux_e_proc_fork(struct proc *, struct proc *, int);
static void linux_e_proc_exit(struct proc *);
static void linux_e_proc_init(struct proc *, struct proc *, int);

#ifdef LINUX_NPTL
void linux_userret(void);
#endif

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
	0,
	NULL,		/* e_startlwp */
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

	e->proc = p;

	if (parent)
		ep = parent->p_emuldata;

	if (forkflags & FORK_SHAREVM) {
#ifdef DIAGNOSTIC
		if (ep == NULL) {
			killproc(p, "FORK_SHAREVM while emuldata is NULL\n");
			FREE(e, M_EMULDATA);
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
		s->p_break = (char *)vm->vm_daddr + ctob(vm->vm_dsize);

		/*
		 * Linux threads are emulated as NetBSD processes (not lwp)
		 * We use native PID for Linux TID. The Linux TID is the
		 * PID of the first process in the group. It is stored
		 * here
		 */
		s->group_pid = p->p_pid;

		/*
		 * Initialize the list of threads in the group
		 */
		LIST_INIT(&s->threads);

		s->xstat = 0;
		s->flags = 0;
	}

	e->s = s;

	/*
	 * Add this thread in the group thread list
	 */
	LIST_INSERT_HEAD(&s->threads, e, threads);

#ifdef LINUX_NPTL
	if (ep != NULL) {
		e->parent_tidptr = ep->parent_tidptr;
		e->child_tidptr = ep->child_tidptr;
		e->clone_flags = ep->clone_flags;
	}
#endif /* LINUX_NPTL */

	p->p_emuldata = e;
}

/*
 * Allocate new per-process structures. Called when executing Linux
 * process. We can reuse the old emuldata - if it's not null,
 * the executed process is of same emulation as original forked one.
 */
static void
linux_e_proc_exec(struct proc *p, struct exec_package *epp)
{
	/* exec, use our vmspace */
	linux_e_proc_init(p, NULL, 0);
}

/*
 * Emulation per-process exit hook.
 */
static void
linux_e_proc_exit(struct proc *p)
{
	struct linux_emuldata *e = p->p_emuldata;

#ifdef LINUX_NPTL
	linux_nptl_proc_exit(p);
#endif
	/* Remove the thread for the group thread list */
	LIST_REMOVE(e, threads);

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
	/*
	 * The new process might share some vmspace-related stuff
	 * with parent, depending on fork flags (CLONE_VM et.al).
	 * Force allocation of new base emuldata, and share the
	 * VM-related parts only if necessary.
	 */
	p->p_emuldata = NULL;
	linux_e_proc_init(p, parent, forkflags);

#ifdef LINUX_NPTL
	linux_nptl_proc_fork(p, parent, linux_userret);
#endif

	return;
}

#ifdef LINUX_NPTL
void
linux_userret(void)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct linux_emuldata *led = p->p_emuldata;
	int error;

	/* LINUX_CLONE_CHILD_SETTID: copy child's TID to child's memory  */
	if (led->clone_flags & LINUX_CLONE_CHILD_SETTID) {
		if ((error = copyout(&l->l_proc->p_pid,
		    led->child_tidptr,  sizeof(l->l_proc->p_pid))) != 0)
			printf("linux_userret: LINUX_CLONE_CHILD_SETTID "
			    "failed (led->child_tidptr = %p, p->p_pid = %d)\n",
			    led->child_tidptr, p->p_pid);
	}

	/* LINUX_CLONE_SETTLS: allocate a new TLS */
	if (led->clone_flags & LINUX_CLONE_SETTLS) {
		if (linux_set_newtls(l, linux_get_newtls(l)) != 0)
			printf("linux_userret: linux_set_tls failed");
	}

	return;	
}

void
linux_nptl_proc_exit(struct proc *p)
{
	struct linux_emuldata *e = p->p_emuldata;

	mutex_enter(&proclist_lock);

	/* 
	 * Check if we are a thread group leader victim of another 
	 * thread doing exit_group(). If we are, change the exit code. 
	 */
	if ((e->s->group_pid == p->p_pid) &&
	    (e->s->flags & LINUX_LES_INEXITGROUP)) {
		p->p_xstat = e->s->xstat;
	}

	/*
	 * Members of the thread groups others than the leader should
	 * exit quietely: no zombie stage, no signal. We do that by
	 * reparenting to init. init will collect us and nobody will 
	 * notice what happened.
	 */
#ifdef DEBUG_LINUX
	printf("%s:%d e->s->group_pid = %d, p->p_pid = %d, flags = 0x%x\n", 
	    __func__, __LINE__, e->s->group_pid, p->p_pid, e->s->flags);
#endif
	if (e->s->group_pid != p->p_pid) {
		proc_reparent(p, initproc);	
		cv_broadcast(&initproc->p_waitcv);
	}

	mutex_exit(&proclist_lock);

	/* Emulate LINUX_CLONE_CHILD_CLEARTID */
	if (e->clear_tid != NULL) {
		int error;
		int null = 0;
		struct linux_sys_futex_args cup;
		register_t retval;

		error = copyout(&null, e->clear_tid, sizeof(null));
#ifdef DEBUG_LINUX
		if (error != 0)
			printf("%s: cannot clear TID\n", __func__);
#endif

		SCARG(&cup, uaddr) = e->clear_tid;
		SCARG(&cup, op) = LINUX_FUTEX_WAKE;
		SCARG(&cup, val) = 0x7fffffff; /* Awake everyone */
		SCARG(&cup, timeout) = NULL;
		SCARG(&cup, uaddr2) = NULL;
		SCARG(&cup, val3) = 0;
		if ((error = linux_sys_futex(curlwp, &cup, &retval)) != 0)
			printf("%s: linux_sys_futex failed\n", __func__);
	}

	return;
}

void
linux_nptl_proc_fork(p, parent, luserret)
	struct proc *p;
	struct proc *parent;
	void (*luserret)(void);
{
#ifdef LINUX_NPTL
	struct linux_emuldata *e;
#endif

	e = p->p_emuldata;

	/* LINUX_CLONE_CHILD_CLEARTID: clear TID in child's memory on exit() */
	if (e->clone_flags & LINUX_CLONE_CHILD_CLEARTID)
		e->clear_tid = e->child_tidptr;

	/* LINUX_CLONE_PARENT_SETTID: set child's TID in parent's memory */
	if (e->clone_flags & LINUX_CLONE_PARENT_SETTID) {
		if (copyout_proc(parent, &p->p_pid,
		    e->parent_tidptr,  sizeof(p->p_pid)) != 0)
			printf("%s: LINUX_CLONE_PARENT_SETTID "
			    "failed (e->parent_tidptr = %p, "
			    "parent->p_pid = %d, p->p_pid = %d)\n",
			    __func__, e->parent_tidptr, 
			    parent->p_pid, p->p_pid);
	}

	/* 
	 * CLONE_CHILD_SETTID and LINUX_CLONE_SETTLS require child's VM
	 * setup to be completed, we postpone them until userret time.
	 */
	if (e->clone_flags &
	    (LINUX_CLONE_CHILD_CLEARTID | LINUX_CLONE_SETTLS))
		p->p_userret = luserret;

	return;
}

void
linux_nptl_proc_init(struct proc *p, struct proc *parent)
{
	struct linux_emuldata *e = p->p_emuldata;
	struct linux_emuldata *ep;

	if ((parent != NULL) && (parent->p_emuldata != NULL)) {
		ep = parent->p_emuldata;

		e->parent_tidptr = ep->parent_tidptr;
		e->child_tidptr = ep->child_tidptr;
		e->clone_flags = ep->clone_flags;
	}

	return;
}


#endif /* LINUX_NPTL */
