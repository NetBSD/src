/*      $NetBSD: sa11x0_com.c,v 1.5.2.2 2001/03/12 13:28:31 bouyer Exp $        */

/*-
 * Copyright (c) 1998, 1999, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
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
 *	@(#)com.c	7.5 (Berkeley) 5/16/91
 */

#include "opt_ddb.h"
#include "opt_ddbparam.h"
#include "opt_com.h"

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

#include <dev/cons.h>

#include <machine/bus.h>
#include <hpcarm/sa11x0/sa11x0_reg.h>
#include <hpcarm/sa11x0/sa11x0_var.h>
#include <hpcarm/sa11x0/sa11x0_comreg.h>
#include <hpcarm/sa11x0/sa11x0_comvar.h>

#include "sacom.h"

cdev_decl(sacom);

static	int	sacom_match(struct device *, struct cfdata *, void *);
static	void	sacom_attach(struct device *, struct device *, void *);
static	void	sacom_filltx(struct sacom_softc *);
static	void	sacom_attach_subr(struct sacom_softc *);
#if defined(DDB) || defined(KGDB)
static	void	sacom_enable_debugport(struct sacom_softc *);
#endif
void		sacom_config(struct sacom_softc *);
void		sacom_shutdown(struct sacom_softc *);
static	u_int	cflag2cr0(tcflag_t);
int		sacomparam(struct tty *, struct termios *);
void		sacomstart(struct tty *);
void		sacomstop(struct tty *, int);
int		sacomhwiflow(struct tty *, int);

void		sacom_loadchannelregs(struct sacom_softc *);
void		sacom_hwiflow(struct sacom_softc *);
void		sacom_break(struct sacom_softc *, int);
void		sacom_modem(struct sacom_softc *, int);
void		tiocm_to_sacom(struct sacom_softc *, u_long, int);
int		sacom_to_tiocm(struct sacom_softc *);
void		sacom_iflush(struct sacom_softc *);

#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
void 		sacomsoft(void *);
#else
void 		sacomsoft(void);
#endif

static inline void sacom_rxsoft(struct sacom_softc *, struct tty *);
static inline void sacom_txsoft(struct sacom_softc *, struct tty *);
static inline void sacom_stsoft(struct sacom_softc *, struct tty *);
static inline void sacom_schedrx(struct sacom_softc *);


#define COMUNIT_MASK	0x7ffff
#define COMDIALOUT_MASK	0x80000

#define COMUNIT(x)	(minor(x) & COMUNIT_MASK)
#define COMDIALOUT(x)	(minor(x) & COMDIALOUT_MASK)

#define COM_ISALIVE(sc)	((sc)->enabled != 0 && \
			 ISSET((sc)->sc_dev.dv_flags, DVF_ACTIVE))

#define COM_BARRIER(t, h, f) bus_space_barrier((t), (h), 0, COM_NPORTS, (f))
#define COM_LOCK(sc)
#define COM_UNLOCK(sc)

#define SET(t, f)	(t) |= (f)
#define CLR(t, f)	(t) &= ~(f)
#define ISSET(t, f)	((t) & (f))

int		sacomintr(void *);
int		sacomcngetc(dev_t);
void		sacomcnputc(dev_t, int);
void		sacomcnpollc(dev_t, int);

void		sacomcnprobe(struct consdev *);
void		sacomcninit(struct consdev *);

extern struct bus_space sa11x0_bs_tag;

static bus_space_tag_t sacomconstag;
static bus_space_handle_t sacomconsioh;
static bus_addr_t sacomconsaddr = SACOM3_BASE; /* XXX */

static int sacomconsattached;
static int sacomconsrate;
static tcflag_t sacomconscflag;

struct cfattach sacom_ca = {
	sizeof(struct sacom_softc), sacom_match, sacom_attach
};
extern struct cfdriver sacom_cd;

struct consdev sacomcons = {
	NULL, NULL, sacomcngetc, sacomcnputc, sacomcnpollc, NULL,
	NODEV, CN_NORMAL
};

#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
#ifndef CONSPEED
#define CONSPEED 9600
#endif
#ifndef CONADDR
#define CONADDR SACOM3_BASE
#endif

static int
sacom_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return(0);
}

void
sacom_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct sacom_softc *sc = (struct sacom_softc*)self;
	struct sa11x0_attach_args *sa = aux;

	sc->sc_iot = sa->sa_iot;
	sc->sc_baseaddr = sa->sa_addr;

	if(bus_space_map(sa->sa_iot, sa->sa_addr, sa->sa_size, 0,
			&sc->sc_ioh)) {
		printf("%s: unable to map registers\n", sc->sc_dev.dv_xname);
		return;
	}

	printf("%s: ", sc->sc_dev.dv_xname);
	switch(sc->sc_baseaddr) {
	case 0x8005000:
		printf("SA11x0 UART3\n");
		break;
	case 0x8003000:
		printf("SA11x0 UART1\n");
		break;
	default:
		printf("unknown SA11x0 UART\n");
		break;
	}

	sacom_attach_subr(sc);

	sa11x0_intr_establish(0, sa->sa_intr, 1, IPL_SERIAL, sacomintr, sc);
}

void
sacom_attach_subr(sc)
	struct sacom_softc *sc;
{
	bus_addr_t iobase = sc->sc_baseaddr;
	bus_space_tag_t iot = sc->sc_iot;
	struct tty *tp;

#if (defined(MULTIPROCESSOR) || defined(LOCKDEBUG)) && defined(COM_MPLOCK)
	simple_lock_init(&sc->sc_lock);
#endif

	/* XXX Do we need to disable interrupts here? */

	if (iot == sacomconstag && iobase == sacomconsaddr) {
		sacomconsattached = 1;
		sc->sc_speed = SACOMSPEED(sacomconsrate);

		/* Make sure the console is always "hardwired". */
		delay(10000);			/* wait for output to finish */
		SET(sc->sc_hwflags, COM_HW_CONSOLE);
		SET(sc->sc_swflags, TIOCFLAG_SOFTCAR);
	}

	tp = ttymalloc();
	tp->t_oproc = sacomstart;
	tp->t_param = sacomparam;
	tp->t_hwiflow = sacomhwiflow;

	sc->sc_tty = tp;
	sc->sc_rbuf = malloc(SACOM_RING_SIZE << 1, M_DEVBUF, M_NOWAIT);
	sc->sc_rbput = sc->sc_rbget = sc->sc_rbuf;
	sc->sc_rbavail = SACOM_RING_SIZE;
	if (sc->sc_rbuf == NULL) {
		printf("%s: unable to allocate ring buffer\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	sc->sc_ebuf = sc->sc_rbuf + (SACOM_RING_SIZE << 1);
	sc->sc_tbc = 0;

	tty_attach(tp);

	if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
		int maj;

		/* locate the major number */
		for (maj = 0; maj < nchrdev; maj++)
			if (cdevsw[maj].d_open == sacomopen)
				break;

		cn_tab->cn_dev = makedev(maj, sc->sc_dev.dv_unit);

		delay(10000); /* XXX */
		printf("%s: console\n", sc->sc_dev.dv_xname);
		delay(10000); /* XXX */
	}


#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
	sc->sc_si = softintr_establish(IPL_SOFTSERIAL, sacomsoft, sc);
#endif

#if NRND > 0 && defined(RND_COM)
	rnd_attach_source(&sc->rnd_source, sc->sc_dev.dv_xname,
			  RND_TYPE_TTY, 0);
#endif

	/* if there are no enable/disable functions, assume the device
	   is always enabled */
	if (!sc->enable)
		sc->enabled = 1;

	sacom_config(sc);

	SET(sc->sc_hwflags, COM_HW_DEV_OK);
}

/* This is necessary when dynamically changing SAIP configuration. */
int
sacom_detach(self, flags)
	struct device *self;
	int flags;
{
	struct sacom_softc *sc = (struct sacom_softc *)self;
	int maj, mn;

	/* locate the major number */
	for (maj = 0; maj < nchrdev; maj++)
		if (cdevsw[maj].d_open == sacomopen)
			break;

	/* Nuke the vnodes for any open instances. */
	mn = self->dv_unit;
	vdevgone(maj, mn, mn, VCHR);

	mn |= COMDIALOUT_MASK;
	vdevgone(maj, mn, mn, VCHR);

	/* Free the receive buffer. */
	free(sc->sc_rbuf, M_DEVBUF);

	/* Detach and free the tty. */
	tty_detach(sc->sc_tty);
	ttyfree(sc->sc_tty);

#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
	/* Unhook the soft interrupt handler. */
	softintr_disestablish(sc->sc_si);
#endif

#if NRND > 0 && defined(RND_COM)
	/* Unhook the entropy source. */
	rnd_detach_source(&sc->rnd_source);
#endif

	return (0);
}

void
sacom_config(sc)
	struct sacom_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	/* Disable engine before configuring the device. */
	sc->sc_cr3 = 0;
	bus_space_write_4(iot, ioh, SACOM_CR3, sc->sc_cr3);

#ifdef DDB
	if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE))
		sacom_enable_debugport(sc);
#endif
}

static void
sacom_enable_debugport(sc)
	struct sacom_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int s;

	s = splserial();
	COM_LOCK(sc);
	sc->sc_cr3 = CR3_RXE | CR3_TXE;
	bus_space_write_4(iot, ioh, SACOM_CR3, sc->sc_cr3);
	COM_UNLOCK(sc);
	splx(s);
}

int
sacom_activate(self, act)
	struct device *self;
	enum devact act;
{
	struct sacom_softc *sc = (struct sacom_softc *)self;
	int s, rv = 0;

	s = splserial();
	COM_LOCK(sc);
	switch (act) {
	case DVACT_ACTIVATE:
		rv = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		if (sc->sc_hwflags & (COM_HW_CONSOLE|COM_HW_KGDB)) {
			rv = EBUSY;
			break;
		}

		if (sc->disable != NULL && sc->enabled != 0) {
			(*sc->disable)(sc);
			sc->enabled = 0;
		}
		break;
	}

	COM_UNLOCK(sc);	
	splx(s);
	return (rv);
}

void
sacom_shutdown(sc)
	struct sacom_softc *sc;
{
	struct tty *tp = sc->sc_tty;
	int s;

	s = splserial();
	COM_LOCK(sc);	

	/* Clear any break condition set with TIOCSBRK. */
	sacom_break(sc, 0);

	/*
	 * Hang up if necessary.  Wait a bit, so the other side has time to
	 * notice even if we immediately open the port again.
	 * Avoid tsleeping above splhigh().
	 */
	if (ISSET(tp->t_cflag, HUPCL)) {
		sacom_modem(sc, 0);
		COM_UNLOCK(sc);
		splx(s);
		/* XXX tsleep will only timeout */
		(void) tsleep(sc, TTIPRI, ttclos, hz);
		s = splserial();
		COM_LOCK(sc);	
	}

	/* Turn off interrupts. */
	sc->sc_cr3 = 0;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SACOM_CR3, sc->sc_cr3);

	if (sc->disable) {
#ifdef DIAGNOSTIC
		if (!sc->enabled)
			panic("sacom_shutdown: not enabled?");
#endif
		(*sc->disable)(sc);
		sc->enabled = 0;
	}
	COM_UNLOCK(sc);
	splx(s);
}

int
sacomopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct sacom_softc *sc;
	struct tty *tp;
	int s, s2;
	int error;

	sc = device_lookup(&sacom_cd, COMUNIT(dev));
	if (sc == NULL || !ISSET(sc->sc_hwflags, COM_HW_DEV_OK) ||
		sc->sc_rbuf == NULL)
		return (ENXIO);

	if (ISSET(sc->sc_dev.dv_flags, DVF_ACTIVE) == 0)
		return (ENXIO);

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
			sacom_config(sc);
		}

		/* Turn on interrupts. */
		sc->sc_cr3 = CR3_RXE | CR3_TXE | CR3_RIE | CR3_TIE;
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, SACOM_CR3,
				  sc->sc_cr3);


		COM_UNLOCK(sc);
		splx(s2);

		/*
		 * Initialize the termios status to the defaults.  Add in the
		 * sticky bits from TIOCSFLAGS.
		 */
		t.c_ispeed = 0;
		if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
			t.c_ospeed = sacomconsrate;
			t.c_cflag = sacomconscflag;
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
		/* Make sure sacomparam() will do something. */
		tp->t_ospeed = 0;
		(void) sacomparam(tp, &t);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		ttychars(tp);
		ttsetwater(tp);

		s2 = splserial();
		COM_LOCK(sc);

		/*
		 * Turn on DTR.  We must always do this, even if carrier is not
		 * present, because otherwise we'd have to use TIOCSDTR
		 * immediately after setting CLOCAL, which applications do not
		 * expect.  We always assert DTR while the device is open
		 * unless explicitly requested to deassert it.
		 */
		sacom_modem(sc, 1);

		/* Clear the input ring, and unblock. */
		sc->sc_rbput = sc->sc_rbget = sc->sc_rbuf;
		sc->sc_rbavail = SACOM_RING_SIZE;
		sacom_iflush(sc);
		CLR(sc->sc_rx_flags, RX_ANY_BLOCK);
		sacom_hwiflow(sc);

#ifdef COM_DEBUG
		if (sacom_debug)
			comstatus(sc, "sacomopen  ");
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
		sacom_shutdown(sc);
	}

	return (error);
}
 
int
sacomclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct sacom_softc *sc = device_lookup(&sacom_cd, COMUNIT(dev));
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
		sacom_shutdown(sc);
	}

	return (0);
}
 
int
sacomread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct sacom_softc *sc = device_lookup(&sacom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);
 
	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}
 
int
sacomwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct sacom_softc *sc = device_lookup(&sacom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);
 
	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

struct tty *
sacomtty(dev)
	dev_t dev;
{
	struct sacom_softc *sc = device_lookup(&sacom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	return (tp);
}

int
sacomioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct sacom_softc *sc = device_lookup(&sacom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;
	int error;
	int s;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, p);
	if (error >= 0)
		return (error);

	error = ttioctl(tp, cmd, data, flag, p);
	if (error >= 0)
		return (error);

	error = 0;

	s = splserial();
	COM_LOCK(sc);	

	switch (cmd) {
	case TIOCSBRK:
		sacom_break(sc, 1);
		break;

	case TIOCCBRK:
		sacom_break(sc, 0);
		break;

	case TIOCSDTR:
		sacom_modem(sc, 1);
		break;

	case TIOCCDTR:
		sacom_modem(sc, 0);
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

	case TIOCMSET:
	case TIOCMBIS:
	case TIOCMBIC:
		tiocm_to_sacom(sc, cmd, *(int *)data);
		break;

	case TIOCMGET:
		*(int *)data = sacom_to_tiocm(sc);
		break;

	default:
		error = ENOTTY;
		break;
	}

	COM_UNLOCK(sc);
	splx(s);

#ifdef COM_DEBUG
	if (sacom_debug)
		comstatus(sc, "comioctl ");
#endif

	return (error);
}

static inline void
sacom_schedrx(sc)
	struct sacom_softc *sc;
{

	sc->sc_rx_ready = 1;

	/* Wake up the poller. */
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
	softintr_schedule(sc->sc_si);
#else
	setsoftserial();
#endif
}

void
sacom_break(sc, onoff)
	struct sacom_softc *sc;
	int onoff;
{

	if (onoff)
		SET(sc->sc_cr3, CR3_BRK);
	else
		CLR(sc->sc_cr3, CR3_BRK);

	if (!sc->sc_heldchange) {
		if (sc->sc_tx_busy) {
			sc->sc_heldtbc = sc->sc_tbc;
			sc->sc_tbc = 0;
			sc->sc_heldchange = 1;
		} else
			sacom_loadchannelregs(sc);
	}
}

void
sacom_modem(sc, onoff)
	struct sacom_softc *sc;
	int onoff;
{
	if (!sc->sc_heldchange) {
		if (sc->sc_tx_busy) {
			sc->sc_heldtbc = sc->sc_tbc;
			sc->sc_tbc = 0;
			sc->sc_heldchange = 1;
		} else
			sacom_loadchannelregs(sc);
	}
}

void
tiocm_to_sacom(sc, how, ttybits)
	struct sacom_softc *sc;
	u_long how;
	int ttybits;
{
}

int
sacom_to_tiocm(sc)
	struct sacom_softc *sc;
{
	int ttybits = 0;

	if (sc->sc_cr3 != 0)
		SET(ttybits, TIOCM_LE);

	return (ttybits);
}

static u_int
cflag2cr0(cflag)
	tcflag_t cflag;
{
	u_int cr0;

	cr0  = (cflag & PARENB) ? CR0_PE : 0;
	cr0 |= (cflag & PARODD) ? 0 : CR0_OES;
	cr0 |= (cflag & CSTOPB) ? CR0_SBS : 0;
	cr0 |= ((cflag & CSIZE) == CS8) ? CR0_DSS : 0;

	return (cr0);
}

int
sacomparam(tp, t)
	struct tty *tp;
	struct termios *t;
{
	struct sacom_softc *sc = device_lookup(&sacom_cd, COMUNIT(tp->t_dev));
	int ospeed = SACOMSPEED(t->c_ospeed);
	u_int cr0;
	int s;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);

	/* Check requested parameters. */
	if (ospeed < 0)
		return (EINVAL);
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

	cr0 = cflag2cr0(t->c_cflag);

	s = splserial();
	COM_LOCK(sc);	

	sc->sc_cr0 = cr0;

	sc->sc_speed = ospeed;

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
			sacom_loadchannelregs(sc);
	}

	if (!ISSET(t->c_cflag, CHWFLOW)) {
		/* Disable the high water mark. */
		if (ISSET(sc->sc_rx_flags, RX_TTY_OVERFLOWED)) {
			CLR(sc->sc_rx_flags, RX_TTY_OVERFLOWED);
			sacom_schedrx(sc);
		}
		if (ISSET(sc->sc_rx_flags, RX_TTY_BLOCKED|RX_IBUF_BLOCKED)) {
			CLR(sc->sc_rx_flags, RX_TTY_BLOCKED|RX_IBUF_BLOCKED);
			sacom_hwiflow(sc);
		}
	}

	COM_UNLOCK(sc);
	splx(s);

	(void) (*tp->t_linesw->l_modem)(tp, 1);

#ifdef COM_DEBUG
	if (sacom_debug)
		comstatus(sc, "comparam ");
#endif

	return (0);
}

void
sacom_iflush(sc)
	struct sacom_softc *sc;
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
	timo = 50;
	/* flush any pending I/O */
	if (sc->sc_cr3 & CR3_RXE) {
	while (ISSET(bus_space_read_4(iot, ioh, SACOM_SR1), SR1_RNE)
	    && --timo)
#ifdef DIAGNOSTIC
		reg =
#else
		    (void)
#endif
		    bus_space_read_4(iot, ioh, SACOM_DR);
	}
#if 0
	/* XXX is it good idea to wait TX finish? */
	if (sc->sc_cr3 & CR3_TXE) {
	timo = 500;
	while (ISSET(bus_space_read_4(iot, ioh, SACOM_SR1), SR1_TBY)
	    && --timo)
		delay(100);
	}
#endif
#ifdef DIAGNOSTIC
	if (!timo)
		printf("%s: sacom_iflush timeout %02x\n", sc->sc_dev.dv_xname,
		       reg);
#endif
}

void
sacom_loadchannelregs(sc)
	struct sacom_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	/* XXXXX necessary? */
	sacom_iflush(sc);

	/* Need to stop engines first. */
	bus_space_write_4(iot, ioh, SACOM_CR3, 0);

	bus_space_write_4(iot, ioh, SACOM_CR1, sc->sc_speed >> 8);
	bus_space_write_4(iot, ioh, SACOM_CR2, sc->sc_speed & 0xff);

	bus_space_write_4(iot, ioh, SACOM_CR0, sc->sc_cr0);
	bus_space_write_4(iot, ioh, SACOM_CR3, sc->sc_cr3);
}

int
sacomhwiflow(tp, block)
	struct tty *tp;
	int block;
{
#if 0
	struct sacom_softc *sc = device_lookup(&sacom_cd, COMUNIT(tp->t_dev));
	int s;

	if (COM_ISALIVE(sc) == 0)
		return (0);

	if (sc->sc_mcr_rts == 0)
		return (0);

	s = splserial();
	COM_LOCK(sc);
	
	if (block) {
		if (!ISSET(sc->sc_rx_flags, RX_TTY_BLOCKED)) {
			SET(sc->sc_rx_flags, RX_TTY_BLOCKED);
			sacom_hwiflow(sc);
		}
	} else {
		if (ISSET(sc->sc_rx_flags, RX_TTY_OVERFLOWED)) {
			CLR(sc->sc_rx_flags, RX_TTY_OVERFLOWED);
			sacom_schedrx(sc);
		}
		if (ISSET(sc->sc_rx_flags, RX_TTY_BLOCKED)) {
			CLR(sc->sc_rx_flags, RX_TTY_BLOCKED);
			sacom_hwiflow(sc);
		}
	}

	COM_UNLOCK(sc);
	splx(s);
#endif
	return (1);
}
	
/*
 * (un)block input via hw flowcontrol
 */
void
sacom_hwiflow(sc)
	struct sacom_softc *sc;
{
#if 0
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	/* XXX implement */
#endif
}


void
sacomstart(tp)
	struct tty *tp;
{
	struct sacom_softc *sc = device_lookup(&sacom_cd, COMUNIT(tp->t_dev));
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int s;

	if (COM_ISALIVE(sc) == 0)
		return;

	s = spltty();
	if (ISSET(tp->t_state, TS_BUSY | TS_TIMEOUT | TS_TTSTOP))
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
	if (!ISSET(sc->sc_cr3, CR3_TIE)) {
		SET(sc->sc_cr3, CR3_TIE);
		bus_space_write_4(iot, ioh, SACOM_CR3, sc->sc_cr3);
	}

	/* Output the first chunk of the contiguous buffer. */
	sacom_filltx(sc);

	COM_UNLOCK(sc);
out:
	splx(s);
	return;
}

void
sacom_filltx(sc)
	struct sacom_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int c, n;

	n = 0;
	while(bus_space_read_4(sacomconstag, sacomconsioh, SACOM_SR1)
	      & SR1_TNF) {
		if (n == SACOM_TXFIFOLEN || n == sc->sc_tbc)
			break;
		c = *(sc->sc_tba + n);
		c &= 0xff;
		bus_space_write_4(iot, ioh, SACOM_DR, c);
		n++;
	}
	sc->sc_tbc -= n;
	sc->sc_tba += n;
}

/*
 * Stop output on a line.
 */
void
sacomstop(tp, flag)
	struct tty *tp;
	int flag;
{
	struct sacom_softc *sc = device_lookup(&sacom_cd, COMUNIT(tp->t_dev));
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

static inline void
sacom_rxsoft(sc, tp)
	struct sacom_softc *sc;
	struct tty *tp;
{
	int (*rint)(int c, struct tty *tp) = tp->t_linesw->l_rint;
	u_char *get, *end;
	u_int cc, scc;
	u_char sr1;
	int code;
	int s;

	end = sc->sc_ebuf;
	get = sc->sc_rbget;
	scc = cc = SACOM_RING_SIZE - sc->sc_rbavail;

	while (cc) {
		code = get[0];
		sr1 = get[1];
		if (ISSET(sr1, SR1_FRE))
			SET(code, TTY_FE);
		if (ISSET(sr1, SR1_PRE))
			SET(code, TTY_PE);
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
					get -= SACOM_RING_SIZE << 1;
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
				SET(sc->sc_cr3, CR3_RIE);
				bus_space_write_4(sc->sc_iot, sc->sc_ioh,
						  SACOM_CR3, sc->sc_cr3);
			}
		}
		COM_UNLOCK(sc);
		splx(s);
	}
}

static inline void
sacom_txsoft(sc, tp)
	struct sacom_softc *sc;
	struct tty *tp;
{

	CLR(tp->t_state, TS_BUSY);
	if (ISSET(tp->t_state, TS_FLUSH))
		CLR(tp->t_state, TS_FLUSH);
	else
		ndflush(&tp->t_outq, (int)(sc->sc_tba - tp->t_outq.c_cf));
	(*tp->t_linesw->l_start)(tp);
}

static inline void
sacom_stsoft(sc, tp)
	struct sacom_softc *sc;
	struct tty *tp;
{
}

#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
void
sacomsoft(arg)
	void *arg;
{
	struct sacom_softc *sc = arg;
	struct tty *tp;

	if (COM_ISALIVE(sc) == 0)
		return;

	{
#else
void
sacomsoft()
{
	struct sacom_softc	*sc;
	struct tty	*tp;
	int	unit;

	for (unit = 0; unit < sacom_cd.cd_ndevs; unit++) {
		sc = device_lookup(&sacom_cd, unit);
		if (sc == NULL || !ISSET(sc->sc_hwflags, COM_HW_DEV_OK))
			continue;

		if (COM_ISALIVE(sc) == 0)
			continue;

		tp = sc->sc_tty;
		if (tp == NULL)
			continue;
		if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0)
			continue;
#endif
		tp = sc->sc_tty;
		
		if (sc->sc_rx_ready) {
			sc->sc_rx_ready = 0;
			sacom_rxsoft(sc, tp);
		}

		if (sc->sc_st_check) {
			sc->sc_st_check = 0;
			sacom_stsoft(sc, tp);
		}

		if (sc->sc_tx_done) {
			sc->sc_tx_done = 0;
			sacom_txsoft(sc, tp);
		}
	}

#ifndef __HAVE_GENERIC_SOFT_INTERRUPTS
#endif
}

#ifdef __ALIGN_BRACKET_LEVEL_FOR_CTAGS
	/* there has got to be a better way to do comsoft() */
}}
#endif

int
sacomintr(arg)
	void *arg;
{
	struct sacom_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_char *put, *end;
	u_int cc;
	u_int sr0, sr1;

	if (COM_ISALIVE(sc) == 0)
		return (0);

	COM_LOCK(sc);
	sr0 = bus_space_read_4(iot, ioh, SACOM_SR0);
	if (! sr0) {
		COM_UNLOCK(sc);
		return (0);
	}
	if (ISSET(sr0, SR0_EIF))
		/* XXX silently discard error bits */
		bus_space_read_4(iot, ioh, SACOM_DR);
	if (ISSET(sr0, SR0_RBB))
		bus_space_write_4(iot, ioh, SACOM_SR0, SR0_RBB);
	if (ISSET(sr0, SR0_REB)) {
		bus_space_write_4(iot, ioh, SACOM_SR0, SR0_REB);
#if defined(DDB) || defined(KGDB)
#ifndef DDB_BREAK_CHAR
		if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
			console_debugger();
		}
#endif
#endif /* DDB || KGDB */
	}


	end = sc->sc_ebuf;
	put = sc->sc_rbput;
	cc = sc->sc_rbavail;

	sr1 = bus_space_read_4(iot, ioh, SACOM_SR1);
	if (ISSET(sr0, SR0_RFS | SR0_RID)) {
		if (! ISSET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED)) {
			while (cc > 0) {
				if (!ISSET(sr1, SR1_RNE)) {
					bus_space_write_4(iot, ioh, SACOM_SR0,
							  SR0_RID);
					break;
				}
				put[0] = bus_space_read_4(iot, ioh, SACOM_DR);
				put[1] = sr1;
#if defined(DDB) && defined(DDB_BREAK_CHAR)
				if (put[0] == DDB_BREAK_CHAR &&
				    ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
					console_debugger();

					sr1 = bus_space_read_4(iot, ioh, SACOM_SR1);
					continue;
				}
#endif
				put += 2;
				if (put >= end)
					put = sc->sc_rbuf;
				cc--;

				sr1 = bus_space_read_4(iot, ioh, SACOM_SR1);
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

			/* XXX do RX hardware flow control */

			/*
			 * If we're out of space, disable receive interrupts
			 * until the queue has drained a bit.
			 */
			if (!cc) {
				SET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED);
				CLR(sc->sc_cr3, CR3_RIE);
				bus_space_write_4(iot, ioh, SACOM_CR3,
						  sc->sc_cr3);
			}
		} else {
#ifdef DIAGNOSTIC
			panic("sacomintr: we shouldn't reach here\n");
#endif
			CLR(sc->sc_cr3, CR3_RIE);
			bus_space_write_4(iot, ioh, SACOM_CR3, sc->sc_cr3);
		}
	}

	/*
	 * Done handling any receive interrupts. See if data can be
	 * transmitted as well. Schedule tx done event if no data left
	 * and tty was marked busy.
	 */
	sr0 = bus_space_read_4(iot, ioh, SACOM_SR0);
	if (ISSET(sr0, SR0_TFS)) {
		/*
		 * If we've delayed a parameter change, do it now, and restart
		 * output.
		 * XXX sacom_loadchanelregs() waits TX completion,
		 * XXX resulting in ~0.1s hang (300bps, 4 bytes) in worst case
		 */
		if (sc->sc_heldchange) {
			sacom_loadchannelregs(sc);
			sc->sc_heldchange = 0;
			sc->sc_tbc = sc->sc_heldtbc;
			sc->sc_heldtbc = 0;
		}

		/* Output the next chunk of the contiguous buffer, if any. */
		if (sc->sc_tbc > 0) {
			sacom_filltx(sc);
		} else {
			/* Disable transmit completion interrupts if necessary. */
			if (ISSET(sc->sc_cr3, CR3_TIE)) {
				CLR(sc->sc_cr3, CR3_TIE);
				bus_space_write_4(iot, ioh, SACOM_CR3,
						  sc->sc_cr3);
			}
			if (sc->sc_tx_busy) {
				sc->sc_tx_busy = 0;
				sc->sc_tx_done = 1;
			}
		}
	}
	COM_UNLOCK(sc);

	/* Wake up the poller. */
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
	softintr_schedule(sc->sc_si);
#else
	setsoftserial();
#endif

#if NRND > 0 && defined(RND_COM)
	rnd_add_uint32(&sc->rnd_source, iir | lsr);
#endif
	return (1);
}

/* Initialization for serial console */
int
sacominit(iot, iobase, baud, cflag, iohp)
	bus_space_tag_t iot;
	bus_addr_t iobase;
	int baud;
	tcflag_t cflag;
	bus_space_handle_t *iohp;
{
	int brd, cr0;

	if (bus_space_map(iot, iobase, SACOM_NPORTS, 0, iohp))
		printf("register map failed\n");

	/* wait for the Tx queue to drain and disable the UART */
	while(bus_space_read_4(iot, *iohp, SACOM_SR1) & SR1_TBY)
		;
	bus_space_write_4(iot, *iohp, SACOM_CR3, 0);

	cr0  = cflag2cr0(cflag);
	bus_space_write_4(iot, *iohp, SACOM_CR0, cr0);

	brd = SACOMSPEED(baud);
	sacomconsrate = baud;
	sacomconsaddr = iobase;
	sacomconscflag = cflag;
	/* XXX assumes little endian */
	bus_space_write_4(iot, *iohp, SACOM_CR1, brd >> 8);
	bus_space_write_4(iot, *iohp, SACOM_CR2, brd & 0xff);

	/* enable the UART */
	bus_space_write_4(iot, *iohp, SACOM_CR3, CR3_RXE | CR3_TXE);

	return (0);
}

void
sacomcnprobe(cp)
	struct consdev *cp;
{
	cp->cn_pri = CN_REMOTE;
}

void
sacomcninit(cp)
	struct consdev *cp;
{
	if (cp == NULL) {
		/* XXX cp == NULL means that MMU is disabled. */
		if (sacominit(&sa11x0_bs_tag, SACOM3_HW_BASE,
				  CONSPEED, CONMODE, &sacomconsioh))
			panic("can't init serial console @%x", CONADDR);
		sacomconstag = &sa11x0_bs_tag;
		cn_tab = &sacomcons;
		return;
	}

	if (sacominit(&sa11x0_bs_tag, CONADDR, CONSPEED,
			  CONMODE, &sacomconsioh))
		panic("can't init serial console @%x", CONADDR);
	cn_tab = &sacomcons;
	sacomconstag = &sa11x0_bs_tag;
}

int
sacomcngetc(dev)
	dev_t dev;
{
	int c, s;

	s = spltty();	/* XXX do we need this? */

	while(! (bus_space_read_4(sacomconstag, sacomconsioh, SACOM_SR1)
		 & SR1_RNE))
		;

	c = bus_space_read_4(sacomconstag, sacomconsioh, SACOM_DR);
	c &= 0xff;
	splx(s);

	return (c);
}

void
sacomcnputc(dev, c)
	dev_t dev;
	int c;
{
	int s;

	s = spltty();	/* XXX do we need this? */

	while(! (bus_space_read_4(sacomconstag, sacomconsioh, SACOM_SR1)
		 & SR1_TNF))
		;

	bus_space_write_4(sacomconstag, sacomconsioh, SACOM_DR, c);
	splx(s);
}

void
sacomcnpollc(dev, on)
	dev_t dev;
	int on;
{

}

#ifdef DEBUG
int
sacomcncharpoll()
{
	int c;

	if (! (bus_space_read_4(sacomconstag, sacomconsioh, SACOM_SR1)
		 & SR1_RNE))
		return -1;

	c = bus_space_read_4(sacomconstag, sacomconsioh, SACOM_DR);
	c &= 0xff;

	return (c);
}

#endif

/* helper function to identify the com ports used by
 console or KGDB (and not yet autoconf attached) */
int
sacom_is_console(iot, iobase, ioh)
	bus_space_tag_t iot;
	bus_addr_t iobase;
	bus_space_handle_t *ioh;
{
	bus_space_handle_t help;

	if (! sacomconsattached &&
	    iot == sacomconstag && iobase == sacomconsaddr)
		help = sacomconsioh;
	else
		return (0);

	if (ioh)
		*ioh = help;
	return (1);
}
