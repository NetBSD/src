/*	$NetBSD: types.h,v 1.14 2004/01/18 18:23:19 martin Exp $	*/

#ifndef _MACHINE_TYPES_H_
#define	_MACHINE_TYPES_H_

#include <m68k/types.h>

#define	__HAVE_DEVICE_REGISTER
#define	__HAVE_GENERIC_SOFT_INTERRUPTS

#if defined(_KERNEL)
#define	__HAVE_RAS
#endif

#endif
