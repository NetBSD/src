/*	$NetBSD: epcom.c,v 1.12 2006/10/01 18:56:21 elad Exp $ */
/*
 * Copyright (c) 1998, 1999, 2001, 2002, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jesse Off
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ichiro FUKUHARA and Naoto Shimazaki.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 * Copyright (c) 1991 The Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
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
 *      @(#)com.c       7.5 (Berkeley) 5/16/91
 */

/*
 * TODO: hardware flow control
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: epcom.c,v 1.12 2006/10/01 18:56:21 elad Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "epcom.h"

#include "rnd.h"
#if NRND > 0 && defined(RND_COM)
#include <sys/rnd.h>
#endif

/*
 * Override cnmagic(9) macro before including <sys/systm.h>.
 * We need to know if cn_check_magic triggered debugger, so set a flag.
 * Callers of cn_check_magic must declare int cn_trapped = 0;
 * XXX: this is *ugly*!
 */
#define cn_trap()				\
	do {					\
		console_debugger();		\
		cn_trapped = 1;			\
	} while (/* CONSTCOND */ 0)


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/tty.h>
#include <sys/uio.h>
#include <sys/vnode.h>
#include <sys/kauth.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <arm/ep93xx/epcomreg.h>
#include <arm/ep93xx/epcomvar.h>
#include <arm/ep93xx/ep93xxreg.h>
#include <arm/ep93xx/ep93xxvar.h>

#include <dev/cons.h>

static int	epcomparam(struct tty *, struct termios *);
static void	epcomstart(struct tty *);
static int	epcomhwiflow(struct tty *, int);

static u_int	cflag2lcrhi(tcflag_t);
static void	epcom_iflush(struct epcom_softc *);
static void	epcom_set(struct epcom_softc *);

int             epcomcngetc(dev_t);
void            epcomcnputc(dev_t, int);
void            epcomcnpollc(dev_t, int);

static void	epcomsoft(void* arg);
inline static void	epcom_txsoft(struct epcom_softc *, struct tty *);
inline static void	epcom_rxsoft(struct epcom_softc *, struct tty *);

void            epcomcnprobe(struct consdev *);
void            epcomcninit(struct consdev *);

static struct epcom_cons_softc {
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_addr_t		sc_hwbase;
	int			sc_ospeed;
	tcflag_t		sc_cflag;
	int			sc_attached;
} epcomcn_sc;

static struct cnm_state epcom_cnm_state;

extern struct cfdriver epcom_cd;

dev_type_open(epcomopen);
dev_type_close(epcomclose);
dev_type_read(epcomread);
dev_type_write(epcomwrite);
dev_type_ioctl(epcomioctl);
dev_type_stop(epcomstop);
dev_type_tty(epcomtty);
dev_type_poll(epcompoll);

const struct cdevsw epcom_cdevsw = {
	epcomopen, epcomclose, epcomread, epcomwrite, epcomioctl,
	epcomstop, epcomtty, epcompoll, nommap, ttykqfilter, D_TTY
};

struct consdev epcomcons = {
	NULL, NULL, epcomcngetc, epcomcnputc, epcomcnpollc, NULL,
	NULL, NULL, NODEV, CN_NORMAL
};

#ifndef DEFAULT_COMSPEED
#define DEFAULT_COMSPEED 115200
#endif

#define COMUNIT_MASK    0x7ffff
#define COMDIALOUT_MASK 0x80000

#define COMUNIT(x)	(minor(x) & COMUNIT_MASK)
#define COMDIALOUT(x)	(minor(x) & COMDIALOUT_MASK)

#define COM_ISALIVE(sc)	((sc)->enabled != 0 && \
			device_is_active(&(sc)->sc_dev))

void
epcom_attach_subr(struct epcom_softc *sc)
{
	struct tty *tp;

	if (sc->sc_iot == epcomcn_sc.sc_iot
	    && sc->sc_hwbase == epcomcn_sc.sc_hwbase) {
		epcomcn_sc.sc_attached = 1;
		sc->sc_lcrlo = EPCOMSPEED2BRD(epcomcn_sc.sc_ospeed) & 0xff;
		sc->sc_lcrmid = EPCOMSPEED2BRD(epcomcn_sc.sc_ospeed) >> 8;

		/* Make sure the console is always "hardwired". */
		delay(10000);	/* wait for output to finish */
		SET(sc->sc_hwflags, COM_HW_CONSOLE);
		SET(sc->sc_swflags, TIOCFLAG_SOFTCAR);
	}

	tp = ttymalloc();
	tp->t_oproc = epcomstart;
	tp->t_param = epcomparam;
	tp->t_hwiflow = epcomhwiflow;

	sc->sc_tty = tp;
	sc->sc_rbuf = malloc(EPCOM_RING_SIZE << 1, M_DEVBUF, M_NOWAIT);
	sc->sc_rbput = sc->sc_rbget = sc->sc_rbuf;
	sc->sc_rbavail = EPCOM_RING_SIZE;
	if (sc->sc_rbuf == NULL) {
		printf("%s: unable to allocate ring buffer\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	sc->sc_ebuf = sc->sc_rbuf + (EPCOM_RING_SIZE << 1);
	sc->sc_tbc = 0;

	sc->sc_lcrlo = EPCOMSPEED2BRD(DEFAULT_COMSPEED) & 0xff;
	sc->sc_lcrmid = EPCOMSPEED2BRD(DEFAULT_COMSPEED) >> 8;
	sc->sc_lcrhi = cflag2lcrhi(CS8); /* 8N1 */

	tty_attach(tp);

	if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
		int maj;

		/* locate the major number */
		maj = cdevsw_lookup_major(&epcom_cdevsw);

		cn_tab->cn_dev = makedev(maj, device_unit(&sc->sc_dev));

		aprint_normal("%s: console\n", sc->sc_dev.dv_xname);
	}

	sc->sc_si = softintr_establish(IPL_SOFTSERIAL, epcomsoft, sc);

#if NRND > 0 && defined(RND_COM)
	rnd_attach_source(&sc->rnd_source, sc->sc_dev.dv_xname,
			  RND_TYPE_TTY, 0);
#endif

	/* if there are no enable/disable functions, assume the device
	   is always enabled */
	if (!sc->enable)
		sc->enabled = 1;

	/* XXX configure register */
	/* xxx_config(sc) */

	SET(sc->sc_hwflags, COM_HW_DEV_OK);
}

static int
epcomparam(struct tty *tp, struct termios *t)
{
	struct epcom_softc *sc
		= device_lookup(&epcom_cd, COMUNIT(tp->t_dev));
	int s;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);

	if (t->c_ispeed && t->c_ispeed != t->c_ospeed)
		return (EINVAL);

	/*
	 * For the console, always force CLOCAL and !HUPCL, so that the port
	 * is always active.
	 */
	if (ISSET(sc->sc_swflags, TIOCFLAG_SOFTCAR) ||
	    ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
		SET(t->c_cflag, CLOCAL);
		CLR(t->c_cflag, HUPCL);
	}

	/*
	 * If there were no changes, don't do anything.  This avoids dropping
	 * input and improves performance when all we did was frob things like
	 * VMIN and VTIME.
	 */
	if (tp->t_ospeed == t->c_ospeed &&
	    tp->t_cflag == t->c_cflag)
		return (0);

	s = splserial();

	sc->sc_lcrhi = cflag2lcrhi(t->c_cflag);
	sc->sc_lcrlo = EPCOMSPEED2BRD(t->c_ospeed) & 0xff;
	sc->sc_lcrmid = EPCOMSPEED2BRD(t->c_ospeed) >> 8;
	
	/* And copy to tty. */
	tp->t_ispeed = 0;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;
	epcom_set(sc);

	splx(s);

	/*
	 * Update the tty layer's idea of the carrier bit.
	 * We tell tty the carrier is always on.
	 */
	(void) (*tp->t_linesw->l_modem)(tp, 1);

#ifdef COM_DEBUG
	if (com_debug)
		comstatus(sc, "comparam ");
#endif

	if (!ISSET(t->c_cflag, CHWFLOW)) {
		if (sc->sc_tx_stopped) {
			sc->sc_tx_stopped = 0;
			epcomstart(tp);
		}
	}

	return (0);
}

static int
epcomhwiflow(struct tty *tp, int block)
{
	return (0);
}

static void
epcom_filltx(struct epcom_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int n;

	n = 0;
        while ((bus_space_read_4(iot, ioh, EPCOM_Flag) & Flag_TXFF) == 0) {
		if (n >= sc->sc_tbc)
			break;
		bus_space_write_4(iot, ioh, EPCOM_Data,
				  0xff & *(sc->sc_tba + n));
		n++;
        }
        sc->sc_tbc -= n;
        sc->sc_tba += n;
}

static void
epcomstart(struct tty *tp)
{
	struct epcom_softc *sc
		= device_lookup(&epcom_cd, COMUNIT(tp->t_dev));
	int s;

	if (COM_ISALIVE(sc) == 0)
		return;

	s = spltty();
	if (ISSET(tp->t_state, TS_BUSY | TS_TIMEOUT | TS_TTSTOP))
		goto out;
	if (sc->sc_tx_stopped)
		goto out;

	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (ISSET(tp->t_state, TS_ASLEEP)) {
			CLR(tp->t_state, TS_ASLEEP);
			wakeup(&tp->t_outq);
		}
		selwakeup(&tp->t_wsel);
		if (tp->t_outq.c_cc == 0)
			goto out;
	}

	/* Grab the first contiguous region of buffer space. */
	{
		u_char *tba;
		int tbc;

		tba = tp->t_outq.c_cf;
		tbc = ndqb(&tp->t_outq, 0);

		(void)splserial();

		sc->sc_tba = tba;
		sc->sc_tbc = tbc;
	}

	SET(tp->t_state, TS_BUSY);
	sc->sc_tx_busy = 1;

	/* Output the first chunk of the contiguous buffer. */
	epcom_filltx(sc);

	if (!ISSET(sc->sc_ctrl, Ctrl_TIE)) {
		SET(sc->sc_ctrl, Ctrl_TIE);
		epcom_set(sc);
	}

out:
	splx(s);
	return;
}

static void
epcom_break(struct epcom_softc *sc, int onoff)
{
	if (onoff)
		SET(sc->sc_lcrhi, LinCtrlHigh_BRK);
	else
		CLR(sc->sc_lcrhi, LinCtrlHigh_BRK);
	epcom_set(sc);
}

static void
epcom_shutdown(struct epcom_softc *sc)
{
	int s;

	s = splserial();

	/* Turn off interrupts. */
	CLR(sc->sc_ctrl, (Ctrl_TIE|Ctrl_RTIE|Ctrl_RIE));

	/* Clear any break condition set with TIOCSBRK. */
	epcom_break(sc, 0);
	epcom_set(sc);

	if (sc->disable) {
#ifdef DIAGNOSTIC
		if (!sc->enabled)
			panic("epcom_shutdown: not enabled?");
#endif
		(*sc->disable)(sc);
		sc->enabled = 0;
	}
	splx(s);
}

int
epcomopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct epcom_softc *sc;
	struct tty *tp;
	int s, s2;
	int error;

	sc = device_lookup(&epcom_cd, COMUNIT(dev));
	if (sc == NULL || !ISSET(sc->sc_hwflags, COM_HW_DEV_OK) ||
		sc->sc_rbuf == NULL)
		return (ENXIO);

	if (!device_is_active(&sc->sc_dev))
		return (ENXIO);

#ifdef KGDB
	/*
	 * If this is the kgdb port, no other use is permitted.
	 */
	if (ISSET(sc->sc_hwflags, COM_HW_KGDB))
		return (EBUSY);
#endif

	tp = sc->sc_tty;

	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp))
		return (EBUSY);

	s = spltty();

	/*
	 * Do the following iff this is a first open.
	 */
	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		struct termios t;

		tp->t_dev = dev;

		s2 = splserial();

		if (sc->enable) {
			if ((*sc->enable)(sc)) {
				splx(s2);
				splx(s);
				printf("%s: device enable failed\n",
				       sc->sc_dev.dv_xname);
				return (EIO);
			}
			sc->enabled = 1;
#if 0
/* XXXXXXXXXXXXXXX */
			com_config(sc);
#endif
		}

		/* Turn on interrupts. */
		SET(sc->sc_ctrl, (Ctrl_UARTE|Ctrl_RIE|Ctrl_RTIE));
		epcom_set(sc);

#if 0
		/* Fetch the current modem control status, needed later. */
		sc->sc_msr = bus_space_read_1(sc->sc_iot, sc->sc_ioh, com_msr);

		/* Clear PPS capture state on first open. */
		sc->sc_ppsmask = 0;
		sc->ppsparam.mode = 0;
#endif

		splx(s2);

		/*
		 * Initialize the termios status to the defaults.  Add in the
		 * sticky bits from TIOCSFLAGS.
		 */
		t.c_ispeed = 0;
		if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
			t.c_ospeed = epcomcn_sc.sc_ospeed;
			t.c_cflag = epcomcn_sc.sc_cflag;
		} else {
			t.c_ospeed = TTYDEF_SPEED;
			t.c_cflag = TTYDEF_CFLAG;
		}
		if (ISSET(sc->sc_swflags, TIOCFLAG_CLOCAL))
			SET(t.c_cflag, CLOCAL);
		if (ISSET(sc->sc_swflags, TIOCFLAG_CRTSCTS))
			SET(t.c_cflag, CRTSCTS);
		if (ISSET(sc->sc_swflags, TIOCFLAG_MDMBUF))
			SET(t.c_cflag, MDMBUF);
		/* Make sure epcomparam() will do something. */
		tp->t_ospeed = 0;
		(void) epcomparam(tp, &t);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		ttychars(tp);
		ttsetwater(tp);

		s2 = splserial();

		/* Clear the input ring, and unblock. */
		sc->sc_rbput = sc->sc_rbget = sc->sc_rbuf;
		sc->sc_rbavail = EPCOM_RING_SIZE;
		epcom_iflush(sc);
		CLR(sc->sc_rx_flags, RX_ANY_BLOCK);

#ifdef COM_DEBUG
		if (epcom_debug)
			comstatus(sc, "epcomopen  ");
#endif

		splx(s2);
	}
	
	splx(s);

	error = ttyopen(tp, COMDIALOUT(dev), ISSET(flag, O_NONBLOCK));
	if (error)
		goto bad;

	error = (*tp->t_linesw->l_open)(dev, tp);
	if (error)
		goto bad;

	return (0);

bad:
	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		/*
		 * We failed to open the device, and nobody else had it opened.
		 * Clean up the state as appropriate.
		 */
		epcom_shutdown(sc);
	}

	return (error);
}

int
epcomclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct epcom_softc *sc = device_lookup(&epcom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	/* XXX This is for cons.c. */
	if (!ISSET(tp->t_state, TS_ISOPEN))
		return (0);

	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);

	if (COM_ISALIVE(sc) == 0)
		return (0);

	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		/*
		 * Although we got a last close, the device may still be in
		 * use; e.g. if this was the dialout node, and there are still
		 * processes waiting for carrier on the non-dialout node.
		 */
		epcom_shutdown(sc);
	}

	return (0);
}

int
epcomread(dev_t dev, struct uio *uio, int flag)
{
	struct epcom_softc *sc = device_lookup(&epcom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);
 
	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}

int
epcomwrite(dev_t dev, struct uio *uio, int flag)
{
	struct epcom_softc *sc = device_lookup(&epcom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);
 
	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
epcompoll(dev_t dev, int events, struct lwp *l)
{
	struct epcom_softc *sc = device_lookup(&epcom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);
 
	return ((*tp->t_linesw->l_poll)(tp, events, l));
}

struct tty *
epcomtty(dev_t dev)
{
	struct epcom_softc *sc = device_lookup(&epcom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	return (tp);
}

int
epcomioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct lwp *l)
{
	struct epcom_softc *sc = device_lookup(&epcom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;
	int error;
	int s;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return (error);

	error = ttioctl(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return (error);

	error = 0;

	s = splserial();

	switch (cmd) {
	case TIOCSBRK:
		epcom_break(sc, 1);
		break;

	case TIOCCBRK:
		epcom_break(sc, 0);
		break;

	case TIOCGFLAGS:
		*(int *)data = sc->sc_swflags;
		break;

	case TIOCSFLAGS:
		error = kauth_authorize_generic(l->l_cred,
		    KAUTH_GENERIC_ISSUSER, &l->l_acflag); 
		if (error)
			break;
		sc->sc_swflags = *(int *)data;
		break;

	default:
		error = EPASSTHROUGH;
		break;
	}

	splx(s);

	return (error);
}

/*
 * Stop output on a line.
 */
void
epcomstop(struct tty *tp, int flag)
{
	struct epcom_softc *sc
		= device_lookup(&epcom_cd, COMUNIT(tp->t_dev));
	int s;

	s = splserial();
	if (ISSET(tp->t_state, TS_BUSY)) {
		/* Stop transmitting at the next chunk. */
		sc->sc_tbc = 0;
		if (!ISSET(tp->t_state, TS_TTSTOP))
			SET(tp->t_state, TS_FLUSH);
	}
	splx(s);
}

static u_int
cflag2lcrhi(tcflag_t cflag)
{
	u_int lcrhi;

	switch (cflag & CSIZE) {
	case CS7:
		lcrhi = 0x40;
		break;
	case CS6:
		lcrhi = 0x20;
		break;
	case CS8:
	default:
		lcrhi = 0x60;
		break;
	}
	lcrhi |= (cflag & PARENB) ? LinCtrlHigh_PEN : 0;
	lcrhi |= (cflag & PARODD) ? 0 : LinCtrlHigh_EPS;
	lcrhi |= (cflag & CSTOPB) ? LinCtrlHigh_STP2 : 0;
	lcrhi |= LinCtrlHigh_FEN;  /* FIFO always enabled */
	
	return (lcrhi);
}

static void
epcom_iflush(struct epcom_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
#ifdef DIAGNOSTIC
	int reg;
#endif
	int timo;

#ifdef DIAGNOSTIC
	reg = 0xffff;
#endif
	timo = 50000;
	/* flush any pending I/O */
	while ((bus_space_read_4(iot, ioh, EPCOM_Flag) & Flag_RXFE) == 0
	       && --timo)
#ifdef DIAGNOSTIC
		reg =
#else
			(void)
#endif
			bus_space_read_4(iot, ioh, EPCOM_Data);
#ifdef DIAGNOSTIC
	if (!timo)
		printf("%s: com_iflush timeout %02x\n", sc->sc_dev.dv_xname,
		       reg);
#endif
}

static void
epcom_set(struct epcom_softc *sc)
{
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, EPCOM_LinCtrlLow,
			  sc->sc_lcrlo);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, EPCOM_LinCtrlMid,
			  sc->sc_lcrmid);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, EPCOM_LinCtrlHigh,
			  sc->sc_lcrhi);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, EPCOM_Ctrl,
			  sc->sc_ctrl);
}

int
epcomcnattach(bus_space_tag_t iot, bus_addr_t iobase, bus_space_handle_t ioh,
    int ospeed, tcflag_t cflag)
{
	u_int lcrlo, lcrmid, lcrhi, ctrl, pwrcnt;
	bus_space_handle_t syscon_ioh;

	cn_tab = &epcomcons;
	cn_init_magic(&epcom_cnm_state);
	cn_set_magic("\047\001");

	epcomcn_sc.sc_iot = iot;
	epcomcn_sc.sc_ioh = ioh;
	epcomcn_sc.sc_hwbase = iobase;
	epcomcn_sc.sc_ospeed = ospeed;
	epcomcn_sc.sc_cflag = cflag;

	lcrhi = cflag2lcrhi(cflag);
	lcrlo = EPCOMSPEED2BRD(ospeed) & 0xff;
	lcrmid = EPCOMSPEED2BRD(ospeed) >> 8;
	ctrl = Ctrl_UARTE;

	bus_space_map(iot, EP93XX_APB_HWBASE + EP93XX_APB_SYSCON,
		EP93XX_APB_SYSCON_SIZE, 0, &syscon_ioh);
	pwrcnt = bus_space_read_4(iot, syscon_ioh, EP93XX_SYSCON_PwrCnt);
	pwrcnt &= ~(PwrCnt_UARTBAUD);
	bus_space_write_4(iot, syscon_ioh, EP93XX_SYSCON_PwrCnt, pwrcnt);
	bus_space_unmap(iot, syscon_ioh, EP93XX_APB_SYSCON_SIZE);

	bus_space_write_4(iot, ioh, EPCOM_LinCtrlLow, lcrlo);
	bus_space_write_4(iot, ioh, EPCOM_LinCtrlMid, lcrmid);
	bus_space_write_4(iot, ioh, EPCOM_LinCtrlHigh, lcrhi);
	bus_space_write_4(iot, ioh, EPCOM_Ctrl, ctrl);

	return (0);
}

void
epcomcnprobe(struct consdev *cp)
{
	cp->cn_pri = CN_REMOTE;
}

void
epcomcnpollc(dev_t dev, int on)
{
}

void
epcomcnputc(dev_t dev, int c)
{
	int			s;
	bus_space_tag_t		iot = epcomcn_sc.sc_iot;
	bus_space_handle_t	ioh = epcomcn_sc.sc_ioh;

	s = splserial();

	while((bus_space_read_4(iot, ioh, EPCOM_Flag) & Flag_TXFF) != 0)
		;

	bus_space_write_4(iot, ioh, EPCOM_Data, c);

#ifdef DEBUG
	if (c == '\r') {
		while((bus_space_read_4(iot, ioh, EPCOM_Flag) & Flag_TXFE) == 0)
			;
	}
#endif

	splx(s);
}

int
epcomcngetc(dev_t dev)
{
	int			c, sts;
	int			s;
	bus_space_tag_t		iot = epcomcn_sc.sc_iot;
	bus_space_handle_t	ioh = epcomcn_sc.sc_ioh;

        s = splserial();

	while((bus_space_read_4(iot, ioh, EPCOM_Flag) & Flag_RXFE) != 0)
		;

	c = bus_space_read_4(iot, ioh, EPCOM_Data);
	sts = bus_space_read_4(iot, ioh, EPCOM_RXSts);
	if (ISSET(sts, RXSts_BE)) c = CNC_BREAK;
#ifdef DDB
	extern int db_active;
	if (!db_active)
#endif
	{
		int cn_trapped = 0; /* unused */

		cn_check_magic(dev, c, epcom_cnm_state);
	}
	c &= 0xff;
	splx(s);

	return (c);
}

inline static void
epcom_txsoft(struct epcom_softc *sc, struct tty *tp)
{
	CLR(tp->t_state, TS_BUSY);
	if (ISSET(tp->t_state, TS_FLUSH))
		CLR(tp->t_state, TS_FLUSH);
        else
		ndflush(&tp->t_outq, (int)(sc->sc_tba - tp->t_outq.c_cf));
	(*tp->t_linesw->l_start)(tp);
}

inline static void
epcom_rxsoft(struct epcom_softc *sc, struct tty *tp)
{
	int (*rint) __P((int, struct tty *)) = tp->t_linesw->l_rint;
	u_char *get, *end;
	u_int cc, scc;
	u_char sts;
	int code;
	int s;

	end = sc->sc_ebuf;
	get = sc->sc_rbget;
	scc = cc = EPCOM_RING_SIZE - sc->sc_rbavail;
#if 0
	if (cc == EPCOM_RING_SIZE) {
		sc->sc_floods++;
		if (sc->sc_errors++ == 0)
			callout_reset(&sc->sc_diag_callout, 60 * hz,
			    comdiag, sc);
	}
#endif
	while (cc) {
		code = get[0];
		sts = get[1];
		if (ISSET(sts, RXSts_OE | RXSts_FE | RXSts_PE | RXSts_BE)) {
#if 0
			if (ISSET(lsr, DR_ROR)) {
				sc->sc_overflows++;
				if (sc->sc_errors++ == 0)
					callout_reset(&sc->sc_diag_callout,
					    60 * hz, comdiag, sc);
			}
#endif
			if (ISSET(sts, (RXSts_FE|RXSts_BE)))
				SET(code, TTY_FE);
			if (ISSET(sts, RXSts_PE))
				SET(code, TTY_PE);
		}
		if ((*rint)(code, tp) == -1) {
			/*
			 * The line discipline's buffer is out of space.
			 */
			if (!ISSET(sc->sc_rx_flags, RX_TTY_BLOCKED)) {
				/*
				 * We're either not using flow control, or the
				 * line discipline didn't tell us to block for
				 * some reason.  Either way, we have no way to
				 * know when there's more space available, so
				 * just drop the rest of the data.
				 */
				get += cc << 1;
				if (get >= end)
					get -= EPCOM_RING_SIZE << 1;
				cc = 0;
			} else {
				/*
				 * Don't schedule any more receive processing
				 * until the line discipline tells us there's
				 * space available (through comhwiflow()).
				 * Leave the rest of the data in the input
				 * buffer.
				 */
				SET(sc->sc_rx_flags, RX_TTY_OVERFLOWED);
			}
			break;
		}
		get += 2;
		if (get >= end)
			get = sc->sc_rbuf;
		cc--;
	}

	if (cc != scc) {
		sc->sc_rbget = get;
		s = splserial();
		
		cc = sc->sc_rbavail += scc - cc;
		/* Buffers should be ok again, release possible block. */
		if (cc >= 1) {
			if (ISSET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED)) {
				CLR(sc->sc_rx_flags, RX_IBUF_OVERFLOWED);
				SET(sc->sc_ctrl, (Ctrl_RIE|Ctrl_RTIE));
				epcom_set(sc);
			}
			if (ISSET(sc->sc_rx_flags, RX_IBUF_BLOCKED)) {
				CLR(sc->sc_rx_flags, RX_IBUF_BLOCKED);
#if 0
				com_hwiflow(sc);
#endif
			}
		}
		splx(s);
	}
}

static void
epcomsoft(void* arg)
{
	struct epcom_softc *sc = arg;

	if (COM_ISALIVE(sc) == 0)
		return;

	if (sc->sc_rx_ready) {
		sc->sc_rx_ready = 0;
		epcom_rxsoft(sc, sc->sc_tty);
	}
	if (sc->sc_tx_done) {
		sc->sc_tx_done = 0;
		epcom_txsoft(sc, sc->sc_tty);
	}
}

int
epcomintr(void* arg)
{
	struct epcom_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_char *put, *end;
	u_int cc;
	u_int flagr;
	u_int intr;
	u_int32_t c, csts;

	intr = bus_space_read_4(iot, ioh, EPCOM_IntIDIntClr);

	if (COM_ISALIVE(sc) == 0) 
		panic("intr on disabled epcom");

	flagr = bus_space_read_4(iot, ioh, EPCOM_Flag);

	end = sc->sc_ebuf;
	put = sc->sc_rbput;
	cc = sc->sc_rbavail;

	if (!(ISSET(flagr, Flag_RXFE))) {
		if (!ISSET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED)) {
			while (cc > 0) {
				if (ISSET(flagr, Flag_RXFE))
					break;
				c = bus_space_read_4(iot, ioh, EPCOM_Data);
				csts = bus_space_read_4(iot, ioh, EPCOM_RXSts);
				if (ISSET(csts, RXSts_BE)) {
					int cn_trapped = 0;

					cn_check_magic(sc->sc_tty->t_dev,
					  CNC_BREAK, epcom_cnm_state);
					if (cn_trapped)
						goto next;
#if defined(KGDB) && !defined(DDB)
					if (ISSET(sc->sc_hwflags, COM_HW_KGDB)){
						kgdb_connect(1);
						goto next;
					}
#endif
				} else {
					int cn_trapped = 0;

					cn_check_magic(sc->sc_tty->t_dev,
					  (c & 0xff), epcom_cnm_state);
					if (cn_trapped)
						goto next;
				}


				put[0] = c & 0xff;
				put[1] = csts & 0xf;
				put += 2;
				if (put >= end)
					put = sc->sc_rbuf;
				cc--;
			next:
				flagr = bus_space_read_4(iot, ioh, EPCOM_Flag);
			}

			/*
			 * Current string of incoming characters ended because
			 * no more data was available or we ran out of space.
			 * Schedule a receive event if any data was received.
			 * If we're out of space, turn off receive interrupts.
			 */
			sc->sc_rbput = put;
			sc->sc_rbavail = cc;
			if (!ISSET(sc->sc_rx_flags, RX_TTY_OVERFLOWED))
				sc->sc_rx_ready = 1;

			/*
			 * See if we are in danger of overflowing a buffer. If
			 * so, use hardware flow control to ease the pressure.
			 */

			/* but epcom cannot. X-( */

			/*
			 * If we're out of space, disable receive interrupts
			 * until the queue has drained a bit.
			 */
			if (!cc) {
				SET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED);
				CLR(sc->sc_ctrl, (Ctrl_RIE|Ctrl_RTIE));
				epcom_set(sc);
			}
		} else {
#ifdef DIAGNOSTIC
			panic("epcomintr: we shouldn't reach here");
#endif
			CLR(sc->sc_ctrl, (Ctrl_RIE|Ctrl_RTIE));
			epcom_set(sc);
		}
	}

	/*
	 * Done handling any receive interrupts. See if data can be
	 * transmitted as well. Schedule tx done event if no data left
	 * and tty was marked busy.
	 */

	if (!ISSET(flagr, Flag_TXFF) && sc->sc_tbc > 0) {
		/* Output the next chunk of the contiguous buffer, if any. */
		epcom_filltx(sc);
	} else {
		/* Disable transmit completion interrupts if necessary. */
		if (ISSET(sc->sc_ctrl, Ctrl_TIE)) {
			CLR(sc->sc_ctrl, Ctrl_TIE);
			epcom_set(sc);
		}
		if (sc->sc_tx_busy) {
			sc->sc_tx_busy = 0;
			sc->sc_tx_done = 1;
		}
	}

	/* Wake up the poller. */
	softintr_schedule(sc->sc_si);

#if 0 /* XXX: broken */
#if NRND > 0 && defined(RND_COM)
	rnd_add_uint32(&sc->rnd_source, intr ^ flagr);
#endif
#endif
	return (1);
}
