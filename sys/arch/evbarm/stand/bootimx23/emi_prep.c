/* $Id: emi_prep.c,v 1.2.6.2 2013/02/25 00:28:38 tls Exp $ */

/*
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Petri Laakso.
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

#include <sys/param.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <arm/imx/imx23_emireg.h>

#include <lib/libsa/stand.h>

#include "common.h"

static void init_dram_registers(void);
static void start_dram(void);
static uint32_t get_dram_int_status(void);

/*
 * Initialize external DRAM memory.
 */
int
emi_prep(void)
{

	init_dram_registers();
	start_dram();

	return 0;
}

/*
 * DRAM register values for 32Mx16 hy5du121622dtp-d43 DDR module.
 *
 * Register values were copied from Freescale's imx-bootlets-src-10.05.02
 * source code, init_ddr_mt46v32m16_133Mhz() function. Only change to those
 * settings were to HW_DRAM_CTL19_DQS_OUT_SHIFT which was set to 16 as a result
 * from trial and error.
 */
static void
init_dram_registers(void)
{
	uint32_t reg;

	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL00, 0x01010001);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL01, 0x00010100);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL02, 0x01000101);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL03, 0x00000001);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL04, 0x00000101);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL05, 0x00000000);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL06, 0x00010000);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL07, 0x01000001);
	// HW_DRAM_CTL08 initialized last.
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL09, 0x00000001);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL10, 0x07000200);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL11, 0x00070202);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL12, 0x02020000);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL13, 0x04040a01);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL14, 0x00000201);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL15, 0x02040000);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL16, 0x02000000);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL17, 0x19000f08);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL18, 0x0d0d0000);
//	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL19, 0x02021313);
	reg = __SHIFTIN(2, HW_DRAM_CTL19_DQS_OUT_SHIFT_BYPASS) |
	    __SHIFTIN(16, HW_DRAM_CTL19_DQS_OUT_SHIFT) |
	    __SHIFTIN(19, HW_DRAM_CTL19_DLL_DQS_DELAY_BYPASS_1) |
	    __SHIFTIN(19, HW_DRAM_CTL19_DLL_DQS_DELAY_BYPASS_0);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL19, reg);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL20, 0x02061521);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL21, 0x0000000a);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL22, 0x00080008);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL23, 0x00200020);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL24, 0x00200020);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL25, 0x00200020);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL26, 0x000003f7);
	// HW_DRAM_CTL27 
	// HW_DRAM_CTL28 
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL29, 0x00000020);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL30, 0x00000020);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL31, 0x00c80000);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL32, 0x000a23cd);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL33, 0x000000c8);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL34, 0x00006665);
	// HW_DRAM_CTL35 is read only register
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL36, 0x00000101);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL37, 0x00040001);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL38, 0x00000000);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL39, 0x00000000);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL40, 0x00010000);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL08, 0x01000000);

	return;
}

/*
 * Start DRAM module. After return DRAM is ready to use.
 */
static void
start_dram(void)
{
	uint32_t reg;

	reg = REG_RD(HW_DRAM_BASE + HW_DRAM_CTL08);
	reg |= HW_DRAM_CTL08_START;
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL08, reg);

	/* Wait until DRAM initialization is complete. */
	while(!(get_dram_int_status() & (1<<2)));

	return;
}

/*
 * Return DRAM controller interrupt status register.
 */
static uint32_t
get_dram_int_status(void)
{
	uint32_t reg;

	reg = REG_RD(HW_DRAM_BASE + HW_DRAM_CTL18);
	return __SHIFTOUT(reg, HW_DRAM_CTL18_INT_STATUS);
}
