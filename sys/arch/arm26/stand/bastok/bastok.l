/*-
 * Copyright (c) 2000 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

/* bastok -- BBC BASIC tokenizer */

/* Not handled:
 * - Abbreviated keywords
 * - Explicit line numbers
 */

/* In condition COMMENT, we ignore everything except EOL. */
%x COMMENT
/*
 * There are a few keywords (PAGE, LOMEM, HIMEM, PTR, TIME)
 * which tokenise differently as statements and as functions.
 * This is handled by the variable "statement", which is 1
 * at the start of a statement and 0 otherwise.
 */
		#define p1(x) do {				\
			thisline[pos++] = x;			\
			statement = 0; /* by default */		\
		} while (0)
		#define p2(x, y) do {				\
			thisline[pos++] = x;			\
			thisline[pos++] = y;			\
			statement = 0;				\
		} while (0)
		#define lr(l, r) do {				\
			if (statement)				\
				p1(l);				\
			else					\
				p1(r);				\
		} while (0)

		char thisline[256];
		int statement;
		int pos;
		int baslinenum = 10;

		void linedone(void);
		int yylex(void);

%%

		pos = 0;
		statement = 1;

ABS		p1(0x94);
ACS		p1(0x95);
ADVAL		p1(0x96);			/* AD. */
AND		p1(0x80);			/* A. */
APPEND		p2(0xC7, 0x8E);			/* AP. */
ASC		p1(0x97);
ASN		p1(0x98);
ATN		p1(0x99);
AUTO		p2(0xC7, 0x8F);			/* AU. */
BEAT		p2(0xC6, 0x8F);
BEATS		p2(0xC8, 0x9E);			/* BEA. */
BGET		p1(0x9A);			/* B. */
BPUT		p1(0xD5);			/* BP. */
CALL		p1(0xD6);			/* CA. */
CASE		p2(0xC8, 0x8E);
CHAIN		p1(0xD7);			/* CH. */
CHR\$		p1(0xBD);
CIRCLE		p2(0xC8, 0x8F);			/* CI. */
CLEAR		p1(0xD8);			/* CL. */
CLG		p1(0xDA);
CLOSE		p1(0xD9);			/* CLO. */
CLS		p1(0xDB);
COLOR		p1(0xFB);			/* C. */
COLOUR		p1(0xFB);			/* C. */
COS		p1(0x9B);
COUNT		p1(0x9C);			/* COU. */
DATA		{ p1(0xDC); BEGIN(COMMENT); }	/* D. */
DEF		p1(0xDD);
DEG		p1(0x9D);
DELETE		p2(0xC7, 0x90);			/* DEL. */
DIM		p1(0xDE);
DIV		p1(0x81);
DRAW		p1(0xDF);			/* DR. */
EDIT		p2(0xC7, 0x91);			/* ED. */
ELLIPSE		p2(0xC8, 0x9D);			/* ELL. */
ELSE		{ p1(0xCC); statement = 1; }	/* EL. */
END		p1(0xE0);
ENDCASE		p1(0xCB);			/* ENDC. */
ENDIF		p1(0xCD);
ENDPROC		p1(0xE1);			/* E. */
ENDWHILE	p1(0xCE);			/* ENDW. */
EOF		p1(0xC5);
EOR		p1(0x82);
ERL		p1(0x9E);
ERR		p1(0x9F);
ERROR		p1(0x85);			/* ERR. */
EVAL		p1(0xA0);			/* EV. */
EXP		p1(0xA1);
EXT		p1(0xA2);
FALSE		p1(0xA3);			/* FA. */
FILL		p2(0xC8, 0x90);			/* FI. */
FN		p1(0xA4);
FOR		p1(0xE3);			/* F. */
GCOL		p1(0xE6);			/* GC. */
GET		p1(0xA5);
GET\$		p1(0xBE);			/* GE. */
GOSUB		p1(0xE4); /* CONSTA = 1 */	/* GOS. */
GOTO		p1(0xE5); /* CONSTA = 1 */	/* G. */
HELP		p2(0xC7, 0x92);			/* HE. */
HIMEM		lr(0xD3, 0x93);			/* H. */
IF		p1(0xE7);
INKEY		p1(0xA6);
INKEY\$		p1(0xBF);			/* INK. */
INPUT		p1(0xE8);			/* I. */
INSTALL		p2(0xC8, 0x9A);			/* INS. */
INSTR\(		p1(0xA7);			/* INS. */
INT		p1(0xA8);
LEFT\$\(	p1(0xC0);			/* LE. */
LEN		p1(0xA9);
LET		p1(0xE9);
LIBRARY		p2(0xC8, 0x9B);			/* LIB. */
LINE		p1(0x86);
LIST		p2(0xC7, 0x93);			/* L. */
LN		p1(0xAA);
LOAD		p2(0xC7, 0x94);			/* LO. */
LOCAL		p1(0xEA);
LOG		p1(0xAB);
LOMEM		lr(0xD2, 0x92);			/* LOM. */
LVAR		p2(0xC7, 0x95);			/* LV. */
MID\$\(		p1(0xC1);			/* M. */
MOD		p1(0x83);
MODE		p1(0xEB);			/* MO. */
MOUSE		p2(0xC8, 0x97);			/* MOU. */
MOVE		p1(0xEC);
NEW		p2(0xC7, 0x96);
NEXT		p1(0xED);			/* N. */
NOT		p1(0xAC);
OF		p1(0xCA);
OFF		p1(0x87);
OLD		p2(0xC7, 0x97);			/* O. */
ON		p1(0xEE);
OPENIN		p1(0x8E);			/* OP. */
OPENOUT		p1(0xAE);			/* OPENO. */
OPENUP		p1(0xAD);
OR		p1(0x84);
ORIGIN		p2(0xC8, 0x91);			/* OR. */
OSCLI		p1(0xFF);			/* OS. */
OTHERWISE	p1(0x7F);			/* OT. */
OVERLAY		p2(0xC8, 0xA3);			/* OV. */
PAGE		lr(0xD0, 0x90);			/* PA. */
PI		p1(0xAF);
PLOT		p1(0xF0);			/* PL. */
POINT		p2(0xC8, 0x92);
POINT\(		p1(0xB0);			/* PO. */
POS		p1(0xB1);
PRINT		p1(0xF1);			/* P. */
PROC		p1(0xF2);
PTR		lr(0xCF, 0x8F);
QUIT		p2(0xC8, 0x98);			/* Q. */
READ		p1(0xF3);
RECTANGLE	p2(0xC8, 0x93);			/* REC. */
REM		{ p1(0xF4); BEGIN(COMMENT); }
RENUMBER	p2(0xC7, 0x98);			/* REN. */
REPEAT		{ p1(0xF5); statement = 1; }	/* REP. */
REPORT		p1(0xF6);			/* REPO. */
RESTORE		p1(0xF7);			/* RES. */
RETURN		p1(0xF8);			/* R. */
RIGHT\$\(	p1(0xC2);			/* RI. */
RND		p1(0xB3);
RUN		p1(0xF9);
SAVE		p2(0xC7, 0x99);			/* SA. */
SGN		p1(0xB4);
SIN		p1(0xB5);
SOUND		p1(0xD4);			/* SO. */
SPC		p1(0x89);
SQR		p1(0xB6);
STEP		p1(0x88);			/* S. */
STEREO		p2(0xC8, 0xA2);			/* STER. */
STOP		p1(0xFA);
STR\$		p1(0xC3);
STRING\$\(	p1(0xC4);			/* STRI. */
SUM		p2(0xC6, 0x8E);
SWAP		p2(0xC8, 0x94);			/* SW. */
SYS		p2(0xC8, 0x99);
TAB\(		p1(0x8A);
TAN		p1(0xB7);			/* T. */
TEMPO		p2(0xC8, 0x9F);			/* TE. */
THEN		{ p1(0x8C); statement = 1; }	/* TH. */
TIME		lr(0xD1, 0x91);			/* TI. */
TINT		p2(0xC8, 0x9C);
TO		p1(0xB8);
TRACE		p1(0xFC);			/* TR. */
TRUE		p1(0xB9);
TWIN		p2(0xC7, 0x9A);
TWINO		p2(0xC7, 0x9B);			/* TW. */
UNTIL		p1(0xFD);			/* U. */
USR		p1(0xBA);
VAL		p1(0xBB);
VDU		p1(0xEF);			/* V. */
VOICE		p2(0xC8, 0xA1);
VOICES		p2(0xC8, 0xA0);			/* VO. */
VPOS		p1(0xBC);			/* VP. */
WAIT		p2(0xC8, 0x96);			/* WA. */
WHEN		p1(0xC9);
WHILE		p2(0xC8, 0x95);			/* W. */
WIDTH		p1(0xFE);			/* WI. */

		/* Strings aren't tokenised */
\"[^\"]*\"	{ memcpy(thisline + pos, yytext, yyleng); pos += yyleng; }

:		{ p1(':'); BEGIN(INITIAL); }

		/* Whitespace doesn't move us into RIGHT */
" "		thisline[pos++] = ' ';

		/* Other random characters might (e.g. '='). */
<COMMENT,INITIAL>.		p1(*yytext);

<COMMENT,INITIAL>\n		{ linedone(); statement = 1; }

%%

#include <sys/types.h>

#include <err.h>
#include <stdio.h>
#include <unistd.h>

void filedone(void);
void usage(void);

void
linedone()
{

	fputc(0x0D, yyout);
	fputc(baslinenum >> 8, yyout);
	fputc(baslinenum & 0xff, yyout);
	fputc(pos + 4, yyout);
	fwrite(thisline, 1, pos, yyout);
	baslinenum += 10;
	pos = 0;
	BEGIN(INITIAL);
}

void
filedone()
{

	fputs("\x0d\xff", yyout);
}

int
main(int argc, char **argv)
{
	int ch;

	while ((ch = getopt(argc, argv, "")) != -1)
		switch (ch) {
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc > 1)
		usage();

	if (*argv != NULL) {
		yyin = fopen(*argv, "r");
		if (yyin == NULL)
			err(1, "%s", *argv);
	}

	yylex();
	filedone();
	exit(0);
}

void
usage()
{

	fprintf(stderr, "usage: bastok [file]\n");
	exit(1);
}
