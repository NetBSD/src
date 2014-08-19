/* $Id: emi_prep.c,v 1.2.6.3 2014/08/20 00:02:56 tls Exp $ */

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
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/types.h>

#include <arm/imx/imx23_emireg.h>

#include "common.h"

static void init_dram_registers(void);
static uint32_t get_dram_int_status(void);

#define DRAM_REGS 41

uint32_t dram_regs[DRAM_REGS] = {
	0x01010001, 0x00010100, 0x01000101, 0x00000001,
	0x00000101, 0x00000000, 0x00010000, 0x01000001,
	0x00000000, 0x00000001, 0x07000200, 0x00070202,
	0x02020000, 0x04040a01, 0x00000201, 0x02040000,
	0x02000000, 0x19000f08, 0x0d0d0000, 0x02021313,
	0x02061521, 0x0000000a, 0x00080008, 0x00200020,
	0x00200020, 0x00200020, 0x000003f7, 0x00000000,
	0x00000000, 0x00000020, 0x00000020, 0x00c80000,
	0x000a23cd, 0x000000c8, 0x00006665, 0x00000000,
	0x00000101, 0x00040001, 0x00000000, 0x00000000,
	0x00010000
};

/*
 * Initialize DRAM memory.
 */
int
emi_prep(void)
{
	uint32_t tmp_r;

	REG_WR(HW_EMI_CTRL_BASE + HW_EMI_CTRL_CLR, HW_EMI_CTRL_SFTRST);
	delay(10000);

	tmp_r = REG_RD(HW_DRAM_BASE + HW_DRAM_CTL08);
	tmp_r &= ~(HW_DRAM_CTL08_START | HW_DRAM_CTL08_SREFRESH);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL08, tmp_r);

	init_dram_registers();

	/* START */
	tmp_r = REG_RD(HW_DRAM_BASE + HW_DRAM_CTL08);
	tmp_r |= HW_DRAM_CTL08_START;
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL08, tmp_r);

	delay(20000);

	/*
	 * Set memory power-down with memory
	 * clock gating mode (Mode 2).
	 */
	tmp_r = REG_RD(HW_DRAM_BASE + HW_DRAM_CTL16);
	tmp_r |= (1 << 19);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL16, tmp_r);

	tmp_r = REG_RD(HW_DRAM_BASE + HW_DRAM_CTL16);
	tmp_r |= (1<<11);
	REG_WR(HW_DRAM_BASE + HW_DRAM_CTL16, tmp_r);

	/* Wait until DRAM initialization is complete. */
	while(!(get_dram_int_status() & (1<<2)));

	delay(20000);

	return 0;
}
/*
 * Set DRAM register values.
 */
static void
init_dram_registers(void)
{
	volatile uint32_t *dram_r;
	int i;

	dram_r = (uint32_t *)(HW_DRAM_BASE);

	for (i=0; i < DRAM_REGS; i++) {
		/* Skip ctrl register 8, obsolete registers 27 and 28,
		 * read only register 35 */
		if (i == 8 || i == 27 || i == 28 || i == 35)
			continue;
		*(dram_r + i) = dram_regs[i];
	}

	/* Set tRAS lockout on. */
	*(dram_r + 8) |= HW_DRAM_CTL08_TRAS_LOCKOUT;

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
