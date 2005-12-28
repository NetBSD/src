/*      $NetBSD: byte_swap.h,v 1.4 2005/12/28 18:40:13 perry Exp $	*/

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

static __inline u_int16_t _sh5_bswap16(u_int16_t);
static __inline u_int32_t _sh5_bswap32(u_int32_t);
static __inline u_int64_t _sh5_bswap64(u_int64_t);

static __inline u_int16_t
_sh5_bswap16(u_int16_t x)
{

	__asm volatile("byterev %0, %0; shlri %0, 32, %0; shlri.l %0, 16, %0"
	    : "+r"(x));

	return (x);
}

static __inline u_int32_t
_sh5_bswap32(u_int32_t x)
{

	__asm volatile("byterev %0, %0; shlri %0, 32, %0; add.l %0, r63, %0"
	    : "+r"(x));

	return (x);
}

static __inline u_int64_t
_sh5_bswap64(u_int64_t x)
{

	__asm volatile("byterev %0, %0" : "+r"(x));

	return (x);
}

#endif /* _SH5_BYTESWAP_H_ */
