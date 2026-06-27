/*	$NetBSD: psc.c,v 1.1 2026/06/27 13:28:35 rkujawa Exp $	*/

/*-
 * Copyright (c) 2008, 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa and Robert Swindells.
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

/*
 * Driver for the MPC5200B Programmable Serial Controller (PSC) in UART mode.
 *
 * The interrupt-driven ttyback end is adapted from the MPC5200B PSC
 * driver by Robert Swindells, simplified onto the mpcobio attachment and 
 * reduced to the subset the console needs.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: psc.c,v 1.1 2026/06/27 13:28:35 rkujawa Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/kauth.h>
#include <sys/intr.h>
#include <sys/kmem.h>
#include <sys/bus.h>

#include <dev/cons.h>
#include <dev/ofw/openfirm.h>

#include <machine/autoconf.h>

#include <powerpc/mpc5200/obiovar.h>
#include <powerpc/mpc5200/mpc5200reg.h>
#include <powerpc/mpc5200/pscreg.h>
#include <powerpc/mpc5200/pscvar.h>
#include <powerpc/mpc5200/cdmvar.h>

#ifndef PSC_CONSOLE_SPEED
#define PSC_CONSOLE_SPEED	115200	/* console baud, programmed from IPB */
#endif

#define PSCUNIT(x)	minor(x)

#define PSC_READ_1(sc, r)	bus_space_read_1((sc)->sc_iot, (sc)->sc_ioh, (r))
#define PSC_READ_2(sc, r)	bus_space_read_2((sc)->sc_iot, (sc)->sc_ioh, (r))
#define PSC_WRITE_1(sc, r, v)	bus_space_write_1((sc)->sc_iot, (sc)->sc_ioh, (r), (v))
#define PSC_WRITE_2(sc, r, v)	bus_space_write_2((sc)->sc_iot, (sc)->sc_ioh, (r), (v))

static int	psc_match(device_t, cfdata_t, void *);
static void	psc_attach(device_t, device_t, void *);
static u_int	psc_baud_divisor(uint32_t, int);
static void	psc_set_baud(bus_space_tag_t, bus_space_handle_t, int);

static int	pscintr(void *);
static void	pscsoft(void *);
static void	psc_rxsoft(struct psc_softc *, struct tty *);

CFATTACH_DECL_NEW(psc, sizeof(struct psc_softc),
    psc_match, psc_attach, NULL, NULL);

extern struct cfdriver psc_cd;

dev_type_open(pscopen);
dev_type_close(pscclose);
dev_type_read(pscread);
dev_type_write(pscwrite);
dev_type_ioctl(pscioctl);
dev_type_stop(pscstop);
dev_type_tty(psctty);
dev_type_poll(pscpoll);

const struct cdevsw psc_cdevsw = {
	.d_open = pscopen,
	.d_close = pscclose,
	.d_read = pscread,
	.d_write = pscwrite,
	.d_ioctl = pscioctl,
	.d_stop = pscstop,
	.d_tty = psctty,
	.d_poll = pscpoll,
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY,
};

static int	pscparam(struct tty *, struct termios *);
static void	pscstart(struct tty *);

/*
 * Console state.
 */
static struct {
	bool			cs_mapped;
	volatile uint8_t	*cs_base;
} psc_console;

#define PSC_CN_SR(cs)	(*(volatile uint16_t *)((cs)->cs_base + PSC_SR))
#define PSC_CN_TB(cs)	(*((cs)->cs_base + PSC_TB))
#define PSC_CN_RB(cs)	(*((cs)->cs_base + PSC_RB))

static void	psccnprobe(struct consdev *);
static void	psccninit(struct consdev *);
static int	psccngetc(dev_t);
static void	psccnputc(dev_t, int);
static void	psccnpollc(dev_t, int);

struct consdev psccons = {
	.cn_probe = psccnprobe,
	.cn_init = psccninit,
	.cn_getc = psccngetc,
	.cn_putc = psccnputc,
	.cn_pollc = psccnpollc,
	.cn_dev = NODEV,
	.cn_pri = CN_REMOTE,
};

static bool
psc_is_uart(int node)
{
	char compat[64];
	int len;

	len = OF_getprop(node, "compatible", compat, sizeof(compat));
	if (len > 0 &&
	    (strcmp(compat, "mpc5200-psc-uart") == 0 ||
	     strcmp(compat, "mpc5200b-psc-uart") == 0))
		return true;
	return false;
}

static int
psc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args *oba = aux;

	if (strcmp(oba->obio_name, "serial") == 0)
		return 1;
	if (psc_is_uart(oba->obio_node))
		return 1;
	return 0;
}

static void
psc_attach(device_t parent, device_t self, void *aux)
{
	struct psc_softc *sc = device_private(self);
	struct obio_attach_args *oba = aux;
	struct tty *tp;
	bus_size_t size;
	int ist;

	sc->sc_dev = self;
	sc->sc_iot = oba->obio_bst;
	sc->sc_node = oba->obio_node;

	size = oba->obio_size != 0 ? oba->obio_size : MPC5200_PSC_SIZE;
	if (bus_space_map(sc->sc_iot, oba->obio_addr, size, 0,
	    &sc->sc_ioh) != 0) {
		aprint_error(": can't map registers\n");
		return;
	}

	sc->sc_console = (sc->sc_node == console_node);

	if (sc->sc_console) {
		uint32_t ipb = mpc5200_cdm_get_ipb_freq();
		u_int div = psc_baud_divisor(ipb, PSC_CONSOLE_SPEED);

		/*
		 * Announce the computed divisor through the firmware-inherited
		 * console *before* reprogramming
		 */
		aprint_normal(": console, IPB %u.%u MHz, %d baud (divisor %u)",
		    ipb / 1000000, (ipb / 100000) % 10, PSC_CONSOLE_SPEED, div);
		psc_set_baud(sc->sc_iot, sc->sc_ioh, PSC_CONSOLE_SPEED);
	}
	aprint_normal("\n");

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_HIGH);

	/* Mask all PSC events and stop the line while reprogramming. */
	sc->sc_imr = 0;
	PSC_WRITE_2(sc, PSC_IMR, sc->sc_imr);
	PSC_WRITE_1(sc, PSC_CR, CMD_TX_DISABLE | CMD_RX_DISABLE);

	/*
	 * Clear MR1[FFULL] so the RxRDY status
	 */
	{
		uint8_t mr1, mr2;

		PSC_WRITE_1(sc, PSC_CR, CMD_RESET_MR);
		mr1 = PSC_READ_1(sc, PSC_MR1);
		mr2 = PSC_READ_1(sc, PSC_MR2);
		mr1 &= ~MR1_FFULL;
		PSC_WRITE_1(sc, PSC_CR, CMD_RESET_MR);
		PSC_WRITE_1(sc, PSC_MR1, mr1);
		PSC_WRITE_1(sc, PSC_MR2, mr2);
	}

	/* Clear errors and bring the receiver and transmitter up. */
	PSC_WRITE_1(sc, PSC_CR, CMD_RESET_ERR);
	PSC_WRITE_1(sc, PSC_CR, CMD_TX_ENABLE | CMD_RX_ENABLE);

	sc->sc_si = softint_establish(SOFTINT_SERIAL, pscsoft, sc);
	if (sc->sc_si == NULL) {
		aprint_error_dev(self, "can't establish soft interrupt\n");
		return;
	}

	if (obio_decode_interrupt(oba->obio_node, 0, &sc->sc_irq, &ist)) {
		sc->sc_ih = intr_establish(sc->sc_irq, ist, IPL_HIGH,
		    pscintr, sc);
		if (sc->sc_ih == NULL)
			aprint_error_dev(self, "can't establish interrupt\n");
	}

	tp = tty_alloc();
	tp->t_oproc = pscstart;
	tp->t_param = pscparam;
	sc->sc_tty = tp;
	sc->sc_rbput = sc->sc_rbget = 0;
	tty_attach(tp);

	/*
	 * Redirect /dev/console opens to this tty: cnopen() follows
	 * cn_tab->cn_dev, which the polled console left as NODEV.
	 */
	if (sc->sc_console && cn_tab == &psccons) {
		int maj = cdevsw_lookup_major(&psc_cdevsw);

		tp->t_dev = cn_tab->cn_dev =
		    makedev(maj, device_unit(self));
	}
}

/*
 * Compute the 16-bit UART baud-rate divisor for a target rate from the IPB
 * clock.
 */
static u_int
psc_baud_divisor(uint32_t ipb, int baud)
{
	u_int div;

	if (baud <= 0)
		return 1;

	div = (ipb + (PSC_BAUD_PRESCALE / 2) * baud) /
	    (PSC_BAUD_PRESCALE * baud);
	if (div < 1)
		div = 1;
	if (div > 0xffff)
		div = 0xffff;
	return div;
}

/*
 * Program the UART baud rate
 */
static void
psc_set_baud(bus_space_tag_t iot, bus_space_handle_t ioh, int baud)
{
	u_int div = psc_baud_divisor(mpc5200_cdm_get_ipb_freq(), baud);
	int timo;

	/* Let any queued console output drain before stopping the line. */
	for (timo = 100000; timo > 0; timo--) {
		if (bus_space_read_2(iot, ioh, PSC_SR) & SR_TXEMP)
			break;
	}

	bus_space_write_1(iot, ioh, PSC_CR, CMD_TX_DISABLE | CMD_RX_DISABLE);
	bus_space_write_2(iot, ioh, PSC_CSR, CSR_UART_CT);
	bus_space_write_1(iot, ioh, PSC_CTUR, (div >> 8) & 0xff);
	bus_space_write_1(iot, ioh, PSC_CTLR, div & 0xff);
	bus_space_write_1(iot, ioh, PSC_CR, CMD_TX_ENABLE | CMD_RX_ENABLE);
}

/*
 * Hard interrupt
 */
static int
pscintr(void *arg)
{
	struct psc_softc *sc = arg;
	uint16_t isr, sr;
	int handled = 0;

	mutex_spin_enter(&sc->sc_lock);
	isr = PSC_READ_2(sc, PSC_ISR);

	if (isr & (INT_RXRDY | INT_ERROR | INT_ORERR)) {
		sr = PSC_READ_2(sc, PSC_SR);
		while (sr & SR_RXRDY) {
			uint8_t c = PSC_READ_1(sc, PSC_RB);
			u_int next = (sc->sc_rbput + 1) & PSC_RING_MASK;

			if (next != sc->sc_rbget) {
				sc->sc_rbuf[sc->sc_rbput] = c;
				sc->sc_rbput = next;
			}
			/* else ring full: drop, mirroring a silo overflow */
			sr = PSC_READ_2(sc, PSC_SR);
		}
		if (sr & SR_ORERR)
			PSC_WRITE_1(sc, PSC_CR, CMD_RESET_ERR);
		sc->sc_rx_ready = true;
		handled = 1;
	}

	sr = PSC_READ_2(sc, PSC_SR);
	if (sr & SR_TXRDY) {
		while (sc->sc_tbc > 0 && (PSC_READ_2(sc, PSC_SR) & SR_TXRDY)) {
			PSC_WRITE_1(sc, PSC_TB, *sc->sc_tba);
			sc->sc_tba++;
			sc->sc_tbc--;
		}
		if (sc->sc_tbc == 0) {
			if (sc->sc_imr & INT_TXRDY) {
				sc->sc_imr &= ~INT_TXRDY;
				PSC_WRITE_2(sc, PSC_IMR, sc->sc_imr);
			}
			if (sc->sc_tx_busy) {
				sc->sc_tx_busy = false;
				sc->sc_tx_done = true;
				handled = 1;
			}
		}
	}

	mutex_spin_exit(&sc->sc_lock);

	if (sc->sc_rx_ready || sc->sc_tx_done)
		softint_schedule(sc->sc_si);

	return handled;
}

/*
 * Soft interrupt (IPL_SOFTSERIAL / spltty)
 */
static void
psc_rxsoft(struct psc_softc *sc, struct tty *tp)
{
	int (*rint)(int, struct tty *) = tp->t_linesw->l_rint;

	/*
	 * Single-producer (pscintr at IPL_HIGH) / single-consumer ring, so
	 * sc_rbput can be read without the lock; we only advance sc_rbget.
	 */
	while (sc->sc_rbget != sc->sc_rbput) {
		uint8_t c = sc->sc_rbuf[sc->sc_rbget];

		if ((*rint)(c, tp) == -1)
			break;			/* tty buffer full; retry later */
		sc->sc_rbget = (sc->sc_rbget + 1) & PSC_RING_MASK;
	}
}

static void
pscsoft(void *arg)
{
	struct psc_softc *sc = arg;
	struct tty *tp = sc->sc_tty;

	if (tp == NULL)
		return;

	if (sc->sc_rx_ready) {
		sc->sc_rx_ready = false;
		psc_rxsoft(sc, tp);
	}

	if (sc->sc_tx_done) {
		sc->sc_tx_done = false;
		CLR(tp->t_state, TS_BUSY);
		if (ISSET(tp->t_state, TS_FLUSH))
			CLR(tp->t_state, TS_FLUSH);
		else
			ndflush(&tp->t_outq,
			    (int)(sc->sc_tba - tp->t_outq.c_cf));
		(*tp->t_linesw->l_start)(tp);
	}
}

/* tty oproc: queue the next contiguous output chunk and arm tx interrupts. */
static void
pscstart(struct tty *tp)
{
	struct psc_softc *sc = device_lookup_private(&psc_cd, PSCUNIT(tp->t_dev));
	int s;

	if (sc == NULL)
		return;

	s = spltty();
	if (ISSET(tp->t_state, TS_BUSY | TS_TIMEOUT | TS_TTSTOP))
		goto out;
	if (!ttypull(tp))
		goto out;

	SET(tp->t_state, TS_BUSY);

	mutex_spin_enter(&sc->sc_lock);
	sc->sc_tba = tp->t_outq.c_cf;
	sc->sc_tbc = ndqb(&tp->t_outq, 0);
	sc->sc_tx_busy = true;
	if (!(sc->sc_imr & INT_TXRDY)) {
		sc->sc_imr |= INT_TXRDY;
		PSC_WRITE_2(sc, PSC_IMR, sc->sc_imr);
	}
	mutex_spin_exit(&sc->sc_lock);
out:
	splx(s);
}

static int
pscparam(struct tty *tp, struct termios *t)
{
	struct psc_softc *sc = device_lookup_private(&psc_cd, PSCUNIT(tp->t_dev));

	if (sc == NULL)
		return ENXIO;

	/*
	 * XXX: accept the requested termios but do not reprogram the 
	 * bit clock here, so a stray speed change cannot make the console 
	 * unreadable.
	 */
	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;

	return 0;
}

int
pscopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct psc_softc *sc = device_lookup_private(&psc_cd, PSCUNIT(dev));
	struct tty *tp;
	int error, s;

	if (sc == NULL)
		return ENXIO;
	tp = sc->sc_tty;

	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp))
		return EBUSY;

	s = spltty();
	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		struct termios t;

		tp->t_dev = dev;

		/* Enable receive (and break) interrupts. */
		mutex_spin_enter(&sc->sc_lock);
		sc->sc_imr |= INT_RXRDY;
		PSC_WRITE_2(sc, PSC_IMR, sc->sc_imr);
		mutex_spin_exit(&sc->sc_lock);

		t.c_ospeed = t.c_ispeed = PSC_CONSOLE_SPEED;
		t.c_cflag = (CREAD | CS8 | HUPCL | CLOCAL);
		tp->t_ospeed = 0;
		(void)pscparam(tp, &t);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		ttychars(tp);
		ttsetwater(tp);
	}
	splx(s);

	error = ttyopen(tp, 0, ISSET(flag, O_NONBLOCK));
	if (error)
		return error;

	return (*tp->t_linesw->l_open)(dev, tp);
}

int
pscclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct psc_softc *sc = device_lookup_private(&psc_cd, PSCUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (!ISSET(tp->t_state, TS_ISOPEN))
		return 0;

	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);

	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		/* Last close: leave RX interrupts on for the console. */
		if (!sc->sc_console) {
			mutex_spin_enter(&sc->sc_lock);
			sc->sc_imr &= ~INT_RXRDY;
			PSC_WRITE_2(sc, PSC_IMR, sc->sc_imr);
			mutex_spin_exit(&sc->sc_lock);
		}
	}
	return 0;
}

int
pscread(dev_t dev, struct uio *uio, int flag)
{
	struct psc_softc *sc = device_lookup_private(&psc_cd, PSCUNIT(dev));
	struct tty *tp = sc->sc_tty;

	return (*tp->t_linesw->l_read)(tp, uio, flag);
}

int
pscwrite(dev_t dev, struct uio *uio, int flag)
{
	struct psc_softc *sc = device_lookup_private(&psc_cd, PSCUNIT(dev));
	struct tty *tp = sc->sc_tty;

	return (*tp->t_linesw->l_write)(tp, uio, flag);
}

int
pscpoll(dev_t dev, int events, struct lwp *l)
{
	struct psc_softc *sc = device_lookup_private(&psc_cd, PSCUNIT(dev));
	struct tty *tp = sc->sc_tty;

	return (*tp->t_linesw->l_poll)(tp, events, l);
}

struct tty *
psctty(dev_t dev)
{
	struct psc_softc *sc = device_lookup_private(&psc_cd, PSCUNIT(dev));

	return sc->sc_tty;
}

int
pscioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct psc_softc *sc = device_lookup_private(&psc_cd, PSCUNIT(dev));
	struct tty *tp = sc->sc_tty;
	int error;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	return ttioctl(tp, cmd, data, flag, l);
}

void
pscstop(struct tty *tp, int flag)
{
	struct psc_softc *sc = device_lookup_private(&psc_cd, PSCUNIT(tp->t_dev));

	if (sc == NULL)
		return;

	mutex_spin_enter(&sc->sc_lock);
	if (ISSET(tp->t_state, TS_BUSY)) {
		sc->sc_tbc = 0;
		if (!ISSET(tp->t_state, TS_TTSTOP))
			SET(tp->t_state, TS_FLUSH);
	}
	mutex_spin_exit(&sc->sc_lock);
}

/*
 * Console interface.
 */

static void
psccnprobe(struct consdev *cp)
{
	/* Selection is performed by ofwoea_cnprobe(); nothing to do. */
}

static void
psccninit(struct consdev *cp)
{
	/*
	 * The board's console is PSC1, at a fixed MBAR offset.
	 * XXX: This should be configurable.
	 */
	psc_console.cs_base =
	    (volatile uint8_t *)(MPC5200_MBAR_DEFAULT + MPC5200_REG_PSC1);
	psc_console.cs_mapped = true;
}

static int
psccngetc(dev_t dev)
{

	if (!psc_console.cs_mapped)
		return -1;
	if ((PSC_CN_SR(&psc_console) & SR_RXRDY) == 0)
		return -1;
	return PSC_CN_RB(&psc_console);
}

static void
psccnputc(dev_t dev, int c)
{

	if (!psc_console.cs_mapped)
		return;
	if (c == '\n')
		psccnputc(dev, '\r');
	while ((PSC_CN_SR(&psc_console) & SR_TXRDY) == 0)
		continue;
	PSC_CN_TB(&psc_console) = c;
}

static void
psccnpollc(dev_t dev, int on)
{
	/* The low-level console path is always polled. */
}
