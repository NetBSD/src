/* $NetBSD: acpipchb.c,v 1.15 2019/12/29 23:47:56 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: acpipchb.c,v 1.15 2019/12/29 23:47:56 jmcneill Exp $");

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

#define	ACPIPCHB_MAX_RANGES	64	/* XXX arbitrary limit */

struct acpipchb_bus_range {
	bus_addr_t		min;
	bus_addr_t		max;
	bus_addr_t		offset;
};

struct acpipchb_bus_space {
	struct bus_space	bs;

	struct acpipchb_bus_range range[ACPIPCHB_MAX_RANGES];
	int			nrange;

	int			(*map)(void *, bus_addr_t, bus_size_t,
				       int, bus_space_handle_t *);

	int			flags;
};

struct acpipchb_softc {
	device_t		sc_dev;

	bus_space_tag_t		sc_memt;

	struct arm32_bus_dma_tag sc_dmat;
	struct acpi_pci_context sc_ap;

	ACPI_HANDLE		sc_handle;
	ACPI_INTEGER		sc_bus;

	struct acpipchb_bus_space sc_pcimem_bst;
	struct acpipchb_bus_space sc_pciio_bst;
};

static int
acpipchb_amazon_graviton_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t *data)
{
	struct acpi_pci_context *ap = pc->pc_conf_v;
	int b, d, f;

	pci_decompose_tag(pc, tag, &b, &d, &f);

	if (ap->ap_bus == b) {
		if (d > 0 || f > 0) {
			*data = -1;
			return EINVAL;
		}
		*data = bus_space_read_4(ap->ap_bst, ap->ap_conf_bsh, reg);
		return 0;
	}
	
	return acpimcfg_conf_read(pc, tag, reg, data);
}

static int
acpipchb_amazon_graviton_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t data)
{
	struct acpi_pci_context *ap = pc->pc_conf_v;
	int b, d, f;

	pci_decompose_tag(pc, tag, &b, &d, &f);

	if (ap->ap_bus == b) {
		if (d > 0 || f > 0) {
			return EINVAL;
		}
		bus_space_write_4(ap->ap_bst, ap->ap_conf_bsh, reg, data);
		return 0;
	}
	
	return acpimcfg_conf_write(pc, tag, reg, data);
}

static ACPI_STATUS
acpipchb_amazon_graviton_map(ACPI_HANDLE handle, UINT32 level, void *ctx, void **retval)
{
	struct acpi_pci_context *ap = ctx;
	struct acpi_resources res;
	struct acpi_mem *mem;
	ACPI_HANDLE parent;
	ACPI_STATUS rv;
	int error;

	rv = AcpiGetParent(handle, &parent);
	if (ACPI_FAILURE(rv))
		return rv;
	if (ap->ap_handle != parent)
		return AE_OK;

	rv = acpi_resource_parse(ap->ap_dev, handle, "_CRS", &res, &acpi_resource_parse_ops_quiet);
	if (ACPI_FAILURE(rv))
		return rv;

	mem = acpi_res_mem(&res, 0);
	if (mem == NULL) {
		acpi_resource_cleanup(&res);
		return AE_NOT_FOUND;
	}

	error = bus_space_map(ap->ap_bst, mem->ar_base, mem->ar_length,
	    _ARM_BUS_SPACE_MAP_STRONGLY_ORDERED, &ap->ap_conf_bsh);
	if (error != 0)
		return AE_NO_MEMORY;

	ap->ap_conf_read = acpipchb_amazon_graviton_conf_read;
	ap->ap_conf_write = acpipchb_amazon_graviton_conf_write;

	return AE_CTRL_TERMINATE;
}

static void
acpipchb_amazon_graviton_init(struct acpi_pci_context *ap)
{
	ACPI_STATUS rv;

	rv = AcpiGetDevices(__UNCONST("AMZN0001"), acpipchb_amazon_graviton_map, ap, NULL);
	if (ACPI_FAILURE(rv))
		return;
}

static const struct acpipchb_quirk {
	const char			q_oemid[ACPI_OEM_ID_SIZE+1];
	const char			q_oemtableid[ACPI_OEM_TABLE_ID_SIZE+1];
	uint32_t			q_oemrevision;
	void				(*q_init)(struct acpi_pci_context *);
} acpipchb_quirks[] = {
	{ "AMAZON",	"GRAVITON",	0,	acpipchb_amazon_graviton_init },
};

static const struct acpipchb_quirk *
acpipchb_find_quirk(void)
{
	ACPI_STATUS rv;
	ACPI_TABLE_MCFG *mcfg;
	u_int n;

	rv = AcpiGetTable(ACPI_SIG_MCFG, 0, (ACPI_TABLE_HEADER **)&mcfg);
	if (ACPI_FAILURE(rv))
		return NULL;

	for (n = 0; n < __arraycount(acpipchb_quirks); n++) {
		const struct acpipchb_quirk *q = &acpipchb_quirks[n];
		if (memcmp(q->q_oemid, mcfg->Header.OemId, ACPI_OEM_ID_SIZE) == 0 &&
		    memcmp(q->q_oemtableid, mcfg->Header.OemTableId, ACPI_OEM_TABLE_ID_SIZE) == 0 &&
		    q->q_oemrevision == mcfg->Header.OemRevision)
			return q;
	}

	return NULL;
}

static int	acpipchb_match(device_t, cfdata_t, void *);
static void	acpipchb_attach(device_t, device_t, void *);

static void	acpipchb_setup_ranges(struct acpipchb_softc *, struct pcibus_attach_args *);

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
	const struct acpipchb_quirk *q;
	ACPI_INTEGER seg;

	sc->sc_dev = self;
	sc->sc_memt = aa->aa_memt;
	sc->sc_handle = aa->aa_node->ad_handle;

	if (ACPI_FAILURE(acpi_eval_integer(sc->sc_handle, "_BBN", &sc->sc_bus)))
		sc->sc_bus = 0;

	if (ACPI_FAILURE(acpi_eval_integer(sc->sc_handle, "_SEG", &seg)))
		seg = 0;

	aprint_naive("\n");
	aprint_normal(": PCI Express Host Bridge\n");

	sc->sc_dmat = *aa->aa_dmat;

	sc->sc_ap.ap_dev = self;
	sc->sc_ap.ap_pc = *aa->aa_pc;
	sc->sc_ap.ap_pc.pc_conf_v = &sc->sc_ap;
	sc->sc_ap.ap_seg = seg;
	sc->sc_ap.ap_handle = sc->sc_handle;
	sc->sc_ap.ap_bus = sc->sc_bus;
	sc->sc_ap.ap_bst = sc->sc_memt;

	q = acpipchb_find_quirk();
	if (q != NULL)
		q->q_init(&sc->sc_ap);

	if (acpi_pci_ignore_boot_config(sc->sc_handle)) {
		if (acpimcfg_configure_bus(self, &sc->sc_ap.ap_pc, sc->sc_handle, sc->sc_bus, PCIHOST_CACHELINE_SIZE) != 0)
			aprint_error_dev(self, "failed to configure bus\n");
	}

	memset(&pba, 0, sizeof(pba));
	pba.pba_flags = aa->aa_pciflags & ~(PCI_FLAGS_MEM_OKAY | PCI_FLAGS_IO_OKAY);
	pba.pba_memt = 0;
	pba.pba_iot = 0;
	pba.pba_dmat = &sc->sc_dmat;
#ifdef _PCI_HAVE_DMA64
	pba.pba_dmat64 = &sc->sc_dmat;
#endif
	pba.pba_pc = &sc->sc_ap.ap_pc;
	pba.pba_bus = sc->sc_bus;

	acpipchb_setup_ranges(sc, &pba);

	config_found_ia(self, "pcibus", &pba, pcibusprint);
}

struct acpipchb_setup_ranges_args {
	struct acpipchb_softc *sc;
	struct pcibus_attach_args *pba;
};

static int
acpipchb_bus_space_map(void *t, bus_addr_t bpa, bus_size_t size, int flag,
    bus_space_handle_t *bshp)
{
	struct acpipchb_bus_space * const abs = t;
	int i;

	if (size == 0)
		return ERANGE;

	if ((abs->flags & PCI_FLAGS_IO_OKAY) != 0) {
		/* Force strongly ordered mapping for all I/O space */
		flag = _ARM_BUS_SPACE_MAP_STRONGLY_ORDERED;
	}

	for (i = 0; i < abs->nrange; i++) {
		struct acpipchb_bus_range * const range = &abs->range[i];
		if (bpa >= range->min && bpa + size - 1 <= range->max) 
			return abs->map(t, bpa + range->offset, size, flag, bshp);
	}

	return ERANGE;
}

static ACPI_STATUS
acpipchb_setup_ranges_cb(ACPI_RESOURCE *res, void *ctx)
{
	struct acpipchb_setup_ranges_args * const args = ctx;
	struct acpipchb_softc * const sc = args->sc;
	struct pcibus_attach_args *pba = args->pba;
	struct acpipchb_bus_space *abs;
	struct acpipchb_bus_range *range;
	const char *range_type;
	u_int pci_flags;

	if (res->Type != ACPI_RESOURCE_TYPE_ADDRESS32 &&
	    res->Type != ACPI_RESOURCE_TYPE_ADDRESS64)
		return AE_OK;

	switch (res->Data.Address.ResourceType) {
	case ACPI_IO_RANGE:
		abs = &sc->sc_pciio_bst;
		range_type = "I/O";
		pci_flags = PCI_FLAGS_IO_OKAY;
		break;
	case ACPI_MEMORY_RANGE:
		abs = &sc->sc_pcimem_bst;
		range_type = "MEM";
		pci_flags = PCI_FLAGS_MEM_OKAY;
		break;
	default:
		return AE_OK;
	}

	if (abs->nrange == ACPIPCHB_MAX_RANGES) {
		aprint_error_dev(sc->sc_dev,
		    "maximum number of ranges reached, increase ACPIPCHB_MAX_RANGES\n");
		return AE_LIMIT;
	}

	range = &abs->range[abs->nrange];
	switch (res->Type) {
	case ACPI_RESOURCE_TYPE_ADDRESS32:
		range->min = res->Data.Address32.Address.Minimum;
		range->max = res->Data.Address32.Address.Maximum;
		range->offset = res->Data.Address32.Address.TranslationOffset;
		break;
	case ACPI_RESOURCE_TYPE_ADDRESS64:
		range->min = res->Data.Address64.Address.Minimum;
		range->max = res->Data.Address64.Address.Maximum;
		range->offset = res->Data.Address64.Address.TranslationOffset;
		break;
	default:
		return AE_OK;
	}
	abs->nrange++;

	aprint_debug_dev(sc->sc_dev, "PCI %s [%#lx-%#lx] -> %#lx\n", range_type, range->min, range->max, range->offset);

	if ((pba->pba_flags & pci_flags) == 0) {
		abs->bs = *sc->sc_memt;
		abs->bs.bs_cookie = abs;
		abs->map = abs->bs.bs_map;
		abs->flags = pci_flags;
		abs->bs.bs_map = acpipchb_bus_space_map;
		if ((pci_flags & PCI_FLAGS_IO_OKAY) != 0)
			pba->pba_iot = &abs->bs;
		else if ((pci_flags & PCI_FLAGS_MEM_OKAY) != 0)
			pba->pba_memt = &abs->bs;
		pba->pba_flags |= pci_flags;
	}

	return AE_OK;
}

static void
acpipchb_setup_ranges(struct acpipchb_softc *sc, struct pcibus_attach_args *pba)
{
	struct acpipchb_setup_ranges_args args;

	args.sc = sc;
	args.pba = pba;

	AcpiWalkResources(sc->sc_handle, "_CRS", acpipchb_setup_ranges_cb, &args);
}
