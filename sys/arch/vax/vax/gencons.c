/*
 * Copyright (c) 1994 Gordon W. Ross
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
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
 *     This product includes software developed at Ludd, University of Lule}.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 *
 *	from: kd.c,v 1.2 1994/05/05 04:46:51 gwr Exp $
 *	$Id: gencons.c,v 1.1 1994/08/16 23:47:26 ragge Exp $
 */

 /* All bugs are subject to removal without further notice */

#include "sys/param.h"
#include "sys/proc.h"
#include "sys/systm.h"
#include "sys/ioctl.h"
#include "sys/tty.h"
#include "sys/file.h"
#include "sys/conf.h"
#include "sys/device.h"
#include "dev/cons.h"

struct	tty gencnttyst;
struct tty *gencntty=&gencnttyst;

#define	BURST	64

int	gencnparam();
void	gencnstart();
int
gencnopen(dev, flag, mode, p)
	dev_t	dev;
	int	flag, mode;
	struct proc *p;
{
        int unit;
        struct tty *tp;

        unit = minor(dev);
        if (unit) return ENXIO;

	tp=gencntty;

	skrivstr("gencnopen\n");
asm("halt");
        tp->t_oproc = gencnstart;
        tp->t_param = gencnparam;
        tp->t_dev = dev;
        if ((tp->t_state & TS_ISOPEN) == 0) {
                tp->t_state |= TS_WOPEN;
                ttychars(tp);
                tp->t_iflag = TTYDEF_IFLAG;
                tp->t_oflag = TTYDEF_OFLAG;
                tp->t_cflag = TTYDEF_CFLAG;
                tp->t_lflag = TTYDEF_LFLAG;
                tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
                gencnparam(tp, &tp->t_termios);
                ttsetwater(tp);
        } else if (tp->t_state & TS_XCLUDE && p->p_ucred->cr_uid != 0)
                return EBUSY;
        tp->t_state |= TS_CARR_ON;

        return ((*linesw[tp->t_line].l_open)(dev, tp));
}

int
gencnclose(dev, flag, mode, p)
        dev_t dev;
        int flag, mode;
        struct proc *p;
{
        int unit = minor(dev);
        struct tty *tp = gencntty;

        (*linesw[tp->t_line].l_close)(tp, flag);
        ttyclose(tp);
        return (0);
}


int
gencnread(dev, uio, flag)
        dev_t dev;
        struct uio *uio;
        int flag;
{
        int unit = minor(dev);
        struct tty *tp = gencntty;

        return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}

int
gencnwrite(dev, uio, flag)
        dev_t dev;
        struct uio *uio;
        int flag;
{
        int unit = minor(dev);
        struct tty *tp = gencntty;

        return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

int
gencnioctl(dev, cmd, data, flag, p)
        dev_t dev;
        int cmd;
        caddr_t data;
        int flag;
        struct proc *p;
{
        int error;
        int unit = minor(dev);
        struct tty *tp = gencntty;

        error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag, p);
        if (error >= 0)
                return error;
        error = ttioctl(tp, cmd, data, flag, p);
        if (error >= 0)
 
	return ENOTTY;
}

void
gencnstart(tp)
        struct tty *tp;
{
        struct clist *cl;
        int s, len,i;
        u_char buf[BURST];

	skrivstr("gencnstart\n");
asm("halt");
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

		for(i=0;i<len;i++)
			ocnputc(buf[i]);

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


int
gencnparam(tp, t)
	struct tty *tp;
	struct termios *t;
{
        /* XXX - These are ignored... */
        tp->t_ispeed = t->c_ispeed;
        tp->t_ospeed = t->c_ospeed;
        tp->t_cflag = t->c_cflag;
	return 0;
}

int
gencnprobe(cndev)
	struct	consdev *cndev;
{
	int i;

	for(i=0;i<nchrdev;i++)
		if(cdevsw[i].d_open==gencnopen){
			cndev->cn_dev=makedev(i,0);
			cndev->cn_pri=CN_NORMAL;
			break;
		}
	return 0;
}

int
gencninit(cndev)
	struct	consdev *cndev;
{
/* Nothing yet */
}

int
gencnputc(dev,ch)
	dev_t	dev;
	int	ch;
{
	ocnputc(ch);
}

int
gencngetc(dev)
	dev_t	dev;
{
	return ocngetc();
}


skrivstr(char *ptr){
	int i;

	for(i=0;*(ptr+i);i++)
		ocnputc(*(ptr+i));
}
