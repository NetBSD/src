/*	$NetBSD: wdc_pcmcia.c,v 1.1 1998/01/19 19:49:03 matt Exp $ */

/*
 * Copyright (c) 1994, 1995 Charles M. Hannum.  All rights reserved.
 *
 * DMA and multi-sector PIO handling are derived from code contributed by
 * Onno van der Linden.
 *
 * Atapi support added by Manuel Bouyer.
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
 *	This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#undef ATAPI_DEBUG_WDC
#define WDDEBUG

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/syslog.h>
#include <sys/proc.h>

#include <vm/vm.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/pcmcia/pcmciavar.h>
#include <dev/isa/isavar.h>
#include <dev/ic/wdcvar.h>

#define WDC_PCMCIA_REG_NPORTS      8
#define WDC_PCMCIA_AUXREG_OFFSET   0x206
#define WDC_PCMCIA_AUXREG_NPORTS   1

struct wdc_pcmcia_softc {
	struct wdc_softc sc_wdcdev;
	struct wdc_attachment_data sc_ad;
	struct pcmcia_io_handle sc_pioh;
	struct pcmcia_io_handle sc_auxpioh;
	int sc_iowindow;
	int sc_auxiowindow;
	void *sc_ih;
};

#ifdef __BROKEN_INDIRECT_CONFIG
static int wdc_pcmcia_match	__P((struct device *, void *, void *));
#else
static int wdc_pcmcia_match	__P((struct device *, struct cfdata *, void *));
#endif
static void wdc_pcmcia_attach	__P((struct device *, struct device *, void *));

struct cfattach wdc_pcmcia_ca = {
	sizeof(struct wdc_pcmcia_softc), wdc_pcmcia_match, wdc_pcmcia_attach
};

static int
wdc_pcmcia_match(
	struct device *parent,
#ifdef __BROKEN_INDIRECT_CONFIG
	void *match,
#else
	struct cfdata *match,
#endif
	void *aux)
{
	struct pcmcia_attach_args *pa = aux;

	if (pa->card->cis1_info[0] != NULL &&
	    strcmp(pa->card->cis1_info[0], "Digital Equipment Corporation.") == 0 &&
	    pa->card->cis1_info[1] != NULL &&
	    strcmp(pa->card->cis1_info[1], "Digital Mobile Media CD-ROM") == 0)
		return 1;

	return 0;
}

static void
wdc_pcmcia_attach(
	struct device *parent,
	struct device *self,
	void *aux)
{
	struct wdc_pcmcia_softc *sc = (void *)self;
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe = pa->pf->cfe_head.sqh_first;

	bzero(&sc->sc_ad, sizeof(sc->sc_ad));

	for (; cfe != NULL; cfe = cfe->cfe_list.sqe_next) {
		if (pcmcia_io_alloc(pa->pf, cfe->iospace[0].start,
		    cfe->iospace[0].length, 0, &sc->sc_pioh))
			continue;
		if (cfe->num_iospace > 1) {
			if (!pcmcia_io_alloc(pa->pf, cfe->iospace[1].start,
			    cfe->iospace[1].length, 0, &sc->sc_auxpioh)) {
				break;
			}
		} else {
			sc->sc_auxpioh.iot = sc->sc_pioh.iot;
			if (!bus_space_subregion(sc->sc_pioh.iot, sc->sc_pioh.ioh,
			    WDC_PCMCIA_REG_NPORTS,
			    cfe->iospace[0].length - WDC_PCMCIA_REG_NPORTS,
			    &sc->sc_auxpioh.ioh))
				break;
		}
		pcmcia_chip_io_free(pa->pf->sc->pct, pa->pf->sc->pch, &sc->sc_pioh);
	}

	if (cfe == NULL) {
		printf(": can't handle card info\n");
		return;
	}

	sc->sc_ad.iot = sc->sc_pioh.iot;
	sc->sc_ad.ioh = sc->sc_pioh.ioh;
	sc->sc_ad.auxiot = sc->sc_auxpioh.iot;
	sc->sc_ad.auxioh = sc->sc_auxpioh.ioh;

	/* Enable the card. */
	pcmcia_function_init(pa->pf, cfe);
	if (pcmcia_function_enable(pa->pf)) {
		printf(": function enable failed\n");
		return;
	}

	if (pcmcia_io_map(pa->pf, PCMCIA_WIDTH_AUTO, 0,
			  sc->sc_pioh.size, &sc->sc_pioh,
			  &sc->sc_iowindow)) {
		printf(": can't map first I/O space\n");
		return;
	} 
	if (pcmcia_io_map(pa->pf, PCMCIA_WIDTH_AUTO, 0,
			  sc->sc_auxpioh.size, &sc->sc_auxpioh,
			  &sc->sc_auxiowindow)) {
		printf(": can't map second I/O space\n");
		return;
	}

	printf("\n");
	sc->sc_ih = pcmcia_intr_establish(pa->pf, IPL_BIO, wdcintr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt\n", self->dv_xname);
		return;
	}
	wdcattach(&sc->sc_wdcdev, &sc->sc_ad);
}
