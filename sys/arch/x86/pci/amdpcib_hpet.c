/* $NetBSD: amdpcib_hpet.c,v 1.1.18.1 2008/01/08 22:10:35 bouyer Exp $ */

/*
 * Copyright (c) 2006 Nicolas Joly
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS
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
__KERNEL_RCSID(0, "$NetBSD: amdpcib_hpet.c,v 1.1.18.1 2008/01/08 22:10:35 bouyer Exp $");

#include <sys/systm.h>
#include <sys/device.h>

#include <sys/time.h>
#include <sys/timetc.h>

#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/ic/hpetvar.h>


static int	amdpcib_hpet_match(struct device *, struct cfdata *, void *);
static void	amdpcib_hpet_attach(struct device *, struct device *, void *);

CFATTACH_DECL(amdpcib_hpet, sizeof(struct hpet_softc), amdpcib_hpet_match,
    amdpcib_hpet_attach, NULL, NULL);

static int
amdpcib_hpet_match(struct device *parent, struct cfdata *match, void *aux)
{
	return 1;
}

static void
amdpcib_hpet_attach(struct device *parent, struct device *self, void *aux)
{
	struct hpet_softc *sc;
	struct pci_attach_args *pa;
	pcireg_t conf, addr;

	sc = (struct hpet_softc *)self;
	pa = (struct pci_attach_args *)aux;

	aprint_naive("\n");
	aprint_normal(": HPET timer\n");

	conf = pci_conf_read(pa->pa_pc, pa->pa_tag, 0xa0);
	if ((conf & 1) == 0) {
		printf("%s: HPET timer is disabled\n", sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_memt = pa->pa_memt;

	addr = conf & 0xfffffc00;
	if (bus_space_map(sc->sc_memt, addr, 1024, 0,
	    &sc->sc_memh)) {
		printf("%s: failed to map mem\n", sc->sc_dev.dv_xname);
		return;
	}

	hpet_attach_subr(sc);
}
