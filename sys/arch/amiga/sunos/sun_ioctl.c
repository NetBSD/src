/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)sun_ioctl.c	8.1 (Berkeley) 6/11/93
 *
 * from: Header: sun_ioctl.c,v 1.7 93/05/28 04:40:43 torek Exp 
 * $Id: sun_ioctl.c,v 1.1 1993/10/30 23:42:23 mw Exp $
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/termios.h>
#include <sys/tty.h>

/*
 * SunOS ioctl calls.
 * This file is something of a hodge-podge.
 * Support gets added as things turn up....
 */

struct sun_ttysize {
	int	ts_row;
	int	ts_col;
};

struct sun_termio {
	u_short	c_iflag;
	u_short	c_oflag;
	u_short	c_cflag;
	u_short	c_lflag;
	char	c_line;
	unsigned char c_cc[8];
};
#define SUN_TCGETA	_IOR('T', 1, struct sun_termio)
#define SUN_TCSETA	_IOW('T', 2, struct sun_termio)
#define SUN_TCSETAW	_IOW('T', 3, struct sun_termio)
#define SUN_TCSETAF	_IOW('T', 4, struct sun_termio)
#define SUN_TCSBRK	_IO('T', 5)

struct sun_termios {
	u_long	c_iflag;
	u_long	c_oflag;
	u_long	c_cflag;
	u_long	c_lflag;
	char	c_line;
	u_char	c_cc[17];
};
#define SUN_TCXONC	_IO('T', 6)
#define SUN_TCFLSH	_IO('T', 7)
#define SUN_TCGETS	_IOR('T', 8, struct sun_termios)
#define SUN_TCSETS	_IOW('T', 9, struct sun_termios)
#define SUN_TCSETSW	_IOW('T', 10, struct sun_termios)
#define SUN_TCSETSF	_IOW('T', 11, struct sun_termios)
#define SUN_TCSNDBRK	_IO('T', 12)
#define SUN_TCDRAIN	_IO('T', 13)

static struct speedtab sptab[] = {
  { 0, 0 }, { 50, 1 }, { 75, 2 }, { 110, 3 },
  { 134, 4 }, { 135, 4 }, { 150, 5 }, { 200, 6 },
  { 300, 7 }, { 600, 8 }, { 1200, 9 }, { 1800, 10 },
  { 2400, 11 }, { 4800, 12 }, { 9600, 13 },
  { 19200, 14 }, { 38400, 15 }, { -1, -1 }
};
static u_long s2btab[] = { 
  0, 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800,
  2400, 4800, 9600, 19200, 38400,
};

/* these two conversion functions have mostly been done
   with some perl cut&paste, then handedited to comment
   out what doesn't exist under NetBSD.
   Note: (l & BITMASK1) / BITMASK1 * BITMASK2
         is translated optimally by gcc m68k, much better
	 than any ?: stuff. Code may vary with different 
	 architectures of course. */

static void
stios2btios (st, bt)
     struct sun_termios *st;
     struct termios *bt;
{
  register u_long l;

  l = st->c_iflag;
  bt->c_iflag = ( ((l & 0x00000001) / 0x00000001 * IGNBRK)
		 |((l & 0x00000002) / 0x00000002 * BRKINT)
		 |((l & 0x00000004) / 0x00000004 * IGNPAR)
		 |((l & 0x00000008) / 0x00000008 * PARMRK)
		 |((l & 0x00000010) / 0x00000010 * INPCK)
		 |((l & 0x00000020) / 0x00000020 * ISTRIP)
		 |((l & 0x00000040) / 0x00000040 * INLCR)
		 |((l & 0x00000080) / 0x00000080 * IGNCR)
		 |((l & 0x00000100) / 0x00000100 * ICRNL)
		 /*|((l & 0x00000200) / 0x00000200 * IUCLC)*/
		 |((l & 0x00000400) / 0x00000400 * IXON)
		 |((l & 0x00000800) / 0x00000800 * IXANY)
		 |((l & 0x00001000) / 0x00001000 * IXOFF)
		 |((l & 0x00002000) / 0x00002000 * IMAXBEL));

  l = st->c_oflag;
  bt->c_oflag = ( ((l & 0x00000001) / 0x00000001 * OPOST)
		 /*|((l & 0x00000002) / 0x00000002 * OLCUC)*/
		 |((l & 0x00000004) / 0x00000004 * ONLCR)
		 /*|((l & 0x00000008) / 0x00000008 * OCRNL)*/
		 /*|((l & 0x00000010) / 0x00000010 * ONOCR)*/
		 /*|((l & 0x00000020) / 0x00000020 * ONLRET)*/
		 /*|((l & 0x00000040) / 0x00000040 * OFILL)*/
		 /*|((l & 0x00000080) / 0x00000080 * OFDEL)*/
		 /*|((l & 0x00000100) / 0x00000100 * NLDLY)*/
		 /*|((l & 0x00000100) / 0x00000100 * NL1)*/
		 /*|((l & 0x00000600) / 0x00000600 * CRDLY)*/
		 /*|((l & 0x00000200) / 0x00000200 * CR1)*/
		 /*|((l & 0x00000400) / 0x00000400 * CR2)*/
		 /*|((l & 0x00000600) / 0x00000600 * CR3)*/
		 /*|((l & 0x00001800) / 0x00001800 * TABDLY)*/
		 /*|((l & 0x00000800) / 0x00000800 * TAB1)*/
		 /*|((l & 0x00001000) / 0x00001000 * TAB2)*/
		 |((l & 0x00001800) / 0x00001800 * OXTABS)
		 /*|((l & 0x00002000) / 0x00002000 * BSDLY)*/
		 /*|((l & 0x00002000) / 0x00002000 * BS1)*/
		 /*|((l & 0x00004000) / 0x00004000 * VTDLY)*/
		 /*|((l & 0x00004000) / 0x00004000 * VT1)*/
		 /*|((l & 0x00008000) / 0x00008000 * FFDLY)*/
		 /*|((l & 0x00008000) / 0x00008000 * FF1)*/
		 /*|((l & 0x00010000) / 0x00010000 * PAGEOUT)*/
		 /*|((l & 0x00020000) / 0x00020000 * WRAP)*/);

  l = st->c_cflag;
  bt->c_cflag = ( ((l & 0x00000010) / 0x00000010 * CS6)
		 |((l & 0x00000020) / 0x00000020 * CS7)
		 |((l & 0x00000030) / 0x00000030 * CS8)
		 |((l & 0x00000040) / 0x00000040 * CSTOPB)
		 |((l & 0x00000080) / 0x00000080 * CREAD)
		 |((l & 0x00000100) / 0x00000100 * PARENB)
		 |((l & 0x00000200) / 0x00000200 * PARODD)
		 |((l & 0x00000400) / 0x00000400 * HUPCL)
		 |((l & 0x00000800) / 0x00000800 * CLOCAL)
		 /*|((l & 0x00001000) / 0x00001000 * LOBLK)*/
		 |((l & 0x80000000) / 0x80000000 * (CRTS_IFLOW|CCTS_OFLOW)));

  bt->c_ispeed = bt->c_ospeed = s2btab[l & 0x0000000f];
  
  l = st->c_lflag;
  bt->c_lflag = ( ((l & 0x00000001) / 0x00000001 * ISIG)
		 |((l & 0x00000002) / 0x00000002 * ICANON)
		 /*|((l & 0x00000004) / 0x00000004 * XCASE)*/
		 |((l & 0x00000008) / 0x00000008 * ECHO)
		 |((l & 0x00000010) / 0x00000010 * ECHOE)
		 |((l & 0x00000020) / 0x00000020 * ECHOK)
		 |((l & 0x00000040) / 0x00000040 * ECHONL)
		 |((l & 0x00000080) / 0x00000080 * NOFLSH)
		 |((l & 0x00000100) / 0x00000100 * TOSTOP)
		 |((l & 0x00000200) / 0x00000200 * ECHOCTL)
		 |((l & 0x00000400) / 0x00000400 * ECHOPRT)
		 |((l & 0x00000800) / 0x00000800 * ECHOKE)
		 /*|((l & 0x00001000) / 0x00001000 * DEFECHO)*/
		 |((l & 0x00002000) / 0x00002000 * FLUSHO)
		 |((l & 0x00004000) / 0x00004000 * PENDIN));

  bt->c_cc[VINTR]    = st->c_cc[0]  ? st->c_cc[0]  : _POSIX_VDISABLE;
  bt->c_cc[VQUIT]    = st->c_cc[1]  ? st->c_cc[1]  : _POSIX_VDISABLE;
  bt->c_cc[VERASE]   = st->c_cc[2]  ? st->c_cc[2]  : _POSIX_VDISABLE;
  bt->c_cc[VKILL]    = st->c_cc[3]  ? st->c_cc[3]  : _POSIX_VDISABLE;
  bt->c_cc[VEOF]     = st->c_cc[4]  ? st->c_cc[4]  : _POSIX_VDISABLE;
  bt->c_cc[VEOL]     = st->c_cc[5]  ? st->c_cc[5]  : _POSIX_VDISABLE;
  bt->c_cc[VEOL2]    = st->c_cc[6]  ? st->c_cc[6]  : _POSIX_VDISABLE;
  /*bt->c_cc[VSWTCH]   = st->c_cc[7]  ? st->c_cc[7]  : _POSIX_VDISABLE;*/
  bt->c_cc[VSTART]   = st->c_cc[8]  ? st->c_cc[8]  : _POSIX_VDISABLE;
  bt->c_cc[VSTOP]    = st->c_cc[9]  ? st->c_cc[9]  : _POSIX_VDISABLE;
  bt->c_cc[VSUSP]    = st->c_cc[10] ? st->c_cc[10] : _POSIX_VDISABLE;
  bt->c_cc[VDSUSP]   = st->c_cc[11] ? st->c_cc[11] : _POSIX_VDISABLE;
  bt->c_cc[VREPRINT] = st->c_cc[12] ? st->c_cc[12] : _POSIX_VDISABLE;
  bt->c_cc[VDISCARD] = st->c_cc[13] ? st->c_cc[13] : _POSIX_VDISABLE;
  bt->c_cc[VWERASE]  = st->c_cc[14] ? st->c_cc[14] : _POSIX_VDISABLE;
  bt->c_cc[VLNEXT]   = st->c_cc[15] ? st->c_cc[15] : _POSIX_VDISABLE;
  bt->c_cc[VSTATUS]  = st->c_cc[16] ? st->c_cc[16] : _POSIX_VDISABLE;
}


static void
btios2stios (bt, st)
     struct termios *bt;
     struct sun_termios *st;
{
  register u_long l;

  l = bt->c_iflag;
  st->c_iflag = ( ((l &  IGNBRK) /  IGNBRK * 0x00000001)
		 |((l &  BRKINT) /  BRKINT * 0x00000002)
		 |((l &  IGNPAR) /  IGNPAR * 0x00000004)
		 |((l &  PARMRK) /  PARMRK * 0x00000008)
		 |((l &   INPCK) /   INPCK * 0x00000010)
		 |((l &  ISTRIP) /  ISTRIP * 0x00000020)
		 |((l &   INLCR) /   INLCR * 0x00000040)
		 |((l &   IGNCR) /   IGNCR * 0x00000080)
		 |((l &   ICRNL) /   ICRNL * 0x00000100)
		 /*|((l &   IUCLC) /   IUCLC * 0x00000200)*/
		 |((l &    IXON) /    IXON * 0x00000400)
		 |((l &   IXANY) /   IXANY * 0x00000800)
		 |((l &   IXOFF) /   IXOFF * 0x00001000)
		 |((l & IMAXBEL) / IMAXBEL * 0x00002000));

  l = bt->c_oflag;
  st->c_oflag = ( ((l &   OPOST) /   OPOST * 0x00000001)
		 /*|((l &   OLCUC) /   OLCUC * 0x00000002)*/
		 |((l &   ONLCR) /   ONLCR * 0x00000004)
		 /*|((l &   OCRNL) /   OCRNL * 0x00000008)*/
		 /*|((l &   ONOCR) /   ONOCR * 0x00000010)*/
		 /*|((l &  ONLRET) /  ONLRET * 0x00000020)*/
		 /*|((l &   OFILL) /   OFILL * 0x00000040)*/
		 /*|((l &   OFDEL) /   OFDEL * 0x00000080)*/
		 /*|((l &   NLDLY) /   NLDLY * 0x00000100)*/
		 /*|((l &     NL1) /     NL1 * 0x00000100)*/
		 /*|((l &   CRDLY) /   CRDLY * 0x00000600)*/
		 /*|((l &     CR1) /     CR1 * 0x00000200)*/
		 /*|((l &     CR2) /     CR2 * 0x00000400)*/
		 /*|((l &     CR3) /     CR3 * 0x00000600)*/
		 /*|((l &  TABDLY) /  TABDLY * 0x00001800)*/
		 /*|((l &    TAB1) /    TAB1 * 0x00000800)*/
		 /*|((l &    TAB2) /    TAB2 * 0x00001000)*/
		 |((l &  OXTABS) /  OXTABS * 0x00001800)
		 /*|((l &   BSDLY) /   BSDLY * 0x00002000)*/
		 /*|((l &     BS1) /     BS1 * 0x00002000)*/
		 /*|((l &   VTDLY) /   VTDLY * 0x00004000)*/
		 /*|((l &     VT1) /     VT1 * 0x00004000)*/
		 /*|((l &   FFDLY) /   FFDLY * 0x00008000)*/
		 /*|((l &     FF1) /     FF1 * 0x00008000)*/
		 /*|((l & PAGEOUT) / PAGEOUT * 0x00010000)*/
		 /*|((l &    WRAP) /    WRAP * 0x00020000)*/);

  l = bt->c_cflag;
  st->c_cflag = ( ((l &     CS6) /     CS6 * 0x00000010)
		 |((l &     CS7) /     CS7 * 0x00000020)
		 |((l &     CS8) /     CS8 * 0x00000030)
		 |((l &  CSTOPB) /  CSTOPB * 0x00000040)
		 |((l &   CREAD) /   CREAD * 0x00000080)
		 |((l &  PARENB) /  PARENB * 0x00000100)
		 |((l &  PARODD) /  PARODD * 0x00000200)
		 |((l &   HUPCL) /   HUPCL * 0x00000400)
		 |((l &  CLOCAL) /  CLOCAL * 0x00000800)
		 /*|((l &   LOBLK) /   LOBLK * 0x00001000)*/
		 |((l & (CRTS_IFLOW|CCTS_OFLOW)) / (CRTS_IFLOW|CCTS_OFLOW) * 0x80000000));

  l = bt->c_lflag;
  st->c_lflag = ( ((l &    ISIG) /    ISIG * 0x00000001)
		 |((l &  ICANON) /  ICANON * 0x00000002)
		 /*|((l &   XCASE) /   XCASE * 0x00000004)*/
		 |((l &    ECHO) /    ECHO * 0x00000008)
		 |((l &   ECHOE) /   ECHOE * 0x00000010)
		 |((l &   ECHOK) /   ECHOK * 0x00000020)
		 |((l &  ECHONL) /  ECHONL * 0x00000040)
		 |((l &  NOFLSH) /  NOFLSH * 0x00000080)
		 |((l &  TOSTOP) /  TOSTOP * 0x00000100)
		 |((l & ECHOCTL) / ECHOCTL * 0x00000200)
		 |((l & ECHOPRT) / ECHOPRT * 0x00000400)
		 |((l &  ECHOKE) /  ECHOKE * 0x00000800)
		 /*|((l & DEFECHO) / DEFECHO * 0x00001000)*/
		 |((l &  FLUSHO) /  FLUSHO * 0x00002000)
		 |((l &  PENDIN) /  PENDIN * 0x00004000));

  l = ttspeedtab (bt->c_ospeed, sptab);
  if (l >= 0)
    st->c_cflag |= l;

  st->c_cc[0]  = bt->c_cc[VINTR]    != _POSIX_VDISABLE ? bt->c_cc[VINTR]    : 0;
  st->c_cc[1]  = bt->c_cc[VQUIT]    != _POSIX_VDISABLE ? bt->c_cc[VQUIT]    : 0;
  st->c_cc[2]  = bt->c_cc[VERASE]   != _POSIX_VDISABLE ? bt->c_cc[VERASE]   : 0;
  st->c_cc[3]  = bt->c_cc[VKILL]    != _POSIX_VDISABLE ? bt->c_cc[VKILL]    : 0;
  st->c_cc[4]  = bt->c_cc[VEOF]     != _POSIX_VDISABLE ? bt->c_cc[VEOF]     : 0;
  st->c_cc[5]  = bt->c_cc[VEOL]     != _POSIX_VDISABLE ? bt->c_cc[VEOL]     : 0;
  st->c_cc[6]  = bt->c_cc[VEOL2]    != _POSIX_VDISABLE ? bt->c_cc[VEOL2]    : 0;
  st->c_cc[7]  = /*bt->c_cc[VSWTCH]   != _POSIX_VDISABLE ? bt->c_cc[VSWTCH]   :*/ 0;
  st->c_cc[8]  = bt->c_cc[VSTART]   != _POSIX_VDISABLE ? bt->c_cc[VSTART]   : 0;
  st->c_cc[9]  = bt->c_cc[VSTOP]    != _POSIX_VDISABLE ? bt->c_cc[VSTOP]    : 0;
  st->c_cc[10] = bt->c_cc[VSUSP]    != _POSIX_VDISABLE ? bt->c_cc[VSUSP]    : 0;
  st->c_cc[11] = bt->c_cc[VDSUSP]   != _POSIX_VDISABLE ? bt->c_cc[VDSUSP]   : 0;
  st->c_cc[12] = bt->c_cc[VREPRINT] != _POSIX_VDISABLE ? bt->c_cc[VREPRINT] : 0;
  st->c_cc[13] = bt->c_cc[VDISCARD] != _POSIX_VDISABLE ? bt->c_cc[VDISCARD] : 0;
  st->c_cc[14] = bt->c_cc[VWERASE]  != _POSIX_VDISABLE ? bt->c_cc[VWERASE]  : 0;
  st->c_cc[15] = bt->c_cc[VLNEXT]   != _POSIX_VDISABLE ? bt->c_cc[VLNEXT]   : 0;
  st->c_cc[16] = bt->c_cc[VSTATUS]  != _POSIX_VDISABLE ? bt->c_cc[VSTATUS]  : 0;

  st->c_line = 0;
}

static void
stios2stio (ts, t)
     struct sun_termios *ts;
     struct sun_termio *t;
{
  t->c_iflag = ts->c_iflag;
  t->c_oflag = ts->c_oflag;
  t->c_cflag = ts->c_cflag;
  t->c_lflag = ts->c_lflag;
  t->c_line  = ts->c_line;
  bcopy (ts->c_cc, t->c_cc, 8);
}

static void
stio2stios (t, ts)
     struct sun_termio *t;
     struct sun_termios *ts;
{
  ts->c_iflag = t->c_iflag;
  ts->c_oflag = t->c_oflag;
  ts->c_cflag = t->c_cflag;
  ts->c_lflag = t->c_lflag;
  ts->c_line  = t->c_line;
  bcopy (t->c_cc, ts->c_cc, 8); /* don't touch the upper fields! */
}



struct sun_ioctl_args {
	int	fd;
	int	cmd;
	caddr_t	data;
};

int
sun_ioctl(p, uap, retval)
     register struct proc *p;
     register struct sun_ioctl_args *uap;
     int *retval;
{
  register struct filedesc *fdp = p->p_fd;
  register struct file *fp;
  register int (*ctl)();
  int error;

  if ((unsigned)uap->fd >= fdp->fd_nfiles 
      || (fp = fdp->fd_ofiles[uap->fd]) == NULL)
    return (EBADF);

  if ((fp->f_flag & (FREAD|FWRITE)) == 0)
    return (EBADF);

  ctl = fp->f_ops->fo_ioctl;

  switch (uap->cmd) 
    {
    case _IOR('t', 0, int):
      uap->cmd = TIOCGETD;
      break;
      
    case _IOW('t', 1, int):
      {
	int disc;

	if ((error = copyin (uap->data, (caddr_t) &disc, sizeof (disc))) != 0)
	  return error;

	/* map SunOS NTTYDISC into our termios discipline */
	if (disc == 2)
	  disc = 0;
	/* all other disciplines are not supported by NetBSD */
	if (disc)
	  return ENXIO;

	return (*ctl)(fp, TIOCSETD, (caddr_t) &disc, p);
      }
      
    case _IO('t', 36): 		/* sun TIOCCONS, no parameters */
      {
	int on = 1;
	return (*ctl)(fp, TIOCCONS, (caddr_t)&on, p);
      }
      
    case _IOW('t', 37, struct sun_ttysize): 
      {
	struct winsize ws;
	struct sun_ttysize ss;

	if ((error = (*ctl)(fp, TIOCGWINSZ, (caddr_t)&ws, p)) != 0)
	  return (error);

	if ((error = copyin (uap->data, &ss, sizeof (ss))) != 0)
	  return error;

	ws.ws_row = ss.ts_row;
	ws.ws_col = ss.ts_col;

	return ((*ctl)(fp, TIOCSWINSZ, (caddr_t)&ws, p));
      }
      
    case _IOW('t', 38, struct sun_ttysize): 
      {
	struct winsize ws;
	struct sun_ttysize ss;

	if ((error = (*ctl)(fp, TIOCGWINSZ, (caddr_t) &ws, p)) != 0)
	  return (error);

	ss.ts_row = ws.ws_row;
	ss.ts_col = ws.ws_col;

	return copyout ((caddr_t) &ss, uap->data, sizeof (ss));
      }

    case _IOR('t', 130, int):
      uap->cmd = TIOCSPGRP;
      break;

    case _IOR('t', 131, int):
      uap->cmd = TIOCGPGRP;
      break;
      
    case _IO('t', 132):
      uap->cmd = TIOCSCTTY;
      break;
      
    case SUN_TCGETA:
    case SUN_TCGETS: 
      {
	struct termios bts;
	struct sun_termios sts;
	struct sun_termio st;
	
	if ((error = (*ctl)(fp, TIOCGETA, (caddr_t) &bts, p)) != 0)
	  return error;
	
	btios2stios (&bts, &sts);
	if (uap->cmd == SUN_TCGETA)
	  {
	    stios2stio (&sts, &st);
	    return copyout ((caddr_t) &st, uap->data, sizeof (st));
	  }
	else
	  return copyout ((caddr_t) &sts, uap->data, sizeof (sts));
	/* not reached */
	
      }
      
    case SUN_TCSETA:
    case SUN_TCSETAW:
    case SUN_TCSETAF:
      {
	struct termios bts;
	struct sun_termios sts;
	struct sun_termio st;

	if ((error = copyin (uap->data, (caddr_t) &st, sizeof (st))) != 0)
	  return error;

	/* get full BSD termios so we don't lose information */
	if ((error = (*ctl)(fp, TIOCGETA, (caddr_t) &bts, p)) != 0)
	  return error;

	/* convert to sun termios, copy in information from termio, and 
	   convert back, then set new values. */
	btios2stios (&bts, &sts);
	stio2stios (&st, &sts);
	stios2btios (&sts, &bts);

	return (*ctl)(fp, uap->cmd - SUN_TCSETA + TIOCSETA, (caddr_t) &bts, p);
      }

    case SUN_TCSETS:
    case SUN_TCSETSW:
    case SUN_TCSETSF:
      {
	struct termios bts;
	struct sun_termios sts;

	if ((error = copyin (uap->data, (caddr_t) &sts, sizeof (sts))) != 0)
	  return error;

	stios2btios (&sts, &bts);

	return (*ctl)(fp, uap->cmd - SUN_TCSETS + TIOCSETA, (caddr_t) &bts, p);
      }
      
    }
     
  return (ioctl(p, uap, retval));
}
