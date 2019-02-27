/*	$NetBSD: syntax.h,v 1.12 2019/02/27 04:10:56 kre Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
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
 */

#include <sys/cdefs.h>
#include <ctype.h>

/* Syntax classes */
#define CWORD 0			/* character is nothing special */
#define CNL 1			/* newline character */
#define CBACK 2			/* a backslash character */
#define CSQUOTE 3		/* single quote */
#define CDQUOTE 4		/* double quote */
#define CBQUOTE 5		/* backwards single quote */
#define CVAR 6			/* a dollar sign */
#define CENDVAR 7		/* a '}' character */
#define CLP 8			/* a left paren in arithmetic */
#define CRP 9			/* a right paren in arithmetic */
#define CEOF 10			/* end of file */
#define CSPCL 11		/* these terminate a word */
#define CCTL 12			/* like CWORD, except it must be escaped */
#define CSBACK 13		/* a backslash in a single quote syntax */
#define CFAKE 14		/* a delimiter that does not exist */
	/*
	 * note CSBACK == (CCTL|1)
	 * the code does not rely upon that, but keeping it allows a
	 * smart enough compiler to optimise some tests
	 */

/* Syntax classes for is_ functions */
#define ISDIGIT 01		/* a digit */
#define ISUPPER 02		/* an upper case letter */
#define ISLOWER 04		/* a lower case letter */
#define ISUNDER 010		/* an underscore */
#define ISSPECL 020		/* the name of a special parameter */
#define ISSPACE 040		/* a white space character */

#define PEOF	(CHAR_MIN - 1)
#define PFAKE	(CHAR_MIN - 2)
#define SYNBASE	(-PFAKE)


#define BASESYNTAX (basesyntax + SYNBASE)
#define DQSYNTAX (dqsyntax + SYNBASE)
#define SQSYNTAX (sqsyntax + SYNBASE)
#define ARISYNTAX (arisyntax + SYNBASE)

/* These defines assume that the digits are contiguous (which is guaranteed) */
#define	is_digit(c)	((unsigned)((c) - '0') <= 9)
#define	sh_ctype(c)	(is_type+SYNBASE)[(int)(c)]
#define	is_upper(c)	(sh_ctype(c) & ISUPPER)
#define	is_lower(c)	(sh_ctype(c) & ISLOWER)
#define	is_alpha(c)	(sh_ctype(c) & (ISUPPER|ISLOWER))
#define	is_name(c)	(sh_ctype(c) & (ISUPPER|ISLOWER|ISUNDER))
#define	is_in_name(c)	(sh_ctype(c) & (ISUPPER|ISLOWER|ISUNDER|ISDIGIT))
#define	is_special(c)	(sh_ctype(c) & (ISSPECL|ISDIGIT))
#define	is_space(c)	(sh_ctype(c) & ISSPACE)
#define	digit_val(c)	((c) - '0')

/* true if the arg char needs CTLESC to protect it */
#define	NEEDESC(c)	(SQSYNTAX[(int)(c)] == CCTL || \
			 SQSYNTAX[(int)(c)] == CSBACK)

#define	ISCTL(c)	((c) >= CTL_FIRST && (c) <= CTL_LAST)
#if 0				/* alternative form (generally slower) */
#define	ICCTL(c)	(BASESYNTAX[(int)(c)] == CCTL)
#endif

extern const char basesyntax[];
extern const char dqsyntax[];
extern const char sqsyntax[];
extern const char arisyntax[];
extern const char is_type[];
