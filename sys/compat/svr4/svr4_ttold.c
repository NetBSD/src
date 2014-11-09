/*	$NetBSD: svr4_ttold.c,v 1.34 2014/11/09 18:16:55 maxv Exp $	 */

/*-
 * Copyright (c) 1994, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: svr4_ttold.c,v 1.34 2014/11/09 18:16:55 maxv Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/termios.h>
#include <sys/tty.h>
#include <sys/socket.h>
#include <sys/mount.h>
#include <net/if.h>
#include <sys/ioctl_compat.h>

#include <sys/syscallargs.h>

#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_util.h>
#include <compat/svr4/svr4_signal.h>
#include <compat/svr4/svr4_lwp.h>
#include <compat/svr4/svr4_ucontext.h>
#include <compat/svr4/svr4_syscallargs.h>
#include <compat/svr4/svr4_stropts.h>
#include <compat/svr4/svr4_ttold.h>
#include <compat/svr4/svr4_ioctl.h>


static void svr4_tchars_to_bsd_tchars(const struct svr4_tchars *st,
					   struct tchars *bt);
static void bsd_tchars_to_svr4_tchars(const struct tchars *bt,
					   struct svr4_tchars *st);
static void svr4_sgttyb_to_bsd_sgttyb(const struct svr4_sgttyb *ss,
					   struct sgttyb *bs);
static void bsd_sgttyb_to_svr4_sgttyb(const struct sgttyb *bs,
					   struct svr4_sgttyb *ss);
static void svr4_ltchars_to_bsd_ltchars(const struct svr4_ltchars *sl,
					     struct ltchars *bl);
static void bsd_ltchars_to_svr4_ltchars(const struct ltchars *bl,
					     struct svr4_ltchars *sl);

#ifdef DEBUG_SVR4
static void print_svr4_sgttyb(const char *, struct svr4_sgttyb *);
static void print_svr4_tchars(const char *, struct svr4_tchars *);
static void print_svr4_ltchars(const char *, struct svr4_ltchars *);

static void
print_svr4_sgttyb(const char *str, struct svr4_sgttyb *ss)
{

	uprintf("%s\nispeed=%o ospeed=%o ", str, ss->sg_ispeed, ss->sg_ospeed);
	uprintf("erase=%o kill=%o flags=%o\n", ss->sg_erase, ss->sg_kill,
	    ss->sg_flags);
}

static void
print_svr4_tchars(const char *str, struct svr4_tchars *st)
{
	uprintf("%s\nintrc=%o quitc=%o ", str, st->t_intrc, st->t_quitc);
	uprintf("startc=%o stopc=%o eofc=%o brkc=%o\n", st->t_startc,
	    st->t_stopc, st->t_eofc, st->t_brkc);
}

static void
print_svr4_ltchars(const char *str, struct svr4_ltchars *sl)
{
	uprintf("%s\nsuspc=%o dsuspc=%o ", str, sl->t_suspc, sl->t_dsuspc);
	uprintf("rprntc=%o flushc=%o werasc=%o lnextc=%o\n", sl->t_rprntc,
	    sl->t_flushc, sl->t_werasc, sl->t_lnextc);
}
#endif /* DEBUG_SVR4 */

static void
svr4_tchars_to_bsd_tchars(const struct svr4_tchars *st, struct tchars *bt)
{
	bt->t_intrc  = st->t_intrc;
	bt->t_quitc  = st->t_quitc;
	bt->t_startc = st->t_startc;
	bt->t_stopc  = st->t_stopc;
	bt->t_eofc   = st->t_eofc;
	bt->t_brkc   = st->t_brkc;
}


static void
bsd_tchars_to_svr4_tchars(const struct tchars *bt, struct svr4_tchars *st)
{
	st->t_intrc  = bt->t_intrc;
	st->t_quitc  = bt->t_quitc;
	st->t_startc = bt->t_startc;
	st->t_stopc  = bt->t_stopc;
	st->t_eofc   = bt->t_eofc;
	st->t_brkc   = bt->t_brkc;
}


static void
svr4_sgttyb_to_bsd_sgttyb(const struct svr4_sgttyb *ss, struct sgttyb *bs)
{
	bs->sg_ispeed = ss->sg_ispeed;
	bs->sg_ospeed = ss->sg_ospeed;
	bs->sg_erase  =	ss->sg_erase;
	bs->sg_kill   = ss->sg_kill;
	bs->sg_flags  = ss->sg_flags;
}


static void
bsd_sgttyb_to_svr4_sgttyb(const struct sgttyb *bs, struct svr4_sgttyb *ss)
{
	ss->sg_ispeed = bs->sg_ispeed;
	ss->sg_ospeed = bs->sg_ospeed;
	ss->sg_erase  =	bs->sg_erase;
	ss->sg_kill   = bs->sg_kill;
	ss->sg_flags  = bs->sg_flags;
}


static void
svr4_ltchars_to_bsd_ltchars(const struct svr4_ltchars *sl, struct ltchars *bl)
{
	bl->t_suspc  = sl->t_suspc;
	bl->t_dsuspc = sl->t_dsuspc;
	bl->t_rprntc = sl->t_rprntc;
	bl->t_flushc = sl->t_flushc;
	bl->t_werasc = sl->t_werasc;
	bl->t_lnextc = sl->t_lnextc;
}


static void
bsd_ltchars_to_svr4_ltchars(const struct ltchars *bl, struct svr4_ltchars *sl)
{
	sl->t_suspc  = bl->t_suspc;
	sl->t_dsuspc = bl->t_dsuspc;
	sl->t_rprntc = bl->t_rprntc;
	sl->t_flushc = bl->t_flushc;
	sl->t_werasc = bl->t_werasc;
	sl->t_lnextc = bl->t_lnextc;
}


int
svr4_ttold_ioctl(file_t *fp, struct lwp *l, register_t *retval,
    int fd, u_long cmd, void *data)
{
	int			error;
	int (*ctl)(file_t *, u_long,  void *) = fp->f_ops->fo_ioctl;

	*retval = 0;

	switch (cmd) {
	case SVR4_TIOCGPGRP:
		{
			pid_t pid;

			if ((error = (*ctl)(fp, TIOCGPGRP, &pid)) != 0)
			    return error;

			DPRINTF(("TIOCGPGRP %d\n", pid));

			if ((error = copyout(&pid, data, sizeof(pid))) != 0)
				return error;

		}

	case SVR4_TIOCSPGRP:
		{
			pid_t pid;

			if ((error = copyin(data, &pid, sizeof(pid))) != 0)
				return error;

			DPRINTF(("TIOCSPGRP %d\n", pid));

			return (*ctl)(fp, TIOCSPGRP, &pid);
		}

	case SVR4_TIOCGSID:
		{
			pid_t pid;

			if ((error = (*ctl)(fp, TIOCGSID, &pid)) != 0)
				return error;

			DPRINTF(("TIOCGSID %d\n", pid));

			return copyout(&pid, data, sizeof(pid));
		}

	case SVR4_TIOCGETP:
		{
			struct sgttyb bs;
			struct svr4_sgttyb ss;

			error = (*ctl)(fp, TIOCGETP, &bs);
			if (error)
				return error;

			bsd_sgttyb_to_svr4_sgttyb(&bs, &ss);
#ifdef DEBUG_SVR4
			print_svr4_sgttyb("SVR4_TIOCGETP", &ss);
#endif /* DEBUG_SVR4 */
			return copyout(&ss, data, sizeof(ss));
		}

	case SVR4_TIOCSETP:
	case SVR4_TIOCSETN:
		{
			struct sgttyb bs;
			struct svr4_sgttyb ss;

			if ((error = copyin(data, &ss, sizeof(ss))) != 0)
				return error;

			svr4_sgttyb_to_bsd_sgttyb(&ss, &bs);
#ifdef DEBUG_SVR4
			print_svr4_sgttyb("SVR4_TIOCSET{P,N}", &ss);
#endif /* DEBUG_SVR4 */
			cmd = (cmd == SVR4_TIOCSETP) ? TIOCSETP : TIOCSETN;
			return (*ctl)(fp, cmd, &bs);
		}

	case SVR4_TIOCGETC:
		{
			struct tchars bt;
			struct svr4_tchars st;

			error = (*ctl)(fp, TIOCGETC, &bt);
			if (error)
				return error;

			bsd_tchars_to_svr4_tchars(&bt, &st);
#ifdef DEBUG_SVR4
			print_svr4_tchars("SVR4_TIOCGETC", &st);
#endif /* DEBUG_SVR4 */
			return copyout(&st, data, sizeof(st));
		}

	case SVR4_TIOCSETC:
		{
			struct tchars bt;
			struct svr4_tchars st;

			if ((error = copyin(data, &st, sizeof(st))) != 0)
				return error;

			svr4_tchars_to_bsd_tchars(&st, &bt);
#ifdef DEBUG_SVR4
			print_svr4_tchars("SVR4_TIOCSETC", &st);
#endif /* DEBUG_SVR4 */
			return (*ctl)(fp, TIOCSETC, &bt);
		}

	case SVR4_TIOCGLTC:
		{
			struct ltchars bl;
			struct svr4_ltchars sl;

			error = (*ctl)(fp, TIOCGLTC, &bl);
			if (error)
				return error;

			bsd_ltchars_to_svr4_ltchars(&bl, &sl);
#ifdef DEBUG_SVR4
			print_svr4_ltchars("SVR4_TIOCGLTC", &sl);
#endif /* DEBUG_SVR4 */
			return copyout(&sl, data, sizeof(sl));
		}

	case SVR4_TIOCSLTC:
		{
			struct ltchars bl;
			struct svr4_ltchars sl;

			if ((error = copyin(data, &sl, sizeof(sl))) != 0)
				return error;

			svr4_ltchars_to_bsd_ltchars(&sl, &bl);
#ifdef DEBUG_SVR4
			print_svr4_ltchars("SVR4_TIOCSLTC", &sl);
#endif /* DEBUG_SVR4 */
			return (*ctl)(fp, TIOCSLTC, &bl);
		}

	case SVR4_TIOCLGET:
		{
			int flags;
			if ((error = (*ctl)(fp, TIOCLGET, &flags)) != 0)
				return error;
			DPRINTF(("SVR4_TIOCLGET %o\n", flags));
			return copyout(&flags, data, sizeof(flags));
		}

	case SVR4_TIOCLSET:
	case SVR4_TIOCLBIS:
	case SVR4_TIOCLBIC:
		{
			int flags;

			if ((error = copyin(data, &flags, sizeof(flags))) != 0)
				return error;

			switch (cmd) {
			case SVR4_TIOCLSET:
				cmd = TIOCLSET;
				break;
			case SVR4_TIOCLBIS:
				cmd = TIOCLBIS;
				break;
			case SVR4_TIOCLBIC:
				cmd = TIOCLBIC;
				break;
			}

			DPRINTF(("SVR4_TIOCL{SET,BIS,BIC} %o\n", flags));
			return (*ctl)(fp, cmd, &flags);
		}

	default:
		DPRINTF(("Unknown svr4 ttold %lx\n", cmd));
		return 0;	/* ENOSYS really */
	}
}
