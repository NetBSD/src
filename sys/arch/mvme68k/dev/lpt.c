/*	$NetBSD: lpt.c,v 1.2 1997/03/19 16:24:39 gwr Exp $	*/

/*
 * Copyright (c) 1996 Steve Woodford
 * Copyright (c) 1993, 1994 Charles Hannum.
 * Copyright (c) 1990 William F. Jolitz, TeleMuse
 * All rights reserved.
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
 *	This software is a component of "386BSD" developed by 
 *	William F. Jolitz, TeleMuse.
 * 4. Neither the name of the developer nor the name "386BSD"
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS A COMPONENT OF 386BSD DEVELOPED BY WILLIAM F. JOLITZ 
 * AND IS INTENDED FOR RESEARCH AND EDUCATIONAL PURPOSES ONLY. THIS 
 * SOFTWARE SHOULD NOT BE CONSIDERED TO BE A COMMERCIAL PRODUCT. 
 * THE DEVELOPER URGES THAT USERS WHO REQUIRE A COMMERCIAL PRODUCT 
 * NOT MAKE USE OF THIS WORK.
 *
 * FOR USERS WHO WISH TO UNDERSTAND THE 386BSD SYSTEM DEVELOPED
 * BY WILLIAM F. JOLITZ, WE RECOMMEND THE USER STUDY WRITTEN 
 * REFERENCES SUCH AS THE  "PORTING UNIX TO THE 386" SERIES 
 * (BEGINNING JANUARY 1991 "DR. DOBBS JOURNAL", USA AND BEGINNING 
 * JUNE 1991 "UNIX MAGAZIN", GERMANY) BY WILLIAM F. JOLITZ AND 
 * LYNNE GREER JOLITZ, AS WELL AS OTHER BOOKS ON UNIX AND THE 
 * ON-LINE 386BSD USER MANUAL BEFORE USE. A BOOK DISCUSSING THE INTERNALS 
 * OF 386BSD ENTITLED "386BSD FROM THE INSIDE OUT" WILL BE AVAILABLE LATE 1992.
 *
 * THIS SOFTWARE IS PROVIDED BY THE DEVELOPER ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE DEVELOPER BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Device Driver for the MVME147's parallel printer port
 * Based mostly on the AT parallel port driver by Charles Hannum and
 * William F. Jolitz.
 * Some additional code is based on the NetBSD/atari parallel port
 * driver, by Leo Weppelman.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/syslog.h>

#include <machine/cpu.h>

#include <mvme68k/dev/lptreg.h>
#include <mvme68k/dev/pccreg.h>
#include <mvme68k/dev/pccvar.h>

#define	TIMEOUT		hz*16	/* wait up to 16 seconds for a ready */
#define	STEP		hz/4

#define	LPTPRI		(PZERO+8)
#define	LPT_BSIZE	1024

#if !defined(DEBUG) || !defined(notdef)
#define LPRINTF(a)
#else
#define LPRINTF		if (lptdebug) printf a
int lptdebug = 1;
#endif

struct lpt_softc {
	struct device sc_dev;
	union lpt_regs *sc_regs;
	size_t sc_count;
	struct buf *sc_inbuf;
	u_char *sc_cp;
	int sc_spinmax;
	int sc_ipl;
	u_char sc_state;
#define	LPT_OPEN	0x01	/* device is open */
#define	LPT_OBUSY	0x02	/* printer is busy doing output */
#define	LPT_INIT	0x04	/* waiting to initialize for open */
	u_char sc_flags;
#define	LPT_FAST_STROBE	0x10	/* Select 1.6uS strobe pulse */
#define	LPT_AUTOLF	0x20	/* automatic LF on CR */
#define	LPT_NOPRIME	0x40	/* don't prime on open */
#define	LPT_NOINTR	0x80	/* do not use interrupt */
	u_char sc_prir;
	u_char sc_laststatus;
};

/* {b,c}devsw[] function prototypes */
dev_type_open(lptopen);
dev_type_close(lptclose);
dev_type_write(lptwrite);
dev_type_ioctl(lptioctl);

int lptintr __P((void *));


/*
 * Autoconfig stuff
 */
static int  lpt_pcc_match  __P((struct device *, struct cfdata *, void *));
static void lpt_pcc_attach __P((struct device *, struct device *, void *));

struct cfattach lpt_pcc_ca = {
	sizeof(struct lpt_softc), lpt_pcc_match, lpt_pcc_attach
};

struct cfdriver lpt_cd = {
	NULL, "lpt", DV_DULL, NULL, 0
};

#define	LPTUNIT(s)	(minor(s) & 0x0f)
#define	LPTFLAGS(s)	(minor(s) & 0xf0)

#define	LPS_INVERT	(LPS_SELECT)
#define	LPS_MASK	(LPS_SELECT|LPS_FAULT|LPS_BUSY|LPS_PAPER_EMPTY)
#define	NOT_READY()	((sc->sc_regs->pr_status ^ LPS_INVERT) & LPS_MASK)
#define	NOT_READY_ERR()	not_ready(sc->sc_regs->pr_status, sc)
static int not_ready __P((u_char, struct lpt_softc *));

static void lptwakeup __P((void *arg));
static int pushbytes __P((struct lpt_softc *));


/*ARGSUSED*/
static	int
lpt_pcc_match(pdp, cf, auxp)
	struct device *pdp;
	struct cfdata *cf;
	void *auxp;
{
	struct pcc_attach_args *pa = auxp;

	if (strcmp(pa->pa_name, lpt_cd.cd_name))
		return (0);

	pa->pa_ipl = cf->pcccf_ipl;
	return (1);
}

/*ARGSUSED*/
static void
lpt_pcc_attach(pdp, dp, auxp)
struct	device *pdp, *dp;
void	*auxp;
{
	struct lpt_softc *sc = (void *)dp;
	struct pcc_attach_args *pa = auxp;

	/*
	 * Get pointer to regs
	 */
	sc->sc_regs = (union lpt_regs *) PCC_VADDR(pa->pa_offset);

	sc->sc_ipl = pa->pa_ipl & PCC_IMASK;
	sc->sc_state = 0;
	sc->sc_laststatus = 0;

	/*
	 * Hook into the printer interrupt
	 */
	pccintr_establish(PCCV_PRINTER, lptintr, sc->sc_ipl, sc);

	/*
	 * Disable interrupts until device is opened
	 */
	sys_pcc->pr_int = 0;

	printf("\n");
}

/*
 * Reset the printer, then wait until it's selected and not busy.
 */
int
lptopen(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	int unit = LPTUNIT(dev);
	u_char flags = LPTFLAGS(dev);
	struct lpt_softc *sc;
	int error;
	int spin;

	if (unit >= lpt_cd.cd_ndevs)
		return ENXIO;
	sc = lpt_cd.cd_devs[unit];
	if (!sc)
		return ENXIO;

#ifdef DIAGNOSTIC
	if (sc->sc_state)
		printf("%s: stat=0x%x not zero\n", sc->sc_dev.dv_xname,
		    sc->sc_state);
#endif

	if (sc->sc_state)
		return EBUSY;

	sc->sc_state = LPT_INIT;
	sc->sc_flags = flags;
	LPRINTF(("%s: open: flags=0x%x\n", sc->sc_dev.dv_xname, flags));

	if ((flags & LPT_NOPRIME) == 0) {
		/* assert Input Prime for 100 usec to start up printer */
		sys_pcc->pr_cr = LPC_INPUT_PRIME;
		delay(100);
	}

	/* select fast or slow strobe depending on minor device number */
	if ( flags & LPT_FAST_STROBE )
		sys_pcc->pr_cr = LPC_FAST_STROBE;
	else
		sys_pcc->pr_cr = 0;

	/* wait till ready (printer running diagnostics) */
	for (spin = 0; NOT_READY_ERR(); spin += STEP) {
		if (spin >= TIMEOUT) {
			sc->sc_state = 0;
			return EBUSY;
		}

		/* wait 1/4 second, give up if we get a signal */
		error = tsleep((caddr_t)sc, LPTPRI | PCATCH, "lptopen", STEP);
		if (error != EWOULDBLOCK) {
			sc->sc_state = 0;
			return error;
		}
	}

	sc->sc_inbuf = geteblk(LPT_BSIZE);
	sc->sc_count = 0;
	sc->sc_state = LPT_OPEN;
	sc->sc_prir = 0;

	sys_pcc->pr_int = LPI_ACKINT | LPI_FAULTINT;

	if ((sc->sc_flags & LPT_NOINTR) == 0)
	{
		int sps;

		lptwakeup(sc);

		sc->sc_prir = sc->sc_ipl | LPI_ENABLE;
		sps = splhigh();
		sys_pcc->pr_int = sc->sc_prir;
		splx(sps);
	}

	LPRINTF(("%s: opened\n", sc->sc_dev.dv_xname));
	return 0;
}

int
not_ready(status, sc)
	u_char status;
	struct lpt_softc *sc;
{
	u_char new;

	status = (status ^ LPS_INVERT) & LPS_MASK;
	new = status & ~sc->sc_laststatus;
	sc->sc_laststatus = status;

	if (new & LPS_SELECT)
		log(LOG_NOTICE, "%s: offline\n", sc->sc_dev.dv_xname);
	else if (new & LPS_PAPER_EMPTY)
		log(LOG_NOTICE, "%s: out of paper\n", sc->sc_dev.dv_xname);
	else if (new & LPS_FAULT)
		log(LOG_NOTICE, "%s: output error\n", sc->sc_dev.dv_xname);

	sys_pcc->pr_int = sc->sc_prir | LPI_FAULTINT;

	return status;
}

void
lptwakeup(arg)
	void *arg;
{
	struct lpt_softc *sc = arg;
	int s;

	s = spltty();
	lptintr(sc);
	splx(s);

	timeout(lptwakeup, sc, STEP);
}

/*
 * Close the device, and free the local line buffer.
 */
int
lptclose(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	int unit = LPTUNIT(dev);
	struct lpt_softc *sc = lpt_cd.cd_devs[unit];

	if (sc->sc_count)
		(void) pushbytes(sc);

	if ((sc->sc_flags & LPT_NOINTR) == 0)
	{
		int sps;

		untimeout(lptwakeup, sc);

		sc->sc_prir = 0;
		sps = splhigh();
		sys_pcc->pr_int = LPI_ACKINT | LPI_FAULTINT;
		splx(sps);
	}

	sc->sc_state = 0;
	brelse(sc->sc_inbuf);

	LPRINTF(("%s: closed\n", sc->sc_dev.dv_xname));
	return 0;
}

int
pushbytes(sc)
	struct lpt_softc *sc;
{
	int error;

	if (sc->sc_flags & LPT_NOINTR) {
		int spin, tic;

		while (sc->sc_count > 0) {
			spin = 0;
			while (NOT_READY()) {
				if (++spin < sc->sc_spinmax)
					continue;
				tic = 0;
				/* adapt busy-wait algorithm */
				sc->sc_spinmax++;
				while (NOT_READY_ERR()) {
					/* exponential backoff */
					tic = tic + tic + 1;
					if (tic > TIMEOUT)
						tic = TIMEOUT;
					error = tsleep((caddr_t)sc,
					    LPTPRI | PCATCH, "lptpsh", tic);
					if (error != EWOULDBLOCK)
						return error;
				}
				break;
			}

			sc->sc_regs->pr_data = *sc->sc_cp++;
			sc->sc_count--;

			/* adapt busy-wait algorithm */
			if (spin*2 + 16 < sc->sc_spinmax)
				sc->sc_spinmax--;
		}
	} else {
		int s;

		while (sc->sc_count > 0) {
			/* if the printer is ready for a char, give it one */
			if ((sc->sc_state & LPT_OBUSY) == 0) {
				LPRINTF(("%s: write %d\n", sc->sc_dev.dv_xname,
				    sc->sc_count));
				s = spltty();
				(void) lptintr(sc);
				splx(s);
			}
			error = tsleep((caddr_t)sc, LPTPRI | PCATCH,
			    "lptwrite2", 0);
			if (error)
				return error;
		}
	}
	return 0;
}

/* 
 * Copy a line from user space to a local buffer, then call putc to get the
 * chars moved to the output queue.
 */
int
lptwrite(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	struct lpt_softc *sc = lpt_cd.cd_devs[LPTUNIT(dev)];
	size_t n;
	int error = 0;

	while ((n = min(LPT_BSIZE, uio->uio_resid)) != 0) {
		uiomove(sc->sc_cp = sc->sc_inbuf->b_data, n, uio);
		sc->sc_count = n;
		error = pushbytes(sc);
		if (error) {
			/*
			 * Return accurate residual if interrupted or timed
			 * out.
			 */
			uio->uio_resid += sc->sc_count;
			sc->sc_count = 0;
			return error;
		}
	}
	return 0;
}

/*
 * Handle printer interrupts which occur when the printer is ready to accept
 * another char.
 */
int
lptintr(arg)
	void *arg;
{
	struct lpt_softc *sc = arg;

#if 0
	if ((sc->sc_state & LPT_OPEN) == 0)
		return 0;
#endif

	/* is printer online and ready for output */
	if (NOT_READY() && NOT_READY_ERR())
		return 0;

	if (sc->sc_count) {
		/* send char */
		sc->sc_regs->pr_data = *sc->sc_cp++;
		sc->sc_count--;
		sc->sc_state |= LPT_OBUSY;
	} else
		sc->sc_state &= ~LPT_OBUSY;

	if ( sys_pcc->pr_int & LPI_ACKINT )
		sys_pcc->pr_int = sc->sc_prir | LPI_ACKINT;

	if (sc->sc_count == 0) {
		/* none, wake up the top half to get more */
		wakeup((caddr_t)sc);
	}

	return 1;
}

int
lptioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	int error = 0;

	switch (cmd) {
	default:
		error = ENODEV;
	}

	return error;
}
