/*	$NetBSD: trap.h,v 1.1.6.2 2000/11/20 19:59:51 bouyer Exp $ */

#ifndef _MACHINE_TRAP_H_
#define _MACHINE_TRAP_H_

#include <powerpc/trap.h>

/* offset when MSR[IP] is set */
#define EXC_UPPER 0xfff00000

#endif /* _MACHINE_TRAP_H_ */
