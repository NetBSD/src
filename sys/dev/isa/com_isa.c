/*	$NetBSD: com_isa.c,v 1.17.6.1 2002/04/06 16:14:19 eeh Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

/*-
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: com_isa.c,v 1.17.6.1 2002/04/06 16:14:19 eeh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/properties.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <dev/isa/isavar.h>

struct com_isa_softc {
	struct	com_softc sc_com;	/* real "com" softc */

	/* ISA-specific goo. */
	void	*sc_ih;			/* interrupt handler */
};

int com_isa_probe __P((struct device *, struct cfdata *, void *));
void com_isa_attach __P((struct device *, struct device *, void *));
void com_isa_cleanup __P((void *));

struct cfattach com_isa_ca = {
	-sizeof(struct com_isa_softc), com_isa_probe, com_isa_attach
};

int
com_isa_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int iobase;
	int rv = 1;
	struct device *self = (struct device *)aux;
	struct isa_attach_args *ia = DEV_PRIVATE(self);

	if (dev_getprop(self, "PNP-name", NULL, 0, NULL, 0) != -1)
		return (0);
	if (dev_getprop(self, "PNP-compat", NULL, 0, NULL, 0) != -1)
		return (0);
	if (ISA_DIRECT_CONFIG(ia)) {
		printf("com_isa: direct config -- how did this happen?\n");
		return (0);
	}

	if (dev_getprop(parent, "io-tag", &iot, sizeof(iot), NULL, 0) !=
		sizeof(iot))
		return (0);
	if (dev_getprop(self, "port", &iobase, sizeof(iobase), NULL, 0) !=
		sizeof(iobase))
		return (0);
	/* Don't allow wildcarded IRQ. */
	if (dev_getprop(self, "irq", NULL, 0, NULL, 0) < sizeof(int))
		return (0);

	/* if it's in use as console, it's there. */
	if (!com_is_console(iot, iobase, 0)) {
		if (bus_space_map(iot, iobase, COM_NPORTS, 0, &ioh)) {
			return 0;
		}
		rv = comprobe1(iot, ioh);
		bus_space_unmap(iot, ioh, COM_NPORTS);
	}
	if (rv) {
		int size = COM_NPORTS;

		dev_setprop(self, "size", &size, sizeof(size), PROP_INT, 0);
		dev_delprop(self, "iomem");
		dev_delprop(self, "iosiz");
		dev_delprop(self, "drq");
		dev_delprop(self, "drq2");
	}
	return (rv);
}

void
com_isa_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct com_isa_softc *isc = (void *)self;
	struct com_softc *sc = &isc->sc_com;
	int iobase, irq;
	bus_space_tag_t iot;
	struct isa_attach_args *ia = aux;

	/*
	 * We're living on an isa.
	 */
	if (dev_getprop(parent, "io-tag", &iot, sizeof(iot), NULL, 0) !=
		sizeof(iot))
		panic("com_isa_attach: no io-tag");
	if (dev_getprop(self, "port", &iobase, sizeof(iobase), NULL, 0) !=
		sizeof(iobase))
		panic("com_isa_attach: no port");

	sc->sc_iobase = iobase;
	sc->sc_iot = iot;

	if (!com_is_console(iot, iobase, &sc->sc_ioh) &&
	    bus_space_map(iot, iobase, COM_NPORTS, 0, &sc->sc_ioh)) {
		printf(": can't map i/o space\n");
		return;
	}

	sc->sc_frequency = COM_FREQ;
	if (dev_getprop(self, "irq", &irq, sizeof(irq), NULL, 0) !=
		sizeof(iobase))
		panic("com_isa_attach: no irq");

	com_attach_subr(sc);

	isc->sc_ih = isa_intr_establish(ia->ia_ic, irq, IST_EDGE, IPL_SERIAL,
	    comintr, sc);

	/*
	 * Shutdown hook for buggy BIOSs that don't recognize the UART
	 * without a disabled FIFO.
	 */
	if (shutdownhook_establish(com_isa_cleanup, sc) == NULL)
		panic("com_isa_attach: could not establish shutdown hook");
}

void
com_isa_cleanup(arg)
	void *arg;
{
	struct com_softc *sc = arg;

	if (ISSET(sc->sc_hwflags, COM_HW_FIFO))
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, com_fifo, 0);
}
