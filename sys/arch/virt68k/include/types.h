/*	$NetBSD: types.h,v 1.1.2.1 2026/11/04 23:21:29 perseant Exp $	*/

#ifndef _VIRT68K_TYPES_H_
#define	_VIRT68K_TYPES_H_

#include <m68k/types.h>

#if defined(_KERNEL) && !defined(_MODULE)
#define	__HAVE_DEVICE_REGISTER
#endif

#endif /* _VIRT68K_TYPES_H_ */
