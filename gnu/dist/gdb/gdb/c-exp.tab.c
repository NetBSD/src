
/*  A Bison parser, made from c-exp.y
 by  GNU Bison version 1.27
  */

#define YYBISON 1  /* Identify Bison output.  */

#define	INT	257
#define	FLOAT	258
#define	STRING	259
#define	NAME	260
#define	TYPENAME	261
#define	NAME_OR_INT	262
#define	STRUCT	263
#define	CLASS	264
#define	UNION	265
#define	ENUM	266
#define	SIZEOF	267
#define	UNSIGNED	268
#define	COLONCOLON	269
#define	TEMPLATE	270
#define	ERROR	271
#define	SIGNED_KEYWORD	272
#define	LONG	273
#define	SHORT	274
#define	INT_KEYWORD	275
#define	CONST_KEYWORD	276
#define	VOLATILE_KEYWORD	277
#define	DOUBLE_KEYWORD	278
#define	VARIABLE	279
#define	ASSIGN_MODIFY	280
#define	THIS	281
#define	TRUEKEYWORD	282
#define	FALSEKEYWORD	283
#define	ABOVE_COMMA	284
#define	OROR	285
#define	ANDAND	286
#define	EQUAL	287
#define	NOTEQUAL	288
#define	LEQ	289
#define	GEQ	290
#define	LSH	291
#define	RSH	292
#define	UNARY	293
#define	INCREMENT	294
#define	DECREMENT	295
#define	ARROW	296
#define	BLOCKNAME	297
#define	FILENAME	298

#line 39 "c-exp.y"


#include "defs.h"
#include "gdb_string.h"
#include <ctype.h>
#include "expression.h"
#include "value.h"
#include "parser-defs.h"
#include "language.h"
#include "c-lang.h"
#include "bfd.h" /* Required by objfiles.h.  */
#include "symfile.h" /* Required by objfiles.h.  */
#include "objfiles.h" /* For have_full_symbols and have_partial_symbols */

/* Flag indicating we're dealing with HP-compiled objects */ 
extern int hp_som_som_object_present;

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror, etc),
   as well as gratuitiously global symbol names, so we can have multiple
   yacc generated parsers in gdb.  Note that these are only the variables
   produced by yacc.  If other parser generators (bison, byacc, etc) produce
   additional global names that conflict at link time, then those parser
   generators need to be fixed instead of adding those names to this list. */

#define	yymaxdepth c_maxdepth
#define	yyparse	c_parse
#define	yylex	c_lex
#define	yyerror	c_error
#define	yylval	c_lval
#define	yychar	c_char
#define	yydebug	c_debug
#define	yypact	c_pact	
#define	yyr1	c_r1			
#define	yyr2	c_r2			
#define	yydef	c_def		
#define	yychk	c_chk		
#define	yypgo	c_pgo		
#define	yyact	c_act		
#define	yyexca	c_exca
#define yyerrflag c_errflag
#define yynerrs	c_nerrs
#define	yyps	c_ps
#define	yypv	c_pv
#define	yys	c_s
#define	yy_yys	c_yys
#define	yystate	c_state
#define	yytmp	c_tmp
#define	yyv	c_v
#define	yy_yyv	c_yyv
#define	yyval	c_val
#define	yylloc	c_lloc
#define yyreds	c_reds		/* With YYDEBUG defined */
#define yytoks	c_toks		/* With YYDEBUG defined */
#define yyname	c_name		/* With YYDEBUG defined */
#define yyrule	c_rule		/* With YYDEBUG defined */
#define yylhs	c_yylhs
#define yylen	c_yylen
#define yydefred c_yydefred
#define yydgoto	c_yydgoto
#define yysindex c_yysindex
#define yyrindex c_yyrindex
#define yygindex c_yygindex
#define yytable	 c_yytable
#define yycheck	 c_yycheck

#ifndef YYDEBUG
#define	YYDEBUG 1		/* Default to yydebug support */
#endif

#define YYFPRINTF parser_fprintf

int yyparse (void);

static int yylex (void);

void yyerror (char *);


#line 122 "c-exp.y"
typedef union
  {
    LONGEST lval;
    struct {
      LONGEST val;
      struct type *type;
    } typed_val_int;
    struct {
      DOUBLEST dval;
      struct type *type;
    } typed_val_float;
    struct symbol *sym;
    struct type *tval;
    struct stoken sval;
    struct ttype tsym;
    struct symtoken ssym;
    int voidval;
    struct block *bval;
    enum exp_opcode opcode;
    struct internalvar *ivar;

    struct type **tvec;
    int *ivec;
  } YYSTYPE;
#line 147 "c-exp.y"

/* YYSTYPE gets defined by %union */
static int parse_number (char *, int, int, YYSTYPE *);
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		240
#define	YYFLAG		-32768
#define	YYNTBASE	69

#define YYTRANSLATE(x) ((unsigned)(x) <= 298 ? yytranslate[x] : 98)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    62,     2,     2,     2,    52,    38,     2,    59,
    65,    50,    48,    30,    49,    57,    51,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,    68,     2,    41,
    32,    42,    33,    47,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
    58,     2,    64,    37,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    66,    36,    67,    63,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     3,     4,     5,     6,
     7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
    17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
    27,    28,    29,    31,    34,    35,    39,    40,    43,    44,
    45,    46,    53,    54,    55,    56,    60,    61
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     2,     4,     6,     8,    12,    15,    18,    21,    24,
    27,    30,    33,    36,    39,    42,    46,    50,    55,    59,
    63,    68,    73,    74,    80,    82,    83,    85,    89,    91,
    95,   100,   105,   109,   113,   117,   121,   125,   129,   133,
   137,   141,   145,   149,   153,   157,   161,   165,   169,   173,
   177,   181,   185,   191,   195,   199,   201,   203,   205,   207,
   209,   214,   216,   218,   220,   222,   224,   226,   230,   234,
   238,   243,   245,   248,   250,   253,   255,   256,   260,   262,
   264,   266,   267,   269,   272,   274,   277,   279,   283,   286,
   288,   291,   293,   296,   300,   303,   307,   309,   313,   315,
   317,   319,   321,   324,   328,   331,   335,   339,   343,   346,
   349,   353,   358,   362,   366,   370,   375,   379,   384,   388,
   393,   396,   400,   403,   407,   410,   414,   416,   419,   422,
   425,   428,   431,   434,   436,   439,   441,   447,   450,   453,
   455,   457,   459,   461,   463,   467,   469,   474,   477,   480,
   482,   484,   486,   488,   490,   492,   494,   496
};

static const short yyrhs[] = {    71,
     0,    70,     0,    89,     0,    72,     0,    71,    30,    72,
     0,    50,    72,     0,    38,    72,     0,    49,    72,     0,
    62,    72,     0,    63,    72,     0,    54,    72,     0,    55,
    72,     0,    72,    54,     0,    72,    55,     0,    13,    72,
     0,    72,    56,    96,     0,    72,    56,    79,     0,    72,
    56,    50,    72,     0,    72,    57,    96,     0,    72,    57,
    79,     0,    72,    57,    50,    72,     0,    72,    58,    71,
    64,     0,     0,    72,    59,    73,    75,    65,     0,    66,
     0,     0,    72,     0,    75,    30,    72,     0,    67,     0,
    74,    75,    76,     0,    74,    89,    76,    72,     0,    59,
    89,    65,    72,     0,    59,    71,    65,     0,    72,    47,
    72,     0,    72,    50,    72,     0,    72,    51,    72,     0,
    72,    52,    72,     0,    72,    48,    72,     0,    72,    49,
    72,     0,    72,    45,    72,     0,    72,    46,    72,     0,
    72,    39,    72,     0,    72,    40,    72,     0,    72,    43,
    72,     0,    72,    44,    72,     0,    72,    41,    72,     0,
    72,    42,    72,     0,    72,    38,    72,     0,    72,    37,
    72,     0,    72,    36,    72,     0,    72,    35,    72,     0,
    72,    34,    72,     0,    72,    33,    72,    68,    72,     0,
    72,    32,    72,     0,    72,    26,    72,     0,     3,     0,
     8,     0,     4,     0,    78,     0,    25,     0,    13,    59,
    89,    65,     0,     5,     0,    27,     0,    28,     0,    29,
     0,    60,     0,    61,     0,    77,    15,    96,     0,    77,
    15,    96,     0,    90,    15,    96,     0,    90,    15,    63,
    96,     0,    79,     0,    15,    96,     0,    97,     0,    47,
     6,     0,    95,     0,     0,    81,    80,    81,     0,    82,
     0,    95,     0,    83,     0,     0,    50,     0,    50,    85,
     0,    38,     0,    38,    85,     0,    86,     0,    59,    85,
    65,     0,    86,    87,     0,    87,     0,    86,    88,     0,
    88,     0,    58,    64,     0,    58,     3,    64,     0,    59,
    65,     0,    59,    92,    65,     0,    93,     0,    90,    15,
    50,     0,     7,     0,    21,     0,    19,     0,    20,     0,
    19,    21,     0,    19,    18,    21,     0,    19,    18,     0,
    18,    19,    21,     0,    14,    19,    21,     0,    19,    14,
    21,     0,    19,    14,     0,    19,    19,     0,    19,    19,
    21,     0,    19,    19,    18,    21,     0,    19,    19,    18,
     0,    18,    19,    19,     0,    14,    19,    19,     0,    14,
    19,    19,    21,     0,    19,    19,    14,     0,    19,    19,
    14,    21,     0,    18,    19,    19,     0,    18,    19,    19,
    21,     0,    20,    21,     0,    20,    18,    21,     0,    20,
    18,     0,    14,    20,    21,     0,    20,    14,     0,    20,
    14,    21,     0,    24,     0,    19,    24,     0,     9,    96,
     0,    10,    96,     0,    11,    96,     0,    12,    96,     0,
    14,    91,     0,    14,     0,    18,    91,     0,    18,     0,
    16,    96,    41,    89,    42,     0,    83,    90,     0,    90,
    83,     0,     7,     0,    21,     0,    19,     0,    20,     0,
    89,     0,    92,    30,    89,     0,    90,     0,    93,    84,
    85,    84,     0,    22,    23,     0,    23,    22,     0,    94,
     0,    22,     0,    23,     0,     6,     0,    60,     0,     7,
     0,     8,     0,     6,     0,    60,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   231,   232,   235,   242,   243,   248,   252,   256,   260,   264,
   268,   272,   276,   280,   284,   288,   294,   302,   306,   312,
   320,   324,   328,   332,   338,   342,   345,   349,   353,   356,
   363,   369,   375,   381,   385,   389,   393,   397,   401,   405,
   409,   413,   417,   421,   425,   429,   433,   437,   441,   445,
   449,   453,   457,   461,   465,   471,   478,   489,   496,   499,
   503,   511,   536,   541,   548,   557,   565,   571,   582,   598,
   611,   635,   636,   670,   727,   733,   734,   737,   740,   741,
   744,   746,   749,   751,   753,   755,   757,   760,   762,   767,
   774,   776,   780,   782,   786,   788,   800,   801,   806,   808,
   810,   812,   814,   816,   818,   820,   822,   824,   826,   828,
   830,   832,   834,   836,   838,   840,   842,   844,   846,   848,
   850,   852,   854,   856,   858,   860,   862,   864,   866,   869,
   872,   875,   878,   880,   882,   884,   889,   893,   895,   899,
   900,   906,   912,   921,   926,   933,   934,   938,   939,   942,
   946,   948,   952,   953,   954,   955,   958,   959
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","INT","FLOAT",
"STRING","NAME","TYPENAME","NAME_OR_INT","STRUCT","CLASS","UNION","ENUM","SIZEOF",
"UNSIGNED","COLONCOLON","TEMPLATE","ERROR","SIGNED_KEYWORD","LONG","SHORT","INT_KEYWORD",
"CONST_KEYWORD","VOLATILE_KEYWORD","DOUBLE_KEYWORD","VARIABLE","ASSIGN_MODIFY",
"THIS","TRUEKEYWORD","FALSEKEYWORD","','","ABOVE_COMMA","'='","'?'","OROR","ANDAND",
"'|'","'^'","'&'","EQUAL","NOTEQUAL","'<'","'>'","LEQ","GEQ","LSH","RSH","'@'",
"'+'","'-'","'*'","'/'","'%'","UNARY","INCREMENT","DECREMENT","ARROW","'.'",
"'['","'('","BLOCKNAME","FILENAME","'!'","'~'","']'","')'","'{'","'}'","':'",
"start","type_exp","exp1","exp","@1","lcurly","arglist","rcurly","block","variable",
"qualified_name","space_identifier","const_or_volatile","cv_with_space_id","const_or_volatile_or_space_identifier_noopt",
"const_or_volatile_or_space_identifier","abs_decl","direct_abs_decl","array_mod",
"func_mod","type","typebase","typename","nonempty_typelist","ptype","const_and_volatile",
"const_or_volatile_noopt","name","name_not_typename", NULL
};
#endif

static const short yyr1[] = {     0,
    69,    69,    70,    71,    71,    72,    72,    72,    72,    72,
    72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
    72,    72,    73,    72,    74,    75,    75,    75,    76,    72,
    72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
    72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
    72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
    72,    72,    72,    72,    72,    77,    77,    77,    78,    79,
    79,    78,    78,    78,    80,    81,    81,    82,    83,    83,
    84,    84,    85,    85,    85,    85,    85,    86,    86,    86,
    86,    86,    87,    87,    88,    88,    89,    89,    90,    90,
    90,    90,    90,    90,    90,    90,    90,    90,    90,    90,
    90,    90,    90,    90,    90,    90,    90,    90,    90,    90,
    90,    90,    90,    90,    90,    90,    90,    90,    90,    90,
    90,    90,    90,    90,    90,    90,    90,    90,    90,    91,
    91,    91,    91,    92,    92,    93,    93,    94,    94,    95,
    95,    95,    96,    96,    96,    96,    97,    97
};

static const short yyr2[] = {     0,
     1,     1,     1,     1,     3,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     3,     3,     4,     3,     3,
     4,     4,     0,     5,     1,     0,     1,     3,     1,     3,
     4,     4,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     5,     3,     3,     1,     1,     1,     1,     1,
     4,     1,     1,     1,     1,     1,     1,     3,     3,     3,
     4,     1,     2,     1,     2,     1,     0,     3,     1,     1,
     1,     0,     1,     2,     1,     2,     1,     3,     2,     1,
     2,     1,     2,     3,     2,     3,     1,     3,     1,     1,
     1,     1,     2,     3,     2,     3,     3,     3,     2,     2,
     3,     4,     3,     3,     3,     4,     3,     4,     3,     4,
     2,     3,     2,     3,     2,     3,     1,     2,     2,     2,
     2,     2,     2,     1,     2,     1,     5,     2,     2,     1,
     1,     1,     1,     1,     3,     1,     4,     2,     2,     1,
     1,     1,     1,     1,     1,     1,     1,     1
};

static const short yydefact[] = {    77,
    56,    58,    62,   157,    99,    57,     0,     0,     0,     0,
    77,   134,     0,     0,   136,   101,   102,   100,   151,   152,
   127,    60,    63,    64,    65,    77,    77,    77,    77,    77,
    77,   158,    67,    77,    77,    25,     2,     1,     4,    26,
     0,    59,    72,     0,    79,    77,     3,   146,    97,   150,
    80,    74,   153,   155,   156,   154,   129,   130,   131,   132,
    77,    15,    77,   140,   142,   143,   141,   133,    73,     0,
   142,   143,   135,   109,   105,   110,   103,   128,   125,   123,
   121,   148,   149,     7,     8,     6,    11,    12,     0,     0,
     9,    10,    77,    77,    77,    77,    77,    77,    77,    77,
    77,    77,    77,    77,    77,    77,    77,    77,    77,    77,
    77,    77,    77,    77,    77,    13,    14,    77,    77,    77,
    23,    27,     0,     0,     0,     0,    77,   138,     0,   139,
    81,     0,     0,     0,   115,   107,   124,    77,   114,   106,
   108,   104,   117,   113,   111,   126,   122,    33,    77,     5,
    55,    54,     0,    52,    51,    50,    49,    48,    42,    43,
    46,    47,    44,    45,    40,    41,    34,    38,    39,    35,
    36,    37,   155,    77,    17,    16,    77,    20,    19,     0,
    26,    77,    29,    30,    77,    69,    75,    78,    76,    98,
     0,    70,    85,    83,     0,    77,    82,    87,    90,    92,
    61,   116,     0,   146,   120,   118,   112,    32,    77,    18,
    21,    22,     0,    28,    31,    71,    86,    84,     0,    93,
    95,     0,   144,     0,   147,    77,    89,    91,   137,     0,
    53,    24,    94,    88,    77,    96,   145,     0,     0,     0
};

static const short yydefgoto[] = {   238,
    37,    89,    39,   181,    40,   123,   184,    41,    42,    43,
   127,    44,    45,    46,   132,   197,   198,   199,   200,   223,
    63,    68,   224,    49,    50,    51,   192,    52
};

static const short yypact[] = {   264,
-32768,-32768,-32768,-32768,-32768,-32768,   112,   112,   112,   112,
   328,   127,   112,   112,   142,   162,   150,-32768,     6,    21,
-32768,-32768,-32768,-32768,-32768,   264,   264,   264,   264,   264,
   264,    39,-32768,   264,   264,-32768,-32768,    34,   563,   200,
    43,-32768,-32768,    26,-32768,   287,-32768,    22,    -8,-32768,
    75,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
   264,   185,   169,-32768,    17,    60,-32768,-32768,-32768,    89,
    28,-32768,-32768,   120,   131,   179,-32768,-32768,   134,   136,
-32768,-32768,-32768,   185,   185,   185,   185,   185,   -17,    94,
   185,   185,   264,   264,   264,   264,   264,   264,   264,   264,
   264,   264,   264,   264,   264,   264,   264,   264,   264,   264,
   264,   264,   264,   264,   264,-32768,-32768,   478,   497,   264,
-32768,   563,   -14,   102,   112,   167,    92,   103,    77,-32768,
-32768,    95,   114,    49,   156,-32768,-32768,   287,   161,-32768,
-32768,-32768,   164,   174,-32768,-32768,-32768,-32768,   264,   563,
   563,   563,   526,   615,   639,   662,   684,   705,   724,   724,
   313,   313,   313,   313,   415,   415,   475,   485,   485,   185,
   185,   185,   109,   264,-32768,-32768,   264,-32768,-32768,   -12,
   200,   264,-32768,-32768,   264,   175,-32768,-32768,-32768,-32768,
   112,-32768,   -24,    85,     7,   418,   103,   107,-32768,-32768,
   392,-32768,   157,   113,-32768,-32768,-32768,   185,   264,   185,
   185,-32768,    -6,   563,   185,-32768,-32768,-32768,   137,-32768,
-32768,   133,-32768,    -5,-32768,    56,-32768,-32768,-32768,   180,
   590,-32768,-32768,-32768,   287,-32768,-32768,   217,   231,-32768
};

static const short yypgoto[] = {-32768,
-32768,     9,    -7,-32768,-32768,    51,   110,-32768,-32768,    69,
-32768,   106,-32768,   -46,    40,   -83,-32768,    47,    48,     1,
     0,   233,-32768,-32768,-32768,   124,    -2,-32768
};


#define	YYLAST		783


static const short yytable[] = {    48,
    47,   130,   131,    62,    57,    58,    59,    60,    38,   219,
    69,    70,    93,    19,    20,   182,   130,    93,    84,    85,
    86,    87,    88,   182,   235,   194,    91,    92,    82,   -82,
    48,    90,   122,   195,   196,   135,   129,   136,   -77,    48,
   124,   -82,    83,    19,    20,   128,   139,   148,   140,   -82,
   -82,   212,   183,   -66,    53,    54,    55,   125,   232,   236,
    48,   133,     5,    93,     7,     8,     9,    10,   -77,    12,
   220,    14,   126,    15,    16,    17,    18,    19,    20,    21,
   137,   130,    53,    54,    55,   150,   151,   152,   153,   154,
   155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
   165,   166,   167,   168,   169,   170,   171,   172,    56,   217,
   218,   191,   222,    19,    20,   176,   179,    53,    54,    55,
   221,   -76,   186,   -99,    19,    20,   190,   230,   180,   138,
   -99,   -99,   193,    64,    19,    20,    56,   204,   203,   191,
   141,   208,   195,   196,   194,    65,    66,    67,    64,   -77,
   131,   142,   195,   196,   146,   -99,   147,   130,   149,   -77,
    71,    72,    67,    79,   195,   226,   210,    80,   183,   211,
    81,    56,   187,   122,   214,    74,   202,   215,   201,    75,
    76,   205,    77,   134,   206,    78,   175,   178,   216,   -68,
    19,    20,   143,   208,   207,   204,   144,   234,   229,   145,
   233,   231,     1,     2,     3,     4,     5,     6,     7,     8,
     9,    10,    11,    12,    13,    14,   239,    15,    16,    17,
    18,    19,    20,    21,    22,   204,    23,    24,    25,   190,
   240,   213,   188,   185,   204,   237,   225,    26,   116,   117,
   118,   119,   120,   121,   227,   228,   -77,    73,    27,    28,
   189,     0,     0,    29,    30,     0,     0,     0,    31,    32,
    33,    34,    35,     0,     0,    36,     1,     2,     3,     4,
     5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
     0,    15,    16,    17,    18,    19,    20,    21,    22,     0,
    23,    24,    25,     5,     0,     7,     8,     9,    10,     0,
    12,    26,    14,     0,    15,    16,    17,    18,    19,    20,
    21,     0,    27,    28,     0,     0,     0,    29,    30,     0,
     0,     0,    31,    32,    33,    34,    35,     0,     0,    36,
     1,     2,     3,     4,     5,     6,     7,     8,     9,    10,
    11,    12,    13,    14,     0,    15,    16,    17,    18,    19,
    20,    21,    22,     0,    23,    24,    25,   108,   109,   110,
   111,   112,   113,   114,   115,    26,   116,   117,   118,   119,
   120,   121,     0,     0,     0,     0,    27,    28,     0,     0,
     0,    29,    30,     0,     0,     0,    61,    32,    33,    34,
    35,     0,     0,    36,     1,     2,     3,     4,     5,     6,
     7,     8,     9,    10,    11,    12,    13,    14,     0,    15,
    16,    17,    18,    19,    20,    21,    22,     0,    23,    24,
    25,     0,     0,     0,     5,     0,     7,     8,     9,    10,
     0,    12,     0,    14,     0,    15,    16,    17,    18,    19,
    20,    21,     0,     0,     0,    29,    30,     0,     0,     0,
    31,    32,    33,    34,    35,   193,     0,    36,     0,     0,
     0,   110,   111,   112,   113,   114,   115,   194,   116,   117,
   118,   119,   120,   121,     0,   195,   196,     0,     0,     0,
     0,     0,   221,    53,   173,    55,     7,     8,     9,    10,
     0,    12,     0,    14,     0,    15,    16,    17,    18,    19,
    20,    21,    53,   173,    55,     7,     8,     9,    10,     0,
    12,     0,    14,     0,    15,    16,    17,    18,    19,    20,
    21,     0,   111,   112,   113,   114,   115,   174,   116,   117,
   118,   119,   120,   121,   113,   114,   115,    56,   116,   117,
   118,   119,   120,   121,     0,     0,   177,     0,     0,     0,
     0,    94,     0,     0,     0,     0,    56,    95,    96,    97,
    98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
   108,   109,   110,   111,   112,   113,   114,   115,     0,   116,
   117,   118,   119,   120,   121,     0,     0,     0,    94,     0,
     0,     0,     0,   209,    95,    96,    97,    98,    99,   100,
   101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
   111,   112,   113,   114,   115,     0,   116,   117,   118,   119,
   120,   121,    96,    97,    98,    99,   100,   101,   102,   103,
   104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
   114,   115,     0,   116,   117,   118,   119,   120,   121,    98,
    99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
   109,   110,   111,   112,   113,   114,   115,     0,   116,   117,
   118,   119,   120,   121,    99,   100,   101,   102,   103,   104,
   105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
   115,     0,   116,   117,   118,   119,   120,   121,   100,   101,
   102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
   112,   113,   114,   115,     0,   116,   117,   118,   119,   120,
   121,   101,   102,   103,   104,   105,   106,   107,   108,   109,
   110,   111,   112,   113,   114,   115,     0,   116,   117,   118,
   119,   120,   121,   102,   103,   104,   105,   106,   107,   108,
   109,   110,   111,   112,   113,   114,   115,     0,   116,   117,
   118,   119,   120,   121,   104,   105,   106,   107,   108,   109,
   110,   111,   112,   113,   114,   115,     0,   116,   117,   118,
   119,   120,   121
};

static const short yycheck[] = {     0,
     0,    48,    49,    11,     7,     8,     9,    10,     0,     3,
    13,    14,    30,    22,    23,    30,    63,    30,    26,    27,
    28,    29,    30,    30,    30,    50,    34,    35,    23,    38,
    31,    31,    40,    58,    59,    19,    15,    21,    47,    40,
    40,    50,    22,    22,    23,    46,    19,    65,    21,    58,
    59,    64,    67,    15,     6,     7,     8,    15,    65,    65,
    61,    61,     7,    30,     9,    10,    11,    12,    47,    14,
    64,    16,    47,    18,    19,    20,    21,    22,    23,    24,
    21,   128,     6,     7,     8,    93,    94,    95,    96,    97,
    98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
   108,   109,   110,   111,   112,   113,   114,   115,    60,   193,
   194,    63,   196,    22,    23,   118,   119,     6,     7,     8,
    65,    47,   125,    15,    22,    23,    50,    15,   120,    41,
    22,    23,    38,     7,    22,    23,    60,   138,   138,    63,
    21,   149,    58,    59,    50,    19,    20,    21,     7,    47,
   197,    21,    58,    59,    21,    47,    21,   204,    65,    47,
    19,    20,    21,    14,    58,    59,   174,    18,    67,   177,
    21,    60,     6,   181,   182,    14,    21,   185,    65,    18,
    19,    21,    21,    15,    21,    24,   118,   119,   191,    15,
    22,    23,    14,   201,    21,   196,    18,    65,    42,    21,
    64,   209,     3,     4,     5,     6,     7,     8,     9,    10,
    11,    12,    13,    14,    15,    16,     0,    18,    19,    20,
    21,    22,    23,    24,    25,   226,    27,    28,    29,    50,
     0,   181,   127,   124,   235,   235,   197,    38,    54,    55,
    56,    57,    58,    59,   198,   198,    47,    15,    49,    50,
   127,    -1,    -1,    54,    55,    -1,    -1,    -1,    59,    60,
    61,    62,    63,    -1,    -1,    66,     3,     4,     5,     6,
     7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
    -1,    18,    19,    20,    21,    22,    23,    24,    25,    -1,
    27,    28,    29,     7,    -1,     9,    10,    11,    12,    -1,
    14,    38,    16,    -1,    18,    19,    20,    21,    22,    23,
    24,    -1,    49,    50,    -1,    -1,    -1,    54,    55,    -1,
    -1,    -1,    59,    60,    61,    62,    63,    -1,    -1,    66,
     3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
    13,    14,    15,    16,    -1,    18,    19,    20,    21,    22,
    23,    24,    25,    -1,    27,    28,    29,    45,    46,    47,
    48,    49,    50,    51,    52,    38,    54,    55,    56,    57,
    58,    59,    -1,    -1,    -1,    -1,    49,    50,    -1,    -1,
    -1,    54,    55,    -1,    -1,    -1,    59,    60,    61,    62,
    63,    -1,    -1,    66,     3,     4,     5,     6,     7,     8,
     9,    10,    11,    12,    13,    14,    15,    16,    -1,    18,
    19,    20,    21,    22,    23,    24,    25,    -1,    27,    28,
    29,    -1,    -1,    -1,     7,    -1,     9,    10,    11,    12,
    -1,    14,    -1,    16,    -1,    18,    19,    20,    21,    22,
    23,    24,    -1,    -1,    -1,    54,    55,    -1,    -1,    -1,
    59,    60,    61,    62,    63,    38,    -1,    66,    -1,    -1,
    -1,    47,    48,    49,    50,    51,    52,    50,    54,    55,
    56,    57,    58,    59,    -1,    58,    59,    -1,    -1,    -1,
    -1,    -1,    65,     6,     7,     8,     9,    10,    11,    12,
    -1,    14,    -1,    16,    -1,    18,    19,    20,    21,    22,
    23,    24,     6,     7,     8,     9,    10,    11,    12,    -1,
    14,    -1,    16,    -1,    18,    19,    20,    21,    22,    23,
    24,    -1,    48,    49,    50,    51,    52,    50,    54,    55,
    56,    57,    58,    59,    50,    51,    52,    60,    54,    55,
    56,    57,    58,    59,    -1,    -1,    50,    -1,    -1,    -1,
    -1,    26,    -1,    -1,    -1,    -1,    60,    32,    33,    34,
    35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
    45,    46,    47,    48,    49,    50,    51,    52,    -1,    54,
    55,    56,    57,    58,    59,    -1,    -1,    -1,    26,    -1,
    -1,    -1,    -1,    68,    32,    33,    34,    35,    36,    37,
    38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
    48,    49,    50,    51,    52,    -1,    54,    55,    56,    57,
    58,    59,    33,    34,    35,    36,    37,    38,    39,    40,
    41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
    51,    52,    -1,    54,    55,    56,    57,    58,    59,    35,
    36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
    46,    47,    48,    49,    50,    51,    52,    -1,    54,    55,
    56,    57,    58,    59,    36,    37,    38,    39,    40,    41,
    42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
    52,    -1,    54,    55,    56,    57,    58,    59,    37,    38,
    39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
    49,    50,    51,    52,    -1,    54,    55,    56,    57,    58,
    59,    38,    39,    40,    41,    42,    43,    44,    45,    46,
    47,    48,    49,    50,    51,    52,    -1,    54,    55,    56,
    57,    58,    59,    39,    40,    41,    42,    43,    44,    45,
    46,    47,    48,    49,    50,    51,    52,    -1,    54,    55,
    56,    57,    58,    59,    41,    42,    43,    44,    45,    46,
    47,    48,    49,    50,    51,    52,    -1,    54,    55,    56,
    57,    58,    59
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/lib/bison.simple"
/* This file comes from bison-1.27.  */

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

#ifndef YYSTACK_USE_ALLOCA
#ifdef alloca
#define YYSTACK_USE_ALLOCA
#else /* alloca not defined */
#ifdef __GNUC__
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi) || (defined (__sun) && defined (__i386))
#define YYSTACK_USE_ALLOCA
#include <alloca.h>
#else /* not sparc */
/* We think this test detects Watcom and Microsoft C.  */
/* This used to test MSDOS, but that is a bad idea
   since that symbol is in the user namespace.  */
#if (defined (_MSDOS) || defined (_MSDOS_)) && !defined (__TURBOC__)
#if 0 /* No need for xmalloc.h, which pollutes the namespace;
	 instead, just don't use alloca.  */
#endif
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
/* I don't know what this was needed for, but it pollutes the namespace.
   So I turned it off.   rms, 2 May 1997.  */
 #pragma alloca
#define YYSTACK_USE_ALLOCA
#else /* not MSDOS, or __TURBOC__, or _AIX */
#if 0
#ifdef __hpux /* haible@ilog.fr says this works for HPUX 9.05 and up,
		 and on HPUX 10.  Eventually we can turn this on.  */
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#endif /* __hpux */
#endif
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc */
#endif /* not GNU C */
#endif /* alloca not defined */
#endif /* YYSTACK_USE_ALLOCA not defined */

#ifdef YYSTACK_USE_ALLOCA
#define YYSTACK_ALLOC alloca
#else
#define YYSTACK_ALLOC xmalloc
#endif

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Define __yy_memcpy.  Note that the size argument
   should be passed with type unsigned int, because that is what the non-GCC
   definitions require.  With GCC, __builtin_memcpy takes an arg
   of type size_t, but it can handle unsigned int.  */

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (to, from, count)
     char *to;
     char *from;
     unsigned int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *to, char *from, unsigned int count)
{
  register char *t = to;
  register char *f = from;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 216 "/usr/lib/bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#ifdef __cplusplus
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else /* not __cplusplus */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif /* not __cplusplus */
#else /* not YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* not YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
#ifdef YYPARSE_PARAM
int yyparse (void *);
#else
int yyparse (void);
#endif
#endif

int
yyparse(YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to xreallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;
  int yyfree_stacks = 0;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to xreallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  if (yyfree_stacks)
	    {
	      free (yyss);
	      free (yyvs);
#ifdef YYLSP_NEEDED
	      free (yyls);
#endif
	    }
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
#ifndef YYSTACK_USE_ALLOCA
      yyfree_stacks = 1;
#endif
      yyss = (short *) YYSTACK_ALLOC (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss, (char *)yyss1,
		   size * (unsigned int) sizeof (*yyssp));
      yyvs = (YYSTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs, (char *)yyvs1,
		   size * (unsigned int) sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls, (char *)yyls1,
		   size * (unsigned int) sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 3:
#line 236 "c-exp.y"
{ write_exp_elt_opcode(OP_TYPE);
			  write_exp_elt_type(yyvsp[0].tval);
			  write_exp_elt_opcode(OP_TYPE);;
    break;}
case 5:
#line 244 "c-exp.y"
{ write_exp_elt_opcode (BINOP_COMMA); ;
    break;}
case 6:
#line 249 "c-exp.y"
{ write_exp_elt_opcode (UNOP_IND); ;
    break;}
case 7:
#line 253 "c-exp.y"
{ write_exp_elt_opcode (UNOP_ADDR); ;
    break;}
case 8:
#line 257 "c-exp.y"
{ write_exp_elt_opcode (UNOP_NEG); ;
    break;}
case 9:
#line 261 "c-exp.y"
{ write_exp_elt_opcode (UNOP_LOGICAL_NOT); ;
    break;}
case 10:
#line 265 "c-exp.y"
{ write_exp_elt_opcode (UNOP_COMPLEMENT); ;
    break;}
case 11:
#line 269 "c-exp.y"
{ write_exp_elt_opcode (UNOP_PREINCREMENT); ;
    break;}
case 12:
#line 273 "c-exp.y"
{ write_exp_elt_opcode (UNOP_PREDECREMENT); ;
    break;}
case 13:
#line 277 "c-exp.y"
{ write_exp_elt_opcode (UNOP_POSTINCREMENT); ;
    break;}
case 14:
#line 281 "c-exp.y"
{ write_exp_elt_opcode (UNOP_POSTDECREMENT); ;
    break;}
case 15:
#line 285 "c-exp.y"
{ write_exp_elt_opcode (UNOP_SIZEOF); ;
    break;}
case 16:
#line 289 "c-exp.y"
{ write_exp_elt_opcode (STRUCTOP_PTR);
			  write_exp_string (yyvsp[0].sval);
			  write_exp_elt_opcode (STRUCTOP_PTR); ;
    break;}
case 17:
#line 295 "c-exp.y"
{ /* exp->type::name becomes exp->*(&type::name) */
			  /* Note: this doesn't work if name is a
			     static member!  FIXME */
			  write_exp_elt_opcode (UNOP_ADDR);
			  write_exp_elt_opcode (STRUCTOP_MPTR); ;
    break;}
case 18:
#line 303 "c-exp.y"
{ write_exp_elt_opcode (STRUCTOP_MPTR); ;
    break;}
case 19:
#line 307 "c-exp.y"
{ write_exp_elt_opcode (STRUCTOP_STRUCT);
			  write_exp_string (yyvsp[0].sval);
			  write_exp_elt_opcode (STRUCTOP_STRUCT); ;
    break;}
case 20:
#line 313 "c-exp.y"
{ /* exp.type::name becomes exp.*(&type::name) */
			  /* Note: this doesn't work if name is a
			     static member!  FIXME */
			  write_exp_elt_opcode (UNOP_ADDR);
			  write_exp_elt_opcode (STRUCTOP_MEMBER); ;
    break;}
case 21:
#line 321 "c-exp.y"
{ write_exp_elt_opcode (STRUCTOP_MEMBER); ;
    break;}
case 22:
#line 325 "c-exp.y"
{ write_exp_elt_opcode (BINOP_SUBSCRIPT); ;
    break;}
case 23:
#line 331 "c-exp.y"
{ start_arglist (); ;
    break;}
case 24:
#line 333 "c-exp.y"
{ write_exp_elt_opcode (OP_FUNCALL);
			  write_exp_elt_longcst ((LONGEST) end_arglist ());
			  write_exp_elt_opcode (OP_FUNCALL); ;
    break;}
case 25:
#line 339 "c-exp.y"
{ start_arglist (); ;
    break;}
case 27:
#line 346 "c-exp.y"
{ arglist_len = 1; ;
    break;}
case 28:
#line 350 "c-exp.y"
{ arglist_len++; ;
    break;}
case 29:
#line 354 "c-exp.y"
{ yyval.lval = end_arglist () - 1; ;
    break;}
case 30:
#line 357 "c-exp.y"
{ write_exp_elt_opcode (OP_ARRAY);
			  write_exp_elt_longcst ((LONGEST) 0);
			  write_exp_elt_longcst ((LONGEST) yyvsp[0].lval);
			  write_exp_elt_opcode (OP_ARRAY); ;
    break;}
case 31:
#line 364 "c-exp.y"
{ write_exp_elt_opcode (UNOP_MEMVAL);
			  write_exp_elt_type (yyvsp[-2].tval);
			  write_exp_elt_opcode (UNOP_MEMVAL); ;
    break;}
case 32:
#line 370 "c-exp.y"
{ write_exp_elt_opcode (UNOP_CAST);
			  write_exp_elt_type (yyvsp[-2].tval);
			  write_exp_elt_opcode (UNOP_CAST); ;
    break;}
case 33:
#line 376 "c-exp.y"
{ ;
    break;}
case 34:
#line 382 "c-exp.y"
{ write_exp_elt_opcode (BINOP_REPEAT); ;
    break;}
case 35:
#line 386 "c-exp.y"
{ write_exp_elt_opcode (BINOP_MUL); ;
    break;}
case 36:
#line 390 "c-exp.y"
{ write_exp_elt_opcode (BINOP_DIV); ;
    break;}
case 37:
#line 394 "c-exp.y"
{ write_exp_elt_opcode (BINOP_REM); ;
    break;}
case 38:
#line 398 "c-exp.y"
{ write_exp_elt_opcode (BINOP_ADD); ;
    break;}
case 39:
#line 402 "c-exp.y"
{ write_exp_elt_opcode (BINOP_SUB); ;
    break;}
case 40:
#line 406 "c-exp.y"
{ write_exp_elt_opcode (BINOP_LSH); ;
    break;}
case 41:
#line 410 "c-exp.y"
{ write_exp_elt_opcode (BINOP_RSH); ;
    break;}
case 42:
#line 414 "c-exp.y"
{ write_exp_elt_opcode (BINOP_EQUAL); ;
    break;}
case 43:
#line 418 "c-exp.y"
{ write_exp_elt_opcode (BINOP_NOTEQUAL); ;
    break;}
case 44:
#line 422 "c-exp.y"
{ write_exp_elt_opcode (BINOP_LEQ); ;
    break;}
case 45:
#line 426 "c-exp.y"
{ write_exp_elt_opcode (BINOP_GEQ); ;
    break;}
case 46:
#line 430 "c-exp.y"
{ write_exp_elt_opcode (BINOP_LESS); ;
    break;}
case 47:
#line 434 "c-exp.y"
{ write_exp_elt_opcode (BINOP_GTR); ;
    break;}
case 48:
#line 438 "c-exp.y"
{ write_exp_elt_opcode (BINOP_BITWISE_AND); ;
    break;}
case 49:
#line 442 "c-exp.y"
{ write_exp_elt_opcode (BINOP_BITWISE_XOR); ;
    break;}
case 50:
#line 446 "c-exp.y"
{ write_exp_elt_opcode (BINOP_BITWISE_IOR); ;
    break;}
case 51:
#line 450 "c-exp.y"
{ write_exp_elt_opcode (BINOP_LOGICAL_AND); ;
    break;}
case 52:
#line 454 "c-exp.y"
{ write_exp_elt_opcode (BINOP_LOGICAL_OR); ;
    break;}
case 53:
#line 458 "c-exp.y"
{ write_exp_elt_opcode (TERNOP_COND); ;
    break;}
case 54:
#line 462 "c-exp.y"
{ write_exp_elt_opcode (BINOP_ASSIGN); ;
    break;}
case 55:
#line 466 "c-exp.y"
{ write_exp_elt_opcode (BINOP_ASSIGN_MODIFY);
			  write_exp_elt_opcode (yyvsp[-1].opcode);
			  write_exp_elt_opcode (BINOP_ASSIGN_MODIFY); ;
    break;}
case 56:
#line 472 "c-exp.y"
{ write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type (yyvsp[0].typed_val_int.type);
			  write_exp_elt_longcst ((LONGEST)(yyvsp[0].typed_val_int.val));
			  write_exp_elt_opcode (OP_LONG); ;
    break;}
case 57:
#line 479 "c-exp.y"
{ YYSTYPE val;
			  parse_number (yyvsp[0].ssym.stoken.ptr, yyvsp[0].ssym.stoken.length, 0, &val);
			  write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type (val.typed_val_int.type);
			  write_exp_elt_longcst ((LONGEST)val.typed_val_int.val);
			  write_exp_elt_opcode (OP_LONG);
			;
    break;}
case 58:
#line 490 "c-exp.y"
{ write_exp_elt_opcode (OP_DOUBLE);
			  write_exp_elt_type (yyvsp[0].typed_val_float.type);
			  write_exp_elt_dblcst (yyvsp[0].typed_val_float.dval);
			  write_exp_elt_opcode (OP_DOUBLE); ;
    break;}
case 61:
#line 504 "c-exp.y"
{ write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type (builtin_type_int);
			  CHECK_TYPEDEF (yyvsp[-1].tval);
			  write_exp_elt_longcst ((LONGEST) TYPE_LENGTH (yyvsp[-1].tval));
			  write_exp_elt_opcode (OP_LONG); ;
    break;}
case 62:
#line 512 "c-exp.y"
{ /* C strings are converted into array constants with
			     an explicit null byte added at the end.  Thus
			     the array upper bound is the string length.
			     There is no such thing in C as a completely empty
			     string. */
			  char *sp = yyvsp[0].sval.ptr; int count = yyvsp[0].sval.length;
			  while (count-- > 0)
			    {
			      write_exp_elt_opcode (OP_LONG);
			      write_exp_elt_type (builtin_type_char);
			      write_exp_elt_longcst ((LONGEST)(*sp++));
			      write_exp_elt_opcode (OP_LONG);
			    }
			  write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type (builtin_type_char);
			  write_exp_elt_longcst ((LONGEST)'\0');
			  write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_opcode (OP_ARRAY);
			  write_exp_elt_longcst ((LONGEST) 0);
			  write_exp_elt_longcst ((LONGEST) (yyvsp[0].sval.length));
			  write_exp_elt_opcode (OP_ARRAY); ;
    break;}
case 63:
#line 537 "c-exp.y"
{ write_exp_elt_opcode (OP_THIS);
			  write_exp_elt_opcode (OP_THIS); ;
    break;}
case 64:
#line 542 "c-exp.y"
{ write_exp_elt_opcode (OP_LONG);
                          write_exp_elt_type (builtin_type_bool);
                          write_exp_elt_longcst ((LONGEST) 1);
                          write_exp_elt_opcode (OP_LONG); ;
    break;}
case 65:
#line 549 "c-exp.y"
{ write_exp_elt_opcode (OP_LONG);
                          write_exp_elt_type (builtin_type_bool);
                          write_exp_elt_longcst ((LONGEST) 0);
                          write_exp_elt_opcode (OP_LONG); ;
    break;}
case 66:
#line 558 "c-exp.y"
{
			  if (yyvsp[0].ssym.sym)
			    yyval.bval = SYMBOL_BLOCK_VALUE (yyvsp[0].ssym.sym);
			  else
			    error ("No file or function \"%s\".",
				   copy_name (yyvsp[0].ssym.stoken));
			;
    break;}
case 67:
#line 566 "c-exp.y"
{
			  yyval.bval = yyvsp[0].bval;
			;
    break;}
case 68:
#line 572 "c-exp.y"
{ struct symbol *tem
			    = lookup_symbol (copy_name (yyvsp[0].sval), yyvsp[-2].bval,
					     VAR_NAMESPACE, (int *) NULL,
					     (struct symtab **) NULL);
			  if (!tem || SYMBOL_CLASS (tem) != LOC_BLOCK)
			    error ("No function \"%s\" in specified context.",
				   copy_name (yyvsp[0].sval));
			  yyval.bval = SYMBOL_BLOCK_VALUE (tem); ;
    break;}
case 69:
#line 583 "c-exp.y"
{ struct symbol *sym;
			  sym = lookup_symbol (copy_name (yyvsp[0].sval), yyvsp[-2].bval,
					       VAR_NAMESPACE, (int *) NULL,
					       (struct symtab **) NULL);
			  if (sym == 0)
			    error ("No symbol \"%s\" in specified context.",
				   copy_name (yyvsp[0].sval));

			  write_exp_elt_opcode (OP_VAR_VALUE);
			  /* block_found is set by lookup_symbol.  */
			  write_exp_elt_block (block_found);
			  write_exp_elt_sym (sym);
			  write_exp_elt_opcode (OP_VAR_VALUE); ;
    break;}
case 70:
#line 599 "c-exp.y"
{
			  struct type *type = yyvsp[-2].tval;
			  if (TYPE_CODE (type) != TYPE_CODE_STRUCT
			      && TYPE_CODE (type) != TYPE_CODE_UNION)
			    error ("`%s' is not defined as an aggregate type.",
				   TYPE_NAME (type));

			  write_exp_elt_opcode (OP_SCOPE);
			  write_exp_elt_type (type);
			  write_exp_string (yyvsp[0].sval);
			  write_exp_elt_opcode (OP_SCOPE);
			;
    break;}
case 71:
#line 612 "c-exp.y"
{
			  struct type *type = yyvsp[-3].tval;
			  struct stoken tmp_token;
			  if (TYPE_CODE (type) != TYPE_CODE_STRUCT
			      && TYPE_CODE (type) != TYPE_CODE_UNION)
			    error ("`%s' is not defined as an aggregate type.",
				   TYPE_NAME (type));

			  tmp_token.ptr = (char*) alloca (yyvsp[0].sval.length + 2);
			  tmp_token.length = yyvsp[0].sval.length + 1;
			  tmp_token.ptr[0] = '~';
			  memcpy (tmp_token.ptr+1, yyvsp[0].sval.ptr, yyvsp[0].sval.length);
			  tmp_token.ptr[tmp_token.length] = 0;

			  /* Check for valid destructor name.  */
			  destructor_name_p (tmp_token.ptr, type);
			  write_exp_elt_opcode (OP_SCOPE);
			  write_exp_elt_type (type);
			  write_exp_string (tmp_token);
			  write_exp_elt_opcode (OP_SCOPE);
			;
    break;}
case 73:
#line 637 "c-exp.y"
{
			  char *name = copy_name (yyvsp[0].sval);
			  struct symbol *sym;
			  struct minimal_symbol *msymbol;

			  sym =
			    lookup_symbol (name, (const struct block *) NULL,
					   VAR_NAMESPACE, (int *) NULL,
					   (struct symtab **) NULL);
			  if (sym)
			    {
			      write_exp_elt_opcode (OP_VAR_VALUE);
			      write_exp_elt_block (NULL);
			      write_exp_elt_sym (sym);
			      write_exp_elt_opcode (OP_VAR_VALUE);
			      break;
			    }

			  msymbol = lookup_minimal_symbol (name, NULL, NULL);
			  if (msymbol != NULL)
			    {
			      write_exp_msymbol (msymbol,
						 lookup_function_type (builtin_type_int),
						 builtin_type_int);
			    }
			  else
			    if (!have_full_symbols () && !have_partial_symbols ())
			      error ("No symbol table is loaded.  Use the \"file\" command.");
			    else
			      error ("No symbol \"%s\" in current context.", name);
			;
    break;}
case 74:
#line 671 "c-exp.y"
{ struct symbol *sym = yyvsp[0].ssym.sym;

			  if (sym)
			    {
			      if (symbol_read_needs_frame (sym))
				{
				  if (innermost_block == 0 ||
				      contained_in (block_found, 
						    innermost_block))
				    innermost_block = block_found;
				}

			      write_exp_elt_opcode (OP_VAR_VALUE);
			      /* We want to use the selected frame, not
				 another more inner frame which happens to
				 be in the same block.  */
			      write_exp_elt_block (NULL);
			      write_exp_elt_sym (sym);
			      write_exp_elt_opcode (OP_VAR_VALUE);
			    }
			  else if (yyvsp[0].ssym.is_a_field_of_this)
			    {
			      /* C++: it hangs off of `this'.  Must
			         not inadvertently convert from a method call
				 to data ref.  */
			      if (innermost_block == 0 || 
				  contained_in (block_found, innermost_block))
				innermost_block = block_found;
			      write_exp_elt_opcode (OP_THIS);
			      write_exp_elt_opcode (OP_THIS);
			      write_exp_elt_opcode (STRUCTOP_PTR);
			      write_exp_string (yyvsp[0].ssym.stoken);
			      write_exp_elt_opcode (STRUCTOP_PTR);
			    }
			  else
			    {
			      struct minimal_symbol *msymbol;
			      register char *arg = copy_name (yyvsp[0].ssym.stoken);

			      msymbol =
				lookup_minimal_symbol (arg, NULL, NULL);
			      if (msymbol != NULL)
				{
				  write_exp_msymbol (msymbol,
						     lookup_function_type (builtin_type_int),
						     builtin_type_int);
				}
			      else if (!have_full_symbols () && !have_partial_symbols ())
				error ("No symbol table is loaded.  Use the \"file\" command.");
			      else
				error ("No symbol \"%s\" in current context.",
				       copy_name (yyvsp[0].ssym.stoken));
			    }
			;
    break;}
case 75:
#line 728 "c-exp.y"
{ push_type_address_space (copy_name (yyvsp[0].ssym.stoken));
		  push_type (tp_space_identifier);
		;
    break;}
case 83:
#line 750 "c-exp.y"
{ push_type (tp_pointer); yyval.voidval = 0; ;
    break;}
case 84:
#line 752 "c-exp.y"
{ push_type (tp_pointer); yyval.voidval = yyvsp[0].voidval; ;
    break;}
case 85:
#line 754 "c-exp.y"
{ push_type (tp_reference); yyval.voidval = 0; ;
    break;}
case 86:
#line 756 "c-exp.y"
{ push_type (tp_reference); yyval.voidval = yyvsp[0].voidval; ;
    break;}
case 88:
#line 761 "c-exp.y"
{ yyval.voidval = yyvsp[-1].voidval; ;
    break;}
case 89:
#line 763 "c-exp.y"
{
			  push_type_int (yyvsp[0].lval);
			  push_type (tp_array);
			;
    break;}
case 90:
#line 768 "c-exp.y"
{
			  push_type_int (yyvsp[0].lval);
			  push_type (tp_array);
			  yyval.voidval = 0;
			;
    break;}
case 91:
#line 775 "c-exp.y"
{ push_type (tp_function); ;
    break;}
case 92:
#line 777 "c-exp.y"
{ push_type (tp_function); ;
    break;}
case 93:
#line 781 "c-exp.y"
{ yyval.lval = -1; ;
    break;}
case 94:
#line 783 "c-exp.y"
{ yyval.lval = yyvsp[-1].typed_val_int.val; ;
    break;}
case 95:
#line 787 "c-exp.y"
{ yyval.voidval = 0; ;
    break;}
case 96:
#line 789 "c-exp.y"
{ free ((PTR)yyvsp[-1].tvec); yyval.voidval = 0; ;
    break;}
case 98:
#line 802 "c-exp.y"
{ yyval.tval = lookup_member_type (builtin_type_int, yyvsp[-2].tval); ;
    break;}
case 99:
#line 807 "c-exp.y"
{ yyval.tval = yyvsp[0].tsym.type; ;
    break;}
case 100:
#line 809 "c-exp.y"
{ yyval.tval = builtin_type_int; ;
    break;}
case 101:
#line 811 "c-exp.y"
{ yyval.tval = builtin_type_long; ;
    break;}
case 102:
#line 813 "c-exp.y"
{ yyval.tval = builtin_type_short; ;
    break;}
case 103:
#line 815 "c-exp.y"
{ yyval.tval = builtin_type_long; ;
    break;}
case 104:
#line 817 "c-exp.y"
{ yyval.tval = builtin_type_long; ;
    break;}
case 105:
#line 819 "c-exp.y"
{ yyval.tval = builtin_type_long; ;
    break;}
case 106:
#line 821 "c-exp.y"
{ yyval.tval = builtin_type_long; ;
    break;}
case 107:
#line 823 "c-exp.y"
{ yyval.tval = builtin_type_unsigned_long; ;
    break;}
case 108:
#line 825 "c-exp.y"
{ yyval.tval = builtin_type_unsigned_long; ;
    break;}
case 109:
#line 827 "c-exp.y"
{ yyval.tval = builtin_type_unsigned_long; ;
    break;}
case 110:
#line 829 "c-exp.y"
{ yyval.tval = builtin_type_long_long; ;
    break;}
case 111:
#line 831 "c-exp.y"
{ yyval.tval = builtin_type_long_long; ;
    break;}
case 112:
#line 833 "c-exp.y"
{ yyval.tval = builtin_type_long_long; ;
    break;}
case 113:
#line 835 "c-exp.y"
{ yyval.tval = builtin_type_long_long; ;
    break;}
case 114:
#line 837 "c-exp.y"
{ yyval.tval = builtin_type_long_long; ;
    break;}
case 115:
#line 839 "c-exp.y"
{ yyval.tval = builtin_type_unsigned_long_long; ;
    break;}
case 116:
#line 841 "c-exp.y"
{ yyval.tval = builtin_type_unsigned_long_long; ;
    break;}
case 117:
#line 843 "c-exp.y"
{ yyval.tval = builtin_type_unsigned_long_long; ;
    break;}
case 118:
#line 845 "c-exp.y"
{ yyval.tval = builtin_type_unsigned_long_long; ;
    break;}
case 119:
#line 847 "c-exp.y"
{ yyval.tval = lookup_signed_typename ("long long"); ;
    break;}
case 120:
#line 849 "c-exp.y"
{ yyval.tval = lookup_signed_typename ("long long"); ;
    break;}
case 121:
#line 851 "c-exp.y"
{ yyval.tval = builtin_type_short; ;
    break;}
case 122:
#line 853 "c-exp.y"
{ yyval.tval = builtin_type_short; ;
    break;}
case 123:
#line 855 "c-exp.y"
{ yyval.tval = builtin_type_short; ;
    break;}
case 124:
#line 857 "c-exp.y"
{ yyval.tval = builtin_type_unsigned_short; ;
    break;}
case 125:
#line 859 "c-exp.y"
{ yyval.tval = builtin_type_unsigned_short; ;
    break;}
case 126:
#line 861 "c-exp.y"
{ yyval.tval = builtin_type_unsigned_short; ;
    break;}
case 127:
#line 863 "c-exp.y"
{ yyval.tval = builtin_type_double; ;
    break;}
case 128:
#line 865 "c-exp.y"
{ yyval.tval = builtin_type_long_double; ;
    break;}
case 129:
#line 867 "c-exp.y"
{ yyval.tval = lookup_struct (copy_name (yyvsp[0].sval),
					      expression_context_block); ;
    break;}
case 130:
#line 870 "c-exp.y"
{ yyval.tval = lookup_struct (copy_name (yyvsp[0].sval),
					      expression_context_block); ;
    break;}
case 131:
#line 873 "c-exp.y"
{ yyval.tval = lookup_union (copy_name (yyvsp[0].sval),
					     expression_context_block); ;
    break;}
case 132:
#line 876 "c-exp.y"
{ yyval.tval = lookup_enum (copy_name (yyvsp[0].sval),
					    expression_context_block); ;
    break;}
case 133:
#line 879 "c-exp.y"
{ yyval.tval = lookup_unsigned_typename (TYPE_NAME(yyvsp[0].tsym.type)); ;
    break;}
case 134:
#line 881 "c-exp.y"
{ yyval.tval = builtin_type_unsigned_int; ;
    break;}
case 135:
#line 883 "c-exp.y"
{ yyval.tval = lookup_signed_typename (TYPE_NAME(yyvsp[0].tsym.type)); ;
    break;}
case 136:
#line 885 "c-exp.y"
{ yyval.tval = builtin_type_int; ;
    break;}
case 137:
#line 890 "c-exp.y"
{ yyval.tval = lookup_template_type(copy_name(yyvsp[-3].sval), yyvsp[-1].tval,
						    expression_context_block);
			;
    break;}
case 138:
#line 894 "c-exp.y"
{ yyval.tval = follow_types (yyvsp[0].tval); ;
    break;}
case 139:
#line 896 "c-exp.y"
{ yyval.tval = follow_types (yyvsp[-1].tval); ;
    break;}
case 141:
#line 901 "c-exp.y"
{
		  yyval.tsym.stoken.ptr = "int";
		  yyval.tsym.stoken.length = 3;
		  yyval.tsym.type = builtin_type_int;
		;
    break;}
case 142:
#line 907 "c-exp.y"
{
		  yyval.tsym.stoken.ptr = "long";
		  yyval.tsym.stoken.length = 4;
		  yyval.tsym.type = builtin_type_long;
		;
    break;}
case 143:
#line 913 "c-exp.y"
{
		  yyval.tsym.stoken.ptr = "short";
		  yyval.tsym.stoken.length = 5;
		  yyval.tsym.type = builtin_type_short;
		;
    break;}
case 144:
#line 922 "c-exp.y"
{ yyval.tvec = (struct type **) xmalloc (sizeof (struct type *) * 2);
		  yyval.ivec[0] = 1;	/* Number of types in vector */
		  yyval.tvec[1] = yyvsp[0].tval;
		;
    break;}
case 145:
#line 927 "c-exp.y"
{ int len = sizeof (struct type *) * (++(yyvsp[-2].ivec[0]) + 1);
		  yyval.tvec = (struct type **) xrealloc ((char *) yyvsp[-2].tvec, len);
		  yyval.tvec[yyval.ivec[0]] = yyvsp[0].tval;
		;
    break;}
case 147:
#line 935 "c-exp.y"
{ yyval.tval = follow_types (yyvsp[-3].tval); ;
    break;}
case 150:
#line 943 "c-exp.y"
{ push_type (tp_const);
			  push_type (tp_volatile); 
			;
    break;}
case 151:
#line 947 "c-exp.y"
{ push_type (tp_const); ;
    break;}
case 152:
#line 949 "c-exp.y"
{ push_type (tp_volatile); ;
    break;}
case 153:
#line 952 "c-exp.y"
{ yyval.sval = yyvsp[0].ssym.stoken; ;
    break;}
case 154:
#line 953 "c-exp.y"
{ yyval.sval = yyvsp[0].ssym.stoken; ;
    break;}
case 155:
#line 954 "c-exp.y"
{ yyval.sval = yyvsp[0].tsym.stoken; ;
    break;}
case 156:
#line 955 "c-exp.y"
{ yyval.sval = yyvsp[0].ssym.stoken; ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 542 "/usr/lib/bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) xmalloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;

 yyacceptlab:
  /* YYACCEPT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 0;

 yyabortlab:
  /* YYABORT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 1;
}
#line 969 "c-exp.y"


/* Take care of parsing a number (anything that starts with a digit).
   Set yylval and return the token type; update lexptr.
   LEN is the number of characters in it.  */

/*** Needs some error checking for the float case ***/

static int
parse_number (p, len, parsed_float, putithere)
     register char *p;
     register int len;
     int parsed_float;
     YYSTYPE *putithere;
{
  /* FIXME: Shouldn't these be unsigned?  We don't deal with negative values
     here, and we do kind of silly things like cast to unsigned.  */
  register LONGEST n = 0;
  register LONGEST prevn = 0;
  ULONGEST un;

  register int i = 0;
  register int c;
  register int base = input_radix;
  int unsigned_p = 0;

  /* Number of "L" suffixes encountered.  */
  int long_p = 0;

  /* We have found a "L" or "U" suffix.  */
  int found_suffix = 0;

  ULONGEST high_bit;
  struct type *signed_type;
  struct type *unsigned_type;

  if (parsed_float)
    {
      /* It's a float since it contains a point or an exponent.  */
      char c;
      int num = 0;	/* number of tokens scanned by scanf */
      char saved_char = p[len];

      p[len] = 0;	/* null-terminate the token */
      if (sizeof (putithere->typed_val_float.dval) <= sizeof (float))
	num = sscanf (p, "%g%c", (float *) &putithere->typed_val_float.dval,&c);
      else if (sizeof (putithere->typed_val_float.dval) <= sizeof (double))
	num = sscanf (p, "%lg%c", (double *) &putithere->typed_val_float.dval,&c);
      else
	{
#ifdef SCANF_HAS_LONG_DOUBLE
	  num = sscanf (p, "%Lg%c", &putithere->typed_val_float.dval,&c);
#else
	  /* Scan it into a double, then assign it to the long double.
	     This at least wins with values representable in the range
	     of doubles. */
	  double temp;
	  num = sscanf (p, "%lg%c", &temp,&c);
	  putithere->typed_val_float.dval = temp;
#endif
	}
      p[len] = saved_char;	/* restore the input stream */
      if (num != 1) 		/* check scanf found ONLY a float ... */
	return ERROR;
      /* See if it has `f' or `l' suffix (float or long double).  */

      c = tolower (p[len - 1]);

      if (c == 'f')
	putithere->typed_val_float.type = builtin_type_float;
      else if (c == 'l')
	putithere->typed_val_float.type = builtin_type_long_double;
      else if (isdigit (c) || c == '.')
	putithere->typed_val_float.type = builtin_type_double;
      else
	return ERROR;

      return FLOAT;
    }

  /* Handle base-switching prefixes 0x, 0t, 0d, 0 */
  if (p[0] == '0')
    switch (p[1])
      {
      case 'x':
      case 'X':
	if (len >= 3)
	  {
	    p += 2;
	    base = 16;
	    len -= 2;
	  }
	break;

      case 't':
      case 'T':
      case 'd':
      case 'D':
	if (len >= 3)
	  {
	    p += 2;
	    base = 10;
	    len -= 2;
	  }
	break;

      default:
	base = 8;
	break;
      }

  while (len-- > 0)
    {
      c = *p++;
      if (c >= 'A' && c <= 'Z')
	c += 'a' - 'A';
      if (c != 'l' && c != 'u')
	n *= base;
      if (c >= '0' && c <= '9')
	{
	  if (found_suffix)
	    return ERROR;
	  n += i = c - '0';
	}
      else
	{
	  if (base > 10 && c >= 'a' && c <= 'f')
	    {
	      if (found_suffix)
		return ERROR;
	      n += i = c - 'a' + 10;
	    }
	  else if (c == 'l')
	    {
	      ++long_p;
	      found_suffix = 1;
	    }
	  else if (c == 'u')
	    {
	      unsigned_p = 1;
	      found_suffix = 1;
	    }
	  else
	    return ERROR;	/* Char not a digit */
	}
      if (i >= base)
	return ERROR;		/* Invalid digit in this base */

      /* Portably test for overflow (only works for nonzero values, so make
	 a second check for zero).  FIXME: Can't we just make n and prevn
	 unsigned and avoid this?  */
      if (c != 'l' && c != 'u' && (prevn >= n) && n != 0)
	unsigned_p = 1;		/* Try something unsigned */

      /* Portably test for unsigned overflow.
	 FIXME: This check is wrong; for example it doesn't find overflow
	 on 0x123456789 when LONGEST is 32 bits.  */
      if (c != 'l' && c != 'u' && n != 0)
	{	
	  if ((unsigned_p && (ULONGEST) prevn >= (ULONGEST) n))
	    error ("Numeric constant too large.");
	}
      prevn = n;
    }

  /* An integer constant is an int, a long, or a long long.  An L
     suffix forces it to be long; an LL suffix forces it to be long
     long.  If not forced to a larger size, it gets the first type of
     the above that it fits in.  To figure out whether it fits, we
     shift it right and see whether anything remains.  Note that we
     can't shift sizeof (LONGEST) * HOST_CHAR_BIT bits or more in one
     operation, because many compilers will warn about such a shift
     (which always produces a zero result).  Sometimes TARGET_INT_BIT
     or TARGET_LONG_BIT will be that big, sometimes not.  To deal with
     the case where it is we just always shift the value more than
     once, with fewer bits each time.  */

  un = (ULONGEST)n >> 2;
  if (long_p == 0
      && (un >> (TARGET_INT_BIT - 2)) == 0)
    {
      high_bit = ((ULONGEST)1) << (TARGET_INT_BIT-1);

      /* A large decimal (not hex or octal) constant (between INT_MAX
	 and UINT_MAX) is a long or unsigned long, according to ANSI,
	 never an unsigned int, but this code treats it as unsigned
	 int.  This probably should be fixed.  GCC gives a warning on
	 such constants.  */

      unsigned_type = builtin_type_unsigned_int;
      signed_type = builtin_type_int;
    }
  else if (long_p <= 1
	   && (un >> (TARGET_LONG_BIT - 2)) == 0)
    {
      high_bit = ((ULONGEST)1) << (TARGET_LONG_BIT-1);
      unsigned_type = builtin_type_unsigned_long;
      signed_type = builtin_type_long;
    }
  else
    {
      int shift;
      if (sizeof (ULONGEST) * HOST_CHAR_BIT < TARGET_LONG_LONG_BIT)
	/* A long long does not fit in a LONGEST.  */
	shift = (sizeof (ULONGEST) * HOST_CHAR_BIT - 1);
      else
	shift = (TARGET_LONG_LONG_BIT - 1);
      high_bit = (ULONGEST) 1 << shift;
      unsigned_type = builtin_type_unsigned_long_long;
      signed_type = builtin_type_long_long;
    }

   putithere->typed_val_int.val = n;

   /* If the high bit of the worked out type is set then this number
      has to be unsigned. */

   if (unsigned_p || (n & high_bit)) 
     {
       putithere->typed_val_int.type = unsigned_type;
     }
   else 
     {
       putithere->typed_val_int.type = signed_type;
     }

   return INT;
}

struct token
{
  char *operator;
  int token;
  enum exp_opcode opcode;
};

static const struct token tokentab3[] =
  {
    {">>=", ASSIGN_MODIFY, BINOP_RSH},
    {"<<=", ASSIGN_MODIFY, BINOP_LSH}
  };

static const struct token tokentab2[] =
  {
    {"+=", ASSIGN_MODIFY, BINOP_ADD},
    {"-=", ASSIGN_MODIFY, BINOP_SUB},
    {"*=", ASSIGN_MODIFY, BINOP_MUL},
    {"/=", ASSIGN_MODIFY, BINOP_DIV},
    {"%=", ASSIGN_MODIFY, BINOP_REM},
    {"|=", ASSIGN_MODIFY, BINOP_BITWISE_IOR},
    {"&=", ASSIGN_MODIFY, BINOP_BITWISE_AND},
    {"^=", ASSIGN_MODIFY, BINOP_BITWISE_XOR},
    {"++", INCREMENT, BINOP_END},
    {"--", DECREMENT, BINOP_END},
    {"->", ARROW, BINOP_END},
    {"&&", ANDAND, BINOP_END},
    {"||", OROR, BINOP_END},
    {"::", COLONCOLON, BINOP_END},
    {"<<", LSH, BINOP_END},
    {">>", RSH, BINOP_END},
    {"==", EQUAL, BINOP_END},
    {"!=", NOTEQUAL, BINOP_END},
    {"<=", LEQ, BINOP_END},
    {">=", GEQ, BINOP_END}
  };

/* Read one token, getting characters through lexptr.  */

static int
yylex ()
{
  int c;
  int namelen;
  unsigned int i;
  char *tokstart;
  char *tokptr;
  int tempbufindex;
  static char *tempbuf;
  static int tempbufsize;
  struct symbol * sym_class = NULL;
  char * token_string = NULL;
  int class_prefix = 0;
  int unquoted_expr;
   
 retry:

  /* Check if this is a macro invocation that we need to expand.  */
  if (! scanning_macro_expansion ())
    {
      char *expanded = macro_expand_next (&lexptr,
                                          expression_macro_lookup_func,
                                          expression_macro_lookup_baton);

      if (expanded)
        scan_macro_expansion (expanded);
    }

  prev_lexptr = lexptr;
  unquoted_expr = 1;

  tokstart = lexptr;
  /* See if it is a special token of length 3.  */
  for (i = 0; i < sizeof tokentab3 / sizeof tokentab3[0]; i++)
    if (STREQN (tokstart, tokentab3[i].operator, 3))
      {
	lexptr += 3;
	yylval.opcode = tokentab3[i].opcode;
	return tokentab3[i].token;
      }

  /* See if it is a special token of length 2.  */
  for (i = 0; i < sizeof tokentab2 / sizeof tokentab2[0]; i++)
    if (STREQN (tokstart, tokentab2[i].operator, 2))
      {
	lexptr += 2;
	yylval.opcode = tokentab2[i].opcode;
	return tokentab2[i].token;
      }

  switch (c = *tokstart)
    {
    case 0:
      /* If we were just scanning the result of a macro expansion,
         then we need to resume scanning the original text.
         Otherwise, we were already scanning the original text, and
         we're really done.  */
      if (scanning_macro_expansion ())
        {
          finished_macro_expansion ();
          goto retry;
        }
      else
        return 0;

    case ' ':
    case '\t':
    case '\n':
      lexptr++;
      goto retry;

    case '\'':
      /* We either have a character constant ('0' or '\177' for example)
	 or we have a quoted symbol reference ('foo(int,int)' in C++
	 for example). */
      lexptr++;
      c = *lexptr++;
      if (c == '\\')
	c = parse_escape (&lexptr);
      else if (c == '\'')
	error ("Empty character constant.");

      yylval.typed_val_int.val = c;
      yylval.typed_val_int.type = builtin_type_char;

      c = *lexptr++;
      if (c != '\'')
	{
	  namelen = skip_quoted (tokstart) - tokstart;
	  if (namelen > 2)
	    {
	      lexptr = tokstart + namelen;
              unquoted_expr = 0;
	      if (lexptr[-1] != '\'')
		error ("Unmatched single quote.");
	      namelen -= 2;
	      tokstart++;
	      goto tryname;
	    }
	  error ("Invalid character constant.");
	}
      return INT;

    case '(':
      paren_depth++;
      lexptr++;
      return c;

    case ')':
      if (paren_depth == 0)
	return 0;
      paren_depth--;
      lexptr++;
      return c;

    case ',':
      if (comma_terminates
          && paren_depth == 0
          && ! scanning_macro_expansion ())
	return 0;
      lexptr++;
      return c;

    case '.':
      /* Might be a floating point number.  */
      if (lexptr[1] < '0' || lexptr[1] > '9')
	goto symbol;		/* Nope, must be a symbol. */
      /* FALL THRU into number case.  */

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      {
	/* It's a number.  */
	int got_dot = 0, got_e = 0, toktype;
	register char *p = tokstart;
	int hex = input_radix > 10;

	if (c == '0' && (p[1] == 'x' || p[1] == 'X'))
	  {
	    p += 2;
	    hex = 1;
	  }
	else if (c == '0' && (p[1]=='t' || p[1]=='T' || p[1]=='d' || p[1]=='D'))
	  {
	    p += 2;
	    hex = 0;
	  }

	for (;; ++p)
	  {
	    /* This test includes !hex because 'e' is a valid hex digit
	       and thus does not indicate a floating point number when
	       the radix is hex.  */
	    if (!hex && !got_e && (*p == 'e' || *p == 'E'))
	      got_dot = got_e = 1;
	    /* This test does not include !hex, because a '.' always indicates
	       a decimal floating point number regardless of the radix.  */
	    else if (!got_dot && *p == '.')
	      got_dot = 1;
	    else if (got_e && (p[-1] == 'e' || p[-1] == 'E')
		     && (*p == '-' || *p == '+'))
	      /* This is the sign of the exponent, not the end of the
		 number.  */
	      continue;
	    /* We will take any letters or digits.  parse_number will
	       complain if past the radix, or if L or U are not final.  */
	    else if ((*p < '0' || *p > '9')
		     && ((*p < 'a' || *p > 'z')
				  && (*p < 'A' || *p > 'Z')))
	      break;
	  }
	toktype = parse_number (tokstart, p - tokstart, got_dot|got_e, &yylval);
        if (toktype == ERROR)
	  {
	    char *err_copy = (char *) alloca (p - tokstart + 1);

	    memcpy (err_copy, tokstart, p - tokstart);
	    err_copy[p - tokstart] = 0;
	    error ("Invalid number \"%s\".", err_copy);
	  }
	lexptr = p;
	return toktype;
      }

    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
    case '|':
    case '&':
    case '^':
    case '~':
    case '!':
    case '@':
    case '<':
    case '>':
    case '[':
    case ']':
    case '?':
    case ':':
    case '=':
    case '{':
    case '}':
    symbol:
      lexptr++;
      return c;

    case '"':

      /* Build the gdb internal form of the input string in tempbuf,
	 translating any standard C escape forms seen.  Note that the
	 buffer is null byte terminated *only* for the convenience of
	 debugging gdb itself and printing the buffer contents when
	 the buffer contains no embedded nulls.  Gdb does not depend
	 upon the buffer being null byte terminated, it uses the length
	 string instead.  This allows gdb to handle C strings (as well
	 as strings in other languages) with embedded null bytes */

      tokptr = ++tokstart;
      tempbufindex = 0;

      do {
	/* Grow the static temp buffer if necessary, including allocating
	   the first one on demand. */
	if (tempbufindex + 1 >= tempbufsize)
	  {
	    tempbuf = (char *) xrealloc (tempbuf, tempbufsize += 64);
	  }
	switch (*tokptr)
	  {
	  case '\0':
	  case '"':
	    /* Do nothing, loop will terminate. */
	    break;
	  case '\\':
	    tokptr++;
	    c = parse_escape (&tokptr);
	    if (c == -1)
	      {
		continue;
	      }
	    tempbuf[tempbufindex++] = c;
	    break;
	  default:
	    tempbuf[tempbufindex++] = *tokptr++;
	    break;
	  }
      } while ((*tokptr != '"') && (*tokptr != '\0'));
      if (*tokptr++ != '"')
	{
	  error ("Unterminated string in expression.");
	}
      tempbuf[tempbufindex] = '\0';	/* See note above */
      yylval.sval.ptr = tempbuf;
      yylval.sval.length = tempbufindex;
      lexptr = tokptr;
      return (STRING);
    }

  if (!(c == '_' || c == '$'
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
    /* We must have come across a bad character (e.g. ';').  */
    error ("Invalid character '%c' in expression.", c);

  /* It's a name.  See how long it is.  */
  namelen = 0;
  for (c = tokstart[namelen];
       (c == '_' || c == '$' || (c >= '0' && c <= '9')
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '<');)
    {
      /* Template parameter lists are part of the name.
	 FIXME: This mishandles `print $a<4&&$a>3'.  */

      if (c == '<')
	{ 
               /* Scan ahead to get rest of the template specification.  Note
                  that we look ahead only when the '<' adjoins non-whitespace
                  characters; for comparison expressions, e.g. "a < b > c",
                  there must be spaces before the '<', etc. */
               
               char * p = find_template_name_end (tokstart + namelen);
               if (p)
                 namelen = p - tokstart;
               break;
	}
      c = tokstart[++namelen];
    }

  /* The token "if" terminates the expression and is NOT removed from
     the input stream.  It doesn't count if it appears in the
     expansion of a macro.  */
  if (namelen == 2
      && tokstart[0] == 'i'
      && tokstart[1] == 'f'
      && ! scanning_macro_expansion ())
    {
      return 0;
    }

  lexptr += namelen;

  tryname:

  /* Catch specific keywords.  Should be done with a data structure.  */
  switch (namelen)
    {
    case 8:
      if (STREQN (tokstart, "unsigned", 8))
	return UNSIGNED;
      if (current_language->la_language == language_cplus
	  && STREQN (tokstart, "template", 8))
	return TEMPLATE;
      if (STREQN (tokstart, "volatile", 8))
	return VOLATILE_KEYWORD;
      break;
    case 6:
      if (STREQN (tokstart, "struct", 6))
	return STRUCT;
      if (STREQN (tokstart, "signed", 6))
	return SIGNED_KEYWORD;
      if (STREQN (tokstart, "sizeof", 6))      
	return SIZEOF;
      if (STREQN (tokstart, "double", 6))      
	return DOUBLE_KEYWORD;
      break;
    case 5:
      if (current_language->la_language == language_cplus)
        {
          if (STREQN (tokstart, "false", 5))
            return FALSEKEYWORD;
          if (STREQN (tokstart, "class", 5))
            return CLASS;
        }
      if (STREQN (tokstart, "union", 5))
	return UNION;
      if (STREQN (tokstart, "short", 5))
	return SHORT;
      if (STREQN (tokstart, "const", 5))
	return CONST_KEYWORD;
      break;
    case 4:
      if (STREQN (tokstart, "enum", 4))
	return ENUM;
      if (STREQN (tokstart, "long", 4))
	return LONG;
      if (current_language->la_language == language_cplus)
          {
            if (STREQN (tokstart, "true", 4))
              return TRUEKEYWORD;

            if (STREQN (tokstart, "this", 4))
              {
                static const char this_name[] =
                { CPLUS_MARKER, 't', 'h', 'i', 's', '\0' };
                
                if (lookup_symbol (this_name, expression_context_block,
                                   VAR_NAMESPACE, (int *) NULL,
                                   (struct symtab **) NULL))
                  return THIS;
              }
          }
      break;
    case 3:
      if (STREQN (tokstart, "int", 3))
	return INT_KEYWORD;
      break;
    default:
      break;
    }

  yylval.sval.ptr = tokstart;
  yylval.sval.length = namelen;

  if (*tokstart == '$')
    {
      write_dollar_variable (yylval.sval);
      return VARIABLE;
    }
  
  /* Look ahead and see if we can consume more of the input
     string to get a reasonable class/namespace spec or a
     fully-qualified name.  This is a kludge to get around the
     HP aCC compiler's generation of symbol names with embedded
     colons for namespace and nested classes. */ 
  if (unquoted_expr)
    {
      /* Only do it if not inside single quotes */ 
      sym_class = parse_nested_classes_for_hpacc (yylval.sval.ptr, yylval.sval.length,
                                                  &token_string, &class_prefix, &lexptr);
      if (sym_class)
        {
          /* Replace the current token with the bigger one we found */ 
          yylval.sval.ptr = token_string;
          yylval.sval.length = strlen (token_string);
        }
    }
  
  /* Use token-type BLOCKNAME for symbols that happen to be defined as
     functions or symtabs.  If this is not so, then ...
     Use token-type TYPENAME for symbols that happen to be defined
     currently as names of types; NAME for other symbols.
     The caller is not constrained to care about the distinction.  */
  {
    char *tmp = copy_name (yylval.sval);
    struct symbol *sym;
    int is_a_field_of_this = 0;
    int hextype;

    sym = lookup_symbol (tmp, expression_context_block,
			 VAR_NAMESPACE,
			 current_language->la_language == language_cplus
			 ? &is_a_field_of_this : (int *) NULL,
			 (struct symtab **) NULL);
    /* Call lookup_symtab, not lookup_partial_symtab, in case there are
       no psymtabs (coff, xcoff, or some future change to blow away the
       psymtabs once once symbols are read).  */
    if (sym && SYMBOL_CLASS (sym) == LOC_BLOCK)
      {
	yylval.ssym.sym = sym;
	yylval.ssym.is_a_field_of_this = is_a_field_of_this;
	return BLOCKNAME;
      }
    else if (!sym)
      {				/* See if it's a file name. */
	struct symtab *symtab;

	symtab = lookup_symtab (tmp);

	if (symtab)
	  {
	    yylval.bval = BLOCKVECTOR_BLOCK (BLOCKVECTOR (symtab), STATIC_BLOCK);
	    return FILENAME;
	  }
      }

    if (sym && SYMBOL_CLASS (sym) == LOC_TYPEDEF)
        {
#if 1
	  /* Despite the following flaw, we need to keep this code enabled.
	     Because we can get called from check_stub_method, if we don't
	     handle nested types then it screws many operations in any
	     program which uses nested types.  */
	  /* In "A::x", if x is a member function of A and there happens
	     to be a type (nested or not, since the stabs don't make that
	     distinction) named x, then this code incorrectly thinks we
	     are dealing with nested types rather than a member function.  */

	  char *p;
	  char *namestart;
	  struct symbol *best_sym;

	  /* Look ahead to detect nested types.  This probably should be
	     done in the grammar, but trying seemed to introduce a lot
	     of shift/reduce and reduce/reduce conflicts.  It's possible
	     that it could be done, though.  Or perhaps a non-grammar, but
	     less ad hoc, approach would work well.  */

	  /* Since we do not currently have any way of distinguishing
	     a nested type from a non-nested one (the stabs don't tell
	     us whether a type is nested), we just ignore the
	     containing type.  */

	  p = lexptr;
	  best_sym = sym;
	  while (1)
	    {
	      /* Skip whitespace.  */
	      while (*p == ' ' || *p == '\t' || *p == '\n')
		++p;
	      if (*p == ':' && p[1] == ':')
		{
		  /* Skip the `::'.  */
		  p += 2;
		  /* Skip whitespace.  */
		  while (*p == ' ' || *p == '\t' || *p == '\n')
		    ++p;
		  namestart = p;
		  while (*p == '_' || *p == '$' || (*p >= '0' && *p <= '9')
			 || (*p >= 'a' && *p <= 'z')
			 || (*p >= 'A' && *p <= 'Z'))
		    ++p;
		  if (p != namestart)
		    {
		      struct symbol *cur_sym;
		      /* As big as the whole rest of the expression, which is
			 at least big enough.  */
		      char *ncopy = alloca (strlen (tmp)+strlen (namestart)+3);
		      char *tmp1;

		      tmp1 = ncopy;
		      memcpy (tmp1, tmp, strlen (tmp));
		      tmp1 += strlen (tmp);
		      memcpy (tmp1, "::", 2);
		      tmp1 += 2;
		      memcpy (tmp1, namestart, p - namestart);
		      tmp1[p - namestart] = '\0';
		      cur_sym = lookup_symbol (ncopy, expression_context_block,
					       VAR_NAMESPACE, (int *) NULL,
					       (struct symtab **) NULL);
		      if (cur_sym)
			{
			  if (SYMBOL_CLASS (cur_sym) == LOC_TYPEDEF)
			    {
			      best_sym = cur_sym;
			      lexptr = p;
			    }
			  else
			    break;
			}
		      else
			break;
		    }
		  else
		    break;
		}
	      else
		break;
	    }

	  yylval.tsym.type = SYMBOL_TYPE (best_sym);
#else /* not 0 */
	  yylval.tsym.type = SYMBOL_TYPE (sym);
#endif /* not 0 */
	  return TYPENAME;
        }
    if ((yylval.tsym.type = lookup_primitive_typename (tmp)) != 0)
      return TYPENAME;

    /* Input names that aren't symbols but ARE valid hex numbers,
       when the input radix permits them, can be names or numbers
       depending on the parse.  Note we support radixes > 16 here.  */
    if (!sym && 
        ((tokstart[0] >= 'a' && tokstart[0] < 'a' + input_radix - 10) ||
         (tokstart[0] >= 'A' && tokstart[0] < 'A' + input_radix - 10)))
      {
 	YYSTYPE newlval;	/* Its value is ignored.  */
	hextype = parse_number (tokstart, namelen, 0, &newlval);
	if (hextype == INT)
	  {
	    yylval.ssym.sym = sym;
	    yylval.ssym.is_a_field_of_this = is_a_field_of_this;
	    return NAME_OR_INT;
	  }
      }

    /* Any other kind of symbol */
    yylval.ssym.sym = sym;
    yylval.ssym.is_a_field_of_this = is_a_field_of_this;
    return NAME;
  }
}

void
yyerror (msg)
     char *msg;
{
  if (prev_lexptr)
    lexptr = prev_lexptr;

  error ("A %s in expression, near `%s'.", (msg ? msg : "error"), lexptr);
}
