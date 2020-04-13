/*	$NetBSD: sni_gpio.c,v 1.7.4.2 2020/04/13 08:03:37 martin Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: sni_gpio.c,v 1.7.4.2 2020/04/13 08:03:37 martin Exp $");

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
 *    DSW3-PIN1 -- what's "varstore" really this
 *    DSW3-PIN3 -- tweek PCIe bus implementation error toggle
 *    PowerButton (PWROFF#) can be detectable.
 *
 *  96board mezzanine
 *    i2c  "/i2c@51221000"
 *    spi  "/spi@54810000"
 *    gpio "/gpio@51000000" pinA-L (10-25) down edge sensitive
 */
static void snigpio_attach_i(struct snigpio_softc *);
static int snigpio_intr(void *);

static int
snigpio_fdt_match(device_t parent, struct cfdata *match, void *aux)
{
	static const char * compatible[] = {
		"socionext,synquacer-gpio",
		"fujitsu,mb86s70-gpio",
		NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
snigpio_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct snigpio_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	prop_dictionary_t dict = device_properties(self);
	const int phandle = faa->faa_phandle;
	bus_space_handle_t ioh;
	bus_addr_t addr;
	bus_size_t size;
	char intrstr[128];
	const char *list;
	_Bool disable;

	prop_dictionary_get_bool(dict, "disable", &disable);
	if (disable) {
		aprint_naive(": disabled\n");
		aprint_normal(": disabled\n");
		return;
	}
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0
	    || bus_space_map(faa->faa_bst, addr, size, 0, &ioh) != 0) {
		aprint_error(": unable to map device\n");
		return;
	}
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		goto fail;
	}
	sc->sc_ih = fdtbus_intr_establish(phandle,
			0, IPL_VM, 0, snigpio_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		goto fail;
	}

	aprint_naive("\n");
	aprint_normal_dev(self, "Socionext GPIO controller\n");
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);
	list = fdtbus_get_string(phandle, "gpio-line-names");
	if (list)
		aprint_normal_dev(self, "%s\n", list);

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_iot = faa->faa_bst;
	sc->sc_ioh = ioh;
	sc->sc_iob = addr;
	sc->sc_ios = size;

	snigpio_attach_i(sc);

	return;
 fail:
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
	return;	
}

static int
snigpio_acpi_match(device_t parent, struct cfdata *match, void *aux)
{
	static const char * compatible[] = {
		"SCX0007",
		NULL
	};
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;
	return acpi_match_hid(aa->aa_node->ad_devinfo, compatible);
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
	struct acpi_irq *irq;
	ACPI_STATUS rv;
	char *list;

	rv = acpi_resource_parse(self, aa->aa_node->ad_handle, "_CRS",
	    &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv))
		return;
	mem = acpi_res_mem(&res, 0);
	irq = acpi_res_irq(&res, 0);
	if (mem == NULL || irq == NULL || mem->ar_length == 0) {
		aprint_error(": incomplete resources\n");
		return;
	}
	if (bus_space_map(aa->aa_memt, mem->ar_base, mem->ar_length, 0,
	    &ioh)) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_ih = acpi_intr_establish(self,
	    (uint64_t)(uintptr_t)aa->aa_node->ad_handle,
	    IPL_VM, false, snigpio_intr, sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		goto fail;
	}

	aprint_normal_dev(self, "Socionext GPIO controller\n");
	rv = acpi_dsd_string(handle, "gpio-line-names", &list);
	if (ACPI_SUCCESS(rv))
		aprint_normal_dev(self, "%s\n", list);

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_memt;
	sc->sc_ioh = ioh;
	sc->sc_ios = mem->ar_length;

	snigpio_attach_i(sc);

	acpi_resource_cleanup(&res);
	return;
 fail:
	acpi_resource_cleanup(&res);
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
	return;	
}

static void
snigpio_attach_i(struct snigpio_softc *sc)
{
	struct gpio_chipset_tag	*gc;
	struct gpiobus_attach_args gba;

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

	(void) config_found_ia(sc->sc_dev, "gpiobus", &gba, gpiobus_print);
}

static int
snigpio_intr(void *arg)
{
	return 1;
}
