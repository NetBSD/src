/*	$NetBSD: cpu.h,v 1.18 1998/03/25 08:35:39 jonathan Exp $	*/

#ifndef __PMAX_CPU_H
#define __PMAX_CPU_H

/*
 * pmax uses standard mips1 convention, wiring FPU to hard interupt 5.
 */
#include <mips/cpu.h>
#include <mips/cpuregs.h> /* XXX */

#define MIPS_INT_MASK_FPU	MIPS_INT_MASK_5
#endif __PMAX_CPU_H
