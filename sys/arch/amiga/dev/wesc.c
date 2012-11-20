/*	$NetBSD: wesc.c,v 1.39.18.1 2012/11/20 03:00:59 tls Exp $ */

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
 * Copyright (c) 1994 Michael L. Hitch
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
__KERNEL_RCSID(0, "$NetBSD: wesc.c,v 1.39.18.1 2012/11/20 03:00:59 tls Exp $");

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
#include <amiga/dev/zbusvar.h>

void wescattach(device_t, device_t, void *);
int wescmatch(device_t, cfdata_t, void *);
int wesc_dmaintr(void *);
#ifdef DEBUG
void wesc_dump(void);
#endif


#ifdef DEBUG
#endif

CFATTACH_DECL_NEW(wesc, sizeof(struct siop_softc),
    wescmatch, wescattach, NULL, NULL);

/*
 * if we are an MacroSystemsUS Warp Engine
 */
int
wescmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct zbus_args *zap;

	zap = aux;
	if (zap->manid == 2203 && zap->prodid == 19)
		return(1);
	return(0);
}

void
wescattach(device_t parent, device_t self, void *aux)
{
	struct siop_softc *sc = device_private(self);
	struct zbus_args *zap;
	siop_regmap_p rp;
	struct scsipi_adapter *adapt = &sc->sc_adapter;
	struct scsipi_channel *chan = &sc->sc_channel;

	sc->sc_dev = self;

	printf("\n");

	zap = aux;

	sc->sc_siopp = rp = (siop_regmap_p)((char *)zap->va + 0x40000);

	/*
	 * CTEST7 = SC0, TT1
	 */
	sc->sc_clock_freq = 50;		/* Clock = 50 MHz */
	sc->sc_ctest7 = SIOP_CTEST7_SC0 | SIOP_CTEST7_TT1;
	sc->sc_dcntl = 0x00;

	/*
	 * Fill in the scsipi_adapter.
	 */
	memset(adapt, 0, sizeof(*adapt));
	adapt->adapt_dev = self;
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

	sc->sc_isr.isr_intr = wesc_dmaintr;
	sc->sc_isr.isr_arg = sc;
	sc->sc_isr.isr_ipl = 2;
	add_isr (&sc->sc_isr);

	/*
	 * attach all scsi units on us
	 */
	config_found(self, chan, scsiprint);
}

int
wesc_dmaintr(void *arg)
{
	struct siop_softc *sc = arg;
	siop_regmap_p rp;
	u_char istat;

	if (sc->sc_flags & SIOP_INTSOFF)
		return (0);	/* interrupts are not active */
	rp = sc->sc_siopp;
	istat = rp->siop_istat;
	if ((istat & (SIOP_ISTAT_SIP | SIOP_ISTAT_DIP)) == 0)
		return (0);
	/*
	 * save interrupt status, DMA status, and SCSI status 0
	 * (may need to deal with stacked interrupts?)
	 */
	sc->sc_sstat0 = rp->siop_sstat0;
	sc->sc_istat = istat;
	sc->sc_dstat = rp->siop_dstat;
	siopintr(sc);
	return(1);
}

#ifdef DEBUG
void
wesc_dump(void)
{
	extern struct cfdriver wesc_cd;
	struct siop_softc *sc;
	int i;

	for (i = 0; i < wesc_cd.cd_ndevs; ++i) {
		sc = device_lookup_private(&wesc_cd, i);
		if (sc != NULL)
			siop_dump(sc);
	}
}
#endif
