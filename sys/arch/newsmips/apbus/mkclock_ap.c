/*	$NetBSD: mkclock_ap.c,v 1.3.6.1 2008/01/21 09:37:57 yamt Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mkclock_ap.c,v 1.3.6.1 2008/01/21 09:37:57 yamt Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <dev/clock_subr.h>
#include <dev/ic/mk48txxreg.h>
#include <dev/ic/mk48txxvar.h>

#include <newsmips/apbus/apbusvar.h>

#define MKCLOCK_AP_STRIDE	2
#define MKCLOCK_AP_OFFSET	\
    ((MK48T02_CLKOFF + MK48TXX_ICSR) << MKCLOCK_AP_STRIDE)

int  mkclock_ap_match(struct device *, struct cfdata  *, void *);
void mkclock_ap_attach(struct device *, struct device *, void *);
static uint8_t mkclock_ap_nvrd(struct mk48txx_softc *, int);
static void mkclock_ap_nvwr(struct mk48txx_softc *, int, uint8_t);

CFATTACH_DECL(mkclock_ap, sizeof(struct mk48txx_softc),
    mkclock_ap_match, mkclock_ap_attach, NULL, NULL);

extern struct cfdriver mkclock_cd;

int
mkclock_ap_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct apbus_attach_args *apa = aux;

	if (strcmp("clock", apa->apa_name) != 0)
		return 0;

	return 1;
}

void
mkclock_ap_attach(struct device *parent, struct device *self, void *aux)
{
	struct mk48txx_softc *sc = (void *)self;
	struct apbus_attach_args *apa = aux;

	printf(" slot%d addr 0x%lx", apa->apa_slotno, apa->apa_hwbase);
	if (bus_space_map(sc->sc_bst, apa->apa_hwbase - MKCLOCK_AP_OFFSET,
	    MK48T02_CLKSZ, 0, &sc->sc_bsh) != 0)
		printf("can't map device space\n");

	sc->sc_model = "mk48t02";
	sc->sc_year0 = 1900;
	sc->sc_nvrd = mkclock_ap_nvrd;
	sc->sc_nvwr = mkclock_ap_nvwr;

	mk48txx_attach(sc);

	printf("\n");
}

static uint8_t
mkclock_ap_nvrd(struct mk48txx_softc *sc, int off)
{
	uint8_t rv;

	rv = bus_space_read_4(sc->sc_bst, sc->sc_bsh, off << MKCLOCK_AP_STRIDE);
	return rv;
}

static void
mkclock_ap_nvwr(struct mk48txx_softc *sc, int off, uint8_t v)
{

	bus_space_write_4(sc->sc_bst, sc->sc_bsh, off << MKCLOCK_AP_STRIDE, v);
}
