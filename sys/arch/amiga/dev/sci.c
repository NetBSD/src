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
 *	@(#)sci.c	7.5 (Berkeley) 5/4/91
 *	$Id: sci.c,v 1.2 1994/03/08 10:30:16 chopps Exp $
 *
 */

/*
 * AMIGA NCR 5380 scsi adaptor driver
 */

#include "mlhscsi.h"
#include "csa12gscsi.h"
#include "suprascsi.h"
#include "ivsscsi.h"
#if (NMLHSCSI + NCSA12GSCSI + NSUPRASCSI + NIVSSCSI) > 0
#define NSCI	NMLHSCSI
#if NSCI < NCSA12GSCSI
#undef NSCI
#define NSCI	NCSA12GSCSI
#endif
#if NSCI < NSUPRASCSI
#undef NSCI
#define NSCI	NSUPRASCSI
#endif
#if NSCI < NIVSSCSI
#undef NSCI
#define NSCI	NIVSSCI
#endif

#ifndef lint
static char rcsid[] = "$Header: /cvsroot/src/sys/arch/amiga/dev/sci.c,v 1.2 1994/03/08 10:30:16 chopps Exp $";
#endif

/* need to know if any tapes have been configured */
#include "st.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>
#include <vm/vm_statistics.h>
#include <machine/pmap.h>

#include <amiga/dev/device.h>

#include <amiga/dev/scsidefs.h>
#include <amiga/dev/scivar.h>
#include <amiga/dev/scireg.h>

#include <amiga/amiga/custom.h>

#include <machine/cpu.h>

extern u_int kvtop();

static int sci_wait __P((char until, int timeo, int line));
static void scsiabort __P((register struct sci_softc *dev, char *where));
static void scsierror __P((register struct sci_softc *dev, u_char csr));
static int issue_select __P((register struct sci_softc *dev, u_char target,
    u_char our_addr));
static int ixfer_out __P((register struct sci_softc *dev, int len,
    register u_char *buf, int phase));
static void ixfer_in __P((register struct sci_softc *dev, int len,
    register u_char *buf, int phase));
static int scsiicmd __P((struct sci_softc *dev, int target, u_char *cbuf,
    int clen, u_char *buf, int len, u_char xferphase));


/*
 * SCSI delays
 * In u-seconds, primarily for state changes on the SPC.
 */
#define	SCSI_CMD_WAIT	50000	/* wait per step of 'immediate' cmds */
#define	SCSI_DATA_WAIT	50000	/* wait per data in/out step */
#define	SCSI_INIT_WAIT	50000	/* wait per step (both) during init */

extern void _insque();
extern void _remque();

void scistart __P((int unit));
int scigo __P((int ctlr, int slave, int unit, struct buf *bp,
    struct scsi_fmt_cdb *cdb, int pad));
int sciintr __P((void));
void scidone __P((int unit));
int sciustart __P((int unit));
int scireq __P((register struct devqueue *dq));
void scifree __P((register struct devqueue *dq));
void scireset __P((int unit));
void sci_delay __P((int delay));
int sci_test_unit_rdy __P((int ctlr, int slave, int unit));
int sci_start_stop_unit __P((int ctlr, int slave, int unit, int start));
int sci_request_sense __P((int ctlr, int slave, int unit, u_char *buf,
    unsigned int len));
int sci_immed_command __P((int ctlr, int slave, int unit,
    struct scsi_fmt_cdb *cdb, u_char *buf, unsigned int len, int rd));
int sci_immed_command_nd __P((int ctlr, int slave, int unit,
     struct scsi_fmt_cdb *cdb));
int sci_tt_read __P((int ctlr, int slave, int unit, u_char *buf,
    u_int len, daddr_t blk, int bshift));
int sci_tt_write __P((int ctlr, int slave, int unit, u_char *buf,
    u_int len, daddr_t blk, int bshift));
#if NST > 0
int sci_tt_oddio __P((int ctlr, int slave, int unit, u_char *buf, u_int len, int b_flags, int freedma));
#endif


#if NMLHSCSI > 0
int mlhscsiinit ();

struct driver mlhscsidriver = {
	(int (*)(void *)) mlhscsiinit, "Mlhscsi", (int (*)(int)) scistart,
	(int (*)(int,...)) scigo, (int (*)(int,int)) sciintr,
	(int (*)())scidone, sciustart, scireq, scifree, scireset,
	sci_delay, sci_test_unit_rdy, sci_start_stop_unit,
	sci_request_sense, sci_immed_command, sci_immed_command_nd,
	sci_tt_read, sci_tt_write,
#if NST > 0
	sci_tt_oddio
#else
	NULL
#endif
};
#endif

#if NCSA12GSCSI > 0
int csa12gscsiinit ();

struct driver csa12gscsidriver = {
	(int (*)(void *)) csa12gscsiinit, "Csa12gscsi", (int (*)(int)) scistart,
	(int (*)(int,...)) scigo, (int (*)(int,int)) sciintr,
	(int (*)())scidone, sciustart, scireq, scifree, scireset,
	sci_delay, sci_test_unit_rdy, sci_start_stop_unit,
	sci_request_sense, sci_immed_command, sci_immed_command_nd,
	sci_tt_read, sci_tt_write,
#if NST > 0
	sci_tt_oddio
#else
	NULL
#endif
};
#endif

#if NSUPRASCSI > 0
int suprascsiinit ();

struct driver suprascsidriver = {
	(int (*)(void *)) suprascsiinit, "Suprascsi", (int (*)(int)) scistart,
	(int (*)(int,...)) scigo, (int (*)(int,int)) sciintr,
	(int (*)())scidone, sciustart, scireq, scifree, scireset,
	sci_delay, sci_test_unit_rdy, sci_start_stop_unit,
	sci_request_sense, sci_immed_command, sci_immed_command_nd,
	sci_tt_read, sci_tt_write,
#if NST > 0
	sci_tt_oddio
#else
	NULL
#endif
};
#endif

#if NIVSSCSI > 0
int ivsscsiinit ();

struct driver ivsscsidriver = {
	(int (*)(void *)) ivsscsiinit, "IVSscsi", (int (*)(int)) scistart,
	(int (*)(int,...)) scigo, (int (*)(int,int)) sciintr,
	(int (*)())scidone, sciustart, scireq, scifree, scireset,
	sci_delay, sci_test_unit_rdy, sci_start_stop_unit,
	sci_request_sense, sci_immed_command, sci_immed_command_nd,
	sci_tt_read, sci_tt_write,
#if NST > 0
	sci_tt_oddio
#else
	NULL
#endif
};
#endif

struct	sci_softc sci_softc[NSCI];

int sci_cmd_wait = SCSI_CMD_WAIT;
int sci_data_wait = SCSI_DATA_WAIT;
int sci_init_wait = SCSI_INIT_WAIT;

int sci_no_dma = 0;

#ifdef DEBUG
int	sci_debug = 0;
#define WAITHIST
#define QUASEL

static long	dmahits[NSCI];
static long	dmamisses[NSCI];
#endif

#ifdef QUASEL
#define QPRINTF(a) if (sci_debug > 1) printf a
#else
#define QPRINTF
#endif

#ifdef WAITHIST
#define MAXWAIT	1022
u_int	ixstart_wait[MAXWAIT+2];
u_int	ixin_wait[MAXWAIT+2];
u_int	ixout_wait[MAXWAIT+2];
u_int	mxin_wait[MAXWAIT+2];
u_int	mxin2_wait[MAXWAIT+2];
u_int	cxin_wait[MAXWAIT+2];
u_int	fxfr_wait[MAXWAIT+2];
u_int	sgo_wait[MAXWAIT+2];
#define HIST(h,w) (++h[((w)>MAXWAIT? MAXWAIT : ((w) < 0 ? -1 : (w))) + 1]);
#else
#define HIST(h,w)
#endif

#define	b_cylin		b_resid

static sci_wait (until, timeo, line)
	char until;
	int timeo;
	int line;
{
	register unsigned char val;

	if (! timeo)
		timeo = 1000000;	/* some large value.. */

	return val;
}

static void
scsiabort(dev, where)
	register struct sci_softc *dev;
	char *where;
{

	printf ("sci%d: abort %s: csr = 0x%02x, bus = 0x%02x\n",
	  dev->sc_ac->amiga_unit,
	  where, *dev->sci_csr, *dev->sci_bus_csr);

	if (dev->sc_flags & SCI_SELECTED) {

		/* XXX */
		scireset (dev->sc_ac->amiga_unit);
		/* lets just hope it worked.. */
		dev->sc_flags &= ~SCI_SELECTED;
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
sci_delay(delay)
	int delay;
{
	static int saved_cmd_wait, saved_data_wait;

	if (delay) {
		saved_cmd_wait = sci_cmd_wait;
		saved_data_wait = sci_data_wait;
		if (delay > 0)
			sci_cmd_wait = sci_data_wait = delay;
		else
			sci_cmd_wait = sci_data_wait = sci_init_wait;
	} else {
		sci_cmd_wait = saved_cmd_wait;
		sci_data_wait = saved_data_wait;
	}
}

static int initialized[NSCI];

#if NMLHSCSI > 0
int
mlhscsiinit(ac)
	register struct amiga_ctlr *ac;
{
	register struct sci_softc *dev = &sci_softc[ac->amiga_unit];

	if (! ac->amiga_addr)
		return 0;

	if (initialized[ac->amiga_unit])
		return 0;

	if (ac->amiga_unit > NSCI)
		return 0;

	initialized[ac->amiga_unit] = 1;

	/* advance ac->amiga_addr to point to the real sci-registers */
	ac->amiga_addr = (caddr_t) ((int)ac->amiga_addr);
	dev->sci_data = (caddr_t) ac->amiga_addr + 1;
	dev->sci_odata = (caddr_t) ac->amiga_addr + 1;
	dev->sci_icmd = (caddr_t) ac->amiga_addr + 3;
	dev->sci_mode = (caddr_t) ac->amiga_addr + 5;
	dev->sci_tcmd = (caddr_t) ac->amiga_addr + 7;
	dev->sci_bus_csr = (caddr_t) ac->amiga_addr + 9;
	dev->sci_sel_enb = (caddr_t) ac->amiga_addr + 9;
	dev->sci_csr = (caddr_t) ac->amiga_addr + 11;
	dev->sci_dma_send = (caddr_t) ac->amiga_addr + 11;
	dev->sci_idata = (caddr_t) ac->amiga_addr + 13;
	dev->sci_trecv = (caddr_t) ac->amiga_addr + 13;
	dev->sci_iack = (caddr_t) ac->amiga_addr + 15;
	dev->sci_irecv = (caddr_t) ac->amiga_addr + 15;
	mlhdmainit (dev);

	/* hardwired IPL */
	ac->amiga_ipl = 0;		/* doesn't use interrupts */
	dev->sc_ac = ac;
	dev->sc_sq.dq_forw = dev->sc_sq.dq_back = &dev->sc_sq;
	scireset (ac->amiga_unit);

	return(1);
}
#endif

#if NCSA12GSCSI > 0
int
csa12gscsiinit(ac)
	register struct amiga_ctlr *ac;
{
	register struct sci_softc *dev = &sci_softc[ac->amiga_unit];

	if (! ac->amiga_addr)
		return 0;

	if (initialized[ac->amiga_unit])
		return 0;

	if (ac->amiga_unit > NSCI)
		return 0;

	initialized[ac->amiga_unit] = 1;

	/* advance ac->amiga_addr to point to the real sci-registers */
	ac->amiga_addr = (caddr_t) ((int)ac->amiga_addr + 0x2000);
	dev->sci_data = (caddr_t) ac->amiga_addr;
	dev->sci_odata = (caddr_t) ac->amiga_addr;
	dev->sci_icmd = (caddr_t) ac->amiga_addr + 0x10;
	dev->sci_mode = (caddr_t) ac->amiga_addr + 0x20;
	dev->sci_tcmd = (caddr_t) ac->amiga_addr + 0x30;
	dev->sci_bus_csr = (caddr_t) ac->amiga_addr + 0x40;
	dev->sci_sel_enb = (caddr_t) ac->amiga_addr + 0x40;
	dev->sci_csr = (caddr_t) ac->amiga_addr + 0x50;
	dev->sci_dma_send = (caddr_t) ac->amiga_addr + 0x50;
	dev->sci_idata = (caddr_t) ac->amiga_addr + 0x60;
	dev->sci_trecv = (caddr_t) ac->amiga_addr + 0x60;
	dev->sci_iack = (caddr_t) ac->amiga_addr + 0x70;
	dev->sci_irecv = (caddr_t) ac->amiga_addr + 0x70;
	csa12gdmainit (dev);

	/* hardwired IPL */
	ac->amiga_ipl = 2;
	dev->sc_ac = ac;
	dev->sc_sq.dq_forw = dev->sc_sq.dq_back = &dev->sc_sq;
	scireset (ac->amiga_unit);

	/* make sure IPL2 interrupts are delivered to the cpu when the sci
	   generates some. Note that this does not yet enable sci-interrupts,
	   this is handled in dma.c, which selectively enables interrupts only
	   while DMA requests are pending.

	   Note that enabling PORTS interrupts also enables keyboard interrupts
	   as soon as the corresponding int-enable bit in CIA-A is set. */

	custom.intreq = INTF_PORTS;
	custom.intena = INTF_SETCLR | INTF_PORTS;
	return(1);
}
#endif

#if NSUPRASCSI > 0
int
suprascsiinit(ac)
	register struct amiga_ctlr *ac;
{
	register struct sci_softc *dev = &sci_softc[ac->amiga_unit];

	if (! ac->amiga_addr)
		return 0;

	if (initialized[ac->amiga_unit])
		return 0;

	if (ac->amiga_unit > NSCI)
		return 0;

	initialized[ac->amiga_unit] = 1;

	/* advance ac->amiga_addr to point to the real sci-registers */
	/* XXX Supra Word Sync version 2 only for now !!! */
	dev->sci_data = (caddr_t) ac->amiga_addr;
	dev->sci_odata = (caddr_t) ac->amiga_addr;
	dev->sci_icmd = (caddr_t) ac->amiga_addr + 2;
	dev->sci_mode = (caddr_t) ac->amiga_addr + 4;
	dev->sci_tcmd = (caddr_t) ac->amiga_addr + 6;
	dev->sci_bus_csr = (caddr_t) ac->amiga_addr + 8;
	dev->sci_sel_enb = (caddr_t) ac->amiga_addr + 8;
	dev->sci_csr = (caddr_t) ac->amiga_addr + 10;
	dev->sci_dma_send = (caddr_t) ac->amiga_addr + 10;
	dev->sci_idata = (caddr_t) ac->amiga_addr + 12;
	dev->sci_trecv = (caddr_t) ac->amiga_addr + 12;
	dev->sci_iack = (caddr_t) ac->amiga_addr + 14;
	dev->sci_irecv = (caddr_t) ac->amiga_addr + 14;
	supradmainit (dev);

	/* hardwired IPL */
	ac->amiga_ipl = 2;
	dev->sc_ac = ac;
	dev->sc_sq.dq_forw = dev->sc_sq.dq_back = &dev->sc_sq;
	scireset (ac->amiga_unit);

	/* make sure IPL2 interrupts are delivered to the cpu when the sci
	   generates some. Note that this does not yet enable sci-interrupts,
	   this is handled in dma.c, which selectively enables interrupts only
	   while DMA requests are pending.

	   Note that enabling PORTS interrupts also enables keyboard interrupts
	   as soon as the corresponding int-enable bit in CIA-A is set. */

	custom.intreq = INTF_PORTS;
	custom.intena = INTF_SETCLR | INTF_PORTS;
	return(1);
}
#endif

#if NIVSSCSI > 0
int
ivsscsiinit(ac)
	register struct amiga_ctlr *ac;
{
	register struct sci_softc *dev = &sci_softc[ac->amiga_unit];

	if (! ac->amiga_addr)
		return 0;

	if (initialized[ac->amiga_unit])
		return 0;

	if (ac->amiga_unit > NSCI)
		return 0;

	initialized[ac->amiga_unit] = 1;

	/* advance ac->amiga_addr to point to the real sci-registers */
	ac->amiga_addr = (caddr_t) ((int)ac->amiga_addr + 0x40);
	dev->sci_data = (caddr_t) ac->amiga_addr;
	dev->sci_odata = (caddr_t) ac->amiga_addr;
	dev->sci_icmd = (caddr_t) ac->amiga_addr + 2;
	dev->sci_mode = (caddr_t) ac->amiga_addr + 4;
	dev->sci_tcmd = (caddr_t) ac->amiga_addr + 6;
	dev->sci_bus_csr = (caddr_t) ac->amiga_addr + 8;
	dev->sci_sel_enb = (caddr_t) ac->amiga_addr + 8;
	dev->sci_csr = (caddr_t) ac->amiga_addr + 10;
	dev->sci_dma_send = (caddr_t) ac->amiga_addr + 10;
	dev->sci_idata = (caddr_t) ac->amiga_addr + 12;
	dev->sci_trecv = (caddr_t) ac->amiga_addr + 12;
	dev->sci_iack = (caddr_t) ac->amiga_addr + 14;
	dev->sci_irecv = (caddr_t) ac->amiga_addr + 14;
	ivsdmainit (dev);

	/* hardwired IPL */
	ac->amiga_ipl = 2;
	dev->sc_ac = ac;
	dev->sc_sq.dq_forw = dev->sc_sq.dq_back = &dev->sc_sq;
	scireset (ac->amiga_unit);

	/* make sure IPL2 interrupts are delivered to the cpu when the sci
	   generates some. Note that this does not yet enable sci-interrupts,
	   this is handled in dma.c, which selectively enables interrupts only
	   while DMA requests are pending.

	   Note that enabling PORTS interrupts also enables keyboard interrupts
	   as soon as the corresponding int-enable bit in CIA-A is set. */

	custom.intreq = INTF_PORTS;
	custom.intena = INTF_SETCLR | INTF_PORTS;
	return(1);
}
#endif

void
scireset(unit)
	register int unit;
{
	register struct sci_softc *dev = &sci_softc[unit];
	u_int i, s;
	u_char my_id, csr;

	if (dev->sc_flags & SCI_ALIVE)
		scsiabort(dev, "reset");

	printf("sci%d: ", unit);

	s = splbio();
	/* preserve our ID for now */
	my_id = 7;

	/*
	 * Disable interrupts (in dmainit) then reset the chip
	 */
	*dev->sci_icmd = SCI_ICMD_TEST;
	*dev->sci_icmd = SCI_ICMD_TEST | SCI_ICMD_RST;
	DELAY (25);
	*dev->sci_icmd = 0;

	/*
	 * Set up various chip parameters
	 */
	*dev->sci_icmd = 0;
	*dev->sci_tcmd = 0;
	*dev->sci_sel_enb = 0;

	/* anything else was zeroed by reset */

	splx (s);

	printf("sci id %d\n", my_id);
	dev->sc_flags |= SCI_ALIVE;
	dev->sc_flags &= ~SCI_SELECTED;
}

static void
scsierror(dev, csr)
	register struct sci_softc *dev;
	u_char csr;
{
	int unit = dev->sc_ac->amiga_unit;
	char *sep = "";

	printf("sci%d: ", unit);
	printf("\n");
}

static int
issue_select(dev, target, our_addr)
	register struct sci_softc *dev;
	u_char target, our_addr;
{
	register int timeo = 2500;

	QPRINTF (("issue_select %d\n", target));

	/* if we're already selected, return */
	if (dev->sc_flags & SCI_SELECTED)	/* XXXX */
		return 1;

	if ((*dev->sci_bus_csr & (SCI_BUS_BSY|SCI_BUS_SEL)) &&
	    (*dev->sci_bus_csr & (SCI_BUS_BSY|SCI_BUS_SEL)) &&
	    (*dev->sci_bus_csr & (SCI_BUS_BSY|SCI_BUS_SEL)))
		return 1;

	*dev->sci_tcmd = 0;
	*dev->sci_odata = 0x80 + (1 << target);
	*dev->sci_icmd = SCI_ICMD_DATA|SCI_ICMD_SEL;
	while ((*dev->sci_bus_csr & SCI_BUS_BSY) == 0) {
		if (--timeo > 0) {
			DELAY(100);
		} else {
			break;
		}
	}
	if (timeo) {
		*dev->sci_icmd = 0;
		dev->sc_flags |= SCI_SELECTED;
		return (0);
	}
	*dev->sci_icmd = 0;
	return (1);
}

static int
ixfer_out(dev, len, buf, phase)
	register struct sci_softc *dev;
	int len;
	register u_char *buf;
	int phase;
{
	register int wait = sci_data_wait;
	u_char csr;

	QPRINTF(("ixfer_out {%d} %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
	  len, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5],
	  buf[6], buf[7], buf[8], buf[9]));

	*dev->sci_tcmd = phase;
	*dev->sci_icmd = SCI_ICMD_DATA;
	for (;len > 0; len--) {
		csr = *dev->sci_bus_csr;
		while (!(csr & SCI_BUS_REQ)) {
			if ((csr & SCI_BUS_BSY) == 0 || --wait < 0) {
#ifdef DEBUG
				if (sci_debug)
					printf("ixfer_out fail: l%d i%x w%d\n",
					  len, csr, wait);
#endif
				HIST(ixout_wait, wait)
				return (len);
			}
			DELAY(1);
			csr = *dev->sci_bus_csr;
		}

		if (!(*dev->sci_csr & SCI_CSR_PHASE_MATCH))
			break;
		*dev->sci_odata = *buf;
		*dev->sci_icmd = SCI_ICMD_DATA|SCI_ICMD_ACK;
		buf++;
		while (*dev->sci_bus_csr & SCI_BUS_REQ);
		*dev->sci_icmd = SCI_ICMD_DATA;
	}

	QPRINTF(("ixfer_out done\n"));
	/* this leaves with one csr to be read */
	HIST(ixout_wait, wait)
	return (0);
}

static void
ixfer_in(dev, len, buf, phase)
	struct sci_softc *dev;
	int len;
	register u_char *buf;
	int phase;
{
	int wait = sci_data_wait;
	u_char *obp = buf;
	u_char csr;
	volatile register u_char *sci_bus_csr = dev->sci_bus_csr;
	volatile register u_char *sci_data = dev->sci_data;
	volatile register u_char *sci_icmd = dev->sci_icmd;

	csr = *sci_bus_csr;

	QPRINTF(("ixfer_in %d, csr=%02x\n", len, csr));

	*dev->sci_tcmd = phase;
	*sci_icmd = 0;
	for (;len > 0; len--) {
		csr = *sci_bus_csr;
		while (!(csr & SCI_BUS_REQ)) {
			if (!(csr & SCI_BUS_BSY) || --wait < 0) {
#ifdef DEBUG
				if (sci_debug)
					printf("ixfer_in fail: l%d i%x w%d\n",
					len, csr, wait);
#endif
				HIST(ixin_wait, wait)
				return;
			}

			DELAY(1);
			csr = *sci_bus_csr;
		}

		if (!(*dev->sci_csr & SCI_CSR_PHASE_MATCH))
			break;
		*buf = *sci_data;
		*sci_icmd = SCI_ICMD_ACK;
		buf++;
		while (*sci_bus_csr & SCI_BUS_REQ);
		*sci_icmd = 0;
	}

	QPRINTF(("ixfer_in {%d} %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
	  len, obp[0], obp[1], obp[2], obp[3], obp[4], obp[5],
	  obp[6], obp[7], obp[8], obp[9]));

	/* this leaves with one csr to be read */
	HIST(ixin_wait, wait)
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
 */
static int
scsiicmd(dev, target, cbuf, clen, buf, len, xferphase)
	struct sci_softc *dev;
	int target;
	u_char *cbuf;
	int clen;
	u_char *buf;
	int len;
	u_char xferphase;
{
	u_char phase, csr, asr;
	register int wait;

	/* select the SCSI bus (it's an error if bus isn't free) */
	if (issue_select (dev, target, dev->sc_scsi_addr))
		return -1;
	/*
	 * Wait for a phase change (or error) then let the device
	 * sequence us through the various SCSI phases.
	 */
	dev->sc_stat[0] = 0xff;
	dev->sc_msg[0] = 0xff;
	phase = CMD_PHASE;
	while (1) {
		wait = sci_cmd_wait;

		while ((*dev->sci_bus_csr & (SCI_BUS_REQ|SCI_BUS_BSY)) == SCI_BUS_BSY);

		QPRINTF((">CSR:%02x<", *dev->sci_bus_csr));
		if ((*dev->sci_bus_csr & SCI_BUS_REQ) == 0) {
			return -1;
		}
		phase = SCI_PHASE(*dev->sci_bus_csr);

		switch (phase) {
		case CMD_PHASE:
			if (ixfer_out (dev, clen, cbuf, phase))
				goto abort;
			phase = xferphase;
			break;

		case DATA_IN_PHASE:
			if (len <= 0)
				goto abort;
			wait = sci_data_wait;
			ixfer_in (dev, len, buf, phase);
			phase = STATUS_PHASE;
			break;

		case DATA_OUT_PHASE:
			if (len <= 0)
				goto abort;
			wait = sci_data_wait;
			if (ixfer_out (dev, len, buf, phase))
				goto abort;
			phase = STATUS_PHASE;
			break;

		case MESG_IN_PHASE:
			dev->sc_msg[0] = 0xff;
			ixfer_in (dev, 1, dev->sc_msg,phase);
			dev->sc_flags &= ~SCI_SELECTED;
			while (*dev->sci_bus_csr & SCI_BUS_BSY);
			goto out;
			break;

		case MESG_OUT_PHASE:
			phase = STATUS_PHASE;
			break;

		case STATUS_PHASE:
			ixfer_in (dev, 1, dev->sc_stat, phase);
			phase = MESG_IN_PHASE;
			break;

		case BUS_FREE_PHASE:
			goto out;

		default:
		printf("sci: unexpected phase %d in icmd from %d\n",
		  phase, target);
		goto abort;
		}
#if 0
		if (wait <= 0)
			goto abort;
#endif
	}

abort:
	scsiabort(dev, "icmd");
out:
	QPRINTF(("=STS:%02x=", dev->sc_stat[0]));
	return (dev->sc_stat[0]);
}

int
sci_test_unit_rdy(ctlr, slave, unit)
	int ctlr, slave, unit;
{
	register struct sci_softc *dev = &sci_softc[ctlr];
	static struct scsi_cdb6 cdb = { CMD_TEST_UNIT_READY };

	cdb.lun = unit;
	return (scsiicmd(dev, slave, (u_char *)&cdb, sizeof(cdb), (u_char *)0, 0,
			 STATUS_PHASE));
}

int
sci_start_stop_unit (ctlr, slave, unit, start)
	int ctlr, slave, unit;
{
	register struct sci_softc *dev = &sci_softc[ctlr];
	static struct scsi_cdb6 cdb = { CMD_LOADUNLOAD };

	cdb.lun = unit;
	/* we don't set the immediate bit, so we wait for the
	   command to succeed.
	   We also don't touch the LoEj bit, which is primarily meant
	   for floppies. */
	cdb.len = start & 0x01;
	return (scsiicmd(dev, slave, (u_char *)&cdb, sizeof(cdb), (u_char *)0, 0,
			 STATUS_PHASE));
}


int
sci_request_sense(ctlr, slave, unit, buf, len)
	int ctlr, slave, unit;
	u_char *buf;
	unsigned len;
{
	register struct sci_softc *dev = &sci_softc[ctlr];
	static struct scsi_cdb6 cdb = { CMD_REQUEST_SENSE };

	cdb.lun = unit;
	cdb.len = len;
	return (scsiicmd(dev, slave, (u_char *)&cdb, sizeof(cdb), buf, len, DATA_IN_PHASE));
}

int
sci_immed_command_nd(ctlr, slave, unit, cdb)
	int ctlr, slave, unit;
	struct scsi_fmt_cdb *cdb;
{
	register struct sci_softc *dev = &sci_softc[ctlr];

	cdb->cdb[1] |= (unit << 5);
	return(scsiicmd(dev, slave, (u_char *) cdb->cdb, cdb->len,
	    0, 0, STATUS_PHASE));
}

int
sci_immed_command(ctlr, slave, unit, cdb, buf, len, rd)
	int ctlr, slave, unit;
	struct scsi_fmt_cdb *cdb;
	u_char *buf;
	unsigned len;
{
	register struct sci_softc *dev = &sci_softc[ctlr];

	cdb->cdb[1] |= (unit << 5);
	return (scsiicmd(dev, slave, (u_char *) cdb->cdb, cdb->len, buf, len,
			 rd != 0? DATA_IN_PHASE : DATA_OUT_PHASE));
}

/*
 * The following routines are test-and-transfer i/o versions of read/write
 * for things like reading disk labels and writing core dumps.  The
 * routine scigo should be used for normal data transfers, NOT these
 * routines.
 */
int
sci_tt_read(ctlr, slave, unit, buf, len, blk, bshift)
	int ctlr, slave, unit;
	u_char *buf;
	u_int len;
	daddr_t blk;
	int bshift;
{
	register struct sci_softc *dev = &sci_softc[ctlr];
	struct scsi_cdb10 cdb;
	int stat;
	int old_wait = sci_data_wait;

	sci_data_wait = 300000;
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
	stat = scsiicmd(dev, slave, (u_char *) &cdb, sizeof(cdb), buf, len, DATA_IN_PHASE);
	sci_data_wait = old_wait;
	return (stat);
}

int
sci_tt_write(ctlr, slave, unit, buf, len, blk, bshift)
	int ctlr, slave, unit;
	u_char *buf;
	u_int len;
	daddr_t blk;
	int bshift;
{
	register struct sci_softc *dev = &sci_softc[ctlr];
	struct scsi_cdb10 cdb;
	int stat;
	int old_wait = sci_data_wait;

	sci_data_wait = 300000;

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
	stat = scsiicmd(dev, slave, (u_char *) &cdb, sizeof(cdb), buf, len, DATA_OUT_PHASE);
	sci_data_wait = old_wait;
	return (stat);
}

int
scireq(dq)
	register struct devqueue *dq;
{
	register struct devqueue *hq;

	hq = &sci_softc[dq->dq_ctlr].sc_sq;
	insque(dq, hq->dq_back);
	if (dq->dq_back == hq)
		return(1);
	return(0);
}

int
sciustart (int unit)
{
	register struct sci_softc *dev = &sci_softc[unit];

	/* If we got here, this controller is not busy
	   so we are ready to accept a command
	 */
	return(1);
}

void
scistart (int unit)
{
	register struct devqueue *dq;
	
	dq = sci_softc[unit].sc_sq.dq_forw;
	(dq->dq_driver->d_go)(dq->dq_unit);
}

int
scigo(ctlr, slave, unit, bp, cdb, pad)
	int ctlr, slave, unit;
	struct buf *bp;
	struct scsi_fmt_cdb *cdb;
	int pad;
{
	register struct sci_softc *dev = &sci_softc[ctlr];
	u_char phase, csr, asr, cmd;
	char *addr;
	int count;
	register struct devqueue *dq;

	cdb->cdb[1] |= unit << 5;

	addr = bp->b_un.b_addr;
	count = bp->b_bcount;

	if (sci_no_dma)	{

		scsiicmd (dev, slave, (u_char *) cdb->cdb, cdb->len,
		  addr, count,
		  bp->b_flags & B_READ ? DATA_IN_PHASE : DATA_OUT_PHASE);

		dq = dev->sc_sq.dq_forw;
		dev->sc_flags &=~ (SCI_IO);
		(dq->dq_driver->d_intr)(dq->dq_unit, dev->sc_stat[0]);
		return dev->sc_stat[0];
	}

	/* select the SCSI bus (it's an error if bus isn't free) */
	if (issue_select (dev, slave, dev->sc_scsi_addr))
		return -1;
	/*
	 * Wait for a phase change (or error) then let the device
	 * sequence us through the various SCSI phases.
	 */
	dev->sc_stat[0] = 0xff;
	dev->sc_msg[0] = 0xff;
	phase = CMD_PHASE;
	while (1) {
		while ((*dev->sci_bus_csr & (SCI_BUS_REQ|SCI_BUS_BSY)) ==
		  SCI_BUS_BSY);

		QPRINTF((">CSR:%02x<", *dev->sci_bus_csr));
		if ((*dev->sci_bus_csr & SCI_BUS_REQ) == 0) {
			goto abort;
		}
		phase = SCI_PHASE(*dev->sci_bus_csr);

		switch (phase) {
		case CMD_PHASE:
			if (ixfer_out (dev, cdb->len, cdb->cdb, phase))
				goto abort;
			phase = bp->b_flags & B_READ ? DATA_IN_PHASE : DATA_OUT_PHASE;
			break;

		case DATA_IN_PHASE:
			if (count <= 0)
				goto abort;
			/* XXX use psuedo DMA if available */
			if (count >= 128 && dev->dma_xfer_in)
				(*dev->dma_xfer_in)(dev, count, addr, phase);
			else
				ixfer_in (dev, count, addr, phase);
			phase = STATUS_PHASE;
			break;

		case DATA_OUT_PHASE:
			if (count <= 0)
				goto abort;
			/* XXX use psuedo DMA if available */
			if (count >= 128 && dev->dma_xfer_out)
				(*dev->dma_xfer_out)(dev, count, addr, phase);
			else
				if (ixfer_out (dev, count, addr, phase))
					goto abort;
			phase = STATUS_PHASE;
			break;

		case MESG_IN_PHASE:
			dev->sc_msg[0] = 0xff;
			ixfer_in (dev, 1, dev->sc_msg,phase);
			dev->sc_flags &= ~SCI_SELECTED;
			while (*dev->sci_bus_csr & SCI_BUS_BSY);
			goto out;
			break;

		case MESG_OUT_PHASE:
			phase = STATUS_PHASE;
			break;

		case STATUS_PHASE:
			ixfer_in (dev, 1, dev->sc_stat, phase);
			phase = MESG_IN_PHASE;
			break;

		case BUS_FREE_PHASE:
			goto out;

		default:
		printf("sci: unexpected phase %d in icmd from %d\n",
		  phase, slave);
		goto abort;
		}
	}

abort:
	scsiabort(dev, "go");
out:
	QPRINTF(("=STS:%02x=", dev->sc_stat[0]));
	dq = dev->sc_sq.dq_forw;
	dev->sc_flags &=~ (SCI_IO);
	(dq->dq_driver->d_intr)(dq->dq_unit, dev->sc_stat[0]);
	return dev->sc_stat[0];
}

void
scidone (int unit)
{

#ifdef DEBUG
	if (sci_debug)
		printf("sci%d: done called!\n", unit);
#endif
}

int
sciintr ()
{
	register struct sci_softc *dev = sci_softc;
	int unit;
	int dummy;
	int found = 0;

	for (unit = 0; unit < NSCI; ++unit, ++dev) {
		if (dev->sc_ac->amiga_ipl == 0)
			continue;
		/* XXX check if expecting interrupt? */
		if (dev->dma_intr)
			found += (*dev->dma_intr)(dev);
		else if ((*dev->sci_csr & SCI_CSR_INT)) {
			*dev->sci_mode = 0;
			dummy = *dev->sci_iack;
			++found;
		}
	}
	return found;
}

void
scifree(dq)
	register struct devqueue *dq;
{
	register struct devqueue *hq;

	hq = &sci_softc[dq->dq_ctlr].sc_sq;
	remque(dq);
	if ((dq = hq->dq_forw) != hq)
		(dq->dq_driver->d_start)(dq->dq_unit);
}

/*
 * (XXX) The following routine is needed for the SCSI tape driver
 * to read odd-size records.
 */

#if NST > 0
int
sci_tt_oddio(ctlr, slave, unit, buf, len, b_flags, freedma)
	int ctlr, slave, unit, b_flags;
	u_char *buf;
	u_int len;
{
	register struct sci_softc *dev = &sci_softc[ctlr];
	struct scsi_cdb6 cdb;
	u_char iphase;
	int stat;

	/*
	 * First free any DMA channel that was allocated.
	 * We can't use DMA to do this transfer.
	 */
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
	sci_delay(30000000);
	stat = scsiicmd(dev, slave, (u_char *) &cdb, sizeof(cdb), buf, len, iphase);
	sci_delay(0);
	return (stat);
}
#endif
#endif
