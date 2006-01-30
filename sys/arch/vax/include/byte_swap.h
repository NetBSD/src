/*	$NetBSD: byte_swap.h,v 1.10 2006/01/30 22:46:36 dsl Exp $	*/

/*
 * Copyright (c) 1987, 1991 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)endian.h    7.8 (Berkeley) 4/3/91
 */

#ifndef _VAX_BYTE_SWAP_H_
#define _VAX_BYTE_SWAP_H_
#ifdef __GNUC__
#include <sys/types.h>
__BEGIN_DECLS

#include <sys/types.h>

#define	__BYTE_SWAP_U32_VARIABLE __byte_swap_u32_variable
static __inline uint32_t __attribute__((__unused__))
__byte_swap_u32_variable(uint32_t x)
{
	uint32_t y;

	__asm volatile(
		"rotl	$-8, %1, %0	\n"
		"insv	%0, $16, $8, %0	\n"
		"rotl	$8, %1, %%r1	\n"
		"movb	%%r1, %0"
		: "=&r" (y)
		: "r" (x)
		: "r1", "cc");

	return (y);
}

#define	__BYTE_SWAP_U16_VARIABLE __byte_swap_u16_variable
static __inline uint16_t __attribute__((__unused__))
__byte_swap_u16_variable(uint16_t x)
{

	return (x << 8 | x >> 8);
}

__END_DECLS
#endif
#endif /* _VAX_BYTE_SWAP_H_ */
