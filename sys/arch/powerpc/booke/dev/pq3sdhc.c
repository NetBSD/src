/*	$NetBSD: pq3sdhc.c,v 1.5.2.1 2017/12/03 11:36:36 jdolecek Exp $	*/
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

#define	ESDHC_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pq3sdhc.c,v 1.5.2.1 2017/12/03 11:36:36 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/queue.h>

#include <sys/bus.h>

#include <powerpc/booke/cpuvar.h>
#include <powerpc/booke/e500var.h>
#include <powerpc/booke/e500reg.h>

#include <dev/sdmmc/sdhcreg.h>
#include <dev/sdmmc/sdhcvar.h>

#define	EDSHC_HOST_CTL_RES	0x05

static int pq3sdhc_match(device_t, cfdata_t, void *);
static void pq3sdhc_attach(device_t, device_t, void *);

struct pq3sdhc_softc {
	struct sdhc_softc	sc;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	struct sdhc_host	*sc_hosts[1];
	void 			*sc_ih;		/* interrupt vectoring */
};

CFATTACH_DECL_NEW(pq3sdhc, sizeof(struct pq3sdhc_softc),
    pq3sdhc_match, pq3sdhc_attach, NULL, NULL);

static int
pq3sdhc_match(device_t parent, cfdata_t cf, void *aux)
{

        if (!e500_cpunode_submatch(parent, cf, cf->cf_name, aux))
                return 0;

        return 1;
}

static void
pq3sdhc_attach(device_t parent, device_t self, void *aux)
{
	struct cpunode_softc * const psc = device_private(parent);
	struct pq3sdhc_softc * const sc = device_private(self);
	struct cpunode_attach_args * const cna = aux;
	struct cpunode_locators * const cnl = &cna->cna_locs;
	int error;

	psc->sc_children |= cna->cna_childmask;
	sc->sc.sc_dmat = cna->cna_dmat;
	sc->sc.sc_dev = self;
	sc->sc.sc_flags |= SDHC_FLAG_USE_DMA;
	sc->sc.sc_flags |=
	    SDHC_FLAG_HAVE_DVS | SDHC_FLAG_32BIT_ACCESS | SDHC_FLAG_ENHANCED;
	sc->sc.sc_host = sc->sc_hosts;
	sc->sc.sc_clkbase = board_info_get_number("bus-frequency") / 2000;
	sc->sc_bst = cna->cna_memt;

	error = bus_space_map(sc->sc_bst, cnl->cnl_addr, cnl->cnl_size, 0,
	    &sc->sc_bsh);
	if (error) {
		aprint_error_dev(self,
		    "can't map registers for %s: %d\n", cnl->cnl_name, error);
		return;
	}

	/*
	 * If using DMA, enable SNOOPing.
	 */
	if (sc->sc.sc_flags & SDHC_FLAG_USE_DMA) {
		uint32_t dcr = bus_space_read_4(sc->sc_bst, sc->sc_bsh, DCR);
		dcr |= DCR_SNOOP | DCR_RD_SAFE | DCR_RD_PFE;
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, DCR, dcr);
	}

	aprint_naive(": SDHC controller\n");
	aprint_normal(": SDHC controller%s\n",
	   (sc->sc.sc_flags & SDHC_FLAG_USE_DMA) ? " (DMA enabled)" : "");

	sc->sc_ih = intr_establish(cnl->cnl_intrs[0], IPL_VM, IST_ONCHIP,
	    sdhc_intr, &sc->sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     cnl->cnl_intrs[0]);
		goto fail;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n",
	     cnl->cnl_intrs[0]);

	error = sdhc_host_found(&sc->sc, sc->sc_bst, sc->sc_bsh,
	    cnl->cnl_size);
	if (error != 0) {
		aprint_error_dev(self, "couldn't initialize host, error=%d\n",
		    error);
		goto fail;
	}
	return;

fail:
	if (sc->sc_ih) {
		intr_disestablish(sc->sc_ih);
		sc->sc_ih = NULL;
	}
	bus_space_unmap(sc->sc_bst, sc->sc_bsh, cnl->cnl_size);
}
