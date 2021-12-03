/*	$NetBSD: linux32_machdep.c,v 1.2 2021/12/03 09:20:23 ryo Exp $	*/

/*-
 * Copyright (c) 2021 Ryo Shimizu <ryo@nerv.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux32_machdep.c,v 1.2 2021/12/03 09:20:23 ryo Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/exec.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_exec.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_errno.h>
#include <compat/linux/common/linux_machdep.h>

#include <compat/linux32/common/linux32_types.h>
#include <compat/linux32/common/linux32_signal.h>
#include <compat/linux32/common/linux32_errno.h>
#include <compat/linux32/common/linux32_exec.h>
#include <compat/linux32/common/linux32_machdep.h>
#include <compat/linux32/linux32_syscall.h>
#include <compat/linux32/linux32_syscallargs.h>

#include <machine/netbsd32_machdep.h>
#include <arm/vfpreg.h>

static void
linux32_save_sigcontext(struct lwp *l, struct linux32_ucontext *luc)
{
	ucontext32_t uc;
	__greg32_t *gr = uc.uc_mcontext.__gregs;
	__vfpregset32_t *vfpregs = &uc.uc_mcontext.__vfpregs;
	struct linux32_aux_sigframe *aux;
	int i;

	cpu_getmcontext32(l, &uc.uc_mcontext, &uc.uc_flags);

	luc->luc_mcontext.arm_r0 = gr[_REG_R0];
	luc->luc_mcontext.arm_r1 = gr[_REG_R1];
	luc->luc_mcontext.arm_r2 = gr[_REG_R2];
	luc->luc_mcontext.arm_r3 = gr[_REG_R3];
	luc->luc_mcontext.arm_r4 = gr[_REG_R4];
	luc->luc_mcontext.arm_r5 = gr[_REG_R5];
	luc->luc_mcontext.arm_r6 = gr[_REG_R6];
	luc->luc_mcontext.arm_r7 = gr[_REG_R7];
	luc->luc_mcontext.arm_r8 = gr[_REG_R8];
	luc->luc_mcontext.arm_r9 = gr[_REG_R9];
	luc->luc_mcontext.arm_r10 = gr[_REG_R10];
	luc->luc_mcontext.arm_fp = gr[_REG_R11];
	luc->luc_mcontext.arm_ip = gr[_REG_R12];
	luc->luc_mcontext.arm_sp = gr[_REG_R13];
	luc->luc_mcontext.arm_lr = gr[_REG_R14];
	luc->luc_mcontext.arm_pc = gr[_REG_R15];
	luc->luc_mcontext.arm_cpsr = gr[_REG_CPSR];
	luc->luc_mcontext.trap_no = 0;
	luc->luc_mcontext.error_code = 0;
	luc->luc_mcontext.oldmask = 0;
	luc->luc_mcontext.fault_address = 0;

	if (uc.uc_flags & _UC_FPU) {
		aux = (struct linux32_aux_sigframe *)luc->luc_regspace;
#define LINUX32_VFP_MAGIC	0x56465001
		aux->vfp.magic = LINUX32_VFP_MAGIC;
		aux->vfp.size = sizeof(struct linux32_vfp_sigframe);

		CTASSERT(__arraycount(aux->vfp.ufp.fpregs) ==
		    __arraycount(vfpregs->__vfp_fstmx));
		for (i = 0; i < __arraycount(aux->vfp.ufp.fpregs); i++)
			aux->vfp.ufp.fpregs[i] = vfpregs->__vfp_fstmx[i];
		aux->vfp.ufp.fpscr = vfpregs->__vfp_fpscr;

		aux->vfp.ufp_exc.fpexc = VFP_FPEXC_EN | VFP_FPEXC_VECITR;
		aux->vfp.ufp_exc.fpinst = 0;
		aux->vfp.ufp_exc.fpinst2 = 0;
		aux->end_magic = 0 ;
	}
}

static int
linux32_restore_sigcontext(struct lwp *l, struct linux32_ucontext *luc)
{
	struct proc * const p = l->l_proc;
	ucontext32_t uc;
	__greg32_t *gr = uc.uc_mcontext.__gregs;
	__vfpregset32_t *vfpregs = &uc.uc_mcontext.__vfpregs;
	struct linux32_aux_sigframe *aux;
	int i, error;

	memset(&uc, 0, sizeof(uc));

	/* build .uc_sigmask */
	linux32_to_native_sigset(&uc.uc_sigmask, &luc->luc_sigmask);
	uc.uc_flags |= _UC_SIGMASK;

	/* build .uc_stack */
	if (luc->luc_stack.ss_flags & LINUX_SS_ONSTACK)
		uc.uc_stack.ss_flags |= SS_ONSTACK;
	if (luc->luc_stack.ss_flags & LINUX_SS_DISABLE)
		uc.uc_stack.ss_flags |= SS_DISABLE;
	uc.uc_flags |= _UC_STACK;

	/* build .uc_mcontext */
	gr[_REG_R0] = luc->luc_mcontext.arm_r0;
	gr[_REG_R1] = luc->luc_mcontext.arm_r1;
	gr[_REG_R2] = luc->luc_mcontext.arm_r2;
	gr[_REG_R3] = luc->luc_mcontext.arm_r3;
	gr[_REG_R4] = luc->luc_mcontext.arm_r4;
	gr[_REG_R5] = luc->luc_mcontext.arm_r5;
	gr[_REG_R6] = luc->luc_mcontext.arm_r6;
	gr[_REG_R7] = luc->luc_mcontext.arm_r7;
	gr[_REG_R8] = luc->luc_mcontext.arm_r8;
	gr[_REG_R9] = luc->luc_mcontext.arm_r9;
	gr[_REG_R10] = luc->luc_mcontext.arm_r10;
	gr[_REG_R11] = luc->luc_mcontext.arm_fp;
	gr[_REG_R12] = luc->luc_mcontext.arm_ip;
	gr[_REG_R13] = luc->luc_mcontext.arm_sp;
	gr[_REG_R14] = luc->luc_mcontext.arm_lr;
	gr[_REG_R15] = luc->luc_mcontext.arm_pc;
	gr[_REG_CPSR] = luc->luc_mcontext.arm_cpsr;
	uc.uc_flags |= _UC_CPU;

	aux = (struct linux32_aux_sigframe *)luc->luc_regspace;
	if (aux->vfp.magic == LINUX32_VFP_MAGIC &&
	    aux->vfp.size == sizeof(struct linux32_vfp_sigframe)) {

		CTASSERT(__arraycount(vfpregs->__vfp_fstmx) ==
		    __arraycount(aux->vfp.ufp.fpregs));
		for (i = 0; i < __arraycount(vfpregs->__vfp_fstmx); i++)
			vfpregs->__vfp_fstmx[i] = aux->vfp.ufp.fpregs[i];
		vfpregs->__vfp_fpscr = aux->vfp.ufp.fpscr;

		uc.uc_flags |= _UC_FPU;
	}

	mutex_enter(p->p_lock);
	error = setucontext32(l, &uc);
	mutex_exit(p->p_lock);

	return error;
}

void
linux32_sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp * const l = curlwp;
	struct proc * const p = l->l_proc;
	struct trapframe * const tf = lwp_trapframe(l);
	stack_t * const ss = &l->l_sigstk;
	const int sig = ksi->ksi_signo;
	const sig_t handler = SIGACTION(p, sig).sa_handler;
	struct linux32_rt_sigframe rt_sigframe_buf, *u_rt_sigframe;
	struct linux32_sigframe *tmp_sigframe, *u_sigframe;
	struct linux32_siginfo *tmp_siginfo, *u_siginfo;
	vaddr_t sp;
	int error;
	const bool onstack_p = /* use signal stack? */
	    (ss->ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;
	const bool rt_p = !!(SIGACTION(curproc, ksi->ksi_signo).sa_flags &
	    SA_SIGINFO);

	sp = onstack_p ? ((vaddr_t)ss->ss_sp + ss->ss_size) & -16 :
	    tf->tf_reg[13];

	memset(&rt_sigframe_buf, 0, sizeof(rt_sigframe_buf));

	if (rt_p) {
		/* allocate rt_sigframe on stack */
		sp -= sizeof(struct linux32_rt_sigframe);
		u_rt_sigframe = (struct linux32_rt_sigframe *)sp;
		u_sigframe = &u_rt_sigframe->sig;
		u_siginfo = &u_rt_sigframe->info;
		tmp_sigframe = &rt_sigframe_buf.sig;
		tmp_siginfo = &rt_sigframe_buf.info;
	} else {
		/* allocate sigframe on stack */
		sp -= sizeof(struct linux32_sigframe);
		u_rt_sigframe = NULL;
		u_sigframe = (struct linux32_sigframe *)sp;
		u_siginfo = NULL;
		tmp_sigframe = &rt_sigframe_buf.sig;
		tmp_siginfo = NULL;
	}

	if ((vaddr_t)sp >= VM_MAXUSER_ADDRESS32) {
		sigexit(l, SIGILL);
		return;
	}

	/* build linux sigframe, and copyout to user stack */
	tmp_sigframe->uc.luc_flags = 0;
	tmp_sigframe->uc.luc_link = NULL;
	NETBSD32PTR32(tmp_sigframe->uc.luc_stack.ss_sp, ss->ss_sp);
	tmp_sigframe->uc.luc_stack.ss_size = ss->ss_size;
	tmp_sigframe->uc.luc_stack.ss_flags = 0;
	if (ss->ss_flags & SS_ONSTACK)
		tmp_sigframe->uc.luc_stack.ss_flags |= LINUX_SS_ONSTACK;
	if (ss->ss_flags & SS_DISABLE)
		tmp_sigframe->uc.luc_stack.ss_flags |= LINUX_SS_DISABLE;
	native_to_linux32_sigset(&tmp_sigframe->uc.luc_sigmask, mask);
	if (tmp_siginfo != NULL)
		native_to_linux32_siginfo(tmp_siginfo, &ksi->ksi_info);
	sendsig_reset(l, sig);

	mutex_exit(p->p_lock);
	linux32_save_sigcontext(l, &tmp_sigframe->uc);

	/* copy linux sigframe onto the user stack */
	if (rt_p) {
		error = copyout(&rt_sigframe_buf, u_rt_sigframe,
		    sizeof(rt_sigframe_buf));
	} else {
		error = copyout(tmp_sigframe, u_sigframe,
		    sizeof(*tmp_sigframe));
	}

	mutex_enter(p->p_lock);

	if (error != 0 || (vaddr_t)handler >= VM_MAXUSER_ADDRESS32) {
		sigexit(l, SIGILL);
		return;
	}

	/* build context to run handler in. */
	tf->tf_reg[0] = native_to_linux_signo[sig];
	tf->tf_reg[1] = NETBSD32PTR32I(u_siginfo);
	tf->tf_reg[2] = NETBSD32PTR32I(&u_sigframe->uc);
	tf->tf_pc = (uint64_t)handler;
	tf->tf_reg[13] = sp;

	/* sigreturn trampoline */
	extern char linux32_sigcode[], linux32_rt_sigcode[];
	vsize_t linux_sigcode_offset;
	if (rt_p) {
		/* set return address to linux32_rt_sigcode() */
		linux_sigcode_offset = linux32_rt_sigcode - linux32_sigcode;
	} else {
		/* set return address to linux32_sigcode() */
		linux_sigcode_offset = linux32_sigcode - linux32_sigcode;
	}
	/* tf->tf_reg[14] is aarch32 lr */
	tf->tf_reg[14] = NETBSD32PTR32I((char *)p->p_sigctx.ps_sigcode +
	    linux_sigcode_offset);

	if (onstack_p)
		ss->ss_flags |= SS_ONSTACK;
}

int
linux32_sys_sigreturn(struct lwp *l,
    const struct linux32_sys_sigreturn_args *uap, register_t *retval)
{
	struct trapframe * const tf = lwp_trapframe(l);
	struct linux32_sigframe sigframe;
	int error;

	error = copyin((void *)tf->tf_reg[13], &sigframe, sizeof(sigframe));
	if (error != 0)
		goto done;

	error = linux32_restore_sigcontext(l, &sigframe.uc);
	if (error != 0)
		goto done;
	error = EJUSTRETURN;

 done:
	return error;
}

int
linux32_sys_rt_sigreturn(struct lwp *l,
    const struct linux32_sys_rt_sigreturn_args *uap, register_t *retval)
{
	struct trapframe * const tf = lwp_trapframe(l);
	struct linux32_rt_sigframe rt_sigframe;
	int error;

	error = copyin((void *)tf->tf_reg[13], &rt_sigframe,
	    sizeof(rt_sigframe));
	if (error != 0)
		goto done;

	error = linux32_restore_sigcontext(l, &rt_sigframe.sig.uc);
	if (error != 0)
		goto done;
	error = EJUSTRETURN;

 done:
	return error;
}

void
linux32_setregs(struct lwp *l, struct exec_package *pack, u_long stack)
{
	struct trapframe * const tf = lwp_trapframe(l);

	netbsd32_setregs(l, pack, stack);

	/* Same as netbsd32_setregs(), but some registers are set for linux */
	tf->tf_reg[0] = 0;
	tf->tf_reg[12] = 0;
	tf->tf_reg[13] = stack;		/* sp */
	tf->tf_reg[14] = pack->ep_entry;/* lr */
	tf->tf_reg[18] = 0;
	tf->tf_pc = pack->ep_entry;
}

int
linux32_sys_set_tls(struct lwp *l, const struct linux32_sys_set_tls_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(netbsd32_voidp) tls;
	} */
	return lwp_setprivate(l, SCARG_P32(uap, tls));
}

int
linux32_sys_get_tls(struct lwp *l, const void *uap, register_t *retval)
{
	retval[0] = NETBSD32PTR32I(l->l_private);
	return 0;
}
