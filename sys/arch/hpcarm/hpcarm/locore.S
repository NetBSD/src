/*	$NetBSD: locore.S,v 1.3 2001/06/01 14:04:29 toshii Exp $	*/

/*
 * Copyright (C) 1994-1997 Mark Brinicombe
 * Copyright (C) 1994 Brini
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
 *	This product includes software developed by Brini.
 * 4. The name of Brini may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_ipkdb.h"
#include "assym.h"
#include <sys/syscall.h>
#include <sys/errno.h>
#include <machine/asm.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/param.h>

/* What size should this really be ? It is only used by init_arm() */
#define INIT_ARM_STACK_SIZE	2048

/*
 * This is for kvm_mkdb, and should be the address of the beginning
 * of the kernel text segment (not necessarily the same as kernbase).
 */

ENTRY_NP(kernel_text)

ASENTRY_NP(start)
	/* Put the processer in SVC mode */
	mov	r5, sp
	mrs	r4, cpsr_all
	bic	r4, r4, #31
	orr	r4, r4, #(PSR_SVC32_MODE)
	msr	cpsr_all, r4
	mov	sp, r5

	/* Disable PID virtual address mapping */
	mov	r4, #0
	mcr	15, 0, r4, c13, c0, 0

	mov	fp, #0x00000000			/* trace back starts here */
	bl	_C_LABEL(initarm)		/* Off we go */

	/* init arm will return the new stack pointer. */
	mov	sp, r0

	mov	fp, #0x00000000		/* trace back starts here */
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4

	/* Setup an initial trap frame for start_init to use */

	PUSHFRAME

	mov	r0, sp			/* parameter to main is trap frame */ 

	bl	_C_LABEL(main)		/* Lets light the flame and start her up */

	PULLFRAME			/* Pull the trap frame, now valid */

	movs	pc, lr			/* Exit to user process */

	/* Never gets here */

	b	.


/*
 * Instructions to copy to the bottom of zero page
 * These are the entry point to the system exception routines
 */

	.text
	.align	0
	.global	_C_LABEL(page0), _C_LABEL(page0_end)
_C_LABEL(page0):
	ldr	pc, [pc, #Lreset - . - 8]
	ldr	pc, [pc, #Lundefined - . - 8]
	ldr	pc, [pc, #Lswi - . - 8]
	ldr	pc, [pc, #Labortpre - . - 8]
	ldr	pc, [pc, #Labortdata - . - 8]
	ldr	pc, [pc, #Laddrexc - . - 8]
	ldr	pc, [pc, #Lirq - . - 8]
	ldr	pc, [pc, #Lfiq - . - 8]

Lreset:
	.word	reset_entry
Lundefined:
	.word	undefined_entry
Lswi:
	.word	swi_entry
Labortpre:
	.word	prefetch_abort_entry
Labortdata:
	.word	data_abort_entry
Laddrexc:
	.word	addrexc
Lirq:
	.word	irq_entry
Lfiq:
	.word	fiq
_C_LABEL(page0_end) =	.

/* vector 0x00000000 - RESET */

ASENTRY_NP(reset_entry)
	PUSHFRAME

 	mov	r0, sp			/* Pass the frame to function */
	b	_C_LABEL(resethandler)	/* It's a branch throught zero ! */

/* vector 0x00000008 - ADDRESS EXCEPTION */

ASENTRY_NP(addrexc)
	mrs	r1, cpsr
	mrs	r2, spsr
	mov	r3, lr
	add	r0, pc, #Laddrexcmsg - . - 8
	bl	_C_LABEL(printf)
	b	data_abort_entry

Laddrexcmsg:
	.asciz	"address exception CPSR=%08x SPSR=%08x lr=%08x\n"
	.align	0

/* vector 0x0000001C - FIQ */

ASENTRY_NP(fiq)
	ldr	r0, Lfiqmsg
	b	_C_LABEL(panic)

Lfiqmsg:
	.asciz	"fiq"
	.align	0

Lcpufuncs:	
	.word	_C_LABEL(cpufuncs)

ENTRY_NP(cpu_reset)
	mrs     r2, cpsr
	bic	r2, r2, #(PSR_MODE)
	orr     r2, r2, #(PSR_SVC32_MODE)
	orr	r2, r2, #(I32_bit | F32_bit)
	msr     cpsr, r2

	ldr	r4, Lcpu_reset_address
	ldr	r4, [r4]

	ldr	r0, Lcpufuncs
	add	lr, pc, #Lboot_cache_purged - . - 8
	ldr	pc, [r0, #CF_CACHE_PURGE_ID]

Lboot_cache_purged:

	/*
 	 * MMU & IDC off, 32 bit program & data space
	 * Hurl ourselves into the ROM
	 */
	mov	r0, #(CPU_CONTROL_32BP_ENABLE | CPU_CONTROL_32BD_ENABLE)
	mcr     15, 0, r0, c1, c0, 0
	mcr     15, 0, r0, c8, c7, 0
	mov     pc, r4

	/*
	 * _cpu_reset_address contains the address to branch to, to complete
	 * the cpu reset after turning the MMU off
	 * This variable is provided by the hardware specific code
	 */

Lcpu_reset_address:
	.word	_C_LABEL(cpu_reset_address)

#ifdef IPKDB
#if	0
/*
 * ipkdbfbyte and ipkdbsbyte are now in ipkdb_glue.c and do not tweak
 * the abort handler anymore
 */
ENTRY_NP(ipkdbfbyte)
	ldr	ip, abortp
	ldr	r2, [ip]
	add	r3, pc, #ipkdbfault - . - 8
	str	r3, [ip]
	ldrb	r0, [r0]
	str	r2, [ip]
	mov	pc, lr

ENTRY_NP(ipkdbsbyte)
	ldr	ip, abortp
	ldr	r2, [ip]
	add	r3, pc, #ipkdbfault - . - 8
	str	r3, [ip]
	strb	r1, [r0]
	sub	r0, r0, r0
	str	r2, [ip]
	mov	pc, lr

abortp:
	.word	Labortdata - _C_LABEL(page0)
ipkdbfault:
	mov	r0, #0xd3
	msr	cpsr, r0
	mvn	r0, #0			/* mov	r0, #-1 */
	str	r2, [ip]
	mov	pc, lr
#endif

/*
 * Execute(inst, psr, args, sp)
 *
 * Execute INSTruction with PSR and ARGS[0] - ARGS[3] making
 * available stack at SP for next undefined instruction trap.
 *
 * Move the instruction onto the stack and jump to it.
 */
ENTRY_NP(Execute)
	mov	ip, sp
	stmfd	sp!, {r2, r4-r7, fp, ip, lr, pc}
	sub	fp, ip, #4
	mov	ip, r3
	ldr	r7, return
	stmfd	sp!, {r0, r7}
	add	r7, pc, #LExec - . - 8
	mov	r5, r1
	mrs	r4, cpsr
	ldmia	r2, {r0-r3}
	mov	r6, sp
	mov	sp, ip
	msr	cpsr, r5
	mov	pc, r6
LExec:
	mrs	r5, cpsr
/* XXX Cannot switch thus easily back from user mode */
	msr	cpsr, r4
	add	sp, r6, #8
	ldmfd	sp!, {r6}
	stmia	r6, {r0-r3}
	mov	r0, r5
	ldmdb	fp, {r4-r7, fp, sp, pc}
return:
	mov	pc, r7
#endif

/*
 * setjump + longjmp
 */
ENTRY(setjmp)
	stmia	r0, {r4-r14}
	mov	r0, #0x00000000
	mov	pc, lr

ENTRY(longjmp)
	ldmia	r0, {r4-r14}
	mov	r0, #0x00000001
	mov	pc, lr

	.data
	.global _C_LABEL(esym)
_C_LABEL(esym):	.word	_C_LABEL(end)

ENTRY_NP(abort)
	b	_C_LABEL(abort)


/*
 * Atomic bit set and clear functions
 */

ENTRY(atomic_set_bit)
	mrs	r2, cpsr
	orr	r3, r2, #(I32_bit)
	msr	cpsr, r3

	ldr	r3, [r0]
	orr	r3, r3, r1
	str	r3, [r0]

	msr	cpsr, r2
	mov	pc, lr


ENTRY(atomic_clear_bit)
	mrs	r2, cpsr
	orr	r3, r2, #(I32_bit)
	msr	cpsr, r3

	ldr	r3, [r0]
	bic	r3, r3, r1
	str	r3, [r0]

	msr	cpsr, r2
	mov	pc, lr

/* End of locore.S */
