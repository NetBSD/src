/*	$NetBSD: types.h,v 1.1 2002/02/10 01:57:46 thorpej Exp $	*/

#ifndef _ARM32_TYPES_H_
#define _ARM32_TYPES_H_

#include <arm/arm32/types.h>

/* We need to invoke FIQs indirectly. */
#define	__ARM_FIQ_INDIRECT

#define __HAVE_DEVICE_REGISTER
#define __HAVE_NWSCONS

#endif
