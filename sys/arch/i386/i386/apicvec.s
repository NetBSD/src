/* $NetBSD: apicvec.s,v 1.1.2.5 2001/04/30 16:23:09 sommerfeld Exp $ */	

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by RedBack Networks Inc.
 *
 * Author: Bill Sommerfeld
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
	
#include <machine/i82489reg.h>
	
#ifdef MULTIPROCESSOR
	.globl	XINTR(ipi)
XINTR(ipi):			
	pushl	$0		
	pushl	$T_ASTFLT
	INTRENTRY		
	MAKE_FRAME
	pushl	CPL
	movl	_C_LABEL(lapic_ppr),%eax
	movl	%eax,CPL
	movl	$0,_C_LABEL(local_apic)+LAPIC_EOI
        sti			/* safe to take interrupts.. */
	call	_C_LABEL(i386_ipi_handler)
	jmp	_C_LABEL(Xdoreti)

#if defined(DDB)
	.globl	XINTR(ddbipi)	
XINTR(ddbipi):			
	pushl	$0		
	pushl	$T_ASTFLT
	INTRENTRY		
	MAKE_FRAME		
	pushl	CPL
	movl	_C_LABEL(lapic_ppr),%eax
	movl	%eax,CPL
	movl	$0,_C_LABEL(local_apic)+LAPIC_EOI
        sti			/* safe to take interrupts.. */
	call	_C_LABEL(ddb_ipi)
	jmp	_C_LABEL(Xdoreti)
#endif
#endif
	
	/*
	 * Interrupt from the local APIC timer.
	 */
	.globl XINTR(ltimer)
XINTR(ltimer):			
	pushl	$0		
	pushl	$T_ASTFLT
	INTRENTRY		
	MAKE_FRAME		
	pushl	CPL
	movl	_C_LABEL(lapic_ppr),%eax
	movl	%eax,CPL
	movl	$0,_C_LABEL(local_apic)+LAPIC_EOI
	sti
	movl	%esp,%eax
	pushl	%eax
	call	_C_LABEL(lapic_clockintr)
	addl	$4,%esp		
	jmp	_C_LABEL(Xdoreti)

	.globl XINTR(softclock), XINTR(softnet), XINTR(softser)
XINTR(softclock):
	pushl	$0		
	pushl	$T_ASTFLT
	INTRENTRY		
	MAKE_FRAME		
	pushl	CPL
	movl	$IPL_SOFTCLOCK,CPL
	andl	$~(1<<SIR_CLOCK),_C_LABEL(ipending)
	movl	$0,_C_LABEL(local_apic)+LAPIC_EOI
	sti
	call	_C_LABEL(apic_intlock)
	pushl	$I386_SOFTINTR_SOFTCLOCK
	call	_C_LABEL(softintr_dispatch)
	addl	$4,%esp
	call	_C_LABEL(apic_intunlock)	
	jmp	_C_LABEL(Xdoreti)
	
XINTR(softnet):
	pushl	$0		
	pushl	$T_ASTFLT
	INTRENTRY		
	MAKE_FRAME		
	pushl	CPL
	movl	$IPL_SOFTNET,CPL
	andl	$~(1<<SIR_NET),_C_LABEL(ipending)
	movl	$0,_C_LABEL(local_apic)+LAPIC_EOI	
	sti
	call	_C_LABEL(apic_intlock)		
	xorl	%edi,%edi
	xchgl	_C_LABEL(netisr),%edi

#include "net/netisr_dispatch.h"
	
	pushl	$I386_SOFTINTR_SOFTNET
	call	_C_LABEL(softintr_dispatch)
	addl	$4,%esp
	call	_C_LABEL(apic_intunlock)		
	jmp	_C_LABEL(Xdoreti)

XINTR(softser):	
	pushl	$0		
	pushl	$T_ASTFLT
	INTRENTRY		
	MAKE_FRAME		
	pushl	CPL
	movl	$IPL_SOFTSERIAL,CPL
	andl	$~(1<<SIR_SERIAL),_C_LABEL(ipending)
	movl	$0,_C_LABEL(local_apic)+LAPIC_EOI	
	sti
	call	_C_LABEL(apic_intlock)
	pushl	$I386_SOFTINTR_SOFTSERIAL
	call	_C_LABEL(softintr_dispatch)
	addl	$4,%esp
	call	_C_LABEL(apic_intunlock)	
	jmp	_C_LABEL(Xdoreti)

#if NIOAPIC > 0
	/*
	 * I/O APIC interrupt.
	 * We sort out which one is which based on the value of 
	 * the processor priority register.
	 *
	 * XXX no stray interrupt mangling stuff..
	 * XXX use cmove when appropriate.
	 * XXX need to sort out these handlers better..
	 * (need a third flavor, for handling shared interrupts 
	 * at differing IPL's)
	 */
	
#define APICINTR(minor)							\
XINTR(ioapic/**/minor):							\
	pushl	$0							;\
	pushl	$T_ASTFLT						;\
	INTRENTRY							;\
	MAKE_FRAME							;\
	pushl	CPL							;\
	movl	_C_LABEL(lapic_ppr),%eax				;\
	movl	%eax,CPL						;\
	movl	$0,_C_LABEL(local_apic)+LAPIC_EOI			;\
	sti								;\
	orl	$minor,%eax						;\
	incl	_C_LABEL(apic_intrcount)(,%eax,4)			;\
	movl	_C_LABEL(apic_intrhand)(,%eax,4),%ebx /* chain head */	;\
	testl	%ebx,%ebx						;\
	jz	8f			/* oops, no handlers.. */	;\
	call	_C_LABEL(apic_intlock)					;\
7:									 \
	movl	IH_ARG(%ebx),%eax	/* get handler arg */		;\
	testl	%eax,%eax						;\
	jnz	6f							;\
	movl	%esp,%eax		/* 0 means frame pointer */	;\
6:									 \
	pushl	%eax							;\
	call	IH_FUN(%ebx)		/* call it */			;\
	addl	$4,%esp			/* toss the arg */		;\
	incl	IH_COUNT(%ebx)		/* count the intrs */		;\
	movl	IH_NEXT(%ebx),%ebx	/* next handler in chain */	;\
	testl	%ebx,%ebx						;\
	jnz	7b							;\
	call	_C_LABEL(apic_intunlock)				;\
8:									 \
	jmp	_C_LABEL(Xdoreti)

#define APICULINTR(minor)						\
XINTR(ioapicul/**/minor):						\
	pushl	$0							;\
	pushl	$T_ASTFLT						;\
	INTRENTRY							;\
	MAKE_FRAME							;\
	pushl	CPL							;\
	movl	_C_LABEL(lapic_ppr),%eax				;\
	movl	%eax,CPL						;\
	movl	$0,_C_LABEL(local_apic)+LAPIC_EOI			;\
	sti								;\
	orl	$minor,%eax						;\
	incl	_C_LABEL(apic_intrcount)(,%eax,4)			;\
	movl	_C_LABEL(apic_intrhand)(,%eax,4),%ebx /* chain head */	;\
	testl	%ebx,%ebx						;\
	jz	8f			/* oops, no handlers.. */	;\
7:									 \
	movl	IH_ARG(%ebx),%eax	/* get handler arg */		;\
	testl	%eax,%eax						;\
	jnz	6f							;\
	movl	%esp,%eax		/* 0 means frame pointer */	;\
6:									 \
	pushl	%eax							;\
	call	IH_FUN(%ebx)		/* call it */			;\
	addl	$4,%esp			/* toss the arg */		;\
	incl	IH_COUNT(%ebx)		/* count the intrs */		;\
	movl	IH_NEXT(%ebx),%ebx	/* next handler in chain */	;\
	testl	%ebx,%ebx						;\
	jnz	7b							;\
8:									 \
	jmp	_C_LABEL(Xdoreti)
		
APICINTR(0)
APICINTR(1)
APICINTR(2)
APICINTR(3)
APICINTR(4)
APICINTR(5)
APICINTR(6)
APICINTR(7)
APICINTR(8)
APICINTR(9)
APICINTR(10)
APICINTR(11)
APICINTR(12)
APICINTR(13)
APICINTR(14)
APICINTR(15)

APICULINTR(0)
APICULINTR(1)
APICULINTR(2)
APICULINTR(3)
APICULINTR(4)
APICULINTR(5)
APICULINTR(6)
APICULINTR(7)
APICULINTR(8)
APICULINTR(9)
APICULINTR(10)
APICULINTR(11)
APICULINTR(12)
APICULINTR(13)
APICULINTR(14)
APICULINTR(15)
	
	.globl _C_LABEL(apichandler)

_C_LABEL(apichandler):	
	.long	_C_LABEL(Xintrioapic0),_C_LABEL(Xintrioapic1)
	.long	_C_LABEL(Xintrioapic2),_C_LABEL(Xintrioapic3)
	.long	_C_LABEL(Xintrioapic4),_C_LABEL(Xintrioapic5)
	.long	_C_LABEL(Xintrioapic6),_C_LABEL(Xintrioapic7)
	.long	_C_LABEL(Xintrioapic8),_C_LABEL(Xintrioapic9)
	.long	_C_LABEL(Xintrioapic10),_C_LABEL(Xintrioapic11)
	.long	_C_LABEL(Xintrioapic12),_C_LABEL(Xintrioapic13)
	.long	_C_LABEL(Xintrioapic14),_C_LABEL(Xintrioapic15)
	.long	_C_LABEL(Xintrioapicul0),_C_LABEL(Xintrioapicul1)
	.long	_C_LABEL(Xintrioapicul2),_C_LABEL(Xintrioapicul3)
	.long	_C_LABEL(Xintrioapicul4),_C_LABEL(Xintrioapicul5)
	.long	_C_LABEL(Xintrioapicul6),_C_LABEL(Xintrioapicul7)
	.long	_C_LABEL(Xintrioapicul8),_C_LABEL(Xintrioapicul9)
	.long	_C_LABEL(Xintrioapicul10),_C_LABEL(Xintrioapicul11)
	.long	_C_LABEL(Xintrioapicul12),_C_LABEL(Xintrioapicul13)
	.long	_C_LABEL(Xintrioapicul14),_C_LABEL(Xintrioapicul15)

#endif
	
