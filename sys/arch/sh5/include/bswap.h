/*      $NetBSD: bswap.h,v 1.1 2002/07/05 13:31:56 scw Exp $	*/

/*
 * Based on Manuel Bouyer's public domain code.
 * The following copyright applies to the SH-5 changes:
 */

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

#ifndef _SH5_BSWAP_H_
#define	_SH5_BSWAP_H_

#include <sys/cdefs.h>

#ifndef _KERNEL

__BEGIN_DECLS
u_int16_t bswap16(u_int16_t);
u_int32_t bswap32(u_int32_t);
u_int64_t bswap64(u_int64_t);
__END_DECLS

#else /* _KERNEL */

__BEGIN_DECLS
static __inline u_int16_t bswap16(u_int16_t);
static __inline u_int32_t bswap32(u_int32_t);
static __inline u_int64_t bswap64(u_int64_t);
__END_DECLS

static __inline u_int16_t
bswap16(u_int16_t x)
{
	u_int64_t rval;

	__asm __volatile ("byterev %1,%0" : "=r"(rval) : "r"(x));

	return ((u_int16_t)(rval >> 48));
}

static __inline u_int32_t
bswap32(u_int32_t x)
{
	u_int64_t rval;

	__asm __volatile ("byterev %1,%0" : "=r"(rval) : "r"(x));

	return ((u_int32_t)(rval >> 32));
}

static __inline u_int64_t
bswap64(u_int64_t x)
{
	u_int64_t rval;

	__asm __volatile ("byterev %1,%0" : "=r"(rval) : "r"(x));

	return (rval);
}
#endif /* _KERNEL */

#endif /* _SH5_BSWAP_H_ */
