/*	$NetBSD: cpu.h,v 1.7 2000/04/14 10:11:06 tsubai Exp $	*/

#ifndef _MACHINE_CPU_H_
#define _MACHINE_CPU_H_

#include <mips/cpu.h>
#include <mips/cpuregs.h>

#ifndef _LOCORE
extern int systype;

#define NEWS3400	1
#define NEWS5000	2

#endif /* _LOCORE */
#endif /* _MACHINE_CPU_H_ */
