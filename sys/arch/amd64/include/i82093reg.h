/*	 $NetBSD: i82093reg.h,v 1.5.60.1 2016/10/05 20:55:23 skrll Exp $ */

#include <x86/i82093reg.h>

#ifdef _KERNEL

#if defined(_KERNEL_OPT)
#include "opt_multiprocessor.h"
#endif

#define ioapic_asm_ack(num) \
	movl	$0,_C_LABEL(local_apic)+LAPIC_EOI

#ifdef MULTIPROCESSOR

#define ioapic_asm_lock(num) 			        \
	movb	$1,%bl				;	\
76:							\
        xchgb	%bl,PIC_LOCK(%rdi)		;	\
	testb	%bl,%bl				;	\
	jz	78f				;	\
77:							\
	pause					;	\
	nop					;	\
	nop					;	\
	cmpb	$0,PIC_LOCK(%rdi)		;	\
	jne	77b				;	\
	jmp	76b				;	\
78:

#define ioapic_asm_unlock(num) \
	movb	$0,PIC_LOCK(%rdi)
	
#else

#define ioapic_asm_lock(num)
#define ioapic_asm_unlock(num)

#endif	/* MULTIPROCESSOR */


#define ioapic_mask(num) \
	movq	IS_PIC(%r14),%rdi				;\
	ioapic_asm_lock(num)					;\
	movl	IS_PIN(%r14),%esi				;\
	leaq	0x10(%rsi,%rsi,1),%rsi				;\
	movq	PIC_IOAPIC(%rdi),%rdi				;\
	movq	IOAPIC_SC_REG(%rdi),%r15			;\
	movl	%esi, (%r15)					;\
	movq	IOAPIC_SC_DATA(%rdi),%r15			;\
	movl	(%r15),%esi					;\
	orl	$IOAPIC_REDLO_MASK,%esi				;\
	movl	%esi,(%r15)					;\
	movq	IS_PIC(%r14),%rdi				;\
	ioapic_asm_unlock(num)

#define ioapic_unmask(num) \
	cmpq	$IREENT_MAGIC,(TF_ERR+8)(%rsp)			;\
	jne	79f						;\
	movq	IS_PIC(%r14),%rdi				;\
	ioapic_asm_lock(num)					;\
	movl	IS_PIN(%r14),%esi				;\
	leaq	0x10(%rsi,%rsi,1),%rsi				;\
	movq	PIC_IOAPIC(%rdi),%rdi				;\
	movq	IOAPIC_SC_REG(%rdi),%r15			;\
	movq	IOAPIC_SC_DATA(%rdi),%r13			;\
	movl	%esi, (%r15)					;\
	movl	(%r13),%r12d					;\
	andl	$~IOAPIC_REDLO_MASK,%r12d			;\
	movl	%esi,(%r15)					;\
	movl	%r12d,(%r13)					;\
	movq	IS_PIC(%r14),%rdi				;\
	ioapic_asm_unlock(num)					;\
79:

#endif
