/*	$NetBSD: cpu.h,v 1.15 2000/05/26 21:19:29 thorpej Exp $	*/
/*	$OpenBSD: cpu.h,v 1.9 1998/01/28 13:46:10 pefo Exp $ */

#ifndef _ARC_CPU_H_
#define _ARC_CPU_H_

/*
 *  Internal timer causes hard interrupt 5.
 */
#define MIPS_INT_MASK_CLOCK	MIPS_INT_MASK_5

#include <mips/cpu.h>
#include <mips/cpuregs.h>

#ifndef _LOCORE
#if defined(_KERNEL) && !defined(_LKM)
#include "opt_lockdebug.h"
#endif

#include <sys/sched.h>
struct cpu_info {
	struct schedstate_percpu ci_schedstate; /* scheduler state */
#if defined(DIAGNOSTIC) || defined(LOCKDEBUG)
	u_long ci_spin_locks;		/* # of spin locks held */
	u_long ci_simple_locks;		/* # of simple locks held */
#endif
};

#ifdef _KERNEL
extern struct cpu_info cpu_info_store;

#define	curcpu()	(&cpu_info_store)
#define	cpu_number()	(0)
#endif
#endif /* !_LOCORE */

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
