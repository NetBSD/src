/* $NetBSD: arcpp.c,v 1.7.20.1 2007/03/12 05:45:14 rmind Exp $ */

/*-
 * Copyright (c) 2001 Ben Harris
 * Copyright (c) 1993, 1994 Charles M. Hannum.
 * Copyright (c) 1990 William F. Jolitz, TeleMuse
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
 *	This software is a component of "386BSD" developed by 
 *	William F. Jolitz, TeleMuse.
 * 4. Neither the name of the developer nor the name "386BSD"
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS A COMPONENT OF 386BSD DEVELOPED BY WILLIAM F. JOLITZ 
 * AND IS INTENDED FOR RESEARCH AND EDUCATIONAL PURPOSES ONLY. THIS 
 * SOFTWARE SHOULD NOT BE CONSIDERED TO BE A COMMERCIAL PRODUCT. 
 * THE DEVELOPER URGES THAT USERS WHO REQUIRE A COMMERCIAL PRODUCT 
 * NOT MAKE USE OF THIS WORK.
 *
 * FOR USERS WHO WISH TO UNDERSTAND THE 386BSD SYSTEM DEVELOPED
 * BY WILLIAM F. JOLITZ, WE RECOMMEND THE USER STUDY WRITTEN 
 * REFERENCES SUCH AS THE  "PORTING UNIX TO THE 386" SERIES 
 * (BEGINNING JANUARY 1991 "DR. DOBBS JOURNAL", USA AND BEGINNING 
 * JUNE 1991 "UNIX MAGAZIN", GERMANY) BY WILLIAM F. JOLITZ AND 
 * LYNNE GREER JOLITZ, AS WELL AS OTHER BOOKS ON UNIX AND THE 
 * ON-LINE 386BSD USER MANUAL BEFORE USE. A BOOK DISCUSSING THE INTERNALS 
 * OF 386BSD ENTITLED "386BSD FROM THE INSIDE OUT" WILL BE AVAILABLE LATE 1992.
 *
 * THIS SOFTWARE IS PROVIDED BY THE DEVELOPER ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE DEVELOPER BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: arcpp.c,v 1.7.20.1 2007/03/12 05:45:14 rmind Exp $");

#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/irq.h>

#include <arch/acorn26/iobus/iocvar.h>
#include <arch/acorn26/ioc/arcppreg.h>
#include <arch/acorn26/ioc/latchreg.h>
#include <arch/acorn26/ioc/latchvar.h>
#include <arch/acorn26/ioc/ioebvar.h>

#include "ioeb.h"

struct arcpp_softc {
	struct device	sc_dev;
	void		*sc_aih;	/* ACK interrupt handler */
	struct evcnt	sc_aintrcnt;	/* ... and count */
	void		*sc_bih;	/* BUSY interrupt handler */
	struct evcnt	sc_bintrcnt;	/* ... and count */
	size_t sc_count;
	void *sc_inbuf;
	u_char *sc_cp;
	int sc_spinmax;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	u_char sc_state;
#define	ARCPP_OPEN	0x01	/* device is open */
#define	ARCPP_OBUSY	0x02	/* printer is busy doing output */
#define	ARCPP_INIT	0x04	/* waiting to initialize for open */
	u_char sc_flags;
/*#define	ARCPP_AUTOLF	0x20	* automatic LF on CR */
/*#define	ARCPP_NOPRIME	0x40	* don't prime on open */
/*#define	ARCPP_NOINTR	0x80	* do not use interrupt */
	u_char sc_laststatus;
};

#define	TIMEOUT		(hz*16)	/* wait up to 16 seconds for a ready */
#define	STEP		(hz/4)

#define	ARCPPPRI	(PZERO+8)
#define	ARCPP_BSIZE	1024

extern struct cfdriver arcpp_cd;

dev_type_open(arcppopen);
dev_type_close(arcppclose);
dev_type_write(arcppwrite);

const struct cdevsw arcpp_cdevsw = {
	arcppopen, arcppclose, noread, arcppwrite, noioctl,
	nostop, notty, nopoll, nommap, nokqfilter,
};

#define	ARCPPUNIT(s)	(minor(s) & 0x1f)
#define	ARCPPFLAGS(s)	(minor(s) & 0xe0)

static int arcpp_match(struct device *, struct cfdata *, void *);
static void arcpp_attach(struct device *, struct device *, void *);

static int arcppintr(void *);
static int arcpppushbytes(struct arcpp_softc *);

CFATTACH_DECL(arcpp, sizeof(struct arcpp_softc),
    arcpp_match, arcpp_attach, NULL, NULL);

static int
arcpp_match(struct device *parent, struct cfdata *cf, void *aux)
{

	/*
	 * The parallel port is write-only, so we can't probe for it.
	 * Happily, the set of machines it exists on is precisely the
	 * set that doesn't have IOEBs, so...
	 */
#if NIOEB > 0
	if (the_ioeb != NULL)
		return 0;
#endif
	return 1;
}

static void
arcpp_attach(struct device *parent, struct device *self, void *aux)
{
	struct arcpp_softc *sc = (void *)self;
	struct ioc_attach_args *ioc = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	iot = sc->sc_iot = ioc->ioc_fast_t;
	ioh = sc->sc_ioh = ioc->ioc_fast_h;

	evcnt_attach_dynamic(&sc->sc_aintrcnt, EVCNT_TYPE_INTR, NULL,
	    self->dv_xname, "ack intr");
	evcnt_attach_dynamic(&sc->sc_bintrcnt, EVCNT_TYPE_INTR, NULL,
	    self->dv_xname, "busy intr");
	sc->sc_aih = irq_establish(IRQ_PACK, IPL_LPT, arcppintr, sc,
	    &sc->sc_aintrcnt);
	sc->sc_bih = irq_establish(IRQ_PBSY, IPL_LPT, arcppintr, sc,
	    &sc->sc_bintrcnt);

	/* We're not interested in interrupts while the device is closed. */
	irq_disable(sc->sc_aih);
	irq_disable(sc->sc_bih);

	printf("\n");
}

/*
 * Wait until the printer's selected and not busy.
 */
int
arcppopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	u_char flags = ARCPPFLAGS(dev);
	struct arcpp_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int error, s;

	sc = device_lookup(&arcpp_cd, ARCPPUNIT(dev));
	if (sc == NULL)
		return ENXIO;

#ifdef DIAGNOSTIC
	if (sc->sc_state)
		printf("%s: stat=0x%x not zero\n", sc->sc_dev.dv_xname,
		    sc->sc_state);
#endif

	if (sc->sc_state)
		return EBUSY;

	sc->sc_state = ARCPP_INIT;
	sc->sc_flags = flags;
	iot = sc->sc_iot;
	ioh = sc->sc_ioh;

	s = spllpt();
	/* wait till ready (printer running diagnostics) */
	irq_enable(sc->sc_bih);
	/* XXX Is it really appropriate to time out? */
	error = tsleep((void *)sc, ARCPPPRI | PCATCH, "arcppopen", TIMEOUT);
	if (error == EWOULDBLOCK) {
		sc->sc_state = 0;
		return EBUSY;
	}
	if (error) {
		sc->sc_state = 0;
		return error;
	}

	sc->sc_inbuf = malloc(ARCPP_BSIZE, M_DEVBUF, M_WAITOK);
	sc->sc_count = 0;

	sc->sc_state = ARCPP_OPEN;

	arcppintr(sc);
	splx(s);

	return 0;
}

/*
 * Close the device, and free the local line buffer.
 */
int
arcppclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct arcpp_softc *sc = device_lookup(&arcpp_cd, ARCPPUNIT(dev));

	if (sc->sc_count)
		(void) arcpppushbytes(sc);

	irq_disable(sc->sc_aih);
	irq_disable(sc->sc_bih);
	sc->sc_state = 0;
	free(sc->sc_inbuf, M_DEVBUF);

	return 0;
}

static int
arcpppushbytes(sc)
	struct arcpp_softc *sc;
{
	int error;
	int s;

	while (sc->sc_count > 0) {
		/* if the printer is ready for a char, give it one */
		if ((sc->sc_state & ARCPP_OBUSY) == 0) {
			s = spllpt();
			arcppintr(sc);
			splx(s);
		}
		error = tsleep((void *)sc, ARCPPPRI | PCATCH, "arcppwr", 0);
		if (error)
			return error;
	}
	return 0;
}

int
arcppwrite(dev_t dev, struct uio *uio, int flags)
{
	struct arcpp_softc *sc = device_lookup(&arcpp_cd, ARCPPUNIT(dev));
	size_t n;
	int error = 0;

	while ((n = min(ARCPP_BSIZE, uio->uio_resid)) != 0) {
		uiomove(sc->sc_cp = sc->sc_inbuf, n, uio);
		sc->sc_count = n;
		error = arcpppushbytes(sc);
		if (error) {
			/*
			 * Return accurate residual if interrupted or timed
			 * out.
			 */
			uio->uio_resid += sc->sc_count;
			sc->sc_count = 0;
			return error;
		}
	}
	return 0;
}

/*
 * Handle printer interrupts which occur when the printer is ready to accept
 * another char.
 */
int
arcppintr(void *arg)
{
	struct arcpp_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	/* is printer online and ready for output */
	if (!ioc_irq_status(IRQ_PBSY)) {
		return 0;
	}

	if (sc->sc_count) {
		/* send char */
		bus_space_write_1(iot, ioh, ARCPP_DATA, *sc->sc_cp++);
		DELAY(1);
		latchb_update(LATCHB_NPSTB, 0);
		DELAY(1);
		sc->sc_count--;
		latchb_update(LATCHB_NPSTB, LATCHB_NPSTB);
		DELAY(1);
		sc->sc_state |= ARCPP_OBUSY;
	} else
		sc->sc_state &= ~ARCPP_OBUSY;

	if (sc->sc_count == 0)
		/* none, wake up the top half to get more */
		wakeup((void *)sc);

	return 1;
}
