/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: ite.c 1.28 92/12/20$
 *
 *	from: @(#)ite.c	8.2 (Berkeley) 1/12/94
 *	$Id: ite.c,v 1.1 1994/07/08 07:55:52 lkestel Exp $
 */

/*
 * ite.c
 *
 * $Id: ite.c,v 1.1 1994/07/08 07:55:52 lkestel Exp $
 *
 * The ite module handles the system console; that is, stuff printed
 * by the kernel and by user programs while "desktop" and X aren't
 * running.  Some parts are based on hp300's 4.4 ite.c, hence the above
 * copyright.
 *
 *   -- Brad and Lawrence, June 26th, 1994
 *
 */

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include "../mac68k/cons.h"
#include "../mac68k/via.h"
#include <machine/frame.h>

#include "keyboard.h"
#include "adbsys.h"

#include "6x10.h"
#define CHARWIDTH	6
#define CHARHEIGHT	10

#define dprintf if (0) printf

/* Received from MacBSDBoot, stored by Locore: */
long		videoaddr;
long		videorowbytes;
long		videobitdepth;
unsigned long	videosize;
char		serial_boot_echo = 0;

/* Calculated in itecninit(): */
static int	width, height, scrrows, scrcols;

/* Cursor location: */
static int	x = 0, y = 0;

/* Our tty: */
struct tty	*ite_tty;

/* For polled ADB mode: */
static int polledkey;
extern int adb_polling;

/* Misc */
void	itestart();

/*
 * Bitmap handling functions
 */

static void putpixel(int xx, int yy, int c)
{
	unsigned int	mask, i;
	unsigned char	*sc;

	sc = (unsigned char *)videoaddr;

	mask = (1 << videobitdepth) - 1;
	c &= mask; /* Not right -- should shift */
	switch (videobitdepth) {
		case 1: i = 7 - (xx & 7);
			c <<= i;
			mask <<= i;
			i = yy * videorowbytes + (xx >> 3);
			sc[i] &= ~mask;
			sc[i] |= c;
			break;
		case 2: i = 6 - ((xx & 3) << 1);
			c <<= i;
			mask <<= i;
			i = yy * videorowbytes + (xx >> 2);
			sc[i] &= ~mask;
			sc[i] |= c;
			break;
		case 4: i = 4 - ((xx & 1) << 2);
			c <<= i;
			mask <<= i;
			i = yy * videorowbytes + (xx >> 1);
			sc[i] &= ~mask;
			sc[i] |= c;
			break;
		case 8: sc[yy * videorowbytes + xx] = c; 
			break;
	}
}

static void writechar (char ch, int x, int y)
{
	int		i, j, mask;
	unsigned char	*c;

	ch &= 0x7F;
	x *= CHARWIDTH;
	y *= CHARHEIGHT;

	c = &Font6x10[ch * CHARHEIGHT];

	for (i = 0; i < CHARHEIGHT; i++) {
		mask = 1 << (CHARWIDTH - 1);
		for (j = 0; j < CHARWIDTH; j++) {
			if (*c & mask) {
				putpixel (x + j, y + i, 255);
			} else {
				putpixel (x + j, y + i, 0);
			}
			mask >>= 1;
		}
		c++;
	}
}

static void drawcursor (void)
{
	int	i, j, X, Y;

	X = x * CHARWIDTH;
	Y = y * CHARHEIGHT;

	for (i = 0; i < CHARHEIGHT; i++) {
		for (j = 0; j < CHARWIDTH; j++) {
			putpixel (X + j, Y + i, 255);
		}
	}
}

static void erasecursor (void)
{
	int	i, j, X, Y;

	X = x * CHARWIDTH;
	Y = y * CHARHEIGHT;

	for (i = 0; i < CHARHEIGHT; i++) {
		for (j = 0; j < CHARWIDTH; j++) {
			putpixel (X + j, Y + i, 0);
		}
	}
}

static void scrollup (void)
{
	unsigned long	*from, *to;
	int		i;

	to = (unsigned long *)videoaddr;
	from = to + videorowbytes * CHARHEIGHT / 4;

	for (i = (scrrows - 1) * videorowbytes * CHARHEIGHT / 4; i > 0; i--) {
		*to++ = *from++;
	}
	for (i = videorowbytes * CHARHEIGHT / 4; i > 0; i--) {
		*to++ = 0;
	}
}

static void clearscreen (void)
{
	unsigned long	*p;
	int		i;

	p = (unsigned long *)videoaddr;

	for (i = scrrows * videorowbytes * CHARHEIGHT / 4; i > 0; i--) {
		*p++ = 0;
	}
}

static void ite_putchar (char ch)
{
	switch (ch) {
		case 7: /* Beep */
			break;
		case 127:			/* Delete		*/
		case 8: if (x > 0) {		/* Backspace		*/
				x--;
			}
			break;
		case 9:	do {		 	/* Tab			*/
				ite_putchar (' ');
			} while (x % 8 != 0);
			break;
		case 10: y++;			/* Line feed		*/
			break;
		case 13: x = 0;			/* Carriage return	*/
			break;
		default:
			if (ch >= ' ') {
				writechar (ch, x, y);
				x++;
			}
			break;
	}
	if (x >= scrcols) {
		x = 0;
		y++;
	}
	if (y >= scrrows) {
		scrollup ();
		y--;
	}
}

/*
 * Keyboard support functions
 */

static int ite_dopollkey(int key)
{
	polledkey = key;

	return 0;
}


static int ite_pollforchar(void)
{
	int		s;
	register int	intbits;

	s = splhigh();

	polledkey = -1;
	adb_polling = 1;

	/* pretend we're VIA interrupt dispatcher */
	while (polledkey == -1) {
		intbits = via_reg(VIA1, vIFR);

		if (intbits & V1IF_ADBRDY) {
			adb_intr();
			via_reg(VIA1, vIFR) = V1IF_ADBRDY;
		}
	}

	adb_polling = 0;

	return polledkey;
}

/*
 * Tty handling functions
 */

static void iteattach (struct device *parent, struct device *dev, void *aux)
{
	printf (" (minimal console)\n");
}

extern int matchbyname();

struct cfdriver itecd =
      { NULL,
	"ite",
	matchbyname,
	iteattach,
	DV_TTY,
	sizeof(struct device),
	NULL,
	0 };

iteopen(dev_t dev, int mode, int devtype, struct proc *p)
{
	register struct tty *tp;
	register int error;
	int first = 0;

	dprintf ("iteopen(): enter(0x%x)\n",(int)dev);
	if (ite_tty == NULL)
		tp = ite_tty = ttymalloc();
	else
		tp = ite_tty;
	if ((tp->t_state&(TS_ISOPEN|TS_XCLUDE)) == (TS_ISOPEN|TS_XCLUDE)
	    && p->p_ucred->cr_uid != 0)
		return (EBUSY);
	tp->t_oproc = itestart;
	tp->t_param = NULL;
	tp->t_dev = dev;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = CS8|CREAD;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		tp->t_state = TS_ISOPEN|TS_CARR_ON;
		ttsetwater(tp);
	}
	error = (*linesw[tp->t_line].l_open)(dev, tp);
	tp->t_winsize.ws_row = scrrows;
	tp->t_winsize.ws_col = scrcols;
	dprintf ("iteopen(): exit(%d)\n", error);
	return (error);
}

iteclose(dev_t dev, int flag, int mode, struct proc *p)
{
	dprintf ("iteclose: enter (%d)\n", (int)dev);
	(*linesw[ite_tty->t_line].l_close)(ite_tty, flag);
	ttyclose (ite_tty);
#if 0
	ttyfree (ite_tty);
	ite_tty = (struct tty *)0;
#endif
	dprintf ("iteclose: exit\n");
	return 0;
}

iteread(dev_t dev, struct uio *uio, int flag)
{
	dprintf ("iteread: enter\n");
	return (*linesw[ite_tty->t_line].l_read)(ite_tty, uio, flag);
	dprintf ("iteread: exit\n");
}

itewrite(dev_t dev, struct uio *uio, int flag)
{
	dprintf ("itewrite: enter\n");
	return (*linesw[ite_tty->t_line].l_write)(ite_tty, uio, flag);
	dprintf ("itewrite: exit\n");
}

iteioctl(dev_t dev, int cmd, caddr_t addr, int flag, struct proc *p)
{
	register struct tty *tp = ite_tty;
	int error;

	dprintf ("iteioctl: enter(%d, 0x%x)\n", cmd, cmd);
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, addr, flag, p);
	if (error >= 0) {
		dprintf ("iteioctl: exit(%d)\n", error);
		return (error);
	}
	error = ttioctl(tp, cmd, addr, flag, p);
	if (error >= 0) {
		dprintf ("iteioctl: exit(%d)\n", error);
		return (error);
	}
	dprintf ("iteioctl: exit(ENOTTY)\n");
	return (ENOTTY);
}

void itestart (register struct tty *tp)
{
	register int cc, s;
	int hiwat = 0, hadcursor = 0;

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP)) {
		splx(s);
		return;
	}
	tp->t_state |= TS_BUSY;

	cc = tp->t_outq.c_cc;
	erasecursor ();
	while (cc-- > 0) {
		ite_putchar (getc (&tp->t_outq));
	}
	drawcursor ();

	tp->t_state &= ~TS_BUSY;
	splx(s);
}

void itestop (struct tty *tp, int flag)
{
	int	s;

	s = spltty ();
	if (tp->t_state & TS_BUSY) {
		if ((tp->t_state & TS_TTSTOP) == 0) {
			tp->t_state |= TS_FLUSH;
		}
	}
	splx(s);
}

ite_intr (adb_event_t *event)
{
	static	int	shift = 0, control = 0;
	int		key, press, val, state, ch;

	key = event->u.k.key;
	press = ADBK_PRESS (key);
	val = ADBK_KEYVAL (key);

	if (val == ADBK_SHIFT) {
		shift = press;
	} else if (val == ADBK_CONTROL) {
		control = press;
	} else if (press) {
		state = 0;
		if (shift) {
			state = 1;
		}
		if (control) {
			state = 2;
		}
		ch = keyboard[val][state];
		if (ch != '\0') {
			if (adb_polling) {
				polledkey = ch;
			} else {
				(*linesw[ite_tty->t_line].l_rint)(ch, ite_tty);
			}
		}
	}
}

/*
 * Console functions
 */

itecnprobe(struct consdev *cp)
{
	int maj, unit;

	/* locate the major number */
	for (maj = 0; maj < nchrdev; maj++) {
		if (cdevsw[maj].d_open == iteopen) {
			break;
		}
	}

	if (maj == nchrdev) {
		panic ("itecnprobe(): did not find iteopen().");
	}

	cdevsw[maj].d_ttys = &ite_tty;

	unit = 0;

	/* initialize required fields */
	cp->cn_dev = makedev (maj, unit);
	cp->cn_tp = ite_tty;
	cp->cn_pri = CN_INTERNAL;
}

itecninit(struct consdev *cp)
{
	width = videosize & 0xffff;
	height = (videosize >> 16) & 0xffff;
	scrrows = height / CHARHEIGHT;
	scrcols = width / CHARWIDTH;

	clearscreen ();
}

itecngetc(dev_t dev)
{
	/* Oh, man... */

	return ite_pollforchar ();
}

itecnputc(dev_t dev, int c)
{
	int s;

	if (serial_boot_echo) {
		/* Serial delay is automatic */
#if 0
		macserputchar ((unsigned char)c);
#endif
	}

	s = splhigh ();

	erasecursor ();
	ite_putchar (c);
	drawcursor ();

	splx (s);
}
