/*	$NetBSD: ipaq_atmelgpio.c,v 1.1 2001/08/01 07:59:43 ichiro Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.  All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ichiro FUKUHARA (ichiro@ichiro.org).
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
/*
 * iPAQ uses Atmel microcontroller to service a few of peripheral devices.
 * This controller connect to UART1 of SA11x0.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/malloc.h>

#include <machine/bus.h>

#include <hpcarm/dev/ipaq_saipvar.h>
#include <hpcarm/dev/ipaq_gpioreg.h>
#include <hpcarm/dev/ipaq_atmel.h>
#include <hpcarm/sa11x0/sa11x0_comreg.h>
#include <hpcarm/sa11x0/sa11x0_reg.h>

struct atmelgpio_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	struct ipaq_softc	*sc_parent;
};

static	int	atmelgpio_match(struct device *, struct cfdata *, void *);
static	void	atmelgpio_attach(struct device *, struct device *, void *);
static	int	atmelgpio_print(void *, const char *);
static	int	atmelgpio_search(struct device *, struct cfdata *, void *);
static	void	atmelgpio_init(struct atmelgpio_softc *);

struct cfattach atmelgpio_ca = {
	sizeof(struct atmelgpio_softc), atmelgpio_match, atmelgpio_attach
};

static int
atmelgpio_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return (1);
}

static void
atmelgpio_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct atmelgpio_softc *sc = (struct atmelgpio_softc *)self;
	struct ipaq_softc *psc = (struct ipaq_softc *)parent;

	printf("\n");
	printf("%s: Atmel microcontroller GPIO\n",  sc->sc_dev.dv_xname);

	sc->sc_iot = psc->sc_iot;
	sc->sc_ioh = psc->sc_ioh;
	sc->sc_parent = (struct ipaq_softc *)parent;

	if (bus_space_map(sc->sc_iot, SACOM1_BASE, SACOM_NPORTS, 0,
                        &sc->sc_ioh)) {
                printf("%s: unable to map of UART1 registers\n", sc->sc_dev.dv_xname);
                return;
        }

	atmelgpio_init(sc);

	/*
	 *  Attach each devices
	 */

	config_search(atmelgpio_search, self, NULL);
}

static int
atmelgpio_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	if ((*cf->cf_attach->ca_match)(parent, cf, NULL) > 0)
		config_attach(parent, cf, NULL, atmelgpio_print);
	return 0;
}


static int
atmelgpio_print(aux, name)
	void *aux;
	const char *name;
{
	return (UNCONF);
}

static void
atmelgpio_init(sc)
	struct atmelgpio_softc *sc;
{
	/* 8 bits no parity 1 stop bit */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SACOM_CR0, CR0_DSS);

	/* Set baud rate 115k */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SACOM_CR1, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SACOM_CR2, SACOMSPEED(115200));
}
