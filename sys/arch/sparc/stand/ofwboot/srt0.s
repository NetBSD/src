/*	$NetBSD: srt0.s,v 1.6.84.1 2015/09/22 12:05:52 skrll Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/psl.h>
#include <machine/param.h>
#include <machine/frame.h>
#include <machine/asm.h>
#include <machine/ctlreg.h>


#ifdef _LP64
#define	LDPTR		ldx
#else
#define	LDPTR		lduw
#endif


	.register %g2,#ignore
	.register %g3,#ignore

/*
 * Globals
 */
	.globl	_esym
	.data
_esym:	.word	0			/* end of symbol table */

/*
 * Startup entry
 */
	.text
	.globl	_start, _C_LABEL(kernel_text)
	_C_LABEL(kernel_text) = _start
_start:
	nop			! For some reason this is needed to fixup the text section

	/*
	 * Start by creating a stack for ourselves.
	 */
#ifdef _LP64
	/* 64-bit stack */
	btst	1, %sp
	set	CC64FSZ, %g1	! Frame Size (negative)
	bnz	1f
	 set	BIAS, %g2	! Bias (negative)
	andn	%sp, 0x0f, %sp	! 16 byte align, per ELF spec.
	add	%g1, %g2, %g1	! Frame + Bias
1:
	sub	%sp, %g1, %g1
	save	%g1, %g0, %sp
#else
	/* 32-bit stack */
	btst	1, %sp
	set	CC64FSZ, %g1	! Frame Size (negative)
	bz	1f
	 set	BIAS, %g2
	sub	%g1, %g2, %g1
1:
	sub	%sp, %g1, %g1	! This is so we properly sign-extend things
	andn	%g1, 0x7, %g1
	save	%g1, %g0, %sp
#endif

	/*
	 * Set the psr into a known state:
	 * Set supervisor mode, interrupt level >= 13, traps enabled
	 */
	wrpr	%g0, 0, %pil	! So I lied
	wrpr	%g0, PSTATE_PRIV+PSTATE_IE, %pstate

	clr	%g4		! Point %g4 to start of data segment
				! only problem is that apparently the
				! start of the data segment is 0

	/*
	 * void
	 * main(void *openfirmware)
	 */
	call	_C_LABEL(main)
	 mov	%i4, %o0
	call	_C_LABEL(OF_exit)
	 nop

/*
 * void syncicache(void* start, int size)
 *
 * I$ flush.  Really simple.  Just flush over the whole range.
 */
	.align	8
	.globl	_C_LABEL(syncicache)
_C_LABEL(syncicache):
	dec	4, %o1
	flush	%o0
	brgz,a,pt	%o1, _C_LABEL(syncicache)
	 inc	4, %o0
	retl
	 nop
	
/*
 * openfirmware(cell* param);
 *
 * OpenFirmware entry point 
 * 
 * If we're running in 32-bit mode we need to convert to a 64-bit stack
 * and 64-bit cells.  The cells we'll allocate off the stack for simplicity.
 */
	.align 8
	.globl	_C_LABEL(openfirmware)
	FTYPE(openfirmware)
_C_LABEL(openfirmware):
	andcc	%sp, 1, %g0
	bz,pt	%icc, 1f
	 sethi	%hi(_C_LABEL(romp)), %o1
	
	LDPTR	[%o1+%lo(_C_LABEL(romp))], %o4		! v9 stack, just load the addr and callit
	save	%sp, -CC64FSZ, %sp
	mov	%i0, %o0				! Copy over our parameter
	mov	%g1, %l1
	mov	%g2, %l2
	mov	%g3, %l3
	mov	%g4, %l4
	mov	%g5, %l5
	mov	%g6, %l6
	mov	%g7, %l7
	rdpr	%pstate, %l0
	jmpl	%i4, %o7
	 wrpr	%g0, PSTATE_PROM|PSTATE_IE, %pstate
	wrpr	%l0, %g0, %pstate
	mov	%l1, %g1
	mov	%l2, %g2
	mov	%l3, %g3
	mov	%l4, %g4
	mov	%l5, %g5
	mov	%l6, %g6
	mov	%l7, %g7
	ret
	 restore	%o0, %g0, %o0

1:						! v8 -- need to screw with stack & params
	save	%sp, -CC64FSZ, %sp			! Get a new 64-bit stack frame
	add	%sp, -BIAS, %sp
	sethi	%hi(_C_LABEL(romp)), %o1
	rdpr	%pstate, %l0
	LDPTR	[%o1+%lo(_C_LABEL(romp))], %o1		! Do the actual call
	srl	%sp, 0, %sp
	mov	%i0, %o0
	mov	%g1, %l1
	mov	%g2, %l2
	mov	%g3, %l3
	mov	%g4, %l4
	mov	%g5, %l5
	mov	%g6, %l6
	mov	%g7, %l7
	jmpl	%o1, %o7
	 wrpr	%g0, PSTATE_PROM|PSTATE_IE, %pstate	! Enable 64-bit addresses for the prom
	wrpr	%l0, 0, %pstate
	mov	%l1, %g1
	mov	%l2, %g2
	mov	%l3, %g3
	mov	%l4, %g4
	mov	%l5, %g5
	mov	%l6, %g6
	mov	%l7, %g7
	ret
	 restore	%o0, %g0, %o0

/*
 * vaddr_t
 * itlb_va_to_pa(vaddr_t)
 *
 * Find out if there is a mapping in iTLB for a given virtual address,
 * return -1 if there is none.
 */
	.align	8
	.globl	_C_LABEL(itlb_va_to_pa)
_C_LABEL(itlb_va_to_pa):
	set	_C_LABEL(itlb_slot_max), %o3
	ld	[%o3], %o3
	dec	%o3
	sllx	%o3, 3, %o3
	clr	%o1
0:	ldxa	[%o1] ASI_IMMU_TLB_TAG, %o2
	cmp	%o2, %o0
	bne,a	%xcc, 1f
	 nop
	/* return PA of matching entry */
	ldxa	[%o1] ASI_IMMU_TLB_DATA, %o0
	sllx	%o0, 23, %o0
	srlx	%o0, PGSHIFT+23, %o0
	sllx	%o0, PGSHIFT, %o0
	retl
	 mov	%o0, %o1
1:	cmp	%o1, %o3
	blu	%xcc, 0b
	 add	%o1, 8, %o1
	clr	%o0
	retl
	 not	%o0

/*
 * vaddr_t
 * dtlb_va_to_pa(vaddr_t)
 *
 * Find out if there is a mapping in dTLB for a given virtual address,
 * return -1 if there is none.
 */
	.align	8
	.globl	_C_LABEL(dtlb_va_to_pa)
_C_LABEL(dtlb_va_to_pa):
	set	_C_LABEL(dtlb_slot_max), %o3
	ld	[%o3], %o3
	dec	%o3
	sllx	%o3, 3, %o3
	clr	%o1
0:	ldxa	[%o1] ASI_DMMU_TLB_TAG, %o2
	cmp	%o2, %o0
	bne,a	%xcc, 1f
	 nop
	/* return PA of matching entry */
	ldxa	[%o1] ASI_DMMU_TLB_DATA, %o0
	sllx	%o0, 23, %o0
	srlx	%o0, PGSHIFT+23, %o0
	sllx	%o0, PGSHIFT, %o0
	retl
	 mov	%o0, %o1
1:	cmp	%o1, %o3
	blu	%xcc, 0b
	 add	%o1, 8, %o1
	clr	%o0
	retl
	 not	%o0

/*
 * void
 * itlb_enter(vaddr_t vpn, uint32_t data_hi, uint32_t data_lo)
 *
 * Insert new mapping into iTLB. Data tag is passed in two different
 * registers so that it works even with 32-bit compilers.
 */
	.align	8
	.globl	_C_LABEL(itlb_enter)
_C_LABEL(itlb_enter):
	sllx	%o1, 32, %o1
	or	%o1, %o2, %o1
	rdpr	%pstate, %o4
	wrpr	%o4, PSTATE_IE, %pstate
	mov	TLB_TAG_ACCESS, %o3
	stxa	%o0, [%o3] ASI_IMMU
	stxa	%o1, [%g0] ASI_IMMU_DATA_IN
	membar	#Sync
	retl
	 wrpr	%o4, 0, %pstate


/*
 * void
 * dtlb_replace(vaddr_t vpn, uint32_t data_hi, uint32_t data_lo)
 *
 * Replace mapping in dTLB. Data tag is passed in two different
 * registers so that it works even with 32-bit compilers.
 */
	.align	8
	.globl	_C_LABEL(dtlb_replace)
_C_LABEL(dtlb_replace):
	sllx	%o1, 32, %o1
	or	%o1, %o2, %o1
	rdpr	%pstate, %o4
	wrpr	%o4, PSTATE_IE, %pstate
	/* loop over dtlb entries */
	clr	%o5
0:
	ldxa	[%o5] ASI_DMMU_TLB_TAG, %o2
	cmp	%o2, %o0
	bne,a	%xcc, 1f
	 nop
	/* found - modify entry */
	mov	TLB_TAG_ACCESS, %o2
	stxa	%o0, [%o2] ASI_DMMU
	stxa	%o1, [%o5] ASI_DMMU_TLB_DATA
	membar	#Sync
	retl
	 wrpr	%o4, 0, %pstate

	/* advance to next tlb entry */
1:	cmp	%o5, 63<<3
	blu	%xcc, 0b
	 add	%o5, 8, %o5
	retl
	 nop

/*
 * void
 * dtlb_enter(vaddr_t vpn, uint32_t data_hi, uint32_t data_lo)
 *
 * Insert new mapping into dTLB. Data tag is passed in two different
 * registers so that it works even with 32-bit compilers.
 */
	.align	8
	.globl	_C_LABEL(dtlb_enter)
_C_LABEL(dtlb_enter):
	sllx	%o1, 32, %o1
	or	%o1, %o2, %o1
	rdpr	%pstate, %o4
	wrpr	%o4, PSTATE_IE, %pstate
	mov	TLB_TAG_ACCESS, %o3
	stxa	%o0, [%o3] ASI_DMMU
	stxa	%o1, [%g0] ASI_DMMU_DATA_IN
	membar	#Sync
	retl
	 wrpr	%o4, 0, %pstate

/*
 * u_int
 * get_cpuid(void);
 *
 * Return UPA identifier for the CPU we're running on.
 */
	.align	8
	.globl	_C_LABEL(get_cpuid)
_C_LABEL(get_cpuid):
	UPA_GET_MID(%o0)
	retl
	 nop

#if 0
	.data
	.align 8
bootstack:	
#define STACK_SIZE	0x14000
	.skip	STACK_SIZE
ebootstack:			! end (top) of boot stack
#endif
