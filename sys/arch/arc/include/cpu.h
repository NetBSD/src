/*	$NetBSD: cpu.h,v 1.17 2001/09/04 06:19:22 simonb Exp $	*/
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
#define	COPY_SIGCODE		/* copy sigcode above user stack in exec */

#define	INT_MASK_REAL_DEV	MIPS3_HARD_INT_MASK	/* XXX */

#ifndef _LOCORE
struct tlb;
extern void mips3_TLBWriteIndexedVPS __P((u_int index, struct tlb *tlb));
#endif /* ! _LOCORE */

#endif /* _ARC_CPU_H_ */
