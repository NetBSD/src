/*	$NetBSD: exception_vector.S,v 1.3 2002/03/17 14:02:03 uch Exp $	*/

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
	
#include <sh3/param.h>
#include <sh3/asm.h>
#include <sh3/locore.h>
#include <sh3/trapreg.h>	
#include <sh3/ubcreg.h>
#include <sh3/mmu_sh3.h>
#include <sh3/mmu_sh4.h>

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
	.text	
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
	__INTR_MASK(r0, r1)
	__EXCEPTION_UNBLOCK(r0, r1)
	mov.l	_L.tlb_handler, r0
	jsr	@r0
	 mov	r14,	r6
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
	mov.l	r2,	@(TF_UBC, r14)	/* clear trapframe->tf_ubc */
#endif /* DDB */

	mov.l	r0,	@(TF_TRAPNO, r14) /* trapframe->tf_trapno = EXPEVT */
	__INTR_UNMASK(r0, r1)
	__EXCEPTION_UNBLOCK(r0, r1)
	mov.l	_L.trap, r0
	jsr	@r0
	 mov	r14,	r6

	/* Check for ASTs on exit to user mode. */
	mov.l	_L.ast,	r0
	jsr	@r0
	 mov	r14,	r4

#ifdef DDB	/* BBRA = trapframe->tf_ubc */	
	__EXCEPTION_BLOCK(r0, r1)
	mov.l	@(TF_UBC, r14), r0
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
_C_LABEL(sh_vector_generic_end):	.long	0
	
#ifdef SH3
/*	
 * void sh3_vector_tlbmiss(void) __attribute__((__noreturn__)):	
 *	copied to VBR+0x400. This code should be relocatable and max 256
 *	instructions.
 */
	.globl	_C_LABEL(sh3_vector_tlbmiss), _C_LABEL(sh3_vector_tlbmiss_end)
	.text	
	.align	2
_C_LABEL(sh3_vector_tlbmiss):
	EXCEPTION_ENTRY	
	__INTR_MASK(r0, r1)
	__EXCEPTION_UNBLOCK(r0, r1)
#if !defined(P1_STACK)
	/* Load kerne stack */
	mov.l	_L.TEA, r1
	mov.l	@r1,	r0	/* r0 = va */
	mov.l	_L.VPN_MASK, r1
	and	r1,	r0	/* va = trunc_page(va) */
	mov.l	_L.CURUPTE, r1
	mov	#UPAGES,r3
	mov	#1,	r2
4:	mov.l	@r1+,	r6
	cmp/eq	r6,	r0	/* md_upte.addr: u-area VPN */	
	bt	5f
	add	#4,	r1	/* skip md_upte.data */
	cmp/eq	r2,	r3
	bf/s	4b
	 add	#1,	r2		
	bra	6f
	 nop
5:	mov.l	@r1,	r2	/* md_upte.data: u-area PTE */
	mov.l	_L.PTEL, r1
	mov.l	r2,	@r1
	ldtlb
	bra	3f
	 nop
#endif /* !P1_STACK */
6:	mov	r0,	r2
	mov	#-22,	r1
	shld	r1,	r2	/* r2 = va >> 22 */
	shll2	r2		/* r2 *= sizeof(pt_entry_t) */
	mov.l	_L.TTB, r1
	mov.l	@r1,	r1
	add	r1,	r2	/* r2 = page directory entry address */
	mov	#1	r1
	swap.b	r1,	r1	/* 0x100 (PG_V) */
	mov.l	@r2,	r2	/* r2 = pde */
	and	r2,	r1
	tst	r1,	r1
	bt	2f		/* (pde & PG_V) == 0 -> tlb_handler */
	mov.l	_L.VPN_MASK, r1
	and	r1,	r2	/* zero attribute bits */
	mov.l	_L.PT_MASK, r1
	mov	r0,	r3
	and	r1,	r3	/* r3 = va & 0x003ff000 */
	mov	#-12	r1
	shld	r1,	r3	/* r3 = (va & 0x003ff000) >> 12 */
	shll2	r3		/* r3 *= sizeof(pt_entry_t) */
	add	r2,	r3	/* r3 = page table entry address */
	mov.l	@r3,	r3	/* r3 = page table entry */
	mov	#1,	r1
	swap.b	r1,	r1	/* r1 = PG_V */
	and	r3,	r1
	tst	r1,	r1
	bt	2f		/* (pte & PG_V) == 0 -> tlb_handler */
	mov.l	_L.PTE_HW_BITS, r2	
	and	r2,	r3	
	mov.l	_L.PTEL, r1
	mov.l	r3,	@r1	/* PTEL = (pte & PG_HW_BITS) */
	ldtlb
	bra	3f
	 nop
2:
	mov.l	1f,	r0
	jsr	@r0
	 mov	r14,	r6
3:	
	EXCEPTION_RETURN
	.align	2
1:		.long	_C_LABEL(tlb_handler)		
_L.TEA:		.long	SH3_TEA
_L.TTB:		.long	SH3_TTB
_L.PTEL:	.long	SH3_PTEL
_L.PTE_HW_BITS:	.long	0x1ffff17e
_L.PT_MASK:	.long	0x003ff000	
_L.VPN_MASK:	.long	0xfffff000
_L.CURUPTE:	.long	_C_LABEL(curupte)
	.align	2
_C_LABEL(sh3_vector_tlbmiss_end):	.long	0
#endif /* SH3 */

#ifdef SH4
/*	
 * void sh4_vector_tlbmiss(void) __attribute__((__noreturn__)):	
 *	copied to VBR+0x400. This code should be relocatable and max 256
 *	instructions.
 */
	.globl	_C_LABEL(sh4_vector_tlbmiss), _C_LABEL(sh4_vector_tlbmiss_end)
	.text	
	.align	2
_C_LABEL(sh4_vector_tlbmiss):
	EXCEPTION_ENTRY	
	__INTR_MASK(r0, r1)
	__EXCEPTION_UNBLOCK(r0, r1)
	mov	#0x20,	r7
	swap.b	r7,	r7
	swap.w	r7,	r7	/* r7 = 0x20000000 */
	mov.l	__L.TEA, r1
	mov.l	@r1,	r0	/* r0 = va */
	mov	r0,	r2
	mov	#-22,	r1
	shld	r1,	r2	/* r2 = va >> 22 */
	shll2	r2		/* r2 *= sizeof(pt_entry_t) */
	mov.l	__L.TTB, r1
	mov.l	@r1,	r1
	or	r7,	r1	/* XXX P2 access */
	add	r1,	r2	/* r2 = page directory entry address */
	mov	#1	r1
	swap.b	r1,	r1	/* 0x100 (PG_V) */
	mov.l	@r2,	r2	/* r2 = pde */
	and	r2,	r1
	tst	r1,	r1
	bt	2f		/* (pde & PG_V) == 0 -> tlb_handler */
	mov.l	__L.VPN_MASK, r1
	and	r1,	r2	/* zero attribute bits */
	mov.l	__L.PT_MASK, r1
	mov	r0,	r3
	and	r1,	r3	/* r3 = va & 0x003ff000 */
	mov	#-12	r1
	shld	r1,	r3	/* r3 = (va & 0x003ff000) >> 12 */
	shll2	r3		/* r3 *= sizeof(pt_entry_t) */
	add	r2,	r3	/* r3 = page table entry address */
	or	r7,	r3	/* XXX P2 access */
	mov.l	@r3,	r3	/* r3 = page table entry */
	mov	#1,	r1
	swap.b	r1,	r1	/* r1 = PG_V */
	and	r3,	r1
	tst	r1,	r1
	bt	2f		/* (pte & PG_V) == 0 -> tlb_handler */
	mov	#0xe,	r1
	swap.b	r1,	r1	/* r1 = _PG_PCMCIA (0x0e00) */
	and	r3,	r1
	tst	r1,	r1
	bt	4f
	mov	r3,	r2
	mov	#-9	r1
	shld	r1,	r2
	mov	#7,	r1
	and	r1,	r2	/* r2 = (pte >> 9) & 0x7 */
	mov.l	__L.PTEA, r1
	mov.l	r2,	@r1
	mov.l	__L.PTE_HW_BITS, r1
	and	r3,	r1
	mov	#-9,	r2
	and	r2,	r3	/* pte &= ~PG_N XXX */
	mov.l	__L.PTEL, r1
	mov.l	r3,	@r1
	bra	6f
	 nop
4:
	mov.l	__L.PTE_HW_BITS, r1
	and	r1,	r3
	mov.l	__L.P3SEGBASE, r1
	cmp/hs	r1,	r0	/* va >= 0xc0000000 ? T = 1 */
	bf	5f
	mov	#1,	r1
	or	r1,	r3	/* PG_WT  P3 write-through XXX */
5:
	mov.l	__L.PTEA, r1
	xor	r0,	r0
	mov.l	r0,	@r1
	mov.l	__L.PTEL, r1
	mov.l	r3,	@r1
	nop
6:	ldtlb
	bra	3f
	 nop
2:
	mov.l	1f,	r0
	jsr	@r0
	 mov	r14,	r6
3:	
	EXCEPTION_RETURN
	.align	2
1:		.long	_C_LABEL(tlb_handler)		
__L.TEA:	.long	SH4_TEA
__L.TTB:	.long	SH4_TTB
__L.PTEL:	.long	SH4_PTEL
__L.PTEA:	.long	SH4_PTEA
__L.P3SEGBASE:	.long	0xc0000000
__L.PTE_HW_BITS:.long	0x1ffff17e
__L.PT_MASK:	.long	0x003ff000	
__L.VPN_MASK:	.long	0xfffff000
_C_LABEL(sh4_vector_tlbmiss_end):	.long	0
#endif /* SH4 */

/*
 * void sh_vector_interrupt(void) __attribute__((__noreturn__)):	
 *	copied to VBR+0x600. This code should be relocatable.
 */        	 	 	
	.globl	_C_LABEL(sh_vector_interrupt), _C_LABEL(sh_vector_interrupt_end)
	.align	2
	.text
_C_LABEL(sh_vector_interrupt):
	EXCEPTION_ENTRY
	mov.l	1f,	r0
	jmp	@r0
	 nop
	.align	2
1:	.long	_C_LABEL(interrupt_exp)
_C_LABEL(sh_vector_interrupt_end):	.long	0
