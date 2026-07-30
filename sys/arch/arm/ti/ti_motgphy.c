/* $NetBSD: ti_motgphy.c,v 1.3 2026/07/30 15:08:08 yurix Exp $ */
/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Yuri Honegger and Brook Milligan.
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

/*
 * Driver for the USB phy on TI AM335X SoCs.
 *
 * While we implement fdt_phy, we only have support for enabling all phys at
 * once. Enabling individual phys is possible, but it requires parsing the alias
 * name of the phy node, which is beyond ugly. Furthermore, the usb controller
 * doesn't support suspend/resume, so we only ever enable the phy. If the need
 * for a full phy implementation arises, it should be possible to extend the
 * driver without much restructuring.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ti_motgphy.c,v 1.3 2026/07/30 15:08:08 yurix Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <arm/ti/ti_otgreg.h>

#include <dev/fdt/fdtvar.h>

struct ti_motgphy_softc {
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
};

static int	ti_motgphy_match(device_t, cfdata_t, void *);
static void	ti_motgphy_attach(device_t, device_t, void *);
static void *	ti_motgphy_acquire(device_t, const void *, size_t);
static void	ti_motgphy_release(device_t, void *);
static int	ti_motgphy_enable(device_t, void *, bool);

#define	PHY_READ(sc, reg)					\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, reg)
#define	PHY_WRITE(sc, reg, val)					\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, reg, val)

CFATTACH_DECL_NEW(timotgphy, sizeof(struct ti_motgphy_softc), ti_motgphy_match,
    ti_motgphy_attach, NULL, NULL);

static const struct fdtbus_phy_controller_func ti_motgphy_funcs = {
	.acquire = &ti_motgphy_acquire,
	.release = &ti_motgphy_release,
	.enable = &ti_motgphy_enable,
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,am335x-usb-phy" },
	DEVICE_COMPAT_EOL
};

static int
ti_motgphy_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
ti_motgphy_attach(device_t parent, device_t self, void *aux)
{
	struct ti_motgphy_softc *sc = device_private(self);
	struct fdt_attach_args *const faa = aux;
	const int phandle = faa->faa_phandle;
	int ctrl_phandle;
	bus_addr_t addr;
	bus_size_t size;

	ctrl_phandle = fdtbus_get_phandle(phandle, "ti,ctrl_mod");
	if (ctrl_phandle < 0) {
		aprint_error(": failed to get usb phy ctrl module\n");
		return;
	}

	if (fdtbus_get_reg_byname(ctrl_phandle, "phy_ctrl", &addr, &size)) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_normal("\n");
	aprint_naive("\n");

	fdtbus_register_phy_controller(self, phandle, &ti_motgphy_funcs);
}

static void *
ti_motgphy_acquire(device_t self, const void *data, size_t len)
{
	KASSERT(len == 0);
	return device_private(self);
}

static void
ti_motgphy_release(device_t self, void *priv)
{
	/* do nothing */
}

static int
ti_motgphy_enable(device_t self, void *priv, bool enable)
{
	struct ti_motgphy_softc * const sc = priv;

	/*
	 * We only support enabling the phy, and even then we always enable both
	 * phys at once.
	 */
	KASSERT(enable);
	uint32_t usb_ctrl;
	for (int i = 0; i < 2; ++i) {
		usb_ctrl = PHY_READ(sc, CM_USBCTRL(i));
		usb_ctrl &= ~(CM_USBCTRL_CM_PWRDN | CM_USBCTRL_OTG_PWRDN);
		usb_ctrl |= (CM_USBCTRL_OTGVDET_EN | CM_USBCTRL_OTGSESSENDEN);
		PHY_WRITE(sc, CM_USBCTRL(i), usb_ctrl);
	}

	return 0;
}

