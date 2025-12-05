/*	$NetBSD: mmu_enable.s,v 1.1 2025/12/05 14:42:07 thorpej Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1980, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: locore.s 1.66 92/12/22$
 *
 *	@(#)locore.s	8.6 (Berkeley) 5/27/94
 */

/*
 * Code fragment for enabling the MMU.
 */

/*
 * NOTICE: This is not a standalone file.  To use it, #include it in
 * your port's locore.s, like so:
 *
 *	#include <m68k/m68k/mmu_enable.s>
 *
 * Your port's locore.s also must provide the local label "Lmmuenabled",
 * which will be used as a jump target after the MMU is enabled.
 *
 * This provides MMU enablement for 68020+68851, 68030, 68040, and 68060.
 */

	RELOC(Sysseg_pa, %a0)		| system segment table addr
	movl	%a0@,%d1		| read value (a PA)
#if defined(M68040) || defined(M68060)
	RELOC(mmutype, %a0)
	cmpl	#MMU_68040,%a0@		| 68040 or 68060?
	jgt	2f			| no, skip
	.long	0x4e7b1807		| movc %d1,%srp

	RELOC(mmu_tt40, %a0)		| pointer to TT reg values
	movl	%a0,%sp@-
	RELOC(mmu_load_tt40,%a0)	| pass it to mmu_load_tt40()
	jbsr	%a0@
	addql	#4,%sp

	.word	0xf4d8			| cinva bc
	.word	0xf518			| pflusha
	movl	#MMU40_TCR_BITS,%d0
	.long	0x4e7b0003		| movc %d0,%tc
#ifdef M68060
	RELOC(cputype, %a0)
	cmpl	#CPU_68060,%a0@		| 68060?
	jne	1f
	movl	#PCR_ESS,%d0		| enable superscalar dispatch
	.long	0x4e7b0808		| movcl %d0,%pcr
	movl	#CACHE60_ON,%d0
	movc	%d0,%cacr		| enable store buffer, I/D/B caches
	jmp	Lmmuenabled:l		| forced not be pc-relative
1:
#endif
	movl	#CACHE40_ON,%d0
	movc	%d0,%cacr		| turn on both caches
	jmp	Lmmuenabled:l		| forced not be pc-relative
2:
#endif /* M68040 || M68060 */

#if defined(M68020) || defined(M68030)
	RELOC(protorp, %a0)
	movl	%d1,%a0@(4)		| segtable address
	pmove	%a0@,%srp		| load the supervisor root pointer
#ifdef M68030
	RELOC(mmutype, %a0)
	cmpl	#MMU_68030,%a0@		| 68030?
	jne	1f			| no, skip
	RELOC(mmu_tt30, %a0)		| pointer to TT reg values
	movl	%a0,%sp@-
	RELOC(mmu_load_tt30,%a0)	| pass it to mmu_load_tt30()
	jbsr	%a0@ 
	addql	#4,%sp
1:
#endif /* M68030 */
	pflusha
	movl	#MMU51_TCR_BITS,%sp@	| value to load TC with
	pmove	%sp@,%tc		| load it
	jmp	Lmmuenabled:l		| forced not be pc-relative
#endif /* M68020 || M68030 */
