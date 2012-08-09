/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: rmixl_fmn_pci.c,v 1.1.2.2 2012/08/09 19:46:40 matt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/extent.h>
#include <sys/malloc.h>		// for M_DEVBUF

#include <uvm/uvm_extern.h>

#include "locators.h"

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>
#include <mips/rmi/rmixl_intr.h>
#include <mips/rmi/rmixl_fmnvar.h>

#include "locators.h"

#ifdef DEBUG
int xlfmn_debug = 0;
#define	DPRINTF(x, ...)	do { if (xlfmn_debug) printf(x, ## __VA_ARGS__); } while (0)
#else
#define	DPRINTF(x)
#endif

static int	xlfmn_pci_match(device_t, cfdata_t, void *);
static void	xlfmn_pci_attach(device_t, device_t, void *);

struct xlfmn_stid_info {
	size_t si_nactive;
};

struct xlfmn_softc {
	device_t sc_dev;
	pci_chipset_tag_t sc_pc;
	pcitag_t sc_tag;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	size_t sc_popq_irtstart;
	size_t sc_popq_irtcount;
	size_t sc_noqueues;
	size_t sc_onchip_limit;
	paddr_t sc_spill_base;
	paddr_t sc_spill_limit;
	size_t sc_spill_max;
	struct extent *sc_spill_ex;
	struct extent *sc_onchip_ex;
	struct extent *sc_oqueue_ex;
	struct xlfmn_stid_info sc_si[RMIXLP_FMN_NSTID];
};

static void	xlfmn_intr_establish(struct xlfmn_softc *, size_t,
		    int (*)(void *), const char *);
static int	xlfmn_fatal_intr(void *);
static int	xlfmn_nonfatal_intr(void *);
static int	xlfmn_popq_intr(void *);

static inline uint32_t
xlfmn_read_4(struct xlfmn_softc *sc, bus_addr_t off)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, off);
}

static inline uint64_t
xlfmn_read_8(struct xlfmn_softc *sc, bus_addr_t off)
{
	return bus_space_read_8(sc->sc_bst, sc->sc_bsh, off);
}

static inline void
xlfmn_write_4(struct xlfmn_softc *sc, bus_addr_t off, uint32_t val)
{
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, off, val);
}

static inline void
xlfmn_write_8(struct xlfmn_softc *sc, bus_addr_t off, uint64_t val)
{
	bus_space_write_8(sc->sc_bst, sc->sc_bsh, off, val);
}

CFATTACH_DECL_NEW(xlfmn_pci, sizeof(struct xlfmn_softc),
    xlfmn_pci_match, xlfmn_pci_attach, NULL, NULL);

static int
xlfmn_pci_match(device_t parent, cfdata_t cf, void *aux)
{
	struct pci_attach_args * const pa = aux;

	if (pa->pa_id == PCI_ID_CODE(PCI_VENDOR_NETLOGIC, PCI_PRODUCT_NETLOGIC_XLP_FMN))
		return 1;

        return 0;
}

static void
xlfmn_pci_attach(device_t parent, device_t self, void *aux)
{
	struct pci_attach_args * const pa = aux;
	struct xlfmn_softc * const sc = device_private(self);
	bus_addr_t base;
	bus_size_t size;
	char buf[8];

	sc->sc_dev = self;
	sc->sc_pc = pa->pa_pc;
	sc->sc_tag = pa->pa_tag;
	sc->sc_noqueues = pci_conf_read(pa->pa_pc, pa->pa_tag,
	    PCI_RMIXLP_OQCOUNT);
	sc->sc_onchip_limit = pci_conf_read(pa->pa_pc, pa->pa_tag,
	    PCI_RMIXLP_ONCHIP);
	sc->sc_spill_max = pci_conf_read(pa->pa_pc, pa->pa_tag,
	    PCI_RMIXLP_OFFCHIP);
	sc->sc_spill_base = ~(paddr_t)0;

	sc->sc_oqueue_ex = extent_create("fmn-oqueues", 0,
	    sc->sc_noqueues - 1, M_DEVBUF, NULL, 0, EX_WAITOK);
	KASSERT(sc->sc_oqueue_ex != NULL);

	sc->sc_onchip_ex = extent_create("fmn-onchip", 0,
	    (sc->sc_onchip_limit - 1) >> 5, M_DEVBUF, NULL, 0, EX_WAITOK);
	KASSERT(sc->sc_onchip_ex != NULL);

	/*
	 * Why isn't this accessible via a BAR?
	 */
	if (pci_mapreg_map(pa, PCI_BAR0, PCI_MAPREG_TYPE_MEM, 0,
            &sc->sc_bst, &sc->sc_bsh, &base, &size) != 0) {
		aprint_error(": can't map registers\n");
		return;
	}

	aprint_normal(": XLP FMN Controller\n");

	aprint_normal_dev(sc->sc_dev, "BAR[0] (%zuKB @ %#"
	    PRIxBUSADDR") mapped at %#"PRIxBSH"\n",
	    (size_t)size >> 10, base, sc->sc_bsh);

	/*
	 * Print output queue limits information.
	 */
	format_bytes(buf, sizeof(buf), sc->sc_onchip_limit);
	aprint_normal_dev(sc->sc_dev,
	    "%zu output queues, message space: %s on-chip",
	    sc->sc_noqueues, buf);

	format_bytes(buf, sizeof(buf), sc->sc_spill_max);
	aprint_normal(", %s max off-chip\n", buf);

	/*
	 * FMN has 16 or 32(!) interrupts which map on 128 PopQs
	 */
	const pcireg_t irtinfo = pci_conf_read(sc->sc_pc, sc->sc_tag,
	    PCI_RMIXLP_IRTINFO);

	const size_t irtstart = PCI_RMIXLP_IRTINFO_BASE(irtinfo);
	const size_t irtcount = PCI_RMIXLP_IRTINFO_COUNT(irtinfo);

	sc->sc_popq_irtstart = irtstart;
	sc->sc_popq_irtcount = irtcount - 2;

	aprint_normal_dev(sc->sc_dev, "%zu interrupts starting at %zu (%s)"
	    ", %zu PopQs per interrupt\n",
	    irtcount, irtstart, rmixl_irt_string(irtstart),
	    256 / sc->sc_popq_irtcount);

	KASSERT(((irtcount - 2) & (irtcount - 3)) == 0);
	KASSERT(irtstart + irtcount - 1 == 45);

	xlfmn_intr_establish(sc, irtstart + irtcount - 1, xlfmn_fatal_intr,
	    "fatal");
	xlfmn_intr_establish(sc, irtstart + irtcount - 2, xlfmn_nonfatal_intr,
	    "non-fatal");

#if 0
	KASSERT(irtcount >= IPL_HIGH - IPL_VM + 1);

	for (size_t irt = 0; irt < irtcount; irt++) {
		if (rmixl_intr_establish(irtstart + irt, IPL_VM,
		    IST_LEVEL_HIGH, xlfmn_popq_intr, sc, true) == NULL)
			panic("%s: failed to establish interrupt %zu",
			    __func__, irtstart + irt);
	}
#else
	(void) xlfmn_popq_intr;
#endif

	for (size_t qid = 0; qid < sc->sc_noqueues; qid++) {
		uint64_t oq_config = xlfmn_read_8(sc, RMIXLP_FMN_OQ_CONFIG(qid));

		uintmax_t sb = __SHIFTOUT(oq_config, RMIXLP_FMN_OQ_CONFIG_SB);
		uintmax_t sl = __SHIFTOUT(oq_config, RMIXLP_FMN_OQ_CONFIG_SL);
		uintmax_t ss = __SHIFTOUT(oq_config, RMIXLP_FMN_OQ_CONFIG_SS);

		uintmax_t ob = __SHIFTOUT(oq_config, RMIXLP_FMN_OQ_CONFIG_OB);
		uintmax_t ol = __SHIFTOUT(oq_config, RMIXLP_FMN_OQ_CONFIG_OL);
		uintmax_t os = __SHIFTOUT(oq_config, RMIXLP_FMN_OQ_CONFIG_OS);

		/*
		 * Even if the queue isn't enabled, record the spill area
		 * allocated to it.
		 */
		if (oq_config & RMIXLP_FMN_OQ_CONFIG_SE) {
			paddr_t spill_base = (64 * sb + ss) << 12;
			paddr_t spill_limit =  (64 * sb + sl + 1) << 12;
			if (spill_base < spill_limit) {
				if (spill_base < sc->sc_spill_base)
					sc->sc_spill_base = spill_base;
				if (spill_limit > sc->sc_spill_limit)
					sc->sc_spill_limit = spill_limit;
			}
		}

		if ((oq_config & RMIXLP_FMN_OQ_CONFIG_OE) == 0)
			continue;

		/*
		 * Note the station has an active queue
		 */
		size_t stid = rmixl_fmn_qid_to_stid(qid);
		sc->sc_si[stid].si_nactive++;

		aprint_debug_dev(sc->sc_dev,
		    "OQ[%zu]: %#"PRIx64"<", qid, oq_config);
		aprint_debug("oe=%ju,se=%ju",
		    __SHIFTOUT(oq_config, RMIXLP_FMN_OQ_CONFIG_OE),
		    __SHIFTOUT(oq_config, RMIXLP_FMN_OQ_CONFIG_SE));

		if (oq_config & RMIXLP_FMN_OQ_CONFIG_INT) {
			aprint_debug(",i=1");
		}
		
		if (oq_config &
			(RMIXLP_FMN_OQ_CONFIG_LV|RMIXLP_FMN_OQ_CONFIG_LT
			|RMIXLP_FMN_OQ_CONFIG_TV|RMIXLP_FMN_OQ_CONFIG_TT)) {
			aprint_debug(", lv=%ju,lt=%ju,tv=%ju,tt=%ju",
			    __SHIFTOUT(oq_config, RMIXLP_FMN_OQ_CONFIG_LV),
			    __SHIFTOUT(oq_config, RMIXLP_FMN_OQ_CONFIG_LT),
			    __SHIFTOUT(oq_config, RMIXLP_FMN_OQ_CONFIG_TV),
			    __SHIFTOUT(oq_config, RMIXLP_FMN_OQ_CONFIG_TT));
		}
		if (oq_config & RMIXLP_FMN_OQ_CONFIG_SE) {
			aprint_debug(", spill=%#jx..%#jx",
			    (64 * sb + ss) << 12,
			    ((64 * sb + sl + 1) << 12) - 1);
			aprint_debug("<sb=%#jx,sl=%#jx,ss=%#jx>", sb,sl,ss);
		}
		aprint_debug(", onchip=%#jx..%#jx",
		    ((ob << 5) + os) << 5,
		    (((ob << 5) + ol + 1) << 5) - 1);
		aprint_debug("<ob=%#jx,ol=%#jx,ob=%#jx>\n", ob, ol, os);
	}

#if 0
	/*
	 * Truncate base to a 256KB boundary.
	 * Round liumit up the next 256KB boundary.
	 */
	sc->sc_spill_base &= -__BIT(18);
	sc->sc_spill_limit = roundup2(sc->sc_spill_limit, __BIT(18)); 
#endif

	format_bytes(buf, sizeof(buf), sc->sc_spill_limit - sc->sc_spill_base);
	aprint_normal_dev(sc->sc_dev,
	    "spill area: %s: base=%#"PRIxPADDR", limit=%#"PRIxPADDR"\n",
	    buf, sc->sc_spill_base, sc->sc_spill_limit);

	if (sc->sc_spill_base < sc->sc_spill_limit) {
		/*
		 * Let's try to allocate the spill area.
		 */
		struct pglist mlist;
		int error = uvm_pglistalloc(
		    sc->sc_spill_limit - sc->sc_spill_base,
		    sc->sc_spill_base, sc->sc_spill_limit,
		    0, 0, &mlist, 1, true);
		if (error)
			aprint_error_dev(sc->sc_dev,
			    "failed to allocate spill area: %d\n", error);
	}

	aprint_normal_dev(sc->sc_dev, "active queues: ");
	const char *pfx = "";
	for (size_t stid = RMIXLP_FMN_STID_CPU;
	     stid < RMIXLP_FMN_NSTID;
	     stid++) {
		struct xlfmn_stid_info * const si = &sc->sc_si[stid];
		if (si->si_nactive > 0) {
			aprint_normal("%s%s(%zu)",
			    pfx,
			    rmixl_fmn_stid_name(stid),
			    si->si_nactive);
			pfx = ", ";
		}
	}
	aprint_normal("\n");
}

static void
xlfmn_intr_establish(struct xlfmn_softc *sc, size_t irt, int (*func)(void *),
	const char *desc)
{
	if (rmixl_intr_establish(irt, IPL_VM, IST_LEVEL_HIGH, func,
	    sc, true) == NULL)
		panic("%s: failed to establish %s interrupt %zu",
		    device_xname(sc->sc_dev), desc, irt);
	aprint_normal_dev(sc->sc_dev, "%s interrupt at %zu (%s)\n",
	    desc, irt, rmixl_irt_string(irt));
}

static int
xlfmn_fatal_intr(void *v)
{
	struct xlfmn_softc * const sc = v;

	panic("%s(%p)", device_xname(sc->sc_dev), v);
}

static int
xlfmn_nonfatal_intr(void *v)
{
	struct xlfmn_softc * const sc = v;

	panic("%s(%p)", device_xname(sc->sc_dev), v);
}

static int
xlfmn_popq_intr(void *v)
{
	struct xlfmn_softc * const sc = v;

	panic("%s(%p)", device_xname(sc->sc_dev), v);
}
