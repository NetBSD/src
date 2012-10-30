/* $NetBSD: sbicvar.h,v 1.4.114.1 2012/10/30 17:18:38 yamt Exp $ */

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
 *	@(#)scsivar.h	7.1 (Berkeley) 5/8/90
 */

#ifndef _SBICVAR_H_
#define _SBICVAR_H_
#include <sys/malloc.h>
#include <sys/callout.h>

/*
 * ACB. Holds additional information for each SCSI command Comments: We
 * need a separate scsi command block because we may need to overwrite it
 * with a request sense command.  Basicly, we refrain from fiddling with
 * the scsi_xfer struct (except do the expected updating of return values).
 * We'll generally update: xs->{flags,resid,error,sense,status} and
 * occasionally xs->retries.
 */
struct sbic_acb {
	TAILQ_ENTRY(sbic_acb) chain;
	struct scsipi_xfer *xs;		/* SCSI xfer ctrl block from above */
	int		flags;		/* Status */
#define ACB_FREE	0x00
#define ACB_ACTIVE	0x01
#define ACB_DONE	0x02
#define ACB_DATAIN	0x04		/* DMA direction flag */
#define	ACB_DMA		0x08		/* ACB using DMA this time */
	struct scsipi_generic cmd;	/* SCSI command block */
	int	 	clen;
	bus_dmamap_t	dmamap_xfer;	/* Handle for dma */
	u_char	       *data;		/* Data buffer... */
	int		datalen;	/* ... and its length. */
	int		offset;
	u_long		sc_tcnt;	/* number of bytes for this DMA */
};

/*
 * Some info about each (possible) target on the SCSI bus.  This should
 * probably have been a "per target+lunit" structure, but we'll leave it at
 * this for now.  Is there a way to reliably hook it up to sc->fordriver??
 */
struct sbic_tinfo {
	int	cmds;		/* #commands processed */
	int	dconns;		/* #disconnects */
	int	touts;		/* #timeouts */
	int	perrs;		/* #parity errors */
	u_char*	bounce;		/* Bounce buffer for this device */
	ushort	lubusy;		/* What local units/subr. are busy? */
	u_char  flags;
	u_char  period;		/* Period suggestion */
	u_char  offset;		/* Offset suggestion */
};

struct	sbic_softc {
	device_t sc_dev;
/*	struct	isr sc_isr;*/
	struct  callout sc_timo_ch;
	struct	target_sync {
		u_char	state;
		u_char	period;
		u_char	offset;
	} sc_sync[8];
	u_char	target;			/* Currently active target */
	u_char  lun;
	struct	scsipi_channel sc_channel;
	struct	scsipi_adapter sc_adapter;
	sbic_regmap	sc_sbicp;	/* Handle for the SBIC */

	volatile void 	*sc_cregs;	/* driver specific regs */

	/* Lists of command blocks */
	TAILQ_HEAD(acb_list, sbic_acb) free_list,
				       ready_list,
				       nexus_list;

	struct sbic_acb *sc_nexus;	/* current command */
	struct sbic_acb sc_acb[8];	/* the real command blocks */
	struct sbic_tinfo sc_tinfo[8];

	u_char	sc_flags;
	u_char	sc_scsiaddr;
	u_char	sc_stat[2];
	u_char	sc_msg[7];
	u_long	sc_clkfreq;

	int	sc_dmaflags;		/* Target-specific busdma flags */
	void	*sc_dmah;		/* Interface specific dma handle */
	bus_dma_tag_t sc_dmat;		/* Tag for dma accesses */
	int	sc_max_dmalen;		/* Maximum DMA segment length */
	int	sc_dmamode;		/* Machine-specific DMA mode for
					   the SBIC chip */

	u_short	sc_dmatimo;		/* dma timeout */
	int  (*sc_dmaok)     (void *, bus_dma_tag_t, struct sbic_acb *);
	int  (*sc_dmasetup)  (void *, bus_dma_tag_t, struct sbic_acb *, int);
	int  (*sc_dmanext)   (void *, bus_dma_tag_t, struct sbic_acb *, int);
	void (*sc_dmastop)   (void *, bus_dma_tag_t, struct sbic_acb *);
	void (*sc_dmafinish) (void *, bus_dma_tag_t, struct sbic_acb *);
	void (*sc_enintr)    (struct sbic_softc *);
};

/* sc_flags */
#define	SBICF_ALIVE	0x01	/* controller initialized */
#define SBICF_SELECTED	0x02	/* bus is in selected state. */
#define SBICF_ICMD	0x04	/* Immediate command in execution */
#define SBICF_BADDMA	0x08	/* controller can only DMA to ztwobus space */
#define SBICF_NODMA	0x10	/* Don't use DMA */
#define	SBICF_INTR	0x20	/* SBICF interrupt expected */
#define	SBICF_INDMA	0x40	/* not used yet, DMA I/O in progress */

/* sync states */
#define SYNC_START	0	/* no sync handshake started */
#define SYNC_SENT	1	/* we sent sync request, no answer yet */
#define SYNC_DONE	2	/* target accepted our (or inferior) settings,
				   or it rejected the request and we stay 
				   async */
#ifdef DEBUG
#define	DDB_FOLLOW	0x04
#define DDB_IO		0x08
#endif
extern u_char sbic_inhibit_sync[8];
extern int sbic_no_dma;
extern int sbic_clock_override;

#define	PHASE_MASK	0x07		/* mask for psns/pctl phase */
#define	DATA_OUT_PHASE	0x00
#define	DATA_IN_PHASE	0x01
#define	CMD_PHASE	0x02
#define	STATUS_PHASE	0x03
#define	BUS_FREE_PHASE	0x04
#define	ARB_SEL_PHASE	0x05	/* Fuji chip combines arbitration with sel. */
#define	MESG_OUT_PHASE	0x06
#define	MESG_IN_PHASE	0x07

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

#define MSG_ISIDENTIFY(x) (x&MSG_IDENTIFY)
#define IFY_TRN		0x20
#define IFY_LUNTRN(x)	(x&0x07)
#define IFY_LUN(x)	(!(x&0x20))

/* Check if high bit set */

#define	STS_CHECKCOND	0x02	/* Check Condition (ie., read sense) */
#define	STS_CONDMET	0x04	/* Condition Met (ie., search worked) */
#define	STS_BUSY	0x08
#define	STS_INTERMED	0x10	/* Intermediate status sent */
#define	STS_EXT		0x80	/* Extended status valid */


/* States returned by our state machine */

#define SBIC_STATE_ERROR	-1
#define SBIC_STATE_DONE		0
#define SBIC_STATE_RUNNING	1
#define SBIC_STATE_DISCONNECT	2

/*
 * XXXX
 */
struct scsi_fmt_cdb {
	int len;		/* cdb length (in bytes) */
	u_char cdb[28];		/* cdb to use on next read/write */
};

struct buf;
struct scsipi_xfer;

void sbic_minphys	(struct buf *bp);
void sbic_scsi_request	(struct scsipi_channel *,
				scsipi_adapter_req_t, void *);
int  sbicinit		(struct sbic_softc *dev);
int  sbicintr 		(struct sbic_softc *);
void sbic_dump		(struct sbic_softc *dev);

#endif /* _SBICVAR_H_ */
