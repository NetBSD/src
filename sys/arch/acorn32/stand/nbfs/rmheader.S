/*	$NetBSD: rmheader.S,v 1.1.10.2 2006/05/24 15:47:49 tron Exp $	*/

/*-
 * Copyright (c) 2001, 2006 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

/*
 * Relocatable module header for NBFS.
 */

#include <arm/armreg.h>
#include <riscoscalls.h>

rmbase:
	.word	0			/* Start code */
	.word	rminit - rmbase		/* Initialisation code */
	.word	rmfinal - rmbase	/* Finalisation code */
	.word	0			/* Service call handler */
	.word	rmtitle - rmbase	/* Title string */
	.word	rmhelp - rmbase		/* Help string */
	.word	0			/* Help and command keyword table */
	.word	0			/* SWI chunk base number */
	.word	0			/* SWI handler code offset */
	.word	0			/* SWI decoding table offset */
	.word	0			/* SWI decoding code offset */
	.word	0			/* Messages filename offset */
	.word	flags - rmbase		/* Module flags extension offset */

fsib:	/* Filing system information block */
	.word	rmtitle - rmbase	/* Filing system name */
	.word	rmtitle - rmbase	/* Filing system boot text */
	.word	fsentry_open - rmbase
	.word	fsentry_getbytes - rmbase
	.word	fsentry_putbytes - rmbase
	.word	fsentry_args - rmbase
	.word	fsentry_close - rmbase
	.word	fsentry_file - rmbase
	.word	fileswitch_SUPPORTS_SPECIAL | fileswitch_READ_ONLY | \
		0xf0 /* borrow UnixFS's number for now */
	.word	fsentry_func - rmbase
	.word	0			/* FSEntry_GBPB */
	.word	0			/* Extra information word */

rmtitle:
	.asciz	"NBFS"
	.align
flags:
	.word	0			/* not 32 bit compatible */

rminit:
	/*
	 * Module initialisation code
	 *
	 * On entry:
	 * r10 = pointer to environment string
	 * r11 = I/O base or instantiation number
	 * r12 = pointer to private word
	 * r14 = return address
	 */
	stmfd	r13!, {r14}
	ldr	r1, Loldbase		/* Get old base address */
	adr	r2, rmbase		/* Get new base address */
	str	r2, Loldbase		/* New is the new old */
	ldr	r0, L_DYNAMIC		/* Pointer to the dynamic table */
	add	r0, r0, r2		/* ... relocated appropriately */
	bl	relocate_self		/* Call the relocation code */
	cmp	r0, #0
	bne	Lerror
	mov	r0, #OSFSControl_AddFS
	adr	r1, rmbase
	mov	r2, #(fsib - rmbase)
	mov	r3, #0
	swi	XOS_FSControl
	ldmfd	r13!, {pc}		/* If that failed, so do we */

rmfinal:
	stmfd	r13!, {r14}
	mov	r0, #OSFSControl_RemoveFS
	adr	r1, rmtitle
	swi	XOS_FSControl
	cmp	r0, r0			/* Clear V flag */
	ldmfd	r13!, {pc}

Lerror:
	teq	pc, pc			/* In 26-bit mode? */
	ldmneia	r13!, {r14}		/* If so, load up return address */
	orrnes	pc, r14, #R15_FLAG_V	/* and return setting V flag */
	mrs	r14, cpsr		/* Otherwise get cpsr */
	orr	r14, r14, #PSR_V_bit	/* set V flag */
	msr	cpsr_c, r14		/* put it back */
	ldmfd	r13!, {pc}		/* and return */

Loldbase:
	.word	0
L_DYNAMIC:
	.word	_DYNAMIC - rmbase


fsentry_open:
	stmfd	r13!, {r0-r7, r14}
	mov	r0, r13
	adr	r14, fsentry_ret
	b	nbfs_open

fsentry_getbytes:
	stmfd	r13!, {r0-r7, r14}
	mov	r0, r13
	adr	r14, fsentry_ret
	b	nbfs_getbytes

fsentry_putbytes:
	stmfd	r13!, {r0-r7, r14}
	mov	r0, r13
	adr	r14, fsentry_ret
	b	nbfs_putbytes

fsentry_args:
	stmfd	r13!, {r0-r7, r14}
	mov	r0, r13
	adr	r14, fsentry_ret
	b	nbfs_args

fsentry_close:
	stmfd	r13!, {r0-r7, r14}
	mov	r0, r13
	adr	r14, fsentry_ret
	b	nbfs_close

fsentry_file:
	stmfd	r13!, {r0-r7, r14}
	mov	r0, r13
	adr	r14, fsentry_ret
	b	nbfs_file

fsentry_func:
	stmfd	r13!, {r0-r7, r14}
	mov	r0, r13
	adr	r14, fsentry_ret
	b	nbfs_func

fsentry_ret:
	cmp	r0, #0
	ldmeqfd	r13!, {r0-r7, pc}
	add	r13, r13, #4
	ldmfd	r13!, {r1-r7}
	b	Lerror