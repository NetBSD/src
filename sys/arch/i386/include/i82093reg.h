/*	 $NetBSD: i82093reg.h,v 1.4 2003/02/26 21:29:00 fvdl Exp $ */

#include <x86/i82093reg.h>

#define ioapic_asm_ack(num) \
	movl	$0,_C_LABEL(local_apic)+LAPIC_EOI
