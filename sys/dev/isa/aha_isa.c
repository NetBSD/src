/*	$NetBSD: aha_isa.c,v 1.4 1997/06/06 23:43:47 thorpej Exp $	*/

/*
 * Copyright (c) 1994, 1996, 1997 Charles M. Hannum.  All rights reserved.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <dev/ic/ahareg.h>
#include <dev/ic/ahavar.h>

#define	AHA_ISA_IOSIZE	4

#ifdef __BROKEN_INDIRECT_CONFIG
int	aha_isa_probe __P((struct device *, void *, void *));
#else
int	aha_isa_probe __P((struct device *, struct cfdata *, void *));
#endif
void	aha_isa_attach __P((struct device *, struct device *, void *));

struct cfattach aha_isa_ca = {
	sizeof(struct aha_softc), aha_isa_probe, aha_isa_attach
};

/*
 * Check the slots looking for a board we recognise
 * If we find one, note it's address (slot) and call
 * the actual probe routine to check it out.
 */
int
aha_isa_probe(parent, match, aux)
	struct device *parent;
#ifdef __BROKEN_INDIRECT_CONFIG
	void *match;
#else
	struct cfdata *match;
#endif
	void *aux;
{
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	struct aha_probe_data apd;
	int rv;

	if (bus_space_map(iot, ia->ia_iobase, AHA_ISA_IOSIZE, 0, &ioh))
		return (0);

	rv = aha_find(iot, ioh, &apd);

	bus_space_unmap(iot, ioh, AHA_ISA_IOSIZE);

	if (rv) {
		if (ia->ia_irq != -1 && ia->ia_irq != apd.sc_irq)
			return (0);
		if (ia->ia_drq != -1 && ia->ia_drq != apd.sc_drq)
			return (0);
		ia->ia_irq = apd.sc_irq;
		ia->ia_drq = apd.sc_drq;
		ia->ia_msize = 0;
		ia->ia_iosize = AHA_ISA_IOSIZE;
	}
	return (rv);
}

/*
 * Attach all the sub-devices we can find
 */
void
aha_isa_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	struct aha_softc *sc = (void *)self;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	struct aha_probe_data apd;
	isa_chipset_tag_t ic = ia->ia_ic;

	printf("\n");

	if (bus_space_map(iot, ia->ia_iobase, AHA_ISA_IOSIZE, 0, &ioh))
		panic("aha_isa_attach: bus_space_map failed");

	sc->sc_iot = iot;
	sc->sc_ioh = ioh;
	sc->sc_dmat = ia->ia_dmat;
	if (!aha_find(iot, ioh, &apd))
		panic("aha_isa_attach: aha_find failed");

	if (apd.sc_drq != -1)
		isa_dmacascade(parent, apd.sc_drq);

	sc->sc_ih = isa_intr_establish(ic, apd.sc_irq, IST_EDGE, IPL_BIO,
	    aha_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	aha_attach(sc, &apd);
}
