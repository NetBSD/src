/*
 * Copyright (c) 1993 Charles Hannum.
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
 *
 *	$Id: lpt.c,v 1.7.4.17 1993/10/29 21:35:20 mycroft Exp $
 */

/*
 * Device Driver for AT parallel printer port
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
#include <sys/syslog.h>

#include <machine/cpu.h>
#include <machine/pio.h>

#include <i386/isa/isavar.h>
#include <i386/isa/icu.h>
#include <i386/isa/lptreg.h>

#define	TIMEOUT		hz*16	/* wait up to 4 seconds for a ready */
#define	STEP		hz/4

#define	MAX_SPIN	255

#define	LPTPRI		(PZERO+8)
#define	LPT_BSIZE	1024

#ifndef DEBUG
#define lprintf
#else
#define lprintf		if (lptdebug) printf
int lptdebug = 1;
#endif

struct lpt_softc {
	struct	device sc_dev;
	struct	isadev sc_id;
	struct	intrhand sc_ih;

	size_t	sc_count;
	struct	buf *sc_inbuf;
	u_char	*sc_cp ;
	u_short	sc_iobase;
	u_short	sc_irq;
	u_char	sc_state;
/* bits for state */
#define	LPT_OPEN	0x01	/* device is open */
#define	LPT_OBUSY	0x02	/* printer is busy doing output */
#define LPT_INIT	0x04	/* waiting to initialize for open */
	u_char	sc_flags;
#define	LPT_AUTOLF	0x20	/* automatic LF on CR */
#define	LPT_NOPRIME	0x40	/* don't prime on open */
#define	LPT_NOINTR	0x80	/* do not use interrupt */
	u_char	sc_control;
	u_char	sc_smax;
};

static int lptprobe __P((struct device *, struct cfdata *, void *));
#if 0 /* XXX */
static void lptforceintr __P((void *));
#endif
static void lptattach __P((struct device *, struct device *, void *));
static int lptintr __P((void *));

struct	cfdriver lptcd =
{ NULL, "lpt", lptprobe, lptattach, DV_TTY, sizeof(struct lpt_softc) };

#define	LPTUNIT(s)	((s)&0x1f)
#define	LPTFLAGS(s)	((s)&0xe0)

#define	LPS_INVERT	(LPS_SELECT|LPS_NERR|LPS_NBSY|LPS_NACK)
#define	LPS_MASK	(LPS_SELECT|LPS_NERR|LPS_NBSY|LPS_NACK|LPS_NOPAPER)
#define	NOT_READY()	notready(inb(iobase + lpt_status), sc)

static int notready __P((u_char, struct lpt_softc *));
static void lptout __P((struct lpt_softc *));
static int pushbytes __P((struct lpt_softc *));

/*
 * Internal routine to lptprobe to do port tests of one byte value
 */
int
lpt_port_test(port, data, mask)
	u_short port;
	u_char data, mask;
{
	int	timeout;
	u_char	temp;

	data &= mask;
	outb(port, data);
	timeout = 100;
	do {
		temp = inb(port) & mask;
	} while (temp != data && --timeout);
	lprintf("lpt: port=0x%x out=0x%x in=0x%x\n", port, data, temp);
	return (temp == data);
}

/*
 * Logic:
 *	1) You should be able to write to and read back the same value
 *	   to the data port.  Do an alternating zeros, alternating ones,
 *	   walking zero, and walking one test to check for stuck bits.
 *
 *	2) You should be able to write to and read back the same value
 *	   to the control port lower 5 bits, the upper 3 bits are reserved
 *	   per the IBM PC technical reference manauls and different boards
 *	   do different things with them.  Do an alternating zeros, alternating
 *	   ones, walking zero, and walking one test to check for stuck bits.
 *
 *	   Some printers drag the strobe line down when the are powered off
 * 	   so this bit has been masked out of the control port test.
 *
 *	   XXX Some printers may not like a fast pulse on init or strobe, I
 *	   don't know at this point, if that becomes a problem these bits
 *	   should be turned off in the mask byte for the control port test.
 *
 *	3) Set the data and control ports to a value of 0
 */

static int
lptprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct	isa_attach_args *ia = aux;
	u_short	iobase = ia->ia_iobase;
	u_short	port = iobase + lpt_data;
	u_char	mask = 0xff;
	u_char	data;
	int	i;

	if (iobase == IOBASEUNK)
		return 0;

#ifdef DEBUG
#define	ABORT	do {printf("lptprobe: mask %x data %x failed\n", mask, data); \
		    return 0;} while (0)
#else
#define	ABORT	return 0
#endif

	for (;;) {
		data = 0x55;				/* Alternating zeros */
		if (!lpt_port_test(port, data, mask))
			ABORT;

		data = 0xaa;				/* Alternating ones */
		if (!lpt_port_test(port, data, mask))
			ABORT;

		for (i = 0; i < CHAR_BIT; i++) {	/* Walking zero */
			data = ~(1 << i);
			if (!lpt_port_test(port, data, mask))
				ABORT;
		}

		for (i = 0; i < CHAR_BIT; i++) {	/* Walking one */
			data = (1 << i);
			if (!lpt_port_test(port, data, mask))
				ABORT;
		}

		if (port == iobase + lpt_data) {
			port = iobase + lpt_control;
#if 1 /* XXX */
			mask = 0x04;
#else
			mask = 0x1e;
#endif
		} else
			break;
	}
	outb(iobase + lpt_data, 0);
	outb(iobase + lpt_control, 0);

	if (ia->ia_irq == IRQUNK)
#if 0 /* XXX */
		ia->ia_irq = isa_discoverintr(lptforceintr, aux);
#else
		ia->ia_irq = IRQNONE;
#endif
	/* okay if we don't have an irq; will force polling */

	ia->ia_iosize = LPT_NPORTS;
	ia->ia_drq = DRQUNK;
	ia->ia_msize = 0;
	return 1;
}

#if 0 /* XXX */
static void
lptforceintr(aux)
	void *aux;
{
	struct isa_attach_args *ia = aux;
	u_short iobase = ia->ia_iobase;

	outb(iobase + lpt_control, LPC_IENABLE);
	delay(100);
	outb(iobase + lpt_control, 0);
}
#endif

static void
lptattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct	isa_attach_args *ia = aux;
	struct	lpt_softc *sc = (struct lpt_softc *)self;
	u_short	iobase = ia->ia_iobase;
	u_short	irq = ia->ia_irq;

	sc->sc_iobase = iobase;
	sc->sc_irq = irq;
	sc->sc_state = 0;
	outb(iobase + lpt_control, LPC_NINIT);

	printf(": %sparallel port\n", irq == IRQNONE ? "polled " : "");
	isa_establish(&sc->sc_id, &sc->sc_dev);

	if (irq != IRQNONE) {
		sc->sc_ih.ih_fun = lptintr;
		sc->sc_ih.ih_arg = sc;
		intr_establish(ia->ia_irq, &sc->sc_ih, DV_TTY);
	}
}

/*
 * lptopen -- reset the printer, then wait until it's selected and not busy.
 */
int
lptopen(dev, flag)
	dev_t dev;
	int flag;
{
	int	unit = LPTUNIT(minor(dev));
	u_char	flags = LPTFLAGS(minor(dev));
	struct	lpt_softc *sc;
	u_short	iobase;
	u_char	control;
	int	error;
	int	spin;

	if (unit >= lptcd.cd_ndevs)
		return ENXIO;
	sc = lptcd.cd_devs[unit];
	if (!sc)
		return ENXIO;

	if (sc->sc_irq == IRQNONE && (flags & LPT_NOINTR) == 0)
		return ENXIO;

	if (sc->sc_state)
		return EBUSY;

#ifdef DIAGNOSTIC
	if (sc->sc_state)
		printf("%s: state=0x%x not zero\n", sc->sc_dev.dv_xname,
		       sc->sc_state);
#endif

	sc->sc_state = LPT_INIT;

	sc->sc_flags = flags;
	lprintf("%s: open flags=0x%x\n", sc->sc_dev.dv_xname, flags);
	iobase = sc->sc_iobase;

	if ((flags & LPT_NOPRIME) == 0) {
		/* assert INIT for 100 usec to start up printer */
		outb(iobase + lpt_control, LPC_SELECT);
		delay(100);
	}

	control = LPC_SELECT | LPC_NINIT;
	outb(iobase + lpt_control, control);

	/* wait till ready (printer running diagnostics) */
	for (spin = 0; NOT_READY(); spin += STEP) {
		if (spin >= TIMEOUT) {
			sc->sc_state = 0;
			return EBUSY;
		}

		/* wait 1/4 second, give up if we get a signal */
		if (error = tsleep((caddr_t)sc,
		    LPTPRI | PCATCH, "lptopen", STEP) != EWOULDBLOCK) {
			sc->sc_state = 0;
			return error;
		}
	}
		
	if ((flags & LPT_NOINTR) == 0)
		control |= LPC_IENABLE;
	if (flags & LPT_AUTOLF)
		control |= LPC_AUTOLF;
	sc->sc_control = control;
	outb(iobase + lpt_control, control);

	sc->sc_state = LPT_OPEN;
	sc->sc_inbuf = geteblk(LPT_BSIZE);
	sc->sc_count = 0;
	lptout(sc);

	lprintf("%s: opened\n", sc->sc_dev.dv_xname);
	return 0;
}

static int
notready(status, sc)
	u_char status;
	struct lpt_softc *sc;
{
	status ^= LPS_INVERT;

	if (status & LPS_NOPAPER)
		log(LOG_NOTICE, "%s: out of paper\n", sc->sc_dev.dv_xname);
	else if (status & LPS_SELECT)
		log(LOG_NOTICE, "%s: offline\n", sc->sc_dev.dv_xname);
	else if (status & LPS_NERR)
		log(LOG_NOTICE, "%s: output error\n", sc->sc_dev.dv_xname);

	return status & LPS_MASK;
}

static void
lptout(sc)
	struct lpt_softc *sc;
{
	int	s;

	if ((sc->sc_state & LPT_OPEN) == 0)
		return;
	if (sc->sc_flags & LPT_NOINTR)
		return;

	/*
	 * Avoid possible hangs due to missed interrupts
	 */
	if (sc->sc_count) {
		s = spltty();
		(void) lptintr(sc);
		splx(s);
	} else {
		sc->sc_state &= ~LPT_OBUSY;
		wakeup((caddr_t)sc);
	}

	timeout((timeout_t)lptout, (caddr_t)sc, STEP);
}

/*
 * lptclose -- close the device, free the local line buffer.
 */
int
lptclose(dev, flag)
	dev_t dev;
	int flag;
{
	int	unit = LPTUNIT(minor(dev));
	struct	lpt_softc *sc = lptcd.cd_devs[unit];
	u_short	iobase = sc->sc_iobase;

	(void) pushbytes(sc);

	outb(iobase + lpt_control, LPC_NINIT);
	brelse(sc->sc_inbuf);
	sc->sc_state = 0;

	lprintf("%s: closed\n", sc->sc_dev.dv_xname);
	return 0;
}

static int
pushbytes(sc)
	struct lpt_softc *sc;
{
	u_short	iobase = sc->sc_iobase;
	int	error;

	if (sc->sc_flags & LPT_NOINTR) {
		int	spin, tic;
		u_char	control = sc->sc_control;
		u_char	ch;

		while (sc->sc_count > 0) {
			spin = tic = 0;
			while (NOT_READY())
				if (++spin >= sc->sc_smax) {
					/* exponential backoff */
					tic = tic + tic + 1;
					if (error = tsleep((caddr_t)sc,
					    LPTPRI | PCATCH, "lptwrite1", tic))
						return error;
				}

			outb(iobase + lpt_data, *sc->sc_cp++);
			outb(iobase + lpt_control, control | LPC_STROBE);
			sc->sc_count--;
			outb(iobase + lpt_control, control);

			/* adapt busy-wait algorithm */
			if (spin >= sc->sc_smax && sc->sc_smax < MAX_SPIN)
				sc->sc_smax++;
			if (spin*2 < sc->sc_smax)
				sc->sc_smax--;
		}
	} else {
		int	s;

		while (sc->sc_count > 0) {
			/* if the printer is ready for a char, give it one */
			if ((sc->sc_state & LPT_OBUSY) == 0) {
				lprintf("%s: write %d\n", sc->sc_dev.dv_xname,
					sc->sc_count);
				s = spltty();
				(void) lptintr(sc);
				splx(s);
			}
			if (error = tsleep((caddr_t)sc,
			    LPTPRI | PCATCH, "lptwrite2", 0))
				return error;
		}
	}
	return 0;
}

/* 
 * copy a line from user space to a local buffer, then call 
 * putc to get the chars moved to the output queue.
 */
int
lptwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	int	unit = LPTUNIT(minor(dev));
	struct	lpt_softc *sc = lptcd.cd_devs[unit];
	size_t	n;
	int	error = 0;

	while (n = MIN(LPT_BSIZE, uio->uio_resid)) {
		uiomove(sc->sc_cp = sc->sc_inbuf->b_un.b_addr, n, uio);
		sc->sc_count = n;
		error = pushbytes(sc);
		if (error) {
			/* return accurate residual if interrupted or
			   timed out */
			uio->uio_resid += sc->sc_count;
			sc->sc_count = 0;
			return error;
		}
	}
	return 0;
}

/*
 * lptintr -- handle printer interrupts which occur when the printer is
 * ready to accept another char.
 */
static int
lptintr(arg)
	void *arg;
{
	struct	lpt_softc *sc = (struct lpt_softc *)arg;
	u_short	iobase = sc->sc_iobase;
	u_char	control = sc->sc_control;
	u_char	status;

	if ((sc->sc_state & LPT_OPEN) == 0)
		return 0;

	/* is printer online and ready for output */
	if (NOT_READY())
		return 0;

	if (sc->sc_count) {
		/* send char */
		sc->sc_state |= LPT_OBUSY;
		while (sc->sc_count && !NOT_READY()) {
			outb(iobase + lpt_data, *sc->sc_cp++);
			outb(iobase + lpt_control, control | LPC_STROBE);
			sc->sc_count--;
			outb(iobase + lpt_control, control);
		}
	} else {
		/* none, wake up the top half to get more */
		sc->sc_state &= ~LPT_OBUSY;
		wakeup((caddr_t)sc);
	}

	return 1;
}

int
lptioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	int	error = 0;

	switch (cmd) {
	    default:
		error = ENODEV;
	}

	return error;
}
