/*	$NetBSD: serreg.h,v 1.1 1997/05/25 12:41:58 leo Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Leo Weppelman.
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

#define	SER_FREQ	153600	/* XXX: Assumes UCR_CLKDIV set!		*/
#define	SER_TOLERANCE	30	/* Baud rate tolerance, in 0.1% units	*/

/* Usart Control Register */
#define	UCR_PEVEN	0x02	/* Even parity				*/
#define	UCR_PENAB	0x04	/* Parity enable			*/
#define	UCR_SYNCH	0x00	/* Synchroneous mode			*/
#define	UCR_STOPB1	0x08	/* 1 stopbit				*/
#define	UCR_STOPB15	0x10	/* 1.5 stopbit				*/
#define	UCR_STOPB2	0x18	/* 2 stopbits				*/
#define	UCR_8BITS	0x00	/* 8 databits				*/
#define	UCR_7BITS	0x20	/* 7 databits				*/
#define	UCR_6BITS	0x40	/* 6 databits				*/
#define	UCR_5BITS	0x60	/* 5 databits				*/
#define	UCR_CLKDIV	0x80	/* Divide clock by 16			*/

/* Receiver Status Register */
#define	RSR_ENAB	0x01	/* Receiver enabled			*/
#define	RSR_SS		0x02	/* Synchroneous strip			*/
#define	RSR_CIP		0x04	/* Character in progress (async)	*/
#define	RSR_BREAK	0x08	/* Break (async)			*/
#define	RSR_MATCH	0x04	/* Character match (sync)		*/
#define	RSR_FS		0x08	/* Found/Search (sync)			*/
#define	RSR_FERR	0x10	/* Framing error			*/
#define RSR_PERR	0x20	/* Parity error				*/
#define	RSR_OERR	0x40	/* Overrun error			*/
#define	RSR_BFULL	0x80	/* Buffer Full				*/

/* Transmitter Status Register */
#define	TSR_ENAB	0x01	/* Transmitter Enable			*/
#define	TSR_SBREAK	0x08	/* Transmit Break			*/
#define	TSR_END		0x10	/* End of character			*/
#define	TSR_UE		0x40	/* Uart empty				*/
#define	TSR_BE		0x80	/* Buffer empty				*/

/*
 * These bits are a mixture of MFP.mf_gpip and PSG.ioa values.
 * Luckily, they were all distinct.
 */
#define MCR_RTS		0x08
#define MCR_CTS		0x04
#define MCR_DCD		0x02
#define MCR_DTR		0x10
#define MCR_RI		0x40
