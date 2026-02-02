/* $NetBSD: imx23_digctl.c,v 1.6 2026/02/02 09:21:30 yurix Exp $ */

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

#include <dev/fdt/fdtvar.h>

#include <arm/imx/imx23_digctlreg.h>
#include <arm/imx/imx23var.h>

struct imx23_digctl_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_hdl;
};

static int	imx23_digctl_match(device_t, cfdata_t, void *);
static void	imx23_digctl_attach(device_t, device_t, void *);

static void     imx23_digctl_init(struct imx23_digctl_softc *);

/* timecounter. */
static u_int imx23_digctl_tc_get_timecount(struct timecounter *);

static struct imx23_digctl_softc *_sc = NULL;

CFATTACH_DECL_NEW(imx23digctl, sizeof(struct imx23_digctl_softc),
		  imx23_digctl_match, imx23_digctl_attach, NULL, NULL);

static struct timecounter tc_useconds;

#define DCTL_RD(sc, reg)                                                 \
        bus_space_read_4(sc->sc_iot, sc->sc_hdl, (reg))
#define DCTL_WR(sc, reg, val)                                            \
        bus_space_write_4(sc->sc_iot, sc->sc_hdl, (reg), (val))

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx23-digctl" },
	DEVICE_COMPAT_EOL
};

static int
imx23_digctl_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
imx23_digctl_attach(device_t parent, device_t self, void *aux)
{
	struct imx23_digctl_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;

	bus_addr_t addr;
	bus_size_t size;
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get register address\n");
		return;
	}
	if (bus_space_map(faa->faa_bst, addr, size, 0, &sc->sc_hdl)) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_normal("\n");

	imx23_digctl_init(sc);

	/*
	 * Setup timecounter to use digctl microseconds counter.
	 */
	tc_useconds.tc_get_timecount = imx23_digctl_tc_get_timecount;
	tc_useconds.tc_poll_pps = NULL;
	tc_useconds.tc_counter_mask = 0xffffffff; /* 32bit counter. */
	tc_useconds.tc_frequency = 1000000;       /* @ 1MHz */
	tc_useconds.tc_name = "digctl";
	tc_useconds.tc_quality = 100;

	/* Enable counter. */
	DCTL_WR(sc, HW_DIGCTL_CTRL_CLR, HW_DIGCTL_CTRL_XTAL24M_GATE);

	tc_init(&tc_useconds);

	/* Enable USB clocks */
	DCTL_WR(sc, HW_DIGCTL_CTRL_CLR, HW_DIGCTL_CTRL_USB_CLKGATE);
}

static void
imx23_digctl_init(struct imx23_digctl_softc *sc)
{
	_sc = sc;
	return;
}

/*
 *
 */
static u_int
imx23_digctl_tc_get_timecount(struct timecounter *tc)
{
	struct imx23_digctl_softc *sc = _sc;
	return DCTL_RD(sc, HW_DIGCTL_MICROSECONDS);
}
