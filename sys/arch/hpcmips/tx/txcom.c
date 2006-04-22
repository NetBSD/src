/*	$NetBSD: txcom.c,v 1.27.6.1 2006/04/22 11:37:30 simonb Exp $ */

/*-
 * Copyright (c) 1999, 2000, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: txcom.c,v 1.27.6.1 2006/04/22 11:37:30 simonb Exp $");

#include "opt_tx39uart_debug.h"

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
#include <machine/config_hook.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx39icureg.h>
#include <hpcmips/tx/tx39uartvar.h>
#include <hpcmips/tx/tx39uartreg.h>

#include <hpcmips/tx/tx39irvar.h>

#include <hpcmips/tx/tx39clockreg.h> /* XXX */

/* 
 * UARTA channel has DTR, DSR, RTS, CTS lines. and they  wired to MFIO/IO port.
 */
#define IS_COM0(s)	((s) == 0)
#define IS_COM1(s)	((s) == 1)
#define ON		((void *)1)
#define OFF		((void *)0)

#ifdef	TX39UART_DEBUG
#define DPRINTF_ENABLE
#define DPRINTF_DEBUG	tx39uart_debug
#endif
#include <machine/debug.h>

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
	
	int sc_dcd;
	int sc_msr_cts;
	int sc_tx_stopped;
};

struct txcom_softc {
	struct	device		sc_dev;
	struct tty		*sc_tty;
	struct txcom_chip	*sc_chip;

	struct callout		sc_txsoft_ch;
	struct callout		sc_rxsoft_ch;

 	u_int8_t	*sc_tba;	/* transmit buffer address */
 	int		sc_tbc;		/* transmit byte count */
	int		sc_heldtbc;
	u_int8_t	*sc_rbuf;	/* receive buffer address */
	int		sc_rbput;	/* receive byte count */
	int		sc_rbget;
};

extern struct cfdriver txcom_cd;

int	txcom_match(struct device *, struct cfdata *, void *);
void	txcom_attach(struct device *, struct device *, void *);
int	txcom_print(void*, const char *);

int	txcom_txintr(void *);
int	txcom_rxintr(void *);
int	txcom_frameerr_intr(void *);
int	txcom_parityerr_intr(void *);
int	txcom_break_intr(void *);

void	txcom_rxsoft(void *);
void	txcom_txsoft(void *);
 
int	txcom_stsoft(void *);
int	txcom_stsoft2(void *);
int	txcom_stsoft3(void *);
int	txcom_stsoft4(void *);


void	txcom_shutdown(struct txcom_softc *);
void	txcom_break(struct txcom_softc *, int);
void	txcom_modem(struct txcom_softc *, int);
void	txcomstart(struct tty *);
int	txcomparam(struct tty *, struct termios *);

void	txcom_reset	(struct txcom_chip *);
int	txcom_enable	(struct txcom_chip *, boolean_t);
void	txcom_disable	(struct txcom_chip *);
void	txcom_setmode	(struct txcom_chip *);
void	txcom_setbaudrate(struct txcom_chip *);
int	txcom_cngetc	(dev_t);
void	txcom_cnputc	(dev_t, int);
void	txcom_cnpollc	(dev_t, int);

int	txcom_dcd_hook(void *, int, long, void *);
int	txcom_cts_hook(void *, int, long, void *);


inline int	__txcom_txbufready(struct txcom_chip *, int);
const char *__txcom_slotname(int);

#ifdef TX39UARTDEBUG
void	txcom_dump(struct txcom_chip *);
#endif

struct consdev txcomcons = {
	NULL, NULL, txcom_cngetc, txcom_cnputc, txcom_cnpollc, NULL, NULL,
	NULL, NODEV, CN_NORMAL
};

/* Serial console */
struct txcom_chip txcom_chip;

CFATTACH_DECL(txcom, sizeof(struct txcom_softc),
    txcom_match, txcom_attach, NULL, NULL);

dev_type_open(txcomopen);
dev_type_close(txcomclose);
dev_type_read(txcomread);
dev_type_write(txcomwrite);
dev_type_ioctl(txcomioctl);
dev_type_stop(txcomstop);
dev_type_tty(txcomtty);
dev_type_poll(txcompoll);

const struct cdevsw txcom_cdevsw = {
	txcomopen, txcomclose, txcomread, txcomwrite, txcomioctl,
	txcomstop, txcomtty, txcompoll, nommap, ttykqfilter, D_TTY
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
txcom_attach(struct device *parent, struct device *self, void *aux)
{
	struct tx39uart_attach_args *ua = aux;
	struct txcom_softc *sc = (void*)self;
	tx_chipset_tag_t tc;
	struct tty *tp;
	struct txcom_chip *chip;
	int slot, console;

	/* Check this slot used as serial console */
	console = (ua->ua_slot == txcom_chip.sc_slot) &&
	    (txcom_chip.sc_hwflags & TXCOM_HW_CONSOLE);

	if (console) {
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
	
#ifdef TX39UARTDEBUG
	txcom_dump(chip);
#endif
	if (!console)
		txcom_reset(chip);

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
		maj = cdevsw_lookup_major(&txcom_cdevsw);

		cn_tab->cn_dev = makedev(maj, device_unit(&sc->sc_dev));

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
	 * UARTA has external signal line. (its wiring is platform dependent)
	 */
	if (IS_COM0(slot)) {
		/* install DCD, CTS hooks. */
		config_hook(CONFIG_HOOK_EVENT, CONFIG_HOOK_COM0_DCD,
		    CONFIG_HOOK_EXCLUSIVE, txcom_dcd_hook, sc);
		config_hook(CONFIG_HOOK_EVENT, CONFIG_HOOK_COM0_CTS,
		    CONFIG_HOOK_EXCLUSIVE, txcom_cts_hook, sc);
	}

	/*
	 * UARTB can connect IR module
	 */
	if (IS_COM1(slot)) {
		struct txcom_attach_args tca;
		tca.tca_tc = tc;
		tca.tca_parent = self;
		config_found(self, &tca, txcom_print);
	}
}

int
txcom_print(void *aux, const char *pnp)
{
	return pnp ? QUIET : UNCONF;
}

void
txcom_reset(struct txcom_chip *chip)
{
	tx_chipset_tag_t tc;
	int slot, ofs;
	txreg_t reg;

	tc = chip->sc_tc;
	slot = chip->sc_slot;
	ofs = TX39_UARTCTRL1_REG(slot);
	
	/* Supply clock */
	reg = tx_conf_read(tc, TX39_CLOCKCTRL_REG);
	reg |= (slot ? TX39_CLOCK_ENUARTBCLK : TX39_CLOCK_ENUARTACLK);
	tx_conf_write(tc, TX39_CLOCKCTRL_REG, reg);

	/* reset UART module */
	tx_conf_write(tc, ofs, 0);	
}

int
txcom_enable(struct txcom_chip *chip, boolean_t console)
{
	tx_chipset_tag_t tc;
	txreg_t reg;
	int slot, ofs, timeout;

	tc = chip->sc_tc;
	slot = chip->sc_slot;
	ofs = TX39_UARTCTRL1_REG(slot);

	/*
	 * External power supply (if any)
	 * When serial console, Windows CE already powered on it.
	 */
	if (!console) {
		config_hook_call(CONFIG_HOOK_POWERCONTROL,
		    CONFIG_HOOK_POWERCONTROL_COM0, PWCTL_ON);
		delay(3);
	}

	/* Supply clock */
	reg = tx_conf_read(tc, TX39_CLOCKCTRL_REG);
	reg |= (slot ? TX39_CLOCK_ENUARTBCLK : TX39_CLOCK_ENUARTACLK);
	tx_conf_write(tc, TX39_CLOCKCTRL_REG, reg);

	/* 
	 * XXX Disable DMA (DMA not coded yet)
	 */
	reg = tx_conf_read(tc, ofs);
	reg &= ~(TX39_UARTCTRL1_ENDMARX | TX39_UARTCTRL1_ENDMATX);
	tx_conf_write(tc, ofs, reg);

	/* enable */
	reg = tx_conf_read(tc, ofs);
	reg |= TX39_UARTCTRL1_ENUART;
	reg &= ~TX39_UARTCTRL1_ENBREAHALT;
	tx_conf_write(tc, ofs, reg);

	timeout = 100000;
	
	while(!(tx_conf_read(tc, ofs) & TX39_UARTCTRL1_UARTON) &&
	    --timeout > 0)
		;
	
	if (timeout == 0 && !cold) {
		printf("%s never power up\n", __txcom_slotname(slot));
		return 1;
	}

	return 0;
}

void
txcom_disable(struct txcom_chip *chip)
{
	tx_chipset_tag_t tc;
	txreg_t reg;
	int slot;

	tc = chip->sc_tc;
	slot = chip->sc_slot;

	reg = tx_conf_read(tc, TX39_UARTCTRL1_REG(slot));
	/* DMA */
	reg &= ~(TX39_UARTCTRL1_ENDMARX | TX39_UARTCTRL1_ENDMATX);

	/* disable module */
	reg &= ~TX39_UARTCTRL1_ENUART;
	tx_conf_write(tc, TX39_UARTCTRL1_REG(slot), reg);

	/* Clock */
	reg = tx_conf_read(tc, TX39_CLOCKCTRL_REG);
	reg &= ~(slot ? TX39_CLOCK_ENUARTBCLK : TX39_CLOCK_ENUARTACLK);
	tx_conf_write(tc, TX39_CLOCKCTRL_REG, reg);
	
}

inline int
__txcom_txbufready(struct txcom_chip *chip, int retry)
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
txcom_pulse_mode(struct device *dev)
{
	struct txcom_softc *sc = (void*)dev;
	struct txcom_chip *chip = sc->sc_chip;
	tx_chipset_tag_t tc = chip->sc_tc;
	int ofs;
	txreg_t reg;
	
	ofs = TX39_UARTCTRL1_REG(chip->sc_slot);

	reg = tx_conf_read(tc, ofs);
	/* WindowsCE use this setting */
	reg |= TX39_UARTCTRL1_PULSEOPT1;
	reg &= ~TX39_UARTCTRL1_PULSEOPT2;
	reg |= TX39_UARTCTRL1_DTINVERT;

	tx_conf_write(tc, ofs, reg);
}

/*
 * console
 */
int
txcom_cngetc(dev_t dev)
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
txcom_cnputc(dev_t dev, int c)
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
txcom_cnpollc(dev_t dev, int on)
{
}

void
txcom_setmode(struct txcom_chip *chip)
{
	tcflag_t cflag = chip->sc_cflag;
	int ofs = TX39_UARTCTRL1_REG(chip->sc_slot);
	txreg_t reg;

	reg = tx_conf_read(chip->sc_tc, ofs);
	reg &= ~TX39_UARTCTRL1_ENUART;
	tx_conf_write(chip->sc_tc, ofs, reg);
	
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

	if (ISSET(cflag, CSTOPB))
		reg |= TX39_UARTCTRL1_TWOSTOP;
	else 
		reg &= ~TX39_UARTCTRL1_TWOSTOP;

	reg |= TX39_UARTCTRL1_ENUART;
	tx_conf_write(chip->sc_tc, ofs, reg);
}

void
txcom_setbaudrate(struct txcom_chip *chip)
{
	int baudrate;
	int ofs = TX39_UARTCTRL1_REG(chip->sc_slot);
	txreg_t reg, reg1;

	if (chip->sc_speed == 0)
		return;

	if (!cold)
		DPRINTF("%d\n", chip->sc_speed);

	reg1 = tx_conf_read(chip->sc_tc, ofs);
	reg1 &= ~TX39_UARTCTRL1_ENUART;
	tx_conf_write(chip->sc_tc, ofs, reg1);

	baudrate = TX39_UARTCLOCKHZ / (chip->sc_speed * 16) - 1;
	reg = TX39_UARTCTRL2_BAUDRATE_SET(0, baudrate);
	
	tx_conf_write(chip->sc_tc, TX39_UARTCTRL2_REG(chip->sc_slot), reg);

	reg1 |= TX39_UARTCTRL1_ENUART;
	tx_conf_write(chip->sc_tc, ofs, reg1);
}

int
txcom_cnattach(int slot, int speed, int cflag)
{
	cn_tab = &txcomcons;

	txcom_chip.sc_tc	= tx_conf_get_tag();
	txcom_chip.sc_slot	= slot;
	txcom_chip.sc_cflag	= cflag;
	txcom_chip.sc_speed	= speed;
	txcom_chip.sc_hwflags |= TXCOM_HW_CONSOLE;
#if notyet
	txcom_reset(&txcom_chip);
#endif	
	txcom_setmode(&txcom_chip);
	txcom_setbaudrate(&txcom_chip);

	if (txcom_enable(&txcom_chip, TRUE) != 0)
		return 1;

	return 0;
}

/*
 * tty
 */
void
txcom_break(struct txcom_softc *sc, int on)
{
	struct txcom_chip *chip = sc->sc_chip;

	tx_conf_write(chip->sc_tc, TX39_UARTTXHOLD_REG(chip->sc_slot), 
	    on ? TX39_UARTTXHOLD_BREAK : 0);
}

void
txcom_modem(struct txcom_softc *sc, int on)
{
	struct txcom_chip *chip = sc->sc_chip;
	tx_chipset_tag_t tc = chip->sc_tc;
	int slot = chip->sc_slot;
	txreg_t reg;

	/* assert DTR */
	if (IS_COM0(slot)) {
		config_hook_call(CONFIG_HOOK_SET, 
		    CONFIG_HOOK_COM0_DTR,
		    (void *)on);
	}

	reg = tx_conf_read(tc, TX39_UARTCTRL1_REG(slot));
	reg &= ~TX39_UARTCTRL1_ENUART;
	tx_conf_write(tc, TX39_UARTCTRL1_REG(slot), reg);

	if (on) {
		reg &= ~TX39_UARTCTRL1_DISTXD;
	} else {
		reg |= TX39_UARTCTRL1_DISTXD; /* low UARTTXD */
	}

	reg |= TX39_UARTCTRL1_ENUART;
	tx_conf_write(tc, TX39_UARTCTRL1_REG(slot), reg);
}

void
txcom_shutdown(struct txcom_softc *sc)
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

const char *
__txcom_slotname(int slot)
{
	static const char *slotname[] = {"UARTA", "UARTB", "unknown"};

	if (slot != 0 && slot != 1)
		return slotname[2];

	return slotname[slot];
}

int
txcom_frameerr_intr(void *arg)
{
	struct txcom_softc *sc = arg;
	
	printf("%s frame error\n", __txcom_slotname(sc->sc_chip->sc_slot));

	return 0;
}

int
txcom_parityerr_intr(void *arg)
{
	struct txcom_softc *sc = arg;
	
	printf("%s parity error\n", __txcom_slotname(sc->sc_chip->sc_slot));

	return 0;
}

int
txcom_break_intr(void *arg)
{
	struct txcom_softc *sc = arg;
	
	printf("%s break\n", __txcom_slotname(sc->sc_chip->sc_slot));

	return 0;
}

int
txcom_rxintr(void *arg)
{
	struct txcom_softc *sc = arg;
	struct txcom_chip *chip = sc->sc_chip;
	u_int8_t c;

	c = TX39_UARTRXHOLD_RXDATA(
		tx_conf_read(chip->sc_tc, 
		    TX39_UARTRXHOLD_REG(chip->sc_slot)));

	sc->sc_rbuf[sc->sc_rbput] = c;
	sc->sc_rbput = (sc->sc_rbput + 1) % TXCOM_RING_MASK;
	
	callout_reset(&sc->sc_rxsoft_ch, 1, txcom_rxsoft, sc);

	return 0;
}

void
txcom_rxsoft(void *arg)
{
	struct txcom_softc *sc = arg;
	struct tty *tp = sc->sc_tty;
	int (*rint)(int, struct tty *);
	int code;
	int s, end, get;

	rint = tp->t_linesw->l_rint;

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
txcom_txintr(void *arg)
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
		callout_reset(&sc->sc_rxsoft_ch, 1, txcom_txsoft, sc);
	}

	return 0;
}

void
txcom_txsoft(void *arg)
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

	(*tp->t_linesw->l_start)(tp);

	splx(s);
}

int
txcomopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct txcom_softc *sc = txcom_cd.cd_devs[minor(dev)];
	struct txcom_chip *chip;
	struct tty *tp;
	int s, err = ENXIO;
;

	if (!sc)
		return err;

	chip = sc->sc_chip;
	tp = sc->sc_tty;

	if (ISSET(tp->t_state, TS_ISOPEN) &&
	    ISSET(tp->t_state, TS_XCLUDE) &&
	    suser(l->l_proc->p_ucred, &l->l_proc->p_acflag) != 0)
		return (EBUSY);

	s = spltty();

	if (txcom_enable(sc->sc_chip, FALSE)) {
		splx(s);
		goto out;
	}
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
#define	TXCOMDIALOUT(x)	(minor(x) & 0x80000)
	if ((err = ttyopen(tp, TXCOMDIALOUT(dev), ISSET(flag, O_NONBLOCK)))) {
		DPRINTF("ttyopen failed\n");
		goto out;
	}
	if ((err = (*tp->t_linesw->l_open)(dev, tp))) {
		DPRINTF("line dicipline open failed\n");
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
txcomclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct txcom_softc *sc = txcom_cd.cd_devs[minor(dev)];
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
		txcom_shutdown(sc);
	}

	return 0;
}

int
txcomread(dev_t dev, struct uio *uio, int flag)
{
	struct txcom_softc *sc = txcom_cd.cd_devs[minor(dev)];
	struct tty *tp = sc->sc_tty;

	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}
 
int
txcomwrite(dev_t dev, struct uio *uio, int flag)
{
	struct txcom_softc *sc = txcom_cd.cd_devs[minor(dev)];
	struct tty *tp = sc->sc_tty;

	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
txcompoll(dev_t dev, int events, struct lwp *l)
{
	struct txcom_softc *sc = txcom_cd.cd_devs[minor(dev)];
	struct tty *tp = sc->sc_tty;
 
	return ((*tp->t_linesw->l_poll)(tp, events, l));
}

struct tty *
txcomtty(dev_t dev)
{
	struct txcom_softc *sc = txcom_cd.cd_devs[minor(dev)];
	
	return sc->sc_tty;
}

int
txcomioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct lwp *l)
{
	struct txcom_softc *sc = txcom_cd.cd_devs[minor(dev)];
	struct tty *tp = sc->sc_tty;
	int s, err;

	err = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (err != EPASSTHROUGH) {
		return err;
	}

	err = ttioctl(tp, cmd, data, flag, l);
	if (err != EPASSTHROUGH) {
		return err;
	}

	err = 0;

	s = spltty();

	switch (cmd) {
	default:
		err = EPASSTHROUGH;
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
		err = suser(l->l_proc->p_ucred, &l->l_proc->p_acflag); 
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
txcomstop(struct tty *tp, int flag)
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
txcomstart(struct tty *tp)
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
txcomparam(struct tty *tp, struct termios *t)
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
	 * If we're not in a mode that assumes a connection is present, then
	 * ignore carrier changes.
	 */
	if (ISSET(t->c_cflag, CLOCAL | MDMBUF))
		chip->sc_dcd = 0;
	else
		chip->sc_dcd = 1;

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
	 * Update the tty layer's idea of the carrier bit, in case we changed
	 * CLOCAL or MDMBUF.  We don't hang up here; we only do that by
	 * explicit request.
	 */
	(void) (*tp->t_linesw->l_modem)(tp, chip->sc_dcd);

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

int
txcom_dcd_hook(void *arg, int type, long id, void *msg)
{
	struct txcom_softc *sc = arg;
	struct tty *tp = sc->sc_tty;
	struct txcom_chip *chip = sc->sc_chip;
	int modem = !(int)msg; /* p-edge 1, n-edge 0 */

	DPRINTF("DCD %s\n", modem ? "ON" : "OFF");
		 
	if (modem && chip->sc_dcd)	
		(void) (*tp->t_linesw->l_modem)(tp, chip->sc_dcd);

	return 0;
}

int
txcom_cts_hook(void *arg, int type, long id, void *msg)
{
	struct txcom_softc *sc = arg;
	struct tty *tp = sc->sc_tty;
	struct txcom_chip *chip = sc->sc_chip;
	int clear = !(int)msg; /* p-edge 1, n-edge 0 */

	DPRINTF("CTS %s\n", clear ? "ON"  : "OFF");

	if (chip->sc_msr_cts) {
		if (!clear) {
			chip->sc_tx_stopped = 1;
		} else {
			chip->sc_tx_stopped = 0;
			(*tp->t_linesw->l_start)(tp);
		}
	}

	return 0;
}

#ifdef TX39UARTDEBUG
void
txcom_dump(struct txcom_chip *chip)
{
	tx_chipset_tag_t tc = chip->sc_tc;
	int slot = chip->sc_slot;
	txreg_t reg;
	
	reg = tx_conf_read(tc, TX39_UARTCTRL1_REG(slot));
#define ISSETPRINT(r, m) \
	dbg_bitmask_print(r, TX39_UARTCTRL1_##m, #m)
	ISSETPRINT(reg, UARTON);
	ISSETPRINT(reg, EMPTY);
	ISSETPRINT(reg, PRXHOLDFULL);
	ISSETPRINT(reg, RXHOLDFULL);
	ISSETPRINT(reg, ENDMARX);
	ISSETPRINT(reg, ENDMATX);
	ISSETPRINT(reg, TESTMODE);
	ISSETPRINT(reg, ENBREAHALT);
	ISSETPRINT(reg, ENDMATEST);
	ISSETPRINT(reg, ENDMALOOP);
	ISSETPRINT(reg, PULSEOPT2);
	ISSETPRINT(reg, PULSEOPT1);
	ISSETPRINT(reg, DTINVERT);
	ISSETPRINT(reg, DISTXD);
	ISSETPRINT(reg, TWOSTOP);
	ISSETPRINT(reg, LOOPBACK);
	ISSETPRINT(reg, BIT7);
	ISSETPRINT(reg, EVENPARITY);
	ISSETPRINT(reg, ENPARITY);
	ISSETPRINT(reg, ENUART);
}
#endif /* TX39UARTDEBUG */
