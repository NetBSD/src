/*	$NetBSD: cpu.h,v 1.1.1.1 1999/09/16 12:23:22 takemura Exp $	*/

#ifndef __HPCMIPS_CPU_H
#define __HPCMIPS_CPU_H

/*
 *  VR4100: Internal timer causes hard interrupt 5.
 */
#define MIPS3_INTERNAL_TIMER_INTERRUPT
#define MIPS_INT_MASK_CLOCK	MIPS_INT_MASK_5

#include <mips/cpu.h>
#include <mips/cpuregs.h> /* XXX */

#if 0
#define	INT_MASK_REAL_DEV	MIPS_HARD_INT_MASK	/* XXX */
#endif
#define	INT_MASK_REAL_DEV	MIPS3_HARD_INT_MASK	/* XXX */

#endif __HPCMIPS_CPU_H
