/* $NetBSD: com_acpi.c,v 1.44 2021/10/23 17:46:26 jmcneill Exp $ */

/*
 * Copyright (c) 2002 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: com_acpi.c,v 1.44 2021/10/23 17:46:26 jmcneill Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/termios.h>

#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_intr.h>

#include <dev/ic/comvar.h>

static int	com_acpi_match(device_t, cfdata_t , void *);
static void	com_acpi_attach(device_t, device_t, void *);

struct com_acpi_softc {
	struct com_softc sc_com;
	void *sc_ih;
};

CFATTACH_DECL_NEW(com_acpi, sizeof(struct com_acpi_softc), com_acpi_match,
    com_acpi_attach, NULL, NULL);

/*
 * Supported device IDs
 */

static const struct device_compatible_entry compat_data[] = {
	/* Standard PC COM port */
	{ .compat = "PNP0500",		.value = COM_TYPE_NORMAL },

	/* 16550A-compatible COM port */
	{ .compat = "PNP0501",		.value = COM_TYPE_NORMAL },

	/* Generic IRDA-compatible device */
	{ .compat = "PNP0510",		.value = COM_TYPE_NORMAL },

	/* Generic IRDA-compatible device */
	{ .compat = "PNP0511",		.value = COM_TYPE_NORMAL },

	/* IBM ThinkPad IRDA device */
	{ .compat = "IBM0071",		.value = COM_TYPE_NORMAL },

	/* SMC SuperIO IRDA device */
	{ .compat = "SMCF010",		.value = COM_TYPE_NORMAL },

	/* NSC IRDA device */
	{ .compat = "NSC6001",		.value = COM_TYPE_NORMAL },

	/* Fujitsu Serial Pen Tablet */
	{ .compat = "FUJ02E6",		.value = COM_TYPE_NORMAL },

	/* Hisilicon UART */
	{ .compat = "HISI0031",		.value = COM_TYPE_DW_APB },

	/* Designware APB UART */
	{ .compat = "8250dw",		.value = COM_TYPE_DW_APB },

	DEVICE_COMPAT_EOL
};

/*
 * com_acpi_match: autoconf(9) match routine
 */
static int
com_acpi_match(device_t parent, cfdata_t match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	return acpi_compatible_match(aa, compat_data);
}

/*
 * com_acpi_attach: autoconf(9) attach routine
 */
static void
com_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct com_acpi_softc *asc = device_private(self);
	struct com_softc *sc = &asc->sc_com;
	struct acpi_attach_args *aa = aux;
	const struct device_compatible_entry *dce;
	struct acpi_resources res;
	struct acpi_io *io;
	struct acpi_mem *mem;
	struct acpi_irq *irq;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_addr_t base;
	bus_size_t size;
	ACPI_STATUS rv;
	ACPI_INTEGER clock_freq;
	ACPI_INTEGER reg_shift;

	sc->sc_dev = self;

	/* parse resources */
	rv = acpi_resource_parse(sc->sc_dev, aa->aa_node->ad_handle, "_CRS",
	    &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv))
		return;

	/* find our i/o registers */
	io = acpi_res_io(&res, 0);
	if (io != NULL) {
		iot = aa->aa_iot;
		base = io->ar_base;
		size = io->ar_length;
	} else {
		mem = acpi_res_mem(&res, 0);
		if (mem != NULL) {
			iot = aa->aa_memt;
			base = mem->ar_base;
			size = mem->ar_length;
		} else {
			aprint_error_dev(self, "unable to find i/o register "
			    "and memory resource\n");
			goto out;
		}
	}

	/* find our IRQ */
	irq = acpi_res_irq(&res, 0);
	if (irq == NULL) {
		sc->sc_poll_ticks = 1;
	}

	if (!com_is_console(iot, base, &ioh))
		if (bus_space_map(iot, base, size, 0, &ioh)) {
			aprint_error_dev(self, "can't map i/o space\n");
			goto out;
		}

	rv = acpi_dsd_integer(aa->aa_node->ad_handle, "reg-shift",
	    &reg_shift);
	if (ACPI_FAILURE(rv)) {
		reg_shift = 0;
	}

	com_init_regs_stride(&sc->sc_regs, iot, ioh, base, reg_shift);

	aprint_normal("%s", device_xname(self));

	dce = acpi_compatible_lookup(aa, compat_data);
	KASSERT(dce != NULL);

	sc->sc_type = dce->value;

	if (com_probe_subr(&sc->sc_regs) == 0) {
		aprint_error(": com probe failed\n");
		goto out;
	}

	rv = acpi_dsd_integer(aa->aa_node->ad_handle, "clock-frequency",
	    &clock_freq);
	if (ACPI_SUCCESS(rv))
		sc->sc_frequency = clock_freq;
	else
		sc->sc_frequency = 115200 * 16;

	com_attach_subr(sc);

	if (sc->sc_poll_ticks == 0) {
		asc->sc_ih = acpi_intr_establish(self,
		    (uint64_t)(uintptr_t)aa->aa_node->ad_handle,
		    IPL_SERIAL, true, comintr, sc, device_xname(self));
	}

	if (!pmf_device_register(self, NULL, com_resume))
		aprint_error_dev(self, "couldn't establish a power handler\n");
	goto cleanup;

	/*
	 * In case of irq resource or i/o space mapping error, just set
	 * a NULL power handler.  This may allow us to sleep later on.
	 */
 out:
	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish a power handler\n");

 cleanup:
	acpi_resource_cleanup(&res);
}
