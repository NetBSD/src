/*	$NetBSD: limits.h,v 1.4 2000/01/23 20:08:25 soda Exp $	*/

#ifndef	_MACHINE_LIMITS_H_
#define	_MACHINE_LIMITS_H_

#include <mips/limits.h>

#ifdef _KERNEL
#define CLK_TCK		100		/* ticks per second */
#endif

#endif /* _MACHINE_LIMITS_H_ */
