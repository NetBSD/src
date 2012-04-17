/*	$NetBSD: locore.s,v 1.159.4.2 2012/04/17 00:06:20 yamt Exp $	*/

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
#include "opt_compat_svr4.h"
#include "opt_compat_sunos.h"
#include "opt_ddb.h"
#include "opt_fpsp.h"
#include "opt_kgdb.h"
#include "opt_lockdebug.h"
#include "opt_fpu_emulate.h"
#include "opt_m68k_arch.h"

#include "assym.h"
#include <machine/asm.h>
#include <machine/trap.h>

#include "opt_useleds.h"
#ifdef USELEDS
#include <hp300/hp300/leds.h>
#endif

#include "ksyms.h"

#define MMUADDR(ar)	movl	_C_LABEL(MMUbase),ar
#define CLKADDR(ar)	movl	_C_LABEL(CLKbase),ar

/*
 * This is for kvm_mkdb, and should be the address of the beginning
 * of the kernel text segment (not necessarily the same as kernbase).
 */
	.text
GLOBAL(kernel_text)

/*
 * Clear and skip the first page of text; it will not be mapped at
 * VA 0.
 *
 * The bootloader places the bootinfo in this page, and we allocate
 * a VA for it and map it later.
 */
	.fill	PAGE_SIZE/4,4,0

/*
 * Temporary stack for a variety of purposes.
 * Try and make this the first thing is the data segment so it
 * is page aligned.  Note that if we overflow here, we run into
 * our text segment.
 */
	.data
	.space	PAGE_SIZE
ASLOCAL(tmpstk)

#include <hp300/hp300/vectors.s>

/*
 * Macro to relocate a symbol, used before MMU is enabled.
 */
#ifdef __STDC__
#define	IMMEDIATE		#
#define	_RELOC(var, ar)			\
	movel	IMMEDIATE var,ar;	\
	addl	%a5,ar
#else
#define	_RELOC(var, ar)			\
	movel	#var,ar;		\
	addl	%a5,ar
#endif /* __STDC__ */

#define	RELOC(var, ar)		_RELOC(_C_LABEL(var), ar)
#define	ASRELOC(var, ar)	_RELOC(_ASM_LABEL(var), ar)

/*
 * Final bits of grunt work required to reboot the system.  The MMU
 * must be disabled when this is invoked.
 */
#define DOREBOOT						\
	/* Reset Vector Base Register to what PROM expects. */  \
	movl	#0,%d0;						\
	movc	%d0,%vbr;					\
	/* Jump to REQ_REBOOT */				\
	jmp	0x1A4;

/*
 * Initialization
 *
 * A4 contains the address of the end of the symtab
 * A5 contains physical load point from boot
 * VBR contains zero from ROM.  Exceptions will continue to vector
 * through ROM until MMU is turned on at which time they will vector
 * through our table (vectors.s).
 */

BSS(lowram,4)
BSS(esym,4)

ASENTRY_NOPROFILE(start)
	movw	#PSL_HIGHIPL,%sr	| no interrupts
	ASRELOC(tmpstk, %a0)
	movl	%a0,%sp			| give ourselves a temporary stack
	RELOC(esym, %a0)
#if 1
	movl	%a4,%a0@		| store end of symbol table
#else
	clrl	%a0@			| no symbol table, yet
#endif
	RELOC(lowram, %a0)
	movl	%a5,%a0@		| store start of physical memory
	movl	#CACHE_OFF,%d0
	movc	%d0,%cacr		| clear and disable on-chip cache(s)

/* check for internal HP-IB in SYSFLAG */
	btst	#5,0xfffffed2		| internal HP-IB?
	jeq	Lhaveihpib		| yes, have HP-IB just continue
	RELOC(internalhpib, %a0)
	movl	#0,%a0@			| no, clear associated address
Lhaveihpib:

	RELOC(boothowto, %a0)		| save reboot flags
	movl	%d7,%a0@
	RELOC(bootdev, %a0)		|   and boot device
	movl	%d6,%a0@

	/*
	 * All data registers are now free.  All address registers
	 * except %a5 are free.  %a5 is used by the RELOC() macro,
	 * and cannot be used until after the MMU is enabled.
	 */

/* determine our CPU/MMU combo - check for all regardless of kernel config */
	movl	#INTIOBASE+MMUBASE,%a1
	movl	#DC_FREEZE,%d0		| data freeze bit
	movc	%d0,%cacr		|   only exists on 68030
	movc	%cacr,%d0		| read it back
	tstl	%d0			| zero?
	jeq	Lnot68030		| yes, we have 68020/68040

	/*
	 * 68030 models
	 */

	RELOC(mmutype, %a0)		| no, we have 68030
	movl	#MMU_68030,%a0@		| set to reflect 68030 PMMU
	RELOC(cputype, %a0)
	movl	#CPU_68030,%a0@		| and 68030 CPU
	RELOC(machineid, %a0)
	movl	#0x80,%a1@(MMUCMD)	| set magic cookie
	movl	%a1@(MMUCMD),%d0	| read it back
	btst	#7,%d0			| cookie still on?
	jeq	Lnot370			| no, 360, 362 or 375
	movl	#0,%a1@(MMUCMD)		| clear magic cookie
	movl	%a1@(MMUCMD),%d0	| read it back
	btst	#7,%d0			| still on?
	jeq	Lisa370			| no, must be a 370
	movl	#HP_340,%a0@		| yes, must be a 340
	jra	Lstart1
Lnot370:
	movl	#HP_360,%a0@		| type is at least a 360
	movl	#0,%a1@(MMUCMD)		| clear magic cookie2
	movl	%a1@(MMUCMD),%d0	| read it back
	btst	#16,%d0			| still on?
	jeq	Lisa36x			| no, must be a 360 or a 362
	RELOC(mmuid, %a0)		| save MMU ID
	lsrl	#MMUID_SHIFT,%d0
	andl	#MMUID_MASK,%d0
	movl	%d0,%a0@
	RELOC(machineid, %a0)
	cmpb	#MMUID_345,%d0		| are we a 345?
	beq	Lisa345
	cmpb	#MMUID_375,%d0		| how about a 375?
	beq	Lisa375
	movl	#HP_400,%a0@		| must be a 400
	jra	Lhaspac
Lisa36x:
	/*
	 * If we found a 360, we need to check for a 362 (neither the 360
	 * nor the 362 have a nonzero mmuid). Identify 362 by checking
	 * on-board VRX framebuffer which has secid 0x11 at dio scode 132.
	 */
	movl	#DIOII_BASE,%a0		| probe dio scode 132
	ASRELOC(phys_badaddr,%a3)
	jbsr	%a3@
	tstl	%d0			| device at scode 132?
	jne	Lstart1			| no, not 362, assume 360
	movb	%a0@(DIO_IDOFF),%d0
	cmpb	#DIO_DEVICE_ID_FRAMEBUFFER,%d0	| framebuffer?
	jne	Lstart1			| no, not 362, assume 360
	movb	%a0@(DIO_SECIDOFF),%d0
	cmpb	#0x11,%d0		| VRX sti on 362?
	jne	Lstart1			| no, not 362, assume 360
	RELOC(machineid,%a0)
	movl	#HP_362,%a0@
	jra	Lstart1
Lisa345:
	movl	#HP_345,%a0@
	jra	Lhaspac
Lisa375:
	movl	#HP_375,%a0@
	jra	Lhaspac
Lisa370:
	movl	#HP_370,%a0@		| set to 370
Lhaspac:
	RELOC(ectype, %a0)
	movl	#EC_PHYS,%a0@		| also has a physical address cache
	jra	Lstart1

	/*
	 * End of 68030 section
	 */

Lnot68030:
	bset	#31,%d0			| data cache enable bit
	movc	%d0,%cacr		|   only exists on 68040
	movc	%cacr,%d0		| read it back
	tstl	%d0			| zero?
	beq	Lis68020		| yes, we have 68020
	moveq	#0,%d0			| now turn it back off
	movec	%d0,%cacr		|   before we access any data

	/*
	 * 68040 models
	 */

	RELOC(mmutype, %a0)
	movl	#MMU_68040,%a0@		| with a 68040 MMU
	RELOC(cputype, %a0)
	movl	#CPU_68040,%a0@		| and a 68040 CPU
	RELOC(fputype, %a0)
	movl	#FPU_68040,%a0@		| ...and FPU
	RELOC(ectype, %a0)
	movl	#EC_NONE,%a0@		| and no cache (for now XXX)
	RELOC(mmuid,%a0)		| save MMU ID
	movl	%a1@(MMUCMD),%d0
	lsrl	#MMUID_SHIFT,%d0
	andl	#MMUID_MASK,%d0
	movl	%d0,%a0@
	RELOC(machineid, %a0)
	cmpb	#MMUID_425_T,%d0	| are we a 425t?
	jeq	Lisa425
	cmpb	#MMUID_425_S,%d0	| how about 425s?
	jeq	Lisa425
	cmpb	#MMUID_425_E,%d0	| or maybe a 425e?
	jeq	Lisa425
	cmpb	#MMUID_433_T,%d0	| or a 433t?
	jeq	Lisa433
	cmpb	#MMUID_433_S,%d0	| or a 433s?
	jeq	Lisa433
	cmpb	#MMUID_385,%d0		| or a 385?
	jeq	Lisa385
	cmpb	#MMUID_382,%d0		| or a 382?
	jeq	Lisa382
	movl	#HP_380,%a0@		| guess we're a 380
	jra	Lstart1
Lisa425:
	movl	#HP_425,%a0@
	jra	Lstart1
Lisa433:
	movl	#HP_433,%a0@
	jra	Lstart1
Lisa385:
	movl	#HP_385,%a0@
	jra	Lstart1
Lisa382:
	movl	#HP_382,%a0@
	jra	Lstart1

	/*
	 * End of 68040 section
	 */

	/*
	 * 68020 models
	 */

Lis68020:
	RELOC(fputype, %a0)		| all of the 68020 systems
	movl	#FPU_68881,%a0@		|   have a 68881 FPU
	movl	#1,%a1@(MMUCMD)		| a 68020, write HP MMU location
	movl	%a1@(MMUCMD),%d0	| read it back
	btst	#0,%d0			| non-zero?
	jne	Lishpmmu		| yes, we have HP MMU
	RELOC(mmutype, %a0)
	movl	#MMU_68851,%a0@		| no, we have PMMU
	RELOC(machineid, %a0)
	movl	#HP_330,%a0@		| and 330 CPU
	jra	Lstart1
Lishpmmu:
	RELOC(ectype, %a0)		| 320 or 350
	movl	#EC_VIRT,%a0@		| both have a virtual address cache
	movl	#0x80,%a1@(MMUCMD)	| set magic cookie
	movl	%a1@(MMUCMD),%d0	| read it back
	btst	#7,%d0			| cookie still on?
	jeq	Lis320			| no, just a 320
	RELOC(machineid, %a0)
	movl	#HP_350,%a0@		| yes, a 350
	jra	Lstart1
Lis320:
	RELOC(machineid, %a0)
	movl	#HP_320,%a0@

	/*
	 * End of 68020 section
	 */

Lstart1:
	/*
	 * Now that we know what CPU we have, initialize the address error
	 * and bus error handlers in the vector table:
	 *
	 *	vectab+8	bus error
	 *	vectab+12	address error
	 */
	RELOC(cputype, %a0)
#if 0
	/* XXX assembler/linker feature/bug */
	RELOC(vectab, %a2)
#else
	movl	#_C_LABEL(vectab),%a2
	addl	%a5,%a2
#endif
#if defined(M68040)
	cmpl	#CPU_68040,%a0@		| 68040?
	jne	1f			| no, skip
	movl	#_C_LABEL(buserr40),%a2@(8)
	movl	#_C_LABEL(addrerr4060),%a2@(12)
	jra	Lstart2
1:
#endif
#if defined(M68020) || defined(M68030)
	cmpl	#CPU_68040,%a0@		| 68040?
	jeq	1f			| yes, skip
	movl	#_C_LABEL(busaddrerr2030),%a2@(8)
	movl	#_C_LABEL(busaddrerr2030),%a2@(12)
	jra	Lstart2
1:
#endif
	/* Config botch; no hope. */
	DOREBOOT

Lstart2:
	movl	#0,%a1@(MMUCMD)		| clear out MMU again
/* initialize source/destination control registers for movs */
	moveq	#FC_USERD,%d0		| user space
	movc	%d0,%sfc		|   as source
	movc	%d0,%dfc		|   and destination of transfers
/* save the first PA as bootinfo_pa to map it to a virtual address later. */
	movl	%a5,%d0			| lowram value from ROM via boot
	RELOC(bootinfo_pa, %a0)
	movl	%d0,%a0@		| save the lowram as bootinfo PA
/* initialize memory sizes (for pmap_bootstrap) */
	movl	#MAXADDR,%d1		| last page
	moveq	#PGSHIFT,%d2
	lsrl	%d2,%d1			| convert to page (click) number
	RELOC(maxmem, %a0)
	movl	%d1,%a0@		| save as maxmem
	lsrl	%d2,%d0			| convert the lowram to page number
	subl	%d0,%d1			| compute amount of RAM present
	RELOC(physmem, %a0)
	movl	%d1,%a0@		| and physmem

/* configure kernel and lwp0 VA space so we can get going */
#if NKSYMS || defined(DDB) || defined(LKM)
	RELOC(esym,%a0)			| end of static kernel test/data/syms
	movl	%a0@,%d5
	jne	Lstart3
#endif
	movl	#_C_LABEL(end),%d5	| end of static kernel text/data
Lstart3:
	addl	#PAGE_SIZE-1,%d5
	andl	#PG_FRAME,%d5		| round to a page
	movl	%d5,%a4
	addl	%a5,%a4			| convert to PA
	pea	%a5@			| firstpa
	pea	%a4@			| nextpa
	RELOC(pmap_bootstrap,%a0)
	jbsr	%a0@			| pmap_bootstrap(firstpa, nextpa)
	addql	#8,%sp

/*
 * Prepare to enable MMU.
 * Since the kernel is not mapped logical == physical we must insure
 * that when the MMU is turned on, all prefetched addresses (including
 * the PC) are valid.  In order guarantee that, we use the last physical
 * page (which is conveniently mapped == VA) and load it up with enough
 * code to defeat the prefetch, then we execute the jump back to here.
 *
 * Is this all really necessary, or am I paranoid??
 */
	RELOC(Sysseg_pa, %a0)		| system segment table addr
	movl	%a0@,%d1		| read value (a PA)
	RELOC(mmutype, %a0)
	tstl	%a0@			| HP MMU?
	jeq	Lhpmmu2			| yes, skip
	cmpl	#MMU_68040,%a0@		| 68040?
	jne	Lmotommu1		| no, skip
	.long	0x4e7b1807		| movc %d1,%srp
	jra	Lstploaddone
Lmotommu1:
	RELOC(protorp, %a0)
	movl	#0x80000202,%a0@	| nolimit + share global + 4 byte PTEs
	movl	%d1,%a0@(4)		| + segtable address
	pmove	%a0@,%srp		| load the supervisor root pointer
	movl	#0x80000002,%a0@	| reinit upper half for CRP loads
	jra	Lstploaddone		| done
Lhpmmu2:
	moveq	#PGSHIFT,%d2
	lsrl	%d2,%d1			| convert to page frame
	movl	%d1,INTIOBASE+MMUBASE+MMUSSTP | load in sysseg table register
Lstploaddone:
	lea	MAXADDR,%a2		| PA of last RAM page
#if 0
	ASRELOC(Lhighcode, %a1)		| addr of high code
	ASRELOC(Lehighcode, %a3)	| end addr
#else
	/* don't want pc-relative addressing */
	.word	0x43f9			| lea Lhighcode, %a1
	.long	Lhighcode
	addl	%a5, %a1
	.word	0x47f9			| lea Lehighcode, %a3
	.long	Lehighcode
	addl	%a5, %a3
#endif
Lcodecopy:
	movw	%a1@+,%a2@+		| copy a word
	cmpl	%a3,%a1			| done yet?
	jcs	Lcodecopy		| no, keep going
	jmp	MAXADDR			| go for it!

	/*
	 * BEGIN MMU TRAMPOLINE.  This section of code is not
	 * executed in-place.  It's copied to the last page
	 * of RAM (mapped va == pa) and executed there.
	 */

Lhighcode:
	/*
	 * Set up the vector table, and race to get the MMU
	 * enabled.
	 */
	movl	#_C_LABEL(vectab),%d0	| set Vector Base Register
	movc	%d0,%vbr

	RELOC(mmutype, %a0)
	tstl	%a0@			| HP MMU?
	jeq	Lhpmmu3			| yes, skip
	cmpl	#MMU_68040,%a0@		| 68040?
	jne	Lmotommu2		| no, skip
	movw	#0,INTIOBASE+MMUBASE+MMUCMD+2
	movw	#MMU_IEN+MMU_CEN+MMU_FPE,INTIOBASE+MMUBASE+MMUCMD+2
					| enable FPU and caches
	moveq	#0,%d0			| ensure TT regs are disabled
	.long	0x4e7b0004		| movc %d0,%itt0
	.long	0x4e7b0005		| movc %d0,%itt1
	.long	0x4e7b0006		| movc %d0,%dtt0
	.long	0x4e7b0007		| movc %d0,%dtt1
	.word	0xf4d8			| cinva bc
	.word	0xf518			| pflusha
#if PGSHIFT == 13
	movl	#0xc000,%d0
#else
	movl	#0x8000,%d0
#endif
	.long	0x4e7b0003		| movc %d0,%tc
	movl	#CACHE40_ON,%d0
	movc	%d0,%cacr		| turn on both caches
	jmp	Lenab1:l		| forced not be pc-relative
Lmotommu2:
	movl	#MMU_IEN+MMU_FPE,INTIOBASE+MMUBASE+MMUCMD
					| enable 68881 and i-cache
	pflusha
	RELOC(prototc, %a2)
#if PGSHIFT == 13
	movl	#0x82d08b00,%a2@	| value to load TC with
#else
	movl	#0x82c0aa00,%a2@	| value to load TC with
#endif
	pmove	%a2@,%tc		| load it
	jmp	Lenab1:l		| forced not be pc-relative
Lhpmmu3:
	movl	#0,INTIOBASE+MMUBASE+MMUCMD		| clear external cache
	movl	#MMU_ENAB,INTIOBASE+MMUBASE+MMUCMD	| turn on MMU
	jmp	Lenab1:l		| forced not be pc-relative
Lehighcode:

	/*
	 * END MMU TRAMPOLINE.  Address register %a5 is now free.
	 */

/*
 * Should be running mapped from this point on
 */
Lenab1:
	lea	_ASM_LABEL(tmpstk),%sp	| temporary stack
/* call final pmap setup */
	jbsr	_C_LABEL(pmap_bootstrap_finalize)
/* set kernel stack, user SP */
	movl	_C_LABEL(lwp0uarea),%a1	| get lwp0 uarea
	lea	%a1@(USPACE-4),%sp	| set kernel stack to end of area
	movl	#USRSTACK-4,%a2
	movl	%a2,%usp		| init user SP

	tstl	_C_LABEL(fputype)	| Have an FPU?
	jeq	Lenab2			| No, skip.
	clrl	%a1@(PCB_FPCTX)		| ensure null FP context
	movl	%a1,%sp@-
	jbsr	_C_LABEL(m68881_restore) | restore it (does not kill %a1)
	addql	#4,%sp
Lenab2:
/* flush TLB and turn on caches */
	jbsr	_C_LABEL(_TBIA)		| invalidate TLB
	cmpl	#MMU_68040,_C_LABEL(mmutype) | 68040?
	jeq	Lnocache0		| yes, cache already on
	movl	#CACHE_ON,%d0
	movc	%d0,%cacr		| clear cache(s)
	tstl	_C_LABEL(ectype)
	jeq	Lnocache0
	MMUADDR(%a0)
	orl	#MMU_CEN,%a0@(MMUCMD)	| turn on external cache
Lnocache0:
/* Final setup for call to main(). */
	jbsr	_C_LABEL(hp300_init)

/*
 * Create a fake exception frame so that cpu_lwp_fork() can copy it.
 * main() nevers returns; we exit to user mode from a forked process
 * later on.
 */
	clrw	%sp@-			| vector offset/frame type
	clrl	%sp@-			| PC - filled in by "execve"
	movw	#PSL_USER,%sp@-		| in user mode
	clrl	%sp@-			| stack adjust count and padding
	lea	%sp@(-64),%sp		| construct space for D0-D7/A0-A7
	lea	_C_LABEL(lwp0),%a0	| save pointer to frame
	movl	%sp,%a0@(L_MD_REGS)	|   in lwp0.l_md.md_regs

	jra	_C_LABEL(main)		| main()
	PANIC("main() returned")
	/* NOTREACHED */

/*
 * Trap/interrupt vector routines
 */
#include <m68k/m68k/trap_subr.s>

	.data
GLOBAL(m68k_fault_addr)
	.long	0

#if defined(M68040) || defined(M68060)
ENTRY_NOPROFILE(addrerr4060)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save user registers
	movl	%usp,%a0		| save the user SP
	movl	%a0,%sp@(FR_SP)		|   in the savearea
	movl	%sp@(FR_HW+8),%sp@-
	clrl	%sp@-			| dummy code
	movl	#T_ADDRERR,%sp@-	| mark address error
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
#endif

#if defined(M68060)
ENTRY_NOPROFILE(buserr60)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save user registers
	movl	%usp,%a0		| save the user SP
	movl	%a0,%sp@(FR_SP)		|   in the savearea
	movel	%sp@(FR_HW+12),%d0	| FSLW
	btst	#2,%d0			| branch prediction error?
	jeq	Lnobpe
	movc	%cacr,%d2
	orl	#IC60_CABC,%d2		| clear all branch cache entries
	movc	%d2,%cacr
	movl	%d0,%d1
	addql	#1,L60bpe
	andl	#0x7ffd,%d1
	jeq	_ASM_LABEL(faultstkadjnotrap2)
Lnobpe:
| we need to adjust for misaligned addresses
	movl	%sp@(FR_HW+8),%d1	| grab VA
	btst	#27,%d0			| check for mis-aligned access
	jeq	Lberr3			| no, skip
	addl	#28,%d1			| yes, get into next page
					| operand case: 3,
					| instruction case: 4+12+12
	andl	#PG_FRAME,%d1           | and truncate
Lberr3:
	movl	%d1,%sp@-
	movl	%d0,%sp@-		| code is FSLW now.
	andw	#0x1f80,%d0
	jeq	Lberr60			| it is a bus error
	movl	#T_MMUFLT,%sp@-		| show that we are an MMU fault
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
Lberr60:
	tstl	_C_LABEL(nofault)	| catch bus error?
	jeq	Lisberr			| no, handle as usual
	movl	%sp@(FR_HW+8+8),_C_LABEL(m68k_fault_addr) | save fault addr
	movl	_C_LABEL(nofault),%sp@-	| yes,
	jbsr	_C_LABEL(longjmp)	|  longjmp(nofault)
	/* NOTREACHED */
#endif
#if defined(M68040)
ENTRY_NOPROFILE(buserr40)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save user registers
	movl	%usp,%a0		| save the user SP
	movl	%a0,%sp@(FR_SP)		|   in the savearea
	movl	%sp@(FR_HW+20),%d1	| get fault address
	moveq	#0,%d0
	movw	%sp@(FR_HW+12),%d0	| get SSW
	btst	#11,%d0			| check for mis-aligned
	jeq	Lbe1stpg		| no skip
	addl	#3,%d1			| get into next page
	andl	#PG_FRAME,%d1		| and truncate
Lbe1stpg:
	movl	%d1,%sp@-		| pass fault address.
	movl	%d0,%sp@-		| pass SSW as code
	btst	#10,%d0			| test ATC
	jeq	Lberr40			| it is a bus error
	movl	#T_MMUFLT,%sp@-		| show that we are an MMU fault
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
Lberr40:
	tstl	_C_LABEL(nofault)	| catch bus error?
	jeq	Lisberr			| no, handle as usual
	movl	%sp@(FR_HW+8+20),_C_LABEL(m68k_fault_addr) | save fault addr
	movl	_C_LABEL(nofault),%sp@-	| yes,
	jbsr	_C_LABEL(longjmp)	|  longjmp(nofault)
	/* NOTREACHED */
#endif

#if defined(M68020) || defined(M68030)
ENTRY_NOPROFILE(busaddrerr2030)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save user registers
	movl	%usp,%a0		| save the user SP
	movl	%a0,%sp@(FR_SP)		|   in the savearea
	moveq	#0,%d0
	movw	%sp@(FR_HW+10),%d0	| grab SSW for fault processing
	btst	#12,%d0			| RB set?
	jeq	LbeX0			| no, test RC
	bset	#14,%d0			| yes, must set FB
	movw	%d0,%sp@(FR_HW+10)	| for hardware too
LbeX0:
	btst	#13,%d0			| RC set?
	jeq	LbeX1			| no, skip
	bset	#15,%d0			| yes, must set FC
	movw	%d0,%sp@(FR_HW+10)	| for hardware too
LbeX1:
	btst	#8,%d0			| data fault?
	jeq	Lbe0			| no, check for hard cases
	movl	%sp@(FR_HW+16),%d1	| fault address is as given in frame
	jra	Lbe10			| thats it
Lbe0:
	btst	#4,%sp@(FR_HW+6)	| long (type B) stack frame?
	jne	Lbe4			| yes, go handle
	movl	%sp@(FR_HW+2),%d1	| no, can use save PC
	btst	#14,%d0			| FB set?
	jeq	Lbe3			| no, try FC
	addql	#4,%d1			| yes, adjust address
	jra	Lbe10			| done
Lbe3:
	btst	#15,%d0			| FC set?
	jeq	Lbe10			| no, done
	addql	#2,%d1			| yes, adjust address
	jra	Lbe10			| done
Lbe4:
	movl	%sp@(FR_HW+36),%d1	| long format, use stage B address
	btst	#15,%d0			| FC set?
	jeq	Lbe10			| no, all done
	subql	#2,%d1			| yes, adjust address
Lbe10:
	movl	%d1,%sp@-		| push fault VA
	movl	%d0,%sp@-		| and padded SSW
	movw	%sp@(FR_HW+8+6),%d0	| get frame format/vector offset
	andw	#0x0FFF,%d0		| clear out frame format
	cmpw	#12,%d0			| address error vector?
	jeq	Lisaerr			| yes, go to it
#if defined(M68K_MMU_MOTOROLA)
#if defined(M68K_MMU_HP)
	tstl	_C_LABEL(mmutype)	| HP MMU?
	jeq	Lbehpmmu		| yes, different MMU fault handler
#endif
	movl	%d1,%a0			| fault address
	movl	%sp@,%d0		| function code from ssw
	btst	#8,%d0			| data fault?
	jne	Lbe10a
	movql	#1,%d0			| user program access FC
					| (we dont separate data/program)
	btst	#5,%sp@(FR_HW+8)	| supervisor mode?
	jeq	Lbe10a			| if no, done
	movql	#5,%d0			| else supervisor program access
Lbe10a:
	ptestr	%d0,%a0@,#7		| do a table search
	pmove	%psr,%sp@		| save result
	movb	%sp@,%d1
	btst	#2,%d1			| invalid (incl. limit viol. and berr)?
	jeq	Lmightnotbemerr		| no -> wp check
	btst	#7,%d1			| is it MMU table berr?
	jne	Lisberr1		| yes, needs not be fast.
#endif /* M68K_MMU_MOTOROLA */
Lismerr:
	movl	#T_MMUFLT,%sp@-		| show that we are an MMU fault
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
#if defined(M68K_MMU_MOTOROLA)
Lmightnotbemerr:
	btst	#3,%d1			| write protect bit set?
	jeq	Lisberr1		| no: must be bus error
	movl	%sp@,%d0		| ssw into low word of %d0
	andw	#0xc0,%d0		| Write protect is set on page:
	cmpw	#0x40,%d0		| was it read cycle?
	jne	Lismerr			| no, was not WPE, must be MMU fault
	jra	Lisberr1		| real bus err needs not be fast.
#endif /* M68K_MMU_MOTOROLA */
#if defined(M68K_MMU_HP)
Lbehpmmu:
	MMUADDR(%a0)
	movl	%a0@(MMUSTAT),%d0	| read MMU status
	btst	#3,%d0			| MMU fault?
	jeq	Lisberr1		| no, just a non-MMU bus error
	andl	#~MMU_FAULT,%a0@(MMUSTAT)| yes, clear fault bits
	movw	%d0,%sp@		| pass MMU stat in upper half of code
	jra	Lismerr			| and handle it
#endif
Lisaerr:
	movl	#T_ADDRERR,%sp@-	| mark address error
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
Lisberr1:
	clrw	%sp@			| re-clear pad word
	tstl	_C_LABEL(nofault)	| catch bus error?
	jeq	Lisberr			| no, handle as usual
	movl	%sp@(FR_HW+8+16),_C_LABEL(m68k_fault_addr) | save fault addr
	movl	_C_LABEL(nofault),%sp@-	| yes,
	jbsr	_C_LABEL(longjmp)	|  longjmp(nofault)
	/* NOTREACHED */
#endif /* M68020 || M68030 */

Lisberr:				| also used by M68040/60
	movl	#T_BUSERR,%sp@-		| mark bus error
	jra	_ASM_LABEL(faultstkadj)	| and deal with it

/*
 * FP exceptions.
 */
ENTRY_NOPROFILE(fpfline)
#if defined(M68040)
	cmpl	#FPU_68040,_C_LABEL(fputype) | 68040 FPU?
	jne	Lfp_unimp		| no, skip FPSP
	cmpw	#0x202c,%sp@(6)		| format type 2?
	jne	_C_LABEL(illinst)	| no, not an FP emulation
Ldofp_unimp:
#ifdef FPSP
	jmp	_ASM_LABEL(fpsp_unimp)	| yes, go handle it
#endif
Lfp_unimp:
#endif /* M68040 */
#ifdef FPU_EMULATE
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save registers
	moveq	#T_FPEMULI,%d0		| denote as FP emulation trap
	jra	_ASM_LABEL(fault)	| do it
#else
	jra	_C_LABEL(illinst)
#endif

ENTRY_NOPROFILE(fpunsupp)
#if defined(M68040)
	cmpl	#FPU_68040,_C_LABEL(fputype) | 68040 FPU?
	jne	_C_LABEL(illinst)	| no, treat as illinst
#ifdef FPSP
	jmp	_ASM_LABEL(fpsp_unsupp)	| yes, go handle it
#endif
Lfp_unsupp:
#endif /* M68040 */
#ifdef FPU_EMULATE
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save registers
	moveq	#T_FPEMULD,%d0		| denote as FP emulation trap
	jra	_ASM_LABEL(fault)	| do it
#else
	jra	_C_LABEL(illinst)
#endif

/*
 * Handles all other FP coprocessor exceptions.
 * Note that since some FP exceptions generate mid-instruction frames
 * and may cause signal delivery, we need to test for stack adjustment
 * after the trap call.
 */
ENTRY_NOPROFILE(fpfault)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save user registers
	movl	%usp,%a0		| and save
	movl	%a0,%sp@(FR_SP)		|   the user stack pointer
	clrl	%sp@-			| no VA arg
	movl	_C_LABEL(curpcb),%a0	| current pcb
	lea	%a0@(PCB_FPCTX),%a0	| address of FP savearea
	fsave	%a0@			| save state
#if defined(M68040) || defined(M68060)
	/* always null state frame on 68040, 68060 */
	cmpl	#FPU_68040,_C_LABEL(fputype)
	jge	Lfptnull
#endif
	tstb	%a0@			| null state frame?
	jeq	Lfptnull		| yes, safe
	clrw	%d0			| no, need to tweak BIU
	movb	%a0@(1),%d0		| get frame size
	bset	#3,%a0@(0,%d0:w)	| set exc_pend bit of BIU
Lfptnull:
	fmovem	%fpsr,%sp@-		| push %fpsr as code argument
	frestore %a0@			| restore state
	movl	#T_FPERR,%sp@-		| push type arg
	jra	_ASM_LABEL(faultstkadj) | call trap and deal with stack cleanup

/*
 * Other exceptions only cause four and six word stack frame and require
 * no post-trap stack adjustment.
 */

ENTRY_NOPROFILE(badtrap)
	moveml	#0xC0C0,%sp@-		| save scratch regs
	movw	%sp@(22),%sp@-		| push exception vector info
	clrw	%sp@-
	movl	%sp@(22),%sp@-		| and PC
	jbsr	_C_LABEL(straytrap)	| report
	addql	#8,%sp			| pop args
	moveml	%sp@+,#0x0303		| restore regs
	jra	_ASM_LABEL(rei)		| all done

ENTRY_NOPROFILE(trap0)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save user registers
	movl	%usp,%a0		| save the user SP
	movl	%a0,%sp@(FR_SP)		|   in the savearea
	movl	%d0,%sp@-		| push syscall number
	jbsr	_C_LABEL(syscall)	| handle it
	addql	#4,%sp			| pop syscall arg
	tstl	_C_LABEL(astpending)	| AST pending?
	jne	Lrei			| yes, handle it via trap
	movl	%sp@(FR_SP),%a0		| grab and restore
	movl	%a0,%usp		|   user SP
	moveml	%sp@+,#0x7FFF		| restore most registers
	addql	#8,%sp			| pop SP and stack adjust
	rte

/*
 * Trap 12 is the entry point for the cachectl "syscall" (both HPUX & BSD)
 *	cachectl(command, addr, length)
 * command in %d0, addr in %a1, length in %d1
 */
ENTRY_NOPROFILE(trap12)
	movl	_C_LABEL(curlwp),%a0
	movl	%a0@(L_PROC),%sp@-	| push current proc pointer
	movl	%d1,%sp@-		| push length
	movl	%a1,%sp@-		| push addr
	movl	%d0,%sp@-		| push command
	jbsr	_C_LABEL(cachectl1)	| do it
	lea	%sp@(16),%sp		| pop args
	jra	_ASM_LABEL(rei)		| all done

/*
 * Trace (single-step) trap.  Kernel-mode is special.
 * User mode traps are simply passed on to trap().
 */
ENTRY_NOPROFILE(trace)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-
	moveq	#T_TRACE,%d0

	| Check PSW and see what happen.
	|   T=0 S=0	(should not happen)
	|   T=1 S=0	trace trap from user mode
	|   T=0 S=1	trace trap on a trap instruction
	|   T=1 S=1	trace trap from system mode (kernel breakpoint)

	movw	%sp@(FR_HW),%d1		| get PSW
	notw	%d1			| XXX no support for T0 on 680[234]0
	andw	#PSL_TS,%d1		| from system mode (T=1, S=1)?
	jeq	Lkbrkpt			| yes, kernel breakpoint
	jra	_ASM_LABEL(fault)	| no, user-mode fault

/*
 * Trap 15 is used for:
 *	- GDB breakpoints (in user programs)
 *	- KGDB breakpoints (in the kernel)
 *	- trace traps for SUN binaries (not fully supported yet)
 * User mode traps are simply passed to trap().
 */
ENTRY_NOPROFILE(trap15)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-
	moveq	#T_TRAP15,%d0
	movw	%sp@(FR_HW),%d1		| get PSW
	andw	#PSL_S,%d1		| from system mode?
	jne	Lkbrkpt			| yes, kernel breakpoint
	jra	_ASM_LABEL(fault)	| no, user-mode fault

Lkbrkpt: | Kernel-mode breakpoint or trace trap. (%d0=trap_type)
	| Save the system sp rather than the user sp.
	movw	#PSL_HIGHIPL,%sr	| lock out interrupts
	lea	%sp@(FR_SIZE),%a6	| Save stack pointer
	movl	%a6,%sp@(FR_SP)		|  from before trap

	| If were are not on tmpstk switch to it.
	| (so debugger can change the stack pointer)
	movl	%a6,%d1
	cmpl	#_ASM_LABEL(tmpstk),%d1
	jls	Lbrkpt2			| already on tmpstk
	| Copy frame to the temporary stack
	movl	%sp,%a0			| %a0=src
	lea	_ASM_LABEL(tmpstk)-96,%a1 | %a1=dst
	movl	%a1,%sp			| %sp=new frame
	moveq	#FR_SIZE,%d1
Lbrkpt1:
	movl	%a0@+,%a1@+
	subql	#4,%d1
	jgt	Lbrkpt1

Lbrkpt2:
	| Call the trap handler for the kernel debugger.
	| Do not call trap() to do it, so that we can
	| set breakpoints in trap() if we want.  We know
	| the trap type is either T_TRACE or T_BREAKPOINT.
	| If we have both DDB and KGDB, let KGDB see it first,
	| because KGDB will just return 0 if not connected.
	| Save args in %d2, %a2
	movl	%d0,%d2			| trap type
	movl	%sp,%a2			| frame ptr
#ifdef KGDB
	| Let KGDB handle it (if connected)
	movl	%a2,%sp@-		| push frame ptr
	movl	%d2,%sp@-		| push trap type
	jbsr	_C_LABEL(kgdb_trap)	| handle the trap
	addql	#8,%sp			| pop args
	cmpl	#0,%d0			| did kgdb handle it?
	jne	Lbrkpt3			| yes, done
#endif
#ifdef DDB
	| Let DDB handle it
	movl	%a2,%sp@-		| push frame ptr
	movl	%d2,%sp@-		| push trap type
	jbsr	_C_LABEL(kdb_trap)	| handle the trap
	addql	#8,%sp			| pop args
#if 0	/* not needed on hp300 */
	cmpl	#0,%d0			| did ddb handle it?
	jne	Lbrkpt3			| yes, done
#endif
#endif
	/* Sun 3 drops into PROM here. */
Lbrkpt3:
	| The stack pointer may have been modified, or
	| data below it modified (by kgdb push call),
	| so push the hardware frame at the current sp
	| before restoring registers and returning.

	movl	%sp@(FR_SP),%a0		| modified %sp
	lea	%sp@(FR_SIZE),%a1	| end of our frame
	movl	%a1@-,%a0@-		| copy 2 longs with
	movl	%a1@-,%a0@-		| ... predecrement
	movl	%a0,%sp@(FR_SP)		| %sp = h/w frame
	moveml	%sp@+,#0x7FFF		| restore all but %sp
	movl	%sp@,%sp		| ... and %sp
	rte				| all done

/* Use common m68k sigreturn */
#include <m68k/m68k/sigreturn.s>

/*
 * Interrupt handlers.
 * All device interrupts are auto-vectored.  The CPU provides
 * the vector 0x18+level.  Note we count spurious interrupts, but
 * we don't do anything else with them.
 */

/* 64-bit evcnt counter increments */
#define EVCNT_COUNTER(ipl)					\
	_C_LABEL(hp300_intr_list) + (ipl)*SIZEOF_HI + HI_EVCNT
#define EVCNT_INCREMENT(ipl)					\
	clrl	%d0;						\
	addql	#1,EVCNT_COUNTER(ipl)+4;			\
	movel	EVCNT_COUNTER(ipl),%d1;				\
	addxl	%d0,%d1;					\
	movel	%d1,EVCNT_COUNTER(ipl)

ENTRY_NOPROFILE(spurintr)	/* level 0 */
	INTERRUPT_SAVEREG
	EVCNT_INCREMENT(0)
	CPUINFO_INCREMENT(CI_NINTR)
	INTERRUPT_RESTOREREG
	jra	_ASM_LABEL(rei)

ENTRY_NOPROFILE(intrhand)	/* levels 1 through 5 */
	addql	#1,_C_LABEL(idepth)	| entering interrupt
	INTERRUPT_SAVEREG
	movw	%sp@(22),%sp@-		| push exception vector info
	clrw	%sp@-
	jbsr	_C_LABEL(intr_dispatch)	| call dispatch routine
	addql	#4,%sp
	INTERRUPT_RESTOREREG
	subql	#1,_C_LABEL(idepth)	| exiting from interrupt
	jra	_ASM_LABEL(rei)		| all done

ENTRY_NOPROFILE(lev6intr)	/* level 6: clock */
	addql	#1,_C_LABEL(idepth)	| entering interrupt
	INTERRUPT_SAVEREG
	CLKADDR(%a0)
	movb	%a0@(CLKSR),%d0		| read clock status
Lclkagain:
	btst	#0,%d0			| clear timer1 int immediately to
	jeq	Lnotim1			|  minimize chance of losing another
	movpw	%a0@(CLKMSB1),%d1	|  due to statintr processing delay
	movl	_C_LABEL(clkint),%d1	| clkcounter += clkint
	addl	%d1,_C_LABEL(clkcounter)
Lnotim1:
	btst	#2,%d0			| timer3 interrupt?
	jeq	Lnotim3			| no, skip statclock
	movpw	%a0@(CLKMSB3),%d1	| clear timer3 interrupt
	lea	%sp@(16),%a1		| a1 = &clockframe
	movl	%d0,%sp@-		| save status
	movl	%a1,%sp@-
	jbsr	_C_LABEL(statintr)	| statintr(&frame)
	addql	#4,%sp
	movl	%sp@+,%d0		| restore pre-statintr status
	CLKADDR(%a0)
Lnotim3:
	btst	#0,%d0			| timer1 interrupt?
	jeq	Lrecheck		| no, skip hardclock
	EVCNT_INCREMENT(6)
	lea	%sp@(16),%a1		| a1 = &clockframe
	movl	%a1,%sp@-
#ifdef USELEDS
	tstl	_C_LABEL(ledaddr)	| using LEDs?
	jeq	Lnoleds0		| no, skip this code
	movl	_ASM_LABEL(heartbeat),%d0 | get tick count
	addql	#1,%d0			|  increment
	movl	_C_LABEL(hz),%d1
	addl	#50,%d1			| get the timing a little closer
	tstb	_ASM_LABEL(beatstatus)	| time to slow down?
	jeq	Lslowthrob		| yes, slow down
	lsrl	#3,%d1			| no, fast throb
Lslowthrob:
	lsrl	#1,%d1			| slow throb
	cmpl	%d0,%d1			| are we there yet?
	jne	Lnoleds1		| no, nothing to do
	addqb	#1,_ASM_LABEL(beatstatus) | incr beat status
	cmpb	#3,_ASM_LABEL(beatstatus) | time to reset?
	jle	Ltwinkle		  | no, twinkle the lights
	movb	#0,_ASM_LABEL(beatstatus) | reset the status indicator
Ltwinkle:
	movl	#LED_PULSE,%sp@-
	movl	#LED_DISK+LED_LANRCV+LED_LANXMT,%sp@-
	clrl	%sp@-
	jbsr	_C_LABEL(ledcontrol)	| toggle pulse, turn all others off
	lea	%sp@(12),%sp
	movql	#0,%d0
Lnoleds1:
	movl	%d0,_ASM_LABEL(heartbeat)
Lnoleds0:
#endif /* USELEDS */
	jbsr	_C_LABEL(hardclock)	| hardclock(&frame)
	addql	#4,%sp
Lrecheck:
	CPUINFO_INCREMENT(CI_NINTR)	| chalk up another interrupt
	CLKADDR(%a0)
	movb	%a0@(CLKSR),%d0		| see if anything happened
	jmi	Lclkagain		|  while we were in hardclock/statintr
	INTERRUPT_RESTOREREG
	subql	#1,_C_LABEL(idepth)	| exiting from interrupt
	jra	_ASM_LABEL(rei)		| all done

ENTRY_NOPROFILE(lev7intr)	/* level 7: parity errors, reset key */
	clrl	%sp@-
	moveml	#0xFFFF,%sp@-		| save registers
	EVCNT_INCREMENT(7)
	movl	%usp,%a0		| and save
	movl	%a0,%sp@(FR_SP)		|   the user stack pointer
	jbsr	_C_LABEL(nmihand)	| call handler
	movl	%sp@(FR_SP),%a0		| restore
	movl	%a0,%usp		|   user SP
	moveml	%sp@+,#0x7FFF		| and remaining registers
	addql	#8,%sp			| pop SP and stack adjust
	jra	_ASM_LABEL(rei)		| all done

/*
 * Emulation of VAX REI instruction.
 *
 * This code deals with checking for and servicing
 * ASTs (profiling, scheduling).
 * After identifing that we need an AST we drop the IPL
 * to allow device interrupts.
 *
 * This code is complicated by the fact that sendsig may have been called
 * necessitating a stack cleanup.
 */

ASENTRY_NOPROFILE(rei)
	tstl	_C_LABEL(astpending)	| AST pending?
	jne	1f			| no, done
	rte
1:
	btst	#5,%sp@			| yes, are we returning to user mode?
	jeq	2f			| no, done
	rte
2:
	movw	#PSL_LOWIPL,%sr		| lower SPL
	clrl	%sp@-			| stack adjust
	moveml	#0xFFFF,%sp@-		| save all registers
	movl	%usp,%a1		| including
	movl	%a1,%sp@(FR_SP)		|    the users SP
Lrei:
	clrl	%sp@-			| VA == none
	clrl	%sp@-			| code == none
	movl	#T_ASTFLT,%sp@-		| type == async system trap
	pea	%sp@(12)		| fp == address of trap frame
	jbsr	_C_LABEL(trap)		| go handle it
	lea	%sp@(16),%sp		| pop value args
	movl	%sp@(FR_SP),%a0		| restore user SP
	movl	%a0,%usp		|   from save area
	movw	%sp@(FR_ADJ),%d0	| need to adjust stack?
	jne	Laststkadj		| yes, go to it
	moveml	%sp@+,#0x7FFF		| no, restore most user regs
	addql	#8,%sp			| toss SP and stack adjust
	rte				| and do real RTE
Laststkadj:
	lea	%sp@(FR_HW),%a1		| pointer to HW frame
	addql	#8,%a1			| source pointer
	movl	%a1,%a0			| source
	addw	%d0,%a0			|  + hole size = dest pointer
	movl	%a1@-,%a0@-		| copy
	movl	%a1@-,%a0@-		|  8 bytes
	movl	%a0,%sp@(FR_SP)		| new SSP
	moveml	%sp@+,#0x7FFF		| restore user registers
	movl	%sp@,%sp		| and our SP
	rte				| and do real RTE

/*
 * Use common m68k sigcode.
 */
#include <m68k/m68k/sigcode.s>
#ifdef COMPAT_SUNOS
#include <m68k/m68k/sunos_sigcode.s>
#endif
#ifdef COMPAT_SVR4
#include <m68k/m68k/svr4_sigcode.s>
#endif

/*
 * Primitives
 */

/*
 * Use common m68k support routines.
 */
#include <m68k/m68k/support.s>

/*
 * Use common m68k process/lwp switch and context save subroutines.
 */
#define FPCOPROC	/* XXX: Temp. reqd. */
#include <m68k/m68k/switch_subr.s>


#if defined(M68040)
ENTRY(suline)
	movl	%sp@(4),%a0		| address to write
	movl	_C_LABEL(curpcb),%a1	| current pcb
	movl	#Lslerr,%a1@(PCB_ONFAULT) | where to return to on a fault
	movl	%sp@(8),%a1		| address of line
	movl	%a1@+,%d0		| get lword
	movsl	%d0,%a0@+		| put lword
	nop				| sync
	movl	%a1@+,%d0		| get lword
	movsl	%d0,%a0@+		| put lword
	nop				| sync
	movl	%a1@+,%d0		| get lword
	movsl	%d0,%a0@+		| put lword
	nop				| sync
	movl	%a1@+,%d0		| get lword
	movsl	%d0,%a0@+		| put lword
	nop				| sync
	moveq	#0,%d0			| indicate no fault
	jra	Lsldone
Lslerr:
	moveq	#-1,%d0
Lsldone:
	movl	_C_LABEL(curpcb),%a1	| current pcb
	clrl	%a1@(PCB_ONFAULT)	| clear fault address
	rts
#endif

ENTRY(ecacheon)
	tstl	_C_LABEL(ectype)
	jeq	Lnocache7
	MMUADDR(%a0)
	orl	#MMU_CEN,%a0@(MMUCMD)
Lnocache7:
	rts

ENTRY(ecacheoff)
	tstl	_C_LABEL(ectype)
	jeq	Lnocache8
	MMUADDR(%a0)
	andl	#~MMU_CEN,%a0@(MMUCMD)
Lnocache8:
	rts

/*
 * Load a new user segment table pointer.
 */
ENTRY(loadustp)
#if defined(M68K_MMU_MOTOROLA)
	tstl	_C_LABEL(mmutype)	| HP MMU?
	jeq	Lhpmmu9			| yes, skip
	movl	%sp@(4),%d0		| new USTP
	moveq	#PGSHIFT,%d1
	lsll	%d1,%d0			| convert to addr
#if defined(M68040)
	cmpl	#MMU_68040,_C_LABEL(mmutype) | 68040?
	jne	LmotommuC		| no, skip
	.word	0xf518			| yes, pflusha
	.long	0x4e7b0806		| movc %d0,%urp
	rts
LmotommuC:
#endif
	pflusha				| flush entire TLB
	lea	_C_LABEL(protorp),%a0	| CRP prototype
	movl	%d0,%a0@(4)		| stash USTP
	pmove	%a0@,%crp		| load root pointer
	movl	#CACHE_CLR,%d0
	movc	%d0,%cacr		| invalidate cache(s)
	rts
Lhpmmu9:
#endif
#if defined(M68K_MMU_HP)
	movl	#CACHE_CLR,%d0
	movc	%d0,%cacr		| invalidate cache(s)
	MMUADDR(%a0)
	movl	%a0@(MMUTBINVAL),%d1	| invalidate TLB
	tstl	_C_LABEL(ectype)	| have external VAC?
	jle	1f			| no, skip
	andl	#~MMU_CEN,%a0@(MMUCMD)	| toggle cache enable
	orl	#MMU_CEN,%a0@(MMUCMD)	| to clear data cache
1:
	movl	%sp@(4),%a0@(MMUUSTP)	| load a new USTP
#endif
	rts

ENTRY(ploadw)
#if defined(M68K_MMU_MOTOROLA)
	movl	%sp@(4),%a0		| address to load
#if defined(M68K_MMU_HP)
	tstl	_C_LABEL(mmutype)	| HP MMU?
	jeq	Lploadwskp		| yes, skip
#endif
#if defined(M68040)
	cmpl	#MMU_68040,_C_LABEL(mmutype) | 68040?
	jeq	Lploadwskp		| yes, skip
#endif
	ploadw	#1,%a0@			| pre-load translation
Lploadwskp:
#endif
	rts

/*
 * _delay(u_int N)
 *
 * Delay for at least (N/256) microseconds.
 * This routine depends on the variable:  delay_divisor
 * which should be set based on the CPU clock rate.
 */
ENTRY_NOPROFILE(_delay)
	| %d0 = arg = (usecs << 8)
	movl	%sp@(4),%d0
	| %d1 = delay_divisor
	movl	_C_LABEL(delay_divisor),%d1
	jra	L_delay			/* Jump into the loop! */

	/*
	 * Align the branch target of the loop to a half-line (8-byte)
	 * boundary to minimize cache effects.  This guarantees both
	 * that there will be no prefetch stalls due to cache line burst
	 * operations and that the loop will run from a single cache
	 * half-line.
	 */
	.align	8
L_delay:
	subl	%d1,%d0
	jgt	L_delay
	rts

/*
 * Probe a memory address, and see if it causes a bus error.
 * This function is only to be used in physical mode, and before our
 * trap vectors are initialized.
 * Invoke with address to probe in %a0.
 * Alters: %a3 %d0
 */
#define BUSERR  0xfffffffc
ASLOCAL(phys_badaddr)
	ASRELOC(_bsave,%a3)
	movl	BUSERR,%a3@		| save ROM bus errror handler
	ASRELOC(_ssave,%a3)
	movl	%sp,%a3@		| and current stack pointer
	ASRELOC(catchbad,%a3)
	movl	%a3,BUSERR		| plug in our handler
	movb	%a0@,%d0		| access address
	ASRELOC(_bsave,%a3)		| no fault!
	movl	%a3@,BUSERR
	clrl	%d0			| return success
	rts
ASLOCAL(catchbad)
	ASRELOC(_bsave,%a3)		| got a bus error, so restore handler
	movl	%a3@,BUSERR
	ASRELOC(_ssave,%a3)
	movl	%a3@,%sp		| and stack
	moveq	#1,%d0			| return fault
	rts
#undef	BUSERR

	.data
ASLOCAL(_bsave)
	.long   0
ASLOCAL(_ssave)
	.long   0
	.text

/*
 * Handle the nitty-gritty of rebooting the machine.
 * Basically we just turn off the MMU and jump to the appropriate ROM routine.
 * Note that we must be running in an address range that is mapped one-to-one
 * logical to physical so that the PC is still valid immediately after the MMU
 * is turned off.  We have conveniently mapped the last page of physical
 * memory this way.
 */
ENTRY_NOPROFILE(doboot)
#if defined(M68040)
	cmpl	#MMU_68040,_C_LABEL(mmutype) | 68040?
	jeq	Lnocache5		| yes, skip
#endif
	movl	#CACHE_OFF,%d0
	movc	%d0,%cacr		| disable on-chip cache(s)
	tstl	_C_LABEL(ectype)	| external cache?
	jeq	Lnocache5		| no, skip
	MMUADDR(%a0)
	andl	#~MMU_CEN,%a0@(MMUCMD)	| disable external cache
Lnocache5:
	lea	MAXADDR,%a0		| last page of physical memory
	movl	_C_LABEL(boothowto),%a0@+ | store howto
	movl	_C_LABEL(bootdev),%a0@+	| and devtype
	lea	Lbootcode,%a1		| start of boot code
	lea	Lebootcode,%a3		| end of boot code
Lbootcopy:
	movw	%a1@+,%a0@+		| copy a word
	cmpl	%a3,%a1			| done yet?
	jcs	Lbootcopy		| no, keep going
#if defined(M68040)
	cmpl	#MMU_68040,_C_LABEL(mmutype) | 68040?
	jne	LmotommuE		| no, skip
	.word	0xf4f8			| cpusha bc
LmotommuE:
#endif
	jmp	MAXADDR+8		| jump to last page

Lbootcode:
	lea	MAXADDR+0x800,%sp	| physical SP in case of NMI
#if defined(M68040)
	cmpl	#MMU_68040,_C_LABEL(mmutype) | 68040?
	jne	LmotommuF		| no, skip
	movl	#0,%d0
	movc	%d0,%cacr		| caches off
	.long	0x4e7b0003		| movc %d0,%tc
	movl	%d2,MAXADDR+PAGE_SIZE-4	| restore old high page contents
	DOREBOOT
LmotommuF:
#endif
#if defined(M68K_MMU_MOTOROLA)
	tstl	_C_LABEL(mmutype)	| HP MMU?
	jeq	LhpmmuB			| yes, skip
	movl	#0,%a0@			| value for pmove to TC (turn off MMU)
	pmove	%a0@,%tc		| disable MMU
	DOREBOOT
LhpmmuB:
#endif
#if defined(M68K_MMU_HP)
	MMUADDR(%a0)
	movl	#0xFFFF0000,%a0@(MMUCMD)	| totally disable MMU
	movl	%d2,MAXADDR+PAGE_SIZE-4	| restore old high page contents
	DOREBOOT
#endif
Lebootcode:

/*
 * Misc. global variables.
 */
	.data
GLOBAL(machineid)
	.long	HP_320			| default to 320

GLOBAL(mmuid)
	.long	0			| default to nothing

GLOBAL(mmutype)
	.long	MMU_HP			| default to HP MMU

GLOBAL(cputype)
	.long	CPU_68020		| default to 68020 CPU

GLOBAL(ectype)
	.long	EC_NONE			| external cache type, default to none

GLOBAL(fputype)
	.long	FPU_68882		| default to 68882 FPU

GLOBAL(protorp)
	.long	0,0			| prototype root pointer

GLOBAL(prototc)
	.long	0			| prototype translation control

GLOBAL(internalhpib)
	.long	1			| has internal HP-IB, default to yes

GLOBAL(intiobase)
	.long	0			| KVA of base of internal IO space

GLOBAL(intiolimit)
	.long	0			| KVA of end of internal IO space

GLOBAL(extiobase)
	.long	0			| KVA of base of external IO space

GLOBAL(CLKbase)
	.long	0			| KVA of base of clock registers

GLOBAL(MMUbase)
	.long	0			| KVA of base of HP MMU registers

#ifdef USELEDS
ASLOCAL(heartbeat)
	.long	0			| clock ticks since last heartbeat

ASLOCAL(beatstatus)
	.long	0			| for determining a fast or slow throb
#endif

#ifdef DEBUG
ASGLOBAL(fulltflush)
	.long	0

ASGLOBAL(fullcflush)
	.long	0
#endif
