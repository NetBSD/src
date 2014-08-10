/* $NetBSD: fusu.S,v 1.1 2014/08/10 05:47:37 matt Exp $ */

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

#include <aarch64/asm.h>

#include "assym.h"

RCSID("$NetBSD: fusu.S,v 1.1 2014/08/10 05:47:37 matt Exp $");

/* LINTSTUB: int fubyte(const void *base); */

ENTRY(fubyte)
	mrs	x2, tpidr_el1
	ldr	x18, [x2, #CI_CURLWP]
	adr	x3, .Lfusu_onfault
	str	x3, [x18, #L_MD_ONFAULT]
	ldtrb	w0, [x0]
	str	xzr, [x18, #L_MD_ONFAULT]
	ret
END(fubyte)

/* LINTSTUB: int fusword(const void *base); */

ENTRY(fusword)
	mrs	x2, tpidr_el1
	ldr	x18, [x2, #CI_CURLWP]
	adr	x3, .Lfusu_onfault
	str	x3, [x18, #L_MD_ONFAULT]
	ldtrh	w0, [x0]
	str	xzr, [x18, #L_MD_ONFAULT]
	ret
END(fusword)

/* LINTSTUB: int fuswintr(const void *base); */

ENTRY(fuswintr)
	mrs	x2, tpidr_el1
	ldr	x18, [x2, #CI_CURLWP]
	adr	x3, .Lfusu_onfault
	str	x3, [x18, #L_MD_ONFAULT]
	ldtrh	w0, [x0]
	str	xzr, [x18, #L_MD_ONFAULT]
	ret
END(fuswintr)

/* LINTSTUB: long fuword(const void *base); */

ENTRY(fuword)
	mrs	x2, tpidr_el1
	ldr	x18, [x2, #CI_CURLWP]
	adr	x3, .Lfusu_onfault
	str	x3, [x18, #L_MD_ONFAULT]
	ldtr	x0, [x0]
	str	xzr, [x18, #L_MD_ONFAULT]
	ret
.Lfusu_onfault:
	mvn	x0, xzr
	ret
END(fuword)

/* LINTSTUB: int subyte(const void *base, int val); */

ENTRY(subyte)
	mrs	x2, tpidr_el1
	ldr	x18, [x2, #CI_CURLWP]
	adr	x3, .Lfusu_onfault
	str	x3, [x18, #L_MD_ONFAULT]
	sttrb	w1, [x0]
	str	xzr, [x18, #L_MD_ONFAULT]
	ret
END(subyte)

/* LINTSTUB: int susword(const void *base, int val); */

ENTRY(susword)
	mrs	x2, tpidr_el1
	ldr	x18, [x2, #CI_CURLWP]
	adr	x3, .Lfusu_onfault
	str	x3, [x18, #L_MD_ONFAULT]
	sttrh	w1, [x0]
	str	xzr, [x18, #L_MD_ONFAULT]
	ret
END(susword)

/* LINTSTUB: int suswintr(const void *base, int val); */

ENTRY(suswintr)
	mrs	x2, tpidr_el1
	ldr	x18, [x2, #CI_CURLWP]
	adr	x3, .Lfusu_onfault
	str	x3, [x18, #L_MD_ONFAULT]
	sttrh	w1, [x0]
	str	xzr, [x18, #L_MD_ONFAULT]
	ret
END(suswintr)

/* LINTSTUB: long suword(const void *base, int val); */

ENTRY(suword)
	mrs	x2, tpidr_el1
	ldr	x18, [x2, #CI_CURLWP]
	adr	x3, .Lfusu_onfault
	str	x3, [x18, #L_MD_ONFAULT]
	sttr	x1, [x0]
	str	xzr, [x18, #L_MD_ONFAULT]
	ret
END(suword)
