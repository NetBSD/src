/*	$NetBSD: byte_swap.h,v 1.4 2005/12/24 20:07:10 perry Exp $	*/

/*	$OpenBSD: endian.h,v 1.7 2001/06/29 20:28:54 mickey Exp $	*/

/*
 * Copyright (c) 1998-2001 Michael Shalayeff
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
 *	This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _HPPA_BYTE_SWAP_H_
#define	_HPPA_BYTE_SWAP_H_

static inline u_int16_t __byte_swap_word __P((u_int16_t));
static inline u_int32_t __byte_swap_long __P((u_int32_t));

static inline u_int32_t
__byte_swap_long(u_int32_t x)
{
	register in_addr_t __swap32md_x;	\
						\
	__asm  ("extru	%1, 7,8,%%r22\n\t"	\
		"shd	%1,%1,8,%0\n\t"		\
		"dep	%0,15,8,%0\n\t"		\
		"dep	%%r22,31,8,%0"		\
		: "=&r" (__swap32md_x)		\
		: "r" (x) : "r22");		\
	return(__swap32md_x);
}

#if 0
/*
 * Use generic C version because w/ asm inline below
 * gcc inserts extra "extru r,31,16,r" to convert
 * to 16 bit entity, which produces overhead we don't need.
 * Besides, gcc does swap16 same way by itself.
 */
#define	__swap16md(x)	__swap16gen(x)
#else
static inline u_int16_t
__byte_swap_word(u_int16_t x)
{
	register in_port_t __swap16md_x;				\
									\
	__asm  ("extru	%1,23,8,%0\n\t"					\
		"dep	%1,23,8,%0"					\
	       : "=&r" (__swap16md_x) : "r" (x));			\
	return(__swap16md_x);
}
#endif

#endif /* !_HPPA_BYTE_SWAP_H_ */
