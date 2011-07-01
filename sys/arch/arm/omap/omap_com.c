/*	$NetBSD: omap_com.c,v 1.4 2011/07/01 20:30:21 dyoung Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: omap_com.c,v 1.4 2011/07/01 20:30:21 dyoung Exp $");

#include "opt_com.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <machine/intr.h>
#include <sys/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <arm/omap/omap_reg.h>
#include <arm/omap/omap_tipb.h>
#include <arm/omap/omap_com.h>

#include "locators.h"

static int	omapuart_match(device_t, cfdata_t , void *);
static void	omapuart_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(omapuart, sizeof(struct com_softc),
    omapuart_match, omapuart_attach, NULL, NULL);

static int
omapuart_match(device_t parent, cfdata_t cf, void *aux)
{
	struct tipb_attach_args *tipb = aux;
	bus_space_handle_t bh;
	int rv;

	if (tipb->tipb_addr == -1 || tipb->tipb_intr == -1)
	    panic("omapuart must have addr and intr specified in config.");

	if (tipb->tipb_size == 0)
		tipb->tipb_size = OMAP_COM_SIZE;

	if (com_is_console(tipb->tipb_iot, tipb->tipb_addr, NULL))
		return (1);

	if (bus_space_map(tipb->tipb_iot, tipb->tipb_addr, tipb->tipb_size,
			  0, &bh))
		return (0);

	/*
	 * The TRM says the mode should be disabled while dll and dlh are
	 * being changed so we disable before probing, then enable.
	 */
	bus_space_write_1(tipb->tipb_iot, bh,
			  OMAP_COM_MDR1, OMAP_COM_MDR1_MODE_DISABLE);

	rv = comprobe1(tipb->tipb_iot, bh);

	bus_space_write_1(tipb->tipb_iot, bh,
			  OMAP_COM_MDR1, OMAP_COM_MDR1_MODE_UART_16X);

	bus_space_unmap(tipb->tipb_iot, bh, tipb->tipb_size);

	return (rv);
}

static void
omapuart_attach(device_t parent, device_t self, void *aux)
{
	struct com_softc *sc = device_private(self);
	struct tipb_attach_args *tipb = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_addr_t iobase;

	sc->sc_dev = self;
	iot = tipb->tipb_iot;
	iobase = tipb->tipb_addr;
	sc->sc_frequency = OMAP_COM_FREQ;
	sc->sc_type = COM_TYPE_NORMAL;

	if (com_is_console(iot, iobase, &ioh) == 0 &&
	    bus_space_map(iot, iobase, tipb->tipb_size, 0, &ioh)) {
		panic(": can't map registers\n");
		return;
	}
	COM_INIT_REGS(sc->sc_regs, iot, ioh, iobase);

	com_attach_subr(sc);
	aprint_naive("\n");

	omap_intr_establish(tipb->tipb_intr, IPL_SERIAL,
	    device_xname(sc->sc_dev), comintr, sc);
}
