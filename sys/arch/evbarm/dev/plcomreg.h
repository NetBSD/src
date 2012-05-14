/*	$NetBSD: plcomreg.h,v 1.3 2012/05/14 19:40:06 skrll Exp $	*/

/*-
 * Copyright (c) 2001 ARM Ltd
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
*/


#define	PLCOM_FREQ	1843200	/* 16-bit baud rate divisor */
#define	PLCOM_TOLERANCE	30	/* baud rate tolerance, in 0.1% units */

/* control register */
#define	PL011_CR_CTSEN	0x8000	/* CTS HW flow control enable */
#define	PL011_CR_RTSEN	0x4000	/* RTS HW flow control enable */
#define	PL011_CR_OUT2	0x2000	/* Complement of UART Out2 MSR */
#define	PL011_CR_OUT1	0x1000	/* Complement of UART Out1 MSR */
#define	PL011_CR_RTS	0x0800	/* Request to send */
#define	PL011_CR_DTR	0x0400	/* Data transmit Ready */
#define	PL011_CR_RXE	0x0200	/* Receive enable */
#define	PL011_CR_TXE	0x0100	/* Transmit enable */
#define	PL01X_CR_LBE	0x0080	/* Loopback enable */
#define	PL010_CR_RTIE	0x0040	/* Receive timeout interrupt enable */
#define	PL010_CR_TIE	0x0020	/* Transmit interrupt enable */
#define	PL010_CR_RIE	0x0010	/* Receive interrrupt enable */
#define	PL010_CR_MSIE	0x0008	/* Modem status interrupt enable */
#define	PL01X_CR_SIRLP	0x0004	/* IrDA SIR Low power mode */
#define	PL01X_CR_SIREN	0x0002	/* SIR Enable */
#define	PL01X_CR_UARTEN	0x0001	/* Uart enable */

/* interrupt identification register */
#define	PL010_IIR_IMASK	0x0f
#define	PL010_IIR_RTIS	0x08
#define	PL010_IIR_TIS	0x04
#define	PL010_IIR_RIS	0x02
#define	PL010_IIR_MIS	0x01

/* line control register */
#define	PL011_LCR_SPS	0x80	/* Stick parity select */
#define	PL01X_LCR_WLEN	0x60	/* Mask of size bits */
#define	PL01X_LCR_8BITS	0x60	/* 8 bits per serial word */
#define	PL01X_LCR_7BITS	0x40	/* 7 bits */
#define	PL01X_LCR_6BITS	0x20	/* 6 bits */
#define	PL01X_LCR_5BITS	0x00	/* 5 bits */
#define	PL01X_LCR_FEN	0x10	/* FIFO enable */
#define	PL01X_LCR_STP2	0x08	/* 2 stop bits per serial word */
#define	PL01X_LCR_EPS	0x04	/* Even parity select */
#define	PL01X_LCR_PEN	0x02	/* Parity enable */
#define	PL01X_LCR_PEVEN	(PL01X_LCR_PEN | PL01X_LCR_EPS)
#define	PL01X_LCR_PODD	PL01X_LCR_PEN
#define	PL01X_LCR_PNONE	0x00	/* No parity */
#define	PL01X_LCR_BRK	0x01	/* Break Control */

/* modem control register */
#define	PL01X_MCR_RTS		0x02	/* Request To Send */
#define	PL01X_MCR_DTR		0x01	/* Data Terminal Ready */

/* receive status register */
#define	PL01X_RSR_OE	0x08	/* Overrun Error */
#define	PL01X_RSR_BE	0x04	/* Break */
#define	PL01X_RSR_PE	0x02	/* Parity Error */
#define	PL01X_RSR_FE	0x01	/* Framing Error */
#define	PL01X_RSR_ERROR	(PL01X_RSR_OE | PL01X_RSR_BE | PL01X_RSR_PE | PL01X_RSR_FE)

/* flag register */
#define	PL01X_FR_RI	0x100	/* Ring Indicator */
#define	PL01X_FR_TXFE	0x080	/* Transmit fifo empty */
#define	PL01X_FR_RXFF	0x040	/* Recive fifo full */
#define	PL01X_FR_TXFF	0x020	/* Transmit fifo full */
#define	PL01X_FR_RXFE	0x010	/* Receive fifo empty */
#define	PL01X_FR_BUSY	0x008	/* Uart Busy */
#define	PL01X_FR_DCD	0x004	/* Data carrier detect */
#define	PL01X_FR_DSR	0x002	/* Data set ready */
#define	PL01X_FR_CTS	0x001	/* Clear to send */

/* modem status register */
/* All deltas are from the last read of the MSR. */
#define	PL01X_MSR_DCD		PL01X_FR_DCD
#define	PL01X_MSR_DSR		PL01X_FR_DSR
#define	PL01X_MSR_CTS		PL01X_FR_CTS

/* All interrupt status/clear registers */
#define	PL011_INT_OE	0x400
#define	PL011_INT_BE	0x200
#define	PL011_INT_PE	0x100
#define	PL011_INT_FE	0x080
#define	PL011_INT_RT	0x040
#define	PL011_INT_TX	0x020
#define	PL011_INT_RX	0x010
#define	PL011_INT_DSR	0x008
#define	PL011_INT_DCD	0x004
#define	PL011_INT_CTS	0x002
#define	PL011_INT_RIR	0x001

/* DMA control registers */
#define	PL011_DMA_ONERR	0x4
#define	PL011_DMA_TXE	0x2
#define	PL011_DMA_RXE	0x1

/* Register offsets */
#define	plcom_dr	0x00
#define	plcom_rsr	0x04
#define	plcom_ecr	0x04
#define	plcom_lcr	0x08
#define	plcom_dlbh	0x0c
#define	plcom_dlbl	0x10
#define	plcom_cr	0x14
#define	plcom_fr	0x18
#define	plcom_iir	0x1c
#define	plcom_icr	0x1c
#define	plcom_ilpr	0x20

/* IFPGA specific */
#define	PLCOM_UART_SIZE	0x24
