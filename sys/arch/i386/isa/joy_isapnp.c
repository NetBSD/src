/*	$NetBSD: joy_isapnp.c,v 1.3 1997/08/03 08:16:55 mikel Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/isa/isavar.h>
#include <dev/isapnp/isapnpreg.h>
#include <dev/isapnp/isapnpvar.h>

#include <i386/isa/joyvar.h>

int	joy_isapnp_match __P((struct device *, void *, void *));
void	joy_isapnp_attach __P((struct device *, struct device *, void *));

struct cfattach joy_isapnp_ca = {
	sizeof(struct joy_softc), joy_isapnp_match, joy_isapnp_attach
};

int
joy_isapnp_match(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct isapnp_attach_args *ipa = aux;

	return(strcmp(ipa->ipa_devcompat, "PNPB02F") == 0);
}

void
joy_isapnp_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct joy_softc *sc = (struct joy_softc *)self;
	struct isapnp_attach_args *ipa = aux;
	bus_space_handle_t ioh;

	printf("\n");

	if (isapnp_config(ipa->ipa_iot, ipa->ipa_memt, ipa)) {
		printf("%s: error in region allocation\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	if (bus_space_subregion(ipa->ipa_iot, ipa->ipa_io[0].h, 1, 1,
	    &ioh) < 0) {
		printf("%s: error in region allocation\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_iot = ipa->ipa_iot;
	sc->sc_ioh = ioh;

	printf("%s: %s %s\n", sc->sc_dev.dv_xname, ipa->ipa_devident,
	    ipa->ipa_devclass);

	joyattach(sc);
}
