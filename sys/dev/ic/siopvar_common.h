/*	$NetBSD: siopvar_common.h,v 1.4 2000/07/24 15:15:01 bouyer Exp $	*/

/*
 * Copyright (c) 2000 Manuel Bouyer.
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
 *	This product includes software developed by Manuel Bouyer
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
 *
 */

/* common struct and routines used by siop and esiop */

#ifndef SIOP_DEFAULT_TARGET
#define SIOP_DEFAULT_TARGET 7
#endif

/* tables used by SCRIPT */
typedef struct scr_table {
	u_int32_t count;
	u_int32_t addr;
} scr_table_t ;

/* Number of scatter/gather entries */
#define SIOP_NSG	(MAXPHYS/NBPG + 1)

/*
 * This structure interfaces the SCRIPT with the driver; it describes a full
 * transfer. It lives in the same chunk of DMA-safe memory as the script.
 */
struct siop_xfer {
	u_int8_t msg_out[8];	/* 0 */
	u_int8_t msg_in[8];	/* 8 */
	int status;		/* 16 */
	u_int32_t pad1;		/* 20 */
	u_int32_t id;		/* 24 */
	u_int32_t pad2;		/* 28 */
	scr_table_t t_msgin;	/* 32 */
	scr_table_t t_extmsgin;	/* 40 */
	scr_table_t t_extmsgdata; /* 48 */
	scr_table_t t_msgtag;	/* 56 */
	scr_table_t t_msgout;	/* 64 */
	scr_table_t cmd;	/* 72 */
	scr_table_t t_status;	/* 80 */
	scr_table_t data[SIOP_NSG]; /* 88 */
} __attribute__((__packed__));

/*
 * This decribes a command handled by the SCSI controller
 * These are chained in either a free list or a active list
 * We have one queue per target
 */
struct siop_cmd {
	TAILQ_ENTRY (siop_cmd) next;
	struct siop_target *siop_target; /* pointer to our target def */
	struct scsipi_xfer *xs; /* xfer from the upper level */
	struct siop_xfer *siop_table; /* tables dealing with this xfer */
	struct siop_cbd *siop_cbdp; /* pointer to our siop_cbd */
	bus_addr_t	dsa; /* DSA value to load */
	bus_dmamap_t	dmamap_cmd;
	bus_dmamap_t	dmamap_data;
	struct scsipi_sense rs_cmd; /* request sense command buffer */
	int       status;
	int       flags;
};

/* command block descriptors: an array of siop_cmd + an array of siop_xfer */

struct siop_cbd {
	TAILQ_ENTRY (siop_cbd) next;
	struct siop_cmd *cmds;
	struct siop_xfer *xfers;
	bus_dmamap_t xferdma; /* DMA map for this block of xfers */
};

/* status defs */
#define CMDST_FREE		0 /* cmd slot is free */
#define CMDST_READY		1 /* cmd slot is waiting for processing */
#define CMDST_ACTIVE		2 /* cmd slot is being processed */
#define CMDST_SENSE		3 /* cmd slot is being requesting sense */
#define CMDST_SENSE_ACTIVE	4 /* request sense active */
#define CMDST_SENSE_DONE 	5 /* request sense done */
#define CMDST_DONE		6 /* cmd slot has been processed */
/* flags defs */
#define CMDFL_TIMEOUT	0x0001 /* cmd timed out */

/* per-target struct */
struct siop_target {
	int status;	/* target status, see below */
	int flags;	/* target flags, see below */
	u_int32_t id;	/* for SELECT FROM */
	struct cmd_list active_list[8]; /* per-lun active cmds */
	struct siop_softc *siop_sc; /* points back to our adapter */
};

/* target status */
#define TARST_PROBING	0 /* target is being probed */
#define TARST_ASYNC	1 /* target needs sync/wide negotiation */
#define TARST_WIDE_NEG	2 /* target is doing wide negotiation */
#define TARST_SYNC_NEG	3 /* target is doing sync negotiation */
#define TARST_OK	4 /* sync/wide agreement is valid */

/* target flags */
#define TARF_SYNC	0x00 /* target is sync */
#define TARF_WIDE	0x01 /* target is wide */

void	siop_common_reset __P((struct siop_softc *));
int	siop_modechange __P((struct siop_softc *));

int	siop_wdtr_neg __P((struct siop_cmd *siop_cmd));
int	siop_sdtr_neg __P((struct siop_cmd *siop_cmd));
/* actions to take at return of siop_wdtr_neg() and siop_sdtr_neg() */
#define SIOP_NEG_NOP	0x0
#define SIOP_NEG_MSGOUT	0x1
#define SIOP_NEG_ACK	0x2

void	siop_minphys __P((struct buf *));
int	siop_ioctl __P((struct scsipi_link *, u_long,
		caddr_t, int, struct proc *));
void 	siop_sdp __P((struct siop_cmd *));
void	siop_clearfifo __P((struct siop_softc *));
void	siop_resetbus __P((struct siop_softc *));
