/*	$NetBSD: skeleton.c,v 1.5 2009/10/29 21:03:59 christos Exp $	*/
/* Id: skeleton.c,v 1.19 2008/12/24 14:52:28 tom Exp */

#include "defs.h"

#include <sys/cdefs.h>
__RCSID("$NetBSD: skeleton.c,v 1.5 2009/10/29 21:03:59 christos Exp $");

/*  The definition of yysccsid in the banner should be replaced with	*/
/*  a #pragma ident directive if the target C compiler supports		*/
/*  #pragma ident directives.						*/
/*									*/
/*  If the skeleton is changed, the banner should be changed so that	*/
/*  the altered version can be easily distinguished from the original.	*/
/*									*/
/*  The #defines included with the banner are there because they are	*/
/*  useful in subsequent code.  The macros #defined in the header or	*/
/*  the body either are not useful outside of semantic actions or	*/
/*  are conditional.							*/

const char * const banner[] =
{
    "#ifndef lint",
    "static const char yysccsid[] = \"@(#)yaccpar	1.9 (Berkeley) 02/21/93\";",
    "#endif",
    "",
    "#ifdef _LIBC",
    "#include \"namespace.h\"",
    "#endif",
    "#include <stdlib.h>",
    "#include <string.h>",
    "",
    "#define YYBYACC 1",
    CONCAT1("#define YYMAJOR ", YYMAJOR),
    CONCAT1("#define YYMINOR ", YYMINOR),
#ifdef YYPATCH
    CONCAT1("#define YYPATCH ", YYPATCH),
#endif
    "",
    "#define YYEMPTY        (-1)",
    "#define yyclearin      (yychar = YYEMPTY)",
    "#define yyerrok        (yyerrflag = 0)",
    "#define YYRECOVERING() (yyerrflag != 0)",
    "",
    0
};

const char * const tables[] =
{
    "extern short yylhs[];",
    "extern short yylen[];",
    "extern short yydefred[];",
    "extern short yydgoto[];",
    "extern short yysindex[];",
    "extern short yyrindex[];",
    "extern short yygindex[];",
    "extern short yytable[];",
    "extern short yycheck[];",
    "",
    "#if YYDEBUG",
    "extern char *yyname[];",
    "extern char *yyrule[];",
    "#endif",
    0
};

const char * const header[] =
{
    "#if YYDEBUG",
    "#include <stdio.h>",
    "#endif",
    "",
    "extern int YYPARSE_DECL();",
    "static int yygrowstack(short **, short **, short **,",
    "    YYSTYPE **, YYSTYPE **, unsigned *);",
    "",
    "/* define the initial stack-sizes */",
    "#ifdef YYSTACKSIZE",
    "#undef YYMAXDEPTH",
    "#define YYMAXDEPTH  YYSTACKSIZE",
    "#else",
    "#ifdef YYMAXDEPTH",
    "#define YYSTACKSIZE YYMAXDEPTH",
    "#else",
    "#define YYSTACKSIZE 500",
    "#define YYMAXDEPTH  500",
    "#endif",
    "#endif",
    "",
    "#define YYINITSTACKSIZE 500",
    "",
    "int      yydebug;",
    "int      yyerrflag;",
    "\003",
    "",
    0
};

const char * const body[] =
{
    "/* allocate initial stack or double stack size, up to YYMAXDEPTH */",
    "static int yygrowstack(short **yyss, short **yyssp, short **yysslim,",
    "    YYSTYPE **yyvs, YYSTYPE **yyvsp, unsigned *yystacksize)",
    "{",
    "    int i;",
    "    unsigned newsize;",
    "    short *newss;",
    "    YYSTYPE *newvs;",
    "",
    "    if ((newsize = *yystacksize) == 0)",
    "        newsize = YYINITSTACKSIZE;",
    "    else if (newsize >= YYMAXDEPTH)",
    "        return -1;",
    "    else if ((newsize *= 2) > YYMAXDEPTH)",
    "        newsize = YYMAXDEPTH;",
    "",
    "    i = *yyssp - *yyss;",
    "    newss = (*yyss != 0)",
    "          ? (short *)realloc(*yyss, newsize * sizeof(*newss))",
    "          : (short *)malloc(newsize * sizeof(*newss));",
    "    if (newss == 0)",
    "        return -1;",
    "",
    "    *yyss  = newss;",
    "    *yyssp = newss + i;",
    "    newvs = (yyvs != 0)",
    "          ? (YYSTYPE *)realloc(*yyvs, newsize * sizeof(*newvs))",
    "          : (YYSTYPE *)malloc(newsize * sizeof(*newvs));",
    "    if (newvs == 0)",
    "        return -1;",
    "",
    "    *yyvs = newvs;",
    "    *yyvsp = newvs + i;",
    "    *yystacksize = newsize;",
    "    *yysslim = *yyss + newsize - 1;",
    "    return 0;",
    "}",
    "",
    "#define YYABORT  goto yyabort",
    "#define YYREJECT goto yyabort",
    "#define YYACCEPT goto yyaccept",
    "#define YYERROR  goto yyerrlab",
    "",
    "int",
    "YYPARSE_DECL()",
    "{",
    "    int yym, yyn, yystate;",
    "\003",
    "    YYSTYPE  yyval;",
    "    /* variables for the parser stack */",
    "    short   *yyssp;",
    "    short   *yyss;",
    "    short   *yysslim;",
    "    YYSTYPE *yyvs;",
    "    YYSTYPE *yyvsp;",
    "    unsigned yystacksize;",
    "#if YYDEBUG",
    "    const char *yys;",
    "",
    "    if ((yys = getenv(\"YYDEBUG\")) != 0)",
    "    {",
    "        yyn = *yys;",
    "        if (yyn >= '0' && yyn <= '9')",
    "            yydebug = yyn - '0';",
    "    }",
    "#endif",
    "",
    "    yynerrs = 0;",
    "    yyerrflag = 0;",
    "    yychar = YYEMPTY;",
    "    yystate = 0;",
    "",
    "    yystacksize = 0;",
    "    yyvs = NULL;",
    "    yyss = NULL;",
    "    if (yygrowstack(&yyss, &yyssp, &yysslim, &yyvs, &yyvsp, &yystacksize))",
    "        goto yyoverflow;",
    "    yyssp = yyss;",
    "    yyvsp = yyvs;",
    "    yystate = 0;",
    "    *yyssp = 0;",
    "",
    "yyloop:",
    "    if ((yyn = yydefred[yystate]) != 0) goto yyreduce;",
    "    if (yychar < 0)",
    "    {",
    "        if ((yychar = yylex(\002)) < 0) yychar = 0;",
    "#if YYDEBUG",
    "        if (yydebug)",
    "        {",
    "            yys = 0;",
    "            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];",
    "            if (!yys) yys = \"illegal-symbol\";",
    "            printf(\"%sdebug: state %d, reading %d (%s)\\n\",",
    "                    YYPREFIX, yystate, yychar, yys);",
    "        }",
    "#endif",
    "    }",
    "    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&",
    "            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)",
    "    {",
    "#if YYDEBUG",
    "        if (yydebug)",
    "            printf(\"%sdebug: state %d, shifting to state %d\\n\",",
    "                    YYPREFIX, yystate, yytable[yyn]);",
    "#endif",
    "        if (yyssp >= yysslim && yygrowstack(&yyss, &yyssp, &yysslim,",
    "            &yyvs, &yyvsp, &yystacksize))",
    "        {",
    "            goto yyoverflow;",
    "        }",
    "        yystate = yytable[yyn];",
    "        *++yyssp = yytable[yyn];",
    "        *++yyvsp = yylval;",
    "        yychar = YYEMPTY;",
    "        if (yyerrflag > 0)  --yyerrflag;",
    "        goto yyloop;",
    "    }",
    "    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&",
    "            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)",
    "    {",
    "        yyn = yytable[yyn];",
    "        goto yyreduce;",
    "    }",
    "    if (yyerrflag) goto yyinrecovery;",
    "",
    "    yyerror(\001\"syntax error\");",
    "",
    "    goto yyerrlab;",
    "",
    "yyerrlab:",
    "    ++yynerrs;",
    "",
    "yyinrecovery:",
    "    if (yyerrflag < 3)",
    "    {",
    "        yyerrflag = 3;",
    "        for (;;)",
    "        {",
    "            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&",
    "                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)",
    "            {",
    "#if YYDEBUG",
    "                if (yydebug)",
    "                    printf(\"%sdebug: state %d, error recovery shifting\\",
    " to state %d\\n\", YYPREFIX, *yyssp, yytable[yyn]);",
    "#endif",
    "                if (yyssp >= yysslim && yygrowstack(&yyss, &yyssp,",
    "                    &yysslim, &yyvs, &yyvsp, &yystacksize))",
    "                {",
    "                    goto yyoverflow;",
    "                }",
    "                yystate = yytable[yyn];",
    "                *++yyssp = yytable[yyn];",
    "                *++yyvsp = yylval;",
    "                goto yyloop;",
    "            }",
    "            else",
    "            {",
    "#if YYDEBUG",
    "                if (yydebug)",
    "                    printf(\"%sdebug: error recovery discarding state %d\
\\n\",",
    "                            YYPREFIX, *yyssp);",
    "#endif",
    "                if (yyssp <= yyss) goto yyabort;",
    "                --yyssp;",
    "                --yyvsp;",
    "            }",
    "        }",
    "    }",
    "    else",
    "    {",
    "        if (yychar == 0) goto yyabort;",
    "#if YYDEBUG",
    "        if (yydebug)",
    "        {",
    "            yys = 0;",
    "            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];",
    "            if (!yys) yys = \"illegal-symbol\";",
    "            printf(\"%sdebug: state %d, error recovery discards token %d\
 (%s)\\n\",",
    "                    YYPREFIX, yystate, yychar, yys);",
    "        }",
    "#endif",
    "        yychar = YYEMPTY;",
    "        goto yyloop;",
    "    }",
    "",
    "yyreduce:",
    "#if YYDEBUG",
    "    if (yydebug)",
    "        printf(\"%sdebug: state %d, reducing by rule %d (%s)\\n\",",
    "                YYPREFIX, yystate, yyn, yyrule[yyn]);",
    "#endif",
    "    yym = yylen[yyn];",
    "    if (yym)",
    "        yyval = yyvsp[1-yym];",
    "    else",
    "        memset(&yyval, 0, sizeof yyval);",
    "    switch (yyn)",
    "    {",
    0
};

const char * const trailer[] =
{
    "    }",
    "    yyssp -= yym;",
    "    yystate = *yyssp;",
    "    yyvsp -= yym;",
    "    yym = yylhs[yyn];",
    "    if (yystate == 0 && yym == 0)",
    "    {",
    "#if YYDEBUG",
    "        if (yydebug)",
    "            printf(\"%sdebug: after reduction, shifting from state 0 to\\",
    " state %d\\n\", YYPREFIX, YYFINAL);",
    "#endif",
    "        yystate = YYFINAL;",
    "        *++yyssp = YYFINAL;",
    "        *++yyvsp = yyval;",
    "        if (yychar < 0)",
    "        {",
    "            if ((yychar = yylex(\002)) < 0) yychar = 0;",
    "#if YYDEBUG",
    "            if (yydebug)",
    "            {",
    "                yys = 0;",
    "                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];",
    "                if (!yys) yys = \"illegal-symbol\";",
    "                printf(\"%sdebug: state %d, reading %d (%s)\\n\",",
    "                        YYPREFIX, YYFINAL, yychar, yys);",
    "            }",
    "#endif",
    "        }",
    "        if (yychar == 0) goto yyaccept;",
    "        goto yyloop;",
    "    }",
    "    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&",
    "            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)",
    "        yystate = yytable[yyn];",
    "    else",
    "        yystate = yydgoto[yym];",
    "#if YYDEBUG",
    "    if (yydebug)",
    "        printf(\"%sdebug: after reduction, shifting from state %d \\",
    "to state %d\\n\", YYPREFIX, *yyssp, yystate);",
    "#endif",
    "    if (yyssp >= yysslim && yygrowstack(&yyss, &yyssp,",
    "        &yysslim, &yyvs, &yyvsp, &yystacksize))",
    "    {",
    "        goto yyoverflow;",
    "    }",
    "    *++yyssp = (short) yystate;",
    "    *++yyvsp = yyval;",
    "    goto yyloop;",
    "",
    "yyoverflow:",
    "    yyerror(\001\"yacc stack overflow\");",
    "",
    "yyabort:",
    "	 free(yyss);",
    "	 free(yyvs);",
    "    return (1);",
    "",
    "yyaccept:",
    "	 free(yyss);",
    "	 free(yyvs);",
    "    return (0);",
    "}",
    0
};

void
write_section(const char * const section[], int dodecls)
{
    int c;
    int i;
    const char *s;
    const char *comma;
    param *p;
    FILE *f;

    f = code_file;
    for (i = 0; (s = section[i]) != 0; ++i)
    {
	++outline;
	while ((c = *s) != 0)
	{
	    switch (c) {
	    case '\001':
		p = parse_param;
		for (; p != NULL; p = p->next)
		    fprintf(f, "%s, ", p->name);
		break;

	    case '\002':
		p = lex_param;
		if (pure_parser) {
		    fprintf(f, "&yylval");
		    comma = ", ";
		} else 
		    comma = "";
		for (; p != NULL; p = p->next) {
		    fprintf(f, "%s%s", comma, p->name);
		    comma = ", ";
		}
		break;

	    case '\003':
		if (!dodecls)
		    break;
		fprintf(f, 
		    "    int      yynerrs;\n"
		    "    int      yychar;\n"
		    "    YYSTYPE  yylval;");
		break;

	    default:
		putc(c, f);
		break;
	    }
	    ++s;
	}
	putc('\n', f);
    }
}
