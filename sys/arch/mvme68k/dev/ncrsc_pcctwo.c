/*	$NetBSD: ncrsc_pcctwo.c,v 1.1 1999/02/20 00:11:59 scw Exp $ */

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

#include <machine/autoconf.h>

#include <mvme68k/dev/siopreg.h>
#include <mvme68k/dev/siopvar.h>
#include <mvme68k/dev/pccvar.h>
#include <mvme68k/dev/pcctworeg.h>


int	ncrsc_pcctwo_match  __P((struct device *, struct cfdata *, void *));
void	ncrsc_pcctwo_attach __P((struct device *, struct device *, void *));

struct cfattach ncrsc_pcctwo_ca = {
	sizeof(struct siop_softc), ncrsc_pcctwo_match, ncrsc_pcctwo_attach
};

static int ncrsc_pcctwo_intr __P((void *));

static struct scsipi_device ncrsc_pcctwo_scsidev = {
	NULL,		/* use default error handler */
	NULL,		/* do not have a start functio */
	NULL,		/* have no async handler */
	NULL,		/* Use default done routine */
};

extern struct cfdriver ncrsc_cd;


/*
 * if we are an MacroSystemsUS Warp Engine
 */
int
ncrsc_pcctwo_match(parent, cf, args)
	struct device *parent;
	struct cfdata *cf;
	void *args;
{
	struct pcc_attach_args *pa = args;

	if ( strcmp(pa->pa_name, ncrsc_cd.cd_name) )
		return 0;

	pa->pa_ipl = cf->pcccf_ipl;

	return 1;
}

void
ncrsc_pcctwo_attach(parent, self, args)
	struct device *parent;
	struct device *self;
	void *args;
{
	struct siop_softc *sc = (struct siop_softc *)self;
	struct pcc_attach_args *pa = args;
	int clk;
	int tmp;

	/*
	 * On the '177 the siop's clock is the same as the cpu clock.
	 * On the other boards, the siop runs at twice the cpu clock.
	 */
	clk = cpuspeed * ((machineid == MVME_177) ? 1 : 2);
	printf(": %dMHz ncr53C710 SCSI I/O Processor\n", clk);

	/*
	 * CTEST7 = SC0, TT1
	 */
	sc->sc_ctest7 = SIOP_CTEST7_SC0 | SIOP_CTEST7_TT1;
	sc->sc_dcntl = SIOP_DCNTL_EA;
	sc->sc_siopp = (siop_regmap_p) PCCTWO_VADDR(pa->pa_offset);
	sc->sc_clock_freq = clk;

	sc->sc_adapter.scsipi_cmd = siop_scsicmd;
	sc->sc_adapter.scsipi_minphys = siop_minphys;

	sc->sc_link.scsipi_scsi.channel = SCSI_CHANNEL_ONLY_ONE;
	sc->sc_link.adapter_softc = sc;
	sc->sc_link.scsipi_scsi.adapter_target = 7;/* Could use NVRAM setting */
	sc->sc_link.adapter = &sc->sc_adapter;
	sc->sc_link.device = &ncrsc_pcctwo_scsidev;
	sc->sc_link.openings = 2;
	sc->sc_link.scsipi_scsi.max_target = 7;
	sc->sc_link.scsipi_scsi.max_lun = 7;
	sc->sc_link.type = BUS_SCSI;

	siopinitialize(sc);

	sys_pcctwo->scsi_icr = 0;
	pcctwointr_establish(PCCTWOV_SCSI, ncrsc_pcctwo_intr, pa->pa_ipl, sc);
	sys_pcctwo->scsi_icr = pa->pa_ipl | PCCTWO_ICR_IEN;

	/*
	 * Attach all scsi units on us, watching for boot device
	 * (see dk_establish).
	 */
	tmp = bootpart;

	if (PCCTWO_PADDR(pa->pa_offset) != bootaddr) 
		bootpart = -1;		/* Invalid flag to dk_establish */

	(void)config_found(self, &sc->sc_link, scsiprint);

	bootpart = tmp;		/* Restore old value */
}

static int
ncrsc_pcctwo_intr(arg)
	void *arg;
{
	struct siop_softc *sc = arg;
	siop_regmap_p rp;
	u_char istat;

	if ( (sys_pcctwo->scsi_err_sr & PCCTWO_ERR_SR_MASK) != 0 ) {
		printf("%s: Local bus error: 0x%02x\n",
			sc->sc_dev.dv_xname, sys_pcctwo->scsi_err_sr);
		sys_pcctwo->scsi_err_sr |= PCCTWO_ERR_SR_SCLR;
	}

#if 0
	/* This is potentially nasty, since the IRQ is level triggered... */
	if ( sc->sc_flags & SIOP_INTSOFF )
		return 0;
#endif

	rp = sc->sc_siopp;
	istat = rp->siop_istat;

	if ( (istat & (SIOP_ISTAT_SIP | SIOP_ISTAT_DIP)) == 0 )
		return 0;

	/*
	 * Save interrupt details for the back-end interrupt handler
	 */
	sc->sc_sstat0 = rp->siop_sstat0;
	sc->sc_istat = istat;
	sc->sc_dstat = rp->siop_dstat;

	siopintr(sc);

	return 1;
}
