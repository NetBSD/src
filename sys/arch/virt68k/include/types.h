/*	$NetBSD: types.h,v 1.2 2026/07/21 14:35:28 thorpej Exp $	*/

#ifndef _VIRT68K_TYPES_H_
#define	_VIRT68K_TYPES_H_

#include <m68k/types.h>

#if defined(_KERNEL) && !defined(_MODULE)
#define	__HAVE_DEVICE_REGISTER
#endif

#endif /* _VIRT68K_TYPES_H_ */
