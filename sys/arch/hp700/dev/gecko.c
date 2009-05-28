/*	$OpenBSD: gecko.c,v 1.1 2008/04/27 14:39:51 kettenis Exp $	*/

/*
 * Copyright (c) 2007 Mark Kettenis
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/iomod.h>
#include <machine/pdc.h>

#include <hp700/dev/cpudevs.h>

struct gecko_softc {
	device_t		sc_dv;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t 	sc_ioh;
};

int	gecko_match(device_t, cfdata_t, void *);
void	gecko_attach(device_t, device_t, void *);
static void gecko_callback(device_t, struct confargs *);

CFATTACH_DECL_NEW(gecko, sizeof(struct gecko_softc), gecko_match,
    gecko_attach, NULL, NULL);

int
gecko_match(device_t parent, cfdata_t match, void *aux)
{
	struct confargs *ca = aux;

	if (ca->ca_type.iodc_type != HPPA_TYPE_BCPORT ||
	    ca->ca_type.iodc_sv_model != HPPA_BCPORT_PORT)
		return (0);

	if (ca->ca_type.iodc_model == 0x50 &&
	    ca->ca_type.iodc_revision == 0x00)
		return (1);

	return (0);
}

void
gecko_attach(device_t parent, device_t self, void *aux)
{
	struct gecko_softc *sc = device_private(self);
	struct confargs *ca = aux, nca;
	volatile struct iomod *regs;

	sc->sc_dv = self;
	sc->sc_iot = ca->ca_iot;
	if (bus_space_map(sc->sc_iot, ca->ca_hpa, IOMOD_HPASIZE, 0,
	    &sc->sc_ioh)) {
		aprint_error(": can't map IO space\n");
		return;
	}
	regs = bus_space_vaddr(ca->ca_iot, sc->sc_ioh);

#if 1
	printf(": %x-%x", regs->io_io_low, regs->io_io_high);
#endif

	printf("\n");

	nca = *ca;
	nca.ca_hpabase = 0;
	nca.ca_nmodules = MAXMODBUS;

	pdc_scanbus(self, &nca, gecko_callback /*regs->io_io_low */);
}

static void
gecko_callback(device_t self, struct confargs *ca)
{

	config_found_sm_loc(self, "gedoens", NULL, ca, mbprint, mbsubmatch);
}

