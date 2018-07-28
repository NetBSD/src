/* $NetBSD: exynos_uart.c,v 1.1.2.2 2018/07/28 04:37:29 pgoyette Exp $ */

/*-
 * Copyright (c) 2013-2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry and Jared McNeill.
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

__KERNEL_RCSID(1, "$NetBSD: exynos_uart.c,v 1.1.2.2 2018/07/28 04:37:29 pgoyette Exp $");

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

#include <dev/fdt/fdtvar.h>

#include <arm/samsung/sscom_reg.h>

static int	exynos_uart_match(device_t, cfdata_t, void *);
static void	exynos_uart_attach(device_t, device_t, void *);

static int	exynos_uart_intr(void *);

static int	exynos_uart_cngetc(dev_t);
static void	exynos_uart_cnputc(dev_t, int);
static void	exynos_uart_cnpollc(dev_t, int);

static void	exynos_uart_start(struct tty *);
static int	exynos_uart_param(struct tty *, struct termios *);

extern struct cfdriver exuart_cd;

struct exynos_uart_softc {
	device_t sc_dev;
	bus_space_tag_t	sc_bst;
	bus_space_handle_t sc_bsh;
	u_int sc_freq;
	void *sc_ih;

	bool sc_console;
	struct tty *sc_tty;

	int sc_ospeed;
	tcflag_t sc_cflag;

	u_char sc_buf[1024];
};

#define	RD4(sc, reg)			\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR4(sc, reg, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static bus_addr_t exynos_uart_consaddr;

static struct exynos_uart_softc exynos_uart_cnsc;

static struct cnm_state exynos_uart_cnm_state;

struct consdev exynos_uart_consdev = {
	.cn_getc = exynos_uart_cngetc,
	.cn_putc = exynos_uart_cnputc,
	.cn_pollc = exynos_uart_cnpollc,
	.cn_dev = NODEV,
	.cn_pri = CN_NORMAL,
};

static dev_type_open(exynos_uart_open);
static dev_type_open(exynos_uart_close);
static dev_type_read(exynos_uart_read);
static dev_type_write(exynos_uart_write);
static dev_type_ioctl(exynos_uart_ioctl);
static dev_type_tty(exynos_uart_tty);
static dev_type_poll(exynos_uart_poll);
static dev_type_stop(exynos_uart_stop);

const struct cdevsw exuart_cdevsw = {
	.d_open = exynos_uart_open,
	.d_close = exynos_uart_close,
	.d_read = exynos_uart_read,
	.d_write = exynos_uart_write,
	.d_ioctl = exynos_uart_ioctl,
	.d_stop = exynos_uart_stop,
	.d_tty = exynos_uart_tty,
	.d_poll = exynos_uart_poll,
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};

static int exynos_uart_cmajor = -1;

static const char * const compatible[] = {
	"samsung,exynos4210-uart",
	NULL
};

CFATTACH_DECL_NEW(exynos_uart, sizeof(struct exynos_uart_softc),
	exynos_uart_match, exynos_uart_attach, NULL, NULL);

static int
exynos_uart_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
exynos_uart_attach(device_t parent, device_t self, void *aux)
{
	struct exynos_uart_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	struct clk *clk_uart, *clk_uart_baud0;
	struct tty *tp;
	int major, minor;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}
	clk_uart = fdtbus_clock_get(phandle, "uart");
	if (clk_uart == NULL || clk_enable(clk_uart) != 0) {
		aprint_error(": failed to enable uart clock\n");
		return;
	}
	clk_uart_baud0 = fdtbus_clock_get(phandle, "clk_uart_baud0");
	if (clk_uart_baud0 == NULL || clk_enable(clk_uart_baud0) != 0) {
		aprint_error(": failed to enable clk_uart_baud0 clock\n");
		return;
	}

	const bool is_console = exynos_uart_consaddr == addr;

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	sc->sc_console = is_console;
	if (is_console) {
		sc->sc_bsh = exynos_uart_cnsc.sc_bsh;
	} else {
		if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
			aprint_error(": failed to map registers\n");
			return;
		}
	}
	sc->sc_freq = clk_get_rate(clk_uart_baud0);

	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_SERIAL,
	    0, exynos_uart_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error(": failed to establish interrupt on %s\n",
		    intrstr);
		return;
	}

	if (exynos_uart_cmajor == -1) {
		/* allocate a major number */
		int bmajor = -1, cmajor = -1;
		int error = devsw_attach("exuart", NULL, &bmajor,
		    &exuart_cdevsw, &cmajor);
		if (error) {
			aprint_error(": couldn't allocate major number\n");
			return;
		}
		exynos_uart_cmajor = cmajor;
	}

	major = cdevsw_lookup_major(&exuart_cdevsw);
	minor = device_unit(self);

	tp = sc->sc_tty = tty_alloc();
	tp->t_oproc = exynos_uart_start;
	tp->t_param = exynos_uart_param;
	tp->t_dev = makedev(major, minor);
	tp->t_sc = sc;
	tty_attach(tp);

	aprint_naive("\n");
	if (is_console) {
		cn_tab->cn_dev = tp->t_dev;
		aprint_normal(": console");
	}
	aprint_normal("\n");

	if (is_console)
		delay(10000);

	/* Initialize device */
	WR4(sc, SSCOM_UFCON,
	    __SHIFTIN(2, UFCON_TXTRIGGER) |
	    __SHIFTIN(1, UFCON_RXTRIGGER) |
	    UFCON_TXFIFO_RESET | UFCON_RXFIFO_RESET |
	    UFCON_FIFO_ENABLE);
	/* Configure PIO mode with RX timeout interrupts */
	WR4(sc, SSCOM_UCON,
	    __SHIFTIN(3, UCON_RXTO) |
	    UCON_TOINT | UCON_ERRINT |
	    UCON_TXMODE_INT | UCON_RXMODE_INT);

	/* Disable interrupts */
	WR4(sc, SSCOM_UINTM, ~0u);

	aprint_normal_dev(self, "interrupting on %s\n", intrstr);
}

static int
exynos_uart_cngetc(dev_t dev)
{
	struct exynos_uart_softc * const sc = &exynos_uart_cnsc;
	uint32_t status;
	int s, c;

	s = splserial();

	status = RD4(sc, SSCOM_UTRSTAT);
	if ((status & UTRSTAT_RXREADY) == 0) {
		splx(s);
		return -1;
	}

	c = bus_space_read_1(sc->sc_bst, sc->sc_bsh, SSCOM_URXH);
#if defined(DDB)
	extern int db_active;
	if (!db_active)
#endif
	{
		int cn_trapped __unused = 0;
		cn_check_magic(dev, c, exynos_uart_cnm_state);
	}

	splx(s);

	return c & 0xff;
}

static void
exynos_uart_cnputc(dev_t dev, int c)
{
	struct exynos_uart_softc * const sc = &exynos_uart_cnsc;
	int s;

	s = splserial();
	while ((RD4(sc, SSCOM_UFSTAT) & UFSTAT_TXFULL) != 0)
		;

	bus_space_write_1(sc->sc_bst, sc->sc_bsh, SSCOM_UTXH, c);

	splx(s);
}
	

static void
exynos_uart_cnpollc(dev_t dev, int on)
{
}

static void
exynos_uart_cnattach(bus_space_tag_t bst, bus_space_handle_t bsh,
    int ospeed, tcflag_t cflag)
{
	struct exynos_uart_softc *sc = &exynos_uart_cnsc;

	cn_tab = &exynos_uart_consdev;
	cn_init_magic(&exynos_uart_cnm_state);
	cn_set_magic("\047\001");

	sc->sc_bst = bst;
	sc->sc_bsh = bsh;
	sc->sc_ospeed = ospeed;
	sc->sc_cflag = cflag;
}

static int
exynos_uart_open(dev_t dev, int flag, int mode, lwp_t *l)
{
	struct exynos_uart_softc *sc =
	    device_lookup_private(&exuart_cd, minor(dev));
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
		tp->t_lflag = TTYDEF_LFLAG;
		if (sc->sc_console) {
			tp->t_ispeed = tp->t_ospeed = exynos_uart_cnsc.sc_ospeed;
			tp->t_cflag = exynos_uart_cnsc.sc_cflag;
		} else {
			tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
			tp->t_cflag = TTYDEF_CFLAG;
		}
		ttsetwater(tp);
	}
	tp->t_state |= TS_CARR_ON;

	/* Enable RX and error interrupts */
	WR4(sc, SSCOM_UINTM, ~0u & ~(UINT_RXD|UINT_ERROR));

	return tp->t_linesw->l_open(dev, tp);
}

static int
exynos_uart_close(dev_t dev, int flag, int mode, lwp_t *l)
{
	struct exynos_uart_softc *sc =
	    device_lookup_private(&exuart_cd, minor(dev));
	struct tty *tp = sc->sc_tty;

	tp->t_linesw->l_close(tp, flag);
	ttyclose(tp);

	/* Disable interrupts */
	WR4(sc, SSCOM_UINTM, ~0u);

	return 0;
}

static int
exynos_uart_read(dev_t dev, struct uio *uio, int flag)
{
	struct exynos_uart_softc *sc =
	    device_lookup_private(&exuart_cd, minor(dev));
	struct tty *tp = sc->sc_tty;

	return tp->t_linesw->l_read(tp, uio, flag);
}

static int
exynos_uart_write(dev_t dev, struct uio *uio, int flag)
{
	struct exynos_uart_softc *sc =
	    device_lookup_private(&exuart_cd, minor(dev));
	struct tty *tp = sc->sc_tty;

	return tp->t_linesw->l_write(tp, uio, flag);
}

static int
exynos_uart_poll(dev_t dev, int events, lwp_t *l)
{
	struct exynos_uart_softc *sc =
	    device_lookup_private(&exuart_cd, minor(dev));
	struct tty *tp = sc->sc_tty;

	return tp->t_linesw->l_poll(tp, events, l);
}

static int
exynos_uart_ioctl(dev_t dev, u_long cmd, void *data, int flag, lwp_t *l)
{
	struct exynos_uart_softc *sc =
	    device_lookup_private(&exuart_cd, minor(dev));
	struct tty *tp = sc->sc_tty;
	int error;

	error = tp->t_linesw->l_ioctl(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	return ttioctl(tp, cmd, data, flag, l);
}

static struct tty *
exynos_uart_tty(dev_t dev)
{
	struct exynos_uart_softc *sc =
	    device_lookup_private(&exuart_cd, minor(dev));

	return sc->sc_tty;
}

static void
exynos_uart_stop(struct tty *tp, int flag)
{
}

static void
exynos_uart_start(struct tty *tp)
{
	struct exynos_uart_softc *sc = tp->t_sc;
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
		while ((RD4(sc, SSCOM_UFSTAT) & UFSTAT_TXFULL) != 0)
			;

		bus_space_write_1(sc->sc_bst, sc->sc_bsh,
		    SSCOM_UTXH, *p);
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
exynos_uart_param(struct tty *tp, struct termios *t)
{
	struct exynos_uart_softc *sc = tp->t_sc;

	if (tp->t_ospeed == t->c_ospeed &&
	    tp->t_cflag == t->c_cflag)
		return 0;

	uint32_t ulcon = 0, ubrdiv;
	switch (ISSET(t->c_cflag, CSIZE)) {
	case CS5:
		ulcon |= ULCON_LENGTH_5;
		break;
	case CS6:
		ulcon |= ULCON_LENGTH_6;
		break;
	case CS7:
		ulcon |= ULCON_LENGTH_7;
		break;
	case CS8:
		ulcon |= ULCON_LENGTH_8;
		break;
	}
	switch (ISSET(t->c_cflag, PARENB|PARODD)) {
	case PARENB|PARODD:
		ulcon |= ULCON_PARITY_ODD;
		break;
	case PARENB:
		ulcon |= ULCON_PARITY_EVEN;
		break;
	default:
		ulcon |= ULCON_PARITY_NONE;
		break;
	}
	if (ISSET(t->c_cflag, CSTOPB))
		ulcon |= ULCON_STOP;
	WR4(sc, SSCOM_ULCON, ulcon);

	ubrdiv = (sc->sc_freq / 16) / t->c_ospeed - 1;
	WR4(sc, SSCOM_UBRDIV, ubrdiv);

	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;

	return 0;
}

static int
exynos_uart_intr(void *priv)
{
	struct exynos_uart_softc *sc = priv;
	struct tty *tp = sc->sc_tty;
	uint32_t uintp, uerstat, ufstat, c;

	uintp = RD4(sc, SSCOM_UINTP);

	for (;;) {
		int cn_trapped = 0;

		uerstat = RD4(sc, SSCOM_UERSTAT);
		if (uerstat & UERSTAT_BREAK) {
			cn_check_magic(tp->t_dev, CNC_BREAK,
			    exynos_uart_cnm_state);
			if (cn_trapped)
				continue;
		}

		ufstat = RD4(sc, SSCOM_UFSTAT);
		if (__SHIFTOUT(ufstat, UFSTAT_RXCOUNT) == 0) {
			break;
		}

		c = bus_space_read_1(sc->sc_bst, sc->sc_bsh, SSCOM_URXH);
		cn_check_magic(tp->t_dev, c & 0xff, exynos_uart_cnm_state);
		if (cn_trapped)
			continue;
		tp->t_linesw->l_rint(c & 0xff, tp);
	}

	WR4(sc, SSCOM_UINTP, uintp);

	return 1;
}

/*
 * Console support
 */

static int
exynos_uart_console_match(int phandle)
{
	return of_match_compatible(phandle, compatible);
}

static void
exynos_uart_console_consinit(struct fdt_attach_args *faa, u_int uart_freq)
{
	const int phandle = faa->faa_phandle;
	bus_space_tag_t bst = faa->faa_bst;
	bus_space_handle_t bsh;
	bus_addr_t addr;
	bus_size_t size;
	tcflag_t flags;
	int speed;

	speed = fdtbus_get_stdout_speed();
	if (speed < 0)
		speed = 115200; /* default */
	flags = fdtbus_get_stdout_flags();

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0)
		panic("exynos_uart: couldn't get registers");
	if (bus_space_map(bst, addr, size, 0, &bsh) != 0)
		panic("exynos_uart: couldn't map registers");

	exynos_uart_consaddr = addr;

	exynos_uart_cnattach(bst, bsh, speed, flags);
}

static const struct fdt_console exynos_uart_console = {
	.match = exynos_uart_console_match,
	.consinit = exynos_uart_console_consinit,
};

FDT_CONSOLE(exynos_uart, &exynos_uart_console);
