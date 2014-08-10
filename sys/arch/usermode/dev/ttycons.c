/* $NetBSD: ttycons.c,v 1.19.12.1 2014/08/10 06:54:09 tls Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: ttycons.c,v 1.19.12.1 2014/08/10 06:54:09 tls Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kauth.h>
#include <sys/termios.h>
#include <sys/tty.h>

#include <dev/cons.h>

#include <machine/mainbus.h>
#include <machine/thunk.h>

static int	ttycons_match(device_t, cfdata_t, void *);
static void	ttycons_attach(device_t, device_t, void *);

void		ttycons_consinit(void);

struct ttycons_softc {
	device_t	sc_dev;
	struct tty	*sc_tty;
	void		*sc_rd_sih;
	void		*sc_ctrlc_sih;
	void		*sc_ctrlz_sih;
	u_char		sc_buf[1024];
};

dev_type_cngetc(ttycons_cngetc);
dev_type_cnputc(ttycons_cnputc);
dev_type_cnpollc(ttycons_cnpollc);

static struct cnm_state ttycons_cnm_state;
struct consdev ttycons_consdev = {
	.cn_getc = ttycons_cngetc,
	.cn_putc = ttycons_cnputc,
	.cn_pollc = ttycons_cnpollc,
	.cn_dev = NODEV,
	.cn_pri = CN_NORMAL,
};

CFATTACH_DECL_NEW(ttycons, sizeof(struct ttycons_softc),
    ttycons_match, ttycons_attach, NULL, NULL);

extern struct cfdriver ttycons_cd;

dev_type_open(ttycons_open);
dev_type_close(ttycons_close);
dev_type_read(ttycons_read);
dev_type_write(ttycons_write);
dev_type_ioctl(ttycons_ioctl);
dev_type_stop(ttycons_stop);
dev_type_tty(ttycons_tty);
dev_type_poll(ttycons_poll);

const struct cdevsw ttycons_cdevsw = {
	.d_open = ttycons_open,
	.d_close = ttycons_close,
	.d_read = ttycons_read,
	.d_write = ttycons_write,
	.d_ioctl = ttycons_ioctl,
	.d_stop = ttycons_stop,
	.d_tty = ttycons_tty,
	.d_poll = ttycons_poll,
	.d_kqfilter = ttykqfilter,
	.d_flag = D_TTY,
	.d_discard = nodiscard,
};

static void	ttycons_start(struct tty *);
static int	ttycons_param(struct tty *, struct termios *);

static int	ttycons_intr(void *);
static void	ttycons_softintr(void *);

static sigfunc_t  ttycons_ctrlc;
static void	ttycons_softctrlc(void *);
static sigfunc_t  ttycons_ctrlz;
static void	ttycons_softctrlz(void *);

static int
ttycons_match(device_t parent, cfdata_t match, void *opaque)
{
	struct thunkbus_attach_args *taa = opaque;

	if (taa->taa_type != THUNKBUS_TYPE_TTYCONS)
		return 0;

	return 1;
}

static void
ttycons_attach(device_t parent, device_t self, void *opaque)
{
	struct ttycons_softc *sc = device_private(self);
	int maj;

	aprint_naive("\n");
	aprint_normal(": console\n");

	sc->sc_dev = self;
	sc->sc_tty = tty_alloc();
	tty_attach(sc->sc_tty);
	sc->sc_tty->t_oproc = ttycons_start;
	sc->sc_tty->t_param = ttycons_param;
	sc->sc_tty->t_sc = sc;

	maj = cdevsw_lookup_major(&ttycons_cdevsw);
	cn_tab->cn_dev = makedev(maj, device_unit(self));
	sc->sc_tty->t_dev = cn_tab->cn_dev;

	sc->sc_rd_sih = softint_establish(SOFTINT_SERIAL,
	    ttycons_softintr, sc);
	if (sc->sc_rd_sih == NULL)
		panic("couldn't establish ttycons intr handler\n");

	sc->sc_ctrlc_sih = softint_establish(SOFTINT_SERIAL,
	    ttycons_softctrlc, sc);
	if (sc->sc_ctrlc_sih == NULL)
		panic("couldn't establish ttycons ctrlc handler\n");
	sc->sc_ctrlz_sih = softint_establish(SOFTINT_SERIAL,
	    ttycons_softctrlz, sc);
	if (sc->sc_ctrlz_sih == NULL)
		panic("couldn't establish ttycons ctrlz handler\n");

	sigio_intr_establish(ttycons_intr, sc);
	signal_intr_establish(SIGINT,  ttycons_ctrlc);
	signal_intr_establish(SIGTSTP, ttycons_ctrlz);

	if (thunk_set_stdin_sigio(true) != 0)
		panic("couldn't enable stdin async mode");
}

void
ttycons_consinit(void)
{
	struct thunk_termios t;

	thunk_tcgetattr(0, &t);
	t.c_lflag &= ~(ECHO|ICANON);
	t.c_cc[VTIME] = 0;
	t.c_cc[VMIN] = 1;
	thunk_tcsetattr(0, TCSANOW, &t);

	cn_tab = &ttycons_consdev;
	cn_init_magic(&ttycons_cnm_state);
	cn_set_magic("\047\001");
}

int
ttycons_cngetc(dev_t dev)
{
	return thunk_getchar();
}

void
ttycons_cnputc(dev_t dev, int c)
{
	thunk_putchar(c);
}

void
ttycons_cnpollc(dev_t dev, int on)
{
}

int
ttycons_open(dev_t dev, int flag, int mode, lwp_t *l)
{
	struct ttycons_softc *sc;
	struct tty *t;

	sc = device_lookup_private(&ttycons_cd, minor(dev));
	if (sc == NULL)
		return ENXIO;
	t = sc->sc_tty;

	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, t))
		return EBUSY;

	if ((t->t_state & TS_ISOPEN) == 0 && t->t_wopen == 0) {
		t->t_dev = dev;
		ttychars(t);
		t->t_iflag = TTYDEF_IFLAG;
		t->t_oflag = TTYDEF_OFLAG;
		t->t_cflag = TTYDEF_CFLAG;
		t->t_lflag = TTYDEF_LFLAG;
		t->t_ispeed = t->t_ospeed = TTYDEF_SPEED;
		ttycons_param(t, &t->t_termios);
		ttsetwater(t);
	}
	t->t_state |= TS_CARR_ON;

	return t->t_linesw->l_open(dev, t);
}

int
ttycons_close(dev_t dev, int flag, int mode, lwp_t *l)
{
	struct ttycons_softc *sc;
	struct tty *t;

	sc = device_lookup_private(&ttycons_cd, minor(dev));
	t = sc->sc_tty;

	if (t == NULL)
		return 0;

	t->t_linesw->l_close(t, flag);
	ttyclose(t);

	return 0;
}

int
ttycons_read(dev_t dev, struct uio *uio, int flag)
{
	struct ttycons_softc *sc;
	struct tty *t;

	sc = device_lookup_private(&ttycons_cd, minor(dev));
	t = sc->sc_tty;

	return t->t_linesw->l_read(t, uio, flag);
}

int
ttycons_write(dev_t dev, struct uio *uio, int flag)
{
	struct ttycons_softc *sc;
	struct tty *t;

	sc = device_lookup_private(&ttycons_cd, minor(dev));
	t = sc->sc_tty;

	return t->t_linesw->l_write(t, uio, flag);
}

int
ttycons_poll(dev_t dev, int events, lwp_t *l)
{
	struct ttycons_softc *sc;
	struct tty *t;

	sc = device_lookup_private(&ttycons_cd, minor(dev));
	t = sc->sc_tty;

	return t->t_linesw->l_poll(t, events, l);
}

struct tty *
ttycons_tty(dev_t dev)
{
	struct ttycons_softc *sc;

	sc = device_lookup_private(&ttycons_cd, minor(dev));

	return sc->sc_tty;
}

int
ttycons_ioctl(dev_t dev, u_long cmd, void *data, int flag, lwp_t *l)
{
	struct ttycons_softc *sc;
	struct tty *t;
	int error;

	sc = device_lookup_private(&ttycons_cd, minor(dev));
	t = sc->sc_tty;

	error = t->t_linesw->l_ioctl(t, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	error = ttioctl(t, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	return EPASSTHROUGH;
}

static void
ttycons_start(struct tty *t)
{
	struct ttycons_softc *sc = t->t_sc;
	u_char *p = sc->sc_buf;
	int s, len, brem;

	s = spltty();
	if (t->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP)) {
		splx(s);
		return;
	}
	t->t_state |= TS_BUSY;
	splx(s);

	brem = q_to_b(&t->t_outq, sc->sc_buf, sizeof(sc->sc_buf));

	while (brem > 0) {
		len = thunk_write(1, p, brem);
		if (len > 0) {
			p += len;
			brem -= len;
		}
	}

	s = spltty();
	t->t_state &= ~TS_BUSY;
	if (ttypull(t)) {
		t->t_state |= TS_TIMEOUT;
		callout_schedule(&t->t_rstrt_ch, 1);
	}
	splx(s);
}

void
ttycons_stop(struct tty *t, int flag)
{
}

static int
ttycons_param(struct tty *t, struct termios *c)
{
	t->t_ispeed = c->c_ispeed;
	t->t_ospeed = c->c_ospeed;
	t->t_cflag = c->c_cflag;
	return 0;
}

static int
ttycons_intr(void *priv)
{
	struct ttycons_softc *sc = priv;

	softint_schedule(sc->sc_rd_sih);

	return 0;
}

static void
ttycons_softintr(void *priv)
{
	struct ttycons_softc *sc = priv;
	struct tty *t = sc->sc_tty;
	unsigned char ch;
	int c;

	while ((c = thunk_pollchar()) >= 0) {
		ch = (unsigned char)c;
		cn_check_magic(t->t_dev, ch, ttycons_cnm_state);
		t->t_linesw->l_rint(ch, t);
	}
}


/*
 * handle SIGINT signal from trap.c
 *
 * argument 'pc' and 'va' are not used.
 */
static void
ttycons_ctrlc(siginfo_t *info, vaddr_t from_userland, vaddr_t pc, vaddr_t va)
{
	struct ttycons_softc *sc;

	sc = device_lookup_private(&ttycons_cd, minor(cn_tab->cn_dev));
	if (sc)
		softint_schedule(sc->sc_ctrlc_sih);

}

static void
ttycons_softctrlc(void *priv)
{
	struct ttycons_softc *sc = priv;
	struct tty *t = sc->sc_tty;
	unsigned char ch = 3;	/* ETX */

	cn_check_magic(t->t_dev, ch, ttycons_cnm_state);
	t->t_linesw->l_rint(ch, t);
}

/*
 * handle SIGTSTP signal from trap.c
 *
 * argument 'pc' and 'va' are not used.
 */
static void
ttycons_ctrlz(siginfo_t *info, vaddr_t from_userland, vaddr_t pc, vaddr_t va)
{
	struct ttycons_softc *sc;

	sc = device_lookup_private(&ttycons_cd, minor(cn_tab->cn_dev));
	if (sc)
		softint_schedule(sc->sc_ctrlz_sih);
}

static void
ttycons_softctrlz(void *priv)
{
	struct ttycons_softc *sc = priv;
	struct tty *t = sc->sc_tty;
	unsigned char ch = 26;	/* SUB */

	cn_check_magic(t->t_dev, ch, ttycons_cnm_state);
	t->t_linesw->l_rint(ch, t);
}
