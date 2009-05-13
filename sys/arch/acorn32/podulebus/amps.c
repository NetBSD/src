/*	$NetBSD: amps.c,v 1.14.14.1 2009/05/13 17:16:03 jym Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: amps.c,v 1.14.14.1 2009/05/13 17:16:03 jym Exp $");

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

#include <machine/intr.h>
#include <machine/io.h>
#include <machine/bus.h>
#include <acorn32/podulebus/podulebus.h>
#include <acorn32/podulebus/ampsreg.h>
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

int	amps_probe(struct device *, struct cfdata *, void *);
void	amps_attach(struct device *, struct device *, void *);

CFATTACH_DECL(amps, sizeof(struct amps_softc),
    amps_probe, amps_attach, NULL, NULL);

int	amps_print(void *, const char *);
void	amps_shutdown(void *);

/*
 * Attach arguments for child devices.
 * Pass the podule details, the parent softc and the channel
 */

struct amps_attach_args {
	bus_space_tag_t aa_iot;			/* bus space tag */
	unsigned int aa_base;			/* base address for port */
	int aa_port;      			/* serial port number */
	podulebus_intr_handle_t aa_irq;		/* IRQ number */
};

/*
 * Define prototypes for custom bus space functions.
 */

/* Print function used during child config */

int
amps_print(void *aux, const char *name)
{
	struct amps_attach_args *aa = aux;

	if (!name)
		aprint_normal(", port %d", aa->aa_port);

	return(QUIET);
}

/*
 * Card probe function
 *
 * Just match the manufacturer and podule ID's
 */

int
amps_probe(struct device *parent, struct cfdata *cf, void *aux)
{
	struct podule_attach_args *pa = (void *)aux;

	return (pa->pa_product == PODULE_ATOMWIDE_SERIAL);
}

/*
 * Card attach function
 *
 * Identify the card version and configure any children.
 * Install a shutdown handler to kill interrupts on shutdown
 */

void
amps_attach(struct device *parent, struct device *self, void *aux)
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
		config_found(self, &aa, amps_print);
	}
}

/*
 * Card shutdown function
 *
 * Called via do_shutdown_hooks() during kernel shutdown.
 * Clear the cards's interrupt mask to stop any podule interrupts.
 */

/*void
amps_shutdown(void *arg)
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
	struct	evcnt sc_intrcnt;	/* interrupt count */
};

/* Prototypes for functions */

static int  com_amps_probe   (device_t, cfdata_t , void *);
static void com_amps_attach  (device_t, device_t, void *);

/* device attach structure */

CFATTACH_DECL_NEW(com_amps, sizeof(struct com_amps_softc),
	com_amps_probe, com_amps_attach, NULL, NULL);

/*
 * Controller probe function
 *
 * Map all the required I/O space for this channel, make sure interrupts
 * are disabled and probe the bus.
 */

int
com_amps_probe(device_t parent, cfdata_t cf, void *aux)
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
com_amps_attach(device_t parent, device_t self, void *aux)
{
	struct com_amps_softc *asc = device_private(self);
	struct com_softc *sc = &asc->sc_com;
	u_int iobase;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	struct amps_attach_args *aa = aux;

	sc->sc_dev = self;
	iot = aa->aa_iot;
	iobase = aa->aa_base;

	if (!com_is_console(iot, iobase, &ioh)
	    && bus_space_map(iot, iobase, COM_NPORTS, 0, &ioh))
		panic("comattach: io mapping failed");
	COM_INIT_REGS(sc->sc_regs, iot, ioh, iobase);

	sc->sc_frequency = AMPS_FREQ;
	com_attach_subr(sc);

	evcnt_attach_dynamic(&asc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
	    device_xname(self), "intr");
	asc->sc_ih = podulebus_irq_establish(aa->aa_irq, IPL_SERIAL, comintr,
	    sc, &asc->sc_intrcnt);
}
