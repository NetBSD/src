/*	$NetBSD: _setjmp.S,v 1.6 2003/03/24 14:29:34 scw Exp $	*/

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
 * by restoring registers from the stack.
 * The previous signal state is NOT restored.
 */

ENTRY_NOPROFILE(_setjmp)
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
	LDUC32(0xBADBABE5, r0)
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
	st.q	r2, _JB_TR2, r18	/* SHmedia addresses in the caller's */
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
	ptabs/l	r18, tr0
	movi	0, r2
	blink	tr0, r63


ENTRY_NOPROFILE(_longjmp)
	pta/u	Lbotch, tr0
	ld.q	r2, _JB_R24, r0		/* Check if jmp_buf was saved by a */
	LDUC32(0xBADBABE5, r1)		/* call to _setjmp, above */
	bne/u	r0, r1, tr0		/* Punt if not */
	movi	1, r1
	cmveq	r3, r1, r3		/* Ensure a non-zero return value */

	/* Restore callee-saved register */
	ld.q	r2, _JB_R10, r10
	ld.q	r2, _JB_R11, r11
	ld.q	r2, _JB_R12, r12
	ld.q	r2, _JB_R13, r13
	ld.q	r2, _JB_R14, r14
	ld.q	r2, _JB_R15, r15
	ld.q	r2, _JB_R18, r18
	ld.q	r2, _JB_R26, r26
	ld.q	r2, _JB_R27, r27
	ld.q	r2, _JB_R28, r28
	ld.q	r2, _JB_R29, r29
	ld.q	r2, _JB_R30, r30
	ld.q	r2, _JB_R31, r31
	ld.q	r2, _JB_R32, r32
	ld.q	r2, _JB_R33, r33
	ld.q	r2, _JB_R34, r34
	ld.q	r2, _JB_R35, r35
	ld.q	r2, _JB_R44, r44
	ld.q	r2, _JB_R45, r45
	ld.q	r2, _JB_R46, r46
	ld.q	r2, _JB_R47, r47
	ld.q	r2, _JB_R48, r48
	ld.q	r2, _JB_R49, r49
	ld.q	r2, _JB_R50, r50
	ld.q	r2, _JB_R51, r51
	ld.q	r2, _JB_R52, r52
	ld.q	r2, _JB_R53, r53
	ld.q	r2, _JB_R54, r54
	ld.q	r2, _JB_R55, r55
	ld.q	r2, _JB_R56, r56
	ld.q	r2, _JB_R57, r57
	ld.q	r2, _JB_R58, r58
	ld.q	r2, _JB_R59, r59
	ld.q	r2, _JB_TR5, r0		/* Could take an IADDERR exception */
	ptabs/u	r0, tr5			/* here if the jmp_buf is corrupt... */
	ld.q	r2, _JB_TR6, r0
	ptabs/u	r0, tr6
	ld.q	r2, _JB_TR7, r0
	ptabs/u	r0, tr7
	ld.l	r2, _JB_SIGFPSTATE, r0	/* Were any FP regs saved? */
	pta/u	1f, tr0
	andi	r0, 3, r0
	beq/u	r0, r63, tr0		/* Skip if not. */
	fld.d	r2, _JB_DR12, dr12	/* Quicker to save all callee-saved */
	fld.d	r2, _JB_DR14, dr14	/* than check USR bits */
	fld.d	r2, _JB_DR36, dr36
	fld.d	r2, _JB_DR38, dr38
	fld.d	r2, _JB_DR40, dr40
	fld.d	r2, _JB_DR42, dr42
	fld.d	r2, _JB_DR44, dr44
	fld.d	r2, _JB_DR46, dr46
	fld.d	r2, _JB_DR48, dr48
	fld.d	r2, _JB_DR50, dr50
	fld.d	r2, _JB_DR52, dr52
	fld.d	r2, _JB_DR54, dr54
	fld.d	r2, _JB_DR56, dr56
	fld.d	r2, _JB_DR58, dr58
	fld.d	r2, _JB_DR60, dr60
	fld.d	r2, _JB_DR62, dr62
1:	fmov.dq dr0, r0			/* Save dr0 temporarily */
	fld.s	r2, _JB_FPSCR, fr0
	fputscr	fr0			/* Always restore FPSCR */
	fmov.qd r0, dr0			/* Restore dr0 */
	ld.q	r2, _JB_USR, r1
	putcon	r1, usr
	or	r3, r63, r2		/* Fix up return value */
	ptabs/l	r18, tr0
	blink	tr0, r63

Lbotch:	LINK_FRAME(0)
	PIC_PROLOGUE
	PIC_PTAL(_C_LABEL(longjmperror), r0, tr0)
	blink	tr0, r18
	PIC_PTAL(_C_LABEL(abort), r0, tr0)
	blink	tr0, r18
	brk
