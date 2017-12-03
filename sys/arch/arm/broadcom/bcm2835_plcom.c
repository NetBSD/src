/*	$NetBSD: bcm2835_plcom.c,v 1.1.4.1 2017/12/03 11:35:52 jdolecek Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bcm2835_plcom.c,v 1.1.4.1 2017/12/03 11:35:52 jdolecek Exp $");

#include <sys/types.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/termios.h>
#include <sys/bus.h>

#include <arm/broadcom/bcm_amba.h>
#include <arm/broadcom/bcm2835_intr.h>
#include <arm/broadcom/bcm2835reg.h>

#include <evbarm/dev/plcomreg.h>
#include <evbarm/dev/plcomvar.h>

static int bcm2835_plcom_match(device_t, cfdata_t, void *);
static void bcm2835_plcom_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(bcmplcom, sizeof(struct plcom_softc),
    bcm2835_plcom_match, bcm2835_plcom_attach, NULL, NULL);

static int
bcm2835_plcom_match(device_t parent, cfdata_t cf, void *aux)
{
	struct amba_attach_args *aaa = aux;

	if (strcmp(aaa->aaa_name, "plcom") != 0)
		return 0;

	return 1;
}

static void
bcm2835_plcom_attach(device_t parent, device_t self, void *aux)
{
	struct plcom_softc *sc = device_private(self);
	prop_dictionary_t dict = device_properties(self);
	struct amba_attach_args *aaa = aux;
	void *ih;

	sc->sc_dev = self;
	sc->sc_frequency = BCM2835_UART0_CLK;

        /* Fetch the UART clock frequency from property if set. */
	prop_number_t frequency = prop_dictionary_get(dict, "frequency");
        if (frequency != NULL) {
		sc->sc_frequency = prop_number_integer_value(frequency);
        }

	sc->sc_hwflags = PLCOM_HW_TXFIFO_DISABLE;
	sc->sc_swflags = 0;
	sc->sc_set_mcr = NULL;
	sc->sc_set_mcr_arg = NULL;

	sc->sc_pi.pi_type = PLCOM_TYPE_PL011;
	sc->sc_pi.pi_flags = PLC_FLAG_32BIT_ACCESS;
	sc->sc_pi.pi_iot = aaa->aaa_iot;
	sc->sc_pi.pi_iobase = aaa->aaa_addr;

	if (bus_space_map(aaa->aaa_iot, aaa->aaa_addr, PL011COM_UART_SIZE, 0,
	    &sc->sc_pi.pi_ioh)) {
		aprint_error_dev(sc->sc_dev, "unable to map device\n");
		return;
	}

	plcom_attach_subr(sc);
	ih = intr_establish(aaa->aaa_intr, IPL_SERIAL, IST_LEVEL, plcomintr, sc);
	if (ih == NULL)
		panic("%s: cannot install interrupt handler",
		    device_xname(sc->sc_dev));
}
