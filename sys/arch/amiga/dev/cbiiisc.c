/*	$NetBSD: cbiiisc.c,v 1.13 2004/03/28 18:59:39 mhitch Exp $ */

/*
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
 * 3. Neither the name of the University nor the names of its contributors
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

/*
 * Copyright (c) 1994,1998 Michael L. Hitch
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	@(#)dma.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cbiiisc.c,v 1.13 2004/03/28 18:59:39 mhitch Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

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

void cbiiiscattach(struct device *, struct device *, void *);
int  cbiiiscmatch(struct device *, struct cfdata *, void *);
int  cbiiisc_dmaintr(void *);
#ifdef DEBUG
void cbiiisc_dump(void);
#endif

#ifdef DEBUG
#endif

CFATTACH_DECL(cbiiisc, sizeof(struct siop_softc),
    cbiiiscmatch, cbiiiscattach, NULL, NULL);

/*
 * if we are a CyberStorm MK III SCSI
 */
int
cbiiiscmatch(struct device *pdp, struct cfdata *cfp, void *auxp)
{
	struct zbus_args *zap;

	zap = auxp;
	if (zap->manid == 8512 && zap->prodid == 100)
		return(1);
	return(0);
}

void
cbiiiscattach(struct device *pdp, struct device *dp, void *auxp)
{
	struct siop_softc *sc = (struct siop_softc *)dp;
	struct zbus_args *zap;
	siop_regmap_p rp;
        struct scsipi_adapter *adapt = &sc->sc_adapter;
        struct scsipi_channel *chan = &sc->sc_channel;

	printf("\n");

	zap = auxp;

	sc->sc_siopp = rp = ztwomap(0xf40000);
	/* siopng_dump_registers(sc); */

	/*
	 * CTEST7 = 00
	 */
	sc->sc_clock_freq = 50;		/* Clock = 50 Mhz >> */
	sc->sc_ctest7 = 0x00;
	sc->sc_dcntl = 0x20;		/* XXX ?? */

	alloc_sicallback();

        /*
         * Fill in the scsipi_adapter.
         */
        memset(adapt, 0, sizeof(*adapt));
        adapt->adapt_dev = &sc->sc_dev;
        adapt->adapt_nchannels = 1;
        adapt->adapt_openings = 7;
        adapt->adapt_max_periph = 1;
        adapt->adapt_request = siopng_scsipi_request;
        adapt->adapt_minphys = siopng_minphys;

        /*
         * Fill in the scsipi_channel.
         */
        memset(chan, 0, sizeof(*chan));
        chan->chan_adapter = adapt;
        chan->chan_bustype = &scsi_bustype;
        chan->chan_channel = 0;
        chan->chan_ntargets = 16;
        chan->chan_nluns = 8;
        chan->chan_id = 7;

	siopnginitialize(sc);

	if (sc->sc_channel.chan_ntargets < 0)
		return;

	sc->sc_isr.isr_intr = cbiiisc_dmaintr;
	sc->sc_isr.isr_arg = sc;
	sc->sc_isr.isr_ipl = 2;		/* ?? */
	add_isr(&sc->sc_isr);

	/*
	 * attach all scsi units on us
	 */
	config_found(dp, chan, scsiprint);
}

int
cbiiisc_dmaintr(void *arg)
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
cbiiisc_dump(void)
{
	extern struct cfdriver cbiiisc_cd;
	int i;

	for (i = 0; i < cbiiisc_cd.cd_ndevs; ++i)
		if (cbiiisc_cd.cd_devs[i])
			siopng_dump(cbiiisc_cd.cd_devs[i]);
}
#endif
