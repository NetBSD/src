/*	$NetBSD: netwalker_reg.h,v 1.1.2.2 2010/11/15 14:38:23 uebayasi Exp $	*/

/*
 * Copyright (c) 2010  Genetec Corporation.  All rights reserved.
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


#ifndef _EVBARM_NETWALKER_REG_H
#define _EVBARM_NETWALKER_REG_H

#include <arm/imx/imx51reg.h>

/* map UART1 and IOMUXC for bootstrap */
#define	NETWALKER_IO_VBASE0	0xfd000000
#define	NETWALKER_IO_PBASE0	0x73f00000	/* GPIO, IOMUXC, UART */

#define	NETWALKER_UART1_VBASE	\
	(IMX51_UART1_BASE-NETWALKER_IO_PBASE0+NETWALKER_IO_VBASE0)
#define	NETWALKER_IOMUXC_VBASE	\
	(IOMUXC_BASE-NETWALKER_IO_PBASE0+NETWALKER_IO_VBASE0)

/* GPIO[1..4] */
#define	NETWALKER_GPIO1_VBASE	\
	(GPIO1_BASE-NETWALKER_IO_PBASE0+NETWALKER_IO_VBASE0)
#define	NETWALKER_GPIO_VBASE(n)	\
	(NETWALKER_GPIO1_VBASE+((n)-1)*0x4000)
#define	NETWALKER_WDOG_VBASE	\
	(WDOG1_BASE-NETWALKER_IO_PBASE0+NETWALKER_IO_VBASE0)
	

#define ioreg_read(a)		(*(volatile uint32_t *)(a))
#define ioreg_write(a,v)	(*(volatile uint32_t *)(a)=(v))

#define	ioreg32_read	ioreg_read
#define	ioreg32_write	ioreg_write

#define	ioreg16_read(a) 	(*(volatile uint16_t *)(a))
#define	ioreg16_write(a,v)	(*(volatile uint16_t *)(a) = (v))

#define	ioreg8_read(a)  	(*(volatile uint8_t *)(a))
#define	ioreg8_write(a,v)	(*(volatile uint8_t *)(a) = (v))

#endif /* _EVBARM_NETWALKER_REG_H */
