/*	$NetBSD: limits.h,v 1.11.16.1 2000/11/20 20:20:30 bouyer Exp $	*/

#ifndef	_PMAX_LIMITS_H_
#define	_PMAX_LIMITS_H_

#include <mips/limits.h>

#ifdef _KERNEL
#define CLK_TCK		60		/* ticks per second */
#endif

#endif	/* !_PMAX_LIMITS_H_ */
