/*	$NetBSD: ncrsc_pcctwo.c,v 1.4 2000/03/18 22:33:03 scw Exp $ */

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

#include <mvme68k/dev/siopreg.h>
#include <mvme68k/dev/siopvar.h>
#include <mvme68k/dev/pcctworeg.h>
#include <mvme68k/dev/pcctwovar.h>


int ncrsc_pcctwo_match __P((struct device *, struct cfdata *, void *));
void ncrsc_pcctwo_attach __P((struct device *, struct device *, void *));

struct cfattach ncrsc_pcctwo_ca = {
	sizeof(struct siop_softc), ncrsc_pcctwo_match, ncrsc_pcctwo_attach
};

static int ncrsc_pcctwo_intr __P((void *));

static struct scsipi_device ncrsc_pcctwo_scsidev = {
	NULL,			/* use default error handler */
	NULL,			/* do not have a start functio */
	NULL,			/* have no async handler */
	NULL,			/* Use default done routine */
};

extern struct cfdriver ncrsc_cd;

/*
 * Define 'scsi_nosync = 0x00' to enable sync SCSI mode.
 * (Required by the main SIOP driver. This was also used by
 * the 147's SBIC driver, but sync scsi is so unreliable on
 * that board that I disabled it permanently. '167 sync.
 * scsi appears to work very well, on the other hand.)
 */
u_long scsi_nosync = 0;
int shift_nosync = 0;

/* ARGSUSED */
int
ncrsc_pcctwo_match(parent, cf, args)
	struct device *parent;
	struct cfdata *cf;
	void *args;
{
	struct pcctwo_attach_args *pa;

	pa = args;

	if (strcmp(pa->pa_name, ncrsc_cd.cd_name))
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
	struct siop_softc *sc;
	bus_space_handle_t bush;
	int clk, ctest7;
	int tmp;

	pa = (struct pcctwo_attach_args *) args;
	sc = (struct siop_softc *) self;

	/*
	 * On the '177 the siop's clock is the same as the cpu clock.
	 * On the other boards, the siop runs at twice the cpu clock.
	 * Also, the 177 cannot do proper bus-snooping (the 68060 is
	 * lame in this repspect) so don't enable it on that board.
	 */
	if (machineid == MVME_177) {
		clk = cpuspeed;
		ctest7 = 0;
	} else {
		clk = cpuspeed * 2;
		ctest7 = SIOP_CTEST7_SC0;
	}

	printf(": %dMHz ncr53C710 SCSI I/O Processor\n", clk);

	/* XXXSCW: This is a hack until siop is bus-spaced */
	bus_space_map(pa->pa_bust, pa->pa_offset, 0x40, 0, &bush);
	sc->sc_siopp = (siop_regmap_p) bush;

	sc->sc_clock_freq = clk;
	sc->sc_ctest7 = ctest7 | SIOP_CTEST7_TT1;
	sc->sc_dcntl = SIOP_DCNTL_EA;

	sc->sc_adapter.scsipi_cmd = siop_scsicmd;
	sc->sc_adapter.scsipi_minphys = siop_minphys;

	sc->sc_link.scsipi_scsi.channel = SCSI_CHANNEL_ONLY_ONE;
	sc->sc_link.adapter_softc = sc;
	sc->sc_link.scsipi_scsi.adapter_target = 7;	/* Could use NVRAM
							 * setting */
	sc->sc_link.adapter = &sc->sc_adapter;
	sc->sc_link.device = &ncrsc_pcctwo_scsidev;
	sc->sc_link.openings = 2;
	sc->sc_link.scsipi_scsi.max_target = 7;
	sc->sc_link.scsipi_scsi.max_lun = 7;
	sc->sc_link.type = BUS_SCSI;

	/* Chip-specific initialisation */
	siopinitialize(sc);

	/* Hook the chip's interrupt */
	pcc2_reg_write(sys_pcctwo, PCC2REG_SCSI_ICSR, 0);
	pcctwointr_establish(PCCTWOV_SCSI, ncrsc_pcctwo_intr, pa->pa_ipl, sc);
	pcc2_reg_write(sys_pcctwo, PCC2REG_SCSI_ICSR,
	    pa->pa_ipl | PCCTWO_ICR_IEN);

	/*
	 * Attach all scsi units on us, watching for boot device
	 * (see dk_establish).
	 */
	tmp = bootpart;

	if (PCCTWO_PADDR(pa->pa_offset) != bootaddr)
		bootpart = -1;	/* Invalid flag to dk_establish */

	(void) config_found(self, &sc->sc_link, scsiprint);

	bootpart = tmp;		/* Restore old value */
}

static int
ncrsc_pcctwo_intr(arg)
	void *arg;
{
	struct siop_softc *sc;
	siop_regmap_p rp;
	u_char istat;

	sc = arg;

	/*
	 * Catch any errors which can happen when the SIOP is
	 * local bus master...
	 */
	istat = pcc2_reg_read(sys_pcctwo, PCC2REG_SCSI_ERR_STATUS);
	if ((istat & PCCTWO_ERR_SR_MASK) != 0) {
		printf("%s: Local bus error: 0x%02x\n",
		    sc->sc_dev.dv_xname, istat);
		istat |= PCCTWO_ERR_SR_SCLR;
		pcc2_reg_write(sys_pcctwo, PCC2REG_SCSI_ERR_STATUS, istat);
	}
	/* This is potentially nasty, since the IRQ is level triggered... */
	if (sc->sc_flags & SIOP_INTSOFF)
		return (0);

	rp = sc->sc_siopp;
	istat = rp->siop_istat;

	if ((istat & (SIOP_ISTAT_SIP | SIOP_ISTAT_DIP)) == 0)
		return (0);

	/* Save interrupt details for the back-end interrupt handler */
	sc->sc_sstat0 = rp->siop_sstat0;
	sc->sc_istat = istat;
	sc->sc_dstat = rp->siop_dstat;

	/* Deal with the interrupt */
	siopintr(sc);

	return (1);
}
