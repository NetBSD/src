/*	$NetBSD: melody.c,v 1.11.60.1 2007/03/12 05:46:43 rmind Exp $ */

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ignatios Souvatzis.
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
__KERNEL_RCSID(0, "$NetBSD: melody.c,v 1.11.60.1 2007/03/12 05:46:43 rmind Exp $");

/*
 * Melody audio driver.
 *
 * Currently, only minimum support for audio output. For audio/video
 * synchronization, more is needed.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/ic/tms320av110reg.h>
#include <dev/ic/tms320av110var.h>

#include <machine/bus.h>

#include <amiga/dev/zbusvar.h>
#include <amiga/amiga/isr.h>

struct melody_softc {
	struct tav_softc	sc_tav;
	struct bus_space_tag	sc_bst_leftbyte;
	struct isr		sc_isr;
	void *			sc_intack;
};

int melody_match(struct device *, struct cfdata *, void *);
void melody_attach(struct device *, struct device *, void *);
void melody_intack(struct tav_softc *);

CFATTACH_DECL(melody, sizeof(struct melody_softc),
    melody_match, melody_attach, NULL, NULL);

int
melody_match(struct device *parent, struct cfdata *cfp, void *aux)
{
	struct zbus_args *zap;

	zap = aux;
	if (zap->manid != 2145)
		return (0);

	if (zap->prodid != 128)
		return (0);

	return (1);
}

void
melody_attach(struct device *parent, struct device *self, void *aux)
{
	struct melody_softc *sc;
	struct zbus_args *zap;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	sc = (struct melody_softc *)self;
	zap = aux;

	sc->sc_bst_leftbyte.base = (u_long)zap->va + 0;
	sc->sc_bst_leftbyte.absm = &amiga_bus_stride_2;
	sc->sc_intack = (void *)zap->va + 0xc000;

	/* set up board specific part in sc_tav */

	iot = &sc->sc_bst_leftbyte;

	if (bus_space_map(iot, 0, 128, 0, &ioh)) {
		panic("melody: cant bus_space_map");
		/* NOTREACHED */
	}
	sc->sc_tav.sc_iot = iot;
	sc->sc_tav.sc_ioh = ioh;
	sc->sc_tav.sc_pcm_ord = 0;
	sc->sc_tav.sc_pcm_18 = 0;
	sc->sc_tav.sc_dif = 0;
	sc->sc_tav.sc_pcm_div = 12;

	/*
	 * Attach option boards now. They might provide additional
	 * functionality to our audio part.
	 */

	/* attach our audio driver */

	printf(" #%d", zap->serno);
	tms320av110_attach_mi(&sc->sc_tav);
	sc->sc_isr.isr_ipl = 6;
	sc->sc_isr.isr_arg = &sc->sc_tav;
	sc->sc_isr.isr_intr = tms320av110_intr;
	add_isr(&sc->sc_isr);
}

void
melody_intack(struct tav_softc *p)
{
	struct melody_softc *sc;

	sc = (struct melody_softc *)p;
	*sc->sc_intack = 0;
}
