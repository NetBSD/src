/*	$NetBSD: types.h,v 1.13.6.3 2004/09/21 13:15:27 skrll Exp $	*/

#ifndef _MACHINE_TYPES_H_
#define	_MACHINE_TYPES_H_

#include <m68k/types.h>

#define	__HAVE_DEVICE_REGISTER
#define	__HAVE_GENERIC_SOFT_INTERRUPTS

#if defined(_KERNEL)
#define	__HAVE_RAS
#endif

#endif
