/*	$NetBSD: com_isa.c,v 1.2 1997/05/24 03:45:40 thorpej Exp $	*/

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

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/isa/isavar.h>
#include <dev/isa/comreg.h>
#include <dev/isa/comvar.h>

#ifdef __BROKEN_INDIRECT_CONFIG
int com_isa_probe __P((struct device *, void *, void *));
#else
int com_isa_probe __P((struct device *, struct cfdata *, void *));
#endif
void com_isa_attach __P((struct device *, struct device *, void *));
void com_isa_cleanup __P((void *));

struct cfattach com_isa_ca = {
	sizeof(struct com_softc), com_isa_probe, com_isa_attach
};

int
com_isa_probe(parent, match, aux)
	struct device *parent;
#ifdef __BROKEN_INDIRECT_CONFIG
	void *match;
#else
	struct cfdata *match;
#endif
	void *aux;
{
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int iobase;
	int rv = 1;
	struct isa_attach_args *ia = aux;

	iot = ia->ia_iot;
	iobase = ia->ia_iobase;

	/* if it's in use as console, it's there. */
	if (iobase != comconsaddr || comconsattached) {
		if (bus_space_map(iot, iobase, COM_NPORTS, 0, &ioh)) {
			return 0;
		}
		rv = comprobe1(iot, ioh, iobase);
		bus_space_unmap(iot, ioh, COM_NPORTS);
	}

	if (rv) {
		ia->ia_iosize = COM_NPORTS;
		ia->ia_msize = 0;
	}
	return (rv);
}

void
com_isa_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct com_softc *sc = (void *)self;
	int iobase, irq;
	bus_space_tag_t iot;
	struct isa_attach_args *ia = aux;

	/*
	 * We're living on an isa.
	 */
	iobase = sc->sc_iobase = ia->ia_iobase;
	iot = sc->sc_iot = ia->ia_iot;
        if (iobase != comconsaddr) {
                if (bus_space_map(iot, iobase, COM_NPORTS, 0, &sc->sc_ioh))
			panic("comattach: io mapping failed");
	} else
                sc->sc_ioh = comconsioh;
	irq = ia->ia_irq;

	com_attach_subr(sc);

	if (irq != IRQUNK) {
		sc->sc_ih = isa_intr_establish(ia->ia_ic, irq,
		    IST_EDGE, IPL_SERIAL, comintr, sc);
	}

	/*
	 * Shutdown hook for buggy BIOSs that don't recognize the UART
	 * without a disabled FIFO.
	 */
	if (shutdownhook_establish(com_isa_cleanup, sc) == NULL)
		panic("comisa: could not establish shutdown hook");
}

void
com_isa_cleanup(arg)
	void *arg;
{
	struct com_softc *sc = arg;

	if (ISSET(sc->sc_hwflags, COM_HW_FIFO))
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, com_fifo, 0);
}
