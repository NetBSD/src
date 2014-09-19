/* $NetBSD: byte_swap.h,v 1.1 2014/09/19 17:36:26 matt Exp $ */

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

#ifndef _RISCV_BYTE_SWAP_H_
#define	_RISCV_BYTE_SWAP_H_

#ifdef _LOCORE

#define	BSWAP16(_src, _dst, _tmp)	\
	andi	_dst, _src, 0xff	;\
	slli	_dst, _dst, 8		;\
	srli	_tmp, _src, 8		;\
	and	_tmp, _tmp, 0xff	;\
	ori	_dst, _dst, _tmp

#define BSWAP32(_src, _dst, _tmp)	\
	li	v1, 0xff00		;\
	slli	_dst, _src, 24		;\
	srli	_tmp, _src, 24		;\
	ori	_dst, _dst, _tmp	;\
	and	_tmp, _src, v1		;\
	slli	_tmp, _src, 8		;\
	ori	_dst, _dst, _tmp	;\
	srli	_tmp, _src, 8		;\
	and	_tmp, _tmp, v1		;\
	ori	_dst, _dst, _tmp

#else

#include <sys/types.h>
__BEGIN_DECLS

#define	__BYTE_SWAP_U64_VARIABLE __byte_swap_u64_variable
static __inline uint64_t
__byte_swap_u64_variable(uint64_t v)
{
	v =   ((v & 0x000000ff) << (56 -  0)) | ((v >> (56 -  0)) & 0x000000ff)
	    | ((v & 0x0000ff00) << (48 -  8)) | ((v << (48 -  8)) & 0x0000ff00) 
	    | ((v & 0x00ff0000) << (40 - 16)) | ((v << (40 - 16)) & 0x00ff0000)
	    | ((v & 0xff000000) << (32 - 24)) | ((v << (32 - 24)) & 0xff000000);

	return v;
}

#define	__BYTE_SWAP_U32_VARIABLE __byte_swap_u32_variable
static __inline uint32_t
__byte_swap_u32_variable(uint32_t v)
{
	v =   ((v & 0x00ff) << (24 - 0)) | ((v >> (24 - 0)) & 0x00ff)
	    | ((v & 0xff00) << (16 - 8)) | ((v << (16 - 8)) & 0xff00);

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

#endif /* _RISCV_BYTE_SWAP_H_ */
