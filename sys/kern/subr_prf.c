/*	$NetBSD: subr_prf.c,v 1.73 2000/05/29 23:10:03 jhawk Exp $	*/

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
 *	@(#)subr_prf.c	8.4 (Berkeley) 5/4/95
 */

#include "opt_ddb.h"
#include "opt_ipkdb.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
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
#include <sys/lock.h>

#include <dev/cons.h>

#ifdef DDB
#include <ddb/ddbvar.h>
#include <machine/db_machdep.h>
#include <ddb/db_command.h>
#include <ddb/db_interface.h>
#endif

#ifdef IPKDB
#include <ipkdb/ipkdb.h>
#endif

#if defined(MULTIPROCESSOR)
struct simplelock kprintf_slock = SIMPLELOCK_INITIALIZER;

/*
 * Use cpu_simple_lock() and cpu_simple_unlock().  These are the actual
 * atomic locking operations, and never attempt to print debugging
 * information.
 */
#define	KPRINTF_MUTEX_ENTER(s)						\
do {									\
	(s) = splhigh();						\
	__cpu_simple_lock(&kprintf_slock.lock_data);			\
} while (0)

#define	KPRINTF_MUTEX_EXIT(s)						\
do {									\
	__cpu_simple_unlock(&kprintf_slock.lock_data);			\
	splx((s));							\
} while (0)
#else /* ! MULTIPROCESSOR */
#define	KPRINTF_MUTEX_ENTER(s)	(s) = splhigh()
#define	KPRINTF_MUTEX_EXIT(s)	splx((s))
#endif /* MULTIPROCESSOR */

/*
 * note that stdarg.h and the ansi style va_start macro is used for both
 * ansi and traditional c complers.
 * XXX: this requires that stdarg.h define: va_alist and va_dcl
 */
#include <machine/stdarg.h>


#ifdef KGDB
#include <sys/kgdb.h>
#include <machine/cpu.h>
#endif
#ifdef DDB
#include <ddb/db_output.h>	/* db_printf, db_putchar prototypes */
#endif


/*
 * defines
 */

/* flags for kprintf */
#define TOCONS		0x01	/* to the console */
#define TOTTY		0x02	/* to the process' tty */
#define TOLOG		0x04	/* to the kernel message buffer */
#define TOBUFONLY	0x08	/* to the buffer (only) [for sprintf] */
#define TODDB		0x10	/* to ddb console */

/* max size buffer kprintf needs to print quad_t [size in base 8 + \0] */
#define KPRINTF_BUFSIZE		(sizeof(quad_t) * NBBY / 3 + 2)


/*
 * local prototypes
 */

static int	 kprintf __P((const char *, int, void *, 
				char *, va_list));
static void	 putchar __P((int, int, struct tty *));
static void	 klogpri __P((int));


/*
 * globals
 */

struct	tty *constty;	/* pointer to console "window" tty */
extern	int log_open;	/* subr_log: is /dev/klog open? */
const	char *panicstr; /* arg to first call to panic (used as a flag
			   to indicate that panic has already been called). */

/*
 * v_putc: routine to putc on virtual console
 *
 * the v_putc pointer can be used to redirect the console cnputc elsewhere
 * [e.g. to a "virtual console"].
 */

void (*v_putc) __P((int)) = cnputc;	/* start with cnputc (normal cons) */


/*
 * functions
 */

/*
 * tablefull: warn that a system table is full
 */

void
tablefull(tab)
	const char *tab;
{
	log(LOG_ERR, "%s: table is full\n", tab);
}

/*
 * panic: handle an unresolvable fatal error
 *
 * prints "panic: <message>" and reboots.   if called twice (i.e. recursive
 * call) we avoid trying to sync the disk and just reboot (to avoid 
 * recursive panics).
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
	printf("panic: ");
	vprintf(fmt, ap);
	printf("\n");
	va_end(ap);

#ifdef IPKDB
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
	else {
		static int intrace = 0;

		if (intrace==0) {
			intrace=1;
			printf("Begin traceback...\n");
			db_stack_trace_print(
			    (db_expr_t)__builtin_frame_address(0),
			    TRUE, 65535, "", printf);
			printf("End traceback...\n");
			intrace=0;
		} else
			printf("Faulted in mid-traceback; aborting...");
	}
#endif
	cpu_reboot(bootopt, NULL);
}

/*
 * kernel logging functions: log, logpri, addlog
 */

/*
 * log: write to the log buffer
 *
 * => will not sleep [so safe to call from interrupt]
 * => will log to console if /dev/klog isn't open
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
	int s;
	va_list ap;

	KPRINTF_MUTEX_ENTER(s);

	klogpri(level);		/* log the level first */
	va_start(ap, fmt);
	kprintf(fmt, TOLOG, NULL, NULL, ap);
	va_end(ap);
	if (!log_open) {
		va_start(ap, fmt);
		kprintf(fmt, TOCONS, NULL, NULL, ap);
		va_end(ap);
	}

	KPRINTF_MUTEX_EXIT(s);

	logwakeup();		/* wake up anyone waiting for log msgs */
}

/*
 * vlog: write to the log buffer [already have va_alist]
 */

void
vlog(level, fmt, ap)
	int level;
	const char *fmt;
	va_list ap;
{
	int s;

	KPRINTF_MUTEX_ENTER(s);

	klogpri(level);		/* log the level first */
	kprintf(fmt, TOLOG, NULL, NULL, ap);
	if (!log_open)
		kprintf(fmt, TOCONS, NULL, NULL, ap);

	KPRINTF_MUTEX_EXIT(s);

	logwakeup();		/* wake up anyone waiting for log msgs */
}

/*
 * logpri: log the priority level to the klog
 */

void
logpri(level)
	int level;
{
	int s;

	KPRINTF_MUTEX_ENTER(s);
	klogpri(level);
	KPRINTF_MUTEX_EXIT(s);
}

/*
 * Note: we must be in the mutex here!
 */
static void
klogpri(level)
	int level;
{
	char *p;
	char snbuf[KPRINTF_BUFSIZE];

	putchar('<', TOLOG, NULL);
	sprintf(snbuf, "%d", level);
	for (p = snbuf ; *p ; p++)
		putchar(*p, TOLOG, NULL);
	putchar('>', TOLOG, NULL);
}

/*
 * addlog: add info to previous log message
 */

void
#ifdef __STDC__
addlog(const char *fmt, ...)
#else
addlog(fmt, va_alist)
	char *fmt;
	va_dcl
#endif
{
	int s;
	va_list ap;

	KPRINTF_MUTEX_ENTER(s);

	va_start(ap, fmt);
	kprintf(fmt, TOLOG, NULL, NULL, ap);
	va_end(ap);
	if (!log_open) {
		va_start(ap, fmt);
		kprintf(fmt, TOCONS, NULL, NULL, ap);
		va_end(ap);
	}

	KPRINTF_MUTEX_EXIT(s);

	logwakeup();
}


/*
 * putchar: print a single character on console or user terminal.
 *
 * => if console, then the last MSGBUFS chars are saved in msgbuf
 *	for inspection later (e.g. dmesg/syslog)
 * => we must already be in the mutex!
 */
static void
putchar(c, flags, tp)
	int c;
	int flags;
	struct tty *tp;
{
	struct kern_msgbuf *mbp;

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
	    c != '\0' && c != '\r' && c != 0177 && msgbufenabled) {
		mbp = msgbufp;
		if (mbp->msg_magic != MSG_MAGIC) {
			/*
			 * Arguably should panic or somehow notify the
			 * user...  but how?  Panic may be too drastic,
			 * and would obliterate the message being kicked
			 * out (maybe a panic itself), and printf
			 * would invoke us recursively.  Silently punt
			 * for now.  If syslog is running, it should
			 * notice.
			 */
			msgbufenabled = 0;
		} else {
			mbp->msg_bufc[mbp->msg_bufx++] = c;
			if (mbp->msg_bufx < 0 || mbp->msg_bufx >= mbp->msg_bufs)
				mbp->msg_bufx = 0;
			/* If the buffer is full, keep the most recent data. */
			if (mbp->msg_bufr == mbp->msg_bufx) {
				 if (++mbp->msg_bufr >= mbp->msg_bufs)
					mbp->msg_bufr = 0;
			}
		}
	}
	if ((flags & TOCONS) && constty == NULL && c != '\0')
		(*v_putc)(c);
#ifdef DDB
	if (flags & TODDB)
		db_putchar(c);
#endif
}


/*
 * uprintf: print to the controlling tty of the current process
 *
 * => we may block if the tty queue is full
 * => no message is printed if the queue doesn't clear in a reasonable
 *	time
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
	struct proc *p = curproc;
	va_list ap;

	if (p->p_flag & P_CONTROLT && p->p_session->s_ttyvp) {
		/* No mutex needed; going to process TTY. */
		va_start(ap, fmt);
		kprintf(fmt, TOTTY, p->p_session->s_ttyp, NULL, ap);
		va_end(ap);
	}
}

/*
 * tprintf functions: used to send messages to a specific process
 *
 * usage:
 *   get a tpr_t handle on a process "p" by using "tprintf_open(p)"
 *   use the handle when calling "tprintf"
 *   when done, do a "tprintf_close" to drop the handle
 */

/*
 * tprintf_open: get a tprintf handle on a process "p"
 *
 * => returns NULL if process can't be printed to
 */

tpr_t
tprintf_open(p)
	struct proc *p;
{

	if (p->p_flag & P_CONTROLT && p->p_session->s_ttyvp) {
		SESSHOLD(p->p_session);
		return ((tpr_t) p->p_session);
	}
	return ((tpr_t) NULL);
}

/*
 * tprintf_close: dispose of a tprintf handle obtained with tprintf_open
 */

void
tprintf_close(sess)
	tpr_t sess;
{

	if (sess)
		SESSRELE((struct session *) sess);
}

/*
 * tprintf: given tprintf handle to a process [obtained with tprintf_open], 
 * send a message to the controlling tty for that process.
 *
 * => also sends message to /dev/klog
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
	struct session *sess = (struct session *)tpr;
	struct tty *tp = NULL;
	int s, flags = TOLOG;
	va_list ap;

	if (sess && sess->s_ttyvp && ttycheckoutq(sess->s_ttyp, 0)) {
		flags |= TOTTY;
		tp = sess->s_ttyp;
	}

	KPRINTF_MUTEX_ENTER(s);

	klogpri(LOG_INFO);
	va_start(ap, fmt);
	kprintf(fmt, flags, tp, NULL, ap);
	va_end(ap);

	KPRINTF_MUTEX_EXIT(s);

	logwakeup();
}


/*
 * ttyprintf: send a message to a specific tty
 *
 * => should be used only by tty driver or anything that knows the
 *    underlying tty will not be revoked(2)'d away.  [otherwise,
 *    use tprintf]
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

	/* No mutex needed; going to process TTY. */
	va_start(ap, fmt);
	kprintf(fmt, TOTTY, tp, NULL, ap);
	va_end(ap);
}

#ifdef DDB

/*
 * db_printf: printf for DDB (via db_putchar)
 */

void
#ifdef __STDC__
db_printf(const char *fmt, ...)
#else
db_printf(fmt, va_alist)
	char *fmt;
	va_dcl
#endif
{
	va_list ap;

	/* No mutex needed; DDB pauses all processors. */
	va_start(ap, fmt);
	kprintf(fmt, TODDB, NULL, NULL, ap);
	va_end(ap);
}

#endif /* DDB */


/*
 * normal kernel printf functions: printf, vprintf, sprintf
 */

/*
 * printf: print a message to the console and the log
 */
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
	int s;

	KPRINTF_MUTEX_ENTER(s);

	va_start(ap, fmt);
	kprintf(fmt, TOCONS | TOLOG, NULL, NULL, ap);
	va_end(ap);

	KPRINTF_MUTEX_EXIT(s);

	if (!panicstr)
		logwakeup();
}

/*
 * vprintf: print a message to the console and the log [already have
 *	va_alist]
 */

void
vprintf(fmt, ap)
	const char *fmt;
	va_list ap;
{
	int s;

	KPRINTF_MUTEX_ENTER(s);

	kprintf(fmt, TOCONS | TOLOG, NULL, NULL, ap);

	KPRINTF_MUTEX_EXIT(s);

	if (!panicstr)
		logwakeup();
}

/*
 * sprintf: print a message to a buffer
 */
int
#ifdef __STDC__
sprintf(char *buf, const char *fmt, ...)
#else
sprintf(buf, fmt, va_alist)
        char *buf;
        const char *cfmt;
        va_dcl
#endif
{
	int retval;
	va_list ap;

	va_start(ap, fmt);
	retval = kprintf(fmt, TOBUFONLY, NULL, buf, ap);
	va_end(ap);
	*(buf + retval) = 0;	/* null terminate */
	return(retval);
}

/*
 * vsprintf: print a message to a buffer [already have va_alist]
 */

int
vsprintf(buf, fmt, ap)
	char *buf;
	const char *fmt;
	va_list ap;
{
	int retval;

	retval = kprintf(fmt, TOBUFONLY, NULL, buf, ap);
	*(buf + retval) = 0;	/* null terminate */
	return (retval);
}

/*
 * snprintf: print a message to a buffer
 */
int
#ifdef __STDC__
snprintf(char *buf, size_t size, const char *fmt, ...)
#else
snprintf(buf, size, fmt, va_alist)
        char *buf;
        size_t size;
        const char *cfmt;
        va_dcl
#endif
{
	int retval;
	va_list ap;
	char *p;

	if (size < 1)
		return (-1);
	p = buf + size - 1;
	va_start(ap, fmt);
	retval = kprintf(fmt, TOBUFONLY, &p, buf, ap);
	va_end(ap);
	*(p) = 0;	/* null terminate */
	return(retval);
}

/*
 * vsnprintf: print a message to a buffer [already have va_alist]
 */
int
vsnprintf(buf, size, fmt, ap)
        char *buf;
        size_t size;
        const char *fmt;
        va_list ap;
{
	int retval;
	char *p;

	if (size < 1)
		return (-1);
	p = buf + size - 1;
	retval = kprintf(fmt, TOBUFONLY, &p, buf, ap);
	*(p) = 0;	/* null terminate */
	return(retval);
}

/*
 * bitmask_snprintf: print a kernel-printf "%b" message to a buffer
 *
 * => returns pointer to the buffer
 * => XXX: useful vs. kernel %b?
 */
char *
bitmask_snprintf(val, p, buf, buflen)
	u_quad_t val;
	const char *p;
	char *buf;
	size_t buflen;
{
	char *bp, *q;
	size_t left;
	char *sbase, snbuf[KPRINTF_BUFSIZE];
	int base, bit, ch, len, sep;
	u_quad_t field;

	bp = buf;
	memset(buf, 0, buflen);

	/*
	 * Always leave room for the trailing NULL.
	 */
	left = buflen - 1;

	/*
	 * Print the value into the buffer.  Abort if there's not
	 * enough room.
	 */
	if (buflen < KPRINTF_BUFSIZE)
		return (buf);

	ch = *p++;
	base = ch != '\177' ? ch : *p++;
	sbase = base == 8 ? "%qo" : base == 10 ? "%qd" : base == 16 ? "%qx" : 0;
	if (sbase == 0)
		return (buf);	/* punt if not oct, dec, or hex */

	sprintf(snbuf, sbase, val);
	for (q = snbuf ; *q ; q++) {
		*bp++ = *q;
		left--;
	}

	/*
	 * If the value we printed was 0 and we're using the old-style format,
	 * or if we don't have room for "<x>", we're done.
	 */
	if (((val == 0) && (ch != '\177')) || left < 3)
		return (buf);

#define PUTBYTE(b, c, l)	\
	*(b)++ = (c);		\
	if (--(l) == 0)		\
		goto out;
#define PUTSTR(b, p, l) do {		\
	int c;				\
	while ((c = *(p)++) != 0) {	\
		*(b)++ = c;		\
		if (--(l) == 0)		\
			goto out;	\
	}				\
} while (0)

	/*
	 * Chris Torek's new style %b format is identified by a leading \177
	 */
	sep = '<';
	if (ch != '\177') {
		/* old (standard) %b format. */
		for (;(bit = *p++) != 0;) {
			if (val & (1 << (bit - 1))) {
				PUTBYTE(bp, sep, left);
				for (; (ch = *p) > ' '; ++p) {
					PUTBYTE(bp, ch, left);
				}
				sep = ',';
			} else
				for (; *p > ' '; ++p)
					continue;
		}
	} else {
		/* new quad-capable %b format; also does fields. */
		field = val;
		while ((ch = *p++) != '\0') {
			bit = *p++;	/* now 0-origin */
			switch (ch) {
			case 'b':
				if (((u_int)(val >> bit) & 1) == 0)
					goto skip;
				PUTBYTE(bp, sep, left);
				PUTSTR(bp, p, left);
				sep = ',';
				break;
			case 'f':
			case 'F':
				len = *p++;	/* field length */
				field = (val >> bit) & ((1ULL << len) - 1);
				if (ch == 'F')	/* just extract */
					break;
				PUTBYTE(bp, sep, left);
				sep = ',';
				PUTSTR(bp, p, left);
				PUTBYTE(bp, '=', left);
				sprintf(snbuf, sbase, field);
				q = snbuf; PUTSTR(bp, q, left);
				break;
			case '=':
			case ':':
				/*
				 * Here "bit" is actually a value instead,
				 * to be compared against the last field.
				 * This only works for values in [0..255],
				 * of course.
				 */
				if ((int)field != bit)
					goto skip;
				if (ch == '=')
					PUTBYTE(bp, '=', left);
				PUTSTR(bp, p, left);
				break;
			default:
			skip:
				while (*p++ != '\0')
					continue;
				break;
			}
		}
	}
	if (sep != '<')
		PUTBYTE(bp, '>', left);

out:
	return (buf);

#undef PUTBYTE
#undef PUTSTR
}

/*
 * kprintf: scaled down version of printf(3).
 *
 * this version based on vfprintf() from libc which was derived from 
 * software contributed to Berkeley by Chris Torek.
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
 * this is the actual printf innards
 *
 * This code is large and complicated...
 *
 * NOTE: The kprintf mutex must be held of we're going TOBUF or TOCONS!
 */

/*
 * macros for converting digits to letters and vice versa
 */
#define	to_digit(c)	((c) - '0')
#define is_digit(c)	((unsigned)to_digit(c) <= 9)
#define	to_char(n)	((n) + '0')

/*
 * flags used during conversion.
 */
#define	ALT		0x001		/* alternate form */
#define	HEXPREFIX	0x002		/* add 0x or 0X prefix */
#define	LADJUST		0x004		/* left adjustment */
#define	LONGDBL		0x008		/* long double; unimplemented */
#define	LONGINT		0x010		/* long integer */
#define	QUADINT		0x020		/* quad integer */
#define	SHORTINT	0x040		/* short integer */
#define	ZEROPAD		0x080		/* zero (as opposed to blank) pad */
#define FPT		0x100		/* Floating point number */

	/*
	 * To extend shorts properly, we need both signed and unsigned
	 * argument extraction methods.
	 */
#define	SARG() \
	(flags&QUADINT ? va_arg(ap, quad_t) : \
	    flags&LONGINT ? va_arg(ap, long) : \
	    flags&SHORTINT ? (long)(short)va_arg(ap, int) : \
	    (long)va_arg(ap, int))
#define	UARG() \
	(flags&QUADINT ? va_arg(ap, u_quad_t) : \
	    flags&LONGINT ? va_arg(ap, u_long) : \
	    flags&SHORTINT ? (u_long)(u_short)va_arg(ap, int) : \
	    (u_long)va_arg(ap, u_int))

#define KPRINTF_PUTCHAR(C) {						\
	if (oflags == TOBUFONLY) {					\
		if ((vp != NULL) && (sbuf == tailp)) {			\
			ret += 1;		/* indicate error */	\
			goto overflow;					\
		}							\
		*sbuf++ = (C);						\
	} else {							\
		putchar((C), oflags, (struct tty *)vp);			\
	}								\
}

/*
 * Guts of kernel printf.  Note, we already expect to be in a mutex!
 */
static int
kprintf(fmt0, oflags, vp, sbuf, ap)
	const char *fmt0;
	int oflags;
	void *vp;
	char *sbuf;
	va_list ap;
{
	char *fmt;		/* format string */
	int ch;			/* character from fmt */
	int n;			/* handy integer (short term usage) */
	char *cp;		/* handy char pointer (short term usage) */
	int flags;		/* flags as above */
	int ret;		/* return value accumulator */
	int width;		/* width from format (%8d), or 0 */
	int prec;		/* precision from format (%.3d), or -1 */
	char sign;		/* sign prefix (' ', '+', '-', or \0) */

	u_quad_t _uquad;	/* integer arguments %[diouxX] */
	enum { OCT, DEC, HEX } base;/* base for [diouxX] conversion */
	int dprec;		/* a copy of prec if [diouxX], 0 otherwise */
	int realsz;		/* field size expanded by dprec */
	int size;		/* size of converted field or string */
	char *xdigs;		/* digits for [xX] conversion */
	char buf[KPRINTF_BUFSIZE]; /* space for %c, %[diouxX] */
	char *tailp;		/* tail pointer for snprintf */

	tailp = NULL;	/* XXX: shutup gcc */
	if (oflags == TOBUFONLY && (vp != NULL))
		tailp = *(char **)vp;

	cp = NULL;	/* XXX: shutup gcc */
	size = 0;	/* XXX: shutup gcc */

	fmt = (char *)fmt0;
	ret = 0;

	xdigs = NULL;		/* XXX: shut up gcc warning */

	/*
	 * Scan the format for conversions (`%' character).
	 */
	for (;;) {
		while (*fmt != '%' && *fmt) {
			ret++;
			KPRINTF_PUTCHAR(*fmt++);
		}
		if (*fmt == 0)
			goto done;

		fmt++;		/* skip over '%' */

		flags = 0;
		dprec = 0;
		width = 0;
		prec = -1;
		sign = '\0';

rflag:		ch = *fmt++;
reswitch:	switch (ch) {
		/* XXX: non-standard '%:' format */
#ifndef __powerpc__
		case ':': 
			if (oflags != TOBUFONLY) {
				cp = va_arg(ap, char *);
				kprintf(cp, oflags, vp, 
					NULL, va_arg(ap, va_list));
			}
			continue;	/* no output */
#endif
		/* XXX: non-standard '%b' format */
		case 'b': {
			char *b, *z;
			int tmp;
			_uquad = va_arg(ap, int);
			b = va_arg(ap, char *);
			if (*b == 8)
				sprintf(buf, "%qo", (unsigned long long)_uquad);
			else if (*b == 10)
				sprintf(buf, "%qd", (unsigned long long)_uquad);
			else if (*b == 16)
				sprintf(buf, "%qx", (unsigned long long)_uquad);
			else
				break;
			b++;

			z = buf;
			while (*z) {
				ret++;
				KPRINTF_PUTCHAR(*z++);
			}

			if (_uquad) {
				tmp = 0;
				while ((n = *b++) != 0) {
					if (_uquad & (1 << (n - 1))) {
						ret++;
						KPRINTF_PUTCHAR(tmp ? ',':'<');
						while ((n = *b) > ' ') {
							ret++;
							KPRINTF_PUTCHAR(n);
							b++;
						}
						tmp = 1;
					} else {
						while(*b > ' ')
							b++;
					}
				}
				if (tmp) {
					ret++;
					KPRINTF_PUTCHAR('>');
				}
			}
			continue;	/* no output */
		}

#ifdef DDB
		/* XXX: non-standard '%r' format (print int in db_radix) */
		case 'r':
			if (db_radix == 16)
				goto case_z;	/* signed hex */
			_uquad = SARG();
			if ((quad_t)_uquad < 0) {
				_uquad = -_uquad;
				sign = '-';
			}
			base = (db_radix == 8) ? OCT : DEC;
			goto number;


		/* XXX: non-standard '%z' format ("signed hex", a "hex %i")*/
		case 'z':
		case_z:
			xdigs = "0123456789abcdef";
			ch = 'x';	/* the 'x' in '0x' (below) */
			_uquad = SARG();
			base = HEX;
			/* leading 0x/X only if non-zero */
			if (flags & ALT && _uquad != 0)
				flags |= HEXPREFIX;
			if ((quad_t)_uquad < 0) {
				_uquad = -_uquad;
				sign = '-';
			}
			goto number;
#endif

		case ' ':
			/*
			 * ``If the space and + flags both appear, the space
			 * flag will be ignored.''
			 *	-- ANSI X3J11
			 */
			if (!sign)
				sign = ' ';
			goto rflag;
		case '#':
			flags |= ALT;
			goto rflag;
		case '*':
			/*
			 * ``A negative field width argument is taken as a
			 * - flag followed by a positive field width.''
			 *	-- ANSI X3J11
			 * They don't exclude field widths read from args.
			 */
			if ((width = va_arg(ap, int)) >= 0)
				goto rflag;
			width = -width;
			/* FALLTHROUGH */
		case '-':
			flags |= LADJUST;
			goto rflag;
		case '+':
			sign = '+';
			goto rflag;
		case '.':
			if ((ch = *fmt++) == '*') {
				n = va_arg(ap, int);
				prec = n < 0 ? -1 : n;
				goto rflag;
			}
			n = 0;
			while (is_digit(ch)) {
				n = 10 * n + to_digit(ch);
				ch = *fmt++;
			}
			prec = n < 0 ? -1 : n;
			goto reswitch;
		case '0':
			/*
			 * ``Note that 0 is taken as a flag, not as the
			 * beginning of a field width.''
			 *	-- ANSI X3J11
			 */
			flags |= ZEROPAD;
			goto rflag;
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			n = 0;
			do {
				n = 10 * n + to_digit(ch);
				ch = *fmt++;
			} while (is_digit(ch));
			width = n;
			goto reswitch;
		case 'h':
			flags |= SHORTINT;
			goto rflag;
		case 'l':
			if (*fmt == 'l') {
				fmt++;
				flags |= QUADINT;
			} else {
				flags |= LONGINT;
			}
			goto rflag;
		case 'q':
			flags |= QUADINT;
			goto rflag;
		case 'c':
			*(cp = buf) = va_arg(ap, int);
			size = 1;
			sign = '\0';
			break;
		case 'D':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'd':
		case 'i':
			_uquad = SARG();
			if ((quad_t)_uquad < 0) {
				_uquad = -_uquad;
				sign = '-';
			}
			base = DEC;
			goto number;
		case 'n':
#ifdef DDB
		/* XXX: non-standard '%n' format */
		/*
		 * XXX: HACK!   DDB wants '%n' to be a '%u' printed
		 * in db_radix format.   this should die since '%n'
		 * is already defined in standard printf to write
		 * the number of chars printed so far to the arg (which
		 * should be a pointer.
		 */
			if (oflags & TODDB) {
				if (db_radix == 16)
					ch = 'x';	/* convert to %x */
				else if (db_radix == 8)
					ch = 'o';	/* convert to %o */
				else
					ch = 'u';	/* convert to %u */

				/* ... and start again */
				goto reswitch;
			}

#endif
			if (flags & QUADINT)
				*va_arg(ap, quad_t *) = ret;
			else if (flags & LONGINT)
				*va_arg(ap, long *) = ret;
			else if (flags & SHORTINT)
				*va_arg(ap, short *) = ret;
			else
				*va_arg(ap, int *) = ret;
			continue;	/* no output */
		case 'O':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'o':
			_uquad = UARG();
			base = OCT;
			goto nosign;
		case 'p':
			/*
			 * ``The argument shall be a pointer to void.  The
			 * value of the pointer is converted to a sequence
			 * of printable characters, in an implementation-
			 * defined manner.''
			 *	-- ANSI X3J11
			 */
			/* NOSTRICT */
			_uquad = (u_long)va_arg(ap, void *);
			base = HEX;
			xdigs = "0123456789abcdef";
			flags |= HEXPREFIX;
			ch = 'x';
			goto nosign;
		case 's':
			if ((cp = va_arg(ap, char *)) == NULL)
				cp = "(null)";
			if (prec >= 0) {
				/*
				 * can't use strlen; can only look for the
				 * NUL in the first `prec' characters, and
				 * strlen() will go further.
				 */
				char *p = memchr(cp, 0, prec);

				if (p != NULL) {
					size = p - cp;
					if (size > prec)
						size = prec;
				} else
					size = prec;
			} else
				size = strlen(cp);
			sign = '\0';
			break;
		case 'U':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'u':
			_uquad = UARG();
			base = DEC;
			goto nosign;
		case 'X':
			xdigs = "0123456789ABCDEF";
			goto hex;
		case 'x':
			xdigs = "0123456789abcdef";
hex:			_uquad = UARG();
			base = HEX;
			/* leading 0x/X only if non-zero */
			if (flags & ALT && _uquad != 0)
				flags |= HEXPREFIX;

			/* unsigned conversions */
nosign:			sign = '\0';
			/*
			 * ``... diouXx conversions ... if a precision is
			 * specified, the 0 flag will be ignored.''
			 *	-- ANSI X3J11
			 */
number:			if ((dprec = prec) >= 0)
				flags &= ~ZEROPAD;

			/*
			 * ``The result of converting a zero value with an
			 * explicit precision of zero is no characters.''
			 *	-- ANSI X3J11
			 */
			cp = buf + KPRINTF_BUFSIZE;
			if (_uquad != 0 || prec != 0) {
				/*
				 * Unsigned mod is hard, and unsigned mod
				 * by a constant is easier than that by
				 * a variable; hence this switch.
				 */
				switch (base) {
				case OCT:
					do {
						*--cp = to_char(_uquad & 7);
						_uquad >>= 3;
					} while (_uquad);
					/* handle octal leading 0 */
					if (flags & ALT && *cp != '0')
						*--cp = '0';
					break;

				case DEC:
					/* many numbers are 1 digit */
					while (_uquad >= 10) {
						*--cp = to_char(_uquad % 10);
						_uquad /= 10;
					}
					*--cp = to_char(_uquad);
					break;

				case HEX:
					do {
						*--cp = xdigs[_uquad & 15];
						_uquad >>= 4;
					} while (_uquad);
					break;

				default:
					cp = "bug in kprintf: bad base";
					size = strlen(cp);
					goto skipsize;
				}
			}
			size = buf + KPRINTF_BUFSIZE - cp;
		skipsize:
			break;
		default:	/* "%?" prints ?, unless ? is NUL */
			if (ch == '\0')
				goto done;
			/* pretend it was %c with argument ch */
			cp = buf;
			*cp = ch;
			size = 1;
			sign = '\0';
			break;
		}

		/*
		 * All reasonable formats wind up here.  At this point, `cp'
		 * points to a string which (if not flags&LADJUST) should be
		 * padded out to `width' places.  If flags&ZEROPAD, it should
		 * first be prefixed by any sign or other prefix; otherwise,
		 * it should be blank padded before the prefix is emitted.
		 * After any left-hand padding and prefixing, emit zeroes
		 * required by a decimal [diouxX] precision, then print the
		 * string proper, then emit zeroes required by any leftover
		 * floating precision; finally, if LADJUST, pad with blanks.
		 *
		 * Compute actual size, so we know how much to pad.
		 * size excludes decimal prec; realsz includes it.
		 */
		realsz = dprec > size ? dprec : size;
		if (sign)
			realsz++;
		else if (flags & HEXPREFIX)
			realsz+= 2;

		/* adjust ret */
		ret += width > realsz ? width : realsz;

		/* right-adjusting blank padding */
		if ((flags & (LADJUST|ZEROPAD)) == 0) {
			n = width - realsz;
			while (n-- > 0)
				KPRINTF_PUTCHAR(' ');
		}

		/* prefix */
		if (sign) {
			KPRINTF_PUTCHAR(sign);
		} else if (flags & HEXPREFIX) {
			KPRINTF_PUTCHAR('0');
			KPRINTF_PUTCHAR(ch);
		}

		/* right-adjusting zero padding */
		if ((flags & (LADJUST|ZEROPAD)) == ZEROPAD) {
			n = width - realsz;
			while (n-- > 0)
				KPRINTF_PUTCHAR('0');
		}

		/* leading zeroes from decimal precision */
		n = dprec - size;
		while (n-- > 0)
			KPRINTF_PUTCHAR('0');

		/* the string or number proper */
		while (size--)
			KPRINTF_PUTCHAR(*cp++);
		/* left-adjusting padding (always blank) */
		if (flags & LADJUST) {
			n = width - realsz;
			while (n-- > 0)
				KPRINTF_PUTCHAR(' ');
		}
	}

done:
	if ((oflags == TOBUFONLY) && (vp != NULL))
		*(char **)vp = sbuf;
overflow:
	return (ret);
	/* NOTREACHED */
}
