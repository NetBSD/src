/*	$NetBSD: cpu.h,v 1.13 2001/09/16 03:20:01 tsutsui Exp $	*/

#ifndef _NEWSMIPS_CPU_H_
#define _NEWSMIPS_CPU_H_

#include <mips/cpu.h>

#ifdef _KERNEL
#ifndef _LOCORE
extern int systype;

#define NEWS3400	1
#define NEWS5000	2
#endif
#endif

#endif /* _NEWSMIPS_CPU_H_ */
