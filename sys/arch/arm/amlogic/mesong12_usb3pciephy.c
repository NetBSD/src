/* $NetBSD: mesong12_usb3pciephy.c,v 1.2 2021/01/27 03:10:18 thorpej Exp $ */

/*
 * Copyright (c) 2021 Ryo Shimizu <ryo@nerv.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mesong12_usb3pciephy.c,v 1.2 2021/01/27 03:10:18 thorpej Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>

#define USB3PCIEPHY_R0_REG				0x00
#define  USB3PCIEPHY_R0_PCIE_USB3_SWITCH		__BITS(6,5)
#define  USB3PCIEPHY_R0_PCIE_POWER_STATE		__BITS(4,0)
#define USB3PCIEPHY_R1_REG				0x04
#define  USB3PCIEPHY_R1_PHY_MPLL_MULTIPLIER		__BITS(31,25)
#define  USB3PCIEPHY_R1_PHY_REF_CLKDIV2			__BIT(24)
#define  USB3PCIEPHY_R1_PHY_LOS_BIAS			__BITS(23,21)
#define  USB3PCIEPHY_R1_PHY_LOS_LEVEL			__BITS(20,16)
#define  USB3PCIEPHY_R1_PHY_RX0_EQ			__BITS(15,13)
#define  USB3PCIEPHY_R1_PHY_RX1_EQ			__BITS(12,10)
#define  USB3PCIEPHY_R1_PHY_TX0_TERM_OFFSET		__BITS(9,5)
#define  USB3PCIEPHY_R1_PHY_TX1_TERM_OFFSET		__BITS(4,0)
#define USB3PCIEPHY_R2_REG				0x08
#define  USB3PCIEPHY_R2_PHY_TX_VBOOST_LVL		__BITS(20,18)
#define  USB3PCIEPHY_R2_PCS_TX_DEEMPH_GEN1		__BITS(17,12)
#define  USB3PCIEPHY_R2_PCS_TX_DEEMPH_GEN2_3P5DB	__BITS(11,6)
#define  USB3PCIEPHY_R2_PCS_TX_DEEMPH_GEN2_6DB		__BITS(5,0)
#define USB3PCIEPHY_R4_REG				0x10
#define  USB3PCIEPHY_R4_PHY_CR_CAP_ADDR			__BIT(19)
#define  USB3PCIEPHY_R4_PHY_CR_CAP_DATA			__BIT(18)
#define  USB3PCIEPHY_R4_PHY_CR_DATA_IN			__BITS(17,2)
#define  USB3PCIEPHY_R4_PHY_CR_READ			__BIT(1)
#define  USB3PCIEPHY_R4_PHY_CR_WRITE			__BIT(0)
#define USB3PCIEPHY_R5_REG				0x14
#define  USB3PCIEPHY_R5_PHY_BS_OUT			__BIT(17)
#define  USB3PCIEPHY_R5_PHY_CR_ACK			__BIT(16)
#define  USB3PCIEPHY_R5_PHY_CR_DATA_OUT			__BITS(15,0)

#define PHY_READ_REG(sc, reg)	\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define PHY_WRITE_REG(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))


/* The values must be matched to those in dt-bindings/phy/phy.h */
#define PHY_NONE	0
#define PHY_TYPE_PCIE	2
#define PHY_TYPE_USB3	4

struct mesong12_usb3pciephy_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	struct clk *sc_clk;
	struct fdtbus_reset *sc_reset;
	struct fdtbus_regulator *sc_supply;
	int sc_phandle;
	int sc_phy_type;
};

static void *
mesong12_usb3pciephy_acquire(device_t dev, const void *data, size_t len)
{
	struct mesong12_usb3pciephy_softc * const sc = device_private(dev);
	const uint32_t *p = data;

	/* already acquired? */
	if (sc->sc_phy_type != PHY_NONE)
		return NULL;

	if (len != sizeof(uint32_t))
		return NULL;

	switch (be32toh(p[0])) {
	case PHY_TYPE_USB3:
		sc->sc_phy_type = PHY_TYPE_USB3;
		break;
	case PHY_TYPE_PCIE:
		return NULL;	/* PCIe mode is not supported */
	default:
		return NULL;
	}

	return sc;
}

static void
mesong12_usb3pciephy_release(device_t dev, void *priv)
{
	struct mesong12_usb3pciephy_softc * const sc = device_private(dev);

	sc->sc_phy_type = PHY_NONE;
}

static inline int
mesong12_usb3pciephy_ack(struct mesong12_usb3pciephy_softc *sc, bool ack,
    const char *str)
{
	int timeout;
	uint32_t val;

	for (timeout = 1000; timeout > 0; timeout--) {
		val = PHY_READ_REG(sc, USB3PCIEPHY_R5_REG);
		if (!(val & USB3PCIEPHY_R5_PHY_CR_ACK) == !ack)
			return 0;
		delay(5);
	}
	device_printf(sc->sc_dev, "phy %s %s timeout\n",
	    str, ack ? "ack" : "nack");
	return ETIMEDOUT;
}

static void
mesong12_usb3pciephy_addr(struct mesong12_usb3pciephy_softc *sc,
    bus_addr_t addr)
{
	uint32_t val;

	val = __SHIFTIN(addr, USB3PCIEPHY_R4_PHY_CR_DATA_IN);
	PHY_WRITE_REG(sc, USB3PCIEPHY_R4_REG, val);
	PHY_WRITE_REG(sc, USB3PCIEPHY_R4_REG, val);
	PHY_WRITE_REG(sc, USB3PCIEPHY_R4_REG, val |
	    USB3PCIEPHY_R4_PHY_CR_CAP_ADDR);
	if (mesong12_usb3pciephy_ack(sc, true, "addr") != 0)
		return;

	PHY_WRITE_REG(sc, USB3PCIEPHY_R4_REG, val);
	mesong12_usb3pciephy_ack(sc, false, "addr");
}

static uint16_t
mesong12_usb3pciephy_read(struct mesong12_usb3pciephy_softc *sc,
    bus_addr_t addr)
{
	uint32_t val;

	mesong12_usb3pciephy_addr(sc, addr);

	PHY_WRITE_REG(sc, USB3PCIEPHY_R4_REG, 0);
	PHY_WRITE_REG(sc, USB3PCIEPHY_R4_REG, USB3PCIEPHY_R4_PHY_CR_READ);
	if (mesong12_usb3pciephy_ack(sc, true, "read data") != 0)
		return 0;

	val = PHY_READ_REG(sc, USB3PCIEPHY_R5_REG);
	PHY_WRITE_REG(sc, USB3PCIEPHY_R4_REG, 0);
	if (mesong12_usb3pciephy_ack(sc, false, "read data") != 0)
		return 0;

	return __SHIFTOUT(val, USB3PCIEPHY_R5_PHY_CR_DATA_OUT);
}

static void
mesong12_usb3pciephy_write(struct mesong12_usb3pciephy_softc *sc,
    bus_addr_t addr, uint16_t data)
{
	uint32_t val;

	mesong12_usb3pciephy_addr(sc, addr);

	val = __SHIFTIN(addr, USB3PCIEPHY_R4_PHY_CR_DATA_IN);
	PHY_WRITE_REG(sc, USB3PCIEPHY_R4_REG, val);
	PHY_WRITE_REG(sc, USB3PCIEPHY_R4_REG, val);
	PHY_WRITE_REG(sc, USB3PCIEPHY_R4_REG, val |
	    USB3PCIEPHY_R4_PHY_CR_CAP_DATA);
	if (mesong12_usb3pciephy_ack(sc, true, "write addr") != 0)
		return;
	PHY_WRITE_REG(sc, USB3PCIEPHY_R4_REG, val);
	if (mesong12_usb3pciephy_ack(sc, false, "write addr") != 0)
		return;

	PHY_WRITE_REG(sc, USB3PCIEPHY_R4_REG, val);
	PHY_WRITE_REG(sc, USB3PCIEPHY_R4_REG, val |
	    USB3PCIEPHY_R4_PHY_CR_WRITE);
	if (mesong12_usb3pciephy_ack(sc, true, "write data") != 0)
		return;
	PHY_WRITE_REG(sc, USB3PCIEPHY_R4_REG, val);
	if (mesong12_usb3pciephy_ack(sc, false, "write data") != 0)
		return;
}

static int
mesong12_usb3pciephy_enable(device_t dev, void *priv, bool enable)
{
	struct mesong12_usb3pciephy_softc * const sc = device_private(dev);
	uint32_t val;

	fdtbus_clock_assign(sc->sc_phandle);
	if (sc->sc_reset != NULL) {
		fdtbus_reset_assert(sc->sc_reset);
		delay(10);
		fdtbus_reset_deassert(sc->sc_reset);
	}

	if (!enable)
		return 0;

	/* switch to USB3.0 */
	val = PHY_READ_REG(sc, USB3PCIEPHY_R0_REG);
	val &= ~USB3PCIEPHY_R0_PCIE_USB3_SWITCH;
	val |= __SHIFTIN(3, USB3PCIEPHY_R0_PCIE_USB3_SWITCH);
	PHY_WRITE_REG(sc, USB3PCIEPHY_R0_REG, val);
	delay(10);

#if 0 /* XXX: doesn't work? */
	/* workaround for SSPHY(SuperSpeedPHY) suspend bug */
	val = mesong12_usb3pciephy_read(sc, 0x102d);
	val |= __BIT(7);
	mesong12_usb3pciephy_write(sc, 0x102d, val);
#endif

	val = mesong12_usb3pciephy_read(sc, 0x1010);
	val &= ~__BITS(11,4);
	val |= __SHIFTIN(2, __BITS(11,4));
	mesong12_usb3pciephy_write(sc, 0x1010, val);

#if 0 /* XXX: doesn't work? */
	/* Rx equalization magic */
	val = mesong12_usb3pciephy_read(sc, 0x1006);
	val &= ~__BITS(7,6);
	val |= __SHIFTIN(2, __BITS(7,6));
	val &= ~__BITS(10,8);
	val |= __SHIFTIN(3, __BITS(10,8));
	val |= __BIT(11);
	mesong12_usb3pciephy_write(sc, 0x1006, val);
#endif

	/* Tx equalization magic */
	val = mesong12_usb3pciephy_read(sc, 0x1002);
	val &= ~__BITS(13,7);
	val |= __SHIFTIN(0x16, __BITS(13,7));
	val &= ~__BITS(6,0);
	val |= __SHIFTIN(0x7f, __BITS(6,0));
	val |= __BIT(14);
	mesong12_usb3pciephy_write(sc, 0x1002, val);

	/* MPLL loop magic */
	val = mesong12_usb3pciephy_read(sc, 0x30);
	val &= ~__BITS(7,4);
	val |= __SHIFTIN(8, __BITS(7,4));
	mesong12_usb3pciephy_write(sc, 0x30, val);


	val = PHY_READ_REG(sc, USB3PCIEPHY_R2_REG);
	val &= ~USB3PCIEPHY_R2_PHY_TX_VBOOST_LVL;
	val |= __SHIFTIN(4, USB3PCIEPHY_R2_PHY_TX_VBOOST_LVL);
	PHY_WRITE_REG(sc, USB3PCIEPHY_R2_REG, val);

	val = PHY_READ_REG(sc, USB3PCIEPHY_R1_REG);
	val &= ~USB3PCIEPHY_R1_PHY_LOS_BIAS;
	val |= __SHIFTIN(4, USB3PCIEPHY_R1_PHY_LOS_BIAS);
	val &= ~USB3PCIEPHY_R1_PHY_LOS_LEVEL;
	val |= __SHIFTIN(9, USB3PCIEPHY_R1_PHY_LOS_LEVEL);
	PHY_WRITE_REG(sc, USB3PCIEPHY_R1_REG, val);

	return 0;
}

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "amlogic,g12a-usb3-pcie-phy" },
	DEVICE_COMPAT_EOL
};

static int
mesong12_usb3pciephy_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static const struct fdtbus_phy_controller_func mesong12_usb3pciephy_funcs = {
	.acquire = mesong12_usb3pciephy_acquire,
	.release = mesong12_usb3pciephy_release,
	.enable = mesong12_usb3pciephy_enable
};

static void
mesong12_usb3pciephy_attach(device_t parent, device_t self, void *aux)
{
	struct mesong12_usb3pciephy_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	sc->sc_phandle = phandle;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	sc->sc_clk = fdtbus_clock_get_index(phandle, 0);
	if (sc->sc_clk == NULL) {
		aprint_error(": couldn't get clock\n");
		goto attach_failure;
	}
	if (clk_enable(sc->sc_clk) != 0) {
		aprint_error(": couldn't enable clock\n");
		goto attach_failure;
	}

	sc->sc_reset = fdtbus_reset_get_index(phandle, 0);
	sc->sc_supply = fdtbus_regulator_acquire(phandle, "phy-supply");
	if (sc->sc_supply != NULL)
		fdtbus_regulator_enable(sc->sc_supply);

	aprint_naive("\n");
	aprint_normal(": USB3 PCIe PHY\n");

	fdtbus_register_phy_controller(self, phandle,
	    &mesong12_usb3pciephy_funcs);
	return;

 attach_failure:
	bus_space_unmap(sc->sc_bst, sc->sc_bsh, size);
	return;
}

CFATTACH_DECL_NEW(mesong12_usb3pciephy,
    sizeof(struct mesong12_usb3pciephy_softc),
    mesong12_usb3pciephy_match, mesong12_usb3pciephy_attach, NULL, NULL);
