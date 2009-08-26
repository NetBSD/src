/* $NetBSD: mips_fputrap.c,v 1.5.66.2 2009/08/26 14:33:59 matt Exp $ */

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

#ifndef SOFTFLOAT
void mips_fpuexcept(struct lwp *, unsigned int);
void mips_fpuillinst(struct lwp *, unsigned int, unsigned long);
static int fpustat2sicode(unsigned int);

void
mips_fpuexcept(struct lwp *l, unsigned int fpustat)
{
	ksiginfo_t ksi;

	KSI_INIT_TRAP(&ksi);
	ksi.ksi_signo = SIGFPE;
	ksi.ksi_code = fpustat2sicode(fpustat);
	ksi.ksi_trap = fpustat;
	(*l->l_proc->p_emul->e_trapsignal)(l, &ksi);
}

void
mips_fpuillinst(struct lwp *l, unsigned int opcode, unsigned long vaddr)
{
	ksiginfo_t ksi;

	KSI_INIT_TRAP(&ksi);
	ksi.ksi_signo = SIGILL;
	ksi.ksi_code = ILL_ILLOPC;
	ksi.ksi_trap = opcode;
	ksi.ksi_addr = (void *)vaddr;
	(*l->l_proc->p_emul->e_trapsignal)(l, &ksi);
}

static struct {
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
fpustat2sicode(unsigned int fpustat)
{
	int i;

	for (i = 0; i < 6; i++)
		if (fpustat & fpecodes[i].bit)
			return (fpecodes[i].code);
	return (FPE_FLTINV);
}
#endif /* !SOFTFLOAT */

void fpemul_trapsignal(struct lwp *, unsigned int, unsigned int);

void
fpemul_trapsignal(struct lwp *l, unsigned int sig, unsigned int code)
{
	ksiginfo_t ksi;

#if DEBUG
	printf("fpemul_trapsignal(%x,%x,%#"PRIxREGISTER")\n",
	   sig, code, l->l_md.md_regs->f_regs[_R_PC]);
#endif

	KSI_INIT_TRAP(&ksi);
	ksi.ksi_signo = sig;
	ksi.ksi_code = 1; /* XXX */
	ksi.ksi_trap = code;
	(*l->l_proc->p_emul->e_trapsignal)(l, &ksi);
}
