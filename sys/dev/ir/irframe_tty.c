/*	$NetBSD: irframe_tty.c,v 1.1 2001/12/03 23:32:32 augustss Exp $	*/

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net).
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
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/poll.h>

#include <dev/ir/ir.h>
#include <dev/ir/irdaio.h>
#include <dev/ir/irframevar.h>

/* Macros to clear/set/test flags. */
#define	SET(t, f)	(t) |= (f)
#define	CLR(t, f)	(t) &= ~((unsigned)(f))
#define	ISSET(t, f)	((t) & (f))

#ifdef IRFRAMET_DEBUG
#define DPRINTF(x)	if (irframetdebug) printf x
int irframetdebug = 1;
#else
#define DPRINTF(x)
#endif

/* Protocol constants */
#define SIR_EXTRA_BOF            0xff
#define SIR_BOF                  0xc0
#define SIR_CE                   0x7d
#define SIR_EOF                  0xc1

#define SIR_ESC_BIT              0x20
/*****/

#define MAX_IRDA_FRAME 5000	/* XXX what is it? */

struct irframet_softc {
	struct irframe_softc sc_irp;
	struct tty *sc_tp;
	int sc_ebofs;
};

/* line discipline methods */
int	irframetopen(dev_t dev, struct tty *tp);
int	irframetclose(struct tty *tp, int flag);
int	irframetioctl(struct tty *tp, u_long cmd, caddr_t data, int flag,
		     struct proc *);
int	irframetinput(int c, struct tty *tp);
int	irframetstart(struct tty *tp);

/* irframe methods */
int	irframet_open(void *h, int flag, int mode, struct proc *p);
int	irframet_close(void *h, int flag, int mode, struct proc *p);
int	irframet_read(void *h, struct uio *uio, int flag);
int	irframet_write(void *h, struct uio *uio, int flag);
int	irframet_poll(void *h, int events, struct proc *p);
int	irframet_set_params(void *h, struct irda_params *params);
int	irframet_reset_params(void *h);
int	irframet_get_speeds(void *h, int *speeds);
int	irframet_get_turnarounds(void *h, int *times);

int	irframet_write_buf(void *h, void *buf, size_t len);

void	irframettyattach(int);

struct irframe_methods irframet_methods = {
	irframet_open, irframet_close, irframet_read, irframet_write,
	irframet_poll, irframet_set_params, irframet_reset_params,
	irframet_get_speeds, irframet_get_speeds
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

	DPRINTF(("%s: linesw=%p disc=%d\n", __FUNCTION__, tp->t_linesw,
		 tp->t_linesw->l_no));
	if (tp->t_linesw->l_no == IRFRAMEDISC) {
		sc = (struct irframet_softc *)tp->t_sc;
		DPRINTF(("%s: sc=%p sc_tp=%p\n", __FUNCTION__, sc, sc->sc_tp));
		if (sc != NULL) {
			splx(s);
			return (EBUSY);
		}
	}

	printf("%s attached at tty%d:", sc->sc_irp.sc_dev.dv_xname,
	    minor(tp->t_dev));
	tp->t_sc = irframe_alloc(sizeof (struct irframet_softc),
			&irframet_methods, tp);
	sc = (struct irframet_softc *)tp->t_sc;
	sc->sc_tp = tp;

	DPRINTF(("%s: set sc=%p\n", __FUNCTION__, sc));

	ttyflush(tp, FREAD | FWRITE);

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
		printf("%s detached from tty%d\n", sc->sc_irp.sc_dev.dv_xname,
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

int
irframetinput(int c, struct tty *tp)
{
	struct irframet_softc *sc = (struct irframet_softc *)tp->t_sc;

	DPRINTF(("%s: tp=%p c=0x%0x\n", __FUNCTION__, tp, c));

	if (sc == NULL || tp != (struct tty *)sc->sc_tp)
		return (0);
#if 0


    ++tk_nin;
    ++sc->sc_stats.irframe_ibytes;

    if (c & TTY_FE) {
	/* framing error or overrun on this char - abort packet */
	if (sc->sc_flags & SC_DEBUG)
	    printf("%s: bad char %x\n", sc->sc_if.if_xname, c);
	goto flush;
    }

    c &= 0xff;

    /*
     * Handle software flow control of output.
     */
    if (tp->t_iflag & IXON) {
	if (c == tp->t_cc[VSTOP] && tp->t_cc[VSTOP] != _POSIX_VDISABLE) {
	    if ((tp->t_state & TS_TTSTOP) == 0) {
		tp->t_state |= TS_TTSTOP;
		(*cdevsw[major(tp->t_dev)].d_stop)(tp, 0);
	    }
	    return 0;
	}
	if (c == tp->t_cc[VSTART] && tp->t_cc[VSTART] != _POSIX_VDISABLE) {
	    tp->t_state &= ~TS_TTSTOP;
	    if (tp->t_oproc != NULL)
		(*tp->t_oproc)(tp);
	    return 0;
	}
    }

    s = spltty();
    if (c & 0x80)
	sc->sc_flags |= SC_RCV_B7_1;
    else
	sc->sc_flags |= SC_RCV_B7_0;
    if (paritytab[c >> 5] & (1 << (c & 0x1F)))
	sc->sc_flags |= SC_RCV_ODDP;
    else
	sc->sc_flags |= SC_RCV_EVNP;
    splx(s);

    if (sc->sc_flags & SC_LOG_RAWIN)
	irframelogchar(sc, c);

    if (c == IRFRAME_FLAG) {
	ilen = sc->sc_ilen;
	sc->sc_ilen = 0;

	if (sc->sc_rawin_count > 0) 
	    irframelogchar(sc, -1);

	/*
	 * If SC_ESCAPED is set, then we've seen the packet
	 * abort sequence "}~".
	 */
	if (sc->sc_flags & (SC_FLUSH | SC_ESCAPED)
	    || (ilen > 0 && sc->sc_fcs != IRFRAME_GOODFCS)) {
	    s = spltty();
	    sc->sc_flags |= SC_PKTLOST;	/* note the dropped packet */
	    if ((sc->sc_flags & (SC_FLUSH | SC_ESCAPED)) == 0){
		if (sc->sc_flags & SC_DEBUG)
		    printf("%s: bad fcs %x\n", sc->sc_if.if_xname,
			sc->sc_fcs);
		sc->sc_if.if_ierrors++;
		sc->sc_stats.irframe_ierrors++;
	    } else
		sc->sc_flags &= ~(SC_FLUSH | SC_ESCAPED);
	    splx(s);
	    return 0;
	}

	if (ilen < IRFRAME_HDRLEN + IRFRAME_FCSLEN) {
	    if (ilen) {
		if (sc->sc_flags & SC_DEBUG)
		    printf("%s: too short (%d)\n", sc->sc_if.if_xname, ilen);
		s = spltty();
		sc->sc_if.if_ierrors++;
		sc->sc_stats.irframe_ierrors++;
		sc->sc_flags |= SC_PKTLOST;
		splx(s);
	    }
	    return 0;
	}

	/*
	 * Remove FCS trailer.  Somewhat painful...
	 */
	ilen -= 2;
	if (--sc->sc_mc->m_len == 0) {
	    for (m = sc->sc_m; m->m_next != sc->sc_mc; m = m->m_next)
		;
	    sc->sc_mc = m;
	}
	sc->sc_mc->m_len--;

	/* excise this mbuf chain */
	m = sc->sc_m;
	sc->sc_m = sc->sc_mc->m_next;
	sc->sc_mc->m_next = NULL;

	irframepktin(sc, m, sc->sc_flags & SC_PKTLOST);
	if (sc->sc_flags & SC_PKTLOST) {
	    s = spltty();
	    sc->sc_flags &= ~SC_PKTLOST;
	    splx(s);
	}

	irframegetm(sc);
	return 0;
    }

    if (sc->sc_flags & SC_FLUSH) {
	if (sc->sc_flags & SC_LOG_FLUSH)
	    irframelogchar(sc, c);
	return 0;
    }

    if (c < 0x20 && (sc->sc_rasyncmap & (1 << c)))
	return 0;

    s = spltty();
    if (sc->sc_flags & SC_ESCAPED) {
	sc->sc_flags &= ~SC_ESCAPED;
	c ^= IRFRAME_TRANS;
    } else if (c == IRFRAME_ESCAPE) {
	sc->sc_flags |= SC_ESCAPED;
	splx(s);
	return 0;
    }
    splx(s);

    /*
     * Initialize buffer on first octet received.
     * First octet could be address or protocol (when compressing
     * address/control).
     * Second octet is control.
     * Third octet is first or second (when compressing protocol)
     * octet of protocol.
     * Fourth octet is second octet of protocol.
     */
    if (sc->sc_ilen == 0) {
	/* reset the first input mbuf */
	if (sc->sc_m == NULL) {
	    irframegetm(sc);
	    if (sc->sc_m == NULL) {
		if (sc->sc_flags & SC_DEBUG)
		    printf("%s: no input mbufs!\n", sc->sc_if.if_xname);
		goto flush;
	    }
	}
	m = sc->sc_m;
	m->m_len = 0;
	m->m_data = M_DATASTART(sc->sc_m);
	sc->sc_mc = m;
	sc->sc_mp = mtod(m, char *);
	sc->sc_fcs = IRFRAME_INITFCS;
	if (c != IRFRAME_ALLSTATIONS) {
	    if (sc->sc_flags & SC_REJ_COMP_AC) {
		if (sc->sc_flags & SC_DEBUG)
		    printf("%s: garbage received: 0x%x (need 0xFF)\n",
		    sc->sc_if.if_xname, c);
		goto flush;
	    }
	    *sc->sc_mp++ = IRFRAME_ALLSTATIONS;
	    *sc->sc_mp++ = IRFRAME_UI;
	    sc->sc_ilen += 2;
	    m->m_len += 2;
	}
    }
    if (sc->sc_ilen == 1 && c != IRFRAME_UI) {
	if (sc->sc_flags & SC_DEBUG)
	    printf("%s: missing UI (0x3), got 0x%x\n",
		sc->sc_if.if_xname, c);
	goto flush;
    }
    if (sc->sc_ilen == 2 && (c & 1) == 1) {
	/* a compressed protocol */
	*sc->sc_mp++ = 0;
	sc->sc_ilen++;
	sc->sc_mc->m_len++;
    }
    if (sc->sc_ilen == 3 && (c & 1) == 0) {
	if (sc->sc_flags & SC_DEBUG)
	    printf("%s: bad protocol %x\n", sc->sc_if.if_xname,
		(sc->sc_mp[-1] << 8) + c);
	goto flush;
    }

    /* packet beyond configured mru? */
    if (++sc->sc_ilen > sc->sc_mru + IRFRAME_HDRLEN + IRFRAME_FCSLEN) {
	if (sc->sc_flags & SC_DEBUG)
	    printf("%s: packet too big\n", sc->sc_if.if_xname);
	goto flush;
    }

    /* is this mbuf full? */
    m = sc->sc_mc;
    if (M_TRAILINGSPACE(m) <= 0) {
	if (m->m_next == NULL) {
	    irframegetm(sc);
	    if (m->m_next == NULL) {
		if (sc->sc_flags & SC_DEBUG)
		    printf("%s: too few input mbufs!\n", sc->sc_if.if_xname);
		goto flush;
	    }
	}
	sc->sc_mc = m = m->m_next;
	m->m_len = 0;
	m->m_data = M_DATASTART(m);
	sc->sc_mp = mtod(m, char *);
    }

    ++m->m_len;
    *sc->sc_mp++ = c;
    sc->sc_fcs = IRFRAME_FCS(sc->sc_fcs, c);
    return 0;

 flush:
    if (!(sc->sc_flags & SC_FLUSH)) {
	s = spltty();
	sc->sc_if.if_ierrors++;
	sc->sc_stats.irframe_ierrors++;
	sc->sc_flags |= SC_FLUSH;
	splx(s);
	if (sc->sc_flags & SC_LOG_FLUSH)
	    irframelogchar(sc, c);
    }
#endif
    return 0;
}


/**********************************************************************
 * CRC
 **********************************************************************/

static const int fcstab[] = {
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

static const int INITFCS = 0xffff;
static const int GOODFCS = 0xf0b8;

static __inline int updateFCS(int fcs, int c) {
	return (fcs >> 8) ^ fcstab[(fcs^c) & 0xff];
} 

/*** irframe methods ***/

int
irframet_open(void *h, int flag, int mode, struct proc *p)
{
	struct tty *tp = h;

	tp = tp;
	DPRINTF(("%s: tp=%p\n", __FUNCTION__, tp));
	return (0);
}

int
irframet_close(void *h, int flag, int mode, struct proc *p)
{
	struct tty *tp = h;

	tp = tp;
	DPRINTF(("%s: tp=%p\n", __FUNCTION__, tp));
	return (0);
}

int
irframet_read(void *h, struct uio *uio, int flag)
{
	struct tty *tp = h;

	tp = tp;
	DPRINTF(("%s: tp=%p\n", __FUNCTION__, tp));
	return (0);
}

static int
putch(int c, struct tty *tp)
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
	if (putc(c, &tp->t_rawq) < 0) {
		printf("irframe: putc failed\n");
		return (EIO);
	}
	return (0);
}

static int
put_esc(int c, struct tty *tp)
{
	int error;

	if (c == SIR_BOF || c == SIR_EOF || c == SIR_CE) {
		error = putch(SIR_CE, tp);
		if (!error)
			error = putch(SIR_ESC_BIT^c, tp);
	} else {
		error = putch(c, tp);
	}
	return (error);
}

int
irframet_write(void *h, struct uio *uio, int flag)
{
	u_int8_t buf[MAX_IRDA_FRAME];
	size_t n;
	int error;

	DPRINTF(("%s\n", __FUNCTION__));

	n = uio->uio_resid;
	if (n > MAX_IRDA_FRAME)
		return (EINVAL);
	error = uiomove(buf, n, uio);
	if (error)
		return (error);
	return (irframet_write_buf(h, buf, n));
}

int
irframet_write_buf(void *h, void *buf, size_t len)
{
	struct tty *tp = h;
	struct irframet_softc *sc = (struct irframet_softc *)tp->t_sc;
	u_int8_t *cp = buf;
	int c, error, ofcs, i;

	DPRINTF(("%s: tp=%p len=%d\n", __FUNCTION__, tp, len));

	ofcs = INITFCS;
	error = 0;

	for (i = 0; i < sc->sc_ebofs; i++)
		putch(SIR_EXTRA_BOF, tp);
	putch(SIR_BOF, tp);

	while (len-- > 0) {
		c = *cp++;
		ofcs = updateFCS(ofcs, c);
		error = put_esc(c, tp);
		if (error)
			return (error);
	}

	ofcs = ~ofcs;
	error = put_esc(ofcs & 0xff, tp);
	if (!error)
		error = put_esc((ofcs >> 8) & 0xff, tp);
	if (!error)
		error = putch(SIR_EOF, tp);

	irframetstart(tp);

	return (error);
}

int
irframet_poll(void *h, int events, struct proc *p)
{
	struct oboe_softc *sc = h;
	int revents = 0;
	int s;

	DPRINTF(("%s: sc=%p\n", __FUNCTION__, sc));

	s = splir();
	if (events & (POLLOUT | POLLWRNORM))
		revents |= events & (POLLOUT | POLLWRNORM);
#if 0
	if (events & (POLLIN | POLLRDNORM)) {
		if (sc->sc_saved > 0) {
			DPRINTF(("%s: have data\n", __FUNCTION__));
			revents |= events & (POLLIN | POLLRDNORM);
		} else {
			DPRINTF(("%s: recording select", __FUNCTION__));
			selrecord(p, &sc->sc_rsel);
		}
	}
#endif
	splx(s);

	return (revents);
}

int
irframet_set_params(void *h, struct irda_params *p)
{
	struct tty *tp = h;
	struct irframet_softc *sc = (struct irframet_softc *)tp->t_sc;

	DPRINTF(("%s: tp=%p\n", __FUNCTION__, tp));

	/* XXX speed */
	sc->sc_ebofs = p->ebofs;
	/* XXX maxsize */

	return (0);
}

int
irframet_reset_params(void *h)
{
	struct tty *tp = h;
	struct irframet_softc *sc = (struct irframet_softc *)tp->t_sc;

	DPRINTF(("%s: tp=%p\n", __FUNCTION__, tp));

	sc->sc_ebofs = IRDA_DEFAULT_EBOFS;

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
