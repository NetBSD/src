/*	$NetBSD: cpu.h,v 1.9 2000/03/24 21:30:58 soren Exp $	*/
/*	$OpenBSD: cpu.h,v 1.9 1998/01/28 13:46:10 pefo Exp $ */

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

/*
 * CTL_MACHDEP definitions.
 */
#define CPU_CONSDEV		1	/* dev_t: console terminal device */
#define CPU_MAXID		2	/* number of valid machdep ids */

#define CTL_MACHDEP_NAMES { \
	{ 0, 0 }, \
	{ "console_device", CTLTYPE_STRUCT }, \
}

#endif /* _ARC_CPU_H_ */
