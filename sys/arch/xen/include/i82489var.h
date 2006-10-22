/*	$NetBSD: i82489var.h,v 1.1.2.2 2006/10/22 06:05:20 yamt Exp $	*/

#include <x86/i82489var.h>
#undef lapic_cpu_number
#define lapic_cpu_number() (0)
