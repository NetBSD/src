/*	$NetBSD: cpu.h,v 1.2 2000/03/24 21:30:59 soren Exp $	*/

#define MIPS3_INTERNAL_TIMER_INTERRUPT

#include <mips/cpu.h>
#include <mips/cpuregs.h>

#define INT_MASK_REAL_DEV	MIPS_HARD_INT_MASK

/*
 * CTL_MACHDEP definitions.
 */
#define CPU_BOOTSTRING	1		/* Command given to the firmware. */
#define CPU_MAXID	2

#define CTL_MACHDEP_NAMES { \  
	{ 0, 0 }, \
	{ "bootstring", CTLTYPE_STRING }, \
}
