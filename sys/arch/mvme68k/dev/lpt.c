/*	$NetBSD: lpt.c,v 1.4.6.1 1999/01/30 21:58:41 scw Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Device Driver for an MVME1xx board's parallel printer port
 * This driver attaches above the board-specific back-end.
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

#include <mvme68k/dev/lptvar.h>


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
	struct device		sc_dev;
	struct lpt_funcs	*sc_funcs;
	void			*sc_arg;
	size_t			sc_count;
	struct buf		*sc_inbuf;
	u_char			*sc_cp;
	int			sc_spinmax;
	u_char			sc_state;
#define	LPT_OPEN	0x01	/* device is open */
#define	LPT_OBUSY	0x02	/* printer is busy doing output */
#define	LPT_INIT	0x04	/* waiting to initialize for open */
	u_char			sc_flags;
#define	LPT_FAST_STROBE	0x10	/* Select 1.6uS strobe pulse */
#define	LPT_AUTOLF	0x20	/* automatic LF on CR */
#define	LPT_NOPRIME	0x40	/* don't prime on open */
#define	LPT_NOINTR	0x80	/* do not use interrupt */
};

/* {b,c}devsw[] function prototypes */
dev_type_open(lptopen);
dev_type_close(lptclose);
dev_type_write(lptwrite);
dev_type_ioctl(lptioctl);



/*
 * Autoconfig stuff
 */
static int  lpt_match  __P((struct device *, struct cfdata *, void *));
static void lpt_attach __P((struct device *, struct device *, void *));

struct cfattach lpt_ca = {
	sizeof(struct lpt_softc), lpt_match, lpt_attach
};

extern struct cfdriver lpt_cd;

#define	LPTUNIT(s)	(minor(s) & 0x0f)
#define	LPTFLAGS(s)	(minor(s) & 0xf0)

static void lpt_wakeup __P((void *arg));
static int lpt_intr __P((void *));
static int pushbytes __P((struct lpt_softc *));


/*ARGSUSED*/
static	int
lpt_match(pdp, cf, auxp)
	struct device *pdp;
	struct cfdata *cf;
	void *auxp;
{
	if (strcmp((char *)auxp, lpt_cd.cd_name))
		return (0);
	return (1);
}

/*ARGSUSED*/
static void
lpt_attach(pdp, dp, auxp)
struct	device *pdp, *dp;
void	*auxp;
{
	struct lpt_softc *sc = (void *)dp;
	struct lpt_attach_args *pa = auxp;

	/*
	 * Get pointer to regs
	 */
	sc->sc_funcs = pa->pa_funcs;
	sc->sc_arg = sc->sc_arg;
	sc->sc_state = 0;

	/*
	 * Hook the printer interrupt
	 */
	(sc->sc_funcs->lf_hook_int)(sc->sc_arg, lpt_intr, sc);

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
		(sc->sc_funcs->lf_iprime)(sc->sc_arg);
	}

	/* select fast or slow strobe depending on minor device number */
	if ( flags & LPT_FAST_STROBE )
		(sc->sc_funcs->lf_speed)(sc->sc_arg, LPT_STROBE_FAST);
	else
		(sc->sc_funcs->lf_speed)(sc->sc_arg, LPT_STROBE_SLOW);

	/* wait till ready (printer running diagnostics) */
	for (spin = 0; (sc->sc_funcs->lf_notrdy)(sc->sc_arg, 1); spin += STEP) {
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

	if ( (sc->sc_flags & LPT_NOINTR) == 0 )
		lpt_wakeup(sc);

	(sc->sc_funcs->lf_open)(sc->sc_arg, sc->sc_flags & LPT_NOINTR);

	LPRINTF(("%s: opened\n", sc->sc_dev.dv_xname));
	return 0;
}

void
lpt_wakeup(arg)
	void *arg;
{
	struct lpt_softc *sc = arg;
	int s;

	s = spltty();
	lpt_intr(sc);
	splx(s);

	timeout(lpt_wakeup, sc, STEP);
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

	if ( (sc->sc_flags & LPT_NOINTR) == 0 )
		untimeout(lpt_wakeup, sc);

	(sc->sc_funcs->lf_close)(sc->sc_arg);

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
			while ( (sc->sc_funcs->lf_notrdy)(sc->sc_arg, 0) ) {
				if (++spin < sc->sc_spinmax)
					continue;
				tic = 0;
				/* adapt busy-wait algorithm */
				sc->sc_spinmax++;
				while((sc->sc_funcs->lf_notrdy)(sc->sc_arg,1)) {
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

			(sc->sc_funcs->lf_wrdata)(sc->sc_arg, *sc->sc_cp++);
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
				(void) lpt_intr(sc);
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
lpt_intr(arg)
	void *arg;
{
	struct lpt_softc *sc = arg;

	if (sc->sc_count) {
		/* send char */
		(sc->sc_funcs->lf_wrdata)(sc->sc_arg, *sc->sc_cp++);
		sc->sc_count--;
		sc->sc_state |= LPT_OBUSY;
	} else
		sc->sc_state &= ~LPT_OBUSY;

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
