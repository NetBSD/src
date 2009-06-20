/*	$NetBSD: if_sm_obio.c,v 1.1.108.2 2009/06/20 07:20:01 yamt Exp $ */

/*
 * Copyright (c) 2002, 2003  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec Corporation may not be used to endorse or 
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * attach sm driver to Lubbock on-board bus
 * based on sys/dev/isa/if_sm_isa.c
 *
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
__KERNEL_RCSID(0, "$NetBSD: if_sm_obio.c,v 1.1.108.2 2009/06/20 07:20:01 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/select.h>
#include <sys/device.h>

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

#include <evbarm/lubbock/lubbock_var.h>

#include "opt_lubbock.h"	/* LUBBOCK_SMC91C96_16BIT */

int	sm_obio_match(struct device *, struct cfdata *, void *);
void	sm_obio_attach(struct device *, struct device *, void *);

struct sm_obio_softc {
	struct	smc91cxx_softc sc_smc;		/* real "smc" softc */

	/* OBIO-specific goo. */
	void	*sc_ih;				/* interrupt handler */
};

CFATTACH_DECL(sm_obio, sizeof(struct sm_obio_softc), sm_obio_match, 
    sm_obio_attach, NULL, NULL);

extern struct bus_space  smobio8_bs_tag;

#ifndef SM_OBIO_INTR_PARANOIA
# define smintr	smc91cxx_intr
#else
# define smintr smc_obio_intr

static int
smc_obio_intr(void *arg)
{
	while (smc91cxx_intr(arg))
		;
	return 1;
}
#endif /* SM_OBIO_INTR_PARANOIA */

int
sm_obio_match(device_t parent, cfdata_t match, void *aux)
{
	struct obio_attach_args *oba = aux;
	bus_space_tag_t iot = &smobio8_bs_tag;
	bus_space_handle_t ioh;
	u_int16_t tmp;
	int rv = 0;
	extern const char *smc91cxx_idstrs[];


	/* Map i/o space. */
	if (bus_space_map(iot, oba->oba_addr, SMC_IOSIZE, 0, &ioh))
		return (0);


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

	/*
	 * Assume we have an SMC91Cxx.
	 */

	rv = 1;

 out:
	bus_space_unmap(iot, ioh, SMC_IOSIZE);
	if (!rv) {
		printf("on-board SMC probe failed\n");
	}
	return (rv);
}

void
sm_obio_attach(device_t parent, device_t self, void *aux)
{
	struct sm_obio_softc *isc = (struct sm_obio_softc *)self;
	struct smc91cxx_softc *sc = &isc->sc_smc;
	struct obio_attach_args *oba = aux;
	bus_space_handle_t ioh;
#ifdef LUBBOCK_SMC91C96_16BIT
	bus_space_tag_t iot = &pxa2x0_a4x_bs_tag;
#else
	bus_space_tag_t iot = &smobio8_bs_tag;
#endif


	printf("\n");

	/* Map i/o space. */
	if (bus_space_map(iot, oba->oba_addr, SMC_IOSIZE, 0, &ioh))
		panic("sm_obio_attach: can't map i/o space");

#ifdef LUBBOCK_SMC91C96_16BIT
	/* RedBoot initializes on-board SMSC91C96 in 8-bit mode.
	   we take it back to 16-bit by clearing ECSR.IOIs8 */
	{
		int tmp;

		bus_space_write_1(&smobio8_bs_tag, ioh, BANK_SELECT_REG_W, 4);
		tmp = bus_space_read_1(&smobio8_bs_tag, ioh, ECSR_REG_B);
		bus_space_write_1(&smobio8_bs_tag, ioh, 
		    ECSR_REG_B, tmp & ~ECSR_IOIS8);
	}

	obio16_write(LUBBOCK_MISCWR, 
	    obio16_read(LUBBOCK_MISCWR) & ~MISCWR_ENETEN16 );

#else
	/* Force 8bit mode */
	obio16_write(LUBBOCK_MISCWR, 
	    obio16_read(LUBBOCK_MISCWR) | MISCWR_ENETEN16);

#endif /* LUBBOCK_SMC91C96_16BIT */

	sc->sc_bst = iot;
	sc->sc_bsh = ioh;

	/* should always be enabled */
	sc->sc_flags |= SMC_FLAGS_ENABLED;

	/* Perform generic intialization. */
	smc91cxx_attach(sc, NULL);

	/* Establish the interrupt handler. */
	isc->sc_ih = obio_intr_establish(device_private(parent),
					 oba->oba_intr, IPL_NET, smintr, sc);

	if (isc->sc_ih == NULL)
		aprint_normal_dev(self, "couldn't establish interrupt handler\n");
}
