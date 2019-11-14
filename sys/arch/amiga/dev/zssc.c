/*	$NetBSD: zssc.c,v 1.45.30.1 2019/11/14 16:04:32 martin Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: zssc.c,v 1.45.30.1 2019/11/14 16:04:32 martin Exp $");

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

void zsscattach(device_t, device_t, void *);
int  zsscmatch(device_t, cfdata_t, void *);
int  zssc_dmaintr(void *);
#ifdef DEBUG
void zssc_dump(void);
#endif

CFATTACH_DECL_NEW(zssc, sizeof(struct siop_softc),
    zsscmatch, zsscattach, NULL, NULL);

/*
 * if we are an PPI Zeus
 */
int
zsscmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct zbus_args *zap;

	zap = aux;
	if (zap->manid == 2026 && zap->prodid == 150)
		return(1);
	return(0);
}

void
zsscattach(device_t parent, device_t self, void *aux)
{
	struct siop_softc *sc = device_private(self);
	struct zbus_args *zap;
	siop_regmap_p rp;

	printf("\n");

	sc->sc_dev = self;
	zap = aux;

	sc->sc_siopp = rp = (siop_regmap_p)((char *)zap->va + 0x4000);

	/*
	 * CTEST7 = 00
	 */
	sc->sc_clock_freq = 66;		/* Clock = 66 MHz */
	sc->sc_ctest7 = 0x00;
	sc->sc_dcntl = 0x00;

	sc->sc_siop_si = softint_establish(SOFTINT_BIO,
	    (void (*)(void *))siopintr, sc);

	sc->sc_adapter.adapt_dev = self;
	sc->sc_adapter.adapt_nchannels = 1;
	sc->sc_adapter.adapt_openings = 7;
	sc->sc_adapter.adapt_max_periph = 1;
	sc->sc_adapter.adapt_ioctl = NULL;
	sc->sc_adapter.adapt_minphys = siop_minphys;
	sc->sc_adapter.adapt_request = siop_scsipi_request;

	sc->sc_channel.chan_adapter = &sc->sc_adapter;
	sc->sc_channel.chan_bustype = &scsi_bustype;
	sc->sc_channel.chan_channel = 0;
	sc->sc_channel.chan_ntargets = 8;
	sc->sc_channel.chan_nluns = 8;
	sc->sc_channel.chan_id = 7;

	siopinitialize(sc);

	sc->sc_isr.isr_intr = zssc_dmaintr;
	sc->sc_isr.isr_arg = sc;
	sc->sc_isr.isr_ipl = 6;
	add_isr(&sc->sc_isr);

	/*
	 * attach all scsi units on us
	 */
	config_found(self, &sc->sc_channel, scsiprint);
}

/*
 * Level 6 interrupt processing for the Progressive Peripherals Inc
 * Zeus SCSI.  Because the level 6 interrupt is above splbio, the
 * interrupt status is saved and a softint scheduled.  This way,
 * the actual processing of the interrupt can be deferred until 
 * splbio is unblocked.
 */

int
zssc_dmaintr(void *arg)
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
	sc->sc_sstat0 = rp->siop_sstat0;
	sc->sc_istat = istat;
	sc->sc_dstat = rp->siop_dstat;
	/*
	 * disable interrupts until the callback can process this
	 * interrupt.
	 */
	rp->siop_sien = 0;
	rp->siop_dien = 0;
	sc->sc_flags |= SIOP_INTDEFER | SIOP_INTSOFF;
	softint_schedule(sc->sc_siop_si);
	return(1);
}

#ifdef DEBUG
void
zssc_dump(void)
{
	extern struct cfdriver zssc_cd;
	struct siop_softc *sc;
	int i;

	for (i = 0; i < zssc_cd.cd_ndevs; ++i) {
		sc = device_lookup_private(&zssc_cd, i);
		if (sc != NULL)
			siop_dump(sc);
	}
}
#endif
