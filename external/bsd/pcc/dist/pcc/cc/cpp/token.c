/*	Id: token.c,v 1.116 2014/06/06 15:32:53 plunky Exp 	*/	
/*	$NetBSD: token.c,v 1.1.1.5 2014/07/24 19:22:42 plunky Exp $	*/

/*
 * Copyright (c) 2004,2009 Anders Magnusson. All rights reserved.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Tokenizer for the C preprocessor.
 * There are three main routines:
 *	- fastscan() loops over the input stream searching for magic
 *		characters that may require actions.
 *	- sloscan() tokenize the input stream and returns tokens.
 *	- yylex() returns something from the input stream that
 *		is suitable for yacc.
 *
 *	Other functions of common use:
 *	- inpch() returns a raw character from the current input stream.
 *	- inch() is like inpch but \\n and trigraphs are expanded.
 *	- unch() pushes back a character to the input stream.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>

#include "compat.h"
#include "cpp.h"
#include "cpy.h"

static void cvtdig(int);
static int dig2num(int);
static int charcon(usch *);
static void elsestmt(void);
static void ifdefstmt(void);
static void ifndefstmt(void);
static void endifstmt(void);
static void ifstmt(void);
static void cpperror(void);
static void cppwarning(void);
static void undefstmt(void);
static void pragmastmt(void);
static void elifstmt(void);

#define	PUTCH(ch) if (!flslvl) putch(ch)
/* protection against recursion in #include */
#define MAX_INCLEVEL	100
static int inclevel;

usch yytext[CPPBUF];

struct includ *ifiles;

/* some common special combos for init */
#define C_NL	(C_SPEC|C_WSNL)
#define C_DX	(C_SPEC|C_ID|C_DIGIT|C_HEX)
#define C_I	(C_SPEC|C_ID|C_ID0)
#define C_IP	(C_SPEC|C_ID|C_ID0|C_EP)
#define C_IX	(C_SPEC|C_ID|C_ID0|C_HEX)
#define C_IXE	(C_SPEC|C_ID|C_ID0|C_HEX|C_EP)

usch spechr[256] = {
	0,	0,	0,	0,	C_SPEC,	C_SPEC,	0,	0,
	0,	C_WSNL,	C_NL,	0,	0,	C_WSNL,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0,

	C_WSNL,	C_2,	C_SPEC,	0,	0,	0,	C_2,	C_SPEC,
	0,	0,	0,	C_2,	0,	C_2,	0,	C_SPEC|C_2,
	C_DX,	C_DX,	C_DX,	C_DX,	C_DX,	C_DX,	C_DX,	C_DX,
	C_DX,	C_DX,	0,	0,	C_2,	C_2,	C_2,	0,

	0,	C_IX,	C_IX,	C_IX,	C_IX,	C_IXE,	C_IX,	C_I,
	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,
	C_IP,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,
	C_I,	C_I,	C_I,	0,	C_SPEC,	0,	0,	C_I,

	0,	C_IX,	C_IX,	C_IX,	C_IX,	C_IXE,	C_IX,	C_I,
	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,
	C_IP,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,
	C_I,	C_I,	C_I,	0,	C_2,	0,	0,	0,

/* utf-8 */
	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,
	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,
	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,
	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,

	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,
	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,
	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,
	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,

	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,
	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,
	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,
	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,

	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,
	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,
	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,
	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,
};

/*
 * fill up the input buffer
 */
static int
inpbuf(void)
{
	int len;

	if (ifiles->infil == -1)
		return 0;
	len = read(ifiles->infil, ifiles->buffer, CPPBUF);
	if (len == -1)
		error("read error on file %s", ifiles->orgfn);
	if (len > 0) {
		ifiles->buffer[len] = 0;
		ifiles->curptr = ifiles->buffer;
		ifiles->maxread = ifiles->buffer + len;
	}
	return len;
}

/*
 * return a raw character from the input stream
 */
static inline int
inpch(void)
{

	do {
		if (ifiles->curptr < ifiles->maxread)
			return *ifiles->curptr++;
	} while (inpbuf() > 0);

	return -1;
}

/*
 * push a character back to the input stream
 */
static void
unch(int c)
{
	if (c == -1)
		return;

	ifiles->curptr--;
	if (ifiles->curptr < ifiles->bbuf)
		error("pushback buffer full");
	*ifiles->curptr = (usch)c;
}

/*
 * Check for (and convert) trigraphs.
 */
static int
chktg(void)
{
	int ch;

	if ((ch = inpch()) != '?') {
		unch(ch);
		return 0;
	}

	switch (ch = inpch()) {
	case '=':  return '#';
	case '(':  return '[';
	case ')':  return ']';
	case '<':  return '{';
	case '>':  return '}';
	case '/':  return '\\';
	case '\'': return '^';
	case '!':  return '|';
	case '-':  return '~';
	}

	unch(ch);
	unch('?');
	return 0;
}

/*
 * check for (and eat) end-of-line
 */
static int
chkeol(void)
{
	int ch;

	ch = inpch();
	if (ch == '\r') {
		ch = inpch();
		if (ch == '\n')
			return '\n';

		unch(ch);
		unch('\r');
		return 0;
	}
	if (ch == '\n')
		return '\n';

	unch(ch);
	return 0;
}

/*
 * return next char, after converting trigraphs and
 * skipping escaped line endings
 */
static inline int
inch(void)
{
	int ch;

	for (;;) {
		ch = inpch();
		if (ch == '?' && (ch = chktg()) == 0)
			return '?';
		if (ch != '\\' || chkeol() == 0)
			return ch;
		ifiles->escln++;
	}
}

/*
 * check for universal-character-name on input, and
 * unput to the pushback buffer encoded as UTF-8.
 */
static int
chkucn(void)
{
	unsigned long cp, m;
	int ch, n;

	if ((ch = inch()) == -1)
		return 0;
	if (ch == 'u')
		n = 4;
	else if (ch == 'U')
		n = 8;
	else {
		unch(ch);
		return 0;
	}

	cp = 0;
	while (n-- > 0) {
		if ((ch = inch()) == -1 || (spechr[ch] & C_HEX) == 0) {
			warning("invalid universal character name");
			// XXX should actually unput the chars and return 0
			unch(ch); // XXX eof
			break;
		}
		cp = cp * 16 + dig2num(ch);
	}

	if ((cp < 0xa0 && cp != 0x24 && cp != 0x40 && cp != 0x60)
	    || (cp >= 0xd800 && cp <= 0xdfff))	/* 6.4.3.2 */
		error("universal character name cannot be used");

	if (cp > 0x7fffffff)
		error("universal character name out of range");

	n = 0;
	m = 0x7f;
	while (cp > m) {
		unch(0x80 | (cp & 0x3f));
		cp >>= 6;
		m >>= (n++ ? 1 : 2);
	}
	unch(((m << 1) ^ 0xfe) | cp);
	return 1;
}

static int
eatcmnt(void)
{
	int ch;

	if (Cflag) {
		PUTCH('/'); PUTCH('*');
	}
	for (;;) {
		ch = inch();
		if (ch == '\n') {
			ifiles->lineno++;
			putch('\n');
			continue;
		}
		if (ch == -1)
			return -1;
		if (ch == '*') {
			ch = inch();
			if (ch == '/')
				break;
			unch(ch);
			ch = '*';
		}
		if (Cflag) PUTCH(ch);
	}
	if (Cflag) {
		PUTCH('*'); PUTCH('/');
	} else
		PUTCH(' ');

	return 0;
}

/*
 * Scan quickly the input file searching for:
 *	- '#' directives
 *	- keywords (if not flslvl)
 *	- comments
 *
 *	Handle strings, numbers and trigraphs with care.
 *	Only data from pp files are scanned here, never any rescans.
 *	TODO: Only print out strings before calling other functions.
 */
static void
fastscan(void)
{
	struct symtab *nl;
	int ch, i;
	usch *cp;

	goto run;
	for (;;) {
		ch = inch();
xloop:		if (ch == -1)
			return;
#ifdef PCC_DEBUG
		if (dflag>1)
			printf("fastscan ch %d (%c)\n", ch, ch > 31 ? ch : '@');
#endif
		if ((spechr[ch] & C_SPEC) == 0) {
			PUTCH(ch);
			continue;
		}
		switch (ch) {
		case EBLOCK:
		case WARN:
		case CONC:
			error("bad char passed");
			break;

		case '/': /* Comments */
			if ((ch = inch()) == '/') {
cppcmt:				if (Cflag) { PUTCH(ch); } else { PUTCH(' '); }
				do {
					if (Cflag) PUTCH(ch);
					ch = inch();
					if (ch == -1)
						goto eof;
				} while (ch != '\n');
				goto xloop;
			} else if (ch == '*') {
				if (eatcmnt() == -1)
					goto eof;
			} else {
				PUTCH('/');
				goto xloop;
			}
			break;

		case '\n': /* newlines, for pp directives */
			i = ifiles->escln + 1;
			ifiles->lineno += i;
			ifiles->escln = 0;
			while (i-- > 0)
				putch('\n');
run:			for(;;) {
				ch = inch();
				if (ch == '/') {
					ch = inch();
					if (ch == '/')
						goto cppcmt;
					if (ch == '*') {
						if (eatcmnt() == -1)
							goto eof;
						continue;
					}
					unch(ch);
					ch = '/';
				}
				if (ch != ' ' && ch != '\t')
					break;
				PUTCH(ch);
			}
			if (ch == '#') {
				ppdir();
				continue;
			} else if (ch == '%') {
				ch = inch();
				if (ch == ':') {
					ppdir();
					continue;
				}
				unch(ch);
				ch = '%';
			}
			goto xloop;

		case '\"': /* strings */
str:			PUTCH(ch);
			while ((ch = inch()) != '\"') {
				if (ch == '\\') {
					if (chkucn())
						continue;
					PUTCH('\\');
					ch = inch();
				}
				if (ch == '\n') {
					warning("unterminated string literal");
					goto xloop;
				}
				if (ch == -1)
					goto eof;
				PUTCH(ch);
			}
			PUTCH(ch);
			break;

		case '.':  /* for pp-number */
			PUTCH(ch);
			ch = inch();
			if (ch < '0' || ch > '9')
				goto xloop;

			/* FALLTHROUGH */
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			do {
nxp:				PUTCH(ch);
				ch = inch();
				if (ch == -1)
					goto eof;
				if ((spechr[ch] & C_EP)) {
					PUTCH(ch);
					ch = inch();
					if (ch == '-' || ch == '+')
						goto nxp;
					if (ch == -1)
						goto eof;
				}
			} while ((spechr[ch] & C_ID) || (ch == '.'));
			goto xloop;

		case '\'': /* character constant */
con:			PUTCH(ch);
			if (tflag)
				break; /* character constants ignored */
			while ((ch = inch()) != '\'') {
				if (ch == '\\') {
					if (chkucn())
						continue;
					PUTCH('\\');
					ch = inch();
				}
				if (ch == '\n') {
					warning("unterminated character constant");
					goto xloop;
				}
				if (ch == -1)
					goto eof;
				PUTCH(ch);
			}
			PUTCH(ch);
			break;

		case 'L':
			ch = inch();
			if (ch == '\"') {
				PUTCH('L');
				goto str;
			}
			if (ch == '\'') {
				PUTCH('L');
				goto con;
			}
			unch(ch);
			ch = 'L';

			/* FALLTHROUGH */
		default:
#ifdef PCC_DEBUG
			if ((spechr[ch] & C_ID) == 0)
				error("fastscan");
#endif
		ident:
			i = 0;
			yytext[i++] = (usch)ch;
			for (;;) {
				if ((ch = inch()) == -1)
					break;
				if (ch == '\\') {
					if (chkucn())
						continue;
					unch(ch);
					break;
				}
				if ((spechr[ch] & C_ID) == 0) {
					unch(ch);
					break;
				}
				yytext[i++] = (usch)ch;
			}
			yytext[i] = 0;

			if (flslvl == 0) {
				cp = stringbuf;
				if ((nl = lookup(yytext, FIND)) && kfind(nl))
					putstr(stringbuf);
				else
					putstr(yytext);
				stringbuf = cp;
			}
			if (ch == -1)
				goto eof;

			break;

		case '\\':
			if (chkucn()) {
				ch = inch();
				goto ident;
			}

			PUTCH('\\');
			break;
		}
	}

eof:	warning("unexpected EOF");
	putch('\n');
}

int
sloscan(void)
{
	int ch;
	int yyp;

zagain:
	yyp = 0;
	ch = inch();
	yytext[yyp++] = (usch)ch;
	switch (ch) {
	case -1: /* EOF */
		return 0;

	case '\n': /* do not pass NL */
		unch(ch);
		yytext[yyp] = 0;
		return ch;

	case '\r': /* Ignore CR */
		goto zagain;

	case '0': case '1': case '2': case '3': case '4': case '5':
	case '6': case '7': case '8': case '9':
		/* reading a "pp-number" */
ppnum:		for (;;) {
			ch = inch();
			if (ch == -1)
				break;
			if ((spechr[ch] & C_EP)) {
				yytext[yyp++] = (usch)ch;
				ch = inch();
				if (ch == '-' || ch == '+') {
					yytext[yyp++] = (usch)ch;
				} else
					unch(ch);
				continue;
			}
			if ((spechr[ch] & C_ID) || ch == '.') {
				yytext[yyp++] = (usch)ch;
				continue;
			}
			break;
		}
		unch(ch);
		yytext[yyp] = 0;
		return NUMBER;

	case '\'':
chlit:
		for (;;) {
			if ((ch = inch()) == '\\') {
				if (chkucn())
					continue;
				yytext[yyp++] = (usch)ch;
				ch = inch();
			} else if (ch == '\'')
				break;
			if (ch == -1 || ch == '\n') {
				/* not a constant */
				while (yyp > 1)
					unch(yytext[--yyp]);
				goto any;
			}
			yytext[yyp++] = (usch)ch;
		}
		yytext[yyp++] = (usch)'\'';
		yytext[yyp] = 0;
		return NUMBER;

	case ' ':
	case '\t':
		while ((ch = inch()) == ' ' || ch == '\t')
			yytext[yyp++] = (usch)ch;
		unch(ch);
		yytext[yyp] = 0;
		return WSPACE;

	case '/':
		if ((ch = inch()) == '/') {	/* C++ comment */
			do {
				yytext[yyp++] = (usch)ch;
				ch = inch();
			} while (ch != -1 && ch != '\n');
			yytext[yyp] = 0;
			unch(ch);
			goto zagain;
		} else if (ch == '*') {		/* C comment */
			int c, wrn;
			extern int readmac;

			if (Cflag && !flslvl && readmac) {
				unch(ch);
				yytext[yyp] = 0;
				return CMNT;
			}

			wrn = 0;
		more:	while ((c = inch()) != '*') {
				if (c == -1)
					return 0;
				if (c == '\n')
					putch(c), ifiles->lineno++;
				else if (c == EBLOCK) {
					(void)inch();
					(void)inch();
				} else if (c == WARN)
					wrn = 1;
			}
			if ((c = inch()) == -1)
				return 0;
			if (c != '/') {
				unch(c);
				goto more;
			}
			if (!tflag && !Cflag && !flslvl)
				unch(' ');
			if (wrn)
				unch(WARN);
			goto zagain;
		}
		unch(ch);
		goto any;

	case '.':
		if ((ch = inch()) == -1)
			goto any;
		if ((spechr[ch] & C_DIGIT)) {
			yytext[yyp++] = (usch)ch;
			goto ppnum;
		}
		unch(ch);
		goto any;

	case '\"':
		if (tflag && defining)
			goto any;
	strng:
		for (;;) {
			if ((ch = inch()) == '\\') {
				if (chkucn())
					continue;
				yytext[yyp++] = (usch)ch;
				ch = inch();
			} else if (ch == '\"') {
				yytext[yyp++] = (usch)ch;
				break;
			}
			if (ch == -1 || ch == '\n') {
				warning("unterminated string");
				unch(ch);
				break;	// XXX the STRING does not have a closing quote
			}
			yytext[yyp++] = (usch)ch;
		}
		yytext[yyp] = 0;
		return STRING;

	case '\\':
		if (chkucn()) {
			--yyp;
			goto ident;
		}
		goto any;

	case 'L':
		if ((ch = inch()) == '\"' && !tflag) {
			yytext[yyp++] = (usch)ch;
			goto strng;
		} else if (ch == '\'' && !tflag) {
			yytext[yyp++] = (usch)ch;
			goto chlit;
		}
		unch(ch);
		/* FALLTHROUGH */

	/* Yetch, all identifiers */
	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
	case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
	case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
	case 's': case 't': case 'u': case 'v': case 'w': case 'x':
	case 'y': case 'z':
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
	case 'G': case 'H': case 'I': case 'J': case 'K':
	case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
	case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
	case 'Y': case 'Z':
	case '_': /* {L}({L}|{D})* */

	ident:
		for (;;) { /* get chars */
			if ((ch = inch()) == -1)
				break;
			if (ch == '\\') {
				if (chkucn())
					continue;
				unch('\\');
				break;
			}
			if ((spechr[ch] & C_ID) == 0) {
				unch(ch);
				break;
			}
			yytext[yyp++] = (usch)ch;
		}
		yytext[yyp] = 0; /* need already string */
		return IDENT;

	default:
		if ((ch & 0x80))
			goto ident;

	any:
		yytext[yyp] = 0;
		return yytext[0];
	} /* endcase */

	/* NOTREACHED */
}

int
yylex(void)
{
	static int ifdef, noex;
	struct symtab *nl;
	int ch, c2;

	while ((ch = sloscan()) == WSPACE)
		;
	if (ch < 128 && (spechr[ch] & C_2))
		c2 = inch();
	else
		c2 = 0;

	switch (ch) {
	case '=':
		if (c2 == '=') return EQ;
		break;
	case '!':
		if (c2 == '=') return NE;
		break;
	case '|':
		if (c2 == '|') return OROR;
		break;
	case '&':
		if (c2 == '&') return ANDAND;
		break;
	case '<':
		if (c2 == '<') return LS;
		if (c2 == '=') return LE;
		break;
	case '>':
		if (c2 == '>') return RS;
		if (c2 == '=') return GE;
		break;
	case '+':
	case '-':
		if (ch == c2)
			error("invalid preprocessor operator %c%c", ch, c2);
		break;

	case '/':
		if (Cflag == 0 || c2 != '*')
			break;
		/* Found comment that need to be skipped */
		for (;;) {
			ch = inch();
		c1:	if (ch != '*')
				continue;
			if ((ch = inch()) == '/')
				break;
			goto c1;
		}
		return yylex();

	case NUMBER:
		if (yytext[0] == '\'') {
			yylval.node.op = NUMBER;
			yylval.node.nd_val = charcon(yytext);
		} else
			cvtdig(yytext[0] != '0' ? 10 :
			    yytext[1] == 'x' || yytext[1] == 'X' ? 16 : 8);
		return NUMBER;

	case IDENT:
		if (strcmp((char *)yytext, "defined") == 0) {
			ifdef = 1;
			return DEFINED;
		}
		nl = lookup(yytext, FIND);
		if (ifdef) {
			yylval.node.nd_val = nl != NULL;
			ifdef = 0;
		} else if (nl && noex == 0) {
			usch *och = stringbuf;
			int i;

			i = kfind(nl);
			unch(WARN);
			if (i)
				unpstr(stringbuf);
			else
				unpstr(nl->namep);
			stringbuf = och;
			noex = 1;
			return yylex();
		} else {
			yylval.node.nd_val = 0;
		}
		yylval.node.op = NUMBER;
		return NUMBER;
	case WARN:
		noex = 0;
		/* FALLTHROUGH */
	case PHOLD:
		return yylex();
	default:
		return ch;
	}
	unch(c2);
	return ch;
}

/*
 * Let the command-line args be faked defines at beginning of file.
 */
static void
prinit(struct initar *it, struct includ *ic)
{
	const char *pre, *post;
	char *a;

	if (it->next)
		prinit(it->next, ic);
	pre = post = NULL; /* XXX gcc */
	switch (it->type) {
	case 'D':
		pre = "#define ";
		if ((a = strchr(it->str, '=')) != NULL) {
			*a = ' ';
			post = "\n";
		} else
			post = " 1\n";
		break;
	case 'U':
		pre = "#undef ";
		post = "\n";
		break;
	case 'i':
		pre = "#include \"";
		post = "\"\n";
		break;
	default:
		error("prinit");
	}
	strlcat((char *)ic->buffer, pre, CPPBUF+1);
	strlcat((char *)ic->buffer, it->str, CPPBUF+1);
	if (strlcat((char *)ic->buffer, post, CPPBUF+1) >= CPPBUF+1)
		error("line exceeds buffer size");

	ic->lineno--;
	while (*ic->maxread)
		ic->maxread++;
}

/*
 * A new file included.
 * If ifiles == NULL, this is the first file and already opened (stdin).
 * Return 0 on success, -1 if file to be included is not found.
 */
int
pushfile(const usch *file, const usch *fn, int idx, void *incs)
{
	extern struct initar *initar;
	struct includ ibuf;
	struct includ *ic;
	int otrulvl;

	ic = &ibuf;
	ic->next = ifiles;

	if (file != NULL) {
		if ((ic->infil = open((const char *)file, O_RDONLY)) < 0)
			return -1;
		ic->orgfn = ic->fname = file;
		if (++inclevel > MAX_INCLEVEL)
			error("limit for nested includes exceeded");
	} else {
		ic->infil = 0;
		ic->orgfn = ic->fname = (const usch *)"<stdin>";
	}
#ifndef BUF_STACK
	ic->bbuf = malloc(BBUFSZ);
#endif
	ic->buffer = ic->bbuf+NAMEMAX;
	ic->curptr = ic->buffer;
	ifiles = ic;
	ic->lineno = 1;
	ic->escln = 0;
	ic->maxread = ic->curptr;
	ic->idx = idx;
	ic->incs = incs;
	ic->fn = fn;
	prtline();
	if (initar) {
		int oin = ic->infil;
		ic->infil = -1;
		*ic->maxread = 0;
		prinit(initar, ic);
		initar = NULL;
		if (dMflag)
			xwrite(ofd, ic->buffer, strlen((char *)ic->buffer));
		fastscan();
		prtline();
		ic->infil = oin;
	}

	otrulvl = trulvl;

	fastscan();

	if (otrulvl != trulvl || flslvl)
		error("unterminated conditional");

#ifndef BUF_STACK
	free(ic->bbuf);
#endif
	ifiles = ic->next;
	close(ic->infil);
	inclevel--;
	return 0;
}

/*
 * Print current position to output file.
 */
void
prtline(void)
{
	usch *sb = stringbuf;

	if (Mflag) {
		if (dMflag)
			return; /* no output */
		if (ifiles->lineno == 1) {
			sheap("%s: %s\n", Mfile, ifiles->fname);
			if (MPflag &&
			    strcmp((const char *)ifiles->fname, (char *)MPfile))
				sheap("%s:\n", ifiles->fname);
			xwrite(ofd, sb, stringbuf - sb);
		}
	} else if (!Pflag) {
		sheap("\n# %d \"%s\"", ifiles->lineno, ifiles->fname);
		if (ifiles->idx == SYSINC)
			sheap(" 3");
		sheap("\n");
		putstr(sb);
	}
	stringbuf = sb;
}

void
cunput(int c)
{
#ifdef PCC_DEBUG
//	if (dflag)printf(": '%c'(%d)\n", c > 31 ? c : ' ', c);
#endif
	unch(c);
}

static int
dig2num(int c)
{
	if (c >= 'a')
		c = c - 'a' + 10;
	else if (c >= 'A')
		c = c - 'A' + 10;
	else
		c = c - '0';
	return c;
}

/*
 * Convert string numbers to unsigned long long and check overflow.
 */
static void
cvtdig(int rad)
{
	unsigned long long rv = 0;
	unsigned long long rv2 = 0;
	usch *y = yytext;
	int c;

	c = *y++;
	if (rad == 16)
		y++;
	while ((spechr[c] & C_HEX)) {
		rv = rv * rad + dig2num(c);
		/* check overflow */
		if (rv / rad < rv2)
			error("constant \"%s\" is out of range", yytext);
		rv2 = rv;
		c = *y++;
	}
	y--;
	while (*y == 'l' || *y == 'L')
		y++;
	yylval.node.op = *y == 'u' || *y == 'U' ? UNUMBER : NUMBER;
	yylval.node.nd_uval = rv;
	if ((rad == 8 || rad == 16) && yylval.node.nd_val < 0)
		yylval.node.op = UNUMBER;
	if (yylval.node.op == NUMBER && yylval.node.nd_val < 0)
		/* too large for signed, see 6.4.4.1 */
		error("constant \"%s\" is out of range", yytext);
}

static int
charcon(usch *p)
{
	int val, c;

	p++; /* skip first ' */
	val = 0;
	if (*p++ == '\\') {
		switch (*p++) {
		case 'a': val = '\a'; break;
		case 'b': val = '\b'; break;
		case 'f': val = '\f'; break;
		case 'n': val = '\n'; break;
		case 'r': val = '\r'; break;
		case 't': val = '\t'; break;
		case 'v': val = '\v'; break;
		case '\"': val = '\"'; break;
		case '\'': val = '\''; break;
		case '\\': val = '\\'; break;
		case 'x':
			while ((spechr[c = *p] & C_HEX)) {
				val = val * 16 + dig2num(c);
				p++;
			}
			break;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7':
			p--;
			while ((spechr[c = *p] & C_DIGIT)) {
				val = val * 8 + (c - '0');
				p++;
			}
			break;
		default: val = p[-1];
		}

	} else
		val = p[-1];
	return val;
}

static void
chknl(int ignore)
{
	int t;

	while ((t = sloscan()) == WSPACE)
		;
	if (t != '\n') {
		if (t) {
			if (ignore) {
				warning("newline expected, got \"%s\"", yytext);
				/* ignore rest of line */
				while ((t = sloscan()) && t != '\n')
					;
			}
			else
				error("newline expected, got \"%s\"", yytext);
		} else {
			if (ignore)
				warning("no newline at end of file");
			else
				error("no newline at end of file");
		}
	}
}

static void
elsestmt(void)
{
	if (flslvl) {
		if (elflvl > trulvl)
			;
		else if (--flslvl!=0)
			flslvl++;
		else
			trulvl++;
	} else if (trulvl) {
		flslvl++;
		trulvl--;
	} else
		error("#else in non-conditional section");
	if (elslvl==trulvl+flslvl)
		error("too many #else");
	elslvl=trulvl+flslvl;
	chknl(1);
}

static void
ifdefstmt(void)
{
	int t;

	if (flslvl) {
		flslvl++;
		while ((t = sloscan()) && t != '\n')
			;
		return;
	}
	while ((t = sloscan()) == WSPACE)
		;
	if (t != IDENT)
		error("bad #ifdef");
	if (lookup(yytext, FIND) == NULL)
		flslvl++;
	else
		trulvl++;
	chknl(0);
}

static void
ifndefstmt(void)
{
	int t;

	if (flslvl) {
		flslvl++;
		while ((t = sloscan()) && t != '\n')
			;
		return;
	}
	while ((t = sloscan()) == WSPACE)
		;
	if (t != IDENT)
		error("bad #ifndef");
	if (lookup(yytext, FIND) != NULL)
		flslvl++;
	else
		trulvl++;
	chknl(0);
}

static void
endifstmt(void)
{
	if (flslvl)
		flslvl--;
	else if (trulvl)
		trulvl--;
	else
		error("#endif in non-conditional section");
	if (flslvl == 0)
		elflvl = 0;
	elslvl = 0;
	chknl(1);
}

static void
ifstmt(void)
{
	if (flslvl || yyparse() == 0)
		flslvl++;
	else
		trulvl++;
}

static void
elifstmt(void)
{
	if (flslvl == 0)
		elflvl = trulvl;
	if (flslvl) {
		if (elflvl > trulvl)
			;
		else if (--flslvl!=0)
			flslvl++;
		else if (yyparse())
			trulvl++;
		else
			flslvl++;
	} else if (trulvl) {
		flslvl++;
		trulvl--;
	} else
		error("#elif in non-conditional section");
}

/* save line into stringbuf */
static usch *
savln(void)
{
	int c;
	usch *cp = stringbuf;

	while ((c = inch()) != -1) {
		if (c == '\n') {
			unch(c);
			break;
		}
		savch(c);
	}
	savch(0);

	return cp;
}

static void
cpperror(void)
{
	usch *cp;
	int c;

	if (flslvl)
		return;
	c = sloscan();
	if (c != WSPACE && c != '\n')
		error("bad #error");
	cp = savln();
	error("#error %s", cp);
}

static void
cppwarning(void)
{
	usch *cp;
	int c;

	if (flslvl)
		return;
	c = sloscan();
	if (c != WSPACE && c != '\n')
		error("bad #warning");
	cp = savln();
	warning("#warning %s", cp);
	stringbuf = cp;
}

static void
undefstmt(void)
{
	struct symtab *np;

	if (flslvl)
		return;
	if (sloscan() != WSPACE || sloscan() != IDENT)
		error("bad #undef");
	if ((np = lookup(yytext, FIND)) != NULL)
		np->value = 0;
	chknl(0);
}

static void
identstmt(void)
{
	/* Just ignore */
	if (sloscan() != WSPACE || sloscan() != STRING)
		error("bad #ident directive");
	return;
}

static void
pragmastmt(void)
{
	usch *sb;

	if (flslvl)
		return;
	if (sloscan() != WSPACE)
		error("bad #pragma");
	sb = stringbuf;
	savstr((const usch *)"\n#pragma ");
	savln();
	putstr(sb);
	prtline();
	stringbuf = sb;
}

int
cinput(void)
{
	return inch();
}

static struct {
	const char *name;
	void (*fun)(void);
} ppd[] = {
	{ "ifndef", ifndefstmt },
	{ "ifdef", ifdefstmt },
	{ "if", ifstmt },
	{ "include", include },
	{ "else", elsestmt },
	{ "endif", endifstmt },
	{ "error", cpperror },
	{ "warning", cppwarning },
	{ "define", define },
	{ "undef", undefstmt },
	{ "line", line },
	{ "pragma", pragmastmt },
	{ "elif", elifstmt },
	{ "ident", identstmt },
#ifdef GCC_COMPAT
	{ "include_next", include_next },
#endif
};
#define	NPPD	(int)(sizeof(ppd) / sizeof(ppd[0]))

static void
skpln(void)
{
	int ch;

	/* just ignore the rest of the line */
	while ((ch = inch()) != -1) {
		if (ch == '\n') {
			unch('\n');
			break;
		}
	}
}

/*
 * Handle a preprocessor directive.
 */
void
ppdir(void)
{
	char bp[20];
	int ch, i;

redo:	while ((ch = inch()) == ' ' || ch == '\t')
		;
	if (ch == '/') {
		if ((ch = inch()) == '/') {
			skpln();
			return;
		}
		if (ch == '*') {
			while ((ch = inch()) != -1) {
				if (ch == '*') {
					if ((ch = inch()) == '/')
						goto redo;
					unch(ch);
				} else if (ch == '\n') {
					putch('\n');
					ifiles->lineno++;
				}
			}
		}
	}
	if (ch == '\n') { /* empty directive */
		unch(ch);
		return;
	}
	if (ch < 'a' || ch > 'z')
		goto out; /* something else, ignore */
	i = 0;
	do {
		bp[i++] = (usch)ch;
		if (i == sizeof(bp)-1)
			goto out; /* too long */
		ch = inch();
	} while ((ch >= 'a' && ch <= 'z') || (ch == '_'));
	unch(ch);
	bp[i++] = 0;

	/* got keyword */
	for (i = 0; i < NPPD; i++) {
		if (bp[0] == ppd[i].name[0] && strcmp(bp, ppd[i].name) == 0) {
			(*ppd[i].fun)();
			return;
		}
	}

out:
	if (flslvl == 0 && Aflag == 0)
		error("invalid preprocessor directive");

	unch(ch);
	skpln();
}
