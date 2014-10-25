/*	$NetBSD: asc.c,v 1.20 2014/10/25 10:58:12 skrll Exp $	*/

/*
 * Copyright (c) 2001 Richard Earnshaw
 * All rights reserved.
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * POSSIBILITY OF SUCH DAMAGE.
 *
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
 * Copyright (c) 1996 Mark Brinicombe
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
 *	from:ahsc.c
 */

/*
 * Driver for the Acorn SCSI card using the SBIC (WD33C93A) generic driver
 *
 * Thanks to Acorn for supplying programming information on this card.
 */

/* #define ASC_DMAMAP_DEBUG */
/* #define DEBUG */

#include "opt_ddb.h"

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: asc.c,v 1.20 2014/10/25 10:58:12 skrll Exp $");

#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/bus.h>

#include <uvm/uvm_extern.h>

#include <machine/intr.h>
#include <machine/bootconfig.h>		/* asc_poll */

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/podulebus/podules.h>
#include <dev/podulebus/powerromreg.h>

#include <acorn32/podulebus/podulebus.h>
#include <acorn32/podulebus/sbicreg.h>
#include <acorn32/podulebus/sbicvar.h>
#include <acorn32/podulebus/ascreg.h>
#include <acorn32/podulebus/ascvar.h>

void ascattach		(device_t, device_t, void *);
int  ascmatch		(device_t, cfdata_t, void *);

void asc_enintr		(struct sbic_softc *);

int  asc_dmaok		(void *, bus_dma_tag_t, struct sbic_acb *);
int  asc_dmasetup	(void *, bus_dma_tag_t, struct sbic_acb *, int);
int  asc_dmanext	(void *, bus_dma_tag_t, struct sbic_acb *, int);
void asc_dmastop	(void *, bus_dma_tag_t, struct sbic_acb *);
void asc_dmafinish	(void *, bus_dma_tag_t, struct sbic_acb *);

int  asc_intr		(void *);
void asc_minphys	(struct buf *);

void asc_dump		(void);

#ifdef DEBUG
int	asc_dmadebug = 0;
#endif

CFATTACH_DECL_NEW(asc, sizeof(struct asc_softc),
    ascmatch, ascattach, NULL, NULL);

extern struct cfdriver asc_cd;

u_long scsi_nosync;
int shift_nosync;

#if ASC_POLL > 0
int asc_poll = 0;

#endif

int
ascmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct podule_attach_args *pa = aux;

	/* Look for the card */

	/* Standard ROM, skipping the MCS card that used the same ID. */
	if (pa->pa_product == PODULE_ACORN_SCSI &&
	    strncmp(pa->pa_podule->description, "MCS", 3) != 0)
		return 1;

	/* PowerROM */
        if (pa->pa_product == PODULE_ALSYSTEMS_SCSI &&
            podulebus_initloader(pa) == 0 &&
            podloader_callloader(pa, 0, 0) == PRID_ACORN_SCSI1)
                return 1;

	return 0;
}

void
ascattach(device_t parent, device_t self, void *aux)
{
	/* volatile struct sdmac *rp;*/
	struct asc_softc *sc;
	struct sbic_softc *sbic;
	struct podule_attach_args *pa;

	sc = device_private(self);
	pa = aux;

	if (pa->pa_podule_number == -1)
		panic("Podule has disappeared !");

	sc->sc_podule_number = pa->pa_podule_number;
	sc->sc_podule = pa->pa_podule;
	podules[sc->sc_podule_number].attached = 1;

	sbic = &sc->sc_softc;

	sbic->sc_dev = self;
	sbic->sc_enintr = asc_enintr;
	sbic->sc_dmaok = asc_dmaok;
	sbic->sc_dmasetup = asc_dmasetup;
	sbic->sc_dmanext = asc_dmanext;
	sbic->sc_dmastop = asc_dmastop;
	sbic->sc_dmafinish = asc_dmafinish;

	/* Map sbic */
	sbic->sc_sbicp.sc_sbiciot = pa->pa_iot;
	if (bus_space_map (sbic->sc_sbicp.sc_sbiciot,
	    sc->sc_podule->mod_base + ASC_SBIC, ASC_SBIC_SPACE, 0,
	    &sbic->sc_sbicp.sc_sbicioh))
		panic("%s: Cannot map SBIC", device_xname(self));

	sbic->sc_clkfreq = sbic_clock_override ? sbic_clock_override : 143;

	sbic->sc_adapter.adapt_dev = self;
	sbic->sc_adapter.adapt_nchannels = 1;
	sbic->sc_adapter.adapt_openings = 7; 
	sbic->sc_adapter.adapt_max_periph = 1;
	sbic->sc_adapter.adapt_ioctl = NULL; 
	sbic->sc_adapter.adapt_minphys = asc_minphys;
	sbic->sc_adapter.adapt_request = sbic_scsi_request;

	sbic->sc_channel.chan_adapter = &sbic->sc_adapter;
	sbic->sc_channel.chan_bustype = &scsi_bustype;
	sbic->sc_channel.chan_channel = 0;
	sbic->sc_channel.chan_ntargets = 8;
	sbic->sc_channel.chan_nluns = 8;
	sbic->sc_channel.chan_id = 7;

	/* Provide an override for the host id */
	(void)get_bootconf_option(boot_args, "asc.hostid",
	    BOOTOPT_TYPE_INT, &sbic->sc_channel.chan_id);

	printf(": hostid=%d", sbic->sc_channel.chan_id);

#if ASC_POLL > 0
        if (boot_args)
		get_bootconf_option(boot_args, "ascpoll", BOOTOPT_TYPE_BOOLEAN,
		    &asc_poll);

	if (asc_poll) {
		sbic->sc_adapter.adapt_flags |= SCSIPI_ADAPT_POLL_ONLY;
		printf(" polling");
	}
#endif
	printf("\n");

	sc->sc_pagereg = sc->sc_podule->fast_base + ASC_PAGEREG;
        sc->sc_intstat = sc->sc_podule->fast_base + ASC_INTSTATUS;

	/* Reset the card */

	WriteByte(sc->sc_pagereg, 0x80);
	DELAY(500000);
	WriteByte(sc->sc_pagereg, 0x00);
	DELAY(250000);

	sbicinit(sbic);

	/* If we are polling only, we don't need a interrupt handler.  */

#ifdef ASC_POLL
	if (!asc_poll)
#endif
	{
		evcnt_attach_dynamic(&sc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
		    device_xname(self), "intr");
		sc->sc_ih = podulebus_irq_establish(pa->pa_ih, IPL_BIO,
		    asc_intr, sc, &sc->sc_intrcnt);
		if (sc->sc_ih == NULL)
			panic("%s: Cannot claim podule IRQ", device_xname(self));
	}

	/*
	 * attach all scsi units on us
	 */
	config_found(self, &sbic->sc_channel, scsiprint);
}


void
asc_enintr(struct sbic_softc *sbicsc)
{
	struct asc_softc *sc = (struct asc_softc *)sbicsc;

	sbicsc->sc_flags |= SBICF_INTR;
	WriteByte(sc->sc_pagereg, 0x40);
}

int
asc_dmaok (void *dma_h, bus_dma_tag_t dma_t, struct sbic_acb *acb)
{
	return 0;
}

int
asc_dmasetup (void *dma_h, bus_dma_tag_t dma_t, struct sbic_acb *acb, int dir)
{
	printf("asc_dmasetup()");
#ifdef DDB
	Debugger();
#else
	panic("Hit a brick wall");
#endif
	return 0;
}

int
asc_dmanext (void *dma_h, bus_dma_tag_t dma_t, struct sbic_acb *acb, int dir)
{
	printf("asc_dmanext()");
#ifdef DDB
	Debugger();
#else
	panic("Hit a brick wall");
#endif
	return 0;
}

void
asc_dmastop (void *dma_h, bus_dma_tag_t dma_t, struct sbic_acb *acb)
{
	printf("asc_dmastop\n");
}

void
asc_dmafinish (void *dma_h, bus_dma_tag_t dma_t, struct sbic_acb *acb)
{
	printf("asc_dmafinish\n");
}

void
asc_dump(void)
{
	int i;
	struct asc_softc *sc;

	for (i = 0; i < asc_cd.cd_ndevs; ++i) {
		sc = device_lookup_private(&asc_cd, i);
		if (sc != NULL)
			sbic_dump(&sc->sc_softc);
	}
}

int
asc_intr(void *arg)
{
	struct asc_softc *sc = arg;
	int intr;
	
	/* printf("ascintr:");*/
       	intr = ReadByte(sc->sc_intstat);
	/* printf("%02x\n", intr);*/

	if (intr & IS_SBIC_IRQ)
		sbicintr((struct sbic_softc *)sc);

	return 0;	/* Pass interrupt on down the chain */
}

/*
 * limit the transfer as required.
 */
void
asc_minphys(struct buf *bp)
{
#if 0
	/*
	 * We must limit the DMA xfer size
	 */
	if (bp->b_bcount > MAX_DMA_LEN) {
		printf("asc: Reducing DMA length\n");
		bp->b_bcount = MAX_DMA_LEN;
	}
#endif
	minphys(bp);
}
