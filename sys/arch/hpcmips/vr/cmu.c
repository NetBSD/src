/*	$NetBSD: cmu.c,v 1.3 2001/04/18 10:48:58 sato Exp $	*/

/*-
 * Copyright (c) 1999 SASAKI Takesi
 * Copyright (c) 1999,2000 PocketBSD Project. All rights reserved.
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

#include "opt_vr41xx.h"
#include <hpcmips/vr/vr.h>
#include <hpcmips/vr/vrcpudef.h>
#include <hpcmips/vr/vripreg.h>
#include <hpcmips/vr/vripvar.h>

#include <hpcmips/vr/cmureg.h>

#include <machine/config_hook.h>

struct vrcmu_softc {
	struct device sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	config_hook_tag sc_hardpower;
	int sc_save;
};

int  vrcmu_match __P((struct device *, struct cfdata *, void *));
void vrcmu_attach __P((struct device *, struct device *, void *));

int  vrcmu_supply __P((vrcmu_chipset_tag_t, u_int16_t, int));

int vrcmu_hardpower __P((void *, int, long, void *));

struct vrcmu_function_tag vrcmu_functions  = {
	vrcmu_supply
};

struct cfattach vrcmu_ca = {
	sizeof(struct vrcmu_softc), vrcmu_match, vrcmu_attach
};

int
vrcmu_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 2; /* 1st attach group of vrip */
}

void
vrcmu_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct vrip_attach_args *va = aux;
	struct vrcmu_softc *sc = (void*)self;
    
	sc->sc_iot = va->va_iot;
	if (bus_space_map(sc->sc_iot, va->va_addr, va->va_size,
			  0 /* no flags */, &sc->sc_ioh)) {
		printf("vrcmu_attach: can't map i/o space\n");
		return;
	}
	vrip_cmu_function_register(va->va_vc, &vrcmu_functions, self);
	printf ("\n");
	sc->sc_hardpower = config_hook(CONFIG_HOOK_PMEVENT,
					CONFIG_HOOK_PMEVENT_HARDPOWER,
					CONFIG_HOOK_SHARE,
						vrcmu_hardpower, sc);

}
/* For serial console */
void
__vrcmu_supply(mask, onoff)
	u_int16_t mask;
	int onoff;
{
	u_int16_t reg;
	u_int32_t addr;
	addr = MIPS_PHYS_TO_KSEG1(VRIP_CMU_ADDR);
	reg = *((volatile u_int16_t*)addr);
	if (onoff)
		reg |= mask;
	else
		reg &= ~mask;
	*((volatile u_int16_t*)addr) = reg;
}


int
vrcmu_supply(cc, mask, onoff)
	vrcmu_chipset_tag_t cc;
	u_int16_t mask;
	int onoff;
{
	struct vrcmu_softc *sc = (void*)cc;
	u_int16_t reg;
    
	reg = bus_space_read_2(sc->sc_iot, sc->sc_ioh, 0);
#ifdef VRCMU_VERBOSE
	printf("cmu register(enter):");    
	bitdisp16(reg);
#endif
	if (onoff)
		reg |= mask;
	else
		reg &= ~mask;
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0, reg);
#ifdef VRCMU_VERBOSE
	printf("cmu register(exit) :");    
	bitdisp16(reg);
#endif
	return 0;
}

int
vrcmu_hardpower(ctx, type, id, msg)
	void *ctx;
	int type;
	long id;
	void *msg;
{
	struct vrcmu_softc *sc = ctx;
	int why =(int)msg;

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
