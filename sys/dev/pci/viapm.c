/*	$NetBSD: viapm.c,v 1.2.8.2 2002/04/01 07:46:47 nathanw Exp $	*/

/*
 * Copyright (c) 2000 Johan Danielsson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of author nor the names of any contributors may
 *    be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS
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
 * frontend for the power management device in the VIA VT82C686a South
 * Bridge. It (the chip) has three functions, power management, hardware
 * monitoring, and an SMBus controller. A driver for the hardware monitoring
 * is provided by the viaenv driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: viapm.c,v 1.2.8.2 2002/04/01 07:46:47 nathanw Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/viapmvar.h>

struct viapm_softc {
	struct device sc_dev;

	pci_chipset_tag_t sc_pc;
	pcitag_t sc_tag;
	bus_space_tag_t sc_iot;
};

static int
viapm_match(struct device * parent, struct cfdata * match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_VIATECH &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_VIATECH_VT82C686A_SMB)
		return 1;
	return 0;
}

static const char *
viapm_device_name(enum vapm_devtype type)
{

	switch (type) {
	case VIAPM_POWER:
		return "power management";
	case VIAPM_HWMON:
		return "hardware monitor";
	case VIAPM_SMBUS:
		return "SMBus controller";
	default:
		panic("viapm_device_name: unknown type");
	}
}

static int
viapm_print(void *aux, const char *pnp)
{
	struct viapm_attach_args *vaa = aux;

	if (pnp)
		printf("%s at %s", viapm_device_name(vaa->va_type), pnp);
	return UNCONF;
}

static int
viapm_submatch(struct device * parent, struct cfdata * cf, void *aux)
{

	return (*cf->cf_attach->ca_match) (parent, cf, aux);
}

static void
viapm_attach(struct device * parent, struct device * self, void *aux)
{
	struct viapm_softc *sc = (struct viapm_softc *) self;
	struct pci_attach_args *pa = aux;
	struct viapm_attach_args vaa;

	printf("\n");

	sc->sc_pc = pa->pa_pc;
	sc->sc_tag = pa->pa_tag;
	sc->sc_iot = pa->pa_iot;

	vaa.va_pc = sc->sc_pc;
	vaa.va_tag = sc->sc_tag;
	vaa.va_iot = sc->sc_iot;

#if 0
	/*
	 * no point in confusing people with these until there are drivers for
	 * them
	 */
	vaa.va_type = VIAPM_POWER;
	vaa.va_offset = 0x40;
	config_found_sm(self, &vaa, viapm_print, viapm_submatch);
#endif

	vaa.va_type = VIAPM_HWMON;
	vaa.va_offset = 0x70;
	config_found_sm(self, &vaa, viapm_print, viapm_submatch);

#if 0
	vaa.va_type = VIAPM_SMBUS;
	vaa.va_offset = 0x93;
	config_found_sm(self, &vaa, viapm_print, viapm_submatch);
#endif
}

struct cfattach viapm_ca = {
	sizeof(struct viapm_softc), viapm_match, viapm_attach
};
