/*	$NetBSD: gpbus.c,v 1.1.1.1 1999/09/16 12:23:32 takemura Exp $	*/

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
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

#include <machine/bus.h>

#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <hpcmips/vr/vripreg.h>
#include <hpcmips/vr/vripvar.h>
#include <hpcmips/vr/vrgiureg.h>

#include "locators.h"

int  gpbusprint __P((void*, const char*));
int  gpbusmatch __P((struct device*, struct cfdata*, void*));
void gpbusattach __P((struct device*, struct device*, void*));
int  gpbus_button_intr __P((void*));

struct gpbus_softc {
	struct device sc_dev;
	vrgiu_chipset_tag_t sc_gc;
	vrgiu_function_tag_t sc_gf;
	u_int32_t sc_button_mask;
};

struct cfattach gpbus_ca = {
	sizeof(struct gpbus_softc), gpbusmatch, gpbusattach
};

int
gpbusmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	platid_mask_t mask;
	struct gpbus_attach_args *gpa = aux;
    
	if (strcmp(gpa->gpa_busname, match->cf_driver->cd_name))
		return 0;
	if (match->cf_loc[GPBUSIFCF_PLATFORM] == 0)
		return 0;
	mask = PLATID_DEREF(match->cf_loc[GPBUSIFCF_PLATFORM]);
	if (!platid_match(&platid, &mask))
		return 0;
	return 1;
}

int
gpbus_button_intr(arg)
	void *arg;
{
	struct gpbus_softc *sc = arg;
	u_int32_t reg, regb, mask;
	reg = sc->sc_gf->gf_regread_4(sc->sc_gc, GIUINTSTAT_REG);
	mask = sc->sc_button_mask;
	regb = reg & mask;
	printf ("button");
	bitdisp32(regb);
	/* Clear interrupt */
	sc->sc_gf->gf_regwrite_4(sc->sc_gc, GIUINTSTAT_REG, reg & ~mask);

	return 0;
}

void
gpbusattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct gpbus_attach_args *gpa = aux;
	int port;
	int *loc;
	int mode;
    
	struct gpbus_softc *sc = (void*)self;
	sc->sc_gc = gpa->gpa_gc;
	sc->sc_gf = gpa->gpa_gf;

	loc = sc->sc_dev.dv_cfdata->cf_loc;
	printf (" comctrl=%d button=[%d %d %d %d %d %d %d %d %d %d %d %d]\n",
		loc[GPBUSIFCF_COMCTRL],
		loc[GPBUSIFCF_BUTTON0],
		loc[GPBUSIFCF_BUTTON1],
		loc[GPBUSIFCF_BUTTON2],
		loc[GPBUSIFCF_BUTTON3],
		loc[GPBUSIFCF_BUTTON4],
		loc[GPBUSIFCF_BUTTON5],
		loc[GPBUSIFCF_BUTTON6],
		loc[GPBUSIFCF_BUTTON7],
		loc[GPBUSIFCF_BUTTON8],
		loc[GPBUSIFCF_BUTTON9],
		loc[GPBUSIFCF_BUTTON10],
		loc[GPBUSIFCF_BUTTON11]
		);
	/*
	 * COM control
	 */
	if ((port = loc[GPBUSIFCF_COMCTRL]) != GPBUSIFCF_COMCTRL)
		gpa->gpa_gf->gf_map(gpa->gpa_gc, GIUPORT_COM, port);
	/*
	 * Button 
	 */
	sc->sc_button_mask = 0;
#if 1 /* Windows CE default */
	mode = VRGIU_INTR_EDGE_HOLD; 
#else /* XXX Don't challenge! Freestyle Only */
	mode = VRGIU_INTR_LEVEL_LOW_HOLD;
#endif
#define ESTABLISH_BUTTON_INTR(n) \
    if ((port = loc[GPBUSIFCF_BUTTON##n]) != GPBUSIFCF_BUTTON##n##_DEFAULT) { \
	sc->sc_gf->gf_intr_establish(sc->sc_gc, port, mode, IPL_TTY, gpbus_button_intr, sc); \
        sc->sc_button_mask |= (1 << port); \
    }
	ESTABLISH_BUTTON_INTR(0);
	ESTABLISH_BUTTON_INTR(1);
	ESTABLISH_BUTTON_INTR(2);
	ESTABLISH_BUTTON_INTR(3);
	ESTABLISH_BUTTON_INTR(4);
	ESTABLISH_BUTTON_INTR(5);
	ESTABLISH_BUTTON_INTR(6);
	ESTABLISH_BUTTON_INTR(7);
	ESTABLISH_BUTTON_INTR(8);
	ESTABLISH_BUTTON_INTR(9);
	ESTABLISH_BUTTON_INTR(10);
	ESTABLISH_BUTTON_INTR(11);
}
