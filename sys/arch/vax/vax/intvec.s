/*      $NetBSD: intvec.s,v 1.6 1995/02/13 00:46:08 ragge Exp $   */

/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
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
 *     This product includes software developed at Ludd, University of Lule}.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 /* All bugs are subject to removal without further notice */
		


#include "vax/include/mtpr.h"
#include "vax/include/pte.h"
#include "vax/include/trap.h"
#include "uba.h"

#define	TRAPCALL(namn, typ)	\
	.align 2; namn ## :;.globl namn ;pushl $0; pushl $typ; jbr trap;

#define	TRAPARGC(namn, typ)	\
	.align 2; namn ## :;.globl namn ; pushl $typ; jbr trap;

#define	FASTINTR(namn, rutin)	\
	.align 2; namn ## :;.globl namn ;pushr $0x3f; \
	calls $0,_ ## rutin ;popr $0x3f;rei
#define	STRAY(scbnr,vecnr) \
	.align 2;stray ## vecnr ## :;pushr $0x3f;pushl $ ## 0x ## vecnr; \
	pushl $scbnr; calls $2,_stray ; popr $0x3f; rei;
#define	KSTACK 0
#define ISTACK 1
#define INTVEC(label,stack)	\
	.long	label+stack;
		.text

		.globl	_kernbase
_kernbase:
	INTVEC(stray00, ISTACK)	# Unused., 0
	INTVEC(mcheck, ISTACK)		# Machine Check., 4
	INTVEC(stray08, ISTACK)	# Kernel Stack Invalid., 8
	INTVEC(stray0C, ISTACK)	# Power Failed., C
	INTVEC(privinflt, KSTACK)	# Privileged/Reserved Instruction.
	INTVEC(stray14, ISTACK)	# Customer Reserved Instruction, 14
	INTVEC(resopflt, KSTACK)	# Reserved Operand/Boot Vector(?), 18
	INTVEC(resadflt, KSTACK)	# # Reserved Address Mode., 1C
	INTVEC(access_v, KSTACK)	# Access Control Violation, 20
	INTVEC(transl_v, KSTACK)	# Translation Invalid, 24
	INTVEC(stray28, ISTACK)	# Trace Pending, 28
	INTVEC(stray2C, ISTACK)	# Breakpoint Instruction, 2C
	INTVEC(stray30, ISTACK)	# Compatibility Exception, 30
	INTVEC(arithflt, KSTACK)	# Arithmetic Fault, 34
	INTVEC(stray38, ISTACK)	# Unused, 38
	INTVEC(stray3C, ISTACK)	# Unused, 3C
	INTVEC(syscall, KSTACK)		# main syscall trap, chmk, 40
	INTVEC(resopflt, KSTACK)	# chme, 44
	INTVEC(resopflt, KSTACK)	# chms, 48
	INTVEC(resopflt, KSTACK)	# chmu, 4C
	INTVEC(stray50, ISTACK)	# System Backplane Exception, 50
	INTVEC(stray54, ISTACK)	# Corrected Memory Read, 54
	INTVEC(stray58, ISTACK)	# System Backplane Alert, 58
	INTVEC(stray5C, ISTACK)	# System Backplane Fault, 5C
	INTVEC(stray60, ISTACK)	# Memory Write Timeout, 60
	INTVEC(stray64, ISTACK)	# Unused, 64
	INTVEC(stray68, ISTACK)	# Unused, 68
	INTVEC(stray6C, ISTACK)	# Unused, 6C
	INTVEC(stray70, ISTACK)	# Unused, 70
	INTVEC(stray74, ISTACK)	# Unused, 74
	INTVEC(stray78, ISTACK)	# Unused, 78
	INTVEC(stray7C, ISTACK)	# Unused, 7C
	INTVEC(stray80, ISTACK)	# Unused, 80
	INTVEC(stray84, ISTACK)	# Unused, 84
	INTVEC(astintr,	 KSTACK)	# Asynchronous Sustem Trap, AST
	INTVEC(stray8C, ISTACK)	# Unused, 8C
	INTVEC(stray90, ISTACK)	# Unused, 90
	INTVEC(stray94, ISTACK)	# Unused, 94
	INTVEC(stray98, ISTACK)	# Unused, 98
	INTVEC(stray9C, ISTACK)	# Unused, 9C
	INTVEC(softclock,ISTACK)	# Software clock interrupt
	INTVEC(strayA4, ISTACK)	# Unused, A4
	INTVEC(strayA8, ISTACK)	# Unused, A8
	INTVEC(strayAC, ISTACK)	# Unused, AC
	INTVEC(netint,   ISTACK)	# Network interrupt
	INTVEC(strayB4, ISTACK)	# Unused, B4
	INTVEC(strayB8, ISTACK)	# Unused, B8
	INTVEC(strayBC, ISTACK)	# Unused, BC
	INTVEC(hardclock,ISTACK)	# Interval Timer
	INTVEC(strayC4, ISTACK)	# Unused, C4
	INTVEC(strayC8, ISTACK)	# Unused, C8
	INTVEC(strayCC, ISTACK)	# Unused, CC
	INTVEC(strayD0, ISTACK)	# Unused, D0
	INTVEC(strayD4, ISTACK)	# Unused, D4
	INTVEC(strayD8, ISTACK)	# Unused, D8
	INTVEC(strayDC, ISTACK)	# Unused, DC
	INTVEC(strayE0, ISTACK)	# Unused, E0
	INTVEC(strayE4, ISTACK)	# Unused, E4
	INTVEC(strayE8, ISTACK)	# Unused, E8
	INTVEC(strayEC, ISTACK)	# Unused, EC
	INTVEC(strayF0, ISTACK)	# Console Storage Recieve Interrupt
	INTVEC(strayF4, ISTACK)	# Console Storage Transmit Interrupt
	INTVEC(consrint, ISTACK)	# Console Terminal Recieve Interrupt
	INTVEC(constint, ISTACK)	# Console Terminal Transmit Interrupt


		.globl _V_DEVICE_VEC
_V_DEVICE_VEC:  .space 0x100

#if NUBA
#include "vax/uba/ubavec.s"
#endif

#if NUBA>4 /* Safety belt */
#error "Number of bus adapters must be increased in ubavec.s"
#endif

	STRAY(0, 00)

		.align 2
#
# mcheck is the badaddress trap, also called when referencing
# a invalid address (busserror)
# _memtest (memtest in C) holds the address to continue execution
# at when returning from a intentional test.
#
mcheck:	.globl	mcheck
	tstl	_cold		# Ar we still in coldstart?
	bneq	1f		# Yes.

	pushr	$0x3f
	pushab	24(sp)
	calls	$1, _machinecheck
	popr	$0x3f
	addl2	(sp)+,sp

        rei

1:	addl2	(sp)+,sp	# remove info pushed on stack
	mtpr	$0xF,$PR_MCESR	# clear the bus error bit
	movl	_memtest,(sp)	# REI to new adress
	rei

	STRAY(0, 08)
	STRAY(0, 0C)

	TRAPCALL(privinflt, T_PRIVINFLT)

	STRAY(0, 14)

	TRAPCALL(resopflt, T_RESOPFLT)
	TRAPCALL(resadflt, T_RESADFLT)

		.align	2
transl_v:	.globl	transl_v	# Translation violation
	pushl	$T_TRANSFLT
3:	bbc	$1,4(sp),1f
	bisl2	$T_PTEFETCH, (sp)
1:	bbc	$2,4(sp),2f
	bisl2	$T_WRITE, (sp)
2:	movl	(sp), 4(sp)
	addl2	$4, sp
	jbr	trap


		.align  2
access_v:.globl	access_v	# Access cntrl viol fault
	blbs	(sp), ptelen
	pushl	$T_ACCFLT
	jbr	3b

ptelen:	movl	$T_PTELEN, (sp)		# PTE must expand (or send segv)
	jbr trap;


	STRAY(0, 28)
	STRAY(0, 2C)
	STRAY(0, 30)

	TRAPARGC(arithflt, T_ARITHFLT)

	STRAY(0, 38)
	STRAY(0, 3C)





	.align 2		# Main system call 
	.globl	syscall
syscall:
	pushl	$T_SYSCALL
	pushr	$0x3f
	pushl	ap
	pushl	fp
	pushl	sp		# pointer to syscall frame; defined in trap.h
	calls	$1,_syscall
	movl	(sp)+,fp
	movl	(sp)+,ap
	popr	$0x3f
	addl2	$8,sp
	mtpr	$0x1f,$PR_IPL	# Be sure we can REI
	rei


	STRAY(0, 44)
	STRAY(0, 48)
	STRAY(0, 4C)
	STRAY(0, 50)
	STRAY(0, 54)
	STRAY(0, 58)
	STRAY(0, 5C)
	STRAY(0, 60)
	STRAY(0, 64)
	STRAY(0, 68)
	STRAY(0, 6C)
	STRAY(0, 70)
	STRAY(0, 74)
	STRAY(0, 78)
	STRAY(0, 7C)
	STRAY(0, 80)
	STRAY(0, 84)

	TRAPCALL(astintr, T_ASTFLT)

	STRAY(0, 8C)
	STRAY(0, 90)
	STRAY(0, 94)
	STRAY(0, 98)
	STRAY(0, 9C)

	FASTINTR(softclock, softclock)

	STRAY(0, A4)
	STRAY(0, A8)
	STRAY(0, AC)

	FASTINTR(netint, netintr)	#network packet interrupt

	STRAY(0, B4)
	STRAY(0, B8)
	STRAY(0, BC)

		.align	2
		.globl	hardclock
hardclock:	mtpr	$0xc1,$PR_ICCS		# Reset interrupt flag
		pushr	$0x3f
		pushl	sp
		addl2	$24,(sp)
		calls	$1,_hardclock
		popr	$0x3f
		rei

	STRAY(0, C4)
	STRAY(0, C8)
	STRAY(0, CC)
	STRAY(0, D0)
	STRAY(0, D4)
	STRAY(0, D8)
	STRAY(0, DC)
	STRAY(0, E0)
	STRAY(0, E4)
	STRAY(0, E8)
	STRAY(0, EC)
	STRAY(0, F0)
	STRAY(0, F4)

	FASTINTR(consrint, gencnrint)
	FASTINTR(constint, gencntint)

trap:	pushr	$0x3f
	pushl	ap
	pushl	fp
	pushl	sp
	calls	$1,_arithflt
	movl	(sp)+,fp
	movl	(sp)+,ap
        popr	$0x3f
	addl2	$8,sp
	mtpr	$0x1f,$PR_IPL	# Be sure we can REI
	rei

#if 0

msg_0, 00:	.asciz	"\r\nTrap:	0x00	Unused.\r\n"
msg_0, 08:	.asciz	"\r\nTrap:	0x08	V_K_STK_INV\r\n"
msg_0, 0C:	.asciz	"\r\nTrap:	0x0C	V_POWER_FAIL\r\n"

msg_0, 14:	.asciz	"\r\nTrap:	0x14	V_CUSTOMER\n"

msg_0, 28:	.asciz	"\r\nTrap:	0x28	V_TRACE_PEND\r\n"
msg_0, 2C:	.asciz	"\r\nTrap:	0x2C	V_BREAKPOINT\r\n"

msg_0, 30:	.asciz	"\r\nTrap:	0x30	V_COMPAT\r\n"
msg_0, 38:	.asciz	"\r\nTrap:	0x38	Unused.\r\n"
msg_0, 3C:	.asciz	"\r\nTrap:	0x3C	Unused.\r\n"

msg_0, 44:	.asciz	"\r\nTrap:	0x44	V_CHME\r\n"
msg_0, 48:	.asciz	"\r\nTrap:	0x48	V_CHMS.\r\n"
msg_0, 4C:	.asciz	"\r\nTrap:	0x4C	V_CHMU\r\n"

msg_0, 50:	.asciz	"\r\nTrap:	0x50	V_SBI_SILO\r\n"
msg_0, 54:	.asciz	"\r\nTrap:	0x54	V_CORR_READ\r\n"
msg_0, 58:	.asciz	"\r\nTrap:	0x58	V_SBI_ALERT\r\n"
msg_0, 5C:	.asciz	"\r\nTrap:	0x5C	V_SBI_FAULT\r\n"

msg_0, 60:	.asciz	"\r\nTrap:	0x60	V_MEM_W_TOUT\r\n"
msg_0, 64:	.asciz	"\r\nTrap:	0x64	Unused.\r\n"
msg_0, 68:	.asciz	"\r\nTrap:	0x68	Unused.\r\n"
msg_0, 6C:	.asciz	"\r\nTrap:	0x6C	Unused.\r\n"

msg_0, 70:	.asciz	"\r\nTrap:	0x70	Unused.\r\n"
msg_0, 74:	.asciz	"\r\nTrap:	0x74	Unused.\r\n"
msg_0, 78:	.asciz	"\r\nTrap:	0x78	Unused.\r\n"
msg_0, 7C:	.asciz	"\r\nTrap:	0x7C	Unused.\r\n"

msg_0, 80:	.asciz	"\r\nTrap:	0x80	Unused.\r\n"
msg_0, 84:	.asciz	"\r\nTrap:	0x84	V_SW_LVL1\r\n"
msg_0, 8C:	.asciz	"\r\nTrap:	0x8C	V_SW_LVL3\r\n"

msg_0, 90:	.asciz	"\r\nTrap:	0x90	V_SW_LVL4\r\n"
msg_0, 94:	.asciz	"\r\nTrap:	0x94	V_SW_LVL5\r\n"
msg_0, 98:	.asciz	"\r\nTrap:	0x98	V_SW_LVL6\r\n"
msg_0, 9C:	.asciz	"\r\nTrap:	0x9C	V_SW_LVL7\r\n"

msg_0, A4:	.asciz	"\r\nTrap:	0xA4	V_SW_LVL9\r\n"
msg_0, A8:	.asciz	"\r\nTrap:	0xA8	V_SW_LVL10\r\n"
msg_0, AC:	.asciz	"\r\nTrap:	0xAC	V_SW_LVL11\r\n"

msg_0, B4:	.asciz	"\r\nTrap:	0xB4	V_SW_LVL13\r\n"
msg_0, B8:	.asciz	"\r\nTrap:	0xB8	V_SW_LVL14\r\n"
msg_0, BC:	.asciz	"\r\nTrap:	0xBC	V_SW_LVL15\r\n"

msg_0, C4:	.asciz	"\r\nTrap:	0xC4	Unused.\r\n"
msg_0, C8:	.asciz	"\r\nTrap:	0xC8	Unused.\r\n"
msg_0, CC:	.asciz	"\r\nTrap:	0xCC	Unused.\r\n"

msg_0, D0:	.asciz	"\r\nTrap:	0xD0	Unused.\r\n"
msg_0, D4:	.asciz	"\r\nTrap:	0xD4	Unused.\r\n"
msg_0, D8:	.asciz	"\r\nTrap:	0xD8	Unused.\r\n"
msg_0, DC:	.asciz	"\r\nTrap:	0xDC	Unused.\r\n"

msg_0, E0:	.asciz	"\r\nTrap:	0xE0	Unused.\r\n"
msg_0, E4:	.asciz	"\r\nTrap:	0xE4	Unused.\r\n"
msg_0, E8:	.asciz	"\r\nTrap:	0xE8	Unused.\r\n"
msg_0, EC:	.asciz	"\r\nTrap:	0xEC	Unused.\r\n"

msg_0, F0:	.asciz	"\r\nTrap:	0xF0	V_CONSOLE_SR\r\n"
msg_0, F4:	.asciz	"\r\nTrap:	0xF4	V_CONSOLE_ST\r\n"
#endif
