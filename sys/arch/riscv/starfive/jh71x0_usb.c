/* $NetBSD: jh71x0_usb.c,v 1.3 2024/09/18 08:31:50 skrll Exp $ */

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
__KERNEL_RCSID(0, "$NetBSD: jh71x0_usb.c,v 1.3 2024/09/18 08:31:50 skrll Exp $");

#include <sys/param.h>


#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

struct jh71x0_usb_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;

	int			sc_syscon_phandle;
	const struct syscon	*sc_syscon;
};


#define RD4(sc, reg)							       \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define WR4(sc, reg, val)						       \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))


/* Register definitions */

#define JH7100_USB0			0x20
#define JH7100_USB0_MODE_STRAP_MASK	__BITS(2, 0)
#define JH7100_USB0_MODE_STRAP_HOST	2

#define JH7100_USB3			0x2c
#define JH7100_USB3_UTMI_IDDIG		__BIT(21)

#define JH7100_USB7			0x3c
#define JH7100_USB7_SSRX_SEL		__BIT(18)
#define JH7100_USB7_SSTX_SEL		__BIT(19)
#define JH7100_USB7_PLL_EN		__BIT(23)
#define JH7100_USB7_EQ_EN		__BIT(25)

#define JH7110_STRAP_HOST		__BIT(17)
#define JH7110_STRAP_DEVICE		__BIT(18)
#define JH7110_STRAP_MASK		__BITS(18, 16)

#define JH7110_SUSPENDM_HOST		__BIT(19)
#define JH7110_SUSPENDM_MASK		__BIT(19)

#define JH7110_MISC_CFG_MASK		__BITS(23, 20)
#define JH7110_SUSPENDM_BYPS		__BIT(20)
#define JH7110_PLL_EN			__BIT(22)
#define JH7110_REFCLK_MODE		__BIT(23)



enum usb_dr_mode {
	USB_DR_MODE_UNKNOWN,
	USB_DR_MODE_HOST,
	USB_DR_MODE_PERIPHERAL,
	USB_DR_MODE_OTG,
};

static inline void
jh71x0_syscon_update(struct jh71x0_usb_softc *sc, bus_size_t off,
    uint32_t clr, uint32_t set)
{
	syscon_lock(sc->sc_syscon);
	const uint32_t old = syscon_read_4(sc->sc_syscon, off);
	const uint32_t new = (old & ~clr) | set;
	if (old != new) {
		syscon_write_4(sc->sc_syscon, off, new);
	}
	syscon_unlock(sc->sc_syscon);
}

static int
jh7100_usb_init(struct jh71x0_usb_softc *sc, const u_int *syscon_data )
{
	enum usb_dr_mode mode = USB_DR_MODE_HOST;

	switch (mode) {
	case USB_DR_MODE_HOST:
		jh71x0_syscon_update(sc, JH7100_USB0,
		    JH7100_USB0_MODE_STRAP_MASK, JH7100_USB0_MODE_STRAP_HOST);
		jh71x0_syscon_update(sc, JH7100_USB7,
		    JH7100_USB7_PLL_EN, JH7100_USB7_PLL_EN);
		jh71x0_syscon_update(sc, JH7100_USB7,
		    JH7100_USB7_EQ_EN, JH7100_USB7_EQ_EN);
		jh71x0_syscon_update(sc, JH7100_USB7,
		    JH7100_USB7_SSRX_SEL, JH7100_USB7_SSRX_SEL);
		jh71x0_syscon_update(sc, JH7100_USB7,
		    JH7100_USB7_SSTX_SEL, JH7100_USB7_SSTX_SEL);
		jh71x0_syscon_update(sc, JH7100_USB3,
		    JH7100_USB3_UTMI_IDDIG, JH7100_USB3_UTMI_IDDIG);
		break;
	default:
		break;
	}

	return 0;
}

static int
jh7110_usb_init(struct jh71x0_usb_softc *sc, const u_int *syscon_data)
{
	enum usb_dr_mode mode = USB_DR_MODE_HOST;
	bus_size_t usb_mode = be32dec(&syscon_data[1]);

	jh71x0_syscon_update(sc, usb_mode, JH7110_MISC_CFG_MASK,
	    JH7110_SUSPENDM_BYPS | JH7110_PLL_EN | JH7110_REFCLK_MODE);

	switch (mode) {
	case USB_DR_MODE_HOST:
		jh71x0_syscon_update(sc, usb_mode, JH7110_STRAP_MASK, JH7110_STRAP_HOST);
		jh71x0_syscon_update(sc, usb_mode, JH7110_SUSPENDM_MASK, JH7110_SUSPENDM_HOST);
		break;

	case USB_DR_MODE_PERIPHERAL:
		jh71x0_syscon_update(sc, usb_mode, JH7110_STRAP_MASK, JH7110_STRAP_DEVICE);
		jh71x0_syscon_update(sc, usb_mode, JH7110_SUSPENDM_MASK, 0);
		break;
	default:
		break;
	}

	return 0;
}

struct jh71x0_usb_config {
	int (*jhuc_init)(struct jh71x0_usb_softc *, const u_int *);
	const char *jhuc_syscon;
	size_t jhuc_sclen;
};

struct jh71x0_usb_config jh7100_usb_data = {
	.jhuc_init = jh7100_usb_init,
	.jhuc_syscon = "starfive,syscon",
	.jhuc_sclen = 1 * sizeof(uint32_t),
};

struct jh71x0_usb_config jh7110_usb_data = {
	.jhuc_init = jh7110_usb_init,
	.jhuc_syscon = "starfive,stg-syscon",
	.jhuc_sclen = 2 * sizeof(uint32_t),
};

/* Compat string(s) */
static const struct device_compatible_entry compat_data[] = {
	{ .compat = "starfive,jh7100-usb", .data = &jh7100_usb_data },
	{ .compat = "starfive,jh7110-usb", .data = &jh7110_usb_data },
	DEVICE_COMPAT_EOL
};

static int
jh71x0_usb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
jh71x0_usb_attach(device_t parent, device_t self, void *aux)
{
	struct jh71x0_usb_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;

	const struct jh71x0_usb_config *jhuc =
	    of_compatible_lookup(sc->sc_phandle, compat_data)->data;

	int len;
	const u_int *syscon_data =
	    fdtbus_get_prop(phandle, jhuc->jhuc_syscon, &len);
	if (syscon_data == NULL) {
		aprint_error(": couldn't get '%s' property\n",
		    jhuc->jhuc_syscon);
		return;
	}
	if (len != jhuc->jhuc_sclen) {
		aprint_error(": incorrect syscon data (len = %u)\n",
		    len);
		return;
	}

	int syscon_phandle =
	    fdtbus_get_phandle_from_native(be32dec(&syscon_data[0]));

	sc->sc_syscon = fdtbus_syscon_lookup(syscon_phandle);
	if (sc->sc_syscon == NULL) {
		aprint_error(": couldn't get syscon\n");
		return;
	}

	jhuc->jhuc_init(sc, syscon_data);

	aprint_naive("\n");
	aprint_normal(": USB\n");

	for (int child = OF_child(phandle); child; child = OF_peer(child)) {
		if (!fdtbus_status_okay(child))
			continue;
		fdt_add_child(parent, child, faa, 0);
	}

	//fdtbus_register_phy_controller(self, phandle, &XXX_usbphy_funcs);

}


CFATTACH_DECL_NEW(jh71x0_usb, sizeof(struct jh71x0_usb_softc),
	jh71x0_usb_match, jh71x0_usb_attach, NULL, NULL);
