/*	$NetBSD: ptsc.c,v 1.19 2014/09/13 18:08:38 matt Exp $	*/

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
 *	@(#)ptsc.c
 */

/*
 * Copyright (c) 1995 Scott Stevens
 * Copyright (c) 1995 Daniel Widenfalk
 * Copyright (c) 1994 Christian E. Hopps
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
 *	@(#)ptsc.c
 */

/*
 * Power-tec SCSI-2 driver uses SFAS216 generic driver
 *
 * Thanks to Alsystems for loaning a development card and providing
 * programming information.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ptsc.c,v 1.19 2014/09/13 18:08:38 matt Exp $");

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
#include <acorn32/podulebus/ptscreg.h>
#include <acorn32/podulebus/ptscvar.h>
#include <dev/podulebus/podules.h>
#include <dev/podulebus/powerromreg.h>

int  ptscmatch(device_t, cfdata_t, void *);
void ptscattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ptsc, sizeof(struct ptsc_softc),
    ptscmatch, ptscattach, NULL, NULL);

int ptsc_intr(void *);
int ptsc_setup_dma(void *, void *, int, int);
int ptsc_build_dma_chain(void *, void *, void *, int);
int ptsc_need_bump(void *, void *, int);
void ptsc_led(void *, int);

void ptsc_set_dma_adr(struct sfas_softc *, void *);
void ptsc_set_dma_tc(struct sfas_softc *, unsigned int);
void ptsc_set_dma_mode(struct sfas_softc *, int);

/*
 * if we are a Power-tec SCSI-2 card
 */
int
ptscmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct podule_attach_args *pa = aux;

	/* Look for the card */

	/*
	 * All Power-tec cards effectively have PowerROMS.  Note,
	 * though, that here, if we fail to initialise the loader, we
	 * assume this _is_ the right kind of card.
	 */
        if (pa->pa_product == PODULE_ALSYSTEMS_SCSI &&
            (podulebus_initloader(pa) != 0 ||
	     podloader_callloader(pa, 0, 0) == PRID_POWERTEC))
                return 1;

	return 0;
}

void
ptscattach(device_t parent, device_t self, void *aux)
{
	struct ptsc_softc *sc = device_private(self);
	struct podule_attach_args  *pa = aux;
	ptsc_regmap_p	   rp = &sc->sc_regmap;
	vu_char		  *fas;

	if (pa->pa_podule_number == -1)
		panic("Podule has disappeared !");

	sc->sc_specific.sc_podule_number = pa->pa_podule_number;
	sc->sc_specific.sc_podule = pa->pa_podule;
	sc->sc_specific.sc_iobase =
	  (vu_char *)sc->sc_specific.sc_podule->fast_base;

	rp->chipreset = &sc->sc_specific.sc_iobase[PTSC_CONTROL_CHIPRESET];
	rp->inten = &sc->sc_specific.sc_iobase[PTSC_CONTROL_INTEN];
	rp->status = &sc->sc_specific.sc_iobase[PTSC_STATUS];
	rp->term = &sc->sc_specific.sc_iobase[PTSC_CONTROL_TERM];
	rp->led = &sc->sc_specific.sc_iobase[PTSC_CONTROL_LED];
	fas = &sc->sc_specific.sc_iobase[PTSC_FASOFFSET_BASE];

	rp->FAS216.sfas_tc_low	= &fas[PTSC_FASOFFSET_TCL];
	rp->FAS216.sfas_tc_mid	= &fas[PTSC_FASOFFSET_TCM];
	rp->FAS216.sfas_fifo	= &fas[PTSC_FASOFFSET_FIFO];
	rp->FAS216.sfas_command	= &fas[PTSC_FASOFFSET_COMMAND];
	rp->FAS216.sfas_dest_id	= &fas[PTSC_FASOFFSET_DESTID];
	rp->FAS216.sfas_timeout	= &fas[PTSC_FASOFFSET_TIMEOUT];
	rp->FAS216.sfas_syncper	= &fas[PTSC_FASOFFSET_PERIOD];
	rp->FAS216.sfas_syncoff	= &fas[PTSC_FASOFFSET_OFFSET];
	rp->FAS216.sfas_config1	= &fas[PTSC_FASOFFSET_CONFIG1];
	rp->FAS216.sfas_clkconv	= &fas[PTSC_FASOFFSET_CLOCKCONV];
	rp->FAS216.sfas_test	= &fas[PTSC_FASOFFSET_TEST];
	rp->FAS216.sfas_config2	= &fas[PTSC_FASOFFSET_CONFIG2];
	rp->FAS216.sfas_config3	= &fas[PTSC_FASOFFSET_CONFIG3];
	rp->FAS216.sfas_tc_high	= &fas[PTSC_FASOFFSET_TCH];
	rp->FAS216.sfas_fifo_bot = &fas[PTSC_FASOFFSET_FIFOBOTTOM];

	sc->sc_softc.sc_dev	= self;
	sc->sc_softc.sc_fas	= (sfas_regmap_p)rp;
	sc->sc_softc.sc_spec	= &sc->sc_specific;

	sc->sc_softc.sc_led	= ptsc_led;

	sc->sc_softc.sc_setup_dma	= ptsc_setup_dma;
	sc->sc_softc.sc_build_dma_chain = ptsc_build_dma_chain;
	sc->sc_softc.sc_need_bump	= ptsc_need_bump;

	sc->sc_softc.sc_clock_freq   = 40;   /* Power-Tec runs at 8MHz */
	sc->sc_softc.sc_timeout      = 250;  /* Set default timeout to 250ms */
	sc->sc_softc.sc_config_flags = SFAS_NO_DMA /*| SFAS_NF_DEBUG*/;
	sc->sc_softc.sc_host_id      = 7;    /* Should check the jumpers */

	sc->sc_softc.sc_bump_sz = PAGE_SIZE;
	sc->sc_softc.sc_bump_pa = 0x0;

	sfasinitialize((struct sfas_softc *)sc);

	sc->sc_softc.sc_adapter.adapt_dev = sc->sc_softc.sc_dev;
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
	(void)get_bootconf_option(boot_args, "ptsc.hostid",
	    BOOTOPT_TYPE_INT, &sc->sc_softc.sc_channel.chan_id);

	printf(": host=%d", sc->sc_softc.sc_channel.chan_id);

	/* initialise the card */
/*	*rp->term = 0;*/
	*rp->inten = (PTSC_POLL?0:1);
	*rp->led = 0;

#if PTSC_POLL == 0
	evcnt_attach_dynamic(&sc->sc_softc.sc_intrcnt, EVCNT_TYPE_INTR, NULL,
	    device_xname(self), "intr");
	sc->sc_softc.sc_ih = podulebus_irq_establish(pa->pa_ih, IPL_BIO,
	    ptsc_intr, &sc->sc_softc, &sc->sc_softc.sc_intrcnt);
	if (sc->sc_softc.sc_ih == NULL)
	    panic("%s: Cannot install IRQ handler", device_xname(self));
#else
	printf(" polling");
	sc->sc_softc.sc_adapter.adapt_flags = SCSIPI_ADAPT_POLL_ONLY;
#endif
	
	printf("\n");

	/* attach all scsi units on us */
	config_found(self, &sc->sc_softc.sc_channel, scsiprint);
}


int
ptsc_intr(void *arg)
{
	struct sfas_softc *dev = arg;
	ptsc_regmap_p	      rp;
	int		      quickints;

	rp = (ptsc_regmap_p)dev->sc_fas;

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
ptsc_set_dma_adr(struct sfas_softc *sc, void *ptr)
{
#if 0
	ptsc_regmap_p	rp;
	unsigned int   *p;
	unsigned int	d;
#endif
#if 0
	printf("ptsc_set_dma_adr(sc = 0x%08x, ptr = 0x%08x)\n", (u_int)sc, (u_int)ptr);
#endif
	return;
#if 0
	rp = (ptsc_regmap_p)sc->sc_fas;

	d = (unsigned int)ptr;
	p = (unsigned int *)((d & 0xFFFFFF) + (int)rp->dmabase);

	*rp->clear=0;
	*p = d;
#endif
}

/* Set DMA transfer counter */
void
ptsc_set_dma_tc(struct sfas_softc *sc, unsigned int len)
{
	printf("ptsc_set_dma_tc(sc, len = 0x%08x)", len);

	*sc->sc_fas->sfas_tc_low  = len; len >>= 8;
	*sc->sc_fas->sfas_tc_mid  = len; len >>= 8;
	*sc->sc_fas->sfas_tc_high = len;
}

/* Set DMA mode */
void
ptsc_set_dma_mode(struct sfas_softc *sc, int mode)
{
#if 0
	struct csc_specific *spec;

	spec = sc->sc_spec;

	spec->portbits = (spec->portbits & ~FLSC_PB_DMA_BITS) | mode;
	*((flsc_regmap_p)sc->sc_fas)->hardbits = spec->portbits;
#endif
}

/* Initialize DMA for transfer */
int
ptsc_setup_dma(void *v, void *ptr, int len, int mode)
{
	return(0);

#if 0
	struct sfas_softc *sc = v;
	int	retval;

	retval = 0;

	printf("ptsc_setup_dma(sc, ptr = 0x%08x, len = 0x%08x, mode = 0x%08x)\n", (u_int)ptr, len, mode);

	switch(mode) {
	case SFAS_DMA_READ:
	case SFAS_DMA_WRITE:
		flsc_set_dma_adr(sc, ptr);
		if (mode == SFAS_DMA_READ)
		  flsc_set_dma_mode(sc,FLSC_PB_ENABLE_DMA | FLSC_PB_DMA_READ);
		else
		  flsc_set_dma_mode(sc,FLSC_PB_ENABLE_DMA | FLSC_PB_DMA_WRITE);

		flsc_set_dma_tc(sc, len);
		break;

	case SFAS_DMA_CLEAR:
	default:
		flsc_set_dma_mode(sc, FLSC_PB_DISABLE_DMA);
		flsc_set_dma_adr(sc, 0);

		retval = (*sc->sc_fas->sfas_tc_high << 16) |
			 (*sc->sc_fas->sfas_tc_mid  <<  8) |
			  *sc->sc_fas->sfas_tc_low;

		flsc_set_dma_tc(sc, 0);
		break;
	}

	return(retval);
#endif
}

/* Check if address and len is ok for DMA transfer */
int
ptsc_need_bump(void *v, void *ptr, int len)
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
ptsc_build_dma_chain(void *v1, void *v2, void *p, int l)
{
#if 0
	vaddr_t  pa, lastpa;
	char	    *ptr;
	int	     len, prelen, postlen, max_t, n;
#endif
#if 0
	printf("ptsc_build_dma_chain()\n");
#endif
	return(0);

#if 0
	if (l == 0)
		return(0);

#define set_link(n, p, l, f)\
do { chain[n].ptr = (p); chain[n].len = (l); chain[n++].flg = (f); } while(0)

	n = 0;

	if (l < 512)
		set_link(n, (vaddr_t)p, l, SFAS_CHAIN_BUMP);
	else if (p >= (void *)0xFF000000) {
		while(l != 0) {
			len = ((l > sc->sc_bump_sz) ? sc->sc_bump_sz : l);
	  
			set_link(n, (vaddr_t)p, len, SFAS_CHAIN_BUMP);
	  
			p += len;
			l -= len;
		}
	} else {
		ptr = p;
		len = l;

		pa = kvtop(ptr);
		prelen = ((int)ptr & 0x03);

		if (prelen) {
			prelen = 4-prelen;
			set_link(n, (vaddr_t)ptr, prelen, SFAS_CHAIN_BUMP);
			ptr += prelen;
			len -= prelen;
		}

		lastpa = 0;
		while(len > 3) {
			pa = kvtop(ptr);
			max_t = PAGE_SIZE - (pa & PGOFSET);
			if (max_t > len)
			  max_t = len;

			max_t &= ~3;

			if (lastpa == pa)
				sc->sc_chain[n-1].len += max_t;
			else
				set_link(n, pa, max_t, SFAS_CHAIN_DMA);
	  
			lastpa = pa+max_t;
	  
			ptr += max_t;
			len -= max_t;
		}
      
		if (len)
			set_link(n, (vaddr_t)ptr, len, SFAS_CHAIN_BUMP);
	}

	return(n);
#endif
}

/* Turn on/off led */
void
ptsc_led(void *v, int mode)
{
	struct sfas_softc	*sc = v;
	ptsc_regmap_p		rp;

	rp = (ptsc_regmap_p)sc->sc_fas;

	if (mode) {
		sc->sc_led_status++;
	} else {
		if (sc->sc_led_status)
			sc->sc_led_status--;
	}
	*rp->led = (sc->sc_led_status?1:0);
}
