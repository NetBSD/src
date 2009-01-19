/*	$NetBSD: gemini_com.c,v 1.1.2.1 2009/01/19 13:15:57 skrll Exp $	*/

/* adapted from:
 *	NetBSD: omap_com.c,v 1.2 2008/03/14 15:09:09 cube Exp
 */

/*
 * Based on arch/arm/xscale/pxa2x0_com.c
 *
 * Copyright 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gemini_com.c,v 1.1.2.1 2009/01/19 13:15:57 skrll Exp $");

#include "opt_com.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <arm/pic/picvar.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <arm/gemini/gemini_reg.h>
#include <arm/gemini/gemini_obiovar.h>

#include <arm/gemini/gemini_com.h>

#include "locators.h"

static int	gemini_com_match(device_t, cfdata_t , void *);
static void	gemini_com_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(gemini_com, sizeof(struct com_softc),
    gemini_com_match, gemini_com_attach, NULL, NULL);

static int
gemini_com_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args *obio = aux;
	bus_space_handle_t bh;
	int rv;

	if (obio->obio_addr == -1 || obio->obio_intr == -1)
	    panic("gemini_com must have addr and intr specified in config.");

	if (obio->obio_size == 0)
		obio->obio_size = GEMINI_UART_SIZE;

	if (com_is_console(obio->obio_iot, obio->obio_addr, NULL))
		return (1);

	if (bus_space_map(obio->obio_iot, obio->obio_addr, obio->obio_size,
			  0, &bh))
		return (0);

	rv = comprobe1(obio->obio_iot, bh);

	bus_space_unmap(obio->obio_iot, bh, obio->obio_size);

	return (rv);
}

static void
gemini_com_attach(device_t parent, device_t self, void *aux)
{
	struct com_softc *sc = device_private(self);
	struct obio_attach_args *obio = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_addr_t iobase;

	sc->sc_dev = self;
	iot = obio->obio_iot;
	iobase = obio->obio_addr;
	sc->sc_frequency = GEMINI_COM_FREQ;
	sc->sc_type = COM_TYPE_16550_NOERS;

	if (com_is_console(iot, iobase, &ioh) == 0 &&
	    bus_space_map(iot, iobase, obio->obio_size, 0, &ioh)) {
		panic(": can't map registers\n");
		return;
	}
	COM_INIT_REGS(sc->sc_regs, iot, ioh, iobase);

	com_attach_subr(sc);
	aprint_naive("\n");

	intr_establish(obio->obio_intr, IPL_SERIAL, IST_LEVEL_HIGH,
		comintr, sc);
}
