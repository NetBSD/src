/*	$NetBSD: ncrsc_pcctwo.c,v 1.11 2001/05/31 18:46:08 scw Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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
 * Front-end attachment code for the ncr53c710 SCSI controller
 * on mvme1[67]7 boards.
 *
 * At the present time, this attaches to an MD driver; that will change
 * when bus_dma* support is added to mvme68k, and a bus_dma* aware MI
 * driver for the NCR chip is available.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/autoconf.h>

#include <dev/ic/osiopreg.h>
#include <dev/ic/osiopvar.h>

#include <mvme68k/dev/pcctworeg.h>
#include <mvme68k/dev/pcctwovar.h>


int ncrsc_pcctwo_match __P((struct device *, struct cfdata *, void *));
void ncrsc_pcctwo_attach __P((struct device *, struct device *, void *));

struct ncrsc_softc {
	struct osiop_softc	sc_osiop;
	struct evcnt		sc_evcnt;
};

struct cfattach ncrsc_pcctwo_ca = {
	sizeof(struct ncrsc_softc), ncrsc_pcctwo_match, ncrsc_pcctwo_attach
};

static int ncrsc_pcctwo_intr __P((void *));

extern struct cfdriver osiop_cd;

/* ARGSUSED */
int
ncrsc_pcctwo_match(parent, cf, args)
	struct device *parent;
	struct cfdata *cf;
	void *args;
{
	struct pcctwo_attach_args *pa;

	pa = args;

	if (strcmp(pa->pa_name, osiop_cd.cd_name))
		return (0);

	pa->pa_ipl = cf->pcctwocf_ipl;

	return (1);
}

/* ARGSUSED */
void
ncrsc_pcctwo_attach(parent, self, args)
	struct device *parent;
	struct device *self;
	void *args;
{
	struct pcctwo_attach_args *pa;
	struct ncrsc_softc *sc;
	int clk, ctest7;

	pa = (struct pcctwo_attach_args *) args;
	sc = (struct ncrsc_softc *) self;

	/*
	 * On the '17x the siop's clock is the same as the cpu clock.
	 * On the other boards, the siop runs at twice the cpu clock.
	 * Also, the 17x cannot do proper bus-snooping (the 68060 is
	 * lame in this repspect) so don't enable it on that board.
	 */
	if (machineid == MVME_172 || machineid == MVME_177) {
		clk = cpuspeed;
		ctest7 = 0;
	} else {
		clk = cpuspeed * 2;
		ctest7 = OSIOP_CTEST7_SC0;
	}

	sc->sc_osiop.sc_bst = pa->pa_bust;
	sc->sc_osiop.sc_dmat = pa->pa_dmat;
	(void) bus_space_map(sc->sc_osiop.sc_bst, pa->pa_offset, OSIOP_NREGS,
	    0, &sc->sc_osiop.sc_reg);

	sc->sc_osiop.sc_clock_freq = clk;
	sc->sc_osiop.sc_ctest7 = ctest7 | OSIOP_CTEST7_TT1;
	sc->sc_osiop.sc_dcntl = OSIOP_DCNTL_EA;
	sc->sc_osiop.sc_id = 7;	/* XXX: Could use NVRAM setting */

	/* Attach main MI driver */
	osiop_attach(&sc->sc_osiop);

	/* Register the event counter */
	evcnt_attach_dynamic(&sc->sc_evcnt, EVCNT_TYPE_INTR,
	    pcctwointr_evcnt(pa->pa_ipl), "disk", sc->sc_osiop.sc_dev.dv_xname);

	/* Hook the chip's interrupt */
	pcctwointr_establish(PCCTWOV_SCSI, ncrsc_pcctwo_intr, pa->pa_ipl, sc,
	    &sc->sc_evcnt);
}

static int
ncrsc_pcctwo_intr(arg)
	void *arg;
{
	struct ncrsc_softc *sc = (struct ncrsc_softc *) arg;
	u_char istat;

	/*
	 * Catch any errors which can happen when the SIOP is
	 * local bus master...
	 */
	istat = pcc2_reg_read(sys_pcctwo, PCC2REG_SCSI_ERR_STATUS);
	if ((istat & PCCTWO_ERR_SR_MASK) != 0) {
		printf("%s: Local bus error: 0x%02x\n",
		    sc->sc_osiop.sc_dev.dv_xname, istat);
		istat |= PCCTWO_ERR_SR_SCLR;
		pcc2_reg_write(sys_pcctwo, PCC2REG_SCSI_ERR_STATUS, istat);
	}

	/* This is potentially nasty, since the IRQ is level triggered... */
	if (sc->sc_osiop.sc_flags & OSIOP_INTSOFF)
		return (0);

	istat = osiop_read_1(&sc->sc_osiop, OSIOP_ISTAT);

	if ((istat & (OSIOP_ISTAT_SIP | OSIOP_ISTAT_DIP)) == 0)
		return (0);

	/* Save interrupt details for the back-end interrupt handler */
	sc->sc_osiop.sc_sstat0 = osiop_read_1(&sc->sc_osiop, OSIOP_SSTAT0);
	sc->sc_osiop.sc_istat = istat;
	sc->sc_osiop.sc_dstat = osiop_read_1(&sc->sc_osiop, OSIOP_DSTAT);

	/* Deal with the interrupt */
	osiop_intr(&sc->sc_osiop);

	return (1);
}
