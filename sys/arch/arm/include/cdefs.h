/*	$NetBSD: cdefs.h,v 1.3.54.1 2012/04/17 00:06:05 yamt Exp $	*/

#ifndef	_MACHINE_CDEFS_H_
#define	_MACHINE_CDEFS_H_

#if defined (__ARM_ARCH_6__) || defined (__ARM_ARCH_6J__)
#define _ARM_ARCH_6
#endif

#if defined (_ARM_ARCH_6) || defined (__ARM_ARCH_5__) || \
    defined (__ARM_ARCH_5T__) || defined (__ARM_ARCH_5TE__) || \
    defined (__ARM_ARCH_5TEJ__)
#define _ARM_ARCH_5
#endif

#if defined (_ARM_ARCH_5) || defined (__ARM_ARCH_4T__)
#define _ARM_ARCH_4T
#endif

#define __ALIGNBYTES		(sizeof(int) - 1)

#endif /* !_MACHINE_CDEFS_H_ */
