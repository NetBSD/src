/*	$NetBSD: infinity.c,v 1.2 2001/10/28 12:40:56 bjh21 Exp $	*/

/*
 * Copyright (c) 1996 Mark Brinicombe.
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
 *	This product includes software developed by Mark Brinicombe
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <math.h>
#include <machine/endian.h>

/* Bytes for +infinity on an ARM (IEEE double precision) */

#if BYTE_ORDER == BIG_ENDIAN
const char __infinity[] __attribute__((__aligned__(4))) =
    { 0x7f, (char)0xf0, 0, 0, 0, 0, 0, 0 };
#else /* LITTLE_ENDIAN */
#ifdef __VFP_FP__
const char __infinity[] __attribute__((__aligned__(4))) =
    { 0, 0, 0, 0, 0, 0, (char)0xf0, 0x7f };
#else /* !__VFP_FP__ */
const char __infinity[] __attribute__((__aligned__(4))) =
    { 0, 0, (char)0xf0, 0x7f, 0, 0, 0, 0 };
#endif /* !__VFP_FP__ */
#endif /* LITTLE_ENDIAN */
