/*	$NetBSD: frame.h,v 1.1 2002/07/05 13:31:58 scw Exp $	*/

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

/*
 * SH-5 Stack Frame Layouts
 */

#ifndef _SH5_FRAME_H_
#define	_SH5_FRAME_H_

#include <sys/signal.h>

/*
 * All exception frames contain a state frame which is a snapshot of the
 * exception-specific control registers at the time of the exception.
 */
struct stateframe {
	register_t	sf_flags;	/* See below */
	register_t	sf_ssr;		/* Saved Status Register */
	register_t	sf_spc;		/* Saved Program Counter */
	register_t	sf_expevt;	/* EXPEVT Control Register */
	register_t	sf_intevt;	/* INTEVT Control Register */
	register_t	sf_tea;		/* TEA Control Register */
	register_t	sf_tra;		/* TRA Control Register */
	register_t	sf_usr;		/* User Status Register */
};

#define	SF_FLAGS_CALLEE_SAVED	0x1	/* Frame contains callee-saved regs */

/*
 * SH5 ABI Callee-saved registers.
 */
struct exc_calleesave {
	register_t r10; register_t r11; register_t r12; register_t r13;

	register_t r28; register_t r29; register_t r30; register_t r31;
	register_t r32; register_t r33; register_t r34; register_t r35;

	register_t r44; register_t r45; register_t r46; register_t r47;
	register_t r48; register_t r49; register_t r50; register_t r51;
	register_t r52; register_t r53; register_t r54; register_t r55;
	register_t r56; register_t r57; register_t r58; register_t r59;

	register_t tr5; register_t tr6; register_t tr7;
};

/*
 * SH5 ABI Caller-saved registers
 *
 * Note: See comments in <sh5/sh5/exception.S> as to why r14 is
 * part of the Caller-Saved set.
 */
struct exc_callersave {
	register_t r0;  register_t r1;  register_t r2;  register_t r3;
	register_t r4;  register_t r5;  register_t r6;  register_t r7;
	register_t r8;  register_t r9;

	register_t r14; register_t r15; register_t r16; register_t r17;
	register_t r18; register_t r19; register_t r20; register_t r21;
	register_t r22; register_t r23;

	register_t r25; register_t r26; register_t r27;

	register_t r36; register_t r37; register_t r38; register_t r39;
	register_t r40; register_t r41; register_t r42; register_t r43;

	register_t r60; register_t r61; register_t r62;

	register_t tr0; register_t tr1; register_t tr2; register_t tr3;
	register_t tr4;
};

/*
 * Interrupt (asynchronous) exception frames contain a stateframe and
 * only the caller-saved register set. This reduces the size of the
 * frame to save both time and stack space during interrupt dispatch.
 * The latter being particularly important when interrupts can nest
 * up to 15 levels deep...
 *
 * The low-level exception handling code is able to grow these into
 * full-blown trapframes (below) if necessary.
 *
 * Note: DO NOT change this structure unless you *really* understand
 *       what happens in exception.S
 */
struct intrframe {
	struct stateframe	if_state;	/* Machine state */
	struct exc_callersave	if_caller;	/* Caller-saved registers */
};

/*
 * Synchronous exception frames contain the same state as the interrupt
 * frame, above, with the addition of Callee-saved registers. This
 * provides a complete snapshot of the machine state at the point where
 * the exception happened.
 *
 * Note: DO NOT change this structure unless you *really* understand
 *       what happens in exception.S
 */
struct trapframe {
	struct exc_calleesave	tf_callee;	/* Callee-saved registers */
	struct intrframe	tf_ifr;		/* Caller-saved/Machine state */
};
/* Convenience macroes for accessing caller-saved registers & machine state */
#define	tf_state	tf_ifr.if_state
#define	tf_caller	tf_ifr.if_caller

#define	USERMODE(tf)	(((tf)->tf_state.sf_ssr & SH5_CONREG_SR_MD) == 0)

/*
 * Floating point state is saved in the following structure
 *
 * Note that the set of FP registers actually saved in here is controlled
 * by the FPRS bits of the USR register, saved in switchframe->sf_usr.
 */
struct fpregs {
	register_t	fpscr;
	register_t	fp[32];
};

/*
 * A switchframe is used by cpu_switch() to save and restore a process'
 * kernel context.
 * This consists of the callee-saved register set, the current kernel
 * stack pointer, the program counter, and the status register.
 */
struct switchframe {
	register_t	sf_pc;			/* Saved program counter */
	register_t	sf_sr;			/* Status register */
	register_t	sf_sp;			/* Kernel stack pointer */
	struct exc_calleesave sf_regs;		/* Saved registers */
	struct fpregs	sf_fpregs;
};

/*
 * Exception handling requires a per-cpu scratch frame with the
 * following contents.
 */
struct exc_scratch_frame {
	register_t	es_critical;	/* Non-zero if valid contents */
	register_t	es_usr;		/* Saved user status register */
	register_t	es_r[3];	/* Saved r0-r2 */
	register_t	es_r15;		/* Saved r15 */
	register_t	es_expevt;	/* Saved expevt */
	register_t	es_intevt;	/* Saved intevt */
	register_t	es_tea;		/* Saved tea */
	register_t	es_tra;		/* Saved tra */
};

/*
 * TLB Miss handling requires a per-cpu scratch frame/stack with the
 * following contents.
 */
struct tlb_scratch_frame {
	register_t	ts_r[7];	/* Saved r0 - r6 */
	register_t	ts_tr[2];	/* Saved tr0 - tr1 */
	char		ts_stack[1024];	/* TLB Stack */
};

#endif /* _SH5_FRAME_H_ */
