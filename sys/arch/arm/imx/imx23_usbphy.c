/* $Id: imx23_usbphy.c,v 1.1.10.2 2014/08/20 00:02:46 tls Exp $ */

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

#include <arm/imx/imx23_usbphyreg.h>
#include <arm/imx/imx23var.h>

typedef struct usbphy_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_hdl;
} *usbphy_softc_t;

static int	usbphy_match(device_t, cfdata_t, void *);
static void	usbphy_attach(device_t, device_t, void *);
static int	usbphy_activate(device_t, enum devact);

static void     usbphy_reset(struct usbphy_softc *);
static void     usbphy_init(struct usbphy_softc *);

CFATTACH_DECL3_NEW(usbphy,
        sizeof(struct usbphy_softc),
        usbphy_match,
        usbphy_attach,
        NULL,
        usbphy_activate,
        NULL,
        NULL,
        0
);

#define PHY_RD(sc, reg)                                                 \
        bus_space_read_4(sc->sc_iot, sc->sc_hdl, (reg))
#define PHY_WR(sc, reg, val)                                            \
        bus_space_write_4(sc->sc_iot, sc->sc_hdl, (reg), (val))

#define USBPHY_SOFT_RST_LOOP 455   /* At least 1 us ... */

static int
usbphy_match(device_t parent, cfdata_t match, void *aux)
{
	struct apb_attach_args *aa = aux;

	if ((aa->aa_addr == HW_USBPHY_BASE) && (aa->aa_size == HW_USBPHY_SIZE))
		return 1;

	return 0;
}

static void
usbphy_attach(device_t parent, device_t self, void *aux)
{
	struct usbphy_softc *sc = device_private(self);
	struct apb_attach_args *aa = aux;
	static int usbphy_attached = 0;
	uint32_t phy_version;

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_iot;
	
	if (usbphy_attached) {
		aprint_error_dev(sc->sc_dev, "already attached\n");
		return;
	}

	if (bus_space_map(sc->sc_iot, aa->aa_addr, aa->aa_size, 0,
	    &sc->sc_hdl))
	{
		aprint_error_dev(sc->sc_dev, "Unable to map bus space\n");
		return;
	}

	usbphy_reset(sc);
	usbphy_init(sc);

	phy_version = PHY_RD(sc, HW_USBPHY_VERSION);
        aprint_normal(": USB PHY v%" __PRIuBIT ".%" __PRIuBIT "\n", 
            __SHIFTOUT(phy_version, HW_USBPHY_VERSION_MAJOR),
            __SHIFTOUT(phy_version, HW_USBPHY_VERSION_MINOR));

	usbphy_attached = 1;

	return;
}

static int
usbphy_activate(device_t self, enum devact act)
{

	return EOPNOTSUPP;
}

/*
 * Reset the USB PHY.
 *
 * Inspired by i.MX23 RM "39.3.10 Correct Way to Soft Reset a Block"
 */
static void
usbphy_reset(struct usbphy_softc *sc)
{
        unsigned int loop;

        /* Prepare for soft-reset by making sure that SFTRST is not currently
         * asserted. Also clear CLKGATE so we can wait for its assertion below.
         */
        PHY_WR(sc, HW_USBPHY_CTRL_CLR, HW_USBPHY_CTRL_SFTRST);

        /* Wait at least a microsecond for SFTRST to deassert. */
        loop = 0;
        while ((PHY_RD(sc, HW_USBPHY_CTRL) & HW_USBPHY_CTRL_SFTRST) ||
            (loop < USBPHY_SOFT_RST_LOOP))
                loop++;

        /* Clear CLKGATE so we can wait for its assertion below. */
        PHY_WR(sc, HW_USBPHY_CTRL_CLR, HW_USBPHY_CTRL_CLKGATE);

        /* Soft-reset the block. */
        PHY_WR(sc, HW_USBPHY_CTRL_SET, HW_USBPHY_CTRL_SFTRST);

        /* Wait until clock is in the gated state. */
        while (!(PHY_RD(sc, HW_USBPHY_CTRL) & HW_USBPHY_CTRL_CLKGATE));

        /* Bring block out of reset. */
        PHY_WR(sc, HW_USBPHY_CTRL_CLR, HW_USBPHY_CTRL_SFTRST);

        loop = 0;
        while ((PHY_RD(sc, HW_USBPHY_CTRL) & HW_USBPHY_CTRL_SFTRST) ||
            (loop < USBPHY_SOFT_RST_LOOP))
                loop++;

        PHY_WR(sc, HW_USBPHY_CTRL_CLR, HW_USBPHY_CTRL_CLKGATE);

        /* Wait until clock is in the NON-gated state. */
        while (PHY_RD(sc, HW_USBPHY_CTRL) & HW_USBPHY_CTRL_CLKGATE);

        return;
}

/*
 * Enable USB PHY.
 */
static void    
usbphy_init(struct usbphy_softc *sc)
{
	/* Disable power down bits. */
	PHY_WR(sc, HW_USBPHY_PWD_CLR,
	    HW_USBPHY_PWD_RXPWDRX |
	    HW_USBPHY_PWD_RXPWDDIFF |
	    HW_USBPHY_PWD_RXPWD1PT1 |
	    HW_USBPHY_PWD_RXPWDENV |
	    HW_USBPHY_PWD_TXPWDV2I |
	    HW_USBPHY_PWD_TXPWDIBIAS |
	    HW_USBPHY_PWD_TXPWDFS
	);

	/* USB PLL Power on. */
	PHY_WR(sc, HW_USBPHY_IP_SET,
	    HW_USBPHY_IP_PLL_POWER);

	/* Wait PLL to lock to 480MHz. */
	delay(10);

	PHY_WR(sc, HW_USBPHY_IP_SET, HW_USBPHY_IP_PLL_LOCKED);

	/* Ungate PLL clock to USB PHY. */
	PHY_WR(sc, HW_USBPHY_IP_SET, HW_USBPHY_IP_EN_USB_CLKS);

	return;
}
