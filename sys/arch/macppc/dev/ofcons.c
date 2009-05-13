/*	$NetBSD: ofcons.c,v 1.23.10.1 2009/05/13 17:18:01 jym Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ofcons.c,v 1.23.10.1 2009/05/13 17:18:01 jym Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/kauth.h>

#include <dev/cons.h>
#include <dev/ofw/openfirm.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <machine/adbsys.h>

#include <machine/autoconf.h>

#include "adb.h"

struct ofcons_softc {
	struct device of_dev;
	struct tty *of_tty;
};

#define	OFBURSTLEN	128	/* max number of bytes to write in one chunk */

static int stdin, stdout;

static int ofcmatch(struct device *, struct cfdata *, void *);
static void ofcattach(struct device *, struct device *, void *);

CFATTACH_DECL(macofcons, sizeof(struct ofcons_softc),
    ofcmatch, ofcattach, NULL, NULL);

extern struct cfdriver macofcons_cd;

dev_type_open(ofcopen);
dev_type_close(ofcclose);
dev_type_read(ofcread);
dev_type_write(ofcwrite);
dev_type_ioctl(ofcioctl);
dev_type_tty(ofctty);
dev_type_poll(ofcpoll);

const struct cdevsw macofcons_cdevsw = {
	ofcopen, ofcclose, ofcread, ofcwrite, ofcioctl,
	nostop, ofctty, ofcpoll, nommap, ttykqfilter, D_TTY
};

/* For polled ADB mode */
#if NADB > 0
static int polledkey;
extern int adb_polling;
#endif /* NADB */

static void ofcstart(struct tty *);
static int ofcparam(struct tty *, struct termios *);
static int ofcons_probe(void);

static int
ofcmatch(struct device *parent, struct cfdata *match, void *aux)
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
ofcattach(struct device *parent, struct device *self, void *aux)
{
	printf("\n");
}

int
ofcopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct ofcons_softc *sc;
	struct tty *tp;
	
	sc = device_lookup_private(&macofcons_cd, minor(dev));
	if (!sc)
		return ENXIO;
	if (!(tp = sc->of_tty))
		sc->of_tty = tp = ttymalloc();
	tp->t_oproc = ofcstart;
	tp->t_param = ofcparam;
	tp->t_dev = dev;
	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp))
		return (EBUSY);
	if (!(tp->t_state & TS_ISOPEN)) {
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		ofcparam(tp, &tp->t_termios);
		ttsetwater(tp);
	}
	tp->t_state |= TS_CARR_ON;
	
	return (*tp->t_linesw->l_open)(dev, tp);
}

int
ofcclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct ofcons_softc *sc = device_lookup_private(&macofcons_cd, minor(dev));
	struct tty *tp = sc->of_tty;

	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);
	return 0;
}

int
ofcread(dev_t dev, struct uio *uio, int flag)
{
	struct ofcons_softc *sc = device_lookup_private(&macofcons_cd, minor(dev));
	struct tty *tp = sc->of_tty;
	
	return (*tp->t_linesw->l_read)(tp, uio, flag);
}

int
ofcwrite(dev_t dev, struct uio *uio, int flag)
{
	struct ofcons_softc *sc = device_lookup_private(&macofcons_cd, minor(dev));
	struct tty *tp = sc->of_tty;
	
	return (*tp->t_linesw->l_write)(tp, uio, flag);
}

int
ofcpoll(dev_t dev, int events, struct lwp *l)
{
	struct ofcons_softc *sc = device_lookup_private(&macofcons_cd, minor(dev));
	struct tty *tp = sc->of_tty;
 
	return ((*tp->t_linesw->l_poll)(tp, events, l));
}

int
ofcioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct ofcons_softc *sc = device_lookup_private(&macofcons_cd, minor(dev));
	struct tty *tp = sc->of_tty;
	int error;
	
	if ((error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l)) != EPASSTHROUGH)
		return error;
	return ttioctl(tp, cmd, data, flag, l);
}

struct tty *
ofctty(dev_t dev)
{
	struct ofcons_softc *sc = device_lookup_private(&macofcons_cd, minor(dev));

	return sc->of_tty;
}

static void
ofcstart(struct tty *tp)
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
	if (ttypull(tp)) {
		tp->t_state |= TS_TIMEOUT;
		callout_schedule(&tp->t_rstrt_ch, 1);
	}
	splx(s);
}

static int
ofcparam(struct tty *tp, struct termios *t)
{
	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;
	return 0;
}

static int
ofcons_probe(void)
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
static void
ofccnprobe(struct consdev *cd)
{
	int maj;

	if (!ofcons_probe())
		return;

	maj = cdevsw_lookup_major(&macofcons_cdevsw);

	printf("major for ofcons: %d\n", maj);

	cd->cn_dev = makedev(maj, 0);
	cd->cn_pri = CN_INTERNAL;
}

static void
ofccninit(struct consdev *cd)
{
}

static int
ofccngetc(dev_t dev)
{
#if NADB > 0
	int s;
	extern void adb_intr_cuda(void);	/* in adb_direct.c */

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

static void
ofccnputc(dev_t dev, int c)
{
	char ch = c;
	
	OF_write(stdout, &ch, 1);
}

static void
ofccnpollc(dev_t dev, int on)
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
