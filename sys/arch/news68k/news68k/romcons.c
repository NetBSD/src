/*	$NetBSD: romcons.c,v 1.2.2.1 2014/08/10 06:54:03 tls Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * romcons.c - from sys/dev/ofw/ofcons.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: romcons.c,v 1.2.2.1 2014/08/10 06:54:03 tls Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/tty.h>
#include <sys/kauth.h>

#include <dev/cons.h>

#include <machine/autoconf.h>
#include <machine/romcall.h>

#include "ioconf.h"

struct romcons_softc {
	device_t sc_dev;
	struct tty *sc_tty;
	struct callout sc_poll_ch;
	int sc_flags;
#define	CONS_POLL	1
};

#define	BURSTLEN	128	/* max number of bytes to write in one chunk */

cons_decl(romcons_);

static int romcons_match(device_t, cfdata_t, void *);
static void romcons_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(romcons, sizeof(struct romcons_softc),
    romcons_match, romcons_attach, NULL, NULL);

dev_type_open(romcons_open);
dev_type_close(romcons_close);
dev_type_read(romcons_read);
dev_type_write(romcons_write);
dev_type_ioctl(romcons_ioctl);
dev_type_tty(romcons_tty);
dev_type_poll(romcons_poll);

void romcons_kbdinput(int);

const struct cdevsw romcons_cdevsw = {
	.d_open = romcons_open,
	.d_close = romcons_close,
	.d_read = romcons_read,
	.d_write = romcons_write,
	.d_ioctl = romcons_ioctl,
	.d_stop = nostop,
	.d_tty = romcons_tty,
	.d_poll = romcons_poll,
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};

struct consdev consdev_rom = cons_init(romcons_);

bool romcons_is_console;

static int
romcons_match(device_t parent, cfdata_t match, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	static bool romcons_matched;

	if (strcmp(ma->ma_name, "romcons"))
		return 0;

	if (!romcons_is_console)
		return 0;

	if (romcons_matched)
		return 0;

	romcons_matched = true;
	return 1;
}

static void
romcons_attach(device_t parent, device_t self, void *aux)
{
	struct romcons_softc *sc = device_private(self);

	sc->sc_dev = self;
	vectab[46] = romcallvec;	/* XXX */
	aprint_normal("\n");

	callout_init(&sc->sc_poll_ch, 0);
}

static void romcons_start(struct tty *);
static int romcons_param(struct tty *, struct termios *);
static void romcons_pollin(void *);

int
romcons_open(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct romcons_softc *sc;
	struct tty *tp;

	sc = device_lookup_private(&romcons_cd, minor(dev));
	if (sc == NULL)
		return ENXIO;
	if ((tp = sc->sc_tty) == 0)
		sc->sc_tty = tp = tty_alloc();
	tp->t_oproc = romcons_start;
	tp->t_param = romcons_param;
	tp->t_dev = dev;
	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp))
		return EBUSY;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		romcons_param(tp, &tp->t_termios);
		ttsetwater(tp);
	}
	tp->t_state |= TS_CARR_ON;

	if ((sc->sc_flags & CONS_POLL) == 0) {
		sc->sc_flags |= CONS_POLL;
		callout_reset(&sc->sc_poll_ch, 1, romcons_pollin, sc);
	}

	return (*tp->t_linesw->l_open)(dev, tp);
}

int
romcons_close(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct romcons_softc *sc;
	struct tty *tp;

	sc = device_lookup_private(&romcons_cd, minor(dev));
	tp = sc->sc_tty;
	callout_stop(&sc->sc_poll_ch);
	sc->sc_flags &= ~CONS_POLL;
	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);
	return 0;
}

int
romcons_read(dev_t dev, struct uio *uio, int flag)
{
	struct romcons_softc *sc;
	struct tty *tp;

	sc = device_lookup_private(&romcons_cd, minor(dev));
	tp = sc->sc_tty;

	return (*tp->t_linesw->l_read)(tp, uio, flag);
}

int
romcons_write(dev_t dev, struct uio *uio, int flag)
{
	struct romcons_softc *sc;
	struct tty *tp;

	sc = device_lookup_private(&romcons_cd, minor(dev));
	tp = sc->sc_tty;
	return (*tp->t_linesw->l_write)(tp, uio, flag);
}

int
romcons_poll(dev_t dev, int events, struct lwp *l)
{
	struct romcons_softc *sc;
	struct tty *tp;

	sc = device_lookup_private(&romcons_cd, minor(dev));
	tp = sc->sc_tty;
	return (*tp->t_linesw->l_poll)(tp, events, l);
}
int
romcons_ioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct romcons_softc *sc;
	struct tty *tp;
	int error;

	sc = device_lookup_private(&romcons_cd, minor(dev));
	tp = sc->sc_tty;
	if ((error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l)) !=
	    EPASSTHROUGH)
		return error;
	return ttioctl(tp, cmd, data, flag, l);
}

struct tty *
romcons_tty(dev_t dev)
{
	struct romcons_softc *sc;

	sc = device_lookup_private(&romcons_cd, minor(dev));
	return sc->sc_tty;
}

static void
romcons_start(struct tty *tp)
{
	int s, len;
	uint8_t buf[BURSTLEN];

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP)) {
		splx(s);
		return;
	}
	tp->t_state |= TS_BUSY;
	splx(s);
	len = q_to_b(&tp->t_outq, buf, BURSTLEN);
	s = splhigh();
	rom_write(1, buf, len);
	splx(s);
	s = spltty();
	tp->t_state &= ~TS_BUSY;
	if (ttypull(tp)) {
		tp->t_state |= TS_TIMEOUT;
		callout_schedule(&tp->t_rstrt_ch, 1);
	}
	splx(s);
}

static int
romcons_param(struct tty *tp, struct termios *t)
{

	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;
	return 0;
}

static void
romcons_pollin(void *aux)
{
	struct romcons_softc *sc = aux;
	struct tty *tp = sc->sc_tty;
	char ch;
	int rv;

	while (0 && (rv = rom_read(1, &ch, 1)) > 0) {
		if (tp && (tp->t_state & TS_ISOPEN))
			(*tp->t_linesw->l_rint)(ch, tp);
	}
	callout_reset(&sc->sc_poll_ch, 1, romcons_pollin, sc);
}

void
romcons_kbdinput(int ks)
{
	struct romcons_softc *sc;
	struct tty *tp;

	sc = device_lookup_private(&romcons_cd, 0);
	tp = sc->sc_tty;
	if (tp && (tp->t_state & TS_ISOPEN))
		(*tp->t_linesw->l_rint)(ks, tp);
}

void
romcons_cnprobe(struct consdev *cd)
{
}

void
romcons_cninit(struct consdev *cd)
{
	int maj;

	maj = cdevsw_lookup_major(&romcons_cdevsw);
	cd->cn_dev = makedev(maj, 0);
	romcons_is_console = true;
	vectab[46] = romcallvec;	/* XXX */
}

int
romcons_cngetc(dev_t dev)
{
	unsigned char ch = '\0';
	int l;

	while ((l = rom_read(1, &ch, 1)) != 1)
		if (l != -2 && l != 0)
			return -1;
	return ch;
}

void
romcons_cnputc(dev_t dev, int c)
{
	char ch = c;

	rom_write(1, &ch, 1);
}

void
romcons_cnpollc(dev_t dev, int on)
{
	struct romcons_softc *sc;

	sc = device_lookup_private(&romcons_cd, minor(dev));

	if (sc == NULL)
		return;
	if (on) {
		if (sc->sc_flags & CONS_POLL)
			callout_stop(&sc->sc_poll_ch);
		sc->sc_flags &= ~CONS_POLL;
	} else {
		if ((sc->sc_flags & CONS_POLL) == 0) {
			sc->sc_flags |= CONS_POLL;
			callout_reset(&sc->sc_poll_ch, 1, romcons_pollin, sc);
		}
	}
}
