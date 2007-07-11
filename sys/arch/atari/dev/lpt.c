/*	$NetBSD: lpt.c,v 1.25.4.1 2007/07/11 19:58:22 mjf Exp $ */

/*
 * Copyright (c) 1996 Leo Weppelman
 * Copyright (c) 1993, 1994 Charles M. Hannum.
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
 * Device Driver originally written for AT parallel printer port. Now
 * drives the printer port on the YM2149.
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lpt.c,v 1.25.4.1 2007/07/11 19:58:22 mjf Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
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
#include <machine/iomap.h>
#include <machine/mfp.h>

#include <atari/dev/ym2149reg.h>
#include <atari/atari/intr.h>

#define	TIMEOUT		hz*16	/* wait up to 16 seconds for a ready */
#define	STEP		hz/4

#define	LPTPRI		(PZERO+8)
#define	LPT_BSIZE	1024

#if !defined(DEBUG) || !defined(notdef)
#define lprintf		if (0) printf
#else
#define lprintf		if (lptdebug) printf
int lptdebug = 1;
#endif

struct lpt_softc {
	struct device	sc_dev;
	struct callout	sc_wakeup_ch;
	size_t		sc_count;
	struct buf	*sc_inbuf;
	u_char		*sc_cp;
	int		sc_spinmax;
	u_char		sc_state;
#define	LPT_OPEN	0x01	/* device is open */
#define	LPT_OBUSY	0x02	/* printer is busy doing output */
#define	LPT_INIT	0x04	/* waiting to initialize for open */
	u_char		sc_flags;
#define	LPT_AUTOLF	0x20	/* automatic LF on CR XXX: LWP - not yet... */
#define	LPT_NOINTR	0x40	/* do not use interrupt */
};

#define	LPTUNIT(s)	(minor(s) & 0x1f)
#define	LPTFLAGS(s)	(minor(s) & 0xe0)
#define	NOT_READY()	(MFP->mf_gpip & IO_PBSY)

/* {b,c}devsw[] function prototypes */
dev_type_open(lpopen);
dev_type_close(lpclose);
dev_type_write(lpwrite);
dev_type_ioctl(lpioctl);

static void lptwakeup __P((void *arg));
static int pushbytes __P((struct lpt_softc *));
static void lptpseudointr __P((struct lpt_softc *));
int lptintr __P((struct lpt_softc *));
int lpthwintr __P((struct lpt_softc *, int));


/*
 * Autoconfig stuff
 */
static void lpattach __P((struct device *, struct device *, void *));
static int  lpmatch __P((struct device *, struct cfdata *, void *));

CFATTACH_DECL(lp, sizeof(struct lpt_softc),
    lpmatch, lpattach, NULL, NULL);

extern struct cfdriver lp_cd;

const struct cdevsw lp_cdevsw = {
	lpopen, lpclose, noread, lpwrite, lpioctl,
	nostop, notty, nopoll, nommap, nokqfilter,
};

/*ARGSUSED*/
static	int
lpmatch(pdp, cfp, auxp)
struct	device	*pdp;
struct	cfdata	*cfp;
void		*auxp;
{
	static int	lpt_matched = 0;

	/* Match at most 1 lpt unit */
	if (strcmp((char *)auxp, "lpt") || lpt_matched)
		return 0;
	lpt_matched = 1;
	return (1);
}

/*ARGSUSED*/
static void
lpattach(pdp, dp, auxp)
struct	device *pdp, *dp;
void	*auxp;
{
	struct lpt_softc *sc = (void *)dp;

	sc->sc_state = 0;

	if (intr_establish(0, USER_VEC, 0, (hw_ifun_t)lpthwintr, sc) == NULL)
		printf("lptattach: Can't establish interrupt\n");
	ym2149_strobe(1);

	printf("\n");

	callout_init(&sc->sc_wakeup_ch, 0);
}

/*
 * Reset the printer, then wait until it's selected and not busy.
 */
int
lpopen(dev, flag, mode, l)
	dev_t		dev;
	int		flag, mode;
	struct lwp	*l;
{
	int			unit = LPTUNIT(dev);
	u_char			flags = LPTFLAGS(dev);
	struct lpt_softc	*sc;
	int 			error;
	int			spin;
	int			sps;

	if (unit >= lp_cd.cd_ndevs)
		return ENXIO;
	sc = lp_cd.cd_devs[unit];
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
	lprintf("%s: open: flags=0x%x\n", sc->sc_dev.dv_xname, flags);

	/* wait till ready (printer running diagnostics) */
	for (spin = 0; NOT_READY(); spin += STEP) {
		if (spin >= TIMEOUT) {
			sc->sc_state = 0;
			return EBUSY;
		}

		/* wait 1/4 second, give up if we get a signal */
		if ((error = tsleep((void *)sc, LPTPRI | PCATCH, "lptopen",
		     STEP)) != EWOULDBLOCK) {
			sc->sc_state = 0;
			return error;
		}
	}

	sc->sc_inbuf = geteblk(LPT_BSIZE);
	sc->sc_count = 0;
	sc->sc_state = LPT_OPEN;

	if ((sc->sc_flags & LPT_NOINTR) == 0) {
		lptwakeup(sc);

		sps = splhigh();
		MFP->mf_imrb |= IB_PBSY;
		MFP->mf_ierb |= IB_PBSY;
		splx(sps);
	}

	lprintf("%s: opened\n", sc->sc_dev.dv_xname);
	return 0;
}

void
lptwakeup(arg)
	void *arg;
{
	struct lpt_softc *sc = arg;

	lptpseudointr(sc);

	callout_reset(&sc->sc_wakeup_ch, STEP, lptwakeup, sc);
}

/*
 * Close the device, and free the local line buffer.
 */
int
lpclose(dev, flag, mode, l)
	dev_t		dev;
	int		flag;
	int		mode;
	struct lwp	*l;
{
	int		 unit = LPTUNIT(dev);
	struct lpt_softc *sc = lp_cd.cd_devs[unit];
	int		 sps;

	if (sc->sc_count)
		(void) pushbytes(sc);

	if ((sc->sc_flags & LPT_NOINTR) == 0) {
		callout_stop(&sc->sc_wakeup_ch);

		sps = splhigh();
		MFP->mf_ierb &= ~IB_PBSY;
		MFP->mf_imrb &= ~IB_PBSY;
		splx(sps);
	}

	sc->sc_state = 0;
	brelse(sc->sc_inbuf);

	lprintf("%s: closed\n", sc->sc_dev.dv_xname);
	return 0;
}

int
pushbytes(sc)
	struct lpt_softc *sc;
{
	int	error;

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
				while (NOT_READY()) {
					/* exponential backoff */
					tic = tic + tic + 1;
					if (tic > TIMEOUT)
						tic = TIMEOUT;
					error = tsleep((void *)sc,
					    LPTPRI | PCATCH, "lptpsh", tic);
					if (error != EWOULDBLOCK)
						return error;
				}
				break;
			}

			ym2149_write_ioport(YM_IOB, *sc->sc_cp++);
			ym2149_strobe(0);
			sc->sc_count--;
			ym2149_strobe(1);

			/* adapt busy-wait algorithm */
			if (spin*2 + 16 < sc->sc_spinmax)
				sc->sc_spinmax--;
		}
	} else {
		while (sc->sc_count > 0) {
			/* if the printer is ready for a char, give it one */
			if ((sc->sc_state & LPT_OBUSY) == 0) {
				lprintf("%s: write %d\n", sc->sc_dev.dv_xname,
				    sc->sc_count);
				(void) lptpseudointr(sc);
			}
			if ((error = tsleep((void *)sc, LPTPRI | PCATCH,
			     "lptwrite2", 0)) != 0)
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
lpwrite(dev, uio, flags)
	dev_t		dev;
	struct uio	*uio;
	int		flags;
{
	struct lpt_softc *sc = lp_cd.cd_devs[LPTUNIT(dev)];
	size_t n;
	int error = 0;

	while ((n = min(LPT_BSIZE, uio->uio_resid)) > 0) {
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
lptintr(sc)
struct lpt_softc *sc;
{
	/* is printer online and ready for output */
	if (NOT_READY())
		return 0;

	if (sc->sc_count) {

		/* send char */
		ym2149_write_ioport(YM_IOB, *sc->sc_cp++);
		ym2149_strobe(0);
		sc->sc_count--;
		ym2149_strobe(1);
		sc->sc_state |= LPT_OBUSY;
	} else
		sc->sc_state &= ~LPT_OBUSY;

	if (sc->sc_count == 0) {
		/* none, wake up the top half to get more */
		wakeup((void *)sc);
	}

	return 1;
}

static void
lptpseudointr(sc)
struct lpt_softc *sc;
{
	int	s;

	s = spltty();
	lptintr(sc);
	splx(s);
}

int
lpthwintr(sc, sr)
struct lpt_softc *sc;
int		  sr;
{
	if (!BASEPRI(sr))
		add_sicallback((si_farg)lptpseudointr, sc, 0);
	else lptpseudointr(sc);
	return 1;
}

int
lpioctl(dev, cmd, data, flag, l)
	dev_t		dev;
	u_long		cmd;
	void *		data;
	int		flag;
	struct lwp	*l;
{
	int error = 0;

	switch (cmd) {
	default:
		error = ENODEV;
	}

	return error;
}
