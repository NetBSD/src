/*	$NetBSD: types.h,v 1.5.32.2 2007/02/26 09:06:20 yamt Exp $	*/

#ifndef _EVBARM_TYPES_H_
#define	_EVBARM_TYPES_H_

#include <arm/arm32/types.h>

#ifndef __OLD_INTERRUPT_CODE		/* XXX */
#define	__HAVE_GENERIC_SOFT_INTERRUPTS
#endif
#define	__HAVE_DEVICE_REGISTER

#define	__HAVE_GENERIC_TODR

#endif
