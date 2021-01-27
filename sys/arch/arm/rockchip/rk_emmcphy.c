/* $NetBSD: rk_emmcphy.c,v 1.4 2021/01/27 03:10:19 thorpej Exp $ */

/*-
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: rk_emmcphy.c,v 1.4 2021/01/27 03:10:19 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/kmem.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

#define	GRF_EMMCPHY_CON0	0x00
#define	 PHYCTRL_FRQSEL			__BITS(13,12)
#define	  PHYCTRL_FRQSEL_200M		0
#define	  PHYCTRL_FRQSEL_50M		1
#define	  PHYCTRL_FRQSEL_100M		2
#define	  PHYCTRL_FRQSEL_150M		3
#define	 PHYCTRL_OTAPDLYENA		__BIT(11)
#define	 PHYCTRL_OTAPDLYSEL		__BITS(10,7)
#define	 PHYCTRL_ITAPCHGWIN		__BIT(6)
#define	 PHYCTRL_ITAPDLYSEL		__BITS(5,1)
#define	 PHYCTRL_ITAPDLYENA		__BIT(0)
#define	GRF_EMMCPHY_CON1	0x04
#define	 PHYCTRL_CLKBUFSEL		__BITS(8,6)
#define	 PHYCTRL_SELDLYTXCLK		__BIT(5)
#define	 PHYCTRL_SELDLYRXCLK		__BIT(4)
#define	 PHYCTRL_STRBSEL		__BITS(3,0)
#define	GRF_EMMCPHY_CON2	0x08
#define	 PHYCTRL_REN_STRB		__BIT(9)
#define	 PHYCTRL_REN_CMD		__BIT(8)
#define	 PHYCTRL_REN_DAT		__BITS(7,0)
#define	GRF_EMMCPHY_CON3	0x0c
#define	 PHYCTRL_PU_STRB		__BIT(9)
#define	 PHYCTRL_PU_CMD			__BIT(8)
#define	 PHYCTRL_PU_DAT			__BITS(7,0)
#define	GRF_EMMCPHY_CON4	0x10
#define	 PHYCTRL_OD_RELEASE_CMD		__BIT(9)
#define	 PHYCTRL_OD_RELEASE_STRB	__BIT(8)
#define	 PHYCTRL_OD_RELEASE_DAT		__BITS(7,0)
#define	GRF_EMMCPHY_CON5	0x14
#define	 PHYCTRL_ODEN_STRB		__BIT(9)
#define	 PHYCTRL_ODEN_CMD		__BIT(8)
#define	 PHYCTRL_ODEN_DAT		__BITS(7,0)
#define	GRF_EMMCPHY_CON6	0x18
#define	 PHYCTRL_DLL_TRM_ICP		__BITS(12,9)
#define	 PHYCTRL_EN_RTRIM		__BIT(8)
#define	 PHYCTRL_RETRIM			__BIT(7)
#define	 PHYCTRL_DR_TY			__BITS(6,4)
#define	 PHYCTRL_RETENB			__BIT(3)
#define	 PHYCTRL_RETEN			__BIT(2)
#define	 PHYCTRL_ENDLL			__BIT(1)
#define	 PHYCTRL_PDB			__BIT(0)
#define	GRF_EMMCPHY_STATUS	0x20
#define	 PHYCTRL_CALDONE		__BIT(6)
#define	 PHYCTRL_DLLRDY			__BIT(5)
#define	 PHYCTRL_RTRIM			__BITS(4,1)
#define	 PHYCTRL_EXR_NINST		__BIT(0)

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "rockchip,rk3399-emmc-phy" },
	DEVICE_COMPAT_EOL
};

struct rk_emmcphy_softc {
	device_t		sc_dev;
	struct syscon		*sc_syscon;
	bus_addr_t		sc_regbase;
	int			sc_phandle;
	struct clk		*sc_clk;
};

#define RD4(sc, reg) 		\
	syscon_read_4((sc)->sc_syscon, (sc)->sc_regbase + (reg))
#define WR4(sc, reg, val) 	\
	syscon_write_4((sc)->sc_syscon, (sc)->sc_regbase + (reg), (val))

static int	rk_emmcphy_match(device_t, cfdata_t, void *);
static void	rk_emmcphy_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(rkemmcphy, sizeof(struct rk_emmcphy_softc),
	rk_emmcphy_match, rk_emmcphy_attach, NULL, NULL);

static void *
rk_emmcphy_acquire(device_t dev, const void *data, size_t len)
{
	struct rk_emmcphy_softc * const sc = device_private(dev);

	if (len != 0)
		return NULL;

	if (sc->sc_clk == NULL)
		sc->sc_clk = fdtbus_clock_get(sc->sc_phandle, "emmcclk");

	return sc;
}

static void
rk_emmcphy_release(device_t dev, void *priv)
{
}

static int
rk_emmcphy_enable(device_t dev, void *priv, bool enable)
{
	struct rk_emmcphy_softc * const sc = device_private(dev);
	uint32_t mask, val;
	u_int rate, frqsel;

	syscon_lock(sc->sc_syscon);

	if (enable) {
		/* Drive strength */
		mask = PHYCTRL_DR_TY;
		val = __SHIFTIN(0, PHYCTRL_DR_TY);
		WR4(sc, GRF_EMMCPHY_CON6, (mask << 16) | val);

		/* Enable output tap delay */
		mask = PHYCTRL_OTAPDLYENA | PHYCTRL_OTAPDLYSEL;
		val = PHYCTRL_OTAPDLYENA | __SHIFTIN(4, PHYCTRL_OTAPDLYSEL);
		WR4(sc, GRF_EMMCPHY_CON0, (mask << 16) | val);
	}

	/* Power down PHY and disable DLL before making changes */
	mask = PHYCTRL_ENDLL | PHYCTRL_PDB;
	val = 0;
	WR4(sc, GRF_EMMCPHY_CON6, (mask << 16) | val);

	if (enable == false) {
		syscon_unlock(sc->sc_syscon);
		return 0;
	}

	rate = sc->sc_clk ? clk_get_rate(sc->sc_clk) : 0;

	if (rate != 0) {
		if (rate < 75000000)
			frqsel = PHYCTRL_FRQSEL_50M;
		else if (rate < 125000000)
			frqsel = PHYCTRL_FRQSEL_100M;
		else if (rate < 175000000)
			frqsel = PHYCTRL_FRQSEL_150M;
		else
			frqsel = PHYCTRL_FRQSEL_200M;
	} else {
		frqsel = PHYCTRL_FRQSEL_200M;
	}

	delay(3);

	/* Power up PHY */
	mask = PHYCTRL_PDB;
	val = PHYCTRL_PDB;
	WR4(sc, GRF_EMMCPHY_CON6, (mask << 16) | val);

	/* Wait for calibration */
	delay(10);
	val = RD4(sc, GRF_EMMCPHY_STATUS);
	if ((val & PHYCTRL_CALDONE) == 0) {
		device_printf(dev, "PHY calibration did not complete\n");
		syscon_unlock(sc->sc_syscon);
		return EIO;
	}

	/* Set DLL frequency */
	mask = PHYCTRL_FRQSEL;
	val = __SHIFTIN(frqsel, PHYCTRL_FRQSEL);
	WR4(sc, GRF_EMMCPHY_CON0, (mask << 16) | val);

	/* Enable DLL */
	mask = PHYCTRL_ENDLL;
	val = PHYCTRL_ENDLL;
	WR4(sc, GRF_EMMCPHY_CON6, (mask << 16) | val);

	if (rate != 0) {
		/* Wait for DLL ready */
		delay(50000);
		val = RD4(sc, GRF_EMMCPHY_STATUS);
		if ((val & PHYCTRL_DLLRDY) == 0) {
			device_printf(dev, "DLL loop failed to lock\n");
			syscon_unlock(sc->sc_syscon);
			return EIO;
		}
	}

	syscon_unlock(sc->sc_syscon);

	return 0;
}

static const struct fdtbus_phy_controller_func rk_emmcphy_funcs = {
	.acquire = rk_emmcphy_acquire,
	.release = rk_emmcphy_release,
	.enable = rk_emmcphy_enable,
};

static int
rk_emmcphy_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
rk_emmcphy_attach(device_t parent, device_t self, void *aux)
{
	struct rk_emmcphy_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_syscon = fdtbus_syscon_lookup(OF_parent(phandle));
	if (sc->sc_syscon == NULL) {
		aprint_error(": couldn't get syscon\n");
		return;
	}
	sc->sc_regbase = addr;

	aprint_naive("\n");
	aprint_normal(": eMMC PHY\n");

	fdtbus_register_phy_controller(self, phandle, &rk_emmcphy_funcs);
}
