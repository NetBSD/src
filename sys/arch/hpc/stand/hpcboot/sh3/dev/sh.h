/* -*-C++-*-	$NetBSD: sh.h,v 1.1.2.2 2002/02/28 04:09:48 nathanw Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
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

#ifndef _HPCBOOT_SH_DEV_SH_H_
#define _HPCBOOT_SH_DEV_SH_H_

/*
 * SH3, SH4 embeded devices.
 */

/*
 * INTC
 */
/* SH7709/7709A */
/* R/W 16bit */
#define SH3_ICR0		0xfffffee0
#define SH3_ICR1		0xa4000010
#define SH3_ICR2		0xa4000012
#define SH3_PINTER		0xa4000014
#define SH3_IPRA		0xfffffee2
#define SH3_IPRB		0xfffffee4
#define SH3_IPRC		0xa4000016
#define SH3_IPRD		0xa4000018
#define SH3_IPRE		0xa400001a
/* R/W 8bit */			
#define SH3_IRR0		0xa4000004
/* R 8bit */			
#define SH3_IRR1		0xa4000006
#define SH3_IRR2		0xa4000008
				
#define SH3_ICR0_NMIL		  0x8000
#define SH3_ICR0_NMIE		  0x0100
				
#define SH3_ICR1_MAI		  0x8000
#define SH3_ICR1_IRQLVL		  0x4000
#define SH3_ICR1_BLMSK		  0x2000
#define SH3_ICR1_IRLSEN		  0x1000
#define SH3_ICR1_IRQ51S		  0x0800
#define SH3_ICR1_IRQ50S		  0x0400
#define SH3_ICR1_IRQ41S		  0x0200
#define SH3_ICR1_IRQ40S		  0x0100
#define SH3_ICR1_IRQ31S		  0x0080
#define SH3_ICR1_IRQ30S		  0x0040
#define SH3_ICR1_IRQ21S		  0x0020
#define SH3_ICR1_IRQ20S		  0x0010
#define SH3_ICR1_IRQ11S		  0x0008
#define SH3_ICR1_IRQ10S		  0x0004
#define SH3_ICR1_IRQ01S		  0x0002
#define SH3_ICR1_IRQ00S		  0x0001

#define SH3_SENSE_SELECT_MASK		  0x3
#define SH3_SENSE_SELECT_FALLING_EDGE	  0x0
#define SH3_SENSE_SELECT_RAISING_EDGE	  0x1
#define SH3_SENSE_SELECT_LOW_LEVEL	  0x2
#define SH3_SENSE_SELECT_RESERVED	  0x3

#define SH3_ICR2_PINT15S	  0x8000
#define SH3_ICR2_PINT14S	  0x4000
#define SH3_ICR2_PINT13S	  0x2000
#define SH3_ICR2_PINT12S	  0x1000
#define SH3_ICR2_PINT11S	  0x0800
#define SH3_ICR2_PINT10S	  0x0400
#define SH3_ICR2_PINT9S		  0x0200
#define SH3_ICR2_PINT8S		  0x0100
#define SH3_ICR2_PINT7S		  0x0080
#define SH3_ICR2_PINT6S		  0x0040
#define SH3_ICR2_PINT5S		  0x0020
#define SH3_ICR2_PINT4S		  0x0010
#define SH3_ICR2_PINT3S		  0x0008
#define SH3_ICR2_PINT2S		  0x0004
#define SH3_ICR2_PINT1S		  0x0002
#define SH3_ICR2_PINT0S		  0x0001

#define SH_IPR_MASK		0xf

/* SH7750 */
#define SH4_ICR			0xffd00000
#define   SH4_ICR_NMIL		  0x8000
#define   SH4_ICR_MAI		  0x4000
#define   SH4_ICR_NMIB		  0x0200
#define   SH4_ICR_NMIE		  0x0100
#define   SH4_ICR_IRLM		  0x0080
#define SH4_IPRA		0xffd00004
#define SH4_IPRB		0xffd00008
#define SH4_IPRC		0xffd0000c
/* SH7750S */
#define SH4_IPRD		0xffd00010

/*
 * Bus State Controller
 */
#define SH3_BCR1	        0xffffff60
#define SH3_BCR2	        0xffffff62
#define SH3_WCR1	        0xffffff64
#define SH3_WCR2	        0xffffff66
#define SH3_MCR		        0xffffff68
#define SH3_DCR		        0xffffff6a
#define SH3_PCR		        0xffffff6c
#define SH3_RTCSR	        0xffffff6e
#define SH3_RTCNT	        0xffffff70
#define SH3_RTCOR	        0xffffff72
#define SH3_RFCR	        0xffffff74
#define SH3_BCR3	        0xffffff7e

/*
 * Pin Function Controller
 */				        
#define SH3_PACR		0xa4000100
#define SH3_PBCR		0xa4000102
#define SH3_PCCR		0xa4000104
#define SH3_PDCR		0xa4000106
#define SH3_PECR		0xa4000108
#define SH3_PFCR		0xa400010a
#define SH3_PGCR		0xa400010c
#define SH3_PHCR		0xa400010e
#define SH3_PJCR		0xa4000110
#define SH3_PKCR		0xa4000112
#define SH3_PLCR		0xa4000114
#define SH3_SCPCR		0xa4000116

/*
 * I/O port
 */
#define SH3_PADR		0xa4000120
#define SH3_PBDR		0xa4000122
#define SH3_PCDR		0xa4000124
#define SH3_PDDR		0xa4000126
#define SH3_PEDR		0xa4000128
#define SH3_PFDR		0xa400012a
#define SH3_PGDR		0xa400012c
#define SH3_PHDR		0xa400012e
#define SH3_PJDR		0xa4000130
#define SH3_PKDR		0xa4000132
#define SH3_PLDR		0xa4000134
#define SH3_SCPDR		0xa4000136

/*
 * TMU
 */
#define SH3_TOCR		0xfffffe90
#define   SH3_TOCR_TCOE		  0x01
#define SH3_TSTR		0xfffffe92
#define   SH3_TSTR_STR2		  0x04
#define   SH3_TSTR_STR1		  0x02
#define   SH3_TSTR_STR0		  0x01
#define SH3_TCOR0		0xfffffe94
#define SH3_TCNT0		0xfffffe98
#define SH3_TCR0		0xfffffe9c
#define SH3_TCOR1		0xfffffea0
#define SH3_TCNT1		0xfffffea4
#define SH3_TCR1		0xfffffea8
#define SH3_TCOR2		0xfffffeac
#define SH3_TCNT2		0xfffffeb0
#define SH3_TCR2		0xfffffeb4
#define SH3_TCPR2		0xfffffeb8
#define   SH3_TCR_ICPF		  0x0200
#define   SH3_TCR_UNF		  0x0100
#define   SH3_TCR_ICPE1		  0x0080
#define   SH3_TCR_ICPE0		  0x0040
#define   SH3_TCR_UNIE		  0x0020
#define   SH3_TCR_CKEG1		  0x0010
#define   SH3_TCR_CKEG0		  0x0008
#define   SH3_TCR_TPSC2		  0x0004
#define   SH3_TCR_TPSC1		  0x0002
#define   SH3_TCR_TPSC0		  0x0001

#define   SH3_TCR_TPSC_P4	  0x0000
#define   SH3_TCR_TPSC_P16	  0x0001
#define   SH3_TCR_TPSC_P64	  0x0002
#define   SH3_TCR_TPSC_P256	  0x0003

/*
 * SCI
 */
#define SH4_SCSMR		0xffe00000
#define SH4_SCBRR		0xffe00004
#define SH4_SCSCR		0xffe00008
#define SH4_SCTDR		0xffe0000c
#define SH4_SCSSR		0xffe00010
#define SH4_SCRDR		0xffe00014

#define SH3_SCRSR		/* can't access from CPU */
#define SH3_SCTSR		/* can't access from CPU */
#define SH3_SCSMR		0xfffffe80
#define SH3_SCBRR		0xfffffe82
#define SH3_SCSCR		0xfffffe84
#define SH3_SCTDR		0xfffffe86
#define SH3_SCSSR		0xfffffe88
#define SH3_SCRDR		0xfffffe8a
#define SH3_SCPCR		0xa4000116
#define SH3_SCPDR		0xa4000136

#define SCSSR_TDRE		  0x80

#define SH3_SCI_TX_BUSY()						\
	while ((_reg_read_1(SH3_SCSSR) & SCSSR_TDRE) == 0)

#define SH3_SCI_PUTC(c)							\
__BEGIN_MACRO								\
	SH3_SCI_TX_BUSY();						\
	_reg_write_1(SH3_SCTDR, c);					\
	_reg_write_1(SH3_SCSSR,						\
	    _reg_read_1(SH3_SCSSR) & ~SCSSR_TDRE);			\
__END_MACRO

#define SH3_SCI_PRINT(s)							\
__BEGIN_MACRO								\
	char *__s =(char *)(s);						\
	int __i;							\
	for (__i = 0; __s[__i] != '\0'; __i++) {			\
		char __c = __s[__i];					\
		if (__c == '\n')					\
			SH3_SCI_PUTC('\r');				\
		SH3_SCI_PUTC(__c);					\
	}								\
__END_MACRO

/* 
 * SCIF
 */
#define SH4_SCSMR2		0xffe80000
#define SH4_SCBRR2		0xffe80004
#define SH4_SCSCR2		0xffe80008
#define SH4_SCFTDR2		0xffe8000c
#define SH4_SCFSR2		0xffe80010
#define SH4_SCFRDR2		0xffe80014
#define SH4_SCFCR2		0xffe80018
#define SH4_SCFDR2		0xffe8001c
#define SH4_SCSPTR2		0xffe80020
#define SH4_SCLSR2		0xffe80024
#define SH4_SCSMR2		0xffe80000
#define SH4_SCBRR2		0xffe80004
#define SH4_SCSCR2		0xffe80008
#define SH4_SCFTDR2		0xffe8000c
#define SH4_SCFSR2		0xffe80010
#define SH4_SCFRDR2		0xffe80014
#define SH4_SCFCR2		0xffe80018
#define SH4_SCFDR2		0xffe8001c
#define SH4_SCSPTR2		0xffe80020
#define SH4_SCLSR2		0xffe80024

#define	SH4_SCSSR2		SH4_SCFSR2
#define	SH4_SCSSR2		SH4_SCFSR2

#define SH3_SCSMR2		0xa4000150	/* R/W */
#define SH3_SCBRR2		0xa4000152	/* R/W */
#define SH3_SCSCR2		0xa4000154	/* R/W */
#define SH3_SCFTDR2		0xa4000156	/* W */
#define SH3_SCSSR2		0xa4000158	/* R/W(0 write only) */
#define SH3_SCFRDR2		0xa400015a	/* R */
#define SH3_SCFCR2		0xa400015c	/* R/W */
#define SH3_SCFDR2		0xa400015e	/* R */
#define SH3_SCSMR2		0xa4000150	/* R/W */
#define SH3_SCBRR2		0xa4000152	/* R/W */
#define SH3_SCSCR2		0xa4000154	/* R/W */
#define SH3_SCFTDR2		0xa4000156	/* W */
#define SH3_SCSSR2		0xa4000158	/* R/W(0 write only) */
#define SH3_SCFRDR2		0xa400015a	/* R */
#define SH3_SCFCR2		0xa400015c	/* R/W */
#define SH3_SCFDR2		0xa400015e	/* R */

#define SCSCR2_TIE	          0x0080	/* Transmit Interrupt Enable */
#define SCSCR2_RIE	          0x0040	/* Receive Interrupt Enable */
#define SCSCR2_TE	          0x0020	/* Transmit Enable */
#define SCSCR2_RE	          0x0010	/* Receive Enable */
#define SCSCR2_CKE1	          0x0002	/* ClocK Enable 1 */
#define SCSCR2_CKE0	          0x0001	/* ClocK Enable 0 */
#define SCSCR2_CKE	          0x0003	/* ClocK Enable mask */

#define SCSSR2_ER	          0x0080	/* ERror */
#define SCSSR2_TEND	          0x0040	/* Transmit END */
#define SCSSR2_TDFE	          0x0020	/* Transmit Data Fifo Empty */
#define SCSSR2_BRK	          0x0010	/* BReaK detection */
#define SCSSR2_FER	          0x0008	/* Framing ERror */
#define SCSSR2_PER	          0x0004	/* Parity ERror */
#define SCSSR2_RDF	          0x0002	/* Receive fifo Data Full */
#define SCSSR2_DR	          0x0001	/* Data Ready */

#define SCFCR2_RTRG1		  0x0080	/* Receive TRiGger 1 */
#define SCFCR2_RTRG0		  0x0040	/* Receive TRiGger 0 */
#define SCFCR2_TTRG1		  0x0020	/* Transmit TRiGger 1 */
#define SCFCR2_TTRG0		  0x0010	/* Transmit TRiGger 0 */
#define SCFCR2_MCE		  0x0008	/* Modem Control Enable */
#define SCFCR2_TFRST		  0x0004	/* Transmit Fifo register ReSeT */
#define SCFCR2_RFRST		  0x0002	/* Receive Fifo register ReSeT */
#define SCFCR2_LOOP		  0x0001	/* LOOP back test */
#define FIFO_RCV_TRIGGER_1	  0x0000
#define FIFO_RCV_TRIGGER_4	  0x0040
#define FIFO_RCV_TRIGGER_8	  0x0080
#define FIFO_RCV_TRIGGER_14	  0x00c0
#define FIFO_XMT_TRIGGER_8	  0x0000
#define FIFO_XMT_TRIGGER_4	  0x0010
#define FIFO_XMT_TRIGGER_2	  0x0020
#define FIFO_XMT_TRIGGER_1	  0x0030

#define SCFDR2_TXCNT		  0xff00	/* Tx CouNT */
#define SCFDR2_RECVCNT		  0x00ff	/* Rx CouNT */
#define SCFDR2_TXF_FULL		  0x1000	/* Tx FULL */
#define SCFDR2_RXF_EPTY		  0x0000	/* Rx EMPTY */

#define SCSMR2_CHR		  0x40		/* Character length */
#define SCSMR2_PE		  0x20		/* Parity enable */
#define SCSMR2_OE		  0x10		/* Parity mode */
#define SCSMR2_STOP		  0x08		/* Stop bit length */
#define SCSMR2_CKS		  0x03		/* Clock select */

/* simple serial console macros. */
#define SH3_SCIF_TX_BUSY()						\
	while ((_reg_read_2(SH3_SCSSR2) & SCSSR2_TDFE) == 0)

#define SH3_SCIF_PUTC(c)						\
__BEGIN_MACRO								\
	SH3_SCIF_TX_BUSY();						\
	/*  wait until previous transmit done. */			\
	_reg_write_1(SH3_SCFTDR2, c);					\
	/* Clear transmit FIFO empty flag */				\
	_reg_write_1(SH3_SCSSR2,					\
	    _reg_read_1(SH3_SCSSR2) & ~(SCSSR2_TDFE | SCSSR2_TEND));	\
__END_MACRO

#define SH3_SCIF_PRINT(s)						\
__BEGIN_MACRO								\
	char *__s =(char *)(s);						\
	int __i;							\
	for (__i = 0; __s[__i] != '\0'; __i++) {			\
		char __c = __s[__i];					\
		if (__c == '\n')					\
			SH3_SCIF_PUTC('\r');				\
		SH3_SCIF_PUTC(__c);					\
	}								\
__END_MACRO

#define SH3_SCIF_PRINT_HEX(h)						\
__BEGIN_MACRO								\
	u_int32_t __h =(u_int32_t)(h);					\
	int __i;							\
	SH3_SCIF_PUTC('0'); SH3_SCIF_PUTC('x');				\
	for (__i = 0; __i < 8; __i++, __h <<= 4) {			\
		int __n =(__h >> 28) & 0xf;				\
		char __c = __n > 9 ? 'A' + __n - 10 : '0' + __n;	\
		SH3_SCIF_PUTC(__c);					\
	}								\
	SH3_SCIF_PUTC('\r'); SH3_SCIF_PUTC('\n');			\
__END_MACRO

#endif /* _HPCBOOT_SH_DEV_SH_H_ */
