/*	$NetBSD: vidc20.c,v 1.1.6.3 2002/06/23 17:34:53 jdolecek Exp $	*/

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

static int  vidcmatch  __P((struct device *self, struct cfdata *cf, void *aux));
static void vidcattach __P((struct device *parent, struct device *self, void *aux));
static int  vidcprint  __P((void *aux, const char *name));
static int  vidcsearch __P((struct device *, struct cfdata *, void *));

/*
 * vidc_base gives the base of the VIDC chip in memory; this is for the rest isnt
 * busspaceified yet. Initialised with VIDC_BASE for backwards compatibility.
 */
int *vidc_base = (int *) VIDC_BASE;


/*
 * vidc_fref is the reference frequency in Mhz of the detected VIDC (dependent on IOMD/IOC)
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
vidcmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
/*	struct mainbus_attach_args *mb = aux;*/

	return(1);
}

/*
 * vidcprint()
 *
 * print routine used during config of children
 */

static int
vidcprint(aux, name)
	void *aux;
	const char *name;
{
	struct mainbus_attach_args *mb = aux;

	if (mb->mb_iobase != MAINBUSCF_BASE_DEFAULT)
		printf(" base 0x%x", mb->mb_iobase);
	if (mb->mb_iosize > 1)
		printf("-0x%x", mb->mb_iobase + mb->mb_iosize - 1);
	if (mb->mb_irq != -1)
		printf(" irq %d", mb->mb_irq);
	if (mb->mb_drq != -1)
		printf(" drq 0x%08x", mb->mb_drq);

/* XXXX print flags */
	return (QUIET);
}

/*
 * vidcsearch()
 *
 * search routine used during the config of children
 */

static int
vidcsearch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct vidc20_softc *sc = (struct vidc20_softc *)parent;
	struct mainbus_attach_args mb;
	int tryagain;

	do {
		if (cf->cf_loc[MAINBUSCF_BASE] == MAINBUSCF_BASE_DEFAULT) {
			mb.mb_iobase = MAINBUSCF_BASE_DEFAULT;
			mb.mb_iosize = 0;
			mb.mb_drq = MAINBUSCF_DACK_DEFAULT;
			mb.mb_irq = MAINBUSCF_IRQ_DEFAULT;
		} else {
			mb.mb_iobase = cf->cf_loc[MAINBUSCF_BASE] + IO_CONF_BASE;
			mb.mb_iosize = 0;
			mb.mb_drq = cf->cf_loc[MAINBUSCF_DACK];
			mb.mb_irq = cf->cf_loc[MAINBUSCF_IRQ];
		}
		mb.mb_iot = sc->sc_iot;

		tryagain = 0;
		if ((*cf->cf_attach->ca_match)(parent, cf, &mb) > 0) {
			config_attach(parent, cf, &mb, vidcprint);
/*			tryagain = (cf->cf_fstate == FSTATE_STAR);*/
		}
	} while (tryagain);

	return (0);
}

/*
 * vidcattach()
 *
 * Configure all the child devices of the VIDC
 */
static void
vidcattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
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
