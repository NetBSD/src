/*
  This driver is slow!  Need to rewrite.
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
#include <newsmips/dev/scsic.h>
#include <newsmips/dev/dmac_0448.h>
#include <newsmips/dev/screg_1185.h>

#include <machine/locore.h>
#include <machine/adrsmap.h>
#include <machine/autoconf.h>
#include <machine/machConst.h>

extern int cold;

/* XXX shared data area with scsi_1185.c */
struct sc_data sc_data[8];

static int sc_disconnect = IDT_DISCON;

struct sc_scb {
	TAILQ_ENTRY(sc_scb) chain;
	struct scsipi_xfer *xs;
	int flags;
};

struct sc_softc {
	struct device sc_dev;
	struct scsipi_link sc_link;

	TAILQ_HEAD(scb_list, sc_scb) ready_list, free_list;
	struct sc_scb sc_scb[3*8];

	struct scsi scsi_xxx[8];
	int inuse[8];
	struct sc_map sc_map[8];
};


static int cxd1185_match __P((struct device *, struct cfdata *, void *));
static void cxd1185_attach __P((struct device *, struct device *, void *));

struct cfattach sc_ca = {
	sizeof(struct sc_softc), cxd1185_match, cxd1185_attach
};

extern struct cfdriver sc_cd;

void cxd1185_init __P((struct sc_softc *));
static void free_scb __P((struct sc_softc *, struct sc_scb *, int));
static struct sc_scb *get_scb __P((struct sc_softc *, int));
static int sc_scsi_cmd __P((struct scsipi_xfer *));
static int sc_poll __P((int, int));
static void sc_sched __P((struct sc_softc *));
static void sc_go __P((int, struct scsi *, int));
void sc_done __P((struct scsi *));
int sc_intr __P((struct sc_softc *));
static void cxd1185_timeout __P((void *));

extern sc_send __P((int, int, struct scsi *));
extern int scintr __P((void));
extern void scsi_hardreset __P((void));
extern int sc_busy __P((int));
extern vm_offset_t kvtophys __P((vm_offset_t));

struct scsipi_adapter cxd1185_switch = {
	sc_scsi_cmd,
	minphys,
	NULL,
	NULL
};

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
	int i;
	extern int scsi_1185AQ;

	if (sc_idenr & 0x08)
		scsi_1185AQ = 1;

	sc->sc_link.scsipi_scsi.channel = SCSI_CHANNEL_ONLY_ONE;
	sc->sc_link.adapter_softc = sc;
	sc->sc_link.scsipi_scsi.adapter_target = 7;
	sc->sc_link.adapter = &cxd1185_switch;
	sc->sc_link.device = &cxd1185_dev;
	sc->sc_link.openings = 2;
	sc->sc_link.scsipi_scsi.max_target = 7;
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

	printf("\n");
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
free_scb(sc, scb, flags)
	struct sc_softc *sc;
	struct sc_scb *scb;
	int flags;
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
		(flags & SCSI_NOSLEEP) == 0)
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
	int intr;
	struct sc_data *scdp;

	flags = xs->flags;
	if ((scb = get_scb(sc, flags)) == NULL)
		return TRY_AGAIN_LATER;

	scb->xs = xs;

	s = splbio();

	TAILQ_INSERT_TAIL(&sc->ready_list, scb, chain);
	sc_sched(sc);
	splx(s);

	if ((flags & SCSI_POLL) == 0)
		return SUCCESSFULLY_QUEUED;

	intr = sc_link->scsipi_scsi.target;

	if (sc_poll(intr, xs->timeout)) {
		printf("sc: timeout (retry)\n");
		if (sc_poll(intr, xs->timeout)) {
			printf("sc: timeout\n");
			return COMPLETE;
		}
	}

	scdp = &sc_data[intr];

	/* if (DATAIN_PHASE_FINISHED) */
	MachFlushDCache((vm_offset_t)scdp->scd_scaddr, sizeof (struct scsi));
	if (MACH_IS_USPACE(scdp->scd_vaddr))
		panic("sc_scsi_cmd: user address is not supported");
	else if (MACH_IS_CACHED(scdp->scd_vaddr))
		MachFlushDCache(scdp->scd_vaddr, scdp->scd_count);
	else if (MACH_IS_MAPPED(scdp->scd_vaddr))
		MachFlushCache(); /* Flush all caches */

	return COMPLETE;
}

/*
 * Used when interrupt driven I/O isn't allowed, e.g. during boot.
 */
int
sc_poll(intr, count)
	int intr, count;
{
	volatile u_char *int_stat = (void *)INTST1;
	volatile u_char *int_clear = (void *)INTCLR1;

	while (sc_busy(intr)) {
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
	struct scsi *sc_param;
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
	flags = xs->flags;

	if (cold)
		flags |= SCSI_POLL;

	if (sc->inuse[chan]) {
		scb = scb->chain.tqe_next;
		goto start;
	}
	sc->inuse[chan] = 1;

	if (flags & SCSI_RESET)
		printf("SCSI RESET\n");

	sc_param = &sc->scsi_xxx[chan];

	bzero(sc_param, sizeof(struct scsi));
	lun = sc_link->scsipi_scsi.lun;
	sc_param->sc_xs = xs;
	sc_param->sc_identify = MSG_IDENT | sc_disconnect | (lun & IDT_DRMASK);
	sc_param->sc_bytesec = DEV_BSIZE;
	sc_param->sc_lun = lun;

	sc_param->sc_cpoint = xs->data;
	sc_param->sc_ctrnscnt = xs->datalen;

	bcopy(xs->cmd, &sc_param->sc_opcode, xs->cmdlen);

	/* make va->pa mapping table for dma */
	if (xs->datalen > 0) {
		int pages, offset;
		int i, pn;
		u_int va;

		bzero(&sc->sc_map[chan], sizeof(struct sc_map));

		va = (u_int)xs->data;

		offset = va & PGOFSET;
		pages = (offset + xs->datalen + NBPG -1 ) >> PGSHIFT;
		if (pages >= NSCMAP)
			panic("sc_map: Too many pages");

		for (i = 0; i < pages; i++) {
			pn = kvtophys((vm_offset_t)va) >> PGSHIFT;
			sc->sc_map[chan].mp_addr[i] = pn;
			va += NBPG;
		}

		sc->sc_map[chan].mp_offset = offset;
		sc->sc_map[chan].mp_pages = pages;
		sc_param->sc_map = &sc->sc_map[chan];
	}

	if (flags & SCSI_POLL)
		ie &= ~SCSI_INTEN;
	else
		ie |= SCSI_INTEN;

	timeout(cxd1185_timeout, scb, hz * 10);

	sc_go(chan, sc_param, ie);

	untimeout(cxd1185_timeout, scb);

	nextscb = scb->chain.tqe_next;

	TAILQ_REMOVE(&sc->ready_list, scb, chain);
	free_scb(sc, scb, flags);

	scb = nextscb;

	goto start;
}


void
sc_go(chan, sc_param, ie)
	int chan;
	struct scsi *sc_param;
	int ie;
{
	register struct sc_data *scdp;

	scdp = &sc_data[chan];

	if (sc_param->sc_cpoint)
		scdp->scd_vaddr = (vm_offset_t)sc_param->sc_cpoint;
	else
		scdp->scd_vaddr = (vm_offset_t)sc_param->sc_param;
	scdp->scd_scaddr = sc_param;
	scdp->scd_count = sc_param->sc_ctrnscnt;
	sc_param->sc_cpoint = (u_char *)ipc_phys(scdp->scd_vaddr);

	sc_send(chan, ie, sc_param);
}

/*static*/ void scop_rsense();

void
sc_done(sc_param)
	struct scsi *sc_param;
{
	struct scsipi_xfer *xs = sc_param->sc_xs;
	struct scsipi_link *sc_link = xs->sc_link;
	struct sc_softc *sc = sc_link->adapter_softc;

	xs->flags |= ITSDONE;
	xs->resid = 0;
	xs->status = 0;

	if (sc_param->sc_istatus != INST_EP) {
		if (! cold)
			printf("SC(i): [istatus=0x%x, tstatus=0x%x]\n",
				sc_param->sc_istatus, sc_param->sc_tstatus);
		xs->error = XS_DRIVER_STUFFUP;
	}

	switch (sc_param->sc_tstatus) {

	case TGST_GOOD:
		break;

	case TGST_CC:
		break;		/* XXX */
#if 0
		chan = sc_link->scsipi_scsi.target;
		lun = sc_link->scsipi_scsi.lun;
		scop_rsense(chan, sc_param, lun, SCSI_INTDIS, 18, 0);
		if (sc_param->sc_tstatus != TGST_GOOD) {
			printf("SC(t2): [istatus=0x%x, tstatus=0x%x]\n",
				sc_param->sc_istatus, sc_param->sc_tstatus);
		}
#endif

	default:
		printf("SC(t): [istatus=0x%x, tstatus=0x%x]\n",
			sc_param->sc_istatus, sc_param->sc_tstatus);
		break;
	}

	scsipi_done(xs);
	sc->inuse[sc_link->scsipi_scsi.target] = 0;

	sc_sched(sc);
}

int
sc_intr(sc)
	struct sc_softc *sc;
{
	return scintr();
}


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
	sc_param->sc_identify = MSG_IDENT | sc_disconnect | (lun & IDT_DRMASK);
	sc_param->sc_bytesec = DEV_BSIZE;
	sc_param->sc_lun = lun;

	sc_param->sc_cpoint = (u_char *)param;
	sc_param->sc_ctrnscnt = count;

	/* sc_cdb */
	sc_param->sc_opcode = SCOP_RSENSE;
	sc_param->sc_count = count;

	sc_go(intr, sc_param, ie);
}

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
