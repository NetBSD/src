/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#define PCIE_PRIVATE

#include "locators.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: bcm53xx_pax.c,v 1.2 2012/09/14 04:53:58 matt Exp $");

#include <sys/bus.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include <arm/broadcom/bcm53xx_reg.h>
#include <arm/broadcom/bcm53xx_var.h>

static const struct {
	paddr_t owin_base;
	psize_t owin_size;
} bcmpax_owins[] = {
	[0] = { BCM53XX_PCIE0_OWIN_PBASE, BCM53XX_PCIE0_OWIN_SIZE },
	[1] = { BCM53XX_PCIE1_OWIN_PBASE, BCM53XX_PCIE1_OWIN_SIZE },
	[2] = { BCM53XX_PCIE2_OWIN_PBASE, BCM53XX_PCIE2_OWIN_SIZE },
};

static int bcmpax_ccb_match(device_t, cfdata_t, void *);
static void bcmpax_ccb_attach(device_t, device_t, void *);

struct bcmpax_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_dma_tag_t sc_dmat;
	kmutex_t *sc_lock;
	kmutex_t *sc_cfg_lock;
	bool sc_linkup;
	int sc_pba_flags;
	struct arm32_pci_chipset sc_pc;
};

static inline uint32_t
bcmpax_read_4(struct bcmpax_softc *sc, bus_size_t o)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, o);
}

static inline void
bcmpax_write_4(struct bcmpax_softc *sc, bus_size_t o, uint32_t v)
{
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, o, v);
}

static void bcmpax_attach_hook(device_t, device_t, struct pcibus_attach_args *);
static int bcmpax_bus_maxdevs(void *, int);
static pcitag_t bcmpax_make_tag(void *, int, int, int);
static void bcmpax_decompose_tag(void *, pcitag_t, int *, int *, int *);
static pcireg_t bcmpax_conf_read(void *, pcitag_t, int);
static void bcmpax_conf_write(void *, pcitag_t, int, pcireg_t);

static int bcmpax_intr_map(const struct pci_attach_args *, pci_intr_handle_t *);
static const char *bcmpax_intr_string(void *, pci_intr_handle_t);
static const struct evcnt *bcmpax_intr_evcnt(void *, pci_intr_handle_t);
static void *bcmpax_intr_establish(void *, pci_intr_handle_t, int,
	   int (*)(void *), void *);
static void bcmpax_intr_disestablish(void *, void *);

#ifdef __HAVE_PCI_CONF_HOOK
static int bcmpax_conf_hook(void *, int, int, int, pcireg_t);
#endif
static void bcmpax_conf_interrupt(void *, int, int, int, int, int *);


CFATTACH_DECL_NEW(bcmpax_ccb, sizeof(struct bcmpax_softc),
	bcmpax_ccb_match, bcmpax_ccb_attach, NULL, NULL);

static int
bcmpax_ccb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct bcmccb_attach_args * const ccbaa = aux;
	const struct bcm_locators * const loc = &ccbaa->ccbaa_loc;

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

#ifdef DIAGNOSTIC
	const int port = cf->cf_loc[BCMCCBCF_PORT];
#endif
	KASSERT(port == BCMCCBCF_PORT_DEFAULT || port == loc->loc_port);

	return 1;
}

static void
bcmpax_ccb_attach(device_t parent, device_t self, void *aux)
{
	struct bcmpax_softc * const sc = device_private(self);
	struct bcmccb_attach_args * const ccbaa = aux;
	const struct bcm_locators * const loc = &ccbaa->ccbaa_loc;

	sc->sc_dev = self;

	sc->sc_bst = ccbaa->ccbaa_ccb_bst;
	sc->sc_dmat = ccbaa->ccbaa_dmat;

	bus_space_subregion(sc->sc_bst, ccbaa->ccbaa_ccb_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	bcmpax_write_4(sc, PCIE_CLK_CONTROL, 3);
	delay(250);
	bcmpax_write_4(sc, PCIE_CLK_CONTROL, 1);
	// delay(100*1000);

	uint32_t v = bcmpax_read_4(sc, PCIE_STRAP_STATUS);
	const bool enabled = (v & STRAP_PCIE_IF_ENABLE) != 0;
	const bool is_v2_p = (v & STRAP_PCIE_USER_FOR_CE_GEN1) == 0;
	const bool is_x2_p = (v & STRAP_PCIE_USER_FOR_CE_1LANE) == 0;
	const bool is_rc_p = (v & STRAP_PCIE_USER_RC_MODE) != 0;

	aprint_naive("\n");
	aprint_normal(": PCI Express V%u %u-lane %s Controller%s\n",
	    is_v2_p ? 2 : 1,
	    is_x2_p ? 2 : 1,
	    is_rc_p ? "RC" : "EP",
	    enabled ? "" : "(disabled)");
	if (!enabled || !is_rc_p)
		return;

	sc->sc_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_VM);
	sc->sc_cfg_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_VM);

	sc->sc_pc.pc_conf_v = sc;
	sc->sc_pc.pc_attach_hook = bcmpax_attach_hook;
	sc->sc_pc.pc_bus_maxdevs = bcmpax_bus_maxdevs;
	sc->sc_pc.pc_make_tag = bcmpax_make_tag;
	sc->sc_pc.pc_decompose_tag = bcmpax_decompose_tag;
	sc->sc_pc.pc_conf_read = bcmpax_conf_read;
	sc->sc_pc.pc_conf_write = bcmpax_conf_write;

	sc->sc_pc.pc_intr_v = sc;
	sc->sc_pc.pc_intr_map = bcmpax_intr_map;
	sc->sc_pc.pc_intr_string = bcmpax_intr_string;
	sc->sc_pc.pc_intr_evcnt = bcmpax_intr_evcnt;
	sc->sc_pc.pc_intr_establish = bcmpax_intr_establish;
	sc->sc_pc.pc_intr_disestablish = bcmpax_intr_disestablish;

#ifdef __HAVE_PCI_CONF_HOOK
	sc->sc_pc.pc_conf_hook = bcmpax_conf_hook;
#endif
	sc->sc_pc.pc_conf_interrupt = bcmpax_conf_interrupt;

	// sc->sc_pba_flags |= PCI_FLAGS_MSI_OKAY;
	// sc->sc_pba_flags |= PCI_FLAGS_MSIX_OKAY;

	int offset;
	const bool ok = pci_get_capability(&sc->sc_pc, 0, PCI_CAP_PCIEXPRESS,
	    &offset, NULL);
	KASSERT(ok);

	/*
	 * Now we wait (.1sec) for the link to come up.
	 */
	offset += PCI_PCIE_LCSR;
	for (size_t timo = 0;; timo++) {
		const pcireg_t lcsr = bcmpax_conf_read(sc, 0, offset); 
		sc->sc_linkup = __SHIFTOUT(lcsr, PCI_PCIE_LCSR_NLW) != 0
		    && (1 || (lcsr & PCI_PCIE_LCSR_DLACTIVE) != 0);
		if (sc->sc_linkup || timo == 250) {
			aprint_debug_dev(self,
			    "lcsr=%#x nlw=%jd linkup=%d, timo=%zu\n",
			    lcsr, __SHIFTOUT(lcsr, PCI_PCIE_LCSR_NLW),
			    sc->sc_linkup, timo);
			break;
		}
		DELAY(1000);
	}

	if (sc->sc_linkup) {
		paddr_t base = bcmpax_owins[loc->loc_port].owin_base;
		psize_t size = bcmpax_owins[loc->loc_port].owin_size;
		KASSERT((size & ~PCIE_OARR_ADDR) == 0);
		if (size > 0) {
			bcmpax_write_4(sc, PCIE_OARR_0, base);
			bcmpax_write_4(sc, PCIE_OMAP_0_LOWER, base | 1);
		}
		if (size > __LOWEST_SET_BIT(PCIE_OARR_ADDR)) {
			paddr_t base1 = base + __LOWEST_SET_BIT(PCIE_OARR_ADDR);
			bcmpax_write_4(sc, PCIE_OARR_1, base1);
			bcmpax_write_4(sc, PCIE_OMAP_1_LOWER, base1 | 1);
		}

		struct extent *memext = extent_create("pcimem", base,
		     base + size, NULL, 0, EX_NOWAIT);

		int error = pci_configure_bus(&sc->sc_pc,
		    NULL, memext, NULL, 0, arm_pcache.dcache_line_size);

		extent_destroy(memext);

		if (error) {
			aprint_normal_dev(self, "configuration failed\n");
			return;
		}
	}

	struct pcibus_attach_args pba;
	memset(&pba, 0, sizeof(pba));
		 
	pba.pba_flags = sc->sc_pba_flags;
	pba.pba_flags |= PCI_FLAGS_MEM_OKAY;
	pba.pba_memt = sc->sc_bst;
	pba.pba_dmat = sc->sc_dmat;
	pba.pba_pc = &sc->sc_pc;
	pba.pba_bus = 0;

	config_found_ia(self, "pcibus", &pba, pcibusprint);
}

static void
bcmpax_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{
}

static int
bcmpax_bus_maxdevs(void *v, int bus)
{
	struct bcmpax_softc * const sc = v;

	if (__predict_true(sc->sc_linkup))
		return bus > 1 ? 32 : 1;

	return bus ? 0 : 1;
}

static void
bcmpax_decompose_tag(void *v, pcitag_t tag, int *busp, int *devp, int *funcp)
{
	if (busp)
		*busp = __SHIFTOUT(tag, CFG_ADDR_BUS);
	if (devp)
		*devp = __SHIFTOUT(tag, CFG_ADDR_DEV);
	if (funcp)
		*funcp = __SHIFTOUT(tag, CFG_ADDR_FUNC);
}       

static pcitag_t
bcmpax_make_tag(void *v, int bus, int dev, int func)
{
	return __SHIFTIN(bus, CFG_ADDR_BUS)
	   | __SHIFTIN(dev, CFG_ADDR_DEV)
	   | __SHIFTIN(func, CFG_ADDR_FUNC)
	   | (bus == 0 ? CFG_ADDR_TYPE0 : CFG_ADDR_TYPE1);
}

static inline bus_size_t
bcmpax_conf_addr_write(struct bcmpax_softc *sc, pcitag_t tag)
{
	if ((tag & (CFG_ADDR_BUS|CFG_ADDR_DEV)) == 0) {
		uint32_t reg = __SHIFTOUT(tag, CFG_ADDR_REG);
		uint32_t func = __SHIFTOUT(tag, CFG_ADDR_FUNC);
		bcmpax_write_4(sc, PCIE_CFG_IND_ADDR,
		    __SHIFTIN(func, CFG_IND_ADDR_FUNC)
		    | __SHIFTIN(reg, CFG_IND_ADDR_REG));
		__asm __volatile("dsb");
		return PCIE_CFG_IND_DATA;
	}
	if (sc->sc_linkup) {
		bcmpax_write_4(sc, PCIE_CFG_ADDR, tag);
		__asm __volatile("dsb");
		return PCIE_CFG_DATA;
	} 
	return 0;
}

static pcireg_t
bcmpax_conf_read(void *v, pcitag_t tag, int reg)
{
	struct bcmpax_softc * const sc = v;

	/*
	 * Even in RC mode, the PCI Express Root Complex return itself
	 * as BCM Ethernet Controller!.  We could change ppb.c to match it
	 * but we'll just lie and say we are a PPB bridge.
	 */
	if ((tag & (CFG_ADDR_BUS|CFG_ADDR_DEV|CFG_ADDR_FUNC)) == 0
	    && reg == PCI_CLASS_REG) {
		return PCI_CLASS_CODE(PCI_CLASS_BRIDGE,
				      PCI_SUBCLASS_BRIDGE_PCI, 0);
	}

	//printf("%s: tag %#lx reg %#x:", __func__, tag, reg);

	mutex_enter(sc->sc_cfg_lock);
	bus_size_t data_reg = bcmpax_conf_addr_write(sc, tag | reg);

	//printf(" [from %#lx]:\n", data_reg);

	pcireg_t rv;
	if (data_reg)
		rv = bcmpax_read_4(sc, data_reg);
	else
		rv = 0xffffffff;

	mutex_exit(sc->sc_cfg_lock);

	//printf(" %#x\n", rv);

	return rv;
}

static void
bcmpax_conf_write(void *v, pcitag_t tag, int reg, pcireg_t val)
{
	struct bcmpax_softc * const sc = v;

	mutex_enter(sc->sc_cfg_lock);
	bus_size_t data_reg = bcmpax_conf_addr_write(sc, tag | reg);

	//printf("%s: tag %#lx reg %#x:", __func__, tag, reg);

	if (data_reg) {
		//printf(" [to %#lx]:\n", data_reg);
		bcmpax_write_4(sc, data_reg, val);
		//printf(" %#x\n", val);
	}

	mutex_exit(sc->sc_cfg_lock);
}

static void
bcmpax_conf_interrupt(void *v, int bus, int dev, int ipin, int swiz, int *ilinep)
{
	*ilinep = 5; /* (ipin + swiz) & 3; */
}

static int
bcmpax_conf_hook(void *v, int bus, int dev, int func, pcireg_t id)
{
	if (func > 0)
		return 0;

	return PCI_CONF_ENABLE_MEM | PCI_CONF_MAP_MEM;
}

static int
bcmpax_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *pihp)
{
	return EINVAL;
}

static const char *
bcmpax_intr_string(void *v, pci_intr_handle_t pih)
{
	return NULL;
}

static const struct evcnt *
bcmpax_intr_evcnt(void *v, pci_intr_handle_t pih)
{
	return NULL;
}

static void *
bcmpax_intr_establish(void *v, pci_intr_handle_t pih, int ipl,
   int (*func)(void *), void *arg)
{
	return NULL;
}

static void
bcmpax_intr_disestablish(void *v, void *ih)
{
}
