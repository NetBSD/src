/*	$NetBSD: byte_swap_4.S,v 1.3.18.1 2007/08/28 17:35:30 matt Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Neil A. Carson
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

#include <machine/asm.h>

#if defined(_KERNEL) || defined(_STANDALONE)
_ENTRY(_C_LABEL(bswap32))
#else
_ENTRY(_C_LABEL(__bswap32))
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
_ENTRY(_C_LABEL(ntohl))
_ENTRY(_C_LABEL(htonl))
#endif
_PROF_PROLOGUE
#ifdef __ARM_ARCH_6
	rev		r0, r0
#elif defined(__thumb__)
	mov		r3, #16
	mov		r2, r0
	ror		r2, r2, r3
	mov		r1, r0
	eor		r1, r1, r2
	mov		r2, #0xff
	lsl		r2, r2, #16
	bic		r1, r1, r3
	mov		r2, #8
	ror		r0, r0, r2
	lsr		r1, r1, #8
	eor		r0, r0, r1
#else
	eor		r1, r0, r0, ror #16
	bic		r1, r1, #0x00FF0000
	mov		r0, r0, ror #8
	eor		r0, r0, r1, lsr #8
#endif
	RET
#if defined(_KERNEL) || defined(_STANDALONE)
	END(bswap32)
#else
	END(__bswap32)
#endif
