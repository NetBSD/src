/*	$NetBSD: pscreg.h,v 1.1 2026/06/27 13:28:35 rkujawa Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa and Robert Swindells.
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

#ifndef _POWERPC_MPC5200_PSCREG_H_
#define _POWERPC_MPC5200_PSCREG_H_

/*
 * Programmable Serial Controller (PSC) registers
 */

#define PSC_MR1		0x00	/* Mode Register 1 (8-bit, indexed) */
#define PSC_MR2		0x00	/* Mode Register 2 (8-bit, indexed) */

/*
 * Mode Register 1 (PSC_MR1) bits
 */
#define MR1_RRTS	(1 << 7)	/* receiver request-to-send control */
#define MR1_FFULL	(1 << 6)	/* RxRDY on FIFO full (vs not empty) */
#define PSC_SR		0x04	/* Status Register (ro, 16-bit) */
#define PSC_CSR		0x04	/* Clock Select Register (wo, 16-bit) */
#define PSC_CR		0x08	/* Command Register (wo, 8-bit) */
#define PSC_RB		0x0c	/* Rx Buffer (ro) */
#define PSC_TB		0x0c	/* Tx Buffer (wo) */
#define PSC_IPCR	0x10	/* Input Port Change Register (ro) */
#define PSC_ACR		0x10	/* Auxiliary Control Register (wo) */
#define PSC_ISR		0x14	/* Interrupt Status Register (ro, 16-bit) */
#define PSC_IMR		0x14	/* Interrupt Mask Register (wo, 16-bit) */
#define PSC_CTUR	0x18	/* Counter Timer Upper Register */
#define PSC_CTLR	0x1c	/* Counter Timer Lower Register */
#define PSC_SICR	0x40	/* Serial Interface Control Register */
#define PSC_TFNUM	0x5c	/* Tx FIFO byte count */
#define PSC_RFNUM	0x58	/* Rx FIFO byte count */
#define PSC_TFSTAT	0x84	/* Tx FIFO Status */
#define PSC_RFSTAT	0x64	/* Rx FIFO Status */

#define PSC_NPORTS	0x100	/* size of one PSC register window */

/*
 * Clock Select Register (PSC_CSR)
 */
#define CSR_UART_CT	0xdd00	/* Rx/Tx clock = counter/timer (/32 prescale) */
#define PSC_BAUD_PRESCALE	32

/* Status Register (PSC_SR) bits. */
#define SR_RB		__BIT(15)	/* received break */
#define SR_FE		__BIT(14)	/* framing error */
#define SR_PE		__BIT(13)	/* parity error */
#define SR_ORERR	__BIT(12)	/* overrun error */
#define SR_TXEMP	__BIT(11)	/* transmitter empty */
#define SR_TXRDY	__BIT(10)	/* transmitter ready */
#define SR_FFULL	__BIT(9)	/* receive FIFO full */
#define SR_RXRDY	__BIT(8)	/* receiver ready */

#define SR_RCV_MASK	(SR_RB | SR_FE | SR_PE | SR_ORERR | SR_RXRDY)

/* Command Register (PSC_CR) bits. */
#define CMD_RESET_MR	(1 << 4)
#define CMD_RESET_RX	(2 << 4)
#define CMD_RESET_TX	(3 << 4)
#define CMD_RESET_ERR	(4 << 4)
#define CMD_RESET_BRK	(5 << 4)
#define CMD_START_BRK	(6 << 4)
#define CMD_STOP_BRK	(7 << 4)
#define CMD_TX_ENABLE	(1 << 2)
#define CMD_TX_DISABLE	(2 << 2)
#define CMD_RX_ENABLE	(1 << 0)
#define CMD_RX_DISABLE	(2 << 0)

/* Interrupt Status/Mask Register bits. */
#define INT_IPC		__BIT(15)
#define INT_ORERR	__BIT(12)
#define INT_TXEMP	__BIT(11)
#define INT_DB		__BIT(10)
#define INT_RXRDY	__BIT(9)
#define INT_TXRDY	__BIT(8)
#define INT_ERROR	__BIT(6)

#endif /* _POWERPC_MPC5200_PSCREG_H_ */
