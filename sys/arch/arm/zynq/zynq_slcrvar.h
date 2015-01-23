/*	$NetBSD: zynq_slcrvar.h,v 1.1 2015/01/23 12:34:09 hkenken Exp $	*/
/*-
 * Copyright (c) 2015  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_ARM_ZYNQ_ZYNQ_SLCRVAR_H
#define	_ARM_ZYNQ_ZYNQ_SLCRVAR_H

enum zynq_clocks {
	CLK_PS,
	CLK_ARM_PLL,
	CLK_DDR_PLL,
	CLK_IO_PLL,
	CLK_CPU_6X4X,
	CLK_CPU_3X2X,
	CLK_CPU_2X,
	CLK_CPU_1X,
	CLK_DDR_3X,
	CLK_DDR_2X,
	CLK_DDR_DCI,
	CLK_SMC,
	CLK_QSPI,
	CLK_GIGE0,
	CLK_GIGE1,
	CLK_SDIO,
	CLK_UART,
	CLK_SPI,
	CLK_CAN,
	CLK_PCAP,
	CLK_DBG,
	CLK_FCLK0,
	CLK_FCLK1,
	CLK_FCLK2,
	CLK_FCLK3,
	CLK_MAX
};

uint32_t zynq_get_clock(enum zynq_clocks);

#endif /* _ARM_ZYNQ_ZYNQ_SLCRVAR_H */
