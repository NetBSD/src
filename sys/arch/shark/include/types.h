/*	$NetBSD: types.h,v 1.2.2.2 2002/02/28 04:11:51 nathanw Exp $	*/

#ifndef _ARM32_TYPES_H_
#define _ARM32_TYPES_H_

#include <arm/arm32/types.h>

/* We need to invoke FIQs indirectly. */
#define	__ARM_FIQ_INDIRECT

#define __HAVE_DEVICE_REGISTER
#define __HAVE_NWSCONS

#endif
