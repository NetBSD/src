/*	$NetBSD: octeon_pcmreg.h,v 1.1 2015/04/29 08:32:01 hikaru Exp $	*/

/*
 * Copyright (c) 2007 Internet Initiative Japan, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * PCM/TDM Registers
 */

#ifndef _OCTEON_PCMREG_H_
#define _OCTEON_PCMREG_H_

/* ---- register addresses */

#define	PCM_CLK0_CFG				0x0001070000010000ULL
#define	PCM_CLK0_GEN				0x0001070000010008ULL
#define	PCM0_TDM_CFG				0x0001070000010010ULL
#define	PCM0_DMA_CFG				0x0001070000010018ULL
#define	PCM0_INT_ENA				0x0001070000010020ULL
#define	PCM0_INT_SUM				0x0001070000010028ULL
#define	PCM0_TDM_DBG				0x0001070000010030ULL
#define	PCM_CLK0_DBG				0x0001070000010038ULL
#define	PCM0_TXSTART				0x0001070000010040ULL
#define	PCM0_TXCNT				0x0001070000010048ULL
#define	PCM0_TXADDR				0x0001070000010050ULL
#define	PCM0_RXSTART				0x0001070000010058ULL
#define	PCM0_RXCNT				0x0001070000010060ULL
#define	PCM0_RXADDR				0x0001070000010068ULL
#define	PCM0_TXMSK0				0x0001070000010080ULL
#define	PCM0_TXMSK1				0x0001070000010088ULL
#define	PCM0_TXMSK2				0x0001070000010090ULL
#define	PCM0_TXMSK3				0x0001070000010098ULL
#define	PCM0_TXMSK4				0x00010700000100a0ULL
#define	PCM0_TXMSK5				0x00010700000100a8ULL
#define	PCM0_TXMSK6				0x00010700000100b0ULL
#define	PCM0_TXMSK7				0x00010700000100b8ULL
#define	PCM0_RXMSK0				0x00010700000100c0ULL
#define	PCM0_RXMSK1				0x00010700000100c8ULL
#define	PCM0_RXMSK2				0x00010700000100d0ULL
#define	PCM0_RXMSK3				0x00010700000100d8ULL
#define	PCM0_RXMSK4				0x00010700000100e0ULL
#define	PCM0_RXMSK5				0x00010700000100e8ULL
#define	PCM0_RXMSK6				0x00010700000100f0ULL
#define	PCM0_RXMSK7				0x00010700000100f8ULL

/* ---- register bits */

/* XXX */

/* ---- bus_space */

#define	PCM_BASE_0				0x0001070000010000ULL
#define	PCM_BASE_1				0x0001070000014000ULL
#define	PCM_BASE_2				0x0001070000018000ULL
#define	PCM_BASE_3				0x000107000001c000ULL
#define	PCM_SIZE				0x0100

#define	PCM_CLKN_CFG_OFFSET			0x0000
#define	PCM_CLKN_GEN_OFFSET			0x0008
#define	PCMN_TDM_CFG_OFFSET			0x0010
#define	PCMN_DMA_CFG_OFFSET			0x0018
#define	PCMN_INT_ENA_OFFSET			0x0020
#define	PCMN_INT_SUM_OFFSET			0x0028
#define	PCMN_TDM_DBG_OFFSET			0x0030
#define	PCM_CLKN_DBG_OFFSET			0x0038
#define	PCMN_TXSTART_OFFSET			0x0040
#define	PCMN_TXCNT_OFFSET			0x0048
#define	PCMN_TXADDR_OFFSET			0x0050
#define	PCMN_RXSTART_OFFSET			0x0058
#define	PCMN_RXCNT_OFFSET			0x0060
#define	PCMN_RXADDR_OFFSET			0x0068
#define	PCMN_TXMSK0_OFFSET			0x0080
#define	PCMN_TXMSK1_OFFSET			0x0088
#define	PCMN_TXMSK2_OFFSET			0x0090
#define	PCMN_TXMSK3_OFFSET			0x0098
#define	PCMN_TXMSK4_OFFSET			0x00a0
#define	PCMN_TXMSK5_OFFSET			0x00a8
#define	PCMN_TXMSK6_OFFSET			0x00b0
#define	PCMN_TXMSK7_OFFSET			0x00b8
#define	PCMN_RXMSK0_OFFSET			0x00c0
#define	PCMN_RXMSK1_OFFSET			0x00c8
#define	PCMN_RXMSK2_OFFSET			0x00d0
#define	PCMN_RXMSK3_OFFSET			0x00d8
#define	PCMN_RXMSK4_OFFSET			0x00e0
#define	PCMN_RXMSK5_OFFSET			0x00e8
#define	PCMN_RXMSK6_OFFSET			0x00f0
#define	PCMN_RXMSK7_OFFSET			0x00f8

#endif /* _OCTEON_PCMREG_H_ */
