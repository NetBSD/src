/*	$NetBSD: lpt_gsc.c,v 1.1 2002/06/06 19:48:05 fredette Exp $	*/

/*	$OpenBSD: lpt_gsc.c,v 1.6 2000/07/21 17:41:06 mickey Exp $	*/

/*
 * Copyright (c) 1998 Michael Shalayeff
 * Copyright (c) 1993, 1994 Charles Hannum.
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
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/iomod.h>
#include <machine/autoconf.h>

#include <dev/ic/lptreg.h>
#include <dev/ic/lptvar.h>

#include <hp700/dev/cpudevs.h>

#include <hp700/gsc/gscbusvar.h>

#define	LPTGSC_OFFSET	0x800

#ifndef	LPTDEBUG 
#define	LPRINTF(a) 
#else
#define	LPRINTF(a)	if (lpt_isa_debug) printf a
int lpt_isa_debug = 0;
#endif

int	lpt_gsc_probe __P((struct device *, struct cfdata *, void *));
void	lpt_gsc_attach __P((struct device *, struct device *, void *));

struct cfattach lpt_gsc_ca = {
	sizeof(struct lpt_softc), lpt_gsc_probe, lpt_gsc_attach
};

int	lpt_port_test __P((bus_space_tag_t, bus_space_handle_t, bus_addr_t,
	    bus_size_t, u_char, u_char));

/*
 * Internal routine to lptprobe to do port tests of one byte value.
 */
int
lpt_port_test(iot, ioh, base, off, data, mask)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_addr_t base;
	bus_size_t off;
	u_char data, mask;
{
	int timeout;
	u_char temp;

	data &= mask;
	bus_space_write_1(iot, ioh, off, data);
	timeout = 1000;
	do {
		delay(10);
		temp = bus_space_read_1(iot, ioh, off) & mask;
	} while (temp != data && --timeout);
	LPRINTF(("lpt: port=0x%x out=0x%x in=0x%x timeout=%d\n",
	    (unsigned)(base + off), (unsigned)data, (unsigned)temp, timeout));
	return (temp == data);
}

/*
 * Logic:
 *	1) You should be able to write to and read back the same value
 *	   to the data port.  Do an alternating zeros, alternating ones,
 *	   walking zero, and walking one test to check for stuck bits.
 *
 *	2) You should be able to write to and read back the same value
 *	   to the control port lower 5 bits, the upper 3 bits are reserved
 *	   per the IBM PC technical reference manauls and different boards
 *	   do different things with them.  Do an alternating zeros, alternating
 *	   ones, walking zero, and walking one test to check for stuck bits.
 *
 *	   Some printers drag the strobe line down when the are powered off
 * 	   so this bit has been masked out of the control port test.
 *
 *	   XXX Some printers may not like a fast pulse on init or strobe, I
 *	   don't know at this point, if that becomes a problem these bits
 *	   should be turned off in the mask byte for the control port test.
 *
 *	3) Set the data and control ports to a value of 0
 */
int
lpt_gsc_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	register struct confargs *ca = aux;
	bus_space_handle_t ioh;
	bus_addr_t base;
	u_int8_t mask, data;
	int i, rv;

	if (ca->ca_type.iodc_type != HPPA_TYPE_FIO ||
	    ca->ca_type.iodc_sv_model != HPPA_FIO_CENT)
		return 0;

#ifdef DEBUG
#define	ABORT								     \
	do {								     \
		printf("lpt_gsc_probe: mask %x data %x failed\n", mask,	     \
		    data);						     \
		return 0;						     \
	} while (0)
#else
#define	ABORT	return 0
#endif

	base = ca->ca_hpa + LPTGSC_OFFSET;
	ioh = ca->ca_hpa + LPTGSC_OFFSET;

	rv = 0;
	mask = 0xff;

	data = 0x55;				/* Alternating zeros */
	if (!lpt_port_test(ca->ca_iot, ioh, base, lpt_data, data, mask))
		ABORT;

	data = 0xaa;				/* Alternating ones */
	if (!lpt_port_test(ca->ca_iot, ioh, base, lpt_data, data, mask))
		ABORT;

	for (i = 0; i < CHAR_BIT; i++) {	/* Walking zero */
		data = ~(1 << i);
		if (!lpt_port_test(ca->ca_iot, ioh, base, lpt_data, data, mask))
			ABORT;
	}

	for (i = 0; i < CHAR_BIT; i++) {	/* Walking one */
		data = (1 << i);
		if (!lpt_port_test(ca->ca_iot, ioh, base, lpt_data, data, mask))
			ABORT;
	}

	bus_space_write_1(ca->ca_iot, ioh, lpt_data, 0);
	bus_space_write_1(ca->ca_iot, ioh, lpt_control, 0);

	return 1;
}

void
lpt_gsc_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct lpt_softc *sc = (void *)self;
	register struct gsc_attach_args *ga = aux;

	/* sc->sc_flags |= LPT_POLLED; */

	sc->sc_state = 0;
	sc->sc_iot = ga->ga_iot;
	sc->sc_ioh = ga->ga_hpa + LPTGSC_OFFSET;

	lpt_attach_subr(sc);

	sc->sc_ih = gsc_intr_establish((struct gsc_softc *)parent, IPL_TTY,
				       ga->ga_irq, lptintr, sc, &sc->sc_dev);
	printf("\n");
}
