/*	$NetBSD: ixp12x0_comreg.h,v 1.6 2003/02/22 05:32:00 igy Exp $ */

/*
 * Copyright (c) 2002
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.  All rights reserved.
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
 *	This product includes software developed by Ichiro FUKUHARA.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS 
 * HEAD BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * IXP12X0 UART register
 *  UART_SR 0x90003400
 *  UART_CR 0x90003800
 *  UART_DR 0x90003C00
 */

#define IXPCOM_FREQ		(3686400 / 16)
#define IXPCOMSPEED(b)		(IXPCOM_FREQ / (b) - 1)
#define IXPCOMSPEED2BRD(b)	(IXPCOMSPEED(b) << 16)

/* I/O space */
#define IXPCOM_UART_SIZE	0x00004000UL
#define IXPCOM_UART_OFFSET	0x00000000UL
#define	IXPCOM_UART_HWBASE	(IXP12X0_SYS_HWBASE + IXPCOM_UART_OFFSET)
/* IXPCOM_UART_VBASE should be used only for console's ioh. */
#define	IXPCOM_UART_VBASE	(IXP12X0_SYS_VBASE + IXPCOM_UART_OFFSET)

/* UART control register */
#define IXPCOM_CR	0x00003800UL
#define CR_BRK		0x01	/* Break */
#define CR_PE		0x02	/* Parity enable */
#define CR_OES		0x04	/* Odd/even parity select */
#define CR_SBS		0x08	/* Stop bit select */
#define	 SBS_1STOP	(0 << 3)	/*  1 Stop Bit */
#define	 SBS_2STOP	(1 << 3)	/*  2 Stop Bit */
#define CR_RIE		0x10	/* Receive FIFO interrupt enable */
#define CR_DSS		0x60	/* Data size select */
#define	 DSS_5BIT	(0 << 5)	/*  5Bits */
#define	 DSS_6BIT	(1 << 5)	/*  6Bits */
#define	 DSS_7BIT	(2 << 5)	/*  7Bits */
#define	 DSS_8BIT	(3 << 5)	/*  8Bits */
#define	CR_UE		0x80	/* UART Enable */
#define	 UE_DISABLE	(0 << 7)	/*  UART Disabled */
#define	 UE_ENABLE	(1 << 7)	/*  UART Enabled */
#define CR_XIE		0x100	/* Transmit FIFO interrupt enable */
#define CR_UIS		0x200	/* UART Interrupt Select */
#define CR_BRD		0x03ff0000	/* Baud rate divisor */


/* UART Status register */
#define IXPCOM_SR	0x00003400UL
#define SR_PRE		0x01	/* Parity error */
#define SR_FRE		0x02	/* Framing error */
#define	SR_TXR		0x04	/* Transmit FIFO Ready */
#define SR_ROR		0x08	/* Receiver overrun */
#define	SR_RXR		0x10	/* Receiver FIFO Ready */
#define SR_TXE		0x20	/* Transmit FIFO Empty */
#define	SR_RXF		0x40	/* Receiver FIFO Full */
#define	SR_TXF		0x80	/* Transmit FIFO Full */


/* UART data register */
#define IXPCOM_DR	0x00003C00UL
#define DR_PRE		0x100	/* Parity error */
#define DR_FRE		0x200	/* Framing error */
#define DR_ROR		0x400	/* Receiver overrun */

#define IXPCOMSPLRAISED		(~(CR_RIE | CR_XIE))
#define IXPCOMSPLLOWERD		(~0UL);
