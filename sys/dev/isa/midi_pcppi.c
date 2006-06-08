/*	$NetBSD: midi_pcppi.c,v 1.12.14.2 2006/06/08 13:21:48 chap Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (augustss@NetBSD.org).
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: midi_pcppi.c,v 1.12.14.2 2006/06/08 13:21:48 chap Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/select.h>
#include <sys/audioio.h>
#include <sys/midiio.h>

#include <machine/bus.h>

#include <dev/isa/pcppivar.h>

#include <dev/audio_if.h>
#include <dev/midi_if.h>
#include <dev/midivar.h>
#include <dev/midisynvar.h>

#define MAX_DURATION 30		/* turn off sound automagically after 30 s */

struct midi_pcppi_softc {
	struct midi_softc sc_mididev;
	midisyn sc_midisyn;
};

int	midi_pcppi_match(struct device *, struct cfdata *, void *);
void	midi_pcppi_attach(struct device *, struct device *, void *);

void	midi_pcppi_on   (midisyn *, uint_fast16_t, uint32_t, int16_t);
void	midi_pcppi_off  (midisyn *, uint_fast16_t, uint_fast8_t);
void	midi_pcppi_close(midisyn *);

CFATTACH_DECL(midi_pcppi, sizeof(struct midi_pcppi_softc),
    midi_pcppi_match, midi_pcppi_attach, NULL, NULL);

struct midisyn_methods midi_pcppi_hw = {
	.close    = midi_pcppi_close,
	.attackv  = midi_pcppi_on,
	.releasev = midi_pcppi_off,
};

int midi_pcppi_attached = 0;	/* Not very nice */

int
midi_pcppi_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return (!midi_pcppi_attached);
}

void
midi_pcppi_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct midi_pcppi_softc *sc = (struct midi_pcppi_softc *)self;
	struct pcppi_attach_args *pa = (struct pcppi_attach_args *)aux;
	midisyn *ms;

	ms = &sc->sc_midisyn;
	ms->mets = &midi_pcppi_hw;
	strcpy(ms->name, "PC speaker");
	ms->nvoice = 1;
	ms->data = pa->pa_cookie;

	midi_pcppi_attached++;

	midisyn_attach(&sc->sc_mididev, ms);
	midi_attach(&sc->sc_mididev, parent);
}

void
midi_pcppi_on(midisyn *ms,
	      uint_fast16_t voice, uint32_t miditune, int16_t level)
{
	pcppi_tag_t t = ms->data;

	pcppi_bell(t,
	           MIDISYN_HZ18_TO_HZ(midisyn_mt2hz18(miditune)),
	           MAX_DURATION * hz, 0);
}

void
midi_pcppi_off(midisyn *ms, uint_fast16_t voice, uint_fast8_t vel)
{
	pcppi_tag_t t = ms->data;

	/*printf("OFF %p %d\n", t, note >> 16);*/
	pcppi_bell(t, 0, 0, 0);
}

void
midi_pcppi_close(ms)
	midisyn *ms;
{
	pcppi_tag_t t = ms->data;

	/* Make sure we are quiet. */
	pcppi_bell(t, 0, 0, 0);
}
