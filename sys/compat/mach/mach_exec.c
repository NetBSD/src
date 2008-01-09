/*	$NetBSD: mach_exec.c,v 1.64.8.1 2008/01/09 01:51:25 matt Exp $	 */

/*-
 * Copyright (c) 2001-2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas and Emmanuel Dreyfus.
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
__KERNEL_RCSID(0, "$NetBSD: mach_exec.c,v 1.64.8.1 2008/01/09 01:51:25 matt Exp $");

#include "opt_syscall_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/queue.h>
#include <sys/exec_macho.h>
#include <sys/malloc.h>

#include <sys/syscall.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_param.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_port.h>
#include <compat/mach/mach_semaphore.h>
#include <compat/mach/mach_notify.h>
#include <compat/mach/mach_exec.h>

static int mach_cold = 1; /* Have we initialized COMPAT_MACH structures? */
static void mach_init(void);

extern struct sysent sysent[];
#ifdef SYSCALL_DEBUG
extern const char * const syscallnames[];
#endif
#ifndef __HAVE_SYSCALL_INTERN
void syscall(void);
#else
void mach_syscall_intern(struct proc *);
#endif

#ifdef COMPAT_16
extern char sigcode[], esigcode[];
struct uvm_object *emul_mach_object;
#endif

const struct emul emul_mach = {
	"mach",
	"/emul/mach",
#ifndef __HAVE_MINIMAL_EMUL
	0,
	0,
	SYS_syscall,
	SYS_NSYSENT,
#endif
	sysent,
#ifdef SYSCALL_DEBUG
	syscallnames,
#else
	NULL,
#endif
	sendsig,
	mach_trapsignal,
	NULL,
#ifdef COMPAT_16
	sigcode,
	esigcode,
	&emul_mach_object,
#else
	NULL,
	NULL,
	NULL,
#endif
	setregs,
	mach_e_proc_exec,
	mach_e_proc_fork,
	mach_e_proc_exit,
	mach_e_lwp_fork,
	mach_e_lwp_exit,
#ifdef __HAVE_SYSCALL_INTERN
	mach_syscall_intern,
#else
	syscall,
#endif
	NULL,	/* e_fault */
	NULL,	/* e_vm_default_addr */

	uvm_default_mapaddr,
	NULL,	/* e_usertrap */
	0,	/* e_ucsize */
	NULL,	/* e_startlwp */
};

/*
 * Copy arguments onto the stack in the normal way, but add some
 * extra information in case of dynamic binding.
 * XXX This needs a cleanup: it is not used anymore by the Darwin
 * emulation, and it probably contains Darwin specific bits.
 */
int
exec_mach_copyargs(struct lwp *l, struct exec_package *pack, struct ps_strings *arginfo, char **stackp, void *argp)
{
	struct exec_macho_emul_arg *emea;
	struct exec_macho_object_header *macho_hdr;
	size_t len;
	size_t zero = 0;
	int error;

	*stackp = (char *)(((unsigned long)*stackp - 1) & ~0xfUL);

	emea = (struct exec_macho_emul_arg *)pack->ep_emul_arg;
	macho_hdr = (struct exec_macho_object_header *)emea->macho_hdr;
	if ((error = copyout(&macho_hdr, *stackp, sizeof(macho_hdr))) != 0)
		return error;

	*stackp += sizeof(macho_hdr);

	if ((error = copyargs(l, pack, arginfo, stackp, argp)) != 0) {
		DPRINTF(("mach: copyargs failed\n"));
		return error;
	}

	if ((error = copyout(&zero, *stackp, sizeof(zero))) != 0)
		return error;
	*stackp += sizeof(zero);

	if ((error = copyoutstr(emea->filename,
	    *stackp, MAXPATHLEN, &len)) != 0) {
		DPRINTF(("mach: copyout path failed\n"));
		return error;
	}
	*stackp += len + 1;

	/* We don't need this anymore */
	free(pack->ep_emul_arg, M_TEMP);
	pack->ep_emul_arg = NULL;

	len = len % sizeof(zero);
	if (len) {
		if ((error = copyout(&zero, *stackp, len)) != 0)
			return error;
		*stackp += len;
	}

	if ((error = copyout(&zero, *stackp, sizeof(zero))) != 0)
		return error;
	*stackp += sizeof(zero);

	return 0;
}

int
exec_mach_probe(const char **path)
{
	*path = emul_mach.e_path;
	return 0;
}

void
mach_e_proc_exec(struct proc *p, struct exec_package *epp)
{
	mach_e_proc_init(p, p->p_vmspace);

	if (p->p_emul != epp->ep_esch->es_emul)
		mach_e_lwp_fork(NULL, proc_representative_lwp(p, NULL, 1));

	return;
}

void
mach_e_proc_fork(struct proc *p, struct proc *parent, int forkflags)
{
	mach_e_proc_fork1(p, parent, 1);
	return;
}

void
mach_e_proc_fork1(struct proc *p, struct proc *parent, int allocate)
{
	struct mach_emuldata *med1;
	struct mach_emuldata *med2;
	int i;

	/*
	 * For Darwin binaries, p->p_emuldata has already been
	 * allocated, no need to throw it away and allocate it again.
	 */
	if (allocate)
		p->p_emuldata = NULL;

	/* Use parent's vmspace because our vmspace may not be set up yet */
	mach_e_proc_init(p, parent->p_vmspace);

	med1 = p->p_emuldata;
	med2 = parent->p_emuldata;

	/*
	 * Exception ports are inherited between forks,
	 * but we need to double their reference counts,
	 * since the ports are referenced by rights in the
	 * parent and in the child.
	 *
	 * XXX we need to convert all the parent's rights
	 * to the child namespace. This will make the
	 * following fixup obsolete.
	 */
	for (i = 0; i <= MACH_EXC_MAX; i++) {
		med1->med_exc[i] = med2->med_exc[i];
		if (med1->med_exc[i] !=  NULL)
			med1->med_exc[i]->mp_refcount *= 2;
	}

	return;
}

void
mach_e_proc_init(struct proc *p, struct vmspace *vmspace)
{
	struct mach_emuldata *med;
	struct mach_right *mr;

	/*
	 * Initialize various things if needed.
	 * XXX Not the best place for this.
	 */
	if (mach_cold == 1)
		mach_init();

	/*
	 * For Darwin binaries, p->p_emuldata is always allocated:
	 * from the previous program if it had the same emulation,
	 * or from darwin_e_proc_exec(). In the latter situation,
	 * everything has been set to zero.
	 */
	if (!p->p_emuldata) {
#ifdef DIAGNOSTIC
		if (p->p_emul != &emul_mach)
			printf("mach_emuldata allocated for non Mach binary\n");
#endif
		p->p_emuldata = malloc(sizeof(struct mach_emuldata),
		    M_EMULDATA, M_WAITOK | M_ZERO);
	}

	med = (struct mach_emuldata *)p->p_emuldata;

	/*
	 * p->p_emudata has med_inited set if we inherited it from
	 * the program that called exec(). In that situation, we
	 * must free anything that will not be used anymore.
	 */
	if (med->med_inited != 0) {
		rw_enter(&med->med_rightlock, RW_WRITER);
		while ((mr = LIST_FIRST(&med->med_right)) != NULL)
			mach_right_put_exclocked(mr, MACH_PORT_TYPE_ALL_RIGHTS);
		rw_exit(&med->med_rightlock);

		/*
		 * Do not touch special ports. Some other process (eg: gdb)
		 * might have grabbed them to control the process, and the
		 * controller intend to keep in control even after exec().
		 */
	} else {
		/*
		 * p->p_emuldata is uninitialized. Go ahead and initialize it.
		 */
		LIST_INIT(&med->med_right);
		rw_init(&med->med_rightlock);
		rw_init(&med->med_exclock);

		/*
		 * For debugging purpose, it's convenient to have each process
		 * using distinct port names, so we prefix the first port name
		 * by the PID. Darwin does not do that, but we can remove it
		 * when we want, it will not hurt.
		 */
		med->med_nextright = p->p_pid << 16;

		/*
		 * Initialize special ports. Bootstrap port is shared
		 * among all Mach processes in our implementation.
		 */
		med->med_kernel = mach_port_get();
		med->med_host = mach_port_get();

		med->med_kernel->mp_flags |= MACH_MP_INKERNEL;
		med->med_host->mp_flags |= MACH_MP_INKERNEL;

		med->med_kernel->mp_data = (void *)p;
		med->med_host->mp_data = (void *)p;

		med->med_kernel->mp_datatype = MACH_MP_PROC;
		med->med_host->mp_datatype = MACH_MP_PROC;

		MACH_PORT_REF(med->med_kernel);
		MACH_PORT_REF(med->med_host);

		med->med_bootstrap = mach_bootstrap_port;
		MACH_PORT_REF(med->med_bootstrap);
	}

	/*
	 * Exception ports are inherited accross exec() calls.
	 * If the structure is initialized, the ports are just
	 * here, so leave them untouched. If the structure is
	 * uninitalized, the ports are all set to zero, which
	 * is the default, so do not touch them either.
	 */

	med->med_dirty_thid = 1;
	med->med_suspend = 0;
	med->med_inited = 1;

	return;
}

void
mach_e_proc_exit(struct proc *p)
{
	struct mach_emuldata *med;
	struct mach_right *mr;
	int i;

	/* There is only one lwp remaining... */
	mach_e_lwp_exit(proc_representative_lwp(p, NULL, 1));

	med = (struct mach_emuldata *)p->p_emuldata;

	rw_enter(&med->med_rightlock, RW_WRITER);
	while ((mr = LIST_FIRST(&med->med_right)) != NULL)
		mach_right_put_exclocked(mr, MACH_PORT_TYPE_ALL_RIGHTS);
	rw_exit(&med->med_rightlock);

	MACH_PORT_UNREF(med->med_bootstrap);

	/*
	 * If the lock on this task exception handler is held,
	 * release it now as it will never be released by the
	 * exception handler.
	 */
	if (rw_lock_held(&med->med_exclock))
		wakeup(&med->med_exclock);

	/*
	 * If the kernel and host port are still referenced, remove
	 * the pointer to this process' struct proc, as it will
	 * become invalid once the process will exit.
	 */
	med->med_kernel->mp_datatype = MACH_MP_NONE;
	med->med_kernel->mp_data = NULL;
	MACH_PORT_UNREF(med->med_kernel);

	med->med_host->mp_datatype = MACH_MP_NONE;
	med->med_host->mp_data = NULL;
	MACH_PORT_UNREF(med->med_host);

	for (i = 0; i <= MACH_EXC_MAX; i++)
		if (med->med_exc[i] != NULL)
			MACH_PORT_UNREF(med->med_exc[i]);

	rw_destroy(&med->med_exclock);
	rw_destroy(&med->med_rightlock);
	free(med, M_EMULDATA);
	p->p_emuldata = NULL;

	return;
}

void
mach_e_lwp_fork(struct lwp *l1, struct lwp *l2)
{
	struct mach_lwp_emuldata *mle;

	mle = malloc(sizeof(*mle), M_EMULDATA, M_WAITOK);
	l2->l_emuldata = mle;

	mle->mle_kernel = mach_port_get();
	MACH_PORT_REF(mle->mle_kernel);

	mle->mle_kernel->mp_flags |= MACH_MP_INKERNEL;
	mle->mle_kernel->mp_datatype = MACH_MP_LWP;
	mle->mle_kernel->mp_data = (void *)l2;

#if 0
	/* Nothing to copy from parent thread for now */
	if (l1 != NULL);
#endif

	return;
}

void
mach_e_lwp_exit(struct lwp *l)
{
	struct mach_lwp_emuldata *mle;

	mach_semaphore_cleanup(l);

#ifdef DIAGNOSTIC
	if (l->l_emuldata == NULL) {
		printf("lwp_emuldata already freed\n");
		return;
	}
#endif
	mle = l->l_emuldata;

	mle->mle_kernel->mp_data = NULL;
	mle->mle_kernel->mp_datatype = MACH_MP_NONE;
	MACH_PORT_UNREF(mle->mle_kernel);

	free(mle, M_EMULDATA);
	l->l_emuldata = NULL;

	return;
}

static void
mach_init(void)
{
	mach_semaphore_init();
	mach_message_init();
	mach_port_init();

	mach_cold = 0;

	return;
}
