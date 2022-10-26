/* $NetBSD: meson_uart.c,v 1.7 2022/10/26 23:38:06 riastradh Exp $ */

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

#include "opt_console.h"
#include "locators.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: meson_uart.c,v 1.7 2022/10/26 23:38:06 riastradh Exp $");

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

#include <ddb/db_active.h>

#include <dev/cons.h>

#include <dev/fdt/fdtvar.h>

#include <arm/amlogic/meson_uart.h>

static int	meson_uart_match(device_t, cfdata_t, void *);
static void	meson_uart_attach(device_t, device_t, void *);

static int	meson_uart_intr(void *);
static void	meson_uart_rxsoft(void *);

static int	meson_uart_cngetc(dev_t);
static void	meson_uart_cnputc(dev_t, int);
static void	meson_uart_cnpollc(dev_t, int);

static void	meson_uart_start(struct tty *);
static int	meson_uart_param(struct tty *, struct termios *);

extern struct cfdriver mesonuart_cd;

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "amlogic,meson6-uart" },
	{ .compat = "amlogic,meson8-uart" },
	{ .compat = "amlogic,meson8b-uart" },
	{ .compat = "amlogic,meson-gx-uart" },
	DEVICE_COMPAT_EOL
};

struct meson_uart_softc {
	device_t sc_dev;
	bus_space_tag_t	sc_bst;
	bus_space_handle_t sc_bsh;
	kmutex_t sc_intr_lock;
	void *sc_ih;
	void *sc_sih;

	struct tty *sc_tty;

	int sc_ospeed;
	unsigned int sc_rbuf_w;		/* write ptr of sc_rbuf[] */
	unsigned int sc_rbuf_r;		/* read ptr of sc_rbuf[] */
	tcflag_t sc_cflag;

	u_char sc_buf[1024];
#define MESON_RBUFSZ	128		/* must be 2^n */
	u_char sc_rbuf[MESON_RBUFSZ];	/* good enough for sizeof RXFIFO */
};

static int meson_uart_console_phandle = -1;

static struct meson_uart_softc meson_uart_cnsc;

static struct cnm_state meson_uart_cnm_state;

struct consdev meson_uart_consdev = {
	.cn_getc = meson_uart_cngetc,
	.cn_putc = meson_uart_cnputc,
	.cn_pollc = meson_uart_cnpollc,
	.cn_dev = NODEV,
	.cn_pri = CN_NORMAL,
};

static dev_type_open(meson_uart_open);
static dev_type_open(meson_uart_close);
static dev_type_read(meson_uart_read);
static dev_type_write(meson_uart_write);
static dev_type_ioctl(meson_uart_ioctl);
static dev_type_tty(meson_uart_tty);
static dev_type_poll(meson_uart_poll);
static dev_type_stop(meson_uart_stop);

const struct cdevsw mesonuart_cdevsw = {
	.d_open = meson_uart_open,
	.d_close = meson_uart_close,
	.d_read = meson_uart_read,
	.d_write = meson_uart_write,
	.d_ioctl = meson_uart_ioctl,
	.d_stop = meson_uart_stop,
	.d_tty = meson_uart_tty,
	.d_poll = meson_uart_poll,
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};

static int meson_uart_cmajor = -1;

CFATTACH_DECL_NEW(meson_uart, sizeof(struct meson_uart_softc),
	meson_uart_match, meson_uart_attach, NULL, NULL);

static int
meson_uart_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
meson_uart_attach(device_t parent, device_t self, void *aux)
{
	struct meson_uart_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	struct tty *tp;
	int major, minor, error;
	uint32_t misc, control;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;

	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_SERIAL);
	sc->sc_ih = fdtbus_intr_establish_xname(phandle, 0, IPL_SERIAL,
	    FDT_INTR_MPSAFE, meson_uart_intr, sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error(": failed to establish interrupt on %s\n",
		    intrstr);
		return;
	}

	sc->sc_sih = softint_establish(SOFTINT_SERIAL, meson_uart_rxsoft, sc);
	if (sc->sc_sih == NULL) {
		aprint_error(": failed to establish softint\n");
		return;
	}

	if (meson_uart_cmajor == -1) {
		/* allocate a major number */
		int bmajor = -1, cmajor = -1;
		error = devsw_attach("mesonuart", NULL, &bmajor,
		    &mesonuart_cdevsw, &cmajor);
		if (error) {
			aprint_error(": couldn't allocate major number\n");
			return;
		}
		meson_uart_cmajor = cmajor;
	}

	major = cdevsw_lookup_major(&mesonuart_cdevsw);
	minor = device_unit(self);

	tp = sc->sc_tty = tty_alloc();
	tp->t_oproc = meson_uart_start;
	tp->t_param = meson_uart_param;
	tp->t_dev = makedev(major, minor);
	tp->t_sc = sc;
	tty_attach(tp);

	aprint_naive("\n");
	if (meson_uart_console_phandle == phandle) {
		cn_tab->cn_dev = tp->t_dev;
		aprint_normal(": console");
	}
	aprint_normal("\n");

	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	misc = bus_space_read_4(sc->sc_bst, sc->sc_bsh, UART_MISC_REG);
	misc &= ~UART_MISC_TX_IRQ_CNT;
	misc |= __SHIFTIN(0, UART_MISC_TX_IRQ_CNT);
	misc &= ~UART_MISC_RX_IRQ_CNT;
	misc |= __SHIFTIN(1, UART_MISC_RX_IRQ_CNT);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, UART_MISC_REG, misc);

	control = bus_space_read_4(sc->sc_bst, sc->sc_bsh, UART_CONTROL_REG);
	control &= ~(UART_CONTROL_TX_INT_EN|UART_CONTROL_RX_INT_EN);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, UART_CONTROL_REG, control);
}

static int
meson_uart_cngetc(dev_t dev)
{
	bus_space_tag_t bst = meson_uart_cnsc.sc_bst;
	bus_space_handle_t bsh = meson_uart_cnsc.sc_bsh;
	uint32_t status;
	int s, c;

	s = splserial();

	status = bus_space_read_4(bst, bsh, UART_STATUS_REG);
	if (status & UART_STATUS_RX_EMPTY) {
		splx(s);
		return -1;
	}

	c = bus_space_read_4(bst, bsh, UART_RFIFO_REG) & 0xff;
	if (!db_active) {
		int cn_trapped __unused = 0;
		cn_check_magic(dev, c, meson_uart_cnm_state);
	}

	splx(s);

	return c;
}

static void
meson_uart_cnputc(dev_t dev, int c)
{
	bus_space_tag_t bst = meson_uart_cnsc.sc_bst;
	bus_space_handle_t bsh = meson_uart_cnsc.sc_bsh;
	int s;

	s = splserial();

	while ((bus_space_read_4(bst, bsh, UART_STATUS_REG) & UART_STATUS_TX_FULL) != 0)
		;

	bus_space_write_4(bst, bsh, UART_WFIFO_REG, c);

	splx(s);
}
	

static void
meson_uart_cnpollc(dev_t dev, int on)
{
}

static int
meson_uart_open(dev_t dev, int flag, int mode, lwp_t *l)
{
	struct meson_uart_softc *sc =
	    device_lookup_private(&mesonuart_cd, minor(dev));
	struct tty *tp = sc->sc_tty;
	uint32_t control;

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

		control = bus_space_read_4(sc->sc_bst, sc->sc_bsh, UART_CONTROL_REG);
		control |= UART_CONTROL_RX_INT_EN;
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, UART_CONTROL_REG, control);
	}
	tp->t_state |= TS_CARR_ON;

	return tp->t_linesw->l_open(dev, tp);
}

static int
meson_uart_close(dev_t dev, int flag, int mode, lwp_t *l)
{
	struct meson_uart_softc *sc =
	    device_lookup_private(&mesonuart_cd, minor(dev));
	struct tty *tp = sc->sc_tty;
	uint32_t control;

	tp->t_linesw->l_close(tp, flag);
	ttyclose(tp);

	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		control = bus_space_read_4(sc->sc_bst, sc->sc_bsh, UART_CONTROL_REG);
		control &= ~UART_CONTROL_RX_INT_EN;
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, UART_CONTROL_REG, control);
	}

	return 0;
}

static int
meson_uart_read(dev_t dev, struct uio *uio, int flag)
{
	struct meson_uart_softc *sc =
	    device_lookup_private(&mesonuart_cd, minor(dev));
	struct tty *tp = sc->sc_tty;

	return tp->t_linesw->l_read(tp, uio, flag);
}

static int
meson_uart_write(dev_t dev, struct uio *uio, int flag)
{
	struct meson_uart_softc *sc =
	    device_lookup_private(&mesonuart_cd, minor(dev));
	struct tty *tp = sc->sc_tty;

	return tp->t_linesw->l_write(tp, uio, flag);
}

static int
meson_uart_poll(dev_t dev, int events, lwp_t *l)
{
	struct meson_uart_softc *sc =
	    device_lookup_private(&mesonuart_cd, minor(dev));
	struct tty *tp = sc->sc_tty;

	return tp->t_linesw->l_poll(tp, events, l);
}

static int
meson_uart_ioctl(dev_t dev, u_long cmd, void *data, int flag, lwp_t *l)
{
	struct meson_uart_softc *sc =
	    device_lookup_private(&mesonuart_cd, minor(dev));
	struct tty *tp = sc->sc_tty;
	int error;

	error = tp->t_linesw->l_ioctl(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	return ttioctl(tp, cmd, data, flag, l);
}

static struct tty *
meson_uart_tty(dev_t dev)
{
	struct meson_uart_softc *sc =
	    device_lookup_private(&mesonuart_cd, minor(dev));

	return sc->sc_tty;
}

static void
meson_uart_stop(struct tty *tp, int flag)
{
}

static void
meson_uart_start(struct tty *tp)
{
	struct meson_uart_softc *sc = tp->t_sc;
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
meson_uart_param(struct tty *tp, struct termios *t)
{

	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;

	return 0;
}

static int
meson_uart_intr(void *priv)
{
	struct meson_uart_softc *sc = priv;
	struct tty *tp = sc->sc_tty;
	uint32_t status, c;

	mutex_spin_enter(&sc->sc_intr_lock);
	for (;;) {
		int cn_trapped = 0;
		status = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
		    UART_STATUS_REG);
		if (status & UART_STATUS_RX_EMPTY) {
			break;
		}
		if (status & UART_STATUS_BREAK) {
			cn_check_magic(tp->t_dev, CNC_BREAK,
			    meson_uart_cnm_state);
			if (cn_trapped)
				continue;
		}

		c = bus_space_read_4(sc->sc_bst, sc->sc_bsh, UART_RFIFO_REG);
		cn_check_magic(tp->t_dev, c & 0xff, meson_uart_cnm_state);
		if (cn_trapped)
			continue;

		if ((sc->sc_rbuf_w - sc->sc_rbuf_r) >= (MESON_RBUFSZ - 1))
			continue;
		sc->sc_rbuf[sc->sc_rbuf_w++ & (MESON_RBUFSZ - 1)] = c;
	}
	mutex_spin_exit(&sc->sc_intr_lock);

	if (sc->sc_rbuf_w != sc->sc_rbuf_r)
		softint_schedule(sc->sc_sih);

	return 0;
}

static void
meson_uart_rxsoft(void *priv)
{
	struct meson_uart_softc *sc = priv;
	struct tty *tp = sc->sc_tty;
	int c;

	while (sc->sc_rbuf_w != sc->sc_rbuf_r) {
		c = sc->sc_rbuf[sc->sc_rbuf_r++ & (MESON_RBUFSZ - 1)];
		if (tp->t_linesw->l_rint(c & 0xff, tp) == -1)
			break;
	}
}

static int
meson_uart_console_match(int phandle)
{
	return of_compatible_match(phandle, compat_data);
}

static void
meson_uart_console_consinit(struct fdt_attach_args *faa, u_int uart_freq)
{
	struct meson_uart_softc *sc = &meson_uart_cnsc;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	fdtbus_get_reg(phandle, 0, &addr, &size);

	sc->sc_bst = faa->faa_bst;
	sc->sc_ospeed = fdtbus_get_stdout_speed();
	if (sc->sc_ospeed < 0)
		sc->sc_ospeed = 115200;
	sc->sc_cflag = fdtbus_get_stdout_flags();

	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error != 0)
		panic("failed to map console, error = %d", error);

	cn_tab = &meson_uart_consdev;
	cn_init_magic(&meson_uart_cnm_state);
	cn_set_magic("\047\001");

	meson_uart_console_phandle = phandle;
}

static const struct fdt_console meson_uart_console = {
	.match = meson_uart_console_match,
	.consinit = meson_uart_console_consinit,
};

FDT_CONSOLE(meson_uart, &meson_uart_console);
