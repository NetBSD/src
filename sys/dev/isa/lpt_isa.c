/*	$NetBSD: lpt_isa.c,v 1.62 2006/10/12 01:31:17 christos Exp $	*/

/*
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

/*
 * Device Driver for AT parallel printer port
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lpt_isa.c,v 1.62 2006/10/12 01:31:17 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/device.h>
#include <sys/syslog.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/isa/isavar.h>
#include <dev/ic/lptreg.h>
#include <dev/ic/lptvar.h>

#define	LPTPRI		(PZERO+8)
#define	LPT_BSIZE	1024

#ifndef LPTDEBUG
#define LPRINTF(a)
#else
#define LPRINTF(a)	if (lpt_isa_debug) printf a
int lpt_isa_debug = 0;
#endif

struct lpt_isa_softc {
	struct lpt_softc sc_lpt;
	int sc_irq;

};

int lpt_isa_probe(struct device *, struct cfdata *, void *);
void lpt_isa_attach(struct device *, struct device *, void *);

CFATTACH_DECL(lpt_isa, sizeof(struct lpt_isa_softc),
    lpt_isa_probe, lpt_isa_attach, NULL, NULL);

int lpt_port_test(bus_space_tag_t, bus_space_handle_t, bus_addr_t,
	    bus_size_t, u_char, u_char);

/*
 * Internal routine to lptprobe to do port tests of one byte value.
 */
int
lpt_port_test(bus_space_tag_t iot, bus_space_handle_t ioh,
    bus_addr_t base __unused, bus_size_t off, u_char data, u_char mask)
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
lpt_isa_probe(struct device *parent __unused, struct cfdata *match __unused,
    void *aux)
{
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	u_long base;
	u_char mask, data;
	int i, rv;

#ifdef LPT_DEBUG
#define	ABORT	do {printf("lptprobe: mask %x data %x failed\n", mask, data); \
		    goto out;} while (0)
#else
#define	ABORT	goto out
#endif

	if (ia->ia_nio < 1)
		return (0);

	if (ISA_DIRECT_CONFIG(ia))
		return (0);

	/* Disallow wildcarded i/o address. */
	if (ia->ia_io[0].ir_addr == ISA_UNKNOWN_PORT)
		return (0);

	iot = ia->ia_iot;
	base = ia->ia_io[0].ir_addr;
	if (bus_space_map(iot, base, LPT_NPORTS, 0, &ioh))
		return 0;

	rv = 0;
	mask = 0xff;

	data = 0x55;				/* Alternating zeros */
	if (!lpt_port_test(iot, ioh, base, lpt_data, data, mask))
		ABORT;

	data = 0xaa;				/* Alternating ones */
	if (!lpt_port_test(iot, ioh, base, lpt_data, data, mask))
		ABORT;

	for (i = 0; i < CHAR_BIT; i++) {	/* Walking zero */
		data = ~(1 << i);
		if (!lpt_port_test(iot, ioh, base, lpt_data, data, mask))
			ABORT;
	}

	for (i = 0; i < CHAR_BIT; i++) {	/* Walking one */
		data = (1 << i);
		if (!lpt_port_test(iot, ioh, base, lpt_data, data, mask))
			ABORT;
	}

	bus_space_write_1(iot, ioh, lpt_data, 0);
	bus_space_write_1(iot, ioh, lpt_control, 0);

	ia->ia_io[0].ir_size = LPT_NPORTS;

	ia->ia_niomem = 0;
	ia->ia_ndrq = 0;

	rv = 1;

out:
	bus_space_unmap(iot, ioh, LPT_NPORTS);
	return rv;
}

void
lpt_isa_attach(struct device *parent __unused, struct device *self, void *aux)
{
	struct lpt_isa_softc *sc = (void *)self;
	struct lpt_softc *lsc = &sc->sc_lpt;
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	if (ia->ia_nirq < 1 ||
	    ia->ia_irq[0].ir_irq == ISA_UNKNOWN_IRQ) {
		sc->sc_irq = -1;
		printf(": polled\n");
	} else {
		sc->sc_irq = ia->ia_irq[0].ir_irq;
		printf("\n");
	}

	iot = lsc->sc_iot = ia->ia_iot;
	if (bus_space_map(iot, ia->ia_io[0].ir_addr, LPT_NPORTS, 0, &ioh)) {
		printf("%s: can't map i/o space\n", self->dv_xname);
		return;
	}
	lsc->sc_ioh = ioh;

	lpt_attach_subr(lsc);

	if (sc->sc_irq != -1)
		lsc->sc_ih = isa_intr_establish(ia->ia_ic, sc->sc_irq,
		    IST_EDGE, IPL_TTY, lptintr, lsc);
}
