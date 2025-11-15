/* $NetBSD: exireg.h,v 1.1 2025/11/15 17:59:23 jmcneill Exp $ */

/*-
 * Copyright (c) 2025 Jared McNeill <jmcneill@invisible.ca>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _WII_DEV_EXIREG_H
#define _WII_DEV_EXIREG_H

#define	EXI_CSR(n)		(0x00 + (n) * 0x14)
#define  EXI_CSR_EXTINT		__BIT(11)
#define  EXI_CSR_EXTINTMASK	__BIT(10)
#define	 EXI_CSR_CS		__BITS(9,7)
#define	 EXI_CSR_CLK		__BITS(6,4)
#define	EXI_MAR(n)		(0x04 + (n) * 0x14)
#define	EXI_LENGTH(n)		(0x08 + (n) * 0x14)
#define	EXI_CR(n)		(0x0c + (n) * 0x14)
#define	 EXI_CR_TLEN		__BITS(5,4)
#define  EXI_CR_RW		__BITS(3,2)
#define  EXI_CR_RW_READ		__SHIFTIN(0, EXI_CR_RW)
#define  EXI_CR_RW_WRITE	__SHIFTIN(1, EXI_CR_RW)
#define  EXI_CR_RW_READWRITE	__SHIFTIN(2, EXI_CR_RW)
#define	 EXI_CR_DMA		__BIT(1)
#define  EXI_CR_TSTART		__BIT(0)
#define	EXI_DATA(n)		(0x10 + (n) * 0x14)

typedef enum {
	EXI_FREQ_1MHZ = 0,
	EXI_FREQ_2MHZ = 1,
	EXI_FREQ_4MHZ = 2,
	EXI_FREQ_8MHZ = 3,
	EXI_FREQ_16MHZ = 4,
	EXI_FREQ_32MHZ = 5,
} exi_freq_t;

#endif /* !_WII_DEV_EXIREG_H */
