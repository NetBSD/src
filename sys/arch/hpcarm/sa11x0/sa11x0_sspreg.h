/*      $NetBSD: sa11x0_sspreg.h,v 1.1.2.2 2001/04/21 17:53:36 bouyer Exp $	*/

/*-
 * Copyright (c) 2001, The NetBSD Foundation, Inc.  All rights reserved.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/* SA11[01]0 integrated SSP (synchronous serial port) interface */

#define SASSP_FREQ	(3686400 / 2)
#define SASSPSPEED(b)	(SACOM_FREQ / (b) - 1)

/* size of I/O space */
#define SASSP_NPORTS	30

#define SASSP_TXFIFOLEN		8
#define SASSP_RXFIFOLEN		12

/* SSP control register 0 */
#define SASSP_CR0	0x60
#define CR0_DSS_MASK	0x000F	/* Data size select */
#define CR0_FRF_MASK	0x0030	/* Frame format */
#define CR0_SSE		0x0080	/* SSP enable */
#define CR0_SCR_MASK	0xFF00	/* Serial clock rate */

/* SSP control register 1 */
#define SASSP_CR1	0x64
#define CR1_RIE		0x01	/* Receive FIFO interrupt enable */
#define CR1_TIE		0x02	/* Transmit FIFO interrupt enable */
#define CR1_LBM		0x04	/* Loopback mode */
#define CR1_SPO		0x08	/* Serial clock polarity */
#define CR1_SPH		0x10	/* Serial clock phase */
#define CR1_ECS		0x20	/* External clock select */

/* SSP data register */
#define SASSP_DR	0x6C

/* SSP status register */
#define SASSP_SR	0x74
#define SR_TNF		0x02	/* Transmit FIFO not full */
#define SR_RNE		0x04	/* Receive FIFO not empty */
#define SR_BSY		0x08	/* SSP busy flag */
#define SR_TFS		0x10	/* Transmit FIFO service request */
#define SR_RFS		0x20	/* Receive FIFO service request */
#define SR_ROR		0x40	/* Receive FIFO overrrun */
