/*	$NetBSD: subr_prf.c,v 1.43 1997/06/26 05:17:45 thorpej Exp $	*/

/*-
 * Copyright (c) 1986, 1988, 1991, 1993
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
 *	@(#)subr_prf.c	8.3 (Berkeley) 1/21/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/tty.h>
#include <sys/tprintf.h>
#include <sys/syslog.h>
#include <sys/malloc.h>

#include <dev/cons.h>

/*
 * Note that stdarg.h and the ANSI style va_start macro is used for both
 * ANSI and traditional C compilers.
 * XXX: This requires that stdarg.h defines: va_alist, va_dcl
 */
#include <machine/stdarg.h>

#include "ipkdb.h"

#ifdef KADB
#include <machine/kdbparam.h>
#endif
#ifdef KGDB
#include <sys/kgdb.h>
#include <machine/cpu.h>
#endif

#define TOCONS	0x01
#define TOTTY	0x02
#define TOLOG	0x04

/*
 * This is the size of the buffer that should be passed to ksnprintn().
 * It's the length of a long in base 8, plus NULL.
 */
#define KSNPRINTN_BUFSIZE	(sizeof(quad_t) * NBBY / 3 + 2)

struct	tty *constty;			/* pointer to console "window" tty */

void	(*v_putc) __P((int)) = cnputc;	/* routine to putc on virtual console */

static void putchar __P((int, int, struct tty *));
static char *ksnprintn __P((u_quad_t, int, int *, char *, size_t));
void kprintf __P((const char *, int, struct tty *, va_list));

int consintr = 1;			/* Ok to handle console interrupts? */

/*
 * Variable panicstr contains argument to first call to panic; used as flag
 * to indicate that the kernel has already called panic.
 */
const char *panicstr;

/*
 * Panic is called on unresolvable fatal errors.  It prints "panic: mesg",
 * and then reboots.  If we are called twice, then we avoid trying to sync
 * the disks as this often leads to recursive panics.
 */
void
#ifdef __STDC__
panic(const char *fmt, ...)
#else
panic(fmt, va_alist)
	char *fmt;
	va_dcl
#endif
{
	int bootopt;
	va_list ap;

	bootopt = RB_AUTOBOOT | RB_DUMP;
	if (panicstr)
		bootopt |= RB_NOSYNC;
	else
		panicstr = fmt;

	va_start(ap, fmt);
#ifdef __powerpc__				/* XXX */
	printf("panic: ");			/* XXX */
	vprintf(fmt, ap);			/* XXX */
	printf("\n");				/* XXX */
#else						/* XXX */
	printf("panic: %:\n", fmt, ap);		/* XXX */
#endif						/* XXX */
	va_end(ap);

#if NIPKDB > 0
	ipkdb_panic();
#endif
#ifdef KGDB
	kgdb_panic();
#endif
#ifdef KADB
	if (boothowto & RB_KDB)
		kdbpanic();
#endif
#ifdef DDB
	if (db_onpanic)
		Debugger();
#endif
	cpu_reboot(bootopt, NULL);
}

/*
 * Warn that a system table is full.
 */
void
tablefull(tab)
	const char *tab;
{

	log(LOG_ERR, "%s: table is full\n", tab);
}

/*
 * Uprintf prints to the controlling terminal for the current process.
 * It may block if the tty queue is overfull.  No message is printed if
 * the queue does not clear in a reasonable time.
 */
void
#ifdef __STDC__
uprintf(const char *fmt, ...)
#else
uprintf(fmt, va_alist)
	char *fmt;
	va_dcl
#endif
{
	register struct proc *p = curproc;
	va_list ap;

	if (p->p_flag & P_CONTROLT && p->p_session->s_ttyvp) {
		va_start(ap, fmt);
		kprintf(fmt, TOTTY, p->p_session->s_ttyp, ap);
		va_end(ap);
	}
}

tpr_t
tprintf_open(p)
	register struct proc *p;
{

	if (p->p_flag & P_CONTROLT && p->p_session->s_ttyvp) {
		SESSHOLD(p->p_session);
		return ((tpr_t) p->p_session);
	}
	return ((tpr_t) NULL);
}

void
tprintf_close(sess)
	tpr_t sess;
{

	if (sess)
		SESSRELE((struct session *) sess);
}

/*
 * tprintf prints on the controlling terminal associated
 * with the given session.
 */
void
#ifdef __STDC__
tprintf(tpr_t tpr, const char *fmt, ...)
#else
tprintf(tpr, fmt, va_alist)
	tpr_t tpr;
	char *fmt;
	va_dcl
#endif
{
	register struct session *sess = (struct session *)tpr;
	struct tty *tp = NULL;
	int flags = TOLOG;
	va_list ap;

	logpri(LOG_INFO);
	if (sess && sess->s_ttyvp && ttycheckoutq(sess->s_ttyp, 0)) {
		flags |= TOTTY;
		tp = sess->s_ttyp;
	}
	va_start(ap, fmt);
	kprintf(fmt, flags, tp, ap);
	va_end(ap);
	logwakeup();
}

/*
 * Ttyprintf displays a message on a tty; it should be used only by
 * the tty driver, or anything that knows the underlying tty will not
 * be revoke(2)'d away.  Other callers should use tprintf.
 */
void
#ifdef __STDC__
ttyprintf(struct tty *tp, const char *fmt, ...)
#else
ttyprintf(tp, fmt, va_alist)
	struct tty *tp;
	char *fmt;
	va_dcl
#endif
{
	va_list ap;

	va_start(ap, fmt);
	kprintf(fmt, TOTTY, tp, ap);
	va_end(ap);
}

extern	int log_open;

/*
 * Log writes to the log buffer, and guarantees not to sleep (so can be
 * called by interrupt routines).  If there is no process reading the
 * log yet, it writes to the console also.
 */
void
#ifdef __STDC__
log(int level, const char *fmt, ...)
#else
log(level, fmt, va_alist)
	int level;
	char *fmt;
	va_dcl
#endif
{
	register int s;
	va_list ap;

	s = splhigh();
	logpri(level);
	va_start(ap, fmt);
	kprintf(fmt, TOLOG, NULL, ap);
	splx(s);
	va_end(ap);
	if (!log_open) {
		va_start(ap, fmt);
		kprintf(fmt, TOCONS, NULL, ap);
		va_end(ap);
	}
	logwakeup();
}

void
logpri(level)
	int level;
{
	register int ch;
	register char *p;
	char snbuf[KSNPRINTN_BUFSIZE];

	putchar('<', TOLOG, NULL);
	for (p = ksnprintn((u_long)level, 10, NULL, snbuf, sizeof(snbuf));
	    (ch = *p--) != 0;)
		putchar(ch, TOLOG, NULL);
	putchar('>', TOLOG, NULL);
}

void
#ifdef __STDC__
addlog(const char *fmt, ...)
#else
addlog(fmt, va_alist)
	char *fmt;
	va_dcl
#endif
{
	register int s;
	va_list ap;

	s = splhigh();
	va_start(ap, fmt);
	kprintf(fmt, TOLOG, NULL, ap);
	splx(s);
	va_end(ap);
	if (!log_open) {
		va_start(ap, fmt);
		kprintf(fmt, TOCONS, NULL, ap);
		va_end(ap);
	}
	logwakeup();
}

void
#ifdef __STDC__
printf(const char *fmt, ...)
#else
printf(fmt, va_alist)
	char *fmt;
	va_dcl
#endif
{
	va_list ap;
	register int savintr;

	savintr = consintr;		/* disable interrupts */
	consintr = 0;
	va_start(ap, fmt);
	kprintf(fmt, TOCONS | TOLOG, NULL, ap);
	va_end(ap);
	if (!panicstr)
		logwakeup();
	consintr = savintr;		/* reenable interrupts */
}

#ifdef __powerpc__			/* XXX XXX XXX */
void
vprintf(fmt, ap)
	const char *fmt;
	va_list ap;
{
	register int savintr;

	savintr = consintr;		/* disable interrupts */
	consintr = 0;
	kprintf(fmt, TOCONS | TOLOG, NULL, ap);
	if (!panicstr)
		logwakeup();
	consintr = savintr;		/* reenable interrupts */
}
#endif /* __powerpc__ */		/* XXX XXX XXX */

/*
 * Scaled down version of printf(3).
 *
 * Two additional formats:
 *
 * The format %b is supported to decode error registers.
 * Its usage is:
 *
 *	printf("reg=%b\n", regval, "<base><arg>*");
 *
 * where <base> is the output base expressed as a control character, e.g.
 * \10 gives octal; \20 gives hex.  Each arg is a sequence of characters,
 * the first of which gives the bit number to be inspected (origin 1), and
 * the next characters (up to a control character, i.e. a character <= 32),
 * give the name of the register.  Thus:
 *
 *	kprintf("reg=%b\n", 3, "\10\2BITTWO\1BITONE\n");
 *
 * would produce output:
 *
 *	reg=3<BITTWO,BITONE>
 *
 * The format %: passes an additional format string and argument list
 * recursively.  Its usage is:
 *
 * fn(char *fmt, ...)
 * {
 *	va_list ap;
 *	va_start(ap, fmt);
 *	printf("prefix: %: suffix\n", fmt, ap);
 *	va_end(ap);
 * }
 *
 * Space or zero padding and a field width are supported for the numeric
 * formats only.
 */
#define	LONGINT		0x010		/* long integer */
#define	QUADINT		0x020		/* quad integer */
#define	SARG() \
	(flags&QUADINT ? va_arg(ap, quad_t) : \
	    flags&LONGINT ? va_arg(ap, long) : \
	    (long)va_arg(ap, int))
#define	UARG() \
	(flags&QUADINT ? va_arg(ap, u_quad_t) : \
	    flags&LONGINT ? va_arg(ap, u_long) : \
	    (u_long)va_arg(ap, u_int))
void
kprintf(fmt, oflags, tp, ap)
	register const char *fmt;
	int oflags;
	struct tty *tp;
	va_list ap;
{
	register char *p, *q;
	register int ch, n;
	u_quad_t ul;
	int base, flags, tmp, width;
	char padc, snbuf[KSNPRINTN_BUFSIZE];

	for (;;) {
		padc = ' ';
		width = 0;
		while ((ch = *(const u_char *)fmt++) != '%') {
			if (ch == '\0')
				return;
			putchar(ch, oflags, tp);
		}
		flags = 0;
reswitch:	switch (ch = *(const u_char *)fmt++) {
		case '\0':
			/* XXX Print the last format character? */
			return;
		case '0':
		case '.':
			padc = '0';
			goto reswitch;
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			for (width = 0;; ++fmt) {
				width = width * 10 + ch - '0';
				ch = *fmt;
				if (ch < '0' || ch > '9')
					break;
			}
			goto reswitch;
		case 'l':
			flags |= LONGINT;
			goto reswitch;
		case 'q':
			flags |= QUADINT;
			goto reswitch;
		case 'b':
			ul = va_arg(ap, int);
			p = va_arg(ap, char *);
			for (q = ksnprintn(ul, *p++, NULL, snbuf,
			    sizeof(snbuf)); (ch = *q--) != 0;)
				putchar(ch, oflags, tp);

			if (!ul)
				break;

			for (tmp = 0; (n = *p++) != 0;) {
				if (ul & (1 << (n - 1))) {
					putchar(tmp ? ',' : '<', oflags, tp);
					for (; (n = *p) > ' '; ++p)
						putchar(n, oflags, tp);
					tmp = 1;
				} else
					for (; *p > ' '; ++p)
						continue;
			}
			if (tmp)
				putchar('>', oflags, tp);
			break;
		case 'c':
			putchar(va_arg(ap, int), oflags, tp);
			break;
#ifndef __powerpc__			/* XXX XXX XXX */
		case ':':
			p = va_arg(ap, char *);
			kprintf(p, oflags, tp, va_arg(ap, va_list));
			break;
#endif /* __powerpc__ */		/* XXX XXX XXX */
		case 's':
			if ((p = va_arg(ap, char *)) == NULL)
				p = "(null)";
			while ((ch = *p++) != 0)
				putchar(ch, oflags, tp);
			break;
		case 'd':
		        ul = SARG();
			if ((quad_t)ul < 0) {
				putchar('-', oflags, tp);
				ul = -ul;
			}
			base = 10;
			goto number;
		case 'o':
			ul = UARG();
			base = 8;
			goto number;
		case 'u':
			ul = UARG();
			base = 10;
			goto number;
		case 'p':
			putchar('0', oflags, tp);
			putchar('x', oflags, tp);
			ul = (u_long)va_arg(ap, void *);
			base = 16;
			goto number;
		case 'x':
			ul = UARG();
			base = 16;
number:			p = ksnprintn(ul, base, &tmp, snbuf, sizeof(snbuf));
			if (width && (width -= tmp) > 0)
				while (width--)
					putchar(padc, oflags, tp);
			while ((ch = *p--) != 0)
				putchar(ch, oflags, tp);
			break;
		default:
			putchar('%', oflags, tp);
		        /* flags??? */
			/* FALLTHROUGH */
		case '%':
			putchar(ch, oflags, tp);
		}
	}
}

/*
 * Print a character on console or users terminal.  If destination is
 * the console then the last MSGBUFS characters are saved in msgbuf for
 * inspection later.
 */
static void
putchar(c, flags, tp)
	register int c;
	int flags;
	struct tty *tp;
{
	extern int msgbufmapped;
	register struct msgbuf *mbp;

	if (panicstr)
		constty = NULL;
	if ((flags & TOCONS) && tp == NULL && constty) {
		tp = constty;
		flags |= TOTTY;
	}
	if ((flags & TOTTY) && tp && tputchar(c, tp) < 0 &&
	    (flags & TOCONS) && tp == constty)
		constty = NULL;
	if ((flags & TOLOG) &&
	    c != '\0' && c != '\r' && c != 0177 && msgbufmapped) {
		mbp = msgbufp;
		if (mbp->msg_magic != MSG_MAGIC) {
			bzero((caddr_t)mbp, sizeof(*mbp));
			mbp->msg_magic = MSG_MAGIC;
		}
		mbp->msg_bufc[mbp->msg_bufx++] = c;
		if (mbp->msg_bufx < 0 || mbp->msg_bufx >= MSG_BSIZE)
			mbp->msg_bufx = 0;
	}
	if ((flags & TOCONS) && constty == NULL && c != '\0')
		(*v_putc)(c);
}

/*
 * Scaled down version of sprintf(3).
 */
int
#ifdef __STDC__
sprintf(char *buf, const char *cfmt, ...)
#else
sprintf(buf, cfmt, va_alist)
	char *buf;
	const char *cfmt;
	va_dcl
#endif
{
	register const char *fmt = cfmt;
	register char *p, *bp;
	register int ch, base;
	u_quad_t ul;
	int flags, tmp, width;
	va_list ap;
	char padc, snbuf[KSNPRINTN_BUFSIZE];

	va_start(ap, cfmt);
	for (bp = buf; ; ) {
		padc = ' ';
		width = 0;
		while ((ch = *(const u_char *)fmt++) != '%')
			if ((*bp++ = ch) == '\0')
				return ((bp - buf) - 1);

		flags = 0;
reswitch:	switch (ch = *(const u_char *)fmt++) {
		case '\0':
			/* XXX Store the last format character? */
			*bp++ = '\0';
			return ((bp - buf) - 1);
		case '0':
			padc = '0';
			goto reswitch;
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			for (width = 0;; ++fmt) {
				width = width * 10 + ch - '0';
				ch = *fmt;
				if (ch < '0' || ch > '9')
					break;
			}
			goto reswitch;
		case 'l':
			flags |= LONGINT;
			goto reswitch;
		case 'q':
			flags |= QUADINT;
			goto reswitch;
		/* case 'b': ... break; XXX */
		case 'c':
			*bp++ = va_arg(ap, int);
			break;
		/* case 'r': ... break; XXX */
		case 's':
			p = va_arg(ap, char *);
			while ((*bp++ = *p++) != 0)
				continue;
			--bp;
			break;
		case 'd':
		        ul = SARG();
			if ((quad_t)ul < 0) {
				*bp++ = '-';
				ul = -ul;
			}
			base = 10;
			goto number;
		case 'o':
			ul = UARG();
			base = 8;
			goto number;
		case 'u':
			ul = UARG();
			base = 10;
			goto number;
		case 'p':
			*bp++ = '0';
			*bp++ = 'x';
			ul = (u_long)va_arg(ap, void *);
			base = 16;
			goto number;
		case 'x':
			ul = UARG();
			base = 16;
number:			p = ksnprintn(ul, base, &tmp, snbuf, sizeof(snbuf));
			if (width && (width -= tmp) > 0)
				while (width--)
					*bp++ = padc;
			while ((ch = *p--) != 0)
				*bp++ = ch;
			break;
		default:
			*bp++ = '%';
		        /* flags??? */
			/* FALLTHROUGH */
		case '%':
			*bp++ = ch;
		}
	}
	va_end(ap);
}

/*
 * Put a number (base <= 16) in a buffer in reverse order; return an
 * optional length and a pointer to the NULL terminated (preceded?)
 * buffer.
 */
static char *
ksnprintn(ul, base, lenp, buf, buflen)
	register u_quad_t ul;
	register int base, *lenp;
	char *buf;
	size_t buflen;
{
	register char *p;

	p = buf;
	*p = '\0';			/* ensure NULL `termination' */

	/*
	 * Don't even bother of the buffer's not big enough.  No
	 * value at all is better than a wrong value, and we
	 * have a lot of control over the buffer that's passed
	 * to this function, since it's not exported.
	 */
	if (buflen < KSNPRINTN_BUFSIZE)
		return (p);

	do {
		*++p = "0123456789abcdef"[ul % base];
	} while (ul /= base);
	if (lenp)
		*lenp = p - buf;
	return (p);
}

/*
 * Print a bitmask into the provided buffer, and return a pointer
 * to that buffer.
 */
char *
bitmask_snprintf(ul, p, buf, buflen)
	u_long ul;
	const char *p;
	char *buf;
	size_t buflen;
{
	char *bp, *q;
	size_t left;
	register int n;
	int ch, tmp;
	char snbuf[KSNPRINTN_BUFSIZE];

	bp = buf;
	bzero(buf, buflen);

	/*
	 * Always leave room for the trailing NULL.
	 */
	left = buflen - 1;

	/*
	 * Print the value into the buffer.  Abort if there's not
	 * enough room.
	 */
	if (buflen < KSNPRINTN_BUFSIZE)
		return (buf);

	for (q = ksnprintn(ul, *p++, NULL, snbuf, sizeof(snbuf));
	    (ch = *q--) != 0;) {
		*bp++ = ch;
		left--;
	}

	/*
	 * If the value we printed was 0, or if we don't have room for
	 * "<x>", we're done.
	 */
	if (ul == 0 || left < 3)
		return (buf);

#define PUTBYTE(b, c, l)	\
	*(b)++ = (c);		\
	if (--(l) == 0)		\
		goto out;

	for (tmp = 0; (n = *p++) != 0;) {
		if (ul & (1 << (n - 1))) {
			PUTBYTE(bp, tmp ? ',' : '<', left);
				for (; (n = *p) > ' '; ++p) {
					PUTBYTE(bp, n, left);
				}
				tmp = 1;
		} else
			for (; *p > ' '; ++p)
				continue;
	}
	if (tmp)
		*bp = '>';

#undef PUTBYTE

 out:
	return (buf);
}
