/*	$NetBSD: types.h,v 1.4 2006/09/13 07:13:03 gdamore Exp $	*/

#ifndef _ARM32_TYPES_H_
#define	_ARM32_TYPES_H_

#include <arm/arm32/types.h>

/* We need to invoke FIQs indirectly. */
#define	__ARM_FIQ_INDIRECT

#define	__HAVE_DEVICE_REGISTER
#define	__HAVE_TIMECOUNTER

#endif
