/*	$NetBSD: types.h,v 1.2.36.4 2008/01/21 09:39:20 yamt Exp $	*/

#ifndef _SHARK_TYPES_H_
#define	_SHARK_TYPES_H_

#include <arm/arm32/types.h>

/* We need to invoke FIQs indirectly. */
#define	__ARM_FIQ_INDIRECT

#define	__HAVE_DEVICE_REGISTER

#endif /* _SHARK_TYPES_H_ */
