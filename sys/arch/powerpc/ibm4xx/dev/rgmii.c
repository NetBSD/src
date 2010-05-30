/* $NetBSD: rgmii.c,v 1.1.4.2 2010/05/30 05:17:02 rmind Exp $ */
/*
 * Copyright (c) 2010 KIYOHARA Takashi
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rgmii.c,v 1.1.4.2 2010/05/30 05:17:02 rmind Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_media.h>

#include <powerpc/ibm4xx/dev/rgmiireg.h>
#include <powerpc/ibm4xx/dev/rmiivar.h>
#include <powerpc/ibm4xx/dev/opbvar.h>


static void rgmii_enable(device_t, int);
static void rgmii_disable(device_t, int);
static void rgmii_speed(device_t, int, int);


void
rgmii_attach(device_t self, int instance,
	     void (**enable)(device_t, int),
	     void (**disable)(device_t, int),
	     void (**speed)(device_t, int, int))
{
	struct opb_softc *sc = device_private(self);
	uint32_t ssr;

	instance %= 2;

	rgmii_disable(self, instance);
	ssr = bus_space_read_4(sc->sc_iot, sc->sc_rgmiih, RGMII0_SSR);
	ssr &= ~SSR_SP(instance, SSR_SP_MASK);
	bus_space_write_4(sc->sc_iot, sc->sc_rgmiih, RGMII0_SSR, ssr);

	*enable = rgmii_enable;
	*disable = rgmii_disable;
	*speed = rgmii_speed;
}

static void
rgmii_enable(device_t self, int instance)
{
	struct opb_softc *sc = device_private(self);
	uint32_t fer;

	instance %= 2;

	fer = bus_space_read_4(sc->sc_iot, sc->sc_rgmiih, RGMII0_FER);
	fer &= ~FER_MDIOEN_MASK;
	fer |= FER_MDIOEN(instance);
	bus_space_write_4(sc->sc_iot, sc->sc_rgmiih, RGMII0_FER, fer);
}

static void
rgmii_disable(device_t self, int instance)
{
	struct opb_softc *sc = device_private(self);
	uint32_t fer;

	instance %= 2;

	fer = bus_space_read_4(sc->sc_iot, sc->sc_rgmiih, RGMII0_FER);
	fer &= ~FER_MDIOEN_MASK;
	bus_space_write_4(sc->sc_iot, sc->sc_rgmiih, RGMII0_FER, fer);
}

static void
rgmii_speed(device_t self, int instance, int speed)
{
	struct opb_softc *sc = device_private(self);
	uint32_t ssr;

	instance %= 2;

	ssr = bus_space_read_4(sc->sc_iot, sc->sc_rgmiih, RGMII0_SSR);
	ssr &= ~SSR_SP(instance, SSR_SP_MASK);
	switch (speed) {
	case IFM_1000_T:
		ssr |= SSR_SP(instance, SSR_SP_1000MBPS);
		break;
	case IFM_100_TX:
		ssr |= SSR_SP(instance, SSR_SP_100MBPS);
		break;
	case IFM_10_T:
		ssr |= SSR_SP(instance, SSR_SP_10MBPS);
		break;
	}
	bus_space_write_4(sc->sc_iot, sc->sc_rgmiih, RGMII0_SSR, ssr);
}
