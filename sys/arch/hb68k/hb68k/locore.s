/*	$NetBSD: locore.s,v 1.1 2026/07/19 01:48:21 thorpej Exp $	*/

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

#include "opt_m68k_arch.h"
#include "opt_hb68k_config.h"

#include "assym.h"
#include <machine/asm.h>
#include <machine/trap.h>

#include "ksyms.h"

/*
 * Temporary stack for a variety of purposes.
 * Try and make this the first thing is the data segment so it
 * is page aligned.  Note that if we overflow here, we run into
 * our text segment.
 */
	.data
	.space	PAGE_SIZE
ASGLOBAL(tmpstk)

/*
 * Macro to relocate a symbol, used before MMU is enabled.
 */
#define	IMMEDIATE		#
#define	_RELOC(var, ar)			\
	movl	IMMEDIATE var,ar;	\
	addl	%a5,ar

#define	RELOC(var, ar)		_RELOC(_C_LABEL(var), ar)
#define	ASRELOC(var, ar)	_RELOC(_ASM_LABEL(var), ar)

BSS(esym,4)

	.globl	_C_LABEL(edata)
	.globl	_C_LABEL(etext),_C_LABEL(end)

/*
 * This is for kvm_mkdb, and should be the address of the beginning
 * of the kernel text segment (not necessarily the same as kernbase).
 */
	.text
GLOBAL(kernel_text)

/*
 * start of kernel and .text!
 */
ASENTRY_NOPROFILE(start)
	movw	#PSL_HIGHIPL,%sr	| no interrupts

#ifndef __mc68010__
	movl	#CACHE_OFF,%d0
	movc	%d0,%cacr		| clear and disable on-chip cache(s)
#endif /* ! __mc68010__ */

	/*
	 * Determine our relocation offset.  We need this to manually
	 * translate the virtual addresses of global references to the
	 * physical addresses we need to use before the MMU is enabled.
	 */
	lea	%pc@(_ASM_LABEL(start)), %a5
	movl	%a5,%d0			| %d0 = phys address of start
	subl	#_ASM_LABEL(start), %d0	| %d0 -= virt address of start
	movl	%d0, %a5		| %a5 = relocation offset

	/*
	 * NOTE: %a5 cannot be used until after the MMU is enabled; it
	 * is used by the RELOC() macro.
	 */

	/*
	 * Stash away the computed relocation offset in case anyone
	 * needs to use it later.
	 */
	RELOC(reloc_offset,%a0)
	movl	%a5,%a0@

#if defined(CONFIG_MACH_PHAETHON1)
	/*
	 * When the firmware transfers control to the kernel, the
	 * stack looks like this:
	 *
	 *	[int]    bootinfo_size
	 *	[void *] bootinfo_ptr
	 *	[int]    MAGIC ('BnFo')
	 *	[int]    MAGIC ('pg68')
	 * SP->	[void *] return-address-back-to-firmware
	 *
	 * Check to see if the bootinfo was written at _end[].  If it
	 * wasn't, then the kernel was likely loaded from an S-Record
	 * file rather than an ELF image, and the S-Record file doesn't
	 * have the BSS segment in it.  The loader doesn't know any better
	 * (and doesn't know how big the BSS segment is, in any case), and
	 * so it plops the bootinfo at the end of where it finished loading.
	 *
	 * If we detect this, move the bootinfo into the expected location
	 * before clearing BSS.
	 */
	cmpl	#0x70673638,%sp@(4)	| 'pg68'
	bne	9f			| skip if magic is wrong
	cmpl	#0x4263466f,%sp@(8)	| 'BnFo'
	bne	9f

	movl	%sp@(12),%a1		| %a1 = bootinfo pointer
	RELOC(end,%a0)			| %a0 = end[]
	cmpal	%a0,%a1			| bootinfo pointer >= end[]?
	bcc	9f			| yes, skip

	movl	%sp@(16),%d0		| bootinfo size
	addl	%d0,%a1			| we're going to copy backwards
	addl	%d0,%a0
	subql	#1,%d0
1:
	movb	%a1@-,%a0@-		| relocate the bootinfo
	dbra	%d0,1b
9:
#endif /* CONFIG_MACH_PHAETHON1 */

	ASRELOC(tmpstk, %a0)
	movl	%a0,%sp			| give ourselves a temporary stack

	RELOC(edata,%a0)		| clear out BSS
	movl	#_C_LABEL(end) - 4, %d0	| (must be <= 256 kB)
	subl	#_C_LABEL(edata), %d0
	lsrl	#2,%d0
1:	clrl	%a0@+
	dbra	%d0,1b

#if defined(CONFIG_MACH_PHAETHON1)
	/*
	 * Initialize the ROM call interface early on this machine so
	 * that early kernel printf() works.
	 */
	RELOC(pgromcall_init,%a0)
	jbsr	%a0@
#endif

#if defined(CONFIG_LINUX_BOOTINFO)
	/*
	 * Linux-style bootinfo immediately follows the kernel.
	 * Go parse it to get CPU/FPU/MMU information and figure
	 * out where the end of the loaded image really is.  There
	 * may be other stuff in the bootinfo, as well, such as
	 * the location of symbols and an FDT blob.
	 */
	RELOC(end,%a4)			| end of static kernel text/data/bss
	pea	%a5@			| reloff
	pea	%a4@			| endpa
	RELOC(bootinfo_startup1,%a0)
	jbsr	%a0@			| bootinfo_startup1(endpa, reloff)
	addql	#8,%sp
	/*
	 * End of boot info returned in %d0.  That represents the
	 * end of the loaded image.
	 */
#elif defined(CONFIG_FDT_BOOTSTRAP)
	/*
	 * Linux-style bootinfo is much easier to parse, so it is
	 * preferred for the stuff needed very early (memory, etc.)
	 * If that's not available, then just use the device tree.
	 *
	 * N.B. this only works if the kernel is currently running
	 * where it's linked (this early, this usually means the
	 * kernel will be mapped VA==PA), due to the fact that there
	 * are several library functions that must be called in order
	 * to read the device tree.
	 *
	 * XXX Assuming it immediately follows the kernel.  Perhaps
	 * XXX bad assumption.
	 */
	RELOC(end,%a4)			| end of static kernel text/data/bss
	pea	%a5@			| reloff
	pea	%a4@			| endpa
	RELOC(fdt_bootstrap1,%a0)
	jbsr	%a0@			| fdt_bootstrap1(endpa, reloff)
	addql	#8,%sp
	/*
	 * End of loaded image returned in %d0.
	 */
#else
#error A bootstrap routine for system information is needed.
#endif

	pea	%a5@			| reloff
	movl	%d0,%sp@-		| nextpa
	RELOC(pmap_bootstrap1,%a0)
	jbsr	%a0@			| pmap_bootstrap1(nextpa, reloff)
	addql	#8,%sp

	/*
	 * Updated nextpa returned in %d0.  We need to squirrel
	 * that away in a callee-saved register to use later,
	 * after the MMU is enabled.
	 */
	movl	%d0,%d7

	/* NOTE: %d7 is now off-limits!! */

#if defined(M68K_MMU_MOTOROLA)
/*
 * Enable the MMU.
 * We're making an assumption here that the kernel is mapped VA==PA, so
 * there is no prep work to do.  This is not necessarily a good assumption,
 * and as new homebrew platforms are added there, there may need to be
 * ajustments.
 */
#include <m68k/m68k/mmu_enable.s>
#else
/*
 * We're making an assumption here that this is a system on which the
 * MMU is already enabled by the firmware or boot loader.  If that's
 * not the case, then we'll need another CONFIG conditional here.
 */
#endif /* M68K_MMU_MOTOROLA */

/*
 * Should be running mapped from this point on
 */
Lmmuenabled:
	lea	_ASM_LABEL(tmpstk),%sp	| re-load the temporary stack
	jbsr	_C_LABEL(vec_init)	| initialize the vector table

	/* phase 2 of pmap setup, returns lwp0 SP in %a0 */
	jbsr	_C_LABEL(pmap_bootstrap2)
	movl	%a0,%sp			| now running on lwp0's stack
	movl	#0,%a6			| terminate the stack back trace

	movl	%d7,%sp@-		| push nextpa saved above
	jbsr	_C_LABEL(machine_init)	| additional pre-main initialization
	addql	#4,%sp
	jra	_C_LABEL(main)		| main() (never returns)

/*
 * Misc. global variables.
 */
	.data

GLOBAL(mmutype)
#ifdef __mc68010__
	.long	MMU_CUSTOM	| default to custom MMU
#else
	.long	MMU_68030	| default to MMU_68030
#endif

GLOBAL(cputype)
#ifdef __mc68010__
	.long	CPU_68010	| default to CPU_68010
#else
	.long	CPU_68030	| default to CPU_68030
#endif

GLOBAL(reloc_offset)
	.long	0		| relocation offset (see RELOC macro above)
