/* $NetBSD: s3c2xx0reg.h,v 1.7 2014/03/17 10:27:46 skrll Exp $ */

/*
 * Copyright (c) 2002, 2003 Fujitsu Component Limited
 * Copyright (c) 2002, 2003 Genetec Corporation
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
 * 3. Neither the name of The Fujitsu Component Limited nor the name of
 *    Genetec corporation may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY FUJITSU COMPONENT LIMITED AND GENETEC
 * CORPORATION ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL FUJITSU COMPONENT LIMITED OR GENETEC
 * CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


/*
 * Register definitions common to S3C2800 and S3C24[01]0
 */
#ifndef _ARM_S3C2XX0_S3C2XX0REG_H_
#define	_ARM_S3C2XX0_S3C2XX0REG_H_

/* UART */
/*
 * S3C2800, 2410 and 2400 have a common built-in UART block. However,
 * there are small diffs in bit position of some registers.
 * Following definitions can be found in s3c{2800,24x0}reg.h for
 * that reason.
 *
 *  UMCON_AFC                (Auto flow control)
 *  UMSTAT_DCTS              (CTS change)
 */

#define	SSCOM_ULCON 0x00 /* UART line control */
#define	 ULCON_IR  	(1<<6)
#define	 ULCON_PARITY_SHIFT  3
#define	 ULCON_PARITY_NONE  (0<<ULCON_PARITY_SHIFT)
#define	 ULCON_PARITY_ODD   (4<<ULCON_PARITY_SHIFT)
#define	 ULCON_PARITY_EVEN  (5<<ULCON_PARITY_SHIFT)
#define	 ULCON_PARITY_ONE   (6<<ULCON_PARITY_SHIFT)
#define	 ULCON_PARITY_ZERO  (7<<ULCON_PARITY_SHIFT)
#define	 ULCON_STOP	(1<<2)
#define	 ULCON_LENGTH_5 0
#define	 ULCON_LENGTH_6 1
#define	 ULCON_LENGTH_7 2
#define	 ULCON_LENGTH_8 3
#define	SSCOM_UCON	0x04	/* UART control */
#define	 UCON_TXINT_TYPE	(1<<9)	/* Tx interrupt. 0=pulse,1=level */
#define	 UCON_TXINT_TYPE_LEVEL  UCON_TXINT_TYPE
#define	 UCON_TXINT_TYPE_PULSE  0
#define	 UCON_RXINT_TYPE	(1<<8)	/* Rx interrupt */
#define	 UCON_RXINT_TYPE_LEVEL  UCON_RXINT_TYPE
#define	 UCON_RXINT_TYPE_PULSE  0
#define	 UCON_TOINT	(1<<7)	/* Rx timeout interrupt */
#define	 UCON_ERRINT	(1<<6)	/* receive error interrupt */
#define	 UCON_LOOP	(1<<5)	/* loopback */
#define	 UCON_SBREAK	(1<<4)	/* send break */
#define	 UCON_TXMODE_DISABLE (0<<2)
#define	 UCON_TXMODE_INT     (1<<2)
#define	 UCON_TXMODE_DMA     (2<<2)
#define	 UCON_TXMODE_MASK    (3<<2)
#define	 UCON_RXMODE_DISABLE (0<<0)
#define	 UCON_RXMODE_INT     (1<<0)
#define	 UCON_RXMODE_DMA     (2<<0)
#define	 UCON_RXMODE_MASK    (3<<0)
#define	SSCOM_UFCON	0x08	/* FIFO control */
#define	 UFCON_TXTRIGGER_0	(0<<6)
#define	 UFCON_TXTRIGGER_4	(1<<6)
#define	 UFCON_TXTRIGGER_8	(2<<6)
#define	 UFCON_TXTRIGGER_16	(3<<6)
#define	 UFCON_RXTRIGGER_4	(0<<4)
#define	 UFCON_RXTRIGGER_8	(1<<4)
#define	 UFCON_RXTRIGGER_12	(2<<4)
#define	 UFCON_RXTRIGGER_16	(3<<4)
#define	 UFCON_TXFIFO_RESET	(1<<2)
#define	 UFCON_RXFIFO_RESET	(1<<1)
#define	 UFCON_FIFO_ENABLE	(1<<0)
#define	SSCOM_UMCON	0x0c	/* MODEM control */
/*       UMCON_AFC is defined in s3c{2800,24x0}reg.h */
#define	 UMCON_RTS	(1<<0)	/* Request to send */
#define	SSCOM_UTRSTAT	0x10	/* Status register */
#define	 UTRSTAT_TXSHIFTER_EMPTY   (1<<2)
#define	 UTRSTAT_TXEMPTY           (1<<1) /* TX fifo or buffer empty */
#define	 UTRSTAT_RXREADY	   (1<<0) /* RX fifo or buffer is not empty */
#define	SSCOM_UERSTAT	0x14	/* Error status register */
#define	 UERSTAT_BREAK	  (1<<3) /* Break signal */
#define	 UERSTAT_FRAME	  (1<<2) /* Frame error */
#define	 UERSTAT_PARITY	  (1<<1) /* Parity error */
#define	 UERSTAT_OVERRUN  (1<<0) /* Overrun */
#define	 UERSTAT_ALL_ERRORS (UERSTAT_OVERRUN|UERSTAT_BREAK|UERSTAT_FRAME|UERSTAT_PARITY)
#define	SSCOM_UFSTAT	0x18	/* Fifo status register */
#define	 UFSTAT_TXFULL	  (1<<9) /* Tx fifo full */
#define	 UFSTAT_RXFULL	  (1<<8) /* Rx fifo full */
#define	 UFSTAT_TXCOUNT_SHIFT 4		/* TX FIFO count */
#define	 UFSTAT_TXCOUNT	  (0x0f<<UFSTAT_TXCOUNT_SHIFT)
#define	 UFSTAT_RXCOUNT_SHIFT 0		/* RX FIFO count */
#define	 UFSTAT_RXCOUNT	  (0x0f<<UFSTAT_RXCOUNT_SHIFT)
#define	SSCOM_UMSTAT	0x1c	/* Modem status register */
/*       UMSTAT_DCTS is defined in s3c{2800,24x0}reg.h */
#define	 UMSTAT_CTS	  (1<<0) /* Clear to send */
#if _BYTE_ORDER == _LITTLE_ENDIAN
#define	SSCOM_UTXH	0x20	/* Transmit data register */
#define	SSCOM_URXH	0x24	/* Receive data register */
#else
#define	SSCOM_UTXH	0x23	/* Transmit data register */
#define	SSCOM_URXH	0x27	/* Receive data register */
#endif
#define	SSCOM_UBRDIV	0x28	/* baud-reate divisor */
#define	SSCOM_SIZE  0x2c

/* Interrupt controller (Common to S3c2800/2400X/2410X) */
#define	INTCTL_SRCPND	0x00	/* Interrupt request status */
#define	INTCTL_INTMOD	0x04	/* Interrupt mode (FIQ/IRQ) */
#define	INTCTL_INTMSK	0x08	/* Interrupt mask */

#endif /* _ARM_S3C2XX0_S3C2XX0REG_H_ */
