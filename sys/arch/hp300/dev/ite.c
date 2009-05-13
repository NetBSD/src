/*	$NetBSD: ite.c,v 1.82.10.1 2009/05/13 17:17:42 jym Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: ite.c 1.28 92/12/20$
 *
 *	@(#)ite.c	8.2 (Berkeley) 1/12/94
 */
/*
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: ite.c 1.28 92/12/20$
 *
 *	@(#)ite.c	8.2 (Berkeley) 1/12/94
 */

/*
 * Bit-mapped display terminal emulator machine independent code.
 * This is a very rudimentary.  Much more can be abstracted out of
 * the hardware dependent routines.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ite.c,v 1.82.10.1 2009/05/13 17:17:42 jym Exp $");

#include "hil.h"

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/kauth.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <dev/cons.h>

#include <hp300/dev/grfioctl.h>
#include <hp300/dev/grfvar.h>
#include <hp300/dev/hilioctl.h>
#include <hp300/dev/hilvar.h>
#include <hp300/dev/itevar.h>
#include <hp300/dev/kbdmap.h>

#include "ioconf.h"

#define set_attr(ip, attr)	((ip)->attribute |= (attr))
#define clr_attr(ip, attr)	((ip)->attribute &= ~(attr))

/*
 * # of chars are output in a single itestart() call.
 * If this is too big, user processes will be blocked out for
 * long periods of time while we are emptying the queue in itestart().
 * If it is too small, console output will be very ragged.
 */
int	iteburst = 64;

static int	itematch(device_t, cfdata_t, void *);
static void	iteattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ite, sizeof(struct ite_softc),
    itematch, iteattach, NULL, NULL);

/* XXX this has always been global, but shouldn't be */
static struct kbdmap *ite_km;

static dev_type_open(iteopen);
static dev_type_close(iteclose);
static dev_type_read(iteread);
static dev_type_write(itewrite);
static dev_type_ioctl(iteioctl);
static dev_type_tty(itetty);
static dev_type_poll(itepoll);

const struct cdevsw ite_cdevsw = {
	iteopen, iteclose, iteread, itewrite, iteioctl,
	nostop, itetty, itepoll, nommap, ttykqfilter, D_TTY
};

/*
 * Terminal emulator state information, statically allocated
 * for the benefit of the console.
 */
static struct	ite_data ite_cn;

/*
 * console stuff
 */
static struct consdev ite_cons = {
	NULL,
	NULL,
	itecngetc,
	itecnputc,
	nullcnpollc,
	NULL,
	NULL,
	NULL,
	NODEV,
	CN_NORMAL
};
static int console_kbd_attached;
static int console_display_attached;
static struct ite_kbdops *console_kbdops;
static struct ite_kbdmap *console_kbdmap;

static void	iteinit(struct ite_data *);
static void	iteputchar(int, struct ite_data *);
static void	itecheckwrap(struct ite_data *, struct itesw *);
static void	ite_dchar(struct ite_data *, struct itesw *);
static void	ite_ichar(struct ite_data *, struct itesw *);
static void	ite_dline(struct ite_data *, struct itesw *);
static void	ite_iline(struct ite_data *, struct itesw *);
static void	ite_clrtoeol(struct ite_data *, struct itesw *, int, int);
static void	ite_clrtoeos(struct ite_data *, struct itesw *);
static void	itestart(struct tty *);

/*
 * Primary attribute buffer to be used by the first bitmapped console
 * found. Secondary displays alloc the attribute buffer as needed.
 * Size is based on a 68x128 display, which is currently our largest.
 */
static u_char  ite_console_attributes[0x2200];

#define ite_erasecursor(ip, sp)	{ \
	if ((ip)->flags & ITE_CURSORON) \
		(*(sp)->ite_cursor)((ip), ERASE_CURSOR); \
}
#define ite_drawcursor(ip, sp) { \
	if ((ip)->flags & ITE_CURSORON) \
		(*(sp)->ite_cursor)((ip), DRAW_CURSOR); \
}
#define ite_movecursor(ip, sp) { \
	if ((ip)->flags & ITE_CURSORON) \
		(*(sp)->ite_cursor)((ip), MOVE_CURSOR); \
}

static int
itematch(device_t parent, cfdata_t cf, void *aux)
{

	return 1;
}

static void
iteattach(device_t parent, device_t self, void *aux)
{
	struct ite_softc *ite = device_private(self);
	struct grf_softc *grf = device_private(parent);
	struct grfdev_attach_args *ga = aux;

	ite->sc_dev = self;

	/* Allocate the ite_data. */
	if (ga->ga_isconsole) {
		ite->sc_data = &ite_cn;
		aprint_normal(": console");

		/*
		 * We didn't know which unit this would be during
		 * the console probe, so we have to fixup cn_dev here.
		 */
		cn_tab->cn_dev = makedev(cdevsw_lookup_major(&ite_cdevsw),
		    device_unit(self));
	} else {
		ite->sc_data = malloc(sizeof(struct ite_data), M_DEVBUF,
		    M_NOWAIT | M_ZERO);
		if (ite->sc_data == NULL) {
			aprint_normal("\n");
			aprint_error_dev(self, "malloc for ite_data failed\n");
			return;
		}
		ite->sc_data->flags = ITE_ALIVE;
	}

	/*
	 * Cross-reference the ite and the grf.
	 */
	ite->sc_grf = grf;
	grf->sc_ite = ite;

	aprint_normal("\n");
}

void
iteinstallkeymap(void *v)
{

	ite_km = (struct kbdmap *)v;
}

/*
 * Perform functions necessary to setup device as a terminal emulator.
 */
int
iteon(struct ite_data *ip, int flag)
{

	if ((ip->flags & ITE_ALIVE) == 0)
		return ENXIO;

	/* force ite active, overriding graphics mode */
	if (flag & 1) {
		ip->flags |= ITE_ACTIVE;
		ip->flags &= ~(ITE_INGRF|ITE_INITED);
	}

	/* leave graphics mode */
	if (flag & 2) {
		ip->flags &= ~ITE_INGRF;
		if ((ip->flags & ITE_ACTIVE) == 0)
			return 0;
	}

	ip->flags |= ITE_ACTIVE;
	if (ip->flags & ITE_INGRF)
		return 0;

	if (console_kbdops != NULL)
		(*console_kbdops->enable)(console_kbdops->arg);

	iteinit(ip);
	return 0;
}

static void
iteinit(struct ite_data *ip)
{

	if (ip->flags & ITE_INITED)
		return;

	ip->curx = 0;
	ip->cury = 0;
	ip->cursorx = 0;
	ip->cursory = 0;

	(*ip->isw->ite_init)(ip);
	ip->flags |= ITE_CURSORON;
	ite_drawcursor(ip, ip->isw);

	ip->attribute = 0;
	if (ip->attrbuf == NULL)
		ip->attrbuf = malloc(ip->rows * ip->cols,
		    M_DEVBUF, M_WAITOK | M_ZERO);

	ip->imode = 0;
	ip->flags |= ITE_INITED;
}

/*
 * "Shut down" device as terminal emulator.
 * Note that we do not deinit the console device unless forced.
 * Deinit'ing the console every time leads to a very active
 * screen when processing /etc/rc.
 */
void
iteoff(struct ite_data *ip, int flag)
{

	if (flag & 2) {
		ip->flags |= ITE_INGRF;
		ip->flags &= ~ITE_CURSORON;
	}
	if ((ip->flags & ITE_ACTIVE) == 0)
		return;
	if ((flag & 1) ||
	    (ip->flags & (ITE_INGRF|ITE_ISCONS|ITE_INITED)) == ITE_INITED)
		(*ip->isw->ite_deinit)(ip);

	/*
	 * XXX When the system is rebooted with "reboot", init(8)
	 * kills the last process to have the console open.
	 * If we don't prevent the ITE_ACTIVE bit from being
	 * cleared, we will never see messages printed during
	 * the process of rebooting.
	 */
	if ((flag & 2) == 0 && (ip->flags & ITE_ISCONS) == 0)
		ip->flags &= ~ITE_ACTIVE;
}

/* ARGSUSED */
static int
iteopen(dev_t dev, int mode, int devtype, struct lwp *l)
{
	int unit = ITEUNIT(dev);
	struct tty *tp;
	struct ite_softc *sc;
	struct ite_data *ip;
	int error;
	int first = 0;

	sc = device_lookup_private(&ite_cd, unit);
	if (sc == NULL)
		return ENXIO;
	ip = sc->sc_data;

	if (ip->tty == NULL) {
	 	tp = ip->tty = ttymalloc();
		tty_attach(tp);
	} else
		tp = ip->tty;
	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp))
		return (EBUSY);
	if ((ip->flags & ITE_ACTIVE) == 0) {
		error = iteon(ip, 0);
		if (error)
			return error;
		first = 1;
	}
	tp->t_oproc = itestart;
	tp->t_param = NULL;
	tp->t_dev = dev;
	if ((tp->t_state&TS_ISOPEN) == 0) {
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = CS8|CREAD;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		tp->t_state = TS_ISOPEN|TS_CARR_ON;
		ttsetwater(tp);
	}
	error = (*tp->t_linesw->l_open)(dev, tp);
	if (error == 0) {
		tp->t_winsize.ws_row = ip->rows;
		tp->t_winsize.ws_col = ip->cols;
	} else if (first)
		iteoff(ip, 0);
	return error;
}

/*ARGSUSED*/
static int
iteclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct ite_softc *sc = device_lookup_private(&ite_cd, ITEUNIT(dev));
	struct ite_data *ip = sc->sc_data;
	struct tty *tp = ip->tty;

	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);
	iteoff(ip, 0);
#if 0
	tty_detach(tp);
	ttyfree(tp);
	ip->tty = NULL;
#endif
	return 0;
}

static int
iteread(dev_t dev, struct uio *uio, int flag)
{
	struct ite_softc *sc = device_lookup_private(&ite_cd, ITEUNIT(dev));
	struct tty *tp = sc->sc_data->tty;

	return (*tp->t_linesw->l_read)(tp, uio, flag);
}

int
itewrite(dev_t dev, struct uio *uio, int flag)
{
	struct ite_softc *sc = device_lookup_private(&ite_cd, ITEUNIT(dev));
	struct tty *tp = sc->sc_data->tty;

	return (*tp->t_linesw->l_write)(tp, uio, flag);
}

int
itepoll(dev_t dev, int events, struct lwp *l)
{
	struct ite_softc *sc = device_lookup_private(&ite_cd, ITEUNIT(dev));
	struct tty *tp = sc->sc_data->tty;

	return (*tp->t_linesw->l_poll)(tp, events, l);
}

struct tty *
itetty(dev_t dev)
{
	struct ite_softc *sc = device_lookup_private(&ite_cd, ITEUNIT(dev));

	return sc->sc_data->tty;
}

int
iteioctl(dev_t dev, u_long cmd, void *addr, int flag, struct lwp *l)
{
	struct ite_softc *sc = device_lookup_private(&ite_cd, ITEUNIT(dev));
	struct ite_data *ip = sc->sc_data;
	struct tty *tp = ip->tty;
	int error;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, addr, flag, l);
	if (error != EPASSTHROUGH)
		return error;
	return ttioctl(tp, cmd, addr, flag, l);
}

static void
itestart(struct tty *tp)
{
	int cc, s;
	int hiwat = 0, hadcursor = 0;
	struct ite_softc *sc;
	struct ite_data *ip;

	sc = device_lookup_private(&ite_cd, ITEUNIT(tp->t_dev));
	ip = sc->sc_data;

	s = splkbd();
	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP)) {
		splx(s);
		return;
	}
	tp->t_state |= TS_BUSY;
	cc = tp->t_outq.c_cc;
	ttypull(tp);
	/*
	 * Handle common (?) case
	 */
	if (cc == 1) {
		iteputchar(getc(&tp->t_outq), ip);
	} else if (cc) {
		/*
		 * Limit the amount of output we do in one burst
		 * to prevent hogging the CPU.
		 */
		if (cc > iteburst) {
			hiwat++;
			cc = iteburst;
		}
		/*
		 * Turn off cursor while we output multiple characters.
		 * Saves a lot of expensive window move operations.
		 */
		if (ip->flags & ITE_CURSORON) {
			ite_erasecursor(ip, ip->isw);
			ip->flags &= ~ITE_CURSORON;
			hadcursor = 1;
		}
		while (--cc >= 0)
			iteputchar(getc(&tp->t_outq), ip);
		if (hadcursor) {
			ip->flags |= ITE_CURSORON;
			ite_drawcursor(ip, ip->isw);
		}
		if (hiwat) {
			tp->t_state |= TS_TIMEOUT;
			callout_schedule(&tp->t_rstrt_ch, 1);
		}
	}
	tp->t_state &= ~TS_BUSY;
	splx(s);
}

void
itefilter(char stat, char c)
{
	static int capsmode = 0;
	static int metamode = 0;
	char code;
	const char *str;
	struct tty *kbd_tty;

	if (ite_cn.tty == NULL)
		return;

	kbd_tty = ite_cn.tty;

	switch (c & 0xFF) {
	case KBD_CAPSLOCK:
		capsmode = !capsmode;
		return;

	case KBD_EXT_LEFT_DOWN:
	case KBD_EXT_RIGHT_DOWN:
		metamode = 1;
		return;

	case KBD_EXT_LEFT_UP:
	case KBD_EXT_RIGHT_UP:
		metamode = 0;
		return;
	}

	c &= KBD_CHARMASK;
	switch ((stat>>KBD_SSHIFT) & KBD_SMASK) {
	default:
	case KBD_KEY:
		code = ite_km->kbd_keymap[(int)c];
	        if (capsmode)
			code = toupper(code);
		break;

	case KBD_SHIFT:
		code = ite_km->kbd_shiftmap[(int)c];
	        if (capsmode)
			code = tolower(code);
		break;

	case KBD_CTRL:
		code = ite_km->kbd_ctrlmap[(int)c];
		break;

	case KBD_CTRLSHIFT:
		code = ite_km->kbd_ctrlshiftmap[(int)c];
		break;
	}

	if (code == '\0' && (str = ite_km->kbd_stringmap[(int)c]) != NULL) {
		while (*str)
			(*kbd_tty->t_linesw->l_rint)(*str++, kbd_tty);
	} else {
		if (metamode)
			code |= 0x80;
		(*kbd_tty->t_linesw->l_rint)(code, kbd_tty);
	}
}

static void
iteputchar(int c, struct ite_data *ip)
{
	struct itesw *sp = ip->isw;
	int n;

	if ((ip->flags & (ITE_ACTIVE|ITE_INGRF)) != ITE_ACTIVE)
	  	return;

	if (ip->escape) {
doesc:
		switch (ip->escape) {

		case '&':			/* Next can be a,d, or s */
			if (ip->fpd++) {
				ip->escape = c;
				ip->fpd = 0;
			}
			return;

		case 'a':				/* cursor change */
			switch (c) {

			case 'Y':			/* Only y coord. */
				ip->cury = min(ip->pos, ip->rows-1);
				ip->pos = 0;
				ip->escape = 0;
				ite_movecursor(ip, sp);
				clr_attr(ip, ATTR_INV);
				break;

			case 'y':			/* y coord first */
				ip->cury = min(ip->pos, ip->rows-1);
				ip->pos = 0;
				ip->fpd = 0;
				break;

			case 'C':			/* x coord */
				ip->curx = min(ip->pos, ip->cols-1);
				ip->pos = 0;
				ip->escape = 0;
				ite_movecursor(ip, sp);
				clr_attr(ip, ATTR_INV);
				break;

			default:	     /* Possibly a 3 digit number. */
				if (c >= '0' && c <= '9' && ip->fpd < 3) {
					ip->pos = ip->pos * 10 + (c - '0');
					ip->fpd++;
				} else {
					ip->pos = 0;
					ip->escape = 0;
				}
				break;
			}
			return;

		case 'd':				/* attribute change */
			switch (c) {

			case 'B':
				set_attr(ip, ATTR_INV);
				break;
		        case 'D':
				/* XXX: we don't do anything for underline */
				set_attr(ip, ATTR_UL);
				break;
		        case '@':
				clr_attr(ip, ATTR_ALL);
				break;
			}
			ip->escape = 0;
			return;

		case 's':				/* keypad control */
			switch (ip->fpd) {

			case 0:
				ip->hold = c;
				ip->fpd++;
				return;

			case 1:
				if (c == 'A') {
					switch (ip->hold) {

					case '0':
						clr_attr(ip, ATTR_KPAD);
						break;
					case '1':
						set_attr(ip, ATTR_KPAD);
						break;
					}
				}
				ip->hold = 0;
			}
			ip->escape = 0;
			return;

		case 'i':			/* back tab */
			if (ip->curx > TABSIZE) {
				n = ip->curx - (ip->curx & (TABSIZE - 1));
				ip->curx -= n;
			} else
				ip->curx = 0;
			ite_movecursor(ip, sp);
			ip->escape = 0;
			return;

		case '3':			/* clear all tabs */
			goto ignore;

		case 'K':			/* clear_eol */
			ite_clrtoeol(ip, sp, ip->cury, ip->curx);
			ip->escape = 0;
			return;

		case 'J':			/* clear_eos */
			ite_clrtoeos(ip, sp);
			ip->escape = 0;
			return;

		case 'B':			/* cursor down 1 line */
			if (++ip->cury == ip->rows) {
				--ip->cury;
				ite_erasecursor(ip, sp);
				(*sp->ite_scroll)(ip, 1, 0, 1, SCROLL_UP);
				ite_clrtoeol(ip, sp, ip->cury, 0);
			}
			else
				ite_movecursor(ip, sp);
			clr_attr(ip, ATTR_INV);
			ip->escape = 0;
			return;

		case 'C':			/* cursor forward 1 char */
			ip->escape = 0;
			itecheckwrap(ip, sp);
			return;

		case 'A':			/* cursor up 1 line */
			if (ip->cury > 0) {
				ip->cury--;
				ite_movecursor(ip, sp);
			}
			ip->escape = 0;
			clr_attr(ip, ATTR_INV);
			return;

		case 'P':			/* delete character */
			ite_dchar(ip, sp);
			ip->escape = 0;
			return;

		case 'M':			/* delete line */
			ite_dline(ip, sp);
			ip->escape = 0;
			return;

		case 'Q':			/* enter insert mode */
			ip->imode = 1;
			ip->escape = 0;
			return;

		case 'R':			/* exit insert mode */
			ip->imode = 0;
			ip->escape = 0;
			return;

		case 'L':			/* insert blank line */
			ite_iline(ip, sp);
			ip->escape = 0;
			return;

		case 'h':			/* home key */
			ip->cury = ip->curx = 0;
			ite_movecursor(ip, sp);
			ip->escape = 0;
			return;

		case 'D':			/* left arrow key */
			if (ip->curx > 0) {
				ip->curx--;
				ite_movecursor(ip, sp);
			}
			ip->escape = 0;
			return;

		case '1':			/* set tab in all rows */
			goto ignore;

		case ESC:
			if ((ip->escape = c) == ESC)
				break;
			ip->fpd = 0;
			goto doesc;

		default:
ignore:
			ip->escape = 0;
			return;

		}
	}

	switch (c &= 0x7F) {

	case '\n':

		if (++ip->cury == ip->rows) {
			--ip->cury;
			ite_erasecursor(ip, sp);
			(*sp->ite_scroll)(ip, 1, 0, 1, SCROLL_UP);
			ite_clrtoeol(ip, sp, ip->cury, 0);
		} else
			ite_movecursor(ip, sp);
		clr_attr(ip, ATTR_INV);
		break;

	case '\r':
		if (ip->curx) {
			ip->curx = 0;
			ite_movecursor(ip, sp);
		}
		break;

	case '\b':
		if (--ip->curx < 0)
			ip->curx = 0;
		else
			ite_movecursor(ip, sp);
		break;

	case '\t':
		if (ip->curx < TABEND(ip)) {
			n = TABSIZE - (ip->curx & (TABSIZE - 1));
			ip->curx += n;
			ite_movecursor(ip, sp);
		} else
			itecheckwrap(ip, sp);
		break;

	case CTRL('G'):
		if (console_kbdops != NULL)
			(*console_kbdops->bell)(console_kbdops->arg);
		break;

	case ESC:
		ip->escape = ESC;
		break;

	default:
		if (c < ' ' || c == DEL)
			break;
		if (ip->imode)
			ite_ichar(ip, sp);
		if ((ip->attribute & ATTR_INV) || attrtest(ip, ATTR_INV)) {
			attrset(ip, ATTR_INV);
			(*sp->ite_putc)(ip, c, ip->cury, ip->curx, ATTR_INV);
		} else
			(*sp->ite_putc)(ip, c, ip->cury, ip->curx, ATTR_NOR);
		ite_drawcursor(ip, sp);
		itecheckwrap(ip, sp);
		break;
	}
}

static void
itecheckwrap(struct ite_data *ip, struct itesw *sp)
{

	if (++ip->curx == ip->cols) {
		ip->curx = 0;
		clr_attr(ip, ATTR_INV);
		if (++ip->cury == ip->rows) {
			--ip->cury;
			ite_erasecursor(ip, sp);
			(*sp->ite_scroll)(ip, 1, 0, 1, SCROLL_UP);
			ite_clrtoeol(ip, sp, ip->cury, 0);
			return;
		}
	}
	ite_movecursor(ip, sp);
}

static void
ite_dchar(struct ite_data *ip, struct itesw *sp)
{

	if (ip->curx < ip->cols - 1) {
		ite_erasecursor(ip, sp);
		(*sp->ite_scroll)(ip, ip->cury, ip->curx + 1, 1, SCROLL_LEFT);
		attrmov(ip, ip->cury, ip->curx + 1, ip->cury, ip->curx,
		    1, ip->cols - ip->curx - 1);
	}
	attrclr(ip, ip->cury, ip->cols - 1, 1, 1);
	(*sp->ite_putc)(ip, ' ', ip->cury, ip->cols - 1, ATTR_NOR);
	ite_drawcursor(ip, sp);
}

static void
ite_ichar(struct ite_data *ip, struct itesw *sp)
{

	if (ip->curx < ip->cols - 1) {
		ite_erasecursor(ip, sp);
		(*sp->ite_scroll)(ip, ip->cury, ip->curx, 1, SCROLL_RIGHT);
		attrmov(ip, ip->cury, ip->curx, ip->cury, ip->curx + 1,
		    1, ip->cols - ip->curx - 1);
	}
	attrclr(ip, ip->cury, ip->curx, 1, 1);
	(*sp->ite_putc)(ip, ' ', ip->cury, ip->curx, ATTR_NOR);
	ite_drawcursor(ip, sp);
}

static void
ite_dline(struct ite_data *ip, struct itesw *sp)
{

	if (ip->cury < ip->rows - 1) {
		ite_erasecursor(ip, sp);
		(*sp->ite_scroll)(ip, ip->cury + 1, 0, 1, SCROLL_UP);
		attrmov(ip, ip->cury + 1, 0, ip->cury, 0,
		    ip->rows - ip->cury - 1, ip->cols);
	}
	ite_clrtoeol(ip, sp, ip->rows - 1, 0);
}

static void
ite_iline(struct ite_data *ip, struct itesw *sp)
{

	if (ip->cury < ip->rows - 1) {
		ite_erasecursor(ip, sp);
		(*sp->ite_scroll)(ip, ip->cury, 0, 1, SCROLL_DOWN);
		attrmov(ip, ip->cury, 0, ip->cury + 1, 0,
		    ip->rows - ip->cury - 1, ip->cols);
	}
	ite_clrtoeol(ip, sp, ip->cury, 0);
}

static void
ite_clrtoeol(struct ite_data *ip, struct itesw *sp, int y, int x)
{

	(*sp->ite_clear)(ip, y, x, 1, ip->cols - x);
	attrclr(ip, y, x, 1, ip->cols - x);
	ite_drawcursor(ip, sp);
}

void
ite_clrtoeos(struct ite_data *ip, struct itesw *sp)
{

	(*sp->ite_clear)(ip, ip->cury, 0, ip->rows - ip->cury, ip->cols);
	attrclr(ip, ip->cury, 0, ip->rows - ip->cury, ip->cols);
	ite_drawcursor(ip, sp);
}



/*
 * Console functions.  Console probes are done by the individual
 * framebuffer drivers.
 */

void
itedisplaycnattach(struct grf_data *gp, struct itesw *isw)
{
	struct ite_data *ip = &ite_cn;

	/*
	 * Set up required ite data and initialize ite.
	 */
	ip->isw = isw;
	ip->grf = gp;
	ip->flags = ITE_ALIVE|ITE_CONSOLE|ITE_ACTIVE|ITE_ISCONS;
	ip->attrbuf = ite_console_attributes;
	iteinit(ip);
	console_display_attached = 1;

	if (console_kbd_attached && console_display_attached)
		itecninit();
}

void
itekbdcnattach(struct ite_kbdops *ops, struct ite_kbdmap *map)
{

	console_kbdops = ops;
	console_kbdmap = map;
	console_kbd_attached = 1;

	if (console_kbd_attached && console_display_attached)
		itecninit();
}

void
itecninit(void)
{

	cn_tab = &ite_cons;
	cn_tab->cn_dev = makedev(cdevsw_lookup_major(&ite_cdevsw), 0);
}

/*ARGSUSED*/
int
itecngetc(dev_t dev)
{
	int c = 0;
	int stat;

	if (console_kbdops == NULL)
		return -1;

	c = (*console_kbdops->getc)(&stat);
	switch ((stat >> KBD_SSHIFT) & KBD_SMASK) {
	case KBD_SHIFT:
		c = console_kbdmap->shiftmap[c & KBD_CHARMASK];
		break;
	case KBD_CTRL:
		c = console_kbdmap->ctrlmap[c & KBD_CHARMASK];
		break;
	case KBD_KEY:
		c = console_kbdmap->keymap[c & KBD_CHARMASK];
		break;
	default:
		c = 0;
		break;
	}
	return c;
}

/* ARGSUSED */
void
itecnputc(dev_t dev, int c)
{
	static int paniced = 0;
	struct ite_data *ip = &ite_cn;

	if (panicstr && !paniced &&
	    (ip->flags & (ITE_ACTIVE|ITE_INGRF)) != ITE_ACTIVE) {
		(void) iteon(ip, 3);
		paniced = 1;
	}
	iteputchar(c, ip);
}
