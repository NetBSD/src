/*	$NetBSD: kd.c,v 1.9 1995/03/10 02:09:35 gwr Exp $	*/

/*
 * Copyright (c) 1994 Gordon W. Ross
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Keyboard/Display device.
 *
 * This driver exists simply to provide a tty device that
 * the indirect console driver can point to.
 * The kbd driver sends its input here.
 * Output goes to the screen via PROM printf.
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/mon.h>

#include <dev/cons.h>

#define BURST	64

struct tty *kd_tty[1];

int kdopen(dev_t, int, int, struct proc *);
int kdclose(dev_t, int, int, struct proc *);
int kdread(dev_t, struct uio *, int);
int kdwrite(dev_t, struct uio *, int);
int kdioctl(dev_t, int, caddr_t, int, struct proc *);

static int kdparam(struct tty *, struct termios *);
static void kdstart(struct tty *);

/* This is called by kbd_serial() like a pseudo-device. */
void
kd_attach(n)
	int n;
{
	kd_tty[0] = ttymalloc();

	/* Tell keyboard module where to send read data. */
	kbd_ascii(kd_tty[0]);
}

int
kdopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	int error, unit;
	struct tty *tp;
	
	unit = minor(dev);
	if (unit) return ENXIO;

	tp = kd_tty[unit];
	if (tp == NULL)
		return ENXIO;

	if ((error = kbd_iopen()) != 0) {
#ifdef	DIAGNOSTIC
		printf("kd: kbd_iopen, error=%d\n", error);
#endif
		return (error);
	}

	tp->t_oproc = kdstart;
	tp->t_param = kdparam;
	tp->t_dev = dev;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		tp->t_state |= TS_WOPEN;
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		kdparam(tp, &tp->t_termios);
		ttsetwater(tp);
	} else if (tp->t_state & TS_XCLUDE && p->p_ucred->cr_uid != 0)
		return EBUSY;
	tp->t_state |= TS_CARR_ON;

	return ((*linesw[tp->t_line].l_open)(dev, tp));
}

int
kdclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	int unit = minor(dev);
	struct tty *tp = kd_tty[unit];

	(*linesw[tp->t_line].l_close)(tp, flag);
	ttyclose(tp);
	return (0);
}

int
kdread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	int unit = minor(dev);
	struct tty *tp = kd_tty[unit];

	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}

int
kdwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	int unit = minor(dev);
	struct tty *tp = kd_tty[unit];

	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

int
kdioctl(dev, cmd, data, flag, p)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	int error;
	int unit = minor(dev);
	struct tty *tp = kd_tty[unit];

	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag, p);
	if (error >= 0)
		return error;
	error = ttioctl(tp, cmd, data, flag, p);
	if (error >= 0)
		return error;

	/* Handle any ioctl commands specific to kbd/display. */
	/* XXX - Send KB* ioctls to kbd module? */
	/* XXX - Send FB* ioctls to fb module?  */

	return ENOTTY;
}

void
kdstart(tp)
	struct tty *tp;
{
	struct clist *cl;
	int s, len;
	u_char buf[BURST];

	s = spltty();
	if (tp->t_state & (TS_BUSY|TS_TTSTOP|TS_TIMEOUT))
		goto out;
	tp->t_state |= TS_BUSY;
	cl = &tp->t_outq;

	/*
	 * XXX - It might be nice to have our own fbputs() so
	 * we would not have to use the (slow) PROM printf.
	 */
	while ((len = q_to_b(cl, buf, BURST-1)) > 0) {
		buf[len] = '\0';
		(void) splhigh();
#if 0
		/* XXX - Not yet.  Not sure what args should be. */
		(romVectorPtr->fbWriteStr)(buf);
#else
		mon_printf("%s", buf);
#endif
		(void) spltty();
	}

	tp->t_state &= ~TS_BUSY;
	if (tp->t_state & TS_ASLEEP) {
		tp->t_state &= ~TS_ASLEEP;
		wakeup((caddr_t)cl);
	}
	selwakeup(&tp->t_wsel);
out:
	splx(s);
}

static int
kdparam(tp, t)
	struct tty *tp;
	struct termios *t;
{
	/* XXX - These are ignored... */
	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;
	return 0;
}


/*
 * kd console support
 */

extern int zscnprobe_kbd(), zscngetc(), kbd_translate();

kdcnprobe(cp)
	struct consdev *cp;
{
	int maj;

	/* locate the major number */
	for (maj = 0; maj < nchrdev; maj++)
		if (cdevsw[maj].d_open == (void*)kdopen)
			break;

	/* initialize required fields */
	cp->cn_dev = makedev(maj, 0);
	cp->cn_pri = zscnprobe_kbd();
}

kdcninit(cp)
	struct consdev *cp;
{

	/* This prepares zscngetc() */
	zs_set_conschan(1, 0);

	/* This prepares kbd_translate() */
	kbd_init_tables();

	mon_printf("console on kd0 (keyboard/display)\n");
}

kdcngetc(dev)
	dev_t dev;
{
	int c;

	do {
		c = zscngetc(0);
		c = kbd_translate(c);
	} while (c == -1);

	return (c);
}

kdcnputc(dev, c)
	dev_t dev;
	int c;
{
	int s;

	s = splhigh();
#if 0
	/* XXX - Is this worth doing? */
	(romVectorPtr->fbWriteChar)(c);
#else
	mon_putchar(c);
#endif
	splx(s);
}

extern void fb_unblank();
void kdcnpollc(dev, on)
	dev_t dev;
	int on;
{
	if (on)
		fb_unblank();
}
