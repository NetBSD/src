/*	$NetBSD: limits.h,v 1.12 2000/01/09 15:34:43 ad Exp $	*/

#ifndef	_PMAX_LIMITS_H_
#define	_PMAX_LIMITS_H_

#include <mips/limits.h>

#ifdef _KERNEL
#define CLK_TCK		60		/* ticks per second */
#endif

#endif	/* !_PMAX_LIMITS_H_ */
