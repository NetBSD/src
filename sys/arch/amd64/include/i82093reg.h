/*	 $NetBSD: i82093reg.h,v 1.2 2003/05/04 23:46:41 fvdl Exp $ */

#include <x86/i82093reg.h>

#ifdef _KERNEL

#if defined(_KERNEL_OPT)
#include "opt_multiprocessor.h"
#endif

#define ioapic_asm_ack(num) \
	movl	$0,(_C_LABEL(local_apic)+LAPIC_EOI)(%rip)

#ifdef MULTIPROCESSOR

#define ioapic_asm_lock(num) \
	movl	$1,%esi						;\
77:								\
	xchgl	%esi,PIC_LOCK(%rdi)				;\
	testl	%esi,%esi					;\
	jne	77b

#define ioapic_asm_unlock(num) \
	movl	$0,PIC_LOCK(%rdi)
	
#else

#define ioapic_asm_lock(num)
#define ioapic_asm_unlock(num)

#endif	/* MULTIPROCESSOR */


#define ioapic_mask(num) \
	movq	IS_PIC(%r14),%rdi				;\
	ioapic_asm_lock(num)					;\
	movl	IS_PIN(%r14),%esi				;\
	leaq	0x10(%rsi,%rsi,1),%rsi				;\
	movq	IOAPIC_SC_REG(%rdi),%r15			;\
	movl	%esi, (%r15)					;\
	movq	IOAPIC_SC_DATA(%rdi),%r15			;\
	movl	(%r15),%esi					;\
	orl	$IOAPIC_REDLO_MASK,%esi				;\
	movl	%esi,(%r15)					;\
	ioapic_asm_unlock(num)

#define ioapic_unmask(num) \
	cmpq	$IREENT_MAGIC,(TF_ERR+8)(%rsp)			;\
	jne	79f						;\
	movq	IS_PIC(%r14),%rdi				;\
	ioapic_asm_lock(num)					;\
	movl	IS_PIN(%r14),%esi				;\
	leaq	0x10(%rsi,%rsi,1),%rsi				;\
	movq	IOAPIC_SC_REG(%rdi),%r15			;\
	movl	%esi, (%r15)					;\
	movq	IOAPIC_SC_DATA(%rdi),%r15			;\
	movl	(%r15),%esi					;\
	andl	$~IOAPIC_REDLO_MASK,%esi			;\
	movl	%esi,(%r15)					;\
	ioapic_asm_unlock(num)					;\
79:

#endif
