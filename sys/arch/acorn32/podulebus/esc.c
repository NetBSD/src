/*	$NetBSD: esc.c,v 1.10 2003/04/01 02:13:53 thorpej Exp $	*/

/*
 * Copyright (c) 1995 Scott Stevens
 * Copyright (c) 1995 Daniel Widenfalk
 * Copyright (c) 1994 Christian E. Hopps
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Van Jacobson of Lawrence Berkeley Laboratory.
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
 *	@(#)scsi.c	7.5 (Berkeley) 5/4/91
 */

/*
 * AMD AM53CF94 scsi adaptor driver
 *
 * Functionally compatible with the FAS216
 *
 * Apart from a very small patch to set up control register 4
 */

/*
 * Modified for NetBSD/arm32 by Scott Stevens
 */

#include <sys/param.h>

__RCSID("$NetBSD: esc.c,v 1.10 2003/04/01 02:13:53 thorpej Exp $");

#include <sys/systm.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <uvm/uvm_extern.h>

#include <machine/pmap.h>
#include <machine/cpu.h>
#include <machine/io.h>
#include <machine/intr.h>
#include <arm/arm32/katelib.h>
#include <acorn32/podulebus/podulebus.h>
#include <acorn32/podulebus/escreg.h>
#include <acorn32/podulebus/escvar.h>

void escinitialize __P((struct esc_softc *));
void esc_minphys   __P((struct buf *bp));
void esc_scsi_request __P((struct scsipi_channel *,
				scsipi_adapter_req_t, void *));
void esc_donextcmd __P((struct esc_softc *dev, struct esc_pending *pendp));
void esc_scsidone  __P((struct esc_softc *dev, struct scsipi_xfer *xs,
			 int stat));
void escintr	    __P((struct esc_softc *dev));
void esciwait	    __P((struct esc_softc *dev));
void escreset	    __P((struct esc_softc *dev, int how));
int  escselect	    __P((struct esc_softc *dev, struct esc_pending *pendp,
			 unsigned char *cbuf, int clen,
			 unsigned char *buf, int len, int mode));
void escicmd	    __P((struct esc_softc *dev, struct esc_pending *pendp));
int escgo         __P((struct esc_softc *dev, struct esc_pending *pendp));

void esc_init_nexus(struct esc_softc *, struct nexus *);
void esc_save_pointers(struct esc_softc *);
void esc_restore_pointers(struct esc_softc *);
void esc_ixfer(struct esc_softc *);
void esc_build_sdtrm(struct esc_softc *, int, int);
int esc_select_unit(struct esc_softc *,	short);
struct nexus *esc_arbitate_target(struct esc_softc *, int);
void esc_setup_nexus(struct esc_softc *, struct nexus *, struct esc_pending *,
    unsigned char *, int, unsigned char *, int, int);
int esc_pretests(struct esc_softc *, esc_regmap_p);
int esc_midaction(struct esc_softc *, esc_regmap_p, struct nexus *);
int esc_postaction(struct esc_softc *, esc_regmap_p, struct nexus *);


/*
 * Initialize these to make 'em patchable. Defaults to enable sync and discon.
 */
u_char	esc_inhibit_sync[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
u_char	esc_inhibit_disc[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

#define DEBUG
#ifdef DEBUG
#define QPRINTF(a) if (esc_debug > 1) printf a
int	esc_debug = 2;
#else
#define QPRINTF
#endif

/*
 * default minphys routine for esc based controllers
 */
void
esc_minphys(bp)
	struct buf *bp;
{

	/*
	 * No max transfer at this level.
	 */
	minphys(bp);
}

/*
 * Initialize the nexus structs.
 */
void
esc_init_nexus(dev, nexus)
	struct esc_softc *dev;
	struct nexus	  *nexus;
{
	bzero(nexus, sizeof(struct nexus));

	nexus->state	= ESC_NS_IDLE;
	nexus->period	= 200;
	nexus->offset	= 0;
	nexus->syncper	= 5;
	nexus->syncoff	= 0;
	nexus->config3	= dev->sc_config3 & ~ESC_CFG3_FASTSCSI;
}

void
escinitialize(dev)
	struct esc_softc *dev;
{
	u_int		*pte;
	int		 i;

	dev->sc_led_status = 0;

	TAILQ_INIT(&dev->sc_xs_pending);
	TAILQ_INIT(&dev->sc_xs_free);

/*
 * Initialize the esc_pending structs and link them into the free list. We
 * have to set vm_link_data.pages to 0 or the vm FIX won't work.
 */
	for(i=0; i<MAXPENDING; i++) {
		TAILQ_INSERT_TAIL(&dev->sc_xs_free, &dev->sc_xs_store[i],
				  link);
	}

/*
 * Calculate the correct clock conversion factor 2 <= factor <= 8, i.e. set
 * the factor to clock_freq / 5 (int).
 */
	if (dev->sc_clock_freq <= 10)
		dev->sc_clock_conv_fact = 2;
	if (dev->sc_clock_freq <= 40)
		dev->sc_clock_conv_fact = 2+((dev->sc_clock_freq-10)/5);
	else
		panic("escinitialize: Clock frequence too high");

/* Setup and save the basic configuration registers */
	dev->sc_config1 = (dev->sc_host_id & ESC_CFG1_BUS_ID_MASK);
	dev->sc_config2 = ESC_CFG2_FEATURES_ENABLE;
	dev->sc_config3 = (dev->sc_clock_freq > 25 ? ESC_CFG3_FASTCLK : 0);

/* Precalculate timeout value and clock period. */
/* Ekkk ... floating point in the kernel !!!! */
/*	dev->sc_timeout_val  = 1+dev->sc_timeout*dev->sc_clock_freq/
				 (7.682*dev->sc_clock_conv_fact);*/
	dev->sc_timeout_val  = 1+dev->sc_timeout*dev->sc_clock_freq/
				 ((7682*dev->sc_clock_conv_fact)/1000);
	dev->sc_clock_period = 1000/dev->sc_clock_freq;

	escreset(dev, 1 | 2);	/* Reset Chip and Bus */

	dev->sc_units_disconnected = 0;
	dev->sc_msg_in_len = 0;
	dev->sc_msg_out_len = 0;

	dev->sc_flags = 0;

	for(i=0; i<8; i++)
		esc_init_nexus(dev, &dev->sc_nexus[i]);

/*
 * Setup bump buffer.
 */
	dev->sc_bump_va = (u_char *)uvm_km_zalloc(kernel_map, dev->sc_bump_sz);
	(void) pmap_extract(pmap_kernel(), (vaddr_t)dev->sc_bump_va,
	    (paddr_t *)&dev->sc_bump_pa);

/*
 * Setup pages to noncachable, that way we don't have to flush the cache
 * every time we need "bumped" transfer.
 */
	pte = vtopte((vaddr_t) dev->sc_bump_va);
	*pte &= ~L2_C;
	PTE_SYNC(pte);
	cpu_tlb_flushD();
	cpu_dcache_wbinv_range((vm_offset_t)dev->sc_bump_va, PAGE_SIZE);

	printf(" dmabuf V0x%08x P0x%08x", (u_int)dev->sc_bump_va, (u_int)dev->sc_bump_pa);
}


/*
 * used by specific esc controller
 */
void
esc_scsi_request(struct scsipi_channel *chan, scsipi_adapter_req_t req,
 								void *arg)
{
	struct scsipi_xfer *xs;
	struct esc_softc	*dev = (void *)chan->chan_adapter->adapt_dev;
	struct scsipi_periph	*periph;
	struct esc_pending	*pendp;
	int			 flags, s, target;

	switch (req) {
	case ADAPTER_REQ_RUN_XFER:
		xs = arg;
		periph = xs->xs_periph;
		flags = xs->xs_control;
		target = periph->periph_target;

		if (flags & XS_CTL_DATA_UIO)
			panic("esc: scsi data uio requested");

		if ((flags & XS_CTL_POLL) && (dev->sc_flags & ESC_ACTIVE))
			panic("esc_scsicmd: busy");

/* Get hold of a esc_pending block. */
		s = splbio();
		pendp = dev->sc_xs_free.tqh_first;
		if (pendp == NULL) {
			splx(s);
			xs->error = XS_RESOURCE_SHORTAGE;
			scsipi_done(xs);
			return;
		}
		TAILQ_REMOVE(&dev->sc_xs_free, pendp, link);
		pendp->xs = xs;
		splx(s);


/* If the chip if busy OR the unit is busy, we have to wait for out turn. */
		if ((dev->sc_flags & ESC_ACTIVE) ||
		    (dev->sc_nexus[target].flags & ESC_NF_UNIT_BUSY)) {
			s = splbio();
			TAILQ_INSERT_TAIL(&dev->sc_xs_pending, pendp, link);
			splx(s);
		} else
			esc_donextcmd(dev, pendp);

		return;
	case ADAPTER_REQ_GROW_RESOURCES:
	case ADAPTER_REQ_SET_XFER_MODE:
		/* XXX Not supported. */
		return;
	}

}

/*
 * Actually select the unit, whereby the whole scsi-process is started.
 */
void
esc_donextcmd(dev, pendp)
	struct esc_softc	*dev;
	struct esc_pending	*pendp;
{
	int	s;

/*
 * Special case for scsi unit reset. I think this is waterproof. We first
 * select the unit during splbio. We then cycle through the generated
 * interrupts until the interrupt routine signals that the unit has
 * acknowledged the reset. After that we have to wait a reset to select
 * delay before anything else can happend.
 */
	if (pendp->xs->xs_control & XS_CTL_RESET) {
		struct nexus	*nexus;

		s = splbio();
		while(!escselect(dev, pendp, 0, 0, 0, 0, ESC_SELECT_K)) {
			splx(s);
			delay(10);
			s = splbio();
		}

		nexus = dev->sc_cur_nexus;
		while(nexus->flags & ESC_NF_UNIT_BUSY) {
			esciwait(dev);
			escintr(dev);
		}

		nexus->flags |= ESC_NF_UNIT_BUSY;
		splx(s);

		escreset(dev, 0);

		s = splbio();
		nexus->flags &= ~ESC_NF_UNIT_BUSY;
		splx(s);
	}

/*
 * If we are polling, go to splbio and perform the command, else we poke
 * the scsi-bus via escgo to get the interrupt machine going.
 */
	if (pendp->xs->xs_control & XS_CTL_POLL) {
		s = splbio();
		escicmd(dev, pendp);
		TAILQ_INSERT_TAIL(&dev->sc_xs_free, pendp, link);
		splx(s);
	} else {
		escgo(dev, pendp);
		return;
	}
}

void
esc_scsidone(dev, xs, stat)
	struct esc_softc *dev;
	struct scsipi_xfer *xs;
	int		 stat;
{
	struct esc_pending	*pendp;
	int			 s;

	xs->status = stat;

	if (stat == 0)
		xs->resid = 0;
	else {
		switch(stat) {
		case SCSI_CHECK:
		case SCSI_BUSY:
			xs->error = XS_BUSY;
			break;
		case -1:
			xs->error = XS_DRIVER_STUFFUP;
			QPRINTF(("esc_scsicmd() bad %x\n", stat));
			break;
		default:
			xs->error = XS_TIMEOUT;
			break;
		}
	}

/* Steal the next command from the queue so that one unit can't hog the bus. */
	s = splbio();
	pendp = dev->sc_xs_pending.tqh_first;
	while(pendp) {
		if (!(dev->sc_nexus[pendp->xs->xs_periph->periph_target].flags &
		      ESC_NF_UNIT_BUSY))
			break;
		pendp = pendp->link.tqe_next;
	}

	if (pendp != NULL) {
		TAILQ_REMOVE(&dev->sc_xs_pending, pendp, link);
	}

	splx(s);
	scsipi_done(xs);

	if (pendp)
		esc_donextcmd(dev, pendp);
}

/*
 * There are two kinds of reset:
 *  1) CHIP-bus reset. This also implies a SCSI-bus reset.
 *  2) SCSI-bus reset.
 * After the appropriate resets have been performed we wait a reset to select
 * delay time.
 */
void
escreset(dev, how)
	struct esc_softc *dev;
	int		 how;
{
	esc_regmap_p	rp;
	int		i, s;

	rp = dev->sc_esc;

	if (how & 1) {
		for(i=0; i<8; i++)
			esc_init_nexus(dev, &dev->sc_nexus[i]);

		*rp->esc_command = ESC_CMD_RESET_CHIP;
		delay(1);
		*rp->esc_command = ESC_CMD_NOP;

		*rp->esc_config1 = dev->sc_config1;
		*rp->esc_config2 = dev->sc_config2;
		*rp->esc_config3 = dev->sc_config3;
		*rp->esc_config4 = dev->sc_config4;
		*rp->esc_timeout = dev->sc_timeout_val;
		*rp->esc_clkconv = dev->sc_clock_conv_fact &
					ESC_CLOCK_CONVERSION_MASK;
	}

	if (how & 2) {
		for(i=0; i<8; i++)
			esc_init_nexus(dev, &dev->sc_nexus[i]);

		s = splbio();

		*rp->esc_command = ESC_CMD_RESET_SCSI_BUS;
		delay(100);

/* Skip interrupt generated by RESET_SCSI_BUS */
		while(*rp->esc_status & ESC_STAT_INTERRUPT_PENDING) {
			dev->sc_status = *rp->esc_status;
			dev->sc_interrupt = *rp->esc_interrupt;

			delay(100);
		}

		dev->sc_status = *rp->esc_status;
		dev->sc_interrupt = *rp->esc_interrupt;

		splx(s);
	}

	if (dev->sc_config_flags & ESC_SLOW_START)
		delay(4*250000); /* RESET to SELECT DELAY*4 for slow devices */
	else
		delay(250000);	 /* RESET to SELECT DELAY */
}

/*
 * Save active data pointers to the nexus block currently active.
 */
void
esc_save_pointers(dev)
	struct esc_softc *dev;
{
	struct nexus	*nx;

	nx = dev->sc_cur_nexus;
	if (nx) {
		nx->cur_link	= dev->sc_cur_link;
		nx->max_link	= dev->sc_max_link;
		nx->buf		= dev->sc_buf;
		nx->len		= dev->sc_len;
		nx->dma_len	= dev->sc_dma_len;
		nx->dma_buf	= dev->sc_dma_buf;
		nx->dma_blk_flg	= dev->sc_dma_blk_flg;
		nx->dma_blk_len	= dev->sc_dma_blk_len;
		nx->dma_blk_ptr	= dev->sc_dma_blk_ptr;
	}
}

/*
 * Restore data pointers from the currently active nexus block.
 */
void
esc_restore_pointers(dev)
	struct esc_softc *dev;
{
	struct nexus	*nx;

	nx = dev->sc_cur_nexus;
	if (nx) {
		dev->sc_cur_link    = nx->cur_link;
		dev->sc_max_link    = nx->max_link;
		dev->sc_buf	    = nx->buf;
		dev->sc_len	    = nx->len;
		dev->sc_dma_len	    = nx->dma_len;
		dev->sc_dma_buf	    = nx->dma_buf;
		dev->sc_dma_blk_flg = nx->dma_blk_flg;
		dev->sc_dma_blk_len = nx->dma_blk_len;
		dev->sc_dma_blk_ptr = nx->dma_blk_ptr;
		dev->sc_chain	    = nx->dma;
		dev->sc_unit	    = (nx->lun_unit & 0x0F);
		dev->sc_lun	    = (nx->lun_unit & 0xF0) >> 4;
	}
}

/*
 * esciwait is used during interrupt and polled IO to wait for an event from
 * the FAS chip. This function MUST NOT BE CALLED without interrupt disabled.
 */
void
esciwait(dev)
	struct esc_softc *dev;
{
	esc_regmap_p	rp;

/*
 * If ESC_DONT_WAIT is set, we have already grabbed the interrupt info
 * elsewhere. So we don't have to wait for it.
 */
	if (dev->sc_flags & ESC_DONT_WAIT) {
		dev->sc_flags &= ~ESC_DONT_WAIT;
		return;
	}

	rp = dev->sc_esc;

/* Wait for FAS chip to signal an interrupt. */
	while(!(*rp->esc_status & ESC_STAT_INTERRUPT_PENDING));
/*		delay(1);*/

/* Grab interrupt info from chip. */
	dev->sc_status = *rp->esc_status;
	dev->sc_interrupt = *rp->esc_interrupt;
	if (dev->sc_interrupt & ESC_INT_RESELECTED) {
		dev->sc_resel[0] = *rp->esc_fifo;
		dev->sc_resel[1] = *rp->esc_fifo;
	}
}

#if 0
/*
 * Transfer info to/from device. esc_ixfer uses polled IO+esciwait so the
 * rules that apply to esciwait also applies here.
 */
void
esc_ixfer(dev)
	struct esc_softc *dev;
{
	esc_regmap_p	 rp;
	u_char		*buf;
	int		 len, mode, phase;

	rp = dev->sc_esc;
	buf = dev->sc_buf;
	len = dev->sc_len;

/*
 * Decode the scsi phase to determine whether we are reading or writing.
 * mode == 1 => READ, mode == 0 => WRITE
 */
	phase = dev->sc_status & ESC_STAT_PHASE_MASK;
	mode = (phase == ESC_PHASE_DATA_IN);

	while(len && ((dev->sc_status & ESC_STAT_PHASE_MASK) == phase))
		if (mode) {
			*rp->esc_command = ESC_CMD_TRANSFER_INFO;

			esciwait(dev);

			*buf++ = *rp->esc_fifo;
			len--;
		} else {
			len--;
			*rp->esc_fifo = *buf++;
			*rp->esc_command = ESC_CMD_TRANSFER_INFO;

			esciwait(dev);
		}

/* Update buffer pointers to reflect the sent/received data. */
	dev->sc_buf = buf;
	dev->sc_len = len;

/*
 * Since the last esciwait will be a phase-change, we can't wait for it
 * again later, so we have to signal that.
 */

	dev->sc_flags |= ESC_DONT_WAIT;
}
#else
/*
 * Transfer info to/from device. esc_ixfer uses polled IO+esciwait so the
 * rules that apply to esciwait also applies here.
 */
void
esc_ixfer(dev)
	struct esc_softc *dev;
{
	esc_regmap_p	 rp;
	vu_char		*esc_status;
	vu_char		*esc_command;
	vu_char		*esc_interrupt;
	vu_char		*esc_fifo;
	u_char		*buf;
	int		 len, mode, phase;

	rp = dev->sc_esc;
	buf = dev->sc_buf;
	len = dev->sc_len;

	/* Use discrete variables for better optimisation */

	esc_status = rp->esc_status;
	esc_command = rp->esc_command;
	esc_interrupt = rp->esc_interrupt;
	esc_fifo = rp->esc_fifo;

/*
 * Decode the scsi phase to determine whether we are reading or writing.
 * mode == 1 => READ, mode == 0 => WRITE
 */
	phase = dev->sc_status & ESC_STAT_PHASE_MASK;
	mode = (phase == ESC_PHASE_DATA_IN);

	if (mode) {
		while(len && ((dev->sc_status & ESC_STAT_PHASE_MASK) == phase)) {
			*esc_command = ESC_CMD_TRANSFER_INFO;

			/* Wait for FAS chip to signal an interrupt. */
			while(!(*esc_status & ESC_STAT_INTERRUPT_PENDING));
/*				delay(1);*/

			/* Grab interrupt info from chip. */
			dev->sc_status = *esc_status;
			dev->sc_interrupt = *esc_interrupt;

			*buf++ = *esc_fifo;
			len--;
		}
	} else {
		while(len && ((dev->sc_status & ESC_STAT_PHASE_MASK) == phase)) {
			len--;
			*esc_fifo = *buf++;
			*esc_command = ESC_CMD_TRANSFER_INFO;

			/* Wait for FAS chip to signal an interrupt. */
			while(!(*esc_status & ESC_STAT_INTERRUPT_PENDING));
/*				delay(1);*/

			/* Grab interrupt info from chip. */
			dev->sc_status = *esc_status;
			dev->sc_interrupt = *esc_interrupt;
		}
	}
	
/* Update buffer pointers to reflect the sent/received data. */
	dev->sc_buf = buf;
	dev->sc_len = len;

/*
 * Since the last esciwait will be a phase-change, we can't wait for it
 * again later, so we have to signal that.
 */

	dev->sc_flags |= ESC_DONT_WAIT;
}
#endif

/*
 * Build a Synchronous Data Transfer Request message
 */
void
esc_build_sdtrm(dev, period, offset)
	struct esc_softc *dev;
	int		  period;
	int		  offset;
{
	dev->sc_msg_out[0] = 0x01;
	dev->sc_msg_out[1] = 0x03;
	dev->sc_msg_out[2] = 0x01;
	dev->sc_msg_out[3] = period/4;
	dev->sc_msg_out[4] = offset;
	dev->sc_msg_out_len= 5;
}

/*
 * Arbitate the scsi bus and select the unit
 */
int
esc_select_unit(dev, target)
	struct esc_softc *dev;
	short		  target;
{
	esc_regmap_p	 rp;
	struct nexus	*nexus;
	int		 s, retcode, i;
	u_char		 cmd;

	s = splbio();	/* Do this at splbio so that we won't be disturbed. */

	retcode = 0;

	nexus = &dev->sc_nexus[target];

/*
 * Check if the chip is busy. If not the we mark it as so and hope that nobody
 * reselects us until we have grabbed the bus.
 */
	if (!(dev->sc_flags & ESC_ACTIVE) && !dev->sc_sel_nexus) {
		dev->sc_flags |= ESC_ACTIVE;

		rp = dev->sc_esc;

		*rp->esc_syncper = nexus->syncper;
		*rp->esc_syncoff = nexus->syncoff;
		*rp->esc_config3 = nexus->config3;

		*rp->esc_config1 = dev->sc_config1;
		*rp->esc_timeout = dev->sc_timeout_val;
		*rp->esc_dest_id = target;

/* If nobody has stolen the bus, we can send a select command to the chip. */
		if (!(*rp->esc_status & ESC_STAT_INTERRUPT_PENDING)) {
			*rp->esc_fifo = nexus->ID;
			if ((nexus->flags & (ESC_NF_DO_SDTR | ESC_NF_RESET))
			    || (dev->sc_msg_out_len != 0))
				cmd = ESC_CMD_SEL_ATN_STOP;
			else {
				for(i=0; i<nexus->clen; i++)
					*rp->esc_fifo = nexus->cbuf[i];

				cmd = ESC_CMD_SEL_ATN;
			}

			dev->sc_sel_nexus = nexus;

			*rp->esc_command = cmd;
			retcode = 1;
			nexus->flags &= ~ESC_NF_RETRY_SELECT;
		} else
			nexus->flags |= ESC_NF_RETRY_SELECT;
	} else
		nexus->flags |= ESC_NF_RETRY_SELECT;

	splx(s);
	return(retcode);
}

/*
 * Grab the nexus if available else return 0.
 */
struct nexus *
esc_arbitate_target(dev, target)
	struct esc_softc *dev;
	int		  target;
{
	struct nexus	*nexus;
	int		 s;

/*
 * This is realy simple. Raise interrupt level to splbio. Grab the nexus and
 * leave.
 */
	nexus = &dev->sc_nexus[target];

	s = splbio();

	if (nexus->flags & ESC_NF_UNIT_BUSY)
		nexus = 0;
	else
		nexus->flags |= ESC_NF_UNIT_BUSY;

	splx(s);
	return(nexus);
}

/*
 * Setup a nexus for use. Initializes command, buffer pointers and dma chain.
 */
void
esc_setup_nexus(dev, nexus, pendp, cbuf, clen, buf, len, mode)
	struct esc_softc	*dev;
	struct nexus		*nexus;
	struct esc_pending	*pendp;
	unsigned char		*cbuf;
	int			 clen;
	unsigned char		*buf;
	int			 len;
	int			 mode;
{
	int	sync, target, lun;

	target = pendp->xs->xs_periph->periph_target;
	lun    = pendp->xs->xs_periph->periph_lun;

/*
 * Adopt mode to reflect the config flags.
 * If we can't use DMA we can't use synch transfer. Also check the
 * esc_inhibit_xxx[target] flags.
 */
	if ((dev->sc_config_flags & (ESC_NO_SYNCH | ESC_NO_DMA)) ||
	    esc_inhibit_sync[(int)target])
		mode &= ~ESC_SELECT_S;

	if ((dev->sc_config_flags & ESC_NO_RESELECT) ||
	    esc_inhibit_disc[(int)target])
		mode &= ~ESC_SELECT_R;

	nexus->xs		= pendp->xs;

/* Setup the nexus struct. */
	nexus->ID	   = ((mode & ESC_SELECT_R) ? 0xC0 : 0x80) | lun;
	nexus->clen	   = clen;
	bcopy(cbuf, nexus->cbuf, nexus->clen);
	nexus->cbuf[1] |= lun << 5;		/* Fix the lun bits */
	nexus->cur_link	   = 0;
	nexus->dma_len	   = 0;
	nexus->dma_buf	   = 0;
	nexus->dma_blk_len = 0;
	nexus->dma_blk_ptr = 0;
	nexus->len	   = len;
	nexus->buf	   = buf;
	nexus->lun_unit	   = (lun << 4) | target;
	nexus->state	   = ESC_NS_SELECTED;

/* We must keep these flags. All else must be zero. */
	nexus->flags	  &= ESC_NF_UNIT_BUSY
			   | ESC_NF_SYNC_TESTED | ESC_NF_SELECT_ME;

	if (mode & ESC_SELECT_I)
		nexus->flags |= ESC_NF_IMMEDIATE;
	if (mode & ESC_SELECT_K)
		nexus->flags |= ESC_NF_RESET;

	sync  = ((mode & ESC_SELECT_S) ? 1 : 0);

/* We can't use sync during polled IO. */
	if (sync && (mode & ESC_SELECT_I))
		sync = 0;

	if (!sync &&
	    ((nexus->flags & ESC_NF_SYNC_TESTED) && (nexus->offset != 0))) {
		/*
		 * If the scsi unit is set to synch transfer and we don't want
		 * that, we have to renegotiate.
		 */

		nexus->flags |= ESC_NF_DO_SDTR;
		nexus->period = 200;
		nexus->offset = 0;
	} else if (sync && !(nexus->flags & ESC_NF_SYNC_TESTED)) {
		/*
		 * If the scsi unit is not set to synch transfer and we want
		 * that, we have to negotiate. This should realy base the
		 * period on the clock frequence rather than just check if
		 * >25Mhz
		 */

		nexus->flags |= ESC_NF_DO_SDTR;
		nexus->period = ((dev->sc_clock_freq>25) ? 100 : 200);
		nexus->offset = 8;

		/* If the user has a long cable, we want to limit the period */
		if ((nexus->period == 100) &&
		    (dev->sc_config_flags & ESC_SLOW_CABLE))
			nexus->period = 200;
	}

/*
 * Fake a dma-block for polled IO. This way we can use the same code to handle
 * reselection. Much nicer this way.
 */
	if ((mode & ESC_SELECT_I) || (dev->sc_config_flags & ESC_NO_DMA)) {
		nexus->dma[0].ptr = buf;
		nexus->dma[0].len = len;
		nexus->dma[0].flg = ESC_CHAIN_PRG;
		nexus->max_link   = 1;
	} else {
		nexus->max_link = dev->sc_build_dma_chain(dev, nexus->dma,
							  buf, len);
	}

/* Flush the caches. */

	if (len && !(mode & ESC_SELECT_I))
		cpu_dcache_wbinv_range((vm_offset_t)buf, len);
}

int
escselect(dev, pendp, cbuf, clen, buf, len, mode)
	struct esc_softc	*dev;
	struct esc_pending	*pendp;
	unsigned char		*cbuf;
	int			 clen;
	unsigned char		*buf;
	int			 len;
	int			 mode;
{
	struct nexus	*nexus;

/* Get the nexus struct. */
	nexus = esc_arbitate_target(dev, pendp->xs->xs_periph->periph_target);
	if (nexus == NULL)
		return(0);

/* Setup the nexus struct. */
	esc_setup_nexus(dev, nexus, pendp, cbuf, clen, buf, len, mode);

/* Post it to the interrupt machine. */
	esc_select_unit(dev, pendp->xs->xs_periph->periph_target);

	return(1);
}

int
escgo(dev, pendp)
	struct esc_softc   *dev;
	struct esc_pending *pendp;
{
	int	 s;
	char	*buf;

	buf    = pendp->xs->data;

	if (escselect(dev, pendp, (char *)pendp->xs->cmd, pendp->xs->cmdlen,
		      buf, pendp->xs->datalen, ESC_SELECT_RS)) {
		/*
		 * We got the command going so the esc_pending struct is now
		 * free to reuse.
		 */

		s = splbio();
		TAILQ_INSERT_TAIL(&dev->sc_xs_free, pendp, link);
		splx(s);
	} else {
		/*
		 * We couldn't make the command fly so we have to wait. The
		 * struct MUST be inserted at the head to keep the order of
		 * the commands.
		 */

		s = splbio();
		TAILQ_INSERT_HEAD(&dev->sc_xs_pending, pendp, link);
		splx(s);
	}

	return(0);
}

/*
 * Part one of the interrupt machine. Error checks and reselection test.
 * We don't know if we have an active nexus here!
 */
int
esc_pretests(dev, rp)
	struct esc_softc *dev;
	esc_regmap_p	  rp;
{
	struct nexus	*nexus;
	int		 i, s;

	if (dev->sc_interrupt & ESC_INT_SCSI_RESET_DETECTED) {
		/*
		 * Cleanup and notify user. Lets hope that this is all we
		 * have to do
		 */

		for(i=0; i<8; i++) {
			if (dev->sc_nexus[i].xs)
				esc_scsidone(dev, dev->sc_nexus[i].xs, -2);

			esc_init_nexus(dev, &dev->sc_nexus[i]);
		}
		printf("escintr: SCSI-RESET detected!");
		return(-1);
	}

	if (dev->sc_interrupt & ESC_INT_ILLEGAL_COMMAND) {
		/* Something went terrible wrong! Dump some data and panic! */

		printf("FIFO:");
		while(*rp->esc_fifo_flags & ESC_FIFO_COUNT_MASK)
			printf(" %x", *rp->esc_fifo);
		printf("\n");

		printf("CMD: %x\n", *rp->esc_command);
		panic("escintr: ILLEGAL COMMAND!");
	}

	if (dev->sc_interrupt & ESC_INT_RESELECTED) {
		/* We were reselected. Set the chip as busy */

		s = splbio();
		dev->sc_flags |= ESC_ACTIVE;
		if (dev->sc_sel_nexus) {
			dev->sc_sel_nexus->flags |= ESC_NF_SELECT_ME;
			dev->sc_sel_nexus = 0;
		}
		splx(s);

		if (dev->sc_units_disconnected) {
			/* Find out who reselected us. */

			dev->sc_resel[0] &= ~(1<<dev->sc_host_id);

			for(i=0; i<8; i++)
				if (dev->sc_resel[0] & (1<<i))
					break;

			if (i == 8)
				panic("Illegal reselection!");

			if (dev->sc_nexus[i].state == ESC_NS_DISCONNECTED) {
				/*
				 * This unit had disconnected, so we reconnect
				 * it.
				 */

				dev->sc_cur_nexus = &dev->sc_nexus[i];
				nexus = dev->sc_cur_nexus;

				*rp->esc_syncper = nexus->syncper;
				*rp->esc_syncoff = nexus->syncoff;
				*rp->esc_config3 = nexus->config3;

				*rp->esc_dest_id = i & 7;

				dev->sc_units_disconnected--;
				dev->sc_msg_in_len= 0;

				/* Restore active pointers. */
				esc_restore_pointers(dev);

				nexus->state = ESC_NS_RESELECTED;

				*rp->esc_command = ESC_CMD_MESSAGE_ACCEPTED;

				return(1);
			}
		}

		/* Somehow we got an illegal reselection. Dump and panic. */
		printf("escintr: resel[0] %x resel[1] %x disconnected %d\n",
		       dev->sc_resel[0], dev->sc_resel[1],
		       dev->sc_units_disconnected);
		panic("escintr: Unexpected reselection!");
	}

	return(0);
}

/*
 * Part two of the interrupt machine. Handle disconnection and post command
 * processing. We know that we have an active nexus here.
 */
int
esc_midaction(dev, rp, nexus)
	struct esc_softc *dev;
	esc_regmap_p	  rp;
	struct nexus	 *nexus;
{
	int	i, left, len, s;
	u_char	status, msg;

	if (dev->sc_interrupt & ESC_INT_DISCONNECT) {
		s = splbio();
		dev->sc_cur_nexus = 0;

		/* Mark chip as busy and clean up the chip FIFO. */
		dev->sc_flags &= ~ESC_ACTIVE;
		*rp->esc_command = ESC_CMD_FLUSH_FIFO;

		/* Let the nexus state reflect what we have to do. */
		switch(nexus->state) {
		case ESC_NS_SELECTED:
			dev->sc_sel_nexus = 0;
			nexus->flags &= ~ESC_NF_SELECT_ME;

			/*
			 * We were trying to select the unit. Probably no unit
			 * at this ID.
			 */
			nexus->xs->resid = dev->sc_len;

			nexus->status = -2;
			nexus->flags &= ~ESC_NF_UNIT_BUSY;
			nexus->state = ESC_NS_FINISHED;
			break;

		case ESC_NS_DONE:
			/* All done. */
			nexus->xs->resid = dev->sc_len;

			nexus->flags &= ~ESC_NF_UNIT_BUSY;
			nexus->state  = ESC_NS_FINISHED;
			dev->sc_led(dev, 0);
			break;

		case ESC_NS_DISCONNECTING:
			/*
			 * We have received a DISCONNECT message, so we are
			 * doing a normal disconnection.
			 */
			nexus->state = ESC_NS_DISCONNECTED;

			dev->sc_units_disconnected++;
			break;

		case ESC_NS_RESET:
			/*
			 * We were reseting this SCSI-unit. Clean up the
			 * nexus struct.
			 */
			dev->sc_led(dev, 0);
			esc_init_nexus(dev, nexus);
			break;

		default:
			/*
			 * Unexpected disconnection! Cleanup and exit. This
			 * shouldn't cause any problems.
			 */
			printf("escintr: Unexpected disconnection\n");
			printf("escintr: u %x s %d p %d f %x c %x\n",
			       nexus->lun_unit, nexus->state,
			       dev->sc_status & ESC_STAT_PHASE_MASK,
			       nexus->flags, nexus->cbuf[0]);

			nexus->xs->resid = dev->sc_len;

			nexus->flags &= ~ESC_NF_UNIT_BUSY;
			nexus->state = ESC_NS_FINISHED;
			nexus->status = -3;

			dev->sc_led(dev, 0);
			break;
		}

		/*
		 * If we have disconnected units, we MUST enable reselection
		 * within 250ms.
		 */
		if (dev->sc_units_disconnected &&
		    !(dev->sc_flags & ESC_ACTIVE))
			*rp->esc_command = ESC_CMD_ENABLE_RESEL;

		splx(s);

		/* Select the first pre-initialized nexus we find. */
		for(i=0; i<8; i++)
			if (dev->sc_nexus[i].flags & (ESC_NF_SELECT_ME | ESC_NF_RETRY_SELECT))
				if (esc_select_unit(dev, i) == 2)
					break;

		/* We are done with this nexus! */
		if (nexus->state == ESC_NS_FINISHED)
			esc_scsidone(dev, nexus->xs, nexus->status);

		return(1);
	}

	switch(nexus->state) {
	case ESC_NS_SELECTED:
		dev->sc_cur_nexus = nexus;
		dev->sc_sel_nexus = 0;

		nexus->flags &= ~ESC_NF_SELECT_ME;

		/*
		 * We have selected a unit. Setup chip, restore pointers and
		 * light the led.
		 */
		*rp->esc_syncper = nexus->syncper;
		*rp->esc_syncoff = nexus->syncoff;
		*rp->esc_config3 = nexus->config3;

		esc_restore_pointers(dev);

		nexus->status	= 0xFF;
		dev->sc_msg_in[0] = 0xFF;
		dev->sc_msg_in_len= 0;

		dev->sc_led(dev, 1);

		break;

	case ESC_NS_DATA_IN:
	case ESC_NS_DATA_OUT:
		/* We have transfered data. */
		if (dev->sc_dma_len)
			if (dev->sc_cur_link < dev->sc_max_link) {
				/*
				 * Clean up dma and at the same time get how
				 * many bytes that were NOT transfered.
				 */
			  left = dev->sc_setup_dma(dev, 0, 0, ESC_DMA_CLEAR);
			  len  = dev->sc_dma_len;

			  if (nexus->state == ESC_NS_DATA_IN) {
			    /*
			     * If we were bumping we may have had an odd length
			     * which means that there may be bytes left in the
			     * fifo. We also need to move the data from the
			     * bump buffer to the actual memory.
			     */
			    if (dev->sc_dma_buf == dev->sc_bump_pa)
			    {
			      while((*rp->esc_fifo_flags&ESC_FIFO_COUNT_MASK)
				    && left)
				dev->sc_bump_va[len-(left--)] = *rp->esc_fifo;

			      bcopy(dev->sc_bump_va, dev->sc_buf, len-left);
			    }
			  } else {
			    /* Count any unsent bytes and flush them. */
			    left+= *rp->esc_fifo_flags & ESC_FIFO_COUNT_MASK;
			    *rp->esc_command = ESC_CMD_FLUSH_FIFO;
			  }

			  /*
			   * Update pointers/length to reflect the transfered
			   * data.
			   */
			  dev->sc_len -= len-left;
			  dev->sc_buf += len-left;

			  dev->sc_dma_buf = (char *)dev->sc_dma_buf + len-left;
			  dev->sc_dma_len = left;

			  dev->sc_dma_blk_ptr = (char *)dev->sc_dma_blk_ptr +
				  len-left;
			  dev->sc_dma_blk_len -= len-left;

			  /*
			   * If it was the end of a dma block, we select the
			   * next to begin with.
			   */
			  if (!dev->sc_dma_blk_len)
			    dev->sc_cur_link++;
			}
		break;

	case ESC_NS_STATUS:
		/*
		 * If we were not sensing, grab the status byte. If we were
		 * sensing and we got a bad status, let the user know.
		 */

		status = *rp->esc_fifo;
		msg = *rp->esc_fifo;

		nexus->status = status;
		if (status != 0)
			nexus->status = -1;

		/*
		 * Preload the command complete message. Handeled in
		 * esc_postaction.
		 */
		dev->sc_msg_in[0] = msg;
		dev->sc_msg_in_len = 1;
		nexus->flags |= ESC_NF_HAS_MSG;
		break;

	default:
		break;
	}

	return(0);
}

/*
 * Part three of the interrupt machine. Handle phase changes (and repeated
 * phase passes). We know that we have an active nexus here.
 */
int
esc_postaction(dev, rp, nexus)
	struct esc_softc *dev;
	esc_regmap_p	  rp;
	struct nexus	 *nexus;
{
	int	i, len;
	u_char	cmd;
	short	offset, period;

	cmd = 0;

	switch(dev->sc_status & ESC_STAT_PHASE_MASK) {
	case ESC_PHASE_DATA_OUT:
	case ESC_PHASE_DATA_IN:
		if ((dev->sc_status & ESC_STAT_PHASE_MASK) ==
		    ESC_PHASE_DATA_OUT)
			nexus->state = ESC_NS_DATA_OUT;
		else
			nexus->state = ESC_NS_DATA_IN;

		/* Make DMA ready to accept new data. Load active pointers
		 * from the DMA block. */
		dev->sc_setup_dma(dev, 0, 0, ESC_DMA_CLEAR);
		if (dev->sc_cur_link < dev->sc_max_link) {
		  if (!dev->sc_dma_blk_len) {
		    dev->sc_dma_blk_ptr = dev->sc_chain[dev->sc_cur_link].ptr;
		    dev->sc_dma_blk_len = dev->sc_chain[dev->sc_cur_link].len;
		    dev->sc_dma_blk_flg = dev->sc_chain[dev->sc_cur_link].flg;
		  }

		  /* We should use polled IO here. */
		  if (dev->sc_dma_blk_flg == ESC_CHAIN_PRG) {
			esc_ixfer(dev/*, nexus->xs->xs_control & XS_CTL_POLL*/);
			dev->sc_cur_link++;
			dev->sc_dma_len = 0;
			break;
		  }
		  else if (dev->sc_dma_blk_flg == ESC_CHAIN_BUMP)
			len = dev->sc_dma_blk_len;
		  else
			len = dev->sc_need_bump(dev,
						(void *)dev->sc_dma_blk_ptr,
						dev->sc_dma_blk_len);

		  /*
		   * If len != 0 we must bump the data, else we just DMA it
		   * straight into memory.
		   */
		  if (len) {
			dev->sc_dma_buf = dev->sc_bump_pa;
			dev->sc_dma_len = len;

			if (nexus->state == ESC_NS_DATA_OUT)
			  bcopy(dev->sc_buf, dev->sc_bump_va, dev->sc_dma_len);
		  } else {
			dev->sc_dma_buf = dev->sc_dma_blk_ptr;
			dev->sc_dma_len = dev->sc_dma_blk_len;
		  }

		  /* Load DMA with adress and length of transfer. */
		  dev->sc_setup_dma(dev, (void *)dev->sc_dma_buf,
		  		    dev->sc_dma_len,
				    ((nexus->state == ESC_NS_DATA_OUT) ?
				     ESC_DMA_WRITE : ESC_DMA_READ));

		  printf("Using DMA !!!!\n");
		  cmd = ESC_CMD_TRANSFER_INFO | ESC_CMD_DMA;
		} else {
			/*
			 * Hmmm, the unit wants more info than we have or has
			 * more than we want. Let the chip handle that.
			 */

			*rp->esc_tc_low = 0;
			*rp->esc_tc_mid = 1;
			*rp->esc_tc_high = 0;
			cmd = ESC_CMD_TRANSFER_PAD;
		}
		break;

	case ESC_PHASE_COMMAND:
		/* The scsi unit wants the command, send it. */
		nexus->state = ESC_NS_SVC;

		*rp->esc_command = ESC_CMD_FLUSH_FIFO;
		for(i=0; i<5; i++);

		for(i=0; i<nexus->clen; i++)
			*rp->esc_fifo = nexus->cbuf[i];
		cmd = ESC_CMD_TRANSFER_INFO;
		break;

	case ESC_PHASE_STATUS:
		/*
		 * We've got status phase. Request status and command
		 * complete message.
		 */
		nexus->state = ESC_NS_STATUS;
		cmd = ESC_CMD_COMMAND_COMPLETE;
		break;

	case ESC_PHASE_MESSAGE_OUT:
		/*
		 * Either the scsi unit wants us to send a message or we have
		 * asked for it by seting the ATN bit.
		 */
		nexus->state = ESC_NS_MSG_OUT;

		*rp->esc_command = ESC_CMD_FLUSH_FIFO;

		if (nexus->flags & ESC_NF_DO_SDTR) {
			/* Send a Synchronous Data Transfer Request. */

			esc_build_sdtrm(dev, nexus->period, nexus->offset);
			nexus->flags |= ESC_NF_SDTR_SENT;
			nexus->flags &= ~ESC_NF_DO_SDTR;
		} else if (nexus->flags & ESC_NF_RESET) {
			/* Send a reset scsi unit message. */

			dev->sc_msg_out[0] = 0x0C;
			dev->sc_msg_out_len = 1;
			nexus->state = ESC_NS_RESET;
			nexus->flags &= ~ESC_NF_RESET;
		} else if (dev->sc_msg_out_len == 0) {
			/* Don't know what to send so we send a NOP message. */

			dev->sc_msg_out[0] = 0x08;
			dev->sc_msg_out_len = 1;
		}

		cmd = ESC_CMD_TRANSFER_INFO;

		for(i=0; i<dev->sc_msg_out_len; i++)
			*rp->esc_fifo = dev->sc_msg_out[i];
		dev->sc_msg_out_len = 0;

		break;

	case ESC_PHASE_MESSAGE_IN:
		/* Receive a message from the scsi unit. */
		nexus->state = ESC_NS_MSG_IN;

		while(!(nexus->flags & ESC_NF_HAS_MSG)) {
			*rp->esc_command = ESC_CMD_TRANSFER_INFO;
			esciwait(dev);

			dev->sc_msg_in[dev->sc_msg_in_len++] = *rp->esc_fifo;

			/* Check if we got all the bytes in the message. */
			if (dev->sc_msg_in[0] >= 0x80)       ;
			else if (dev->sc_msg_in[0] >= 0x30)  ;
			else if (((dev->sc_msg_in[0] >= 0x20) &&
				  (dev->sc_msg_in_len == 2)) ||
				 ((dev->sc_msg_in[0] != 0x01) &&
				  (dev->sc_msg_in_len == 1))) {
				nexus->flags |= ESC_NF_HAS_MSG;
				break;
			} else {
			  if (dev->sc_msg_in_len >= 2)
			    if ((dev->sc_msg_in[1]+2) == dev->sc_msg_in_len) {
				nexus->flags |= ESC_NF_HAS_MSG;
				break;
			    }
			}

			*rp->esc_command = ESC_CMD_MESSAGE_ACCEPTED;
			esciwait(dev);

			if ((dev->sc_status & ESC_STAT_PHASE_MASK) !=
			    ESC_PHASE_MESSAGE_IN)
				break;
		}

		cmd = ESC_CMD_MESSAGE_ACCEPTED;
		if (nexus->flags & ESC_NF_HAS_MSG) {
			/* We have a message. Decode it. */

			switch(dev->sc_msg_in[0]) {
			case 0x00:	/* COMMAND COMPLETE */
				nexus->state = ESC_NS_DONE;
			case 0x04:	/* DISCONNECT */
				nexus->state = ESC_NS_DISCONNECTING;
				break;
			case 0x02:	/* SAVE DATA POINTER */
				esc_save_pointers(dev);
				break;
			case 0x03:	/* RESTORE DATA POINTERS */
				esc_restore_pointers(dev);
				break;
			case 0x07:	/* MESSAGE REJECT */
				/*
				 * If we had sent a SDTR and we got a message
				 * reject, the scsi docs say that we must go
				 * to async transfer.
				 */
				if (nexus->flags & ESC_NF_SDTR_SENT) {
					nexus->flags &= ~ESC_NF_SDTR_SENT;

					nexus->config3 &= ~ESC_CFG3_FASTSCSI;
					nexus->syncper = 5;
					nexus->syncoff = 0;

					*rp->esc_syncper = nexus->syncper;
					*rp->esc_syncoff = nexus->syncoff;
					*rp->esc_config3 = nexus->config3;
				} else
				/*
				 * Something was rejected but we don't know
				 * what! PANIC!
				 */
				  panic("escintr: Unknown message rejected!");
				break;
			case 0x08:	/* MO OPERATION */
				break;
			case 0x01:	/* EXTENDED MESSAGE */
				switch(dev->sc_msg_in[2]) {
				case 0x01:/* SYNC. DATA TRANSFER REQUEST */
					/* Decode the SDTR message. */
					period = 4*dev->sc_msg_in[3];
					offset = dev->sc_msg_in[4];

					/*
					 * Make sure that the specs are within
					 * chip limits. Note that if we
					 * initiated the negotiation the specs
					 * WILL be withing chip limits. If it
					 * was the scsi unit that initiated
					 * the negotiation, the specs may be
					 * to high.
					 */
					if (offset > 16)
						offset = 16;
					if ((period < 200) &&
					    (dev->sc_clock_freq <= 25))
						period = 200;

					if (offset == 0)
					       period = 5*dev->sc_clock_period;

					nexus->syncper = period/
							  dev->sc_clock_period;
					nexus->syncoff = offset;

					if (period < 200)
					  nexus->config3 |= ESC_CFG3_FASTSCSI;
					else
					  nexus->config3 &=~ESC_CFG3_FASTSCSI;

					nexus->flags |= ESC_NF_SYNC_TESTED;

					*rp->esc_syncper = nexus->syncper;
					*rp->esc_syncoff = nexus->syncoff;
					*rp->esc_config3 = nexus->config3;

					/*
					 * Hmmm, it seems that the scsi unit
					 * initiated sync negotiation, so lets
					 * reply acording to scsi-2 standard.
					 */
					if (!(nexus->flags& ESC_NF_SDTR_SENT))
					{
					  if ((dev->sc_config_flags &
					       ESC_NO_SYNCH) ||
					      (dev->sc_config_flags &
					       ESC_NO_DMA) ||
					      esc_inhibit_sync[
							nexus->lun_unit & 7]) {
					          period = 200;
					          offset = 0;
					  }

					  nexus->offset = offset;
					  nexus->period = period;
					  nexus->flags |= ESC_NF_DO_SDTR;
					  *rp->esc_command = ESC_CMD_SET_ATN;
					}

					nexus->flags &= ~ESC_NF_SDTR_SENT;
					break;

				case 0x00: /* MODIFY DATA POINTERS */
				case 0x02: /* EXTENDED IDENTIFY (SCSI-1) */
				case 0x03: /* WIDE DATA TRANSFER REQUEST */
			        default:
					/* Reject any unhandeled messages. */

					dev->sc_msg_out[0] = 0x07;
					dev->sc_msg_out_len = 1;
					*rp->esc_command = ESC_CMD_SET_ATN;
					cmd = ESC_CMD_MESSAGE_ACCEPTED;
					break;
				}
				break;

			default:
				/* Reject any unhandeled messages. */

				dev->sc_msg_out[0] = 0x07;
				dev->sc_msg_out_len = 1;
				*rp->esc_command = ESC_CMD_SET_ATN;
				cmd = ESC_CMD_MESSAGE_ACCEPTED;
				break;
			}
			nexus->flags &= ~ESC_NF_HAS_MSG;
			dev->sc_msg_in_len = 0;
		}
		break;
	default:
		printf("ESCINTR: UNKNOWN PHASE! phase: %d\n",
		       dev->sc_status & ESC_STAT_PHASE_MASK);
		dev->sc_led(dev, 0);
		esc_scsidone(dev, nexus->xs, -4);

		return(-1);
	}

	if (cmd)
		*rp->esc_command = cmd;

	return(0);
}

/*
 * Stub for interrupt machine.
 */
void
escintr(dev)
	struct esc_softc *dev;
{
	esc_regmap_p	 rp;
	struct nexus	*nexus;

	rp = dev->sc_esc;

	if (!esc_pretests(dev, rp)) {

		nexus = dev->sc_cur_nexus;
		if (nexus == NULL)
			nexus = dev->sc_sel_nexus;

		if (nexus)
			if (!esc_midaction(dev, rp, nexus))
				esc_postaction(dev, rp, nexus);
	}
}

/*
 * escicmd is used to perform IO when we can't use interrupts. escicmd
 * emulates the normal environment by waiting for the chip and calling
 * escintr.
 */
void
escicmd(dev, pendp)
	struct esc_softc   *dev;
	struct esc_pending *pendp;
{
	esc_regmap_p	 rp;
	struct nexus	*nexus;

	nexus = &dev->sc_nexus[pendp->xs->xs_periph->periph_target];
	rp = dev->sc_esc;

	if (!escselect(dev, pendp, (char *)pendp->xs->cmd, pendp->xs->cmdlen,
			(char *)pendp->xs->data, pendp->xs->datalen,
			ESC_SELECT_I))
		panic("escicmd: Couldn't select unit");

	while(nexus->state != ESC_NS_FINISHED) {
		esciwait(dev);
		escintr(dev);
	}

	nexus->flags &= ~ESC_NF_SYNC_TESTED;
}
