/*	$NetBSD: clmpcc.c,v 1.1 1999/02/13 17:05:19 scw Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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
 * Cirrus Logic CD2400/CD2401 Four Channel Multi-Protocol Comms. Controller.
 */

#include "opt_ddb.h"

#include <sys/types.h>
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
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/bus.h>

#include <dev/ic/clmpccreg.h>
#include <dev/ic/clmpccvar.h>
#include <dev/cons.h>


#if defined(CLMPCC_ONLY_BYTESWAP_LOW) && defined(CLMPCC_ONLY_BYTESWAP_HIGH)
#error	"CLMPCC_ONLY_BYTESWAP_LOW and CLMPCC_ONLY_BYTESWAP_HIGH are mutually exclusive."
#endif


static int	clmpcc_init	__P((struct clmpcc_softc *sc));
static void	clmpcc_shutdown	__P((struct clmpcc_chan *));
static int	clmpcc_speed	__P((struct clmpcc_softc *, speed_t,
					int *, int *));
static int	clmpcc_param	__P((struct tty *, struct termios *));
static void	clmpcc_start	__P((struct tty *));
static int 	clmpcc_modem_control	__P((struct clmpcc_chan *, int, int));


cdev_decl(clmpcc);

#define	CLMPCCUNIT(x)		(minor(x) & 0x7fffc)
#define CLMPCCCHAN(x)		(minor(x) & 0x00003)
#define	CLMPCCDIALOUT(x)	(minor(x) & 0x80000)

/*
 * These should be in a header file somewhere...
 */
#define	ISSET(v, f)	(((v) & (f)) != 0)
#define	ISCLR(v, f)	(((v) & (f)) == 0)
#define SET(v, f)	(v) |= (f)
#define CLR(v, f)	(v) &= ~(f)


extern struct cfdriver clmpcc_cd;


/*
 * Make this an option variable one can patch.
 * But be warned:  this must be a power of 2!
 */
u_int clmpcc_ibuf_size = CLMPCC_RING_SIZE;


/*
 * Things needed when the device is used as a console
 */
static struct clmpcc_softc *cons_sc = NULL;
static int cons_chan;
static int cons_rate;

static int	clmpcc_common_getc	__P((struct clmpcc_softc *, int));
static void	clmpcc_common_putc	__P((struct clmpcc_softc *, int, int));
int		clmpcccngetc	__P((dev_t));
void		clmpcccnputc	__P((dev_t, int));


/*
 * Convenience functions, inlined for speed
 */
#define	integrate   static inline
integrate u_int8_t  clmpcc_rdreg __P((struct clmpcc_softc *, u_int));
integrate void      clmpcc_wrreg __P((struct clmpcc_softc *, u_int, u_int));
integrate u_int8_t  clmpcc_rdreg_odd __P((struct clmpcc_softc *, u_int));
integrate void      clmpcc_wrreg_odd __P((struct clmpcc_softc *, u_int, u_int));
integrate u_int8_t  clmpcc_select_channel __P((struct clmpcc_softc *, u_int));
integrate void      clmpcc_channel_cmd __P((struct clmpcc_softc *,int,int));
integrate void      clmpcc_enable_transmitter __P((struct clmpcc_chan *));

#define clmpcc_rd_msvr(s)	clmpcc_rdreg_odd(s,CLMPCC_REG_MSVR)
#define clmpcc_wr_msvr(s,r,v)	clmpcc_wrreg_odd(s,r,v)
#define clmpcc_wr_pilr(s,r,v)	clmpcc_wrreg_odd(s,r,v)
#define clmpcc_rd_rxdata(s)	clmpcc_rdreg_odd(s,CLMPCC_REG_RDR)
#define clmpcc_wr_txdata(s,v)	clmpcc_wrreg_odd(s,CLMPCC_REG_TDR,v)


integrate u_int8_t
clmpcc_rdreg(sc, offset)
	struct clmpcc_softc *sc;
	u_int offset;
{
#if !defined(CLMPCC_ONLY_BYTESWAP_LOW) && !defined(CLMPCC_ONLY_BYTESWAP_HIGH)
	offset ^= sc->sc_byteswap;
#elif defined(CLMPCC_ONLY_BYTESWAP_HIGH)
	offset ^= CLMPCC_BYTESWAP_HIGH;
#endif
	return bus_space_read_1(sc->sc_iot, sc->sc_ioh, offset);
}

integrate void
clmpcc_wrreg(sc, offset, val)
	struct clmpcc_softc *sc;
	u_int offset;
	u_int val;
{
#if !defined(CLMPCC_ONLY_BYTESWAP_LOW) && !defined(CLMPCC_ONLY_BYTESWAP_HIGH)
	offset ^= sc->sc_byteswap;
#elif defined(CLMPCC_ONLY_BYTESWAP_HIGH)
	offset ^= CLMPCC_BYTESWAP_HIGH;
#endif
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, offset, val);
}

integrate u_int8_t
clmpcc_rdreg_odd(sc, offset)
	struct clmpcc_softc *sc;
	u_int offset;
{
#if !defined(CLMPCC_ONLY_BYTESWAP_LOW) && !defined(CLMPCC_ONLY_BYTESWAP_HIGH)
	offset ^= (sc->sc_byteswap & 2);
#elif defined(CLMPCC_ONLY_BYTESWAP_HIGH)
	offset ^= (CLMPCC_BYTESWAP_HIGH & 2);
#endif
	return bus_space_read_1(sc->sc_iot, sc->sc_ioh, offset);
}

integrate void
clmpcc_wrreg_odd(sc, offset, val)
	struct clmpcc_softc *sc;
	u_int offset;
	u_int val;
{
#if !defined(CLMPCC_ONLY_BYTESWAP_LOW) && !defined(CLMPCC_ONLY_BYTESWAP_HIGH)
	offset ^= (sc->sc_byteswap & 2);
#elif defined(CLMPCC_ONLY_BYTESWAP_HIGH)
	offset ^= (CLMPCC_BYTESWAP_HIGH & 2);
#endif
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, offset, val);
}

integrate u_int8_t
clmpcc_select_channel(sc, new_chan)
	struct clmpcc_softc *sc;
	u_int new_chan;
{
	u_int old_chan = clmpcc_rdreg_odd(sc, CLMPCC_REG_CAR);

	clmpcc_wrreg_odd(sc, CLMPCC_REG_CAR, new_chan);

	return old_chan;
}

integrate void
clmpcc_channel_cmd(sc, chan, cmd)
	struct clmpcc_softc *sc;
	int chan;
	int cmd;
{
	int i;

	for (i = 5000; i; i--) {
		if ( clmpcc_rdreg(sc, CLMPCC_REG_CCR) == 0 )
			break;
		delay(1);
	}

	if ( i == 0 )
		printf("%s: channel %d command timeout (idle)\n",
			sc->sc_dev.dv_xname, chan);

	clmpcc_wrreg(sc, CLMPCC_REG_CCR, cmd);
}

integrate void
clmpcc_enable_transmitter(ch)
	struct clmpcc_chan *ch;
{
	u_int old;

	old = clmpcc_select_channel(ch->ch_sc, ch->ch_car);

	clmpcc_wrreg(ch->ch_sc, CLMPCC_REG_IER,
		clmpcc_rdreg(ch->ch_sc, CLMPCC_REG_IER) | CLMPCC_IER_TX_EMPTY);

	clmpcc_select_channel(ch->ch_sc, old);
}

static int
clmpcc_speed(sc, speed, cor, bpr)
	struct clmpcc_softc *sc;
	speed_t speed;
	int *cor, *bpr;
{
	int c, co, br;

	for (co = 0, c = 8; c <= 2048; co++, c *= 4) {
		br = ((sc->sc_clk / c) / speed) - 1;
		if ( br < 0x100 ) {
			*cor = co;
			*bpr = br;
			return 0;
		}
	}

	return -1;
}

void
clmpcc_attach(sc)
	struct clmpcc_softc *sc;
{
	struct clmpcc_chan *ch;
	struct tty *tp;
	int chan;

	if ( cons_sc != NULL &&
	     sc->sc_iot == cons_sc->sc_iot && sc->sc_ioh == cons_sc->sc_ioh )
		cons_sc = sc;

	/* Initialise the chip */
	clmpcc_init(sc);

	printf(": Cirrus Logic CD240%c Serial Controller\n",
		(clmpcc_rd_msvr(sc) & CLMPCC_MSVR_PORT_ID) ? '0' : '1');

	sc->sc_soft_running = 0;
	memset(&(sc->sc_chans[0]), 0, sizeof(sc->sc_chans));

	for (chan = 0; chan < CLMPCC_NUM_CHANS; chan++) {
		ch = &sc->sc_chans[chan];

		ch->ch_sc = sc;
		ch->ch_car = chan;

		tp = ttymalloc();
		tp->t_oproc = clmpcc_start;
		tp->t_param = clmpcc_param;

		ch->ch_tty = tp;

		ch->ch_ibuf = malloc(clmpcc_ibuf_size * 2, M_DEVBUF, M_NOWAIT);
		if ( ch->ch_ibuf == NULL ) {
			printf("%s(%d): unable to allocate ring buffer\n",
		    		sc->sc_dev.dv_xname, chan);
			return;
		}

		ch->ch_ibuf_end = &(ch->ch_ibuf[clmpcc_ibuf_size * 2]);
		ch->ch_ibuf_rd = ch->ch_ibuf_wr = ch->ch_ibuf;

		tty_attach(tp);
	}

	printf("%s: %d channels available", sc->sc_dev.dv_xname,
					    CLMPCC_NUM_CHANS);
	if ( cons_sc == sc ) {
		printf(", console on channel %d.\n", cons_chan);
		SET(sc->sc_chans[cons_chan].ch_flags, CLMPCC_FLG_IS_CONSOLE);
		SET(sc->sc_chans[cons_chan].ch_openflags, TIOCFLAG_SOFTCAR);
	} else
		printf(".\n");
}

static int
clmpcc_init(sc)
	struct clmpcc_softc *sc;
{
	u_int tcor, tbpr;
	u_int rcor, rbpr;
	u_int msvr_rts, msvr_dtr;
	u_int ccr;
	int is_console;
	int i;

	/*
	 * All we're really concerned about here is putting the chip
	 * into a quiescent state so that it won't do anything until
	 * clmpccopen() is called. (Except the console channel.)
	 */

	/*
	 * If the chip is acting as console, set all channels to the supplied
	 * console baud rate. Otherwise, plump for 9600.
	 */
	if ( cons_sc &&
	     sc->sc_ioh == cons_sc->sc_ioh && sc->sc_iot == cons_sc->sc_iot ) {
		clmpcc_speed(sc, cons_rate, &tcor, &tbpr);
		clmpcc_speed(sc, cons_rate, &rcor, &rbpr);
		is_console = 1;
	} else {
		clmpcc_speed(sc, 9600, &tcor, &tbpr);
		clmpcc_speed(sc, 9600, &rcor, &rbpr);
		is_console = 0;
	}

	/* Allow any pending output to be sent */
	delay(10000);

	/* Send the Reset All command  to channel 0 (resets all channels!) */
	clmpcc_channel_cmd(sc, 0, CLMPCC_CCR_T0_RESET_ALL);

	delay(1000);

	/*
	 * The chip will set it's firmware revision register to a non-zero
	 * value to indicate completion of reset.
	 */
	for (i = 10000; clmpcc_rdreg(sc, CLMPCC_REG_GFRCR) == 0 && i; i--)
		delay(1);

	if ( i == 0 ) {
		/*
		 * Watch out... If this chip is console, the message
		 * probably won't be sent since we just reset it!
		 */
		printf("%s: Failed to reset chip\n", sc->sc_dev.dv_xname);
		return -1;
	}

	for (i = 0; i < CLMPCC_NUM_CHANS; i++) {
		clmpcc_select_channel(sc, i);

		/* All interrupts are disabled to begin with */
		clmpcc_wrreg(sc, CLMPCC_REG_IER, 0);

		/* Make sure the channel interrupts on the correct vectors */
		clmpcc_wrreg(sc, CLMPCC_REG_LIVR, sc->sc_vector_base);
		clmpcc_wr_pilr(sc, CLMPCC_REG_RPILR, sc->sc_rpilr);
		clmpcc_wr_pilr(sc, CLMPCC_REG_TPILR, sc->sc_tpilr);
		clmpcc_wr_pilr(sc, CLMPCC_REG_MPILR, sc->sc_mpilr);

		/* Receive timer prescaler set to 1ms */
		clmpcc_wrreg(sc, CLMPCC_REG_TPR,
				 CLMPCC_MSEC_TO_TPR(sc->sc_clk, 1));

		/* We support Async mode only */
		clmpcc_wrreg(sc, CLMPCC_REG_CMR, CLMPCC_CMR_ASYNC);

		/* Set the required baud rate */
		clmpcc_wrreg(sc, CLMPCC_REG_TCOR, CLMPCC_TCOR_CLK(tcor));
		clmpcc_wrreg(sc, CLMPCC_REG_TBPR, tbpr);
		clmpcc_wrreg(sc, CLMPCC_REG_RCOR, CLMPCC_RCOR_CLK(rcor));
		clmpcc_wrreg(sc, CLMPCC_REG_RBPR, rbpr);

		/* Always default to 8N1 (XXX what about console?) */
		clmpcc_wrreg(sc, CLMPCC_REG_COR1, CLMPCC_COR1_CHAR_8BITS |
						  CLMPCC_COR1_NO_PARITY |
						  CLMPCC_COR1_IGNORE_PAR);

		clmpcc_wrreg(sc, CLMPCC_REG_COR2, 0);

		clmpcc_wrreg(sc, CLMPCC_REG_COR3, CLMPCC_COR3_STOP_1);

		clmpcc_wrreg(sc, CLMPCC_REG_COR4, CLMPCC_COR4_DSRzd |
						  CLMPCC_COR4_CDzd |
						  CLMPCC_COR4_CTSzd);

		clmpcc_wrreg(sc, CLMPCC_REG_COR5, CLMPCC_COR5_DSRod |
						  CLMPCC_COR5_CDod |
						  CLMPCC_COR5_CTSod |
						  CLMPCC_COR5_FLOW_NORM);

		clmpcc_wrreg(sc, CLMPCC_REG_COR6, 0);
		clmpcc_wrreg(sc, CLMPCC_REG_COR7, 0);

		/* Set the receive FIFO timeout */
		clmpcc_wrreg(sc, CLMPCC_REG_RTPRl, CLMPCC_RTPR_DEFAULT);
		clmpcc_wrreg(sc, CLMPCC_REG_RTPRh, 0);

		/* At this point, we set up the console differently */
		if ( is_console && i == cons_chan ) {
			msvr_rts = CLMPCC_MSVR_RTS;
			msvr_dtr = CLMPCC_MSVR_DTR;
			ccr = CLMPCC_CCR_T0_RX_EN | CLMPCC_CCR_T0_TX_EN;
		} else {
			msvr_rts = 0;
			msvr_dtr = 0;
			ccr = CLMPCC_CCR_T0_RX_DIS | CLMPCC_CCR_T0_TX_DIS;
		}

		clmpcc_wrreg(sc, CLMPCC_REG_MSVR_RTS, msvr_rts);
		clmpcc_wrreg(sc, CLMPCC_REG_MSVR_DTR, msvr_dtr);
		clmpcc_channel_cmd(sc, i, CLMPCC_CCR_T0_INIT | ccr);
		delay(100);
	}

	return 0;
}

static void
clmpcc_shutdown(ch)
	struct clmpcc_chan *ch;
{
	int oldch;

	oldch = clmpcc_select_channel(ch->ch_sc, ch->ch_car);

	/* Turn off interrupts. */
	clmpcc_wrreg(ch->ch_sc, CLMPCC_REG_IER, 0);

	if ( ISCLR(ch->ch_flags, CLMPCC_FLG_IS_CONSOLE) ) {
		/* Disable the transmitter and receiver */
		clmpcc_channel_cmd(ch->ch_sc, ch->ch_car, CLMPCC_CCR_T0_RX_DIS |
							  CLMPCC_CCR_T0_TX_DIS);

		/* Drop RTS and DTR */
		clmpcc_modem_control(ch, TIOCM_RTS | TIOCM_DTR, DMBIS);
	}

	clmpcc_select_channel(ch->ch_sc, oldch);
}

int
clmpccopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct clmpcc_softc *sc;
	struct clmpcc_chan *ch;
	struct tty *tp;
	int oldch;
	int error;
	int unit;
 
	if ( (unit = CLMPCCUNIT(dev)) >= clmpcc_cd.cd_ndevs ||
	     (sc = clmpcc_cd.cd_devs[unit]) == NULL ) {
		return ENXIO;
	}

	ch = &sc->sc_chans[CLMPCCCHAN(dev)];

	tp = ch->ch_tty;

	if ( ISSET(tp->t_state, TS_ISOPEN) &&
	     ISSET(tp->t_state, TS_XCLUDE) && p->p_ucred->cr_uid != 0 )
		return EBUSY;

	/*
	 * Do the following iff this is a first open.
	 */
	if ( ISCLR(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0 ) {

		ttychars(tp);

		tp->t_dev = dev;
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_cflag = TTYDEF_CFLAG;
		tp->t_ospeed = tp->t_ispeed = TTYDEF_SPEED;

		if ( ISSET(ch->ch_openflags, TIOCFLAG_CLOCAL) )
			SET(tp->t_cflag, CLOCAL);
		if ( ISSET(ch->ch_openflags, TIOCFLAG_CRTSCTS) )
			SET(tp->t_cflag, CRTSCTS);
		if ( ISSET(ch->ch_openflags, TIOCFLAG_MDMBUF) )
			SET(tp->t_cflag, MDMBUF);

		/*
		 * Override some settings if the channel is being
		 * used as the console.
		 */
		if ( ISSET(ch->ch_flags, CLMPCC_FLG_IS_CONSOLE) ) {
			tp->t_ospeed = tp->t_ispeed = cons_rate;
			SET(tp->t_cflag, CLOCAL);
			CLR(tp->t_cflag, CRTSCTS);
			CLR(tp->t_cflag, HUPCL);
		}

		ch->ch_control = 0;

		clmpcc_param(tp, &tp->t_termios);
		ttsetwater(tp);

		/* Clear the input ring */
		ch->ch_ibuf_rd = ch->ch_ibuf_wr = ch->ch_ibuf;

		/* Select the channel */
		oldch = clmpcc_select_channel(sc, ch->ch_car);

		/* Reset it */
		clmpcc_channel_cmd(sc, ch->ch_car, CLMPCC_CCR_T0_CLEAR |
						   CLMPCC_CCR_T0_RX_EN |
						   CLMPCC_CCR_T0_TX_EN);

		/* Enable receiver and modem change interrupts. */
		clmpcc_wrreg(sc, CLMPCC_REG_IER, CLMPCC_IER_MODEM |
						 CLMPCC_IER_RET |
						 CLMPCC_IER_RX_FIFO);

		/* Raise RTS and DTR */
		clmpcc_modem_control(ch, TIOCM_RTS | TIOCM_DTR, DMBIS);

		clmpcc_select_channel(sc, oldch);
	} else
	if ( ISSET(tp->t_state, TS_XCLUDE) && p->p_ucred->cr_uid != 0 )
		return EBUSY;
	
	error = ttyopen(tp, CLMPCCDIALOUT(dev), ISSET(flag, O_NONBLOCK));
	if (error)
		goto bad;

	error = (*linesw[tp->t_line].l_open)(dev, tp);
	if (error)
		goto bad;

	return 0;

bad:
	if ( ISCLR(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0 ) {
		/*
		 * We failed to open the device, and nobody else had it opened.
		 * Clean up the state as appropriate.
		 */
		clmpcc_shutdown(ch);
	}

	return error;
}
 
int
clmpccclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct clmpcc_softc	*sc = clmpcc_cd.cd_devs[CLMPCCUNIT(dev)];
	struct clmpcc_chan	*ch = &sc->sc_chans[CLMPCCCHAN(dev)];
	struct tty		*tp = ch->ch_tty;
	int s;

	if ( ISCLR(tp->t_state, TS_ISOPEN) )
		return 0;

	(*linesw[tp->t_line].l_close)(tp, flag);

	s = spltty();

	if ( ISCLR(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0 ) {
		/*
		 * Although we got a last close, the device may still be in
		 * use; e.g. if this was the dialout node, and there are still
		 * processes waiting for carrier on the non-dialout node.
		 */
		clmpcc_shutdown(ch);
	}

	ttyclose(tp);

	splx(s);

	return 0;
}
 
int
clmpccread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct clmpcc_softc *sc = clmpcc_cd.cd_devs[CLMPCCUNIT(dev)];
	struct tty *tp = sc->sc_chans[CLMPCCCHAN(dev)].ch_tty;
 
	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}
 
int
clmpccwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct clmpcc_softc *sc = clmpcc_cd.cd_devs[CLMPCCUNIT(dev)];
	struct tty *tp = sc->sc_chans[CLMPCCCHAN(dev)].ch_tty;
 
	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

struct tty *
clmpcctty(dev)
	dev_t dev;
{
	struct clmpcc_softc *sc = clmpcc_cd.cd_devs[CLMPCCUNIT(dev)];

	return (sc->sc_chans[CLMPCCCHAN(dev)].ch_tty);
}

int
clmpccioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct clmpcc_softc *sc = clmpcc_cd.cd_devs[CLMPCCUNIT(dev)];
	struct clmpcc_chan *ch = &sc->sc_chans[CLMPCCCHAN(dev)];
	struct tty *tp = ch->ch_tty;
	int error;

	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag, p);
	if (error >= 0)
		return error;

	error = ttioctl(tp, cmd, data, flag, p);
	if (error >= 0)
		return error;

	error = 0;

	switch (cmd) {
	case TIOCSBRK:
		SET(ch->ch_flags, CLMPCC_FLG_START_BREAK);
		clmpcc_enable_transmitter(ch);
		break;

	case TIOCCBRK:
		SET(ch->ch_flags, CLMPCC_FLG_END_BREAK);
		clmpcc_enable_transmitter(ch);
		break;

	case TIOCSDTR:
		clmpcc_modem_control(ch, TIOCM_DTR, DMBIS);
		break;

	case TIOCCDTR:
		clmpcc_modem_control(ch, TIOCM_DTR, DMBIC);
		break;

	case TIOCMSET:
		clmpcc_modem_control(ch, *((int *)data), DMSET);
		break;

	case TIOCMBIS:
		clmpcc_modem_control(ch, *((int *)data), DMBIS);
		break;

	case TIOCMBIC:
		clmpcc_modem_control(ch, *((int *)data), DMBIC);
		break;

	case TIOCMGET:
		*((int *)data) = clmpcc_modem_control(ch, 0, DMGET);
		break;

	case TIOCGFLAGS:
		*((int *)data) = ch->ch_openflags;
		break;

	case TIOCSFLAGS:
		error = suser(p->p_ucred, &p->p_acflag); 
		if ( error )
			break;
		ch->ch_openflags = *((int *)data) &
			(TIOCFLAG_SOFTCAR | TIOCFLAG_CLOCAL |
			 TIOCFLAG_CRTSCTS | TIOCFLAG_MDMBUF);
		if ( ISSET(ch->ch_flags, CLMPCC_FLG_IS_CONSOLE) )
			SET(ch->ch_openflags, TIOCFLAG_SOFTCAR);
		break;

	default:
		error = ENOTTY;
		break;
	}

	return error;
}

int
clmpcc_modem_control(ch, bits, howto)
	struct clmpcc_chan *ch;
	int bits;
	int howto;
{
	struct clmpcc_softc *sc = ch->ch_sc;
	struct tty *tp = ch->ch_tty;
	int oldch;
	int msvr;
	int rbits = 0;

	oldch = clmpcc_select_channel(sc, ch->ch_car);

	switch ( howto ) {
	case DMGET:
		msvr = clmpcc_rd_msvr(sc);

		if ( sc->sc_swaprtsdtr ) {
			rbits |= (msvr & CLMPCC_MSVR_RTS) ? TIOCM_DTR : 0;
			rbits |= (msvr & CLMPCC_MSVR_DTR) ? TIOCM_RTS : 0;
		} else {
			rbits |= (msvr & CLMPCC_MSVR_RTS) ? TIOCM_RTS : 0;
			rbits |= (msvr & CLMPCC_MSVR_DTR) ? TIOCM_DTR : 0;
		}

		rbits |= (msvr & CLMPCC_MSVR_CTS) ? TIOCM_CTS : 0;
		rbits |= (msvr & CLMPCC_MSVR_CD)  ? TIOCM_CD  : 0;
		rbits |= (msvr & CLMPCC_MSVR_DSR) ? TIOCM_DSR : 0;
		break;

	case DMSET:
		if ( sc->sc_swaprtsdtr ) {
		    if ( ISCLR(tp->t_cflag, CRTSCTS) )
			clmpcc_wr_msvr(sc, CLMPCC_REG_MSVR_DTR,
					bits & TIOCM_RTS ? CLMPCC_MSVR_DTR : 0);
		    clmpcc_wr_msvr(sc, CLMPCC_REG_MSVR_RTS,
				bits & TIOCM_DTR ? CLMPCC_MSVR_RTS : 0);
		} else {
		    if ( ISCLR(tp->t_cflag, CRTSCTS) )
			clmpcc_wr_msvr(sc, CLMPCC_REG_MSVR_RTS,
					bits & TIOCM_RTS ? CLMPCC_MSVR_RTS : 0);
		    clmpcc_wr_msvr(sc, CLMPCC_REG_MSVR_DTR,
				bits & TIOCM_DTR ? CLMPCC_MSVR_DTR : 0);
		}
		break;

	case DMBIS:
		if ( sc->sc_swaprtsdtr ) {
		    if ( ISCLR(tp->t_cflag, CRTSCTS) && ISSET(bits, TIOCM_RTS) )
			clmpcc_wr_msvr(sc,CLMPCC_REG_MSVR_DTR, CLMPCC_MSVR_DTR);
		    if ( ISSET(bits, TIOCM_DTR) )
			clmpcc_wr_msvr(sc,CLMPCC_REG_MSVR_RTS, CLMPCC_MSVR_RTS);
		} else {
		    if ( ISCLR(tp->t_cflag, CRTSCTS) && ISSET(bits, TIOCM_RTS) )
			clmpcc_wr_msvr(sc,CLMPCC_REG_MSVR_RTS, CLMPCC_MSVR_RTS);
		    if ( ISSET(bits, TIOCM_DTR) )
			clmpcc_wr_msvr(sc,CLMPCC_REG_MSVR_DTR, CLMPCC_MSVR_DTR);
		}
		break;

	case DMBIC:
		if ( sc->sc_swaprtsdtr ) {
		    if ( ISCLR(tp->t_cflag, CRTSCTS) && ISCLR(bits, TIOCM_RTS) )
			clmpcc_wr_msvr(sc, CLMPCC_REG_MSVR_DTR, 0);
		    if ( ISCLR(bits, TIOCM_DTR) )
			clmpcc_wr_msvr(sc, CLMPCC_REG_MSVR_RTS, 0);
		} else {
		    if ( ISCLR(tp->t_cflag, CRTSCTS) && ISCLR(bits, TIOCM_RTS) )
			clmpcc_wr_msvr(sc, CLMPCC_REG_MSVR_RTS, 0);
		    if ( ISCLR(bits, TIOCM_DTR) )
			clmpcc_wr_msvr(sc, CLMPCC_REG_MSVR_DTR, 0);
		}
		break;
	}

	clmpcc_select_channel(sc, oldch);

	return rbits;
}

static int
clmpcc_param(tp, t)
	struct tty *tp;
	struct termios *t;
{
	struct clmpcc_softc *sc = clmpcc_cd.cd_devs[CLMPCCUNIT(tp->t_dev)];
	struct clmpcc_chan *ch = &sc->sc_chans[CLMPCCCHAN(tp->t_dev)];
	int oclk, obpr;
	int iclk, ibpr;
	int oldch;
	int cor;
	int ier;
	int s;

	/* Check requested parameters. */
	if ( t->c_ospeed && clmpcc_speed(sc, t->c_ospeed, &oclk, &obpr) < 0 )
		return EINVAL;

	if ( t->c_ispeed && clmpcc_speed(sc, t->c_ispeed, &iclk, &ibpr) < 0 )
		return EINVAL;

	oldch = clmpcc_select_channel(sc, ch->ch_car);

	s = splhigh();
	/* Disable channel interrupt while we do all this */
	ier = clmpcc_rdreg(sc, CLMPCC_REG_IER);
	clmpcc_wrreg(sc, CLMPCC_REG_IER, 0);
	splx(s);

	/*
	 * For the console, always force CLOCAL and !HUPCL, so that the port
	 * is always active.
	 */
	if ( ISSET(ch->ch_openflags, TIOCFLAG_SOFTCAR) ||
	     ISSET(ch->ch_flags, CLMPCC_FLG_IS_CONSOLE) ) {
		SET(t->c_cflag, CLOCAL);
		CLR(t->c_cflag, HUPCL);
	}

	/* If ospeed it zero, hangup the line */
	clmpcc_modem_control(ch, TIOCM_DTR, t->c_ospeed == 0 ? DMBIC : DMBIS);

	if ( t->c_ospeed ) {
		clmpcc_wrreg(sc, CLMPCC_REG_TCOR, CLMPCC_TCOR_CLK(oclk));
		clmpcc_wrreg(sc, CLMPCC_REG_TBPR, obpr);
	}

	if ( t->c_ispeed ) {
		clmpcc_wrreg(sc, CLMPCC_REG_RCOR, CLMPCC_RCOR_CLK(iclk));
		clmpcc_wrreg(sc, CLMPCC_REG_RBPR, ibpr);
	}

	/* Work out value to use for COR1 */
	cor = 0;
	if ( ISSET(t->c_cflag, PARENB) ) {
		cor |= CLMPCC_COR1_NORM_PARITY;
		if ( ISSET(t->c_cflag, PARODD) )
			cor |= CLMPCC_COR1_ODD_PARITY;
	}

	if ( ISCLR(t->c_cflag, INPCK) )
		cor |= CLMPCC_COR1_IGNORE_PAR;

	switch ( t->c_cflag & CSIZE ) {
	  case CS5:
		cor |= CLMPCC_COR1_CHAR_5BITS;
		break;

	  case CS6:
		cor |= CLMPCC_COR1_CHAR_6BITS;
		break;

	  case CS7:
		cor |= CLMPCC_COR1_CHAR_7BITS;
		break;

	  case CS8:
		cor |= CLMPCC_COR1_CHAR_8BITS;
		break;
	}

	clmpcc_wrreg(sc, CLMPCC_REG_COR1, cor);

	/*
	 * The only interesting bit in COR2 is 'CTS Automatic Enable'
	 * when hardware flow control is in effect.
	 */
	cor = ISSET(t->c_cflag, CRTSCTS) ? CLMPCC_COR2_CtsAE : 0;
	clmpcc_wrreg(sc, CLMPCC_REG_COR2, cor);

	/* COR3 needs to be set to the number of stop bits... */
	cor = ISSET(t->c_cflag, CSTOPB) ? CLMPCC_COR3_STOP_2 :
					  CLMPCC_COR3_STOP_1;
	clmpcc_wrreg(sc, CLMPCC_REG_COR3, cor);

	/*
	 * COR4 contains the FIFO threshold setting.
	 * We adjust the threshold depending on the input speed...
	 */
	cor = clmpcc_rdreg(sc, CLMPCC_REG_COR4) & ~CLMPCC_COR4_FIFO_MASK;
	if ( t->c_ispeed <= 1200 )
		ch->ch_fifo = CLMPCC_COR4_FIFO_LOW;
	else if ( t->c_ispeed <= 19200 )
		ch->ch_fifo = CLMPCC_COR4_FIFO_MED;
	else
		ch->ch_fifo = CLMPCC_COR4_FIFO_HIGH;
	clmpcc_wrreg(sc, CLMPCC_REG_COR4, cor | ch->ch_fifo);

	/* This ensure the new fifo threshold causes an initial interrupt */
	ier |= CLMPCC_IER_RET;
	CLR(ch->ch_flags, CLMPCC_FLG_FIFO_CLEAR);

	/*
	 * If chip is used with CTS and DTR swapped, we can enable
	 * automatic hardware flow control.
	 */
	cor = clmpcc_rdreg(sc, CLMPCC_REG_COR5) & ~CLMPCC_COR5_FLOW_MASK;
	if ( sc->sc_swaprtsdtr && ISSET(t->c_cflag, CRTSCTS) )
		cor |= CLMPCC_COR5_FLOW_NORM;
	clmpcc_wrreg(sc, CLMPCC_REG_COR5, cor);

	/* The chip needs to be told that registers have changed... */
	clmpcc_channel_cmd(sc, ch->ch_car, CLMPCC_CCR_T0_INIT |
					   CLMPCC_CCR_T0_RX_EN |
					   CLMPCC_CCR_T0_TX_EN);

	/* Restore channel interrupts */
	clmpcc_wrreg(sc, CLMPCC_REG_IER, ier);

	/*
	 * Update the tty layer's idea of the carrier bit, in case we changed
	 * CLOCAL or MDMBUF.  We don't hang up here; we only do that by
	 * explicit request.
	 */
	cor = clmpcc_rd_msvr(sc) & CLMPCC_MSVR_CD;
	(void) (*linesw[tp->t_line].l_modem)(tp, cor != 0);

	clmpcc_select_channel(sc, oldch);

	return 0;
}

static void
clmpcc_start(tp)
	struct tty *tp;
{
	struct clmpcc_softc *sc = clmpcc_cd.cd_devs[CLMPCCUNIT(tp->t_dev)];
	struct clmpcc_chan *ch = &sc->sc_chans[CLMPCCCHAN(tp->t_dev)];
	int s;

	s = spltty();

	if ( ISCLR(tp->t_state, TS_TTSTOP | TS_TIMEOUT | TS_BUSY) ) {
		if ( tp->t_outq.c_cc <= tp->t_lowat ) {
			if ( ISSET(tp->t_state, TS_ASLEEP) ) {
				CLR(tp->t_state, TS_ASLEEP);
				wakeup(&tp->t_outq);
			}
			selwakeup(&tp->t_wsel);

			if ( tp->t_outq.c_cc == 0 )
				goto out;
		}
		SET(tp->t_state, TS_BUSY);
		clmpcc_enable_transmitter(ch);
	}

out:
	splx(s);
}

/*
 * Stop output on a line.
 */
void
clmpccstop(tp, flag)
	struct tty *tp;
	int flag;
{
	struct clmpcc_softc *sc = clmpcc_cd.cd_devs[CLMPCCUNIT(tp->t_dev)];
	struct clmpcc_chan *ch = &sc->sc_chans[CLMPCCCHAN(tp->t_dev)];
	int s;

	s = spltty();

	if ( ISSET(tp->t_state, TS_BUSY) ) {
		if ( ISCLR(tp->t_state, TS_TTSTOP) )
			SET(tp->t_state, TS_FLUSH);

		/*
		 * The transmit interrupt routine will disable transmit when it
		 * notices that CLMPCC_FLG_STOP has been set.
		 */
		SET(ch->ch_flags, CLMPCC_FLG_STOP);
	}
	splx(s);
}

/*
 * RX interrupt routine
 */
int
clmpcc_rxintr(arg)
	void *arg;
{
	struct clmpcc_softc *sc = (struct clmpcc_softc *)arg;
	struct clmpcc_chan *ch;
	u_int8_t *put, *end, rxd;
	u_char errstat;
	u_int fc, tc;
	int risr;
	int rir;
#ifdef DDB
	int saw_break = 0;
#endif

	/* Receive interrupt active? */
	rir = clmpcc_rdreg(sc, CLMPCC_REG_RIR);

	/*
	 * If we're using auto-vectored interrupts, we have to
	 * verify if the chip is generating the interrupt.
	 */
	if ( sc->sc_vector_base == 0 && (rir & CLMPCC_RIR_RACT) == 0 )
		return 0;

	/* Get pointer to interrupting channel's data structure */
	ch = &sc->sc_chans[rir & CLMPCC_RIR_RCN_MASK];

	/* Get the interrupt status register */
	risr = clmpcc_rdreg(sc, CLMPCC_REG_RISRl);
	if ( risr & CLMPCC_RISR_TIMEOUT ) {
		u_char reg;
		/*
		 * Set the FIFO threshold to zero, and disable
		 * further receive timeout interrupts.
		 */
		reg = clmpcc_rdreg(sc, CLMPCC_REG_COR4);
		clmpcc_wrreg(sc, CLMPCC_REG_COR4, reg & CLMPCC_COR4_FIFO_MASK);
		reg = clmpcc_rdreg(sc, CLMPCC_REG_IER);
		clmpcc_wrreg(sc, CLMPCC_REG_IER, reg & ~CLMPCC_IER_RET);
		clmpcc_wrreg(sc, CLMPCC_REG_REOIR, CLMPCC_REOIR_NO_TRANS);
		SET(ch->ch_flags, CLMPCC_FLG_FIFO_CLEAR);
		return 1;
	}

	/* How many bytes are waiting in the FIFO?  */
	fc = tc = clmpcc_rdreg(sc, CLMPCC_REG_RFOC) & CLMPCC_RFOC_MASK;

#ifdef DDB
	/*
	 * Allow BREAK on the console to drop to the debugger.
	 */
	if ( ISSET(ch->ch_flags, CLMPCC_FLG_IS_CONSOLE) &&
	     risr & CLMPCC_RISR_BREAK ) {
		saw_break = 1;
	}
#endif

	if ( ISCLR(ch->ch_tty->t_state, TS_ISOPEN) && fc ) {
		/* Just get rid of the data */
		while ( fc-- )
			(void) clmpcc_rd_rxdata(sc);
		goto rx_done;
	}

	put = ch->ch_ibuf_wr;
	end = ch->ch_ibuf_end;

	/*
	 * Note: The chip is completely hosed WRT these error
	 *       conditions; there seems to be no way to associate
	 *       the error with the correct character in the FIFO. 
	 *       We compromise by tagging the first character we read
	 *       with the error. Not perfect, but there's no other way.
	 */
	errstat = 0;
	if ( risr & CLMPCC_RISR_PARITY )
		errstat |= TTY_PE;
	if ( risr & (CLMPCC_RISR_FRAMING | CLMPCC_RISR_BREAK) )
		errstat |= TTY_FE;

	/*
	 * As long as there are characters in the FIFO, and we
	 * have space for them...
	 */
	while ( fc > 0 ) {

		*put++ = rxd = clmpcc_rd_rxdata(sc);
		*put++ = errstat;

		if ( put >= end )
			put = ch->ch_ibuf;

		if ( put == ch->ch_ibuf_rd ) {
			put -= 2;
			if ( put < ch->ch_ibuf )
				put = end - 2;
		}

		errstat = 0;
		fc--;
	}

	ch->ch_ibuf_wr = put;

#if 0
	if ( sc->sc_swaprtsdtr == 0 &&
	     ISSET(cy->cy_tty->t_cflag, CRTSCTS) && cc < ch->ch_r_hiwat) {
		/*
		 * If RTS/DTR are not physically swapped, we have to
		 * do hardware flow control manually
		 */
		clmpcc_wr_msvr(sc, CLMPCC_MSVR_RTS, 0);
	}
#endif

rx_done:
	if ( fc != tc ) {
		if ( ISSET(ch->ch_flags, CLMPCC_FLG_FIFO_CLEAR) ) {
			u_char reg;
			/*
			 * Set the FIFO threshold to the preset value,
			 * and enable receive timeout interrupts.
			 */
			reg = clmpcc_rdreg(sc, CLMPCC_REG_COR4);
			reg = (reg & ~CLMPCC_COR4_FIFO_MASK) | ch->ch_fifo;
			clmpcc_wrreg(sc, CLMPCC_REG_COR4, reg);
			reg = clmpcc_rdreg(sc, CLMPCC_REG_IER);
			clmpcc_wrreg(sc, CLMPCC_REG_IER, reg | CLMPCC_IER_RET);
			CLR(ch->ch_flags, CLMPCC_FLG_FIFO_CLEAR);
		}

		clmpcc_wrreg(sc, CLMPCC_REG_REOIR, 0);
		if ( sc->sc_soft_running == 0 ) {
			sc->sc_soft_running = 1;
			(sc->sc_softhook)(sc);
		}
	} else
		clmpcc_wrreg(sc, CLMPCC_REG_REOIR, CLMPCC_REOIR_NO_TRANS);

#ifdef DDB
	/*
	 * Only =after= we write REOIR is it safe to drop to the debugger.
	 */
	if ( saw_break )
		Debugger();
#endif

	return 1;
}

/*
 * Tx interrupt routine
 */
int
clmpcc_txintr(arg)
	void *arg;
{
	struct clmpcc_softc *sc = (struct clmpcc_softc *)arg;
	struct clmpcc_chan *ch;
	struct tty *tp;
	int ftc, oftc;
	int tir;

	/* Tx interrupt active? */
	tir = clmpcc_rdreg(sc, CLMPCC_REG_TIR);

	/*
	 * If we're using auto-vectored interrupts, we have to
	 * verify if the chip is generating the interrupt.
	 */
	if ( sc->sc_vector_base == 0 && (tir & CLMPCC_TIR_TACT) == 0 )
		return 0;

	/* Get pointer to interrupting channel's data structure */
	ch = &sc->sc_chans[tir & CLMPCC_TIR_TCN_MASK];

	/* Dummy read of the interrupt status register */
	(void) clmpcc_rdreg(sc, CLMPCC_REG_TISR);

	ftc = oftc = clmpcc_rdreg(sc, CLMPCC_REG_TFTC);

	/* Stop transmitting if CLMPCC_FLG_STOP is set */
	tp = ch->ch_tty;
	if ( ISSET(ch->ch_flags, CLMPCC_FLG_STOP) )
		goto tx_done;

	if ( tp->t_outq.c_cc > 0 ) {
		SET(tp->t_state, TS_BUSY);
		while (tp->t_outq.c_cc > 0 && ftc > 0 ) {
			clmpcc_wr_txdata(sc, getc(&tp->t_outq));
			ftc--;
		}
	} else {
		/*
		 * No data to send -- check if we should
		 * start/stop a break
		 */
		/*
		 * XXX does this cause too much delay before
		 * breaks?
		 */
		if ( ISSET(ch->ch_flags, CLMPCC_FLG_START_BREAK) ) {
			CLR(ch->ch_flags, CLMPCC_FLG_START_BREAK);
		}

		if ( ISSET(ch->ch_flags, CLMPCC_FLG_END_BREAK) ) {
			CLR(ch->ch_flags, CLMPCC_FLG_END_BREAK);
		}
	}

	if ( tp->t_outq.c_cc == 0 ) {
tx_done:
		/*
		 * No data to send or requested to stop.
		 * Disable transmit interrupt
		 */
		clmpcc_wrreg(ch->ch_sc, CLMPCC_REG_IER,
			clmpcc_rdreg(ch->ch_sc, CLMPCC_REG_IER) &
						~CLMPCC_IER_TX_EMPTY);
		CLR(ch->ch_flags, CLMPCC_FLG_STOP);
		CLR(tp->t_state, TS_BUSY);
	}

	if ( tp->t_outq.c_cc <= tp->t_lowat )
		SET(ch->ch_flags, CLMPCC_FLG_START);

	if ( ftc != oftc )
		clmpcc_wrreg(sc, CLMPCC_REG_TEOIR, 0);
	else
		clmpcc_wrreg(sc, CLMPCC_REG_TEOIR, CLMPCC_TEOIR_NO_TRANS);

	return 1;
}

/*
 * Modem change interrupt routine
 */
int
clmpcc_mdintr(arg)
	void *arg;
{
	struct clmpcc_softc *sc = (struct clmpcc_softc *)arg;
	int mir;

	/* Modem status interrupt active? */
	mir = clmpcc_rdreg(sc, CLMPCC_REG_MIR);

	/*
	 * If we're using auto-vectored interrupts, we have to
	 * verify if the chip is generating the interrupt.
	 */
	if ( sc->sc_vector_base == 0 && (mir & CLMPCC_MIR_MACT) == 0 )
		return 0;

	/* Dummy read of the interrupt status register */
	(void) clmpcc_rdreg(sc, CLMPCC_REG_MISR);

	/* Retrieve current status of modem lines. */
	sc->sc_chans[mir & CLMPCC_MIR_MCN_MASK].ch_control |=
		clmpcc_rd_msvr(sc) & CLMPCC_MSVR_CD;

	clmpcc_wrreg(sc, CLMPCC_REG_MEOIR, 0);

	if ( sc->sc_soft_running == 0 ) {
		sc->sc_soft_running = 1;
		(sc->sc_softhook)(sc);
	}

	return 1;
}

int
clmpcc_softintr(arg)
	void *arg;
{
	struct clmpcc_softc *sc = (struct clmpcc_softc *)arg;
	struct clmpcc_chan *ch;
	int (*rint) __P((int, struct tty *));
	u_char *get;
	u_int c;
	int chan;

	sc->sc_soft_running = 0;

	/* Handle Modem state changes too... */

	for (chan = 0; chan < CLMPCC_NUM_CHANS; chan++) {
		ch = &sc->sc_chans[chan];
		get = ch->ch_ibuf_rd;
		rint = linesw[ch->ch_tty->t_line].l_rint;

		/* Squirt buffered incoming data into the tty layer */
		while ( get != ch->ch_ibuf_wr ) {
			c = *get++;
			c |= ((u_int)*get++) << 8;
			(rint)(c, ch->ch_tty);

			if ( get == ch->ch_ibuf_end )
				get = ch->ch_ibuf;

			ch->ch_ibuf_rd = get;
		}
	}

	return 0;
}


/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/
/*
 * Following are all routines needed for a cd240x channel to act as console
 */
int
clmpcc_cnattach(sc, chan, rate)
	struct clmpcc_softc *sc;
	int chan;
	int rate;
{
	cons_sc = sc;
	cons_chan = chan;
	cons_rate = rate;

	return 0;
}

/*
 * The following functions are polled getc and putc routines, for console use.
 */
static int
clmpcc_common_getc(sc, chan)
	struct clmpcc_softc *sc;
	int chan;
{
	u_char old_chan;
	u_char old_ier;
	u_char ch, rir, risr;
	int s;

	s = splhigh();

	old_chan = clmpcc_select_channel(sc, chan);

	/*
	 * We have to put the channel into RX interrupt mode before
	 * trying to read the Rx data register. So save the previous
	 * interrupt mode.
	 */
	old_ier = clmpcc_rdreg(sc, CLMPCC_REG_IER);
	clmpcc_wrreg(sc, CLMPCC_REG_IER, CLMPCC_IER_RX_FIFO);

	/* Loop until we get a character */
	for (;;) {
		/*
		 * The REN bit will be set in the Receive Interrupt Register
		 * when the CD240x has a character to process. Remember,
		 * the RACT bit won't be set until we generate an interrupt
		 * acknowledge cycle via the MD front-end.
		 */
		rir = clmpcc_rdreg(sc, CLMPCC_REG_RIR);
		if ( (rir & CLMPCC_RIR_REN) == 0 )
			continue;

		/* Acknowledge the request */
		if ( sc->sc_iackhook )
			(sc->sc_iackhook)(sc, CLMPCC_IACK_RX);

		/*
		 * Determine if the interrupt is for the required channel
		 * and if valid data is available.
		 */
		rir = clmpcc_rdreg(sc, CLMPCC_REG_RIR);
		risr = clmpcc_rdreg(sc, CLMPCC_REG_RISR);
		if ( (rir & CLMPCC_RIR_RCN_MASK) != chan ||
		     risr != 0 ) {
			/* Rx error, or BREAK */
			clmpcc_wrreg(sc, CLMPCC_REG_REOIR,
					 CLMPCC_REOIR_NO_TRANS);
		} else {
			/* Dummy read of the FIFO count register */
			(void) clmpcc_rdreg(sc, CLMPCC_REG_RFOC);

			/* Fetch the received character */
			ch = clmpcc_rd_rxdata(sc);

			clmpcc_wrreg(sc, CLMPCC_REG_REOIR, 0);
			break;
		}
	}

	clmpcc_wrreg(sc, CLMPCC_REG_IER, old_ier);
	clmpcc_select_channel(sc, old_chan);

	splx(s);
	return ch;
}


static void
clmpcc_common_putc(sc, chan, c)
	struct clmpcc_softc *sc;
	int chan;
	int c;
{
	u_char old_chan;
	int s = splhigh();

	old_chan = clmpcc_select_channel(sc, chan);

	clmpcc_wrreg(sc, CLMPCC_REG_SCHR4, c);
	clmpcc_wrreg(sc, CLMPCC_REG_STCR, CLMPCC_STCR_SSPC(4) |
					  CLMPCC_STCR_SND_SPC);

	while ( clmpcc_rdreg(sc, CLMPCC_REG_STCR) != 0 )
		;

	delay(5);

	clmpcc_select_channel(sc, old_chan);

	splx(s);
}

int
clmpcccngetc(dev)
	dev_t dev;
{
	return clmpcc_common_getc(cons_sc, cons_chan);
}

/*
 * Console kernel output character routine.
 */
void
clmpcccnputc(dev, c)
	dev_t dev;
	int c;
{
	if ( c == '\n' )
		clmpcc_common_putc(cons_sc, cons_chan, '\r');

	clmpcc_common_putc(cons_sc, cons_chan, c);
}
