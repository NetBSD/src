/*	$NetBSD: _context_u.S,v 1.2 2003/01/21 11:31:38 scw Exp $	*/

/*
 * Copyright 2003 Wasabi Systems, Inc.
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
#include "assym.h"

/*
 * Save the current context into the ucontext_t structure pointed to r2.
 *
 * We only need to save the `callee' saved registers.
 * Assumes the PC value to save is in r18.
 *
 * Clobbers r0, r1 and r3
 *
 * XXX: Will need to be revisited in order to support FPU-less cores.
 */
#define	GETC						\
	st.q	r2, UC_REGS_PC, r18			;\
	st.q	r2, UC_REGS_R10, r10			;\
	st.q	r2, UC_REGS_R11, r11			;\
	st.q	r2, UC_REGS_R12, r12			;\
	st.q	r2, UC_REGS_R13, r13			;\
	st.q	r2, UC_REGS_R14, r14			;\
	st.q	r2, UC_REGS_SP, r15			;\
	st.q	r2, UC_REGS_R26, r26			;\
	st.q	r2, UC_REGS_R27, r27			;\
	st.q	r2, UC_REGS_R28, r28			;\
	st.q	r2, UC_REGS_R29, r29			;\
	st.q	r2, UC_REGS_R30, r30			;\
	st.q	r2, UC_REGS_R31, r31			;\
	st.q	r2, UC_REGS_R32, r32			;\
	st.q	r2, UC_REGS_R33, r33			;\
	st.q	r2, UC_REGS_R34, r34			;\
	st.q	r2, UC_REGS_R35, r35			;\
	st.q	r2, UC_REGS_R44, r44			;\
	st.q	r2, UC_REGS_R45, r45			;\
	st.q	r2, UC_REGS_R46, r46			;\
	st.q	r2, UC_REGS_R47, r47			;\
	st.q	r2, UC_REGS_R48, r48			;\
	st.q	r2, UC_REGS_R49, r49			;\
	st.q	r2, UC_REGS_R50, r50			;\
	st.q	r2, UC_REGS_R51, r51			;\
	st.q	r2, UC_REGS_R52, r52			;\
	st.q	r2, UC_REGS_R53, r53			;\
	st.q	r2, UC_REGS_R54, r54			;\
	st.q	r2, UC_REGS_R55, r55			;\
	st.q	r2, UC_REGS_R56, r56			;\
	st.q	r2, UC_REGS_R57, r57			;\
	st.q	r2, UC_REGS_R58, r58			;\
	st.q	r2, UC_REGS_R59, r59			;\
	getcon	usr, r1					;\
	gettr	tr5, r0					;\
	st.q	r2, UC_REGS_TR5, r0			;\
	gettr	tr6, r0					;\
	st.q	r2, UC_REGS_TR6, r0			;\
	gettr	tr7, r0					;\
	st.q	r2, UC_REGS_TR7, r0			;\
	pta/l	3f, tr0					;\
	st.q	r2, UC_REGS_USR, r1			;\
	fgetscr	fr0					;\
	shlri	r1, SH5_CONREG_USR_FPRS_SHIFT, r1	;\
	LDC32((_UC_CPU | _UC_USER), r3)			;\
	andi	r1, SH5_CONREG_USR_FPRS_MASK, r0	;\
	fst.s	r2, UC_FPREGS_FPSCR, fr0		;\
	beq/l	r0, r63, tr0				;\
	fst.d	r2, UC_FPREGS_DR12, dr12		;\
	fst.d	r2, UC_FPREGS_DR14, dr14		;\
	fst.d	r2, UC_FPREGS_DR36, dr36		;\
	fst.d	r2, UC_FPREGS_DR38, dr38		;\
	pta/l	1f, tr0					;\
	andi	r1, 0x20, r0				;\
	beq/l	r0, r63, tr0				;\
	fst.d	r2, UC_FPREGS_DR40, dr40		;\
	fst.d	r2, UC_FPREGS_DR42, dr42		;\
	fst.d	r2, UC_FPREGS_DR44, dr44		;\
	fst.d	r2, UC_FPREGS_DR46, dr46		;\
1:	pta/l	2f, tr0					;\
	andi	r1, 0x40, r0				;\
	beq/l	r0, r63, tr0				;\
	fst.d	r2, UC_FPREGS_DR48, dr48		;\
	fst.d	r2, UC_FPREGS_DR50, dr50		;\
	fst.d	r2, UC_FPREGS_DR52, dr52		;\
	fst.d	r2, UC_FPREGS_DR54, dr54		;\
2:	pta/l	3f, tr0					;\
	andi	r1, 0x80, r0				;\
	beq/l	r0, r63, tr0				;\
	fst.d	r2, UC_FPREGS_DR56, dr56		;\
	fst.d	r2, UC_FPREGS_DR58, dr58		;\
	fst.d	r2, UC_FPREGS_DR60, dr60		;\
	fst.d	r2, UC_FPREGS_DR62, dr62		;\
	ori	r3, _UC_FPU, r3				;\
3:	st.l	r2, UC_FLAGS, r3

/*
 * Restore the context pointed to by r2
 */
#define	SETC						\
	/*						\
	 * If _UC_USER is set, we only need to restore	\
	 * the callee-saved set. If not, we have no	\
	 * choice but to defer to the setcontext(2)	\
	 * system call as we can't do it ourselves.	\
	 * (Actually, we can restore everything except	\
	 * for the branch target register we use to	\
	 * jump to the new context. Which just isn't	\
	 * quite good enough. Sigh.)			\
	 */						\
	ld.q	r2, UC_FLAGS, r3			;\
	ld.q	r2, UC_REGS_R10, r10			;\
	shlri	r3, _UC_USER_BIT, r0			;\
	ld.q	r2, UC_REGS_R11, r11			;\
	andi	r0, 1, r0				;\
	pta/u	4f, tr0					;\
	ld.q	r2, UC_REGS_R12, r12			;\
	beq/u	r0, r63, tr0				;\
	ld.q	r2, UC_REGS_R13, r13			;\
	ld.q	r2, UC_REGS_R14, r14			;\
	ld.q	r2, UC_REGS_R26, r26			;\
	ld.q	r2, UC_REGS_R27, r27			;\
	ld.q	r2, UC_REGS_R28, r28			;\
	ld.q	r2, UC_REGS_R29, r29			;\
	ld.q	r2, UC_REGS_R30, r30			;\
	ld.q	r2, UC_REGS_R31, r31			;\
	ld.q	r2, UC_REGS_R32, r32			;\
	ld.q	r2, UC_REGS_R33, r33			;\
	ld.q	r2, UC_REGS_R34, r34			;\
	ld.q	r2, UC_REGS_R35, r35			;\
	ld.q	r2, UC_REGS_R44, r44			;\
	ld.q	r2, UC_REGS_R45, r45			;\
	ld.q	r2, UC_REGS_R46, r46			;\
	ld.q	r2, UC_REGS_R47, r47			;\
	ld.q	r2, UC_REGS_R48, r48			;\
	ld.q	r2, UC_REGS_R49, r49			;\
	ld.q	r2, UC_REGS_R50, r50			;\
	ld.q	r2, UC_REGS_R51, r51			;\
	ld.q	r2, UC_REGS_R52, r52			;\
	ld.q	r2, UC_REGS_R53, r53			;\
	ld.q	r2, UC_REGS_R54, r54			;\
	ld.q	r2, UC_REGS_R55, r55			;\
	ld.q	r2, UC_REGS_R56, r56			;\
	ld.q	r2, UC_REGS_R57, r57			;\
	ld.q	r2, UC_REGS_R58, r58			;\
	ld.q	r2, UC_REGS_R59, r59			;\
	ld.q	r2, UC_REGS_TR5, r0			;\
	ptabs/u	r0, tr5					;\
	ld.q	r2, UC_REGS_TR6, r0			;\
	ptabs/u	r0, tr6					;\
	ld.q	r2, UC_REGS_TR7, r0			;\
	pta/l	3f, tr0					;\
	ptabs/u	r0, tr7					;\
	andi	r3, _UC_FPU, r0				;\
	ld.q	r2, UC_REGS_USR, r3			;\
	beq/l	r0, r63, tr0				;\
	shlri	r3, SH5_CONREG_USR_FPRS_SHIFT, r1	;\
	fld.d	r2, UC_FPREGS_DR12, dr12		;\
	fld.d	r2, UC_FPREGS_DR14, dr14		;\
	fld.d	r2, UC_FPREGS_DR36, dr36		;\
	fld.d	r2, UC_FPREGS_DR38, dr38		;\
	pta/l	1f, tr0					;\
	andi	r1, 0x20, r0				;\
	beq/l	r0, r63, tr0				;\
	fld.d	r2, UC_FPREGS_DR40, dr40		;\
	fld.d	r2, UC_FPREGS_DR42, dr42		;\
	fld.d	r2, UC_FPREGS_DR44, dr44		;\
	fld.d	r2, UC_FPREGS_DR46, dr46		;\
1:	pta/l	2f, tr0					;\
	andi	r1, 0x40, r0				;\
	beq/l	r0, r63, tr0				;\
	fld.d	r2, UC_FPREGS_DR48, dr48		;\
	fld.d	r2, UC_FPREGS_DR50, dr50		;\
	fld.d	r2, UC_FPREGS_DR52, dr52		;\
	fld.d	r2, UC_FPREGS_DR54, dr54		;\
2:	pta/l	3f, tr0					;\
	andi	r1, 0x80, r0				;\
	beq/l	r0, r63, tr0				;\
	fld.d	r2, UC_FPREGS_DR56, dr56		;\
	fld.d	r2, UC_FPREGS_DR58, dr58		;\
	fld.d	r2, UC_FPREGS_DR60, dr60		;\
	fld.d	r2, UC_FPREGS_DR62, dr62		;\
3:	fld.s	r2, UC_FPREGS_FPSCR, fr0		;\
	ld.q	r2, UC_REGS_PC, r0			;\
	ld.q	r2, UC_REGS_SP, r15			;\
	ptabs/l	r0, tr0					;\
	fputscr	fr0					;\
	putcon	r3, usr					;\
	blink	tr0, r63				;\
							\
	/* ! _UC_USER case. Restore full context */	\
							\
4:	pta/l	_C_LABEL(setcontext), tr0		;\
	blink	tr0, r18				;\
	trapa	r63	/* Catch setcontex() failure */


/*
 * int _getcontext_u(ucontext_t *ctx)
 *	Store the current context in the provided ctx structure.
 */
ENTRY_NOPROFILE(_getcontext_u)
	GETC
	ptabs/l	r18, tr0
	movi	0, r2
	blink	tr0, r63

/*
 * int _setcontext_u(const ucontext_t *ctx)
 *	Make the context stored in ctx current.
 */
ENTRY_NOPROFILE(_setcontext_u)
	SETC
	/* NOTREACHED */

/*
 * int _swapcontext_u(ucontext_t *from, const ucontext_t *to)
 *	Save the current context in `from' before switching to the
 *	new context in `to'.
 */
ENTRY_NOPROFILE(_swapcontext_u)
	or	r3, r63, r5		/* Save 'to' in a safe place */
	GETC
	or	r5, r63, r2		/* Restore 'to' */
	SETC
	/* NOTREACHED */
