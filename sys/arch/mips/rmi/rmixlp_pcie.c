/*	$NetBSD: rmixlp_pcie.c,v 1.1.2.6 2011/12/31 08:20:43 matt Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * PCI configuration support for RMI XLS SoC
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rmixlp_pcie.c,v 1.1.2.6 2011/12/31 08:20:43 matt Exp $");

#include "opt_pci.h"
#include "pci.h"

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/kernel.h>		/* for 'hz' */
#include <sys/cpu.h>
#include <sys/bus.h>
#include <sys/intr.h>

#include <uvm/uvm_extern.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>

#include <mips/rmi/rmixl_intr.h>

#include <evbmips/rmixl/autoconf.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciconf.h>

#ifdef PCI_NETBSD_CONFIGURE
#include <mips/cache.h>
#endif

#include <machine/pci_machdep.h>

#ifdef PCI_DEBUG
int rmixlp_pcie_debug = PCI_DEBUG;
# define DPRINTF(x)	do { if (rmixlp_pcie_debug) printf x ; } while (0)
#else
# define DPRINTF(x)
#endif

#ifndef DDB
# define STATIC static
#else
# define STATIC
#endif

typedef struct {
	rmixlp_variant_t lnk_variant;
	uint8_t lnk_ports;
	uint8_t lnk_plc_mask;
	uint8_t lnk_plc_match;
	uint8_t lnk_lanes[4];
} rmixlp_pcie_lnkcfg_t;

static struct rmixlp_pcie_softc {
	device_t	 		sc_dev;
	pci_chipset_tag_t		sc_pc;	 /* in rmixl_configuration */
	bus_space_tag_t			sc_pci_cfg_memt;
	bus_space_tag_t			sc_pci_ecfg_eb_memt;
	bus_space_tag_t			sc_pci_ecfg_el_memt;
	bus_dma_tag_t			sc_dmat29;
	bus_dma_tag_t			sc_dmat32;
	bus_dma_tag_t			sc_dmat64;
	rmixlp_pcie_lnkcfg_t		sc_lnkcfg;
	uint8_t				sc_lnkmode;
	bool				sc_usb_bswapped;
	kmutex_t			sc_mutex;
} rmixlp_pcie_softc = {
	.sc_pc = &rmixl_configuration.rc_pci_chipset,
	.sc_pci_cfg_memt = &rmixl_configuration.rc_pci_cfg_memt,
	.sc_pci_ecfg_eb_memt = &rmixl_configuration.rc_pci_ecfg_eb_memt,
	.sc_pci_ecfg_el_memt = &rmixl_configuration.rc_pci_ecfg_el_memt,
};

static int	rmixlp_pcie_match(device_t, cfdata_t, void *);
static void	rmixlp_pcie_attach(device_t, device_t, void *);
static void	rmixlp_pcie_bar_alloc(struct rmixlp_pcie_softc *,
		    struct rmixl_region *, u_long, u_long);
static void	rmixlp_pcie_attach_hook(device_t, device_t,
		    struct pcibus_attach_args *);

static void	rmixlp_pcie_lnkcfg_get(struct rmixlp_pcie_softc *);
static void	rmixlp_pcie_usb_init_hook(pci_chipset_tag_t, pcitag_t);

static void	rmixlp_pcie_conf_interrupt(void *, int, int, int, int, int *);
static int	rmixlp_pcie_bus_maxdevs(void *, int);
static pcitag_t	rmixlp_pcie_make_tag(void *, int, int, int);
static void	rmixlp_pcie_decompose_tag(void *, pcitag_t, int *, int *,
		    int *);
void		rmixlp_pcie_print_tag(const char * restrict, void *, pcitag_t,
		    int, vaddr_t, u_long);
static pcireg_t	rmixlp_pcie_conf_read(void *, pcitag_t, int);
static void	rmixlp_pcie_conf_write(void *, pcitag_t, int, pcireg_t);
#ifdef PCI_NETBSD_CONFIGURE
static void	rmixlp_pcie_configure_bus(struct rmixlp_pcie_softc *);
#endif
#ifdef __PCI_DEV_FUNCORDER
static bool	rmixlp_pcie_dev_funcorder(void *, int, int, int, char *);
#endif
#ifdef __HAVE_PCI_CONF_HOOK
static int	rmixlp_pcie_conf_hook(void *, int, int, int, pcireg_t);
#endif

static int	rmixlp_pcie_intr_map(struct pci_attach_args *,
		    pci_intr_handle_t *);
static const char *
		rmixlp_pcie_intr_string(void *, pci_intr_handle_t);
static const struct evcnt *
		rmixlp_pcie_intr_evcnt(void *, pci_intr_handle_t);
#if 0
static pci_intr_handle_t
		rmixlp_pcie_make_pih(u_int, u_int, u_int);
static void	rmixlp_pcie_decompose_pih(pci_intr_handle_t, u_int *, u_int *, u_int *);
#endif
static void	rmixlp_pcie_intr_disestablish(void *, void *);
static void *	rmixlp_pcie_intr_establish(void *, pci_intr_handle_t,
		    int, int (*)(void *), void *);

static const rmixlp_pcie_lnkcfg_t rmixlp_pcie_lnkcfgs[] = {
	{ RMIXLP_3XXL,	1, 0, 0, { 4, 0, 0, 0 } },
	{ RMIXLP_3XXH,	2, 0, 0, { 2, 0, 2, 0 } },
	{ RMIXLP_3XXQ,	4, 0, 0, { 1, 1, 1, 1 } },
	{ RMIXLP_ANY,	4, 3, 0, { 8, 0, 8, 0 } },	/* 8XX, 4XX, 3XX */
	{ RMIXLP_ANY,	4, 3, 1, { 4, 4, 8, 0 } },	/* 8XX, 4XX, 3XX */
	{ RMIXLP_ANY,	4, 3, 2, { 8, 0, 4, 4 } },	/* 8XX, 4XX, 3XX */
	{ RMIXLP_ANY,	4, 3, 3, { 4, 4, 4, 4 } },	/* 8XX, 4XX, 3XX */
};

CFATTACH_DECL_NEW(rmixlp_pcie, 0,
    rmixlp_pcie_match, rmixlp_pcie_attach, NULL, NULL); 

static int  
rmixlp_pcie_match(device_t parent, cfdata_t cf, void *aux)
{        
	/*
	 * A PCIe interface at mainbus exists only on XLP chips.
	 */
	if (! cpu_rmixlp(mips_options.mips_cpu))
		return 0;

	/*
	 * There can be only one!
	 */
	if (rmixlp_pcie_softc.sc_dev != NULL)
		return 0;

	return 1;
}    

static void 
rmixlp_pcie_attach(device_t parent, device_t self, void *aux)
{
	struct rmixl_config * const rcp = &rmixl_configuration;
	struct rmixlp_pcie_softc * const sc = &rmixlp_pcie_softc;
	struct mainbus_attach_args * const ma = aux;
        struct pcibus_attach_args pba;

	self->dv_private = sc;
	sc->sc_dev = self;

	aprint_normal(": RMI XLP PCIe Interface\n");
	aprint_naive(": RMI XLP PCIe Interface\n");

	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_HIGH);

	sc->sc_dmat29 = ma->ma_dmat29;
	sc->sc_dmat32 = ma->ma_dmat32;
	sc->sc_dmat64 = ma->ma_dmat64;
	sc->sc_pc = &rcp->rc_pci_chipset;

	KASSERT(rcp->rc_pci_ecfg.r_pbase != (bus_addr_t)-1);
	KASSERT(rcp->rc_pci_ecfg.r_size != 0);

	/*
	 * Get our link configuration.
	 */
	rmixlp_pcie_lnkcfg_get(sc);

	aprint_debug_dev(sc->sc_dev, "using link variant %d (system is %d)\n",
	    sc->sc_lnkcfg.lnk_variant, rmixl_xlp_variant);
	for (size_t port = 0; port < sc->sc_lnkcfg.lnk_ports; port++) {
		if (sc->sc_lnkcfg.lnk_lanes[port] == 0)
			continue;
		const char modes[2][3] = { "EP", "RC" };
		char confstr[8];
		size_t mode = (sc->sc_lnkmode & __BIT(port)) != 0;
		snprintf(confstr, sizeof(confstr), "%sx%u",
		    modes[mode], sc->sc_lnkcfg.lnk_lanes[port]);
		aprint_normal_dev(sc->sc_dev, "port %zu: %s\n", port, confstr);
	}

	/*
	 * initialize the PCI MEM and IO bus space tags
	 */
	if (rcp->rc_pci_memt.bs_cookie == NULL) {
#ifdef PCI_NETBSD_CONFIGURE
		for (size_t i = 0; i < RMIXLP_SBC_NPCIE_MEM; i++) {
			struct rmixl_region * const rp = &rcp->rc_pci_link_mem[i];
			if (rp->r_pbase != (bus_addr_t)-1) {
				rmixlp_write_4(RMIXLP_SBC_PCITAG,
				    RMIXLP_SBC_PCIE_MEM_BASEn(i), 0xfffff000);
				rmixlp_write_4(RMIXLP_SBC_PCITAG,
				    RMIXLP_SBC_PCIE_MEM_LIMITn(i), 0x00000000);
				extent_free(rcp->rc_phys_ex,
				    rp->r_pbase >> 20, rp->r_size >> 20,
				    EX_NOWAIT);
				rp->r_pbase = (bus_addr_t) -1;
				rp->r_size = 0;
			}
		}
		if (0)
			rmixlp_pcie_bar_alloc(sc, &rcp->rc_pci_mem, 256, 1);
		rcp->rc_pci_mem.r_pbase = rmixlp_read_4(RMIXLP_EHCI0_PCITAG,
		    PCI_BAR0) & -8;
		rcp->rc_pci_mem.r_size = 256 << 20;
		extent_alloc_region(rcp->rc_phys_ex, 
		    rcp->rc_pci_mem.r_pbase >> 20,
		    rcp->rc_pci_mem.r_size >> 20,
		    EX_NOWAIT);
#endif
		rmixl_pci_bus_mem_init(&rcp->rc_pci_memt, rcp);
	}

	if (rcp->rc_pci_iot.bs_cookie == NULL) {
#ifdef PCI_NETBSD_CONFIGURE
		for (size_t i = 0; i < RMIXLP_SBC_NPCIE_IO; i++) {
			struct rmixl_region * const rp = &rcp->rc_pci_link_io[i];
			if (rp->r_pbase != (bus_addr_t)-1) {
				rmixlp_write_4(RMIXLP_SBC_PCITAG,
				    RMIXLP_SBC_PCIE_IO_BASEn(i), 0xfffff000);
				rmixlp_write_4(RMIXLP_SBC_PCITAG,
				    RMIXLP_SBC_PCIE_IO_LIMITn(i), 0x00000000);
				extent_free(rcp->rc_phys_ex,
				    rp->r_pbase >> 20, rp->r_size >> 20,
				    EX_NOWAIT);
				rp->r_pbase = (bus_addr_t) -1;
				rp->r_size = 0;
			}
		}
		// rmixlp_pcie_bar_alloc(sc, &rcp->rc_pci_io, 4, 1);
#endif
		rmixl_pci_bus_io_init(&rcp->rc_pci_iot, rcp);
	}

#if 0
	/*
	 * initialize the PCI chipset tag
	 */
	rmixlp_pcie_pc_init();
#endif

	/*
	 * Make sure the USB devices aren't still in reset.
	 */
	/*
	 * Disable byte swapping.
	 */
#ifdef BYTESWAP_USB
	pci_conf_write(sc->sc_pc, RMIXLP_EHCI0_PCITAG,
	    RMIXLP_USB_BYTE_SWAP_DIS, 1);
#endif
	sc->sc_usb_bswapped = (pci_conf_read(sc->sc_pc, RMIXLP_EHCI0_PCITAG,
	    RMIXLP_USB_BYTE_SWAP_DIS) != 0);
	rmixlp_pcie_usb_init_hook(sc->sc_pc, RMIXLP_EHCI0_PCITAG);
	rmixlp_pcie_usb_init_hook(sc->sc_pc, RMIXLP_OHCI0_PCITAG);
	rmixlp_pcie_usb_init_hook(sc->sc_pc, RMIXLP_OHCI1_PCITAG);
	rmixlp_pcie_usb_init_hook(sc->sc_pc, RMIXLP_EHCI1_PCITAG);
	rmixlp_pcie_usb_init_hook(sc->sc_pc, RMIXLP_OHCI2_PCITAG);
	rmixlp_pcie_usb_init_hook(sc->sc_pc, RMIXLP_OHCI3_PCITAG);

#if NPCI > 0
#ifdef PCI_NETBSD_CONFIGURE
	rmixlp_pcie_configure_bus(sc);
#endif

	/*
	 * attach the PCI bus
	 */
	memset(&pba, 0, sizeof(pba));
	pba.pba_memt = &rcp->rc_pci_memt;
	pba.pba_iot =  &rcp->rc_pci_iot;
	pba.pba_dmat = sc->sc_dmat32;
	pba.pba_dmat64 = sc->sc_dmat64;
	pba.pba_pc = sc->sc_pc;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_intrswiz = 0;
	pba.pba_intrtag = 0;
	pba.pba_flags = 0
	    // | PCI_FLAGS_IO_ENABLED
	    | PCI_FLAGS_MEM_ENABLED
	    | PCI_FLAGS_MRL_OKAY
	    | PCI_FLAGS_MRM_OKAY
	    | PCI_FLAGS_MWI_OKAY;

	(void) config_found_ia(self, "pcibus", &pba, pcibusprint);
#endif
}

#ifdef PCI_NETBSD_CONFIGURE
void
rmixlp_pcie_bar_alloc(struct rmixlp_pcie_softc *sc, struct rmixl_region *rp,
	u_long size_mb, u_long align_mb)
{
	struct rmixl_config * const rcp = &rmixl_configuration;
	u_long region_start;
	int error;

	error = extent_alloc(rcp->rc_phys_ex, size_mb, align_mb,
	    0UL, EX_NOWAIT, &region_start);
	if (error != 0)
		panic("%s: extent_alloc(%p, %#lx, %#lx, %#lx, %#x, %p): %d",
		    __func__, rcp->rc_phys_ex, size_mb, align_mb,
		    0UL, EX_NOWAIT, &region_start, error);

	const uint64_t pbase = (uint64_t)region_start << 20;
	const uint64_t limit = pbase + ((uint64_t)(size_mb - 1) << 20);

	aprint_debug_dev(sc->sc_dev,
	    "%s: pbase=%#"PRIx64" limit=%#"PRIx64" size=%luMB\n",
	    __func__, pbase, limit, size_mb);

	rp->r_pbase = pbase;
	rp->r_size = (uint64_t)size_mb << 20;
}
#endif /* PCI_NETBSD_CONFIGURE */

/*
 * rmixlp_pcie_lnkcfg_get - lookup the lnkcfg for this XLP
 *	uses IO_AD[24:23] for PCI Lane Control and
 *	IO_AD[22:19] for EP(0)/RC(1) determination
 */
static void
rmixlp_pcie_lnkcfg_get(struct rmixlp_pcie_softc *sc)
{
	const uint32_t por_cfg = rmixlp_read_4(RMIXLP_SM_PCITAG,
	    RMIXLP_SM_POWER_ON_RESET_CFG);
	const u_int plc = __SHIFTOUT(por_cfg, RMIXLP_SM_POWER_ON_RESET_CFG_PLC);
	const u_int pm = __SHIFTOUT(por_cfg, RMIXLP_SM_POWER_ON_RESET_CFG_PM);
	for (const rmixlp_pcie_lnkcfg_t *lnk = rmixlp_pcie_lnkcfgs;; lnk++) {
		if ((lnk->lnk_variant == rmixl_xlp_variant
		    || lnk->lnk_variant == RMIXLP_ANY)
		    && (plc & lnk->lnk_plc_mask) == lnk->lnk_plc_match) {
			sc->sc_lnkcfg = *lnk;
			sc->sc_lnkmode = pm & (__BIT(lnk->lnk_ports) - 1);
			return;
		}
	}
	panic("%s: missing lnkcfg for XLP variant %d",
	    __func__, rmixl_xlp_variant);
}

#if 0
/*
 * rmixlp_pcie_intcfg - init PCIe Link interrupt enables
 */
static void
rmixlp_pcie_intcfg(struct rmixlp_pcie_softc *sc)
{
	int link;
	size_t size;
	rmixlp_pcie_evcnt_t *ev;

	DPRINTF(("%s: disable all link interrupts\n", __func__));
	for (link=0; link < sc->sc_pcie_lnktab.ncfgs; link++) {
		RMIXL_IOREG_WRITE(RMIXL_IO_DEV_PCIE_LE + int_enb_offset[link].r0,
			RMIXL_PCIE_LINK_STATUS0_ERRORS); 
		RMIXL_IOREG_WRITE(RMIXL_IO_DEV_PCIE_LE + int_enb_offset[link].r1,
			RMIXL_PCIE_LINK_STATUS1_ERRORS); 
		RMIXL_IOREG_WRITE(RMIXL_IO_DEV_PCIE_LE + msi_enb_offset[link], 0); 
		sc->sc_link_intr[link] = NULL;

		/*
		 * allocate per-cpu, per-pin interrupt event counters
		 */
		size = ncpu * PCI_INTERRUPT_PIN_MAX * sizeof(rmixlp_pcie_evcnt_t);
		ev = malloc(size, M_DEVBUF, M_NOWAIT);
		if (ev == NULL)
			panic("%s: cannot malloc evcnts\n", __func__);
		sc->sc_evcnts[link] = ev;
		for (int pin=PCI_INTERRUPT_PIN_A; pin <= PCI_INTERRUPT_PIN_MAX; pin++) {
			for (int cpu=0; cpu < ncpu; cpu++) {
				ev = RMIXL_PCIE_EVCNT(sc, link, pin - 1, cpu);
				snprintf(ev->name, sizeof(ev->name),
					"cpu%d, link %d, pin %d", cpu, link, pin);
				evcnt_attach_dynamic(&ev->evcnt, EVCNT_TYPE_INTR,
					NULL, "rmixlp_pcie", ev->name);
			}
		}
	}
}
#endif

void
rmixlp_pcie_pc_init(void)
{
	struct rmixlp_pcie_softc * const sc = &rmixlp_pcie_softc;
	pci_chipset_tag_t pc = &rmixl_configuration.rc_pci_chipset;

	pc->pc_conf_v = (void *)sc;
	pc->pc_attach_hook = rmixlp_pcie_attach_hook;
	pc->pc_bus_maxdevs = rmixlp_pcie_bus_maxdevs;
	pc->pc_make_tag = rmixlp_pcie_make_tag;
	pc->pc_decompose_tag = rmixlp_pcie_decompose_tag;
	pc->pc_conf_read = rmixlp_pcie_conf_read;
	pc->pc_conf_write = rmixlp_pcie_conf_write;
#ifdef __PCI_DEV_FUNCORDER
	pc->pc_dev_funcorder = rmixlp_pcie_dev_funcorder;
#endif
#ifdef __HAVE_PCI_CONF_HOOK
	pc->pc_conf_hook = rmixlp_pcie_conf_hook;
#endif

	pc->pc_intr_v = (void *)sc;
	pc->pc_intr_map = rmixlp_pcie_intr_map;
	pc->pc_intr_string = rmixlp_pcie_intr_string;
	pc->pc_intr_evcnt = rmixlp_pcie_intr_evcnt;
	pc->pc_intr_establish = rmixlp_pcie_intr_establish;
	pc->pc_intr_disestablish = rmixlp_pcie_intr_disestablish;
	pc->pc_conf_interrupt = rmixlp_pcie_conf_interrupt;
}

#if defined(PCI_NETBSD_CONFIGURE)
static void
rmixlp_pcie_link_bar_update(struct rmixlp_pcie_softc *sc, pcitag_t tag)
{
	pcireg_t idreg = rmixlp_pcie_conf_read(sc, tag, PCI_ID_REG);
	pcireg_t classreg = rmixlp_pcie_conf_read(sc, tag, PCI_CLASS_REG);
	const size_t port = _RMIXL_PCITAG_FUNC(tag);
	if (idreg != PCI_ID_CODE(PCI_VENDOR_NETLOGIC, PCI_PRODUCT_NETLOGIC_XLP_PCIROOT))
		return;
	/*
	 * If the port is in EP mode, it's no longer a bridge.
	 */
	if (PCI_SUBCLASS(classreg) != PCI_SUBCLASS_BRIDGE_PCI)
		return;
	if ((sc->sc_lnkmode & __BIT(port)) == 0)
		return;

	pcireg_t statio = rmixlp_pcie_conf_read(sc, tag, PCI_BRIDGE_STATIO_REG);
	pcireg_t iohigh = rmixlp_pcie_conf_read(sc, tag, PCI_BRIDGE_IOHIGH_REG);

#define	PCI_BRIDGE_GET(r,f,v)	\
	(((v) >> PCI_BRIDGE_##r##_##f##_SHIFT) & PCI_BRIDGE_##r##_##f##_MASK)

	uint32_t iobase = (PCI_BRIDGE_GET(STATIO, IOBASE, statio) << 12)
	    | (PCI_BRIDGE_GET(IOHIGH, BASE, iohigh) << 16);
	uint32_t iolimit = ((PCI_BRIDGE_GET(STATIO, IOLIMIT, statio) << 12)
	    | ((PCI_BRIDGE_GET(IOHIGH, LIMIT, iohigh) + 1) << 16)) - 1;

	aprint_verbose_dev(sc->sc_dev,
	    "%s[%zu] base=%#x limit=%#x (%s)\n",
	    "pci_io", port, iobase, iolimit,
	    (iobase <= iolimit ? "enabled" : "disabled"));

	if (iobase <= iolimit) {
		KASSERT((iobase & 0xfffff) == 0);
		rmixlp_write_4(RMIXLP_SBC_PCITAG,
		    RMIXLP_SBC_PCIE_IO_LIMITn(port), iolimit >> 20);
		rmixlp_write_4(RMIXLP_SBC_PCITAG,
		    RMIXLP_SBC_PCIE_IO_BASEn(port), iobase >> 20);
	}

	pcireg_t mem = rmixlp_pcie_conf_read(sc, tag, PCI_BRIDGE_MEMORY_REG);

	uint32_t membase = PCI_BRIDGE_GET(MEMORY, BASE, mem) << 20;
	uint32_t memlimit = ((PCI_BRIDGE_GET(MEMORY, LIMIT, mem) + 1) << 20) - 1;

	aprint_verbose_dev(sc->sc_dev, "%s[%zu] base=%#x limit=%#x (%s)\n",
	    "pci_mem", port, membase, memlimit,
	    (membase <= memlimit ? "enabled" : "disabled"));

	if (membase <= memlimit && 0) {
		rmixlp_write_4(RMIXLP_SBC_PCITAG,
		    RMIXLP_SBC_PCIE_MEM_LIMITn(port), memlimit >> 20);
		rmixlp_write_4(RMIXLP_SBC_PCITAG,
		    RMIXLP_SBC_PCIE_MEM_BASEn(port), membase >> 20);
	}
}

static void
rmixlp_pcie_print_bus0_bar0(struct rmixlp_pcie_softc *sc, pcitag_t tag)
{
	const size_t bar = PCI_BAR0;
	pcireg_t ml = rmixlp_pcie_conf_read(sc, tag, PCI_BAR0);

	switch (PCI_MAPREG_TYPE(ml)|PCI_MAPREG_MEM_TYPE(ml)) {
	case PCI_MAPREG_TYPE_MEM|PCI_MAPREG_MEM_TYPE_32BIT: {
		bus_addr_t addr = PCI_MAPREG_MEM_ADDR(ml);
		if (addr) {
			aprint_normal_dev(sc->sc_dev,
			    "tag %#lx bar[0]: mem32=%#"PRIxBUSADDR"\n",
			    tag, addr);
		}
		break;
	}
	case PCI_MAPREG_TYPE_MEM|PCI_MAPREG_MEM_TYPE_64BIT: {
		pcireg_t mu = rmixlp_pcie_conf_read(sc, tag, bar + 4);
		bus_addr_t addr =
		    PCI_MAPREG_MEM64_ADDR(ml|((uint64_t)mu << 32));
		if (addr) {
			aprint_normal_dev(sc->sc_dev,
			    "tag %#lx bar[0]: mem64=%#"PRIxBUSADDR"\n",
			    tag, addr);
		}
		break;
	}
	default:
		return;
	}
}

static bus_addr_t
rmixlp_pcie_set_bus0_bar0(struct rmixlp_pcie_softc *sc, pcitag_t tag,
	bus_addr_t pbase)
{
	const size_t bar = PCI_BAR0;
	pcireg_t ml = rmixlp_pcie_conf_read(sc, tag, PCI_BAR0);
	bus_size_t bar_size = 0;

	switch (PCI_MAPREG_TYPE(ml)|PCI_MAPREG_MEM_TYPE(ml)) {
	case PCI_MAPREG_TYPE_MEM|PCI_MAPREG_MEM_TYPE_32BIT: {
		rmixlp_pcie_conf_write(sc, tag, bar, 0xffffffff);
		pcireg_t maskl = rmixlp_pcie_conf_read(sc, tag, PCI_BAR0);
		bar_size = PCI_MAPREG_MEM_SIZE(maskl);
		if (bar_size == 0)
			return pbase;
		pbase = (pbase + bar_size - 1) & -bar_size;
		KASSERT((uint32_t)pbase == pbase);
		rmixlp_pcie_conf_write(sc, tag, bar, pbase);
		aprint_normal_dev(sc->sc_dev,
		    "tag %#lx bar[0]: mem32=%#"PRIxBUSADDR
		    " size=%#"PRIxBUSSIZE"\n",
		    tag, pbase, bar_size);
		pbase += bar_size;
		break;
	}
	case PCI_MAPREG_TYPE_MEM|PCI_MAPREG_MEM_TYPE_64BIT: {
		// pcireg_t mu = rmixlp_pcie_conf_read(sc, tag, bar + 4);
		rmixlp_pcie_conf_write(sc, tag, bar, 0xffffffff);
		rmixlp_pcie_conf_write(sc, tag, bar + 4, 0xffffffff);
		pcireg_t maskl = rmixlp_pcie_conf_read(sc, tag, bar);
		pcireg_t masku = rmixlp_pcie_conf_read(sc, tag, bar + 4);
		bar_size = PCI_MAPREG_MEM64_SIZE(maskl|((uint64_t)masku << 32));
		if (bar_size == 0)
			return pbase;
		pbase = (pbase + bar_size - 1) & -bar_size;
		rmixlp_pcie_conf_write(sc, tag, bar,
		    (pbase >> 0) & 0xffffffff);
		rmixlp_pcie_conf_write(sc, tag, bar + 4,
		    (pbase >> 32) & 0xffffffff);
		aprint_normal_dev(sc->sc_dev,
		    "tag %#lx bar[0]: mem64=%#"PRIxBUSADDR
		    " size=%#"PRIxBUSSIZE"\n",
		    tag, pbase, bar_size);
		pbase += bar_size;

		break;
	}
	default:
		return pbase;
	}
	pcireg_t cmdsts = rmixlp_pcie_conf_read(sc, tag,
	    PCI_COMMAND_STATUS_REG);
	cmdsts |= PCI_COMMAND_MEM_ENABLE;
	rmixlp_pcie_conf_write(sc, tag, PCI_COMMAND_STATUS_REG, cmdsts);

	return pbase;
}

void
rmixlp_pcie_configure_bus(struct rmixlp_pcie_softc *sc)
{
	/*
	 * Configure the PCI bus.
	 */
	struct rmixl_config *rcp = &rmixl_configuration;

	aprint_debug_dev(sc->sc_dev, "configuring PCI bus\n");

	struct extent *ioext = NULL;
	struct extent *memext = NULL;
	struct extent *pmemext = NULL;

#if 0
	ioext  = extent_create("pciio",
	    rcp->rc_pci_io.r_pbase,
	    rcp->rc_pci_io.r_pbase + rcp->rc_pci_io.r_size - 1,
	    M_DEVBUF, NULL, 0, EX_NOWAIT);
#endif

	memext = extent_create("pcimem",
	    rcp->rc_pci_mem.r_pbase + 0x10000,
	    rcp->rc_pci_mem.r_pbase + rcp->rc_pci_mem.r_size - 1 - 0x10000,
	    M_DEVBUF, NULL, 0, EX_NOWAIT);

#if NPCI > 0
	// { extern int pci_conf_debug; pci_conf_debug = 1; }
	pci_configure_bus(sc->sc_pc, ioext, memext, pmemext, 0,
	    mips_cache_info.mci_dcache_align);
#endif

	if (0) {
		bus_addr_t mem = rcp->rc_pci_mem.r_pbase;
		mem = rmixlp_pcie_set_bus0_bar0(sc, RMIXLP_EHCI0_PCITAG, mem);
		mem = rmixlp_pcie_set_bus0_bar0(sc, RMIXLP_OHCI0_PCITAG, mem);
		mem = rmixlp_pcie_set_bus0_bar0(sc, RMIXLP_OHCI1_PCITAG, mem);
		mem = rmixlp_pcie_set_bus0_bar0(sc, RMIXLP_EHCI1_PCITAG, mem);
		mem = rmixlp_pcie_set_bus0_bar0(sc, RMIXLP_OHCI2_PCITAG, mem);
		mem = rmixlp_pcie_set_bus0_bar0(sc, RMIXLP_OHCI3_PCITAG, mem);
		mem = rmixlp_pcie_set_bus0_bar0(sc, RMIXLP_NAE_PCITAG, mem);
		mem = rmixlp_pcie_set_bus0_bar0(sc, RMIXLP_POE_PCITAG, mem);
		mem = rmixlp_pcie_set_bus0_bar0(sc, RMIXLP_AHCI_PCITAG, mem);
		mem = rmixlp_pcie_set_bus0_bar0(sc, RMIXLP_FMN_PCITAG, mem);
		mem = rmixlp_pcie_set_bus0_bar0(sc, RMIXLP_SRIO_PCITAG, mem);
	} else {
		rmixlp_pcie_print_bus0_bar0(sc, RMIXLP_EHCI0_PCITAG);
		rmixlp_pcie_print_bus0_bar0(sc, RMIXLP_OHCI0_PCITAG);
		rmixlp_pcie_print_bus0_bar0(sc, RMIXLP_OHCI1_PCITAG);
		rmixlp_pcie_print_bus0_bar0(sc, RMIXLP_EHCI1_PCITAG);
		rmixlp_pcie_print_bus0_bar0(sc, RMIXLP_OHCI2_PCITAG);
		rmixlp_pcie_print_bus0_bar0(sc, RMIXLP_OHCI3_PCITAG);
		rmixlp_pcie_print_bus0_bar0(sc, RMIXLP_NAE_PCITAG);
		rmixlp_pcie_print_bus0_bar0(sc, RMIXLP_POE_PCITAG);
		rmixlp_pcie_print_bus0_bar0(sc, RMIXLP_AHCI_PCITAG);
		rmixlp_pcie_print_bus0_bar0(sc, RMIXLP_FMN_PCITAG);
		rmixlp_pcie_print_bus0_bar0(sc, RMIXLP_SRIO_PCITAG);
	}

	for (size_t i = 0; i < 4; i++) {
		rmixlp_pcie_link_bar_update(sc, _RMIXL_PCITAG(0, 1, i));
	}

	if (ioext)
		extent_destroy(ioext);
	if (memext)
		extent_destroy(memext);
	if (pmemext)
		extent_destroy(memext);
}
#endif

#if 0
static void
rmixlp_pcie_init_ecfg(struct rmixlp_pcie_softc *sc)
{
	void *v;
	pcitag_t tag;
	pcireg_t r;

	v = sc;
	tag = rmixlp_pcie_make_tag(v, 0, 0, 0);

#ifdef PCI_DEBUG
	int i, offset;
	static const int offtab[] =
		{ 0, 4, 8, 0xc, 0x10, 0x14, 0x18, 0x1c,
		  0x2c, 0x30, 0x34 };
	for (i=0; i < sizeof(offtab)/sizeof(offtab[0]); i++) {
		offset = 0x100 + offtab[i];
		r = rmixlp_pcie_conf_read(v, tag, offset);
		printf("%s: %#x: %#x\n", __func__, offset, r);
	}
#endif
	r = rmixlp_pcie_conf_read(v, tag, 0x100);
	if (r == -1)
		return;	/* cannot access */

	/* check pre-existing uncorrectable errs */
	r = rmixlp_pcie_conf_read(v, tag, RMIXL_PCIE_ECFG_UESR);
	r &= ~PCIE_ECFG_UExR_RESV;
	if (r != 0)
		panic("%s: Uncorrectable Error Status: %#x\n",
			__func__, r);

	/* unmask all uncorrectable errs */
	r = rmixlp_pcie_conf_read(v, tag, RMIXL_PCIE_ECFG_UEMR);
	r &= ~PCIE_ECFG_UExR_RESV;
	rmixlp_pcie_conf_write(v, tag, RMIXL_PCIE_ECFG_UEMR, r);

	/* ensure default uncorrectable err severity confniguration */
	r = rmixlp_pcie_conf_read(v, tag, RMIXL_PCIE_ECFG_UEVR);
	r &= ~PCIE_ECFG_UExR_RESV;
	r |= PCIE_ECFG_UEVR_DFLT;
	rmixlp_pcie_conf_write(v, tag, RMIXL_PCIE_ECFG_UEVR, r);

	/* check pre-existing correctable errs */
	r = rmixlp_pcie_conf_read(v, tag, RMIXL_PCIE_ECFG_CESR);
	r &= ~PCIE_ECFG_CExR_RESV;
#ifdef DIAGNOSTIC
	if (r != 0)
		aprint_normal("%s: Correctable Error Status: %#x\n",
			device_xname(sc->sc_dev), r);
#endif

	/* unmask all correctable errs */
	r = rmixlp_pcie_conf_read(v, tag, RMIXL_PCIE_ECFG_CEMR);
	r &= ~PCIE_ECFG_CExR_RESV;
	rmixlp_pcie_conf_write(v, tag, RMIXL_PCIE_ECFG_UEMR, r);

	/* check pre-existing Root Error Status */
	r = rmixlp_pcie_conf_read(v, tag, RMIXL_PCIE_ECFG_RESR);
	r &= ~PCIE_ECFG_RESR_RESV;
	if (r != 0)
		panic("%s: Root Error Status: %#x\n", __func__, r);
			/* XXX TMP FIXME */

	/* enable all Root errs */
	r = (pcireg_t)(~PCIE_ECFG_RECR_RESV);
	rmixlp_pcie_conf_write(v, tag, RMIXL_PCIE_ECFG_RECR, r);

	/*
	 * establish ISR for PCIE Fatal Error interrupt
	 * - for XLS4xxLite, XLS2xx, XLS1xx only
	 */
	switch (MIPS_PRID_IMPL(mips_options.mips_cpu_id)) {
	case MIPS_XLS104:
	case MIPS_XLS108:
	case MIPS_XLS204:
	case MIPS_XLS208:
	case MIPS_XLS404LITE:
	case MIPS_XLS408LITE:
		sc->sc_fatal_ih = rmixl_intr_establish(29, IPL_HIGH,
		    IST_LEVEL_HIGH, rmixlp_pcie_error_intr, v, false);
		break;
	default:
		break;
	}

#if defined(DEBUG) || defined(DDB)
	rmixlp_pcie_v = v;
#endif
}
#endif

void
rmixlp_pcie_conf_interrupt(void *v, int bus, int dev, int ipin, int swiz,
	int *iline)
{
	DPRINTF(("%s: %p, %d, %d, %d, %d, %p\n",
		__func__, v, bus, dev, ipin, swiz, iline));
}

static void
rmixlp_pcie_usb_init_hook(pci_chipset_tag_t pc, pcitag_t tag)
{
	const size_t func = _RMIXL_PCITAG_FUNC(tag);
	const pcireg_t id = pci_conf_read(pc, tag, PCI_ID_REG);
	if (id != PCI_ID_CODE(PCI_VENDOR_NETLOGIC, PCI_PRODUCT_NETLOGIC_XLP_EHCIUSB)
	    && id != PCI_ID_CODE(PCI_VENDOR_NETLOGIC, PCI_PRODUCT_NETLOGIC_XLP_OHCIUSB))
		return;

	if (func == 0 || func == 3) {
		/*
		 * Bring the PHY out of reset.
		 */
		pcireg_t phy0 = pci_conf_read(pc, tag, RMIXLP_USB_PHY0);
		phy0 &= ~(RMIXLP_USB_PHY0_PHYPORTRST1
		    |RMIXLP_USB_PHY0_PHYPORTRST0
		    |RMIXLP_USB_PHY0_USBPHYRESET);
		pci_conf_write(pc, tag, RMIXLP_USB_PHY0, phy0);
	}

	/*
	 * Bring the USB controller out of reset.
	 */
	pcireg_t ctl0 = pci_conf_read(pc, tag, RMIXLP_USB_CTL0);
	if (ctl0 & RMIXLP_USB_CTL0_USBCTLRRST) {
		ctl0 &= ~RMIXLP_USB_CTL0_USBCTLRRST;
		pci_conf_write(pc, tag, RMIXLP_USB_CTL0, ctl0);
	}
}

void
rmixlp_pcie_attach_hook(device_t parent, device_t self,
	struct pcibus_attach_args *pba)
{
	DPRINTF(("%s: pba_bus %d pba_bridgetag %p intrtag %#lx pc_conf_v %p\n",
		__func__, pba->pba_bus, pba->pba_bridgetag,
		pba->pba_intrtag, 
		pba->pba_pc->pc_conf_v));
}

int
rmixlp_pcie_bus_maxdevs(void *v, int busno)
{
	return 32;	/* depends on the number of XLP nodes */
}

/*
 * XLS pci tag is a 40 bit address composed thusly:
 *	39:29   (reserved)
 *	28      Swap (0=little, 1=big endian)
 *	27:20   Bus number
 *	19:15   Device number
 *	14:12   Function number
 *	11:8    Extended Register number
 *	7:0     Register number
 *
 * Note: this is the "native" composition for addressing CFG space, but not for ECFG space.
 */
pcitag_t
rmixlp_pcie_make_tag(void *v, int bus, int dev, int func)
{
	return _RMIXL_PCITAG(bus, dev, func);
}

void
rmixlp_pcie_decompose_tag(void *v, pcitag_t tag, int *bp, int *dp, int *fp)
{
	if (bp != NULL)
		*bp = _RMIXL_PCITAG_BUS(tag);
	if (dp != NULL)
		*dp = _RMIXL_PCITAG_DEV(tag);
	if (fp != NULL)
		*fp = _RMIXL_PCITAG_FUNC(tag);
}

void
rmixlp_pcie_print_tag(const char * restrict s, void *v, pcitag_t tag,
	int offset, vaddr_t va, u_long r)
{
	int bus, dev, fun;

	rmixlp_pcie_decompose_tag(v, tag, &bus, &dev, &fun);
	printf("%s: %#lx: %d/%d/%d/%d - %#" PRIxVADDR ":%#lx\n",
		s, tag, bus, dev, fun, offset, va, r);
}

pcireg_t
rmixlp_pcie_conf_read(void *v, pcitag_t tag, int offset)
{
	struct rmixl_config * const rcp = &rmixl_configuration;
	struct rmixlp_pcie_softc * const sc = v;
	bus_space_tag_t bst = &rcp->rc_pci_ecfg_eb_memt;
	bus_space_handle_t bsh = rcp->rc_pci_ecfg_eb_memh;
	pcireg_t rv;

	KASSERT(offset < 0x1000);
	KASSERT((offset & 3) == 0);

	/*
	 * If this register isn't addressable via PCI ECFG space, it might
	 * be accesible via normal PCI CFG space.
	 */
	if (tag >= rcp->rc_pci_ecfg.r_size) {
#if 0
		if (offset >= 0x100)
			return 0xffffffff;
		tag >>= 4;	/* Make it compatible with CFG space */
		if (tag >= rcp->rc_pci_cfg.r_size)
			return 0xffffffff;
		bst = &rcp->rc_pci_cfg_memt;
		bsh = rcp->rc_pci_cfg_memh;
#else
		return 0xffffffff;
#endif
	}

	if (__predict_true(!cold))
		mutex_enter(&sc->sc_mutex);

	//uint64_t cfg0 = rmixlp_cache_err_dis();
	rv = bus_space_read_4(bst, bsh, (bus_size_t)tag | offset);
#if 0
	if (rmixlp_cache_err_check() != 0) {
#ifdef DIAGNOSTIC
		int bus, dev, fun;

		rmixlp_pcie_decompose_tag(v, tag, &bus, &dev, &fun);
		printf("%s: %d/%d/%d, offset %#x: bad address\n",
			__func__, bus, dev, fun, offset);
#endif
		rv = (pcireg_t) -1;
	}
#endif
	//rmixlp_cache_err_restore(cfg0);

	if (__predict_true(!cold))
		mutex_exit(&sc->sc_mutex);

	if (_RMIXL_PCITAG_BUS(tag) == _RMIXL_PCITAG_BUS(RMIXLP_EHCI0_PCITAG)
	    && _RMIXL_PCITAG_DEV(tag) == _RMIXL_PCITAG_DEV(RMIXLP_EHCI0_PCITAG)
	    && sc->sc_usb_bswapped) {
		rv = bswap32(rv);
	}

	return rv;
}

void
rmixlp_pcie_conf_write(void *v, pcitag_t tag, int offset, pcireg_t val)
{
	struct rmixl_config * const rcp = &rmixl_configuration;
	struct rmixlp_pcie_softc * const sc = v;
	bus_space_tag_t bst = &rcp->rc_pci_ecfg_eb_memt;
	bus_space_handle_t bsh = rcp->rc_pci_ecfg_eb_memh;

	KASSERT(offset < 0x1000);
	KASSERT((offset & 3) == 0);

	if (_RMIXL_PCITAG_BUS(tag) == _RMIXL_PCITAG_BUS(RMIXLP_EHCI0_PCITAG)
	    && _RMIXL_PCITAG_DEV(tag) == _RMIXL_PCITAG_DEV(RMIXLP_EHCI0_PCITAG)
	    && sc->sc_usb_bswapped) {
		val = bswap32(val);
	}

	/*
	 * If this register isn't addressable via PCI ECFG space, it might
	 * be accesible via normal PCI CFG space.
	 */
	if (__predict_false(tag >= rcp->rc_pci_ecfg.r_size)) {
#if 0
		if (offset >= 0x100)
			return;
		tag >>= 4;	/* Make it compatible with CFG space */
		if (tag >= rcp->rc_pci_cfg.r_size)
			return;
		bst = &rcp->rc_pci_cfg_memt;
		bsh = rcp->rc_pci_cfg_memh;
#else
		return;
#endif
	}

	if (__predict_true(!cold))
		mutex_enter(&sc->sc_mutex);

	//uint64_t cfg0 = rmixl_cache_err_dis();
	bus_space_write_4(bst, bsh, (bus_size_t)tag | offset, val);
#if 0
	if (rmixlp_cache_err_check() != 0) {
#ifdef DIAGNOSTIC
		int bus, dev, fun;

		rmixlp_pcie_decompose_tag(v, tag, &bus, &dev, &fun);
		printf("%s: %d/%d/%d, offset %#x: bad address\n",
			__func__, bus, dev, fun, offset);
#endif
	}
#endif
	//rmixlp_cache_err_restore(cfg0);

	/*
	 * When updating the PPB header for the PCIE bridge functions,
	 * we need to copy the bridge bus info to the SBC BUSNUM BAR for 
	 * the that function.
	 */
	if (offset == PCI_BRIDGE_BUS_REG
	    && (tag & _RMIXL_PCITAG(255,31,0)) == _RMIXL_PCITAG(0,1,0)) {
		const u_int func = _RMIXL_PCITAG_FUNC(tag);
		if (sc->sc_lnkmode & __BIT(func)) {
			uint32_t nval = (val & RMIXLP_SBC_BUSNUM_BAR_MASK)
			    | RMIXLP_SBC_BUSNUM_BAR_ENABLE;
			rmixlp_write_4(RMIXLP_SBC_PCITAG,
			    RMIXLP_SBC_BUSNUM_BARn(func),
			    nval);
		}
	}

	if (__predict_true(!cold))
		mutex_exit(&sc->sc_mutex);
}

#ifdef __PCI_DEV_FUNCORDER
bool
rmixlp_pcie_dev_funcorder(void *v, int bus, int device, int nfunctions,
	char *funcs)
{
	if (bus != 0 || device != 2)
		return false;

	funcs[0] = 1; /* OHCI */
	funcs[1] = 2; /* OHCI */
	funcs[2] = 0; /* EHCI */
	funcs[3] = 4; /* OHCI */
	funcs[4] = 5; /* OHCI */
	funcs[5] = 3; /* EHCI */
	funcs[6] = -1;
	return true;
}
#endif

#ifdef __HAVE_PCI_CONF_HOOK
int
rmixlp_pcie_conf_hook(void *v, int bus, int device, int function, pcireg_t id)
{
	if (bus == 0 && device != 1)
		return 0;

	return PCI_CONF_MAP_MEM | PCI_CONF_ENABLE_MEM | PCI_CONF_ENABLE_BM;
}
#endif

int
rmixlp_pcie_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *pih)
{
	int device;
	int bus;

	DPRINTF(("%s: pa_bus %d, pa_intrswiz %#x, pa_intrtag %#lx,"
		" pa_intrpin %d,  pa_intrline %d, pa_rawintrpin %d\n",
		__func__, pa->pa_bus, pa->pa_intrswiz, pa->pa_intrtag,
		pa->pa_intrpin, pa->pa_intrline, pa->pa_rawintrpin));

	/*
	 * If we aren't on device 1, then this device is a XLP device and its
	 * interrupt routing information in PCI config space.
	 */
	pci_decompose_tag(pa->pa_pc, pa->pa_intrtag, &bus, &device, NULL);
	const u_int node = device / 8;
	KASSERT(bus == 0);
	uint32_t r = pci_conf_read(pa->pa_pc, pa->pa_intrtag,
	    PCI_RMIXLP_IRTINFO);
	r |= node << 24; /* owning node */
	if (_RMIXL_PCITAG_BUS(pa->pa_tag) != 0) {
		/*
		 * Encode PIN info (1..4) into bits [10:8]
		 */
		r |= (((device + pa->pa_intrswiz + pa->pa_intrpin - PCI_INTERRUPT_PIN_A) % 4) + PCI_INTERRUPT_PIN_A) << 8;
	}

	if ((r & 0x00ff0000) != 0)
		*pih = r;
	else
		*pih = 0;

	return 0;
}

const char *
rmixlp_pcie_intr_string(void *v, pci_intr_handle_t pih)
{
	if ((pih & 0x00ff0000) == 0)
		return NULL;

	return rmixl_irt_string(pih & 0xff);
}

const struct evcnt *
rmixlp_pcie_intr_evcnt(void *v, pci_intr_handle_t pih)
{
	return NULL;
}

static void
rmixlp_pcie_intr_disestablish(void *v, void *ih)
{
#if 0
	rmixlp_pcie_softc_t * const sc = v;
	rmixlp_pcie_link_dispatch_t * const dip = ih;
	rmixlp_pcie_link_intr_t *lip = sc->sc_link_intr[dip->link];
	uint32_t r;
	uint32_t bit;
	u_int offset;
	u_int other;
	bool busy;

	DPRINTF(("%s: link=%d pin=%d irq=%d\n",
		__func__, dip->link, dip->bitno + 1, dip->irq));

	mutex_enter(&sc->sc_mutex);

	dip->func = NULL;	/* mark unused, prevent further dispatch */

	/*
	 * if no other dispatch handle is using this interrupt,
	 * we can disable it
	 */
	busy = false;
	for (int i=0; i < lip->dispatch_count; i++) {
		rmixlp_pcie_link_dispatch_t *d = &lip->dispatch_data[i];
		if (d == dip)
			continue;
		if (d->bitno == dip->bitno) {
			busy = true;
			break;
		}
	}
	if (! busy) {
		if (dip->bitno < 32) {
			bit = 1 << dip->bitno;
			offset = int_enb_offset[dip->link].r0;
			other  = int_enb_offset[dip->link].r1;
		} else {
			bit = 1 << (dip->bitno - 32);
			offset = int_enb_offset[dip->link].r1;
			other  = int_enb_offset[dip->link].r0;
		}

		/* disable this interrupt in the PCIe bridge */
		r = RMIXL_IOREG_READ(RMIXL_IO_DEV_PCIE_LE + offset);
		r &= ~bit;
		RMIXL_IOREG_WRITE(RMIXL_IO_DEV_PCIE_LE + offset, r);

		/*
		 * if both ENABLE0 and ENABLE1 are 0
		 * disable the link interrupt
		 */
		if (r == 0) {
			/* check the other reg */
			if (RMIXL_IOREG_READ(RMIXL_IO_DEV_PCIE_LE + other) == 0) {
				DPRINTF(("%s: disable link %d\n", __func__, lip->link));

				/* tear down interrupt on this link */
				rmixl_intr_disestablish(lip->ih);

				/* commit NULL interrupt set */
				sc->sc_link_intr[dip->link] = NULL;

				/* schedule delayed free of the old link interrupt set */
				rmixlp_pcie_lip_free_callout(lip);
			}
		}
	}

	mutex_exit(&sc->sc_mutex);
#endif
}

static void *
rmixlp_pcie_intr_establish(void *v, pci_intr_handle_t pih, int ipl,
        int (*func)(void *), void *arg)
{
#if 0
	struct rmixlp_pcie_softc *sc = v;
	u_int link, bitno, irq;
	uint32_t r;
	rmixlp_pcie_link_intr_t *lip;
	rmixlp_pcie_link_dispatch_t *dip = NULL;
	uint32_t bit;
	u_int offset;

	if (pih == ~0) {
		DPRINTF(("%s: bad pih=%#lx, implies PCI_INTERRUPT_PIN_NONE\n",
			__func__, pih));
		return NULL;
	}

	rmixlp_pcie_decompose_pih(pih, &link, &bitno, &irq);
	DPRINTF(("%s: link=%d pin=%d irq=%d\n",
		__func__, link, bitno + 1, irq));

	mutex_enter(&sc->sc_mutex);

	lip = rmixlp_pcie_lip_add_1(sc, link, irq, ipl);
	if (lip == NULL)
		return NULL;

	/*
	 * initializae our new interrupt, the last element in dispatch_data[]
	 */
	dip = &lip->dispatch_data[lip->dispatch_count - 1];
	dip->link = link;
	dip->bitno = bitno;
	dip->irq = irq;
	dip->func = func;
	dip->arg = arg;
	dip->counts = RMIXL_PCIE_EVCNT(sc, link, bitno, 0);

	if (bitno < 32) {
		offset = int_enb_offset[link].r0;
		bit = 1 << bitno;
	} else {
		offset = int_enb_offset[link].r1;
		bit = 1 << (bitno - 32);
	}

	/* commit the new link interrupt set */
	sc->sc_link_intr[link] = lip;

	/* enable this interrupt in the PCIe bridge */
	r = RMIXL_IOREG_READ(RMIXL_IO_DEV_PCIE_LE + offset); 
	r |= bit;
	RMIXL_IOREG_WRITE(RMIXL_IO_DEV_PCIE_LE + offset, r); 

	mutex_exit(&sc->sc_mutex);
	return dip;
#else
	DPRINTF(("%s: pih=%#lx ipl=%d func=%p arg=%p\n",
	    __func__, pih, ipl, func, arg));
	if (pih == 0)
		return NULL;
	size_t irt = (pih >> 0) & 0xff;
	pcireg_t pin = (pih >> 8) & 0x03;
	bool is_mpsafe_p = (pih >> 31) & 0x01;
	// size_t node = (pih >> 24) & 0x03;
	if (pin)
		return NULL;
	return rmixl_intr_establish(irt, ipl, IST_LEVEL_HIGH,
	    func, arg, is_mpsafe_p);
#endif
}

#if 0
rmixlp_pcie_link_intr_t *
rmixlp_pcie_lip_add_1(rmixlp_pcie_softc_t *sc, u_int link, int irq, int ipl)
{
	rmixlp_pcie_link_intr_t *lip_old = sc->sc_link_intr[link];
	rmixlp_pcie_link_intr_t *lip_new;
	u_int dispatch_count;
	size_t size;

	dispatch_count = 1;
	size = sizeof(rmixlp_pcie_link_intr_t);
	if (lip_old != NULL) {
		/*
		 * count only those dispatch elements still in use
		 * unused ones will be pruned during copy
		 * i.e. we are "lazy" there is no rmixlp_pcie_lip_sub_1
                 */     
		for (int i=0; i < lip_old->dispatch_count; i++) {
			if (lip_old->dispatch_data[i].func != NULL) {
				dispatch_count++;
				size += sizeof(rmixlp_pcie_link_intr_t);
			}
		}
	}

	/*
	 * allocate and initialize link intr struct
	 * with one or more dispatch handles
	 */
	lip_new = malloc(size, M_DEVBUF, M_NOWAIT);
	if (lip_new == NULL) {
#ifdef DIAGNOSTIC
		printf("%s: cannot malloc\n", __func__);
#endif
		return NULL;
	}

	if (lip_old == NULL) {
		/* initialize the link interrupt struct */
		lip_new->sc = sc;
		lip_new->link = link;
		lip_new->ipl = ipl;
		lip_new->ih = rmixl_intr_establish(irq, sc->sc_tmsk,
			ipl, RMIXL_TRIG_LEVEL, RMIXL_POLR_HIGH,
			rmixlp_pcie_intr, lip_new, false);
		if (lip_new->ih == NULL)
			panic("%s: cannot establish irq %d", __func__, irq);
	} else {
		/*
		 * all intrs on a link get same ipl and sc
		 * first intr established sets the standard
		 */
		KASSERT(sc == lip_old->sc);
		if (sc != lip_old->sc) {
			printf("%s: sc %p mismatch\n", __func__, sc); 
			free(lip_new, M_DEVBUF);
			return NULL;
		}
		KASSERT (ipl == lip_old->ipl);
		if (ipl != lip_old->ipl) {
			printf("%s: ipl %d mismatch\n", __func__, ipl); 
			free(lip_new, M_DEVBUF);
			return NULL;
		}
		/*
		 * copy lip_old to lip_new, skipping unused dispatch elemets
		 */
		memcpy(lip_new, lip_old, sizeof(rmixlp_pcie_link_intr_t));
		for (int j=0, i=0; i < lip_old->dispatch_count; i++) {
			if (lip_old->dispatch_data[i].func != NULL) {
				memcpy(&lip_new->dispatch_data[j],
					&lip_old->dispatch_data[i],
					sizeof(rmixlp_pcie_link_dispatch_t));
				j++;
			}
		}

		/*
		 * schedule delayed free of old link interrupt set
		 */
		rmixlp_pcie_lip_free_callout(lip_old);
	}
	lip_new->dispatch_count = dispatch_count;

	return lip_new;
}

/*
 * delay free of the old link interrupt set
 * to allow anyone still using it to do so safely
 * XXX 2 seconds should be plenty?
 */
static void
rmixlp_pcie_lip_free_callout(rmixlp_pcie_link_intr_t *lip)
{
	callout_init(&lip->callout, 0);
	callout_reset(&lip->callout, 2 * hz, rmixlp_pcie_lip_free, lip);
}

static void
rmixlp_pcie_lip_free(void *arg)
{
	rmixlp_pcie_link_intr_t *lip = arg;
	
	callout_destroy(&lip->callout);
	free(lip, M_DEVBUF);
}

static int
rmixlp_pcie_intr(void *arg)
{
	rmixlp_pcie_link_intr_t *lip = arg;
	u_int link = lip->link;
	int rv = 0;

	uint32_t status0 = RMIXL_IOREG_READ(RMIXL_IO_DEV_PCIE_LE + int_sts_offset[link].r0); 
	uint32_t status1 = RMIXL_IOREG_READ(RMIXL_IO_DEV_PCIE_LE + int_sts_offset[link].r1); 
	uint64_t status = ((uint64_t)status1 << 32) | status0;
	DPRINTF(("%s: %d:%#"PRIx64"\n", __func__, link, status));

	if (status != 0) {
		rmixlp_pcie_link_dispatch_t *dip;

		if (status & RMIXL_PCIE_LINK_STATUS_ERRORS)
			rmixlp_pcie_link_error_intr(link, status0, status1);

		for (u_int i=0; i < lip->dispatch_count; i++) {
			dip = &lip->dispatch_data[i];
			int (*func)(void *) = dip->func;
			if (func != NULL) {
				uint64_t bit = 1 << dip->bitno;
				if ((status & bit) != 0) {
					(void)(*func)(dip->arg);
					dip->counts[cpu_index(curcpu())].evcnt.ev_count++;
					rv = 1;
				}
			}
		}
	}

	return rv;
}

static void
rmixlp_pcie_link_error_intr(u_int link, uint32_t status0, uint32_t status1)
{
	printf("%s: mask %#"PRIx64"\n", 
		__func__, RMIXL_PCIE_LINK_STATUS_ERRORS);
	printf("%s: PCIe Link Error: link=%d status0=%#x status1=%#x\n",
		__func__, link, status0, status1);
#if defined(DDB) && defined(DEBUG)
	Debugger();
#endif
}
#endif

#if 0
static int
rmixlp_pcie_error_check(void *v)
{
	int err=0;
#if 0
	int i, offset;
	pcireg_t r;
	pcitag_t tag;
#ifdef DIAGNOSTIC
	pcireg_t regs[PCIE_ECFG_ERRS_OFFTAB_NENTRIES];
#endif

	tag = _RMIXL_PCITAG(0, ,0, 0);

	for (i=0; i < PCIE_ECFG_ERRS_OFFTAB_NENTRIES; i++) {
		offset = pcie_ecfg_errs_tab[i].offset;
		r = rmixlp_pcie_conf_read(v, tag, offset);
#ifdef DIAGNOSTIC
		regs[i] = r;
#endif
		if (r != 0) {
			pcireg_t rw1c = r & pcie_ecfg_errs_tab[i].rw1c;
			if (rw1c != 0) {
				/* attempt to clear the error */
				rmixlp_pcie_conf_write(v, tag, offset, rw1c);
			};
			if (offset == RMIXL_PCIE_ECFG_CESR)
				err |= 1;	/* correctable */
			else
				err |= 2;	/* uncorrectable */
		}
	}
#ifdef DIAGNOSTIC
	if (err != 0) {
		for (i=0; i < PCIE_ECFG_ERRS_OFFTAB_NENTRIES; i++) {
			offset = pcie_ecfg_errs_tab[i].offset;
			printf("%s: %#x: %#x\n", __func__, offset, regs[i]);
		}
	}
#endif
#endif
	return err;
}

static int
rmixlp_pcie_error_intr(void *v)
{
	if (rmixlp_pcie_error_check(v) < 2)
		return 0;	/* correctable */

	/* uncorrectable */
#if DDB
	Debugger();
#endif

	/* XXX reset and recover? */

	panic("%s\n", __func__);
}
#endif
