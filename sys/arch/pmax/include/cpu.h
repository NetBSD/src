/*	$NetBSD: cpu.h,v 1.21 2000/03/24 21:31:00 soren Exp $	*/

#ifndef _PMAX_CPU_H_
#define _PMAX_CPU_H_

/*
 * pmax uses standard mips1 convention, wiring FPU to hard interupt 5.
 */
#include <mips/cpu.h>
#include <mips/cpuregs.h>

#define MIPS_INT_MASK_FPU	MIPS_INT_MASK_5

#define	INT_MASK_REAL_DEV	(MIPS_HARD_INT_MASK &~ MIPS_INT_MASK_5)
#define	INT_MASK_FPU_DEAL	MIPS_INT_MASK_5

/*
 * CTL_MACHDEP definitions.
 */
#define CPU_CONSDEV		1	/* dev_t: console terminal device */
#define CPU_BOOTED_KERNEL	2	/* string: booted kernel name */
#define CPU_MAXID		3	/* number of valid machdep ids */

#define CTL_MACHDEP_NAMES { \
	{ 0, 0 }, \
	{ "console_device", CTLTYPE_STRUCT }, \
	{ "booted_kernel", CTLTYPE_STRING }, \
}

#endif	/* !_PMAX_CPU_H_ */
