/*	$NetBSD: sni_gpio.c,v 1.13 2022/01/25 10:38:56 nisimura Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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
 * Socionext SC2A11 SynQuacer GPIO driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sni_gpio.c,v 1.13 2022/01/25 10:38:56 nisimura Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/gpio.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <machine/endian.h>
#include <sys/bus.h>
#include <sys/intr.h>

#include <dev/gpio/gpiovar.h>
#include <dev/fdt/fdtvar.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_intr.h>

static int snigpio_fdt_match(device_t, struct cfdata *, void *);
static void snigpio_fdt_attach(device_t, device_t, void *);
static int snigpio_acpi_match(device_t, struct cfdata *, void *);
static void snigpio_acpi_attach(device_t, device_t, void *);

struct snigpio_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_addr_t		sc_iob;
	bus_size_t		sc_ios;
	void			*sc_ih;
	kmutex_t		sc_lock;
	struct gpio_chipset_tag	sc_gpio_gc;
	gpio_pin_t		sc_gpio_pins[32];
	int			sc_maxpins;
	int			sc_phandle;
};

CFATTACH_DECL_NEW(snigpio_fdt, sizeof(struct snigpio_softc),
    snigpio_fdt_match, snigpio_fdt_attach, NULL, NULL);

CFATTACH_DECL_NEW(snigpio_acpi, sizeof(struct snigpio_softc),
    snigpio_acpi_match, snigpio_acpi_attach, NULL, NULL);

/*
 * "DevelopmentBox" implementation
 *    DSW3-PIN1,  DSW3-PIN2,  DSW3-PIN3,    DSW3-PIN4,
 *    DSW3-PIN5,  DSW3-PIN6,  DSW3-PIN7,    DSW3-PIN8,
 *    PSIN#,      PWROFF#,    GPIO-A,       GPIO-B,
 *    GPIO-C,     GPIO-D,     PCIE1EXTINT,  PCIE0EXTINT,
 *    PHY2-INT#,  PHY1-INT#,  GPIO-E,       GPIO-F,
 *    GPIO-G,     GPIO-H,     GPIO-I,       GPIO-J,
 *    GPIO-K,     GPIO-L,     PEC-PD26,     PEC-PD27,
 *    PEC-PD28,   PEC-PD29,   PEC-PD30,     PEC-PD31
 *
 *    DSW3-PIN1 -- erase NOR "UEFI variable store" region
 *    DSW3-PIN3 -- tweek PCIe bus implementation error toggle
 *    PowerButton (PWROFF#) can be detectable.
 *
 *  DevelopmentBox has 96board mezzanine 2x 20 receptacle
 *    gpio  "/gpio@51000000" pinA-L (23-34) down edge sensitive
 *    i2c   "/i2c1@51221000"
 *    spi   "/spi1@54810000"
 *    uart0 "/uart@2a400000" pin3,5,7,9 for real S2CA11 console
 *    uart1 SCP secure co-prorcessor uart console in pin11,13
 */
static void snigpio_attach_i(struct snigpio_softc *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "socionext,synquacer-gpio" },
	{ .compat = "fujitsu,mb86s70-gpio" },
	DEVICE_COMPAT_EOL
};
static const struct device_compatible_entry compatible[] = {
	{ .compat = "SCX0007" },
	DEVICE_COMPAT_EOL
};

static int
snigpio_fdt_match(device_t parent, struct cfdata *match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
snigpio_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct snigpio_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_space_handle_t ioh;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0
	    || bus_space_map(faa->faa_bst, addr, size, 0, &ioh) != 0) {
		aprint_error_dev(self, "unable to map device\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;
	sc->sc_ioh = ioh;
	sc->sc_iob = addr;
	sc->sc_ios = size;
	sc->sc_phandle = phandle;
	/* could use FDI "gpio-line-names" array via device_set_handle() */

	snigpio_attach_i(sc);

	return;
}

static int
snigpio_acpi_match(device_t parent, struct cfdata *match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	return acpi_compatible_match(aa, compatible);
}

static void
snigpio_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct snigpio_softc * const sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	ACPI_HANDLE handle = aa->aa_node->ad_handle;
	bus_space_handle_t ioh;
	struct acpi_resources res;
	struct acpi_mem *mem;
	ACPI_STATUS rv;
	char *list;

	rv = acpi_resource_parse(self, aa->aa_node->ad_handle, "_CRS",
	    &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv))
		return;

	mem = acpi_res_mem(&res, 0);
	if (mem == NULL || mem->ar_length == 0) {
		aprint_error_dev(self, "incomplete resources\n");
		return;
	}
	if (bus_space_map(aa->aa_memt, mem->ar_base, mem->ar_length, 0,
	    &ioh)) {
		aprint_error_dev(self, "couldn't map registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_memt;
	sc->sc_ioh = ioh;
	sc->sc_ios = mem->ar_length;
	sc->sc_phandle = 0;
	/* UEFI provides "gpio-line-names" for us */

	aprint_normal("%s", device_xname(self));
	snigpio_attach_i(sc);

	acpi_resource_cleanup(&res);
	return;
}

static void
snigpio_attach_i(struct snigpio_softc *sc)
{
	struct gpio_chipset_tag	*gc;
	struct gpiobus_attach_args gba;

	aprint_naive("\n");
	aprint_normal(": Socionext GPIO controller\n");

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);
	sc->sc_maxpins = 32;

	/* create controller tag */
	gc = &sc->sc_gpio_gc;
	gc->gp_cookie = sc;
	gc->gp_pin_read = NULL; /* AAA */
	gc->gp_pin_write = NULL; /* AAA */
	gc->gp_pin_ctl = NULL; /* AAA */
	gc->gp_intr_establish = NULL; /* AAA */
	gc->gp_intr_disestablish = NULL; /* AAA */
	gc->gp_intr_str = NULL; /* AAA */

	gba.gba_gc = gc;
	gba.gba_pins = &sc->sc_gpio_pins[0];
	gba.gba_npins = sc->sc_maxpins;

	config_found(sc->sc_dev, &gba, gpiobus_print, CFARGS_NONE);
}
