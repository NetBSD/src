/*	$NetBSD: rmixl_pcie_subr.c,v 1.1.2.1 2013/11/05 18:43:31 matt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: rmixl_pcie_subr.c,v 1.1.2.1 2013/11/05 18:43:31 matt Exp $");

#include "opt_pci.h"
#include "pci.h"

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/kernel.h>		/* for 'hz' */
#include <sys/atomic.h>
#include <sys/kmem.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>
#include <mips/rmi/rmixl_intr.h>
#include <mips/rmi/rmixl_pcievar.h>

#include <mips/rmi/rmixl_obiovar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciconf.h>

#ifdef	PCI_NETBSD_CONFIGURE
#include <mips/cache.h>
#endif

#include <machine/pci_machdep.h>

#ifdef PCI_DEBUG
int rmixl_pcie_debug = PCI_DEBUG;
# define DPRINTF(x)	do { if (rmixl_pcie_debug) printf x ; } while (0)
#else
# define DPRINTF(x)
#endif

#ifndef DDB
# define STATIC static
#else
# define STATIC
#endif

static int
rmixl_pcie_intr_stray(void *v)
{
	return 0;
}

pci_intr_handle_t
rmixl_pcie_make_pih(u_int link, u_int bitno, u_int irq)
{
	pci_intr_handle_t pih;

	KASSERT(link < RMIXL_PCIE_NLINKS_MAX);
	KASSERT(bitno < 64);
	KASSERT(irq < 256);

	pih  = (link << 14);
	pih |= (bitno << 8);
	pih |= irq;

	return pih;
}

void
rmixl_pcie_decompose_pih(pci_intr_handle_t pih, u_int *link, u_int *bitno,
    u_int *irq)
{
	if (link) {
		*link = (u_int)((pih >> 14) & 0x3);
		KASSERT(*link < RMIXL_PCIE_NLINKS_MAX);
	}
	if (bitno) {
		*bitno = (u_int)((pih >> 8) & 0x3f);
		KASSERT(*bitno < 64);
	}
	if (irq) {
		*irq = (u_int)((pih >> 0) & 0xff);
		KASSERT(*irq < 256);
	}
}

static inline size_t
rmixl_pcie_pli_size(size_t maxplds)
{
	return offsetof(rmixl_pcie_link_intr_t, pli_plds[maxplds]);
}

static void
rmixl_pcie_pli_free(void *arg)
{
	rmixl_pcie_link_intr_t * const pli = arg;
	
	callout_destroy(&pli->pli_callout);
	kmem_free(pli, rmixl_pcie_pli_size(pli->pli_maxplds));
}

/*
 * delay free of the old link interrupt set
 * to allow anyone still using it to do so safely
 * XXX 2 seconds should be plenty?
 */
static void
rmixl_pcie_pli_free_callout(rmixl_pcie_link_intr_t *pli)
{
	callout_init(&pli->pli_callout, 0);
	callout_reset(&pli->pli_callout, 2 * hz, rmixl_pcie_pli_free, pli);
}

static rmixl_pcie_link_intr_t *
rmixl_pcie_pli_add_slot(rmixl_pcie_softc_t *sc, u_int link, int irq, int ipl)
{
	rmixl_pcie_link_intr_t * const pli_old = sc->sc_link_intr[link];

	if (pli_old != NULL && pli_old->pli_nplds < pli_old->pli_maxplds) {
		return pli_old;
	}

	/*
	 * allocate and initialize link intr struct
	 * with one or more dispatch handles
	 */
	const size_t maxplds = 1 + pli_old->pli_maxplds;
	const size_t size = rmixl_pcie_pli_size(maxplds);
	rmixl_pcie_link_intr_t * const pli_new = kmem_zalloc(size, KM_SLEEP);
	if (pli_new == NULL) {
#ifdef DIAGNOSTIC
		printf("%s: cannot alloc\n", __func__);
#endif
		return NULL;
	}

	if (pli_old == NULL) {
		/* initialize the link interrupt struct */
		pli_new->pli_sc = sc;
		pli_new->pli_link = link;
		pli_new->pli_ipl = ipl;
		pli_new->pli_ih = rmixl_intr_establish(irq, ipl, IST_LEVEL_HIGH,
			rmixl_pcie_intr, pli_new, false);
		if (pli_new->pli_ih == NULL)
			panic("%s: cannot establish irq %d", __func__, irq);
	} else {
		/*
		 * all intrs on a link get same ipl and sc
		 * first intr established sets the standard
		 */
		KASSERT(sc == pli_old->pli_sc);
		if (sc != pli_old->pli_sc) {
			printf("%s: sc %p mismatch\n", __func__, sc); 
			kmem_free(pli_new, size);
			return NULL;
		}
		KASSERT (ipl == pli_old->pli_ipl);
		if (ipl != pli_old->pli_ipl) {
			printf("%s: ipl %d mismatch\n", __func__, ipl); 
			kmem_free(pli_new, size);
			return NULL;
		}
		/*
		 * copy pli_old to pli_new, skipping unused dispatch elemets
		 */
		memcpy(pli_new, pli_old, size);

		/*
		 * schedule delayed free of old link interrupt set
		 */
		rmixl_pcie_pli_free_callout(pli_old);
	}
	pli_new->pli_maxplds = maxplds;

	/* commit the new link interrupt set */
	sc->sc_link_intr[link] = pli_new;

	return pli_new;
}

void
rmixl_pcie_link_intr_disestablish(void *v, void *ih)
{
	rmixl_pcie_softc_t * const sc = v;
	rmixl_pcie_link_dispatch_t * const pld = ih;
	rmixl_pcie_link_intr_t * const pli = sc->sc_link_intr[pld->pld_link];
	const u_int link = pld->pld_link;
	const u_int bitno = pld->pld_bitno;

	DPRINTF(("%s: link=%d bitno=%d irq=%d\n",
	    __func__, link, bitno, pld->pld_irq));

	mutex_enter(&sc->sc_mutex);

	/* mark unused, prevent further dispatch */
	pld->pld_func = rmixl_pcie_intr_stray;

	/*
	 * if no other dispatch handle is using this interrupt,
	 * we can disable it
	 */
	bool inuse = false;
	for (size_t i = 0; !inuse && i < pli->pli_nplds; i++) {
		rmixl_pcie_link_dispatch_t * const d = pli->pli_plds[i];
		inuse = (pld != d && d->pld_bitno == pld->pld_bitno);
	}

	if (!inuse && (*sc->sc_link_intr_change)(sc, link, bitno, false)) {
		/* tear down interrupt on this link */
		rmixl_intr_disestablish(pli->pli_ih);

		/* commit NULL interrupt set */
		sc->sc_link_intr[pld->pld_link] = NULL;

		/* schedule delayed free of the old link interrupt set */
		rmixl_pcie_pli_free_callout(pli);
	} else {
		KASSERT(pli->pli_nplds > 1);
		rmixl_pcie_link_dispatch_t * const last_pld = 
		    pli->pli_plds[--pli->pli_nplds];
		pli->pli_plds[pli->pli_nplds] = NULL;
		if (last_pld != pld) {
			for (size_t i = 0; i < pli->pli_nplds; i++) {
				if (pli->pli_plds[i] == pld) {
					pli->pli_plds[i] = last_pld;
					break;
				}
			}
		}
	}
	kmem_free(pld, sizeof(*pld));

	mutex_exit(&sc->sc_mutex);
}

void *
rmixl_pcie_link_intr_establish(rmixl_pcie_softc_t *sc, pci_intr_handle_t pih,
	int ipl, int (*func)(void *), void *arg)
{
	if (pih == ~0) {
		DPRINTF(("%s: bad pih=%#lx, implies PCI_INTERRUPT_PIN_NONE\n",
			__func__, pih));
		return NULL;
	}

	u_int link, bitno, irq;
	rmixl_pcie_decompose_pih(pih, &link, &bitno, &irq);
	DPRINTF(("%s: link=%d pin=%d irq=%d\n",
		__func__, link, bitno + 1, irq));

	mutex_enter(&sc->sc_mutex);

	rmixl_pcie_link_intr_t * const pli =
	    rmixl_pcie_pli_add_slot(sc, link, irq, ipl);
	if (pli == NULL)
		return NULL;

	/*
	 * initialize our new interrupt, the last element in plds[]
	 */
	rmixl_pcie_link_dispatch_t * const pld =
	    kmem_zalloc(sizeof(*pld), KM_SLEEP);

	pld->pld_link = link;
	pld->pld_bitno = bitno;
	pld->pld_irq = irq;
	pld->pld_func = func;
	pld->pld_arg = arg;
	pld->pld_evs = RMIXL_PCIE_EVCNT(sc, link, bitno, 0);

	pli->pli_plds[pli->pli_nplds++] = pld;
	membar_producer();

	(*sc->sc_link_intr_change)(sc, link, bitno, true);

	mutex_exit(&sc->sc_mutex);
	return pld;
}


int
rmixl_pcie_link_intr(rmixl_pcie_link_intr_t *pli, uint64_t status)
{
	int rv = 0;

	KASSERT(status != 0);

	DPRINTF(("%s: %d:%#"PRIx64"\n", __func__, pli->pli_link, status));

	const size_t ci_idx = cpu_index(curcpu());

	for (size_t i = 0; i < pli->pli_nplds; i++) {
		rmixl_pcie_link_dispatch_t * const d = pli->pli_plds[i];
		if ((status & __BIT(d->pld_bitno)) != 0) {
			rv = rmixl_intr_deliver(&d->pld_common,
			    &d->pld_evs[ci_idx].evcnt, pli->pli_ipl);
		}
	}

	return rv;
}
