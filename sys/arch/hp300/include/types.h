/*	$NetBSD: types.h,v 1.13.6.1 2004/08/03 10:34:37 skrll Exp $	*/

#ifndef _MACHINE_TYPES_H_
#define	_MACHINE_TYPES_H_

#include <m68k/types.h>

#define	__HAVE_DEVICE_REGISTER
#define	__HAVE_GENERIC_SOFT_INTERRUPTS

#if defined(_KERNEL)
#define	__HAVE_RAS
#endif

#endif
