/*	$NetBSD: strncasecmp.S,v 1.1 2005/12/20 19:28:50 christos Exp $	*/

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

#include <machine/asm.h>

/*
 * int strncasecmp(const char *s1, const char *s2, size_t len)
 *
 * Note:
 * We assume that this routine is mostly called for short strings, and that
 * the overhead of aligning s1/s2 to permit long or quad comparisons is
 * just Not Worth The Effort.
 */

#ifdef WEAK_ALIAS
WEAK_ALIAS(strncasecmp,_strncasecmp)
#endif

ENTRY(_strncasecmp)
#ifndef _LP64
	add.l	r2, r63, r5
	add.l	r3, r63, r3
	addz.l	r4, r63, r4
#else
	add	r2, r63, r5
#endif
	pta/l	1f, tr1
	pta/l	2f, tr2
	pta/u	3f, tr3
	ptabs/u	r18, tr0
	movi	0, r2
	beq/u	r4, r63, tr0		/* Bail if "len" is zero */
	movi	0, r17
	movi	'a', r6
	movi	'z', r7
	movi	'A', r8
	movi	'Z', r9

1:	ldx.ub	r5, r17, r0		/* r0 = *s1 */
	ldx.ub	r3, r17, r1		/* r1 = *s2 */
	beq/u	r0, r63, tr3		/* If *s1 == '\0', return result */
	beq/l	r0, r1, tr2		/* Loop if the two characters match */

	/*
	 * if (r0 >= 'A' && r0 <= 'Z' && r1 >= 'a' && r1 <= 'z')
	 *     r0 = tolower(r0);
	 */
	cmpgtu	r8, r0, r19		/* r19 == 1 IFF r0 < 'A', else 0 */
	cmpgtu	r0, r9, r20		/* r20 == 1 IFF r0 > 'Z', else 0 */
	cmpgtu	r6, r1, r21		/* r21 == 1 IFF r1 < 'a', else 0 */
	cmpgtu	r1, r7, r22		/* r22 == 1 IFF r1 > 'z', else 0 */
	or	r19, r20, r19		/* r19 == 0 if 'A' <= r0 <= 'Z' */
	or	r21, r22, r21		/* r21 == 0 if 'a' <= r1 <= 'z' */
	or	r19, r21, r19		/* Combine results */
	addi	r0, 'a' - 'A', r20
	cmveq	r19, r20, r0		/* r0 = tolower(r0) */
	beq/u	r19, r63, tr2

	/*
	 * else if (r0 >= 'a' && r0 <= 'a' && r1 >= 'A' && r1 <= 'Z')
	 *     r1 = tolower(r1);
	 */
	cmpgtu	r6, r0, r19		/* r19 == 1 IFF r0 < 'a', else 0 */
	cmpgtu	r0, r7, r20		/* r20 == 1 IFF r0 > 'z', else 0 */
	cmpgtu	r8, r1, r21		/* r21 == 1 IFF r1 < 'A', else 0 */
	cmpgtu	r1, r9, r22		/* r22 == 1 IFF r1 > 'Z', else 0 */
	or	r19, r20, r19		/* r19 == 0 if 'a' <= r0 <= 'a' */
	or	r21, r22, r21		/* r21 == 0 if 'A' <= r1 <= 'Z' */
	or	r19, r21, r19		/* Combine results */
	addi	r1, 'a' - 'A', r20
	cmveq	r19, r20, r1		/* r1 = tolower(r1) */

2:	addi	r17, 1, r17		/* s1++, s2++ */
	beq/u	r4, r17, tr3		/* Return now if compared "len" bytes */
	beq/l	r0, r1, tr1		/* Back for more if they match */
3:	sub	r0, r1, r2		/* r2 = r0 - r1 */
	blink	tr0, r63		/* Return result to caller */
