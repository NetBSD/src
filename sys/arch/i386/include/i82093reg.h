/*	 $NetBSD: i82093reg.h,v 1.5 2003/05/04 22:01:58 fvdl Exp $ */

#include <x86/i82093reg.h>

#ifdef _KERNEL

#if defined(_KERNEL_OPT)
#include "opt_multiprocessor.h"
#endif

#define ioapic_asm_ack(num) \
	movl	$0,_C_LABEL(local_apic)+LAPIC_EOI

#ifdef MULTIPROCESSOR

#define ioapic_asm_lock(num) \
	movl	$1,%esi						;\
77:								\
	xchgl	%esi,PIC_LOCK(%edi)				;\
	testl	%esi,%esi					;\
	jne	77b

#define ioapic_asm_unlock(num) \
	movl	$0,PIC_LOCK(%edi)
	
#else

#define ioapic_asm_lock(num)
#define ioapic_asm_unlock(num)

#endif	/* MULTIPROCESSOR */


#define ioapic_mask(num) \
	movl	IS_PIC(%ebp),%edi				;\
	ioapic_asm_lock(num)					;\
	movl	IS_PIN(%ebp),%esi				;\
	leal	0x10(%esi,%esi,1),%esi				;\
	movl	IOAPIC_SC_REG(%edi),%ebx			;\
	movl	%esi, (%ebx)					;\
	movl	IOAPIC_SC_DATA(%edi),%ebx			;\
	movl	(%ebx),%esi					;\
	orl	$IOAPIC_REDLO_MASK,%esi				;\
	movl	%esi,(%ebx)					;\
	ioapic_asm_unlock(num)

#define ioapic_unmask(num) \
	cmpl	$IREENT_MAGIC,(TF_ERR+4)(%esp)			;\
	jne	79f						;\
	movl	IS_PIC(%ebp),%edi				;\
	ioapic_asm_lock(num)					;\
	movl	IS_PIN(%ebp),%esi				;\
	leal	0x10(%esi,%esi,1),%esi				;\
	movl	IOAPIC_SC_REG(%edi),%ebx			;\
	movl	%esi, (%ebx)					;\
	movl	IOAPIC_SC_DATA(%edi),%ebx			;\
	movl	(%ebx),%esi					;\
	andl	$~IOAPIC_REDLO_MASK,%esi			;\
	movl	%esi,(%ebx)					;\
	ioapic_asm_unlock(num)					;\
79:

#endif
