/*	$NetBSD: amps.c,v 1.5 2001/03/17 20:34:44 bjh21 Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mark Brinicombe of Causality Limited.
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
 *
 * Card driver and probe and attach functions to use generic 16550 com
 * driver for the Atomwide multiport serial podule
 */

/*
 * Thanks to Martin Coulson, Atomwide, for providing the hardware
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/device.h>

#include <machine/irqhandler.h>
#include <machine/io.h>
#include <machine/bus.h>
#include <arm32/podulebus/podulebus.h>
#include <arm32/podulebus/ampsreg.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#include <dev/podulebus/podules.h>

#include "locators.h"

/*
 * Atomwide Mulit-port serial podule device.
 *
 * This probes and attaches the top level multi-port serial device to the
 * podulebus. It then configures the child com devices.
 */

/*
 * Atomwide Multi-port serial card softc structure.
 *
 * Contains the device node, podule information and global information
 * required by the driver such as the card version and the interrupt mask.
 */

struct amps_softc {
	struct device		sc_dev;			/* device node */
	podule_t 		*sc_podule;		/* Our podule info */
	int 			sc_podule_number;	/* Our podule number */
	bus_space_tag_t		sc_iot;			/* Bus tag */
};

int	amps_probe	__P((struct device *, struct cfdata *, void *));
void	amps_attach	__P((struct device *, struct device *, void *));
void	amps_shutdown	__P((void *arg));

struct cfattach amps_ca = {
	sizeof(struct amps_softc), amps_probe, amps_attach
};

/*
 * Attach arguments for child devices.
 * Pass the podule details, the parent softc and the channel
 */

struct amps_attach_args {
	bus_space_tag_t aa_iot;			/* bus space tag */
	unsigned int aa_base;			/* base address for port */
	int aa_port;      			/* serial port number */
	int aa_irq;				/* IRQ number */
};

/*
 * Define prototypes for custom bus space functions.
 */

/* Print function used during child config */

int
amps_print(aux, name)
	void *aux;
	const char *name;
{
	struct amps_attach_args *aa = aux;

	if (!name)
		printf(", port %d", aa->aa_port);

	return(QUIET);
}

/*
 * Card probe function
 *
 * Just match the manufacturer and podule ID's
 */

int
amps_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct podule_attach_args *pa = (void *)aux;

	if (matchpodule(pa, MANUFACTURER_ATOMWIDE2, PODULE_ATOMWIDE2_SERIAL, -1) == 0)
		return(0);
	return(1);
}

/*
 * Card attach function
 *
 * Identify the card version and configure any children.
 * Install a shutdown handler to kill interrupts on shutdown
 */

void
amps_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct amps_softc *sc = (void *)self;
	struct podule_attach_args *pa = (void *)aux;
	struct amps_attach_args aa;

	/* Note the podule number and validate */

	if (pa->pa_podule_number == -1)
		panic("Podule has disappeared !");

	sc->sc_podule_number = pa->pa_podule_number;
	sc->sc_podule = pa->pa_podule;
	podules[sc->sc_podule_number].attached = 1;

	sc->sc_iot = pa->pa_iot;

	/* Install a clean up handler to make sure IRQ's are disabled */
/*	if (shutdownhook_establish(amps_shutdown, (void *)sc) == NULL)
		panic("%s: Cannot install shutdown handler", self->dv_xname);*/

	/* Set the interrupt info for this podule */

/*	sc->sc_podule->irq_addr = pa->pa_podule->slow_base +;*/	/* XXX */
/*	sc->sc_podule->irq_mask = ;*/

	printf("\n");

	/* Configure the children */

	aa.aa_iot = sc->sc_iot;
	aa.aa_base = pa->pa_podule->slow_base + AMPS_BASE_OFFSET;
	aa.aa_base += MAX_AMPS_PORTS * AMPS_PORT_SPACING;
	aa.aa_irq = pa->pa_podule->interrupt;
	for (aa.aa_port = 0; aa.aa_port < MAX_AMPS_PORTS; ++aa.aa_port) {
		aa.aa_base -= AMPS_PORT_SPACING;
		config_found_sm(self, &aa, amps_print, NULL);
	}
}

/*
 * Card shutdown function
 *
 * Called via do_shutdown_hooks() during kernel shutdown.
 * Clear the cards's interrupt mask to stop any podule interrupts.
 */

/*void
amps_shutdown(arg)
	void *arg;
{
}*/

/*
 * Atomwide Multi-Port Serial probe and attach code for the com device.
 *
 * This provides a different pair of probe and attach functions
 * for attaching the com device (dev/ic/com.c) to the Atomwide serial card.
 */

struct com_amps_softc {
	struct	com_softc sc_com;	/* real "com" softc */
	void	*sc_ih;			/* interrupt handler */
};

/* Prototypes for functions */

static int  com_amps_probe   __P((struct device *, struct cfdata *, void *));
static void com_amps_attach  __P((struct device *, struct device *, void *));

/* device attach structure */

struct cfattach com_amps_ca = {
	sizeof(struct com_amps_softc), com_amps_probe, com_amps_attach
};

/*
 * Controller probe function
 *
 * Map all the required I/O space for this channel, make sure interrupts
 * are disabled and probe the bus.
 */

int
com_amps_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int iobase;
	int rv = 1;
	struct amps_attach_args *aa = aux;

	iot = aa->aa_iot;
	iobase = aa->aa_base;

	/* if it's in use as console, it's there. */
	if (!com_is_console(iot, iobase, 0)) {
		if (bus_space_map(iot, iobase, COM_NPORTS, 0, &ioh)) {
			return 0;
		}
		/*
		 * We don't use the generic comprobe1() function as it
		 * is not good enough to identify which ports are present.
		 *
		 * Instead test for the presence of the scratch register
		 */

		bus_space_write_1(iot, ioh, com_scratch, 0x55);
		bus_space_write_1(iot, ioh, com_ier, 0);
		(void)bus_space_read_1(iot, ioh, com_data);
		if (bus_space_read_1(iot, ioh, com_scratch) != 0x55)
			rv = 0;
		bus_space_write_1(iot, ioh, com_scratch, 0xaa);
		bus_space_write_1(iot, ioh, com_ier, 0);
		(void)bus_space_read_1(iot, ioh, com_data);
		if (bus_space_read_1(iot, ioh, com_scratch) != 0xaa)
			rv = 0;

		bus_space_unmap(iot, ioh, COM_NPORTS);
	}
	return (rv);
}

/*
 * Controller attach function
 *
 * Map all the required I/O space for this port and attach the driver
 * The generic attach will probe and attach any device.
 * Install an interrupt handler and we are ready to rock.
 */    

void
com_amps_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct com_amps_softc *asc = (void *)self;
	struct com_softc *sc = &asc->sc_com;
	u_int iobase;
	bus_space_tag_t iot;
	struct amps_attach_args *aa = aux;

	iot = sc->sc_iot = aa->aa_iot;
	iobase = sc->sc_iobase = aa->aa_base;

	if (!com_is_console(iot, iobase, &sc->sc_ioh)
		&& bus_space_map(iot, iobase, COM_NPORTS, 0, &sc->sc_ioh))
			panic("comattach: io mapping failed");

	sc->sc_frequency = AMPS_FREQ;
	com_attach_subr(sc);

	asc->sc_ih = intr_claim(aa->aa_irq, IPL_SERIAL, "com",
	    comintr, sc);
}
