/*	$NetBSD: limits.h,v 1.11 1998/01/09 22:24:01 perry Exp $	*/

#ifndef	_MACHINE_LIMITS_H_
#define	_MACHINE_LIMITS_H_

#include <mips/limits.h>

#ifdef _KERNEL
#define CLK_TCK		60		/* ticks per second */
#endif

#endif /* _MACHINE_LIMITS_H_ */
