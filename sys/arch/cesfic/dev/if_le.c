/*	$NetBSD: if_le.c,v 1.1 2001/05/14 18:23:07 drochner Exp $	*/

/*
 * Copyright (c) 1997, 1999
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
 *
 */

#include "opt_inet.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#include <machine/cpu.h>
#include <machine/pte.h>

#include <machine/autoconf.h>

#include <dev/ic/lancereg.h>
#include <dev/ic/lancevar.h>
#include <dev/ic/am79900reg.h>
#include <dev/ic/am79900var.h>

#include <cesfic/cesfic/isr.h>

int lematch __P((struct device *, struct cfdata *, void *));
void leattach __P((struct device *, struct device *, void *));

struct cfattach le_ca = {
	sizeof(struct am79900_softc), lematch, leattach
};

int	leintr __P((void *));

void lewrcsr __P((struct lance_softc *, u_int16_t, u_int16_t));
u_int16_t lerdcsr __P((struct lance_softc *, u_int16_t));  

static char *lebase, *lemembase;

void
lewrcsr(sc, port, val)
	struct lance_softc *sc;
	u_int16_t port, val;
{
	*(volatile int *)(lebase + 4) = port;
	*(volatile int *)(lebase + 0) = val;
}

u_int16_t
lerdcsr(sc, port)
	struct lance_softc *sc;
	u_int16_t port;
{
	u_int16_t val;

	*(volatile int *)(lebase + 4) = port;
	val = *(volatile int *)(lebase + 0);
	return (val);
}

int
lematch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return (1);
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
extern void sic_enable_int __P((int, int, int, int, int));
static char hwa[6] = {0x00,0x80,0xa2,0x00,0x30,0x23};
void
leattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	int i;
	struct lance_softc *sc = (struct lance_softc *)self;

	mainbus_map(0x4c000000, 0x10000, 0, (void **)&lemembase);
	mainbus_map(0x48000000, 0x1000, 0, (void **)&lebase);

	sc->sc_mem = lemembase;
	sc->sc_conf3 = LE_C3_BSWP;
	sc->sc_addr = 0x4c000000;
	sc->sc_memsize = 64 * 1024;

	if (cesfic_getetheraddr(sc->sc_enaddr)) {
		/* fallback */
		for (i = 0; i < sizeof(sc->sc_enaddr); i++) {
			sc->sc_enaddr[i] = hwa[i];
		}
	}

	sc->sc_copytodesc = lance_copytobuf_contig;
	sc->sc_copyfromdesc = lance_copyfrombuf_contig;
	sc->sc_copytobuf = lance_copytobuf_contig;
	sc->sc_copyfrombuf = lance_copyfrombuf_contig;
	sc->sc_zerobuf = lance_zerobuf_contig;

	sc->sc_rdcsr = lerdcsr;
	sc->sc_wrcsr = lewrcsr;
	sc->sc_hwinit = NULL;

	lewrcsr(sc, 4, 0x44);
	am79900_config((struct am79900_softc *)sc);

	/* Establish the interrupt handler. */
	(void) isrlink(leintr, sc, 3, ISRPRI_NET);
	sic_enable_int(17, 0, 1, 3, 0);
}

int
leintr(arg)
	void *arg;
{
	struct lance_softc *sc = arg;
	u_int16_t isr4;

	isr4 = lerdcsr(sc, 4);
	if (isr4 & 0x08) {
		lewrcsr(sc, 4, isr4);
		return (1);
	}

	return (am79900_intr(sc));
}
