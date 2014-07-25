/*	$NetBSD: ser.c,v 1.55 2014/07/25 08:10:32 dholland Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Leo Weppelman.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
/*-
 * Copyright (c) 1993, 1994, 1995, 1996, 1997
 *      Charles M. Hannum.  All rights reserved.
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
 *      This product includes software developed by Charles M. Hannum.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ser.c,v 1.55 2014/07/25 08:10:32 dholland Exp $");

#include "opt_ddb.h"
#include "opt_mbtype.h"
#include "opt_serconsole.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/kauth.h>

#include <m68k/asm_single.h>

#include <machine/iomap.h>
#include <machine/mfp.h>
#include <machine/intr.h>
#include <atari/dev/serreg.h>

#include "ioconf.h"

#if !defined(_MILANHW_)
#include <atari/dev/ym2149reg.h>
#else
	/* MILAN has no ym2149 */
#define ym2149_dtr(set) {					\
	if (set)						\
		single_inst_bset_b(MFP->mf_gpip, 0x08);		\
	else							\
		single_inst_bclr_b(MFP->mf_gpip, 0x08);		\
}

#define ym2149_rts(set) {					\
	if (set)						\
		single_inst_bset_b(MFP->mf_gpip, 0x01);		\
	else							\
		single_inst_bclr_b(MFP->mf_gpip, 0x01);		\
}
#endif /* _MILANHW_ */

/* #define SER_DEBUG */

#define	SERUNIT(x)	(minor(x) & 0x7ffff)
#define	SERDIALOUT(x)	(minor(x) & 0x80000)

/* XXX */
#define	CONSBAUD	9600
#define	CONSCFLAG	TTYDEF_CFLAG
/* end XXX */

/* Buffer size for character buffer */
#define RXBUFSIZE 2048		/* More than enough..			*/
#define RXBUFMASK (RXBUFSIZE-1)	/* Only iff previous is a power of 2	*/
#define RXHIWAT   (RXBUFSIZE >> 2)

struct ser_softc {
	device_t	 sc_dev;
	struct tty	*sc_tty;

	struct callout sc_diag_ch;

	int		 sc_overflows;
	int		 sc_floods;
	int		 sc_errors;

	uint8_t		 sc_hwflags;
	uint8_t		 sc_swflags;

	int		 sc_ospeed;		/* delay + timer-d data	*/
	uint8_t		 sc_imra;
	uint8_t		 sc_imrb;
	uint8_t		 sc_ucr;		/* Uart control		*/
	uint8_t		 sc_msr;		/* Modem status		*/
	uint8_t		 sc_tsr;		/* Tranceiver status	*/
	uint8_t		 sc_rsr;		/* Receiver status	*/
	uint8_t		 sc_mcr;		/* (Pseudo) Modem ctrl. */

	uint8_t		 sc_msr_delta;
	uint8_t		 sc_msr_mask;
	uint8_t		 sc_mcr_active;
	uint8_t		 sc_mcr_dtr, sc_mcr_rts, sc_msr_cts, sc_msr_dcd;

	int		 sc_r_hiwat;
 	volatile u_int	 sc_rbget;
 	volatile u_int	 sc_rbput;
	volatile u_int	 sc_rbavail;
 	uint8_t		 sc_rbuf[RXBUFSIZE];
	uint8_t		 sc_lbuf[RXBUFSIZE];

	volatile uint8_t sc_rx_blocked;
	volatile uint8_t sc_rx_ready;
	volatile uint8_t sc_tx_busy;
	volatile uint8_t sc_tx_done;
	volatile uint8_t sc_tx_stopped;
	volatile uint8_t sc_st_check;

 	uint8_t		*sc_tba;
 	int		 sc_tbc;
	int		 sc_heldtbc;

	volatile uint8_t sc_heldchange;

	void		*sc_sicookie;
};

/*
 * For sc_hwflags:
 */
#define	SER_HW_CONSOLE	0x01

static void	ser_break(struct ser_softc *, int);
static void	ser_hwiflow(struct ser_softc *, int);
static void	ser_iflush(struct ser_softc *);
static void	ser_loadchannelregs(struct ser_softc *);
static void	ser_modem(struct ser_softc *, int);
static void	serdiag(void *);
static int	serhwiflow(struct tty *, int);
static void	serinit(int);
static void	serinitcons(int);
static int	sermintr(void *);
static int	sertrintr(void *);
static int	serparam(struct tty *, struct termios *);
static void	serstart(struct tty *);

struct consdev;
void	sercnprobe(struct consdev *);
void	sercninit(struct consdev *);
int	sercngetc(dev_t);
void	sercnputc(dev_t, int);
void	sercnpollc(dev_t, int);

static void sermsrint(struct ser_softc *, struct tty*);
static void serrxint(struct ser_softc *, struct tty*);
static void ser_shutdown(struct ser_softc *);
static int serspeed(long);
static void sersoft(void *);
static void sertxint(struct ser_softc *, struct tty*);

/*
 * Autoconfig stuff
 */
static int  sermatch(device_t, cfdata_t, void *);
static void serattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ser, sizeof(struct ser_softc),
    sermatch, serattach, NULL, NULL);

dev_type_open(seropen);
dev_type_close(serclose);
dev_type_read(serread);
dev_type_write(serwrite);
dev_type_ioctl(serioctl);
dev_type_stop(serstop);
dev_type_tty(sertty);
dev_type_poll(serpoll);

const struct cdevsw ser_cdevsw = {
	.d_open = seropen,
	.d_close = serclose,
	.d_read = serread,
	.d_write = serwrite,
	.d_ioctl = serioctl,
	.d_stop = serstop,
	.d_tty = sertty,
	.d_poll = serpoll,
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};

#ifndef SERCONSOLE
#define SERCONSOLE	0
#endif
int serconsole = SERCONSOLE;	/* patchable */

/*ARGSUSED*/
static	int
sermatch(device_t parent, cfdata_t cf, void *aux)
{
	static int ser_matched = 0;

	/* Match at most one ser unit */
	if (strcmp((char *)aux, "ser") || ser_matched)
		return 0;

	ser_matched = 1;
	return 1;
}

/*ARGSUSED*/
static void
serattach(device_t parent, device_t self, void *aux)
{
	struct ser_softc *sc = device_private(self);

	sc->sc_dev = self;

	if (intr_establish(1, USER_VEC, 0, (hw_ifun_t)sermintr, sc) == NULL)
		aprint_error(": Can't establish interrupt (1)\n");
	if (intr_establish(2, USER_VEC, 0, (hw_ifun_t)sermintr, sc) == NULL)
		aprint_error(": Can't establish interrupt (2)\n");
	if (intr_establish(14, USER_VEC, 0, (hw_ifun_t)sermintr, sc) == NULL)
		aprint_error(": Can't establish interrupt (14)\n");
	if (intr_establish(9, USER_VEC, 0, (hw_ifun_t)sertrintr, sc) == NULL)
		aprint_error(": Can't establish interrupt (9)\n");
	if (intr_establish(10, USER_VEC, 0, (hw_ifun_t)sertrintr, sc) == NULL)
		aprint_error(": Can't establish interrupt (10)\n");
	if (intr_establish(11, USER_VEC, 0, (hw_ifun_t)sertrintr, sc) == NULL)
		aprint_error(": Can't establish interrupt (11)\n");
	if (intr_establish(12, USER_VEC, 0, (hw_ifun_t)sertrintr, sc) == NULL)
		aprint_error(": Can't establish interrupt (12)\n");

	sc->sc_sicookie = softint_establish(SOFTINT_SERIAL, sersoft, sc);

	ym2149_rts(1);
	ym2149_dtr(1);

	/*
	 * Enable but mask interrupts...
	 * XXX: Look at edge-sensitivity for DCD/CTS interrupts.
	 */
	MFP->mf_ierb |= IB_SCTS|IB_SDCD;
	MFP->mf_iera |= IA_RRDY|IA_RERR|IA_TRDY|IA_TERR;
	MFP->mf_imrb &= ~(IB_SCTS|IB_SDCD);
	MFP->mf_imra &= ~(IA_RRDY|IA_RERR|IA_TRDY|IA_TERR);

	callout_init(&sc->sc_diag_ch, 0);

	if (serconsole) {
		/*
		 * Activate serial console when DCD present...
		 */
		if ((MFP->mf_gpip & MCR_DCD) == 0)
			SET(sc->sc_hwflags, SER_HW_CONSOLE);
	}

	aprint_normal(": modem1 on 68901 MFP1 USART\n");
	if (ISSET(sc->sc_hwflags, SER_HW_CONSOLE)) {
		serinit(CONSBAUD);
		aprint_normal_dev(self, "console\n");
	}
}

#ifdef SER_DEBUG
void serstatus(struct ser_softc *, char *);
void
serstatus(struct ser_softc *sc, char *str)
{
	struct tty *tp = sc->sc_tty;

	printf("%s: %s %sclocal  %sdcd %sts_carr_on %sdtr %stx_stopped\n",
	    device_xname(sc->sc_dev), str,
	    ISSET(tp->t_cflag, CLOCAL) ? "+" : "-",
	    ISSET(sc->sc_msr, MCR_DCD) ? "+" : "-",
	    ISSET(tp->t_state, TS_CARR_ON) ? "+" : "-",
	    ISSET(sc->sc_mcr, MCR_DTR) ? "+" : "-",
	    sc->sc_tx_stopped ? "+" : "-");

	printf("%s: %s %scrtscts %scts %sts_ttstop  %srts %srx_blocked\n",
	    device_xname(sc->sc_dev), str,
	    ISSET(tp->t_cflag, CRTSCTS) ? "+" : "-",
	    ISSET(sc->sc_msr, MCR_CTS) ? "+" : "-",
	    ISSET(tp->t_state, TS_TTSTOP) ? "+" : "-",
	    ISSET(sc->sc_mcr, MCR_RTS) ? "+" : "-",
	    sc->sc_rx_blocked ? "+" : "-");
}
#endif /* SER_DEBUG */

int
seropen(dev_t dev, int flag, int mode, struct lwp *l)
{
	int unit = SERUNIT(dev);
	struct ser_softc *sc;
	struct tty *tp;
	int s, s2;
	int error = 0;
 
	sc = device_lookup_private(&ser_cd, unit);
	if (sc == NULL)
		return ENXIO;

	if (!sc->sc_tty) {
		tp = sc->sc_tty = tty_alloc();
		tty_attach(tp);
	} else
		tp = sc->sc_tty;

	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp))
		return EBUSY;

	s = spltty();

	/*
	 * Do the following if this is a first open.
	 */
	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		struct termios t;

		/* Turn on interrupts. */
		sc->sc_imra = IA_RRDY|IA_RERR|IA_TRDY|IA_TERR;
		sc->sc_imrb = IB_SCTS|IB_SDCD;
		single_inst_bset_b(MFP->mf_imra, sc->sc_imra);
		single_inst_bset_b(MFP->mf_imrb, sc->sc_imrb);

		/* Fetch the current modem control status, needed later. */
		sc->sc_msr = ~MFP->mf_gpip & (IO_SDCD|IO_SCTS|IO_SRI);

		/* Add some entry points needed by the tty layer. */
		tp->t_oproc = serstart;
		tp->t_param = serparam;
		tp->t_hwiflow = serhwiflow;
		tp->t_dev = dev;

		/*
		 * Initialize the termios status to the defaults.  Add in the
		 * sticky bits from TIOCSFLAGS.
		 */
		t.c_ispeed = 0;
		if (ISSET(sc->sc_hwflags, SER_HW_CONSOLE)) {
			t.c_ospeed = CONSBAUD;
			t.c_cflag  = CONSCFLAG;
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
		(void)serparam(tp, &t);
		ttsetwater(tp);

		s2 = splhigh();

		/*
		 * Turn on DTR.  We must always do this, even if carrier is not
		 * present, because otherwise we'd have to use TIOCSDTR
		 * immediately after setting CLOCAL.  We will drop DTR only on
		 * the next high-low transition of DCD, or by explicit request.
		 */
		ser_modem(sc, 1);

		/* Clear the input ring, and unblock. */
		sc->sc_rbput = sc->sc_rbget = 0;
		sc->sc_rbavail = RXBUFSIZE;
		ser_iflush(sc);
		sc->sc_rx_blocked = 0;
		ser_hwiflow(sc, 0);

#ifdef SER_DEBUG
		serstatus(sc, "seropen  ");
#endif

		splx(s2);
	}

	splx(s);

	error = ttyopen(tp, SERDIALOUT(dev), ISSET(flag, O_NONBLOCK));
	if (error)
		goto bad;

	error = (*tp->t_linesw->l_open)(dev, tp);
        if (error)
		goto bad;

	return 0;

bad:
	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		/*
		 * We failed to open the device, and nobody else had it opened.
		 * Clean up the state as appropriate.
		 */
		ser_shutdown(sc);
	}

	return error;
}
 
int
serclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	int unit = SERUNIT(dev);
	struct ser_softc *sc = device_lookup_private(&ser_cd, unit);
	struct tty *tp = sc->sc_tty;

	/* XXX This is for cons.c. */
	if (!ISSET(tp->t_state, TS_ISOPEN))
		return 0;

	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);

	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		/*
		 * Although we got a last close, the device may still be in
		 * use; e.g. if this was the dialout node, and there are still
		 * processes waiting for carrier on the non-dialout node.
		 */
		ser_shutdown(sc);
	}

	return 0;
}

int
serread(dev_t dev, struct uio *uio, int flag)
{
	struct ser_softc *sc = device_lookup_private(&ser_cd, SERUNIT(dev));
	struct tty *tp = sc->sc_tty;
 
	return (*tp->t_linesw->l_read)(tp, uio, flag);
}
 
int
serwrite(dev_t dev, struct uio *uio, int flag)
{
	struct ser_softc *sc = device_lookup_private(&ser_cd, SERUNIT(dev));
	struct tty *tp = sc->sc_tty;
 
	return (*tp->t_linesw->l_write)(tp, uio, flag);
}

int
serpoll(dev_t dev, int events, struct lwp *l)
{
	struct ser_softc *sc = device_lookup_private(&ser_cd, SERUNIT(dev));
	struct tty *tp = sc->sc_tty;
 
	return (*tp->t_linesw->l_poll)(tp, events, l);
}

struct tty *
sertty(dev_t dev)
{
	struct ser_softc *sc = device_lookup_private(&ser_cd, SERUNIT(dev));
	struct tty *tp = sc->sc_tty;

	return tp;
}

int
serioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	int unit = SERUNIT(dev);
	struct ser_softc *sc = device_lookup_private(&ser_cd, unit);
	struct tty *tp = sc->sc_tty;
	int error;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	error = ttioctl(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	switch (cmd) {
	case TIOCSBRK:
		ser_break(sc, 1);
		break;

	case TIOCCBRK:
		ser_break(sc, 0);
		break;

	case TIOCSDTR:
		ser_modem(sc, 1);
		break;

	case TIOCCDTR:
		ser_modem(sc, 0);
		break;

	case TIOCGFLAGS:
		*(int *)data = sc->sc_swflags;
		break;

	case TIOCSFLAGS:
		error = kauth_authorize_device_tty(l->l_cred,
		    KAUTH_DEVICE_TTY_PRIVSET, tp); 
		if (error)
			return error; 
		sc->sc_swflags = *(int *)data;
		break;

	case TIOCMSET:
	case TIOCMBIS:
	case TIOCMBIC:
	case TIOCMGET:
	default:
		return EPASSTHROUGH;
	}

#ifdef SER_DEBUG
	serstatus(sc, "serioctl ");
#endif

	return 0;
}

void
ser_break(struct ser_softc *sc, int onoff)
{
	int s;

	s = splhigh();
	if (onoff)
		SET(sc->sc_tsr, TSR_SBREAK);
	else
		CLR(sc->sc_tsr, TSR_SBREAK);

	if (!sc->sc_heldchange) {
		if (sc->sc_tx_busy) {
			sc->sc_heldtbc = sc->sc_tbc;
			sc->sc_tbc = 0;
			sc->sc_heldchange = 1;
		} else
			ser_loadchannelregs(sc);
	}
	splx(s);
}

void
ser_modem(struct ser_softc *sc, int onoff)
{
	int s;

	s = splhigh();
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
			ser_loadchannelregs(sc);
	}
	splx(s);
}

int
serparam(struct tty *tp, struct termios *t)
{
	struct ser_softc *sc =
	    device_lookup_private(&ser_cd, SERUNIT(tp->t_dev));
	int ospeed = serspeed(t->c_ospeed);
	uint8_t ucr;
	int s;


	/* check requested parameters */
	if (ospeed < 0)
		return EINVAL;
	if (t->c_ispeed && t->c_ispeed != t->c_ospeed)
		return EINVAL;

	sc->sc_rsr = RSR_ENAB;
	sc->sc_tsr = TSR_ENAB;

	ucr = UCR_CLKDIV;

	switch (ISSET(t->c_cflag, CSIZE)) {
	case CS5:
		SET(ucr, UCR_5BITS);
		break;
	case CS6:
		SET(ucr, UCR_6BITS);
		break;
	case CS7:
		SET(ucr, UCR_7BITS);
		break;
	case CS8:
		SET(ucr, UCR_8BITS);
		break;
	}
	if (ISSET(t->c_cflag, PARENB)) {
		SET(ucr, UCR_PENAB);
		if (!ISSET(t->c_cflag, PARODD))
			SET(ucr, UCR_PEVEN);
	}
	if (ISSET(t->c_cflag, CSTOPB))
		SET(ucr, UCR_STOPB2);
	else
		SET(ucr, UCR_STOPB1);

	s = splhigh();

	sc->sc_ucr = ucr;

	/*
	 * For the console, always force CLOCAL and !HUPCL, so that the port
	 * is always active.
	 */
	if (ISSET(sc->sc_swflags, TIOCFLAG_SOFTCAR) ||
	    ISSET(sc->sc_hwflags, SER_HW_CONSOLE)) {
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
		sc->sc_msr_dcd = MCR_DCD;
	/*
	 * Set the flow control pins depending on the current flow control
	 * mode.
	 */
	if (ISSET(t->c_cflag, CRTSCTS)) {
		sc->sc_mcr_dtr = MCR_DTR;
		sc->sc_mcr_rts = MCR_RTS;
		sc->sc_msr_cts = MCR_CTS;
		sc->sc_r_hiwat = RXHIWAT;
	} else if (ISSET(t->c_cflag, MDMBUF)) {
		/*
		 * For DTR/DCD flow control, make sure we don't toggle DTR for
		 * carrier detection.
		 */
		sc->sc_mcr_dtr = 0;
		sc->sc_mcr_rts = MCR_DTR;
		sc->sc_msr_cts = MCR_DCD;
		sc->sc_r_hiwat = RXHIWAT;
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

	sc->sc_ospeed = ospeed;
	
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
			ser_loadchannelregs(sc);
	}

	splx(s);

	/*
	 * Update the tty layer's idea of the carrier bit, in case we changed
	 * CLOCAL or MDMBUF.  We don't hang up here; we only do that if we
	 * lose carrier while carrier detection is on.
	 */
	(void)(*tp->t_linesw->l_modem)(tp, ISSET(sc->sc_msr, MCR_DCD));

#ifdef SER_DEBUG
	serstatus(sc, "serparam ");
#endif

	/* XXXXX FIX ME */
	/* Block or unblock as needed. */
	if (!ISSET(t->c_cflag, CHWFLOW)) {
		if (sc->sc_rx_blocked) {
			sc->sc_rx_blocked = 0;
			ser_hwiflow(sc, 0);
		}
		if (sc->sc_tx_stopped) {
			sc->sc_tx_stopped = 0;
			serstart(tp);
		}
	} else {
#if 0
		sermsrint(sc, tp);
#endif
	}

	return 0;
}

void
ser_iflush(struct ser_softc *sc)
{
	uint8_t tmp;

	/* flush any pending I/O */
	while (ISSET(MFP->mf_rsr, RSR_CIP|RSR_BFULL))
		tmp = MFP->mf_udr;
	__USE(tmp);
}

void
ser_loadchannelregs(struct ser_softc *sc)
{

	/* XXXXX necessary? */
	ser_iflush(sc);

	/*
	 * No interrupts please...
	 */
	if ((MFP->mf_imra & (IA_RRDY|IA_RERR|IA_TRDY|IA_TERR)) != sc->sc_imra) {
		printf("loadchannelregs: mf_imra: %x sc_imra: %x\n",
		    (u_int)MFP->mf_imra, (u_int)sc->sc_imra);
	}
	if ((MFP->mf_imrb & (IB_SCTS|IB_SDCD)) != sc->sc_imrb) {
		printf("loadchannelregs: mf_imrb: %x sc_imrb: %x\n",
		     (u_int)MFP->mf_imrb, (u_int)sc->sc_imrb);
	}
	single_inst_bclr_b(MFP->mf_imra, IA_RRDY|IA_RERR|IA_TRDY|IA_TERR);
	single_inst_bclr_b(MFP->mf_imrb, IB_SCTS|IB_SDCD);

	MFP->mf_ucr = sc->sc_ucr;
	MFP->mf_rsr = sc->sc_rsr;
	MFP->mf_tsr = sc->sc_tsr;

	single_inst_bclr_b(MFP->mf_tcdcr, 0x07);
	MFP->mf_tddr  = sc->sc_ospeed;
	single_inst_bset_b(MFP->mf_tcdcr, (sc->sc_ospeed >> 8) & 0x0f);

	sc->sc_mcr_active = sc->sc_mcr;

	if (machineid & ATARI_HADES) {
		/* PCB fault, wires exchanged..... */
		ym2149_rts(!(sc->sc_mcr_active & MCR_DTR));
		ym2149_dtr(!(sc->sc_mcr_active & MCR_RTS));
	} else {
		ym2149_rts(!(sc->sc_mcr_active & MCR_RTS));
		ym2149_dtr(!(sc->sc_mcr_active & MCR_DTR));
	}

	single_inst_bset_b(MFP->mf_imra, sc->sc_imra);
	single_inst_bset_b(MFP->mf_imrb, sc->sc_imrb);
}

int
serhwiflow(struct tty *tp, int block)
{
	struct ser_softc *sc =
	    device_lookup_private(&ser_cd, SERUNIT(tp->t_dev));
	int s;

	if (sc->sc_mcr_rts == 0)
		return 0;

	s = splhigh();
	if (block) {
		/*
		 * The tty layer is asking us to block input.
		 * If we already did it, just return TRUE.
		 */
		if (sc->sc_rx_blocked)
			goto out;
		sc->sc_rx_blocked = 1;
	} else {
		/*
		 * The tty layer is asking us to resume input.
		 * The input ring is always empty by now.
		 */
		sc->sc_rx_blocked = 0;
	}
	ser_hwiflow(sc, block);
out:
	splx(s);
	return 1;
}

/*
 * (un)block input via hw flowcontrol
 */
void
ser_hwiflow(struct ser_softc *sc, int block)
{

	if (sc->sc_mcr_rts == 0)
		return;

	if (block) {
		CLR(sc->sc_mcr, sc->sc_mcr_rts);
		CLR(sc->sc_mcr_active, sc->sc_mcr_rts);
	} else {
		SET(sc->sc_mcr, sc->sc_mcr_rts);
		SET(sc->sc_mcr_active, sc->sc_mcr_rts);
	}
	if (machineid & ATARI_HADES) {
		/* PCB fault, wires exchanged..... */
		ym2149_dtr(sc->sc_mcr_active & MCR_RTS);
	}
	else {
		ym2149_rts(sc->sc_mcr_active & MCR_RTS);
	}
}

void
serstart(struct tty *tp)
{
	struct ser_softc *sc =
	    device_lookup_private(&ser_cd, SERUNIT(tp->t_dev));
	int s;

	s = spltty();
	if (ISSET(tp->t_state, TS_BUSY))
		goto out;
	if (ISSET(tp->t_state, TS_TIMEOUT | TS_TTSTOP))
		goto stopped;

	if (sc->sc_tx_stopped)
		goto stopped;
	if (!ttypull(tp))
		goto stopped;

	/* Grab the first contiguous region of buffer space. */
	{
		uint8_t *tba;
		int tbc;

		tba = tp->t_outq.c_cf;
		tbc = ndqb(&tp->t_outq, 0);

		(void)splhigh();

		sc->sc_tba = tba;
		sc->sc_tbc = tbc;
	}

	SET(tp->t_state, TS_BUSY);
	sc->sc_tx_busy = 1;

	/* Enable transmit completion interrupts if necessary. */
	if (!ISSET(sc->sc_imra, IA_TRDY)) {
		SET(sc->sc_imra, IA_TRDY|IA_TERR);
		single_inst_bset_b(MFP->mf_imra, IA_TRDY|IA_TERR);
	}

	/* Output the first char */
	MFP->mf_udr = *sc->sc_tba;
	sc->sc_tbc--;
	sc->sc_tba++;

	splx(s);
	return;

stopped:
	/* Disable transmit completion interrupts if necessary. */
	if (ISSET(sc->sc_imra, IA_TRDY)) {
		CLR(sc->sc_imra, IA_TRDY|IA_TERR);
		single_inst_bclr_b(MFP->mf_imra, IA_TRDY|IA_TERR);
	}
out:
	splx(s);
	return;
}

/*
 * Stop output on a line.
 */
void
serstop(struct tty *tp, int flag)
{
	struct ser_softc *sc =
	    device_lookup_private(&ser_cd, SERUNIT(tp->t_dev));
	int s;

	s = splhigh();
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
serdiag(void *arg)
{
	struct ser_softc *sc = arg;
	int overflows, floods;
	int s;

	s = splhigh();
	overflows = sc->sc_overflows;
	sc->sc_overflows = 0;
	floods = sc->sc_floods;
	sc->sc_floods = 0;
	sc->sc_errors = 0;
	splx(s);

	log(LOG_WARNING,
	    "%s: %d silo overflow%s, %d ibuf flood%s\n",
	    device_xname(sc->sc_dev),
	    overflows, overflows == 1 ? "" : "s",
	    floods, floods == 1 ? "" : "s");
}

static void
ser_shutdown(struct ser_softc *sc)
{
	int s;
	struct tty *tp = sc->sc_tty;


	s = splhigh();
	
	/* If we were asserting flow control, then deassert it. */
	sc->sc_rx_blocked = 1;
	ser_hwiflow(sc, 1);

	/* Clear any break condition set with TIOCSBRK. */
	ser_break(sc, 0);

	/*
	 * Hang up if necessary.  Wait a bit, so the other side has time to
	 * notice even if we immediately open the port again.
	 */
	if (ISSET(tp->t_cflag, HUPCL)) {
		ser_modem(sc, 0);
		(void)tsleep(sc, TTIPRI, ttclos, hz);
	}

	/* Turn off interrupts. */
	CLR(sc->sc_imra, IA_RRDY|IA_RERR|IA_TRDY|IA_TERR);
	CLR(sc->sc_imrb, IB_SCTS|IB_SDCD);
	single_inst_bclr_b(MFP->mf_imrb, IB_SCTS|IB_SDCD);
	single_inst_bclr_b(MFP->mf_imra, IA_RRDY|IA_RERR|IA_TRDY|IA_TERR);
	splx(s);
}

static void
serrxint(struct ser_softc *sc, struct tty *tp)
{
	u_int get, cc, scc;
	int code;
	uint8_t rsr;
	int s;
	static const int lsrmap[8] = {
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
			callout_reset(&sc->sc_diag_ch, 60 * hz, serdiag, sc);
	}

	while (cc--) {
		rsr = sc->sc_lbuf[get];
		if (ISSET(rsr, RSR_BREAK)) {
#ifdef DDB
			if (ISSET(sc->sc_hwflags, SER_HW_CONSOLE))
				Debugger();
#endif
		} else if (ISSET(rsr, RSR_OERR)) {
			sc->sc_overflows++;
			if (sc->sc_errors++ == 0)
				callout_reset(&sc->sc_diag_ch, 60 * hz,
				    serdiag, sc);
		}
		code = sc->sc_rbuf[get] |
		    lsrmap[(rsr & (RSR_BREAK|RSR_FERR|RSR_PERR)) >> 3];
		(*tp->t_linesw->l_rint)(code, tp);
		get = (get + 1) & RXBUFMASK;
	}

	sc->sc_rbget = get;
	s = splhigh();
	sc->sc_rbavail += scc;
	/*
	 * Buffers should be ok again, release possible block, but only if the
	 * tty layer isn't blocking too.
	 */
	if (sc->sc_rx_blocked && !ISSET(tp->t_state, TS_TBLOCK)) {
		sc->sc_rx_blocked = 0;
		ser_hwiflow(sc, 0);
	}
	splx(s);
}

static void
sertxint(struct ser_softc *sc, struct tty *tp)
{

	CLR(tp->t_state, TS_BUSY);
	if (ISSET(tp->t_state, TS_FLUSH))
		CLR(tp->t_state, TS_FLUSH);
	else
		ndflush(&tp->t_outq, (int)(sc->sc_tba - tp->t_outq.c_cf));
	(*tp->t_linesw->l_start)(tp);
}

static void
sermsrint(struct ser_softc *sc, struct tty *tp)
{
	uint8_t msr, delta;
	int s;

	s = splhigh();
	msr = sc->sc_msr;
	delta = sc->sc_msr_delta;
	sc->sc_msr_delta = 0;
	splx(s);

	if (ISSET(delta, sc->sc_msr_dcd)) {
		/*
		 * Inform the tty layer that carrier detect changed.
		 */
		(void)(*tp->t_linesw->l_modem)(tp, ISSET(msr, MCR_DCD));
	}

	if (ISSET(delta, sc->sc_msr_cts)) {
		/* Block or unblock output according to flow control. */
		if (ISSET(msr, sc->sc_msr_cts)) {
			sc->sc_tx_stopped = 0;
			(*tp->t_linesw->l_start)(tp);
		} else {
			sc->sc_tx_stopped = 1;
			serstop(tp, 0);
		}
	}

#ifdef SER_DEBUG
	serstatus(sc, "sermsrint");
#endif
}

void
sersoft(void *arg)
{
	struct ser_softc *sc = arg;
	struct tty *tp;

	tp = sc->sc_tty;
	if (tp == NULL)
		return;

	if (!ISSET(tp->t_state, TS_ISOPEN) && (tp->t_wopen == 0))
		return;
	
	if (sc->sc_rx_ready) {
		sc->sc_rx_ready = 0;
		serrxint(sc, tp);
	}

	if (sc->sc_st_check) {
		sc->sc_st_check = 0;
		sermsrint(sc, tp);
	}

	if (sc->sc_tx_done) {
		sc->sc_tx_done = 0;
		sertxint(sc, tp);
	}
}

int
sermintr(void *arg)
{
	struct ser_softc *sc = arg;
	uint8_t msr, delta;

	msr = ~MFP->mf_gpip;
	delta = msr ^ sc->sc_msr;
	sc->sc_msr  = sc->sc_msr & ~(MCR_CTS|MCR_DCD|MCR_RI);
	sc->sc_msr |= msr & (MCR_CTS|MCR_DCD|MCR_RI);

	if (ISSET(delta, sc->sc_msr_mask)) {
		sc->sc_msr_delta |= delta;

		/*
		 * Stop output immediately if we lose the output
		 * flow control signal or carrier detect.
		 */
		if (ISSET(~msr, sc->sc_msr_mask)) {
			sc->sc_tbc = 0;
			sc->sc_heldtbc = 0;
#ifdef SER_DEBUG
			serstatus(sc, "sermintr  ");
#endif
		}

		sc->sc_st_check = 1;
	}
	softint_schedule(sc->sc_sicookie);
	return 1;
}

int
sertrintr(void *arg)
{
	struct ser_softc *sc = arg;
	u_int put, cc;
	uint8_t rsr, tsr;

	put = sc->sc_rbput;
	cc = sc->sc_rbavail;

	rsr = MFP->mf_rsr;
	if (ISSET(rsr, RSR_BFULL|RSR_BREAK)) {
		for (; ISSET(rsr, RSR_BFULL|RSR_BREAK) && cc > 0; cc--) {
			sc->sc_rbuf[put] = MFP->mf_udr;
			sc->sc_lbuf[put] = rsr;
			put = (put + 1) & RXBUFMASK;
			if ((rsr & RSR_BREAK) && (MFP->mf_rsr & RSR_BREAK))
				rsr = 0;
			else
				rsr = MFP->mf_rsr;
		}
		/*
		 * Current string of incoming characters ended because
		 * no more data was available. Schedule a receive event
		 * if any data was received. Drop any characters that
		 * we couldn't handle.
		 */
		sc->sc_rbput    = put;
		sc->sc_rbavail  = cc;
		sc->sc_rx_ready = 1;
		/*
		 * See if we are in danger of overflowing a buffer. If
		 * so, use hardware flow control to ease the pressure.
		 */
		if (sc->sc_rx_blocked == 0 &&
		    cc < sc->sc_r_hiwat) {
			sc->sc_rx_blocked = 1;
			ser_hwiflow(sc, 1);
		}
		/*
		 * If we're out of space, throw away any further input.
		 */
		if (!cc) {
			while (ISSET(rsr, RSR_BFULL|RSR_BREAK)) {
				rsr = MFP->mf_udr;
				rsr = MFP->mf_rsr;
			}
		}
	}

	/*
	 * Done handling any receive interrupts. See if data can be
	 * transmitted as well. Schedule tx done event if no data left
	 * and tty was marked busy.
	 */
	tsr = MFP->mf_tsr;
	if (ISSET(tsr, TSR_BE)) {
		/*
		 * If we've delayed a parameter change, do it now, and restart
		 * output.
		 */
		if (sc->sc_heldchange) {
			ser_loadchannelregs(sc);
			sc->sc_heldchange = 0;
			sc->sc_tbc = sc->sc_heldtbc;
			sc->sc_heldtbc = 0;
		}
		/* Output the next character, if any. */
		if (sc->sc_tbc > 0) {
			MFP->mf_udr = *sc->sc_tba;
			sc->sc_tbc--;
			sc->sc_tba++;
		} else if (sc->sc_tx_busy) {
			sc->sc_tx_busy = 0;
			sc->sc_tx_done = 1;
		}
	}
	softint_schedule(sc->sc_sicookie);
	return 1;
}

static int
serspeed(long speed)
{
#define	divrnd(n, q)	(((n)*2/(q)+1)/2)	/* divide and round off */

	int div, x, err;

	if (speed <= 0)
		return -1;

	for (div = 4; div <= 64; div *= 4) {
		x = divrnd((SER_FREQ / div), speed);

		/*
		 * The value must fit in the timer-d dataregister. If
		 * not, try another delay-mode.
		 */
		if ((x / 2) > 255)
			continue;

		/*
		 * Baudrate to high for the interface or cannot be made
		 * within tolerance.
		 */
		if (x <= 0)
			return -1;

		err = divrnd((SER_FREQ / div) * 1000, speed * x) - 1000;
		if (err < 0)
			err = -err;
		if (err > SER_TOLERANCE)
			continue;

		/*
		 * Translate 'div' to delay-code
		 */
		if (div == 4)
			div = 1;
		else if (div == 16)
			div = 3;
		else if (div == 64)
			div = 5;

		return (x / 2) | (div << 8);
	}
	return -1;

#undef	divrnd
}

/*
 * Following are all routines needed for SER to act as console
 */
#include <dev/cons.h>

void
sercnprobe(struct consdev *cp)
{

	/*
	 * Activate serial console when DCD present...
	 */
	if (MFP->mf_gpip & MCR_DCD) {
		cp->cn_pri = CN_DEAD;
		return;
	}

	/* initialize required fields */
	/* XXX: LWP What unit? */
	cp->cn_dev = makedev(cdevsw_lookup_major(&ser_cdevsw), 0);
	if (serconsole)
		cp->cn_pri = CN_REMOTE;	/* Force a serial port console */
	else
		cp->cn_pri = CN_NORMAL;
}

void
sercninit(struct consdev *cp)
{

	serinitcons(CONSBAUD);
}

/*
 * Initialize UART to known state.
 */
void
serinit(int baud)
{
	int ospeed = serspeed(baud);

	MFP->mf_ucr = UCR_CLKDIV|UCR_8BITS|UCR_STOPB1;
	MFP->mf_rsr = RSR_ENAB;
	MFP->mf_tsr = TSR_ENAB;

	single_inst_bclr_b(MFP->mf_tcdcr, 0x07);
	MFP->mf_tddr  = ospeed;
	single_inst_bset_b(MFP->mf_tcdcr, (ospeed >> 8) & 0x0f);
}

/*
 * Set UART for console use. Do normal init, then enable interrupts.
 */
void
serinitcons(int baud)
{

	serinit(baud);

	/* Set rts/dtr */
	ym2149_rts(0);
	ym2149_dtr(0);

	single_inst_bset_b(MFP->mf_imra, (IA_RRDY|IA_RERR|IA_TRDY|IA_TERR));
}

int
sercngetc(dev_t dev)
{
	uint8_t stat, c;
	int s;

	s = splhigh();
	while (!ISSET(stat = MFP->mf_rsr, RSR_BFULL)) {
		if (!ISSET(stat, RSR_ENAB)) /* XXX */
			MFP->mf_rsr |= RSR_ENAB;
		if (stat & (RSR_FERR|RSR_PERR|RSR_OERR))
			c = MFP->mf_udr;
	}
	c = MFP->mf_udr;
	splx(s);
	return c;
}

#if 1
u_int s_imra;
u_int s_stat1, s_stat2, s_stat3;
#endif
void
sercnputc(dev_t dev, int c)
{
	int timo;
	uint8_t stat, imra;

	/* Mask serial interrupts */
	imra  = MFP->mf_imra & (IA_RRDY|IA_RERR|IA_TRDY|IA_TERR);
	single_inst_bclr_b(MFP->mf_imra, imra);
#if 1
	s_imra = imra;
#endif

	/* wait for any pending transmission to finish */
	timo = 50000;
#if 1
	s_stat1 = MFP->mf_tsr;
#endif
	while (!ISSET(stat = MFP->mf_tsr, TSR_BE) && --timo)
		;
	MFP->mf_udr = c;
	/* wait for this transmission to complete */
	timo = 1500000;
#if 1
	s_stat2 = MFP->mf_tsr;
#endif
	while (!ISSET(stat = MFP->mf_tsr, TSR_BE) && --timo)
		;

#if 1
	s_stat3 = MFP->mf_tsr;
#endif
	/* Clear pending serial interrupts and re-enable */
	MFP->mf_ipra = (uint8_t)~imra;
	single_inst_bset_b(MFP->mf_imra, imra);
}

void
sercnpollc(dev_t dev, int on)
{

}
