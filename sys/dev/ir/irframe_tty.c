/*	$NetBSD: irframe_tty.c,v 1.8 2001/12/05 04:31:02 augustss Exp $	*/

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
 * Framing and dongle handing written by Tommy Bohlin.
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

/* Protocol constants */
#define SIR_EXTRA_BOF            0xff
#define SIR_BOF                  0xc0
#define SIR_CE                   0x7d
#define SIR_EOF                  0xc1

#define SIR_ESC_BIT              0x20
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
Static int	irt_write_buf(void *h, void *buf, size_t len);
Static int	irt_putc(int c, struct tty *tp);
Static int	irt_putesc(int c, struct tty *tp);
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
 * CRC computation
 */
static const u_int16_t fcstab[] = {
	0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
	0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
	0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
	0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
	0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
	0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
	0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
	0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
	0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
	0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
	0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
	0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
	0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
	0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
	0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
	0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
	0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
	0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
	0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
	0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
	0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
	0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
	0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
	0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
	0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
	0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
	0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
	0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
	0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
	0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
	0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
	0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

#define INITFCS 0xffff
#define GOODFCS 0xf0b8

static __inline u_int16_t updateFCS(u_int16_t fcs, int c) {
	return (fcs >> 8) ^ fcstab[(fcs^c) & 0xff];
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
#if 0
		error = irframeioctl(sc, cmd, data, flag, p);
#else
		error = EINVAL;
#endif
		break;
	}

	return (error);
}

/*
 * Start output on async tty interface.  If the transmit queue
 * has drained sufficiently, arrange for irframeasyncstart to be
 * called later at splsoftnet.
 * Called at spltty or higher.
 */
int
irframetstart(struct tty *tp)
{
	/*struct irframet_softc *sc = (struct irframet_softc *)tp->t_sc;*/

	DPRINTF(("%s: tp=%p\n", __FUNCTION__, tp));

	/*
	 * If there is stuff in the output queue, send it now.
	 * We are being called in lieu of ttstart and must do what it would.
	 */
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
irt_putesc(int c, struct tty *tp)
{
	int error;

	if (c == SIR_BOF || c == SIR_EOF || c == SIR_CE) {
		error = irt_putc(SIR_CE, tp);
		if (!error)
			error = irt_putc(SIR_ESC_BIT^c, tp);
	} else {
		error = irt_putc(c, tp);
	}
	return (error);
}

int
irframet_write(void *h, struct uio *uio, int flag)
{
	u_int8_t buf[MAX_IRDA_FRAME];
	size_t n;
	int error;

	DPRINTF(("%s: resid=%d, iovcnt=%d, offset=%ld\n", 
		 __FUNCTION__, uio->uio_resid, uio->uio_iovcnt, 
		 (long)uio->uio_offset));

	n = uio->uio_resid;
	if (n > MAX_IRDA_FRAME)
		return (EINVAL);
	error = uiomove(buf, n, uio);
	if (error)
		return (error);
	return (irt_write_buf(h, buf, n));
}

int
irt_write_buf(void *h, void *buf, size_t len)
{
	struct tty *tp = h;
	struct irframet_softc *sc = (struct irframet_softc *)tp->t_sc;
	u_int8_t *cp = buf;
	int c, error, ofcs, i;

	DPRINTF(("%s: tp=%p len=%d\n", __FUNCTION__, tp, len));

	ofcs = INITFCS;
	error = 0;

	for (i = 0; i < sc->sc_ebofs; i++)
		irt_putc(SIR_EXTRA_BOF, tp);
	irt_putc(SIR_BOF, tp);

	while (len-- > 0) {
		c = *cp++;
		ofcs = updateFCS(ofcs, c);
		error = irt_putesc(c, tp);
		if (error)
			return (error);
	}

	ofcs = ~ofcs;
	error = irt_putesc(ofcs & 0xff, tp);
	if (!error)
		error = irt_putesc((ofcs >> 8) & 0xff, tp);
	if (!error)
		error = irt_putc(SIR_EOF, tp);

	irframetstart(tp);

	DPRINTF(("%s: done\n", __FUNCTION__));

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
	/* XXX should check with tty */
	if (events & (POLLOUT | POLLWRNORM))
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
