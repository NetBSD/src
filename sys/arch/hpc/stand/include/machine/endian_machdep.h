/*	$NetBSD: endian_machdep.h,v 1.3 2005/12/11 12:17:29 christos Exp $	*/

/* Windows CE architecture */

#ifndef _LITTLE_ENDIAN
#define	_LITTLE_ENDIAN	1234
#endif
#ifndef LITTLE_ENDIAN
#define	LITTLE_ENDIAN	1234
#endif

#define	_BYTE_ORDER _LITTLE_ENDIAN
