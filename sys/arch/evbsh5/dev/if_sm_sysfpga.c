/*	$NetBSD: if_sm_sysfpga.c,v 1.1 2002/10/22 15:19:08 scw Exp $	*/

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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_sm_sysfpga.c,v 1.1 2002/10/22 15:19:08 scw Exp $");

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

#include <evbsh5/dev/sysfpgavar.h>

static int	sm_sysfpga_match(struct device *, struct cfdata *, void *);
static void	sm_sysfpga_attach(struct device *, struct device *, void *);

struct sm_sysfpga_softc {
	struct	smc91cxx_softc sc_smc;		/* real "smc" softc */
	void	*sc_ih;				/* interrupt cookie */
};

CFATTACH_DECL(sm_sysfpga, sizeof(struct sm_sysfpga_softc),
    sm_sysfpga_match, sm_sysfpga_attach, NULL, NULL);
extern struct cfdriver sm_cd;

static int
sm_sysfpga_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct sysfpga_attach_args *sa = aux;
	bus_space_tag_t iot = sa->sa_bust;
	bus_space_handle_t ioh;
	u_int16_t tmp;
	int rv = 0;
	extern const char *smc91cxx_idstrs[];

	if (strcmp(sa->sa_name, sm_cd.cd_name) != 0)
		return (0);

	/* Map the device. */
	if (bus_space_map(iot, sa->sa_offset, SMC_IOSIZE, 0, &ioh))
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
sm_sysfpga_attach(struct device *parent, struct device *self, void *aux)
{
	struct sm_sysfpga_softc *isc = (struct sm_sysfpga_softc *)self;
	struct smc91cxx_softc *sc = &isc->sc_smc;
	struct sysfpga_attach_args *sa = aux;
	bus_space_tag_t iot = sa->sa_bust;
	bus_space_handle_t ioh;

	printf("\n");

	/* Map the device. */
	if (bus_space_map(iot, sa->sa_offset, SMC_IOSIZE, 0, &ioh))
		panic("sm_sysfpga_attach: can't map device registers");

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
	isc->sc_ih = sysfpga_intr_establish(SYSFPGA_IGROUP_IRL1, IPL_NET,
	    SYSFPGA_IRL1_INUM_LAN, smc91cxx_intr, sc);
	if (isc->sc_ih == NULL)
		printf("%s: couldn't establish interrupt handler\n",
		    sc->sc_dev.dv_xname);
}
