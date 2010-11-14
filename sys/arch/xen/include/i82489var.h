/*	$NetBSD: i82489var.h,v 1.2 2010/11/14 13:40:31 bouyer Exp $	*/

#include <x86/i82489var.h>

/*
 * Xen doesn't give acces to the lapic. In addition, the lapic number provided
 * by the dom0 to Xen when setting up iopics is ignored, the hypervisor will
 * decide itself to which physical CPU the interrupt should be routed to.
 */
#undef lapic_cpu_number
#define lapic_cpu_number() (0)
