/*	$NetBSD: trap_subr.s,v 1.11.32.1 2007/07/11 20:00:26 mjf Exp $	*/

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

#include "opt_compat_sunos.h"

/*
 * NOTICE: This is not a standalone file.  To use it, #include it in
 * your port's locore.s, like so:
 *
 *	#include <m68k/m68k/trap_subr.s>
 */

/*
 * Common fault handling code.  Called by exception vector handlers.
 * Registers have been saved, and type for trap() placed in d0.
 */
ASENTRY_NOPROFILE(fault)
	movl	%usp,%a0		| get and save
	movl	%a0,%sp@(FR_SP)		|   the user stack pointer
	clrl	%sp@-			| no VA arg
	clrl	%sp@-			| or code arg
	movl	%d0,%sp@-		| push trap type
	pea	%sp@(12)		| address of trap frame
	jbsr	_C_LABEL(trap)		| handle trap
	lea	%sp@(16),%sp		| pop value args
	movl	%sp@(FR_SP),%a0		| restore
	movl	%a0,%usp		|   user SP
	moveml	%sp@+,#0x7FFF		| restore most user regs
	addql	#8,%sp			| pop SP and stack adjust
	jra	_ASM_LABEL(rei)		| all done

/*
 * Similar to above, but will tidy up the stack, if necessary.
 */
ASENTRY(faultstkadj)
	pea	%sp@(12)		| address of trap frame
	jbsr	_C_LABEL(trap)		| handle the error
	lea	%sp@(16),%sp		| pop value args
/* for new 68060 Branch Prediction Error handler */
_ASM_LABEL(faultstkadjnotrap2):
	movl	%sp@(FR_SP),%a0		| restore user SP
	movl	%a0,%usp		|   from save area 
	movw	%sp@(FR_ADJ),%d0	| need to adjust stack?
	jne	1f			| yes, go to it 
	moveml	%sp@+,#0x7FFF		| no, restore most user regs
	addql	#8,%sp			| toss SSP and stkadj 
	jra	_ASM_LABEL(rei)		| all done
1:
	lea	%sp@(FR_HW),%a1		| pointer to HW frame
	addql	#8,%a1			| source pointer
	movl	%a1,%a0			| source
	addw	%d0,%a0			|  + hole size = dest pointer
	movl	%a1@-,%a0@-		| copy
	movl	%a1@-,%a0@-		|  8 bytes
	movl	%a0,%sp@(FR_SP)		| new SSP
	moveml	%sp@+,#0x7FFF		| restore user registers
	movl	%sp@,%sp		| and our SP
	jra	_ASM_LABEL(rei)		| all done

#if defined(COMPAT_13) || defined(COMPAT_SUNOS)
/*
 * Trap 1 - compat_13_sigreturn13
 */
ENTRY_NOPROFILE(trap1)
	jra	_C_LABEL(m68k_compat_13_sigreturn13_stub)
#endif

/*
 * Trap 2 - trace trap
 *
 * XXX SunOS uses this for a cache flush!  What do we do here?
 * XXX
 * XXX	movl	#IC_CLEAR,%d0
 * XXX	movc	%d0,%cacr
 * XXX	rte
 */
ENTRY_NOPROFILE(trap2)
	jra	_C_LABEL(trace)

#if defined(COMPAT_16)
/*
 * Trap 3 - sigreturn system call
 */
ENTRY_NOPROFILE(trap3)
	jra	_C_LABEL(m68k_compat_16_sigreturn14_stub)
#endif

/*
 * The following exceptions only cause four and six word stack frames
 * and require no post-trap stack adjustment.
 */
ENTRY_NOPROFILE(illinst)
	clrl	%sp@-
	moveml	#0xFFFF,%sp@-
	moveq	#T_ILLINST,%d0
	jra	_ASM_LABEL(fault)

ENTRY_NOPROFILE(zerodiv)
	clrl	%sp@-
	moveml	#0xFFFF,%sp@-
	moveq	#T_ZERODIV,%d0
	jra	_ASM_LABEL(fault)

ENTRY_NOPROFILE(chkinst)
	clrl	%sp@-
	moveml	#0xFFFF,%sp@-
	moveq	#T_CHKINST,%d0
	jra	_ASM_LABEL(fault)

ENTRY_NOPROFILE(trapvinst)
	clrl	%sp@-
	moveml	#0xFFFF,%sp@-
	moveq	#T_TRAPVINST,%d0
	jra	_ASM_LABEL(fault)

ENTRY_NOPROFILE(privinst)
	clrl	%sp@-
	moveml	#0xFFFF,%sp@-
	moveq	#T_PRIVINST,%d0
	jra	_ASM_LABEL(fault)

/*
 * Coprocessor and format errors can generate mid-instruction stack
 * frames and cause signal delivery, hence we need to check for potential
 * stack adjustment.
 */
ENTRY_NOPROFILE(coperr)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-
	movl	%usp,%a0		| get and save
	movl	%a0,%sp@(FR_SP)		|   the user stack pointer
	clrl	%sp@-			| no VA arg
	clrl	%sp@-			| or code arg
	movl	#T_COPERR,%sp@-		| push trap type
	jra	_ASM_LABEL(faultstkadj)	| call trap and deal with stack
					|   adjustments

ENTRY_NOPROFILE(fmterr)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-
	movl	%usp,%a0		| get and save
	movl	%a0,%sp@(FR_SP)		|   the user stack pointer
	clrl	%sp@-			| no VA arg
	clrl	%sp@-			| or code arg
	movl	#T_FMTERR,%sp@-		| push trap type
	jra	_ASM_LABEL(faultstkadj)	| call trap and deal with stack
					|   adjustments
