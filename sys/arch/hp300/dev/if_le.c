/*	$NetBSD: if_le.c,v 1.62 2010/01/19 22:06:20 pooka Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_le.c,v 1.62 2010/01/19 22:06:20 pooka Exp $");

#include "opt_inet.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/ic/lancereg.h>
#include <dev/ic/lancevar.h>
#include <dev/ic/am7990var.h>

#include <hp300/dev/diovar.h>
#include <hp300/dev/diodevs.h>
#include <hp300/dev/if_lereg.h>

#include "opt_useleds.h"

#ifdef USELEDS
#include <hp300/hp300/leds.h>
#endif

struct  le_softc {
	struct  am7990_softc sc_am7990; /* glue to MI code */

	bus_space_tag_t sc_bst;

	bus_space_handle_t sc_bsh0;	/* DIO registers */
	bus_space_handle_t sc_bsh1;	/* LANCE registers */
	bus_space_handle_t sc_bsh2;	/* buffer area */
};

static int	lematch(device_t, cfdata_t, void *);
static void	leattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(le, sizeof(struct le_softc),
    lematch, leattach, NULL, NULL);

static int	leintr(void *);

#if defined(_KERNEL_OPT)
#include "opt_ddb.h"
#endif

static void	le_copytobuf(struct lance_softc *, void *, int, int);
static void	le_copyfrombuf(struct lance_softc *, void *, int, int);
static void	le_zerobuf(struct lance_softc *, int, int);

/* offsets for:	   ID,   REGS,    MEM,  NVRAM */
static const int lestd[] = { 0, 0x4000, 0x8000, 0xC008 };

static void
lewrcsr(struct lance_softc *sc, uint16_t port, uint16_t val)
{
	struct le_softc *lesc = (struct le_softc *)sc;
	bus_space_tag_t bst = lesc->sc_bst;
	bus_space_handle_t bsh0 = lesc->sc_bsh0;
	bus_space_handle_t bsh1 = lesc->sc_bsh1;

	do {
		bus_space_write_2(bst, bsh1, LER1_RAP, port);
	} while ((bus_space_read_1(bst, bsh0, LER0_STATUS) & LE_ACK) == 0);
	do {
		bus_space_write_2(bst, bsh1, LER1_RDP, val);
	} while ((bus_space_read_1(bst, bsh0, LER0_STATUS) & LE_ACK) == 0);
}

static uint16_t
lerdcsr(struct lance_softc *sc, uint16_t port)
{
	struct le_softc *lesc = (struct le_softc *)sc;
	bus_space_tag_t bst = lesc->sc_bst;
	bus_space_handle_t bsh0 = lesc->sc_bsh0;
	bus_space_handle_t bsh1 = lesc->sc_bsh1;
	uint16_t val;

	do {
		bus_space_write_2(bst, bsh1, LER1_RAP, port);
	} while ((bus_space_read_1(bst, bsh0, LER0_STATUS) & LE_ACK) == 0);
	do {
		val = bus_space_read_2(bst, bsh1, LER1_RDP);
	} while ((bus_space_read_1(bst, bsh0, LER0_STATUS) & LE_ACK) == 0);

	return val;
}

static int
lematch(device_t parent, cfdata_t cf, void *aux)
{
	struct dio_attach_args *da = aux;

	if (da->da_id == DIO_DEVICE_ID_LAN)
		return 1;
	return 0;
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
static void
leattach(device_t parent, device_t self, void *aux)
{
	struct le_softc *lesc = device_private(self);
	struct lance_softc *sc = &lesc->sc_am7990.lsc;
	struct dio_attach_args *da = aux;
	bus_space_tag_t bst = da->da_bst;
	bus_space_handle_t bsh0, bsh1, bsh2;
	bus_size_t offset;
	int i;

	sc->sc_dev = self;

	if (bus_space_map(bst, (bus_addr_t)dio_scodetopa(da->da_scode),
	    da->da_size, 0, &bsh0)) {
		aprint_error(": can't map LANCE registers\n");
		return;
	}

	if (bus_space_subregion(bst, bsh0, lestd[1], LER1_SIZE, &bsh1)) {
		aprint_error(": can't subregion LANCE space\n");
		return;
	}

	if (bus_space_subregion(bst, bsh0, lestd[2], LE_BUFSIZE, &bsh2)) {
		aprint_error(": can't subregion buffer space\n");
		return;
	}

	lesc->sc_bst = bst;
	lesc->sc_bsh0 = bsh0;
	lesc->sc_bsh1 = bsh1;
	lesc->sc_bsh2 = bsh2;

	bus_space_write_1(bst, bsh0, LER0_ID, 0xff);
	DELAY(100);

	sc->sc_conf3 = LE_C3_BSWP;
	sc->sc_addr = 0;
	sc->sc_memsize = LE_BUFSIZE;

	/*
	 * Read the ethernet address off the board, one nibble at a time.
	 */
	offset = lestd[3];
	for (i = 0; i < sizeof(sc->sc_enaddr); i++) {
		sc->sc_enaddr[i] =
		    (bus_space_read_1(bst, bsh0, ++offset) & 0xf) << 4;
		offset++;
		sc->sc_enaddr[i] |=
		    (bus_space_read_1(bst, bsh0, ++offset) & 0xf);
		offset++;
	}

	sc->sc_copytodesc = le_copytobuf;
	sc->sc_copyfromdesc = le_copyfrombuf;
	sc->sc_copytobuf = le_copytobuf;
	sc->sc_copyfrombuf = le_copyfrombuf;
	sc->sc_zerobuf = le_zerobuf;

	sc->sc_rdcsr = lerdcsr;
	sc->sc_wrcsr = lewrcsr;
	sc->sc_hwinit = NULL;

	am7990_config(&lesc->sc_am7990);

	/* Establish the interrupt handler. */
	(void) dio_intr_establish(leintr, sc, da->da_ipl, IPL_NET);
	bus_space_write_1(bst, bsh0, LER0_STATUS, LE_IE);
}

static int
leintr(void *arg)
{
	struct lance_softc *sc = arg;
#ifdef USELEDS
	uint16_t isr;

	isr = lerdcsr(sc, LE_CSR0);

	if ((isr & LE_C0_INTR) == 0)
		return 0;

	if (isr & LE_C0_RINT)
		ledcontrol(0, 0, LED_LANRCV);

	if (isr & LE_C0_TINT)
		ledcontrol(0, 0, LED_LANXMT);
#endif /* USELEDS */

	return am7990_intr(sc);
}

static void
le_copytobuf(struct lance_softc *sc, void *from, int boff, int len)
{
	struct le_softc *lesc = (struct le_softc *)sc;

	bus_space_write_region_1(lesc->sc_bst, lesc->sc_bsh2, boff, from, len);
}

static void
le_copyfrombuf(struct lance_softc *sc, void *to, int boff, int len)
{
	struct le_softc *lesc = (struct le_softc *)sc;

	bus_space_read_region_1(lesc->sc_bst, lesc->sc_bsh2, boff, to, len);
}

static void
le_zerobuf(struct lance_softc *sc, int boff, int len)
{
	struct le_softc *lesc = (struct le_softc *)sc;

	bus_space_set_region_1(lesc->sc_bst, lesc->sc_bsh2, boff, 0, len);
}
