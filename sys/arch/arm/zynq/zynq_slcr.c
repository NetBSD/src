/*	$NetBSD: zynq_slcr.c,v 1.1.20.2 2017/12/03 11:35:57 jdolecek Exp $	*/
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zynq_slcr.c,v 1.1.20.2 2017/12/03 11:35:57 jdolecek Exp $");

#include "opt_zynq.h"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/param.h>

#include <machine/cpu.h>

#include <arm/zynq/zynq_slcrvar.h>
#include <arm/zynq/zynq_slcrreg.h>

#include <arm/zynq/zynq7000_var.h>
#include <arm/zynq/zynq7000_reg.h>

#include "locators.h"

#define	ZYNQSLCRDEBUG

struct zynqslcr_softc {
	device_t	sc_dev;
	bus_space_tag_t	sc_iot;
	bus_space_handle_t	sc_ioh;

	struct zynq7000_clock_info *sc_clk_info;
};

struct zynqslcr_softc *slcr_softc;

static int zynqslcr_match(device_t, cfdata_t, void *);
static void zynqslcr_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(zynqslcr, sizeof(struct zynqslcr_softc),
    zynqslcr_match, zynqslcr_attach, NULL, NULL);

#ifdef ZYNQSLCRDEBUG
static void
zynqslcr_dumpclocks(struct zynqslcr_softc * const sc)
{
	enum zynq_clocks clk;
	static const char *str[CLK_MAX] = {
		"REF     ",
		"ARM_PLL ",
		"DDR_PLL ",
		"IO_PLL  ",
		"CPU_6X4X",
		"CPU_3X2X",
		"CPU_2X  ",
		"CPU_1X  ",
		"DDR_3X  ",
		"DDR_2X  ",
		"DDR_DCI ",
		"SMC     ",
		"QSPI    ",
		"GIGE0   ",
		"GIGE1   ",
		"SDIO    ",
		"UART    ",
		"SPI     ",
		"CAN     ",
		"PCAP    ",
		"DBG     ",
		"FCLK0   ",
		"FCLK1   ",
		"FCLK2   ",
		"FCLK3   ",
	};
	printf("%s\n", __func__);
	for (clk = CLK_PS; clk < CLK_MAX; clk++)
		printf("%s : %4d MHz\n", str[clk], (zynq_get_clock(clk) + 500000) / 1000000);
}
#endif

static int
zynqslcr_match(device_t parent, cfdata_t cfdata, void *aux)
{
	struct axi_attach_args *aa = aux;

	if (slcr_softc != NULL)
		return 0;

	if (aa->aa_addr == SLCR_BASE)
		return 1;

	return 0;
}

static void
zynqslcr_attach(device_t parent, device_t self, void *aux)
{
	struct zynqslcr_softc * const sc = device_private(self);
	struct axi_attach_args *aa = aux;
	bus_space_tag_t iot = aa->aa_iot;

	slcr_softc = sc;
	sc->sc_dev = self;
	sc->sc_iot = iot;

	if (bus_space_map(iot, aa->aa_addr, SLCR_SIZE, 0, &sc->sc_ioh)) {
		aprint_error(": can't map registers\n");
		return;
	}

	aprint_normal(": System Level Control Module\n");
	aprint_naive("\n");

	sc->sc_clk_info = &clk_info;

#ifdef ZYNQSLCRDEBUG
	zynqslcr_dumpclocks(sc);
#endif
}

static uint32_t
get_clock_value(struct zynqslcr_softc *sc, enum zynq_clocks base, uint32_t addr)
{
	uint32_t reg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, addr);
	int div0 = __SHIFTOUT(reg, CLK_DIVISOR0);
	int div1 = __SHIFTOUT(reg, CLK_DIVISOR1);
	uint32_t val = zynq_get_clock(base);

	if (div0 > 0)
	        val /= div0;
	if (div1 > 0)
	        val /= div1;

	return val;
}

uint32_t
zynq_get_clock(enum zynq_clocks name)
{
	struct zynqslcr_softc *sc = slcr_softc;
	struct zynq7000_clock_info *clk = sc->sc_clk_info;
	uint32_t reg;

	if (slcr_softc == NULL)
		return -1;

	switch (name) {
	case CLK_PS:
		return clk->clk_ps;
	case CLK_ARM_PLL:
		if (clk->clk_arm_base == 0) {
			reg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SLCR_ARM_PLL_CTRL);
			clk->clk_arm_base = __SHIFTOUT(reg, PLL_FDIV) * clk->clk_ps;
		}

		return clk->clk_arm_base;
	case CLK_DDR_PLL:
		if (clk->clk_ddr_base == 0) {
			reg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SLCR_DDR_PLL_CTRL);
			clk->clk_ddr_base = __SHIFTOUT(reg, PLL_FDIV) * clk->clk_ps;
		}

		return clk->clk_ddr_base;
	case CLK_IO_PLL:
		if (clk->clk_io_base == 0) {
			reg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SLCR_IO_PLL_CTRL);
			clk->clk_io_base = __SHIFTOUT(reg, PLL_FDIV) * clk->clk_ps;
		}

		return clk->clk_io_base;
	case CLK_CPU_6X4X:
		if (clk->clk_cpu_6x4x == 0) {
			reg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SLCR_ARM_CLK_CTRL);
			clk->clk_cpu_6x4x = zynq_get_clock(CLK_ARM_PLL) / __SHIFTOUT(reg, CLK_DIVISOR0);
		}

		return clk->clk_cpu_6x4x;
	case CLK_CPU_3X2X:
		if (clk->clk_cpu_3x2x == 0)
			clk->clk_cpu_3x2x = zynq_get_clock(CLK_CPU_6X4X) / 2;

		return clk->clk_cpu_3x2x;
	case CLK_CPU_2X:
		if (clk->clk_cpu_2x == 0)
			clk->clk_cpu_2x = zynq_get_clock(CLK_CPU_6X4X) / 3;

		return clk->clk_cpu_2x;
	case CLK_CPU_1X:
		if (clk->clk_cpu_1x == 0)
			clk->clk_cpu_1x = zynq_get_clock(CLK_CPU_6X4X) / 6;

		return clk->clk_cpu_1x;
	case CLK_DDR_3X:
		if (clk->clk_ddr_3x == 0) {
			reg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SLCR_DDR_CLK_CTRL);
			clk->clk_ddr_3x = zynq_get_clock(CLK_DDR_PLL) / __SHIFTOUT(reg, CLK_DDR_3XCLK_DIVISO);
		}

		return clk->clk_ddr_3x;
	case CLK_DDR_2X:
		if (clk->clk_ddr_2x == 0) {
			reg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SLCR_DDR_CLK_CTRL);
			clk->clk_ddr_2x = zynq_get_clock(CLK_DDR_PLL) / __SHIFTOUT(reg, CLK_DDR_2XCLK_DIVISO);
		}

		return clk->clk_ddr_2x;
	case CLK_DDR_DCI:
		if (clk->clk_ddr_dci == 0)
			clk->clk_ddr_dci = get_clock_value(sc, CLK_DDR_PLL, SLCR_DCI_CLK_CTRL);

		return clk->clk_ddr_dci;
	case CLK_SMC:
		if (clk->clk_smc == 0)
			clk->clk_smc = get_clock_value(sc, CLK_IO_PLL, SLCR_SMC_CLK_CTRL);

		return clk->clk_smc;
	case CLK_QSPI:
		if (clk->clk_qspi == 0)
			clk->clk_qspi = get_clock_value(sc, CLK_IO_PLL, SLCR_LQSPI_CLK_CTRL);

		return clk->clk_qspi;
	case CLK_GIGE0:
		if (clk->clk_gige0 == 0)
			clk->clk_gige0 = get_clock_value(sc, CLK_IO_PLL, SLCR_GEM0_CLK_CTRL);

		return clk->clk_gige0;
	case CLK_GIGE1:
		if (clk->clk_gige1 == 0)
			clk->clk_gige1 = get_clock_value(sc, CLK_IO_PLL, SLCR_GEM1_CLK_CTRL);

		return clk->clk_gige1;
	case CLK_SDIO:
		if (clk->clk_sdio == 0)
			clk->clk_sdio = get_clock_value(sc, CLK_IO_PLL, SLCR_SDIO_CLK_CTRL);

		return clk->clk_sdio;
	case CLK_UART:
		if (clk->clk_uart == 0)
			clk->clk_uart = get_clock_value(sc, CLK_IO_PLL, SLCR_UART_CLK_CTRL);

		return clk->clk_uart;
	case CLK_SPI:
		if (clk->clk_spi == 0)
			clk->clk_spi = get_clock_value(sc, CLK_IO_PLL, SLCR_SPI_CLK_CTRL);

		return clk->clk_spi;
	case CLK_CAN:
		if (clk->clk_can == 0)
			clk->clk_can = get_clock_value(sc, CLK_IO_PLL, SLCR_CAN_CLK_CTRL);

		return clk->clk_can;
	case CLK_PCAP:
		if (clk->clk_pcap == 0)
			clk->clk_pcap = get_clock_value(sc, CLK_IO_PLL, SLCR_PCAP_CLK_CTRL);

		return clk->clk_pcap;
	case CLK_DBG:
		if (clk->clk_dbg == 0)
			clk->clk_dbg = get_clock_value(sc, CLK_IO_PLL, SLCR_DBG_CLK_CTRL);

		return clk->clk_dbg;
	case CLK_FCLK0:
		if (clk->clk_fclk0 == 0)
			clk->clk_fclk0 = get_clock_value(sc, CLK_IO_PLL, SLCR_FPGA0_CLK_CTRL);

		return clk->clk_fclk0;
	case CLK_FCLK1:
		if (clk->clk_fclk1 == 0)
			clk->clk_fclk1 = get_clock_value(sc, CLK_IO_PLL, SLCR_FPGA1_CLK_CTRL);

		return clk->clk_fclk1;
	case CLK_FCLK2:
		if (clk->clk_fclk2 == 0)
			clk->clk_fclk2 = get_clock_value(sc, CLK_IO_PLL, SLCR_FPGA2_CLK_CTRL);

		return clk->clk_fclk2;
	case CLK_FCLK3:
		if (clk->clk_fclk3 == 0)
			clk->clk_fclk3 = get_clock_value(sc, CLK_IO_PLL, SLCR_FPGA3_CLK_CTRL);

		return clk->clk_fclk3;
	default:
		return -1;
	}
}
