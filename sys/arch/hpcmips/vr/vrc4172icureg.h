/*	$NetBSD: vrc4172icureg.h,v 1.2 2001/04/13 08:11:44 itojun Exp $	*/

/*
 * Copyright (c) 2000 SATO Kazumi.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

/*
 * Vrc4172 ICU unit register definition
 */
#define VRC2_ICU_1284INTRQ	0x0
#define 	VRC2_ICU_1284IRQ	0x08	/* interrupt to IRQ */
#define 	VRC2_ICU_1284INTRP	0x04	/* interrupt to INTRP */
#define 	VRC2_ICU_1284THRU	0x02	/* turu status */
#define 	VRC2_ICU_1284LATCH	0x01	/* latched status and clear */

#define VRC2_ICU_16550INTRQ	0x2
#define 	VRC2_ICU_16550THRU	0x02	/* turu status */
#define 	VRC2_ICU_16550LATCH	0x01	/* latched status and clear */

/* end */
