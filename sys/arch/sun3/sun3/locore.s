/*
 * Copyright (c) 1993 Adam Glass
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
 *	This product includes software developed by Adam Glass.
 * 4. The name of the Author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Adam Glass ``AS IS'' AND
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
 * $Header: /cvsroot/src/sys/arch/sun3/sun3/locore.s,v 1.13 1993/10/12 05:26:27 glass Exp $
 */
#include "assym.s"
#include "../include/asm.h"
#include "syscall.h"
					| remember this is a fun project :)


.globl tmpstk
.data
.space NBPG
tmpstk:
.set	_kstack,MONSHORTSEG
.globl _kstack
.text
.globl start; .globl _start

start: _start:
/*
 * First we need to set it up so we can access the sun MMU, and be otherwise
 * undisturbed.  Until otherwise noted, all code must be position independent
 * as the boot loader put us low in memory, but we are linked high.
 */
	movw #PSL_HIGHIPL, sr		| no interrupts
	moveq #FC_CONTROL, d0		| make movs get us to the control
	movc d0, dfc			| space where the sun3 designers
	movc d0, sfc			| put all the "useful" stuff
	moveq #CONTEXT_0, d0
	movsb d0, CONTEXT_REG		| now in context 0

/*
 * In order to "move" the kernel to high memory, we are going to copy
 * the first 8 Mb of pmegs such that we will be mapped at the linked address.
 * This is all done by playing with the segment map, and then propigating it
 * to the other contexts.
 * We will unscramble which pmegs we actually need later.
 *
 */

percontext:					|loop among the contexts
	clrl d1					
	movl #(SEGMAP_BASE+KERNBASE), a0	| base index into seg map

perpmeg:

	movsb d1, a0@				| establish mapping
	addql #1, d1			
	addl #NBSG, a0
	cmpl #(MAINMEM_MONMAP / NBSG), d1	| are we done
	bne perpmeg


	addql #1, d0				| next context ....
	cmpl #CONTEXT_NUM, d0
	bne percontext

	clrl d0					
	movsb d0, CONTEXT_REG			| back to context 0
	
	jmp mapped_right:l			| things are now mapped right:)



mapped_right:

	movl #_edata, a0
	movl #_end, a1
bsszero: clrl a0@
	addql #4, a0
	cmpl a0, a1
	bne bsszero

final_before_main:

	lea tmpstk, sp			| switch to tmpstack
	jsr _sun3_bootstrap		| init everything but calling main()
	lea _kstack, a1			| proc0 kernel stack
	lea a1@(UPAGES*NBPG-4),sp	| set kernel stack to end of area
	pea _kstack_fall_off		| push something to fall back on :)
	movl #USRSTACK-4, a2		
	movl a2, usp			| set user stack
	movl	_proc0paddr,a1		| get proc0 pcb addr
	movl	a1,_curpcb		| proc0 is running
	clrw	a1@(PCB_FLAGS)		| clear flags
#ifdef FPCOPROC
	clrl	a1@(PCB_FPCTX)		| ensure null FP context
	movl	a1,sp@-
	jbsr	_m68881_restore		| restore it (does not kill a1)
	addql	#4,sp
#endif
	movw #PSL_LOWIPL, sr		| nothing blocked
	jsr _main
/* proc[1] == init now running here;
 * create a null exception frame and return to user mode in icode
 */
post_main:
	clrw	sp@-			| vector offset/frame type
	clrl	sp@-			| return to icode location 0
	movw	#PSL_USER,sp@-		| in user mode
	rte				|should get here :)
.text
/*
 * Icode is copied out to process 1 to exec init.
 * If the exec fails, process 1 exits.
 */
	.globl	_icode,_szicode
	.text
_icode:
	clrl	sp@-
	pea	pc@((argv-.)+2)
	pea	pc@((init-.)+2)
	clrl	sp@-
	moveq	#SYS_execve,d0
	trap	#0
	moveq	#SYS_exit,d0
	trap	#0
init:
	.asciz	"/sbin/init"
	.even
argv:
	.long	init+6-_icode		| argv[0] = "init" ("/sbin/init" + 6)
	.long	eicode-_icode		| argv[1] follows icode after copyout
	.long	0
eicode:

_szicode:
	.long	_szicode-_icode
#include "lib.s"
#include "copy.s"
#include "m68k.s"
#include "signal.s"
#include "process.s"
#include "softint.s"
#include "interrupt.s"
#include "trap.s"
