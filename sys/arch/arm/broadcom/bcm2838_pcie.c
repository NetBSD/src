/*	$NetBSD: bcm2838_pcie.c,v 1.6.2.1 2024/02/16 12:08:02 skrll Exp $ */

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Michael van Elst
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
__KERNEL_RCSID(0, "$NetBSD: bcm2838_pcie.c,v 1.6.2.1 2024/02/16 12:08:02 skrll Exp $");

#include <sys/param.h>
#include <sys/device.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include <dev/fdt/fdtvar.h>

#include <arch/arm/pci/pci_msi_machdep.h>
#include <arch/arm/broadcom/bcm2838_pcie.h>

#include <arch/evbarm/rpi/vcprop.h>
#include <dev/pci/pcidevs.h>

#define PHYS_HI_RELO		__BIT(31)
#define PHYS_HI_PREFETCH	__BIT(30)
#define PHYS_HI_ALIASED		__BIT(29)
#define PHYS_HI_SPACE		__BITS(25,24)
#define  PHYS_HI_SPACE_CFG	0
#define  PHYS_HI_SPACE_IO	1
#define  PHYS_HI_SPACE_MEM32	2
#define  PHYS_HI_SPACE_MEM64	3

#define CFG_OFFSET(b,d,f,r) ((b) << 16 | (d) << 1 | (f) << 8 | (r))

struct bcmstb_busspace {
	struct bus_space	bst;
	int			(*map)(void *, bus_addr_t, bus_size_t,
				    int, bus_space_handle_t *);
	int			flags;
	struct {
		bus_addr_t	bpci;
		bus_addr_t	bbus;
		bus_size_t	size;
	}			ranges[4];
	size_t			nranges;
};

struct bcmstb_softc {
        bus_space_tag_t         sc_bst;
        bus_space_handle_t      sc_bsh;
	bus_dma_tag_t		sc_dmat;

        kmutex_t                sc_lock;
        const char              *sc_name;

	int			sc_phandle;

	uint32_t		sc_bus_min;
	uint32_t		sc_bus_max;

	struct arm32_pci_chipset	sc_pc;

	struct bcmstb_busspace	sc_io;
	struct bcmstb_busspace	sc_mem;

	int			sc_pci_flags;
};

static void	bcmstb_attach(device_t, struct bcmstb_softc *);
static int	bcmstb_config(struct bcmstb_softc *);
static int	bcmstb_setup(struct bcmstb_softc *);
static void	bcmstb_attach_hook(device_t, device_t, struct pcibus_attach_args *);
static int	bcmstb_bus_maxdevs(void *, int);
static pcitag_t bcmstb_make_tag(void *, int, int, int);
static void	bcmstb_decompose_tag(void *, pcitag_t, int *, int *, int *);
static u_int	bcmstb_get_segment(void *);
static pcireg_t bcmstb_conf_read(void *, pcitag_t, int);
static void	bcmstb_conf_write(void *, pcitag_t, int, pcireg_t);
static int	bcmstb_conf_hook(void *, int, int, int, pcireg_t);
static void	bcmstb_conf_interrupt(void *, int, int, int, int, int *);

static int			bcmstb_intr_map(const struct pci_attach_args *, pci_intr_handle_t *);
static const char		*bcmstb_intr_string(void *, pci_intr_handle_t, char *, size_t);
static const struct evcnt	*bcmstb_intr_evcnt(void *, pci_intr_handle_t);
static int			bcmstb_intr_setattr(void *, pci_intr_handle_t *, int, uint64_t);
static void			*bcmstb_intr_establish(void *, pci_intr_handle_t, int,
				    int (*)(void *), void *, const char *);
static void			bcmstb_intr_disestablish(void *, void *);
static int			bcmstb_bus_space_map(void *, bus_addr_t,
				    bus_size_t, int, bus_space_handle_t *);

struct bcm2838pcie_softc {
	device_t		sc_dev;
	struct bcmstb_softc	sc_bcmstb;
};

static int bcm2838pcie_match(device_t, cfdata_t, void *);
static void bcm2838pcie_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(bcm2838pcie_fdt, sizeof(struct bcm2838pcie_softc),
    bcm2838pcie_match, bcm2838pcie_attach, NULL, NULL);


static inline void
stb_write(struct bcmstb_softc *sc, int r, uint32_t v)
{
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, r, v);
}
static inline uint32_t
stb_read(struct bcmstb_softc *sc, int r)
{
	uint32_t v;

	v = bus_space_read_4(sc->sc_bst, sc->sc_bsh, r);

	return v;
}
static inline void
stb_setbits(struct bcmstb_softc *sc, int r, uint32_t clr, uint32_t set)
{
	uint32_t w;

	w = stb_read(sc, r);
	w = (w & ~clr) | set;
	stb_write(sc, r, w);
}
#define STBWRITE(sc, r, v)  stb_write((sc), (r), (v))
#define STBREAD(sc, r)      stb_read((sc), (r))
#define STBRMW(sc, r, c, s) stb_setbits((sc), (r), (c), (s))

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "brcm,pci-plat-dev" },
	{ .compat = "brcm,bcm2711-pcie" },
	DEVICE_COMPAT_EOL
};

/* ARGSUSED */
static int
bcm2838pcie_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
bcm2838pcie_attach(device_t parent, device_t self, void *aux)
{
	struct bcm2838pcie_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;
	bus_dma_tag_t dmat;
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	int error;

	sc->sc_dev = self;

	mutex_init(&sc->sc_bcmstb.sc_lock, MUTEX_DEFAULT, IPL_HIGH);

	error = fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size);
	if (error) {
		aprint_error_dev(sc->sc_dev, ": couldn't get registers\n");
		return;
	}

	// bst = faa->faa_bst;
	extern struct bus_space arm_generic_bs_tag;
	bst = &arm_generic_bs_tag;

	if (bus_space_map(faa->faa_bst, addr, size, 0, &bsh)) {
		aprint_error_dev(sc->sc_dev, ": unable to map device\n");
		return;
	}

	/* RPI4 limits PCIe DMA to the first 3GB */
	error = bus_dmatag_subregion(faa->faa_dmat, 0, 0xbfffffff,
		&dmat, BUS_DMA_WAITOK);

	if (error == EOPNOTSUPP) {
		/* assume default DMA tag is fine */
		dmat = faa->faa_dmat;
	} else if (error) {
		aprint_error_dev(sc->sc_dev, ": unable to subregion DMA\n");
		bus_space_unmap(faa->faa_bst, bsh, size);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Broadcom PCIE Host Controller\n");

	if (error == 0)
		aprint_normal_dev(sc->sc_dev, "Using 3GB DMA subregion\n");

	sc->sc_bcmstb.sc_bst = bst;
	sc->sc_bcmstb.sc_bsh = bsh;
	sc->sc_bcmstb.sc_dmat = dmat;
	sc->sc_bcmstb.sc_name = device_xname(sc->sc_dev);
	sc->sc_bcmstb.sc_phandle = faa->faa_phandle;

	bcmstb_attach(sc->sc_dev, &sc->sc_bcmstb);
}

static void
bcmstb_attach(device_t self, struct bcmstb_softc *sc)
{
	struct arm32_pci_chipset *pc;
	struct pcibus_attach_args pba;
	int error;

	// fdtbus_register_interrupt_controller(self, OF_child(sc->sc_phandle),
	  //           &bcmstb_intrfuncs);

	pc = &sc->sc_pc;

	pc->pc_conf_v = (void *)sc;
	pc->pc_attach_hook = bcmstb_attach_hook;
	pc->pc_bus_maxdevs = bcmstb_bus_maxdevs;
	pc->pc_make_tag = bcmstb_make_tag;
	pc->pc_decompose_tag = bcmstb_decompose_tag;
	pc->pc_get_segment = bcmstb_get_segment;
	pc->pc_conf_read = bcmstb_conf_read;
	pc->pc_conf_write = bcmstb_conf_write;
	pc->pc_conf_hook = bcmstb_conf_hook;
	pc->pc_conf_interrupt = bcmstb_conf_interrupt;

	pc->pc_intr_v = (void *)sc;
	pc->pc_intr_map = bcmstb_intr_map;
	pc->pc_intr_string = bcmstb_intr_string;
	pc->pc_intr_evcnt = bcmstb_intr_evcnt;
	pc->pc_intr_setattr = bcmstb_intr_setattr;
	pc->pc_intr_establish = bcmstb_intr_establish;
	pc->pc_intr_disestablish = bcmstb_intr_disestablish;


	/* XXX bus-range */
	sc->sc_bus_min = 0x00;
	sc->sc_bus_max = 0x01;

	error = bcmstb_config(sc);
	if (error) {
		aprint_error_dev(self, "configuration failed: %d\n", error);
		return;
	}

	memset(&pba, 0, sizeof(pba));
	pba.pba_flags = sc->sc_pci_flags;
	pba.pba_iot = &sc->sc_io.bst;
	pba.pba_memt = &sc->sc_mem.bst;
	pba.pba_dmat = sc->sc_dmat;
#ifdef _PCI_HAVE_DMA64
	pba.pba_dmat64 = sc->sc_dmat;
#endif
	pba.pba_pc = pc;
	pba.pba_bus = sc->sc_bus_min;

	config_found(self, &pba, pcibusprint,
	    CFARGS(.devhandle = device_handle(self)));
}

static void
bcmstb_makespace(struct bcmstb_softc *sc, struct bcmstb_busspace *bs, int flags)
{
	bs->bst = *sc->sc_bst;
	bs->bst.bs_cookie = bs;
	bs->map = bs->bst.bs_map;
	bs->bst.bs_map = bcmstb_bus_space_map;
	bs->flags = flags;
}

static int
bcmstb_addrange(struct bcmstb_busspace *bs, bus_addr_t pci, bus_addr_t bus, bus_size_t sz)
{
	if (bs->nranges >= __arraycount(bs->ranges))
		return -1;

	bs->ranges[bs->nranges].bpci = pci;
	bs->ranges[bs->nranges].bbus = bus;
	bs->ranges[bs->nranges].size = sz;
	++bs->nranges;

	return 0;
}

static int
bcmstb_config(struct bcmstb_softc *sc)
{
	const u_int *ranges;
	int len, type, error;
	struct pciconf_resources *pcires;
	uint32_t phys_hi;
	uint64_t bus_phys, cpu_phys, size;

	bcmstb_makespace(sc, &sc->sc_io, PCI_FLAGS_IO_OKAY);
	bcmstb_makespace(sc, &sc->sc_mem, PCI_FLAGS_MEM_OKAY);

	ranges = fdtbus_get_prop(sc->sc_phandle, "ranges", &len);
	if (ranges == NULL) {
		aprint_error("%s: missing 'ranges' property\n", sc->sc_name);
		return ENXIO;
	}

	pcires = pciconf_resource_init();

	while (len >= 28) {
		phys_hi = be32toh(ranges[0]);
		bus_phys = ((uint64_t)be32toh(ranges[1])) << 32 | be32toh(ranges[2]);
		cpu_phys = ((uint64_t)be32toh(ranges[3])) << 32 | be32toh(ranges[4]);
		size     = ((uint64_t)be32toh(ranges[5])) << 32 | be32toh(ranges[6]);

		len -= 28;
		ranges += 7;

		switch (__SHIFTOUT(phys_hi, PHYS_HI_SPACE)) {
		case PHYS_HI_SPACE_IO:
			if (bcmstb_addrange(&sc->sc_io, bus_phys, cpu_phys, size)) {
				aprint_error("%s: too many IO ranges\n", sc->sc_name);
				continue;
			}
			type = PCICONF_RESOURCE_IO;

			aprint_verbose("%s: IO: 0x%" PRIx64 "+0x%" PRIx64 "@0x%" PRIx64 "\n",
			    sc->sc_name,
			    bus_phys, size, cpu_phys);

			error = pciconf_resource_add(pcires, type, bus_phys, size);
			if (error == 0)
				sc->sc_pci_flags |= PCI_FLAGS_IO_OKAY;
			break;
		case PHYS_HI_SPACE_MEM64:
			/* FALLTHROUGH */
		case PHYS_HI_SPACE_MEM32:
			if (bcmstb_addrange(&sc->sc_mem, bus_phys, cpu_phys, size)) {
				aprint_error("%s: too many mem ranges\n", sc->sc_name);
				continue;
			}
			if ((phys_hi & PHYS_HI_PREFETCH) != 0 ||
			    __SHIFTOUT(phys_hi, PHYS_HI_SPACE) == PHYS_HI_SPACE_MEM64) {
				type = PCICONF_RESOURCE_PREFETCHABLE_MEM;
			} else {
				type = PCICONF_RESOURCE_MEM;
			}

			aprint_verbose("%s: MMIO (%d-bit%s): 0x%" PRIx64 "+0x%" PRIx64 "@0x%" PRIx64 "\n",
			    sc->sc_name,
			    __SHIFTOUT(phys_hi, PHYS_HI_SPACE) == PHYS_HI_SPACE_MEM64 ? 64 : 32,
			    type == PCICONF_RESOURCE_PREFETCHABLE_MEM ? " prefetchable" : "",
			    bus_phys, size, cpu_phys);

			error = pciconf_resource_add(pcires, type, bus_phys, size);
			if (error == 0)
				sc->sc_pci_flags |= PCI_FLAGS_MEM_OKAY;
			break;
		default:
			break;
		}
	}

	error = bcmstb_setup(sc);
	if (error)
		return error;

	error = pci_configure_bus(&sc->sc_pc, pcires, sc->sc_bus_min, 64);
	pciconf_resource_fini(pcires);

	return error;
}

static void
bcmstb_setwin(struct bcmstb_softc *sc, int win, uint64_t pa, uint64_t ca,
    uint64_t sz)
{
	uint32_t base, limit;

	STBWRITE(sc, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LO(win), pa);
	STBWRITE(sc, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_HI(win), pa >> 32);

	base = (ca >> 20) & 0xfff;
	limit = ((ca + sz - 1) >> 20) & 0xfff;

	STBRMW(sc, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT(win),
	    PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT_BASE
	    | PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT_LIMIT,
	    __SHIFTIN(base, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT_BASE)
	    | __SHIFTIN(limit, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT_LIMIT));

	base = (ca >> 32) & 0xff;
	limit = ((ca + sz - 1) >> 32) & 0xff;

	STBRMW(sc, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_HI(win),
	    PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_HI_BASE,
	    __SHIFTIN(base, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_HI_BASE));

	STBRMW(sc, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LIMIT_HI(win),
	    PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LIMIT_HI_LIMIT,
	    __SHIFTIN(base, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LIMIT_HI_LIMIT));
}

static int
bcmstb_encode_size(int bits)
{
	/* 4K .. 32K */
	if (bits >= 12 && bits <= 15)
		return 28 + (bits - 12);

	/* 64K .. 32G */
	if (bits >= 16 && bits <= 35)
		return 1 + (bits - 16);

	/* invalid */
	return 0;
}

static int
bcmstb_setup(struct bcmstb_softc *sc)
{
        struct bcmstb_busspace * const bs = &sc->sc_mem;
	uint32_t w, m;
	uint64_t ad;
	int t, i, sz;

	/* Reset */
	STBRMW(sc, PCIE_RGR1_SW_INIT_1, 0, PCIE_RGR1_SW_INIT_1_INIT);
	delay(200);
	STBRMW(sc, PCIE_RGR1_SW_INIT_1, PCIE_RGR1_SW_INIT_1_INIT, 0);

	/* Clear IDDQ */
	STBRMW(sc, PCIE_MISC_HARD_PCIE_HARD_DEBUG,
	    PCIE_MISC_HARD_PCIE_HARD_DEBUG_SERDES_IDDQ, 0);
	w = STBREAD(sc, PCIE_MISC_HARD_PCIE_HARD_DEBUG);

	delay(100);

	w = STBREAD(sc, PCIE_MISC_REVISION);
	printf("RootBridge Revision %x\n",
	    (u_int)__SHIFTOUT(w, PCIE_MISC_REVISION_MAJMIN));

	STBRMW(sc, PCIE_MISC_MISC_CTRL,
	    PCIE_MISC_MISC_CTRL_MAX_BURST_SIZE, /* 128B */
	    PCIE_MISC_MISC_CTRL_SCB_ACCESS_EN
	    | PCIE_MISC_MISC_CTRL_CFG_READ_UR_MODE);

	/*
	 * XXX Inbound window for RPI4 is 3GB, should be parsed
	 * from dma-ranges attribute
	*/
	ad = 0;
	sz = bcmstb_encode_size(32);

	w = (__SHIFTIN(ad, PCIE_MISC_RC_BARx_CONFIG_LO_MATCH_ADDRESS)
	     & PCIE_MISC_RC_BARx_CONFIG_LO_MATCH_ADDRESS)
	    | __SHIFTIN(sz, PCIE_MISC_RC_BARx_CONFIG_LO_SIZE);
	STBWRITE(sc, PCIE_MISC_RC_BAR2_CONFIG_LO, w);
	STBWRITE(sc, PCIE_MISC_RC_BAR2_CONFIG_HI, ad >> 32);

	STBRMW(sc, PCIE_MISC_MISC_CTRL,
	    PCIE_MISC_MISC_CTRL_SCB0_SIZE,
	    __SHIFTIN(sz, PCIE_MISC_MISC_CTRL_SCB0_SIZE));

	/* disable PCIe->GISB window */
	STBWRITE(sc, PCIE_MISC_RC_BAR1_CONFIG_LO, 0);
	STBWRITE(sc, PCIE_MISC_RC_BAR1_CONFIG_HI, 0);
	/* disable PCIe->SCB window */
	STBWRITE(sc, PCIE_MISC_RC_BAR3_CONFIG_LO, 0);
	STBWRITE(sc, PCIE_MISC_RC_BAR3_CONFIG_HI, 0);

	for (i=0; i<bs->nranges; ++i) {
		bcmstb_setwin(sc, i,
		    bs->ranges[i].bpci,
		    bs->ranges[i].bbus,
		    bs->ranges[i].size);
	}

	STBWRITE(sc, PCIE_INTR2_MASK_CLR, ~0);
	STBWRITE(sc, PCIE_INTR2_MASK_SET, ~0);

	/* Release PERST */
	STBRMW(sc, PCIE_RGR1_SW_INIT_1, PCIE_RGR1_SW_INIT_1_PERST, 0);

	t = 100;
	m = PCIE_MISC_PCIE_STATUS_PCIE_PHYLINKUP
	  | PCIE_MISC_PCIE_STATUS_PCIE_DL_ACTIVE;
	do {
		w = STBREAD(sc, PCIE_MISC_PCIE_STATUS);
		delay(1000);
		--t;
	} while ((w & m) != m && t > 0);
	if ((w & m) != m) {
		printf("PCIe link not ready\n");
		return 1;
	}

	m = PCIE_MISC_PCIE_STATUS_PCIE_PORT;
	if ((w & m) != m) {
		printf("PCIe link not in RC mode\n");
		return 1;
	}

	/* advertise L0 and L1s capability */
	STBRMW(sc, PCIE_RC_CFG_PRIV1_LINK_CAPABILITY,
	    PCIE_RC_CFG_PRIV1_LINK_CAPABILITY_ASPM_SUPPORT,
	    __SHIFTIN(0x3, PCIE_RC_CFG_PRIV1_LINK_CAPABILITY_ASPM_SUPPORT));

	/* present as PCIe->PCIe bridge */
	STBRMW(sc, PCIE_RC_CFG_PRIV1_ID_VAL3,
	    PCIE_RC_CFG_PRIV1_ID_VAL3_CLASS_CODE,
	    __SHIFTIN(0x60400, PCIE_RC_CFG_PRIV1_ID_VAL3_CLASS_CODE));

	/* XXX enable SSC */

	/* clear endian mode == little endian */
	STBRMW(sc, PCIE_RC_CFG_VENDOR_VENDOR_SPECIFIC_REG1,
	    PCIE_RC_CFG_VENDOR_VENDOR_SPECIFIC_REG1_ENDIAN_MODE_BAR2,
	    0);

	/* Gate clock with CLKREQ# */
	STBRMW(sc, PCIE_MISC_HARD_PCIE_HARD_DEBUG,
	    0,
	    PCIE_MISC_HARD_PCIE_HARD_DEBUG_CLKREQ);

	return 0;
}

static void
bcmstb_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{
}

static int
bcmstb_bus_maxdevs(void *v, int bus)
{
	// struct bcmstb_softc *sc = v;

	return 1;
}

static pcitag_t
bcmstb_make_tag(void *v, int bus, int device, int function)
{
	return (bus << 20) | (device << 15) | (function << 12);
}

static void
bcmstb_decompose_tag(void *v, pcitag_t tag, int *bp, int *dp, int *fp)
{
	if (bp != NULL)
		*bp = (tag >> 20) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 15) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 12) & 0x7;
}

static u_int
bcmstb_get_segment(void *v)
{
	// struct bcmstb_softc *sc = v;

	return 1;
}

static pcireg_t
bcmstb_conf_read(void *v, pcitag_t tag, int offset)
{
	struct bcmstb_softc *sc = v;
	int bus, dev, fn;
	uint32_t idx, reg;
	pcireg_t data;

	if (offset < 0 || offset > 4092)
		return (pcireg_t) -1;

	bcmstb_decompose_tag(v, tag, &bus, &dev, &fn);

	if (bus < 2 && dev > 0)
		return (pcireg_t) -1;

	idx = __SHIFTIN(bus, PCIE_EXT_CFG_INDEX_BUSNUM)
	    | __SHIFTIN(dev, PCIE_EXT_CFG_INDEX_SLOT)
	    | __SHIFTIN(fn, PCIE_EXT_CFG_INDEX_FUNC);

	reg = bus > 0 ? PCIE_EXT_CFG_DATA + offset : offset;

	mutex_enter(&sc->sc_lock);
	STBWRITE(sc, PCIE_EXT_CFG_INDEX, idx);
	data = STBREAD(sc, reg);
	mutex_exit(&sc->sc_lock);

	return data;
}

static void
bcmstb_conf_write(void *v, pcitag_t tag, int offset, pcireg_t data)
{
	struct bcmstb_softc *sc = v;
	int bus, dev, fn;
	uint32_t idx, reg;

	if (offset < 0 || offset > 4092)
		return;

	bcmstb_decompose_tag(v, tag, &bus, &dev, &fn);

	if (bus < 2 && dev > 0)
		return;

	idx = __SHIFTIN(bus, PCIE_EXT_CFG_INDEX_BUSNUM)
	    | __SHIFTIN(dev, PCIE_EXT_CFG_INDEX_SLOT)
	    | __SHIFTIN(fn, PCIE_EXT_CFG_INDEX_FUNC);

	reg = bus > 0 ? PCIE_EXT_CFG_DATA + offset : offset;

	mutex_enter(&sc->sc_lock);
	STBWRITE(sc, PCIE_EXT_CFG_INDEX, idx);
	STBWRITE(sc, reg, data);
	mutex_exit(&sc->sc_lock);
}

static int
bcmstb_conf_hook(void *v, int bus, int dev, int fn, pcireg_t id)
{
	/*
	 * Newer RPi4 lacks the VL805 EEPROM, so the
	 * firmware needs to be loaded after a reset.
	 * Trigger the GPU to do this.
	 */

	if (bus == 1 && dev == 0 && fn == 0 &&
	    PCI_VENDOR(id) == PCI_VENDOR_VIATECH &&
	    PCI_PRODUCT(id) == PCI_PRODUCT_VIATECH_VL805_XHCI) {

		uint32_t idx;
		idx = __SHIFTIN(bus, PCIE_EXT_CFG_INDEX_BUSNUM)
		    | __SHIFTIN(dev, PCIE_EXT_CFG_INDEX_SLOT)
		    | __SHIFTIN(fn, PCIE_EXT_CFG_INDEX_FUNC);

		rpi_notify_xhci_reset(idx);
	}

	return PCI_CONF_DEFAULT;
}

static void
bcmstb_conf_interrupt(void *v, int bus, int dev, int ipin, int swiz, int *ilinep)
{
}

static int
bcmstb_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ih)
{
        struct bcmstb_softc *sc = pa->pa_pc->pc_intr_v;
        u_int addr_cells, interrupt_cells;
        const u_int *imap, *imask;
        int imaplen, imasklen;
        u_int match[4];
        int index, off;

        if (pa->pa_intrpin == 0)
                return EINVAL;

        imap = fdtbus_get_prop(sc->sc_phandle, "interrupt-map", &imaplen);
        imask = fdtbus_get_prop(sc->sc_phandle, "interrupt-map-mask", &imasklen)
;
        if (imap == NULL || imask == NULL || imasklen != 16)
                return EINVAL;

	off = CFG_OFFSET(pa->pa_bus, pa->pa_device, pa->pa_function, 0);

        /* Convert attach args to specifier */
        match[0] = htobe32(off) & imask[0];
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
bcmstb_find_intr(struct bcmstb_softc *sc, pci_intr_handle_t ih, int *pihandle)
{
	u_int addr_cells, int_cells;
	u_int iaddr_cells, iint_cells;
	int imaplen, index;
	const u_int *imap;
	int intc;

	imap = fdtbus_get_prop(sc->sc_phandle, "interrupt-map", &imaplen);
	if (imap == NULL)
		return NULL;
	imaplen /= 4;

	if (of_getprop_uint32(sc->sc_phandle, "#address-cells", &addr_cells))
		return NULL;

	if (of_getprop_uint32(sc->sc_phandle, "#interrupt-cells", &int_cells))
		return NULL;

	index = 0;
	while (imaplen >= int_cells + addr_cells + 1) {

		intc = fdtbus_get_phandle_from_native(be32toh(imap[int_cells + addr_cells]));
		if (of_getprop_uint32(intc, "#interrupt-cells", &iint_cells))
			break;

		if (of_getprop_uint32(intc, "#address-cells", &iaddr_cells))
			iaddr_cells = 0;

		imap += addr_cells + int_cells + 1;
		imaplen -= addr_cells + int_cells + 1;

		if (imaplen < iint_cells + iaddr_cells)
			break;

		/* XXX, should really match child */
		if (index == ih) {
			*pihandle = intc;
			return imap;
		}

		imap += iaddr_cells + iint_cells;
		imaplen -= iaddr_cells + iint_cells;
		index++;
	}

	return NULL;
}

static const char *
bcmstb_intr_string(void *v, pci_intr_handle_t ih, char *buf, size_t len)
{
	struct bcmstb_softc *sc = v;
	const int irq = __SHIFTOUT(ih, ARM_PCI_INTR_IRQ);
	const int vec = __SHIFTOUT(ih, ARM_PCI_INTR_MSI_VEC);
	const u_int *specifier;
	int ihandle;

	if (ih & ARM_PCI_INTR_MSIX) {
		snprintf(buf, len, "irq %d (MSI-X vec %d)", irq, vec);
	} else if (ih & ARM_PCI_INTR_MSI) {
		snprintf(buf, len, "irq %d (MSI vec %d)", irq, vec);
	} else {
		specifier = bcmstb_find_intr(sc, ih & ARM_PCI_INTR_IRQ, &ihandle);
		if (specifier == NULL)
			return NULL;
		if (!fdtbus_intr_str_raw(ihandle, specifier, buf, len))
			return NULL;
	}

	return buf;
}

static const struct evcnt *
bcmstb_intr_evcnt(void *v, pci_intr_handle_t ih)
{
	return NULL;
}

static int
bcmstb_intr_setattr(void *v, pci_intr_handle_t *ih, int attr, uint64_t data)
{
	switch (attr) {
	default:
		return ENODEV;
	}
}

static void *
bcmstb_intr_establish(void *v, pci_intr_handle_t ih, int ipl,
    int (*callback)(void *), void *arg, const char *xname)
{
	struct bcmstb_softc *sc = v;
	const int flags = (ih & ARM_PCI_INTR_MPSAFE) ? FDT_INTR_MPSAFE : 0;
	const u_int *specifier;
	int ihandle;

	if ((ih & (ARM_PCI_INTR_MSI | ARM_PCI_INTR_MSIX)) != 0) {
//		return arm_pci_msi_intr_establish(&sc->sc_pc, ih, ipl,
//		                                  callback, arg, xname);
		return NULL;
	}

	/* should search for PCI device */
	specifier = bcmstb_find_intr(sc, ih & ARM_PCI_INTR_IRQ, &ihandle);
	if (specifier == NULL)
		return NULL;

	return fdtbus_intr_establish_raw(ihandle, specifier, ipl, flags,
	                                 callback, arg, xname);
}

static void
bcmstb_intr_disestablish(void *v, void *vih)
{
	struct bcmstb_softc *sc = v;

	fdtbus_intr_disestablish(sc->sc_phandle, vih);
}

static int
bcmstb_bus_space_map(void *t, bus_addr_t bpa, bus_size_t size, int flag,
    bus_space_handle_t *bshp)
{
        struct bcmstb_busspace * const bs = t;

//        if ((bs->flags & PCI_FLAGS_IO_OKAY) != 0) {
                /* Force strongly ordered mapping for all I/O space */
                flag = BUS_SPACE_MAP_NONPOSTED;
//        }

        for (size_t i = 0; i < bs->nranges; i++) {
                const bus_addr_t rmin = bs->ranges[i].bpci;
                const bus_addr_t rmax = bs->ranges[i].bpci - 1 + bs->ranges[i]
.size;
                if ((bpa >= rmin) && ((bpa - 1 + size) <= rmax)) {
			const bus_addr_t pa = bs->ranges[i].bbus + (bpa - rmin);

                        return bs->map(t, pa, size, flag, bshp);
                }
        }

        return ERANGE;
}

