/*	$NetBSD: limits.h,v 1.3 1998/01/09 22:24:00 perry Exp $	*/

#ifndef	_MACHINE_LIMITS_H_
#define	_MACHINE_LIMITS_H_

#include <mips/limits.h>

#ifdef _KERNEL
#define CLK_TCK		100		/* ticks per second */
#endif

#endif /* _MACHINE_LIMITS_H_ */
