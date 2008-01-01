/*	$NetBSD: cdefs.h,v 1.1.106.1 2008/01/01 15:39:29 chris Exp $	*/

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

#endif /* !_MACHINE_CDEFS_H_ */
