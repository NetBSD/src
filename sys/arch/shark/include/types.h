/*	$NetBSD: types.h,v 1.2.50.1 2006/09/09 02:43:07 rpaulo Exp $	*/

#ifndef _ARM32_TYPES_H_
#define	_ARM32_TYPES_H_

#include <arm/arm32/types.h>

/* We need to invoke FIQs indirectly. */
#define	__ARM_FIQ_INDIRECT

#define	__HAVE_DEVICE_REGISTER

#endif
