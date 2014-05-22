/*	$NetBSD: cpu.h,v 1.13.12.1 2014/05/22 11:39:49 yamt Exp $	*/

#include <mips/cpu.h>
#ifndef _LOCORE
int cpuname_printf(const char *, ...) __printflike(1, 2);
#endif
