/*	$NetBSD: wdc_pioc.c,v 1.3 1998/02/21 22:51:43 mark Exp $	*/

/*
 * Copyright (c) 1994, 1995 Charles M. Hannum.  All rights reserved.
 *
 * DMA and multi-sector PIO handling are derived from code contributed by
 * Onno van der Linden.
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
 *
 *	from: wd.c,v 1.156 1997/01/17 20:45:29 perry Exp
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <machine/bus.h>
#include <machine/bootconfig.h>
#include <machine/irqhandler.h>
#include <arm32/mainbus/piocvar.h>
#include <arm32/dev/wdreg.h>
#include <arm32/dev/wdlink.h>

#include "locators.h"

/* prototypes for functions */

static int  wdc_pioc_probe  __P((struct device *, struct cfdata *, void *));
static void wdc_pioc_attach __P((struct device *, struct device *, void *));

/* device attach structure */

struct cfattach wdc_pioc_ca = {
	sizeof(struct wdc_softc), wdc_pioc_probe, wdc_pioc_attach
};

/*
 * int wdc_pioc_probe(struct device *parent, struct cfdata *cf, void *aux)
 *
 * Make sure we are trying to attach a wdc device and then
 * probe for one.
 */

static int
wdc_pioc_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pioc_attach_args *pa = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_space_handle_t aux_ioh;
	int rv = 0;
	u_int iobase;

	if (pa->pa_name && strcmp(pa->pa_name, "wdc") != 0)
		return(0);

	/* We need an offset */
	if (pa->pa_offset == PIOCCF_OFFSET_DEFAULT)
		return(0);

	/* XXX - need xname */

	iot = pa->pa_iot;
	iobase = pa->pa_iobase + pa->pa_offset;
	
	if (bus_space_map(iot, iobase, 8, 0, &ioh))
		return(0);

	if (bus_space_map(iot, iobase + (WD_ALTSTATUS*4), 1, 0, &aux_ioh))
		goto out;

	rv = wdcprobe_internal(iot, ioh, aux_ioh, ioh, -1, "wdc");

	pa->pa_iosize = 8*4;

out:
	bus_space_unmap(iot, ioh, 8);
	bus_space_unmap(iot, aux_ioh, 1);
	return(rv);
}

/*
 * void wdc_pioc_attach(struct device *parent, struct device *self, void *aux)
 *
 * attach the wdc device
 */

static void
wdc_pioc_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct wdc_softc *wdc = (void *)self;
	struct pioc_attach_args *pa = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_space_handle_t aux_ioh;
	u_int iobase;

/*	TAILQ_INIT(&wdc->sc_drives);*/
	iot = pa->pa_iot;
	iobase = pa->pa_iobase + pa->pa_offset;

	if (bus_space_map(iot, iobase, 8, 0, &ioh))
		panic("%s: Cannot map IO\n", wdc->sc_dev.dv_xname);

	if (bus_space_map(iot, iobase + (WD_ALTSTATUS*4), 1, 0,
	     &aux_ioh))
		panic("%s: Cannot map IO\n", wdc->sc_dev.dv_xname);

	wdc->sc_ih = intr_claim(pa->pa_irq, IPL_BIO, "wdc",
	    wdcintr, wdc);

	/* Hack needed for some drives */
        if (boot_args) {
        	char *ptr;
       
		ptr = strstr(boot_args, "nowdreset");
		if (ptr)
			wdc->sc_flags |= WDCF_NORESET;
	}

	wdcattach_internal(wdc, iot, ioh, aux_ioh, ioh, -1, pa->pa_drq);
}

/* End of wdc_pioc.c */
