/*	$NetBSD: rk3399_pcie_phy.c,v 1.4 2021/01/27 03:10:19 thorpej Exp $	*/
/*	$OpenBSD: rkpcie.c,v 1.6 2018/08/28 09:33:18 jsg Exp $	*/
/*
 * Copyright (c) 2018 Mark Kettenis <kettenis@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: rk3399_pcie_phy.c,v 1.4 2021/01/27 03:10:19 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <machine/intr.h>
#include <sys/bus.h>
#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

#include <sys/gpio.h>

#define RKPCIEPHY_MAXPHY 4

struct rkpciephy_softc {
	device_t		sc_dev;
	int			sc_phy_node;
	uint8_t			sc_phys[RKPCIEPHY_MAXPHY];
	u_int			sc_phys_on;
};

static int rkpciephy_match(device_t, cfdata_t, void *);
static void rkpciephy_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(rkpciephy, sizeof(struct rkpciephy_softc),
        rkpciephy_match, rkpciephy_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "rockchip,rk3399-pcie-phy" },
	DEVICE_COMPAT_EOL
};

static int
rkpciephy_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args *faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void rkpcie_phy_poweron(struct rkpciephy_softc *, u_int);

static inline void
clock_enable(int phandle, const char *name)
{
	struct clk * clk = fdtbus_clock_get(phandle, name);
	if (clk == NULL)
		return;
	if (clk_enable(clk) != 0)
		return;
}

static void
reset_assert(int phandle, const char *name)
{
	struct fdtbus_reset *rst;

	rst = fdtbus_reset_get(phandle, name);
	fdtbus_reset_assert(rst);
	fdtbus_reset_put(rst);
}

static void
reset_deassert(int phandle, const char *name)
{
        struct fdtbus_reset *rst;

	rst = fdtbus_reset_get(phandle, name);
	fdtbus_reset_deassert(rst);
	fdtbus_reset_put(rst);
}

static void *
rkpciephy_phy_acquire(device_t dev, const void *data, size_t len)
{
	struct rkpciephy_softc * const sc = device_private(dev);

	if (len != 4)
		return NULL;

	const int phy_id = be32dec(data);
	if (phy_id >= RKPCIEPHY_MAXPHY)
		return NULL;
//	device_printf(dev, "%s phy_id %d %d\n", __func__, phy_id, sc->sc_phys[phy_id]);

	if (true /*XXX*/ || sc->sc_phys_on == 0) {
		clock_enable(sc->sc_phy_node, "refclk");
		reset_assert(sc->sc_phy_node, "phy");
	}

	return &sc->sc_phys[phy_id];
}

static int
rkpciephy_phy_enable(device_t dev, void *priv, bool enable)
{
	struct rkpciephy_softc * const sc = device_private(dev);
	uint8_t * const lane = priv;

//	device_printf(dev, "%s %u %u\n", __func__, *lane, enable);

	if (enable) {
		rkpcie_phy_poweron(sc, *lane);
		sc->sc_phys_on |= 1U << *lane;
	} else {
#if notyet
		sc->sc_phys_on &= ~(1U << *lane);
#endif
	}

	return 0;
}

const struct fdtbus_phy_controller_func rkpciephy_phy_funcs = {
 	.acquire = rkpciephy_phy_acquire,
 	.release = (void *)voidop,
 	.enable = rkpciephy_phy_enable,
};

static void
rkpciephy_attach(device_t parent, device_t self, void *aux)
{
	struct rkpciephy_softc *sc = device_private(self);
	struct fdt_attach_args *faa = aux;

	sc->sc_dev = self;
	sc->sc_phy_node = faa->faa_phandle;
	
	aprint_naive("\n");
	aprint_normal(": RK3399 PCIe PHY\n");

	for (size_t i = 0; i < RKPCIEPHY_MAXPHY; i++)
		sc->sc_phys[i] = i;

	fdtbus_register_phy_controller(self, faa->faa_phandle, &rkpciephy_phy_funcs);
}

/*
 * PHY Support.
 */

#define RK3399_GRF_SOC_CON5_PCIE	0xe214
#define  RK3399_TX_ELEC_IDLE_OFF_MASK	((1 << 3) << 16)
#define  RK3399_TX_ELEC_IDLE_OFF	(1 << 3)
#define RK3399_GRF_SOC_CON8		0xe220
#define  RK3399_PCIE_TEST_DATA_MASK	((0xf << 7) << 16)
#define  RK3399_PCIE_TEST_DATA_SHIFT	7
#define  RK3399_PCIE_TEST_ADDR_MASK	((0x3f << 1) << 16)
#define  RK3399_PCIE_TEST_ADDR_SHIFT	1
#define  RK3399_PCIE_TEST_WRITE_ENABLE	(((1 << 0) << 16) | (1 << 0))
#define  RK3399_PCIE_TEST_WRITE_DISABLE	(((1 << 0) << 16) | (0 << 0))
#define RK3399_GRF_SOC_STATUS1		0xe2a4
#define  RK3399_PCIE_PHY_PLL_LOCKED	(1 << 9)
#define  RK3399_PCIE_PHY_PLL_OUTPUT	(1 << 10)

#define RK3399_PCIE_PHY_CFG_PLL_LOCK	0x10
#define RK3399_PCIE_PHY_CFG_CLK_TEST	0x10
#define  RK3399_PCIE_PHY_CFG_SEPE_RATE	(1 << 3)
#define RK3399_PCIE_PHY_CFG_CLK_SCC	0x12
#define  RK3399_PCIE_PHY_CFG_PLL_100M	(1 << 3)

static void
rkpcie_phy_write_conf(struct syscon *rm, uint8_t addr, uint8_t data)
{
	syscon_write_4(rm, RK3399_GRF_SOC_CON8,
	    RK3399_PCIE_TEST_ADDR_MASK |
	    (addr << RK3399_PCIE_TEST_ADDR_SHIFT) |
	    RK3399_PCIE_TEST_DATA_MASK |
	    (data << RK3399_PCIE_TEST_DATA_SHIFT) |
	    RK3399_PCIE_TEST_WRITE_DISABLE);
	delay(1);
	syscon_write_4(rm, RK3399_GRF_SOC_CON8,
	    RK3399_PCIE_TEST_WRITE_ENABLE);
	delay(1);
	syscon_write_4(rm, RK3399_GRF_SOC_CON8,
	    RK3399_PCIE_TEST_WRITE_DISABLE);
}

static void
rkpcie_phy_poweron(struct rkpciephy_softc *sc, u_int lane)
{
	struct syscon *rm;
	uint32_t status;
	int timo;

	reset_deassert(sc->sc_phy_node, "phy");

	rm = fdtbus_syscon_lookup(OF_parent(sc->sc_phy_node));
	if (rm == NULL)
		return;

	syscon_lock(rm);
	syscon_write_4(rm, RK3399_GRF_SOC_CON8,
	    RK3399_PCIE_TEST_ADDR_MASK |
	    RK3399_PCIE_PHY_CFG_PLL_LOCK << RK3399_PCIE_TEST_ADDR_SHIFT);
	syscon_write_4(rm, RK3399_GRF_SOC_CON5_PCIE,
	    RK3399_TX_ELEC_IDLE_OFF_MASK << lane | 0);
	//printf("%s %x\n", __func__, syscon_read_4(rm, RK3399_GRF_SOC_CON5_PCIE));

	for (timo = 50; timo > 0; timo--) {
		status = syscon_read_4(rm, RK3399_GRF_SOC_STATUS1);
		if (status & RK3399_PCIE_PHY_PLL_LOCKED)
			break;
		delay(20000);
	}
	if (timo == 0) {
		device_printf(sc->sc_dev, "PHY PLL lock timeout\n");
		syscon_unlock(rm);
		return;
	}

	rkpcie_phy_write_conf(rm, RK3399_PCIE_PHY_CFG_CLK_TEST,
	    RK3399_PCIE_PHY_CFG_SEPE_RATE);
	rkpcie_phy_write_conf(rm, RK3399_PCIE_PHY_CFG_CLK_SCC,
	    RK3399_PCIE_PHY_CFG_PLL_100M);

	for (timo = 50; timo > 0; timo--) {
		status = syscon_read_4(rm, RK3399_GRF_SOC_STATUS1);
		if ((status & RK3399_PCIE_PHY_PLL_OUTPUT) == 0)
			break;
		delay(20000);
	}
	if (timo == 0) {
		device_printf(sc->sc_dev, "PHY PLL output enable timeout\n");
		syscon_unlock(rm);
		return;
	}

	syscon_write_4(rm, RK3399_GRF_SOC_CON8,
	    RK3399_PCIE_TEST_ADDR_MASK |
	    RK3399_PCIE_PHY_CFG_PLL_LOCK << RK3399_PCIE_TEST_ADDR_SHIFT);
		
	for (timo = 50; timo > 0; timo--) {
		status = syscon_read_4(rm, RK3399_GRF_SOC_STATUS1);
		if (status & RK3399_PCIE_PHY_PLL_LOCKED)
			break;
		delay(20000);
	}
	if (timo == 0) {
		device_printf(sc->sc_dev, "PHY PLL relock timeout\n");
		syscon_unlock(rm);
		return;
	}
	syscon_unlock(rm);
}
