/* $NetBSD: ti_usb.c,v 1.2 2021/01/27 03:10:20 thorpej Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: ti_usb.c,v 1.2 2021/01/27 03:10:20 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/mutex.h>
#include <sys/bus.h>

#include <dev/fdt/fdtvar.h>

#include <arm/ti/ti_prcm.h>

#define	UHH_SYSCONFIG				0x10
#define	 UHH_SYSCONFIG_MIDLEMODE_MASK		0x00003000
#define   UHH_SYSCONFIG_MIDLEMODE_SMARTSTANDBY	0x00002000
#define	 UHH_SYSCONFIG_CLOCKACTIVITY		0x00000100
#define	 UHH_SYSCONFIG_SIDLEMODE_MASK		0x00000018
#define   UHH_SYSCONFIG_SIDLEMODE_SMARTIDLE	0x00000008
#define	 UHH_SYSCONFIG_ENAWAKEUP		0x00000004
#define	 UHH_SYSCONFIG_SOFTRESET		0x00000002
#define	 UHH_SYSCONFIG_AUTOIDLE			0x00000001

#define	UHH_HOSTCONFIG				0x40
#define  UHH_HOSTCONFIG_APP_START_CLK		__BIT(31)
#define	 UHH_HOSTCONFIG_P3_MODE			__BITS(21,20)
#define	 UHH_HOSTCONFIG_P2_MODE			__BITS(19,18)
#define	 UHH_HOSTCONFIG_P1_MODE			__BITS(17,16)
#define   UHH_HOSTCONFIG_PMODE_ULPI_PHY		0
#define   UHH_HOSTCONFIG_PMODE_UTMI		1
#define   UHH_HOSTCONFIG_PMODE_HSIC		3
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

enum {
	PORT_UNUSED,
	PORT_EHCI_PHY,
	PORT_EHCI_TLL,
	PORT_EHCI_HSIC,
};

struct ti_usb_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;

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

static void
ti_usb_init(struct ti_usb_softc *sc)
{
	uint32_t val;
	int port;

	val = RD4(sc, UHH_SYSCONFIG);
	val &= ~(UHH_SYSCONFIG_SIDLEMODE_MASK|UHH_SYSCONFIG_MIDLEMODE_MASK);
	val |= UHH_SYSCONFIG_MIDLEMODE_SMARTSTANDBY;
	val |= UHH_SYSCONFIG_CLOCKACTIVITY;
	val |= UHH_SYSCONFIG_SIDLEMODE_SMARTIDLE;
	val |= UHH_SYSCONFIG_ENAWAKEUP;
	val &= ~UHH_SYSCONFIG_AUTOIDLE;
	WR4(sc, UHH_SYSCONFIG, val);

	val = RD4(sc, UHH_SYSCONFIG);

	val = RD4(sc, UHH_HOSTCONFIG);
	val |= UHH_HOSTCONFIG_ENA_INCR16;
	val |= UHH_HOSTCONFIG_ENA_INCR8;
	val |= UHH_HOSTCONFIG_ENA_INCR4;
	val |= UHH_HOSTCONFIG_APP_START_CLK;
	val &= ~UHH_HOSTCONFIG_ENA_INCR_ALIGN;
	for (port = 0; port < TI_USB_NPORTS; port++) {
		if (sc->sc_portmode[port] == PORT_UNUSED)
			val &= ~ti_usb_portbits[port][CONNECT_STATUS];
		if (sc->sc_portmode[port] == PORT_EHCI_PHY)
			val &= ~ti_usb_portbits[port][ULPI_BYPASS];
		else
			val |= ti_usb_portbits[port][ULPI_BYPASS];
	}
	WR4(sc, UHH_HOSTCONFIG, val);
}

static int
ti_usb_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

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

	for (port = 0; port < TI_USB_NPORTS; port++) {
		snprintf(propname, sizeof(propname), "port%d-mode", port + 1);
		portmode = fdtbus_get_string(phandle, propname);
		if (portmode == NULL)
			continue;
		if (strcmp(portmode, "ehci-phy") == 0)
			sc->sc_portmode[port] = PORT_EHCI_PHY;
		else if (strcmp(portmode, "ehci-tll") == 0)
			sc->sc_portmode[port] = PORT_EHCI_TLL;
		else if (strcmp(portmode, "ehci-hsic") == 0)
			sc->sc_portmode[port] = PORT_EHCI_HSIC;

		if (sc->sc_portmode[port] != PORT_UNUSED)
			tl_usbtll_enable_port(port);
	}

	aprint_naive("\n");
	aprint_normal(": OMAP HS USB Host\n");

	ti_usb_init(sc);

	fdt_add_bus(self, phandle, faa);
}
