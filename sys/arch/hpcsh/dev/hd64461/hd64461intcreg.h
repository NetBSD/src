/*	$NetBSD: hd64461intcreg.h,v 1.1.4.2 2001/03/12 13:28:51 bouyer Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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

/* Interrupt Request Register */
#define HD64461_INTCNIRR_REG16				0xb0005000
#define HD64461_INTCNIRR_PCC0R			HD64461_INTC_PCC0
#define HD64461_INTCNIRR_PCC1R			HD64461_INTC_PCC1
#define HD64461_INTCNIRR_AFER			HD64461_INTC_AFE
#define HD64461_INTCNIRR_GPIOR			HD64461_INTC_GPIO
#define HD64461_INTCNIRR_TMU0R			HD64461_INTC_TMU0
#define HD64461_INTCNIRR_TMU1R			HD64461_INTC_TMU1
#define HD64461_INTCNIRR_IRDAR			HD64461_INTC_IRDA
#define HD64461_INTCNIRR_UARTR			HD64461_INTC_UART

/* Interrupt Mask Register */
#define HD64461_INTCNIMR_REG16				0xb0005002
#define HD64461_INTCNIMR_PCC0M			HD64461_INTC_PCC0
#define HD64461_INTCNIMR_PCC1M			HD64461_INTC_PCC1
#define HD64461_INTCNIMR_AFEM			HD64461_INTC_AFE
#define HD64461_INTCNIMR_GPIOM			HD64461_INTC_GPIO
#define HD64461_INTCNIMR_TMU0M			HD64461_INTC_TMU0
#define HD64461_INTCNIMR_TMU1M			HD64461_INTC_TMU1
#define HD64461_INTCNIMR_IRDAM			HD64461_INTC_IRDA
#define HD64461_INTCNIMR_UARTM			HD64461_INTC_UART

/* common defines. */
#define HD64461_INTC_PCC0			0x4000
#define HD64461_INTC_PCC1			0x2000
#define HD64461_INTC_AFE			0x1000
#define HD64461_INTC_GPIO			0x0800
#define HD64461_INTC_TMU0			0x0400
#define HD64461_INTC_TMU1			0x0200
#define HD64461_INTC_IRDA			0x0040
#define HD64461_INTC_UART			0x0020

