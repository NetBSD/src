/*	$NetBSD: txcom.c,v 1.3.2.2 1999/12/27 18:32:13 wrstuden Exp $ */

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include "opt_tx39_debug.h"
#include "opt_tx39uartdebug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/proc.h> /* tsleep/wakeup */

#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/file.h>

#include <sys/tty.h>
#include <sys/conf.h>
#include <dev/cons.h> /* consdev */

#include <machine/bus.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx39icureg.h>
#include <hpcmips/tx/tx39uartvar.h>
#include <hpcmips/tx/tx39uartreg.h>

#include <hpcmips/tx/tx39clockreg.h> /* XXX */

#define SET(t, f)	(t) |= (f)
#define CLR(t, f)	(t) &= ~(f)
#define ISSET(t, f)	((t) & (f))

#ifdef TX39UARTDEBUG
#define	DPRINTF(arg) printf arg
#else
#define	DPRINTF(arg)
#endif

#define MAXBUF	16
struct txcom_buf {
	int b_cnt;
	int b_in;
	int b_out;
	char b_buf[MAXBUF];
};

#define TXCOM_HW_CONSOLE 0x40
struct txcom_softc {
	struct	device sc_dev;
	struct tty *sc_tty;
	tx_chipset_tag_t sc_tc;
	int sc_slot;	/* UARTA or UARTB */
	int sc_cflag;	
	int sc_speed;
	struct txcom_buf *sc_rxbuf;
	struct txcom_buf *sc_txbuf;
	char **sc_msg;
	int sc_hwflags;
 	u_int8_t *sc_tba;		/* transmit buffer address */
 	int sc_tbc, sc_heldtbc;		/* transmit byte count */
	u_int8_t sc_rbuf; /* XXX */

};
volatile int	com_softrxintr_scheduled;

extern struct cfdriver txcom_cd;

int	txcom_match	__P((struct device*, struct cfdata*, void*));
void	txcom_attach	__P((struct device*, struct device*, void*));
int	txcom_txintr	__P((void*));
int	txcom_a_rxintr	__P((void*));
int	txcom_b_rxintr	__P((void*));
void	txcom_rxsoft	__P((void*));

int	txcom_cngetc		__P((dev_t));
void	txcom_cnputc		__P((dev_t, int));
void	txcom_cnpollc	__P((dev_t, int));

void	txcomstart __P((struct tty*));
int	txcomparam __P((struct tty*, struct termios*));
cdev_decl(txcom);

/* Serial console */
static	struct consdev txcomcons = {
	NULL, NULL, txcom_cngetc, txcom_cnputc, 
	txcom_cnpollc, NODEV, CN_NORMAL
};
static	struct txcom_softc cn_sc;

struct cfattach txcom_ca = {
	sizeof(struct txcom_softc), txcom_match, txcom_attach
};

int	txcom_enable __P((struct txcom_softc*));
void	txcom_disable __P((struct txcom_softc*));
void	txcom_setmode __P((struct txcom_softc*));
void	txcom_setbaudrate __P((struct txcom_softc*));

int
txcom_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	/* if the autoconfiguration got this far, there's a slot here */
	return 1;
}

void
txcom_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct tx39uart_attach_args *ua = aux;
	struct txcom_softc *sc = (void*)self;
	tx_chipset_tag_t tc;
	struct tty *tp;

	printf("\n");

	/* Check this slot used as serial console */
	if (ua->ua_slot == cn_sc.sc_slot &&
	    (cn_sc.sc_hwflags & TXCOM_HW_CONSOLE)) {
		memcpy(&cn_sc, self, sizeof(struct device));
		memcpy(self, &cn_sc, sizeof(struct txcom_softc));
	}

	tc = sc->sc_tc = ua->ua_tc;
	sc->sc_slot = ua->ua_slot;

	tp = ttymalloc();
	tp->t_oproc = txcomstart;
	tp->t_param = txcomparam;
	tp->t_hwiflow = NULL;
	sc->sc_tty = tp;
	cn_sc.sc_tty = tp; 
	tty_attach(tp);

	if (ISSET(sc->sc_hwflags, TXCOM_HW_CONSOLE)) {
		int maj;
		/* locate the major number */
		for (maj = 0; maj < nchrdev; maj++)
			if (cdevsw[maj].d_open == txcomopen)
				break;

		cn_tab->cn_dev = makedev(maj, sc->sc_dev.dv_unit);

		printf("%s: console\n", sc->sc_dev.dv_xname);
	}


	/* 
	 * Enable interrupt
	 */
	switch (sc->sc_slot) {
	case TX39_UARTA:
		tx_intr_establish(tc, MAKEINTR(2, TX39_INTRSTATUS2_UARTARXINT),
				    IST_EDGE, IPL_TTY,
				    txcom_a_rxintr, sc);
		break;
	case TX39_UARTB:
		tx_intr_establish(tc, MAKEINTR(2, TX39_INTRSTATUS2_UARTBRXINT),
				    IST_EDGE, IPL_TTY,
				    txcom_b_rxintr, sc);
		break;
	}
}

int
txcom_enable(sc)
	struct txcom_softc *sc;
{
	tx_chipset_tag_t tc;
	txreg_t reg;
	int slot;

	tc = sc->sc_tc;
	slot = sc->sc_slot;

	reg = tx_conf_read(tc, TX39_UARTCTRL1_REG(slot));
	/* Power */
	reg |= TX39_UARTCTRL1_ENUART;
	reg &= ~TX39_UARTCTRL1_ENBREAHALT;
	tx_conf_write(tc, TX39_UARTCTRL1_REG(slot), reg);
	/* 
	 * XXX Disable DMA (DMA not coded yet)
	 */
	reg &= ~(TX39_UARTCTRL1_ENDMARX | TX39_UARTCTRL1_ENDMATX);
	tx_conf_write(tc, TX39_UARTCTRL1_REG(slot), reg);

	/* XXX Clock */
	reg = tx_conf_read(tc, TX39_CLOCKCTRL_REG);
	reg |= (slot ? TX39_CLOCK_ENUARTBCLK : TX39_CLOCK_ENUARTACLK);
	tx_conf_write(tc, TX39_CLOCKCTRL_REG, reg);


	return 0;
}

void
txcom_disable(sc)
	struct txcom_softc *sc;
{
	tx_chipset_tag_t tc;
	txreg_t reg;
	int slot;

	tc = sc->sc_tc;
	slot = sc->sc_slot;

	reg = tx_conf_read(tc, TX39_UARTCTRL1_REG(slot));
	/* DMA */
	reg &= ~(TX39_UARTCTRL1_ENDMARX | TX39_UARTCTRL1_ENDMATX);
	/* Power */
	reg &= ~TX39_UARTCTRL1_ENUART;
	tx_conf_write(tc, TX39_UARTCTRL1_REG(slot), reg);
	/* Clock */
	reg = tx_conf_read(tc, TX39_CLOCKCTRL_REG);
	reg &= ~(slot ? TX39_CLOCK_ENUARTBCLK : TX39_CLOCK_ENUARTACLK);
	tx_conf_write(tc, TX39_CLOCKCTRL_REG, reg);
	
}

int
txcom_cnattach(slot, speed, cflag)
	int slot, speed, cflag;
{
	cn_tab = &txcomcons;

	cn_sc.sc_tc	= tx_conf_get_tag();
	cn_sc.sc_slot	= slot;
	cn_sc.sc_cflag	= cflag;
	cn_sc.sc_speed	= speed;
	cn_sc.sc_hwflags |= TXCOM_HW_CONSOLE;
#ifdef WINCE_DEFAULT_SETTING
#warning WINCE_DEFAULT_SETTING
#else
	txcom_enable(&cn_sc);
	txcom_setmode(&cn_sc);
	txcom_setbaudrate(&cn_sc);
#endif
	return 0;
}

int
txcom_cngetc(dev)
	dev_t dev;
{
	tx_chipset_tag_t tc;
	int ofs, c;
	tc = cn_sc.sc_tc;

	ofs = TX39_UARTCTRL1_REG(cn_sc.sc_slot);

	while(!(TX39_UARTCTRL1_RXHOLDFULL & tx_conf_read(tc, ofs)))
		;
	ofs = TX39_UARTRXHOLD_REG(cn_sc.sc_slot);
	c = TX39_UARTRXHOLD_RXDATA(tx_conf_read(tc, ofs));
	if (c == '\r') {
		c = '\n';
	}

	return c;
}

void
txcom_cnputc(dev, c)
	dev_t dev;
	int c;
{
	tx_chipset_tag_t tc;
	int ofs;

	tc = cn_sc.sc_tc;
	ofs = TX39_UARTCTRL1_REG(cn_sc.sc_slot);

	while (!(tx_conf_read(tc, ofs) & TX39_UARTCTRL1_EMPTY))
		delay(20);

	tx_conf_write(tc, TX39_UARTTXHOLD_REG(cn_sc.sc_slot), 
		      (c & TX39_UARTTXHOLD_TXDATA_MASK));

	while (!(tx_conf_read(tc, ofs) & TX39_UARTCTRL1_EMPTY))
		delay(20);

}

void
txcom_cnpollc(dev, on)
	dev_t dev;
	int on;
{
}

void
txcom_setmode(sc)
	struct txcom_softc *sc;
{
	tcflag_t cflag;	
	txreg_t reg;

	cflag = sc->sc_cflag;
	reg = tx_conf_read(sc->sc_tc, TX39_UARTCTRL1_REG(sc->sc_slot));

	switch (ISSET(cflag, CSIZE)) {
	default:
		printf("txcom_setmode: CS7, CS8 only. use CS7");
		/* FALL THROUGH */
	case CS7:
		reg |= TX39_UARTCTRL1_BIT7;
		break;
	case CS8:
		reg &= ~TX39_UARTCTRL1_BIT7;
		break;
	}
	if (ISSET(cflag, PARENB)) {
		reg |= TX39_UARTCTRL1_ENPARITY;
		if (ISSET(cflag, PARODD)) {
			reg &= ~TX39_UARTCTRL1_EVENPARITY;
		} else {
			reg |= TX39_UARTCTRL1_EVENPARITY;
		}
	} else {
		reg &= ~TX39_UARTCTRL1_ENPARITY;
	}
	if (ISSET(cflag, CSTOPB)) {
		reg |= TX39_UARTCTRL1_TWOSTOP;
	}
	tx_conf_write(sc->sc_tc, TX39_UARTCTRL1_REG(sc->sc_slot), reg);

}

void
txcom_setbaudrate(sc)
	struct txcom_softc *sc;
{
	int baudrate;
	txreg_t reg;

	baudrate = TX39_UARTCLOCKHZ / (sc->sc_speed * 16) - 1;
	reg = TX39_UARTCTRL2_BAUDRATE_SET(0, baudrate);
	
	tx_conf_write(sc->sc_tc, TX39_UARTCTRL2_REG(sc->sc_slot), reg);
}

void
txcom_rxsoft(arg)
	void *arg;
{
	struct txcom_softc *sc = arg;
	struct tty *tp = sc->sc_tty;
	int (*rint) __P((int c, struct tty *tp)) = linesw[tp->t_line].l_rint;
	int code = sc->sc_rbuf;

	DPRINTF(("txcom_rxsoft %c %08x\n", code, code));
	com_softrxintr_scheduled = 0;

	if ((*rint)(code, tp) == -1) {

	}
}

int
txcom_a_rxintr(arg)
	void *arg;
{
	struct txcom_softc *sc = arg;
	u_int8_t c;

	if (!com_softrxintr_scheduled) {
		com_softrxintr_scheduled = 1;
		timeout(txcom_rxsoft, arg, 1);
	}
	DPRINTF(("txcom_rxintr\n"));
	c = 0xff & tx_conf_read(sc->sc_tc, TX39_UARTRXHOLD_REG(sc->sc_slot));
	sc->sc_rbuf = c;

	return 0;
}

int
txcom_b_rxintr(arg)
	void *arg;
{
	struct txcom_softc *sc = arg;
	u_int8_t c;

	if (!com_softrxintr_scheduled) {
		com_softrxintr_scheduled = 1;
		timeout(txcom_rxsoft, arg, 1);
	}
	DPRINTF(("txcom_rxintr\n"));
	c = 0xff & tx_conf_read(sc->sc_tc, TX39_UARTRXHOLD_REG(sc->sc_slot));
	sc->sc_rbuf = c;

	return 0;
}


int
txcomopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct txcom_softc *sc = txcom_cd.cd_devs[minor(dev)];
	struct tty *tp = sc->sc_tty;
	int err;
	struct termios t;

	tp->t_dev = dev;

	/* XXX XXX XXX */
	tp->t_ispeed = 0;
	tp->t_ospeed = 9600;
	tp->t_cflag = (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8 | CLOCAL | HUPCL;
	txcomparam(tp, &t); /* not yet */
	/* XXX XXX XXX */

	tp->t_iflag = TTYDEF_IFLAG;
	tp->t_oflag = TTYDEF_OFLAG;
	tp->t_lflag = TTYDEF_LFLAG;
	ttychars(tp);
	ttsetwater(tp);

	if ((err = ttyopen(tp, minor(dev), ISSET(flag, O_NONBLOCK)))) {
		DPRINTF(("txcomopen: ttyopen failed\n"));
		return err;
	}
	if ((err = (*linesw[tp->t_line].l_open)(dev, tp))) {
		DPRINTF(("txcomopen: line dicipline open failed\n"));
		return err;
	}

	return err;
}

int
txcomclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct txcom_softc *sc = txcom_cd.cd_devs[minor(dev)];
	struct tty *tp = sc->sc_tty;

	DPRINTF(("txcomclose\n"));
	(*linesw[tp->t_line].l_close)(tp, flag);
	ttyclose(tp);

	return 0;
}

int
txcomread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct txcom_softc *sc = txcom_cd.cd_devs[minor(dev)];
	struct tty *tp = sc->sc_tty;
	DPRINTF(("txcomread\n"));
	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}
 
int
txcomwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct txcom_softc *sc = txcom_cd.cd_devs[minor(dev)];
	struct tty *tp = sc->sc_tty;

	DPRINTF(("txcomwrite\n")); 
	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

struct tty *
txcomtty(dev)
	dev_t dev;
{
	struct txcom_softc *sc = txcom_cd.cd_devs[minor(dev)];
	struct tty *tp = sc->sc_tty;
	DPRINTF(("txcomtty\n"));
	return tp;
}

int
txcomioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct txcom_softc *sc = txcom_cd.cd_devs[minor(dev)];
	struct tty *tp = sc->sc_tty;
	int error;

	DPRINTF(("txcomioctl\n"));
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag, p);
	if (error >= 0)
		return (error);

	error = ttioctl(tp, cmd, data, flag, p);
	if (error >= 0)
		return (error);

	return 0;
}

void
txcomstop(tp, flag)
	struct tty *tp;
	int flag;
{
	struct txcom_softc *sc = txcom_cd.cd_devs[minor(tp->t_dev)];
	int s;

	DPRINTF(("txcomstop\n"));
	s = spltty();

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
txcomstart(tp)
	struct tty *tp;
{
	struct txcom_softc *sc = txcom_cd.cd_devs[minor(tp->t_dev)];
	int s;

	DPRINTF(("txcomstart\n"));
	s = spltty();
	if (ISSET(tp->t_state, TS_BUSY | TS_TIMEOUT | TS_TTSTOP))
		return;

	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (ISSET(tp->t_state, TS_ASLEEP)) {
			CLR(tp->t_state, TS_ASLEEP);
			wakeup(&tp->t_outq);
		}
		selwakeup(&tp->t_wsel);
		if (tp->t_outq.c_cc == 0)
			return;
	}
	sc->sc_tba = tp->t_outq.c_cf;
	sc->sc_tbc = ndqb(&tp->t_outq, 0);
	while (sc->sc_tbc-- > 0) {
		txcom_cnputc(tp->t_dev, *sc->sc_tba++);
	}
	sc->sc_tbc = 0;

	CLR(tp->t_state, TS_BUSY);
	if (ISSET(tp->t_state, TS_FLUSH)) {
		CLR(tp->t_state, TS_FLUSH);
	} else {
		ndflush(&tp->t_outq, (int)(sc->sc_tba - tp->t_outq.c_cf));
	}
	splx(s);
}

int
txcomparam(tp, t)
	struct tty *tp;
	struct termios *t;
{
	DPRINTF(("txcomparam\n"));
	return 0;
}
