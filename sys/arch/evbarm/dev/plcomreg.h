/*	$NetBSD: plcomreg.h,v 1.1.160.1 2012/04/17 00:06:14 yamt Exp $	*/

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
#define	CR_LBE		0x80	/* Loopback enable */
#define	CR_RTIE		0x40	/* Receive timeout interrupt enable */
#define	CR_TIE		0x20	/* Transmit interrupt enable */
#define	CR_RIE		0x10	/* Receive interrrupt enable */
#define	CR_MSIE		0x08	/* Modem status interrupt enable */
#define	CR_SIRLP	0x04	/* IrDA SIR Low power mode */
#define	CR_SIREN	0x02	/* SIR Enable */
#define	CR_UARTEN	0x01	/* Uart enable */

/* interrupt identification register */
#define	IIR_IMASK	0x0f
#define	IIR_RTIS	0x08
#define	IIR_TIS		0x04
#define	IIR_RIS		0x02
#define	IIR_MIS		0x01

/* line control register */
#define	LCR_WLEN	0x60	/* Mask of size bits */
#define	LCR_8BITS	0x60	/* 8 bits per serial word */
#define	LCR_7BITS	0x40	/* 7 bits */
#define	LCR_6BITS	0x20	/* 6 bits */
#define	LCR_5BITS	0x00	/* 5 bits */
#define	LCR_FEN		0x10	/* FIFO enable */
#define	LCR_STP2	0x08	/* 2 stop bits per serial word */
#define	LCR_EPS		0x04	/* Even parity select */
#define	LCR_PEN		0x02	/* Parity enable */
#define	LCR_PEVEN	(LCR_PEN | LCR_EPS)
#define	LCR_PODD	LCR_PEN
#define	LCR_PNONE	0x00	/* No parity */
#define	LCR_BRK		0x01	/* Break Control */

/* modem control register */
#define	MCR_RTS		0x02	/* Request To Send */
#define	MCR_DTR		0x01	/* Data Terminal Ready */

/* receive status register */

#define	RSR_OE		0x08	/* Overrun Error */
#define	RSR_BE		0x04	/* Break */
#define	RSR_PE		0x02	/* Parity Error */
#define	RSR_FE		0x01	/* Framing Error */

/* flag register */
#define	FR_TXFE		0x80	/* Transmit fifo empty */
#define	FR_RXFF		0x40	/* Recive fifo full */
#define	FR_TXFF		0x20	/* Transmit fifo full */
#define	FR_RXFE		0x10	/* Receive fifo empty */
#define	FR_BUSY		0x08	/* Uart Busy */
#define	FR_DCD		0x04	/* Data carrier detect */
#define	FR_DSR		0x02	/* Data set ready */
#define	FR_CTS		0x01	/* Clear to send */

/* modem status register */
/* All deltas are from the last read of the MSR. */
#define	MSR_DCD		FR_DCD
#define	MSR_DSR		FR_DSR
#define	MSR_CTS		FR_CTS

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
