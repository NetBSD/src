/*	$NetBSD: console.c,v 1.3 2001/05/02 10:32:22 scw Exp $	*/

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

#include <sys/param.h>
#include <sys/user.h>
#include <sys/uio.h> 
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/termios.h>

#include <machine/conf.h>
#include <machine/arcs.h>

#include <dev/cons.h>

static void	arcs_cnputc(dev_t, int);
static int	arcs_cngetc(dev_t);
static int	arcs_cnlookc(dev_t, int *);

struct consdev arcs_cn = {
        NULL, NULL, arcs_cngetc, arcs_cnputc, nullcnpollc, NULL, NODEV,
	CN_NORMAL
};

struct callout arcs_ch = CALLOUT_INITIALIZER;

void
consinit()
{
	arcs_cn.cn_dev = makedev(37, 0);

	cn_tab = &arcs_cn;

	return;
}

static void
arcs_cnputc(dev, c)
	dev_t dev;
	int c;
{
	char ch = c;
	u_int32_t count;

	ARCS->Write(ARCS_STDOUT, &ch, 1, &count);

	return;
}

static int
arcs_cngetc(dev)
	dev_t dev;
{
	char c;
	u_int32_t count;

	ARCS->Read(ARCS_STDIN, &c, 1, &count);

	return c;
}

static int
arcs_cnlookc(dev, cp)
	dev_t dev;
	int *cp;
{
	char c;
	int32_t q;
	u_int32_t count;

	q = ARCS->GetReadStatus(ARCS_STDIN);

	if (q == 0) {
		ARCS->Read(ARCS_STDIN, &c, 1, &count);
		*cp = c;	

		return 1;
	}
	else {
		return 0;
	}
}

static struct  tty *arcs_tty[1];

void	arcs_start(struct tty *);
void	arcs_poll(void *);
int	arcs_param(struct tty *, struct termios *);

int
arcsopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	int unit = minor(dev);
	struct tty *tp;
	int s;
	int error = 0, setuptimeout = 0;

	s = spltty();

	if (!arcs_tty[unit]) {
		tp = arcs_tty[unit] = ttymalloc();
		tty_attach(tp);
	} else
		tp = arcs_tty[unit];

	tp->t_oproc = arcs_start;
	tp->t_param = arcs_param;
	tp->t_dev = dev;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		tp->t_state |= TS_CARR_ON;
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG | CLOCAL;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = 9600;
		ttsetwater(tp);

		setuptimeout = 1;
	} else if (tp->t_state & TS_XCLUDE && p->p_ucred->cr_uid != 0) {
		splx(s);
		return EBUSY;
	}

	splx(s);

	error = (*tp->t_linesw->l_open)(dev, tp);
	if (error == 0 && setuptimeout) {
		callout_reset(&arcs_ch, 1, arcs_poll, tp);
	}
	return error;
}
 
int
arcsclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	int unit = minor(dev);
	struct tty *tp = arcs_tty[unit];

	callout_stop(&arcs_ch);
	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);
	return 0;
}
 
int
arcsread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct tty *tp = arcs_tty[minor(dev)];

	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}
 
int
arcswrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct tty *tp = arcs_tty[minor(dev)];
 
	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
arcspoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct tty *tp = arcs_tty[minor(dev)];
 
	return ((*tp->t_linesw->l_poll)(tp, events, p));
}
 
int
arcsioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	int unit = minor(dev);
	struct tty *tp = arcs_tty[unit];
	int error;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, p);
	if (error >= 0)
		return error;
	error = ttioctl(tp, cmd, data, flag, p);
	if (error >= 0)
		return error;

	return ENOTTY;
}

int
arcs_param(tp, t)
	struct tty *tp;
	struct termios *t;
{

	return 0;
}

void
arcs_start(tp)
	struct tty *tp;
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
		arcs_cnputc(tp->t_dev, getc(&tp->t_outq));
	tp->t_state &= ~TS_BUSY;
out:
	splx(s);
}

void
arcsstop(tp, flag)
	struct tty *tp;
{
	int s;

	s = spltty();
	if (tp->t_state & TS_BUSY)
		if ((tp->t_state & TS_TTSTOP) == 0)
			tp->t_state |= TS_FLUSH;
	splx(s);
}

void
arcs_poll(v)
	void *v;
{
	struct tty *tp = v;
	int c;
	int l_r;

	while (arcs_cnlookc(tp->t_dev, &c)) {
		if (tp->t_state & TS_ISOPEN)
			l_r = (*tp->t_linesw->l_rint)(c, tp);
	}
	callout_reset(&arcs_ch, 1, arcs_poll, tp);
}

struct tty *
arcstty(dev)
	dev_t dev;
{

	if (minor(dev) != 0)
		panic("arcs_tty: bogus");

	return arcs_tty[0];
}
