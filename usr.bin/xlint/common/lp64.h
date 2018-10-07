/*	$NetBSD: lp64.h,v 1.6 2018/10/07 14:20:01 christos Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
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

/*
 * Type sizes for LP64 platforms (long, pointer: 64-bit)
 */

#define	CHAR_SIZE	(CHAR_BIT)
#define	SHORT_SIZE	(2 * CHAR_BIT)
#define	INT_SIZE	(4 * CHAR_BIT)
#define	LONG_SIZE	(8 * CHAR_BIT)
#define	QUAD_SIZE	(8 * CHAR_BIT)
#define	PTR_SIZE	(8 * CHAR_BIT)
#define	INT128_SIZE	(16 * CHAR_BIT)

#define	TARG_SCHAR_MAX	((signed char) (((unsigned char) -1) >> 1))
#define	TARG_SCHAR_MIN	((-TARG_CHAR_MAX) - 1)
#define	TARG_UCHAR_MAX	((unsigned char) -1)

#define	TARG_SHRT_MAX	((int16_t) (((uint16_t) -1) >> 1))
#define	TARG_SHRT_MIN	((-TARG_SHRT_MAX) - 1)
#define	TARG_USHRT_MAX	((uint16_t) -1)

#define	TARG_INT_MAX	((int32_t) (((uint32_t) -1) >> 1))
#define	TARG_INT_MIN	((-TARG_INT_MAX) - 1)
#define	TARG_UINT_MAX	((uint32_t) -1)

#define	TARG_LONG_MAX	TARG_QUAD_MAX
#define	TARG_LONG_MIN	TARG_QUAD_MIN
#define	TARG_ULONG_MAX	TARG_UQUAD_MAX

#define	TARG_QUAD_MAX	((int64_t) (((uint64_t) -1) >> 1))
#define	TARG_QUAD_MIN	((-TARG_QUAD_MAX) - 1)
#define	TARG_UQUAD_MAX	((uint64_t) -1)

#ifndef _LP64
/* XXX on a 32 build for a 64 build host we skip these */
#define	TARG_INT128_MAX		((__int128_t) (((__uint128_t) -1) >> 1))
#define	TARG_INT128_MIN		((-TARG_INT128_MAX) - 1)
#define	TARG_UINT128_MAX	((__uint128_t) -1)
#endif
