/*	$NetBSD: process_machdep.c,v 1.10 2003/01/19 19:49:56 scw Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/exec.h>
#include <sys/proc.h>
#include <sys/ptrace.h>
#include <sys/user.h>

#include <machine/cpu.h>
#include <machine/cacheops.h>
#include <machine/pcb.h>
#include <machine/frame.h>
#include <machine/reg.h>

#include <sh5/conreg.h>
#include <sh5/fpu.h>

void
setregs(struct lwp *l, struct exec_package *pack, u_long stack)
{
	struct trapframe *tf = l->l_md.md_regs;
	register_t sstack;
	long argc;

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
	tf->tf_caller.r3 = (register_t) (sstack + sizeof(long)); /* argv */
	tf->tf_caller.r4 = (register_t) (sstack + ((argc + 2) * sizeof(long)));

	/*
	 * r5 and r6 are reserved for the `cleanup' and `obj' parameters
	 * passed by the dynamic loader. The kernel always sets them to 0.
	 */

	tf->tf_caller.r7 = (register_t)(long)l->l_proc->p_psstr;

	/* Align the stack as required by the SH-5 ABI */
	tf->tf_caller.r15 = tf->tf_caller.r14 = (register_t) (sstack & ~0xf);

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

	/*
	 * XXX: This is a disgusting hack to work-around an unknown
	 * problem with the pmap which results in stale icache data
	 * during process exec.
	 *
	 * It seems that tearing down the original vmspace can, under some
	 * circumstances, leave turds in the icache, leading to random
	 * lossage in the new executable.
	 *
	 * Until such time as the cause is determined and fixed, this works
	 * around the problem.
	 */
	if (__cpu_cache_iinv_all)
		__cpu_cache_iinv_all();
}

int
process_read_regs(struct lwp *l, struct reg *regs)
{
	struct trapframe *tf = l->l_md.md_regs;

	regs->r_intregs[0]  = tf->tf_caller.r0;
	regs->r_intregs[1]  = tf->tf_caller.r1;
	regs->r_intregs[2]  = tf->tf_caller.r2;
	regs->r_intregs[3]  = tf->tf_caller.r3;
	regs->r_intregs[4]  = tf->tf_caller.r4;
	regs->r_intregs[5]  = tf->tf_caller.r5;
	regs->r_intregs[6]  = tf->tf_caller.r6;
	regs->r_intregs[7]  = tf->tf_caller.r7;
	regs->r_intregs[8]  = tf->tf_caller.r8;
	regs->r_intregs[9]  = tf->tf_caller.r9;

	regs->r_intregs[10] = tf->tf_caller.r10;
	regs->r_intregs[11] = tf->tf_caller.r11;
	regs->r_intregs[12] = tf->tf_caller.r12;
	regs->r_intregs[13] = tf->tf_caller.r13;

	regs->r_intregs[14] = tf->tf_caller.r14;
	regs->r_intregs[15] = tf->tf_caller.r15;
	regs->r_intregs[16] = tf->tf_caller.r16;
	regs->r_intregs[17] = tf->tf_caller.r17;
	regs->r_intregs[18] = tf->tf_caller.r18;
	regs->r_intregs[19] = tf->tf_caller.r19;
	regs->r_intregs[20] = tf->tf_caller.r20;
	regs->r_intregs[21] = tf->tf_caller.r21;
	regs->r_intregs[22] = tf->tf_caller.r22;
	regs->r_intregs[23] = tf->tf_caller.r23;
	regs->r_intregs[24] = 0;		/* OS Reserved */
	regs->r_intregs[25] = tf->tf_caller.r25;
	regs->r_intregs[26] = tf->tf_caller.r26;
	regs->r_intregs[27] = tf->tf_caller.r27;

	regs->r_intregs[28] = tf->tf_callee.r28;
	regs->r_intregs[29] = tf->tf_callee.r29;
	regs->r_intregs[30] = tf->tf_callee.r30;
	regs->r_intregs[31] = tf->tf_callee.r31;
	regs->r_intregs[32] = tf->tf_callee.r32;
	regs->r_intregs[33] = tf->tf_callee.r33;
	regs->r_intregs[34] = tf->tf_callee.r34;
	regs->r_intregs[35] = tf->tf_callee.r35;

	regs->r_intregs[36] = tf->tf_caller.r36;
	regs->r_intregs[37] = tf->tf_caller.r37;
	regs->r_intregs[38] = tf->tf_caller.r38;
	regs->r_intregs[39] = tf->tf_caller.r39;
	regs->r_intregs[40] = tf->tf_caller.r40;
	regs->r_intregs[41] = tf->tf_caller.r41;
	regs->r_intregs[42] = tf->tf_caller.r42;
	regs->r_intregs[43] = tf->tf_caller.r43;

	regs->r_intregs[44] = tf->tf_callee.r44;
	regs->r_intregs[45] = tf->tf_callee.r45;
	regs->r_intregs[46] = tf->tf_callee.r46;
	regs->r_intregs[47] = tf->tf_callee.r47;
	regs->r_intregs[48] = tf->tf_callee.r48;
	regs->r_intregs[49] = tf->tf_callee.r49;
	regs->r_intregs[50] = tf->tf_callee.r50;
	regs->r_intregs[51] = tf->tf_callee.r51;
	regs->r_intregs[52] = tf->tf_callee.r52;
	regs->r_intregs[53] = tf->tf_callee.r53;
	regs->r_intregs[54] = tf->tf_callee.r54;
	regs->r_intregs[55] = tf->tf_callee.r55;
	regs->r_intregs[56] = tf->tf_callee.r56;
	regs->r_intregs[57] = tf->tf_callee.r57;
	regs->r_intregs[58] = tf->tf_callee.r58;
	regs->r_intregs[59] = tf->tf_callee.r59;

	regs->r_intregs[60] = tf->tf_caller.r60;
	regs->r_intregs[61] = tf->tf_caller.r61;
	regs->r_intregs[62] = tf->tf_caller.r62;

	regs->r_tr[0] = tf->tf_caller.tr0;
	regs->r_tr[1] = tf->tf_caller.tr1;
	regs->r_tr[2] = tf->tf_caller.tr2;
	regs->r_tr[3] = tf->tf_caller.tr3;
	regs->r_tr[4] = tf->tf_caller.tr4;
	regs->r_tr[5] = tf->tf_callee.tr5;
	regs->r_tr[6] = tf->tf_callee.tr6;
	regs->r_tr[7] = tf->tf_callee.tr7;

	regs->r_pc = tf->tf_state.sf_spc;
	regs->r_usr = tf->tf_state.sf_usr;

	return (0);
}

int
process_read_fpregs(struct lwp *l, struct reg *regs)
{
	struct fpregs *fpr = &l->l_addr->u_pcb.pcb_ctx.sf_fpregs;

	memcpy(regs->r_fpregs, fpr->fp, sizeof(regs->r_fpregs));

	regs->r_fpscr = fpr->fpscr;

	return (0);
}

int
process_write_regs(struct lwp *l, struct reg *regs)
{
	struct trapframe *tf = l->l_md.md_regs;

	tf->tf_caller.r0 = regs->r_intregs[0];
	tf->tf_caller.r1 = regs->r_intregs[1];
	tf->tf_caller.r2 = regs->r_intregs[2];
	tf->tf_caller.r3 = regs->r_intregs[3];
	tf->tf_caller.r4 = regs->r_intregs[4];
	tf->tf_caller.r5 = regs->r_intregs[5];
	tf->tf_caller.r6 = regs->r_intregs[6];
	tf->tf_caller.r7 = regs->r_intregs[7];
	tf->tf_caller.r8 = regs->r_intregs[8];
	tf->tf_caller.r9 = regs->r_intregs[9];

	tf->tf_caller.r10 = regs->r_intregs[10];
	tf->tf_caller.r11 = regs->r_intregs[11];
	tf->tf_caller.r12 = regs->r_intregs[12];
	tf->tf_caller.r13 = regs->r_intregs[13];

	tf->tf_caller.r14 = regs->r_intregs[14];
	tf->tf_caller.r15 = regs->r_intregs[15];
	tf->tf_caller.r16 = regs->r_intregs[16];
	tf->tf_caller.r17 = regs->r_intregs[17];
	tf->tf_caller.r18 = regs->r_intregs[18];
	tf->tf_caller.r19 = regs->r_intregs[19];
	tf->tf_caller.r20 = regs->r_intregs[20];
	tf->tf_caller.r21 = regs->r_intregs[21];
	tf->tf_caller.r22 = regs->r_intregs[22];
	tf->tf_caller.r23 = regs->r_intregs[23];

#if 0
	tf->tf_caller.r24 = regs->r_intregs[24];	/* OS reserved */
#endif

	tf->tf_caller.r25 = regs->r_intregs[25];
	tf->tf_caller.r26 = regs->r_intregs[26];
	tf->tf_caller.r27 = regs->r_intregs[27];

	tf->tf_callee.r28 = regs->r_intregs[28];
	tf->tf_callee.r29 = regs->r_intregs[29];
	tf->tf_callee.r30 = regs->r_intregs[30];
	tf->tf_callee.r31 = regs->r_intregs[31];
	tf->tf_callee.r32 = regs->r_intregs[32];
	tf->tf_callee.r33 = regs->r_intregs[33];
	tf->tf_callee.r34 = regs->r_intregs[34];
	tf->tf_callee.r35 = regs->r_intregs[35];

	tf->tf_caller.r36 = regs->r_intregs[36];
	tf->tf_caller.r37 = regs->r_intregs[37];
	tf->tf_caller.r38 = regs->r_intregs[38];
	tf->tf_caller.r39 = regs->r_intregs[39];
	tf->tf_caller.r40 = regs->r_intregs[40];
	tf->tf_caller.r41 = regs->r_intregs[41];
	tf->tf_caller.r42 = regs->r_intregs[42];
	tf->tf_caller.r43 = regs->r_intregs[43];

	tf->tf_callee.r44 = regs->r_intregs[44];
	tf->tf_callee.r45 = regs->r_intregs[45];
	tf->tf_callee.r46 = regs->r_intregs[46];
	tf->tf_callee.r47 = regs->r_intregs[47];
	tf->tf_callee.r48 = regs->r_intregs[48];
	tf->tf_callee.r49 = regs->r_intregs[49];
	tf->tf_callee.r50 = regs->r_intregs[50];
	tf->tf_callee.r51 = regs->r_intregs[51];
	tf->tf_callee.r52 = regs->r_intregs[52];
	tf->tf_callee.r53 = regs->r_intregs[53];
	tf->tf_callee.r54 = regs->r_intregs[54];
	tf->tf_callee.r55 = regs->r_intregs[55];
	tf->tf_callee.r56 = regs->r_intregs[56];
	tf->tf_callee.r57 = regs->r_intregs[57];
	tf->tf_callee.r58 = regs->r_intregs[58];
	tf->tf_callee.r59 = regs->r_intregs[59];

	tf->tf_caller.r60 = regs->r_intregs[60];
	tf->tf_caller.r61 = regs->r_intregs[61];
	tf->tf_caller.r62 = regs->r_intregs[62];

	tf->tf_caller.tr0 = regs->r_tr[0];
	tf->tf_caller.tr1 = regs->r_tr[1];
	tf->tf_caller.tr2 = regs->r_tr[2];
	tf->tf_caller.tr3 = regs->r_tr[3];
	tf->tf_caller.tr4 = regs->r_tr[4];
	tf->tf_callee.tr5 = regs->r_tr[5];
	tf->tf_callee.tr6 = regs->r_tr[6];
	tf->tf_callee.tr7 = regs->r_tr[7];

	tf->tf_state.sf_spc = regs->r_pc;
	tf->tf_state.sf_usr = regs->r_usr;

	return (0);
}

int
process_write_fpregs(struct lwp *l, struct reg *regs)
{
	struct fpregs *fpr = &l->l_addr->u_pcb.pcb_ctx.sf_fpregs;

	memcpy(fpr->fp, regs->r_fpregs, sizeof(regs->r_fpregs));

	fpr->fpscr = regs->r_fpscr;

	return (0);
}

/*
 * DO NOT enable single stepping on SH5. It doesn't work in any obviously
 * useful way.
 */
int
process_sstep(struct lwp *l, int set)
{
#if 0
	struct trapframe *tf = p->p_md.md_regs;

	if (set)
		tf->tf_state.sf_ssr |= SH5_CONREG_SR_STEP;
	else
		tf->tf_state.sf_ssr &= ~SH5_CONREG_SR_STEP;

	return (0);
#else
	return (EINVAL);
#endif
}

/*
 * XXX: This will barf when userland's sizeof(void *) != kernel's.
 */
int
process_set_pc(struct lwp *l, caddr_t pc)
{

	l->l_md.md_regs->tf_state.sf_spc = (register_t)(uintptr_t)pc;

	return (0);
}
