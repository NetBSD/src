/* $NetBSD: amlogic_board.c,v 1.2 2015/02/27 19:57:10 jmcneill Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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

#include "opt_amlogic.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: amlogic_board.c,v 1.2 2015/02/27 19:57:10 jmcneill Exp $");

#define	_ARM32_BUS_DMA_PRIVATE
#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <arm/bootconfig.h>
#include <arm/cpufunc.h>

#include <arm/amlogic/amlogic_reg.h>
#include <arm/amlogic/amlogic_crureg.h>
#include <arm/amlogic/amlogic_var.h>

bus_space_handle_t amlogic_core_bsh;

struct arm32_bus_dma_tag amlogic_dma_tag = {
	_BUS_DMAMAP_FUNCS,
	_BUS_DMAMEM_FUNCS,
	_BUS_DMATAG_FUNCS,
};

#define CBUS_READ(x)	\
	bus_space_read_4(&amlogic_bs_tag, amlogic_core_bsh, (x))

static uint32_t	amlogic_get_rate_xtal(void);
static uint32_t	amlogic_get_rate_sys(void);
static uint32_t	amlogic_get_rate_a9(void);


void
amlogic_bootstrap(void)
{
	int error;

	error = bus_space_map(&amlogic_bs_tag, AMLOGIC_CORE_BASE,
	    AMLOGIC_CORE_SIZE, 0, &amlogic_core_bsh);
	if (error)
		panic("%s: failed to map CORE registers: %d", __func__, error);

	curcpu()->ci_data.cpu_cc_freq = amlogic_get_rate_a9();
}

static uint32_t
amlogic_get_rate_xtal(void)
{
	uint32_t ctlreg0;
	
	ctlreg0 = CBUS_READ(PREG_CTLREG0_ADDR_REG);

	return __SHIFTOUT(ctlreg0, PREG_CTLREG0_ADDR_CLKRATE) * 1000000;
}

static uint32_t
amlogic_get_rate_sys(void)
{
	uint32_t cntl;
	uint64_t clk;
	u_int mul, div, od;

	clk = amlogic_get_rate_xtal();
	cntl = CBUS_READ(HHI_SYS_PLL_CNTL_REG);
	mul = __SHIFTOUT(cntl, HHI_SYS_PLL_CNTL_MUL);
	div = __SHIFTOUT(cntl, HHI_SYS_PLL_CNTL_DIV);
	od = __SHIFTOUT(cntl, HHI_SYS_PLL_CNTL_OD);

	clk *= mul;
	clk /= div;
	clk >>= od;

	return (uint32_t)clk;
}

static uint32_t
amlogic_get_rate_a9(void)
{
	uint32_t cntl0, cntl1;
	uint32_t rate = 0;

	cntl0 = CBUS_READ(HHI_SYS_CPU_CLK_CNTL0_REG);
	if (cntl0 & HHI_SYS_CPU_CLK_CNTL0_CLKSEL) {
		switch (__SHIFTOUT(cntl0, HHI_SYS_CPU_CLK_CNTL0_PLLSEL)) {
		case 0:
			rate = amlogic_get_rate_xtal();
			break;
		case 1:
			rate = amlogic_get_rate_sys();
			break;
		case 2:
			rate = 1250000000;
			break;
		}
	} else {
		rate = amlogic_get_rate_xtal();
	}

	KASSERTMSG(rate != 0, "couldn't determine A9 rate");

	switch (__SHIFTOUT(cntl0, HHI_SYS_CPU_CLK_CNTL0_SOUTSEL)) {
	case 0:
		break;
	case 1:
		rate /= 2;
		break;
	case 2:
		rate /= 3;
		break;
	case 3:
		cntl1 = CBUS_READ(HHI_SYS_CPU_CLK_CNTL1_REG);
		rate /= (__SHIFTOUT(cntl1, HHI_SYS_CPU_CLK_CNTL1_SDIV) + 1);
		break;
	}

	return rate;
}
