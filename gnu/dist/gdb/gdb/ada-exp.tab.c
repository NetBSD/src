
/*  A Bison parser, made from ./ada-exp.y
 by  GNU Bison version 1.27
  */

#define YYBISON 1  /* Identify Bison output.  */

#define	INT	257
#define	NULL_PTR	258
#define	CHARLIT	259
#define	FLOAT	260
#define	TYPENAME	261
#define	BLOCKNAME	262
#define	STRING	263
#define	NAME	264
#define	DOT_ID	265
#define	OBJECT_RENAMING	266
#define	DOT_ALL	267
#define	LAST	268
#define	REGNAME	269
#define	INTERNAL_VARIABLE	270
#define	ASSIGN	271
#define	_AND_	272
#define	OR	273
#define	XOR	274
#define	THEN	275
#define	ELSE	276
#define	NOTEQUAL	277
#define	LEQ	278
#define	GEQ	279
#define	IN	280
#define	DOTDOT	281
#define	UNARY	282
#define	MOD	283
#define	REM	284
#define	STARSTAR	285
#define	ABS	286
#define	NOT	287
#define	TICK_ACCESS	288
#define	TICK_ADDRESS	289
#define	TICK_FIRST	290
#define	TICK_LAST	291
#define	TICK_LENGTH	292
#define	TICK_MAX	293
#define	TICK_MIN	294
#define	TICK_MODULUS	295
#define	TICK_POS	296
#define	TICK_RANGE	297
#define	TICK_SIZE	298
#define	TICK_TAG	299
#define	TICK_VAL	300
#define	ARROW	301
#define	NEW	302

#line 38 "./ada-exp.y"


#include "defs.h"
#include <string.h>
#include <ctype.h>
#include "expression.h"
#include "value.h"
#include "parser-defs.h"
#include "language.h"
#include "ada-lang.h"
#include "bfd.h" /* Required by objfiles.h.  */
#include "symfile.h" /* Required by objfiles.h.  */
#include "objfiles.h" /* For have_full_symbols and have_partial_symbols */
#include "frame.h"

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror, etc),
   as well as gratuitiously global symbol names, so we can have multiple
   yacc generated parsers in gdb.  These are only the variables
   produced by yacc.  If other parser generators (bison, byacc, etc) produce
   additional global names that conflict at link time, then those parser
   generators need to be fixed instead of adding those names to this list. */

/* NOTE: This is clumsy, especially since BISON and FLEX provide --prefix  
   options.  I presume we are maintaining it to accommodate systems
   without BISON?  (PNH) */

#define	yymaxdepth ada_maxdepth
#define	yyparse	_ada_parse	/* ada_parse calls this after  initialization */
#define	yylex	ada_lex
#define	yyerror	ada_error
#define	yylval	ada_lval
#define	yychar	ada_char
#define	yydebug	ada_debug
#define	yypact	ada_pact	
#define	yyr1	ada_r1			
#define	yyr2	ada_r2			
#define	yydef	ada_def		
#define	yychk	ada_chk		
#define	yypgo	ada_pgo		
#define	yyact	ada_act		
#define	yyexca	ada_exca
#define yyerrflag ada_errflag
#define yynerrs	ada_nerrs
#define	yyps	ada_ps
#define	yypv	ada_pv
#define	yys	ada_s
#define	yy_yys	ada_yys
#define	yystate	ada_state
#define	yytmp	ada_tmp
#define	yyv	ada_v
#define	yy_yyv	ada_yyv
#define	yyval	ada_val
#define	yylloc	ada_lloc
#define yyreds	ada_reds		/* With YYDEBUG defined */
#define yytoks	ada_toks		/* With YYDEBUG defined */
#define yyname	ada_name		/* With YYDEBUG defined */
#define yyrule	ada_rule		/* With YYDEBUG defined */

#ifndef YYDEBUG
#define	YYDEBUG	1		/* Default to yydebug support */
#endif

#define YYFPRINTF parser_fprintf

struct name_info {
  struct symbol* sym;
  struct minimal_symbol* msym;
  struct block* block;
  struct stoken stoken;
};

/* If expression is in the context of TYPE'(...), then TYPE, else
 * NULL. */
static struct type* type_qualifier;

int yyparse (void);

static int yylex (void);

void yyerror (char *);

static struct stoken string_to_operator (struct stoken);

static void write_attribute_call0 (enum ada_attribute);

static void write_attribute_call1 (enum ada_attribute, LONGEST);

static void write_attribute_calln (enum ada_attribute, int);

static void write_object_renaming (struct block*, struct symbol*);

static void write_var_from_name (struct block*, struct name_info);

static LONGEST
convert_char_literal (struct type*, LONGEST);

#line 135 "./ada-exp.y"
typedef union
  {
    LONGEST lval;
    struct {
      LONGEST val;
      struct type *type;
    } typed_val;
    struct {
      DOUBLEST dval;
      struct type *type;
    } typed_val_float;
    struct type *tval;
    struct stoken sval;
    struct name_info ssym;
    int voidval;
    struct block *bval;
    struct internalvar *ivar;

  } YYSTYPE;
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		184
#define	YYFLAG		-32768
#define	YYNTBASE	68

#define YYTRANSLATE(x) ((unsigned)(x) <= 302 ? yytranslate[x] : 82)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,    34,    63,    57,
    62,    36,    32,    64,    33,    56,    37,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,    61,    25,
    23,    26,     2,    31,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
    58,     2,    67,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    65,     2,    66,     2,     2,     2,     2,     2,
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
    17,    18,    19,    20,    21,    22,    24,    27,    28,    29,
    30,    35,    38,    39,    40,    41,    42,    43,    44,    45,
    46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
    59,    60
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     2,     4,     6,    10,    13,    16,    21,    26,    27,
    35,    36,    43,    47,    49,    51,    53,    55,    57,    61,
    64,    67,    70,    73,    74,    76,    80,    84,    90,    95,
    99,   103,   107,   111,   115,   119,   123,   127,   131,   135,
   139,   143,   149,   155,   159,   166,   173,   178,   182,   186,
   190,   194,   199,   203,   208,   212,   215,   218,   222,   226,
   230,   233,   236,   244,   252,   258,   262,   266,   270,   276,
   279,   280,   284,   286,   288,   289,   291,   293,   295,   297,
   299,   302,   304,   307,   309,   312,   314,   316,   318,   320,
   323,   325,   328,   331,   335,   338,   341
};

static const short yyrhs[] = {    69,
     0,    81,     0,    73,     0,    69,    61,    73,     0,    70,
    13,     0,    70,    11,     0,    70,    57,    74,    62,     0,
    81,    57,    73,    62,     0,     0,    81,    63,    72,    71,
    57,    73,    62,     0,     0,    70,    57,    73,    30,    73,
    62,     0,    57,    69,    62,     0,    78,     0,    15,     0,
    16,     0,    70,     0,    14,     0,    73,    17,    73,     0,
    33,    73,     0,    32,    73,     0,    42,    73,     0,    41,
    73,     0,     0,    73,     0,    79,    59,    73,     0,    74,
    64,    73,     0,    74,    64,    79,    59,    73,     0,    65,
    81,    66,    73,     0,    73,    40,    73,     0,    73,    36,
    73,     0,    73,    37,    73,     0,    73,    39,    73,     0,
    73,    38,    73,     0,    73,    31,    73,     0,    73,    32,
    73,     0,    73,    34,    73,     0,    73,    33,    73,     0,
    73,    23,    73,     0,    73,    24,    73,     0,    73,    27,
    73,     0,    73,    29,    73,    30,    73,     0,    73,    29,
    73,    52,    75,     0,    73,    29,     7,     0,    73,    42,
    29,    73,    30,    73,     0,    73,    42,    29,    73,    52,
    75,     0,    73,    42,    29,     7,     0,    73,    28,    73,
     0,    73,    25,    73,     0,    73,    26,    73,     0,    73,
    18,    73,     0,    73,    18,    21,    73,     0,    73,    19,
    73,     0,    73,    19,    22,    73,     0,    73,    20,    73,
     0,    70,    43,     0,    70,    44,     0,    70,    45,    75,
     0,    70,    46,    75,     0,    70,    47,    75,     0,    70,
    53,     0,    70,    54,     0,    77,    49,    57,    73,    64,
    73,    62,     0,    77,    48,    57,    73,    64,    73,    62,
     0,    77,    51,    57,    73,    62,     0,    76,    45,    75,
     0,    76,    46,    75,     0,    76,    47,    75,     0,    76,
    55,    57,    73,    62,     0,    76,    50,     0,     0,    57,
     3,    62,     0,     7,     0,    76,     0,     0,     3,     0,
     5,     0,     6,     0,     4,     0,     9,     0,    60,     7,
     0,    10,     0,    80,    10,     0,    12,     0,    80,    12,
     0,    10,     0,     7,     0,    12,     0,     8,     0,    80,
     8,     0,     7,     0,    80,     7,     0,     7,    43,     0,
    80,     7,    43,     0,    36,    73,     0,    34,    73,     0,
    73,    58,    73,    67,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   207,   208,   214,   215,   220,   224,   231,   239,   247,   247,
   258,   260,   265,   268,   271,   278,   286,   289,   296,   300,
   304,   308,   312,   316,   319,   321,   323,   325,   329,   339,
   343,   347,   351,   355,   359,   363,   367,   371,   375,   379,
   383,   387,   391,   397,   404,   409,   417,   427,   431,   435,
   439,   443,   447,   451,   455,   459,   461,   467,   469,   471,
   473,   475,   477,   479,   481,   483,   485,   487,   489,   491,
   495,   497,   501,   508,   510,   517,   525,   537,   545,   552,
   579,   583,   584,   586,   587,   591,   592,   593,   596,   598,
   603,   604,   605,   607,   614,   616,   618
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","INT","NULL_PTR",
"CHARLIT","FLOAT","TYPENAME","BLOCKNAME","STRING","NAME","DOT_ID","OBJECT_RENAMING",
"DOT_ALL","LAST","REGNAME","INTERNAL_VARIABLE","ASSIGN","_AND_","OR","XOR","THEN",
"ELSE","'='","NOTEQUAL","'<'","'>'","LEQ","GEQ","IN","DOTDOT","'@'","'+'","'-'",
"'&'","UNARY","'*'","'/'","MOD","REM","STARSTAR","ABS","NOT","TICK_ACCESS","TICK_ADDRESS",
"TICK_FIRST","TICK_LAST","TICK_LENGTH","TICK_MAX","TICK_MIN","TICK_MODULUS",
"TICK_POS","TICK_RANGE","TICK_SIZE","TICK_TAG","TICK_VAL","'.'","'('","'['",
"ARROW","NEW","';'","')'","'\\''","','","'{'","'}'","']'","start","exp1","simple_exp",
"@1","save_qualifier","exp","arglist","tick_arglist","type_prefix","opt_type_prefix",
"variable","any_name","block","type", NULL
};
#endif

static const short yyr1[] = {     0,
    68,    68,    69,    69,    70,    70,    70,    70,    71,    70,
    72,    70,    70,    70,    70,    70,    73,    70,    73,    73,
    73,    73,    73,    74,    74,    74,    74,    74,    73,    73,
    73,    73,    73,    73,    73,    73,    73,    73,    73,    73,
    73,    73,    73,    73,    73,    73,    73,    73,    73,    73,
    73,    73,    73,    73,    73,    70,    70,    70,    70,    70,
    70,    70,    70,    70,    70,    70,    70,    70,    70,    70,
    75,    75,    76,    77,    77,    73,    73,    73,    73,    73,
    73,    78,    78,    78,    78,    79,    79,    79,    80,    80,
    81,    81,    81,    81,    73,    73,    73
};

static const short yyr2[] = {     0,
     1,     1,     1,     3,     2,     2,     4,     4,     0,     7,
     0,     6,     3,     1,     1,     1,     1,     1,     3,     2,
     2,     2,     2,     0,     1,     3,     3,     5,     4,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     5,     5,     3,     6,     6,     4,     3,     3,     3,
     3,     4,     3,     4,     3,     2,     2,     3,     3,     3,
     2,     2,     7,     7,     5,     3,     3,     3,     5,     2,
     0,     3,     1,     1,     0,     1,     1,     1,     1,     1,
     2,     1,     2,     1,     2,     1,     1,     1,     1,     2,
     1,     2,     2,     3,     2,     2,     4
};

static const short yydefact[] = {    75,
    76,    79,    77,    78,    73,    89,    80,    82,    84,    18,
    15,    16,    75,    75,    75,    75,    75,    75,    75,     0,
     0,     1,    17,     3,    74,     0,    14,     0,     2,    93,
    21,     0,    20,    96,    95,    23,    22,     0,    81,    91,
     0,     0,    75,     6,     5,    56,    57,    71,    71,    71,
    61,    62,    75,    75,    75,    75,    75,    75,    75,    75,
    75,    75,    75,    75,    75,    75,    75,    75,    75,    75,
    75,    75,    75,     0,    75,    71,    71,    71,    70,     0,
     0,     0,     0,    92,    90,    83,    85,    75,    11,    13,
    75,     4,     0,    58,    59,    60,    73,    82,    84,    25,
     0,     0,    19,    75,    51,    75,    53,    55,    39,    40,
    49,    50,    41,    48,    44,     0,    35,    36,    38,    37,
    31,    32,    34,    33,    30,    75,     0,    66,    67,    68,
    75,    75,    75,    75,    94,     0,     9,    29,     0,    75,
     7,    75,    75,    52,    54,    75,    71,    47,     0,    97,
     0,     0,     0,     0,     8,     0,    72,     0,    27,     0,
    26,    42,    43,    75,    71,    69,    75,    75,    65,    75,
    12,    75,    45,    46,     0,     0,     0,    28,    64,    63,
    10,     0,     0,     0
};

static const short yydefgoto[] = {   182,
    22,    23,   156,   137,    24,   101,    94,    25,    26,    27,
   102,    28,    32
};

static const short yypact[] = {   251,
-32768,-32768,-32768,-32768,    20,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,   251,   251,   251,   251,   251,   251,   251,     2,
    79,   -47,    53,   958,   -23,    54,-32768,   104,   -32,-32768,
    31,   -32,    31,   -22,   -22,    31,    31,    33,-32768,    -5,
   101,   -27,   251,-32768,-32768,-32768,-32768,     4,     4,     4,
-32768,-32768,   131,   251,   171,   211,   251,   251,   251,   251,
   251,   251,   251,   291,   251,   251,   251,   251,   251,   251,
   251,   251,   251,    47,   251,     4,     4,     4,-32768,    23,
    25,    27,    35,    45,-32768,-32768,-32768,   251,-32768,-32768,
   251,   958,    98,-32768,-32768,-32768,    22,    56,    58,   930,
   -36,    64,   986,   251,  1009,   251,  1009,  1009,   -21,   -21,
   -21,   -21,   -21,   -21,   534,   858,   387,    31,    31,    31,
    32,    32,    32,    32,    32,   331,   415,-32768,-32768,-32768,
   251,   251,   251,   251,-32768,   536,-32768,   -22,    62,   251,
-32768,   371,   251,  1009,  1009,   251,     4,   534,   894,-32768,
   582,   452,   494,   628,-32768,    68,-32768,   674,   958,    67,
   958,   -21,-32768,   251,     4,-32768,   251,   251,-32768,   251,
-32768,   251,   -21,-32768,   720,   766,   812,   958,-32768,-32768,
-32768,   128,   132,-32768
};

static const short yypgoto[] = {-32768,
   112,-32768,-32768,-32768,   -13,-32768,   -43,-32768,-32768,-32768,
     0,   123,     8
};


#define	YYLAST		1067


static const short yytable[] = {    31,
    33,    34,    35,    36,    37,    95,    96,    29,    39,    65,
    66,    67,    68,    43,    69,    70,    71,    72,    73,   -91,
    74,    76,    77,    78,    88,   141,    79,   142,    42,    92,
    89,    80,   128,   129,   130,    75,    75,    30,    91,   100,
   103,   105,   107,   108,   109,   110,   111,   112,   113,   114,
   116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
    93,   127,    30,    44,    30,    45,    69,    70,    71,    72,
    73,    73,    74,    74,   136,   126,   -91,   138,   -91,   131,
   -87,   132,   -91,   133,   -91,    40,     6,   135,    75,    75,
   144,   134,   145,    43,    90,    46,    47,    48,    49,    50,
   139,    81,    82,   163,    83,    51,    52,    84,    85,    53,
    84,    85,   149,    86,   -86,    87,   -88,   151,   152,   153,
   154,   174,   143,   157,   170,   172,   158,   183,   159,   161,
    38,   184,   162,     1,     2,     3,     4,    97,     6,     7,
    98,   160,    99,    41,    10,    11,    12,     0,     0,     0,
   173,     0,     0,   175,   176,     0,   177,     0,   178,     0,
     0,     0,    13,    14,    15,     0,    16,     0,     0,     0,
     0,    17,    18,     1,     2,     3,     4,     5,     6,     7,
     8,     0,     9,     0,    10,    11,    12,    19,     0,     0,
    20,   104,   -24,     0,   -24,    21,     0,     0,     0,     0,
     0,     0,    13,    14,    15,     0,    16,     0,     0,     0,
     0,    17,    18,     1,     2,     3,     4,     5,     6,     7,
     8,     0,     9,     0,    10,    11,    12,    19,     0,     0,
    20,     0,   106,     0,     0,    21,     0,     0,     0,     0,
     0,     0,    13,    14,    15,     0,    16,     0,     0,     0,
     0,    17,    18,     1,     2,     3,     4,     5,     6,     7,
     8,     0,     9,     0,    10,    11,    12,    19,     0,     0,
    20,     0,     0,     0,     0,    21,     0,     0,     0,     0,
     0,     0,    13,    14,    15,     0,    16,     0,     0,     0,
     0,    17,    18,     1,     2,     3,     4,   115,     6,     7,
     8,     0,     9,     0,    10,    11,    12,    19,     0,     0,
    20,     0,     0,     0,     0,    21,     0,     0,     0,     0,
     0,     0,    13,    14,    15,     0,    16,     0,     0,     0,
     0,    17,    18,     1,     2,     3,     4,   148,     6,     7,
     8,     0,     9,     0,    10,    11,    12,    19,     0,     0,
    20,     0,     0,     0,     0,    21,     0,     0,     0,     0,
     0,     0,    13,    14,    15,     0,    16,     0,     0,     0,
     0,    17,    18,     1,     2,     3,     4,    97,     6,     7,
    98,     0,    99,     0,    10,    11,    12,    19,     0,     0,
    20,     0,     0,     0,     0,    21,     0,     0,     0,     0,
     0,     0,    13,    14,    15,     0,    16,     0,     0,     0,
     0,    17,    18,     0,     0,     0,     0,     0,    66,    67,
    68,     0,    69,    70,    71,    72,    73,    19,    74,     0,
    20,    54,    55,    56,    57,    21,     0,    58,    59,    60,
    61,    62,    63,    64,    75,    65,    66,    67,    68,     0,
    69,    70,    71,    72,    73,     0,    74,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,    54,    55,
    56,    57,    75,     0,    58,    59,    60,    61,    62,    63,
    64,   150,    65,    66,    67,    68,     0,    69,    70,    71,
    72,    73,     0,    74,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,    75,
    54,    55,    56,    57,     0,   167,    58,    59,    60,    61,
    62,    63,    64,     0,    65,    66,    67,    68,     0,    69,
    70,    71,    72,    73,     0,    74,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,    75,    54,    55,    56,    57,     0,   168,    58,    59,
    60,    61,    62,    63,    64,     0,    65,    66,    67,    68,
     0,    69,    70,    71,    72,    73,    30,    74,   -73,   -73,
   -73,   -73,   -73,   -73,   -73,     0,     0,     0,   -73,     0,
   -91,     0,     0,    75,     0,     0,   -91,   155,    54,    55,
    56,    57,     0,     0,    58,    59,    60,    61,    62,    63,
    64,     0,    65,    66,    67,    68,     0,    69,    70,    71,
    72,    73,     0,    74,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,    75,
     0,     0,     0,   166,    54,    55,    56,    57,     0,     0,
    58,    59,    60,    61,    62,    63,    64,     0,    65,    66,
    67,    68,     0,    69,    70,    71,    72,    73,     0,    74,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,    75,     0,     0,     0,   169,
    54,    55,    56,    57,     0,     0,    58,    59,    60,    61,
    62,    63,    64,     0,    65,    66,    67,    68,     0,    69,
    70,    71,    72,    73,     0,    74,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,    75,     0,     0,     0,   171,    54,    55,    56,    57,
     0,     0,    58,    59,    60,    61,    62,    63,    64,     0,
    65,    66,    67,    68,     0,    69,    70,    71,    72,    73,
     0,    74,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,    75,     0,     0,
     0,   179,    54,    55,    56,    57,     0,     0,    58,    59,
    60,    61,    62,    63,    64,     0,    65,    66,    67,    68,
     0,    69,    70,    71,    72,    73,     0,    74,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,    75,     0,     0,     0,   180,    54,    55,
    56,    57,     0,     0,    58,    59,    60,    61,    62,    63,
    64,     0,    65,    66,    67,    68,     0,    69,    70,    71,
    72,    73,     0,    74,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,    75,
     0,     0,     0,   181,    54,    55,    56,    57,     0,     0,
    58,    59,    60,    61,    62,    63,    64,   146,    65,    66,
    67,    68,     0,    69,    70,    71,    72,    73,     0,    74,
     0,     0,     0,     0,     0,     0,     0,     0,     0,   147,
    54,    55,    56,    57,     0,    75,    58,    59,    60,    61,
    62,    63,    64,   164,    65,    66,    67,    68,     0,    69,
    70,    71,    72,    73,     0,    74,     0,     0,     0,     0,
     0,     0,     0,     0,     0,   165,    54,    55,    56,    57,
     0,    75,    58,    59,    60,    61,    62,    63,    64,   140,
    65,    66,    67,    68,     0,    69,    70,    71,    72,    73,
     0,    74,     0,     0,    54,    55,    56,    57,     0,     0,
    58,    59,    60,    61,    62,    63,    64,    75,    65,    66,
    67,    68,     0,    69,    70,    71,    72,    73,     0,    74,
     0,     0,-32768,    55,    56,    57,     0,     0,    58,    59,
    60,    61,    62,    63,    64,    75,    65,    66,    67,    68,
     0,    69,    70,    71,    72,    73,     0,    74,     0,     0,
     0,    58,    59,    60,    61,    62,    63,    64,     0,    65,
    66,    67,    68,    75,    69,    70,    71,    72,    73,     0,
    74,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,    75
};

static const short yycheck[] = {    13,
    14,    15,    16,    17,    18,    49,    50,     0,     7,    31,
    32,    33,    34,    61,    36,    37,    38,    39,    40,     0,
    42,    45,    46,    47,    57,    62,    50,    64,    21,    43,
    63,    55,    76,    77,    78,    58,    58,    43,    66,    53,
    54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
    64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
    57,    75,    43,    11,    43,    13,    36,    37,    38,    39,
    40,    40,    42,    42,    88,    29,    57,    91,    57,    57,
    59,    57,    63,    57,    63,     7,     8,    43,    58,    58,
   104,    57,   106,    61,    62,    43,    44,    45,    46,    47,
     3,    48,    49,   147,    51,    53,    54,     7,     8,    57,
     7,     8,   126,    10,    59,    12,    59,   131,   132,   133,
   134,   165,    59,    62,    57,    59,   140,     0,   142,   143,
    19,     0,   146,     3,     4,     5,     6,     7,     8,     9,
    10,   142,    12,    21,    14,    15,    16,    -1,    -1,    -1,
   164,    -1,    -1,   167,   168,    -1,   170,    -1,   172,    -1,
    -1,    -1,    32,    33,    34,    -1,    36,    -1,    -1,    -1,
    -1,    41,    42,     3,     4,     5,     6,     7,     8,     9,
    10,    -1,    12,    -1,    14,    15,    16,    57,    -1,    -1,
    60,    21,    62,    -1,    64,    65,    -1,    -1,    -1,    -1,
    -1,    -1,    32,    33,    34,    -1,    36,    -1,    -1,    -1,
    -1,    41,    42,     3,     4,     5,     6,     7,     8,     9,
    10,    -1,    12,    -1,    14,    15,    16,    57,    -1,    -1,
    60,    -1,    22,    -1,    -1,    65,    -1,    -1,    -1,    -1,
    -1,    -1,    32,    33,    34,    -1,    36,    -1,    -1,    -1,
    -1,    41,    42,     3,     4,     5,     6,     7,     8,     9,
    10,    -1,    12,    -1,    14,    15,    16,    57,    -1,    -1,
    60,    -1,    -1,    -1,    -1,    65,    -1,    -1,    -1,    -1,
    -1,    -1,    32,    33,    34,    -1,    36,    -1,    -1,    -1,
    -1,    41,    42,     3,     4,     5,     6,     7,     8,     9,
    10,    -1,    12,    -1,    14,    15,    16,    57,    -1,    -1,
    60,    -1,    -1,    -1,    -1,    65,    -1,    -1,    -1,    -1,
    -1,    -1,    32,    33,    34,    -1,    36,    -1,    -1,    -1,
    -1,    41,    42,     3,     4,     5,     6,     7,     8,     9,
    10,    -1,    12,    -1,    14,    15,    16,    57,    -1,    -1,
    60,    -1,    -1,    -1,    -1,    65,    -1,    -1,    -1,    -1,
    -1,    -1,    32,    33,    34,    -1,    36,    -1,    -1,    -1,
    -1,    41,    42,     3,     4,     5,     6,     7,     8,     9,
    10,    -1,    12,    -1,    14,    15,    16,    57,    -1,    -1,
    60,    -1,    -1,    -1,    -1,    65,    -1,    -1,    -1,    -1,
    -1,    -1,    32,    33,    34,    -1,    36,    -1,    -1,    -1,
    -1,    41,    42,    -1,    -1,    -1,    -1,    -1,    32,    33,
    34,    -1,    36,    37,    38,    39,    40,    57,    42,    -1,
    60,    17,    18,    19,    20,    65,    -1,    23,    24,    25,
    26,    27,    28,    29,    58,    31,    32,    33,    34,    -1,
    36,    37,    38,    39,    40,    -1,    42,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,    18,
    19,    20,    58,    -1,    23,    24,    25,    26,    27,    28,
    29,    67,    31,    32,    33,    34,    -1,    36,    37,    38,
    39,    40,    -1,    42,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,
    17,    18,    19,    20,    -1,    64,    23,    24,    25,    26,
    27,    28,    29,    -1,    31,    32,    33,    34,    -1,    36,
    37,    38,    39,    40,    -1,    42,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    58,    17,    18,    19,    20,    -1,    64,    23,    24,
    25,    26,    27,    28,    29,    -1,    31,    32,    33,    34,
    -1,    36,    37,    38,    39,    40,    43,    42,    45,    46,
    47,    48,    49,    50,    51,    -1,    -1,    -1,    55,    -1,
    57,    -1,    -1,    58,    -1,    -1,    63,    62,    17,    18,
    19,    20,    -1,    -1,    23,    24,    25,    26,    27,    28,
    29,    -1,    31,    32,    33,    34,    -1,    36,    37,    38,
    39,    40,    -1,    42,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,
    -1,    -1,    -1,    62,    17,    18,    19,    20,    -1,    -1,
    23,    24,    25,    26,    27,    28,    29,    -1,    31,    32,
    33,    34,    -1,    36,    37,    38,    39,    40,    -1,    42,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    58,    -1,    -1,    -1,    62,
    17,    18,    19,    20,    -1,    -1,    23,    24,    25,    26,
    27,    28,    29,    -1,    31,    32,    33,    34,    -1,    36,
    37,    38,    39,    40,    -1,    42,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    58,    -1,    -1,    -1,    62,    17,    18,    19,    20,
    -1,    -1,    23,    24,    25,    26,    27,    28,    29,    -1,
    31,    32,    33,    34,    -1,    36,    37,    38,    39,    40,
    -1,    42,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,    -1,    -1,
    -1,    62,    17,    18,    19,    20,    -1,    -1,    23,    24,
    25,    26,    27,    28,    29,    -1,    31,    32,    33,    34,
    -1,    36,    37,    38,    39,    40,    -1,    42,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    58,    -1,    -1,    -1,    62,    17,    18,
    19,    20,    -1,    -1,    23,    24,    25,    26,    27,    28,
    29,    -1,    31,    32,    33,    34,    -1,    36,    37,    38,
    39,    40,    -1,    42,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,
    -1,    -1,    -1,    62,    17,    18,    19,    20,    -1,    -1,
    23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
    33,    34,    -1,    36,    37,    38,    39,    40,    -1,    42,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    52,
    17,    18,    19,    20,    -1,    58,    23,    24,    25,    26,
    27,    28,    29,    30,    31,    32,    33,    34,    -1,    36,
    37,    38,    39,    40,    -1,    42,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    52,    17,    18,    19,    20,
    -1,    58,    23,    24,    25,    26,    27,    28,    29,    30,
    31,    32,    33,    34,    -1,    36,    37,    38,    39,    40,
    -1,    42,    -1,    -1,    17,    18,    19,    20,    -1,    -1,
    23,    24,    25,    26,    27,    28,    29,    58,    31,    32,
    33,    34,    -1,    36,    37,    38,    39,    40,    -1,    42,
    -1,    -1,    17,    18,    19,    20,    -1,    -1,    23,    24,
    25,    26,    27,    28,    29,    58,    31,    32,    33,    34,
    -1,    36,    37,    38,    39,    40,    -1,    42,    -1,    -1,
    -1,    23,    24,    25,    26,    27,    28,    29,    -1,    31,
    32,    33,    34,    58,    36,    37,    38,    39,    40,    -1,
    42,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    58
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

case 2:
#line 208 "./ada-exp.y"
{ write_exp_elt_opcode (OP_TYPE);
			  write_exp_elt_type (yyvsp[0].tval);
 			  write_exp_elt_opcode (OP_TYPE); ;
    break;}
case 4:
#line 216 "./ada-exp.y"
{ write_exp_elt_opcode (BINOP_COMMA); ;
    break;}
case 5:
#line 221 "./ada-exp.y"
{ write_exp_elt_opcode (UNOP_IND); ;
    break;}
case 6:
#line 225 "./ada-exp.y"
{ write_exp_elt_opcode (STRUCTOP_STRUCT);
			  write_exp_string (yyvsp[0].ssym.stoken);
			  write_exp_elt_opcode (STRUCTOP_STRUCT); 
			  ;
    break;}
case 7:
#line 232 "./ada-exp.y"
{
			  write_exp_elt_opcode (OP_FUNCALL);
			  write_exp_elt_longcst (yyvsp[-1].lval);
			  write_exp_elt_opcode (OP_FUNCALL);
		        ;
    break;}
case 8:
#line 240 "./ada-exp.y"
{
			  write_exp_elt_opcode (UNOP_CAST);
			  write_exp_elt_type (yyvsp[-3].tval);
			  write_exp_elt_opcode (UNOP_CAST); 
			;
    break;}
case 9:
#line 247 "./ada-exp.y"
{ type_qualifier = yyvsp[-2].tval; ;
    break;}
case 10:
#line 248 "./ada-exp.y"
{
			  /*			  write_exp_elt_opcode (UNOP_QUAL); */
			  /* FIXME: UNOP_QUAL should be defined in expression.h */
			  write_exp_elt_type (yyvsp[-6].tval);
			  /* write_exp_elt_opcode (UNOP_QUAL); */
			  /* FIXME: UNOP_QUAL should be defined in expression.h */
			  type_qualifier = yyvsp[-4].tval;
			;
    break;}
case 11:
#line 258 "./ada-exp.y"
{ yyval.tval = type_qualifier; ;
    break;}
case 12:
#line 262 "./ada-exp.y"
{ write_exp_elt_opcode (TERNOP_SLICE); ;
    break;}
case 13:
#line 265 "./ada-exp.y"
{ ;
    break;}
case 15:
#line 272 "./ada-exp.y"
{ write_exp_elt_opcode (OP_REGISTER);
			  write_exp_elt_longcst ((LONGEST) yyvsp[0].lval);
			  write_exp_elt_opcode (OP_REGISTER); 
			;
    break;}
case 16:
#line 279 "./ada-exp.y"
{ write_exp_elt_opcode (OP_INTERNALVAR);
			  write_exp_elt_intern (yyvsp[0].ivar);
			  write_exp_elt_opcode (OP_INTERNALVAR); 
			;
    break;}
case 18:
#line 290 "./ada-exp.y"
{ write_exp_elt_opcode (OP_LAST);
			  write_exp_elt_longcst ((LONGEST) yyvsp[0].lval);
			  write_exp_elt_opcode (OP_LAST); 
			 ;
    break;}
case 19:
#line 297 "./ada-exp.y"
{ write_exp_elt_opcode (BINOP_ASSIGN); ;
    break;}
case 20:
#line 301 "./ada-exp.y"
{ write_exp_elt_opcode (UNOP_NEG); ;
    break;}
case 21:
#line 305 "./ada-exp.y"
{ write_exp_elt_opcode (UNOP_PLUS); ;
    break;}
case 22:
#line 309 "./ada-exp.y"
{ write_exp_elt_opcode (UNOP_LOGICAL_NOT); ;
    break;}
case 23:
#line 313 "./ada-exp.y"
{ write_exp_elt_opcode (UNOP_ABS); ;
    break;}
case 24:
#line 316 "./ada-exp.y"
{ yyval.lval = 0; ;
    break;}
case 25:
#line 320 "./ada-exp.y"
{ yyval.lval = 1; ;
    break;}
case 26:
#line 322 "./ada-exp.y"
{ yyval.lval = 1; ;
    break;}
case 27:
#line 324 "./ada-exp.y"
{ yyval.lval = yyvsp[-2].lval + 1; ;
    break;}
case 28:
#line 326 "./ada-exp.y"
{ yyval.lval = yyvsp[-4].lval + 1; ;
    break;}
case 29:
#line 331 "./ada-exp.y"
{ write_exp_elt_opcode (UNOP_MEMVAL);
			  write_exp_elt_type (yyvsp[-2].tval);
			  write_exp_elt_opcode (UNOP_MEMVAL); 
			;
    break;}
case 30:
#line 340 "./ada-exp.y"
{ write_exp_elt_opcode (BINOP_EXP); ;
    break;}
case 31:
#line 344 "./ada-exp.y"
{ write_exp_elt_opcode (BINOP_MUL); ;
    break;}
case 32:
#line 348 "./ada-exp.y"
{ write_exp_elt_opcode (BINOP_DIV); ;
    break;}
case 33:
#line 352 "./ada-exp.y"
{ write_exp_elt_opcode (BINOP_REM); ;
    break;}
case 34:
#line 356 "./ada-exp.y"
{ write_exp_elt_opcode (BINOP_MOD); ;
    break;}
case 35:
#line 360 "./ada-exp.y"
{ write_exp_elt_opcode (BINOP_REPEAT); ;
    break;}
case 36:
#line 364 "./ada-exp.y"
{ write_exp_elt_opcode (BINOP_ADD); ;
    break;}
case 37:
#line 368 "./ada-exp.y"
{ write_exp_elt_opcode (BINOP_CONCAT); ;
    break;}
case 38:
#line 372 "./ada-exp.y"
{ write_exp_elt_opcode (BINOP_SUB); ;
    break;}
case 39:
#line 376 "./ada-exp.y"
{ write_exp_elt_opcode (BINOP_EQUAL); ;
    break;}
case 40:
#line 380 "./ada-exp.y"
{ write_exp_elt_opcode (BINOP_NOTEQUAL); ;
    break;}
case 41:
#line 384 "./ada-exp.y"
{ write_exp_elt_opcode (BINOP_LEQ); ;
    break;}
case 42:
#line 388 "./ada-exp.y"
{ /*write_exp_elt_opcode (TERNOP_MBR); */ ;
    break;}
case 43:
#line 392 "./ada-exp.y"
{ /*write_exp_elt_opcode (BINOP_MBR); */
			  /* FIXME: BINOP_MBR should be defined in expression.h */
			  write_exp_elt_longcst ((LONGEST) yyvsp[0].lval);
			  /*write_exp_elt_opcode (BINOP_MBR); */
			;
    break;}
case 44:
#line 398 "./ada-exp.y"
{ /*write_exp_elt_opcode (UNOP_MBR); */
			  /* FIXME: UNOP_QUAL should be defined in expression.h */			  
		          write_exp_elt_type (yyvsp[0].tval);
			  /*		          write_exp_elt_opcode (UNOP_MBR); */
			  /* FIXME: UNOP_MBR should be defined in expression.h */			  
			;
    break;}
case 45:
#line 405 "./ada-exp.y"
{ /*write_exp_elt_opcode (TERNOP_MBR); */
			  /* FIXME: TERNOP_MBR should be defined in expression.h */			  			  
		          write_exp_elt_opcode (UNOP_LOGICAL_NOT); 
			;
    break;}
case 46:
#line 410 "./ada-exp.y"
{ /* write_exp_elt_opcode (BINOP_MBR); */
			  /* FIXME: BINOP_MBR should be defined in expression.h */
			  write_exp_elt_longcst ((LONGEST) yyvsp[0].lval);
			  /*write_exp_elt_opcode (BINOP_MBR);*/
			  /* FIXME: BINOP_MBR should be defined in expression.h */			  
		          write_exp_elt_opcode (UNOP_LOGICAL_NOT); 
			;
    break;}
case 47:
#line 418 "./ada-exp.y"
{ /*write_exp_elt_opcode (UNOP_MBR);*/
			  /* FIXME: UNOP_MBR should be defined in expression.h */			  
		          write_exp_elt_type (yyvsp[0].tval);
			  /*		          write_exp_elt_opcode (UNOP_MBR);*/
			  /* FIXME: UNOP_MBR should be defined in expression.h */			  			  
		          write_exp_elt_opcode (UNOP_LOGICAL_NOT); 
			;
    break;}
case 48:
#line 428 "./ada-exp.y"
{ write_exp_elt_opcode (BINOP_GEQ); ;
    break;}
case 49:
#line 432 "./ada-exp.y"
{ write_exp_elt_opcode (BINOP_LESS); ;
    break;}
case 50:
#line 436 "./ada-exp.y"
{ write_exp_elt_opcode (BINOP_GTR); ;
    break;}
case 51:
#line 440 "./ada-exp.y"
{ write_exp_elt_opcode (BINOP_BITWISE_AND); ;
    break;}
case 52:
#line 444 "./ada-exp.y"
{ write_exp_elt_opcode (BINOP_LOGICAL_AND); ;
    break;}
case 53:
#line 448 "./ada-exp.y"
{ write_exp_elt_opcode (BINOP_BITWISE_IOR); ;
    break;}
case 54:
#line 452 "./ada-exp.y"
{ write_exp_elt_opcode (BINOP_LOGICAL_OR); ;
    break;}
case 55:
#line 456 "./ada-exp.y"
{ write_exp_elt_opcode (BINOP_BITWISE_XOR); ;
    break;}
case 56:
#line 460 "./ada-exp.y"
{ write_exp_elt_opcode (UNOP_ADDR); ;
    break;}
case 57:
#line 462 "./ada-exp.y"
{ write_exp_elt_opcode (UNOP_ADDR);
			  write_exp_elt_opcode (UNOP_CAST);
			  write_exp_elt_type (builtin_type_ada_system_address);
			  write_exp_elt_opcode (UNOP_CAST);
			;
    break;}
case 58:
#line 468 "./ada-exp.y"
{ write_attribute_call1 (ATR_FIRST, yyvsp[0].lval); ;
    break;}
case 59:
#line 470 "./ada-exp.y"
{ write_attribute_call1 (ATR_LAST, yyvsp[0].lval); ;
    break;}
case 60:
#line 472 "./ada-exp.y"
{ write_attribute_call1 (ATR_LENGTH, yyvsp[0].lval); ;
    break;}
case 61:
#line 474 "./ada-exp.y"
{ write_attribute_call0 (ATR_SIZE); ;
    break;}
case 62:
#line 476 "./ada-exp.y"
{ write_attribute_call0 (ATR_TAG); ;
    break;}
case 63:
#line 478 "./ada-exp.y"
{ write_attribute_calln (ATR_MIN, 2); ;
    break;}
case 64:
#line 480 "./ada-exp.y"
{ write_attribute_calln (ATR_MAX, 2); ;
    break;}
case 65:
#line 482 "./ada-exp.y"
{ write_attribute_calln (ATR_POS, 1); ;
    break;}
case 66:
#line 484 "./ada-exp.y"
{ write_attribute_call1 (ATR_FIRST, yyvsp[0].lval); ;
    break;}
case 67:
#line 486 "./ada-exp.y"
{ write_attribute_call1 (ATR_LAST, yyvsp[0].lval); ;
    break;}
case 68:
#line 488 "./ada-exp.y"
{ write_attribute_call1 (ATR_LENGTH, yyvsp[0].lval); ;
    break;}
case 69:
#line 490 "./ada-exp.y"
{ write_attribute_calln (ATR_VAL, 1); ;
    break;}
case 70:
#line 492 "./ada-exp.y"
{ write_attribute_call0 (ATR_MODULUS); ;
    break;}
case 71:
#line 496 "./ada-exp.y"
{ yyval.lval = 1; ;
    break;}
case 72:
#line 498 "./ada-exp.y"
{ yyval.lval = yyvsp[-1].typed_val.val; ;
    break;}
case 73:
#line 503 "./ada-exp.y"
{ write_exp_elt_opcode (OP_TYPE);
			  write_exp_elt_type (yyvsp[0].tval);
			  write_exp_elt_opcode (OP_TYPE); ;
    break;}
case 75:
#line 511 "./ada-exp.y"
{ write_exp_elt_opcode (OP_TYPE);
			  write_exp_elt_type (builtin_type_void);
			  write_exp_elt_opcode (OP_TYPE); ;
    break;}
case 76:
#line 518 "./ada-exp.y"
{ write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type (yyvsp[0].typed_val.type);
			  write_exp_elt_longcst ((LONGEST)(yyvsp[0].typed_val.val));
			  write_exp_elt_opcode (OP_LONG); 
			;
    break;}
case 77:
#line 526 "./ada-exp.y"
{ write_exp_elt_opcode (OP_LONG);
			  if (type_qualifier == NULL) 
			    write_exp_elt_type (yyvsp[0].typed_val.type);
			  else
			    write_exp_elt_type (type_qualifier);
			  write_exp_elt_longcst 
			    (convert_char_literal (type_qualifier, yyvsp[0].typed_val.val));
			  write_exp_elt_opcode (OP_LONG); 
			;
    break;}
case 78:
#line 538 "./ada-exp.y"
{ write_exp_elt_opcode (OP_DOUBLE);
			  write_exp_elt_type (yyvsp[0].typed_val_float.type);
			  write_exp_elt_dblcst (yyvsp[0].typed_val_float.dval);
			  write_exp_elt_opcode (OP_DOUBLE); 
			;
    break;}
case 79:
#line 546 "./ada-exp.y"
{ write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type (builtin_type_int);
			  write_exp_elt_longcst ((LONGEST)(0));
			  write_exp_elt_opcode (OP_LONG); 
			 ;
    break;}
case 80:
#line 553 "./ada-exp.y"
{ /* Ada strings are converted into array constants 
			     a lower bound of 1.  Thus, the array upper bound 
			     is the string length. */
			  char *sp = yyvsp[0].sval.ptr; int count;
			  if (yyvsp[0].sval.length == 0) 
			    { /* One dummy character for the type */
			      write_exp_elt_opcode (OP_LONG);
			      write_exp_elt_type (builtin_type_ada_char);
			      write_exp_elt_longcst ((LONGEST)(0));
			      write_exp_elt_opcode (OP_LONG);
			    }
			  for (count = yyvsp[0].sval.length; count > 0; count -= 1)
			    {
			      write_exp_elt_opcode (OP_LONG);
			      write_exp_elt_type (builtin_type_ada_char);
			      write_exp_elt_longcst ((LONGEST)(*sp));
			      sp += 1;
			      write_exp_elt_opcode (OP_LONG);
			    }
			  write_exp_elt_opcode (OP_ARRAY);
			  write_exp_elt_longcst ((LONGEST) 1);
			  write_exp_elt_longcst ((LONGEST) (yyvsp[0].sval.length));
			  write_exp_elt_opcode (OP_ARRAY); 
			 ;
    break;}
case 81:
#line 580 "./ada-exp.y"
{ error ("NEW not implemented."); ;
    break;}
case 82:
#line 583 "./ada-exp.y"
{ write_var_from_name (NULL, yyvsp[0].ssym); ;
    break;}
case 83:
#line 585 "./ada-exp.y"
{ write_var_from_name (yyvsp[-1].bval, yyvsp[0].ssym); ;
    break;}
case 84:
#line 586 "./ada-exp.y"
{ write_object_renaming (NULL, yyvsp[0].ssym.sym); ;
    break;}
case 85:
#line 588 "./ada-exp.y"
{ write_object_renaming (yyvsp[-1].bval, yyvsp[0].ssym.sym); ;
    break;}
case 86:
#line 591 "./ada-exp.y"
{ ;
    break;}
case 87:
#line 592 "./ada-exp.y"
{ ;
    break;}
case 88:
#line 593 "./ada-exp.y"
{ ;
    break;}
case 89:
#line 597 "./ada-exp.y"
{ yyval.bval = yyvsp[0].bval; ;
    break;}
case 90:
#line 599 "./ada-exp.y"
{ yyval.bval = yyvsp[0].bval; ;
    break;}
case 91:
#line 603 "./ada-exp.y"
{ yyval.tval = yyvsp[0].tval; ;
    break;}
case 92:
#line 604 "./ada-exp.y"
{ yyval.tval = yyvsp[0].tval; ;
    break;}
case 93:
#line 606 "./ada-exp.y"
{ yyval.tval = lookup_pointer_type (yyvsp[-1].tval); ;
    break;}
case 94:
#line 608 "./ada-exp.y"
{ yyval.tval = lookup_pointer_type (yyvsp[-1].tval); ;
    break;}
case 95:
#line 615 "./ada-exp.y"
{ write_exp_elt_opcode (UNOP_IND); ;
    break;}
case 96:
#line 617 "./ada-exp.y"
{ write_exp_elt_opcode (UNOP_ADDR); ;
    break;}
case 97:
#line 619 "./ada-exp.y"
{ write_exp_elt_opcode (BINOP_SUBSCRIPT); ;
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
#line 622 "./ada-exp.y"


/* yylex defined in ada-lex.c: Reads one token, getting characters */
/* through lexptr.  */

/* Remap normal flex interface names (yylex) as well as gratuitiously */
/* global symbol names, so we can have multiple flex-generated parsers */
/* in gdb.  */

/* (See note above on previous definitions for YACC.) */

#define yy_create_buffer ada_yy_create_buffer
#define yy_delete_buffer ada_yy_delete_buffer
#define yy_init_buffer ada_yy_init_buffer
#define yy_load_buffer_state ada_yy_load_buffer_state
#define yy_switch_to_buffer ada_yy_switch_to_buffer
#define yyrestart ada_yyrestart
#define yytext ada_yytext
#define yywrap ada_yywrap

/* The following kludge was found necessary to prevent conflicts between */
/* defs.h and non-standard stdlib.h files.  */
#define qsort __qsort__dummy
#include "ada-lex.c"

int
ada_parse ()
{
  lexer_init (yyin);		/* (Re-)initialize lexer. */
  left_block_context = NULL;
  type_qualifier = NULL;
  
  return _ada_parse ();
}

void
yyerror (msg)
     char *msg;
{
  error ("A %s in expression, near `%s'.", (msg ? msg : "error"), lexptr);
}

/* The operator name corresponding to operator symbol STRING (adds 
   quotes and maps to lower-case).  Destroys the previous contents of
   the array pointed to by STRING.ptr.  Error if STRING does not match
   a valid Ada operator.  Assumes that STRING.ptr points to a
   null-terminated string and that, if STRING is a valid operator
   symbol, the array pointed to by STRING.ptr contains at least
   STRING.length+3 characters. */ 

static struct stoken
string_to_operator (string)
     struct stoken string;
{
  int i;

  for (i = 0; ada_opname_table[i].mangled != NULL; i += 1)
    {
      if (string.length == strlen (ada_opname_table[i].demangled)-2
	  && strncasecmp (string.ptr, ada_opname_table[i].demangled+1,
			  string.length) == 0)
	{
	  strncpy (string.ptr, ada_opname_table[i].demangled,
		   string.length+2);
	  string.length += 2;
	  return string;
	}
    }
  error ("Invalid operator symbol `%s'", string.ptr);
}

/* Emit expression to access an instance of SYM, in block BLOCK (if
 * non-NULL), and with :: qualification ORIG_LEFT_CONTEXT. */
static void
write_var_from_sym (orig_left_context, block, sym)
     struct block* orig_left_context;
     struct block* block;
     struct symbol* sym;
{
  if (orig_left_context == NULL && symbol_read_needs_frame (sym))
    {
      if (innermost_block == 0 ||
	  contained_in (block, innermost_block))
	innermost_block = block;
    }

  write_exp_elt_opcode (OP_VAR_VALUE);
  /* We want to use the selected frame, not another more inner frame
     which happens to be in the same block */
  write_exp_elt_block (NULL);
  write_exp_elt_sym (sym);
  write_exp_elt_opcode (OP_VAR_VALUE);
}

/* Emit expression to access an instance of NAME. */
static void
write_var_from_name (orig_left_context, name)
     struct block* orig_left_context;
     struct name_info name;
{
  if (name.msym != NULL)
    {
      write_exp_msymbol (name.msym, 
			 lookup_function_type (builtin_type_int),
			 builtin_type_int);
    }
  else if (name.sym == NULL) 
    {
      /* Multiple matches: record name and starting block for later 
         resolution by ada_resolve. */
      /*      write_exp_elt_opcode (OP_UNRESOLVED_VALUE); */
      /* FIXME: OP_UNRESOLVED_VALUE should be defined in expression.h */      
      write_exp_elt_block (name.block);
      /*      write_exp_elt_name (name.stoken.ptr); */
      /* FIXME: write_exp_elt_name should be defined in defs.h, located in parse.c */      
      /*      write_exp_elt_opcode (OP_UNRESOLVED_VALUE); */
      /* FIXME: OP_UNRESOLVED_VALUE should be defined in expression.h */      
    }
  else
    write_var_from_sym (orig_left_context, name.block, name.sym);
}

/* Write a call on parameterless attribute ATR.  */

static void
write_attribute_call0 (atr)
     enum ada_attribute atr;
{
  /*  write_exp_elt_opcode (OP_ATTRIBUTE); */
  /* FIXME: OP_ATTRIBUTE should be defined in expression.h */      
  write_exp_elt_longcst ((LONGEST) 0);
  write_exp_elt_longcst ((LONGEST) atr);
  /*  write_exp_elt_opcode (OP_ATTRIBUTE); */
  /* FIXME: OP_ATTRIBUTE should be defined in expression.h */      
}

/* Write a call on an attribute ATR with one constant integer
 * parameter. */

static void
write_attribute_call1 (atr, arg)
     enum ada_attribute atr;
     LONGEST arg;
{
  write_exp_elt_opcode (OP_LONG);
  write_exp_elt_type (builtin_type_int);
  write_exp_elt_longcst (arg);
  write_exp_elt_opcode (OP_LONG);
  /*write_exp_elt_opcode (OP_ATTRIBUTE);*/
  /* FIXME: OP_ATTRIBUTE should be defined in expression.h */
  write_exp_elt_longcst ((LONGEST) 1);
  write_exp_elt_longcst ((LONGEST) atr);
  /*write_exp_elt_opcode (OP_ATTRIBUTE);*/
  /* FIXME: OP_ATTRIBUTE should be defined in expression.h */        
}  

/* Write a call on an attribute ATR with N parameters, whose code must have
 * been generated previously. */

static void
write_attribute_calln (atr, n)
     enum ada_attribute atr;
     int n;
{
  /*write_exp_elt_opcode (OP_ATTRIBUTE);*/
  /* FIXME: OP_ATTRIBUTE should be defined in expression.h */      
  write_exp_elt_longcst ((LONGEST) n);
  write_exp_elt_longcst ((LONGEST) atr);
  /*  write_exp_elt_opcode (OP_ATTRIBUTE);*/
  /* FIXME: OP_ATTRIBUTE should be defined in expression.h */        
}  

/* Emit expression corresponding to the renamed object designated by 
 * the type RENAMING, which must be the referent of an object renaming
 * type, in the context of ORIG_LEFT_CONTEXT (?). */
static void
write_object_renaming (orig_left_context, renaming)
     struct block* orig_left_context;
     struct symbol* renaming;
{
  const char* qualification = SYMBOL_NAME (renaming);
  const char* simple_tail;
  const char* expr = TYPE_FIELD_NAME (SYMBOL_TYPE (renaming), 0);
  const char* suffix;
  char* name;
  struct symbol* sym;
  enum { SIMPLE_INDEX, LOWER_BOUND, UPPER_BOUND } slice_state;

  /* if orig_left_context is null, then use the currently selected
     block, otherwise we might fail our symbol lookup below */
  if (orig_left_context == NULL)
    orig_left_context = get_selected_block (NULL);

  for (simple_tail = qualification + strlen (qualification); 
       simple_tail != qualification; simple_tail -= 1)
    {
      if (*simple_tail == '.')
	{
	  simple_tail += 1;
	  break;
	} 
      else if (STREQN (simple_tail, "__", 2))
	{
	  simple_tail += 2;
	  break;
	}
    }

  suffix = strstr (expr, "___XE");
  if (suffix == NULL)
    goto BadEncoding;

  name = (char*) xmalloc (suffix - expr + 1);
  /*  add_name_string_cleanup (name); */
  /* FIXME: add_name_string_cleanup should be defined in
     parser-defs.h, implemented in parse.c */    
  strncpy (name, expr, suffix-expr);
  name[suffix-expr] = '\000';
  sym = lookup_symbol (name, orig_left_context, VAR_NAMESPACE, 0, NULL);
  /*  if (sym == NULL) 
    error ("Could not find renamed variable: %s", ada_demangle (name));
  */
  /* FIXME: ada_demangle should be defined in defs.h, implemented in ada-lang.c */  
  write_var_from_sym (orig_left_context, block_found, sym);

  suffix += 5;
  slice_state = SIMPLE_INDEX;
  while (*suffix == 'X') 
    {
      suffix += 1;

      switch (*suffix) {
      case 'L':
	slice_state = LOWER_BOUND;
      case 'S':
	suffix += 1;
	if (isdigit (*suffix)) 
	  {
	    char* next;
	    long val = strtol (suffix, &next, 10);
	    if (next == suffix) 
	      goto BadEncoding;
	    suffix = next;
	    write_exp_elt_opcode (OP_LONG);
	    write_exp_elt_type (builtin_type_ada_int);
	    write_exp_elt_longcst ((LONGEST) val);
	    write_exp_elt_opcode (OP_LONG);
	  } 
	else
	  {
	    const char* end;
	    char* index_name;
	    int index_len;
	    struct symbol* index_sym;

	    end = strchr (suffix, 'X');
	    if (end == NULL) 
	      end = suffix + strlen (suffix);
	    
	    index_len = simple_tail - qualification + 2 + (suffix - end) + 1;
	    index_name = (char*) xmalloc (index_len);
	    memset (index_name, '\000', index_len);
	    /*	    add_name_string_cleanup (index_name);*/
	    /* FIXME: add_name_string_cleanup should be defined in
	       parser-defs.h, implemented in parse.c */    	    
	    strncpy (index_name, qualification, simple_tail - qualification);
	    index_name[simple_tail - qualification] = '\000';
	    strncat (index_name, suffix, suffix-end);
	    suffix = end;

	    index_sym = 
	      lookup_symbol (index_name, NULL, VAR_NAMESPACE, 0, NULL);
	    if (index_sym == NULL)
	      error ("Could not find %s", index_name);
	    write_var_from_sym (NULL, block_found, sym);
	  }
	if (slice_state == SIMPLE_INDEX)
	  { 
	    write_exp_elt_opcode (OP_FUNCALL);
	    write_exp_elt_longcst ((LONGEST) 1);
	    write_exp_elt_opcode (OP_FUNCALL);
	  }
	else if (slice_state == LOWER_BOUND)
	  slice_state = UPPER_BOUND;
	else if (slice_state == UPPER_BOUND)
	  {
	    write_exp_elt_opcode (TERNOP_SLICE);
	    slice_state = SIMPLE_INDEX;
	  }
	break;

      case 'R':
	{
	  struct stoken field_name;
	  const char* end;
	  suffix += 1;
	  
	  if (slice_state != SIMPLE_INDEX)
	    goto BadEncoding;
	  end = strchr (suffix, 'X');
	  if (end == NULL) 
	    end = suffix + strlen (suffix);
	  field_name.length = end - suffix;
	  field_name.ptr = (char*) xmalloc (end - suffix + 1);
	  strncpy (field_name.ptr, suffix, end - suffix);
	  field_name.ptr[end - suffix] = '\000';
	  suffix = end;
	  write_exp_elt_opcode (STRUCTOP_STRUCT);
	  write_exp_string (field_name);
	  write_exp_elt_opcode (STRUCTOP_STRUCT); 	  
	  break;
	}
	  
      default:
	goto BadEncoding;
      }
    }
  if (slice_state == SIMPLE_INDEX)
    return;

 BadEncoding:
  error ("Internal error in encoding of renaming declaration: %s",
	 SYMBOL_NAME (renaming));
}

/* Convert the character literal whose ASCII value would be VAL to the
   appropriate value of type TYPE, if there is a translation.
   Otherwise return VAL.  Hence, in an enumeration type ('A', 'B'), 
   the literal 'A' (VAL == 65), returns 0. */
static LONGEST
convert_char_literal (struct type* type, LONGEST val)
{
  char name[7];
  int f;

  if (type == NULL || TYPE_CODE (type) != TYPE_CODE_ENUM)
    return val;
  sprintf (name, "QU%02x", (int) val);
  for (f = 0; f < TYPE_NFIELDS (type); f += 1) 
    {
      if (STREQ (name, TYPE_FIELD_NAME (type, f)))
	return TYPE_FIELD_BITPOS (type, f);
    }
  return val;
}
