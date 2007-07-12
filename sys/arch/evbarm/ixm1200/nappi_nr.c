/* $NetBSD: nappi_nr.c,v 1.6 2007/07/12 22:02:38 he Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: nappi_nr.c,v 1.6 2007/07/12 22:02:38 he Exp $");

/*
 * LED support for NAPPI.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include <arm/ixp12x0/ixpsipvar.h>

static int	nappinr_match(struct device *, struct cfdata *, void *);
static void	nappinr_attach(struct device *, struct device *, void *);
#if 0
static int	nappinr_activate(struct device *, enum devact);
#endif
static void	nappinr_callout(void *);

struct nappinr_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_addr_t		sc_baseaddr;
	bus_space_handle_t	sc_pos;
	struct callout		sc_co;
};

CFATTACH_DECL(nappinr, sizeof(struct nappinr_softc),
    nappinr_match, nappinr_attach, NULL, NULL);

static int
nappinr_match(struct device *parent, struct cfdata *match, void *aux)
{
	return (1);
}

static void
nappinr_attach(struct device *parent, struct device *self, void *aux)
{
	struct nappinr_softc*		sc = (struct nappinr_softc*) self;
	struct ixpsip_attach_args*	sa = aux;

	printf("\n");

  	sc->sc_iot = sa->sa_iot;
  	sc->sc_baseaddr = sa->sa_addr;

	if(bus_space_map(sa->sa_iot, sa->sa_addr, sa->sa_size, 0,
			 &sc->sc_ioh)) {
		printf("%s: unable to map registers\n", self->dv_xname);
		return;
	}

	callout_init(&sc->sc_co, 0);
	callout_reset(&sc->sc_co, hz / 10, nappinr_callout, sc);
}

#if 0
static int
nappinr_activate(struct device *self, enum devact act)
{
	printf("nappinr_activate act=%d\n", act);
	return 0;
}
#endif

static void
nappinr_callout(void *arg)
{
	static const int	ptn[] = { 1, 2, 4, 8, 4, 2 };
	struct nappinr_softc*	sc = arg;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, 0,
			  ptn[sc->sc_pos++ % 6] | ptn[sc->sc_pos++ % 6]<< 4);
	callout_reset(&sc->sc_co, hz / 10, nappinr_callout, sc);
}
