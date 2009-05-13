/*	$NetBSD: fpu.c,v 1.16.8.1 2009/05/13 17:17:47 jym Exp $	*/

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthew Fredette.
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

/*
 * FPU handling for NetBSD/hppa.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fpu.c,v 1.16.8.1 2009/05/13 17:17:47 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/user.h>

#include <uvm/uvm_extern.h>

#include <machine/cpufunc.h>
#include <machine/frame.h>
#include <machine/reg.h>
#include <machine/pmap.h>

#include <hppa/hppa/machdep.h>

#include "../spmath/float.h"
#include "../spmath/fpudispatch.h"

/* Some macros representing opcodes. */
#define OPCODE_NOP	0x08000240
#define OPCODE_COPR_0_0	0x30000000

/* Some macros representing fields in load/store opcodes. */
#define	OPCODE_CMPLT_S	0x00002000
#define	OPCODE_CMPLT_M	0x00000020
#define	OPCODE_CMPLT_SM	(OPCODE_CMPLT_S | OPCODE_CMPLT_M)
#define	OPCODE_CMPLT_MB	OPCODE_CMPLT_M
#define	OPCODE_CMPLT_MA	(OPCODE_CMPLT_S | OPCODE_CMPLT_M)
#define	OPCODE_CMPLT	(OPCODE_CMPLT_S | OPCODE_CMPLT_M)
#define	OPCODE_DOUBLE	0x08000000
#define	OPCODE_STORE	0x00000200
#define OPCODE_INDEXED	0x00001000

/* This is nonzero iff we're using a hardware FPU. */
int fpu_present;

/* If we have any FPU, this is its version. */
u_int fpu_version;

/* The number of times we have had to switch the FPU context. */
u_int fpu_csw;

/* The U-space physical address of the proc in the FPU, or zero. */
paddr_t fpu_cur_uspace;

/* In locore.S, this swaps states in and out of the FPU. */
void hppa_fpu_swap(struct pcb *, struct pcb *);

#ifdef FPEMUL
/*
 * Given a trapframe and a general register number, the 
 * FRAME_REG macro returns a pointer to that general
 * register.  The _frame_reg_positions array is a lookup 
 * table, since the general registers aren't in order 
 * in a trapframe.
 * 
 * NB: this more or less assumes that all members of 
 * struct trapframe are u_ints.
 */
#define FRAME_REG(f, reg, r0)	\
	((reg) == 0 ? (&r0) : ((&(f)->tf_t1) + _frame_reg_positions[reg]))
#define _FRAME_POSITION(f)	\
	((&((struct trapframe *) 0)->f) - (&((struct trapframe *) 0)->tf_t1))
const int _frame_reg_positions[32] = {
	-1,				/* r0 */
	_FRAME_POSITION(tf_r1),
	_FRAME_POSITION(tf_rp),		/* r2 */
	_FRAME_POSITION(tf_r3),
	_FRAME_POSITION(tf_r4),
	_FRAME_POSITION(tf_r5),
	_FRAME_POSITION(tf_r6),
	_FRAME_POSITION(tf_r7),
	_FRAME_POSITION(tf_r8),
	_FRAME_POSITION(tf_r9),
	_FRAME_POSITION(tf_r10),
	_FRAME_POSITION(tf_r11),
	_FRAME_POSITION(tf_r12),
	_FRAME_POSITION(tf_r13),
	_FRAME_POSITION(tf_r14),
	_FRAME_POSITION(tf_r15),
	_FRAME_POSITION(tf_r16),
	_FRAME_POSITION(tf_r17),
	_FRAME_POSITION(tf_r18),
	_FRAME_POSITION(tf_t4),		/* r19 */
	_FRAME_POSITION(tf_t3),		/* r20 */
	_FRAME_POSITION(tf_t2),		/* r21 */
	_FRAME_POSITION(tf_t1),		/* r22 */
	_FRAME_POSITION(tf_arg3),	/* r23 */
	_FRAME_POSITION(tf_arg2),	/* r24 */
	_FRAME_POSITION(tf_arg1),	/* r25 */
	_FRAME_POSITION(tf_arg0),	/* r26 */
	_FRAME_POSITION(tf_dp),		/* r27 */
	_FRAME_POSITION(tf_ret0),	/* r28 */
	_FRAME_POSITION(tf_ret1),	/* r29 */
	_FRAME_POSITION(tf_sp),		/* r30 */
	_FRAME_POSITION(tf_r31),
};
#endif /* FPEMUL */

/*
 * Bootstraps the FPU.
 */
void
hppa_fpu_bootstrap(u_int ccr_enable)
{
	u_int32_t junk[2];
	u_int32_t vers[2];
	extern u_int hppa_fpu_nop0;
	extern u_int hppa_fpu_nop1;

	/* See if we have a present and functioning hardware FPU. */
	fpu_present = (ccr_enable & HPPA_FPUS) == HPPA_FPUS;

	/* Initialize the FPU and get its version. */
	if (fpu_present) {

		/*
		 * To somewhat optimize the emulation
		 * assist trap handling and context
		 * switching (to save them from having
	 	 * to always load and check fpu_present),
		 * there are two instructions in locore.S
		 * that are replaced with nops when 
		 * there is a hardware FPU.
	 	 */
		hppa_fpu_nop0 = OPCODE_NOP;
		hppa_fpu_nop1 = OPCODE_NOP;
		fcacheall();

		/*
		 * We track what process has the FPU,
		 * and how many times we have to swap
		 * in and out.
		 */

		/*
		 * The PA-RISC 1.1 Architecture manual is 
		 * pretty clear that the copr,0,0 must be 
		 * wrapped in double word stores of fr0, 
		 * otherwise its operation is undefined.
		 */
		__asm volatile(
			"	ldo	%0, %%r22	\n"
			"	fstds	%%fr0, 0(%%r22)	\n"
			"	ldo	%1, %%r22	\n"
			"	copr,0,0		\n"
			"	fstds	%%fr0, 0(%%r22)	\n"
			: "=m" (junk), "=m" (vers) : : "r22");

		/*
		 * Now mark that no process has the FPU,
		 * and disable it, so the first time it
		 * gets used the process' state gets
		 * swapped in.
		 */
		fpu_csw = 0;
		fpu_cur_uspace = 0;
		mtctl(ccr_enable & (CCR_MASK ^ HPPA_FPUS), CR_CCR);	
	} 
#ifdef FPEMUL
	else
		/*
		 * XXX This is a hack - to avoid
		 * having to set up the emulator so
		 * it can work for one instruction for
		 * proc0, we dispatch the copr,0,0 opcode 
		 * into the emulator directly.  
		 */
		decode_0c(OPCODE_COPR_0_0, 0, 0, vers);
#endif /* FPEMUL */
	fpu_version = vers[0];
}

/*
 * If the given LWP has its state in the FPU,
 * flush that state out into the LWP's PCB.
 */
void
hppa_fpu_flush(struct lwp *l)
{
	struct trapframe *tf = l->l_md.md_regs;

	/*
	 * If we have a hardware FPU, and this process'
	 * state is currently in it, swap it out.
	 */

	if (!fpu_present || fpu_cur_uspace == 0 ||
	    fpu_cur_uspace != tf->tf_cr30) {
		return;
	}

	hppa_fpu_swap(&l->l_addr->u_pcb, NULL);
	fpu_cur_uspace = 0;
}

#ifdef FPEMUL

/*
 * This emulates a coprocessor load/store instruction.
 */
static int hppa_fpu_ls(struct trapframe *, struct lwp *);
static int 
hppa_fpu_ls(struct trapframe *frame, struct lwp *l)
{
	u_int inst, inst_b, inst_x, inst_s, inst_t;
	int log2size;
	u_int *base;
	u_int offset, index, im5;
	void *fpreg;
	u_int r0 = 0;
	int error;

	/*
	 * Get the instruction that we're emulating,
	 * and break it down.  Using HP bit notation,
	 * b is a five-bit field starting at bit 10, 
	 * x is a five-bit field starting at bit 15,
	 * s is a two-bit field starting at bit 17, 
	 * and t is a five-bit field starting at bit 31.
	 */
	inst = frame->tf_iir;
	__asm volatile(
		"	extru %4, 10, 5, %1	\n"
		"	extru %4, 15, 5, %2	\n"
		"	extru %4, 17, 2, %3	\n"
		"	extru %4, 31, 5, %4	\n"
		: "=r" (inst_b), "=r" (inst_x), "=r" (inst_s), "=r" (inst_t)
		: "r" (inst));

	/*
	 * The space must be the user's space, else we
	 * segfault.
	 */
	if (inst_s != l->l_addr->u_pcb.pcb_space)
		return (EFAULT);

	/* See whether or not this is a doubleword load/store. */
	log2size = (inst & OPCODE_DOUBLE) ? 3 : 2;

	/* Get the floating point register. */
	fpreg = ((char *)l->l_addr->u_pcb.pcb_fpregs) + (inst_t << log2size);

	/* Get the base register. */
	base = FRAME_REG(frame, inst_b, r0);
	
	/* Dispatch on whether or not this is an indexed load/store. */
	if (inst & OPCODE_INDEXED) {
		
		/* Get the index register value. */
		index = *FRAME_REG(frame, inst_x, r0);
		
		/* Dispatch on the completer. */
		switch (inst & OPCODE_CMPLT) {
		case OPCODE_CMPLT_S:
			offset = *base + (index << log2size);
			break;
		case OPCODE_CMPLT_M:
			offset = *base;
			*base = *base + index;
			break;
		case OPCODE_CMPLT_SM:
			offset = *base;
			*base = *base + (index << log2size);
			break;
		default:
			offset = *base + index;
			break;
		}
	} else {

		/* Do a low_sign_ext(x, 5). */
		im5 = inst_x >> 1;
		if (inst_x & 1)
			im5 |= 0xfffffff0;

		/* Dispatch on the completer. */
		switch (inst & OPCODE_CMPLT) {
		case OPCODE_CMPLT_MB:
			offset = *base + im5;
			*base = *base + im5;
			break;
		case OPCODE_CMPLT_MA:
			offset = *base;
			*base = *base + im5;
			break;
		default:
			offset = *base + im5;
			break;
		}
	}

	/*
	 * The offset we calculated must be the same as the
	 * offset in the IOR.
	 */
	KASSERT(offset == frame->tf_ior);

	/* Perform the load or store. */
	error = (inst & OPCODE_STORE) ?
		copyout(fpreg, (void *) offset, 1 << log2size) :
		copyin((const void *) offset, fpreg, 1 << log2size);
	fdcache(HPPA_SID_KERNEL, (vaddr_t)fpreg,
		sizeof(l->l_addr->u_pcb.pcb_fpregs));
	return error;
}

/*
 * This is called to emulate an instruction.
 */
void 
hppa_fpu_emulate(struct trapframe *frame, struct lwp *l, u_int inst)
{
	u_int opcode, class, sub;
	u_int *fpregs;
	int exception;
	ksiginfo_t ksi;

	/*
	 * If the process' state is in any hardware FPU, 
	 * flush it out - we need to operate on it.
	 */
	hppa_fpu_flush(l);

	/*
	 * Get the instruction that we're emulating,
	 * and break it down.  Using HP bit notation,
	 * the class is a two-bit field starting at
	 * bit 22, the opcode is a 6-bit field starting
	 * at bit 5, and sub for a class 1 instruction
	 * is a two bit field starting at bit 16, else
	 * it is a three bit field starting at bit 18.
	 */
#if 0
	__asm volatile(
		"	extru %3, 22, 2, %1	\n"
		"	extru %3, 5, 6, %0	\n"
		"	extru %3, 18, 3, %2	\n"
		"	comib,<> 1, %1, 0	\n"
		"	extru %3, 16, 2, %2	\n"
		: "=r" (opcode), "=r" (class), "=r" (sub)
		: "r" (inst));
#else
	opcode = (inst >> (31 - 5)) & 0x3f;
	class = (inst >> (31 - 22)) & 0x3;
	if (class == 1) {
		sub = (inst >> (31 - 16)) & 3;
	} else {
		sub = (inst >> (31 - 18)) & 7;
	}
#endif

	/* Get this LWP's FPU registers. */
	fpregs = (u_int *) l->l_addr->u_pcb.pcb_fpregs;

	/* Dispatch on the opcode. */
	switch (opcode) {
	case 0x09:
	case 0x0b:
		if (hppa_fpu_ls(frame, l) != 0) {
			KSI_INIT_TRAP(&ksi);
			ksi.ksi_signo = SIGSEGV;
			ksi.ksi_code = SEGV_MAPERR;
			ksi.ksi_trap = T_DTLBMISS;
			ksi.ksi_addr = (void *)frame->tf_iioq_head;
			trapsignal(l, &ksi);
		}
		return;
	case 0x0c:
		exception = decode_0c(inst, class, sub, fpregs);
		break;
	case 0x0e:
		exception = decode_0e(inst, class, sub, fpregs);
		break;
	case 0x06:
		exception = decode_06(inst, fpregs);
		break;
	case 0x26:  
		exception = decode_26(inst, fpregs);
		break;
	default: 
		exception = UNIMPLEMENTEDEXCEPTION;
		break;
        }

	if (exception) {
		KSI_INIT_TRAP(&ksi);
		if (exception & UNIMPLEMENTEDEXCEPTION) {
			ksi.ksi_signo = SIGILL;
			ksi.ksi_code = ILL_COPROC;
		} else {
			ksi.ksi_signo = SIGFPE;
			if (exception & INVALIDEXCEPTION) {
				ksi.ksi_code = FPE_FLTINV;
			} else if (exception & DIVISIONBYZEROEXCEPTION) {
				ksi.ksi_code = FPE_FLTDIV;
			} else if (exception & OVERFLOWEXCEPTION) {
				ksi.ksi_code = FPE_FLTOVF;
			} else if (exception & UNDERFLOWEXCEPTION) {
				ksi.ksi_code = FPE_FLTUND;
			} else if (exception & INEXACTEXCEPTION) {
				ksi.ksi_code = FPE_FLTRES;
			}
		}
		ksi.ksi_trap = T_EMULATION;
		ksi.ksi_addr = (void *)frame->tf_iioq_head;
		trapsignal(l, &ksi);
	}
	fdcache(HPPA_SID_KERNEL, (vaddr_t)fpregs,
		sizeof(l->l_addr->u_pcb.pcb_fpregs));
}

#endif /* FPEMUL */
