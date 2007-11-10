/*	$NetBSD: types.h,v 1.6.6.1.4.1 2007/11/10 02:56:57 matt Exp $	*/

#ifndef _EVBARM_TYPES_H_
#define	_EVBARM_TYPES_H_

#include <arm/arm32/types.h>

#ifndef __OLD_INTERRUPT_CODE		/* XXX */
#define	__HAVE_GENERIC_SOFT_INTERRUPTS
#endif
#define	__HAVE_DEVICE_REGISTER

#ifdef ARM_GENERIC_TODR
# define	__HAVE_GENERIC_TODR
#endif

#endif
