/* $NetBSD: zynq7000_clkc.c,v 1.5 2022/11/11 20:29:47 jmcneill Exp $ */

/*-
 * Copyright (c) 2022 Jared McNeill <jmcneill@invisible.ca>
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

__KERNEL_RCSID(0, "$NetBSD: zynq7000_clkc.c,v 1.5 2022/11/11 20:29:47 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kmem.h>

#include <dev/clk/clk_backend.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

#define	ARM_PLL_CTRL	0x100
#define	DDR_PLL_CTRL	0x104
#define	IO_PLL_CTRL	0x108
#define	 PLL_FDIV		__BITS(18,12)
#define	ARM_CLK_CTRL	0x120
#define	 ARM_CLK_CTRL_CPU_1XCLKACT	__BIT(27)
#define	 ARM_CLK_CTRL_CPU_2XCLKACT	__BIT(26)
#define	 ARM_CLK_CTRL_CPU_3OR2XCLKACT	__BIT(25)
#define	 ARM_CLK_CTRL_CPU_6OR4XCLKACT	__BIT(24)
#define	 ARM_CLK_CTRL_DIVISOR		__BITS(13,8)
#define	APER_CLK_CTRL	0x12c
#define	 UART1_CPU_1XCLKACT	__BIT(21)
#define	 UART0_CPU_1XCLKACT	__BIT(20)
#define	 I2C1_CPU_1XCLKACT	__BIT(19)
#define	 I2C0_CPU_1XCLKACT	__BIT(18)
#define	 SDI1_CPU_1XCLKACT	__BIT(11)
#define	 SDI0_CPU_1XCLKACT	__BIT(10)
#define	SDIO_CLK_CTRL	0x150
#define	UART_CLK_CTRL	0x154
#define	PCAP_CLK_CTRL	0x168
#define	 CLK_CTRL_DIVISOR	__BITS(13,8)
#define	 CLK_CTRL_SRCSEL	__BITS(5,4)
#define	 CLK_CTRL_CLKACT1	__BIT(1)
#define	 CLK_CTRL_CLKACT0	__BIT(0)
#define	CLK_621_TRUE	0x1C4
#define	 CLK_621_TRUE_EN	__BIT(0)

enum xynq7000_clkid {
	clkid_armpll,
	clkid_ddrpll,
	clkid_iopll,
	clkid_cpu_6or4x,
	clkid_cpu_3or2x,
	clkid_cpu_2x,
	clkid_cpu_1x,
	clkid_ddr2x,
	clkid_ddr3x,
	clkid_dci,
	clkid_lqspi,
	clkid_smc,
	clkid_pcap,
	clkid_gem0,
	clkid_gem1,
	clkid_fclk0,
	clkid_fclk1,
	clkid_fclk2,
	clkid_fclk3,
	clkid_can0,
	clkid_can1,
	clkid_sdio0,
	clkid_sdio1,
	clkid_uart0,
	clkid_uart1,
	clkid_spi0,
	clkid_spi1,
	clkid_dma,
	clkid_usb0_aper,
	clkid_usb1_aper,
	clkid_gem0_aper,
	clkid_gem1_aper,
	clkid_sdio0_aper,
	clkid_sdio1_aper,
	clkid_spi0_aper,
	clkid_spi1_aper,
	clkid_can0_aper,
	clkid_can1_aper,
	clkid_i2c0_aper,
	clkid_i2c1_aper,
	clkid_uart0_aper,
	clkid_uart1_aper,
	clkid_gpio_aper,
	clkid_lqspi_aper,
	clkid_smc_aper,
	clkid_swdt,
	clkid_dbg_trc,
	clkid_dbg_apb,
	num_clkid
};
CTASSERT(clkid_dbg_apb == 47);

static int zynq7000_clkc_match(device_t, cfdata_t, void *);
static void zynq7000_clkc_attach(device_t, device_t, void *);

static u_int zynq7000_clkc_clk_get_rate(void *, struct clk *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "xlnx,ps7-clkc" },
	DEVICE_COMPAT_EOL
};

struct zynq7000_clkc_softc {
	device_t		sc_dev;
	struct clk_domain	sc_clkdom;
	struct clk		sc_clk[num_clkid];

	u_int			sc_ps_clk_frequency;
	struct syscon		*sc_syscon;
};

CFATTACH_DECL_NEW(zynq7000_clkc, sizeof(struct zynq7000_clkc_softc),
	zynq7000_clkc_match, zynq7000_clkc_attach, NULL, NULL);

static struct clk *
zynq7000_clkc_clk_get(void *priv, const char *name)
{
	struct zynq7000_clkc_softc * const sc = priv;
	u_int clkid;

	for (clkid = 0; clkid < num_clkid; clkid++) {
		if (strcmp(name, sc->sc_clk[clkid].name) == 0) {
			return &sc->sc_clk[clkid];
		}
	}

	return NULL;
}

static void
zynq7000_clkc_clk_put(void *priv, struct clk *clk)
{
}

static u_int
zynq7000_clkc_get_rate_pll(struct zynq7000_clkc_softc *sc,
    bus_addr_t reg)
{
	uint32_t val;

	syscon_lock(sc->sc_syscon);
	val = syscon_read_4(sc->sc_syscon, reg);
	syscon_unlock(sc->sc_syscon);

	return sc->sc_ps_clk_frequency * __SHIFTOUT(val, PLL_FDIV);
}

static u_int
zynq7000_clkc_get_rate_iop(struct zynq7000_clkc_softc *sc,
    bus_addr_t reg)
{
	uint32_t val;
	u_int prate, sel;

	syscon_lock(sc->sc_syscon);
	val = syscon_read_4(sc->sc_syscon, reg);
	syscon_unlock(sc->sc_syscon);

	sel = __SHIFTOUT(val, CLK_CTRL_SRCSEL);
	if (sel == 2) {
		prate = zynq7000_clkc_clk_get_rate(sc,
		    &sc->sc_clk[clkid_armpll]);
	} else if (sel == 3) {
		prate = zynq7000_clkc_clk_get_rate(sc,
		    &sc->sc_clk[clkid_ddrpll]);
	} else {
		prate = zynq7000_clkc_clk_get_rate(sc,
		    &sc->sc_clk[clkid_iopll]);
	}

	return prate / __SHIFTOUT(val, CLK_CTRL_DIVISOR);
}

static u_int
zynq7000_clkc_clk_get_rate(void *priv, struct clk *clk)
{
	struct zynq7000_clkc_softc * const sc = priv;
	uint32_t val;
	u_int prate;

	if (clk == &sc->sc_clk[clkid_armpll]) {
		return zynq7000_clkc_get_rate_pll(sc, ARM_PLL_CTRL);
	} else if (clk == &sc->sc_clk[clkid_iopll]) {
		return zynq7000_clkc_get_rate_pll(sc, IO_PLL_CTRL);
	} else if (clk == &sc->sc_clk[clkid_cpu_6or4x]) {
		prate = zynq7000_clkc_clk_get_rate(sc,
		    &sc->sc_clk[clkid_cpu_1x]);
		syscon_lock(sc->sc_syscon);
		val = syscon_read_4(sc->sc_syscon, CLK_621_TRUE);
		syscon_unlock(sc->sc_syscon);
		return (val & CLK_621_TRUE_EN) != 0 ?
		    prate * 6 : prate * 4;
	} else if (clk == &sc->sc_clk[clkid_cpu_3or2x]) {
		prate = zynq7000_clkc_clk_get_rate(sc,
		    &sc->sc_clk[clkid_cpu_1x]);
		syscon_lock(sc->sc_syscon);
		val = syscon_read_4(sc->sc_syscon, CLK_621_TRUE);
		syscon_unlock(sc->sc_syscon);
		return (val & CLK_621_TRUE_EN) != 0 ?
		    prate * 3 : prate * 2;
	} else if (clk == &sc->sc_clk[clkid_cpu_2x]) {
		prate = zynq7000_clkc_clk_get_rate(sc,
		    &sc->sc_clk[clkid_cpu_1x]);
		return prate * 2;
	} else if (clk == &sc->sc_clk[clkid_cpu_1x]) {
		prate = zynq7000_clkc_clk_get_rate(sc,
		    &sc->sc_clk[clkid_armpll]);
		syscon_lock(sc->sc_syscon);
		val = syscon_read_4(sc->sc_syscon, ARM_CLK_CTRL);
		syscon_unlock(sc->sc_syscon);
		return prate / __SHIFTOUT(val, ARM_CLK_CTRL_DIVISOR) / 6;
	} else if (clk == &sc->sc_clk[clkid_sdio0] ||
		   clk == &sc->sc_clk[clkid_sdio1]) {
		return zynq7000_clkc_get_rate_iop(sc, SDIO_CLK_CTRL);
	} else if (clk == &sc->sc_clk[clkid_uart0] ||
		   clk == &sc->sc_clk[clkid_uart1]) {
		return zynq7000_clkc_get_rate_iop(sc, UART_CLK_CTRL);
	} else if (clk == &sc->sc_clk[clkid_pcap]) {
		return zynq7000_clkc_get_rate_iop(sc, PCAP_CLK_CTRL);
	} else if (clk == &sc->sc_clk[clkid_uart0_aper] ||
		   clk == &sc->sc_clk[clkid_uart1_aper] ||
		   clk == &sc->sc_clk[clkid_i2c0_aper] ||
		   clk == &sc->sc_clk[clkid_i2c1_aper]) {
		return zynq7000_clkc_clk_get_rate(sc,
		    &sc->sc_clk[clkid_cpu_1x]);
	} else {
		/* Not implemented. */
		return 0;
	}
}

static int
zynq7000_clkc_clk_enable(void *priv, struct clk *clk)
{
	struct zynq7000_clkc_softc * const sc = priv;
	uint32_t val, mask;
	bus_addr_t reg;

	if (clk == &sc->sc_clk[clkid_cpu_1x]) {
		reg = ARM_CLK_CTRL;
		mask = ARM_CLK_CTRL_CPU_1XCLKACT;
	} else if (clk == &sc->sc_clk[clkid_cpu_2x]) {
		reg = ARM_CLK_CTRL;
		mask = ARM_CLK_CTRL_CPU_2XCLKACT;
	} else if (clk == &sc->sc_clk[clkid_cpu_3or2x]) {
		reg = ARM_CLK_CTRL;
		mask = ARM_CLK_CTRL_CPU_3OR2XCLKACT;
	} else if (clk == &sc->sc_clk[clkid_cpu_6or4x]) {
		reg = ARM_CLK_CTRL;
		mask = ARM_CLK_CTRL_CPU_6OR4XCLKACT;
	} else if (clk == &sc->sc_clk[clkid_sdio0]) {
		reg = SDIO_CLK_CTRL;
		mask = CLK_CTRL_CLKACT0;
	} else if (clk == &sc->sc_clk[clkid_sdio1]) {
		reg = SDIO_CLK_CTRL;
		mask = CLK_CTRL_CLKACT1;
	} else if (clk == &sc->sc_clk[clkid_uart0]) {
		reg = UART_CLK_CTRL;
		mask = CLK_CTRL_CLKACT0;
	} else if (clk == &sc->sc_clk[clkid_uart1]) {
		reg = UART_CLK_CTRL;
		mask = CLK_CTRL_CLKACT1;
	} else if (clk == &sc->sc_clk[clkid_pcap]) {
		reg = PCAP_CLK_CTRL;
		mask = CLK_CTRL_CLKACT0;
	} else if (clk == &sc->sc_clk[clkid_sdio0_aper]) {
		reg = APER_CLK_CTRL;
		mask = SDI0_CPU_1XCLKACT;
	} else if (clk == &sc->sc_clk[clkid_sdio1_aper]) {
		reg = APER_CLK_CTRL;
		mask = SDI1_CPU_1XCLKACT;
	} else if (clk == &sc->sc_clk[clkid_uart0_aper]) {
		reg = APER_CLK_CTRL;
		mask = UART0_CPU_1XCLKACT;
	} else if (clk == &sc->sc_clk[clkid_uart1_aper]) {
		reg = APER_CLK_CTRL;
		mask = UART1_CPU_1XCLKACT;
	} else if (clk == &sc->sc_clk[clkid_i2c0_aper]) {
		reg = APER_CLK_CTRL;
		mask = I2C0_CPU_1XCLKACT;
	} else if (clk == &sc->sc_clk[clkid_i2c1_aper]) {
		reg = APER_CLK_CTRL;
		mask = I2C1_CPU_1XCLKACT;
	} else {
		return ENXIO;
	}

	syscon_lock(sc->sc_syscon);
	val = syscon_read_4(sc->sc_syscon, reg);
	syscon_write_4(sc->sc_syscon, reg, val | mask);
	syscon_unlock(sc->sc_syscon);

	return 0;
}

static int
zynq7000_clkc_clk_disable(void *priv, struct clk *clk)
{
	return ENXIO;
}

static const struct clk_funcs zynq7000_clkc_clk_funcs = {
	.get = zynq7000_clkc_clk_get,
	.put = zynq7000_clkc_clk_put,
	.get_rate = zynq7000_clkc_clk_get_rate,
	.enable = zynq7000_clkc_clk_enable,
	.disable = zynq7000_clkc_clk_disable,
};

static struct clk *
zynq7000_clkc_fdt_decode(device_t dev, int cc_phandle, const void *data, size_t len)
{
	struct zynq7000_clkc_softc * const sc = device_private(dev);
	u_int clkid;

	if (len != 4) {
		return NULL;
	}

	clkid = be32dec(data);
	if (clkid >= num_clkid) {
		return NULL;
	}

	return &sc->sc_clk[clkid];
}

static const struct fdtbus_clock_controller_func zynq7000_clkc_fdt_funcs = {
	.decode = zynq7000_clkc_fdt_decode
};

static int
zynq7000_clkc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
zynq7000_clkc_attach(device_t parent, device_t self, void *aux)
{
	struct zynq7000_clkc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	const char *clkname;
	bus_addr_t addr;
	bus_size_t size;
	u_int clkid;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_syscon = fdtbus_syscon_lookup(OF_parent(phandle));
	if (sc->sc_syscon == NULL) {
		aprint_error(": couldn't get syscon registers\n");
		return;
	}

	error = of_getprop_uint32(phandle, "ps-clk-frequency",
	    &sc->sc_ps_clk_frequency);
	if (error != 0) {
		aprint_error(": couldn't get ps-clk-frequency\n");
		return;
	}

	sc->sc_clkdom.name = device_xname(self);
	sc->sc_clkdom.funcs = &zynq7000_clkc_clk_funcs;
	sc->sc_clkdom.priv = sc;
	for (clkid = 0; clkid < num_clkid; clkid++) {
		clkname = fdtbus_get_string_index(phandle,
		    "clock-output-names", clkid);
		sc->sc_clk[clkid].domain = &sc->sc_clkdom;
		if (clkname != NULL) {
			sc->sc_clk[clkid].name = kmem_asprintf("%s", clkname);
		}
		clk_attach(&sc->sc_clk[clkid]);
	}

	aprint_naive("\n");
	aprint_normal(": Zynq-7000 PS clock subsystem (PS_CLK %u Hz)\n",
	    sc->sc_ps_clk_frequency);

	for (clkid = 0; clkid < num_clkid; clkid++) {
		aprint_debug_dev(self, "clkid %u [%s]: %u Hz\n", clkid,
		    sc->sc_clk[clkid].name ? sc->sc_clk[clkid].name : "<none>",
		    clk_get_rate(&sc->sc_clk[clkid]));
	}

	fdtbus_register_clock_controller(self, phandle, &zynq7000_clkc_fdt_funcs);
}
