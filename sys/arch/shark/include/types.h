/*	$NetBSD: types.h,v 1.9.54.1 2017/12/03 11:36:42 jdolecek Exp $	*/

#ifndef _SHARK_TYPES_H_
#define	_SHARK_TYPES_H_

#include <arm/arm32/types.h>

/* We need to invoke FIQs indirectly. */
#define	__ARM_FIQ_INDIRECT

#define	__HAVE_DEVICE_REGISTER
#define	__HAVE_COMPAT_NETBSD32

#endif /* _SHARK_TYPES_H_ */
