/*
 * ST01/02, Future Domain TMC-885, TMC-950 SCSI driver
 *
 * Copyright 1994, Charles Hannum (mycroft@ai.mit.edu)
 * Copyright 1994, Kent Palmkvist (kentp@isy.liu.se)
 * Copyright 1994, Robert Knier (rknier@qgraph.com) 
 * Copyright 1992, 1994 Drew Eckhardt (drew@colorado.edu)
 * Copyright 1994, Julian Elischer (julian@tfs.com)
 *
 * Others that has contributed by example code is
 * 		Glen Overby (overby@cray.com)
 *		Tatu Yllnen
 *		Brian E Litzinger
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
 * THIS SOFTWARE IS PROVIDED BY THE DEVELOPERS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE DEVELOPERS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * kentp  940307 alpha version based on newscsi-03 version of Julians SCSI-code
 * kentp  940314 Added possibility to not use messages
 * rknier 940331 Added fast transfer code 
 * rknier 940407 Added assembler coded data transfers 
 */

/*
 * What should really be done:
 * 
 * Add missing tests for timeouts
 * Restructure interrupt enable/disable code (runs to long with int disabled)
 * Find bug? giving problem with tape status
 * Add code to handle Future Domain 840, 841, 880 and 881
 * adjust timeouts (startup is very slow)
 * add code to use tagged commands in SCSI2
 * Add code to handle slow devices better (sleep if device not disconnecting)
 * Fix unnecessary interrupts
 */

/*
 * Note to users trying to share a disk between DOS and unix:
 * The ST01/02 is a translating host-adapter. It is not giving DOS
 * the same number of heads/tracks/sectors as specified by the disk.
 * It is therefore important to look at what numbers DOS thinks the
 * disk has. Use these to disklabel your disk in an appropriate manner
 */
 
#include <sys/types.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/device.h>

#include <machine/pio.h>

#include <i386/isa/isavar.h>
#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>

#define	SEA_SCB_MAX	32	/* allow maximally 8 scsi control blocks */
#define SCB_TABLE_SIZE	8	/* start with 8 scb entries in table */
#define BLOCK_SIZE	512	/* size of READ/WRITE areas on SCSI card */

/*
 * defining PARITY causes parity data to be checked
 */
#define	PARITY

/*
 * defining SEA_BLINDTRANSFER will make DATA IN and DATA OUT to be done with
 * blind transfers, i.e. no check is done for scsi phase changes. This will
 * result in data loss if the scsi device does not send its data using
 * BLOCK_SIZE bytes at a time.
 * If SEA_BLINDTRANSFER defined and SEA_ASSEMBLER also defined will result in
 * the use of blind transfers coded in assembler. SEA_ASSEMBLER is no good
 * without SEA_BLINDTRANSFER defined.
 */
#define	SEA_BLINDTRANSFER	/* do blind transfers */
#define	SEA_ASSEMBLER		/* Use assembly code for fast transfers */

/*
 * defining SEA_NOMSGS causes messages not to be used (thereby disabling
 * disconnects)
 */
#undef	SEA_NOMSGS

/*
 * defining SEA_NODATAOUT makes dataout phase being aborted
 */
#undef	SEA_NODATAOUT

/*
 * defining SEA_SENSEFIRST make REQUEST_SENSE opcode to be placed first
 */
#undef	SEA_SENSEFIRST

/* Debugging definitions. Should not be used unless you want a lot of
   printouts even under normal conditions */

#undef	SEA_DEBUGQUEUE		/* Display info about queue-lengths */

/******************************* board definitions **************************/
/*
 * CONTROL defines
 */
#define CMD_RST		0x01		/* scsi reset */
#define CMD_SEL		0x02		/* scsi select */
#define CMD_BSY		0x04		/* scsi busy */
#define	CMD_ATTN	0x08		/* scsi attention */
#define CMD_START_ARB	0x10		/* start arbitration bit */
#define	CMD_EN_PARITY	0x20		/* enable scsi parity generation */
#define CMD_INTR	0x40		/* enable scsi interrupts */
#define CMD_DRVR_ENABLE	0x80		/* scsi enable */

/*
 * STATUS
 */
#define STAT_BSY	0x01		/* scsi busy */
#define STAT_MSG	0x02		/* scsi msg */
#define STAT_IO		0x04		/* scsi I/O */
#define STAT_CD		0x08		/* scsi C/D */
#define STAT_REQ	0x10		/* scsi req */
#define STAT_SEL	0x20		/* scsi select */
#define STAT_PARITY	0x40		/* parity error bit */
#define STAT_ARB_CMPL	0x80		/* arbitration complete bit */

/*
 * REQUESTS
 */
#define REQ_MASK	(STAT_CD | STAT_IO | STAT_MSG)
#define REQ_DATAOUT	0
#define REQ_DATAIN	STAT_IO
#define REQ_CMDOUT	STAT_CD
#define REQ_STATIN	(STAT_CD | STAT_IO)
#define REQ_MSGOUT	(STAT_MSG | STAT_CD)
#define REQ_MSGIN	(STAT_MSG | STAT_CD | STAT_IO)

#define REQ_UNKNOWN	0xff

#define SEA_RAMOFFSET	0x00001800

#ifdef PARITY
#define BASE_CMD	(CMD_INTR | CMD_EN_PARITY)
#else
#define BASE_CMD	(CMD_INTR)
#endif

#define	SEAGATE		1
#define FDOMAIN		2

/******************************************************************************
 *	This should be placed in a more generic file (presume in /sys/scsi)
 *	Message codes:
 */
#define MSG_ABORT		0x06
#define MSG_NOP			0x08
#define MSG_COMMAND_COMPLETE	0x00
#define	MSG_DISCONNECT		0x04
#define MSG_IDENTIFY		0x80
#define MSG_BUS_DEV_RESET	0x0c
#define	MSG_MESSAGE_REJECT	0x07
#define MSG_SAVE_POINTERS	0x02
#define MSG_RESTORE_POINTERS	0x03
/******************************************************************************/

#define IDENTIFY(can_disconnect, lun) \
	(MSG_IDENTIFY | ((can_disconnect) ? 0x40 : 0) | ((lun) & 0x07))

/* scsi control block used to keep info about a scsi command */ 
struct sea_scb {
        u_char *data;			/* position in data buffer so far */
	int32 datalen;			/* bytes remaining to transfer */
	TAILQ_ENTRY(sea_scb) chain;
	struct scsi_xfer *xfer;		/* the scsi_xfer for this cmd */
	int flags;			/* status of the instruction */
#define	SCB_FREE	0
#define	SCB_ACTIVE	1
#define SCB_ABORTED	2
#define SCB_TIMEOUT	4
#define SCB_ERROR	8
};

/*
 * data structure describing current status of the scsi bus. One for each
 * controller card.
 */
struct sea_softc {
	struct device sc_dev;
	struct isadev sc_id;
	struct intrhand sc_ih;

	struct scsi_link sc_link;	/* struct connecting different data */
	struct sea_scb *connected;	/* currently connected command */
	TAILQ_HEAD(chainhead, sea_scb)
	    issue_queue, disconnected_queue, free_queue;
	int numscbs;			/* number of scsi control blocks */
	struct sea_scb scbs[SCB_TABLE_SIZE];

	caddr_t	maddr;			/* Base address for card */
	caddr_t	maddr_cr_sr;		/* Address of control and status reg */
	caddr_t	maddr_dr;		/* Address of data register */
	int type;			/* FDOMAIN or SEAGATE */
	int our_id;			/* our scsi id */
	u_char our_id_mask;
	volatile u_char busy[8];	/* index=target, bit=lun, Keep track of
					   busy luns at device target */
};

/* flag showing if main routine is running. */
static volatile int main_running = 0;

#define	STATUS	(*(volatile u_char *)sea->maddr_cr_sr)
#define CONTROL	STATUS
#define DATA	(*(volatile u_char *)sea->maddr_dr)

/*
 * These are "special" values for the tag parameter passed to sea_select
 * Not implemented right now.
 */
#define TAG_NEXT	-1	/* Use next free tag */
#define TAG_NONE	-2	/*
				 * Establish I_T_L nexus instead of I_T_L_Q
				 * even on SCSI-II devices.
				 */

typedef struct {
	char *signature;
	int offset, length;
	int type;
} BiosSignature;

/*
 * Signatures for automatic recognition of board type
 */
static const BiosSignature signatures[] = {
{"ST01 v1.7  (C) Copyright 1987 Seagate", 15, 37, SEAGATE},
{"SCSI BIOS 2.00  (C) Copyright 1987 Seagate", 15, 40, SEAGATE},

/*
 * The following two lines are NOT mistakes. One detects ROM revision
 * 3.0.0, the other 3.2. Since seagate has only one type of SCSI adapter,
 * and this is not going to change, the "SEAGATE" and "SCSI" together
 * are probably "good enough"
 */
{"SEAGATE SCSI BIOS ", 16, 17, SEAGATE},
{"SEAGATE SCSI BIOS ", 17, 17, SEAGATE},

/*
 * However, future domain makes several incompatible SCSI boards, so specific
 * signatures must be used.
 */
{"FUTURE DOMAIN CORP. (C) 1986-1989 V5.0C2/14/89", 5, 45, FDOMAIN},
{"FUTURE DOMAIN CORP. (C) 1986-1989 V6.0A7/28/89", 5, 46, FDOMAIN},
{"FUTURE DOMAIN CORP. (C) 1986-1990 V6.0105/31/90",5, 47, FDOMAIN},
{"FUTURE DOMAIN CORP. (C) 1986-1990 V6.0209/18/90",5, 47, FDOMAIN},
{"FUTURE DOMAIN CORP. (C) 1986-1990 V7.009/18/90", 5, 46, FDOMAIN},
{"FUTURE DOMAIN CORP. (C) 1992 V8.00.004/02/92",   5, 44, FDOMAIN},
{"FUTURE DOMAIN TMC-950",			   5, 21, FDOMAIN},
};

#define	nsignatures	(sizeof(signatures) / sizeof(signatures[0]))

static const char *bases[] = {
	(char *) 0xc8000, (char *) 0xca000, (char *) 0xcc000,
	(char *) 0xce000, (char *) 0xdc000, (char *) 0xde000
};

#define	nbases		(sizeof(bases) / sizeof(bases[0]))

int seaintr __P((struct sea_softc *));
int sea_scsi_cmd __P((struct scsi_xfer *xs));
void sea_timeout __P((void *));
void seaminphys __P((struct buf *));
void sea_done __P((struct sea_softc *, struct sea_scb *));
u_int sea_adapter_info __P((struct sea_softc *));
struct sea_scb *sea_get_scb __P((struct sea_softc *, int));
void sea_free_scb __P((struct sea_softc *, struct sea_scb *, int));
static void sea_main __P((void));
static void sea_information_transfer __P((struct sea_softc *));
int sea_poll __P((struct sea_softc *, struct scsi_xfer *, struct sea_scb *));
void sea_init __P((struct sea_softc *));
void sea_send_scb __P((struct sea_softc *sea, struct sea_scb *scb));
void sea_reselect __P((struct sea_softc *sea));
int sea_select __P((struct sea_softc *sea, struct sea_scb *scb));
int sea_transfer_pio __P((struct sea_softc *sea, u_char *phase,
    int32 *count, u_char **data));
int sea_abort __P((struct sea_softc *, struct sea_scb *scb));

struct scsi_adapter sea_switch = {
	sea_scsi_cmd,
	seaminphys,
	0,
	0,
	sea_adapter_info,
	"sea",
};

/* the below structure is so we have a default dev struct for our link struct */
struct scsi_device sea_dev = {
	NULL,		/* use default error handler */
	NULL,		/* have a queue, served by this */
	NULL,		/* have no async handler */
	NULL,		/* Use default 'done' routine */
	"sea",
	0,
};

int seaprobe();
void seaattach();

struct cfdriver seacd = {
	NULL, "sea", seaprobe, seaattach, DV_DULL, sizeof(struct sea_softc)
};

#ifdef SEA_DEBUGQUEUE
void
sea_queue_length(sea)
	struct sea_softc *sea;
{
	struct sea_scb *scb;
	int connected, issued, disconnected;

	connected = sea->connected ? 1 : 0;
	for (scb = sea->issue_queue.tqh_first, issued = 0; scb;
	    scb = scb->chain.tqe_next, issued++);
	for (scb = sea->disconnected_queue.tqh_first, disconnected = 0; scb;
	    scb = scb->chain.tqe_next, disconnected++);
	printf("%s: length: %d/%d/%d\n", sea->sc_dev.dv_xname, connected,
	    issued, disconnected);
}
#endif

/*
 * Check if the device can be found at the port given and if so, detect the
 * type the type of board.  Set it up ready for further work. Takes the isa_dev
 * structure from autoconf as an argument.
 * Returns 1 if card recognized, 0 if errors.
 */
int
seaprobe(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct sea_softc *sea = (void *)self;
	struct isa_attach_args *ia = aux;
	int i;

	/*
	 * Could try to find a board by looking through all possible addresses.
	 * This is not done the right way now, because I have not found a way
	 * to get a boards virtual memory address given its physical.  There is
	 * a function that returns the physical address for a given virtual
	 * address, but not the other way around.
	 */

	if (ia->ia_maddr == 0) {
		/* XXX */
		return 0;
	} else
		sea->maddr = ia->ia_maddr;
	
	/* check board type */	/* No way to define this through config */
	for (i = 0; i < nsignatures; i++)
		if (!memcmp(sea->maddr + signatures[i].offset,
		    signatures[i].signature, signatures[i].length)) {
			sea->type = signatures[i].type;
			break;
		}

	/* Find controller and data memory addresses */
	switch (sea->type) {
	case SEAGATE:
		sea->maddr_cr_sr =
		    (void *) (((u_char *)sea->maddr) + 0x1a00);
		sea->maddr_dr =
		    (void *) (((u_char *)sea->maddr) + 0x1c00);
		break;
	case FDOMAIN:
		sea->maddr_cr_sr =
		    (void *) (((u_char *)sea->maddr) + 0x1c00);
		sea->maddr_dr =
		    (void *) (((u_char *)sea->maddr) + 0x1e00);
		break;
	default:
		printf("%s: board type unknown at address 0x%lx\n",
		    sea->sc_dev.dv_xname, sea->maddr);
		return 0;
	}

	/* Test controller RAM (works the same way on future domain cards?) */
	*((u_char *)sea->maddr + SEA_RAMOFFSET) = 0xa5;
	*((u_char *)sea->maddr + SEA_RAMOFFSET + 1) = 0x5a;

	if ((*((u_char *)sea->maddr + SEA_RAMOFFSET) != 0xa5) ||
	    (*((u_char *)sea->maddr + SEA_RAMOFFSET + 1) != 0x5a)) {
		printf("%s: board RAM failure\n", sea->sc_dev.dv_xname);
		return 0;
	}
  
	ia->ia_drq = DRQUNK;
	ia->ia_msize = 0x2000;
	ia->ia_iosize = 0;
	return 1;
}

seaprint()
{

}

/*
 * Attach all sub-devices we can find
 */
void
seaattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	struct sea_softc *sea = (void *)self;

	sea_init(sea);

	/*
	 * fill in the prototype scsi_link.
	 */
	sea->sc_link.adapter_softc = sea;
	sea->sc_link.adapter_targ = sea->our_id;
	sea->sc_link.adapter = &sea_switch;
	sea->sc_link.device = &sea_dev;
  
	printf("\n");

#ifdef NEWCONFIG
	isa_establish(&sea->sc_id, &sea->sc_deV);
#endif
	sea->sc_ih.ih_fun = seaintr;
	sea->sc_ih.ih_arg = sea;
	sea->sc_ih.ih_level = IPL_BIO;
	intr_establish(ia->ia_irq, &sea->sc_ih);

	/*
	 * ask the adapter what subunits are present
	 */
	config_found(self, &sea->sc_link, seaprint);
}

/*
 * Return some information to the caller about
 * the adapter and its capabilities
 */
u_int
sea_adapter_info(sea)
	struct sea_softc *sea;
{

	return 1;	/* 1 outstanding request at a time per device */
}

/*
 * Catch an interrupt from the adaptor
 */
int
seaintr(sea)
	struct sea_softc *sea;
{

#ifdef DEBUG	/* extra overhead, and only needed for intr debugging */
	if ((STATUS & STAT_PARITY) == 0 &&
	    (STATUS & (STAT_SEL | STAT_IO)) != (STAT_SEL | STAT_IO))
		return 0;
#endif

loop:
	/* dispatch to appropriate routine if found and done=0 */
	/* should check to see that this card really caused the interrupt */

	if (STATUS & STAT_PARITY) {
		/* Parity error interrupt */
		printf("%s: parity error\n", sea->sc_dev.dv_xname);
		return 1;
	}

	if ((STATUS & (STAT_SEL | STAT_IO)) == (STAT_SEL | STAT_IO)) {
		/* Reselect interrupt */
		sea_reselect(sea);
		if (!main_running)
			sea_main();
		goto loop;
	}

	return 1;
}

/*
 * Setup data structures, and reset the board and the SCSI bus.
 */
void
sea_init(sea)
	struct sea_softc *sea;
{
	int i;
  
	/* Reset the scsi bus (I don't know if this is needed */
	CONTROL = BASE_CMD | CMD_DRVR_ENABLE | CMD_RST;
	delay(25);	/* hold reset for at least 25 microseconds */
	CONTROL = BASE_CMD;
	delay(10); 	/* wait a Bus Clear Delay (800 ns + bus free delay (800 ns) */

	/* Set our id (don't know anything about this) */
	switch (sea->type) {
	case SEAGATE:
		sea->our_id = 7;
		break;
	case FDOMAIN:
		sea->our_id = 6;
		break;
	}
	sea->our_id_mask = 1 << sea->our_id;

	/* init fields used by our routines */
	sea->connected = 0;
	TAILQ_INIT(&sea->issue_queue);
	TAILQ_INIT(&sea->disconnected_queue);
	TAILQ_INIT(&sea->free_queue);
	for (i = 0; i < 8; i++)
		sea->busy[i] = 0x00;

	/* link up the free list of scbs */
	sea->numscbs = SCB_TABLE_SIZE;
	for (i = 0; i < SCB_TABLE_SIZE; i++) {
		TAILQ_INSERT_TAIL(&sea->free_queue, &sea->scbs[i], chain);
	}
}

void
seaminphys(bp)
	struct buf *bp;
{

	/* No need for a max since we're doing PIO. */
}

/*
 * start a scsi operation given the command and the data address. Also needs
 * the unit, target and lu.
 */
int
sea_scsi_cmd(xs)
	struct scsi_xfer *xs;
{
	struct scsi_link *sc_link = xs->sc_link;
	struct sea_softc *sea = sc_link->adapter_softc;
	struct sea_scb *scb;
	int flags;

	SC_DEBUG(sc_link, SDEV_DB2, ("sea_scsi_cmd\n"));

	flags = xs->flags;
	if (xs->bp)
		flags |= SCSI_NOSLEEP;
	if (flags & ITSDONE) {
		printf("%s: already done?", sea->sc_dev.dv_xname);
		xs->flags &= ~ITSDONE;
	}
	if (!(flags & INUSE)) {
		printf("%s: not in use?", sea->sc_dev.dv_xname);
		xs->flags |= INUSE;
	}
	if (!(scb = sea_get_scb(sea, flags))) {
		xs->error = XS_DRIVER_STUFFUP;
		return TRY_AGAIN_LATER;
	}

	scb->xfer = xs;

	if (flags & SCSI_RESET) {
		/*
		 * Try to send a reset command to the card.
		 * XXX Not implemented.
		 */
		printf("%s: resetting\n", sea->sc_dev.dv_xname);
		xs->error = XS_DRIVER_STUFFUP;
		return HAD_ERROR;
	}

	/*
	 * Put all the arguments for the xfer in the scb
	 */
	scb->datalen = xs->datalen;
	scb->data = xs->data;

#ifdef SEA_DEBUGQUEUE
	sea_queue_length(sea);
#endif

	/*
	 * Usually return SUCCESSFULLY QUEUED
	 */
	if (!(flags & SCSI_NOMASK)) {
		int s = splbio();
		sea_send_scb(sea, scb);
		if (!(xs->flags & ITSDONE))
			timeout(sea_timeout, scb, (xs->timeout * hz) / 1000);
		splx(s);
		return SUCCESSFULLY_QUEUED;
	}

	/*
	 * If we can't use interrupts, poll on completion
	 */
	sea_send_scb(sea, scb);
	/* XXX Check ITSDONE? */
	return sea_poll(sea, xs, scb);
}

/*
 * Get a free scb. If there are none, see if we can allocate a new one.  If so,
 * put it in the hash table too; otherwise return an error or sleep.
 */
struct sea_scb *
sea_get_scb(sea, flags)
	struct sea_softc *sea;
	int flags;
{
	int s;
	struct sea_scb *scb;

	if (!(flags & SCSI_NOMASK))
		s = splbio();

	/*
	 * If we can and have to, sleep waiting for one to come free
	 * but only if we can't allocate a new one.
	 */
	for (;;) {
		scb = sea->free_queue.tqh_first;
		if (scb) {
			TAILQ_REMOVE(&sea->free_queue, scb, chain);
			break;
		}
		if (sea->numscbs < SEA_SCB_MAX) {
			printf("malloced new scbs\n");
			if (scb = (void *) malloc(sizeof(struct sea_scb),
			    M_TEMP, M_NOWAIT)) {
				bzero(scb, sizeof(struct sea_scb));
				sea->numscbs++;
				scb->flags = SCB_ACTIVE;
			} else
				printf("%s: can't malloc scb\n",
				    sea->sc_dev.dv_xname);
			break;
		} else {
			if (!(flags & SCSI_NOSLEEP))
				tsleep((caddr_t)&sea->free_queue, PRIBIO,
				    "seascb", 0);
		}
	}
	if (!(flags & SCSI_NOMASK))
		splx(s);

	return scb;
}

/*
 * Try to send this command to the board. Because this board does not use any
 * mailboxes, this routine simply adds the command to the queue held by the
 * sea_softc structure.
 * A check is done to see if the command contains a REQUEST_SENSE command, and
 * if so the command is put first in the queue, otherwise the command is added
 * to the end of the queue. ?? Not correct ??
 */
void
sea_send_scb(sea, scb)
	struct sea_softc *sea;
	struct sea_scb *scb;
{

#ifdef SEA_SENSEFIRST
	if (scb->xfer->cmd->opcode == (u_char) REQUEST_SENSE) {
		TAILQ_INSERT_HEAD(&sea->issue_queue, scb, chain);
	} else {
		TAILQ_INSERT_TAIL(&sea->issue_queue, scb, chain);
	}
#else
	TAILQ_INSERT_TAIL(&sea->issue_queue, scb, chain);
#endif
	/* Try to do some work on the card */
	if (!main_running)
		sea_main();
}

/*
 * Coroutine that runs as long as more work can be done on the seagate host
 * adapter in a system.  Both sea_scsi_cmd and sea_intr will try to start it in
 * case it is not running.
 */
void
sea_main()
{
	struct sea_softc *sea;
	struct sea_scb *scb;
	int done;
	int unit;
	int s;

	main_running = 1;

	/*
	 * This should not be run with interrupts disabled, but use the splx
	 * code instead.
	 */
loop:
	done = 1;
	for (unit = 0; unit < seacd.cd_ndevs; unit++) {
		sea = seacd.cd_devs[unit];
		if (!sea)
			continue;
		s = splbio();
		if (!sea->connected) {
			/*
			 * Search through the issue_queue for a command
			 * destined for a target that's not busy.
			 */
			for (scb = sea->issue_queue.tqh_first; scb;
			    scb = scb->chain.tqe_next) {
				if (!(sea->busy[scb->xfer->sc_link->target] &
				    (1 << scb->xfer->sc_link->lun))) {
					TAILQ_REMOVE(&sea->issue_queue, scb,
					    chain);
	    
					/* Re-enable interrupts. */
					splx(s);

					/*
					 * Attempt to establish an I_T_L nexus.
					 * On success, sea->connected is set.
					 * On failure, we must add the command
					 * back to the issue queue so we can
					 * keep trying.
					 */

					/*
					 * REQUEST_SENSE commands are issued
					 * without tagged queueing, even on
					 * SCSI-II devices because the
					 * contingent alligence condition
					 * exists for the entire unit.
					 */

					/*
					 * First check that if any device has
					 * tried a reconnect while we have done
					 * other things with interrupts
					 * disabled.
					 */

					if ((STATUS & (STAT_SEL | STAT_IO)) ==
					    (STAT_SEL | STAT_IO)) {
						sea_reselect(sea);
						break;
					}
					if (sea_select(sea, scb)) {
						s = splbio();
						TAILQ_INSERT_HEAD(&sea->issue_queue,
						    scb, chain);
						splx(s);
					} else
						break;
				} /* if target/lun is not busy */
			} /* for scb */
		} /* if (!sea->connected) */
      
		splx(s);
		if (sea->connected) {	/* we are connected. Do the task */
			sea_information_transfer(sea);
			done = 0;
		} else
			break;
	} /* for instance */

	if (!done)
		goto loop;

	main_running = 0;
}

void
sea_free_scb(sea, scb, flags)
	struct sea_softc *sea;
	struct sea_scb *scb;
	int flags;
{
	int s;

	if (!(flags & SCSI_NOMASK))
		s = splbio();

	TAILQ_INSERT_HEAD(&sea->free_queue, scb, chain);
	scb->flags = SCB_FREE;
	/*
	 * If there were none, wake anybody waiting for one to come free,
	 * starting with queued entries.
	 */
	if (!scb->chain.tqe_next)
		wakeup((caddr_t)&sea->free_queue);

	if (!(flags & SCSI_NOMASK))
		splx(s);
}

void
sea_timeout(arg)
	void *arg;
{
	int s = splbio();
	struct sea_scb *scb = arg;
	struct sea_softc *sea;

	sea = scb->xfer->sc_link->adapter_softc;
	sc_print_addr(scb->xfer->sc_link);
	printf("timed out");

	/*
	 * If it has been through before, then
	 * a previous abort has failed, don't
	 * try abort again
	 */
	if (scb->flags & SCB_ABORTED) {
		printf(" AGAIN\n");
	 	scb->xfer->retries = 0;
		scb->flags |= SCB_ABORTED;
		sea_done(sea, scb);
	} else {
		printf("\n");
		sea_abort(sea, scb);
		timeout(sea_timeout, scb, 2 * hz);
		scb->flags |= SCB_ABORTED;
	}
	splx(s);
}
 
void
sea_reselect(sea)
	struct sea_softc *sea;
{
	u_char target_mask;
	int i;
	u_char lun, phase;
	u_char msg[3];
	int32 len;
	u_char *data;
	struct sea_scb *scb;
	int abort = 0;
  
	if (!((target_mask = STATUS) & STAT_SEL)) {
		printf("%s: wrong state 0x%x\n", sea->sc_dev.dv_xname,
		    target_mask);
		return;
	}

	/* wait for a device to win the reselection phase */
	/* signals this by asserting the I/O signal */
	for (i = 10; i && (STATUS & (STAT_SEL | STAT_IO | STAT_BSY)) !=
	    (STAT_SEL | STAT_IO | 0); i--);
	/* !! Check for timeout here */
	/* the data bus contains original initiator id ORed with target id */
	target_mask = DATA;
	/* see that we really are the initiator */
	if (!(target_mask & sea->our_id_mask)) {
		printf("%s: polled reselection was not for me: 0x%x\n",
		    sea->sc_dev.dv_xname, target_mask);
		return;
	}
	/* find target who won */
	target_mask &= ~sea->our_id_mask;
	/* host responds by asserting the BSY signal */
	CONTROL = BASE_CMD | CMD_DRVR_ENABLE | CMD_BSY;
	/* target should respond by deasserting the SEL signal */
	for (i = 50000; i && (STATUS & STAT_SEL); i++);
	/* remove the busy status */
	CONTROL = BASE_CMD | CMD_DRVR_ENABLE;
	/* we are connected. Now we wait for the MSGIN condition */
	for (i = 50000; i && !(STATUS & STAT_REQ); i--);
	/* !! Add timeout check here */
	/* hope we get an IDENTIFY message */
	len = 3;
	data = msg;
	phase = REQ_MSGIN;
	sea_transfer_pio(sea, &phase, &len, &data); 

	if (!(msg[0] & 0x80)) {
		printf("%s: expecting IDENTIFY message, got 0x%x\n",
		    sea->sc_dev.dv_xname, msg[0]);
		abort = 1;
	} else {
		lun = msg[0] & 0x07;

		/*
		 * Find the command corresponding to the I_T_L or I_T_L_Q nexus
		 * we just reestablished, and remove it from the disconnected
		 * queue.
		 */
		for (scb = sea->disconnected_queue.tqh_first; scb;
		    scb = scb->chain.tqe_next)
			if (target_mask == (1 << scb->xfer->sc_link->target) &&
			    lun == scb->xfer->sc_link->lun) {
				TAILQ_REMOVE(&sea->disconnected_queue, scb,
				    chain);
				break;
			}
		if (!scb) {
			printf("%s: target %02x lun %d not disconnected\n",
			    sea->sc_dev.dv_xname, target_mask, lun);
			/*
			 * Since we have an established nexus that we can't do
			 * anything with, we must abort it.
			 */
			abort = 1;
		}
	}

	if (abort) {
		msg[0] = MSG_ABORT;
		len = 1;
		data = msg;
		phase = REQ_MSGOUT;
		CONTROL = BASE_CMD | CMD_ATTN;
		sea_transfer_pio(sea, &phase, &len, &data);
	} else
		sea->connected = scb;

	return;
}

/*
 * Transfer data in given phase using polled I/O.
 */
int
sea_transfer_pio(sea, phase, count, data)
	struct sea_softc *sea;
	u_char *phase;
	int32 *count;
	u_char **data;
{
	register u_char p = *phase, tmp;
	register int c = *count;
	register u_char *d = *data;
	int timeout;

	do {
		/*
		 * Wait for assertion of REQ, after which the phase bits will
		 * be valid.
		 */
		for (timeout = 0; timeout < 5000000L; timeout++)
			if ((tmp = STATUS) & STAT_REQ)
				break;
		if (!(tmp & STAT_REQ)) {
			printf("%s: timeout waiting for STAT_REQ\n",
			    sea->sc_dev.dv_xname);
			break;
		}

		/*
		 * Check for phase mismatch.  Reached if the target decides
		 * that it has finished the transfer.
		 */
		if ((tmp & REQ_MASK) != p)
			break;

		/* Do actual transfer from SCSI bus to/from memory. */
		if (!(p & STAT_IO))
			DATA = *d;
		else
			*d = DATA;
		++d;

		/*
		 * The SCSI standard suggests that in MSGOUT phase, the
		 * initiator should drop ATN on the last byte of the message
		 * phase after REQ has been asserted for the handshake but
		 * before the initiator raises ACK.
		 * Don't know how to accomplish this on the ST01/02.
		 */

#if 0
		/*
		 * XXX
		 * The st01 code doesn't wait for STAT_REQ to be deasserted.
		 * Is this ok?
		 */
		for (timeout = 0; timeout < 200000L; timeout++)
			if (!(STATUS & STAT_REQ))
				break;
		if (STATUS & STAT_REQ)
			printf("%s: timeout on wait for !STAT_REQ",
			    sea->sc_dev.dv_xname);
#endif
	} while (--c);

	*count = c;
	*data = d;
	tmp = STATUS;
	if (tmp & STAT_REQ)
		*phase = tmp & REQ_MASK;
	else
		*phase = REQ_UNKNOWN;

	if (c && (*phase != p))
		return -1;
	return 0;
}

/*
 * Establish I_T_L or I_T_L_Q nexus for new or existing command including
 * ARBITRATION, SELECTION, and initial message out for IDENTIFY and queue
 * messages.  Return -1 if selection could not execute for some reason, 0 if
 * selection succeded or failed because the target did not respond.
 */
int
sea_select(sea, scb)
	struct sea_softc *sea;
	struct sea_scb *scb;
{
	u_char msg[3], phase;
	u_char *data;
	int32 len;
	int timeout;

	CONTROL = BASE_CMD;
	DATA = sea->our_id_mask;
	CONTROL = (BASE_CMD & ~CMD_INTR) | CMD_START_ARB;

	/* wait for arbitration to complete */
	for (timeout = 0; timeout < 3000000L; timeout++)
		if (STATUS & STAT_ARB_CMPL)
			break;
	if (!(STATUS & STAT_ARB_CMPL)) {
		if (STATUS & STAT_SEL) {
			printf("%s: arbitration lost\n", sea->sc_dev.dv_xname);
			scb->flags |= SCB_ERROR;
		} else {
			printf("%s: arbitration timeout\n",
			    sea->sc_dev.dv_xname);
			scb->flags |= SCB_TIMEOUT;
		}
		CONTROL = BASE_CMD;
		return -1;
	}

	delay(2);
	DATA = (u_char)((1 << scb->xfer->sc_link->target) | sea->our_id_mask);
	CONTROL =
#ifdef SEA_NOMSGS
	    (BASE_CMD & ~CMD_INTR) | CMD_DRVR_ENABLE | CMD_SEL;
#else
	    (BASE_CMD & ~CMD_INTR) | CMD_DRVR_ENABLE | CMD_SEL | CMD_ATTN;
#endif
	delay(1); 

	/* wait for a bsy from target */
	for (timeout = 0; timeout < 2000000L; timeout++)
		if (STATUS & STAT_BSY)
			break;
	if (!(STATUS & STAT_BSY)) {
		/* should return some error to the higher level driver */
		CONTROL = BASE_CMD;
		scb->flags |= SCB_TIMEOUT;
		return 0;
	}

	/* Try to make the target to take a message from us */
#ifdef SEA_NOMSGS
	CONTROL = (BASE_CMD & ~CMD_INTR) | CMD_DRVR_ENABLE;
#else
	CONTROL = (BASE_CMD & ~CMD_INTR) | CMD_DRVR_ENABLE | CMD_ATTN;
#endif
	delay(1);
  
	/* should start a msg_out phase */
	for (timeout = 0; timeout < 2000000L; timeout++)
		if (STATUS & STAT_REQ)
			break;
	CONTROL = BASE_CMD | CMD_DRVR_ENABLE;
	if (!(STATUS & STAT_REQ)) {
		/*
		 * This should not be taken as an error, but more like an
		 * unsupported feature!  Should set a flag indicating that the
		 * target don't support messages, and continue without failure.
		 * (THIS IS NOT AN ERROR!)
		 */
	} else {
		msg[0] = IDENTIFY(1, scb->xfer->sc_link->lun);
		len = 1;
		data = msg;
		phase = REQ_MSGOUT;
		/* Should do test on result of sea_transfer_pio(). */
		sea_transfer_pio(sea, &phase, &len, &data);
	}
	if (!(STATUS & STAT_BSY))
		printf("%s: after successful arbitrate: no STAT_BSY!\n",
		    sea->sc_dev.dv_xname);
  
	sea->connected = scb;
	sea->busy[scb->xfer->sc_link->target] |= 1 << scb->xfer->sc_link->lun;
	/* This assignment should depend on possibility to send a message to target. */
	CONTROL = BASE_CMD | CMD_DRVR_ENABLE;
	/* XXX Reset pointer in command? */
	return 0;
}

/*
 * Send an abort to the target.  Return 1 success, 0 on failure.
 */
int
sea_abort(sea, scb)
	struct sea_softc *sea;
	struct sea_scb *scb;
{
	struct sea_scb *tmp;
	u_char msg, phase, *msgptr;
	int32 len;
	int s;

	s = splbio();

	/*
	 * If the command hasn't been issued yet, we simply remove it from the
	 * issue queue
	 * XXX Could avoid this loop.
	 */
	for (tmp = sea->issue_queue.tqh_first; tmp; tmp = tmp->chain.tqe_next)
		if (scb == tmp) {
			TAILQ_REMOVE(&sea->issue_queue, scb, chain);
			/* XXX Set some type of error result for operation. */
			splx(s);
			return 1;
		}

	/*
	 * If any commands are connected, we're going to fail the abort and let
	 * the high level SCSI driver retry at a later time or issue a reset.
	 */
	if (sea->connected) {
		splx(s);
		return 0;
	}

	/*
	 * If the command is currently disconnected from the bus, and there are
	 * no connected commands, we reconnect the I_T_L or I_T_L_Q nexus
	 * associated with it, go into message out, and send an abort message.
	 */
	for (tmp = sea->disconnected_queue.tqh_first; tmp;
	    tmp = tmp->chain.tqe_next)
		if (scb == tmp) {
			splx(s);
			if (sea_select(sea, scb))
				return 0;

			msg = MSG_ABORT;
			msgptr = &msg;
			len = 1;
			phase = REQ_MSGOUT;
			CONTROL = BASE_CMD | CMD_ATTN;
			sea_transfer_pio(sea, &phase, &len, &msgptr);

			s = splbio();
			for (tmp = sea->disconnected_queue.tqh_first; tmp;
			    tmp = tmp->chain.tqe_next)
				if (scb == tmp) {
					TAILQ_REMOVE(&sea->disconnected_queue,
					    scb, chain);
					/* XXX Set some type of error result
					   for the operation. */
					splx(s);
					return 1;
				}
		}

	/* Command not found in any queue; race condition? */
	splx(s);
	return 1;
}

void
sea_done(sea, scb)
	struct sea_softc *sea;
	struct sea_scb *scb;
{
	struct scsi_xfer *xs = scb->xfer;

	untimeout(sea_timeout, scb);

	xs->resid = scb->datalen;

	if ((scb->flags == SCB_ACTIVE) || (xs->flags & SCSI_ERR_OK)) {
		xs->resid = 0;
		xs->error = 0;
	} else {
		if (!(scb->flags == SCB_ACTIVE)) {
			if ((scb->flags & SCB_TIMEOUT) ||
			    (scb->flags & SCB_ABORTED))
				xs->error = XS_TIMEOUT;
			if (scb->flags & SCB_ERROR)
				xs->error = XS_DRIVER_STUFFUP;
		} else {
			/* XXX Add code to check for target status. */
			xs->error = XS_DRIVER_STUFFUP;
		}
	}
	xs->flags |= ITSDONE;
	sea_free_scb(sea, scb, xs->flags);
	scsi_done(xs);
}

/*
 * Wait for completion of command in polled mode.
 */
int
sea_poll(sea, xs, scb)
	struct sea_softc *sea;
	struct scsi_xfer *xs;
	struct sea_scb *scb;
{
	int count = 500; /* XXX xs->timeout; */
	int s;

	while (count) {
		/* try to do something */
		s = splbio();
		if (!main_running)
			sea_main();
		splx(s);
		if (xs->flags & ITSDONE)
			break;
		delay(10);
		count--;
	}
	if (count == 0) {
		/*
		 * We timed out, so call the timeout handler manually,
		 * accounting for the fact that the clock is not running yet
		 * by taking out the clock queue entry it makes.
		 */
		sea_timeout(scb);

		/*
		 * Because we are polling, take out the timeout entry
		 * sea_timeout() made.
		 */
		untimeout(sea_timeout, scb);
		count = 50;
		while (count) {
			/* Once again, wait for the int bit. */
			s = splbio();
			if (!main_running)
				sea_main();
			splx(s);
			if (xs->flags & ITSDONE)
				break;
			delay(10);
			count--;
		}
		if (count == 0) {
			/* 
			 * We timed out again... This is bad.  Notice that
			 * this time there is no clock queue entry to remove
			 */
			sea_timeout(scb);
		}
	}
	if (xs->error)
		return HAD_ERROR;
	return COMPLETE;
}

/*
 * Do the transfer.  We know we are connected.  Update the flags, and call
 * sea_done() when task accomplished.  Dialog controlled by the target.
 */
void
sea_information_transfer(sea)
	struct sea_softc *sea;
{
	int timeout;
	u_char msgout = MSG_NOP;
	int32 len;
	int s;
	u_char *data;
	u_char phase, tmp, old_phase = REQ_UNKNOWN;
	struct sea_scb *scb = sea->connected;
	int loop;

	for (timeout = 0; timeout < 10000000L; timeout++) {
		tmp = STATUS;
		if (!(tmp & STAT_BSY)) {
#if 0
			for (loop = 0; loop < 20; loop++)
				if ((tmp = STATUS) & STAT_BSY)
					break;
#endif
			if (!(tmp & STAT_BSY)) {
				printf("%s: !STAT_BSY unit in data transfer!\n",
				    sea->sc_dev.dv_xname);
				s = splbio();
				sea->connected = NULL;
				scb->flags = SCB_ERROR;
				splx(s);
				sea_done(sea, scb);
				return;
			}
		}

		/* we only have a valid SCSI phase when REQ is asserted */
		if (!(tmp & STAT_REQ))
			continue;

		phase = (tmp & REQ_MASK);
		if (phase != old_phase)
			old_phase = phase;

		switch (phase) {
		case REQ_DATAOUT:
#ifdef SEA_NODATAOUT
			printf("%s: SEA_NODATAOUT set, attempted DATAOUT aborted\n",
			    sea->sc_dev.dv_xname);
			msgout = MSG_ABORT;
			CONTROL = BASE_CMD | CMD_ATTN;
			break;
#endif
		case REQ_DATAIN:
			if (!scb->data)
				printf("no data address!\n");
#ifdef SEA_BLINDTRANSFER
			if (scb->datalen && !(scb->datalen % BLOCK_SIZE)) {
				while (scb->datalen) {
					for (timeout = 0; timeout < 5000000L;
					    timeout++)
						if ((tmp = STATUS) & STAT_REQ)
							break;
					if (!(tmp & STAT_REQ)) {
						printf("%s: timeout waiting for STAT_REQ\n",
						    sea->sc_dev.dv_xname);
						/* XXX Do something? */
					}
					if ((tmp & REQ_MASK) != phase)
						break;
					if (!(phase & STAT_IO)) {
#ifdef SEA_ASSEMBLER
						asm("shr $2, %%ecx\n\t\
						    cld\n\t\
						    rep\n\t\
						    movsl" :
						    "=S" (scb->data) :
						    "0" (scb->data),
						    "D" (sea->maddr_dr),
						    "c" (BLOCK_SIZE) :
						    "%ecx", "%edi");
#else
						for (count = 0;
						    count < BLOCK_SIZE;
						    count++)
							DATA = *(scb->data++);
#endif
					} else {
#ifdef SEA_ASSEMBLER
						asm("shr $2, %%ecx\n\t\
						    cld\n\t\
						    rep\n\t\
						    movsl" :
						    "=D" (scb->data) :
						    "S" (sea->maddr_dr),
						    "0" (scb->data),
						    "c" (BLOCK_SIZE) :
						    "%ecx", "%esi");
#else
					        for (count = 0;
						    count < BLOCK_SIZE;
						    count++)
							*(scb->data++) = DATA;
#endif
					}
					scb->datalen -= BLOCK_SIZE;
				}
			}
#endif 
			if (scb->datalen)
				sea_transfer_pio(sea, &phase, &scb->datalen,
				    &scb->data);
			break;
		case REQ_MSGIN:
			/* Multibyte messages should not be present here. */
			len = 1;
			data = &tmp;
			sea_transfer_pio(sea, &phase, &len, &data);
			/* scb->MessageIn = tmp; */

			switch (tmp) {
			case MSG_ABORT:
				scb->flags = SCB_ABORTED;
				printf("sea: command aborted by target\n");
				CONTROL = BASE_CMD;
				sea_done(sea, scb);
				return;
			case MSG_COMMAND_COMPLETE:
				s = splbio();
				sea->connected = NULL;
				splx(s);
				sea->busy[scb->xfer->sc_link->target] &= 
				    ~(1 << scb->xfer->sc_link->lun);
				CONTROL = BASE_CMD;
				sea_done(sea, scb);
				return;
			case MSG_MESSAGE_REJECT:
				printf("%s: message_reject recieved\n",
				    sea->sc_dev.dv_xname);
				break;
			case MSG_DISCONNECT:
				s = splbio();
				TAILQ_INSERT_TAIL(&sea->disconnected_queue,
				    scb, chain);
				sea->connected = NULL;
				CONTROL = BASE_CMD;
				splx(s);
				return;
			case MSG_SAVE_POINTERS:
			case MSG_RESTORE_POINTERS:
				/* save/restore of pointers are ignored */
				break;
			default:
				/*
				 * This should be handled in the pio data
				 * transfer phase, as the ATN should be raised
				 * before ACK goes false when rejecting a
				 * message.
				 */
				printf("%s: unknown message in: %x\n",
				    sea->sc_dev.dv_xname, tmp);
				break;
			} /* switch (tmp) */
			break;
		case REQ_MSGOUT:
			len = 1;
			data = &msgout;
			/* sea->last_message = msgout; */
			sea_transfer_pio(sea, &phase, &len, &data);
			if (msgout == MSG_ABORT) {
				printf("%s: sent message abort to target\n",
				    sea->sc_dev.dv_xname);
				s = splbio();
				sea->busy[scb->xfer->sc_link->target] &= 
				    ~(1 << scb->xfer->sc_link->lun);
				sea->connected = NULL;
				scb->flags = SCB_ABORTED;
				splx(s); 
				/* enable interrupt from scsi */
				sea_done(sea, scb);
				return;
			}
			msgout = MSG_NOP;
			break;
		case REQ_CMDOUT:
			len = scb->xfer->cmdlen;
			data = (char *) scb->xfer->cmd;
			sea_transfer_pio(sea, &phase, &len, &data);
			break;
		case REQ_STATIN:
			len = 1;
			data = &tmp;
			sea_transfer_pio(sea, &phase, &len, &data);
			scb->xfer->status = tmp;
			break;
		default:
			printf("sea: unknown phase\n");
		} /* switch (phase) */
	} /* for (...) */

	/* If we get here we have got a timeout! */
	printf("%s: timeout in data transfer\n", sea->sc_dev.dv_xname);
	scb->flags = SCB_TIMEOUT;
	/* XXX Should I clear scsi-bus state? */
	sea_done(sea, scb);
}
