/*	$NetBSD: netbsd32_machdep.c,v 1.5 2004/03/17 20:23:28 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_machdep.c,v 1.5 2004/03/17 20:23:28 scw Exp $");

#ifdef _KERNEL_OPT
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/exec.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/mount.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/igmp.h>
#include <netinet/igmp_var.h>
#include <netinet/ip_mroute.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_ioctl.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

#include <machine/cpu.h>
#include <machine/reg.h>

#include <sh5/fpu.h>

char	machine_arch32[] = "sh5";

#ifdef COMPAT_16
int
compat_16_netbsd32___sigreturn14(struct lwp *l, void *v, register_t *retval)
{
	struct compat_16_netbsd32___sigreturn14_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	struct compat_16_sys___sigreturn14_args csra;
	netbsd32_sigcontextp_t scp;

	scp = (netbsd32_sigcontextp_t) SCARG(uap, sigcntxp);

	SCARG(&csra, sigcntxp) = (struct sigcontext *)(intptr_t)scp;

	return (compat_16_sys___sigreturn14(l, &csra, retval));
}
#endif

/*ARGSUSED*/
int
cpu_coredump32(struct lwp *l, struct vnode *vp,
    struct ucred *cred, struct core32 *chdr)
{

	return (0);
}

int
netbsd32_sysarch(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_sysarch_args /* {
		syscallarg(int) op;
		syscallarg(netbsd32_voidp) parms;
	} */ *uap = v;

	switch (SCARG(uap, op)) {
	default:
		printf("(%s) netbsd32_sysarch(%d)\n", MACHINE, SCARG(uap, op));
		return EINVAL;
	}
}

void
netbsd32_setregs(struct lwp *l, struct exec_package *pack, u_long stack)
{
	struct trapframe *tf = l->l_md.md_regs;
	register_t sstack;
	int argc;

	/* Ensure stack pointer is sign-extended */
	sstack = (register_t)(long)stack;

	/* This cannot fail, as copyargs() has already used copyout() on it */
#ifdef DIAGNOSTIC
	if (copyin((caddr_t)(uintptr_t)sstack, &argc, sizeof(argc)) != 0)
		panic("setregs: argc copyin failed!");
#else
	(void) copyin((caddr_t)(uintptr_t)sstack, &argc, sizeof(argc));
#endif

	memset(tf, 0, sizeof(*tf));

	tf->tf_state.sf_spc = pack->ep_entry;
	tf->tf_state.sf_ssr = SH5_CONREG_SR_MMU;
	tf->tf_state.sf_flags = SF_FLAGS_CALLEE_SAVED;

	tf->tf_caller.r2 = (register_t) argc;			 /* argc */
	tf->tf_caller.r3 = (register_t) (sstack + sizeof(int)); /* argv */
	tf->tf_caller.r4 = (register_t) (sstack + ((argc + 2) * sizeof(int)));

	/*
	 * r5 and r6 are reserved for the `cleanup' and `obj' parameters
	 * passed by the dynamic loader. The kernel always sets them to 0.
	 */

	tf->tf_caller.r7 = (register_t)(long)l->l_proc->p_psstr;

	/* Align the stack as required by the SH-5 ABI */
	tf->tf_caller.r15 = (register_t) (sstack & ~0xf);

	/* Give the new process a clean set of FP regs */
	memset(&l->l_addr->u_pcb.pcb_ctx.sf_fpregs, 0, sizeof(struct fpregs));

	/*
	 * I debated with myself about the following for a wee while.
	 *
	 * Disable FPU Exceptions for arithmetic operations on de-normalised
	 * numbers. While this is contrary to the IEEE-754, it's how things
	 * work in NetBSD/i386.
	 *
	 * Since most applications are likely to have been developed on i386
	 * platforms, most application programmers would never see this
	 * fault in their code. The in-tree top(1) program is one such
	 * offender, under certain circumstances.
	 *
	 * With FPSCR.DN set, denormalised numbers are quietly flushed to zero.
	 */
	l->l_addr->u_pcb.pcb_ctx.sf_fpregs.fpscr = SH5_FPSCR_DN_FLUSH_ZERO;

	sh5_fprestore(SH5_CONREG_USR_FPRS_MASK << SH5_CONREG_USR_FPRS_SHIFT,
	    &l->l_addr->u_pcb);
}
