/*	$NetBSD: pcons.c,v 1.7.4.2 2001/10/11 00:01:54 fvdl Exp $	*/

/*-
 * Copyright (c) 2000 Eduardo E. Horvath
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Default console driver.  Uses the PROM or whatever
 * driver(s) are appropriate.
 */

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/time.h>
#include <sys/syslog.h>
#include <sys/vnode.h>

#include <machine/autoconf.h>
#include <machine/openfirm.h>
#include <machine/conf.h>
#include <machine/cpu.h>
#include <machine/eeprom.h>
#include <machine/psl.h>

#include <dev/cons.h>

#include <sparc64/dev/cons.h>

static int pconsmatch __P((struct device *, struct cfdata *, void *));
static void pconsattach __P((struct device *, struct device *, void *));

struct cfattach pcons_ca = {
	sizeof(struct pconssoftc), pconsmatch, pconsattach
};

extern struct cfdriver pcons_cd;
static struct cnm_state pcons_cnm_state;

static int pconsprobe __P((void));
extern struct consdev *cn_tab;

static int
pconsmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;
	extern int  prom_cngetc __P((dev_t));

	/* Only attach if no other console has attached. */
	return ((strcmp("pcons", ma->ma_name) == 0) &&
		(cn_tab->cn_getc == prom_cngetc));

}

static void
pconsattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pconssoftc *sc = (struct pconssoftc *) self;

	printf("\n");
	if (!pconsprobe())
		return;

	cn_init_magic(&pcons_cnm_state);
	cn_set_magic("+++++");
	callout_init(&sc->sc_poll_ch);
}

static void pconsstart __P((struct tty *));
static int pconsparam __P((struct tty *, struct termios *));
static void pcons_poll __P((void *));

int
pconsopen(devvp, flag, mode, p)
	struct vnode *devvp;
	int flag, mode;
	struct proc *p;
{
	struct pconssoftc *sc;
	int unit;
	struct tty *tp;
	dev_t rdev;

	rdev = vdev_rdev(devvp);
	unit = minor(rdev);
	if (unit >= pcons_cd.cd_ndevs)
		return ENXIO;
	sc = pcons_cd.cd_devs[unit];
	if (!sc)
		return ENXIO;
	vdev_setprivdata(devvp, sc);
	if (!(tp = sc->of_tty))
		sc->of_tty = tp = ttymalloc();
	tp->t_oproc = pconsstart;
	tp->t_param = pconsparam;
	tp->t_devvp = devvp;
	cn_tab->cn_dev = rdev;
	if (!(tp->t_state & TS_ISOPEN)) {
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		pconsparam(tp, &tp->t_termios);
		ttsetwater(tp);
	} else if ((tp->t_state&TS_XCLUDE) && suser(p->p_ucred, &p->p_acflag))
		return EBUSY;
	tp->t_state |= TS_CARR_ON;
	
	if (!(sc->of_flags & OFPOLL)) {
		sc->of_flags |= OFPOLL;
		callout_reset(&sc->sc_poll_ch, 1, pcons_poll, sc);
	}

	return (*tp->t_linesw->l_open)(devvp, tp);
}

int
pconsclose(devvp, flag, mode, p)
	struct vnode *devvp;
	int flag, mode;
	struct proc *p;
{
	struct pconssoftc *sc;
	struct tty *tp;

	sc = vdev_privdata(devvp);
	tp = sc->of_tty;
	callout_stop(&sc->sc_poll_ch);
	sc->of_flags &= ~OFPOLL;
	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);
	return 0;
}

int
pconsread(devvp, uio, flag)
	struct vnode *devvp;
	struct uio *uio;
	int flag;
{
	struct pconssoftc *sc;
	struct tty *tp;

	sc = vdev_privdata(devvp);
	tp = sc->of_tty;

	return (*tp->t_linesw->l_read)(tp, uio, flag);
}

int
pconswrite(devvp, uio, flag)
	struct vnode *devvp;
	struct uio *uio;
	int flag;
{
	struct pconssoftc *sc;
	struct tty *tp;

	sc = vdev_privdata(devvp);
	tp = sc->of_tty;
	
	return (*tp->t_linesw->l_write)(tp, uio, flag);
}

int
pconspoll(devvp, events, p)
	struct vnode *devvp;
	int events;
	struct proc *p;
{
	struct pconssoftc *sc;
	struct tty *tp;

	sc = vdev_privdata(devvp);
	tp = sc->of_tty;
 
	return ((*tp->t_linesw->l_poll)(tp, events, p));
}

int
pconsioctl(devvp, cmd, data, flag, p)
	struct vnode *devvp;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct pconssoftc *sc;
	struct tty *tp;
	int error;

	sc = vdev_privdata(devvp);
	tp = sc->of_tty;
	
	if ((error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, p)) >= 0)
		return error;
	if ((error = ttioctl(tp, cmd, data, flag, p)) >= 0)
		return error;
	return ENOTTY;
}

struct tty *
pconstty(devvp)
	struct vnode *devvp;
{
	struct pconssoftc *sc;

	sc = vdev_privdata(devvp);
	return sc->of_tty;
}

void
pconsstop(tp, flag)
	struct tty *tp;
	int flag;
{
}

static void
pconsstart(tp)
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
		callout_reset(&tp->t_rstrt_ch, 1, ttrstrt, (void *)tp);
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
pconsparam(tp, t)
	struct tty *tp;
	struct termios *t;
{
	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;
	return 0;
}

static void
pcons_poll(aux)
	void *aux;
{
	struct pconssoftc *sc = aux;
	struct tty *tp = sc->of_tty;
	char ch;
	
	while (OF_read(stdin, &ch, 1) > 0) {
		cn_check_magic(vdev_rdev(tp->t_devvp), ch, pcons_cnm_state);
		if (tp && (tp->t_state & TS_ISOPEN))
			(*tp->t_linesw->l_rint)(ch, tp);
	}
	callout_reset(&sc->sc_poll_ch, 1, pcons_poll, sc);
}

int
pconsprobe()
{
	if (!stdin) stdin = OF_stdin();
	if (!stdout) stdout = OF_stdout();

	return (stdin && stdout);
}

void
pcons_cnpollc(dev, on)
	dev_t dev;
	int on;
{
	struct pconssoftc *sc = NULL;

	if (pcons_cd.cd_devs) 
		sc = pcons_cd.cd_devs[minor(dev)];

	if (on) {
		if (!sc) return;
		if (sc->of_flags & OFPOLL)
			callout_stop(&sc->sc_poll_ch);
		sc->of_flags &= ~OFPOLL;
	} else {
                /* Resuming kernel. */
		if (sc && !(sc->of_flags & OFPOLL)) {
			sc->of_flags |= OFPOLL;
			callout_reset(&sc->sc_poll_ch, 1, pcons_poll, sc);
		}
	}
}

void pcons_dopoll __P((void));
void
pcons_dopoll() {
		pcons_poll((void*)pcons_cd.cd_devs[0]);
}
