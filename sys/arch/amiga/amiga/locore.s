/*	$NetBSD: locore.s,v 1.141 2007/05/18 01:39:52 mhitch Exp $	*/

/*
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
 * from: Utah $Hdr: locore.s 1.58 91/04/22$
 *
 *	@(#)locore.s	7.11 (Berkeley) 5/9/91
 *
 * Original (hp300) Author: unknown, maybe Mike Hibler?
 * Amiga author: Markus Wild
 * Other contributors: Bryan Ford (kernel reload stuff)
 */

/*
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
 * from: Utah $Hdr: locore.s 1.58 91/04/22$
 *
 *	@(#)locore.s	7.11 (Berkeley) 5/9/91
 *
 * Original (hp300) Author: unknown, maybe Mike Hibler?
 * Amiga author: Markus Wild
 * Other contributors: Bryan Ford (kernel reload stuff)
 */

#include "opt_fpu_emulate.h"
#include "opt_bb060stupidrom.h"
#include "opt_p5ppc68kboard.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_svr4.h"
#include "opt_compat_sunos.h"
#include "opt_fpsp.h"
#include "opt_kgdb.h"
#include "opt_lockdebug.h"

#include "opt_lev6_defer.h"

#include "assym.h"
#include <machine/asm.h>
#include <machine/trap.h>

	.text
GLOBAL(kernel_text)
L_base:
	.long	0x4ef80400+PAGE_SIZE	/* jmp jmp0.w */
	.fill	PAGE_SIZE/4-1,4,0/*xdeadbeef*/

#include <amiga/amiga/vectors.s>
#include <amiga/amiga/custom.h>

#ifdef DRACO
#include <amiga/amiga/drcustom.h>
#endif

#define CIAAADDR(ar)	movl	_C_LABEL(CIAAbase),ar
#define CIABADDR(ar)	movl	_C_LABEL(CIABbase),ar
#define CUSTOMADDR(ar)	movl	_C_LABEL(CUSTOMbase),ar
#define INTREQRADDR(ar)	movl	_C_LABEL(INTREQRaddr),ar
#define INTREQWADDR(ar)	movl	_C_LABEL(INTREQWaddr),ar
#define INTENAWADDR(ar) movl	_C_LABEL(amiga_intena_write),ar
#define	INTENARADDR(ar)	movl	_C_LABEL(amiga_intena_read),ar

	.text
/*
 * This is where we wind up if the kernel jumps to location 0.
 * (i.e. a bogus PC)  This is known to immediately follow the vector
 * table and is hence at 0x400 (see reset vector in vectors.s).
 */
	pea	Ljmp0panic
	jbsr	_C_LABEL(panic)
	/* NOTREACHED */
Ljmp0panic:
	.asciz	"kernel jump to zero"
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
	moveml	%d0-%d7/%a0-%a7,%sp@-	| save user registers
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
	moveml	%d0-%d7/%a0-%a7,%sp@-	| save user registers
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
#endif
#if defined(M68040)
ENTRY_NOPROFILE(buserr40)
	clrl	%sp@-			| stack adjust count
	moveml	%d0-%d7/%a0-%a7,%sp@-	| save user registers
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
#endif

ENTRY_NOPROFILE(buserr)
ENTRY_NOPROFILE(addrerr)
#if !(defined(M68020) || defined(M68030))
	jra	_C_LABEL(badtrap)
#else
	clrl	%sp@-			| stack adjust count
	moveml	%d0-%d7/%a0-%a7,%sp@-	| save user registers
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
#endif
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
 * FP exceptions.
 */
ENTRY_NOPROFILE(fpfline)
#if defined(M68040)
	cmpw	#0x202c,%sp@(6)		| format type 2?
	jne	_C_LABEL(illinst)	| no, not an FP emulation
#ifdef FPSP
	jmp	_ASM_LABEL(fpsp_unimp)	| yes, go handle it
#endif
#endif

#ifdef FPU_EMULATE
ENTRY_NOPROFILE(fpemuli)
	addql	#1,Lfpecnt
	clrl	%sp@-			| stack adjust count
	moveml	%d0-%d7/%a0-%a7,%sp@-	| save registers
	movql	#T_FPEMULI,%d0		| denote as FP emulation trap
	jra	_ASM_LABEL(fault)	| do it
#endif

ENTRY_NOPROFILE(fpunsupp)
#if defined(M68040)
	cmpl	#MMU_68040,_C_LABEL(mmutype)	| 68040?
	jne	_C_LABEL(illinst)	| no, treat as illinst
#ifdef FPSP
	jmp	_ASM_LABEL(fpsp_unsupp)	| yes, go handle it
#else
	clrl	%sp@-			| stack adjust count
	moveml	%d0-%d7/%a0-%a7,%sp@-	| save registers
	movql	#T_FPEMULD,%d0		| denote as FP emulation trap
	jra	_ASM_LABEL(fault)	| do it
#endif
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
#ifdef FPCOPROC
	clrl	%sp@-			| stack adjust count
	moveml	%d0-%d7/%a0-%a7,%sp@-	| save user registers
	movl	%usp,%a0		| and save
	movl	%a0,%sp@(FR_SP)		|   the user stack pointer
	clrl	%sp@-			| no VA arg
	movl	_C_LABEL(curpcb),%a0	| current pcb
	lea	%a0@(PCB_FPCTX),%a0	| address of FP savearea
	fsave	%a0@			| save state
#if defined(M68020) || defined(M68030)
#if defined(M68060) || defined(M68040)
	movb	_C_LABEL(machineid)+3,%d0
	andb	#0x90,%d0		| AMIGA_68060 | AMIGA_68040
	jne	Lfptnull		| XXX
#endif
	tstb	%a0@			| null state frame?
	jeq	Lfptnull		| yes, safe
	clrw	%d0			| no, need to tweak BIU
	movb	%a0@(1),%d0		| get frame size
	bset	#3,%a0@(0,%d0:w)	| set exc_pend bit of BIU
Lfptnull:
#endif
	fmovem	%fpsr,%sp@-		| push fpsr as code argument
	frestore %a0@			| restore state
	movl	#T_FPERR,%sp@-		| push type arg
	jra	_ASM_LABEL(faultstkadj) | call trap and deal with stack cleanup
#else
	jra	_C_LABEL(badtrap)	| treat as an unexpected trap
#endif

/*
 * Other exceptions only cause four and six word stack frame and require
 * no post-trap stack adjustment.
 */

ENTRY_NOPROFILE(badtrap)
	moveml	%d0/%d1/%a0/%a1,%sp@-	| save scratch regs
	movw	%sp@(22),%sp@-		| push exception vector info
	clrw	%sp@-
	movl	%sp@(22),%sp@-		| and PC
	jbsr	_C_LABEL(straytrap)	| report
	addql	#8,%sp			| pop args
	moveml	%sp@+,%d0/%d1/%a0/%a1	| restore regs
	jra	_ASM_LABEL(rei)		| all done

ENTRY_NOPROFILE(trap0)
	clrl	%sp@-			| stack adjust count
	moveml	%d0-%d7/%a0-%a7,%sp@-	| save user registers
	movl	%usp,%a0		| save the user SP
	movl	%a0,%sp@(FR_SP)		|   in the savearea
	movl	%d0,%sp@-		| push syscall number
	jbsr	_C_LABEL(syscall)	| handle it
	addql	#4,%sp			| pop syscall arg
	movl	%sp@(FR_SP),%a0		| grab and restore
	movl	%a0,%usp		|   user SP
	moveml	%sp@+,%d0-%d7/%a0-%a6	| restore most registers
	addql	#8,%sp			| pop SP and stack adjust
	jra	_ASM_LABEL(rei)		| all done

/*
 * Trap 12 is the entry point for the cachectl "syscall"
 *	cachectl(command, addr, length)
 * command in d0, addr in a1, length in d1
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
 * Trap 15 is used for:
 *	- KGDB traps
 *	- trace traps for SUN binaries (not fully supported yet)
 * We just pass it on and let trap() sort it all out
 */
ENTRY_NOPROFILE(trap15)
	clrl	%sp@-
	moveml	%d0-%d7/%a0-%a7,%sp@-
#ifdef KGDB
	moveq	#T_TRAP15,%d0
	movw	%sp@(FR_HW),%d1		| get PSW
	andw	#PSL_S,%d1		| from user mode?
	jeq	_ASM_LABEL(fault)	| yes, just a regular fault
	movl	%d0,%sp@-
	jbsr	_C_LABEL(kgdb_trap_glue) | returns if no debugger
	addl	#4,%sp
#endif
	moveq	#T_TRAP15,%d0
	jra	_ASM_LABEL(fault)

/*
 * Hit a breakpoint (trap 1 or 2) instruction.
 * Push the code and treat as a normal fault.
 */
ENTRY_NOPROFILE(trace)
	clrl	%sp@-
	moveml	%d0-%d7/%a0-%a7,%sp@-
#ifdef KGDB
	moveq	#T_TRACE,%d0
	movw	%sp@(FR_HW),%d1		| get SSW
	andw	#PSL_S,%d1		| from user mode?
	jeq	_ASM_LABEL(fault)	| no, regular fault
	movl	%d0,%sp@-
	jbsr	_C_LABEL(kgdb_trap_glue) | returns if no debugger
	addl	#4,%sp
#endif
	moveq	#T_TRACE,%d0
	jra	_ASM_LABEL(fault)

/* Use common m68k sigreturn */
#include <m68k/m68k/sigreturn.s>

/*
 * Interrupt handlers.
 *
 *	Level 0:	Spurious: ignored.
 *	Level 1:	builtin-RS232 TBE, softint (not used yet)
 *	Level 2:	keyboard (CIA-A) + DMA + SCSI
 *	Level 3:	VBL
 *	Level 4:	not used
 *	Level 5:	builtin-RS232 RBF
 *	Level 6:	Clock (CIA-B-Timers), Floppy index pulse
 *	Level 7:	Non-maskable: shouldn't be possible. ignore.
 */

/* Provide a generic interrupt dispatcher, only handle hardclock (int6)
 * and serial RBF (int5) specially, to improve performance
 */

ENTRY_NOPROFILE(spurintr)
	addql	#1,_C_LABEL(interrupt_depth)
	addql	#1,_C_LABEL(intrcnt)+0
	addql	#1,_C_LABEL(uvmexp)+UVMEXP_INTRS
	subql	#1,_C_LABEL(interrupt_depth)
	jra	_ASM_LABEL(rei)

ENTRY_NOPROFILE(lev5intr)
	addql	#1,_C_LABEL(interrupt_depth)
	moveml	%d0/%d1/%a0/%a1,%sp@-
#include "ser.h"
#if NSER > 0
	jsr	_C_LABEL(ser_fastint)
#else
	INTREQWADDR(%a0)
	movew	#INTF_RBF,%a0@		| clear RBF interrupt in intreq
#endif
	moveml	%sp@+,%d0/%d1/%a0/%a1
	addql	#1,_C_LABEL(intrcnt)+20
	addql	#1,_C_LABEL(uvmexp)+UVMEXP_INTRS
	subql	#1,_C_LABEL(interrupt_depth)
	jra	_ASM_LABEL(rei)

#ifdef DRACO
ENTRY_NOPROFILE(DraCoLev2intr)
	addql	#1,_C_LABEL(interrupt_depth)
	moveml	%d0/%d1/%a0/%a1,%sp@-

	CIAAADDR(%a0)
	movb	%a0@(CIAICR),%d0	| read irc register (clears ints!)
	jge     Ldrintrcommon		| CIAA IR not set, go through isr chain
	movel	_C_LABEL(draco_intpen),%a0
|	andib	#4,%a0@
|XXX this would better be
	bclr	#2,%a0@
	btst	#0,%d0			| timerA interrupt?
	jeq	Ldraciaend

	lea	%sp@(16),%a1		| get pointer to PS
	movl	%a1,%sp@-		| push pointer to PS, PC

	movw	#PSL_HIGHIPL,%sr	| hardclock at high IPL
	jbsr	_C_LABEL(hardclock)	| call generic clock int routine
	addql	#4,%sp			| pop params
	addql	#1,_C_LABEL(intrcnt)+32	| add another system clock interrupt

Ldraciaend:
	moveml	%sp@+,%d0/%d1/%a0/%a1
	addql	#1,_C_LABEL(uvmexp)+UVMEXP_INTRS
	subql	#1,_C_LABEL(interrupt_depth)
	jra	_ASM_LABEL(rei)

/* XXX on the DraCo rev. 4 or later, lev 1 is vectored here. */
ENTRY_NOPROFILE(DraCoLev1intr)
	addql	#1,_C_LABEL(interrupt_depth)
	moveml	%d0/%d1/%a0/%a1,%sp@-
	movl	_C_LABEL(draco_ioct),%a0
	btst	#5,%a0@(7)
	jeq	Ldrintrcommon
	btst	#4,%a0@(7)	| this only happens during autoconfiguration,
	jeq	Ldrintrcommon	| so test last.
	movw	#PSL_HIGHIPL,%sr	| run clock at high ipl
Ldrclockretry:
	lea	%sp@(16),%a1	| get pointer to PS
	movl	%a1,%sp@-	| push pointer to PS, PC
	jbsr	_C_LABEL(hardclock)
	addql	#4,%sp		| pop params
	addql	#1,_C_LABEL(intrcnt)+32	| add another system clock interrupt

	movl	_C_LABEL(draco_ioct),%a0
	tstb	%a0@(9)		| latch timer value
	movw	%a0@(11),%d0	| can't use movpw here, might be 68060
	movb	%a0@(13),%d0
	addw	_C_LABEL(amiga_clk_interval)+2,%d0
	movb	%d0,%a0@(13)	| low byte: latch write value
	movw	%d0,%a0@(11)	| ...and write it into timer
	tstw	%d0		| already positive?
	jcs	Ldrclockretry	| we lost more than one tick, call us again.

	clrb	%a0@(9)		| reset timer irq

	moveml	%sp@+,%d0/%d1/%a0/%a1
	addql	#1,_C_LABEL(uvmexp)+UVMEXP_INTRS
	subql	#1,_C_LABEL(interrupt_depth)
	jra	_ASM_LABEL(rei)	| XXXX: shouldn't we call the normal lev1?

/* XXX on the DraCo, lev 1, 3, 4, 5 and 6 are vectored here by initcpu() */
ENTRY_NOPROFILE(DraCoIntr)
	addql	#1,_C_LABEL(interrupt_depth)
	moveml  %d0/%d1/%a0/%a1,%sp@-
Ldrintrcommon:
	lea	_ASM_LABEL(Drintrcnt)-4,%a0
	movw	%sp@(22),%d0		| use vector offset
	andw	#0xfff,%d0		|   sans frame type
	addql	#1,%a0@(-0x60,%d0:w)	|     to increment apropos counter
	movw	%sr,%sp@-		| push current SR value
	clrw	%sp@-			|    padded to longword
	jbsr	_C_LABEL(intrhand)	| handle interrupt
	addql	#4,%sp			| pop SR
	moveml	%sp@+,%d0/%d1/%a0/%a1
	addql	#1,_C_LABEL(uvmexp)+UVMEXP_INTRS
	subql	#1,_C_LABEL(interrupt_depth)
	jra	_ASM_LABEL(rei)
#endif


ENTRY_NOPROFILE(lev1intr)
ENTRY_NOPROFILE(lev2intr)
ENTRY_NOPROFILE(lev3intr)
#ifndef LEV6_DEFER
ENTRY_NOPROFILE(lev4intr)
#endif
	addql	#1,_C_LABEL(interrupt_depth)
	moveml	%d0/%d1/%a0/%a1,%sp@-
Lintrcommon:
	lea	_C_LABEL(intrcnt),%a0
	movw	%sp@(22),%d0		| use vector offset
	andw	#0xfff,%d0		|   sans frame type
	addql	#1,%a0@(-0x60,%d0:w)	|     to increment apropos counter
	movw	%sr,%sp@-		| push current SR value
	clrw	%sp@-			|    padded to longword
	jbsr	_C_LABEL(intrhand)	| handle interrupt
	addql	#4,%sp			| pop SR
	moveml	%sp@+,%d0/%d1/%a0/%a1
	addql	#1,_C_LABEL(uvmexp)+UVMEXP_INTRS
	subql	#1,_C_LABEL(interrupt_depth)
	jra	_ASM_LABEL(rei)

/* XXX used to be ifndef DRACO; vector will be overwritten by initcpu() */

ENTRY_NOPROFILE(lev6intr)
#ifdef LEV6_DEFER
	/*
	 * cause a level 4 interrupt (AUD3) to occur as soon
	 * as we return. Block generation of level 6 ints until
	 * we have dealt with this one.
	 */
	addql	#1,_C_LABEL(interrupt_depth)
	moveml	%d0/%a0,%sp@-
	INTREQRADDR(%a0)
	movew	%a0@,%d0
	btst	#INTB_EXTER,%d0
	jeq	Llev6spur
	INTREQWADDR(%a0)
	movew	#INTF_SETCLR+INTF_AUD3,%a0@
	INTENAWADDR(%a0)
	movew	#INTF_EXTER,%a0@
	movew	#INTF_SETCLR+INTF_AUD3,%a0@	| make sure THIS one is ok...
	moveml	%sp@+,%d0/%a0
	subql	#1,_C_LABEL(interrupt_depth)
	rte
Llev6spur:
	addql	#1,_C_LABEL(intrcnt)+36	| count spurious level 6 interrupts
	moveml	%sp@+,%d0/%a0
	subql	#1,_C_LABEL(interrupt_depth)
	rte

ENTRY_NOPROFILE(lev4intr)
ENTRY_NOPROFILE(fake_lev6intr)
#endif
	addql	#1,_C_LABEL(interrupt_depth)
	moveml	%d0/%d1/%a0/%a1,%sp@-
#ifdef LEV6_DEFER
	/*
	 * check for fake level 6
	 */
	INTREQRADDR(%a0)
	movew	%a0@,%d0
	btst	#INTB_EXTER,%d0
	jeq	Lintrcommon		| if EXTER not pending, handle normally
#endif

	CIABADDR(%a0)
	movb	%a0@(CIAICR),%d0	| read irc register (clears ints!)
	jge	Lchkexter		| CIAB IR not set, go through isr chain
	INTREQWADDR(%a0)
#ifndef LEV6_DEFER
	movew	#INTF_EXTER,%a0@	| clear EXTER interrupt in intreq
#else
	movew	#INTF_EXTER+INTF_AUD3,%a0@ | clear EXTER & AUD3 in intreq
	INTENAWADDR(%a0)
	movew	#INTF_SETCLR+INTF_EXTER,%a0@ | reenable EXTER interrupts
#endif
	btst	#0,%d0			| timerA interrupt?
	jeq     Ltstciab4		| no
	movl	%d0,%sp@-		| push CIAB interrupt flags
	lea	%sp@(20),%a1		| get pointer to PS
	movl	%a1,%sp@-		| push pointer to PS, PC
	jbsr	_C_LABEL(hardclock)	| call generic clock int routine
	addql	#4,%sp			| pop params
	addql	#1,_C_LABEL(intrcnt)+32	| add another system clock interrupt
	movl	%sp@+,%d0		| pop interrupt flags
Ltstciab4:
#include "fd.h"
#if NFD > 0
	btst	#4,%d0			| FLG (dskindex) interrupt?
	jeq	Lskipciab		| no
	jbsr	_C_LABEL(fdidxintr)	| tell floppy driver we got it
Lskipciab:
#endif
| other ciab interrupts?
Llev6done:
	moveml	%sp@+,%d0/%d1/%a0/%a1	| restore scratch regs
	addql	#1,_C_LABEL(uvmexp)+UVMEXP_INTRS
	subql	#1,_C_LABEL(interrupt_depth)
	jra	_ASM_LABEL(rei)		| all done [can we do rte here?]
Lchkexter:
| check to see if EXTER request is really set?
	movl	_C_LABEL(isr_exter),%a0	| get head of EXTER isr chain
Lnxtexter:
	movl	%a0,%d0			| test if any more entries
	jeq	Lexterdone		| (spurious interrupt?)
	movl	%a0,%sp@-		| save isr pointer
	movl	%a0@(ISR_ARG),%sp@-
	movl	%a0@(ISR_INTR),%a0
	jsr	%a0@			| call isr handler
	addql	#4,%sp
	movl	%sp@+,%a0		| restore isr pointer
	movl	%a0@(ISR_FORW),%a0	| get next pointer
	tstl	%d0			| did handler process the int?
	jeq	Lnxtexter		| no, try next
Lexterdone:
	INTREQWADDR(%a0)
#ifndef LEV6_DEFER
	movew	#INTF_EXTER,%a0@	| clear EXTER interrupt
#else
	movew	#INTF_EXTER+INTF_AUD3,%a0@ | clear EXTER & AUD3 interrupt
	INTENAWADDR(%a0)
	movew	#INTF_SETCLR+INTF_EXTER,%a0@ | reenable EXTER interrupts
#endif
	addql	#1,_C_LABEL(intrcnt)+24	| count EXTER interrupts
	jra	Llev6done
/* XXX endifndef DRACO used to be here */

ENTRY_NOPROFILE(lev7intr)
	addql	#1,_C_LABEL(intrcnt)+28
	/*
	 * some amiga zorro2 boards seem to generate spurious NMIs. Best
	 * thing to do is to return as quick as possible. That's the
	 * reason why I do RTE here instead of jra rei.
	 */
	rte				| all done


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
ASENTRY_NOPROFILE(rei)
#ifdef DEBUG
	tstl	_C_LABEL(panicstr)	| have we paniced?
	jne	Ldorte			| yes, do not make matters worse
#endif
	tstl	_C_LABEL(astpending)	| AST pending?
	jeq	Ldorte			| no, done
Lrei1:
	btst	#5,%sp@			| yes, are we returning to user mode?
	jne	Ldorte			| no, done
	movw	#PSL_LOWIPL,%sr		| lower SPL
	clrl	%sp@-			| stack adjust
	moveml	%d0-%d7/%a0-%a7,%sp@-	| save all registers
	movl	%usp,%a1		| including
	movl	%a1,%sp@(FR_SP)		|    the users SP
	clrl	%sp@-			| VA == none
	clrl	%sp@-			| code == none
	movl	#T_ASTFLT,%sp@-		| type == async system trap
	jbsr	_C_LABEL(trap)		| go handle it
	lea	%sp@(12),%sp		| pop value args
	movl	%sp@(FR_SP),%a0		| restore user SP
	movl	%a0,%usp		|   from save area
	movw	%sp@(FR_ADJ),%d0	| need to adjust stack?
	jne	Laststkadj		| yes, go to it
	moveml	%sp@+,%d0-%d7/%a0-%a6	| no, restore most user regs
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
	moveml	%sp@+,%d0-%d7/%a0-%a6	| restore user registers
	movl	%sp@,%sp		| and our SP
Ldorte:
	rte				| real return

/*
 * Kernel access to the current processes kernel stack is via a fixed
 * virtual address.  It is at the same address as in the users VA space.
 */
BSS(esym,4)


/*
 * Initialization
 *
 * A5 contains physical load point from boot
 * exceptions vector thru our table, that's bad.. just hope nothing exceptional
 * happens till we had time to initialize ourselves..
 */
BSS(lowram,4)

#define	RELOC(var, ar)			\
	lea	_C_LABEL(var),ar;	\
	addl	%a5,ar

#define	ASRELOC(var, ar)		\
	lea	_ASM_LABEL(var),ar;	\
	addl	%a5,ar

	.text

	| XXX should be a symbol?
	| 2: needs a4 = esym
	| 3: no chipmem requirement
	|    bootinfo data structure

	.word	0
	.word	0x0003			| loadbsd version required
ASENTRY_NOPROFILE(start)
	lea	%pc@(L_base),%a5	| initialize relocation register

	movw	#PSL_HIGHIPL,%sr	| no interrupts
	ASRELOC(tmpstk,%a6)
	movl	%a6,%sp			| give ourselves a temporary stack

	| save the passed parameters. "prepass" them on the stack for
	| later catch by start_c()
	movl	%a5,%sp@-		| pass loadbase
	movl	%d6,%sp@-		| pass boot partition offset
	movl	%a2,%sp@-		| pass sync inhibit flags
	movl	%d3,%sp@-		| pass AGA mode
	movl	%a4,%sp@-		| pass address of _esym
	movl	%d1,%sp@-		| pass chipmem-size
	movl	%d0,%sp@-		| pass fastmem-size
	movl	%a0,%sp@-		| pass fastmem_start
	movl	%d5,%sp@-		| pass machine id

	/*
	 * initialize some hw addresses to their physical address
	 * for early running
	 */
#ifdef DRACO
	/*
	 * this is already dynamically done on DraCo
	 */
	cmpb	#0x7D,%sp@
	jne	LisAmiga1
| debug code:
| we should need about 1 uSec for the loop.
| we dont need the AGA mode register.
	movel	#100000,%d3
LisDraco0:
#ifdef DEBUG_KERNEL_START
	movb	#0,0x200003c8
	movb	#00,0x200003c9
	movb	#40,0x200003c9
	movb	#00,0x200003c9
|XXX:
	movb	#0,0x200003c8
	movb	#40,0x200003c9
	movb	#00,0x200003c9
	movb	#00,0x200003c9
	subql	#1,%d3
	jcc	LisDraco0
#endif

	RELOC(chipmem_start, %a0)
	movl	#0,%a0@

	RELOC(CIAAbase, %a0)
	movl	#0x2801001, %a0@
	RELOC(CIABbase, %a0)
	movl	#0x2800000, %a0@

	/* XXXX more to come here; as we need it */

	jra	LisDraco1
LisAmiga1:
#endif
	RELOC(chipmem_start, %a0)
	movl	#0x400,%a0@
	RELOC(CIAAbase, %a0)
	movl	#0xbfe001,%a0@
	RELOC(CIABbase, %a0)
	movl	#0xbfd000,%a0@
	RELOC(CUSTOMbase, %a0)
	movl	#0xdff000,%a0@

#ifdef DRACO
LisDraco1:
#endif
	/*
	 * initialize the timer frequency
	 */
	RELOC(eclockfreq, %a0)
	movl	%d4,%a0@

	movl	#AMIGA_68030,%d1	| 68030 Attn flag from exec
	andl	%d5,%d1
	jeq	Ltestfor020
	RELOC(mmutype, %a0)
	movl	#MMU_68030,%a0@		| assume 020 means 851
	RELOC(cputype, %a0)
	movl	#CPU_68030,%a0@
	jra	Lsetcpu040		| skip to init.
Ltestfor020:
	movl	#AMIGA_68020,%d1	| 68020 Attn flag from exec
	andl	%d5,%d1
	jeq	Lsetcpu040
	RELOC(mmutype, %a0)
	movl	#MMU_68851,%a0@
	RELOC(cputype, %a0)
	movl	#CPU_68020,%a0@
Lsetcpu040:
	movl	#CACHE_OFF,%d0		| 68020/030 cache
	movl	#AMIGA_68040,%d1
	andl	%d1,%d5
	jeq	Lstartnot040		| it is not 68040
	RELOC(mmutype, %a0)
	movl	#MMU_68040,%a0@		| same as hp300 for compat
	RELOC(cputype, %a0)
	movl	#CPU_68040,%a0@
	.word	0xf4f8			| cpusha bc - push and invalidate caches
	movl	#CACHE40_OFF,%d0	| 68040 cache disable
#ifndef BB060STUPIDROM
	btst	#7,%sp@(3)
	jeq	Lstartnot040
	movl	#CPU_68060,%a0@		| and in the cputype
	orl	#IC60_CABC,%d0		| XXX and clear all 060 branch cache
#else
	movc	%d0,%cacr
	bset	#30,%d0			| not allocate data cache bit
	movc	%d0,%cacr		| does it stick?
	movc	%cacr,%d0
	tstl	%d0
	jeq	Lstartnot040
	bset	#7,%sp@(3)		| note it is '60 family in machineid
	movl	#CPU_68060,%a0@		| and in the cputype
	orl	#IC60_CABC,%d0		| XXX and clear all 060 branch cache
	.word	0x4e7a,0x1808		| movc	pcr,d1
	swap	%d1
	cmpw	#0x430,%d1
	jne	Lstartnot040		| but no FPU
	bset	#6,%sp@(3)		| yes, we have FPU, note that
	swap	%d1
	bclr	#1,%d1			| ... and switch it on.
	.word	0x4e7b,0x1808		| movc	d1,pcr
#endif
Lstartnot040:
	movc	%d0,%cacr		| clear and disable on-chip cache(s)
	movl	#_C_LABEL(vectab),%a0
	movc	%a0,%vbr

/* initialize source/destination control registers for movs */
	moveq	#FC_USERD,%d0		| user space
	movc	%d0,%sfc		|   as source
	movc	%d0,%dfc		|   and destination of transfers

/* let the C function initialize everything */
	RELOC(start_c, %a0)
	jbsr	%a0@
	lea	%sp@(4*9),%sp

#ifdef DRACO
	RELOC(machineid,%a0)
	cmpb	#0x7d,%a0@
	jne	LAmiga_enable_MMU

	lea	%pc@(0),%a0
	movl	%a0,%d0
	andl	#0xff000000,%d0
	orl	#0x0000c044,%d0		| 16 MB, ro, cache inhibited
	.word	0x4e7b,0x0004		| movc %d0,%itt0
	.word	0xf518			| pflusha
	movl	#0xc000,%d0		| enable MMU
	.word	0x4e7b,0x0003		| movc %d0,%tc
	jmp	Lcleanitt0:l
Lcleanitt0:
	movq	#0,%d0
	.word	0x4e7b,0x0004		| movc %d0,%itt0
	bra	LMMUenable_end

LAmiga_enable_MMU:
#endif /* DRACO */

/* Copy just the code to enable the MMU into chip memory */
	lea	LMMUenable_start,%a0
	movl	#LMMUenable_start:l,%a1
	lea	LMMUenable_end,%a2
Lcopy_MMU_enabler:
	movl	%a0@+,%a1@+
	cmpl	%a0,%a2
	jgt	Lcopy_MMU_enabler

	jmp	LMMUenable_start:l

LMMUenable_start:

/* enable the MMU */
#if defined(M68040) || defined(M68060)
	RELOC(mmutype, %a0)
	cmpl	#MMU_68040,%a0@
	jne	Lenable030
	.word	0xf518			| pflusha
	movl	#0xc000,%d0		| enable MMU
	.word	0x4e7b,0x0003		| movc	%d0,%tc
	jmp	LMMUenable_end:l
#endif /* M68040 || M68060 */
Lenable030:
	lea	Ltc,%a0
	pmove	%a0@,%tc
	jmp	LMMUenable_end:l

/* ENABLE, SRP_ENABLE, 8K pages, 8bit A-level, 11bit B-level */
Ltc:	.long	0x82d08b00

LMMUenable_end:

	lea	_ASM_LABEL(tmpstk),%sp	| give ourselves a temporary stack
	jbsr	_C_LABEL(start_c_finish)

/* set kernel stack, user SP, and initial pcb */
	movl	_C_LABEL(proc0paddr),%a1	| proc0 kernel stack
	lea	%a1@(USPACE),%sp	| set kernel stack to end of area
	lea	_C_LABEL(lwp0),%a2	| initialize lwp0.p_addr so that
	movl	%a2,_C_LABEL(curlwp)
	movl	%a1,%a2@(L_ADDR)	|   we don't dref NULL in trap()
	movl	#USRSTACK-4,%a2
	movl	%a2,%usp		| init user SP
	movl	%a2,%a1@(PCB_USP)	| and save it
	movl	%a1,_C_LABEL(curpcb)	| lwp0 is running
	clrw	%a1@(PCB_FLAGS)		| clear flags
#ifdef FPCOPROC
	clrl	%a1@(PCB_FPCTX)		| ensure null FP context
|WRONG!	movl	%a1,%sp@-
|	pea	%a1@(PCB_FPCTX)
|	jbsr	_C_LABEL(m68881_restore)	| restore it (does not kill a1)
|	addql	#4,%sp
#endif
/* flush TLB and turn on caches */

	jbsr	_C_LABEL(_TBIA)		| invalidate TLB
	movl	#CACHE_ON,%d0
	tstl	%d5
	jeq	Lcacheon
| is this needed? MLH
	.word	0xf4f8			| cpusha bc - push & invalidate caches
	movl	#CACHE40_ON,%d0
#ifdef M68060
	cmpl	#CPU_68060,_C_LABEL(cputype)
	jne	Lcacheon
	movl	#CACHE60_ON,%d0
#endif
Lcacheon:
	movc	%d0,%cacr		| clear cache(s)
/* final setup for C code */

	movw	#PSL_LOWIPL,%sr		| lower SPL

	movl	%d7,_C_LABEL(boothowto)	| save reboot flags
/*
 * Create a fake exception frame that returns to user mode,
 * make space for the rest of a fake saved register set, and
 * pass the first available RAM and a pointer to the register
 * set to "main()".  "main()" will do an "execve()" using that
 * stack frame.
 * When "main()" returns, we're running in process 1 and have
 * successfully executed the "execve()".  We load up the registers from
 * that set; the "rte" loads the PC and PSR, which jumps to "init".
 */
  	clrw	%sp@-			| vector offset/frame type
	clrl	%sp@-			| PC - filled in by "execve"
  	movw	#PSL_USER,%sp@-		| in user mode
	clrl	%sp@-			| stack adjust count
	lea	%sp@(-64),%sp		| construct space for D0-D7/A0-A7
	lea	_C_LABEL(lwp0),%a0		| lwp0 in a0
	movl	%sp,%a0@(L_MD_REGS)	| save frame for lwp0
	movl	%usp,%a1
	movl	%a1,%sp@(FR_SP)		| save user stack pointer in frame
	pea	%sp@			| addr of space for D0

	jbsr	_C_LABEL(main)		| main(firstaddr, r0)
	addql	#4,%sp			| pop args

	cmpl	#MMU_68040,_C_LABEL(mmutype)	| 68040?
	jne	Lnoflush		| no, skip
	.word	0xf478			| cpusha dc
	.word	0xf498			| cinva ic, also clears the 060 btc
Lnoflush:
	movl	%sp@(FR_SP),%a0		| grab and load
	movl	%a0,%usp		|   user SP
	moveml	%sp@+,%d0-%d7/%a0-%a6	| load most registers (all but SSP)
	addql	#8,%sp			| pop SSP and stack adjust count
  	rte

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
	movl	%a2,%sp@-		| scratch register
	movl	%sp@(12),%a2		| get &u.u_prof
	movl	%sp@(8),%d0		| get user pc
	subl	%a2@(8),%d0		| pc -= pr->pr_off
	jlt	Lauexit			| less than 0, skip it
	movl	%a2@(12),%d1		| get pr->pr_scale
	lsrl	#1,%d0			| pc /= 2
	lsrl	#1,%d1			| scale /= 2
	mulul	%d1,%d0			| pc /= scale
	moveq	#14,%d1
	lsrl	%d1,%d0			| pc >>= 14
	bclr	#0,%d0			| pc &= ~1
	cmpl	%a2@(4),%d0		| too big for buffer?
	jge	Lauexit			| yes, screw it
	addl	%a2@,%d0		| no, add base
	movl	%d0,%sp@-		| push address
	jbsr	_C_LABEL(fusword)	| grab old value
	movl	%sp@+,%a0		| grab address back
	cmpl	#-1,%d0			| access ok
	jeq	Lauerror		| no, skip out
	addw	%sp@(18),%d0		| add tick to current value
	movl	%d0,%sp@-		| push value
	movl	%a0,%sp@-		| push address
	jbsr	_C_LABEL(susword)	| write back new value
	addql	#8,%sp			| pop params
	tstl	%d0			| fault?
	jeq	Lauexit			| no, all done
Lauerror:
	clrl	%a2@(12)		| clear scale (turn off prof)
Lauexit:
	movl	%sp@+,%a2		| restore scratch reg
	rts

/*
 * non-local gotos
 */
ENTRY(qsetjmp)
	movl	%sp@(4),%a0	| savearea pointer
	lea	%a0@(40),%a0	| skip regs we do not save
	movl	%a6,%a0@+	| save FP
	movl	%sp,%a0@+	| save SP
	movl	%sp@,%a0@	| and return address
	moveq	#0,%d0		| return 0
	rts

/*
 * Use common m68k process/lwp switch and context save subroutines.
 */
#include <m68k/m68k/switch_subr.s>

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
ENTRY(getsp)
	movl	%sp,%d0				| get current SP
	addql	#4,%d0				| compensate for return address
	movl	%d0,%a0				| Comply with ELF ABI
	rts

ENTRY(getsfc)
	movc	%sfc,%d0
	rts
ENTRY(getdfc)
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
	.word	0xf548				| ptestw (a0)
	moveq	#FC_USERD,%d0			| restore DFC to user space
	movc	%d0,%dfc
	.word	0x4e7a,0x0805			| movec  MMUSR,d0
	rts

/*
 * Load a new user segment table pointer.
 */
ENTRY(loadustp)
	movl	%sp@(4),%d0			| new USTP
	moveq	#PGSHIFT,%d1
	lsll	%d1,%d0				| convert to addr
#ifdef M68060
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
	movc	%d0,%cacr			| invalidate cache(s)
	rts
#ifdef M68060
Lldustp060:
	movc	%cacr,%d1
	orl	#IC60_CUBC,%d1			| clear user btc entries
	movc	%d1,%cacr
#endif
Lldustp040:
	.word	0xf518				| pflusha
	.word	0x4e7b,0x0806			| movec d0,URP
	rts

/*
 * Flush any hardware context associated with given USTP.
 * Only does something for HP330 where we must flush RPT
 * and ATC entries in PMMU.
 */
ENTRY(flushustp)
#ifdef M68060
	cmpl	#CPU_68060,_C_LABEL(cputype)
	jeq	Lflustp060
#endif
	cmpl	#MMU_68040,_C_LABEL(mmutype)
	jeq	Lnot68851
	tstl	_C_LABEL(mmutype)		| 68851 PMMU?
	jle	Lnot68851			| no, nothing to do
	movl	%sp@(4),%d0			| get USTP to flush
	moveq	#PGSHIFT,%d1
	lsll	%d1,%d0				| convert to address
	movl	%d0,_C_LABEL(protorp)+4		| stash USTP
	pflushr	_C_LABEL(protorp)		| flush RPT/TLB entries
Lnot68851:
	rts
#ifdef M68060
Lflustp060:
	movc	%cacr,%d1
	orl	#IC60_CUBC,%d1			| clear user btc entries
	movc	%d1,%cacr
	rts
#endif


ENTRY(ploadw)
	movl	%sp@(4),%a0			| address to load
	cmpl	#MMU_68040,_C_LABEL(mmutype)
	jeq	Lploadw040
	ploadw	#1,%a0@				| pre-load translation
Lploadw040:					| should 68040 do a ptest?
	rts

#ifdef FPCOPROC
/*
 * Save and restore 68881 state.
 * Pretty awful looking since our assembler does not
 * recognize FP mnemonics.
 */
ENTRY(m68881_save)
	movl	%sp@(4),%a0			| save area pointer
	fsave	%a0@				| save state
#if defined(M68020) || defined(M68030) || defined(M68040)
#ifdef M68060
	cmpl	#CPU_68060,_C_LABEL(cputype)
	jeq	Lm68060fpsave
#endif
	tstb	%a0@				| null state frame?
	jeq	Lm68881sdone			| yes, all done
	fmovem	%fp0-%fp7,%a0@(FPF_REGS)	| save FP general registers
	fmovem	%fpcr/%fpsr/%fpi,%a0@(FPF_FPCR)	| save FP control registers
Lm68881sdone:
	rts
#endif

#ifdef M68060
Lm68060fpsave:
	tstb	%a0@(2)				| null state frame?
	jeq	Lm68060sdone			| yes, all done
	fmovem	%fp0-%fp7,%a0@(FPF_REGS)	| save FP general registers
	fmovem	%fpcr,%a0@(FPF_FPCR)		| save FP control registers
	fmovem	%fpsr,%a0@(FPF_FPSR)
	fmovem	%fpi,%a0@(FPF_FPI)
Lm68060sdone:
	rts
#endif

ENTRY(m68881_restore)
	movl	%sp@(4),%a0			| save area pointer
#if defined(M68020) || defined(M68030) || defined(M68040)
#if defined(M68060)
	cmpl	#CPU_68060,_C_LABEL(cputype)
	jeq	Lm68060fprestore
#endif
	tstb	%a0@				| null state frame?
	jeq	Lm68881rdone			| yes, easy
	fmovem	%a0@(FPF_FPCR),%fpcr/%fpsr/%fpi	| restore FP control registers
	fmovem	%a0@(FPF_REGS),%fp0-%fp7	| restore FP general registers
Lm68881rdone:
	frestore %a0@				| restore state
	rts
#endif

#ifdef M68060
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
#endif

/*
 * Handle the nitty-gritty of rebooting the machine.
 *
 */
#if defined(P5PPC68KBOARD)
	.data
GLOBAL(p5ppc)
	.long	0
	.text
#endif

ENTRY_NOPROFILE(doboot)
	movl	#CACHE_OFF,%d0
	cmpl	#MMU_68040,_C_LABEL(mmutype)	| is it 68040
	jne	Ldoboot0
	.word	0xf4f8			| cpusha bc - push and invalidate caches
	nop
	movl	#CACHE40_OFF,%d0
Ldoboot0:
	movc	%d0,%cacr			| disable on-chip cache(s)

	movw	#0x2700,%sr			| cut off any interrupts

#if defined(P5PPC68KBOARD)
	tstl	_C_LABEL(p5ppc)
	jne	Lp5ppcboot
#endif
#if defined(DRACO)
	cmpb	#0x7d,_C_LABEL(machineid)
	jeq	LdbOnDraCo
#endif

	| clear first 4k of CHIPMEM
	movl	_C_LABEL(CHIPMEMADDR),%a0
	movl	%a0,%a1
	movl	#1024,%d0
Ldb1:
	clrl	%a0@+
	dbra	%d0,Ldb1

	| now, copy the following code over
|	lea	%a1@(Ldoreboot),%a0	| KVA starts at 0, CHIPMEM is phys 0
|	lea	%a1@(Ldorebootend),%a1
|	lea	%pc@(Ldoreboot-.+2),%a0
|	addl	%a1,%a0
|	lea	%a0@(128),%a1
|	lea	%pc@(Ldoreboot-.+2),%a2
	lea	Ldoreboot,%a2
	lea	Ldorebootend,%a0
	addl	%a1,%a0
	addl	%a2,%a1
	exg	%a0,%a1
Ldb2:
	movel	%a2@+,%a0@+
	cmpl	%a1,%a0
	jle	Ldb2

	| ok, turn off MMU..
Ldoreboot:
	cmpl	#MMU_68040,_C_LABEL(mmutype)	| is it 68040
 	jeq	Lmmuoff040
	lea	_ASM_LABEL(zero),%a0
	pmove	%a0@,%tc		| Turn off MMU
	lea	_ASM_LABEL(nullrp),%a0
	pmove	%a0@,%crp		| Turn off MMU some more
	pmove	%a0@,%srp		| Really, really, turn off MMU
	jra	Ldoboot1
Lmmuoff040:
	movl	#0,%d0
	.word	0x4e7b,0x0003		| movc d0,TC
	.word	0x4e7b,0x0806		| movc d0,URP
	.word	0x4e7b,0x0807		| movc d0,SRP
Ldoboot1:

	| this weird code is the OFFICIAL way to reboot an Amiga ! ..
	lea	0x1000000,%a0
	subl	%a0@(-0x14),%a0
	movl	%a0@(4),%a0
	subl	#2,%a0
	cmpw	#0x4e70,%a0@		| 68040 kludge: if ROM entry is not
	jne	Ldoreset		| a reset, do the reset here
	jmp	%a0@			| otherwise, jump to the ROM to reset
	| reset needs to be on longword boundary
	nop
#ifdef __ELF__
	.align	4
#else
	.align	2
#endif
Ldoreset:
	| reset unconfigures all memory!
	reset
	| now rely on prefetch for next jmp
	jmp	%a0@
	| NOT REACHED

#if defined(P5PPC68KBOARD)
Lp5ppcboot:
| The Linux-Apus boot code does it in a similar way
| For 040 on uncached pages, eieio can be replaced by nothing.
	movl	_C_LABEL(ZTWOROMADDR),%a0
	lea	%a0@(0xf60000-0xd80000),%a0
	movb	#0x60,%a0@(0x20)
	movb	#0x50,%a0@(0x20)
	movb	#0x30,%a0@(0x20)
	movb	#0x40,%a0@(0x18)
	movb	#0x04,%a0@
Lwaithere:
	jra	Lwaithere
#endif

#ifdef DRACO
LdbOnDraCo:
| we use a TTR. We want to boot even if half of us is already dead.

	movl	_C_LABEL(boot_fphystart), %d0
	lea	LdoDraCoBoot, %a0
	lea	%a0@(%d0),%a0
	andl	#0xFF000000,%d0
	orl	#0x0000C044,%d0	| enable, supervisor, CI, RO
	.word	0x4e7b,0x0004	| movc d0,ITT0
	jmp	%a0@

#ifdef __ELF__
	.align	4
#else
	.align	2
#endif
LdoDraCoBoot:
| turn off MMU now ... were more ore less guaranteed to run on 040/060:
	movl	#0,%d0
	.word	0x4e7b,0x0003	| movc d0,TC
	.word	0x4e7b,0x0806	| movc d0,URP
	.word	0x4e7b,0x0807	| movc d0,SRP
	.word	0x4e7b,0x0004	| movc d0,ITT0
	nop
| map in boot ROM @0:
	reset
| and simulate what a reset exception would have done.
	movl	4,%a0
	movl	0,%a7
	jmp	%a0@
	| NOT REACHED
#endif
/*
 * Reboot directly into a new kernel image.
 * kernel_reload(image, image_size, entry,
 *		 fastram_start, fastram_size, chipram_start, esym, eclockfreq)
 */
ENTRY_NOPROFILE(kernel_reload)
	lea	Lreload_copy,%a0	| cursory validity check of new kernel
	movl	%a0@,%d0		|  to see if the kernel reload code
	addl	%sp@(4),%a0		|  in new image matches running kernel
	cmpl	%a0@,%d0
	jeq	Lreload_ok
	rts				| It doesn't match - can't reload
Lreload_ok:
	jsr	_C_LABEL(bootsync)
	CUSTOMADDR(%a5)

	movew	#(1<<9),%a5@(0x096)	| disable DMA (before clobbering chipmem)

	movl	#CACHE_OFF,%d0
	cmpl	#MMU_68040,_C_LABEL(mmutype)
	jne	Lreload1
	.word	0xf4f8		| cpusha bc - push and invalidate caches
	nop
	movl	#CACHE40_OFF,%d0
Lreload1:
	movc	%d0,%cacr		| disable on-chip cache(s)

	movw	#0x2700,%sr		| cut off any interrupts
	movel	_C_LABEL(boothowto),%d7	| save boothowto
	movel	_C_LABEL(machineid),%d5	| (and machineid)

	movel	%sp@(16),%a0		| load memory parameters
	movel	%sp@(20),%d0
	movel	%sp@(24),%d1
	movel	%sp@(28),%a4		| esym
	movel	%sp@(32),%d4		| eclockfreq
	movel	%sp@(36),%d3		| AGA mode
	movel	%sp@(40),%a2		| sync inhibit flags
	movel	%sp@(44),%d6		| boot partition offset

	movel	%sp@(12),%a6		| find entrypoint (a6)

	movel	%sp@(4),%a1		| copy kernel to low chip memory
	movel	%sp@(8),%d2
	movl	_C_LABEL(CHIPMEMADDR),%a3
Lreload_copy:
	movel	%a1@+,%a3@+
	subl	#4,%d2
	jcc	Lreload_copy

	| ok, turn off MMU..
	cmpl	#MMU_68040,_C_LABEL(mmutype)
	jeq	Lreload040
	lea	_ASM_LABEL(zero),%a3
	pmove	%a3@,%tc		| Turn off MMU
	lea	_ASM_LABEL(nullrp),%a3
	pmove	%a3@,%crp		| Turn off MMU some more
	pmove	%a3@,%srp		| Really, really, turn off MMU
	jra	Lreload2
Lreload040:
	movl	#0,%d2
	.word	0x4e7b,0x2003	| movc d2,TC
	.word	0x4e7b,0x2806	| movc d2,URP
	.word	0x4e7b,0x2807	| movc d2,SRP
Lreload2:

	moveq	#0,%d2			| clear unused registers
	subl	%a1,%a1
	subl	%a3,%a3
	subl	%a5,%a5
	jmp	%a6@			| start new kernel


| A do-nothing MMU root pointer (includes the following long as well)

ASLOCAL(nullrp)
	.long	0x7fff0001
ASLOCAL(zero)
	.long	0
Ldorebootend:

#ifdef __ELF__
	.align 4
#else
	.align 2
#endif
	nop
ENTRY_NOPROFILE(delay)
ENTRY_NOPROFILE(DELAY)
	movql #10,%d1		| 2 +2
	movl %sp@(4),%d0	| 4 +4
	lsll %d1,%d0		| 8 +2
	movl _C_LABEL(delaydivisor),%d1	| A +6
Ldelay:				| longword aligned again.
	subl %d1,%d0
	jcc Ldelay
	rts

#ifdef M68060
ENTRY_NOPROFILE(intemu60)
	addql	#1,L60iem
	jra	_C_LABEL(I_CALL_TOP)+128+0x00
ENTRY_NOPROFILE(fpiemu60)
	addql	#1,L60fpiem
	jra	_C_LABEL(FP_CALL_TOP)+128+0x30
ENTRY_NOPROFILE(fpdemu60)
	addql	#1,L60fpdem
	jra	_C_LABEL(FP_CALL_TOP)+128+0x38
ENTRY_NOPROFILE(fpeaemu60)
	addql	#1,L60fpeaem
	jra	_C_LABEL(FP_CALL_TOP)+128+0x40
#endif

	.data
	.space	PAGE_SIZE
ASLOCAL(tmpstk)

GLOBAL(mmutype)
	.long	MMU_68851
GLOBAL(cputype)
	.long	CPU_68020
GLOBAL(ectype)
	.long	EC_NONE
GLOBAL(fputype)
	.long	FPU_NONE
GLOBAL(protorp)
	.long	0x80000002,0	| prototype root pointer

GLOBAL(proc0paddr)
	.long	0		| KVA of proc0 u-area

GLOBAL(delaydivisor)
	.long	12		| should be enough for 80 MHz 68060
				| will be adapted to other CPUs in
				| start_c_cleanup and calibrated
				| at clock attach time.
#ifdef DEBUG
ASGLOBAL(fulltflush)
	.long	0
ASGLOBAL(fullcflush)
	.long	0
ASGLOBAL(timebomb)
	.long	0
#endif
/* interrupt counters */
GLOBAL(intrnames)
	.asciz	"spur"		| spurious interrupt
	.asciz	"tbe/soft"	| serial TBE & software
	.asciz	"kbd/ports"	| keyboard & PORTS
	.asciz	"vbl"		| vertical blank
	.asciz	"audio"		| audio channels
	.asciz	"rbf"		| serial receive
	.asciz	"exter"		| EXTERN
	.asciz	"nmi"		| non-maskable
	.asciz	"clock"		| clock interrupts
	.asciz	"spur6"		| spurious level 6
#ifdef DRACO
	.asciz	"kbd/soft"	| 1: native keyboard, soft ints
	.asciz	"cia/zbus"	| 2: cia, PORTS
	.asciz	"lclbus"	| 3: local bus, e.g. Altais vbl
	.asciz	"drscsi"	| 4: mainboard scsi
	.asciz	"superio"	| 5: superio chip
	.asciz	"lcl/zbus"	| 6: lcl/zorro lev6
	.asciz	"buserr"	| 7: nmi: bus timeout
#endif
#ifdef M68060
	.asciz	"60intemu"
	.asciz	"60fpiemu"
	.asciz	"60fpdemu"
	.asciz	"60fpeaemu"
	.asciz	"60bpe"
#endif
#ifdef FPU_EMULATE
	.asciz	"fpe"
#endif
GLOBAL(eintrnames)
#ifdef __ELF__
	.align	4
#else
	.align	2
#endif
GLOBAL(intrcnt)
	.long	0,0,0,0,0,0,0,0,0,0
#ifdef DRACO
ASLOCAL(Drintrcnt)
	.long	0,0,0,0,0,0,0
#endif
#ifdef M68060
L60iem:		.long	0
L60fpiem:	.long	0
L60fpdem:	.long	0
L60fpeaem:	.long	0
L60bpe:		.long	0
#endif
#ifdef FPU_EMULATE
Lfpecnt:	.long	0
#endif
GLOBAL(eintrcnt)
