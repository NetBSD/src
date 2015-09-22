/* $NetBSD: amlogic_board.c,v 1.11.2.4 2015/09/22 12:05:37 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: amlogic_board.c,v 1.11.2.4 2015/09/22 12:05:37 skrll Exp $");

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

#define CBUS_READ(x)		\
	bus_space_read_4(&armv7_generic_bs_tag, amlogic_core_bsh, \
			 AMLOGIC_CBUS_OFFSET + (x))
#define CBUS_WRITE(x, v)	\
	bus_space_write_4(&armv7_generic_bs_tag, amlogic_core_bsh, \
			  AMLOGIC_CBUS_OFFSET + (x), (v))

#define CBUS_SET_CLEAR(x, s, c)	\
	amlogic_reg_set_clear(&armv7_generic_bs_tag, amlogic_core_bsh, \
			      AMLOGIC_CBUS_OFFSET + (x), (s), (c))

void
amlogic_bootstrap(void)
{
	int error;

	error = bus_space_map(&armv7_generic_bs_tag, AMLOGIC_CORE_BASE,
	    AMLOGIC_CORE_SIZE, 0, &amlogic_core_bsh);
	if (error)
		panic("%s: failed to map CORE registers: %d", __func__, error);

	curcpu()->ci_data.cpu_cc_freq = amlogic_get_rate_a9();
}

uint32_t
amlogic_get_rate_xtal(void)
{
	uint32_t ctlreg0;

	ctlreg0 = CBUS_READ(PREG_CTLREG0_ADDR_REG);

	return __SHIFTOUT(ctlreg0, PREG_CTLREG0_ADDR_CLKRATE) * 1000000;
}

uint32_t
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

uint32_t
amlogic_get_rate_clk81(void)
{
	uint32_t cc, rate;

	rate = amlogic_get_rate_fixed();
	cc = CBUS_READ(HHI_MPEG_CLK_CNTL_REG);

	switch (__SHIFTOUT(cc, HHI_MPEG_CLK_CNTL_DIV_SRC)) {
	case 7:
		rate /= 5;
		break;
	case 6:
		rate /= 3;
		break;
	case 5:
		rate /= 4;
		break;
	default:
		panic("CLK81: unknown rate, HHI_MPEG_CLK_CNTL_REG = %#x", cc);
	}

	return rate / (__SHIFTOUT(cc, HHI_MPEG_CLK_CNTL_DIV_N) + 1);
}

uint32_t
amlogic_get_rate_fixed(void)
{
	uint32_t cntl;
	uint64_t clk;
	u_int mul, div, od;

	clk = amlogic_get_rate_xtal();
	cntl = CBUS_READ(HHI_MPLL_CNTL_REG);
	mul = __SHIFTOUT(cntl, HHI_MPLL_CNTL_MUL);
	div = __SHIFTOUT(cntl, HHI_MPLL_CNTL_DIV);
	od = __SHIFTOUT(cntl, HHI_MPLL_CNTL_OD);

	clk *= mul;
	clk /= div;
	clk >>= od;

	return (uint32_t)clk;
}

uint32_t
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

uint32_t
amlogic_get_rate_a9periph(void)
{
	const uint32_t cntl1 = CBUS_READ(HHI_SYS_CPU_CLK_CNTL1_REG);
	const u_int div = __SHIFTOUT(cntl1,
				     HHI_SYS_CPU_CLK_CNTL1_PERIPH_CLK_MUX) + 2;

	return amlogic_get_rate_a9() / div;
}

void
amlogic_eth_init(void)
{
	CBUS_WRITE(EE_CLK_GATING1_REG,
	    CBUS_READ(EE_CLK_GATING1_REG) | EE_CLK_GATING1_ETHERNET);
}

void
amlogic_rng_init(void)
{
	CBUS_SET_CLEAR(EE_CLK_GATING0_REG, EE_CLK_GATING0_RNG, 0);
	CBUS_SET_CLEAR(EE_CLK_GATING3_REG, EE_CLK_GATING3_RNG, 0);
	CBUS_SET_CLEAR(AM_RING_OSC_REG,
	    AM_RING_OSC_ENABLE | AM_RING_OSC_HF_MODE, 0);
}

void
amlogic_sdhc_init(void)
{
	/* enable SDHC clk */
	CBUS_WRITE(EE_CLK_GATING0_REG,
	    CBUS_READ(EE_CLK_GATING0_REG) | EE_CLK_GATING0_SDHC);
}

int
amlogic_sdhc_select_port(int port)
{
	switch (port) {
	case AMLOGIC_SDHC_PORT_B:
		/* Set CARD 0-5 to input */
		CBUS_SET_CLEAR(PAD_PULL_UP_2_REG, 0x03f00000, 0);
		CBUS_SET_CLEAR(PAD_PULL_UP_EN_2_REG, 0x03f00000, 0);
		CBUS_SET_CLEAR(PREG_PAD_GPIO0_EN_N_REG, 0x0fc00000, 0);

		/* CARD -> SDHC pin mux settings */
		CBUS_SET_CLEAR(PERIPHS_PIN_MUX_5_REG, 0, 0x00007c00);
		CBUS_SET_CLEAR(PERIPHS_PIN_MUX_4_REG, 0, 0x7c000000);
		CBUS_SET_CLEAR(PERIPHS_PIN_MUX_2_REG, 0, 0x0000fc00);
		CBUS_SET_CLEAR(PERIPHS_PIN_MUX_8_REG, 0, 0x00000600);
		CBUS_SET_CLEAR(PERIPHS_PIN_MUX_2_REG, 0x000000f0, 0);

		/* XXX ODROID-C1 */
		CBUS_SET_CLEAR(PREG_PAD_GPIO5_EN_N_REG, 0x20000000, 0);
		CBUS_SET_CLEAR(PREG_PAD_GPIO5_OUT_REG, 0, 0x80000000);
		CBUS_SET_CLEAR(PREG_PAD_GPIO5_EN_N_REG, 0, 0x80000000);
		break;
	case AMLOGIC_SDHC_PORT_C:
		/* Set BOOT 0-8,10 to input */
		CBUS_SET_CLEAR(PAD_PULL_UP_2_REG, 0x000005ff, 0);
		CBUS_SET_CLEAR(PAD_PULL_UP_EN_2_REG, 0x000005ff, 0);
		CBUS_SET_CLEAR(PREG_PAD_GPIO3_EN_N_REG, 0x000005ff, 0);

		/* BOOT -> SDHC pin mux settings */
		CBUS_SET_CLEAR(PERIPHS_PIN_MUX_2_REG, 0, 0x04c000f0);
		CBUS_SET_CLEAR(PERIPHS_PIN_MUX_5_REG, 0, 0x00007c00);
		CBUS_SET_CLEAR(PERIPHS_PIN_MUX_6_REG, 0, 0xff000000);
		CBUS_SET_CLEAR(PERIPHS_PIN_MUX_4_REG, 0x70000000, 0);
		CBUS_SET_CLEAR(PERIPHS_PIN_MUX_7_REG, 0x000c0000, 0);

		/* XXX ODROID-C1 */
		CBUS_SET_CLEAR(PAD_PULL_UP_3_REG, 0, 0x000000ff);
		break;
	default:
		return EINVAL;
	}

	return 0;
}

void
amlogic_sdhc_reset_port(int port)
{
	switch (port) {
	case AMLOGIC_SDHC_PORT_C:
		CBUS_SET_CLEAR(PERIPHS_PIN_MUX_2_REG, 0, 0x01000000);
		CBUS_SET_CLEAR(PREG_PAD_GPIO3_EN_N_REG, 0, 0x00000200);
		CBUS_SET_CLEAR(PREG_PAD_GPIO3_OUT_REG, 0x00000200, 0);
		delay(1000);
		CBUS_SET_CLEAR(PREG_PAD_GPIO3_OUT_REG, 0, 0x00000200);
		delay(2000);
		CBUS_SET_CLEAR(PREG_PAD_GPIO3_OUT_REG, 0x00000200, 0);
		delay(1000);
		break;
	}
}

bool
amlogic_sdhc_is_removable(int port)
{
	switch (port) {
	case AMLOGIC_SDHC_PORT_B:
		return true;
	default:
		return false;
	}
}

bool
amlogic_sdhc_is_card_present(int port)
{
	switch (port) {
	case AMLOGIC_SDHC_PORT_B:
		/* GPIO CARD_6 */
		return !(CBUS_READ(PREG_PAD_GPIO0_IN_REG) & __BIT(28));
	default:
		return true;
	}
}

void
amlogic_sdhc_set_voltage(int port, int voltage)
{
	const bus_size_t gpioao_reg = AMLOGIC_GPIOAO_OFFSET;
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh = amlogic_core_bsh;
	uint32_t val;
	u_int pin;

	switch (port) {
	case AMLOGIC_SDHC_PORT_B:
		/* GPIO GPIOAO_3 */
		pin = 3;
		val = bus_space_read_4(bst, bsh, gpioao_reg);
		val &= ~__BIT(pin);	/* OEN */
		bus_space_write_4(bst, bsh, gpioao_reg, val);
		if (voltage == AMLOGIC_SDHC_VOL_180) {
			val |= __BIT(pin + 16);		/* OUT */
		} else {
			val &= ~__BIT(pin + 16);	/* OUT */
		}
		bus_space_write_4(bst, bsh, gpioao_reg, val);
		delay(20000);
		break;
	}
}

void
amlogic_sdio_init(void)
{
	/* enable SDIO clk */
	CBUS_WRITE(EE_CLK_GATING0_REG,
	    CBUS_READ(EE_CLK_GATING0_REG) | EE_CLK_GATING0_SDIO);

	/* reset */
	CBUS_SET_CLEAR(RESET6_REG, RESET6_SDIO, 0);
}

int
amlogic_sdio_select_port(int port)
{
	switch (port) {
	case AMLOGIC_SDIO_PORT_B:
		/* Set CARD 0-5 to input */
		CBUS_SET_CLEAR(PAD_PULL_UP_2_REG, 0x03f00000, 0);
		CBUS_SET_CLEAR(PAD_PULL_UP_EN_2_REG, 0x03f00000, 0);
		CBUS_SET_CLEAR(PREG_PAD_GPIO0_EN_N_REG, 0x0fc00000, 0);

		/* CARD -> SDIO pin mux settings */
		CBUS_SET_CLEAR(PERIPHS_PIN_MUX_6_REG, 0, 0x3f000000);
		CBUS_SET_CLEAR(PERIPHS_PIN_MUX_8_REG, 0, 0x0000063f);
		CBUS_SET_CLEAR(PERIPHS_PIN_MUX_2_REG, 0, 0x000000f0);
		CBUS_SET_CLEAR(PERIPHS_PIN_MUX_2_REG, 0x0000fc00, 0);

		/* XXX ODROID-C1 */
		CBUS_SET_CLEAR(PREG_PAD_GPIO5_EN_N_REG, 0x20000000, 0);
		CBUS_SET_CLEAR(PREG_PAD_GPIO5_OUT_REG, 0, 0x80000000);
		CBUS_SET_CLEAR(PREG_PAD_GPIO5_EN_N_REG, 0, 0x80000000);
		break;
	case AMLOGIC_SDIO_PORT_C:
		delay(100);

		/* Set BOOT 0-8,10 to input */
		CBUS_SET_CLEAR(PAD_PULL_UP_2_REG, 0x0000050f, 0);
		CBUS_SET_CLEAR(PAD_PULL_UP_EN_2_REG, 0x0000050f, 0);
		CBUS_SET_CLEAR(PREG_PAD_GPIO3_EN_N_REG, 0x0000050f, 0);

		/* BOOT -> SDIO pin mux settings */
		CBUS_SET_CLEAR(PERIPHS_PIN_MUX_2_REG, 0, 0x06c2fc00);
		CBUS_SET_CLEAR(PERIPHS_PIN_MUX_8_REG, 0, 0x0000003f);
		CBUS_SET_CLEAR(PERIPHS_PIN_MUX_4_REG, 0, 0x6c000000);
		CBUS_SET_CLEAR(PERIPHS_PIN_MUX_6_REG, 0xfc000000, 0);

		/* XXX ODROID-C1 */
		CBUS_SET_CLEAR(PAD_PULL_UP_3_REG, 0, 0x000000ff);
		break;
	default:
		return EINVAL;
	}

	return 0;
}

static void
amlogic_usbphy_clkgate_enable(int port)
{
	switch (port) {
	case 0:
		CBUS_WRITE(EE_CLK_GATING1_REG,
		    CBUS_READ(EE_CLK_GATING1_REG) |
		    EE_CLK_GATING1_USB_GENERAL |
		    EE_CLK_GATING1_USB0);
		CBUS_WRITE(EE_CLK_GATING2_REG,
		    CBUS_READ(EE_CLK_GATING2_REG) |
		    EE_CLK_GATING2_USB0_TO_DDR);
		break;
	case 1:
		CBUS_WRITE(EE_CLK_GATING1_REG,
		    CBUS_READ(EE_CLK_GATING1_REG) |
		    EE_CLK_GATING1_USB_GENERAL |
		    EE_CLK_GATING1_USB1);
		CBUS_WRITE(EE_CLK_GATING2_REG,
		    CBUS_READ(EE_CLK_GATING2_REG) |
		    EE_CLK_GATING2_USB1_TO_DDR);
		break;
	}
}

void
amlogic_usbphy_init(int port)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh = amlogic_core_bsh;
	bus_size_t ctrl_reg, cfg_reg, adp_bc_reg, gpioao_reg;
	uint32_t ctrl, cfg, adp_bc, gpioao;
	u_int pin, pol;
	bool gpio_power = false, gpio_reset = false, aca_enable = false;

	gpioao_reg = AMLOGIC_GPIOAO_OFFSET;

	switch (port) {
	case 0:
		cfg_reg = PREI_USB_PHY_A_CFG_REG;
		ctrl_reg = PREI_USB_PHY_A_CTRL_REG;
		adp_bc_reg = PREI_USB_PHY_A_ADP_BC_REG;
		pin = 5;
		pol = 1;
		gpio_power = true;
		break;
	case 1:
		cfg_reg = PREI_USB_PHY_B_CFG_REG;
		ctrl_reg = PREI_USB_PHY_B_CTRL_REG;
		adp_bc_reg = PREI_USB_PHY_B_ADP_BC_REG;
		pin = 4;
		pol = 0;
		gpio_reset = true;
		aca_enable = true;
		break;
	default:
		return;
	}

	if (port == 0) {
		CBUS_SET_CLEAR(RESET1_REG, RESET1_USB, 0);
	}

	amlogic_usbphy_clkgate_enable(port);

	if (gpio_power) {
		gpioao = bus_space_read_4(bst, bsh, gpioao_reg);
		gpioao &= ~__BIT(pin);		/* OEN */
		bus_space_write_4(bst, bsh, gpioao_reg, gpioao);
		if (pol) {
			gpioao |= __BIT(pin + 16);	/* OUT */
		} else {
			gpioao &= ~__BIT(pin + 16);	/* OUT */
		}
		bus_space_write_4(bst, bsh, gpioao_reg, gpioao);
	}

	delay(1000);

	cfg = CBUS_READ(cfg_reg);
	cfg |= PREI_USB_PHY_CFG_CLK_32K_ALT_SEL;
	CBUS_WRITE(cfg_reg, cfg);

	ctrl = CBUS_READ(ctrl_reg);
	ctrl &= ~PREI_USB_PHY_CTRL_FSEL;
	ctrl |= __SHIFTIN(PREI_USB_PHY_CTRL_FSEL_24M,
			  PREI_USB_PHY_CTRL_FSEL);
	ctrl |= PREI_USB_PHY_CTRL_POR;
	CBUS_WRITE(ctrl_reg, ctrl);

	delay(1000);

	ctrl = CBUS_READ(ctrl_reg);
	ctrl &= ~PREI_USB_PHY_CTRL_POR;
	CBUS_WRITE(ctrl_reg, ctrl);

	delay(50000);

	ctrl = CBUS_READ(ctrl_reg);

	if ((ctrl & PREI_USB_PHY_CTRL_CLK_DET) == 0)
		printf("WARNING: USB PHY port %d clock not detected\n", port);

	if (aca_enable) {
		adp_bc = CBUS_READ(adp_bc_reg);
		adp_bc |= PREI_USB_PHY_ADP_BC_ACA_ENABLE;
		CBUS_WRITE(adp_bc_reg, adp_bc);

		delay(1000);

		adp_bc = CBUS_READ(adp_bc_reg);
		if (adp_bc & PREI_USB_PHY_ADP_BC_ACA_FLOATING)
			printf("WARNING: USB PHY port %d failed to enable "
			    "ACA detection\n", port);
	}

	if (gpio_reset) {
		/* Reset */
		gpioao = bus_space_read_4(bst, bsh, gpioao_reg);
		gpioao &= ~__BIT(pin);		/* OEN */
		bus_space_write_4(bst, bsh, gpioao_reg, gpioao);
		if (pol) {
			gpioao |= __BIT(pin + 16);	/* OUT */
		} else {
			gpioao &= ~__BIT(pin + 16);	/* OUT */
		}
		bus_space_write_4(bst, bsh, gpioao_reg, gpioao);

		delay(1000);

		if (pol) {
			gpioao &= ~__BIT(pin + 16);	/* OUT */
		} else {
			gpioao |= __BIT(pin + 16);	/* OUT */
		}
		bus_space_write_4(bst, bsh, gpioao_reg, gpioao);

		delay(60000);
	}
}
