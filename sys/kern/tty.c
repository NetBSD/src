/*	$NetBSD: tty.c,v 1.208.4.1 2008/01/02 21:56:18 bouyer Exp $	*/

/*-
 * Copyright (c) 1982, 1986, 1990, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)tty.c	8.13 (Berkeley) 1/9/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tty.c,v 1.208.4.1 2008/01/02 21:56:18 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#define	TTYDEFCHARS
#include <sys/tty.h>
#undef	TTYDEFCHARS
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/dkstat.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/syslog.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/signalvar.h>
#include <sys/resourcevar.h>
#include <sys/poll.h>
#include <sys/kprintf.h>
#include <sys/namei.h>
#include <sys/sysctl.h>
#include <sys/kauth.h>
#include <sys/intr.h>

#include <machine/stdarg.h>

static int	ttnread(struct tty *);
static void	ttyblock(struct tty *);
static void	ttyecho(int, struct tty *);
static void	ttyrubo(struct tty *, int);
static void	ttyprintf_nolock(struct tty *, const char *fmt, ...)
    __attribute__((__format__(__printf__,2,3)));
static int	proc_compare(struct proc *, struct proc *);
static void	ttysigintr(void *);

/* Symbolic sleep message strings. */
const char	ttclos[] = "ttycls";
const char	ttopen[] = "ttyopn";
const char	ttybg[] = "ttybg";
const char	ttyin[] = "ttyin";
const char	ttyout[] = "ttyout";

/*
 * Used to determine whether we still have a connection.  This is true in
 * one of 3 cases:
 * 1) We have carrier.
 * 2) It's a locally attached terminal, and we are therefore ignoring carrier.
 * 3) We're using a flow control mechanism that overloads the carrier signal.
 */
#define	CONNECTED(tp)	(ISSET(tp->t_state, TS_CARR_ON) ||	\
			 ISSET(tp->t_cflag, CLOCAL | MDMBUF))

/*
 * Table with character classes and parity. The 8th bit indicates parity,
 * the 7th bit indicates the character is an alphameric or underscore (for
 * ALTWERASE), and the low 6 bits indicate delay type.  If the low 6 bits
 * are 0 then the character needs no special processing on output; classes
 * other than 0 might be translated or (not currently) require delays.
 */
#define	E	0x00	/* Even parity. */
#define	O	0x80	/* Odd parity. */
#define	PARITY(c)	(char_type[c] & O)

#define	ALPHA	0x40	/* Alpha or underscore. */
#define	ISALPHA(c)	(char_type[(c) & TTY_CHARMASK] & ALPHA)

#define	CCLASSMASK	0x3f
#define	CCLASS(c)	(char_type[c] & CCLASSMASK)

#define	BS	BACKSPACE
#define	CC	CONTROL
#define	CR	RETURN
#define	NA	ORDINARY | ALPHA
#define	NL	NEWLINE
#define	NO	ORDINARY
#define	TB	TAB
#define	VT	VTAB

unsigned char const char_type[] = {
	E|CC, O|CC, O|CC, E|CC, O|CC, E|CC, E|CC, O|CC,	/* nul - bel */
	O|BS, E|TB, E|NL, O|CC, E|VT, O|CR, O|CC, E|CC,	/* bs - si */
	O|CC, E|CC, E|CC, O|CC, E|CC, O|CC, O|CC, E|CC,	/* dle - etb */
	E|CC, O|CC, O|CC, E|CC, O|CC, E|CC, E|CC, O|CC,	/* can - us */
	O|NO, E|NO, E|NO, O|NO, E|NO, O|NO, O|NO, E|NO,	/* sp - ' */
	E|NO, O|NO, O|NO, E|NO, O|NO, E|NO, E|NO, O|NO,	/* ( - / */
	E|NA, O|NA, O|NA, E|NA, O|NA, E|NA, E|NA, O|NA,	/* 0 - 7 */
	O|NA, E|NA, E|NO, O|NO, E|NO, O|NO, O|NO, E|NO,	/* 8 - ? */
	O|NO, E|NA, E|NA, O|NA, E|NA, O|NA, O|NA, E|NA,	/* @ - G */
	E|NA, O|NA, O|NA, E|NA, O|NA, E|NA, E|NA, O|NA,	/* H - O */
	E|NA, O|NA, O|NA, E|NA, O|NA, E|NA, E|NA, O|NA,	/* P - W */
	O|NA, E|NA, E|NA, O|NO, E|NO, O|NO, O|NO, O|NA,	/* X - _ */
	E|NO, O|NA, O|NA, E|NA, O|NA, E|NA, E|NA, O|NA,	/* ` - g */
	O|NA, E|NA, E|NA, O|NA, E|NA, O|NA, O|NA, E|NA,	/* h - o */
	O|NA, E|NA, E|NA, O|NA, E|NA, O|NA, O|NA, E|NA,	/* p - w */
	E|NA, O|NA, O|NA, E|NO, O|NO, E|NO, E|NO, O|CC,	/* x - del */
	/*
	 * Meta chars; should be settable per character set;
	 * for now, treat them all as normal characters.
	 */
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
};
#undef	BS
#undef	CC
#undef	CR
#undef	NA
#undef	NL
#undef	NO
#undef	TB
#undef	VT

static struct ttylist_head tty_sigqueue = TAILQ_HEAD_INITIALIZER(tty_sigqueue);
static void *tty_sigsih;

struct ttylist_head ttylist = TAILQ_HEAD_INITIALIZER(ttylist);
int tty_count;
kmutex_t tty_lock;

POOL_INIT(tty_pool, sizeof(struct tty), 0, 0, 0, "ttypl",
    &pool_allocator_nointr, IPL_NONE);

uint64_t tk_cancc;
uint64_t tk_nin;
uint64_t tk_nout;
uint64_t tk_rawcc;

SYSCTL_SETUP(sysctl_kern_tkstat_setup, "sysctl kern.tkstat subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "kern", NULL,
		       NULL, 0, NULL, 0,
		       CTL_KERN, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "tkstat",
		       SYSCTL_DESCR("Number of characters sent and and "
				    "received on ttys"),
		       NULL, 0, NULL, 0,
		       CTL_KERN, KERN_TKSTAT, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_QUAD, "nin",
		       SYSCTL_DESCR("Total number of tty input characters"),
		       NULL, 0, &tk_nin, 0,
		       CTL_KERN, KERN_TKSTAT, KERN_TKSTAT_NIN, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_QUAD, "nout",
		       SYSCTL_DESCR("Total number of tty output characters"),
		       NULL, 0, &tk_nout, 0,
		       CTL_KERN, KERN_TKSTAT, KERN_TKSTAT_NOUT, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_QUAD, "cancc",
		       SYSCTL_DESCR("Number of canonical tty input characters"),
		       NULL, 0, &tk_cancc, 0,
		       CTL_KERN, KERN_TKSTAT, KERN_TKSTAT_CANCC, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_QUAD, "rawcc",
		       SYSCTL_DESCR("Number of raw tty input characters"),
		       NULL, 0, &tk_rawcc, 0,
		       CTL_KERN, KERN_TKSTAT, KERN_TKSTAT_RAWCC, CTL_EOL);
}

int
ttyopen(struct tty *tp, int dialout, int nonblock)
{
	int	error;

	error = 0;

	mutex_spin_enter(&tty_lock);

	if (dialout) {
		/*
		 * If the device is already open for non-dialout, fail.
		 * Otherwise, set TS_DIALOUT to block any pending non-dialout
		 * opens.
		 */
		if (ISSET(tp->t_state, TS_ISOPEN) &&
		    !ISSET(tp->t_state, TS_DIALOUT)) {
			error = EBUSY;
			goto out;
		}
		SET(tp->t_state, TS_DIALOUT);
	} else {
		if (!nonblock) {
			/*
			 * Wait for carrier.  Also wait for any dialout
			 * processes to close the tty first.
			 */
			while (ISSET(tp->t_state, TS_DIALOUT) ||
			       !CONNECTED(tp)) {
				tp->t_wopen++;
				error = ttysleep(tp, &tp->t_rawq.c_cv, true, 0);
				tp->t_wopen--;
				if (error)
					goto out;
			}
		} else {
			/*
			 * Don't allow a non-blocking non-dialout open if the
			 * device is already open for dialout.
			 */
			if (ISSET(tp->t_state, TS_DIALOUT)) {
				error = EBUSY;
				goto out;
			}
		}
	}

out:
	mutex_spin_exit(&tty_lock);
	return (error);
}

/*
 * Initial open of tty, or (re)entry to standard tty line discipline.
 */
int
ttylopen(dev_t device, struct tty *tp)
{

	mutex_spin_enter(&tty_lock);
	tp->t_dev = device;
	if (!ISSET(tp->t_state, TS_ISOPEN)) {
		SET(tp->t_state, TS_ISOPEN);
		memset(&tp->t_winsize, 0, sizeof(tp->t_winsize));
#ifdef COMPAT_OLDTTY
		tp->t_flags = 0;
#endif
	}
	mutex_spin_exit(&tty_lock);
	return (0);
}

/*
 * Handle close() on a tty line: flush and set to initial state,
 * bumping generation number so that pending read/write calls
 * can detect recycling of the tty.
 */
int
ttyclose(struct tty *tp)
{
	extern struct tty *constty;	/* Temporary virtual console. */
	struct session *sess;

	mutex_spin_enter(&tty_lock);

	if (constty == tp)
		constty = NULL;

	ttyflush(tp, FREAD | FWRITE);

	tp->t_gen++;
	tp->t_pgrp = NULL;
	tp->t_state = 0;
	sess = tp->t_session;
	tp->t_session = NULL;

	mutex_spin_exit(&tty_lock);

	mutex_enter(&proclist_lock);
	if (sess != NULL)
		SESSRELE(sess);
	mutex_exit(&proclist_lock);

	return (0);
}

#define	FLUSHQ(q) {							\
	if ((q)->c_cc)							\
		ndflush(q, (q)->c_cc);					\
}

/*
 * This macro is used in canonical mode input processing, where a read
 * request shall not return unless a 'line delimiter' ('\n') or 'break'
 * (EOF, EOL, EOL2) character (or a signal) has been received. As EOL2
 * is an extension to the POSIX.1 defined set of special characters,
 * recognize it only if IEXTEN is set in the set of local flags.
 */
#define	TTBREAKC(c, lflg)						\
	((c) == '\n' || (((c) == cc[VEOF] || (c) == cc[VEOL] ||		\
	((c) == cc[VEOL2] && ISSET(lflg, IEXTEN))) && (c) != _POSIX_VDISABLE))



/*
 * ttyinput() helper.
 * Call with the tty lock held.
 */
static int
ttyinput_wlock(int c, struct tty *tp)
{
	int	iflag, lflag, i, error;
	u_char	*cc;

	KASSERT(mutex_owned(&tty_lock));

	/*
	 * If input is pending take it first.
	 */
	lflag = tp->t_lflag;
	if (ISSET(lflag, PENDIN))
		ttypend(tp);
	/*
	 * Gather stats.
	 */
	if (ISSET(lflag, ICANON)) {
		++tk_cancc;
		++tp->t_cancc;
	} else {
		++tk_rawcc;
		++tp->t_rawcc;
	}
	++tk_nin;

	cc = tp->t_cc;

	/*
	 * Handle exceptional conditions (break, parity, framing).
	 */
	iflag = tp->t_iflag;
	if ((error = (ISSET(c, TTY_ERRORMASK))) != 0) {
		CLR(c, TTY_ERRORMASK);
		if (ISSET(error, TTY_FE) && c == 0) {		/* Break. */
			if (ISSET(iflag, IGNBRK))
				return (0);
			else if (ISSET(iflag, BRKINT)) {
				ttyflush(tp, FREAD | FWRITE);
				ttysig(tp, TTYSIG_PG1, SIGINT);
				return (0);
			} else if (ISSET(iflag, PARMRK))
				goto parmrk;
		} else if ((ISSET(error, TTY_PE) && ISSET(iflag, INPCK)) ||
		    ISSET(error, TTY_FE)) {
			if (ISSET(iflag, IGNPAR))
				return (0);
			else if (ISSET(iflag, PARMRK)) {
 parmrk:			(void)putc(0377 | TTY_QUOTE, &tp->t_rawq);
				(void)putc(0    | TTY_QUOTE, &tp->t_rawq);
				(void)putc(c    | TTY_QUOTE, &tp->t_rawq);
				return (0);
			} else
				c = 0;
		}
	} else if (c == 0377 &&
	    ISSET(iflag, ISTRIP|IGNPAR|INPCK|PARMRK) == (INPCK|PARMRK)) {
		/* "Escape" a valid character of '\377'. */
		(void)putc(0377 | TTY_QUOTE, &tp->t_rawq);
		(void)putc(0377 | TTY_QUOTE, &tp->t_rawq);
		goto endcase;
	}

	/*
	 * In tandem mode, check high water mark.
	 */
	if (ISSET(iflag, IXOFF) || ISSET(tp->t_cflag, CHWFLOW))
		ttyblock(tp);
	if (!ISSET(tp->t_state, TS_TYPEN) && ISSET(iflag, ISTRIP))
		CLR(c, 0x80);
	if (!ISSET(lflag, EXTPROC)) {
		/*
		 * Check for literal nexting very first
		 */
		if (ISSET(tp->t_state, TS_LNCH)) {
			SET(c, TTY_QUOTE);
			CLR(tp->t_state, TS_LNCH);
		}
		/*
		 * Scan for special characters.  This code
		 * is really just a big case statement with
		 * non-constant cases.  The bottom of the
		 * case statement is labeled ``endcase'', so goto
		 * it after a case match, or similar.
		 */

		/*
		 * Control chars which aren't controlled
		 * by ICANON, ISIG, or IXON.
		 */
		if (ISSET(lflag, IEXTEN)) {
			if (CCEQ(cc[VLNEXT], c)) {
				if (ISSET(lflag, ECHO)) {
					if (ISSET(lflag, ECHOE)) {
						(void)ttyoutput('^', tp);
						(void)ttyoutput('\b', tp);
					} else
						ttyecho(c, tp);
				}
				SET(tp->t_state, TS_LNCH);
				goto endcase;
			}
			if (CCEQ(cc[VDISCARD], c)) {
				if (ISSET(lflag, FLUSHO))
					CLR(tp->t_lflag, FLUSHO);
				else {
					ttyflush(tp, FWRITE);
					ttyecho(c, tp);
					if (tp->t_rawq.c_cc + tp->t_canq.c_cc)
						ttyretype(tp);
					SET(tp->t_lflag, FLUSHO);
				}
				goto startoutput;
			}
		}
		/*
		 * Signals.
		 */
		if (ISSET(lflag, ISIG)) {
			if (CCEQ(cc[VINTR], c) || CCEQ(cc[VQUIT], c)) {
				if (!ISSET(lflag, NOFLSH))
					ttyflush(tp, FREAD | FWRITE);
				ttyecho(c, tp);
				ttysig(tp, TTYSIG_PG1, CCEQ(cc[VINTR], c) ?
				    SIGINT : SIGQUIT);
				goto endcase;
			}
			if (CCEQ(cc[VSUSP], c)) {
				if (!ISSET(lflag, NOFLSH))
					ttyflush(tp, FREAD);
				ttyecho(c, tp);
				ttysig(tp, TTYSIG_PG1, SIGTSTP);
				goto endcase;
			}
		}
		/*
		 * Handle start/stop characters.
		 */
		if (ISSET(iflag, IXON)) {
			if (CCEQ(cc[VSTOP], c)) {
				if (!ISSET(tp->t_state, TS_TTSTOP)) {
					SET(tp->t_state, TS_TTSTOP);
					cdev_stop(tp, 0);
					return (0);
				}
				if (!CCEQ(cc[VSTART], c))
					return (0);
				/*
				 * if VSTART == VSTOP then toggle
				 */
				goto endcase;
			}
			if (CCEQ(cc[VSTART], c))
				goto restartoutput;
		}
		/*
		 * IGNCR, ICRNL, & INLCR
		 */
		if (c == '\r') {
			if (ISSET(iflag, IGNCR))
				goto endcase;
			else if (ISSET(iflag, ICRNL))
				c = '\n';
		} else if (c == '\n' && ISSET(iflag, INLCR))
			c = '\r';
	}
	if (!ISSET(lflag, EXTPROC) && ISSET(lflag, ICANON)) {
		/*
		 * From here on down canonical mode character
		 * processing takes place.
		 */
		/*
		 * erase (^H / ^?)
		 */
		if (CCEQ(cc[VERASE], c)) {
			if (tp->t_rawq.c_cc)
				ttyrub(unputc(&tp->t_rawq), tp);
			goto endcase;
		}
		/*
		 * kill (^U)
		 */
		if (CCEQ(cc[VKILL], c)) {
			if (ISSET(lflag, ECHOKE) &&
			    tp->t_rawq.c_cc == tp->t_rocount &&
			    !ISSET(lflag, ECHOPRT))
				while (tp->t_rawq.c_cc)
					ttyrub(unputc(&tp->t_rawq), tp);
			else {
				ttyecho(c, tp);
				if (ISSET(lflag, ECHOK) ||
				    ISSET(lflag, ECHOKE))
					ttyecho('\n', tp);
				FLUSHQ(&tp->t_rawq);
				tp->t_rocount = 0;
			}
			CLR(tp->t_state, TS_LOCAL);
			goto endcase;
		}
		/*
		 * Extensions to the POSIX.1 GTI set of functions.
		 */
		if (ISSET(lflag, IEXTEN)) {
			/*
			 * word erase (^W)
			 */
			if (CCEQ(cc[VWERASE], c)) {
				int alt = ISSET(lflag, ALTWERASE);
				int ctype;

				/*
				 * erase whitespace
				 */
				while ((c = unputc(&tp->t_rawq)) == ' ' ||
				    c == '\t')
					ttyrub(c, tp);
				if (c == -1)
					goto endcase;
				/*
				 * erase last char of word and remember the
				 * next chars type (for ALTWERASE)
				 */
				ttyrub(c, tp);
				c = unputc(&tp->t_rawq);
				if (c == -1)
					goto endcase;
				if (c == ' ' || c == '\t') {
					(void)putc(c, &tp->t_rawq);
					goto endcase;
				}
				ctype = ISALPHA(c);
				/*
				 * erase rest of word
				 */
				do {
					ttyrub(c, tp);
					c = unputc(&tp->t_rawq);
					if (c == -1)
						goto endcase;
				} while (c != ' ' && c != '\t' &&
				    (alt == 0 || ISALPHA(c) == ctype));
				(void)putc(c, &tp->t_rawq);
				goto endcase;
			}
			/*
			 * reprint line (^R)
			 */
			if (CCEQ(cc[VREPRINT], c)) {
				ttyretype(tp);
				goto endcase;
			}
			/*
			 * ^T - kernel info and generate SIGINFO
			 */
			if (CCEQ(cc[VSTATUS], c)) {
				if (!ISSET(lflag, NOKERNINFO))
					ttyinfo(tp, 1);
				if (ISSET(lflag, ISIG))
					ttysig(tp, TTYSIG_PG1, SIGINFO);
				goto endcase;
			}
		}
	}
	/*
	 * Check for input buffer overflow
	 */
	if (tp->t_rawq.c_cc + tp->t_canq.c_cc >= TTYHOG) {
		if (ISSET(iflag, IMAXBEL)) {
			if (tp->t_outq.c_cc < tp->t_hiwat)
				(void)ttyoutput(CTRL('g'), tp);
		} else
			ttyflush(tp, FREAD | FWRITE);
		goto endcase;
	}
	/*
	 * Put data char in q for user and
	 * wakeup on seeing a line delimiter.
	 */
	if (putc(c, &tp->t_rawq) >= 0) {
		if (!ISSET(lflag, ICANON)) {
			ttwakeup(tp);
			ttyecho(c, tp);
			goto endcase;
		}
		if (TTBREAKC(c, lflag)) {
			tp->t_rocount = 0;
			catq(&tp->t_rawq, &tp->t_canq);
			ttwakeup(tp);
		} else if (tp->t_rocount++ == 0)
			tp->t_rocol = tp->t_column;
		if (ISSET(tp->t_state, TS_ERASE)) {
			/*
			 * end of prterase \.../
			 */
			CLR(tp->t_state, TS_ERASE);
			(void)ttyoutput('/', tp);
		}
		i = tp->t_column;
		ttyecho(c, tp);
		if (CCEQ(cc[VEOF], c) && ISSET(lflag, ECHO)) {
			/*
			 * Place the cursor over the '^' of the ^D.
			 */
			i = min(2, tp->t_column - i);
			while (i > 0) {
				(void)ttyoutput('\b', tp);
				i--;
			}
		}
	}
 endcase:
	/*
	 * IXANY means allow any character to restart output.
	 */
	if (ISSET(tp->t_state, TS_TTSTOP) &&
	    !ISSET(iflag, IXANY) && cc[VSTART] != cc[VSTOP]) {
		return (0);
	}
 restartoutput:
	CLR(tp->t_lflag, FLUSHO);
	CLR(tp->t_state, TS_TTSTOP);
 startoutput:
	return (ttstart(tp));
}

/*
 * Process input of a single character received on a tty.
 *
 * XXX - this is a hack, all drivers must changed to acquire the
 *	 lock before calling linesw->l_rint()
 */
int
ttyinput(int c, struct tty *tp)
{
	int error;

	/*
	 * Unless the receiver is enabled, drop incoming data.
	 */
	if (!ISSET(tp->t_cflag, CREAD))
		return (0);

	mutex_spin_enter(&tty_lock);
	error = ttyinput_wlock(c, tp);
	mutex_spin_exit(&tty_lock);

	return (error);
}

/*
 * Output a single character on a tty, doing output processing
 * as needed (expanding tabs, newline processing, etc.).
 * Returns < 0 if succeeds, otherwise returns char to resend.
 * Must be recursive.
 *
 * Call with tty lock held.
 */
int
ttyoutput(int c, struct tty *tp)
{
	long	oflag;
	int	col, notout;

	KASSERT(mutex_owned(&tty_lock));

	oflag = tp->t_oflag;
	if (!ISSET(oflag, OPOST)) {
		tk_nout++;
		tp->t_outcc++;
		if (!ISSET(tp->t_lflag, FLUSHO) && putc(c, &tp->t_outq))
			return (c);
		return (-1);
	}
	/*
	 * Do tab expansion if OXTABS is set.  Special case if we do external
	 * processing, we don't do the tab expansion because we'll probably
	 * get it wrong.  If tab expansion needs to be done, let it happen
	 * externally.
	 */
	CLR(c, ~TTY_CHARMASK);
	if (c == '\t' &&
	    ISSET(oflag, OXTABS) && !ISSET(tp->t_lflag, EXTPROC)) {
		c = 8 - (tp->t_column & 7);
		if (ISSET(tp->t_lflag, FLUSHO)) {
			notout = 0;
		} else {
			notout = b_to_q("        ", c, &tp->t_outq);
			c -= notout;
			tk_nout += c;
			tp->t_outcc += c;
		}
		tp->t_column += c;
		return (notout ? '\t' : -1);
	}
	if (c == CEOT && ISSET(oflag, ONOEOT))
		return (-1);

	/*
	 * Newline translation: if ONLCR is set,
	 * translate newline into "\r\n".
	 */
	if (c == '\n' && ISSET(tp->t_oflag, ONLCR)) {
		tk_nout++;
		tp->t_outcc++;
		if (!ISSET(tp->t_lflag, FLUSHO) && putc('\r', &tp->t_outq))
			return (c);
	}
	/* If OCRNL is set, translate "\r" into "\n". */
	else if (c == '\r' && ISSET(tp->t_oflag, OCRNL))
		c = '\n';
	/* If ONOCR is set, don't transmit CRs when on column 0. */
	else if (c == '\r' && ISSET(tp->t_oflag, ONOCR) && tp->t_column == 0)
		return (-1);

	tk_nout++;
	tp->t_outcc++;
	if (!ISSET(tp->t_lflag, FLUSHO) && putc(c, &tp->t_outq))
		return (c);

	col = tp->t_column;
	switch (CCLASS(c)) {
	case BACKSPACE:
		if (col > 0)
			--col;
		break;
	case CONTROL:
		break;
	case NEWLINE:
		if (ISSET(tp->t_oflag, ONLCR | ONLRET))
			col = 0;
		break;
	case RETURN:
		col = 0;
		break;
	case ORDINARY:
		++col;
		break;
	case TAB:
		col = (col + 8) & ~7;
		break;
	}
	tp->t_column = col;
	return (-1);
}

/*
 * Ioctls for all tty devices.  Called after line-discipline specific ioctl
 * has been called to do discipline-specific functions and/or reject any
 * of these ioctl commands.
 */
/* ARGSUSED */
int
ttioctl(struct tty *tp, u_long cmd, void *data, int flag, struct lwp *l)
{
	extern struct tty *constty;	/* Temporary virtual console. */
	struct proc *p = l ? l->l_proc : NULL;
	struct linesw	*lp;
	int		s, error;
	struct nameidata nd;

	/* If the ioctl involves modification, hang if in the background. */
	switch (cmd) {
	case  TIOCFLUSH:
	case  TIOCDRAIN:
	case  TIOCSBRK:
	case  TIOCCBRK:
	case  TIOCSTART:
	case  TIOCSETA:
	case  TIOCSETD:
	case  TIOCSLINED:
	case  TIOCSETAF:
	case  TIOCSETAW:
#ifdef notdef
	case  TIOCSPGRP:
	case  FIOSETOWN:
#endif
	case  TIOCSTAT:
	case  TIOCSTI:
	case  TIOCSWINSZ:
#ifdef COMPAT_OLDTTY
	case  TIOCLBIC:
	case  TIOCLBIS:
	case  TIOCLSET:
	case  TIOCSETC:
	case OTIOCSETD:
	case  TIOCSETN:
	case  TIOCSETP:
	case  TIOCSLTC:
#endif
		mutex_spin_enter(&tty_lock);
		while (isbackground(curproc, tp) &&
		    p->p_pgrp->pg_jobc && (p->p_sflag & PS_PPWAIT) == 0 &&
		    !sigismasked(l, SIGTTOU)) {
			mutex_spin_exit(&tty_lock);

			mutex_enter(&proclist_mutex);
			pgsignal(p->p_pgrp, SIGTTOU, 1);
			mutex_exit(&proclist_mutex);
			
			mutex_spin_enter(&tty_lock);
			error = ttysleep(tp, &lbolt, true, 0);
			if (error) {
				mutex_spin_exit(&tty_lock);
				return (error);
			}
		}
		mutex_spin_exit(&tty_lock);
		break;
	}

	switch (cmd) {			/* Process the ioctl. */
	case FIOASYNC:			/* set/clear async i/o */
		mutex_spin_enter(&tty_lock);
		if (*(int *)data)
			SET(tp->t_state, TS_ASYNC);
		else
			CLR(tp->t_state, TS_ASYNC);
		mutex_spin_exit(&tty_lock);
		break;
	case FIONBIO:			/* set/clear non-blocking i/o */
		break;			/* XXX: delete. */
	case FIONREAD:			/* get # bytes to read */
		mutex_spin_enter(&tty_lock);
		*(int *)data = ttnread(tp);
		mutex_spin_exit(&tty_lock);
		break;
	case FIONWRITE:			/* get # bytes to written & unsent */
		mutex_spin_enter(&tty_lock);
		*(int *)data = tp->t_outq.c_cc;
		mutex_spin_exit(&tty_lock);
		break;
	case FIONSPACE:			/* get # bytes to written & unsent */
		mutex_spin_enter(&tty_lock);
		*(int *)data = tp->t_outq.c_cn - tp->t_outq.c_cc;
		mutex_spin_exit(&tty_lock);
		break;
	case TIOCEXCL:			/* set exclusive use of tty */
		mutex_spin_enter(&tty_lock);
		SET(tp->t_state, TS_XCLUDE);
		mutex_spin_exit(&tty_lock);
		break;
	case TIOCFLUSH: {		/* flush buffers */
		int flags = *(int *)data;

		if (flags == 0)
			flags = FREAD | FWRITE;
		else
			flags &= FREAD | FWRITE;
		mutex_spin_enter(&tty_lock);
		ttyflush(tp, flags);
		mutex_spin_exit(&tty_lock);
		break;
	}
	case TIOCCONS:			/* become virtual console */
		if (*(int *)data) {
			if (constty && constty != tp &&
			    ISSET(constty->t_state, TS_CARR_ON | TS_ISOPEN) ==
			    (TS_CARR_ON | TS_ISOPEN))
				return EBUSY;

			NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_SYSSPACE,
			    "/dev/console");
			if ((error = namei(&nd)) != 0)
				return error;
			error = VOP_ACCESS(nd.ni_vp, VREAD, l->l_cred);
			vput(nd.ni_vp);
			if (error)
				return error;

			constty = tp;
		} else if (tp == constty)
			constty = NULL;
		break;
	case TIOCDRAIN:			/* wait till output drained */
		if ((error = ttywait(tp)) != 0)
			return (error);
		break;
	case TIOCGETA: {		/* get termios struct */
		struct termios *t = (struct termios *)data;

		memcpy(t, &tp->t_termios, sizeof(struct termios));
		break;
	}
	case TIOCGETD:			/* get line discipline (old) */
		*(int *)data = tp->t_linesw->l_no;
		break;
	case TIOCGLINED:		/* get line discipline (new) */
		(void)strncpy((char *)data, tp->t_linesw->l_name,
		    TTLINEDNAMELEN - 1);
		break;
	case TIOCGWINSZ:		/* get window size */
		*(struct winsize *)data = tp->t_winsize;
		break;
	case FIOGETOWN:
		if (tp->t_session != NULL && !isctty(p, tp))
			return (ENOTTY);
		*(int *)data = tp->t_pgrp ? -tp->t_pgrp->pg_id : 0;
		break;
	case TIOCGPGRP:			/* get pgrp of tty */
		if (!isctty(p, tp))
			return (ENOTTY);
		*(int *)data = tp->t_pgrp ? tp->t_pgrp->pg_id : NO_PGID;
		break;
	case TIOCGSID:			/* get sid of tty */
		if (!isctty(p, tp))
			return (ENOTTY);
		*(int *)data = tp->t_session->s_sid;
		break;
#ifdef TIOCHPCL
	case TIOCHPCL:			/* hang up on last close */
		mutex_spin_enter(&tty_lock);
		SET(tp->t_cflag, HUPCL);
		mutex_spin_exit(&tty_lock);
		break;
#endif
	case TIOCNXCL:			/* reset exclusive use of tty */
		mutex_spin_enter(&tty_lock);
		CLR(tp->t_state, TS_XCLUDE);
		mutex_spin_exit(&tty_lock);
		break;
	case TIOCOUTQ:			/* output queue size */
		*(int *)data = tp->t_outq.c_cc;
		break;
	case TIOCSETA:			/* set termios struct */
	case TIOCSETAW:			/* drain output, set */
	case TIOCSETAF: {		/* drn out, fls in, set */
		struct termios *t = (struct termios *)data;

		if (cmd == TIOCSETAW || cmd == TIOCSETAF) {
			if ((error = ttywait(tp)) != 0)
				return (error);

			if (cmd == TIOCSETAF) {
				mutex_spin_enter(&tty_lock);
				ttyflush(tp, FREAD);
				mutex_spin_exit(&tty_lock);
			}
		}

		s = spltty();
		/*
		 * XXXSMP - some drivers call back on us from t_param(), so
		 *	    don't take the tty spin lock here.
		 *	    require t_param() to unlock upon callback?
		 */
		/* wanted here: mutex_spin_enter(&tty_lock); */
		if (!ISSET(t->c_cflag, CIGNORE)) {
			/*
			 * Set device hardware.
			 */
			if (tp->t_param && (error = (*tp->t_param)(tp, t))) {
				/* wanted here: mutex_spin_exit(&tty_lock); */
				splx(s);
				return (error);
			} else {
				tp->t_cflag = t->c_cflag;
				tp->t_ispeed = t->c_ispeed;
				tp->t_ospeed = t->c_ospeed;
				if (t->c_ospeed == 0)
					ttysig(tp, TTYSIG_LEADER, SIGHUP);
			}
			ttsetwater(tp);
		}

		/* delayed lock acquiring */
		mutex_spin_enter(&tty_lock);
		if (cmd != TIOCSETAF) {
			if (ISSET(t->c_lflag, ICANON) !=
			    ISSET(tp->t_lflag, ICANON)) {
				if (ISSET(t->c_lflag, ICANON)) {
					SET(tp->t_lflag, PENDIN);
					ttwakeup(tp);
				} else {
					struct clist tq;

					catq(&tp->t_rawq, &tp->t_canq);
					tq = tp->t_rawq;
					tp->t_rawq = tp->t_canq;
					tp->t_canq = tq;
					CLR(tp->t_lflag, PENDIN);
				}
			}
		}
		tp->t_iflag = t->c_iflag;
		tp->t_oflag = t->c_oflag;
		/*
		 * Make the EXTPROC bit read only.
		 */
		if (ISSET(tp->t_lflag, EXTPROC))
			SET(t->c_lflag, EXTPROC);
		else
			CLR(t->c_lflag, EXTPROC);
		tp->t_lflag = t->c_lflag | ISSET(tp->t_lflag, PENDIN);
		memcpy(tp->t_cc, t->c_cc, sizeof(t->c_cc));
		mutex_spin_exit(&tty_lock);
		splx(s);
		break;
	}
	case TIOCSETD:			/* set line discipline (old) */
		lp = ttyldisc_lookup_bynum(*(int *)data);
		goto setldisc;

	case TIOCSLINED: {		/* set line discipline (new) */
		char *name = (char *)data;
		dev_t device;

		/* Null terminate to prevent buffer overflow */
		name[TTLINEDNAMELEN - 1] = '\0';
		lp = ttyldisc_lookup(name);
 setldisc:
		if (lp == NULL)
			return (ENXIO);

		if (lp != tp->t_linesw) {
			device = tp->t_dev;
			s = spltty();
			(*tp->t_linesw->l_close)(tp, flag);
			error = (*lp->l_open)(device, tp);
			if (error) {
				(void)(*tp->t_linesw->l_open)(device, tp);
				splx(s);
				ttyldisc_release(lp);
				return (error);
			}
			ttyldisc_release(tp->t_linesw);
			tp->t_linesw = lp;
			splx(s);
		} else {
			/* Drop extra reference. */
			ttyldisc_release(lp);
		}
		break;
	}
	case TIOCSTART:			/* start output, like ^Q */
		mutex_spin_enter(&tty_lock);
		if (ISSET(tp->t_state, TS_TTSTOP) ||
		    ISSET(tp->t_lflag, FLUSHO)) {
			CLR(tp->t_lflag, FLUSHO);
			CLR(tp->t_state, TS_TTSTOP);
			ttstart(tp);
		}
		mutex_spin_exit(&tty_lock);
		break;
	case TIOCSTI:			/* simulate terminal input */
		if (kauth_authorize_generic(l->l_cred, KAUTH_GENERIC_ISSUSER,
		    NULL) != 0) {
			if (!ISSET(flag, FREAD))
				return (EPERM);
			if (!isctty(p, tp))
				return (EACCES);
		}
		(*tp->t_linesw->l_rint)(*(u_char *)data, tp);
		break;
	case TIOCSTOP:			/* stop output, like ^S */
	{
		mutex_spin_enter(&tty_lock);
		if (!ISSET(tp->t_state, TS_TTSTOP)) {
			SET(tp->t_state, TS_TTSTOP);
			cdev_stop(tp, 0);
		}
		mutex_spin_exit(&tty_lock);
		break;
	}
	case TIOCSCTTY:			/* become controlling tty */
		mutex_enter(&proclist_lock);
		mutex_spin_enter(&tty_lock);

		/* Session ctty vnode pointer set in vnode layer. */
		if (!SESS_LEADER(p) ||
		    ((p->p_session->s_ttyvp || tp->t_session) &&
		    (tp->t_session != p->p_session))) {
			mutex_spin_exit(&tty_lock);
			mutex_exit(&proclist_lock);
			return (EPERM);
		}

		/*
		 * `p_session' acquires a reference.
		 * But note that if `t_session' is set at this point,
		 * it must equal `p_session', in which case the session
		 * already has the correct reference count.
		 */
		if (tp->t_session == NULL)
			SESSHOLD(p->p_session);

		tp->t_session = p->p_session;
		tp->t_pgrp = p->p_pgrp;
		p->p_session->s_ttyp = tp;
		p->p_lflag |= PL_CONTROLT;
		mutex_spin_exit(&tty_lock);
		mutex_exit(&proclist_lock);
		break;
	case FIOSETOWN: {		/* set pgrp of tty */
		pid_t pgid = *(int *)data;
		struct pgrp *pgrp;

		mutex_enter(&proclist_lock); 
		if (tp->t_session != NULL && !isctty(p, tp)) {
			mutex_exit(&proclist_lock);
			return (ENOTTY);
		}

		if (pgid < 0) {
			pgrp = pg_find(-pgid, PFIND_LOCKED | PFIND_UNLOCK_FAIL);
			if (pgrp == NULL)
				return (EINVAL);
		} else {
			struct proc *p1;
			p1 = p_find(pgid, PFIND_LOCKED | PFIND_UNLOCK_FAIL);
			if (!p1)
				return (ESRCH);
			pgrp = p1->p_pgrp;
		}

		if (pgrp->pg_session != p->p_session) {
			mutex_exit(&proclist_lock);
			return (EPERM);
		}
		mutex_spin_enter(&tty_lock);
		tp->t_pgrp = pgrp;
		mutex_spin_exit(&tty_lock);
		mutex_exit(&proclist_lock);
		break;
	}
	case TIOCSPGRP: {		/* set pgrp of tty */
		struct pgrp *pgrp;

		mutex_enter(&proclist_lock); 
		if (!isctty(p, tp)) {
			mutex_exit(&proclist_lock);
			return (ENOTTY);
		}
		pgrp = pg_find(*(int *)data, PFIND_LOCKED | PFIND_UNLOCK_FAIL);
		if (pgrp == NULL)
			return (EINVAL);
		if (pgrp->pg_session != p->p_session) {
			mutex_exit(&proclist_lock);
			return (EPERM);
		}
		mutex_spin_enter(&tty_lock);
		tp->t_pgrp = pgrp;
		mutex_spin_exit(&tty_lock);
		mutex_exit(&proclist_lock);
		break;
	}
	case TIOCSTAT:			/* get load avg stats */
		mutex_spin_enter(&tty_lock);
		ttyinfo(tp, 0);
		mutex_spin_exit(&tty_lock);
		break;
	case TIOCSWINSZ:		/* set window size */
		mutex_spin_enter(&tty_lock);
		if (memcmp((void *)&tp->t_winsize, data,
		    sizeof(struct winsize))) {
			tp->t_winsize = *(struct winsize *)data;
			ttysig(tp, TTYSIG_PG1, SIGWINCH);
		}
		mutex_spin_exit(&tty_lock);
		break;
	default:
#ifdef COMPAT_OLDTTY
		return (ttcompat(tp, cmd, data, flag, l));
#else
		return (EPASSTHROUGH);
#endif
	}
	return (0);
}

int
ttpoll(struct tty *tp, int events, struct lwp *l)
{
	int	revents;

	revents = 0;
	mutex_spin_enter(&tty_lock);
	if (events & (POLLIN | POLLRDNORM))
		if (ttnread(tp) > 0)
			revents |= events & (POLLIN | POLLRDNORM);

	if (events & (POLLOUT | POLLWRNORM))
		if (tp->t_outq.c_cc <= tp->t_lowat)
			revents |= events & (POLLOUT | POLLWRNORM);

	if (events & POLLHUP)
		if (!CONNECTED(tp))
			revents |= POLLHUP;

	if (revents == 0) {
		if (events & (POLLIN | POLLHUP | POLLRDNORM))
			selrecord(l, &tp->t_rsel);

		if (events & (POLLOUT | POLLWRNORM))
			selrecord(l, &tp->t_wsel);
	}

	mutex_spin_exit(&tty_lock);

	return (revents);
}

static void
filt_ttyrdetach(struct knote *kn)
{
	struct tty	*tp;

	tp = kn->kn_hook;
	mutex_spin_enter(&tty_lock);
	SLIST_REMOVE(&tp->t_rsel.sel_klist, kn, knote, kn_selnext);
	mutex_spin_exit(&tty_lock);
}

static int
filt_ttyread(struct knote *kn, long hint)
{
	struct tty	*tp;

	tp = kn->kn_hook;
	if ((hint & NOTE_SUBMIT) == 0)
		mutex_spin_enter(&tty_lock);
	kn->kn_data = ttnread(tp);
	if ((hint & NOTE_SUBMIT) == 0)
		mutex_spin_exit(&tty_lock);
	return (kn->kn_data > 0);
}

static void
filt_ttywdetach(struct knote *kn)
{
	struct tty	*tp;

	tp = kn->kn_hook;
	mutex_spin_enter(&tty_lock);
	SLIST_REMOVE(&tp->t_wsel.sel_klist, kn, knote, kn_selnext);
	mutex_spin_exit(&tty_lock);
}

static int
filt_ttywrite(struct knote *kn, long hint)
{
	struct tty	*tp;
	int		canwrite;

	tp = kn->kn_hook;
	if ((hint & NOTE_SUBMIT) == 0)
		mutex_spin_enter(&tty_lock);
	kn->kn_data = tp->t_outq.c_cn - tp->t_outq.c_cc;
	canwrite = (tp->t_outq.c_cc <= tp->t_lowat) && CONNECTED(tp);
	if ((hint & NOTE_SUBMIT) == 0)
		mutex_spin_exit(&tty_lock);
	return (canwrite);
}

static const struct filterops ttyread_filtops =
	{ 1, NULL, filt_ttyrdetach, filt_ttyread };
static const struct filterops ttywrite_filtops =
	{ 1, NULL, filt_ttywdetach, filt_ttywrite };

int
ttykqfilter(dev_t dev, struct knote *kn)
{
	struct tty	*tp;
	struct klist	*klist;

	if ((tp = cdev_tty(dev)) == NULL)
		return (ENXIO);

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &tp->t_rsel.sel_klist;
		kn->kn_fop = &ttyread_filtops;
		break;
	case EVFILT_WRITE:
		klist = &tp->t_wsel.sel_klist;
		kn->kn_fop = &ttywrite_filtops;
		break;
	default:
		return EINVAL;
	}

	kn->kn_hook = tp;

	mutex_spin_enter(&tty_lock);
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	mutex_spin_exit(&tty_lock);

	return (0);
}

/*
 * Find the number of chars ready to be read from this tty.
 * Call with the tty lock held.
 */
static int
ttnread(struct tty *tp)
{
	int	nread;

	KASSERT(mutex_owned(&tty_lock));

	if (ISSET(tp->t_lflag, PENDIN))
		ttypend(tp);
	nread = tp->t_canq.c_cc;
	if (!ISSET(tp->t_lflag, ICANON)) {
		nread += tp->t_rawq.c_cc;
		if (nread < tp->t_cc[VMIN] && !tp->t_cc[VTIME])
			nread = 0;
	}
	return (nread);
}

/*
 * Wait for output to drain.
 */
int
ttywait(struct tty *tp)
{
	int	error;

	error = 0;

	mutex_spin_enter(&tty_lock);
	while ((tp->t_outq.c_cc || ISSET(tp->t_state, TS_BUSY)) &&
	    CONNECTED(tp) && tp->t_oproc) {
		(*tp->t_oproc)(tp);
		error = ttysleep(tp, &tp->t_outq.c_cv, true, 0);
		if (error)
			break;
	}
	mutex_spin_exit(&tty_lock);

	return (error);
}

/*
 * Flush if successfully wait.
 */
int
ttywflush(struct tty *tp)
{
	int	error;

	if ((error = ttywait(tp)) == 0) {
		mutex_spin_enter(&tty_lock);
		ttyflush(tp, FREAD);
		mutex_spin_exit(&tty_lock);
	}
	return (error);
}

/*
 * Flush tty read and/or write queues, notifying anyone waiting.
 * Call with the tty lock held.
 */
void
ttyflush(struct tty *tp, int rw)
{

	KASSERT(mutex_owned(&tty_lock));

	if (rw & FREAD) {
		FLUSHQ(&tp->t_canq);
		FLUSHQ(&tp->t_rawq);
		tp->t_rocount = 0;
		tp->t_rocol = 0;
		CLR(tp->t_state, TS_LOCAL);
		ttwakeup(tp);
	}
	if (rw & FWRITE) {
		CLR(tp->t_state, TS_TTSTOP);
		cdev_stop(tp, rw);
		FLUSHQ(&tp->t_outq);
		clwakeup(&tp->t_outq);
		selnotify(&tp->t_wsel, NOTE_SUBMIT);
	}
}

/*
 * Copy in the default termios characters.
 */
void
ttychars(struct tty *tp)
{

	memcpy(tp->t_cc, ttydefchars, sizeof(ttydefchars));
}

/*
 * Send stop character on input overflow.
 * Call with the tty lock held.
 */
static void
ttyblock(struct tty *tp)
{
	int	total;

	KASSERT(mutex_owned(&tty_lock));

	total = tp->t_rawq.c_cc + tp->t_canq.c_cc;
	if (tp->t_rawq.c_cc > TTYHOG) {
		ttyflush(tp, FREAD | FWRITE);
		CLR(tp->t_state, TS_TBLOCK);
	}
	/*
	 * Block further input iff: current input > threshold
	 * AND input is available to user program.
	 */
	if (total >= TTYHOG / 2 &&
	    !ISSET(tp->t_state, TS_TBLOCK) &&
	    (!ISSET(tp->t_lflag, ICANON) || tp->t_canq.c_cc > 0)) {
		if (ISSET(tp->t_iflag, IXOFF) &&
		    tp->t_cc[VSTOP] != _POSIX_VDISABLE &&
		    putc(tp->t_cc[VSTOP], &tp->t_outq) == 0) {
			SET(tp->t_state, TS_TBLOCK);
			ttstart(tp);
		}
		/* Try to block remote output via hardware flow control. */
		if (ISSET(tp->t_cflag, CHWFLOW) && tp->t_hwiflow &&
		    (*tp->t_hwiflow)(tp, 1) != 0)
			SET(tp->t_state, TS_TBLOCK);
	}
}

/*
 * Delayed line discipline output
 */
void
ttrstrt(void *tp_arg)
{
	struct tty	*tp;

#ifdef DIAGNOSTIC
	if (tp_arg == NULL)
		panic("ttrstrt");
#endif
	tp = tp_arg;
	mutex_spin_enter(&tty_lock);

	CLR(tp->t_state, TS_TIMEOUT);
	ttstart(tp); /* XXX - Shouldn't this be tp->l_start(tp)? */

	mutex_spin_exit(&tty_lock);
}

/*
 * start a line discipline
 * Always call with tty lock held?
 */
int
ttstart(struct tty *tp)
{

	if (tp->t_oproc != NULL)	/* XXX: Kludge for pty. */
		(*tp->t_oproc)(tp);
	return (0);
}

/*
 * "close" a line discipline
 */
int
ttylclose(struct tty *tp, int flag)
{

	if (flag & FNONBLOCK) {
		mutex_spin_enter(&tty_lock);
		ttyflush(tp, FREAD | FWRITE);
		mutex_spin_exit(&tty_lock);
	} else
		ttywflush(tp);
	return (0);
}

/*
 * Handle modem control transition on a tty.
 * Flag indicates new state of carrier.
 * Returns 0 if the line should be turned off, otherwise 1.
 */
int
ttymodem(struct tty *tp, int flag)
{

	mutex_spin_enter(&tty_lock);
	if (flag == 0) {
		if (ISSET(tp->t_state, TS_CARR_ON)) {
			/*
			 * Lost carrier.
			 */
			CLR(tp->t_state, TS_CARR_ON);
			if (ISSET(tp->t_state, TS_ISOPEN) && !CONNECTED(tp)) {
				ttysig(tp, TTYSIG_LEADER, SIGHUP);
				ttyflush(tp, FREAD | FWRITE);
				mutex_spin_exit(&tty_lock);
				return (0);
			}
		}
	} else {
		if (!ISSET(tp->t_state, TS_CARR_ON)) {
			/*
			 * Carrier now on.
			 */
			SET(tp->t_state, TS_CARR_ON);
			ttwakeup(tp);
		}
	}
	mutex_spin_exit(&tty_lock);

	return (1);
}

/*
 * Default modem control routine (for other line disciplines).
 * Return argument flag, to turn off device on carrier drop.
 */
int
nullmodem(struct tty *tp, int flag)
{

	mutex_spin_enter(&tty_lock);
	if (flag)
		SET(tp->t_state, TS_CARR_ON);
	else {
		CLR(tp->t_state, TS_CARR_ON);
		if (!CONNECTED(tp)) {
			ttysig(tp, TTYSIG_LEADER, SIGHUP);
			mutex_spin_exit(&tty_lock);
			return (0);
		}
	}
	mutex_spin_exit(&tty_lock);

	return (1);
}

/*
 * Reinput pending characters after state switch.
 */
void
ttypend(struct tty *tp)
{
	struct clist	tq;
	int		c;

	KASSERT(mutex_owned(&tty_lock));

	CLR(tp->t_lflag, PENDIN);
	SET(tp->t_state, TS_TYPEN);
	tq = tp->t_rawq;
	tp->t_rawq.c_cc = 0;
	tp->t_rawq.c_cf = tp->t_rawq.c_cl = 0;
	while ((c = getc(&tq)) >= 0)
		ttyinput_wlock(c, tp);
	CLR(tp->t_state, TS_TYPEN);
}

/*
 * Process a read call on a tty device.
 */
int
ttread(struct tty *tp, struct uio *uio, int flag)
{
	struct clist	*qp;
	u_char		*cc;
	struct proc	*p;
	int		c, first, error, has_stime, last_cc;
	long		lflag, slp;
	struct timeval	now, stime;

	stime.tv_usec = 0;	/* XXX gcc */
	stime.tv_sec = 0;	/* XXX gcc */

	cc = tp->t_cc;
	p = curproc;
	error = 0;
	has_stime = 0;
	last_cc = 0;
	slp = 0;

 loop:
	mutex_spin_enter(&tty_lock);
	lflag = tp->t_lflag;
	/*
	 * take pending input first
	 */
	if (ISSET(lflag, PENDIN))
		ttypend(tp);

	/*
	 * Hang process if it's in the background.
	 */
	if (isbackground(p, tp)) {
		if (sigismember(&p->p_sigctx.ps_sigignore, SIGTTIN) ||
		    sigismember(&curlwp->l_sigmask, SIGTTIN) ||
		    p->p_sflag & PS_PPWAIT || p->p_pgrp->pg_jobc == 0) {
			mutex_spin_exit(&tty_lock);
			return (EIO);
		}
		mutex_spin_exit(&tty_lock);

		mutex_enter(&proclist_mutex);
		pgsignal(p->p_pgrp, SIGTTIN, 1);
		mutex_exit(&proclist_mutex);

		mutex_spin_enter(&tty_lock);
		error = ttysleep(tp, &lbolt, true, 0);
		mutex_spin_exit(&tty_lock);
		if (error)
			return (error);
		goto loop;
	}

	if (!ISSET(lflag, ICANON)) {
		int m = cc[VMIN];
		long t = cc[VTIME];

		qp = &tp->t_rawq;
		/*
		 * Check each of the four combinations.
		 * (m > 0 && t == 0) is the normal read case.
		 * It should be fairly efficient, so we check that and its
		 * companion case (m == 0 && t == 0) first.
		 * For the other two cases, we compute the target sleep time
		 * into slp.
		 */
		if (t == 0) {
			if (qp->c_cc < m)
				goto sleep;
			goto read;
		}
		t *= hz;		/* time in deca-ticks */
/*
 * Time difference in deca-ticks, split division to avoid numeric overflow.
 * Ok for hz < ~200kHz
 */
#define	diff(t1, t2) (((t1).tv_sec - (t2).tv_sec) * 10 * hz + \
			 ((t1).tv_usec - (t2).tv_usec) / 100 * hz / 1000)
		if (m > 0) {
			if (qp->c_cc <= 0)
				goto sleep;
			if (qp->c_cc >= m)
				goto read;
			if (!has_stime) {
				/* first character, start timer */
				has_stime = 1;
				getmicrotime(&stime);
				slp = t;
			} else if (qp->c_cc > last_cc) {
				/* got a character, restart timer */
				getmicrotime(&stime);
				slp = t;
			} else {
				/* nothing, check expiration */
				getmicrotime(&now);
				slp = t - diff(now, stime);
			}
		} else {	/* m == 0 */
			if (qp->c_cc > 0)
				goto read;
			if (!has_stime) {
				has_stime = 1;
				getmicrotime(&stime);
				slp = t;
			} else {
				getmicrotime(&now);
				slp = t - diff(now, stime);
			}
		}
		last_cc = qp->c_cc;
#undef diff
		if (slp > 0) {
			/*
			 * Convert deca-ticks back to ticks.
			 * Rounding down may make us wake up just short
			 * of the target, so we round up.
			 * Maybe we should do 'slp/10 + 1' because the
			 * first tick maybe almost immediate.
			 * However it is more useful for a program that sets
			 * VTIME=10 to wakeup every second not every 1.01
			 * seconds (if hz=100).
			 */
			slp = (slp + 9)/ 10;
			goto sleep;
		}
	} else if ((qp = &tp->t_canq)->c_cc <= 0) {
		int	carrier;

 sleep:
		/*
		 * If there is no input, sleep on rawq
		 * awaiting hardware receipt and notification.
		 * If we have data, we don't need to check for carrier.
		 */
		carrier = CONNECTED(tp);
		if (!carrier && ISSET(tp->t_state, TS_ISOPEN)) {
			mutex_spin_exit(&tty_lock);
			return (0);	/* EOF */
		}
		if (flag & IO_NDELAY) {
			mutex_spin_exit(&tty_lock);
			return (EWOULDBLOCK);
		}
		error = ttysleep(tp, &tp->t_rawq.c_cv, true, slp);
		mutex_spin_exit(&tty_lock);
		/* VMIN == 0: any quantity read satisfies */
		if (cc[VMIN] == 0 && error == EWOULDBLOCK)
			return (0);
		if (error && error != EWOULDBLOCK)
			return (error);
		goto loop;
	}
 read:
	mutex_spin_exit(&tty_lock);

	/*
	 * Input present, check for input mapping and processing.
	 */
	first = 1;
	while ((c = getc(qp)) >= 0) {
		/*
		 * delayed suspend (^Y)
		 */
		if (CCEQ(cc[VDSUSP], c) &&
		    ISSET(lflag, IEXTEN|ISIG) == (IEXTEN|ISIG)) {
			mutex_spin_enter(&tty_lock);
			ttysig(tp, TTYSIG_PG1, SIGTSTP);
			if (first) {
				error = ttysleep(tp, &lbolt, true, 0);
				mutex_spin_exit(&tty_lock);
				if (error)
					break;
				goto loop;
			} else
				mutex_spin_exit(&tty_lock);
			break;
		}
		/*
		 * Interpret EOF only in canonical mode.
		 */
		if (CCEQ(cc[VEOF], c) && ISSET(lflag, ICANON))
			break;
		/*
		 * Give user character.
		 */
 		error = ureadc(c, uio);
		if (error)
			break;
 		if (uio->uio_resid == 0)
			break;
		/*
		 * In canonical mode check for a "break character"
		 * marking the end of a "line of input".
		 */
		if (ISSET(lflag, ICANON) && TTBREAKC(c, lflag))
			break;
		first = 0;
	}
	/*
	 * Look to unblock output now that (presumably)
	 * the input queue has gone down.
	 */
	mutex_spin_enter(&tty_lock);
	if (ISSET(tp->t_state, TS_TBLOCK) && tp->t_rawq.c_cc < TTYHOG / 5) {
		if (ISSET(tp->t_iflag, IXOFF) &&
		    cc[VSTART] != _POSIX_VDISABLE &&
		    putc(cc[VSTART], &tp->t_outq) == 0) {
			CLR(tp->t_state, TS_TBLOCK);
			ttstart(tp);
		}
		/* Try to unblock remote output via hardware flow control. */
		if (ISSET(tp->t_cflag, CHWFLOW) && tp->t_hwiflow &&
		    (*tp->t_hwiflow)(tp, 0) != 0)
			CLR(tp->t_state, TS_TBLOCK);
	}
	mutex_spin_exit(&tty_lock);

	return (error);
}

/*
 * Check the output queue on tp for space for a kernel message (from uprintf
 * or tprintf).  Allow some space over the normal hiwater mark so we don't
 * lose messages due to normal flow control, but don't let the tty run amok.
 * Sleeps here are not interruptible, but we return prematurely if new signals
 * arrive.
 * Call with tty lock held.
 */
static int
ttycheckoutq_wlock(struct tty *tp, int wait)
{
	int	hiwat, error;

	KASSERT(mutex_owned(&tty_lock));

	hiwat = tp->t_hiwat;
	if (tp->t_outq.c_cc > hiwat + 200)
		while (tp->t_outq.c_cc > hiwat) {
			ttstart(tp);
			if (wait == 0)
				return (0);
			error = ttysleep(tp, &tp->t_outq.c_cv, true, hz);
			if (error == EINTR)
				wait = 0;
		}

	return (1);
}

int
ttycheckoutq(struct tty *tp, int wait)
{
	int	r;

	mutex_spin_enter(&tty_lock);
	r = ttycheckoutq_wlock(tp, wait);
	mutex_spin_exit(&tty_lock);

	return (r);
}

/*
 * Process a write call on a tty device.
 */
int
ttwrite(struct tty *tp, struct uio *uio, int flag)
{
	u_char		*cp;
	struct proc	*p;
	int		cc, ce, i, hiwat, error;
	size_t		cnt;
	u_char		obuf[OBUFSIZ];

	cp = NULL;
	hiwat = tp->t_hiwat;
	cnt = uio->uio_resid;
	error = 0;
	cc = 0;
 loop:
	mutex_spin_enter(&tty_lock);
	if (!CONNECTED(tp)) {
		if (ISSET(tp->t_state, TS_ISOPEN)) {
			mutex_spin_exit(&tty_lock);
			return (EIO);
		} else if (flag & IO_NDELAY) {
			mutex_spin_exit(&tty_lock);
			error = EWOULDBLOCK;
			goto out;
		} else {
			/* Sleep awaiting carrier. */
			error = ttysleep(tp, &tp->t_rawq.c_cv, true, 0);
			mutex_spin_exit(&tty_lock);
			if (error)
				goto out;
			goto loop;
		}
	}

	/*
	 * Hang the process if it's in the background.
	 */
	p = curproc;
	if (isbackground(p, tp) &&
	    ISSET(tp->t_lflag, TOSTOP) && (p->p_sflag & PS_PPWAIT) == 0 &&
	    !sigismember(&p->p_sigctx.ps_sigignore, SIGTTOU) &&
	    !sigismember(&curlwp->l_sigmask, SIGTTOU)) {
		if (p->p_pgrp->pg_jobc == 0) {
			error = EIO;
			mutex_spin_exit(&tty_lock);
			goto out;
		}
		mutex_spin_exit(&tty_lock);

		mutex_enter(&proclist_mutex);
		pgsignal(p->p_pgrp, SIGTTOU, 1);
		mutex_exit(&proclist_mutex);

		mutex_spin_enter(&tty_lock);
		error = ttysleep(tp, &lbolt, true, 0);
		mutex_spin_exit(&tty_lock);
		if (error)
			goto out;
		goto loop;
	}
	mutex_spin_exit(&tty_lock);

	/*
	 * Process the user's data in at most OBUFSIZ chunks.  Perform any
	 * output translation.  Keep track of high water mark, sleep on
	 * overflow awaiting device aid in acquiring new space.
	 */
	while (uio->uio_resid > 0 || cc > 0) {
		if (ISSET(tp->t_lflag, FLUSHO)) {
			uio->uio_resid = 0;
			return (0);
		}
		if (tp->t_outq.c_cc > hiwat)
			goto ovhiwat;
		/*
		 * Grab a hunk of data from the user, unless we have some
		 * leftover from last time.
		 */
		if (cc == 0) {
			cc = min(uio->uio_resid, OBUFSIZ);
			cp = obuf;
			error = uiomove(cp, cc, uio);
			if (error) {
				cc = 0;
				goto out;
			}
		}
		/*
		 * If nothing fancy need be done, grab those characters we
		 * can handle without any of ttyoutput's processing and
		 * just transfer them to the output q.  For those chars
		 * which require special processing (as indicated by the
		 * bits in char_type), call ttyoutput.  After processing
		 * a hunk of data, look for FLUSHO so ^O's will take effect
		 * immediately.
		 */
		mutex_spin_enter(&tty_lock);
		while (cc > 0) {
			if (!ISSET(tp->t_oflag, OPOST))
				ce = cc;
			else {
				ce = cc - scanc((u_int)cc, cp, char_type,
				    CCLASSMASK);
				/*
				 * If ce is zero, then we're processing
				 * a special character through ttyoutput.
				 */
				if (ce == 0) {
					tp->t_rocount = 0;
					if (ttyoutput(*cp, tp) >= 0) {
						/* out of space */
						mutex_spin_exit(&tty_lock);
						goto overfull;
					}
					cp++;
					cc--;
					if (ISSET(tp->t_lflag, FLUSHO) ||
					    tp->t_outq.c_cc > hiwat) {
						mutex_spin_exit(&tty_lock);
						goto ovhiwat;
					}
					continue;
				}
			}
			/*
			 * A bunch of normal characters have been found.
			 * Transfer them en masse to the output queue and
			 * continue processing at the top of the loop.
			 * If there are any further characters in this
			 * <= OBUFSIZ chunk, the first should be a character
			 * requiring special handling by ttyoutput.
			 */
			tp->t_rocount = 0;
			i = b_to_q(cp, ce, &tp->t_outq);
			ce -= i;
			tp->t_column += ce;
			cp += ce, cc -= ce, tk_nout += ce;
			tp->t_outcc += ce;
			if (i > 0) {
				/* out of space */
				mutex_spin_exit(&tty_lock);
				goto overfull;
			}
			if (ISSET(tp->t_lflag, FLUSHO) ||
			    tp->t_outq.c_cc > hiwat)
				break;
		}
		ttstart(tp);
		mutex_spin_exit(&tty_lock);
	}

 out:
	/*
	 * If cc is nonzero, we leave the uio structure inconsistent, as the
	 * offset and iov pointers have moved forward, but it doesn't matter
	 * (the call will either return short or restart with a new uio).
	 */
	uio->uio_resid += cc;
	return (error);

 overfull:
	/*
	 * Since we are using ring buffers, if we can't insert any more into
	 * the output queue, we can assume the ring is full and that someone
	 * forgot to set the high water mark correctly.  We set it and then
	 * proceed as normal.
	 */
	hiwat = tp->t_outq.c_cc - 1;

 ovhiwat:
	mutex_spin_enter(&tty_lock);
	ttstart(tp);
	/*
	 * This can only occur if FLUSHO is set in t_lflag,
	 * or if ttstart/oproc is synchronous (or very fast).
	 */
	if (tp->t_outq.c_cc <= hiwat) {
		mutex_spin_exit(&tty_lock);
		goto loop;
	}
	if (flag & IO_NDELAY) {
		mutex_spin_exit(&tty_lock);
		error = EWOULDBLOCK;
		goto out;
	}
	error = ttysleep(tp, &tp->t_outq.c_cv, true, 0);
	mutex_spin_exit(&tty_lock);
	if (error)
		goto out;
	goto loop;
}

/*
 * Try to pull more output from the producer.  Return non-zero if
 * there is output ready to be sent.
 */
bool
ttypull(struct tty *tp)
{

	/* XXXSMP not yet KASSERT(mutex_owned(&tty_lock)); */

	if (tp->t_outq.c_cc <= tp->t_lowat) {
		clwakeup(&tp->t_outq);
		selnotify(&tp->t_wsel, NOTE_SUBMIT);
	}
	return tp->t_outq.c_cc != 0;
}

/*
 * Rubout one character from the rawq of tp
 * as cleanly as possible.
 * Called with tty lock held.
 */
void
ttyrub(int c, struct tty *tp)
{
	u_char	*cp;
	int	savecol, tabc;

	KASSERT(mutex_owned(&tty_lock));

	if (!ISSET(tp->t_lflag, ECHO) || ISSET(tp->t_lflag, EXTPROC))
		return;
	CLR(tp->t_lflag, FLUSHO);
	if (ISSET(tp->t_lflag, ECHOE)) {
		if (tp->t_rocount == 0) {
			/*
			 * Screwed by ttwrite; retype
			 */
			ttyretype(tp);
			return;
		}
		if (c == ('\t' | TTY_QUOTE) || c == ('\n' | TTY_QUOTE))
			ttyrubo(tp, 2);
		else {
			CLR(c, ~TTY_CHARMASK);
			switch (CCLASS(c)) {
			case ORDINARY:
				ttyrubo(tp, 1);
				break;
			case BACKSPACE:
			case CONTROL:
			case NEWLINE:
			case RETURN:
			case VTAB:
				if (ISSET(tp->t_lflag, ECHOCTL))
					ttyrubo(tp, 2);
				break;
			case TAB:
				if (tp->t_rocount < tp->t_rawq.c_cc) {
					ttyretype(tp);
					return;
				}
				savecol = tp->t_column;
				SET(tp->t_state, TS_CNTTB);
				SET(tp->t_lflag, FLUSHO);
				tp->t_column = tp->t_rocol;
				for (cp = firstc(&tp->t_rawq, &tabc); cp;
				    cp = nextc(&tp->t_rawq, cp, &tabc))
					ttyecho(tabc, tp);
				CLR(tp->t_lflag, FLUSHO);
				CLR(tp->t_state, TS_CNTTB);

				/* savecol will now be length of the tab. */
				savecol -= tp->t_column;
				tp->t_column += savecol;
				if (savecol > 8)
					savecol = 8;	/* overflow screw */
				while (--savecol >= 0)
					(void)ttyoutput('\b', tp);
				break;
			default:			/* XXX */
				(void)printf("ttyrub: would panic c = %d, "
				    "val = %d\n", c, CCLASS(c));
			}
		}
	} else if (ISSET(tp->t_lflag, ECHOPRT)) {
		if (!ISSET(tp->t_state, TS_ERASE)) {
			SET(tp->t_state, TS_ERASE);
			(void)ttyoutput('\\', tp);
		}
		ttyecho(c, tp);
	} else
		ttyecho(tp->t_cc[VERASE], tp);
	--tp->t_rocount;
}

/*
 * Back over cnt characters, erasing them.
 * Called with tty lock held.
 */
static void
ttyrubo(struct tty *tp, int cnt)
{

	KASSERT(mutex_owned(&tty_lock));

	while (cnt-- > 0) {
		(void)ttyoutput('\b', tp);
		(void)ttyoutput(' ', tp);
		(void)ttyoutput('\b', tp);
	}
}

/*
 * ttyretype --
 *	Reprint the rawq line.  Note, it is assumed that c_cc has already
 *	been checked.
 *
 * Called with tty lock held.
 */
void
ttyretype(struct tty *tp)
{
	u_char	*cp;
	int	c;

	KASSERT(mutex_owned(&tty_lock));

	/* Echo the reprint character. */
	if (tp->t_cc[VREPRINT] != _POSIX_VDISABLE)
		ttyecho(tp->t_cc[VREPRINT], tp);

	(void)ttyoutput('\n', tp);

	for (cp = firstc(&tp->t_canq, &c); cp; cp = nextc(&tp->t_canq, cp, &c))
		ttyecho(c, tp);
	for (cp = firstc(&tp->t_rawq, &c); cp; cp = nextc(&tp->t_rawq, cp, &c))
		ttyecho(c, tp);
	CLR(tp->t_state, TS_ERASE);

	tp->t_rocount = tp->t_rawq.c_cc;
	tp->t_rocol = 0;
}

/*
 * Echo a typed character to the terminal.
 * Called with tty lock held.
 */
static void
ttyecho(int c, struct tty *tp)
{

	KASSERT(mutex_owned(&tty_lock));

	if (!ISSET(tp->t_state, TS_CNTTB))
		CLR(tp->t_lflag, FLUSHO);
	if ((!ISSET(tp->t_lflag, ECHO) &&
	    (!ISSET(tp->t_lflag, ECHONL) || c != '\n')) ||
	    ISSET(tp->t_lflag, EXTPROC))
		return;
	if (((ISSET(tp->t_lflag, ECHOCTL) &&
	    (ISSET(c, TTY_CHARMASK) <= 037 && c != '\t' && c != '\n')) ||
	    ISSET(c, TTY_CHARMASK) == 0177)) {
		(void)ttyoutput('^', tp);
		CLR(c, ~TTY_CHARMASK);
		if (c == 0177)
			c = '?';
		else
			c += 'A' - 1;
	}
	(void)ttyoutput(c, tp);
}

/*
 * Wake up any readers on a tty.
 * Called with tty lock held.
 */
void
ttwakeup(struct tty *tp)
{

	KASSERT(mutex_owned(&tty_lock));

	selnotify(&tp->t_rsel, NOTE_SUBMIT);
	if (ISSET(tp->t_state, TS_ASYNC))
		ttysig(tp, TTYSIG_PG2, SIGIO);
#if 0
	/* XXX tp->t_rawq.c_cv.cv_waiters dropping to zero early!? */
	clwakeup(&tp->t_rawq);
#else
	cv_wakeup(&tp->t_rawq.c_cv);
#endif
}

/*
 * Look up a code for a specified speed in a conversion table;
 * used by drivers to map software speed values to hardware parameters.
 */
int
ttspeedtab(int speed, const struct speedtab *table)
{

	for (; table->sp_speed != -1; table++)
		if (table->sp_speed == speed)
			return (table->sp_code);
	return (-1);
}

/*
 * Set tty hi and low water marks.
 *
 * Try to arrange the dynamics so there's about one second
 * from hi to low water.
 */
void
ttsetwater(struct tty *tp)
{
	int	cps, x;

	/* XXX not yet KASSERT(mutex_owned(&tty_lock)); */

#define	CLAMP(x, h, l)	((x) > h ? h : ((x) < l) ? l : (x))

	cps = tp->t_ospeed / 10;
	tp->t_lowat = x = CLAMP(cps / 2, TTMAXLOWAT, TTMINLOWAT);
	x += cps;
	x = CLAMP(x, TTMAXHIWAT, TTMINHIWAT);
	tp->t_hiwat = roundup(x, CBSIZE);
#undef	CLAMP
}

/*
 * Report on state of foreground process group.
 * Call with tty lock held.
 */
void
ttyinfo(struct tty *tp, int fromsig)
{
	struct lwp	*l;
	struct proc	*p, *pick = NULL;
	struct timeval	utime, stime;
	int		tmp;
	fixpt_t		pctcpu = 0;
	const char	*msg;

	if (ttycheckoutq_wlock(tp, 0) == 0)
		return;

	if (tp->t_session == NULL)
		msg = "not a controlling terminal\n";
	else if (tp->t_pgrp == NULL)
		msg = "no foreground process group\n";
	else if ((p = LIST_FIRST(&tp->t_pgrp->pg_members)) == NULL)
		msg = "empty foreground process group\n";
	else {
		/* Pick interesting process. */
		for (; p != NULL; p = LIST_NEXT(p, p_pglist))
			if (proc_compare(pick, p))
				pick = p;
		if (fromsig &&
		    (SIGACTION_PS(pick->p_sigacts, SIGINFO).sa_flags &
		    SA_NOKERNINFO))
			return;
		msg = NULL;
	}

	/* Print load average. */
	tmp = (averunnable.ldavg[0] * 100 + FSCALE / 2) >> FSHIFT;
	ttyprintf_nolock(tp, "load: %d.%02d ", tmp / 100, tmp % 100);

	if (pick == NULL) {
		ttyprintf_nolock(tp, msg);
		tp->t_rocount = 0; /* so pending input will be retyped if BS */
		return;
	}

	ttyprintf_nolock(tp, " cmd: %s %d [", pick->p_comm, pick->p_pid);
	LIST_FOREACH(l, &pick->p_lwps, l_sibling) {
	    ttyprintf_nolock(tp, "%s%s",
	    l->l_stat == LSONPROC ? "running" :
	    l->l_stat == LSRUN ? "runnable" :
	    l->l_wmesg ? l->l_wmesg : "iowait",
		(LIST_NEXT(l, l_sibling) != NULL) ? " " : "] ");
	    pctcpu += l->l_pctcpu;
	}
	pctcpu += pick->p_pctcpu;

	/* XXXXSMP order tty_lock -> p_smutex is bad */
	mutex_enter(&pick->p_smutex);
	calcru(pick, &utime, &stime, NULL, NULL);
	mutex_exit(&pick->p_smutex);

	/* Round up and print user time. */
	utime.tv_usec += 5000;
	if (utime.tv_usec >= 1000000) {
		utime.tv_sec += 1;
		utime.tv_usec -= 1000000;
	}
	ttyprintf_nolock(tp, "%ld.%02ldu ", (long int)utime.tv_sec,
	    (long int)utime.tv_usec / 10000);

	/* Round up and print system time. */
	stime.tv_usec += 5000;
	if (stime.tv_usec >= 1000000) {
		stime.tv_sec += 1;
		stime.tv_usec -= 1000000;
	}
	ttyprintf_nolock(tp, "%ld.%02lds ", (long int)stime.tv_sec,
	    (long int)stime.tv_usec / 10000);

#define	pgtok(a)	(((u_long) ((a) * PAGE_SIZE) / 1024))
	/* Print percentage CPU. */
	tmp = (pctcpu * 10000 + FSCALE / 2) >> FSHIFT;
	ttyprintf_nolock(tp, "%d%% ", tmp / 100);

	/* Print resident set size. */
	if (pick->p_stat == SIDL || P_ZOMBIE(pick))
		tmp = 0;
	else {
		struct vmspace *vm = pick->p_vmspace;
		tmp = pgtok(vm_resident_count(vm));
	}
	ttyprintf_nolock(tp, "%dk\n", tmp);
	tp->t_rocount = 0;	/* so pending input will be retyped if BS */
}

/*
 * Returns 1 if p2 is "better" than p1
 *
 * The algorithm for picking the "interesting" process is thus:
 *
 *	1) Only foreground processes are eligible - implied.
 *	2) Runnable processes are favored over anything else.  The runner
 *	   with the highest CPU utilization is picked (l_pctcpu).  Ties are
 *	   broken by picking the highest pid.
 *	3) The sleeper with the shortest sleep time is next.  With ties,
 *	   we pick out just "short-term" sleepers (P_SINTR == 0).
 *	4) Further ties are broken by picking the highest pid.
 *
 * XXXSMP
 */
#define	ISRUN(p)	((p)->p_nrlwps > 0)
#define	TESTAB(a, b)	((a)<<1 | (b))
#define	ONLYA	2
#define	ONLYB	1
#define	BOTH	3

static int
proc_compare(struct proc *p1, struct proc *p2)
{
	lwp_t *l1, *l2;

	if (p1 == NULL)
		return (1);
	/*
	 * see if at least one of them is runnable
	 */
	switch (TESTAB(ISRUN(p1), ISRUN(p2))) {
	case ONLYA:
		return (0);
	case ONLYB:
		return (1);
	case BOTH:
		/*
		 * tie - favor one with highest recent CPU utilization
		 */
		l1 = LIST_FIRST(&p1->p_lwps);
		l2 = LIST_FIRST(&p2->p_lwps);
		if (l2->l_pctcpu > l1->l_pctcpu)
			return (1);
		return (p2->p_pid > p1->p_pid);	/* tie - return highest pid */
	}
	/*
 	 * weed out zombies
	 */
	switch (TESTAB(P_ZOMBIE(p1), P_ZOMBIE(p2))) {
	case ONLYA:
		return (1);
	case ONLYB:
		return (0);
	case BOTH:
		return (p2->p_pid > p1->p_pid);	/* tie - return highest pid */
	}
#if 0 /* XXX NJWLWP */
	/*
	 * pick the one with the smallest sleep time
	 */
	if (p2->p_slptime > p1->p_slptime)
		return (0);
	if (p1->p_slptime > p2->p_slptime)
		return (1);
	/*
	 * favor one sleeping in a non-interruptible sleep
	 */
	if (p1->p_flag & P_SINTR && (p2->p_flag & P_SINTR) == 0)
		return (1);
	if (p2->p_flag & P_SINTR && (p1->p_flag & P_SINTR) == 0)
		return (0);
#endif
	return (p2->p_pid > p1->p_pid);		/* tie - return highest pid */
}

/*
 * Output char to tty; console putchar style.
 * Can be called with tty lock held through kprintf() machinery..
 */
int
tputchar(int c, int flags, struct tty *tp)
{
	int r = 0;

	if ((flags & NOLOCK) == 0)
		mutex_spin_enter(&tty_lock);
	if (!CONNECTED(tp)) {
		r = -1;
		goto out;
	}
	if (c == '\n')
		(void)ttyoutput('\r', tp);
	(void)ttyoutput(c, tp);
	ttstart(tp);
out:
	if ((flags & NOLOCK) == 0)
		mutex_spin_exit(&tty_lock);
	return (r);
}

/*
 * Sleep on chan, returning ERESTART if tty changed while we napped and
 * returning any errors (e.g. EINTR/ETIMEDOUT) reported by tsleep.  If
 * the tty is revoked, restarting a pending call will redo validation done
 * at the start of the call.
 *
 * Must be called with the tty lock held.
 */
int
ttysleep(struct tty *tp, kcondvar_t *cv, bool catch, int timo)
{
	int	error;
	short	gen;

	KASSERT(mutex_owned(&tty_lock));

	gen = tp->t_gen;
	if (catch)
		error = cv_timedwait_sig(cv, &tty_lock, timo);
	else
		error = cv_timedwait(cv, &tty_lock, timo);
	if (error != 0)
		return (error);
	return (tp->t_gen == gen ? 0 : ERESTART);
}

/*
 * Attach a tty to the tty list.
 *
 * This should be called ONLY once per real tty (including pty's).
 * eg, on the sparc, the keyboard and mouse have struct tty's that are
 * distinctly NOT usable as tty's, and thus should not be attached to
 * the ttylist.  This is why this call is not done from ttymalloc().
 *
 * Device drivers should attach tty's at a similar time that they are
 * ttymalloc()'ed, or, for the case of statically allocated struct tty's
 * either in the attach or (first) open routine.
 */
void
tty_attach(struct tty *tp)
{

	mutex_spin_enter(&tty_lock);
	TAILQ_INSERT_TAIL(&ttylist, tp, tty_link);
	++tty_count;
	mutex_spin_exit(&tty_lock);
}

/*
 * Remove a tty from the tty list.
 */
void
tty_detach(struct tty *tp)
{

	mutex_spin_enter(&tty_lock);
	--tty_count;
#ifdef DIAGNOSTIC
	if (tty_count < 0)
		panic("tty_detach: tty_count < 0");
#endif
	TAILQ_REMOVE(&ttylist, tp, tty_link);
	mutex_spin_exit(&tty_lock);
}

/*
 * Allocate a tty structure and its associated buffers.
 */
struct tty *
ttymalloc(void)
{
	struct tty	*tp;
	int i;	

	tp = pool_get(&tty_pool, PR_WAITOK);
	memset(tp, 0, sizeof(*tp));
	callout_init(&tp->t_rstrt_ch, 0);
	callout_setfunc(&tp->t_rstrt_ch, ttrstrt, tp);
	/* XXX: default to 1024 chars for now */
	clalloc(&tp->t_rawq, 1024, 1);
	clalloc(&tp->t_canq, 1024, 1);
	/* output queue doesn't need quoting */
	clalloc(&tp->t_outq, 1024, 0);
	/* Set default line discipline. */
	tp->t_linesw = ttyldisc_default();
	selinit(&tp->t_rsel);
	selinit(&tp->t_wsel);
	for (i = 0; i < TTYSIG_COUNT; i++) 
		sigemptyset(&tp->t_sigs[i]);
	return (tp);
}

/*
 * Free a tty structure and its buffers.
 *
 * Be sure to call tty_detach() for any tty that has been
 * tty_attach()ed.
 */
void
ttyfree(struct tty *tp)
{
	int i;

	mutex_enter(&tty_lock);
	for (i = 0; i < TTYSIG_COUNT; i++) 
		sigemptyset(&tp->t_sigs[i]);
	if (tp->t_sigcount != 0)
		TAILQ_REMOVE(&tty_sigqueue, tp, t_sigqueue);
	mutex_exit(&tty_lock);

	callout_stop(&tp->t_rstrt_ch);
	ttyldisc_release(tp->t_linesw);
	clfree(&tp->t_rawq);
	clfree(&tp->t_canq);
	clfree(&tp->t_outq);
	callout_destroy(&tp->t_rstrt_ch);
	seldestroy(&tp->t_rsel);
	seldestroy(&tp->t_wsel);
	pool_put(&tty_pool, tp);
}

/*
 * ttyprintf_nolock: send a message to a specific tty, without locking.
 *
 * => should be used only by tty driver or anything that knows the
 *    underlying tty will not be revoked(2)'d away.  [otherwise,
 *    use tprintf]
 */
static void
ttyprintf_nolock(struct tty *tp, const char *fmt, ...)
{
	va_list ap;

	/* No mutex needed; going to process TTY. */
	va_start(ap, fmt);
	kprintf(fmt, TOTTY|NOLOCK, tp, NULL, ap);
	va_end(ap);
}

/*
 * Initialize the tty subsystem.
 */
void
tty_init(void)
{

	mutex_init(&tty_lock, MUTEX_DEFAULT, IPL_VM);
	tty_sigsih = softint_establish(SOFTINT_CLOCK, ttysigintr, NULL);
	KASSERT(tty_sigsih != NULL);
}

/*
 * Send a signal from a tty to its process group or session leader.
 * Handoff to the target is deferred to a soft interrupt.
 */
void
ttysig(struct tty *tp, enum ttysigtype st, int sig)
{
	sigset_t *sp;

	/* XXXSMP not yet KASSERT(mutex_owned(&tty_lock)); */

	sp = &tp->t_sigs[st];
	if (sigismember(sp, sig))
		return;
	sigaddset(sp, sig);
	if (tp->t_sigcount++ == 0)
		TAILQ_INSERT_TAIL(&tty_sigqueue, tp, t_sigqueue);
	softint_schedule(tty_sigsih);
}

/*
 * Deliver deferred signals from ttys.  Note that the process groups
 * and sessions associated with the ttys may have changed from when
 * the signal was originally sent, but in practice it should not matter.
 * For signals produced as a result of a syscall, the soft interrupt
 * will fire before the syscall returns to the user.
 */
static void
ttysigintr(void *cookie)
{
	struct tty *tp;
	enum ttysigtype st;
	struct pgrp *pgrp;
	struct session *sess;
	int sig;

	mutex_enter(&proclist_lock);
	for (;;) {
		mutex_spin_enter(&tty_lock);
		if ((tp = TAILQ_FIRST(&tty_sigqueue)) == NULL) {
			mutex_spin_exit(&tty_lock);
			break;
		}
		KASSERT(tp->t_sigcount > 0);
		for (st = 0; st < TTYSIG_COUNT; st++) {
			if ((sig = firstsig(&tp->t_sigs[st])) != 0)
				break;
		}
		KASSERT(st < TTYSIG_COUNT);
		sigdelset(&tp->t_sigs[st], sig);
		if (--tp->t_sigcount == 0)
			TAILQ_REMOVE(&tty_sigqueue, tp, t_sigqueue);
		pgrp = tp->t_pgrp;
		sess = tp->t_session;
		mutex_spin_exit(&tty_lock);
		if (sig == 0)
			panic("ttysigintr");
		mutex_enter(&proclist_mutex);
		switch (st) {
		case TTYSIG_PG1:
			if (pgrp != NULL)
				pgsignal(pgrp, sig, 1);
			break;
		case TTYSIG_PG2:
			if (pgrp != NULL)
				pgsignal(pgrp, sig, sess != NULL);
			break;
		case TTYSIG_LEADER:
			if (sess != NULL && sess->s_leader != NULL)
				psignal(sess->s_leader, sig);
			break;
		default:
			/* NOTREACHED */
			break;
		}
		mutex_exit(&proclist_mutex);
	}
	mutex_exit(&proclist_lock);
}
