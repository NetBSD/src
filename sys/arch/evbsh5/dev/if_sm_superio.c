/*	$NetBSD: if_sm_superio.c,v 1.3 2002/09/04 14:55:42 scw Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Based almost entirely on dev/isa/if_sm_isa.c
 *
 * There are just enough differences to make this copy worthwhile.
 * Note that this call isa_intr_establish() because most of the Super IO
 * device is abstracted through an isa bus. The sm(4) device happens
 * to use one of the Super IO device's interrupt pins for itself...
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_sm_superio.c,v 1.3 2002/09/04 14:55:42 scw Exp $");

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

#include <dev/isa/isavar.h>

#include <evbsh5/dev/superiovar.h>

static int	sm_superio_match(struct device *, struct cfdata *, void *);
static void	sm_superio_attach(struct device *, struct device *, void *);

struct sm_superio_softc {
	struct	smc91cxx_softc sc_smc;		/* real "smc" softc */
	void	*sc_ih;				/* interrupt cookie */
};

struct cfattach sm_superio_ca = {
	sizeof(struct sm_superio_softc), sm_superio_match, sm_superio_attach
};
extern struct cfdriver sm_cd;

static int
sm_superio_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct superio_attach_args *saa = aux;
	bus_space_tag_t iot = saa->saa_bust;
	bus_space_handle_t ioh;
	u_int16_t tmp;
	int rv = 0;
	extern const char *smc91cxx_idstrs[];

	if (strcmp(saa->saa_name, sm_cd.cd_name) != 0)
		return (0);

	/* Disallow wildcarded values. */
	if (cf->cf_loc[SUPERIOCF_OFFSET] == SUPERIOCF_OFFSET_DEFAULT)
		return (0);
	if (cf->cf_loc[SUPERIOCF_IRQ] == SUPERIOCF_IRQ_DEFAULT)
		return (0);

	saa->saa_offset += cf->cf_loc[SUPERIOCF_OFFSET];
	saa->saa_irq = cf->cf_loc[SUPERIOCF_IRQ];

	/* Map the device. */
	if (bus_space_map(iot, saa->saa_offset, SMC_IOSIZE, 0, &ioh))
		return (0);

	/* Check that high byte of BANK_SELECT is what we expect. */
	tmp = bus_space_read_2(iot, ioh, BANK_SELECT_REG_W);
	if ((tmp & BSR_DETECT_MASK) != BSR_DETECT_VALUE)
		goto out;

	/*
	 * Switch to bank 0 and perform the test again.
	 * XXX INVASIVE!
	 */
	bus_space_write_2(iot, ioh, BANK_SELECT_REG_W, 0);
	tmp = bus_space_read_2(iot, ioh, BANK_SELECT_REG_W);
	if ((tmp & BSR_DETECT_MASK) != BSR_DETECT_VALUE)
		goto out;

	/*
	 * Check for a recognized chip id.
	 * XXX INVASIVE!
	 */
	bus_space_write_2(iot, ioh, BANK_SELECT_REG_W, 3);
	tmp = bus_space_read_2(iot, ioh, REVISION_REG_W);
	if (smc91cxx_idstrs[RR_ID(tmp)] == NULL)
		goto out;

	rv = 1;

 out:
	bus_space_unmap(iot, ioh, SMC_IOSIZE);
	return (rv);
}

static void
sm_superio_attach(struct device *parent, struct device *self, void *aux)
{
	struct sm_superio_softc *isc = (struct sm_superio_softc *)self;
	struct smc91cxx_softc *sc = &isc->sc_smc;
	struct superio_attach_args *saa = aux;
	bus_space_tag_t iot = saa->saa_bust;
	bus_space_handle_t ioh;

	printf("\n");

	/* Map the device. */
	if (bus_space_map(iot, saa->saa_offset, SMC_IOSIZE, 0, &ioh))
		panic("sm_superio_attach: can't map device registers");

	sc->sc_bst = iot;
	sc->sc_bsh = ioh;

	/* should always be enabled */
	sc->sc_flags |= SMC_FLAGS_ENABLED;

	/* read accesses are always 32-bits wide on Cayman ... */
	sc->sc_flags |= SMC_FLAGS_32BIT_READ;

	/* The PHY does not need to be isolated */
	sc->sc_mii.mii_flags |= MIIF_NOISOLATE;

	/* Perform generic intialization. */
	smc91cxx_attach(sc, NULL);

	/* Establish the interrupt handler. */
	isc->sc_ih = isa_intr_establish(NULL, saa->saa_irq,
	    IST_EDGE, IPL_NET, smc91cxx_intr, sc);
	if (isc->sc_ih == NULL)
		printf("%s: couldn't establish interrupt handler\n",
		    sc->sc_dev.dv_xname);
}
