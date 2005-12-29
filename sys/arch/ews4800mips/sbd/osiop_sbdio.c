/*	$NetBSD: osiop_sbdio.c,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

/*
 * Copyright (c) 2001, 2005 Izumi Tsutsui.  All rights reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: osiop_sbdio.c,v 1.1 2005/12/29 15:20:09 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/sbdiovar.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/ic/osiopreg.h>
#include <dev/ic/osiopvar.h>

int osiop_sbdio_match(struct device *, struct cfdata *, void *);
void osiop_sbdio_attach(struct device *, struct device *, void *);
int osiop_sbdio_intr(void *);

CFATTACH_DECL(osiop_sbdio, sizeof(struct osiop_softc),
    osiop_sbdio_match, osiop_sbdio_attach, NULL, NULL);

int
osiop_sbdio_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct sbdio_attach_args *sa = aux;

	return strcmp(sa->sa_name, "osiop") ? 0 : 1;
}

void
osiop_sbdio_attach(struct device *parent, struct device *self, void *aux)
{
	struct sbdio_attach_args *sa = aux;
	struct osiop_softc *sc = (void *)self;
	int error, scid;

	printf(" at %p irq %d ", (void *)sa->sa_addr1, sa->sa_irq);

	sc->sc_dmat = sa->sa_dmat;
	sc->sc_bst  = sa->sa_bust;

	error = bus_space_map(sc->sc_bst, sa->sa_addr1, OSIOP_NREGS, 0,
	    &sc->sc_reg);
	if (error != 0) {
		printf(": can't map registers, error = %d\n", error);
		return;
	}

	sc->sc_clock_freq = 25;	/* MHz */
	sc->sc_ctest7 = 0;
	sc->sc_dcntl = OSIOP_DCNTL_EA;
	if (sa->sa_flags == 0x0001) /* TR2A */
		sc->sc_ctest4 = OSIOP_CTEST4_MUX; /* Host bus multiplex mode */

	scid = ffs(osiop_read_1(sc, OSIOP_SCID));
	if (scid == 0)
		scid = 7;
	else
		scid--;
	sc->sc_id = scid;

	intr_establish(sa->sa_irq, osiop_sbdio_intr, self);

	osiop_attach(sc);
}

int
osiop_sbdio_intr(void *arg)
{
	struct osiop_softc *sc = arg;
	uint8_t istat;

	if (sc->sc_flags & OSIOP_INTSOFF)
		return 0;

	istat = osiop_read_1(sc, OSIOP_ISTAT);

	if ((istat & (OSIOP_ISTAT_SIP | OSIOP_ISTAT_DIP)) == 0)
		return 0;

	/* Save interrupt details for the back-end interrupt handler */
	sc->sc_sstat0 = osiop_read_1(sc, OSIOP_SSTAT0);
	sc->sc_istat = istat;
	sc->sc_dstat = osiop_read_1(sc, OSIOP_DSTAT);

	osiop_intr(sc);

	return 1;
}
