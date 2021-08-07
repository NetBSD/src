/* $NetBSD: bcm2835_bsc_acpi.c,v 1.4 2021/08/07 16:18:43 thorpej Exp $ */

/*-
 * Copyright (c) 2020 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: bcm2835_bsc_acpi.c,v 1.4 2021/08/07 16:18:43 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_intr.h>
#include <dev/acpi/acpi_i2c.h>

#include <arm/broadcom/bcm2835var.h>
#include <arm/broadcom/bcm2835_mbox.h>
#include <arm/broadcom/bcm2835_bscvar.h>

#include <evbarm/rpi/vcio.h>
#include <evbarm/rpi/vcpm.h>
#include <evbarm/rpi/vcprop.h>

static int	bsciic_acpi_match(device_t, cfdata_t, void *);
static void	bsciic_acpi_attach(device_t, device_t, void *);

static u_int	bsciic_acpi_vpu_clock_rate(void);

CFATTACH_DECL_NEW(bsciic_acpi, sizeof(struct bsciic_softc), bsciic_acpi_match, bsciic_acpi_attach, NULL, NULL);

static const char * const compatible[] = {
	"BCM2841",
	NULL
};

static struct {
	struct vcprop_buffer_hdr	vb_hdr;
	struct vcprop_tag_clockrate	vbt_vpuclockrate;
	struct vcprop_tag end;
} vb_vpu __cacheline_aligned = {
	.vb_hdr = {
		.vpb_len = sizeof(vb_vpu),
		.vpb_rcode = VCPROP_PROCESS_REQUEST
	},
	.vbt_vpuclockrate = {
		.tag = {
			.vpt_tag = VCPROPTAG_GET_CLOCKRATE,
			.vpt_len = VCPROPTAG_LEN(vb_vpu.vbt_vpuclockrate),
			.vpt_rcode = VCPROPTAG_REQUEST
		},
		.id = VCPROP_CLK_CORE
	},
	.end = {
		.vpt_tag = VCPROPTAG_NULL
	}
};

static int
bsciic_acpi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, compatible);
}

static void
bsciic_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct bsciic_softc * const sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	struct i2cbus_attach_args iba;
	struct acpi_resources res;
	struct acpi_mem *mem;
	struct acpi_irq *irq;
	ACPI_STATUS rv;
	ACPI_INTEGER clock_freq;
	void *ih;

	sc->sc_dev = self;

	rv = acpi_resource_parse(sc->sc_dev, aa->aa_node->ad_handle, "_CRS",
	    &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv))
		return;

	mem = acpi_res_mem(&res, 0);
	if (mem == NULL) {
		aprint_error_dev(self, "couldn't find mem resource\n");
		goto done;
	}

	irq = acpi_res_irq(&res, 0);
	if (irq == NULL) {
		aprint_error_dev(self, "couldn't find irq resource\n");
		goto done;
	}

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_memt;
	if (bus_space_map(aa->aa_memt, mem->ar_base, mem->ar_length, 0, &sc->sc_ioh) != 0) {
		aprint_error_dev(self, "couldn't map registers\n");
		goto done;
	}

	sc->sc_frequency = bsciic_acpi_vpu_clock_rate();
	if (sc->sc_frequency == 0) {
		aprint_error_dev(self, "couldn't determine parent clock rate\n");
		goto done;
	}

	rv = acpi_dsd_integer(aa->aa_node->ad_handle, "clock-frequency", &clock_freq);
	if (ACPI_SUCCESS(rv))
		sc->sc_clkrate = clock_freq;
	else
		sc->sc_clkrate = 100000;

	bsciic_attach(sc);

	ih = acpi_intr_establish(self, (uint64_t)aa->aa_node->ad_handle,
	    IPL_VM, true, bsciic_intr, sc, device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't install interrupt handler\n");
		goto done;
	}

	iic_tag_init(&sc->sc_i2c);
	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_acquire_bus = bsciic_acquire_bus;
	sc->sc_i2c.ic_release_bus = bsciic_release_bus;
	sc->sc_i2c.ic_exec = bsciic_exec;

	memset(&iba, 0, sizeof(iba));
	iba.iba_tag = &sc->sc_i2c;
	iba.iba_child_devices = acpi_enter_i2c_devs(self, aa->aa_node);
	config_found(self, &iba, iicbus_print, CFARGS_NONE);

done:
	acpi_resource_cleanup(&res);
}

static u_int
bsciic_acpi_vpu_clock_rate(void)
{
	uint32_t res;
	int error;

	error = bcmmbox_request(BCMMBOX_CHANARM2VC, &vb_vpu,
	    sizeof(vb_vpu), &res);
	if (error != 0) {
		printf("%s: mbox request failed (%d)\n", __func__, error);
		return 0;
	}

	if (!vcprop_buffer_success_p(&vb_vpu.vb_hdr) ||
	    !vcprop_tag_success_p(&vb_vpu.vbt_vpuclockrate.tag) ||
	    vb_vpu.vbt_vpuclockrate.rate < 0)
		return 0;

	return vb_vpu.vbt_vpuclockrate.rate;
}
