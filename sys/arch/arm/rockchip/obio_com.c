/*	$NetBSD: obio_com.c,v 1.3.18.2 2017/12/03 11:35:55 jdolecek Exp $	*/

/*	based on omap/obio_com.c	*/

/*
 * Based on arch/arm/omap/omap_com.c
 *
 * Copyright 2003 Wasabi Systems, Inc.
 * OMAP support Copyright (c) 2007 Microsoft
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
__KERNEL_RCSID(0, "$NetBSD: obio_com.c,v 1.3.18.2 2017/12/03 11:35:55 jdolecek Exp $");

#include "opt_rockchip.h"
/*#include "opt_com.h"*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <machine/intr.h>
#include <sys/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <arm/rockchip/rockchip_var.h>
#include <arm/rockchip/rockchip_reg.h>

#include "locators.h"

static int	obiouart_match(device_t, cfdata_t, void *);
static void	obiouart_attach(device_t, device_t, void *);

struct com_obio_softc {
	struct com_softc sc_sc;
	void *sc_ih;
};

CFATTACH_DECL_NEW(obiouart, sizeof(struct com_obio_softc),
    obiouart_match, obiouart_attach, NULL, NULL);

static int
obiouart_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args *obio = aux;
	bus_space_handle_t bsh;
	bus_addr_t ioaddr;

	switch (obio->obio_base) {
	case ROCKCHIP_CORE0_BASE:
		KASSERT(obio->obio_offset == ROCKCHIP_UART0_OFFSET ||
			obio->obio_offset == ROCKCHIP_UART1_OFFSET);
		break;
	case ROCKCHIP_CORE1_BASE:
		KASSERT(obio->obio_offset == ROCKCHIP_UART2_OFFSET ||
			obio->obio_offset == ROCKCHIP_UART3_OFFSET);
		break;
	default:
		panic("obiouart must have addr specified in config.");
	}

	ioaddr = obio->obio_base + obio->obio_offset;

#if 0
	/*
	 * XXX this should be ifdefed on a board-dependent switch
	 * We don't know what is the irq for com0 on the sdp2430 
	 */
	if (obio->obio_intr == OBIOCF_INTR_DEFAULT)
		panic("obiouart must have intr specified in config.");
#endif

	if (obio->obio_size == OBIOCF_SIZE_DEFAULT)
		obio->obio_size = ROCKCHIP_UART_SIZE;

	if (com_is_console(obio->obio_bst, ioaddr, NULL))
		return 1;

	bus_space_subregion(obio->obio_bst, obio->obio_bsh, obio->obio_size, 0,
	    &bsh);

	return comprobe1(obio->obio_bst, bsh);
}

static void
obiouart_attach(device_t parent, device_t self, void *aux)
{
	struct com_obio_softc *osc = device_private(self);
	struct com_softc *sc = &osc->sc_sc;
	struct obio_attach_args *obio = aux;
	bus_space_tag_t bst;
	bus_space_handle_t bsh = 0;
	bus_addr_t iobase;

	sc->sc_dev = self;

	bst = obio->obio_bst;
	iobase = obio->obio_base + obio->obio_offset;
	sc->sc_frequency = ROCKCHIP_UART_FREQ;
	sc->sc_type = COM_TYPE_NORMAL;

	if (com_is_console(bst, iobase, &bsh) == 0 &&
	    bus_space_subregion(bst, obio->obio_bsh, obio->obio_size, 0, &bsh)) {
		panic(": can't map registers\n");
		return;
	}
	COM_INIT_REGS(sc->sc_regs, bst, bsh, iobase);

	com_attach_subr(sc);
	aprint_naive("\n");

	KASSERT(obio->obio_intr != OBIOCF_INTR_DEFAULT);
	osc->sc_ih = intr_establish(obio->obio_intr, IPL_SERIAL, IST_LEVEL,
			comintr, sc);
	if (osc->sc_ih == NULL)
		panic("%s: failed to establish interrup %d",
		    device_xname(self), obio->obio_intr);
}
