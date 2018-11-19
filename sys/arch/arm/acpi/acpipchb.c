/* $NetBSD: acpipchb.c,v 1.7 2018/11/19 10:45:47 jmcneill Exp $ */

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jared McNeill <jmcneill@invisible.ca>.
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
__KERNEL_RCSID(0, "$NetBSD: acpipchb.c,v 1.7 2018/11/19 10:45:47 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/extent.h>
#include <sys/queue.h>
#include <sys/mutex.h>
#include <sys/kmem.h>

#include <machine/cpu.h>

#include <arm/cpufunc.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_pci.h>
#include <dev/acpi/acpi_mcfg.h>

#include <arm/acpi/acpi_pci_machdep.h>

#define	PCIHOST_CACHELINE_SIZE		arm_dcache_align

struct acpipchb_bus_space {
	struct bus_space	bs;

	bus_addr_t		min;
	bus_addr_t		max;
	bus_addr_t		offset;

	int			(*map)(void *, bus_addr_t, bus_size_t,
				       int, bus_space_handle_t *);
};

struct acpipchb_softc {
	device_t		sc_dev;

	struct arm32_bus_dma_tag sc_dmat;
	struct acpi_pci_context sc_ap;

	ACPI_HANDLE		sc_handle;
	ACPI_INTEGER		sc_bus;

	struct acpipchb_bus_space sc_pciio_bst;
};

static struct arm32_dma_range ahcipchb_coherent_ranges[] = {
	[0] = {
		.dr_sysbase = 0,
		.dr_busbase = 0,
		.dr_len = UINTPTR_MAX,
		.dr_flags = _BUS_DMAMAP_COHERENT,
	}
};

static int	acpipchb_match(device_t, cfdata_t, void *);
static void	acpipchb_attach(device_t, device_t, void *);

static void	acpipchb_setup_pciio(struct acpipchb_softc *, struct pcibus_attach_args *);

CFATTACH_DECL_NEW(acpipchb, sizeof(struct acpipchb_softc),
	acpipchb_match, acpipchb_attach, NULL, NULL);

static const char * const compatible[] = {
	"PNP0A08",
	NULL
};

static int
acpipchb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, compatible);
}

static void
acpipchb_attach(device_t parent, device_t self, void *aux)
{
	struct acpipchb_softc * const sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	struct pcibus_attach_args pba;
	ACPI_INTEGER cca, seg;

	sc->sc_dev = self;
	sc->sc_handle = aa->aa_node->ad_handle;

	if (ACPI_FAILURE(acpi_eval_integer(sc->sc_handle, "_BBN", &sc->sc_bus)))
		sc->sc_bus = 0;

	if (ACPI_FAILURE(acpi_eval_integer(sc->sc_handle, "_SEG", &seg)))
		seg = 0;

	if (ACPI_FAILURE(acpi_eval_integer(sc->sc_handle, "_CCA", &cca)))
		cca = 0;

	aprint_naive("\n");
	aprint_normal(": PCI Express Host Bridge\n");

	sc->sc_dmat = *aa->aa_dmat;
	if (cca) {
		sc->sc_dmat._ranges = ahcipchb_coherent_ranges;
		sc->sc_dmat._nranges = __arraycount(ahcipchb_coherent_ranges);
	}

	sc->sc_ap.ap_pc = *aa->aa_pc;
	sc->sc_ap.ap_pc.pc_conf_v = &sc->sc_ap;
	sc->sc_ap.ap_seg = seg;

	if (acpi_pci_ignore_boot_config(sc->sc_handle)) {
		if (acpimcfg_configure_bus(self, &sc->sc_ap.ap_pc, sc->sc_handle, sc->sc_bus, PCIHOST_CACHELINE_SIZE) != 0)
			aprint_error_dev(self, "failed to configure bus\n");
	}

	memset(&pba, 0, sizeof(pba));
	pba.pba_flags = aa->aa_pciflags;
	pba.pba_iot = 0;
	pba.pba_memt = aa->aa_memt;
	pba.pba_dmat = &sc->sc_dmat;
#ifdef _PCI_HAVE_DMA64
	pba.pba_dmat64 = &sc->sc_dmat;
#endif
	pba.pba_pc = &sc->sc_ap.ap_pc;
	pba.pba_bus = sc->sc_bus;

	acpipchb_setup_pciio(sc, &pba);

	config_found_ia(self, "pcibus", &pba, pcibusprint);
}

struct acpipchb_setup_pciio_args {
	struct acpipchb_softc *sc;
	struct pcibus_attach_args *pba;
};

static int
acpipchb_bus_space_map(void *t, bus_addr_t bpa, bus_size_t size, int flag,
    bus_space_handle_t *bshp)
{
	struct acpipchb_bus_space * const abs = t;

	if (bpa < abs->min || bpa + size >= abs->max)
		return ERANGE;

	return abs->map(t, bpa + abs->offset, size, flag, bshp);
}

static ACPI_STATUS
acpipchb_setup_pciio_cb(ACPI_RESOURCE *res, void *ctx)
{
	struct acpipchb_setup_pciio_args * const args = ctx;
	struct acpipchb_softc * const sc = args->sc;
	struct acpipchb_bus_space * const abs = &sc->sc_pciio_bst;
	struct pcibus_attach_args *pba = args->pba;

	if (res->Type != ACPI_RESOURCE_TYPE_ADDRESS32 &&
	    res->Type != ACPI_RESOURCE_TYPE_ADDRESS64)
		return AE_OK;

	if (res->Data.Address.ResourceType != ACPI_IO_RANGE)
		return AE_OK;

	abs->bs = *pba->pba_memt;
	abs->bs.bs_cookie = abs;
	abs->map = abs->bs.bs_map;
	abs->bs.bs_map = acpipchb_bus_space_map;

	switch (res->Type) {
	case ACPI_RESOURCE_TYPE_ADDRESS32:
		abs->min = res->Data.Address32.Address.Minimum;
		abs->max = res->Data.Address32.Address.Maximum;
		abs->offset = res->Data.Address32.Address.TranslationOffset;
		break;
	case ACPI_RESOURCE_TYPE_ADDRESS64:
		abs->min = res->Data.Address64.Address.Minimum;
		abs->max = res->Data.Address64.Address.Maximum;
		abs->offset = res->Data.Address64.Address.TranslationOffset;
		break;
	}

	aprint_debug_dev(sc->sc_dev, "PCI I/O [%#lx-%#lx] -> %#lx\n", abs->min, abs->max, abs->offset);

	pba->pba_iot = &sc->sc_pciio_bst.bs;
	pba->pba_flags |= PCI_FLAGS_IO_OKAY;

	return AE_LIMIT;
}

static void
acpipchb_setup_pciio(struct acpipchb_softc *sc, struct pcibus_attach_args *pba)
{
	struct acpipchb_setup_pciio_args args;

	args.sc = sc;
	args.pba = pba;

	AcpiWalkResources(sc->sc_handle, "_CRS", acpipchb_setup_pciio_cb, &args);
}
