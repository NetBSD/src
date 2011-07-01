/* $NetBSD: amdpcib_hpet.c,v 1.8 2011/07/01 18:22:08 dyoung Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: amdpcib_hpet.c,v 1.8 2011/07/01 18:22:08 dyoung Exp $");

#include <sys/systm.h>
#include <sys/device.h>

#include <sys/time.h>
#include <sys/timetc.h>

#include <sys/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/ic/hpetreg.h>
#include <dev/ic/hpetvar.h>

static int	amdpcib_hpet_match(device_t , cfdata_t , void *);
static void	amdpcib_hpet_attach(device_t, device_t, void *);
static int	amdpcib_hpet_detach(device_t, int);
static pcireg_t amdpcib_hpet_addr(struct pci_attach_args *);

CFATTACH_DECL_NEW(amdpcib_hpet, sizeof(struct hpet_softc),
    amdpcib_hpet_match, amdpcib_hpet_attach, amdpcib_hpet_detach, NULL);

static int
amdpcib_hpet_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;
	bus_space_handle_t bh;
	bus_space_tag_t bt;
	pcireg_t addr;

	addr = amdpcib_hpet_addr(pa);

	if (addr == 0)
		return 0;

	bt = pa->pa_memt;

	if (bus_space_map(bt, addr, HPET_WINDOW_SIZE, 0, &bh) != 0)
		return 0;

	bus_space_unmap(bt, bh, HPET_WINDOW_SIZE);

	return 1;
}

static void
amdpcib_hpet_attach(device_t parent, device_t self, void *aux)
{
	struct hpet_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	pcireg_t addr;

	sc->sc_mapped = false;
	addr = amdpcib_hpet_addr(pa);

	if (addr == 0) {
		aprint_error(": failed to get address\n");
		return;
	}

	sc->sc_memt = pa->pa_memt;
	sc->sc_mems = HPET_WINDOW_SIZE;

	if (bus_space_map(sc->sc_memt, addr, sc->sc_mems, 0, &sc->sc_memh)) {
		aprint_error(": failed to map mem space\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": high precision event timer (mem 0x%08x-0x%08x)\n",
	    addr, addr + HPET_WINDOW_SIZE);

	sc->sc_mapped = true;
	hpet_attach_subr(self);
}

static int
amdpcib_hpet_detach(device_t self, int flags)
{
	struct hpet_softc *sc = device_private(self);
	int rv;

	if (sc->sc_mapped != true)
		return 0;

	rv = hpet_detach(self, flags);

	if (rv != 0)
		return rv;

	bus_space_unmap(sc->sc_memt, sc->sc_memh, sc->sc_mems);

	return 0;
}

static pcireg_t
amdpcib_hpet_addr(struct pci_attach_args *pa)
{
	pcireg_t conf;

	conf = pci_conf_read(pa->pa_pc, pa->pa_tag, 0xa0);

	if ((conf & 1) == 0)
		return 0;

	return conf & 0xfffffc00;
}
