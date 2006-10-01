/* $NetBSD: promcons.c,v 1.30 2006/10/01 19:28:43 elad Exp $ */

/*
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: promcons.c,v 1.30 2006/10/01 19:28:43 elad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/kauth.h>

#include <uvm/uvm_extern.h>

#include <machine/cpuconf.h>
#include <machine/prom.h>

#ifdef _PMAP_MAY_USE_PROM_CONSOLE

dev_type_open(promopen);
dev_type_close(promclose);
dev_type_read(promread);
dev_type_write(promwrite);
dev_type_ioctl(promioctl);
dev_type_stop(promstop);
dev_type_tty(promtty);
dev_type_poll(prompoll);

const struct cdevsw prom_cdevsw = {
	promopen, promclose, promread, promwrite, promioctl,
	promstop, promtty, prompoll, nommap, ttykqfilter, D_TTY
};

#define	PROM_POLL_HZ	50

static struct  tty *prom_tty[1];
static int polltime;

void	promstart(struct tty *);
void	promtimeout(void *);
int	promparam(struct tty *, struct termios *);

struct callout prom_ch = CALLOUT_INITIALIZER;

int
promopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	int unit = minor(dev);
	struct tty *tp;
	int s;
	int error = 0, setuptimeout = 0;
 
	if (!pmap_uses_prom_console() || unit >= 1)
		return ENXIO;

	s = spltty();

	if (!prom_tty[unit]) {
		tp = prom_tty[unit] = ttymalloc();
		tty_attach(tp);
	} else
		tp = prom_tty[unit];

	tp->t_oproc = promstart;
	tp->t_param = promparam;
	tp->t_dev = dev;

	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp)) {
		splx(s);
		return (EBUSY);
	}

	if ((tp->t_state & TS_ISOPEN) == 0) {
		tp->t_state |= TS_CARR_ON;
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG|CLOCAL;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = 9600;
		ttsetwater(tp);

		setuptimeout = 1;
	}

	splx(s);

	error = (*tp->t_linesw->l_open)(dev, tp);
	if (error == 0 && setuptimeout) {
		polltime = hz / PROM_POLL_HZ;
		if (polltime < 1)
			polltime = 1;
		callout_reset(&prom_ch, polltime, promtimeout, tp);
	}
	return error;
}
 
int
promclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	int unit = minor(dev);
	struct tty *tp = prom_tty[unit];

	callout_stop(&prom_ch);
	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);
	return 0;
}
 
int
promread(dev_t dev, struct uio *uio, int flag)
{
	struct tty *tp = prom_tty[minor(dev)];

	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}
 
int
promwrite(dev_t dev, struct uio *uio, int flag)
{
	struct tty *tp = prom_tty[minor(dev)];
 
	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}
 
int
prompoll(dev_t dev, int events, struct lwp *l)
{
	struct tty *tp = prom_tty[minor(dev)];
 
	return ((*tp->t_linesw->l_poll)(tp, events, l));
}

int
promioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct lwp *l)
{
	int unit = minor(dev);
	struct tty *tp = prom_tty[unit];
	int error;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;
	return ttioctl(tp, cmd, data, flag, l);
}

int
promparam(struct tty *tp, struct termios *t)
{

	return 0;
}

void
promstart(struct tty *tp)
{
	int s;

	s = spltty();
	if (tp->t_state & (TS_TTSTOP | TS_BUSY))
		goto out;
	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)&tp->t_outq);
		}
		selwakeup(&tp->t_wsel);
	}
	tp->t_state |= TS_BUSY;
	while (tp->t_outq.c_cc != 0)
		promcnputc(tp->t_dev, getc(&tp->t_outq));
	tp->t_state &= ~TS_BUSY;
out:
	splx(s);
}

/*
 * Stop output on a line.
 */
void
promstop(struct tty *tp, int flag)
{
	int s;

	s = spltty();
	if (tp->t_state & TS_BUSY)
		if ((tp->t_state & TS_TTSTOP) == 0)
			tp->t_state |= TS_FLUSH;
	splx(s);
}

void
promtimeout(void *v)
{
	struct tty *tp = v;
	u_char c;

	while (promcnlookc(tp->t_dev, &c)) {
		if (tp->t_state & TS_ISOPEN)
			(*tp->t_linesw->l_rint)(c, tp);
	}
	callout_reset(&prom_ch, polltime, promtimeout, tp);
}

struct tty *
promtty(dev_t dev)
{

	if (minor(dev) != 0)
		panic("promtty: bogus");

	return prom_tty[0];
}

#else /* _PMAP_MAY_USE_PROM_CONSOLE */

/*
 * If not defined _PMAP_MAY_USE_PROM_CONSOLE,
 * this fake prom_cdevsw is attached to the kernel.
 * NEVER REMOVE!
 */
const struct cdevsw prom_cdevsw = {
	noopen, noclose, noread, nowrite, noioctl,
	nostop, notty, nopoll, nommap,
};

#endif /* _PMAP_MAY_USE_PROM_CONSOLE */
