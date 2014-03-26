/*	$NetBSD: cpu.h,v 1.14 2014/03/26 17:53:36 christos Exp $	*/

#include <mips/cpu.h>
#ifndef _LOCORE
int cpuname_printf(const char *, ...) __printflike(1, 2);
#endif
