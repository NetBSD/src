/*	$NetBSD: siopvar.h,v 1.8 1995/02/12 19:19:29 chopps Exp $	*/

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
 *	@(#)siopvar.h	7.1 (Berkeley) 5/8/90
 */
#ifndef _SIOPVAR_H_
#define _SIOPVAR_H_

/*
 * The largest single request will be MAXPHYS bytes which will require
 * at most MAXPHYS/NBPG+1 chain elements to describe, i.e. if none of
 * the buffer pages are physically contiguous (MAXPHYS/NBPG) and the
 * buffer is not page aligned (+1).
 */
#define	DMAMAXIO	(MAXPHYS/NBPG+1)

struct	siop_pending {
	TAILQ_ENTRY(siop_pending) link;
	struct scsi_xfer *xs;
};

struct siop_ds {			/* Data Structure for SCRIPTS */
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

struct	siop_softc {
	struct	device sc_dev;
	struct	isr sc_isr;

	u_char	sc_istat;
	u_char	sc_dstat;
	u_char	sc_sstat0;
	u_char	sc_sstat1;
	struct	scsi_link sc_link;	/* proto for sub devices */
	siop_regmap_p	sc_siopp;	/* the SIOP */
	volatile void 	*sc_cregs;	/* driver specific regs */
	struct	scsi_xfer *sc_xs;	/* transfer from high level code */
	u_long	sc_active;		/* number of active I/O's */
	/* I/O blocks for each active I/O */
	struct siop_iob {
		struct	scsi_xfer *sc_xs;
		struct siop_ds sc_ds;
		void	*iob_buf;
		u_long	iob_curbuf;
		u_long	iob_len, iob_curlen;
		u_char	sc_msgout[6];
		u_char	sc_msg[6];
		u_char	sc_stat[1];
		u_char	sc_status;
		u_char	sc_dummy[2];
	} *sc_cur;			/* current I/O block */
	struct	siop_iob sc_iob[2];	/* I/O blocks */
	u_long	sc_scriptspa;		/* physical address of scripts */
	u_long	sc_clock_freq;
	u_char	sc_flags;
	u_char	sc_slave;
	/* one for each target */
	struct syncpar {
	  u_char state;
	  u_char period, offset;
	} sc_sync[8];
	TAILQ_HEAD(,siop_pending) sc_xslist;	/* LIFO */
	struct	siop_pending sc_xsstore[8][8];	/* one for every unit */
};

/* sc_flags */
#define	SIOP_DMA	0x80	/* DMA I/O in progress */
#define	SIOP_ALIVE	0x01	/* controller initialized */
#define SIOP_SELECTED	0x04	/* bus is in selected state. Needed for
				   correct abort procedure. */

/* sync states */
#define SYNC_START	0	/* no sync handshake started */
#define SYNC_SENT	1	/* we sent sync request, no answer yet */
#define SYNC_DONE	2	/* target accepted our (or inferior) settings,
				   or it rejected the request and we stay async */

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
#define	MSG_SYNC_REQ 		0x01

#define	STS_CHECKCOND	0x02	/* Check Condition (ie., read sense) */
#define	STS_CONDMET	0x04	/* Condition Met (ie., search worked) */
#define	STS_BUSY	0x08
#define	STS_INTERMED	0x10	/* Intermediate status sent */
#define	STS_EXT		0x80	/* Extended status valid */

void siop_minphys __P((struct buf *bp));
int siop_scsicmd __P((struct scsi_xfer *));

#endif /* _SIOPVAR_H */
