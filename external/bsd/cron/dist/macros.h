/*	$NetBSD: macros.h,v 1.2 2010/05/06 18:53:17 christos Exp $	*/

/*
 * Id: macros.h,v 1.9 2004/01/23 18:56:43 vixie Exp
 */

/*
 * Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 1997,2000 by Internet Software Consortium, Inc.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

	/* these are really immutable, and are
	 *   defined for symbolic convenience only
	 * TRUE, FALSE, and ERR must be distinct
	 * ERR must be < OK.
	 */
#define TRUE		1
#define FALSE		0
	/* system calls return this on success */
#define OK		0
	/*   or this on error */
#define ERR		(-1)

	/* turn this on to get '-x' code */
#ifndef DEBUGGING
#define DEBUGGING	FALSE
#endif

#define	INIT_PID	1	/* parent of orphans */
#define READ_PIPE	0	/* which end of a pipe pair do you read? */
#define WRITE_PIPE	1	/*   or write to? */
#define STDIN		0	/* what is stdin's file descriptor? */
#define STDOUT		1	/*   stdout's? */
#define STDERR		2	/*   stderr's? */
#define ERROR_EXIT	1	/* exit() with this will scare the shell */
#define	OK_EXIT		0	/* exit() with this is considered 'normal' */
#define	MAX_FNAME	100	/* max length of internally generated fn */
#define	MAX_COMMAND	1000	/* max length of internally generated cmd */
#define	MAX_ENVSTR	1000	/* max length of envvar=value\0 strings */
#define	MAX_TEMPSTR	100	/* obvious */
#define	MAX_UNAME	33	/* max length of username, should be overkill */
#define	ROOT_UID	0	/* don't change this, it really must be root */
#define	ROOT_USER	"root"	/* ditto */

				/* NOTE: these correspond to DebugFlagNames,
				 *	defined below.
				 */
#define	DEXT		0x0001	/* extend flag for other debug masks */
#define	DSCH		0x0002	/* scheduling debug mask */
#define	DPROC		0x0004	/* process control debug mask */
#define	DPARS		0x0008	/* parsing debug mask */
#define	DLOAD		0x0010	/* database loading debug mask */
#define	DMISC		0x0020	/* misc debug mask */
#define	DTEST		0x0040	/* test mode: don't execute any commands */

#define	PPC_NULL	((const char **)NULL)

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

#define	Skip_Blanks(c, f) \
			while (c == '\t' || c == ' ') \
				c = get_char(f)

#define	Skip_Nonblanks(c, f) \
			while (c!='\t' && c!=' ' && c!='\n' && c != EOF) \
				c = get_char(f)

#if DEBUGGING
# define Debug(mask, message) \
			if ((DebugFlags & (mask)) != 0) \
				printf message
#else /* !DEBUGGING */
# define Debug(mask, message) do {} while (/*CONSTCOND*/0)
#endif /* DEBUGGING */

#define	Set_LineNum(ln)	do { \
	Debug(DPARS|DEXT,("linenum=%d\n", ln)); \
	LineNumber = ln; \
} while (/*CONSTCOND*/0)

#ifdef HAVE_TM_GMTOFF
#define	get_gmtoff(c, t)	((t)->tm_gmtoff)
#endif

#define	SECONDS_PER_MINUTE	60
#define	SECONDS_PER_HOUR	3600

#define	FIRST_MINUTE	0
#define	LAST_MINUTE	59
#define	MINUTE_COUNT	(LAST_MINUTE - FIRST_MINUTE + 1)

#define	FIRST_HOUR	0
#define	LAST_HOUR	23
#define	HOUR_COUNT	(LAST_HOUR - FIRST_HOUR + 1)

#define	FIRST_DOM	1
#define	LAST_DOM	31
#define	DOM_COUNT	(LAST_DOM - FIRST_DOM + 1)

#define	FIRST_MONTH	1
#define	LAST_MONTH	12
#define	MONTH_COUNT	(LAST_MONTH - FIRST_MONTH + 1)

/* note on DOW: 0 and 7 are both Sunday, for compatibility reasons. */
#define	FIRST_DOW	0
#define	LAST_DOW	7
#define	DOW_COUNT	(LAST_DOW - FIRST_DOW + 1)

/*
 * Because crontab/at files may be owned by their respective users we
 * take extreme care in opening them.  If the OS lacks the O_NOFOLLOW
 * we will just have to live without it.  In order for this to be an
 * issue an attacker would have to subvert group CRON_GROUP.
 */
#ifndef O_NOFOLLOW
#define O_NOFOLLOW	0
#endif
