/*	$NetBSD: sa11x0_mcpreg.h,v 1.2 2001/07/12 03:58:35 ichiro Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.  All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ichiro FUKUHARA (ichiro@ichiro.org).
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

/* SA11[01]0 MCP(Multimedia communications Port) */

#define SAMCP_NPORTS	7

#define SAMCP_CR0	0x00	/* MCP control register 0 */
/* MCP control register 1 locate PPC area */

#define SAMCP_DR0	0x08	/* MCP data register 0 */
#define SAMCP_DR1	0x0C	/* MCP data register 1 */
#define SAMCP_DR2	0x10	/* MCP data register 2 */
#define SAMCP_SR	0x18	/* MCP status register */

/* MCP control register 0*/
#define CR0_ASD
#define CR0_TSD
#define CR0_MCE		(1 << 16)	/* MCP enable */
#define CR0_ECS		(1 << 17)	/* External clock used */ 
#define CR0_ADM		(1 << 18)	/* A/D sampling mode */
#define CR0_TTE		(1 << 19)	/* Telecom tx FIFO intr enable */
#define CR0_TRE		(1 << 20)	/* Telecom rx FIFO intr enable */
#define CR0_ATE		(1 << 21) 	/* Audio tx FIFO intr enable */
#define CR0_ARE		(1 << 22) 	/* Audio rx FIFO intr enable */
#define CR0_LBM		(1 << 23)	/* Output of serial shifter connect
					   to Input of serial shifter internal */
#define CR0_ECP(x)	((x) << 24)	/* External clock prescaler */

/* MCP control register 0 */
#define CR1_CFS		(1 << 20)

/* MCP status register */
#define SR_ATS		(1 << 0)	/* Audio transmit FIFO req-flag */
#define SR_ARS		(1 << 1)	/* Audio receive FIFO req */
#define SR_TTS		(1 << 2)	/* Telecom transmit FIFO req-flag */
#define SR_TRS		(1 << 3)	/* Telecom receive FIFO req */ 
#define SR_ATU		(1 << 4)	/* Audio transmit FIFO underrun */
#define SR_ARO		(1 << 5) 	/* Audio receive FIFO overrun */
#define SR_TTU		(1 << 6)	/* Telecom transmit FIFO underrun */
#define SR_TRO		(1 << 7)	/* Telecom receive FIFO overrun */
#define SR_ANF		(1 << 8)	/* Audio transmit FIFO not full */
#define SR_ANE		(1 << 9)	/* Audio receive FIFO not empty */
#define SR_TNF		(1 << 10)	/* Telecom transmit FIFO not full */
#define SR_TNE		(1 << 11)	/* Telecom receive FIFO not empty */
#define SR_CWC		(1 << 12)	/* Codec write completed */
#define SR_CRC		(1 << 13)	/* Codec read completed */
#define SR_ACE		(1 << 14)	/* Audio codec enabled */
#define SR_TCE		(1 << 15)	/* Telecom codec enabled */

