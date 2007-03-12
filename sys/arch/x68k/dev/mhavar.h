/*	$NetBSD: mhavar.h,v 1.7.26.1 2007/03/12 05:51:39 rmind Exp $	*/

/*
 * Copyright (c) 1994 Peter Galbavy.  All rights reserved.
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
 *	This product includes software developed by Peter Galbavy.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * ACB. Holds additional information for each SCSI command Comments: We
 * need a separate scsi command block because we may need to overwrite it
 * with a request sense command.  Basicly, we refrain from fiddling with
 * the scsi_xfer struct (except do the expected updating of return values).
 * We'll generally update: xs->{flags,resid,error,sense,status} and
 * occasionally xs->retries.
 */
struct acb {
	TAILQ_ENTRY(acb) chain;
	struct scsipi_xfer *xs;	/* SCSI xfer ctrl block from above */
	int		flags;	/* Status */
#define ACB_QNONE	0
#define ACB_QFREE	1
#define ACB_QREADY	2
#define ACB_QNEXUS	3

#define ACB_QBITS	0x07
#define ACB_CHKSENSE	0x08
#define	ACB_ABORTED	0x10
#define	ACB_RESET	0x80
#define ACB_SETQ(e, q)	do (e)->flags = ((e)->flags&~ACB_QBITS)|(q); while(0)
	struct scsipi_generic cmd;  /* SCSI command block */
	int	 clen;
	char	*daddr;		/* Saved data pointer */
	int	 dleft;		/* Residue */
	u_char 	 stat;		/* SCSI status byte */

/*	struct spc_dma_seg dma[SPC_NSEG];*/ /* Physical addresses+len */
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
#define T_NEED_TO_RESET	0x01	/* Should send a BUS_DEV_RESET */
#define T_NEGOTIATE	0x02	/* (Re)Negotiate synchronous options */
#define T_BUSY		0x04	/* Target is busy, i.e. cmd in progress */
#define T_SYNCMODE	0x08	/* sync mode has been negotiated */
#define T_SYNCHOFF	0x10	/* .. */
#define T_RSELECTOFF	0x20	/* .. */
	u_char  period;		/* Period suggestion */
	u_char  offset;		/* Offset suggestion */
	u_char	width;		/* Width suggestion */
} tinfo_t;

struct mha_softc {
	struct device sc_dev;			/* us as a device */
	volatile void *sc_iobase;
	volatile u_char	*sc_pc;
	volatile u_short *sc_ps;
	volatile u_char *sc_pcx;

	struct scsipi_channel sc_channel;
	struct scsipi_adapter sc_adapter;

	TAILQ_HEAD(, acb) free_list, ready_list, nexus_list;
	struct acb *sc_nexus;	/* current command */
	struct acb sc_acb[8];	/* one per target */
	struct spc_tinfo sc_tinfo[8];

	/* Data about the current nexus (updated for every cmd switch) */
	u_char	*sc_dp;		/* Current data pointer */
	size_t	sc_dleft;	/* Data bytes left to transfer */

	u_char	*sc_cp;		/* Current command pointer */
	size_t	sc_cleft;	/* Command bytes left to transfer */

	/* Adapter state */
	int	sc_phase;	/* Copy of what bus phase we are in */
	int	sc_prevphase;	/* Copy of what bus phase we were in */
	u_char	sc_state;	/* State applicable to the adapter */
	u_char	 sc_flags;
	u_char	sc_selid;	/* Reselection ID */
	volatile u_char	 sc_spcinitialized;	/* */

	/* Message stuff */
	u_char	sc_msgpriq;	/* Messages we want to send */
	u_char	sc_msgout;	/* What message is on its way out? */
	u_char	sc_msgoutq;	/* Messages sent during last MESSAGE OUT */
	u_char	sc_lastmsg;	/* Message last transmitted */
	u_char	sc_currmsg;	/* Message currently ready to transmit */
#define SPC_MAX_MSG_LEN 8
	u_char  sc_omess[SPC_MAX_MSG_LEN];
	u_char	*sc_omp;		/* Outgoing message pointer */
	u_char	sc_imess[SPC_MAX_MSG_LEN];
	u_char	*sc_imp;		/* Incoming message pointer */
	size_t	sc_imlen;

	/* Hardware stuff */
	int	sc_freq;		/* Clock frequency in MHz */
	int	sc_id;			/* our scsi id */
	int	sc_minsync;		/* Minimum sync period / 4 */
	int	sc_maxsync;		/* Maximum sync period / 4 */

	/* DMA buffer */
	bus_dma_tag_t		sc_dmat;
	bus_dma_segment_t	sc_dmaseg[1];
	int			sc_ndmasegs;
	bus_dmamap_t		sc_dmamap;
	void *			sc_dmabuf;
	u_char			*sc_p;
	u_int32_t		sc_dmasize;
};

/* values for sc_state */
#define	SPC_INIT	0x00
#define SPC_IDLE	0x01	/* waiting for something to do */
#define ESP_TMP_UNAVAIL	0x02	/* Don't accept SCSI commands */
#define SPC_SELECTING	0x03	/* SCSI command is arbiting  */
#define SPC_RESELECTED	0x04	/* Has been reselected */
#define SPC_HASNEXUS	0x05	/* Actively using the SCSI bus */
#define SPC_CLEANING	0x06
#define ESP_SBR		0x07	/* Expect a SCSI RST because we commanded it */
#define SPC_CONNECTED	0x08	/* Actively using the SCSI bus */
#define	SPC_DISCONNECT	0x09	/* MSG_DISCONNECT received */
#define	SPC_CMDCOMPLETE	0x0a	/* MSG_CMDCOMPLETE received */


/* values for sc_flags */
#define SPC_DROP_MSGI	0x01	/* Discard all msgs (parity err detected) */
#define SPC_DOINGDMA	0x02	/* The FIFO data path is active! */
#define SPC_BUSFREE_OK	0x04	/* Bus free phase is OK. */
#define SPC_SYNCHNEGO	0x08	/* Synch negotiation in progress. */
#define SPC_DISCON	0x10	/* Target sent DISCONNECT msg */
#define	SPC_ABORTING	0x20	/* Bailing out */
#define ESP_ICCS	0x40	/* Expect status phase results */
#define ESP_WAITI	0x80	/* Waiting for non-DMA data to arrive */


/* values for sc_msgout */
#define SEND_DEV_RESET		0x01
#define SEND_PARITY_ERROR	0x02
#define SEND_INIT_DET_ERR	0x04
#define SEND_REJECT		0x08
#define SEND_IDENTIFY  		0x10
#define SEND_ABORT		0x20
#define SEND_SDTR		0x40
#define	SEND_WDTR		0x80

/* SCSI Status codes */
#define ST_GOOD			0x00
#define ST_CHKCOND		0x02
#define ST_CONDMET		0x04
#define ST_BUSY			0x08
#define ST_INTERMED		0x10
#define ST_INTERMED_CONDMET	0x14
#define ST_RESERVATION_CONFLICT	0x18
#define ST_CMD_TERM		0x22
#define ST_QUEUE_FULL		0x28

#define ST_MASK			0x3e /* bit 0,6,7 is reserved */

/* phase bits */
#define IOI			0x01
#define CDI			0x02
#define MSGI			0x04

/* Information transfer phases */
#define DATA_OUT_PHASE		(0)
#define DATA_IN_PHASE		(IOI)
#define COMMAND_PHASE		(CDI)
#define STATUS_PHASE		(CDI|IOI)
#define MESSAGE_OUT_PHASE	(MSGI|CDI)
#define MESSAGE_IN_PHASE	(MSGI|CDI|IOI)

#define PHASE_MASK		(MSGI|CDI|IOI)

/* Some pseudo phases for getphase()*/
#define BUSFREE_PHASE		0x100	/* Re/Selection no longer valid */
#define INVALID_PHASE		0x101	/* Re/Selection valid, but no REQ yet */
#define PSEUDO_PHASE		0x100	/* "pseudo" bit */
