/* $Id: clock_prep.c,v 1.2.6.2 2013/02/25 00:28:38 tls Exp $ */

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

#include <arm/imx/imx23_clkctrlreg.h>

#include <lib/libsa/stand.h>

#include "common.h"

static void enable_pll(void);
static void enable_ref_cpu(int);
static void enable_ref_emi(int);
static void enable_ref_io(int);
static void use_ref_cpu(void);
static void use_ref_emi(void);
static void use_ref_io(void);
static void set_hbus_div(int);
static void set_emi_div(int);
static void set_ssp_div(int);

/*
 * Clock frequences set by clock_prep()
 */
#define CPU_FRAC	0x13		/* CPUCLK @ 454.74 MHz */
#define HBUS_DIV	0x3		/* AHBCLK @ 151.58 MHz */
#define EMI_FRAC	0x21		/* EMICLK @ 130.91 MHz */
#define EMI_DIV		0x2
#define IO_FRAC		0x12		/* IOCLK  @ 480.00 MHz */
#define SSP_DIV		0x5		/* SSPCLK @  96.00 MHz */

/* Offset to frac register for CLKCTRL_WR_BYTE macro. */
#define HW_CLKCTRL_FRAC_CPU (HW_CLKCTRL_FRAC+0)
#define HW_CLKCTRL_FRAC_EMI (HW_CLKCTRL_FRAC+1)
#define HW_CLKCTRL_FRAC_IO (HW_CLKCTRL_FRAC+3)

#define CLKCTRL_RD(reg) *(volatile uint32_t *)((HW_CLKCTRL_BASE) + (reg))
#define CLKCTRL_WR(reg, val)						\
do {									\
	*(volatile uint32_t *)((HW_CLKCTRL_BASE) + (reg)) = val;	\
} while (0)
#define CLKCTRL_WR_BYTE(reg, val)					\
do {									\
	*(volatile uint8_t *)((HW_CLKCTRL_BASE) + (reg)) = val;	\
} while (0)

/*
 * Initializes fast PLL based clocks for CPU, HBUS and DRAM.
 */
int
clock_prep(void)
{

	enable_pll();
	enable_ref_cpu(CPU_FRAC);
	enable_ref_emi(EMI_FRAC);
	enable_ref_io(IO_FRAC);
	set_emi_div(EMI_DIV);
	set_hbus_div(HBUS_DIV);
	use_ref_cpu();
	use_ref_emi();
	use_ref_io();
	set_ssp_div(SSP_DIV);

	return 0;
}

/*
 * Turn PLL on and wait until it's locked to 480 MHz.
 */
static void
enable_pll(void)
{

	CLKCTRL_WR(HW_CLKCTRL_PLLCTRL0_SET, HW_CLKCTRL_PLLCTRL0_POWER);
	while (!(CLKCTRL_RD(HW_CLKCTRL_PLLCTRL1) & HW_CLKCTRL_PLLCTRL1_LOCK));

	return;
}

/*
 * Enable fractional divider clock ref_cpu with divide value "frac".
 */
static void
enable_ref_cpu(int frac)
{
	uint32_t reg;

	reg = CLKCTRL_RD(HW_CLKCTRL_FRAC);
	reg &= ~(HW_CLKCTRL_FRAC_CLKGATECPU | HW_CLKCTRL_FRAC_CPUFRAC);
	reg |= __SHIFTIN(frac, HW_CLKCTRL_FRAC_CPUFRAC);
	CLKCTRL_WR_BYTE(HW_CLKCTRL_FRAC_CPU, reg);

	return;
}

/*
 * Enable fractional divider clock ref_emi with divide value "frac".
 */
static void
enable_ref_emi(int frac)
{
	uint32_t reg;

	reg = CLKCTRL_RD(HW_CLKCTRL_FRAC);
	reg &= ~(HW_CLKCTRL_FRAC_CLKGATEEMI | HW_CLKCTRL_FRAC_EMIFRAC);
	reg |= __SHIFTIN(frac, HW_CLKCTRL_FRAC_EMIFRAC);
	CLKCTRL_WR_BYTE(HW_CLKCTRL_FRAC_EMI, (reg >> 8));

	return;
}

/*
 * Enable fractional divider clock ref_io with divide value "frac".
 */
static void
enable_ref_io(int frac)
{
	uint32_t reg;

	reg = CLKCTRL_RD(HW_CLKCTRL_FRAC);
	reg &= ~(HW_CLKCTRL_FRAC_CLKGATEIO | HW_CLKCTRL_FRAC_IOFRAC);
	reg |= __SHIFTIN(frac, HW_CLKCTRL_FRAC_IOFRAC);
	CLKCTRL_WR_BYTE(HW_CLKCTRL_FRAC_IO, (reg >> 24));

	return;
}

/*
 * Divide CLK_P by "div" to get CLK_H frequency.
 */
static void
set_hbus_div(int div)
{
	uint32_t reg;

	reg = CLKCTRL_RD(HW_CLKCTRL_HBUS);
	reg &= ~(HW_CLKCTRL_HBUS_DIV);
	reg |= __SHIFTIN(div, HW_CLKCTRL_HBUS_DIV);
	while (CLKCTRL_RD(HW_CLKCTRL_HBUS) & HW_CLKCTRL_HBUS_BUSY);
	CLKCTRL_WR(HW_CLKCTRL_HBUS, reg);

	return;
}

/*
 * ref_emi is divied "div" to get CLK_EMI.
 */
static void
set_emi_div(int div)
{
	uint32_t reg;

	reg = CLKCTRL_RD(HW_CLKCTRL_EMI);
	reg &= ~(HW_CLKCTRL_EMI_DIV_EMI);
	reg |= __SHIFTIN(div, HW_CLKCTRL_EMI_DIV_EMI);
	CLKCTRL_WR(HW_CLKCTRL_EMI, reg);

	return;
}

/*
 * ref_io is divied "div" to get CLK_SSP.
 */
static void
set_ssp_div(int div)
{
	uint32_t reg;

	reg = CLKCTRL_RD(HW_CLKCTRL_SSP);
	reg &= ~(HW_CLKCTRL_SSP_DIV);
	reg |= __SHIFTIN(div, HW_CLKCTRL_SSP_DIV);
	CLKCTRL_WR(HW_CLKCTRL_SSP, reg);

	return;
}

/*
 * Transition from ref_xtal to use ref_cpu.
 */
static void
use_ref_cpu(void)
{
	CLKCTRL_WR(HW_CLKCTRL_CLKSEQ_CLR, HW_CLKCTRL_CLKSEQ_BYPASS_CPU);
	return;
}

/*
 * Transition from ref_xtal to use ref_emi and source CLK_EMI from ref_emi.
 */
static void
use_ref_emi(void)
{
	uint32_t reg;

	/* Enable ref_emi. */
	CLKCTRL_WR(HW_CLKCTRL_CLKSEQ_CLR, HW_CLKCTRL_CLKSEQ_BYPASS_EMI);

	/* CLK_EMI sourced by the ref_emi. */
	reg = CLKCTRL_RD(HW_CLKCTRL_EMI);
	reg &= ~(HW_CLKCTRL_EMI_CLKGATE);
	CLKCTRL_WR(HW_CLKCTRL_EMI, reg);

	return;
}

/*
 * Transition from ref_xtal to use ref_io and source CLK_SSP from ref_io.
 */
static void
use_ref_io(void)
{
	uint32_t reg;

	/* Enable ref_io for SSP. */
	CLKCTRL_WR(HW_CLKCTRL_CLKSEQ_CLR, HW_CLKCTRL_CLKSEQ_BYPASS_SSP);

	/* CLK_SSP sourced by the ref_io. */
	reg = CLKCTRL_RD(HW_CLKCTRL_SSP);
	reg &= ~(HW_CLKCTRL_SSP_CLKGATE);
	CLKCTRL_WR(HW_CLKCTRL_SSP, reg);

	return;
}
