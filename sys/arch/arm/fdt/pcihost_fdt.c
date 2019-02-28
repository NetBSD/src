/* $NetBSD: pcihost_fdt.c,v 1.8 2019/02/28 00:47:10 jakllsch Exp $ */

/*-
 * Copyright (c) 2018 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: pcihost_fdt.c,v 1.8 2019/02/28 00:47:10 jakllsch Exp $");

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

#include <dev/fdt/fdtvar.h>

#include <arm/pci/pci_msi_machdep.h>
#include <arm/fdt/pcihost_fdtvar.h>

#define	IH_INDEX_MASK			0x0000ffff
#define	IH_MPSAFE			0x80000000

#define	PCIHOST_DEFAULT_BUS_MIN		0
#define	PCIHOST_DEFAULT_BUS_MAX		255

#define	PCIHOST_CACHELINE_SIZE		arm_dcache_align

int pcihost_segment = 0;

static int	pcihost_match(device_t, cfdata_t, void *);
static void	pcihost_attach(device_t, device_t, void *);

static int	pcihost_config(struct pcihost_softc *);

static void	pcihost_attach_hook(device_t, device_t,
				       struct pcibus_attach_args *);
static int	pcihost_bus_maxdevs(void *, int);
static pcitag_t	pcihost_make_tag(void *, int, int, int);
static void	pcihost_decompose_tag(void *, pcitag_t, int *, int *, int *);
static u_int	pcihost_get_segment(void *);
static pcireg_t	pcihost_conf_read(void *, pcitag_t, int);
static void	pcihost_conf_write(void *, pcitag_t, int, pcireg_t);
static int	pcihost_conf_hook(void *, int, int, int, pcireg_t);
static void	pcihost_conf_interrupt(void *, int, int, int, int, int *);

static int	pcihost_intr_map(const struct pci_attach_args *,
				    pci_intr_handle_t *);
static const char *pcihost_intr_string(void *, pci_intr_handle_t,
					  char *, size_t);
static const struct evcnt *pcihost_intr_evcnt(void *, pci_intr_handle_t);
static int	pcihost_intr_setattr(void *, pci_intr_handle_t *, int,
					uint64_t);
static void *	pcihost_intr_establish(void *, pci_intr_handle_t,
					 int, int (*)(void *), void *,
					 const char *);
static void	pcihost_intr_disestablish(void *, void *);

static int	pcihost_bus_space_map(void *, bus_addr_t, bus_size_t,
		int, bus_space_handle_t *);

CFATTACH_DECL_NEW(pcihost_fdt, sizeof(struct pcihost_softc),
	pcihost_match, pcihost_attach, NULL, NULL);

static const struct of_compat_data compat_data[] = {
	{ "pci-host-cam-generic",	PCIHOST_CAM },
	{ "pci-host-ecam-generic",	PCIHOST_ECAM },
	{ NULL,				0 }
};

static int
pcihost_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
pcihost_attach(device_t parent, device_t self, void *aux)
{
	struct pcihost_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t cs_addr;
	bus_size_t cs_size;
	int error;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &cs_addr, &cs_size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_dmat = faa->faa_dmat;
	sc->sc_bst = faa->faa_bst;
	sc->sc_phandle = faa->faa_phandle;
	error = bus_space_map(sc->sc_bst, cs_addr, cs_size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map registers: %d\n", error);
		return;
	}
	sc->sc_type = of_search_compatible(sc->sc_phandle, compat_data)->data;

	aprint_naive("\n");
	aprint_normal(": Generic PCI host controller\n");

	pcihost_init(&sc->sc_pc, sc);
	pcihost_init2(sc);
}

void
pcihost_init2(struct pcihost_softc *sc)
{
	struct pcibus_attach_args pba;
	const u_int *data;
	int len;

	if ((data = fdtbus_get_prop(sc->sc_phandle, "bus-range", &len)) != NULL) {
		if (len != 8) {
			aprint_error_dev(sc->sc_dev, "malformed 'bus-range' property\n");
			return;
		}
		sc->sc_bus_min = be32toh(data[0]);
		sc->sc_bus_max = be32toh(data[1]);
	} else {
		sc->sc_bus_min = PCIHOST_DEFAULT_BUS_MIN;
		sc->sc_bus_max = PCIHOST_DEFAULT_BUS_MAX;
	}

	/*
	 * Assign a fixed PCI segment ("domain") number. If the property is not
	 * present, assign one. The binding spec says if this property is used to
	 * assign static segment numbers, all host bridges should have segments
	 * astatic assigned to prevent overlaps.
	 */
	if (of_getprop_uint32(sc->sc_phandle, "linux,pci-domain", &sc->sc_seg))
		sc->sc_seg = pcihost_segment++;

	if (pcihost_config(sc) != 0)
		return;

	memset(&pba, 0, sizeof(pba));
	pba.pba_flags = PCI_FLAGS_MRL_OKAY |
			PCI_FLAGS_MRM_OKAY |
			PCI_FLAGS_MWI_OKAY |
			PCI_FLAGS_IO_OKAY |
			PCI_FLAGS_MEM_OKAY;
#ifdef __HAVE_PCI_MSI_MSIX
	if (sc->sc_type == PCIHOST_ECAM) {
		pba.pba_flags |= PCI_FLAGS_MSI_OKAY |
				 PCI_FLAGS_MSIX_OKAY;
	}
#endif
	pba.pba_iot = &sc->sc_io.bst;
	pba.pba_memt = &sc->sc_mem.bst;
	pba.pba_dmat = sc->sc_dmat;
#ifdef _PCI_HAVE_DMA64
	pba.pba_dmat64 = sc->sc_dmat;
#endif
	pba.pba_pc = &sc->sc_pc;
	pba.pba_bus = sc->sc_bus_min;

	config_found_ia(sc->sc_dev, "pcibus", &pba, pcibusprint);
}

void
pcihost_init(pci_chipset_tag_t pc, void *priv)
{
	pc->pc_conf_v = priv;
	pc->pc_attach_hook = pcihost_attach_hook;
	pc->pc_bus_maxdevs = pcihost_bus_maxdevs;
	pc->pc_make_tag = pcihost_make_tag;
	pc->pc_decompose_tag = pcihost_decompose_tag;
	pc->pc_get_segment = pcihost_get_segment;
	pc->pc_conf_read = pcihost_conf_read;
	pc->pc_conf_write = pcihost_conf_write;
	pc->pc_conf_hook = pcihost_conf_hook;
	pc->pc_conf_interrupt = pcihost_conf_interrupt;

	pc->pc_intr_v = priv;
	pc->pc_intr_map = pcihost_intr_map;
	pc->pc_intr_string = pcihost_intr_string;
	pc->pc_intr_evcnt = pcihost_intr_evcnt;
	pc->pc_intr_setattr = pcihost_intr_setattr;
	pc->pc_intr_establish = pcihost_intr_establish;
	pc->pc_intr_disestablish = pcihost_intr_disestablish;
}

static int
pcihost_config(struct pcihost_softc *sc)
{
	struct extent *ioext = NULL, *memext = NULL, *pmemext = NULL;
	const u_int *ranges;
	u_int probe_only;
	int error, len;

	struct pcih_bus_space * const pibs = &sc->sc_io;
	pibs->bst = *sc->sc_bst;
	pibs->bst.bs_cookie = pibs;
	pibs->map = pibs->bst.bs_map;
	pibs->bst.bs_map = pcihost_bus_space_map;

	struct pcih_bus_space * const pmbs = &sc->sc_mem;
	pmbs->bst = *sc->sc_bst;
	pmbs->bst.bs_cookie = pmbs;
	pmbs->map = pmbs->bst.bs_map;
	pmbs->bst.bs_map = pcihost_bus_space_map;

	/*
	 * If this flag is set, skip configuration of the PCI bus and use existing config.
	 */
	if (of_getprop_uint32(sc->sc_phandle, "linux,pci-probe-only", &probe_only))
		probe_only = 0;
	if (probe_only)
		return 0;

	ranges = fdtbus_get_prop(sc->sc_phandle, "ranges", &len);
	if (ranges == NULL) {
		aprint_error_dev(sc->sc_dev, "missing 'ranges' property\n");
		return EINVAL;
	}

	/*
	 * Each entry in the ranges table contains:
	 *  - bus address (3 cells)
	 *  - cpu physical address (2 cells)
	 *  - size (2 cells)
	 * Total size for each entry is 28 bytes (7 cells).
	 */
	while (len >= 28) {
		const uint32_t phys_hi = be32dec(&ranges[0]);
		const uint64_t bus_phys = be64dec(&ranges[1]);
		const uint64_t cpu_phys = be64dec(&ranges[3]);
		const uint64_t size = be64dec(&ranges[5]);

		len -= 28;
		ranges += 7;

		const bool is64 = (__SHIFTOUT(phys_hi, PHYS_HI_SPACE) ==
		    PHYS_HI_SPACE_MEM64) ? true : false;
		switch (__SHIFTOUT(phys_hi, PHYS_HI_SPACE)) {
		case PHYS_HI_SPACE_IO:
			if (pibs->nranges + 1 >= __arraycount(pibs->ranges)) {
				aprint_error_dev(sc->sc_dev, "too many IO ranges\n");
				continue;
			}
			pibs->ranges[pibs->nranges].bpci = bus_phys;
			pibs->ranges[pibs->nranges].bbus = cpu_phys;
			pibs->ranges[pibs->nranges].size = size;
			++pibs->nranges;
			if (ioext != NULL) {
				aprint_error_dev(sc->sc_dev, "ignoring duplicate IO space range\n");
				continue;
			}
			ioext = extent_create("pciio", bus_phys, bus_phys + size - 1, NULL, 0, EX_NOWAIT);
			aprint_verbose_dev(sc->sc_dev,
			    "IO: 0x%" PRIx64 "+0x%" PRIx64 "@0x%" PRIx64 "\n",
			    bus_phys, size, cpu_phys);
			/* reserve a PC-like legacy IO ports range, perhaps for access to VGA registers */
			if (bus_phys == 0 && size >= 0x10000)
				extent_alloc_region(ioext, 0, 0x1000, EX_WAITOK);
			break;
		case PHYS_HI_SPACE_MEM64:
			/* FALLTHROUGH */
		case PHYS_HI_SPACE_MEM32:
			if (pmbs->nranges + 1 >= __arraycount(pmbs->ranges)) {
				aprint_error_dev(sc->sc_dev, "too many mem ranges\n");
				continue;
			}
			/* both pmem and mem spaces are in the same tag */
			pmbs->ranges[pmbs->nranges].bpci = bus_phys;
			pmbs->ranges[pmbs->nranges].bbus = cpu_phys;
			pmbs->ranges[pmbs->nranges].size = size;
			++pmbs->nranges;
			if ((phys_hi & PHYS_HI_PREFETCH) != 0 ||
			    __SHIFTOUT(phys_hi, PHYS_HI_SPACE) == PHYS_HI_SPACE_MEM64) {
				if (pmemext != NULL) {
					aprint_error_dev(sc->sc_dev, "ignoring duplicate mem (prefetchable) range\n");
					continue;
				}
				pmemext = extent_create("pcipmem", bus_phys, bus_phys + size - 1, NULL, 0, EX_NOWAIT);
				aprint_verbose_dev(sc->sc_dev,
				    "MMIO (%d-bit prefetchable): 0x%" PRIx64 "+0x%" PRIx64 "@0x%" PRIx64 "\n",
				    is64 ? 64 : 32, bus_phys, size, cpu_phys);
			} else {
				if (memext != NULL) {
					aprint_error_dev(sc->sc_dev, "ignoring duplicate mem (non-prefetchable) range\n");
					continue;
				}
				memext = extent_create("pcimem", bus_phys, bus_phys + size - 1, NULL, 0, EX_NOWAIT);
				aprint_verbose_dev(sc->sc_dev,
				    "MMIO (%d-bit non-prefetchable): 0x%" PRIx64 "+0x%" PRIx64 "@0x%" PRIx64 "\n",
				    is64 ? 64 : 32, bus_phys, size, cpu_phys);
			}
			break;
		default:
			break;
		}
	}

	error = pci_configure_bus(&sc->sc_pc, ioext, memext, pmemext, sc->sc_bus_min, PCIHOST_CACHELINE_SIZE);

	if (ioext)
		extent_destroy(ioext);
	if (memext)
		extent_destroy(memext);
	if (pmemext)
		extent_destroy(pmemext);

	if (error) {
		aprint_error_dev(sc->sc_dev, "configuration failed: %d\n", error);
		return error;
	}

	return 0;
}

static void
pcihost_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{
}

static int
pcihost_bus_maxdevs(void *v, int busno)
{
	return 32;
}

static pcitag_t
pcihost_make_tag(void *v, int b, int d, int f)
{
	return (b << 16) | (d << 11) | (f << 8);
}

static void
pcihost_decompose_tag(void *v, pcitag_t tag, int *bp, int *dp, int *fp)
{
	if (bp)
		*bp = (tag >> 16) & 0xff;
	if (dp)
		*dp = (tag >> 11) & 0x1f;
	if (fp)
		*fp = (tag >> 8) & 0x7;
}

static u_int
pcihost_get_segment(void *v)
{
	struct pcihost_softc *sc = v;

	return sc->sc_seg;
}

static pcireg_t
pcihost_conf_read(void *v, pcitag_t tag, int offset)
{
	struct pcihost_softc *sc = v;
	int b, d, f;
	u_int reg;

	pcihost_decompose_tag(v, tag, &b, &d, &f);

	if (b < sc->sc_bus_min || b > sc->sc_bus_max)
		return (pcireg_t) -1;

	if (sc->sc_type == PCIHOST_CAM) {
		if (offset & ~0xff)
			return (pcireg_t) -1;
		reg = (b << 16) | (d << 11) | (f << 8) | offset;
	} else if (sc->sc_type == PCIHOST_ECAM) {
		if (offset & ~0xfff)
			return (pcireg_t) -1;
		reg = (b << 20) | (d << 15) | (f << 12) | offset;
	} else {
		return (pcireg_t) -1;
	}

	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, reg);
}

static void
pcihost_conf_write(void *v, pcitag_t tag, int offset, pcireg_t val)
{
	struct pcihost_softc *sc = v;
	int b, d, f;
	u_int reg;

	pcihost_decompose_tag(v, tag, &b, &d, &f);

	if (b < sc->sc_bus_min || b > sc->sc_bus_max)
		return;

	if (sc->sc_type == PCIHOST_CAM) {
		if (offset & ~0xff)
			return;
		reg = (b << 16) | (d << 11) | (f << 8) | offset;
	} else if (sc->sc_type == PCIHOST_ECAM) {
		if (offset & ~0xfff)
			return;
		reg = (b << 20) | (d << 15) | (f << 12) | offset;
	} else {
		return;
	}

	bus_space_write_4(sc->sc_bst, sc->sc_bsh, reg, val);
}

static int
pcihost_conf_hook(void *v, int b, int d, int f, pcireg_t id)
{
	return PCI_CONF_DEFAULT;
}

static void
pcihost_conf_interrupt(void *v, int bus, int dev, int ipin, int swiz, int *ilinep)
{
}

static int
pcihost_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ih)
{
	struct pcihost_softc *sc = pa->pa_pc->pc_intr_v;
	u_int addr_cells, interrupt_cells;
	const u_int *imap, *imask;
	int imaplen, imasklen;
	u_int match[4];
	int index;

	if (pa->pa_intrpin == 0)
		return EINVAL;

	imap = fdtbus_get_prop(sc->sc_phandle, "interrupt-map", &imaplen);
	imask = fdtbus_get_prop(sc->sc_phandle, "interrupt-map-mask", &imasklen);
	if (imap == NULL || imask == NULL || imasklen != 16)
		return EINVAL;

	/* Convert attach args to specifier */
	match[0] = htobe32(
			__SHIFTIN(pa->pa_bus, PHYS_HI_BUS) |
			__SHIFTIN(pa->pa_device, PHYS_HI_DEVICE) |
			__SHIFTIN(pa->pa_function, PHYS_HI_FUNCTION)
		   ) & imask[0];
	match[1] = htobe32(0) & imask[1];
	match[2] = htobe32(0) & imask[2];
	match[3] = htobe32(pa->pa_intrpin) & imask[3];

	index = 0;
	while (imaplen >= 20) {
		const int map_ihandle = fdtbus_get_phandle_from_native(be32toh(imap[4]));
	        if (of_getprop_uint32(map_ihandle, "#address-cells", &addr_cells))
                	addr_cells = 2;
		if (of_getprop_uint32(map_ihandle, "#interrupt-cells", &interrupt_cells))
			interrupt_cells = 0;
		if (imaplen < (addr_cells + interrupt_cells) * 4)
			return ENXIO;

		if ((imap[0] & imask[0]) == match[0] &&
		    (imap[1] & imask[1]) == match[1] &&
		    (imap[2] & imask[2]) == match[2] &&
		    (imap[3] & imask[3]) == match[3]) {
			*ih = index;
			return 0;
		}

		imap += (5 + addr_cells + interrupt_cells);
		imaplen -= (5 + addr_cells + interrupt_cells) * 4;
		index++;
	}

	return EINVAL;
}

static const u_int *
pcihost_find_intr(struct pcihost_softc *sc, pci_intr_handle_t ih, int *pihandle)
{
	u_int addr_cells, interrupt_cells;
	int imaplen, index;
	const u_int *imap;

	imap = fdtbus_get_prop(sc->sc_phandle, "interrupt-map", &imaplen);
	KASSERT(imap != NULL);

	index = 0;
	while (imaplen >= 20) {
		const int map_ihandle = fdtbus_get_phandle_from_native(be32toh(imap[4]));
	        if (of_getprop_uint32(map_ihandle, "#address-cells", &addr_cells))
                	addr_cells = 2;
		if (of_getprop_uint32(map_ihandle, "#interrupt-cells", &interrupt_cells))
			interrupt_cells = 0;
		if (imaplen < (addr_cells + interrupt_cells) * 4)
			return NULL;

		if (index == ih) {
			*pihandle = map_ihandle;
			return imap + 5 + addr_cells;
		}

		imap += (5 + addr_cells + interrupt_cells);
		imaplen -= (5 + addr_cells + interrupt_cells) * 4;
		index++;
	}

	return NULL;
}

static const char *
pcihost_intr_string(void *v, pci_intr_handle_t ih, char *buf, size_t len)
{
	const int irq = __SHIFTOUT(ih, ARM_PCI_INTR_IRQ);
	const int vec = __SHIFTOUT(ih, ARM_PCI_INTR_MSI_VEC);
	struct pcihost_softc *sc = v;
	const u_int *specifier;
	int ihandle;

	if (ih & ARM_PCI_INTR_MSIX) {
		snprintf(buf, len, "irq %d (MSI-X vec %d)", irq, vec);
	} else if (ih & ARM_PCI_INTR_MSI) {
		snprintf(buf, len, "irq %d (MSI vec %d)", irq, vec);
	} else {
		specifier = pcihost_find_intr(sc, ih & IH_INDEX_MASK, &ihandle);
		if (specifier == NULL)
			return NULL;

		if (!fdtbus_intr_str_raw(ihandle, specifier, buf, len))
			return NULL;
	}

	return buf;
}

const struct evcnt *
pcihost_intr_evcnt(void *v, pci_intr_handle_t ih)
{
	return NULL;
}

static int
pcihost_intr_setattr(void *v, pci_intr_handle_t *ih, int attr, uint64_t data)
{
	switch (attr) {
	case PCI_INTR_MPSAFE:
		if (data)
			*ih |= IH_MPSAFE;
		else
			*ih &= ~IH_MPSAFE;
		return 0;
	default:
		return ENODEV;
	}
}

static void *
pcihost_intr_establish(void *v, pci_intr_handle_t ih, int ipl,
    int (*callback)(void *), void *arg, const char *xname)
{
	struct pcihost_softc *sc = v;
	const int flags = (ih & IH_MPSAFE) ? FDT_INTR_MPSAFE : 0;
	const u_int *specifier;
	int ihandle;

	if ((ih & (ARM_PCI_INTR_MSI | ARM_PCI_INTR_MSIX)) != 0)
		return arm_pci_msi_intr_establish(&sc->sc_pc, ih, ipl, callback, arg, xname);

	specifier = pcihost_find_intr(sc, ih & IH_INDEX_MASK, &ihandle);
	if (specifier == NULL)
		return NULL;

	return fdtbus_intr_establish_raw(ihandle, specifier, ipl, flags, callback, arg);
}

static void
pcihost_intr_disestablish(void *v, void *vih)
{
	struct pcihost_softc *sc = v;

	fdtbus_intr_disestablish(sc->sc_phandle, vih);
}

static int
pcihost_bus_space_map(void *t, bus_addr_t bpa, bus_size_t size, int flag,
    bus_space_handle_t *bshp)
{
	struct pcih_bus_space * const pbs = t;

	for (size_t i = 0; i < pbs->nranges; i++) {
		const bus_addr_t rmin = pbs->ranges[i].bpci;
		const bus_addr_t rmax = pbs->ranges[i].bpci - 1 + pbs->ranges[i].size;
		if ((bpa >= rmin) && ((bpa - 1 + size) <= rmax)) {
			return pbs->map(t, bpa - pbs->ranges[i].bpci + pbs->ranges[i].bbus, size, flag, bshp);
		}
	}

	return ERANGE;
}
