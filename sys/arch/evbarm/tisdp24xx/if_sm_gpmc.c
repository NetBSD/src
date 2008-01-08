/* adapted from: */
/*	NetBSD: if_sm_emifs.c,v 1.1.6.1 2007/02/24 19:03:14 snj Exp	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by The NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. The name of the company nor the name of the author may be used to
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
__KERNEL_RCSID(0, "$NetBSD: if_sm_gpmc.c,v 1.1.2.3 2008/01/08 07:16:28 matt Exp $");

#include "locators.h"

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

#include <arch/arm/omap/omap2430gpmcvar.h>
#include <arch/arm/omap/omap_gpio.h>

static int	sm_gpmc_match(struct device *, struct cfdata *, void *);
static void	sm_gpmc_attach(struct device *, struct device *, void *);

struct sm_gpmc_softc {
	struct smc91cxx_softc sc_sm;
	struct evcnt sc_incomplete_ev;
	struct evcnt sc_spurious_ev;
	void *ih;
};

CFATTACH_DECL(sm_gpmc, sizeof(struct sm_gpmc_softc), sm_gpmc_match,
    sm_gpmc_attach, NULL, NULL);

static int
sm_gpmc_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct gpmc_attach_args *gpmc = aux;

	if (gpmc->gpmc_addr == GPMCCF_ADDR_DEFAULT)
		panic("sm must have addr specified in config.");

	if (gpmc->gpmc_intr == GPMCCF_INTR_DEFAULT)
		panic("sm must have intr specified in config.");

	/* Trust the config file. */
	return (1);
}

static int
sm_gpmc_intr(void *arg)
{
	struct sm_gpmc_softc * const gpmcsc = (struct sm_gpmc_softc *)arg;
	struct smc91cxx_softc * const sc = &gpmcsc->sc_sm;

	if (bus_space_read_2(sc->sc_bst, sc->sc_bsh, 0x1a)) {
		int rv = smc91cxx_intr(sc);
		if (bus_space_read_2(sc->sc_bst, sc->sc_bsh, 0x1a))
			gpmcsc->sc_incomplete_ev.ev_count++;
		return rv;
	} else {
		gpmcsc->sc_spurious_ev.ev_count++;
		return 0;
	}
}

static void
sm_gpmc_attach(struct device *parent, struct device *self, void *aux)
{
	struct sm_gpmc_softc *gpmcsc = (struct sm_gpmc_softc *)self;
	struct smc91cxx_softc *sc = &gpmcsc->sc_sm;
	struct gpmc_attach_args *gpmc = aux;
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	uint8_t enaddr[ETHER_ADDR_LEN];
	unsigned int count = 0;

	bst = gpmc->gpmc_iot;

	/* map i/o space */
	if (bus_space_map(bst, gpmc->gpmc_addr, SMC_IOSIZE + 16, 0, &bsh) != 0) {
		aprint_error(": sm_gpmc_attach: can't map i/o space");
		return;
	}

	aprint_normal("\n");

	/* fill in master sc */
	sc->sc_bst = bst;
	sc->sc_bsh = bsh;

	/* register the interrupt handler */
	gpmcsc->ih = intr_establish(gpmc->gpmc_intr, IPL_NET, IST_LEVEL,
	    sm_gpmc_intr, gpmcsc);

	if (gpmcsc->ih == NULL) {
		aprint_error(": couldn't establish interrupt\n");
		bus_space_unmap(bst, bsh, SMC_IOSIZE + 16);
		return;
	}

	evcnt_attach_dynamic(&gpmcsc->sc_spurious_ev, EVCNT_TYPE_INTR, NULL,
	    self->dv_xname, "spurious intr");
	evcnt_attach_dynamic(&gpmcsc->sc_incomplete_ev, EVCNT_TYPE_INTR, NULL,
	    self->dv_xname, "incomplete intr");

	SMC_SELECT_BANK(sc, 1);
	for (count = 0; count < ETHER_ADDR_LEN; count += 2) {
		uint16_t tmp;
		tmp = bus_space_read_2(bst, bsh, IAR_ADDR0_REG_W + count);
		enaddr[count + 1] = (tmp >> 8) & 0xff;
		enaddr[count] = tmp & 0xff;
	}

	/*
	 * Reset the SMC
	 */
	bus_space_write_2(bst, bsh, 0x1c, 1);
	delay(2);
	bus_space_write_2(bst, bsh, 0x1c, 6);

	/*
	 * Wait for the reset to complete.
	 */
	while (bus_space_read_2(bst, bsh, 0x1a) & 1) {
		if (++count > 1000) {
			aprint_error(": didn't come out of reset\n");
			return;
		}
	}

	/* Perform generic attach. */
	sc->sc_flags |= SMC_FLAGS_ENABLED;
	smc91cxx_attach(sc, enaddr);
}
