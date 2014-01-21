/*	$NetBSD: csc.c,v 1.19 2014/01/21 19:50:40 christos Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Scott Stevens.
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
 * Cumana SCSI-2 driver uses the SFAS216 generic driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: csc.c,v 1.19 2014/01/21 19:50:40 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <machine/io.h>
#include <machine/intr.h>
#include <machine/bootconfig.h>
#include <acorn32/podulebus/podulebus.h>
#include <acorn32/podulebus/sfasreg.h>
#include <acorn32/podulebus/sfasvar.h>
#include <acorn32/podulebus/cscreg.h>
#include <acorn32/podulebus/cscvar.h>
#include <dev/podulebus/podules.h>
#include <dev/podulebus/powerromreg.h>

int  cscmatch(device_t, cfdata_t, void *);
void cscattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(csc, sizeof(struct csc_softc),
    cscmatch, cscattach, NULL, NULL);

int csc_intr(void *);
int csc_setup_dma(void *, void *, int, int);
int csc_build_dma_chain(void *, void *, void *, int);
int csc_need_bump(void *, void *, int);
void csc_led(void *, int);

void csc_set_dma_adr(struct sfas_softc *, void *);
void csc_set_dma_tc(struct sfas_softc *, unsigned int);
void csc_set_dma_mode(struct sfas_softc *, int);

/*
 * if we are a Cumana SCSI-2 card
 */
int
cscmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct podule_attach_args *pa = aux;

	/* Look for the card */
	if (pa->pa_product == PODULE_CUMANA_SCSI2)
		return 1;

	/* PowerROM */
        if (pa->pa_product == PODULE_ALSYSTEMS_SCSI &&
            podulebus_initloader(pa) == 0 &&
            podloader_callloader(pa, 0, 0) == PRID_CUMANA_SCSI2)
                return 1;

	return 0;
}

void
cscattach(device_t parent, device_t self, void *aux)
{
	struct csc_softc *sc = device_private(self);
	struct podule_attach_args  *pa;
	csc_regmap_p	   rp = &sc->sc_regmap;
	vu_char		  *fas;
	int loop;

	pa = aux;

	if (pa->pa_podule_number == -1)
		panic("Podule has disappeared !");

	sc->sc_specific.sc_podule_number = pa->pa_podule_number;
	sc->sc_specific.sc_podule = pa->pa_podule;
	sc->sc_specific.sc_iobase =
	    (vu_char *)sc->sc_specific.sc_podule->mod_base;

	rp->status0 = &sc->sc_specific.sc_iobase[CSC_STATUS0];
	rp->alatch = &sc->sc_specific.sc_iobase[CSC_ALATCH];
	rp->dack = (vu_short *)&sc->sc_specific.sc_iobase[CSC_DACK];
	fas = &sc->sc_specific.sc_iobase[CSC_FAS_OFFSET_BASE];

	rp->FAS216.sfas_tc_low	= &fas[CSC_FAS_OFFSET_TCL];
	rp->FAS216.sfas_tc_mid	= &fas[CSC_FAS_OFFSET_TCM];
	rp->FAS216.sfas_fifo	= &fas[CSC_FAS_OFFSET_FIFO];
	rp->FAS216.sfas_command	= &fas[CSC_FAS_OFFSET_COMMAND];
	rp->FAS216.sfas_dest_id	= &fas[CSC_FAS_OFFSET_DESTID];
	rp->FAS216.sfas_timeout	= &fas[CSC_FAS_OFFSET_TIMEOUT];
	rp->FAS216.sfas_syncper	= &fas[CSC_FAS_OFFSET_PERIOD];
	rp->FAS216.sfas_syncoff	= &fas[CSC_FAS_OFFSET_OFFSET];
	rp->FAS216.sfas_config1	= &fas[CSC_FAS_OFFSET_CONFIG1];
	rp->FAS216.sfas_clkconv	= &fas[CSC_FAS_OFFSET_CLKCONV];
	rp->FAS216.sfas_test	= &fas[CSC_FAS_OFFSET_TEST];
	rp->FAS216.sfas_config2	= &fas[CSC_FAS_OFFSET_CONFIG2];
	rp->FAS216.sfas_config3	= &fas[CSC_FAS_OFFSET_CONFIG3];
	rp->FAS216.sfas_tc_high	= &fas[CSC_FAS_OFFSET_TCH];
	rp->FAS216.sfas_fifo_bot = &fas[CSC_FAS_OFFSET_FIFOBOT];

	sc->sc_softc.sc_dev	= self;
	sc->sc_softc.sc_fas	= (sfas_regmap_p)rp;
	sc->sc_softc.sc_spec	= &sc->sc_specific;

	sc->sc_softc.sc_led	= csc_led;

	sc->sc_softc.sc_setup_dma	= csc_setup_dma;
	sc->sc_softc.sc_build_dma_chain = csc_build_dma_chain;
	sc->sc_softc.sc_need_bump	= csc_need_bump;

	sc->sc_softc.sc_clock_freq   = 8;   /* Cumana runs at 8MHz */
	sc->sc_softc.sc_timeout      = 250;  /* Set default timeout to 250ms */
	sc->sc_softc.sc_config_flags = SFAS_NO_DMA /*| SFAS_NF_DEBUG*/;
	sc->sc_softc.sc_host_id      = 7;    /* Should check the jumpers */

	sc->sc_softc.sc_bump_sz = PAGE_SIZE;
	sc->sc_softc.sc_bump_pa = 0x0;

	sfasinitialize((struct sfas_softc *)sc);

	sc->sc_softc.sc_adapter.adapt_dev = self;
	sc->sc_softc.sc_adapter.adapt_nchannels = 1;
	sc->sc_softc.sc_adapter.adapt_openings = 7;
	sc->sc_softc.sc_adapter.adapt_max_periph = 1;
	sc->sc_softc.sc_adapter.adapt_ioctl = NULL;
	sc->sc_softc.sc_adapter.adapt_minphys = sfas_minphys;
	sc->sc_softc.sc_adapter.adapt_request = sfas_scsi_request;

	sc->sc_softc.sc_channel.chan_adapter = &sc->sc_softc.sc_adapter;
	sc->sc_softc.sc_channel.chan_bustype = &scsi_bustype;
	sc->sc_softc.sc_channel.chan_channel = 0;
	sc->sc_softc.sc_channel.chan_ntargets = 8;
	sc->sc_softc.sc_channel.chan_nluns = 8;
	sc->sc_softc.sc_channel.chan_id = sc->sc_softc.sc_host_id;

	/* Provide an override for the host id */
	(void)get_bootconf_option(boot_args, "csc.hostid",
	    BOOTOPT_TYPE_INT, &sc->sc_softc.sc_channel.chan_id);

	printf(": host=%d", sc->sc_softc.sc_channel.chan_id);

	/* initialise the alatch */
	sc->sc_specific.sc_alatch_defs = (CSC_POLL?0:CSC_ALATCH_DEFS_INTEN);
	for (loop = 0; loop < 8; loop ++) {
	    if(loop != 3)
		*rp->alatch = (loop << 1) |
		    ((sc->sc_specific.sc_alatch_defs & (1 << loop))?1:0);
	}

#if CSC_POLL == 0
	evcnt_attach_dynamic(&sc->sc_softc.sc_intrcnt, EVCNT_TYPE_INTR, NULL,
	    device_xname(self), "intr");
	sc->sc_softc.sc_ih = podulebus_irq_establish(pa->pa_ih, IPL_BIO,
	    csc_intr, &sc->sc_softc, &sc->sc_softc.sc_intrcnt);
	if (sc->sc_softc.sc_ih == NULL)
	    panic("%s: Cannot install IRQ handler", device_xname(self));
#else
	printf(" polling");
	sc->sc_softc.sc_adapter.adapt_flags |= SCSIPI_ADAPT_POLL_ONLY;
#endif
	printf("\n");

	/* attach all scsi units on us */
	config_found(self, &sc->sc_softc.sc_channel, scsiprint);
}


int
csc_intr(void *arg)
{
	struct sfas_softc *dev = arg;
	csc_regmap_p	      rp;
	int		      quickints;

	rp = (csc_regmap_p)dev->sc_fas;
	
	if (*rp->FAS216.sfas_status & SFAS_STAT_INTERRUPT_PENDING) {
		quickints = 16;
		do {
			dev->sc_status = *rp->FAS216.sfas_status;
			dev->sc_interrupt = *rp->FAS216.sfas_interrupt;
	  
			if (dev->sc_interrupt & SFAS_INT_RESELECTED) {
				dev->sc_resel[0] = *rp->FAS216.sfas_fifo;
				dev->sc_resel[1] = *rp->FAS216.sfas_fifo;
			}
			sfasintr(dev);

		} while((*rp->FAS216.sfas_status & SFAS_STAT_INTERRUPT_PENDING)
			&& --quickints);
	}

	return(0);	/* Pass interrupt on down the chain */
}

/* Load transfer address into DMA register */
void
csc_set_dma_adr(struct sfas_softc *sc, void *ptr)
{
	return;
}

/* Set DMA transfer counter */
void
csc_set_dma_tc(struct sfas_softc *sc, unsigned int len)
{
	*sc->sc_fas->sfas_tc_low  = len; len >>= 8;
	*sc->sc_fas->sfas_tc_mid  = len; len >>= 8;
	*sc->sc_fas->sfas_tc_high = len;
}

/* Set DMA mode */
void
csc_set_dma_mode(struct sfas_softc *sc, int mode)
{
}

/* Initialize DMA for transfer */
int
csc_setup_dma(void *sc, void *ptr, int len, int mode)
{

	return (0);
}

/* Check if address and len is ok for DMA transfer */
int
csc_need_bump(void *sc, void *ptr, int len)
{
	int	p;

	p = (int)ptr & 0x03;

	if (p) {
		p = 4-p;
	    
		if (len < 256)
			p = len;
	}

	return(p);
}

/* Interrupt driven routines */
int
csc_build_dma_chain(void *sc, void *chain, void *p, int l)
{
	return(0);
}

/* Turn on/off led */
void
csc_led(void *v, int mode)
{
	struct sfas_softc *sc = v;

	if (mode) {
		sc->sc_led_status++;
	} else {
		if (sc->sc_led_status)
			sc->sc_led_status--;
	}
}
