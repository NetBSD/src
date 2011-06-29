/*	$NetBSD: pq3sdhc.c,v 1.3 2011/06/29 06:12:10 matt Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: pq3sdhc.c,v 1.3 2011/06/29 06:12:10 matt Exp $");

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
	struct powerpc_bus_space sc_mybst;
	struct sdhc_softc	sc;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	struct sdhc_host	*sc_hosts[1];
	void 			*sc_ih;		/* interrupt vectoring */
};

CFATTACH_DECL_NEW(pq3sdhc, sizeof(struct pq3sdhc_softc),
    pq3sdhc_match, pq3sdhc_attach, NULL, NULL);

static uint8_t
pq3sdhc_read_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{
	const struct pq3sdhc_softc * const sc = (const void *) t;
	
	KASSERT((o & -4) != SDHC_DATA);

	const uint32_t v = bus_space_read_4(sc->sc_bst, h, o & -4);

	return v >> ((o & 3) * 8);
}

static uint16_t
pq3sdhc_read_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{
	const struct pq3sdhc_softc * const sc = (const void *) t;

	KASSERT((o & 1) == 0);
	KASSERT((o & -4) != SDHC_DATA);

	uint32_t v = bus_space_read_4(sc->sc_bst, h, o & -4);

	if (__predict_false(o == SDHC_HOST_VER))
		return v;
	if (__predict_false(o == SDHC_NINTR_STATUS)) {
		v |= SDHC_ERROR_INTERRUPT * ((v > 0xffff) != 0);
		if (v != 0)
			printf("get(INTR_STATUS)=%#x\n", v);
	}
	if (__predict_false(o == SDHC_EINTR_STATUS)) {
		if (v != 0)
			printf("get(INTR_STATUS)=%#x\n", v);
	}

	return v >> ((o & 2) * 8);
}

static uint32_t
pq3sdhc_read_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{
	const struct pq3sdhc_softc * const sc = (const void *) t;

	KASSERT((o & 3) == 0);

	uint32_t v = bus_space_read_4(sc->sc_bst, h, o & -4);

	if (__predict_false(o == SDHC_DATA))
		v = htole32(v);

	return v;
}

static void
pq3sdhc_write_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, uint8_t nv)
{
	const struct pq3sdhc_softc * const sc = (const void *) t;
	KASSERT((o & -4) != SDHC_DATA);
	uint32_t v = bus_space_read_4(sc->sc_bst, h, o & -4);
	const u_int shift = (o & 3) * 8;

	if (o == SDHC_HOST_CTL) {
		nv &= ~EDSHC_HOST_CTL_RES;
	}

	v &= ~(0xff << shift);
	v |= (nv << shift);

	bus_space_write_4(sc->sc_bst, h, o & -4, v);
}

static void
pq3sdhc_write_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, uint16_t nv)
{
	const struct pq3sdhc_softc * const sc = (const void *) t;
	KASSERT((o & 1) == 0);
	KASSERT((o & -4) != SDHC_DATA);
	const u_int shift = (o & 2) * 8;
	uint32_t v;

	/*
	 * Since NINTR_STATUS and EINTR_STATUS are W1C, don't bother getting
	 * the previous value since we'd clear them.
	 */
	if (__predict_true((o & -4) != SDHC_NINTR_STATUS)) {
		v = bus_space_read_4(sc->sc_bst, h, o & -4);
		v &= ~(0xffff << shift);
		v |= nv << shift;
	} else {
		v = nv << shift;
		printf("put(INTR_STATUS,%#x)\n", v);
	}

	bus_space_write_4(sc->sc_bst, h, o & -4, v);
}

static void
pq3sdhc_write_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, uint32_t v)
{
	const struct pq3sdhc_softc * const sc = (const void *) t;

	KASSERT((o & 3) == 0);

	if (__predict_false(o == SDHC_DATA))
		v = le32toh(v);

	bus_space_write_4(sc->sc_bst, h, o & -4, v);
}

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
	//sc->sc.sc_flags |= SDHC_FLAG_USE_DMA;
	sc->sc.sc_flags |= SDHC_FLAG_HAVE_DVS;
	sc->sc.sc_host = sc->sc_hosts;
	sc->sc.sc_clkbase = board_info_get_number("bus-frequency") / 2000;
	sc->sc_bst = cna->cna_memt;
	sc->sc_mybst = *cna->cna_memt;

	sc->sc_mybst.pbs_scalar.pbss_read_1 = pq3sdhc_read_1;
	sc->sc_mybst.pbs_scalar.pbss_read_2 = pq3sdhc_read_2;
	sc->sc_mybst.pbs_scalar.pbss_read_4 = pq3sdhc_read_4;
	sc->sc_mybst.pbs_scalar.pbss_write_1 = pq3sdhc_write_1;
	sc->sc_mybst.pbs_scalar.pbss_write_2 = pq3sdhc_write_2;
	sc->sc_mybst.pbs_scalar.pbss_write_4 = pq3sdhc_write_4;

	error = bus_space_map(sc->sc_bst, cnl->cnl_addr, cnl->cnl_size, 0,
	    &sc->sc_bsh);
	if (error) {
		aprint_error_dev(self,
		    "can't map registers for %s: %d\n", cnl->cnl_name, error);
		return;
	}

	aprint_naive(": SDHC controller\n");
	aprint_normal(": SDHC controller\n");

	sc->sc_ih = intr_establish(cnl->cnl_intrs[0], IPL_VM, IST_ONCHIP,
	    sdhc_intr, &sc->sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     cnl->cnl_intrs[0]);
		goto fail;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n",
	     cnl->cnl_intrs[0]);

	error = sdhc_host_found(&sc->sc, &sc->sc_mybst, sc->sc_bsh,
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
