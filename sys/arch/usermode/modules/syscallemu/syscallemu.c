/* $NetBSD: syscallemu.c,v 1.1.4.2 2012/04/17 00:07:00 yamt Exp $ */

/*-
 * Copyright (c) 2012 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: syscallemu.c,v 1.1.4.2 2012/04/17 00:07:00 yamt Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/module.h>
#include <sys/atomic.h>
#include <sys/syscallvar.h>

#include "syscallemu.h"

#if !defined(__HAVE_SYSCALL_INTERN)
#error syscallemu requires __HAVE_SYSCALL_INTERN
#endif

static specificdata_key_t syscallemu_data_key;
static unsigned int syscallemu_refcnt;

static const struct syscall_package syscallemu_syscalls[] = {
	{ SYS_syscallemu,	0, (sy_call_t *)sys_syscallemu	},
	{ 0, 0, NULL },
};

struct syscallemu_data *
syscallemu_getsce(struct proc *p)
{
	return proc_getspecific(p, syscallemu_data_key);
}

void
syscallemu_setsce(struct proc *p, struct syscallemu_data *sce)
{
	proc_setspecific(p, syscallemu_data_key, sce);
}

/*
 * specificdata destructor
 */
static void
syscallemu_dtor(void *priv)
{
	struct syscallemu_data *sce = priv;

	kmem_free(sce, sizeof(*sce));
	atomic_dec_uint(&syscallemu_refcnt);
}

/*
 * Allocate private storage for the syscallemu parameters and stash it
 * in process specificdata. This can only be called once per process.
 *
 * Returns EINVAL if the specified start address falls after the end.
 * Returns EACCESS if syscallemu has already been configured for this process.
 */
int
sys_syscallemu(lwp_t *l, const struct sys_syscallemu_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(uintptr_t) user_start;
		syscallarg(uintptr_t) user_end;
	} */
	vaddr_t user_start = (vaddr_t)SCARG(uap, user_start);
	vaddr_t user_end = (vaddr_t)SCARG(uap, user_end);
	struct syscallemu_data *sce;
	struct proc *p = l->l_proc;

	if (syscallemu_getsce(p) != NULL)
		return EACCES;
	if (user_start >= user_end)
		return EINVAL;

	sce = kmem_alloc(sizeof(*sce), KM_SLEEP);
	sce->sce_user_start = user_start;
	sce->sce_user_end = user_end;
	sce->sce_md_syscall = md_syscallemu(p);
	KASSERT(sce->sce_md_syscall != NULL);

	atomic_inc_uint(&syscallemu_refcnt);
	syscallemu_setsce(p, sce);

#ifdef DEBUG
	printf("syscallemu: enabled for pid %d\n", p->p_pid);
#endif

	return 0;
}

/*
 * Initialize the syscallemu module
 */
static int
syscallemu_init(void)
{
	int error;

	syscallemu_refcnt = 0;

	/* XXX workaround for kern/45781 */
	if (emul_netbsd.e_sysent[SYS_syscallemu].sy_call == sys_nosys) {
		printf("syscallemu: applying workaround for kern/45781\n");
		emul_netbsd.e_sysent[SYS_syscallemu].sy_call = sys_nomodule;
	}
	emul_netbsd.e_sysent[SYS_syscallemu].sy_narg =
	    sizeof(struct sys_syscallemu_args) / sizeof(register_t);
	emul_netbsd.e_sysent[SYS_syscallemu].sy_argsize =
	    sizeof(struct sys_syscallemu_args);

	error = proc_specific_key_create(&syscallemu_data_key, syscallemu_dtor);
	if (error) {
		printf("syscallemu: couldn't create proc specific key (%d)\n",
		    error);
		return error;
	}

	error = syscall_establish(NULL, syscallemu_syscalls);
	if (error) {
		printf("syscallemu: couldn't establish syscalls\n");
		proc_specific_key_delete(syscallemu_data_key);
		return ENXIO;
	}

	return 0;
}

/*
 * Finalize the syscallemu module
 */
static int
syscallemu_fini(void)
{
	if (syscallemu_refcnt > 0)
		return EBUSY;

	syscall_disestablish(NULL, syscallemu_syscalls);
	proc_specific_key_delete(syscallemu_data_key);
	return 0;
}

/*
 * Module glue
 */
MODULE(MODULE_CLASS_MISC, syscallemu, NULL);

static int
syscallemu_modcmd(modcmd_t cmd, void *arg)
{
	switch (cmd) {
	case MODULE_CMD_INIT:
		return syscallemu_init();	
	case MODULE_CMD_FINI:
		return syscallemu_fini();
	case MODULE_CMD_AUTOUNLOAD:
		return EBUSY;
	default:
		return ENOTTY;
	}
}
