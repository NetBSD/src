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
 *	@(#)siop.c	7.5 (Berkeley) 5/4/91
 *	$Id: siop.c,v 1.5 1994/02/21 06:30:44 chopps Exp $
 *
 * MULTICONTROLLER support only working for multiple controllers of the 
 * same kind at the moment !! 
 *
 */

/*
 * AMIGA 53C710 scsi adaptor driver
 */

#include "zeusscsi.h"
#include "magnumscsi.h"
#define NSIOP (NZEUSSCSI + NMAGNUMSCSI)
#if NSIOP > 0

#ifndef lint
static char rcsid[] = "$Header: /cvsroot/src/sys/arch/amiga/dev/siop.c,v 1.5 1994/02/21 06:30:44 chopps Exp $";
#endif

/* need to know if any tapes have been configured */
#include "st.h"
 
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <amiga/dev/device.h>

#include <amiga/dev/scsidefs.h>
#include <amiga/dev/siopvar.h>
#include <amiga/dev/siopreg.h>

#include <amiga/amiga/custom.h>

#include <machine/cpu.h>

extern u_int	kvtop();

/*
 * SCSI delays
 * In u-seconds, primarily for state changes on the SPC.
 */
#define	SCSI_CMD_WAIT	50000	/* wait per step of 'immediate' cmds */
#define	SCSI_DATA_WAIT	50000	/* wait per data in/out step */
#define	SCSI_INIT_WAIT	50000	/* wait per step (both) during init */

void siopstart (int unit);
int siopgo (int ctlr, int slave, int unit, struct buf *bp, struct scsi_fmt_cdb *cdb, int pad);
int siopintr2 (void);
void siopdone (int unit);
int siopustart (int unit);
int siopreq (register struct devqueue *dq);
void siopfree (register struct devqueue *dq);
void siopreset (int unit);
void siop_delay (int delay);
int siop_test_unit_rdy (int ctlr, int slave, int unit);
int siop_start_stop_unit (int ctlr, int slave, int unit, int start);
int siop_request_sense (int ctlr, int slave, int unit, u_char *buf, unsigned int len);
int siop_immed_command (int ctlr, int slave, int unit, struct scsi_fmt_cdb *cdb, u_char *buf, unsigned int len, int rd);
int siop_immed_command_nd (int ctlr, int slave, int unit, struct scsi_fmt_cdb *cdb);
int siop_tt_read (int ctlr, int slave, int unit, u_char *buf, u_int len, daddr_t blk, int bshift);
int siop_tt_write (int ctlr, int slave, int unit, u_char *buf, u_int len, daddr_t blk, int bshift);
#if NST > 0
int siop_tt_oddio (int ctlr, int slave, int unit, u_char *buf, u_int len, int b_flags, int freedma);
#endif

#if NZEUSSCSI > 0
int zeusscsiinit();

struct driver zeusscsidriver = {
	zeusscsiinit, "Zeusscsi", (int (*)())siopstart, (int (*)(int,...))siopgo,
	(int (*)())siopintr2, (int (*)())siopdone,
	siopustart, siopreq, siopfree, siopreset,
	siop_delay, siop_test_unit_rdy, siop_start_stop_unit,
	siop_request_sense, siop_immed_command, siop_tt_read, siop_tt_write,
#if NST > 0
	siop_tt_oddio
#else
	0
#endif
};
#endif

#if NMAGNUMSCSI > 0
int magnumscsiinit();

struct driver magnumscsidriver = {
	magnumscsiinit, "Magnumscsi", (int (*)())siopstart, (int (*)(int,...))siopgo,
	(int (*)())siopintr2, (int (*)())siopdone,
	siopustart, siopreq, siopfree, siopreset,
	siop_delay, siop_test_unit_rdy, siop_start_stop_unit,
	siop_request_sense, siop_immed_command, siop_tt_read, siop_tt_write,
#if NST > 0
	siop_tt_oddio
#else
	0
#endif
};
#endif

/* 53C710 script */
unsigned long scripts[] = {
	0x47000000, 0x00000298,	/* 000 -   0 */
	0x838b0000, 0x000000d0,	/* 008 -   8 */
	0x7a1b1000, 0x00000000,	/* 010 -  16 */
	0x828a0000, 0x00000088,	/* 018 -  24 */
	0x9e020000, 0x0000ff01,	/* 020 -  32 */
	0x72350000, 0x00000000,	/* 028 -  40 */
	0x808c0000, 0x00000048,	/* 030 -  48 */
	0x58000008, 0x00000000,	/* 038 -  56 */
	0x1e000024, 0x00000024,	/* 040 -  64 */
	0x838b0000, 0x00000090,	/* 048 -  72 */
	0x1f00002c, 0x0000002c,	/* 050 -  80 */
	0x838b0000, 0x00000080,	/* 058 -  88 */
	0x868a0000, 0xffffffd0,	/* 060 -  96 */
	0x838a0000, 0x00000070,	/* 068 - 104 */
	0x878a0000, 0x00000120,	/* 070 - 112 */
	0x80880000, 0x00000028,	/* 078 - 120 */
	0x1e000004, 0x00000004,	/* 080 - 128 */
	0x838b0000, 0x00000050,	/* 088 - 136 */
	0x868a0000, 0xffffffe8,	/* 090 - 144 */
	0x838a0000, 0x00000040,	/* 098 - 152 */
	0x878a0000, 0x000000f0,	/* 0a0 - 160 */
	0x9a020000, 0x0000ff02,	/* 0a8 - 168 */
	0x1a00000c, 0x0000000c,	/* 0b0 - 176 */
	0x878b0000, 0x00000130,	/* 0b8 - 184 */
	0x838a0000, 0x00000018,	/* 0c0 - 192 */
	0x818a0000, 0x000000b0,	/* 0c8 - 200 */
	0x808a0000, 0x00000080,	/* 0d0 - 208 */
	0x98080000, 0x0000ff03,	/* 0d8 - 216 */
	0x1b000014, 0x00000014,	/* 0e0 - 224 */
	0x72090000, 0x00000000,	/* 0e8 - 232 */
	0x6a340000, 0x00000000,	/* 0f0 - 240 */
	0x9f030000, 0x0000ff04,	/* 0f8 - 248 */
	0x1f00001c, 0x0000001c,	/* 100 - 256 */
	0x98040000, 0x0000ff26,	/* 108 - 264 */
	0x60000040, 0x00000000,	/* 110 - 272 */
	0x48000000, 0x00000000,	/* 118 - 280 */
	0x7c1bef00, 0x00000000,	/* 120 - 288 */
	0x72340000, 0x00000000,	/* 128 - 296 */
	0x980c0002, 0x0000fffc,	/* 130 - 304 */
	0x980c0008, 0x0000fffb,	/* 138 - 312 */
	0x980c0018, 0x0000fffd,	/* 140 - 320 */
	0x98040000, 0x0000fffe,	/* 148 - 328 */
	0x98080000, 0x0000ff00,	/* 150 - 336 */
	0x18000034, 0x00000034,	/* 158 - 344 */
	0x808b0000, 0x000001c0,	/* 160 - 352 */
	0x838b0000, 0xffffff70,	/* 168 - 360 */
	0x878a0000, 0x000000d0,	/* 170 - 368 */
	0x98080000, 0x0000ff05,	/* 178 - 376 */
	0x19000034, 0x00000034,	/* 180 - 384 */
	0x818b0000, 0x00000160,	/* 188 - 392 */
	0x80880000, 0xffffffd0,	/* 190 - 400 */
	0x1f00001c, 0x0000001c,	/* 198 - 408 */
	0x808c0001, 0x00000018,	/* 1a0 - 416 */
	0x980c0002, 0x0000ff08,	/* 1a8 - 424 */
	0x808c0004, 0x00000020,	/* 1b0 - 432 */
	0x98080000, 0x0000ff06,	/* 1b8 - 440 */
	0x60000040, 0x00000000,	/* 1c0 - 448 */
	0x1f00002c, 0x0000002c,	/* 1c8 - 456 */
	0x98080000, 0x0000ff07,	/* 1d0 - 464 */
	0x60000040, 0x00000000,	/* 1d8 - 472 */
	0x48000000, 0x00000000,	/* 1e0 - 480 */
	0x98080000, 0x0000ff09,	/* 1e8 - 488 */
	0x1f00001c, 0x0000001c,	/* 1f0 - 496 */
	0x808c0001, 0x00000018,	/* 1f8 - 504 */
	0x980c0002, 0x0000ff10,	/* 200 - 512 */
	0x808c0004, 0x00000020,	/* 208 - 520 */
	0x98080000, 0x0000ff11,	/* 210 - 528 */
	0x60000040, 0x00000000,	/* 218 - 536 */
	0x1f00002c, 0x0000002c,	/* 220 - 544 */
	0x98080000, 0x0000ff12,	/* 228 - 552 */
	0x60000040, 0x00000000,	/* 230 - 560 */
	0x48000000, 0x00000000,	/* 238 - 568 */
	0x98080000, 0x0000ff13,	/* 240 - 576 */
	0x1f00001c, 0x0000001c,	/* 248 - 584 */
	0x808c0001, 0x00000018,	/* 250 - 592 */
	0x980c0002, 0x0000ff14,	/* 258 - 600 */
	0x808c0004, 0x00000020,	/* 260 - 608 */
	0x98080000, 0x0000ff15,	/* 268 - 616 */
	0x60000040, 0x00000000,	/* 270 - 624 */
	0x1f00002c, 0x0000002c,	/* 278 - 632 */
	0x98080000, 0x0000ff16,	/* 280 - 640 */
	0x60000040, 0x00000000,	/* 288 - 648 */
	0x48000000, 0x00000000,	/* 290 - 656 */
	0x98080000, 0x0000ff17,	/* 298 - 664 */
	0x54000000, 0x00000040,	/* 2a0 - 672 */
	0x9f030000, 0x0000ff18,	/* 2a8 - 680 */
	0x1f00001c, 0x0000001c,	/* 2b0 - 688 */
	0x990b0000, 0x0000ff19,	/* 2b8 - 696 */
	0x980a0000, 0x0000ff20,	/* 2c0 - 704 */
	0x9f0a0000, 0x0000ff21,	/* 2c8 - 712 */
	0x9b0a0000, 0x0000ff22,	/* 2d0 - 720 */
	0x9e0a0000, 0x0000ff23,	/* 2d8 - 728 */
	0x98080000, 0x0000ff24,	/* 2e0 - 736 */
	0x98080000, 0x0000ff25,	/* 2e8 - 744 */
	0x76100800, 0x00000000,	/* 2f0 - 752 */
	0x80840700, 0x00000008,	/* 2f8 - 760 */
	0x7e110100, 0x00000000,	/* 300 - 768 */
	0x6a100000, 0x00000000,	/* 308 - 776 */
	0x19000034, 0x00000034,	/* 310 - 784 */
	0x818b0000, 0xffffffd0,	/* 318 - 792 */
	0x98080000, 0x0000ff27,	/* 320 - 800 */
	0x76100800, 0x00000000,	/* 328 - 808 */
	0x80840700, 0x00000008,	/* 330 - 816 */
	0x7e110100, 0x00000000,	/* 338 - 824 */
	0x6a100000, 0x00000000,	/* 340 - 832 */
	0x18000034, 0x00000034,	/* 348 - 840 */
	0x808b0000, 0xffffffd0,	/* 350 - 848 */
	0x98080000, 0x0000ff27	/* 358 - 856 */
};

#define	Ent_msgout	0x00000018
#define	Ent_cmd		0x000000a8
#define	Ent_status	0x000000e0
#define	Ent_msgin	0x000000f8
#define	Ent_dataout	0x00000158
#define	Ent_datain	0x00000180

struct	siop_softc siop_softc[NSIOP];

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

/* default to not inhibit sync negotiation on any drive */
u_char siop_inhibit_sync[NSIOP][8] = { 0, 0, 1, 0, 0, 0, 0 }; /* initialize, so patchable */
int siop_no_dma = 0;

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

static int initialized[NSIOP];

#ifdef DEBUG
/*	0x01 - full debug	*/
/*	0x02 - DMA chaining	*/
/*	0x04 - siopintr		*/
/*	0x08 - phase mismatch	*/
int	siop_debug = 0;
int	siopsync_debug = 0;
int	siopdma_hits = 0;
int	siopdma_misses = 0;
#endif

static void
siopabort(dev, regs, where)
	register struct siop_softc *dev;
	volatile register siop_regmap_t *regs;
	char *where;
{

	printf ("siop%d: abort %s: dstat %02x, sstat0 %02x sbcl %02x\n", 
	    dev->sc_ac->amiga_unit,
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
	  DELAY(25);
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

#if NZEUSSCSI > 0
int
zeusscsiinit(ac)
	register struct amiga_ctlr *ac;
{
	register struct siop_softc *dev = &siop_softc[ac->amiga_unit];

#ifdef DEBUG
	if (siop_debug)
		printf ("zeusscsiinit: addr %x unit %d scripts %x ds %x\n",
		    ac->amiga_addr, ac->amiga_unit, scripts, &dev->sc_ds);
#endif
	if ((int) scripts & 3)
  		panic ("zeusscsiinit: scripts not on a longword boundary");
	if ((long)&dev->sc_ds & 3)
		panic("zeusscsiinit: data storage alignment error\n");
	if (! ac->amiga_addr)
		return 0;

	if (initialized[ac->amiga_unit])
		return 0;
 
	initialized[ac->amiga_unit] = 1;
  
	/* advance ac->amiga_addr to point to the real siop-registers */
	ac->amiga_addr = (caddr_t) ((int)ac->amiga_addr + 0x4000);
	dev->sc_clock_freq = 0xc0;

	/* hardwired IPL */
	ac->amiga_ipl = 6;
	dev->sc_ac = ac;
	dev->sc_dq.dq_driver = &zeusscsidriver;
	dev->sc_sq.dq_forw = dev->sc_sq.dq_back = &dev->sc_sq;
	siopreset (ac->amiga_unit);

	/* For the ZEUS SCSI, both IPL6 and IPL2 interrupts have to be
	   enabled.  The ZEUS SCSI generates IPL6 interrupts, which are
	   not blocked by splbio().  The ZEUS interrupt handler saves the
	   siop interrupt status information in siop_softc and sets the
	   PORTS interrupt to process the interrupt at IPL2.

	   make sure IPL2 interrupts are delivered to the cpu when siopintr6
	   generates some. Note that this does not yet enable siop-interrupts,
	   this is handled in siopgo, which selectively enables interrupts only
	   while DMA requests are pending.

	   Note that enabling PORTS interrupts also enables keyboard interrupts
	   as soon as the corresponding int-enable bit in CIA-A is set. */
     
#if 0	/* EXTER interrupts are enabled in the clock initialization */
	custom.intreq = INTF_EXTER;
	custom.intena = INTF_SETCLR | INTF_EXTER;
#endif
	custom.intreq = INTF_PORTS;
	custom.intena = INTF_SETCLR | INTF_PORTS;
	return(1);
}
#endif

#if NMAGNUMSCSI > 0
int
magnumscsiinit(ac)
	register struct amiga_ctlr *ac;
{
	register struct siop_softc *dev = &siop_softc[ac->amiga_unit];

#ifdef DEBUG
	if (siop_debug)
		printf ("magnumscsiinit: addr %x unit %d scripts %x ds %x\n",
		    ac->amiga_addr, ac->amiga_unit, scripts, &dev->sc_ds);
#endif
	if ((int) scripts & 3)
  		panic ("magnumscsiinit: scripts not on a longword boundary");
	if ((long)&dev->sc_ds & 3)
		panic("magnumscsiinit: data storage alignment error\n");
	if (! ac->amiga_addr)
		return 0;

	if (initialized[ac->amiga_unit])
		return 0;
 
	initialized[ac->amiga_unit] = 1;
  
	/* advance ac->amiga_addr to point to the real siop-registers */
	ac->amiga_addr = (caddr_t) ((int)ac->amiga_addr + 0x8000);
	dev->sc_clock_freq = 0x40;	/* DCNTL = 25.01->37.5MHZ / SCLK/1.5 */

	/* hardwired IPL */
	ac->amiga_ipl = 2;
	dev->sc_ac = ac;
	dev->sc_dq.dq_driver = &magnumscsidriver;
	dev->sc_sq.dq_forw = dev->sc_sq.dq_back = &dev->sc_sq;
	siopreset (ac->amiga_unit);

	/* make sure IPL2 interrupts are delivered to the cpu when the siop
	   generates some. Note that this does not yet enable siop-interrupts,
	   this is handled in siopgo, which selectively enables interrupts only
	   while DMA requests are pending.

	   Note that enabling PORTS interrupts also enables keyboard interrupts
	   as soon as the corresponding int-enable bit in CIA-A is set. */
     
	custom.intreq = INTF_PORTS;
	custom.intena = INTF_SETCLR | INTF_PORTS;
	return(1);
}
#endif


void
siopreset(unit)
	register int unit;
{
	register struct siop_softc *dev = &siop_softc[unit];
	volatile register siop_regmap_t *regs =
				(siop_regmap_t *)dev->sc_ac->amiga_addr;
	u_int i, s;
	u_char  my_id, csr;

	if (dev->sc_flags & SIOP_ALIVE)
		siopabort(dev, regs, "reset");
		
	printf("siop%d: ", unit);

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
	regs->siop_dcntl = dev->sc_clock_freq;
	regs->siop_dmode = 0x80;
	regs->siop_sien = 0x00;	/* don't enable interrupts yet */
	regs->siop_dien = 0x00;	/* don't enable interrupts yet */
	regs->siop_scid = 1 << my_id;
	regs->siop_dwt = 0x00;
	regs->siop_ctest0 |= 0x20;	/* Enable Active Negation ?? */
	regs->siop_ctest7 |= 0x02;	/* TT1 */

	/* will need to re-negotiate sync xfers */
	bzero(&dev->sc_sync, sizeof (dev->sc_sync));

	splx (s);

	printf("siop id %d\n", my_id);
	dev->sc_flags |= SIOP_ALIVE;
	dev->sc_flags &= ~(SIOP_SELECTED | SIOP_DMA);
}

/*
 * Setup Data Storage for 53C710 and start SCRIPTS processing
 */

static void
siop_setup (dev, target, cbuf, clen, buf, len)
	struct siop_softc *dev;
	int target;
	u_char *cbuf;
	int clen;
	u_char *buf;
	int len;
{
	volatile register siop_regmap_t *regs =
		(siop_regmap_t *)dev->sc_ac->amiga_addr;
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
	dev->sc_ds.idbuf = (char *) kvtop(&dev->sc_lun);
	dev->sc_ds.cmdlen = clen;
	dev->sc_ds.cmdbuf = (char *) kvtop(cbuf);
	dev->sc_ds.stslen = 1;
	dev->sc_ds.stsbuf = (char *) kvtop(&dev->sc_stat[0]);
	dev->sc_ds.msglen = 1;
	dev->sc_ds.msgbuf = (char *) kvtop(&dev->sc_msg[0]);
	dev->sc_ds.sdtrolen = 0;
	dev->sc_ds.sdtrilen = 0;
	dev->sc_ds.chain[0].datalen = len;
	dev->sc_ds.chain[0].databuf = (char *) kvtop(buf);

	if (dev->sc_sync[target].state == SYNC_START) {
		if (siop_inhibit_sync[dev->sc_ac->amiga_unit][target]) {
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
			dev->sc_msg[5] = 100/4;		/* min period = 100 nsec */
			dev->sc_msg[6] = SIOP_MAX_OFFSET;
			dev->sc_ds.sdtrolen = 6;
			dev->sc_ds.sdtrilen = sizeof (dev->sc_msg) - 1;
			dev->sc_ds.sdtrobuf = dev->sc_ds.sdtribuf = (char *) kvtop(dev->sc_msg + 1);
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
			++siopdma_misses;
#endif
		}
		++nchain;
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
	regs->siop_dsa = (long) kvtop(&dev->sc_ds);
	DCIS();				/* push data cache */
	regs->siop_dsp = (long) kvtop(scripts);
}

/*
 * Process a DMA or SCSI interrupt from the 53C710 SIOP
 */

static int
siop_checkintr(dev, istat, dstat, sstat0, status)
	struct	siop_softc *dev;
	u_char	istat;
	u_char	dstat;
	u_char	sstat0;
	int	*status;
{
	volatile register siop_regmap_t *regs =
		(siop_regmap_t *)dev->sc_ac->amiga_addr;
	int	target;

	regs->siop_ctest8 |= 0x04;
	while ((regs->siop_ctest1 & SIOP_CTEST1_FMT) == 0)
		;
	regs->siop_ctest8 &= ~0x04;
#ifdef DEBUG
	if (siop_debug & 1) {
		DCIAS(kvtop(&dev->sc_stat));	/* XXX */
		printf ("siopchkintr: istat %x dstat %x sstat0 %x dsps %x sbcl %x sts %x msg %x\n",
		    istat, dstat, sstat0, regs->siop_dsps, regs->siop_sbcl, dev->sc_stat[0], dev->sc_msg[0]);
	}
#endif
	if (dstat & SIOP_DSTAT_SIR && (regs->siop_dsps == 0xff00 ||
	    regs->siop_dsps == 0xfffc)) {
		/* Normal completion status, or check condition */
		if (regs->siop_dsa != (long) kvtop(&dev->sc_ds)) {
			printf ("siop: invalid dsa: %x %x\n", regs->siop_dsa,
			    kvtop(&dev->sc_ds));
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
			dev->sc_sync[target].state = SYNC_DONE;
			dev->sc_sync[target].period = 0;
			dev->sc_sync[target].offset = 0;
			if (dev->sc_msg[1] == MSG_EXT_MESSAGE &&
			    dev->sc_msg[2] == 3 &&
			    dev->sc_msg[3] == MSG_SYNC_REQ) {
				printf ("siop%d: target %d now synchronous, period=%dns, offset=%d\n",
				    dev->sc_ac->amiga_unit, target,
				    dev->sc_msg[4] * 4, dev->sc_msg[5]);
				scsi_period_to_siop (dev, target);
			}
		}
		DCIAS(kvtop(&dev->sc_stat));	/* XXX */
		*status = dev->sc_stat[0];
		return 1;
	}
	if (sstat0 & SIOP_SSTAT0_M_A) {		/* Phase mismatch */
#ifdef DEBUG
		if (siop_debug & 9)
			printf ("Phase mismatch: %x dsp +%x\n", regs->siop_sbcl,
			    regs->siop_dsp - kvtop(scripts));
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
			regs->siop_dsp = kvtop(scripts) + Ent_dataout;
			break;
		case 1:
			regs->siop_dsp = kvtop(scripts) + Ent_datain;
			break;
		case 2:
			regs->siop_dsp = kvtop(scripts) + Ent_cmd;
			break;
		case 3:
			regs->siop_dsp = kvtop(scripts) + Ent_status;
			break;
		case 6:
			regs->siop_dsp = kvtop(scripts) + Ent_msgout;
			break;
		case 7:
			regs->siop_dsp = kvtop(scripts) + Ent_msgin;
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
		siopreset (dev->sc_ac->amiga_unit);
		*status = -1;
		return 1;
	}
	if (dstat & SIOP_DSTAT_SIR && regs->siop_dsps == 0xff27) {
#ifdef DEBUG
		if (siop_debug & 3)
			printf ("DMA chaining completed: dsa %x dnad %x addr %x\n",
				regs->siop_dsa,	regs->siop_dnad, regs->siop_addr);
#endif
		regs->siop_dsa = kvtop (&dev->sc_ds);
		regs->siop_dsp = kvtop (scripts) + Ent_status;
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
		siop_inhibit_sync[dev->sc_ac->amiga_unit][target] = -1;
		if ((regs->siop_sbcl & 7) == 6) {
			regs->siop_dsp = kvtop(scripts) + Ent_msgout;
			return (0);
		}
	}
	if (sstat0 == 0 && dstat & SIOP_DSTAT_SIR) {
		DCIAS(kvtop(&dev->sc_stat));
		printf ("SIOP interrupt: %x sts %x msg %x sbcl %x\n",
		    regs->siop_dsps, dev->sc_stat[0], dev->sc_msg[0],
		    regs->siop_sbcl);
		siopreset (dev->sc_ac->amiga_unit);
		*status = -1;
		return 1;
	}
bad_phase:
/*
 * temporary panic for unhandled conditions
 * displays various things about the 53C710 status and registers
 * then panics
 */
printf ("siopchkintr: target %x ds %x\n", target, &dev->sc_ds);
printf ("scripts %x ds %x regs %x dsp %x dcmd %x\n", kvtop(scripts),
    kvtop(&dev->sc_ds), kvtop(regs), regs->siop_dsp, *((long *)&regs->siop_dcmd));
printf ("siopchkintr: istat %x dstat %x sstat0 %x dsps %x dsa %x sbcl %x sts %x msg %x\n",
    istat, dstat, sstat0, regs->siop_dsps, regs->siop_dsa, regs->siop_sbcl,
    dev->sc_stat[0], dev->sc_msg[0]);
panic("siopchkintr: **** temp ****");
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
static int
siopicmd(dev, target, cbuf, clen, buf, len)
	struct siop_softc *dev;
	int target;
	u_char *cbuf;
	int clen;
	u_char *buf;
	int len;
{
	volatile register siop_regmap_t *regs =
		(siop_regmap_t *)dev->sc_ac->amiga_addr;
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
		    cbuf[0], &dev->sc_ds);
#endif
	siop_setup (dev, target, cbuf, clen, buf, len);

	for (;;) {
		/* use cmd_wait values? */
		i = siop_cmd_wait << 1;
		while (((istat = regs->siop_istat) &
		    (SIOP_ISTAT_SIP | SIOP_ISTAT_DIP)) == 0) {
			if (--i <= 0) {
				printf ("waiting: tgt %d cmd %02x sbcl %02x dsp %x (+%x) dcmd %x ds %x\n",
				    target, cbuf[0],
				    regs->siop_sbcl, regs->siop_dsp,
				    regs->siop_dsp - kvtop(scripts),
				    *((long *)&regs->siop_dcmd), &dev->sc_ds);
				i = siop_cmd_wait << 2;
			}
			DELAY(1);
		}
		dstat = regs->siop_dstat;
		sstat0 = regs->siop_sstat0;
#ifdef DEBUG
		if (siop_debug & 1) {
			DCIAS(kvtop(&dev->sc_stat));	/* XXX should just invalidate dev->sc_stat */
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
siop_test_unit_rdy(ctlr, slave, unit)
	int ctlr, slave, unit;
{
	register struct siop_softc *dev = &siop_softc[ctlr];
	static struct scsi_cdb6 cdb = { CMD_TEST_UNIT_READY };

	cdb.lun = unit;
	return (siopicmd(dev, slave, (u_char *)&cdb, sizeof(cdb), (u_char *)0, 0));
}

int
siop_start_stop_unit (ctlr, slave, unit, start)
	int ctlr, slave, unit;
{
	register struct siop_softc *dev = &siop_softc[ctlr];
	static struct scsi_cdb6 cdb = { CMD_LOADUNLOAD };

	cdb.lun = unit;
	/* we don't set the immediate bit, so we wait for the 
	   command to succeed.
	   We also don't touch the LoEj bit, which is primarily meant
	   for floppies. */
	cdb.len = start & 0x01;
	return (siopicmd(dev, slave, (u_char *)&cdb, sizeof(cdb), (u_char *)0, 0));
}


int
siop_request_sense(ctlr, slave, unit, buf, len)
	int ctlr, slave, unit;
	u_char *buf;
	unsigned len;
{
	register struct siop_softc *dev = &siop_softc[ctlr];
	static struct scsi_cdb6 cdb = { CMD_REQUEST_SENSE };

	cdb.lun = unit;
	cdb.len = len;
	return (siopicmd(dev, slave, (u_char *)&cdb, sizeof(cdb), buf, len));
}

int
siop_immed_command_nd(ctlr, slave, unit, cdb)
	int ctlr, slave, unit;
	struct scsi_fmt_cdb *cdb;
{
	return(siop_immed_command(ctlr, slave, unit, cdb, 0, 0, 0));
}


int
siop_immed_command(ctlr, slave, unit, cdb, buf, len, rd)
	int ctlr, slave, unit;
	struct scsi_fmt_cdb *cdb;
	u_char *buf;
	unsigned len;
{
	register struct siop_softc *dev = &siop_softc[ctlr];

	cdb->cdb[1] |= (unit << 5);
	return (siopicmd(dev, slave, (u_char *) cdb->cdb, cdb->len, buf, len));
}

/*
 * The following routines are test-and-transfer i/o versions of read/write
 * for things like reading disk labels and writing core dumps.  The
 * routine scsigo should be used for normal data transfers, NOT these
 * routines.
 */
int
siop_tt_read(ctlr, slave, unit, buf, len, blk, bshift)
	int ctlr, slave, unit;
	u_char *buf;
	u_int len;
	daddr_t blk;
	int bshift;
{
	register struct siop_softc *dev = &siop_softc[ctlr];
	struct scsi_cdb10 cdb;
	int stat;
	int old_wait = siop_data_wait;

#ifdef DEBUG
	if (siop_debug & 1)
		printf ("siop%d: tt_read blk %x\n", slave, blk);
#endif
	siop_data_wait = 300000;
	bzero(&cdb, sizeof(cdb));
	cdb.cmd = CMD_READ_EXT;
	cdb.lun = unit;
	blk >>= bshift;
	cdb.lbah = blk >> 24;
	cdb.lbahm = blk >> 16;
	cdb.lbalm = blk >> 8;
	cdb.lbal = blk;
	cdb.lenh = len >> (8 + DEV_BSHIFT + bshift);
	cdb.lenl = len >> (DEV_BSHIFT + bshift);
	stat = siopicmd(dev, slave, (u_char *) &cdb, sizeof(cdb), buf, len);
	siop_data_wait = old_wait;
	return (stat);
}

int
siop_tt_write(ctlr, slave, unit, buf, len, blk, bshift)
	int ctlr, slave, unit;
	u_char *buf;
	u_int len;
	daddr_t blk;
	int bshift;
{
	register struct siop_softc *dev = &siop_softc[ctlr];
	struct scsi_cdb10 cdb;
	int stat;
	int old_wait = siop_data_wait;

#ifdef DEBUG
	if (siop_debug)
		printf ("siop%d: tt_write blk %d from %08x\n", slave,
		   blk, kvtop(buf));
	if (blk < 604)
		panic("siop_tt_write: writing block < 604");
#endif
	siop_data_wait = 300000;

	bzero(&cdb, sizeof(cdb));
	cdb.cmd = CMD_WRITE_EXT;
	cdb.lun = unit;
	blk >>= bshift;
	cdb.lbah = blk >> 24;
	cdb.lbahm = blk >> 16;
	cdb.lbalm = blk >> 8;
	cdb.lbal = blk;
	cdb.lenh = len >> (8 + DEV_BSHIFT + bshift);
	cdb.lenl = len >> (DEV_BSHIFT + bshift);
	stat = siopicmd(dev, slave, (u_char *) &cdb, sizeof(cdb), buf, len);
	siop_data_wait = old_wait;
	return (stat);
}

int
siopreq(dq)
	register struct devqueue *dq;
{
	register struct devqueue *hq;

	hq = &siop_softc[dq->dq_ctlr].sc_sq;
	insque(dq, hq->dq_back);
#ifdef DEBUG
	if (siop_debug & 1)
		printf ("siopreq %d ", dq->dq_back == hq ? 1 : 0);
#endif
	if (dq->dq_back == hq)
		return(1);
	return(0);
}

int
siopustart (int unit)
{
	register struct siop_softc *dev = &siop_softc[unit];

#ifdef DEBUG
	if (siop_debug & 1)
		printf ("siop%d: ustart ", unit);
#endif
	return(1);
}

void
siopstart (int unit)
{
	register struct devqueue *dq;
	
#ifdef DEBUG
	if (siop_debug & 1)
		printf ("siop%d: start ", unit);
#endif
	dq = siop_softc[unit].sc_sq.dq_forw;
	(dq->dq_driver->d_go)(dq->dq_unit);
}

int
siopgo(ctlr, slave, unit, bp, cdb, pad)
	int ctlr, slave, unit;
	struct buf *bp;
	struct scsi_fmt_cdb *cdb;
	int pad;
{
	register struct siop_softc *dev = &siop_softc[ctlr];
	volatile register siop_regmap_t *regs =
			(siop_regmap_t *)dev->sc_ac->amiga_addr;
	int i;
	int nchain;
	int count, tcount;
	char *addr, *dmaend;

#ifdef DEBUG
	if (siop_debug & 1)
		printf ("siop%d: go ", slave);
	if ((cdb->cdb[1] & 1) == 0 &&
	    ((cdb->cdb[0] == CMD_WRITE && cdb->cdb[2] == 0 && cdb->cdb[3] == 0) ||
	    (cdb->cdb[0] == CMD_WRITE_EXT && cdb->cdb[2] == 0 && cdb->cdb[3] == 0
	    && cdb->cdb[4] == 0)))
		panic ("siopgo: attempted write to block < 0x100");
#endif
	cdb->cdb[1] |= unit << 5;

	if (dev->sc_flags & SIOP_SELECTED) {
		printf ("siopgo%d: bus busy\n", slave);
		return 1;
	}

	if (siop_no_dma) {
		register struct devqueue *dq;

		/* in this case do the transfer with programmed I/O :-( This is
		probably still faster than doing the transfer with DMA into a
		buffer and copying it later to its final destination, comments? */
		/* XXX - 53C710 uses DMA, but non-interrupt */
		siopicmd (dev, slave, (u_char *) cdb->cdb, cdb->len, 
			bp->b_un.b_addr, bp->b_bcount);

		dq = dev->sc_sq.dq_forw;
		(dq->dq_driver->d_intr)(dq->dq_unit, dev->sc_stat[0]);
		return dev->sc_stat[0];
	}

	dev->sc_flags |= SIOP_SELECTED | SIOP_DMA;
	dev->sc_slave = slave;
	/* enable SCSI and DMA interrupts */
	regs->siop_sien = SIOP_SIEN_M_A | SIOP_SIEN_STO | SIOP_SIEN_SEL | SIOP_SIEN_SGE |
	    SIOP_SIEN_UDC | SIOP_SIEN_RST | SIOP_SIEN_PAR;
	regs->siop_dien = 0x20 | SIOP_DIEN_ABRT | SIOP_DIEN_SIR | SIOP_DIEN_WTD |
	    SIOP_DIEN_OPC;
#ifdef DEBUG
	if (siop_debug & 1)
		printf ("siopgo: target %x cmd %02x ds %x\n", slave, cdb->cdb[0], &dev->sc_ds);
#endif

	siop_setup(dev, slave, cdb->cdb, cdb->len, bp->b_un.b_addr, bp->b_bcount);

	return (0);
}

void
siopdone (int unit)
{
	volatile register siop_regmap_t *regs =
			(siop_regmap_t *)siop_softc[unit].sc_ac->amiga_addr;

#ifdef DEBUG
	if (siop_debug & 1)
		printf("siop%d: done called!\n", unit);
#endif
}

/*
 * Level 6 interrupt processing for the Progressive Peripherals Inc
 * Zeus SCSI.  Because the level 6 interrupt is above splbio, the
 * interrupt status is saved and the INTF_PORTS interrupt is set.
 * This way, the actual processing of the interrupt can be deferred
 * until splbio is unblocked
 */

#if NZEUSSCSI > 0
int
siopintr6 ()
{
	register struct siop_softc *dev;
	volatile register siop_regmap_t *regs;
	register u_char istat;
	int unit;
	int found = 0;

	for (unit = 0, dev = siop_softc; unit < NSIOP; unit++, dev++) {
		if (!initialized[dev->sc_ac->amiga_unit])
			continue;
		if (dev->sc_ac->amiga_ipl != 6)
			continue;
		regs = (siop_regmap_t *)dev->sc_ac->amiga_addr;
		istat = regs->siop_istat;
		if ((istat & (SIOP_ISTAT_SIP | SIOP_ISTAT_DIP)) == 0)
			continue;
		if ((dev->sc_flags & (SIOP_DMA | SIOP_SELECTED)) == SIOP_SELECTED)
			continue;	/* doing non-interrupt I/O */
		found++;
		dev->sc_istat = istat;
		dev->sc_dstat = regs->siop_dstat;
		dev->sc_sstat0 = regs->siop_sstat0;
		custom.intreq = INTF_EXTER;
		custom.intreq = INTF_SETCLR | INTF_PORTS;
	}
	return (found);
}
#endif

/*
 * Check for 53C710 interrupts
 */

int
siopintr2 ()
{
	register struct siop_softc *dev;
	volatile register siop_regmap_t *regs;
	register u_char istat, dstat, sstat0;
	register struct devqueue *dq;
	int unit;
	int status;
	int found = 0;

	for (unit = 0, dev = siop_softc; unit < NSIOP; unit++, dev++) {
		if (!initialized[dev->sc_ac->amiga_unit])
			continue;
		regs = (siop_regmap_t *)dev->sc_ac->amiga_addr;
		if (dev->sc_ac->amiga_ipl == 6)
			istat = dev->sc_istat;
		else
			istat = regs->siop_istat;
		if ((istat & (SIOP_ISTAT_SIP | SIOP_ISTAT_DIP)) == 0)
			continue;
		if ((dev->sc_flags & (SIOP_DMA | SIOP_SELECTED)) == SIOP_SELECTED)
			continue;	/* doing non-interrupt I/O */
		/* Got a valid interrupt on this device */
		found++;
		if (dev->sc_ac->amiga_ipl == 6) {
			dstat = dev->sc_dstat;
			sstat0 = dev->sc_sstat0;
			dev->sc_istat = 0;
		}
		else {
			dstat = regs->siop_dstat;
			sstat0 = regs->siop_sstat0;
		}
#ifdef DEBUG
		if (siop_debug & 1)
			printf ("siop%d: intr istat %x dstat %x sstat0 %x\n",
			    unit, istat, dstat, sstat0);
		if ((dev->sc_flags & SIOP_DMA) == 0) {
			printf ("siop%d: spurious interrupt? istat %x dstat %x sstat0 %x\n",
			    unit, istat, dstat, sstat0);
		}
#endif

#ifdef DEBUG
		if (siop_debug & 5) {
			DCIAS(kvtop(&dev->sc_stat));
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
			dq = dev->sc_sq.dq_forw;
			(dq->dq_driver->d_intr)(dq->dq_unit, status);
		}
	}
	return (found);
}

void
siopfree(dq)
	register struct devqueue *dq;
{
	register struct devqueue *hq;

#ifdef DEBUG
	if (siop_debug & 1)
		printf ("siopfree\n");
#endif
	hq = &siop_softc[dq->dq_ctlr].sc_sq;
	remque(dq);
	if ((dq = hq->dq_forw) != hq)
		(dq->dq_driver->d_start)(dq->dq_unit);
}

/*
 * (XXX) The following routine is needed for the SCSI tape driver
 * to read odd-size records.
 */

/* XXX - probably not needed for the 53C710 (and not implemented yet!) */

#if NST > 0
int
siop_tt_oddio(ctlr, slave, unit, buf, len, b_flags, freedma)
	int ctlr, slave, unit, b_flags;
	u_char *buf;
	u_int len;
{
	register struct siop_softc *dev = &siop_softc[ctlr];
	struct scsi_cdb6 cdb;
	u_char iphase;
	int stat;

printf ("siop%d: tt_oddio\n", slave);
#if 0
	/*
	 * First free any DMA channel that was allocated.
	 * We can't use DMA to do this transfer.
	 */
	if (freedma)
		dev->dmafree(&dev->sc_dq);
	/*
	 * Initialize command block
	 */
	bzero(&cdb, sizeof(cdb));
	cdb.lun  = unit;
	cdb.lbam = (len >> 16) & 0xff;
	cdb.lbal = (len >> 8) & 0xff;
	cdb.len = len & 0xff;
	if (buf == 0) {
		cdb.cmd = CMD_SPACE;
		cdb.lun |= 0x00;
		len = 0;
		iphase = MESG_IN_PHASE;
	} else if (b_flags & B_READ) {
		cdb.cmd = CMD_READ;
		iphase = DATA_IN_PHASE;
	} else {
		cdb.cmd = CMD_WRITE;
		iphase = DATA_OUT_PHASE;
	}
	/*
	 * Perform command (with very long delays)
	 */
	scsi_delay(30000000);
	stat = siopicmd(dev, slave, (u_char *) &cdb, sizeof(cdb), buf, len, iphase);
	scsi_delay(0);
	return (stat);
#endif
	return -1;
}
#endif
#endif
