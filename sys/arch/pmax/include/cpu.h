/*	$NetBSD: cpu.h,v 1.19 1999/01/16 02:36:01 nisimura Exp $	*/

#ifndef __PMAX_CPU_H
#define __PMAX_CPU_H

/*
 * pmax uses standard mips1 convention, wiring FPU to hard interupt 5.
 */
#include <mips/cpu.h>
#include <mips/cpuregs.h>

#define MIPS_INT_MASK_FPU	MIPS_INT_MASK_5

#define	INT_MASK_REAL_DEV	(MIPS_HARD_INT_MASK &~ MIPS_INT_MASK_5)
#define	INT_MASK_FPU_DEAL	MIPS_INT_MASK_5

#endif __PMAX_CPU_H
