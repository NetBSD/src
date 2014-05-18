/* $NetBSD: if_le.c,v 1.6.26.1 2014/05/18 17:45:15 rmind Exp $ */

/*-
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
 * 3. Neither the name of the University nor the names of its contributors
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

/*-
 * Copyright (c) 1995 Charles M. Hannum.  All rights reserved.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: if_le.c,v 1.6.26.1 2014/05/18 17:45:15 rmind Exp $");

#include "opt_inet.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
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
#include <machine/autoconf.h>

#include <luna68k/luna68k/isr.h>

#include <dev/ic/lancereg.h>
#include <dev/ic/lancevar.h>
#include <dev/ic/am7990reg.h>
#include <dev/ic/am7990var.h>

#include "ioconf.h"

/*
 * LANCE registers.
 */
struct lereg1 {
	volatile uint16_t	ler1_rdp;	/* data port */
	volatile uint16_t	ler1_rap;	/* register select port */
};

/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * arpcom.ac_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 */
struct	le_softc {
	struct	am7990_softc sc_am7990;	/* glue to MI code */

	struct	lereg1 *sc_r1;		/* LANCE registers */
};

static int  le_match(device_t, cfdata_t, void *);
static void le_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(le, sizeof(struct le_softc),
    le_match, le_attach, NULL, NULL);

static void lesrcsr(struct lance_softc *, uint16_t, uint16_t);
static uint16_t lerdcsr(struct lance_softc *, uint16_t);
static void myetheraddr(uint8_t *);

static void
lesrcsr(struct lance_softc *sc, uint16_t port, uint16_t val)
{
	struct lereg1 *ler1 = ((struct le_softc *)sc)->sc_r1;

	ler1->ler1_rap = port;
	ler1->ler1_rdp = val;
}

static uint16_t
lerdcsr(struct lance_softc *sc, uint16_t port)
{
	struct lereg1 *ler1 = ((struct le_softc *)sc)->sc_r1;
	uint16_t val;

	ler1->ler1_rap = port;
	val = ler1->ler1_rdp;
	return (val);
}

static int
le_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, le_cd.cd_name))
		return (0);

	return (1);
}

void
le_attach(device_t parent, device_t self, void *aux)
{
	struct le_softc *lesc = device_private(self);
	struct lance_softc *sc = &lesc->sc_am7990.lsc;
	struct mainbus_attach_args *ma = aux;

	sc->sc_dev = self;

	/* Map control registers. */
	lesc->sc_r1 = (struct lereg1 *)ma->ma_addr;	/* LANCE */

	sc->sc_mem = (void *)0x71010000;		/* SRAM */
	sc->sc_conf3 = LE_C3_BSWP;
	sc->sc_addr = (u_long)sc->sc_mem & 0xffffff;
	sc->sc_memsize = 64 * 1024;			/* 64KB */

	myetheraddr(sc->sc_enaddr);

	sc->sc_copytodesc = lance_copytobuf_contig;
	sc->sc_copyfromdesc = lance_copyfrombuf_contig;
	sc->sc_copytobuf = lance_copytobuf_contig;
	sc->sc_copyfrombuf = lance_copyfrombuf_contig;
	sc->sc_zerobuf = lance_zerobuf_contig;

	sc->sc_rdcsr = lerdcsr;
	sc->sc_wrcsr = lesrcsr;
	sc->sc_hwinit = NULL;

	am7990_config(&lesc->sc_am7990);

	isrlink_autovec(am7990_intr, (void *)sc, ma->ma_ilvl, ISRPRI_NET);
}

/*
 * LUNA-I has 1Mbit CPU ROM, which contains MAC address directly
 * readable at 0x4100FFE0 as a sequence of 12 ASCII characters.
 *
 * LUNA-II has 16Kbit NVSRAM on its ethercard, whose contents are
 * accessible 4bit-wise by ctl register operation.  The register is
 * mapped at 0xF1000004.
 */
void
myetheraddr(uint8_t *ether)
{
	unsigned int i, loc;
	uint8_t *ea;
	volatile struct { uint32_t ctl; } *ds1220;

	switch (machtype) {
	case LUNA_I:
		ea = (uint8_t *)0x4101FFE0;
		for (i = 0; i < ETHER_ADDR_LEN; i++) {
			int u, l;

			u = ea[0];
			u = (u < 'A') ? u & 0xf : u - 'A' + 10;
			l = ea[1];
			l = (l < 'A') ? l & 0xf : l - 'A' + 10;

			ether[i] = l | (u << 4);
			ea += 2;
		}
		break;
	case LUNA_II:
		ds1220 = (void *)0xF1000004;
		loc = 12;
		for (i = 0; i < ETHER_ADDR_LEN; i++) {
			unsigned int u, l, hex;

			ds1220->ctl = (loc) << 16;
			u = 0xf0 & (ds1220->ctl >> 12);
			ds1220->ctl = (loc + 1) << 16;
			l = 0x0f & (ds1220->ctl >> 16);
			hex = (u < '9') ? l : l + 9;

			ds1220->ctl = (loc + 2) << 16;
			u = 0xf0 & (ds1220->ctl >> 12);
			ds1220->ctl = (loc + 3) << 16;
			l = 0x0f & (ds1220->ctl >> 16);

			ether[i] = ((u < '9') ? l : l + 9) | (hex << 4);
			loc += 4;
		}
		break;
	default:
		ether[0] = 0x00; ether[1] = 0x00; ether[2] = 0x0a;
		ether[3] = 0xDE; ether[4] = 0xAD; ether[5] = 0x00;
		break;
	}
}
