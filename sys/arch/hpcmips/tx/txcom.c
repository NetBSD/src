/*	$NetBSD: txcom.c,v 1.5 2000/01/13 17:53:37 uch Exp $ */

/*
 * Copyright (c) 1999, 2000, by UCHIYAMA Yasushi
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
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

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

#include <hpcmips/tx/tx39irvar.h>

#include <hpcmips/tx/tx39clockreg.h> /* XXX */

#define SET(t, f)	(t) |= (f)
#define CLR(t, f)	(t) &= ~(f)
#define ISSET(t, f)	((t) & (f))

#ifdef TX39UARTDEBUG
#define	DPRINTF(arg) printf arg
#else
#define	DPRINTF(arg)
#endif

#define TXCOM_HW_CONSOLE	0x40
#define	TXCOM_RING_SIZE		256 /* must be a power of two! */
#define TXCOM_RING_MASK		(TXCOM_RING_SIZE - 1)

struct txcom_chip {
	tx_chipset_tag_t sc_tc;
	int sc_slot;	/* UARTA or UARTB */
	int sc_cflag;	
	int sc_speed;
	int sc_swflags;
	int sc_hwflags;
};

struct txcom_softc {
	struct	device		sc_dev;
	struct tty		*sc_tty;
	struct txcom_chip	*sc_chip;

 	u_int8_t	*sc_tba;	/* transmit buffer address */
 	int		sc_tbc;		/* transmit byte count */
	int		sc_heldtbc;
	u_int8_t	*sc_rbuf;	/* receive buffer address */
	int		sc_rbput;	/* receive byte count */
	int		sc_rbget;
};

extern struct cfdriver txcom_cd;

int	txcom_match	__P((struct device*, struct cfdata*, void*));
void	txcom_attach	__P((struct device*, struct device*, void*));
int	txcom_print	__P((void*, const char*));

int	txcom_txintr		__P((void*));
int	txcom_rxintr		__P((void*));
int	txcom_overrun_intr	__P((void*));
int	txcom_frameerr_intr	__P((void*));
int	txcom_parityerr_intr	__P((void*));
int	txcom_break_intr	__P((void*));

void	txcom_rxsoft	__P((void*));
void	txcom_txsoft	__P((void*));
 
void	txcom_shutdown	__P((struct txcom_softc*));
void	txcom_break	__P((struct txcom_softc*, int));
void	txcom_modem	__P((struct txcom_softc*, int));
void	txcomstart	__P((struct tty*));
int	txcomparam	__P((struct tty*, struct termios*));

int	txcom_enable		__P((struct txcom_chip*));
void	txcom_disable		__P((struct txcom_chip*));
void	txcom_setmode		__P((struct txcom_chip*));
void	txcom_setbaudrate	__P((struct txcom_chip*));
int	txcom_cngetc		__P((dev_t));
void	txcom_cnputc		__P((dev_t, int));
void	txcom_cnpollc		__P((dev_t, int));

__inline int	__txcom_txbufready __P((struct txcom_chip*, int));
__inline const char *__txcom_slotname __P((int));

cdev_decl(txcom);

struct consdev txcomcons = {
	NULL, NULL, txcom_cngetc, txcom_cnputc, txcom_cnpollc, 
	NODEV, CN_NORMAL
};

/* Serial console */
struct txcom_chip txcom_chip;

struct cfattach txcom_ca = {
	sizeof(struct txcom_softc), txcom_match, txcom_attach
};

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
	struct txcom_chip *chip;
	int slot;

	/* Check this slot used as serial console */
	if (ua->ua_slot == txcom_chip.sc_slot &&
	    (txcom_chip.sc_hwflags & TXCOM_HW_CONSOLE)) {
		sc->sc_chip = &txcom_chip;
	} else {
		if (!(sc->sc_chip = malloc(sizeof(struct txcom_chip),
					   M_DEVBUF, M_WAITOK))) {
			printf(": can't allocate chip\n");
			return;
		}
		memset(sc->sc_chip, 0, sizeof(struct txcom_chip));
	}

	chip = sc->sc_chip;
	tc = chip->sc_tc = ua->ua_tc;
	slot = chip->sc_slot = ua->ua_slot;
	
	if (!(sc->sc_rbuf = malloc(TXCOM_RING_SIZE, M_DEVBUF, M_WAITOK))) {
		printf(": can't allocate buffer.\n");
		return;
	}
	memset(sc->sc_rbuf, 0, TXCOM_RING_SIZE);

	tp = ttymalloc();
	tp->t_oproc = txcomstart;
	tp->t_param = txcomparam;
	tp->t_hwiflow = NULL;
	sc->sc_tty = tp;
	tty_attach(tp);

	if (ISSET(chip->sc_hwflags, TXCOM_HW_CONSOLE)) {
		int maj;
		/* locate the major number */
		for (maj = 0; maj < nchrdev; maj++)
			if (cdevsw[maj].d_open == txcomopen)
				break;

		cn_tab->cn_dev = makedev(maj, sc->sc_dev.dv_unit);

		printf(": console");
	}

	printf("\n");

	/* 
	 * Enable interrupt
	 */
#define TXCOMINTR(i, s) MAKEINTR(2, TX39_INTRSTATUS2_UART##i##INT(s))
	
	tx_intr_establish(tc, TXCOMINTR(RX, slot), IST_EDGE, IPL_TTY, 
			  txcom_rxintr, sc);
	tx_intr_establish(tc, TXCOMINTR(TX, slot), IST_EDGE, IPL_TTY,
			  txcom_txintr, sc);
	tx_intr_establish(tc, TXCOMINTR(RXOVERRUN, slot), IST_EDGE, IPL_TTY,
			  txcom_rxintr, sc);
	tx_intr_establish(tc, TXCOMINTR(TXOVERRUN, slot), IST_EDGE, IPL_TTY,
			  txcom_txintr, sc);
	tx_intr_establish(tc, TXCOMINTR(FRAMEERR, slot), IST_EDGE, IPL_TTY,
			  txcom_frameerr_intr, sc);
	tx_intr_establish(tc, TXCOMINTR(PARITYERR, slot), IST_EDGE, IPL_TTY,
			  txcom_parityerr_intr, sc);
	tx_intr_establish(tc, TXCOMINTR(BREAK, slot), IST_EDGE, IPL_TTY,
			  txcom_break_intr, sc);

	/*
	 * UARTB can connect IR module
	 */
	if (ua->ua_slot == 1) {
		struct txcom_attach_args tca;
		tca.tca_tc = tc;
		tca.tca_parent = self;
		config_found(self, &tca, txcom_print);
	}
}

int
txcom_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	return pnp ? QUIET : UNCONF;
}

int
txcom_enable(chip)
	struct txcom_chip *chip;
{
	tx_chipset_tag_t tc;
	
	txreg_t reg;
	int slot, ofs, timeout;

	tc = chip->sc_tc;
	slot = chip->sc_slot;
	ofs = TX39_UARTCTRL1_REG(slot);

	/* Supply clock XXX should call clock module routine. */
	reg = tx_conf_read(tc, TX39_CLOCKCTRL_REG);
	reg |= (slot ? TX39_CLOCK_ENUARTBCLK : TX39_CLOCK_ENUARTACLK);
	tx_conf_write(tc, TX39_CLOCKCTRL_REG, reg);

	/* Power on */
	reg = tx_conf_read(tc, ofs);
	reg |= TX39_UARTCTRL1_ENUART;
	reg &= ~TX39_UARTCTRL1_ENBREAHALT;
	tx_conf_write(tc, ofs, reg);

	timeout = 100;
	
	while(!(tx_conf_read(tc, ofs) & TX39_UARTCTRL1_UARTON) &&
	      --timeout > 0)
		;
	
	if (timeout == 0 && !cold) {
		printf("UART%c never power up\n", "AB"[chip->sc_slot]);
		return 1;
	}

	/* 
	 * XXX Disable DMA (DMA not coded yet)
	 */
	reg &= ~(TX39_UARTCTRL1_ENDMARX | TX39_UARTCTRL1_ENDMATX);
	tx_conf_write(tc, ofs, reg);

	return 0;
}

void
txcom_disable(chip)
	struct txcom_chip *chip;
{
	tx_chipset_tag_t tc;
	txreg_t reg;
	int slot;

	tc = chip->sc_tc;
	slot = chip->sc_slot;

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

__inline int
__txcom_txbufready(chip, retry)
	struct txcom_chip *chip;
	int retry;
{
	tx_chipset_tag_t tc = chip->sc_tc;
	int ofs = TX39_UARTCTRL1_REG(chip->sc_slot);

	do {
		if (tx_conf_read(tc, ofs) & TX39_UARTCTRL1_EMPTY)
			return 1;
	} while(--retry != 0);
	
	return 0;
}

void
txcom_pulse_mode(dev)
	struct device *dev;
{
	struct txcom_softc *sc = (void*)dev;
	struct txcom_chip *chip = sc->sc_chip;
	tx_chipset_tag_t tc = chip->sc_tc;
	int ofs;
	txreg_t reg;
	
	ofs = TX39_UARTCTRL1_REG(chip->sc_slot);

	reg = tx_conf_read(tc, ofs);
	reg |= (TX39_UARTCTRL1_PULSEOPT2 | TX39_UARTCTRL1_PULSEOPT1);
	tx_conf_write(tc, ofs, reg);
}

/*
 * console
 */
int
txcom_cngetc(dev)
	dev_t dev;
{
	tx_chipset_tag_t tc;
	int ofs, c, s;

	s = spltty();
	
	tc = txcom_chip.sc_tc;
	ofs = TX39_UARTCTRL1_REG(txcom_chip.sc_slot);

	while(!(TX39_UARTCTRL1_RXHOLDFULL & tx_conf_read(tc, ofs)))
		;
	
	c = TX39_UARTRXHOLD_RXDATA(
		tx_conf_read(tc, TX39_UARTRXHOLD_REG(txcom_chip.sc_slot)));

	if (c == '\r')
		c = '\n';

	splx(s);

	return c;
}

void
txcom_cnputc(dev, c)
	dev_t dev;
	int c;
{
	struct txcom_chip *chip = &txcom_chip;
	tx_chipset_tag_t tc = chip->sc_tc;
	int s;

	s = spltty();

	/* Wait for transmitter to empty */
	__txcom_txbufready(chip, -1);

	tx_conf_write(tc, TX39_UARTTXHOLD_REG(chip->sc_slot), 
		      (c & TX39_UARTTXHOLD_TXDATA_MASK));

	__txcom_txbufready(chip, -1);
	
	splx(s);
}

void
txcom_cnpollc(dev, on)
	dev_t dev;
	int on;
{
}

void
txcom_setmode(chip)
	struct txcom_chip *chip;
{
	tcflag_t cflag = chip->sc_cflag;
	int ofs = TX39_UARTCTRL1_REG(chip->sc_slot);
	txreg_t reg;

	reg = tx_conf_read(chip->sc_tc, ofs);

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
	
	tx_conf_write(chip->sc_tc, ofs, reg);
}

void
txcom_setbaudrate(chip)
	struct txcom_chip *chip;
{
	int baudrate;
	txreg_t reg;

	if (chip->sc_speed == 0)
		return;

	if (!cold)
		DPRINTF(("txcom_setbaudrate: %d\n", chip->sc_speed));

	baudrate = TX39_UARTCLOCKHZ / (chip->sc_speed * 16) - 1;
	reg = TX39_UARTCTRL2_BAUDRATE_SET(0, baudrate);
	
	tx_conf_write(chip->sc_tc, TX39_UARTCTRL2_REG(chip->sc_slot), reg);
}

int
txcom_cnattach(slot, speed, cflag)
	int slot, speed, cflag;
{
	cn_tab = &txcomcons;

	txcom_chip.sc_tc	= tx_conf_get_tag();
	txcom_chip.sc_slot	= slot;
	txcom_chip.sc_cflag	= cflag;
	txcom_chip.sc_speed	= speed;
	txcom_chip.sc_hwflags |= TXCOM_HW_CONSOLE;

	if (txcom_enable(&txcom_chip))
		return 1;

	txcom_setmode(&txcom_chip);
	txcom_setbaudrate(&txcom_chip);

	return 0;
}

/*
 * tty
 */
void
txcom_break(sc, on)
	struct txcom_softc *sc;
	int on;
{
	struct txcom_chip *chip = sc->sc_chip;

	tx_conf_write(chip->sc_tc, TX39_UARTTXHOLD_REG(chip->sc_slot), 
		      on ? TX39_UARTTXHOLD_BREAK : 0);
}

void
txcom_modem(sc, on)
	struct txcom_softc *sc;
	int on;
{
	struct txcom_chip *chip = sc->sc_chip;
	tx_chipset_tag_t tc = chip->sc_tc;
	int slot = chip->sc_slot;
	txreg_t reg;

	reg = tx_conf_read(tc, TX39_UARTCTRL1_REG(slot));

	if (on) {
		reg &= ~TX39_UARTCTRL1_DISTXD;
	} else {
		reg |= TX39_UARTCTRL1_DISTXD;
	}
	
	reg = tx_conf_read(tc, TX39_UARTCTRL1_REG(slot));
}

void
txcom_shutdown(sc)
	struct txcom_softc *sc;
{
	struct tty *tp = sc->sc_tty;
	int s = spltty();

	/* Clear any break condition set with TIOCSBRK. */
	txcom_break(sc, 0);

	/*
	 * Hang up if necessary.  Wait a bit, so the other side has time to
	 * notice even if we immediately open the port again.
	 */
	if (ISSET(tp->t_cflag, HUPCL)) {
		txcom_modem(sc, 0);
		(void) tsleep(sc, TTIPRI, ttclos, hz);
	}


	/* Turn off interrupts if not the console. */
	if (!ISSET(sc->sc_chip->sc_hwflags, TXCOM_HW_CONSOLE)) {
		txcom_disable(sc->sc_chip);
	}

	splx(s);
}

__inline const char *
__txcom_slotname(slot)
	int slot;
{
	static const char *slotname[] = {"UARTA", "UARTB"};
	if (slot != 0 && slot != 1) {
		return "bogus slot";
	} else {
		return slotname[slot];
	}
}

int
txcom_overrun_intr(arg)
	void *arg;
{
	struct txcom_softc *sc = arg;
	
	printf("%s overrun\n", __txcom_slotname(sc->sc_chip->sc_slot));

	return 0;
}

int
txcom_frameerr_intr(arg)
	void *arg;
{
	struct txcom_softc *sc = arg;
	
	printf("%s frame error\n", __txcom_slotname(sc->sc_chip->sc_slot));

	return 0;
}

int
txcom_parityerr_intr(arg)
	void *arg;
{
	struct txcom_softc *sc = arg;
	
	printf("%s parity error\n", __txcom_slotname(sc->sc_chip->sc_slot));

	return 0;
}

int
txcom_break_intr(arg)
	void *arg;
{
	struct txcom_softc *sc = arg;
	
	printf("%s break\n", __txcom_slotname(sc->sc_chip->sc_slot));

	return 0;
}

int
txcom_rxintr(arg)
	void *arg;
{
	struct txcom_softc *sc = arg;
	struct txcom_chip *chip = sc->sc_chip;
	u_int8_t c;

	c = TX39_UARTRXHOLD_RXDATA(
		tx_conf_read(chip->sc_tc, 
			     TX39_UARTRXHOLD_REG(chip->sc_slot)));

	sc->sc_rbuf[sc->sc_rbput] = c;
	sc->sc_rbput = (sc->sc_rbput + 1) % TXCOM_RING_MASK;
	
	timeout(txcom_rxsoft, arg, 1);

	return 0;
}

void
txcom_rxsoft(arg)
	void *arg;
{
	struct txcom_softc *sc = arg;
	struct tty *tp = sc->sc_tty;
	int (*rint) __P((int c, struct tty *tp));
	int code;
	int s, end, get;

	rint = linesw[tp->t_line].l_rint;

	s = spltty();
	end = sc->sc_rbput;
	get = sc->sc_rbget;

	while (get != end) {
		code = sc->sc_rbuf[get];

		if ((*rint)(code, tp) == -1) {
			/*
			 * The line discipline's buffer is out of space.
			 */
		}
		get = (get + 1) % TXCOM_RING_MASK;
	}
	sc->sc_rbget = get;

	splx(s);
}

int
txcom_txintr(arg)
	void *arg;
{
	struct txcom_softc *sc = arg;
	struct txcom_chip *chip = sc->sc_chip;
	tx_chipset_tag_t tc = chip->sc_tc;

	if (sc->sc_tbc > 0) {
		tx_conf_write(tc, TX39_UARTTXHOLD_REG(chip->sc_slot), 
			      (*sc->sc_tba & 
			       TX39_UARTTXHOLD_TXDATA_MASK));
		sc->sc_tbc--;
		sc->sc_tba++;
	} else {
		timeout(txcom_txsoft, arg, 1);
	}

	return 0;
}

void
txcom_txsoft(arg)
	void *arg;
{
	struct txcom_softc *sc = arg;
	struct tty *tp = sc->sc_tty;
	int s = spltty();

	CLR(tp->t_state, TS_BUSY);
	if (ISSET(tp->t_state, TS_FLUSH)) {
		CLR(tp->t_state, TS_FLUSH);
	} else {
		ndflush(&tp->t_outq, (int)(sc->sc_tba - tp->t_outq.c_cf));
	}

	(*linesw[tp->t_line].l_start)(tp);

	splx(s);
}

int
txcomopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct txcom_softc *sc = txcom_cd.cd_devs[minor(dev)];
	struct txcom_chip *chip;
	struct tty *tp;
	int s, err;

	if (!sc)
		return ENXIO;

	chip = sc->sc_chip;
	tp = sc->sc_tty;

	if (ISSET(tp->t_state, TS_ISOPEN) &&
	    ISSET(tp->t_state, TS_XCLUDE) &&
	    p->p_ucred->cr_uid != 0)
		return (EBUSY);

	s = spltty();

	if (txcom_enable(sc->sc_chip))
		goto out;
	
	/*
	 * Do the following iff this is a first open.
	 */
	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		struct termios t;
		
		tp->t_dev = dev;

		t.c_ispeed = 0;
		if (ISSET(chip->sc_hwflags, TXCOM_HW_CONSOLE)) {
			t.c_ospeed = chip->sc_speed;
			t.c_cflag = chip->sc_cflag;
		} else {
			t.c_ospeed = TTYDEF_SPEED;
			t.c_cflag = TTYDEF_CFLAG;
		}

		if (ISSET(chip->sc_swflags, TIOCFLAG_CLOCAL))
			SET(t.c_cflag, CLOCAL);
		if (ISSET(chip->sc_swflags, TIOCFLAG_CRTSCTS))
			SET(t.c_cflag, CRTSCTS);
		if (ISSET(chip->sc_swflags, TIOCFLAG_MDMBUF))
			SET(t.c_cflag, MDMBUF);
		
		/* Make sure txcomparam() will do something. */
		tp->t_ospeed = 0;
		txcomparam(tp, &t);

		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;

		ttychars(tp);
		ttsetwater(tp);

		/*
		 * Turn on DTR.  We must always do this, even if carrier is not
		 * present, because otherwise we'd have to use TIOCSDTR
		 * immediately after setting CLOCAL, which applications do not
		 * expect.  We always assert DTR while the device is open
		 * unless explicitly requested to deassert it.
		 */
		txcom_modem(sc, 1);
	
		/* Clear the input ring, and unblock. */
		sc->sc_rbget = sc->sc_rbput = 0;
	}

	splx(s);

	if ((err = ttyopen(tp, minor(dev), ISSET(flag, O_NONBLOCK)))) {
		DPRINTF(("txcomopen: ttyopen failed\n"));
		goto out;
	}
	if ((err = (*linesw[tp->t_line].l_open)(dev, tp))) {
		DPRINTF(("txcomopen: line dicipline open failed\n"));
		goto out;
	}

	return err;

 out:
	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		/*
		 * We failed to open the device, and nobody else had it opened.
		 * Clean up the state as appropriate.
		 */
		txcom_shutdown(sc);
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

	/* XXX This is for cons.c. */
	if (!ISSET(tp->t_state, TS_ISOPEN))
		return 0;

	(*linesw[tp->t_line].l_close)(tp, flag);
	ttyclose(tp);

	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		/*
		 * Although we got a last close, the device may still be in
		 * use; e.g. if this was the dialout node, and there are still
		 * processes waiting for carrier on the non-dialout node.
		 */
		txcom_shutdown(sc);
	}

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

	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

struct tty *
txcomtty(dev)
	dev_t dev;
{
	struct txcom_softc *sc = txcom_cd.cd_devs[minor(dev)];
	
	return sc->sc_tty;
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
	int s, err;

	err = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag, p);
	if (err >= 0) {
		return err;
	}

	err = ttioctl(tp, cmd, data, flag, p);
	if (err >= 0) {
		return err;
	}

	err = 0;

	s = spltty();

	switch (cmd) {
	default:
		err = ENOTTY;
		break;

	case TIOCSBRK:
		txcom_break(sc, 1);
		break;

	case TIOCCBRK:
		txcom_break(sc, 0);
		break;

	case TIOCSDTR:
		txcom_modem(sc, 1);
		break;
		
	case TIOCCDTR:
		txcom_modem(sc, 0);
		break;

	case TIOCGFLAGS:
		*(int *)data = sc->sc_chip->sc_swflags;
		break;

	case TIOCSFLAGS:
		err = suser(p->p_ucred, &p->p_acflag); 
		if (err) {
			break;
		}
		sc->sc_chip->sc_swflags = *(int *)data;
		break;

	}

	splx(s);

	return err;
}

void
txcomstop(tp, flag)
	struct tty *tp;
	int flag;
{
	struct txcom_softc *sc = txcom_cd.cd_devs[minor(tp->t_dev)];
	int s;

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
	struct txcom_chip *chip = sc->sc_chip;
	tx_chipset_tag_t tc = chip->sc_tc;
	int slot = chip->sc_slot;
	int s;

	s = spltty();
	
	if (!__txcom_txbufready(chip, 0) ||
	    ISSET(tp->t_state, TS_BUSY | TS_TIMEOUT | TS_TTSTOP))
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

	sc->sc_tba = tp->t_outq.c_cf;
	sc->sc_tbc = ndqb(&tp->t_outq, 0);
	SET(tp->t_state, TS_BUSY);

	/* Output the first character of the contiguous buffer. */
	tx_conf_write(tc, TX39_UARTTXHOLD_REG(slot), 
		      (*sc->sc_tba & TX39_UARTTXHOLD_TXDATA_MASK));
		       
	sc->sc_tbc--;
	sc->sc_tba++;

 out:
	splx(s);
}

/*
 * Set TXcom tty parameters from termios.
 */
int
txcomparam(tp, t)
	struct tty *tp;
	struct termios *t;
{
	struct txcom_softc *sc = txcom_cd.cd_devs[minor(tp->t_dev)];
	struct txcom_chip *chip;
	int ospeed;
	int s;
	
	if (!sc)
		return ENXIO;
	
	ospeed = t->c_ospeed;

	/* Check requested parameters. */
	if (ospeed < 0) {
		return EINVAL;
	}
	if (t->c_ispeed && t->c_ispeed != ospeed) {
		return EINVAL;
	}

	s = spltty();
	chip = sc->sc_chip;
	/*
	 * For the console, always force CLOCAL and !HUPCL, so that the port
	 * is always active.
	 */
	if (ISSET(chip->sc_swflags, TIOCFLAG_SOFTCAR) ||
	    ISSET(chip->sc_hwflags, TXCOM_HW_CONSOLE)) {
		SET(t->c_cflag, CLOCAL);
		CLR(t->c_cflag, HUPCL);
	}
	splx(s);

	/*
	 * Only whack the UART when params change.
	 * Some callers need to clear tp->t_ospeed
	 * to make sure initialization gets done.
	 */
	if (tp->t_ospeed == ospeed && tp->t_cflag == t->c_cflag) {
		return 0;
	}

	s = spltty();
	chip = sc->sc_chip;
	chip->sc_speed = ospeed;
	chip->sc_cflag = t->c_cflag;
	
	txcom_setmode(chip);
	txcom_setbaudrate(chip);

	/* And copy to tty. */
	tp->t_ispeed = 0;
	tp->t_ospeed = chip->sc_speed;
	tp->t_cflag = chip->sc_cflag;

	/*
	 * If hardware flow control is disabled, unblock any hard flow 
	 * control state.
	 */
	if (!ISSET(chip->sc_cflag, CHWFLOW)) {
		txcomstart(tp);
	}

	splx(s);

	return 0;
}
