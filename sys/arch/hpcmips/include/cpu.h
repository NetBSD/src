/*	$NetBSD: cpu.h,v 1.5 2000/04/15 22:05:52 soda Exp $	*/

#ifndef __HPCMIPS_CPU_H
#define __HPCMIPS_CPU_H

/*
 *  VR4100: Internal timer causes hard interrupt 5.
 */
#define MIPS_INT_MASK_CLOCK	MIPS_INT_MASK_5

#include <mips/cpu.h>
#include <mips/cpuregs.h> /* XXX */

#ifdef ENABLE_MIPS_TX3900
#define	INT_MASK_REAL_DEV	MIPS_HARD_INT_MASK
#else
#define	INT_MASK_REAL_DEV	MIPS3_HARD_INT_MASK	/* XXX */
#endif

#endif __HPCMIPS_CPU_H
