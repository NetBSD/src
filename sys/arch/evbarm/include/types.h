/*	$NetBSD: types.h,v 1.6 2006/09/03 13:51:23 bjh21 Exp $	*/

#ifndef _EVBARM_TYPES_H_
#define	_EVBARM_TYPES_H_

#include <arm/arm32/types.h>

#ifndef __OLD_INTERRUPT_CODE		/* XXX */
#define	__HAVE_GENERIC_SOFT_INTERRUPTS
#endif
#define	__HAVE_DEVICE_REGISTER

#endif
