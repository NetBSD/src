/*	$NetBSD: ultrix_ioctl.c,v 1.29.8.1 2007/07/11 20:04:51 mjf Exp $ */
/*	from : NetBSD: sunos_ioctl.c,v 1.21 1995/10/07 06:27:31 mycroft Exp */

/*
 * Copyright (c) 1993 Markus Wild.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
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
 * loosely from: Header: sunos_ioctl.c,v 1.7 93/05/28 04:40:43 torek Exp
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ultrix_ioctl.c,v 1.29.8.1 2007/07/11 20:04:51 mjf Exp $");

#if defined(_KERNEL_OPT)
#include "opt_compat_ultrix.h"
#include "opt_compat_sunos.h"
#endif

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/termios.h>
#include <sys/tty.h>
#include <sys/socket.h>
#include <sys/audioio.h>
#include <net/if.h>

#include <sys/mount.h>

#include <compat/sys/sockio.h>
#include <compat/ultrix/ultrix_syscallargs.h>
#include <sys/syscallargs.h>

#include <compat/sunos/sunos.h>

#include <compat/ultrix/ultrix_tty.h>

#define emul_termio	ultrix_termio
#define emul_termios	ultrix_termios

/*
 * SunOS ioctl calls.
 * This file is something of a hodge-podge.
 * Support gets added as things turn up....
 */

static const struct speedtab sptab[] = {
	{ 0, 0 },
	{ 50, 1 },
	{ 75, 2 },
	{ 110, 3 },
	{ 134, 4 },
	{ 135, 4 },
	{ 150, 5 },
	{ 200, 6 },
	{ 300, 7 },
	{ 600, 8 },
	{ 1200, 9 },
	{ 1800, 10 },
	{ 2400, 11 },
	{ 4800, 12 },
	{ 9600, 13 },
	{ 19200, 14 },
	{ 38400, 15 },
	{ -1, -1 }
};

static const u_long s2btab[] = {
	0,
	50,
	75,
	110,
	134,
	150,
	200,
	300,
	600,
	1200,
	1800,
	2400,
	4800,
	9600,
	19200,
	38400,
};


/*
 * Translate a single tty control char from the emulation value
 * to native termios, and vice-versa. Special-case
 * the value of POSIX_VDISABLE, mapping it to and from 0.
 */
#define NATIVE_TO_EMUL_CC(bsd_cc) \
 (((bsd_cc)   != _POSIX_VDISABLE) ? (bsd_cc) : 0)

#define EMUL_TO_NATIVE_CC(emul_cc) \
 (emul_cc) ? (emul_cc) : _POSIX_VDISABLE;


static void stios2btios(struct emul_termios *, struct termios *);
static void btios2stios(struct termios *, struct emul_termios *);
static void stios2stio(struct emul_termios *, struct emul_termio *);
static void stio2stios(struct emul_termio *, struct emul_termios *);

/*
 * these two conversion functions have mostly been done
 * with some perl cut&paste, then handedited to comment
 * out what doesn't exist under NetBSD.
 * A note from Markus's code:
 *	(l & BITMASK1) / BITMASK1 * BITMASK2  is translated
 *	optimally by gcc m68k, much better than any ?: stuff.
 *	Code may vary with different architectures of course.
 *
 * I don't know what optimizer you used, but seeing divu's and
 * bfextu's in the m68k assembly output did not encourage me...
 * as well, gcc on the sparc definately generates much better
 * code with ?:.
 */


static void
stios2btios(struct emul_termios *st, struct termios *bt)
{
	u_long l, r;

	l = st->c_iflag;
	r = 	((l & 0x00000001) ? IGNBRK	: 0);
	r |=	((l & 0x00000002) ? BRKINT	: 0);
	r |=	((l & 0x00000004) ? IGNPAR	: 0);
	r |=	((l & 0x00000008) ? PARMRK	: 0);
	r |=	((l & 0x00000010) ? INPCK	: 0);
	r |=	((l & 0x00000020) ? ISTRIP	: 0);
	r |= 	((l & 0x00000040) ? INLCR	: 0);
	r |=	((l & 0x00000080) ? IGNCR	: 0);
	r |=	((l & 0x00000100) ? ICRNL	: 0);
	/*	((l & 0x00000200) ? IUCLC	: 0) */
	r |=	((l & 0x00000400) ? IXON	: 0);
	r |=	((l & 0x00000800) ? IXANY	: 0);
	r |=	((l & 0x00001000) ? IXOFF	: 0);
	r |=	((l & 0x00002000) ? IMAXBEL	: 0);
	bt->c_iflag = r;

	l = st->c_oflag;
	r = 	((l & 0x00000001) ? OPOST	: 0);
	/*	((l & 0x00000002) ? OLCUC	: 0) */
	r |=	((l & 0x00000004) ? ONLCR	: 0);
	/*	((l & 0x00000008) ? OCRNL	: 0) */
	/*	((l & 0x00000010) ? ONOCR	: 0) */
	/*	((l & 0x00000020) ? ONLRET	: 0) */
	/*	((l & 0x00000040) ? OFILL	: 0) */
	/*	((l & 0x00000080) ? OFDEL	: 0) */
	/*	((l & 0x00000100) ? NLDLY	: 0) */
	/*	((l & 0x00000100) ? NL1		: 0) */
	/*	((l & 0x00000600) ? CRDLY	: 0) */
	/*	((l & 0x00000200) ? CR1		: 0) */
	/*	((l & 0x00000400) ? CR2		: 0) */
	/*	((l & 0x00000600) ? CR3		: 0) */
	/*	((l & 0x00001800) ? TABDLY	: 0) */
	/*	((l & 0x00000800) ? TAB1	: 0) */
	/*	((l & 0x00001000) ? TAB2	: 0) */
	r |=	((l & 0x00001800) ? OXTABS	: 0);
	/*	((l & 0x00002000) ? BSDLY	: 0) */
	/*	((l & 0x00002000) ? BS1		: 0) */
	/*	((l & 0x00004000) ? VTDLY	: 0) */
	/*	((l & 0x00004000) ? VT1		: 0) */
	/*	((l & 0x00008000) ? FFDLY	: 0) */
	/*	((l & 0x00008000) ? FF1		: 0) */
	/*	((l & 0x00010000) ? PAGEOUT	: 0) */
	/*	((l & 0x00020000) ? WRAP	: 0) */
	bt->c_oflag = r;

	l = st->c_cflag;
	switch (l & 0x00000030) {
	case 0:
		r = CS5;
		break;
	case 0x00000010:
		r = CS6;
		break;
	case 0x00000020:
		r = CS7;
		break;
	case 0x00000030:
		r = CS8;
		break;
	}
	r |=	((l & 0x00000040) ? CSTOPB	: 0);
	r |=	((l & 0x00000080) ? CREAD	: 0);
	r |= 	((l & 0x00000100) ? PARENB	: 0);
	r |=	((l & 0x00000200) ? PARODD	: 0);
	r |=	((l & 0x00000400) ? HUPCL	: 0);
	r |=	((l & 0x00000800) ? CLOCAL	: 0);
	/*	((l & 0x00001000) ? LOBLK	: 0) */
	r |=	((l & 0x80000000) ? (CRTS_IFLOW|CCTS_OFLOW) : 0);
	bt->c_cflag = r;

	bt->c_ispeed = bt->c_ospeed = s2btab[l & 0x0000000f];

	l = st->c_lflag;
	r = 	((l & 0x00000001) ? ISIG	: 0);
	r |=	((l & 0x00000002) ? ICANON	: 0);
	/*	((l & 0x00000004) ? XCASE	: 0) */
	r |=	((l & 0x00000008) ? ECHO	: 0);
	r |=	((l & 0x00000010) ? ECHOE	: 0);
	r |=	((l & 0x00000020) ? ECHOK	: 0);
	r |=	((l & 0x00000040) ? ECHONL	: 0);
	r |= 	((l & 0x00000080) ? NOFLSH	: 0);
	r |=	((l & 0x00000100) ? TOSTOP	: 0);
	r |=	((l & 0x00000200) ? ECHOCTL	: 0);
	r |=	((l & 0x00000400) ? ECHOPRT	: 0);
	r |=	((l & 0x00000800) ? ECHOKE	: 0);
	/*	((l & 0x00001000) ? DEFECHO	: 0) */
	r |=	((l & 0x00002000) ? FLUSHO	: 0);
	r |=	((l & 0x00004000) ? PENDIN	: 0);
	bt->c_lflag = r;

	bt->c_cc[VINTR]    = EMUL_TO_NATIVE_CC(st->c_cc[0]);
	bt->c_cc[VQUIT]    = EMUL_TO_NATIVE_CC(st->c_cc[1]);
	bt->c_cc[VERASE]   = EMUL_TO_NATIVE_CC(st->c_cc[2]);
	bt->c_cc[VKILL]    = EMUL_TO_NATIVE_CC(st->c_cc[3]);
	bt->c_cc[VEOF]     = EMUL_TO_NATIVE_CC(st->c_cc[4]);
	bt->c_cc[VEOL]     = EMUL_TO_NATIVE_CC(st->c_cc[5]);
	bt->c_cc[VEOL2]    = EMUL_TO_NATIVE_CC(st->c_cc[6]);
	/* not present on NetBSD */
	/* bt->c_cc[VSWTCH]   = EMUL_TO_NATIVE_CC(st->c_cc[7]); */
	bt->c_cc[VSTART]   = EMUL_TO_NATIVE_CC(st->c_cc[10]);
	bt->c_cc[VSTOP]    = EMUL_TO_NATIVE_CC(st->c_cc[11]);
	bt->c_cc[VSUSP]    = EMUL_TO_NATIVE_CC(st->c_cc[12]);
	bt->c_cc[VDSUSP]   = EMUL_TO_NATIVE_CC(st->c_cc[13]);
	bt->c_cc[VREPRINT] = EMUL_TO_NATIVE_CC(st->c_cc[14]);
	bt->c_cc[VDISCARD] = EMUL_TO_NATIVE_CC(st->c_cc[15]);
	bt->c_cc[VWERASE]  = EMUL_TO_NATIVE_CC(st->c_cc[16]);
	bt->c_cc[VLNEXT]   = EMUL_TO_NATIVE_CC(st->c_cc[17]);
	bt->c_cc[VSTATUS]  = EMUL_TO_NATIVE_CC(st->c_cc[18]);

#ifdef COMPAT_ULTRIX
	/* Ultrix termio/termios has real vmin/vtime */
	bt->c_cc[VMIN]	   = EMUL_TO_NATIVE_CC(st->c_cc[8]);
	bt->c_cc[VTIME]	   = EMUL_TO_NATIVE_CC(st->c_cc[9]);
#else
	/* if `raw mode', create native VMIN/VTIME from SunOS VEOF/VEOL */
	bt->c_cc[VMIN]	   = (bt->c_lflag & ICANON) ? 1 : bt->c_cc[VEOF];
	bt->c_cc[VTIME]	   = (bt->c_lflag & ICANON) ? 1 : bt->c_cc[VEOL];
#endif

}

/*
 * Convert bsd termios to "sunos" emulated termios
 */
static void
btios2stios(struct termios *bt, struct emul_termios *st)
{
	u_long l, r;

	l = bt->c_iflag;
	r = 	((l &  IGNBRK) ? 0x00000001	: 0);
	r |=	((l &  BRKINT) ? 0x00000002	: 0);
	r |=	((l &  IGNPAR) ? 0x00000004	: 0);
	r |=	((l &  PARMRK) ? 0x00000008	: 0);
	r |=	((l &   INPCK) ? 0x00000010	: 0);
	r |=	((l &  ISTRIP) ? 0x00000020	: 0);
	r |=	((l &   INLCR) ? 0x00000040	: 0);
	r |=	((l &   IGNCR) ? 0x00000080	: 0);
	r |=	((l &   ICRNL) ? 0x00000100	: 0);
	/*	((l &   IUCLC) ? 0x00000200	: 0) */
	r |=	((l &    IXON) ? 0x00000400	: 0);
	r |=	((l &   IXANY) ? 0x00000800	: 0);
	r |=	((l &   IXOFF) ? 0x00001000	: 0);
	r |=	((l & IMAXBEL) ? 0x00002000	: 0);
	st->c_iflag = r;

	l = bt->c_oflag;
	r =	((l &   OPOST) ? 0x00000001	: 0);
	/*	((l &   OLCUC) ? 0x00000002	: 0) */
	r |=	((l &   ONLCR) ? 0x00000004	: 0);
	/*	((l &   OCRNL) ? 0x00000008	: 0) */
	/*	((l &   ONOCR) ? 0x00000010	: 0) */
	/*	((l &  ONLRET) ? 0x00000020	: 0) */
	/*	((l &   OFILL) ? 0x00000040	: 0) */
	/*	((l &   OFDEL) ? 0x00000080	: 0) */
	/*	((l &   NLDLY) ? 0x00000100	: 0) */
	/*	((l &     NL1) ? 0x00000100	: 0) */
	/*	((l &   CRDLY) ? 0x00000600	: 0) */
	/*	((l &     CR1) ? 0x00000200	: 0) */
	/*	((l &     CR2) ? 0x00000400	: 0) */
	/*	((l &     CR3) ? 0x00000600	: 0) */
	/*	((l &  TABDLY) ? 0x00001800	: 0) */
	/*	((l &    TAB1) ? 0x00000800	: 0) */
	/*	((l &    TAB2) ? 0x00001000	: 0) */
	r |=	((l &  OXTABS) ? 0x00001800	: 0);
	/*	((l &   BSDLY) ? 0x00002000	: 0) */
	/*	((l &     BS1) ? 0x00002000	: 0) */
	/*	((l &   VTDLY) ? 0x00004000	: 0) */
	/*	((l &     VT1) ? 0x00004000	: 0) */
	/*	((l &   FFDLY) ? 0x00008000	: 0) */
	/*	((l &     FF1) ? 0x00008000	: 0) */
	/*	((l & PAGEOUT) ? 0x00010000	: 0) */
	/*	((l &    WRAP) ? 0x00020000	: 0) */
	st->c_oflag = r;

	l = bt->c_cflag;
	switch (l & CSIZE) {
	case CS5:
		r = 0;
		break;
	case CS6:
		r = 0x00000010;
		break;
	case CS7:
		r = 0x00000020;
		break;
	case CS8:
		r = 0x00000030;
		break;
	}
	r |=	((l &  CSTOPB) ? 0x00000040	: 0);
	r |=	((l &   CREAD) ? 0x00000080	: 0);
	r |=	((l &  PARENB) ? 0x00000100	: 0);
	r |=	((l &  PARODD) ? 0x00000200	: 0);
	r |=	((l &   HUPCL) ? 0x00000400	: 0);
	r |=	((l &  CLOCAL) ? 0x00000800	: 0);
	/*	((l &   LOBLK) ? 0x00001000	: 0) */
	r |=	((l & (CRTS_IFLOW|CCTS_OFLOW)) ? 0x80000000 : 0);
	st->c_cflag = r;

	l = bt->c_lflag;
	r =	((l &    ISIG) ? 0x00000001	: 0);
	r |=	((l &  ICANON) ? 0x00000002	: 0);
	/*	((l &   XCASE) ? 0x00000004	: 0) */
	r |=	((l &    ECHO) ? 0x00000008	: 0);
	r |=	((l &   ECHOE) ? 0x00000010	: 0);
	r |=	((l &   ECHOK) ? 0x00000020	: 0);
	r |=	((l &  ECHONL) ? 0x00000040	: 0);
	r |=	((l &  NOFLSH) ? 0x00000080	: 0);
	r |=	((l &  TOSTOP) ? 0x00000100	: 0);
	r |=	((l & ECHOCTL) ? 0x00000200	: 0);
	r |=	((l & ECHOPRT) ? 0x00000400	: 0);
	r |=	((l &  ECHOKE) ? 0x00000800	: 0);
	/*	((l & DEFECHO) ? 0x00001000	: 0) */
	r |=	((l &  FLUSHO) ? 0x00002000	: 0);
	r |=	((l &  PENDIN) ? 0x00004000	: 0);
	st->c_lflag = r;

	l = ttspeedtab(bt->c_ospeed, sptab);
	if (l >= 0)
		st->c_cflag |= l;

	st->c_cc[0] = NATIVE_TO_EMUL_CC(bt->c_cc[VINTR]);
	st->c_cc[1] = NATIVE_TO_EMUL_CC(bt->c_cc[VQUIT]);
	st->c_cc[2] = NATIVE_TO_EMUL_CC(bt->c_cc[VERASE]);
	st->c_cc[3] = NATIVE_TO_EMUL_CC(bt->c_cc[VKILL]);
	st->c_cc[4] = NATIVE_TO_EMUL_CC(bt->c_cc[VEOF]);
	st->c_cc[5] = NATIVE_TO_EMUL_CC(bt->c_cc[VEOL]);
	st->c_cc[6] = NATIVE_TO_EMUL_CC(bt->c_cc[VEOL2]);

	/* XXX ultrix has a VSWTCH.  NetBSD does not. */
#ifdef notdef
	st->c_cc[7] = NATIVE_TO_EMUL_CC(bt->c_cc[VSWTCH]);
#else
	st->c_cc[7] = 0;
#endif
	st->c_cc[10] = NATIVE_TO_EMUL_CC(bt->c_cc[VSTART]);
	st->c_cc[11] = NATIVE_TO_EMUL_CC(bt->c_cc[VSTOP]);
	st->c_cc[12]= NATIVE_TO_EMUL_CC(bt->c_cc[VSUSP]);
	st->c_cc[13]= NATIVE_TO_EMUL_CC(bt->c_cc[VDSUSP]);
	st->c_cc[14]= NATIVE_TO_EMUL_CC(bt->c_cc[VREPRINT]);
	st->c_cc[15]= NATIVE_TO_EMUL_CC(bt->c_cc[VDISCARD]);
	st->c_cc[16]= NATIVE_TO_EMUL_CC(bt->c_cc[VWERASE]);
	st->c_cc[17]= NATIVE_TO_EMUL_CC(bt->c_cc[VLNEXT]);
	st->c_cc[18]= NATIVE_TO_EMUL_CC(bt->c_cc[VSTATUS]);

#ifdef COMPAT_ULTRIX
	st->c_cc[8]= NATIVE_TO_EMUL_CC(bt->c_cc[VMIN]);
	st->c_cc[9]= NATIVE_TO_EMUL_CC(bt->c_cc[VTIME]);
#else
	if (!(bt->c_lflag & ICANON)) {
		/* SunOS stores VMIN/VTIME in VEOF/VEOL (if ICANON is off) */
		st->c_cc[4] = bt->c_cc[VMIN];
		st->c_cc[5] = bt->c_cc[VTIME];
	}
#endif

#ifdef COMPAT_SUNOS
	st->c_line = 0;	/* 4.3bsd "old" line discipline */
#else
	st->c_line = 2;	/* 4.3bsd "new" line discipline, Ultrix default. */
#endif
}

#define TERMIO_NCC 10	/* ultrix termio NCC is 10 */

/*
 * Convert emulated struct termios to termio(?)
 */
static void
stios2stio(struct emul_termios *ts, struct emul_termio *t)
{
	t->c_iflag = ts->c_iflag;
	t->c_oflag = ts->c_oflag;
	t->c_cflag = ts->c_cflag;
	t->c_lflag = ts->c_lflag;
	t->c_line  = ts->c_line;
	memcpy(t->c_cc, ts->c_cc, TERMIO_NCC);
}

/*
 * Convert the other way
 */
static void
stio2stios(struct emul_termio *t, struct emul_termios *ts)
{
	ts->c_iflag = t->c_iflag;
	ts->c_oflag = t->c_oflag;
	ts->c_cflag = t->c_cflag;
	ts->c_lflag = t->c_lflag;
	ts->c_line  = t->c_line;
	memcpy(ts->c_cc, t->c_cc, TERMIO_NCC); /* don't touch the upper fields! */
}

int
ultrix_sys_ioctl(struct lwp *l, void *v, register_t *retval)
{
	struct ultrix_sys_ioctl_args *uap = v;
	struct proc *p = l->l_proc;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	int (*ctl)(struct file *, u_long, void *, struct lwp *);
	int error;

	if ((fp = fd_getfile(fdp, SCARG(uap, fd))) == NULL)
		return EBADF;

	if ((fp->f_flag & (FREAD|FWRITE)) == 0)
		return EBADF;

	ctl = fp->f_ops->fo_ioctl;

	switch (SCARG(uap, com)) {
	case _IOR('t', 0, int):
		SCARG(uap, com) = TIOCGETD;
		break;
	case _IOW('t', 1, int):
	    {
		int disc;

		if ((error = copyin(SCARG(uap, data), &disc, sizeof disc)) != 0)
			return error;

		/* map SunOS NTTYDISC into our termios discipline */
		if (disc == 2)
			disc = 0;
		/* all other disciplines are not supported by NetBSD */
		if (disc)
			return ENXIO;

		return (*ctl)(fp, TIOCSETD, &disc, l);
	    }
	case _IOW('t', 101, int):	/* sun SUNOS_TIOCSSOFTCAR */
	    {
		int x;	/* unused */

		return copyin(&x, SCARG(uap, data), sizeof x);
	    }
	case _IOR('t', 100, int):	/* sun SUNOS_TIOCSSOFTCAR */
	    {
		int x = 0;

		return copyout(&x, SCARG(uap, data), sizeof x);
	    }
	case _IO('t', 36): 		/* sun TIOCCONS, no parameters */
	    {
		int on = 1;
		return (*ctl)(fp, TIOCCONS, &on, l);
	    }
	case _IOW('t', 37, struct sunos_ttysize):
	    {
		struct winsize ws;
		struct sunos_ttysize ss;

		if ((error = (*ctl)(fp, TIOCGWINSZ, &ws, l)) != 0)
			return (error);

		if ((error = copyin(SCARG(uap, data), &ss, sizeof (ss))) != 0)
			return error;

		ws.ws_row = ss.ts_row;
		ws.ws_col = ss.ts_col;

		return (*ctl)(fp, TIOCSWINSZ, &ws, l);
	    }
	case _IOW('t', 38, struct sunos_ttysize):
	    {
		struct winsize ws;
		struct sunos_ttysize ss;

		if ((error = (*ctl)(fp, TIOCGWINSZ, &ws, l)) != 0)
			return (error);

		ss.ts_row = ws.ws_row;
		ss.ts_col = ws.ws_col;

		return copyout (&ss, SCARG(uap, data), sizeof (ss));
	    }
	case _IOW('t', 118, int):
		SCARG(uap, com) = TIOCSPGRP;
		break;
	case _IOR('t', 119, int):
		SCARG(uap, com) = TIOCGPGRP;
		break;

	/* Emulate termio or termios tcget() */
	case ULTRIX_TCGETA:
	case ULTRIX_TCGETS:
	    {
		struct termios bts;
		struct ultrix_termios sts;
		struct ultrix_termio st;

		if ((error = (*ctl)(fp, TIOCGETA, &bts, l)) != 0)
			return error;

		btios2stios (&bts, &sts);
		if (SCARG(uap, com) == ULTRIX_TCGETA) {
			stios2stio (&sts, &st);
			return copyout(&st, SCARG(uap, data), sizeof (st));
		} else
			return copyout(&sts, SCARG(uap, data), sizeof (sts));
		/*NOTREACHED*/
	    }
	/* Emulate termio tcset() */
	case ULTRIX_TCSETA:
	case ULTRIX_TCSETAW:
	case ULTRIX_TCSETAF:
	    {
		struct termios bts;
		struct ultrix_termios sts;
		struct ultrix_termio st;
		int result;

		if ((error = copyin(SCARG(uap, data), &st, sizeof (st))) != 0)
			return error;

		/* get full BSD termios so we don't lose information */
		if ((error = (*ctl)(fp, TIOCGETA, &bts, l)) != 0)
			return error;

		/*
		 * convert to sun termios, copy in information from
		 * termio, and convert back, then set new values.
		 */
		btios2stios(&bts, &sts);
		stio2stios(&st, &sts);
		stios2btios(&sts, &bts);

		/*
		 * map ioctl code: ultrix tcsets are numbered in reverse order
		 */
#ifdef notyet
		return (*ctl)(fp, ULTRIX_TCSETA - SCARG(uap, com) + TIOCSETA,
		    &bts, l);
#else
		result= (*ctl)(fp, ULTRIX_TCSETA -  SCARG(uap, com) + TIOCSETA,
		    &bts, l);
		printf("ultrix TCSETA %lx returns %d\n",
		    ULTRIX_TCSETA - SCARG(uap, com), result);
		return result;
#endif

	    }
	/* Emulate termios tcset() */
	case ULTRIX_TCSETS:
	case ULTRIX_TCSETSW:
	case ULTRIX_TCSETSF:
	    {
		struct termios bts;
		struct ultrix_termios sts;

		if ((error = copyin(SCARG(uap, data), &sts, sizeof (sts))) != 0)
			return error;
		stios2btios (&sts, &bts);
		return (*ctl)(fp, ULTRIX_TCSETS - SCARG(uap, com) + TIOCSETA,
		    &bts, l);
	    }
/*
 * Pseudo-tty ioctl translations.
 */
	case _IOW('t', 32, int): {	/* TIOCTCNTL */
		int on;

		error = copyin(SCARG(uap, data), &on, sizeof (on));
		if (error != 0)
			return error;
		return (*ctl)(fp, TIOCUCNTL, &on, l);
	}
	case _IOW('t', 33, int): {	/* TIOCSIGNAL */
		int sig;

		error = copyin(SCARG(uap, data), &sig, sizeof (sig));
		if (error != 0)
			return error;
		return (*ctl)(fp, TIOCSIG, &sig, l);
	}

/*
 * Socket ioctl translations.
 */
#define IN_TYPE(a, type_t) { \
	type_t localbuf; \
	if ((error = copyin(SCARG(uap, data), \
				&localbuf, sizeof (type_t))) != 0) \
		return error; \
	return (*ctl)(fp, a, &localbuf, l); \
}

#define INOUT_TYPE(a, type_t) { \
	type_t localbuf; \
	if ((error = copyin(SCARG(uap, data), &localbuf,	\
			     sizeof (type_t))) != 0) \
		return error; \
	if ((error = (*ctl)(fp, a, &localbuf, l)) != 0) \
		return error; \
	return copyout(&localbuf, SCARG(uap, data), sizeof (type_t)); \
}


#define IFREQ_IN(a) { \
	struct oifreq ifreq; \
	if ((error = copyin(SCARG(uap, data), &ifreq, sizeof (ifreq))) != 0) \
		return error; \
	return (*ctl)(fp, a, &ifreq, l); \
}

#define IFREQ_INOUT(a) { \
	struct oifreq ifreq; \
	if ((error = copyin(SCARG(uap, data), &ifreq, sizeof (ifreq))) != 0) \
		return error; \
	if ((error = (*ctl)(fp, a, &ifreq, l)) != 0) \
		return error; \
	return copyout(&ifreq, SCARG(uap, data), sizeof (ifreq)); \
}

	case _IOW('i', 12, struct oifreq):
		/* SIOCSIFADDR */
		break;

	case _IOWR('i', 13, struct oifreq):
		IFREQ_INOUT(OOSIOCGIFADDR);

	case _IOW('i', 14, struct oifreq):
		/* SIOCSIFDSTADDR */
		break;

	case _IOWR('i', 15, struct oifreq):
		IFREQ_INOUT(OOSIOCGIFDSTADDR);

	case _IOW('i', 16, struct oifreq):
		/* SIOCSIFFLAGS */
		break;

	case _IOWR('i', 17, struct oifreq):
		/* SIOCGIFFLAGS */
		break;

	case _IOWR('i', 18, struct oifreq):
		IFREQ_INOUT(SIOCGIFBRDADDR);

	case _IOWR('i', 19, struct oifreq):
		IFREQ_INOUT(SIOCSIFBRDADDR);

	case _IOWR('i', 20, struct ifconf):	/* SIOCGIFCONF */
	    {
		struct ifconf ifconfarg;

		/*
		 * XXX: two more problems
		 * 1. our sockaddr's are variable length, not always sizeof(sockaddr)
		 * 2. this returns a name per protocol, ie. it returns two "lo0"'s
		 */
		error = copyin(SCARG(uap, data), &ifconfarg, sizeof (ifconfarg));
		if (error)
			return error;
		error = (*ctl)(fp, OSIOCGIFCONF, &ifconfarg, l);
		if (error)
			return error;
		return copyout(&ifconfarg, SCARG(uap, data), sizeof (ifconfarg));
	    }


	case _IOWR('i', 21, struct oifreq):
		IFREQ_INOUT(OOSIOCGIFNETMASK);

	case _IOW('i', 22, struct oifreq):
		IFREQ_IN(SIOCSIFNETMASK);

	/* 23: _IOWR('i', 23, struct oifreq):  Ultrix SIOCSPHYADDR */
	/* 24: _IOWR('i', 24, struct oifreq):  Ultrix SIOCSADDMULTI */
	/* 25: _IOWR('i', 25, struct oifreq):  Ultrix SIOCSDELMULTI */

	case _IOW('i',  26, struct oifreq):	/* SIOCSIFRDCTRS? */
	case _IOWR('i', 27, struct oifreq):	/* SIOCGIFZCTRS? */
	case _IOWR('i', 28, struct oifreq):	/* read physaddr ? */
		return EOPNOTSUPP;


	case _IOW('i', 30, struct arpreq):
		/* SIOCSARP */
		break;

	case _IOWR('i', 31, struct arpreq):
		/* SIOCGARP */
		break;

	case _IOW('i', 32, struct arpreq):
		/* SIOCDARP */
		break;

	case _IOW('i', 40, struct oifreq):	/* SIOCARPREQ */
		return EOPNOTSUPP;

	case _IOWR('i', 41, struct oifreq):
		IFREQ_INOUT(SIOCGIFMETRIC);

	case _IOWR('i', 42, struct oifreq):
		IFREQ_IN(SIOCSIFMETRIC);

	case _IOW('i', 44, struct oifreq):	/* SIOCSETSYNC */
	case _IOWR('i', 45, struct oifreq):	/* SIOCGETSYNC */
	case _IOWR('i', 46, struct oifreq):	/* SIOCSDSTATS */
	case _IOWR('i', 47, struct oifreq):	/* SIOCSESTATS */
	case _IOW('i', 48, int):		/* SIOCSPROMISC */
		return EOPNOTSUPP;

	/* emulate for vat, vic tools */
	case _IOW('i', 49, struct oifreq):	/* SIOCADDMULTI */
	case _IOW('i', 50, struct oifreq):	/* SIOCDELMULTI */
		return EOPNOTSUPP;

	}
	return (sys_ioctl(l, uap, retval));
}
