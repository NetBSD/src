/*	$NetBSD: aica_arm_locore.S,v 1.1 2003/08/24 17:33:31 marcus Exp $	*/

/*
 * Copyright (c) 2003 SHIMIZU Ryo <ryo@misakimix.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

	.text
	.globl

	b	exp_reset
	b	exp_undef
	b	exp_swi
	b	exp_pabort
	b	exp_dabort
	b	exp_reserved
	b	exp_irq
/*	b	exp_fiq	*/
exp_fiq:
	sub	pc,r14,#4

exp_reset:
	mov	sp,#0xff00	/* setup stack */

	mrs	r0,CPSR		/* disable interrupt */
	bic	r0,r0,#0x80
	bic	r0,r0,#0x40
	msr	CPSR_all,r0

	bl	aica_main

exp_reserved:
	b	exp_reserved

exp_irq:
	sub	pc,r14,#4

exp_dabort:
	sub	pc,r14,#8

exp_pabort:
	sub	pc,r14,#4

exp_swi:
	mov	pc,r14

exp_undef:
	mov	pc,r14

