/*	$NetBSD: ixp425_com.c,v 1.2 2003/05/23 10:14:03 ichiro Exp $ */
/*
 * Copyright (c) 2003
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.
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
 *	This product includes software developed by Ichiro FUKUHARA.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ixp425_com.c,v 1.2 2003/05/23 10:14:03 ichiro Exp $");

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

#include <arm/xscale/ixp425reg.h>
#include <arm/xscale/ixp425_comvar.h>

#include <dev/cons.h>

#include "ixpcom.h"

static int	ixp4xx_comparam(struct tty *, struct termios *);
static void	ixp4xx_comstart(struct tty *);
static int	ixp4xx_comhwiflow(struct tty *, int);

static u_int	cflag2lcr(tcflag_t);
static void	ixp4xx_com_modem(struct ixp4xx_com_softc *, int);
static void	ixp4xx_com_iflush(struct ixp4xx_com_softc *);
static void	tiocm_to_ixp4xx_com(struct ixp4xx_com_softc *, u_long, int);
static int	ixp4xx_com_to_tiocm(struct ixp4xx_com_softc *);

static void	ixp4xx_com_set_cr(struct ixp4xx_com_softc *);

static void	ixp4xxcomsoft(void *);
inline static void	ixp4xx_com_txsoft(struct ixp4xx_com_softc *, struct tty *);
inline static void	ixp4xx_com_rxsoft(struct ixp4xx_com_softc *, struct tty *);

int		ixp4xx_cominit(bus_space_tag_t, bus_space_handle_t, int, int, tcflag_t);

int             ixp4xx_comcngetc(dev_t);
void            ixp4xx_comcnputc(dev_t, int);
void            ixp4xx_comcnpollc(dev_t, int);
void            ixp4xx_comcnprobe(struct consdev *);
void            ixp4xx_comcninit(struct consdev *);

static struct ixp4xx_com_cons_softc {
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_addr_t		sc_baseaddr;
	int			sc_ospeed;
	tcflag_t		sc_cflag;
	int			sc_attached;
} ixp4xx_comcn_sc;

static struct cnm_state ixp4xx_com_cnm_state;

struct ixp4xx_com_softc* ixp4xx_com_sc = NULL;

extern struct cfdriver ixpcom_cd;

dev_type_open(ixp4xx_comopen);
dev_type_close(ixp4xx_comclose);
dev_type_read(ixp4xx_comread);
dev_type_write(ixp4xx_comwrite);
dev_type_ioctl(ixp4xx_comioctl);
dev_type_stop(ixp4xx_comstop);
dev_type_tty(ixp4xx_comtty);
dev_type_poll(ixp4xx_compoll);

const struct cdevsw ixpcom_cdevsw = {
	ixp4xx_comopen, ixp4xx_comclose, ixp4xx_comread, ixp4xx_comwrite,
	ixp4xx_comioctl, ixp4xx_comstop, ixp4xx_comtty, ixp4xx_compoll,
	nommap, ttykqfilter, D_TTY
};

#define COMUNIT_MASK    0x7ffff
#define COMDIALOUT_MASK 0x80000

#define COMUNIT(x)      (minor(x) & COMUNIT_MASK)
#define COMDIALOUT(x)   (minor(x) & COMDIALOUT_MASK)

#define COM_ISALIVE(sc) ((sc)->enabled != 0 && \
			ISSET((sc)->sc_dev.dv_flags, DVF_ACTIVE))

#define COM_BARRIER(t, h, f) bus_space_barrier((t), (h), 0, COM_NPORTS, (f))

#define COM_LOCK(sc);
#define COM_UNLOCK(sc);

#ifndef COM_TOLERANCE
#define COM_TOLERANCE 30	/* XXX: baud rate tolerance, in 0.1% units */
#endif

#define SET(t, f)       (t) |= (f)
#define CLR(t, f)       (t) &= ~(f)
#define ISSET(t, f)     ((t) & (f))

#ifdef COM_DEBUG
int com_debug = 0;
#endif

int
ixp4xx_comspeed(long speed, long frequency)
{
#define divrnd(n, q)	(((n)*2/(q)+1)/2)	/* divide and round off */

	int x, err;

	if (speed <= 0)
		return (-1);
	x = divrnd(frequency / 16, speed);
	if (x <= 0)
		return (-1);
	err = divrnd(((quad_t)frequency) * 1000 / 16, speed * x) - 1000;
	if (err < 0)
		err = -err;
	if (err > COM_TOLERANCE)
		return (-1);
	return (x);
#undef  divrnd
}

#if XXX
static void
ixp4xx_enable_debugport(struct ixp4xx_com_softc *sc)
{
	int s;

	/* Turn on line break interrupt, set carrier. */
	s = splserial();
	COM_LOCK(sc);
	sc->sc_ier = IER_RAVIE | IER_UUE;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IXP425_UART_IER, sc->sc_ier);
	SET(sc->sc_mcr, MCR_DTR | MCR_RTS);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IXP425_UART_MCR, sc->sc_mcr);
	COM_UNLOCK(sc);
	splx(s);
}
#endif

void
ixp4xx_com_attach_subr(struct ixp4xx_com_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	struct tty *tp;

	ixp4xx_com_sc = sc;
#if 0
	callout_init(&sc->sc_diag_callout);
#endif
	/* Disable interrupts before configuring the device. */
	sc->sc_ier = IER_UUE;
	bus_space_write_4(iot, ioh, IXP425_UART_IER, sc->sc_ier);

	if (iot == ixp4xx_comcn_sc.sc_iot
		&& sc->sc_baseaddr == ixp4xx_comcn_sc.sc_baseaddr) {
		ixp4xx_comcn_sc.sc_attached = 1;

		/* Make sure the console is always "hardwired". */
		delay(20000);		/* wait for output to finish */
		SET(sc->sc_hwflags, COM_HW_CONSOLE);
		SET(sc->sc_swflags, TIOCFLAG_SOFTCAR);
	}
	bus_space_write_4(iot, ioh, IXP425_UART_FCR,
			  FCR_TRIGGER_8 | FCR_RESETTF | FCR_RESETRF | FCR_ENABLE);
#ifdef KGDB
	if (ISSET(sc->sc_hwflags, COM_HW_KGDB)) {
                ixp4xx_com_kgdb_attached = 1;
                printf("%s: kgdb\n", sc->sc_dev.dv_xname);
                ixp4xx_enable_debugport(sc);
                return;
        }
#endif

	tp = ttymalloc();
	tp->t_oproc = ixp4xx_comstart;
	tp->t_param = ixp4xx_comparam;
	tp->t_hwiflow = ixp4xx_comhwiflow;

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

	tty_attach(tp);

	if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
		int maj;

		/* locate the major number */
		maj = cdevsw_lookup_major(&ixpcom_cdevsw);
		tp->t_dev = cn_tab->cn_dev = makedev(maj, sc->sc_dev.dv_unit);
		printf("%s: console (major=%d)\n", sc->sc_dev.dv_xname, maj);
	}

	sc->sc_si = softintr_establish(IPL_SOFTSERIAL, ixp4xxcomsoft, sc);

#if NRND > 0 && defined(RND_COM)
	rnd_attach_source(&sc->rnd_source, sc->sc_dev.dv_xname,
			  RND_TYPE_TTY, 0);
#endif

	/* if there are no enable/disable functions, assume the device
	   is always enabled */
	if (!sc->enable)
		sc->enabled = 1;

        SET(sc->sc_hwflags, COM_HW_DEV_OK);
}

static int
ixp4xx_comparam(struct tty *tp, struct termios *t)
{
	struct ixp4xx_com_softc *sc =
		device_lookup(&ixpcom_cd, COMUNIT(tp->t_dev));
        int ospeed;
        u_char lcr;
        int s;

#ifdef DEBUG
	printf("comparam()\n");
#endif

	ospeed = ixp4xx_comspeed(t->c_ospeed, sc->sc_frequency);

	/* Check requested parameters. */
	if (ospeed < 0)
		return EINVAL;
	if (t->c_ispeed && t->c_ispeed != t->c_ospeed)
		return EINVAL;

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

	lcr = ISSET(sc->sc_lcr, LCR_SBREAK) | cflag2lcr(t->c_cflag);

	s = splserial();
	COM_LOCK(sc);

	sc->sc_lcr = lcr;

        /*
         * If we're not in a mode that assumes a connection is present, then
         * ignore carrier changes.
         */
        if (ISSET(t->c_cflag, CLOCAL | MDMBUF))
                sc->sc_msr_dcd = 0;
        else
                sc->sc_msr_dcd = MSR_DCD;
        /*
         * Set the flow control pins depending on the current flow control
         * mode.
         */
        if (ISSET(t->c_cflag, CRTSCTS)) {
                sc->sc_mcr_dtr = MCR_DTR;
                sc->sc_mcr_rts = MCR_RTS;
                sc->sc_msr_cts = MSR_CTS;
        } else if (ISSET(t->c_cflag, MDMBUF)) {
                /*
                 * For DTR/DCD flow control, make sure we don't toggle DTR for
                 * carrier detection.
                 */
                sc->sc_mcr_dtr = 0;
                sc->sc_mcr_rts = MCR_DTR;
                sc->sc_msr_cts = MSR_DCD;
        } else {
                /*
                 * If no flow control, then always set RTS.  This will make
                 * the other side happy if it mistakenly thinks we're doing
                 * RTS/CTS flow control.
                 */
                sc->sc_mcr_dtr = MCR_DTR | MCR_RTS;
                sc->sc_mcr_rts = 0;
                sc->sc_msr_cts = 0;
                if (ISSET(sc->sc_mcr, MCR_DTR))
                        SET(sc->sc_mcr, MCR_RTS);
                else
                        CLR(sc->sc_mcr, MCR_RTS);
        }
        sc->sc_msr_mask = sc->sc_msr_cts | sc->sc_msr_dcd;

	sc->sc_dlbl = ospeed;
	sc->sc_dlbh = ospeed >> 8;

        /*
         * Set the FIFO threshold based on the receive speed.
         *
         *  * If it's a low speed, it's probably a mouse or some other
         *    interactive device, so set the threshold low.
         *  * If it's a high speed, trim the trigger level down to prevent
         *    overflows.
         *  * Otherwise set it a bit higher.
         */
#if NOTYET
	if (ISSET(sc->sc_hwflags, COM_HW_FIFO))
		sc->sc_fcr = FCR_ENABLE |
			(t->c_ospeed <= 1200 ? FCR_TRIGGER_1 :
			 t->c_ospeed <= 38400 ? FCR_TRIGGER_16 : FCR_TRIGGER_8);
	else
		sc->sc_fcr = 0;
#endif

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
			ixp4xx_com_set_cr(sc);
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
			ixp4xx_comstart(tp);
		}
	}

	return (0);
}

void
ixp4xx_com_set_cr(struct ixp4xx_com_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
#ifdef DEBUG
        printf("com_set_cr()\n");
#endif

	ixp4xx_com_iflush(sc);

	bus_space_write_4(iot, ioh, IXP425_UART_IER, IER_UUE);

	bus_space_write_4(iot, ioh, IXP425_UART_LCR, sc->sc_lcr | LCR_DLAB);
	bus_space_write_4(iot, ioh, IXP425_UART_DLL, sc->sc_dlbl);
	bus_space_write_4(iot, ioh, IXP425_UART_DLH, sc->sc_dlbh);
	bus_space_write_4(iot, ioh, IXP425_UART_LCR, sc->sc_lcr);
	bus_space_write_4(iot, ioh, IXP425_UART_MCR, sc->sc_mcr_active = sc->sc_mcr);
	bus_space_write_4(iot, ioh, IXP425_UART_FCR, sc->sc_fcr);
	bus_space_write_4(iot, ioh, IXP425_UART_IER, sc->sc_ier);
}
 
static int
ixp4xx_comhwiflow(struct tty *tp, int block)
{
#ifdef DEBUG
        printf("comhwiflow()\n");
#endif

        return (0);
}

static void
ixp4xx_com_filltx(struct ixp4xx_com_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int n;
#ifdef DEBUG
        printf("comfilltx()\n");
#endif

	n = 0;
	while (bus_space_read_4(iot, ioh, IXP425_UART_LSR) & LSR_TDRQ) {
		if (n >= sc->sc_tbc)
			break;
		bus_space_write_4(iot, ioh, IXP425_UART_DATA,
				  0xff & *(sc->sc_tba + n));
		n++;
	}
	sc->sc_tbc -= n;
}

static void
ixp4xx_comstart(struct tty *tp)
{
	struct ixp4xx_com_softc *sc
		= device_lookup(&ixpcom_cd, COMUNIT(tp->t_dev));
	int s;

#ifdef DEBUG
        printf("comstart()\n");
#endif

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
	if (!ISSET(sc->sc_ier, IER_TIE)) {
		SET(sc->sc_ier, IER_TIE);
                ixp4xx_com_set_cr(sc);
	}

	/* Output the first chunk of the contiguous buffer. */
	ixp4xx_com_filltx(sc);

	COM_UNLOCK(sc);
out:
	splx(s);
	return;
}

static void
ixp4xx_com_break(struct ixp4xx_com_softc *sc, int onoff)
{
#ifdef DEBUG
        printf("combreak()\n");
#endif
	if (onoff)
		SET(sc->sc_lcr, LCR_SBREAK);
	else
		CLR(sc->sc_lcr, LCR_SBREAK);

	if (!sc->sc_heldchange) {
		if (sc->sc_tx_busy) {
			sc->sc_heldtbc = sc->sc_tbc;
			sc->sc_tbc = 0;
			sc->sc_heldchange = 1;
		} else
			ixp4xx_com_set_cr(sc);
	}
}

static void
ixp4xx_com_shutdown(struct ixp4xx_com_softc *sc)
{
	struct tty *tp = sc->sc_tty;
	int s;
#ifdef DEBUG
        printf("comshutdown()\n");
#endif

	s = splserial();
	COM_LOCK(sc);

	/* Clear any break condition set with TIOCSBRK. */
	ixp4xx_com_break(sc, 0);

	/*
	 * Hang up if necessary.  Wait a bit, so the other side has time to
	 * notice even if we immediately open the port again.
	 * Avoid tsleeping above splhigh().
	 */
	if (ISSET(tp->t_cflag, HUPCL)) {
		ixp4xx_com_modem(sc, 0);
		COM_UNLOCK(sc);
		splx(s);
		/* XXX tsleep will only timeout */
		(void) tsleep(sc, TTIPRI, ttclos, hz);
		s = splserial();
		COM_LOCK(sc);
	}

	/* Turn off interrupts. */
	sc->sc_ier &= ~(IER_RAVIE | IER_TIE);
	ixp4xx_com_set_cr(sc);

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
ixp4xx_comopen(dev, flag, mode, p)
        dev_t dev;
        int flag, mode;
        struct proc *p;
{
	struct ixp4xx_com_softc *sc;
	struct tty *tp;
	int s, s2;
	int error;
#ifdef DEBUG
        printf("comopen()\n");
#endif

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
		}
		/* Turn on interrupts. */
		sc->sc_ier = IER_RAVIE | IER_RLSE | IER_RIE;
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
				  IXP425_UART_IER, sc->sc_ier);

		/* Fetch the current modem control status, needed later. */
		sc->sc_msr = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
					      IXP425_UART_MSR);

		COM_UNLOCK(sc);
		splx(s2);

		/*
                 * Initialize the termios status to the defaults.  Add in the
                 * sticky bits from TIOCSFLAGS.
                 */
                t.c_ispeed = 0;
                if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
                        t.c_ospeed = ixp4xx_comcn_sc.sc_ospeed;
                        t.c_cflag = ixp4xx_comcn_sc.sc_cflag;
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
                (void) ixp4xx_comparam(tp, &t);
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
		ixp4xx_com_modem(sc, 1);
                /* Clear the input ring, and unblock. */
		sc->sc_rbput = sc->sc_rbget = sc->sc_rbuf;
		sc->sc_rbavail = IXPCOM_RING_SIZE;
		ixp4xx_com_iflush(sc);
		CLR(sc->sc_rx_flags, RX_ANY_BLOCK);

#ifdef COM_DEBUG
		if (com_debug)
			comstatus(sc, "ixp4xx_comopen  ");
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
		ixp4xx_com_shutdown(sc);
	}
	return (error);
}

int
ixp4xx_comclose(dev, flag, mode, p)
        dev_t dev;
        int flag, mode;
        struct proc *p;
{
	struct ixp4xx_com_softc *sc =
		device_lookup(&ixpcom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

#ifdef DEBUG
        printf("comclose()\n");
#endif
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
		ixp4xx_com_shutdown(sc);
	}

	return (0);
}

int
ixp4xx_comread(dev, uio, flag)
        dev_t dev;
        struct uio *uio;
        int flag;
{
	struct ixp4xx_com_softc *sc =
		device_lookup(&ixpcom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;
#ifdef DEBUG
        printf("comread()\n");
#endif

	if (COM_ISALIVE(sc) == 0)
		return (EIO);

	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}

int
ixp4xx_comwrite(dev, uio, flag)
        dev_t dev;
        struct uio *uio;
        int flag;
{
	struct ixp4xx_com_softc *sc =
		device_lookup(&ixpcom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

#ifdef DEBUG
        printf("comwrite()\n");
#endif
	if (COM_ISALIVE(sc) == 0)
		return (EIO);

	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
ixp4xx_compoll(dev, events, p)
        dev_t dev;
        int events;
        struct proc *p;
{
	struct ixp4xx_com_softc *sc =
		device_lookup(&ixpcom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

#ifdef DEBUG
        printf("compoll()\n");
#endif
	if (COM_ISALIVE(sc) == 0)
		return (EIO);

	return ((*tp->t_linesw->l_poll)(tp, events, p));
}

struct tty *
ixp4xx_comtty(dev_t dev)
{
	struct ixp4xx_com_softc *sc =
		device_lookup(&ixpcom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

#ifdef DEBUG
        printf("comtty()\n");
#endif
	return (tp);
}

int
ixp4xx_comioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct ixp4xx_com_softc *sc =
		device_lookup(&ixpcom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;
	int error;
	int s;

#ifdef DEBUG
        printf("comioctl()\n");
#endif
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
		ixp4xx_com_break(sc, 1);
		break;

	case TIOCCBRK:
		ixp4xx_com_break(sc, 0);
		break;

	case TIOCSDTR:
		ixp4xx_com_modem(sc, 1);
		break;

	case TIOCCDTR:
		ixp4xx_com_modem(sc, 0);
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
		tiocm_to_ixp4xx_com(sc, cmd, *(int *)data);
		break;

	case TIOCMGET:
		*(int *)data = ixp4xx_com_to_tiocm(sc);
		break;

	default:
		error = EPASSTHROUGH;
		break;
	}

	COM_UNLOCK(sc);
	splx(s);

#ifdef COM_DEBUG
	if (com_debug)
		comstatus(sc, "comioctl ");
#endif

	return (error);
}

void
ixp4xx_comstop(struct tty *tp, int flag)
{
	struct ixp4xx_com_softc *sc =
		device_lookup(&ixpcom_cd, COMUNIT(tp->t_dev));
	int s;

#ifdef DEBUG
        printf("comstop()\n");
#endif

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

static void
ixp4xx_com_modem(struct ixp4xx_com_softc *sc, int onoff)
{

	if (sc->sc_mcr_dtr == 0)
		return;

	if (onoff)
		SET(sc->sc_mcr, sc->sc_mcr_dtr);
	else
		CLR(sc->sc_mcr, sc->sc_mcr_dtr);

	if (!sc->sc_heldchange) {
		if (sc->sc_tx_busy) {
			sc->sc_heldtbc = sc->sc_tbc;
			sc->sc_tbc = 0;
			sc->sc_heldchange = 1;
		} else
			ixp4xx_com_set_cr(sc);
	}
}

static void
tiocm_to_ixp4xx_com(struct ixp4xx_com_softc *sc, u_long how, int ttybits)
{
	u_char combits;

	combits = 0;
	if (ISSET(ttybits, TIOCM_DTR))
		SET(combits, MCR_DTR);
	if (ISSET(ttybits, TIOCM_RTS))
		SET(combits, MCR_RTS);

	switch (how) {
	case TIOCMBIC:
		CLR(sc->sc_mcr, combits);
		break;

	case TIOCMBIS:
		SET(sc->sc_mcr, combits);
		break;

	case TIOCMSET:
		CLR(sc->sc_mcr, MCR_DTR | MCR_RTS);
		SET(sc->sc_mcr, combits);
		break;
	}

	if (!sc->sc_heldchange) {
		if (sc->sc_tx_busy) {
			sc->sc_heldtbc = sc->sc_tbc;
			sc->sc_tbc = 0;
			sc->sc_heldchange = 1;
		} else
			ixp4xx_com_set_cr(sc);
	}
}

static int
ixp4xx_com_to_tiocm(struct ixp4xx_com_softc *sc)
{
	u_char combits;
	int ttybits = 0;

	combits = sc->sc_mcr;
	if (ISSET(combits, MCR_DTR))
		SET(ttybits, TIOCM_DTR);
	if (ISSET(combits, MCR_RTS))
		SET(ttybits, TIOCM_RTS);

	combits = sc->sc_msr;
	if (ISSET(combits, MSR_DCD))
		SET(ttybits, TIOCM_CD);
	if (ISSET(combits, MSR_CTS))
		SET(ttybits, TIOCM_CTS);
	if (ISSET(combits, MSR_DSR))
		SET(ttybits, TIOCM_DSR);
	if (ISSET(combits, MSR_RI | MSR_TERI))
		SET(ttybits, TIOCM_RI);

	if ((sc->sc_ier & IER_UUE) != 0)
		SET(ttybits, TIOCM_LE);

	return (ttybits);
}

static u_int
cflag2lcr(tcflag_t cflag)
{
	u_char lcr = 0;

	switch (ISSET(cflag, CSIZE)) {
	case CS5:
		SET(lcr, LCR_5BITS);
		break;
	case CS6:
		SET(lcr, LCR_6BITS);
		break;
	case CS7:
		SET(lcr, LCR_7BITS);
		break;
	case CS8:
		SET(lcr, LCR_8BITS);
		break;
	}
	if (ISSET(cflag, PARENB)) {
		SET(lcr, LCR_PENE);
		if (!ISSET(cflag, PARODD))
			SET(lcr, LCR_PODD);
	}
	if (ISSET(cflag, CSTOPB))
		SET(lcr, LCR_1STOP);

	return (lcr);
}

static void
ixp4xx_com_iflush(struct ixp4xx_com_softc *sc)
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
	while (ISSET(bus_space_read_4(iot, ioh, IXP425_UART_LSR), LSR_DR)
		&& --timo)
#ifdef DIAGNOSTIC
		reg =
#else
			(void)
#endif
			bus_space_read_4(iot, ioh, IXP425_UART_DATA);
#ifdef DIAGNOSTIC
	if (!timo)
		printf("%s: com_iflush timeout %02x\n", sc->sc_dev.dv_xname,
			reg);
#endif
}

static void
ixp4xxcomsoft(void* arg)
{
	struct ixp4xx_com_softc *sc = arg;

	if (COM_ISALIVE(sc) == 0)
		return;

	if (sc->sc_rx_ready) {
		sc->sc_rx_ready = 0;
		ixp4xx_com_rxsoft(sc, sc->sc_tty);
	}
	if (sc->sc_tx_done) {
		sc->sc_tx_done = 0;
		ixp4xx_com_txsoft(sc, sc->sc_tty);
	}
}

inline static void
ixp4xx_com_txsoft(struct ixp4xx_com_softc *sc, struct tty *tp)
{
	CLR(tp->t_state, TS_BUSY);
	if (ISSET(tp->t_state, TS_FLUSH))
		CLR(tp->t_state, TS_FLUSH);
	else
		ndflush(&tp->t_outq, (int)(sc->sc_tba - tp->t_outq.c_cf));
	(*tp->t_linesw->l_start)(tp);
}

inline static void
ixp4xx_com_rxsoft(struct ixp4xx_com_softc *sc, struct tty *tp)
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
	while (cc) {
		code = get[0];
		lsr = get[1];
		if (ISSET(lsr, LSR_OE | LSR_FE | LSR_PE)) {
			if (ISSET(lsr, LSR_FE))
				SET(code, TTY_FE);
			if (ISSET(lsr, LSR_PE))
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
				SET(sc->sc_ier,IER_RAVIE);
				ixp4xx_com_set_cr(sc);
			}
			if (ISSET(sc->sc_rx_flags, RX_IBUF_BLOCKED)) {
				CLR(sc->sc_rx_flags, RX_IBUF_BLOCKED);
			}
		}
		COM_UNLOCK(sc);
		splx(s);
	}
}

int
ixp4xxcomintr(void* arg)
{
	struct ixp4xx_com_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_char *put, *end;
	u_int cc, res;
	u_int32_t c;

	if (COM_ISALIVE(sc) == 0)
		return (0);

	COM_LOCK(sc);
	res = bus_space_read_4(iot, ioh, IXP425_UART_IER) & IER_UUE;

	if (!res) {
		COM_UNLOCK(sc);
		return (0);
	}

	res = bus_space_read_4(iot, ioh, IXP425_UART_LSR);
	if (!ISSET(res, LSR_DR | LSR_TDRQ))
		return (0);

	end = sc->sc_ebuf;
	put = sc->sc_rbput;
	cc = sc->sc_rbavail;

	if (ISSET(res, LSR_DR)) {
		if (!ISSET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED)) {
			while (cc > 0) {
				if (!ISSET(res, LSR_DR))
					break;
				c = bus_space_read_4(iot, ioh, IXP425_UART_DATA);
				if (ISSET(res, LSR_FE)) {
					cn_check_magic(sc->sc_tty->t_dev,
						       CNC_BREAK,
						       ixp4xx_com_cnm_state);
				}
				put[0] = c & 0xff;
				put[1] = (c >> 8) & 0xff;
				cn_check_magic(sc->sc_tty->t_dev,
					       put[0], ixp4xx_com_cnm_state);
				put += 2;
				if (put >= end)
					put = sc->sc_rbuf;
				cc--;
				res = bus_space_read_4(iot, ioh, IXP425_UART_LSR);
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
			/* later XXXX */

			/*
                         * If we're out of space, disable receive interrupts
                         * until the queue has drained a bit.
                         */
			if (!cc) {
				SET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED);
				CLR(sc->sc_ier, IER_RAVIE);
				ixp4xx_com_set_cr(sc);
			}
		} else {
#ifdef DIAGNOSTIC
			panic("ixpcomintr: we shouldn't reach here");
#endif
			CLR(sc->sc_ier, IER_RAVIE);
			ixp4xx_com_set_cr(sc);
		}
	}

	/*
	 * Done handling any receive interrupts. See if data can be
	 * transmitted as well. Schedule tx done event if no data left
	 * and tty was marked busy.
	 */

	res = bus_space_read_4(iot, ioh, IXP425_UART_LSR);
	if (ISSET(res, LSR_TDRQ)) {
		/*
		 * If we've delayed a parameter change, do it now, and restart
		 * output.
		 */
		if (sc->sc_heldchange) {
			ixp4xx_com_set_cr(sc);
			sc->sc_heldchange = 0;
			sc->sc_tbc = sc->sc_heldtbc;
			sc->sc_heldtbc = 0;
		}

		/* Output the next chunk of the contiguous buffer, if any. */
		if (sc->sc_tbc > 0) {
			ixp4xx_com_filltx(sc);
		} else {
			/* Disable transmit completion interrupts if necessary. */
			if (ISSET(sc->sc_ier, IER_TIE)) {
				CLR(sc->sc_ier, IER_TIE);
				ixp4xx_com_filltx(sc);
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

/*
 * Initialize UART for use as console 
 */
int
ixp4xx_cominit(bus_space_tag_t iot, bus_space_handle_t ioh, int rate,
	int frequency, tcflag_t cflag)
{
	bus_space_write_4(iot, ioh, IXP425_UART_IER, IER_UUE);

	bus_space_write_4(iot, ioh, IXP425_UART_LCR, LCR_DLAB);
	rate = ixp4xx_comspeed(rate, frequency);
	bus_space_write_4(iot, ioh, IXP425_UART_DLL, rate);
	bus_space_write_4(iot, ioh, IXP425_UART_DLH, rate >> 8);
	bus_space_write_4(iot, ioh, IXP425_UART_LCR, cflag2lcr(cflag));
	bus_space_write_4(iot, ioh, IXP425_UART_MCR, MCR_DTR | MCR_RTS);
	bus_space_write_4(iot, ioh, IXP425_UART_FCR,
		FCR_ENABLE | FCR_RESETRF | FCR_RESETTF | FCR_TRIGGER_1);

	return (0);
}


/*
 * There are routines needed to act as console
 */

struct consdev ixp4xx_comcons = {
        NULL, NULL, ixp4xx_comcngetc, ixp4xx_comcnputc, ixp4xx_comcnpollc,
	NULL, NULL, NULL, NODEV, CN_NORMAL
};

int
ixp4xx_comcnattach(bus_space_tag_t iot, bus_addr_t iobase, bus_space_handle_t ioh,
	int rate, int frequency, tcflag_t cflag)
{
	int res;

	res = ixp4xx_cominit(iot, ioh, rate, frequency, cflag);

	if (res)
		return (res);

	cn_tab = &ixp4xx_comcons;
	cn_init_magic(&ixp4xx_com_cnm_state);

	/* default magic is a couple of BREAK. */
        cn_set_magic("\047\001\047\001");

	ixp4xx_comcn_sc.sc_iot = iot;
	ixp4xx_comcn_sc.sc_ioh = ioh;
	ixp4xx_comcn_sc.sc_baseaddr = iobase;
	ixp4xx_comcn_sc.sc_ospeed = rate;
	ixp4xx_comcn_sc.sc_cflag = cflag;

	return (0);
}

void
ixp4xx_comcnputc(dev, c)
        dev_t dev;
        int c;
{
        bus_space_tag_t         iot = ixp4xx_comcn_sc.sc_iot;
	bus_space_handle_t      ioh = ixp4xx_comcn_sc.sc_ioh;

	while(!(bus_space_read_4(iot, ioh, 0x14) & 0x20))
		;

        bus_space_write_4(iot, ioh, 0x0, c);
}

int
ixp4xx_comcngetc(dev)
        dev_t dev;
{
	int c;
        bus_space_tag_t         iot = ixp4xx_comcn_sc.sc_iot;
        bus_space_handle_t      ioh = ixp4xx_comcn_sc.sc_ioh;

        while(!(bus_space_read_4(iot, ioh, 0x14) & 0x01))
                ;
	c = bus_space_read_4(iot, ioh, 0x0);
	c &= 0xff;

	return (c);
}


void
ixp4xx_comcnprobe(cp)
        struct consdev *cp;
{
        cp->cn_pri = CN_REMOTE;
}

void
ixp4xx_comcnpollc(dev, on)
        dev_t dev;
        int on;
{
}
