/*	$NetBSD: spc.c,v 1.9.8.1 1997/07/01 17:34:45 bouyer Exp $	*/

#define	integrate	static inline

/*
 * Copyright (c) 1996 Masaru Oki.  All rights reserved.
 * Copyright (c) 1994, 1995, 1996 Charles M. Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
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

/*
 * Acknowledgements: Many of the algorithms used in this driver are
 * inspired by the work of Julian Elischer (julian@tfs.com) and
 * Charles Hannum (mycroft@duality.gnu.ai.mit.edu).  Thanks a million!
 */

/* TODO list:
 * 1) Get the DMA stuff working.
 * 2) Get the iov/uio stuff working. Is this a good thing ???
 * 3) Get the synch stuff working.
 * 4) Rewrite it to use malloc for the acb structs instead of static alloc.?
 */

/*
 * A few customizable items:
 */

/* Use doubleword transfers to/from SCSI chip.  Note: This requires
 * motherboard support.  Basicly, some motherboard chipsets are able to
 * split a 32 bit I/O operation into two 16 bit I/O operations,
 * transparently to the processor.  This speeds up some things, notably long
 * data transfers.
 */
#define SPC_USE_DWORDS		0

/* Synchronous data transfers? */
#define SPC_USE_SYNCHRONOUS	0
#define SPC_SYNC_REQ_ACK_OFS 	8

/* Wide data transfers? */
#define	SPC_USE_WIDE		0
#define	SPC_MAX_WIDTH		0

/* Max attempts made to transmit a message */
#define SPC_MSG_MAX_ATTEMPT	3 /* Not used now XXX */

/* Some spin loop parameters (essentially how long to wait some places)
 * The problem(?) is that sometimes we expect either to be able to transmit a
 * byte or to get a new one from the SCSI bus pretty soon.  In order to avoid
 * returning from the interrupt just to get yanked back for the next byte we
 * may spin in the interrupt routine waiting for this byte to come.  How long?
 * This is really (SCSI) device and processor dependent.  Tuneable, I guess.
 */
#define SPC_MSGIN_SPIN		1 	/* Will spinwait upto ?ms for a new msg byte */
#define SPC_MSGOUT_SPIN		1

/* Include debug functions?  At the end of this file there are a bunch of
 * functions that will print out various information regarding queued SCSI
 * commands, driver state and chip contents.  You can call them from the
 * kernel debugger.  If you set SPC_DEBUG to 0 they are not included (the
 * kernel uses less memory) but you lose the debugging facilities.
 */
#define SPC_DEBUG		1

#define	SPC_ABORT_TIMEOUT	2000	/* time to wait for abort */

/* End of customizable parameters */

/*
 * MB89352 SCSI Protocol Controller (SPC) routines.
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

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsi_message.h>
#include <dev/scsipi/scsiconf.h>

#include <x68k/x68k/iodevice.h>
#include <x68k/dev/mb89352reg.h>

/*
 * Definitions, most of them has turned out to be unneccesary, but here they
 * are anyway.
 */

#define IOBASE		sc->sc_iobase
#define BDID		(IOBASE->scsi_bdid)
#define SCTL		(IOBASE->scsi_sctl)
#define SCMD		(IOBASE->scsi_scmd)
#define TMOD		(IOBASE->scsi_tmod)
#define INTS		(IOBASE->scsi_ints)
#define PSNS		(IOBASE->scsi_psns)
#define SSTS		(IOBASE->scsi_ssts)
#define SERR		(IOBASE->scsi_serr)
#define PCTL		(IOBASE->scsi_pctl)
#define MBC		(IOBASE->scsi_mbc)
#define DREG		(IOBASE->scsi_dreg)
#define TEMP		(IOBASE->scsi_temp)
#define TCH		(IOBASE->scsi_tch)
#define TCM		(IOBASE->scsi_tcm)
#define TCL		(IOBASE->scsi_tcl)
#define EXBF		(IOBASE->scsi_exbf)

/* PSNS */
#define REQI		0x80
#define ACKI		0x40
#define ATNI		0x20
#define SELI		0x10
#define BSYI		0x08
#define MSGI		0x04
#define CDI		0x02
#define IOI		0x01

/* Important! The 3 most significant bits of this register, in initiator mode,
 * represents the "expected" SCSI bus phase and can be used to trigger phase
 * mismatch and phase change interrupts.  But more important:  If there is a
 * phase mismatch the chip will not transfer any data!  This is actually a nice
 * feature as it gives us a bit more control over what is happening when we are
 * bursting data (in) through the FIFOs and the phase suddenly changes from
 * DATA IN to STATUS or MESSAGE IN.  The transfer will stop and wait for the
 * proper phase to be set in this register instead of dumping the bits into the
 * FIFOs.
 */
#if 0
#define REQO		0x80
#define ACKO		0x40
#define ATNO		0x20
#define SELO		0x10
#define BSYO		0x08
#endif
/* PCTL */
#define MSGO		0x04
#define CDO		0x02
#define IOO		0x01

/* Information transfer phases */
#define PH_DATAOUT	(0)
#define PH_DATAIN	(IOI)
#define PH_CMD		(CDI)
#define PH_STAT		(CDI | IOI)
#define PH_MSGOUT	(MSGI | CDI)
#define PH_MSGIN	(MSGI | CDI | IOI)

#define PH_MASK		(MSGI | CDI | IOI)

#define	PH_INVALID	0xff

/* SCSI selection/reselection ID (both target *and* initiator) */
#define SELID7		0x80
#define SELID6		0x40
#define SELID5		0x20
#define SELID4		0x10
#define SELID3		0x08
#define SELID2		0x04
#define SELID1		0x02
#define SELID0		0x01

#ifndef DDB
#define	Debugger() panic("should call debugger here (spc.c)")
#endif /* ! DDB */

/*
 * ACB. Holds additional information for each SCSI command Comments: We
 * need a separate scsi command block because we may need to overwrite it
 * with a request sense command.  Basicly, we refrain from fiddling with
 * the scsi_xfer struct (except do the expected updating of return values).
 * We'll generally update: xs->{flags,resid,error,sense,status} and
 * occasionally xs->retries.
 */
struct spc_acb {
	struct scsi_generic scsi_cmd;
	int scsi_cmd_length;
	u_char *data_addr;		/* Saved data pointer */
	int data_length;		/* Residue */

	u_char target_stat;		/* SCSI status byte */

/*	struct spc_dma_seg dma[SPC_NSEG];*/ /* Physical addresses+len */

	TAILQ_ENTRY(spc_acb) chain;
	struct scsipi_xfer *xs;	/* SCSI xfer ctrl block from above */
	int flags;
#define ACB_ALLOC	0x01
#define	ACB_NEXUS	0x02
#define ACB_SENSE	0x04
#define	ACB_ABORT	0x40
#define	ACB_RESET	0x80
	int timeout;
};

/*
 * Some info about each (possible) target on the SCSI bus.  This should
 * probably have been a "per target+lunit" structure, but we'll leave it at
 * this for now.
 */
struct spc_tinfo {
	int	cmds;		/* #commands processed */
	int	dconns;		/* #disconnects */
	int	touts;		/* #timeouts */
	int	perrs;		/* #parity errors */
	int	senses;		/* #request sense commands sent */
	ushort	lubusy;		/* What local units/subr. are busy? */
	u_char  flags;
#define DO_SYNC		0x01	/* (Re)Negotiate synchronous options */
#define	DO_WIDE		0x02	/* (Re)Negotiate wide options */
	u_char  period;		/* Period suggestion */
	u_char  offset;		/* Offset suggestion */
	u_char	width;		/* Width suggestion */
} tinfo_t;

struct spc_softc {
	struct device sc_dev;
	volatile struct mb89352 *sc_iobase;

	struct scsipi_link sc_link;	/* prototype for subdevs */

	TAILQ_HEAD(, spc_acb) free_list, ready_list, nexus_list;
	struct spc_acb *sc_nexus;	/* current command */
	struct spc_acb sc_acb[8];
	struct spc_tinfo sc_tinfo[8];

	/* Data about the current nexus (updated for every cmd switch) */
	u_char	*sc_dp;		/* Current data pointer */
	size_t	sc_dleft;	/* Data bytes left to transfer */
	u_char	*sc_cp;		/* Current command pointer */
	size_t	sc_cleft;	/* Command bytes left to transfer */

	/* Adapter state */
	u_char	 sc_phase;	/* Current bus phase */
	u_char	 sc_prevphase;	/* Previous bus phase */
	u_char	 sc_state;	/* State applicable to the adapter */
#define	SPC_INIT	0
#define SPC_IDLE	1
#define SPC_SELECTING	2	/* SCSI command is arbiting  */
#define SPC_RESELECTED	3	/* Has been reselected */
#define SPC_CONNECTED	4	/* Actively using the SCSI bus */
#define	SPC_DISCONNECT	5	/* MSG_DISCONNECT received */
#define	SPC_CMDCOMPLETE	6	/* MSG_CMDCOMPLETE received */
#define SPC_CLEANING	7
	u_char	 sc_flags;
#define SPC_DROP_MSGIN	0x01	/* Discard all msgs (parity err detected) */
#define	SPC_ABORTING	0x02	/* Bailing out */
#define SPC_DOINGDMA	0x04	/* The FIFO data path is active! */
	u_char	sc_selid;	/* Reselection ID */

	/* Message stuff */
	u_char	sc_msgpriq;	/* Messages we want to send */
	u_char	sc_msgoutq;	/* Messages sent during last MESSAGE OUT */
	u_char	sc_lastmsg;	/* Message last transmitted */
	u_char	sc_currmsg;	/* Message currently ready to transmit */
#define SEND_DEV_RESET		0x01
#define SEND_PARITY_ERROR	0x02
#define SEND_INIT_DET_ERR	0x04
#define SEND_REJECT		0x08
#define SEND_IDENTIFY  		0x10
#define SEND_ABORT		0x20
#define SEND_SDTR		0x40
#define	SEND_WDTR		0x80
#define SPC_MAX_MSG_LEN 8
	u_char  sc_omess[SPC_MAX_MSG_LEN];
	u_char	*sc_omp;		/* Outgoing message pointer */
	u_char	sc_imess[SPC_MAX_MSG_LEN];
	u_char	*sc_imp;		/* Incoming message pointer */

	/* Hardware stuff */
	int	sc_initiator;		/* Our scsi id */
	int	sc_freq;		/* Clock frequency in MHz */
	int	sc_minsync;		/* Minimum sync period / 4 */
	int	sc_maxsync;		/* Maximum sync period / 4 */
};

#if SPC_DEBUG
#define SPC_SHOWACBS	0x01
#define SPC_SHOWINTS	0x02
#define SPC_SHOWCMDS	0x04
#define SPC_SHOWMISC	0x08
#define SPC_SHOWTRACE	0x10
#define SPC_SHOWSTART	0x20
#define SPC_DOBREAK	0x40
int spc_debug = 0x00; /* SPC_SHOWSTART|SPC_SHOWMISC|SPC_SHOWTRACE; */
#define	SPC_PRINT(b, s)	do {if ((spc_debug & (b)) != 0) printf s;} while (0)
#define	SPC_BREAK()	do {if ((spc_debug & SPC_DOBREAK) != 0) Debugger();} while (0)
#define	SPC_ASSERT(x)	do {if (x) {} else {printf("%s at line %d: assertion failed\n", sc->sc_dev.dv_xname, __LINE__); Debugger();}} while (0)
#else
#define	SPC_PRINT(b, s)
#define	SPC_BREAK()
#define	SPC_ASSERT(x)
#endif

#define SPC_ACBS(s)	SPC_PRINT(SPC_SHOWACBS, s)
#define SPC_INTS(s)	SPC_PRINT(SPC_SHOWINTS, s)
#define SPC_CMDS(s)	SPC_PRINT(SPC_SHOWCMDS, s)
#define SPC_MISC(s)	SPC_PRINT(SPC_SHOWMISC, s)
#define SPC_TRACE(s)	SPC_PRINT(SPC_SHOWTRACE, s)
#define SPC_START(s)	SPC_PRINT(SPC_SHOWSTART, s)

int	spcmatch	__P((struct device *, void *, void *));
void	spcattach	__P((struct device *, struct device *, void *));
void	spc_minphys	__P((struct buf *));
int	spcintr		__P((int));
void 	spc_init	__P((struct spc_softc *));
void	spc_done	__P((struct spc_softc *, struct spc_acb *));
void	spc_dequeue	__P((struct spc_softc *, struct spc_acb *));
int	spc_scsi_cmd	__P((struct scsipi_xfer *));
int	spc_poll	__P((struct spc_softc *, struct scsipi_xfer *, int));
integrate void	spc_sched_msgout __P((struct spc_softc *, u_char));
integrate void	spc_setsync	__P((struct spc_softc *, struct spc_tinfo *));
void	spc_select	__P((struct spc_softc *, struct spc_acb *));
void	spc_timeout	__P((void *));
void	spc_sched	__P((struct spc_softc *));
void	spc_scsi_reset	__P((struct spc_softc *));
void	spc_reset	__P((struct spc_softc *));
#if SPC_DEBUG
void	spc_print_active_acb();
void	spc_dump_driver();
#endif
volatile void *	spc_find	__P((int));

struct cfattach spc_ca = {
	sizeof(struct spc_softc), spcmatch, spcattach
};

struct cfdriver spc_cd = {
	NULL, "spc", DV_DULL
};

struct scsipi_adapter spc_switch = {
	spc_scsi_cmd,
	spc_minphys,
	0,
	0,
};

struct scsipi_device spc_dev = {
	NULL,			/* Use default error handler */
	NULL,			/* have a queue, served by this */
	NULL,			/* have no async handler */
	NULL,			/* Use default 'done' routine */
};

/*
 * INITIALIZATION ROUTINES (probe, attach ++)
 */

/*
 * returns non-zero value if a controller is found.
 */
int
spcmatch(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct cfdata *cf = match;

	if (strcmp(aux, "spc") || spc_find(cf->cf_unit) == 0)
		return 0;
	return 1;
}

/*
 * Find the board
 */
volatile void *
spc_find(unit)
	int unit;
{
	volatile void *addr;

	if (unit > 1)
		return 0;
	switch(unit) {
	case 0: /* builtin */
		if (badaddr(IODEVbase->inscsirom) ||
	    	    badbaddr(&IODEVbase->io_inspc.bdid) ||
	    	    bcmp((void *)&IODEVbase->inscsirom[0x24], "SCSIIN", 6))
			return 0;
		addr = &IODEVbase->io_inspc;
		break;
	case 1: /* external */
		if (badaddr(IODEVbase->exscsirom) ||
	    	    badbaddr(&IODEVbase->io_exspc.bdid) ||
	    	    bcmp((void *)&IODEVbase->exscsirom[0x24], "SCSIEX", 6))
			return 0;
		addr = &IODEVbase->io_exspc;
		break;
	}

	if (badaddr(addr))
		return 0;

	return addr;
}

/*
 */
void
spcattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct spc_softc *sc = (void *)self;

	SPC_TRACE(("spcattach  "));
	sc->sc_state = SPC_INIT;
	sc->sc_iobase = spc_find(sc->sc_dev.dv_unit); /* XXX */
	spc_init(sc);	/* Init chip and driver */

	/*
	 * Fill in the prototype scsipi_link
	 */
	sc->sc_link.scsipi_scsi.channel = SCSI_CHANNEL_ONLY_ONE;
	sc->sc_link.adapter_softc = sc;
	sc->sc_link.scsipi_scsi.adapter_target = sc->sc_initiator;
	sc->sc_link.adapter = &spc_switch;
	sc->sc_link.device = &spc_dev;
	sc->sc_link.openings = 2;
	sc->sc_link.scsipi_scsi.max_target = 7;
	sc->sc_link.type = BUS_SCSI;

	printf("\n");

	config_found(self, &sc->sc_link, scsiprint);
}

void
spc_reset(sc)
	struct spc_softc *sc;
{
	sc->sc_initiator = IODEVbase->io_sram[0x70] & 0x7; /* XXX */
	/*
	 * Disable interrupts then reset the FUJITSU chip.
	 */
	SCTL = SCTL_DISABLE | SCTL_CTRLRST;
	SCMD = 0;
	PCTL = 0;
	TEMP = 0;
	TCH  = 0;
	TCM  = 0;
	TCL  = 0;
	INTS = 0;
	SCTL = SCTL_DISABLE | SCTL_ABRT_ENAB | SCTL_PARITY_ENAB | SCTL_RESEL_ENAB;
	BDID = sc->sc_initiator;
	delay(400);
	SCTL &= ~SCTL_DISABLE;
}

/*
 * Pull the SCSI RST line for 500us.
 */
void
spc_scsi_reset(sc)
	struct spc_softc *sc;
{

	SCMD |= SCMD_RST;
	delay(500);
	SCMD &= ~SCMD_RST;
	delay(50);
}

/*
 * Initialize spc SCSI driver.
 */
void
spc_init(sc)
	struct spc_softc *sc;
{
	struct spc_acb *acb;
	int r;

	spc_reset(sc);
	spc_scsi_reset(sc);
	spc_reset(sc);

	if (sc->sc_state == SPC_INIT) {
		/* First time through; initialize. */
		TAILQ_INIT(&sc->ready_list);
		TAILQ_INIT(&sc->nexus_list);
		TAILQ_INIT(&sc->free_list);
		sc->sc_nexus = NULL;
		acb = sc->sc_acb;
		bzero(acb, sizeof(sc->sc_acb));
		for (r = 0; r < sizeof(sc->sc_acb) / sizeof(*acb); r++) {
			TAILQ_INSERT_TAIL(&sc->free_list, acb, chain);
			acb++;
		}
		bzero(&sc->sc_tinfo, sizeof(sc->sc_tinfo));
	} else {
		/* Cancel any active commands. */
		sc->sc_state = SPC_CLEANING;
		if ((acb = sc->sc_nexus) != NULL) {
			acb->xs->error = XS_DRIVER_STUFFUP;
			untimeout(spc_timeout, acb);
			spc_done(sc, acb);
		}
		while ((acb = sc->nexus_list.tqh_first) != NULL) {
			acb->xs->error = XS_DRIVER_STUFFUP;
			untimeout(spc_timeout, acb);
			spc_done(sc, acb);
		}
	}

	sc->sc_prevphase = PH_INVALID;
	for (r = 0; r < 8; r++) {
		struct spc_tinfo *ti = &sc->sc_tinfo[r];

		ti->flags = 0;
#if SPC_USE_SYNCHRONOUS
		ti->flags |= DO_SYNC;
		ti->period = sc->sc_minsync;
		ti->offset = SPC_SYNC_REQ_ACK_OFS;
#else
		ti->period = ti->offset = 0;
#endif
#if SPC_USE_WIDE
		ti->flags |= DO_WIDE;
		ti->width = SPC_MAX_WIDTH;
#else
		ti->width = 0;
#endif
	}

	sc->sc_state = SPC_IDLE;
	SCTL |= SCTL_INTR_ENAB;
}

void
spc_free_acb(sc, acb, flags)
	struct spc_softc *sc;
	struct spc_acb *acb;
	int flags;
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

struct spc_acb *
spc_get_acb(sc, flags)
	struct spc_softc *sc;
	int flags;
{
	struct spc_acb *acb;
	int s;

	s = splbio();

	while ((acb = sc->free_list.tqh_first) == NULL &&
	       (flags & SCSI_NOSLEEP) == 0)
		tsleep(&sc->free_list, PRIBIO, "spcacb", 0);
	if (acb) {
		TAILQ_REMOVE(&sc->free_list, acb, chain);
		acb->flags |= ACB_ALLOC;
	}

	splx(s);
	return acb;
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
 * Start a SCSI-command
 * This function is called by the higher level SCSI-driver to queue/run
 * SCSI-commands.
 */
int
spc_scsi_cmd(xs)
	struct scsipi_xfer *xs;
{
	struct scsipi_link *sc_link = xs->sc_link;
	struct spc_softc *sc = sc_link->adapter_softc;
	struct spc_acb *acb;
	int s, flags;

	SPC_TRACE(("spc_scsi_cmd  "));
	SPC_CMDS(("[0x%x, %d]->%d ", (int)xs->cmd->opcode, xs->cmdlen,
	    sc_link->scsipi_scsi.target));

	flags = xs->flags;
	if ((acb = spc_get_acb(sc, flags)) == NULL) {
		xs->error = XS_DRIVER_STUFFUP;
		return TRY_AGAIN_LATER;
	}

	/* Initialize acb */
	acb->xs = xs;
	acb->timeout = xs->timeout;

	if (xs->flags & SCSI_RESET) {
		acb->flags |= ACB_RESET;
		acb->scsi_cmd_length = 0;
		acb->data_length = 0;
	} else {
		bcopy(xs->cmd, &acb->scsi_cmd, xs->cmdlen);
#if 1
		acb->scsi_cmd.bytes[0] |= sc_link->scsipi_scsi.lun << 5; /* XXX? */
#endif
		acb->scsi_cmd_length = xs->cmdlen;
		acb->data_addr = xs->data;
		acb->data_length = xs->datalen;
	}
	acb->target_stat = 0;

	s = splbio();

	TAILQ_INSERT_TAIL(&sc->ready_list, acb, chain);
	/*
	 * キューの処理中でなければ、スケジューリング開始する
	 */
	if (sc->sc_state == SPC_IDLE)
		spc_sched(sc);
	/*
	 * 送信に成功したら、すぐにリターンするか調べる
	 * すぐリターンするなら SUCCESSFULLY_QUEUED を返す
	 */

	splx(s);

	if ((flags & SCSI_POLL) == 0)
		return SUCCESSFULLY_QUEUED;

	/* Not allowed to use interrupts, use polling instead */
	s = splbio();
	if (spc_poll(sc, xs, acb->timeout)) {
		spc_timeout(acb);
		if (spc_poll(sc, xs, acb->timeout))
			spc_timeout(acb);
	}
	splx(s);
	return COMPLETE;
}

/*
 * Adjust transfer size in buffer structure
 */
void
spc_minphys(bp)
	struct buf *bp;
{

	SPC_TRACE(("spc_minphys  "));
	minphys(bp);
}

/*
 * Used when interrupt driven I/O isn't allowed, e.g. during boot.
 */
int
spc_poll(sc, xs, count)
	struct spc_softc *sc;
	struct scsipi_xfer *xs;
	int count;
{

	SPC_TRACE(("spc_poll  "));
	while (count) {
		/*
		 * If we had interrupts enabled, would we
		 * have got an interrupt?
		 */
		if (INTS != 0)
			spcintr(sc->sc_dev.dv_unit);
		if ((xs->flags & ITSDONE) != 0)
			return 0;
		delay(1000);
		count--;
	}
	return 1;
}

/*
 * LOW LEVEL SCSI UTILITIES
 */

integrate void
spc_sched_msgout(sc, m)
	struct spc_softc *sc;
	u_char m;
{
	if (sc->sc_msgpriq == 0)
		SCMD = SCMD_SET_ATN;
	sc->sc_msgpriq |= m;
}

/*
 * Set synchronous transfer offset and period.
 */
integrate void
spc_setsync(sc, ti)
	struct spc_softc *sc;
	struct spc_tinfo *ti;
{
#if SPC_USE_SYNCHRONOUS

	if (ti->offset != 0)
		TMOD = 
		    ((ti->period * sc->sc_freq) / 250 - 2) << 4 | ti->offset);
	else
		TMOD = 0;
#endif
}

/*
 * Start a selection.  This is used by spc_sched() to select an idle target,
 * and by spc_done() to immediately reselect a target to get sense information.
 */
void
spc_select(sc, acb)
	struct spc_softc *sc;
	struct spc_acb *acb;
{
	struct scsipi_link *sc_link = acb->xs->sc_link;
	int target = sc_link->scsipi_scsi.target;
	struct spc_tinfo *ti = &sc->sc_tinfo[target];

	spc_setsync(sc, ti);

#if 0
	SCMD = SCMD_SET_ATN;
#endif
	PCTL = 0;
	TEMP = (1 << sc->sc_initiator) | (1 << target);
	/*
	 * BSY による応答待ち時間設定 (設定時間を過ぎると selection timeout)
	 * 0 にすると無限待ち (x68k では Tclf == 200ns)
	 * T = (X * 256 + 15) * Tclf * 2 なので... 256ms 待つとすると
	 * 128000ns/200ns = X * 256 + 15
	 * 640 - 15 = X * 256
	 * X = 625 / 256
	 * X = 2 + 113 / 256
	 * なので tch に 2, tcm に 113 を代入。(いいのか?)
	 */
	TCH = 2;
	TCM = 113;
	/* BSY と SEL が 0 になってからフェーズ開始までの時間 */
	TCL = 3;
	SCMD = SCMD_SELECT;

	sc->sc_state = SPC_SELECTING;
}

int
spc_reselect(sc, message)
	struct spc_softc *sc;
	u_char message;
{
	u_char selid, target, lun;
	struct spc_acb *acb;
	struct scsipi_link *sc_link;
	struct spc_tinfo *ti;

	/*
	 * The SCSI chip made a snapshot of the data bus while the reselection
	 * was being negotiated.  This enables us to determine which target did
	 * the reselect.
	 */
	selid = sc->sc_selid & ~(1 << sc->sc_initiator);
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
		sc_link = acb->xs->sc_link;
		if (sc_link->scsipi_scsi.target == target &&
			sc_link->scsipi_scsi.lun == lun)
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
	sc->sc_state = SPC_CONNECTED;
	sc->sc_nexus = acb;
	ti = &sc->sc_tinfo[target];
	ti->lubusy |= (1 << lun);
	spc_setsync(sc, ti);

	if (acb->flags & ACB_RESET)
		spc_sched_msgout(sc, SEND_DEV_RESET);
	else if (acb->flags & ACB_ABORT)
		spc_sched_msgout(sc, SEND_ABORT);

	/* Do an implicit RESTORE POINTERS. */
	sc->sc_dp = acb->data_addr;
	sc->sc_dleft = acb->data_length;
	sc->sc_cp = (u_char *)&acb->scsi_cmd;
	sc->sc_cleft = acb->scsi_cmd_length;

	return (0);

reset:
	spc_sched_msgout(sc, SEND_DEV_RESET);
	return (1);

abort:
	spc_sched_msgout(sc, SEND_ABORT);
	return (1);
}

/*
 * Schedule a SCSI operation.  This has now been pulled out of the interrupt
 * handler so that we may call it from spc_scsi_cmd and spc_done.  This may
 * save us an unecessary interrupt just to get things going.  Should only be
 * called when state == SPC_IDLE and at bio pl.
 */
void
spc_sched(sc)
	register struct spc_softc *sc;
{
	struct spc_acb *acb;
	struct scsipi_link *sc_link;
	struct spc_tinfo *ti;

	/*
	 * Find first acb in ready queue that is for a target/lunit pair that
	 * is not busy.
	 */
	for (acb = sc->ready_list.tqh_first; acb != NULL;
	    acb = acb->chain.tqe_next) {
		sc_link = acb->xs->sc_link;
		ti = &sc->sc_tinfo[sc_link->scsipi_scsi.target];
		if ((ti->lubusy & (1 << sc_link->scsipi_scsi.lun)) == 0) {
			SPC_MISC(("selecting %d:%d  ",
			    sc_link->scsipi_scsi.target, sc_link->scsipi_scsi.lun));
			TAILQ_REMOVE(&sc->ready_list, acb, chain);
			sc->sc_nexus = acb;
			spc_select(sc, acb);
			return;
		} else
			SPC_MISC(("%d:%d busy\n",
			    sc_link->scsipi_scsi.target, sc_link->scsipi_scsi.lun));
	}
	SPC_MISC(("idle  "));
	/* Nothing to start; just enable reselections and wait. */
}

void
spc_sense(sc, acb)
	struct spc_softc *sc;
	struct spc_acb *acb;
{
	struct scsipi_xfer *xs = acb->xs;
	struct scsipi_link *sc_link = xs->sc_link;
	struct spc_tinfo *ti = &sc->sc_tinfo[sc_link->scsipi_scsi.target];
	struct scsipi_sense *ss = (void *)&acb->scsi_cmd;

	SPC_MISC(("requesting sense  "));
	/* Next, setup a request sense command block */
	bzero(ss, sizeof(*ss));
	ss->opcode = REQUEST_SENSE;
	ss->byte2 = sc_link->scsipi_scsi.lun << 5;
	ss->length = sizeof(struct scsipi_sense_data);
	acb->scsi_cmd_length = sizeof(*ss);
	acb->data_addr = (char *)&xs->sense.scsi_sense;
	acb->data_length = sizeof(struct scsipi_sense_data);
	acb->flags |= ACB_SENSE;
	ti->senses++;
	if (acb->flags & ACB_NEXUS)
		ti->lubusy &= ~(1 << sc_link->scsipi_scsi.lun);
	if (acb == sc->sc_nexus) {
		spc_select(sc, acb);
	} else {
		spc_dequeue(sc, acb);
		TAILQ_INSERT_HEAD(&sc->ready_list, acb, chain);
		if (sc->sc_state == SPC_IDLE)
			spc_sched(sc);
	}
}

/*
 * POST PROCESSING OF SCSI_CMD (usually current)
 */
void
spc_done(sc, acb)
	struct spc_softc *sc;
	struct spc_acb *acb;
{
	struct scsipi_xfer *xs = acb->xs;
	struct scsipi_link *sc_link = xs->sc_link;
	struct spc_tinfo *ti = &sc->sc_tinfo[sc_link->scsipi_scsi.target];

	SPC_TRACE(("spc_done  "));

	/*
	 * Now, if we've come here with no error code, i.e. we've kept the
	 * initial XS_NOERROR, and the status code signals that we should
	 * check sense, we'll need to set up a request sense cmd block and
	 * push the command back into the ready queue *before* any other
	 * commands for this target/lunit, else we lose the sense info.
	 * We don't support chk sense conditions for the request sense cmd.
	 */
	if (xs->error == XS_NOERROR) {
		if (acb->flags & ACB_ABORT) {
			xs->error = XS_DRIVER_STUFFUP;
		} else if (acb->flags & ACB_SENSE) {
			xs->error = XS_SENSE;
		} else if (acb->target_stat == SCSI_CHECK) {
			/* First, save the return values */
			xs->resid = acb->data_length;
			xs->status = acb->target_stat;
			spc_sense(sc, acb);
			return;
		} else {
			xs->resid = acb->data_length;
		}
	}

	xs->flags |= ITSDONE;

#if SPC_DEBUG
	if ((spc_debug & SPC_SHOWMISC) != 0) {
		if (xs->resid != 0)
			printf("resid=%d ", xs->resid);
		if (xs->error == XS_SENSE)
			printf("sense=0x%02x\n", xs->sense.scsi_sense.error_code);
		else
			printf("error=%d\n", xs->error);
	}
#endif

	/*
	 * Remove the ACB from whatever queue it happens to be on.
	 */
	if (acb->flags & ACB_NEXUS)
		ti->lubusy &= ~(1 << sc_link->scsipi_scsi.lun);
	if (acb == sc->sc_nexus) {
		sc->sc_nexus = NULL;
		sc->sc_state = SPC_IDLE;
		spc_sched(sc);
	} else
		spc_dequeue(sc, acb);

	spc_free_acb(sc, acb, xs->flags);
	ti->cmds++;
	scsipi_done(xs);
}

void
spc_dequeue(sc, acb)
	struct spc_softc *sc;
	struct spc_acb *acb;
{

	if (acb->flags & ACB_NEXUS) {
		TAILQ_REMOVE(&sc->nexus_list, acb, chain);
	} else {
		TAILQ_REMOVE(&sc->ready_list, acb, chain);
	}
}

/*
 * INTERRUPT/PROTOCOL ENGINE
 */

#define IS1BYTEMSG(m) (((m) != 0x01 && (m) < 0x20) || (m) >= 0x80)
#define IS2BYTEMSG(m) (((m) & 0xf0) == 0x20)
#define ISEXTMSG(m) ((m) == 0x01)

/*
 * Precondition:
 * The SCSI bus is already in the MSGI phase and there is a message byte
 * on the bus, along with an asserted REQ signal.
 */
void
spc_msgin(sc)
	register struct spc_softc *sc;
{
	int n;

	SPC_TRACE(("spc_msgin  "));

	if (sc->sc_prevphase == PH_MSGIN) {
		/* This is a continuation of the previous message. */
		n = sc->sc_imp - sc->sc_imess;
		goto nextbyte;
	}

	/* This is a new MESSAGE IN phase.  Clean up our state. */
	sc->sc_flags &= ~SPC_DROP_MSGIN;

nextmsg:
	n = 0;
	sc->sc_imp = &sc->sc_imess[n];

nextbyte:
	/*
	 * Read a whole message, but don't ack the last byte.  If we reject the
	 * message, we have to assert ATN during the message transfer phase
	 * itself.
	 */
	for (;;) {
#if 0
		for (;;) {
			if ((PSNS & PSNS_REQ) != 0)
				break;
			/* Wait for REQINIT.  XXX Need timeout. */
		}
#endif
		if (INTS != 0) {
			/*
			 * Target left MESSAGE IN, probably because it
			 * a) noticed our ATN signal, or
			 * b) ran out of messages.
			 */
			goto out;
		}

		/* If parity error, just dump everything on the floor. */
		if ((SERR & (SERR_SCSI_PAR|SERR_SPC_PAR)) != 0) {
			sc->sc_flags |= SPC_DROP_MSGIN;
			spc_sched_msgout(sc, SEND_PARITY_ERROR);
		}

		/* send TRANSFER command. */
		TCH = 0;
		TCM = 0;
		TCL = 1;
		PCTL = sc->sc_phase | PCTL_BFINT_ENAB;
		SCMD = SCMD_XFR; /* | SCMD_PROG_XFR */
		for (;;) {
			/*if ((SSTS & SSTS_BUSY) != 0 && (SSTS & SSTS_DREG_EMPTY) != 0)*/
			if ((SSTS & SSTS_DREG_EMPTY) == 0)
				break;
			if (INTS != 0)
				goto out;
		}

		/* Gather incoming message bytes if needed. */
		if ((sc->sc_flags & SPC_DROP_MSGIN) == 0) {
			if (n >= SPC_MAX_MSG_LEN) {
				(void) DREG;
				sc->sc_flags |= SPC_DROP_MSGIN;
				spc_sched_msgout(sc, SEND_REJECT);
			} else {
				*sc->sc_imp++ = DREG;
				n++;
				/*
				 * This testing is suboptimal, but most
				 * messages will be of the one byte variety, so
				 * it should not affect performance
				 * significantly.
				 */
				if (n == 1 && IS1BYTEMSG(sc->sc_imess[0]))
					break;
				if (n == 2 && IS2BYTEMSG(sc->sc_imess[0]))
					break;
				if (n >= 3 && ISEXTMSG(sc->sc_imess[0]) &&
				    n == sc->sc_imess[1] + 2)
					break;
			}
		} else
			(void) DREG;

		/*
		 * If we reach this spot we're either:
		 * a) in the middle of a multi-byte message, or
		 * b) dropping bytes.
		 */

#if 0
		/* Ack the last byte read. */
		/*(void) DREG;*/
		while ((PSNS & ACKI) != 0)
			;
#endif
	}

	SPC_MISC(("n=%d imess=0x%02x  ", n, sc->sc_imess[0]));

	/* We now have a complete message.  Parse it. */
	switch (sc->sc_state) {
		struct spc_acb *acb;
		struct scsipi_link *sc_link;
		struct spc_tinfo *ti;

	case SPC_CONNECTED:
		SPC_ASSERT(sc->sc_nexus != NULL);
		acb = sc->sc_nexus;
		ti = &sc->sc_tinfo[acb->xs->sc_link->scsipi_scsi.target];

		switch (sc->sc_imess[0]) {
		case MSG_CMDCOMPLETE:
			if (sc->sc_dleft < 0) {
				sc_link = acb->xs->sc_link;
				printf("%s: %d extra bytes from %d:%d\n",
				    sc->sc_dev.dv_xname, -sc->sc_dleft,
				    sc_link->scsipi_scsi.target, sc_link->scsipi_scsi.lun);
				acb->data_length = 0;
			}
			acb->xs->resid = acb->data_length = sc->sc_dleft;
			sc->sc_state = SPC_CMDCOMPLETE;
			break;

		case MSG_PARITY_ERROR:
			/* Resend the last message. */
			spc_sched_msgout(sc, sc->sc_lastmsg);
			break;

		case MSG_MESSAGE_REJECT:
			SPC_MISC(("message rejected %02x  ", sc->sc_lastmsg));
			switch (sc->sc_lastmsg) {
#if SPC_USE_SYNCHRONOUS + SPC_USE_WIDE
			case SEND_IDENTIFY:
				ti->flags &= ~(DO_SYNC | DO_WIDE);
				ti->period = ti->offset = 0;
				spc_setsync(sc, ti);
				ti->width = 0;
				break;
#endif
#if SPC_USE_SYNCHRONOUS
			case SEND_SDTR:
				ti->flags &= ~DO_SYNC;
				ti->period = ti->offset = 0;
				spc_setsync(sc, ti);
				break;
#endif
#if SPC_USE_WIDE
			case SEND_WDTR:
				ti->flags &= ~DO_WIDE;
				ti->width = 0;
				break;
#endif
			case SEND_INIT_DET_ERR:
				spc_sched_msgout(sc, SEND_ABORT);
				break;
			}
			break;

		case MSG_NOOP:
			break;

		case MSG_DISCONNECT:
			ti->dconns++;
			sc->sc_state = SPC_DISCONNECT;
			break;

		case MSG_SAVEDATAPOINTER:
			acb->data_addr = sc->sc_dp;
			acb->data_length = sc->sc_dleft;
			break;

		case MSG_RESTOREPOINTERS:
			sc->sc_dp = acb->data_addr;
			sc->sc_dleft = acb->data_length;
			sc->sc_cp = (u_char *)&acb->scsi_cmd;
			sc->sc_cleft = acb->scsi_cmd_length;
			break;

		case MSG_EXTENDED:
			switch (sc->sc_imess[2]) {
#if SPC_USE_SYNCHRONOUS
			case MSG_EXT_SDTR:
				if (sc->sc_imess[1] != 3)
					goto reject;
				ti->period = sc->sc_imess[3];
				ti->offset = sc->sc_imess[4];
				ti->flags &= ~DO_SYNC;
				if (ti->offset == 0) {
				} else if (ti->period < sc->sc_minsync ||
					   ti->period > sc->sc_maxsync ||
					   ti->offset > 8) {
					ti->period = ti->offset = 0;
					spc_sched_msgout(sc, SEND_SDTR);
				} else {
					scsi_print_addr(acb->xs->sc_link);
					printf("sync, offset %d, period %dnsec\n",
					    ti->offset, ti->period * 4);
				}
				spc_setsync(sc, ti);
				break;
#endif

#if SPC_USE_WIDE
			case MSG_EXT_WDTR:
				if (sc->sc_imess[1] != 2)
					goto reject;
				ti->width = sc->sc_imess[3];
				ti->flags &= ~DO_WIDE;
				if (ti->width == 0) {
				} else if (ti->width > SPC_MAX_WIDTH) {
					ti->width = 0;
					spc_sched_msgout(sc, SEND_WDTR);
				} else {
					scsi_print_addr(acb->xs->sc_link);
					printf("wide, width %d\n",
					    1 << (3 + ti->width));
				}
				break;
#endif

			default:
				printf("%s: unrecognized MESSAGE EXTENDED; sending REJECT\n",
				    sc->sc_dev.dv_xname);
				SPC_BREAK();
				goto reject;
			}
			break;

		default:
			printf("%s: unrecognized MESSAGE; sending REJECT\n",
			    sc->sc_dev.dv_xname);
			SPC_BREAK();
		reject:
			spc_sched_msgout(sc, SEND_REJECT);
			break;
		}
		break;

	case SPC_RESELECTED:
		if (!MSG_ISIDENTIFY(sc->sc_imess[0])) {
			printf("%s: reselect without IDENTIFY; sending DEVICE RESET\n",
			    sc->sc_dev.dv_xname);
			SPC_BREAK();
			goto reset;
		}

		(void) spc_reselect(sc, sc->sc_imess[0]);
		break;

	default:
		printf("%s: unexpected MESSAGE IN; sending DEVICE RESET\n",
		    sc->sc_dev.dv_xname);
		SPC_BREAK();
	reset:
		spc_sched_msgout(sc, SEND_DEV_RESET);
		break;

	abort:
		spc_sched_msgout(sc, SEND_ABORT);
		break;
	}

	/* Ack the last message byte. */
#if 0 /* XXX? */
	(void) DREG;
	while ((PSNS & ACKI) != 0)
		;
#endif

	/* Go get the next message, if any. */
	goto nextmsg;

out:
	SCMD = SCMD_RST_ACK;
	SPC_MISC(("n=%d imess=0x%02x  ", n, sc->sc_imess[0]));
}

/*
 * Send the highest priority, scheduled message.
 */
void
spc_msgout(sc)
	register struct spc_softc *sc;
{
	struct spc_tinfo *ti;
	int n;

	SPC_TRACE(("spc_msgout  "));

	if (sc->sc_prevphase == PH_MSGOUT) {
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
			SCMD = SCMD_SET_ATN; /* XXX? */
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
		    MSG_IDENTIFY(sc->sc_nexus->xs->sc_link->scsipi_scsi.lun, 1);
		n = 1;
		break;

#if SPC_USE_SYNCHRONOUS
	case SEND_SDTR:
		SPC_ASSERT(sc->sc_nexus != NULL);
		ti = &sc->sc_tinfo[sc->sc_nexus->xs->sc_link->scsipi_scsi.target];
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
		ti = &sc->sc_tinfo[sc->sc_nexus->xs->sc_link->scsipi_scsi.target];
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
	TCH = n >> 16;
	TCM = n >> 8;
	TCL = n;
	PCTL = sc->sc_phase | PCTL_BFINT_ENAB;
	SCMD = SCMD_XFR; /* | SCMD_PROG_XFR */
	for (;;) {
		if ((SSTS & SSTS_BUSY) != 0)
			break;
		if (INTS != 0)
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
		if (INTS != 0) {
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
				SCMD = SCMD_RST_ATN;
#endif
			goto out;
		}

#if 0
		/* Clear ATN before last byte if this is the last message. */
		if (n == 1 && sc->sc_msgpriq == 0)
			SCMD = SCMD_RST_ATN;
#endif

		while ((SSTS & SSTS_DREG_FULL) != 0)
			;
		/* Send message byte. */
		DREG = *--sc->sc_omp;
		--n;
		/* Keep track of the last message we've sent any bytes of. */
		sc->sc_lastmsg = sc->sc_currmsg;
#if 0
		/* Wait for ACK to be negated.  XXX Need timeout. */
		while ((PSNS & ACKI) != 0)
			;
#endif

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
}

/*
 * This new revision has been optimized (I tried) to make the common case fast,
 * and the rarer cases (as a result) somewhat more comlex
 */
int
spc_dataout_pio(sc, p, n)
	register struct spc_softc *sc;
	u_char *p;
	int n;
{
	register u_char intstat = 0;
	int out = 0;
#define DOUTAMOUNT 8		/* Full FIFO */

	/* send TRANSFER command. */
	TCH = n >> 16;
	TCM = n >> 8;
	TCL = n;
	PCTL = sc->sc_phase | PCTL_BFINT_ENAB;
	SCMD = SCMD_XFR;
	for (;;) {
		if ((SSTS & SSTS_BUSY) != 0)
			break;
		if (INTS != 0)
			break;
	}

	/*
	 * I have tried to make the main loop as tight as possible.  This
	 * means that some of the code following the loop is a bit more
	 * complex than otherwise.
	 */
	while (n > 0) {
		int xfer;

		for (;;) {
			intstat = INTS;
			/* バッファが空になるまで待つ */
			if ((SSTS & SSTS_DREG_EMPTY) != 0)
				break;
			/* ただし割り込みが入ってきたら抜ける */
			if (intstat != 0)
				goto phasechange;
		}

		xfer = min(DOUTAMOUNT, n);

		SPC_MISC(("%d> ", xfer));

		n -= xfer;
		out += xfer;

		while (xfer-- > 0) {
			DREG = *p++;
		}
	}

	if (out == 0) {
		for (;;) {
			if (INTS != 0)
				break;
		}
		SPC_MISC(("extra data  "));
	} else {
		/* See the bytes off chip */
		for (;;) {
			/* バッファが空になるまで待つ */
			if ((SSTS & SSTS_DREG_EMPTY) != 0)
				break;
			intstat = INTS;
			/* ただし割り込みが入ってきたら抜ける */
			if (intstat != 0)
				goto phasechange;
		}
	}

phasechange:
	/* Stop the FIFO data path. */

	if (intstat != 0) {
		/* Some sort of phase change. */
		int amount;

		amount = (TCH << 16) | (TCM << 8) | TCL;
		if (amount > 0) {
			out -= amount;
			SPC_MISC(("+%d ", amount));
		}
	}
	/* Turn on ENREQINIT again. */

	return out;
}

/*
 * For now, uses a pretty dumb algorithm, hangs around until all data has been
 * transferred.  This, is OK for fast targets, but not so smart for slow
 * targets which don't disconnect or for huge transfers.
 */
int
spc_datain_pio(sc, p, n)
	register struct spc_softc *sc;
	u_char *p;
	int n;
{
	register u_short intstat;
	int in = 0;
#define DINAMOUNT 8		/* Full FIFO */

	/* send TRANSFER command. */
	TCH = n >> 16;
	TCM = n >> 8;
	TCL = n;
	PCTL = sc->sc_phase | PCTL_BFINT_ENAB;
	SCMD = SCMD_XFR;
	for (;;) {
		if ((SSTS & SSTS_BUSY) != 0)
			break;
		if (INTS != 0)
			goto phasechange;
	}

	/*
	 * We leave this loop if one or more of the following is true:
	 * a) phase != PH_DATAIN && FIFOs are empty
	 * b) reset has occurred or busfree is detected.
	 */
	while (n > 0) {
		int xfer;

#define INTSMASK 0xff
		/* Wait for fifo half full or phase mismatch */
		for (;;) {
			intstat = (SSTS << 8) | INTS;
			if ((intstat & (INTSMASK | (SSTS_DREG_FULL << 8))) != 0)
				break;
			if ((intstat & (SSTS_DREG_EMPTY << 8)) == 0)
				break;
		}

#if 1
		if ((intstat & INTSMASK) != 0)
			goto phasechange;
#else
		if ((intstat & INTSMASK) != 0 &&
		    (intstat & (SSTS_DREG_EMPTY << 8)))
			goto phasechange;
#endif
		if ((intstat & (SSTS_DREG_FULL << 8)) != 0)
			xfer = min(DINAMOUNT, n);
		else
			xfer = min(1, n);

		SPC_MISC((">%d ", xfer));

		n -= xfer;
		in += xfer;

		while (xfer-- > 0) {
			*p++ = DREG;
		}

		if ((intstat & INTSMASK) != 0)
			goto phasechange;
	}

	/*
	 * Some SCSI-devices are rude enough to transfer more data than what
	 * was requested, e.g. 2048 bytes from a CD-ROM instead of the
	 * requested 512.  Test for progress, i.e. real transfers.  If no real
	 * transfers have been performed (n is probably already zero) and the
	 * FIFO is not empty, waste some bytes....
	 */
	if (in == 0) {
		for (;;) {
			if (INTS != 0)
				break;
		}
		SPC_MISC(("extra data  "));
	}

phasechange:
	/* Stop the FIFO data path. */

	/* Turn on ENREQINIT again. */

	return in;
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
spcintr(unit)
	int unit;
{
	register struct spc_softc *sc = spc_cd.cd_devs[unit]; /* XXX */
	u_char ints;
	register struct spc_acb *acb;
	register struct scsipi_link *sc_link;
	struct spc_tinfo *ti;
	int n;

	/*
	 * 割り込み禁止にする
	 */
	SCTL &= ~SCTL_INTR_ENAB;

	SPC_TRACE(("spcintr  "));

loop:
	/*
	 * 全転送が完全に終了するまでループする
	 */
	/*
	 * First check for abnormal conditions, such as reset.
	 */
#if 1 /* XXX? */
	while ((ints = INTS) == 0)
		delay(1);
	SPC_MISC(("ints = 0x%x  ", ints));
#else /* usually? */
	ints = INTS;
#endif
	if ((ints & INTS_RST) != 0) {
		printf("%s: SCSI bus reset\n", sc->sc_dev.dv_xname);
		goto reset;
	}

	/*
	 * Check for less serious errors.
	 */
	if ((SERR & (SERR_SCSI_PAR|SERR_SPC_PAR)) != 0) {
		printf("%s: SCSI bus parity error\n", sc->sc_dev.dv_xname);
		if (sc->sc_prevphase == PH_MSGIN) {
			sc->sc_flags |= SPC_DROP_MSGIN;
			spc_sched_msgout(sc, SEND_PARITY_ERROR);
		} else
			spc_sched_msgout(sc, SEND_INIT_DET_ERR);
	}

	/*
	 * If we're not already busy doing something test for the following
	 * conditions:
	 * 1) We have been reselected by something
	 * 2) We have selected something successfully
	 * 3) Our selection process has timed out
	 * 4) This is really a bus free interrupt just to get a new command
	 *    going?
	 * 5) Spurious interrupt?
	 */
	switch (sc->sc_state) {
	case SPC_IDLE:
	case SPC_SELECTING:

		if ((ints & INTS_SEL) != 0) {
			/*
			 * We don't currently support target mode.
			 */
			printf("%s: target mode selected; going to BUS FREE\n",
			    sc->sc_dev.dv_xname);

			goto sched;
		} else if ((ints & INTS_RESEL) != 0) {
			SPC_MISC(("reselected  "));

			/*
			 * If we're trying to select a target ourselves,
			 * push our command back into the ready list.
			 */
			if (sc->sc_state == SPC_SELECTING) {
				SPC_MISC(("backoff selector  "));
				SPC_ASSERT(sc->sc_nexus != NULL);
				acb = sc->sc_nexus;
				sc->sc_nexus = NULL;
				TAILQ_INSERT_HEAD(&sc->ready_list, acb, chain);
			}

			/* Save reselection ID. */
			sc->sc_selid = TEMP;

			sc->sc_state = SPC_RESELECTED;
		} else if ((ints & INTS_CMD_DONE) != 0) {
			SPC_MISC(("selected  "));

			/*
			 * We have selected a target. Things to do:
			 * a) Determine what message(s) to send.
			 * b) Verify that we're still selecting the target.
			 * c) Mark device as busy.
			 */
			if (sc->sc_state != SPC_SELECTING) {
				printf("%s: selection out while idle; resetting\n",
				    sc->sc_dev.dv_xname);
				SPC_BREAK();
				goto reset;
			}
			SPC_ASSERT(sc->sc_nexus != NULL);
			acb = sc->sc_nexus;
			sc_link = acb->xs->sc_link;
			ti = &sc->sc_tinfo[sc_link->scsipi_scsi.target];

			sc->sc_msgpriq = SEND_IDENTIFY;
			if (acb->flags & ACB_RESET)
				sc->sc_msgpriq |= SEND_DEV_RESET;
			else if (acb->flags & ACB_ABORT)
				sc->sc_msgpriq |= SEND_ABORT;
			else {
#if SPC_USE_SYNCHRONOUS
				if ((ti->flags & DO_SYNC) != 0)
					sc->sc_msgpriq |= SEND_SDTR;
#endif
#if SPC_USE_WIDE
				if ((ti->flags & DO_WIDE) != 0)
					sc->sc_msgpriq |= SEND_WDTR;
#endif
			}

			acb->flags |= ACB_NEXUS;
			ti->lubusy |= (1 << sc_link->scsipi_scsi.lun);

			/* Do an implicit RESTORE POINTERS. */
			sc->sc_dp = acb->data_addr;
			sc->sc_dleft = acb->data_length;
			sc->sc_cp = (u_char *)&acb->scsi_cmd;
			sc->sc_cleft = acb->scsi_cmd_length;

			/* On our first connection, schedule a timeout. */
			if ((acb->xs->flags & SCSI_POLL) == 0)
				timeout(spc_timeout, acb, (acb->timeout * hz) / 1000);

			sc->sc_state = SPC_CONNECTED;
		} else if ((ints & INTS_TIMEOUT) != 0) {
			SPC_MISC(("selection timeout  "));

			if (sc->sc_state != SPC_SELECTING) {
				printf("%s: selection timeout while idle; resetting\n",
				    sc->sc_dev.dv_xname);
				SPC_BREAK();
				goto reset;
			}
			SPC_ASSERT(sc->sc_nexus != NULL);
			acb = sc->sc_nexus;

			delay(250);

			acb->xs->error = XS_SELTIMEOUT;
			goto finish;
		} else {
			if (sc->sc_state != SPC_IDLE) {
				printf("%s: BUS FREE while not idle; state=%d\n",
				    sc->sc_dev.dv_xname, sc->sc_state);
				SPC_BREAK();
				goto out;
			}

			goto sched;
		}

		/*
		 * Turn off selection stuff, and prepare to catch bus free
		 * interrupts, parity errors, and phase changes.
		 */

		sc->sc_flags = 0;
		sc->sc_prevphase = PH_INVALID;
		goto dophase;
	}

	if ((ints & INTS_DISCON) != 0) {
		/* We've gone to BUS FREE phase. */
		PCTL &= ~PCTL_BFINT_ENAB; /* disable disconnect interrupt */
		INTS = ints; /* XXX reset interrput */

		switch (sc->sc_state) {
		case SPC_RESELECTED:
			goto sched;

		case SPC_CONNECTED:
			SPC_ASSERT(sc->sc_nexus != NULL);
			acb = sc->sc_nexus;

#if SPC_USE_SYNCHRONOUS + SPC_USE_WIDE
			if (sc->sc_prevphase == PH_MSGOUT) {
				/*
				 * If the target went to BUS FREE phase during
				 * or immediately after sending a SDTR or WDTR
				 * message, disable negotiation.
				 */
				sc_link = acb->xs->sc_link;
				ti = &sc->sc_tinfo[sc_link->scsipi_scsi.target];
				switch (sc->sc_lastmsg) {
#if SPC_USE_SYNCHRONOUS
				case SEND_SDTR:
					ti->flags &= ~DO_SYNC;
					ti->period = ti->offset = 0;
					break;
#endif
#if SPC_USE_WIDE
				case SEND_WDTR:
					ti->flags &= ~DO_WIDE;
					ti->width = 0;
					break;
#endif
				}
			}
#endif

			if ((sc->sc_flags & SPC_ABORTING) == 0) {
				/*
				 * Section 5.1.1 of the SCSI 2 spec suggests
				 * issuing a REQUEST SENSE following an
				 * unexpected disconnect.  Some devices go into
				 * a contingent allegiance condition when
				 * disconnecting, and this is necessary to
				 * clean up their state.
				 */
				printf("%s: unexpected disconnect; sending REQUEST SENSE\n",
				    sc->sc_dev.dv_xname);
				SPC_BREAK();
				spc_sense(sc, acb);
				goto out;
			}

			acb->xs->error = XS_DRIVER_STUFFUP;
			goto finish;

		case SPC_DISCONNECT:
			SPC_ASSERT(sc->sc_nexus != NULL);
			acb = sc->sc_nexus;
			TAILQ_INSERT_HEAD(&sc->nexus_list, acb, chain);
			sc->sc_nexus = NULL;
			goto sched;

		case SPC_CMDCOMPLETE:
			SPC_ASSERT(sc->sc_nexus != NULL);
			acb = sc->sc_nexus;
			goto finish;
		}
	}
	else if ((ints & INTS_CMD_DONE) != 0 &&
		 sc->sc_prevphase == PH_MSGIN && sc->sc_state != SPC_CONNECTED)
		goto out;

dophase:
#if 0
	if ((PSNS & PSNS_REQ) == 0) {
		/* Wait for REQINIT. */
		goto out;
	}
#else
	INTS = ints;
	ints = 0;
	while ((PSNS & PSNS_REQ) == 0)
		delay(1);	/* need timeout XXX */
#endif

	/*
	 * フェーズによって状態遷移する
	 */
	sc->sc_phase = PSNS & PH_MASK;
/*	PCTL = sc->sc_phase;*/

	switch (sc->sc_phase) {
	case PH_MSGOUT:
		if (sc->sc_state != SPC_CONNECTED &&
		    sc->sc_state != SPC_RESELECTED)
			break;
		spc_msgout(sc);
		sc->sc_prevphase = PH_MSGOUT;
		goto loop;

	case PH_MSGIN:
		if (sc->sc_state != SPC_CONNECTED &&
		    sc->sc_state != SPC_RESELECTED)
			break;
		spc_msgin(sc);
		sc->sc_prevphase = PH_MSGIN;
		goto loop;

	case PH_CMD:
		if (sc->sc_state != SPC_CONNECTED)
			break;
#if SPC_DEBUG
		if ((spc_debug & SPC_SHOWMISC) != 0) {
			SPC_ASSERT(sc->sc_nexus != NULL);
			acb = sc->sc_nexus;
			printf("cmd=0x%02x+%d  ",
			    acb->scsi_cmd.opcode, acb->scsi_cmd_length-1);
		}
#endif
		n = spc_dataout_pio(sc, sc->sc_cp, sc->sc_cleft);
		sc->sc_cp += n;
		sc->sc_cleft -= n;
		sc->sc_prevphase = PH_CMD;
		goto loop;

	case PH_DATAOUT:
		if (sc->sc_state != SPC_CONNECTED)
			break;
		SPC_MISC(("dataout dleft=%d  ", sc->sc_dleft));
		n = spc_dataout_pio(sc, sc->sc_dp, sc->sc_dleft);
		sc->sc_dp += n;
		sc->sc_dleft -= n;
		sc->sc_prevphase = PH_DATAOUT;
		goto loop;

	case PH_DATAIN:
		if (sc->sc_state != SPC_CONNECTED)
			break;
		SPC_MISC(("datain  "));
		n = spc_datain_pio(sc, sc->sc_dp, sc->sc_dleft);
		sc->sc_dp += n;
		sc->sc_dleft -= n;
		sc->sc_prevphase = PH_DATAIN;
		goto loop;

	case PH_STAT:
		if (sc->sc_state != SPC_CONNECTED)
			break;
		SPC_ASSERT(sc->sc_nexus != NULL);
		acb = sc->sc_nexus;
		/*acb->target_stat = DREG;*/
		spc_datain_pio(sc, &acb->target_stat, 1);
		SPC_MISC(("target_stat=0x%02x  ", acb->target_stat));
		sc->sc_prevphase = PH_STAT;
		goto loop;
	}

	printf("%s: unexpected bus phase; resetting\n", sc->sc_dev.dv_xname);
	SPC_BREAK();
reset:
	spc_init(sc);
	return 1;

finish:
	untimeout(spc_timeout, acb);
	INTS = ints;
	ints = 0;
	spc_done(sc, acb);
	goto out;

sched:
	sc->sc_state = SPC_IDLE;
	spc_sched(sc);
	goto out;

out:
	if (ints)
		INTS = ints;
	SCTL |= SCTL_INTR_ENAB;
	return 1;
}

void
spc_abort(sc, acb)
	struct spc_softc *sc;
	struct spc_acb *acb;
{

	/* 2 secs for the abort */
	acb->timeout = SPC_ABORT_TIMEOUT;
	acb->flags |= ACB_ABORT;

	if (acb == sc->sc_nexus) {
		/*
		 * If we're still selecting, the message will be scheduled
		 * after selection is complete.
		 */
		if (sc->sc_state == SPC_CONNECTED)
			spc_sched_msgout(sc, SEND_ABORT);
	} else {
		spc_dequeue(sc, acb);
		TAILQ_INSERT_HEAD(&sc->ready_list, acb, chain);
		if (sc->sc_state == SPC_IDLE)
			spc_sched(sc);
	}
}

void
spc_timeout(arg)
	void *arg;
{
	struct spc_acb *acb = arg;
	struct scsipi_xfer *xs = acb->xs;
	struct scsipi_link *sc_link = xs->sc_link;
	struct spc_softc *sc = sc_link->adapter_softc;
	int s;

	scsi_print_addr(sc_link);
	printf("timed out");

	s = splbio();

	if (acb->flags & ACB_ABORT) {
		/* abort timed out */
		printf(" AGAIN\n");
		/* XXX Must reset! */
	} else {
		/* abort the operation that has timed out */
		printf("\n");
		acb->xs->error = XS_TIMEOUT;
		spc_abort(sc, acb);
	}

	splx(s);
}

#ifdef SPC_DEBUG
/*
 * The following functions are mostly used for debugging purposes, either
 * directly called from the driver or from the kernel debugger.
 */

void
spc_show_scsi_cmd(acb)
	struct spc_acb *acb;
{
	u_char  *b = (u_char *)&acb->scsi_cmd;
	struct scsipi_link *sc_link = acb->xs->sc_link;
	int i;

	scsi_print_addr(sc_link);
	if ((acb->xs->flags & SCSI_RESET) == 0) {
		for (i = 0; i < acb->scsi_cmd_length; i++) {
			if (i)
				printf(",");
			printf("%x", b[i]);
		}
		printf("\n");
	} else
		printf("RESET\n");
}

void
spc_print_acb(acb)
	struct spc_acb *acb;
{

	printf("acb@%x xs=%x flags=%x", acb, acb->xs, acb->flags);
	printf(" dp=%x dleft=%d target_stat=%x\n",
	    (long)acb->data_addr, acb->data_length, acb->target_stat);
	spc_show_scsi_cmd(acb);
}

void
spc_print_active_acb()
{
	struct spc_acb *acb;
	struct spc_softc *sc = spc_cd.cd_devs[0]; /* XXX */

	printf("ready list:\n");
	for (acb = sc->ready_list.tqh_first; acb != NULL;
	    acb = acb->chain.tqe_next)
		spc_print_acb(acb);
	printf("nexus:\n");
	if (sc->sc_nexus != NULL)
		spc_print_acb(sc->sc_nexus);
	printf("nexus list:\n");
	for (acb = sc->nexus_list.tqh_first; acb != NULL;
	    acb = acb->chain.tqe_next)
		spc_print_acb(acb);
}

void
spc_dump_driver(sc)
	struct spc_softc *sc;
{
	struct spc_tinfo *ti;
	int i;

	printf("nexus=%x prevphase=%x\n", sc->sc_nexus, sc->sc_prevphase);
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
