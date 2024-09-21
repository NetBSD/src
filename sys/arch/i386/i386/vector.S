/*	$NetBSD: vector.S,v 1.89.4.1 2024/09/21 12:37:02 martin Exp $	*/

/*
 * Copyright 2002 (c) Wasabi Systems, Inc.
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

/*-
 * Copyright (c) 1998, 2007, 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum, and by Andrew Doran.
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

#include <machine/asm.h>
__KERNEL_RCSID(0, "$NetBSD: vector.S,v 1.89.4.1 2024/09/21 12:37:02 martin Exp $");

#include "opt_ddb.h"
#include "opt_multiprocessor.h"
#include "opt_xen.h"
#include "opt_dtrace.h"

#include <machine/i8259.h>
#include <machine/i82093reg.h>
#include <machine/i82489reg.h>
#include <machine/frameasm.h>
#include <machine/segments.h>
#include <machine/specialreg.h>
#include <machine/trap.h>
#ifdef XEN
#include <xen/xen.h>
#endif

#include "ioapic.h"
#include "lapic.h"
#ifndef XENPV
#include "hyperv.h"
#endif

#include "assym.h"

/*
 * Macros for interrupt entry, call to handler, and exit.
 *
 * XXX
 * The interrupt frame is set up to look like a trap frame.  This may be a
 * waste.  The only handler which needs a frame is the clock handler, and it
 * only needs a few bits.  Xdoreti() needs a trap frame for handling ASTs, but
 * it could easily convert the frame on demand.
 *
 * The direct costs of setting up a trap frame are two pushl's (error code and
 * trap number), an addl to get rid of these, and pushing and popping the
 * callee-saved registers %esi, %edi, %ebx, and %ebp twice.
 *
 * If the interrupt frame is made more flexible,  INTR can push %eax first and
 * decide the ipending case with less overhead, e.g., by avoiding loading the
 * segment registers.
 */

/*
 * Store address of TSS in %eax, given a selector in %eax.
 * Clobbers %eax, %ecx, %edx, but that's ok for its usage.
 * This is a bit complicated, but it's done to make as few
 * assumptions as possible about the validity of the environment.
 * The GDT and the current and previous TSS are known to be OK,
 * otherwise we would not be here. The only other thing that needs
 * to be OK is the cpu_info structure for the current CPU.
 */
#define GET_TSS \
	andl	$0xfff8,%eax				;\
	addl	CPUVAR(GDT),%eax			;\
	movl	2(%eax),%edx				;\
	andl	$0xffffff,%edx				;\
	movzbl	7(%eax),%eax				;\
	shl	$24,%eax				;\
	orl	%edx,%eax

#ifdef KDTRACE_HOOKS
	.bss
	.globl	dtrace_invop_jump_addr
	.align	4
	.type	dtrace_invop_jump_addr, @object
	.size	dtrace_invop_jump_addr, 4
dtrace_invop_jump_addr:
	.zero	4
	.globl	dtrace_invop_calltrap_addr
	.align	4
	.type	dtrace_invop_calltrap_addr, @object
	.size	dtrace_invop_calltrap_addr, 4
dtrace_invop_calltrap_addr:
	.zero	8
	.text
#endif

#ifndef XENPV
#if NLAPIC > 0
#ifdef MULTIPROCESSOR
/*
 * General purpose IPI handler.
 */
IDTVEC(recurse_lapic_ipi)
	INTR_RECURSE_HWFRAME
	pushl	$0
	pushl	$T_ASTFLT
	INTRENTRY
	jmp	1f
IDTVEC_END(recurse_lapic_ipi)
IDTVEC(intr_x2apic_ipi)
	pushl	$0
	pushl	$T_ASTFLT
	INTRENTRY
	movl	$(MSR_X2APIC_BASE + MSR_X2APIC_EOI),%ecx
	xorl	%eax,%eax
	xorl	%edx,%edx
	wrmsr
	movzbl	CPUVAR(ILEVEL),%ebx
	cmpl	$IPL_HIGH,%ebx
	jae	2f
	jmp	1f
IDTVEC_END(intr_x2apic_ipi)
IDTVEC(intr_lapic_ipi)
	pushl	$0
	pushl	$T_ASTFLT
	INTRENTRY
	movl	_C_LABEL(local_apic_va),%ebx
	movl	$0,LAPIC_EOI(%ebx)
	movzbl	CPUVAR(ILEVEL),%ebx
	cmpl	$IPL_HIGH,%ebx
	jae	2f
IDTVEC_END(intr_lapic_ipi)
IDTVEC(resume_lapic_ipi)
1:
	pushl	%ebx
	IDEPTH_INCR
	movb	$IPL_HIGH,CPUVAR(ILEVEL)
	sti
	call	_C_LABEL(x86_ipi_handler)
	cli
	jmp	_C_LABEL(Xdoreti)
2:
	btsl	$(LIR_IPI - 32),CPUVAR(IPENDING)+4
	INTRFASTEXIT
IDTVEC_END(resume_lapic_ipi)

/*
 * TLB shootdown handler.
 */
IDTVEC(intr_lapic_tlb)
	pushl	$0
	pushl	$T_ASTFLT
	INTRENTRY
	call	_C_LABEL(pmap_tlb_intr)
	movl	_C_LABEL(local_apic_va),%eax
	movl	$0,LAPIC_EOI(%eax)
	INTRFASTEXIT
IDTVEC_END(intr_lapic_tlb)

IDTVEC(intr_x2apic_tlb)
	pushl	$0
	pushl	$T_ASTFLT
	INTRENTRY
	call	_C_LABEL(pmap_tlb_intr)
	movl	$(MSR_X2APIC_BASE + MSR_X2APIC_EOI),%ecx
	xorl	%eax,%eax
	xorl	%edx,%edx
	wrmsr
	INTRFASTEXIT
IDTVEC_END(intr_x2apic_tlb)

#if defined(DDB)
/*
 * No need to use INTRENTRY, since we were brought here through a task-gate
 * which triggered a hardware context switch and saved the GPRs in the TSS.
 */
IDTVEC(intr_ddbipi)
1:
	str	%ax
	GET_TSS
	movzwl	(%eax),%eax
	GET_TSS
	pushl	%eax
	movl	_C_LABEL(local_apic_va),%ebx
	movl	$0xff,LAPIC_TPRI(%ebx)
	movl	_C_LABEL(local_apic_va),%ebx
	movl	$0,LAPIC_EOI(%ebx)
	sti
	call	_C_LABEL(ddb_ipi_tss)
	addl	$4,%esp
	movl	_C_LABEL(local_apic_va),%ebx
	movl	$0,LAPIC_TPRI(%ebx)
	iret
	jmp	1b
IDTVEC_END(intr_ddbipi)

IDTVEC(intr_x2apic_ddbipi)
1:
	str	%ax
	GET_TSS
	movzwl	(%eax),%eax
	GET_TSS
	pushl	%eax
	movl	$(MSR_X2APIC_BASE + MSR_X2APIC_TPRI),%ecx
	movl	0xff,%eax
	xorl	%edx,%edx
	wrmsr
	movl	$(MSR_X2APIC_BASE + MSR_X2APIC_EOI),%ecx
	xorl	%eax,%eax
	xorl	%edx,%edx
	wrmsr
	sti
	call	_C_LABEL(ddb_ipi_tss)
	addl	$4,%esp
	movl	$(MSR_X2APIC_BASE + MSR_X2APIC_TPRI),%ecx
	xorl	%eax,%eax
	xorl	%edx,%edx
	wrmsr
	iret
	jmp	1b
IDTVEC_END(intr_x2apic_ddbipi)
#endif /* DDB */
#endif /* MULTIPROCESSOR */

	/*
	 * Interrupt from the local APIC timer.
	 */
IDTVEC(recurse_lapic_ltimer)
	INTR_RECURSE_HWFRAME
	pushl	$0
	pushl	$T_ASTFLT
	INTRENTRY
	jmp	1f
IDTVEC_END(recurse_lapic_ltimer)
IDTVEC(intr_x2apic_ltimer)
	pushl	$0
	pushl	$T_ASTFLT
	INTRENTRY
	movl	$(MSR_X2APIC_BASE + MSR_X2APIC_EOI),%ecx
	xorl	%eax,%eax
	xorl	%edx,%edx
	wrmsr
	movzbl	CPUVAR(ILEVEL),%ebx
	cmpl	$IPL_CLOCK,%ebx
	jae	2f
	jmp	1f
IDTVEC_END(intr_x2apic_ltimer)
IDTVEC(intr_lapic_ltimer)
	pushl	$0
	pushl	$T_ASTFLT
	INTRENTRY
	movl	_C_LABEL(local_apic_va),%ebx
	movl	$0,LAPIC_EOI(%ebx)
	movzbl	CPUVAR(ILEVEL),%ebx
	cmpl	$IPL_CLOCK,%ebx
	jae	2f
IDTVEC_END(intr_lapic_ltimer)
IDTVEC(resume_lapic_ltimer)
1:
	pushl	%ebx
	IDEPTH_INCR
	movb	$IPL_CLOCK,CPUVAR(ILEVEL)
	sti
	pushl	$0
	call	_C_LABEL(lapic_clockintr)
	addl	$4,%esp
	cli
	jmp	_C_LABEL(Xdoreti)
2:
	btsl	$(LIR_TIMER - 32),CPUVAR(IPENDING)+4
	INTRFASTEXIT
IDTVEC_END(resume_lapic_ltimer)

#if NHYPERV > 0
	/*
	 * Hyper-V event channel upcall interrupt handler.
	 * Only used when the hypervisor supports direct vector callbacks.
	 */
IDTVEC(recurse_hyperv_hypercall)
	INTR_RECURSE_HWFRAME
	pushl	$0
	pushl	$T_ASTFLT
	INTRENTRY
	jmp	1f
IDTVEC_END(recurse_hyperv_hypercall)
IDTVEC(intr_hyperv_hypercall)
	pushl	$0
	pushl	$T_ASTFLT
	INTRENTRY
	movzbl	CPUVAR(ILEVEL),%ebx
	cmpl	$IPL_NET,%ebx
	jae	2f
	jmp	1f
IDTVEC_END(intr_hyperv_hypercall)
IDTVEC(resume_hyperv_hypercall)
1:
	pushl	%ebx
	IDEPTH_INCR
	movb	$IPL_NET,CPUVAR(ILEVEL)
	sti
	pushl	%esp
	call	_C_LABEL(hyperv_hypercall_intr)
	addl	$4,%esp
	cli
	jmp	_C_LABEL(Xdoreti)
2:
	orl	$(1 << LIR_HV),CPUVAR(IPENDING)
	INTRFASTEXIT
IDTVEC_END(resume_hyperv_hypercall)
#endif	/* NHYPERV > 0 */

#endif /* NLAPIC > 0 */


#define voidop(num)


/*
 * This macro defines the generic stub code. Its arguments modifiy it
 * for specific PICs.
 */

#define	INTRSTUB1(name, num, sub, off, early_ack, late_ack, mask, unmask, level_mask) \
IDTVEC(recurse_ ## name ## num)						;\
	INTR_RECURSE_HWFRAME						;\
	subl	$4,%esp							;\
	pushl	$T_ASTFLT		/* trap # for doing ASTs */	;\
	INTRENTRY							;\
IDTVEC_END(recurse_ ## name ## num)					;\
IDTVEC(resume_ ## name ## num)						\
	movl	$IREENT_MAGIC,TF_ERR(%esp)				;\
	movl	%ebx,%esi						;\
	movl	CPUVAR(ISOURCES) + (num) * 4,%ebp			;\
	movl	IS_MAXLEVEL(%ebp),%ebx					;\
	jmp	1f							;\
IDTVEC_END(resume_ ## name ## num)					;\
IDTVEC(intr_ ## name ## num)						;\
	pushl	$0			/* dummy error code */		;\
	pushl	$T_ASTFLT		/* trap # for doing ASTs */	;\
	INTRENTRY							;\
	movl	CPUVAR(ISOURCES) + (num) * 4,%ebp			;\
	mask(num)			/* mask it in hardware */	;\
	early_ack(num)			/* and allow other intrs */	;\
	testl	%ebp,%ebp						;\
	jz	9f			/* stray */			;\
	movl	IS_MAXLEVEL(%ebp),%ebx					;\
	movzbl	CPUVAR(ILEVEL),%esi					;\
	cmpl	%ebx,%esi						;\
	jae	10f			/* currently masked; hold it */	;\
	addl	$1,CPUVAR(NINTR)	/* statistical info */		;\
	adcl	$0,CPUVAR(NINTR)+4					;\
	addl	$1,IS_EVCNTLO(%ebp)	/* inc event counter */		;\
	adcl	$0,IS_EVCNTHI(%ebp)					;\
1:									\
	pushl	%esi			/* if_ppi */			;\
	movb	%bl,CPUVAR(ILEVEL)					;\
	/* switch stack if necessary, and push a ptr to our intrframe */ \
	IDEPTH_INCR							;\
	sti								;\
	movl	IS_HANDLERS(%ebp),%ebx					;\
	cmpl	$0,IS_MASK_COUNT(%ebp)	/* source currently masked? */	;\
	jne	12f			/* yes, hold it */		;\
6:									\
	movl	IH_LEVEL(%ebx),%eax					;\
	cmpl	%esi,%eax						;\
	jle	7f							;\
	pushl	IH_ARG(%ebx)						;\
	movl	IH_FUN(%ebx),%edi					;\
	movb	%al,CPUVAR(ILEVEL)					;\
	movl	IH_NEXT(%ebx),%ebx	/* next handler in chain */	;\
	call	*%edi			/* call it */			;\
	addl	$4,%esp			/* toss the arg */		;\
	testl	%ebx,%ebx						;\
	jnz	6b							;\
	cmpl	$0,IS_MASK_COUNT(%ebp)	/* source now masked? */	;\
	jne	12f			/* yes, deal */			;\
	cli								;\
	unmask(num)			/* unmask it in hardware */	;\
	late_ack(num)							;\
	jmp	_C_LABEL(Xdoreti)	/* lower spl and do ASTs */	;\
7:									\
	cli								;\
	btsl	$(num - sub),CPUVAR(IPENDING) + off			;\
8:	level_mask(num)							;\
	late_ack(num)							;\
	jmp	_C_LABEL(Xdoreti)	/* lower spl and do ASTs */	;\
12:									\
	cli								;\
	btsl	$(num - sub),CPUVAR(IMASKED) + off			;\
	btrl	$(num - sub),CPUVAR(IPENDING) + off			;\
	jmp	8b							;\
10:									\
	btsl	$(num - sub),CPUVAR(IPENDING) + off			;\
	level_mask(num)							;\
	late_ack(num)							;\
	INTRFASTEXIT							;\
9:									\
	pushl	%esp			/* for unmask */		;\
	unmask(num)							;\
	late_ack(num)							;\
	addl	$4,%esp							;\
	INTRFASTEXIT							;\
IDTVEC_END(intr_ ## name ## num)

#define	INTRSTUB(name, num, early_ack, late_ack, mask, unmask, level_mask) \
    INTRSTUB1(name, num, 0, 0, early_ack, late_ack, mask, unmask, level_mask)
#define	INTRSTUB32(name, num, early_ack, late_ack, mask, unmask, level_mask) \
    INTRSTUB1(name, num, 32, 4, early_ack, late_ack, mask, unmask, level_mask)

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

#define INTRSTUB_56(name,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,0,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,1,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,2,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,3,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,4,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,5,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,6,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,7,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,8,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,9,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,10,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,11,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,12,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,13,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,14,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,15,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,16,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,17,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,18,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,19,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,20,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,21,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,22,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,23,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,24,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,25,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,26,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,27,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,28,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,29,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,30,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB(name,31,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB32(name,32,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB32(name,33,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB32(name,34,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB32(name,35,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB32(name,36,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB32(name,37,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB32(name,38,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB32(name,39,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB32(name,40,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB32(name,41,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB32(name,42,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB32(name,43,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB32(name,44,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB32(name,45,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB32(name,46,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB32(name,47,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB32(name,48,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB32(name,49,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB32(name,50,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB32(name,51,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB32(name,52,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB32(name,53,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB32(name,54,early_ack,late_ack,mask,unmask,level_mask)	;\
	INTRSTUB32(name,55,early_ack,late_ack,mask,unmask,level_mask)

INTRSTUB_56(ioapic_edge,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB_56(ioapic_level,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)

INTRSTUB_56(x2apic_edge,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB_56(x2apic_level,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)

#endif

/*
 * Create a struct intrstub.
 */
#define INTRSTUB_ENTRY(name) \
	.long _C_LABEL(Xintr_ ## name ), _C_LABEL(Xrecurse_ ## name ) ; \
	.long _C_LABEL(Xresume_ ## name ) ;

/*
 * Create an array of structs intrstub (16 entries).
 */
#define INTRSTUB_ARRAY_16(name) 		; \
	.type _C_LABEL(name ## _stubs), @object	; \
LABEL(name ## _stubs)				; \
	INTRSTUB_ENTRY(name ## 0)		; \
	INTRSTUB_ENTRY(name ## 1)		; \
	INTRSTUB_ENTRY(name ## 2)		; \
	INTRSTUB_ENTRY(name ## 3)		; \
	INTRSTUB_ENTRY(name ## 4)		; \
	INTRSTUB_ENTRY(name ## 5)		; \
	INTRSTUB_ENTRY(name ## 6)		; \
	INTRSTUB_ENTRY(name ## 7)		; \
	INTRSTUB_ENTRY(name ## 8)		; \
	INTRSTUB_ENTRY(name ## 9)		; \
	INTRSTUB_ENTRY(name ## 10)		; \
	INTRSTUB_ENTRY(name ## 11)		; \
	INTRSTUB_ENTRY(name ## 12)		; \
	INTRSTUB_ENTRY(name ## 13)		; \
	INTRSTUB_ENTRY(name ## 14)		; \
	INTRSTUB_ENTRY(name ## 15)		; \
END(name ## _stubs)

/*
 * Create an array of structs intrstub (56 entries).
 */
#define INTRSTUB_ARRAY_56(name) 		; \
	.type _C_LABEL(name ## _stubs), @object	; \
LABEL(name ## _stubs)				; \
	INTRSTUB_ENTRY(name ## 0)		; \
	INTRSTUB_ENTRY(name ## 1)		; \
	INTRSTUB_ENTRY(name ## 2)		; \
	INTRSTUB_ENTRY(name ## 3)		; \
	INTRSTUB_ENTRY(name ## 4)		; \
	INTRSTUB_ENTRY(name ## 5)		; \
	INTRSTUB_ENTRY(name ## 6)		; \
	INTRSTUB_ENTRY(name ## 7)		; \
	INTRSTUB_ENTRY(name ## 8)		; \
	INTRSTUB_ENTRY(name ## 9)		; \
	INTRSTUB_ENTRY(name ## 10)		; \
	INTRSTUB_ENTRY(name ## 11)		; \
	INTRSTUB_ENTRY(name ## 12)		; \
	INTRSTUB_ENTRY(name ## 13)		; \
	INTRSTUB_ENTRY(name ## 14)		; \
	INTRSTUB_ENTRY(name ## 15)		; \
	INTRSTUB_ENTRY(name ## 16)		; \
	INTRSTUB_ENTRY(name ## 17)		; \
	INTRSTUB_ENTRY(name ## 18)		; \
	INTRSTUB_ENTRY(name ## 19)		; \
	INTRSTUB_ENTRY(name ## 20)		; \
	INTRSTUB_ENTRY(name ## 21)		; \
	INTRSTUB_ENTRY(name ## 22)		; \
	INTRSTUB_ENTRY(name ## 23)		; \
	INTRSTUB_ENTRY(name ## 24)		; \
	INTRSTUB_ENTRY(name ## 25)		; \
	INTRSTUB_ENTRY(name ## 26)		; \
	INTRSTUB_ENTRY(name ## 27)		; \
	INTRSTUB_ENTRY(name ## 28)		; \
	INTRSTUB_ENTRY(name ## 29)		; \
	INTRSTUB_ENTRY(name ## 30)		; \
	INTRSTUB_ENTRY(name ## 31)		; \
	INTRSTUB_ENTRY(name ## 32)		; \
	INTRSTUB_ENTRY(name ## 33)		; \
	INTRSTUB_ENTRY(name ## 34)		; \
	INTRSTUB_ENTRY(name ## 35)		; \
	INTRSTUB_ENTRY(name ## 36)		; \
	INTRSTUB_ENTRY(name ## 37)		; \
	INTRSTUB_ENTRY(name ## 38)		; \
	INTRSTUB_ENTRY(name ## 39)		; \
	INTRSTUB_ENTRY(name ## 40)		; \
	INTRSTUB_ENTRY(name ## 41)		; \
	INTRSTUB_ENTRY(name ## 42)		; \
	INTRSTUB_ENTRY(name ## 43)		; \
	INTRSTUB_ENTRY(name ## 44)		; \
	INTRSTUB_ENTRY(name ## 45)		; \
	INTRSTUB_ENTRY(name ## 46)		; \
	INTRSTUB_ENTRY(name ## 47)		; \
	INTRSTUB_ENTRY(name ## 48)		; \
	INTRSTUB_ENTRY(name ## 49)		; \
	INTRSTUB_ENTRY(name ## 50)		; \
	INTRSTUB_ENTRY(name ## 51)		; \
	INTRSTUB_ENTRY(name ## 52)		; \
	INTRSTUB_ENTRY(name ## 53)		; \
	INTRSTUB_ENTRY(name ## 54)		; \
	INTRSTUB_ENTRY(name ## 55)		; \
END(name ## _stubs)

#endif /* XENPV */

#if defined(XEN)
#define voidop(num)

#define	XENINTRSTUB(name, sir, level, unmask) \
IDTVEC(recurse_ ## name ## sir)						;\
	INTR_RECURSE_HWFRAME						;\
	subl	$4,%esp							;\
	pushl	$T_ASTFLT		/* trap # for doing ASTs */	;\
	INTRENTRY							;\
IDTVEC(resume_ ## name ## sir)						\
	movl	$IREENT_MAGIC,TF_ERR(%esp)				;\
	pushl	%ebx							;\
	movl	CPUVAR(ISOURCES) + (sir) * 4,%ebp			;\
	movb	$level,CPUVAR(ILEVEL)					;\
	IDEPTH_INCR /* leaves old %esp on stack	*/			;\
	STI(%eax)							;\
	movl	IS_HANDLERS(%ebp),%ebx					;\
6:									\
	cmpl	$0, IH_PENDING(%ebx)	/* is handler pending ? */	;\
	je	7f			/* no */			;\
	movl	$0, IH_PENDING(%ebx)					;\
	pushl	IH_ARG(%ebx)						;\
	call	*IH_FUN(%ebx)		/* call it */			;\
	addl	$4,%esp			/* toss the arg */		;\
7:									\
	movl	IH_NEXT(%ebx),%ebx	/* next handler in chain */	;\
	testl	%ebx,%ebx						;\
	jnz	6b							;\
									\
	CLI(%eax)							;\
	unmask(sir)			/* unmask it in hardware */	;\
	jmp	_C_LABEL(Xdoreti)	/* lower spl and do ASTs */	;\

/*
 * Just unmasking the event isn't enough, we also need to
 * reassert the event pending bit if needed. For now just call
 * the C function doing it, maybe rewrite in inline assembly ?
 */
#define hypervisor_asm_unmask(sir)			\
	pushl	$sir					;\
	call	_C_LABEL(hypervisor_enable_sir)		;\
	addl	$4,%esp

XENINTRSTUB(xenev,SIR_XENIPL_VM,IPL_VM,hypervisor_asm_unmask)
XENINTRSTUB(xenev,SIR_XENIPL_SCHED,IPL_SCHED,hypervisor_asm_unmask)
XENINTRSTUB(xenev,SIR_XENIPL_HIGH,IPL_HIGH,hypervisor_asm_unmask)

/* On Xen, the xenev_stubs are purely for spl entry, since there is no
 * vector based mechanism. We however provide the entrypoint to ensure
 * that native and Xen struct intrstub ; definitions are uniform.
 */
panicmsg:	.ascii "vector Xen event entry path entered."
LABEL(entry_xenev)
	pushl $panicmsg
	call  _C_LABEL(panic)
END(entry_xenev)

#define XENINTRSTUB_ENTRY(name, sir) \
	.long entry_xenev , _C_LABEL(Xrecurse_ ## name ## sir); \
	.long _C_LABEL(Xresume_ ## name ## sir);

	.type	_C_LABEL(xenev_stubs), @object
LABEL(xenev_stubs)
	XENINTRSTUB_ENTRY(xenev, SIR_XENIPL_VM) ;
	XENINTRSTUB_ENTRY(xenev, SIR_XENIPL_SCHED) ;
	XENINTRSTUB_ENTRY(xenev, SIR_XENIPL_HIGH) ;
END(xenev_stubs)

#endif /* XEN */

#include "i386_trap.S"

#ifdef XEN

ENTRY(hypervisor_callback)
IDTVEC(hypervisor_pvhvm_callback)	
	pushl	$0			/* dummy error code */
	pushl	$T_ASTFLT
	INTRENTRY
	movzbl	CPUVAR(ILEVEL),%eax
	pushl	%eax
	IDEPTH_INCR
	/* IDEPTH_INCR puts %esp on stack; we use it as argument to
	 * do_hypervisor_callback. But don't restore the stack after,
	 * Xdoreti needs it too.
	 */
	call	do_hypervisor_callback
#ifndef XENPV
	movzbl	_C_LABEL(xenhvm_use_percpu_callback),%eax
	testl	%eax, %eax
	jz	1f
	movl	_C_LABEL(local_apic_va),%eax
	movl	$0, LAPIC_EOI(%eax)
1:
#endif
	jmp	_C_LABEL(Xdoreti)
IDTVEC_END(hypervisor_pvhvm_callback)
END(hypervisor_callback)

#endif /* XEN */
#ifdef XENPV	
/*
 * Hypervisor uses this for application faults while it executes.
 */
ENTRY(failsafe_callback)
	pop	%ds
	pop	%es
	pop	%fs
	pop	%gs
	call	_C_LABEL(xen_failsafe_handler)
	iret
END(failsafe_callback)

#else	/* XENPV */

	.section .rodata

INTRSTUB_ARRAY_16(legacy)

#if NIOAPIC > 0
INTRSTUB_ARRAY_56(ioapic_edge)
INTRSTUB_ARRAY_56(ioapic_level)

INTRSTUB_ARRAY_56(x2apic_edge)
INTRSTUB_ARRAY_56(x2apic_level)
#endif

#endif /* XENPV */
