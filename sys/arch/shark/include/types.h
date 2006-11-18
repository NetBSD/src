/*	$NetBSD: types.h,v 1.3.2.1 2006/11/18 21:29:31 ad Exp $	*/

#ifndef _ARM32_TYPES_H_
#define	_ARM32_TYPES_H_

#include <arm/arm32/types.h>

/* We need to invoke FIQs indirectly. */
#define	__ARM_FIQ_INDIRECT

#define	__HAVE_DEVICE_REGISTER
#define	__HAVE_TIMECOUNTER
#define	__HAVE_GENERIC_TODR

#endif
