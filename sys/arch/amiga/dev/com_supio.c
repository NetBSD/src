/*	$NetBSD: com_supio.c,v 1.1 1997/08/27 19:32:53 is Exp $	*/

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

/*#include <dev/isa/isavar.h>*/
#include <dev/isa/comreg.h>
#include <dev/isa/comvar.h>

#include <amiga/dev/supio.h>

struct comsupio_softc {
	struct com_softc sc_com;
	struct isr sc_isr;
};

#ifdef __BROKEN_INDIRECT_CONFIG
int com_supio_probe __P((struct device *, void *, void *));
#else
int com_supio_probe __P((struct device *, struct cfdata *, void *));
#endif
void com_supio_attach __P((struct device *, struct device *, void *));
void com_supio_cleanup __P((void *));

static int      comconsaddr;
static bus_space_handle_t comconsioh; 
#if 0
static int      comconsattached;
static bus_space_tag_t comconstag;
static int comconsrate; 
static tcflag_t comconscflag;
#endif

struct cfattach com_supio_ca = {
	sizeof(struct comsupio_softc), com_supio_probe, com_supio_attach
};

int
com_supio_probe(parent, match, aux)
	struct device *parent;
#ifdef __BROKEN_INDIRECT_CONFIG
	void *match;
#else
	struct cfdata *match;
#endif
	void *aux;
{
	bus_space_tag_t iot;
/* XXX	bus_space_handle_t ioh;*/
	int iobase;
	int rv = 1;
	struct supio_attach_args *supa = aux;

	iot = supa->supio_iot;
	iobase = supa->supio_iobase;

	if (strcmp(supa->supio_name,"com") || (match->cf_unit > 1))
		return 0;
#if 0
	/* if it's in use as console, it's there. */
	if (iobase != comconsaddr || comconsattached) {
		if (bus_space_map(iot, iobase, COM_NPORTS, 0, &ioh)) {
			return 0;
		}
		rv = comprobe1(iot, ioh, iobase);
		bus_space_unmap(iot, ioh, COM_NPORTS);
	}
#endif
	return (rv);
}

void
com_supio_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct comsupio_softc *sc = (void *)self;
	struct com_softc *csc = &sc->sc_com;
	int iobase;
	bus_space_tag_t iot;
	struct supio_attach_args *supa = aux;

	/*
	 * We're living on a superio chip.
	 */
	iobase = csc->sc_iobase = supa->supio_iobase;
	iot = csc->sc_iot = supa->supio_iot;
        if (iobase != comconsaddr) {
                if (bus_space_map(iot, iobase, COM_NPORTS, 0, &csc->sc_ioh))
			panic("comattach: io mapping failed");
	} else
                csc->sc_ioh = comconsioh;

	printf(" port 0x%x", iobase);
	com_attach_subr(csc);

	if (amiga_ttyspl < (PSL_S|PSL_IPL5)) {
		printf("%s: raising amiga_ttyspl from 0x%x to 0x%x\n",
		    csc->sc_dev.dv_xname, amiga_ttyspl, PSL_S|PSL_IPL5);
		amiga_ttyspl = PSL_S|PSL_IPL5;
	}
	sc->sc_isr.isr_intr = comintr;
	sc->sc_isr.isr_arg = csc;
	sc->sc_isr.isr_ipl = 5;
	add_isr(&sc->sc_isr);

	/*
	 * Shutdown hook for buggy BIOSs that don't recognize the UART
	 * without a disabled FIFO.
	 */
	if (shutdownhook_establish(com_supio_cleanup, csc) == NULL)
		panic("comsupio: could not establish shutdown hook");
}

void
com_supio_cleanup(arg)
	void *arg;
{
	struct com_softc *sc = arg;

	if (ISSET(sc->sc_hwflags, COM_HW_FIFO))
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, com_fifo, 0);
}
