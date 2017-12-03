/* $NetBSD: amlogic_com.c,v 1.5.18.2 2017/12/03 11:35:51 jdolecek Exp $ */

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include "locators.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: amlogic_com.c,v 1.5.18.2 2017/12/03 11:35:51 jdolecek Exp $");

#define cn_trap()			\
	do {				\
		console_debugger();	\
		cn_trapped = 1;		\
	} while (/* CONSTCOND */ 0)

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/termios.h>
#include <sys/kauth.h>
#include <sys/lwp.h>
#include <sys/tty.h>

#include <dev/cons.h>

#include <arm/amlogic/amlogic_reg.h>
#include <arm/amlogic/amlogic_var.h>
#include <arm/amlogic/amlogic_comreg.h>
#include <arm/amlogic/amlogic_comvar.h>

static int	amlogic_com_match(device_t, cfdata_t, void *);
static void	amlogic_com_attach(device_t, device_t, void *);

static int	amlogic_com_intr(void *);

static int	amlogic_com_cngetc(dev_t);
static void	amlogic_com_cnputc(dev_t, int);
static void	amlogic_com_cnpollc(dev_t, int);

static void	amlogic_com_start(struct tty *);
static int	amlogic_com_param(struct tty *, struct termios *);

extern struct cfdriver amlogiccom_cd;

struct amlogic_com_softc {
	device_t sc_dev;
	bus_space_tag_t	sc_bst;
	bus_space_handle_t sc_bsh;
	void *sc_ih;

	struct tty *sc_tty;

	int sc_ospeed;
	tcflag_t sc_cflag;

	u_char sc_buf[1024];
};

static struct amlogic_com_softc amlogic_com_cnsc;

static struct cnm_state amlogic_com_cnm_state;

struct consdev amlogic_com_consdev = {
	.cn_getc = amlogic_com_cngetc,
	.cn_putc = amlogic_com_cnputc,
	.cn_pollc = amlogic_com_cnpollc,
	.cn_dev = NODEV,
	.cn_pri = CN_NORMAL,
};

static dev_type_open(amlogic_com_open);
static dev_type_open(amlogic_com_close);
static dev_type_read(amlogic_com_read);
static dev_type_write(amlogic_com_write);
static dev_type_ioctl(amlogic_com_ioctl);
static dev_type_tty(amlogic_com_tty);
static dev_type_poll(amlogic_com_poll);
static dev_type_stop(amlogic_com_stop);

const struct cdevsw amlogiccom_cdevsw = {
	.d_open = amlogic_com_open,
	.d_close = amlogic_com_close,
	.d_read = amlogic_com_read,
	.d_write = amlogic_com_write,
	.d_ioctl = amlogic_com_ioctl,
	.d_stop = amlogic_com_stop,
	.d_tty = amlogic_com_tty,
	.d_poll = amlogic_com_poll,
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};

static int amlogic_com_cmajor = -1;

CFATTACH_DECL_NEW(amlogic_com, sizeof(struct amlogic_com_softc),
	amlogic_com_match, amlogic_com_attach, NULL, NULL);

static int
amlogic_com_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
amlogic_com_attach(device_t parent, device_t self, void *aux)
{
	struct amlogic_com_softc * const sc = device_private(self);
	struct amlogicio_attach_args * const aio = aux;
	const struct amlogic_locators * const loc = &aio->aio_loc;
	const bus_addr_t iobase = AMLOGIC_CORE_BASE + loc->loc_offset;
	struct tty *tp;
	int major, minor;
	uint32_t misc, control;

	sc->sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	bus_space_subregion(aio->aio_core_bst, aio->aio_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	sc->sc_ih = intr_establish(loc->loc_intr, IPL_SERIAL,
	    IST_EDGE | IST_MPSAFE, amlogic_com_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error(": failed to establish interrupt %d\n",
		    loc->loc_intr);
		return;
	}

	if (amlogic_com_cmajor == -1) {
		/* allocate a major number */
		int bmajor = -1, cmajor = -1;
		int error = devsw_attach("amlogiccom", NULL, &bmajor,
		    &amlogiccom_cdevsw, &cmajor);
		if (error) {
			aprint_error(": couldn't allocate major number\n");
			return;
		}
		amlogic_com_cmajor = cmajor;
	}

	major = cdevsw_lookup_major(&amlogiccom_cdevsw);
	minor = device_unit(self);

	tp = sc->sc_tty = tty_alloc();
	tp->t_oproc = amlogic_com_start;
	tp->t_param = amlogic_com_param;
	tp->t_dev = makedev(major, minor);
	tp->t_sc = sc;
	tty_attach(tp);

	aprint_naive("\n");
	if (amlogic_com_is_console(iobase)) {
		cn_tab->cn_dev = tp->t_dev;
		aprint_normal(": console");
	}
	aprint_normal("\n");

	aprint_normal_dev(self, "interrupting at irq %d\n", loc->loc_intr);

	misc = bus_space_read_4(sc->sc_bst, sc->sc_bsh, UART_MISC_REG);
	misc &= ~UART_MISC_TX_IRQ_CNT;
	misc |= __SHIFTIN(0, UART_MISC_TX_IRQ_CNT);
	misc &= ~UART_MISC_RX_IRQ_CNT;
	misc |= __SHIFTIN(1, UART_MISC_RX_IRQ_CNT);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, UART_MISC_REG, misc);

	control = bus_space_read_4(sc->sc_bst, sc->sc_bsh, UART_CONTROL_REG);
	control &= ~UART_CONTROL_TX_INT_EN;
	control |= UART_CONTROL_RX_INT_EN;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, UART_CONTROL_REG, control);
}

static int
amlogic_com_cngetc(dev_t dev)
{
	bus_space_tag_t bst = amlogic_com_cnsc.sc_bst;
	bus_space_handle_t bsh = amlogic_com_cnsc.sc_bsh;
	uint32_t status;
	int s, c;

	s = splserial();

	status = bus_space_read_4(bst, bsh, UART_STATUS_REG);
	if (status & UART_STATUS_RX_EMPTY) {
		splx(s);
		return -1;
	}

	c = bus_space_read_4(bst, bsh, UART_RFIFO_REG);
#if defined(DDB)
	extern int db_active;
	if (!db_active)
#endif
	{
		int cn_trapped __unused = 0;
		cn_check_magic(dev, c, amlogic_com_cnm_state);
	}

	splx(s);

	return c & 0xff;
}

static void
amlogic_com_cnputc(dev_t dev, int c)
{
	bus_space_tag_t bst = amlogic_com_cnsc.sc_bst;
	bus_space_handle_t bsh = amlogic_com_cnsc.sc_bsh;
	int s;

	s = splserial();

	while ((bus_space_read_4(bst, bsh, UART_STATUS_REG) & UART_STATUS_TX_FULL) != 0)
		;

	bus_space_write_4(bst, bsh, UART_WFIFO_REG, c);

	splx(s);
}
	

static void
amlogic_com_cnpollc(dev_t dev, int on)
{
}

bool
amlogic_com_cnattach(bus_space_tag_t bst, bus_space_handle_t bsh,
    int ospeed, tcflag_t cflag)
{
	struct amlogic_com_softc *sc = &amlogic_com_cnsc;

	cn_tab = &amlogic_com_consdev;
	cn_init_magic(&amlogic_com_cnm_state);
	cn_set_magic("\047\001");

	sc->sc_bst = bst;
	sc->sc_bsh = bsh;
	sc->sc_ospeed = ospeed;
	sc->sc_cflag = cflag;

	return true;
}

bool
amlogic_com_is_console(bus_addr_t iobase)
{
	return iobase == CONSADDR;
}

static int
amlogic_com_open(dev_t dev, int flag, int mode, lwp_t *l)
{
	struct amlogic_com_softc *sc =
	    device_lookup_private(&amlogiccom_cd, minor(dev));
	struct tty *tp = sc->sc_tty;

	if (kauth_authorize_device_tty(l->l_cred,
	    KAUTH_DEVICE_TTY_OPEN, tp) != 0) {
		return EBUSY;
	}

	if ((tp->t_state & TS_ISOPEN) == 0 && tp->t_wopen == 0) {
		tp->t_dev = dev;
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		ttsetwater(tp);
	}
	tp->t_state |= TS_CARR_ON;

	return tp->t_linesw->l_open(dev, tp);
}

static int
amlogic_com_close(dev_t dev, int flag, int mode, lwp_t *l)
{
	struct amlogic_com_softc *sc =
	    device_lookup_private(&amlogiccom_cd, minor(dev));
	struct tty *tp = sc->sc_tty;

	tp->t_linesw->l_close(tp, flag);
	ttyclose(tp);

	return 0;
}

static int
amlogic_com_read(dev_t dev, struct uio *uio, int flag)
{
	struct amlogic_com_softc *sc =
	    device_lookup_private(&amlogiccom_cd, minor(dev));
	struct tty *tp = sc->sc_tty;

	return tp->t_linesw->l_read(tp, uio, flag);
}

static int
amlogic_com_write(dev_t dev, struct uio *uio, int flag)
{
	struct amlogic_com_softc *sc =
	    device_lookup_private(&amlogiccom_cd, minor(dev));
	struct tty *tp = sc->sc_tty;

	return tp->t_linesw->l_write(tp, uio, flag);
}

static int
amlogic_com_poll(dev_t dev, int events, lwp_t *l)
{
	struct amlogic_com_softc *sc =
	    device_lookup_private(&amlogiccom_cd, minor(dev));
	struct tty *tp = sc->sc_tty;

	return tp->t_linesw->l_poll(tp, events, l);
}

static int
amlogic_com_ioctl(dev_t dev, u_long cmd, void *data, int flag, lwp_t *l)
{
	struct amlogic_com_softc *sc =
	    device_lookup_private(&amlogiccom_cd, minor(dev));
	struct tty *tp = sc->sc_tty;
	int error;

	error = tp->t_linesw->l_ioctl(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	return ttioctl(tp, cmd, data, flag, l);
}

static struct tty *
amlogic_com_tty(dev_t dev)
{
	struct amlogic_com_softc *sc =
	    device_lookup_private(&amlogiccom_cd, minor(dev));

	return sc->sc_tty;
}

static void
amlogic_com_stop(struct tty *tp, int flag)
{
}

static void
amlogic_com_start(struct tty *tp)
{
	struct amlogic_com_softc *sc = tp->t_sc;
	u_char *p = sc->sc_buf;
	int s, brem;

	s = spltty();

	if (tp->t_state & (TS_TTSTOP | TS_BUSY | TS_TIMEOUT)) {
		splx(s);
		return;
	}
	tp->t_state |= TS_BUSY;

	splx(s);

	for (brem = q_to_b(&tp->t_outq, sc->sc_buf, sizeof(sc->sc_buf));
	     brem > 0;
	     brem--, p++) {
		while ((bus_space_read_4(sc->sc_bst, sc->sc_bsh,
		    UART_STATUS_REG) & UART_STATUS_TX_FULL) != 0)
			;

		bus_space_write_4(sc->sc_bst, sc->sc_bsh,
		    UART_WFIFO_REG, *p);
	}

	s = spltty();
	tp->t_state &= ~TS_BUSY;
	if (ttypull(tp)) {
		tp->t_state |= TS_TIMEOUT;
		callout_schedule(&tp->t_rstrt_ch, 1);
	}
	splx(s);
}

static int
amlogic_com_param(struct tty *tp, struct termios *t)
{

	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;

	return 0;
}

static int
amlogic_com_intr(void *priv)
{
	struct amlogic_com_softc *sc = priv;
	struct tty *tp = sc->sc_tty;
	uint32_t status, c;

	for (;;) {
		int cn_trapped = 0;
		status = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
		    UART_STATUS_REG);
		if (status & UART_STATUS_RX_EMPTY) {
			break;
		}
		if (status & UART_STATUS_BREAK) {
			cn_check_magic(tp->t_dev, CNC_BREAK,
			    amlogic_com_cnm_state);
			if (cn_trapped)
				continue;
		}

		c = bus_space_read_4(sc->sc_bst, sc->sc_bsh, UART_RFIFO_REG);
		cn_check_magic(tp->t_dev, c & 0xff, amlogic_com_cnm_state);
		if (cn_trapped)
			continue;
		tp->t_linesw->l_rint(c & 0xff, tp);
	}

	return 0;
}
