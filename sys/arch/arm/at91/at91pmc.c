/*	$Id: at91pmc.c,v 1.4.2.2 2013/01/16 05:32:45 yamt Exp $	*/
/*	$NetBSD: at91pmc.c,v 1.4.2.2 2013/01/16 05:32:45 yamt Exp $	*/

/*
 * Copyright (c) 2007 Embedtronics Oy
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
__KERNEL_RCSID(0, "$NetBSD");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/device.h>

#include <sys/bus.h>
#include <machine/intr.h>

#include <arm/cpufunc.h>

#include <arm/at91/at91reg.h>
#include <arm/at91/at91var.h>
#include <arm/at91/at91pmcreg.h>
#include <arm/at91/at91pmcvar.h>

#define	SLOW_CLOCK	32768LU

void
at91pmc_get_clocks(struct at91bus_clocks *clocks)
{
	uint64_t		mclk, pllaclk, pllbclk, pclk, mstclk;
	uint32_t		reg;

	if (!((reg = PMCREG(PMC_MOR)) & PMC_MOR_MOSCEN))
		panic("%s: main oscillator not enabled (MOR=0x%#X)", __FUNCTION__, reg);

	if (!((reg = PMCREG(PMC_MCFR)) & PMC_MCFR_MAINRDY))
		panic("%s: main oscillator not ready (MCFR=0x%#X)", __FUNCTION__, reg);

	mclk  = ((reg & PMC_MCFR_MAINF) * SLOW_CLOCK) / 16U;

	// try to guess some nice MHz value
	if (((mclk / 1000) % 1000) >= 990) {
	  mclk += 1000000U - (mclk % 1000000U);
	} else if (((mclk / 1000) % 1000) <= 10) {
	  mclk -= (mclk % 1000000U);
	}

	PMCREG(PMC_PLLICPR) = PMC_PLLICPR_ICPPLLA | PMC_PLLICPR_ICPPLLB;

	reg = PMCREG(PMC_PLLAR); pllaclk = 0;
	if (reg & PMC_PLL_DIV) {
		pllaclk = mclk * (((reg & PMC_PLL_MUL) >> PMC_PLL_MUL_SHIFT) + 1);
		pllaclk /= (reg & PMC_PLL_DIV) >> PMC_PLL_DIV_SHIFT;
	}

	reg = PMCREG(PMC_PLLBR); pllbclk = 0;
	if (reg & PMC_PLL_DIV) {
		pllbclk = mclk * (((reg & PMC_PLL_MUL) >> PMC_PLL_MUL_SHIFT) + 1);
		pllbclk /= (reg & PMC_PLL_DIV) >> PMC_PLL_DIV_SHIFT;
		if (reg & PMC_PLLBR_USB_96M) {
			pllbclk /= 2;
		}
	}

	reg = PMCREG(PMC_MCKR);
	switch ((reg & PMC_MCKR_CSS)) {
	case PMC_MCKR_CSS_SLOW_CLK:
	  pclk = SLOW_CLOCK;
	  break;
	default:
	case PMC_MCKR_CSS_MAIN_CLK:
	  pclk = mclk;
	  break;
	case PMC_MCKR_CSS_PLLA:
	  pclk = pllaclk;
	  break;
	case PMC_MCKR_CSS_PLLB:
	  pclk = pllbclk;
	  break;
	}
	pclk >>= (reg & PMC_MCKR_PRES) >> PMC_MCKR_PRES_SHIFT;
	mstclk = pclk / (((reg & PMC_MCKR_MDIV) >> PMC_MCKR_MDIV_SHIFT) + 1);

	clocks->slow = SLOW_CLOCK;
	clocks->main = mclk;
	clocks->cpu = pclk;
	clocks->master = mstclk;
	clocks->plla = pllaclk;
	clocks->pllb = pllbclk;
}


#define	PID_COUNT	32
static int pid_enable_count[PID_COUNT] = {0};

void
at91pmc_peripheral_clock(int pid, int enable)
{
	int s;

	if (pid < 0 || pid >= PID_COUNT)
		panic("%s: pid %d out of range", __FUNCTION__, pid);

	s = splhigh();

	if (enable) {
		pid_enable_count[pid]++;
		PMCREG(PMC_PCER) = (1U << pid);
	} else {
		if (--pid_enable_count[pid] < 0)
			panic("%s: pid %d enable count got negative (%d)",
			      __FUNCTION__, pid, pid_enable_count[pid]);
		if (pid_enable_count[pid] == 0)
			PMCREG(PMC_PCDR) = (1U << pid);
	}

	splx(s);

	if (enable) {
		int c;
		for (c = 0; c < 10000; c++) {
		  __insn_barrier();
		}
	}
}
