/*	$NetBSD: com_pioc.c,v 1.1.2.2 1997/10/15 05:40:34 thorpej Exp $	*/

/*-
 * Copyright (c) 1993, 1994, 1995, 1996
 *	Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)com.c	7.5 (Berkeley) 5/16/91
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
#include <machine/bus.h>

#include <arm32/mainbus/piocvar.h>
#include <arm32/dev/comreg.h>
#include <arm32/dev/comvar.h>

#include "locators.h"

/* Prototypes for functions */

static int  com_pioc_probe   __P((struct device *, struct cfdata *, void *));
static void com_pioc_attach  __P((struct device *, struct device *, void *));
static void com_pioc_cleanup __P((void *));

/* device attach structure */

struct cfattach com_pioc_ca = {
	sizeof(struct com_softc), com_pioc_probe, com_pioc_attach
};

/*
 * int com_pioc_probe(struct device *parent, struct cfdata *cf, void *aux)
 *
 * Make sure we are trying to attach a com device and then
 * probe for one.
 */

static int
com_pioc_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int iobase;
	int rv = 1;
	struct pioc_attach_args *pa = aux;

	/* We need an offset */
	if (pa->pa_offset == PIOCCF_OFFSET_DEFAULT)
		return(0);

	iot = pa->pa_iot;
	iobase = pa->pa_iobase + pa->pa_offset;

	/* if it's in use as console, it's there. */
	if (iobase != comconsaddr || comconsattached) {
		if (bus_space_map(iot, iobase, COM_NPORTS, 0, &ioh)) {
			return 0;
		}
		rv = comprobe1(iot, ioh, iobase);
		bus_space_unmap(iot, ioh, COM_NPORTS);
	}

	if (rv) {
		pa->pa_iosize = COM_NPORTS;
	}
	return (rv);
}

/*
 * void com_pioc_attach(struct device *parent, struct device *self, void *aux)
 *
 * attach the com device
 */

static void
com_pioc_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct com_softc *sc = (void *)self;
	int iobase, irq;
	bus_space_tag_t iot;
	struct pioc_attach_args *pa = aux;
	int count;

	iot = pa->pa_iot;
	iobase = pa->pa_iobase + pa->pa_offset;

        if (iobase != comconsaddr) {
                if (bus_space_map(iot, iobase, COM_NPORTS, 0, &sc->sc_ioh))
			panic("comattach: io mapping failed");
	} else
                sc->sc_ioh = comconsioh;
	irq = pa->pa_irq;

	sc->sc_iot = iot;
	sc->sc_iobase = iobase;

	com_attach_subr(sc);

	if (irq != MAINBUSCF_IRQ_DEFAULT) {
		sc->sc_ih = intr_claim(irq, IPL_TTY, "com",
		    comintr, sc);
	}

	/*
	 * Shutdown hook for buggy BIOSs that don't recognize the UART
	 * without a disabled FIFO.
	 */
	if (shutdownhook_establish(com_pioc_cleanup, sc) == NULL)
		panic("%s: could not establish shutdown hook", sc->sc_dev.dv_xname);

	/*
	 * This is a patch for bugged revision 1-4 SMC FDC37C665
	 * I/O controllers.
	 * If there is RX data pending when the FIFO in turned on
	 * the RX register cannot be emptied.
	 *
	 * Solution:
	 *   Make sure FIFO is off.
	 *   Read pending data / int status etc.
	 */
	bus_space_write_1(iot, sc->sc_ioh, com_fifo, 0);
	for (count = 0; count < 8; ++count)
		(void)bus_space_read_1(iot, sc->sc_ioh, count);

}

/*
 * void com_pioc_cleanup(void *arg)
 *
 * clean up driver
 */

static void
com_pioc_cleanup(arg)
	void *arg;
{
	struct com_softc *sc = arg;

	if (ISSET(sc->sc_hwflags, COM_HW_FIFO))
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, com_fifo, 0);
}

/*
 * XXX - temporary
 *
 * Temporary measure to implement splserial and softserial handling.
 * These are new with the new serial driver and require mods to the
 * interrupt code so for initial testing of the new driver
 * emulate them.
 */

void 	comsoft		__P((void *));

int splserial(s)
	int s;
{
	return(spltty());
}

void
setsoftserial()
{
	timeout(comsoft, NULL, 1);
}

/* End of com_pioc.c */
