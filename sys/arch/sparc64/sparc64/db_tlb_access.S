/*	$NetBSD: db_tlb_access.S,v 1.2.24.1 2017/12/03 11:36:45 jdolecek Exp $	*/

/*
 * Copyright (c) 1996-2002 Eduardo Horvath
 * Copyright (c) 2010 Matthew R. Green
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR  ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR  BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <machine/ctlreg.h>
#include <machine/asm.h>

/*
 * Basic routines to print the I/D-TLB tag & data info.
 */

#ifdef _LP64
ENTRY_NOPROFILE(print_dtlb)
	save	%sp, -CC64FSZ, %sp
	sll	%i0, 3, %l3
	or	%l3, %i1, %l3
	mov	%i1, %l1
	clr	%l2
1:
	ldxa	[%l1] ASI_DMMU_TLB_TAG, %o2
	membar	#Sync
	mov	%l2, %o1
	ldxa	[%l1] ASI_DMMU_TLB_DATA, %o3
	membar	#Sync
	inc	%l2
	set	2f, %o0
	call	_C_LABEL(db_printf)
	 inc	8, %l1

	ldxa	[%l1] ASI_DMMU_TLB_TAG, %o2
	membar	#Sync
	mov	%l2, %o1
	ldxa	[%l1] ASI_DMMU_TLB_DATA, %o3
	membar	#Sync
	inc	%l2
	set	3f, %o0
	call	_C_LABEL(db_printf)
	 inc	8, %l1

	cmp	%l1, %l3
	bl	1b
	 inc	8, %l0

	ret
	 restore

ENTRY_NOPROFILE(print_itlb)
	save	%sp, -CC64FSZ, %sp
	sll	%i0, 3, %l3
	or	%l3, %i1, %l3
	mov	%i1, %l1
	clr	%l2
1:
	ldxa	[%l1] ASI_IMMU_TLB_TAG, %o2
	membar	#Sync
	mov	%l2, %o1
	ldxa	[%l1] ASI_IMMU_TLB_DATA, %o3
	membar	#Sync
	inc	%l2
	set	2f, %o0
	call	_C_LABEL(db_printf)
	 inc	8, %l1

	ldxa	[%l1] ASI_IMMU_TLB_TAG, %o2
	membar	#Sync
	mov	%l2, %o1
	ldxa	[%l1] ASI_IMMU_TLB_DATA, %o3
	membar	#Sync
	inc	%l2
	set	3f, %o0
	call	_C_LABEL(db_printf)
	 inc	8, %l1

	cmp	%l1, %l3
	bl	1b
	 inc	8, %l0

	ret
	 restore

	.data
2:
	.asciz	"%2d:%016lx %016lx "
3:
	.asciz	"%2d:%016lx %016lx\n"
	.text
#else
ENTRY_NOPROFILE(print_dtlb)
	save	%sp, -CC64FSZ, %sp
	sll	%i0, 3, %l3
	or	%l3, %i1, %l3
	mov	%i1, %l1
	clr	%l2
1:
	ldxa	[%l1] ASI_DMMU_TLB_TAG, %o2
	membar	#Sync
	srl	%o2, 0, %o3
	mov	%l2, %o1
	srax	%o2, 32, %o2
	ldxa	[%l1] ASI_DMMU_TLB_DATA, %o4
	membar	#Sync
	srl	%o4, 0, %o5
	inc	%l2
	srax	%o4, 32, %o4
	set	2f, %o0
	call	_C_LABEL(db_printf)
	 inc	8, %l1

	ldxa	[%l1] ASI_DMMU_TLB_TAG, %o2
	membar	#Sync
	srl	%o2, 0, %o3
	mov	%l2, %o1
	srax	%o2, 32, %o2
	ldxa	[%l1] ASI_DMMU_TLB_DATA, %o4
	membar	#Sync
	srl	%o4, 0, %o5
	inc	%l2
	srax	%o4, 32, %o4
	set	3f, %o0
	call	_C_LABEL(db_printf)
	 inc	8, %l1

	cmp	%l1, %l3
	bl	1b
	 inc	8, %l0

	ret
	 restore

ENTRY_NOPROFILE(print_itlb)
	save	%sp, -CC64FSZ, %sp
	sll	%i0, 3, %l3
	or	%l3, %i1, %l3
	mov	%i1, %l1
	clr	%l2
1:
	ldxa	[%l1] ASI_IMMU_TLB_TAG, %o2
	membar	#Sync
	srl	%o2, 0, %o3
	mov	%l2, %o1
	srax	%o2, 32, %o2
	ldxa	[%l1] ASI_IMMU_TLB_DATA, %o4
	membar	#Sync
	srl	%o4, 0, %o5
	inc	%l2
	srax	%o4, 32, %o4
	set	2f, %o0
	call	_C_LABEL(db_printf)
	 inc	8, %l1

	ldxa	[%l1] ASI_IMMU_TLB_TAG, %o2
	membar	#Sync
	srl	%o2, 0, %o3
	mov	%l2, %o1
	srax	%o2, 32, %o2
	ldxa	[%l1] ASI_IMMU_TLB_DATA, %o4
	membar	#Sync
	srl	%o4, 0, %o5
	inc	%l2
	srax	%o4, 32, %o4
	set	3f, %o0
	call	_C_LABEL(db_printf)
	 inc	8, %l1

	cmp	%l1, %l3
	bl	1b
	 inc	8, %l0

	ret
	 restore

	.data
2:
	.asciz	"%2d:%08x:%08x %08x:%08x "
3:
	.asciz	"%2d:%08x:%08x %08x:%08x\n"
	.text
#endif
