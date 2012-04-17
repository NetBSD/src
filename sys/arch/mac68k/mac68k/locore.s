/*	$NetBSD: locore.s,v 1.163.4.1 2012/04/17 00:06:36 yamt Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990 The Regents of the University of California.
 * All rights reserved.
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
 */

/*-
 * Copyright (C) 1993	Allen K. Briggs, Chris P. Caputo,
 *			Michael L. Finch, Bradley A. Grantham, and
 *			Lawrence A. Kesteloot
 * All rights reserved.
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
 *	This product includes software developed by the Alice Group.
 * 4. The names of the Alice Group or any of its members may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ALICE GROUP ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE ALICE GROUP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * from: Utah $Hdr: locore.s 1.58 91/04/22$
 *
 *	@(#)locore.s	7.11 (Berkeley) 5/9/91
 */

#include "opt_compat_netbsd.h"
#include "opt_compat_svr4.h"
#include "opt_compat_sunos.h"
#include "opt_ddb.h"
#include "opt_fpu_emulate.h"
#include "opt_kgdb.h"
#include "opt_lockdebug.h"
#include "opt_fpsp.h"
#include "opt_m68k_arch.h"

#include "assym.h"

#include <machine/asm.h>
#include <machine/trap.h>

/*
 * This is for kvm_mkdb, and should be the address of the beginning
 * of the kernel text segment (not necessarily the same as kernbase).
 */
	.text
GLOBAL(kernel_text)

#include <mac68k/mac68k/vectors.s>
#include <mac68k/mac68k/macglobals.s>

/*
 * Initialization
 */

	.data
| Scratch memory.  Careful when messing with these...
ASLOCAL(longscratch)
	.long	0
ASLOCAL(longscratch2)
	.long	0
ASLOCAL(pte_tmp)			| for get_pte()
	.long	0 
GLOBAL(macos_crp1)
	.long	0
GLOBAL(macos_crp2)
	.long	0
GLOBAL(macos_tc)
	.long	0
GLOBAL(macos_tt0)
	.long	0
GLOBAL(macos_tt1)
	.long	0
GLOBAL(bletch)
	.long	0

BSS(esym,4)

ASENTRY_NOPROFILE(start)
	movw	#PSL_HIGHIPL,%sr	| no interrupts.  ever.
	lea	_ASM_LABEL(tmpstk),%sp	| give ourselves a temporary stack

	movl	#CACHE_OFF,%d0
	movc	%d0,%cacr		| clear and disable on-chip cache(s)

	/* Initialize source/destination control registers for movs */
	movql	#FC_USERD,%d0		| user space
	movc	%d0,%sfc		|   as source
	movc	%d0,%dfc		|   and destination of transfers

	/*
	 * Some parameters provided by MacOS
	 *
	 * LAK: This section is the new way to pass information from the booter
	 * to the kernel.  At A1 there is an environment variable which has
	 * a bunch of stuff in ascii format, "VAR=value\0VAR=value\0\0".
	 */
	movl	%a1,%sp@-		| Address of buffer
	movl	%d4,%sp@-		| Some flags... (mostly not used)
	jbsr	_C_LABEL(getenvvars)	| Parse the environment buffer
	addql	#8,%sp

	/* Determine MMU/MPU from what we can test empirically */
	movl	#0x200,%d0		| data freeze bit
	movc	%d0,%cacr		|   only exists on 68030
	movc	%cacr,%d0		| read it back
	tstl	%d0			| zero?
	jeq	Lnot68030		| yes, we have 68020/68040

	movl	#CACHE_OFF,%d0		| disable and clear both caches
	movc	%d0,%cacr
	lea	_C_LABEL(mmutype),%a0	| no, we have 68030
	movl	#MMU_68030,%a0@		| set to reflect 68030 PMMU
	lea	_C_LABEL(cputype),%a0
	movl	#CPU_68030,%a0@		| and 68030 MPU
	jra	Lstart1

Lnot68030:
	bset	#31,%d0			| data cache enable bit
	movc	%d0,%cacr		|   only exists on 68040
	movc	%cacr,%d0		| read it back
	tstl	%d0			| zero?
	beq	Lis68020		| yes, we have 68020

	movql	#CACHE40_OFF,%d0	| now turn it back off
	movc	%d0,%cacr		|   before we access any data
	.word	0xf4f8			| cpusha bc ;push and invalidate caches
	lea	_C_LABEL(mmutype),%a0
	movl	#MMU_68040,%a0@		| Reflect 68040 MMU
	lea	_C_LABEL(cputype),%a0
	movl	#CPU_68040,%a0@		| and 68040 MPU
	jra	Lstart1

Lis68020:
	movl	#CACHE_OFF,%d0		| disable and clear cache
	movc	%d0,%cacr
	lea	_C_LABEL(mmutype),%a0	| Must be 68020+68851
	movl	#MMU_68851,%a0@		| Reflect 68851 PMMU
	lea	_C_LABEL(cputype),%a0
	movl	#CPU_68020,%a0@		| and 68020 MPU

Lstart1:
	/*
	 * Now that we know what CPU we have, initialize the address error
	 * and bus error handlers in the vector table:
	 *
	 *	vectab+8	bus error
	 *	vectab+12	address error
	 */
	lea	_C_LABEL(cputype),%a0
	lea	_C_LABEL(vectab),%a2
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
	movl	_C_LABEL(MacOSROMBase),%a1 | Load MacOS ROMBase
	jra	Ldoboot1

Lstart2:
	jbsr	_C_LABEL(setmachdep)	| Set some machine-dep stuff
	jbsr	_C_LABEL(consinit)	| XXX Should only be if graybar on

/*
 * Figure out MacOS mappings and bootstrap NetBSD
 */
	lea	_C_LABEL(macos_tc),%a0	| get current %TC
	cmpl	#MMU_68040,_C_LABEL(mmutype) | check to see if 68040
	jeq	Lget040TC

	pmove	%tc,%a0@
	jra	Lstart3

Lget040TC:
#if 0
	movl	_C_LABEL(current_mac_model),%a1	 | if an AV Mac, save current
	cmpl	#MACH_CLASSAV,%a1@(CPUINFO_CLASS) | %TC so internal video will
	jne	LnotAV				 | get configured
#endif
	.long	0x4e7a0003		| movc %tc,%d0
	jra	LsaveTC
LnotAV:	
	movql	#0,%d0			| otherwise,
	.long	0x4e7b0003		| movc %d0,%tc ;Disable MMU
LsaveTC:	
	movl	%d0,%a0@

Lstart3:
	movl	%a0@,%sp@-		| get Mac OS mapping, relocate video,
	jbsr	_C_LABEL(bootstrap_mac68k) |   bootstrap pmap, et al.
	addql	#4,%sp

	/*
	 * Set up the vector table, and race to get the MMU
	 * enabled.
	 */
	movl	#_C_LABEL(vectab),%d0	| set Vector Base Register
	movc	%d0,%vbr
	
	movl	_C_LABEL(Sysseg),%a1	| system segment table addr
	addl	_C_LABEL(load_addr),%a1	| Make it physical addr
	cmpl	#MMU_68040,_C_LABEL(mmutype)
	jne	Lenablepre040MMU	| if not 040, skip

	movql	#0,%d0
	.long	0x4e7b0003		| movc %d0,%tc   ;Disable MMU
	.long	0x4e7b0004		| movc %d0,%itt0 ;Disable itt0
	.long	0x4e7b0005		| movc %d0,%itt1 ;Disable itt1
	.long	0x4e7b0006		| movc %d0,%dtt0 ;Disable dtt0
	.long	0x4e7b0007		| movc %d0,%dtt1 ;Disable dtt1
	movl	%a1,%d1
	.word	0xf518			| pflusha
	.long	0x4e7b1807		| movc %d1,%srp
#if PGSHIFT == 13
	movl	#0xc000,%d0
#else
	movl	#0x8000,%d0
#endif
	.long	0x4e7b0003		| movc %d0,%tc   ;Enable MMU
	movl	#CACHE40_ON,%d0
	movc	%d0,%cacr		| turn on both caches
	jra	Lloaddone

Lenablepre040MMU:
	tstl	_C_LABEL(mmutype)	| TTx instructions will break 68851
	jgt	LnokillTT

	lea	_ASM_LABEL(longscratch),%a0 | disable TTx registers on 68030
	movl	#0,%a0@
	.long	0xf0100800		| movl %a0@,%tt0
	.long	0xf0100c00		| movl %a0@,%tt1

LnokillTT:
	lea	_C_LABEL(protorp),%a0
	movl	#0x80000202,%a0@	| nolimit + share global + 4 byte PTEs
	movl	%a1,%a0@(4)		| + segtable address
	pmove	%a0@,%srp		| load the supervisor root pointer
	movl	#0x80000002,%a0@	| reinit upper half for CRP loads
	pflusha
	lea	_ASM_LABEL(longscratch),%a2
#if PGSHIFT == 13
	movl	#0x82d08b00,%a2@	| value to load %TC with
#else
	movl	#0x82c0aa00,%a2@	| value to load %TC with
#endif
	pmove	%a2@,%tc		| load it

Lloaddone:

/*
 * Should be running mapped from this point on
 */
	lea	_ASM_LABEL(tmpstk),%sp	| temporary stack
/* call final pmap setup */
	jbsr	_C_LABEL(pmap_bootstrap_finalize)
/* set kernel stack, user SP, lwp0, and initial pcb */
	movl	_C_LABEL(lwp0uarea),%a1	| get lwp0 uarea
	lea	%a1@(USPACE-4),%sp	|   set kernel stack to end of area
	movl	#USRSTACK-4,%a2
	movl	%a2,%usp		| init %USP

/* flush TLB and turn on caches */
	cmpl	#MMU_68040,_C_LABEL(mmutype) | 68040?
	jeq	Ltbia040		| yes, cache already on
	pflusha
	movl	#CACHE_ON,%d0
	movc	%d0,%cacr		| clear cache(s)
#ifdef __notyet__
	tstl	_C_LABEL(ectype)
	jeq	Lnocache0
					| Enable external cache here
#endif
	jra	Lnocache0

Ltbia040:
	.word	0xf518			| pflusha

Lnocache0:
/* Final setup for call to main(). */
	jbsr	_C_LABEL(mac68k_init)

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
GLOBAL(mac68k_a2_fromfault)
	.long	0
GLOBAL(m68k_fault_addr)
	.long	0

#if defined(M68040) || defined(M68060)
ENTRY_NOPROFILE(addrerr4060)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save user registers
	movl	%usp,%a0		| save %USP
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
	movl	%usp,%a0		| save %USP
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
	movl	%a2,_C_LABEL(mac68k_a2_fromfault) | save %a2
	movl	%sp@(FR_HW+8+8),_C_LABEL(m68k_fault_addr) | save fault addr
	movl	_C_LABEL(nofault),%sp@-	| yes,
	jbsr	_C_LABEL(longjmp)	|  longjmp(nofault)
	/* NOTREACHED */
#endif
#if defined(M68040)
ENTRY_NOPROFILE(buserr40)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save user registers
	movl	%usp,%a0		| save %USP
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
	movl	%a2,_C_LABEL(mac68k_a2_fromfault) | save %a2
	movl	%sp@(FR_HW+8+20),_C_LABEL(m68k_fault_addr) | save fault addr
	movl	_C_LABEL(nofault),%sp@-	| yes,
	jbsr	_C_LABEL(longjmp)	|  longjmp(nofault)
	/* NOTREACHED */
#endif

ENTRY_NOPROFILE(busaddrerr2030)
#if !(defined(M68020) || defined(M68030))
	jra	_C_LABEL(badtrap)
#else
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save user registers
	movl	%usp,%a0		| save %USP
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
	movl	%d1,%a0			| fault address
	movl	%sp@,%d0		| function code from ssw
	btst	#8,%d0			| data fault?
	jne	Lbe10a
	movql	#1,%d0			| user program access FC
					| (we do not separate data/program)
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
Lismerr:
	movl	#T_MMUFLT,%sp@-		| show that we are an MMU fault
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
Lmightnotbemerr:
	btst	#3,%d1			| write protect bit set?
	jeq	Lisberr1		| no: must be bus error
	movl	%sp@,%d0		| ssw into low word of %d0
	andw	#0xc0,%d0		| Write protect is set on page:
	cmpw	#0x40,%d0		| was it read cycle?
	jne	Lismerr			| no, was not WPE, must be MMU fault
	jra	Lisberr1		| real bus err needs not be fast.
Lisaerr:
	movl	#T_ADDRERR,%sp@-	| mark address error
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
Lisberr1:
	clrw	%sp@			| re-clear pad word
	tstl	_C_LABEL(nofault)	| catch bus error?
	jeq	Lisberr			| no, handle as usual
	movl	%a2,_C_LABEL(mac68k_a2_fromfault) | save %a2
	movl	%sp@(FR_HW+8+16),_C_LABEL(m68k_fault_addr) | save fault addr
	movl	_C_LABEL(nofault),%sp@-	| yes,
	jbsr	_C_LABEL(longjmp)	|  longjmp(nofault)
	/* NOTREACHED */
#endif
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
	movl	%usp,%a0	| and save
	movl	%a0,%sp@(FR_SP)	|   the user stack pointer
	clrl	%sp@-		| no VA arg
	movl	_C_LABEL(curpcb),%a0 | current pcb
	lea	%a0@(PCB_FPCTX),%a0 | address of FP savearea
	fsave	%a0@		| save state
#if defined(M68040) || defined(M68060)
	/* always null state frame on 68040, 68060 */
	cmpl	#FPU_68040,_C_LABEL(fputype)
	jge	Lfptnull
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
	clrl	%sp@-			| pad SR to longword
	moveml	#0xFFFF,%sp@-		| save user registers
	movl	%usp,%a0		| save %USP
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
	movl	%a0,%usp		|   %USP
	moveml	%sp@+,#0x7FFF		| restore most registers
	addql	#8,%sp			| pop SSP and align word
	rte

/*
 * Trap 12 is the entry point for the cachectl "syscall" (both HP-UX & BSD)
 *	cachectl(command, addr, length)
 * command in %d0, addr in %a1, length in %d1
 */
ENTRY_NOPROFILE(trap12)
	movl	_C_LABEL(curlwp),%a0
	movl	%a0@(L_PROC),%sp@-	| push proc pointer
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
	| Save the system %sp rather than the user %usp.
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
 * Most 68k-based Macintosh computers
 *
 *      Level 0:        Spurious: ignored
 *      Level 1:        VIA1 (clock, ADB)
 *      Level 2:        VIA2 (NuBus, SCSI)
 *      Level 3:
 *      Level 4:        Serial (SCC)
 *      Level 5:
 *      Level 6:
 *      Level 7:        Non-maskable: parity errors, RESET button
 *
 * On the Q700, Q900 and Q950 in "A/UX mode": this should become:
 *
 *	Level 0:        Spurious: ignored
 *	Level 1:        Software
 *	Level 2:        VIA2 (except ethernet, sound)
 *	Level 3:        Ethernet
 *	Level 4:        Serial (SCC)
 *	Level 5:        Sound
 *	Level 6:        VIA1
 *	Level 7:        NMIs: parity errors, RESET button, YANCC error
 *
 * On the 660AV and 840AV:
 *
 *	Level 0:        Spurious: ignored
 *	Level 1:        VIA1 (clock, ADB)
 *	Level 2:        VIA2 (NuBus, SCSI)
 *	Level 3:        PSC device interrupt
 *	Level 4:        PSC DMA and serial
 *	Level 5:        ???
 *	Level 6:        ???
 *	Level 7:        NMIs: parity errors?, RESET button
 */

ENTRY_NOPROFILE(spurintr)
	addql	#1,_C_LABEL(intrcnt)+0
	INTERRUPT_SAVEREG
	CPUINFO_INCREMENT(CI_NINTR)
	INTERRUPT_RESTOREREG
	jra	_ASM_LABEL(rei)

ENTRY_NOPROFILE(intrhand)
	INTERRUPT_SAVEREG
	movw	%sp@(22),%sp@-		| push exception vector info
	clrw	%sp@-
	jbsr	_C_LABEL(intr_dispatch)	| call dispatch routine
	addql	#4,%sp
	INTERRUPT_RESTOREREG
	jra	_ASM_LABEL(rei)		| all done

ENTRY_NOPROFILE(lev7intr)
	addql	#1,_C_LABEL(intrcnt)+16
	clrl	%sp@-			| pad %SR to longword
	moveml	#0xFFFF,%sp@-		| save registers
	movl	%usp,%a0		| and save
	movl	%a0,%sp@(FR_SP)		|   the user stack pointer
	jbsr	_C_LABEL(nmihand)	| call handler
	movl	%sp@(FR_SP),%a0		| restore
	movl	%a0,%usp		|   %USP
	moveml	%sp@+,#0x7FFF		| and remaining registers
	addql	#8,%sp			| pop SSP and align word
	jra	_ASM_LABEL(rei)

/* 
 * We could tweak rtclock_intr and gain 12 cycles on the 020 and 030 by
 * saving the status register directly to the stack, but this would lose
 * badly on the 040.  Aligning the stack takes 10 more cycles than this
 * code does, so it's a good compromise.
 */
ENTRY_NOPROFILE(rtclock_intr)
	movl	%d2,%sp@-		| save %d2
	movw	%sr,%d2			| save SPL
	movw	_C_LABEL(ipl2psl_table)+IPL_CLOCK*2,%sr
					| raise SPL to splclock()
	movl	%a6@,%a1		| unwind to frame in intr_dispatch
	lea	%a1@(28),%a1		| push pointer to interrupt frame
	movl	%a1,%sp@-			| 28 = 16 for regs in intrhand,
					|    + 4 for args to intr_dispatch
					|    + 4 for return address to intrhand
					|    + 4 for value of %A6
	jbsr	_C_LABEL(hardclock)	| call generic clock int routine
	addql	#4,%sp			| pop param
	jbsr	_C_LABEL(mrg_VBLQueue)	| give programs in the VBLqueue a chance
	addql	#1,_C_LABEL(intrcnt)+32	| record a clock interrupt
	INTERRUPT_SAVEREG
	CPUINFO_INCREMENT(CI_NINTR)
	INTERRUPT_RESTOREREG
	movw	%d2,%sr			| restore SPL
	movl	%sp@+,%d2		| restore %d2
	rts				| go back from whence we came

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
	movl	%a1,%sp@(FR_SP)		|    %USP
Lrei2:
	clrl	%sp@-			| VA == none
	clrl	%sp@-			| code == none
	movl	#T_ASTFLT,%sp@-		| type == async system trap
	pea	%sp@(12)		| fp == address of trap frame
	jbsr	_C_LABEL(trap)		| go handle it
	lea	%sp@(16),%sp		| pop value args
	movl	%sp@(FR_SP),%a0		| restore %USP
	movl	%a0,%usp		|   from save area
	movw	%sp@(FR_ADJ),%d0	| need to adjust stack?
	jne	Laststkadj		| yes, go to it
	moveml	%sp@+,#0x7FFF		| no, restore most user regs
	addql	#8,%sp			| toss %SP and stack adjust
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
	movl	%sp@,%sp		| and our %SP
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
	movl	%a1,%sp@(FR_SP)		|    %USP
Lsir1:	
	clrl	%sp@-			| VA == none
	clrl	%sp@-			| code == none
	movl	#T_SSIR,%sp@-		| type == software interrupt
	pea	%sp@(12)		| fp == address of trap frame
	jbsr	_C_LABEL(trap)		| go handle it
	lea	%sp@(16),%sp		| pop value args
	movl	%sp@(FR_SP),%a0		| restore
	movl	%a0,%usp		|   %USP
	moveml	%sp@+,#0x7FFF		| and all remaining registers
	addql	#8,%sp			| pop %SP and stack adjust
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

ENTRY(ecacheon)
	rts

ENTRY(ecacheoff)
	rts

/*
 * Load a new user segment table pointer.
 */
ENTRY(loadustp)
	movl	%sp@(4),%d0		| new USTP
	moveq	#PGSHIFT,%d1
	lsll	%d1,%d0			| convert to addr
#if defined(M68040)
	cmpl	#MMU_68040,_C_LABEL(mmutype) | 68040?
	jne	LmotommuC		| no, skip
	.word	0xf518			| pflusha
	.long	0x4e7b0806		| movec %d0, URP
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

/*
 * Set processor priority level calls.  Most are implemented with
 * inline asm expansions.  However, spl0 requires special handling
 * as we need to check for our emulated software interrupts.
 */

ALTENTRY(splnone, _spl0)
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

/*
 * delay() - delay for a specified number of microseconds
 * _delay() - calibrator helper for delay()
 *
 * Notice that delay_factor is scaled up by a factor of 128 to avoid loss
 * of precision for small delays.  As a result of this we need to avoid
 * overflow.
 *
 * The branch target for the loops must be aligned on a half-line (8-byte)
 * boundary to minimize cache effects.  This guarantees both that there
 * will be no prefetch stalls due to cache line burst operations and that
 * the loops will run from a single cache half-line.
 */
	.align	8			| align to half-line boundary
					| (use nop instructions if necessary!)
ALTENTRY(_delay, _delay)
ENTRY(delay)
	movl	%sp@(4),%d0		| get microseconds to delay
	cmpl	#0x40000,%d0		| is it a "large" delay?
	bls	Ldelayshort		| no, normal calculation
	movql	#0x7f,%d1		| adjust for scaled multipler (to
	addl	%d1,%d0			|   avoid overflow)
	lsrl	#7,%d0
	mulul	_C_LABEL(delay_factor),%d0 | calculate number of loop iterations
	bra	Ldelaysetup		| go do it!
Ldelayshort:
	mulul	_C_LABEL(delay_factor),%d0 | calculate number of loop iterations
	lsrl	#7,%d0			| adjust for scaled multiplier
Ldelaysetup:
	jeq	Ldelayexit		| bail out if nothing to do
	movql	#0,%d1			| put bits 15-0 in %d1 for the
	movw	%d0,%d1			|   inner loop, and move bits
	movw	#0,%d0			|   31-16 to the low-order word
	subql	#1,%d1			|   of %d0 for the outer loop
	swap	%d0
Ldelay:
	tstl	_C_LABEL(delay_flag)	| this never changes for delay()!
	dbeq	%d1,Ldelay		|   (used only for timing purposes)
	dbeq	%d0,Ldelay
	addqw	#1,%d1			| adjust end count and
	swap	%d0			|    return the longword result
	orl	%d1,%d0
Ldelayexit:
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
#if defined(M68040)
	cmpl	#MMU_68040,_C_LABEL(mmutype) | 68040?
	jeq	Lnocache5		| yes, skip
#endif
	movl	#CACHE_OFF,%d0
	movc	%d0,%cacr		| disable on-chip cache(s)
Lnocache5:
	movl	_C_LABEL(maxaddr),%a0	| last page of physical memory
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
	movl	_C_LABEL(maxaddr),%a0
	jmp	%a0@			| jump to last page

Lbootcode:
	lea	%a0@(0x800),%sp		| physical %SP in case of NMI
	movl	_C_LABEL(MacOSROMBase),%a1 | Load MacOS ROMBase

#if defined(M68040)
	cmpl	#MMU_68040,_C_LABEL(mmutype) | 68040?
	jne	LmotommuF		| no, skip
	movl	#0,%d0
	movc	%d0,%cacr		| caches off
	.long	0x4e7b0003		| movc %d0,%tc (disable MMU)
	jra	Ldoboot1
LmotommuF:
#endif
	lea	_ASM_LABEL(longscratch),%a3
	movl	#0,%a3@			| value for pmove to %TC (turn off MMU)
	pmove	%a3@,%tc		| disable MMU

Ldoboot1:
	lea	%a1@(0x90),%a1		| offset of ROM reset routine
	jmp	%a1@			| and jump to ROM to reset machine
Lebootcode:

/*
 * u_long ptest040(void *addr, u_int fc);
 *
 * ptest040() does an 040 PTESTR (addr) and returns the 040 MMUSR iff
 * translation is enabled.  This allows us to find the physical address
 * corresponding to a MacOS logical address for get_physical().
 * sar  01-oct-1996
 */
ENTRY_NOPROFILE(ptest040)
#if defined(M68040)
	.long	0x4e7a0003		| movc %tc,%d0
	andw	#0x8000,%d0
	jeq	Lget_phys1		| MMU is disabled
	movc	%dfc,%d1		| Save %DFC
	movl	%sp@(8),%d0		| Set FC for ptestr
	movc	%d0,%dfc
	movl	%sp@(4),%a0		| logical address to look up
	.word	0xf568			| ptestr (%a0)
	.long	0x4e7a0805		| movc %mmusr,%d0
	movc	%d1,%dfc		| Restore %DFC
	rts
Lget_phys1:
#endif
	movql	#0,%d0			| return failure
	rts

/*
 * LAK: (7/24/94) This routine was added so that the
 *  C routine that runs at startup can figure out how MacOS
 *  had mapped memory.  We want to keep the same mapping so
 *  that when we set our MMU pointer, the PC doesn't point
 *  in the middle of nowhere.
 *
 * long get_pte(void *addr, unsigned long pte[2], unsigned short *psr)
 *
 *  Takes "addr" and looks it up in the current MMU pages.  Puts
 *  the PTE of that address in "pte" and the result of the
 *  search in "psr".  "pte" should be 2 longs in case it is
 *  a long-format entry.
 *
 *  One possible problem here is that setting the TT register
 *  may screw something up if we access user data space in a
 *  called function or in an interrupt service routine.
 *
 *  Returns -1 on error, 0 if pte is a short-format pte, or
 *  1 if pte is a long-format pte.
 *
 *  Be sure to only call this routine if the MMU is enabled.  This
 *  routine is probably more general than it needs to be -- it
 *  could simply return the physical address (replacing
 *  get_physical() in machdep).
 *
 *  "gas" does not understand the %tt0 register, so we must hand-
 *  assemble the instructions.
 */
ENTRY_NOPROFILE(get_pte)
	subql	#4,%sp		| make temporary space

	lea	_ASM_LABEL(longscratch),%a0
	movl	#0x00ff8710,%a0@ | Set up FC 1 r/w access
	.long	0xf0100800	| pmove %a0@,%tt0

	movl	%sp@(8),%a0	| logical address to look up
	movl	#0,%a1		| clear in case of failure
	ptestr	#FC_USERD,%a0@,#7,%a1 | search for logical address
	pmove	%psr,%sp@	| store processor status register
	movw	%sp@,%d1
	movl	%sp@(16),%a0	| where to store the %psr
	movw	%d1,%a0@	| send back to caller
	andw	#0xc400,%d1	| if bus error, exceeded limit, or invalid
	jne	get_pte_fail1	| leave now
	tstl	%a1		| check address we got back
	jeq	get_pte_fail2	| if 0, then was not set -- fail

	movl	%a1,%d0
	movl	%d0,_ASM_LABEL(pte_tmp)	| save for later

	| send first long back to user
	movl	%sp@(12),%a0	| address of where to put pte
	movsl	%a1@,%d0	|
	movl	%d0,%a0@	| first long

	andl	#3,%d0		| dt bits of pte
	cmpl	#1,%d0		| should be 1 if page descriptor
	jne	get_pte_fail3	| if not, get out now

	movl	%sp@(16),%a0	| addr of stored %psr
	movw	%a0@,%d0	| get %psr again
	andw	#7,%d0		| number of levels it found
	addw	#-1,%d0		| find previous level
	movl	%sp@(8),%a0	| logical address to look up
	movl	#0,%a1		| clear in case of failure

	cmpl	#0,%d0
	jeq	pte_level_zero
	cmpl	#1,%d0
	jeq	pte_level_one
	cmpl	#2,%d0
	jeq	pte_level_two
	cmpl	#3,%d0
	jeq	pte_level_three
	cmpl	#4,%d0
	jeq	pte_level_four
	cmpl	#5,%d0
	jeq	pte_level_five
	cmpl	#6,%d0
	jeq	pte_level_six
	jra	get_pte_fail4	| really should have been one of these...

pte_level_zero:
	| must get CRP to get length of entries at first level
	lea	_ASM_LABEL(longscratch),%a0 | space for two longs
	pmove	%crp,%a0@	| save root pointer
	movl	%a0@,%d0	| load high long
	jra	pte_got_parent
pte_level_one:
	ptestr	#FC_USERD,%a0@,#1,%a1 | search for logical address
	pmove	%psr,%sp@	| store processor status register
	movw	%sp@,%d1
	jra	pte_got_it
pte_level_two:
	ptestr	#FC_USERD,%a0@,#2,%a1 | search for logical address
	pmove	%psr,%sp@	| store processor status register
	movw	%sp@,%d1
	jra	pte_got_it
pte_level_three:
	ptestr	#FC_USERD,%a0@,#3,%a1 | search for logical address
	pmove	%psr,%sp@	| store processor status register
	movw	%sp@,%d1
	jra	pte_got_it
pte_level_four:
	ptestr	#FC_USERD,%a0@,#4,%a1 | search for logical address
	pmove	%psr,%sp@	| store processor status register
	movw	%sp@,%d1
	jra	pte_got_it
pte_level_five:
	ptestr	#FC_USERD,%a0@,#5,%a1 | search for logical address
	pmove	%psr,%sp@	| store processor status register
	movw	%sp@,%d1
	jra	pte_got_it
pte_level_six:
	ptestr	#FC_USERD,%a0@,#6,%a1 | search for logical address
	pmove	%psr,%sp@	| store processor status register
	movw	%sp@,%d1

pte_got_it:
	andw	#0xc400,%d1	| if bus error, exceeded limit, or invalid
	jne	get_pte_fail5	| leave now
	tstl	%a1		| check address we got back
	jeq	get_pte_fail6	| if 0, then was not set -- fail

	movsl	%a1@,%d0		| get pte of parent
	movl	%d0,_C_LABEL(macos_tt0)	| XXX for later analysis (kill me)
pte_got_parent:
	andl	#3,%d0		| dt bits of pte
	cmpl	#2,%d0		| child is short-format descriptor
	jeq	short_format
	cmpl	#3,%d0		| child is long-format descriptor
	jne	get_pte_fail7

	| long_format -- we must go back, change the tt, and get the
	|  second long.  The reason we didn't do this in the first place
	|  is that the first long might have been the last long of RAM.

	movl	_ASM_LABEL(pte_tmp),%a1	| get address of our original pte
	addql	#4,%a1		| address of ite second long

	| send second long back to user
	movl	%sp@(12),%a0	| address of where to put pte
	movsl	%a1@,%d0	|
	movl	%d0,%a0@(4)	| write in second long

	movql	#1,%d0		| return long-format
	jra	get_pte_success

short_format:
	movql	#0,%d0		| return short-format
	jra	get_pte_success

get_pte_fail:
	movql	#-1,%d0		| return failure

get_pte_success:
	lea	_ASM_LABEL(longscratch),%a0 | disable tt
	movl	#0,%a0@
	.long	0xf0100800	| pmove %a0@,%tt0

	addql	#4,%sp		| return temporary space
	rts

get_pte_fail1:
	jbsr	_C_LABEL(printstar)
	jra	get_pte_fail
get_pte_fail2:
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jra	get_pte_fail
get_pte_fail3:
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jra	get_pte_fail
get_pte_fail4:
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jra	get_pte_fail
get_pte_fail5:
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jra	get_pte_fail
get_pte_fail6:
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jra	get_pte_fail
get_pte_fail7:
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jra	get_pte_fail
get_pte_fail8:
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jra	get_pte_fail
get_pte_fail9:
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jra	get_pte_fail
get_pte_fail10:
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jbsr	_C_LABEL(printstar)
	jra	get_pte_fail

/*
 * Misc. global variables.
 */
	.data
GLOBAL(sanity_check)
	.long	0x18621862	| this is our stack overflow checker.

	.space	4 * PAGE_SIZE
ASLOCAL(tmpstk)

GLOBAL(machineid)
	.long	0		| default to 320

GLOBAL(mmutype)
	.long	MMU_68851	| default to 68851 PMMU

GLOBAL(cputype)
	.long	CPU_68020	| default to 68020 CPU

#ifdef __notyet__
GLOBAL(ectype)
	.long	EC_NONE		| external cache type, default to none
#endif

GLOBAL(fputype)
	.long	FPU_68882	| default to 68882 FPU

GLOBAL(protorp)
	.long	0,0		| prototype root pointer

GLOBAL(intiolimit)
	.long	0		| KVA of end of internal IO space

GLOBAL(load_addr)
	.long	0		| Physical address of kernel

ASLOCAL(lastpage)
	.long	0		| LAK: to store the addr of last page in mem

GLOBAL(MacOSROMBase)
	.long	0x40800000
GLOBAL(mac68k_vrsrc_cnt)
	.long	0
GLOBAL(mac68k_vrsrc_vec)
	.word	0, 0, 0, 0, 0, 0

#ifdef DEBUG
ASGLOBAL(fulltflush)
	.long	0

ASGLOBAL(fullcflush)
	.long	0
#endif

/* interrupt counters -- leave some space for overriding the names */

GLOBAL(intrnames)
	.asciz	"spur    "
	.asciz	"via1    "
	.asciz	"via2    "
	.asciz	"unused1 "
	.asciz	"scc     "
	.asciz	"unused2 "
	.asciz	"unused3 "
	.asciz	"nmi     "
	.asciz	"clock   "
GLOBAL(eintrnames)
	.even

GLOBAL(intrcnt)
	.long	0,0,0,0,0,0,0,0,0
GLOBAL(eintrcnt)
