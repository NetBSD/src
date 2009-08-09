/*	$NetBSD: if_sm_gxio.c,v 1.7 2009/08/09 07:10:13 kiyohara Exp $ */
/*
 * Copyright (C) 2005, 2006 WIDE Project and SOUM Corporation.
 * All rights reserved.
 *
 * Written by Takashi Kiyohara and Susumu Miki for WIDE Project and SOUM
 * Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the name of SOUM Corporation
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT and SOUM CORPORATION ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT AND SOUM CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_sm_gxio.c,v 1.7 2009/08/09 07:10:13 kiyohara Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/select.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/ic/smc91cxxreg.h>
#include <dev/ic/smc91cxxvar.h>

#include <evbarm/gumstix/gumstixvar.h>

#include "locators.h"


static int sm_gxio_match(device_t, struct cfdata *, void *);
static void sm_gxio_attach(device_t, device_t, void *);

static int ether_serial_digit = 1;

struct sm_gxio_softc {
	struct	smc91cxx_softc sc_smc;
	void	*sc_ih;	
};

CFATTACH_DECL(sm_gxio, sizeof(struct sm_gxio_softc),
    sm_gxio_match, sm_gxio_attach, NULL, NULL);


/* ARGSUSED */
static int
sm_gxio_match(device_t parent, struct cfdata *match, void *aux)
{
	struct gxio_attach_args *gxa = aux;
	bus_space_tag_t iot = gxa->gxa_iot;
	bus_space_handle_t ioh;
	uint16_t tmp;
	int rv = 0;
	extern const char *smc91cxx_idstrs[];

	/* Disallow wildcarded values. */
	if (gxa->gxa_addr == GXIOCF_ADDR_DEFAULT)
		return 0;
	if (gxa->gxa_gpirq == GXIOCF_GPIRQ_DEFAULT)
		return 0;

	if (bus_space_map(iot, gxa->gxa_addr, SMC_IOSIZE, 0, &ioh) != 0)
		return 0;

	/* Check that high byte of BANK_SELECT is what we expect. */
	tmp = bus_space_read_2(iot, ioh, BANK_SELECT_REG_W);
	if ((tmp & BSR_DETECT_MASK) != BSR_DETECT_VALUE)
		goto out;

	/*
	 * Switch to bank 0 and perform the test again.
	 * XXX INVASIVE!
	 */
	bus_space_write_1(iot, ioh, BANK_SELECT_REG_W, 0);
	tmp = bus_space_read_2(iot, ioh, BANK_SELECT_REG_W);
	if ((tmp & BSR_DETECT_MASK) != BSR_DETECT_VALUE)
		goto out;

	/*
	 * Check for a recognized chip id.
	 * XXX INVASIVE!
	 */
	bus_space_write_1(iot, ioh, BANK_SELECT_REG_W, 3);
	tmp = bus_space_read_2(iot, ioh, REVISION_REG_W);
	if (smc91cxx_idstrs[RR_ID(tmp)] == NULL)
		goto out;

	if (ether_serial_digit > 15)
		goto out;
	/*
	 * Assume we have an SMC91Cxx.
	 */

	rv = 1;

 out:
	bus_space_unmap(iot, ioh, SMC_IOSIZE);
	return rv;
}

/* ARGSUSED */
static void
sm_gxio_attach(device_t parent, device_t self, void *aux)
{
	struct sm_gxio_softc *gsc = device_private(self);
	struct smc91cxx_softc *sc = &gsc->sc_smc;
	struct gxio_attach_args *gxa = aux;
	bus_space_handle_t ioh;
	uint8_t myea[ETHER_ADDR_LEN];

	aprint_normal("\n");
	aprint_naive("\n");

	KASSERT(system_serial_high != 0 || system_serial_low != 0);

	/* Map i/o space. */
	if (bus_space_map(gxa->gxa_iot, gxa->gxa_addr, SMC_IOSIZE, 0, &ioh))
		panic("sm_gxio_attach: can't map i/o space");

	sc->sc_bst = gxa->gxa_iot;
	sc->sc_bsh = ioh;

	/* should always be enabled */
	sc->sc_flags |= SMC_FLAGS_ENABLED;

	myea[0] = ((system_serial_high >> 8) & 0xfe) | 0x02;
	myea[1] = system_serial_high;
	myea[2] = system_serial_low >> 24;
	myea[3] = system_serial_low >> 16;
	myea[4] = system_serial_low >> 8;
	myea[5] = (system_serial_low & 0xc0) |
	    (1 << 4) | ((ether_serial_digit++) & 0x0f);
	smc91cxx_attach(sc, myea);

	/* Establish the interrupt handler. */
	gsc->sc_ih = gxio_intr_establish(gxa->gxa_sc,
	    gxa->gxa_gpirq, IST_EDGE_RISING, IPL_NET, smc91cxx_intr, sc);

	if (gsc->sc_ih == NULL)
		aprint_error_dev(self,
		    "couldn't establish interrupt handler\n");
}
