/*	$NetBSD: ixp12x0_com.c,v 1.15 2003/03/06 07:39:34 igy Exp $ */
/*
 * Copyright (c) 1998, 1999, 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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
 *      @(#)com.c       7.5 (Berkeley) 5/16/91
 */


#include "opt_ddb.h"
#include "opt_kgdb.h"

#include "rnd.h"
#if NRND > 0 && defined(RND_COM)
#include <sys/rnd.h>
#endif

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

#include <machine/intr.h>
#include <machine/bus.h>

#include <arm/ixp12x0/ixp12x0_comreg.h>
#include <arm/ixp12x0/ixp12x0_comvar.h>
#include <arm/ixp12x0/ixp12x0reg.h>
#include <arm/ixp12x0/ixp12x0var.h>

#include <arm/ixp12x0/ixpsipvar.h> 

#include <dev/cons.h>
#include "ixpcom.h"

static int	ixpcomparam(struct tty *, struct termios *);
static void	ixpcomstart(struct tty *);
static int	ixpcomhwiflow(struct tty *, int);

static u_int	cflag2cr(tcflag_t);
static void	ixpcom_iflush(struct ixpcom_softc *);
static void	ixpcom_set_cr(struct ixpcom_softc *);

int             ixpcomcngetc(dev_t);
void            ixpcomcnputc(dev_t, int);
void            ixpcomcnpollc(dev_t, int);

static void	ixpcomsoft(void* arg);
inline static void	ixpcom_txsoft(struct ixpcom_softc *, struct tty *);
inline static void	ixpcom_rxsoft(struct ixpcom_softc *, struct tty *);

void            ixpcomcnprobe(struct consdev *);
void            ixpcomcninit(struct consdev *);

u_int32_t	ixpcom_cr = 0;		/* tell cr to *_intr.c */
u_int32_t	ixpcom_imask = 0;	/* intrrupt mask from *_intr.c */


static struct ixpcom_cons_softc {
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_addr_t		sc_baseaddr;
	int			sc_ospeed;
	tcflag_t		sc_cflag;
	int			sc_attached;
} ixpcomcn_sc;

static struct cnm_state ixpcom_cnm_state;

struct ixpcom_softc* ixpcom_sc = NULL;

extern struct cfdriver ixpcom_cd;

dev_type_open(ixpcomopen);
dev_type_close(ixpcomclose);
dev_type_read(ixpcomread);
dev_type_write(ixpcomwrite);
dev_type_ioctl(ixpcomioctl);
dev_type_stop(ixpcomstop);
dev_type_tty(ixpcomtty);
dev_type_poll(ixpcompoll);

const struct cdevsw ixpcom_cdevsw = {
	ixpcomopen, ixpcomclose, ixpcomread, ixpcomwrite, ixpcomioctl,
	ixpcomstop, ixpcomtty, ixpcompoll, nommap, ttykqfilter, D_TTY
};

struct consdev ixpcomcons = {
	NULL, NULL, ixpcomcngetc, ixpcomcnputc, ixpcomcnpollc, NULL,
	NULL, NULL, NODEV, CN_NORMAL
};

#ifndef DEFAULT_COMSPEED
#define DEFAULT_COMSPEED 38400
#endif

#define COMUNIT_MASK    0x7ffff
#define COMDIALOUT_MASK 0x80000

#define COMUNIT(x)	(minor(x) & COMUNIT_MASK)
#define COMDIALOUT(x)	(minor(x) & COMDIALOUT_MASK)

#define COM_ISALIVE(sc)	((sc)->enabled != 0 && \
			ISSET((sc)->sc_dev.dv_flags, DVF_ACTIVE))

#define COM_BARRIER(t, h, f) bus_space_barrier((t), (h), 0, COM_NPORTS, (f))

#define	COM_LOCK(sc);
#define	COM_UNLOCK(sc);

#define SET(t, f)	(t) |= (f)
#define CLR(t, f)	(t) &= ~(f)
#define ISSET(t, f)	((t) & (f))

#define CFLAGS2CR_MASK	(CR_PE | CR_OES | CR_SBS | CR_DSS | CR_BRD)

void
ixpcom_attach_subr(sc)
	struct ixpcom_softc *sc;
{
	struct tty *tp;

	ixpcom_sc = sc;

	/* force to use ixpcom0 for console */
	if (sc->sc_iot == ixpcomcn_sc.sc_iot
	    && sc->sc_baseaddr == ixpcomcn_sc.sc_baseaddr) {
		ixpcomcn_sc.sc_attached = 1;
		sc->sc_speed = IXPCOMSPEED2BRD(ixpcomcn_sc.sc_ospeed);

		/* Make sure the console is always "hardwired". */
		/* XXX IXPCOM_SR should be checked  */
		delay(10000);	/* wait for output to finish */
		SET(sc->sc_hwflags, COM_HW_CONSOLE);
		SET(sc->sc_swflags, TIOCFLAG_SOFTCAR);
	}

	tp = ttymalloc();
	tp->t_oproc = ixpcomstart;
	tp->t_param = ixpcomparam;
	tp->t_hwiflow = ixpcomhwiflow;

	sc->sc_tty = tp;
	sc->sc_rbuf = malloc(IXPCOM_RING_SIZE << 1, M_DEVBUF, M_NOWAIT);
	sc->sc_rbput = sc->sc_rbget = sc->sc_rbuf;
	sc->sc_rbavail = IXPCOM_RING_SIZE;
	if (sc->sc_rbuf == NULL) {
		printf("%s: unable to allocate ring buffer\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	sc->sc_ebuf = sc->sc_rbuf + (IXPCOM_RING_SIZE << 1);
	sc->sc_tbc = 0;

	sc->sc_rie = sc->sc_xie = 0;
	ixpcom_cr = IXPCOMSPEED2BRD(DEFAULT_COMSPEED)
		| CR_UE | sc->sc_rie | sc->sc_xie | DSS_8BIT;

	tty_attach(tp);

	if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
		int maj;

		/* locate the major number */
		maj = cdevsw_lookup_major(&ixpcom_cdevsw);

		cn_tab->cn_dev = makedev(maj, sc->sc_dev.dv_unit);

		aprint_normal("%s: console\n", sc->sc_dev.dv_xname);
	}

	sc->sc_si = softintr_establish(IPL_SOFTSERIAL, ixpcomsoft, sc);

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
ixpcomparam(tp, t)
	struct tty *tp;
	struct termios *t;
{
	struct ixpcom_softc *sc
		= device_lookup(&ixpcom_cd, COMUNIT(tp->t_dev));
	u_int32_t cr;
	int s;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);

	cr = IXPCOMSPEED2BRD(t->c_ospeed);

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

	cr |= cflag2cr(t->c_cflag);

	s = splserial();
	COM_LOCK(sc);	
	
	ixpcom_cr = (ixpcom_cr & ~CFLAGS2CR_MASK) | cr;

	/*
	 * ixpcom don't have any hardware flow control.
	 * we skip it.
	 */

	/* And copy to tty. */
	tp->t_ispeed = 0;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;

	if (!sc->sc_heldchange) {
		if (sc->sc_tx_busy) {
			sc->sc_heldtbc = sc->sc_tbc;
			sc->sc_tbc = 0;
			sc->sc_heldchange = 1;
		} else
			ixpcom_set_cr(sc);
	}

	COM_UNLOCK(sc);
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
			ixpcomstart(tp);
		}
	}

	return (0);
}

static int
ixpcomhwiflow(tp, block)
	struct tty *tp;
	int block;
{
	return (0);
}

static void
ixpcom_filltx(struct ixpcom_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int n;

	n = 0;
        while (bus_space_read_4(iot, ioh, IXPCOM_SR) & SR_TXR) {
		if (n >= sc->sc_tbc)
			break;
		bus_space_write_4(iot, ioh, IXPCOM_DR,
				  0xff & *(sc->sc_tba + n));
		n++;
        }
        sc->sc_tbc -= n;
        sc->sc_tba += n;
}

static void
ixpcomstart(tp)
	struct tty *tp;
{
	struct ixpcom_softc *sc
		= device_lookup(&ixpcom_cd, COMUNIT(tp->t_dev));
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
		COM_LOCK(sc);

		sc->sc_tba = tba;
		sc->sc_tbc = tbc;
	}

	SET(tp->t_state, TS_BUSY);
	sc->sc_tx_busy = 1;

	/* Enable transmit completion interrupts if necessary. */
	if (!ISSET(sc->sc_xie, CR_XIE)) {
		SET(sc->sc_xie, CR_XIE);
		ixpcom_set_cr(sc);
	}

	/* Output the first chunk of the contiguous buffer. */
	ixpcom_filltx(sc);

	COM_UNLOCK(sc);
out:
	splx(s);
	return;
}

static void
ixpcom_break(struct ixpcom_softc *sc, int onoff)
{
	if (onoff)
		SET(ixpcom_cr, CR_BRK);
	else
		CLR(ixpcom_cr, CR_BRK);
	if (!sc->sc_heldchange) {
		if (sc->sc_tx_busy) {
			sc->sc_heldtbc = sc->sc_tbc;
			sc->sc_tbc = 0;
			sc->sc_heldchange = 1;
		} else
			ixpcom_set_cr(sc);
	}
}

static void
ixpcom_shutdown(struct ixpcom_softc *sc)
{
	int s;

	s = splserial();
	COM_LOCK(sc);	

	/* Clear any break condition set with TIOCSBRK. */
	ixpcom_break(sc, 0);

	/* Turn off interrupts. */
	sc->sc_rie = sc->sc_xie = 0;
	ixpcom_set_cr(sc);

	if (sc->disable) {
#ifdef DIAGNOSTIC
		if (!sc->enabled)
			panic("ixpcom_shutdown: not enabled?");
#endif
		(*sc->disable)(sc);
		sc->enabled = 0;
	}
	COM_UNLOCK(sc);
	splx(s);
}

int
ixpcomopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct ixpcom_softc *sc;
	struct tty *tp;
	int s, s2;
	int error;

	sc = device_lookup(&ixpcom_cd, COMUNIT(dev));
	if (sc == NULL || !ISSET(sc->sc_hwflags, COM_HW_DEV_OK) ||
		sc->sc_rbuf == NULL)
		return (ENXIO);

	if (ISSET(sc->sc_dev.dv_flags, DVF_ACTIVE) == 0)
		return (ENXIO);

#ifdef KGDB
	/*
	 * If this is the kgdb port, no other use is permitted.
	 */
	if (ISSET(sc->sc_hwflags, COM_HW_KGDB))
		return (EBUSY);
#endif

	tp = sc->sc_tty;

	if (ISSET(tp->t_state, TS_ISOPEN) &&
	    ISSET(tp->t_state, TS_XCLUDE) &&
		p->p_ucred->cr_uid != 0)
		return (EBUSY);

	s = spltty();

	/*
	 * Do the following iff this is a first open.
	 */
	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		struct termios t;

		tp->t_dev = dev;

		s2 = splserial();
		COM_LOCK(sc);

		if (sc->enable) {
			if ((*sc->enable)(sc)) {
				COM_UNLOCK(sc);
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
		SET(sc->sc_rie, CR_RIE);
		ixpcom_set_cr(sc);

#if 0
		/* Fetch the current modem control status, needed later. */
		sc->sc_msr = bus_space_read_1(sc->sc_iot, sc->sc_ioh, com_msr);

		/* Clear PPS capture state on first open. */
		sc->sc_ppsmask = 0;
		sc->ppsparam.mode = 0;
#endif

		COM_UNLOCK(sc);
		splx(s2);

		/*
		 * Initialize the termios status to the defaults.  Add in the
		 * sticky bits from TIOCSFLAGS.
		 */
		t.c_ispeed = 0;
		if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
			t.c_ospeed = ixpcomcn_sc.sc_ospeed;
			t.c_cflag = ixpcomcn_sc.sc_cflag;
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
		/* Make sure ixpcomparam() will do something. */
		tp->t_ospeed = 0;
		(void) ixpcomparam(tp, &t);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		ttychars(tp);
		ttsetwater(tp);

		s2 = splserial();
		COM_LOCK(sc);

		/* Clear the input ring, and unblock. */
		sc->sc_rbput = sc->sc_rbget = sc->sc_rbuf;
		sc->sc_rbavail = IXPCOM_RING_SIZE;
		ixpcom_iflush(sc);
		CLR(sc->sc_rx_flags, RX_ANY_BLOCK);

#ifdef COM_DEBUG
		if (ixpcom_debug)
			comstatus(sc, "ixpcomopen  ");
#endif

		COM_UNLOCK(sc);
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
		ixpcom_shutdown(sc);
	}

	return (error);
}

int
ixpcomclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct ixpcom_softc *sc = device_lookup(&ixpcom_cd, COMUNIT(dev));
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
		ixpcom_shutdown(sc);
	}

	return (0);
}

int
ixpcomread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct ixpcom_softc *sc = device_lookup(&ixpcom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);
 
	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}

int
ixpcomwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct ixpcom_softc *sc = device_lookup(&ixpcom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);
 
	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
ixpcompoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct ixpcom_softc *sc = device_lookup(&ixpcom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);
 
	return ((*tp->t_linesw->l_poll)(tp, events, p));
}

struct tty *
ixpcomtty(dev)
	dev_t dev;
{
	struct ixpcom_softc *sc = device_lookup(&ixpcom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	return (tp);
}

int
ixpcomioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct ixpcom_softc *sc = device_lookup(&ixpcom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;
	int error;
	int s;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, p);
	if (error != EPASSTHROUGH)
		return (error);

	error = ttioctl(tp, cmd, data, flag, p);
	if (error != EPASSTHROUGH)
		return (error);

	error = 0;

	s = splserial();
	COM_LOCK(sc);	

	switch (cmd) {
	case TIOCSBRK:
		ixpcom_break(sc, 1);
		break;

	case TIOCCBRK:
		ixpcom_break(sc, 0);
		break;

	case TIOCGFLAGS:
		*(int *)data = sc->sc_swflags;
		break;

	case TIOCSFLAGS:
		error = suser(p->p_ucred, &p->p_acflag); 
		if (error)
			break;
		sc->sc_swflags = *(int *)data;
		break;

	default:
		error = EPASSTHROUGH;
		break;
	}

	COM_UNLOCK(sc);
	splx(s);

	return (error);
}

/*
 * Stop output on a line.
 */
void
ixpcomstop(tp, flag)
	struct tty *tp;
	int flag;
{
	struct ixpcom_softc *sc
		= device_lookup(&ixpcom_cd, COMUNIT(tp->t_dev));
	int s;

	s = splserial();
	COM_LOCK(sc);
	if (ISSET(tp->t_state, TS_BUSY)) {
		/* Stop transmitting at the next chunk. */
		sc->sc_tbc = 0;
		sc->sc_heldtbc = 0;
		if (!ISSET(tp->t_state, TS_TTSTOP))
			SET(tp->t_state, TS_FLUSH);
	}
	COM_UNLOCK(sc);	
	splx(s);
}

static u_int
cflag2cr(cflag)
	tcflag_t cflag;
{
	u_int cr;

	cr  = (cflag & PARENB) ? CR_PE : 0;
	cr |= (cflag & PARODD) ? CR_OES : 0;
	cr |= (cflag & CSTOPB) ? CR_SBS : 0;
	cr |= ((cflag & CSIZE) == CS8) ? DSS_8BIT : 0;
	
	return (cr);
}

static void
ixpcom_iflush(sc)
	struct ixpcom_softc *sc;
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
	while (ISSET(bus_space_read_4(iot, ioh, IXPCOM_SR), SR_RXR)
	       && --timo)
#ifdef DIAGNOSTIC
		reg =
#else
			(void)
#endif
			bus_space_read_4(iot, ioh, IXPCOM_DR);
#ifdef DIAGNOSTIC
	if (!timo)
		printf("%s: com_iflush timeout %02x\n", sc->sc_dev.dv_xname,
		       reg);
#endif
}

static void
ixpcom_set_cr(struct ixpcom_softc *sc)
{
	/* XXX */
	ixpcom_cr &= ~(CR_RIE | CR_XIE);
	ixpcom_cr |= sc->sc_rie | sc->sc_xie;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IXPCOM_CR,
			  ixpcom_cr & ~ixpcom_imask);
}

int
ixpcomcnattach(iot, iobase, ioh, ospeed, cflag)
	bus_space_tag_t iot;
	bus_addr_t iobase;
	bus_space_handle_t ioh;
	int ospeed;
	tcflag_t cflag;
{
	int cr;

	cn_tab = &ixpcomcons;
	cn_init_magic(&ixpcom_cnm_state);
	/*
	 * XXX
	 *
	 * ixpcom cannot detect a break.  It can only detect framing
	 * errors.  And, a sequence of "framing error, framing error"
	 * meens a break.  So I made this hack:
	 *
	 * 1. Set default magic is a sequence of "BREAK, BREAK".
	 * 2. Tell cn_check_magic() a "framing error" as BREAK.
	 *
	 * see ixpcom_intr() too.
	 *
	 */
	/* default magic is a couple of BREAK. */
	cn_set_magic("\047\001\047\001");

	ixpcomcn_sc.sc_iot = iot;
	ixpcomcn_sc.sc_ioh = ioh;
	ixpcomcn_sc.sc_baseaddr = iobase;
	ixpcomcn_sc.sc_ospeed = ospeed;
	ixpcomcn_sc.sc_cflag = cflag;

	cr = cflag2cr(cflag);
	cr |= IXPCOMSPEED2BRD(ospeed);
	cr |= CR_UE;
	ixpcom_cr = cr;

	/* enable the UART */
	bus_space_write_4(iot, iobase, IXPCOM_CR, cr);

	return (0);
}

void
ixpcomcnprobe(cp)
	struct consdev *cp;
{
	cp->cn_pri = CN_REMOTE;
}

void
ixpcomcnpollc(dev, on)
	dev_t dev;
	int on;
{
}

void
ixpcomcnputc(dev, c)
	dev_t dev;
	int c;
{
	int			s;
	bus_space_tag_t		iot = ixpcomcn_sc.sc_iot;
	bus_space_handle_t	ioh = ixpcomcn_sc.sc_ioh;

	s = splserial();

	while(!(bus_space_read_4(iot, ioh, IXPCOM_SR) & SR_TXR))
		;

	bus_space_write_4(iot, ioh, IXPCOM_DR, c);

#ifdef DEBUG
	if (c == '\r') {
		while(!(bus_space_read_4(iot, ioh, IXPCOM_SR) & SR_TXE))
			;
	}
#endif

	splx(s);
}

int
ixpcomcngetc(dev)
        dev_t dev;
{
	int			c;
	int			s;
	bus_space_tag_t		iot = ixpcomcn_sc.sc_iot;
	bus_space_handle_t	ioh = ixpcomcn_sc.sc_ioh;

        s = splserial();

	while(!(bus_space_read_4(iot, ioh, IXPCOM_SR) & SR_RXR))
		;

	c = bus_space_read_4(iot, ioh, IXPCOM_DR);
	c &= 0xff;
	splx(s);

	return (c);
}

inline static void
ixpcom_txsoft(sc, tp)
	struct ixpcom_softc *sc;
	struct tty *tp;
{
	CLR(tp->t_state, TS_BUSY);
	if (ISSET(tp->t_state, TS_FLUSH))
		CLR(tp->t_state, TS_FLUSH);
        else
		ndflush(&tp->t_outq, (int)(sc->sc_tba - tp->t_outq.c_cf));
	(*tp->t_linesw->l_start)(tp);
}

inline static void
ixpcom_rxsoft(sc, tp)
	struct ixpcom_softc *sc;
	struct tty *tp;
{
	int (*rint) __P((int c, struct tty *tp)) = tp->t_linesw->l_rint;
	u_char *get, *end;
	u_int cc, scc;
	u_char lsr;
	int code;
	int s;

	end = sc->sc_ebuf;
	get = sc->sc_rbget;
	scc = cc = IXPCOM_RING_SIZE - sc->sc_rbavail;
#if 0
	if (cc == IXPCOM_RING_SIZE) {
		sc->sc_floods++;
		if (sc->sc_errors++ == 0)
			callout_reset(&sc->sc_diag_callout, 60 * hz,
			    comdiag, sc);
	}
#endif
	while (cc) {
		code = get[0];
		lsr = get[1];
		if (ISSET(lsr, DR_ROR | DR_FRE | DR_PRE)) {
#if 0
			if (ISSET(lsr, DR_ROR)) {
				sc->sc_overflows++;
				if (sc->sc_errors++ == 0)
					callout_reset(&sc->sc_diag_callout,
					    60 * hz, comdiag, sc);
			}
#endif
			if (ISSET(lsr, DR_FRE))
				SET(code, TTY_FE);
			if (ISSET(lsr, DR_PRE))
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
					get -= IXPCOM_RING_SIZE << 1;
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
		COM_LOCK(sc);
		
		cc = sc->sc_rbavail += scc - cc;
		/* Buffers should be ok again, release possible block. */
		if (cc >= 1) {
			if (ISSET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED)) {
				CLR(sc->sc_rx_flags, RX_IBUF_OVERFLOWED);
				SET(sc->sc_rie, CR_RIE);
				ixpcom_set_cr(sc);
			}
			if (ISSET(sc->sc_rx_flags, RX_IBUF_BLOCKED)) {
				CLR(sc->sc_rx_flags, RX_IBUF_BLOCKED);
#if 0
				com_hwiflow(sc);
#endif
			}
		}
		COM_UNLOCK(sc);
		splx(s);
	}
}

static void
ixpcomsoft(void* arg)
{
	struct ixpcom_softc *sc = arg;

	if (COM_ISALIVE(sc) == 0)
		return;

	if (sc->sc_rx_ready) {
		sc->sc_rx_ready = 0;
		ixpcom_rxsoft(sc, sc->sc_tty);
	}
	if (sc->sc_tx_done) {
		sc->sc_tx_done = 0;
		ixpcom_txsoft(sc, sc->sc_tty);
	}
}

int
ixpcomintr(void* arg)
{
	struct ixpcom_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_char *put, *end;
	u_int cc;
	u_int cr;
	u_int sr;
	u_int32_t c;

	if (COM_ISALIVE(sc) == 0)
		return (0);

	COM_LOCK(sc);
	cr = bus_space_read_4(iot, ioh, IXPCOM_CR) & CR_UE;

	if (!cr) {
		COM_UNLOCK(sc);
		return (0);
	}

	sr = bus_space_read_4(iot, ioh, IXPCOM_SR);
	if (!ISSET(sr, SR_TXR | SR_RXR))
		return (0);

	/*
	 * XXX
	 *
	 * ixpcom cannot detect a break.  It can only detect framing
	 * errors.  And, a sequence of "framing error, framing error"
	 * meens a break.  So I made this hack:
	 *
	 * 1. Set default magic is a sequence of "BREAK, BREAK".
	 * 2. Tell cn_check_magic() a "framing error" as BREAK.
	 *
	 * see ixpcomcnattach() too.
	 *
	 */
	if (ISSET(sr, SR_FRE)) {
		cn_check_magic(sc->sc_tty->t_dev,
			       CNC_BREAK, ixpcom_cnm_state);
	}

	end = sc->sc_ebuf;
	put = sc->sc_rbput;
	cc = sc->sc_rbavail;

	if (ISSET(sr, SR_RXR)) {
		if (!ISSET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED)) {
			while (cc > 0) {
				if (!ISSET(sr, SR_RXR))
					break;
				c = bus_space_read_4(iot, ioh, IXPCOM_DR);
				if (ISSET(c, DR_FRE)) {
					cn_check_magic(sc->sc_tty->t_dev,
						       CNC_BREAK,
						       ixpcom_cnm_state);
				}
				put[0] = c & 0xff;
				put[1] = (c >> 8) & 0xff;
				cn_check_magic(sc->sc_tty->t_dev,
					       put[0], ixpcom_cnm_state);
				put += 2;
				if (put >= end)
					put = sc->sc_rbuf;
				cc--;

				sr = bus_space_read_4(iot, ioh, IXPCOM_SR);
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

			/* but ixpcom cannot. X-( */

			/*
			 * If we're out of space, disable receive interrupts
			 * until the queue has drained a bit.
			 */
			if (!cc) {
				SET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED);
				CLR(sc->sc_rie, CR_RIE);
				ixpcom_set_cr(sc);
			}
		} else {
#ifdef DIAGNOSTIC
			panic("ixpcomintr: we shouldn't reach here");
#endif
			CLR(sc->sc_rie, CR_RIE);
			ixpcom_set_cr(sc);
		}
	}

	/*
	 * Done handling any receive interrupts. See if data can be
	 * transmitted as well. Schedule tx done event if no data left
	 * and tty was marked busy.
	 */
	sr = bus_space_read_4(iot, ioh, IXPCOM_SR);
	if (ISSET(sr, SR_TXR)) {
		/*
		 * If we've delayed a parameter change, do it now, and restart
		 * output.
		 */
		if (sc->sc_heldchange) {
			ixpcom_set_cr(sc);
			sc->sc_heldchange = 0;
			sc->sc_tbc = sc->sc_heldtbc;
			sc->sc_heldtbc = 0;
		}

		/* Output the next chunk of the contiguous buffer, if any. */
		if (sc->sc_tbc > 0) {
			ixpcom_filltx(sc);
		} else {
			/* Disable transmit completion interrupts if necessary. */
			if (ISSET(sc->sc_xie, CR_XIE)) {
				CLR(sc->sc_xie, CR_XIE);
				ixpcom_set_cr(sc);
			}
			if (sc->sc_tx_busy) {
				sc->sc_tx_busy = 0;
				sc->sc_tx_done = 1;
			}
		}
	}
	COM_UNLOCK(sc);

	/* Wake up the poller. */
	softintr_schedule(sc->sc_si);

#if NRND > 0 && defined(RND_COM)
	rnd_add_uint32(&sc->rnd_source, iir | lsr);
#endif
	return (1);
}
