/*	$NetBSD: cpu.h,v 1.9 2000/05/30 11:42:05 tsubai Exp $	*/

#ifndef _MACHINE_CPU_H_
#define _MACHINE_CPU_H_

#include <mips/cpu.h>
#include <mips/cpuregs.h>

#ifndef _LOCORE
#if defined(_KERNEL) && !defined(_LKM)
#include "opt_lockdebug.h"
#endif

extern int systype;

#define NEWS3400	1
#define NEWS5000	2

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

#define	curcpu()		(&cpu_info_store)
#define	cpu_number()		0
#endif /* _KERNEL */

#endif /* _LOCORE */
#endif /* _MACHINE_CPU_H_ */
