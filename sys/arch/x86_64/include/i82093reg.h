/*	 $NetBSD: i82093reg.h,v 1.2 2003/03/05 23:56:01 fvdl Exp $ */

#include <x86/i82093reg.h>

#define ioapic_asm_ack(num) \
	movl	$0,(_C_LABEL(local_apic)+LAPIC_EOI)(%rip)
