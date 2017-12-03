/*	$NetBSD: mesh.c,v 1.35.12.2 2017/12/03 11:36:25 jdolecek Exp $	*/

/*-
 * Copyright (c) 2000	Tsubai Masanari.
 * Copyright (c) 1999	Internet Research Institute, Inc.
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
 *	This product includes software developed by
 *	Internet Research Institute, Inc.
 * 4. The name of the author may not be used to endorse or promote products
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mesh.c,v 1.35.12.2 2017/12/03 11:36:25 jdolecek Exp $");

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/scsipi/scsi_message.h>

#include <dev/ofw/openfirm.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/pio.h>

#include <macppc/dev/dbdma.h>
#include <macppc/dev/meshreg.h>

#ifdef MESH_DEBUG
# define DPRINTF printf
#else
# define DPRINTF while (0) printf
#endif

#define T_SYNCMODE 0x01		/* target uses sync mode */
#define T_SYNCNEGO 0x02		/* sync negotiation done */

struct mesh_tinfo {
	int flags;
	int period;
	int offset;
};

/* scb flags */
#define MESH_POLL	0x01
#define MESH_CHECK	0x02
#define MESH_READ	0x80

struct mesh_scb {
	TAILQ_ENTRY(mesh_scb) chain;
	int flags;
	struct scsipi_xfer *xs;
	struct scsipi_generic cmd;
	int cmdlen;
	int target;			/* target SCSI ID */
	int resid;
	vaddr_t daddr;
	vsize_t dlen;
	int status;
};

/* sc_flags value */
#define MESH_DMA_ACTIVE	0x01

struct mesh_softc {
	device_t sc_dev;		/* us as a device */
	struct scsipi_channel sc_channel;
	struct scsipi_adapter sc_adapter;

	u_char *sc_reg;			/* MESH base address */
	dbdma_regmap_t *sc_dmareg;	/* DMA register address */
	dbdma_command_t *sc_dmacmd;	/* DMA command area */

	int sc_flags;
	int sc_cfflags;			/* copy of config flags */
	int sc_meshid;			/* MESH version */
	int sc_minsync;			/* minimum sync period */
	int sc_irq;
	int sc_freq;			/* SCSI bus frequency in MHz */
	int sc_id;			/* our SCSI ID */
	struct mesh_tinfo sc_tinfo[8];	/* target information */

	int sc_nextstate;
	int sc_prevphase;
	struct mesh_scb *sc_nexus;	/* current command */

	int sc_msgout;
	int sc_imsglen;
	u_char sc_imsg[16];
	u_char sc_omsg[16];

	TAILQ_HEAD(, mesh_scb) free_scb;
	TAILQ_HEAD(, mesh_scb) ready_scb;
	struct mesh_scb sc_scb[16];
};

/* mesh_msgout() values */
#define SEND_REJECT	1
#define SEND_IDENTIFY	2
#define SEND_SDTR	4

static inline int mesh_read_reg(struct mesh_softc *, int);
static inline void mesh_set_reg(struct mesh_softc *, int, int);

static int mesh_match(device_t, cfdata_t, void *);
static void mesh_attach(device_t, device_t, void *);
static bool mesh_shutdown(device_t, int);
static int mesh_intr(void *);
static void mesh_error(struct mesh_softc *, struct mesh_scb *, int, int);
static void mesh_select(struct mesh_softc *, struct mesh_scb *);
static void mesh_identify(struct mesh_softc *, struct mesh_scb *);
static void mesh_command(struct mesh_softc *, struct mesh_scb *);
static void mesh_dma_setup(struct mesh_softc *, struct mesh_scb *);
static void mesh_dataio(struct mesh_softc *, struct mesh_scb *);
static void mesh_status(struct mesh_softc *, struct mesh_scb *);
static void mesh_msgin(struct mesh_softc *, struct mesh_scb *);
static void mesh_msgout(struct mesh_softc *, int);
static void mesh_bus_reset(struct mesh_softc *);
static void mesh_reset(struct mesh_softc *);
static int mesh_stp(struct mesh_softc *, int);
static void mesh_setsync(struct mesh_softc *, struct mesh_tinfo *);
static struct mesh_scb *mesh_get_scb(struct mesh_softc *);
static void mesh_free_scb(struct mesh_softc *, struct mesh_scb *);
static void mesh_scsi_request(struct scsipi_channel *,
				scsipi_adapter_req_t, void *);
static void mesh_sched(struct mesh_softc *);
static int mesh_poll(struct mesh_softc *, struct scsipi_xfer *);
static void mesh_done(struct mesh_softc *, struct mesh_scb *);
static void mesh_timeout(void *);
static void mesh_minphys(struct buf *);


#define MESH_DATAOUT	0
#define MESH_DATAIN	MESH_STATUS0_IO
#define MESH_COMMAND	MESH_STATUS0_CD
#define MESH_STATUS	(MESH_STATUS0_CD | MESH_STATUS0_IO)
#define MESH_MSGOUT	(MESH_STATUS0_MSG | MESH_STATUS0_CD)
#define MESH_MSGIN	(MESH_STATUS0_MSG | MESH_STATUS0_CD | MESH_STATUS0_IO)

#define MESH_SELECTING	8
#define MESH_IDENTIFY	9
#define MESH_COMPLETE	10
#define MESH_BUSFREE	11
#define MESH_UNKNOWN	-1

#define MESH_PHASE_MASK	(MESH_STATUS0_MSG | MESH_STATUS0_CD | MESH_STATUS0_IO)

CFATTACH_DECL_NEW(mesh, sizeof(struct mesh_softc),
    mesh_match, mesh_attach, NULL, NULL);

int
mesh_match(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;
	char compat[32];

	if (strcmp(ca->ca_name, "mesh") == 0)
		return 1;

	memset(compat, 0, sizeof(compat));
	OF_getprop(ca->ca_node, "compatible", compat, sizeof(compat));
	if (strcmp(compat, "chrp,mesh0") == 0)
		return 1;

	return 0;
}

void
mesh_attach(device_t parent, device_t self, void *aux)
{
	struct mesh_softc *sc = device_private(self);
	struct confargs *ca = aux;
	int i;
	u_int *reg;

	sc->sc_dev = self;
	reg = ca->ca_reg;
	reg[0] += ca->ca_baseaddr;
	reg[2] += ca->ca_baseaddr;
	sc->sc_reg = mapiodev(reg[0], reg[1], false);
	sc->sc_irq = ca->ca_intr[0];
	sc->sc_dmareg = mapiodev(reg[2], reg[3], false);

	sc->sc_cfflags = device_cfdata(self)->cf_flags;
	sc->sc_meshid = mesh_read_reg(sc, MESH_MESH_ID) & 0x1f;
#if 0
	if (sc->sc_meshid != (MESH_SIGNATURE & 0x1f) {
		aprint_error(": unknown MESH ID (0x%x)\n", sc->sc_meshid);
		return;
	}
#endif
	if (OF_getprop(ca->ca_node, "clock-frequency", &sc->sc_freq, 4) != 4) {
		aprint_error(": cannot get clock-frequency\n");
		return;
	}
	sc->sc_freq /= 1000000;	/* in MHz */
	sc->sc_minsync = 25;	/* maximum sync rate = 10MB/sec */
	sc->sc_id = 7;

	TAILQ_INIT(&sc->free_scb);
	TAILQ_INIT(&sc->ready_scb);
	for (i = 0; i < sizeof(sc->sc_scb)/sizeof(sc->sc_scb[0]); i++)
		TAILQ_INSERT_TAIL(&sc->free_scb, &sc->sc_scb[i], chain);

	sc->sc_dmacmd = dbdma_alloc(sizeof(dbdma_command_t) * 20, NULL);

	mesh_reset(sc);
	mesh_bus_reset(sc);

	aprint_normal(" irq %d: %dMHz, SCSI ID %d\n",
		sc->sc_irq, sc->sc_freq, sc->sc_id);

	sc->sc_adapter.adapt_dev = self;
	sc->sc_adapter.adapt_nchannels = 1;
	sc->sc_adapter.adapt_openings = 7;
	sc->sc_adapter.adapt_max_periph = 1;
	sc->sc_adapter.adapt_ioctl = NULL;
	sc->sc_adapter.adapt_minphys = mesh_minphys;
	sc->sc_adapter.adapt_request = mesh_scsi_request;

	sc->sc_channel.chan_adapter = &sc->sc_adapter;
	sc->sc_channel.chan_bustype = &scsi_bustype;
	sc->sc_channel.chan_channel = 0;
	sc->sc_channel.chan_ntargets = 8;
	sc->sc_channel.chan_nluns = 8;
	sc->sc_channel.chan_id = sc->sc_id;

	config_found(self, &sc->sc_channel, scsiprint);

	intr_establish(sc->sc_irq, IST_EDGE, IPL_BIO, mesh_intr, sc);

	/* Reset SCSI bus when halt. */
	if (!pmf_device_register1(self, NULL, NULL, mesh_shutdown))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

#define MESH_SET_XFER(sc, count) do {					\
	mesh_set_reg(sc, MESH_XFER_COUNT0, count);			\
	mesh_set_reg(sc, MESH_XFER_COUNT1, count >> 8);			\
} while (0)

#define MESH_GET_XFER(sc) ((mesh_read_reg(sc, MESH_XFER_COUNT1) << 8) |	\
			   mesh_read_reg(sc, MESH_XFER_COUNT0))

int
mesh_read_reg(struct mesh_softc *sc, int reg)
{
	return in8(sc->sc_reg + reg);
}

void
mesh_set_reg(struct mesh_softc *sc, int reg, int val)
{
	out8(sc->sc_reg + reg, val);
}

bool
mesh_shutdown(device_t self, int howto)
{
	struct mesh_softc *sc;

	sc = device_private(self);

	/* Set to async mode. */
	mesh_set_reg(sc, MESH_SYNC_PARAM, 2);
	mesh_bus_reset(sc);

	return true;
}

#ifdef MESH_DEBUG
static char scsi_phase[][8] = {
	"DATAOUT",
	"DATAIN",
	"COMMAND",
	"STATUS",
	"",
	"",
	"MSGOUT",
	"MSGIN"
};
#endif

int
mesh_intr(void *arg)
{
	struct mesh_softc *sc = arg;
	struct mesh_scb *scb;
	int fifocnt;
	u_char intr, exception, error, status0;

	intr = mesh_read_reg(sc, MESH_INTERRUPT);
	if (intr == 0) {
		DPRINTF("%s: stray interrupt\n", device_xname(sc->sc_dev));
		return 0;
	}

	exception = mesh_read_reg(sc, MESH_EXCEPTION);
	error = mesh_read_reg(sc, MESH_ERROR);
	status0 = mesh_read_reg(sc, MESH_BUS_STATUS0);
	(void)mesh_read_reg(sc, MESH_BUS_STATUS1);

	/* clear interrupt */
	mesh_set_reg(sc, MESH_INTERRUPT, intr);

#ifdef MESH_DEBUG
{
	char buf1[64], buf2[64];

	snprintb(buf1, sizeof buf1, MESH_STATUS0_BITMASK, status0);
	snprintb(buf2, sizeof buf2, MESH_EXC_BITMASK, exception);
	printf("mesh_intr status0 = %s (%s), exc = %s\n",
	    buf1, scsi_phase[status0 & 7], buf2);
}
#endif

	scb = sc->sc_nexus;
	if (scb == NULL) {
		DPRINTF("%s: NULL nexus\n", device_xname(sc->sc_dev));
		return 1;
	}

	if (intr & MESH_INTR_CMDDONE) {
		if (sc->sc_flags & MESH_DMA_ACTIVE) {
			dbdma_stop(sc->sc_dmareg);

			sc->sc_flags &= ~MESH_DMA_ACTIVE;
			scb->resid = MESH_GET_XFER(sc);

			fifocnt = mesh_read_reg(sc, MESH_FIFO_COUNT);
			if (fifocnt != 0) {
				if (scb->flags & MESH_READ) {
					char *cp;

					cp = (char *)scb->daddr + scb->dlen
						- fifocnt;
					DPRINTF("fifocnt = %d, resid = %d\n",
						fifocnt, scb->resid);
					while (fifocnt > 0) {
						*cp++ = mesh_read_reg(sc,
								MESH_FIFO);
						fifocnt--;
					}
				} else {
					mesh_set_reg(sc, MESH_SEQUENCE,
							MESH_CMD_FLUSH_FIFO);
				}
			} else {
				/* Clear all interrupts */
				mesh_set_reg(sc, MESH_INTERRUPT, 7);
			}
		}
	}

	if (intr & MESH_INTR_ERROR) {
		printf("%s: error %02x %02x\n",
			device_xname(sc->sc_dev), error, exception);
		mesh_error(sc, scb, error, 0);
		return 1;
	}

	if (intr & MESH_INTR_EXCEPTION) {
		/* selection timeout */
		if (exception & MESH_EXC_SELTO) {
			mesh_error(sc, scb, 0, exception);
			return 1;
		}

		/* phase mismatch */
		if (exception & MESH_EXC_PHASEMM) {
			DPRINTF("%s: PHASE MISMATCH; nextstate = %d -> ",
				device_xname(sc->sc_dev), sc->sc_nextstate);
			sc->sc_nextstate = status0 & MESH_PHASE_MASK;

			DPRINTF("%d, resid = %d\n",
				sc->sc_nextstate, scb->resid);
		}
	}

	if (sc->sc_nextstate == MESH_UNKNOWN)
		sc->sc_nextstate = status0 & MESH_PHASE_MASK;

	switch (sc->sc_nextstate) {

	case MESH_IDENTIFY:
		mesh_identify(sc, scb);
		break;
	case MESH_COMMAND:
		mesh_command(sc, scb);
		break;
	case MESH_DATAIN:
	case MESH_DATAOUT:
		mesh_dataio(sc, scb);
		break;
	case MESH_STATUS:
		mesh_status(sc, scb);
		break;
	case MESH_MSGIN:
		mesh_msgin(sc, scb);
		break;
	case MESH_COMPLETE:
		mesh_done(sc, scb);
		break;

	default:
		printf("%s: unknown state (%d)\n", device_xname(sc->sc_dev),
		    sc->sc_nextstate);
		scb->xs->error = XS_DRIVER_STUFFUP;
		mesh_done(sc, scb);
	}

	return 1;
}

void
mesh_error(struct mesh_softc *sc, struct mesh_scb *scb, int error, int exception)
{
	if (error & MESH_ERR_SCSI_RESET) {
		printf("%s: SCSI RESET\n", device_xname(sc->sc_dev));

		/* Wait until the RST signal is deasserted. */
		while (mesh_read_reg(sc, MESH_BUS_STATUS1) & MESH_STATUS1_RST);
		mesh_reset(sc);
		return;
	}

	if (error & MESH_ERR_PARITY_ERR0) {
		printf("%s: parity error\n", device_xname(sc->sc_dev));
		scb->xs->error = XS_DRIVER_STUFFUP;
	}

	if (error & MESH_ERR_DISCONNECT) {
		printf("%s: unexpected disconnect\n", device_xname(sc->sc_dev));
		if (sc->sc_nextstate != MESH_COMPLETE)
			scb->xs->error = XS_DRIVER_STUFFUP;
	}

	if (exception & MESH_EXC_SELTO) {
		/* XXX should reset bus here? */
		scb->xs->error = XS_SELTIMEOUT;
	}

	mesh_done(sc, scb);
}

void
mesh_select(struct mesh_softc *sc, struct mesh_scb *scb)
{
	struct mesh_tinfo *ti = &sc->sc_tinfo[scb->target];
	int timeout;

	DPRINTF("mesh_select\n");

	mesh_setsync(sc, ti);
	MESH_SET_XFER(sc, 0);

	/* arbitration */

	/*
	 * MESH mistakenly asserts TARGET ID bit along with its own ID bit
	 * in arbitration phase (like selection).  So we should load
	 * initiator ID to DestID register temporarily.
	 */
	mesh_set_reg(sc, MESH_DEST_ID, sc->sc_id);
	mesh_set_reg(sc, MESH_INTR_MASK, 0);	/* disable intr. */
	mesh_set_reg(sc, MESH_SEQUENCE, MESH_CMD_ARBITRATE);

	while (mesh_read_reg(sc, MESH_INTERRUPT) == 0);
	mesh_set_reg(sc, MESH_INTERRUPT, 1);
	mesh_set_reg(sc, MESH_INTR_MASK, 7);

	/* selection */
	mesh_set_reg(sc, MESH_DEST_ID, scb->target);
	mesh_set_reg(sc, MESH_SEQUENCE, MESH_CMD_SELECT | MESH_SEQ_ATN);

	sc->sc_prevphase = MESH_SELECTING;
	sc->sc_nextstate = MESH_IDENTIFY;

	timeout = mstohz(scb->xs->timeout);
	if (timeout == 0)
		timeout = 1;

	callout_reset(&scb->xs->xs_callout, timeout, mesh_timeout, scb);
}

void
mesh_identify(struct mesh_softc *sc, struct mesh_scb *scb)
{
	struct mesh_tinfo *ti = &sc->sc_tinfo[scb->target];

	DPRINTF("mesh_identify\n");
	mesh_set_reg(sc, MESH_SEQUENCE, MESH_CMD_FLUSH_FIFO);

	if ((ti->flags & T_SYNCNEGO) == 0) {
		ti->period = sc->sc_minsync;
		ti->offset = 15;
		mesh_msgout(sc, SEND_IDENTIFY | SEND_SDTR);
		sc->sc_nextstate = MESH_MSGIN;
	} else {
		mesh_msgout(sc, SEND_IDENTIFY);
		sc->sc_nextstate = MESH_COMMAND;
	}
}

void
mesh_command(struct mesh_softc *sc, struct mesh_scb *scb)
{
	int i;
	char *cmdp;

#ifdef MESH_DEBUG
	printf("mesh_command cdb = %02x", scb->cmd.opcode);
	for (i = 0; i < 5; i++)
		printf(" %02x", scb->cmd.bytes[i]);
	printf("\n");
#endif

	mesh_set_reg(sc, MESH_SEQUENCE, MESH_CMD_FLUSH_FIFO);

	MESH_SET_XFER(sc, scb->cmdlen);
	mesh_set_reg(sc, MESH_SEQUENCE, MESH_CMD_COMMAND);

	cmdp = (char *)&scb->cmd;
	for (i = 0; i < scb->cmdlen; i++)
		mesh_set_reg(sc, MESH_FIFO, *cmdp++);

	if (scb->resid == 0)
		sc->sc_nextstate = MESH_STATUS;		/* no data xfer */
	else
		sc->sc_nextstate = MESH_DATAIN;
}

void
mesh_dma_setup(struct mesh_softc *sc, struct mesh_scb *scb)
{
	int datain = scb->flags & MESH_READ;
	dbdma_command_t *cmdp;
	u_int cmd;
	vaddr_t va;
	int count, offset;

	cmdp = sc->sc_dmacmd;
	cmd = datain ? DBDMA_CMD_IN_MORE : DBDMA_CMD_OUT_MORE;

	count = scb->dlen;

	if (count / PAGE_SIZE > 32)
		panic("mesh: transfer size >= 128k");

	va = scb->daddr;
	offset = va & PGOFSET;

	/* if va is not page-aligned, setup the first page */
	if (offset != 0) {
		int rest = PAGE_SIZE - offset;	/* the rest in the page */

		if (count > rest) {		/* if continues to next page */
			DBDMA_BUILD(cmdp, cmd, 0, rest, vtophys(va),
				DBDMA_INT_NEVER, DBDMA_WAIT_NEVER,
				DBDMA_BRANCH_NEVER);
			count -= rest;
			va += rest;
			cmdp++;
		}
	}

	/* now va is page-aligned */
	while (count > PAGE_SIZE) {
		DBDMA_BUILD(cmdp, cmd, 0, PAGE_SIZE, vtophys(va),
			DBDMA_INT_NEVER, DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);
		count -= PAGE_SIZE;
		va += PAGE_SIZE;
		cmdp++;
	}

	/* the last page (count <= PAGE_SIZE here) */
	cmd = datain ? DBDMA_CMD_IN_LAST : DBDMA_CMD_OUT_LAST;
	DBDMA_BUILD(cmdp, cmd , 0, count, vtophys(va),
		DBDMA_INT_NEVER, DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);
	cmdp++;

	DBDMA_BUILD(cmdp, DBDMA_CMD_STOP, 0, 0, 0,
		DBDMA_INT_NEVER, DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);
}

void
mesh_dataio(struct mesh_softc *sc, struct mesh_scb *scb)
{
	DPRINTF("mesh_dataio len = %ld (%s)\n", scb->dlen,
		scb->flags & MESH_READ ? "read" : "write");

	mesh_dma_setup(sc, scb);

	if (scb->dlen == 65536)
		MESH_SET_XFER(sc, 0);	/* TC = 0 means 64KB transfer */
	else
		MESH_SET_XFER(sc, scb->dlen);

	if (scb->flags & MESH_READ)
		mesh_set_reg(sc, MESH_SEQUENCE, MESH_CMD_DATAIN | MESH_SEQ_DMA);
	else
		mesh_set_reg(sc, MESH_SEQUENCE, MESH_CMD_DATAOUT | MESH_SEQ_DMA);
	dbdma_start(sc->sc_dmareg, sc->sc_dmacmd);
	sc->sc_flags |= MESH_DMA_ACTIVE;
	sc->sc_nextstate = MESH_STATUS;
}

void
mesh_status(struct mesh_softc *sc, struct mesh_scb *scb)
{
	if (mesh_read_reg(sc, MESH_FIFO_COUNT) == 0) {	/* XXX cheat */
		DPRINTF("mesh_status(0)\n");
		MESH_SET_XFER(sc, 1);
		mesh_set_reg(sc, MESH_SEQUENCE, MESH_CMD_STATUS);
		sc->sc_nextstate = MESH_STATUS;
		return;
	}

	scb->status = mesh_read_reg(sc, MESH_FIFO);
	DPRINTF("mesh_status(1): status = 0x%x\n", scb->status);
	if (mesh_read_reg(sc, MESH_FIFO_COUNT) != 0)
		DPRINTF("FIFO_COUNT=%d\n", mesh_read_reg(sc, MESH_FIFO_COUNT));

	mesh_set_reg(sc, MESH_SEQUENCE, MESH_CMD_FLUSH_FIFO);
	MESH_SET_XFER(sc, 1);
	mesh_set_reg(sc, MESH_SEQUENCE, MESH_CMD_MSGIN);

	sc->sc_nextstate = MESH_MSGIN;
}

void
mesh_msgin(struct mesh_softc *sc, struct mesh_scb *scb)
{
	DPRINTF("mesh_msgin\n");

	if (mesh_read_reg(sc, MESH_FIFO_COUNT) == 0) {	/* XXX cheat */
		MESH_SET_XFER(sc, 1);
		mesh_set_reg(sc, MESH_SEQUENCE, MESH_CMD_MSGIN);
		sc->sc_imsglen = 0;
		sc->sc_nextstate = MESH_MSGIN;
		return;
	}

	sc->sc_imsg[sc->sc_imsglen++] = mesh_read_reg(sc, MESH_FIFO);

	if (sc->sc_imsglen == 1 && MSG_IS1BYTE(sc->sc_imsg[0]))
		goto gotit;
	if (sc->sc_imsglen == 2 && MSG_IS2BYTE(sc->sc_imsg[0]))
		goto gotit;
	if (sc->sc_imsglen >= 3 && MSG_ISEXTENDED(sc->sc_imsg[0]) &&
	    sc->sc_imsglen == sc->sc_imsg[1] + 2)
		goto gotit;

	sc->sc_nextstate = MESH_MSGIN;
	MESH_SET_XFER(sc, 1);
	mesh_set_reg(sc, MESH_SEQUENCE, MESH_CMD_MSGIN);
	return;

gotit:
#ifdef MESH_DEBUG
	printf("msgin:");
	for (i = 0; i < sc->sc_imsglen; i++)
		printf(" 0x%02x", sc->sc_imsg[i]);
	printf("\n");
#endif

	switch (sc->sc_imsg[0]) {
	case MSG_CMDCOMPLETE:
		mesh_set_reg(sc, MESH_SEQUENCE, MESH_CMD_BUSFREE);
		sc->sc_nextstate = MESH_COMPLETE;
		sc->sc_imsglen = 0;
		return;

	case MSG_MESSAGE_REJECT:
		if (sc->sc_msgout & SEND_SDTR) {
			printf("SDTR rejected\n");
			printf("using async mode\n");
			sc->sc_tinfo[scb->target].period = 0;
			sc->sc_tinfo[scb->target].offset = 0;
			mesh_setsync(sc, &sc->sc_tinfo[scb->target]);
			break;
		}
		break;

	case MSG_NOOP:
		break;

	case MSG_EXTENDED:
		goto extended_msg;

	default:
		scsipi_printaddr(scb->xs->xs_periph);
		printf("unrecognized MESSAGE(0x%02x); sending REJECT\n",
			sc->sc_imsg[0]);

	reject:
		mesh_msgout(sc, SEND_REJECT);
		return;
	}
	goto done;

extended_msg:
	/* process an extended message */
	switch (sc->sc_imsg[2]) {
	case MSG_EXT_SDTR:
	  {
		struct mesh_tinfo *ti = &sc->sc_tinfo[scb->target];
		int period = sc->sc_imsg[3];
		int offset = sc->sc_imsg[4];
		int r = 250 / period;
		int s = (100*250) / period - 100 * r;

		if (period < sc->sc_minsync) {
			ti->period = sc->sc_minsync;
			ti->offset = 15;
			mesh_msgout(sc, SEND_SDTR);
			return;
		}
		scsipi_printaddr(scb->xs->xs_periph);
		/* XXX if (offset != 0) ... */
		printf("max sync rate %d.%02dMb/s\n", r, s);
		ti->period = period;
		ti->offset = offset;
		ti->flags |= T_SYNCNEGO;
		ti->flags |= T_SYNCMODE;
		mesh_setsync(sc, ti);
		goto done;
	  }
	default:
		printf("%s target %d: rejecting extended message 0x%x\n",
			device_xname(sc->sc_dev), scb->target, sc->sc_imsg[0]);
		goto reject;
	}

done:
	sc->sc_imsglen = 0;
	sc->sc_nextstate = MESH_UNKNOWN;

	mesh_set_reg(sc, MESH_SEQUENCE, MESH_CMD_BUSFREE); /* XXX really? */
}

void
mesh_msgout(struct mesh_softc *sc, int msg)
{
	struct mesh_scb *scb = sc->sc_nexus;
	struct mesh_tinfo *ti;
	int lun, len, i;

	DPRINTF("mesh_msgout: sending");

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
		ti = &sc->sc_tinfo[scb->target];
		sc->sc_omsg[len++] = MSG_EXTENDED;
		sc->sc_omsg[len++] = 3;
		sc->sc_omsg[len++] = MSG_EXT_SDTR;
		sc->sc_omsg[len++] = ti->period;
		sc->sc_omsg[len++] = ti->offset;
	}
	DPRINTF("\n");

	MESH_SET_XFER(sc, len);
	if (len == 1) {
		mesh_set_reg(sc, MESH_SEQUENCE, MESH_CMD_MSGOUT);
		mesh_set_reg(sc, MESH_FIFO, sc->sc_omsg[0]);
	} else {
		mesh_set_reg(sc, MESH_SEQUENCE, MESH_CMD_MSGOUT | MESH_SEQ_ATN);

		for (i = 0; i < len - 1; i++)
			mesh_set_reg(sc, MESH_FIFO, sc->sc_omsg[i]);

		/* Wait for the FIFO empty... */
		while (mesh_read_reg(sc, MESH_FIFO_COUNT) > 0);

		/* ...then write the last byte. */
		mesh_set_reg(sc, MESH_FIFO, sc->sc_omsg[i]);
	}
	sc->sc_nextstate = MESH_UNKNOWN;
}

void
mesh_bus_reset(struct mesh_softc *sc)
{
	DPRINTF("mesh_bus_reset\n");

	/* Disable interrupts. */
	mesh_set_reg(sc, MESH_INTR_MASK, 0);

	/* Assert RST line. */
	mesh_set_reg(sc, MESH_BUS_STATUS1, MESH_STATUS1_RST);
	delay(50);
	mesh_set_reg(sc, MESH_BUS_STATUS1, 0);

	mesh_reset(sc);
}

void
mesh_reset(struct mesh_softc *sc)
{
	int i;

	DPRINTF("mesh_reset\n");

	/* Reset DMA first. */
	dbdma_reset(sc->sc_dmareg);

	/* Disable interrupts. */
	mesh_set_reg(sc, MESH_INTR_MASK, 0);

	mesh_set_reg(sc, MESH_SEQUENCE, MESH_CMD_RESET_MESH);
	delay(1);

	/* Wait for reset done. */
	while (mesh_read_reg(sc, MESH_INTERRUPT) == 0);

	/* Clear interrupts */
	mesh_set_reg(sc, MESH_INTERRUPT, 0x7);

	/* Set SCSI ID */
	mesh_set_reg(sc, MESH_SOURCE_ID, sc->sc_id);

	/* Set to async mode by default. */
	mesh_set_reg(sc, MESH_SYNC_PARAM, 2);

	/* Set selection timeout to 250ms. */
	mesh_set_reg(sc, MESH_SEL_TIMEOUT, 250 * sc->sc_freq / 500);

	/* Enable parity check. */
	mesh_set_reg(sc, MESH_SEQUENCE, MESH_CMD_ENABLE_PARITY);

	/* Enable all interrupts. */
	mesh_set_reg(sc, MESH_INTR_MASK, 0x7);

	for (i = 0; i < 7; i++) {
		struct mesh_tinfo *ti = &sc->sc_tinfo[i];

		ti->flags = 0;
		ti->period = ti->offset = 0;
		if (sc->sc_cfflags & (0x100 << i))
			ti->flags |= T_SYNCNEGO;
	}
	sc->sc_nexus = NULL;
}

int
mesh_stp(struct mesh_softc *sc, int v)
{
	/*
	 * stp(v) = 5 * clock_period         (v == 0)
	 *        = (v + 2) * 2 clock_period (v > 0)
	 */

	if (v == 0)
		return 5 * 250 / sc->sc_freq;
	else
		return (v + 2) * 2 * 250 / sc->sc_freq;
}

void
mesh_setsync(struct mesh_softc *sc, struct mesh_tinfo *ti)
{
	int period = ti->period;
	int offset = ti->offset;
	int v;

	if ((ti->flags & T_SYNCMODE) == 0)
		offset = 0;

	if (offset == 0) {	/* async mode */
		mesh_set_reg(sc, MESH_SYNC_PARAM, 2);
		return;
	}

	v = period * sc->sc_freq / 250 / 2 - 2;
	if (v < 0)
		v = 0;
	if (mesh_stp(sc, v) < period)
		v++;
	if (v > 15)
		v = 15;
	mesh_set_reg(sc, MESH_SYNC_PARAM, (offset << 4) | v);
}

struct mesh_scb *
mesh_get_scb(struct mesh_softc *sc)
{
	struct mesh_scb *scb;
	int s;

	s = splbio();
	if ((scb = sc->free_scb.tqh_first) != NULL)
		TAILQ_REMOVE(&sc->free_scb, scb, chain);
	splx(s);

	return scb;
}

void
mesh_free_scb(struct mesh_softc *sc, struct mesh_scb *scb)
{
	int s;

	s = splbio();
	TAILQ_INSERT_HEAD(&sc->free_scb, scb, chain);
	splx(s);
}

void
mesh_scsi_request(struct scsipi_channel *chan, scsipi_adapter_req_t req, void *arg)
{
	struct scsipi_xfer *xs;
	struct scsipi_periph *periph;
	struct mesh_softc *sc = device_private(chan->chan_adapter->adapt_dev);
	struct mesh_scb *scb;
	u_int flags;
	int s;

	switch (req) {
	case ADAPTER_REQ_RUN_XFER:
		xs = arg;
		periph = xs->xs_periph;
		flags = xs->xs_control;


		if ((scb = mesh_get_scb(sc)) == NULL) {
			xs->error = XS_RESOURCE_SHORTAGE;
			scsipi_done(xs);
			return;
		}
		scb->xs = xs;
		scb->flags = 0;
		scb->status = 0;
		scb->daddr = (vaddr_t)xs->data;
		scb->dlen = xs->datalen;
		scb->resid = xs->datalen;
		memcpy(&scb->cmd, xs->cmd, xs->cmdlen);
		scb->cmdlen = xs->cmdlen;
		scb->target = periph->periph_target;
		sc->sc_imsglen = 0;	/* XXX ? */

#ifdef MESH_DEBUG
{
		int i;
		printf("mesh_scsi_cmd: target = %d, cdb = %02x",
		       scb->target, scb->cmd.opcode);
		for (i = 0; i < 5; i++)
			printf(" %02x", scb->cmd.bytes[i]);
		printf("\n");
}
#endif

		if (flags & XS_CTL_POLL)
			scb->flags |= MESH_POLL;
#if 0
		if (flags & XS_CTL_DATA_OUT)
			scb->flags &= ~MESH_READ;
#endif
		if (flags & XS_CTL_DATA_IN)
			scb->flags |= MESH_READ;

		s = splbio();

		TAILQ_INSERT_TAIL(&sc->ready_scb, scb, chain);

		if (sc->sc_nexus == NULL)	/* IDLE */
			mesh_sched(sc);

		splx(s);

		if ((flags & XS_CTL_POLL) == 0)
			return;

		if (mesh_poll(sc, xs)) {
			printf("%s: timeout\n", device_xname(sc->sc_dev));
			if (mesh_poll(sc, xs))
				printf("%s: timeout again\n",
				    device_xname(sc->sc_dev));
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

void
mesh_sched(struct mesh_softc *sc)
{
	struct mesh_scb *scb;

	scb = sc->ready_scb.tqh_first;
start:
	if (scb == NULL)
		return;

	if (sc->sc_nexus == NULL) {
		TAILQ_REMOVE(&sc->ready_scb, scb, chain);
		sc->sc_nexus = scb;
		mesh_select(sc, scb);
		return;
	}

	scb = scb->chain.tqe_next;
	goto start;
}

int
mesh_poll(struct mesh_softc *sc, struct scsipi_xfer *xs)
{
	int count = xs->timeout;

	while (count) {
		if (mesh_read_reg(sc, MESH_INTERRUPT))
			mesh_intr(sc);

		if (xs->xs_status & XS_STS_DONE)
			return 0;
		delay(1000);
		count--;
	};
	return 1;
}

void
mesh_done(struct mesh_softc *sc, struct mesh_scb *scb)
{
	struct scsipi_xfer *xs = scb->xs;

	DPRINTF("mesh_done\n");

	sc->sc_nextstate = MESH_BUSFREE;
	sc->sc_nexus = NULL;

	callout_stop(&scb->xs->xs_callout);

	if (scb->status == SCSI_BUSY) {
		xs->error = XS_BUSY;
		printf("Target busy\n");
	}

	xs->status = scb->status;
	xs->resid = scb->resid;
	if (scb->status == SCSI_CHECK) {
		xs->error = XS_BUSY;
	}

	mesh_set_reg(sc, MESH_SYNC_PARAM, 2);

	if ((xs->xs_control & XS_CTL_POLL) == 0)
		mesh_sched(sc);

	scsipi_done(xs);
	mesh_free_scb(sc, scb);
}

void
mesh_timeout(void *arg)
{
	struct mesh_scb *scb = arg;
	struct mesh_softc *sc =
	    device_private(scb->xs->xs_periph->periph_channel->chan_adapter->adapt_dev);
	int s;
	int status0, status1;
	int intr, error, exception, imsk;

	printf("%s: timeout state %d\n", device_xname(sc->sc_dev),
	    sc->sc_nextstate);

	intr = mesh_read_reg(sc, MESH_INTERRUPT);
	imsk = mesh_read_reg(sc, MESH_INTR_MASK);
	exception = mesh_read_reg(sc, MESH_EXCEPTION);
	error = mesh_read_reg(sc, MESH_ERROR);
	status0 = mesh_read_reg(sc, MESH_BUS_STATUS0);
	status1 = mesh_read_reg(sc, MESH_BUS_STATUS1);

	printf("%s: intr/msk %02x/%02x, exc %02x, err %02x, st0/1 %02x/%02x\n",
		device_xname(sc->sc_dev),
		intr, imsk, exception, error, status0, status1);

	s = splbio();
	if (sc->sc_flags & MESH_DMA_ACTIVE) {
		printf("mesh: resetting DMA\n");
		dbdma_reset(sc->sc_dmareg);
	}
	scb->xs->error = XS_TIMEOUT;

	mesh_set_reg(sc, MESH_SEQUENCE, MESH_CMD_BUSFREE);
	sc->sc_nextstate = MESH_COMPLETE;

	splx(s);
}

void
mesh_minphys(struct buf *bp)
{
	if (bp->b_bcount > 64*1024)
		bp->b_bcount = 64*1024;

	minphys(bp);
}
