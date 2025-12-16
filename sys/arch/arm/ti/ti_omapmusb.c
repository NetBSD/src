/* $NetBSD: ti_omapmusb.c,v 1.1 2025/12/16 12:20:22 skrll Exp $ */

/*-
 * Copyright (c) 2025 Rui-Xiang Guo
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: ti_omapmusb.c,v 1.1 2025/12/16 12:20:22 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/mutex.h>
#include <sys/bus.h>
#include <sys/pool.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/motgvar.h>
#include <dev/usb/motgreg.h>

#include <dev/fdt/fdtvar.h>

#include <arm/ti/ti_prcm.h>

#define	OTG_SYSCONFIG	0x404
#define	 OTG_SYSCONFIG_MIDLEMODE	__BITS(13,12)
#define	 OTG_SYSCONFIG_SIDLEMODE	__BITS(4,3)
#define	 OTG_SYSCONFIG_ENAWAKEUP	__BIT(2)
#define	 OTG_SYSCONFIG_SOFTRESET	__BIT(1)
#define	 OTG_SYSCONFIG_AUTOIDLE		__BIT(0)

#define	OTG_SYSSTATUS	0x408
#define	 OTG_SYSSTATUS_RESETDONE	__BIT(0)

#define	IDLEMODE_FORCE		0x0
#define	IDLEMODE_NO		0x1
#define	IDLEMODE_SMART		0x2
#define	IDLEMODE_SMART_WKUP	0x3

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,omap4-musb" },
	DEVICE_COMPAT_EOL
};

static int omapmusb_match(device_t, cfdata_t, void *);
static void omapmusb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(omapmusb, sizeof(struct motg_softc),
    omapmusb_match, omapmusb_attach, NULL, NULL);

#define RD1(sc, reg) \
	bus_space_read_1((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define WR1(sc, reg, val) \
	bus_space_write_1((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))
#define RD2(sc, reg) \
	bus_space_read_2((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define WR2(sc, reg, val) \
	bus_space_write_2((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))
#define RD4(sc, reg) \
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define WR4(sc, reg, val) \
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))

static int
omapmusb_intr(void *priv)
{
	struct motg_softc * const sc = priv;
	uint16_t inttx, intrx;
	uint8_t intusb;
	int ret;

	mutex_enter(&sc->sc_intr_lock);

	intusb = RD1(sc, MUSB2_REG_INTUSB);
	inttx = RD2(sc, MUSB2_REG_INTTX);
	intrx = RD2(sc, MUSB2_REG_INTRX);
	if (!intusb && !inttx && !intrx) {
		mutex_exit(&sc->sc_intr_lock);
		return 0;
	}

	if (intusb)
		WR1(sc, MUSB2_REG_INTUSB, intusb);
	if (inttx)
		WR2(sc, MUSB2_REG_INTTX, inttx);
	if (intrx)
		WR2(sc, MUSB2_REG_INTRX, intrx);

	ret = motg_intr(sc, intrx, inttx, intusb);

	mutex_exit(&sc->sc_intr_lock);

	return ret;
}

static void
omapmusb_poll(void *priv)
{
	omapmusb_intr(priv);
}

static void
omapmusb_reset(struct motg_softc *sc)
{
	uint32_t val;
	int retry = 5000;

	WR4(sc, OTG_SYSCONFIG, OTG_SYSCONFIG_SOFTRESET);
	do {
		val = RD4(sc, OTG_SYSSTATUS);
		if (val & OTG_SYSSTATUS_RESETDONE)
			break;
		delay(10);
	} while (--retry > 0);
	if (retry == 0)
		aprint_error_dev(sc->sc_dev, "reset timeout\n");
}

static void
omapmusb_init(struct motg_softc *sc)
{
	uint32_t val;

	omapmusb_reset(sc);

	val = __SHIFTIN(IDLEMODE_SMART, OTG_SYSCONFIG_MIDLEMODE) |
	      __SHIFTIN(IDLEMODE_SMART, OTG_SYSCONFIG_SIDLEMODE) |
	      OTG_SYSCONFIG_AUTOIDLE;
	WR4(sc, OTG_SYSCONFIG, val);

	motg_init(sc);
}

static int
omapmusb_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
omapmusb_attach(device_t parent, device_t self, void *aux)
{
	struct motg_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct fdtbus_phy *phy;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	uint32_t ep_max;
	void *ih;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (ti_prcm_enable_hwmod(phandle, 0) != 0) {
		aprint_error(": couldn't enable module\n");
		return;
	}

	if (of_getprop_uint32(phandle, "num-eps", &ep_max))
		ep_max = MOTG_MAX_HW_EP;

	phy = fdtbus_phy_get(phandle, "usb2-phy");
	if (phy && fdtbus_phy_enable(phy, true) != 0) {
		aprint_error(": couldn't enable phy\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bus.ub_hcpriv = sc;
	sc->sc_bus.ub_dmatag = faa->faa_dmat;
	sc->sc_size = size;
	sc->sc_iot = faa->faa_bst;
	if (bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_intr_poll = omapmusb_poll;
	sc->sc_intr_poll_arg = sc;
	sc->sc_mode = MOTG_MODE_HOST;
	sc->sc_ep_max = ep_max;
	sc->sc_ep_fifosize = 512;

	aprint_naive("\n");
	aprint_normal(": USB OTG\n");

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	ih = fdtbus_intr_establish_xname(phandle, 0, IPL_USB, FDT_INTR_MPSAFE,
	    omapmusb_intr, sc, device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	omapmusb_init(sc);
}
