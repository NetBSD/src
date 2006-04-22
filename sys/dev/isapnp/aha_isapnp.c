/*	$NetBSD: aha_isapnp.c,v 1.9.6.1 2006/04/22 11:39:09 simonb Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: aha_isapnp.c,v 1.9.6.1 2006/04/22 11:39:09 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/isa/isavar.h>

#include <dev/isapnp/isapnpreg.h>
#include <dev/isapnp/isapnpvar.h>
#include <dev/isapnp/isapnpdevs.h>


#include <dev/ic/ahareg.h>
#include <dev/ic/ahavar.h>

int	aha_isapnp_probe(struct device *, struct cfdata *, void *);
void	aha_isapnp_attach(struct device *, struct device *, void *);

CFATTACH_DECL(aha_isapnp, sizeof(struct aha_softc),
    aha_isapnp_probe, aha_isapnp_attach, NULL, NULL);

int
aha_isapnp_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	int pri, variant;

	pri = isapnp_devmatch(aux, &isapnp_aha_devinfo, &variant);
	if (pri && variant > 0)
		pri = 0;
	return (pri);
}

void
aha_isapnp_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct aha_softc *sc = device_private(self);
	struct aha_probe_data apd;
	struct isapnp_attach_args *ipa = aux;

	printf("\n");

	if (isapnp_config(ipa->ipa_iot, ipa->ipa_memt, ipa)) {
		printf("%s: error in region allocation\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_iot = ipa->ipa_iot;
	sc->sc_ioh = ipa->ipa_io[0].h;
	sc->sc_dmat = ipa->ipa_dmat;

	if (!aha_find(sc->sc_iot, sc->sc_ioh, &apd)) {
		printf("%s: aha_find failed\n", sc->sc_dev.dv_xname);
		return;
	}

	if (ipa->ipa_ndrq == 0) {
		if (apd.sc_drq != -1) {
			printf("%s: no PnP drq, but card has one\n",
			    sc->sc_dev.dv_xname);
			return;
		}
	} else if (apd.sc_drq != ipa->ipa_drq[0].num) {
		printf("%s: card drq # (%d) != PnP # (%d)\n",
		    sc->sc_dev.dv_xname, apd.sc_drq, ipa->ipa_drq[0].num);
		return;
	} else {
		int error = isa_dmacascade(ipa->ipa_ic, ipa->ipa_drq[0].num);
		if (error) {
			printf("%s: unable to cascade DRQ, error = %d\n",
			    sc->sc_dev.dv_xname, error);
			return;
		}
	}

	if (apd.sc_irq != ipa->ipa_irq[0].num) {
		printf("%s: card irq # (%d) != PnP # (%d)\n",
		    sc->sc_dev.dv_xname, apd.sc_irq, ipa->ipa_irq[0].num);
		return;
	}


	sc->sc_ih = isa_intr_establish(ipa->ipa_ic, ipa->ipa_irq[0].num,
	    ipa->ipa_irq[0].type, IPL_BIO, aha_intr, sc);

	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	aha_attach(sc, &apd);
}
