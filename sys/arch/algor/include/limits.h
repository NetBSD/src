/*	$NetBSD: limits.h,v 1.1 2001/05/28 16:22:19 thorpej Exp $	*/

#ifndef	_ALGOR_LIMITS_H_
#define	_ALGOR_LIMITS_H_

#include <mips/limits.h>

#ifdef _KERNEL
#define CLK_TCK		60		/* ticks per second */
#endif

#endif	/* !_ALGOR_LIMITS_H_ */
