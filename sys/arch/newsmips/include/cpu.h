/*	$NetBSD: cpu.h,v 1.3 1999/01/16 08:26:24 nisimura Exp $	*/

#ifndef	_MACHINE_CPU_H_
#define	_MACHINE_CPU_H_

#include <mips/cpu.h>
#include <mips/cpuregs.h>

#define MIPS_INT_MASK_FPU	MIPS_INT_MASK_3

#define	INT_MASK_REAL_DEV	(MIPS_HARD_INT_MASK &~ MIPS_INT_MASK_3)
#define	INT_MASK_FPU_DEAL	MIPS_INT_MASK_3

#endif	_MACHINE_CPU_H_
