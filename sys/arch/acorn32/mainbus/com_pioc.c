/*	$NetBSD: com_pioc.c,v 1.14.28.1 2011/06/06 09:04:40 jruoho Exp $	*/

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
 * 3. Neither the name of the University nor the names of its contributors
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

__KERNEL_RCSID(0, "$NetBSD: com_pioc.c,v 1.14.28.1 2011/06/06 09:04:40 jruoho Exp $");

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

static int  com_pioc_probe   (device_t, cfdata_t , void *);
static void com_pioc_attach  (device_t, device_t, void *);

/* device attach structure */

CFATTACH_DECL_NEW(com_pioc, sizeof(struct com_pioc_softc),
    com_pioc_probe, com_pioc_attach, NULL, NULL);

extern bus_space_tag_t comconstag;	/* From pioc.c */

/*
 * int com_pioc_probe(device_t parent, cfdata_t cf, void *aux)
 *
 * Make sure we are trying to attach a com device and then
 * probe for one.
 */

static int
com_pioc_probe(device_t parent, cfdata_t cf, void *aux)
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
 * void com_pioc_attach(device_t parent, device_t self, void *aux)
 *
 * attach the com device
 */

static void
com_pioc_attach(device_t parent, device_t self, void *aux)
{
	struct com_pioc_softc *psc = device_private(self);
	struct com_softc *sc = &psc->sc_com;
	u_int iobase;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	struct pioc_attach_args *pa = aux;
	int count;

	sc->sc_dev = self;
	iot = pa->pa_iot;
	iobase = pa->pa_iobase + pa->pa_offset;

/*
	printf(" (iot = %p, iobase = 0x%08x) ", iot, iobase);
*/
	if (!com_is_console(iot, iobase, &ioh)
	    && bus_space_map(iot, iobase, COM_NPORTS, 0, &ioh))
		panic("comattach: io mapping failed");
	COM_INIT_REGS(sc->sc_regs, iot, ioh, iobase);

	sc->sc_frequency = COM_FREQ;

	com_attach_subr(sc);

	if (pa->pa_irq != MAINBUSCF_IRQ_DEFAULT) {
		psc->sc_ih = intr_claim(pa->pa_irq, IPL_SERIAL, "com",
		    comintr, sc);
	}

	if (!pmf_device_register1(self, com_suspend, com_resume, com_cleanup)) {
		aprint_error_dev(self,
		    "could not establish shutdown hook");
	}

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
	bus_space_write_1(iot, ioh, com_fifo, 0);
	for (count = 0; count < 8; ++count)
		(void)bus_space_read_1(iot, ioh, count);

}

/*
 * Console attachment functions
 */

void
comcnprobe(struct consdev *cp)
{

#ifdef  COMCONSOLE
	cp->cn_pri = CN_REMOTE;	/* Force a serial port console */
#else
	cp->cn_pri = CN_NORMAL;
#endif
}

void
comcninit(struct consdev *cp)
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

	result = comcnattach(comconstag, (IO_CONF_BASE + CONADDR), CONSPEED,
	    COM_FREQ, COM_TYPE_NORMAL, CONMODE);
	if (result) {
		printf("initialising serial; got errornr %d\n", result);
		panic("can't init serial console @%x", CONADDR); 
	};
}


/* End of com_pioc.c */
