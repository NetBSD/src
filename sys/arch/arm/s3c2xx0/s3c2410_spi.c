/* $NetBSD: s3c2410_spi.c,v 1.2 2005/06/30 17:03:52 drochner Exp $ */

/*
 * Copyright (c) 2004  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec Corporation may not be used to endorse or 
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Support S3C2410's SPI dirver.
 * Real works are done by drivers attached to SPI ports.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: s3c2410_spi.c,v 1.2 2005/06/30 17:03:52 drochner Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <arm/s3c2xx0/s3c24x0var.h>
#include <arm/s3c2xx0/s3c24x0reg.h>
#include <arm/s3c2xx0/s3c2410reg.h>

#include <arm/s3c2xx0/s3c24x0_spi.h>

#include "locators.h"

struct ssspi_softc {
	struct device dev;

	bus_space_tag_t    iot;
	bus_space_handle_t ioh;
	short	index;
};


/* prototypes */
static int	ssspi_match(struct device *, struct cfdata *, void *);
static void	ssspi_attach(struct device *, struct device *, void *);
static int 	ssspi_search(struct device *, struct cfdata *,
			     const locdesc_t *, void *);
static int	ssspi_print(void *, const char *);

/* attach structures */
CFATTACH_DECL(ssspi, sizeof(struct ssspi_softc), ssspi_match, ssspi_attach,
    NULL, NULL);


static int
ssspi_print(void *aux, const char *name)
{
	struct ssspi_attach_args *spia = aux;

	if (spia->spia_aux_intr != SSSPICF_INTR_DEFAULT)
		printf(" intr %d", spia->spia_aux_intr);
        return (UNCONF);
}

int
ssspi_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct s3c2xx0_attach_args *sa = aux;

	/* S3C2410 have only two SPIs */
	switch (sa->sa_index) {
	case 0:
	case 1:
		break;
	default:
		return 0;
	}

	return 1;
}

void
ssspi_attach(struct device *parent, struct device *self, void *aux)
{
	struct ssspi_softc *sc = (struct ssspi_softc*)self;
	struct s3c2xx0_attach_args *sa = (struct s3c2xx0_attach_args *)aux;
	bus_space_tag_t iot = sa->sa_iot;

	static bus_space_handle_t spi_ioh = 0;

	/* we map all registers for SPI0 and SPI1 at once, then
	   use subregions */
	if (spi_ioh == 0) {
		if (bus_space_map(iot, S3C2410_SPI0_BASE,
				  2 * S3C24X0_SPI_SIZE,
				  0, &spi_ioh)) {
			aprint_error(": can't map registers\n");
			return;
		}
	}

	aprint_normal("\n");

	sc->index = sa->sa_index;
	sc->iot = iot;

	bus_space_subregion(iot, spi_ioh, sc->index == 0 ? 0 : S3C24X0_SPI_SIZE,
	    S3C24X0_SPI_SIZE, &sc->ioh);

	/*
	 *  Attach child devices
	 */
	config_search_ia(ssspi_search, self, "ssspi", NULL);
}

int
ssspi_search(parent, cf, ldesc, aux)
	struct device *parent;
	struct cfdata *cf;
	const locdesc_t *ldesc;
	void *aux;
{
	struct ssspi_softc *sc = (struct ssspi_softc *)parent;
	struct ssspi_attach_args spia;
	static const unsigned char intr[] = { S3C24X0_INT_SPI0,
					      S3C2410_INT_SPI1 };

	KASSERT(sc->index == 0 || sc->index == 1);

	spia.spia_iot = sc->iot;
	spia.spia_ioh = sc->ioh;
	spia.spia_gpioh = s3c2xx0_softc->sc_gpio_ioh;
	spia.spia_index = sc->index;
	spia.spia_intr = intr[sc->index];
	spia.spia_aux_intr = cf->cf_loc[SSSPICF_INTR];
	spia.spia_dmat = s3c2xx0_softc->sc_dmat;

        if (config_match(parent, cf, &spia))
                config_attach(parent, cf, &spia, ssspi_print);

        return 0;
}

/*
 * Intiialze SPI port. called by child devices.
 */
int
s3c24x0_spi_setup(struct ssspi_softc *sc, uint32_t mode, int bps, int use_ss)
{
	int pclk = s3c2xx0_softc->sc_pclk;
	int prescaler;
	uint32_t pgcon, pecon;
	bus_space_handle_t gpioh = s3c2xx0_softc->sc_gpio_ioh;
	bus_space_tag_t iot = sc->iot;

	if (bps > 1) {
		prescaler = pclk / 2 / bps - 1;

		if (prescaler <= 0 || 0xff < prescaler)
			return -1;
		bus_space_write_1(sc->iot, sc->ioh, SPI_SPPRE, prescaler);
	}


	if (sc->index == 0) {
		pecon = bus_space_read_4(iot, gpioh, GPIO_PECON);
			
		if (use_ss) {
			pgcon = bus_space_read_4(iot, gpioh, GPIO_PGCON);
			pgcon = GPIO_SET_FUNC(pgcon, 2, PCON_ALTFUN2);
			bus_space_write_4(iot, gpioh, GPIO_PGCON, pgcon);
		}

		pecon = GPIO_SET_FUNC(pecon, 11, PCON_ALTFUN2); /* SPIMISO0 */
		pecon = GPIO_SET_FUNC(pecon, 12, PCON_ALTFUN2); /* SPIMOSI0 */
		pecon = GPIO_SET_FUNC(pecon, 13, PCON_ALTFUN2); /* SPICL0 */

		bus_space_write_4(iot, gpioh, GPIO_PECON, pecon);
	}
	else {
		pgcon = bus_space_read_4(iot, gpioh, GPIO_PGCON);

		if (use_ss)
			pgcon = GPIO_SET_FUNC(pgcon, 3, PCON_ALTFUN2);

		pgcon = GPIO_SET_FUNC(pgcon, 5, PCON_ALTFUN2); /* SPIMISO1 */
		pgcon = GPIO_SET_FUNC(pgcon, 6, PCON_ALTFUN2); /* SPIMOSI1 */
		pgcon = GPIO_SET_FUNC(pgcon, 7, PCON_ALTFUN2); /* SPICLK1 */

		bus_space_write_4(iot, gpioh, GPIO_PGCON, pgcon);
	}

	bus_space_write_4(iot, sc->ioh, SPI_SPCON, mode);

	return 0;
}
