/*	$NetBSD: locore.S,v 1.13 2003/03/13 13:44:20 scw Exp $	*/

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
 *   void start(long		    kseg0_phys,		r2
 *		void		   *symtab,		r3
 *		vaddr_t		    end_kernel);	r4
 *
 * To support the simulator, or when the kernel is loaded directly into
 * memory via JTAG by the debugger, we check for end_kernel = 0 and use
 * a default kseg0_phys value if necessary. C code will assume ddb
 * symbols are in SYMTAB_SPACE in this case.
 *
 * If we were booted by the bootloader, our boot_params structure will
 * be located at 'end_kernel'.
 */
	.globl	_C_LABEL(kernel_text)
_C_LABEL(kernel_text):
ASENTRY_NOPROFILE(start)
ASENTRY_NOPROFILE(text)

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
 * Now initialise the TLB, Cache and MMU.
 *
 * If we were loaded by a real bootloader, the physical address of KSEG0
 * will be in r2. However, we might have been loaded over jtag, in which
 * case the registers will be zero. Or, we might be running in the
 * simulator, in which case the registers will be == 0xdeadbeefdeadbeef.
 */
1:	LDSC32(0x80000000, r0)
	cmveq	r4, r0, r2
	LDSC64(0xdeadbeefdeadbeef, r1)
	cmpeq	r1, r2, r1
	cmvne	r1, r0, r2
	cmvne	r1, r63, r3
	LEA(_C_LABEL(end), r0)
	cmvne	r1, r0, r4

	LEAF(Lmapped_start, r18)
	add.l	r2, r63, r60		/* Save physical load address */
	add.l	r3, r63, r61		/* Save symbol table start */
	add.l	r4, r63, r62		/* Save end kernel */
	pta/l	Lsh5_stb1_init, tr0
	blink	tr0, r63

/*
 * We are now running mapped in KSEG0, with the MMU enabled.
 */
Lmapped_start:
	LEA(_C_LABEL(edata), r0)
	LEA(_C_LABEL(end), r1)
	cmveq	r62, r1, r62		/* Fix up end_kernel, if necessary */
	pta/u	1f, tr0
1:	st.q	r0, 0, r63		/* Clear BSS */
	addi	r0, 8, r0
	bgtu/l	r1, r0, tr0

	/*
	 * Do generic runtime environment initialisation
	 */

	LEAF(Lsh5_init_env, r0)
	ptabs/l	r0, tr0
	add	r60, r63, r2
	blink	tr0, r18

	/* Now safe to call C code */

	/*
	 * Do basic initialisation, including setting up the pmap.
	 *
	 * void evbsh5_init(void *symtab, vaddr_t endkernel)
	 */
	or	r61, r63, r2
	or	r62, r63, r3
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


/*
 * Pull in the common SH5 locore code
 */

#include <sh5/sh5/exception.S>
#include <sh5/sh5/locore_subr.S>
#include <sh5/sh5/cpu_switch.S>
#include <sh5/sh5/stb1_locore.S>
#include <sh5/sh5/sigcode.S>
#include <sh5/sh5/bus_subr.S>


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
