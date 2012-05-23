/*	$NetBSD: com_intio.c,v 1.1.4.2 2012/05/23 10:07:50 yamt Exp $	*/

/*
 * Copyright (c) 2009 Tetsuya Isaki. All rights reserved.
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
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * PSX16550, High-speed RS-232C board
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: com_intio.c,v 1.1.4.2 2012/05/23 10:07:50 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <machine/cpu.h>

#include <arch/x68k/dev/intiovar.h>

#define COM_PSX16550_FREQ	(22118400 / 2)
#define COM_PSX16550_SIZE	(COM_NPORTS << 1)
#define COM_PSX16550_REG_VECT	com_scratch

static int  com_intio_match(device_t, cfdata_t, void *);
static void com_intio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(com_intio, sizeof(struct com_softc),
    com_intio_match, com_intio_attach, NULL, NULL);

static int
com_intio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct intio_attach_args *ia = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int iobase;
	int rv;

	/* Check whether the board is inserted or not */
	if (badaddr((void *)IIOV(ia->ia_addr)))
		return 0;

	ia->ia_size = COM_PSX16550_SIZE;
	if (intio_map_allocate_region(parent, ia, INTIO_MAP_TESTONLY) < 0)
		return 0;

	iot = ia->ia_bst;
	iobase = ia->ia_addr;

	/* if it's in use as console, it's there. */
	if (com_is_console(iot, iobase, NULL))
		return 1;

	if (bus_space_map(iot, iobase, ia->ia_size,
	    BUS_SPACE_MAP_SHIFTED_ODD, &ioh))
		return 0;

	rv = comprobe1(iot, ioh);
	bus_space_unmap(iot, ioh, ia->ia_size);

	return rv;
}

static void
com_intio_attach(device_t parent, device_t self, void *aux)
{
	struct com_softc *sc = device_private(self);
	struct intio_attach_args *ia = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int iobase;

	intio_map_allocate_region(parent, ia, INTIO_MAP_ALLOCATE);

	sc->sc_dev = self;
	iot = ia->ia_bst;
	iobase = ia->ia_addr;
	if (bus_space_map(iot, iobase, ia->ia_size,
	    BUS_SPACE_MAP_SHIFTED_ODD, &ioh)) {
		aprint_error(": can't map I/O space\n");
		return;
	}

	/* check and set interrupt vector */
	if (ia->ia_intr < 16 || ia->ia_intr > 255) {
		aprint_error(": invalid intr vector (0x%02x)\n", ia->ia_intr);
		return;
	}
	bus_space_write_1(iot, ioh, COM_PSX16550_REG_VECT, ia->ia_intr);

	sc->sc_frequency = COM_PSX16550_FREQ;
	COM_INIT_REGS(sc->sc_regs, iot, ioh, iobase);

	/* PSX16550 uses MCR_DRS to switch interrupt priority level */
	SET(sc->sc_mcr, MCR_DRS);	/* 0: ipl2 / 1: ipl4 */

	com_attach_subr(sc);

	if (intio_intr_establish(ia->ia_intr, device_xname(self),
	    comintr, sc))
		aprint_error_dev(self, "can't establish interrupt handler\n");
}
