/*	$NetBSD: console.c,v 1.2 2004/04/24 17:35:28 cl Exp $	*/

/*
 *
 * Copyright (c) 2004 Christian Limpach.
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
 *      This product includes software developed by Christian Limpach.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: console.c,v 1.2 2004/04/24 17:35:28 cl Exp $");

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>

#include <machine/stdarg.h>
#include <machine/xen.h>
#include <machine/hypervisor.h>

#include <dev/cons.h>

#include <ddb/db_output.h>	/* XXX for db_max_line */

static int xenc_isconsole = 0;

#define	XENC_UNIT(x)	(minor(x))
#define XENC_BURST 128

int xenc_match (struct device *, struct cfdata *, void *);
void xenc_attach (struct device *, struct device *, void *);
/* int xenc_intr (void *); */
void xenc_init (void);

struct xenc_softc {
	struct	device sc_dev;
	struct	tty *sc_tty;
};

CFATTACH_DECL(xenc, sizeof(struct xenc_softc),
    xenc_match, xenc_attach, NULL, NULL);

extern struct cfdriver xenc_cd;

dev_type_open(xenc_open);
dev_type_close(xenc_close);
dev_type_read(xenc_read);
dev_type_write(xenc_write);
dev_type_ioctl(xenc_ioctl);
dev_type_stop(xenc_stop);
dev_type_tty(xenc_tty);
dev_type_poll(xenc_poll);

const struct cdevsw xenc_cdevsw = {
	xenc_open, xenc_close, xenc_read, xenc_write, xenc_ioctl,
	xenc_stop, xenc_tty, xenc_poll, NULL, ttykqfilter, D_TTY
};


void xencn_attach(void);
int xencn_getc(dev_t);
void xencn_putc(dev_t, int);
void xencn_pollc(dev_t, int);

static struct consdev xencons = {
	NULL, NULL, xencn_getc, xencn_putc, xencn_pollc, NULL, NULL, NULL,
	NODEV, CN_NORMAL
};

void	xenc_start (struct tty *);
int	xenc_param (struct tty *, struct termios *);

int
xenc_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct xenc_attach_args *xa = (struct xenc_attach_args *)aux;

	if (strcmp(xa->xa_device, "xenc") == 0)
		return 1;
	return 0;
}

void
xenc_attach(struct device *parent, struct device *self, void *aux)
{
	struct xenc_softc *sc = (void *)self;

	aprint_normal(": Xen Virtual Console Driver\n");

	if (xenc_isconsole) {
		int maj;

		/* Locate the major number. */
		maj = cdevsw_lookup_major(&xenc_cdevsw);

		/* There can be only one, but it can have any unit number. */
		cn_tab->cn_dev = makedev(maj, sc->sc_dev.dv_unit);

		aprint_verbose("%s: console major %d, unit %d\n",
		    sc->sc_dev.dv_xname, maj, sc->sc_dev.dv_unit);

		/* Set db_max_line to avoid paging. */
		db_max_line = 0x7fffffff;
	}
}

int
xenc_open(dev_t dev, int flag, int mode, struct proc *p)
{
	struct xenc_softc *sc;
	int unit = XENC_UNIT(dev);
	struct tty *tp;

	sc = device_lookup(&xenc_cd, unit);
	if (sc == NULL)
		return (ENXIO);

	if (!sc->sc_tty) {
		tp = sc->sc_tty = ttymalloc();
		tty_attach(tp);
	} else
		tp = sc->sc_tty;

	tp->t_oproc = xenc_start;
	tp->t_param = xenc_param;
	tp->t_dev = dev;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		xenc_param(tp, &tp->t_termios);
		ttsetwater(tp);
	} else if (tp->t_state&TS_XCLUDE && p->p_ucred->cr_uid != 0)
		return (EBUSY);
	tp->t_state |= TS_CARR_ON;

	return ((*tp->t_linesw->l_open)(dev, tp));
}

int
xenc_close(dev_t dev, int flag, int mode, struct proc *p)
{
	struct xenc_softc *sc = device_lookup(&xenc_cd, XENC_UNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (tp == NULL)
		return (0);
	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);
#ifdef notyet /* XXX */
	ttyfree(tp);
#endif
	return (0);
}

int
xenc_read(dev_t dev, struct uio *uio, int flag)
{
	struct xenc_softc *sc = device_lookup(&xenc_cd, XENC_UNIT(dev));
	struct tty *tp = sc->sc_tty;

	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}

int
xenc_write(dev_t dev, struct uio *uio, int flag)
{
	struct xenc_softc *sc = device_lookup(&xenc_cd, XENC_UNIT(dev));
	struct tty *tp = sc->sc_tty;

	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
xenc_poll(dev_t dev, int events, struct proc *p)
{
	struct xenc_softc *sc = device_lookup(&xenc_cd, XENC_UNIT(dev));
	struct tty *tp = sc->sc_tty;
 
	return ((*tp->t_linesw->l_poll)(tp, events, p));
}

struct tty *
xenc_tty(dev_t dev)
{
	struct xenc_softc *sc = device_lookup(&xenc_cd, XENC_UNIT(dev));
	struct tty *tp = sc->sc_tty;

	return (tp);
}

int
xenc_ioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct xenc_softc *sc = device_lookup(&xenc_cd, XENC_UNIT(dev));
	struct tty *tp = sc->sc_tty;
	int error;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, p);
	if (error != EPASSTHROUGH)
		return (error);

	error = ttioctl(tp, cmd, data, flag, p);
	if (error != EPASSTHROUGH)
		return (error);

	switch (cmd) {
	default:
		return (EPASSTHROUGH);
	}

#ifdef DIAGNOSTIC
	panic("xenc_ioctl: impossible");
#endif
}

void
xenc_start(struct tty *tp)
{
	struct clist *cl;
	int s, len;
	u_char buf[XENC_BURST+1];

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP))
		goto out;
	tp->t_state |= TS_BUSY;
	splx(s);

	/*
	 * We need to do this outside spl since it could be fairly
	 * expensive and we don't want our serial ports to overflow.
	 */
	cl = &tp->t_outq;
	len = q_to_b(cl, buf, XENC_BURST);
	(void)HYPERVISOR_console_write(buf, len);

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
out:
	splx(s);
}

void
xenc_stop(struct tty *tp, int flag)
{

}



void
xencn_attach()
{

	cn_tab = &xencons;

	xenc_isconsole = 1;
}

int
xencn_getc(dev_t dev)
{

	printf("\n");
	for (;;);
}

#define MAXLINELEN 1024
void
xencn_putc(dev_t dev, int c)
{
	static char buf[1024+1];
	static int bufpos = 0;

	buf[bufpos++] = c;
	if (c == '\n') {
		buf[bufpos] = 0;
		(void)HYPERVISOR_console_write(buf, bufpos);
		bufpos = 0;
	}
}

void
xencn_pollc(dev_t dev, int on)
{
	
}

/*
 * Set line parameters.
 */
int
xenc_param(struct tty *tp, struct termios *t)
{

	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;
	return (0);
}

