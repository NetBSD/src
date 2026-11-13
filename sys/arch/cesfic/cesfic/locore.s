/*	$NetBSD: locore.s,v 1.76 2026/11/13 01:23:13 thorpej Exp $	*/

/*
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
 * Copyright (c) 1994, 1995 Gordon W. Ross
 * Copyright (c) 1988 University of Utah.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

#include "opt_compat_netbsd.h"
#include "opt_compat_sunos.h"
#include "opt_ddb.h"
#include "opt_fpsp.h"
#include "opt_fpu_emulate.h"
#include "opt_m68k_arch.h"

#include "ksyms.h"
#include "assym.h"
#include <machine/asm.h>
#include <machine/trap.h>

/*
 * This is for kvm_mkdb, and should be the address of the beginning
 * of the kernel text segment (not necessarily the same as kernbase).
 */
	.text
GLOBAL(kernel_text)

/*
 * Temporary stack for a variety of purposes.
 * Try and make this the first thing is the data segment so it
 * is page aligned.  Note that if we overflow here, we run into
 * our text segment.
 */
	.data
	.space	PAGE_SIZE
ASGLOBAL(tmpstk)

	.text
/*
 * Macro to relocate a symbol, used before MMU is enabled.
 */
#define	_RELOC(var, ar)			\
	movl	__IMMEDIATE var,ar;	\
	addl	%a5,ar

#define	RELOC(var, ar)		_RELOC(_C_LABEL(var), ar)
#define	ASRELOC(var, ar)	_RELOC(_ASM_LABEL(var), ar)

/*
 * Initialization
 *
 * We are loaded via OS-9 at some address beyond what OS-9 is using,
 * and need to relocate ourselves down to our link address.
 */

BSS(lowram,4)
BSS(esym,4)

	.text
ASENTRY_NOPROFILE(start)
	movw	#PSL_HIGHIPL,%sr	| no interrupts

	movl	#CACHE_OFF,%d0
	movc	%d0,%cacr		| clear and disable on-chip cache(s)

	/* Determine where where we were loaded. */
	lea	%pc@(_ASM_LABEL(start)),%a5

	movl	#_ASM_LABEL(start),%a1	| destination address = link addr
	addl	#0x20000000,%a0		| (phys)
	movl	#_C_LABEL(edata),%a1	| destination limit
	addl	#0x20000000,%a1		| (phys)

	/* Copy text+data to run-time location. */
1:	movl	%a5@+,%a0@+
	cmpl	%a0,%a1			| dest == limit?
	bne	1b			| no, copy more

	/* Now jump into the relocated kernel. */
	movl	#_ASM_LABEL(Lrelocdone),%a0
	addl	#0x20000000,%a0		| (phys)
	jmp	%a0@
Lrelocdone:
	/* %a5 == start of RAM == relocation offset until MMU is enabled. */
	movl	#0x20000000,%a5

	/*
	 * NOTE: %a5 cannot be used until after the MMU is enabled; it
	 * is used by the RELOC() macro.
	 */

	ASRELOC(tmpstk, %a0)
	INIT_KSP(%a0)			| give ourselves a temporary stack

	RELOC(edata, %a0)		| clear out BSS
	RELOC(end, %a1)
1:	clrb	%a0@+
	cmpl	%a0, %a1
	bne	1b

#ifdef __HAVE_NEW_PMAP_68K
	/*
	 * We've linked the kernel at 0x2000 so that the first
	 * virtual page can remain unmapped.  We use those pages
	 * of RAM for LWP0's u-area.
	 */
	RELOC(kernel_text,%a1)		| %a1 = phys address of kernel_text
	subl	#USPACE,%a1		| %a1 -= USPACE
	RELOC(lwp0upa,%a0)
	movl	%a1,%a0@
#endif /* __HAVE_NEW_PMAP_68K */

	RELOC(esym, %a0)
	clrl	%a0@			| no symbol table from OS-9 loader

	RELOC(lowram, %a0)
	movl	%a5, %a0@		| store start of physical memory

	/*
	 * All FIC8234 boards have 68040s; don't bother probing
	 * for the CPU type.  The FPU will be checked later.
	 */

/* initialize memory size (for pmap_bootstrap1) */
	movl	0x5c00ac00, %d0
	andb	#0x60, %d0
	jne	Lnot8M
	movl	#0x00800000, %d1	| memory size, 8M
	jra	Lmemok
Lnot8M:
	cmpb	#0x20, %d0
	jne	Lunkmem
	movl	#0x02000000, %d1	| memory size, 32M
	jra	Lmemok
Lunkmem:
	/* ??? */
	movl	#0x00400000, %d1	| assume at least 4M
	
Lmemok:
	moveq	#PGSHIFT,%d2
	lsrl	%d2,%d1			| convert to page (click) number
	RELOC(physmem, %a0)		| physmem = %d1
	movl	%d1,%a0@

#if NKSYMS || defined(DDB) || defined(MODULAR)
	RELOC(esym,%a0)			| end of static kernel test/data/syms
	movl	%a0@,%a4
	tstl	%a4
	jne	1f
#endif
	movl	#_C_LABEL(end),%a4	| end of static kernel text/data
1:
	addl	%a5,%a4			| convert to PA
	pea	%a5@			| reloff
	pea	%a4@			| nextpa
	RELOC(pmap_bootstrap1,%a0)
	jbsr	%a0@			| pmap_bootstrap1(firstpa, nextpa)
	addql	#8,%sp

	/*
	 * Updated nextpa returned in %d0.  We need to squirrel
	 * that away in a callee-saved register to use later,
	 * after the MMU is enabled.
	 */
	movl	%d0, %d7

	/* NOTE: %d7 is now off-limits!! */

/*
 * Prepare to enable MMU.
 * Since the kernel is not mapped logical == physical, we must ensure
 * that when the MMU is turned on, all prefetched addresses (including
 * the PC) are valid.  In order to guarantee that, we use the transparent
 * translation registers (which provide VA == PA mappings) and just
 * turn on the MMU, then jump from the VA == PA address (at 0x20XXXXXX)
 * to the actual kernel virtual address (at 0x00XXXXXX) code via a far
 * jump instruction so that we can defeat the prefetch.
 *
 * The TT register values are pre-initialized in the mmu_tt40[]
 * array (see genassym.cf) and loaded from there before the MMU is
 * turned on.
 */
#include <m68k/m68k/mmu_enable.s>

/*
 * Should be running mapped from this point on
 */
Lmmuenabled:
/*
 * Now that the MMU is enabled, we need to clear out the TT values
 * from mmu_tt40[] that we don't want (so no other code gets confused
 * and thinks we're using them) and disable those TT registers.
 */
	lea	_ASM_LABEL(tmpstk),%a0	| re-load the temporary stack
	INIT_KSP(%a0)

	moveq	#0,%d0

	lea	_C_LABEL(mmu_tt40),%a0	| zap the 4 '040 TT slots
	movl	%d0,%a0@+
	movl	%d0,%a0@+
	movl	%d0,%a0@+
	movl	%d0,%a0@+

	/* disable '040 TTs */
	pea	_C_LABEL(mmu_tt40)
	jbsr	_C_LABEL(mmu_load_tt40)

	jbsr	_C_LABEL(vec_init)	| initialize vector table

	/* phase 2 of pmap setup, returns lwp0 SP in %a0 */
	jbsr	_C_LABEL(pmap_bootstrap2)
	INIT_KSP(%a0)			| now running on lwp0's stack

	movl	%d7,%sp@-		| push nextpa saved above
	jbsr	_C_LABEL(machine_init)	| additional pre-main initialization
	addql	#4,%sp
	jra	_C_LABEL(main)		| main() (never returns)

/*
 * Interrupt handlers.
 */

ENTRY_NOPROFILE(lev6intr)	/* Level 6: clock */
	INTERRUPT_SAVEREG
	/* XXX */
	movl _C_LABEL(clockbase), %a0
	movl %a0@, %d0
	movl %d0, %a0@
	btst #2, %d0
	jeq 1f
	addql	#1,_C_LABEL(m68k_intr_evcnt)+CLOCK_INTRCNT
	lea	%sp@(0), %a1		| a1 = &clockframe
	movl	%a1, %sp@-
	jbsr	_C_LABEL(hardclock)	| hardclock(&frame)
	addql	#4, %sp
	jra 2f
1:
	movl	%d0, %sp@-
	jbsr	_C_LABEL(otherclock)
	addql	#4, %sp
2:
	INTERRUPT_RESTOREREG
	jra	_ASM_LABEL(rei)		| all done

ENTRY_NOPROFILE(lev7intr)	/* level 7: parity errors, reset key */
	addql	#1,_C_LABEL(m68k_intr_evcnt)+NMI_INTRCNT
	clrl	%sp@-
	moveml	#0xFFFF,%sp@-		| save registers
	movl	%usp,%a0			| and save
	movl	%a0,%sp@(FR_SP)		|   the user stack pointer
	jbsr	_C_LABEL(nmihand)	| call handler
	movl	%sp@(FR_SP),%a0		| restore
	movl	%a0,%usp			|   user SP
	moveml	%sp@+,#0x7FFF		| and remaining registers
	addql	#8,%sp			| pop SP and stack adjust
	jra	_ASM_LABEL(rei)		| all done

ENTRY_NOPROFILE(machine_reboot)
	movl #0x5c00c060, %d0		| want phys addressing
	.long	0x4e7b0006		| movc d0,dtt0
	movl	#1, 0x5c00b800		| reset
	stop	#0x2700			| paranoia

	.data
GLOBAL(mmutype)
	.long	MMU_68040	| default to 68040
GLOBAL(cputype)
	.long	CPU_68040	| default to 68040
GLOBAL(prototc)
	.long	0		| prototype translation control

#ifdef DEBUG
	.globl	fulltflush, fullcflush
fulltflush:
	.long	0
fullcflush:
	.long	0
#endif
