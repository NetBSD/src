/*	$NetBSD: locore.S,v 1.1 2002/07/05 13:31:40 scw Exp $	*/

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

#include "assym.h"
#include <machine/asm.h>

	.section	.text
	.align 3


/*
 * The main entry point.
 *
 * The bootloader (to be written) will have stashed the boot parameters
 * in r56 - r62
 */
GLOBAL(text)
	.globl start
start:
	LDC32(SH5_CONREG_SR_IMASK_ALL|SH5_CONREG_SR_BL|SH5_CONREG_SR_MD, r0)
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
	LDC32(0x02000000, r56)
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

	/* Now safe to call C code */

	/*
	 * Do basic pre-pmap initialisation. This is mostly to do with
	 * sizing physical memory.
	 *
	 * vaddr_t evbsh5_memory_init(vaddr_t endkernel, struct mem_region *mr);
	 */
	LEA(_C_LABEL(end), r2)
	LEA(Lmem_regions, r3)
	LEAF(_C_LABEL(evbsh5_memory_init), r0)
	ptabs/l	r0, tr0
	blink	tr0, r18

	/*
	 * Bootstrap the pmap:
	 *
	 * void pmap_bootstrap(vaddr_t avail, struct mem_region *mr);
	 */
	LEA(Lmem_regions, r3)
	LEAF(_C_LABEL(pmap_bootstrap), r0)
	ptabs/l	r0, tr0
	blink	tr0, r18

	/*
	 * We can now deal with exceptions, particularly TLB misses
	 */
	getcon	sr, r1
	movi	1, r0
	shlli	r0, SH5_CONREG_SR_BL_SHIFT, r0
	andc	r1, r0, r1
	putcon	r1, sr

	/*
	 * Secondary initialisation. The pmap has been set up enough
	 * to map device register into kernel virtual address space.
	 */
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

#include "dtfcons.h"

#if NDTFCONS > 0
	.section .data
	.balign 64
GLOBAL(_evbsh5_dtf_buffer)
	.space	4096,0
#endif
