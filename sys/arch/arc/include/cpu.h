/*	$NetBSD: cpu.h,v 1.20 2005/11/15 13:45:30 tsutsui Exp $	*/
/*	$OpenBSD: cpu.h,v 1.9 1998/01/28 13:46:10 pefo Exp $ */

#ifndef _ARC_CPU_H_
#define _ARC_CPU_H_

/*
 *  Internal timer causes hard interrupt 5.
 */
#define MIPS_INT_MASK_CLOCK	MIPS_INT_MASK_5

#include <mips/cpu.h>
#include <mips/cpuregs.h>

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#define	INT_MASK_REAL_DEV	MIPS3_HARD_INT_MASK	/* XXX */

#endif /* _ARC_CPU_H_ */
