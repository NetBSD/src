/*	$NetBSD: obio_com.c,v 1.4.26.1 2014/08/10 06:53:52 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: obio_com.c,v 1.4.26.1 2014/08/10 06:53:52 tls Exp $");

#include "opt_omap.h"
#include "opt_com.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <machine/intr.h>
#include <sys/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <arm/omap/omap2_obiovar.h>
#include <arm/omap/omap2_reg.h>
#include <arm/omap/omap_com.h>

#include "locators.h"

static int	obiouart_match(device_t, cfdata_t, void *);
static void	obiouart_attach(device_t, device_t, void *);
static int	uart_enable(struct obio_attach_args *);
static void	obiouart_callout(void *);

struct com_obio_softc {
	struct com_softc sc_sc;
	struct callout sc_callout;
};

CFATTACH_DECL_NEW(obiouart, sizeof(struct com_obio_softc),
    obiouart_match, obiouart_attach, NULL, NULL);

static int
obiouart_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args *obio = aux;
	bus_space_handle_t bh;
	int rv;

	if (obio->obio_addr == OBIOCF_ADDR_DEFAULT)
		panic("obiouart must have addr specified in config.");

#if 0
	/*
	 * XXX this should be ifdefed on a board-dependent switch
	 * We don't know what is the irq for com0 on the sdp2430 
	 */
	if (obio->obio_intr == OBIOCF_INTR_DEFAULT)
		panic("obiouart must have intr specified in config.");
#endif

	if (obio->obio_size == OBIOCF_SIZE_DEFAULT)
		obio->obio_size = OMAP_COM_SIZE;

	if (com_is_console(obio->obio_iot, obio->obio_addr, NULL))
		return 1;

	if (uart_enable(obio) != 0)
		return 1;

	if (bus_space_map(obio->obio_iot, obio->obio_addr, obio->obio_size,
			  0, &bh))
		return 1;

	rv = comprobe1(obio->obio_iot, bh);

	bus_space_unmap(obio->obio_iot, bh, obio->obio_size);

	return rv;
}

static void
obiouart_attach(device_t parent, device_t self, void *aux)
{
	struct com_obio_softc *osc = device_private(self);
	struct com_softc *sc = &osc->sc_sc;
	struct obio_attach_args *obio = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh = 0;
	bus_addr_t iobase;

	sc->sc_dev = self;

	iot = obio->obio_iot;
	iobase = obio->obio_addr;
	sc->sc_frequency = OMAP_COM_FREQ;
	sc->sc_type = COM_TYPE_NORMAL;

	if (com_is_console(iot, iobase, &ioh) == 0 &&
	    bus_space_map(iot, iobase, obio->obio_size, 0, &ioh)) {
		panic(": can't map registers\n");
		return;
	}
	COM_INIT_REGS(sc->sc_regs, iot, ioh, iobase);

	com_attach_subr(sc);
	aprint_naive("\n");

	if (obio->obio_intr != OBIOCF_INTR_DEFAULT) {
		intr_establish(obio->obio_intr, IPL_SERIAL, IST_LEVEL,
			comintr, sc);
	} else {
		callout_init(&osc->sc_callout, 0);
		callout_reset(&osc->sc_callout, 1, obiouart_callout, osc);
	}
}

static void
obiouart_callout(void *arg)
{
	struct com_obio_softc * const osc = arg;
	int s;
	s = splserial();
	comintr(arg);
	splx(s);
	callout_schedule(&osc->sc_callout, 1);
}


static int
uart_enable(struct obio_attach_args *obio)
{
	bus_space_handle_t ioh;
	uint32_t r;
#if 0
	uint32_t clksel2;
#endif
	uint32_t fclken1;
	uint32_t iclken1;
	int err __diagused;
	int n=-1;

	KASSERT(obio != NULL);

	err = bus_space_map(obio->obio_iot, OMAP2_CM_BASE,
		OMAP2_CM_SIZE, 0, &ioh);
	KASSERT(err == 0);


	switch(obio->obio_addr) {
	case 0x4806a000:
		fclken1 = OMAP2_CM_FCLKEN1_CORE_EN_UART1;
		iclken1 = OMAP2_CM_ICLKEN1_CORE_EN_UART1;
		n = 1;
		break;
	case 0x4806c000:
		fclken1 = OMAP2_CM_FCLKEN1_CORE_EN_UART2;
		iclken1 = OMAP2_CM_ICLKEN1_CORE_EN_UART2;
		n = 2;
		break;
	case 0x4806e000:
		fclken1 = OMAP2_CM_FCLKEN2_CORE_EN_UART3;
		iclken1 = OMAP2_CM_ICLKEN2_CORE_EN_UART3;
		n = 3;
		break;
	default:
		goto err;
	}

printf("%s: UART#%d\n", __func__, n);

#if 0
	r = bus_space_read_4(obio->obio_iot, ioh, OMAP2_CM_CLKSEL2_CORE);
	r |= clksel2;
	bus_space_write_4(obio->obio_iot, ioh, OMAP2_CM_CLKSEL2_CORE, r);
#endif

	r = bus_space_read_4(obio->obio_iot, ioh, OMAP2_CM_FCLKEN1_CORE);
	r |= fclken1;
	bus_space_write_4(obio->obio_iot, ioh, OMAP2_CM_FCLKEN1_CORE, r);

	r = bus_space_read_4(obio->obio_iot, ioh, OMAP2_CM_ICLKEN1_CORE);
	r |= iclken1;
	bus_space_write_4(obio->obio_iot, ioh, OMAP2_CM_ICLKEN1_CORE, r);

err:
//	bus_space_unmap(obio->obio_iot, ioh, OMAP2_CM_SIZE);

	return 0;
}
