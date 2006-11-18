/*	$NetBSD: i82489var.h,v 1.1.4.2 2006/11/18 21:29:39 ad Exp $	*/

#include <x86/i82489var.h>
#undef lapic_cpu_number
#define lapic_cpu_number() (0)
