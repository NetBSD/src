/*	$NetBSD: endian.h,v 1.17 1999/08/21 05:53:51 simonb Exp $	*/

#ifndef _MACHINE_ENDIAN_H_
#define	_MACHINE_ENDIAN_H_

#ifndef _BYTE_ORDER
# error  Define MIPS target CPU endian-ness in port-specific header file.
#endif

#include <sys/endian.h>

#ifdef _LOCORE
/*
 *   Endian-independent assembly-code aliases for unaligned memory accesses.
 */
#if BYTE_ORDER == LITTLE_ENDIAN
# define LWHI lwr
# define LWLO lwl
# define SWHI swr
# define SWLO swl
#endif /* BYTE_ORDER == LITTLE_ENDIAN */

#if BYTE_ORDER == BIG_ENDIAN
# define LWHI lwl
# define LWLO lwr
# define SWHI swl
# define SWLO swr
#endif /* BYTE_ORDER == BIG_ENDIAN */

#endif /* LOCORE */

#endif /* !_MACHINE_ENDIAN_H_ */
