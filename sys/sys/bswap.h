/*      $NetBSD: bswap.h,v 1.21 2025/11/29 15:53:02 riastradh Exp $      */

/* Written by Manuel Bouyer. Public domain */

#ifndef _SYS_BSWAP_H_
#define _SYS_BSWAP_H_

#ifndef _LOCORE
#include <sys/stdint.h>

#include <machine/bswap.h>

__BEGIN_DECLS
/* Always declare the functions in case their address is taken (etc) */
#if defined(_KERNEL) || defined(_STANDALONE) || !defined(__BSWAP_RENAME)
uint16_t bswap16(uint16_t) __constfunc;
uint32_t bswap32(uint32_t) __constfunc;
#else
uint16_t bswap16(uint16_t) __RENAME(__bswap16) __constfunc;
uint32_t bswap32(uint32_t) __RENAME(__bswap32) __constfunc;
#endif
uint64_t bswap64(uint64_t) __constfunc;
__END_DECLS

#if defined(__GNUC__) && !defined(__lint__)

#define	__byte_swap_u64_constexpr(x) \
	(__CAST(uint64_t, \
	 ((((x) & 0xff00000000000000ull) >> 56) | \
	  (((x) & 0x00ff000000000000ull) >> 40) | \
	  (((x) & 0x0000ff0000000000ull) >> 24) | \
	  (((x) & 0x000000ff00000000ull) >>  8) | \
	  (((x) & 0x00000000ff000000ull) <<  8) | \
	  (((x) & 0x0000000000ff0000ull) << 24) | \
	  (((x) & 0x000000000000ff00ull) << 40) | \
	  (((x) & 0x00000000000000ffull) << 56))))

#define	__byte_swap_u32_constexpr(x) \
	(__CAST(uint32_t, \
	((((x) & 0xff000000) >> 24) | \
	 (((x) & 0x00ff0000) >>  8) | \
	 (((x) & 0x0000ff00) <<  8) | \
	 (((x) & 0x000000ff) << 24))))

#define	__byte_swap_u16_constexpr(x) \
	(__CAST(uint16_t, \
	((((x) & 0xff00) >> 8) | \
	 (((x) & 0x00ff) << 8))))

/*
 * The compiler always generates an expensive function call to bswap
 * on some architectures, we want the inline versions there.
 */
#ifdef __HAVE_SLOW_BSWAP_BUILTIN

static __inline uint64_t
__byte_swap_u64_inline(uint64_t x)
{
	return __byte_swap_u64_constexpr(x);
}

static __inline uint32_t
__byte_swap_u32_inline(uint32_t x)
{
	return __byte_swap_u32_constexpr(x);
}

static __inline uint16_t
__byte_swap_u16_inline(uint16_t x)
{
	return __byte_swap_u16_constexpr(x);
}

#define	__BYTE_SWAP_U64_VARIABLE __byte_swap_u64_inline
#define	__BYTE_SWAP_U32_VARIABLE __byte_swap_u32_inline
#define	__BYTE_SWAP_U16_VARIABLE __byte_swap_u16_inline

#else /* !__HAVE_SLOW_BSWAP_BUILTIN */

/* allow machine/bswap.h to override these with inline versions */
#ifndef __BYTE_SWAP_U64_VARIABLE
#define	__BYTE_SWAP_U64_VARIABLE bswap64
#endif

#ifndef __BYTE_SWAP_U32_VARIABLE
#define	__BYTE_SWAP_U32_VARIABLE bswap32
#endif

#ifndef __BYTE_SWAP_U16_VARIABLE
#define	__BYTE_SWAP_U16_VARIABLE bswap16
#endif

#endif /* __HAVE_SLOW_BSWAP_BUILTIN */

#define	bswap64(x) \
	__CAST(uint64_t, __builtin_constant_p((x)) ? \
	 __byte_swap_u64_constexpr(x) : __BYTE_SWAP_U64_VARIABLE(x))

#define	bswap32(x) \
	__CAST(uint32_t, __builtin_constant_p((x)) ? \
	 __byte_swap_u32_constexpr(x) : __BYTE_SWAP_U32_VARIABLE(x))

#define	bswap16(x) \
	__CAST(uint16_t, __builtin_constant_p((x)) ? \
	 __byte_swap_u16_constexpr(x) : __BYTE_SWAP_U16_VARIABLE(x))

#endif /* __GNUC__ && !__lint__ */
#endif /* !_LOCORE */

#endif /* !_SYS_BSWAP_H_ */
