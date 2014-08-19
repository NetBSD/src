/*	$NetBSD: cpu.h,v 1.13.22.1 2014/08/20 00:03:03 tls Exp $	*/

#include <mips/cpu.h>
#ifndef _LOCORE
int cpuname_printf(const char *, ...) __printflike(1, 2);
#endif
