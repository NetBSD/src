/*	$NetBSD: endian.h,v 1.18 2000/03/16 15:09:36 mycroft Exp $	*/

#ifndef _MIPS_ENDIAN_H_
#define	_MIPS_ENDIAN_H_

#ifndef _BYTE_ORDER
# error  Define MIPS target CPU endian-ness in port-specific header file.
#endif

#include <sys/endian.h>

#ifdef _LOCORE

/*
 *   Endian-independent assembly-code aliases for unaligned memory accesses.
 */
#if _BYTE_ORDER == _LITTLE_ENDIAN
# define LWHI lwr
# define LWLO lwl
# define SWHI swr
# define SWLO swl
#endif

#if _BYTE_ORDER == _BIG_ENDIAN
# define LWHI lwl
# define LWLO lwr
# define SWHI swl
# define SWLO swr
#endif

#endif /* LOCORE */

#endif /* !_MIPS_ENDIAN_H_ */
