/*	$NetBSD: byte_swap.h,v 1.2 2000/07/01 06:43:42 matt Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

#define	__byte_swap_long_variable(x) __extension__	\
({ register u_int32_t __y, __x = (x);			\
							\
	__asm ("rotl	$-8, %1, %0;   			\
		insv	%0, $16, $8, %0;		\
		rotl	$8, %1, r1; 			\
		movb	r1, %0"				\
		: "=&r" (__y)				\
		: "r" (__x)				\
		: "r1", "cc" );				\
	__y; })

#define __byte_swap_word_variable(x) __extension__	\
({ register u_int16_t __x = (x);			\
							\
	(u_int16_t)(__x << 8 | __x >> 8);		\
})

#define __byte_swap_long(x)     __byte_swap_long_variable(x)
#define __byte_swap_word(x)     __byte_swap_word_variable(x)

#endif /* _VAX_BYTE_SWAP_H_ */
