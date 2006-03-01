/* $NetBSD: aupsc.c,v 1.1.2.2 2006/03/01 09:27:59 yamt Exp $ */

/*-
 * Copyright (c) 2006 Shigeyuki Fukushima.
 * All rights reserved.
 *
 * Written by Shigeyuki Fukushima.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: aupsc.c,v 1.1.2.2 2006/03/01 09:27:59 yamt Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <mips/alchemy/include/aubusvar.h>
#include <mips/alchemy/include/aureg.h>
#include <mips/alchemy/dev/aupscreg.h>
#include <mips/alchemy/dev/aupscvar.h>

struct aupsc_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_bust;
	bus_space_handle_t	sc_bush;
	int			sc_pscsel;
};

const struct aupsc_proto {
	const char *name;
} aupsc_protos [] = {
#if 0
	{ "auaudio" },
	{ "aui2s" },
	{ "ausmbus" },
	{ "auspi" },
#endif
	{ NULL }
};

static int	aupsc_match(struct device *, struct cfdata *, void *);
static void	aupsc_attach(struct device *, struct device *, void *);
static int	aupsc_submatch(struct device *parent, struct cfdata *cf,
				const int *ldesc, void *aux);
static int	aupsc_print(void *aux, const char *pnp);

CFATTACH_DECL(aupsc, sizeof(struct aupsc_softc),
	aupsc_match, aupsc_attach, NULL, NULL);

static int
aupsc_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct aubus_attach_args *aa = (struct aubus_attach_args *)aux;

	if (strcmp(aa->aa_name, cf->cf_name) != 0)
		return 0;

	return 1;
}

static void
aupsc_attach(struct device *parent, struct device *self, void *aux)
{
	int i;
	uint32_t rv;
	struct aupsc_softc *sc = (struct aupsc_softc *)self;
	struct aubus_attach_args *aa = (struct aubus_attach_args *)aux;
	struct aupsc_attach_args pa;

	sc->sc_bust = aa->aa_st;
	if (bus_space_map(sc->sc_bust, aa->aa_addr,
			AUPSC_SIZE, 0, &sc->sc_bush) != 0) {
		aprint_normal(": unable to map device registers\n");
		return;
	}

	/* Initialize PSC_SEL register */
	sc->sc_pscsel = AUPSC_SEL_DISABLE;
	rv = bus_space_read_4(sc->sc_bust, sc->sc_bush, AUPSC_SEL);
	bus_space_write_4(sc->sc_bust, sc->sc_bush,
		AUPSC_SEL, (rv & AUPSC_SEL_PS(AUPSC_SEL_DISABLE)));

	aprint_normal(": Alchemy PSC\n");

	for (i = 0 ; aupsc_protos[i].name != NULL ; i++) {
		pa.aupsc_name = aupsc_protos[i].name;
		pa.aupsc_bust = sc->sc_bust;
		pa.aupsc_bush = sc->sc_bush;

		(void) config_found_sm_loc(self, "aupsc", NULL,
			&pa, aupsc_print, aupsc_submatch);
        }
}

static int
aupsc_submatch(struct device *parent, struct cfdata *cf,
	const int *ldesc, void *aux)
{

	return config_match(parent, cf, aux);
}

static int
aupsc_print(void *aux, const char *pnp)
{
	struct aupsc_attach_args *pa = aux;

	if (pnp)
		aprint_normal("%s at %s", pa->aupsc_name, pnp);

	return UNCONF;
}
