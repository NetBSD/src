/* $NetBSD: bcm2835_bscreg.h,v 1.1.8.2 2013/02/25 00:28:25 tls Exp $ */

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

#ifndef _BROADCOM_BCM2835_BSCREG_H_
#define _BROADCOM_BCM2835_BSCREG_H_

#include <sys/cdefs.h>

#define __BIT32(x)	((uint32_t)__BIT(x))
#define __BITS32(x, y)	((uint32_t)__BITS((x), (y)))

#define BSC_C		0x000
#define BSC_C_READ		__BIT32(0)
#define BSC_C_CLEAR		__BITS32(5,4)
#define BSC_C_CLEAR_NONE	__SHIFTIN(0, BSC_C_CLEAR)
#define BSC_C_CLEAR_CLEAR	__SHIFTIN(3, BSC_C_CLEAR)
#define BSC_C_ST		__BIT32(7)
#define BSC_C_INTD		__BIT32(8)
#define BSC_C_INTT		__BIT32(9)
#define BSC_C_INTR		__BIT32(10)
#define BSC_C_I2CEN		__BIT32(15)

#define BSC_S		0x004
#define BSC_S_TA		__BIT32(0)
#define BSC_S_DONE		__BIT32(1)
#define BSC_S_TXW		__BIT32(2)
#define BSC_S_RXR		__BIT32(3)
#define BSC_S_TXD		__BIT32(4)
#define BSC_S_RXD		__BIT32(5)
#define BSC_S_TXE		__BIT32(6)
#define BSC_S_RXF		__BIT32(7)
#define BSC_S_ERR		__BIT32(8)
#define BSC_S_CLKT		__BIT32(9)

#define BSC_DLEN	0x008
#define BSC_DLEN_DLEN		__BITS32(15,0)

#define BSC_A		0x00c
#define BSC_A_ADDR		__BITS32(6,0)

#define BSC_FIFO	0x010
#define BSC_FIFO_DATA		__BITS32(7,0)

#define BSC_DIV		0x014
#define BSC_DIV_CDIV		__BITS32(15,0)

#define BSC_DEL		0x018
#define BSC_DEL_REDL		__BITS32(15,0)
#define BSC_DEL_FEDL		__BITS32(31,16)

#define BSC_CLKT	0x01c
#define BSC_CLKT_TOUT		__BITS32(15,0)

#endif /* _BROADCOM_BCM2835_BSCREG_H_ */
