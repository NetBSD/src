/* $NetBSD: scifreg.h,v 1.3 2002/04/28 17:10:36 uch Exp $ */

/*-
 * Copyright (C) 1999 SAITOH Masanobu.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH3_SCIFREG_H_
#define	_SH3_SCIFREG_H_

/*
 * Serial Communication Interface (SCIF)
 */

#if !defined(SH4)

/* SH3 definitions */

#define	SHREG_SCSMR2  (*(volatile unsigned char *)	0xa4000150)
#define	SHREG_SCBRR2  (*(volatile unsigned char *)	0xa4000152)
#define	SHREG_SCSCR2  (*(volatile unsigned char *)	0xa4000154)
#define	SHREG_SCFTDR2 (*(volatile unsigned char *)	0xa4000156)
#define	SHREG_SCSSR2  (*(volatile unsigned short *)	0xa4000158)
#define	SHREG_SCFRDR2 (*(volatile unsigned char *)	0xa400015A)
#define	SHREG_SCFCR2  (*(volatile unsigned char *)	0xa400015C)
#define	SHREG_SCFDR2  (*(volatile unsigned short *)	0xa400015E)

#define	SCSCR2_TIE	0x80	/* Transmit Interrupt Enable */
#define	SCSCR2_RIE	0x40	/* Recieve Interrupt Enable */
#define	SCSCR2_TE	0x20	/* Transmit Enable */
#define	SCSCR2_RE	0x10	/* Receive Enable */
#define	SCSCR2_CKE1	0x02	/* ClocK Enable 1 */
#define	SCSCR2_CKE0	0x01	/* ClocK Enable 0 */

#define	SCSSR2_ER	0x0080	/* ERror */
#define	SCSSR2_TEND	0x0040	/* Transmit END */
#define	SCSSR2_TDFE	0x0020	/* Transmit Data Fifo Empty */
#define	SCSSR2_BRK	0x0010	/* BReaK detection */
#define	SCSSR2_FER	0x0008	/* Framing ERror */
#define	SCSSR2_PER	0x0004	/* Parity ERror */
#define	SCSSR2_RDF	0x0002	/* Recieve fifo Data Full */
#define	SCSSR2_DR	0x0001	/* Data Ready */

#define	SCFCR2_RTRG1	0x80	/* Receive TRiGger 1 */
#define	SCFCR2_RTRG0	0x40	/* Receive TRiGger 0 */
#define	SCFCR2_TTRG1	0x20	/* Transmit TRiGger 1 */
#define	SCFCR2_TTRG0	0x10	/* Transmit TRiGger 0 */
#define	SCFCR2_MCE	0x08	/* Modem Control Enable */
#define	SCFCR2_TFRST	0x04	/* Transmit Fifo register ReSeT */
#define	SCFCR2_RFRST	0x02	/* Receive Fifo register ReSeT */
#define	SCFCR2_LOOP	0x01	/* LOOP back test */
#define	FIFO_RCV_TRIGGER_1	0x00
#define	FIFO_RCV_TRIGGER_4	0x40
#define	FIFO_RCV_TRIGGER_8	0x80
#define	FIFO_RCV_TRIGGER_14	0xc0
#define	FIFO_XMT_TRIGGER_8	0x00
#define	FIFO_XMT_TRIGGER_4	0x10
#define	FIFO_XMT_TRIGGER_2	0x20
#define	FIFO_XMT_TRIGGER_1	0x30

#else

/* SH4 definitions */

#define	SHREG_SCSMR2  (*(volatile unsigned short *)	0xffe80000)
#define	SHREG_SCBRR2  (*(volatile unsigned char *)	0xffe80004)
#define	SHREG_SCSCR2  (*(volatile unsigned short *)	0xffe80008)
#define	SHREG_SCFTDR2 (*(volatile unsigned char *)	0xffe8000c)
#define	SHREG_SCFSR2  (*(volatile unsigned short *)	0xffe80010)
#define	SHREG_SCFRDR2 (*(volatile unsigned char *)	0xffe80014)
#define	SHREG_SCFCR2  (*(volatile unsigned short *)	0xffe80018)
#define	SHREG_SCFDR2  (*(volatile unsigned short *)	0xffe8001c)
#define	SHREG_SCSPTR2 (*(volatile unsigned short *)	0xffe80020)
#define	SHREG_SCLSR2  (*(volatile unsigned short *)	0xffe80024)

/* alias */
#define	SHREG_SCSFDR2	SHREG_SCFTDR2
#define	SHREG_SCSSR2	SHREG_SCFSR2

#define	SCSCR2_TIE	0x0080	/* Transmit Interrupt Enable */
#define	SCSCR2_RIE	0x0040	/* Recieve Interrupt Enable */
#define	SCSCR2_TE	0x0020	/* Transmit Enable */
#define	SCSCR2_RE	0x0010	/* Receive Enable */
#define	SCSCR2_CKE1	0x0002	/* ClocK Enable 1 */

#define	SCSSR2_ER	0x0080	/* ERror */
#define	SCSSR2_TEND	0x0040	/* Transmit END */
#define	SCSSR2_TDFE	0x0020	/* Transmit Data Fifo Empty */
#define	SCSSR2_BRK	0x0010	/* BReaK detection */
#define	SCSSR2_FER	0x0008	/* Framing ERror */
#define	SCSSR2_PER	0x0004	/* Parity ERror */
#define	SCSSR2_RDF	0x0002	/* Recieve fifo Data Full */
#define	SCSSR2_DR	0x0001	/* Data Ready */

#define	SCFCR2_RTRG1	0x0080	/* Receive TRiGger 1 */
#define	SCFCR2_RTRG0	0x0040	/* Receive TRiGger 0 */
#define	SCFCR2_TTRG1	0x0020	/* Transmit TRiGger 1 */
#define	SCFCR2_TTRG0	0x0010	/* Transmit TRiGger 0 */
#define	SCFCR2_MCE	0x0008	/* Modem Control Enable */
#define	SCFCR2_TFRST	0x0004	/* Transmit Fifo register ReSeT */
#define	SCFCR2_RFRST	0x0002	/* Receive Fifo register ReSeT */
#define	SCFCR2_LOOP	0x0001	/* LOOP back test */
#define	FIFO_RCV_TRIGGER_1	0x0000
#define	FIFO_RCV_TRIGGER_4	0x0040
#define	FIFO_RCV_TRIGGER_8	0x0080
#define	FIFO_RCV_TRIGGER_14	0x00c0
#define	FIFO_XMT_TRIGGER_8	0x0000
#define	FIFO_XMT_TRIGGER_4	0x0010
#define	FIFO_XMT_TRIGGER_2	0x0020
#define	FIFO_XMT_TRIGGER_1	0x0030

#endif

/* common definitions */

#define	SCFDR2_TXCNT	0xff00	/* Tx CouNT */
#define	SCFDR2_RECVCNT	0x00ff	/* Rx CouNT */
#define	SCFDR2_TXF_FULL	0x1000	/* Tx FULL */
#define	SCFDR2_RXF_EPTY	0x0000	/* Rx EMPTY */

#endif /* !_SH3_SCIFREG_ */
