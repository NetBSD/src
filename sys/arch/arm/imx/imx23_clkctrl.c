/* $Id: imx23_clkctrl.c,v 1.1.10.3 2017/12/03 11:35:53 jdolecek Exp $ */

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

#include <arm/imx/imx23_clkctrlreg.h>
#include <arm/imx/imx23_clkctrlvar.h>
#include <arm/imx/imx23var.h>

typedef struct clkctrl_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_hdl;
} *clkctrl_softc_t;

static int	clkctrl_match(device_t, cfdata_t, void *);
static void	clkctrl_attach(device_t, device_t, void *);
static int	clkctrl_activate(device_t, enum devact);

static void     clkctrl_init(struct clkctrl_softc *);

static clkctrl_softc_t _sc = NULL;

CFATTACH_DECL3_NEW(clkctrl,
        sizeof(struct clkctrl_softc),
        clkctrl_match,
        clkctrl_attach,
        NULL,
        clkctrl_activate,
        NULL,
        NULL,
        0
);

#define CLKCTRL_RD(sc, reg)                                                 \
        bus_space_read_4(sc->sc_iot, sc->sc_hdl, (reg))
#define CLKCTRL_WR(sc, reg, val)                                            \
        bus_space_write_4(sc->sc_iot, sc->sc_hdl, (reg), (val))

#define CLKCTRL_SOFT_RST_LOOP 455 /* At least 1 us ... */

static int
clkctrl_match(device_t parent, cfdata_t match, void *aux)
{
	struct apb_attach_args *aa = aux;

	if ((aa->aa_addr == HW_CLKCTRL_BASE) &&
	    (aa->aa_size == HW_CLKCTRL_SIZE))
		return 1;

	return 0;
}

static void
clkctrl_attach(device_t parent, device_t self, void *aux)
{
	struct clkctrl_softc *sc = device_private(self);
	struct apb_attach_args *aa = aux;
	static int clkctrl_attached = 0;

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_iot;
	
	if (clkctrl_attached) {
		aprint_error_dev(sc->sc_dev, "already attached\n");
		return;
	}

	if (bus_space_map(sc->sc_iot, aa->aa_addr, aa->aa_size, 0,
	    &sc->sc_hdl))
	{
		aprint_error_dev(sc->sc_dev, "Unable to map bus space\n");
		return;
	}


	clkctrl_init(sc);

	aprint_normal("\n");

	clkctrl_attached = 1;

	return;
}

static int
clkctrl_activate(device_t self, enum devact act)
{

	return EOPNOTSUPP;
}

static void    
clkctrl_init(struct clkctrl_softc *sc)
{
	_sc = sc;
	return;
}

/*
 * Power up 8-phase PLL outputs for USB PHY 
 *
 */
void
clkctrl_en_usb(void)    
{
	struct clkctrl_softc *sc = _sc;

        if (sc == NULL) {
                aprint_error("clkctrl is not initalized");
                return;
        }

	CLKCTRL_WR(sc, HW_CLKCTRL_PLLCTRL0_SET,
	    HW_CLKCTRL_PLLCTRL0_EN_USB_CLKS);

	return;
}

/*
 * Enable 24MHz clock for the Digital Filter. 
 *
 */
void
clkctrl_en_filtclk(void)
{
	struct clkctrl_softc *sc = _sc;

	if (sc == NULL) {
		aprint_error("clkctrl is not initalized");
		return;
	}

	CLKCTRL_WR(sc, HW_CLKCTRL_XTAL_CLR, HW_CLKCTRL_XTAL_FILT_CLK24M_GATE);

	return;
}
