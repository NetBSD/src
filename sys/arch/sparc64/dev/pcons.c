/*	$NetBSD: pcons.c,v 1.25.10.2 2008/01/09 01:49:03 matt Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pcons.c,v 1.25.10.2 2008/01/09 01:49:03 matt Exp $");

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
#include <sys/kauth.h>

#include <machine/autoconf.h>
#include <machine/openfirm.h>
#include <machine/cpu.h>
#include <machine/eeprom.h>
#include <machine/psl.h>

#include <dev/cons.h>

#include <sparc64/dev/cons.h>

static int pconsmatch(struct device *, struct cfdata *, void *);
static void pconsattach(struct device *, struct device *, void *);

CFATTACH_DECL(pcons, sizeof(struct pconssoftc),
    pconsmatch, pconsattach, NULL, NULL);

extern struct cfdriver pcons_cd;

dev_type_open(pconsopen);
dev_type_close(pconsclose);
dev_type_read(pconsread);
dev_type_write(pconswrite);
dev_type_ioctl(pconsioctl);
dev_type_tty(pconstty);
dev_type_poll(pconspoll);

const struct cdevsw pcons_cdevsw = {
	pconsopen, pconsclose, pconsread, pconswrite, pconsioctl,
	nostop, pconstty, pconspoll, nommap, ttykqfilter, D_TTY
};

static struct cnm_state pcons_cnm_state;

static int pconsprobe(void);
extern struct consdev *cn_tab;

static int
pconsmatch(struct device *parent, struct cfdata *match, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	extern int  prom_cngetc(dev_t);

	/* Only attach if no other console has attached. */
	return ((strcmp("pcons", ma->ma_name) == 0) &&
		(cn_tab->cn_getc == prom_cngetc));

}

static void
pconsattach(struct device *parent, struct device *self, void *aux)
{
	struct pconssoftc *sc = (struct pconssoftc *) self;

	printf("\n");
	if (!pconsprobe())
		return;

	cn_init_magic(&pcons_cnm_state);
	cn_set_magic("+++++");
	callout_init(&sc->sc_poll_ch, 0);
}

static void pconsstart(struct tty *);
static int pconsparam(struct tty *, struct termios *);
static void pcons_poll(void *);

int
pconsopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct pconssoftc *sc;
	int unit = minor(dev);
	struct tty *tp;
	struct proc *p;

	p = l->l_proc;
	
	if (unit >= pcons_cd.cd_ndevs)
		return ENXIO;
	sc = pcons_cd.cd_devs[unit];
	if (!sc)
		return ENXIO;
	if (!(tp = sc->of_tty))
		sc->of_tty = tp = ttymalloc();
	tp->t_oproc = pconsstart;
	tp->t_param = pconsparam;
	tp->t_dev = dev;
	cn_tab->cn_dev = dev;
	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp))
		return (EBUSY);
	if (!(tp->t_state & TS_ISOPEN)) {
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		pconsparam(tp, &tp->t_termios);
		ttsetwater(tp);
	}
	tp->t_state |= TS_CARR_ON;
	
	if (!(sc->of_flags & OFPOLL)) {
		sc->of_flags |= OFPOLL;
		callout_reset(&sc->sc_poll_ch, 1, pcons_poll, sc);
	}

	return (*tp->t_linesw->l_open)(dev, tp);
}

int
pconsclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct pconssoftc *sc = pcons_cd.cd_devs[minor(dev)];
	struct tty *tp = sc->of_tty;

	callout_stop(&sc->sc_poll_ch);
	sc->of_flags &= ~OFPOLL;
	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);
	return 0;
}

int
pconsread(dev_t dev, struct uio *uio, int flag)
{
	struct pconssoftc *sc = pcons_cd.cd_devs[minor(dev)];
	struct tty *tp = sc->of_tty;
	
	return (*tp->t_linesw->l_read)(tp, uio, flag);
}

int
pconswrite(dev_t dev, struct uio *uio, int flag)
{
	struct pconssoftc *sc = pcons_cd.cd_devs[minor(dev)];
	struct tty *tp = sc->of_tty;
	
	return (*tp->t_linesw->l_write)(tp, uio, flag);
}

int
pconspoll(dev_t dev, int events, struct lwp *l)
{
	struct pconssoftc *sc = pcons_cd.cd_devs[minor(dev)];
	struct tty *tp = sc->of_tty;
 
	return ((*tp->t_linesw->l_poll)(tp, events, l));
}

int
pconsioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct pconssoftc *sc = pcons_cd.cd_devs[minor(dev)];
	struct tty *tp = sc->of_tty;
	int error;
	
	if ((error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l)) != EPASSTHROUGH)
		return error;
	return ttioctl(tp, cmd, data, flag, l);
}

struct tty *
pconstty(dev_t dev)
{
	struct pconssoftc *sc = pcons_cd.cd_devs[minor(dev)];

	return sc->of_tty;
}

static void
pconsstart(struct tty *tp)
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
	prom_write(prom_stdout(), buf, len);
	s = spltty();
	tp->t_state &= ~TS_BUSY;
	if (ttypull(tp)) {
		tp->t_state |= TS_TIMEOUT;
		callout_schedule(&tp->t_rstrt_ch, 1);
	}
	splx(s);
}

static int
pconsparam(struct tty *tp, struct termios *t)
{
	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;
	return 0;
}

static void
pcons_poll(void *aux)
{
	struct pconssoftc *sc = aux;
	struct tty *tp = sc->of_tty;
	char ch;
	
	while (prom_read(prom_stdin(), &ch, 1) > 0) {
		cn_check_magic(tp->t_dev, ch, pcons_cnm_state);
		if (tp && (tp->t_state & TS_ISOPEN))
			(*tp->t_linesw->l_rint)(ch, tp);
	}
	callout_reset(&sc->sc_poll_ch, 1, pcons_poll, sc);
}

int
pconsprobe()
{

	return (prom_stdin() && prom_stdout());
}

void
pcons_cnpollc(dev_t dev, int on)
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

void pcons_dopoll(void);
void
pcons_dopoll() {
		pcons_poll((void*)pcons_cd.cd_devs[0]);
}
