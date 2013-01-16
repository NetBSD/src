/* $Id: imx23_plcom.c,v 1.1.2.2 2013/01/16 05:32:47 yamt Exp $ */

/*
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Petri Laakso
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* Interface to plcom (PL011) serial driver. */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/termios.h>

#include <arm/pic/picvar.h>

#include <arm/imx/imx23var.h>
#include <arm/imx/imx23_uartdbgreg.h>

#include <evbarm/dev/plcomreg.h>
#include <evbarm/dev/plcomvar.h>

static int imx23_plcom_match(device_t, cfdata_t, void *);
static void imx23_plcom_attach(device_t, device_t, void *);

CFATTACH_DECL3_NEW(imx23plcom,
	sizeof(struct plcom_softc),
	imx23_plcom_match,
	imx23_plcom_attach,
	NULL,
	NULL,
	NULL,
	NULL,
	0);

static int
imx23_plcom_match(device_t parent, cfdata_t cf, void *aux)
{
	struct apb_attach_args *aaa = aux;

	if (aaa->aa_addr == HW_UARTDBG_BASE &&
	    aaa->aa_size == PL011COM_UART_SIZE) {
		return 1;
	}

	return 0;
}

static void
imx23_plcom_attach(device_t parent, device_t self, void *aux)
{
	static int plcom_attached = 0;
	struct plcom_softc *sc = device_private(self);
	struct apb_attach_args *aaa = aux;
	void *ih;

	if (plcom_attached)
		return;

	sc->sc_dev = self;
	sc->sc_frequency = IMX23_UART_CLK;
	sc->sc_hwflags = PLCOM_HW_TXFIFO_DISABLE;
	sc->sc_swflags = 0;
	sc->sc_set_mcr = NULL;
	sc->sc_set_mcr_arg = NULL;

	sc->sc_pi.pi_type = PLCOM_TYPE_PL011;
	sc->sc_pi.pi_flags = PLC_FLAG_32BIT_ACCESS;
	sc->sc_pi.pi_iot = aaa->aa_iot;
	sc->sc_pi.pi_iobase = aaa->aa_addr;

	if (bus_space_map(aaa->aa_iot, aaa->aa_addr, PL011COM_UART_SIZE, 0,
	    &sc->sc_pi.pi_ioh)) {
		aprint_error_dev(sc->sc_dev, "unable to map device\n");
		return;
	}

	plcom_attach_subr(sc);

	ih = intr_establish(aaa->aa_irq, IPL_SERIAL, IST_LEVEL,
		plcomintr, sc);

	if (ih == NULL)
		panic("%s: cannot install interrupt handler",
		    device_xname(sc->sc_dev));

	plcom_attached = 1;

	return;
}
