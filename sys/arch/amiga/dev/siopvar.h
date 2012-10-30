/*	$NetBSD: siopvar.h,v 1.26.12.1 2012/10/30 17:18:51 yamt Exp $	*/

/*
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
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)siopvar.h	7.1 (Berkeley) 5/8/90
 */
#ifndef _SIOPVAR_H_
#define _SIOPVAR_H_

/*
 * The largest single request will be MAXPHYS bytes which will require
 * at most MAXPHYS/PAGE_SIZE+1 chain elements to describe, i.e. if none of
 * the buffer pages are physically contiguous (MAXPHYS/PAGE_SIZE) and the
 * buffer is not page aligned (+1).
 */
#define	DMAMAXIO	(MAXPHYS/PAGE_SIZE+1)

/*
 * Data Structure for SCRIPTS program
 */
struct siop_ds {
	long	scsi_addr;		/* SCSI ID & sync */
	long	idlen;			/* Identify message */
	char	*idbuf;
	long	cmdlen;			/* SCSI command */
	char	*cmdbuf;
	long	stslen;			/* Status */
	char	*stsbuf;
	long	msglen;			/* Message */
	char	*msgbuf;
	long	msginlen;		/* Message in */
	char	*msginbuf;
	long	extmsglen;		/* Extended message in */
	char	*extmsgbuf;
	long	synmsglen;		/* Sync transfer request */
	char	*synmsgbuf;
	struct {
		long datalen;
		char *databuf;
	} chain[DMAMAXIO];
};

/*
 * ACB. Holds additional information for each SCSI command Comments: We
 * need a separate scsi command block because we may need to overwrite it
 * with a request sense command.  Basicly, we refrain from fiddling with
 * the scsi_xfer struct (except do the expected updating of return values).
 * We'll generally update: xs->{flags,resid,error,sense,status} and
 * occasionally xs->retries.
 */
struct siop_acb {
	TAILQ_ENTRY(siop_acb) chain;
	struct scsipi_xfer *xs;	/* SCSI xfer ctrl block from above */
	int		flags;	/* Status */
#define ACB_FREE	0x00
#define ACB_ACTIVE	0x01
#define ACB_DONE	0x04
	struct scsipi_generic cmd;  /* SCSI command block */
	struct siop_ds ds;
	void	*iob_buf;
	u_long	iob_curbuf;
	u_long	iob_len, iob_curlen;
	u_char	msgout[6];
	u_char	msg[6];
	u_char	stat[1];
	u_char	status;
	u_char	dummy[2];
	int	 clen;
	char	*daddr;		/* Saved data pointer */
	int	 dleft;		/* Residue */
};

/*
 * Some info about each (possible) target on the SCSI bus.  This should
 * probably have been a "per target+lunit" structure, but we'll leave it at
 * this for now.  Is there a way to reliably hook it up to sc->fordriver??
 */
struct siop_tinfo {
	int	cmds;		/* #commands processed */
	int	dconns;		/* #disconnects */
	int	touts;		/* #timeouts */
	int	perrs;		/* #parity errors */
	ushort	lubusy;		/* What local units/subr. are busy? */
	u_char  flags;
	u_char  period;		/* Period suggestion */
	u_char  offset;		/* Offset suggestion */
};

struct	siop_softc {
	device_t sc_dev;
	struct	isr sc_isr;
	void	*sc_siop_si;

	u_char	sc_istat;
	u_char	sc_dstat;
#ifndef ARCH_720
	u_char	sc_sstat0;
#endif
	u_char	sc_sstat1;
#ifdef ARCH_720
	u_short	sc_sist;
#endif
	u_long	sc_intcode;
	struct	scsipi_adapter sc_adapter;
	struct	scsipi_channel sc_channel;
	u_long	sc_scriptspa;		/* physical address of scripts */
	siop_regmap_p	sc_siopp;	/* the SIOP */
	u_long	sc_active;		/* number of active I/O's */

	/* Lists of command blocks */
	TAILQ_HEAD(acb_list, siop_acb) free_list,
				       ready_list,
				       nexus_list;

	struct siop_acb *sc_nexus;	/* current command */
#define SIOP_NACB 16
	struct siop_acb *sc_acb;	/* the real command blocks */
#ifndef ARCH_720
	struct siop_tinfo sc_tinfo[8];
#else
	struct siop_tinfo sc_tinfo[16];
#endif

	u_short	sc_clock_freq;
	u_char	sc_dcntl;
	u_char	sc_ctest7;
	u_short	sc_tcp[4];
	u_char	sc_flags;
	u_char	sc_dien;
	u_char	sc_minsync;
#ifndef ARCH_720
	u_char	sc_sien;
#else
	u_short	sc_sien;
#endif
	/* one for each target */
	struct syncpar {
		u_char state;
		u_char sxfer;
#ifndef ARCH_720
		u_char sbcl;
#else
		u_char scntl3;
#endif
	} sc_sync[16];
};

/* sc_flags */
#define	SIOP_INTSOFF	0x80	/* Interrupts turned off */
#define	SIOP_INTDEFER	0x40	/* Level 6 interrupt has been deferred */
#define	SIOP_ALIVE	0x01	/* controller initialized */
#define SIOP_SELECTED	0x04	/* bus is in selected state. Needed for
				   correct abort procedure. */

/* negotiation states */
#define NEG_WIDE	0	/* Negotiate wide transfers */
#define	NEG_WAITW	1	/* Waiting for wide negotation response */
#define NEG_SYNC	2	/* Negotiate synch transfers */
#define	NEG_WAITS	3	/* Waiting for synch negoation response */
#define NEG_DONE	4	/* Wide and/or sync negotation done */

#define	MSG_CMD_COMPLETE	0x00
#define MSG_EXT_MESSAGE		0x01
#define	MSG_SAVE_DATA_PTR	0x02
#define	MSG_RESTORE_PTR		0x03
#define	MSG_DISCONNECT		0x04
#define	MSG_INIT_DETECT_ERROR	0x05
#define	MSG_ABORT		0x06
#define	MSG_REJECT		0x07
#define	MSG_NOOP		0x08
#define	MSG_PARITY_ERROR	0x09
#define	MSG_BUS_DEVICE_RESET	0x0C
#define	MSG_IDENTIFY		0x80
#define	MSG_IDENTIFY_DR		0xc0	/* (disconnect/reconnect allowed) */
#define	MSG_SYNC_REQ		0x01
#define	MSG_WIDE_REQ		0x03

#define	STS_CHECKCOND	0x02	/* Check Condition (ie., read sense) */
#define	STS_CONDMET	0x04	/* Condition Met (ie., search worked) */
#define	STS_BUSY	0x08
#define	STS_INTERMED	0x10	/* Intermediate status sent */
#define	STS_EXT		0x80	/* Extended status valid */

#ifdef ARCH_720
void siopng_minphys(struct buf *bp);
void siopng_scsipi_request(struct scsipi_channel *,
			scsipi_adapter_req_t, void *);
void siopnginitialize(struct siop_softc *);
void siopngintr(struct siop_softc *);
void siopng_dump_registers(struct siop_softc *);
#ifdef DEBUG
void siopng_dump(struct siop_softc *);
#endif

#else

void siop_minphys(struct buf *bp);
void siop_scsipi_request(struct scsipi_channel *,
			scsipi_adapter_req_t, void *);
void siopinitialize(struct siop_softc *);
void siopintr(struct siop_softc *);
void siop_dump_registers(struct siop_softc *);
#ifdef DEBUG
void siop_dump(struct siop_softc *);
#endif
#endif

#endif /* _SIOPVAR_H */
