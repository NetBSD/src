/*	$NetBSD: endian_machdep.h,v 1.1 2000/03/17 00:09:22 mycroft Exp $	*/

#ifndef _BYTE_ORDER
# error  Define MIPS target CPU endian-ness in port-specific header file.
#endif

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
