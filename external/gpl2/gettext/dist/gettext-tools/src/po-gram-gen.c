/* A Bison parser, made from po-gram-gen.y
   by GNU bison 1.35.  */

#define YYBISON 1  /* Identify Bison output.  */

# define	COMMENT	257
# define	DOMAIN	258
# define	JUNK	259
# define	PREV_MSGCTXT	260
# define	PREV_MSGID	261
# define	PREV_MSGID_PLURAL	262
# define	PREV_STRING	263
# define	MSGCTXT	264
# define	MSGID	265
# define	MSGID_PLURAL	266
# define	MSGSTR	267
# define	NAME	268
# define	NUMBER	269
# define	STRING	270

#line 20 "po-gram-gen.y"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* Specification.  */
#include "po-gram.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "str-list.h"
#include "po-lex.h"
#include "po-charset.h"
#include "error.h"
#include "xalloc.h"
#include "gettext.h"
#include "read-catalog-abstract.h"

#define _(str) gettext (str)

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror, etc),
   as well as gratuitiously global symbol names, so we can have multiple
   yacc generated parsers in the same program.  Note that these are only
   the variables produced by yacc.  If other parser generators (bison,
   byacc, etc) produce additional global names that conflict at link time,
   then those parser generators need to be fixed instead of adding those
   names to this list. */

#define yymaxdepth po_gram_maxdepth
#define yyparse po_gram_parse
#define yylex   po_gram_lex
#define yyerror po_gram_error
#define yylval  po_gram_lval
#define yychar  po_gram_char
#define yydebug po_gram_debug
#define yypact  po_gram_pact
#define yyr1    po_gram_r1
#define yyr2    po_gram_r2
#define yydef   po_gram_def
#define yychk   po_gram_chk
#define yypgo   po_gram_pgo
#define yyact   po_gram_act
#define yyexca  po_gram_exca
#define yyerrflag po_gram_errflag
#define yynerrs po_gram_nerrs
#define yyps    po_gram_ps
#define yypv    po_gram_pv
#define yys     po_gram_s
#define yy_yys  po_gram_yys
#define yystate po_gram_state
#define yytmp   po_gram_tmp
#define yyv     po_gram_v
#define yy_yyv  po_gram_yyv
#define yyval   po_gram_val
#define yylloc  po_gram_lloc
#define yyreds  po_gram_reds          /* With YYDEBUG defined */
#define yytoks  po_gram_toks          /* With YYDEBUG defined */
#define yylhs   po_gram_yylhs
#define yylen   po_gram_yylen
#define yydefred po_gram_yydefred
#define yydgoto po_gram_yydgoto
#define yysindex po_gram_yysindex
#define yyrindex po_gram_yyrindex
#define yygindex po_gram_yygindex
#define yytable  po_gram_yytable
#define yycheck  po_gram_yycheck

static long plural_counter;

#define check_obsolete(value1,value2) \
  if ((value1).obsolete != (value2).obsolete) \
    po_gram_error_at_line (&(value2).pos, _("inconsistent use of #~"));

static inline void
do_callback_message (char *msgctxt,
		     char *msgid, lex_pos_ty *msgid_pos, char *msgid_plural,
		     char *msgstr, size_t msgstr_len, lex_pos_ty *msgstr_pos,
		     char *prev_msgctxt,
		     char *prev_msgid, char *prev_msgid_plural,
		     bool obsolete)
{
  /* Test for header entry.  Ignore fuzziness of the header entry.  */
  if (msgctxt == NULL && msgid[0] == '\0' && !obsolete)
    po_lex_charset_set (msgstr, gram_pos.file_name);

  po_callback_message (msgctxt,
		       msgid, msgid_pos, msgid_plural,
		       msgstr, msgstr_len, msgstr_pos,
		       prev_msgctxt, prev_msgid, prev_msgid_plural,
		       false, obsolete);
}

#define free_message_intro(value) \
  if ((value).prev_ctxt != NULL)	\
    free ((value).prev_ctxt);		\
  if ((value).prev_id != NULL)		\
    free ((value).prev_id);		\
  if ((value).prev_id_plural != NULL)	\
    free ((value).prev_id_plural);	\
  if ((value).ctxt != NULL)		\
    free ((value).ctxt);


#line 143 "po-gram-gen.y"
#ifndef YYSTYPE
typedef union
{
  struct { char *string; lex_pos_ty pos; bool obsolete; } string;
  struct { string_list_ty stringlist; lex_pos_ty pos; bool obsolete; } stringlist;
  struct { long number; lex_pos_ty pos; bool obsolete; } number;
  struct { lex_pos_ty pos; bool obsolete; } pos;
  struct { char *ctxt; char *id; char *id_plural; lex_pos_ty pos; bool obsolete; } prev;
  struct { char *prev_ctxt; char *prev_id; char *prev_id_plural; char *ctxt; lex_pos_ty pos; bool obsolete; } message_intro;
  struct { struct msgstr_def rhs; lex_pos_ty pos; bool obsolete; } rhs;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
#ifndef YYDEBUG
# define YYDEBUG 0
#endif



#define	YYFINAL		46
#define	YYFLAG		-32768
#define	YYNTBASE	19

/* YYTRANSLATE(YYLEX) -- Bison token number corresponding to YYLEX. */
#define YYTRANSLATE(x) ((unsigned)(x) <= 270 ? yytranslate[x] : 33)

/* YYTRANSLATE[YYLEX] -- Bison token number corresponding to YYLEX. */
static const char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    15,     2,    16,     2,     2,     2,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    17,
      18
};

#if YYDEBUG
static const short yyprhs[] =
{
       0,     0,     1,     4,     7,    10,    13,    15,    18,    23,
      28,    32,    36,    39,    41,    44,    47,    51,    53,    57,
      59,    63,    66,    69,    71,    74,    80,    82,    85,    87
};
static const short yyrhs[] =
{
      -1,    19,    20,     0,    19,    21,     0,    19,    22,     0,
      19,     1,     0,     3,     0,     4,    18,     0,    23,    31,
      13,    31,     0,    23,    31,    27,    29,     0,    23,    31,
      27,     0,    23,    31,    29,     0,    23,    31,     0,    25,
       0,    24,    25,     0,    26,    32,     0,    26,    32,    28,
       0,    11,     0,    10,    31,    11,     0,     7,     0,     6,
      32,     7,     0,    12,    31,     0,     8,    32,     0,    30,
       0,    29,    30,     0,    13,    15,    17,    16,    31,     0,
      18,     0,    31,    18,     0,     9,     0,    32,     9,     0
};

#endif

#if YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined. */
static const short yyrline[] =
{
       0,   170,   171,   172,   173,   174,   179,   187,   195,   216,
     237,   246,   255,   266,   275,   289,   298,   312,   318,   329,
     335,   347,   358,   369,   373,   388,   411,   418,   429,   436
};
#endif


#if (YYDEBUG) || defined YYERROR_VERBOSE

/* YYTNAME[TOKEN_NUM] -- String name of the token TOKEN_NUM. */
static const char *const yytname[] =
{
  "$", "error", "$undefined.", "COMMENT", "DOMAIN", "JUNK", "PREV_MSGCTXT", 
  "PREV_MSGID", "PREV_MSGID_PLURAL", "PREV_STRING", "MSGCTXT", "MSGID", 
  "MSGID_PLURAL", "MSGSTR", "NAME", "'['", "']'", "NUMBER", "STRING", 
  "po_file", "comment", "domain", "message", "message_intro", "prev", 
  "msg_intro", "prev_msg_intro", "msgid_pluralform", 
  "prev_msgid_pluralform", "pluralform_list", "pluralform", "string_list", 
  "prev_string_list", 0
};
#endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives. */
static const short yyr1[] =
{
       0,    19,    19,    19,    19,    19,    20,    21,    22,    22,
      22,    22,    22,    23,    23,    24,    24,    25,    25,    26,
      26,    27,    28,    29,    29,    30,    31,    31,    32,    32
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN. */
static const short yyr2[] =
{
       0,     0,     2,     2,     2,     2,     1,     2,     4,     4,
       3,     3,     2,     1,     2,     2,     3,     1,     3,     1,
       3,     2,     2,     1,     2,     5,     1,     2,     1,     2
};

/* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
   doesn't specify something else to do.  Zero means the default is an
   error. */
static const short yydefact[] =
{
       1,     0,     5,     6,     0,     0,    19,     0,    17,     2,
       3,     4,     0,     0,    13,     0,     7,    28,     0,    26,
       0,    12,    14,    15,    20,    29,    18,    27,     0,     0,
      10,    11,    23,     0,    16,    21,     0,     8,     0,     9,
      24,    22,     0,     0,    25,     0,     0
};

static const short yydefgoto[] =
{
       1,     9,    10,    11,    12,    13,    14,    15,    30,    34,
      31,    32,    20,    18
};

static const short yypact[] =
{
  -32768,     2,-32768,-32768,    -8,     5,-32768,     0,-32768,-32768,
  -32768,-32768,     0,    13,-32768,     5,-32768,-32768,    20,-32768,
      -7,     8,-32768,    24,-32768,-32768,-32768,-32768,     0,     7,
      15,    15,-32768,     5,-32768,    12,    17,    12,    21,    15,
  -32768,    26,    22,     0,    12,    37,-32768
};

static const short yypgoto[] =
{
  -32768,-32768,-32768,-32768,-32768,-32768,    27,-32768,-32768,-32768,
       9,   -24,   -12,   -14
};


#define	YYLAST		40


static const short yytable[] =
{
      21,    23,    45,     2,    26,     3,     4,    40,     5,     6,
      16,    27,     7,     8,    17,    40,    35,    37,    19,    41,
      28,    29,    36,     7,     8,    19,    27,    24,    38,    25,
      27,    44,    33,    25,    42,    25,    36,    46,    43,    39,
      22
};

static const short yycheck[] =
{
      12,    15,     0,     1,    11,     3,     4,    31,     6,     7,
      18,    18,    10,    11,     9,    39,    28,    29,    18,    33,
      12,    13,    15,    10,    11,    18,    18,     7,    13,     9,
      18,    43,     8,     9,    17,     9,    15,     0,    16,    30,
      13
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "bison.simple"

/* Skeleton output parser for bison,

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software
   Foundation, Inc.

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

/* This is the parser code that is written into each bison parser when
   the %semantic_parser declaration is not specified in the grammar.
   It was written by Richard Stallman by simplifying the hairy parser
   used when %semantic_parser is specified.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

#if ! defined (yyoverflow) || defined (YYERROR_VERBOSE)

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || defined (YYERROR_VERBOSE) */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYLTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
# if YYLSP_NEEDED
  YYLTYPE yyls;
# endif
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAX (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# if YYLSP_NEEDED
#  define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE) + sizeof (YYLTYPE))	\
      + 2 * YYSTACK_GAP_MAX)
# else
#  define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAX)
# endif

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAX;	\
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif


#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");			\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).

   When YYLLOC_DEFAULT is run, CURRENT is set the location of the
   first token.  By default, to implement support for ranges, extend
   its range to the last symbol.  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)       	\
   Current.last_line   = Rhs[N].last_line;	\
   Current.last_column = Rhs[N].last_column;
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#if YYPURE
# if YYLSP_NEEDED
#  ifdef YYLEX_PARAM
#   define YYLEX		yylex (&yylval, &yylloc, YYLEX_PARAM)
#  else
#   define YYLEX		yylex (&yylval, &yylloc)
#  endif
# else /* !YYLSP_NEEDED */
#  ifdef YYLEX_PARAM
#   define YYLEX		yylex (&yylval, YYLEX_PARAM)
#  else
#   define YYLEX		yylex (&yylval)
#  endif
# endif /* !YYLSP_NEEDED */
#else /* !YYPURE */
# define YYLEX			yylex ()
#endif /* !YYPURE */


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)
/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
#endif /* !YYDEBUG */

/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif

#ifdef YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif
#endif

#line 315 "bison.simple"


/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
#  define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL
# else
#  define YYPARSE_PARAM_ARG YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
# endif
#else /* !YYPARSE_PARAM */
# define YYPARSE_PARAM_ARG
# define YYPARSE_PARAM_DECL
#endif /* !YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
# ifdef YYPARSE_PARAM
int yyparse (void *);
# else
int yyparse (void);
# endif
#endif

/* YY_DECL_VARIABLES -- depending whether we use a pure parser,
   variables are global, or local to YYPARSE.  */

#define YY_DECL_NON_LSP_VARIABLES			\
/* The lookahead symbol.  */				\
int yychar;						\
							\
/* The semantic value of the lookahead symbol. */	\
YYSTYPE yylval;						\
							\
/* Number of parse errors so far.  */			\
int yynerrs;

#if YYLSP_NEEDED
# define YY_DECL_VARIABLES			\
YY_DECL_NON_LSP_VARIABLES			\
						\
/* Location data for the lookahead symbol.  */	\
YYLTYPE yylloc;
#else
# define YY_DECL_VARIABLES			\
YY_DECL_NON_LSP_VARIABLES
#endif


/* If nonreentrant, generate the variables here. */

#if !YYPURE
YY_DECL_VARIABLES
#endif  /* !YYPURE */

int
yyparse (YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  /* If reentrant, generate the variables here. */
#if YYPURE
  YY_DECL_VARIABLES
#endif  /* !YYPURE */

  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yychar1 = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack. */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;

#if YYLSP_NEEDED
  /* The location stack.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
#endif

#if YYLSP_NEEDED
# define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
# define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  YYSIZE_T yystacksize = YYINITDEPTH;


  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
#if YYLSP_NEEDED
  YYLTYPE yyloc;
#endif

  /* When reducing, the number of symbols on the RHS of the reduced
     rule. */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;
#if YYLSP_NEEDED
  yylsp = yyls;
#endif
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  */
# if YYLSP_NEEDED
	YYLTYPE *yyls1 = yyls;
	/* This used to be a conditional around just the two extra args,
	   but that might be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);
	yyls = yyls1;
# else
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);
# endif
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);
# if YYLSP_NEEDED
	YYSTACK_RELOCATE (yyls);
# endif
# undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
#if YYLSP_NEEDED
      yylsp = yyls + yysize - 1;
#endif

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
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
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yychar1 = YYTRANSLATE (yychar);

#if YYDEBUG
     /* We have to keep this `#if YYDEBUG', since we use variables
	which are defined only if `YYDEBUG' is set.  */
      if (yydebug)
	{
	  YYFPRINTF (stderr, "Next token is %d (%s",
		     yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise
	     meaning of a token, for further debugging info.  */
# ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
# endif
	  YYFPRINTF (stderr, ")\n");
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
  YYDPRINTF ((stderr, "Shifting token %d (%s), ",
	      yychar, yytname[yychar1]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#if YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to the semantic value of
     the lookahead token.  This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

#if YYLSP_NEEDED
  /* Similarly for the default location.  Let the user run additional
     commands if for instance locations are ranges.  */
  yyloc = yylsp[1-yylen];
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
#endif

#if YYDEBUG
  /* We have to keep this `#if YYDEBUG', since we use variables which
     are defined only if `YYDEBUG' is set.  */
  if (yydebug)
    {
      int yyi;

      YYFPRINTF (stderr, "Reducing via rule %d (line %d), ",
		 yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (yyi = yyprhs[yyn]; yyrhs[yyi] > 0; yyi++)
	YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
      YYFPRINTF (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif

  switch (yyn) {

case 6:
#line 180 "po-gram-gen.y"
{
		  po_callback_comment_dispatcher (yyvsp[0].string.string);
		}
    break;
case 7:
#line 188 "po-gram-gen.y"
{
		   po_callback_domain (yyvsp[0].string.string);
		}
    break;
case 8:
#line 196 "po-gram-gen.y"
{
		  char *string2 = string_list_concat_destroy (&yyvsp[-2].stringlist.stringlist);
		  char *string4 = string_list_concat_destroy (&yyvsp[0].stringlist.stringlist);

		  check_obsolete (yyvsp[-3].message_intro, yyvsp[-2].stringlist);
		  check_obsolete (yyvsp[-3].message_intro, yyvsp[-1].pos);
		  check_obsolete (yyvsp[-3].message_intro, yyvsp[0].stringlist);
		  if (!yyvsp[-3].message_intro.obsolete || pass_obsolete_entries)
		    do_callback_message (yyvsp[-3].message_intro.ctxt, string2, &yyvsp[-3].message_intro.pos, NULL,
					 string4, strlen (string4) + 1, &yyvsp[-1].pos.pos,
					 yyvsp[-3].message_intro.prev_ctxt,
					 yyvsp[-3].message_intro.prev_id, yyvsp[-3].message_intro.prev_id_plural,
					 yyvsp[-3].message_intro.obsolete);
		  else
		    {
		      free_message_intro (yyvsp[-3].message_intro);
		      free (string2);
		      free (string4);
		    }
		}
    break;
case 9:
#line 217 "po-gram-gen.y"
{
		  char *string2 = string_list_concat_destroy (&yyvsp[-2].stringlist.stringlist);

		  check_obsolete (yyvsp[-3].message_intro, yyvsp[-2].stringlist);
		  check_obsolete (yyvsp[-3].message_intro, yyvsp[-1].string);
		  check_obsolete (yyvsp[-3].message_intro, yyvsp[0].rhs);
		  if (!yyvsp[-3].message_intro.obsolete || pass_obsolete_entries)
		    do_callback_message (yyvsp[-3].message_intro.ctxt, string2, &yyvsp[-3].message_intro.pos, yyvsp[-1].string.string,
					 yyvsp[0].rhs.rhs.msgstr, yyvsp[0].rhs.rhs.msgstr_len, &yyvsp[0].rhs.pos,
					 yyvsp[-3].message_intro.prev_ctxt,
					 yyvsp[-3].message_intro.prev_id, yyvsp[-3].message_intro.prev_id_plural,
					 yyvsp[-3].message_intro.obsolete);
		  else
		    {
		      free_message_intro (yyvsp[-3].message_intro);
		      free (string2);
		      free (yyvsp[-1].string.string);
		      free (yyvsp[0].rhs.rhs.msgstr);
		    }
		}
    break;
case 10:
#line 238 "po-gram-gen.y"
{
		  check_obsolete (yyvsp[-2].message_intro, yyvsp[-1].stringlist);
		  check_obsolete (yyvsp[-2].message_intro, yyvsp[0].string);
		  po_gram_error_at_line (&yyvsp[-2].message_intro.pos, _("missing `msgstr[]' section"));
		  free_message_intro (yyvsp[-2].message_intro);
		  string_list_destroy (&yyvsp[-1].stringlist.stringlist);
		  free (yyvsp[0].string.string);
		}
    break;
case 11:
#line 247 "po-gram-gen.y"
{
		  check_obsolete (yyvsp[-2].message_intro, yyvsp[-1].stringlist);
		  check_obsolete (yyvsp[-2].message_intro, yyvsp[0].rhs);
		  po_gram_error_at_line (&yyvsp[-2].message_intro.pos, _("missing `msgid_plural' section"));
		  free_message_intro (yyvsp[-2].message_intro);
		  string_list_destroy (&yyvsp[-1].stringlist.stringlist);
		  free (yyvsp[0].rhs.rhs.msgstr);
		}
    break;
case 12:
#line 256 "po-gram-gen.y"
{
		  check_obsolete (yyvsp[-1].message_intro, yyvsp[0].stringlist);
		  po_gram_error_at_line (&yyvsp[-1].message_intro.pos, _("missing `msgstr' section"));
		  free_message_intro (yyvsp[-1].message_intro);
		  string_list_destroy (&yyvsp[0].stringlist.stringlist);
		}
    break;
case 13:
#line 267 "po-gram-gen.y"
{
		  yyval.message_intro.prev_ctxt = NULL;
		  yyval.message_intro.prev_id = NULL;
		  yyval.message_intro.prev_id_plural = NULL;
		  yyval.message_intro.ctxt = yyvsp[0].string.string;
		  yyval.message_intro.pos = yyvsp[0].string.pos;
		  yyval.message_intro.obsolete = yyvsp[0].string.obsolete;
		}
    break;
case 14:
#line 276 "po-gram-gen.y"
{
		  check_obsolete (yyvsp[-1].prev, yyvsp[0].string);
		  yyval.message_intro.prev_ctxt = yyvsp[-1].prev.ctxt;
		  yyval.message_intro.prev_id = yyvsp[-1].prev.id;
		  yyval.message_intro.prev_id_plural = yyvsp[-1].prev.id_plural;
		  yyval.message_intro.ctxt = yyvsp[0].string.string;
		  yyval.message_intro.pos = yyvsp[0].string.pos;
		  yyval.message_intro.obsolete = yyvsp[0].string.obsolete;
		}
    break;
case 15:
#line 290 "po-gram-gen.y"
{
		  check_obsolete (yyvsp[-1].string, yyvsp[0].stringlist);
		  yyval.prev.ctxt = yyvsp[-1].string.string;
		  yyval.prev.id = string_list_concat_destroy (&yyvsp[0].stringlist.stringlist);
		  yyval.prev.id_plural = NULL;
		  yyval.prev.pos = yyvsp[-1].string.pos;
		  yyval.prev.obsolete = yyvsp[-1].string.obsolete;
		}
    break;
case 16:
#line 299 "po-gram-gen.y"
{
		  check_obsolete (yyvsp[-2].string, yyvsp[-1].stringlist);
		  check_obsolete (yyvsp[-2].string, yyvsp[0].string);
		  yyval.prev.ctxt = yyvsp[-2].string.string;
		  yyval.prev.id = string_list_concat_destroy (&yyvsp[-1].stringlist.stringlist);
		  yyval.prev.id_plural = yyvsp[0].string.string;
		  yyval.prev.pos = yyvsp[-2].string.pos;
		  yyval.prev.obsolete = yyvsp[-2].string.obsolete;
		}
    break;
case 17:
#line 313 "po-gram-gen.y"
{
		  yyval.string.string = NULL;
		  yyval.string.pos = yyvsp[0].pos.pos;
		  yyval.string.obsolete = yyvsp[0].pos.obsolete;
		}
    break;
case 18:
#line 319 "po-gram-gen.y"
{
		  check_obsolete (yyvsp[-2].pos, yyvsp[-1].stringlist);
		  check_obsolete (yyvsp[-2].pos, yyvsp[0].pos);
		  yyval.string.string = string_list_concat_destroy (&yyvsp[-1].stringlist.stringlist);
		  yyval.string.pos = yyvsp[0].pos.pos;
		  yyval.string.obsolete = yyvsp[0].pos.obsolete;
		}
    break;
case 19:
#line 330 "po-gram-gen.y"
{
		  yyval.string.string = NULL;
		  yyval.string.pos = yyvsp[0].pos.pos;
		  yyval.string.obsolete = yyvsp[0].pos.obsolete;
		}
    break;
case 20:
#line 336 "po-gram-gen.y"
{
		  check_obsolete (yyvsp[-2].pos, yyvsp[-1].stringlist);
		  check_obsolete (yyvsp[-2].pos, yyvsp[0].pos);
		  yyval.string.string = string_list_concat_destroy (&yyvsp[-1].stringlist.stringlist);
		  yyval.string.pos = yyvsp[0].pos.pos;
		  yyval.string.obsolete = yyvsp[0].pos.obsolete;
		}
    break;
case 21:
#line 348 "po-gram-gen.y"
{
		  check_obsolete (yyvsp[-1].pos, yyvsp[0].stringlist);
		  plural_counter = 0;
		  yyval.string.string = string_list_concat_destroy (&yyvsp[0].stringlist.stringlist);
		  yyval.string.pos = yyvsp[-1].pos.pos;
		  yyval.string.obsolete = yyvsp[-1].pos.obsolete;
		}
    break;
case 22:
#line 359 "po-gram-gen.y"
{
		  check_obsolete (yyvsp[-1].pos, yyvsp[0].stringlist);
		  yyval.string.string = string_list_concat_destroy (&yyvsp[0].stringlist.stringlist);
		  yyval.string.pos = yyvsp[-1].pos.pos;
		  yyval.string.obsolete = yyvsp[-1].pos.obsolete;
		}
    break;
case 23:
#line 370 "po-gram-gen.y"
{
		  yyval.rhs = yyvsp[0].rhs;
		}
    break;
case 24:
#line 374 "po-gram-gen.y"
{
		  check_obsolete (yyvsp[-1].rhs, yyvsp[0].rhs);
		  yyval.rhs.rhs.msgstr = (char *) xmalloc (yyvsp[-1].rhs.rhs.msgstr_len + yyvsp[0].rhs.rhs.msgstr_len);
		  memcpy (yyval.rhs.rhs.msgstr, yyvsp[-1].rhs.rhs.msgstr, yyvsp[-1].rhs.rhs.msgstr_len);
		  memcpy (yyval.rhs.rhs.msgstr + yyvsp[-1].rhs.rhs.msgstr_len, yyvsp[0].rhs.rhs.msgstr, yyvsp[0].rhs.rhs.msgstr_len);
		  yyval.rhs.rhs.msgstr_len = yyvsp[-1].rhs.rhs.msgstr_len + yyvsp[0].rhs.rhs.msgstr_len;
		  free (yyvsp[-1].rhs.rhs.msgstr);
		  free (yyvsp[0].rhs.rhs.msgstr);
		  yyval.rhs.pos = yyvsp[-1].rhs.pos;
		  yyval.rhs.obsolete = yyvsp[-1].rhs.obsolete;
		}
    break;
case 25:
#line 389 "po-gram-gen.y"
{
		  check_obsolete (yyvsp[-4].pos, yyvsp[-3].pos);
		  check_obsolete (yyvsp[-4].pos, yyvsp[-2].number);
		  check_obsolete (yyvsp[-4].pos, yyvsp[-1].pos);
		  check_obsolete (yyvsp[-4].pos, yyvsp[0].stringlist);
		  if (yyvsp[-2].number.number != plural_counter)
		    {
		      if (plural_counter == 0)
			po_gram_error_at_line (&yyvsp[-4].pos.pos, _("first plural form has nonzero index"));
		      else
			po_gram_error_at_line (&yyvsp[-4].pos.pos, _("plural form has wrong index"));
		    }
		  plural_counter++;
		  yyval.rhs.rhs.msgstr = string_list_concat_destroy (&yyvsp[0].stringlist.stringlist);
		  yyval.rhs.rhs.msgstr_len = strlen (yyval.rhs.rhs.msgstr) + 1;
		  yyval.rhs.pos = yyvsp[-4].pos.pos;
		  yyval.rhs.obsolete = yyvsp[-4].pos.obsolete;
		}
    break;
case 26:
#line 412 "po-gram-gen.y"
{
		  string_list_init (&yyval.stringlist.stringlist);
		  string_list_append (&yyval.stringlist.stringlist, yyvsp[0].string.string);
		  yyval.stringlist.pos = yyvsp[0].string.pos;
		  yyval.stringlist.obsolete = yyvsp[0].string.obsolete;
		}
    break;
case 27:
#line 419 "po-gram-gen.y"
{
		  check_obsolete (yyvsp[-1].stringlist, yyvsp[0].string);
		  yyval.stringlist.stringlist = yyvsp[-1].stringlist.stringlist;
		  string_list_append (&yyval.stringlist.stringlist, yyvsp[0].string.string);
		  yyval.stringlist.pos = yyvsp[-1].stringlist.pos;
		  yyval.stringlist.obsolete = yyvsp[-1].stringlist.obsolete;
		}
    break;
case 28:
#line 430 "po-gram-gen.y"
{
		  string_list_init (&yyval.stringlist.stringlist);
		  string_list_append (&yyval.stringlist.stringlist, yyvsp[0].string.string);
		  yyval.stringlist.pos = yyvsp[0].string.pos;
		  yyval.stringlist.obsolete = yyvsp[0].string.obsolete;
		}
    break;
case 29:
#line 437 "po-gram-gen.y"
{
		  check_obsolete (yyvsp[-1].stringlist, yyvsp[0].string);
		  yyval.stringlist.stringlist = yyvsp[-1].stringlist.stringlist;
		  string_list_append (&yyval.stringlist.stringlist, yyvsp[0].string.string);
		  yyval.stringlist.pos = yyvsp[-1].stringlist.pos;
		  yyval.stringlist.obsolete = yyvsp[-1].stringlist.obsolete;
		}
    break;
}

#line 705 "bison.simple"


  yyvsp -= yylen;
  yyssp -= yylen;
#if YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG
  if (yydebug)
    {
      short *yyssp1 = yyss - 1;
      YYFPRINTF (stderr, "state stack now");
      while (yyssp1 != yyssp)
	YYFPRINTF (stderr, " %d", *++yyssp1);
      YYFPRINTF (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;
#if YYLSP_NEEDED
  *++yylsp = yyloc;
#endif

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("parse error, unexpected ") + 1;
	  yysize += yystrlen (yytname[YYTRANSLATE (yychar)]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "parse error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[YYTRANSLATE (yychar)]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exhausted");
	}
      else
#endif /* defined (YYERROR_VERBOSE) */
	yyerror ("parse error");
    }
  goto yyerrlab1;


/*--------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action |
`--------------------------------------------------*/
yyerrlab1:
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;
      YYDPRINTF ((stderr, "Discarding token %d (%s).\n",
		  yychar, yytname[yychar1]));
      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;


/*-------------------------------------------------------------------.
| yyerrdefault -- current state does not do anything special for the |
| error token.                                                       |
`-------------------------------------------------------------------*/
yyerrdefault:
#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */

  /* If its default is to accept any token, ok.  Otherwise pop it.  */
  yyn = yydefact[yystate];
  if (yyn)
    goto yydefault;
#endif


/*---------------------------------------------------------------.
| yyerrpop -- pop the current state because it cannot handle the |
| error token                                                    |
`---------------------------------------------------------------*/
yyerrpop:
  if (yyssp == yyss)
    YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#if YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG
  if (yydebug)
    {
      short *yyssp1 = yyss - 1;
      YYFPRINTF (stderr, "Error: state stack now");
      while (yyssp1 != yyssp)
	YYFPRINTF (stderr, " %d", *++yyssp1);
      YYFPRINTF (stderr, "\n");
    }
#endif

/*--------------.
| yyerrhandle.  |
`--------------*/
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

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;
#if YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

/*---------------------------------------------.
| yyoverflowab -- parser overflow comes here.  |
`---------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}
#line 445 "po-gram-gen.y"
