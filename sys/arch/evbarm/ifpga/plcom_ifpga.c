/*      $NetBSD: plcom_ifpga.c,v 1.14.2.2 2013/02/25 00:28:36 tls Exp $ */

/*
 * Copyright (c) 2001 ARM Ltd
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
 * 3. The name of the company may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Interface to plcom (PL010) serial driver. */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: plcom_ifpga.c,v 1.14.2.2 2013/02/25 00:28:36 tls Exp $");

#include <sys/types.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/malloc.h>

#include <sys/termios.h>

#include <machine/intr.h>
#include <sys/bus.h>

#include <evbarm/dev/plcomreg.h>
#include <evbarm/dev/plcomvar.h>

#include <evbarm/ifpga/plcom_ifpgavar.h>

#include <evbarm/ifpga/ifpgareg.h>
#include <evbarm/ifpga/ifpgavar.h>

static int  plcom_ifpga_match(device_t, cfdata_t, void *);
static void plcom_ifpga_attach(device_t, device_t, void *);
static void plcom_ifpga_set_mcr(void *, int, u_int);

CFATTACH_DECL_NEW(plcom_ifpga, sizeof(struct plcom_ifpga_softc),
    plcom_ifpga_match, plcom_ifpga_attach, NULL, NULL);

static int
plcom_ifpga_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
plcom_ifpga_attach(device_t parent, device_t self, void *aux)
{
	struct plcom_ifpga_softc *isc = device_private(self);
	struct plcom_softc *sc = &isc->sc_plcom;
	struct ifpga_attach_args *ifa = aux;

	isc->sc_iot = ifa->ifa_iot;
	isc->sc_ioh = ifa->ifa_sc_ioh;

	sc->sc_dev = self;
#if defined(INTEGRATOR_CP)
	sc->sc_pi.pi_type = PLCOM_TYPE_PL011;
#else
	sc->sc_pi.pi_type = PLCOM_TYPE_PL010;
#endif
	sc->sc_pi.pi_iot = ifa->ifa_iot;
	sc->sc_pi.pi_iobase = ifa->ifa_addr;
	sc->sc_pi.pi_size = IFPGA_UART_SIZE;
	sc->sc_frequency = IFPGA_UART_CLK;
	sc->sc_hwflags = 0;
	sc->sc_swflags = 0;
	sc->sc_set_mcr = plcom_ifpga_set_mcr;
	sc->sc_set_mcr_arg = (void *)isc;

	if (bus_space_map(ifa->ifa_iot, ifa->ifa_addr, IFPGA_UART_SIZE, 0,
	    &sc->sc_pi.pi_ioh)) {
		printf("%s: unable to map device\n", device_xname(sc->sc_dev));
		return;
	}

	plcom_attach_subr(sc);
	isc->sc_ih = ifpga_intr_establish(ifa->ifa_irq, IPL_SERIAL,
	    plcomintr, sc);
	if (isc->sc_ih == NULL)
		panic("%s: cannot install interrupt handler",
		    device_xname(sc->sc_dev));
}

static void plcom_ifpga_set_mcr(void *aux, int unit, u_int mcr)
{
	struct plcom_ifpga_softc *isc = aux;
	u_int set, clr;

	set = clr = 0;

	switch (unit) {
	case 0:
		if (mcr & PL01X_MCR_RTS)
			set |= IFPGA_SC_CTRL_UART0RTS;
		else
			clr |= IFPGA_SC_CTRL_UART0RTS;
		if (mcr & PL01X_MCR_DTR)
			set |= IFPGA_SC_CTRL_UART0DTR;
		else
			clr |= IFPGA_SC_CTRL_UART0DTR;
	case 1:
		if (mcr & PL01X_MCR_RTS)
			set |= IFPGA_SC_CTRL_UART1RTS;
		else
			clr |= IFPGA_SC_CTRL_UART1RTS;
		if (mcr & PL01X_MCR_DTR)
			set |= IFPGA_SC_CTRL_UART1DTR;
		else
			clr |= IFPGA_SC_CTRL_UART1DTR;
	default:
		return;
	}

	if (set)
		bus_space_write_1(isc->sc_iot, isc->sc_ioh, IFPGA_SC_CTRLS,
		    set);
	if (clr)
		bus_space_write_1(isc->sc_iot, isc->sc_ioh, IFPGA_SC_CTRLC,
		    clr);
}
