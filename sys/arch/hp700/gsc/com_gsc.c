/*	$NetBSD: com_gsc.c,v 1.7 2003/07/15 02:29:25 lukem Exp $	*/

/*	$OpenBSD: com_gsc.c,v 1.8 2000/03/13 14:39:59 mickey Exp $	*/

/*
 * Copyright (c) 1998 Michael Shalayeff
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
 *	This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: com_gsc.c,v 1.7 2003/07/15 02:29:25 lukem Exp $");

#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/tty.h>

#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/iomod.h>
#include <machine/autoconf.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <hp700/dev/cpudevs.h>
#include <hp700/gsc/gscbusvar.h>

#define	COMGSC_OFFSET	0x800
#define	COMGSC_FREQUENCY (1843200 * 4) /* 16-bit baud rate divisor */

struct com_gsc_regs {
	u_int8_t reset;
};

struct com_gsc_softc {
	struct	com_softc sc_com;	/* real "com" softc */
 
	/* GSC-specific goo. */
	void	*sc_ih;			/* interrupt handler */
}; 

int	com_gsc_probe __P((struct device *, struct cfdata *, void *));
void	com_gsc_attach __P((struct device *, struct device *, void *));

CFATTACH_DECL(com_gsc, sizeof(struct com_gsc_softc),
    com_gsc_probe, com_gsc_attach, NULL, NULL);

int
com_gsc_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	register struct gsc_attach_args *ga = aux;
	bus_space_handle_t ioh;
	int rv;

	if (ga->ga_type.iodc_type != HPPA_TYPE_FIO ||
	    (ga->ga_type.iodc_sv_model != HPPA_FIO_GRS232 &&
	     (ga->ga_type.iodc_sv_model != HPPA_FIO_RS232)))
		return 0;

#if 1
	/*
	 * XXX fredette - don't match anything unless it
	 * happens to be the KGDB port, since any other
	 * port might be a serial console, and I haven't
	 * done any console work yet.
	 */
	if (
#ifdef KGDB
	    ga->ga_hpa != KGDBADDR
#else
	    /* CONSTCOND */ 1
#endif
	    )
		return 0;
#endif

	/*
	 * If this port is the console or a KGDB port,
	 * we definitely match, otherwise map and probe
	 * the port.
	 */
	if (!com_is_console(ga->ga_iot, ga->ga_hpa + COMGSC_OFFSET, 0)) {
		if (bus_space_map(ga->ga_iot, ga->ga_hpa + COMGSC_OFFSET, 
				  COM_NPORTS, 0, &ioh))
			return 0;
		rv = comprobe1(ga->ga_iot, ioh);
		bus_space_unmap(ga->ga_iot, ioh, COM_NPORTS);
		if (!rv)
			return 0;
	}

	/* Success. */
	return 1;
}

void
com_gsc_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct com_gsc_softc *gsc = (void *)self;
	struct com_softc *sc = &gsc->sc_com;
	register struct gsc_attach_args *ga = aux;

	sc->sc_hwflags = 0;
	sc->sc_swflags = 0;
	sc->sc_iot = ga->ga_iot;
	sc->sc_iobase = (bus_addr_t)ga->ga_hpa + COMGSC_OFFSET;
	sc->sc_frequency = COMGSC_FREQUENCY;

	if (!com_is_console(sc->sc_iot, sc->sc_iobase, &sc->sc_ioh) &&
	    bus_space_map(sc->sc_iot, sc->sc_iobase,
			  COM_NPORTS, 0, &sc->sc_ioh)) {
		printf(": can't map i/o space\n");
		return;
	}

#if notyet
	*(volatile u_int8_t *)ga->ga_hpa = 0xd0;	/* reset */
	DELAY(1000);
#endif

	com_attach_subr(sc);
	gsc->sc_ih = hp700_intr_establish(&sc->sc_dev, IPL_TTY,
					  comintr, sc,
					  ga->ga_int_reg, ga->ga_irq);
}

#ifdef	KGDB
int com_gsc_kgdb_attach __P((void));
int
com_gsc_kgdb_attach(void)
{
	int error;

	printf("kgdb: attaching com at 0x%x at %d baud...",
		KGDBADDR, KGDBRATE);
	error = com_kgdb_attach(&hppa_bustag, 
				KGDBADDR + COMGSC_OFFSET, 
				KGDBRATE, COMGSC_FREQUENCY,
				COM_TYPE_NORMAL,
	   /* 8N1 */
	   ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8));
	if (error) {
		printf(" failed (%d)\n", error);
	} else {
		printf(" ok\n");
	}
	return (error);
}
#endif	/* KGDB */
