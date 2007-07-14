/*	$NetBSD: types.h,v 1.18 2007/07/14 21:48:19 ad Exp $	*/

#ifndef _HP300_TYPES_H_
#define	_HP300_TYPES_H_

#include <m68k/types.h>

#define	__HAVE_DEVICE_REGISTER
#define	__HAVE_GENERIC_TODR
#define	__HAVE_TIMECOUNTER

#if defined(_KERNEL)
#define	__HAVE_RAS
#endif

#endif /* !_HP300_TYPES_H_ */
