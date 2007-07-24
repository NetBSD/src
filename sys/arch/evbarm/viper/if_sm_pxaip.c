/*	$NetBSD: if_sm_pxaip.c,v 1.3 2007/07/24 14:41:29 pooka Exp $	*/

/*
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
__KERNEL_RCSID(0, "$NetBSD: if_sm_pxaip.c,v 1.3 2007/07/24 14:41:29 pooka Exp $");

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

#include <arch/arm/xscale/pxa2x0var.h>
#include <arch/arm/xscale/pxa2x0_gpio.h>

#include <arch/evbarm/viper/viper_reg.h>

static int	sm_pxaip_match(struct device *, struct cfdata *, void *);
static void	sm_pxaip_attach(struct device *, struct device *, void *);

struct sm_pxaip_softc {
	struct smc91cxx_softc sc_sm;
	void *ih;
};

CFATTACH_DECL(sm_pxaip, sizeof(struct sm_pxaip_softc), sm_pxaip_match,
    sm_pxaip_attach, NULL, NULL);

static int
sm_pxaip_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pxaip_attach_args *paa = aux;

	if (paa->pxa_addr == VIPER_SMSC_PBASE)
		return 1;
	return 0;
}

static void
sm_pxaip_attach(struct device *parent, struct device *self, void *aux)
{
	struct sm_pxaip_softc *pxasc = (struct sm_pxaip_softc *)self;
	struct smc91cxx_softc *sc = &pxasc->sc_sm;
	struct pxaip_attach_args *paa = aux;
	bus_space_tag_t bst = &pxa2x0_bs_tag;
	bus_space_handle_t bsh;

	/* map i/o space */
	if (bus_space_map(bst, paa->pxa_addr, SMC_IOSIZE, 0, &bsh) != 0) {
		aprint_error(": sm_pxaip_attach: can't map i/o space");
		return;
	}

	/* register the interrupt handler */
	pxasc->ih = pxa2x0_gpio_intr_establish(paa->pxa_intr, IST_EDGE_RISING,
	    IPL_NET, smc91cxx_intr, sc);
	if (pxasc->ih == NULL) {
		aprint_error(": couldn't establish interrupt\n");
		bus_space_unmap(bst, bsh, SMC_IOSIZE);
		return;
	}

	printf("\n");

	/* fill in master sc */
	sc->sc_bst = bst;
	sc->sc_bsh = bsh;

	sc->sc_flags = SMC_FLAGS_ENABLED;
	smc91cxx_attach(sc, NULL);
}
