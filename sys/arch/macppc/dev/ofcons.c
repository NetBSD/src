/*	$NetBSD: ofcons.c,v 1.7.2.1 2002/06/23 17:37:54 jdolecek Exp $	*/

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

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/tty.h>

#include <dev/cons.h>
#include <dev/ofw/openfirm.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#define KEYBOARD_ARRAY
#include <machine/keyboard.h>
#include <machine/adbsys.h>

#include <machine/autoconf.h>

#include "adb.h"

struct ofcons_softc {
	struct device of_dev;
	struct tty *of_tty;
};

#define	OFBURSTLEN	128	/* max number of bytes to write in one chunk */

static int stdin, stdout;

static int ofcmatch __P((struct device *, struct cfdata *, void *));
static void ofcattach __P((struct device *, struct device *, void *));

struct cfattach macofcons_ca = {
	sizeof(struct ofcons_softc), ofcmatch, ofcattach
};

extern struct cfdriver macofcons_cd;

/* For polled ADB mode */
static int polledkey;
extern int adb_polling;

static void ofcstart __P((struct tty *));
static int ofcparam __P((struct tty *, struct termios *));
static int ofcons_probe __P((void));

static int
ofcmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;
	static int attached = 0;

	if (attached)
		return 0;

	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_DISPLAY)
		return 0;

	if (!ofcons_probe())
		return 0;

	attached = 1;
	return 1;
}

static void
ofcattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	printf("\n");
}

int
ofcopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct ofcons_softc *sc;
	int unit = minor(dev);
	struct tty *tp;
	
	if (unit >= macofcons_cd.cd_ndevs)
		return ENXIO;
	sc = macofcons_cd.cd_devs[unit];
	if (!sc)
		return ENXIO;
	if (!(tp = sc->of_tty))
		sc->of_tty = tp = ttymalloc();
	tp->t_oproc = ofcstart;
	tp->t_param = ofcparam;
	tp->t_dev = dev;
	if (!(tp->t_state & TS_ISOPEN)) {
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		ofcparam(tp, &tp->t_termios);
		ttsetwater(tp);
	} else if ((tp->t_state&TS_XCLUDE) && suser(p->p_ucred, &p->p_acflag))
		return EBUSY;
	tp->t_state |= TS_CARR_ON;
	
	return (*tp->t_linesw->l_open)(dev, tp);
}

int
ofcclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct ofcons_softc *sc = macofcons_cd.cd_devs[minor(dev)];
	struct tty *tp = sc->of_tty;

	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);
	return 0;
}

int
ofcread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct ofcons_softc *sc = macofcons_cd.cd_devs[minor(dev)];
	struct tty *tp = sc->of_tty;
	
	return (*tp->t_linesw->l_read)(tp, uio, flag);
}

int
ofcwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct ofcons_softc *sc = macofcons_cd.cd_devs[minor(dev)];
	struct tty *tp = sc->of_tty;
	
	return (*tp->t_linesw->l_write)(tp, uio, flag);
}

int
ofcpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct ofcons_softc *sc = macofcons_cd.cd_devs[minor(dev)];
	struct tty *tp = sc->of_tty;
 
	return ((*tp->t_linesw->l_poll)(tp, events, p));
}

int
ofcioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct ofcons_softc *sc = macofcons_cd.cd_devs[minor(dev)];
	struct tty *tp = sc->of_tty;
	int error;
	
	if ((error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, p)) != EPASSTHROUGH)
		return error;
	return ttioctl(tp, cmd, data, flag, p);
}

struct tty *
ofctty(dev)
	dev_t dev;
{
	struct ofcons_softc *sc = macofcons_cd.cd_devs[minor(dev)];

	return sc->of_tty;
}

void
ofcstop(tp, flag)
	struct tty *tp;
	int flag;
{
}

static void
ofcstart(tp)
	struct tty *tp;
{
	struct clist *cl;
	int s, len;
	u_char buf[OFBURSTLEN];
	
	s = spltty();
	if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP)) {
		splx(s);
		return;
	}
	tp->t_state |= TS_BUSY;
	splx(s);
	cl = &tp->t_outq;
	len = q_to_b(cl, buf, OFBURSTLEN);
	OF_write(stdout, buf, len);
	s = spltty();
	tp->t_state &= ~TS_BUSY;
	if (cl->c_cc) {
		tp->t_state |= TS_TIMEOUT;
		callout_reset(&tp->t_rstrt_ch, 1, ttrstrt, tp);
	}
	if (cl->c_cc <= tp->t_lowat) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup(cl);
		}
		selwakeup(&tp->t_wsel);
	}
	splx(s);
}

static int
ofcparam(tp, t)
	struct tty *tp;
	struct termios *t;
{
	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;
	return 0;
}

static int
ofcons_probe()
{
	int chosen;

	if (stdout)
		return 1;
	if ((chosen = OF_finddevice("/chosen")) == -1)
		return 0;
	if (OF_getprop(chosen, "stdin", &stdin, sizeof(stdin)) !=
			sizeof(stdin) ||
	    OF_getprop(chosen, "stdout", &stdout, sizeof(stdout)) !=
			sizeof(stdout))
		return 0;

	return 1;
}

/*
 * Console support functions
 */
void
ofccnprobe(cd)
	struct consdev *cd;
{
	int maj;

	if (!ofcons_probe())
		return;

	for (maj = 0; maj < nchrdev; maj++) {
		if (cdevsw[maj].d_open == ofcopen) {
			cd->cn_dev = makedev(maj, 0);
			cd->cn_pri = CN_INTERNAL;
			break;
		}
	}
}

void
ofccninit(cd)
	struct consdev *cd;
{
}

int
ofccngetc(dev)
	dev_t dev;
{
#if NADB > 0
	int intbits, s;

	s = splhigh();

	polledkey = -1;
	adb_polling = 1;

	while (polledkey == -1)
		adb_intr_cuda();

	adb_polling = 0;

	splx(s);
	return polledkey;
#else
	unsigned char ch = '\0';
	int l;
	
	while ((l = OF_read(stdin, &ch, 1)) != 1)
		if (l != -2 && l != 0)
			return -1;
	return ch;
#endif
}

void
ofccnputc(dev, c)
	dev_t dev;
	int c;
{
	char ch = c;
	
	OF_write(stdout, &ch, 1);
}

void
ofccnpollc(dev, on)
	dev_t dev;
	int on;
{
}

struct consdev consdev_ofcons = {
	ofccnprobe,
	ofccninit,
	ofccngetc,
	ofccnputc,
	ofccnpollc,
	NULL,
};

struct consdev *cn_tab = &consdev_ofcons;


/* For capslock key functionality */
#define isealpha(ch) \
  (((ch)>='A'&&(ch)<='Z')||((ch)>='a'&&(ch)<='z')||((ch)>=0xC0&&(ch)<=0xFF))

int
kbd_intr(event)
	adb_event_t *event;
{
	static int shift = 0, control = 0, capslock = 0;

	int key, press, val, state;
	char str[10], *s;
	struct ofcons_softc *sc = macofcons_cd.cd_devs[0];
	struct tty *ite_tty = sc->of_tty;

	key = event->u.k.key;
	press = ADBK_PRESS(key);
	val = ADBK_KEYVAL(key);

	if (val == ADBK_SHIFT)
		shift = press;
	else if (val == ADBK_CAPSLOCK)
		capslock = !capslock;
	else if (val == ADBK_CONTROL)
		control = press;
	else if (press) {
		switch (val) {
		case ADBK_UP:
			str[0] = '\e';
			str[1] = 'O';
			str[2] = 'A';
			str[3] = '\0';
			break;
		case ADBK_DOWN:
			str[0] = '\e';
			str[1] = 'O';
			str[2] = 'B';
			str[3] = '\0';
			break;
		case ADBK_RIGHT:
			str[0] = '\e';
			str[1] = 'O';
			str[2] = 'C';
			str[3] = '\0';
			break;
		case ADBK_LEFT:
			str[0] = '\e';
			str[1] = 'O';
			str[2] = 'D';
			str[3] = '\0';
			break;
		default:
			state = 0;
			if (capslock && isealpha(keyboard[val][1]))
				state = 1;
			if (shift)
				state = 1;
			if (control)
				state = 2;
			str[0] = keyboard[val][state];
			str[1] = '\0';
			break;
		}
		if (adb_polling)
			polledkey = str[0];
		else
			for (s = str; *s; s++)
				(*ite_tty->t_linesw->l_rint)(*s, ite_tty);
	}
	return 0;
}
