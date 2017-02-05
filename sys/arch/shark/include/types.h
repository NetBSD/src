/*	$NetBSD: types.h,v 1.9.74.1 2017/02/05 13:40:20 skrll Exp $	*/

#ifndef _SHARK_TYPES_H_
#define	_SHARK_TYPES_H_

#include <arm/arm32/types.h>

/* We need to invoke FIQs indirectly. */
#define	__ARM_FIQ_INDIRECT

#define	__HAVE_DEVICE_REGISTER
#define	__HAVE_COMPAT_NETBSD32

#endif /* _SHARK_TYPES_H_ */
