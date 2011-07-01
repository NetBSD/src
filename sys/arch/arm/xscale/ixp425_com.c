/*	$NetBSD: ixp425_com.c,v 1.18 2011/07/01 20:32:51 dyoung Exp $	*/

/*
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
__KERNEL_RCSID(0, "$NetBSD: ixp425_com.c,v 1.18 2011/07/01 20:32:51 dyoung Exp $");

#include "opt_com.h"
#ifndef COM_PXA2X0
#error "You must use options COM_PXA2X0 to get IXP425 serial port support"
#endif

#include <sys/systm.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <machine/intr.h>
#include <sys/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <arm/xscale/ixp425reg.h>
#include <arm/xscale/ixp425var.h>
#include <arm/xscale/ixp425_sipvar.h>

static int	ixsipcom_match(device_t, cfdata_t , void *);
static void	ixsipcom_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ixsipcom, sizeof(struct com_softc),
    ixsipcom_match, ixsipcom_attach, NULL, NULL);

static const int uart_irq[] = {IXP425_INT_UART0, IXP425_INT_UART1};

static int
ixsipcom_match(device_t parent, cfdata_t match, void *aux)
{
	struct ixpsip_attach_args *sa = aux;
	bus_space_tag_t bt = &ixp425_a4x_bs_tag;
	bus_space_handle_t bh;
	int rv;

	if (strcmp(match->cf_name, "ixsipcom") == 0)
		return 1;

	if (com_is_console(bt, sa->sa_addr, NULL))
		return (1);

	if (bus_space_map(bt, sa->sa_addr, sa->sa_size, 0, &bh))
		return (0);

	/* Make sure the UART is enabled */
	bus_space_write_1(bt, bh, com_ier, IER_EUART);

	rv = comprobe1(bt, bh);
	bus_space_unmap(bt, bh, sa->sa_size);

	return (rv);
}

static void
ixsipcom_attach(device_t parent, device_t self, void *aux)
{
	struct com_softc *sc = device_private(self);
	struct ixpsip_attach_args *sa = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_addr_t iobase;

	sc->sc_dev = self;
	iot = &ixp425_a4x_bs_tag;
	iobase = sa->sa_addr;
	sc->sc_frequency = IXP425_UART_FREQ;
	sc->sc_type = COM_TYPE_PXA2x0;

	if (com_is_console(iot, iobase, &ioh) == 0 &&
	    bus_space_map(iot, iobase, sa->sa_size, 0, &ioh)) {
		aprint_error(": can't map registers\n");
		return;
	}
	COM_INIT_REGS(sc->sc_regs, iot, ioh, iobase);

	com_attach_subr(sc);

	ixp425_intr_establish(uart_irq[sa->sa_index], IPL_SERIAL, comintr, sc);
}
