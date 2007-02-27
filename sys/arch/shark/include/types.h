/*	$NetBSD: types.h,v 1.5.6.1 2007/02/27 16:53:04 yamt Exp $	*/

#ifndef _SHARK_TYPES_H_
#define	_SHARK_TYPES_H_

#include <arm/arm32/types.h>

/* We need to invoke FIQs indirectly. */
#define	__ARM_FIQ_INDIRECT

#define	__HAVE_DEVICE_REGISTER
#define	__HAVE_TIMECOUNTER
#define	__HAVE_GENERIC_TODR
#define	__HAVE_GENERIC_SOFT_INTERRUPTS

#endif /* _SHARK_TYPES_H_ */
