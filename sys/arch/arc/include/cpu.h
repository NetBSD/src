/*	$NetBSD: cpu.h,v 1.7 2000/01/23 21:01:56 soda Exp $	*/
/*	$OpenBSD: cpu.h,v 1.8 1997/04/19 17:19:56 pefo Exp $	*/

#ifndef _ARC_CPU_H_
#define _ARC_CPU_H_

/*
 *  Internal timer causes hard interrupt 5.
 */
#define MIPS3_INTERNAL_TIMER_INTERRUPT
#define MIPS_INT_MASK_CLOCK	MIPS_INT_MASK_5

#include <mips/cpu.h>
#include <machine/cpuregs.h>

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
