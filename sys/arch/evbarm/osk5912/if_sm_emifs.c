/*	$NetBSD: if_sm_emifs.c,v 1.2 2007/07/24 14:41:29 pooka Exp $	*/

/*
 * OSK5912 SMC91Cxx wrapper, based on sys/arch/evbarm/viper/if_sm_pxaip.c
 *
 * Copyright (c) 2005 Antti Kantee.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_sm_emifs.c,v 1.2 2007/07/24 14:41:29 pooka Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/ic/smc91cxxreg.h>
#include <dev/ic/smc91cxxvar.h>

#include <arch/arm/omap/omap_emifs.h>
#include <arch/arm/omap/omap_gpio.h>

static int	sm_emifs_match(struct device *, struct cfdata *, void *);
static void	sm_emifs_attach(struct device *, struct device *, void *);

struct sm_emifs_softc {
	struct smc91cxx_softc sc_sm;
	void *ih;
};

CFATTACH_DECL(sm_emifs, sizeof(struct sm_emifs_softc), sm_emifs_match,
    sm_emifs_attach, NULL, NULL);

static int
sm_emifs_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct emifs_attach_args *emifs = aux;

	if (emifs->emifs_addr == -1)
		panic("sm must have addr specified in config.");

	/* Trust the config file. */
	return (1);
}

static void
sm_emifs_attach(struct device *parent, struct device *self, void *aux)
{
	struct sm_emifs_softc *emifssc = (struct sm_emifs_softc *)self;
	struct smc91cxx_softc *sc = &emifssc->sc_sm;
	struct emifs_attach_args *emifs = aux;
	bus_space_tag_t bst;
	bus_space_handle_t bsh;

	bst = emifs->emifs_iot;

	/* map i/o space */
	if (bus_space_map(bst, emifs->emifs_addr, SMC_IOSIZE, 0, &bsh) != 0) {
		aprint_error(": sm_emifs_attach: can't map i/o space");
		return;
	}

	aprint_normal("\n");

	/* fill in master sc */
	sc->sc_bst = bst;
	sc->sc_bsh = bsh;

	/* register the interrupt handler */
	emifssc->ih = 
	    omap_gpio_intr_establish(emifs->emifs_intr, IST_EDGE_RISING,
				     IPL_NET, sc->sc_dev.dv_xname,
				     smc91cxx_intr, sc);

	if (emifssc->ih == NULL) {
		aprint_error(": couldn't establish interrupt\n");
		bus_space_unmap(bst, bsh, SMC_IOSIZE);
		return;
	}

	/* Perform generic attach. */
	sc->sc_flags |= SMC_FLAGS_ENABLED;
	smc91cxx_attach(sc, NULL);
}

