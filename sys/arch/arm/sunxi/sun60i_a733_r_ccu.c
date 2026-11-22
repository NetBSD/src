/* $NetBSD: sun60i_a733_r_ccu.c,v 1.1 2026/11/22 14:29:22 jmcneill Exp $ */

/*-
 * Copyright (c) 2026 Jared McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: sun60i_a733_r_ccu.c,v 1.1 2026/11/22 14:29:22 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_ccu.h>
#include <arm/sunxi/sun60i_a733_r_ccu.h>

#define	AHBS_CLK_REG			0x000
#define	APBS0_CLK_REG			0x00c
#define	APBS1_CLK_REG			0x010
#define	S_TIMER0_CLK_REG		0x100
#define	S_TIMER1_CLK_REG		0x104
#define	S_TIMER2_CLK_REG		0x108
#define	S_TIMER3_CLK_REG		0x10c
#define	S_TIMER_BGR_REG			0x11c
#define	S_TWD_BGR_REG			0x12c
#define	S_PWM_CLK_REG			0x130
#define	S_PWM_BGR_REG			0x13c
#define	S_SPI_CLK_REG			0x150
#define	S_SPI_BGR_REG			0x15c
#define	S_MBOX_BGR_REG			0x17c
#define	S_UART_BGR_REG			0x18c
#define	S_TWI_BGR_REG			0x19c
#define	S_PPU_BGR_REG			0x1ac
#define	S_TZMA_BGR_REG			0x1b0
#define	S_CPUS_BIST_BGR_REG		0x1bc
#define	S_IRRX_CLK_REG			0x1c0
#define	S_IRRX_BGR_REG			0x1cc
#define	RTC_BGR_REG			0x20c
#define	RISCV_24M_CLK_REG		0x210
#define	RISCV_BGR_REG			0x21c
#define	S_CPUCFG_BGR_REG		0x22c
#define	DEV_BUS_AUTOG_CTRL_REG		0x338
#define	BUS_ACG_REG			0x33c

static int sun60i_a733_r_ccu_match(device_t, cfdata_t, void *);
static void sun60i_a733_r_ccu_attach(device_t, device_t, void *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "allwinner,sun60iw2-r-ccu" },
	DEVICE_COMPAT_EOL
};

CFATTACH_DECL_NEW(sunxi_a733_r_ccu, sizeof(struct sunxi_ccu_softc),
	sun60i_a733_r_ccu_match, sun60i_a733_r_ccu_attach, NULL, NULL);

static struct sunxi_ccu_reset sun60i_a733_r_ccu_resets[] = {
	SUNXI_CCU_RESET(A733_R_RST_TIMER,	0x11c, 16),
	SUNXI_CCU_RESET(A733_R_RST_PWM,		0x13c, 16),
	SUNXI_CCU_RESET(A733_R_RST_SPI,		0x15c, 16),
	SUNXI_CCU_RESET(A733_R_RST_MBOX, 	0x17c, 16),
	SUNXI_CCU_RESET(A733_R_RST_UART1,	0x18c, 17),
	SUNXI_CCU_RESET(A733_R_RST_UART0,	0x18c, 16),
	SUNXI_CCU_RESET(A733_R_RST_TWI2,	0x19c, 18),
	SUNXI_CCU_RESET(A733_R_RST_TWI1,	0x19c, 17),
	SUNXI_CCU_RESET(A733_R_RST_TWI0,	0x19c, 16),
	SUNXI_CCU_RESET(A733_R_RST_IRRX,	0x1cc, 16),
	SUNXI_CCU_RESET(A733_R_RST_RTC,		0x20c, 16),
	SUNXI_CCU_RESET(A733_R_RST_RISCV_CFG,	0x21c, 16),
	SUNXI_CCU_RESET(A733_R_RST_CPUCFG,	0x22c, 16),
	SUNXI_CCU_RESET(A733_R_RST_MODULE,	0x260, 0),
};

static const char *r_ahb_parents[] = { "dcxo", "rtc32k", "iosc", "pll-peri0-200m", "pll-peri0-300m" };
static const char *r_apbs_parents[] = { "dcxo", "rtc32k", "iosc", "pll-peri0-200m", "pll-ref" };
static const char *r_timer_parents[] = { "dcxo", "rtc32k", "iosc", "pll-peri0-200m", "pll-ref" };
static const char *r_pwm_parents[] = { "dcxo", "rtc32k", "iosc", "pll-ref" };
static const char *r_spi_parents[] = { "dcxo", "pll-peri0-200m", "pll-peri0-300m", "pll-peri1-300m", "pll-ref" };
static const char *r_irrx_parents[] = { "rtc32k", "dcxo", "pll-ref" };
static const char *riscv_24m_parents[] = { "dcxo", "osc32k", "iosc" };

static struct sunxi_ccu_clk sun60i_a733_r_ccu_clks[] = {
	SUNXI_CCU_M(A733_R_CLK_AHB, "r-ahb", r_ahb_parents,
	    AHBS_CLK_REG,		/* reg */
	    __BITS(4, 0),		/* m */
	    __BITS(26, 24),		/* sel */
	    0),
	SUNXI_CCU_M(A733_R_CLK_APBS0, "r-apbs0", r_apbs_parents,
	    APBS0_CLK_REG,		/* reg */
	    __BITS(4, 0),		/* m */
	    __BITS(26, 24),		/* sel */
	    0),
	SUNXI_CCU_M(A733_R_CLK_APBS0, "r-apbs1", r_apbs_parents,
	    APBS1_CLK_REG,		/* reg */
	    __BITS(4, 0),		/* m */
	    __BITS(26, 24),		/* sel */
	    0),

	SUNXI_CCU_DIV_GATE(A733_R_CLK_TIMER0, "r-timer0", r_timer_parents,
	    S_TIMER0_CLK_REG,		/* reg */
	    __BITS(3, 1),		/* div */
	    __BITS(6, 4),		/* sel */
	    __BIT(0),			/* enable */
	    SUNXI_CCU_DIV_POWER_OF_TWO),
	SUNXI_CCU_DIV_GATE(A733_R_CLK_TIMER1, "r-timer1", r_timer_parents,
	    S_TIMER1_CLK_REG,		/* reg */
	    __BITS(3, 1),		/* div */
	    __BITS(6, 4),		/* sel */
	    __BIT(0),			/* enable */
	    SUNXI_CCU_DIV_POWER_OF_TWO),
	SUNXI_CCU_DIV_GATE(A733_R_CLK_TIMER2, "r-timer2", r_timer_parents,
	    S_TIMER2_CLK_REG,		/* reg */
	    __BITS(3, 1),		/* div */
	    __BITS(6, 4),		/* sel */
	    __BIT(0),			/* enable */
	    SUNXI_CCU_DIV_POWER_OF_TWO),
	SUNXI_CCU_DIV_GATE(A733_R_CLK_TIMER3, "r-timer3", r_timer_parents,
	    S_TIMER3_CLK_REG,		/* reg */
	    __BITS(3, 1),		/* div */
	    __BITS(6, 4),		/* sel */
	    __BIT(0),			/* enable */
	    SUNXI_CCU_DIV_POWER_OF_TWO),

	SUNXI_CCU_GATE(A733_R_CLK_TIMER, "r-timer", "dcxo",
	    S_TIMER_BGR_REG, 0),
	SUNXI_CCU_GATE(A733_R_CLK_TWD, "r-twd", "dcxo",
	    S_TWD_BGR_REG, 0),

	SUNXI_CCU_DIV_GATE(A733_R_CLK_PWM, "r-pwm", r_pwm_parents,
	    S_PWM_CLK_REG,		/* reg */
	    0,				/* div */
	    __BITS(25, 24),		/* sel */
	    __BIT(31),			/* enable */
	    0),
	SUNXI_CCU_GATE(A733_R_CLK_BUS_PWM, "r-bus-pwm", "dcxo",
	    S_PWM_BGR_REG, 0),

	SUNXI_CCU_NM(A733_R_CLK_SPI, "r-spi", r_spi_parents,
	    S_SPI_CLK_REG,		/* reg */
	    __BITS(12, 8),		/* n */
	    __BITS(4, 0),		/* m */
	    __BITS(26, 24),		/* sel */
	    __BIT(31),			/* enable */
	    0),
	SUNXI_CCU_GATE(A733_R_CLK_BUS_SPI, "r-spi-bus", "dcxo",
	    S_SPI_BGR_REG, 0),

	SUNXI_CCU_GATE(A733_R_CLK_MBOX, "r-mbox", "dcxo",
	    S_MBOX_BGR_REG, 0),

	SUNXI_CCU_GATE(A733_R_CLK_UART1, "r-uart1", "r-apbs1",
	    S_UART_BGR_REG, 1),
	SUNXI_CCU_GATE(A733_R_CLK_UART0, "r-uart0", "r-apbs1",
	    S_UART_BGR_REG, 0),

	SUNXI_CCU_GATE(A733_R_CLK_TWI2, "r-twi2", "r-apbs1",
	    S_TWI_BGR_REG, 2),
	SUNXI_CCU_GATE(A733_R_CLK_TWI1, "r-twi1", "r-apbs1",
	    S_TWI_BGR_REG, 1),
	SUNXI_CCU_GATE(A733_R_CLK_TWI0, "r-twi0", "r-apbs1",
	    S_TWI_BGR_REG, 0),

	SUNXI_CCU_GATE(A733_R_CLK_PPU, "r-ppu", "dcxo",
	    S_PPU_BGR_REG, 0),

	SUNXI_CCU_GATE(A733_R_CLK_TZMA, "r-tzma", "dcxo",
	    S_TZMA_BGR_REG, 0),

	SUNXI_CCU_GATE(A733_R_CLK_CPUS_BIST, "r-cpus-bist", "dcxo",
	    S_CPUS_BIST_BGR_REG, 0),

	SUNXI_CCU_M_GATE(A733_R_CLK_IRRX, "r-irrx", r_irrx_parents,
	    S_IRRX_CLK_REG,		/* reg */
	    __BITS(4, 0),		/* m */
	    __BITS(25, 24),		/* sel */
	    __BIT(31),			/* enable */
	    0),
	SUNXI_CCU_GATE(A733_R_CLK_BUS_IRRX, "r-irrx-bus", "dcxo",
	    S_IRRX_BGR_REG, 0),

	SUNXI_CCU_GATE(A733_R_CLK_RTC, "rtc", "dcxo",
	    RTC_BGR_REG, 0),

	SUNXI_CCU_DIV_GATE(A733_R_CLK_RISCV_24M, "riscv-24m", riscv_24m_parents,
	    RISCV_24M_CLK_REG,		/* reg */
	    0,				/* div */
	    __BITS(25, 24),		/* sel */
	    __BIT(31),			/* enable */
	    0),
	SUNXI_CCU_GATE(A733_R_CLK_RISCV_CFG, "riscv-cfg", "dcxo",
	    RISCV_BGR_REG, 0),

	SUNXI_CCU_GATE(A733_R_CLK_CPUCFG, "r-cpucfg", "dcxo",
	    S_CPUCFG_BGR_REG, 0),

	SUNXI_CCU_GATE(A733_R_CLK_VDD_USB2CPUS, "vdd-usb2cpus", "dcxo", 0x250, 8),
	SUNXI_CCU_GATE(A733_R_CLK_VDD_SYS2USB, "vdd-sys2usb", "dcxo", 0x250, 3),
	SUNXI_CCU_GATE(A733_R_CLK_VDD_SYS2CPUS, "vdd-sys2cpus", "dcxo", 0x250, 2),
	SUNXI_CCU_GATE(A733_R_CLK_VDD_DDR, "vdd-ddr", "dcxo", 0x250, 0),

	SUNXI_CCU_GATE(A733_R_CLK_TT_AUTO, "tt-auto", "dcxo",
	    DEV_BUS_AUTOG_CTRL_REG, 9),
	SUNXI_CCU_GATE(A733_R_CLK_CPU_ICACHE_AUTO, "cpu-icache-auto", "dcxo",
	    DEV_BUS_AUTOG_CTRL_REG, 8),

	SUNXI_CCU_GATE(A733_R_CLK_AHBS_AUTO_CLK, "ahbs-auto-clk", "dcxo",
	    BUS_ACG_REG, 24),
};

static int
sun60i_a733_r_ccu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
sun60i_a733_r_ccu_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_ccu_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_resets = sun60i_a733_r_ccu_resets;
	sc->sc_nresets = __arraycount(sun60i_a733_r_ccu_resets);

	sc->sc_clks = sun60i_a733_r_ccu_clks;
	sc->sc_nclks = __arraycount(sun60i_a733_r_ccu_clks);

	if (sunxi_ccu_attach(sc) != 0) {
		return;
	}

	aprint_naive("\n");
	aprint_normal(": A733 PRCM CCU\n");

	sunxi_ccu_print(sc);
}
