/*	$NetBSD: cpu.h,v 1.4 1999/12/22 05:55:26 tsubai Exp $	*/

#ifndef _MACHINE_CPU_H_
#define _MACHINE_CPU_H_

#include <mips/cpu.h>
#include <mips/cpuregs.h>

#define MIPS_INT_MASK_FPU	MIPS_INT_MASK_3

#define INT_MASK_REAL_DEV	(MIPS_HARD_INT_MASK &~ MIPS_INT_MASK_3)
#define INT_MASK_FPU_DEAL	MIPS_INT_MASK_3

#ifndef _LOCORE
extern int systype;

#define NEWS3400	1
#define NEWS5000	2

#endif /* _LOCORE */
#endif /* _MACHINE_CPU_H_ */
