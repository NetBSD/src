/*	$NetBSD: types.h,v 1.9.82.1 2017/04/21 16:53:36 bouyer Exp $	*/

#ifndef _SHARK_TYPES_H_
#define	_SHARK_TYPES_H_

#include <arm/arm32/types.h>

/* We need to invoke FIQs indirectly. */
#define	__ARM_FIQ_INDIRECT

#define	__HAVE_DEVICE_REGISTER
#define	__HAVE_COMPAT_NETBSD32

#endif /* _SHARK_TYPES_H_ */
