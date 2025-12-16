/* $NetBSD: ti_usb.c,v 1.3 2025/12/16 12:20:23 skrll Exp $ */

/*-
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: ti_usb.c,v 1.3 2025/12/16 12:20:23 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/mutex.h>
#include <sys/bus.h>

#include <dev/fdt/fdtvar.h>

#include <arm/ti/ti_prcm.h>

#define UHH_REVISION				0x00
#define  UHH_REVISION1		0x00000010	/* OMAP3 */
#define  UHH_REVISION2		0x50700100	/* OMAP4 */


#define	UHH_SYSCONFIG				0x10
/* OMAP4 definitions */
#define  UHH_SYSCONFIG_IDLEMODE_MASK		__BITS(3, 2)
#define  UHH_SYSCONFIG_IDLEMODE_NOIDLE		1
#define  UHH_SYSCONFIG_STANDBYMODE_MASK		__BITS(5, 4)
#define  UHH_SYSCONFIG_STANDBYMODE_NOSTDBY      1
/* OMAP definitions */
#define	 UHH_SYSCONFIG_MIDLEMODE_MASK		__BITS(13,12)
#define   UHH_SYSCONFIG_MIDLEMODE_SMARTSTANDBY	__BIT(12)
#define	 UHH_SYSCONFIG_CLOCKACTIVITY		__BIT(8)
#define	 UHH_SYSCONFIG_SIDLEMODE_MASK		__BITS(4,3)
#define   UHH_SYSCONFIG_SIDLEMODE_SMARTIDLE	__BIT(3)
#define	 UHH_SYSCONFIG_ENAWAKEUP		__BIT(2)
#define	 UHH_SYSCONFIG_SOFTRESET		__BIT(1)
#define	 UHH_SYSCONFIG_AUTOIDLE			__BIT(0)

#define	UHH_HOSTCONFIG				0x40
#define  UHH_HOSTCONFIG_APP_START_CLK		__BIT(31)
/* OMAP4 definitions */
#define  UHH_HOSTCONFIG_PN_MODE_MASK(p)		__BITS(1 + 16 + 2 * (p), 16 + 2 * (p))
#define   UHH_HOSTCONFIG_PMODE_ULPI_PHY		0
#define   UHH_HOSTCONFIG_PMODE_TLL		1
#define   UHH_HOSTCONFIG_PMODE_HSIC		3
/* OMAP definitions */
#define	 UHH_HOSTCONFIG_P3_ULPI_BYPASS		__BIT(12)
#define	 UHH_HOSTCONFIG_P2_ULPI_BYPASS		__BIT(11)
#define	 UHH_HOSTCONFIG_P3_CONNECT_STATUS	__BIT(10)
#define	 UHH_HOSTCONFIG_P2_CONNECT_STATUS	__BIT(9)
#define	 UHH_HOSTCONFIG_P1_CONNECT_STATUS	__BIT(8)
#define	 UHH_HOSTCONFIG_ENA_INCR_ALIGN		__BIT(5)
#define	 UHH_HOSTCONFIG_ENA_INCR16		__BIT(4)
#define	 UHH_HOSTCONFIG_ENA_INCR8		__BIT(3)
#define	 UHH_HOSTCONFIG_ENA_INCR4		__BIT(2)
#define	 UHH_HOSTCONFIG_AUTOPPD_ON_OVERCUR_EN	__BIT(1)
#define	 UHH_HOSTCONFIG_P1_ULPI_BYPASS		__BIT(0)

extern void tl_usbtll_enable_port(u_int);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,usbhs-host" },
	DEVICE_COMPAT_EOL
};

/* Maximum number of ports */
#define	TI_USB_NPORTS	3

enum {
	CONNECT_STATUS,
	ULPI_BYPASS,
	TI_USB_NBITS
};

static const uint32_t ti_usb_portbits[TI_USB_NPORTS][TI_USB_NBITS] = {
	[0] = {
		[CONNECT_STATUS] = UHH_HOSTCONFIG_P1_CONNECT_STATUS,
		[ULPI_BYPASS] = UHH_HOSTCONFIG_P1_ULPI_BYPASS,
	},
	[1] = {
		[CONNECT_STATUS] = UHH_HOSTCONFIG_P2_CONNECT_STATUS,
		[ULPI_BYPASS] = UHH_HOSTCONFIG_P2_ULPI_BYPASS,
	},
	[2] = {
		[CONNECT_STATUS] = UHH_HOSTCONFIG_P3_CONNECT_STATUS,
		[ULPI_BYPASS] = UHH_HOSTCONFIG_P3_ULPI_BYPASS,
	},
};

enum ti_usb_portmode_type {
	PORT_UNUSED,
	PORT_EHCI_PHY,
	PORT_EHCI_TLL,
	PORT_EHCI_HSIC,
	PORT_OHCI,
};

struct ti_usb_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;

	uint32_t sc_usbhsrev;
	u_int sc_nports;

	u_int sc_portmode[TI_USB_NPORTS];
};

static int	ti_usb_match(device_t, cfdata_t, void *);
static void	ti_usb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ti_usb, sizeof(struct ti_usb_softc),
    ti_usb_match, ti_usb_attach, NULL, NULL);

#define RD4(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define WR4(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))


static uint32_t
ti_usb_hostconfig1(struct ti_usb_softc *sc, uint32_t val)
{
	for (u_int port = 0; port < sc->sc_nports; port++) {
		if (sc->sc_portmode[port] == PORT_UNUSED)
			val &= ~ti_usb_portbits[port][CONNECT_STATUS];
		if (sc->sc_portmode[port] == PORT_EHCI_PHY)
			val &= ~ti_usb_portbits[port][ULPI_BYPASS];
		else
			val |= ti_usb_portbits[port][ULPI_BYPASS];
	}
	return val;
}

static uint32_t
ti_usb_hostconfig2(struct ti_usb_softc *sc, uint32_t val)
{
	for (u_int port = 0; port < sc->sc_nports; port++) {
		uint32_t mode = 0;

		if (sc->sc_portmode[port] == PORT_UNUSED)
			val &= ~UHH_HOSTCONFIG_PN_MODE_MASK(port);
		if (sc->sc_portmode[port] == PORT_EHCI_TLL ||
		    sc->sc_portmode[port] == PORT_OHCI)
			mode = UHH_HOSTCONFIG_PMODE_TLL;
		else if (sc->sc_portmode[port] == PORT_EHCI_HSIC)
			mode = UHH_HOSTCONFIG_PMODE_HSIC;
		val |= __SHIFTIN(mode, UHH_HOSTCONFIG_PN_MODE_MASK(port));
	}
	return val;
}


static void
ti_usb_init(struct ti_usb_softc *sc)
{
	uint32_t val;

	val = RD4(sc, UHH_SYSCONFIG);
	switch (sc->sc_usbhsrev) {
	case UHH_REVISION1:
		val &= ~UHH_SYSCONFIG_SIDLEMODE_MASK;
		val &= ~UHH_SYSCONFIG_MIDLEMODE_MASK;
		val |=  UHH_SYSCONFIG_MIDLEMODE_SMARTSTANDBY;
		val |=  UHH_SYSCONFIG_CLOCKACTIVITY;
		val |=  UHH_SYSCONFIG_SIDLEMODE_SMARTIDLE;
		val |=  UHH_SYSCONFIG_ENAWAKEUP;
		val &= ~UHH_SYSCONFIG_AUTOIDLE;
		break;
	case UHH_REVISION2:
		val &= ~UHH_SYSCONFIG_IDLEMODE_MASK;
		val |= __SHIFTIN(UHH_SYSCONFIG_IDLEMODE_NOIDLE, UHH_SYSCONFIG_IDLEMODE_MASK);
		val &= ~UHH_SYSCONFIG_STANDBYMODE_MASK;
		val |= __SHIFTIN(UHH_SYSCONFIG_STANDBYMODE_NOSTDBY, UHH_SYSCONFIG_STANDBYMODE_MASK);
		break;
	}
	WR4(sc, UHH_SYSCONFIG, val);

	val = RD4(sc, UHH_HOSTCONFIG);
	val |= UHH_HOSTCONFIG_ENA_INCR16;
	val |= UHH_HOSTCONFIG_ENA_INCR8;
	val |= UHH_HOSTCONFIG_ENA_INCR4;
	val |= UHH_HOSTCONFIG_APP_START_CLK;
	val &= ~UHH_HOSTCONFIG_ENA_INCR_ALIGN;

	switch (sc->sc_usbhsrev) {
	case UHH_REVISION1:
		val = ti_usb_hostconfig1(sc, val);
		break;
	case UHH_REVISION2:
	default:
		val = ti_usb_hostconfig2(sc, val);
	}

	WR4(sc, UHH_HOSTCONFIG, val);
}

static int
ti_usb_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

struct ti_usb_portmode {
	const char *			tup_name;
	enum ti_usb_portmode_type	tup_type;
} ti_usb_portmodes[] = {
	{ "ehci-phy", PORT_EHCI_PHY },
	{ "ehci-tll", PORT_EHCI_TLL },
	{ "ehci-hsic", PORT_EHCI_HSIC },
	{ "ohci-phy-6pin-datse0", PORT_OHCI, },
	{ "ohci-phy-6pin-dpdm", PORT_OHCI, },
	{ "ohci-phy-3pin-datse0", PORT_OHCI, },
	{ "ohci-phy-4pin-dpdm", PORT_OHCI, },
	{ "ohci-tll-6pin-datse0", PORT_OHCI, },
	{ "ohci-tll-6pin-dpdm", PORT_OHCI, },
	{ "ohci-tll-3pin-datse0", PORT_OHCI, },
	{ "ohci-tll-4pin-dpdm", PORT_OHCI, },
	{ "ohci-tll-2pin-datse0", PORT_OHCI, },
	{ "ohci-tll-2pin-dpdm", PORT_OHCI, },
};

static void
ti_usb_attach(device_t parent, device_t self, void *aux)
{
	struct ti_usb_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	char propname[16];
	const char *portmode;
	int port;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (ti_prcm_enable_hwmod(phandle, 0) != 0) {
		aprint_error(": couldn't enable module\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	sc->sc_usbhsrev = RD4(sc, UHH_REVISION);
	sc->sc_nports = TI_USB_NPORTS;
	switch (sc->sc_usbhsrev) {
	case UHH_REVISION1:
		sc->sc_nports = 3;
		break;
	case UHH_REVISION2:
		sc->sc_nports = 2;
		break;
	}

	for (port = 0; port < sc->sc_nports; port++) {
		snprintf(propname, sizeof(propname), "port%d-mode", port + 1);
		portmode = fdtbus_get_string(phandle, propname);
		if (portmode == NULL)
			continue;
		for (u_int i = 0; i < __arraycount(ti_usb_portmodes); i++) {
			if (strcmp(portmode, ti_usb_portmodes[i].tup_name) == 0)
				sc->sc_portmode[port] =
				    ti_usb_portmodes[i].tup_type;
		}

		if (sc->sc_portmode[port] != PORT_UNUSED)
			tl_usbtll_enable_port(port);
	}

	aprint_naive("\n");
	aprint_normal(": OMAP HS USB Host (ports %u)\n", sc->sc_nports);
	aprint_verbose_dev(sc->sc_dev, "revision %x\n", sc->sc_usbhsrev);

	ti_usb_init(sc);

	fdt_add_bus(self, phandle, faa);
}
