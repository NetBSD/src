/*	$NetBSD: mmu_subr.s,v 1.2 2024/01/09 07:28:26 thorpej Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * Copyright (c) 1980, 1990, 1993
 *      The Regents of the University of California.  All rights reserved.
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
 * void mmu_load_urp(paddr_t urp);
 *
 *	Load the user root pointer into the MMU.  A version is provided
 *	for each supported MMU type.
 *
 *	We keep the HP MMU versions of these routines here, as well,
 *	even though they'll only ever be used on the hp300.
 */

#include "opt_m68k_arch.h"

#include <machine/asm.h>

#include "assym.h"

	.file	"mmu_subr.s"
	.text

#if defined(M68K_MMU_MOTOROLA)
#if defined(M68020) || defined(M68030)
	.data
/*
 * protorp is set up to initialize the Supervisor Root Pointer.
 * pmap_init() will re-configure it to load the CPU Root Pointer.
 */
GLOBAL(protorp)
	.long	MMU51_SRP_BITS,0	| prototype CPU root pointer

	.text
ENTRY_NOPROFILE(mmu_load_urp51)
	movl	%sp@(4),%d0		| get root table PA argument
	lea	_C_LABEL(protorp),%a0	| CRP prototype
	movl	%d0,%a0@(4)		| set new root table PA
	pflusha				| flush ATC
	pmove	%a0@,%crp		| load CRP register
	movl	#CACHE_CLR,%d0
	movc	%d0,%cacr		| invalidate caches
	rts
#endif /* M68020 || M68030 */

#if defined(M68040) || defined(M68060)
#if defined(M68060)
ENTRY_NOPROFILE(mmu_load_urp60)
	movc	%cacr,%d0
	orl	#IC60_CUBC,%d0		| clear user branch cache entries
	movc	%d0,%cacr
	/* FALLTHROUGH */
#endif /* M68060 */
ENTRY_NOPROFILE(mmu_load_urp40)
	movl	%sp@(4),%d0		| get root table PA argument
	.word 0xf518	 |pflusha	| flush ATC
	.long 0x4e7b0806 |movc %d0,%urp	| load the URP register
	rts
#endif /* M68040 || M68060 */
#endif /* M68K_MMU_MOTOROLA */

#if defined(M68K_MMU_HP)
ENTRY_NOPROFILE(mmu_load_urp20hp)
	movl	#CACHE_CLR,%d0
	movc	%d0,%cacr		| invalidate caches
	movl	_C_LABEL(MMUbase),%a0
	movl	%a0@(MMUTBINVAL),%d1	| invalidate TLB
	tstl	_C_LABEL(ectype)	| have an external VAC?
	jle	1f			| no, skip
	andl	#~MMU_CEN,%a0@(MMUCMD)	| toggle cache enable
	orl	#MMU_CEN,%a0@(MMUCMD)	| to clear data cache
1:	movl	%sp@(4),%d0		| get root table PA argument
	moveq	#PGSHIFT,%d1
	lsrl	%d1,%d0			| convert to page frame number
	movl	%d0,%a0@(MMUUSTP)	| load to USTP register
	rts
#endif /* M68K_MMU_HP */
