/*	$NetBSD: locore.S,v 1.12 2003/01/20 22:25:09 scw Exp $	*/

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

#include "opt_sh5_debug.h"

#include "assym.h"
#include <machine/asm.h>

/*
 * The main entry point.
 *
 * The bootloader (to be written) will have stashed the boot parameters
 * in r56 - r62
 */
	.globl	_C_LABEL(kernel_text)
_C_LABEL(kernel_text):

ASENTRY_NOPROFILE(text)
ASENTRY_NOPROFILE(start)
	getcon	sr, r1
	LDC32(SH5_CONREG_SR_IMASK_ALL|SH5_CONREG_SR_BL|SH5_CONREG_SR_MD, r0)
	or	r0, r1, r0
	putcon	r0, ssr
	pta/u	1f, tr0			/* PC-Relative */
	pta/u	1f, tr1
	pta/u	1f, tr2
	pta/u	1f, tr3
	pta/u	1f, tr4
	pta/u	1f, tr5
	pta/u	1f, tr6
	pta/u	1f, tr7
	gettr	tr0, r0
	putcon	r0, spc
	synco
	rte
	nop
	nop
	nop
	nop
	nop

/*
 * The SR is sane. Go initialise the TLB, Cache and MMU.
 */
1:	LEAF(Lmapped_start, r18)
	LDUC32(0x80000000, r2)
	pta/l	Lsh5_stb1_init, tr0
	blink	tr0, r63

/*
 * We are now running mapped in KSEG0, with the MMU enabled.
 */
Lmapped_start:
#ifndef SH5_SIM
	LDC32(0x08000000, r56)		/* RAM size. XXX: Can we probe this? */
#else
	LDC32(0x01000000, r56)		/* Fake 16MB for the simulator */
#endif
	LEA(_C_LABEL(edata), r0)
	LEA(_C_LABEL(end), r1)
	pta/u	1f, tr0
1:	st.q	r0, 0, r63		/* Clear BSS */
	addi	r0, 8, r0
	bgtu/l	r1, r0, tr0

	LEA(_C_LABEL(evbsh5_bootparams), r0)
	st.q	r0, 0, r56		/* Save boot parameters */
	st.q	r0, 8, r57
	st.q	r0, 16, r58
	st.q	r0, 24, r59
	st.q	r0, 32, r60
	st.q	r0, 40, r61
	st.q	r0, 48, r62

	/*
	 * Do generic runtime environment initialisation
	 */

	LEAF(Lsh5_init_env, r0)
	ptabs/l	r0, tr0
	blink	tr0, r18

	/*
	 * Arrange to catch panic traps with the mmu off.
	 */
	LEA(_C_LABEL(sh5_resvec_vector_table), r0)
	LEA(0x40000000, r1)
	sub	r0, r1, r0
	ori	r0, 1, r0
	putcon  r0, resvec

	/* Now safe to call C code */

	/*
	 * Do basic initialisation, including setting up the pmap.
	 *
	 * void evbsh5_init(vaddr_t endkernel)
	 */
	LEA(_C_LABEL(end), r2)
	LEAF(_C_LABEL(evbsh5_init), r0)
	ptabs/l	r0, tr0
	blink	tr0, r18

	/*
	 * Just about set to call main(). All that's left is to
	 * enable interrupts.
	 */
	getcon	sr, r1
	movi	SH5_CONREG_SR_IMASK_ALL, r0
	andc	r1, r0, r1
	putcon	r1, sr

	LEAF(_C_LABEL(main), r0)	/* Call main(), never to return */
	ptabs/l	r0, tr0
	blink	tr0, r18
	/*NOTREACHED*/

	pta/l	1f, tr0			/* But just in case ... */
1:	nop
	blink	tr0, r63


	.comm	Lmem_regions,SIZEOF_MEM_REGION*2,8


/*
 * Pull in the common SH5 locore code
 */

#include <sh5/sh5/exception.S>
#include <sh5/sh5/locore_subr.S>
#include <sh5/sh5/cpu_switch.S>
#include <sh5/sh5/stb1_locore.S>
#include <sh5/sh5/sigcode.S>
#include <sh5/sh5/bus_subr.S>


/*===================== Start of RESVEC Vector Table =========================*/

	_TEXT_SECTION
	.balign	0x10

GLOBAL(sh5_resvec_vector_table)

/******************************************************************************
 * Reset/Panic Exception Vector.
 *
 * RESVEC Offset: 0x0
 *        Length: 0x100
 */
Lsh5_resvec_vector_panic:
	getcon	usr, r24
	putcon	r15, kcr1
	LEA(_C_LABEL(sh5_panic_stack) - (0x40000000-(USPACE-SZ_TRAPFRAME)), r15)
	st.q	r15, SFO(SF_USR,SZ_TRAPFRAME), r24	/* Save USR */
	st.q	r15, IFO(IF_R0,SZ_TRAPFRAME), r0	/* Save r0 */
	st.q	r15, IFO(IF_R1,SZ_TRAPFRAME), r1	/* Save r1 */
	st.q	r15, IFO(IF_R2,SZ_TRAPFRAME), r2	/* Save r2 */
	getcon	kcr1, r0
	st.q	r15, IFO(IF_R15,SZ_TRAPFRAME), r0	/* Save r15 */
	gettr	tr0, r0
	st.q	r15, IFO(IF_TR0,SZ_TRAPFRAME), r0	/* Save tr0 */
	getcon	pssr, r0
	st.q	r15, SFO(SF_SSR,SZ_TRAPFRAME), r0	/* Save PSSR */
	getcon	pspc, r0
	st.q	r15, SFO(SF_SPC,SZ_TRAPFRAME), r0	/* Save PSPC */
	getcon	pexpevt, r0
	st.q	r15, SFO(SF_EXPEVT, SZ_TRAPFRAME), r0	/* Save PEXPEVT */
	getcon	intevt, r0
	st.q	r15, SFO(SF_INTEVT, SZ_TRAPFRAME), r0	/* Save INTEVT */
	getcon	tea, r0
	st.q	r15, SFO(SF_TEA, SZ_TRAPFRAME), r0	/* Save TEA */
	getcon	tra, r0
	st.q	r15, SFO(SF_TRA, SZ_TRAPFRAME), r0	/* Save TRA */
	movi	0, r2
	pta/l	Lsh5_event_panic, tr0
	blink	tr0, r63

	.space	(0x100 - (. - Lsh5_resvec_vector_panic))


/******************************************************************************
 * Debug Synchronous Exception Handler
 *
 * RESVEC Offset: 0x100
 *        Length: 0x100
 *
 * We have to handle these as regular synchronous exceptions since a BRK
 * instruction from userland results one of these.
 *
 * Trouble is, we're now running with the MMU disabled, so we do the minimum
 * necessary to re-enable the MMU before jumping to the regular synchronous
 * exception handler.
 */
Lsh5_resvec_vector_dbg_general:
	getcon	usr, r24		/* Stash USR somewhere safe for now */
	putcon	r0, kcr1		/* Save r0 */
	getcon	spc, r0			/* Fetch PC at point of exception */
	putcon	r0, pspc		/* Make it available to real handler */
	getcon	ssr, r0			/* Fetch SR at point of exception */
	putcon	r0, pssr		/* Make it available to real handler */
	getcon	vbr, r0
	addi	r0, 0x101, r0		/* The real Synchronous Exc. handler */
	putcon	r0, spc
	getcon	sr, r0			/* Arrange to set SR.MMU */
	shlli	r0, 32, r0
	byterev	r0, r0			/* This trick avoids the need to have */
	ori	r0, 0x80, r0		/* another register available to load */
	byterev	r0, r0			/* a 32-bit constant into. */
	shlri	r0, 32, r0
	putcon	r0, ssr
	getcon	kcr1, r0		/* Restore r0 */
	putcon	r24, usr		/* Restore USR */
	synco
	rte				/* Enable the MMU and jump to handler */
	nop
	nop
	nop
	nop
	nop

	.space	(0x100 - (. - Lsh5_resvec_vector_dbg_general))


/******************************************************************************
 * Debug Interrupt Handler.
 *
 * RESVEC Offset: 0x200
 *        Length: 0x200
 *
 * Deal with these the same way as above.
 */
Lsh5_resvec_vector_debugint:
	getcon	usr, r24		/* Stash USR somewhere safe for now */
	putcon	r0, kcr1		/* Save r0 */
	getcon	spc, r0			/* Fetch PC at point of exception */
	putcon	r0, pspc		/* Make it available to real handler */
	getcon	ssr, r0			/* Fetch SR at point of exception */
	putcon	r0, pssr		/* Make it available to real handler */
	getcon	vbr, r0
	addi	r0, 0x1ff, r0
	addi	r0, 0x1ff, r0
	addi	r0, 0x1ff, r0
	addi	r0, 0x004, r0		/* The real Asynchronous Exc. handler */
	putcon	r0, spc
	getcon	sr, r0			/* Arrange to set SR.MMU */
	shlli	r0, 32, r0
	byterev	r0, r0			/* This trick avoids the need to have */
	ori	r0, 0x80, r0		/* another register available to load */
	byterev	r0, r0			/* a 32-bit constant into. */
	shlri	r0, 32, r0
	putcon	r0, ssr
	getcon	kcr1, r0		/* Restore r0 */
	putcon	r24, usr		/* Restore USR */
	synco
	rte				/* Enable the MMU and jump to handler */
	nop
	nop
	nop
	nop
	nop



/******************************************************************************
 * Continuation of Panic Event.
 *
 * Save the remainder of the machine state, re-enable the MMU and head off
 * into C code to report the problem on the console (if possible), never
 * to return.
 */
Lsh5_event_panic:
	_INTR_FRAME_SAVE(SZ_TRAPFRAME)
	_TRAP_FRAME_SAVE(SZ_TRAPFRAME)
	getcon	ssr, r3		/* Parameters for panic_trap() */
	getcon	spc, r4
	getcon	expevt, r5
	or	r2, r63, r6
	LDC32(0x01600000, r0)	/* Disable (freeze) the caches */
	putcfg	r0, 0, r63
	LDC32(0x01e00000, r0)
	putcfg	r0, 0, r63
	LEAF(1f, r0)
	putcon	r0, spc
	LDUC32(SH5_CONREG_SR_MD|SH5_CONREG_SR_MMU|SH5_CONREG_SR_IMASK_ALL, r0)
	putcon	r0, ssr
	LEA(_C_LABEL(sh5_panic_stack), r15)
	movi	(USPACE - SZ_TRAPFRAME), r0
	add	r15, r0, r15
	synco
	rte			/* Re-enable the MMU */
	nop
	nop
	nop
	nop
	nop
1:	LEAF(_C_LABEL(panic_trap), r0)
	ptabs/l	r0, tr0
	or	r15, r63, r2
	blink	tr0, r63
	/*NOTREACHED*/

	.comm	_C_LABEL(sh5_panic_stack),USPACE,16


#include "dtfcons.h"

#if NDTFCONS > 0
	.section .data
	.balign 64
GLOBAL(_evbsh5_dtf_buffer)
	.space	4096,0
#endif

	.data
	.balign 4

/*
 * Interrupt names for Cayman
 *
 * These are here because some userland tools insist on grovelling
 * for intrnames/eintrnames.
 */
GLOBAL(intrnames)
	.asciz	"spurious"
	.asciz	"softmisc"
	.asciz	"softclock"
	.asciz	"softnet"
	.asciz	"irq4"
	.asciz	"irq5"
	.asciz	"irq6"
	.asciz	"irq7"
	.asciz	"irq8"
	.asciz	"irq9"
	.asciz	"softserial"
	.asciz	"irq11"
	.asciz	"scif"
	.asciz	"irq13"
	.asciz	"clock"
	.asciz	"irq15"
GLOBAL(eintrnames)
