/*	$NetBSD: mha.c,v 1.42.2.1 2007/03/12 05:51:39 rmind Exp $	*/

/*-
 * Copyright (c) 1996-1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum, Masaru Oki, Takumi Nakamura, Masanobu Saitoh and
 * Minoura Makoto.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*-
 * Copyright (c) 1994 Jarle Greipsland
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mha.c,v 1.42.2.1 2007/03/12 05:51:39 rmind Exp $");

#include "opt_ddb.h"

/* Synchronous data transfers? */
#define SPC_USE_SYNCHRONOUS	0
#define SPC_SYNC_REQ_ACK_OFS 	8

/* Default DMA mode? */
#define MHA_DMA_LIMIT_XFER	1
#define MHA_DMA_BURST_XFER	1
#define MHA_DMA_SHORT_BUS_CYCLE	1

#define MHA_DMA_DATAIN	(0 | (MHA_DMA_LIMIT_XFER << 1)		\
			   | (MHA_DMA_BURST_XFER << 2)		\
			   | (MHA_DMA_SHORT_BUS_CYCLE << 3))
#define MHA_DMA_DATAOUT	(1 | (MHA_DMA_LIMIT_XFER << 1)		\
			   | (MHA_DMA_BURST_XFER << 2)		\
			   | (MHA_DMA_SHORT_BUS_CYCLE << 3))

/* Include debug functions?  At the end of this file there are a bunch of
 * functions that will print out various information regarding queued SCSI
 * commands, driver state and chip contents.  You can call them from the
 * kernel debugger.  If you set SPC_DEBUG to 0 they are not included (the
 * kernel uses less memory) but you lose the debugging facilities.
 */
#define SPC_DEBUG		0

/* End of customizable parameters */

/*
 * MB86601A SCSI Protocol Controller (SPC) routines for MANKAI Mach-2
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/queue.h>

#include <machine/bus.h>

#include <dev/scsipi/scsi_spc.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsi_message.h>
#include <dev/scsipi/scsiconf.h>

#include <x68k/x68k/iodevice.h>
#include <x68k/dev/mb86601reg.h>
#include <x68k/dev/mhavar.h>
#include <x68k/dev/intiovar.h>
#include <x68k/dev/scsiromvar.h>

#if 0
#define WAIT {if (sc->sc_pc[2]) {printf("[W_%d", __LINE__); while (sc->sc_pc[2] & 0x40);printf("]");}}
#else
#define WAIT {while (sc->sc_pc[2] & 0x40);}
#endif

#define SSR	(sc->sc_pc[2])
#define	SS_IREQUEST	0x80
#define	SS_BUSY		0x40
#define	SS_DREG_FULL	0x02

#define	NSR	(sc->sc_pc[3])

#define	SIR	(sc->sc_pc[4])

#define	CMR	(sc->sc_pc[5])
#define	CMD_SEL_AND_CMD	0x00
#define	CMD_SELECT	0x09
#define	CMD_SET_ATN	0x0a
#define	CMD_RESET_ATN	0x0b
#define	CMD_RESET_ACK	0x0d
#define	CMD_SEND_FROM_MPU	0x10
#define	CMD_SEND_FROM_DMA	0x11
#define	CMD_RECEIVE_TO_MPU	0x12
#define	CMD_RECEIVE_TO_DMA	0x13
#define	CMD_RECEIVE_MSG	0x1a
#define	CMD_RECEIVE_STS	0x1c
#define	CMD_SOFT_RESET	0x40
#define	CMD_SCSI_RESET	0x42
#define	CMD_SET_UP_REG	0x43

#define	SCR	(sc->sc_pc[11])

#define	TMR	(sc->sc_pc[12])
#define	TM_SYNC		0x80
#define	TM_ASYNC	0x00

#define	WAR	(sc->sc_pc[15])
#define	WA_MCSBUFWIN	0x00
#define	WA_UPMWIN	0x80
#define	WA_INITWIN	0xc0

#define	MBR	(sc->sc_pc[15])

#define ISCSR	(sc->sc_ps[2])

#define	CCR	(sc->sc_pcx[0])
#define	OIR	(sc->sc_pcx[1])
#define	AMR	(sc->sc_pcx[2])
#define	SMR	(sc->sc_pcx[3])
#define	SRR	(sc->sc_pcx[4])
#define	STR	(sc->sc_pcx[5])
#define	RTR	(sc->sc_pcx[6])
#define	ATR	(sc->sc_pcx[7])
#define	PER	(sc->sc_pcx[8])
#define	IER	(sc->sc_pcx[9])
#define	IE_ALL	0xBF

#define	GLR	(sc->sc_pcx[10])
#define	DMR	(sc->sc_pcx[11])
#define	IMR	(sc->sc_pcx[12])

#ifndef DDB
#define	Debugger() panic("should call debugger here (mha.c)")
#endif /* ! DDB */


#if SPC_DEBUG
#define SPC_SHOWACBS	0x01
#define SPC_SHOWINTS	0x02
#define SPC_SHOWCMDS	0x04
#define SPC_SHOWMISC	0x08
#define SPC_SHOWTRAC	0x10
#define SPC_SHOWSTART	0x20
#define SPC_SHOWPHASE	0x40
#define SPC_SHOWDMA	0x80
#define SPC_SHOWCCMDS	0x100
#define SPC_SHOWMSGS	0x200
#define SPC_DOBREAK	0x400

int mha_debug =
#if 0
0x7FF;
#else
SPC_SHOWSTART|SPC_SHOWTRAC;
#endif


#define SPC_ACBS(str)  do {if (mha_debug & SPC_SHOWACBS) printf str;} while (0)
#define SPC_MISC(str)  do {if (mha_debug & SPC_SHOWMISC) printf str;} while (0)
#define SPC_INTS(str)  do {if (mha_debug & SPC_SHOWINTS) printf str;} while (0)
#define SPC_TRACE(str) do {if (mha_debug & SPC_SHOWTRAC) printf str;} while (0)
#define SPC_CMDS(str)  do {if (mha_debug & SPC_SHOWCMDS) printf str;} while (0)
#define SPC_START(str) do {if (mha_debug & SPC_SHOWSTART) printf str;}while (0)
#define SPC_PHASE(str) do {if (mha_debug & SPC_SHOWPHASE) printf str;}while (0)
#define SPC_DMA(str)   do {if (mha_debug & SPC_SHOWDMA) printf str;}while (0)
#define SPC_MSGS(str)  do {if (mha_debug & SPC_SHOWMSGS) printf str;}while (0)
#define	SPC_BREAK()    do {if ((mha_debug & SPC_DOBREAK) != 0) Debugger();} while (0)
#define	SPC_ASSERT(x)  do {if (x) {} else {printf("%s at line %d: assertion failed\n", sc->sc_dev.dv_xname, __LINE__); Debugger();}} while (0)
#else
#define SPC_ACBS(str)
#define SPC_MISC(str)
#define SPC_INTS(str)
#define SPC_TRACE(str)
#define SPC_CMDS(str)
#define SPC_START(str)
#define SPC_PHASE(str)
#define SPC_DMA(str)
#define SPC_MSGS(str)
#define	SPC_BREAK()
#define	SPC_ASSERT(x)
#endif

int	mhamatch(struct device *, struct cfdata *, void *);
void	mhaattach(struct device *, struct device *, void *);
void	mhaselect(struct mha_softc *, u_char, u_char, u_char *, u_char);
void	mha_scsi_reset(struct mha_softc *);
void	mha_reset(struct mha_softc *);
void	mha_free_acb(struct mha_softc *, struct acb *, int);
void	mha_sense(struct mha_softc *, struct acb *);
void	mha_msgin(struct mha_softc *);
void	mha_msgout(struct mha_softc *);
int	mha_dataout_pio(struct mha_softc *, u_char *, int);
int	mha_datain_pio(struct mha_softc *, u_char *, int);
int	mha_dataout(struct mha_softc *, u_char *, int);
int	mha_datain(struct mha_softc *, u_char *, int);
void	mha_abort(struct mha_softc *, struct acb *);
void 	mha_init(struct mha_softc *);
void	mha_scsi_request(struct scsipi_channel *, scsipi_adapter_req_t, void *);
void	mha_poll(struct mha_softc *, struct acb *);
void	mha_sched(struct mha_softc *);
void	mha_done(struct mha_softc *, struct acb *);
int	mhaintr(void *);
void	mha_timeout(void *);
void	mha_minphys(struct buf *);
void	mha_dequeue(struct mha_softc *, struct acb *);
inline void	mha_setsync(struct mha_softc *, struct spc_tinfo *);
#if SPC_DEBUG
void	mha_print_acb(struct acb *);
void	mha_show_scsi_cmd(struct acb *);
void	mha_print_active_acb(void);
void	mha_dump_driver(struct mha_softc *);
#endif

static int mha_dataio_dma(int, int, struct mha_softc *, u_char *, int);

CFATTACH_DECL(mha, sizeof(struct mha_softc),
    mhamatch, mhaattach, NULL, NULL);

extern struct cfdriver mha_cd;

/*
 * returns non-zero value if a controller is found.
 */
int
mhamatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct intio_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_bst;
	bus_space_handle_t ioh;

	ia->ia_size=0x20;
	if (ia->ia_addr != 0xea0000)
		return 0;

	if (intio_map_allocate_region(device_parent(parent), ia,
				      INTIO_MAP_TESTONLY) < 0) /* FAKE */
		return 0;

	if (bus_space_map(iot, ia->ia_addr, 0x20, BUS_SPACE_MAP_SHIFTED,
			  &ioh) < 0)
		return 0;
	if (!badaddr(INTIO_ADDR(ia->ia_addr + 0)))
		return 0;
	bus_space_unmap(iot, ioh, 0x20);

	return 1;
}

/*
 */

struct mha_softc *tmpsc;

void
mhaattach(struct device *parent, struct device *self, void *aux)
{
	struct mha_softc *sc = (void *)self;
	struct intio_attach_args *ia = aux;

	tmpsc = sc;	/* XXX */

	printf(": Mankai Mach-2 Fast SCSI Host Adaptor\n");

	SPC_TRACE(("mhaattach  "));
	sc->sc_state = SPC_INIT;
	sc->sc_iobase = INTIO_ADDR(ia->ia_addr + 0x80); /* XXX */
	intio_map_allocate_region(device_parent(parent), ia, INTIO_MAP_ALLOCATE);
				/* XXX: FAKE  */
	sc->sc_dmat = ia->ia_dmat;

	sc->sc_pc = (volatile u_char *)sc->sc_iobase;
	sc->sc_ps = (volatile u_short *)sc->sc_iobase;
	sc->sc_pcx = &sc->sc_pc[0x10];

	sc->sc_id = IODEVbase->io_sram[0x70] & 0x7; /* XXX */

	intio_intr_establish(ia->ia_intr, "mha", mhaintr, sc);

	mha_init(sc);	/* Init chip and driver */

	mha_scsi_reset(sc);	/* XXX: some devices need this. */

	sc->sc_phase  = BUSFREE_PHASE;

	/*
	 * Fill in the adapter.
	 */
	sc->sc_adapter.adapt_dev = &sc->sc_dev;
	sc->sc_adapter.adapt_nchannels = 1;
	sc->sc_adapter.adapt_openings = 7;
	sc->sc_adapter.adapt_max_periph = 1;
	sc->sc_adapter.adapt_ioctl = NULL;
	sc->sc_adapter.adapt_minphys = mha_minphys;
	sc->sc_adapter.adapt_request = mha_scsi_request;

	sc->sc_channel.chan_adapter = &sc->sc_adapter;
	sc->sc_channel.chan_bustype = &scsi_bustype;
	sc->sc_channel.chan_channel = 0;
	sc->sc_channel.chan_ntargets = 8;
	sc->sc_channel.chan_nluns = 8;
	sc->sc_channel.chan_id = sc->sc_id;

	sc->sc_spcinitialized = 0;
	WAR = WA_INITWIN;
#if 1
	CCR = 0x14;
	OIR = sc->sc_id;
	AMR = 0x00;
	SMR = 0x00;
	SRR = 0x00;
	STR = 0x20;
	RTR = 0x40;
	ATR = 0x01;
	PER = 0xc9;
#endif
	IER = IE_ALL;	/* すべての割り込みを許可 */
#if 1
	GLR = 0x00;
	DMR = 0x30;
	IMR = 0x00;
#endif
	WAR = WA_MCSBUFWIN;

	/* drop off */
	while(SSR & SS_IREQUEST) {
		(void) ISCSR;
	}

	CMR = CMD_SET_UP_REG;	/* setup reg cmd. */

	SPC_TRACE(("waiting for intr..."));
	while (!(SSR & SS_IREQUEST))
	  delay(10);
	mhaintr(sc);

	tmpsc = NULL;

	config_found(self, &sc->sc_channel, scsiprint);
}

#if 0
void
mha_reset(struct mha_softc *sc)
{
	u_short	dummy;
printf("reset...");
	CMR = CMD_SOFT_RESET;
	__asm volatile ("nop");	/* XXX wait (4clk in 20 MHz) ??? */
	dummy = sc->sc_ps[-1];
	dummy = sc->sc_ps[-1];
	dummy = sc->sc_ps[-1];
	dummy = sc->sc_ps[-1];
	__asm volatile ("nop");
	CMR = CMD_SOFT_RESET;
	sc->sc_spcinitialized = 0;
	CMR = CMD_SET_UP_REG;	/* setup reg cmd. */
	while(!sc->sc_spcinitialized);

	sc->sc_id = IODEVbase->io_sram[0x70] & 0x7; /* XXX */
printf("done.\n");
}
#endif

/*
 * Pull the SCSI RST line for 500us.
 */
void
mha_scsi_reset(struct mha_softc *sc)
{

	CMR = CMD_SCSI_RESET;	/* SCSI RESET */
	while (!(SSR&SS_IREQUEST))
		delay(10);
}

/*
 * Initialize mha SCSI driver.
 */
void
mha_init(struct mha_softc *sc)
{
	struct acb *acb;
	int r;

	if (sc->sc_state == SPC_INIT) {
		/* First time through; initialize. */
		TAILQ_INIT(&sc->ready_list);
		TAILQ_INIT(&sc->nexus_list);
		TAILQ_INIT(&sc->free_list);
		sc->sc_nexus = NULL;
		acb = sc->sc_acb;
		memset(acb, 0, sizeof(sc->sc_acb));
		for (r = 0; r < sizeof(sc->sc_acb) / sizeof(*acb); r++) {
			TAILQ_INSERT_TAIL(&sc->free_list, acb, chain);
			acb++;
		}
		memset(&sc->sc_tinfo, 0, sizeof(sc->sc_tinfo));

		r = bus_dmamem_alloc(sc->sc_dmat, MAXBSIZE, 0, 0,
				     sc->sc_dmaseg, 1, &sc->sc_ndmasegs,
				     BUS_DMA_NOWAIT);
		if (r)
			panic("mha_init: cannot allocate DMA memory");
		if (sc->sc_ndmasegs != 1)
			panic("mha_init: number of segment > 1??");
		r = bus_dmamem_map(sc->sc_dmat, sc->sc_dmaseg, sc->sc_ndmasegs,
				   MAXBSIZE, &sc->sc_dmabuf, BUS_DMA_NOWAIT);
		if (r)
			panic("mha_init: cannot map DMA memory");
		r = bus_dmamap_create(sc->sc_dmat, MAXBSIZE, 1,
				      MAXBSIZE, 0, BUS_DMA_NOWAIT,
				      &sc->sc_dmamap);
		if (r)
			panic("mha_init: cannot create dmamap structure");
		r = bus_dmamap_load(sc->sc_dmat, sc->sc_dmamap,
				    sc->sc_dmabuf, MAXBSIZE, NULL,
				    BUS_DMA_NOWAIT);
		if (r)
			panic("mha_init: cannot load DMA buffer into dmamap");
		sc->sc_p = 0;
	} else {
		/* Cancel any active commands. */
		sc->sc_flags |= SPC_ABORTING;
		sc->sc_state = SPC_IDLE;
		if ((acb = sc->sc_nexus) != NULL) {
			acb->xs->error = XS_DRIVER_STUFFUP;
			mha_done(sc, acb);
		}
		while ((acb = sc->nexus_list.tqh_first) != NULL) {
			acb->xs->error = XS_DRIVER_STUFFUP;
			mha_done(sc, acb);
		}
	}

	sc->sc_phase = sc->sc_prevphase = INVALID_PHASE;
	for (r = 0; r < 8; r++) {
		struct spc_tinfo *ti = &sc->sc_tinfo[r];

		ti->flags = 0;
#if SPC_USE_SYNCHRONOUS
		ti->flags |= T_SYNCMODE;
		ti->period = sc->sc_minsync;
		ti->offset = SPC_SYNC_REQ_ACK_OFS;
#else
		ti->period = ti->offset = 0;
#endif
		ti->width = 0;
	}

	sc->sc_state = SPC_IDLE;
}

void
mha_free_acb(struct mha_softc *sc, struct acb *acb, int flags)
{
	int s;

	s = splbio();

	acb->flags = 0;
	TAILQ_INSERT_HEAD(&sc->free_list, acb, chain);

	/*
	 * If there were none, wake anybody waiting for one to come free,
	 * starting with queued entries.
	 */
	if (acb->chain.tqe_next == 0)
		wakeup(&sc->free_list);

	splx(s);
}

/*
 * DRIVER FUNCTIONS CALLABLE FROM HIGHER LEVEL DRIVERS
 */

/*
 * Expected sequence:
 * 1) Command inserted into ready list
 * 2) Command selected for execution
 * 3) Command won arbitration and has selected target device
 * 4) Send message out (identify message, eventually also sync.negotiations)
 * 5) Send command
 * 5a) Receive disconnect message, disconnect.
 * 5b) Reselected by target
 * 5c) Receive identify message from target.
 * 6) Send or receive data
 * 7) Receive status
 * 8) Receive message (command complete etc.)
 * 9) If status == SCSI_CHECK construct a synthetic request sense SCSI cmd.
 *    Repeat 2-8 (no disconnects please...)
 */

/*
 * Start a selection.  This is used by mha_sched() to select an idle target,
 * and by mha_done() to immediately reselect a target to get sense information.
 */
void
mhaselect(struct mha_softc *sc, u_char target, u_char lun, u_char *cmd,
    u_char clen)
{
	int i;
	int s;

	s = splbio();	/* XXX */

	SPC_TRACE(("[mhaselect(t%d,l%d,cmd:%x)] ", target, lun, *(u_char *)cmd));

	/* CDB を SPC の MCS REG にセットする */
	/* Now the command into the FIFO */
	WAIT;
#if 1
	SPC_MISC(("[cmd:"));
	for (i = 0; i < clen; i++)
	  {
	    unsigned c = cmd[i];
	    if (i == 1)
	      c |= lun << 5;
	    SPC_MISC((" %02x", c));
	    sc->sc_pcx[i] = c;
	  }
	SPC_MISC(("], target=%d\n", target));
#else
	memcpy(sc->sc_pcx, cmd, clen);
#endif
	if (NSR & 0x80)
		panic("scsistart: already selected...");
	sc->sc_phase  = COMMAND_PHASE;

	/* new state ASP_SELECTING */
	sc->sc_state = SPC_SELECTING;

	SIR = target;
#if 0
	CMR = CMD_SELECT;
#else
	CMR = CMD_SEL_AND_CMD;	/* select & cmd */
#endif
	splx(s);
}

#if 0
int
mha_reselect(struct mha_softc *sc, u_char message)
{
	u_char selid, target, lun;
	struct acb *acb;
	struct scsipi_periph *periph;
	struct spc_tinfo *ti;

	/*
	 * The SCSI chip made a snapshot of the data bus while the reselection
	 * was being negotiated.  This enables us to determine which target did
	 * the reselect.
	 */
	selid = sc->sc_selid & ~(1 << sc->sc_id);
	if (selid & (selid - 1)) {
		printf("%s: reselect with invalid selid %02x; sending DEVICE RESET\n",
		    sc->sc_dev.dv_xname, selid);
		SPC_BREAK();
		goto reset;
	}

	/*
	 * Search wait queue for disconnected cmd
	 * The list should be short, so I haven't bothered with
	 * any more sophisticated structures than a simple
	 * singly linked list.
	 */
	target = ffs(selid) - 1;
	lun = message & 0x07;
	for (acb = sc->nexus_list.tqh_first; acb != NULL;
	     acb = acb->chain.tqe_next) {
		periph = acb->xs->xs_periph;
		if (periph->periph_target == target &&
		    periph->periph_lun == lun)
			break;
	}
	if (acb == NULL) {
		printf("%s: reselect from target %d lun %d with no nexus; sending ABORT\n",
		    sc->sc_dev.dv_xname, target, lun);
		SPC_BREAK();
		goto abort;
	}

	/* Make this nexus active again. */
	TAILQ_REMOVE(&sc->nexus_list, acb, chain);
	sc->sc_state = SPC_HASNEXUS;
	sc->sc_nexus = acb;
	ti = &sc->sc_tinfo[target];
	ti->lubusy |= (1 << lun);
	mha_setsync(sc, ti);

	if (acb->flags & ACB_RESET)
		mha_sched_msgout(sc, SEND_DEV_RESET);
	else if (acb->flags & ACB_ABORTED)
		mha_sched_msgout(sc, SEND_ABORT);

	/* Do an implicit RESTORE POINTERS. */
	sc->sc_dp = acb->daddr;
	sc->sc_dleft = acb->dleft;
	sc->sc_cp = (u_char *)&acb->cmd;
	sc->sc_cleft = acb->clen;

	return (0);

reset:
	mha_sched_msgout(sc, SEND_DEV_RESET);
	return (1);

abort:
	mha_sched_msgout(sc, SEND_ABORT);
	return (1);
}
#endif
/*
 * Start a SCSI-command
 * This function is called by the higher level SCSI-driver to queue/run
 * SCSI-commands.
 */
void
mha_scsi_request(struct scsipi_channel *chan, scsipi_adapter_req_t req,
    void *arg)
{
	struct scsipi_xfer *xs;
	struct scsipi_periph *periph;
	struct mha_softc *sc = (void *)chan->chan_adapter->adapt_dev;
	struct acb *acb;
	int s, flags;

	switch (req) {
	case ADAPTER_REQ_RUN_XFER:
		xs = arg;
		periph = xs->xs_periph;

		SPC_TRACE(("[mha_scsi_cmd] "));
		SPC_CMDS(("[0x%x, %d]->%d ", (int)xs->cmd->opcode, xs->cmdlen,
		    periph->periph_target));

		flags = xs->xs_control;

		/* Get a mha command block */
		s = splbio();
		acb = sc->free_list.tqh_first;
		if (acb) {
			TAILQ_REMOVE(&sc->free_list, acb, chain);
			ACB_SETQ(acb, ACB_QNONE);
		}

		if (acb == NULL) {
			xs->error = XS_RESOURCE_SHORTAGE;
			scsipi_done(xs);
			splx(s);
			return;
		}
		splx(s);

		/* Initialize acb */
		acb->xs = xs;
		memcpy(&acb->cmd, xs->cmd, xs->cmdlen);
		acb->clen = xs->cmdlen;
		acb->daddr = xs->data;
		acb->dleft = xs->datalen;
		acb->stat = 0;

		s = splbio();
		ACB_SETQ(acb, ACB_QREADY);
		TAILQ_INSERT_TAIL(&sc->ready_list, acb, chain);
#if 1
		callout_reset(&acb->xs->xs_callout,
		    mstohz(xs->timeout), mha_timeout, acb);
#endif

		/*
		 * キューの処理中でなければ、スケジューリング開始する
		 */
		if (sc->sc_state == SPC_IDLE)
			mha_sched(sc);

		splx(s);

		if (flags & XS_CTL_POLL) {
			/* Not allowed to use interrupts, use polling instead */
			mha_poll(sc, acb);
		}

		SPC_MISC(("SUCCESSFULLY_QUEUED"));
		return;

	case ADAPTER_REQ_GROW_RESOURCES:
		/* XXX Not supported. */
		return;

	case ADAPTER_REQ_SET_XFER_MODE:
		/* XXX Not supported. */
		return;
	}
}

/*
 * Adjust transfer size in buffer structure
 */
void
mha_minphys(struct buf *bp)
{

	SPC_TRACE(("mha_minphys  "));
	minphys(bp);
}

/*
 * Used when interrupt driven I/O isn't allowed, e.g. during boot.
 */
void
mha_poll(struct mha_softc *sc, struct acb *acb)
{
	struct scsipi_xfer *xs = acb->xs;
	int count = xs->timeout * 100;
	int s;

	s = splbio();

	SPC_TRACE(("[mha_poll] "));

	while (count) {
		/*
		 * If we had interrupts enabled, would we
		 * have got an interrupt?
		 */
		if (SSR & SS_IREQUEST)
			mhaintr(sc);
		if ((xs->xs_status & XS_STS_DONE) != 0)
			break;
		DELAY(10);
#if 1
		if (sc->sc_state == SPC_IDLE) {
			SPC_TRACE(("[mha_poll: rescheduling] "));
			mha_sched(sc);
		}
#endif
		count--;
	}

	if (count == 0) {
		SPC_MISC(("mha_poll: timeout"));
		mha_timeout((void *)acb);
	}
	splx(s);
	scsipi_done(xs);
}

/*
 * LOW LEVEL SCSI UTILITIES
 */

/*
 * Set synchronous transfer offset and period.
 */
inline void
mha_setsync(struct mha_softc *sc, struct spc_tinfo *ti)
{
}

/*
 * Schedule a SCSI operation.  This has now been pulled out of the interrupt
 * handler so that we may call it from mha_scsi_cmd and mha_done.  This may
 * save us an unecessary interrupt just to get things going.  Should only be
 * called when state == SPC_IDLE and at bio pl.
 */
void
mha_sched(struct mha_softc *sc)
{
	struct scsipi_periph *periph;
	struct acb *acb;
	int t;

	SPC_TRACE(("[mha_sched] "));
	if (sc->sc_state != SPC_IDLE)
		panic("mha_sched: not IDLE (state=%d)", sc->sc_state);

	if (sc->sc_flags & SPC_ABORTING)
		return;

	/*
	 * Find first acb in ready queue that is for a target/lunit
	 * combinations that is not busy.
	 */
	for (acb = sc->ready_list.tqh_first; acb ; acb = acb->chain.tqe_next) {
		struct spc_tinfo *ti;
		periph = acb->xs->xs_periph;
		t = periph->periph_target;
		ti = &sc->sc_tinfo[t];
		if (!(ti->lubusy & (1 << periph->periph_lun))) {
			if ((acb->flags & ACB_QBITS) != ACB_QREADY)
				panic("mha: busy entry on ready list");
			TAILQ_REMOVE(&sc->ready_list, acb, chain);
			ACB_SETQ(acb, ACB_QNONE);
			sc->sc_nexus = acb;
			sc->sc_flags = 0;
			sc->sc_prevphase = INVALID_PHASE;
			sc->sc_dp = acb->daddr;
			sc->sc_dleft = acb->dleft;
			ti->lubusy |= (1<<periph->periph_lun);
			mhaselect(sc, t, periph->periph_lun,
				     (u_char *)&acb->cmd, acb->clen);
			break;
		} else {
			SPC_MISC(("%d:%d busy\n",
			    periph->periph_target,
			    periph->periph_lun));
		}
	}
}

/*
 * POST PROCESSING OF SCSI_CMD (usually current)
 */
void
mha_done(struct mha_softc *sc, struct acb *acb)
{
	struct scsipi_xfer *xs = acb->xs;
	struct scsipi_periph *periph = xs->xs_periph;
	struct spc_tinfo *ti = &sc->sc_tinfo[periph->periph_target];

	SPC_TRACE(("[mha_done(error:%x)] ", xs->error));

#if 1
	callout_stop(&acb->xs->xs_callout);
#endif

	/*
	 * Now, if we've come here with no error code, i.e. we've kept the
	 * initial XS_NOERROR, and the status code signals that we should
	 * check sense, we'll need to set up a request sense cmd block and
	 * push the command back into the ready queue *before* any other
	 * commands for this target/lunit, else we lose the sense info.
	 * We don't support chk sense conditions for the request sense cmd.
	 */
	if (xs->error == XS_NOERROR) {
		if ((acb->flags & ACB_ABORTED) != 0) {
			xs->error = XS_TIMEOUT;
		} else if (acb->flags & ACB_CHKSENSE) {
			xs->error = XS_SENSE;
		} else {
			xs->status = acb->stat & ST_MASK;
			switch (xs->status) {
			case SCSI_CHECK:
				xs->resid = acb->dleft;
				/* FALLTHROUGH */
			case SCSI_BUSY:
				xs->error = XS_BUSY;
				break;
			case SCSI_OK:
				xs->resid = acb->dleft;
				break;
			default:
				xs->error = XS_DRIVER_STUFFUP;
#if SPC_DEBUG
				printf("%s: mha_done: bad stat 0x%x\n",
					sc->sc_dev.dv_xname, acb->stat);
#endif
				break;
			}
		}
	}

#if SPC_DEBUG
	if ((mha_debug & SPC_SHOWMISC) != 0) {
		if (xs->resid != 0)
			printf("resid=%d ", xs->resid);
		if (xs->error == XS_SENSE)
			printf("sense=0x%02x\n", xs->sense.scsi_sense.response_code);
		else
			printf("error=%d\n", xs->error);
	}
#endif

	/*
	 * Remove the ACB from whatever queue it's on.
	 */
	switch (acb->flags & ACB_QBITS) {
	case ACB_QNONE:
		if (acb != sc->sc_nexus) {
			panic("%s: floating acb", sc->sc_dev.dv_xname);
		}
		sc->sc_nexus = NULL;
		sc->sc_state = SPC_IDLE;
		ti->lubusy &= ~(1<<periph->periph_lun);
		mha_sched(sc);
		break;
	case ACB_QREADY:
		TAILQ_REMOVE(&sc->ready_list, acb, chain);
		break;
	case ACB_QNEXUS:
		TAILQ_REMOVE(&sc->nexus_list, acb, chain);
		ti->lubusy &= ~(1<<periph->periph_lun);
		break;
	case ACB_QFREE:
		panic("%s: dequeue: busy acb on free list",
			sc->sc_dev.dv_xname);
		break;
	default:
		panic("%s: dequeue: unknown queue %d",
			sc->sc_dev.dv_xname, acb->flags & ACB_QBITS);
	}

	/* Put it on the free list, and clear flags. */
#if 0
	TAILQ_INSERT_HEAD(&sc->free_list, acb, chain);
	acb->flags = ACB_QFREE;
#else
	mha_free_acb(sc, acb, xs->xs_control);
#endif

	ti->cmds++;
	scsipi_done(xs);
}

void
mha_dequeue(struct mha_softc *sc, struct acb *acb)
{

	if (acb->flags & ACB_QNEXUS) {
		TAILQ_REMOVE(&sc->nexus_list, acb, chain);
	} else {
		TAILQ_REMOVE(&sc->ready_list, acb, chain);
	}
}

/*
 * INTERRUPT/PROTOCOL ENGINE
 */

/*
 * Schedule an outgoing message by prioritizing it, and asserting
 * attention on the bus. We can only do this when we are the initiator
 * else there will be an illegal command interrupt.
 */
#define mha_sched_msgout(m) \
	do {				\
		SPC_MISC(("mha_sched_msgout %d ", m)); \
		CMR = CMD_SET_ATN;	\
		sc->sc_msgpriq |= (m);	\
	} while (0)

/*
 * Precondition:
 * The SCSI bus is already in the MSGI phase and there is a message byte
 * on the bus, along with an asserted REQ signal.
 */
void
mha_msgin(struct mha_softc *sc)
{
	int v;

	SPC_TRACE(("[mha_msgin(curmsglen:%d)] ", sc->sc_imlen));

	/*
	 * Prepare for a new message.  A message should (according
	 * to the SCSI standard) be transmitted in one single
	 * MESSAGE_IN_PHASE. If we have been in some other phase,
	 * then this is a new message.
	 */
	if (sc->sc_prevphase != MESSAGE_IN_PHASE) {
		sc->sc_flags &= ~SPC_DROP_MSGI;
		sc->sc_imlen = 0;
	}

	WAIT;

	v = MBR;	/* modified byte */
	v = sc->sc_pcx[0];

	sc->sc_imess[sc->sc_imlen] = v;

	/*
	 * If we're going to reject the message, don't bother storing
	 * the incoming bytes.  But still, we need to ACK them.
	 */

	if ((sc->sc_flags & SPC_DROP_MSGI)) {
		CMR = CMD_SET_ATN;
/*		ESPCMD(sc, ESPCMD_MSGOK);*/
		printf("<dropping msg byte %x>",
			sc->sc_imess[sc->sc_imlen]);
		return;
	}

	if (sc->sc_imlen >= SPC_MAX_MSG_LEN) {
		mha_sched_msgout(SEND_REJECT);
		sc->sc_flags |= SPC_DROP_MSGI;
	} else {
		sc->sc_imlen++;
		/*
		 * This testing is suboptimal, but most
		 * messages will be of the one byte variety, so
		 * it should not effect performance
		 * significantly.
		 */
		if (sc->sc_imlen == 1 && MSG_IS1BYTE(sc->sc_imess[0]))
			goto gotit;
		if (sc->sc_imlen == 2 && MSG_IS2BYTE(sc->sc_imess[0]))
			goto gotit;
		if (sc->sc_imlen >= 3 && MSG_ISEXTENDED(sc->sc_imess[0]) &&
		    sc->sc_imlen == sc->sc_imess[1] + 2)
			goto gotit;
	}
#if 0
	/* Ack what we have so far */
	ESPCMD(sc, ESPCMD_MSGOK);
#endif
	return;

gotit:
	SPC_MSGS(("gotmsg(%x)", sc->sc_imess[0]));
	/*
	 * Now we should have a complete message (1 byte, 2 byte
	 * and moderately long extended messages).  We only handle
	 * extended messages which total length is shorter than
	 * SPC_MAX_MSG_LEN.  Longer messages will be amputated.
	 */
	if (sc->sc_state == SPC_HASNEXUS) {
		struct acb *acb = sc->sc_nexus;
		struct spc_tinfo *ti =
			&sc->sc_tinfo[acb->xs->xs_periph->periph_target];

		switch (sc->sc_imess[0]) {
		case MSG_CMDCOMPLETE:
			SPC_MSGS(("cmdcomplete "));
			if (sc->sc_dleft < 0) {
				struct scsipi_periph *periph = acb->xs->xs_periph;
				printf("mha: %d extra bytes from %d:%d\n",
					-sc->sc_dleft,
					periph->periph_target,
				        periph->periph_lun);
				sc->sc_dleft = 0;
			}
			acb->xs->resid = acb->dleft = sc->sc_dleft;
			sc->sc_flags |= SPC_BUSFREE_OK;
			break;

		case MSG_MESSAGE_REJECT:
#if SPC_DEBUG
			if (mha_debug & SPC_SHOWMSGS)
				printf("%s: our msg rejected by target\n",
					sc->sc_dev.dv_xname);
#endif
#if 1 /* XXX - must remember last message */
			scsipi_printaddr(acb->xs->xs_periph);
			printf("MSG_MESSAGE_REJECT>>");
#endif
			if (sc->sc_flags & SPC_SYNCHNEGO) {
				ti->period = ti->offset = 0;
				sc->sc_flags &= ~SPC_SYNCHNEGO;
				ti->flags &= ~T_NEGOTIATE;
			}
			/* Not all targets understand INITIATOR_DETECTED_ERR */
			if (sc->sc_msgout == SEND_INIT_DET_ERR)
				mha_sched_msgout(SEND_ABORT);
			break;
		case MSG_NOOP:
			SPC_MSGS(("noop "));
			break;
		case MSG_DISCONNECT:
			SPC_MSGS(("disconnect "));
			ti->dconns++;
			sc->sc_flags |= SPC_DISCON;
			sc->sc_flags |= SPC_BUSFREE_OK;
			if ((acb->xs->xs_periph->periph_quirks & PQUIRK_AUTOSAVE) == 0)
				break;
			/*FALLTHROUGH*/
		case MSG_SAVEDATAPOINTER:
			SPC_MSGS(("save datapointer "));
			acb->dleft = sc->sc_dleft;
			acb->daddr = sc->sc_dp;
			break;
		case MSG_RESTOREPOINTERS:
			SPC_MSGS(("restore datapointer "));
			if (!acb) {
				mha_sched_msgout(SEND_ABORT);
				printf("%s: no DATAPOINTERs to restore\n",
				    sc->sc_dev.dv_xname);
				break;
			}
			sc->sc_dp = acb->daddr;
			sc->sc_dleft = acb->dleft;
			break;
		case MSG_PARITY_ERROR:
			printf("%s:target%d: MSG_PARITY_ERROR\n",
				sc->sc_dev.dv_xname,
				acb->xs->xs_periph->periph_target);
			break;
		case MSG_EXTENDED:
			SPC_MSGS(("extended(%x) ", sc->sc_imess[2]));
			switch (sc->sc_imess[2]) {
			case MSG_EXT_SDTR:
				SPC_MSGS(("SDTR period %d, offset %d ",
					sc->sc_imess[3], sc->sc_imess[4]));
				ti->period = sc->sc_imess[3];
				ti->offset = sc->sc_imess[4];
				if (sc->sc_minsync == 0) {
					/* We won't do synch */
					ti->offset = 0;
					mha_sched_msgout(SEND_SDTR);
				} else if (ti->offset == 0) {
					printf("%s:%d: async\n", "mha",
						acb->xs->xs_periph->periph_target);
					ti->offset = 0;
					sc->sc_flags &= ~SPC_SYNCHNEGO;
				} else if (ti->period > 124) {
					printf("%s:%d: async\n", "mha",
						acb->xs->xs_periph->periph_target);
					ti->offset = 0;
					mha_sched_msgout(SEND_SDTR);
				} else {
#if 0
					int p;
					p =  mha_stp2cpb(sc, ti->period);
					ti->period = mha_cpb2stp(sc, p);
#endif

#if SPC_DEBUG
					scsipi_printaddr(acb->xs->xs_periph);
#endif
					if ((sc->sc_flags&SPC_SYNCHNEGO) == 0) {
						/* Target initiated negotiation */
						if (ti->flags & T_SYNCMODE) {
						    ti->flags &= ~T_SYNCMODE;
#if SPC_DEBUG
						    printf("renegotiated ");
#endif
						}
						TMR=TM_ASYNC;
						/* Clamp to our maxima */
						if (ti->period < sc->sc_minsync)
							ti->period = sc->sc_minsync;
						if (ti->offset > 15)
							ti->offset = 15;
						mha_sched_msgout(SEND_SDTR);
					} else {
						/* we are sync */
						sc->sc_flags &= ~SPC_SYNCHNEGO;
						TMR = TM_SYNC;
						ti->flags |= T_SYNCMODE;
					}
				}
				ti->flags &= ~T_NEGOTIATE;
				break;
			default: /* Extended messages we don't handle */
				CMR = CMD_SET_ATN; /* XXX? */
				break;
			}
			break;
		default:
			SPC_MSGS(("ident "));
			/* thanks for that ident... */
			if (!MSG_ISIDENTIFY(sc->sc_imess[0])) {
				SPC_MISC(("unknown "));
printf("%s: unimplemented message: %d\n", sc->sc_dev.dv_xname, sc->sc_imess[0]);
				CMR = CMD_SET_ATN; /* XXX? */
			}
			break;
		}
	} else if (sc->sc_state == SPC_RESELECTED) {
		struct scsipi_periph *periph = NULL;
		struct acb *acb;
		struct spc_tinfo *ti;
		u_char lunit;

		if (MSG_ISIDENTIFY(sc->sc_imess[0])) { 	/* Identify? */
			SPC_MISC(("searching "));
			/*
			 * Search wait queue for disconnected cmd
			 * The list should be short, so I haven't bothered with
			 * any more sophisticated structures than a simple
			 * singly linked list.
			 */
			lunit = sc->sc_imess[0] & 0x07;
			for (acb = sc->nexus_list.tqh_first; acb;
			     acb = acb->chain.tqe_next) {
				periph = acb->xs->xs_periph;
				if (periph->periph_lun == lunit &&
				    sc->sc_selid == (1<<periph->periph_target)) {
					TAILQ_REMOVE(&sc->nexus_list, acb,
					    chain);
					ACB_SETQ(acb, ACB_QNONE);
					break;
				}
			}

			if (!acb) {		/* Invalid reselection! */
				mha_sched_msgout(SEND_ABORT);
				printf("mha: invalid reselect (idbit=0x%2x)\n",
				    sc->sc_selid);
			} else {		/* Reestablish nexus */
				/*
				 * Setup driver data structures and
				 * do an implicit RESTORE POINTERS
				 */
				ti = &sc->sc_tinfo[periph->periph_target];
				sc->sc_nexus = acb;
				sc->sc_dp = acb->daddr;
				sc->sc_dleft = acb->dleft;
				sc->sc_tinfo[periph->periph_target].lubusy
					|= (1<<periph->periph_lun);
				if (ti->flags & T_SYNCMODE) {
					TMR = TM_SYNC;	/* XXX */
				} else {
					TMR = TM_ASYNC;
				}
				SPC_MISC(("... found acb"));
				sc->sc_state = SPC_HASNEXUS;
			}
		} else {
			printf("%s: bogus reselect (no IDENTIFY) %0x2x\n",
			    sc->sc_dev.dv_xname, sc->sc_selid);
			mha_sched_msgout(SEND_DEV_RESET);
		}
	} else { /* Neither SPC_HASNEXUS nor SPC_RESELECTED! */
		printf("%s: unexpected message in; will send DEV_RESET\n",
		    sc->sc_dev.dv_xname);
		mha_sched_msgout(SEND_DEV_RESET);
	}

	/* Ack last message byte */
#if 0
	ESPCMD(sc, ESPCMD_MSGOK);
#endif

	/* Done, reset message pointer. */
	sc->sc_flags &= ~SPC_DROP_MSGI;
	sc->sc_imlen = 0;
}

/*
 * Send the highest priority, scheduled message.
 */
void
mha_msgout(struct mha_softc *sc)
{
#if (SPC_USE_SYNCHRONOUS || SPC_USE_WIDE)
	struct spc_tinfo *ti;
#endif
	int n;

	SPC_TRACE(("mha_msgout  "));

	if (sc->sc_prevphase == MESSAGE_OUT_PHASE) {
		if (sc->sc_omp == sc->sc_omess) {
			/*
			 * This is a retransmission.
			 *
			 * We get here if the target stayed in MESSAGE OUT
			 * phase.  Section 5.1.9.2 of the SCSI 2 spec indicates
			 * that all of the previously transmitted messages must
			 * be sent again, in the same order.  Therefore, we
			 * requeue all the previously transmitted messages, and
			 * start again from the top.  Our simple priority
			 * scheme keeps the messages in the right order.
			 */
			SPC_MISC(("retransmitting  "));
			sc->sc_msgpriq |= sc->sc_msgoutq;
			/*
			 * Set ATN.  If we're just sending a trivial 1-byte
			 * message, we'll clear ATN later on anyway.
			 */
			CMR = CMD_SET_ATN; /* XXX? */
		} else {
			/* This is a continuation of the previous message. */
			n = sc->sc_omp - sc->sc_omess;
			goto nextbyte;
		}
	}

	/* No messages transmitted so far. */
	sc->sc_msgoutq = 0;
	sc->sc_lastmsg = 0;

nextmsg:
	/* Pick up highest priority message. */
	sc->sc_currmsg = sc->sc_msgpriq & -sc->sc_msgpriq;
	sc->sc_msgpriq &= ~sc->sc_currmsg;
	sc->sc_msgoutq |= sc->sc_currmsg;

	/* Build the outgoing message data. */
	switch (sc->sc_currmsg) {
	case SEND_IDENTIFY:
		SPC_ASSERT(sc->sc_nexus != NULL);
		sc->sc_omess[0] =
		    MSG_IDENTIFY(sc->sc_nexus->xs->xs_periph->periph_lun, 1);
		n = 1;
		break;

#if SPC_USE_SYNCHRONOUS
	case SEND_SDTR:
		SPC_ASSERT(sc->sc_nexus != NULL);
		ti = &sc->sc_tinfo[sc->sc_nexus->xs->xs_periph->periph_target];
		sc->sc_omess[4] = MSG_EXTENDED;
		sc->sc_omess[3] = 3;
		sc->sc_omess[2] = MSG_EXT_SDTR;
		sc->sc_omess[1] = ti->period >> 2;
		sc->sc_omess[0] = ti->offset;
		n = 5;
		break;
#endif

#if SPC_USE_WIDE
	case SEND_WDTR:
		SPC_ASSERT(sc->sc_nexus != NULL);
		ti = &sc->sc_tinfo[sc->sc_nexus->xs->xs_periph->periph_target];
		sc->sc_omess[3] = MSG_EXTENDED;
		sc->sc_omess[2] = 2;
		sc->sc_omess[1] = MSG_EXT_WDTR;
		sc->sc_omess[0] = ti->width;
		n = 4;
		break;
#endif

	case SEND_DEV_RESET:
		sc->sc_flags |= SPC_ABORTING;
		sc->sc_omess[0] = MSG_BUS_DEV_RESET;
		n = 1;
		break;

	case SEND_REJECT:
		sc->sc_omess[0] = MSG_MESSAGE_REJECT;
		n = 1;
		break;

	case SEND_PARITY_ERROR:
		sc->sc_omess[0] = MSG_PARITY_ERROR;
		n = 1;
		break;

	case SEND_INIT_DET_ERR:
		sc->sc_omess[0] = MSG_INITIATOR_DET_ERR;
		n = 1;
		break;

	case SEND_ABORT:
		sc->sc_flags |= SPC_ABORTING;
		sc->sc_omess[0] = MSG_ABORT;
		n = 1;
		break;

	default:
		printf("%s: unexpected MESSAGE OUT; sending NOOP\n",
		    sc->sc_dev.dv_xname);
		SPC_BREAK();
		sc->sc_omess[0] = MSG_NOOP;
		n = 1;
		break;
	}
	sc->sc_omp = &sc->sc_omess[n];

nextbyte:
	/* Send message bytes. */
	/* send TRANSFER command. */
	sc->sc_ps[3] = 1;
	sc->sc_ps[4] = n >> 8;
	sc->sc_pc[10] = n;
	sc->sc_ps[-1] = 0x000F;	/* burst */
	__asm volatile ("nop");
	CMR = CMD_SEND_FROM_DMA;	/* send from DMA */
	for (;;) {
		if ((SSR & SS_BUSY) != 0)
			break;
		if (SSR & SS_IREQUEST)
			goto out;
	}
	for (;;) {
#if 0
		for (;;) {
			if ((PSNS & PSNS_REQ) != 0)
				break;
			/* Wait for REQINIT.  XXX Need timeout. */
		}
#endif
		if (SSR & SS_IREQUEST) {
			/*
			 * Target left MESSAGE OUT, possibly to reject
			 * our message.
			 *
			 * If this is the last message being sent, then we
			 * deassert ATN, since either the target is going to
			 * ignore this message, or it's going to ask for a
			 * retransmission via MESSAGE PARITY ERROR (in which
			 * case we reassert ATN anyway).
			 */
#if 0
			if (sc->sc_msgpriq == 0)
				CMR = CMD_RESET_ATN;
#endif
			goto out;
		}

#if 0
		/* Clear ATN before last byte if this is the last message. */
		if (n == 1 && sc->sc_msgpriq == 0)
			CMR = CMD_RESET_ATN;
#endif

		while ((SSR & SS_DREG_FULL) != 0)
			;
		/* Send message byte. */
		sc->sc_pc[0] = *--sc->sc_omp;
		--n;
		/* Keep track of the last message we've sent any bytes of. */
		sc->sc_lastmsg = sc->sc_currmsg;

		if (n == 0)
			break;
	}

	/* We get here only if the entire message has been transmitted. */
	if (sc->sc_msgpriq != 0) {
		/* There are more outgoing messages. */
		goto nextmsg;
	}

	/*
	 * The last message has been transmitted.  We need to remember the last
	 * message transmitted (in case the target switches to MESSAGE IN phase
	 * and sends a MESSAGE REJECT), and the list of messages transmitted
	 * this time around (in case the target stays in MESSAGE OUT phase to
	 * request a retransmit).
	 */

out:
	/* Disable REQ/ACK protocol. */
	return;
}

/***************************************************************
 *
 *	datain/dataout
 *
 */

int
mha_datain_pio(struct mha_softc *sc, u_char *p, int n)
{
	u_short d;
	int a;
	int total_n = n;

	SPC_TRACE(("[mha_datain_pio(%p,%d)", p, n));

	WAIT;
	sc->sc_ps[3] = 1;
	sc->sc_ps[4] = n >> 8;
	sc->sc_pc[10] = n;
	/* 悲しきソフト転送 */
	CMR = CMD_RECEIVE_TO_MPU;
	for (;;) {
		a = SSR;
		if (a & 0x04) {
			d = sc->sc_ps[0];
			*p++ = d >> 8;
			if (--n > 0) {
				*p++ = d;
				--n;
			}
			a = SSR;
		}
		if (a & 0x40)
			continue;
		if (a & 0x80)
			break;
	}
	SPC_TRACE(("...%d resd]", n));
	return total_n - n;
}

int
mha_dataout_pio(struct mha_softc *sc, u_char *p, int n)
{
	u_short d;
	int a;
	int total_n = n;

	SPC_TRACE(("[mha_dataout_pio(%p,%d)", p, n));

	WAIT;
	sc->sc_ps[3] = 1;
	sc->sc_ps[4] = n >> 8;
	sc->sc_pc[10] = n;
	/* 悲しきソフト転送 */
	CMR = CMD_SEND_FROM_MPU;
	for (;;) {
		a = SSR;
		if (a & 0x04) {
			d = *p++ << 8;
			if (--n > 0) {
				d |= *p++;
				--n;
			}
			sc->sc_ps[0] = d;
			a = SSR;
		}
		if (a & 0x40)
			continue;
		if (a & 0x80)
			break;
	}
	SPC_TRACE(("...%d resd]", n));
	return total_n - n;
}

/*
 * dw: DMA word
 * cw: CMR word
 */
static int
mha_dataio_dma(int dw, int cw, struct mha_softc *sc, u_char *p, int n)
{
	char *paddr;

	if (n > MAXBSIZE)
		panic("transfer size exceeds MAXBSIZE");
	if (sc->sc_dmasize > 0)
		panic("DMA request while another DMA transfer is in pregress");

	if (cw == CMD_SEND_FROM_DMA) {
		memcpy(sc->sc_dmabuf, p, n);
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap, 0, n, BUS_DMASYNC_PREWRITE);
	} else {
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap, 0, n, BUS_DMASYNC_PREREAD);
	}
	sc->sc_p = p;
	sc->sc_dmasize = n;

	paddr = (char *)sc->sc_dmaseg[0].ds_addr;
#if MHA_DMA_SHORT_BUS_CYCLE == 1
	if ((*(volatile int *)&IODEVbase->io_sram[0xac]) &
	    (1 << ((paddr_t)paddr >> 19)))
		dw &= ~(1 << 3);
#endif
	sc->sc_pc[0x80 + (((long)paddr >> 16) & 0xFF)] = 0;
	sc->sc_pc[0x180 + (((long)paddr >> 8) & 0xFF)] = 0;
	sc->sc_pc[0x280 + (((long)paddr >> 0) & 0xFF)] = 0;
	WAIT;
	sc->sc_ps[3] = 1;
	sc->sc_ps[4] = n >> 8;
	sc->sc_pc[10] = n;
	/* DMA 転送制御は以下の通り。
	   3 ... short bus cycle
	   2 ... MAXIMUM XFER.
	   1 ... BURST XFER.
	   0 ... R/W */
	sc->sc_ps[-1] = dw;	/* burst */
	__asm volatile ("nop");
	CMR = cw;	/* receive to DMA */
	return n;
}

int
mha_dataout(struct mha_softc *sc, u_char *p, int n)
{
	if (n == 0)
		return n;

	if (n & 1)
		return mha_dataout_pio(sc, p, n);
	return mha_dataio_dma(MHA_DMA_DATAOUT, CMD_SEND_FROM_DMA, sc, p, n);
}

int
mha_datain(struct mha_softc *sc, u_char *p, int n)
{
	 struct acb *acb = sc->sc_nexus;

	 if (n == 0)
		 return n;
	 if (acb->cmd.opcode == SCSI_REQUEST_SENSE || (n & 1))
		 return mha_datain_pio(sc, p, n);
	 return mha_dataio_dma(MHA_DMA_DATAIN, CMD_RECEIVE_TO_DMA, sc, p, n);
}

/*
 * Catch an interrupt from the adaptor
 */
/*
 * This is the workhorse routine of the driver.
 * Deficiencies (for now):
 * 1) always uses programmed I/O
 */
int
mhaintr(void *arg)
{
	struct mha_softc *sc = arg;
#if 0
	u_char ints;
#endif
	struct acb *acb;
	u_char ph;
	u_short r;
	int n;

#if 1	/* XXX called during attach? */
	if (tmpsc != NULL) {
		SPC_MISC(("[%p %p]\n", mha_cd.cd_devs, sc));
		sc = tmpsc;
	} else {
#endif

#if 1	/* XXX */
	}
#endif

#if 0
	/*
	 * 割り込み禁止にする
	 */
	SCTL &= ~SCTL_INTR_ENAB;
#endif

	SPC_TRACE(("[mhaintr]"));

	/*
	 * 全転送が完全に終了するまでループする
	 */
	/*
	 * First check for abnormal conditions, such as reset.
	 */
#if 0
#if 1 /* XXX? */
	while (((ints = SSR) & SS_IREQUEST) == 0)
		delay(1);
	SPC_MISC(("ints = 0x%x  ", ints));
#else /* usually? */
	ints = SSR;
#endif
#endif
	while (SSR & SS_IREQUEST) {
		acb = sc->sc_nexus;
		r = ISCSR;
		SPC_MISC(("[r=0x%x]", r));
		switch (r >> 8) {
		default:
			printf("[addr=%p\n"
			       "result=0x%x\n"
			       "cmd=0x%x\n"
			       "ph=0x%x(ought to be %d)]\n",
			       &ISCSR,
			       r,
			       acb->xs->cmd->opcode,
			       SCR, sc->sc_phase);
			panic("unexpected result.");
		case 0x82:	/* selection timeout */
			SPC_MISC(("selection timeout  "));
			sc->sc_phase = BUSFREE_PHASE;
			SPC_ASSERT(sc->sc_nexus != NULL);
			acb = sc->sc_nexus;
			delay(250);
			acb->xs->error = XS_SELTIMEOUT;
			mha_done(sc, acb);
			continue;	/* XXX ??? msaitoh */
		case 0x60:	/* command completed */
			sc->sc_spcinitialized++;
			if (sc->sc_phase == BUSFREE_PHASE)
				continue;
			ph = SCR;
			if (ph & PSNS_ACK) {
				int s;
				/* ふつーのコマンドが終了したらしい */
				SPC_MISC(("0x60)phase = %x(ought to be %x)\n",
					  ph & PHASE_MASK, sc->sc_phase));
#if 0
/*				switch (sc->sc_phase) {*/
#else
				switch (ph & PHASE_MASK) {
#endif
				case STATUS_PHASE:
					if (sc->sc_state != SPC_HASNEXUS)
						printf("stsin: !SPC_HASNEXUS->(%d)\n",
						       sc->sc_state);
					SPC_ASSERT(sc->sc_nexus != NULL);
					acb = sc->sc_nexus;
					WAIT;
					s = MBR;
					SPC_ASSERT(s == 1);
					acb->stat = sc->sc_pcx[0]; /* XXX */
					SPC_MISC(("stat=0x%02x  ", acb->stat));
					sc->sc_prevphase = STATUS_PHASE;
					break;
				case MESSAGE_IN_PHASE:
					mha_msgin(sc);
					sc->sc_prevphase = MESSAGE_IN_PHASE;
					/* thru */
				case DATA_IN_PHASE:
					if (sc->sc_dmasize == 0)
						break;
					bus_dmamap_sync(sc->sc_dmat,
							sc->sc_dmamap,
							0, sc->sc_dmasize,
							BUS_DMASYNC_POSTREAD);
					memcpy(sc->sc_p, sc->sc_dmabuf,
					       sc->sc_dmasize);
					sc->sc_dmasize = 0;
					break;
				case DATA_OUT_PHASE:
					if (sc->sc_dmasize == 0)
						break;
					bus_dmamap_sync(sc->sc_dmat,
							sc->sc_dmamap,
							0, sc->sc_dmasize,
							BUS_DMASYNC_POSTWRITE);
					sc->sc_dmasize = 0;
					break;
				}
				WAIT;
				CMR = CMD_RESET_ACK;	/* reset ack */
				/*mha_done(sc, acb);	XXX */
				continue;
			} else if (NSR & 0x80) { /* nexus */
#if 1
				if (sc->sc_state == SPC_SELECTING)	/* XXX msaitoh */
					sc->sc_state = SPC_HASNEXUS;
				/* フェーズの決め打ちをする
				   外れたら、initial-phase error(0x54) が
				   返ってくるんで注意したまえ。
				   でもなぜか 0x65 が返ってきたりしてねーか? */
				WAIT;
				if (SSR & SS_IREQUEST)
					continue;
				switch (sc->sc_phase) {
				default:
					panic("見知らぬ phase が来ちまっただよ");
				case MESSAGE_IN_PHASE:
					/* 何もしない */
					continue;
				case STATUS_PHASE:
					sc->sc_phase = MESSAGE_IN_PHASE;
					CMR = CMD_RECEIVE_MSG;	/* receive msg */
					continue;
				case DATA_IN_PHASE:
					sc->sc_prevphase = DATA_IN_PHASE;
					if (sc->sc_dleft == 0) {
						/* 転送データはもうないので
						   ステータスフェーズを期待しよう */
						sc->sc_phase = STATUS_PHASE;
						CMR = CMD_RECEIVE_STS;	/* receive sts */
						continue;
					}
					n = mha_datain(sc, sc->sc_dp,
						       sc->sc_dleft);
					sc->sc_dp += n;
					sc->sc_dleft -= n;
					continue;
				case DATA_OUT_PHASE:
					sc->sc_prevphase = DATA_OUT_PHASE;
					if (sc->sc_dleft == 0) {
						/* 転送データはもうないので
						   ステータスフェーズを期待しよう */
						sc->sc_phase = STATUS_PHASE;
						CMR = CMD_RECEIVE_STS;	/* receive sts */
						continue;
					}
					/* data phase の続きをやろう */
					n = mha_dataout(sc, sc->sc_dp, sc->sc_dleft);
					sc->sc_dp += n;
					sc->sc_dleft -= n;
					continue;
				case COMMAND_PHASE:
					/* 最初は CMD PHASE ということらしい */
					if (acb->dleft) {
						/* データ転送がありうる場合 */
						if (acb->xs->xs_control & XS_CTL_DATA_IN) {
							sc->sc_phase = DATA_IN_PHASE;
							n = mha_datain(sc, sc->sc_dp, sc->sc_dleft);
							sc->sc_dp += n;
							sc->sc_dleft -= n;
						}
						else if (acb->xs->xs_control & XS_CTL_DATA_OUT) {
							sc->sc_phase = DATA_OUT_PHASE;
							n = mha_dataout(sc, sc->sc_dp, sc->sc_dleft);
							sc->sc_dp += n;
							sc->sc_dleft -= n;
						}
						continue;
					}
					else {
						/* データ転送はないらしい?! */
						WAIT;
						sc->sc_phase = STATUS_PHASE;
						CMR = CMD_RECEIVE_STS;	/* receive sts */
						continue;
					}
				}
#endif
			}
			continue;
		case 0x31:	/* disconnected in xfer progress. */
			SPC_MISC(("[0x31]"));
		case 0x70:	/* disconnected. */
			SPC_ASSERT(sc->sc_flags & SPC_BUSFREE_OK);
			sc->sc_phase = BUSFREE_PHASE;
			sc->sc_state = SPC_IDLE;
#if 1
			acb = sc->sc_nexus;
			SPC_ASSERT(sc->sc_nexus != NULL);
			acb->xs->error = XS_NOERROR;
			mha_done(sc, acb);
#else
			TAILQ_INSERT_HEAD(&sc->nexus_list, acb, chain);
			mha_sched(sc);
#endif
			continue;
		case 0x32:	/* phase error in xfer progress. */
			SPC_MISC(("[0x32]"));
#if 0
		case 0x65:	/* invalid command.
				   なぜこんなものが出るのか
				   俺には全く理解できない */
#if 1
			SPC_MISC(("[0x%04x]", r));
#endif
#endif
		case 0x54:	/* initial-phase error. */
			SPC_MISC(("[0x54, ns=%x, ph=%x(ought to be %x)]",
				  NSR,
				  SCR, sc->sc_phase));
			/* thru */
		case 0x71:	/* assert req */
			WAIT;
			if (SSR & 0x40) {
				printf("SPC sts=%2x, r=%04x, ns=%x, ph=%x\n",
				       SSR, r, NSR, SCR);
				WAIT;
			}
			ph = SCR;
			if (sc->sc_state == SPC_SELECTING) {	/* XXX msaitoh */
				sc->sc_state = SPC_HASNEXUS;
			}
			if (ph & 0x80) {
				switch (ph & PHASE_MASK) {
				default:
					printf("phase = %x\n", ph);
					panic("assert req: the phase I don't know!");
				case DATA_IN_PHASE:
					sc->sc_prevphase = DATA_IN_PHASE;
					SPC_MISC(("DATAIN(%d)...", sc->sc_dleft));
					n = mha_datain(sc, sc->sc_dp, sc->sc_dleft);
					sc->sc_dp += n;
					sc->sc_dleft -= n;
					SPC_MISC(("done\n"));
					continue;
				case DATA_OUT_PHASE:
					sc->sc_prevphase = DATA_OUT_PHASE;
					SPC_MISC(("DATAOUT\n"));
					n = mha_dataout(sc, sc->sc_dp, sc->sc_dleft);
					sc->sc_dp += n;
					sc->sc_dleft -= n;
					continue;
				case STATUS_PHASE:
					sc->sc_phase = STATUS_PHASE;
					SPC_MISC(("[RECV_STS]"));
					WAIT;
					CMR = CMD_RECEIVE_STS;	/* receive sts */
					continue;
				case MESSAGE_IN_PHASE:
					sc->sc_phase = MESSAGE_IN_PHASE;
					WAIT;
					CMR = CMD_RECEIVE_MSG;
					continue;
				}
			}
			continue;
		}
	}

	return 1;
}

void
mha_abort(struct mha_softc *sc, struct acb *acb)
{
	acb->flags |= ACB_ABORTED;

	if (acb == sc->sc_nexus) {
		/*
		 * If we're still selecting, the message will be scheduled
		 * after selection is complete.
		 */
		if (sc->sc_state == SPC_HASNEXUS) {
			sc->sc_flags |= SPC_ABORTING;
			mha_sched_msgout(SEND_ABORT);
		}
	} else {
		if (sc->sc_state == SPC_IDLE)
			mha_sched(sc);
	}
}

void
mha_timeout(void *arg)
{
	struct acb *acb = (struct acb *)arg;
	struct scsipi_xfer *xs = acb->xs;
	struct scsipi_periph *periph = xs->xs_periph;
	struct mha_softc *sc =
	    (void *)periph->periph_channel->chan_adapter->adapt_dev;
	int s;

	s = splbio();

	scsipi_printaddr(periph);
	printf("%s: timed out [acb %p (flags 0x%x, dleft %x, stat %x)], "
	       "<state %d, nexus %p, phase(c %x, p %x), resid %x, msg(q %x,o %x) >",
		sc->sc_dev.dv_xname,
		acb, acb->flags, acb->dleft, acb->stat,
		sc->sc_state, sc->sc_nexus, sc->sc_phase, sc->sc_prevphase,
		sc->sc_dleft, sc->sc_msgpriq, sc->sc_msgout
		);
	printf("[%04x %02x]\n", sc->sc_ps[1], SCR);
	panic("timeout, ouch!");

	if (acb->flags & ACB_ABORTED) {
		/* abort timed out */
		printf(" AGAIN\n");
#if 0
		mha_init(sc, 1); /* XXX 1?*/
#endif
	} else {
		/* abort the operation that has timed out */
		printf("\n");
		xs->error = XS_TIMEOUT;
		mha_abort(sc, acb);
	}

	splx(s);
}

#if SPC_DEBUG
/*
 * The following functions are mostly used for debugging purposes, either
 * directly called from the driver or from the kernel debugger.
 */

void
mha_show_scsi_cmd(struct acb *acb)
{
	u_char *b = (u_char *)&acb->cmd;
	struct scsipi_periph *periph = acb->xs->xs_periph;
	int i;

	scsipi_printaddr(periph);
	if ((acb->xs->xs_control & XS_CTL_RESET) == 0) {
		for (i = 0; i < acb->clen; i++) {
			if (i)
				printf(",");
			printf("%x", b[i]);
		}
		printf("\n");
	} else
		printf("RESET\n");
}

void
mha_print_acb(struct acb *acb)
{

	printf("acb@%p xs=%p flags=%x", acb, acb->xs, acb->flags);
	printf(" dp=%p dleft=%d stat=%x\n",
	    acb->daddr, acb->dleft, acb->stat);
	mha_show_scsi_cmd(acb);
}

void
mha_print_active_acb(void)
{
	struct acb *acb;
	struct mha_softc *sc = mha_cd.cd_devs[0]; /* XXX */

	printf("ready list:\n");
	for (acb = sc->ready_list.tqh_first; acb != NULL;
	    acb = acb->chain.tqe_next)
		mha_print_acb(acb);
	printf("nexus:\n");
	if (sc->sc_nexus != NULL)
		mha_print_acb(sc->sc_nexus);
	printf("nexus list:\n");
	for (acb = sc->nexus_list.tqh_first; acb != NULL;
	    acb = acb->chain.tqe_next)
		mha_print_acb(acb);
}

void
mha_dump_driver(struct mha_softc *sc)
{
	struct spc_tinfo *ti;
	int i;

	printf("nexus=%p prevphase=%x\n", sc->sc_nexus, sc->sc_prevphase);
	printf("state=%x msgin=%x msgpriq=%x msgoutq=%x lastmsg=%x currmsg=%x\n",
	    sc->sc_state, sc->sc_imess[0],
	    sc->sc_msgpriq, sc->sc_msgoutq, sc->sc_lastmsg, sc->sc_currmsg);
	for (i = 0; i < 7; i++) {
		ti = &sc->sc_tinfo[i];
		printf("tinfo%d: %d cmds %d disconnects %d timeouts",
		    i, ti->cmds, ti->dconns, ti->touts);
		printf(" %d senses flags=%x\n", ti->senses, ti->flags);
	}
}
#endif
