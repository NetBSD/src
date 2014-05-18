/*	$NetBSD: cpu.h,v 1.13.26.1 2014/05/18 17:45:10 rmind Exp $	*/

#include <mips/cpu.h>
#ifndef _LOCORE
int cpuname_printf(const char *, ...) __printflike(1, 2);
#endif
