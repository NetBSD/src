/*	$NetBSD: i82489var.h,v 1.1.8.2 2006/12/30 20:47:25 yamt Exp $	*/

#include <x86/i82489var.h>
#undef lapic_cpu_number
#define lapic_cpu_number() (0)
