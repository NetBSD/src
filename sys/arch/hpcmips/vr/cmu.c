/*	$NetBSD: cmu.c,v 1.4.2.2 2002/02/11 20:08:12 jdolecek Exp $	*/

/*-
 * Copyright (c) 1999 SASAKI Takesi
 * Copyright (c) 1999, 2000, 2002 PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <mips/cpuregs.h>

#include <machine/bus.h>
#include <machine/debug.h>

#include "opt_vr41xx.h"
#include <hpcmips/vr/vr.h>
#include <hpcmips/vr/vrcpudef.h>
#include <hpcmips/vr/vripif.h>
#include <hpcmips/vr/vripreg.h>

#include <hpcmips/vr/cmureg.h>

#include <machine/config_hook.h>

struct vrcmu_softc {
	struct device sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	config_hook_tag sc_hardpower;
	int sc_save;
	struct vrcmu_chipset_tag sc_chipset;
};

int	vrcmu_match(struct device *, struct cfdata *, void *);
void	vrcmu_attach(struct device *, struct device *, void *);
int	vrcmu_supply(vrcmu_chipset_tag_t, u_int16_t, int);
int	vrcmu_hardpower(void *, int, long, void *);

struct cfattach vrcmu_ca = {
	sizeof(struct vrcmu_softc), vrcmu_match, vrcmu_attach
};

int
vrcmu_match(struct device *parent, struct cfdata *cf, void *aux)
{

	return (2);		/* 1st attach group of vrip */
}

void
vrcmu_attach(struct device *parent, struct device *self, void *aux)
{
	struct vrip_attach_args *va = aux;
	struct vrcmu_softc *sc = (void *)self;

	sc->sc_iot = va->va_iot;
	sc->sc_chipset.cc_sc = sc;
	sc->sc_chipset.cc_clock = vrcmu_supply;
	if (bus_space_map(sc->sc_iot, va->va_addr, va->va_size,
	    0 /* no flags */, &sc->sc_ioh)) {
		printf(": can't map i/o space\n");
		return;
	}
	printf ("\n");

	vrip_register_cmu(va->va_vc, &sc->sc_chipset);
	sc->sc_hardpower = config_hook(CONFIG_HOOK_PMEVENT,
	    CONFIG_HOOK_PMEVENT_HARDPOWER,
	    CONFIG_HOOK_SHARE,
	    vrcmu_hardpower, sc);
}

int
vrcmu_supply(vrcmu_chipset_tag_t cc, u_int16_t mask, int onoff)
{
	struct vrcmu_softc *sc = cc->cc_sc;
	u_int16_t reg;

	reg = bus_space_read_2(sc->sc_iot, sc->sc_ioh, 0);
#ifdef VRCMU_VERBOSE
	printf("cmu register(enter):");
	dbg_bit_print(reg);
#endif
	if (onoff)
		reg |= mask;
	else
		reg &= ~mask;
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0, reg);
#ifdef VRCMU_VERBOSE
	printf("cmu register(exit) :");
	dbg_bit_print(reg);
#endif
	return (0);
}

int
vrcmu_hardpower(void *ctx, int type, long id, void *msg)
{
	struct vrcmu_softc *sc = ctx;
	int why = (int)msg;

	switch (why) {
	case PWR_STANDBY:
	case PWR_SUSPEND:
		sc->sc_save = bus_space_read_2(sc->sc_iot, sc->sc_ioh, 0);
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0, 0);
		break;
	case PWR_RESUME:
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0, sc->sc_save);
		break;
	}
	return (0);
}
