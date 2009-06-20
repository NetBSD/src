/*	$NetBSD: if_le.c,v 1.42.4.2 2009/06/20 07:20:00 yamt Exp $ */

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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

/*-
 * Copyright (c) 1997 Bernd Ernesti.  All rights reserved.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell and Rick Macklem.
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
 *	This product includes software developed for the NetBSD Project
 *	by Bernd Ernesti.
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
 *	@(#)if_le.c	8.2 (Berkeley) 11/16/93
 */

#include "opt_inet.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_le.c,v 1.42.4.2 2009/06/20 07:20:00 yamt Exp $");

#include "bpfilter.h"

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

#include <machine/cpu.h>

#include <amiga/amiga/device.h>
#include <amiga/amiga/isr.h>

#include <dev/ic/lancereg.h>
#include <dev/ic/lancevar.h>
#include <dev/ic/am7990reg.h>
#include <dev/ic/am7990var.h>

#include <amiga/dev/zbusvar.h>
#include <amiga/dev/if_levar.h>

int le_zbus_match(device_t, cfdata_t, void *);
void le_zbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(le_zbus, sizeof(struct le_softc),
    le_zbus_match, le_zbus_attach, NULL, NULL);

#if defined(_KERNEL_OPT)
#include "opt_ddb.h"
#endif

#ifdef DDB
#define	integrate
#define hide
#else
#define	integrate	static inline
#define hide		static
#endif

hide void lepcnet_reset(struct lance_softc *);
hide void lewrcsr(struct lance_softc *, u_int16_t, u_int16_t);
hide u_int16_t lerdcsr(struct lance_softc *, u_int16_t);

hide u_int16_t ariadne_swapreg(u_int16_t);
hide void ariadne_wrcsr(struct lance_softc *, u_int16_t, u_int16_t);
hide u_int16_t ariadne_rdcsr(struct lance_softc *, u_int16_t);
hide void ariadne_wribcr(struct lance_softc *, u_int16_t, u_int16_t);
integrate void ariadne_copytodesc_word(struct lance_softc *, void *, int, int);
integrate void ariadne_copyfromdesc_word(struct lance_softc *, void *,
				int, int);
integrate void ariadne_copytobuf_word(struct lance_softc *, void *, int, int);
integrate void ariadne_copyfrombuf_word(struct lance_softc *, void *, int, int);
integrate void ariadne_zerobuf_word(struct lance_softc *, int, int);
void ariadne_autoselect(struct lance_softc *, int);
int ariadne_mediachange(struct lance_softc *);
void ariadne_hwinit(struct lance_softc *);

/*
 * Media types supported by the Ariadne.
 */
int lemedia_ariadne[] = {
	IFM_ETHER | IFM_10_T,
	IFM_ETHER | IFM_10_2,
	IFM_ETHER | IFM_AUTO,
};
#define NLEMEDIA_ARIADNE __arraycount(lemedia_ariadne)


hide u_int16_t
ariadne_swapreg(u_int16_t val)
{

	return (((val & 0xff) << 8 ) | (( val >> 8) & 0xff));
}

hide void
ariadne_wrcsr(struct lance_softc *sc, u_int16_t port, u_int16_t val)
{
	struct lereg1 *ler1 = ((struct le_softc *)sc)->sc_r1;

	ler1->ler1_rap = ariadne_swapreg(port);
	ler1->ler1_rdp = ariadne_swapreg(val);
}

hide u_int16_t
ariadne_rdcsr(struct lance_softc *sc, u_int16_t port)
{
	struct lereg1 *ler1 = ((struct le_softc *)sc)->sc_r1;
	u_int16_t val;

	ler1->ler1_rap = ariadne_swapreg(port);
	val = ariadne_swapreg(ler1->ler1_rdp);
	return (val);
}

hide void
ariadne_wribcr(struct lance_softc *sc, u_int16_t port, u_int16_t val)
{
	struct lereg1 *ler1 = ((struct le_softc *)sc)->sc_r1;

	ler1->ler1_rap = ariadne_swapreg(port);
	ler1->ler1_idp = ariadne_swapreg(val);
}

hide void
lewrcsr(struct lance_softc *sc, u_int16_t port, u_int16_t val)
{
	struct lereg1 *ler1 = ((struct le_softc *)sc)->sc_r1;

	ler1->ler1_rap = port;
	ler1->ler1_rdp = val;
}

hide u_int16_t
lerdcsr(struct lance_softc *sc, u_int16_t port)
{
	struct lereg1 *ler1 = ((struct le_softc *)sc)->sc_r1;
	u_int16_t val;

	ler1->ler1_rap = port;
	val = ler1->ler1_rdp;
	return (val);
}

hide void
lepcnet_reset(struct lance_softc *sc)
{
	struct lereg1 *ler1 = ((struct le_softc *)sc)->sc_r1;
	volatile int dummy;

	dummy = ler1->ler1_reset;	/* Reset PCNet-ISA */
}

void
ariadne_autoselect(struct lance_softc *sc, int on)
{

	/*
	 * on = 0: autoselect disabled
	 * on = 1: autoselect enabled
	 */
	if (on == 0)
		ariadne_wribcr(sc, LE_BCR_MC, 0x0000);
	else
		ariadne_wribcr(sc, LE_BCR_MC, LE_MC_ASEL);
}

int
ariadne_mediachange(struct lance_softc *sc)
{
	struct ifmedia *ifm = &sc->sc_media;

	if (IFM_TYPE(ifm->ifm_media) != IFM_ETHER)
		return (EINVAL);

	/*
	 * Switch to the selected media.  If autoselect is
	 * set, switch it on otherwise disable it.  We'll
	 * switch to the other media when we detect loss of
	 * carrier.
	 */
	switch (IFM_SUBTYPE(ifm->ifm_media)) {
	    case IFM_10_T:
		sc->sc_initmodemedia = 1;
		lance_init(&sc->sc_ethercom.ec_if);
		break;

	    case IFM_10_2:
		sc->sc_initmodemedia = 0;
		lance_init(&sc->sc_ethercom.ec_if);
		break;

	    case IFM_AUTO:
		sc->sc_initmodemedia = 2;
		ariadne_hwinit(sc);
		break;

	    default:
		return (EINVAL);
	}

	return (0);
}

void
ariadne_hwinit(struct lance_softc *sc)
{

	/*
	 * Re-program LEDs to match meaning used on the Ariadne board.
         */
	ariadne_wribcr(sc, LE_BCR_LED1, 0x0090);
	ariadne_wribcr(sc, LE_BCR_LED2, 0x0081);
	ariadne_wribcr(sc, LE_BCR_LED3, 0x0084);

	/*
	 * Enabel/Disable auto selection
	 */
	if (sc->sc_initmodemedia == 2)
		ariadne_autoselect(sc, 1);
	else
		ariadne_autoselect(sc, 0);
}

int
le_zbus_match(device_t parent, cfdata_t cfp, void *aux)
{
	struct zbus_args *zap = aux;

	/* Commodore ethernet card */
	if (zap->manid == 514 && zap->prodid == 112)
		return (1);

	/* Ameristar ethernet card */
	if (zap->manid == 1053 && zap->prodid == 1)
		return (1);

	/* Ariadne ethernet card */
	if (zap->manid == 2167 && zap->prodid == 201)
		return (1);

	return (0);
}

void
le_zbus_attach(device_t parent, device_t self, void *aux)
{
	struct le_softc *lesc = device_private(self);
	struct lance_softc *sc = &lesc->sc_am7990.lsc;
	struct zbus_args *zap = aux;
	u_long ser;

	sc->sc_dev = self;

	/* This has no effect on PCnet-ISA LANCE chips */
	sc->sc_conf3 = LE_C3_BSWP;

	/*
	 * Manufacturer decides the 3 first bytes, i.e. ethernet vendor ID.
	 */
	switch (zap->manid) {
	    case 514:
		/* Commodore */
		sc->sc_memsize = 32768;
		sc->sc_enaddr[0] = 0x00;
		sc->sc_enaddr[1] = 0x80;
		sc->sc_enaddr[2] = 0x10;
		lesc->sc_r1 = (struct lereg1 *)(0x4000 + (int)zap->va);
		sc->sc_mem = (void *)(0x8000 + (int)zap->va);
		sc->sc_addr = 0x8000;
		sc->sc_copytodesc = lance_copytobuf_contig;
		sc->sc_copyfromdesc = lance_copyfrombuf_contig;
		sc->sc_copytobuf = lance_copytobuf_contig;
		sc->sc_copyfrombuf = lance_copyfrombuf_contig;
		sc->sc_zerobuf = lance_zerobuf_contig;
		sc->sc_rdcsr = lerdcsr;
		sc->sc_wrcsr = lewrcsr;
		sc->sc_hwreset = NULL;
		sc->sc_hwinit = NULL;
		break;

	    case 1053:
		/* Ameristar */
		sc->sc_memsize = 32768;
		sc->sc_enaddr[0] = 0x00;
		sc->sc_enaddr[1] = 0x00;
		sc->sc_enaddr[2] = 0x9f;
		lesc->sc_r1 = (struct lereg1 *)(0x4000 + (int)zap->va);
		sc->sc_mem = (void *)(0x8000 + (int)zap->va);
		sc->sc_addr = 0x8000;
		sc->sc_copytodesc = lance_copytobuf_contig;
		sc->sc_copyfromdesc = lance_copyfrombuf_contig;
		sc->sc_copytobuf = lance_copytobuf_contig;
		sc->sc_copyfrombuf = lance_copyfrombuf_contig;
		sc->sc_zerobuf = lance_zerobuf_contig;
		sc->sc_rdcsr = lerdcsr;
		sc->sc_wrcsr = lewrcsr;
		sc->sc_hwreset = NULL;
		sc->sc_hwinit = NULL;
		break;

	    case 2167:
		/* Village Tronic */
		sc->sc_memsize = 32768;
		sc->sc_enaddr[0] = 0x00;
		sc->sc_enaddr[1] = 0x60;
		sc->sc_enaddr[2] = 0x30;
		lesc->sc_r1 = (struct lereg1 *)(0x0370 + (int)zap->va);
		sc->sc_mem = (void *)(0x8000 + (int)zap->va);
		sc->sc_addr = 0x8000;
		sc->sc_copytodesc = ariadne_copytodesc_word;
		sc->sc_copyfromdesc = ariadne_copyfromdesc_word;
		sc->sc_copytobuf = ariadne_copytobuf_word;
		sc->sc_copyfrombuf = ariadne_copyfrombuf_word;
		sc->sc_zerobuf = ariadne_zerobuf_word;
		sc->sc_rdcsr = ariadne_rdcsr;
		sc->sc_wrcsr = ariadne_wrcsr;
		sc->sc_hwreset = lepcnet_reset;
		sc->sc_hwinit = ariadne_hwinit;
		sc->sc_mediachange = ariadne_mediachange;
		sc->sc_supmedia = lemedia_ariadne;
		sc->sc_nsupmedia = NLEMEDIA_ARIADNE;
		sc->sc_defaultmedia = IFM_ETHER | IFM_AUTO;
		sc->sc_initmodemedia = 2;
		break;

	    default:
		panic("le_zbus_attach: bad manid");
	}

	/*
	 * Serial number for board is used as host ID.
	 */
	ser = (u_long)zap->serno;
	sc->sc_enaddr[3] = (ser >> 16) & 0xff;
	sc->sc_enaddr[4] = (ser >>  8) & 0xff;
	sc->sc_enaddr[5] = (ser      ) & 0xff;

	am7990_config(&lesc->sc_am7990);

	lesc->sc_isr.isr_intr = am7990_intr;
	lesc->sc_isr.isr_arg = sc;
	lesc->sc_isr.isr_ipl = 2;
	add_isr(&lesc->sc_isr);
}


integrate void
ariadne_copytodesc_word(struct lance_softc *sc, void *from, int boff, int len)
{
	u_short *b1 = from;
	volatile u_short *b2 = (u_short *)((u_char *)sc->sc_mem + boff);

	for (len >>= 1; len > 0; len--)
		*b2++ = ariadne_swapreg(*b1++);
}

integrate void
ariadne_copyfromdesc_word(struct lance_softc *sc, void *to, int boff, int len)
{
	volatile u_short *b1 = (u_short *)((u_char *)sc->sc_mem + boff);
	u_short *b2 = to;

	for (len >>= 1; len > 0; len--)
		*b2++ = ariadne_swapreg(*b1++);
}

#define	isodd(n)	((n) & 1)

integrate void
ariadne_copytobuf_word(struct lance_softc *sc, void *from, int boff, int len)
{
	u_char *a1 = from;
	volatile u_char *a2 = (u_char *)sc->sc_mem + boff;
	u_short *b1;
	volatile u_short *b2;
	int i;

	if (len > 0 && isodd(boff)) {
		/* adjust source pointer */
		b1 = (u_short *)(a1 + 1);
		/* compute aligned destination pointer */
		b2 = (volatile u_short *)(a2 + 1);
		/* copy first unaligned byte to buf */
		b2[-1] = (b2[-1] & 0xff00) | *a1;
		--len;
	} else {
		/* destination is aligned or length is zero */
		b1 = (u_short *)a1;
		b2 = (volatile u_short *)a2;
	}

	/* copy full words with aligned destination */
	for (i = len >> 1; i > 0; i--)
		*b2++ = *b1++;

	/* copy remaining byte */
	if (isodd(len))
		*b2 = (*b2 & 0x00ff) | (*(u_char *)b1) << 8;
}

integrate void
ariadne_copyfrombuf_word(struct lance_softc *sc, void *to, int boff, int len)
{
	volatile u_char *a1 = (u_char *)sc->sc_mem + boff;
	u_char *a2 = to;
	volatile u_short *b1;
	u_short *b2;
	int i;

	if (len > 0 && isodd(boff)) {
		/* compute aligned source pointer */
		b1  = (volatile u_short *)(a1 + 1);
		/* adjust destination pointer (possibly unaligned) */
		b2  = (u_short *)(a2 + 1);
		/* copy first unaligned byte from buf */
		*a2 = b1[-1];
		--len;
	} else {
		/* source is aligned or length is zero */
		b1 = (volatile u_short *)a1;
		b2 = (u_short *)a2;
	}

	/* copy full words with aligned source */
	for (i = len >> 1; i > 0; i--)
		*b2++ = *b1++;

	/* copy remaining byte */
	if (isodd(len))
		*(u_char *)b2 = *b1 >> 8;
}

integrate void
ariadne_zerobuf_word(struct lance_softc *sc, int boff, int len)
{
	volatile u_char *a1 = (u_char *)sc->sc_mem + boff;
	volatile u_short *b1;
	int i;

	if (len > 0 && isodd(boff)) {
		b1 = (volatile u_short *)(a1 + 1);
		b1[-1] &= 0xff00;
		--len;
	} else {
		b1 = (volatile u_short *)a1;
	}

	for (i = len >> 1; i > 0; i--)
		*b1++ = 0;

	if (isodd(len))
		*b1 &= 0x00ff;
}
