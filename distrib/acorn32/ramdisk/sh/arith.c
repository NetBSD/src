#include <sys/cdefs.h>
#ifndef lint
#if 0
static char yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93";
#else
#if defined(__NetBSD__) && defined(__IDSTRING)
__IDSTRING(yyrcsid, "$NetBSD: arith.c,v 1.1 2002/01/25 15:35:49 reinoud Exp $");
#endif /* __NetBSD__ && __IDSTRING */
#endif /* 0 */
#endif /* lint */
#include <stdlib.h>
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define YYLEX yylex()
#define YYEMPTY -1
#define yyclearin (yychar=(YYEMPTY))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING (yyerrflag!=0)
#define YYPREFIX "yy"
#line 2 "/usr/sources/cvs.netbsd.org/src/distrib/arm32/ramdisk/../../../bin/sh/arith.y"
/*	$NetBSD: arith.c,v 1.1 2002/01/25 15:35:49 reinoud Exp $	*/

/*-
 * Copyright (c) 1993
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
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)arith.y	8.3 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: arith.c,v 1.1 2002/01/25 15:35:49 reinoud Exp $");
#endif
#endif /* not lint */

#include <stdlib.h>
#include "expand.h"
#include "shell.h"
#include "error.h"
#include "output.h"
#include "memalloc.h"

const char *arith_buf, *arith_startbuf;

void yyerror __P((const char *));
#ifdef TESTARITH
int main __P((int , char *[]));
int error __P((char *));
#endif

#line 85 "arith.c"
#define ARITH_NUM 257
#define ARITH_LPAREN 258
#define ARITH_RPAREN 259
#define ARITH_OR 260
#define ARITH_AND 261
#define ARITH_BOR 262
#define ARITH_BXOR 263
#define ARITH_BAND 264
#define ARITH_EQ 265
#define ARITH_NE 266
#define ARITH_LT 267
#define ARITH_GT 268
#define ARITH_GE 269
#define ARITH_LE 270
#define ARITH_LSHIFT 271
#define ARITH_RSHIFT 272
#define ARITH_ADD 273
#define ARITH_SUB 274
#define ARITH_MUL 275
#define ARITH_DIV 276
#define ARITH_REM 277
#define ARITH_UNARYMINUS 278
#define ARITH_UNARYPLUS 279
#define ARITH_NOT 280
#define ARITH_BNOT 281
#define YYERRCODE 256
const short yylhs[] = {                                        -1,
    0,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,
};
const short yylen[] = {                                         2,
    1,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    2,    2,    2,    2,    1,
};
const short yydefred[] = {                                      0,
   25,    0,    0,    0,    0,    0,    0,    0,    0,   24,
   23,   21,   22,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    2,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   18,   19,   20,
};
const short yydgoto[] = {                                       7,
    8,
};
const short yysindex[] = {                                   -255,
    0, -255, -255, -255, -255, -255,    0,  -67,  -85,    0,
    0,    0,    0, -255, -255, -255, -255, -255, -255, -255,
 -255, -255, -255, -255, -255, -255, -255, -255, -255, -255,
 -255,    0,  -50,  -34,  -19,  141, -261, -233, -233, -223,
 -223, -223, -223, -253, -253, -248, -248,    0,    0,    0,
};
const short yyrindex[] = {                                      0,
    0,    0,    0,    0,    0,    0,    0,   30,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,  143,  140,  136,  131,  125,  109,  117,   61,
   73,   85,   97,   33,   47,    1,   17,    0,    0,    0,
};
const short yygindex[] = {                                      0,
  142,
};
#define YYTABLESIZE 418
const short yytable[] = {                                       0,
   16,    1,    2,   19,   20,   21,   22,   23,   24,   25,
   26,   27,   28,   29,   30,   31,   17,    3,    4,   27,
   28,   29,   30,   31,    5,    6,   29,   30,   31,    1,
    0,    0,   14,   21,   22,   23,   24,   25,   26,   27,
   28,   29,   30,   31,    0,    0,   15,   25,   26,   27,
   28,   29,   30,   31,    0,    0,    0,    0,    0,    0,
   11,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    9,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   10,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   12,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    8,    0,
    0,    0,    0,    0,    0,    0,   13,    0,    0,    0,
    0,    0,    0,    0,    7,    0,    0,    0,    0,    0,
    6,    0,    0,    0,    0,    5,    0,    0,    0,    4,
    0,    0,    3,    9,   10,   11,   12,   13,    0,    0,
    0,    0,    0,    0,    0,   33,   34,   35,   36,   37,
   38,   39,   40,   41,   42,   43,   44,   45,   46,   47,
   48,   49,   50,   32,   14,   15,   16,   17,   18,   19,
   20,   21,   22,   23,   24,   25,   26,   27,   28,   29,
   30,   31,   14,   15,   16,   17,   18,   19,   20,   21,
   22,   23,   24,   25,   26,   27,   28,   29,   30,   31,
   15,   16,   17,   18,   19,   20,   21,   22,   23,   24,
   25,   26,   27,   28,   29,   30,   31,   16,   17,   18,
   19,   20,   21,   22,   23,   24,   25,   26,   27,   28,
   29,   30,   31,   17,   18,   19,   20,   21,   22,   23,
   24,   25,   26,   27,   28,   29,   30,   31,    0,   16,
   16,   16,   16,   16,   16,   16,   16,   16,   16,   16,
   16,   16,   16,   16,   16,   17,   17,   17,   17,   17,
   17,   17,   17,   17,   17,   17,   17,   17,   17,   17,
   17,   14,   14,   14,   14,   14,   14,   14,   14,   14,
   14,   14,   14,   14,   14,   15,   15,   15,   15,   15,
   15,   15,   15,   15,   15,   15,   15,   15,   15,   11,
   11,   11,   11,   11,   11,   11,   11,   11,   11,   11,
   11,    9,    9,    9,    9,    9,    9,    9,    9,    9,
    9,    9,    9,   10,   10,   10,   10,   10,   10,   10,
   10,   10,   10,   10,   10,   12,   12,   12,   12,   12,
   12,   12,   12,   12,   12,   12,   12,    8,    8,    8,
    8,    8,    8,    8,    8,   13,   13,   13,   13,   13,
   13,   13,   13,    7,    7,    7,    7,    7,    7,    6,
    6,    6,    6,    6,    5,    5,    5,    5,    4,    4,
    4,    3,    3,    0,   18,   19,   20,   21,   22,   23,
   24,   25,   26,   27,   28,   29,   30,   31,
};
const short yycheck[] = {                                      -1,
    0,  257,  258,  265,  266,  267,  268,  269,  270,  271,
  272,  273,  274,  275,  276,  277,    0,  273,  274,  273,
  274,  275,  276,  277,  280,  281,  275,  276,  277,    0,
   -1,   -1,    0,  267,  268,  269,  270,  271,  272,  273,
  274,  275,  276,  277,   -1,   -1,    0,  271,  272,  273,
  274,  275,  276,  277,   -1,   -1,   -1,   -1,   -1,   -1,
    0,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,    0,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,    0,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,    0,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,    0,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,    0,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,    0,   -1,   -1,   -1,   -1,   -1,
    0,   -1,   -1,   -1,   -1,    0,   -1,   -1,   -1,    0,
   -1,   -1,    0,    2,    3,    4,    5,    6,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   14,   15,   16,   17,   18,
   19,   20,   21,   22,   23,   24,   25,   26,   27,   28,
   29,   30,   31,  259,  260,  261,  262,  263,  264,  265,
  266,  267,  268,  269,  270,  271,  272,  273,  274,  275,
  276,  277,  260,  261,  262,  263,  264,  265,  266,  267,
  268,  269,  270,  271,  272,  273,  274,  275,  276,  277,
  261,  262,  263,  264,  265,  266,  267,  268,  269,  270,
  271,  272,  273,  274,  275,  276,  277,  262,  263,  264,
  265,  266,  267,  268,  269,  270,  271,  272,  273,  274,
  275,  276,  277,  263,  264,  265,  266,  267,  268,  269,
  270,  271,  272,  273,  274,  275,  276,  277,   -1,  259,
  260,  261,  262,  263,  264,  265,  266,  267,  268,  269,
  270,  271,  272,  273,  274,  259,  260,  261,  262,  263,
  264,  265,  266,  267,  268,  269,  270,  271,  272,  273,
  274,  259,  260,  261,  262,  263,  264,  265,  266,  267,
  268,  269,  270,  271,  272,  259,  260,  261,  262,  263,
  264,  265,  266,  267,  268,  269,  270,  271,  272,  259,
  260,  261,  262,  263,  264,  265,  266,  267,  268,  269,
  270,  259,  260,  261,  262,  263,  264,  265,  266,  267,
  268,  269,  270,  259,  260,  261,  262,  263,  264,  265,
  266,  267,  268,  269,  270,  259,  260,  261,  262,  263,
  264,  265,  266,  267,  268,  269,  270,  259,  260,  261,
  262,  263,  264,  265,  266,  259,  260,  261,  262,  263,
  264,  265,  266,  259,  260,  261,  262,  263,  264,  259,
  260,  261,  262,  263,  259,  260,  261,  262,  259,  260,
  261,  259,  260,   -1,  264,  265,  266,  267,  268,  269,
  270,  271,  272,  273,  274,  275,  276,  277,
};
#define YYFINAL 7
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 281
#if YYDEBUG
const char * const yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"ARITH_NUM","ARITH_LPAREN",
"ARITH_RPAREN","ARITH_OR","ARITH_AND","ARITH_BOR","ARITH_BXOR","ARITH_BAND",
"ARITH_EQ","ARITH_NE","ARITH_LT","ARITH_GT","ARITH_GE","ARITH_LE",
"ARITH_LSHIFT","ARITH_RSHIFT","ARITH_ADD","ARITH_SUB","ARITH_MUL","ARITH_DIV",
"ARITH_REM","ARITH_UNARYMINUS","ARITH_UNARYPLUS","ARITH_NOT","ARITH_BNOT",
};
const char * const yyrule[] = {
"$accept : exp",
"exp : expr",
"expr : ARITH_LPAREN expr ARITH_RPAREN",
"expr : expr ARITH_OR expr",
"expr : expr ARITH_AND expr",
"expr : expr ARITH_BOR expr",
"expr : expr ARITH_BXOR expr",
"expr : expr ARITH_BAND expr",
"expr : expr ARITH_EQ expr",
"expr : expr ARITH_GT expr",
"expr : expr ARITH_GE expr",
"expr : expr ARITH_LT expr",
"expr : expr ARITH_LE expr",
"expr : expr ARITH_NE expr",
"expr : expr ARITH_LSHIFT expr",
"expr : expr ARITH_RSHIFT expr",
"expr : expr ARITH_ADD expr",
"expr : expr ARITH_SUB expr",
"expr : expr ARITH_MUL expr",
"expr : expr ARITH_DIV expr",
"expr : expr ARITH_REM expr",
"expr : ARITH_NOT expr",
"expr : ARITH_BNOT expr",
"expr : ARITH_SUB expr",
"expr : ARITH_ADD expr",
"expr : ARITH_NUM",
};
#endif
#ifndef YYSTYPE
typedef int YYSTYPE;
#endif
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 10000
#define YYMAXDEPTH 10000
#endif
#endif
#define YYINITSTACKSIZE 200
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
short *yyss;
short *yysslim;
YYSTYPE *yyvs;
int yystacksize;
int yyparse __P((void));
#line 120 "/usr/sources/cvs.netbsd.org/src/distrib/arm32/ramdisk/../../../bin/sh/arith.y"
int
arith(s)
	const char *s;
{
	long result;

	arith_buf = arith_startbuf = s;

	INTOFF;
	result = yyparse();
	arith_lex_reset();	/* reprime lex */
	INTON;

	return (result);
}


/*
 *  The exp(1) builtin.
 */
int
expcmd(argc, argv)
	int argc;
	char **argv;
{
	const char *p;
	char *concat;
	char **ap;
	long i;

	if (argc > 1) {
		p = argv[1];
		if (argc > 2) {
			/*
			 * concatenate arguments
			 */
			STARTSTACKSTR(concat);
			ap = argv + 2;
			for (;;) {
				while (*p)
					STPUTC(*p++, concat);
				if ((p = *ap++) == NULL)
					break;
				STPUTC(' ', concat);
			}
			STPUTC('\0', concat);
			p = grabstackstr(concat);
		}
	} else
		p = "";

	i = arith(p);

	out1fmt("%ld\n", i);
	return (! i);
}

/*************************/
#ifdef TEST_ARITH
#include <stdio.h>
main(argc, argv)
	char *argv[];
{
	printf("%d\n", exp(argv[1]));
}
error(s)
	char *s;
{
	fprintf(stderr, "exp: %s\n", s);
	exit(1);
}
#endif

void
yyerror(s)
	const char *s;
{

	yyerrok;
	yyclearin;
	arith_lex_reset();	/* reprime lex */
	error("arithmetic expression: %s: \"%s\"", s, arith_startbuf);
	/* NOTREACHED */
}
#line 399 "arith.c"
/* allocate initial stack or double stack size, up to YYMAXDEPTH */
static int yygrowstack __P((void));
static int yygrowstack()
{
    int newsize, i;
    short *newss;
    YYSTYPE *newvs;

    if ((newsize = yystacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return -1;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;
    i = yyssp - yyss;
    if ((newss = (short *)realloc(yyss, newsize * sizeof *newss)) == NULL)
        return -1;
    yyss = newss;
    yyssp = newss + i;
    if ((newvs = (YYSTYPE *)realloc(yyvs, newsize * sizeof *newvs)) == NULL)
        return -1;
    yyvs = newvs;
    yyvsp = newvs + i;
    yystacksize = newsize;
    yysslim = yyss + newsize - 1;
    return 0;
}

#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab
int
yyparse()
{
    int yym, yyn, yystate;
#if YYDEBUG
    const char *yys;

    if ((yys = getenv("YYDEBUG")) != NULL)
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    if (yyss == NULL && yygrowstack()) goto yyoverflow;
    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if ((yyn = yydefred[yystate]) != 0) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yyssp >= yysslim && yygrowstack())
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = (-1);
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
    goto yynewerror;
yynewerror:
    yyerror("syntax error");
    goto yyerrlab;
yyerrlab:
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yysslim && yygrowstack())
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 1:
#line 80 "/usr/sources/cvs.netbsd.org/src/distrib/arm32/ramdisk/../../../bin/sh/arith.y"
 {
			return (yyvsp[0]);
		}
break;
case 2:
#line 86 "/usr/sources/cvs.netbsd.org/src/distrib/arm32/ramdisk/../../../bin/sh/arith.y"
 { yyval = yyvsp[-1]; }
break;
case 3:
#line 87 "/usr/sources/cvs.netbsd.org/src/distrib/arm32/ramdisk/../../../bin/sh/arith.y"
 { yyval = yyvsp[-2] ? yyvsp[-2] : yyvsp[0] ? yyvsp[0] : 0; }
break;
case 4:
#line 88 "/usr/sources/cvs.netbsd.org/src/distrib/arm32/ramdisk/../../../bin/sh/arith.y"
 { yyval = yyvsp[-2] ? ( yyvsp[0] ? yyvsp[0] : 0 ) : 0; }
break;
case 5:
#line 89 "/usr/sources/cvs.netbsd.org/src/distrib/arm32/ramdisk/../../../bin/sh/arith.y"
 { yyval = yyvsp[-2] | yyvsp[0]; }
break;
case 6:
#line 90 "/usr/sources/cvs.netbsd.org/src/distrib/arm32/ramdisk/../../../bin/sh/arith.y"
 { yyval = yyvsp[-2] ^ yyvsp[0]; }
break;
case 7:
#line 91 "/usr/sources/cvs.netbsd.org/src/distrib/arm32/ramdisk/../../../bin/sh/arith.y"
 { yyval = yyvsp[-2] & yyvsp[0]; }
break;
case 8:
#line 92 "/usr/sources/cvs.netbsd.org/src/distrib/arm32/ramdisk/../../../bin/sh/arith.y"
 { yyval = yyvsp[-2] == yyvsp[0]; }
break;
case 9:
#line 93 "/usr/sources/cvs.netbsd.org/src/distrib/arm32/ramdisk/../../../bin/sh/arith.y"
 { yyval = yyvsp[-2] > yyvsp[0]; }
break;
case 10:
#line 94 "/usr/sources/cvs.netbsd.org/src/distrib/arm32/ramdisk/../../../bin/sh/arith.y"
 { yyval = yyvsp[-2] >= yyvsp[0]; }
break;
case 11:
#line 95 "/usr/sources/cvs.netbsd.org/src/distrib/arm32/ramdisk/../../../bin/sh/arith.y"
 { yyval = yyvsp[-2] < yyvsp[0]; }
break;
case 12:
#line 96 "/usr/sources/cvs.netbsd.org/src/distrib/arm32/ramdisk/../../../bin/sh/arith.y"
 { yyval = yyvsp[-2] <= yyvsp[0]; }
break;
case 13:
#line 97 "/usr/sources/cvs.netbsd.org/src/distrib/arm32/ramdisk/../../../bin/sh/arith.y"
 { yyval = yyvsp[-2] != yyvsp[0]; }
break;
case 14:
#line 98 "/usr/sources/cvs.netbsd.org/src/distrib/arm32/ramdisk/../../../bin/sh/arith.y"
 { yyval = yyvsp[-2] << yyvsp[0]; }
break;
case 15:
#line 99 "/usr/sources/cvs.netbsd.org/src/distrib/arm32/ramdisk/../../../bin/sh/arith.y"
 { yyval = yyvsp[-2] >> yyvsp[0]; }
break;
case 16:
#line 100 "/usr/sources/cvs.netbsd.org/src/distrib/arm32/ramdisk/../../../bin/sh/arith.y"
 { yyval = yyvsp[-2] + yyvsp[0]; }
break;
case 17:
#line 101 "/usr/sources/cvs.netbsd.org/src/distrib/arm32/ramdisk/../../../bin/sh/arith.y"
 { yyval = yyvsp[-2] - yyvsp[0]; }
break;
case 18:
#line 102 "/usr/sources/cvs.netbsd.org/src/distrib/arm32/ramdisk/../../../bin/sh/arith.y"
 { yyval = yyvsp[-2] * yyvsp[0]; }
break;
case 19:
#line 103 "/usr/sources/cvs.netbsd.org/src/distrib/arm32/ramdisk/../../../bin/sh/arith.y"
 {
			if (yyvsp[0] == 0)
				yyerror("division by zero");
			yyval = yyvsp[-2] / yyvsp[0];
			}
break;
case 20:
#line 108 "/usr/sources/cvs.netbsd.org/src/distrib/arm32/ramdisk/../../../bin/sh/arith.y"
 {
			if (yyvsp[0] == 0)
				yyerror("division by zero");
			yyval = yyvsp[-2] % yyvsp[0];
			}
break;
case 21:
#line 113 "/usr/sources/cvs.netbsd.org/src/distrib/arm32/ramdisk/../../../bin/sh/arith.y"
 { yyval = !(yyvsp[0]); }
break;
case 22:
#line 114 "/usr/sources/cvs.netbsd.org/src/distrib/arm32/ramdisk/../../../bin/sh/arith.y"
 { yyval = ~(yyvsp[0]); }
break;
case 23:
#line 115 "/usr/sources/cvs.netbsd.org/src/distrib/arm32/ramdisk/../../../bin/sh/arith.y"
 { yyval = -(yyvsp[0]); }
break;
case 24:
#line 116 "/usr/sources/cvs.netbsd.org/src/distrib/arm32/ramdisk/../../../bin/sh/arith.y"
 { yyval = yyvsp[0]; }
break;
#line 670 "arith.c"
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if (yyssp >= yysslim && yygrowstack())
    {
        goto yyoverflow;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    return (1);
yyaccept:
    return (0);
}
