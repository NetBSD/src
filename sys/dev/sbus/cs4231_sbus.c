/*	$NetBSD: cs4231_sbus.c,v 1.13.2.1 2000/06/22 17:08:07 minoura Exp $	*/

/*-
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

#include "audio.h"
#if NAUDIO > 0

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>

#include <dev/ic/ad1848reg.h>
#include <dev/ic/cs4231reg.h>
#include <dev/ic/ad1848var.h>
#include <dev/ic/cs4231var.h>

#define AUDIO_ROM_NAME		"SUNW,CS4231"
#define CS4231_REG_SIZE		16

/* autoconfiguration driver */
void	cs4231_attach_sbus __P((struct device *, struct device *, void *));
int	cs4231_match_sbus __P((struct device *, struct cfdata *, void *));

struct cfattach audiocs_sbus_ca = {
	sizeof(struct cs4231_softc), cs4231_match_sbus, cs4231_attach_sbus
};

/* autoconfig routines */

int
cs4231_match_sbus(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct sbus_attach_args *sa = aux;

	return (strcmp(AUDIO_ROM_NAME, sa->sa_name) == 0);
}

/*
 * Audio chip found.
 */
void
cs4231_attach_sbus(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct cs4231_softc *sc = (struct cs4231_softc *)self;
	struct sbus_attach_args *sa = aux;
	bus_space_handle_t bh;

	sc->sc_bustag = sa->sa_bustag;
	sc->sc_dmatag = sa->sa_dmatag;

	sc->sc_ad1848.parent = sc;
	sc->sc_ad1848.sc_iot = sc->sc_bustag;
	sc->sc_ad1848.sc_readreg = cs4231_read;
	sc->sc_ad1848.sc_writereg = cs4231_write;

	/*
	 * Map my registers in, if they aren't already in virtual
	 * address space.
	 */
	if (sa->sa_npromvaddrs) {
		bh = (bus_space_handle_t)sa->sa_promvaddrs[0];
	} else {
		if (sbus_bus_map(sa->sa_bustag, sa->sa_slot,
				 sa->sa_offset,
				 sa->sa_size,
				 BUS_SPACE_MAP_LINEAR,
				 0, &bh) != 0) {
			printf("%s @ sbus: cannot map registers\n",
				self->dv_xname);
			return;
		}
	}

	sc->sc_ad1848.sc_ioh = bh;
	sc->sc_dmareg = (struct apc_dma *)(u_long)(bh + CS4231_REG_SIZE);

	cs4231_init(sc);

	/* Put ad1848 driver in `MODE 2' mode */
	sc->sc_ad1848.mode = 2;
	ad1848_attach(&sc->sc_ad1848);

	printf("\n");

	sbus_establish(&sc->sc_sd, &sc->sc_ad1848.sc_dev);

	/* Establish interrupt channel */
	if (sa->sa_nintr)
		bus_intr_establish(sa->sa_bustag,
				   sa->sa_pri, 0,
				   cs4231_intr, sc);

	evcnt_attach_dynamic(&sc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
	    sc->sc_ad1848.sc_dev.dv_xname, "intr");
	audio_attach_mi(&audiocs_hw_if, sc, &sc->sc_ad1848.sc_dev);
}
#endif
