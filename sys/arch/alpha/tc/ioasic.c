/* $NetBSD: ioasic.c,v 1.44.2.1 2012/04/17 00:05:58 yamt Exp $ */

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

/*
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Keith Bostic, Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include "opt_dec_3000_300.h"

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: ioasic.c,v 1.44.2.1 2012/04/17 00:05:58 yamt Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <sys/bus.h>
#include <machine/pte.h>
#include <machine/rpb.h>

#include <dev/tc/tcvar.h>
#include <dev/tc/ioasicreg.h>
#include <dev/tc/ioasicvar.h>

/* Definition of the driver for autoconfig. */
int	ioasicmatch(device_t, cfdata_t, void *);
void	ioasicattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ioasic, sizeof(struct ioasic_softc),
    ioasicmatch, ioasicattach, NULL, NULL);

int	ioasic_intr(void *);
int	ioasic_intrnull(void *);

#define	C(x)	((void *)(x))

#define	IOASIC_DEV_LANCE	0
#define	IOASIC_DEV_SCC0		1
#define	IOASIC_DEV_SCC1		2
#define	IOASIC_DEV_ISDN		3

#define	IOASIC_DEV_BOGUS	-1

#define	IOASIC_NCOOKIES		4

struct ioasic_dev ioasic_devs[] = {
	{ "PMAD-BA ", IOASIC_SLOT_3_START, C(IOASIC_DEV_LANCE),
	  IOASIC_INTR_LANCE, },
	{ "z8530   ", IOASIC_SLOT_4_START, C(IOASIC_DEV_SCC0),
	  IOASIC_INTR_SCC_0, },
	{ "z8530   ", IOASIC_SLOT_6_START, C(IOASIC_DEV_SCC1),
	  IOASIC_INTR_SCC_1, },
	{ "TOY_RTC ", IOASIC_SLOT_8_START, C(IOASIC_DEV_BOGUS),
	  0, },
	{ "AMD79c30", IOASIC_SLOT_9_START, C(IOASIC_DEV_ISDN),
	  IOASIC_INTR_ISDN_TXLOAD | IOASIC_INTR_ISDN_RXLOAD,  },
};
int ioasic_ndevs = sizeof(ioasic_devs) / sizeof(ioasic_devs[0]);

struct ioasicintr {
	int	(*iai_func)(void *);
	void	*iai_arg;
	struct evcnt iai_evcnt;
} ioasicintrs[IOASIC_NCOOKIES];

tc_addr_t ioasic_base;		/* XXX XXX XXX */

/* There can be only one. */
int ioasicfound;

int
ioasicmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct tc_attach_args *ta = aux;

	/* Make sure that we're looking for this type of device. */
	if (strncmp("FLAMG-IO", ta->ta_modname, TC_ROM_LLEN))
		return (0);

	/* Check that it can actually exist. */
	if ((cputype != ST_DEC_3000_500) && (cputype != ST_DEC_3000_300))
		panic("ioasicmatch: how did we get here?");

	if (ioasicfound)
		return (0);

	return (1);
}

void
ioasicattach(device_t parent, device_t self, void *aux)
{
	struct ioasic_softc *sc = device_private(self);
	struct tc_attach_args *ta = aux;
#ifdef DEC_3000_300
	u_long ssr;
#endif
	u_long i, imsk;
	const struct evcnt *pevcnt;
	char *cp;

	ioasicfound = 1;

	sc->sc_dev = self;
	sc->sc_bst = ta->ta_memt;
	if (bus_space_map(ta->ta_memt, ta->ta_addr,
			0x400000, 0, &sc->sc_bsh)) {
		printf("%s: unable to map device\n", device_xname(self));
		return;
	}
	sc->sc_dmat = ta->ta_dmat;

	ioasic_base = sc->sc_base = ta->ta_addr; /* XXX XXX XXX */

#ifdef DEC_3000_300
	if (cputype == ST_DEC_3000_300) {
		ssr = bus_space_read_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR);
		ssr |= IOASIC_CSR_FASTMODE;
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR, ssr);
		printf(": slow mode\n");
	} else
#endif
		printf(": fast mode\n");

	/*
	 * Turn off all device interrupt bits.
	 * (This does _not_ include 3000/300 TC option slot bits.
	 */
	imsk = bus_space_read_4(sc->sc_bst, sc->sc_bsh, IOASIC_IMSK);
	for (i = 0; i < ioasic_ndevs; i++)
		imsk &= ~ioasic_devs[i].iad_intrbits;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_IMSK, imsk);

	/*
	 * Set up interrupt handlers.
	 */
	pevcnt = tc_intr_evcnt(parent, ta->ta_cookie);
	for (i = 0; i < IOASIC_NCOOKIES; i++) {
		ioasicintrs[i].iai_func = ioasic_intrnull;
		ioasicintrs[i].iai_arg = (void *)i;

		cp = malloc(12, M_DEVBUF, M_NOWAIT);
		if (cp == NULL)
			panic("ioasicattach");
		sprintf(cp, "slot %lu", i);
		evcnt_attach_dynamic(&ioasicintrs[i].iai_evcnt,
		    EVCNT_TYPE_INTR, pevcnt, device_xname(self), cp);
	}
	tc_intr_establish(parent, ta->ta_cookie, TC_IPL_NONE, ioasic_intr, sc);

	/*
	 * Try to configure each device.
	 */
	ioasic_attach_devs(sc, ioasic_devs, ioasic_ndevs);
}

void
ioasic_intr_establish(device_t ioa, void *cookie, tc_intrlevel_t level,
		int (*func)(void *), void *arg)
{
	struct ioasic_softc *sc = device_lookup_private(&ioasic_cd,0);
	u_long dev, i, imsk;

	dev = (u_long)cookie;
#ifdef DIAGNOSTIC
	/* XXX check cookie. */
#endif

	if (ioasicintrs[dev].iai_func != ioasic_intrnull)
		panic("ioasic_intr_establish: cookie %lu twice", dev);

	ioasicintrs[dev].iai_func = func;
	ioasicintrs[dev].iai_arg = arg;

	/* Enable interrupts for the device. */
	for (i = 0; i < ioasic_ndevs; i++)
		if (ioasic_devs[i].iad_cookie == cookie)
			break;
	if (i == ioasic_ndevs)
		panic("ioasic_intr_establish: invalid cookie.");

	imsk = bus_space_read_4(sc->sc_bst, sc->sc_bsh, IOASIC_IMSK);
	imsk |= ioasic_devs[i].iad_intrbits;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_IMSK, imsk);
}

void
ioasic_intr_disestablish(device_t ioa, void *cookie)
{
	struct ioasic_softc *sc = device_lookup_private(&ioasic_cd,0);
	u_long dev, i, imsk;

	dev = (u_long)cookie;
#ifdef DIAGNOSTIC
	/* XXX check cookie. */
#endif

	if (ioasicintrs[dev].iai_func == ioasic_intrnull)
		panic("ioasic_intr_disestablish: cookie %lu missing intr", dev);

	/* Enable interrupts for the device. */
	for (i = 0; i < ioasic_ndevs; i++)
		if (ioasic_devs[i].iad_cookie == cookie)
			break;
	if (i == ioasic_ndevs)
		panic("ioasic_intr_disestablish: invalid cookie.");

	imsk = bus_space_read_4(sc->sc_bst, sc->sc_bsh, IOASIC_IMSK);
	imsk &= ~ioasic_devs[i].iad_intrbits;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_IMSK, imsk);

	ioasicintrs[dev].iai_func = ioasic_intrnull;
	ioasicintrs[dev].iai_arg = (void *)dev;
}

int
ioasic_intrnull(void *val)
{

	panic("ioasic_intrnull: uncaught IOASIC intr for cookie %ld",
	    (u_long)val);
}

/*
 * ASIC interrupt handler.
 */
int
ioasic_intr(void *val)
{
	register struct ioasic_softc *sc = val;
	register int ifound;
	int gifound;
	uint32_t sir, osir;

	gifound = 0;
	do {
		ifound = 0;
		tc_syncbus();

		osir = sir =
		    bus_space_read_4(sc->sc_bst, sc->sc_bsh, IOASIC_INTR);

#define	INCRINTRCNT(slot)	ioasicintrs[slot].iai_evcnt.ev_count++

		/* XXX DUPLICATION OF INTERRUPT BIT INFORMATION... */
#define	CHECKINTR(slot, bits, clear)					\
		if (sir & (bits)) {					\
			ifound = 1;					\
			INCRINTRCNT(slot);				\
			(*ioasicintrs[slot].iai_func)			\
			    (ioasicintrs[slot].iai_arg);		\
			if (clear)					\
				sir &= ~(bits);				\
		}
		CHECKINTR(IOASIC_DEV_SCC0, IOASIC_INTR_SCC_0, 0);
		CHECKINTR(IOASIC_DEV_SCC1, IOASIC_INTR_SCC_1, 0);
		CHECKINTR(IOASIC_DEV_LANCE, IOASIC_INTR_LANCE, 0);
		CHECKINTR(IOASIC_DEV_ISDN, IOASIC_INTR_ISDN_TXLOAD |
		    IOASIC_INTR_ISDN_RXLOAD | IOASIC_INTR_ISDN_OVRUN, 1);

		if (sir != osir)
			bus_space_write_4(sc->sc_bst, sc->sc_bsh,
			    IOASIC_INTR, sir);

		gifound |= ifound;
	} while (ifound);

	return (gifound);
}
