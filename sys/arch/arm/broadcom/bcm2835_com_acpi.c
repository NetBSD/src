/* $NetBSD: bcm2835_com_acpi.c,v 1.2 2021/08/08 18:55:12 jmcneill Exp $ */

/*
 * Copyright (c) 2021 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: bcm2835_com_acpi.c,v 1.2 2021/08/08 18:55:12 jmcneill Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/termios.h>

#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_intr.h>

#include <dev/ic/comvar.h>

#include <arm/broadcom/bcm2835_mbox.h>
#include <evbarm/rpi/vcio.h>
#include <evbarm/rpi/vcprop.h>

static int	bcmcom_acpi_match(device_t, cfdata_t , void *);
static void	bcmcom_acpi_attach(device_t, device_t, void *);

static u_int	bcmcom_acpi_get_clockrate(device_t);

struct vcmbox_clockrate_request {
	struct vcprop_buffer_hdr	vb_hdr;
	struct vcprop_tag_clockrate	vbt_clockrate;
	struct vcprop_tag end;
} __packed;

CFATTACH_DECL_NEW(bcmcom_acpi, sizeof(struct com_softc), bcmcom_acpi_match,
    bcmcom_acpi_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "BCM2836",		.value = COM_TYPE_BCMAUXUART },
	DEVICE_COMPAT_EOL
};

static int
bcmcom_acpi_match(device_t parent, cfdata_t match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	return acpi_compatible_match(aa, compat_data);
}

static void
bcmcom_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct com_softc *sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	const struct device_compatible_entry *dce;
	struct acpi_resources res;
	struct acpi_mem *mem;
	struct acpi_irq *irq;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_addr_t base;
	bus_size_t size;
	ACPI_STATUS rv;
	void *ih;

	sc->sc_dev = self;

	rv = acpi_resource_parse(sc->sc_dev, aa->aa_node->ad_handle, "_CRS",
	    &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv)) {
		return;
	}

	mem = acpi_res_mem(&res, 0);
	if (mem == NULL) {
		aprint_error_dev(self, "couldn't find mem resource\n");
		goto cleanup;
	}

	iot = aa->aa_memt;
	base = mem->ar_base + 0x40;
	size = mem->ar_length - 0x40;

	irq = acpi_res_irq(&res, 0);
	if (irq == NULL) {
		aprint_error_dev(self, "couldn't find irq resource\n");
		goto cleanup;
	}

	dce = acpi_compatible_lookup(aa, compat_data);
	KASSERT(dce != NULL);

	sc->sc_type = dce->value;

	if (!com_is_console(iot, base, &ioh)) {
		if (bus_space_map(iot, base, size, 0, &ioh)) {
			aprint_error_dev(self, "can't map mem space\n");
			goto cleanup;
		}
	}

	com_init_regs_stride(&sc->sc_regs, iot, ioh, base, 2);

	aprint_normal("%s", device_xname(self));

	sc->sc_frequency = bcmcom_acpi_get_clockrate(self);

	com_attach_subr(sc);

	ih = acpi_intr_establish(self,
	    (uint64_t)(uintptr_t)aa->aa_node->ad_handle,
	    IPL_SERIAL, true, comintr, sc, device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		goto cleanup;
	}

	if (!pmf_device_register(self, NULL, com_resume))
		aprint_error_dev(self, "couldn't establish a power handler\n");

cleanup:
	acpi_resource_cleanup(&res);
}

static u_int
bcmcom_acpi_get_clockrate(device_t dev)
{
	struct vcmbox_clockrate_request vb;
	uint32_t res;
	int error;

	VCPROP_INIT_REQUEST(vb);
	VCPROP_INIT_TAG(vb.vbt_clockrate, VCPROPTAG_GET_CLOCKRATE);
	vb.vbt_clockrate.id = htole32(VCPROP_CLK_CORE);
	error = bcmmbox_request(BCMMBOX_CHANARM2VC, &vb, sizeof(vb), &res);
	if (error != 0) {
		aprint_error_dev(dev, "MBOX request failed: %d\n", error);
		return 0;
	}
	if (!vcprop_buffer_success_p(&vb.vb_hdr) ||
	    !vcprop_tag_success_p(&vb.vbt_clockrate.tag)) {
		return 0;
	}

	return le32toh(vb.vbt_clockrate.rate) * 2;
}
