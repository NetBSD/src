/*	$NetBSD: windermerereg.h,v 1.1 2013/04/28 12:11:26 kiyohara Exp $	*/
/*
 * Copyright (c) 2012 KIYOHARA Takashi
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define WINDERMERE_LCD_OFFSET		0x200
#define WINDERMERE_INTR_OFFSET		0x500
#define WINDERMERE_COM0_OFFSET		0x600
#define WINDERMERE_COM1_OFFSET		0x700
#define WINDERMERE_TC_OFFSET		0xc00
#define   TC1_OFFSET			 0x00
#define   TC2_OFFSET			 0x20
#define WINDERMERE_RTC_OFFSET		0xd00
#define WINDERMERE_GPIO_OFFSET		0xe00

/* LCD */
#define LCD_PALETTE_SIZE	0x20
#define LCDCTL		0x00
#define LCDCTL_EN		(1 << 0)
#define LCDCTL_BW		(1 << 1)
#define LCDCTL_DP		(1 << 2)
#define LCDCTL_DONE		(1 << 3)
#define LCDCTL_NEXT		(1 << 4)
#define LCDCTL_ERR		(1 << 5)
#define LCDCTL_TFT		(1 << 7)
#define LCDCTL_M8B		(1 << 8)
#define LCDST		0x01
#define LCDST_NEXT		(1 << 1)
#define LCDST_BER		(1 << 2)
#define LCDST_ABC		(1 << 3)
#define LCDST_FUF		(1 << 5)
#define LCDDBAR1	0x04
#define LCDT0		0x08
#define LCDT0_HSW		(2 << 10)	/* Horizontal sync plus width */
#define LCDT0_HFP		(1 << 16)	/* Horizontal front porch */
#define LCDT0_HBP		(1 << 24)	/* Horizontal back porch */
#define LCDT1		0x09
#define LCDT1_VSW		(2 << 10)	/* Vertical sync plus width */
#define LCDT1_VFP		(1 << 16)	/* Vertical front porch */
#define LCDT1_VBP		(1 << 24)	/* Vertical back porch */
#define LCDT2		0x0a
#define LCDT2_IVS		(1 << 20)
#define LCDT2_IHS		(1 << 21)
#define LCDT2_IPC		(1 << 22)
#define LCDT2_IEO		(1 << 23)

/* Interrupt */
#define INTSR		0x00	/* Interrupt Status(after masking) */
#define INTRSR		0x01	/* Interrupt Status(before masking) */
#define INTENS		0x02	/* Interrupr Enable */
#define INTENC		0x03	/* Interrupr Disable */

/* UART */
#define UART_SIZE	0x20
#define UART_FIFO_SIZE	0x10
#define UARTDR		0x00
#define   RSR_FE		(1 << 0)	/* bit8:  Frame Error */
#define   RSR_PE		(1 << 1)	/* bit9:  Parity Error */
#define   RSR_OE		(1 << 2)	/* bit10: Overrun Error */
#define UARTFCR		0x01
#define   FCR_BREAK		(1 << 0)
#define   FCR_PRTEN		(1 << 1)
#define   FCR_EVENPRT		(1 << 2)
#define   FCR_XSTOP		(1 << 3)
#define   FCR_UFIFOEN		(1 << 4)
#define   FCR_WLEN_5		(0 << 5)
#define   FCR_WLEN_6		(1 << 5)
#define   FCR_WLEN_7		(2 << 5)
#define   FCR_WLEN_8		(3 << 5)
#define UARTLCR		0x02
#define UARTCON		0x03
#define   CON_UARTEN		(1 << 0)	/* UART Enable */
#define   CON_SIREN		(1 << 1)	/* SiR disable */
#define   CON_IRTXM		(1 << 2)	/* IrDA TX mode bit */
#define UARTFR		0x04
#define   FR_TXFF		(1 << 5)
#define   FR_RXFE		(1 << 4)
#define   FR_BUSY		(1 << 3)
#define   FR_DCD		(1 << 2)
#define   FR_DSR		(1 << 1)
#define   FR_CTS		(1 << 0)
#define UARTINT		0x05
#define UARTINTM	0x06	/* Mask register */
#define UARTINTR	0x07	/* Raw status register */
#define   INT_RXINT		(1 << 0)
#define   INT_TXINT		(1 << 1)
#define   INT_MSINT		(1 << 2)	/* MODEM status */

/* Time Counter (decremental counter) */
#define TC_BITS		16	/* 16bit counter */
#define TC_MAX		((1 << TC_BITS) - 1)
#define TC_MASK		TC_MAX
#define TC_LOAD		0x00
#define TC_VAL		0x01
#define TC_CTRL		0x02
#define   CTRL_CLKSEL		(1 << 3)	/* 512kHz */
#define   CTRL_MODE		(1 << 6)	/* Periodic Mode */
#define   CTRL_ENABLE		(1 << 7)
#define TC_EOI		0x03
#define   EOI_EOI		(1 << 0)

/* Real Time Clock */
#define RTC_DRL		0x00	/* Data Register Low */
#define RTC_DRH		0x01	/* Data Register High */
#define RTC_MRL		0x02	/* Match Register Low */
#define RTC_MRH		0x03	/* Match Register High */

/* GPIO/KBD */
#define GPIO_PADR	0x00
#define GPIO_PBDR	0x01
#define GPIO_PCDR	0x02
#define GPIO_PDDR	0x03
#define GPIO_PADDR	0x04
#define GPIO_PBDDR	0x05
#define GPIO_PCDDR	0x06
#define GPIO_PDDDR	0x07
#define GPIO_PEDR	0x08
#define GPIO_PEDDR	0x09
#define KSCAN		0x0a
