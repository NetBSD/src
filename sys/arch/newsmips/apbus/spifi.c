/*	$NetBSD: spifi.c,v 1.4 2001/07/26 11:44:06 tsubai Exp $	*/

/*-
 * Copyright (c) 2000 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/queue.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsi_message.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <newsmips/apbus/apbusvar.h>
#include <newsmips/apbus/spifireg.h>
#include <newsmips/apbus/dmac3reg.h>

#include <machine/adrsmap.h>

/* #define SPIFI_DEBUG */

#ifdef SPIFI_DEBUG
# define DPRINTF printf
#else
# define DPRINTF while (0) printf
#endif

struct spifi_scb {
	TAILQ_ENTRY(spifi_scb) chain;
	int flags;
	struct scsipi_xfer *xs;
	struct scsi_generic cmd;
	int cmdlen;
	int resid;
	vaddr_t daddr;
	u_char target;
	u_char lun;
	u_char lun_targ;
	u_char status;
};
/* scb flags */
#define SPIFI_READ	0x80
#define SPIFI_DMA	0x01

struct spifi_softc {
	struct device sc_dev;
	struct scsipi_channel sc_channel;
	struct scsipi_adapter sc_adapter;

	struct spifi_reg *sc_reg;
	struct spifi_scb *sc_nexus;
	void *sc_dma;			/* attached DMA softc */
	int sc_id;			/* my SCSI ID */
	int sc_msgout;
	u_char sc_omsg[16];
	struct spifi_scb sc_scb[16];
	TAILQ_HEAD(, spifi_scb) free_scb;
	TAILQ_HEAD(, spifi_scb) ready_scb;
};

#define SPIFI_SYNC_OFFSET_MAX	7

#define SEND_REJECT	1
#define SEND_IDENTIFY	2
#define SEND_SDTR	4

#define SPIFI_DATAOUT	0
#define SPIFI_DATAIN	PRS_IO
#define SPIFI_COMMAND	PRS_CD
#define SPIFI_STATUS	(PRS_CD | PRS_IO)
#define SPIFI_MSGOUT	(PRS_MSG | PRS_CD)
#define SPIFI_MSGIN	(PRS_MSG | PRS_CD | PRS_IO)

int spifi_match(struct device *, struct cfdata *, void *);
void spifi_attach(struct device *, struct device *, void *);

void spifi_scsipi_request(struct scsipi_channel *, scsipi_adapter_req_t, void *);
struct spifi_scb *spifi_get_scb(struct spifi_softc *);
void spifi_free_scb(struct spifi_softc *, struct spifi_scb *);
int spifi_poll(struct spifi_softc *);
void spifi_minphys(struct buf *);

void spifi_sched(struct spifi_softc *);
int spifi_intr(void *);
void spifi_pmatch(struct spifi_softc *);

void spifi_select(struct spifi_softc *);
void spifi_sendmsg(struct spifi_softc *, int);
void spifi_command(struct spifi_softc *);
void spifi_data_io(struct spifi_softc *);
void spifi_status(struct spifi_softc *);
int spifi_done(struct spifi_softc *);
void spifi_fifo_drain(struct spifi_softc *);
void spifi_reset(struct spifi_softc *);
void spifi_bus_reset(struct spifi_softc *);

static int spifi_read_count(struct spifi_reg *);
static void spifi_write_count(struct spifi_reg *, int);

#define DMAC3_FASTACCESS(sc)  dmac3_misc((sc)->sc_dma, DMAC3_CONF_FASTACCESS)
#define DMAC3_SLOWACCESS(sc)  dmac3_misc((sc)->sc_dma, DMAC3_CONF_SLOWACCESS)

struct cfattach spifi_ca = {
	sizeof(struct spifi_softc), spifi_match, spifi_attach
};

int
spifi_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct apbus_attach_args *apa = aux;

	if (strcmp(apa->apa_name, "spifi") == 0)
		return 1;

	return 0;
}

void
spifi_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct spifi_softc *sc = (void *)self;
	struct apbus_attach_args *apa = aux;
	struct device *dma;
	int intr, i;

	/* Initialize scbs. */
	TAILQ_INIT(&sc->free_scb);
	TAILQ_INIT(&sc->ready_scb);
	for (i = 0; i < sizeof(sc->sc_scb)/sizeof(sc->sc_scb[0]); i++)
		TAILQ_INSERT_TAIL(&sc->free_scb, &sc->sc_scb[i], chain);

	sc->sc_reg = (struct spifi_reg *)apa->apa_hwbase;
	sc->sc_id = 7;					/* XXX */

	/* Find my dmac3. */
	dma = dmac3_link(apa->apa_ctlnum);
	if (dma == NULL) {
		printf(": cannot find slave dmac\n");
		return;
	}
	sc->sc_dma = dma;

	printf(" slot%d addr 0x%lx", apa->apa_slotno, apa->apa_hwbase);
	printf(": SCSI ID = %d, using %s\n", sc->sc_id, dma->dv_xname);

	dmac3_reset(sc->sc_dma);

	DMAC3_SLOWACCESS(sc);
	spifi_reset(sc);
	DMAC3_FASTACCESS(sc);

	sc->sc_adapter.adapt_dev = &sc->sc_dev;
	sc->sc_adapter.adapt_nchannels = 1;
	sc->sc_adapter.adapt_openings = 7;
	sc->sc_adapter.adapt_max_periph = 1;
	sc->sc_adapter.adapt_ioctl = NULL;
	sc->sc_adapter.adapt_minphys = minphys;
	sc->sc_adapter.adapt_request = spifi_scsipi_request;

	memset(&sc->sc_channel, 0, sizeof(sc->sc_channel));
	sc->sc_channel.chan_adapter = &sc->sc_adapter;
	sc->sc_channel.chan_bustype = &scsi_bustype;
	sc->sc_channel.chan_channel = 0;
	sc->sc_channel.chan_ntargets = 8;
	sc->sc_channel.chan_nluns = 8;
	sc->sc_channel.chan_id = sc->sc_id;

	if (apa->apa_slotno == 0)
		intr = NEWS5000_INT0_DMAC;
	else
		intr = SLOTTOMASK(apa->apa_slotno);
	apbus_intr_establish(0, intr, 0, spifi_intr, sc, apa->apa_name,
	    apa->apa_ctlnum);

	config_found(&sc->sc_dev, &sc->sc_channel, scsiprint);
}

void
spifi_scsipi_request(chan, req, arg)
	struct scsipi_channel *chan;
	scsipi_adapter_req_t req;
	void *arg;
{
	struct scsipi_xfer *xs;
	struct scsipi_periph *periph;
	struct spifi_softc *sc = (void *)chan->chan_adapter->adapt_dev;
	struct spifi_scb *scb;
	u_int flags;
	int s;

	switch (req) {
	case ADAPTER_REQ_RUN_XFER:
		xs = arg;
		periph = xs->xs_periph;

		DPRINTF("spifi_scsi_cmd\n");

		flags = xs->xs_control;

		scb = spifi_get_scb(sc);
		if (scb == NULL) {
			panic("spifi_scsipi_request: no scb\n");
		}

		scb->xs = xs;
		scb->flags = 0;
		scb->status = 0;
		scb->daddr = (vaddr_t)xs->data;
		scb->resid = xs->datalen;
		bcopy(xs->cmd, &scb->cmd, xs->cmdlen);
		scb->cmdlen = xs->cmdlen;

		scb->target = periph->periph_target;
		scb->lun = periph->periph_lun;
		scb->lun_targ = scb->target | (scb->lun << 3);

		if (flags & XS_CTL_DATA_IN)
			scb->flags |= SPIFI_READ;

		s = splbio();

		TAILQ_INSERT_TAIL(&sc->ready_scb, scb, chain);

		if (sc->sc_nexus == NULL)	/* IDLE */
			spifi_sched(sc);

		splx(s);

		if (flags & XS_CTL_POLL) {
			if (spifi_poll(sc)) {
				printf("spifi: timeout\n");
				if (spifi_poll(sc))
					printf("spifi: timeout again\n");
			}
		}
		return;
	case ADAPTER_REQ_GROW_RESOURCES:
		/* XXX Not supported. */
		return;
	case ADAPTER_REQ_SET_XFER_MODE:
		/* XXX Not supported. */
		return;
	}
}

struct spifi_scb *
spifi_get_scb(sc)
	struct spifi_softc *sc;
{
	struct spifi_scb *scb;
	int s;

	s = splbio();
	scb = sc->free_scb.tqh_first;
	if (scb)
		TAILQ_REMOVE(&sc->free_scb, scb, chain);
	splx(s);

	return scb;
}

void
spifi_free_scb(sc, scb)
	struct spifi_softc *sc;
	struct spifi_scb *scb;
{
	int s;

	s = splbio();
	TAILQ_INSERT_HEAD(&sc->free_scb, scb, chain);
	splx(s);
}

int
spifi_poll(sc)
	struct spifi_softc *sc;
{
	struct spifi_scb *scb = sc->sc_nexus;
	struct scsipi_xfer *xs;
	int count;

	printf("spifi_poll: not implemented yet\n");
	delay(10000);
	scb->status = SCSI_OK;
	scb->resid = 0;
	spifi_done(sc);
	return 0;

	if (xs == NULL)
		return 0;

	xs = scb->xs;
	count = xs->timeout;

	while (count > 0) {
		if (dmac3_intr(sc->sc_dma) != 0)
			spifi_intr(sc);

		if (xs->xs_status & XS_STS_DONE)
			return 0;
		DELAY(1000);
		count--;
	};
	return 1;
}

void
spifi_minphys(bp)
	struct buf *bp;
{
	if (bp->b_bcount > 64*1024)
		bp->b_bcount = 64*1024;

	minphys(bp);
}

void
spifi_sched(sc)
	struct spifi_softc *sc;
{
	struct spifi_scb *scb;

	scb = sc->ready_scb.tqh_first;
start:
	if (scb == NULL || sc->sc_nexus != NULL)
		return;
/*
	if (sc->sc_targets[scb->target] & (1 << scb->lun))
		goto next;
*/
	TAILQ_REMOVE(&sc->ready_scb, scb, chain);

#ifdef SPIFI_DEBUG
{
	int i;

	printf("spifi_sched: ID:LUN = %d:%d, ", scb->target, scb->lun);
	printf("cmd = 0x%x", scb->cmd.opcode);
	for (i = 0; i < 5; i++)
		printf(" 0x%x", scb->cmd.bytes[i]);
	printf("\n");
}
#endif

	DMAC3_SLOWACCESS(sc);
	sc->sc_nexus = scb;
	spifi_select(sc);
	DMAC3_FASTACCESS(sc);

	scb = scb->chain.tqe_next;
	goto start;
}

static inline int
spifi_read_count(reg)
	struct spifi_reg *reg;
{
	int count;

	count = (reg->count_hi  & 0xff) |
		(reg->count_mid & 0xff) |
		(reg->count_low & 0xff);
	return count;
}

static inline void
spifi_write_count(reg, count)
	struct spifi_reg *reg;
	int count;
{
	reg->count_hi  = count >> 16;
	reg->count_mid = count >> 8;
	reg->count_low = count;
}


#ifdef SPIFI_DEBUG
static char scsi_phase_name[][8] = {
	"DATAOUT", "DATAIN", "COMMAND", "STATUS",
	"", "", "MSGOUT", "MSGIN"
};
#endif

int
spifi_intr(v)
	void *v;
{
	struct spifi_softc *sc = v;
	struct spifi_reg *reg = sc->sc_reg;
	int intr, state, icond;
	struct spifi_scb *scb;
	struct scsipi_xfer *xs;
#ifdef SPIFI_DEBUG
	char bitmask[64];
#endif

	switch (dmac3_intr(sc->sc_dma)) {
	case 0:
		DPRINTF("sprious dma intr\n");
		return 0;
	case -1:
		printf("DMAC parity error, data PAD\n");

		DMAC3_SLOWACCESS(sc);
		reg->prcmd = PRC_TRPAD;
		DMAC3_FASTACCESS(sc);
		return 1;

	default:
		break;
	}
	DMAC3_SLOWACCESS(sc);

	intr = reg->intr & 0xff;
	if (intr == 0) {
		DMAC3_FASTACCESS(sc);
		DPRINTF("sprious intr (not me)\n");
		return 0;
	}

	scb = sc->sc_nexus;
	xs = scb->xs;
	state = reg->spstat;
	icond = reg->icond;

	/* clear interrupt */
	reg->intr = ~intr;

#ifdef SPIFI_DEBUG
	bitmask_snprintf(intr, INTR_BITMASK, bitmask, sizeof bitmask);
	printf("spifi_intr intr = 0x%s (%s), ", bitmask,
		scsi_phase_name[(reg->prstat >> 3) & 7]);
	printf("state = 0x%x, icond = 0x%x\n", state, icond);
#endif

	if (intr & INTR_FCOMP) {
		spifi_fifo_drain(sc);
		scb->status = reg->cmbuf[scb->target].status;
		scb->resid = spifi_read_count(reg);

		DPRINTF("datalen = %d, resid = %d, status = 0x%x\n",
			xs->datalen, scb->resid, scb->status);
		DPRINTF("msg = 0x%x\n", reg->cmbuf[sc->sc_id].cdb[0]);

		DMAC3_FASTACCESS(sc);
		spifi_done(sc);
		return 1;
	}
	if (intr & INTR_DISCON)
		panic("disconnect");

	if (intr & INTR_TIMEO) {
		xs->error = XS_SELTIMEOUT;
		DMAC3_FASTACCESS(sc);
		spifi_done(sc);
		return 1;
	}
	if (intr & INTR_BSRQ) {
		if (scb == NULL)
			panic("reconnect?");

		if (intr & INTR_PERR) {
			printf("%s: %d:%d parity error\n", sc->sc_dev.dv_xname,
			       scb->target, scb->lun);

			/* XXX reset */
			xs->error = XS_DRIVER_STUFFUP;
			spifi_done(sc);
			return 1;
		}

		if (state >> 4 == SPS_MSGIN && icond == ICOND_NXTREQ)
			panic("spifi_intr: NXTREQ");
		if (reg->fifoctrl & FIFOC_RQOVRN)
			panic("spifi_intr RQOVRN");
		if (icond == ICOND_UXPHASEZ)
			panic("ICOND_UXPHASEZ");

		if ((icond & 0x0f) == ICOND_ADATAOFF) {
			spifi_data_io(sc);
			goto done;
		}
		if ((icond & 0xf0) == ICOND_UBF) {
			reg->exstat = reg->exstat & ~EXS_UBF;
			spifi_pmatch(sc);
			goto done;
		}

		/*
		 * XXX Work around the SPIFI bug that interrupts during
		 * XXX dataout phase.
		 */
		if (state == ((SPS_DATAOUT << 4) | SPS_INTR) &&
		    (reg->prstat & PRS_PHASE) == SPIFI_DATAOUT) {
			reg->prcmd = PRC_DATAOUT;
			goto done;
		}
		if ((reg->prstat & PRS_Z) == 0) {
			spifi_pmatch(sc);
			goto done;
		}

		panic("spifi_intr: unknown intr state");
	}

done:
	DMAC3_FASTACCESS(sc);
	return 1;
}

void
spifi_pmatch(sc)
	struct spifi_softc *sc;
{
	struct spifi_reg *reg = sc->sc_reg;
	int phase;

	phase = (reg->prstat & PRS_PHASE);

#ifdef SPIFI_DEBUG
	printf("spifi_pmatch (%s)\n", scsi_phase_name[phase >> 3]);
#endif

	switch (phase) {

	case SPIFI_COMMAND:
		spifi_command(sc);
		break;
	case SPIFI_DATAIN:
	case SPIFI_DATAOUT:
		spifi_data_io(sc);
		break;
	case SPIFI_STATUS:
		spifi_status(sc);
		break;

	case SPIFI_MSGIN:	/* XXX */
	case SPIFI_MSGOUT:	/* XXX */
	default:
		printf("spifi: unknown phase %d\n", phase);
	}
}

void
spifi_select(sc)
	struct spifi_softc *sc;
{
	struct spifi_reg *reg = sc->sc_reg;
	struct spifi_scb *scb = sc->sc_nexus;
	int sel;

#if 0
	if (reg->loopdata || reg->intr)
		return;
#endif

	if (scb == NULL) {
		printf("%s: spifi_select: NULL nexus\n", sc->sc_dev.dv_xname);
		return;
	}

	reg->exctrl = EXC_IPLOCK;

	dmac3_reset(sc->sc_dma);
	sel = scb->target << 4 | SEL_ISTART | SEL_IRESELEN | SEL_WATN;
	spifi_sendmsg(sc, SEND_IDENTIFY);
	reg->select = sel;
}

void
spifi_sendmsg(sc, msg)
	struct spifi_softc *sc;
	int msg;
{
	struct spifi_scb *scb = sc->sc_nexus;
	/* struct mesh_tinfo *ti; */
	int lun, len, i;

	int id = sc->sc_id;
	struct spifi_reg *reg = sc->sc_reg;

	DPRINTF("spifi_sendmsg: sending");
	sc->sc_msgout = msg;
	len = 0;

	if (msg & SEND_REJECT) {
		DPRINTF(" REJECT");
		sc->sc_omsg[len++] = MSG_MESSAGE_REJECT;
	}
	if (msg & SEND_IDENTIFY) {
		DPRINTF(" IDENTIFY");
		lun = scb->xs->xs_periph->periph_lun;
		sc->sc_omsg[len++] = MSG_IDENTIFY(lun, 0);
	}
	if (msg & SEND_SDTR) {
		DPRINTF(" SDTR");
#if 0
		ti = &sc->sc_tinfo[scb->target];
		sc->sc_omsg[len++] = MSG_EXTENDED;
		sc->sc_omsg[len++] = 3;
		sc->sc_omsg[len++] = MSG_EXT_SDTR;
		sc->sc_omsg[len++] = ti->period;
		sc->sc_omsg[len++] = ti->offset;
#endif
	}
	DPRINTF("\n");

	reg->cmlen = CML_AMSG_EN | len;
	for (i = 0; i < len; i++)
		reg->cmbuf[id].cdb[i] = sc->sc_omsg[i];
}
void
spifi_command(struct spifi_softc *sc)
{
	struct spifi_scb *scb = sc->sc_nexus;
	struct spifi_reg *reg = sc->sc_reg;
	int len = scb->cmdlen;
	u_char *cmdp = (char *)&scb->cmd;
	int i;

	DPRINTF("spifi_command\n");

	reg->cmdpage = scb->lun_targ;

	if (reg->init_status & IST_ACK) {
		/* Negate ACK. */
		reg->prcmd = PRC_NJMP | PRC_CLRACK | PRC_COMMAND;
		reg->prcmd = PRC_NJMP | PRC_COMMAND;
	}

	reg->cmlen = CML_AMSG_EN | len;

	for (i = 0; i < len; i++)
		reg->cmbuf[sc->sc_id].cdb[i] = *cmdp++;

	reg->prcmd = PRC_COMMAND;
}

void
spifi_data_io(struct spifi_softc *sc)
{
	struct spifi_scb *scb = sc->sc_nexus;
	struct spifi_reg *reg = sc->sc_reg;
	int phase;

	DPRINTF("spifi_data_io\n");

	phase = reg->prstat & PRS_PHASE;
	dmac3_reset(sc->sc_dma);

	spifi_write_count(reg, scb->resid);
	reg->cmlen = CML_AMSG_EN | 1;
	reg->data_xfer = 0;

	scb->flags |= SPIFI_DMA;
	if (phase == SPIFI_DATAIN) {
		if (reg->fifoctrl & FIFOC_SSTKACT) {
			/*
			 * Clear FIFO and load the contents of synchronous
			 * stack into the FIFO.
			 */
			reg->fifoctrl = FIFOC_CLREVEN;
			reg->fifoctrl = FIFOC_LOAD;
		}
		reg->autodata = ADATA_IN | scb->lun_targ;
		dmac3_start(sc->sc_dma, scb->daddr, scb->resid, DMAC3_CSR_RECV);
		reg->prcmd = PRC_DATAIN;
	} else {
		reg->fifoctrl = FIFOC_CLREVEN;
		reg->autodata = scb->lun_targ;
		dmac3_start(sc->sc_dma, scb->daddr, scb->resid, DMAC3_CSR_SEND);
		reg->prcmd = PRC_DATAOUT;
	}
}

void
spifi_status(struct spifi_softc *sc)
{
	struct spifi_reg *reg = sc->sc_reg;

	DPRINTF("spifi_status\n");
	spifi_fifo_drain(sc);
	reg->cmlen = CML_AMSG_EN | 1;
	reg->prcmd = PRC_STATUS;
}

int
spifi_done(sc)
	struct spifi_softc *sc;
{
	struct spifi_scb *scb = sc->sc_nexus;
	struct scsipi_xfer *xs = scb->xs;

	DPRINTF("spifi_done\n");

	xs->status = scb->status;
	if (xs->status == SCSI_CHECK) {
		DPRINTF("spifi_done: CHECK CONDITION\n");
		if (xs->error == XS_NOERROR)
			xs->error = XS_BUSY;
	}

	xs->resid = scb->resid;

	scsipi_done(xs);
	spifi_free_scb(sc, scb);

	sc->sc_nexus = NULL;
	spifi_sched(sc);

	return FALSE;
}

void
spifi_fifo_drain(sc)
	struct spifi_softc *sc;
{
	struct spifi_scb *scb = sc->sc_nexus;
	struct spifi_reg *reg = sc->sc_reg;
	int fifoctrl, fifo_count;

	DPRINTF("spifi_fifo_drain\n");

	if ((scb->flags & SPIFI_READ) == 0)
		return;

	fifoctrl = reg->fifoctrl;
	if (fifoctrl & FIFOC_SSTKACT)
		return;

	fifo_count = 8 - (fifoctrl & FIFOC_FSLOT);
	if (fifo_count > 0 && (scb->flags & SPIFI_DMA)) {
		/* Flush data still in FIFO. */
		reg->fifoctrl = FIFOC_FLUSH;
		return;
	}

	reg->fifoctrl = FIFOC_CLREVEN;
}

void
spifi_reset(sc)
	struct spifi_softc *sc;
{
	struct spifi_reg *reg = sc->sc_reg;
	int id = sc->sc_id;

	DPRINTF("spifi_reset\n");

	reg->auxctrl = AUXCTRL_SRST;
	reg->auxctrl = AUXCTRL_CRST;

	dmac3_reset(sc->sc_dma);

	reg->auxctrl = AUXCTRL_SRST;
	reg->auxctrl = AUXCTRL_CRST;
	reg->auxctrl = AUXCTRL_DMAEDGE;

	/* Mask (only) target mode interrupts. */
	reg->imask = INTR_TGSEL | INTR_COMRECV;

	reg->config = CONFIG_DMABURST | CONFIG_PCHKEN | CONFIG_PGENEN | id;
	reg->fastwide = FAST_FASTEN;
	reg->prctrl = 0;
	reg->loopctrl = 0;

	/* Enable automatic status input except the initiator. */
	reg->autostat = ~(1 << id);

	reg->fifoctrl = FIFOC_CLREVEN;
	spifi_write_count(reg, 0);

	/* Flush write buffer. */
	(void)reg->spstat;
}

void
spifi_bus_reset(sc)
	struct spifi_softc *sc;
{
	struct spifi_reg *reg = sc->sc_reg;

	printf("%s: bus reset\n", sc->sc_dev.dv_xname);

	sc->sc_nexus = NULL;

	reg->auxctrl = AUXCTRL_SETRST;
	delay(100);
	reg->auxctrl = 0;
}

#if 0
static u_char spifi_sync_period[] = {
/* 0    1    2    3   4   5   6   7   8   9  10  11 */
 137, 125, 112, 100, 87, 75, 62, 50, 43, 37, 31, 25
};

void
spifi_setsync(sc, ti)
	struct spifi_softc *sc;
	struct spifi_tinfo *ti;
{
	if ((ti->flags & T_SYNCMODE) == 0)
		reg->data_xfer = 0;
	else {
		int period = ti->period;
		int offset = ti->offset;
		int v;

		for (v = sizeof(spifi_sync_period) - 1; v >= 0; v--)
			if (spifi_sync_period[v] >= period)
				break;
		if (v == -1)
			reg->data_xfer = 0;			/* XXX */
		else
			reg->data_xfer = v << 4 | offset;
	}
}
#endif
