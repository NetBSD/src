/*	$NetBSD: ibcs2_ioctl.c,v 1.41.4.1 2008/01/02 21:52:01 bouyer Exp $	*/

/*
 * Copyright (c) 1994, 1995 Scott Bartram
 * All rights reserved.
 *
 * based on compat/sunos/sun_ioctl.c
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ibcs2_ioctl.c,v 1.41.4.1 2008/01/02 21:52:01 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/dirent.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/socket.h>
#include <sys/termios.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/tty.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sys/unistd.h>

#include <net/if.h>
#include <sys/syscallargs.h>

#include <compat/ibcs2/ibcs2_types.h>
#include <compat/ibcs2/ibcs2_signal.h>
#include <compat/ibcs2/ibcs2_socksys.h>
#include <compat/ibcs2/ibcs2_stropts.h>
#include <compat/ibcs2/ibcs2_syscallargs.h>
#include <compat/ibcs2/ibcs2_termios.h>
#include <compat/ibcs2/ibcs2_util.h>

/*
 * iBCS2 ioctl calls.
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

static void stios2btios(struct ibcs2_termios *, struct termios *);
static void btios2stios(struct termios *, struct ibcs2_termios *);
static void stios2stio(struct ibcs2_termios *, struct ibcs2_termio *);
static void stio2stios(struct ibcs2_termio *, struct ibcs2_termios *);

static void
stios2btios(struct ibcs2_termios *st, struct termios *bt)
{
	u_long l, r;

	l = st->c_iflag;	r = 0;
	if (l & IBCS2_IGNBRK)	r |= IGNBRK;
	if (l & IBCS2_BRKINT)	r |= BRKINT;
	if (l & IBCS2_IGNPAR)	r |= IGNPAR;
	if (l & IBCS2_PARMRK)	r |= PARMRK;
	if (l & IBCS2_INPCK)	r |= INPCK;
	if (l & IBCS2_ISTRIP)	r |= ISTRIP;
	if (l & IBCS2_INLCR)	r |= INLCR;
	if (l & IBCS2_IGNCR)	r |= IGNCR;
	if (l & IBCS2_ICRNL)	r |= ICRNL;
	if (l & IBCS2_IXON)	r |= IXON;
	if (l & IBCS2_IXANY)	r |= IXANY;
	if (l & IBCS2_IXOFF)	r |= IXOFF;
	if (l & IBCS2_IMAXBEL)	r |= IMAXBEL;
	bt->c_iflag = r;

	l = st->c_oflag;	r = 0;
	if (l & IBCS2_OPOST)	r |= OPOST;
	if (l & IBCS2_ONLCR)	r |= ONLCR;
	if (l & IBCS2_TAB3)	r |= OXTABS;
	bt->c_oflag = r;

	l = st->c_cflag;	r = 0;
	switch (l & IBCS2_CSIZE) {
	case IBCS2_CS5:		r |= CS5; break;
	case IBCS2_CS6:		r |= CS6; break;
	case IBCS2_CS7:		r |= CS7; break;
	case IBCS2_CS8:		r |= CS8; break;
	}
	if (l & IBCS2_CSTOPB)	r |= CSTOPB;
	if (l & IBCS2_CREAD)	r |= CREAD;
	if (l & IBCS2_PARENB)	r |= PARENB;
	if (l & IBCS2_PARODD)	r |= PARODD;
	if (l & IBCS2_HUPCL)	r |= HUPCL;
	if (l & IBCS2_CLOCAL)	r |= CLOCAL;
	bt->c_cflag = r;
	bt->c_ispeed = bt->c_ospeed = s2btab[l & 0x0f];

	l = st->c_lflag;	r = 0;
	if (l & IBCS2_ISIG)	r |= ISIG;
	if (l & IBCS2_ICANON)	r |= ICANON;
	if (l & IBCS2_ECHO)	r |= ECHO;
	if (l & IBCS2_ECHOE)	r |= ECHOE;
	if (l & IBCS2_ECHOK)	r |= ECHOK;
	if (l & IBCS2_ECHONL)	r |= ECHONL;
	if (l & IBCS2_NOFLSH)	r |= NOFLSH;
	if (l & IBCS2_TOSTOP)	r |= TOSTOP;
	bt->c_lflag = r;

	bt->c_cc[VINTR]	=
	    st->c_cc[IBCS2_VINTR]  ? st->c_cc[IBCS2_VINTR]  : _POSIX_VDISABLE;
	bt->c_cc[VQUIT] =
	    st->c_cc[IBCS2_VQUIT]  ? st->c_cc[IBCS2_VQUIT]  : _POSIX_VDISABLE;
	bt->c_cc[VERASE] =
	    st->c_cc[IBCS2_VERASE] ? st->c_cc[IBCS2_VERASE] : _POSIX_VDISABLE;
	bt->c_cc[VKILL] =
	    st->c_cc[IBCS2_VKILL]  ? st->c_cc[IBCS2_VKILL]  : _POSIX_VDISABLE;
	if (bt->c_lflag & ICANON) {
		bt->c_cc[VEOF] =
		    st->c_cc[IBCS2_VEOF] ? st->c_cc[IBCS2_VEOF] : _POSIX_VDISABLE;
		bt->c_cc[VEOL] =
		    st->c_cc[IBCS2_VEOL] ? st->c_cc[IBCS2_VEOL] : _POSIX_VDISABLE;
	} else {
		bt->c_cc[VMIN]  = st->c_cc[IBCS2_VMIN];
		bt->c_cc[VTIME] = st->c_cc[IBCS2_VTIME];
	}
	bt->c_cc[VEOL2] =
	    st->c_cc[IBCS2_VEOL2]  ? st->c_cc[IBCS2_VEOL2]  : _POSIX_VDISABLE;
#if 0
	bt->c_cc[VSWTCH] =
	    st->c_cc[IBCS2_VSWTCH] ? st->c_cc[IBCS2_VSWTCH] : _POSIX_VDISABLE;
#endif
	bt->c_cc[VSTART] =
	    st->c_cc[IBCS2_VSTART] ? st->c_cc[IBCS2_VSTART] : _POSIX_VDISABLE;
	bt->c_cc[VSTOP] =
	    st->c_cc[IBCS2_VSTOP]  ? st->c_cc[IBCS2_VSTOP]  : _POSIX_VDISABLE;
	bt->c_cc[VSUSP] =
	    st->c_cc[IBCS2_VSUSP]  ? st->c_cc[IBCS2_VSUSP]  : _POSIX_VDISABLE;
	bt->c_cc[VDSUSP]   = _POSIX_VDISABLE;
	bt->c_cc[VREPRINT] = _POSIX_VDISABLE;
	bt->c_cc[VDISCARD] = _POSIX_VDISABLE;
	bt->c_cc[VWERASE]  = _POSIX_VDISABLE;
	bt->c_cc[VLNEXT]   = _POSIX_VDISABLE;
	bt->c_cc[VSTATUS]  = _POSIX_VDISABLE;
}

static void
btios2stios(struct termios *bt, struct ibcs2_termios *st)
{
	int i;
	u_long l, r;

	l = bt->c_iflag;	r = 0;
	if (l & IGNBRK)		r |= IBCS2_IGNBRK;
	if (l & BRKINT)		r |= IBCS2_BRKINT;
	if (l & IGNPAR)		r |= IBCS2_IGNPAR;
	if (l & PARMRK)		r |= IBCS2_PARMRK;
	if (l & INPCK)		r |= IBCS2_INPCK;
	if (l & ISTRIP)		r |= IBCS2_ISTRIP;
	if (l & INLCR)		r |= IBCS2_INLCR;
	if (l & IGNCR)		r |= IBCS2_IGNCR;
	if (l & ICRNL)		r |= IBCS2_ICRNL;
	if (l & IXON)		r |= IBCS2_IXON;
	if (l & IXANY)		r |= IBCS2_IXANY;
	if (l & IXOFF)		r |= IBCS2_IXOFF;
	if (l & IMAXBEL)	r |= IBCS2_IMAXBEL;
	st->c_iflag = r;

	l = bt->c_oflag;	r = 0;
	if (l & OPOST)		r |= IBCS2_OPOST;
	if (l & ONLCR)		r |= IBCS2_ONLCR;
	if (l & OXTABS)		r |= IBCS2_TAB3;
	st->c_oflag = r;

	l = bt->c_cflag;	r = 0;
	switch (l & CSIZE) {
	case CS5:		r |= IBCS2_CS5; break;
	case CS6:		r |= IBCS2_CS6; break;
	case CS7:		r |= IBCS2_CS7; break;
	case CS8:		r |= IBCS2_CS8; break;
	}
	if (l & CSTOPB)		r |= IBCS2_CSTOPB;
	if (l & CREAD)		r |= IBCS2_CREAD;
	if (l & PARENB)		r |= IBCS2_PARENB;
	if (l & PARODD)		r |= IBCS2_PARODD;
	if (l & HUPCL)		r |= IBCS2_HUPCL;
	if (l & CLOCAL)		r |= IBCS2_CLOCAL;
	st->c_cflag = r;

	l = bt->c_lflag;	r = 0;
	if (l & ISIG)		r |= IBCS2_ISIG;
	if (l & ICANON)		r |= IBCS2_ICANON;
	if (l & ECHO)		r |= IBCS2_ECHO;
	if (l & ECHOE)		r |= IBCS2_ECHOE;
	if (l & ECHOK)		r |= IBCS2_ECHOK;
	if (l & ECHONL)		r |= IBCS2_ECHONL;
	if (l & NOFLSH)		r |= IBCS2_NOFLSH;
	if (l & TOSTOP)		r |= IBCS2_TOSTOP;
	st->c_lflag = r;

	i = ttspeedtab(bt->c_ospeed, sptab);
	if (i >= 0)
		st->c_cflag |= i;

	st->c_cc[IBCS2_VINTR] =
	    bt->c_cc[VINTR]  != _POSIX_VDISABLE ? bt->c_cc[VINTR]  : 0;
	st->c_cc[IBCS2_VQUIT] =
	    bt->c_cc[VQUIT]  != _POSIX_VDISABLE ? bt->c_cc[VQUIT]  : 0;
	st->c_cc[IBCS2_VERASE] =
	    bt->c_cc[VERASE] != _POSIX_VDISABLE ? bt->c_cc[VERASE] : 0;
	st->c_cc[IBCS2_VKILL] =
	    bt->c_cc[VKILL]  != _POSIX_VDISABLE ? bt->c_cc[VKILL]  : 0;
	if (bt->c_lflag & ICANON) {
		st->c_cc[IBCS2_VEOF] =
		    bt->c_cc[VEOF] != _POSIX_VDISABLE ? bt->c_cc[VEOF] : 0;
		st->c_cc[IBCS2_VEOL] =
		    bt->c_cc[VEOL] != _POSIX_VDISABLE ? bt->c_cc[VEOL] : 0;
	} else {
		st->c_cc[IBCS2_VMIN]  = bt->c_cc[VMIN];
		st->c_cc[IBCS2_VTIME] = bt->c_cc[VTIME];
	}
	st->c_cc[IBCS2_VEOL2] =
	    bt->c_cc[VEOL2]  != _POSIX_VDISABLE ? bt->c_cc[VEOL2]  : 0;
	st->c_cc[IBCS2_VSWTCH] =
	    0;
	st->c_cc[IBCS2_VSUSP] =
	    bt->c_cc[VSUSP]  != _POSIX_VDISABLE ? bt->c_cc[VSUSP]  : 0;
	st->c_cc[IBCS2_VSTART] =
	    bt->c_cc[VSTART] != _POSIX_VDISABLE ? bt->c_cc[VSTART] : 0;
	st->c_cc[IBCS2_VSTOP] =
	    bt->c_cc[VSTOP]  != _POSIX_VDISABLE ? bt->c_cc[VSTOP]  : 0;

	st->c_line = 0;
}

static void
stios2stio(struct ibcs2_termios *ts, struct ibcs2_termio *t)
{

	t->c_iflag = ts->c_iflag;
	t->c_oflag = ts->c_oflag;
	t->c_cflag = ts->c_cflag;
	t->c_lflag = ts->c_lflag;
	t->c_line  = ts->c_line;
	memcpy(t->c_cc, ts->c_cc, IBCS2_NCC);
}

static void
stio2stios(struct ibcs2_termio *t, struct ibcs2_termios *ts)
{

	ts->c_iflag = t->c_iflag;
	ts->c_oflag = t->c_oflag;
	ts->c_cflag = t->c_cflag;
	ts->c_lflag = t->c_lflag;
	ts->c_line  = t->c_line;
	memcpy(ts->c_cc, t->c_cc, IBCS2_NCC);
}

int
ibcs2_sys_ioctl(struct lwp *l, const struct ibcs2_sys_ioctl_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) cmd;
		syscallarg(void *) data;
	} */
	struct proc *p = l->l_proc;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	int (*ctl)(struct file *, u_long, void *, struct lwp *);
	struct termios bts;
	struct ibcs2_termios sts;
	struct ibcs2_termio st;
	struct sys_ioctl_args bsd_ua;
	int error, t;

	SCARG(&bsd_ua, fd) = SCARG(uap, fd);
	SCARG(&bsd_ua, data) = SCARG(uap, data);

	/* Handle the easy ones first */
	switch (SCARG(uap, cmd)) {
	case IBCS2_TIOCGWINSZ:
		SCARG(&bsd_ua, com) = TIOCGWINSZ;
		return sys_ioctl(l, &bsd_ua, retval);

	case IBCS2_TIOCSWINSZ:
		SCARG(&bsd_ua, com) = TIOCSWINSZ;
		return sys_ioctl(l, &bsd_ua, retval);

	case IBCS2_TIOCGPGRP:
		return copyout(&p->p_pgrp->pg_id, SCARG(uap, data),
		    sizeof(p->p_pgrp->pg_id));

	case IBCS2_TIOCSPGRP:	/* XXX - is uap->data a pointer to pgid? */
	    {
		struct sys_setpgid_args sa;

		SCARG(&sa, pid) = 0;
		SCARG(&sa, pgid) = (int)SCARG(uap, data);
		if ((error = sys_setpgid(l, &sa, retval)) != 0)
			return error;
		return 0;
	    }

	case IBCS2_TCGETSC:	/* SCO console - get scancode flags */
	case IBCS2_TCSETSC:	/* SCO console - set scancode flags */
		return ENOSYS;

	case IBCS2_SIOCSOCKSYS:
		return ibcs2_socksys(l, (const void *)uap, retval);

	case IBCS2_I_NREAD:     /* STREAMS */
		SCARG(&bsd_ua, com) = FIONREAD;
		return sys_ioctl(l, &bsd_ua, retval);
	default:
		break;
	}

	if ((fp = fd_getfile(fdp, SCARG(uap, fd))) == NULL) {
		DPRINTF(("ibcs2_ioctl(%d): bad fd %d ", p->p_pid,
			 SCARG(uap, fd)));
		return EBADF;
	}

	FILE_USE(fp);
	if ((fp->f_flag & (FREAD|FWRITE)) == 0) {
		DPRINTF(("ibcs2_ioctl(%d): bad fp flag ", p->p_pid));
		error = EBADF;
		goto out;
	}

	ctl = fp->f_ops->fo_ioctl;

	switch (SCARG(uap, cmd)) {
	case IBCS2_TCGETA:
	case IBCS2_XCGETA:
	case IBCS2_OXCGETA:
		if ((error = (*ctl)(fp, TIOCGETA, &bts, l)) != 0)
			goto out;

		btios2stios(&bts, &sts);
		if (SCARG(uap, cmd) == IBCS2_TCGETA) {
			stios2stio(&sts, &st);
			error = copyout(&st, SCARG(uap, data), sizeof(st));
			if (error)
				DPRINTF(("ibcs2_ioctl(%d): copyout failed ",
				     p->p_pid));
		} else
			error = copyout(&sts, SCARG(uap, data), sizeof(sts));
		break;

	case IBCS2_TCSETA:
	case IBCS2_TCSETAW:
	case IBCS2_TCSETAF:
		if ((error = copyin(SCARG(uap, data), &st, sizeof(st))) != 0) {
			DPRINTF(("ibcs2_ioctl(%d): TCSET copyin failed ",
			    p->p_pid));
			goto out;
		}

		/* get full BSD termios so we don't lose information */
		if ((error = (*ctl)(fp, TIOCGETA, &bts, l)) != 0) {
			DPRINTF(("ibcs2_ioctl(%d): TCSET ctl failed fd %d ",
			    p->p_pid, SCARG(uap, fd)));
			goto out;
		}

		/*
		 * convert to iBCS2 termios, copy in information from
		 * termio, and convert back, then set new values.
		 */
		btios2stios(&bts, &sts);
		stio2stios(&st, &sts);
		stios2btios(&sts, &bts);

		t = SCARG(uap, cmd) - IBCS2_TCSETA + TIOCSETA;
		error = (*ctl)(fp, t, &bts, l);
		break;

	case IBCS2_XCSETA:
	case IBCS2_XCSETAW:
	case IBCS2_XCSETAF:
		if ((error = copyin(SCARG(uap, data), &sts, sizeof(sts))) != 0)
			goto out;

		stios2btios(&sts, &bts);
		t = SCARG(uap, cmd) - IBCS2_XCSETA + TIOCSETA;
		error = (*ctl)(fp, t, &bts, l);
		break;

	case IBCS2_OXCSETA:
	case IBCS2_OXCSETAW:
	case IBCS2_OXCSETAF:
		if ((error = copyin(SCARG(uap, data), &sts, sizeof(sts))) != 0)
			goto out;
		stios2btios(&sts, &bts);
		t = SCARG(uap, cmd) - IBCS2_OXCSETA + TIOCSETA;
		error = (*ctl)(fp, t, &bts, l);
		break;

	case IBCS2_TCSBRK:
		t = (int) SCARG(uap, data);
		t = (t ? t : 1) * hz * 4;
		t /= 10;
		if ((error = (*ctl)(fp, TIOCSBRK, NULL, l)) != 0)
			goto out;
		error = tsleep(&t, PZERO | PCATCH, "ibcs2_tcsbrk", t);
		if (error == EINTR || error == ERESTART) {
			(void)(*ctl)(fp, TIOCCBRK, NULL, l);
			error = EINTR;
		} else
			error = (*ctl)(fp, TIOCCBRK, NULL, l);
		break;

	case IBCS2_TCXONC:
		switch ((int)SCARG(uap, data)) {
		case 0:
		case 1:
			DPRINTF(("ibcs2_ioctl(%d): TCXONC ", p->p_pid));
			error = ENOSYS;
			break;
		case 2:
			error = (*ctl)(fp, TIOCSTOP, NULL, l);
			break;
		case 3:
			error = (*ctl)(fp, TIOCSTART, (void *)1, l);
			break;
		default:
			error = EINVAL;
			break;
		}
		break;

	case IBCS2_TCFLSH:
		switch ((int)SCARG(uap, data)) {
		case 0:
			t = FREAD;
			break;
		case 1:
			t = FWRITE;
			break;
		case 2:
			t = FREAD | FWRITE;
			break;
		default:
			error = EINVAL;
			goto out;
		}
		error = (*ctl)(fp, TIOCFLUSH, &t, l);
		break;

	case IBCS2_FIONBIO:
		if ((error = copyin(SCARG(uap, data), &t, sizeof(t))) != 0)
			goto out;
		error = (*ctl)(fp, FIONBIO, (void *)&t, l);
		break;

	default:
		DPRINTF(("ibcs2_ioctl(%d): unknown cmd 0x%x ",
		    p->p_pid, SCARG(uap, cmd)));
		error = ENOSYS;
		break;
	}
out:
	FILE_UNUSE(fp, l);
	return error;
}

int
ibcs2_sys_gtty(struct lwp *l, const struct ibcs2_sys_gtty_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(struct sgttyb *) tb;
	} */
	struct proc *p = l->l_proc;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct sgttyb tb;
	struct ibcs2_sgttyb itb;
	int error;

	if ((fp = fd_getfile(fdp, SCARG(uap, fd))) == NULL) {
		DPRINTF(("ibcs2_sys_gtty(%d): bad fd %d ", p->p_pid,
			 SCARG(uap, fd)));
		return EBADF;
	}
	FILE_USE(fp);

	if ((fp->f_flag & (FREAD|FWRITE)) == 0) {
		DPRINTF(("ibcs2_sys_gtty(%d): bad fp flag ", p->p_pid));
		error = EBADF;
		goto out;
	}

	error = (*fp->f_ops->fo_ioctl)(fp, TIOCGETP, (void *)&tb, l);
	if (error)
		goto out;

	FILE_UNUSE(fp, l);

	itb.sg_ispeed = tb.sg_ispeed;
	itb.sg_ospeed = tb.sg_ospeed;
	itb.sg_erase = tb.sg_erase;
	itb.sg_kill = tb.sg_kill;
	itb.sg_flags = tb.sg_flags & ~(IBCS2_GHUPCL|IBCS2_GXTABS);
	return copyout((void *)&itb, SCARG(uap, tb), sizeof(itb));
out:
	FILE_UNUSE(fp, l);
	return error;
}
