/*	$NetBSD: sa11x1_pcicreg.h,v 1.1 2001/03/10 19:01:27 toshii Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro.
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

/* Register locations */
#define SACPCIC_CR	0x1800
#define SACPCIC_SSR	0x1804
#define SACPCIC_SR	0x1808

/* Control register */
#define CR_S0_RST	0x01	/* 1 = Assert reset */
#define CR_S1_RST	0x02
#define CR_S0_FLT	0x04	/* 0 = Float all S0 control lines */
#define CR_S1_FLT	0x08
#define CR_S0_PWAITEN	0x10	/* S0_nPWAIT enable */
#define CR_S1_PWAITEN	0x20
#define CR_S0_PSE	0x40	/* 0 = 3V card */
#define CR_S1_PSE	0x80

/* Sleep state register */
#define SSR_S0		0x01
#define SSR_S1		0x02

/* Status register */
#define SR_S0_READY		0x0001
#define SR_S1_READY		0x0002
#define SR_S0_CARDDETECT	0x0004
#define SR_S1_CARDDETECT	0x0008
#define SR_S0_VS1		0x0010
#define SR_S0_VS2		0x0020
#define SR_S1_VS1		0x0040
#define SR_S1_VS2		0x0080
#define SR_S0_WP		0x0100
#define SR_S1_WP		0x0200
#define SR_S0_BVD1		0x0400
#define SR_S0_BVD2		0x0800
#define SR_S1_BVD1		0x1000
#define SR_S1_BVD2		0x2000

/* IRQ numbers */
#define IRQ_S0_READY		49
#define IRQ_S1_READY		50
#define IRQ_S0_CDVALID		51
#define IRQ_S1_CDVALID		52
#define IRQ_S0_BVD1		53
#define IRQ_S1_BVD1		54
