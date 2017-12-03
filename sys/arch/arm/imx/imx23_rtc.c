/* $Id: imx23_rtc.c,v 1.1.18.2 2017/12/03 11:35:53 jdolecek Exp $ */

/*
* Copyright (c) 2014 The NetBSD Foundation, Inc.
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

#include <arm/imx/imx23_rtcreg.h>
#include <arm/imx/imx23_rtcvar.h>
#include <arm/imx/imx23var.h>

typedef struct rtc_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_hdl;
} *rtc_softc_t;

static int	rtc_match(device_t, cfdata_t, void *);
static void	rtc_attach(device_t, device_t, void *);
static int	rtc_activate(device_t, enum devact);

static void     rtc_init(struct rtc_softc *);
static void     rtc_reset(struct rtc_softc *);

static rtc_softc_t _sc = NULL;

CFATTACH_DECL3_NEW(rtc,
        sizeof(struct rtc_softc),
        rtc_match,
        rtc_attach,
        NULL,
        rtc_activate,
        NULL,
        NULL,
        0
);

#define RTC_RD(sc, reg)                                                 \
        bus_space_read_4(sc->sc_iot, sc->sc_hdl, (reg))
#define RTC_WR(sc, reg, val)                                            \
        bus_space_write_4(sc->sc_iot, sc->sc_hdl, (reg), (val))

#define RTC_SOFT_RST_LOOP 455 /* At least 1 us ... */

static int
rtc_match(device_t parent, cfdata_t match, void *aux)
{
	struct apb_attach_args *aa = aux;

	if ((aa->aa_addr == HW_RTC_BASE) &&
	    (aa->aa_size == HW_RTC_BASE_SIZE))
		return 1;

	return 0;
}

static void
rtc_attach(device_t parent, device_t self, void *aux)
{
	struct rtc_softc *sc = device_private(self);
	struct apb_attach_args *aa = aux;
	static int rtc_attached = 0;

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_iot;
	
	if (rtc_attached) {
		aprint_error_dev(sc->sc_dev, "rtc is already attached\n");
		return;
	}

	if (bus_space_map(sc->sc_iot, aa->aa_addr, aa->aa_size, 0,
	    &sc->sc_hdl))
	{
		aprint_error_dev(sc->sc_dev, "Unable to map bus space\n");
		return;
	}


	rtc_init(sc);
	rtc_reset(sc);

	aprint_normal("\n");

	rtc_attached = 1;

	return;
}

static int
rtc_activate(device_t self, enum devact act)
{

	return EOPNOTSUPP;
}

static void    
rtc_init(struct rtc_softc *sc)
{
	_sc = sc;
	return;
}

/*
 * Remove pulldown resistors on the headphone outputs.
 */
void
rtc_release_gnd(int val)
{
	struct rtc_softc *sc = _sc;

        if (sc == NULL) {
                aprint_error("rtc is not initalized");
                return;
        }
	if(val)
		RTC_WR(sc, HW_RTC_PERSISTENT0_SET, (1<<19));
	else
		RTC_WR(sc, HW_RTC_PERSISTENT0_CLR, (1<<19));

	return;
}

/*
 * Reset the RTC block.
 *
 * Inspired by i.MX23 RM "39.3.10 Correct Way to Soft Reset a Block"
 */
static void
rtc_reset(struct rtc_softc *sc)
{
        unsigned int loop;

        /* Prepare for soft-reset by making sure that SFTRST is not currently
        * asserted. Also clear CLKGATE so we can wait for its assertion below.
        */
        RTC_WR(sc, HW_RTC_CTRL_CLR, HW_RTC_CTRL_SFTRST);

        /* Wait at least a microsecond for SFTRST to deassert. */
        loop = 0;
        while ((RTC_RD(sc, HW_RTC_CTRL) & HW_RTC_CTRL_SFTRST) ||
            (loop < RTC_SOFT_RST_LOOP))
                loop++;

        /* Clear CLKGATE so we can wait for its assertion below. */
        RTC_WR(sc, HW_RTC_CTRL_CLR, HW_RTC_CTRL_CLKGATE);

        /* Soft-reset the block. */
        RTC_WR(sc, HW_RTC_CTRL_SET, HW_RTC_CTRL_SFTRST);

        /* Wait until clock is in the gated state. */
        while (!(RTC_RD(sc, HW_RTC_CTRL) & HW_RTC_CTRL_CLKGATE));

        /* Bring block out of reset. */
        RTC_WR(sc, HW_RTC_CTRL_CLR, HW_RTC_CTRL_SFTRST);

        loop = 0;
        while ((RTC_RD(sc, HW_RTC_CTRL) & HW_RTC_CTRL_SFTRST) ||
            (loop < RTC_SOFT_RST_LOOP))
                loop++;

        RTC_WR(sc, HW_RTC_CTRL_CLR, HW_RTC_CTRL_CLKGATE);

        /* Wait until clock is in the NON-gated state. */
        while (RTC_RD(sc, HW_RTC_CTRL) & HW_RTC_CTRL_CLKGATE);

        return;
}
