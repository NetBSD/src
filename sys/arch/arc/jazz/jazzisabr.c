/*	$NetBSD: jazzisabr.c,v 1.1 2001/06/13 15:03:26 soda Exp $	*/
/*	$OpenBSD: isabus.c,v 1.15 1998/03/16 09:38:46 pefo Exp $	*/
/*	NetBSD: isa.c,v 1.33 1995/06/28 04:30:51 cgd Exp 	*/

/*-
 * Copyright (c) 1995 Per Fogelstrom
 * Copyright (c) 1993, 1994 Charles M. Hannum.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)isa.c	7.2 (Berkeley) 5/12/91
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/platform.h>

#include <dev/isa/isavar.h>

#include <arc/isa/isabrvar.h>

/* Definition of the driver for autoconfig. */
int	jazzisabrmatch(struct device *, struct cfdata *, void *);
void	jazzisabrattach(struct device *, struct device *, void *);
int	jazzisabr_iointr(unsigned mask, struct clockframe *cf);

struct cfattach jazzisabr_ca = {
	sizeof(struct isabr_softc), jazzisabrmatch, jazzisabrattach
};
extern struct cfdriver jazzisabr_cd;

int
jazzisabrmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct confargs *ca = aux;

        /* Make sure that we're looking for a JAZZISABR. */
        if (strcmp(ca->ca_name, jazzisabr_cd.cd_name) != 0)
                return (0);

	return (1);
}

void
jazzisabrattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct isabr_softc *sc = (struct isabr_softc *)self;

	jazz_bus_dma_tag_init(&sc->sc_dmat);
	(*platform->set_intr)(MIPS_INT_MASK_2, isabr_iointr, 3);

	isabrattach(sc);
}
