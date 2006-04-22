/*	$NetBSD: j720ssp.c,v 1.28.6.1 2006/04/22 11:37:28 simonb Exp $	*/

/*-
 * Copyright (c) 2001, 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro. 
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

/* Jornada 720 SSP port. */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: j720ssp.c,v 1.28.6.1 2006/04/22 11:37:28 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <arm/sa11x0/sa11x0_var.h>
#include <arm/sa11x0/sa11x0_gpioreg.h>
#include <arm/sa11x0/sa11x0_ppcreg.h>
#include <arm/sa11x0/sa11x0_sspreg.h>

#include <hpcarm/dev/j720sspvar.h>

#ifdef DEBUG
#define DPRINTF(arg)	printf arg
#else
#define DPRINTF(arg)	/* nothing */
#endif

static int	j720ssp_match(struct device *, struct cfdata *, void *);
static void	j720ssp_attach(struct device *, struct device *, void *);
static int	j720ssp_search(struct device *, struct cfdata *,
		    const int *, void *);
static int	j720ssp_print(void *, const char *);

CFATTACH_DECL(j720ssp, sizeof(struct j720ssp_softc),
    j720ssp_match, j720ssp_attach, NULL, NULL);


static int
j720ssp_match(struct device *parent, struct cfdata *cf, void *aux)
{

	if (strcmp(cf->cf_name, "j720ssp") != 0)
		return 0;

	return 1;
}

static void
j720ssp_attach(struct device *parent, struct device *self, void *aux)
{
	struct j720ssp_softc *sc = (void *)self;
	struct sa11x0_softc *psc = (void *)parent;
	struct sa11x0_attach_args *sa = aux;

	sc->sc_iot = psc->sc_iot;
	sc->sc_gpioh = psc->sc_gpioh;
	sc->sc_parent = psc;

	if (bus_space_map(sc->sc_iot, sa->sa_addr, sa->sa_size, 0,
	    &sc->sc_ssph)) {
		printf(": unable to map SSP registers\n");
		return;
	}

	printf("\n");

	config_search_ia(j720ssp_search, self, "j720ssp", NULL);
}

static int
j720ssp_search(struct device *parent, struct cfdata *cf,
    const int *ldesc, void *aux)
{

	if (config_match(parent, cf, NULL) > 0)
		config_attach(parent, cf, NULL, j720ssp_print);

	return 0;
}

static int
j720ssp_print(void *aux, const char *pnp)
{

	return pnp ? QUIET : UNCONF;
}

int
j720ssp_readwrite(struct j720ssp_softc *sc, int drainfifo, int in,
    int *out, int wait)
{
	int timeout;

	while (!(bus_space_read_4(sc->sc_iot, sc->sc_ssph, SASSP_SR) & SR_TNF))
		continue;

	timeout = 400000;
	while (bus_space_read_4(sc->sc_iot, sc->sc_gpioh, SAGPIO_PLR) & 0x400)
		if (--timeout == 0) {
			DPRINTF(("j720ssp_readwrite: timeout 0\n"));
			return -1;
		}
	if (drainfifo) {
		while (bus_space_read_4(sc->sc_iot, sc->sc_ssph, SASSP_SR) &
		      SR_RNE)
			bus_space_read_4(sc->sc_iot, sc->sc_ssph, SASSP_DR);
		delay(wait);
	}

	bus_space_write_4(sc->sc_iot, sc->sc_ssph, SASSP_DR, in);

	delay(wait);
	timeout = 100000;
	while (!(bus_space_read_4(sc->sc_iot, sc->sc_ssph, SASSP_SR) & SR_RNE))
		if (--timeout == 0) {
			DPRINTF(("j720ssp_readwrite: timeout 1\n"));
			return -1;
		}

	*out = bus_space_read_4(sc->sc_iot, sc->sc_ssph, SASSP_DR);

	return 0;
}
