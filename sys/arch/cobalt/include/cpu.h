/*	$NetBSD: cpu.h,v 1.1 2000/03/19 23:07:46 soren Exp $	*/

#include <mips/cpu.h>
#include <mips/cpuregs.h>

#define MIPS3_INTERNAL_TIMER_INTERRUPT
#if 0
#define MIPS_INT_MASK_CLOCK	MIPS_INT_MASK_5
#endif

#define INT_MASK_REAL_DEV	MIPS_HARD_INT_MASK
