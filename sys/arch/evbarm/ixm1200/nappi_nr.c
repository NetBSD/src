/* $NetBSD: nappi_nr.c,v 1.10.2.1 2012/11/20 03:01:15 tls Exp $ */

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ichiro FUKUHARA and Naoto Shimazaki.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nappi_nr.c,v 1.10.2.1 2012/11/20 03:01:15 tls Exp $");

/*
 * LED support for NAPPI.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <sys/bus.h>

#include <arm/ixp12x0/ixpsipvar.h>

static int	nappinr_match(device_t, cfdata_t, void *);
static void	nappinr_attach(device_t, device_t, void *);
#if 0
static int	nappinr_activate(device_t, enum devact);
#endif
static void	nappinr_callout(void *);

struct nappinr_softc {
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_addr_t		sc_baseaddr;
	bus_space_handle_t	sc_pos;
	struct callout		sc_co;
};

CFATTACH_DECL_NEW(nappinr, sizeof(struct nappinr_softc),
    nappinr_match, nappinr_attach, NULL, NULL);

static int
nappinr_match(device_t parent, cfdata_t match, void *aux)
{
	return (1);
}

static void
nappinr_attach(device_t parent, device_t self, void *aux)
{
	struct nappinr_softc*		sc = device_private(self);
	struct ixpsip_attach_args*	sa = aux;

	printf("\n");

  	sc->sc_iot = sa->sa_iot;
  	sc->sc_baseaddr = sa->sa_addr;

	if (bus_space_map(sa->sa_iot, sa->sa_addr, sa->sa_size, 0,
			  &sc->sc_ioh)) {
		printf("%s: unable to map registers\n", device_xname(self));
		return;
	}

	callout_init(&sc->sc_co, 0);
	callout_reset(&sc->sc_co, hz / 10, nappinr_callout, sc);
}

#if 0
static int
nappinr_activate(device_t self, enum devact act)
{
	printf("nappinr_activate act=%d\n", act);
	return 0;
}
#endif

static void
nappinr_callout(void *arg)
{
	static const int	ptn[] = { 1, 2, 4, 8, 4, 2 };
	struct nappinr_softc	*sc = arg;
	uint32_t v;

	v = ptn[sc->sc_pos++ % 6];
	v |= ptn[sc->sc_pos++ % 6] << 4;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, 0, v);
	callout_reset(&sc->sc_co, hz / 10, nappinr_callout, sc);
}
