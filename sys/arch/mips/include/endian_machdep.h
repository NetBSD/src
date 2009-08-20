/*	$NetBSD: endian_machdep.h,v 1.1.154.1 2009/08/20 10:05:34 matt Exp $	*/

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
# if SZREG == 4
#  define REG_LHI   lwr
#  define REG_LLO   lwl
# else
#  define REG_LHI   ldr
#  define REG_LLO   ldl
# endif
#endif

#if _BYTE_ORDER == _BIG_ENDIAN
# define LWHI lwl
# define LWLO lwr
# define SWHI swl
# define SWLO swr
# if SZREG == 4
#  define REG_LHI   lwl
#  define REG_LLO   lwr
# else
#  define REG_LHI   ldl
#  define REG_LLO   ldr
# endif
#endif

#endif /* LOCORE */
