/*	$NetBSD: amdpm.c,v 1.10 2006/01/28 23:31:24 kleink Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Enami Tsugutomo.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: amdpm.c,v 1.10 2006/01/28 23:31:24 kleink Exp $");

#include "opt_amdpm.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/callout.h>
#include <sys/rnd.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/amdpmreg.h>

struct amdpm_softc {
	struct device sc_dev;

	pci_chipset_tag_t sc_pc;
	pcitag_t sc_tag;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;		/* PMxx space */

	struct callout sc_rnd_ch;
	rndsource_element_t sc_rnd_source;
#ifdef AMDPM_RND_COUNTERS
	struct evcnt sc_rnd_hits;
	struct evcnt sc_rnd_miss;
	struct evcnt sc_rnd_data[256];
#endif
};

static void	amdpm_rnd_callout(void *);

#ifdef AMDPM_RND_COUNTERS
#define	AMDPM_RNDCNT_INCR(ev)	(ev)->ev_count++
#else
#define	AMDPM_RNDCNT_INCR(ev)	/* nothing */
#endif

static int
amdpm_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_AMD)
		return (0);

	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_AMD_PBC768_PMC:
	case PCI_PRODUCT_AMD_PBC8111_ACPI:
		return (1);
	}

	return (0);
}

static void
amdpm_attach(struct device *parent, struct device *self, void *aux)
{
	struct amdpm_softc *sc = (struct amdpm_softc *) self;
	struct pci_attach_args *pa = aux;
	char devinfo[256];
	pcireg_t reg;
	u_int32_t pmreg;
	int i;

	aprint_naive("\n");
	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_normal(": %s (rev. 0x%02x)\n", devinfo,
	    PCI_REVISION(pa->pa_class));

	sc->sc_pc = pa->pa_pc;
	sc->sc_tag = pa->pa_tag;
	sc->sc_iot = pa->pa_iot;

#if 0
	aprint_normal("%s: ", sc->sc_dev.dv_xname);
	pci_conf_print(pa->pa_pc, pa->pa_tag, NULL);
#endif

	reg = pci_conf_read(pa->pa_pc, pa->pa_tag, AMDPM_CONFREG);
	if ((reg & AMDPM_PMIOEN) == 0) {
		aprint_error("%s: PMxx space isn't enabled\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	reg = pci_conf_read(pa->pa_pc, pa->pa_tag, AMDPM_PMPTR);
	if (bus_space_map(sc->sc_iot, AMDPM_PMBASE(reg), AMDPM_PMSIZE,
	    0, &sc->sc_ioh)) {
		aprint_error("%s: failed to map PMxx space\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	reg = pci_conf_read(pa->pa_pc, pa->pa_tag, AMDPM_CONFREG);
	pci_conf_write(pa->pa_pc, pa->pa_tag, AMDPM_CONFREG, reg | AMDPM_RNGEN);
	reg = pci_conf_read(pa->pa_pc, pa->pa_tag, AMDPM_CONFREG);
	if (reg & AMDPM_RNGEN) {
		/* Check to see if we can read data from the RNG. */
		(void) bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		    AMDPM_RNGDATA);
		for (i = 0; i < 1000; i++) {
			pmreg = bus_space_read_4(sc->sc_iot,
			    sc->sc_ioh, AMDPM_RNGSTAT);
			if (pmreg & AMDPM_RNGDONE)
				break;
			delay(1);
		}
		if ((pmreg & AMDPM_RNGDONE) != 0) {
			aprint_normal("%s: "
			    "random number generator enabled (apprx. %dms)\n",
			    sc->sc_dev.dv_xname, i);
			callout_init(&sc->sc_rnd_ch);
			rnd_attach_source(&sc->sc_rnd_source,
			    sc->sc_dev.dv_xname, RND_TYPE_RNG,
			    /*
			     * XXX Careful!  The use of RND_FLAG_NO_ESTIMATE
			     * XXX here is unobvious: we later feed raw bits
			     * XXX into the "entropy pool" with rnd_add_data,
			     * XXX explicitly supplying an entropy estimate.
			     * XXX In this context, NO_ESTIMATE serves only
			     * XXX to prevent rnd_add_data from trying to
			     * XXX use the *time at which we added the data*
			     * XXX as entropy, which is not a good idea since
			     * XXX we add data periodically from a callout.
			     */
			    RND_FLAG_NO_ESTIMATE);
#ifdef AMDPM_RND_COUNTERS
			evcnt_attach_dynamic(&sc->sc_rnd_hits, EVCNT_TYPE_MISC,
			    NULL, sc->sc_dev.dv_xname, "rnd hits");
			evcnt_attach_dynamic(&sc->sc_rnd_miss, EVCNT_TYPE_MISC,
			    NULL, sc->sc_dev.dv_xname, "rnd miss");
			for (i = 0; i < 256; i++) {
				evcnt_attach_dynamic(&sc->sc_rnd_data[i],
				    EVCNT_TYPE_MISC, NULL, sc->sc_dev.dv_xname,
				    "rnd data");
			}
#endif
			amdpm_rnd_callout(sc);
		}
	}
}

CFATTACH_DECL(amdpm, sizeof(struct amdpm_softc),
    amdpm_match, amdpm_attach, NULL, NULL);

static void
amdpm_rnd_callout(void *v)
{
	struct amdpm_softc *sc = v;
	u_int32_t reg;
#ifdef AMDPM_RND_COUNTERS
	int i;
#endif

	if ((bus_space_read_4(sc->sc_iot, sc->sc_ioh, AMDPM_RNGSTAT) &
	    AMDPM_RNGDONE) != 0) {
		reg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, AMDPM_RNGDATA);
		rnd_add_data(&sc->sc_rnd_source, &reg,
		    sizeof(reg), sizeof(reg) * NBBY);
#ifdef AMDPM_RND_COUNTERS
		AMDPM_RNDCNT_INCR(&sc->sc_rnd_hits);
		for (i = 0; i < sizeof(reg); i++, reg >>= NBBY)
			AMDPM_RNDCNT_INCR(&sc->sc_rnd_data[reg & 0xff]);
#endif
	} else
		AMDPM_RNDCNT_INCR(&sc->sc_rnd_miss);
	callout_reset(&sc->sc_rnd_ch, 1, amdpm_rnd_callout, sc);
}
