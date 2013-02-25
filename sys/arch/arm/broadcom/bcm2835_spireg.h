/*	$NetBSD: bcm2835_spireg.h,v 1.1.8.2 2013/02/25 00:28:25 tls Exp $	*/

/*
 * Copyright (c) 2012 Jonathan A. Kollasch
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _BROADCOM_BCM2835_SPIREG_H_
#define _BROADCOM_BCM2835_SPIREG_H_

#include <sys/cdefs.h>

#define __BIT32(x)	((uint32_t)__BIT(x))
#define __BITS32(x, y)	((uint32_t)__BITS((x), (y)))

#define SPI_CS		0x000
#define SPI_CS_CS		__BITS32(1,0)
#define SPI_CS_CPHA		__BIT32(2)
#define SPI_CS_CPOL		__BIT32(3)
#define SPI_CS_CLEAR_TX		__BIT32(4)
#define SPI_CS_CLEAR_RX		__BIT32(5)
#define SPI_CS_CSPOL		__BIT32(6)
#define SPI_CS_TA		__BIT32(7)
#define SPI_CS_DMAEN		__BIT32(8)
#define SPI_CS_INTD		__BIT32(9)
#define SPI_CS_INTR		__BIT32(10)
#define SPI_CS_ADCS		__BIT32(11)
#define SPI_CS_REN		__BIT32(12)
#define SPI_CS_LEN		__BIT32(13)
#define SPI_CS_LMONO		__BIT32(14)
#define SPI_CS_TE_EN		__BIT32(15)
#define SPI_CS_DONE		__BIT32(16)
#define SPI_CS_RXD		__BIT32(17)
#define SPI_CS_TXD		__BIT32(18)
#define SPI_CS_RXR		__BIT32(19)
#define SPI_CS_RXF		__BIT32(20)
#define SPI_CS_CSPOL0		__BIT32(21)
#define SPI_CS_CSPOL1		__BIT32(22)
#define SPI_CS_CSPOL2		__BIT32(23)
#define SPI_CS_DMA_LEN		__BIT32(24)
#define SPI_CS_LEN_LONG		__BIT32(25)

#define SPI_FIFO	0x004

#define SPI_CLK		0x008
#define SPI_CLK_CDIV		__BITS32(15,0)

#define SPI_DLEN	0x00c

#define SPI_LTOH	0x010

#define SPI_DC		0x014

#endif /* _BROADCOM_BCM2835_SPIREG_H_ */
