/*	$NetBSD: endian_machdep.h,v 1.2 2004/08/06 18:33:09 uch Exp $	*/

/* Windows CE architecture */

#ifndef _LITTLE_ENDIAN
#define	_LITTLE_ENDIAN	1234
#endif
#ifndef LITTLE_ENDIAN
#define	LITTLE_ENDIAN	1234
#endif

#define	_BYTE_ORDER _LITTLE_ENDIAN
