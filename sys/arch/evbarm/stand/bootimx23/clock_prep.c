/* $Id: clock_prep.c,v 1.2.6.3 2014/08/20 00:02:56 tls Exp $ */

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

#include <arm/imx/imx23_clkctrlreg.h>

#include <lib/libsa/stand.h>

#include "common.h"

#define CLKCTRL_HBUS	(HW_CLKCTRL_BASE + HW_CLKCTRL_HBUS)
#define CLKCTRL_PLL0	(HW_CLKCTRL_BASE + HW_CLKCTRL_PLLCTRL0)
#define CLKCTRL_PLL0_S	(HW_CLKCTRL_BASE + HW_CLKCTRL_PLLCTRL0_SET)
#define CLKCTRL_PLL0_C	(HW_CLKCTRL_BASE + HW_CLKCTRL_PLLCTRL0_CLR)
#define CLKCTRL_PLL1	(HW_CLKCTRL_BASE + HW_CLKCTRL_PLLCTRL1)
#define CLKCTRL_FRAC	(HW_CLKCTRL_BASE + HW_CLKCTRL_FRAC)
#define CLKCTRL_FRAC_S	(HW_CLKCTRL_BASE + HW_CLKCTRL_FRAC_SET)
#define CLKCTRL_FRAC_C	(HW_CLKCTRL_BASE + HW_CLKCTRL_FRAC_CLR)
#define CLKCTRL_SEQ	(HW_CLKCTRL_BASE + HW_CLKCTRL_CLKSEQ)
#define CLKCTRL_SEQ_S	(HW_CLKCTRL_BASE + HW_CLKCTRL_CLKSEQ_SET)
#define CLKCTRL_SEQ_C	(HW_CLKCTRL_BASE + HW_CLKCTRL_CLKSEQ_CLR)
#define CLKCTRL_EMI	(HW_CLKCTRL_BASE + HW_CLKCTRL_EMI)
#define CLKCTRL_SSP	(HW_CLKCTRL_BASE + HW_CLKCTRL_SSP)

void en_pll(void);
void set_hbus_div(unsigned int);
void set_cpu_frac(unsigned int);
void bypass_cpu(void);
void set_emi_div(unsigned int);
void set_emi_frac(unsigned int);
void bypass_emi(void);

/*
 * Power on 480 MHz PLL.
 */
void
en_pll(void)
{

	REG_WR(CLKCTRL_PLL0_S, HW_CLKCTRL_PLLCTRL0_POWER);
	while(!(REG_RD(CLKCTRL_PLL1) & HW_CLKCTRL_PLLCTRL1))
		;

	return;
}
void
bypass_cpu(void)
{

	REG_WR(CLKCTRL_SEQ_C, HW_CLKCTRL_CLKSEQ_BYPASS_CPU);

	return;
}
void
bypass_emi(void)
{
	REG_WR(CLKCTRL_SEQ_C, HW_CLKCTRL_CLKSEQ_BYPASS_EMI);

	return;
}
void
bypass_ssp(void)
{
	REG_WR(CLKCTRL_SEQ_C, HW_CLKCTRL_CLKSEQ_BYPASS_SSP);

	return;
}
void
bypass_saif(void)
{
	REG_WR(CLKCTRL_SEQ_C, HW_CLKCTRL_CLKSEQ_BYPASS_SAIF);
	
	return;
}
/*
 * Set HBUS divider value.
 */
void
set_hbus_div(unsigned int div)
{
	uint32_t tmp_r;

	while (REG_RD(CLKCTRL_HBUS) & HW_CLKCTRL_HBUS_BUSY)
		;

	tmp_r = REG_RD(CLKCTRL_HBUS);
	tmp_r &= ~HW_CLKCTRL_HBUS_DIV;
	tmp_r |= __SHIFTIN(div, HW_CLKCTRL_HBUS_DIV);
	REG_WR(CLKCTRL_HBUS, tmp_r);

	while (REG_RD(CLKCTRL_HBUS) & HW_CLKCTRL_HBUS_BUSY)
		;

	return;
}
/*
 * Set CPU frac.
 */
void
set_cpu_frac(unsigned int frac)
{
	uint8_t tmp_r;

	tmp_r = REG_RD_BYTE(CLKCTRL_FRAC);
	tmp_r &= ~(HW_CLKCTRL_FRAC_CLKGATECPU | HW_CLKCTRL_FRAC_CPUFRAC);
	tmp_r |= __SHIFTIN(frac, HW_CLKCTRL_FRAC_CPUFRAC);
	REG_WR_BYTE(CLKCTRL_FRAC, tmp_r);

	return;
}
/*
 * Set EMI frac.
 */
void
set_emi_frac(unsigned int frac)
{
	uint8_t *emi_frac;
	uint16_t tmp_r;

	emi_frac = (uint8_t *)(CLKCTRL_FRAC);
	emi_frac++;
	
	tmp_r = *emi_frac<<8;
	tmp_r &= ~(HW_CLKCTRL_FRAC_CLKGATEEMI | HW_CLKCTRL_FRAC_EMIFRAC);
	tmp_r |= __SHIFTIN(frac, HW_CLKCTRL_FRAC_EMIFRAC);

	*emi_frac = tmp_r>>8;

	return;
}
/*
 * Set EMU divider value.
 */
void
set_emi_div(unsigned int div)
{
	uint32_t tmp_r;

	while (REG_RD(CLKCTRL_EMI) &
		(HW_CLKCTRL_EMI_BUSY_REF_XTAL | HW_CLKCTRL_EMI_BUSY_REF_EMI))
		;

	tmp_r = REG_RD(CLKCTRL_EMI);
	tmp_r &= ~(HW_CLKCTRL_EMI_CLKGATE | HW_CLKCTRL_EMI_DIV_EMI);
	tmp_r |= __SHIFTIN(div, HW_CLKCTRL_EMI_DIV_EMI);
	REG_WR(CLKCTRL_EMI, tmp_r);
	
	return;
}
/*
 * Set SSP divider value.
 */
void
set_ssp_div(unsigned int div)
{
	uint32_t tmp_r;

	tmp_r = REG_RD(CLKCTRL_SSP);
	tmp_r &= ~HW_CLKCTRL_SSP_CLKGATE;
	REG_WR(CLKCTRL_SSP, tmp_r);

	while (REG_RD(CLKCTRL_SSP) & HW_CLKCTRL_SSP_BUSY)
		;

	tmp_r = REG_RD(CLKCTRL_SSP);
	tmp_r &= ~HW_CLKCTRL_SSP_DIV;
	tmp_r |= __SHIFTIN(div, HW_CLKCTRL_SSP_DIV);
	REG_WR(CLKCTRL_SSP, tmp_r);

	while (REG_RD(CLKCTRL_SSP) & HW_CLKCTRL_SSP_BUSY)
		;

	return;
}
/*
 * Set IO frac.
 */
void
set_io_frac(unsigned int frac)
{
	uint8_t *io_frac;
	uint32_t tmp_r;
	
	io_frac = (uint8_t *)(CLKCTRL_FRAC);
	io_frac++; /* emi */
	io_frac++; /* pix */
	io_frac++; /* io */
	tmp_r = (*io_frac)<<24;
	tmp_r &= ~(HW_CLKCTRL_FRAC_CLKGATEIO | HW_CLKCTRL_FRAC_IOFRAC);
	tmp_r |= __SHIFTIN(frac, HW_CLKCTRL_FRAC_IOFRAC);

	*io_frac = (uint8_t)(tmp_r>>24);

	return;
}
