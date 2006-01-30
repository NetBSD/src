/*      $NetBSD: byte_swap.h,v 1.5 2006/01/30 22:46:36 dsl Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH5_BYTESWAP_H_
#define	_SH5_BYTESWAP_H_

#include <sys/types.h>
#ifdef __GNUC__
#include <sys/types.h>
__BEGIN_DECLS


#define	__BYTE_SWAP_U16_VARIABLE __byte_swap_u16_variable
static __inline uint16_t
__byte_swap_u16_variable(uint16_t x)
{

	__asm volatile("byterev %0, %0; shlri %0, 32, %0; shlri.l %0, 16, %0"
	    : "+r"(x));

	return (x);
}

#define	__BYTE_SWAP_U32_VARIABLE __byte_swap_u32_variable
static __inline uint32_t
__byte_swap_u32_variable(uint32_t x)
{

	__asm volatile("byterev %0, %0; shlri %0, 32, %0; add.l %0, r63, %0"
	    : "+r"(x));

	return (x);
}

#define	__BYTE_SWAP_U64_VARIABLE __byte_swap_u64_variable
static __inline uint64_t
__byte_swap_u64_variable(uint64_t x)
{

	__asm volatile("byterev %0, %0" : "+r"(x));

	return (x);
}

__END_DECLS
#endif

#endif /* _SH5_BYTESWAP_H_ */
