/*	$NetBSD: types.h,v 1.10 2017/01/26 15:55:10 christos Exp $	*/

#ifndef _SHARK_TYPES_H_
#define	_SHARK_TYPES_H_

#include <arm/arm32/types.h>

/* We need to invoke FIQs indirectly. */
#define	__ARM_FIQ_INDIRECT

#define	__HAVE_DEVICE_REGISTER
#define	__HAVE_COMPAT_NETBSD32

#endif /* _SHARK_TYPES_H_ */
