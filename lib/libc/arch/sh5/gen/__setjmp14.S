/*	$NetBSD: __setjmp14.S,v 1.4 2003/03/24 14:29:34 scw Exp $	*/

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

#include <machine/asm.h>
#include <machine/setjmp.h>

/*
 * C library -- setjmp, longjmp
 *
 *	longjmp(a,v)
 * will generate a "return(v)" from
 * the last call to
 *	setjmp(a)
 * by restoring registers from the stack,
 * and the previous signal state.
 */

ENTRY_NOPROFILE(__setjmp14)
	/* Save callee-saved register */
	getcon	usr, r1
	st.q	r2, _JB_R10, r10
	st.q	r2, _JB_R11, r11
	st.q	r2, _JB_R12, r12
	st.q	r2, _JB_R13, r13
	st.q	r2, _JB_R14, r14
	st.q	r2, _JB_R15, r15
	st.q	r2, _JB_R18, r18
	st.q	r2, _JB_PC, r18
	LDUC32(0xACEBABE5, r0)
	st.q	r2, _JB_R24, r0		/* sigcontext magic number */
	st.q	r2, _JB_R26, r26
	st.q	r2, _JB_R27, r27
	st.q	r2, _JB_R28, r28
	st.q	r2, _JB_R29, r29
	st.q	r2, _JB_R30, r30
	st.q	r2, _JB_R31, r31
	st.q	r2, _JB_R32, r32
	st.q	r2, _JB_R33, r33
	st.q	r2, _JB_R34, r34
	st.q	r2, _JB_R35, r35
	st.q	r2, _JB_R44, r44
	st.q	r2, _JB_R45, r45
	st.q	r2, _JB_R46, r46
	st.q	r2, _JB_R47, r47
	st.q	r2, _JB_R48, r48
	st.q	r2, _JB_R49, r49
	st.q	r2, _JB_R50, r50
	st.q	r2, _JB_R51, r51
	st.q	r2, _JB_R52, r52
	st.q	r2, _JB_R53, r53
	st.q	r2, _JB_R54, r54
	st.q	r2, _JB_R55, r55
	st.q	r2, _JB_R56, r56
	st.q	r2, _JB_R57, r57
	st.q	r2, _JB_R58, r58
	st.q	r2, _JB_R59, r59
	st.q	r2, _JB_TR0, r18	/* Ensure caller-saved copies of */
	st.q	r2, _JB_TR1, r18	/* branch-target registers have valid */
	st.q	r2, _JB_TR2, r18	/* SHmedia addresses for the caller's */
	st.q	r2, _JB_TR3, r18	/* address space. */
	st.q	r2, _JB_TR4, r18
	gettr	tr5, r0
	st.q	r2, _JB_TR5, r0
	gettr	tr6, r0
	st.q	r2, _JB_TR6, r0
	gettr	tr7, r0
	st.q	r2, _JB_TR7, r0
	pta/u	1f, tr0
	fmov.dq dr0, r0			/* Save dr0 temporarily */
	fgetscr	fr0			/* Always save FPSCR */
	fst.s	r2, _JB_FPSCR, fr0
	fmov.qd r0, dr0			/* Restore dr0 */
	putcon	r1, usr			/* Compensate for using dr0 */
	shlri	r1, 8, r3
	andi	r3, 0xff, r3
	movi	0, r0			/* Assume not saved */
	beq/u	r3, r63, tr0		/* Jump if no FP regs used */
	movi	-3329, r0		/* Mask for FP regs saved/used */
	and	r1, r0, r1		/* Update USR */
	fst.d	r2, _JB_DR12, dr12	/* Quicker to save all callee-saved */
	fst.d	r2, _JB_DR14, dr14	/* than check USR bits */
	fst.d	r2, _JB_DR36, dr36
	fst.d	r2, _JB_DR38, dr38
	fst.d	r2, _JB_DR40, dr40
	fst.d	r2, _JB_DR42, dr42
	fst.d	r2, _JB_DR44, dr44
	fst.d	r2, _JB_DR46, dr46
	fst.d	r2, _JB_DR48, dr48
	fst.d	r2, _JB_DR50, dr50
	fst.d	r2, _JB_DR52, dr52
	fst.d	r2, _JB_DR54, dr54
	fst.d	r2, _JB_DR56, dr56
	fst.d	r2, _JB_DR58, dr58
	fst.d	r2, _JB_DR60, dr60
	fst.d	r2, _JB_DR62, dr62
	movi	3, r0			/* Regs saved/used */
1:	st.l	r2, _JB_SIGFPSTATE, r0
	st.q	r2, _JB_USR, r1

	or	r2, r63, r28		/* r28 is a safe-haven for jmpbuf */

	PIC_PROLOGUE

	/* Get signal information */
	PIC_PTAL(_C_LABEL(__sigprocmask14), r0, tr0)
	movi	0, r2			/* how */
	movi	0, r3			/* set (NULL) */
	movi	_JB_SIGMASK, r0
	add	r28, r0, r4		/* point to mask in jmpbuf */
	blink	tr0, r18

	/* Prepare to fetch the current signal stack state */
	PIC_PTAL(_C_LABEL(__sigaltstack14), r0, tr0)
#ifndef _LP64
	addi	r15, -16, r15		/* ALIGN(sizeof(struct sigaltstack)) */
#else
	addi	r15, -24, r15		/* sizeof(struct sigaltstack) */
#endif
	movi	0, r2			/* ss = NULL */
	or	r15, r63, r3		/* oss -> sigaltstack created above */
	blink	tr0, r18
#ifndef _LP64
	ld.l	r15, 8, r0		/* Fetch oss->ss_flags */
	addi	r15, 16, r15		/* Clear stack */
#else
	ld.l	r15, 16, r0		/* Fetch oss->ss_flags */
	addi	r15, 24, r15		/* Clear stack */
#endif

	PIC_EPILOGUE

	ld.q	r28, _JB_R18, r18	/* Restore return address */
	pta/u	Lbotch, tr0		/* Exit on error */
	bgt/u	r63, r2, tr0
	andi	r0, 1, r0		/* Get SA_ONSTACK flag */
	st.l	r28, _JB_SIGONSTACK, r0	/* Save it in sc_onstack */
	st.l	r28, _JB_SIGMASK13, r63	/* Zero unused fields */
	st.l	r28, _JB_SIGPAD, r63
	ld.q	r28, _JB_R28, r28	/* Restore r28 */
	ptabs/l	r18, tr0
	movi	0, r2
	blink	tr0, r63


ENTRY_NOPROFILE(__longjmp14)
	LINK_FRAME(0)
	PIC_PROLOGUE
	PIC_PTAL(_C_LABEL(__sigreturn14), r0, tr0)
	movi	1, r1
	cmveq	r3, r1, r3		/* Ensure a non-zero return value */
	st.q	r2, _JB_R2, r3		/* Modify jmp_buf's copy of r2 */
	blink	tr0, r18
	PIC_EPILOGUE
	UNLINK_FRAME(0)

	/*
	 * The call to __sigreturn14 will only return here on error.
	 * This usually happens if the kernel determines that the jmp_buf
	 * is invalid in some way.
	 */

Lbotch:	LINK_FRAME(0)
	PIC_PROLOGUE
	PIC_PTAL(_C_LABEL(longjmperror), r0, tr0)
	blink	tr0, r18
	PIC_PTAL(_C_LABEL(abort), r0, tr0)
	blink	tr0, r18
	brk

