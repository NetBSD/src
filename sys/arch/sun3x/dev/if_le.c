/*	$NetBSD: if_le.c,v 1.5 1997/03/15 18:11:06 is Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass and Gordon W. Ross.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_ether.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/dvma.h>
#include <machine/obio.h>
#include <machine/idprom.h>

#include <dev/ic/am7990reg.h>
#include <dev/ic/am7990var.h>

#define LE_MEMSIZE	0x4000	/* 16K */

/*
 * LANCE registers.
 * The real stuff is in dev/ic/am7990reg.h
 */
struct lereg1 {
	volatile u_int16_t	ler1_rdp;	/* data port */
	volatile u_int16_t	ler1_rap;	/* register select port */
};

/*
 * Ethernet software status per interface.
 * The real stuff is in dev/ic/am7990var.h
 */
struct	le_softc {
	struct	am7990_softc sc_am7990;	/* glue to MI code */
	void *	sc_kmem;	/* from malloc */

	struct	lereg1 *sc_r1;		/* LANCE registers */
};

static int	le_match __P((struct device *, struct cfdata *, void *));
static void	le_attach __P((struct device *, struct device *, void *));

struct cfattach le_ca = {
	sizeof(struct le_softc), le_match, le_attach
};

hide void lewrcsr __P((struct am7990_softc *, u_int16_t, u_int16_t));
hide u_int16_t lerdcsr __P((struct am7990_softc *, u_int16_t));  

hide void
lewrcsr(sc, port, val)
	struct am7990_softc *sc;
	u_int16_t port, val;
{
	register struct lereg1 *ler1 = ((struct le_softc *)sc)->sc_r1;

	ler1->ler1_rap = port;
	ler1->ler1_rdp = val;
}

hide u_int16_t
lerdcsr(sc, port)
	struct am7990_softc *sc;
	u_int16_t port;
{
	register struct lereg1 *ler1 = ((struct le_softc *)sc)->sc_r1;
	u_int16_t val;

	ler1->ler1_rap = port;
	val = ler1->ler1_rdp;
	return (val);
} 

int
le_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;

	/* Make sure there is something there... */
	if (bus_peek(ca->ca_bustype, ca->ca_paddr, 2) == -1)
		return (0);

	/* Default interrupt priority. */
	if (ca->ca_intpri == -1)
		ca->ca_intpri = 3;

	return (1);
}

void
le_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct le_softc *lesc = (struct le_softc *)self;
	struct am7990_softc *sc = &lesc->sc_am7990;
	struct confargs *ca = aux;

	lesc->sc_r1 = bus_mapin(ca->ca_bustype,
	    ca->ca_paddr, sizeof(struct lereg1));

	lesc->sc_kmem = malloc(LE_MEMSIZE, M_DEVBUF, M_NOWAIT);
	sc->sc_memsize = LE_MEMSIZE;
	sc->sc_mem = dvma_mapin(lesc->sc_kmem, LE_MEMSIZE, 0);
	sc->sc_addr = dvma_kvtopa(sc->sc_mem, ca->ca_bustype);

	idprom_etheraddr(sc->sc_enaddr);

	sc->sc_copytodesc = am7990_copytobuf_contig;
	sc->sc_copyfromdesc = am7990_copyfrombuf_contig;
	sc->sc_copytobuf = am7990_copytobuf_contig;
	sc->sc_copyfrombuf = am7990_copyfrombuf_contig;
	sc->sc_zerobuf = am7990_zerobuf_contig;

	sc->sc_rdcsr = lerdcsr;
	sc->sc_wrcsr = lewrcsr;
	sc->sc_hwinit = NULL;
	sc->sc_conf3 = LE_C3_BSWP | LE_C3_ACON | LE_C3_BCON;

	am7990_config(sc);

	/* Install interrupt handler. */
	isr_add_autovect(am7990_intr, (void *)sc, ca->ca_intpri);
}
