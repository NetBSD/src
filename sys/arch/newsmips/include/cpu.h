/*	$NetBSD: cpu.h,v 1.12.2.1 2001/10/01 12:41:11 fvdl Exp $	*/

#ifndef _NEWSMIPS_CPU_H_
#define _NEWSMIPS_CPU_H_

#include <mips/cpu.h>

#if defined(_KERNEL) || defined(_STANDALONE)
#ifndef _LOCORE
extern int systype;

#define NEWS3400	1
#define NEWS5000	2
#endif
#endif

#endif /* _NEWSMIPS_CPU_H_ */
