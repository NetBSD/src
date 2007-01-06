/*	$NetBSD: omap_com.h,v 1.1 2007/01/06 00:29:52 christos Exp $	*/

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain this list of conditions
 *    and the following disclaimer.
 * 2. Redistributions in binary form must reproduce this list of conditions
 *    and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_OMAP_OMAP_COM_H_
#define _ARM_OMAP_OMAP_COM_H_

/*
 * Minimal defines for the bits and bytes we need.  We need these definitions
 * in both omap_com.c and the machdep startup file to get the console and kgdb
 * ports initialized.
 */
#define OMAP_COM_SIZE			1024 	/* Per the OMAP TRM. */

/*
 * Registers 0x00 to 0x07 are pretty much 16550 compatible.  Let the com
 * driver handle them.
 */

/* Mode Definition Register 1 */
#define OMAP_COM_MDR1			0x08
/* There are IrDA specific bits as well. */
#define  OMAP_COM_MDR1_MODE_MASK		(7<<0)
#define  OMAP_COM_MDR1_MODE_UART_16X		(0<<0)
#define  OMAP_COM_MDR1_MODE_SIR			(1<<0)
#define  OMAP_COM_MDR1_MODE_UART_16X_AUTOBAUD	(2<<0)
#define  OMAP_COM_MDR1_MODE_UART_13X		(3<<0)
#define  OMAP_COM_MDR1_MODE_MIR			(4<<0)
#define  OMAP_COM_MDR1_MODE_FIR			(5<<0)
#define  OMAP_COM_MDR1_MODE_DISABLE		(7<<0)

/* Mode Definition Register 2 (0x09) is for IrDA */

/* Status FIFO Line Status and Transmit Frame Length Low (0x0A) are for IrDA */
/* Resume and Transmit Frame Length High (0x0B) are for IrDA */
/* Status FIFO Low and Received Frame Length Low (0x0C) are for IrDA */
/* Status FIFO High and Received Frame Length High (0x0D) are for IrDA */
/* BOF Control Register (0x0E) is for IrDA */

/* UART Autobauding Status Register */
#define OMAP_COM_UASR			0x0E
#define  OMAP_COM_UASR_PARITY_MASK		(3<<6)
#define   OMAP_COM_UASR_PARITY_NONE		(0<<6)
#define   OMAP_COM_UASR_PARITY_SPACE		(1<<6)
#define   OMAP_COM_UASR_PARITY_EVEN		(2<<6)
#define   OMAP_COM_UASR_PARITY_ODD		(3<<6)
#define  OMAP_COM_UASR_BIT_BY_CHAR_MASK		(1<<5)
#define   OMAP_COM_UASR_BIT_BY_CHAR_7		(0<<5)
#define   OMAP_COM_UASR_BIT_BY_CHAR_8		(1<<5)
#define  OMAP_COM_UASR_SPEED_MASK		(0x1F<<0)
#define   OMAP_COM_UASR_SPEED_NONE		(0x00<<0)
#define   OMAP_COM_UASR_SPEED_115200		(0x01<<0)
#define   OMAP_COM_UASR_SPEED_57600		(0x02<<0)
#define   OMAP_COM_UASR_SPEED_38400		(0x03<<0)
#define   OMAP_COM_UASR_SPEED_28800		(0x04<<0)
#define   OMAP_COM_UASR_SPEED_19200		(0x05<<0)
#define   OMAP_COM_UASR_SPEED_14400		(0x06<<0)
#define   OMAP_COM_UASR_SPEED_9600		(0x07<<0)
#define   OMAP_COM_UASR_SPEED_4800		(0x08<<0)
#define   OMAP_COM_UASR_SPEED_2400		(0x09<<0)
#define   OMAP_COM_UASR_SPEED_1200		(0x0A<<0)

/* Auxiliary Control Register (0x0F) is for IrDA */

/* Supplementary Control Register */
#define OMAP_COM_SCR			0x10
#define  OMAP_COM_SCR_RX_TRIG_GRANU1		(1<<7)
#define  OMAP_COM_SCR_TX_TRIG_GRANU1		(1<<6)
#define  OMAP_COM_SCR_DSR_IT			(1<<5)
#define  OMAP_COM_SCR_RX_CTS_DSR_WAKE_UP_ENABLE	(1<<4)
#define  OMAP_COM_SCR_TX_EMPTY_CTL_IT		(1<<3)
#define  OMAP_COM_SCR_DMA_MODE_2_0		(0<<1)
#define  OMAP_COM_SCR_DMA_MODE_2_1		(1<<1)
#define  OMAP_COM_SCR_DMA_MODE_2_2		(2<<1)
#define  OMAP_COM_SCR_DMA_MODE_2_3		(3<<1)
#define  OMAP_COM_SCR_DMA_MODE_CTL		(1<<0)

/* Supplementary Status Register */
#define OMAP_COM_SSR			0x11
#define  OMAP_COM_SSR_RX_CTS_DSR_WAKE_UP_STS	(1<<1)
#define  OMAP_COM_SSR_TX_FIFO_FULL		(1<<0)

/* BOF Length Register (0x12) is for IrDA */

/* Module Version Register */
#define OMAP_COM_MVR			0x14
#define  OMAP_COM_MVR_MAJOR(r)			((r>>4) & 0x0F)
#define  OMAP_COM_MVR_MINOR(r)			((r>>0) & 0x0F)

/* System Configuration Register */
#define OMAP_COM_SYSC			0x15
#define  OMAP_COM_SYSC_FORCE_IDLE		(0<<3)
#define  OMAP_COM_SYSC_NO_IDLE			(1<<3)
#define  OMAP_COM_SYSC_SMART_IDLE		(2<<3)
#define  OMAP_COM_SYSC_ENA_WAKE_UP		(1<<2)
#define  OMAP_COM_SYSC_SOFT_RESET		(1<<1)
#define  OMAP_COM_SYSC_AUTOIDLE			(1<<0)

/* System Status Register */
#define OMAP_COM_SYSS			0x16
#define  OMAP_COM_SYSS_RESET_DONE		(1<<0)

/* Wake-Up Enable Register */
#define OMAP_COM_WER			0x17
#define  OMAP_COM_WER_LINE_STATUS		(1<<6)
#define  OMAP_COM_WER_RHR			(1<<5)
#define  OMAP_COM_WER_RX			(1<<4)
#define  OMAP_COM_WER_DCD			(1<<3)
#define  OMAP_COM_WER_RI			(1<<2)
#define  OMAP_COM_WER_DSR			(1<<1)
#define  OMAP_COM_WER_CTS			(1<<0)

/* Base frequency */
#define OMAP_COM_FREQ	48000000L

#endif /* _ARM_OMAP_OMAP_COM_H_ */
