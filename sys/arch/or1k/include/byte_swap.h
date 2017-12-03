/* $NetBSD: byte_swap.h,v 1.2.18.2 2017/12/03 11:36:34 jdolecek Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#ifndef _OR1K_BYTE_SWAP_H_
#define	_OR1K_BYTE_SWAP_H_

#ifdef _LOCORE

#define	BSWAP16(_src, _dst, _tmp)	\
	l.extbz	_dst, _src;		\
	l.slli	_dst, _dst, 8;		\
	l.srli	_tmp, _src, 8;		\
	l.extbz	_tmp, _tmp;		\
	l.or	_dst, _dst, _tmp

#define BSWAP32(_src, _dst, _tmp)				\
	l.movhi	_tmp, 0xff00					;\
	l.ori	_tmp, _tmp, 0xff00	/* tmp = 0xff00ff00 */	;\
	l.slri	_dst, _tmp, 8		/* dst = 0x00ff00ff */	;\
	l.and	_tmp, _tmp, _src	/* tmp = 0xaa00cc00 */	;\
	l.and	_dst, _dst, _src	/* dst = 0x00bb00dd */	;\
	l.ror	_tmp, _tmp, 24		/* tmp = 0x00cc00aa */	;\
	l.ror	_dst, _dst, 8		/* dst = 0xdd00bb00 */	;\
	l.or	_dst, _dst, _tmp	/* dst = 0xddccbbaa */	;\

#else

#include <sys/types.h>
__BEGIN_DECLS

#define	__BYTE_SWAP_U64_VARIABLE __byte_swap_u64_variable
static __inline uint64_t
__byte_swap_u64_variable(uint64_t v)
{
	v =   ((v & 0x000000ff) << (56 -  0)) | ((v >> (56 -  0)) & 0x000000ff)
	    | ((v & 0x0000ff00) << (48 -  8)) | ((v >> (48 -  8)) & 0x0000ff00) 
	    | ((v & 0x00ff0000) << (40 - 16)) | ((v >> (40 - 16)) & 0x00ff0000)
	    | ((v & 0xff000000) << (32 - 24)) | ((v >> (32 - 24)) & 0xff000000);

	return v;
}

#define	__BYTE_SWAP_U32_VARIABLE __byte_swap_u32_variable
static __inline uint32_t
__byte_swap_u32_variable(uint32_t v)
{
	v =   ((v & 0x00ff) << (24 - 0)) | ((v >> (24 - 0)) & 0x00ff)
	    | ((v & 0xff00) << (16 - 8)) | ((v >> (16 - 8)) & 0xff00);

	return v;
}

#define	__BYTE_SWAP_U16_VARIABLE __byte_swap_u16_variable
static __inline uint16_t
__byte_swap_u16_variable(uint16_t v)
{
	v &= 0xffff;
	v = (v >> 8) | (v << 8);

	return v;
}

__END_DECLS

#endif	/* _LOCORE */

#endif /* _OR1K_BYTE_SWAP_H_ */
