/* $NetBSD: mips_fputrap.c,v 1.6.6.1 2011/06/06 09:06:07 jruoho Exp $ */

/*
 * Copyright (c) 2004
 *	Matthias Drochner. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/siginfo.h>

#include <mips/cpuregs.h>
#include <mips/regnum.h>
#include <mips/locore.h>

#if defined(FPEMUL) || !defined(NOFPU)
void mips_fpuexcept(struct lwp *, uint32_t);
void mips_fpuillinst(struct lwp *, uint32_t);
static int fpustat2sicode(uint32_t);

void
mips_fpuexcept(struct lwp *l, uint32_t fpustat)
{
	ksiginfo_t ksi;

#ifdef FPEMUL_DEBUG
	printf("%s(%x,%#"PRIxREGISTER")\n",
	   __func__, fpustat, l->l_md.md_utf->tf_regs[_R_PC]);
#endif

	KSI_INIT_TRAP(&ksi);
	ksi.ksi_signo = SIGFPE;
	ksi.ksi_code = fpustat2sicode(fpustat);
	ksi.ksi_trap = fpustat;
	(*l->l_proc->p_emul->e_trapsignal)(l, &ksi);
}

void
mips_fpuillinst(struct lwp *l, uint32_t opcode)
{
	ksiginfo_t ksi;

#ifdef FPEMUL_DEBUG
	printf("%s(%x,%#"PRIxREGISTER")\n",
	   __func__, opcode, l->l_md.md_utf->tf_regs[_R_PC]);
#endif

	KSI_INIT_TRAP(&ksi);
	ksi.ksi_signo = SIGILL;
	ksi.ksi_code = ILL_ILLOPC;
	ksi.ksi_trap = opcode;
	ksi.ksi_addr = (void *)(uintptr_t)l->l_md.md_utf->tf_regs[_R_PC];
	(*l->l_proc->p_emul->e_trapsignal)(l, &ksi);
}

static const struct {
	unsigned int bit;
	int code;
} fpecodes[] = {
	{ MIPS_FPU_EXCEPTION_UNDERFLOW, FPE_FLTUND },
	{ MIPS_FPU_EXCEPTION_OVERFLOW, FPE_FLTOVF },
	{ MIPS_FPU_EXCEPTION_INEXACT, FPE_FLTRES },
	{ MIPS_FPU_EXCEPTION_DIV0, FPE_FLTDIV },
	{ MIPS_FPU_EXCEPTION_INVALID, FPE_FLTINV },
	{ MIPS_FPU_EXCEPTION_UNIMPL, FPE_FLTINV }
};

static int
fpustat2sicode(uint32_t fpustat)
{
	for (size_t i = 0; i < __arraycount(fpecodes); i++) {
		if (fpustat & fpecodes[i].bit)
			return fpecodes[i].code;
	}

	return FPE_FLTINV;
}
#endif /* FPEMUL || !NOFPU */
