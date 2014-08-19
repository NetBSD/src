/* $Id: imx23_digctl.c,v 1.1.10.2 2014/08/20 00:02:46 tls Exp $ */

/*
* Copyright (c) 2013 The NetBSD Foundation, Inc.
* All rights reserved.
*
* This code is derived from software contributed to The NetBSD Foundation
* by Petri Laakso.
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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/bus.h>
#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/timetc.h>

#include <arm/imx/imx23_digctlreg.h>
#include <arm/imx/imx23_digctlvar.h>
#include <arm/imx/imx23var.h>

typedef struct digctl_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_hdl;
} *digctl_softc_t;

static int	digctl_match(device_t, cfdata_t, void *);
static void	digctl_attach(device_t, device_t, void *);
static int	digctl_activate(device_t, enum devact);

static void     digctl_reset(struct digctl_softc *);
static void     digctl_init(struct digctl_softc *);

/* timecounter. */
static u_int digctl_tc_get_timecount(struct timecounter *);

static digctl_softc_t _sc = NULL;

CFATTACH_DECL3_NEW(digctl,
        sizeof(struct digctl_softc),
        digctl_match,
        digctl_attach,
        NULL,
        digctl_activate,
        NULL,
        NULL,
        0
);

static struct timecounter tc_useconds;

#define DCTL_RD(sc, reg)                                                 \
        bus_space_read_4(sc->sc_iot, sc->sc_hdl, (reg))
#define DCTL_WR(sc, reg, val)                                            \
        bus_space_write_4(sc->sc_iot, sc->sc_hdl, (reg), (val))

static int
digctl_match(device_t parent, cfdata_t match, void *aux)
{
	struct apb_attach_args *aa = aux;

	if ((aa->aa_addr == HW_DIGCTL_BASE) && (aa->aa_size == HW_DIGCTL_SIZE))
		return 1;

	return 0;
}

static void
digctl_attach(device_t parent, device_t self, void *aux)
{
	struct digctl_softc *sc = device_private(self);
	struct apb_attach_args *aa = aux;
	static int digctl_attached = 0;

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_iot;
	
	if (digctl_attached) {
		aprint_error_dev(sc->sc_dev, "already attached\n");
		return;
	}

	if (bus_space_map(sc->sc_iot, aa->aa_addr, aa->aa_size, 0,
	    &sc->sc_hdl))
	{
		aprint_error_dev(sc->sc_dev, "Unable to map bus space\n");
		return;
	}

	digctl_reset(sc);
	digctl_init(sc);

	aprint_normal("\n");

	/*
	 * Setup timecounter to use digctl microseconds counter.
	 */
	tc_useconds.tc_get_timecount = digctl_tc_get_timecount;
	tc_useconds.tc_poll_pps = NULL;
	tc_useconds.tc_counter_mask = 0xffffffff; /* 32bit counter. */
	tc_useconds.tc_frequency = 1000000;       /* @ 1MHz */
	tc_useconds.tc_name = "digctl";
	tc_useconds.tc_quality = 100;

	/* Enable counter. */
	DCTL_WR(sc, HW_DIGCTL_CTRL_CLR, HW_DIGCTL_CTRL_XTAL24M_GATE);

	tc_init(&tc_useconds);

	digctl_attached = 1;

	return;
}

static int
digctl_activate(device_t self, enum devact act)
{

	return EOPNOTSUPP;
}

/*
 * Inspired by i.MX23 RM "39.3.10 Correct Way to Soft Reset a Block"
 */
static void
digctl_reset(struct digctl_softc *sc)
{
        return;
}

static void    
digctl_init(struct digctl_softc *sc)
{
	_sc = sc;
	return;
}

/*
 * Control USB controller clocks.
 */
void
digctl_usb_clkgate(int value)
{
	struct digctl_softc *sc = _sc;

	if (sc == NULL) {
		aprint_error("digctl is not initalized");
		return;
	}

	if (value) {
		/* Clocks OFF. */
		DCTL_WR(sc, HW_DIGCTL_CTRL_SET, HW_DIGCTL_CTRL_USB_CLKGATE);
	} else {
		/* Clocks ON. */
		DCTL_WR(sc, HW_DIGCTL_CTRL_CLR, HW_DIGCTL_CTRL_USB_CLKGATE);
	}

	return;
}

/*
 *
 */
static u_int
digctl_tc_get_timecount(struct timecounter *tc)
{
	struct digctl_softc *sc = _sc;
	return DCTL_RD(sc, HW_DIGCTL_MICROSECONDS);
}
