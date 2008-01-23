/*	$NetBSD: types.h,v 1.8.8.1 2008/01/23 19:27:25 bouyer Exp $	*/

#ifndef _SHARK_TYPES_H_
#define	_SHARK_TYPES_H_

#include <arm/arm32/types.h>

/* We need to invoke FIQs indirectly. */
#define	__ARM_FIQ_INDIRECT

#define	__HAVE_DEVICE_REGISTER

#endif /* _SHARK_TYPES_H_ */
