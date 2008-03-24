/* $NetBSD: amdpcib_hpet.c,v 1.1.2.4 2008/03/24 09:38:40 yamt Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: amdpcib_hpet.c,v 1.1.2.4 2008/03/24 09:38:40 yamt Exp $");

#include <sys/systm.h>
#include <sys/device.h>

#include <sys/time.h>
#include <sys/timetc.h>

#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/ic/hpetvar.h>


static int	amdpcib_hpet_match(device_t , cfdata_t , void *);
static void	amdpcib_hpet_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(amdpcib_hpet, sizeof(struct hpet_softc), amdpcib_hpet_match,
    amdpcib_hpet_attach, NULL, NULL);

static int
amdpcib_hpet_match(device_t parent, cfdata_t match, void *aux)
{
	return 1;
}

static void
amdpcib_hpet_attach(device_t parent, device_t self, void *aux)
{
	struct hpet_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	pcireg_t conf, addr;

	aprint_naive("\n");
	aprint_normal(": HPET timer\n");

	conf = pci_conf_read(pa->pa_pc, pa->pa_tag, 0xa0);
	if ((conf & 1) == 0) {
		aprint_normal_dev(self, "HPET timer is disabled\n");
		return;
	}

	sc->sc_memt = pa->pa_memt;

	addr = conf & 0xfffffc00;
	if (bus_space_map(sc->sc_memt, addr, 1024, 0,
	    &sc->sc_memh)) {
		aprint_error_dev(self, "failed to map mem\n");
		return;
	}

	hpet_attach_subr(self);
}
