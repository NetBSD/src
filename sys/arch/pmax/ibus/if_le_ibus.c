/*	$NetBSD: if_le_ibus.c,v 1.1.2.3 1999/11/19 11:06:24 nisimura Exp $	*/

/*
 * Copyright 1996 The Board of Trustees of The Leland Stanford
 * Junior University. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  Stanford University
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * This driver was contributed by Jonathan Stone.
 */

/*
 * LANCE on DECstation 3100 and DECsystem 5100 baseboard.
 */
#include "opt_inet.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#include <dev/ic/lancereg.h>
#include <dev/ic/lancevar.h>
#include <dev/ic/am7990reg.h>
#include <dev/ic/am7990var.h>

#include <dev/tc/if_levar.h>
#include <pmax/ibus/ibusvar.h>
#include <pmax/pmax/kn01.h>

extern void le_dec_copytobuf_gap2 __P((struct lance_softc *, void *,
	    int, int));
extern void le_dec_copyfrombuf_gap2 __P((struct lance_softc *, void *,
	    int, int));

void le_dec_zerobuf_gap2 __P((struct lance_softc *, int, int));

int	le_pmax_match __P((struct device *, struct cfdata *, void *));
void	le_pmax_attach __P((struct device *, struct device *, void *));

struct cfattach le_pmax_ca = {
	sizeof(struct le_softc), le_pmax_match, le_pmax_attach
};
extern struct cfdriver ibus_cd;

int
le_pmax_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
  	struct ibus_attach_args *d = aux;

	if (parent->dv_cfdata->cf_driver != &ibus_cd)
		return (0);
	if (strcmp("lance", d->ia_name) != 0)
		return (0);
	return (1);
}

void
le_pmax_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct le_softc *lesc = (void *)self;
	struct lance_softc *sc = &lesc->sc_am7990.lsc;
	u_char *cp;
	struct ibus_attach_args *ia = aux;

	/*
	 * It's on the baseboard, with a dedicated interrupt line.
	 */
	lesc->sc_r1 = (struct lereg1 *)(ia->ia_addr);
/*XXX*/	sc->sc_mem = (void *)MIPS_PHYS_TO_KSEG1(0x19000000);
/*XXX*/	cp = (u_char *)(MIPS_PHYS_TO_KSEG1(KN01_SYS_CLOCK) + 1);

	sc->sc_copytodesc = le_dec_copytobuf_gap2;
	sc->sc_copyfromdesc = le_dec_copyfrombuf_gap2;
	sc->sc_copytobuf = le_dec_copytobuf_gap2;
	sc->sc_copyfrombuf = le_dec_copyfrombuf_gap2;
	sc->sc_zerobuf = le_dec_zerobuf_gap2;

	dec_le_common_attach(&lesc->sc_am7990, cp);

	ibus_intr_establish(parent, ia->ia_cookie, IPL_NET, am7990_intr, sc);
}

/*
 * gap2: two bytes of data followed by two bytes of pad.
 *
 * Buffers must be 4-byte aligned.  The code doesn't worry about
 * doing an extra byte.
 */

void
le_dec_copytobuf_gap2(sc, fromv, boff, len)
	struct lance_softc *sc;  
	void *fromv;
	int boff;
	register int len;
{
	volatile caddr_t buf = sc->sc_mem;
	register caddr_t from = fromv;
	register volatile u_int16_t *bptr;  

	if (boff & 0x1) {
		/* handle unaligned first byte */
		bptr = ((volatile u_int16_t *)buf) + (boff - 1);
		*bptr = (*from++ << 8) | (*bptr & 0xff);
		bptr += 2;  
		len--;
	} else
		bptr = ((volatile u_int16_t *)buf) + boff;
	while (len > 1) {
		*bptr = (from[1] << 8) | (from[0] & 0xff);
		bptr += 2;
		from += 2;
		len -= 2;
	}
	if (len == 1)
		*bptr = (u_int16_t)*from;
}

void
le_dec_copyfrombuf_gap2(sc, tov, boff, len)
	struct lance_softc *sc;
	void *tov;
	int boff, len;
{
	volatile caddr_t buf = sc->sc_mem;
	register caddr_t to = tov;
	register volatile u_int16_t *bptr;
	register u_int16_t tmp;

	if (boff & 0x1) {
		/* handle unaligned first byte */
		bptr = ((volatile u_int16_t *)buf) + (boff - 1);
		*to++ = (*bptr >> 8) & 0xff;
		bptr += 2;
		len--;
	} else
		bptr = ((volatile u_int16_t *)buf) + boff;
	while (len > 1) {
		tmp = *bptr;
		*to++ = tmp & 0xff;
		*to++ = (tmp >> 8) & 0xff;
		bptr += 2;
		len -= 2;
	}
	if (len == 1)
		*to = *bptr & 0xff;
}

void
le_dec_zerobuf_gap2(sc, boff, len)
	struct lance_softc *sc;
	int boff, len;
{
	volatile caddr_t buf = sc->sc_mem;
	register volatile u_int16_t *bptr;

	if ((unsigned)boff & 0x1) {
		bptr = ((volatile u_int16_t *)buf) + (boff - 1);
		*bptr &= 0xff;
		bptr += 2;
		len--;
	} else
		bptr = ((volatile u_int16_t *)buf) + boff;
	while (len > 0) {
		*bptr = 0;
		bptr += 2;
		len -= 2;
	}
}
