/*	$NetBSD: biconsdev.c,v 1.2 2000/03/23 06:38:02 thorpej Exp $	*/

/*-
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
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
 */
/*
 * Copyright (c) 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ted Lemon.
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
 */

#include "biconsdev.h"
#if NBICONSDEV > 0

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>

#include <dev/cons.h>
#include <hpcmips/dev/bicons.h>
#include <hpcmips/dev/biconsvar.h>


struct tty biconsdev_tty[NBICONSDEV];
void	biconsdevattach __P((int n));
static	void biconsdev_output __P((struct tty *));

cdev_decl(biconsdev);

void
biconsdevattach (n)
	int n;
{
	register struct tty *tp = &biconsdev_tty [0];

	/* Set up the tty queues now... */
	clalloc(&tp->t_rawq, 1024, 1);
	clalloc(&tp->t_canq, 1024, 1);
	/* output queue doesn't need quoting */
	clalloc(&tp->t_outq, 1024, 0);

	tp->t_dev = makedev(34, 0);
	tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
	tp->t_param = (int (*)(struct tty *, struct termios *))nullop;
	tp->t_winsize.ws_row = bicons_height;
	tp->t_winsize.ws_col = bicons_width;
	tp->t_winsize.ws_xpixel = bicons_xpixel;
	tp->t_winsize.ws_ypixel = bicons_ypixel;
	tp->t_oproc = biconsdev_output;

	/*
	 *  XXX, We must check the bootinfo.
	 */
	if (cn_tab->cn_dev == makedev(0, 0)) {
		cn_tab->cn_dev = tp->t_dev;
	}
#ifdef DEBUG
	printf("%s(%d): biconsdevattach(): cn_tab->cn_dev=%x %x\n",
	       __FILE__, __LINE__, (u_int)&cn_tab->cn_dev, tp->t_dev);
#endif
}


static void
biconsdev_output(tp)
	register struct tty *tp;
{
	register int s, n;
	char buf[OBUFSIZ];

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP)) {
		splx(s);
		return;
	}
	tp->t_state |= TS_BUSY;
	splx(s);
	n = q_to_b(&tp->t_outq, buf, sizeof(buf));
	bicons_putn(buf, n);

	s = spltty();
	tp->t_state &= ~TS_BUSY;
	/* Come back if there's more to do */
	if (tp->t_outq.c_cc) {
		tp->t_state |= TS_TIMEOUT;
		callout_reset(&tp->t_rstrt_ch, 1, ttrstrt, tp);
	}
	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (tp->t_state&TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)&tp->t_outq);
		}
		selwakeup(&tp->t_wsel);
	}
	splx(s);
}


int
biconsdevopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	register struct tty *tp = &biconsdev_tty [0];
	int status;

	if ((tp->t_state & TS_ISOPEN) == 0) {
		/*
		 * Leave baud rate alone!
		 */
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_cflag = TTYDEF_CFLAG;
		tp->t_state = TS_ISOPEN | TS_CARR_ON;
		(void)(*tp->t_param)(tp, &tp->t_termios);
		ttsetwater(tp);
	} else if (tp->t_state & TS_XCLUDE && p->p_ucred->cr_uid != 0)
		return (EBUSY);

	status = (*linesw[tp->t_line].l_open)(dev, tp);
	return status;
}


int
biconsdevclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	register struct tty *tp = &biconsdev_tty [0];

	(*linesw[tp->t_line].l_close)(tp, flag);
	ttyclose(tp);

	return (0);
}


int
biconsdevread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register struct tty *tp = &biconsdev_tty [0];

	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}


int
biconsdevwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register struct tty *tp = &biconsdev_tty [0];

	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}


struct tty *
biconsdevtty(dev)
        dev_t dev;
{
	register struct tty *tp = &biconsdev_tty [0];

        return (tp);
}

int
biconsdevioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	register struct tty *tp = &biconsdev_tty [0];
	int error;

	if ((error = linesw[tp->t_line].l_ioctl(tp, cmd, data, flag, p)) >= 0)
		return (error);
	if ((error = ttioctl(tp, cmd, data, flag, p)) >= 0)
		return (error);
	return (ENOTTY);
}

void
biconsdevstop(tp, rw)
	struct tty *tp;
	int rw;
{

}

#endif /* NBICONSDEV > 0 */
