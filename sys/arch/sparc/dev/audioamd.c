/*	$NetBSD: audioamd.c,v 1.30 2020/09/12 05:19:16 isaki Exp $	*/
/*	NetBSD: am7930_sparc.c,v 1.44 1999/03/14 22:29:00 jonathan Exp 	*/

/*
 * Copyright (c) 1995 Rolf Grossmann
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Rolf Grossmann.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
__KERNEL_RCSID(0, "$NetBSD: audioamd.c,v 1.30 2020/09/12 05:19:16 isaki Exp $");

#include "audio.h"
#if NAUDIO > 0

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/intr.h>
#include <sys/mutex.h>

#include <machine/autoconf.h>

#include <sys/audioio.h>
#include <dev/audio/audio_if.h>

#include <dev/ic/am7930reg.h>
#include <dev/ic/am7930var.h>

#define AUDIO_ROM_NAME "audio"

struct audioamd_softc {
	struct am7930_softc sc_am7930;	/* glue to MI code */

	bus_space_tag_t sc_bt;		/* bus cookie */
	bus_space_handle_t sc_bh;	/* device registers */
};

int	audioamd_mainbus_match(device_t, cfdata_t, void *);
void	audioamd_mainbus_attach(device_t, device_t, void *);
int	audioamd_obio_match(device_t, cfdata_t, void *);
void	audioamd_obio_attach(device_t, device_t, void *);
int	audioamd_sbus_match(device_t, cfdata_t, void *);
void	audioamd_sbus_attach(device_t, device_t, void *);
void	audioamd_attach(struct audioamd_softc *, int);

CFATTACH_DECL_NEW(audioamd_mainbus, sizeof(struct audioamd_softc),
    audioamd_mainbus_match, audioamd_mainbus_attach, NULL, NULL);

CFATTACH_DECL_NEW(audioamd_obio, sizeof(struct audioamd_softc),
    audioamd_obio_match, audioamd_obio_attach, NULL, NULL);

CFATTACH_DECL_NEW(audioamd_sbus, sizeof(struct audioamd_softc),
    audioamd_sbus_match, audioamd_sbus_attach, NULL, NULL);

/*
 * Define our interface into the am7930 MI driver.
 */

uint8_t	audioamd_codec_dread(struct am7930_softc *, int);
void	audioamd_codec_dwrite(struct am7930_softc *, int, uint8_t);

struct am7930_glue audioamd_glue = {
	audioamd_codec_dread,
	audioamd_codec_dwrite,
};

/*
 * Define our interface to the higher level audio driver.
 */
int	audioamd_getdev(void *, struct audio_device *);

const struct audio_hw_if sa_hw_if = {
	.query_format		= am7930_query_format,
	.set_format		= am7930_set_format,
	.commit_settings	= am7930_commit_settings,
	.trigger_output		= am7930_trigger_output,
	.trigger_input		= am7930_trigger_input,
	.halt_output		= am7930_halt_output,
	.halt_input		= am7930_halt_input,
	.getdev			= audioamd_getdev,
	.set_port		= am7930_set_port,
	.get_port		= am7930_get_port,
	.query_devinfo		= am7930_query_devinfo,
	.get_props		= am7930_get_props,
	.get_locks		= am7930_get_locks,
};

struct audio_device audioamd_device = {
	"am7930",
	"x",
	"audioamd"
};


int
audioamd_mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma;

	ma = aux;
	if (CPU_ISSUN4)
		return 0;
	return strcmp(AUDIO_ROM_NAME, ma->ma_name) == 0;
}

int
audioamd_obio_match(device_t parent, cfdata_t cf, void *aux)
{
	union obio_attach_args *uoba;

	uoba = aux;
	if (uoba->uoba_isobio4 != 0)
		return 0;

	return strcmp("audio", uoba->uoba_sbus.sa_name) == 0;
}

int
audioamd_sbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct sbus_attach_args *sa;

	sa = aux;
	return strcmp(AUDIO_ROM_NAME, sa->sa_name) == 0;
}

void
audioamd_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args *ma;
	struct audioamd_softc *sc;
	bus_space_handle_t bh;

	ma = aux;
	sc = device_private(self);
	sc->sc_am7930.sc_dev = self;
	sc->sc_bt = ma->ma_bustag;

	if (bus_space_map(
			ma->ma_bustag,
			ma->ma_paddr,
			AM7930_DREG_SIZE,
			BUS_SPACE_MAP_LINEAR,
			&bh) != 0) {
		printf("%s: cannot map registers\n", device_xname(self));
		return;
	}
	sc->sc_bh = bh;
	audioamd_attach(sc, ma->ma_pri);
}

void
audioamd_obio_attach(device_t parent, device_t self, void *aux)
{
	union obio_attach_args *uoba;
	struct sbus_attach_args *sa;
	struct audioamd_softc *sc;
	bus_space_handle_t bh;

	uoba = aux;
	sa = &uoba->uoba_sbus;
	sc = device_private(self);
	sc->sc_am7930.sc_dev = self;
	sc->sc_bt = sa->sa_bustag;

	if (sbus_bus_map(sa->sa_bustag,
			 sa->sa_slot, sa->sa_offset,
			 AM7930_DREG_SIZE,
			 0, &bh) != 0) {
		printf("%s: cannot map registers\n", device_xname(self));
		return;
	}
	sc->sc_bh = bh;
	audioamd_attach(sc, sa->sa_pri);
}

void
audioamd_sbus_attach(device_t parent, device_t self, void *aux)
{
	struct sbus_attach_args *sa;
	struct audioamd_softc *sc;
	bus_space_handle_t bh;

	sa = aux;
	sc = device_private(self);
	sc->sc_am7930.sc_dev = self;
	sc->sc_bt = sa->sa_bustag;

	if (sbus_bus_map(sa->sa_bustag,
			 sa->sa_slot, sa->sa_offset,
			 AM7930_DREG_SIZE,
			 0, &bh) != 0) {
		printf("%s: cannot map registers\n", device_xname(self));
		return;
	}
	sc->sc_bh = bh;
	audioamd_attach(sc, sa->sa_pri);
}

void
audioamd_attach(struct audioamd_softc *sc, int pri)
{
	struct am7930_softc *amsc = &sc->sc_am7930;
	device_t self;

	/*
	 * Set up glue for MI code early; we use some of it here.
	 */
	amsc->sc_glue = &audioamd_glue;
	am7930_init(amsc, AUDIOAMD_POLL_MODE);

	(void)bus_intr_establish2(sc->sc_bt, pri, IPL_HIGH,
				  am7930_hwintr, sc, NULL);

	printf("\n");

	self = amsc->sc_dev;
	evcnt_attach_dynamic(&amsc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
	    device_xname(self), "intr");

	audio_attach_mi(&sa_hw_if, sc, self);
}


/* direct read */
uint8_t
audioamd_codec_dread(struct am7930_softc *amsc, int reg)
{
	struct audioamd_softc *sc = (struct audioamd_softc *)amsc;

	return bus_space_read_1(sc->sc_bt, sc->sc_bh, reg);
}

/* direct write */
void
audioamd_codec_dwrite(struct am7930_softc *amsc, int reg, uint8_t val)
{
	struct audioamd_softc *sc = (struct audioamd_softc *)amsc;

	bus_space_write_1(sc->sc_bt, sc->sc_bh, reg, val);
}

int
audioamd_getdev(void *addr, struct audio_device *retp)
{

	*retp = audioamd_device;
	return 0;
}

#endif /* NAUDIO > 0 */
