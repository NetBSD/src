/*	$NetBSD: cosc.c,v 1.16 2001/04/25 17:53:11 bouyer Exp $	*/

/*
 * Copyright (c) 1996 Mark Brinicombe
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
 *	This product includes software developed by Mark Brinicombe
 *      for the NetBSD Project.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *	from: asc.c,v 1.8 1996/06/12 20:46:58 mark Exp
 */

/*
 * Driver for the MCS Connect 32 SCSI 2 card with AM53C94 SCSI controller.
 *
 * Thanks to Mike <mcsmike@knipp.de> at MCS for loaning a card.
 * Thanks to Andreas Gandor <andi@knipp.de> for some technical information
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <machine/bootconfig.h>
#include <machine/io.h>
#include <machine/irqhandler.h>
#include <machine/katelib.h>
#include <arm32/podulebus/podulebus.h>
#include <arm32/podulebus/escreg.h>
#include <arm32/podulebus/escvar.h>
#include <arm32/podulebus/coscreg.h>
#include <arm32/podulebus/coscvar.h>
#include <dev/podulebus/podules.h>

void coscattach	__P((struct device *, struct device *, void *));
int coscmatch	__P((struct device *, struct cfdata *, void *));
void cosc_scsi_request	__P((struct scsipi_channel *,
				scsipi_adapter_req_t, void *));

struct cfattach cosc_ca = {
	sizeof(struct cosc_softc), coscmatch, coscattach
};

int cosc_intr		 __P((void *arg));
int cosc_setup_dma	 __P((struct esc_softc *sc, void *ptr, int len,
			      int mode));
int cosc_build_dma_chain __P((struct esc_softc *sc,
			      struct esc_dma_chain *chain, void *p, int l));
int cosc_need_bump	 __P((struct esc_softc *sc, void *ptr, int len));

void cosc_led		 __P((struct esc_softc *sc, int mode));

#if COSC_POLL > 0
int cosc_poll = 1;
#endif


int
coscmatch(pdp, cf, auxp)
	struct device *pdp;
	struct cfdata *cf;
	void *auxp;
{
	struct podule_attach_args *pa = (struct podule_attach_args *)auxp;

	/* Look for the card */

	if (matchpodule(pa, MANUFACTURER_MCS, PODULE_MCS_SCSI, -1) != 0)
		return(1);

	/* Old versions of the ROM on this card could have the wrong ID */

	if (matchpodule(pa, MANUFACTURER_ACORN, PODULE_ACORN_SCSI, -1) == 0)
		return(0);

	if (strncmp(pa->pa_podule->description, "MCS", 3) != 0)
		return(0);

	return(1);
}

static int dummy[6];

void
coscattach(pdp, dp, auxp)
	struct device *pdp, *dp;
	void *auxp;
{
	struct cosc_softc *sc = (struct cosc_softc *)dp;
	struct podule_attach_args *pa;
	cosc_regmap_p	   rp = &sc->sc_regmap;
	vu_char		  *esc;

	pa = (struct podule_attach_args *)auxp;

	if (pa->pa_podule_number == -1)
		panic("Podule has disappeared !");

	sc->sc_podule_number = pa->pa_podule_number;
	sc->sc_podule = pa->pa_podule;
	podules[sc->sc_podule_number].attached = 1;

	printf(":");

	if (pa->pa_podule->manufacturer == MANUFACTURER_ACORN
	    && pa->pa_podule->product == PODULE_ACORN_SCSI)
		printf(" Faulty expansion card identity\n");

	sc->sc_iobase = (vu_char *)sc->sc_podule->fast_base;

	/* Select page zero (so we can see the config info) */

	sc->sc_iobase[COSC_PAGE_REGISTER] = 0;

	rp->chipreset = (vu_char *)&dummy[0];
	rp->inten = (vu_char *)&dummy[1];
	rp->status = (vu_char *)&dummy[2];
	rp->term = &sc->sc_iobase[COSC_TERMINATION_CONTROL];
	rp->led = (vu_char *)&dummy[4];
	esc = &sc->sc_iobase[COSC_ESCOFFSET_BASE];

	rp->esc.esc_tc_low	= &esc[COSC_ESCOFFSET_TCL];
	rp->esc.esc_tc_mid	= &esc[COSC_ESCOFFSET_TCM];
	rp->esc.esc_fifo	= &esc[COSC_ESCOFFSET_FIFO];
	rp->esc.esc_command	= &esc[COSC_ESCOFFSET_COMMAND];
	rp->esc.esc_dest_id	= &esc[COSC_ESCOFFSET_DESTID];
	rp->esc.esc_timeout	= &esc[COSC_ESCOFFSET_TIMEOUT];
	rp->esc.esc_syncper	= &esc[COSC_ESCOFFSET_PERIOD];
	rp->esc.esc_syncoff	= &esc[COSC_ESCOFFSET_OFFSET];
	rp->esc.esc_config1	= &esc[COSC_ESCOFFSET_CONFIG1];
	rp->esc.esc_clkconv	= &esc[COSC_ESCOFFSET_CLOCKCONV];
	rp->esc.esc_test	= &esc[COSC_ESCOFFSET_TEST];
	rp->esc.esc_config2	= &esc[COSC_ESCOFFSET_CONFIG2];
	rp->esc.esc_config3	= &esc[COSC_ESCOFFSET_CONFIG3];
	rp->esc.esc_config4	= &esc[COSC_ESCOFFSET_CONFIG4];
	rp->esc.esc_tc_high	= &esc[COSC_ESCOFFSET_TCH];
	rp->esc.esc_fifo_bot	= &esc[COSC_ESCOFFSET_FIFOBOTTOM];

	*rp->esc.esc_command = ESC_CMD_RESET_CHIP;
	delay(1000);
	*rp->esc.esc_command = ESC_CMD_NOP;

	/* See if we recognise the controller */

	switch (*rp->esc.esc_tc_high) {
		case 0x12:
			printf(" AM53CF94");
			break;
		default:
			printf(" Unknown controller (%02x)", *rp->esc.esc_tc_high);
			break;
	}

	/* Set termination power */

	if (sc->sc_iobase[COSC_CONFIG_TERMINATION] & COSC_CONFIG_TERMINATION_ON) {
		printf(" termpwr on");
		sc->sc_iobase[COSC_TERMINATION_CONTROL] = COSC_TERMINATION_ON;
	} else {
		printf(" termpwr off");
		sc->sc_iobase[COSC_TERMINATION_CONTROL] = COSC_TERMINATION_OFF;
	}
	
	/* Don't know what this is for */

	{
		int byte;
		int loop;
		
		byte = sc->sc_iobase[COSC_REGISTER_01];
		byte = 0;
		for (loop = 0; loop < 8; ++loop) {
			if (sc->sc_iobase[COSC_REGISTER_00] & 0x01)
				byte |= (1 << loop);
		}
		printf(" byte=%02x", byte);
	}

	/*
	 * Control register 4 is an AMD special (not on FAS216)
	 *
	 * The powerdown and glitch eater facilities could be useful
	 * Use the podule configuration for this register
	 */

	sc->sc_softc.sc_config4 = sc->sc_iobase[COSC_CONFIG_CONTROL_REG4];

	sc->sc_softc.sc_esc	= (esc_regmap_p)rp;
/*	sc->sc_softc.sc_spec	= &sc->sc_specific;*/

	sc->sc_softc.sc_led	= cosc_led;
	sc->sc_softc.sc_setup_dma	= cosc_setup_dma;
	sc->sc_softc.sc_build_dma_chain = cosc_build_dma_chain;
	sc->sc_softc.sc_need_bump	= cosc_need_bump;

	sc->sc_softc.sc_clock_freq   = 40;   /* Connect32 runs at 40MHz */
	sc->sc_softc.sc_timeout      = 250;  /* Set default timeout to 250ms */
	sc->sc_softc.sc_config_flags = ESC_NO_DMA;
	sc->sc_softc.sc_host_id      = sc->sc_iobase[COSC_CONFIG_CONTROL_REG1] & ESC_DEST_ID_MASK;

	printf(" hostid=%d", sc->sc_softc.sc_host_id);

#if COSC_POLL > 0
        if (boot_args) {
        	char *ptr;
       
		ptr = strstr(boot_args, "nocoscpoll");
		if (ptr)
			cosc_poll = 0;
	}

	if (cosc_poll)
		printf(" polling");
#endif

	sc->sc_softc.sc_bump_sz = NBPG;
	sc->sc_softc.sc_bump_pa = 0x0;

	escinitialize((struct esc_softc *)sc);

	sc->sc_softc.sc_adapter.adapt_dev = &sc->sc_softc.sc_dev;
	sc->sc_softc.sc_adapter.adapt_nchannels = 1;
	sc->sc_softc.sc_adapter.adapt_openings = 7;
	sc->sc_softc.sc_adapter.adapt_max_periph = 1;
	sc->sc_softc.sc_adapter.adapt_ioctl = NULL;
	sc->sc_softc.sc_adapter.adapt_minphys = esc_minphys;
	sc->sc_softc.sc_adapter.adapt_request = cosc_scsi_request;

	sc->sc_softc.sc_channel.chan_adapter = &sc->sc_softc.sc_adapter;
	sc->sc_softc.sc_channel.chan_bustype = &scsi_bustype;
	sc->sc_softc.sc_channel.chan_channel = 0;
	sc->sc_softc.sc_channel.chan_ntargets = 8;
	sc->sc_softc.sc_channel.chan_nluns = 8;  
	sc->sc_softc.sc_channel.chan_id = sc->sc_softc.sc_host_id;

	/* initialise the card */
#if 0
	*rp->inten = (COSC_POLL?0:1);
	*rp->led = 0;
#endif

	sc->sc_softc.sc_ih.ih_func = cosc_intr;
	sc->sc_softc.sc_ih.ih_arg  = &sc->sc_softc;
	sc->sc_softc.sc_ih.ih_level = IPL_BIO;
	sc->sc_softc.sc_ih.ih_name = "scsi: cosc";
	sc->sc_softc.sc_ih.ih_maskaddr = sc->sc_podule->irq_addr;
	sc->sc_softc.sc_ih.ih_maskbits = sc->sc_podule->irq_mask;

#if COSC_POLL > 0
	if (!cosc_poll)
#endif
	{
		evcnt_attach_dynamic(&sc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
		    dp->dv_xname, "intr");
		sc->sc_ih = podulebus_irq_establish(pa->pa_ih, IPL_BIO,
		    cosc_intr, sc, &sc->sc_intrcnt);
		if (sc->sc_ih == NULL)
			panic("%s: Cannot install IRQ handler\n",
			    dp->dv_xname);
	}

	printf("\n");

	/* attach all scsi units on us */
	config_found(dp, &sc->sc_softc.sc_channel, scsiprint);
}


/* Turn on/off led */

void
cosc_led(sc, mode)
	struct esc_softc *sc;
	int		  mode;
{
	cosc_regmap_p		rp;

	rp = (cosc_regmap_p)sc->sc_esc;

	if (mode) {
		sc->sc_led_status++;
	} else {
		if (sc->sc_led_status)
			sc->sc_led_status--;
	}
/*	*rp->led = (sc->sc_led_status?1:0);*/
}


int
cosc_intr(arg)
	void *arg;
{
	struct esc_softc *dev = arg;
	cosc_regmap_p	      rp;
	int		      quickints;

	rp = (cosc_regmap_p)dev->sc_esc;

	printf("cosc_intr:%08x %02x\n", (u_int)rp->esc.esc_status, *rp->esc.esc_status);

	if (*rp->esc.esc_status & ESC_STAT_INTERRUPT_PENDING) {
		quickints = 16;
		do {
			dev->sc_status = *rp->esc.esc_status;
			dev->sc_interrupt = *rp->esc.esc_interrupt;
	  
			if (dev->sc_interrupt & ESC_INT_RESELECTED) {
				dev->sc_resel[0] = *rp->esc.esc_fifo;
				dev->sc_resel[1] = *rp->esc.esc_fifo;
			}

			escintr(dev);

		} while((*rp->esc.esc_status & ESC_STAT_INTERRUPT_PENDING)
			&& --quickints);
	}

	return(0);	/* Pass interrupt on down the chain */
}


/* Load transfer address into dma register */

void
cosc_set_dma_adr(sc, ptr)
	struct esc_softc *sc;
	void		 *ptr;
{
	printf("cosc_set_dma_adr(sc = 0x%08x, ptr = 0x%08x)\n", (u_int)sc, (u_int)ptr);
	return;
}


/* Set DMA transfer counter */

void
cosc_set_dma_tc(sc, len)
	struct esc_softc *sc;
	unsigned int	  len;
{
	printf("cosc_set_dma_tc(sc, len = 0x%08x)", len);

	/* Set the transfer size on the SCSI controller */

	*sc->sc_esc->esc_tc_low  = len; len >>= 8;
	*sc->sc_esc->esc_tc_mid  = len; len >>= 8;
	*sc->sc_esc->esc_tc_high = len;
}


/* Set DMA mode */

void
cosc_set_dma_mode(sc, mode)
	struct esc_softc *sc;
	int		  mode;
{
	printf("cosc_set_dma_mode(sc, mode = %d)", mode);
}


/* Initialize DMA for transfer */

int
cosc_setup_dma(sc, ptr, len, mode)
	struct esc_softc *sc;
	void		 *ptr;
	int		  len;
	int		  mode;
{
/*	printf("cosc_setup_dma(sc, ptr = 0x%08x, len = 0x%08x, mode = 0x%08x)\n", (u_int)ptr, len, mode);*/
	return(0);

}


/* Check if address and len is ok for DMA transfer */

int
cosc_need_bump(sc, ptr, len)
	struct esc_softc *sc;
	void		 *ptr;
	int		  len;
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
cosc_build_dma_chain(sc, chain, p, l)
	struct esc_softc	*sc;
	struct esc_dma_chain	*chain;
	void			*p;
	int			 l;
{
	printf("cosc_build_dma_chain()\n");
	return(0);
}


void
cosc_scsi_request(chan, req, arg)
	struct scsipi_channel *chan;
	scsipi_adapter_req_t req;
	void *arg;
{
	struct scsipi_xfer *xs;

	switch (req) {
	case ADAPTER_REQ_RUN_XFER:
		xs = arg;

#if COSC_POLL > 0
		if (cosc_poll)
			xs->xs_control |= XS_CTL_POLL;
#endif
#if 0
		if (periph->periph_lun == 0)
		printf("id=%d lun=%d cmdlen=%d datalen=%d opcode=%02x flags=%08x status=%02x blk=%02x %02x\n",
		    xs->xs_periph->periph_target, xs->xs_periph->periph_lun, xs->cmdlen, xs->datalen, xs->cmd->opcode,
		    xs->xs_control, xs->status, xs->cmd->bytes[0], xs->cmd->bytes[1]);
#endif
	default:
	}
	esc_scsi_request(chan, req, arg);
}
