/*	$NetBSD: at91usartreg.h,v 1.4 2022/02/13 00:39:45 andvar Exp $	*/

/*
 * Copyright (c) 2007 Embedtronics Oy. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _AT91USARTREG_H_
#define _AT91USARTREG_H_

#define	US_CR		0x00U	/* 0x00: Control Register	*/
#define	US_MR		0x04U	/* 0x04: Mode Register		*/
#define	US_IER		0x08U	/* 0x08: Interrupt Enable Reg	*/
#define	US_IDR		0x0CU	/* 0x0C: Interrupt Disable Reg	*/
#define	US_IMR		0x10U	/* 0x10: Interrupt Mask Reg	*/
#define	US_CSR		0x14U	/* 0x14: Channel Status Reg	*/
#define	US_RHR		0x18U	/* 0x18: Receive Holding Reg	*/
#define	US_THR		0x1CU	/* 0x1C: Transmit Holding Reg	*/
#define	US_BRGR		0x20U	/* 0x20: Baud Rate Generator Rg	*/
#define	US_RTOR		0x24U	/* 0x24: Receiver Time-out Reg	*/
#define	US_TTGR		0x28U	/* 0x28: Transmitter Timeguard Reg */
#define	US_FIDI		0x40U	/* 0x40: FI DI Ratio Register	*/
#define	US_NER		0x44U	/* 0x44: Number of Errors Reg	*/
#define	US_IF		0x4CU	/* 0x4C: IrDA Filter Register	*/
#define	US_PDC		0x100U	/* 0x100: PDC			*/

/* Control Register bits: */
#define	US_CR_RTSDIS	0x00080000U	/* 1 = disable RTS		*/
#define	US_CR_RTSEN	0x00040000U	/* 1 = enable RTS		*/
#define	US_CR_DTRDIS	0x00020000U	/* 1 = disable DTR		*/
#define	US_CR_DTREN	0x00010000U	/* 1 = enable DTR		*/
#define	US_CR_RETTO	0x00008000U	/* 1 = Rearm Time-out		*/
#define	US_CR_RSTNACK	0x00004000U	/* 1 = Reset Non Acknowledge	*/
#define	US_CR_RSTIT	0x00002000U	/* 1 = Reset Iterations		*/
#define	US_CR_SENDA	0x00001000U	/* 1 = Send Address		*/
#define	US_CR_STTTO	0x00000800U	/* 1 = Start Time-out		*/
#define	US_CR_STPBRK	0x00000400U	/* 1 = stop break		*/
#define	US_CR_STTBRK	0x00000200U	/* 1 = start break		*/
#define	US_CR_RSTSTA	0x00000100U	/* 1 = reset status bits	*/
#define	US_CR_TXDIS	0x00000080U	/* 1 = disable transmitter	*/
#define	US_CR_TXEN	0x00000040U	/* 1 = enable transmitter	*/
#define	US_CR_RXDIS	0x00000020U	/* 1 = disable receiver		*/
#define	US_CR_RXEN	0x00000010U	/* 1 = enable receiver		*/
#define	US_CR_RSTTX	0x00000008U	/* 1 = reset transmitter	*/
#define	US_CR_RSTRX	0x00000004U	/* 1 = reset receiver		*/

/* Mode Register: */
#define	US_MR_FILTER	0x10000000U	/* 1 = Infrared Receive filter	*/
#define	US_MR_MAX_ITER 0x07000000U	/* maximum number of iterations in ISO7816 */
#define	US_MR_MAX_ITER_SHIFT 24
#define	US_MR_DSNACK	0x00200000U	/* 1 = disable successive NACK	*/
#define	US_MR_INACK	0x00100000U	/* 1 = the NACK is not generated*/
#define	US_MR_OVER	0x00080000U	/* 1 = 8x oversampling (0 = 16x	*/
#define	US_MR_CLKO	0x00040000U	/* 1 = drive SCK		*/
#define	US_MR_MODE9	0x00020000U	/* 9-bit character length	*/
#define	US_MR_MSBF	0x00010000U	/* 1 = send MSB first		*/

#define	US_MR_CHMODE	0x0000C000U	/* channel mode			*/
#define	US_MR_CHMODE_SHIFT	14U
#define	US_MR_CHMODE_NORMAL	0x00000000U
#define	US_MR_CHMODE_ECHO	0x00004000U
#define	US_MR_CHMODE_LOCAL_LOOP	0x00008000U
#define	US_MR_CHMODE_REMOTE_LOOP 0x0000C000U

#define	US_MR_NBSTOP		0x00003000U /* number of stop bits	*/
#define	US_MR_NBSTOP_SHIFT	12U
#define	US_MR_NBSTOP_1		0x00000000U
#define	US_MR_NBSTOP_1_5	0x00001000U
#define	US_MR_NBSTOP_2		0x00002000U

#define	US_MR_PAR		0x00000E00U /* parity type		*/
#define	US_MR_PAR_SHIFT		9U
#define	US_MR_PAR_EVEN		0x00000000U
#define	US_MR_PAR_ODD		0x00000200U
#define	US_MR_PAR_SPACE		0x00000400U
#define	US_MR_PAR_MARK		0x00000600U
#define	US_MR_PAR_NONE		0x00000800U
#define	US_MR_PAR_MULTI_DROP	0x00000C00U

#define	US_MR_SYNC		0x00000100U /* 1 = synchronous mode	*/

#define	US_MR_CHRL	   	0x000000C0U /* character length		*/
#define	US_MR_CHRL_SHIFT   	6U
#define	US_MR_CHRL_5		0x00000000U
#define	US_MR_CHRL_6		0x00000040U
#define	US_MR_CHRL_7		0x00000080U
#define	US_MR_CHRL_8		0x000000C0U

#define	US_MR_USCLKS 	0x00000030U /* clock selection		*/
#define	US_MR_USCLKS_SHIFT	4U
#define	US_MR_USCLKS_MCK	0x00000000U
#define	US_MR_USCLKS_MCK_DIV	0x00000010U
#define	US_MR_USCLKS_SCK	0x00000030U

#define	US_MR_MODE		0x0000000FU
#define	US_MR_MODE_SHIFT	0U
#define	US_MR_MODE_NORMAL	0x00000000U
#define	US_MR_MODE_RS485	0x00000001U
#define	US_MR_MODE_AUTO_RTSCTS	0x00000002U
#define	US_MR_MODE_MODEM	0x00000003U
#define	US_MR_MODE_ISO7816_T0	0x00000004U
#define	US_MR_MODE_ISO7816_T1	0x00000006U
#define	US_MR_MODE_IRDA		0x00000008U


/* Interrupt bits: */
#define	US_CSR_CTSIC		0x00080000U
#define	US_CSR_DCDIC		0x00040000U
#define	US_CSR_DSRIC		0x00020000U
#define	US_CSR_RIIC		0x00010000U
#define	US_CSR_NACK		0x00002000U
#define	US_CSR_RXBUFF		0x00001000U
#define	US_CSR_TXBUFE		0x00000800U
#define	US_CSR_ITERATION	0x00000400U
#define	US_CSR_TXEMPTY		0x00000200U
#define	US_CSR_TIMEOUT		0x00000100U
#define	US_CSR_PARE		0x00000080U
#define	US_CSR_FRAME		0x00000040U
#define	US_CSR_OVRE		0x00000020U
#define	US_CSR_ENDTX		0x00000010U
#define	US_CSR_ENDRX		0x00000008U
#define	US_CSR_RXBRK		0x00000004U
#define	US_CSR_TXRDY		0x00000002U
#define	US_CSR_RXRDY		0x00000001U

/* Channel Status Register bits (int bits + bits below): */
#define	US_CSR_CTS		0x00800000U
#define	US_CSR_DCD		0x00400000U
#define	US_CSR_DSR		0x00200000U
#define	US_CSR_RI		0x00100000U


#define	USART_INIT(sc, speed) do {					\
  at91usart_writereg(sc, US_PDC + PDC_PTCR,				\
		      PDC_PTCR_TXTDIS | PDC_PTCR_RXTDIS);		\
  at91usart_writereg(sc, US_PDC + PDC_RNCR, 0);				\
  at91usart_writereg(sc, US_PDC + PDC_RCR, 0);				\
  at91usart_writereg(sc, US_PDC + PDC_TNCR, 0);				\
  at91usart_writereg(sc, US_PDC + PDC_TCR, 0);				\
  at91usart_writereg(sc, US_MR, US_MR_USCLKS_MCK | US_MR_CHRL_8		\
		      | US_MR_PAR_NONE | US_MR_NBSTOP_1);		\
  at91usart_writereg(sc, US_BRGR,					\
		      (AT91_MSTCLK / 16 + (speed) / 2) / (speed));	\
  at91usart_writereg(sc, US_CR, US_CR_RSTRX | US_CR_RSTTX);		\
  at91usart_writereg(sc, US_CR, US_CR_RSTSTA | US_CR_RXEN | US_CR_TXEN	\
		      | US_CR_STPBRK);					\
  (void)at91usart_readreg(sc, US_CSR);					\
} while (/*CONSTCOND*/0)

#if 0
#define	USART_PUTC(sc, ch) do {						\
  while ((USARTREG(USART_SR) & USART_SR_TXRDY) == 0) ;			\
  USARTREG(USART_THR) = ch;						\
} while (/*CONSTCOND*/0)

#define	USART_PEEKC() ((USARTREG(USART_SR) & USART_SR_RXRDY) ? USARTREG(USART_RHR) : -1)

#define	USART_PUTS(string) do {						\
  const char *_ptr = (string);						\
  while (*_ptr) {							\
    USART_PUTC(*_ptr);							\
    _ptr++;								\
  }									\
} while (/*CONSTCOND*/0)
#endif

#endif	// _AT91USARTREG_H_

