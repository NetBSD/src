/*	$NetBSD: sc_wrap.c,v 1.11.8.1 1999/12/27 18:33:07 wrstuden Exp $	*/

/*
 * This driver is slow!  Need to rewrite.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/malloc.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/scsipi/scsi_message.h>

#include <newsmips/dev/scsireg.h>
#include <newsmips/dev/dmac_0448.h>
#include <newsmips/dev/screg_1185.h>

#include <machine/locore.h>
#include <machine/adrsmap.h>
#include <machine/autoconf.h>
#include <machine/machConst.h>

static int cxd1185_match __P((struct device *, struct cfdata *, void *));
static void cxd1185_attach __P((struct device *, struct device *, void *));

struct cfattach sc_ca = {
	sizeof(struct sc_softc), cxd1185_match, cxd1185_attach
};

void cxd1185_init __P((struct sc_softc *));
static void free_scb __P((struct sc_softc *, struct sc_scb *));
static struct sc_scb *get_scb __P((struct sc_softc *, int));
static int sc_scsi_cmd __P((struct scsipi_xfer *));
static int sc_poll __P((struct sc_softc *, int, int));
static void sc_sched __P((struct sc_softc *));
void sc_done __P((struct sc_scb *));
int sc_intr __P((void *));
static void cxd1185_timeout __P((void *));

extern void sc_send __P((struct sc_scb *, int, int));
extern int scintr __P((void));
extern void scsi_hardreset __P((void));
extern int sc_busy __P((struct sc_softc *, int));
extern paddr_t kvtophys __P((vaddr_t));

static int sc_disconnect = IDT_DISCON;

struct scsipi_device cxd1185_dev = {
	NULL,
	NULL,
	NULL,
	NULL
};

int
cxd1185_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, "sc"))
		return 0;

	return 1;
}

void
cxd1185_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct sc_softc *sc = (void *)self;
	struct sc_scb *scb;
	int i, intlevel;

	intlevel = sc->sc_dev.dv_cfdata->cf_level;
	if (intlevel == -1) {
#if 0
		printf(": interrupt level not configured\n");
		return;
#else
		printf(": interrupt level not configured; using");
		intlevel = 0;
#endif
	}
	printf(" level %d\n", intlevel);

	if (sc_idenr & 0x08)
		sc->scsi_1185AQ = 1;
	else
		sc->scsi_1185AQ = 0;

	sc->sc_adapter.scsipi_cmd = sc_scsi_cmd;
	sc->sc_adapter.scsipi_minphys = minphys;

	sc->sc_link.scsipi_scsi.channel = SCSI_CHANNEL_ONLY_ONE;
	sc->sc_link.adapter_softc = sc;
	sc->sc_link.scsipi_scsi.adapter_target = 7;
	sc->sc_link.adapter = &sc->sc_adapter;
	sc->sc_link.device = &cxd1185_dev;
	sc->sc_link.openings = 2;
	sc->sc_link.scsipi_scsi.max_target = 7;
	sc->sc_link.scsipi_scsi.max_lun = 7;
	sc->sc_link.type = BUS_SCSI;

	TAILQ_INIT(&sc->ready_list);
	TAILQ_INIT(&sc->free_list);

	scb = sc->sc_scb;
	for (i = 0; i < 24; i++) {	/* XXX 24 */
		TAILQ_INSERT_TAIL(&sc->free_list, scb, chain);
		scb++;
	}

	cxd1185_init(sc);
	DELAY(100000);

	hb_intr_establish(intlevel, IPL_BIO, sc_intr, sc);

	config_found(&sc->sc_dev, &sc->sc_link, scsiprint);
}

void
cxd1185_init(sc)
	struct sc_softc *sc;
{
	int i;

	for (i = 0; i < 8; i++)
		sc->inuse[i] = 0;

	scsi_hardreset();
}

void
free_scb(sc, scb)
	struct sc_softc *sc;
	struct sc_scb *scb;
{
	int s;

	s = splbio();

	TAILQ_INSERT_HEAD(&sc->free_list, scb, chain);

	/*
	 * If there were none, wake anybody waiting for one to come free,
	 * starting with queued entries.
	 */
	if (scb->chain.tqe_next == 0)
		wakeup(&sc->free_list);

	splx(s);
}

struct sc_scb *
get_scb(sc, flags)
	struct sc_softc *sc;
	int flags;
{
	int s;
	struct sc_scb *scb;

	s = splbio();

	while ((scb = sc->free_list.tqh_first) == NULL &&
		(flags & XS_CTL_NOSLEEP) == 0)
		tsleep(&sc->free_list, PRIBIO, "sc_scb", 0);
	if (scb) {
		TAILQ_REMOVE(&sc->free_list, scb, chain);
	}

	splx(s);
	return scb;
}

int
sc_scsi_cmd(xs)
	struct scsipi_xfer *xs;
{
	struct scsipi_link *sc_link = xs->sc_link;
	struct sc_softc *sc = sc_link->adapter_softc;
	struct sc_scb *scb;
	int flags, s;
	int chan;

	flags = xs->xs_control;
	if ((scb = get_scb(sc, flags)) == NULL)
		return TRY_AGAIN_LATER;

	scb->xs = xs;
	scb->flags = 0;
	scb->sc_ctag = 0;
	scb->sc_coffset = 0;
	scb->istatus = 0;
	scb->tstatus = 0;
	scb->message = 0;
	bzero(scb->msgbuf, sizeof(scb->msgbuf));

	s = splbio();

	TAILQ_INSERT_TAIL(&sc->ready_list, scb, chain);
	sc_sched(sc);
	splx(s);

	if ((flags & XS_CTL_POLL) == 0)
		return SUCCESSFULLY_QUEUED;

	chan = sc_link->scsipi_scsi.target;

	if (sc_poll(sc, chan, xs->timeout)) {
		printf("sc: timeout (retry)\n");
		if (sc_poll(sc, chan, xs->timeout)) {
			printf("sc: timeout\n");
			return COMPLETE;
		}
	}

	/* called during autoconfig only... */

	MachFlushCache(); /* Flush all caches */
	return COMPLETE;
}

/*
 * Used when interrupt driven I/O isn't allowed, e.g. during boot.
 */
int
sc_poll(sc, chan, count)
	struct sc_softc *sc;
	int chan, count;
{
	volatile u_char *int_stat = (void *)INTST1;
	volatile u_char *int_clear = (void *)INTCLR1;

	while (sc_busy(sc, chan)) {
		if (*int_stat & INTST1_DMA) {
		    *int_clear = INTST1_DMA;
		    if (dmac_gstat & CH_INT(CH_SCSI)) {
			if (dmac_gstat & CH_MRQ(CH_SCSI)) {
			    DELAY(50);
			    if (dmac_gstat & CH_MRQ(CH_SCSI))
				printf("dma_poll\n");
			}
			DELAY(10);
			scintr();
		    }
		}
		DELAY(1000);
		count--;
		if (count <= 0)
			return 1;
	}
	return 0;
}

void
sc_sched(sc)
	struct sc_softc *sc;
{
	struct scsipi_xfer *xs;
	struct scsipi_link *sc_link;
	int ie = 0;
	int flags;
	int chan, lun;
	struct sc_scb *scb, *nextscb;

	scb = sc->ready_list.tqh_first;
start:
	if (scb == NULL)
		return;

	xs = scb->xs;
	sc_link = xs->sc_link;
	chan = sc_link->scsipi_scsi.target;
	flags = xs->xs_control;

	if (cold)
		flags |= XS_CTL_POLL;

	if (sc->inuse[chan]) {
		scb = scb->chain.tqe_next;
		goto start;
	}
	sc->inuse[chan] = 1;

	if (flags & XS_CTL_RESET)
		printf("SCSI RESET\n");

	lun = sc_link->scsipi_scsi.lun;

	scb->identify = MSG_IDENT | sc_disconnect | (lun & IDT_DRMASK);
	scb->sc_ctrnscnt = xs->datalen;

	/* make va->pa mapping table for dma */
	if (xs->datalen > 0) {
		int pages, offset;
		int i, pn;
		vaddr_t va;

		/* bzero(&sc->sc_map[chan], sizeof(struct sc_map)); */

		va = (vaddr_t)xs->data;

		offset = va & PGOFSET;
		pages = (offset + xs->datalen + NBPG -1 ) >> PGSHIFT;
		if (pages >= NSCMAP)
			panic("sc_map: Too many pages");

		for (i = 0; i < pages; i++) {
			pn = kvtophys(va) >> PGSHIFT;
			sc->sc_map[chan].mp_addr[i] = pn;
			va += NBPG;
		}

		sc->sc_map[chan].mp_offset = offset;
		sc->sc_map[chan].mp_pages = pages;
		scb->sc_map = &sc->sc_map[chan];
	}

	if ((flags & XS_CTL_POLL) == 0)
		ie = SCSI_INTEN;

	if (xs->data)
		scb->sc_cpoint = (void *)xs->data;
	else
		scb->sc_cpoint = scb->msgbuf;
	scb->scb_softc = sc;

	timeout(cxd1185_timeout, scb, hz * 10);
	sc_send(scb, chan, ie);
	untimeout(cxd1185_timeout, scb);

	nextscb = scb->chain.tqe_next;

	TAILQ_REMOVE(&sc->ready_list, scb, chain);

	scb = nextscb;

	goto start;
}

void
sc_done(scb)
	struct sc_scb *scb;
{
	struct scsipi_xfer *xs = scb->xs;
	struct scsipi_link *sc_link = xs->sc_link;
	struct sc_softc *sc = sc_link->adapter_softc;

	xs->xs_status |= XS_STS_DONE;
	xs->resid = 0;
	xs->status = 0;

	if (scb->istatus != INST_EP) {
		if (scb->istatus == INST_EP|INST_TO)
			xs->error = XS_SELTIMEOUT;
		else {
			printf("SC(i): [istatus=0x%x, tstatus=0x%x]\n",
				scb->istatus, scb->tstatus);
			xs->error = XS_DRIVER_STUFFUP;
		}
	}

	switch (scb->tstatus) {

	case TGST_GOOD:
		break;

	case TGST_CC:
		break;		/* XXX */
#if 0
		chan = sc_link->scsipi_scsi.target;
		lun = sc_link->scsipi_scsi.lun;
		scop_rsense(chan, scb, lun, SCSI_INTDIS, 18, 0);
		if (scb->tstatus != TGST_GOOD) {
			printf("SC(t2): [istatus=0x%x, tstatus=0x%x]\n",
				scb->istatus, scb->tstatus);
		}
#endif

	default:
		printf("SC(t): [istatus=0x%x, tstatus=0x%x]\n",
			scb->istatus, scb->tstatus);
		break;
	}

	scsipi_done(xs);
	free_scb(sc, scb);
	sc->inuse[sc_link->scsipi_scsi.target] = 0;
	sc_sched(sc);
}

int
sc_intr(v)
	void *v;
{
	/* struct sc_softc *sc = v; */
	volatile u_char *gsp = (u_char *)DMAC_GSTAT;
	u_int gstat = *gsp;
	int mrqb, i;

	if ((gstat & CH_INT(CH_SCSI)) == 0)
		return 0;

	/*
	 * when DMA interrupt occurs there remain some untransferred data.
	 * wait data transfer completion.
	 */
	mrqb = (gstat & CH_INT(CH_SCSI)) << 1;
	if (gstat & mrqb) {
		/*
		 * XXX SHOULD USE DELAY()
		 */
		for (i = 0; i < 50; i++)
			;
		if (*gsp & mrqb)
			printf("sc_intr: MRQ\n");
	}
	scintr();

	return 1;
}


#if 0
/*
 * SCOP_RSENSE request
 */
void
scop_rsense(intr, sc_param, lun, ie, count, param)
	register int intr;
	register struct scsi *sc_param;
	register int lun;
	register int ie;
	register int count;
	register caddr_t param;
{
	bzero(sc_param, sizeof(struct scsi));
	sc_param->identify = MSG_IDENT | sc_disconnect | (lun & IDT_DRMASK);
	sc_param->sc_lun = lun;

	sc_param->sc_cpoint = (u_char *)param;
	sc_param->sc_ctrnscnt = count;

	/* sc_cdb */
	sc_param->sc_opcode = SCOP_RSENSE;
	sc_param->sc_count = count;

	sc_go(intr, sc_param, ie, sc_param);
}
#endif

void
cxd1185_timeout(arg)
	void *arg;
{
	struct sc_scb *scb = arg;
	struct scsipi_xfer *xs = scb->xs;
	struct scsipi_link *sc_link = xs->sc_link;
	int chan;

	chan = sc_link->scsipi_scsi.target;

	printf("sc: timeout ch=%d\n", chan);

	/* XXX abort transfer and ... */
}
