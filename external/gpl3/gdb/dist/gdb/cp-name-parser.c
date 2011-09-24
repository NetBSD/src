/* A Bison parser, made by GNU Bison 1.875c.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

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

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     INT = 258,
     FLOAT = 259,
     NAME = 260,
     STRUCT = 261,
     CLASS = 262,
     UNION = 263,
     ENUM = 264,
     SIZEOF = 265,
     UNSIGNED = 266,
     COLONCOLON = 267,
     TEMPLATE = 268,
     ERROR = 269,
     NEW = 270,
     DELETE = 271,
     OPERATOR = 272,
     STATIC_CAST = 273,
     REINTERPRET_CAST = 274,
     DYNAMIC_CAST = 275,
     SIGNED_KEYWORD = 276,
     LONG = 277,
     SHORT = 278,
     INT_KEYWORD = 279,
     CONST_KEYWORD = 280,
     VOLATILE_KEYWORD = 281,
     DOUBLE_KEYWORD = 282,
     BOOL = 283,
     ELLIPSIS = 284,
     RESTRICT = 285,
     VOID = 286,
     FLOAT_KEYWORD = 287,
     CHAR = 288,
     WCHAR_T = 289,
     ASSIGN_MODIFY = 290,
     TRUEKEYWORD = 291,
     FALSEKEYWORD = 292,
     DEMANGLER_SPECIAL = 293,
     CONSTRUCTION_VTABLE = 294,
     CONSTRUCTION_IN = 295,
     OROR = 296,
     ANDAND = 297,
     NOTEQUAL = 298,
     EQUAL = 299,
     GEQ = 300,
     LEQ = 301,
     RSH = 302,
     LSH = 303,
     DECREMENT = 304,
     INCREMENT = 305,
     UNARY = 306,
     ARROW = 307
   };
#endif
#define INT 258
#define FLOAT 259
#define NAME 260
#define STRUCT 261
#define CLASS 262
#define UNION 263
#define ENUM 264
#define SIZEOF 265
#define UNSIGNED 266
#define COLONCOLON 267
#define TEMPLATE 268
#define ERROR 269
#define NEW 270
#define DELETE 271
#define OPERATOR 272
#define STATIC_CAST 273
#define REINTERPRET_CAST 274
#define DYNAMIC_CAST 275
#define SIGNED_KEYWORD 276
#define LONG 277
#define SHORT 278
#define INT_KEYWORD 279
#define CONST_KEYWORD 280
#define VOLATILE_KEYWORD 281
#define DOUBLE_KEYWORD 282
#define BOOL 283
#define ELLIPSIS 284
#define RESTRICT 285
#define VOID 286
#define FLOAT_KEYWORD 287
#define CHAR 288
#define WCHAR_T 289
#define ASSIGN_MODIFY 290
#define TRUEKEYWORD 291
#define FALSEKEYWORD 292
#define DEMANGLER_SPECIAL 293
#define CONSTRUCTION_VTABLE 294
#define CONSTRUCTION_IN 295
#define OROR 296
#define ANDAND 297
#define NOTEQUAL 298
#define EQUAL 299
#define GEQ 300
#define LEQ 301
#define RSH 302
#define LSH 303
#define DECREMENT 304
#define INCREMENT 305
#define UNARY 306
#define ARROW 307




/* Copy the first part of user declarations.  */
#line 31 "cp-name-parser.y"


#include "defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "safe-ctype.h"
#include "libiberty.h"
#include "demangle.h"
#include "cp-support.h"

/* Bison does not make it easy to create a parser without global
   state, unfortunately.  Here are all the global variables used
   in this parser.  */

/* LEXPTR is the current pointer into our lex buffer.  PREV_LEXPTR
   is the start of the last token lexed, only used for diagnostics.
   ERROR_LEXPTR is the first place an error occurred.  GLOBAL_ERRMSG
   is the first error message encountered.  */

static const char *lexptr, *prev_lexptr, *error_lexptr, *global_errmsg;

/* The components built by the parser are allocated ahead of time,
   and cached in this structure.  */

#define ALLOC_CHUNK 100

struct demangle_info {
  int used;
  struct demangle_info *prev, *next;
  struct demangle_component comps[ALLOC_CHUNK];
};

static struct demangle_info *demangle_info;

static struct demangle_component *
d_grab (void)
{
  struct demangle_info *more;

  if (demangle_info->used >= ALLOC_CHUNK)
    {
      if (demangle_info->next == NULL)
	{
	  more = xmalloc (sizeof (struct demangle_info));
	  more->prev = demangle_info;
	  more->next = NULL;
	  demangle_info->next = more;
	}
      else
	more = demangle_info->next;

      more->used = 0;
      demangle_info = more;
    }
  return &demangle_info->comps[demangle_info->used++];
}

/* The parse tree created by the parser is stored here after a successful
   parse.  */

static struct demangle_component *global_result;

/* Prototypes for helper functions used when constructing the parse
   tree.  */

static struct demangle_component *d_qualify (struct demangle_component *, int,
					     int);

static struct demangle_component *d_int_type (int);

static struct demangle_component *d_unary (const char *,
					   struct demangle_component *);
static struct demangle_component *d_binary (const char *,
					    struct demangle_component *,
					    struct demangle_component *);

/* Flags passed to d_qualify.  */

#define QUAL_CONST 1
#define QUAL_RESTRICT 2
#define QUAL_VOLATILE 4

/* Flags passed to d_int_type.  */

#define INT_CHAR	(1 << 0)
#define INT_SHORT	(1 << 1)
#define INT_LONG	(1 << 2)
#define INT_LLONG	(1 << 3)

#define INT_SIGNED	(1 << 4)
#define INT_UNSIGNED	(1 << 5)

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror, etc),
   as well as gratuitiously global symbol names, so we can have multiple
   yacc generated parsers in gdb.  Note that these are only the variables
   produced by yacc.  If other parser generators (bison, byacc, etc) produce
   additional global names that conflict at link time, then those parser
   generators need to be fixed instead of adding those names to this list. */

#define	yymaxdepth cpname_maxdepth
#define	yyparse	cpname_parse
#define	yylex	cpname_lex
#define	yyerror	cpname_error
#define	yylval	cpname_lval
#define	yychar	cpname_char
#define	yydebug	cpname_debug
#define	yypact	cpname_pact	
#define	yyr1	cpname_r1			
#define	yyr2	cpname_r2			
#define	yydef	cpname_def		
#define	yychk	cpname_chk		
#define	yypgo	cpname_pgo		
#define	yyact	cpname_act		
#define	yyexca	cpname_exca
#define yyerrflag cpname_errflag
#define yynerrs	cpname_nerrs
#define	yyps	cpname_ps
#define	yypv	cpname_pv
#define	yys	cpname_s
#define	yy_yys	cpname_yys
#define	yystate	cpname_state
#define	yytmp	cpname_tmp
#define	yyv	cpname_v
#define	yy_yyv	cpname_yyv
#define	yyval	cpname_val
#define	yylloc	cpname_lloc
#define yyreds	cpname_reds		/* With YYDEBUG defined */
#define yytoks	cpname_toks		/* With YYDEBUG defined */
#define yyname	cpname_name		/* With YYDEBUG defined */
#define yyrule	cpname_rule		/* With YYDEBUG defined */
#define yylhs	cpname_yylhs
#define yylen	cpname_yylen
#define yydefred cpname_yydefred
#define yydgoto	cpname_yydgoto
#define yysindex cpname_yysindex
#define yyrindex cpname_yyrindex
#define yygindex cpname_yygindex
#define yytable	 cpname_yytable
#define yycheck	 cpname_yycheck

int yyparse (void);
static int yylex (void);
static void yyerror (char *);

/* Enable yydebug for the stand-alone parser.  */
#ifdef TEST_CPNAMES
# define YYDEBUG	1
#endif

/* Helper functions.  These wrap the demangler tree interface, handle
   allocation from our global store, and return the allocated component.  */

static struct demangle_component *
fill_comp (enum demangle_component_type d_type, struct demangle_component *lhs,
	   struct demangle_component *rhs)
{
  struct demangle_component *ret = d_grab ();
  cplus_demangle_fill_component (ret, d_type, lhs, rhs);
  return ret;
}

static struct demangle_component *
make_empty (enum demangle_component_type d_type)
{
  struct demangle_component *ret = d_grab ();
  ret->type = d_type;
  return ret;
}

static struct demangle_component *
make_operator (const char *name, int args)
{
  struct demangle_component *ret = d_grab ();
  cplus_demangle_fill_operator (ret, name, args);
  return ret;
}

static struct demangle_component *
make_dtor (enum gnu_v3_dtor_kinds kind, struct demangle_component *name)
{
  struct demangle_component *ret = d_grab ();
  cplus_demangle_fill_dtor (ret, kind, name);
  return ret;
}

static struct demangle_component *
make_builtin_type (const char *name)
{
  struct demangle_component *ret = d_grab ();
  cplus_demangle_fill_builtin_type (ret, name);
  return ret;
}

static struct demangle_component *
make_name (const char *name, int len)
{
  struct demangle_component *ret = d_grab ();
  cplus_demangle_fill_name (ret, name, len);
  return ret;
}

#define d_left(dc) (dc)->u.s_binary.left
#define d_right(dc) (dc)->u.s_binary.right



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 242 "cp-name-parser.y"
typedef union YYSTYPE {
    struct demangle_component *comp;
    struct nested {
      struct demangle_component *comp;
      struct demangle_component **last;
    } nested;
    struct {
      struct demangle_component *comp, *last;
    } nested1;
    struct {
      struct demangle_component *comp, **last;
      struct nested fn;
      struct demangle_component *start;
      int fold_flag;
    } abstract;
    int lval;
    const char *opname;
  } YYSTYPE;
/* Line 191 of yacc.c.  */
#line 409 "cp-name-parser.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 421 "cp-name-parser.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

# ifndef YYFREE
#  define YYFREE xfree
# endif
# ifndef YYMALLOC
#  define YYMALLOC xmalloc
# endif

/* The parser invokes alloca or xmalloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   define YYSTACK_ALLOC alloca
#  endif
# else
#  if defined (alloca) || defined (_ALLOCA_H)
#   define YYSTACK_ALLOC alloca
#  else
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
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
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
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
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  84
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1097

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  75
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  40
/* YYNRULES -- Number of rules. */
#define YYNRULES  194
/* YYNRULES -- Number of states. */
#define YYNSTATES  324

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   307

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    72,     2,     2,     2,    63,    49,     2,
      73,    41,    61,    59,    42,    60,    67,    62,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    74,     2,
      52,    43,    53,    44,    58,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    68,     2,    70,    48,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    47,     2,    71,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    45,    46,    50,    51,
      54,    55,    56,    57,    64,    65,    66,    69
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    11,    12,    15,    18,
      22,    26,    29,    32,    35,    40,    43,    46,    51,    56,
      59,    62,    65,    68,    71,    74,    77,    80,    83,    86,
      89,    92,    95,    98,   101,   104,   107,   110,   113,   116,
     119,   122,   125,   128,   131,   135,   138,   142,   146,   149,
     152,   154,   158,   161,   163,   168,   171,   173,   176,   179,
     181,   184,   186,   188,   190,   192,   195,   198,   200,   203,
     207,   210,   214,   219,   221,   225,   227,   230,   233,   238,
     240,   242,   245,   249,   254,   258,   263,   268,   272,   273,
     275,   277,   279,   281,   283,   286,   288,   290,   292,   294,
     296,   298,   300,   303,   305,   307,   309,   312,   314,   316,
     318,   321,   323,   327,   332,   335,   339,   342,   344,   348,
     351,   354,   356,   360,   363,   367,   370,   375,   379,   381,
     384,   386,   390,   393,   396,   398,   400,   403,   405,   410,
     413,   415,   418,   421,   423,   427,   430,   433,   435,   438,
     440,   442,   447,   452,   457,   460,   463,   466,   469,   473,
     475,   479,   482,   487,   490,   493,   496,   501,   509,   517,
     525,   529,   533,   537,   541,   545,   549,   553,   557,   561,
     565,   569,   573,   577,   581,   585,   589,   593,   597,   601,
     607,   609,   611,   616,   618
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      76,     0,    -1,    77,    -1,   108,    -1,    80,    -1,    79,
      -1,    -1,    12,    77,    -1,   104,   111,    -1,   104,    95,
      78,    -1,    88,    95,    78,    -1,    83,    78,    -1,    83,
     107,    -1,    38,    77,    -1,    39,    77,    40,    77,    -1,
      17,    15,    -1,    17,    16,    -1,    17,    15,    68,    70,
      -1,    17,    16,    68,    70,    -1,    17,    59,    -1,    17,
      60,    -1,    17,    61,    -1,    17,    62,    -1,    17,    63,
      -1,    17,    48,    -1,    17,    49,    -1,    17,    47,    -1,
      17,    71,    -1,    17,    72,    -1,    17,    43,    -1,    17,
      52,    -1,    17,    53,    -1,    17,    35,    -1,    17,    57,
      -1,    17,    56,    -1,    17,    51,    -1,    17,    50,    -1,
      17,    55,    -1,    17,    54,    -1,    17,    46,    -1,    17,
      45,    -1,    17,    65,    -1,    17,    64,    -1,    17,    42,
      -1,    17,    69,    61,    -1,    17,    69,    -1,    17,    73,
      41,    -1,    17,    68,    70,    -1,    17,   104,    -1,    90,
      82,    -1,    82,    -1,    12,    90,    82,    -1,    12,    82,
      -1,    81,    -1,    81,    52,    92,    53,    -1,    71,     5,
      -1,    86,    -1,    12,    86,    -1,    90,     5,    -1,     5,
      -1,    90,    91,    -1,    91,    -1,    85,    -1,    88,    -1,
      89,    -1,    12,    89,    -1,    90,    84,    -1,    84,    -1,
       5,    12,    -1,    90,     5,    12,    -1,    91,    12,    -1,
      90,    91,    12,    -1,     5,    52,    92,    53,    -1,    93,
      -1,    92,    42,    93,    -1,   104,    -1,   104,   105,    -1,
      49,    77,    -1,    49,    73,    77,    41,    -1,   113,    -1,
     104,    -1,   104,   105,    -1,    94,    42,   104,    -1,    94,
      42,   104,   105,    -1,    94,    42,    29,    -1,    73,    94,
      41,    96,    -1,    73,    31,    41,    96,    -1,    73,    41,
      96,    -1,    -1,    98,    -1,    30,    -1,    26,    -1,    25,
      -1,    97,    -1,    97,    98,    -1,    24,    -1,    21,    -1,
      11,    -1,    33,    -1,    22,    -1,    23,    -1,    99,    -1,
     100,    99,    -1,   100,    -1,    32,    -1,    27,    -1,    22,
      27,    -1,    28,    -1,    34,    -1,    31,    -1,    61,    96,
      -1,    49,    -1,    90,    61,    96,    -1,    12,    90,    61,
      96,    -1,    68,    70,    -1,    68,     3,    70,    -1,   101,
      98,    -1,   101,    -1,    98,   101,    98,    -1,    98,   101,
      -1,    86,    98,    -1,    86,    -1,    98,    86,    98,    -1,
      98,    86,    -1,    12,    86,    98,    -1,    12,    86,    -1,
      98,    12,    86,    98,    -1,    98,    12,    86,    -1,   102,
      -1,   102,   105,    -1,   106,    -1,    73,   105,    41,    -1,
     106,    95,    -1,   106,   103,    -1,   103,    -1,   102,    -1,
     102,   107,    -1,   106,    -1,   106,    95,    12,    77,    -1,
      95,    78,    -1,   104,    -1,   104,   105,    -1,   102,   109,
      -1,   110,    -1,    73,   109,    41,    -1,   110,    95,    -1,
     110,   103,    -1,    87,    -1,   102,   111,    -1,    87,    -1,
     112,    -1,    87,    95,    12,    77,    -1,   112,    95,    12,
      77,    -1,    73,   102,   109,    41,    -1,   112,    95,    -1,
     112,   103,    -1,    87,    95,    -1,    87,   103,    -1,    73,
     114,    41,    -1,   113,    -1,   113,    53,   113,    -1,    49,
      77,    -1,    49,    73,    77,    41,    -1,    60,   113,    -1,
      72,   113,    -1,    71,   113,    -1,    73,   108,    41,   113,
      -1,    18,    52,   108,    53,    73,   114,    41,    -1,    20,
      52,   108,    53,    73,   114,    41,    -1,    19,    52,   108,
      53,    73,   114,    41,    -1,   113,    61,   113,    -1,   113,
      62,   113,    -1,   113,    63,   113,    -1,   113,    59,   113,
      -1,   113,    60,   113,    -1,   113,    57,   113,    -1,   113,
      56,   113,    -1,   113,    51,   113,    -1,   113,    50,   113,
      -1,   113,    55,   113,    -1,   113,    54,   113,    -1,   113,
      52,   113,    -1,   113,    49,   113,    -1,   113,    48,   113,
      -1,   113,    47,   113,    -1,   113,    46,   113,    -1,   113,
      45,   113,    -1,   113,    69,     5,    -1,   113,    67,     5,
      -1,   113,    44,   113,    74,   113,    -1,     3,    -1,     4,
      -1,    10,    73,   108,    41,    -1,    36,    -1,    37,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   356,   356,   360,   362,   364,   369,   370,   377,   386,
     389,   393,   396,   415,   419,   423,   425,   427,   429,   431,
     433,   435,   437,   439,   441,   443,   445,   447,   449,   451,
     453,   455,   457,   459,   461,   463,   465,   467,   469,   471,
     473,   475,   477,   479,   481,   483,   485,   487,   495,   500,
     505,   509,   514,   522,   523,   525,   537,   538,   544,   546,
     547,   549,   552,   553,   556,   557,   561,   563,   566,   572,
     579,   585,   596,   600,   603,   614,   615,   619,   621,   623,
     626,   630,   635,   640,   646,   656,   660,   664,   672,   673,
     676,   678,   680,   684,   685,   692,   694,   696,   698,   700,
     702,   706,   707,   711,   713,   715,   717,   719,   721,   723,
     727,   733,   737,   745,   755,   759,   775,   777,   778,   780,
     783,   785,   786,   788,   791,   793,   795,   797,   802,   805,
     810,   817,   821,   832,   838,   856,   859,   867,   869,   880,
     887,   888,   894,   898,   902,   904,   909,   914,   927,   931,
     936,   944,   949,   958,   962,   967,   972,   976,   982,   988,
     991,   998,  1000,  1005,  1009,  1013,  1020,  1036,  1043,  1050,
    1069,  1073,  1077,  1081,  1085,  1089,  1093,  1097,  1101,  1105,
    1109,  1113,  1117,  1121,  1125,  1129,  1133,  1138,  1142,  1146,
    1153,  1157,  1160,  1165,  1174
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INT", "FLOAT", "NAME", "STRUCT",
  "CLASS", "UNION", "ENUM", "SIZEOF", "UNSIGNED", "COLONCOLON", "TEMPLATE",
  "ERROR", "NEW", "DELETE", "OPERATOR", "STATIC_CAST", "REINTERPRET_CAST",
  "DYNAMIC_CAST", "SIGNED_KEYWORD", "LONG", "SHORT", "INT_KEYWORD",
  "CONST_KEYWORD", "VOLATILE_KEYWORD", "DOUBLE_KEYWORD", "BOOL",
  "ELLIPSIS", "RESTRICT", "VOID", "FLOAT_KEYWORD", "CHAR", "WCHAR_T",
  "ASSIGN_MODIFY", "TRUEKEYWORD", "FALSEKEYWORD", "DEMANGLER_SPECIAL",
  "CONSTRUCTION_VTABLE", "CONSTRUCTION_IN", "')'", "','", "'='", "'?'",
  "OROR", "ANDAND", "'|'", "'^'", "'&'", "NOTEQUAL", "EQUAL", "'<'", "'>'",
  "GEQ", "LEQ", "RSH", "LSH", "'@'", "'+'", "'-'", "'*'", "'/'", "'%'",
  "DECREMENT", "INCREMENT", "UNARY", "'.'", "'['", "ARROW", "']'", "'~'",
  "'!'", "'('", "':'", "$accept", "result", "start", "start_opt",
  "function", "demangler_special", "operator", "conversion_op",
  "conversion_op_name", "unqualified_name", "colon_name", "name",
  "colon_ext_name", "colon_ext_only", "ext_only_name", "nested_name",
  "template", "template_params", "template_arg", "function_args",
  "function_arglist", "qualifiers_opt", "qualifier", "qualifiers",
  "int_part", "int_seq", "builtin_type", "ptr_operator", "array_indicator",
  "typespec_2", "abstract_declarator", "direct_abstract_declarator",
  "abstract_declarator_fn", "type", "declarator", "direct_declarator",
  "declarator_1", "direct_declarator_1", "exp", "exp1", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,    41,    44,    61,    63,   296,   297,   124,    94,    38,
     298,   299,    60,    62,   300,   301,   302,   303,    64,    43,
      45,    42,    47,    37,   304,   305,   306,    46,    91,   307,
      93,   126,    33,    40,    58
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    75,    76,    77,    77,    77,    78,    78,    79,    79,
      79,    79,    79,    80,    80,    81,    81,    81,    81,    81,
      81,    81,    81,    81,    81,    81,    81,    81,    81,    81,
      81,    81,    81,    81,    81,    81,    81,    81,    81,    81,
      81,    81,    81,    81,    81,    81,    81,    81,    82,    83,
      83,    83,    83,    84,    84,    84,    85,    85,    86,    86,
      86,    86,    87,    87,    88,    88,    89,    89,    90,    90,
      90,    90,    91,    92,    92,    93,    93,    93,    93,    93,
      94,    94,    94,    94,    94,    95,    95,    95,    96,    96,
      97,    97,    97,    98,    98,    99,    99,    99,    99,    99,
      99,   100,   100,   101,   101,   101,   101,   101,   101,   101,
     102,   102,   102,   102,   103,   103,   104,   104,   104,   104,
     104,   104,   104,   104,   104,   104,   104,   104,   105,   105,
     105,   106,   106,   106,   106,   107,   107,   107,   107,   107,
     108,   108,   109,   109,   110,   110,   110,   110,   111,   111,
     111,   111,   111,   112,   112,   112,   112,   112,   113,   114,
     114,   114,   114,   113,   113,   113,   113,   113,   113,   113,
     113,   113,   113,   113,   113,   113,   113,   113,   113,   113,
     113,   113,   113,   113,   113,   113,   113,   113,   113,   113,
     113,   113,   113,   113,   113
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     1,     1,     1,     0,     2,     2,     3,
       3,     2,     2,     2,     4,     2,     2,     4,     4,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     3,     2,     3,     3,     2,     2,
       1,     3,     2,     1,     4,     2,     1,     2,     2,     1,
       2,     1,     1,     1,     1,     2,     2,     1,     2,     3,
       2,     3,     4,     1,     3,     1,     2,     2,     4,     1,
       1,     2,     3,     4,     3,     4,     4,     3,     0,     1,
       1,     1,     1,     1,     2,     1,     1,     1,     1,     1,
       1,     1,     2,     1,     1,     1,     2,     1,     1,     1,
       2,     1,     3,     4,     2,     3,     2,     1,     3,     2,
       2,     1,     3,     2,     3,     2,     4,     3,     1,     2,
       1,     3,     2,     2,     1,     1,     2,     1,     4,     2,
       1,     2,     2,     1,     3,     2,     2,     1,     2,     1,
       1,     4,     4,     4,     2,     2,     2,     2,     3,     1,
       3,     2,     4,     2,     2,     2,     4,     7,     7,     7,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     5,
       1,     1,     4,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,    59,    97,     0,     0,    96,    99,   100,    95,    92,
      91,   105,   107,    90,   109,   104,    98,   108,     0,     0,
       0,     0,     2,     5,     4,    53,    50,     6,    67,   121,
       0,    64,     0,    61,    93,     0,   101,   103,   117,   140,
       3,    68,     0,    52,   125,    65,     0,     0,    15,    16,
      32,    43,    29,    40,    39,    26,    24,    25,    36,    35,
      30,    31,    38,    37,    34,    33,    19,    20,    21,    22,
      23,    42,    41,     0,    45,    27,    28,     0,     0,    48,
     106,    13,     0,    55,     1,     0,     0,     0,   111,    88,
       0,     0,    11,     0,     0,     6,   135,   134,   137,    12,
     120,     0,     6,    58,    49,    66,    60,    70,    94,     0,
     123,   119,    99,   102,   116,     0,     0,     0,    62,    56,
     149,    63,     0,     6,   128,   141,   130,     8,   150,   190,
     191,     0,     0,     0,     0,   193,   194,     0,     0,     0,
       0,     0,     0,    73,    75,    79,   124,    51,     0,     0,
      47,    44,    46,     0,     0,     7,     0,   110,    89,     0,
     114,     0,   109,    88,     0,     0,     0,   128,    80,     0,
       0,    88,     0,     0,   139,     0,   136,   132,   133,    10,
      69,    71,   127,   122,   118,    57,     0,   128,   156,   157,
       9,     0,   129,   148,   132,   154,   155,     0,     0,     0,
       0,     0,    77,   163,   165,   164,     0,   140,     0,   159,
       0,     0,    72,    76,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    17,    18,    14,    54,    88,   115,
       0,    88,    87,    88,     0,    81,   131,   112,     0,     0,
     126,     0,   147,   128,     0,   143,     0,     0,     0,     0,
       0,     0,     0,     0,   161,     0,     0,   158,    74,     0,
     186,   185,   184,   183,   182,   178,   177,   181,   180,   179,
     176,   175,   173,   174,   170,   171,   172,   188,   187,   113,
      86,    85,    84,    82,   138,     0,   142,   153,   145,   146,
     151,   152,   192,     0,     0,     0,    78,     0,   166,   160,
       0,    83,   144,     0,     0,     0,   162,   189,     0,     0,
       0,   167,   169,   168
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,    21,   155,    92,    23,    24,    25,    26,    27,    28,
     118,    29,   252,    30,    31,    78,    33,   142,   143,   166,
      95,   157,    34,    35,    36,    37,    38,   167,    97,    39,
     169,   126,    99,    40,   254,   255,   127,   128,   209,   210
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -193
static const short yypact[] =
{
     771,    28,  -193,    45,   545,  -193,   -16,  -193,  -193,  -193,
    -193,  -193,  -193,  -193,  -193,  -193,  -193,  -193,   771,   771,
      15,    58,  -193,  -193,  -193,    -4,  -193,     5,  -193,   116,
     -22,  -193,    48,    55,   116,   887,  -193,   244,   116,   295,
    -193,  -193,   390,  -193,   116,  -193,    48,    66,    31,    36,
    -193,  -193,  -193,  -193,  -193,  -193,  -193,  -193,  -193,  -193,
    -193,  -193,  -193,  -193,  -193,  -193,  -193,  -193,  -193,  -193,
    -193,  -193,  -193,    13,    34,  -193,  -193,    70,   102,  -193,
    -193,  -193,    81,  -193,  -193,   390,    28,   771,  -193,   116,
       6,   610,  -193,     8,    55,    98,    52,  -193,   -38,  -193,
    -193,   512,    98,    33,  -193,  -193,   115,  -193,  -193,    66,
     116,   116,  -193,  -193,  -193,    65,   798,   610,  -193,  -193,
     -38,  -193,    51,    98,   311,  -193,   -38,  -193,   -38,  -193,
    -193,    56,    83,    87,    91,  -193,  -193,   663,   266,   266,
     266,   454,     7,  -193,   285,   904,  -193,  -193,    75,    96,
    -193,  -193,  -193,   771,    10,  -193,    67,  -193,  -193,   100,
    -193,    66,   110,   116,   285,    27,   106,   285,   285,   130,
      33,   116,   115,   771,  -193,   169,  -193,   167,  -193,  -193,
    -193,  -193,   116,  -193,  -193,  -193,    69,   718,   171,  -193,
    -193,   285,  -193,  -193,  -193,   172,  -193,   863,   863,   863,
     863,   771,  -193,   -41,   -41,   -41,   687,   285,   140,   878,
     144,   390,  -193,  -193,   266,   266,   266,   266,   266,   266,
     266,   266,   266,   266,   266,   266,   266,   266,   266,   266,
     266,   266,   183,   185,  -193,  -193,  -193,  -193,   116,  -193,
      32,   116,  -193,   116,   795,  -193,  -193,  -193,    37,   771,
    -193,   718,  -193,   718,   152,   -38,   771,   771,   153,   145,
     146,   147,   156,   771,  -193,   266,   266,  -193,  -193,   694,
     928,   951,   973,   994,  1014,   593,   593,  1028,  1028,  1028,
     375,   375,   308,   308,   -41,   -41,   -41,  -193,  -193,  -193,
    -193,  -193,  -193,   285,  -193,   161,  -193,  -193,  -193,  -193,
    -193,  -193,  -193,   131,   160,   164,  -193,   162,   -41,   904,
     266,  -193,  -193,   492,   492,   492,  -193,   904,   193,   199,
     200,  -193,  -193,  -193
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
    -193,  -193,    25,    57,  -193,  -193,  -193,     1,  -193,     9,
    -193,    -1,   -34,   -24,     3,     0,   150,   157,    47,  -193,
     -23,  -149,  -193,   210,   208,  -193,   212,   -15,   -97,   188,
     -18,   -19,   165,   151,  -192,  -193,   138,  -193,    -6,  -159
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned short yytable[] =
{
      32,   178,    44,    46,    43,   120,    45,   102,    98,   159,
      86,    80,    96,   170,   242,   121,   123,    87,    32,    32,
      83,   125,   247,   189,   124,    22,   232,    93,   233,   178,
      90,   196,   103,   104,   110,   101,   145,   103,   119,   122,
      41,   105,   170,    81,    82,   180,    44,   147,    85,   211,
       1,   101,   211,   103,    88,   105,   103,    86,    84,   295,
     212,   296,     4,   237,   175,     4,    89,   107,   116,   171,
       1,     1,   103,    90,   103,   177,   160,    98,    91,   145,
      42,    96,   116,   150,     4,    42,   116,   156,   171,   289,
     120,   165,   290,   238,   291,   151,    93,   188,   238,   148,
     121,    88,   187,   194,   149,   195,   192,   103,   182,   124,
     173,   152,   171,    89,   185,   186,    20,   165,    45,    20,
      90,   153,    20,   119,   122,    91,   213,   181,   238,   197,
     238,   105,   203,   204,   205,   198,    20,    32,    20,   199,
      20,     9,    10,   200,    93,   234,    13,   243,   244,   192,
     245,   241,   174,    32,   318,   319,   320,   104,   299,   179,
      44,   240,   202,   121,    93,   105,   235,    93,    93,   192,
     239,   246,   253,    32,    86,   248,   187,    94,   236,   249,
     190,   265,   106,   256,   257,   267,   119,   122,   287,   125,
     288,    93,    79,   297,   302,   105,   106,   306,   303,   304,
     305,    32,   312,   316,   313,   145,    32,    93,   269,   270,
     271,   272,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   282,   283,   284,   285,   286,   262,   121,   106,   121,
     144,   264,   298,   314,   321,   192,   253,   315,   253,   100,
     322,   323,   154,   172,   108,   113,    94,   111,   114,    32,
     119,   122,   119,   122,   146,     2,    32,    32,   268,   308,
     309,   176,   193,    32,     0,     5,   112,     7,     8,   129,
     130,     0,   106,   144,   294,   311,   131,    16,     0,   168,
       0,   300,   301,     0,   132,   133,   134,     0,   307,   168,
      86,     0,   208,    93,    94,     0,     0,   175,     0,   158,
       1,     0,   135,   136,   317,   168,   106,   115,     0,     0,
       0,     0,   116,     0,    94,   106,     1,    94,    94,     0,
     183,   184,     0,   115,     0,    94,   138,     0,   116,   207,
       0,     0,     0,     0,    88,     0,   106,   139,   140,   141,
       0,    94,     0,     0,    88,     0,    89,     0,   258,   259,
     260,   261,     0,    90,     0,     0,    89,    94,   164,     0,
      88,     0,     0,    90,     0,     0,    20,     0,   117,   229,
     230,   231,    89,   158,     0,   232,     0,   233,     0,    90,
       0,   158,    20,     0,   191,   207,   207,   207,   207,     0,
     106,     0,   250,   129,   130,     1,     0,     0,   172,   144,
     131,     2,    47,     0,     0,     0,     0,     0,   132,   133,
     134,     5,     6,     7,     8,     9,    10,    11,    12,     0,
      13,    14,    15,    16,    17,     0,   135,   136,     0,     0,
       0,     0,   293,     0,   227,   228,   229,   230,   231,   137,
       0,     0,   232,    94,   233,     0,     0,     0,   158,     0,
     138,   158,     0,   158,     0,     0,     0,   129,   130,     1,
       0,   139,   140,   141,   131,     2,    47,     0,     0,     0,
       0,     0,   132,   133,   134,     5,     6,     7,     8,     9,
      10,    11,    12,     0,    13,    14,    15,    16,    17,     0,
     135,   136,     0,     0,     0,   129,   130,     0,     0,     0,
       0,     0,   131,   206,     0,     0,     0,     0,     0,     0,
     132,   133,   134,     0,   138,     0,     0,     1,     0,     0,
       0,     0,     0,     2,    47,   139,   140,   141,   135,   136,
       0,     0,     0,     5,     6,     7,     8,     9,    10,    11,
      12,   206,    13,   162,    15,    16,    17,     0,     0,     0,
       1,     0,   138,   163,     0,     0,     2,    47,     0,     0,
      48,    49,     0,   139,   140,   141,     5,     6,     7,     8,
       9,    10,    11,    12,     0,    13,    14,    15,    16,    17,
      50,     0,     0,     0,     0,     0,     0,    51,    52,     0,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,     0,    66,    67,    68,    69,    70,    71,
      72,     0,     0,    73,    74,     1,    75,    76,    77,     0,
       0,     2,   161,     0,     0,     0,     0,     0,     0,     0,
       0,     5,     6,     7,     8,     9,    10,    11,    12,     0,
      13,   162,    15,    16,    17,   222,     0,   223,   224,   225,
     226,   163,   227,   228,   229,   230,   231,     0,     0,    88,
     232,     0,   233,     0,     0,     0,     0,     0,     1,     0,
       0,    89,     0,     0,     2,     3,     0,     0,    90,     0,
       4,     0,     0,   164,     5,     6,     7,     8,     9,    10,
      11,    12,     1,    13,    14,    15,    16,    17,     2,     3,
       0,    18,    19,     0,     4,     0,     0,     0,     5,     6,
       7,     8,     9,    10,    11,    12,     0,    13,    14,    15,
      16,    17,     0,     1,     0,    18,    19,     0,     0,     0,
     115,     0,     0,     0,    20,   116,   201,     0,   214,   215,
     216,   217,   218,   219,   220,   221,   222,     0,   223,   224,
     225,   226,     0,   227,   228,   229,   230,   231,    20,     0,
     263,   232,     0,   233,     0,     0,     0,    88,   310,     0,
       0,     0,     0,     0,     0,     0,     1,     0,     0,    89,
       0,     0,     2,     3,     0,     0,    90,     0,     4,    20,
       0,   251,     5,     6,     7,     8,     9,    10,    11,    12,
       1,    13,    14,    15,    16,    17,     2,    47,     0,    18,
      19,     0,     0,    48,    49,     0,     5,     6,     7,     8,
       9,    10,    11,    12,   292,    13,    14,    15,    16,    17,
       0,     0,     0,    50,     0,     0,     0,     0,     0,     0,
      51,    52,    20,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,     0,    66,    67,    68,
      69,    70,    71,    72,     0,     0,    73,    74,     1,    75,
      76,    77,     0,     0,     2,    47,     0,     0,     0,     0,
       0,     0,     0,     0,     5,     6,     7,     8,     9,    10,
      11,    12,     1,    13,    14,    15,    16,    17,     2,   109,
       0,     0,     0,     0,     0,     0,     0,     0,     5,     6,
       7,     8,     0,     0,    11,    12,     0,     0,    14,    15,
      16,    17,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   266,   223,   224,   225,   226,     0,   227,   228,   229,
     230,   231,     0,     0,     0,   232,     0,   233,   214,   215,
     216,   217,   218,   219,   220,   221,   222,     0,   223,   224,
     225,   226,     0,   227,   228,   229,   230,   231,     0,     0,
       0,   232,     0,   233,   216,   217,   218,   219,   220,   221,
     222,     0,   223,   224,   225,   226,     0,   227,   228,   229,
     230,   231,     0,     0,     0,   232,     0,   233,   217,   218,
     219,   220,   221,   222,     0,   223,   224,   225,   226,     0,
     227,   228,   229,   230,   231,     0,     0,     0,   232,     0,
     233,   218,   219,   220,   221,   222,     0,   223,   224,   225,
     226,     0,   227,   228,   229,   230,   231,     0,     0,     0,
     232,     0,   233,   219,   220,   221,   222,     0,   223,   224,
     225,   226,     0,   227,   228,   229,   230,   231,     0,     0,
       0,   232,     0,   233,   220,   221,   222,     0,   223,   224,
     225,   226,     0,   227,   228,   229,   230,   231,     0,     0,
       0,   232,     0,   233,   225,   226,     0,   227,   228,   229,
     230,   231,     0,     0,     0,   232,     0,   233
};

static const short yycheck[] =
{
       0,    98,     3,     3,     3,    39,     3,    30,    27,     3,
       5,    27,    27,     5,   163,    39,    39,    12,    18,    19,
       5,    39,   171,   120,    39,     0,    67,    27,    69,   126,
      68,   128,     5,    32,    35,    73,    42,     5,    39,    39,
      12,    32,     5,    18,    19,    12,    47,    46,    52,    42,
       5,    73,    42,     5,    49,    46,     5,     5,     0,   251,
      53,   253,    17,    53,    12,    17,    61,    12,    17,    61,
       5,     5,     5,    68,     5,    98,    70,    96,    73,    85,
      52,    96,    17,    70,    17,    52,    17,    87,    61,   238,
     124,    91,   241,    61,   243,    61,    96,   120,    61,    68,
     124,    49,   117,   126,    68,   128,   124,     5,   109,   124,
      12,    41,    61,    61,   115,   115,    71,   117,   115,    71,
      68,    40,    71,   124,   124,    73,   144,    12,    61,    73,
      61,   122,   138,   139,   140,    52,    71,   137,    71,    52,
      71,    25,    26,    52,   144,    70,    30,    41,    42,   167,
     168,    41,    95,   153,   313,   314,   315,   156,   255,   102,
     161,   161,   137,   187,   164,   156,    70,   167,   168,   187,
      70,    41,   187,   173,     5,   175,   191,    27,   153,    12,
     123,    41,    32,    12,    12,    41,   187,   187,     5,   207,
       5,   191,     4,    41,    41,   186,    46,    41,    53,    53,
      53,   201,    41,    41,    73,   211,   206,   207,   214,   215,
     216,   217,   218,   219,   220,   221,   222,   223,   224,   225,
     226,   227,   228,   229,   230,   231,   201,   251,    78,   253,
      42,   206,   255,    73,    41,   253,   251,    73,   253,    29,
      41,    41,    85,    93,    34,    37,    96,    35,    38,   249,
     251,   251,   253,   253,    44,    11,   256,   257,   211,   265,
     266,    96,   124,   263,    -1,    21,    22,    23,    24,     3,
       4,    -1,   122,    85,   249,   293,    10,    33,    -1,    91,
      -1,   256,   257,    -1,    18,    19,    20,    -1,   263,   101,
       5,    -1,   141,   293,   144,    -1,    -1,    12,    -1,    89,
       5,    -1,    36,    37,   310,   117,   156,    12,    -1,    -1,
      -1,    -1,    17,    -1,   164,   165,     5,   167,   168,    -1,
     110,   111,    -1,    12,    -1,   175,    60,    -1,    17,   141,
      -1,    -1,    -1,    -1,    49,    -1,   186,    71,    72,    73,
      -1,   191,    -1,    -1,    49,    -1,    61,    -1,   197,   198,
     199,   200,    -1,    68,    -1,    -1,    61,   207,    73,    -1,
      49,    -1,    -1,    68,    -1,    -1,    71,    -1,    73,    61,
      62,    63,    61,   163,    -1,    67,    -1,    69,    -1,    68,
      -1,   171,    71,    -1,    73,   197,   198,   199,   200,    -1,
     240,    -1,   182,     3,     4,     5,    -1,    -1,   248,   211,
      10,    11,    12,    -1,    -1,    -1,    -1,    -1,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    -1,
      30,    31,    32,    33,    34,    -1,    36,    37,    -1,    -1,
      -1,    -1,   244,    -1,    59,    60,    61,    62,    63,    49,
      -1,    -1,    67,   293,    69,    -1,    -1,    -1,   238,    -1,
      60,   241,    -1,   243,    -1,    -1,    -1,     3,     4,     5,
      -1,    71,    72,    73,    10,    11,    12,    -1,    -1,    -1,
      -1,    -1,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    -1,    30,    31,    32,    33,    34,    -1,
      36,    37,    -1,    -1,    -1,     3,     4,    -1,    -1,    -1,
      -1,    -1,    10,    49,    -1,    -1,    -1,    -1,    -1,    -1,
      18,    19,    20,    -1,    60,    -1,    -1,     5,    -1,    -1,
      -1,    -1,    -1,    11,    12,    71,    72,    73,    36,    37,
      -1,    -1,    -1,    21,    22,    23,    24,    25,    26,    27,
      28,    49,    30,    31,    32,    33,    34,    -1,    -1,    -1,
       5,    -1,    60,    41,    -1,    -1,    11,    12,    -1,    -1,
      15,    16,    -1,    71,    72,    73,    21,    22,    23,    24,
      25,    26,    27,    28,    -1,    30,    31,    32,    33,    34,
      35,    -1,    -1,    -1,    -1,    -1,    -1,    42,    43,    -1,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    -1,    59,    60,    61,    62,    63,    64,
      65,    -1,    -1,    68,    69,     5,    71,    72,    73,    -1,
      -1,    11,    12,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    21,    22,    23,    24,    25,    26,    27,    28,    -1,
      30,    31,    32,    33,    34,    52,    -1,    54,    55,    56,
      57,    41,    59,    60,    61,    62,    63,    -1,    -1,    49,
      67,    -1,    69,    -1,    -1,    -1,    -1,    -1,     5,    -1,
      -1,    61,    -1,    -1,    11,    12,    -1,    -1,    68,    -1,
      17,    -1,    -1,    73,    21,    22,    23,    24,    25,    26,
      27,    28,     5,    30,    31,    32,    33,    34,    11,    12,
      -1,    38,    39,    -1,    17,    -1,    -1,    -1,    21,    22,
      23,    24,    25,    26,    27,    28,    -1,    30,    31,    32,
      33,    34,    -1,     5,    -1,    38,    39,    -1,    -1,    -1,
      12,    -1,    -1,    -1,    71,    17,    73,    -1,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    -1,    54,    55,
      56,    57,    -1,    59,    60,    61,    62,    63,    71,    -1,
      73,    67,    -1,    69,    -1,    -1,    -1,    49,    74,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     5,    -1,    -1,    61,
      -1,    -1,    11,    12,    -1,    -1,    68,    -1,    17,    71,
      -1,    73,    21,    22,    23,    24,    25,    26,    27,    28,
       5,    30,    31,    32,    33,    34,    11,    12,    -1,    38,
      39,    -1,    -1,    15,    16,    -1,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      -1,    -1,    -1,    35,    -1,    -1,    -1,    -1,    -1,    -1,
      42,    43,    71,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    -1,    59,    60,    61,
      62,    63,    64,    65,    -1,    -1,    68,    69,     5,    71,
      72,    73,    -1,    -1,    11,    12,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    21,    22,    23,    24,    25,    26,
      27,    28,     5,    30,    31,    32,    33,    34,    11,    12,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      23,    24,    -1,    -1,    27,    28,    -1,    -1,    31,    32,
      33,    34,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    -1,    59,    60,    61,
      62,    63,    -1,    -1,    -1,    67,    -1,    69,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    -1,    54,    55,
      56,    57,    -1,    59,    60,    61,    62,    63,    -1,    -1,
      -1,    67,    -1,    69,    46,    47,    48,    49,    50,    51,
      52,    -1,    54,    55,    56,    57,    -1,    59,    60,    61,
      62,    63,    -1,    -1,    -1,    67,    -1,    69,    47,    48,
      49,    50,    51,    52,    -1,    54,    55,    56,    57,    -1,
      59,    60,    61,    62,    63,    -1,    -1,    -1,    67,    -1,
      69,    48,    49,    50,    51,    52,    -1,    54,    55,    56,
      57,    -1,    59,    60,    61,    62,    63,    -1,    -1,    -1,
      67,    -1,    69,    49,    50,    51,    52,    -1,    54,    55,
      56,    57,    -1,    59,    60,    61,    62,    63,    -1,    -1,
      -1,    67,    -1,    69,    50,    51,    52,    -1,    54,    55,
      56,    57,    -1,    59,    60,    61,    62,    63,    -1,    -1,
      -1,    67,    -1,    69,    56,    57,    -1,    59,    60,    61,
      62,    63,    -1,    -1,    -1,    67,    -1,    69
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     5,    11,    12,    17,    21,    22,    23,    24,    25,
      26,    27,    28,    30,    31,    32,    33,    34,    38,    39,
      71,    76,    77,    79,    80,    81,    82,    83,    84,    86,
      88,    89,    90,    91,    97,    98,    99,   100,   101,   104,
     108,    12,    52,    82,    86,    89,    90,    12,    15,    16,
      35,    42,    43,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    59,    60,    61,    62,
      63,    64,    65,    68,    69,    71,    72,    73,    90,   104,
      27,    77,    77,     5,     0,    52,     5,    12,    49,    61,
      68,    73,    78,    90,    91,    95,   102,   103,   106,   107,
      98,    73,    95,     5,    82,    84,    91,    12,    98,    12,
      86,   101,    22,    99,    98,    12,    17,    73,    85,    86,
      87,    88,    90,    95,   102,   105,   106,   111,   112,     3,
       4,    10,    18,    19,    20,    36,    37,    49,    60,    71,
      72,    73,    92,    93,   104,   113,    98,    82,    68,    68,
      70,    61,    41,    40,    92,    77,    90,    96,    98,     3,
      70,    12,    31,    41,    73,    90,    94,   102,   104,   105,
       5,    61,    91,    12,    78,    12,   107,    95,   103,    78,
      12,    12,    86,    98,    98,    86,    90,   102,    95,   103,
      78,    73,   105,   111,    95,    95,   103,    73,    52,    52,
      52,    73,    77,   113,   113,   113,    49,   104,   108,   113,
     114,    42,    53,   105,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    54,    55,    56,    57,    59,    60,    61,
      62,    63,    67,    69,    70,    70,    77,    53,    61,    70,
      90,    41,    96,    41,    42,   105,    41,    96,    90,    12,
      98,    73,    87,   102,   109,   110,    12,    12,   108,   108,
     108,   108,    77,    73,    77,    41,    53,    41,    93,   113,
     113,   113,   113,   113,   113,   113,   113,   113,   113,   113,
     113,   113,   113,   113,   113,   113,   113,     5,     5,    96,
      96,    96,    29,   104,    77,   109,   109,    41,    95,   103,
      77,    77,    41,    53,    53,    53,    41,    77,   113,   113,
      74,   105,    41,    73,    73,    73,    41,   113,   114,   114,
     114,    41,    41,    41
};

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
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


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
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)		\
   ((Current).first_line   = (Rhs)[1].first_line,	\
    (Current).first_column = (Rhs)[1].first_column,	\
    (Current).last_line    = (Rhs)[N].last_line,	\
    (Current).last_column  = (Rhs)[N].last_column)
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

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

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
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

#if defined (YYMAXDEPTH) && YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

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

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to xreallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
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

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to xreallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
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
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


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

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 357 "cp-name-parser.y"
    { global_result = yyvsp[0].comp; }
    break;

  case 6:
#line 369 "cp-name-parser.y"
    { yyval.comp = NULL; }
    break;

  case 7:
#line 371 "cp-name-parser.y"
    { yyval.comp = yyvsp[0].comp; }
    break;

  case 8:
#line 378 "cp-name-parser.y"
    { yyval.comp = yyvsp[0].nested.comp;
			  *yyvsp[0].nested.last = yyvsp[-1].comp;
			}
    break;

  case 9:
#line 387 "cp-name-parser.y"
    { yyval.comp = fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, yyvsp[-2].comp, yyvsp[-1].nested.comp);
			  if (yyvsp[0].comp) yyval.comp = fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME, yyval.comp, yyvsp[0].comp); }
    break;

  case 10:
#line 390 "cp-name-parser.y"
    { yyval.comp = fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, yyvsp[-2].comp, yyvsp[-1].nested.comp);
			  if (yyvsp[0].comp) yyval.comp = fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME, yyval.comp, yyvsp[0].comp); }
    break;

  case 11:
#line 394 "cp-name-parser.y"
    { yyval.comp = yyvsp[-1].nested.comp;
			  if (yyvsp[0].comp) yyval.comp = fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME, yyval.comp, yyvsp[0].comp); }
    break;

  case 12:
#line 397 "cp-name-parser.y"
    { if (yyvsp[0].abstract.last)
			    {
			       /* First complete the abstract_declarator's type using
				  the typespec from the conversion_op_name.  */
			      *yyvsp[0].abstract.last = *yyvsp[-1].nested.last;
			      /* Then complete the conversion_op_name with the type.  */
			      *yyvsp[-1].nested.last = yyvsp[0].abstract.comp;
			    }
			  /* If we have an arglist, build a function type.  */
			  if (yyvsp[0].abstract.fn.comp)
			    yyval.comp = fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, yyvsp[-1].nested.comp, yyvsp[0].abstract.fn.comp);
			  else
			    yyval.comp = yyvsp[-1].nested.comp;
			  if (yyvsp[0].abstract.start) yyval.comp = fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME, yyval.comp, yyvsp[0].abstract.start);
			}
    break;

  case 13:
#line 416 "cp-name-parser.y"
    { yyval.comp = make_empty (yyvsp[-1].lval);
			  d_left (yyval.comp) = yyvsp[0].comp;
			  d_right (yyval.comp) = NULL; }
    break;

  case 14:
#line 420 "cp-name-parser.y"
    { yyval.comp = fill_comp (DEMANGLE_COMPONENT_CONSTRUCTION_VTABLE, yyvsp[-2].comp, yyvsp[0].comp); }
    break;

  case 15:
#line 424 "cp-name-parser.y"
    { yyval.comp = make_operator ("new", 1); }
    break;

  case 16:
#line 426 "cp-name-parser.y"
    { yyval.comp = make_operator ("delete", 1); }
    break;

  case 17:
#line 428 "cp-name-parser.y"
    { yyval.comp = make_operator ("new[]", 1); }
    break;

  case 18:
#line 430 "cp-name-parser.y"
    { yyval.comp = make_operator ("delete[]", 1); }
    break;

  case 19:
#line 432 "cp-name-parser.y"
    { yyval.comp = make_operator ("+", 2); }
    break;

  case 20:
#line 434 "cp-name-parser.y"
    { yyval.comp = make_operator ("-", 2); }
    break;

  case 21:
#line 436 "cp-name-parser.y"
    { yyval.comp = make_operator ("*", 2); }
    break;

  case 22:
#line 438 "cp-name-parser.y"
    { yyval.comp = make_operator ("/", 2); }
    break;

  case 23:
#line 440 "cp-name-parser.y"
    { yyval.comp = make_operator ("%", 2); }
    break;

  case 24:
#line 442 "cp-name-parser.y"
    { yyval.comp = make_operator ("^", 2); }
    break;

  case 25:
#line 444 "cp-name-parser.y"
    { yyval.comp = make_operator ("&", 2); }
    break;

  case 26:
#line 446 "cp-name-parser.y"
    { yyval.comp = make_operator ("|", 2); }
    break;

  case 27:
#line 448 "cp-name-parser.y"
    { yyval.comp = make_operator ("~", 1); }
    break;

  case 28:
#line 450 "cp-name-parser.y"
    { yyval.comp = make_operator ("!", 1); }
    break;

  case 29:
#line 452 "cp-name-parser.y"
    { yyval.comp = make_operator ("=", 2); }
    break;

  case 30:
#line 454 "cp-name-parser.y"
    { yyval.comp = make_operator ("<", 2); }
    break;

  case 31:
#line 456 "cp-name-parser.y"
    { yyval.comp = make_operator (">", 2); }
    break;

  case 32:
#line 458 "cp-name-parser.y"
    { yyval.comp = make_operator (yyvsp[0].opname, 2); }
    break;

  case 33:
#line 460 "cp-name-parser.y"
    { yyval.comp = make_operator ("<<", 2); }
    break;

  case 34:
#line 462 "cp-name-parser.y"
    { yyval.comp = make_operator (">>", 2); }
    break;

  case 35:
#line 464 "cp-name-parser.y"
    { yyval.comp = make_operator ("==", 2); }
    break;

  case 36:
#line 466 "cp-name-parser.y"
    { yyval.comp = make_operator ("!=", 2); }
    break;

  case 37:
#line 468 "cp-name-parser.y"
    { yyval.comp = make_operator ("<=", 2); }
    break;

  case 38:
#line 470 "cp-name-parser.y"
    { yyval.comp = make_operator (">=", 2); }
    break;

  case 39:
#line 472 "cp-name-parser.y"
    { yyval.comp = make_operator ("&&", 2); }
    break;

  case 40:
#line 474 "cp-name-parser.y"
    { yyval.comp = make_operator ("||", 2); }
    break;

  case 41:
#line 476 "cp-name-parser.y"
    { yyval.comp = make_operator ("++", 1); }
    break;

  case 42:
#line 478 "cp-name-parser.y"
    { yyval.comp = make_operator ("--", 1); }
    break;

  case 43:
#line 480 "cp-name-parser.y"
    { yyval.comp = make_operator (",", 2); }
    break;

  case 44:
#line 482 "cp-name-parser.y"
    { yyval.comp = make_operator ("->*", 2); }
    break;

  case 45:
#line 484 "cp-name-parser.y"
    { yyval.comp = make_operator ("->", 2); }
    break;

  case 46:
#line 486 "cp-name-parser.y"
    { yyval.comp = make_operator ("()", 2); }
    break;

  case 47:
#line 488 "cp-name-parser.y"
    { yyval.comp = make_operator ("[]", 2); }
    break;

  case 48:
#line 496 "cp-name-parser.y"
    { yyval.comp = fill_comp (DEMANGLE_COMPONENT_CAST, yyvsp[0].comp, NULL); }
    break;

  case 49:
#line 501 "cp-name-parser.y"
    { yyval.nested.comp = yyvsp[-1].nested1.comp;
			  d_right (yyvsp[-1].nested1.last) = yyvsp[0].comp;
			  yyval.nested.last = &d_left (yyvsp[0].comp);
			}
    break;

  case 50:
#line 506 "cp-name-parser.y"
    { yyval.nested.comp = yyvsp[0].comp;
			  yyval.nested.last = &d_left (yyvsp[0].comp);
			}
    break;

  case 51:
#line 510 "cp-name-parser.y"
    { yyval.nested.comp = yyvsp[-1].nested1.comp;
			  d_right (yyvsp[-1].nested1.last) = yyvsp[0].comp;
			  yyval.nested.last = &d_left (yyvsp[0].comp);
			}
    break;

  case 52:
#line 515 "cp-name-parser.y"
    { yyval.nested.comp = yyvsp[0].comp;
			  yyval.nested.last = &d_left (yyvsp[0].comp);
			}
    break;

  case 54:
#line 524 "cp-name-parser.y"
    { yyval.comp = fill_comp (DEMANGLE_COMPONENT_TEMPLATE, yyvsp[-3].comp, yyvsp[-1].nested.comp); }
    break;

  case 55:
#line 526 "cp-name-parser.y"
    { yyval.comp = make_dtor (gnu_v3_complete_object_dtor, yyvsp[0].comp); }
    break;

  case 57:
#line 539 "cp-name-parser.y"
    { yyval.comp = yyvsp[0].comp; }
    break;

  case 58:
#line 545 "cp-name-parser.y"
    { yyval.comp = yyvsp[-1].nested1.comp; d_right (yyvsp[-1].nested1.last) = yyvsp[0].comp; }
    break;

  case 60:
#line 548 "cp-name-parser.y"
    { yyval.comp = yyvsp[-1].nested1.comp; d_right (yyvsp[-1].nested1.last) = yyvsp[0].comp; }
    break;

  case 65:
#line 558 "cp-name-parser.y"
    { yyval.comp = yyvsp[0].comp; }
    break;

  case 66:
#line 562 "cp-name-parser.y"
    { yyval.comp = yyvsp[-1].nested1.comp; d_right (yyvsp[-1].nested1.last) = yyvsp[0].comp; }
    break;

  case 68:
#line 567 "cp-name-parser.y"
    { yyval.nested1.comp = make_empty (DEMANGLE_COMPONENT_QUAL_NAME);
			  d_left (yyval.nested1.comp) = yyvsp[-1].comp;
			  d_right (yyval.nested1.comp) = NULL;
			  yyval.nested1.last = yyval.nested1.comp;
			}
    break;

  case 69:
#line 573 "cp-name-parser.y"
    { yyval.nested1.comp = yyvsp[-2].nested1.comp;
			  d_right (yyvsp[-2].nested1.last) = make_empty (DEMANGLE_COMPONENT_QUAL_NAME);
			  yyval.nested1.last = d_right (yyvsp[-2].nested1.last);
			  d_left (yyval.nested1.last) = yyvsp[-1].comp;
			  d_right (yyval.nested1.last) = NULL;
			}
    break;

  case 70:
#line 580 "cp-name-parser.y"
    { yyval.nested1.comp = make_empty (DEMANGLE_COMPONENT_QUAL_NAME);
			  d_left (yyval.nested1.comp) = yyvsp[-1].comp;
			  d_right (yyval.nested1.comp) = NULL;
			  yyval.nested1.last = yyval.nested1.comp;
			}
    break;

  case 71:
#line 586 "cp-name-parser.y"
    { yyval.nested1.comp = yyvsp[-2].nested1.comp;
			  d_right (yyvsp[-2].nested1.last) = make_empty (DEMANGLE_COMPONENT_QUAL_NAME);
			  yyval.nested1.last = d_right (yyvsp[-2].nested1.last);
			  d_left (yyval.nested1.last) = yyvsp[-1].comp;
			  d_right (yyval.nested1.last) = NULL;
			}
    break;

  case 72:
#line 597 "cp-name-parser.y"
    { yyval.comp = fill_comp (DEMANGLE_COMPONENT_TEMPLATE, yyvsp[-3].comp, yyvsp[-1].nested.comp); }
    break;

  case 73:
#line 601 "cp-name-parser.y"
    { yyval.nested.comp = fill_comp (DEMANGLE_COMPONENT_TEMPLATE_ARGLIST, yyvsp[0].comp, NULL);
			yyval.nested.last = &d_right (yyval.nested.comp); }
    break;

  case 74:
#line 604 "cp-name-parser.y"
    { yyval.nested.comp = yyvsp[-2].nested.comp;
			  *yyvsp[-2].nested.last = fill_comp (DEMANGLE_COMPONENT_TEMPLATE_ARGLIST, yyvsp[0].comp, NULL);
			  yyval.nested.last = &d_right (*yyvsp[-2].nested.last);
			}
    break;

  case 76:
#line 616 "cp-name-parser.y"
    { yyval.comp = yyvsp[0].abstract.comp;
			  *yyvsp[0].abstract.last = yyvsp[-1].comp;
			}
    break;

  case 77:
#line 620 "cp-name-parser.y"
    { yyval.comp = fill_comp (DEMANGLE_COMPONENT_UNARY, make_operator ("&", 1), yyvsp[0].comp); }
    break;

  case 78:
#line 622 "cp-name-parser.y"
    { yyval.comp = fill_comp (DEMANGLE_COMPONENT_UNARY, make_operator ("&", 1), yyvsp[-1].comp); }
    break;

  case 80:
#line 627 "cp-name-parser.y"
    { yyval.nested.comp = fill_comp (DEMANGLE_COMPONENT_ARGLIST, yyvsp[0].comp, NULL);
			  yyval.nested.last = &d_right (yyval.nested.comp);
			}
    break;

  case 81:
#line 631 "cp-name-parser.y"
    { *yyvsp[0].abstract.last = yyvsp[-1].comp;
			  yyval.nested.comp = fill_comp (DEMANGLE_COMPONENT_ARGLIST, yyvsp[0].abstract.comp, NULL);
			  yyval.nested.last = &d_right (yyval.nested.comp);
			}
    break;

  case 82:
#line 636 "cp-name-parser.y"
    { *yyvsp[-2].nested.last = fill_comp (DEMANGLE_COMPONENT_ARGLIST, yyvsp[0].comp, NULL);
			  yyval.nested.comp = yyvsp[-2].nested.comp;
			  yyval.nested.last = &d_right (*yyvsp[-2].nested.last);
			}
    break;

  case 83:
#line 641 "cp-name-parser.y"
    { *yyvsp[0].abstract.last = yyvsp[-1].comp;
			  *yyvsp[-3].nested.last = fill_comp (DEMANGLE_COMPONENT_ARGLIST, yyvsp[0].abstract.comp, NULL);
			  yyval.nested.comp = yyvsp[-3].nested.comp;
			  yyval.nested.last = &d_right (*yyvsp[-3].nested.last);
			}
    break;

  case 84:
#line 647 "cp-name-parser.y"
    { *yyvsp[-2].nested.last
			    = fill_comp (DEMANGLE_COMPONENT_ARGLIST,
					   make_builtin_type ("..."),
					   NULL);
			  yyval.nested.comp = yyvsp[-2].nested.comp;
			  yyval.nested.last = &d_right (*yyvsp[-2].nested.last);
			}
    break;

  case 85:
#line 657 "cp-name-parser.y"
    { yyval.nested.comp = fill_comp (DEMANGLE_COMPONENT_FUNCTION_TYPE, NULL, yyvsp[-2].nested.comp);
			  yyval.nested.last = &d_left (yyval.nested.comp);
			  yyval.nested.comp = d_qualify (yyval.nested.comp, yyvsp[0].lval, 1); }
    break;

  case 86:
#line 661 "cp-name-parser.y"
    { yyval.nested.comp = fill_comp (DEMANGLE_COMPONENT_FUNCTION_TYPE, NULL, NULL);
			  yyval.nested.last = &d_left (yyval.nested.comp);
			  yyval.nested.comp = d_qualify (yyval.nested.comp, yyvsp[0].lval, 1); }
    break;

  case 87:
#line 665 "cp-name-parser.y"
    { yyval.nested.comp = fill_comp (DEMANGLE_COMPONENT_FUNCTION_TYPE, NULL, NULL);
			  yyval.nested.last = &d_left (yyval.nested.comp);
			  yyval.nested.comp = d_qualify (yyval.nested.comp, yyvsp[0].lval, 1); }
    break;

  case 88:
#line 672 "cp-name-parser.y"
    { yyval.lval = 0; }
    break;

  case 90:
#line 677 "cp-name-parser.y"
    { yyval.lval = QUAL_RESTRICT; }
    break;

  case 91:
#line 679 "cp-name-parser.y"
    { yyval.lval = QUAL_VOLATILE; }
    break;

  case 92:
#line 681 "cp-name-parser.y"
    { yyval.lval = QUAL_CONST; }
    break;

  case 94:
#line 686 "cp-name-parser.y"
    { yyval.lval = yyvsp[-1].lval | yyvsp[0].lval; }
    break;

  case 95:
#line 693 "cp-name-parser.y"
    { yyval.lval = 0; }
    break;

  case 96:
#line 695 "cp-name-parser.y"
    { yyval.lval = INT_SIGNED; }
    break;

  case 97:
#line 697 "cp-name-parser.y"
    { yyval.lval = INT_UNSIGNED; }
    break;

  case 98:
#line 699 "cp-name-parser.y"
    { yyval.lval = INT_CHAR; }
    break;

  case 99:
#line 701 "cp-name-parser.y"
    { yyval.lval = INT_LONG; }
    break;

  case 100:
#line 703 "cp-name-parser.y"
    { yyval.lval = INT_SHORT; }
    break;

  case 102:
#line 708 "cp-name-parser.y"
    { yyval.lval = yyvsp[-1].lval | yyvsp[0].lval; if (yyvsp[-1].lval & yyvsp[0].lval & INT_LONG) yyval.lval = yyvsp[-1].lval | INT_LLONG; }
    break;

  case 103:
#line 712 "cp-name-parser.y"
    { yyval.comp = d_int_type (yyvsp[0].lval); }
    break;

  case 104:
#line 714 "cp-name-parser.y"
    { yyval.comp = make_builtin_type ("float"); }
    break;

  case 105:
#line 716 "cp-name-parser.y"
    { yyval.comp = make_builtin_type ("double"); }
    break;

  case 106:
#line 718 "cp-name-parser.y"
    { yyval.comp = make_builtin_type ("long double"); }
    break;

  case 107:
#line 720 "cp-name-parser.y"
    { yyval.comp = make_builtin_type ("bool"); }
    break;

  case 108:
#line 722 "cp-name-parser.y"
    { yyval.comp = make_builtin_type ("wchar_t"); }
    break;

  case 109:
#line 724 "cp-name-parser.y"
    { yyval.comp = make_builtin_type ("void"); }
    break;

  case 110:
#line 728 "cp-name-parser.y"
    { yyval.nested.comp = make_empty (DEMANGLE_COMPONENT_POINTER);
			  yyval.nested.comp->u.s_binary.left = yyval.nested.comp->u.s_binary.right = NULL;
			  yyval.nested.last = &d_left (yyval.nested.comp);
			  yyval.nested.comp = d_qualify (yyval.nested.comp, yyvsp[0].lval, 0); }
    break;

  case 111:
#line 734 "cp-name-parser.y"
    { yyval.nested.comp = make_empty (DEMANGLE_COMPONENT_REFERENCE);
			  yyval.nested.comp->u.s_binary.left = yyval.nested.comp->u.s_binary.right = NULL;
			  yyval.nested.last = &d_left (yyval.nested.comp); }
    break;

  case 112:
#line 738 "cp-name-parser.y"
    { yyval.nested.comp = make_empty (DEMANGLE_COMPONENT_PTRMEM_TYPE);
			  yyval.nested.comp->u.s_binary.left = yyvsp[-2].nested1.comp;
			  /* Convert the innermost DEMANGLE_COMPONENT_QUAL_NAME to a DEMANGLE_COMPONENT_NAME.  */
			  *yyvsp[-2].nested1.last = *d_left (yyvsp[-2].nested1.last);
			  yyval.nested.comp->u.s_binary.right = NULL;
			  yyval.nested.last = &d_right (yyval.nested.comp);
			  yyval.nested.comp = d_qualify (yyval.nested.comp, yyvsp[0].lval, 0); }
    break;

  case 113:
#line 746 "cp-name-parser.y"
    { yyval.nested.comp = make_empty (DEMANGLE_COMPONENT_PTRMEM_TYPE);
			  yyval.nested.comp->u.s_binary.left = yyvsp[-2].nested1.comp;
			  /* Convert the innermost DEMANGLE_COMPONENT_QUAL_NAME to a DEMANGLE_COMPONENT_NAME.  */
			  *yyvsp[-2].nested1.last = *d_left (yyvsp[-2].nested1.last);
			  yyval.nested.comp->u.s_binary.right = NULL;
			  yyval.nested.last = &d_right (yyval.nested.comp);
			  yyval.nested.comp = d_qualify (yyval.nested.comp, yyvsp[0].lval, 0); }
    break;

  case 114:
#line 756 "cp-name-parser.y"
    { yyval.comp = make_empty (DEMANGLE_COMPONENT_ARRAY_TYPE);
			  d_left (yyval.comp) = NULL;
			}
    break;

  case 115:
#line 760 "cp-name-parser.y"
    { yyval.comp = make_empty (DEMANGLE_COMPONENT_ARRAY_TYPE);
			  d_left (yyval.comp) = yyvsp[-1].comp;
			}
    break;

  case 116:
#line 776 "cp-name-parser.y"
    { yyval.comp = d_qualify (yyvsp[-1].comp, yyvsp[0].lval, 0); }
    break;

  case 118:
#line 779 "cp-name-parser.y"
    { yyval.comp = d_qualify (yyvsp[-1].comp, yyvsp[-2].lval | yyvsp[0].lval, 0); }
    break;

  case 119:
#line 781 "cp-name-parser.y"
    { yyval.comp = d_qualify (yyvsp[0].comp, yyvsp[-1].lval, 0); }
    break;

  case 120:
#line 784 "cp-name-parser.y"
    { yyval.comp = d_qualify (yyvsp[-1].comp, yyvsp[0].lval, 0); }
    break;

  case 122:
#line 787 "cp-name-parser.y"
    { yyval.comp = d_qualify (yyvsp[-1].comp, yyvsp[-2].lval | yyvsp[0].lval, 0); }
    break;

  case 123:
#line 789 "cp-name-parser.y"
    { yyval.comp = d_qualify (yyvsp[0].comp, yyvsp[-1].lval, 0); }
    break;

  case 124:
#line 792 "cp-name-parser.y"
    { yyval.comp = d_qualify (yyvsp[-1].comp, yyvsp[0].lval, 0); }
    break;

  case 125:
#line 794 "cp-name-parser.y"
    { yyval.comp = yyvsp[0].comp; }
    break;

  case 126:
#line 796 "cp-name-parser.y"
    { yyval.comp = d_qualify (yyvsp[-1].comp, yyvsp[-3].lval | yyvsp[0].lval, 0); }
    break;

  case 127:
#line 798 "cp-name-parser.y"
    { yyval.comp = d_qualify (yyvsp[0].comp, yyvsp[-2].lval, 0); }
    break;

  case 128:
#line 803 "cp-name-parser.y"
    { yyval.abstract.comp = yyvsp[0].nested.comp; yyval.abstract.last = yyvsp[0].nested.last;
			  yyval.abstract.fn.comp = NULL; yyval.abstract.fn.last = NULL; }
    break;

  case 129:
#line 806 "cp-name-parser.y"
    { yyval.abstract = yyvsp[0].abstract; yyval.abstract.fn.comp = NULL; yyval.abstract.fn.last = NULL;
			  if (yyvsp[0].abstract.fn.comp) { yyval.abstract.last = yyvsp[0].abstract.fn.last; *yyvsp[0].abstract.last = yyvsp[0].abstract.fn.comp; }
			  *yyval.abstract.last = yyvsp[-1].nested.comp;
			  yyval.abstract.last = yyvsp[-1].nested.last; }
    break;

  case 130:
#line 811 "cp-name-parser.y"
    { yyval.abstract.fn.comp = NULL; yyval.abstract.fn.last = NULL;
			  if (yyvsp[0].abstract.fn.comp) { yyval.abstract.last = yyvsp[0].abstract.fn.last; *yyvsp[0].abstract.last = yyvsp[0].abstract.fn.comp; }
			}
    break;

  case 131:
#line 818 "cp-name-parser.y"
    { yyval.abstract = yyvsp[-1].abstract; yyval.abstract.fn.comp = NULL; yyval.abstract.fn.last = NULL; yyval.abstract.fold_flag = 1;
			  if (yyvsp[-1].abstract.fn.comp) { yyval.abstract.last = yyvsp[-1].abstract.fn.last; *yyvsp[-1].abstract.last = yyvsp[-1].abstract.fn.comp; }
			}
    break;

  case 132:
#line 822 "cp-name-parser.y"
    { yyval.abstract.fold_flag = 0;
			  if (yyvsp[-1].abstract.fn.comp) { yyval.abstract.last = yyvsp[-1].abstract.fn.last; *yyvsp[-1].abstract.last = yyvsp[-1].abstract.fn.comp; }
			  if (yyvsp[-1].abstract.fold_flag)
			    {
			      *yyval.abstract.last = yyvsp[0].nested.comp;
			      yyval.abstract.last = yyvsp[0].nested.last;
			    }
			  else
			    yyval.abstract.fn = yyvsp[0].nested;
			}
    break;

  case 133:
#line 833 "cp-name-parser.y"
    { yyval.abstract.fn.comp = NULL; yyval.abstract.fn.last = NULL; yyval.abstract.fold_flag = 0;
			  if (yyvsp[-1].abstract.fn.comp) { yyval.abstract.last = yyvsp[-1].abstract.fn.last; *yyvsp[-1].abstract.last = yyvsp[-1].abstract.fn.comp; }
			  *yyvsp[-1].abstract.last = yyvsp[0].comp;
			  yyval.abstract.last = &d_right (yyvsp[0].comp);
			}
    break;

  case 134:
#line 839 "cp-name-parser.y"
    { yyval.abstract.fn.comp = NULL; yyval.abstract.fn.last = NULL; yyval.abstract.fold_flag = 0;
			  yyval.abstract.comp = yyvsp[0].comp;
			  yyval.abstract.last = &d_right (yyvsp[0].comp);
			}
    break;

  case 135:
#line 857 "cp-name-parser.y"
    { yyval.abstract.comp = yyvsp[0].nested.comp; yyval.abstract.last = yyvsp[0].nested.last;
			  yyval.abstract.fn.comp = NULL; yyval.abstract.fn.last = NULL; yyval.abstract.start = NULL; }
    break;

  case 136:
#line 860 "cp-name-parser.y"
    { yyval.abstract = yyvsp[0].abstract;
			  if (yyvsp[0].abstract.last)
			    *yyval.abstract.last = yyvsp[-1].nested.comp;
			  else
			    yyval.abstract.comp = yyvsp[-1].nested.comp;
			  yyval.abstract.last = yyvsp[-1].nested.last;
			}
    break;

  case 137:
#line 868 "cp-name-parser.y"
    { yyval.abstract.comp = yyvsp[0].abstract.comp; yyval.abstract.last = yyvsp[0].abstract.last; yyval.abstract.fn = yyvsp[0].abstract.fn; yyval.abstract.start = NULL; }
    break;

  case 138:
#line 870 "cp-name-parser.y"
    { yyval.abstract.start = yyvsp[0].comp;
			  if (yyvsp[-3].abstract.fn.comp) { yyval.abstract.last = yyvsp[-3].abstract.fn.last; *yyvsp[-3].abstract.last = yyvsp[-3].abstract.fn.comp; }
			  if (yyvsp[-3].abstract.fold_flag)
			    {
			      *yyval.abstract.last = yyvsp[-2].nested.comp;
			      yyval.abstract.last = yyvsp[-2].nested.last;
			    }
			  else
			    yyval.abstract.fn = yyvsp[-2].nested;
			}
    break;

  case 139:
#line 881 "cp-name-parser.y"
    { yyval.abstract.fn = yyvsp[-1].nested;
			  yyval.abstract.start = yyvsp[0].comp;
			  yyval.abstract.comp = NULL; yyval.abstract.last = NULL;
			}
    break;

  case 141:
#line 889 "cp-name-parser.y"
    { yyval.comp = yyvsp[0].abstract.comp;
			  *yyvsp[0].abstract.last = yyvsp[-1].comp;
			}
    break;

  case 142:
#line 895 "cp-name-parser.y"
    { yyval.nested.comp = yyvsp[0].nested.comp;
			  yyval.nested.last = yyvsp[-1].nested.last;
			  *yyvsp[0].nested.last = yyvsp[-1].nested.comp; }
    break;

  case 144:
#line 903 "cp-name-parser.y"
    { yyval.nested = yyvsp[-1].nested; }
    break;

  case 145:
#line 905 "cp-name-parser.y"
    { yyval.nested.comp = yyvsp[-1].nested.comp;
			  *yyvsp[-1].nested.last = yyvsp[0].nested.comp;
			  yyval.nested.last = yyvsp[0].nested.last;
			}
    break;

  case 146:
#line 910 "cp-name-parser.y"
    { yyval.nested.comp = yyvsp[-1].nested.comp;
			  *yyvsp[-1].nested.last = yyvsp[0].comp;
			  yyval.nested.last = &d_right (yyvsp[0].comp);
			}
    break;

  case 147:
#line 915 "cp-name-parser.y"
    { yyval.nested.comp = make_empty (DEMANGLE_COMPONENT_TYPED_NAME);
			  d_left (yyval.nested.comp) = yyvsp[0].comp;
			  yyval.nested.last = &d_right (yyval.nested.comp);
			}
    break;

  case 148:
#line 928 "cp-name-parser.y"
    { yyval.nested.comp = yyvsp[0].nested.comp;
			  yyval.nested.last = yyvsp[-1].nested.last;
			  *yyvsp[0].nested.last = yyvsp[-1].nested.comp; }
    break;

  case 149:
#line 932 "cp-name-parser.y"
    { yyval.nested.comp = make_empty (DEMANGLE_COMPONENT_TYPED_NAME);
			  d_left (yyval.nested.comp) = yyvsp[0].comp;
			  yyval.nested.last = &d_right (yyval.nested.comp);
			}
    break;

  case 151:
#line 945 "cp-name-parser.y"
    { yyval.nested.comp = fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, yyvsp[-3].comp, yyvsp[-2].nested.comp);
			  yyval.nested.last = yyvsp[-2].nested.last;
			  yyval.nested.comp = fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME, yyval.nested.comp, yyvsp[0].comp);
			}
    break;

  case 152:
#line 950 "cp-name-parser.y"
    { yyval.nested.comp = yyvsp[-3].nested.comp;
			  *yyvsp[-3].nested.last = yyvsp[-2].nested.comp;
			  yyval.nested.last = yyvsp[-2].nested.last;
			  yyval.nested.comp = fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME, yyval.nested.comp, yyvsp[0].comp);
			}
    break;

  case 153:
#line 959 "cp-name-parser.y"
    { yyval.nested.comp = yyvsp[-1].nested.comp;
			  yyval.nested.last = yyvsp[-2].nested.last;
			  *yyvsp[-1].nested.last = yyvsp[-2].nested.comp; }
    break;

  case 154:
#line 963 "cp-name-parser.y"
    { yyval.nested.comp = yyvsp[-1].nested.comp;
			  *yyvsp[-1].nested.last = yyvsp[0].nested.comp;
			  yyval.nested.last = yyvsp[0].nested.last;
			}
    break;

  case 155:
#line 968 "cp-name-parser.y"
    { yyval.nested.comp = yyvsp[-1].nested.comp;
			  *yyvsp[-1].nested.last = yyvsp[0].comp;
			  yyval.nested.last = &d_right (yyvsp[0].comp);
			}
    break;

  case 156:
#line 973 "cp-name-parser.y"
    { yyval.nested.comp = fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, yyvsp[-1].comp, yyvsp[0].nested.comp);
			  yyval.nested.last = yyvsp[0].nested.last;
			}
    break;

  case 157:
#line 977 "cp-name-parser.y"
    { yyval.nested.comp = fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, yyvsp[-1].comp, yyvsp[0].comp);
			  yyval.nested.last = &d_right (yyvsp[0].comp);
			}
    break;

  case 158:
#line 983 "cp-name-parser.y"
    { yyval.comp = yyvsp[-1].comp; }
    break;

  case 160:
#line 992 "cp-name-parser.y"
    { yyval.comp = d_binary (">", yyvsp[-2].comp, yyvsp[0].comp); }
    break;

  case 161:
#line 999 "cp-name-parser.y"
    { yyval.comp = fill_comp (DEMANGLE_COMPONENT_UNARY, make_operator ("&", 1), yyvsp[0].comp); }
    break;

  case 162:
#line 1001 "cp-name-parser.y"
    { yyval.comp = fill_comp (DEMANGLE_COMPONENT_UNARY, make_operator ("&", 1), yyvsp[-1].comp); }
    break;

  case 163:
#line 1006 "cp-name-parser.y"
    { yyval.comp = d_unary ("-", yyvsp[0].comp); }
    break;

  case 164:
#line 1010 "cp-name-parser.y"
    { yyval.comp = d_unary ("!", yyvsp[0].comp); }
    break;

  case 165:
#line 1014 "cp-name-parser.y"
    { yyval.comp = d_unary ("~", yyvsp[0].comp); }
    break;

  case 166:
#line 1021 "cp-name-parser.y"
    { if (yyvsp[0].comp->type == DEMANGLE_COMPONENT_LITERAL
		      || yyvsp[0].comp->type == DEMANGLE_COMPONENT_LITERAL_NEG)
		    {
		      yyval.comp = yyvsp[0].comp;
		      d_left (yyvsp[0].comp) = yyvsp[-2].comp;
		    }
		  else
		    yyval.comp = fill_comp (DEMANGLE_COMPONENT_UNARY,
				      fill_comp (DEMANGLE_COMPONENT_CAST, yyvsp[-2].comp, NULL),
				      yyvsp[0].comp);
		}
    break;

  case 167:
#line 1037 "cp-name-parser.y"
    { yyval.comp = fill_comp (DEMANGLE_COMPONENT_UNARY,
				    fill_comp (DEMANGLE_COMPONENT_CAST, yyvsp[-4].comp, NULL),
				    yyvsp[-1].comp);
		}
    break;

  case 168:
#line 1044 "cp-name-parser.y"
    { yyval.comp = fill_comp (DEMANGLE_COMPONENT_UNARY,
				    fill_comp (DEMANGLE_COMPONENT_CAST, yyvsp[-4].comp, NULL),
				    yyvsp[-1].comp);
		}
    break;

  case 169:
#line 1051 "cp-name-parser.y"
    { yyval.comp = fill_comp (DEMANGLE_COMPONENT_UNARY,
				    fill_comp (DEMANGLE_COMPONENT_CAST, yyvsp[-4].comp, NULL),
				    yyvsp[-1].comp);
		}
    break;

  case 170:
#line 1070 "cp-name-parser.y"
    { yyval.comp = d_binary ("*", yyvsp[-2].comp, yyvsp[0].comp); }
    break;

  case 171:
#line 1074 "cp-name-parser.y"
    { yyval.comp = d_binary ("/", yyvsp[-2].comp, yyvsp[0].comp); }
    break;

  case 172:
#line 1078 "cp-name-parser.y"
    { yyval.comp = d_binary ("%", yyvsp[-2].comp, yyvsp[0].comp); }
    break;

  case 173:
#line 1082 "cp-name-parser.y"
    { yyval.comp = d_binary ("+", yyvsp[-2].comp, yyvsp[0].comp); }
    break;

  case 174:
#line 1086 "cp-name-parser.y"
    { yyval.comp = d_binary ("-", yyvsp[-2].comp, yyvsp[0].comp); }
    break;

  case 175:
#line 1090 "cp-name-parser.y"
    { yyval.comp = d_binary ("<<", yyvsp[-2].comp, yyvsp[0].comp); }
    break;

  case 176:
#line 1094 "cp-name-parser.y"
    { yyval.comp = d_binary (">>", yyvsp[-2].comp, yyvsp[0].comp); }
    break;

  case 177:
#line 1098 "cp-name-parser.y"
    { yyval.comp = d_binary ("==", yyvsp[-2].comp, yyvsp[0].comp); }
    break;

  case 178:
#line 1102 "cp-name-parser.y"
    { yyval.comp = d_binary ("!=", yyvsp[-2].comp, yyvsp[0].comp); }
    break;

  case 179:
#line 1106 "cp-name-parser.y"
    { yyval.comp = d_binary ("<=", yyvsp[-2].comp, yyvsp[0].comp); }
    break;

  case 180:
#line 1110 "cp-name-parser.y"
    { yyval.comp = d_binary (">=", yyvsp[-2].comp, yyvsp[0].comp); }
    break;

  case 181:
#line 1114 "cp-name-parser.y"
    { yyval.comp = d_binary ("<", yyvsp[-2].comp, yyvsp[0].comp); }
    break;

  case 182:
#line 1118 "cp-name-parser.y"
    { yyval.comp = d_binary ("&", yyvsp[-2].comp, yyvsp[0].comp); }
    break;

  case 183:
#line 1122 "cp-name-parser.y"
    { yyval.comp = d_binary ("^", yyvsp[-2].comp, yyvsp[0].comp); }
    break;

  case 184:
#line 1126 "cp-name-parser.y"
    { yyval.comp = d_binary ("|", yyvsp[-2].comp, yyvsp[0].comp); }
    break;

  case 185:
#line 1130 "cp-name-parser.y"
    { yyval.comp = d_binary ("&&", yyvsp[-2].comp, yyvsp[0].comp); }
    break;

  case 186:
#line 1134 "cp-name-parser.y"
    { yyval.comp = d_binary ("||", yyvsp[-2].comp, yyvsp[0].comp); }
    break;

  case 187:
#line 1139 "cp-name-parser.y"
    { yyval.comp = d_binary ("->", yyvsp[-2].comp, yyvsp[0].comp); }
    break;

  case 188:
#line 1143 "cp-name-parser.y"
    { yyval.comp = d_binary (".", yyvsp[-2].comp, yyvsp[0].comp); }
    break;

  case 189:
#line 1147 "cp-name-parser.y"
    { yyval.comp = fill_comp (DEMANGLE_COMPONENT_TRINARY, make_operator ("?", 3),
				    fill_comp (DEMANGLE_COMPONENT_TRINARY_ARG1, yyvsp[-4].comp,
						 fill_comp (DEMANGLE_COMPONENT_TRINARY_ARG2, yyvsp[-2].comp, yyvsp[0].comp)));
		}
    break;

  case 192:
#line 1161 "cp-name-parser.y"
    { yyval.comp = d_unary ("sizeof", yyvsp[-1].comp); }
    break;

  case 193:
#line 1166 "cp-name-parser.y"
    { struct demangle_component *i;
		  i = make_name ("1", 1);
		  yyval.comp = fill_comp (DEMANGLE_COMPONENT_LITERAL,
				    make_builtin_type ("bool"),
				    i);
		}
    break;

  case 194:
#line 1175 "cp-name-parser.y"
    { struct demangle_component *i;
		  i = make_name ("0", 1);
		  yyval.comp = fill_comp (DEMANGLE_COMPONENT_LITERAL,
				    make_builtin_type ("bool"),
				    i);
		}
    break;


    }

/* Line 1000 of yacc.c.  */
#line 2834 "cp-name-parser.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  const char* yyprefix;
	  char *yymsg;
	  int yyx;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 0;

	  yyprefix = ", expecting ";
	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		yysize += yystrlen (yyprefix) + yystrlen (yytname [yyx]);
		yycount += 1;
		if (yycount == 5)
		  {
		    yysize = 0;
		    break;
		  }
	      }
	  yysize += (sizeof ("syntax error, unexpected ")
		     + yystrlen (yytname[yytype]));
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yyprefix = ", expecting ";
		  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			yyp = yystpcpy (yyp, yyprefix);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yyprefix = " or ";
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* If at end of input, pop the error token,
	     then the rest of the stack, then return failure.  */
	  if (yychar == YYEOF)
	     for (;;)
	       {
		 YYPOPSTACK;
		 if (yyssp == yyss)
		   YYABORT;
		 YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
		 yydestruct (yystos[*yyssp], yyvsp);
	       }
        }
      else
	{
	  YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
	  yydestruct (yytoken, &yylval);
	  yychar = YYEMPTY;

	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

#ifdef __GNUC__
  /* Pacify GCC when the user code never invokes YYERROR and the label
     yyerrorlab therefore never appears in user code.  */
  if (0)
     goto yyerrorlab;
#endif

  yyvsp -= yylen;
  yyssp -= yylen;
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


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

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 1185 "cp-name-parser.y"


/* Apply QUALIFIERS to LHS and return a qualified component.  IS_METHOD
   is set if LHS is a method, in which case the qualifiers are logically
   applied to "this".  We apply qualifiers in a consistent order; LHS
   may already be qualified; duplicate qualifiers are not created.  */

struct demangle_component *
d_qualify (struct demangle_component *lhs, int qualifiers, int is_method)
{
  struct demangle_component **inner_p;
  enum demangle_component_type type;

  /* For now the order is CONST (innermost), VOLATILE, RESTRICT.  */

#define HANDLE_QUAL(TYPE, MTYPE, QUAL)				\
  if ((qualifiers & QUAL) && (type != TYPE) && (type != MTYPE))	\
    {								\
      *inner_p = fill_comp (is_method ? MTYPE : TYPE,	\
			      *inner_p, NULL);			\
      inner_p = &d_left (*inner_p);				\
      type = (*inner_p)->type;					\
    }								\
  else if (type == TYPE || type == MTYPE)			\
    {								\
      inner_p = &d_left (*inner_p);				\
      type = (*inner_p)->type;					\
    }

  inner_p = &lhs;

  type = (*inner_p)->type;

  HANDLE_QUAL (DEMANGLE_COMPONENT_RESTRICT, DEMANGLE_COMPONENT_RESTRICT_THIS, QUAL_RESTRICT);
  HANDLE_QUAL (DEMANGLE_COMPONENT_VOLATILE, DEMANGLE_COMPONENT_VOLATILE_THIS, QUAL_VOLATILE);
  HANDLE_QUAL (DEMANGLE_COMPONENT_CONST, DEMANGLE_COMPONENT_CONST_THIS, QUAL_CONST);

  return lhs;
}

/* Return a builtin type corresponding to FLAGS.  */

static struct demangle_component *
d_int_type (int flags)
{
  const char *name;

  switch (flags)
    {
    case INT_SIGNED | INT_CHAR:
      name = "signed char";
      break;
    case INT_CHAR:
      name = "char";
      break;
    case INT_UNSIGNED | INT_CHAR:
      name = "unsigned char";
      break;
    case 0:
    case INT_SIGNED:
      name = "int";
      break;
    case INT_UNSIGNED:
      name = "unsigned int";
      break;
    case INT_LONG:
    case INT_SIGNED | INT_LONG:
      name = "long";
      break;
    case INT_UNSIGNED | INT_LONG:
      name = "unsigned long";
      break;
    case INT_SHORT:
    case INT_SIGNED | INT_SHORT:
      name = "short";
      break;
    case INT_UNSIGNED | INT_SHORT:
      name = "unsigned short";
      break;
    case INT_LLONG | INT_LONG:
    case INT_SIGNED | INT_LLONG | INT_LONG:
      name = "long long";
      break;
    case INT_UNSIGNED | INT_LLONG | INT_LONG:
      name = "unsigned long long";
      break;
    default:
      return NULL;
    }

  return make_builtin_type (name);
}

/* Wrapper to create a unary operation.  */

static struct demangle_component *
d_unary (const char *name, struct demangle_component *lhs)
{
  return fill_comp (DEMANGLE_COMPONENT_UNARY, make_operator (name, 1), lhs);
}

/* Wrapper to create a binary operation.  */

static struct demangle_component *
d_binary (const char *name, struct demangle_component *lhs, struct demangle_component *rhs)
{
  return fill_comp (DEMANGLE_COMPONENT_BINARY, make_operator (name, 2),
		      fill_comp (DEMANGLE_COMPONENT_BINARY_ARGS, lhs, rhs));
}

/* Find the end of a symbol name starting at LEXPTR.  */

static const char *
symbol_end (const char *lexptr)
{
  const char *p = lexptr;

  while (*p && (ISALNUM (*p) || *p == '_' || *p == '$' || *p == '.'))
    p++;

  return p;
}

/* Take care of parsing a number (anything that starts with a digit).
   The number starts at P and contains LEN characters.  Store the result in
   YYLVAL.  */

static int
parse_number (const char *p, int len, int parsed_float)
{
  int unsigned_p = 0;

  /* Number of "L" suffixes encountered.  */
  int long_p = 0;

  struct demangle_component *signed_type;
  struct demangle_component *unsigned_type;
  struct demangle_component *type, *name;
  enum demangle_component_type literal_type;

  if (p[0] == '-')
    {
      literal_type = DEMANGLE_COMPONENT_LITERAL_NEG;
      p++;
      len--;
    }
  else
    literal_type = DEMANGLE_COMPONENT_LITERAL;

  if (parsed_float)
    {
      /* It's a float since it contains a point or an exponent.  */
      char c;

      /* The GDB lexer checks the result of scanf at this point.  Not doing
         this leaves our error checking slightly weaker but only for invalid
         data.  */

      /* See if it has `f' or `l' suffix (float or long double).  */

      c = TOLOWER (p[len - 1]);

      if (c == 'f')
      	{
      	  len--;
      	  type = make_builtin_type ("float");
      	}
      else if (c == 'l')
	{
	  len--;
	  type = make_builtin_type ("long double");
	}
      else if (ISDIGIT (c) || c == '.')
	type = make_builtin_type ("double");
      else
	return ERROR;

      name = make_name (p, len);
      yylval.comp = fill_comp (literal_type, type, name);

      return FLOAT;
    }

  /* This treats 0x1 and 1 as different literals.  We also do not
     automatically generate unsigned types.  */

  long_p = 0;
  unsigned_p = 0;
  while (len > 0)
    {
      if (p[len - 1] == 'l' || p[len - 1] == 'L')
	{
	  len--;
	  long_p++;
	  continue;
	}
      if (p[len - 1] == 'u' || p[len - 1] == 'U')
	{
	  len--;
	  unsigned_p++;
	  continue;
	}
      break;
    }

  if (long_p == 0)
    {
      unsigned_type = make_builtin_type ("unsigned int");
      signed_type = make_builtin_type ("int");
    }
  else if (long_p == 1)
    {
      unsigned_type = make_builtin_type ("unsigned long");
      signed_type = make_builtin_type ("long");
    }
  else
    {
      unsigned_type = make_builtin_type ("unsigned long long");
      signed_type = make_builtin_type ("long long");
    }

   if (unsigned_p)
     type = unsigned_type;
   else
     type = signed_type;

   name = make_name (p, len);
   yylval.comp = fill_comp (literal_type, type, name);

   return INT;
}

static char backslashable[] = "abefnrtv";
static char represented[] = "\a\b\e\f\n\r\t\v";

/* Translate the backslash the way we would in the host character set.  */
static int
c_parse_backslash (int host_char, int *target_char)
{
  const char *ix;
  ix = strchr (backslashable, host_char);
  if (! ix)
    return 0;
  else
    *target_char = represented[ix - backslashable];
  return 1;
}

/* Parse a C escape sequence.  STRING_PTR points to a variable
   containing a pointer to the string to parse.  That pointer
   should point to the character after the \.  That pointer
   is updated past the characters we use.  The value of the
   escape sequence is returned.

   A negative value means the sequence \ newline was seen,
   which is supposed to be equivalent to nothing at all.

   If \ is followed by a null character, we return a negative
   value and leave the string pointer pointing at the null character.

   If \ is followed by 000, we return 0 and leave the string pointer
   after the zeros.  A value of 0 does not mean end of string.  */

static int
cp_parse_escape (const char **string_ptr)
{
  int target_char;
  int c = *(*string_ptr)++;
  if (c_parse_backslash (c, &target_char))
    return target_char;
  else
    switch (c)
      {
      case '\n':
	return -2;
      case 0:
	(*string_ptr)--;
	return 0;
      case '^':
	{
	  c = *(*string_ptr)++;

	  if (c == '?')
	    return 0177;
	  else if (c == '\\')
	    target_char = cp_parse_escape (string_ptr);
	  else
	    target_char = c;

	  /* Now target_char is something like `c', and we want to find
	     its control-character equivalent.  */
	  target_char = target_char & 037;

	  return target_char;
	}

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
	{
	  int i = c - '0';
	  int count = 0;
	  while (++count < 3)
	    {
	      c = (**string_ptr);
	      if (c >= '0' && c <= '7')
		{
		  (*string_ptr)++;
		  i *= 8;
		  i += c - '0';
		}
	      else
		{
		  break;
		}
	    }
	  return i;
	}
      default:
	return c;
      }
}

#define HANDLE_SPECIAL(string, comp)				\
  if (strncmp (tokstart, string, sizeof (string) - 1) == 0)	\
    {								\
      lexptr = tokstart + sizeof (string) - 1;			\
      yylval.lval = comp;					\
      return DEMANGLER_SPECIAL;					\
    }

#define HANDLE_TOKEN2(string, token)			\
  if (lexptr[1] == string[1])				\
    {							\
      lexptr += 2;					\
      yylval.opname = string;				\
      return token;					\
    }      

#define HANDLE_TOKEN3(string, token)			\
  if (lexptr[1] == string[1] && lexptr[2] == string[2])	\
    {							\
      lexptr += 3;					\
      yylval.opname = string;				\
      return token;					\
    }      

/* Read one token, getting characters through LEXPTR.  */

static int
yylex (void)
{
  int c;
  int namelen;
  const char *tokstart;

 retry:
  prev_lexptr = lexptr;
  tokstart = lexptr;

  switch (c = *tokstart)
    {
    case 0:
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
	c = cp_parse_escape (&lexptr);
      else if (c == '\'')
	{
	  yyerror (_("empty character constant"));
	  return ERROR;
	}

      c = *lexptr++;
      if (c != '\'')
	{
	  yyerror (_("invalid character constant"));
	  return ERROR;
	}

      /* FIXME: We should refer to a canonical form of the character,
	 presumably the same one that appears in manglings - the decimal
	 representation.  But if that isn't in our input then we have to
	 allocate memory for it somewhere.  */
      yylval.comp = fill_comp (DEMANGLE_COMPONENT_LITERAL,
				 make_builtin_type ("char"),
				 make_name (tokstart, lexptr - tokstart));

      return INT;

    case '(':
      if (strncmp (tokstart, "(anonymous namespace)", 21) == 0)
	{
	  lexptr += 21;
	  yylval.comp = make_name ("(anonymous namespace)",
				     sizeof "(anonymous namespace)" - 1);
	  return NAME;
	}
	/* FALL THROUGH */

    case ')':
    case ',':
      lexptr++;
      return c;

    case '.':
      if (lexptr[1] == '.' && lexptr[2] == '.')
	{
	  lexptr += 3;
	  return ELLIPSIS;
	}

      /* Might be a floating point number.  */
      if (lexptr[1] < '0' || lexptr[1] > '9')
	goto symbol;		/* Nope, must be a symbol. */

      goto try_number;

    case '-':
      HANDLE_TOKEN2 ("-=", ASSIGN_MODIFY);
      HANDLE_TOKEN2 ("--", DECREMENT);
      HANDLE_TOKEN2 ("->", ARROW);

      /* For construction vtables.  This is kind of hokey.  */
      if (strncmp (tokstart, "-in-", 4) == 0)
	{
	  lexptr += 4;
	  return CONSTRUCTION_IN;
	}

      if (lexptr[1] < '0' || lexptr[1] > '9')
	{
	  lexptr++;
	  return '-';
	}
      /* FALL THRU into number case.  */

    try_number:
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
	const char *p = tokstart;
	int hex = 0;

	if (c == '-')
	  p++;

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
	       a decimal floating point number regardless of the radix.

	       NOTE drow/2005-03-09: This comment is not accurate in C99;
	       however, it's not clear that all the floating point support
	       in this file is doing any good here.  */
	    else if (!got_dot && *p == '.')
	      got_dot = 1;
	    else if (got_e && (p[-1] == 'e' || p[-1] == 'E')
		     && (*p == '-' || *p == '+'))
	      /* This is the sign of the exponent, not the end of the
		 number.  */
	      continue;
	    /* We will take any letters or digits.  parse_number will
	       complain if past the radix, or if L or U are not final.  */
	    else if (! ISALNUM (*p))
	      break;
	  }
	toktype = parse_number (tokstart, p - tokstart, got_dot|got_e);
        if (toktype == ERROR)
	  {
	    char *err_copy = (char *) alloca (p - tokstart + 1);

	    memcpy (err_copy, tokstart, p - tokstart);
	    err_copy[p - tokstart] = 0;
	    yyerror (_("invalid number"));
	    return ERROR;
	  }
	lexptr = p;
	return toktype;
      }

    case '+':
      HANDLE_TOKEN2 ("+=", ASSIGN_MODIFY);
      HANDLE_TOKEN2 ("++", INCREMENT);
      lexptr++;
      return c;
    case '*':
      HANDLE_TOKEN2 ("*=", ASSIGN_MODIFY);
      lexptr++;
      return c;
    case '/':
      HANDLE_TOKEN2 ("/=", ASSIGN_MODIFY);
      lexptr++;
      return c;
    case '%':
      HANDLE_TOKEN2 ("%=", ASSIGN_MODIFY);
      lexptr++;
      return c;
    case '|':
      HANDLE_TOKEN2 ("|=", ASSIGN_MODIFY);
      HANDLE_TOKEN2 ("||", OROR);
      lexptr++;
      return c;
    case '&':
      HANDLE_TOKEN2 ("&=", ASSIGN_MODIFY);
      HANDLE_TOKEN2 ("&&", ANDAND);
      lexptr++;
      return c;
    case '^':
      HANDLE_TOKEN2 ("^=", ASSIGN_MODIFY);
      lexptr++;
      return c;
    case '!':
      HANDLE_TOKEN2 ("!=", NOTEQUAL);
      lexptr++;
      return c;
    case '<':
      HANDLE_TOKEN3 ("<<=", ASSIGN_MODIFY);
      HANDLE_TOKEN2 ("<=", LEQ);
      HANDLE_TOKEN2 ("<<", LSH);
      lexptr++;
      return c;
    case '>':
      HANDLE_TOKEN3 (">>=", ASSIGN_MODIFY);
      HANDLE_TOKEN2 (">=", GEQ);
      HANDLE_TOKEN2 (">>", RSH);
      lexptr++;
      return c;
    case '=':
      HANDLE_TOKEN2 ("==", EQUAL);
      lexptr++;
      return c;
    case ':':
      HANDLE_TOKEN2 ("::", COLONCOLON);
      lexptr++;
      return c;

    case '[':
    case ']':
    case '?':
    case '@':
    case '~':
    case '{':
    case '}':
    symbol:
      lexptr++;
      return c;

    case '"':
      /* These can't occur in C++ names.  */
      yyerror (_("unexpected string literal"));
      return ERROR;
    }

  if (!(c == '_' || c == '$' || ISALPHA (c)))
    {
      /* We must have come across a bad character (e.g. ';').  */
      yyerror (_("invalid character"));
      return ERROR;
    }

  /* It's a name.  See how long it is.  */
  namelen = 0;
  do
    c = tokstart[++namelen];
  while (ISALNUM (c) || c == '_' || c == '$');

  lexptr += namelen;

  /* Catch specific keywords.  Notice that some of the keywords contain
     spaces, and are sorted by the length of the first word.  They must
     all include a trailing space in the string comparison.  */
  switch (namelen)
    {
    case 16:
      if (strncmp (tokstart, "reinterpret_cast", 16) == 0)
        return REINTERPRET_CAST;
      break;
    case 12:
      if (strncmp (tokstart, "construction vtable for ", 24) == 0)
	{
	  lexptr = tokstart + 24;
	  return CONSTRUCTION_VTABLE;
	}
      if (strncmp (tokstart, "dynamic_cast", 12) == 0)
        return DYNAMIC_CAST;
      break;
    case 11:
      if (strncmp (tokstart, "static_cast", 11) == 0)
        return STATIC_CAST;
      break;
    case 9:
      HANDLE_SPECIAL ("covariant return thunk to ", DEMANGLE_COMPONENT_COVARIANT_THUNK);
      HANDLE_SPECIAL ("reference temporary for ", DEMANGLE_COMPONENT_REFTEMP);
      break;
    case 8:
      HANDLE_SPECIAL ("typeinfo for ", DEMANGLE_COMPONENT_TYPEINFO);
      HANDLE_SPECIAL ("typeinfo fn for ", DEMANGLE_COMPONENT_TYPEINFO_FN);
      HANDLE_SPECIAL ("typeinfo name for ", DEMANGLE_COMPONENT_TYPEINFO_NAME);
      if (strncmp (tokstart, "operator", 8) == 0)
	return OPERATOR;
      if (strncmp (tokstart, "restrict", 8) == 0)
	return RESTRICT;
      if (strncmp (tokstart, "unsigned", 8) == 0)
	return UNSIGNED;
      if (strncmp (tokstart, "template", 8) == 0)
	return TEMPLATE;
      if (strncmp (tokstart, "volatile", 8) == 0)
	return VOLATILE_KEYWORD;
      break;
    case 7:
      HANDLE_SPECIAL ("virtual thunk to ", DEMANGLE_COMPONENT_VIRTUAL_THUNK);
      if (strncmp (tokstart, "wchar_t", 7) == 0)
	return WCHAR_T;
      break;
    case 6:
      if (strncmp (tokstart, "global constructors keyed to ", 29) == 0)
	{
	  const char *p;
	  lexptr = tokstart + 29;
	  yylval.lval = DEMANGLE_COMPONENT_GLOBAL_CONSTRUCTORS;
	  /* Find the end of the symbol.  */
	  p = symbol_end (lexptr);
	  yylval.comp = make_name (lexptr, p - lexptr);
	  lexptr = p;
	  return DEMANGLER_SPECIAL;
	}
      if (strncmp (tokstart, "global destructors keyed to ", 28) == 0)
	{
	  const char *p;
	  lexptr = tokstart + 28;
	  yylval.lval = DEMANGLE_COMPONENT_GLOBAL_DESTRUCTORS;
	  /* Find the end of the symbol.  */
	  p = symbol_end (lexptr);
	  yylval.comp = make_name (lexptr, p - lexptr);
	  lexptr = p;
	  return DEMANGLER_SPECIAL;
	}

      HANDLE_SPECIAL ("vtable for ", DEMANGLE_COMPONENT_VTABLE);
      if (strncmp (tokstart, "delete", 6) == 0)
	return DELETE;
      if (strncmp (tokstart, "struct", 6) == 0)
	return STRUCT;
      if (strncmp (tokstart, "signed", 6) == 0)
	return SIGNED_KEYWORD;
      if (strncmp (tokstart, "sizeof", 6) == 0)
	return SIZEOF;
      if (strncmp (tokstart, "double", 6) == 0)
	return DOUBLE_KEYWORD;
      break;
    case 5:
      HANDLE_SPECIAL ("guard variable for ", DEMANGLE_COMPONENT_GUARD);
      if (strncmp (tokstart, "false", 5) == 0)
	return FALSEKEYWORD;
      if (strncmp (tokstart, "class", 5) == 0)
	return CLASS;
      if (strncmp (tokstart, "union", 5) == 0)
	return UNION;
      if (strncmp (tokstart, "float", 5) == 0)
	return FLOAT_KEYWORD;
      if (strncmp (tokstart, "short", 5) == 0)
	return SHORT;
      if (strncmp (tokstart, "const", 5) == 0)
	return CONST_KEYWORD;
      break;
    case 4:
      if (strncmp (tokstart, "void", 4) == 0)
	return VOID;
      if (strncmp (tokstart, "bool", 4) == 0)
	return BOOL;
      if (strncmp (tokstart, "char", 4) == 0)
	return CHAR;
      if (strncmp (tokstart, "enum", 4) == 0)
	return ENUM;
      if (strncmp (tokstart, "long", 4) == 0)
	return LONG;
      if (strncmp (tokstart, "true", 4) == 0)
	return TRUEKEYWORD;
      break;
    case 3:
      HANDLE_SPECIAL ("VTT for ", DEMANGLE_COMPONENT_VTT);
      HANDLE_SPECIAL ("non-virtual thunk to ", DEMANGLE_COMPONENT_THUNK);
      if (strncmp (tokstart, "new", 3) == 0)
	return NEW;
      if (strncmp (tokstart, "int", 3) == 0)
	return INT_KEYWORD;
      break;
    default:
      break;
    }

  yylval.comp = make_name (tokstart, namelen);
  return NAME;
}

static void
yyerror (char *msg)
{
  if (global_errmsg)
    return;

  error_lexptr = prev_lexptr;
  global_errmsg = msg ? msg : "parse error";
}

/* Allocate a chunk of the components we'll need to build a tree.  We
   generally allocate too many components, but the extra memory usage
   doesn't hurt because the trees are temporary and the storage is
   reused.  More may be allocated later, by d_grab.  */
static void
allocate_info (void)
{
  if (demangle_info == NULL)
    {
      demangle_info = xmalloc (sizeof (struct demangle_info));
      demangle_info->prev = NULL;
      demangle_info->next = NULL;
    }
  else
    while (demangle_info->prev)
      demangle_info = demangle_info->prev;

  demangle_info->used = 0;
}

/* Convert RESULT to a string.  The return value is allocated
   using xmalloc.  ESTIMATED_LEN is used only as a guide to the
   length of the result.  This functions handles a few cases that
   cplus_demangle_print does not, specifically the global destructor
   and constructor labels.  */

char *
cp_comp_to_string (struct demangle_component *result, int estimated_len)
{
  size_t err;

  return cplus_demangle_print (DMGL_PARAMS | DMGL_ANSI, result, estimated_len,
			       &err);
}

/* Convert a demangled name to a demangle_component tree.  On success,
   the root of the new tree is returned; it is valid until the next
   call to this function and should not be freed.  On error, NULL is
   returned, and an error message will be set in *ERRMSG (which does
   not need to be freed).  */

struct demangle_component *
cp_demangled_name_to_comp (const char *demangled_name, const char **errmsg)
{
  static char errbuf[60];
  struct demangle_component *result;

  prev_lexptr = lexptr = demangled_name;
  error_lexptr = NULL;
  global_errmsg = NULL;

  allocate_info ();

  if (yyparse ())
    {
      if (global_errmsg && errmsg)
	{
	  snprintf (errbuf, sizeof (errbuf) - 2, "%s, near `%s",
		    global_errmsg, error_lexptr);
	  strcat (errbuf, "'");
	  *errmsg = errbuf;
	}
      return NULL;
    }

  result = global_result;
  global_result = NULL;

  return result;
}

#ifdef TEST_CPNAMES

static void
cp_print (struct demangle_component *result)
{
  char *str;
  size_t err = 0;

  str = cplus_demangle_print (DMGL_PARAMS | DMGL_ANSI, result, 64, &err);
  if (str == NULL)
    return;

  fputs (str, stdout);

  xfree (str);
}

static char
trim_chars (char *lexptr, char **extra_chars)
{
  char *p = (char *) symbol_end (lexptr);
  char c = 0;

  if (*p)
    {
      c = *p;
      *p = 0;
      *extra_chars = p + 1;
    }

  return c;
}

/* When this file is built as a standalone program, xmalloc comes from
   libiberty --- in which case we have to provide xfree ourselves.  */

void
xfree (void *ptr)
{
  if (ptr != NULL)
    xfree (ptr);
}

int
main (int argc, char **argv)
{
  char *str2, *extra_chars = "", c;
  char buf[65536];
  int arg;
  const char *errmsg;
  struct demangle_component *result;

  arg = 1;
  if (argv[arg] && strcmp (argv[arg], "--debug") == 0)
    {
      yydebug = 1;
      arg++;
    }

  if (argv[arg] == NULL)
    while (fgets (buf, 65536, stdin) != NULL)
      {
	int len;
	buf[strlen (buf) - 1] = 0;
	/* Use DMGL_VERBOSE to get expanded standard substitutions.  */
	c = trim_chars (buf, &extra_chars);
	str2 = cplus_demangle (buf, DMGL_PARAMS | DMGL_ANSI | DMGL_VERBOSE);
	if (str2 == NULL)
	  {
	    /* printf ("Demangling error\n"); */
	    if (c)
	      printf ("%s%c%s\n", buf, c, extra_chars);
	    else
	      printf ("%s\n", buf);
	    continue;
	  }
	result = cp_demangled_name_to_comp (str2, &errmsg);
	if (result == NULL)
	  {
	    fputs (errmsg, stderr);
	    fputc ('\n', stderr);
	    continue;
	  }

	cp_print (result);

	xfree (str2);
	if (c)
	  {
	    putchar (c);
	    fputs (extra_chars, stdout);
	  }
	putchar ('\n');
      }
  else
    {
      result = cp_demangled_name_to_comp (argv[arg], &errmsg);
      if (result == NULL)
	{
	  fputs (errmsg, stderr);
	  fputc ('\n', stderr);
	  return 0;
	}
      cp_print (result);
      putchar ('\n');
    }
  return 0;
}

#endif


