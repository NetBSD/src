/*	$NetBSD: zaurus_reg.h,v 1.3.38.1 2012/04/17 00:07:14 yamt Exp $	*/
/*	$OpenBSD: zaurus_reg.h,v 1.7 2005/12/14 14:39:38 uwe Exp $	*/
/*	NetBSD: lubbock_reg.h,v 1.1 2003/06/18 10:51:15 bsh Exp */

/*
 * Copyright (c) 2002, 2003  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec Corporation may not be used to endorse or 
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ZAURUS_REG_H
#define _ZAURUS_REG_H

#include <arm/xscale/pxa2x0cpu.h>

/*
 * Logical mapping for onboard/integrated peripherals
 */
#define	ZAURUS_IO_AREA_VBASE	0xfd000000
#define ZAURUS_GPIO_VBASE	0xfd000000
#define ZAURUS_CLKMAN_VBASE 	0xfd100000
#define ZAURUS_INTCTL_VBASE 	0xfd200000
#define ZAURUS_MEMCTL_VBASE 	0xfd300000
#define ZAURUS_SCOOP0_VBASE	0xfd400000
#define ZAURUS_SCOOP1_VBASE	0xfd500000
#define ZAURUS_FFUART_VBASE	0xfd600000
#define ZAURUS_BTUART_VBASE	0xfd700000
#define ZAURUS_STUART_VBASE	0xfd800000
#define ZAURUS_VBASE_FREE	0xfd900000

#define ioreg_read(a)		(*(volatile uint32_t *)(a))
#define ioreg_write(a,v)	(*(volatile uint32_t *)(a)=(v))
#define ioreg16_read(a)		(*(volatile uint16_t *)(a))
#define ioreg16_write(a,v)	(*(volatile uint16_t *)(a)=(v))
#define ioreg8_read(a)		(*(volatile uint8_t *)(a))
#define ioreg8_write(a,v)	(*(volatile uint8_t *)(a)=(v))

/*
 * Magic numbers for the C860 (PXA255) and C3000 (PXA27x).
 */

/* physical adresses of companion chips */
#define C860_SCOOP0_BASE		0x10800000
#define C3000_SCOOP0_BASE		0x10800000
#define C3000_SCOOP1_BASE		0x08800040

/* processor IRQ numbers */
#define C860_CF0_IRQ			17
#define C3000_CF0_IRQ			105
#define C3000_CF1_IRQ			106

/* processor GPIO pins */
#define C860_GPIO_SD_DETECT_PIN		9
#define C860_RC_IRQ_PIN			4	/* remote control */
#define C860_GPIO_SD_WP_PIN		7
#define C860_CF0_IRQ_PIN		14
#define C860_GPIO_SD_POWER_PIN		33
#define	C3000_GPIO_SD_DETECT_PIN	9
#define C3000_RC_IRQ_PIN		13	/* remote control */
#define	C3000_GPIO_SD_WP_PIN		81
#define C3000_CF1_IRQ_PIN		93
#define C3000_CF0_IRQ_PIN		94

/* USB related pins */
#define C3000_USB_DEVICE_PIN		35	/* Client cable is connected */
#define C3000_USB_HOST_PIN		41	/* Host cable is connected */
#define C3000_USB_HOST_POWER_PIN	37	/* host provides power for USB */

#endif /* _ZAURUS_REG_H */
