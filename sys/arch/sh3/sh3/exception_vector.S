/*	$NetBSD: exception_vector.S,v 1.1 2002/02/24 18:19:43 uch Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "assym.h"
	
#include <sh3/asm.h>
#include <sh3/locore.h>

/*
 * Exception vectors. following routines are copied to vector addreses.
 *	sh_vector_generic:	VBR + 0x100	
 *	sh_vector_tlbmiss:	VBR + 0x400
 *	sh_vector_interrupt:	VBR + 0x600
 */ 		

/*
 * void sh_vector_generic(void) __attribute__((__noreturn__)):	
 *	copied to VBR+0x100. This code should be relocatable and max 384
 *	instructions.	
 *		0x40 TLB miss (load)
 *		0x60 TLB miss (store)
 *		0xc0	TLB protection (store)
 *		  -> tlbmiss_exp()
 *		0xa0 TLB protection (load)
 *		0x80 Initial page write.
 *		 and other...
 *		  -> trap()
 */		
	.globl	_C_LABEL(sh_vector_generic), _C_LABEL(sh_vector_generic_end)
	.align	2
_C_LABEL(sh_vector_generic):
	EXCEPTION_ENTRY
	/* Identify exception cause */
	MOV	(EXPEVT, r0)
	mov.l	@r0,	r0
	/*
	 * TLB exception.
	 */
	cmp/eq	#0x40,	r0		/* T_TLBINVALIDR */
	bf	1f
3:
	__INTR_MASK_r0_r1
	__EXCEPTION_UNBLOCK_r0_r1
	mov.l	_L.tlb_handler, r0
	jsr	@r0
	 nop
	bra	4f
	 nop
1:
	cmp/eq	#0x60,	r0		/* T_TLBINVALIDW */
	bt	3b

	mov.l	_L.TLBPROTWR, r1	/* T_TLBPRIVW */
	cmp/eq	r0,	r1
	bt	3b
	
	/*
	 * General exception.
	 */	
#ifdef DDB
	mov	#0,	r2
	MOV	(BBRA, r1)
	mov.w	r2,	@r1	/* disable UBC */
	mov.l	r2,	@(TF_UBC, r15)	/* clear trapframe->tf_ubc */
#endif /* DDB */
	
	mov.l	r0,	@(TF_TRAPNO, r15) /* trapframe->tf_trapno = EXPEVT */
	__INTR_UNMASK_r0_r1
	__EXCEPTION_UNBLOCK_r0_r1
	mov.l	_L.trap, r0
	jsr	@r0
	 mov	r15,	r4

	/* Check for ASTs on exit to user mode. */
	mov.l	_L.ast,	r0
	jsr	@r0
	 mov	r15,	r4		

#ifdef DDB	/* BBRA = trapframe->tf_ubc */	
	__EXCEPTION_BLOCK_r0_r1
	mov.l	@(TF_UBC, r15), r0
	MOV	(BBRA, r1)
	mov.w	r0,	@r1
#endif /* DDB */
4:	
	EXCEPTION_RETURN
	/* NOTREACHED */
	.align	2
REG_SYMBOL(EXPEVT)
REG_SYMBOL(BBRA)
_L.TLBPROTWR:	.long	0x000000c0
_L.trap:	.long	_C_LABEL(trap)
_L.ast:		.long	_C_LABEL(ast)
_L.tlb_handler:	.long	_C_LABEL(tlb_handler)
_C_LABEL(sh_vector_generic_end):
	
/*	
 * void sh_vector_tlbmiss(void) __attribute__((__noreturn__)):	
 *	copied to VBR+0x400. This code should be relocatable and max 256
 *	instructions.
 */
	.globl	_C_LABEL(sh_vector_tlbmiss), _C_LABEL(sh_vector_tlbmiss_end)
	.align	2	
_C_LABEL(sh_vector_tlbmiss):
	EXCEPTION_ENTRY	
	__INTR_MASK_r0_r1
	__EXCEPTION_UNBLOCK_r0_r1
	mov.l	1f,	r0
	jsr	@r0
	 nop
	EXCEPTION_RETURN
	.align	2
1:	.long	_C_LABEL(tlb_handler)
_C_LABEL(sh_vector_tlbmiss_end):


/*
 * void sh_vector_interrupt(void) __attribute__((__noreturn__)):	
 *	copied to VBR+0x600. This code should be relocatable.
 */        	 	 	
	.globl	_C_LABEL(sh_vector_interrupt), _C_LABEL(sh_vector_interrupt_end)
	.align	2
_C_LABEL(sh_vector_interrupt):
	EXCEPTION_ENTRY
	mov.l	1f,	r0
	jmp	@r0
	 nop
	.align	2
1:	.long	_C_LABEL(interrupt_exp)
_C_LABEL(sh_vector_interrupt_end):

