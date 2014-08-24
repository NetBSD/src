/*	$NetBSD: arcofi_dio.c,v 1.1 2014/08/24 08:17:44 tsutsui Exp $	*/
/*	$OpenBSD: arcofi_dio.c,v 1.1 2011/12/21 23:12:03 miod Exp $	*/

/*
 * Copyright (c) 2011 Miodrag Vallat.
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
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/intr.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/ic/arcofivar.h>

#include <hp300/dev/dioreg.h>
#include <hp300/dev/diovar.h>

#include <hp300/dev/diodevs.h>

#define SOFTINT_AUDIO	SOFTINT_SERIAL	/* XXX */

static void	arcofi_dio_attach(device_t, device_t, void *);
static int	arcofi_dio_match(device_t, cfdata_t, void *);

struct arcofi_dio_softc {
	struct arcofi_softc	sc_arcofi;

	struct bus_space_tag sc_tag;
};

CFATTACH_DECL_NEW(arcofi_dio, sizeof(struct arcofi_dio_softc),
    arcofi_dio_match, arcofi_dio_attach, NULL, NULL);

static int
arcofi_dio_match(device_t parent, cfdata_t match, void *aux)
{
	struct dio_attach_args *da = aux;

	if (da->da_id != DIO_DEVICE_ID_AUDIO)
		return 0;

	return 1;
}

static void
arcofi_dio_attach(device_t parent, device_t self, void *aux)
{
	struct arcofi_dio_softc *adsc = device_private(self);
	struct arcofi_softc *sc = &adsc->sc_arcofi;
	struct dio_attach_args *da = aux;
	bus_space_tag_t iot = &adsc->sc_tag;
	int ipl;

	sc->sc_dev = self;

	/* XXX is it better to use sc->sc_reg[] to handle odd sparse map? */
	memcpy(iot, da->da_bst, sizeof(struct bus_space_tag));
	dio_set_bus_space_oddbyte(iot);
	sc->sc_iot = iot;

	if (bus_space_map(iot, da->da_addr, DIOII_SIZEOFF, 0,
	    &sc->sc_ioh) != 0) {
		aprint_error(": can't map registers\n");
		return;
	}

	sc->sc_sih = softint_establish(SOFTINT_AUDIO, arcofi_swintr, sc);
	if (sc->sc_sih == NULL) {
		aprint_error(": can't register soft interrupt\n");
		return;
	}
	ipl = da->da_ipl;
	dio_intr_establish(arcofi_hwintr, sc, ipl, IPL_AUDIO);

	aprint_normal("\n");

	arcofi_attach(sc, "dio");
}
