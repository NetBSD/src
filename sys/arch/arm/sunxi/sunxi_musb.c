/* $NetBSD: sunxi_musb.c,v 1.5 2018/04/09 16:21:09 jakllsch Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: sunxi_musb.c,v 1.5 2018/04/09 16:21:09 jakllsch Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/pool.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/motgvar.h>
#include <dev/usb/motgreg.h>

#include <dev/fdt/fdtvar.h>

#include <machine/bus_defs.h>

#define	MUSB2_REG_AWIN_VEND0	0x43

static int	sunxi_musb_match(device_t, cfdata_t, void *);
static void	sunxi_musb_attach(device_t, device_t, void *);

struct sunxi_musb_softc {
	struct motg_softc	sc_otg;
	struct bus_space	sc_bs;
};

CFATTACH_DECL_NEW(sunxi_musb, sizeof(struct sunxi_musb_softc),
	sunxi_musb_match, sunxi_musb_attach, NULL, NULL);

static const struct of_compat_data compat_data[] = {
	{ "allwinner,sun4i-a10-musb",		5 },
	{ "allwinner,sun6i-a13-musb",		5 },
	{ "allwinner,sun8i-h3-musb",		4 },
	{ "allwinner,sun8i-a33-musb",		5 },
	{ NULL }
};

#define	REMAPFLAG	0x8000
#define	REGDECL(a, b)	[(a)] = ((b) | REMAPFLAG)

/* Allwinner USB DRD register mappings */
static const uint16_t sunxi_musb_regmap[] = {
	REGDECL(MUSB2_REG_EPFIFO(0),	0x0000),
	REGDECL(MUSB2_REG_EPFIFO(1),	0x0004),
	REGDECL(MUSB2_REG_EPFIFO(2),	0x0008),
	REGDECL(MUSB2_REG_EPFIFO(3),	0x000c),
	REGDECL(MUSB2_REG_EPFIFO(4),	0x0010),
	REGDECL(MUSB2_REG_EPFIFO(5),	0x0014),
	REGDECL(MUSB2_REG_POWER,	0x0040),
	REGDECL(MUSB2_REG_DEVCTL,	0x0041),
	REGDECL(MUSB2_REG_EPINDEX,	0x0042),
	REGDECL(MUSB2_REG_AWIN_VEND0,	0x0043),
	REGDECL(MUSB2_REG_INTTX,	0x0044),
	REGDECL(MUSB2_REG_INTRX,	0x0046),
	REGDECL(MUSB2_REG_INTTXE,	0x0048),
	REGDECL(MUSB2_REG_INTRXE,	0x004a),
	REGDECL(MUSB2_REG_INTUSB,	0x004c),
	REGDECL(MUSB2_REG_INTUSBE,	0x0050),
	REGDECL(MUSB2_REG_FRAME,	0x0054),
	REGDECL(MUSB2_REG_TESTMODE,	0x007c),
	REGDECL(MUSB2_REG_TXMAXP,	0x0080),
	REGDECL(MUSB2_REG_TXCSRL,	0x0082),
	REGDECL(MUSB2_REG_TXCSRH,	0x0083),
	REGDECL(MUSB2_REG_RXMAXP,	0x0084),
	REGDECL(MUSB2_REG_RXCSRL,	0x0086),
	REGDECL(MUSB2_REG_RXCSRH,	0x0087),
	REGDECL(MUSB2_REG_RXCOUNT,	0x0088),
	REGDECL(MUSB2_REG_TXTI,		0x008c),
	REGDECL(MUSB2_REG_TXNAKLIMIT,	0x008d),
	REGDECL(MUSB2_REG_RXNAKLIMIT,	0x008d),
	REGDECL(MUSB2_REG_RXTI,		0x008e),
	REGDECL(MUSB2_REG_TXFIFOSZ,	0x0090),
	REGDECL(MUSB2_REG_TXFIFOADD,	0x0092),
	REGDECL(MUSB2_REG_RXFIFOSZ,	0x0094),
	REGDECL(MUSB2_REG_RXFIFOADD,	0x0096),
	REGDECL(MUSB2_REG_FADDR,	0x0098),
	REGDECL(MUSB2_REG_TXFADDR(0),	0x0098),
	REGDECL(MUSB2_REG_TXHADDR(0),	0x009a),
	REGDECL(MUSB2_REG_TXHUBPORT(0),	0x009b),
	REGDECL(MUSB2_REG_RXFADDR(0),	0x009c),
	REGDECL(MUSB2_REG_RXHADDR(0),	0x009e),
	REGDECL(MUSB2_REG_RXHUBPORT(0),	0x009f),
	REGDECL(MUSB2_REG_TXFADDR(1),	0x0098),
	REGDECL(MUSB2_REG_TXHADDR(1),	0x009a),
	REGDECL(MUSB2_REG_TXHUBPORT(1),	0x009b),
	REGDECL(MUSB2_REG_RXFADDR(1),	0x009c),
	REGDECL(MUSB2_REG_RXHADDR(1),	0x009e),
	REGDECL(MUSB2_REG_RXHUBPORT(1),	0x009f),
	REGDECL(MUSB2_REG_TXFADDR(2),	0x0098),
	REGDECL(MUSB2_REG_TXHADDR(2),	0x009a),
	REGDECL(MUSB2_REG_TXHUBPORT(2),	0x009b),
	REGDECL(MUSB2_REG_RXFADDR(2),	0x009c),
	REGDECL(MUSB2_REG_RXHADDR(2),	0x009e),
	REGDECL(MUSB2_REG_RXHUBPORT(2),	0x009f),
	REGDECL(MUSB2_REG_TXFADDR(3),	0x0098),
	REGDECL(MUSB2_REG_TXHADDR(3),	0x009a),
	REGDECL(MUSB2_REG_TXHUBPORT(3),	0x009b),
	REGDECL(MUSB2_REG_RXFADDR(3),	0x009c),
	REGDECL(MUSB2_REG_RXHADDR(3),	0x009e),
	REGDECL(MUSB2_REG_RXHUBPORT(3),	0x009f),
	REGDECL(MUSB2_REG_TXFADDR(4),	0x0098),
	REGDECL(MUSB2_REG_TXHADDR(4),	0x009a),
	REGDECL(MUSB2_REG_TXHUBPORT(4),	0x009b),
	REGDECL(MUSB2_REG_RXFADDR(4),	0x009c),
	REGDECL(MUSB2_REG_RXHADDR(4),	0x009e),
	REGDECL(MUSB2_REG_RXHUBPORT(4),	0x009f),
	REGDECL(MUSB2_REG_TXFADDR(5),	0x0098),
	REGDECL(MUSB2_REG_TXHADDR(5),	0x009a),
	REGDECL(MUSB2_REG_TXHUBPORT(5),	0x009b),
	REGDECL(MUSB2_REG_RXFADDR(5),	0x009c),
	REGDECL(MUSB2_REG_RXHADDR(5),	0x009e),
	REGDECL(MUSB2_REG_RXHUBPORT(5),	0x009f),
	REGDECL(MUSB2_REG_CONFDATA,	0x00c0),
};

static bus_size_t
sunxi_musb_reg(bus_size_t o)
{
	bus_size_t v;

	if (o >= __arraycount(sunxi_musb_regmap))
		return o;

	v = sunxi_musb_regmap[o];
	KASSERTMSG((v & REMAPFLAG) != 0, "%s: reg %#lx not in regmap",
	    __func__, o);

	return v & ~REMAPFLAG;
}

static int
sunxi_musb_filt(bus_size_t o)
{
	switch (o) {
	case MUSB2_REG_MISC:
	case MUSB2_REG_RXDBDIS:
	case MUSB2_REG_TXDBDIS:
		return 1;
	default:
		return 0;
	}
}

static uint8_t
sunxi_musb_bs_r_1(void *t, bus_space_handle_t h, bus_size_t o)
{
	switch (o) {
	case MUSB2_REG_HWVERS:
		return 0;	/* no known equivalent */
	}

	return bus_space_read_1((bus_space_tag_t)t, h, sunxi_musb_reg(o));
}

static uint16_t
sunxi_musb_bs_r_2(void *t, bus_space_handle_t h, bus_size_t o)
{
	return bus_space_read_2((bus_space_tag_t)t, h, sunxi_musb_reg(o));
}

static void
sunxi_musb_bs_w_1(void *t, bus_space_handle_t h, bus_size_t o,
    uint8_t v)
{
	if (sunxi_musb_filt(o) != 0)
		return;

	bus_space_write_1((bus_space_tag_t)t, h, sunxi_musb_reg(o), v);
}

static void
sunxi_musb_bs_w_2(void *t, bus_space_handle_t h, bus_size_t o,
    uint16_t v)
{
	if (sunxi_musb_filt(o) != 0)
		return;

	bus_space_write_2((bus_space_tag_t)t, h, sunxi_musb_reg(o), v);
}

static void
sunxi_musb_bs_rm_1(void *t, bus_space_handle_t h, bus_size_t o,
    uint8_t *d, bus_size_t c)
{
	bus_space_read_multi_1((bus_space_tag_t)t, h, sunxi_musb_reg(o), d, c);
}

static void
sunxi_musb_bs_rm_4(void *t, bus_space_handle_t h, bus_size_t o,
    uint32_t *d, bus_size_t c)
{
	bus_space_read_multi_4((bus_space_tag_t)t, h, sunxi_musb_reg(o), d, c);
}

static void
sunxi_musb_bs_wm_1(void *t, bus_space_handle_t h, bus_size_t o,
    const uint8_t *d, bus_size_t c)
{
	if (sunxi_musb_filt(o) != 0)
		return;

	bus_space_write_multi_1((bus_space_tag_t)t, h, sunxi_musb_reg(o), d, c);
}

static void
sunxi_musb_bs_wm_4(void *t, bus_space_handle_t h, bus_size_t o,
    const uint32_t *d, bus_size_t c)
{
	if (sunxi_musb_filt(o) != 0)
		return;

	bus_space_write_multi_4((bus_space_tag_t)t, h, sunxi_musb_reg(o), d, c);
}

static void
sunxi_musb_bs_barrier(void *t, bus_space_handle_t h, bus_size_t o,
    bus_size_t l, int f)
{
	bus_space_barrier((bus_space_tag_t)t, h, o, l, f);
}

static int
sunxi_musb_intr(void *priv)
{
	struct motg_softc * const sc = priv;
	uint16_t inttx, intrx;
	uint8_t intusb;

	mutex_enter(&sc->sc_intr_lock);

	intusb = bus_space_read_1(sc->sc_iot, sc->sc_ioh, MUSB2_REG_INTUSB);
	inttx = bus_space_read_2(sc->sc_iot, sc->sc_ioh, MUSB2_REG_INTTX);
	intrx = bus_space_read_2(sc->sc_iot, sc->sc_ioh, MUSB2_REG_INTRX);
	if (!intusb && !inttx && !intrx) {
		mutex_exit(&sc->sc_intr_lock);
		return 0;
	}

	if (intusb)
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, MUSB2_REG_INTUSB, intusb);
	if (inttx)
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, MUSB2_REG_INTTX, inttx);
	if (intrx)
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, MUSB2_REG_INTRX, intrx);

	motg_intr(sc, intrx, inttx, intusb);

	mutex_exit(&sc->sc_intr_lock);

	return 1;
}

static void
sunxi_musb_poll(void *priv)
{
	sunxi_musb_intr(priv);
}

static int
sunxi_musb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
sunxi_musb_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_musb_softc * const msc = device_private(self);
	struct motg_softc * const sc = &msc->sc_otg;
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct fdtbus_reset *rst;
	struct fdtbus_phy *phy;
	struct clk *clk;
	char intrstr[128];
	const char *dr_mode;
	bus_addr_t addr;
	bus_size_t size;
	void *ih;
	u_int n;

	/* Only "host" mode is supported */
	dr_mode = fdtbus_get_string(phandle, "dr_mode");
	if (dr_mode == NULL || strcmp(dr_mode, "host") != 0) {
		aprint_normal(": '%s' mode not supported\n", dr_mode);
		return;
	}

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	/* Enable clocks */
	for (n = 0; (clk = fdtbus_clock_get_index(phandle, n)) != NULL; n++)
		if (clk_enable(clk) != 0) {
			aprint_error(": couldn't enable clock #%d\n", n);
			return;
		}
	/* De-assert resets */
	for (n = 0; (rst = fdtbus_reset_get_index(phandle, n)) != NULL; n++)
		if (fdtbus_reset_deassert(rst) != 0) {
			aprint_error(": couldn't de-assert reset #%d\n", n);
			return;
		}

	/* Enable optional phy */
	phy = fdtbus_phy_get(phandle, "usb");
	if (phy && fdtbus_phy_enable(phy, true) != 0) {
		aprint_error(": couldn't enable phy\n");
		return;
	}

	/* Create custom bus space tag for remapping registers */
	msc->sc_bs.bs_cookie = faa->faa_bst;
	msc->sc_bs.bs_r_1 = sunxi_musb_bs_r_1;
	msc->sc_bs.bs_r_2 = sunxi_musb_bs_r_2;
	msc->sc_bs.bs_w_1 = sunxi_musb_bs_w_1;
	msc->sc_bs.bs_w_2 = sunxi_musb_bs_w_2;
	msc->sc_bs.bs_rm_1 = sunxi_musb_bs_rm_1;
	msc->sc_bs.bs_rm_4 = sunxi_musb_bs_rm_4;
	msc->sc_bs.bs_wm_1 = sunxi_musb_bs_wm_1;
	msc->sc_bs.bs_wm_4 = sunxi_musb_bs_wm_4;
	msc->sc_bs.bs_barrier = sunxi_musb_bs_barrier;

	sc->sc_dev = self;
	sc->sc_bus.ub_hcpriv = sc;
	sc->sc_bus.ub_dmatag = faa->faa_dmat;
	sc->sc_size = size;
	sc->sc_iot = &msc->sc_bs;
	if (bus_space_map(faa->faa_bst, addr, size, 0, &sc->sc_ioh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_intr_poll = sunxi_musb_poll;
	sc->sc_intr_poll_arg = sc;
	sc->sc_mode = MOTG_MODE_HOST;
	sc->sc_ep_max = of_search_compatible(phandle, compat_data)->data;
	sc->sc_ep_fifosize = 512;

	aprint_naive("\n");
	aprint_normal(": USB OTG\n");

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	ih = fdtbus_intr_establish(phandle, 0, IPL_USB, FDT_INTR_MPSAFE,
	    sunxi_musb_intr, sc);
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, MUSB2_REG_AWIN_VEND0, 0);

	motg_init(sc);
}
