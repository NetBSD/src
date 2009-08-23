/*	$NetBSD: endian_machdep.h,v 1.1.154.2 2009/08/23 03:51:35 matt Exp $	*/

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
#  define REG_SHI   swr
#  define REG_SLO   swl
# else
#  define REG_LHI   ldr
#  define REG_LLO   ldl
#  define REG_SHI   sdr
#  define REG_SLO   sdl
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
#  define REG_SHI   swl
#  define REG_SLO   swr
# else
#  define REG_LHI   ldl
#  define REG_LLO   ldr
#  define REG_SHI   sdl
#  define REG_SLO   sdr
# endif
#endif

#endif /* LOCORE */
