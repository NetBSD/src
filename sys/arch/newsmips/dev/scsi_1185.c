/*	$NetBSD: scsi_1185.c,v 1.20.14.2 2017/12/03 11:36:33 jdolecek Exp $	*/

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
 * from: $Hdr: scsi_1185.c,v 4.300 91/06/09 06:22:20 root Rel41 $ SONY
 *
 *	@(#)scsi_1185.c	8.1 (Berkeley) 6/11/93
 */

/*
 * Copyright (c) 1989- by SONY Corporation.
 *
 *	scsi_1185.c
 *
 *	CXD1185Q
 *	SCSI bus low level common routines
 *				for one CPU machine
 *
 * MODIFY HISTORY:
 *
 *	DMAC_WAIT	--- DMAC_0266 wo tukau-baai, DMAC mata-wa SCSI-chip ni
 *				tuzukete access suru-baai,
 *				kanarazu wait wo ireru-beshi !
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: scsi_1185.c,v 1.20.14.2 2017/12/03 11:36:33 jdolecek Exp $");

#define	__INTR_PRIVATE
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/intr.h>

#include <uvm/uvm_extern.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <mips/locore.h>
#include <mips/cache.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/machConst.h>


#include <newsmips/dev/screg_1185.h>
#include <newsmips/dev/scsireg.h>

#include "ioconf.h"

#if defined(news3400)
# include <newsmips/dev/dmac_0448.h>
# ifndef NDMACMAP
# define NDMACMAP 144
# endif
#endif

#define ABORT_SYNCTR_MES_FROM_TARGET
#define SCSI_1185AQ
#define RESET_RECOVER
#define DMAC_MAP_INIT			/* for nws-3700 parity error */
#define APAD_ALWAYS_ON

#define CHECK_LOOP_CNT	60
#define RSL_LOOP_CNT	60

#ifndef DMAC_MAP_INIT
# define MAP_OVER_ACCESS		/* for nws-3700 parity error */
#endif

#undef	CHECK_MRQ

#ifdef NOT_SUPPORT_SYNCTR
# define MAX_OFFSET_BYTES 0
#else
# define MAX_OFFSET_BYTES MAX_OFFSET
#endif

#define	act_point	spoint
#define	act_trcnt	stcnt
#define	act_tag		stag
#define	act_offset	soffset

#define	splscsi splsc

#if defined(__mips__) && defined(CPU_SINGLE)
#define nops(x)		{ int __i; for (__i = 0; __i < (x); __i++) ; }
#define	DMAC_WAIT0	;
#else
#define	DMAC_WAIT0	DMAC_WAIT
#endif

#ifdef DMAC_MAP_INIT
static int dmac_map_init = 0;
#endif

/*
 *	command flag status
 */
#define	CF_SET		1
#define	CF_SEND		2
#define	CF_ENOUGH	3
#define	CF_EXEC		4

#define	SEL_TIMEOUT_VALUE 0x7a

void sc_send(struct sc_scb *, int, int);
int scintr(void);
void scsi_hardreset(void);
void scsi_chipreset(struct sc_softc *);
void scsi_softreset(struct sc_softc *);
int sc_busy(struct sc_softc *, int);

static int WAIT_STATR_BITCLR(int);
static int WAIT_STATR_BITSET(int);
static void SET_CMD(struct sc_softc *, int);
static void SET_CNT(int);
static int GET_CNT(void);
static void GET_INTR(uint8_t *, uint8_t *);
static void sc_start(struct sc_softc *);
static void sc_resel(struct sc_softc *);
static void sc_discon(struct sc_softc *);
static void sc_pmatch(struct sc_softc *);
static void flush_fifo(struct sc_softc *);
static void sc_cout(struct sc_softc *, struct sc_chan_stat *);
static void sc_min(struct sc_softc *, struct sc_chan_stat *);
static void sc_mout(struct sc_softc *, struct sc_chan_stat *);
static void sc_sin(struct sc_softc *, volatile struct sc_chan_stat *);
static void sc_dio(struct sc_softc *, volatile struct sc_chan_stat *);
static void sc_dio_pad(struct sc_softc *, volatile struct sc_chan_stat *);
static void print_scsi_stat(struct sc_softc *);
static void append_wb(struct sc_softc *, struct sc_chan_stat *);
static struct sc_chan_stat *get_wb_chan(struct sc_softc *);
static int release_wb(struct sc_softc *);
static void adjust_transfer(struct sc_softc *, struct sc_chan_stat *);
static void clean_k2dcache(struct sc_scb *);

extern void sc_done(struct sc_scb *);
extern paddr_t kvtophys(vaddr_t);

#if defined(__mips__) && defined(CPU_SINGLE)
#define dma_reset(x) do {					\
	int __s = splscsi();					\
	dmac_gsel = (x); dmac_cctl = DM_RST; dmac_cctl = 0;	\
	splx(__s);						\
} while (/* CONSTCOND */ 0)
#endif

int
WAIT_STATR_BITCLR(int bitmask)
{
	int iloop;
	uint8_t dummy;

	iloop = 0;
	do {
		dummy = sc_statr;
		DMAC_WAIT0;
		if (iloop++ > CHECK_LOOP_CNT)
			return -1;
	} while (dummy & bitmask);
	return 0;
}

int
WAIT_STATR_BITSET(int bitmask)
{
	int iloop;
	uint8_t dummy;

	iloop = 0;
	do {
		dummy = sc_statr;
		DMAC_WAIT0;
		if (iloop++ > CHECK_LOOP_CNT)
			return -1;
	} while ((dummy & bitmask) == 0);
	return 0;
}

void
SET_CMD(struct sc_softc *sc, int CMD)
{

	(void)WAIT_STATR_BITCLR(R0_CIP);
	sc->lastcmd = CMD;
	sc_comr = CMD;
	DMAC_WAIT0;
}

void
SET_CNT(int COUNT)
{

	sc_tclow = COUNT & 0xff;
	DMAC_WAIT0;
	sc_tcmid = (COUNT >> 8) & 0xff;
	DMAC_WAIT0;
	sc_tchi = (COUNT >> 16) & 0xff;
	DMAC_WAIT0;
}

int
GET_CNT(void)
{
	int COUNT;

	COUNT = sc_tclow;
	DMAC_WAIT0;
	COUNT += (sc_tcmid << 8) & 0xff00;
	DMAC_WAIT0;
	COUNT += (sc_tchi << 16) & 0xff0000;
	DMAC_WAIT0;
	return COUNT;
}

void
GET_INTR(uint8_t *DATA1, uint8_t *DATA2)
{

	(void)WAIT_STATR_BITCLR(R0_CIP);
	while (sc_statr & R0_MIRQ) {
		DMAC_WAIT0;
		*DATA1 |= sc_intrq1;
		DMAC_WAIT0;
		*DATA2 |= sc_intrq2;
		DMAC_WAIT0;
	}
}


void
sc_send(struct sc_scb *scb, int chan, int ie)
{
	struct sc_softc *sc = scb->scb_softc;
	struct sc_chan_stat *cs;
	struct scsipi_xfer *xs;
	int i;
	uint8_t *p;

	cs = &sc->chan_stat[chan];
	xs = scb->xs;

	p = (uint8_t *)xs->cmd;
	if (cs->scb != NULL) {
		printf("SCSI%d: sc_send() NOT NULL cs->sc\n", chan);
		printf("ie=0x%x scb=%p cs->sc=%p\n", ie, scb, cs->scb);
		printf("cdb=");
		for (i = 0; i < 6; i++)
			printf(" 0x%x", *p++);
		printf("\n");
		panic("SCSI soft error");
		/*NOTREACHED*/
	}

	if (p[0] == SCOP_RESET && p[1] == SCOP_RESET) {
		/*
		 * SCSI bus reset command procedure
		 *	(vender unique by Sony Corp.)
		 */
#ifdef SCSI_1185AQ
		if (sc_idenr & 0x08)
			sc->scsi_1185AQ = 1;
		else
			sc->scsi_1185AQ = 0;
#endif
		cs->scb = scb;
		scsi_hardreset();
		scb->istatus = INST_EP;
		cs->scb = NULL;
		sc_done(scb);
		return;
	}

	if (scb->sc_map && (scb->sc_map->mp_pages > 0)) {
		/*
		 * use map table
		 */
		scb->sc_coffset = scb->sc_map->mp_offset & PGOFSET;
		if (scb->sc_map->mp_pages > NSCMAP) {
			printf("SCSI%d: map table overflow\n", chan);
			scb->istatus = INST_EP|INST_LB|INST_PRE;
			return;
		}
	} else {
		/*
		 * no use map table
		 */
		scb->sc_coffset = (u_int)scb->sc_cpoint & PGOFSET;
	}
	scb->sc_ctag = 0;

	cs->scb = scb;
	cs->comflg = OFF;

	cs->intr_flg = ie;
	cs->chan_num = chan;
	sc->perr_flag[chan] = 0;
	sc->mout_flag[chan] = 0;
	sc->min_cnt[chan] = 0;

	sc->sel_stat[chan] = SEL_WAIT;
	append_wb(sc, cs);
	sc_start(sc);
}

/*
 *	SCSI start up routine
 */
void
sc_start(struct sc_softc *sc)
{
	struct sc_chan_stat *cs;
	int chan, s;
	uint8_t dummy;

	s = splscsi();
	cs = get_wb_chan(sc);
	if ((cs == NULL) || (sc->ipc >= 0))
		goto sc_start_exit;
	chan = cs->chan_num;
	if (sc->sel_stat[chan] != SEL_WAIT) {
		/*
		 * already started
		 */
		goto sc_start_exit;
	}
	sc->sel_stat[chan] = SEL_START;

	dummy = sc_cmonr;
	DMAC_WAIT0;
	if (dummy & (R4_MBSY|R4_MSEL)) {
		sc->sel_stat[chan] = SEL_WAIT;
		goto sc_start_exit;
	}

	/*
	 *	send SELECT with ATN command
	 */
	sc->dma_stat = OFF;
	sc->pad_start = 0;
	dummy = sc_statr;
	DMAC_WAIT0;
	if (dummy & R0_CIP) {
		sc->sel_stat[chan] = SEL_WAIT;
		goto sc_start_exit;
	}
	sc_idenr = (chan << SC_TG_SHIFT) | SC_OWNID;
	DMAC_WAIT0;
#ifdef SCSI_1185AQ
	if (sc->scsi_1185AQ)
		sc_intok1 = Ra_STO|Ra_ARBF;
	else
		sc_intok1 = Ra_STO|Ra_RSL|Ra_ARBF;
#else
	sc_intok1 = Ra_STO|Ra_RSL|Ra_ARBF;
#endif
	DMAC_WAIT0;
	/*
	 * BUGFIX for signal reflection on BSY
	 *	!Rb_DCNT
	 */
	sc_intok2 = Rb_FNC|Rb_SRST|Rb_PHC|Rb_SPE;
	DMAC_WAIT0;

	dummy = sc_cmonr;
	DMAC_WAIT0;
	if (dummy & (R4_MBSY|R4_MSEL)) {
		sc->sel_stat[chan] = SEL_WAIT;
		goto sc_start_exit;
	}
	SET_CMD(sc, SCMD_SEL_ATN);

sc_start_exit:
	splx(s);
}

/*
 *	SCSI interrupt service routine
 */
int
scintr(void)
{
	int iloop;
	int chan;
	uint8_t dummy;
	struct sc_softc *sc;
	struct sc_chan_stat *cs;
	uint8_t s_int1, s_int2;

	sc = device_lookup_private(&sc_cd, 0);		/* XXX */

scintr_loop:

#if defined(CHECK_MRQ) && defined(news3400)
	while (dmac_gstat & CH_MRQ(CH_SCSI))
		DMAC_WAIT;
#endif

	for (iloop = 0; iloop < 100; iloop++) {
		dummy = sc_statr;
		DMAC_WAIT;
		if ((dummy & R0_CIP) == 0)
			break;
	}

	/*
	 * get SCSI interrupt request
	 */
	while (sc_statr & R0_MIRQ) {
		DMAC_WAIT0;
		s_int1 = sc_intrq1;
		DMAC_WAIT0;
		s_int2 = sc_intrq2;
		DMAC_WAIT0;
		sc->int_stat1 |= s_int1;
		sc->int_stat2 |= s_int2;
	}

	if (sc->int_stat2 & R3_SRST) {
		/*
		 * RST signal is drived
		 */
		sc->int_stat2 &= ~R3_SRST;
		scsi_softreset(sc);
		goto scintr_exit;
	}

	if ((sc->ipc < 0) && (sc->wrc <= 0) && (sc->wbc <= 0)) {
		sc->int_stat1 = 0;
		sc->int_stat2 = 0;
		goto scintr_exit;
	}

	cs = get_wb_chan(sc);
	if (cs)
		chan = cs->chan_num;

	if (cs && (sc->sel_stat[chan] == SEL_START) &&
		(sc->lastcmd == SCMD_SEL_ATN)) {
		/*
		 *	Check the result of SELECTION command
		 */
		if (sc->int_stat1 & R2_RSL) {
			/*
			 * RESELECTION occur
			 */
			if (sc->wrc > 0) {
				sc->sel_stat[chan] = SEL_RSLD;
			} else {
				/*
				 * Ghost RESELECTION ???
				 */
				sc->int_stat1 &= ~R2_RSL;
			}
		}
		if (sc->int_stat1 & R2_ARBF) {
			/*
			 * ARBITRATION fault
			 */
			sc->int_stat1 &= ~R2_ARBF;
			sc->sel_stat[chan] = SEL_ARBF;
		}
		if (sc->int_stat1 & R2_STO) {
			/*
			 * SELECTION timeout
			 */
			sc->int_stat1 &= ~R2_STO;
			if ((sc->int_stat2&(R3_PHC|R3_RMSG)) !=
			     (R3_PHC|R3_RMSG)) {
				sc->ipc = chan;
				sc->ip = &sc->chan_stat[chan];
				sc->sel_stat[chan] = SEL_TIMEOUT;
				sc->chan_stat[chan].scb->istatus
					= INST_EP|INST_TO;
				release_wb(sc);
			}
		}

		/*
		 *	SELECTION command done
		 */
		switch (sc->sel_stat[chan]) {

		case SEL_START:
			if ((sc->int_stat2 & R3_FNC) == 0)
				break;
			/*
			 * SELECTION success
			 */
			sc_intok2 = Rb_FNC|Rb_DCNT|Rb_SRST|Rb_PHC|Rb_SPE;
			sc->ipc = chan;
			sc->ip = &sc->chan_stat[chan];
			sc->ip->scb->istatus |= INST_IP;
			sc->dma_stat = OFF;
			sc->pad_start = 0;
			sc->sel_stat[chan] = SEL_SUCCESS;
			release_wb(sc);
#ifndef NOT_SUPPORT_SYNCTR
			sc_syncr = sc->sync_tr[chan];
			DMAC_WAIT0;
#endif
			DMAC_WAIT0;
			break;

		case SEL_TIMEOUT:
			/*
			 * SELECTION time out
			 */
			sc_discon(sc);
			goto scintr_exit;

		/* case SEL_RSLD: */
		/* case SEL_ARBF: */
		default:
			/*
			 * SELECTION failed
			 */
			sc->sel_stat[chan] = SEL_WAIT;
			break;
		}
		if ((sc->int_stat1 & R2_RSL) == 0)
			sc->int_stat2 &= ~R3_FNC;
	}

	if (sc->ip != NULL) {
		/*
		 * check In Process channel's request
		 */
		if (sc->dma_stat != OFF) {
			/*
			 * adjust pointer & counter
			 */
			adjust_transfer(sc, sc->ip);
		}
		if (sc->int_stat2 & R3_SPE) {
			int volatile statr;
			int volatile cmonr;

			statr = sc_statr;
			__USE(statr);
			DMAC_WAIT0;
			cmonr = sc_cmonr;
			__USE(cmonr);
			sc->int_stat2 &= ~R3_SPE;
			sc->perr_flag[sc->ip->chan_num] = 1;
		}
	}

	if (sc->int_stat2 & R3_DCNT) {
		/*
		 * Bus Free
		 */
		sc_discon(sc);
		sc->int_stat2 &= ~R3_DCNT;
	}

	if ((sc->ipc >= 0) && (sc->sel_stat[sc->ipc] == SEL_RSL_WAIT)) {
		sc->sel_stat[sc->ipc] = SEL_RSLD;
		sc->ipc = -1;
		sc->int_stat1 |= R2_RSL;
	}
	if (sc->int_stat1 & R2_RSL) {
		/*
		 * Reselection
		 */
		sc_resel(sc);
		sc->int_stat1 &= ~R2_RSL;
		if (sc->sel_stat[sc->ipc] == SEL_RSL_WAIT)
			goto scintr_exit;
	}


	if ((sc->ipc >= 0) && (sc->ipc != SC_OWNID) &&
	    (sc->sel_stat[sc->ipc] == SEL_SUCCESS)) {
		if (sc->int_stat2 & R3_PHC) {
			/*
			 * Phase change
			 */
			sc->int_stat2 &= ~(R3_PHC|R3_RMSG);
			sc_pmatch(sc);
		} else if (sc->int_stat2 & R3_RMSG) {
			/*
			 * message Phase
			 */
			if (sc->min_flag > 0) {
				sc->int_stat2 &= ~(R3_PHC|R3_RMSG);
				sc_pmatch(sc);
			}
		}
		else if (sc->dma_stat != OFF) {
			dummy = sc_cmonr;
			DMAC_WAIT0;
			if ((dummy & (R4_MMSG|R4_MCD|R4_MREQ)) == R4_MREQ) {
				/*
				 * still DATA transfer phase
				 */
				sc_dio_pad(sc, sc->ip);
			}
		}
		else if (sc->ip->comflg == CF_SEND) {
			dummy = sc_cmonr;
			DMAC_WAIT0;
			if ((dummy & SC_PMASK) == COM_OUT) {
				/*
				 * command out phase
				 */
				sc_cout(sc, sc->ip);
			}
		}
	} else {
		if (sc->int_stat2 & (R3_PHC|R3_RMSG))
			goto scintr_exit;
	}

	if ((sc->int_stat1 & (R2_STO|R2_RSL|R2_ARBF))
	    || (sc->int_stat2 & (R3_DCNT|R3_SRST|R3_PHC|R3_SPE))) {
		/*
		 * still remain intrq
		 */
		goto scintr_loop;
	}

scintr_exit:
	return 1;
}

/*
 *	SCSI bus reset routine
 *		scsi_hardreset() is occered a reset interrupt.
 *		And call scsi_softreset().
 */
void
scsi_hardreset(void)
{
	int s;
#ifdef DMAC_MAP_INIT
	int i;
#endif
	struct sc_softc *sc;

	sc = device_lookup_private(&sc_cd, 0);	/* XXX */
	s = splscsi();

	scsi_chipreset(sc);
	DMAC_WAIT0;
	sc->int_stat1 = 0;
	sc->int_stat2 = 0;
	SET_CMD(sc, SCMD_AST_RST);			/* assert RST signal */

#ifdef DMAC_MAP_INIT
	if (dmac_map_init == 0) {
		dmac_map_init++;
		for (i = 0; i < NDMACMAP; i++) {
# if defined(__mips__) && defined(CPU_SINGLE)
			dmac_gsel = CH_SCSI;
			dmac_ctag = (uint8_t)i;
			dmac_cmap = (uint16_t)0;
# endif
		}
	}
#endif
	/*cxd1185_init();*/
	splx(s);
}

/*
 * I/O port (sc_ioptr) bit assign
 *
 *	Rf_PRT3		-	<reserved>
 *	Rf_PRT2		-	<reserved>
 *	Rf_PRT1		out	Floppy Disk Density control
 *	Rf_PRT0		out	Floppy Disk Eject control
 */

void
scsi_chipreset(struct sc_softc *sc)
{
	int s;
	uint8_t save_ioptr;

	s = splscsi();

#if defined(__mips__) && defined(CPU_SINGLE)
	dmac_gsel = CH_SCSI;
	dmac_cwid = 4;				/* initialize DMAC SCSI chan */
	*(volatile uint8_t *)PINTEN |= DMA_INTEN;
	dma_reset(CH_SCSI);
#endif
	sc_envir = 0;				/* 1/4 clock */
	DMAC_WAIT0;
	save_ioptr = sc_ioptr;
	DMAC_WAIT0;
	sc->lastcmd = SCMD_CHIP_RST;
	sc_comr = SCMD_CHIP_RST;		/* reset chip */
	DMAC_WAIT;
	(void)WAIT_STATR_BITCLR(R0_CIP);
	/*
	 * SCMD_CHIP_RST command reset all register
	 *				except sc_statr<7:6> & sc_cmonr.
	 * So, bit R0_MIRQ & R3_FNC will be not set.
	 */
	sc_idenr = SC_OWNID;
	DMAC_WAIT0;

	sc_intok1 = Ra_STO|Ra_RSL|Ra_ARBF;
	DMAC_WAIT0;
	sc_intok2 = Rb_FNC|Rb_SRST|Rb_PHC|Rb_SPE|Rb_RMSG;
	DMAC_WAIT0;

	sc_ioptr = save_ioptr;
	DMAC_WAIT;

	sc_moder = Rc_TMSL;			/* RST drive time = 25.5 us */
	DMAC_WAIT0;
	sc_timer = 0x2;
	DMAC_WAIT0;

	sc_moder = Rc_SPHI;			/* selection timeout = 252 ms */
	DMAC_WAIT0;
	sc_timer = SEL_TIMEOUT_VALUE;
	DMAC_WAIT0;

#ifdef SCSI_1185AQ
	if (sc->scsi_1185AQ)
		SET_CMD(sc, SCMD_ENB_SEL);		/* enable reselection */
#endif

	sc->int_stat1 &= ~R2_RSL;		/* ignore RSL inter request */

	splx(s);
}

void
scsi_softreset(struct sc_softc *sc)
{
	struct sc_chan_stat *cs;
	int i;
	/* int (*handler)(); */

	sc->wbq_actf = NULL;
	sc->wbq_actl = NULL;
	sc->wbc = 0;
	sc->wrc = 0;
	sc->ip = NULL;
	sc->ipc = -1;
	sc->dma_stat = OFF;
	sc->pad_start = 0;

	for (i = 0; i < NTARGET; ++i) {
		if (i == SC_OWNID)
			continue;
		cs = &sc->chan_stat[i];
		cs->wb_next = NULL;
#ifndef NOT_SUPPORT_SYNCTR
		sc->sync_tr[i] = 0;		/* asynchronous mode */
#endif
		sc->sel_stat[i] = SEL_WAIT;
		if (cs->scb != NULL) {
			struct sc_scb *scb = cs->scb;

			if ((cs->scb->istatus & INST_EP) == 0)
				cs->scb->istatus = (INST_EP|INST_HE);
			cs->scb = NULL;
#ifdef __mips__
			clean_k2dcache(scb);
#endif
			if (cs->intr_flg == SCSI_INTEN) {
				intrcnt[SCSI_INTR]++;
#if 0
				handler = scintsw[i].sci_inthandler;
				if (handler)
					(*handler)(scintsw[i].sci_ctlr);
#endif
			}
			sc_done(scb);
		}
	}
}

/*
 *	RESELECTION interrupt service routine
 *		( RESELECTION phase )
 */
void
sc_resel(struct sc_softc *sc)
{
	struct sc_chan_stat *cs;
	uint8_t chan;
	uint8_t statr;
	int iloop;

	sc->min_flag = 0;
	chan = (sc_idenr & R6_SID_MASK) >> SC_TG_SHIFT;

	if (chan == SC_OWNID)
		return;

	statr = sc_statr;
	DMAC_WAIT0;
	if (statr & R0_CIP) {
		if (sc->lastcmd == SCMD_SEL_ATN) {
			/*
			 * SELECTION command dead lock ?
			 *	save interrupt request
			 */
			while (sc_statr & R0_MIRQ) {
				DMAC_WAIT0;
				sc->int_stat1 |= sc_intrq1;
				DMAC_WAIT0;
				sc->int_stat2 |= sc_intrq2;
				DMAC_WAIT0;
			}
			scsi_chipreset(sc);
		}
	}

	cs = &sc->chan_stat[chan];
	if (cs->scb == NULL) {
		scsi_hardreset();
		return;
	}
	if ((cs->scb->istatus & INST_WR) == 0) {
		scsi_hardreset();
		return;
	}

	if (sc->ipc >= 0) {
		scsi_hardreset();
		return;
	}

	sc->ip = cs;
	sc->ipc = chan;

	sc_intok2 = Rb_FNC|Rb_DCNT|Rb_SRST|Rb_PHC|Rb_SPE;
	DMAC_WAIT0;

	iloop = 0;
	while ((sc->int_stat2 & R3_FNC) == 0) {
		/*
		 * Max 6 usec wait
		 */
		if (iloop++ > RSL_LOOP_CNT) {
			sc->sel_stat[chan] = SEL_RSL_WAIT;
			return;
		}
		GET_INTR(&sc->int_stat1, &sc->int_stat2);
	}
	sc->int_stat2 &= ~R3_FNC;

	sc->sel_stat[chan] = SEL_SUCCESS;

	sc->wrc--;
	sc->dma_stat = OFF;
	sc->pad_start = 0;
	cs->scb->istatus |= INST_IP;
	cs->scb->istatus &= ~INST_WR;

#ifndef NOT_SUPPORT_SYNCTR
	sc_syncr = sc->sync_tr[chan];
	DMAC_WAIT0;
#endif
}

/*
 *	DISCONNECT interrupt service routine
 *		( Target disconnect / job done )
 */
void
sc_discon(struct sc_softc *sc)
{
	struct sc_chan_stat *cs;
	/* int (*handler)(); */
	uint8_t dummy;

	/*
	 * Signal reflection on BSY has occurred.
	 *	Not Bus Free Phase, ignore.
	 *
	 *	But, CXD1185Q reset INIT bit of sc_statr.
	 *	So, can't issue Transfer Information command.
	 *
	 *	What shall we do ?  Bus reset ?
	 */
	if ((sc->int_stat2 & R3_DCNT) && ((sc_intok2 & Rb_DCNT) == 0))
		return;

	sc_intok2 = Rb_FNC|Rb_SRST|Rb_PHC|Rb_SPE;
	DMAC_WAIT0;

	sc->min_flag = 0;
	dummy = sc_cmonr;
	DMAC_WAIT0;
	if (dummy & R4_MATN) {
		SET_CMD(sc, SCMD_NGT_ATN);
		(void) WAIT_STATR_BITSET(R0_MIRQ);
		GET_INTR(&sc->int_stat1, &sc->int_stat2); /* clear interrupt */
	}

	if ((sc->int_stat1 & R2_RSL) == 0)
		sc->int_stat2 &= ~R3_FNC;

	cs = sc->ip;
	if ((cs == NULL) || (sc->ipc < 0))
		goto sc_discon_exit;

	if ((sc->sel_stat[cs->chan_num] != SEL_SUCCESS)
			&& (sc->sel_stat[cs->chan_num] != SEL_TIMEOUT))
		printf("%s: eh!\n", __func__);

	/*
	 * indicate abnormal terminate
	 */
	if ((cs->scb->istatus & (INST_EP|INST_WR)) == 0)
		cs->scb->istatus |= (INST_EP|INST_PRE|INST_LB);

	cs->scb->istatus &= ~INST_IP;
	sc->dma_stat = OFF;
	sc->pad_start = 0;
	sc->ip = NULL;
	sc->ipc = -1;

	if ((cs->scb->istatus & INST_WR) == 0) {
		struct sc_scb *scb = cs->scb;

		if (sc->perr_flag[cs->chan_num] > 0)
			cs->scb->istatus |= INST_EP|INST_PRE;
		cs->scb = NULL;
#ifdef __mips__
		clean_k2dcache(scb);
#endif
		if (cs->intr_flg == SCSI_INTEN) {
			intrcnt[SCSI_INTR]++;
#if 0
			handler = scintsw[cs->chan_num].sci_inthandler;
			if (handler)
				(*handler)(scintsw[cs->chan_num].sci_ctlr);
#endif
		}
		sc_done(scb);
	}

sc_discon_exit:
	sc_start(sc);
}

/*
 *	SCSI phase match interrupt service routine
 */
void
sc_pmatch(struct sc_softc *sc)
{
	struct sc_chan_stat *cs;
	uint8_t phase;
	uint8_t phase2;
	uint8_t cmonr;

	sc->int_stat2 &= ~R3_FNC;			/* XXXXXXXX */

	cs = sc->ip;
	if (cs == NULL)
		return;

#if defined(__mips__) && defined(CPU_SINGLE)
	dma_reset(CH_SCSI);
#endif
	phase = sc_cmonr & SC_PMASK;
	DMAC_WAIT0;
	for (;;) {
		phase2 = phase;
		cmonr = sc_cmonr;
		DMAC_WAIT0;
		phase = cmonr & SC_PMASK;
		if (phase == phase2) {
			if ((phase == DAT_IN) || (phase == DAT_OUT))
				break;
			else if (cmonr & R4_MREQ)
				break;
		}
	}


	sc->dma_stat = OFF;
	sc->pad_start = 0;

	if (phase == COM_OUT) {
		sc->min_flag = 0;
		if (cs->comflg != CF_SEND)
			cs->comflg = CF_SET;
		sc_cout(sc, cs);
	} else {
		cs->comflg = CF_ENOUGH;
		sc_intok2 &= ~Rb_FNC;
		if (phase == MES_IN) {
			sc->min_flag++;
			sc_min(sc, cs);
		} else {
			sc->min_flag = 0;

			switch (phase) {

			case MES_OUT:
				sc_mout(sc, cs);
				break;

			case DAT_IN:
			case DAT_OUT:
				sc_dio(sc, cs);
				break;

			case STAT_IN:
				sc_sin(sc, cs);
				break;

			default:
				printf("SCSI%d: unknown phase\n", cs->chan_num);
				break;
			}
		}
	}
}


void
flush_fifo(struct sc_softc *sc)
{
	uint8_t dummy;
	uint8_t tmp;
	uint8_t tmp0;

	dummy = sc_ffstr;
	DMAC_WAIT0;
	if (dummy & R5_FIFOREM) {
		/*
		 * flush FIFO
		 */
		SET_CMD(sc, SCMD_FLSH_FIFO);
		tmp = 0;
		do {
			do {
				dummy = sc_statr;
				DMAC_WAIT0;
			} while (dummy & R0_CIP);
			GET_INTR(&tmp0, &tmp); /* clear interrupt */
		} while ((tmp & R3_FNC) == 0);
	}
}

/*
 *	SCSI command send routine
 */
void
sc_cout(struct sc_softc *sc, struct sc_chan_stat *cs)
{
	int iloop;
	int cdb_bytes;
	uint8_t dummy;
	uint8_t statr;
	struct scsipi_xfer *xs;

	if (cs->comflg == CF_SET) {
		struct sc_scb *scb = cs->scb;

		cs->comflg = CF_SEND;

		flush_fifo(sc);

		xs = scb->xs;
		cdb_bytes = xs->cmdlen;

		switch (xs->cmd->opcode & CMD_TYPEMASK) {
		case CMD_T0:
		case CMD_T1:
		case CMD_T5:
			break;

		default:
			cdb_bytes = 6;
			sc_intok2 |= Rb_FNC;
			break;
		}

		/*
		 * set Active pointers
		 */
		sc->act_cmd_pointer = (char *)xs->cmd;
		cs->act_trcnt = scb->sc_ctrnscnt;
		cs->act_point = scb->sc_cpoint;
		cs->act_tag = scb->sc_ctag;
		cs->act_offset = scb->sc_coffset;

	} else {
		cdb_bytes = 1;
		iloop = 0;
		do {
			dummy = sc_cmonr;
			DMAC_WAIT0;
			if ((dummy & SC_PMASK) != COM_OUT)
				return;
			statr = sc_statr;
			DMAC_WAIT0;
			if (statr & R0_MIRQ)
				return;
		} while ((dummy & R4_MREQ) == 0);
		statr = sc_statr;
		DMAC_WAIT0;
		if (statr & R0_MIRQ)
			return;
	}


	SET_CNT(cdb_bytes);
	SET_CMD(sc, SCMD_TR_INFO|R0_TRBE);

	for (iloop = 0; iloop < cdb_bytes; iloop++) {
		do {
			dummy = sc_cmonr;
			DMAC_WAIT0;
			if ((dummy & SC_PMASK) != COM_OUT)
				return;
		} while ((dummy & R4_MREQ) == 0);
		statr = sc_statr;
		DMAC_WAIT0;
		if (statr & R0_MIRQ)
			return;
		sc_datr = *sc->act_cmd_pointer++;
		do {
			dummy = sc_cmonr;
			DMAC_WAIT0;
		} while ((dummy & R4_MACK) != 0);
	}
}

#define GET_MIN_COUNT	127

/*
 *	SCSI message accept routine
 */
void
sc_min(struct sc_softc *sc, struct sc_chan_stat *cs)
{
	struct sc_scb *scb = cs->scb;
	struct scsipi_xfer *xs = scb->xs;
	uint8_t dummy;

	sc_intok2 = Rb_FNC|Rb_DCNT|Rb_SRST|Rb_PHC|Rb_SPE|Rb_RMSG;
	DMAC_WAIT0;

	if (sc->min_flag == 1)
		flush_fifo(sc);

	dummy = sc_cmonr;
	DMAC_WAIT0;
	if ((dummy & R4_MREQ) == 0) {
		printf("sc_min: !REQ cmonr=%x\n", dummy);
		print_scsi_stat(sc);
		scsi_hardreset();
		return;
	}

/*  retry_cmd_issue: */
	sc->int_stat2 &= ~R3_FNC;
	SET_CMD(sc, SCMD_TR_INFO);
	do {
		do {
			dummy = sc_statr;
			DMAC_WAIT0;
		} while (dummy & R0_CIP);
		GET_INTR(&sc->int_stat1, &sc->int_stat2); /* clear interrupt */
	} while ((sc->int_stat2 & R3_FNC) == 0);
	sc->int_stat2 &= ~R3_FNC;

	dummy = sc_ffstr;
	if (dummy & R5_FIE) {
		DMAC_WAIT;
		dummy = sc_ffstr;
		DMAC_WAIT0;
		if (dummy & R5_FIE) {
			dummy = sc_statr;
			DMAC_WAIT0;
			if ((dummy & R0_INIT) == 0) {
				/*
				 * CXD1185 detect BSY false
				 */
				scsi_hardreset();
				return;
			}
		}
	}
	dummy = sc_datr;				/* get message byte */
	DMAC_WAIT0;

	if (sc->min_cnt[cs->chan_num] == 0) {
		scb->message = scb->identify;
		if (dummy == MSG_EXTND) {
			/* Extended Message */
			sc->min_cnt[cs->chan_num] = GET_MIN_COUNT;
			sc->min_point[cs->chan_num] = scb->msgbuf;
			memset(scb->msgbuf, 0, 8);
			*sc->min_point[cs->chan_num]++ = dummy;
		} else {
			switch ((dummy & MSG_IDENT)? MSG_IDENT : dummy) {

			case MSG_CCOMP:
				scb->istatus |= INST_EP;
				break;

			case MSG_MREJ:
#ifndef NOT_SUPPORT_SYNCTR
				if (sc->mout_flag[cs->chan_num] == MOUT_SYNC_TR)
					sc->sync_tr[cs->chan_num] = 0;
#endif
				break;

			case MSG_IDENT:
			case MSG_RDP:

				sc->dma_stat = OFF;
				sc->pad_start = 0;
				cs->comflg = OFF;
				/*
			 	 * restore the saved value to Active pointers
			 	 */
				sc->act_cmd_pointer = (char *)xs->cmd;
				cs->act_trcnt = scb->sc_ctrnscnt;
				cs->act_point = scb->sc_cpoint;
				cs->act_tag = scb->sc_ctag;
				cs->act_offset = scb->sc_coffset;
				break;

			case MSG_SDP:
				/*
				 * save Active pointers
				 */
				scb->sc_ctrnscnt = cs->act_trcnt;
				scb->sc_ctag = cs->act_tag;
				scb->sc_coffset = cs->act_offset;
				scb->sc_cpoint = cs->act_point;
				break;

			case MSG_DCNT:
				scb->istatus |= INST_WR;
				sc->wrc++;
				break;

			default:
				scb->message = MSG_MREJ;
				SET_CMD(sc, SCMD_AST_ATN);
				printf("SCSI%d:sc_min() Unknown mes=0x%x, \n",
					cs->chan_num, dummy);
			}
		}
	} else {
		*sc->min_point[cs->chan_num]++ = dummy;
		if (sc->min_cnt[cs->chan_num] == GET_MIN_COUNT)
			sc->min_cnt[cs->chan_num] = dummy;
		else
			sc->min_cnt[cs->chan_num]--;
		if (sc->min_cnt[cs->chan_num] <= 0) {
#ifdef ABORT_SYNCTR_MES_FROM_TARGET
			if ((scb->msgbuf[2] == 0x01) &&
			    (sc->mout_flag[cs->chan_num] == MOUT_SYNC_TR)) {
#else
			if (scb->msgbuf[2] == 0x01) {
#endif
				int i;
				/*
				 * receive Synchronous transfer message reply
				 *	calculate transfer period val
				 *	tpm * 4/1000 us = 4/16 * (tpv + 1)
				 */
#define	TPM2TPV(tpm)	(((tpm)*16 + 999) / 1000 - 1)
#ifndef NOT_SUPPORT_SYNCTR
				i = scb->msgbuf[3];	/* get tpm */
				i = TPM2TPV(i) << 4;
				if (scb->msgbuf[4] == 0)
					sc->sync_tr[cs->chan_num] = 0;
				else
					sc->sync_tr[cs->chan_num] =
						i | scb->msgbuf[4];
#endif /* !NOT_SUPPORT_SYNCTR */
			} else {
				scb->message = MSG_MREJ;
				SET_CMD(sc, SCMD_AST_ATN);	/* assert ATN */
			}
		}
	}
	SET_CMD(sc, SCMD_NGT_ACK);
}

/*
 *	SCSI message send routine
 */
void
sc_mout(struct sc_softc *sc, struct sc_chan_stat *cs)
{
	struct sc_scb *scb = cs->scb;
	u_char *mp;
	int cnt;
	int iloop;
	uint8_t dummy;
	uint8_t tmp;
	uint8_t tmp0;

	flush_fifo(sc);

	if (sc->mout_flag[cs->chan_num] == 0) {
		sc->mout_flag[cs->chan_num] = MOUT_IDENTIFY;
		if (scb->message != 0) {
			sc_intok2 = Rb_FNC|Rb_DCNT|Rb_SRST|Rb_PHC|Rb_SPE|Rb_RMSG;
			DMAC_WAIT0;
			if ((scb->message == MSG_EXTND)
					&& (scb->msgbuf[2] == 0x01)) {
				cnt = 5;
				mp = scb->msgbuf;
				scb->msgbuf[3] = MIN_TP;
				if (scb->msgbuf[4] > MAX_OFFSET_BYTES)
					scb->msgbuf[4] = MAX_OFFSET_BYTES;
				sc->mout_flag[cs->chan_num] = MOUT_SYNC_TR;
			} else {
				cnt = 1;
				mp = &scb->message;
			}

			SET_CNT(cnt);
			SET_CMD(sc, SCMD_TR_INFO|R0_TRBE);
			sc_datr = scb->identify;
			DMAC_WAIT0;
			for (iloop = 1; iloop < cnt; iloop++) {
				sc_datr = *mp++;
				DMAC_WAIT;
			}
			do {
				dummy = sc_cmonr;
				DMAC_WAIT0;
				if ((dummy & R4_MBSY) == 0)
					return;
				dummy = sc_statr;
				DMAC_WAIT0;
			} while (dummy & R0_CIP);

			tmp = 0;
			GET_INTR(&tmp0, &tmp);		/* clear interrupt */
			if ((tmp & R3_FNC) == 0) {
				(void) WAIT_STATR_BITSET(R0_MIRQ);
				GET_INTR(&tmp0, &tmp);	/* clear interrupt */
			}

			do {
				dummy = sc_cmonr;
				DMAC_WAIT0;
				if ((dummy & R4_MBSY) == 0)
					return;
			} while ((dummy & R4_MREQ) == 0);
			SET_CMD(sc, SCMD_NGT_ATN);
			(void)WAIT_STATR_BITCLR(R0_CIP);
			GET_INTR(&tmp0, &tmp);		/* clear interrupt */

			dummy = sc_cmonr;
			DMAC_WAIT0;
			if ((dummy & R4_MREQ) == 0) {
				printf("sc_mout: !REQ cmonr=%x\n", dummy);
				print_scsi_stat(sc);
				scsi_hardreset();
				return;
			}

			SET_CMD(sc, SCMD_TR_INFO);
			sc_datr = *mp++;
			DMAC_WAIT0;
		} else {
			dummy = sc_cmonr;
			DMAC_WAIT0;
			if (dummy & R4_MATN) {
				SET_CMD(sc, SCMD_NGT_ATN);
				(void) WAIT_STATR_BITCLR(R0_CIP);
				GET_INTR(&tmp0, &tmp);	/* clear interrupt */
			}

			iloop = 0;
			do {
				dummy = sc_cmonr;
				DMAC_WAIT0;
				if (iloop++ > CHECK_LOOP_CNT)
					break;
			} while ((dummy & R4_MREQ) == 0);
			SET_CMD(sc, SCMD_TR_INFO);
			sc_datr = scb->identify;
			DMAC_WAIT0;
		}
	} else {
		dummy = sc_cmonr;
		DMAC_WAIT0;
		if (dummy & R4_MATN) {
			SET_CMD(sc, SCMD_NGT_ATN);
			(void) WAIT_STATR_BITCLR(R0_CIP);
			GET_INTR(&tmp0, &tmp);		/* clear interrupt */
		}

		dummy = sc_cmonr;
		DMAC_WAIT0;
		if ((dummy & R4_MREQ) == 0) {
			printf("sc_mout: !REQ cmonr=%x\n", dummy);
			print_scsi_stat(sc);
			scsi_hardreset();
			return;
		}

		SET_CMD(sc, SCMD_TR_INFO);
		sc_datr = scb->message;
		DMAC_WAIT0;
	}
}

/*
 *	SCSI status accept routine
 */
void
sc_sin(struct sc_softc *sc, volatile struct sc_chan_stat *cs)
{
	uint8_t dummy;
	int iloop;

	flush_fifo(sc);

	dummy = sc_cmonr;
	DMAC_WAIT0;
	if ((dummy & R4_MREQ) == 0) {
		printf("sc_sin: !REQ cmonr=%x\n", dummy);
		print_scsi_stat(sc);
		scsi_hardreset();
		return;
	}

	sc_intok2 = Rb_FNC|Rb_DCNT|Rb_SRST|Rb_PHC|Rb_SPE|Rb_RMSG;
	DMAC_WAIT0;

	SET_CMD(sc, SCMD_TR_INFO);

	(void)WAIT_STATR_BITCLR(R0_CIP);

	sc->int_stat2 &= ~R3_FNC;
	iloop = 0;
	do {
		if (iloop++ > CHECK_LOOP_CNT)
			break;
		GET_INTR(&sc->int_stat1, &sc->int_stat2);	/* clear interrupt */
	} while ((sc->int_stat2 & R3_FNC) == 0);
	sc->int_stat2 &= ~R3_FNC;

	cs->scb->tstatus = sc_datr;		/* get status byte */
	DMAC_WAIT0;
}

/*
 *	SCSI data in/out routine
 */
void
sc_dio(struct sc_softc *sc, volatile struct sc_chan_stat *cs)
{
	struct sc_scb *scb;
	int i;
	int pages;
	uint8_t tag;
	uint32_t pfn;
	uint8_t phase;
	struct scsipi_xfer *xs;

	scb = cs->scb;
	xs = scb->xs;

	sc_intok2 = Rb_FNC|Rb_DCNT|Rb_SRST|Rb_PHC|Rb_SPE;
	DMAC_WAIT0;

	if (cs->act_trcnt <= 0) {
		sc_dio_pad(sc, cs);
		return;
	}

	switch (xs->cmd->opcode) {

	case SCOP_READ:
	case SCOP_WRITE:
	case SCOP_EREAD:
	case SCOP_EWRITE:
		i = (cs->act_trcnt + DEV_BSIZE -1) / DEV_BSIZE;
		i *= DEV_BSIZE;
		break;

	default:
		i = cs->act_trcnt;
		break;
	}

	SET_CNT(i);
	sc->pad_cnt[cs->chan_num] = i - cs->act_trcnt;

	phase = sc_cmonr & SC_PMASK;
	DMAC_WAIT0;
	if (phase == DAT_IN) {
		if (sc_syncr == OFF) {
			DMAC_WAIT0;
			flush_fifo(sc);
		}
	}

#if defined(__mips__) && defined(CPU_SINGLE)
	SET_CMD(sc, SCMD_TR_INFO|R0_DMA|R0_TRBE);
#endif

#if defined(__mips__) && defined(CPU_SINGLE)
	dmac_gsel = CH_SCSI;
	dmac_ctrcl = (uint8_t)(cs->act_trcnt & 0xff);
	dmac_ctrcm = (uint8_t)((cs->act_trcnt >> 8) & 0xff);
	dmac_ctrch = (uint8_t)((cs->act_trcnt >> 16) & 0x0f);
	dmac_cofsh = (uint8_t)((cs->act_offset >> 8) & 0xf);
	dmac_cofsl = (uint8_t)(cs->act_offset & 0xff);
#endif
	tag = 0;

	if (scb->sc_map && (scb->sc_map->mp_pages > 0)) {
		/*
		 * Set DMAC map entry from map table
		 */
		pages = scb->sc_map->mp_pages;
		for (i = cs->act_tag; i < pages; i++) {
			if ((pfn = scb->sc_map->mp_addr[i]) == 0)
				panic("SCSI:sc_dma() zero entry");
#if defined(__mips__) && defined(CPU_SINGLE)
			dmac_gsel = CH_SCSI;
			dmac_ctag = (uint8_t)tag++;
			dmac_cmap = (uint16_t)pfn;
#endif
		}
#ifdef MAP_OVER_ACCESS
# if defined(__mips__) && defined(CPU_SINGLE)
		dmac_gsel = CH_SCSI;
		dmac_ctag = (uint8_t)tag++;
		dmac_cmap = (uint16_t)pfn;
# endif
#endif
	} else {
		/*
		 * Set DMAC map entry from logical address
		 */
		pfn = kvtophys((vaddr_t)cs->act_point) >> PGSHIFT;
		pages = (cs->act_trcnt >> PGSHIFT) + 2;
		for (i = 0; i < pages; i++) {
#if defined(__mips__) && defined(CPU_SINGLE)
			dmac_gsel = CH_SCSI;
			dmac_ctag = (uint8_t)tag++;
			dmac_cmap = (uint8_t)pfn + i;
#endif
		}
	}

#if defined(__mips__) && defined(CPU_SINGLE)
	dmac_gsel = CH_SCSI;
	dmac_ctag = 0;
#endif

	if (phase == DAT_IN) {
		sc->dma_stat = SC_DMAC_RD;
#if defined(__mips__) && defined(CPU_SINGLE)
		/*
		 * auto pad flag is always on
		 */
		dmac_gsel = CH_SCSI;
		dmac_cctl = DM_MODE|DM_APAD;
		DMAC_WAIT;
		dmac_cctl = DM_MODE|DM_APAD|DM_ENABLE;
		DMAC_WAIT0;
#endif
	}
	else if (phase == DAT_OUT) {
		sc->dma_stat = SC_DMAC_WR;
#if defined(__mips__) && defined(CPU_SINGLE)
		dmac_gsel = CH_SCSI;
		dmac_cctl = DM_APAD;
		DMAC_WAIT;
		dmac_cctl = DM_APAD|DM_ENABLE;
		DMAC_WAIT0;
#endif
						/* DMAC start on mem->I/O */
	}
}

#define MAX_TR_CNT24	((1 << 24) -1)
void
sc_dio_pad(struct sc_softc *sc, volatile struct sc_chan_stat *cs)
{
	uint8_t dummy;

	if (cs->act_trcnt >= 0)
		return;
	sc->pad_start = 1;

	SET_CNT(MAX_TR_CNT24);
	SET_CMD(sc, SCMD_TR_PAD|R0_TRBE);
	dummy = sc_cmonr & SC_PMASK;
	DMAC_WAIT0;
	if (dummy == DAT_IN)
		dummy = sc_datr;		/* get data */
	else
		sc_datr = 0;			/* send data */
}

void
print_scsi_stat(struct sc_softc *sc)
{

	printf("ipc=%d wrc=%d wbc=%d\n", sc->ipc, sc->wrc, sc->wbc);
}

/*
 *	return 0 if it was done.  Or retun TRUE if it is busy.
 */
int
sc_busy(struct sc_softc *sc, int chan)
{

	return (int)sc->chan_stat[chan].scb;
}


/*
 *	append channel into Waiting Bus_free queue
 */
void
append_wb(struct sc_softc *sc, struct sc_chan_stat *cs)
{
	int s;

	s = splclock();			/* inhibit process switch */
	if (sc->wbq_actf == NULL)
		sc->wbq_actf = cs;
	else
		sc->wbq_actl->wb_next = cs;
	sc->wbq_actl = cs;
	cs->scb->istatus = INST_WAIT;
	sc->wbc++;
	splx(s);
}

/*
 *	get channel from Waiting Bus_free queue
 */
struct sc_chan_stat *
get_wb_chan(struct sc_softc *sc)
{
	struct sc_chan_stat *cs;
	int s;

	s = splclock();			/* inhibit process switch */
	cs = sc->wbq_actf;
	if (cs && cs->chan_num == SC_OWNID)	/* needed? */
		cs = NULL;
	splx(s);
	return cs;
}

/*
 *	release channel from Waiting Bus_free queue
 */
int
release_wb(struct sc_softc *sc)
{
	struct sc_chan_stat *cs;
	int error = 0;
	int s;

	s = splclock();			/* inhibit process switch */
	if (sc->wbq_actf == NULL) {
		error = -1;
	} else {
		cs = sc->wbq_actf;
		sc->wbq_actf = cs->wb_next;
		cs->wb_next = NULL;
		if (sc->wbq_actl == cs)
			sc->wbq_actl = NULL;
		cs->scb->istatus &= ~INST_WAIT;
		sc->wbc--;
	}
	splx(s);
	return error;
}

void
adjust_transfer(struct sc_softc *sc, struct sc_chan_stat *cs)
{
	struct sc_scb *scb = cs->scb;
	u_int remain_cnt = 0;
	u_int offset, sent_byte;

	if (sc->pad_start) {
		sc->pad_start = 0;
	} else {
# if defined(__mips__) && defined(CPU_SINGLE)
		remain_cnt = GET_CNT();
		remain_cnt -= sc->pad_cnt[cs->chan_num];
		if (sc->dma_stat == SC_DMAC_WR) {
			/*
			 * adjust counter in the FIFO
			 */
			remain_cnt += sc_ffstr & R5_FIFOREM;
		}
# endif
	}

	sent_byte = scb->sc_ctrnscnt - remain_cnt;
	cs->act_trcnt = remain_cnt;

	offset = scb->sc_coffset + sent_byte;
	cs->act_tag += (offset >> PGSHIFT);
	cs->act_offset = offset & PGOFSET;
	if ((scb->sc_map == NULL) || (scb->sc_map->mp_pages <= 0))
		cs->act_point += sent_byte;
}

#ifdef __mips__
static void
clean_k2dcache(struct sc_scb *scb)
{
	struct sc_map *sc_map = scb->sc_map;
	paddr_t pa;
	int i, pages;

	pa = kvtophys((vaddr_t)scb->msgbuf);
	mips_dcache_wbinv_range_index(MIPS_PHYS_TO_KSEG0(pa),
	    sizeof(scb->msgbuf));

	if (MACH_IS_USPACE(scb->sc_cpoint))
		panic("clean_k2dcache: user address is not supported");

	if (MACH_IS_CACHED(scb->sc_cpoint)) {
		mips_dcache_wbinv_range_index((vaddr_t)scb->sc_cpoint,
		    scb->sc_ctrnscnt);
		return;
	}

	if (sc_map) {
		pages = sc_map->mp_pages;
		for (i = 0; i < pages; i++) {
			pa = sc_map->mp_addr[i] << PGSHIFT;
			mips_dcache_wbinv_range_index(MIPS_PHYS_TO_KSEG0(pa),
			    PAGE_SIZE);
		}
	}
}
#endif
