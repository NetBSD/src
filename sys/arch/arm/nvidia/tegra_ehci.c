/* $NetBSD: tegra_ehci.c,v 1.1.2.5 2016/02/16 21:26:37 skrll Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_ehci.c,v 1.1.2.5 2016/02/16 21:26:37 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_var.h>
#include <arm/nvidia/tegra_usbreg.h>

#include <dev/fdt/fdtvar.h>

#define TEGRA_EHCI_REG_OFFSET	0x100

static int	tegra_ehci_match(device_t, cfdata_t, void *);
static void	tegra_ehci_attach(device_t, device_t, void *);

static void	tegra_ehci_init(struct ehci_softc *);

struct tegra_ehci_softc {
	struct ehci_softc	sc;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	void			*sc_ih;
};

static int	tegra_ehci_port_status(struct ehci_softc *sc, uint32_t v,
		    int i);

CFATTACH_DECL2_NEW(tegra_ehci, sizeof(struct tegra_ehci_softc),
	tegra_ehci_match, tegra_ehci_attach, NULL,
	ehci_activate, NULL, ehci_childdet);

static int
tegra_ehci_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "nvidia,tegra124-ehci", NULL };
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
tegra_ehci_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_ehci_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map USB\n");
		return;
	}

	sc->sc.sc_dev = self;
	sc->sc.sc_bus.ub_hcpriv = &sc->sc;
	sc->sc.sc_bus.ub_dmatag = faa->faa_dmat;
	sc->sc.sc_bus.ub_revision = USBREV_2_0;
	sc->sc.sc_ncomp = 0;
	sc->sc.sc_flags = EHCIF_ETTF;
	sc->sc.sc_id_vendor = 0x10de;
	strlcpy(sc->sc.sc_vendor, "Tegra", sizeof(sc->sc.sc_vendor));
	sc->sc.sc_size = size - TEGRA_EHCI_REG_OFFSET;
	sc->sc.iot = sc->sc_bst;
	bus_space_subregion(sc->sc_bst, sc->sc_bsh, TEGRA_EHCI_REG_OFFSET,
	    sc->sc.sc_size, &sc->sc.ioh);
	sc->sc.sc_vendor_init = tegra_ehci_init;
	sc->sc.sc_vendor_port_status = tegra_ehci_port_status;

	aprint_naive("\n");
	aprint_normal(": USB\n");

	sc->sc.sc_offs = EREAD1(&sc->sc, EHCI_CAPLENGTH);

	if (!fdtbus_intr_str(faa->faa_phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish(faa->faa_phandle, 0, IPL_USB, IST_MPSAFE,
	    ehci_intr, &sc->sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	error = ehci_init(&sc->sc);
	if (error) {
		aprint_error_dev(self, "init failed, error = %d\n", error);
		return;
	}

	sc->sc.sc_child = config_found(self, &sc->sc.sc_bus, usbctlprint);
}

static void
tegra_ehci_init(struct ehci_softc *esc)
{
	struct tegra_ehci_softc * const sc = device_private(esc->sc_dev);
	uint32_t usbmode;

	usbmode = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
	    TEGRA_EHCI_USBMODE_REG);

	const u_int cm = __SHIFTOUT(usbmode, TEGRA_EHCI_USBMODE_CM);
	if (cm != TEGRA_EHCI_USBMODE_CM_HOST) {
		aprint_verbose_dev(esc->sc_dev, "switching to host mode\n");
		usbmode &= ~TEGRA_EHCI_USBMODE_CM;
		usbmode |= __SHIFTIN(TEGRA_EHCI_USBMODE_CM_HOST,
				     TEGRA_EHCI_USBMODE_CM);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh,
		    TEGRA_EHCI_USBMODE_REG, usbmode);
	}

	/* Parallel transceiver select */
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh,
	    TEGRA_EHCI_HOSTPC1_DEVLC_REG,
	    __SHIFTIN(TEGRA_EHCI_HOSTPC1_DEVLC_PTS_UTMI,
		      TEGRA_EHCI_HOSTPC1_DEVLC_PTS),
	    TEGRA_EHCI_HOSTPC1_DEVLC_PTS |
	    TEGRA_EHCI_HOSTPC1_DEVLC_STS);

	bus_space_write_4(sc->sc_bst, sc->sc_bsh, TEGRA_EHCI_TXFILLTUNING_REG,
	    __SHIFTIN(0x10, TEGRA_EHCI_TXFILLTUNING_TXFIFOTHRES));
}

static int
tegra_ehci_port_status(struct ehci_softc *ehci_sc, uint32_t v, int i)
{
	struct tegra_ehci_softc * const sc = device_private(ehci_sc->sc_dev);
	bus_space_tag_t iot = sc->sc_bst;
	bus_space_handle_t ioh = sc->sc_bsh;

	i &= ~(UPS_HIGH_SPEED|UPS_LOW_SPEED);

	uint32_t val = bus_space_read_4(iot, ioh,
	    TEGRA_EHCI_HOSTPC1_DEVLC_REG);

	switch (__SHIFTOUT(val, TEGRA_EHCI_HOSTPC1_DEVLC_PSPD)) {
	case TEGRA_EHCI_HOSTPC1_DEVLC_PSPD_FS:
		i |= UPS_FULL_SPEED;
		break;
	case TEGRA_EHCI_HOSTPC1_DEVLC_PSPD_LS:
		i |= UPS_LOW_SPEED;
		break;
	case TEGRA_EHCI_HOSTPC1_DEVLC_PSPD_HS:
	default:
		i |= UPS_HIGH_SPEED;
		break;
	}
	return i;
}
