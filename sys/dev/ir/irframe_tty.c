/*	$NetBSD: irframe_tty.c,v 1.9 2001/12/05 14:50:14 augustss Exp $	*/

/*
 * TODO
 * Implement dongle support.
 * Test!!!
 * Get rid of MAX_IRDA_FRAME
 */

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) and Tommy Bohlin
 * (tommy@gatespace.com).
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
 * Loosely based on ppp_tty.c.
 * Framing and dongle handling written by Tommy Bohlin.
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/poll.h>

#include <dev/ir/ir.h>
#include <dev/ir/sir.h>
#include <dev/ir/irdaio.h>
#include <dev/ir/irframevar.h>

/* Macros to clear/set/test flags. */
#define	SET(t, f)	(t) |= (f)
#define	CLR(t, f)	(t) &= ~(f)
#define	ISSET(t, f)	((t) & (f))

#ifdef IRFRAMET_DEBUG
#define DPRINTF(x)	if (irframetdebug) printf x
#define Static
int irframetdebug = 0;
#else
#define DPRINTF(x)
#define Static static
#endif

/*****/

#define MAX_IRDA_FRAME 5000	/* XXX what is it? */

struct frame {
	u_char *buf;
	u_int len;
};
#define MAXFRAMES 4

struct irframet_softc {
	struct irframe_softc sc_irp;
	struct tty *sc_tp;

	int sc_dongle;

	int sc_state;
#define	IRT_RSLP		0x01	/* waiting for data (read) */
#if 0
#define	IRT_WSLP		0x02	/* waiting for data (write) */
#define IRT_CLOSING		0x04	/* waiting for output to drain */
#endif

	int sc_ebofs;
	int sc_speed;

	u_char* sc_inbuf;
	int sc_maxsize;
	int sc_framestate;
#define FRAME_OUTSIDE    0
#define FRAME_INSIDE     1
#define FRAME_ESCAPE     2
	int sc_inchars;
	int sc_inFCS;
	struct callout sc_timeout;

	u_int sc_nframes;
	u_int sc_framei;
	u_int sc_frameo;
	struct frame sc_frames[MAXFRAMES];
	struct selinfo sc_rsel;
};

/* line discipline methods */
int	irframetopen(dev_t dev, struct tty *tp);
int	irframetclose(struct tty *tp, int flag);
int	irframetioctl(struct tty *tp, u_long cmd, caddr_t data, int flag,
		      struct proc *);
int	irframetinput(int c, struct tty *tp);
int	irframetstart(struct tty *tp);

/* pseudo device init */
void	irframettyattach(int);

/* irframe methods */
Static int	irframet_open(void *h, int flag, int mode, struct proc *p);
Static int	irframet_close(void *h, int flag, int mode, struct proc *p);
Static int	irframet_read(void *h, struct uio *uio, int flag);
Static int	irframet_write(void *h, struct uio *uio, int flag);
Static int	irframet_poll(void *h, int events, struct proc *p);
Static int	irframet_set_params(void *h, struct irda_params *params);
Static int	irframet_get_speeds(void *h, int *speeds);
Static int	irframet_get_turnarounds(void *h, int *times);

/* internal */
Static int	irt_write_frame(void *h, u_int8_t *buf, size_t len);
Static int	irt_putc(int c, struct tty *tp);
Static void	irt_frame(struct irframet_softc *sc, u_char *buf, u_int len);
Static void	irt_timeout(void *v);

Static struct irframe_methods irframet_methods = {
	irframet_open, irframet_close, irframet_read, irframet_write,
	irframet_poll, irframet_set_params,
	irframet_get_speeds, irframet_get_turnarounds
};

void
irframettyattach(int n)
{
}

/*
 * Line specific open routine for async tty devices.
 * Attach the given tty to the first available irframe unit.
 * Called from device open routine or ttioctl.
 */
/* ARGSUSED */
int
irframetopen(dev_t dev, struct tty *tp)
{
	struct proc *p = curproc;		/* XXX */
	struct irframet_softc *sc;
	int error, s;

	DPRINTF(("%s\n", __FUNCTION__));

	if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
		return (error);

	s = spltty();

	DPRINTF(("%s: linesw=%p disc=%s\n", __FUNCTION__, tp->t_linesw,
		 tp->t_linesw->l_name));
	if (strcmp(tp->t_linesw->l_name, "irframe") == 0) { /* XXX */
		sc = (struct irframet_softc *)tp->t_sc;
		DPRINTF(("%s: sc=%p sc_tp=%p\n", __FUNCTION__, sc, sc->sc_tp));
		if (sc != NULL) {
			splx(s);
			return (EBUSY);
		}
	}

	tp->t_sc = irframe_alloc(sizeof (struct irframet_softc),
			&irframet_methods, tp);
	sc = (struct irframet_softc *)tp->t_sc;
	sc->sc_tp = tp;
	printf("%s attached at tty%02d\n", sc->sc_irp.sc_dev.dv_xname,
	    minor(tp->t_dev));

	DPRINTF(("%s: set sc=%p\n", __FUNCTION__, sc));

	ttyflush(tp, FREAD | FWRITE);

	sc->sc_dongle = DONGLE_NONE;

	splx(s);

	return (0);
}

/*
 * Line specific close routine, called from device close routine
 * and from ttioctl.
 * Detach the tty from the irframe unit.
 * Mimics part of ttyclose().
 */
int
irframetclose(struct tty *tp, int flag)
{
	struct irframet_softc *sc = (struct irframet_softc *)tp->t_sc;
	int s;

	DPRINTF(("%s: tp=%p\n", __FUNCTION__, tp));

	s = spltty();
	ttyflush(tp, FREAD | FWRITE);
	tp->t_linesw = linesw[0]; /* default line discipline */
	if (sc != NULL) {
		tp->t_sc = NULL;
		printf("%s detached from tty%02d\n", sc->sc_irp.sc_dev.dv_xname,
		    minor(tp->t_dev));

		if (sc->sc_tp == tp)
			irframe_dealloc(&sc->sc_irp.sc_dev);
	}
	splx(s);
	return (0);
}

/*
 * Line specific (tty) ioctl routine.
 * This discipline requires that tty device drivers call
 * the line specific l_ioctl routine from their ioctl routines.
 */
/* ARGSUSED */
int
irframetioctl(struct tty *tp, u_long cmd, caddr_t data, int flag, 
	     struct proc *p)
{
	struct irframet_softc *sc = (struct irframet_softc *)tp->t_sc;
	int error;

	DPRINTF(("%s: tp=%p\n", __FUNCTION__, tp));

	if (sc == NULL || tp != sc->sc_tp)
		return (-1);

	error = 0;
	switch (cmd) {
	case IRFRAMETTY_GET_DEVICE:
		*(int *)data = sc->sc_irp.sc_dev.dv_unit;
		break;
	case IRFRAMETTY_GET_DONGLE:
		*(int *)data = sc->sc_dongle;
		break;
	case IRFRAMETTY_SET_DONGLE:
		sc->sc_dongle = *(int *)data;
		break;
	default:
		error = EINVAL;
		break;
	}

	return (error);
}

/*
 * Start output on async tty interface.
 * Called at spltty or higher.
 */
int
irframetstart(struct tty *tp)
{
	/*struct irframet_softc *sc = (struct irframet_softc *)tp->t_sc;*/

	DPRINTF(("%s: tp=%p\n", __FUNCTION__, tp));

	if (tp->t_oproc != NULL)
		(*tp->t_oproc)(tp);

	return (0);
}

void
irt_frame(struct irframet_softc *sc, u_char *buf, u_int len)
{
	DPRINTF(("%s: nframe=%d framei=%d frameo=%d\n",
		 __FUNCTION__, sc->sc_nframes, sc->sc_framei, sc->sc_frameo));

	if (sc->sc_nframes >= MAXFRAMES) {
#ifdef IRFRAMET_DEBUG
		printf("%s: dropped frame\n", __FUNCTION__);
#endif
		return;
	}
	if (sc->sc_frames[sc->sc_framei].buf == NULL)
		return;
	memcpy(sc->sc_frames[sc->sc_framei].buf, buf, len);
	sc->sc_frames[sc->sc_framei].len = len;
	sc->sc_framei = (sc->sc_framei+1) % MAXFRAMES;
	sc->sc_nframes++;
	if (sc->sc_state & IRT_RSLP) {
		sc->sc_state &= ~IRT_RSLP;
		DPRINTF(("%s: waking up reader\n", __FUNCTION__));
		wakeup(sc->sc_frames);
	}
	selwakeup(&sc->sc_rsel);
}

void
irt_timeout(void *v)
{
	struct irframet_softc *sc = v;

#ifdef IRFRAMET_DEBUG
	if (sc->sc_framestate != FRAME_OUTSIDE)
		printf("%s: input frame timeout\n", __FUNCTION__);
#endif
	sc->sc_framestate = FRAME_OUTSIDE;
}

int
irframetinput(int c, struct tty *tp)
{
	struct irframet_softc *sc = (struct irframet_softc *)tp->t_sc;

	c &= 0xff;

#if IRFRAMET_DEBUG
	if (irframetdebug > 1)
		DPRINTF(("%s: tp=%p c=0x%02x\n", __FUNCTION__, tp, c));
#endif

	if (sc == NULL || tp != (struct tty *)sc->sc_tp)
		return (0);

	if (sc->sc_inbuf == NULL)
		return (0);

	switch (c) {
	case SIR_BOF:
		DPRINTF(("%s: BOF\n", __FUNCTION__));
		sc->sc_framestate = FRAME_INSIDE;
		sc->sc_inchars = 0;
		sc->sc_inFCS = INITFCS;
		break;
	case SIR_EOF:
		DPRINTF(("%s: EOF state=%d inchars=%d fcs=0x%04x\n",
			 __FUNCTION__,
			 sc->sc_framestate, sc->sc_inchars, sc->sc_inFCS));
		if (sc->sc_framestate == FRAME_INSIDE &&
		    sc->sc_inchars >= 4 && sc->sc_inFCS == GOODFCS) {
			irt_frame(sc, sc->sc_inbuf, sc->sc_inchars - 2);
		} else if (sc->sc_framestate != FRAME_OUTSIDE) {
#ifdef IRFRAMET_DEBUG
			printf("%s: malformed input frame\n", __FUNCTION__);
#endif
		}
		sc->sc_framestate = FRAME_OUTSIDE;
		break;
	case SIR_CE:
		DPRINTF(("%s: CE\n", __FUNCTION__));
		if (sc->sc_framestate == FRAME_INSIDE)
			sc->sc_framestate = FRAME_ESCAPE;
		break;
	default:
#if IRFRAMET_DEBUG
	if (irframetdebug > 1)
		DPRINTF(("%s: c=0x%02x, inchar=%d state=%d\n", __FUNCTION__, c,
			 sc->sc_inchars, sc->sc_state));
#endif
		if (sc->sc_framestate != FRAME_OUTSIDE) {
			if (sc->sc_framestate == FRAME_ESCAPE) {
				sc->sc_framestate = FRAME_INSIDE;
				c ^= SIR_ESC_BIT;
			}
			if (sc->sc_inchars < sc->sc_maxsize + 2) {
				sc->sc_inbuf[sc->sc_inchars++] = c;
				sc->sc_inFCS = updateFCS(sc->sc_inFCS, c);
			} else {
				sc->sc_framestate = FRAME_OUTSIDE;
#ifdef IRFRAMET_DEBUG
				printf("%s: input frame overrun\n",
				       __FUNCTION__);
#endif
			}
		}
		break;
	}

#if 1
	if (sc->sc_framestate != FRAME_OUTSIDE) {
		callout_reset(&sc->sc_timeout, hz/20, irt_timeout, sc);
	}
#endif

	return (0);
}


/*** irframe methods ***/

int
irframet_open(void *h, int flag, int mode, struct proc *p)
{
	struct tty *tp = h;
	struct irframet_softc *sc = (struct irframet_softc *)tp->t_sc;

	DPRINTF(("%s: tp=%p\n", __FUNCTION__, tp));

	sc->sc_speed = 0;
	sc->sc_ebofs = IRDA_DEFAULT_EBOFS;
	sc->sc_maxsize = 0;
	sc->sc_framestate = FRAME_OUTSIDE;
	sc->sc_nframes = 0;
	sc->sc_framei = 0;
	sc->sc_frameo = 0;
	callout_init(&sc->sc_timeout);

	return (0);
}

int
irframet_close(void *h, int flag, int mode, struct proc *p)
{
	struct tty *tp = h;
	struct irframet_softc *sc = (struct irframet_softc *)tp->t_sc;
	int i, s;

	DPRINTF(("%s: tp=%p\n", __FUNCTION__, tp));

	callout_stop(&sc->sc_timeout);
	s = splir();
	if (sc->sc_inbuf != NULL) {
		free(sc->sc_inbuf, M_DEVBUF);
		sc->sc_inbuf = NULL;
	}
	for (i = 0; i < MAXFRAMES; i++) {
		if (sc->sc_frames[i].buf != NULL) {
			free(sc->sc_frames[i].buf, M_DEVBUF);
			sc->sc_frames[i].buf = NULL;
		}
	}
	splx(s);

	return (0);
}

int
irframet_read(void *h, struct uio *uio, int flag)
{
	struct tty *tp = h;
	struct irframet_softc *sc = (struct irframet_softc *)tp->t_sc;
	int error = 0;
	int s;

	DPRINTF(("%s: resid=%d, iovcnt=%d, offset=%ld\n", 
		 __FUNCTION__, uio->uio_resid, uio->uio_iovcnt, 
		 (long)uio->uio_offset));
	DPRINTF(("%s: nframe=%d framei=%d frameo=%d\n",
		 __FUNCTION__, sc->sc_nframes, sc->sc_framei, sc->sc_frameo));


	s = splir();	
	while (sc->sc_nframes == 0) {
		if (flag & IO_NDELAY) {
			splx(s);
			return (EWOULDBLOCK);
		}
		sc->sc_state |= IRT_RSLP;
		DPRINTF(("%s: sleep\n", __FUNCTION__));
		error = tsleep(sc->sc_frames, PZERO | PCATCH, "irtrd", 0);
		DPRINTF(("%s: woke, error=%d\n", __FUNCTION__, error));
		if (error) {
			sc->sc_state &= ~IRT_RSLP;
			break;
		}
	}

	/* Do just one frame transfer per read */
	if (!error) {
		if (uio->uio_resid < sc->sc_frames[sc->sc_frameo].len) {
			DPRINTF(("%s: uio buffer smaller than frame size (%d < %d)\n", __FUNCTION__, uio->uio_resid, sc->sc_frames[sc->sc_frameo].len));
			error = EINVAL;
		} else {
			DPRINTF(("%s: moving %d bytes\n", __FUNCTION__,
				 sc->sc_frames[sc->sc_frameo].len));
			error = uiomove(sc->sc_frames[sc->sc_frameo].buf,
					sc->sc_frames[sc->sc_frameo].len, uio);
			DPRINTF(("%s: error=%d\n", __FUNCTION__, error));
		}
		sc->sc_frameo = (sc->sc_frameo+1) % MAXFRAMES;
		sc->sc_nframes--;
	}
	splx(s);

	return (error);
}

int
irt_putc(int c, struct tty *tp)
{
	int s;
	int error;

#if IRFRAMET_DEBUG
	if (irframetdebug > 3)
		DPRINTF(("%s: tp=%p c=0x%02x cc=%d\n", __FUNCTION__, tp, c,
			 tp->t_outq.c_cc));
#endif
	if (tp->t_outq.c_cc > tp->t_hiwat) {
		irframetstart(tp);
		s = spltty();
		/*
		 * This can only occur if FLUSHO is set in t_lflag,
		 * or if ttstart/oproc is synchronous (or very fast).
		 */
		if (tp->t_outq.c_cc <= tp->t_hiwat) {
			splx(s);
			goto go;
		}
		SET(tp->t_state, TS_ASLEEP);
		error = ttysleep(tp, &tp->t_outq, TTOPRI | PCATCH, ttyout, 0);
		splx(s);
		if (error)
			return (error);
	}
 go:
	if (putc(c, &tp->t_outq) < 0) {
		printf("irframe: putc failed\n");
		return (EIO);
	}
	return (0);
}

int
irframet_write(void *h, struct uio *uio, int flag)
{
	struct tty *tp = h;
	struct irframet_softc *sc = (struct irframet_softc *)tp->t_sc;
	u_int8_t buf[MAX_IRDA_FRAME];
	int n;

	DPRINTF(("%s: resid=%d, iovcnt=%d, offset=%ld\n", 
		 __FUNCTION__, uio->uio_resid, uio->uio_iovcnt, 
		 (long)uio->uio_offset));

	n = irda_sir_frame(buf, MAX_IRDA_FRAME, uio, sc->sc_ebofs);
	if (n < 0)
		return (-n);
	return (irt_write_frame(h, buf, n));
}

int
irt_write_frame(void *h, u_int8_t *buf, size_t len)
{
	struct tty *tp = h;
	int error, i;

	DPRINTF(("%s: tp=%p len=%d\n", __FUNCTION__, tp, len));

	error = 0;
	for (i = 0; !error && i < len; i++)
		error = irt_putc(buf[i], tp);

	irframetstart(tp);

	DPRINTF(("%s: done, error=%d\n", __FUNCTION__, error));

	return (error);
}

int
irframet_poll(void *h, int events, struct proc *p)
{
	struct tty *tp = h;
	struct irframet_softc *sc = (struct irframet_softc *)tp->t_sc;
	int revents = 0;
	int s;

	DPRINTF(("%s: sc=%p\n", __FUNCTION__, sc));

	s = splir();
	/* XXX is this a good check? */
	if (events & (POLLOUT | POLLWRNORM))
		if (tp->t_outq.c_cc <= tp->t_lowat)
			revents |= events & (POLLOUT | POLLWRNORM);

	if (events & (POLLIN | POLLRDNORM)) {
		if (sc->sc_nframes > 0) {
			DPRINTF(("%s: have data\n", __FUNCTION__));
			revents |= events & (POLLIN | POLLRDNORM);
		} else {
			DPRINTF(("%s: recording select\n", __FUNCTION__));
			selrecord(p, &sc->sc_rsel);
		}
	}
	splx(s);

	return (revents);
}

int
irframet_set_params(void *h, struct irda_params *p)
{
	struct tty *tp = h;
	struct irframet_softc *sc = (struct irframet_softc *)tp->t_sc;	
	struct termios tt;
	int i;

	DPRINTF(("%s: tp=%p speed=%d ebofs=%d maxsize=%d\n",
		 __FUNCTION__, tp, p->speed, p->ebofs, p->maxsize));

	if (p->speed != sc->sc_speed) {
		switch (p->speed) {
		case   2400:
		case   9600:
		case  19200:
		case  38400:
		case  57600:
		case 115200:
			break;
		default: return (EINVAL);
		}
		ttioctl(tp, TIOCGETA,  (caddr_t)&tt, 0, curproc);
		sc->sc_speed = tt.c_ispeed = tt.c_ospeed = p->speed;
		ttioctl(tp, TIOCSETAF, (caddr_t)&tt, 0, curproc);
	}

	sc->sc_ebofs = p->ebofs;
	if (sc->sc_maxsize != p->maxsize) {
		sc->sc_maxsize = p->maxsize;
		if (sc->sc_inbuf != NULL)
			free(sc->sc_inbuf, M_DEVBUF);
		for (i = 0; i < MAXFRAMES; i++)
			if (sc->sc_frames[i].buf != NULL)
				free(sc->sc_frames[i].buf, M_DEVBUF);
		if (sc->sc_maxsize != 0) {
			sc->sc_inbuf = malloc(sc->sc_maxsize+2, M_DEVBUF,
					      M_WAITOK);
			for (i = 0; i < MAXFRAMES; i++)
				sc->sc_frames[i].buf = malloc(sc->sc_maxsize,
							   M_DEVBUF, M_WAITOK);
		} else {
			sc->sc_inbuf = NULL;
			for (i = 0; i < MAXFRAMES; i++)
				sc->sc_frames[i].buf = NULL;
		}
	}
	sc->sc_framestate = FRAME_OUTSIDE;

	return (0);
}

int
irframet_get_speeds(void *h, int *speeds)
{
	struct tty *tp = h;

	tp = tp;
	DPRINTF(("%s: tp=%p\n", __FUNCTION__, tp));
	*speeds = IRDA_SPEEDS_SIR;
	return (0);
}

int
irframet_get_turnarounds(void *h, int *turnarounds)
{
	struct tty *tp = h;

	tp = tp;
	DPRINTF(("%s: tp=%p\n", __FUNCTION__, tp));
	*turnarounds = IRDA_TURNT_10000;
	return (0);
}
