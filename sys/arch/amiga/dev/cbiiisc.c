/*	$NetBSD: cbiiisc.c,v 1.4 1999/06/06 19:58:31 is Exp $	*/

/*
 * Copyright (c) 1994,1998 Michael L. Hitch
 * Copyright (c) 1982, 1990 The Regents of the University of California.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)dma.c
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <amiga/amiga/custom.h>
#include <amiga/amiga/cc.h>
#include <amiga/amiga/device.h>
#include <amiga/amiga/isr.h>

#define ARCH_720			/* This is for a 53c770 */

#include <amiga/dev/siopreg.h>
#include <amiga/dev/siopvar.h>
#include <amiga/dev/zbusvar.h>

void cbiiiscattach __P((struct device *, struct device *, void *));
int  cbiiiscmatch __P((struct device *, struct cfdata *, void *));
int  cbiiisc_dmaintr __P((void *));
#ifdef DEBUG
void cbiiisc_dump __P((void));
#endif

#if 0
struct scsipi_adapter cbiiisc_scsiswitch = {
	siopng_scsicmd,
	siopng_minphys,
	NULL,			/* scsipi_ioctl */
};
#endif

struct scsipi_device cbiiisc_scsidev = {
	NULL,		/* use default error handler */
	NULL,		/* do not have a start functio */
	NULL,		/* have no async handler */
	NULL,		/* Use default done routine */
};


#ifdef DEBUG
#endif

struct cfattach cbiiisc_ca = {
	sizeof(struct siop_softc), cbiiiscmatch, cbiiiscattach
};

/*
 * if we are a CyberStorm MK III SCSI
 */
int
cbiiiscmatch(pdp, cfp, auxp)
	struct device *pdp;
	struct cfdata *cfp;

	void *auxp;
{
	struct zbus_args *zap;

	zap = auxp;
	if (zap->manid == 8512 && zap->prodid == 100)
		return(1);
	return(0);
}

void
cbiiiscattach(pdp, dp, auxp)
	struct device *pdp, *dp;
	void *auxp;
{
	struct siop_softc *sc;
	struct zbus_args *zap;
	siop_regmap_p rp;

	printf("\n");

	zap = auxp;

	sc = (struct siop_softc *)dp;
	sc->sc_siopp = rp = ztwomap(0xf40000);
	/* siopng_dump_registers(sc); */

	/*
	 * CTEST7 = 00
	 */
	sc->sc_clock_freq = 50;		/* Clock = 50 Mhz >> */
	sc->sc_ctest7 = 0x00;
	sc->sc_dcntl = 0x20;		/* XXX ?? */

	alloc_sicallback();

	sc->sc_adapter.scsipi_cmd = siopng_scsicmd;
	sc->sc_adapter.scsipi_minphys = siopng_minphys;

	sc->sc_link.scsipi_scsi.channel = SCSI_CHANNEL_ONLY_ONE;
	sc->sc_link.adapter_softc = sc;
	sc->sc_link.scsipi_scsi.adapter_target = 7;
#if 0
	sc->sc_link.adapter = &cbiiisc_scsiswitch;
#endif
	sc->sc_link.adapter = &sc->sc_adapter;
	sc->sc_link.device = &cbiiisc_scsidev;
	sc->sc_link.openings = 2;
	sc->sc_link.scsipi_scsi.max_target = 15;
	sc->sc_link.scsipi_scsi.max_lun = 7;
	sc->sc_link.type = BUS_SCSI;

	siopnginitialize(sc);

	sc->sc_isr.isr_intr = cbiiisc_dmaintr;
	sc->sc_isr.isr_arg = sc;
	sc->sc_isr.isr_ipl = 2;		/* ?? */
	add_isr(&sc->sc_isr);

	/*
	 * attach all scsi units on us
	 */
	config_found(dp, &sc->sc_link, scsiprint);
}

int
cbiiisc_dmaintr(arg)
	void *arg;
{
	struct siop_softc *sc = arg;
	siop_regmap_p rp;
	int istat;

	if (sc->sc_flags & SIOP_INTSOFF)
		return (0);	/* interrupts are not active */
	rp = sc->sc_siopp;
	istat = rp->siop_istat;
	if ((istat & (SIOP_ISTAT_SIP | SIOP_ISTAT_DIP)) == 0)
		return(0);
	/*
	 * save interrupt status, DMA status, and SCSI status 0
	 * (may need to deal with stacked interrupts?)
	 */
	sc->sc_sist = rp->siop_sist;
	sc->sc_istat = istat;
	sc->sc_dstat = rp->siop_dstat;
	siopngintr(sc);
	return(1);
}

#ifdef DEBUG
void
cbiiisc_dump()
{
	extern struct cfdriver cbiiisc_cd;
	int i;

	for (i = 0; i < cbiiisc_cd.cd_ndevs; ++i)
		if (cbiiisc_cd.cd_devs[i])
			siopng_dump(cbiiisc_cd.cd_devs[i]);
}
#endif
