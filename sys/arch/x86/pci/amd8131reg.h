/*	$NetBSD: amd8131reg.h,v 1.1.2.3 2004/09/18 14:42:43 skrll Exp $	*/

/*
 * Some register definitions for the AMD 8131 PCI-X Tunnel / IO apic.
 * Only the registers/bits that are currently used are defined here.
 */

#define AMD8131_PCIX_MISC	0x40
#	define AMD8131_NIOAMODE		0x00000001

#define AMD8131_IOAPIC_CTL	0x44
#	define AMD8131_IOAEN		0x00000002
