/*	$NetBSD: hd64465intcreg.h,v 1.1.16.2 2002/02/11 17:27:16 uch Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

/* Interrupt Request Register (16bit) */
#define HD64465_NIRR					0xb0005000
/* Interrupt Mask Register (16bit) */
#define HD64465_NIMR					0xb0005002
/* Interrupt Trigger Mode Register (16bit) */
#define HD64465_NITR					0xb0005004

/* common defines. */
#define   HD64465_PS2KB		0x8000
#define   HD64465_PCC0		0x4000
#define   HD64465_PCC1		0x2000
#define   HD64465_AFE		0x1000
#define   HD64465_GPIO		0x0800
#define   HD64465_TMU0		0x0400
#define   HD64465_TMU1		0x0200
#define   HD64465_KBC		0x0100
#define   HD64465_PS2MS		0x0080
#define   HD64465_IRDA		0x0040
#define   HD64465_UART		0x0020
#define   HD64465_PPR		0x0008
#define   HD64465_SCDI		0x0004
#define   HD64465_OHCI		0x0002
#define   HD64465_ADC		0x0001
