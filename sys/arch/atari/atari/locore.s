/*	$NetBSD: locore.s,v 1.87 2002/05/19 20:26:58 leo Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1980, 1990 The Regents of the University of California.
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
 * from: Utah $Hdr: locore.s 1.58 91/04/22$
 *
 *	@(#)locore.s	7.11 (Berkeley) 5/9/91
 */

/*
 *
 * Original (hp300) Author: unknown, maybe Mike Hibler?
 * Amiga author: Markus Wild
 * Atari Modifications: Leo Weppelman
 */

#include "opt_compat_netbsd.h"
#include "opt_compat_svr4.h"
#include "opt_compat_sunos.h"
#include "opt_ddb.h"
#include "opt_fpsp.h"
#include "opt_kgdb.h"
#include "opt_lockdebug.h"
#include "opt_mbtype.h"
#include "kbd.h"
#include "ncrscsi.h"
#include "zs.h"

#include "assym.h"
#include <machine/asm.h>

/*
 * This is for kvm_mkdb, and should be the address of the beginning
 * of the kernel text segment (not necessarily the same as kernbase).
 */
	.text
	GLOBAL(kernel_text)

/*
 * Clear & skip page zero, it will not be mapped
 */
	.fill	NBPG/4,4,0

#include <atari/atari/vectors.s>

	.text
	.even
/*
 * Do a dump.
 * Called by auto-restart.
 */
ENTRY_NOPROFILE(doadump)
	jbsr	_C_LABEL(dumpsys)
	jbsr	_C_LABEL(doboot)
	/*NOTREACHED*/

/*
 * Trap/interrupt vector routines
 */ 
#include <m68k/m68k/trap_subr.s>

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
#endif /* defined(M68040) || defined(M68060) */

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
	jeq	Lisberr
	movl	#T_MMUFLT,%sp@-		| show that we are an MMU fault
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
#endif /* defined(M68060) */

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
	jeq	Lisberr			| it is a bus error
	movl	#T_MMUFLT,%sp@-		| show that we are an MMU fault
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
#endif /* defined(M68040) */

#if defined(M68020) || defined(M68030)
ENTRY_NOPROFILE(buserr2030)
ENTRY_NOPROFILE(addrerr2030)
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
	jeq	Lismerr			| no, must be fast
	jra	Lisberr1		| real bus err needs not be fast.
Lmightnotbemerr:
	btst	#3,%d1			| write protect bit set?
	jeq	Lisberr1		| no: must be bus error
	movl	%sp@,%d0		| ssw into low word of d0
	andw	#0xc0,%d0		| Write protect is set on page:
	cmpw	#0x40,%d0		| was it read cycle?
	jeq	Lisberr1		| yes, was not WPE, must be bus err
Lismerr:
	movl	#T_MMUFLT,%sp@-		| show that we are an MMU fault
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
Lisaerr:
	movl	#T_ADDRERR,%sp@-	| mark address error
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
Lisberr1:
	clrw	%sp@			| re-clear pad word
#endif /* !(defined(M68020) || defined(M68030)) */

Lisberr:				| also used by M68040/60
	tstl	_C_LABEL(nofault)	| device probe?
	jeq	LberrIsProbe		| no, handle as usual
	movl	_C_LABEL(nofault),%sp@-	| yes,
	jbsr	_C_LABEL(longjmp)	|  longjmp(nofault)
	/* NOTREACHED */
LberrIsProbe:
	movl	#T_BUSERR,%sp@-		| mark bus error
	jra	_ASM_LABEL(faultstkadj)	| and deal with it

	/*
	 * This is where the default vectors end-up!
	 * At the time of the 'machine-type' probes, it seems necessary
	 * that the 'nofault' test is done first. Because the MMU is not
	 * yet setup at this point, the real fault handlers sometimes
	 * misinterpret the cause of the fault.
	 */
ENTRY_NOPROFILE(buserr)
ENTRY_NOPROFILE(addrerr)
	tstl	_C_LABEL(nofault)	| device probe?
	jeq	1f			| no, halt...
	movl	_C_LABEL(nofault),%sp@-	| yes,
	jbsr	_C_LABEL(longjmp)	|  longjmp(nofault)
	/* NOTREACHED */
1:
	jra	_C_LABEL(badtrap)	| only catch probes!

/*
 * FP exceptions.
 */
ENTRY_NOPROFILE(fpfline)
	cmpl	#MMU_68040,_C_LABEL(mmutype)
	jne	fpfline_not40		|  not a 040 FPU, do 6888? emulation
	cmpw	#0x202c,%sp@(6)		|  format type 2?
	jne	_C_LABEL(illinst)	|  no, not an FP emulation
#ifdef FPSP
	jmp	_ASM_LABEL(fpsp_unimp)	|  yes, go handle it
#endif
fpfline_not40:
	clrl	%sp@-			|  stack adjust count
	moveml	#0xFFFF,%sp@-		|  save registers
	moveq	#T_FPEMULI,%d0		|  denote as FP emulation trap
	jra	_ASM_LABEL(fault)	|  do it

ENTRY_NOPROFILE(fpunsupp)
	cmpl	#MMU_68040,_C_LABEL(mmutype)
	jne	fpunsupp_not40		| not a 040 FPU, try 6888?
#ifdef FPSP
	jmp	_ASM_LABEL(fpsp_unsupp)	|  yes, go handle it
#endif
fpunsupp_not40:
	clrl	%sp@-			|  stack adjust count
	moveml	#0xFFFF,%sp@-		|  save registers
	moveq	#T_FPEMULD,%d0		|  denote as FP emulation trap
	jra	_ASM_LABEL(fault)	|  do it

/*
 * Handles all other FP coprocessor exceptions.
 * Note that since some FP exceptions generate mid-instruction frames
 * and may cause signal delivery, we need to test for stack adjustment
 * after the trap call.
 */
ENTRY_NOPROFILE(fpfault)
	clrl	%sp@-			|  stack adjust count
	moveml	#0xFFFF,%sp@-		|  save user registers
	movl	%usp,%a0		|  and save
	movl	%a0,%sp@(FR_SP)		|    the user stack pointer
	clrl	%sp@-			|  no VA arg
	movl	_C_LABEL(curpcb),%a0	|  current pcb
	lea	%a0@(PCB_FPCTX),%a0	|  address of FP savearea
	fsave	%a0@			|  save state

#if defined(M68040) || defined(M68060)
#ifdef notdef /* XXX: Can't use this while we don't have the cputype */
	movb	_C_LABEL(cputype), %d0
	andb	#(ATARI_68040|ATARI_68060), %d0
	jne	Lfptnull
#else
	cmpb	#0x41,%a0@		|  is it the 68040 FPU-frame format?
	jeq	Lfptnull		|  yes, safe
#endif /* notdef */
#endif /* defined(M68040) || defined(M68060) */

	tstb	%a0@			|  null state frame?
	jeq	Lfptnull		|  yes, safe
	clrw	%d0			|  no, need to tweak BIU
	movb	%a0@(1),%d0		|  get frame size
	bset	#3,%a0@(0,%d0:w)	|  set exc_pend bit of BIU
Lfptnull:
	fmovem	%fpsr,%sp@-		|  push fpsr as code argument
	frestore %a0@			|  restore state
	movl	#T_FPERR,%sp@-		|  push type arg
	jra	_ASM_LABEL(faultstkadj)	|  call trap and deal with stack cleanup

/*
 * Other exceptions only cause four and six word stack frame and require
 * no post-trap stack adjustment.
 */

ENTRY_NOPROFILE(intr_glue)
	moveml	%d0-%d1/%a0-%a1,%sp@-	|  Save scratch registers
	jbsr	_C_LABEL(intr_dispatch)	|  handle interrupt
	moveml	%sp@+,%d0-%d1/%a0-%a1
	jra	_ASM_LABEL(rei)

ENTRY_NOPROFILE(lev2intr)
	rte				|  HBL, can't be turned off on Falcon!

ENTRY_NOPROFILE(lev4intr)		|  VBL interrupt
#ifdef FALCON_VIDEO
	tstl	_C_LABEL(falcon_needs_vbl)
	jne	1f			|  Yes, go service a VBL-request
	rte				|  Nothing to do.
1:
	moveml	%d0-%d1/%a0-%a1,%sp@-
	jbsr	_C_LABEL(falcon_display_switch)
	moveml	%sp@+,%d0-%d1/%a0-%a1
#endif /* FALCON_VIDEO */
	rte

ENTRY_NOPROFILE(lev5intr)
ENTRY_NOPROFILE(lev6intr)

#ifdef _MILANHW_
	/* XXX
	 * Need to find better places to define these (Leo)
	 */
#define	PLX_PCICR	0x4204
#define	PLX_CNTRL	0x42ec
#define	PLX_DMCFGA	0x42ac
	moveml	%d0-%d2/%a0-%a1,%sp@-
	movw	%sp@(20),%sp@-		|  push previous SR value
	clrw	%sp@-			|	padded to longword
	movl	_C_LABEL(stio_addr),%a0	| get KVA of ST-IO area
	movew	#0xffff,%a0@(PLX_PCICR)	| clear PCI_SR error bits
	movel	%a0@(PLX_CNTRL),%d0	| Change PCI command code from
	andw	#0xf0ff,%d0
	movw	%sr,%d2			| Block interrupts for now
	oriw	#0x0700,%sr
	movl	%d0,%a0@(PLX_CNTRL)
	movq	#0,%d1			| clear upper bits
					| Read any (uncached!) PCI address
					|  to fetch vector number
	movl	_C_LABEL(pci_mem_uncached),%a1
	movb	%a1@,%d1
	orw	#0x0600,%d0		| Change PCI command code back
	movel	%d0,%a0@(PLX_CNTRL)	|  to Read Cycle
	movew	%d2,%sr			| Re-enable interrupts
	movel	%d1,%sp@-		| Call handler
	jbsr	_C_LABEL(milan_isa_intr)
	addql	#8,%sp
	moveml	%sp@+,%d0-%d2/%a0-%a1
	jra	_ASM_LABEL(rei)

/*
 * Support functions for reading and writing the Milan PCI config space.
 * Of interest:
 *   - We need exclusive access to the PLX9080 during config space
 *     access, hence the splhigh().
 *   - The 'confread' function shortcircuits the NMI to make probes to
 *     unexplored pci-config space possible.
 */
ENTRY(milan_pci_confread)
	movl	%sp@(4),%d0		| get tag and regno
	bset	#31,%d0			| add config space flag
	andl	#~3,%d0			| access type 0
	movl	_C_LABEL(stio_addr),%a0	| get KVA of ST-IO area
	movw	%sr,%d1			| goto splhigh
	oriw	#0x0700,%sr
	movb	#1,_ASM_LABEL(plx_nonmi)| no NMI interrupts please!
	movl	%d0,%a0@(PLX_DMCFGA)	| write tag to the config register
	movl	_C_LABEL(pci_io_addr),%a1
	movl	%a1@,%d0		| fetch value
	movl	#0,%a0@(PLX_DMCFGA)	| back to normal PCI access

					| Make sure the C-function can peek
	movw	%a0@(PLX_PCICR),_C_LABEL(plx_status) | at the access results.

	movw	#0xf900,%a0@(PLX_PCICR)	| Clear potential error bits
	movb	#0, _ASM_LABEL(plx_nonmi)
	movw	%d1,%sr			| splx
	rts

ENTRY(milan_pci_confwrite)
	movl	%sp@(4),%d0		| get tag and regno
	bset	#31,%d0			| add config space flag
	andl	#~3,%d0			| access type 0
	movl	_C_LABEL(stio_addr),%a0	| get KVA of ST-IO area
	movw	%sr,%d1			| goto splhigh
	oriw	#0x0700,%sr
	movl	%d0,%a0@(PLX_DMCFGA)	| write tag to the config register
	movl	_C_LABEL(pci_io_addr),%a1
	movl	%sp@(8),%a1@		| write value
	movl	#0,%a0@(PLX_DMCFGA)	| back to normal PCI access
	movw	%d1,%sr			| splx
	rts

ENTRY_NOPROFILE(lev7intr)
	tstl	_ASM_LABEL(plx_nonmi)	| milan_conf_read shortcut
	jne	1f			| .... get out immediately
	moveml	%d0-%d1/%a0-%a1,%sp@-
	movl	_C_LABEL(stio_addr),%a0	| get KVA of ST-IO area
	movw	%a0@(PLX_PCICR),_C_LABEL(plx_status)
	movw	#0xf900,%a0@(PLX_PCICR)	| Clear error bits
	jbsr	_C_LABEL(nmihandler)	| notify...
	moveml	%sp@+,%d0-%d1/%a0-%a1
	addql	#1,_C_LABEL(intrcnt)+28	| add another nmi interrupt
1:
	rte				| all done
#endif /* _MILANHW_ */

ENTRY_NOPROFILE(lev3intr)
ENTRY_NOPROFILE(badtrap)
	moveml	#0xC0C0,%sp@-		|  save scratch regs
	movw	%sp@(22),%sp@-		|  push exception vector info
	clrw	%sp@-
	movl	%sp@(22),%sp@-		|  and PC
	jbsr	_C_LABEL(straytrap)	|  report
	addql	#8,%sp			|  pop args
	moveml	%sp@+,#0x0303		|  restore regs
	jra	_ASM_LABEL(rei)		|  all done

ENTRY_NOPROFILE(badmfpint)
	moveml	#0xC0C0,%sp@-		|  save scratch regs
	movw	%sp@(22),%sp@-		|  push exception vector info
	clrw	%sp@-
	movl	%sp@(22),%sp@-		|  and PC
	jbsr	_C_LABEL(straymfpint)	|  report
	addql	#8,%sp			|  pop args
	moveml	%sp@+,#0x0303		|  restore regs
	jra	_ASM_LABEL(rei)		|  all done

ENTRY_NOPROFILE(trap0)
	clrl	%sp@-			|  stack adjust count
	moveml	#0xFFFF,%sp@-		|  save user registers
	movl	%usp,%a0		|  save the user SP
	movl	%a0,%sp@(FR_SP)		|    in the savearea
	movl	%d0,%sp@-		|  push syscall number
	jbsr	_C_LABEL(syscall)	|  handle it
	addql	#4,%sp			|  pop syscall arg
	movl	%sp@(FR_SP),%a0		|  grab and restore
	movl	%a0,%usp		|    user SP
	moveml	%sp@+,#0x7FFF		|  restore most registers
	addql	#8,%sp			|  pop SP and stack adjust
	jra	_ASM_LABEL(rei)		|  all done

/*
 * Trap 12 is the entry point for the cachectl "syscall"
 *	cachectl(command, addr, length)
 * command in d0, addr in a1, length in d1
 */
ENTRY_NOPROFILE(trap12)
	movl	_C_LABEL(curproc),%sp@-	|  push curproc pointer
	movl	%d1,%sp@-		|  push length
	movl	%a1,%sp@-		|  push addr
	movl	%d0,%sp@-		|  push command
	jbsr	_C_LABEL(cachectl1)	|  do it
	lea	%sp@(16),%sp		|  pop args
	jra	_ASM_LABEL(rei)		|  all done

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
	clrl	%sp@-
	moveml	#0xFFFF,%sp@-
	moveq	#T_TRAP15,%d0
	movw	%sp@(FR_HW),%d1		|  get PSW
	andw	#PSL_S,%d1		|  from system mode?
	jne	Lkbrkpt			|  yes, kernel breakpoint
	jra	_ASM_LABEL(fault)	|  no, user-mode fault

Lkbrkpt:
	| Kernel-mode breakpoint or trace trap. (d0=trap_type)
	| Save the system sp rather than the user sp.
	movw	#PSL_HIGHIPL,%sr	| lock out interrupts
	lea	%sp@(FR_SIZE),%a6	| Save stack pointer
	movl	%a6,%sp@(FR_SP)		|  from before trap

	| If were are not on tmpstk switch to it.
	| (so debugger can change the stack pointer)
	movl	%a6,%d1
	cmpl	#_ASM_LABEL(tmpstk),%d1
	jls	Lbrkpt2				| already on tmpstk
	| Copy frame to the temporary stack
	movl	%sp,%a0				| a0=src
	lea	_ASM_LABEL(tmpstk)-96,%a1	| a1=dst
	movl	%a1,%sp				| sp=new frame
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
	| Save args in d2, a2
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
#if 0	/* not needed on atari */
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

	movl	%sp@(FR_SP),%a0		| modified sp
	lea	%sp@(FR_SIZE),%a1	| end of our frame
	movl	%a1@-,%a0@-		| copy 2 longs with
	movl	%a1@-,%a0@-		| ... predecrement
	movl	%a0,%sp@(FR_SP)		| sp = h/w frame
	moveml	%sp@+,#0x7FFF		| restore all but sp
	movl	%sp@,%sp		| ... and sp
	rte				| all done

/* Use common m68k sigreturn */
#include <m68k/m68k/sigreturn.s>

/*
 * Interrupt handlers.
 *
 *	Level 0:	Spurious: ignored.
 *	Level 1:	softint
 *	Level 2:	HBL
 *	Level 3:	not used
 *	Level 4:	not used
 *	Level 5:	SCC (not used)
 *	Level 6:	MFP1/MFP2 (not used -> autovectored)
 *	Level 7:	Non-maskable: shouldn't be possible. ignore.
 */

/* Provide a generic interrupt dispatcher, only handle hardclock (int6)
 * specially, to improve performance
 */

ENTRY_NOPROFILE(spurintr)
	addql	#1,_C_LABEL(intrcnt)+0
	addql	#1,_C_LABEL(uvmexp)+UVMEXP_INTRS
	jra	_ASM_LABEL(rei)

	/* MFP timer A handler --- System clock --- */
ASENTRY_NOPROFILE(mfp_tima)
	moveml	%d0-%d1/%a0-%a1,%sp@-	|  save scratch registers
	movl	%sp,%sp@-		|  push pointer to clockframe
	jbsr	_C_LABEL(hardclock)	|  call generic clock int routine
	addql	#4,%sp			|  pop params
	addql	#1,_C_LABEL(intrcnt_user)+52
					|  add another system clock interrupt
	moveml	%sp@+,%d0-%d1/%a0-%a1	|  restore scratch regs	
	addql	#1,_C_LABEL(uvmexp)+UVMEXP_INTRS
	jra	_ASM_LABEL(rei)		|  all done

#ifdef STATCLOCK
	/* MFP timer C handler --- Stat/Prof clock --- */
ASENTRY_NOPROFILE(mfp_timc)
	moveml	%d0-%d1/%a0-%a1,%sp@-	|  save scratch registers
	jbsr	_C_LABEL(statintr)	|  call statistics clock handler
	addql	#1,_C_LABEL(intrcnt)+36	|  add another stat clock interrupt
	moveml	%sp@+,%d0-%d1/%a0-%a1	|  restore scratch regs	
	addql	#1,_C_LABEL(uvmexp)+UVMEXP_INTRS
	jra	_ASM_LABEL(rei)		|  all done
#endif /* STATCLOCK */

#if NKBD > 0
	/* MFP ACIA handler --- keyboard/midi --- */
ASENTRY_NOPROFILE(mfp_kbd)
	addql	#1,_C_LABEL(intrcnt)+8	|  add another kbd/mouse interrupt

	moveml	%d0-%d1/%a0-%a1,%sp@-	|  Save scratch registers
	movw	%sp@(16),%sp@-		|  push previous SR value
	clrw	%sp@-			|     padded to longword
	jbsr	_C_LABEL(kbdintr)	|  handle interrupt
	addql	#4,%sp			|  pop SR
	moveml	%sp@+,%d0-%d1/%a0-%a1
	addql	#1,_C_LABEL(uvmexp)+UVMEXP_INTRS
	jra	_ASM_LABEL(rei)
#endif /* NKBD */

#if NNCRSCSI > 0
	/* MFP2 SCSI DMA handler --- NCR5380 --- */
ASENTRY_NOPROFILE(mfp2_5380dm)
	addql	#1,_C_LABEL(intrcnt)+24	|  add another 5380-DMA interrupt

	moveml	%d0-%d1/%a0-%a1,%sp@-	|  Save scratch registers
	movw	%sp@(16),%sp@-		|  push previous SR value
	clrw	%sp@-			|     padded to longword
	jbsr	_C_LABEL(scsi_dma)	|  handle interrupt
	addql	#4,%sp			|  pop SR
	moveml	%sp@+,%d0-%d1/%a0-%a1
	addql	#1,_C_LABEL(uvmexp)+UVMEXP_INTRS
	jra	_ASM_LABEL(rei)

	/* MFP2 SCSI handler --- NCR5380 --- */
ASENTRY_NOPROFILE(mfp2_5380)
	addql	#1,_C_LABEL(intrcnt)+20	|  add another 5380-SCSI interrupt

	moveml	%d0-%d1/%a0-%a1,%sp@-	|  Save scratch registers
	movw	%sp@(16),%sp@-		|  push previous SR value
	clrw	%sp@-			|     padded to longword
	jbsr	_C_LABEL(scsi_ctrl)	|  handle interrupt
	addql	#4,%sp			|  pop SR
	moveml	%sp@+,%d0-%d1/%a0-%a1
	addql	#1,_C_LABEL(uvmexp)+UVMEXP_INTRS
	jra	_ASM_LABEL(rei)
#endif /* NNCRSCSI > 0 */

#if NZS > 0
	/* SCC Interrupt --- modem2/serial2 --- */
ASENTRY_NOPROFILE(sccint)
	addql	#1,_C_LABEL(intrcnt)+32	|  add another SCC interrupt

	moveml	%d0-%d1/%a0-%a1,%sp@-	|  Save scratch registers
	movw	%sp@(16),%sp@-		|  push previous SR value
	clrw	%sp@-			|     padded to longword
	jbsr	_C_LABEL(zshard)	|  handle interrupt
	addql	#4,%sp			|  pop SR
	moveml	%sp@+,%d0-%d1/%a0-%a1
	addql	#1,_C_LABEL(uvmexp)+UVMEXP_INTRS
	jra	_ASM_LABEL(rei)
#endif /* NZS > 0 */

#ifdef _ATARIHW_
	/* Level 1 (Software) interrupt handler */
ENTRY_NOPROFILE(lev1intr)
	moveml	%d0-%d1/%a0-%a1,%sp@-
	movl	_C_LABEL(stio_addr),%a0 |  get KVA of ST-IO area
	moveb	#0, %a0@(SCU_SOFTINT)	|  Turn off software interrupt
	addql	#1,_C_LABEL(intrcnt)+16	|  add another software interrupt
	jbsr	_C_LABEL(softint)	|  handle software interrupts
	moveml	%sp@+,%d0-%d1/%a0-%a1
	addql	#1,_C_LABEL(uvmexp)+UVMEXP_INTRS
	jra	_ASM_LABEL(rei)

	/*
	 * Should never occur, except when special hardware modification
	 * is installed. In this case, one expects to be dropped into
	 * the debugger.
	 */
ENTRY_NOPROFILE(lev7intr)
#ifdef DDB
	/*
	 * Note that the nmi has to be turned off while handling it because
	 * the hardware modification has no de-bouncing logic....
	 */
	movl	%a0, %sp@-		|  save a0
	movl	_C_LABEL(stio_addr),%a0	|  get KVA of ST-IO area
	movb	%a0@(SCU_SYSMASK),%sp@-	|  save current sysmask
	movb	#0, %a0@(SCU_SYSMASK)	|  disable all interrupts
	trap	#15			|  drop into the debugger
	movb	%sp@+, %a0@(SCU_SYSMASK)|  restore sysmask
	movl	%sp@+, %a0		|  restore a0
#endif
	addql	#1,_C_LABEL(intrcnt)+28	|  add another nmi interrupt
	rte				|  all done

#endif /* _ATARIHW_ */


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
 * necessitating a stack cleanup.  A cleanup should only be needed at this
 * point for coprocessor mid-instruction frames (type 9), but we also test
 * for bus error frames (type 10 and 11).
 */
	BSS(ssir,1)
ASENTRY_NOPROFILE(rei)
#ifdef DEBUG
	tstl	_C_LABEL(panicstr)	|  have we paniced?
	jne	Ldorte			|  yes, do not make matters worse
#endif
	tstl	_C_LABEL(astpending)	|  AST pending?
	jeq	Lchksir			|  no, go check for SIR
Lrei1:
	btst	#5,%sp@			|  yes, are we returning to user mode?
	jne	Lchksir			|  no, go check for SIR
	movw	#PSL_LOWIPL,%sr		|  lower SPL
	clrl	%sp@-			|  stack adjust
	moveml	#0xFFFF,%sp@-		|  save all registers
	movl	%usp,%a1		|  including
	movl	%a1,%sp@(FR_SP)		|     the users SP
	clrl	%sp@-			|  VA == none
	clrl	%sp@-			|  code == none
	movl	#T_ASTFLT,%sp@-		|  type == async system trap
	jbsr	_C_LABEL(trap)		|  go handle it	
	lea	%sp@(12),%sp		|  pop value args
	movl	%sp@(FR_SP),%a0		|  restore user SP
	movl	%a0,%usp		|    from save area
	movw	%sp@(FR_ADJ),%d0	|  need to adjust stack?
	jne	Laststkadj		|  yes, go to it
	moveml	%sp@+,#0x7FFF		|  no, restore most user regs
	addql	#8,%sp			|  toss SP and stack adjust
	rte				|  and do real RTE
Laststkadj:
	lea	%sp@(FR_HW),%a1		|  pointer to HW frame
	addql	#8,%a1			|  source pointer
	movl	%a1,%a0			|  source
	addw	%d0,%a0			|   + hole size = dest pointer
	movl	%a1@-,%a0@-		|  copy
	movl	%a1@-,%a0@-		|   8 bytes
	movl	%a0,%sp@(FR_SP)		|  new SSP
	moveml	%sp@+,#0x7FFF		|  restore user registers
	movl	%sp@,%sp		|  and our SP
	rte				|  and do real RTE
Lchksir:
	tstb	_C_LABEL(ssir)		|  SIR pending?
	jeq	Ldorte			|  no, all done
	movl	%d0,%sp@-		|  need a scratch register
	movw	%sp@(4),%d0		|  get SR
	andw	#PSL_IPL7,%d0		|  mask all but IPL
	jne	Lnosir			|  came from interrupt, no can do
	movl	%sp@+,%d0		|  restore scratch register
Lgotsir:
	movw	#SPL1,%sr		|  prevent others from servicing int
	tstb	_C_LABEL(ssir)		|  too late?
	jeq	Ldorte			|  yes, oh well...
	clrl	%sp@-			|  stack adjust
	moveml	#0xFFFF,%sp@-		|  save all registers
	movl	%usp,%a1		|  including
	movl	%a1,%sp@(FR_SP)		|     the users SP
	clrl	%sp@-			|  VA == none
	clrl	%sp@-			|  code == none
	movl	#T_SSIR,%sp@-		|  type == software interrupt
	jbsr	_C_LABEL(trap)		|  go handle it
	lea	%sp@(12),%sp		|  pop value args
	movl	%sp@(FR_SP),%a0		|  restore	
	movl	%a0,%usp		|    user SP
	moveml	%sp@+,#0x7FFF		|  and all remaining registers
	addql	#8,%sp			|  pop SP and stack adjust	
	rte
Lnosir:
	movl	%sp@+,%d0		|  restore scratch register
Ldorte:
	rte				|  real return	

/*
 * Initialization
 *
 * A5 contains physical load point from boot
 * exceptions vector thru our table, that's bad.. just hope nothing exceptional
 * happens till we had time to initialize ourselves..
 */
	BSS(lowram,4)
	BSS(esym,4)

	.globl	_C_LABEL(edata)
	.globl	_C_LABEL(etext),_C_LABEL(end)

GLOBAL(bootversion)
	.word	0x0003			|  Glues kernel/installboot/loadbsd
					|    and other bootcode together.
ASENTRY_NOPROFILE(start)
	movw	#PSL_HIGHIPL,%sr	| No interrupts

	/*
	 * a0 = start of loaded kernel
	 * a1 = value of esym
	 * d0 = fastmem size
	 * d1 = stmem size
	 * d2 = cputype
	 * d3 = boothowto
	 * d4 = size of loaded kernel
	 */
	movl	#8,%a5			| Addresses 0-8 are mapped to ROM on the
	addql	#8,%a0			|  atari ST. We cannot set these.
	subl	#8,%d4

	/*
	 * Copy until end of kernel relocation code.
	 */
Lstart0:
	movl	%a0@+,%a5@+
	subl	#4, %d4
	cmpl	#Lstart3,%a5
	jle	Lstart0
	/*
	 * Enter kernel at destination address and continue copy
	 * The 'start2' label is used to defeat the assembler optimizer. If
	 * we jump to a local label, it tries to use a pc-relative jump.
	 * Which is definitely not what is needed at this point!
	 */
	jmp	_ASM_LABEL(start2)
GLOBAL(start2)
Lstart2:
	movl	%a0@+,%a5@+		| copy the rest of the kernel
	subl	#4, %d4
	jcc	Lstart2
Lstart3:

	lea	_ASM_LABEL(tmpstk),%sp	| give ourselves a temporary stack

	/*
	 *  save the passed parameters. `prepass' them on the stack for
	 *  later catch by _start_c
	 */
	movl	%a1,%sp@-		| pass address of _esym
	movl	%d1,%sp@-		| pass stmem-size
	movl	%d0,%sp@-		| pass fastmem-size
	movl	%d5,%sp@-		| pass fastmem_start
	movl	%d2,%sp@-		| pass machine id
	movl	%d3,_C_LABEL(boothowto)	| save reboot flags


	/*
	 * Set cputype and mmutype dependent on the machine-id passed
	 * in from the loader. Also make sure that all caches are cleared.
	 */
	movl	#ATARI_68030,%d1		| 68030 type from loader
	andl	%d2,%d1
	jeq	Ltestfor020			| Not an 68030, try 68020
	movl	#MMU_68030,_C_LABEL(mmutype)	| Use 68030 MMU
	movl	#CPU_68030,_C_LABEL(cputype)	|   and a 68030 CPU
	movl	#CACHE_OFF,%d0			| 68020/030 cache clear
	jra	Lend_cpuset			| skip to init.
Ltestfor020:
	movl	#ATARI_68020,%d1		| 68020 type from loader
	andl	%d2,%d1
	jeq	Ltestfor040
	movl	#MMU_68851,_C_LABEL(mmutype)	| Assume 68851 with 68020
	movl	#CPU_68020,_C_LABEL(cputype)	|   and a 68020 CPU
	movl	#CACHE_OFF,%d0			| 68020/030 cache clear
	jra	Lend_cpuset			| skip to init.
Ltestfor040:
	movl	#CACHE_OFF,%d0			| 68020/030 cache
	movl	#ATARI_68040,%d1
	andl	%d2,%d1
	jeq	Ltestfor060
	movl	#MMU_68040,_C_LABEL(mmutype)	| Use a 68040 MMU
	movl	#CPU_68040,_C_LABEL(cputype)	|   and a 68040 CPU
	.word	0xf4f8				| cpusha bc - push&inval caches
	movl	#CACHE40_OFF,%d0		| 68040 cache disable
	jra	Lend_cpuset			| skip to init.
Ltestfor060:
	movl    #ATARI_68060,%d1
	andl	%d2,%d1
	jeq	Lend_cpuset
	movl	#MMU_68040,_C_LABEL(mmutype)	| Use a 68040 MMU
	movl	#CPU_68060,_C_LABEL(cputype)	|   and a 68060 CPU
	.word	0xf4f8				| cpusha bc - push&inval caches
	movl	#CACHE40_OFF,%d0		| 68040 cache disable
	orl	#IC60_CABC,%d0			|   and clear  060 branch cache

Lend_cpuset:
	movc	%d0,%cacr		| clear and disable on-chip cache(s)
	movl	#_C_LABEL(vectab),%a0	| set address of vector table
	movc	%a0,%vbr

	/*
	 * Initialize source/destination control registers for movs
	 */
	moveq	#FC_USERD,%d0		| user space
	movc	%d0,%sfc		|   as source
	movc	%d0,%dfc		|   and destination of transfers

	/*
	 * let the C function initialize everything and enable the MMU
	 */
	jsr	_C_LABEL(start_c)

	/*
	 * set kernel stack, user SP, and initial pcb
	 */
	movl	_C_LABEL(proc0paddr),%a1| proc0 kernel stack
	lea	%a1@(USPACE),%sp	| set kernel stack to end of area
	movl	#USRSTACK-4,%a2
	movl	%a2,%usp		| init user SP
	movl	%a2,%a1@(PCB_USP)	| and save it
	movl	%a1,_C_LABEL(curpcb)	| proc0 is running
	clrw	%a1@(PCB_FLAGS)		| clear flags

	/* flush TLB and turn on caches */
	jbsr	_C_LABEL(TBIA)		|  invalidate TLB
	movl	#CACHE_ON,%d0
	cmpl	#MMU_68040,_C_LABEL(mmutype)
	jne	Lcacheon
	/*  is this needed? MLH */
	.word	0xf4f8			|  cpusha bc - push & invalidate caches
	movl	#CACHE40_ON,%d0
#ifdef M68060
	cmpl	#CPU_68060,_C_LABEL(cputype)
	jne	Lcacheon
	movl	#CACHE60_ON,%d0
#endif
Lcacheon:
	movc	%d0,%cacr		|  clear cache(s)

	/*
	 * Final setup for C code
	 */
	movw	#PSL_LOWIPL,%sr		|  lower SPL

#ifdef notdef
	movl	%d6,_C_LABEL(bootdev)	|    and boot device
#endif

	/*
	 * Create a fake exception frame that returns to user mode,
	 * make space for the rest of a fake saved register set, and
	 * pass a pointer to the register set to "main()".
	 * "main()" will call "icode()", which fakes
	 * an "execve()" system call, which is why we need to do that
	 * ("main()" sets "u.u_ar0" to point to the register set).
	 * When "main()" returns, we're running in process 1 and have
	 * successfully faked the "execve()".  We load up the registers from
	 * that set; the "rte" loads the PC and PSR, which jumps to "init".
 	 */
	movl	#0,%a6			|  make DDB stack_trace() work
  	clrw	%sp@-			|  vector offset/frame type
	clrl	%sp@-			|  PC - filled in by "execve"
  	movw	#PSL_USER,%sp@-		|  in user mode
	clrl	%sp@-			|  stack adjust count
	lea	%sp@(-64),%sp		|  construct space for D0-D7/A0-A7
	lea	_C_LABEL(proc0),%a0	| proc0 in a0
	movl	%sp,%a0@(P_MD + MD_REGS)| save frame for proc0
	movl	%usp,%a1
	movl	%a1,%sp@(FR_SP)		| save user stack pointer in frame
	pea	%sp@			|  addr of space for D0
	jbsr	_C_LABEL(main)		|  main(r0)
	addql	#4,%sp			|  pop args
	cmpl	#MMU_68040,_C_LABEL(mmutype)
	jne	Lnoflush		|  Not an 68040, skip flush
	.word	0xf478			|  cpusha dc
	.word	0xf498			|  cinva ic
Lnoflush:
	movl	%sp@(FR_SP),%a0		|  grab and load
	movl	%a0,%usp		|    user SP
	moveml	%sp@+,#0x7FFF		|  load most registers (all but SSP)
	addql	#8,%sp			|  pop SSP and stack adjust count
  	rte

/*
 * proc_trampoline call function in register a2 with a3 as an arg
 * and then rei.
 */
ENTRY_NOPROFILE(proc_trampoline)
	movl	%a3,%sp@-		| push function arg
	jbsr	%a2@			| call function
	addql	#4,%sp			| pop arg
	movl	%sp@(FR_SP),%a0		| usp to a0
	movl	%a0,%usp		| setup user stack pointer
	moveml	%sp@+,#0x7FFF		| restore all but sp
	addql	#8,%sp			| pop sp and stack adjust
	jra	_ASM_LABEL(rei)		| all done

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
 * update profiling information for the user
 * addupc(pc, &u.u_prof, ticks)
 */
ENTRY(addupc)
	movl	%a2,%sp@-		|  scratch register
	movl	%sp@(12),%a2		|  get &u.u_prof
	movl	%sp@(8),%d0		|  get user pc
	subl	%a2@(8),%d0		|  pc -= pr->pr_off
	jlt	Lauexit			|  less than 0, skip it
	movl	%a2@(12),%d1		|  get pr->pr_scale
	lsrl	#1,%d0			|  pc /= 2
	lsrl	#1,%d1			|  scale /= 2
	mulul	%d1,%d0			|  pc /= scale
	moveq	#14,%d1
	lsrl	%d1,%d0			|  pc >>= 14
	bclr	#0,%d0			|  pc &= ~1
	cmpl	%a2@(4),%d0		|  too big for buffer?
	jge	Lauexit			|  yes, screw it
	addl	%a2@,%d0		|  no, add base
	movl	%d0,%sp@-		|  push address
	jbsr	_C_LABEL(fusword)	|  grab old value
	movl	%sp@+,%a0		|  grab address back
	cmpl	#-1,%d0			|  access ok
	jeq	Lauerror		|  no, skip out
	addw	%sp@(18),%d0		|  add tick to current value
	movl	%d0,%sp@-		|  push value
	movl	%a0,%sp@-		|  push address
	jbsr	_C_LABEL(susword)	|  write back new value
	addql	#8,%sp			|  pop params
	tstl	%d0			|  fault?
	jeq	Lauexit			|  no, all done
Lauerror:
	clrl	%a2@(12)		|  clear scale (turn off prof)
Lauexit:
	movl	%sp@+,%a2		|  restore scratch reg
	rts

/*
 * non-local gotos
 */
ENTRY(qsetjmp)
	movl	%sp@(4),%a0		|  savearea pointer
	lea	%a0@(40),%a0		|  skip regs we do not save
	movl	%a6,%a0@+		|  save FP
	movl	%sp,%a0@+		|  save SP
	movl	%sp@,%a0@		|  and return address
	moveq	#0,%d0			|  return 0
	rts

	BSS(want_resched,4)

/*
 * Use common m68k process manipulation routines.
 */
#include <m68k/m68k/proc_subr.s>

Lsw0:
	.asciz	"cpu_switch"
	.even

	.data
GLOBAL(masterpaddr)			|  XXX compatibility (debuggers)
GLOBAL(curpcb)
	.long	0
Lpcbflag:
	.byte	0			|  copy of pcb_flags low byte
	.align	2
	ASBSS(nullpcb,SIZEOF_PCB)
	.text

/*
 * At exit of a process, do a switch for the last time.
 * Switch to a safe stack and PCB, and select a new process to run.  The
 * old stack and u-area will be freed by the reaper.
 */
ENTRY(switch_exit)
	movl	%sp@(4),%a0
	movl	#nullpcb,_C_LABEL(curpcb)	| save state into garbage pcb
	lea	tmpstk,%sp			| goto a tmp stack

	/* Schedule the vmspace and stack to be freed. */
	movl	%a0,%sp@-			| exit2(p)
	jbsr	_C_LABEL(exit2)
	lea	%sp@(4),%sp			| pop args

#if defined(LOCKDEBUG)
	/* Acquire sched_lock */
	jbsr	_C_LABEL(sched_lock_idle)
#endif

	jra	_C_LABEL(cpu_switch)

/*
 * When no processes are on the runq, Swtch branches to idle
 * to wait for something to come ready.
 */
ASENTRY_NOPROFILE(Idle)
#if defined(LOCKDEBUG)
	/* Release sched_lock */
	jbsr	_C_LABEL(sched_unlock_idle)
#endif
	stop	#PSL_LOWIPL
	movw	#PSL_HIGHIPL,%sr
#if defined(LOCKDEBUG)
	/* Acquire sched_lock */
	jbsr	_C_LABEL(sched_lock_idle)
#endif
	movl	_C_LABEL(sched_whichqs),%d0
	jeq	_ASM_LABEL(Idle)
	jra	Lsw1

Lbadsw:
	movl	#Lsw0,%sp@-
	jbsr	_C_LABEL(panic)
	/*NOTREACHED*/

/*
 * Cpu_switch()
 *
 * NOTE: On the mc68851 (318/319/330) we attempt to avoid flushing the
 * entire ATC.  The effort involved in selective flushing may not be
 * worth it, maybe we should just flush the whole thing?
 *
 * NOTE 2: With the new VM layout we now no longer know if an inactive
 * user's PTEs have been changed (formerly denoted by the SPTECHG p_flag
 * bit).  For now, we just always flush the full ATC.
 */
ENTRY(cpu_switch)
	movl	_C_LABEL(curpcb),%a0	|  current pcb
	movw	%sr,%a0@(PCB_PS)	|  save sr before changing ipl
#ifdef notyet
	movl	_C_LABEL(curproc),%sp@-	|  remember last proc running
#endif
	clrl	_C_LABEL(curproc)

	/*
	 * Find the highest-priority queue that isn't empty,
	 * then take the first proc from that queue.
	 */
	movl	_C_LABEL(sched_whichqs),%d0
	jeq	_ASM_LABEL(Idle)
Lsw1:
	/*
	 * Interrupts are blocked, sched_lock is held.  If
	 * we come here via Idle, %d0 contains the contents
	 * of a non-zero sched_whichqs.
	 */
	movl	%d0,%d1
	negl	%d0
	andl	%d1,%d0
	bfffo	%d0{#0:#32},%d1
	eorib	#31,%d1

	movl	%d1,%d0
	lslb	#3,%d1			| convert queue number to index
	addl	#_C_LABEL(sched_qs),%d1	| locate queue (q)
	movl	%d1,%a1
	movl	%a1@(P_FORW),%a0	| p = q->p_forw
	cmpal	%d1,%a0			| anyone on queue?
	jeq	Lbadsw			| no, panic
#ifdef DIAGNOSTIC
	tstl	%a0@(P_WCHAN)
	jne	Lbadsw
	cmpb	#SRUN,%a0@(P_STAT)
	jne	Lbadsw
#endif
	movl	%a0@(P_FORW),%a1@(P_FORW)	| q->p_forw = p->p_forw
	movl	%a0@(P_FORW),%a1		| n = p->p_forw
	movl	%a0@(P_BACK),%a1@(P_BACK)	| n->p_back = q
	cmpal	%d1,%a1			| anyone left on queue?
	jne	Lsw2			| yes, skip
	movl	_C_LABEL(sched_whichqs),%d1
	bclr	%d0,%d1			| no, clear bit
	movl	%d1,_C_LABEL(sched_whichqs)
Lsw2:
	/* p->p_cpu initialized in fork1() for single-processor */
	movb	#SONPROC,%a0@(P_STAT)	| p->p_stat = SONPROC
	movl	%a0,_C_LABEL(curproc)
	clrl	_C_LABEL(want_resched)
#ifdef notyet
	movl	%sp@+,%a1
	cmpl	%a0,%a1			| switching to same proc?
	jeq	Lswdone			| yes, skip save and restore
#endif

	/*
	 * Save state of previous process in its pcb.
	 */
	movl	_C_LABEL(curpcb),%a1
	moveml	#0xFCFC,%a1@(PCB_REGS)	| save non-scratch registers
	movl	%usp,%a2		| grab USP (a2 has been saved)
	movl	%a2,%a1@(PCB_USP)	| and save it
	movl	_C_LABEL(CMAP2),%a1@(PCB_CMAP2)	| save temporary map PTE
	tstl	_C_LABEL(fputype)	| do we have an FPU?
	jeq	Lswnofpsave		| no? don't attempt to save
	lea	%a1@(PCB_FPCTX),%a2	| pointer to FP save area
	fsave	%a2@			| save FP state
#ifdef M68060
	cmpl	#CPU_68060,_C_LABEL(cputype)
	jeq	Lsavfp60		| It's a 060
#endif
	tstb	%a2@			| null state frame?
	jeq	Lswnofpsave		| yes, all done
	fmovem	%fp0-%fp7,%a2@(FPF_REGS) | save FP general registers
	fmovem	%fpcr/%fpsr/%fpi,%a2@(FPF_FPCR)	| save FP control registers
#ifdef M68060
	jra	Lswnofpsave
Lsavfp60:
	tstb	%a2@(2)			| null state frame?
	jeq	Lswnofpsave		| yes, all done
	fmovem	%fp0-%fp7,%a2@(FPF_REGS) | save FP general registers
	fmovem	%fpcr,%a2@(FPF_FPCR)	| save FP control registers
	fmovem  %fpsr,%a2@(FPF_FPSR)
	fmovem  %fpi,%a2@(FPF_FPI)
#endif
Lswnofpsave:

	clrl	%a0@(P_BACK)			| clear back link
	movl	%a0@(P_ADDR),%a1		| get p_addr
	movl	%a1,_C_LABEL(curpcb)
	movb	%a1@(PCB_FLAGS+1),Lpcbflag	| copy of pcb_flags low byte

#if defined(LOCKDEBUG)
	/*
	 * Done mucking with the run queues, release the
	 * scheduler lock, but keep interrupts out.
	 */
	movl	%a0,sp@-		| not args...
	movl	%a1,sp@-		| ...just saving
	jbsr	_C_LABEL(sched_unlock_idle)
	movl	sp@+,%a1
	movl	sp@+,%a0
#endif

	/*
	 * Activate process's address space.
	 * XXX Should remember the last USTP value loaded, and call this
	 * XXX only if it has changed.
	 */
	pea	%a0@			| push proc
	jbsr	_C_LABEL(pmap_activate)	| pmap_activate(p) 
	addql	#4,%sp
	movl	_C_LABEL(curpcb),%a1	| restore p_addr 

	lea	_ASM_LABEL(tmpstk),%sp	| now goto a tmp stack for NMI 

	movl	%a1@(PCB_CMAP2),_C_LABEL(CMAP2)	| reload tmp map
	moveml	%a1@(PCB_REGS),#0xFCFC	| and registers
	movl	%a1@(PCB_USP),%a0
	movl	%a0,%usp		| and USP
	tstl	_C_LABEL(fputype)	| do we have an FPU?
	jeq	Lnofprest		| no, don't attempt to restore
	lea	%a1@(PCB_FPCTX),%a0	| pointer to FP save area
#ifdef M68060
	cmpl	#CPU_68060,_C_LABEL(cputype)
	jeq	Lresfp60rest1		| handle a 060
#endif
	tstb	%a0@			| null state frame?
	jeq	Lresfprest		| yes, easy
	fmovem	%a0@(FPF_FPCR),%fpcr/%fpsr/%fpi	| restore FP control registers
	fmovem	%a0@(FPF_REGS),%fp0-%fp7 | restore FP general registers
Lresfprest:
	frestore %a0@			| restore state

Lnofprest:
	movw	%a1@(PCB_PS),%sr	| no, restore PS
	moveq	#1,%d0			| return 1 (for alternate returns)
	rts

#ifdef M68060
Lresfp60rest1:
	tstb	%a0@(2)			| null state frame?
	jeq	Lresfp60rest2		| yes, easy
	fmovem	%a0@(FPF_FPCR),%fpcr	| restore FP control registers
	fmovem	%a0@(FPF_FPSR),%fpsr
	fmovem	%a0@(FPF_FPI),%fpi
	fmovem	%a0@(FPF_REGS),%fp0-%fp7 | restore FP general registers
Lresfp60rest2:
	frestore %a0@			| restore state
	movw	%a1@(PCB_PS),%sr	| no, restore PS
	moveq	#1,%d0			| return 1 (for alternate returns)
	rts
#endif

/*
 * savectx(pcb)
 * Update pcb, saving current processor state
 */
ENTRY(savectx)
	movl	%sp@(4),%a1
	movw	%sr,%a1@(PCB_PS)
	movl	%usp,%a0		| grab USP 
	movl	%a0,%a1@(PCB_USP)	| and save it 
	moveml	#0xFCFC,%a1@(PCB_REGS)	| save non-scratch registers 
	movl	_C_LABEL(CMAP2),%a1@(PCB_CMAP2)	| save temporary map PTE 
	tstl	_C_LABEL(fputype)	| do we have an FPU?
	jeq	Lsavedone		| no, don't attempt to save
	lea	%a1@(PCB_FPCTX),%a0	| pointer to FP save area 
	fsave	%a0@			| save FP state 
#ifdef M68060
	cmpl	#CPU_68060,_C_LABEL(cputype)
	jeq	Lsavctx60		| handle a 060
#endif
	tstb	%a0@			| null state frame? 
	jeq	Lsavedone		| yes, all done 
	fmovem	%fp0-%fp7,%a0@(FPF_REGS)	| save FP general registers 
	fmovem	%fpcr/%fpsr/%fpi,%a0@(FPF_FPCR) | save FP control registers 
Lsavedone:
	moveq	#0,%d0			| return 0 
	rts

#ifdef M68060
Lsavctx60:
	tstb	%a0@(2)
	jeq	Lsavedone
	fmovem	%fp0-%fp7,%a0@(FPF_REGS)| save FP general registers
	fmovem	%fpcr,%a0@(FPF_FPCR)	| save FP control registers
	fmovem	%fpsr,%a0@(FPF_FPSR)
	fmovem	%fpi,%a0@(FPF_FPI)
	moveq	#0,%d0			| return 0
	rts
#endif

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
#endif /* defined(M68040) */

/*
 * Copy 1 relocation unit (NBPG bytes)
 * from user virtual address to physical address
 */
ENTRY(copyseg)
	movl	_C_LABEL(curpcb),%a1	|  current pcb 
	movl	#Lcpydone,%a1@(PCB_ONFAULT) |  where to return to on a fault 
	movl	%sp@(8),%d0		|  destination page number 
	moveq	#PGSHIFT,%d1
	lsll	%d1,%d0			|  convert to address 
	orl	#PG_CI+PG_RW+PG_V,%d0	|  make sure valid and writable 
	movl	_C_LABEL(CMAP2),%a0
	movl	_C_LABEL(CADDR2),%sp@-	|  destination kernel VA 
	movl	%d0,%a0@		|  load in page table 
	jbsr	_C_LABEL(TBIS)		|  invalidate any old mapping 
	addql	#4,%sp
	movl	_C_LABEL(CADDR2),%a1	|  destination addr 
	movl	%sp@(4),%a0		|  source addr 
	movl	#NBPG/4-1,%d0		|  count 
Lcpyloop:
	movsl	%a0@+,%d1		|  read longword 
	movl	%d1,%a1@+		|  write longword
	dbf	%d0,Lcpyloop		|  continue until done
Lcpydone:
	movl	_C_LABEL(curpcb),%a1	|  current pcb
	clrl	%a1@(PCB_ONFAULT)	|  clear error catch
	rts

/*
 * Invalidate entire TLB.
 */
ENTRY(TBIA)
Ltbia:
	cmpl	#MMU_68040,_C_LABEL(mmutype)
	jeq	Ltbia040
	pflusha				|  flush entire TLB
	tstl	_C_LABEL(mmutype)
	jpl	Lmc68851a		|  68851 implies no d-cache
	movl	#DC_CLEAR,%d0
	movc	%d0,%cacr		|  invalidate on-chip d-cache
Lmc68851a:
	rts
Ltbia040:
	.word	0xf518			|  pflusha
#ifdef M68060
	cmpl	#CPU_68060,_C_LABEL(cputype)
	jne	Ltbiano60		| No 060 -> no branch cache
	movc	%cacr,%d0
	orl	#IC60_CABC,%d0		| and clear all branch cache entries
	movc	%d0,%cacr
Ltbiano60:
#endif
	rts

/*
 * Invalidate any TLB entry for given VA (TB Invalidate Single)
 */
ENTRY(TBIS)
#ifdef DEBUG
	tstl	fulltflush		|  being conservative?
	jne	Ltbia			|  yes, flush entire TLB
#endif
	movl	%sp@(4),%a0		|  get addr to flush
	cmpl	#MMU_68040,_C_LABEL(mmutype)
	jeq	Ltbis040
	tstl	_C_LABEL(mmutype)
	jpl	Lmc68851b		|  is 68851?
	pflush	#0,#0,%a0@		|  flush address from both sides
	movl	#DC_CLEAR,%d0
	movc	%d0,%cacr		|  invalidate on-chip data cache
	rts
Lmc68851b:
	pflushs	#0,#0,%a0@		|  flush address from both sides
	rts
Ltbis040:
	moveq	#FC_SUPERD,%d0		|  select supervisor
	movc	%d0,%dfc
	.word	0xf508			|  pflush a0@
	moveq	#FC_USERD,%d0		|  select user
	movc	%d0,%dfc
	.word	0xf508			|  pflush a0@
#if defined(M68060)
	cmpl	#CPU_68060,_C_LABEL(cputype)
	jne	Ltbisnot060		|   no 060, no branch cache
	movc	%cacr,%d0
	orl	#IC60_CABC,%d0		| and clear all branch cache entries
	movc	%d0,%cacr
Ltbisnot060:
#endif /* defined(M68060) */

	rts

ENTRY(ecacheon)
	rts

ENTRY(ecacheoff)
	rts

/*
 * Get callers current SP value.
 * Note that simply taking the address of a local variable in a C function
 * doesn't work because callee saved registers may be outside the stack frame
 * defined by A6 (e.g. GCC generated code).
 */
ENTRY_NOPROFILE(getsp)
	movl	%sp,%d0			|  get current SP
	addql	#4,%d0			|  compensate for return address
	rts

ENTRY_NOPROFILE(getsfc)
	movc	%sfc,%d0
	rts
ENTRY_NOPROFILE(getdfc)
	movc	%dfc,%d0
	rts

/*
 * Check out a virtual address to see if it's okay to write to.
 *
 * probeva(va, fc)
 *
 */
ENTRY(probeva)
	movl	%sp@(8),%d0
	movec	%d0,%dfc
	movl	%sp@(4),%a0
	.word	0xf548			|  ptestw (a0)
	moveq	#FC_USERD,%d0		|  restore DFC to user space
	movc	%d0,%dfc
	.word	0x4e7a,0x0805		|  movec  MMUSR,d0
	rts

/*
 * Load a new user segment table pointer.
 */
ENTRY(loadustp)
	movl	%sp@(4),%d0			| new USTP
	moveq	#PGSHIFT,%d1
	lsll	%d1,%d0				| convert to addr
#if defined(M68060)
	cmpl	#CPU_68060,_C_LABEL(cputype)	| 68060?
	jeq	Lldustp060			|  yes, skip
#endif
	cmpl	#MMU_68040,_C_LABEL(mmutype)	| 68040?
	jeq	Lldustp040			|  yes, skip
	pflusha					| flush entire TLB
	lea	_C_LABEL(protorp),%a0		| CRP prototype
	movl	%d0,%a0@(4)			| stash USTP
	pmove	%a0@,%crp			| load root pointer
	movl	#CACHE_CLR,%d0
	movc	%d0,%cacr			| invalidate on-chip d-cache
	rts
#if defined(M68060)
Lldustp060:
	movc	%cacr,%d1
	orl	#IC60_CUBC,%d1		| clear user branch cache entries
	movc	%d1,%cacr
#endif
Lldustp040:
	.word	0xf518			| pflusha
	.word	0x4e7b,0x0806		| movec d0,URP
	rts

/*
 * Flush any hardware context associated with given USTP.
 * Only does something for HP330 where we must flush RPT
 * and ATC entries in PMMU.
 */
ENTRY(flushustp)
#if defined(M68060)
	cmpl	#CPU_68060,_C_LABEL(cputype)
	jeq	Lflustp060		|  A 060 needs special treatment
#endif
	cmpl	#MMU_68040,_C_LABEL(mmutype)
	jeq	Lnot68851
	tstl	_C_LABEL(mmutype)	|  68851 PMMU?
	jle	Lnot68851		|  no, nothing to do
	movl	%sp@(4),%d0		|  get USTP to flush
	moveq	#PGSHIFT,%d1
	lsll	%d1,%d0			|  convert to address
	movl	%d0,_C_LABEL(protorp)+4	|  stash USTP
	pflushr	_C_LABEL(protorp)	|  flush RPT/TLB entries
Lnot68851:
	rts
#if defined(M68060)
Lflustp060:
	movc	%cacr,%d1
	orl	IC60_CUBC,%d1		| clear user branch cache entries
	movc	%d1,%cacr
	rts
#endif

ENTRY(ploadw)
	movl	%sp@(4),%a0		|  address to load
	cmpl	#MMU_68040,_C_LABEL(mmutype)
	jeq	Lploadw040
	ploadw	#1,%a0@			|  pre-load translation
Lploadw040:				|  should 68040 do a ptest?
	rts

/*
 * Set processor priority level calls.  Most are implemented with
 * inline asm expansions.  However, spl0 requires special handling
 * as we need to check for our emulated software interrupts.
 */

ENTRY(spl0)
	moveq	#0,%d0
	movw	%sr,%d0			|  get old SR for return
	movw	#PSL_LOWIPL,%sr		|  restore new SR
	tstb	_C_LABEL(ssir)		|  software interrupt pending?
	jeq	Lspldone		|  no, all done
	subql	#4,%sp			|  make room for RTE frame
	movl	%sp@(4),%sp@(2)		|  position return address
	clrw	%sp@(6)			|  set frame type 0
	movw	#PSL_LOWIPL,%sp@	|  and new SR
	jra	Lgotsir			|  go handle it
Lspldone:
	rts

/*
 * Save and restore 68881 state.
 * Pretty awful looking since our assembler does not
 * recognize FP mnemonics.
 */
ENTRY(m68881_save)
	movl	%sp@(4),%a0		| save area pointer
	fsave	%a0@			| save state
#if defined(M68060)
	cmpl	#CPU_68060,_C_LABEL(cputype)
	jeq	Lm68060fpsave		| Handle 060 fpu
#endif
	tstb	%a0@			| null state frame?
	jeq	Lm68881sdone		| yes, all done
	fmovem	%fp0-%fp7,%a0@(FPF_REGS)	| save FP general registers
	fmovem	%fpcr/%fpsr/%fpi,%a0@(FPF_FPCR) | save FP control registers
Lm68881sdone:
	rts

#if defined(M68060)
Lm68060fpsave:
	tstb	%a0@(2)			| null state frame?
	jeq	Lm68060sdone		| yes, all done
	fmovem	%fp0-%fp7,%a0@(FPF_REGS)| save FP general registers
	fmovem	%fpcr,%a0@(FPF_FPCR)	| save FP control registers
	fmovem	%fpsr,%a0@(FPF_FPSR)
	fmovem	%fpi,%a0@(FPF_FPI)
Lm68060sdone:
	rts
#endif

ENTRY(m68881_restore)
	movl	%sp@(4),%a0			| save area pointer
#if defined(M68060)
	cmpl	#CPU_68060,_C_LABEL(cputype)
	jeq	Lm68060fprestore		| a 060 fpu
#endif
	tstb	%a0@				| null state frame?
	jeq	Lm68881rdone			| yes, easy
	fmovem	%a0@(FPF_FPCR),%fpcr/%fpsr/%fpi | restore FP control registers
	fmovem	%a0@(FPF_REGS),%fp0-%fp7	| restore FP general registers
Lm68881rdone:
	frestore %a0@				| restore state
	rts

#if defined(M68060)
Lm68060fprestore:
	tstb	%a0@(2)				| null state frame?
	jeq	Lm68060fprdone			| yes, easy
	fmovem	%a0@(FPF_FPCR),%fpcr		| restore FP control registers
	fmovem	%a0@(FPF_FPSR),%fpsr
	fmovem	%a0@(FPF_FPI),%fpi
	fmovem	%a0@(FPF_REGS),%fp0-%fp7	| restore FP general registers
Lm68060fprdone:
	frestore %a0@				| restore state
	rts
#endif

/*
 * Handle the nitty-gritty of rebooting the machine.
 *
 */
ENTRY_NOPROFILE(doboot)
	movl	#CACHE_OFF,%d0
	cmpl	#MMU_68040,_C_LABEL(mmutype) |  is it 68040?
	jne	Ldoboot0
	.word	0xf4f8			|  cpusha bc - push and inval caches
	nop
	movl	#CACHE40_OFF,%d0
Ldoboot0:
	movc	%d0,%cacr		|  disable on-chip cache(s)

	movw	#0x2700,%sr		|  cut off any interrupts

	/*
	 * Clear first 2k of ST-memory. We start clearing at address 0x8
	 * because the lower 8 bytes are mapped to ROM.
	 * This makes sure that the machine will 'cold-boot'.
	 */
	movl	_C_LABEL(page_zero),%a0
	addl	#0x8,%a0
	movl	#512,%d0
Ldb1:
	clrl	%a0@+
	dbra	%d0,Ldb1
	
	lea	Ldoreboot,%a1		| a1 = start of copy range
	lea	Ldorebootend,%a2	| a2 = end of copy range
	movl	_C_LABEL(page_zero),%a0	| a0 = virtual base for page zero
	addl	%a1,%a0			|		+ offset of Ldoreboot
Ldb2:					| Do the copy
	movl	%a1@+,%a0@+
	cmpl	%a2,%a1
	jle	Ldb2

	/*
	 * Ok, turn off MMU..
	 */
Ldoreboot:
	cmpl	#MMU_68040,_C_LABEL(mmutype)
	jeq	Lmmuoff040		| Go turn off 68040 MMU
	lea	_ASM_LABEL(zero),%a0
	pmove	%a0@,%tc		| Turn off MMU
	lea	_ASM_LABEL(nullrp),%a0
	pmove	%a0@,%crp		| Invalidate Cpu root pointer
	pmove	%a0@,%srp		|  and the Supervisor root pointer
	jra	Ldoboot1		| Ok, continue with actual reboot
Lmmuoff040:
	movl	#0,%d0
	.word	0x4e7b,0x0003		|  movc d0,TC
	.word	0x4e7b,0x0806		|  movc d0,URP
	.word	0x4e7b,0x0807		|  movc d0,SRP

Ldoboot1:
	movl	#0, %a0
	movc	%a0,%vbr
	movl	%a0@(4), %a0		| fetch reset-vector
	jmp	%a0@			| jump through it
	/* NOTREACHED */

/*  A do-nothing MMU root pointer (includes the following long as well) */

ASLOCAL(nullrp)
	.long	0x7fff0001
ASLOCAL(zero)
	.long	0
Ldorebootend:

	.data
	.space	NBPG
ASLOCAL(tmpstk)
GLOBAL(protorp)
	.long	0x80000002,0		|  prototype root pointer

GLOBAL(proc0paddr)
	.long	0			|  KVA of proc0 u-area
#ifdef M68060 /* XXX */
L60iem:		.long	0
L60fpiem:	.long	0
L60fpdem:	.long	0
L60fpeaem:	.long	0
L60bpe:		.long	0
#endif
#ifdef DEBUG

ASLOCAL(fulltflush)
	.long	0
ASLOCAL(fullcflush)
	.long	0
GLOBAL(timebomb)
	.long	0
#endif
ASLOCAL(plx_nonmi)
	.long	0
GLOBAL(plx_status)
	.long	0

/* interrupt counters & names */
#include <atari/atari/intrcnt.h>
