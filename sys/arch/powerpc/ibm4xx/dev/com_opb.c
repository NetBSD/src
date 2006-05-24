/* $NetBSD: com_opb.c,v 1.14.2.1 2006/05/24 10:57:09 yamt Exp $ */

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1998
 *	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: com_opb.c,v 1.14.2.1 2006/05/24 10:57:09 yamt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/tty.h>
#include <sys/systm.h>

#include <lib/libkern/libkern.h>

#include <machine/cpu.h>

#include <powerpc/ibm4xx/dev/opbvar.h>
#include <powerpc/ibm4xx/dev/comopbvar.h>

#include "com.h"
#if (NCOM > 0)
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

struct com_opb_softc {
	struct com_softc sc_com;
	void *sc_ih;
};

static int	com_opb_probe(struct device *, struct cfdata *, void *);
static void	com_opb_attach(struct device *, struct device *, void *);

CFATTACH_DECL(com_opb, sizeof(struct com_opb_softc),
    com_opb_probe, com_opb_attach, NULL, NULL);

int
com_opb_probe(struct device *parent, struct cfdata *cf, void *aux)
{
	struct opb_attach_args *oaa = aux;

	/* match only com devices */
	if (strcmp(oaa->opb_name, cf->cf_name) != 0)
		return 0;

	return (1);
}

void
com_opb_attach(struct device *parent, struct device *self, void *aux)
{
	struct com_opb_softc *msc = (void *)self;
	struct com_softc *sc = &msc->sc_com;
	struct opb_attach_args *oaa = aux;
	prop_number_t freq;

	sc->sc_iot = oaa->opb_bt;
	sc->sc_iobase = oaa->opb_addr;

	/* XXX console check */

	bus_space_map(sc->sc_iot, oaa->opb_addr, COM_NPORTS, 0, &sc->sc_ioh);

	freq = prop_dictionary_get(device_properties(&sc->sc_dev), "frequency");
	if (freq == NULL) {
		printf(": unable to get frequency property\n");
		return;
	}
	KASSERT(prop_object_type(freq) == PROP_TYPE_NUMBER);
	sc->sc_frequency = (int) prop_number_integer_value(freq);

	com_attach_subr(sc);

	intr_establish(oaa->opb_irq, IST_LEVEL, IPL_SERIAL, comintr, sc);
}

/*
 * com_opb_cnattach:
 * Initialize the system console.
 */
void
com_opb_cnattach(int com_freq, int conaddr, int conspeed, int conmode)
{
	static int attached = 0;
#if (NCOM > 0)
	bus_space_tag_t tag;
#endif

	if (attached)
		return;
	attached = 1;

#if (NCOM > 0)
	/* We *know* the com-console attaches to opb */
	tag = opb_get_bus_space_tag();

	if (comcnattach(tag,
		conaddr, conspeed, com_freq, COM_TYPE_NORMAL, conmode))
		panic("can't init serial console @%x", conaddr);
	else
		return;
#endif
	panic("console device missing -- serial console not in kernel");
	/* Of course, this is moot if there is no console... */
}

/*
 * com_opb_device_register:
 */
void
com_opb_device_register(struct device *dev, int frequency)
{
	/* Set the frequency of the on-chip UART. */
	prop_number_t pn = prop_number_create_integer(frequency);
	KASSERT(pn != NULL);

	if (prop_dictionary_set(device_properties(dev),
				"frequency", pn) == FALSE) {
		printf("WARNING: unable to set frequency "
			"property for %s\n", dev->dv_xname);
	}
	prop_object_release(pn);
}
