/*	 $NetBSD: i82093reg.h,v 1.1 2003/02/26 21:31:12 fvdl Exp $ */

#include <x86/i82093reg.h>

#define ioapic_asm_ack(num) \
	movl	$0,_C_LABEL(local_apic)(%rip)+LAPIC_EOI
