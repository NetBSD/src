/*	$NetBSD: vector.S,v 1.41.2.2 2014/08/20 00:02:42 tls Exp $	*/

/*-
 * Copyright (c) 1998, 2007, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Andrew Doran.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/asm.h>

#include "opt_ddb.h"
#include "opt_multiprocessor.h"
#include "opt_xen.h"
#include "opt_dtrace.h"

#define ALIGN_TEXT	.align 16,0x90

#include <machine/i8259.h>
#include <machine/i82093reg.h>
#include <machine/i82489reg.h>
#include <machine/frameasm.h>
#include <machine/segments.h>
#include <machine/trap.h>
#include <machine/specialreg.h>

#include "ioapic.h"
#include "lapic.h"
#include "assym.h"

#include "amd64_trap.S"

/*****************************************************************************/

#define __HAVE_GENERIC_SOFT_INTERRUPTS	/* XXX */

/*
 * Macros for interrupt entry, call to handler, and exit.
 *
 * XXX
 * The interrupt frame is set up to look like a trap frame.  This may be a
 * waste.  The only handler which needs a frame is the clock handler, and it
 * only needs a few bits.  Xdoreti() needs a trap frame for handling ASTs, but
 * it could easily convert the frame on demand.
 *
 * The direct costs of setting up a trap frame are two pushq's (error code and
 * trap number), an addl to get rid of these, and pushing and popping the
 * callee-saved registers %esi, %edi, %ebx, and %ebp twice.
 *
 * If the interrupt frame is made more flexible,  INTR can push %eax first and
 * decide the ipending case with less overhead, e.g., by avoiding loading the
 * segment registers.
 *
 */

/* XXX See comment in locore.s */
#ifdef __ELF__
#define	XINTR(name,num)		Xintr_ ## name ## num
#else
#define	XINTR(name,num)		_Xintr_ ## name ## num
#endif

#if NLAPIC > 0
#ifdef MULTIPROCESSOR
IDTVEC(recurse_lapic_ipi)
	INTR_RECURSE_HWFRAME
	pushq	$0		
	pushq	$T_ASTFLT
	INTRENTRY		
	jmp	1f
IDTVEC_END(recurse_lapic_ipi)

IDTVEC(intr_lapic_ipi)
	pushq	$0		
	pushq	$T_ASTFLT
	INTRENTRY		
	movl	$0,_C_LABEL(local_apic)+LAPIC_EOI
	movl	CPUVAR(ILEVEL),%ebx
	cmpl	$IPL_HIGH,%ebx
	jae	2f
IDTVEC(resume_lapic_ipi)
1:
	incl	CPUVAR(IDEPTH)
	movl	$IPL_HIGH,CPUVAR(ILEVEL)
        sti
	pushq	%rbx
	call	_C_LABEL(x86_ipi_handler)
	jmp	_C_LABEL(Xdoreti)
2:
	orl	$(1 << LIR_IPI),CPUVAR(IPENDING)
	INTRFASTEXIT
IDTVEC_END(resume_lapic_ipi)
IDTVEC_END(intr_lapic_ipi)

#if defined(DDB)
IDTVEC(intrddb)
1:
	pushq	$0
	pushq	$T_BPTFLT
	INTRENTRY
	movl	$0xf,%eax
	movq	%rax,%cr8
	movl	$0,_C_LABEL(local_apic)+LAPIC_EOI
	sti
	call	_C_LABEL(ddb_ipi)
	xorl	%eax,%eax
	movq	%rax,%cr8
	INTRFASTEXIT
IDTVEC_END(intrddb)
#endif /* DDB */
#endif /* MULTIPROCESSOR */
	
	/*
	 * Interrupt from the local APIC timer.
	 */
IDTVEC(recurse_lapic_ltimer)
	INTR_RECURSE_HWFRAME
	pushq	$0		
	pushq	$T_ASTFLT
	INTRENTRY		
	jmp	1f
IDTVEC_END(recurse_lapic_ltimer)

IDTVEC(intr_lapic_ltimer)
	pushq	$0		
	pushq	$T_ASTFLT
	INTRENTRY		
	movl	$0,_C_LABEL(local_apic)+LAPIC_EOI
	movl	CPUVAR(ILEVEL),%ebx
	cmpl	$IPL_CLOCK,%ebx
	jae	2f
IDTVEC(resume_lapic_ltimer)
1:
	incl	CPUVAR(IDEPTH)
	movl	$IPL_CLOCK,CPUVAR(ILEVEL)
	sti
	pushq	%rbx
	movq	%rsp,%rsi
	xorq	%rdi,%rdi
	call	_C_LABEL(lapic_clockintr)
	jmp	_C_LABEL(Xdoreti)
2:
	orl	$(1 << LIR_TIMER),CPUVAR(IPENDING)
	INTRFASTEXIT
IDTVEC_END(resume_lapic_ltimer)
IDTVEC_END(intr_lapic_ltimer)
#endif /* NLAPIC > 0 */

#ifndef XEN
/*
 * TLB shootdown handler.
 */
IDTVEC(intr_lapic_tlb)
	pushq	$0
	pushq	$T_ASTFLT
	INTRENTRY
	movl	$0, _C_LABEL(local_apic)+LAPIC_EOI
	callq	_C_LABEL(pmap_tlb_intr)
	INTRFASTEXIT
IDTVEC_END(intr_lapic_tlb)

#endif /* !XEN */

#define voidop(num)

#ifndef XEN

/*
 * This macro defines the generic stub code. Its arguments modifiy it
 * for specific PICs.
 */

#define	INTRSTUB(name, num, early_ack, late_ack, mask, unmask, level_mask) \
IDTVEC(recurse_ ## name ## num)						;\
	INTR_RECURSE_HWFRAME						;\
	subq	$8,%rsp							;\
	pushq	$T_ASTFLT		/* trap # for doing ASTs */	;\
	INTRENTRY							;\
IDTVEC(resume_ ## name ## num)						\
	movq	$IREENT_MAGIC,TF_ERR(%rsp)				;\
	movl	%ebx,%r13d						;\
	movq	CPUVAR(ISOURCES) + (num) * 8, %r14			;\
	movl	IS_MAXLEVEL(%r14),%ebx					;\
	jmp	1f							;\
IDTVEC(intr_ ## name ## num)						;\
	pushq	$0			/* dummy error code */		;\
	pushq	$T_ASTFLT		/* trap # for doing ASTs */	;\
	INTRENTRY							;\
	movq	CPUVAR(ISOURCES) + (num) * 8, %r14			;\
	mask(num)			/* mask it in hardware */	;\
	early_ack(num)			/* and allow other intrs */	;\
	testq	%r14,%r14						;\
	jz	9f			/* stray */			;\
	movl	IS_MAXLEVEL(%r14),%ebx					;\
	movl	CPUVAR(ILEVEL),%r13d					;\
	cmpl	%ebx,%r13d						;\
	jae	10f			/* currently masked; hold it */	;\
	incq	CPUVAR(NINTR)		/* statistical info */		;\
	incq	IS_EVCNT(%r14)						;\
1:									\
	pushq	%r13			/* save for Xdoreti */		;\
	movl	%ebx,CPUVAR(ILEVEL)					;\
	sti								;\
	incl	CPUVAR(IDEPTH)						;\
	movq	IS_HANDLERS(%r14),%rbx					;\
6:									\
	movl	IH_LEVEL(%rbx),%r12d					;\
	cmpl	%r13d,%r12d						;\
	jle	7f							;\
	movq	%rsp,%rsi						;\
	movq	IH_ARG(%rbx),%rdi					;\
	movl	%r12d,CPUVAR(ILEVEL)					;\
	call	*IH_FUN(%rbx)		/* call it */			;\
	movq	IH_NEXT(%rbx),%rbx	/* next handler in chain */	;\
	testq	%rbx,%rbx						;\
	jnz	6b							;\
5:									\
	cli								;\
	unmask(num)			/* unmask it in hardware */	;\
	late_ack(num)							;\
	sti								;\
	jmp	_C_LABEL(Xdoreti)	/* lower spl and do ASTs */	;\
7:									\
	cli								;\
	orl     $(1 << num),CPUVAR(IPENDING)				;\
	level_mask(num)							;\
	late_ack(num)							;\
	sti								;\
	jmp	_C_LABEL(Xdoreti)	/* lower spl and do ASTs */	;\
10:									\
	cli								;\
	orl     $(1 << num),CPUVAR(IPENDING)				;\
	level_mask(num)							;\
	late_ack(num)							;\
	INTRFASTEXIT							;\
9:									\
	unmask(num)							;\
	late_ack(num)							;\
	INTRFASTEXIT

#define ICUADDR IO_ICU1

INTRSTUB(legacy,0,i8259_asm_ack1,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,1,i8259_asm_ack1,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,2,i8259_asm_ack1,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,3,i8259_asm_ack1,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,4,i8259_asm_ack1,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,5,i8259_asm_ack1,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,6,i8259_asm_ack1,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,7,i8259_asm_ack1,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
#undef ICUADDR
#define ICUADDR IO_ICU2

INTRSTUB(legacy,8,i8259_asm_ack2,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,9,i8259_asm_ack2,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,10,i8259_asm_ack2,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,11,i8259_asm_ack2,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,12,i8259_asm_ack2,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,13,i8259_asm_ack2,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,14,i8259_asm_ack2,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,15,i8259_asm_ack2,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)

#if NIOAPIC > 0

INTRSTUB(ioapic_edge,0,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,1,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,2,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,3,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,4,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,5,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,6,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,7,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,8,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,9,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,10,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,11,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,12,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,13,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,14,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,15,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,16,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,17,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,18,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,19,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,20,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,21,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,22,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,23,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,24,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,25,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,26,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,27,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,28,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,29,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,30,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,31,voidop,ioapic_asm_ack,voidop,voidop,voidop)

INTRSTUB(ioapic_level,0,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,1,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,2,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,3,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,4,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,5,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,6,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,7,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,8,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,9,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,10,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,11,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,12,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,13,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,14,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,15,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,16,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,17,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,18,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,19,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,20,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,21,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,22,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,23,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,24,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,25,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,26,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,27,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,28,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,29,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,30,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,31,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)

#endif

	.type _C_LABEL(i8259_stubs), @object
LABEL(i8259_stubs)
	.quad _C_LABEL(Xintr_legacy0), _C_LABEL(Xrecurse_legacy0)
	.quad _C_LABEL(Xresume_legacy0)
	.quad _C_LABEL(Xintr_legacy1), _C_LABEL(Xrecurse_legacy1)
	.quad _C_LABEL(Xresume_legacy1)
	.quad _C_LABEL(Xintr_legacy2), _C_LABEL(Xrecurse_legacy2)
	.quad _C_LABEL(Xresume_legacy2)
	.quad _C_LABEL(Xintr_legacy3), _C_LABEL(Xrecurse_legacy3)
	.quad _C_LABEL(Xresume_legacy3)
	.quad _C_LABEL(Xintr_legacy4), _C_LABEL(Xrecurse_legacy4)
	.quad _C_LABEL(Xresume_legacy4)
	.quad _C_LABEL(Xintr_legacy5), _C_LABEL(Xrecurse_legacy5)
	.quad _C_LABEL(Xresume_legacy5)
	.quad _C_LABEL(Xintr_legacy6), _C_LABEL(Xrecurse_legacy6)
	.quad _C_LABEL(Xresume_legacy6)
	.quad _C_LABEL(Xintr_legacy7), _C_LABEL(Xrecurse_legacy7)
	.quad _C_LABEL(Xresume_legacy7)
	.quad _C_LABEL(Xintr_legacy8), _C_LABEL(Xrecurse_legacy8)
	.quad _C_LABEL(Xresume_legacy8)
	.quad _C_LABEL(Xintr_legacy9), _C_LABEL(Xrecurse_legacy9)
	.quad _C_LABEL(Xresume_legacy9)
	.quad _C_LABEL(Xintr_legacy10), _C_LABEL(Xrecurse_legacy10)
	.quad _C_LABEL(Xresume_legacy10)
	.quad _C_LABEL(Xintr_legacy11), _C_LABEL(Xrecurse_legacy11)
	.quad _C_LABEL(Xresume_legacy11)
	.quad _C_LABEL(Xintr_legacy12), _C_LABEL(Xrecurse_legacy12)
	.quad _C_LABEL(Xresume_legacy12)
	.quad _C_LABEL(Xintr_legacy13), _C_LABEL(Xrecurse_legacy13)
	.quad _C_LABEL(Xresume_legacy13)
	.quad _C_LABEL(Xintr_legacy14), _C_LABEL(Xrecurse_legacy14)
	.quad _C_LABEL(Xresume_legacy14)
	.quad _C_LABEL(Xintr_legacy15), _C_LABEL(Xrecurse_legacy15)
	.quad _C_LABEL(Xresume_legacy15)
END(i8259_stubs)

#if NIOAPIC > 0
	.type _C_LABEL(ioapic_edge_stubs), @object
LABEL(ioapic_edge_stubs)
	.quad _C_LABEL(Xintr_ioapic_edge0), _C_LABEL(Xrecurse_ioapic_edge0)
	.quad _C_LABEL(Xresume_ioapic_edge0)
	.quad _C_LABEL(Xintr_ioapic_edge1), _C_LABEL(Xrecurse_ioapic_edge1)
	.quad _C_LABEL(Xresume_ioapic_edge1)
	.quad _C_LABEL(Xintr_ioapic_edge2), _C_LABEL(Xrecurse_ioapic_edge2)
	.quad _C_LABEL(Xresume_ioapic_edge2)
	.quad _C_LABEL(Xintr_ioapic_edge3), _C_LABEL(Xrecurse_ioapic_edge3)
	.quad _C_LABEL(Xresume_ioapic_edge3)
	.quad _C_LABEL(Xintr_ioapic_edge4), _C_LABEL(Xrecurse_ioapic_edge4)
	.quad _C_LABEL(Xresume_ioapic_edge4)
	.quad _C_LABEL(Xintr_ioapic_edge5), _C_LABEL(Xrecurse_ioapic_edge5)
	.quad _C_LABEL(Xresume_ioapic_edge5)
	.quad _C_LABEL(Xintr_ioapic_edge6), _C_LABEL(Xrecurse_ioapic_edge6)
	.quad _C_LABEL(Xresume_ioapic_edge6)
	.quad _C_LABEL(Xintr_ioapic_edge7), _C_LABEL(Xrecurse_ioapic_edge7)
	.quad _C_LABEL(Xresume_ioapic_edge7)
	.quad _C_LABEL(Xintr_ioapic_edge8), _C_LABEL(Xrecurse_ioapic_edge8)
	.quad _C_LABEL(Xresume_ioapic_edge8)
	.quad _C_LABEL(Xintr_ioapic_edge9), _C_LABEL(Xrecurse_ioapic_edge9)
	.quad _C_LABEL(Xresume_ioapic_edge9)
	.quad _C_LABEL(Xintr_ioapic_edge10), _C_LABEL(Xrecurse_ioapic_edge10)
	.quad _C_LABEL(Xresume_ioapic_edge10)
	.quad _C_LABEL(Xintr_ioapic_edge11), _C_LABEL(Xrecurse_ioapic_edge11)
	.quad _C_LABEL(Xresume_ioapic_edge11)
	.quad _C_LABEL(Xintr_ioapic_edge12), _C_LABEL(Xrecurse_ioapic_edge12)
	.quad _C_LABEL(Xresume_ioapic_edge12)
	.quad _C_LABEL(Xintr_ioapic_edge13), _C_LABEL(Xrecurse_ioapic_edge13)
	.quad _C_LABEL(Xresume_ioapic_edge13)
	.quad _C_LABEL(Xintr_ioapic_edge14), _C_LABEL(Xrecurse_ioapic_edge14)
	.quad _C_LABEL(Xresume_ioapic_edge14)
	.quad _C_LABEL(Xintr_ioapic_edge15), _C_LABEL(Xrecurse_ioapic_edge15)
	.quad _C_LABEL(Xresume_ioapic_edge15)
	.quad _C_LABEL(Xintr_ioapic_edge16), _C_LABEL(Xrecurse_ioapic_edge16)
	.quad _C_LABEL(Xresume_ioapic_edge16)
	.quad _C_LABEL(Xintr_ioapic_edge17), _C_LABEL(Xrecurse_ioapic_edge17)
	.quad _C_LABEL(Xresume_ioapic_edge17)
	.quad _C_LABEL(Xintr_ioapic_edge18), _C_LABEL(Xrecurse_ioapic_edge18)
	.quad _C_LABEL(Xresume_ioapic_edge18)
	.quad _C_LABEL(Xintr_ioapic_edge19), _C_LABEL(Xrecurse_ioapic_edge19)
	.quad _C_LABEL(Xresume_ioapic_edge19)
	.quad _C_LABEL(Xintr_ioapic_edge20), _C_LABEL(Xrecurse_ioapic_edge20)
	.quad _C_LABEL(Xresume_ioapic_edge20)
	.quad _C_LABEL(Xintr_ioapic_edge21), _C_LABEL(Xrecurse_ioapic_edge21)
	.quad _C_LABEL(Xresume_ioapic_edge21)
	.quad _C_LABEL(Xintr_ioapic_edge22), _C_LABEL(Xrecurse_ioapic_edge22)
	.quad _C_LABEL(Xresume_ioapic_edge22)
	.quad _C_LABEL(Xintr_ioapic_edge23), _C_LABEL(Xrecurse_ioapic_edge23)
	.quad _C_LABEL(Xresume_ioapic_edge23)
	.quad _C_LABEL(Xintr_ioapic_edge24), _C_LABEL(Xrecurse_ioapic_edge24)
	.quad _C_LABEL(Xresume_ioapic_edge24)
	.quad _C_LABEL(Xintr_ioapic_edge25), _C_LABEL(Xrecurse_ioapic_edge25)
	.quad _C_LABEL(Xresume_ioapic_edge25)
	.quad _C_LABEL(Xintr_ioapic_edge26), _C_LABEL(Xrecurse_ioapic_edge26)
	.quad _C_LABEL(Xresume_ioapic_edge26)
	.quad _C_LABEL(Xintr_ioapic_edge27), _C_LABEL(Xrecurse_ioapic_edge27)
	.quad _C_LABEL(Xresume_ioapic_edge27)
	.quad _C_LABEL(Xintr_ioapic_edge28), _C_LABEL(Xrecurse_ioapic_edge28)
	.quad _C_LABEL(Xresume_ioapic_edge28)
	.quad _C_LABEL(Xintr_ioapic_edge29), _C_LABEL(Xrecurse_ioapic_edge29)
	.quad _C_LABEL(Xresume_ioapic_edge29)
	.quad _C_LABEL(Xintr_ioapic_edge30), _C_LABEL(Xrecurse_ioapic_edge30)
	.quad _C_LABEL(Xresume_ioapic_edge30)
	.quad _C_LABEL(Xintr_ioapic_edge31), _C_LABEL(Xrecurse_ioapic_edge31)
	.quad _C_LABEL(Xresume_ioapic_edge31)
END(ioapic_edge_stubs)

	.type _C_LABEL(ioapic_level_stubs), @object
LABEL(ioapic_level_stubs)
	.quad _C_LABEL(Xintr_ioapic_level0), _C_LABEL(Xrecurse_ioapic_level0)
	.quad _C_LABEL(Xresume_ioapic_level0)
	.quad _C_LABEL(Xintr_ioapic_level1), _C_LABEL(Xrecurse_ioapic_level1)
	.quad _C_LABEL(Xresume_ioapic_level1)
	.quad _C_LABEL(Xintr_ioapic_level2), _C_LABEL(Xrecurse_ioapic_level2)
	.quad _C_LABEL(Xresume_ioapic_level2)
	.quad _C_LABEL(Xintr_ioapic_level3), _C_LABEL(Xrecurse_ioapic_level3)
	.quad _C_LABEL(Xresume_ioapic_level3)
	.quad _C_LABEL(Xintr_ioapic_level4), _C_LABEL(Xrecurse_ioapic_level4)
	.quad _C_LABEL(Xresume_ioapic_level4)
	.quad _C_LABEL(Xintr_ioapic_level5), _C_LABEL(Xrecurse_ioapic_level5)
	.quad _C_LABEL(Xresume_ioapic_level5)
	.quad _C_LABEL(Xintr_ioapic_level6), _C_LABEL(Xrecurse_ioapic_level6)
	.quad _C_LABEL(Xresume_ioapic_level6)
	.quad _C_LABEL(Xintr_ioapic_level7), _C_LABEL(Xrecurse_ioapic_level7)
	.quad _C_LABEL(Xresume_ioapic_level7)
	.quad _C_LABEL(Xintr_ioapic_level8), _C_LABEL(Xrecurse_ioapic_level8)
	.quad _C_LABEL(Xresume_ioapic_level8)
	.quad _C_LABEL(Xintr_ioapic_level9), _C_LABEL(Xrecurse_ioapic_level9)
	.quad _C_LABEL(Xresume_ioapic_level9)
	.quad _C_LABEL(Xintr_ioapic_level10), _C_LABEL(Xrecurse_ioapic_level10)
	.quad _C_LABEL(Xresume_ioapic_level10)
	.quad _C_LABEL(Xintr_ioapic_level11), _C_LABEL(Xrecurse_ioapic_level11)
	.quad _C_LABEL(Xresume_ioapic_level11)
	.quad _C_LABEL(Xintr_ioapic_level12), _C_LABEL(Xrecurse_ioapic_level12)
	.quad _C_LABEL(Xresume_ioapic_level12)
	.quad _C_LABEL(Xintr_ioapic_level13), _C_LABEL(Xrecurse_ioapic_level13)
	.quad _C_LABEL(Xresume_ioapic_level13)
	.quad _C_LABEL(Xintr_ioapic_level14), _C_LABEL(Xrecurse_ioapic_level14)
	.quad _C_LABEL(Xresume_ioapic_level14)
	.quad _C_LABEL(Xintr_ioapic_level15), _C_LABEL(Xrecurse_ioapic_level15)
	.quad _C_LABEL(Xresume_ioapic_level15)
	.quad _C_LABEL(Xintr_ioapic_level16), _C_LABEL(Xrecurse_ioapic_level16)
	.quad _C_LABEL(Xresume_ioapic_level16)
	.quad _C_LABEL(Xintr_ioapic_level17), _C_LABEL(Xrecurse_ioapic_level17)
	.quad _C_LABEL(Xresume_ioapic_level17)
	.quad _C_LABEL(Xintr_ioapic_level18), _C_LABEL(Xrecurse_ioapic_level18)
	.quad _C_LABEL(Xresume_ioapic_level18)
	.quad _C_LABEL(Xintr_ioapic_level19), _C_LABEL(Xrecurse_ioapic_level19)
	.quad _C_LABEL(Xresume_ioapic_level19)
	.quad _C_LABEL(Xintr_ioapic_level20), _C_LABEL(Xrecurse_ioapic_level20)
	.quad _C_LABEL(Xresume_ioapic_level20)
	.quad _C_LABEL(Xintr_ioapic_level21), _C_LABEL(Xrecurse_ioapic_level21)
	.quad _C_LABEL(Xresume_ioapic_level21)
	.quad _C_LABEL(Xintr_ioapic_level22), _C_LABEL(Xrecurse_ioapic_level22)
	.quad _C_LABEL(Xresume_ioapic_level22)
	.quad _C_LABEL(Xintr_ioapic_level23), _C_LABEL(Xrecurse_ioapic_level23)
	.quad _C_LABEL(Xresume_ioapic_level23)
	.quad _C_LABEL(Xintr_ioapic_level24), _C_LABEL(Xrecurse_ioapic_level24)
	.quad _C_LABEL(Xresume_ioapic_level24)
	.quad _C_LABEL(Xintr_ioapic_level25), _C_LABEL(Xrecurse_ioapic_level25)
	.quad _C_LABEL(Xresume_ioapic_level25)
	.quad _C_LABEL(Xintr_ioapic_level26), _C_LABEL(Xrecurse_ioapic_level26)
	.quad _C_LABEL(Xresume_ioapic_level26)
	.quad _C_LABEL(Xintr_ioapic_level27), _C_LABEL(Xrecurse_ioapic_level27)
	.quad _C_LABEL(Xresume_ioapic_level27)
	.quad _C_LABEL(Xintr_ioapic_level28), _C_LABEL(Xrecurse_ioapic_level28)
	.quad _C_LABEL(Xresume_ioapic_level28)
	.quad _C_LABEL(Xintr_ioapic_level29), _C_LABEL(Xrecurse_ioapic_level29)
	.quad _C_LABEL(Xresume_ioapic_level29)
	.quad _C_LABEL(Xintr_ioapic_level30), _C_LABEL(Xrecurse_ioapic_level30)
	.quad _C_LABEL(Xresume_ioapic_level30)
	.quad _C_LABEL(Xintr_ioapic_level31), _C_LABEL(Xrecurse_ioapic_level31)
	.quad _C_LABEL(Xresume_ioapic_level31)
END(ioapic_level_stubs)
#endif

#else	/* !XEN */
/* Resume/recurse procedures for spl() */
#define	XENINTRSTUB(name, num, early_ack, late_ack, mask, unmask, level_mask) \
IDTVEC(recurse_ ## name ## num)						;\
  	INTR_RECURSE_HWFRAME						;\
  	subq	$8,%rsp							;\
  	pushq	$T_ASTFLT		/* trap # for doing ASTs */	;\
  	INTRENTRY							;\
IDTVEC(resume_ ## name ## num)						\
  	movq	$IREENT_MAGIC,TF_ERR(%rsp)				;\
  	movl	%ebx,%r13d						;\
  	movq	CPUVAR(ISOURCES) + (num) * 8, %r14			;\
1:									\
  	pushq	%r13							;\
  	movl	$num,CPUVAR(ILEVEL)					;\
  	STI(si)								;\
  	incl	CPUVAR(IDEPTH)						;\
  	movq	IS_HANDLERS(%r14),%rbx					;\
6:									\
  	movq	IH_ARG(%rbx),%rdi					;\
  	movq	%rsp,%rsi						;\
  	call	*IH_FUN(%rbx)		/* call it */			;\
  	movq	IH_IPL_NEXT(%rbx),%rbx	/* next handler in chain */	;\
  	testq	%rbx,%rbx						;\
  	jnz	6b							;\
5:									\
  	CLI(si)								;\
  	unmask(num)			/* unmask it in hardware */	;\
  	late_ack(num)							;\
  	STI(si)								;\
  	jmp	_C_LABEL(Xdoreti)	/* lower spl and do ASTs */	;\
  
# The unmask func for Xen events
#define hypervisor_asm_unmask(num)			\
  	movq $num, %rdi					;\
  	call _C_LABEL(hypervisor_enable_ipl)
  
XENINTRSTUB(xenev,0,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,1,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,2,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,3,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,4,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,5,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,6,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,7,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,8,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,9,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,10,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,11,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,12,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,13,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,14,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,15,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,16,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,17,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,18,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,19,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,20,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,21,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,22,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,23,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,24,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,25,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,26,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,27,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,28,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,29,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,30,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,31,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
  
LABEL(xenev_stubs)
  	.quad _C_LABEL(Xrecurse_xenev0), _C_LABEL(Xresume_xenev0)
  	.quad _C_LABEL(Xrecurse_xenev1) ,_C_LABEL(Xresume_xenev1)
  	.quad _C_LABEL(Xrecurse_xenev2) ,_C_LABEL(Xresume_xenev2)
  	.quad _C_LABEL(Xrecurse_xenev3) ,_C_LABEL(Xresume_xenev3)
  	.quad _C_LABEL(Xrecurse_xenev4) ,_C_LABEL(Xresume_xenev4)
  	.quad _C_LABEL(Xrecurse_xenev5) ,_C_LABEL(Xresume_xenev5)
  	.quad _C_LABEL(Xrecurse_xenev6) ,_C_LABEL(Xresume_xenev6)
  	.quad _C_LABEL(Xrecurse_xenev7) ,_C_LABEL(Xresume_xenev7)
  	.quad _C_LABEL(Xrecurse_xenev8) ,_C_LABEL(Xresume_xenev8)
  	.quad _C_LABEL(Xrecurse_xenev9) ,_C_LABEL(Xresume_xenev9)
  	.quad _C_LABEL(Xrecurse_xenev10), _C_LABEL(Xresume_xenev10)
  	.quad _C_LABEL(Xrecurse_xenev11), _C_LABEL(Xresume_xenev11)
  	.quad _C_LABEL(Xrecurse_xenev12), _C_LABEL(Xresume_xenev12)
  	.quad _C_LABEL(Xrecurse_xenev13), _C_LABEL(Xresume_xenev13)
  	.quad _C_LABEL(Xrecurse_xenev14), _C_LABEL(Xresume_xenev14)
  	.quad _C_LABEL(Xrecurse_xenev15), _C_LABEL(Xresume_xenev15)
  	.quad _C_LABEL(Xrecurse_xenev16), _C_LABEL(Xresume_xenev16)
  	.quad _C_LABEL(Xrecurse_xenev17), _C_LABEL(Xresume_xenev17)
  	.quad _C_LABEL(Xrecurse_xenev18), _C_LABEL(Xresume_xenev18)
  	.quad _C_LABEL(Xrecurse_xenev19), _C_LABEL(Xresume_xenev19)
  	.quad _C_LABEL(Xrecurse_xenev20), _C_LABEL(Xresume_xenev20)
  	.quad _C_LABEL(Xrecurse_xenev21), _C_LABEL(Xresume_xenev21)
  	.quad _C_LABEL(Xrecurse_xenev22), _C_LABEL(Xresume_xenev22)
  	.quad _C_LABEL(Xrecurse_xenev23), _C_LABEL(Xresume_xenev23)
  	.quad _C_LABEL(Xrecurse_xenev24), _C_LABEL(Xresume_xenev24)
  	.quad _C_LABEL(Xrecurse_xenev25), _C_LABEL(Xresume_xenev25)
  	.quad _C_LABEL(Xrecurse_xenev26), _C_LABEL(Xresume_xenev26)
  	.quad _C_LABEL(Xrecurse_xenev27), _C_LABEL(Xresume_xenev27)
  	.quad _C_LABEL(Xrecurse_xenev28), _C_LABEL(Xresume_xenev28)
  	.quad _C_LABEL(Xrecurse_xenev29), _C_LABEL(Xresume_xenev29)
  	.quad _C_LABEL(Xrecurse_xenev30), _C_LABEL(Xresume_xenev30)
  	.quad _C_LABEL(Xrecurse_xenev31), _C_LABEL(Xresume_xenev31)
END(xenev_stubs)

# Xen callbacks

# Hypervisor callback
NENTRY(hypervisor_callback)
	movq (%rsp),%rcx
	movq 8(%rsp),%r11
	addq $16,%rsp
	pushq	$0	# Dummy error code
	pushq	$T_ASTFLT
	INTRENTRY
	# sti??
	movq	%rsp, %rdi
	subq	$8, %rdi; /* don't forget if_ppl */
	call	do_hypervisor_callback
	testb   $SEL_RPL,TF_CS(%rsp)
	jnz	doreti_checkast
1:
	INTRFASTEXIT
END(hypervisor_callback)

# Panic?
NENTRY(failsafe_callback)
	movq (%rsp),%rcx
	movq 8(%rsp),%r11
	addq $16,%rsp
	pushq	$0
	pushq	$T_ASTFLT
	INTRENTRY
	movq	%rsp, %rdi
	subq	$8, %rdi; /* don't forget if_ppl */
	call xen_failsafe_handler
	INTRFASTEXIT
#	jmp	HYPERVISOR_iret
END(failsafe_callback)

#endif	/* !XEN */
