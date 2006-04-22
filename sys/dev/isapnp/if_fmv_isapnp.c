/*	$NetBSD: if_fmv_isapnp.c,v 1.5.6.1 2006/04/22 11:39:09 simonb Exp $	*/

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center and Matt Thomas of the 3am Software Foundry.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: if_fmv_isapnp.c,v 1.5.6.1 2006/04/22 11:39:09 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ic/mb86960reg.h>
#include <dev/ic/mb86960var.h>
#include <dev/ic/fmvreg.h>
#include <dev/ic/fmvvar.h>

#include <dev/isa/isavar.h>

#include <dev/isapnp/isapnpreg.h>
#include <dev/isapnp/isapnpvar.h>
#include <dev/isapnp/isapnpdevs.h>

int	fmv_isapnp_match(struct device *, struct cfdata *, void *);
void	fmv_isapnp_attach(struct device *, struct device *, void *);

struct fmv_isapnp_softc {
	struct	mb86960_softc sc_mb86960;	/* real "mb86960" softc */

	/* ISAPnP-specific goo. */
	void	*sc_ih;				/* interrupt cookie */
};

CFATTACH_DECL(fmv_isapnp, sizeof(struct fmv_isapnp_softc),
    fmv_isapnp_match, fmv_isapnp_attach, NULL, NULL);

int
fmv_isapnp_match(struct device *parent, struct cfdata *match, void *aux)
{
	int pri, variant;

	pri = isapnp_devmatch(aux, &isapnp_fmv_devinfo, &variant);
	if (pri && variant > 0)
		pri = 0;
	return pri;
}

void
fmv_isapnp_attach(struct device *parent, struct device *self, void *aux)
{
	struct fmv_isapnp_softc *isc = device_private(self);
	struct mb86960_softc *sc = &isc->sc_mb86960;
	struct isapnp_attach_args * const ipa = aux;

	if (isapnp_config(ipa->ipa_iot, ipa->ipa_memt, ipa)) {
		printf(": can't configure isapnp resources\n");
		return;
	}

	sc->sc_bst = ipa->ipa_iot;
	sc->sc_bsh = ipa->ipa_io[0].h;

	fmv_attach(sc);

	/* Establish the interrupt handler. */
	isc->sc_ih = isa_intr_establish(ipa->ipa_ic, ipa->ipa_irq[0].num,
	    ipa->ipa_irq[0].type, IPL_NET, mb86960_intr, sc);
	if (isc->sc_ih == NULL)
		printf("%s: couldn't establish interrupt handler\n",
		    sc->sc_dev.dv_xname);
}
