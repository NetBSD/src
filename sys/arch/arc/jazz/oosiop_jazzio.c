/* $NetBSD: oosiop_jazzio.c,v 1.4.8.1 2006/05/24 10:56:34 yamt Exp $ */

/*
 * Copyright (c) 2001 Shuichiro URATA.  All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: oosiop_jazzio.c,v 1.4.8.1 2006/05/24 10:56:34 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/malloc.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/scsipi/scsi_message.h>

#include <machine/cpu.h>
#include <machine/autoconf.h>
#include <machine/bus.h>

#include <dev/ic/oosiopreg.h>
#include <dev/ic/oosiopvar.h>
#include <arc/jazz/jazziovar.h>

#include "ioconf.h"

int	oosiop_jazzio_match(struct device *, struct cfdata *, void *);
void	oosiop_jazzio_attach(struct device *, struct device *, void *);

CFATTACH_DECL(oosiop_jazzio, sizeof(struct oosiop_softc),
    oosiop_jazzio_match, oosiop_jazzio_attach, NULL, NULL);

/*
 * Match driver based on name
 */
int
oosiop_jazzio_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct jazzio_attach_args *ja = aux;

	if (strcmp(ja->ja_name, "NCRC700") != 0)
		return 0;

	return 1;
}

void
oosiop_jazzio_attach(struct device *parent, struct device *self, void *aux)
{
	struct jazzio_attach_args *ja = aux;
	struct oosiop_softc *sc = (void *)self;
	int i, scid;

	sc->sc_bst = ja->ja_bust;
	sc->sc_dmat = ja->ja_dmat;

	if (bus_space_map(sc->sc_bst, ja->ja_addr,
	    OOSIOP_NREGS, 0, &sc->sc_bsh) != 0) {
		printf(": failed to map regsters\n");
		return;
	}

	sc->sc_chip = OOSIOP_700_66;
	sc->sc_freq = 50000000;

	/* Preserve host id */
	scid = oosiop_read_1(sc, OOSIOP_SCID);
	for (i = 0; i < OOSIOP_NTGT; i++)
		if (scid & (1 << i))
			break;
	if (i == OOSIOP_NTGT)
		i = OOSIOP_NTGT - 1;

	sc->sc_id = i;

	jazzio_intr_establish(ja->ja_intr, (intr_handler_t)oosiop_intr, sc);

	oosiop_attach(sc);
}
