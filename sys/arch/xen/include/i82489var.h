/*	$NetBSD: i82489var.h,v 1.1 2006/09/28 18:53:15 bouyer Exp $	*/

#include <x86/i82489var.h>
#undef lapic_cpu_number
#define lapic_cpu_number() (0)
