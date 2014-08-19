/*	$NetBSD: kobo_reg.h,v 1.1.6.2 2014/08/20 00:02:55 tls Exp $	*/

/*
 * Copyright (c) 2012  Genetec Corporation.  All rights reserved.
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
 *
 */

#ifndef _EVBARM_KOBO_REG_H
#define _EVBARM_KOBO_REG_H

#include "opt_imx.h"

#include <arm/imx/imx51reg.h>

#define DDRSDRAM_BASE	CSD0DDR_BASE

/* map UART2 and IOMUXC for bootstrap */
#define	KOBO_PRIVATE_VBASE	0xf0000000
#define	KOBO_IO_VBASE0		0xfd000000
#define	KOBO_IO_PBASE0		0x53f00000	/* GPIO, IOMUXC, UART */

#define	KOBO_UART2_VBASE	\
	(IMX51_UART2_BASE-KOBO_IO_PBASE0+KOBO_IO_VBASE0)
#define	KOBO_IOMUXC_VBASE	\
	(IOMUXC_BASE-KOBO_IO_PBASE0+KOBO_IO_VBASE0)

/* GPIO[1..4] */
#define	KOBO_GPIO1_VBASE	\
	(GPIO1_BASE-KOBO_IO_PBASE0+KOBO_IO_VBASE0)
#define	KOBO_GPIO_VBASE(n)	\
	(KOBO_GPIO1_VBASE+((n)-1)*0x4000)
#define	KOBO_WDOG_VBASE	\
	(WDOG1_BASE-KOBO_IO_PBASE0+KOBO_IO_VBASE0)

#define ioreg_read(a)		(*(volatile uint32_t *)(a))
#define ioreg_write(a,v)	(*(volatile uint32_t *)(a)=(v))

#define	ioreg32_read	ioreg_read
#define	ioreg32_write	ioreg_write

#define	ioreg16_read(a) 	(*(volatile uint16_t *)(a))
#define	ioreg16_write(a,v)	(*(volatile uint16_t *)(a) = (v))

#define	ioreg8_read(a)  	(*(volatile uint8_t *)(a))
#define	ioreg8_write(a,v)	(*(volatile uint8_t *)(a) = (v))

#endif /* _EVBARM_KOBO_REG_H */
