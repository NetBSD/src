/*	$NetBSD: scsireg.h,v 1.3.10.1 2000/12/08 09:28:51 bouyer Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Sony Corp. and Kazumasa Utashiro of Software Research Associates, Inc.
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
 * from: $Hdr: scsireg.h,v 4.300 91/06/09 06:38:12 root Rel41 $ SONY
 *
 *	@(#)scsireg.h	8.1 (Berkeley) 6/11/93
 */

/*
 *	scsireg.h
 */

#ifndef __SCSIREG__
#define __SCSIREG__ 1

/*
 *	initiator status byte bit image
 */
#define	INST_EP		0x80		/* End of Process */
#define INST_WR		0x40		/* Waiting Reselection */
#define	INST_IP		0x20		/* In Process */
#define	INST_WAIT	0x10		/* Waiting Bus free */
#define	INST_LB		0x8		/* Loss of BUSY */
#define	INST_TO		0x4		/* Time Out */
#define	INST_PRE	0x2		/* PaRameter Error */
#define	INST_HE		0x1		/* Hard Error */

#define	INSTERMASK	0x7


/*
 *	target status byte bit image
 */
#define	VENDOR		0x61
#define	TGSTMASK	0x1e
#define	TGST_RSVCFLCT	0x18
#define	TGST_INTERMED	0x10
#define	TGST_BUSY	0x8
#define	TGST_CC		0x2
#define	TGST_GOOD	0x0

#define	TS_MAPPED_PIO	0x01		/* program I/O with map */
#define	TS_CONTR_ON	0x02		/* contiguous transfer on */
#define	TS_CONTR_OFF	0x04		/* contiguous transfer off */
#define	TS_BYTE_DMA	0x08		/* DMA transfer(byte access) */
#define	TS_LONG_DMA	0x10		/* DMA transfer(long access) */


/*
 *	message byte
 */
#define MSG_IDENT	0x80
#define MSG_RESELEN	0x40
#define MSG_CCOMP	0
#define	MSG_EXTND	1
#define MSG_SDP		2
#define MSG_RDP		3
#define MSG_DCNT	4
#define MSG_IDE		5
/*#define MSG_ABORT	6*/
#define MSG_MREJ	7
#define MSG_NOP		8
#define MSG_PERROR	9


/*
 *	message identify byte bit image
 */
#define	IDT_DISCON	0x40
#define	IDT_DRMASK	0x7


/*
 *	scsi command opcodes
 */
#define SCOP_TST	0x00
#define SCOP_REZERO	0x01
#define	SCOP_REWIND	0x01
#define SCOP_RSENSE	0x03
#define SCOP_FMT	0x04
#define SCOP_RBLIM	0x05
#define SCOP_SPARAM	0x06
#define SCOP_RASBLK	0x07
#define SCOP_READ	0x08
#define SCOP_MOERASE	0x09
#define SCOP_WRITE	0x0a
#define SCOP_SEEK	0x0b
#define	SCOP_MERASE	0x0e
#define	SCOP_WFMARK	0x10
#define	SCOP_SPACE	0x11
#define SCOP_INQUIRY	0x12
#define	SCOP_SVERIFY	0x13
#define	SCOP_RBDATA	0x14
#define SCOP_MSELECT	0x15
#define	SCOP_ERASE	0x19
#define SCOP_MSENSE	0x1a
#define SCOP_STST	0x1b
#define	SCOP_LOAD	0x1b
#define SCOP_RECDIAG	0x1c
#define SCOP_SNDDIAG	0x1d
#define	SCOP_MEDRMV	0x1e
#define SCOP_RCAP	0x25
#define SCOP_EREAD	0x28
#define SCOP_EWRITE	0x2a
#define	SCOP_BSSRCH	0x2c
#define	SCOP_WSSRCH	0x2d
#define	SCOP_WRTVRFY	0x2e
#define SCOP_VERIFY	0x2f
#define SCOP_RDL	0x37
#define SCOP_WBUF	0x3b
#define SCOP_RBUF	0x3c
#define SCOP_EJECT	0xc0
#define SCOP_EESENSE	0xc1
#define SCOP_READTOC	0xc1
#define SCOP_READID	0xc2
#define SCOP_ADP	0xc2
#define SCOP_READQ	0xc2
#define SCOP_BLANKS	0xc3
#define SCOP_READHEAD	0xc3
#define SCOP_PBSTS	0xc4
#define SCOP_RCVDISK	0xc4
#define SCOP_PAUSE	0xc5
#define SCOP_PLAYTRACK	0xc6
#define SCOP_PLAYMSF	0xc7
#define SCOP_PLAYAUDIO	0xc8
#define SCOP_ERASED	0xe7
#define SCOP_RESET	0xff


#ifdef CPU_DOUBLE
# ifdef mips
#  define	ipc_phys(x)	(caddr_t)K0_TT0(x)
# else
#  define	ipc_phys(x)	(caddr_t)((int)(x) & ~0x80000000)
# endif
# ifdef news3800
#  define	splsc		spl4
#  define	splscon		spl3
# endif
#endif /* CPU_DOUBLE */

#ifdef CPU_SINGLE
# define	ipc_phys(x)	(caddr_t)(x)
# ifdef news3400
#  define	splsc		cpu_spl0	/* Lite2 used spl3 */
#  define	splscon		spl2 XXX not used
   extern int cpu_spl0 __P((void));
# else
#  define	splsc		spl4
#  define	splscon		spl3
# endif
#endif /* CPU_SINGLE */

#define	SCSI_INTEN	1
#define	SCSI_INTDIS	0


/*
 *	other definition
 */
#define	ON	1
#define	OFF	0


/*
 *	scsi map table format
 */
#if defined(news3400)
#define NSCMAP 120
#endif

#if defined(news3800)
#define NSCMAP 129
#endif

struct sc_map {
	u_int	mp_offset;
	u_int	mp_pages;
	u_int	mp_addr[NSCMAP];	/* page number */
};

struct sc_chan_stat {
	struct sc_chan_stat *wb_next;	/* wait bus channel queue */
	struct sc_scb	*scb;		/* scsi struct address */
	u_int		stcnt;		/* save transfer count */
	u_char		*spoint;	/* save transfer point */
	u_int		stag;		/* save tag register */
	u_int		soffset;	/* save offset register */
	int		chan_num;	/* channel NO. */
	u_char		comflg;		/* flag for save comand pointer */
	u_char		intr_flg;	/* interrupt flag. SCSI_INTEN/INTDIS */
};

struct sc_scb {
	TAILQ_ENTRY(sc_scb) chain;
	struct scsipi_xfer *xs;
	int	flags;

	struct sc_softc *scb_softc;
	struct sc_map *sc_map;
	u_char	*sc_cpoint;		/* pointer to buffer address */
	u_int	sc_ctrnscnt;		/* transfer count */
	u_int	sc_ctag;
	u_int	sc_coffset;

	u_char	istatus;
	u_char	tstatus;
	u_char	identify;
	u_char	message;
	u_char	msgbuf[20];
};

#define	NTARGET 8

struct sc_softc {
	struct device sc_dev;
	struct scsipi_link sc_link;
	struct scsipi_adapter sc_adapter;

	TAILQ_HEAD(scb_list, sc_scb) ready_list, free_list;
	struct sc_scb sc_scb[3*NTARGET];

	int inuse[NTARGET];
	struct sc_map sc_map[NTARGET];
	struct sc_chan_stat chan_stat[NTARGET];	/* SCSI channel status */
	int sel_stat[NTARGET];			/* target select status */

	int scsi_1185AQ;
	int pad_start;

	int	wbc;	/* # of channel that is waiting for scsi bus free */	
	int	wrc;	/* # of channel that is waiting for reselection */	
	struct sc_chan_stat *ip;
			/* In progress channel. Same as ISTAT.IP */
	int	ipc;		/* number of in progress channel. */
	int	dma_stat;	/* OFF = DMAC is not used */
#define SC_DMAC_RD	1
#define SC_DMAC_WR	2

	struct sc_chan_stat *wbq_actf;		/* forword active pointer */
	struct sc_chan_stat *wbq_actl;		/* last active pointer */

	u_char	*act_cmd_pointer;
	u_char	*min_point[NTARGET];
	int pad_cnt[NTARGET];
	char min_cnt[NTARGET];
	char sync_tr[NTARGET];			/* sync/async flag */
	char mout_flag[NTARGET];
	char perr_flag[NTARGET];
	int int_stat1;
	int int_stat2;
	int min_flag;
	int lastcmd;
};

/*
 * sel_stat values
 */
#define	SEL_WAIT	0
#define	SEL_START	1
#define	SEL_TIMEOUT	2
#define	SEL_ARBF	3
#define	SEL_SUCCESS	4
#define	SEL_RSLD	5
#define	SEL_RSL_WAIT	6

/*
 * mout_flag values
 */
#define MOUT_IDENTIFY	1
#define MOUT_SYNC_TR	2


struct scintsw {
/*00*/	int	(*sci_inthandler)(int);	/* pointer to interrupt handler */
/*04*/	int	sci_ctlr;		/* controller number */
/*08*/
};

#endif /* !__SCSIREG__ */
