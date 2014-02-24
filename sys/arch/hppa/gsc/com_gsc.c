/*	$NetBSD: com_gsc.c,v 1.1 2014/02/24 07:23:43 skrll Exp $	*/

/*	$OpenBSD: com_gsc.c,v 1.8 2000/03/13 14:39:59 mickey Exp $	*/

/*
 * Copyright (c) 1998-2004 Michael Shalayeff
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: com_gsc.c,v 1.1 2014/02/24 07:23:43 skrll Exp $");

#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/tty.h>

#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include <sys/bus.h>
#include <machine/intr.h>
#include <machine/iomod.h>
#include <machine/autoconf.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <hppa/dev/cpudevs.h>
#include <hppa/gsc/gscbusvar.h>
#include <hppa/hppa/machdep.h>

#define	COMGSC_OFFSET	0x800
#define	COMGSC_FREQUENCY (1843200 * 4) /* 16-bit baud rate divisor */

struct com_gsc_regs {
	uint8_t reset;
};

struct com_gsc_softc {
	struct	com_softc sc_com;	/* real "com" softc */

	/* GSC-specific goo. */
	void	*sc_ih;			/* interrupt handler */
};

int	com_gsc_probe(device_t, cfdata_t , void *);
void	com_gsc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(com_gsc, sizeof(struct com_gsc_softc),
    com_gsc_probe, com_gsc_attach, NULL, NULL);

int
com_gsc_probe(device_t parent, cfdata_t match, void *aux)
{
	struct gsc_attach_args *ga = aux;

	if (ga->ga_type.iodc_type != HPPA_TYPE_FIO ||
	    (ga->ga_type.iodc_sv_model != HPPA_FIO_GRS232 &&
	     ga->ga_type.iodc_sv_model != HPPA_FIO_RS232))
		return 0;

	return 1;
}

void
com_gsc_attach(device_t parent, device_t self, void *aux)
{
	struct com_gsc_softc *gsc = device_private(self);
	struct com_softc *sc = &gsc->sc_com;
	struct gsc_attach_args *ga = aux;
	int pagezero_cookie;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_addr_t iobase;

	sc->sc_dev = self;
	sc->sc_hwflags = 0;
	sc->sc_swflags = 0;
	iot = ga->ga_iot;
	iobase = (bus_addr_t)ga->ga_hpa + COMGSC_OFFSET;
	sc->sc_frequency = COMGSC_FREQUENCY;

	/* Test if this is the console.  Compare either HPA or device path. */
	pagezero_cookie = hppa_pagezero_map();
	if ((hppa_hpa_t)PAGE0->mem_cons.pz_hpa == ga->ga_hpa ) {

		/*
		 * This port is the console.  In this case we must call
		 * comcnattach() and later com_is_console() to initialize
		 * everything properly.
		 */

		if (comcnattach(iot, iobase, B9600,
			sc->sc_frequency, COM_TYPE_NORMAL,
			(TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8) != 0) {
			aprint_error(": can't comcnattach\n");
			hppa_pagezero_unmap(pagezero_cookie);
			return;
		}
	}
	hppa_pagezero_unmap(pagezero_cookie);

	/*
	 * Get the already initialized console ioh via com_is_console() if
	 * this is the console or map the I/O space if this isn't the console.
	 */

	if (!com_is_console(iot, iobase, &ioh) &&
	    bus_space_map(iot, iobase, COM_NPORTS, 0, &ioh) != 0) {
		aprint_error(": can't map I/O space\n");
		return;
	}
	COM_INIT_REGS(sc->sc_regs, iot, ioh, iobase);

	com_attach_subr(sc);
	gsc->sc_ih = hppa_intr_establish(IPL_TTY, comintr, sc, ga->ga_ir,
	    ga->ga_irq);
}

#ifdef	KGDB
int com_gsc_kgdb_attach(void);
int
com_gsc_kgdb_attach(void)
{
	int error;

	printf("kgdb: attaching com at 0x%x at %d baud...",
		KGDBADDR, KGDBRATE);
	error = com_kgdb_attach(&hppa_bustag, KGDBADDR + COMGSC_OFFSET,
				KGDBRATE, COMGSC_FREQUENCY, COM_TYPE_NORMAL,
				((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) |
				 CS8));
	if (error) {
		printf(" failed (%d)\n", error);
	} else {
		printf(" ok\n");
	}
	return (error);
}
#endif	/* KGDB */
