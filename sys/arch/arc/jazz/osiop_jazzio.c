/* $NetBSD: osiop_jazzio.c,v 1.1 2001/04/30 04:52:54 tsutsui Exp $ */

/*
 * Copyright (c) 2001 Izumi Tsutsui.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/malloc.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <machine/cpu.h>
#include <machine/autoconf.h>
#include <machine/bus.h>

#include <dev/ic/osiopreg.h>
#include <dev/ic/osiopvar.h>

#include <arc/jazz/jazziovar.h>

int osiop_jazzio_match(struct device *, struct cfdata *, void *);
void osiop_jazzio_attach(struct device *, struct device *, void *);
int osiop_jazzio_intr(void *);

struct cfattach osiop_jazzio_ca = {
	sizeof(struct osiop_softc), osiop_jazzio_match, osiop_jazzio_attach
};

int
osiop_jazzio_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct jazzio_attach_args *ja = aux;

	if (strcmp(ja->ja_name, "osiop") != 0)
		return (0);

	return (1);
}

void
osiop_jazzio_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct jazzio_attach_args *ja = aux;
	struct osiop_softc *sc = (void *)self;
	int err, scid;

	sc->sc_bst = ja->ja_bust;
	sc->sc_dmat = ja->ja_dmat;

	/*
	 * Map registers
	 */
	err = bus_space_map(sc->sc_bst, ja->ja_addr,
	    OSIOP_NREGS, 0, &sc->sc_reg);
	if (err) {
		printf("%s: failed to map registers, err=%d\n",
		    sc->sc_dev.dv_xname, err);
		return;
	}

	sc->sc_clock_freq = 50;	/* MHz */
	sc->sc_ctest7 = 0; /* | OSIOP_CTEST7_TT1 */
	sc->sc_dcntl = OSIOP_DCNTL_EA;

	/* XXX should check ID in BIOS variables? */
	scid = ffs(osiop_read_1(sc, OSIOP_SCID));

	if (scid == 0)
		scid = 7;
	else
		scid--;

	sc->sc_id = scid;

	/*
	 * Call common attachment
	 */
	osiop_attach(sc);

	/*
	 * Set up interrupt handler.
         */
	jazzio_intr_establish(ja->ja_intr,
	    (intr_handler_t)osiop_jazzio_intr, sc);
}

/*
 * interrupt handler
 */
int
osiop_jazzio_intr(arg)
	void *arg;
{
	struct osiop_softc *sc = arg;
	u_int8_t istat;

	/* This is potentially nasty, since the IRQ is level triggered... */
	if (sc->sc_flags & OSIOP_INTSOFF)
		return (0);

	istat = osiop_read_1(sc, OSIOP_ISTAT);

	if ((istat & (OSIOP_ISTAT_SIP | OSIOP_ISTAT_DIP)) == 0)
		return (0);

	/* Save interrupt details for the back-end interrupt handler */
	sc->sc_sstat0 = osiop_read_1(sc, OSIOP_SSTAT0);
	sc->sc_istat = istat;
	sc->sc_dstat = osiop_read_1(sc, OSIOP_DSTAT);

	/* Deal with the interrupt */
	osiop_intr(sc);

	return (1);
}
