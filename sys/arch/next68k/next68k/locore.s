/*	$NetBSD: locore.s,v 1.33.6.3 2002/06/20 03:40:23 nathanw Exp $	*/

/*
 * Copyright (c) 1998 Darrin B. Jewell
 * Copyright (c) 1994, 1995 Gordon W. Ross
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

/* This is currently amid development by
 * Darrin Jewell <jewell@mit.edu>  Fri Jan  2 14:36:47 1998
 * for the next68k port
 */

#include "opt_compat_netbsd.h"
#include "opt_compat_svr4.h"
#include "opt_compat_sunos.h"
#include "opt_ddb.h"
#include "opt_fpsp.h"
#include "opt_kgdb.h"
#include "opt_lockdebug.h"

#include "assym.h"
#include <machine/asm.h>
#include <machine/trap.h>

#if (!defined(M68040))
#error "M68040 is not defined! (check that the generated assym.h is not empty)"
#endif

/*
 * This is for kvm_mkdb, and should be the address of the beginning
 * of the kernel text segment (not necessarily the same as kernbase).
 */
	.text
GLOBAL(kernel_text)

 /*
  * Leave page zero empty so it can be unmapped
  */     
	.space	NBPG

/*
 * Temporary stack for a variety of purposes.
 */
	.data
GLOBAL(endstack)
	.space	NBPG
GLOBAL(bgnstack)
ASLOCAL(tmpstk)

#include <next68k/next68k/vectors.s>

/*
 * Macro to relocate a symbol, used before MMU is enabled.
 * On the NeXT, memory is laid out as in the mach header
 * so therefore we need to relocate symbols until the MMU
 * is turned on.
 */
#define	_RELOC(var, ar)		\
	lea	var,ar;		\
	addl	%a5,ar

#define	RELOC(var, ar)		_RELOC(_C_LABEL(var), ar)
#define	ASRELOC(var, ar)	_RELOC(_ASM_LABEL(var), ar)

/*
 * Initialization info as per grossman's boot loader:
 *
 * We are called from the boot prom, not the boot loader. We have the
 * prom's stack initialized for us and we were called like this:
 * start(mg, mg->mg_console_i, mg->mg_console_o,
 *       mg->mg_boot_dev, mg->mg_boot_arg, mg->mg_boot_info,
 *       mg->mg_sid, mg->mg_pagesize, 4, mg->mg_region,
 *       etheraddr, mg->mg_boot_file);
 * so we actually only really need the first parameter from the stack.
 * Exceptions will be handled by the prom until we feel ready to handle
 * them ourselves.
 * By the way, we get loaded at our final address i.e. PA==VA for the kernel.
 */
 /* I think the PA==VA comment to be a lie, but I have yet to verify it.
  * Darrin B Jewell <jewell@mit.edu>  Sun Jan 11 01:05:54 1998
 */
BSS(lowram,4)
BSS(esym,4)

ASENTRY_NOPROFILE(start)
	movw	#PSL_HIGHIPL,%sr	| no interrupts
	movl	#CACHE_OFF,%d0
	movc	%d0,%cacr		| clear and disable on-chip cache(s)

	moveal	#NEXT_RAMBASE,%a5	| amount to RELOC by.
	RELOC(lowram,%a0)		| store base of memory.
	movl    %a5,%a0@

	| Create a new stack at address tmpstk, and push
	| The existing sp onto it as an arg for next68k_bootargs.
	ASRELOC(tmpstk, %a0)
	movel	%sp,%a0@-
	moveal  %a0,%sp
	moveal  #0,%a6

	/* Read the header to get our segment list */
	RELOC(next68k_bootargs,%a0)
	jbsr	%a0@			| next68k_bootargs(args)
	addqw	#4,%sp			| clear arg from stack.

	/*
	 * All data registers are now free.  All address registers
	 * except %a5 are free.  %a5 is used by the RELOC() macro on hp300
	 * and cannot be used until after the MMU is enabled.
	 */

/* determine our CPU/MMU combo - check for all regardless of kernel config */
	movl	#0x200,%d0		| data freeze bit
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
	movl	#30,%a0@		| @@@ useless
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
#if defined(ENABLE_HP_CODE)
	RELOC(ectype, %a0)
	movl	#EC_NONE,%a0@		| and no cache (for now XXX)
#endif
	RELOC(machineid, %a0)
	movl	#40,%a0@		| @@@ useless
	jra	Lstart1

	/*
	 * End of 68040 section
	 */

	/*
	 * 68020 models
	 * (There are no 68020 models of NeXT, but we'll pretend)
	 */

Lis68020:
	RELOC(mmutype, %a0)
	movl	#MMU_68851,%a0@		| no, we have PMMU
	RELOC(fputype, %a0)		| all of the 68020 systems
	movl	#FPU_68881,%a0@		|   have a 68881 FPU
	RELOC(cputype, %a0)
	movl	#CPU_68020,%a0@		| and a 68020 CPU
	RELOC(machineid, %a0)
	movl	#20,%a0@			| @@@ useless
	jra	Lstart1

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
	PANIC("Config botch in locore")

Lstart2:
/* initialize source/destination control registers for movs */
	moveq	#FC_USERD,%d0		| user space
	movc	%d0,%sfc		|   as source
	movc	%d0,%dfc		|   and destination of transfers
/* configure kernel and lwp0 VA space so we can get going */
#ifdef DDB
	RELOC(esym,%a0)			| end of static kernel test/data/syms
	movl	%a0@,%d5
	jne	Lstart3
#endif
	movl	#_C_LABEL(end),%d5	| end of static kernel text/data

Lstart3:
	addl	#NBPG-1,%d5
	andl	#PG_FRAME,%d5		| round to a page
	movl	%d5,%a4
	addl	%a5,%a4			| convert to PA
	pea	%a5@			| firstpa
	pea	%a4@			| nextpa
	RELOC(pmap_bootstrap,%a0)
	jbsr	%a0@			| pmap_bootstrap(firstpa,nextpa)
	addql	#8,%sp

/*
 * Prepare to enable MMU.
 * Since the kernel is not mapped logical == physical we must insure
 * that when the MMU is turned on, all prefetched addresses (including
 * the PC) are valid.  In order guarentee that, we use the last physical
 * page (which is conveniently mapped == VA) and load it up with enough
 * code to defeat the prefetch, then we execute the jump back to here.
 *
 * Is this all really necessary, or am I paranoid??
 */
	RELOC(Sysseg, %a0)		| system segment table addr
	movl	%a0@,%d1		| read value (a KVA)
	addl	%a5,%d1			| convert to PA

	RELOC(mmutype, %a0)
#if defined(ENABLE_HP_CODE)
	tstl	%a0@			| HP MMU?
	jeq	Lhpmmu2			| yes, skip
#endif
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

#if defined(ENABLE_HP_CODE)
	jra	Lstploaddone		| done
Lhpmmu2:
	moveq	#PGSHIFT,%d2
	lsrl	%d2,%d1			| convert to page frame
	movl	%d1,INTIOBASE+MMUBASE+MMUSSTP | load in sysseg table register
#endif
Lstploaddone:
#if defined(ENABLE_MAXADDR_TRAMPOLINE)
	lea	MAXADDR,%a2		| PA of last RAM page
	ASRELOC(Lhighcode, %a1)		| addr of high code
	ASRELOC(Lehighcode, %a3)	| end addr
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
#endif /* ENABLE_MAXADDR_TRAMPOLINE */

	/*
	 * Set up the vector table, and race to get the MMU
	 * enabled.
	 */

	movc    %vbr,%d0		| Keep copy of ROM VBR
	ASRELOC(save_vbr,%a0)
	movl    %d0,%a0@
	movl	#_C_LABEL(vectab),%d0	| set Vector Base Register
	movc	%d0,%vbr

	RELOC(mmutype, %a0)
#if defined(ENABLE_HP_CODE)
	tstl	%a0@			| HP MMU?
	jeq	Lhpmmu3			| yes, skip
#endif
	cmpl	#MMU_68040,%a0@		| 68040?
	jne	Lmotommu2		| no, skip
#if defined(ENABLE_HP_CODE)
	movw	#0,INTIOBASE+MMUBASE+MMUCMD+2
	movw	#MMU_IEN+MMU_CEN+MMU_FPE,INTIOBASE+MMUBASE+MMUCMD+2
					| enable FPU and caches
#endif

	| This is a hack to get PA=KVA when turning on MMU
	| it will only work on 68040's.  We should fix something
	| to boot 68030's later.
	movel	#0x0200c040,%d0		| intio devices are at 0x02000000
	.long	0x4e7b0004		| movc %d0,%itt0
	.long	0x4e7b0006		| movc %d0,%dtt0
	movel	#0x0403c000,%d0		| kernel text and data at 0x04000000
	.long	0x4e7b0005		| movc %d0,%itt1
	.long	0x4e7b0007		| movc %d0,%dtt1

	.word	0xf4d8			| cinva bc
	.word	0xf518			| pflusha
	movl	#0x8000,%d0
	.long	0x4e7b0003		| movc %d0,tc
	movl	#0x80008000,%d0
	movc	%d0,%cacr		| turn on both caches

	jmp     Lturnoffttr:l		| global jump into mapped memory.
Lturnoffttr:
	moveq	#0,%d0			| ensure TT regs are disabled
	.long	0x4e7b0004		| movc %d0,%itt0
	.long	0x4e7b0006		| movc %d0,%dtt0
	.long	0x4e7b0005		| movc %d0,%itt1
	.long	0x4e7b0007		| movc %d0,%dtt1
	jmp	Lenab1
Lmotommu2:
#if defined(ENABLE_HP_CODE)
	movl	#MMU_IEN+MMU_FPE,INTIOBASE+MMUBASE+MMUCMD
					| enable 68881 and i-cache
#endif
	RELOC(prototc, %a2)
	movl	#0x82c0aa00,%a2@	| value to load TC with
	pmove	%a2@,%tc		| load it
	jmp	Lenab1:l		| force absolute (not pc-relative) jmp
#if defined(ENABLE_HP_CODE)
Lhpmmu3:
	movl	#0,INTIOBASE+MMUBASE+MMUCMD	| clear external cache
	movl	#MMU_ENAB,INTIOBASE+MMUBASE+MMUCMD | turn on MMU
	jmp	Lenab1:l			| jmp to mapped code
#endif
#if defined(ENABLE_MAXADDR_TRAMPOLINE)
Lehighcode:

	/*
	 * END MMU TRAMPOLINE.  Address register %a5 is now free.
	 */
#endif

/*
 * Should be running mapped from this point on
 */
Lenab1:
/* select the software page size now */
	lea	_ASM_LABEL(tmpstk),%sp	| temporary stack
	jbsr	_C_LABEL(uvm_setpagesize) | select software page size
	bsr     Lpushpc			| Push the PC on the stack.
Lpushpc:


/* set kernel stack, user %SP, and initial pcb */
	movl	_C_LABEL(proc0paddr),%a1 | get lwp0 pcb addr
	lea	%a1@(USPACE-4),%sp	| set kernel stack to end of area
	lea	_C_LABEL(lwp0),%a2	| initialize lwp0.l_addr so that
	movl	%a1,%a2@(L_ADDR)	|   we don't deref NULL in trap()
	movl	#USRSTACK-4,%a2
	movl	%a2,%usp		| init user SP
	movl	%a1,_C_LABEL(curpcb)	| lwp0 is running

	tstl	_C_LABEL(fputype)	| Have an FPU?
	jeq	Lenab2			| No, skip.
	clrl	%a1@(PCB_FPCTX)		| ensure null FP context
	movl	%a1,%sp@-
	jbsr	_C_LABEL(m68881_restore) | restore it (does not kill %a1)
	addql	#4,%sp
Lenab2:
	cmpl	#MMU_68040,_C_LABEL(mmutype)	| 68040?
	jeq	Ltbia040		| yes, cache already on
	pflusha
	movl	#CACHE_ON,%d0
	movc	%d0,%cacr		| clear cache(s)
	jra	Lenab3
Ltbia040:
	.word	0xf518
Lenab3:

	jbsr	_C_LABEL(next68k_init)

/* Final setup for call to main(). */
/*
 * Create a fake exception frame so that cpu_fork() can copy it.
 * main() nevers returns; we exit to user mode from a forked process
 * later on.
 */
	clrw	%sp@-			| vector offset/frame type
	clrl	%sp@-			| PC - filled in by "execve"
	movw	#PSL_USER,%sp@-		| in user mode
	clrl	%sp@-			| stack adjust count and padding
	lea	%sp@(-64),%sp		| construct space for %D0-%D7/%A0-%A7
	lea	_C_LABEL(lwp0),%a0	| save pointer to frame
	movl	%sp,%a0@(L_MD_REGS)	|   in lwp0.p_md.md_regs

	jra	_C_LABEL(main)		| main()
	PANIC("main() returned")
	/* NOTREACHED */

/*
 * proc_trampoline: call function in register %a2 with %a3 as an arg
 * and then rei.
 */
GLOBAL(proc_trampoline)
	movl	%a3,%sp@-		| push function arg
	jbsr	%a2@			| call function
	addql	#4,%sp			| pop arg
	movl	%sp@(FR_SP),%a0		| grab and load
	movl	%a0,%usp		|   user SP
	moveml	%sp@+,#0x7FFF		| restore most user regs
	addql	#8,%sp			| toss SP and stack adjust
	jra	_ASM_LABEL(rei)		| and return


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
	andl	#PG_FRAME,%d1		| and truncate
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
	movl	%sp@,%d0			| ssw into low word of %d0
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
	clrl	%sp@-		| stack adjust count
	moveml	#0xFFFF,%sp@-	| save user registers
	movl	%usp,%a0		| and save
	movl	%a0,%sp@(FR_SP)	|   the user stack pointer
	clrl	%sp@-		| no VA arg
	movl	_C_LABEL(curpcb),%a0 | current pcb
	lea	%a0@(PCB_FPCTX),%a0 | address of FP savearea
	fsave	%a0@		| save state
#if defined(M68040) || defined(M68060)
	/* always null state frame on 68040, 68060 */
	cmpl	#FPU_68040,_C_LABEL(fputype)
	jle	Lfptnull
#endif
	tstb	%a0@		| null state frame?
	jeq	Lfptnull	| yes, safe
	clrw	%d0		| no, need to tweak BIU
	movb	%a0@(1),%d0	| get frame size
	bset	#3,%a0@(0,%d0:w) | set exc_pend bit of BIU
Lfptnull:
	fmovem	%fpsr,%sp@-	| push %fpsr as code argument
	frestore %a0@		| restore state
	movl	#T_FPERR,%sp@-	| push type arg
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
	tstl	_C_LABEL(astpending)
	jne	Lrei2
	tstb	_C_LABEL(ssir)
	jeq	Ltrap1
	movw	#SPL1,%sr
	tstb	_C_LABEL(ssir)
	jne	Lsir1
Ltrap1:	
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
	movl	_C_LABEL(curproc),%a0
	movl	%a0@(L_PROC),%sp@-	| push curproc pointer
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
	| Save the system %sp rather than the user %sp.
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
	bgt	Lbrkpt1

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
	| so push the hardware frame at the current %sp
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
 *
 * For auto-vectored interrupts, the CPU provides the
 * vector 0x18+level.  Note we count spurious interrupts,
 * but don't do anything else with them.
 *
 * _intrhand_autovec is the entry point for auto-vectored
 * interrupts.
 *
 * For vectored interrupts, we pull the pc, evec, and exception frame
 * and pass them to the vectored interrupt dispatcher.  The vectored
 * interrupt dispatcher will deal with strays.
 *
 * _intrhand_vectored is the entry point for vectored interrupts.
 */

#define INTERRUPT_SAVEREG	moveml  #0xC0C0,%sp@-
#define INTERRUPT_RESTOREREG	moveml  %sp@+,#0x0303

ENTRY_NOPROFILE(spurintr)	/* Level 0 */
	addql	#1,_C_LABEL(intrcnt)+0
	addql	#1,_C_LABEL(uvmexp)+UVMEXP_INTRS
	jra	_ASM_LABEL(rei)

ENTRY_NOPROFILE(intrhand_autovec)	/* Levels 1 through 6 */
	INTERRUPT_SAVEREG
	lea	%sp@(16),%a1		| get pointer to frame
	movl	%a1,%sp@-
	movw	%sp@(26),%d0
	movl	%d0,%sp@-		| push exception vector info
	movl	%sp@(26),%sp@-		| and PC
	jbsr	_C_LABEL(isrdispatch_autovec)	| call dispatcher
	lea	%sp@(12),%sp		| pop value args
	INTERRUPT_RESTOREREG
	jra	_ASM_LABEL(rei)		| all done

ENTRY_NOPROFILE(lev7intr)	/* level 7: parity errors, reset key */
	addql	#1,_C_LABEL(intrcnt)+32
	clrl	%sp@-
	moveml	#0xFFFF,%sp@-		| save registers
	movl	%usp,%a0		| and save
	movl	%a0,%sp@(FR_SP)		|   the user stack pointer
	jbsr	_C_LABEL(nmihand)	| call handler
	movl	%sp@(FR_SP),%a0		| restore
	movl	%a0,%usp			|   user SP
	moveml	%sp@+,#0x7FFF		| and remaining registers
	addql	#8,%sp			| pop SP and stack adjust
	jra	_ASM_LABEL(rei)		| all done

ENTRY_NOPROFILE(intrhand_vectored)
	INTERRUPT_SAVEREG
	lea	%sp@(16),%a1		| get pointer to frame
	movl	%a1,%sp@-
	movw	%sp@(26),%d0
	movl	%d0,%sp@-		| push exception vector info
	movl	%sp@(26),%sp@-		| and PC
	jbsr	_C_LABEL(isrdispatch_vectored)	| call dispatcher
	lea	%sp@(12),%sp		| pop value args
	INTERRUPT_RESTOREREG
	jra	_ASM_LABEL(rei)		| all done

#undef INTERRUPT_SAVEREG
#undef INTERRUPT_RESTOREREG

/*
 * Emulation of VAX REI instruction.
 *
 * This code deals with checking for and servicing ASTs
 * (profiling, scheduling) and software interrupts (network, softclock).
 * We check for ASTs first, just like the VAX.  To avoid excess overhead
 * the T_ASTFLT handling code will also check for software interrupts so we
 * do not have to do it here.  After identifing that we need an AST we
 * drop the IPL to allow device interrupts.
 *
 * This code is complicated by the fact that sendsig may have been called
 * necessitating a stack cleanup.
 */

BSS(ssir,1)

ASENTRY_NOPROFILE(rei)
	tstl	_C_LABEL(astpending)	| AST pending?
	jeq	Lchksir			| no, go check for SIR
Lrei1:
	btst	#5,%sp@			| yes, are we returning to user mode?
	jne	Lchksir			| no, go check for SIR
	movw	#PSL_LOWIPL,%sr		| lower SPL
	clrl	%sp@-			| stack adjust
	moveml	#0xFFFF,%sp@-		| save all registers
	movl	%usp,%a1		| including
	movl	%a1,%sp@(FR_SP)		|    the users SP
Lrei2:
	clrl	%sp@-			| VA == none
	clrl	%sp@-			| code == none
	movl	#T_ASTFLT,%sp@-		| type == async system trap
	jbsr	_C_LABEL(trap)		| go handle it
	lea	%sp@(12),%sp		| pop value args
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
Lchksir:
	tstb	_C_LABEL(ssir)		| SIR pending?
	jeq	Ldorte			| no, all done
	movl	%d0,%sp@-		| need a scratch register
	movw	%sp@(4),%d0		| get SR
	andw	#PSL_IPL7,%d0		| mask all but IPL
	jne	Lnosir			| came from interrupt, no can do
	movl	%sp@+,%d0		| restore scratch register
Lgotsir:
	movw	#SPL1,%sr		| prevent others from servicing int
	tstb	_C_LABEL(ssir)		| too late?
	jeq	Ldorte			| yes, oh well...
	clrl	%sp@-			| stack adjust
	moveml	#0xFFFF,%sp@-		| save all registers
	movl	%usp,%a1		| including
	movl	%a1,%sp@(FR_SP)		|    the users SP
Lsir1:	
	clrl	%sp@-			| VA == none
	clrl	%sp@-			| code == none
	movl	#T_SSIR,%sp@-		| type == software interrupt
	jbsr	_C_LABEL(trap)		| go handle it
	lea	%sp@(12),%sp		| pop value args
	movl	%sp@(FR_SP),%a0		| restore
	movl	%a0,%usp		|   user SP
	moveml	%sp@+,#0x7FFF		| and all remaining registers
	addql	#8,%sp			| pop SP and stack adjust
	rte
Lnosir:
	movl	%sp@+,%d0		| restore scratch register
Ldorte:
	rte				| real return

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
 * Use common m68k process manipulation routines.
 */
#include <m68k/m68k/proc_subr.s>

/*
 * Use common m68k process/lwp switch and context save subroutines.
 */
#define FPCOPROC	/* XXX: Temp. Reqd. */
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
	clrl	%a1@(PCB_ONFAULT) 	| clear fault address
	rts
#endif

#if defined(ENABLE_HP_CODE)
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
#endif

ENTRY_NOPROFILE(getsfc)
	movc	%sfc,%d0
	rts

ENTRY_NOPROFILE(getdfc)
	movc	%dfc,%d0
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
	.long	0x4e7b0806		| movc %d0,urp
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
 * Set processor priority level calls.  Most are implemented with
 * inline asm expansions.  However, spl0 requires special handling
 * as we need to check for our emulated software interrupts.
 */

ENTRY(spl0)
	moveq	#0,%d0
	movw	%sr,%d0			| get old SR for return
	movw	#PSL_LOWIPL,%sr		| restore new SR
	tstb	_C_LABEL(ssir)		| software interrupt pending?
	jeq	Lspldone		| no, all done
	subql	#4,%sp			| make room for RTE frame
	movl	%sp@(4),%sp@(2)		| position return address
	clrw	%sp@(6)			| set frame type 0
	movw	#PSL_LOWIPL,%sp@	| and new SR
	jra	Lgotsir			| go handle it
Lspldone:
	rts

ENTRY(getsr)
	moveq	#0,%d0
	movw	%sr,%d0
	rts

/*
 * _delay(u_int N)
 *
 * Delay for at least (N/256) microsecends.
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
 * Save and restore 68881 state.
 */
ENTRY(m68881_save)
	movl	%sp@(4),%a0		| save area pointer
	fsave	%a0@			| save state
	tstb	%a0@			| null state frame?
	jeq	Lm68881sdone		| yes, all done
	fmovem	%fp0-%fp7,%a0@(FPF_REGS) | save FP general registers
	fmovem	%fpcr/%fpsr/%fpi,%a0@(FPF_FPCR)	| save FP control registers
Lm68881sdone:
	rts

ENTRY(m68881_restore)
	movl	%sp@(4),%a0		| save area pointer
	tstb	%a0@			| null state frame?
	jeq	Lm68881rdone		| yes, easy
	fmovem	%a0@(FPF_FPCR),%fpcr/%fpsr/%fpi	| restore FP control registers
	fmovem	%a0@(FPF_REGS),%fp0-%fp7 | restore FP general registers
Lm68881rdone:
	frestore %a0@			| restore state
	rts

/*
 * Handle the nitty-gritty of rebooting the machine.
 * Basically we just turn off the MMU and jump to the appropriate ROM routine.
 * Note that we must be running in an address range that is mapped one-to-one
 * logical to physical so that the PC is still valid immediately after the MMU
 * is turned off.  We have conveniently mapped the last page of physical
 * memory this way.
 */
ENTRY_NOPROFILE(doboot)
	movw	#PSL_HIGHIPL,%sr	| no interrupts

	movl	#CACHE_OFF,%d0
	movc	%d0,%cacr		| clear and disable on-chip cache(s)

	| Turn on physical memory mapping.
	| @@@ This is also 68040 specific and needs fixing.
	movel	#0x0200c040,%d0		| intio devices are at 0x02000000
	.long	0x4e7b0004		| movc %d0,%itt0
	.long	0x4e7b0006		| movc %d0,%dtt0
	movel	#0x0403c000,%d0		| kernel text and data at 0x04000000
	.long	0x4e7b0005		| movc %d0,%itt1
	.long	0x4e7b0007		| movc %d0,%dtt1

	moveal   #NEXT_RAMBASE,%a5	| amount to RELOC by.

	| Create a new stack at address tmpstk, and push
	| The existing sp onto it for kicks.
	ASRELOC(tmpstk, %a0)
	movel	%sp,%a0@-
	moveal  %a0,%sp
	moveal  #0,%a6

	ASRELOC(Ldoboot1, %a0)
	jmp     %a0@			| jump into physical address space.
Ldoboot1:
	ASRELOC(save_vbr, %a0)
	movl    %a0@,%d0
	movc    %d0,%vbr

	| reset the registers as the boot rom likes them:
	movel	#0x0200c040,%d0		|
	.long	0x4e7b0004		| movc %d0,%itt0
	.long	0x4e7b0006		| movc %d0,%dtt0
	movel	#0x00ffc000,%d0		|
	.long	0x4e7b0005		| movc %d0,%itt1
	.long	0x4e7b0007		| movc %d0,%dtt1

	RELOC(monbootflag, %a0)
	movel %a0,%d0			| "-h" halts instead of reboot.
	trap #13

hloop:  
	bra hloop			| This shouldn't be reached.
/*
 * Misc. global variables.
 */
	.data
GLOBAL(machineid)
	.long	0xdeadbeef	| default to @@@

GLOBAL(mmuid)
	.long	0		| default to nothing

GLOBAL(mmutype)
	.long	0xdeadbeef	| default to 68040 mmu

GLOBAL(cputype)
	.long	0xdeadbeef	| default to 68020 CPU

#if defined(ENABLE_HP_CODE)
GLOBAL(ectype)
	.long	EC_NONE		| external cache type, default to none
#endif

GLOBAL(fputype)
	.long	0xdeadbeef	| default to 68882 FPU

GLOBAL(protorp)
	.long	0,0		| prototype root pointer

GLOBAL(prototc)
	.long	0		| prototype translation control

GLOBAL(want_resched)
	.long	0

GLOBAL(proc0paddr)
	.long	0		| KVA of lwp0 u-area

GLOBAL(intiobase)
	.long	INTIOBASE	| KVA of base of internal IO space

GLOBAL(intiolimit)
	.long	INTIOTOP	| KVA of end of internal IO space

GLOBAL(monobase)
	.long	MONOBASE	| KVA of base of mono FB

GLOBAL(monolimit)
	.long	MONOTOP		| KVA of end of mono FB

GLOBAL(colorbase)
	.long	COLORBASE	| KVA of base of color FB

GLOBAL(colorlimit)
	.long	COLORTOP	| KVA of end of color FB

ASLOCAL(save_vbr)		| VBR from ROM
	.long 0xdeadbeef

GLOBAL(monbootflag)
	.long 0

#if defined(ENABLE_HP_CODE)
GLOBAL(extiobase)
	.long	0		| KVA of base of external IO space

GLOBAL(CLKbase)
	.long	0		| KVA of base of clock registers

GLOBAL(MMUbase)
	.long	0		| KVA of base of HP MMU registers

GLOBAL(pagezero)
	.long	0		| PA of first page of kernel text
#endif

#ifdef USELEDS
ASLOCAL(heartbeat)
	.long	0		| clock ticks since last pulse of heartbeat

ASLOCAL(beatstatus)
	.long	0		| for determining a fast or slow throb
#endif

#ifdef DEBUG
ASGLOBAL(fulltflush)
	.long	0

ASGLOBAL(fullcflush)
	.long	0
#endif

/* interrupt counters */
GLOBAL(intrnames)
	.asciz	"spur"
	.asciz	"lev1"
	.asciz	"lev2"
	.asciz	"lev3"
	.asciz	"lev4"
	.asciz	"lev5"
	.asciz	"lev6"
	.asciz  "lev7"
	.asciz	"nmi"
GLOBAL(eintrnames)
	.even
GLOBAL(intrcnt)
	.long	0,0,0,0,0,0,0,0,0
GLOBAL(eintrcnt)

