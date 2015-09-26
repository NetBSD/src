/*	$NetBSD: sunos32_ioctl.c,v 1.32 2015/09/26 04:13:39 christos Exp $	*/
/* from: NetBSD: sunos_ioctl.c,v 1.35 2001/02/03 22:20:02 mrg Exp 	*/

/*
 * Copyright (c) 2001 Matthew R. Green
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

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
__KERNEL_RCSID(0, "$NetBSD: sunos32_ioctl.c,v 1.32 2015/09/26 04:13:39 christos Exp $");

#if defined(_KERNEL_OPT)
#include "opt_compat_netbsd32.h"
#include "opt_execfmt.h"
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
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/disklabel.h>
#include <sys/syscallargs.h>

#include <miscfs/specfs/specdev.h>

#include <net/if.h>

#include <dev/sun/disklabel.h>

#include <compat/sys/sockio.h>

#include <compat/sunos/sunos.h>
#include <compat/sunos/sunos_syscallargs.h>
#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/sunos32/sunos32.h>
#include <compat/sunos32/sunos32_syscallargs.h>
#include <compat/common/compat_util.h>

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

static const netbsd32_u_long s2btab[] = {
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

static void stios2btios(struct sunos_termios *, struct termios *);
static void btios2stios(struct termios *, struct sunos_termios *);
static void stios2stio(struct sunos_termios *, struct sunos_termio *);
static void stio2stios(struct sunos_termio *, struct sunos_termios *);

/*
 * These two conversion functions have mostly been done
 * with some perl cut&paste, then hand-edited to comment
 * out what doesn't exist under NetBSD.
 * A note from Markus's code:
 *	(l & BITMASK1) / BITMASK1 * BITMASK2  is translated
 *	optimally by gcc m68k, much better than any ?: stuff.
 *	Code may vary with different architectures of course.
 *
 * I don't know what optimizer you used, but seeing divu's and
 * bfextu's in the m68k assembly output did not encourage me...
 * as well, gcc on the sparc definitely generates much better
 * code with `?:'.
 */

static void
stios2btios(struct sunos_termios *st, struct termios *bt)
{
	netbsd32_u_long l, r;

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

	bt->c_cc[VINTR]    = st->c_cc[0]  ? st->c_cc[0]  : _POSIX_VDISABLE;
	bt->c_cc[VQUIT]    = st->c_cc[1]  ? st->c_cc[1]  : _POSIX_VDISABLE;
	bt->c_cc[VERASE]   = st->c_cc[2]  ? st->c_cc[2]  : _POSIX_VDISABLE;
	bt->c_cc[VKILL]    = st->c_cc[3]  ? st->c_cc[3]  : _POSIX_VDISABLE;
	bt->c_cc[VEOF]     = st->c_cc[4]  ? st->c_cc[4]  : _POSIX_VDISABLE;
	bt->c_cc[VEOL]     = st->c_cc[5]  ? st->c_cc[5]  : _POSIX_VDISABLE;
	bt->c_cc[VEOL2]    = st->c_cc[6]  ? st->c_cc[6]  : _POSIX_VDISABLE;
    /*	bt->c_cc[VSWTCH]   = st->c_cc[7]  ? st->c_cc[7]  : _POSIX_VDISABLE; */
	bt->c_cc[VSTART]   = st->c_cc[8]  ? st->c_cc[8]  : _POSIX_VDISABLE;
	bt->c_cc[VSTOP]    = st->c_cc[9]  ? st->c_cc[9]  : _POSIX_VDISABLE;
	bt->c_cc[VSUSP]    = st->c_cc[10] ? st->c_cc[10] : _POSIX_VDISABLE;
	bt->c_cc[VDSUSP]   = st->c_cc[11] ? st->c_cc[11] : _POSIX_VDISABLE;
	bt->c_cc[VREPRINT] = st->c_cc[12] ? st->c_cc[12] : _POSIX_VDISABLE;
	bt->c_cc[VDISCARD] = st->c_cc[13] ? st->c_cc[13] : _POSIX_VDISABLE;
	bt->c_cc[VWERASE]  = st->c_cc[14] ? st->c_cc[14] : _POSIX_VDISABLE;
	bt->c_cc[VLNEXT]   = st->c_cc[15] ? st->c_cc[15] : _POSIX_VDISABLE;
	bt->c_cc[VSTATUS]  = st->c_cc[16] ? st->c_cc[16] : _POSIX_VDISABLE;

	/* if `raw mode', create native VMIN/VTIME from SunOS VEOF/VEOL */
	bt->c_cc[VMIN]	   = (bt->c_lflag & ICANON) ? 1 : bt->c_cc[VEOF];
	bt->c_cc[VTIME]	   = (bt->c_lflag & ICANON) ? 1 : bt->c_cc[VEOL];
}


static void
btios2stios(struct termios *bt, struct sunos_termios *st)
{
	netbsd32_u_long l, r;
	int s;

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

	s = ttspeedtab(bt->c_ospeed, sptab);
	if (s >= 0)
		st->c_cflag |= s;

	st->c_cc[0] = bt->c_cc[VINTR]   != _POSIX_VDISABLE? bt->c_cc[VINTR]:0;
	st->c_cc[1] = bt->c_cc[VQUIT]   != _POSIX_VDISABLE? bt->c_cc[VQUIT]:0;
	st->c_cc[2] = bt->c_cc[VERASE]  != _POSIX_VDISABLE? bt->c_cc[VERASE]:0;
	st->c_cc[3] = bt->c_cc[VKILL]   != _POSIX_VDISABLE? bt->c_cc[VKILL]:0;
	st->c_cc[4] = bt->c_cc[VEOF]    != _POSIX_VDISABLE? bt->c_cc[VEOF]:0;
	st->c_cc[5] = bt->c_cc[VEOL]    != _POSIX_VDISABLE? bt->c_cc[VEOL]:0;
	st->c_cc[6] = bt->c_cc[VEOL2]   != _POSIX_VDISABLE? bt->c_cc[VEOL2]:0;
	st->c_cc[7] = 0;
		/*    bt->c_cc[VSWTCH]  != _POSIX_VDISABLE? bt->c_cc[VSWTCH]: */
	st->c_cc[8] = bt->c_cc[VSTART]  != _POSIX_VDISABLE? bt->c_cc[VSTART]:0;
	st->c_cc[9] = bt->c_cc[VSTOP]   != _POSIX_VDISABLE? bt->c_cc[VSTOP]:0;
	st->c_cc[10]= bt->c_cc[VSUSP]   != _POSIX_VDISABLE? bt->c_cc[VSUSP]:0;
	st->c_cc[11]= bt->c_cc[VDSUSP]  != _POSIX_VDISABLE? bt->c_cc[VDSUSP]:0;
	st->c_cc[12]= bt->c_cc[VREPRINT]!= _POSIX_VDISABLE? bt->c_cc[VREPRINT]:0;
	st->c_cc[13]= bt->c_cc[VDISCARD]!= _POSIX_VDISABLE? bt->c_cc[VDISCARD]:0;
	st->c_cc[14]= bt->c_cc[VWERASE] != _POSIX_VDISABLE? bt->c_cc[VWERASE]:0;
	st->c_cc[15]= bt->c_cc[VLNEXT]  != _POSIX_VDISABLE? bt->c_cc[VLNEXT]:0;
	st->c_cc[16]= bt->c_cc[VSTATUS] != _POSIX_VDISABLE? bt->c_cc[VSTATUS]:0;

	if (!(bt->c_lflag & ICANON)) {
		/* SunOS stores VMIN/VTIME in VEOF/VEOL (if ICANON is off) */
		st->c_cc[4] = bt->c_cc[VMIN];
		st->c_cc[5] = bt->c_cc[VTIME];
	}

	st->c_line = 0;
}

static void
stios2stio(struct sunos_termios *ts, struct sunos_termio *t)
{
	t->c_iflag = ts->c_iflag;
	t->c_oflag = ts->c_oflag;
	t->c_cflag = ts->c_cflag;
	t->c_lflag = ts->c_lflag;
	t->c_line  = ts->c_line;
	memcpy(t->c_cc, ts->c_cc, 8);
}

static void
stio2stios(struct sunos_termio *t, struct sunos_termios *ts)
{
	ts->c_iflag = t->c_iflag;
	ts->c_oflag = t->c_oflag;
	ts->c_cflag = t->c_cflag;
	ts->c_lflag = t->c_lflag;
	ts->c_line  = t->c_line;
	memcpy(ts->c_cc, t->c_cc, 8); /* don't touch the upper fields! */
}


static int
sunos32_do_ioctl(int fd, int cmd, void *arg, struct lwp *l)
{
	file_t *fp;
	struct vnode *vp;
	int error;

	if ((error = fd_getvnode(fd, &fp)) != 0)
		return error;
	if ((fp->f_flag & (FREAD|FWRITE)) == 0) {
		fd_putfile(fd);
		return EBADF;
	}
	error = fp->f_ops->fo_ioctl(fp, cmd, arg);
	if (error == EIO && cmd == TIOCGPGRP) {
		vp = fp->f_vnode;
		if (vp != NULL && vp->v_type == VCHR && major(vp->v_rdev) == 21)
			error = ENOTTY;
	}
	fd_putfile(fd);
	return error;
}

int
sunos32_sys_ioctl(struct lwp *l, const struct sunos32_sys_ioctl_args *uap, register_t *retval)
{
	/* {
		int	fd;
		netbsd32_u_long	com;
		netbsd32_caddr_t	data;
	} */
	struct netbsd32_ioctl_args bsd_ua;
	int error;

	SCARG(&bsd_ua, fd) = SCARG(uap, fd);
	SCARG(&bsd_ua, com) = SCARG(uap, com);
	SCARG(&bsd_ua, data) = SCARG(uap, data);

	switch (SCARG(uap, com)) {
	case _IOR('t', 0, int):
		SCARG(&bsd_ua, com) = TIOCGETD;
		break;
	case _IOW('t', 1, int):
	    {
		int disc;

		if ((error = copyin(SCARG_P32(uap, data), &disc,
		    sizeof disc)) != 0)
			return error;

		/* map SunOS NTTYDISC into our termios discipline */
		if (disc == 2)
			disc = 0;
		/* all other disciplines are not supported by NetBSD */
		if (disc)
			return ENXIO;

		return sunos32_do_ioctl(SCARG(&bsd_ua, fd), TIOCSETD, &disc, l);
	    }
	case _IOW('t', 101, int):	/* sun SUNOS_TIOCSSOFTCAR */
	    {
		int x;	/* unused */

		return copyin(SCARG_P32(uap, data), &x, sizeof x);
	    }
	case _IOR('t', 100, int):	/* sun SUNOS_TIOCSSOFTCAR */
	    {
		int x = 0;

		return copyout(&x, SCARG_P32(uap, data), sizeof x);
	    }
	case _IO('t', 36): 		/* sun TIOCCONS, no parameters */
	    {
		int on = 1;
		return sunos32_do_ioctl(SCARG(&bsd_ua, fd), TIOCCONS, &on, l);
	    }
	case _IOW('t', 37, struct sunos_ttysize):
	    {
		struct winsize ws;
		struct sunos_ttysize ss;

		if ((error = sunos32_do_ioctl(SCARG(&bsd_ua, fd), TIOCGWINSZ, &ws, l)) != 0)
			return (error);

		if ((error = copyin(SCARG_P32(uap, data), &ss, sizeof (ss))) != 0)
			return error;

		ws.ws_row = ss.ts_row;
		ws.ws_col = ss.ts_col;

		return (sunos32_do_ioctl(SCARG(&bsd_ua, fd), TIOCSWINSZ, &ws, l));
	    }
	case _IOW('t', 38, struct sunos_ttysize):
	    {
		struct winsize ws;
		struct sunos_ttysize ss;

		if ((error = sunos32_do_ioctl(SCARG(&bsd_ua, fd), TIOCGWINSZ, &ws, l)) != 0)
			return (error);

		ss.ts_row = ws.ws_row;
		ss.ts_col = ws.ws_col;

		return copyout(&ss, SCARG_P32(uap, data), sizeof (ss));
	    }
	case _IOW('t', 130, int):	/* TIOCSETPGRP: posix variant */
		SCARG(&bsd_ua, com) = TIOCSPGRP;
		break;
	case _IOR('t', 131, int):	/* TIOCGETPGRP: posix variant */
	    {
		/*
		 * sigh, must do error translation on pty devices
		 * (see also kern/tty_pty.c)
		 */
		int pgrp;
		error = sunos32_do_ioctl(SCARG(&bsd_ua, fd), TIOCGPGRP, &pgrp, l);
		if (error)
			return (error);
		return copyout(&pgrp, SCARG_P32(uap, data), sizeof(pgrp));
	    }
	case _IO('t', 132):
		SCARG(&bsd_ua, com) = TIOCSCTTY;
		break;
	case SUNOS_TCGETA:
	case SUNOS_TCGETS:
	    {
		struct termios bts;
		struct sunos_termios sts;
		struct sunos_termio st;

		if ((error = sunos32_do_ioctl(SCARG(&bsd_ua, fd), TIOCGETA, &bts, l)) != 0)
			return error;

		btios2stios (&bts, &sts);
		if (SCARG(uap, com) == SUNOS_TCGETA) {
			stios2stio (&sts, &st);
			return copyout(&st, SCARG_P32(uap, data),
			    sizeof (st));
		} else
			return copyout(&sts, SCARG_P32(uap, data),
			    sizeof (sts));
		/*NOTREACHED*/
	    }
	case SUNOS_TCSETA:
	case SUNOS_TCSETAW:
	case SUNOS_TCSETAF:
	    {
		struct termios bts;
		struct sunos_termios sts;
		struct sunos_termio st;

		if ((error = copyin(SCARG_P32(uap, data), &st,
		    sizeof (st))) != 0)
			return error;

		/* get full BSD termios so we don't lose information */
		if ((error = sunos32_do_ioctl(SCARG(&bsd_ua, fd), TIOCGETA, &bts, l)) != 0)
			return error;

		/*
		 * convert to sun termios, copy in information from
		 * termio, and convert back, then set new values.
		 */
		btios2stios(&bts, &sts);
		stio2stios(&st, &sts);
		stios2btios(&sts, &bts);

		return sunos32_do_ioctl(SCARG(&bsd_ua, fd), SCARG(uap, com) - SUNOS_TCSETA + TIOCSETA,
		    &bts, l);
	    }
	case SUNOS_TCSETS:
	case SUNOS_TCSETSW:
	case SUNOS_TCSETSF:
	    {
		struct termios bts;
		struct sunos_termios sts;

		if ((error = copyin(SCARG_P32(uap, data), &sts,
		    sizeof (sts))) != 0)
			return error;
		stios2btios (&sts, &bts);
		return sunos32_do_ioctl(SCARG(&bsd_ua, fd), SCARG(uap, com) - SUNOS_TCSETS + TIOCSETA,
		    &bts, l);
	    }
/*
 * Pseudo-tty ioctl translations.
 */
	case _IOW('t', 32, int): {	/* TIOCTCNTL */
		int error1, on;

		error1 = copyin(SCARG_P32(uap, data), &on, sizeof (on));
		if (error1)
			return error1;
		return sunos32_do_ioctl(SCARG(&bsd_ua, fd), TIOCUCNTL, &on, l);
	}
	case _IOW('t', 33, int): {	/* TIOCSIGNAL */
		int error1, sig;

		error1 = copyin(SCARG_P32(uap, data), &sig, sizeof (sig));
		if (error1)
			return error1;
		return sunos32_do_ioctl(SCARG(&bsd_ua, fd), TIOCSIG, &sig, l);
	}

/*
 * Socket ioctl translations.
 */
#define IFREQ_IN(a) { \
	struct oifreq ifreq; \
	error = copyin(SCARG_P32(uap, data), &ifreq, sizeof (ifreq)); \
	if (error) \
		return error; \
	return sunos32_do_ioctl(SCARG(&bsd_ua, fd), a, &ifreq, l); \
}
#define IFREQ_INOUT(a) { \
	struct oifreq ifreq; \
	error = copyin(SCARG_P32(uap, data), &ifreq, sizeof (ifreq)); \
	if (error) \
		return error; \
	if ((error = sunos32_do_ioctl(SCARG(&bsd_ua, fd), a, &ifreq, l)) != 0) \
		return error; \
	return copyout(&ifreq, SCARG_P32(uap, data), sizeof (ifreq)); \
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

	case _IOW('i', 21, struct oifreq):
		IFREQ_IN(SIOCSIFMTU);

	case _IOWR('i', 22, struct oifreq):
		IFREQ_INOUT(SIOCGIFMTU);

	case _IOWR('i', 23, struct oifreq):
		IFREQ_INOUT(SIOCGIFBRDADDR);

	case _IOW('i', 24, struct oifreq):
		IFREQ_IN(SIOCSIFBRDADDR);

	case _IOWR('i', 25, struct oifreq):
		IFREQ_INOUT(OOSIOCGIFNETMASK);

	case _IOW('i', 26, struct oifreq):
		IFREQ_IN(SIOCSIFNETMASK);

	case _IOWR('i', 27, struct oifreq):
		IFREQ_INOUT(SIOCGIFMETRIC);

	case _IOWR('i', 28, struct oifreq):
		IFREQ_IN(SIOCSIFMETRIC);

	case _IOW('i', 30, struct arpreq):
		/* SIOCSARP */
		break;

	case _IOWR('i', 31, struct arpreq):
		/* SIOCGARP */
		break;

	case _IOW('i', 32, struct arpreq):
		/* SIOCDARP */
		break;

	case _IOW('i', 18, struct oifreq):	/* SIOCSIFMEM */
	case _IOWR('i', 19, struct oifreq):	/* SIOCGIFMEM */
	case _IOW('i', 40, struct oifreq):	/* SIOCUPPER */
	case _IOW('i', 41, struct oifreq):	/* SIOCLOWER */
	case _IOW('i', 44, struct oifreq):	/* SIOCSETSYNC */
	case _IOWR('i', 45, struct oifreq):	/* SIOCGETSYNC */
	case _IOWR('i', 46, struct oifreq):	/* SIOCSDSTATS */
	case _IOWR('i', 47, struct oifreq):	/* SIOCSESTATS */
	case _IOW('i', 48, int):		/* SIOCSPROMISC */
	case _IOW('i', 49, struct oifreq):	/* SIOCADDMULTI */
	case _IOW('i', 50, struct oifreq):	/* SIOCDELMULTI */
		return EOPNOTSUPP;

	case _IOWR('i', 20, struct oifconf):	/* SIOCGIFCONF */
	    {
		struct oifconf ifcf;

		/*
		 * XXX: two more problems
		 * 1. our sockaddr's are variable length, not always sizeof(sockaddr)
		 * 2. this returns a name per protocol, ie. it returns two "lo0"'s
		 */
		error = copyin(SCARG_P32(uap, data), &ifcf,
		    sizeof (ifcf));
		if (error)
			return error;
		error = sunos32_do_ioctl(SCARG(&bsd_ua, fd), OOSIOCGIFCONF, &ifcf, l);
		if (error)
			return error;
		return copyout(&ifcf, SCARG_P32(uap, data),
		    sizeof (ifcf));
	    }

/*
 * Audio ioctl translations.
 */
	case _IOR('A', 1, struct sunos_audio_info):	/* AUDIO_GETINFO */
	sunos_au_getinfo:
	    {
		struct audio_info aui;
		struct sunos_audio_info sunos_aui;

		error = sunos32_do_ioctl(SCARG(&bsd_ua, fd), AUDIO_GETINFO, &aui, l);
		if (error)
			return error;

		sunos_aui.play = *(struct sunos_audio_prinfo *)&aui.play;
		sunos_aui.record = *(struct sunos_audio_prinfo *)&aui.record;

		/* `avail_ports' is `seek' in BSD */
		sunos_aui.play.avail_ports = AUDIO_SPEAKER | AUDIO_HEADPHONE;
		sunos_aui.record.avail_ports = AUDIO_SPEAKER | AUDIO_HEADPHONE;

		sunos_aui.play.waiting = 0;
		sunos_aui.record.waiting = 0;
		sunos_aui.play.eof = 0;
		sunos_aui.record.eof = 0;
		sunos_aui.monitor_gain = 0; /* aui.__spare; XXX */
		/*XXXsunos_aui.output_muted = 0;*/
		/*XXX*/sunos_aui.reserved[0] = 0;
		/*XXX*/sunos_aui.reserved[1] = 0;
		/*XXX*/sunos_aui.reserved[2] = 0;
		/*XXX*/sunos_aui.reserved[3] = 0;

		return copyout(&sunos_aui, SCARG_P32(uap, data),
				sizeof (sunos_aui));
	    }

	case _IOWR('A', 2, struct sunos_audio_info):	/* AUDIO_SETINFO */
	    {
		struct audio_info aui;
		struct sunos_audio_info sunos_aui;

		error = copyin(SCARG_P32(uap, data), &sunos_aui,
		    sizeof (sunos_aui));
		if (error)
			return error;

		aui.play = *(struct audio_prinfo *)&sunos_aui.play;
		aui.record = *(struct audio_prinfo *)&sunos_aui.record;
		/* aui.__spare = sunos_aui.monitor_gain; */
		aui.blocksize = ~0;
		aui.hiwat = ~0;
		aui.lowat = ~0;
		/* XXX somebody check this please. - is: aui.backlog = ~0; */
		aui.mode = ~0;
		/*
		 * The bsd driver does not distinguish between paused and
		 * active. (In the sun driver, not active means samples are
		 * not output at all, but paused means the last streams buffer
		 * is drained and then output stops.)  If either are 0, then
		 * when stop output. Otherwise, if either are non-zero,
		 * we resume.
		 */
		if (sunos_aui.play.pause == 0 || sunos_aui.play.active == 0)
			aui.play.pause = 0;
		else if (sunos_aui.play.pause != (u_char)~0 ||
			 sunos_aui.play.active != (u_char)~0)
			aui.play.pause = 1;
		if (sunos_aui.record.pause == 0 || sunos_aui.record.active == 0)
			aui.record.pause = 0;
		else if (sunos_aui.record.pause != (u_char)~0 ||
			 sunos_aui.record.active != (u_char)~0)
			aui.record.pause = 1;

		error = sunos32_do_ioctl(SCARG(&bsd_ua, fd), AUDIO_SETINFO, &aui, l);
		if (error)
			return error;
		/* Return new state */
		goto sunos_au_getinfo;
	    }
	case _IO('A', 3):	/* AUDIO_DRAIN */
		return sunos32_do_ioctl(SCARG(&bsd_ua, fd), AUDIO_DRAIN, NULL, l);
	case _IOR('A', 4, int):	/* AUDIO_GETDEV */
	    {
		int devtype = SUNOS_AUDIO_DEV_AMD;
		return copyout(&devtype, SCARG_P32(uap, data),
				sizeof (devtype));
	    }

/*
 * Selected streams ioctls.
 */
#define SUNOS_S_FLUSHR		1
#define SUNOS_S_FLUSHW		2
#define SUNOS_S_FLUSHRW		3

#define SUNOS_S_INPUT		1
#define SUNOS_S_HIPRI		2
#define SUNOS_S_OUTPUT		4
#define SUNOS_S_MSG		8

	case _IO('S', 5):	/* I_FLUSH */
	    {
		int tmp = 0;
		switch ((intptr_t)SCARG_P32(uap, data)) {
		case SUNOS_S_FLUSHR:	tmp = FREAD;
		case SUNOS_S_FLUSHW:	tmp = FWRITE;
		case SUNOS_S_FLUSHRW:	tmp = FREAD|FWRITE;
		}
                return sunos32_do_ioctl(SCARG(&bsd_ua, fd), TIOCFLUSH, &tmp, l);
	    }
	case _IO('S', 9):	/* I_SETSIG */
	    {
		int on = 1;
		if (((intptr_t)SCARG_P32(uap, data) &
			(SUNOS_S_HIPRI|SUNOS_S_INPUT)) ==
		    SUNOS_S_HIPRI)
			return EOPNOTSUPP;
                return sunos32_do_ioctl(SCARG(&bsd_ua, fd), FIOASYNC, &on, l);
	    }
	/*
	 * SunOS disk ioctls, taken from arch/sparc/sparc/disksubr.c
	 * (which was from the old sparc/scsi/sun_disklabel.c), and
	 * modified to suite.
	 */
	case SUN_DKIOCGGEOM:
            {
		struct disklabel dl;

		error = sunos32_do_ioctl(SCARG(&bsd_ua, fd), DIOCGDINFO, &dl, l);
		if (error)
			return (error);

#define datageom	((struct sun_dkgeom *)SCARG_P32(uap, data))
		/* XXX can't do memset() on a user address (dsl) */
		memset(SCARG_P32(uap, data), 0, sizeof(*datageom));

		datageom->sdkc_ncylinders = dl.d_ncylinders;
		datageom->sdkc_acylinders = dl.d_acylinders;
		datageom->sdkc_ntracks = dl.d_ntracks;
		datageom->sdkc_nsectors = dl.d_nsectors;
		datageom->sdkc_interleave = dl.d_interleave;
		datageom->sdkc_sparespercyl = dl.d_sparespercyl;
		datageom->sdkc_rpm = dl.d_rpm;
		datageom->sdkc_pcylinders = dl.d_ncylinders + dl.d_acylinders;
#undef datageom
		break;
	    }

	case SUN_DKIOCINFO:
		/* Homey don't do DKIOCINFO */
		/* XXX can't do memset() on a user address (dsl) */
		memset(SCARG_P32(uap, data), 0, sizeof(struct sun_dkctlr));
		break;

	case SUN_DKIOCGPART:
            {
		struct partinfo pi;

		error = sunos32_do_ioctl(SCARG(&bsd_ua, fd), DIOCGPART, &pi, l);
		if (error)
			return (error);

		if (pi.disklab->d_secpercyl == 0)
			return (ERANGE);	/* XXX */
		if (pi.part->p_offset % pi.disklab->d_secpercyl != 0)
			return (ERANGE);	/* XXX */
		/* XXX can't do direct writes to a user address (dsl) */
#define datapart	((struct sun_dkpart *)SCARG_P32(uap, data))
		datapart->sdkp_cyloffset = pi.part->p_offset / pi.disklab->d_secpercyl;
		datapart->sdkp_nsectors = pi.part->p_size;
#undef datapart
	    }

	}
	return (netbsd32_ioctl(l, &bsd_ua, retval));
}

/* SunOS fcntl(2) cmds not implemented */
#define SUN_F_RGETLK	10
#define SUN_F_RSETLK	11
#define SUN_F_CNVT	12
#define SUN_F_RSETLKW	13

/* SunOS flock translation */
struct sunos_flock {
	short	l_type;
	short	l_whence;
	netbsd32_long	l_start;
	netbsd32_long	l_len;
	short	l_pid;
	short	l_xxx;
};

static void bsd_to_sunos_flock(struct flock *, struct sunos_flock *);
static void sunos_to_bsd_flock(struct sunos_flock *, struct flock *);

#define SUNOS_F_RDLCK	1
#define	SUNOS_F_WRLCK	2
#define SUNOS_F_UNLCK	3

static void
bsd_to_sunos_flock(struct flock *iflp, struct sunos_flock *oflp)
{
	switch (iflp->l_type) {
	case F_RDLCK:
		oflp->l_type = SUNOS_F_RDLCK;
		break;
	case F_WRLCK:
		oflp->l_type = SUNOS_F_WRLCK;
		break;
	case F_UNLCK:
		oflp->l_type = SUNOS_F_UNLCK;
		break;
	default:
		oflp->l_type = -1;
		break;
	}

	oflp->l_whence = (short) iflp->l_whence;
	oflp->l_start = (netbsd32_long) iflp->l_start;
	oflp->l_len = (netbsd32_long) iflp->l_len;
	oflp->l_pid = (short) iflp->l_pid;
	oflp->l_xxx = 0;
}


static void
sunos_to_bsd_flock(struct sunos_flock *iflp, struct flock *oflp)
{
	switch (iflp->l_type) {
	case SUNOS_F_RDLCK:
		oflp->l_type = F_RDLCK;
		break;
	case SUNOS_F_WRLCK:
		oflp->l_type = F_WRLCK;
		break;
	case SUNOS_F_UNLCK:
		oflp->l_type = F_UNLCK;
		break;
	default:
		oflp->l_type = -1;
		break;
	}

	oflp->l_whence = iflp->l_whence;
	oflp->l_start = (off_t) iflp->l_start;
	oflp->l_len = (off_t) iflp->l_len;
	oflp->l_pid = (pid_t) iflp->l_pid;

}
static struct {
	netbsd32_long	sun_flg;
	netbsd32_long	bsd_flg;
} sunfcntl_flgtab[] = {
	/* F_[GS]ETFLags that differ: */
#define SUN_FSETBLK	0x0010
#define SUN_SHLOCK	0x0080
#define SUN_EXLOCK	0x0100
#define SUN_FNBIO	0x1000
#define SUN_FSYNC	0x2000
#define SUN_NONBLOCK	0x4000
#define SUN_FNOCTTY	0x8000
	{ SUN_NONBLOCK, O_NONBLOCK },
	{ SUN_FNBIO, O_NONBLOCK },
	{ SUN_SHLOCK, O_SHLOCK },
	{ SUN_EXLOCK, O_EXLOCK },
	{ SUN_FSYNC, O_FSYNC },
	{ SUN_FSETBLK, 0 },
	{ SUN_FNOCTTY, 0 }
};

int
sunos32_sys_fcntl(struct lwp *l, const struct sunos32_sys_fcntl_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) cmd;
		syscallarg(netbsd32_voidp) arg;
	} */
	struct sys_fcntl_args bsd_ua;
	uintptr_t flg;
	int n, ret;

	SCARG(&bsd_ua, fd) = SCARG(uap, fd);
	SCARG(&bsd_ua, cmd) = SCARG(uap, cmd);
	SCARG(&bsd_ua, arg) = SCARG_P32(uap, arg);

	switch (SCARG(uap, cmd)) {
	case F_SETFL:
		flg = (intptr_t)SCARG_P32(uap, arg);
		n = sizeof(sunfcntl_flgtab) / sizeof(sunfcntl_flgtab[0]);
		while (--n >= 0) {
			if (flg & sunfcntl_flgtab[n].sun_flg) {
				flg &= ~sunfcntl_flgtab[n].sun_flg;
				flg |= sunfcntl_flgtab[n].bsd_flg;
			}
		}
		SCARG(&bsd_ua, arg) = (void *)flg;
		break;

	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
		{
			int error;
			struct sunos_flock	ifl;
			struct flock		fl;

			error = copyin(SCARG_P32(uap, arg), &ifl, sizeof ifl);
			if (error)
				return error;
			sunos_to_bsd_flock(&ifl, &fl);

			error = do_fcntl_lock(SCARG(uap, fd), SCARG(uap, cmd), &fl);
			if (error || SCARG(uap, cmd) != F_GETLK)
				return error;

			bsd_to_sunos_flock(&fl, &ifl);
			return copyout(&ifl, SCARG_P32(uap, arg), sizeof ifl);
		}
		break;
	case SUN_F_RGETLK:
	case SUN_F_RSETLK:
	case SUN_F_CNVT:
	case SUN_F_RSETLKW:
		return (EOPNOTSUPP);
	}

	ret = sys_fcntl(l, &bsd_ua, retval);
	if (ret != 0)
		return ret;

	switch (SCARG(uap, cmd)) {
	case F_GETFL:
		n = sizeof(sunfcntl_flgtab) / sizeof(sunfcntl_flgtab[0]);
		ret = *retval;
		while (--n >= 0) {
			if (ret & sunfcntl_flgtab[n].bsd_flg) {
				ret &= ~sunfcntl_flgtab[n].bsd_flg;
				ret |= sunfcntl_flgtab[n].sun_flg;
			}
		}
		*retval = ret;
		break;
	}

	return 0;
}
