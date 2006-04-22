/* $NetBSD: aupsc.c,v 1.4.6.2 2006/04/22 11:37:41 simonb Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: aupsc.c,v 1.4.6.2 2006/04/22 11:37:41 simonb Exp $");

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
#include <mips/alchemy/dev/ausmbus_pscreg.h>

struct aupsc_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_bust;
	bus_space_handle_t	sc_bush;
	int			sc_pscsel;
};

const struct aupsc_proto {
	const char *name;
	int protocol;
	int statreg;
	int statbit;
} aupsc_protos [] = {
	{ "ausmbus", AUPSC_SEL_SMBUS, AUPSC_SMBSTAT, SMBUS_STAT_SR },
#if 0
	{ "auaudio" },
	{ "aui2s" },
	{ "auspi" },
#endif
	{ NULL, AUPSC_SEL_DISABLE, 0, 0 }
};

static int	aupsc_match(struct device *, struct cfdata *, void *);
static void	aupsc_attach(struct device *, struct device *, void *);
static int	aupsc_submatch(struct device *, struct cfdata *, const int *,
				void *);
static int	aupsc_print(void *, const char *);

static void	aupsc_enable(void *, int);
static void	aupsc_disable(void *);
static void	aupsc_suspend(void *);


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
	struct aupsc_controller ctrl;

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
	bus_space_write_4(sc->sc_bust, sc->sc_bush,
		AUPSC_CTRL, AUPSC_CTRL_ENA(AUPSC_CTRL_DISABLE));

	aprint_normal(": Alchemy PSC\n");

	ctrl.psc_bust = sc->sc_bust;
	ctrl.psc_bush = sc->sc_bush;
	ctrl.psc_sel = &(sc->sc_pscsel);
	ctrl.psc_enable = aupsc_enable;
	ctrl.psc_disable = aupsc_disable;
	ctrl.psc_suspend = aupsc_suspend;
	pa.aupsc_ctrl = ctrl;

	for (i = 0 ; aupsc_protos[i].name != NULL ; i++) {
		struct aupsc_protocol_device p;
		uint32_t s;

		pa.aupsc_name = aupsc_protos[i].name;

		p.sc_dev = sc->sc_dev;
		p.sc_ctrl = ctrl;

		aupsc_enable(&p, aupsc_protos[i].protocol);
		s = bus_space_read_4(sc->sc_bust, sc->sc_bush,
			aupsc_protos[i].statreg);
		aupsc_disable(&p);

		if (s & aupsc_protos[i].statbit) {
			(void) config_found_sm_loc(self, "aupsc", NULL,
				&pa, aupsc_print, aupsc_submatch);
		}
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

static void
aupsc_enable(void *arg, int proto)
{
	struct aupsc_protocol_device *sc = arg;

	/* XXX: (TODO) setting clock AUPSC_SEL_CLK */
	switch (proto) {
	case AUPSC_SEL_SPI:
	case AUPSC_SEL_I2S:
	case AUPSC_SEL_AC97:
	case AUPSC_SEL_SMBUS:
		break;
	case AUPSC_SEL_DISABLE:
		aupsc_disable(arg);
		break;
	default:
		printf("%s: aupsc_enable: unsupported protocol.\n",
			sc->sc_dev.dv_xname);
		return;
	}

	if (*(sc->sc_ctrl.psc_sel) != AUPSC_SEL_DISABLE) {
		printf("%s: aupsc_enable: please disable first.\n",
			sc->sc_dev.dv_xname);
		return;
	}

	bus_space_write_4(sc->sc_ctrl.psc_bust, sc->sc_ctrl.psc_bush,
			AUPSC_SEL, AUPSC_SEL_PS(proto));
	bus_space_write_4(sc->sc_ctrl.psc_bust, sc->sc_ctrl.psc_bush,
			AUPSC_CTRL, AUPSC_CTRL_ENA(AUPSC_CTRL_ENABLE));
	delay(1);
}

static void
aupsc_disable(void *arg)
{
	struct aupsc_protocol_device *sc = arg;

	bus_space_write_4(sc->sc_ctrl.psc_bust, sc->sc_ctrl.psc_bush,
			AUPSC_SEL, AUPSC_SEL_PS(AUPSC_SEL_DISABLE));
	bus_space_write_4(sc->sc_ctrl.psc_bust, sc->sc_ctrl.psc_bush,
			AUPSC_CTRL, AUPSC_CTRL_ENA(AUPSC_CTRL_DISABLE));
	delay(1);
}

static void
aupsc_suspend(void *arg)
{
	struct aupsc_protocol_device *sc = arg;

	bus_space_write_4(sc->sc_ctrl.psc_bust, sc->sc_ctrl.psc_bush,
			AUPSC_CTRL, AUPSC_CTRL_ENA(AUPSC_CTRL_SUSPEND));
	delay(1);
}
