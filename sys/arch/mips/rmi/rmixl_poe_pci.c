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

__KERNEL_RCSID(1, "$NetBSD: rmixl_poe_pci.c,v 1.1.2.1 2012/01/19 17:35:58 matt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/percpu.h>
#include <sys/kmem.h>

#include "locators.h"

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>

#include <mips/rmi/rmixl_poereg.h>
#include <mips/rmi/rmixl_fmnvar.h>

#include "locators.h"

#ifdef DEBUG
int xlpoe_debug = 0;
#define	DPRINTF(x, ...)	do { if (xlpoe_debug) printf(x, ## __VA_ARGS__); } while (0)
#else
#define	DPRINTF(x)
#endif

struct xlpoe_softc;

static int	xlpoe_pci_match(device_t, cfdata_t, void *);
static void	xlpoe_pci_attach(device_t, device_t, void *);
static int	xlpoe_intr(void *);
static int	xlpoe_msg_intr(void *, rmixl_fmn_rxmsg_t *);

static void	xlpoe_percpu_evcnt_attach(void *, void *, struct cpu_info *);
static void	xlpoe_find_memory(struct xlpoe_softc *);

struct xlpoe_class {
	paddr_t pcl_spill_enq_base;
	paddr_t pcl_spill_deq_base;
	size_t pcl_spill_enq_maxline;
	size_t pcl_spill_deq_maxline;

	size_t pcl_max_msg;		// max # of POE buffers
	size_t pcl_max_loc_buf_stg;	// max # of local POE buffers
};

struct xlpoe_softc {
	device_t sc_dev;
	pci_chipset_tag_t sc_pc;
	pcitag_t sc_tag;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;

	size_t sc_nflows;
	size_t sc_nonchip;
	size_t sc_noffchip;

	struct xlpoe_class sc_classes[RMIXLP_POE_NCLASS];
	paddr_t sc_msg_storage_base;
	paddr_t sc_msg_storage_limit;
	paddr_t sc_fbp_base;
	paddr_t sc_fbp_limit;

	percpu_t *sc_percpu_ev;
};

CFATTACH_DECL_NEW(xlpoe_pci, sizeof(struct xlpoe_softc),
    xlpoe_pci_match, xlpoe_pci_attach, NULL, NULL);

static int
xlpoe_pci_match(device_t parent, cfdata_t cf, void *aux)
{
	struct pci_attach_args * const pa = aux;

	if (pa->pa_id == PCI_ID_CODE(PCI_VENDOR_NETLOGIC, PCI_PRODUCT_NETLOGIC_XLP_POE))
		return 1;

	/*
	 * For some on the XLP3xxL, POE has a product_id of 0.
	 */
	if (pa->pa_tag == RMIXLP_POE_PCITAG)
		return 1;

        return 0;
}

static void
xlpoe_pci_attach(device_t parent, device_t self, void *aux)
{
	struct pci_attach_args * const pa = aux;
	struct xlpoe_softc * const sc = device_private(self);
	bus_addr_t base;
	bus_size_t size;
	char buf[8];

	sc->sc_dev = self;
	sc->sc_pc = pa->pa_pc;
	sc->sc_tag = pa->pa_tag;
	sc->sc_nflows = pci_conf_read(sc->sc_pc, sc->sc_tag,
	    PCI_RMIXLP_POE_FLOWS);
	sc->sc_nonchip = pci_conf_read(sc->sc_pc, sc->sc_tag,
	    PCI_RMIXLP_ONCHIP);
	sc->sc_noffchip = pci_conf_read(sc->sc_pc, sc->sc_tag,
	    PCI_RMIXLP_OFFCHIP);

	/*
	 * Why isn't this accessible via a BAR?
	 */
	if (pci_mapreg_map(pa, PCI_BAR0, PCI_MAPREG_TYPE_MEM, 0,
		    &sc->sc_bst, &sc->sc_bsh, &base, &size) != 0) {
		aprint_error(": can't map registers\n");
		return;
	}

	aprint_naive(": XLP POE Controller\n");
	aprint_normal(": XLP Packet Ordering Engine\n");

	format_bytes(buf, sizeof(buf), sc->sc_nonchip);
	aprint_normal_dev(sc->sc_dev, "%zu flows, messages: %s on-chip",
	    sc->sc_nflows, buf);

	format_bytes(buf, sizeof(buf), sc->sc_noffchip);
	aprint_normal(", %s off-chip\n", buf);

	aprint_normal_dev(sc->sc_dev, "BAR[0] (%zuKB @ %#"
	    PRIxBUSADDR") mapped at %#"PRIxBSH"\n",
	    (size_t)size >> 10, base, sc->sc_bsh);

	/*
	 * Now allocate and attach the per-cpu evcnt.
	 * We can't allocate the evcnt structure(s) directly since
	 * percpu will move the contents of percpu memory around and 
	 * corrupt the pointers in the evcnts themselves.  Remember, any
	 * problem can be solved with sufficient indirection.
	 */
	sc->sc_percpu_ev = percpu_alloc(sizeof(struct evcnt *));
	percpu_foreach(sc->sc_percpu_ev, xlpoe_percpu_evcnt_attach, sc);

	const pcireg_t statinfo = pci_conf_read(sc->sc_pc, sc->sc_tag,
	    PCI_RMIXLP_STATID);

	const size_t stid_start = PCI_RMIXLP_STATID_BASE(statinfo);
	const size_t stid_count = PCI_RMIXLP_STATID_COUNT(statinfo);
	if (stid_count) {
		aprint_normal_dev(sc->sc_dev, "%zu station%s starting at %zu\n",
		    stid_count, (stid_count == 1 ? "" : "s"), stid_start);
	}

	pci_intr_handle_t pcih;
	pci_intr_map(pa, &pcih);

	if (pci_intr_establish(pa->pa_pc, pcih, IPL_VM, xlpoe_intr, sc) == NULL) {
		aprint_error_dev(self, "failed to establish interrupt\n");
	} else {
		const char * const intrstr = pci_intr_string(pa->pa_pc, pcih);
		aprint_normal_dev(self, "interrupting at %s\n", intrstr);
	}

	if (NULL == rmixl_fmn_intr_establish(RMIXLP_FMN_STID_POE,
		    xlpoe_msg_intr, sc)) {
		aprint_error_dev(self, "failed to establish FMN msg interrupt\n");
	}

	xlpoe_find_memory(sc);
}

static int
xlpoe_intr(void *v)
{
	struct xlpoe_softc * const sc = v;

	panic("%s(%p)", device_xname(sc->sc_dev), v);
}

static int
xlpoe_msg_intr(void *v, rmixl_fmn_rxmsg_t *msg)
{
	struct xlpoe_softc * const sc = v;
	struct evcnt **ev_p = percpu_getref(sc->sc_percpu_ev);

	(*ev_p)->ev_count++;

	percpu_putref(sc->sc_percpu_ev);
	return 0;
}

static void
xlpoe_percpu_evcnt_attach(void *v0, void *v1, struct cpu_info *ci)
{
	struct evcnt ** const ev_p = v0;
	const char * const xname = device_xname(ci->ci_dev);

	struct evcnt * const ev = kmem_zalloc(sizeof(*ev), KM_SLEEP);

	*ev_p = ev;

	evcnt_attach_dynamic(ev, EVCNT_TYPE_MISC, NULL, xname, "poe msg");
}

static paddr_t
xlpoe_read_base(struct xlpoe_softc *sc, bus_size_t r_lo, bus_size_t r_hi)
{
	paddr_t lo = pci_conf_read(sc->sc_pc, sc->sc_tag, r_lo);
	paddr_t hi = pci_conf_read(sc->sc_pc, sc->sc_tag, r_hi);

	return lo + (hi << 32);
}

static void
xlpoe_find_memory(struct xlpoe_softc *sc)
{
	sc->sc_msg_storage_base = xlpoe_read_base(sc,
	    RMIXLP_POE_MSG_STORAGE_BASE_L, RMIXLP_POE_MSG_STORAGE_BASE_H);
	sc->sc_msg_storage_limit = sc->sc_msg_storage_base + 28*1024;

	sc->sc_fbp_base = xlpoe_read_base(sc,
	    RMIXLP_POE_FBP_BASE_L, RMIXLP_POE_FBP_BASE_H);
	sc->sc_fbp_limit = sc->sc_fbp_base + 56*1024;

	for (size_t cl = 0; cl < RMIXLP_POE_NCLASS; cl++) {
		struct xlpoe_class * const pcl = &sc->sc_classes[cl];
		pcl->pcl_spill_enq_base =
		    xlpoe_read_base(sc, RMIXLP_POE_CL_ENQ_SPILL_BASE_L(cl),
			RMIXLP_POE_CL_ENQ_SPILL_BASE_H(cl));
		pcl->pcl_spill_deq_base =
		    xlpoe_read_base(sc, RMIXLP_POE_CL_DEQ_SPILL_BASE_L(cl),
			RMIXLP_POE_CL_DEQ_SPILL_BASE_H(cl));
		pcl->pcl_spill_enq_maxline = pcl->pcl_spill_enq_base 
		    + pci_conf_read(sc->sc_pc, sc->sc_tag, 
			RMIXLP_POE_CL_ENQ_SPILL_MAXLINE(cl));
		pcl->pcl_spill_deq_maxline = pcl->pcl_spill_deq_base 
		    + pci_conf_read(sc->sc_pc, sc->sc_tag, 
			RMIXLP_POE_CL_DEQ_SPILL_MAXLINE(cl));

		pcl->pcl_max_msg = pci_conf_read(sc->sc_pc, sc->sc_tag, 
		    RMIXLP_POE_MAX_MSG_CL(cl));
		pcl->pcl_max_loc_buf_stg = pci_conf_read(sc->sc_pc, sc->sc_tag, 
		    RMIXLP_POE_MAX_LOC_BUF_STG_CL(cl));
	}
}
