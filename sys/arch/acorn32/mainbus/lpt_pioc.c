/*	$NetBSD: lpt_pioc.c,v 1.10.32.1 2011/06/06 09:04:40 jruoho Exp $	*/

/*
 * Copyright (c) 1997 Mark Brinicombe
 * Copyright (c) 1997 Causality Limited
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
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTERS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Device Driver for AT parallel printer port
 */

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: lpt_pioc.c,v 1.10.32.1 2011/06/06 09:04:40 jruoho Exp $");

#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <acorn32/mainbus/piocvar.h>
#include <dev/ic/lptreg.h>
#include <dev/ic/lptvar.h>

#include "locators.h"

/* Prototypes for functions */

static int lpt_port_test (bus_space_tag_t, bus_space_handle_t, bus_addr_t,
    bus_size_t,	u_char, u_char);
static int lptprobe (bus_space_tag_t, u_int);
static int  lpt_pioc_probe  (device_t, cfdata_t , void *);
static void lpt_pioc_attach (device_t, device_t, void *);

/* device attach structure */

CFATTACH_DECL_NEW(lpt_pioc, sizeof(struct lpt_softc),
    lpt_pioc_probe, lpt_pioc_attach, NULL, NULL);

/*
 * Internal routine to lptprobe to do port tests of one byte value.
 */
static int
lpt_port_test(bus_space_tag_t iot, bus_space_handle_t ioh,
    bus_addr_t base, bus_size_t off, u_char data, u_char mask)
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
static int
lptprobe(bus_space_tag_t iot, u_int iobase)
{
	bus_space_handle_t ioh;
	u_char mask, data;
	int i, rv;
#ifdef DEBUG
#define	ABORT	do {printf("lptprobe: mask %x data %x failed\n", mask, data); \
		    goto out;} while (0)
#else
#define	ABORT	goto out
#endif

	if (bus_space_map(iot, iobase, LPT_NPORTS, 0, &ioh))
		return 0;
	rv = 0;
	mask = 0xff;

	data = 0x55;				/* Alternating zeros */
	if (!lpt_port_test(iot, ioh, iobase, lpt_data, data, mask))
		ABORT;

	data = 0xaa;				/* Alternating ones */
	if (!lpt_port_test(iot, ioh, iobase, lpt_data, data, mask))
		ABORT;

	for (i = 0; i < CHAR_BIT; i++) {	/* Walking zero */
		data = ~(1 << i);
		if (!lpt_port_test(iot, ioh, iobase, lpt_data, data, mask))
			ABORT;
	}

	for (i = 0; i < CHAR_BIT; i++) {	/* Walking one */
		data = (1 << i);
		if (!lpt_port_test(iot, ioh, iobase, lpt_data, data, mask))
			ABORT;
	}

	bus_space_write_1(iot, ioh, lpt_data, 0);
	bus_space_write_1(iot, ioh, lpt_control, 0);

	rv = LPT_NPORTS;

out:
	bus_space_unmap(iot, ioh, LPT_NPORTS);
	return rv;
}

/*
 * int lpt_pioc_probe(device_t parent, cfdata_t cf, void *aux)
 *
 * Make sure we are trying to attach a lpt device and then
 * probe for one.
 */

static int
lpt_pioc_probe(device_t parent, cfdata_t match, void *aux)
{
	struct pioc_attach_args *pa = aux;
	int rv;

	if (pa->pa_name && strcmp(pa->pa_name, "lpt") != 0)
		return(0);

	/* We need an offset */
	if (pa->pa_offset == PIOCCF_OFFSET_DEFAULT)
		return(0);

	rv = lptprobe(pa->pa_iot, pa->pa_iobase + pa->pa_offset);

	if (rv) {
		pa->pa_iosize = rv;
		return(1);
	}
	return(0);
}

/*
 * void lpt_pioc_attach(device_t parent, device_t self, void *aux)
 *
 * attach the lpt device
 */

static void
lpt_pioc_attach(device_t parent, device_t self, void *aux)
{
	struct lpt_softc *sc = device_private(self);
	struct pioc_attach_args *pa = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	u_int iobase;

	sc->sc_dev = self;
	if (pa->pa_irq != MAINBUSCF_IRQ_DEFAULT)
		aprint_normal("\n");
	else
		aprint_normal(": polled\n");

	iobase = pa->pa_iobase + pa->pa_offset;

	iot = sc->sc_iot = pa->pa_iot;
	if (bus_space_map(iot, iobase, LPT_NPORTS, 0, &ioh)) {
		aprint_error_dev(self, "couldn't map I/O ports");
		return;
	}
	sc->sc_ioh = ioh;

	lpt_attach_subr(sc);

	if (pa->pa_irq != MAINBUSCF_IRQ_DEFAULT)
		sc->sc_ih = intr_claim(pa->pa_irq, IPL_TTY, "lpt",
		    lptintr, sc);
}

/* End of lpt_pioc.c */
