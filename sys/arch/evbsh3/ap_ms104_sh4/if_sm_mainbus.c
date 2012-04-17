/*	$NetBSD: if_sm_mainbus.c,v 1.2.2.1 2012/04/17 00:06:19 yamt Exp $	*/

/*-
 * Copyright (C) 2009 NONAKA Kimihiro <nonaka@netbsd.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_sm_mainbus.c,v 1.2.2.1 2012/04/17 00:06:19 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <machine/autoconf.h>
#include <machine/intr.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/ic/smc91cxxreg.h>
#include <dev/ic/smc91cxxvar.h>

#include <evbsh3/ap_ms104_sh4/ap_ms104_sh4reg.h>
#include <evbsh3/ap_ms104_sh4/ap_ms104_sh4var.h>

static int	sm_mainbus_match(device_t, cfdata_t, void *);
static void	sm_mainbus_attach(device_t, device_t, void *);

struct sm_mainbus_softc {
	struct smc91cxx_softc sc_sm;

	void *sc_ih;
};

CFATTACH_DECL(sm_mainbus, sizeof(struct sm_mainbus_softc),
    sm_mainbus_match, sm_mainbus_attach, NULL, NULL);

static int
sm_mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *maa = (struct mainbus_attach_args *)aux;

	if (strcmp(maa->ma_name, "sm") != 0)
		return 0;
	return 1;
}

static void
sm_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct sm_mainbus_softc *smsc = device_private(self);
	struct smc91cxx_softc *sc = &smsc->sc_sm;
	bus_space_tag_t bst = &ap_ms104_sh4_bus_io;
	bus_space_handle_t bsh;

	aprint_naive("\n");
	aprint_normal("\n");

	/* map i/o space */
	if (bus_space_map(bst, 0x08000000, SMC_IOSIZE, 0, &bsh) != 0) {
		aprint_error_dev(self, "can't map i/o space");
		return;
	}

	/* register the interrupt handler */
	smsc->sc_ih = extintr_establish(EXTINTR_INTR_SMC91C111, IST_LEVEL,
	    IPL_NET, smc91cxx_intr, sc);
	if (smsc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		bus_space_unmap(bst, bsh, SMC_IOSIZE);
		return;
	}

	/* fill in master sc */
	sc->sc_bst = bst;
	sc->sc_bsh = bsh;

	sc->sc_flags = SMC_FLAGS_ENABLED;
	smc91cxx_attach(sc, NULL);
}
