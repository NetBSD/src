/*	$NetBSD: com_pioc.c,v 1.1.4.3 2002/02/28 04:05:56 nathanw Exp $	*/

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

#include <sys/param.h>

__RCSID("$NetBSD: com_pioc.c,v 1.1.4.3 2002/02/28 04:05:56 nathanw Exp $");

#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <machine/intr.h>
#include <machine/bus.h>
#include <machine/io.h>

#include <acorn32/mainbus/piocvar.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <dev/cons.h>

#include "locators.h"

struct com_pioc_softc {
	struct	com_softc sc_com;	/* real "com" softc */
	void	*sc_ih;			/* interrupt handler */
};

/* Prototypes for functions */

cons_decl(com);   

static int  com_pioc_probe   __P((struct device *, struct cfdata *, void *));
static void com_pioc_attach  __P((struct device *, struct device *, void *));
static void com_pioc_cleanup __P((void *));

/* device attach structure */

struct cfattach com_pioc_ca = {
	sizeof(struct com_pioc_softc), com_pioc_probe, com_pioc_attach
};

extern bus_space_tag_t comconstag;	/* From pioc.c */

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
	if (!com_is_console(iot, iobase, 0)) {
		if (bus_space_map(iot, iobase, COM_NPORTS, 0, &ioh)) {
			return 0;
		}
		rv = comprobe1(iot, ioh);
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
	struct com_pioc_softc *psc = (void *)self;
	struct com_softc *sc = &psc->sc_com;
	u_int iobase;
	bus_space_tag_t iot;
	struct pioc_attach_args *pa = aux;
	int count;

	iot = sc->sc_iot = pa->pa_iot;
	iobase = sc->sc_iobase = pa->pa_iobase + pa->pa_offset;

/*
	printf(" (iot = %p, iobase = 0x%08x) ", iot, iobase);
*/
	if (!com_is_console(iot, iobase, &sc->sc_ioh)
		&& bus_space_map(iot, iobase, COM_NPORTS, 0, &sc->sc_ioh))
			panic("comattach: io mapping failed");

	sc->sc_frequency = COM_FREQ;

	com_attach_subr(sc);

	if (pa->pa_irq != MAINBUSCF_IRQ_DEFAULT) {
		psc->sc_ih = intr_claim(pa->pa_irq, IPL_SERIAL, "com",
		    comintr, sc);
	}

	/*
	 * Shutdown hook for buggy BIOSs that don't recognize the UART
	 * without a disabled FIFO.
	 */
	if (shutdownhook_establish(com_pioc_cleanup, sc) == NULL)
		panic("%s: could not establish shutdown hook",
		    sc->sc_dev.dv_xname);

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
 * Console attachment functions
 */

void
comcnprobe(cp)
	struct consdev *cp;
{

#ifdef  COMCONSOLE
	cp->cn_pri = CN_REMOTE;	/* Force a serial port console */
#else
	cp->cn_pri = CN_NORMAL;
#endif
}

void
comcninit(cp)
	struct consdev *cp;
{
	int result;

#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
#ifndef CONSPEED
#define CONSPEED 38400
#endif
#ifndef CONADDR
#define CONADDR	0x3f8
#endif

	result = comcnattach(comconstag, (IO_CONF_BASE + CONADDR), CONSPEED, COM_FREQ, CONMODE);
	if (result) {
		printf("initialising serial; got errornr %d\n", result);
		panic("can't init serial console @%x", CONADDR); 
	};
}


/* End of com_pioc.c */
