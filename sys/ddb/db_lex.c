/*	$NetBSD: db_lex.c,v 1.21.6.1 2011/06/06 09:07:37 jruoho Exp $	*/

/*
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 *
 *	Author: David B. Golub, Carnegie Mellon University
 *	Date:	7/90
 */

/*
 * Lexical analyzer.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_lex.c,v 1.21.6.1 2011/06/06 09:07:37 jruoho Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <ddb/ddb.h>

db_expr_t	db_tok_number;
char		db_tok_string[TOK_STRING_SIZE];

static char	db_line[DB_LINE_MAXLEN];
static const char *db_lp;
static const char *db_endlp;

static int	db_look_char = 0;
static int	db_look_token = 0;

static void	db_flush_line(void);
static int	db_read_char(void);
static void	db_unread_char(int);
static int	db_lex(void);

int
db_read_line(void)
{
	int	i;

	i = db_readline(db_line, sizeof(db_line));
	if (i == 0)
		return (0);	/* EOI */
	db_set_line(db_line, db_line + i);
	return (i);
}

void
db_set_line(const char *sp, const char *ep)
{

	db_lp = sp;
	db_endlp = ep;
}

static void
db_flush_line(void)
{

	db_lp = db_line;
	db_endlp = db_line;
}

static int
db_read_char(void)
{
	int	c;

	if (db_look_char != 0) {
		c = db_look_char;
		db_look_char = 0;
	}
	else if (db_lp >= db_endlp)
		c = -1;
	else
		c = *db_lp++;
	return (c);
}

static void
db_unread_char(int c)
{

	db_look_char = c;
}

void
db_unread_token(int t)
{

	db_look_token = t;
}

int
db_read_token(void)
{
	int	t;

	if (db_look_token) {
		t = db_look_token;
		db_look_token = 0;
	}
	else
		t = db_lex();
	return (t);
}

int	db_radix = 16;

/*
 * Convert the number to a string in the current radix.
 * This replaces the non-standard %n printf() format.
 */

char *
db_num_to_str(db_expr_t val)
{

	/*
	 * 2 chars for "0x", 1 for a sign ("-")
	 * up to 21 chars for a 64-bit number:
	 *   % echo 2^64 | bc | wc -c
	 *   21
	 * and 1 char for a terminal NUL
	 * 2+1+21+1 => 25
	 */
	static char buf[25];

	if (db_radix == 16)
		snprintf(buf, sizeof(buf), "%" DDB_EXPR_FMT "x", val);
	else if (db_radix == 8)
		snprintf(buf, sizeof(buf), "%" DDB_EXPR_FMT "o", val);
	else
		snprintf(buf, sizeof(buf), "%" DDB_EXPR_FMT "u", val);

	return (buf);
}

void
db_flush_lex(void)
{

	db_flush_line();
	db_look_char = 0;
	db_look_token = 0;
}

static int
db_lex(void)
{
	int	c;

	c = db_read_char();
	while (c <= ' ' || c > '~') {
		if (c == '\n' || c == -1)
			return (tEOL);
		c = db_read_char();
	}

	if (c >= '0' && c <= '9') {
		/* number */
		db_expr_t	r, digit = 0;

		if (c > '0')
			r = db_radix;
		else {
			c = db_read_char();
			if (c == 'O' || c == 'o')
				r = 8;
			else if (c == 'T' || c == 't')
				r = 10;
			else if (c == 'X' || c == 'x')
				r = 16;
			else {
				r = db_radix;
				db_unread_char(c);
			}
			c = db_read_char();
		}
		db_tok_number = 0;
		for (;;) {
			if (c >= '0' && c <= ((r == 8) ? '7' : '9'))
				digit = c - '0';
			else if (r == 16 && ((c >= 'A' && c <= 'F') ||
				(c >= 'a' && c <= 'f'))) {
				if (c >= 'a')
					digit = c - 'a' + 10;
				else if (c >= 'A')
					digit = c - 'A' + 10;
			}
			else
				break;
			db_tok_number = db_tok_number * r + digit;
			c = db_read_char();
		}
		if ((c >= '0' && c <= '9') ||
		    (c >= 'A' && c <= 'Z') ||
		    (c >= 'a' && c <= 'z') ||
		    (c == '_')) {
			db_error("Bad character in number\n");
			/*NOTREACHED*/
		}
		db_unread_char(c);
		return (tNUMBER);
	}
	if ((c >= 'A' && c <= 'Z') ||
	    (c >= 'a' && c <= 'z') ||
	    c == '_' || c == '\\') {
		/* string */
		char *cp;

		cp = db_tok_string;
		if (c == '\\') {
			c = db_read_char();
			if (c == '\n' || c == -1) {
				db_error("Bad escape\n");
				/*NOTREACHED*/
			}
		}
		*cp++ = c;
		while (1) {
			c = db_read_char();
			if ((c >= 'A' && c <= 'Z') ||
			    (c >= 'a' && c <= 'z') ||
			    (c >= '0' && c <= '9') ||
			    c == '_' || c == '\\' || c == ':') {
				if (c == '\\') {
					c = db_read_char();
					if (c == '\n' || c == -1) {
						db_error("Bad escape\n");
						/*NOTREACHED*/
					}
				}
				*cp++ = c;
				if (cp == db_tok_string+sizeof(db_tok_string)) {
					db_error("String too long\n");
					/*NOTREACHED*/
				}
				continue;
			} else {
				*cp = '\0';
				break;
			}
		}
		db_unread_char(c);
		return (tIDENT);
	}

	switch (c) {
	case '+':
		return (tPLUS);
	case '-':
		return (tMINUS);
	case '.':
		c = db_read_char();
		if (c == '.')
			return (tDOTDOT);
		db_unread_char(c);
		return (tDOT);
	case '*':
		return (tSTAR);
	case '/':
		return (tSLASH);
	case '=':
		return (tEQ);
	case '%':
		return (tPCT);
	case '#':
		return (tHASH);
	case '(':
		return (tLPAREN);
	case ')':
		return (tRPAREN);
	case ',':
		return (tCOMMA);
	case '"':
		return (tDITTO);
	case '$':
		return (tDOLLAR);
	case '!':
		return (tEXCL);
	case '<':
		c = db_read_char();
		if (c == '<')
			return (tSHIFT_L);
		db_unread_char(c);
		break;
	case '>':
		c = db_read_char();
		if (c == '>')
			return (tSHIFT_R);
		db_unread_char(c);
		break;
	case -1:
		return (tEOF);
	}
	db_printf("Bad character\n");
	db_flush_lex();
	return (tEOF);
}

/*
 * Utility routine - discard tokens through end-of-line.
 */
void
db_skip_to_eol(void)
{
	int t;

	do {
		t = db_read_token();
	} while (t != tEOL);
}

void
db_error(const char *s)
{

	if (s)
		db_printf("%s", s);
	db_flush_lex();
	longjmp(db_recover);
}
