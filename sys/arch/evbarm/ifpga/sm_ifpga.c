/*	$NetBSD: sm_ifpga.c,v 1.2 2013/02/23 08:23:03 skrll Exp $	*/

/*-
 * Copyright (c) 2013 Sergio Lopez <slp@sinrega.org>
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
__KERNEL_RCSID(0, "$NetBSD: sm_ifpga.c,v 1.2 2013/02/23 08:23:03 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <machine/intr.h>
#include <sys/bus.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/ic/smc91cxxreg.h>
#include <dev/ic/smc91cxxvar.h>

#include <evbarm/ifpga/ifpgareg.h>
#include <evbarm/ifpga/ifpgavar.h>

static int	sm_ifpga_match(device_t, cfdata_t, void *);
static void	sm_ifpga_attach(device_t, device_t, void *);

struct sm_ifpga_softc {
	struct smc91cxx_softc sc_sm;
	void *ih;
};

CFATTACH_DECL_NEW(sm_ifpga, sizeof(struct sm_ifpga_softc), sm_ifpga_match,
    sm_ifpga_attach, NULL, NULL);

static int
sm_ifpga_match(device_t parent, cfdata_t match, void *aux)
{
	struct ifpga_attach_args *ifa = aux;

	if (ifa->ifa_addr == IFPGA_SMC911_BASE)
		return 1;
	return 0;
}

static void
sm_ifpga_attach(device_t parent, device_t self, void *aux)
{
	struct sm_ifpga_softc *isc = device_private(self);
	struct smc91cxx_softc *sc = &isc->sc_sm;
	struct ifpga_attach_args *ifa = aux;
	bus_space_tag_t bst = ifa->ifa_iot;
	bus_space_handle_t bsh;

	/* map i/o space */
	if (bus_space_map(bst, ifa->ifa_addr, SMC_IOSIZE, 0, &bsh) != 0) {
		aprint_error(": sm_ifpga_attach: can't map i/o space");
		return;
	}

 	isc->ih = ifpga_intr_establish(ifa->ifa_irq, IPL_NET, smc91cxx_intr, sc);
	if (isc->ih == NULL) {
		aprint_error(": couldn't establish interrupt\n");
		bus_space_unmap(bst, bsh, SMC_IOSIZE);
		return;
	}

	aprint_normal("\n");

	/* fill in master sc */
	sc->sc_dev = self;
	sc->sc_bst = bst;
	sc->sc_bsh = bsh;

	sc->sc_flags = SMC_FLAGS_ENABLED;
	smc91cxx_attach(sc, NULL);
}

