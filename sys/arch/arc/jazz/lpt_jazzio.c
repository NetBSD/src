/*	$NetBSD: lpt_jazzio.c,v 1.8.32.1 2011/06/06 09:05:00 jruoho Exp $	*/
/*	$OpenBSD: lpt_lbus.c,v 1.3 1997/04/10 16:29:17 pefo Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: lpt_jazzio.c,v 1.8.32.1 2011/06/06 09:05:00 jruoho Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ic/lptreg.h>
#include <dev/ic/lptvar.h>

#include <arc/jazz/jazziovar.h>

int lpt_jazzio_probe(device_t, cfdata_t , void *);
void lpt_jazzio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(lpt_jazzio, sizeof(struct lpt_softc),
    lpt_jazzio_probe, lpt_jazzio_attach, NULL, NULL);

/*
 * XXX - copied from lpt_isa.c
 *	sys/arch/arm32/mainbus/lpt_pioc.c also copies this.
 *	sys/arch/amiga/dev/lpt_supio.c doesn't.
 */
static int lpt_port_test(bus_space_tag_t, bus_space_handle_t, bus_addr_t,
		bus_size_t, u_char, u_char);
/*
 * Internal routine to lptprobe to do port tests of one byte value.
 */
static int
lpt_port_test(bus_space_tag_t iot, bus_space_handle_t ioh, bus_addr_t base,
    bus_size_t off, u_char data, u_char mask)
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
	return temp == data;
}

int
lpt_jazzio_probe(device_t parent, cfdata_t match, void *aux)
{
	struct jazzio_attach_args *ja = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_addr_t base;
	uint8_t mask, data;
	int i, rv;

#ifdef DEBUG
#define	ABORT								     \
	do {								     \
		printf("lpt_jazzio_probe: mask %x data %x failed\n", mask,   \
		    data);						     \
		goto out;						     \
	} while (0)
#else
#define	ABORT	goto out
#endif

	if (strcmp(ja->ja_name, "LPT1") != 0)
		 return (0);

	iot = ja->ja_bust;
	base = ja->ja_addr;
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

	rv = 1;

out:
	bus_space_unmap(iot, ioh, LPT_NPORTS);
	return rv;
}

void
lpt_jazzio_attach(device_t parent, device_t self, void *aux)
{
	struct lpt_softc *sc = device_private(self);
	struct jazzio_attach_args *ja = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_state = 0;
	iot = sc->sc_iot = ja->ja_bust;
	if (bus_space_map(iot, ja->ja_addr, LPT_NPORTS, 0, &ioh)) {
		aprint_error_dev(self, "can't map i/o space\n");
		return;
	}
	sc->sc_ioh = ioh;

	bus_space_write_1(iot, ioh, lpt_control, LPC_NINIT);

	jazzio_intr_establish(ja->ja_intr, lptintr, sc);
}
