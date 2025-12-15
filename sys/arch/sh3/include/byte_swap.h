/*      $NetBSD: byte_swap.h,v 1.5 2025/12/15 22:10:34 nia Exp $      */

/* Written by Manuel Bouyer. Public domain */

#ifndef _SH3_BYTE_SWAP_H_
#define	_SH3_BYTE_SWAP_H_

#include <sys/cdefs.h>

#ifdef  __GNUC__
#include <sys/stdint.h>
__BEGIN_DECLS 

#define __BYTE_SWAP_U16_VARIABLE __byte_swap_u16_variable
static __inline uint16_t
__byte_swap_u16_variable(uint16_t x)
{
	uint16_t rval;

	__asm volatile ("swap.b %1,%0" : "=r"(rval) : "r"(x));

	return (rval);
}

#define __BYTE_SWAP_U32_VARIABLE __byte_swap_u32_variable
static __inline uint32_t
__byte_swap_u32_variable(uint32_t x)
{
	uint32_t rval;

	__asm volatile ("swap.b %1,%0; swap.w %0,%0; swap.b %0,%0"
			  : "=r"(rval) : "r"(x));

	return (rval);
}

__END_DECLS
#endif /* __GNUC_ */

#endif /* !_SH3_BYPE_SWAP_H_ */
