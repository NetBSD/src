/*	$NetBSD: drsc.c,v 1.18.8.2 2002/02/28 04:06:35 nathanw Exp $ */

/*
 * Copyright (c) 1996 Ignatios Souvatzis
 * Copyright (c) 1994 Michael L. Hitch
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drsc.c,v 1.18.8.2 2002/02/28 04:06:35 nathanw Exp $");

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
#include <amiga/dev/siopreg.h>
#include <amiga/dev/siopvar.h>
#include <amiga/amiga/drcustom.h>

#include <machine/cpu.h>	/* is_xxx(), */

void drscattach(struct device *, struct device *, void *);
int drscmatch(struct device *, struct cfdata *, void *);
int drsc_dmaintr(struct siop_softc *);
#ifdef DEBUG
void drsc_dump(void);
#endif

#ifdef DEBUG
#endif

struct cfattach drsc_ca = {
	sizeof(struct siop_softc),
	drscmatch,
	drscattach
};

static struct siop_softc *drsc_softc;

/*
 * One of us is on every DraCo motherboard,
 */
int
drscmatch(struct device *pdp, struct cfdata *cfp, void *auxp)
{
	static int drsc_matched = 0;

	/* Allow only one instance. */
	if (!is_draco() || !matchname(auxp, "drsc") || drsc_matched)
		return (0);

	drsc_matched = 1;
	return(1);
}

void
drscattach(struct device *pdp, struct device *dp, void *auxp)
{
	struct siop_softc *sc = (struct siop_softc *)dp;
	struct zbus_args *zap;
	siop_regmap_p rp;
	struct scsipi_adapter *adapt = &sc->sc_adapter;
	struct scsipi_channel *chan = &sc->sc_channel;

	printf("\n");

	zap = auxp;

	sc->sc_siopp = rp = (siop_regmap_p)(DRCCADDR+NBPG*DRSCSIPG);

	/*
	 * CTEST7 = TT1
	 */
	sc->sc_clock_freq = 50;		/* Clock = 50MHz */
	sc->sc_ctest7 = 0x02;

	alloc_sicallback();

	/*
	 * Fill in the scsipi_adapter.
	 */
	memset(adapt, 0, sizeof(*adapt));
	adapt->adapt_dev = &sc->sc_dev;
	adapt->adapt_nchannels = 1;
	adapt->adapt_openings = 7;
	adapt->adapt_max_periph = 1;
	adapt->adapt_request = siop_scsipi_request;
	adapt->adapt_minphys = siop_minphys;

	/*
	 * Fill in the scsipi_channel.
	 */
	memset(chan, 0, sizeof(*chan));
	chan->chan_adapter = adapt;
	chan->chan_bustype = &scsi_bustype;
	chan->chan_channel = 0;
	chan->chan_ntargets = 8;
	chan->chan_nluns = 8;
	chan->chan_id = 7;

	siopinitialize(sc);

#if 0
	sc->sc_isr.isr_intr = drsc_dmaintr;
	sc->sc_isr.isr_arg = sc;
	sc->sc_isr.isr_ipl = 4;
	add_isr(&sc->sc_isr);
#else
	drsc_softc = sc;
	single_inst_bclr_b(*draco_intpen, DRIRQ_SCSI);
	single_inst_bset_b(*draco_intena, DRIRQ_SCSI);
#endif
	/*
	 * attach all scsi units on us
	 */
	config_found(dp, chan, scsiprint);
}

/*
 * Level 4 interrupt processing for the MacroSystem DraCo mainboard
 * SCSI.  Because the level 4 interrupt is above splbio, the
 * interrupt status is saved and an sicallback to the level 2 interrupt
 * handler scheduled.  This way, the actual processing of the interrupt
 * can be deferred until splbio is unblocked.
 */

void
drsc_handler(void)
{
	struct siop_softc *sc = drsc_softc;

	siop_regmap_p rp;
	int istat;

	if (sc->sc_flags & SIOP_INTSOFF)
		return;		/* interrupts are not active */

	rp = sc->sc_siopp;
	istat = rp->siop_istat;

	if ((istat & (SIOP_ISTAT_SIP | SIOP_ISTAT_DIP)) == 0)
		return;

	/*
	 * save interrupt status, DMA status, and SCSI status 0
	 * (may need to deal with stacked interrupts?)
	 */
	sc->sc_sstat0 = rp->siop_sstat0;
	sc->sc_istat = istat;
	sc->sc_dstat = rp->siop_dstat;
	/*
	 * disable interrupts until the callback can process this
	 * interrupt.
	 */
#ifdef DRSC_NOCALLBACK
	(void)spl1();
	siopintr(sc);
#else
	rp->siop_sien = 0;
	rp->siop_dien = 0;
	sc->sc_flags |= SIOP_INTDEFER | SIOP_INTSOFF;
	single_inst_bclr_b(*draco_intpen, DRIRQ_SCSI);
#ifdef DEBUG
	if (*draco_intpen & DRIRQ_SCSI)
		printf("%s: intpen still 0x%x\n", sc->sc_dev.dv_xname,
		    *draco_intpen);
#endif
	add_sicallback((sifunc_t)siopintr, sc, NULL);
#endif
	return;
}

#ifdef DEBUG
void
drsc_dump(void)
{
	extern struct cfdriver drsc_cd;
	int i;

	for (i = 0; i < drsc_cd.cd_ndevs; ++i)
		if (drsc_cd.cd_devs[i])
			siop_dump(drsc_cd.cd_devs[i]);
}
#endif
