/*	$NetBSD: com.c,v 1.101 1997/06/15 11:18:59 mycroft Exp $	*/

/*-
 * Copyright (c) 1993, 1994, 1995, 1996, 1997
 *	Charles M. Hannum.  All rights reserved.
 *
 * Interrupt processing and hardware flow control partly based on code from
 * Onno van der Linden and Gordon Ross.
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
 *	This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

/*
 * COM driver, uses National Semiconductor NS16450/NS16550AF UART
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/device.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/isa/comreg.h>
#include <dev/isa/comvar.h>
#include <dev/ic/ns16550reg.h>
#ifdef COM_HAYESP
#include <dev/ic/hayespreg.h>
#endif
#define	com_lcr	com_cfcr

#include "com.h"

#ifdef COM_HAYESP
int comprobeHAYESP __P((bus_space_handle_t hayespioh, struct com_softc *sc));
#endif
#ifdef KGDB
void com_kgdb_attach __P((struct com_softc *, bus_space_tag_t,
			  bus_space_handle_t));
#endif
void	com_attach_subr	__P((struct com_softc *sc));
void	comdiag		__P((void *));
int	comspeed	__P((long));
int	comparam	__P((struct tty *, struct termios *));
void	comstart	__P((struct tty *));
void	comstop		__P((struct tty *, int));
#ifdef __GENERIC_SOFT_INTERRUPTS
void 	comsoft		__P((void *));
#else
#ifndef alpha
void 	comsoft		__P((void));
#else
void 	comsoft		__P((void *));
#endif
#endif
int	comhwiflow	__P((struct tty *, int));

void	com_loadchannelregs __P((struct com_softc *));
void	com_hwiflow	__P((struct com_softc *));
void	com_break	__P((struct com_softc *, int));
void	com_modem	__P((struct com_softc *, int));
void	com_iflush	__P((struct com_softc *));

/* XXX: These belong elsewhere */
cdev_decl(com);
bdev_decl(com);

struct consdev;
void	comcnprobe	__P((struct consdev *));
void	comcninit	__P((struct consdev *));
int	comcngetc	__P((dev_t));
void	comcnputc	__P((dev_t, int));
void	comcnpollc	__P((dev_t, int));

#define	integrate	static inline
integrate void comrxint		__P((struct com_softc *, struct tty *));
integrate void comtxint		__P((struct com_softc *, struct tty *));
integrate void commsrint	__P((struct com_softc *, struct tty *));
integrate void com_schedrx	__P((struct com_softc *));

struct cfdriver com_cd = {
	NULL, "com", DV_TTY
};

void cominitcons 	__P((bus_space_tag_t, bus_space_handle_t, int));

#ifdef CONSPEED
int	comconsrate = CONSPEED;
#else
int	comconsrate = TTYDEF_SPEED;
#endif
int	comconsaddr;
int	comconsattached;
bus_space_tag_t comconstag;
bus_space_handle_t comconsioh;
tcflag_t comconscflag = TTYDEF_CFLAG;

int	commajor;

#ifndef __GENERIC_SOFT_INTERRUPTS
#ifdef alpha
volatile int	com_softintr_scheduled;
#endif
#endif

#ifdef KGDB
#include <machine/remote-sl.h>
extern int kgdb_dev;
extern int kgdb_rate;
extern int kgdb_debug_init;
#endif

#define	COMUNIT(x)	(minor(x))

int
comspeed(speed)
	long speed;
{
#define	divrnd(n, q)	(((n)*2/(q)+1)/2)	/* divide and round off */

	int x, err;

#if 0
	if (speed == 0)
		return (0);
#endif
	if (speed <= 0)
		return (-1);
	x = divrnd((COM_FREQ / 16), speed);
	if (x <= 0)
		return (-1);
	err = divrnd((COM_FREQ / 16) * 1000, speed * x) - 1000;
	if (err < 0)
		err = -err;
	if (err > COM_TOLERANCE)
		return (-1);
	return (x);

#undef	divrnd(n, q)
}

#ifdef COM_DEBUG
int	com_debug = 0;

void comstatus __P((struct com_softc *, char *));
void
comstatus(sc, str)
	struct com_softc *sc;
	char *str;
{
	struct tty *tp = sc->sc_tty;

	printf("%s: %s %sclocal  %sdcd %sts_carr_on %sdtr %stx_stopped\n",
	    sc->sc_dev.dv_xname, str,
	    ISSET(tp->t_cflag, CLOCAL) ? "+" : "-",
	    ISSET(sc->sc_msr, MSR_DCD) ? "+" : "-",
	    ISSET(tp->t_state, TS_CARR_ON) ? "+" : "-",
	    ISSET(sc->sc_mcr, MCR_DTR) ? "+" : "-",
	    sc->sc_tx_stopped ? "+" : "-");

	printf("%s: %s %scrtscts %scts %sts_ttstop  %srts %xrx_flags\n",
	    sc->sc_dev.dv_xname, str,
	    ISSET(tp->t_cflag, CRTSCTS) ? "+" : "-",
	    ISSET(sc->sc_msr, MSR_CTS) ? "+" : "-",
	    ISSET(tp->t_state, TS_TTSTOP) ? "+" : "-",
	    ISSET(sc->sc_mcr, MCR_RTS) ? "+" : "-",
	    sc->sc_rx_flags);
}
#endif

int
comprobe1(iot, ioh, iobase)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int iobase;
{

	/* force access to id reg */
	bus_space_write_1(iot, ioh, com_lcr, 0);
	bus_space_write_1(iot, ioh, com_iir, 0);
	if (bus_space_read_1(iot, ioh, com_iir) & 0x38)
		return (0);

	return (1);
}

#ifdef COM_HAYESP
int
comprobeHAYESP(hayespioh, sc)
	bus_space_handle_t hayespioh;
	struct com_softc *sc;
{
	char	val, dips;
	int	combaselist[] = { 0x3f8, 0x2f8, 0x3e8, 0x2e8 };
	bus_space_tag_t iot = sc->sc_iot;

	/*
	 * Hayes ESP cards have two iobases.  One is for compatibility with
	 * 16550 serial chips, and at the same ISA PC base addresses.  The
	 * other is for ESP-specific enhanced features, and lies at a
	 * different addressing range entirely (0x140, 0x180, 0x280, or 0x300).
	 */

	/* Test for ESP signature */
	if ((bus_space_read_1(iot, hayespioh, 0) & 0xf3) == 0)
		return (0);

	/*
	 * ESP is present at ESP enhanced base address; unknown com port
	 */

	/* Get the dip-switch configurations */
	bus_space_write_1(iot, hayespioh, HAYESP_CMD1, HAYESP_GETDIPS);
	dips = bus_space_read_1(iot, hayespioh, HAYESP_STATUS1);

	/* Determine which com port this ESP card services: bits 0,1 of  */
	/*  dips is the port # (0-3); combaselist[val] is the com_iobase */
	if (sc->sc_iobase != combaselist[dips & 0x03])
		return (0);

	printf(": ESP");

 	/* Check ESP Self Test bits. */
	/* Check for ESP version 2.0: bits 4,5,6 == 010 */
	bus_space_write_1(iot, hayespioh, HAYESP_CMD1, HAYESP_GETTEST);
	val = bus_space_read_1(iot, hayespioh, HAYESP_STATUS1); /* Clear reg1 */
	val = bus_space_read_1(iot, hayespioh, HAYESP_STATUS2);
	if ((val & 0x70) < 0x20) {
		printf("-old (%o)", val & 0x70);
		/* we do not support the necessary features */
		return (0);
	}

	/* Check for ability to emulate 16550: bit 8 == 1 */
	if ((dips & 0x80) == 0) {
		printf(" slave");
		/* XXX Does slave really mean no 16550 support?? */
		return (0);
	}

	/*
	 * If we made it this far, we are a full-featured ESP v2.0 (or
	 * better), at the correct com port address.
	 */

	SET(sc->sc_hwflags, COM_HW_HAYESP);
	printf(", 1024 byte fifo\n");
	return (1);
}
#endif

#ifdef KGDB
void
com_kgdb_attach(sc, iot, ioh)
	struct com_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
{

	if (kgdb_dev == makedev(commajor, unit)) {
		if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE))
			kgdb_dev = -1;	/* can't debug over console port */
		else {
			cominitcons(iot, ioh, kgdb_rate);
			if (kgdb_debug_init) {
				/*
				 * Print prefix of device name,
				 * let kgdb_connect print the rest.
				 */
				printf("%s: ", sc->sc_dev.dv_xname);
				kgdb_connect(1);
			} else
				printf("%s: kgdb enabled\n",
				    sc->sc_dev.dv_xname);
		}
	}
}
#endif


void
com_attach_subr(sc)
	struct com_softc *sc;
{
	int iobase = sc->sc_iobase;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
#ifdef COM_HAYESP
	int	hayesp_ports[] = { 0x140, 0x180, 0x280, 0x300, 0 };
	int	*hayespp;
#endif

	if (iobase == comconsaddr) {
		comconsattached = 1;

		/* Make sure the console is always "hardwired". */
		delay(1000);			/* wait for output to finish */
		SET(sc->sc_hwflags, COM_HW_CONSOLE);
		SET(sc->sc_swflags, TIOCFLAG_SOFTCAR);
	}

#ifdef COM_HAYESP
	/* Look for a Hayes ESP board. */
	for (hayespp = hayesp_ports; *hayespp != 0; hayespp++) {
		bus_space_handle_t hayespioh;

#define	HAYESP_NPORTS	8			/* XXX XXX XXX ??? ??? ??? */
		if (bus_space_map(iot, *hayespp, HAYESP_NPORTS, 0, &hayespioh))
			continue;
		if (comprobeHAYESP(hayespioh, sc)) {
			sc->sc_hayespioh = hayespioh;
			sc->sc_fifolen = 1024;

			/* Set 16550 compatibility mode */
			bus_space_write_1(iot, hayespioh, HAYESP_CMD1, HAYESP_SETMODE);
			bus_space_write_1(iot, hayespioh, HAYESP_CMD2, 
			     HAYESP_MODE_FIFO|HAYESP_MODE_RTS|
			     HAYESP_MODE_SCALE);

			/* Set RTS/CTS flow control */
			bus_space_write_1(iot, hayespioh, HAYESP_CMD1, HAYESP_SETFLOWTYPE);
			bus_space_write_1(iot, hayespioh, HAYESP_CMD2, HAYESP_FLOW_RTS);
			bus_space_write_1(iot, hayespioh, HAYESP_CMD2, HAYESP_FLOW_CTS);

			/* Set flow control levels */
			bus_space_write_1(iot, hayespioh, HAYESP_CMD1, HAYESP_SETRXFLOW);
			bus_space_write_1(iot, hayespioh, HAYESP_CMD2, 
			     HAYESP_HIBYTE(HAYESP_RXHIWMARK));
			bus_space_write_1(iot, hayespioh, HAYESP_CMD2,
			     HAYESP_LOBYTE(HAYESP_RXHIWMARK));
			bus_space_write_1(iot, hayespioh, HAYESP_CMD2,
			     HAYESP_HIBYTE(HAYESP_RXLOWMARK));
			bus_space_write_1(iot, hayespioh, HAYESP_CMD2,
			     HAYESP_LOBYTE(HAYESP_RXLOWMARK));

			break;
		}
		bus_space_unmap(iot, hayespioh, HAYESP_NPORTS);
	}
	/* No ESP; look for other things. */
	if (*hayespp == 0) {
#endif

	sc->sc_fifolen = 1;
	/* look for a NS 16550AF UART with FIFOs */
	bus_space_write_1(iot, ioh, com_fifo,
	    FIFO_ENABLE | FIFO_RCV_RST | FIFO_XMT_RST | FIFO_TRIGGER_14);
	delay(100);
	if (ISSET(bus_space_read_1(iot, ioh, com_iir), IIR_FIFO_MASK)
	    == IIR_FIFO_MASK)
		if (ISSET(bus_space_read_1(iot, ioh, com_fifo), FIFO_TRIGGER_14)
		    == FIFO_TRIGGER_14) {
			SET(sc->sc_hwflags, COM_HW_FIFO);
			printf(": ns16550a, working fifo\n");
			sc->sc_fifolen = 16;
		} else
			printf(": ns16550, broken fifo\n");
	else
		printf(": ns8250 or ns16450, no fifo\n");
	bus_space_write_1(iot, ioh, com_fifo, 0);
#ifdef COM_HAYESP
	}
#endif

	if (!ISSET(sc->sc_hwflags, COM_HW_NOIEN))
		SET(sc->sc_mcr, MCR_IENABLE);

	if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
		cominit(iot, ioh, comconsrate);
		printf("%s: console\n", sc->sc_dev.dv_xname);
	}

#ifdef KGDB
	com_kgdb_attach(sc, iot, ioh);
#endif

#ifdef __GENERIC_SOFT_INTERRUPTS
	sc->sc_si = softintr_establish(IPL_SOFTSERIAL, comsoft, sc);
#endif
}

int
comopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	int unit = COMUNIT(dev);
	struct com_softc *sc;
	struct tty *tp;
	int s, s2;
	int error = 0;
 
	if (unit >= com_cd.cd_ndevs)
		return (ENXIO);
	sc = com_cd.cd_devs[unit];
	if (!sc)
		return (ENXIO);

	if (!sc->sc_tty) {
		tp = sc->sc_tty = ttymalloc();
		tty_attach(tp);
	} else
		tp = sc->sc_tty;

	if (ISSET(tp->t_state, TS_ISOPEN) &&
	    ISSET(tp->t_state, TS_XCLUDE) &&
	    p->p_ucred->cr_uid != 0)
		return (EBUSY);

	s = spltty();

	/* We need to set this early for the benefit of comsoft(). */
	SET(tp->t_state, TS_WOPEN);

	/*
	 * Do the following iff this is a first open.
	 */
	if (!ISSET(tp->t_state, TS_ISOPEN)) {
		struct termios t;

		/* Turn on interrupts. */
		sc->sc_ier = IER_ERXRDY | IER_ERLS | IER_EMSC;
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, com_ier, sc->sc_ier);

		/* Fetch the current modem control status, needed later. */
		sc->sc_msr = bus_space_read_1(sc->sc_iot, sc->sc_ioh, com_msr);

		/* Add some entry points needed by the tty layer. */
		tp->t_oproc = comstart;
		tp->t_param = comparam;
		tp->t_hwiflow = comhwiflow;
		tp->t_dev = dev;

		/*
		 * Initialize the termios status to the defaults.  Add in the
		 * sticky bits from TIOCSFLAGS.
		 */
		t.c_ispeed = 0;
		if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
			t.c_ospeed = comconsrate;
			t.c_cflag = comconscflag;
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
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		ttychars(tp);
		(void) comparam(tp, &t);
		ttsetwater(tp);

		s2 = splserial();

		/*
		 * Turn on DTR.  We must always do this, even if carrier is not
		 * present, because otherwise we'd have to use TIOCSDTR
		 * immediately after setting CLOCAL.  We will drop DTR only on
		 * the next high-low transition of DCD, or by explicit request.
		 */
		com_modem(sc, 1);

		/* Clear the input ring, and unblock. */
		sc->sc_rbput = sc->sc_rbget = 0;
		sc->sc_rbavail = RXBUFSIZE;
		com_iflush(sc);
		CLR(sc->sc_rx_flags, RX_ANY_BLOCK);
		com_hwiflow(sc);

#ifdef COM_DEBUG
		if (com_debug)
			comstatus(sc, "comopen  ");
#endif

		splx(s2);
	}
	error = 0;

	/* If we're doing a blocking open... */
	if (!ISSET(flag, O_NONBLOCK))
		/* ...then wait for carrier. */
		while (!ISSET(tp->t_state, TS_CARR_ON) &&
		    !ISSET(tp->t_cflag, CLOCAL | MDMBUF)) {
			error = ttysleep(tp, &tp->t_rawq, TTIPRI | PCATCH,
			    ttopen, 0);
			if (error) {
				/*
				 * If the open was interrupted and nobody
				 * else has the device open, then hang up.
				 */
				if (!ISSET(tp->t_state, TS_ISOPEN)) {
					com_modem(sc, 0);
					CLR(tp->t_state, TS_WOPEN);
					ttwakeup(tp);
				}
				break;
			}
			SET(tp->t_state, TS_WOPEN);
		}

	splx(s);
	if (error == 0)
		error = (*linesw[tp->t_line].l_open)(dev, tp);
	return (error);
}
 
int
comclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	int unit = COMUNIT(dev);
	struct com_softc *sc = com_cd.cd_devs[unit];
	struct tty *tp = sc->sc_tty;
	int s;

	/* XXX This is for cons.c. */
	if (!ISSET(tp->t_state, TS_ISOPEN))
		return (0);

	(*linesw[tp->t_line].l_close)(tp, flag);
	ttyclose(tp);

	/* If we were asserting flow control, then deassert it. */
	SET(sc->sc_rx_flags, RX_IBUF_BLOCKED);
	com_hwiflow(sc);
	/* Clear any break condition set with TIOCSBRK. */
	com_break(sc, 0);
	/*
	 * Hang up if necessary.  Wait a bit, so the other side has time to
	 * notice even if we immediately open the port again.
	 */
	if (ISSET(tp->t_cflag, HUPCL)) {
		com_modem(sc, 0);
		(void) tsleep(sc, TTIPRI, ttclos, hz);
	}

	s = splserial();
	/* Turn off interrupts. */
	sc->sc_ier = 0;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, com_ier, sc->sc_ier);
	splx(s);
	
	return (0);
}
 
int
comread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct com_softc *sc = com_cd.cd_devs[COMUNIT(dev)];
	struct tty *tp = sc->sc_tty;
 
	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}
 
int
comwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct com_softc *sc = com_cd.cd_devs[COMUNIT(dev)];
	struct tty *tp = sc->sc_tty;
 
	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

struct tty *
comtty(dev)
	dev_t dev;
{
	struct com_softc *sc = com_cd.cd_devs[COMUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	return (tp);
}
 
int
comioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	int unit = COMUNIT(dev);
	struct com_softc *sc = com_cd.cd_devs[unit];
	struct tty *tp = sc->sc_tty;
	int error;

	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag, p);
	if (error >= 0)
		return (error);

	error = ttioctl(tp, cmd, data, flag, p);
	if (error >= 0)
		return (error);

	switch (cmd) {
	case TIOCSBRK:
		com_break(sc, 1);
		break;

	case TIOCCBRK:
		com_break(sc, 0);
		break;

	case TIOCSDTR:
		com_modem(sc, 1);
		break;

	case TIOCCDTR:
		com_modem(sc, 0);
		break;

	case TIOCGFLAGS:
		*(int *)data = sc->sc_swflags;
		break;

	case TIOCSFLAGS:
		error = suser(p->p_ucred, &p->p_acflag); 
		if (error)
			return (error); 
		sc->sc_swflags = *(int *)data;
		break;

	case TIOCMSET:
	case TIOCMBIS:
	case TIOCMBIC:
	case TIOCMGET:
	default:
		return (ENOTTY);
	}

#ifdef COM_DEBUG
	if (com_debug)
		comstatus(sc, "comioctl ");
#endif

	return (0);
}

integrate void
com_schedrx(sc)
	struct com_softc *sc;
{

	sc->sc_rx_ready = 1;

	/* Wake up the poller. */
#ifdef __GENERIC_SOFT_INTERRUPTS
	softintr_schedule(sc->sc_si);
#else
#ifndef alpha
	setsoftserial();
#else
	if (!com_softintr_scheduled) {
		com_softintr_scheduled = 1;
		timeout(comsoft, NULL, 1);
	}
#endif
#endif
}

void
com_break(sc, onoff)
	struct com_softc *sc;
	int onoff;
{
	int s;

	s = splserial();
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
			com_loadchannelregs(sc);
	}
	splx(s);
}

void
com_modem(sc, onoff)
	struct com_softc *sc;
	int onoff;
{
	int s;

	s = splserial();
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
			com_loadchannelregs(sc);
	}
	splx(s);
}

int
comparam(tp, t)
	struct tty *tp;
	struct termios *t;
{
	struct com_softc *sc = com_cd.cd_devs[COMUNIT(tp->t_dev)];
	int ospeed = comspeed(t->c_ospeed);
	u_char lcr;
	int s;

	/* check requested parameters */
	if (ospeed < 0)
		return (EINVAL);
	if (t->c_ispeed && t->c_ispeed != t->c_ospeed)
		return (EINVAL);

	lcr = ISSET(sc->sc_lcr, LCR_SBREAK);

	switch (ISSET(t->c_cflag, CSIZE)) {
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
	if (ISSET(t->c_cflag, PARENB)) {
		SET(lcr, LCR_PENAB);
		if (!ISSET(t->c_cflag, PARODD))
			SET(lcr, LCR_PEVEN);
	}
	if (ISSET(t->c_cflag, CSTOPB))
		SET(lcr, LCR_STOPB);

	s = splserial();

	sc->sc_lcr = lcr;

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
		sc->sc_r_hiwat = RXHIWAT;
		sc->sc_r_lowat = RXLOWAT;
	} else if (ISSET(t->c_cflag, MDMBUF)) {
		/*
		 * For DTR/DCD flow control, make sure we don't toggle DTR for
		 * carrier detection.
		 */
		sc->sc_mcr_dtr = 0;
		sc->sc_mcr_rts = MCR_DTR;
		sc->sc_msr_cts = MSR_DCD;
		sc->sc_r_hiwat = RXHIWAT;
		sc->sc_r_lowat = RXLOWAT;
	} else {
		/*
		 * If no flow control, then always set RTS.  This will make
		 * the other side happy if it mistakenly thinks we're doing
		 * RTS/CTS flow control.
		 */
		sc->sc_mcr_dtr = MCR_DTR | MCR_RTS;
		sc->sc_mcr_rts = 0;
		sc->sc_msr_cts = 0;
		sc->sc_r_hiwat = 0;
		sc->sc_r_lowat = 0;
		if (ISSET(sc->sc_mcr, MCR_DTR))
			SET(sc->sc_mcr, MCR_RTS);
		else
			CLR(sc->sc_mcr, MCR_RTS);
	}
	sc->sc_msr_mask = sc->sc_msr_cts | sc->sc_msr_dcd;

#if 0
	if (ospeed == 0)
		CLR(sc->sc_mcr, sc->sc_mcr_dtr);
	else
		SET(sc->sc_mcr, sc->sc_mcr_dtr);
#endif

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
	if (ISSET(sc->sc_hwflags, COM_HW_HAYESP))
		sc->sc_fifo = FIFO_DMA_MODE | FIFO_ENABLE | FIFO_TRIGGER_8;
	else if (ISSET(sc->sc_hwflags, COM_HW_FIFO))
		sc->sc_fifo = FIFO_ENABLE |
		    (t->c_ospeed <= 1200 ? FIFO_TRIGGER_1 :
		     t->c_ospeed <= 38400 ? FIFO_TRIGGER_8 : FIFO_TRIGGER_4);
	else
		sc->sc_fifo = 0;

	/* and copy to tty */
	tp->t_ispeed = 0;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;

	if (!sc->sc_heldchange) {
		if (sc->sc_tx_busy) {
			sc->sc_heldtbc = sc->sc_tbc;
			sc->sc_tbc = 0;
			sc->sc_heldchange = 1;
		} else
			com_loadchannelregs(sc);
	}

	splx(s);

	/*
	 * Update the tty layer's idea of the carrier bit, in case we changed
	 * CLOCAL or MDMBUF.  We don't hang up here; we only do that if we
	 * lose carrier while carrier detection is on.
	 */
	(void) (*linesw[tp->t_line].l_modem)(tp, ISSET(sc->sc_msr, MSR_DCD));

#ifdef COM_DEBUG
	if (com_debug)
		comstatus(sc, "comparam ");
#endif

	/* Block or unblock as needed. */
	if (!ISSET(t->c_cflag, CHWFLOW)) {
		if (ISSET(sc->sc_rx_flags, RX_TTY_OVERFLOWED)) {
			CLR(sc->sc_rx_flags, RX_TTY_OVERFLOWED);
			com_schedrx(sc);
		}
		if (ISSET(sc->sc_rx_flags, RX_TTY_BLOCKED|RX_IBUF_BLOCKED)) {
			CLR(sc->sc_rx_flags, RX_TTY_BLOCKED|RX_IBUF_BLOCKED);
			com_hwiflow(sc);
		}
		if (sc->sc_tx_stopped) {
			sc->sc_tx_stopped = 0;
			comstart(tp);
		}
	} else {
		/* XXXXX FIX ME */
#if 0
		commsrint(sc, tp);
#endif
	}

	return (0);
}

void
com_iflush(sc)
	struct com_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	/* flush any pending I/O */
	while (ISSET(bus_space_read_1(iot, ioh, com_lsr), LSR_RXRDY))
		(void) bus_space_read_1(iot, ioh, com_data);
}

void
com_loadchannelregs(sc)
	struct com_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	/* XXXXX necessary? */
	com_iflush(sc);

	bus_space_write_1(iot, ioh, com_ier, 0);

	bus_space_write_1(iot, ioh, com_lcr, sc->sc_lcr | LCR_DLAB);
	bus_space_write_1(iot, ioh, com_dlbl, sc->sc_dlbl);
	bus_space_write_1(iot, ioh, com_dlbh, sc->sc_dlbh);
	bus_space_write_1(iot, ioh, com_lcr, sc->sc_lcr);
	bus_space_write_1(iot, ioh, com_mcr, sc->sc_mcr_active = sc->sc_mcr);
	bus_space_write_1(iot, ioh, com_fifo, sc->sc_fifo);

	bus_space_write_1(iot, ioh, com_ier, sc->sc_ier);
}

int
comhwiflow(tp, block)
	struct tty *tp;
	int block;
{
	struct com_softc *sc = com_cd.cd_devs[COMUNIT(tp->t_dev)];
	int s;

	if (sc->sc_mcr_rts == 0)
		return (0);

	s = splserial();
	if (block) {
		if (!ISSET(sc->sc_rx_flags, RX_TTY_BLOCKED)) {
			SET(sc->sc_rx_flags, RX_TTY_BLOCKED);
			com_hwiflow(sc);
		}
	} else {
		if (ISSET(sc->sc_rx_flags, RX_TTY_OVERFLOWED)) {
			CLR(sc->sc_rx_flags, RX_TTY_OVERFLOWED);
			com_schedrx(sc);
		}
		if (ISSET(sc->sc_rx_flags, RX_TTY_BLOCKED)) {
			CLR(sc->sc_rx_flags, RX_TTY_BLOCKED);
			com_hwiflow(sc);
		}
	}
	splx(s);
	return (1);
}
	
/*
 * (un)block input via hw flowcontrol
 */
void
com_hwiflow(sc)
	struct com_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	if (sc->sc_mcr_rts == 0)
		return;

	if (ISSET(sc->sc_rx_flags, RX_ANY_BLOCK)) {
		CLR(sc->sc_mcr, sc->sc_mcr_rts);
		CLR(sc->sc_mcr_active, sc->sc_mcr_rts);
	} else {
		SET(sc->sc_mcr, sc->sc_mcr_rts);
		SET(sc->sc_mcr_active, sc->sc_mcr_rts);
	}
	bus_space_write_1(iot, ioh, com_mcr, sc->sc_mcr_active);
}


void
comstart(tp)
	struct tty *tp;
{
	struct com_softc *sc = com_cd.cd_devs[COMUNIT(tp->t_dev)];
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int s;

	s = spltty();
	if (ISSET(tp->t_state, TS_BUSY))
		goto out;
	if (ISSET(tp->t_state, TS_TIMEOUT | TS_TTSTOP))
		goto stopped;

	if (sc->sc_tx_stopped)
		goto stopped;

	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (ISSET(tp->t_state, TS_ASLEEP)) {
			CLR(tp->t_state, TS_ASLEEP);
			wakeup(&tp->t_outq);
		}
		selwakeup(&tp->t_wsel);
		if (tp->t_outq.c_cc == 0)
			goto stopped;
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

	/* Enable transmit completion interrupts if necessary. */
	if (!ISSET(sc->sc_ier, IER_ETXRDY)) {
		SET(sc->sc_ier, IER_ETXRDY);
		bus_space_write_1(iot, ioh, com_ier, sc->sc_ier);
	}

	/* Output the first chunk of the contiguous buffer. */
	{
		int n;

		n = sc->sc_fifolen;
		if (n > sc->sc_tbc)
			n = sc->sc_tbc;
		bus_space_write_multi_1(iot, ioh, com_data, sc->sc_tba, n);
		sc->sc_tbc -= n;
		sc->sc_tba += n;
	}
	splx(s);
	return;

stopped:
	/* Disable transmit completion interrupts if necessary. */
	if (ISSET(sc->sc_ier, IER_ETXRDY)) {
		CLR(sc->sc_ier, IER_ETXRDY);
		bus_space_write_1(iot, ioh, com_ier, sc->sc_ier);
	}
out:
	splx(s);
	return;
}

/*
 * Stop output on a line.
 */
void
comstop(tp, flag)
	struct tty *tp;
	int flag;
{
	struct com_softc *sc = com_cd.cd_devs[COMUNIT(tp->t_dev)];
	int s;

	s = splserial();
	if (ISSET(tp->t_state, TS_BUSY)) {
		/* Stop transmitting at the next chunk. */
		sc->sc_tbc = 0;
		sc->sc_heldtbc = 0;
		if (!ISSET(tp->t_state, TS_TTSTOP))
			SET(tp->t_state, TS_FLUSH);
	}
	splx(s);
}

void
comdiag(arg)
	void *arg;
{
	struct com_softc *sc = arg;
	int overflows, floods;
	int s;

	s = splserial();
	overflows = sc->sc_overflows;
	sc->sc_overflows = 0;
	floods = sc->sc_floods;
	sc->sc_floods = 0;
	sc->sc_errors = 0;
	splx(s);

	log(LOG_WARNING,
	    "%s: %d silo overflow%s, %d ibuf flood%s\n",
	    sc->sc_dev.dv_xname,
	    overflows, overflows == 1 ? "" : "s",
	    floods, floods == 1 ? "" : "s");
}

integrate void
comrxint(sc, tp)
	struct com_softc	*sc;
	struct tty	*tp;
{
	u_int	get, cc, scc;
	int	code;
	u_char	lsr;
	int	s;
	static int lsrmap[8] = {
		0,      TTY_PE,
		TTY_FE, TTY_PE|TTY_FE,
		TTY_FE, TTY_PE|TTY_FE,
		TTY_FE, TTY_PE|TTY_FE
	};

	get = sc->sc_rbget;
	scc = cc = RXBUFSIZE - sc->sc_rbavail;

	if (cc == RXBUFSIZE) {
		sc->sc_floods++;
		if (sc->sc_errors++ == 0)
			timeout(comdiag, sc, 60 * hz);
	}

	while (cc) {
		lsr = sc->sc_lbuf[get];
		if (ISSET(lsr, LSR_BI)) {
#ifdef DDB
			if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE))
				Debugger();
#endif
		}
		else if (ISSET(lsr, LSR_OE)) {
			sc->sc_overflows++;
			if (sc->sc_errors++ == 0)
				timeout(comdiag, sc, 60 * hz);
		}
		code = sc->sc_rbuf[get] |
		    lsrmap[(lsr & (LSR_BI|LSR_FE|LSR_PE)) >> 2];
		if ((*linesw[tp->t_line].l_rint)(code, tp) == -1) {
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
				get = (get + cc) & RXBUFMASK;
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
		get = (get + 1) & RXBUFMASK;
		cc--;
	}

	if (cc != scc) {
		sc->sc_rbget = get;
		s = splserial();
		cc = sc->sc_rbavail += scc - cc;
		/* Buffers should be ok again, release possible block. */
		if (cc >= sc->sc_r_lowat) {
			if (ISSET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED)) {
				CLR(sc->sc_rx_flags, RX_IBUF_OVERFLOWED);
				SET(sc->sc_ier, IER_ERXRDY);
				bus_space_write_1(sc->sc_iot, sc->sc_ioh, com_ier, sc->sc_ier);
			}
			if (ISSET(sc->sc_rx_flags, RX_IBUF_BLOCKED)) {
				CLR(sc->sc_rx_flags, RX_IBUF_BLOCKED);
				com_hwiflow(sc);
			}
		}
		splx(s);
	}
}

integrate void
comtxint(sc, tp)
	struct com_softc	*sc;
	struct tty	*tp;
{

	CLR(tp->t_state, TS_BUSY);
	if (ISSET(tp->t_state, TS_FLUSH))
		CLR(tp->t_state, TS_FLUSH);
	else
		ndflush(&tp->t_outq, (int)(sc->sc_tba - tp->t_outq.c_cf));
	(*linesw[tp->t_line].l_start)(tp);
}

integrate void
commsrint(sc, tp)
	struct com_softc	*sc;
	struct tty	*tp;
{
	u_char msr, delta;
	int s;

	s = splserial();
	msr = sc->sc_msr;
	delta = sc->sc_msr_delta;
	sc->sc_msr_delta = 0;
	splx(s);

	if (ISSET(delta, sc->sc_msr_dcd)) {
		/*
		 * Inform the tty layer that carrier detect changed.
		 */
		(void) (*linesw[tp->t_line].l_modem)(tp, ISSET(msr, MSR_DCD));
	}

	if (ISSET(delta, sc->sc_msr_cts)) {
		/* Block or unblock output according to flow control. */
		if (ISSET(msr, sc->sc_msr_cts)) {
			sc->sc_tx_stopped = 0;
			(*linesw[tp->t_line].l_start)(tp);
		} else {
			sc->sc_tx_stopped = 1;
		}
	}

#ifdef COM_DEBUG
	if (com_debug)
		comstatus(sc, "commsrint");
#endif
}

#ifdef __GENERIC_SOFT_INTERRUPTS
void
comsoft(arg)
	void *arg;
{
	struct com_softc *sc = arg;
	struct tty *tp;

	{
#else
void
#ifndef alpha
comsoft()
#else
comsoft(arg)
	void *arg;
#endif
{
	struct com_softc	*sc;
	struct tty	*tp;
	int	unit;
#ifdef alpha
	int s;

	s = splsoftserial();
	com_softintr_scheduled = 0;
#endif

	for (unit = 0; unit < com_cd.cd_ndevs; unit++) {
		sc = com_cd.cd_devs[unit];
		if (sc == NULL)
			continue;
#endif

		tp = sc->sc_tty;
		if (tp == NULL || !ISSET(tp->t_state, TS_ISOPEN | TS_WOPEN))
			continue;
		
		if (sc->sc_rx_ready) {
			sc->sc_rx_ready = 0;
			comrxint(sc, tp);
		}

		if (sc->sc_st_check) {
			sc->sc_st_check = 0;
			commsrint(sc, tp);
		}

		if (sc->sc_tx_done) {
			sc->sc_tx_done = 0;
			comtxint(sc, tp);
		}
	}

#ifndef __GENERIC_SOFT_INTERRUPTS
#ifdef alpha
	splx(s);
#endif
#endif
}

int
comintr(arg)
	void	*arg;
{
	struct com_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_char	lsr, iir;
	u_int	put, cc;

	iir = bus_space_read_1(iot, ioh, com_iir);
	if (ISSET(iir, IIR_NOPEND))
		return (0);

	put = sc->sc_rbput;
	cc = sc->sc_rbavail;

	do {
		u_char	msr, delta;

		lsr = bus_space_read_1(iot, ioh, com_lsr);
		if (ISSET(lsr, LSR_RCV_MASK) &&
		    !ISSET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED)) {
			for (; ISSET(lsr, LSR_RCV_MASK) && cc > 0; cc--) {
				sc->sc_rbuf[put] =
				    bus_space_read_1(iot, ioh, com_data);
				sc->sc_lbuf[put] = lsr;
				put = (put + 1) & RXBUFMASK;
				lsr = bus_space_read_1(iot, ioh, com_lsr);
			}
			/*
			 * Current string of incoming characters ended because
			 * no more data was available. Schedule a receive event
			 * if any data was received. Drop any characters that
			 * we couldn't handle.
			 */
			sc->sc_rbput = put;
			sc->sc_rbavail = cc;
			if (!ISSET(sc->sc_rx_flags, RX_TTY_OVERFLOWED))
				sc->sc_rx_ready = 1;
			/*
			 * See if we are in danger of overflowing a buffer. If
			 * so, use hardware flow control to ease the pressure.
			 */
			if (!ISSET(sc->sc_rx_flags, RX_IBUF_BLOCKED) &&
			    cc < sc->sc_r_hiwat) {
				SET(sc->sc_rx_flags, RX_IBUF_BLOCKED);
				com_hwiflow(sc);
			}
			/*
			 * If we're out of space, disable receive interrupts
			 * until the queue has drained a bit.
			 */
			if (!cc) {
				SET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED);
				CLR(sc->sc_ier, IER_ERXRDY);
				bus_space_write_1(iot, ioh, com_ier, sc->sc_ier);
			}
		} else {
			if ((iir & IIR_IMASK) == IIR_RXRDY) {
				bus_space_write_1(iot, ioh, com_ier, 0);
				delay(10);
				bus_space_write_1(iot, ioh, com_ier,sc->sc_ier);
				iir = IIR_NOPEND;
				continue;
			}
		}

		msr = bus_space_read_1(iot, ioh, com_msr);
		delta = msr ^ sc->sc_msr;
		sc->sc_msr = msr;
		if (ISSET(delta, sc->sc_msr_mask)) {
			sc->sc_msr_delta |= delta;

			/*
			 * Stop output immediately if we lose the output
			 * flow control signal or carrier detect.
			 */
			if (ISSET(~msr, sc->sc_msr_mask)) {
				sc->sc_tbc = 0;
				sc->sc_heldtbc = 0;
#ifdef COM_DEBUG
				if (com_debug)
					comstatus(sc, "comintr  ");
#endif
			}

			sc->sc_st_check = 1;
		}
	} while (!ISSET((iir = bus_space_read_1(iot, ioh, com_iir)), IIR_NOPEND));

	/*
	 * Done handling any receive interrupts. See if data can be
	 * transmitted as well. Schedule tx done event if no data left
	 * and tty was marked busy.
	 */
	if (ISSET(lsr, LSR_TXRDY)) {
		/*
		 * If we've delayed a parameter change, do it now, and restart
		 * output.
		 */
		if (sc->sc_heldchange) {
			com_loadchannelregs(sc);
			sc->sc_heldchange = 0;
			sc->sc_tbc = sc->sc_heldtbc;
			sc->sc_heldtbc = 0;
		}
		/* Output the next chunk of the contiguous buffer, if any. */
		if (sc->sc_tbc > 0) {
			int n;

			n = sc->sc_fifolen;
			if (n > sc->sc_tbc)
				n = sc->sc_tbc;
			bus_space_write_multi_1(iot, ioh, com_data, sc->sc_tba, n);
			sc->sc_tbc -= n;
			sc->sc_tba += n;
		} else if (sc->sc_tx_busy) {
			sc->sc_tx_busy = 0;
			sc->sc_tx_done = 1;
		}
	}

	/* Wake up the poller. */
#ifdef __GENERIC_SOFT_INTERRUPTS
	softintr_schedule(sc->sc_si);
#else
#ifndef alpha
	setsoftserial();
#else
	if (!com_softintr_scheduled) {
		com_softintr_scheduled = 1;
		timeout(comsoft, NULL, 1);
	}
#endif
#endif
	return (1);
}

/*
 * Following are all routines needed for COM to act as console
 */
#include <dev/cons.h>

void
comcnprobe(cp)
	struct consdev *cp;
{
	/* XXX NEEDS TO BE FIXED XXX */
	bus_space_tag_t iot = 0;
	bus_space_handle_t ioh;
	int found;

	if (bus_space_map(iot, CONADDR, COM_NPORTS, 0, &ioh)) {
		cp->cn_pri = CN_DEAD;
		return;
	}
	found = comprobe1(iot, ioh, CONADDR);
	bus_space_unmap(iot, ioh, COM_NPORTS);
	if (!found) {
		cp->cn_pri = CN_DEAD;
		return;
	}

	/* locate the major number */
	for (commajor = 0; commajor < nchrdev; commajor++)
		if (cdevsw[commajor].d_open == comopen)
			break;

	/* initialize required fields */
	cp->cn_dev = makedev(commajor, CONUNIT);
#ifdef	COMCONSOLE
	cp->cn_pri = CN_REMOTE;		/* Force a serial port console */
#else
	cp->cn_pri = CN_NORMAL;
#endif
}

void
comcninit(cp)
	struct consdev *cp;
{

#if 0
	XXX NEEDS TO BE FIXED XXX
	comconstag = ???;
#endif
	if (bus_space_map(comconstag, CONADDR, COM_NPORTS, 0, &comconsioh))
		panic("comcninit: mapping failed");

	cominitcons(comconstag, comconsioh, comconsrate);
	comconsaddr = CONADDR;
}

/*
 * Initialize UART to known state.
 */
void
cominit(iot, ioh, rate)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int rate;
{

	bus_space_write_1(iot, ioh, com_lcr, LCR_DLAB);
	rate = comspeed(rate);
	bus_space_write_1(iot, ioh, com_dlbl, rate);
	bus_space_write_1(iot, ioh, com_dlbh, rate >> 8);
	bus_space_write_1(iot, ioh, com_lcr, LCR_8BITS);
	bus_space_write_1(iot, ioh, com_mcr, 0);
	bus_space_write_1(iot, ioh, com_fifo,
	    FIFO_ENABLE | FIFO_RCV_RST | FIFO_XMT_RST | FIFO_TRIGGER_1);
	bus_space_write_1(iot, ioh, com_ier, 0);
}

/*
 * Set UART for console use. Do normal init, then enable interrupts.
 */
void
cominitcons(iot, ioh, rate)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int rate;
{
	int s = splserial();
	u_char stat;

	cominit(iot, ioh, rate);
	bus_space_write_1(iot, ioh, com_ier, IER_ERXRDY | IER_ETXRDY);
	bus_space_write_1(iot, ioh, com_mcr, MCR_DTR | MCR_RTS);
	DELAY(100);
	stat = bus_space_read_1(iot, ioh, com_iir);
	splx(s);
}

int
comcngetc(dev)
	dev_t dev;
{
	int s = splserial();
	bus_space_tag_t iot = comconstag;
	bus_space_handle_t ioh = comconsioh;
	u_char stat, c;

	while (!ISSET(stat = bus_space_read_1(iot, ioh, com_lsr), LSR_RXRDY))
		;
	c = bus_space_read_1(iot, ioh, com_data);
	stat = bus_space_read_1(iot, ioh, com_iir);
	splx(s);
	return (c);
}

/*
 * Console kernel output character routine.
 */
void
comcnputc(dev, c)
	dev_t dev;
	int c;
{
	int s = splserial();
	bus_space_tag_t iot = comconstag;
	bus_space_handle_t ioh = comconsioh;
	u_char stat;
	register int timo;

	/* wait for any pending transmission to finish */
	timo = 50000;
	while (!ISSET(stat = bus_space_read_1(iot, ioh, com_lsr), LSR_TXRDY) && --timo)
		;
	bus_space_write_1(iot, ioh, com_data, c);
	/* wait for this transmission to complete */
	timo = 1500000;
	while (!ISSET(stat = bus_space_read_1(iot, ioh, com_lsr), LSR_TXRDY) && --timo)
		;
	/* clear any interrupts generated by this transmission */
	stat = bus_space_read_1(iot, ioh, com_iir);
	splx(s);
}

void
comcnpollc(dev, on)
	dev_t dev;
	int on;
{

}
