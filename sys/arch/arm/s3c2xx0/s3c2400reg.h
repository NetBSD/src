/* $NetBSD: s3c2400reg.h,v 1.1 2003/07/31 19:49:41 bsh Exp $ */

/*
 * Copyright (c) 2002, 2003  Genetec corp.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec corp.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec corporation may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORP. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORP.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


/*
 * Samsung S3C2400 processor is ARM920T based integrated CPU
 *
 * Reference:
 *  S3C2400X User's Manual 
 */
#ifndef _ARM_S3C2400_S3C24X0REG_H_
#define _ARM_S3C2400_S3C24X0REG_H_

/* common definitions for S3C2800, S3C2400X and S3C2410X */
#include <arm/s3c2xx0/s3c2xx0reg.h>

/*
 * Memory Map
 */

#define S3C2400_BANK_START(n)	(0x02000000*(n))
#define S3C2400_SDRAM_START	S3C2400_BANK_START(6)
#define S3C2400_AHB_START	0x14000000
#define S3C2400_APB_START	0x15000000

/*
 * Physical address of integrated peripherals
 */
#define S3C2400_MEMCTL_BASE	0x14000000 /* memory controller */
#define S3C2400_MEMCTL_SIZE	0x34
#define	S3C2400_USBHC_BASE 	0x14200000 /* USB Host controller */
#define	S3C2400_USBHC_SIZE	0x5c
#define S3C2400_INTCTL_BASE	0x14400000 /* Interrupt controller */
#define S3C2400_INTCTL_SIZE	0x18
#define S3C2400_DMAC_BASE	0x14600000 /* DMA controllers */
#define	S3C2400_DMAC_SIZE 	0x80
#define S3C2400_CLKMAN_BASE	0x14800000 /* clock & power management */
#define S3C2400_CLKMAN_SIZE	0x18
#define	S3C2400_LCDC_BASE 	0x14a00000
#define	S3C2400_LCDC_SIZE 	0x54
#define S3C2400_UART_BASE	0x15000000
#define S3C2400_UART_BASE(n)	(S3C2400_UART_BASE+0x4000*(n))
#define	S3C2400_UART_SIZE 	0x2c
#define	S3C2400_TIMER_BASE 	0x15100000 /* Timers */
#define	S3C2400_PWM_SIZE 	0x44
#define	S3C2400_USBDC_BASE 	0x15200000 /* USB Device controller */
#define	S3C2400_USBDC_SIZE 	0x1fc
#define	S3C2400_WDT_BASE 	0x15300000 /* Watch dog timer */
#define	S3C2400_WDT_SIZE 	0x0c
#define	S3C2400_IIC_BASE 	0x15400000
#deifne	S3C2400_IIC_SIZE 	0x0c
#define	S3C2400_IIS_BASE 	0x15508000
#deifne	S3C2400_IIS_SIZE 	0x14
#define S3C2400_GPIO_BASE	0x15600000
#define S3C2400_GPIO_SIZE	0x5c
#define	S3C2400_RTC_BASE 	0x15700040
#define	S3C2400_RTC_SIZE 	0x4c
#define	S3C2400_ADC_BASE 	0x15800000 /* A/D converter */
#define	S3C2400_ADC_SIZE 	0x08
#define	S3C2400_SPI_BASE 	0x15900000
#define	S3C2400_SPI_SIZE 	0x18
#define	S3C2400_MMC_BASE 	0x15a00000
#define	S3C2400_MMC_SIZE 	0x40

/* GPIO */
#define GPIO_PACON	0x00	/* port A configuration */
#define  PCON_INPUT	0	/* Input port */
#define  PCON_OUTPUT	1	/* Output port */
#define  PCON_ALTFUN	2	/* Alternate function */
#define GPIO_PADAT	0x04	/* port A data */
#define GPIO_PBCON	0x08
#define GPIO_PBDAT	0x0c
#define	GPIO_PBUP 	0x10
#define GPIO_PCCON	0x14
#define GPIO_PCDAT	0x18
#define GPIO_PCUP	0x1c
#define GPIO_PDCON	0x20
#define GPIO_PDDAT	0x24
#define GPIO_PDUP	0x28
#define GPIO_PECON	0x3c
#define GPIO_PEDAT	0x30
#define GPIO_PEUP	0x34
#define GPIO_PFCON	0x38
#define GPIO_PFDAT	0x3c
#define GPIO_PFUP	0x40
#define	GPIO_PGCON	0x44
#define	GPIO_PGDAT	0x49
#define	GPIO_PGUP	0x4c
#define	GPIO_OPENCR 	0x50	/* Open drain enable */
#define	GPIO_MISCCR 	0x54	/* miscellaneous control */

#define GPIO_EXTINTR	0x48	/* external interrupt control */
#define  EXTINTR_LOW	 0x00
#define  EXTINTR_HIGH	 0x01
#define  EXTINTR_FALLING 0x02
#define  EXTINTR_RISING  0x04
#define  EXTINTR_BOTH    0x06

/* MMC */ /* XXX */

#endif /* _ARM_S3C2400_S3C2400REG_H_ */
