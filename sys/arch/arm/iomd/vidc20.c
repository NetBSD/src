/*	$NetBSD: vidc20.c,v 1.1.4.2 2002/06/20 03:38:13 nathanw Exp $	*/

/*
 * Copyright (c) 1997 Mark Brinicombe
 * Copyright (c) 1997 Causality Limited
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
 *	This product includes software developed by Mark Brinicombe
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * vidc20.c
 *
 * VIDC20 driver
 *
 * Created      : 22/02/97
 */

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <arm/iomd/vidc.h>
#include <machine/io.h>
#include <arm/iomd/iomdreg.h>
#include <arm/iomd/iomdvar.h>
#include <arm/mainbus/mainbus.h>

#include "locators.h"

struct vidc20_softc {
	struct device	sc_dev;
	bus_space_tag_t	sc_iot;
};

static int  vidcmatch(struct device *, struct cfdata *, void *);
static void vidcattach(struct device *, struct device *, void *);
static int  vidcsearch(struct device *, struct cfdata *, void *);

/*
 * vidc_base gives the base of the VIDC chip in memory; this is for
 * the rest isnt busspaceified yet. Initialised with VIDC_BASE for
 * backwards compatibility.
 */
int *vidc_base = (int *)VIDC_BASE;


/*
 * vidc_fref is the reference frequency in Mhz of the detected VIDC
 * (dependent on IOMD/IOC)
 * XXX default is RPC600 ?
 */
int  vidc_fref = 24000000;


struct cfattach vidc_ca = {
	sizeof (struct vidc20_softc), vidcmatch, vidcattach
};

/*
 * vidcmatch()
 *
 * VIDC20 is a write only device so we cannot probe it
 * We must assume things are ok.
 */
static int
vidcmatch(struct device *parent, struct cfdata *cf, void *aux)
{

	return(1);
}

/*
 * vidcsearch()
 *
 * search routine used during the config of children
 */

static int
vidcsearch(struct device *parent, struct cfdata *cf, void *aux)
{
	
	if ((*cf->cf_attach->ca_match)(parent, cf, NULL) > 0)
		config_attach(parent, cf, NULL, NULL);

	return (0);
}

/*
 * vidcattach()
 *
 * Configure all the child devices of the VIDC
 */
static void
vidcattach(struct device *parent, struct device *self, void *aux)
{
	struct vidc20_softc *sc = (struct vidc20_softc *)self;
	struct mainbus_attach_args *mb = aux;

	sc->sc_iot = mb->mb_iot;

	/*
	 * Since the VIDC is write-only, infer the type of VIDC from the
	 * type of IOMD.
	 */
	switch (IOMD_ID) {
	case ARM7500_IOC_ID:
		printf(": ARM7500 video and sound macrocell\n");
		vidc_fref = 32000000;
		break;
	case ARM7500FE_IOC_ID:
		printf(": ARM7500FE video and sound macrocell\n");
		vidc_fref = 32000000;
		break;
	default:				/* XXX default? */
	case RPC600_IOMD_ID:
		printf(": VIDC20\n");
		vidc_fref = 24000000;
		break;
	};

	config_search(vidcsearch, self, NULL);
}

/* End of vidc20.c */
