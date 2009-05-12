/*	$NetBSD: obio.c,v 1.12 2009/05/12 14:30:25 cegger Exp $	*/

/*
 * Copyright (c) 2002 Allegro Networks, Inc., Wasabi Systems, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Allegro Networks, Inc., and Wasabi Systems, Inc.
 * 4. The name of Allegro Networks, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 * 5. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ALLEGRO NETWORKS, INC. AND
 * WASABI SYSTEMS, INC. ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL EITHER ALLEGRO NETWORKS, INC. OR WASABI SYSTEMS, INC.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * gt.c -- GT system controller driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: obio.c,v 1.12 2009/05/12 14:30:25 cegger Exp $");

#include "opt_marvell.h"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/extent.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#define _BUS_SPACE_PRIVATE
#define _BUS_DMA_PRIVATE
#include <sys/bus.h>

#include <powerpc/spr.h>
#include <powerpc/oea/hid.h>

#include <dev/pci/pcivar.h>
#include <dev/marvell/gtreg.h>
#include <dev/marvell/gtvar.h>

#ifdef DEBUG
#include <sys/systm.h>	/* for Debugger() */
#endif

#include "locators.h"

static int obio_cfprint(void *, const char *);
static int obio_cfmatch(device_t, cfdata_t, void *);
static int obio_cfsearch(device_t, cfdata_t,
			 const int *, void *);
static void obio_cfattach(device_t, device_t, void *);

struct obio_softc {
	struct device sc_dev;
	bus_space_tag_t sc_memt;
	bus_space_tag_t sc_gt_memt;
	bus_space_tag_t sc_gt_memh;
};

CFATTACH_DECL(obio, sizeof(struct obio_softc),
    obio_cfmatch, obio_cfattach, NULL, NULL);

extern struct cfdriver obio_cd;

static const struct {
    bus_addr_t low_decode;
    bus_addr_t high_decode;
} obio_info[5] = {
    { GT_CS0_Low_Decode, GT_CS0_High_Decode, },
    { GT_CS1_Low_Decode, GT_CS1_High_Decode, },
    { GT_CS2_Low_Decode, GT_CS2_High_Decode, },
    { GT_CS3_Low_Decode, GT_CS3_High_Decode, },
    { GT_BootCS_Low_Decode, GT_BootCS_High_Decode, },
};

extern bus_space_tag_t obio_bs_tags[5];

int
obio_cfprint(void *aux, const char *pnp)
{
	struct obio_attach_args *oa = aux;

	if (pnp) {
		aprint_normal("%s at %s", oa->oa_name, pnp);
	}
	aprint_normal(" offset %#x size %#x", oa->oa_offset, oa->oa_size);
	if (oa->oa_irq != OBIOCF_IRQ_DEFAULT)
		aprint_normal(" irq %d", oa->oa_irq);

	return (UNCONF);
}


int
obio_cfsearch(device_t parent, cfdata_t cf,
	      const int *ldesc, void *aux)
{
	struct obio_softc *sc = (struct obio_softc *) parent;
	struct obio_attach_args oa;

	oa.oa_name = cf->cf_name;
	oa.oa_memt = sc->sc_memt;
	oa.oa_offset = cf->cf_loc[OBIOCF_OFFSET];
	oa.oa_size = cf->cf_loc[OBIOCF_SIZE];
	oa.oa_irq = cf->cf_loc[OBIOCF_IRQ];

	if (config_match(parent, cf, &oa) > 0)
		config_attach(parent, cf, &oa, obio_cfprint);

	return (0);
}

int
obio_cfmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct gt_softc * const gt = (struct gt_softc *)parent;
	struct gt_attach_args * const ga = aux;

	return GT_OBIOOK(gt, ga, &obio_cd);
}

void
obio_cfattach(device_t parent, device_t self, void *aux)
{
	struct gt_softc * const gt = device_private(parent);
	struct obio_softc *sc = device_private(self);
	struct gt_attach_args *ga = aux;
	uint32_t datal, datah;

	GT_OBIOFOUND(gt, ga);

	datal = bus_space_read_4(ga->ga_memt, ga->ga_memh,
	    obio_info[ga->ga_unit].low_decode);
	datah = bus_space_read_4(ga->ga_memt, ga->ga_memh,
	    obio_info[ga->ga_unit].high_decode);

	if (GT_LowAddr_GET(datal) > GT_HighAddr_GET(datah)) {
		aprint_normal(": disabled\n");
		return;
	}

	sc->sc_memt = obio_bs_tags[ga->ga_unit];
	if (sc->sc_memt == NULL) {
		aprint_normal(": unused\n");
		return;
	}

	aprint_normal(": addr %#x-%#x, %s-endian\n",
	    GT_LowAddr_GET(datal), GT_HighAddr_GET(datah),
	    GT_PCISwap_GET(datal) == 1 ? "little" : "big");

        config_search_ia(obio_cfsearch, &sc->sc_dev, "obio", NULL);
}
