/*	$NetBSD: cpu.h,v 1.2 2001/05/30 12:28:37 mrg Exp $	*/

#ifndef _ALGOR_CPU_H_
#define _ALGOR_CPU_H_

#include <mips/cpu.h>
#include <mips/cpuregs.h>

#ifndef _LOCORE
#if defined(_KERNEL_OPT)
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

#define	curcpu()		(&cpu_info_store)
#define	cpu_number()		(0)
#endif
#endif /* !_LOCORE */

#endif	/* !_ALGOR_CPU_H_ */
