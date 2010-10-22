/*	$NetBSD: gumstixreg.h,v 1.4.2.2 2010/10/22 07:21:16 uebayasi Exp $  */
/*
 * Copyright (C) 2005, 2006 WIDE Project and SOUM Corporation.
 * All rights reserved.
 *
 * Written by Takashi Kiyohara and Susumu Miki for WIDE Project and SOUM
 * Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the name of SOUM Corporation
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT and SOUM CORPORATION ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT AND SOUM CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _EVBARM_GUMSTIXREG_H_
#define _EVBARM_GUMSTIXREG_H_

/*
 * Logical mapping for onboard/integrated peripherals
 * that are used while bootstrapping.
 */
#define GUMSTIX_IO_AREA_VBASE		0xfd000000
#define GUMSTIX_INTCTL_VBASE		0xfd000000
#define GUMSTIX_CLKMAN_VBASE		0xfd100000
#define GUMSTIX_GPIO_VBASE		0xfd200000
#define GUMSTIX_FFUART_VBASE		0xfd300000
#define GUMSTIX_STUART_VBASE		0xfd400000
#define GUMSTIX_BTUART_VBASE		0xfd500000
#define GUMSTIX_HWUART_VBASE		0xfd600000
#define GUMSTIX_LCDC_VBASE		0xfd700000

#define OVERO_L4_PERIPHERAL_VBASE	0x90000000
#define OVERO_GPMC_VBASE		0x90100000


#define ioreg_read(a)		(*(volatile unsigned *)(a))
#define ioreg_write(a,v)	(*(volatile unsigned *)(a)=(v))

#define ioreg16_read(a)		(*(volatile uint16_t *)(a))
#define ioreg16_write(a,v)	(*(volatile uint16_t *)(a)=(v))

#define ioreg8_read(a)		(*(volatile uint8_t *)(a))
#define ioreg8_write(a,v)	(*(volatile uint8_t *)(a)=(v))

#endif /* _EVBARM_GUMSTIXREG_H_ */
