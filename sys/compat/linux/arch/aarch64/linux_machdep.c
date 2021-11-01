/*	$NetBSD: linux_machdep.c,v 1.3 2021/11/01 05:07:16 thorpej Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux_machdep.c,v 1.3 2021/11/01 05:07:16 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_errno.h>
#include <compat/linux/common/linux_exec.h>
#include <compat/linux/common/linux_ioctl.h>
#include <compat/linux/common/linux_prctl.h>
#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>
#include <compat/linux/linux_syscall.h>
#include <compat/linux/linux_syscallargs.h>

void
linux_setregs(struct lwp *l, struct exec_package *epp, vaddr_t stack)
{
	setregs(l, epp, stack);
}

static void
linux_save_sigcontext(struct lwp *l, struct linux_sigcontext *ctx)
{
	ucontext_t uc;

	cpu_getmcontext(l, &uc.uc_mcontext, &uc.uc_flags);

	memset(ctx, 0, sizeof(*ctx));
	/* ctx->fault_address = 0; */
	CTASSERT(sizeof(ctx->regs) <= sizeof(uc.uc_mcontext.__gregs));
	for (int i = 0; i < __arraycount(ctx->regs); i++)
		ctx->regs[i] = uc.uc_mcontext.__gregs[i];
	ctx->sp = uc.uc_mcontext.__gregs[_REG_SP];
	ctx->pc = uc.uc_mcontext.__gregs[_REG_PC];
	ctx->pstate = uc.uc_mcontext.__gregs[_REG_SPSR];

	if (uc.uc_flags & _UC_FPU) {
		struct fpsimd_context *fpsimd;

		fpsimd = (struct fpsimd_context *)ctx->__reserved;
		fpsimd->head.magic = FPSIMD_MAGIC;
		fpsimd->head.size = sizeof(struct fpsimd_context);
		fpsimd->fpsr = uc.uc_mcontext.__fregs.__fpsr;
		fpsimd->fpcr = uc.uc_mcontext.__fregs.__fpcr;
		CTASSERT(sizeof(fpsimd->vregs) ==
		    sizeof(uc.uc_mcontext.__fregs.__qregs));
		memcpy(fpsimd->vregs, uc.uc_mcontext.__fregs.__qregs,
		    sizeof(uc.uc_mcontext.__fregs.__qregs));
	}

	/* ctx->__reserved[] has already been terminated by memset() */
}

static void
aarch64_linux_to_native_ucontext(ucontext_t *uc, struct linux_ucontext *luc)
{
	struct linux_sigcontext *ctx = &luc->luc_mcontext;
	struct fpsimd_context *fpsimd;

	memset(uc, 0, sizeof(*uc));

	/* build .uc_flags, .uc_link, and .uc_sigmask */
	uc->uc_flags = (_UC_SIGMASK | _UC_CPU | _UC_STACK | _UC_CLRSTACK);
	uc->uc_link = NULL;
	linux_to_native_sigset(&uc->uc_sigmask, &luc->luc_sigmask);

	/* build .uc_stack */
	if (luc->luc_stack.ss_flags & LINUX_SS_ONSTACK)
		uc->uc_stack.ss_flags |= SS_ONSTACK;
	if (luc->luc_stack.ss_flags & LINUX_SS_DISABLE)
		uc->uc_stack.ss_flags |= SS_DISABLE;
	uc->uc_stack.ss_sp = luc->luc_stack.ss_sp;
	uc->uc_stack.ss_size = luc->luc_stack.ss_size;

	/* build .uc_mcontext */
	CTASSERT(sizeof(ctx->regs) <= sizeof(uc->uc_mcontext.__gregs));
	for (int i = 0; i < __arraycount(ctx->regs); i++)
		uc->uc_mcontext.__gregs[i] = ctx->regs[i];
	uc->uc_mcontext.__gregs[_REG_SP] = ctx->sp;
	uc->uc_mcontext.__gregs[_REG_PC] = ctx->pc;
	uc->uc_mcontext.__gregs[_REG_SPSR] = ctx->pstate;

	fpsimd = (struct fpsimd_context *)ctx->__reserved;
	if (fpsimd->head.magic == FPSIMD_MAGIC) {
		uc->uc_flags |= _UC_FPU;
		uc->uc_mcontext.__fregs.__fpsr = fpsimd->fpsr;
		uc->uc_mcontext.__fregs.__fpcr = fpsimd->fpcr;
		CTASSERT(sizeof(fpsimd->vregs) ==
		    sizeof(uc->uc_mcontext.__fregs.__qregs));
		memcpy(uc->uc_mcontext.__fregs.__qregs, fpsimd->vregs,
		    sizeof(uc->uc_mcontext.__fregs.__qregs));
	}
}

void
linux_sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp * const l = curlwp;
	struct proc * const p = l->l_proc;
	struct trapframe * const tf = lwp_trapframe(l);
	stack_t * const ss = &l->l_sigstk;
	const int sig = ksi->ksi_signo;
	const sig_t handler = SIGACTION(p, sig).sa_handler;
	struct linux_rt_sigframe *u_sigframe, *tmp_sigframe;
	vaddr_t sp;
	int error;
	const bool onstack_p = /* use signal stack? */
	    (ss->ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	sp = onstack_p ? ((vaddr_t)ss->ss_sp + ss->ss_size) & -16 : tf->tf_sp;

	/* allocate sigframe on userstack */
	sp -= sizeof(struct linux_rt_sigframe);
	u_sigframe = (struct linux_rt_sigframe *)sp;

	/* build linux sigframe, and copyout to user stack */
	tmp_sigframe = kmem_zalloc(sizeof(*tmp_sigframe), KM_SLEEP);
	tmp_sigframe->uc.luc_flags = 0;
	tmp_sigframe->uc.luc_link = NULL;
	tmp_sigframe->uc.luc_stack.ss_sp = ss->ss_sp;
	tmp_sigframe->uc.luc_stack.ss_size = ss->ss_size;
	tmp_sigframe->uc.luc_stack.ss_flags = 0;
	if (ss->ss_flags & SS_ONSTACK)
		tmp_sigframe->uc.luc_stack.ss_flags |= LINUX_SS_ONSTACK;
	if (ss->ss_flags & SS_DISABLE)
		tmp_sigframe->uc.luc_stack.ss_flags |= LINUX_SS_DISABLE;
	native_to_linux_sigset(&tmp_sigframe->uc.luc_sigmask, mask);
	native_to_linux_siginfo(&tmp_sigframe->info, &ksi->ksi_info);
	sendsig_reset(l, sig);

	mutex_exit(p->p_lock);
	linux_save_sigcontext(l, &tmp_sigframe->uc.luc_mcontext);

	/* copy linux sigframe onto the user stack */
	error = copyout(tmp_sigframe, u_sigframe, sizeof(*tmp_sigframe));
	kmem_free(tmp_sigframe, sizeof(*tmp_sigframe));

	mutex_enter(p->p_lock);

	if (error != 0 || (vaddr_t)handler >= VM_MAXUSER_ADDRESS) {
		sigexit(l, SIGILL);
		return;
	}

	/* build context to run handler in. */
	tf->tf_reg[0] = native_to_linux_signo[sig];
	tf->tf_reg[1] = (uint64_t)&u_sigframe->info;
	tf->tf_reg[2] = (uint64_t)&u_sigframe->uc;
	tf->tf_pc = (uint64_t)handler;
	tf->tf_sp = sp;

	/* sigreturn trampoline */
	extern char linux_sigcode[], linux_rt_sigcode[];
	vsize_t linux_rt_sigcode_offset = linux_rt_sigcode - linux_sigcode;
	tf->tf_lr = (uint64_t)p->p_sigctx.ps_sigcode + linux_rt_sigcode_offset;

	if (onstack_p)
		ss->ss_flags |= SS_ONSTACK;
}

int
linux_sys_rt_sigreturn(struct lwp *l, const void *v, register_t *retval)
{
	struct trapframe * const tf = lwp_trapframe(l);
	struct proc * const p = l->l_proc;
	struct linux_rt_sigframe *lsigframe;
	ucontext_t uc;
	int error;

	/*
	 * struct linux_rt_sigframe is variable size,
	 * but netbsd/aarch64's linux_sendsig() always pushes a full size
	 * linux_rt_sigframe.
	 */
	lsigframe = kmem_zalloc(sizeof(*lsigframe), KM_SLEEP);
	error = copyin((void *)tf->tf_sp, lsigframe, sizeof(*lsigframe));
	if (error != 0)
		goto done;

	aarch64_linux_to_native_ucontext(&uc, &lsigframe->uc);

	mutex_enter(p->p_lock);
	error = setucontext(l, &uc);
	mutex_exit(p->p_lock);
	if (error != 0)
		goto done;
	error = EJUSTRETURN;

 done:
	kmem_free(lsigframe, sizeof(*lsigframe));
	return error;
}

void
linux_trapsignal(struct lwp *l, ksiginfo_t *ksi)
{
	/*
	 * XXX: TODO
	 * should be convert siginfo from native to linux
	 */
	trapsignal(l, ksi);
}

int
linux_usertrap(struct lwp *l, vaddr_t trapaddr, void *arg)
{
	/* not used */
	return 0;
}

dev_t
linux_fakedev(dev_t dev, int raw)
{
	/* TODO if needed */
	return dev;
}

int
linux_machdepioctl(struct lwp *l, const struct linux_sys_ioctl_args *uap,
    register_t *retval)
{
	/* TODO if needed */
	return EINVAL;
}
