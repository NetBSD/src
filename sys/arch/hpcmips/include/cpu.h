/*	$NetBSD: cpu.h,v 1.3 2000/03/24 21:30:59 soren Exp $	*/

#ifndef __HPCMIPS_CPU_H
#define __HPCMIPS_CPU_H

/*
 *  VR4100: Internal timer causes hard interrupt 5.
 */
#define MIPS3_INTERNAL_TIMER_INTERRUPT
#define MIPS_INT_MASK_CLOCK	MIPS_INT_MASK_5

#include <mips/cpu.h>
#include <mips/cpuregs.h> /* XXX */

#ifdef ENABLE_MIPS_TX3900
#define	INT_MASK_REAL_DEV	MIPS_HARD_INT_MASK
#else
#define	INT_MASK_REAL_DEV	MIPS3_HARD_INT_MASK	/* XXX */
#endif

/*
 * CTL_MACHDEP definitions.
 */
#define CPU_CONSDEV		1	/* dev_t: console terminal device */
#define CPU_MAXID		2	/* number of valid machdep ids */

#define CTL_MACHDEP_NAMES { \
	{ 0, 0 }, \
	{ "console_device", CTLTYPE_STRUCT }, \
}

#endif __HPCMIPS_CPU_H
