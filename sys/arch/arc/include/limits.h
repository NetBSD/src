/*	$NetBSD: limits.h,v 1.5.6.2 2000/11/20 20:00:36 bouyer Exp $	*/
/*	$OpenBSD: limits.h,v 1.1.1.1 1996/06/24 09:07:17 pefo Exp $	*/
/*	NetBSD: limits.h,v 1.8 1995/03/28 18:19:16 jtc Exp 	*/

#ifndef	_MACHINE_LIMITS_H_
#define	_MACHINE_LIMITS_H_

#include <mips/limits.h>

#ifdef _KERNEL
#define CLK_TCK		100		/* ticks per second */
#endif

#endif /* _MACHINE_LIMITS_H_ */
