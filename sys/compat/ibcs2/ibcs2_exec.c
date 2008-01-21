/*	$NetBSD: ibcs2_exec.c,v 1.63.2.6 2008/01/21 09:40:58 yamt Exp $	*/

/*
 * Copyright (c) 1994, 1995, 1998 Scott Bartram
 * Copyright (c) 1994 Adam Glass
 * Copyright (c) 1993, 1994 Christopher G. Demetriou
 * All rights reserved.
 *
 * originally from kern/exec_ecoff.c
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
 *      This product includes software developed by Scott Bartram.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ibcs2_exec.c,v 1.63.2.6 2008/01/21 09:40:58 yamt Exp $");

#if defined(_KERNEL_OPT)
#include "opt_syscall_debug.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/exec.h>

#include <uvm/uvm_extern.h>

#include <machine/ibcs2_machdep.h>

#include <compat/ibcs2/ibcs2_types.h>
#include <compat/ibcs2/ibcs2_exec.h>
#include <compat/ibcs2/ibcs2_signal.h>
#include <compat/ibcs2/ibcs2_errno.h>
#include <compat/ibcs2/ibcs2_syscall.h>

static void ibcs2_e_proc_exec(struct proc *, struct exec_package *);

extern struct sysent ibcs2_sysent[];
extern const char * const ibcs2_syscallnames[];
extern char ibcs2_sigcode[], ibcs2_esigcode[];
#ifndef __HAVE_SYSCALL_INTERN
void syscall(void);
#endif

#ifdef IBCS2_DEBUG
int ibcs2_debug = 1;
#endif

struct uvm_object *emul_ibcs2_object;

const struct emul emul_ibcs2 = {
	"ibcs2",
	"/emul/ibcs2",
#ifndef __HAVE_MINIMAL_EMUL
	0,
	native_to_ibcs2_errno,
	IBCS2_SYS_syscall,
	IBCS2_SYS_NSYSENT,
#endif
	ibcs2_sysent,
#ifdef SYSCALL_DEBUG
	ibcs2_syscallnames,
#else
	NULL,
#endif
	ibcs2_sendsig,
	trapsignal,
	NULL,	/* e_tracesig */
	ibcs2_sigcode,
	ibcs2_esigcode,
	&emul_ibcs2_object,
	ibcs2_setregs,
	ibcs2_e_proc_exec,
	NULL,	/* e_proc_fork */
	NULL,	/* e_proc_exit */
	NULL,	/* e_lwp_fork */
	NULL,	/* e_lwp_exec */
#ifdef __HAVE_SYSCALL_INTERN
	ibcs2_syscall_intern,
#else
	syscall,
#endif
	NULL,	/* e_sysctlovly */
	NULL,	/* e_fault */

	uvm_default_mapaddr,
	NULL,	/* e_usertrap */
	0,	/* e_ucsize */
	NULL,	/* e_startlwp */
};

/*
 * This is exec process hook. Find out if this is x.out executable, if
 * yes, set flag appropriately, so that emul code which needs to adjust
 * behaviour accordingly can do so.
 */
static void
ibcs2_e_proc_exec(struct proc *p, struct exec_package *epp)
{
	if (epp->ep_esch->es_makecmds == exec_ibcs2_xout_makecmds)
		p->p_emuldata = IBCS2_EXEC_XENIX;
	else
		p->p_emuldata = IBCS2_EXEC_OTHER;
}

/*
 * ibcs2_exec_setup_stack(): Set up the stack segment for an
 * executable.
 *
 * Note that the ep_ssize parameter must be set to be the current stack
 * limit; this is adjusted in the body of execve() to yield the
 * appropriate stack segment usage once the argument length is
 * calculated.
 *
 * This function returns an int for uniformity with other (future) formats'
 * stack setup functions.  They might have errors to return.
 */

int
ibcs2_exec_setup_stack(struct lwp *l, struct exec_package *epp)
{
	u_long max_stack_size;
	u_long access_linear_min, access_size;
	u_long noaccess_linear_min, noaccess_size;

#ifndef	USRSTACK32
#define USRSTACK32	(0x00000000ffffffffL&~PGOFSET)
#endif

	if (epp->ep_flags & EXEC_32) {
		epp->ep_minsaddr = USRSTACK32;
		max_stack_size = MAXSSIZ;
	} else {
		epp->ep_minsaddr = USRSTACK;
		max_stack_size = MAXSSIZ;
	}
	epp->ep_maxsaddr = (u_long)STACK_GROW(epp->ep_minsaddr,
		max_stack_size);
	epp->ep_ssize = l->l_proc->p_rlimit[RLIMIT_STACK].rlim_cur;

	/*
	 * set up commands for stack.  note that this takes *two*, one to
	 * map the part of the stack which we can access, and one to map
	 * the part which we can't.
	 *
	 * arguably, it could be made into one, but that would require the
	 * addition of another mapping proc, which is unnecessary
	 */
	access_size = epp->ep_ssize;
	access_linear_min = (u_long)STACK_ALLOC(epp->ep_minsaddr, access_size);
	noaccess_size = max_stack_size - access_size;
	noaccess_linear_min = (u_long)STACK_ALLOC(STACK_GROW(epp->ep_minsaddr,
	    access_size), noaccess_size);
	if (noaccess_size > 0) {
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, noaccess_size,
		    noaccess_linear_min, NULL, 0, VM_PROT_NONE);
	}
	KASSERT(access_size > 0);
	/* XXX: some ibcs2 binaries need an executable stack. */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, access_size,
	    access_linear_min, NULL, 0, VM_PROT_READ | VM_PROT_WRITE |
	    VM_PROT_EXECUTE);

	return 0;
}
