/*
 * Copyright (c) 1994 Michael L. Hitch
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
 *	@(#)siop.c	7.5 (Berkeley) 5/4/91
 *	$Id: siop.c,v 1.16 1994/07/18 08:06:40 chopps Exp $
 */

/*
 * AMIGA 53C710 scsi adaptor driver
 */

/* need to know if any tapes have been configured */
#include "st.h"
 
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>
#include <machine/cpu.h>
#include <amiga/amiga/custom.h>
#include <amiga/dev/siopreg.h>
#include <amiga/dev/siopvar.h>

extern u_int	kvtop();

/*
 * SCSI delays
 * In u-seconds, primarily for state changes on the SPC.
 */
#define	SCSI_CMD_WAIT	500000	/* wait per step of 'immediate' cmds */
#define	SCSI_DATA_WAIT	500000	/* wait per data in/out step */
#define	SCSI_INIT_WAIT	500000	/* wait per step (both) during init */

int  siopicmd __P((struct siop_softc *, int, void *, int, void *, int));
int  siopgo __P((struct siop_softc *, struct scsi_xfer *));
int  siopgetsense __P((struct siop_softc *, struct scsi_xfer *));
void siopabort __P((struct siop_softc *, siop_regmap_p, char *));
void sioperror __P((struct siop_softc *, siop_regmap_p, u_char));
void siopstart __P((struct siop_softc *));
void siopreset __P((struct siop_softc *));
void siopsetdelay __P((int));
void siop_scsidone __P((struct siop_softc *, int));
void siop_donextcmd __P((struct siop_softc *));
int  siopintr __P((struct siop_softc *));

/* 53C710 script */
unsigned long scripts[] = {
	0x47000000, 0x000002d0,			/* 000 -   0 */
	0x838b0000, 0x000000d0,			/* 008 -   8 */
	0x7a1b1000, 0x00000000,			/* 010 -  16 */
	0x828a0000, 0x00000088,			/* 018 -  24 */
	0x9e020000, 0x0000ff01,			/* 020 -  32 */
	0x72350000, 0x00000000,			/* 028 -  40 */
	0x808c0000, 0x00000048,			/* 030 -  48 */
	0x58000008, 0x00000000,			/* 038 -  56 */
	0x1e000024, 0x00000024,			/* 040 -  64 */
	0x838b0000, 0x00000090,			/* 048 -  72 */
	0x1f00002c, 0x0000002c,			/* 050 -  80 */
	0x838b0000, 0x00000080,			/* 058 -  88 */
	0x868a0000, 0xffffffd0,			/* 060 -  96 */
	0x838a0000, 0x00000070,			/* 068 - 104 */
	0x878a0000, 0x00000158,			/* 070 - 112 */
	0x80880000, 0x00000028,			/* 078 - 120 */
	0x1e000004, 0x00000004,			/* 080 - 128 */
	0x838b0000, 0x00000050,			/* 088 - 136 */
	0x868a0000, 0xffffffe8,			/* 090 - 144 */
	0x838a0000, 0x00000040,			/* 098 - 152 */
	0x878a0000, 0x00000128,			/* 0a0 - 160 */
	0x9a020000, 0x0000ff02,			/* 0a8 - 168 */
	0x1a00000c, 0x0000000c,			/* 0b0 - 176 */
	0x878b0000, 0x00000168,			/* 0b8 - 184 */
	0x838a0000, 0x00000018,			/* 0c0 - 192 */
	0x818a0000, 0x000000e8,			/* 0c8 - 200 */
	0x808a0000, 0x000000b8,			/* 0d0 - 208 */
	0x98080000, 0x0000ff03,			/* 0d8 - 216 */
	0x1b000014, 0x00000014,			/* 0e0 - 224 */
	0x72090000, 0x00000000,			/* 0e8 - 232 */
	0x6a340000, 0x00000000,			/* 0f0 - 240 */
	0x9f030000, 0x0000ff04,			/* 0f8 - 248 */
	0x1f00001c, 0x0000001c,			/* 100 - 256 */
	0x808c0007, 0x00000050,			/* 108 - 264 */
	0x98040000, 0x0000ff26,			/* 110 - 272 */
	0x60000040, 0x00000000,			/* 118 - 280 */
	0x48000000, 0x00000000,			/* 120 - 288 */
	0x7c1bef00, 0x00000000,			/* 128 - 296 */
	0x72340000, 0x00000000,			/* 130 - 304 */
	0x980c0002, 0x0000fffc,			/* 138 - 312 */
	0x980c0008, 0x0000fffb,			/* 140 - 320 */
	0x980c0018, 0x0000fffd,			/* 148 - 328 */
	0x98040000, 0x0000fffe,			/* 150 - 336 */
	0x98080000, 0x0000ff00,			/* 158 - 344 */
	0x60000008, 0x00000000,			/* 160 - 352 */
	0x98080000, 0x0000ff26,			/* 168 - 360 */
	0x60000040, 0x00000000,			/* 170 - 368 */
	0x828b0000, 0xffffff28,			/* 178 - 376 */
	0x838b0000, 0xffffff58,			/* 180 - 384 */
	0x878b0000, 0xffffff68,			/* 188 - 392 */
	0x18000034, 0x00000034,			/* 190 - 400 */
	0x808b0000, 0x000001c0,			/* 198 - 408 */
	0x838b0000, 0xffffff38,			/* 1a0 - 416 */
	0x878a0000, 0x000000d0,			/* 1a8 - 424 */
	0x98080000, 0x0000ff05,			/* 1b0 - 432 */
	0x19000034, 0x00000034,			/* 1b8 - 440 */
	0x818b0000, 0x00000160,			/* 1c0 - 448 */
	0x80880000, 0xffffffd0,			/* 1c8 - 456 */
	0x1f00001c, 0x0000001c,			/* 1d0 - 464 */
	0x808c0001, 0x00000018,			/* 1d8 - 472 */
	0x980c0002, 0x0000ff08,			/* 1e0 - 480 */
	0x808c0004, 0x00000020,			/* 1e8 - 488 */
	0x98080000, 0x0000ff06,			/* 1f0 - 496 */
	0x60000040, 0x00000000,			/* 1f8 - 504 */
	0x1f00002c, 0x0000002c,			/* 200 - 512 */
	0x98080000, 0x0000ff07,			/* 208 - 520 */
	0x60000040, 0x00000000,			/* 210 - 528 */
	0x48000000, 0x00000000,			/* 218 - 536 */
	0x98080000, 0x0000ff09,			/* 220 - 544 */
	0x1f00001c, 0x0000001c,			/* 228 - 552 */
	0x808c0001, 0x00000018,			/* 230 - 560 */
	0x980c0002, 0x0000ff10,			/* 238 - 568 */
	0x808c0004, 0x00000020,			/* 240 - 576 */
	0x98080000, 0x0000ff11,			/* 248 - 584 */
	0x60000040, 0x00000000,			/* 250 - 592 */
	0x1f00002c, 0x0000002c,			/* 258 - 600 */
	0x98080000, 0x0000ff12,			/* 260 - 608 */
	0x60000040, 0x00000000,			/* 268 - 616 */
	0x48000000, 0x00000000,			/* 270 - 624 */
	0x98080000, 0x0000ff13,			/* 278 - 632 */
	0x1f00001c, 0x0000001c,			/* 280 - 640 */
	0x808c0001, 0x00000018,			/* 288 - 648 */
	0x980c0002, 0x0000ff14,			/* 290 - 656 */
	0x808c0004, 0x00000020,			/* 298 - 664 */
	0x98080000, 0x0000ff15,			/* 2a0 - 672 */
	0x60000040, 0x00000000,			/* 2a8 - 680 */
	0x1f00002c, 0x0000002c,			/* 2b0 - 688 */
	0x98080000, 0x0000ff16,			/* 2b8 - 696 */
	0x60000040, 0x00000000,			/* 2c0 - 704 */
	0x48000000, 0x00000000,			/* 2c8 - 712 */
	0x98080000, 0x0000ff17,			/* 2d0 - 720 */
	0x54000000, 0x00000040,			/* 2d8 - 728 */
	0x9f030000, 0x0000ff18,			/* 2e0 - 736 */
	0x1f00001c, 0x0000001c,			/* 2e8 - 744 */
	0x990b0000, 0x0000ff19,			/* 2f0 - 752 */
	0x980a0000, 0x0000ff20,			/* 2f8 - 760 */
	0x9f0a0000, 0x0000ff21,			/* 300 - 768 */
	0x9b0a0000, 0x0000ff22,			/* 308 - 776 */
	0x9e0a0000, 0x0000ff23,			/* 310 - 784 */
	0x98080000, 0x0000ff24,			/* 318 - 792 */
	0x98080000, 0x0000ff25,			/* 320 - 800 */
	0x76100800, 0x00000000,			/* 328 - 808 */
	0x80840700, 0x00000008,			/* 330 - 816 */
	0x7e110100, 0x00000000,			/* 338 - 824 */
	0x6a100000, 0x00000000,			/* 340 - 832 */
	0x19000034, 0x00000034,			/* 348 - 840 */
	0x818b0000, 0xffffffd0,			/* 350 - 848 */
	0x98080000, 0x0000ff27,			/* 358 - 856 */
	0x76100800, 0x00000000,			/* 360 - 864 */
	0x80840700, 0x00000008,			/* 368 - 872 */
	0x7e110100, 0x00000000,			/* 370 - 880 */
	0x6a100000, 0x00000000,			/* 378 - 888 */
	0x18000034, 0x00000034,			/* 380 - 896 */
	0x808b0000, 0xffffffd0,			/* 388 - 904 */
	0x98080000, 0x0000ff27	/* 390 - 912 */
};

#define	Ent_msgout	0x00000018
#define	Ent_cmd	0x000000a8
#define	Ent_status	0x000000e0
#define	Ent_msgin	0x000000f8
#define	Ent_dataout	0x00000190
#define	Ent_datain	0x000001b8

/* default to not inhibit sync negotiation on any drive */
/* XXXX - unit 2 inhibits sync for my WangTek tape drive - mlh */
u_char siop_inhibit_sync[8] = { 0, 0, 0, 0, 0, 0, 0 }; /* initialize, so patchable */
int siop_no_dma = 0;

int siop_reset_delay = 2000;	/* delay after reset, in milleseconds */
int siop_sync_period = 50;	/* synchronous transfer period, in nanoseconds */

int siop_cmd_wait = SCSI_CMD_WAIT;
int siop_data_wait = SCSI_DATA_WAIT;
int siop_init_wait = SCSI_INIT_WAIT;

static struct {
	unsigned char x;	/* period from sync request message */
	unsigned char y;	/* siop_period << 4 | sbcl */
} xxx[] = {
	{0x0f, 0x01},
	{0x13, 0x11},
	{0x17, 0x21},
/*	{0x17, 0x02},	*/
	{0x1b, 0x31},
	{0x1d, 0x12},
	{0x1e, 0x41},
/*	{0x1e, 0x03},	*/
	{0x22, 0x51},
	{0x23, 0x22},
	{0x26, 0x61},
/*	{0x26, 0x13},	*/
	{0x29, 0x32},
	{0x2a, 0x71},
	{0x2d, 0x23},
	{0x2e, 0x42},
	{0x34, 0x52},
	{0x35, 0x33},
	{0x3a, 0x62},
	{0x3c, 0x43},
	{0x40, 0x72},
	{0x44, 0x53},
	{0x4b, 0x63},
	{0x53, 0x73}
};

#ifdef DEBUG
#define QPRINTF(a) if (siop_debug > 1) printf a
/*
 *	0x01 - full debug
 *	0x02 - DMA chaining
 *	0x04 - siopintr
 *	0x08 - phase mismatch
 *	0x10 - panic on phase mismatch
 *	0x20 - panic on unhandled exceptions
 */
int	siop_debug = 0;
int	siopsync_debug = 0;
int	siopdma_hits = 0;
int	siopdma_misses = 0;
int	siopchain_ints = 0;
#else
#define QPRINTF(a)
#endif


/*
 * default minphys routine for siop based controllers
 */
void
siop_minphys(bp)
	struct buf *bp;
{
	/*
	 * no max transfer at this level
	 */
}

/*
 * must be used
 */
u_int
siop_adinfo()
{
	/* 
	 * one request at a time please
	 */
	return(1);
}

/*
 * used by specific siop controller
 *
 * it appears that the higher level code does nothing with LUN's
 * so I will too.  I could plug it in, however so could they
 * in scsi_scsi_cmd().
 */
int
siop_scsicmd(xs)
	struct scsi_xfer *xs;
{
	struct siop_pending *pendp;
	struct siop_softc *dev;
	struct scsi_link *slp;
	int flags, s;

	slp = xs->sc_link;
	dev = slp->adapter_softc;
	flags = xs->flags;

	if (flags & SCSI_DATA_UIO)
		panic("siop: scsi data uio requested");
	
	if (dev->sc_xs && flags & SCSI_NOMASK)
		panic("siop_scsicmd: busy");

	s = splbio();
	pendp = &dev->sc_xsstore[slp->target][slp->lun];
	if (pendp->xs) {
		splx(s);
		return(TRY_AGAIN_LATER);
	}

	if (dev->sc_xs) {
		pendp->xs = xs;
		TAILQ_INSERT_TAIL(&dev->sc_xslist, pendp, link);
		splx(s);
		return(SUCCESSFULLY_QUEUED);
	}
	pendp->xs = NULL;
	dev->sc_xs = xs;
	splx(s);

	/*
	 * nothing is pending do it now.
	 */
	siop_donextcmd(dev);

	if (flags & SCSI_NOMASK)
		return(COMPLETE);
	return(SUCCESSFULLY_QUEUED);
}

/*
 * entered with dev->sc_xs pointing to the next xfer to perform
 */
void
siop_donextcmd(dev)
	struct siop_softc *dev;
{
	struct scsi_xfer *xs;
	struct scsi_link *slp;
	int flags, phase, stat;

	xs = dev->sc_xs;
	slp = xs->sc_link;
	flags = xs->flags;

#if 0
	if (flags & SCSI_DATA_IN)
		phase = DATA_IN_PHASE;
	else if (flags & SCSI_DATA_OUT)
		phase = DATA_OUT_PHASE;
	else
		phase = STATUS_PHASE;
#endif

	if (flags & SCSI_RESET)
		siopreset(dev);

	dev->sc_stat[0] = -1;
#if 0
	if (phase == STATUS_PHASE || flags & SCSI_NOMASK) 
#else
	if (flags & SCSI_NOMASK || siop_no_dma) 
#endif
		stat = siopicmd(dev, slp->target, xs->cmd, xs->cmdlen, 
		    xs->data, xs->datalen/*, phase*/);
	else if (siopgo(dev, xs) == 0)
		return;
	else 
		stat = dev->sc_stat[0];
	
	siop_scsidone(dev, stat);
}

void
siop_scsidone(dev, stat)
	struct siop_softc *dev;
	int stat;
{
	struct siop_pending *pendp;
	struct scsi_xfer *xs;
	int s, donext;

	xs = dev->sc_xs;
#ifdef DIAGNOSTIC
	if (xs == NULL)
		panic("siop_scsidone");
#endif
	/*
	 * is this right?
	 */
	xs->status = stat;

	if (stat == 0 || xs->flags & SCSI_ERR_OK)
		xs->resid = 0;
	else {
		switch(stat) {
		case SCSI_CHECK:
			if (stat = siopgetsense(dev, xs))
				goto bad_sense;
			xs->error = XS_SENSE;
			break;
		case SCSI_BUSY:
			xs->error = XS_BUSY;
			break;
		bad_sense:
		default:
			xs->error = XS_DRIVER_STUFFUP;
			QPRINTF(("siop_scsicmd() bad %x\n", stat));
			break;
		}
	}
	xs->flags |= ITSDONE;

	/*
	 * grab next command before scsi_done()
	 * this way no single device can hog scsi resources.
	 */
	s = splbio();
	pendp = dev->sc_xslist.tqh_first;
	if (pendp == NULL) {
		donext = 0;
		dev->sc_xs = NULL;
	} else {
		donext = 1;
		TAILQ_REMOVE(&dev->sc_xslist, pendp, link);
		dev->sc_xs = pendp->xs;
		pendp->xs = NULL;
	}
	splx(s);
	scsi_done(xs);

	if (donext)
		siop_donextcmd(dev);
}

int
siopgetsense(dev, xs)
	struct siop_softc *dev;
	struct scsi_xfer *xs;
{
	struct scsi_sense rqs;
	struct scsi_link *slp;
	int stat;

	slp = xs->sc_link;
	
	rqs.op_code = REQUEST_SENSE;
	rqs.byte2 = slp->lun << 5;
#ifdef not_yet
	rqs.length = xs->req_sense_length ? xs->req_sense_length : 
	    sizeof(xs->sense);
#else
	rqs.length = sizeof(xs->sense);
#endif
	if (rqs.length > sizeof (xs->sense))
		rqs.length = sizeof (xs->sense);
	rqs.unused[0] = rqs.unused[1] = rqs.control = 0;
	
	return(siopicmd(dev, slp->target, &rqs, sizeof(rqs), &xs->sense,
	    rqs.length));
}

void
siopabort(dev, regs, where)
	register struct siop_softc *dev;
	siop_regmap_p regs;
	char *where;
{

	printf ("%s: abort %s: dstat %02x, sstat0 %02x sbcl %02x\n", 
	    dev->sc_dev.dv_xname,
	    where, regs->siop_dstat, regs->siop_sstat0, regs->siop_sbcl);

	if (dev->sc_flags & SIOP_SELECTED) {
#ifdef TODO
      SET_SBIC_cmd (regs, SBIC_CMD_ABORT);
      WAIT_CIP (regs);

      GET_SBIC_asr (regs, asr);
      if (asr & (SBIC_ASR_BSY|SBIC_ASR_LCI))
        {
          /* ok, get more drastic.. */
          
	  SET_SBIC_cmd (regs, SBIC_CMD_RESET);
	  delay(25);
	  SBIC_WAIT(regs, SBIC_ASR_INT, 0);
	  GET_SBIC_csr (regs, csr);       /* clears interrupt also */

          dev->sc_flags &= ~SIOP_SELECTED;
          return;
        }

      do
        {
          SBIC_WAIT (regs, SBIC_ASR_INT, 0);
          GET_SBIC_csr (regs, csr);
        }
      while ((csr != SBIC_CSR_DISC) && (csr != SBIC_CSR_DISC_1)
	      && (csr != SBIC_CSR_CMD_INVALID));
#endif

		/* lets just hope it worked.. */
		dev->sc_flags &= ~SIOP_SELECTED;
	}
}

/*
 * XXX Set/reset long delays.
 *
 * if delay == 0, reset default delays
 * if delay < 0,  set both delays to default long initialization values
 * if delay > 0,  set both delays to this value
 *
 * Used when a devices is expected to respond slowly (e.g. during
 * initialization).
 */
void
siop_delay(delay)
	int delay;
{
	static int saved_cmd_wait, saved_data_wait;

	if (delay) {
		saved_cmd_wait = siop_cmd_wait;
		saved_data_wait = siop_data_wait;
		if (delay > 0)
			siop_cmd_wait = siop_data_wait = delay;
		else
			siop_cmd_wait = siop_data_wait = siop_init_wait;
	} else {
		siop_cmd_wait = saved_cmd_wait;
		siop_data_wait = saved_data_wait;
	}
}

void
siopinitialize(dev)
	struct siop_softc *dev;
{
	/*
	 * Need to check that scripts is on a long word boundary
	 * and that DS is on a long word boundary.
	 * Also need to verify that dev doesn't non-contiguous
	 * physical pages.
	 */
	dev->sc_scriptspa = kvtop(scripts);
	dev->sc_dspa = kvtop(&dev->sc_ds);
	dev->sc_lunpa = kvtop(&dev->sc_lun);
	dev->sc_statuspa = kvtop(&dev->sc_stat[0]);
	dev->sc_msgpa = kvtop(&dev->sc_msg[0]);
	siopreset (dev);
}

void
siopreset(dev)
	struct siop_softc *dev;
{
	siop_regmap_p regs;
	u_int i, s;
	u_char  my_id, csr;

	regs = dev->sc_siopp;

	if (dev->sc_flags & SIOP_ALIVE)
		siopabort(dev, regs, "reset");
		
	printf("%s: ", dev->sc_dev.dv_xname);		/* XXXX */

	s = splbio();
	my_id = 7;

	/*
	 * Reset the chip
	 * XXX - is this really needed?
	 */
	regs->siop_sien &= ~SIOP_SIEN_RST;
	regs->siop_scntl1 |= SIOP_SCNTL1_RST;
	for (i = 0; i < 1000; ++i)
		;
	regs->siop_scntl1 &= ~SIOP_SCNTL1_RST;
	regs->siop_sien |= SIOP_SIEN_RST;

	/*
	 * Set up various chip parameters
	 */
	regs->siop_istat = 0x40;
	for (i = 0; i < 1000; ++i)
		;
	regs->siop_istat = 0x00;
	regs->siop_scntl0 = SIOP_ARB_FULL | SIOP_SCNTL0_EPC | SIOP_SCNTL0_EPG;
	regs->siop_dcntl = dev->sc_clock_freq & 0xff;
	regs->siop_dmode = 0x80;	/* burst length = 4 */
	regs->siop_sien = 0x00;	/* don't enable interrupts yet */
	regs->siop_dien = 0x00;	/* don't enable interrupts yet */
	regs->siop_scid = 1 << my_id;
	regs->siop_dwt = 0x00;
	regs->siop_ctest0 |= 0x20;	/* Enable Active Negation ?? */
	regs->siop_ctest7 |= (dev->sc_clock_freq >> 8) & 0xff;

	/* will need to re-negotiate sync xfers */
	bzero(&dev->sc_sync, sizeof (dev->sc_sync));

	splx (s);

 	delay (siop_reset_delay * 1000);
	printf("siop id %d reset\n", my_id);
	dev->sc_flags |= SIOP_ALIVE;
	dev->sc_flags &= ~(SIOP_SELECTED | SIOP_DMA);
}

/*
 * Setup Data Storage for 53C710 and start SCRIPTS processing
 */

void
siop_setup (dev, target, cbuf, clen, buf, len)
	struct siop_softc *dev;
	int target;
	u_char *cbuf;
	int clen;
	u_char *buf;
	int len;
{
	siop_regmap_p regs = dev->sc_siopp;
	int i;
	int nchain;
	int count, tcount;
	char *addr, *dmaend;

	dev->sc_istat = 0;
	dev->sc_lun = 0x80;			/* XXX */
	dev->sc_stat[0] = -1;
	dev->sc_msg[0] = -1;
	dev->sc_ds.scsi_addr = (0x10000 << target) | (dev->sc_sync[target].period << 8);
	dev->sc_ds.idlen = 1;
	dev->sc_ds.idbuf = (char *) dev->sc_lunpa;
	dev->sc_ds.cmdlen = clen;
	dev->sc_ds.cmdbuf = (char *) kvtop(cbuf);
	dev->sc_ds.stslen = 1;
	dev->sc_ds.stsbuf = (char *) dev->sc_statuspa;
	dev->sc_ds.msglen = 1;
	dev->sc_ds.msgbuf = (char *) dev->sc_msgpa;
	dev->sc_ds.sdtrolen = 0;
	dev->sc_ds.sdtrilen = 0;
	bzero(&dev->sc_ds.chain, sizeof (dev->sc_ds.chain));

	if (dev->sc_sync[target].state == SYNC_START) {
		if (siop_inhibit_sync[target]) {
			dev->sc_sync[target].state = SYNC_DONE;
			dev->sc_sync[target].offset = 0;
			dev->sc_sync[target].period = 0;
#ifdef DEBUG
			if (siopsync_debug)
				printf ("Forcing target %d asynchronous\n", target);
#endif
		}
		else {
			dev->sc_msg[1] = MSG_IDENTIFY;
			dev->sc_msg[2] = MSG_EXT_MESSAGE;
			dev->sc_msg[3] = 3;
			dev->sc_msg[4] = MSG_SYNC_REQ;
			dev->sc_msg[5] = siop_sync_period / 4;
			dev->sc_msg[6] = SIOP_MAX_OFFSET;
			dev->sc_ds.sdtrolen = 6;
			dev->sc_ds.sdtrilen = 6;
			dev->sc_ds.sdtrobuf = dev->sc_ds.sdtribuf = (char *) (dev->sc_msgpa + 1);
			dev->sc_sync[target].state = SYNC_SENT;
#ifdef DEBUG
			if (siopsync_debug)
				printf ("Sending sync request to target %d\n", target);
#endif
		}
	}

/*
 * If length is > 1 page, check for consecutive physical pages
 * Need to set up chaining if not
 */
	nchain = 0;
	count = len;
	addr = buf;
	dmaend = NULL;
	while (count > 0) {
		dev->sc_ds.chain[nchain].databuf = (char *) kvtop (addr);
		if (count < (tcount = NBPG - ((int) addr & PGOFSET)))
			tcount = count;
		dev->sc_ds.chain[nchain].datalen = tcount;
		addr += tcount;
		count -= tcount;
		if (dev->sc_ds.chain[nchain].databuf == dmaend) {
			dmaend += dev->sc_ds.chain[nchain].datalen;
			dev->sc_ds.chain[--nchain].datalen += tcount;
#ifdef DEBUG
			++siopdma_hits;
#endif
		}
		else {
			dmaend = dev->sc_ds.chain[nchain].databuf +
			    dev->sc_ds.chain[nchain].datalen;
			dev->sc_ds.chain[nchain].datalen = tcount;
#ifdef DEBUG
			if (nchain)	/* Don't count miss on first one */
				++siopdma_misses;
#endif
		}
		++nchain;
		if (nchain < DMAMAXIO)	/* force error if buffer too small */
			dev->sc_ds.chain[nchain].datalen = 0;
	}
#ifdef DEBUG
	if (nchain != 1 && len != 0 && siop_debug & 3) {
		printf ("DMA chaining set: %d\n", nchain);
		for (i = 0; i < nchain; ++i) {
			printf ("  [%d] %8x %4x\n", i, dev->sc_ds.chain[i].databuf,
			    dev->sc_ds.chain[i].datalen);
		}
	}
#endif

	regs->siop_sbcl = dev->sc_sync[target].offset;
	if (dev->sc_ds.sdtrolen)
		regs->siop_scratch = regs->siop_scratch | 0x100;
	else
		regs->siop_scratch = regs->siop_scratch & ~0xff00;
	regs->siop_dsa = dev->sc_dspa;
	/* push data case on things the 53c710 needs to access */
	dma_cachectl (dev, sizeof (struct siop_softc));
	dma_cachectl (cbuf, clen);
	if (buf != NULL && len != 0)
		dma_cachectl (buf, len);
	regs->siop_dsp = dev->sc_scriptspa;
}

/*
 * Process a DMA or SCSI interrupt from the 53C710 SIOP
 */

int
siop_checkintr(dev, istat, dstat, sstat0, status)
	struct	siop_softc *dev;
	u_char	istat;
	u_char	dstat;
	u_char	sstat0;
	int	*status;
{
	siop_regmap_p regs = dev->sc_siopp;
	int	target;

	regs->siop_ctest8 |= 0x04;
	while ((regs->siop_ctest1 & SIOP_CTEST1_FMT) == 0)
		;
	regs->siop_ctest8 &= ~0x04;
#ifdef DEBUG
	if (siop_debug & 1) {
		DCIAS(dev->sc_statuspa);	/* XXX */
		printf ("siopchkintr: istat %x dstat %x sstat0 %x dsps %x sbcl %x sts %x msg %x\n",
		    istat, dstat, sstat0, regs->siop_dsps, regs->siop_sbcl, dev->sc_stat[0], dev->sc_msg[0]);
	}
#endif
	if (dstat & SIOP_DSTAT_SIR && (regs->siop_dsps == 0xff00 ||
	    regs->siop_dsps == 0xfffc)) {
		/* Normal completion status, or check condition */
		if (regs->siop_dsa != dev->sc_dspa) {
			printf ("siop: invalid dsa: %x %x\n", regs->siop_dsa,
			    dev->sc_dspa);
			panic("*** siop DSA invalid ***");
		}
		target = dev->sc_slave;
		if (dev->sc_sync[target].state == SYNC_SENT) {
#ifdef DEBUG
			if (siopsync_debug)
				printf ("sync msg in: %02x %02x %02x %02x %02x %02x\n",
				    dev->sc_msg[1], dev->sc_msg[2], dev->sc_msg[3],
				    dev->sc_msg[4], dev->sc_msg[5], dev->sc_msg[6]);
#endif
			if (dev->sc_msg[0] == MSG_REJECT)
				printf ("target %d sync request was rejected\n",
				    target);
			dev->sc_sync[target].state = SYNC_DONE;
			dev->sc_sync[target].period = 0;
			dev->sc_sync[target].offset = 0;
			if (dev->sc_msg[1] == MSG_EXT_MESSAGE &&
			    dev->sc_msg[2] == 3 &&
			    dev->sc_msg[3] == MSG_SYNC_REQ &&
			    dev->sc_msg[5] != 0) {
				if (dev->sc_msg[4] && dev->sc_msg[4] < 100 / 4) {
#ifdef DEBUG
					printf ("%d: target %d wanted %dns period\n",
					    dev->sc_dev.dv_xname, target,
					    dev->sc_msg[4] * 4);
#endif
					/*
					 * Kludge for Maxtor XT8580S
					 * It accepts whatever we request, even
					 * though it won't work.  So we ask for
					 * a short period than we can handle.  If
					 * the device says it can do it, use 208ns.
					 * If the device says it can do less than
					 * 100ns, then we limit it to 100ns.
					 */
					if (dev->sc_msg[4] == siop_sync_period / 4)
						dev->sc_msg[4] = 208 / 4;
					else
						dev->sc_msg[4] = 100 / 4;
				}
				printf ("%s: target %d now synchronous, period=%dns, offset=%d\n",
				    dev->sc_dev.dv_xname, target,
				    dev->sc_msg[4] * 4, dev->sc_msg[5]);
				scsi_period_to_siop (dev, target);
			}
		}
#if 0
		DCIAS(dev->sc_statuspa);	/* XXX */
#else
		dma_cachectl(&dev->sc_stat[0], 1);
#endif
		*status = dev->sc_stat[0];
		return 1;
	}
	if (sstat0 & SIOP_SSTAT0_M_A) {		/* Phase mismatch */
#ifdef DEBUG
		if (siop_debug & 9)
			printf ("Phase mismatch: %x dsp +%x\n", regs->siop_sbcl,
			    regs->siop_dsp - dev->sc_scriptspa);
		if (siop_debug & 0x10)
			panic ("53c710 phase mismatch");
#endif
		if ((regs->siop_sbcl & SIOP_REQ) == 0)
			printf ("Phase mismatch: REQ not asserted! %02x\n",
			    regs->siop_sbcl);
		switch (regs->siop_sbcl & 7) {
/*
 * For data out and data in phase, check for DMA chaining
 */

/*
 * for message in, check for possible reject for sync request
 */
		case 0:
			regs->siop_dsp = dev->sc_scriptspa + Ent_dataout;
			break;
		case 1:
			regs->siop_dsp = dev->sc_scriptspa + Ent_datain;
			break;
		case 2:
			regs->siop_dsp = dev->sc_scriptspa + Ent_cmd;
			break;
		case 3:
			regs->siop_dsp = dev->sc_scriptspa + Ent_status;
			break;
		case 6:
			regs->siop_dsp = dev->sc_scriptspa + Ent_msgout;
			break;
		case 7:
			regs->siop_dsp = dev->sc_scriptspa + Ent_msgin;
			break;
		default:
			goto bad_phase;
		}
		return 0;
	}
	if (sstat0 & SIOP_SSTAT0_STO) {		/* Select timed out */
		*status = -1;
		return 1;
	}
	if (dstat & SIOP_DSTAT_SIR && regs->siop_dsps == 0xff05 &&
	    (regs->siop_sbcl & (SIOP_MSG | SIOP_CD)) == 0) {
		printf ("DMA chaining failed\n");
		siopreset (dev);
		*status = -1;
		return 1;
	}
	if (dstat & SIOP_DSTAT_SIR && regs->siop_dsps == 0xff27) {
#ifdef DEBUG
		if (siop_debug & 3)
			printf ("DMA chaining completed: dsa %x dnad %x addr %x\n",
				regs->siop_dsa,	regs->siop_dnad, regs->siop_addr);
		++siopchain_ints;
#endif
		regs->siop_dsa = dev->sc_dspa;
		regs->siop_dsp = dev->sc_scriptspa + Ent_status;
		return 0;
	}
	target = dev->sc_slave;
	if (dstat & SIOP_DSTAT_SIR && regs->siop_dsps == 0xff26 &&
	    dev->sc_msg[0] == MSG_REJECT && dev->sc_sync[target].state == SYNC_SENT) {
		dev->sc_sync[target].state = SYNC_DONE;
		dev->sc_sync[target].period = 0;
		dev->sc_sync[target].offset = 0;
		dev->sc_ds.sdtrolen = 0;
		dev->sc_ds.sdtrilen = 0;
#ifdef DEBUG
		if (siopsync_debug || 1)
			printf ("target %d rejected sync, going asynchronous\n", target);
#endif
		siop_inhibit_sync[target] = -1;
		if ((regs->siop_sbcl & 7) == 6) {
			regs->siop_dsp = dev->sc_scriptspa + Ent_msgout;
			return (0);
		}
		regs->siop_dcntl |= SIOP_DCNTL_STD;
		return (0);
	}
	if ((dstat & SIOP_DSTAT_SIR && regs->siop_dsps == 0xff13) ||
	    sstat0 & SIOP_SSTAT0_UDC) {
#ifdef DEBUG
		printf ("%s: target %d disconnected unexpectedly\n",
		   dev->sc_dev.dv_xname, target);
#endif
#if 0
		siopabort (dev, regs, "siopchkintr");
#endif
		*status = STS_BUSY;
		return 1;
	}
	if (dstat & SIOP_DSTAT_SIR &&regs->siop_dsps == 0xfffb) {
#if 0
		printf ("%s: target %d busy\n", dev->sc_dev.dv_xname, target);
#endif
#if 0
		siopabort (dev, regs, "siopchkintr");
#endif
		*status = STS_BUSY;
		return 1;
	}
	if (sstat0 == 0 && dstat & SIOP_DSTAT_SIR) {
#if 0
		DCIAS(dev->sc_statuspa);
#else
		dma_cachectl (&dev->sc_stat[0], 1);
#endif
		printf ("SIOP interrupt: %x sts %x msg %x sbcl %x\n",
		    regs->siop_dsps, dev->sc_stat[0], dev->sc_msg[0],
		    regs->siop_sbcl);
		siopreset (dev);
		*status = -1;
		return 1;
	}
	if (sstat0 & SIOP_SSTAT0_SGE)
		printf ("SIOP: SCSI Gross Error\n");
	if (sstat0 & SIOP_SSTAT0_PAR)
		printf ("SIOP: Parity Error\n");
	if (dstat & SIOP_DSTAT_OPC)
		printf ("SIOP: Invalid SCRIPTS Opcode\n");
bad_phase:
	/*
	 * temporary panic for unhandled conditions
	 * displays various things about the 53C710 status and registers
	 * then panics.
	 * XXXX need to clean this up to print out the info, reset, and continue
	 */
	printf ("siopchkintr: target %x ds %x\n", target, &dev->sc_ds);
	printf ("scripts %x ds %x regs %x dsp %x dcmd %x\n", dev->sc_scriptspa,
	    dev->sc_dspa, kvtop(regs), regs->siop_dsp,
	    *((long *)&regs->siop_dcmd));
	printf ("siopchkintr: istat %x dstat %x sstat0 %x dsps %x dsa %x sbcl %x sts %x msg %x\n",
	    istat, dstat, sstat0, regs->siop_dsps, regs->siop_dsa,
	     regs->siop_sbcl, dev->sc_stat[0], dev->sc_msg[0]);
#ifdef DEBUG
	if (siop_debug & 0x20)
		panic("siopchkintr: **** temp ****");
#endif
	siopreset (dev);		/* hard reset */
	*status = -1;
	return 1;
}

/*
 * SCSI 'immediate' command:  issue a command to some SCSI device
 * and get back an 'immediate' response (i.e., do programmed xfer
 * to get the response data).  'cbuf' is a buffer containing a scsi
 * command of length clen bytes.  'buf' is a buffer of length 'len'
 * bytes for data.  The transfer direction is determined by the device
 * (i.e., by the scsi bus data xfer phase).  If 'len' is zero, the
 * command must supply no data.  'xferphase' is the bus phase the
 * caller expects to happen after the command is issued.  It should
 * be one of DATA_IN_PHASE, DATA_OUT_PHASE or STATUS_PHASE.
 *
 * XXX - 53C710 will use DMA, but no interrupts (it's a heck of a
 * lot easier to do than to use programmed I/O).
 *
 */
int
siopicmd(dev, target, cbuf, clen, buf, len)
	struct siop_softc *dev;
	int target;
	void *cbuf;
	int clen;
	void *buf;
	int len;
{
	siop_regmap_p regs = dev->sc_siopp;
	int i;
	int status;
	u_char istat;
	u_char dstat;
	u_char sstat0;

	if (dev->sc_flags & SIOP_SELECTED) {
		printf ("siopicmd%d: bus busy\n", target);
		return -1;
	}
	regs->siop_sien = 0x00;		/* disable SCSI and DMA interrupts */
	regs->siop_dien = 0x00;
	dev->sc_flags |= SIOP_SELECTED;
	dev->sc_slave = target;
#ifdef DEBUG
	if (siop_debug & 1)
		printf ("siopicmd: target %x cmd %02x ds %x\n", target,
		    *((char *)cbuf), &dev->sc_ds);
#endif
	siop_setup (dev, target, cbuf, clen, buf, len);

	for (;;) {
		/* use cmd_wait values? */
		i = siop_cmd_wait << 1;
		while (((istat = regs->siop_istat) &
		    (SIOP_ISTAT_SIP | SIOP_ISTAT_DIP)) == 0) {
			if (--i <= 0) {
				printf ("waiting: tgt %d cmd %02x sbcl %02x dsp %x (+%x) dcmd %x ds %x\n",
				    target, *((char *)cbuf),
				    regs->siop_sbcl, regs->siop_dsp,
				    regs->siop_dsp - dev->sc_scriptspa,
				    *((long *)&regs->siop_dcmd), &dev->sc_ds);
				i = siop_cmd_wait << 2;
				/* XXXX need an upper limit and reset */
			}
			delay(1);
		}
		dstat = regs->siop_dstat;
		sstat0 = regs->siop_sstat0;
#ifdef DEBUG
		if (siop_debug & 1) {
			DCIAS(dev->sc_statuspa);	/* XXX should just invalidate dev->sc_stat */
			printf ("siopicmd: istat %x dstat %x sstat0 %x dsps %x sbcl %x sts %x msg %x\n",
			    istat, dstat, sstat0, regs->siop_dsps, regs->siop_sbcl,
			    dev->sc_stat[0], dev->sc_msg[0]);
		}
#endif
		if (siop_checkintr(dev, istat, dstat, sstat0, &status)) {
			dev->sc_flags &= ~SIOP_SELECTED;
			return (status);
		}
	}
}

int
siopgo(dev, xs)
	struct siop_softc *dev;
	struct scsi_xfer *xs;
{
	siop_regmap_p regs;
	int i;
	int nchain;
	int count, tcount;
	char *addr, *dmaend;

#ifdef DEBUG
	if (siop_debug & 1)
		printf ("%s: go ", dev->sc_dev.dv_xname);
#if 0
	if ((cdb->cdb[1] & 1) == 0 &&
	    ((cdb->cdb[0] == CMD_WRITE && cdb->cdb[2] == 0 && cdb->cdb[3] == 0) ||
	    (cdb->cdb[0] == CMD_WRITE_EXT && cdb->cdb[2] == 0 && cdb->cdb[3] == 0
	    && cdb->cdb[4] == 0)))
		panic ("siopgo: attempted write to block < 0x100");
#endif
#endif
#if 0
	cdb->cdb[1] |= unit << 5;
#endif

	if (dev->sc_flags & SIOP_SELECTED) {
		printf ("%s: bus busy\n", dev->sc_dev.dv_xname);
		return 1;
	}

	dev->sc_flags |= SIOP_SELECTED | SIOP_DMA;
	dev->sc_slave = xs->sc_link->target;
	regs = dev->sc_siopp;
	/* enable SCSI and DMA interrupts */
	regs->siop_sien = SIOP_SIEN_M_A | SIOP_SIEN_STO | SIOP_SIEN_SEL | SIOP_SIEN_SGE |
	    SIOP_SIEN_UDC | SIOP_SIEN_RST | SIOP_SIEN_PAR;
	regs->siop_dien = 0x20 | SIOP_DIEN_ABRT | SIOP_DIEN_SIR | SIOP_DIEN_WTD |
	    SIOP_DIEN_OPC;
#ifdef DEBUG
	if (siop_debug & 1)
		printf ("siopgo: target %x cmd %02x ds %x\n", dev->sc_slave, xs->cmd->opcode, &dev->sc_ds);
#endif

	siop_setup(dev, dev->sc_slave, xs->cmd, xs->cmdlen, xs->data, xs->datalen);

	return (0);
}

/*
 * Check for 53C710 interrupts
 */

int
siopintr (dev)
	register struct siop_softc *dev;
{
	siop_regmap_p regs;
	register u_char istat, dstat, sstat0;
	int unit;
	int status;
	int found = 0;

	regs = dev->sc_siopp;
	istat = dev->sc_istat;
	if ((istat & (SIOP_ISTAT_SIP | SIOP_ISTAT_DIP)) == 0)
		return;
	if ((dev->sc_flags & (SIOP_DMA | SIOP_SELECTED)) == SIOP_SELECTED)
		return;	/* doing non-interrupt I/O */
		/* Got a valid interrupt on this device */
	dstat = dev->sc_dstat;
	sstat0 = dev->sc_sstat0;
	dev->sc_istat = 0;
#ifdef DEBUG
	if (siop_debug & 1)
		printf ("%s: intr istat %x dstat %x sstat0 %x\n",
		    dev->sc_dev.dv_xname, istat, dstat, sstat0);
	if ((dev->sc_flags & SIOP_DMA) == 0) {
		printf ("%s: spurious interrupt? istat %x dstat %x sstat0 %x\n",
		    dev->sc_dev.dv_xname, istat, dstat, sstat0);
	}
#endif

#ifdef DEBUG
	if (siop_debug & 5) {
		DCIAS(dev->sc_statuspa);
		printf ("siopintr%d: istat %x dstat %x sstat0 %x dsps %x sbcl %x sts %x msg %x\n",
		    unit, istat, dstat, sstat0, regs->siop_dsps,
		    regs->siop_sbcl, dev->sc_stat[0], dev->sc_msg[0]);
	}
#endif
	if (siop_checkintr (dev, istat, dstat, sstat0, &status)) {
#if 1
		regs->siop_sien = 0;
		regs->siop_dien = 0;
		if (status == 0xff)
			printf ("siopintr: status == 0xff\n");
#endif
		dev->sc_flags &= ~(SIOP_DMA | SIOP_SELECTED);
		siop_scsidone(dev, dev->sc_stat[0]);
	}
}

scsi_period_to_siop (dev, target)
	struct siop_softc *dev;
{
	int period, offset, i, sxfer;

	period = dev->sc_msg[4];
	offset = dev->sc_msg[5];
	sxfer = 0;
	if (offset <= SIOP_MAX_OFFSET)
		sxfer = offset;
	for (i = 0; i < sizeof (xxx) / 2; ++i) {
		if (period <= xxx[i].x) {
			sxfer |= xxx[i].y & 0x70;
			offset = xxx[i].y & 0x03;
			break;
		}
	}
	dev->sc_sync[target].period = sxfer;
	dev->sc_sync[target].offset = offset;
#ifdef DEBUG
	printf ("sync: siop_sxfr %02x, siop_sbcl %02x\n", sxfer, offset);
#endif
}
