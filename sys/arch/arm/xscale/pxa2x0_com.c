/*	$NetBSD: pxa2x0_com.c,v 1.11 2009/08/04 12:11:33 kiyohara Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: pxa2x0_com.c,v 1.11 2009/08/04 12:11:33 kiyohara Exp $");

#include "opt_com.h"

#ifndef COM_PXA2X0
#error "You must use options COM_PXA2X0 to get PXA2x0 serial port support"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <arm/xscale/pxa2x0cpu.h>
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>

#include "locators.h"

static int	pxauart_match(device_t, cfdata_t , void *);
static void	pxauart_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(pxauart, sizeof(struct com_softc),
    pxauart_match, pxauart_attach, NULL, NULL);

static int
pxauart_match(device_t parent, cfdata_t cf, void *aux)
{
	struct pxaip_attach_args *pxa = aux;
	bus_space_tag_t bt = &pxa2x0_a4x_bs_tag;	/* XXX: This sucks */
	bus_space_handle_t bh;
	struct pxa2x0_gpioconf *gpioconf;
	u_int gpio;
	int rv, i;

	switch (pxa->pxa_addr) {
	case PXA2X0_FFUART_BASE:
		if (pxa->pxa_intr != PXA2X0_INT_FFUART)
			return (0);
		gpioconf = CPU_IS_PXA250 ? pxa25x_com_ffuart_gpioconf :
			    pxa27x_com_ffuart_gpioconf;
		break;

	case PXA2X0_STUART_BASE:
		if (pxa->pxa_intr != PXA2X0_INT_STUART)
			return (0);
		gpioconf = CPU_IS_PXA250 ? pxa25x_com_stuart_gpioconf :
			    pxa27x_com_stuart_gpioconf;
		break;

	case PXA2X0_BTUART_BASE:	/* XXX: Config file option ... */
		if (pxa->pxa_intr != PXA2X0_INT_BTUART)
			return (0);
		gpioconf = CPU_IS_PXA250 ? pxa25x_com_btuart_gpioconf :
			    pxa27x_com_btuart_gpioconf;
		break;

	case PXA2X0_HWUART_BASE:
		if (pxa->pxa_intr != PXA2X0_INT_HWUART)
			return (0);
		if (CPU_IS_PXA270)
			return (0);
		gpioconf = pxa25x_com_hwuart_gpioconf;
		break;

	default:
		return (0);
	}
	for (i = 0; gpioconf[i].pin != -1; i++) {
		gpio = pxa2x0_gpio_get_function(gpioconf[i].pin);
		if (GPIO_FN(gpio) != GPIO_FN(gpioconf[i].value) ||
		    GPIO_FN_IS_OUT(gpio) != GPIO_FN_IS_OUT(gpioconf[i].value))
			return (0);
	}

	pxa->pxa_size = 0x20;

	if (com_is_console(bt, pxa->pxa_addr, NULL))
		return (1);

	if (bus_space_map(bt, pxa->pxa_addr, pxa->pxa_size, 0, &bh))
		return (0);

	/* Make sure the UART is enabled */
	bus_space_write_1(bt, bh, com_ier, IER_EUART);

	rv = comprobe1(bt, bh);
	bus_space_unmap(bt, bh, pxa->pxa_size);

	return (rv);
}

static void
pxauart_attach(device_t parent, device_t self, void *aux)
{
	struct com_softc *sc = device_private(self);
	struct pxaip_attach_args *pxa = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_addr_t iobase;

	sc->sc_dev = self;
	iot = &pxa2x0_a4x_bs_tag;	/* XXX: This sucks */
	iobase = pxa->pxa_addr;
	sc->sc_frequency = PXA2X0_COM_FREQ;
	sc->sc_type = COM_TYPE_PXA2x0;

	if (com_is_console(iot, iobase, &ioh) == 0 &&
	    bus_space_map(iot, iobase, pxa->pxa_size, 0, &ioh)) {
		aprint_error(": can't map registers\n");
		return;
	}
	COM_INIT_REGS(sc->sc_regs, iot, ioh, iobase);

	com_attach_subr(sc);

	pxa2x0_intr_establish(pxa->pxa_intr, IPL_SERIAL, comintr, sc);
}
