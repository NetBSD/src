/*	$NetBSD: irix_exec.c,v 1.58.4.1 2011/03/05 15:10:12 bouyer Exp $ */

/*-
 * Copyright (c) 2001-2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
__KERNEL_RCSID(0, "$NetBSD: irix_exec.c,v 1.58.4.1 2011/03/05 15:10:12 bouyer Exp $");

#ifdef _KERNEL_OPT
#include "opt_syscall_debug.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/rwlock.h>
#include <sys/exec.h>
#include <sys/types.h>
#include <sys/malloc.h>

#include <mips/locore.h>
#include <mips/regnum.h>

#include <uvm/uvm_extern.h>

#include <compat/irix/irix_syscall.h>
#include <compat/irix/irix_types.h>
#include <compat/irix/irix_exec.h>
#include <compat/irix/irix_prctl.h>
#include <compat/irix/irix_signal.h>
#include <compat/irix/irix_errno.h>
#include <compat/irix/irix_sysctl.h>
#include <compat/irix/irix_usema.h>

extern const int native_to_svr4_signo[];

static void irix_e_proc_exec(struct proc *, struct exec_package *);
static void irix_e_proc_fork(struct proc *, struct lwp *, int);
static void irix_e_proc_exit(struct proc *);
static void irix_e_proc_init(struct proc *);

extern struct sysent irix_sysent[];
extern const char * const irix_syscallnames[];

#ifndef __HAVE_SYSCALL_INTERN
void irix_syscall(void);
#else
void irix_syscall_intern(struct proc *);
#endif

/*
 * Fake sigcode. COMPAT_IRIX does not use it, since the
 * signal trampoline is provided by libc. However, some
 * other part of the kernel will be happier if we still
 * provide non NULL sigcode and esigcode.
 */
char irix_sigcode[] = { 0 };

struct emul emul_irix = {
	.e_name =		"irix",
	.e_path = 		"/emul/irix",
#ifndef __HAVE_MINIMAL_EMUL
	.e_flags =		0,
	.e_errno =		native_to_irix_errno,
	.e_nosys =		IRIX_SYS_syscall,
	.e_nsysent =		IRIX_SYS_NSYSENT,
#endif
	.e_sysent =		irix_sysent,
#ifdef SYSCALL_DEBUG
	.e_syscallnames =	irix_syscallnames,
#else
	.e_syscallnames =	NULL,
#endif
	.e_sendsig =		irix_sendsig,
	.e_trapsignal =		trapsignal,
	.e_tracesig =		NULL,
	.e_sigcode =		irix_sigcode,
	.e_esigcode =		irix_sigcode,
	.e_sigobject =		NULL,
	.e_setregs =		setregs,
	.e_proc_exec =		irix_e_proc_exec,
	.e_proc_fork =		irix_e_proc_fork,
	.e_proc_exit =		irix_e_proc_exit,
	.e_lwp_fork =		NULL,
	.e_lwp_exit =		NULL,
#ifdef __HAVE_SYSCALL_INTERN
	.e_syscall_intern =	irix_syscall_intern,
#else
	.e_syscall =		irix_syscall,
#endif
	.e_fault =		irix_vm_fault,

	.e_vm_default_addr =	uvm_default_mapaddr,
};

/*
 * set registers on exec for N32 applications
 */
void
irix_n32_setregs(struct lwp *l, struct exec_package *pack, vaddr_t stack)
{
	struct trapframe *tf = l->l_md.md_utf;

	/* Enable 64 bit instructions (eg: sd) */
	tf->tf_regs[_R_SR] |= MIPS3_SR_UX | MIPS3_SR_FR;
#ifdef _LP64
	tf->tf_regs[_R_SR] |= MIPS3_SR_KX;
#endif
}

/*
 * per-process emuldata allocation
 */
static void
irix_e_proc_init(struct proc *p)
{
	struct irix_emuldata *ied;
	struct vmspace *vmspace = p->p_vmspace;
	vaddr_t vm_min;
	vsize_t vm_len;

	if (!p->p_emuldata)
		p->p_emuldata = malloc(sizeof(struct irix_emuldata),
		    M_EMULDATA, M_WAITOK | M_ZERO);

	ied = p->p_emuldata;
	ied->ied_p = p;

	LIST_INIT(&ied->ied_shared_regions);
	vm_min = vm_map_min(&vmspace->vm_map);
	vm_len = vm_map_max(&vmspace->vm_map) - vm_min;
	irix_isrr_insert(vm_min, vm_len, IRIX_ISRR_SHARED, p);
}

/*
 * exec() hook used to allocate per process structures
 */
static void
irix_e_proc_exec(struct proc *p, struct exec_package *epp)
{
	int error;

	irix_e_proc_init(p);

	/* Initialize the process private area (PRDA) */
	error = irix_prda_init(p);
#ifdef DEBUG_IRIX
	if (error != 0)
		printf("irix_e_proc_exec(): PRDA map failed ");
#endif
}

/*
 * exit() hook used to free per process data structures
 */
static void
irix_e_proc_exit(struct proc *p)
{
	struct proc *pp;
	struct irix_emuldata *ied;
	struct irix_share_group *isg;
	struct irix_shared_regions_rec *isrr;

	/*
	 * Send SIGHUP to child process as requested using prctl(2)
	 */
	mutex_enter(proc_lock);
	PROCLIST_FOREACH(pp, &allproc) {
		/* Select IRIX processes */
		if (irix_check_exec(pp) == 0)
			continue;

		ied = (struct irix_emuldata *)(pp->p_emuldata);
		if (ied->ied_termchild && pp->p_pptr == p)
			psignal(pp, native_to_svr4_signo[SIGHUP]);
	}
	mutex_exit(proc_lock);

	/*
	 * Remove the process from share group processes list, if relevant.
	 */
	ied = (struct irix_emuldata *)(p->p_emuldata);

	if ((isg = ied->ied_share_group) != NULL) {
		rw_enter(&isg->isg_lock, RW_WRITER);
		LIST_REMOVE(ied, ied_sglist);
		isg->isg_refcount--;

		if (isg->isg_refcount == 0) {
			/*
		 	 * This was the last process in the share group.
			 * Call irix_usema_exit_cleanup() to free in-kernel
			 * structures hold by the share group through
			 * the irix_usync_cntl system call.
			 */
			irix_usema_exit_cleanup(p, NULL);
			 /*
			  * Free the share group structure (no need to free
			  * the lock since we destroy it now).
			  */
			rw_destroy(&isg->isg_lock);
			free(isg, M_EMULDATA);
			ied->ied_share_group = NULL;
		} else {
			/*
			 * There are other processes remaining in the share
			 * group. Call irix_usema_exit_cleanup() to set the
			 * first of them as the owner of the structures
			 * hold in the kernel by the share group.
			 */
			irix_usema_exit_cleanup(p,
			    LIST_FIRST(&isg->isg_head)->ied_p);
			rw_exit(&isg->isg_lock);
		}

	} else {
		/*
		 * The process is not part of a share group. Call
		 * irix_usema_exit_cleanup() to free in-kernel structures hold
		 * by the process through the irix_usync_cntl system call.
		 */
		irix_usema_exit_cleanup(p, NULL);
	}

	/* Free (un)shared region list */
	while (!LIST_EMPTY(&ied->ied_shared_regions)) {
		isrr = LIST_FIRST(&ied->ied_shared_regions);
		LIST_REMOVE(isrr , isrr_list);
		free(isrr, M_EMULDATA);
	}

	free(p->p_emuldata, M_EMULDATA);
	p->p_emuldata = NULL;
}

/*
 * fork() hook used to allocate per process structures
 */
static void
irix_e_proc_fork(struct proc *p2, struct lwp *l1, int forkflags)
{
	struct irix_emuldata *ied1;
	struct irix_emuldata *ied2;

        p2->p_emuldata = NULL;

        irix_e_proc_init(p2);

	ied1 = p2->p_emuldata;
	ied2 = l1->l_proc->p_emuldata;

	(void) memcpy(ied1, ied2, (unsigned)
	    ((char *)&ied1->ied_endcopy - (char *)&ied1->ied_startcopy));
}
