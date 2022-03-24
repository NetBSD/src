/* $NetBSD: igpio_acpi.c,v 1.1 2022/03/24 02:24:25 manu Exp $ */

/*-
 * Copyright (c) 2021,2022 Emmanuel Dreyfus
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
__KERNEL_RCSID(0, "$NetBSD: igpio_acpi.c,v 1.1 2022/03/24 02:24:25 manu Exp $");

#include <sys/param.h>
#include <sys/kmem.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/gpio.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_intr.h>
#include <dev/acpi/acpi_event.h>

#include <dev/gpio/gpiovar.h>
#include <dev/ic/igpiovar.h>

struct igpio_acpi_softc {
	ACPI_HANDLE		sc_handle;
	struct igpio_softc	sc_isc;
	struct acpi_event *	sc_event[8];
	int			sc_pmf;
	void			*sc_intr;
};

static int	igpio_acpi_match(device_t, cfdata_t, void *);
static void	igpio_acpi_attach(device_t, device_t, void *);
static int	igpio_acpi_detach(device_t, int);

static void	igpio_acpi_register_event(void *, struct acpi_event *, ACPI_RESOURCE_GPIO *);
static int	igpio_acpi_intr(void *);

CFATTACH_DECL_NEW(igpio_acpi, sizeof(struct igpio_acpi_softc), igpio_acpi_match, igpio_acpi_attach, igpio_acpi_detach, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "INT33B2" },	/* baytrail */
	{ .compat = "INT33C7" },	/* lynxpoint */
	{ .compat = "INT33FC" },	/* baytrail */
	{ .compat = "INT3437" },	/* lynxpoint */
	{ .compat = "INT344B" },	/* sunrisepoint */
	{ .compat = "INT3450" },	/* cannonlake */
	{ .compat = "INT3451" },	/* sunrisepoint */
	{ .compat = "INT3453" },	/* geminilake */
	{ .compat = "INT3455" },	/* icelake */
	{ .compat = "INT345D" },	/* sunrisepoint */
	{ .compat = "INT34BB" },	/* cannonlake */
	{ .compat = "INT34C4" },	/* lakefield */
	{ .compat = "INT34C5" },	/* tigerlake */
	{ .compat = "INT34C6" },	/* tigerlake */
	{ .compat = "INT34C8" },	/* jasperlake */
	{ .compat = "INT3536" },	/* lewisburg */
	{ .compat = "INTC1055" },	/* tigerlake */
	{ .compat = "INTC1056" },	/* alderlake */
	{ .compat = "INTC1057" },	/* tigerlake */
	{ .compat = "INTC1071" },	/* emmitsburg */
	{ .compat = "INTC3000" },	/* denverton */
	{ .compat = "INTC3001" },	/* cedarfork */
#ifdef notyet
	/*
	 * Complete bank setup in src/sys/dev/ic/igpioreg.h
	 * before enabling
	 */
	{ .compat = "INT3452" },	/* broxton */
	{ .compat = "INT34D1" },	/* broxton */
	{ .compat = "apollolake-pinctrl" },	/* broxton */
	{ .compat = "broxton-pinctrl" },	/* broxton */
	{ .compat = "INT33FF" },	/* cherryview */
#endif
	DEVICE_COMPAT_EOL
};

static int
igpio_acpi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct acpi_attach_args *aa = aux;

	return acpi_compatible_match(aa, compat_data);
}

static void
igpio_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct igpio_acpi_softc * const asc = device_private(self);
	struct acpi_attach_args *aa = aux;
	ACPI_DEVICE_INFO *ad = aa->aa_node->ad_devinfo;
	struct acpi_resources res;
	struct acpi_mem *mem;
	struct acpi_irq *irq;
	int nbar;
	ACPI_STATUS rv;
	int i;

	asc->sc_handle = aa->aa_node->ad_handle;

	rv = acpi_resource_parse(self, aa->aa_node->ad_handle, "_CRS",
	    &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv))
		return;

	irq = acpi_res_irq(&res, 0);
	if (irq == NULL) {
		aprint_error_dev(self, "couldn't find irq resource\n");
		goto done;
	}

	asc->sc_isc.sc_dev = self;

	asc->sc_isc.sc_bst = aa->aa_memt;
	for (nbar = 0; acpi_res_mem(&res, nbar); nbar++);
	asc->sc_isc.sc_nbar = nbar;
	asc->sc_isc.sc_base = 
	    kmem_zalloc(sizeof(*asc->sc_isc.sc_base) * nbar, KM_SLEEP);
	asc->sc_isc.sc_length = 
	    kmem_zalloc(sizeof(*asc->sc_isc.sc_length) * nbar, KM_SLEEP);
	asc->sc_isc.sc_bsh = 
	    kmem_zalloc(sizeof(*asc->sc_isc.sc_bsh) * nbar, KM_SLEEP);

	asc->sc_isc.sc_acpi_hid = ad->HardwareId.String;

	for (i = 0; i < nbar; i++) {
		mem = acpi_res_mem(&res, i);
		if (mem == NULL) {
			aprint_error_dev(self, "couldn't find mem resource\n");
			goto done;
		}

		asc->sc_isc.sc_base[i] = mem->ar_base;
		asc->sc_isc.sc_length[i] = mem->ar_length;
	}

	igpio_attach(&asc->sc_isc);
	
	/* If attachement failed */
	if (asc->sc_isc.sc_banks == NULL) {
		igpio_acpi_detach(self, 0);
		goto done;
	}

	rv = acpi_event_create_gpio(self, asc->sc_handle, igpio_acpi_register_event, asc);
	if (ACPI_FAILURE(rv)) {
		if (rv != AE_NOT_FOUND)
			aprint_error_dev(self, "failed to create events: %s\n", AcpiFormatException(rv));
		goto done;
	}

	asc->sc_intr = acpi_intr_establish(self,
	    (uint64_t)(uintptr_t)asc->sc_handle,
	    IPL_VM, false, igpio_acpi_intr, asc, device_xname(self));
	if (asc->sc_intr == NULL)
		aprint_error_dev(self, "couldn't establish interrupt\n");

done:
	acpi_resource_cleanup(&res);
	(void)pmf_device_register(self, NULL, NULL);
	asc->sc_pmf = 1;
}

static int
igpio_acpi_detach(device_t self, int flags)
{
	struct igpio_acpi_softc * const asc = device_private(self);
	struct igpio_softc * const isc = &asc->sc_isc;
	int nbar = isc->sc_nbar;

	acpi_intr_disestablish(asc->sc_intr);

	igpio_detach(&asc->sc_isc);

	if (isc->sc_base != NULL) {	
		kmem_free(isc->sc_base, sizeof(*isc->sc_base) * nbar);
		isc->sc_base = NULL;
	}
	
	if (isc->sc_length != NULL) {
		kmem_free(isc->sc_length, sizeof(*isc->sc_length) * nbar);
		isc->sc_length = NULL;
	}

	if (isc->sc_bsh != NULL) {
		kmem_free(isc->sc_bsh, sizeof(*isc->sc_bsh) * nbar);
		isc->sc_length = NULL;
	}

	if (asc->sc_pmf) {
		pmf_device_deregister(self);
		asc->sc_pmf = 0;
	}

	return 0;
}

static void
igpio_acpi_register_event(void *priv, struct acpi_event *ev, ACPI_RESOURCE_GPIO *gpio)
{
	return;
}

static int
igpio_acpi_intr(void *priv)
{
	struct igpio_acpi_softc *asc = priv;
	struct igpio_softc * const isc = &asc->sc_isc;
	int ret;	

	ret = igpio_intr(isc);
	
	return ret;
}
