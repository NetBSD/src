/*	$NetBSD: com_gsc.c,v 1.1 2002/06/06 19:48:04 fredette Exp $	*/

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
 
	/* ISA-specific goo. */
	void	*sc_ih;			/* interrupt handler */
}; 

int	com_gsc_probe __P((struct device *, struct cfdata *, void *));
void	com_gsc_attach __P((struct device *, struct device *, void *));

struct cfattach com_gsc_ca = {
	sizeof(struct com_gsc_softc), com_gsc_probe, com_gsc_attach
};

/* XXX fredette - this is gross */
extern const struct hppa_bus_space_tag hppa_hpa_bustag;

int
com_gsc_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	register struct gsc_attach_args *ga = aux;

	if (ga->ga_type.iodc_type != HPPA_TYPE_FIO ||
	    (ga->ga_type.iodc_sv_model != HPPA_FIO_GRS232 &&
	     (ga->ga_type.iodc_sv_model != HPPA_FIO_RS232)))
		return 0;

	return comprobe1(&hppa_hpa_bustag, ga->ga_hpa + COMGSC_OFFSET);
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
	sc->sc_iot = &hppa_hpa_bustag;
	sc->sc_ioh = ga->ga_hpa + COMGSC_OFFSET;
	sc->sc_iobase = (bus_addr_t)ga->ga_hpa + COMGSC_OFFSET;
	sc->sc_frequency = COMGSC_FREQUENCY;

	/*
	 * XXX fredette - don't really attach unit 1,
	 * as my serial console is on it, and I haven't
	 * really done any console work yet.
	 */
	if (self->dv_unit == 1) {
		printf(": untouched, assumed serial console\n");
		return;
	}

#if notyet
	*(volatile u_int8_t *)ga->ga_hpa = 0xd0;	/* reset */
	DELAY(1000);
#endif

	com_attach_subr(sc);
	gsc->sc_ih = gsc_intr_establish((struct gsc_softc *)parent, IPL_TTY,
				       ga->ga_irq, comintr, sc, &sc->sc_dev);
}

#ifdef	KGDB
int com_gsc_kgdb_attach __P((void));
int
com_gsc_kgdb_attach(void)
{
	int error;

	/*
	 * We need to conjure up a bus_space_tag that doesn't
	 * do any mapping, because com_kdgb_attach wants to
	 * call bus_space_map.
	 */
	printf("kgdb: attaching com at 0x%x at %d baud...",
		KGDBADDR, KGDBRATE);
	error = com_kgdb_attach(&hppa_hpa_bustag, 
				KGDBADDR + COMGSC_OFFSET, 
				KGDBRATE, COMGSC_FREQUENCY,
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
