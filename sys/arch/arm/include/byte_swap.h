/*	$NetBSD: byte_swap.h,v 1.3.2.2 2002/11/18 01:04:18 he Exp $	*/

/*-
 * Copyright (c) 1997, 1999, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum, Neil A. Carson, and Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_BYTE_SWAP_H_
#define	_ARM_BYTE_SWAP_H_

#include <sys/types.h>

static __inline u_int32_t
__byte_swap_long_variable(u_int32_t v)
{
	int tmp;

	__asm __volatile(
		"eor	%1, %2, %2, ror #16\n"
		"bic	%1, %1, #0x00ff0000\n"
		"mov	%0, %2, ror #8\n"
		"eor	%0, %0, %1, lsr #8"
	: "=r" (v), "=&r" (tmp)
	: "0" (v));

	return (v);
}

static __inline u_int16_t
__byte_swap_word_variable(u_int16_t v)
{

	__asm __volatile(
		"mov	%0, %1, ror #8\n"
		"orr	%0, %0, %0, lsr #16\n"
		"bic	%0, %0, %0, lsl #16"
	: "=r" (v)
	: "0" (v));

	return (v);
}

#ifdef __OPTIMIZE__

#define __byte_swap_long_constant(x)	\
	((((x) & 0xff000000) >> 24) |	\
	 (((x) & 0x00ff0000) >>  8) |	\
	 (((x) & 0x0000ff00) <<  8) |	\
	 (((x) & 0x000000ff) << 24))

#define	__byte_swap_word_constant(x)	\
	((((x) & 0xff00) >> 8) |	\
	 (((x) & 0x00ff) << 8))

#define	__byte_swap_long(x)		\
	(__builtin_constant_p((x)) ?	\
	 __byte_swap_long_constant(x) :	__byte_swap_long_variable(x))

#define	__byte_swap_word(x)		\
	(__builtin_constant_p((x)) ?	\
	 __byte_swap_word_constant(x) : __byte_swap_word_variable(x))

#else

#define	__byte_swap_long(x)	__byte_swap_long_variable(x)
#define	__byte_swap_word(x)	__byte_swap_word_variable(x)

#endif /* __OPTIMIZE__ */

#endif /* _ARM_BYTE_SWAP_H_ */
