/*	$NetBSD: sa11x0_lcdreg.h,v 1.3.2.2 2001/03/12 13:28:33 bouyer Exp $ */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ichiro FUKUHARA.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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

/* #define SALCD_BASE           0xd0010000 */

/* size of I/O space */
#define SALCD_NPORTS      11

/* LCD framebuffer offset */
#define SALCD_12_16_OFFSET	0x20	/* 12BIT - 16BIT */
#define SALCD_8BIT_OFFSET	0x200	/*  8BIT */

/* LCD Control Register 0 */
#define SALCD_CR0	0
#define CR0_LEN		(1<<0)	/* LCD enable */
#define CR0_CMS		(1<<1)	/* color op enable */
#define CR0_SDS		(1<<2)	/* Single display or Double display */
#define CR0_LDM		(1<<3)	/* LDD status bit ignore(dont intrrupt) */
#define CR0_BAM		(1<<4)	/* Base address update does not 
						generate an intrrupt */
#define CR0_ERM		(1<<5)	/* Bus error generate an intrrupt */
#define CR0_PAS		(1<<7)	/* Passive / Active and TFT-LCD enable */
#define CR0_BLE		(1<<8)	/* endial select 0=little */
#define CR0_DPD		(1<<9)

/* LCD Control Register 1 */
#define SALCD_CR1	0x20
	/* PPL ; Pixel per line - 16 */
	/* HSW ; */
	/* ELW ; */
	/* BLW ; */

/* LCD Control Register 2 */
#define SALCD_CR2	0x24
	/* LPP ; Lines per panel */
	/* VSW ; */
	/* EFW ; */
	/* BFW ; */

/* LCD Control Register 3 */
#define SALCD_CR3	0x28
	/* PCD ; Pixel clock divisor	*/
	/* ACB ; */
	/* API ; AC Bias 		*/
	/* VSP ; Vertical sync		*/
	/* HSP ; Horizontal sync	*/
	/* PCP ; Pixel clock polarity	*/

/* DMA Channel 1 Base Address Register */
#define SALCD_BA1	0x10

/* DMA Channel 1 Current Address Register */
#define SALCD_CA1	0x14

/* DMA Channel 1 Base Address Register */
#define SALCD_BA2	0x18

/* DMA Channel 1 Current Address Register */
#define SALCD_CA2	0x1C

/* LCD Status Register */
#define SALCD_SR	0x04
#define SR_LDD		(1<<0)
#define SR_BAU		(1<<1)
#define SR_BER		(1<<2)
#define SR_ABC		(1<<3)
#define SR_IOL		(1<<4)
#define SR_IUL		(1<<5)
#define SR_IOU		(1<<6)
#define SR_IUU		(1<<7)
#define SR_OOL		(1<<8)
#define SR_OUL		(1<<9)
#define SR_OOU		(1<<10)
#define SR_OUU		(1<<11)

/* end of sa11x0_lcdreg.h */
