/* A Bison parser, made by GNU Bison 2.1.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

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

/* Bison version.  */
#define YYBISON_VERSION "2.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Using locations.  */
#define YYLSP_NEEDED 1



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     TOK_IDENT = 258,
     TOK_STRING_LITERAL = 259,
     TOK_INT_LITERAL = 260,
     TOK_SECTION_NAME = 261,
     TOK_SOURCE_NAME = 262,
     TOK_BLOB = 263,
     TOK_DOT_DOT = 264,
     TOK_AND = 265,
     TOK_OR = 266,
     TOK_GEQ = 267,
     TOK_LEQ = 268,
     TOK_EQ = 269,
     TOK_NEQ = 270,
     TOK_POWER = 271,
     TOK_LSHIFT = 272,
     TOK_RSHIFT = 273,
     TOK_INT_SIZE = 274,
     TOK_OPTIONS = 275,
     TOK_CONSTANTS = 276,
     TOK_SOURCES = 277,
     TOK_FILTERS = 278,
     TOK_SECTION = 279,
     TOK_EXTERN = 280,
     TOK_FROM = 281,
     TOK_RAW = 282,
     TOK_LOAD = 283,
     TOK_JUMP = 284,
     TOK_CALL = 285,
     TOK_MODE = 286,
     TOK_IF = 287,
     TOK_ELSE = 288,
     TOK_DEFINED = 289,
     TOK_INFO = 290,
     TOK_WARNING = 291,
     TOK_ERROR = 292,
     TOK_SIZEOF = 293,
     TOK_DCD = 294,
     TOK_HAB = 295,
     TOK_IVT = 296,
     UNARY_OP = 297
   };
#endif
/* Tokens.  */
#define TOK_IDENT 258
#define TOK_STRING_LITERAL 259
#define TOK_INT_LITERAL 260
#define TOK_SECTION_NAME 261
#define TOK_SOURCE_NAME 262
#define TOK_BLOB 263
#define TOK_DOT_DOT 264
#define TOK_AND 265
#define TOK_OR 266
#define TOK_GEQ 267
#define TOK_LEQ 268
#define TOK_EQ 269
#define TOK_NEQ 270
#define TOK_POWER 271
#define TOK_LSHIFT 272
#define TOK_RSHIFT 273
#define TOK_INT_SIZE 274
#define TOK_OPTIONS 275
#define TOK_CONSTANTS 276
#define TOK_SOURCES 277
#define TOK_FILTERS 278
#define TOK_SECTION 279
#define TOK_EXTERN 280
#define TOK_FROM 281
#define TOK_RAW 282
#define TOK_LOAD 283
#define TOK_JUMP 284
#define TOK_CALL 285
#define TOK_MODE 286
#define TOK_IF 287
#define TOK_ELSE 288
#define TOK_DEFINED 289
#define TOK_INFO 290
#define TOK_WARNING 291
#define TOK_ERROR 292
#define TOK_SIZEOF 293
#define TOK_DCD 294
#define TOK_HAB 295
#define TOK_IVT 296
#define UNARY_OP 297




/* Copy the first part of user declarations.  */

#include "ElftosbLexer.h"
#include "ElftosbAST.h"
#include "Logging.h"
#include "Blob.h"
#include "format_string.h"
#include "Value.h"
#include "ConversionController.h"

using namespace elftosb;

//! Our special location type.
#define YYLTYPE token_loc_t

// this indicates that we're using our own type. it should be unset automatically
// but that's not working for some reason with the .hpp file.
#if defined(YYLTYPE_IS_TRIVIAL)
	#undef YYLTYPE_IS_TRIVIAL
	#define YYLTYPE_IS_TRIVIAL 0
#endif

//! Default location action
#define YYLLOC_DEFAULT(Current, Rhs, N)	\
	do {		\
		if (N)	\
		{		\
			(Current).m_firstLine = YYRHSLOC(Rhs, 1).m_firstLine;	\
			(Current).m_lastLine = YYRHSLOC(Rhs, N).m_lastLine;		\
		}		\
		else	\
		{		\
			(Current).m_firstLine = (Current).m_lastLine = YYRHSLOC(Rhs, 0).m_lastLine;	\
		}		\
	} while (0)

//! Forward declaration of yylex().
static int yylex(YYSTYPE * lvalp, YYLTYPE * yylloc, ElftosbLexer * lexer);

// Forward declaration of error handling function.
static void yyerror(YYLTYPE * yylloc, ElftosbLexer * lexer, CommandFileASTNode ** resultAST, const char * error);



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
typedef union YYSTYPE {
	int m_num;
	elftosb::SizedIntegerValue * m_int;
	Blob * m_blob;
	std::string * m_str;
	elftosb::ASTNode * m_ast;	// must use full name here because this is put into *.tab.hpp
} YYSTYPE;
/* Line 196 of yacc.c.  */
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

#if ! defined (YYLTYPE) && ! defined (YYLTYPE_IS_DECLARED)
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T) && (defined (__STDC__) || defined (__cplusplus))
# include <stddef.h> /* INFRINGES ON USER NAME SPACE */
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if defined (__STDC__) || defined (__cplusplus)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     define YYINCLUDED_STDLIB_H
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2005 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM ((YYSIZE_T) -1)
#  endif
#  ifdef __cplusplus
extern "C" {
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if (! defined (malloc) && ! defined (YYINCLUDED_STDLIB_H) \
	&& (defined (__STDC__) || defined (__cplusplus)))
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if (! defined (free) && ! defined (YYINCLUDED_STDLIB_H) \
	&& (defined (__STDC__) || defined (__cplusplus)))
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifdef __cplusplus
}
#  endif
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYLTYPE_IS_TRIVIAL) && YYLTYPE_IS_TRIVIAL \
             && defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short int yyss;
  YYSTYPE yyvs;
    YYLTYPE yyls;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short int) + sizeof (YYSTYPE) + sizeof (YYLTYPE))	\
      + 2 * YYSTACK_GAP_MAXIMUM)

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
	  YYSIZE_T yyi;				\
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
   typedef short int yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  13
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   418

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  66
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  52
/* YYNRULES -- Number of rules. */
#define YYNRULES  133
/* YYNRULES -- Number of states. */
#define YYNSTATES  238

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   297

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    26,     2,     2,     2,    64,    23,     2,
       9,    10,    62,    60,    16,    61,    20,    63,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    18,    17,
      25,    15,    19,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    13,     2,    14,    59,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    11,    24,    12,    22,     2,     2,     2,
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
       5,     6,     7,     8,    21,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    65
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
{
       0,     0,     3,     6,     8,    11,    13,    15,    17,    22,
      27,    29,    32,    35,    36,    40,    45,    47,    50,    54,
      55,    59,    66,    70,    71,    73,    77,    81,    83,    86,
      93,    96,    97,    99,   100,   104,   108,   110,   113,   116,
     118,   120,   121,   123,   126,   129,   131,   132,   134,   136,
     138,   140,   145,   147,   148,   150,   152,   154,   156,   160,
     165,   167,   169,   171,   175,   177,   180,   183,   184,   186,
     188,   193,   195,   196,   200,   205,   207,   209,   211,   213,
     217,   220,   221,   227,   230,   233,   236,   239,   246,   251,
     254,   255,   257,   261,   263,   265,   267,   271,   275,   279,
     283,   287,   291,   295,   299,   302,   307,   311,   316,   318,
     322,   325,   327,   329,   331,   335,   339,   343,   347,   351,
     355,   359,   363,   367,   371,   375,   377,   381,   385,   390,
     395,   400,   403,   406
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      67,     0,    -1,    68,    82,    -1,    69,    -1,    68,    69,
      -1,    70,    -1,    71,    -1,    75,    -1,    37,    11,    72,
      12,    -1,    38,    11,    72,    12,    -1,    73,    -1,    72,
      73,    -1,    74,    17,    -1,    -1,     3,    15,   111,    -1,
      39,    11,    76,    12,    -1,    77,    -1,    76,    77,    -1,
      78,    79,    17,    -1,    -1,     3,    15,     4,    -1,     3,
      15,    42,     9,   113,    10,    -1,     9,    80,    10,    -1,
      -1,    81,    -1,    80,    16,    81,    -1,     3,    15,   111,
      -1,    83,    -1,    82,    83,    -1,    41,     9,   113,    84,
      10,    86,    -1,    17,    85,    -1,    -1,    80,    -1,    -1,
      30,    94,    17,    -1,    11,    87,    12,    -1,    88,    -1,
      87,    88,    -1,    91,    17,    -1,   105,    -1,   108,    -1,
      -1,    90,    -1,    89,    90,    -1,    91,    17,    -1,   108,
      -1,    -1,    92,    -1,   101,    -1,   106,    -1,   107,    -1,
      45,    93,    94,    97,    -1,    56,    -1,    -1,   113,    -1,
       4,    -1,     7,    -1,    95,    -1,    95,    43,     7,    -1,
       7,    13,    95,    14,    -1,     8,    -1,    99,    -1,    96,
      -1,    95,    16,    96,    -1,     6,    -1,    22,     6,    -1,
      19,    98,    -1,    -1,    20,    -1,   110,    -1,    58,     9,
     100,    10,    -1,    80,    -1,    -1,   102,   103,   104,    -1,
      57,   102,   110,   104,    -1,    47,    -1,    46,    -1,     7,
      -1,   113,    -1,     9,   113,    10,    -1,     9,    10,    -1,
      -1,    43,     7,    11,    89,    12,    -1,    48,   113,    -1,
      52,     4,    -1,    53,     4,    -1,    54,     4,    -1,    49,
     112,    11,    87,    12,   109,    -1,    50,    11,    87,    12,
      -1,    50,   108,    -1,    -1,   113,    -1,   113,    21,   113,
      -1,   112,    -1,     4,    -1,   113,    -1,   112,    25,   112,
      -1,   112,    19,   112,    -1,   112,    29,   112,    -1,   112,
      30,   112,    -1,   112,    31,   112,    -1,   112,    32,   112,
      -1,   112,    27,   112,    -1,   112,    28,   112,    -1,    26,
     112,    -1,     3,     9,     7,    10,    -1,     9,   112,    10,
      -1,    51,     9,     3,    10,    -1,   115,    -1,     7,    18,
       3,    -1,    18,     3,    -1,   117,    -1,     3,    -1,   114,
      -1,   115,    60,   115,    -1,   115,    61,   115,    -1,   115,
      62,   115,    -1,   115,    63,   115,    -1,   115,    64,   115,
      -1,   115,    33,   115,    -1,   115,    23,   115,    -1,   115,
      24,   115,    -1,   115,    59,   115,    -1,   115,    34,   115,
      -1,   115,    35,   115,    -1,   116,    -1,   115,    20,    36,
      -1,     9,   115,    10,    -1,    55,     9,   114,    10,    -1,
      55,     9,     3,    10,    -1,    55,     9,     7,    10,    -1,
      60,   115,    -1,    61,   115,    -1,     5,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   162,   162,   172,   178,   186,   187,   188,   191,   197,
     203,   209,   216,   217,   220,   227,   233,   239,   247,   259,
     262,   267,   275,   276,   280,   286,   294,   301,   307,   314,
     329,   334,   340,   345,   351,   357,   365,   371,   379,   380,
     381,   382,   385,   391,   399,   400,   401,   404,   405,   406,
     407,   410,   433,   443,   445,   449,   454,   459,   464,   469,
     474,   479,   484,   490,   498,   503,   510,   515,   521,   526,
     532,   544,   545,   548,   577,   614,   615,   618,   623,   630,
     631,   632,   635,   642,   649,   654,   659,   666,   677,   681,
     688,   691,   696,   703,   707,   714,   718,   725,   732,   739,
     746,   753,   760,   767,   774,   779,   784,   789,   796,   799,
     804,   812,   816,   821,   832,   839,   846,   853,   860,   867,
     874,   881,   888,   895,   902,   909,   913,   918,   923,   928,
     933,   940,   944,   951
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "\"identifier\"", "\"string\"",
  "\"integer\"", "\"section name\"", "\"source name\"",
  "\"binary object\"", "'('", "')'", "'{'", "'}'", "'['", "']'", "'='",
  "','", "';'", "':'", "'>'", "'.'", "\"..\"", "'~'", "'&'", "'|'", "'<'",
  "'!'", "\"&&\"", "\"||\"", "\">=\"", "\"<=\"", "\"==\"", "\"!=\"",
  "\"**\"", "\"<<\"", "\">>\"", "\"integer size\"", "\"options\"",
  "\"constants\"", "\"sources\"", "\"filters\"", "\"section\"",
  "\"extern\"", "\"from\"", "\"raw\"", "\"load\"", "\"jump\"", "\"call\"",
  "\"mode\"", "\"if\"", "\"else\"", "\"defined\"", "\"info\"",
  "\"warning\"", "\"error\"", "\"sizeof\"", "\"dcd\"", "\"hab\"",
  "\"ivt\"", "'^'", "'+'", "'-'", "'*'", "'/'", "'%'", "UNARY_OP",
  "$accept", "command_file", "blocks_list", "pre_section_block",
  "options_block", "constants_block", "const_def_list",
  "const_def_list_elem", "const_def", "sources_block", "source_def_list",
  "source_def_list_elem", "source_def", "source_attrs_opt",
  "source_attr_list", "source_attr_list_elem", "section_defs",
  "section_def", "section_options_opt", "source_attr_list_opt",
  "section_contents", "full_stmt_list", "full_stmt_list_elem",
  "basic_stmt_list", "basic_stmt_list_elem", "basic_stmt", "load_stmt",
  "dcd_opt", "load_data", "section_list", "section_list_elem",
  "load_target_opt", "load_target", "ivt_def", "assignment_list_opt",
  "call_stmt", "call_or_jump", "call_target", "call_arg_opt", "from_stmt",
  "mode_stmt", "message_stmt", "if_stmt", "else_opt", "address_or_range",
  "const_expr", "bool_expr", "int_const_expr", "symbol_ref", "expr",
  "unary_expr", "int_value", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short int yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,    40,
      41,   123,   125,    91,    93,    61,    44,    59,    58,    62,
      46,   264,   126,    38,   124,    60,    33,   265,   266,   267,
     268,   269,   270,   271,   272,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,   291,   292,   293,   294,   295,   296,    94,
      43,    45,    42,    47,    37,   297
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    66,    67,    68,    68,    69,    69,    69,    70,    71,
      72,    72,    73,    73,    74,    75,    76,    76,    77,    77,
      78,    78,    79,    79,    80,    80,    81,    82,    82,    83,
      84,    84,    85,    85,    86,    86,    87,    87,    88,    88,
      88,    88,    89,    89,    90,    90,    90,    91,    91,    91,
      91,    92,    93,    93,    94,    94,    94,    94,    94,    94,
      94,    94,    95,    95,    96,    96,    97,    97,    98,    98,
      99,   100,   100,   101,   101,   102,   102,   103,   103,   104,
     104,   104,   105,   106,   107,   107,   107,   108,   109,   109,
     109,   110,   110,   111,   111,   112,   112,   112,   112,   112,
     112,   112,   112,   112,   112,   112,   112,   112,   113,   114,
     114,   115,   115,   115,   115,   115,   115,   115,   115,   115,
     115,   115,   115,   115,   115,   115,   115,   115,   115,   115,
     115,   116,   116,   117
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     2,     1,     2,     1,     1,     1,     4,     4,
       1,     2,     2,     0,     3,     4,     1,     2,     3,     0,
       3,     6,     3,     0,     1,     3,     3,     1,     2,     6,
       2,     0,     1,     0,     3,     3,     1,     2,     2,     1,
       1,     0,     1,     2,     2,     1,     0,     1,     1,     1,
       1,     4,     1,     0,     1,     1,     1,     1,     3,     4,
       1,     1,     1,     3,     1,     2,     2,     0,     1,     1,
       4,     1,     0,     3,     4,     1,     1,     1,     1,     3,
       2,     0,     5,     2,     2,     2,     2,     6,     4,     2,
       0,     1,     3,     1,     1,     1,     3,     3,     3,     3,
       3,     3,     3,     3,     2,     4,     3,     4,     1,     3,
       2,     1,     1,     1,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     1,     3,     3,     4,     4,
       4,     2,     2,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,     0,     0,     0,     0,     0,     3,     5,     6,     7,
      13,    13,    19,     1,     0,     4,     2,    27,     0,     0,
      10,     0,     0,     0,     0,    16,    23,     0,    28,     0,
       8,    11,    12,     9,     0,    15,    17,     0,     0,   112,
     133,     0,     0,     0,     0,     0,     0,    31,   113,   108,
     125,   111,   112,    94,     0,     0,     0,    14,    93,    95,
      20,     0,     0,     0,    24,    18,     0,     0,   110,     0,
     131,   132,    33,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   108,   104,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    22,     0,   109,   127,     0,     0,     0,    32,    30,
       0,   126,   120,   121,   119,   123,   124,   122,   114,   115,
     116,   117,   118,     0,   106,     0,    97,    96,   102,   103,
      98,    99,   100,   101,     0,    26,    25,   129,   130,   128,
      41,     0,    29,   105,   107,    21,     0,    53,    76,    75,
       0,     0,     0,     0,     0,     0,     0,    36,     0,    47,
      48,     0,    39,    49,    50,    40,    55,    64,    56,    60,
       0,     0,     0,    57,    62,    61,    54,     0,    52,     0,
      83,     0,    84,    85,    86,     0,    35,    37,    38,    77,
      81,    78,     0,    65,    72,    34,     0,     0,    46,    67,
      41,    81,    91,     0,    73,     0,    71,     0,    63,    58,
       0,    42,     0,    45,     0,    51,     0,    74,     0,    80,
       0,    59,    70,    82,    43,    44,    68,    66,    69,    90,
      92,    79,     0,    87,    41,    89,     0,    88
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,     4,     5,     6,     7,     8,    19,    20,    21,     9,
      24,    25,    26,    38,    63,    64,    16,    17,    73,   109,
     142,   156,   157,   210,   211,   158,   159,   179,   172,   173,
     174,   215,   227,   175,   207,   160,   161,   190,   204,   162,
     163,   164,   165,   233,   201,    57,    58,    59,    48,    49,
      50,    51
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -181
static const short int yypact[] =
{
     128,    17,    25,    48,    69,   123,  -181,  -181,  -181,  -181,
      96,    96,   101,  -181,    80,  -181,    68,  -181,   112,    85,
    -181,   115,    89,   114,    91,  -181,   124,    30,  -181,    47,
    -181,  -181,  -181,  -181,    11,  -181,  -181,   134,   125,  -181,
    -181,   133,    30,   140,   144,    30,    30,   153,  -181,   225,
    -181,  -181,   148,  -181,    61,    61,   162,  -181,   359,  -181,
    -181,   164,   159,    22,  -181,  -181,   172,   121,  -181,     9,
    -181,  -181,   134,   168,   143,    30,    30,    30,    30,    30,
      30,    30,    30,    30,    30,    30,   179,   303,   121,  -181,
     194,    61,    61,    61,    61,    61,    61,    61,    61,    30,
      47,  -181,   134,  -181,  -181,   188,     4,   200,   199,  -181,
      56,  -181,   241,   231,   236,    86,    86,   247,    76,    76,
     196,   196,   196,   208,  -181,   210,  -181,  -181,   373,   373,
    -181,  -181,  -181,  -181,   216,  -181,  -181,  -181,  -181,  -181,
     314,     2,  -181,  -181,  -181,  -181,   220,   175,  -181,  -181,
      30,    61,   228,   230,   237,    28,   147,  -181,   223,  -181,
    -181,   108,  -181,  -181,  -181,  -181,  -181,  -181,    92,  -181,
     240,   243,   233,    15,  -181,  -181,  -181,   242,  -181,     2,
    -181,   345,  -181,  -181,  -181,    30,  -181,  -181,  -181,   133,
     246,  -181,     7,  -181,   134,  -181,     7,   250,   361,   244,
     314,   246,   248,    16,  -181,   104,   199,   252,  -181,  -181,
     190,  -181,   251,  -181,    75,  -181,   160,  -181,    30,  -181,
     261,  -181,  -181,  -181,  -181,  -181,  -181,  -181,  -181,   222,
    -181,  -181,     6,  -181,   314,  -181,   176,  -181
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -181,  -181,  -181,   268,  -181,  -181,   266,   106,  -181,  -181,
    -181,   254,  -181,  -181,   -70,   177,  -181,   267,  -181,  -181,
    -181,  -151,  -155,  -181,   107,  -180,  -181,  -181,   127,   122,
     129,  -181,  -181,  -181,  -181,  -181,   163,  -181,   118,  -181,
    -181,  -181,   -21,  -181,   109,   221,   -51,   -27,   257,   270,
    -181,  -181
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
      47,   187,   108,    87,    89,    39,   166,    40,   167,   168,
     169,    42,   105,   167,   138,    60,   106,   234,   212,    39,
      43,    40,    66,    41,   170,    42,   219,    43,    10,   170,
     212,   196,   101,    39,    43,    40,    11,    41,   102,    42,
     126,   127,   128,   129,   130,   131,   132,   133,    43,   216,
      52,    53,    40,    61,    41,   151,    54,    44,   197,    12,
     171,   187,    45,    46,    52,    43,    40,   140,    41,    13,
      54,    44,   134,    55,   148,   149,    45,    46,    39,    43,
      40,   187,    41,   236,    42,    44,   141,    55,    18,    27,
      45,    46,    18,    43,    23,   226,    74,    30,    56,    18,
     181,    33,    44,    35,    23,   192,    74,    45,    46,    14,
      66,    39,    56,    40,   176,   189,    44,    42,   221,    77,
     196,    45,    46,   180,   206,    31,    43,    29,    31,    34,
      44,   104,    32,    37,   191,    45,    46,    62,    83,    84,
      85,    74,    65,    68,    75,    76,    81,    82,    83,    84,
      85,    66,   176,    69,    77,    78,    79,    86,   202,   186,
       1,     2,     3,    44,    14,     1,     2,     3,    45,    46,
      72,    90,   229,    99,   100,   103,   220,   213,   110,   111,
      80,    81,    82,    83,    84,    85,   123,   202,   237,   213,
     146,   230,   147,   148,   149,   150,   151,   125,   137,   152,
     153,   154,   223,   146,   155,   147,   148,   149,   150,   151,
     139,   235,   152,   153,   154,   102,    74,   155,   143,   146,
     144,   147,   148,   149,   150,   151,   145,   177,   152,   153,
     154,   178,   182,   155,   183,   147,   148,   149,   150,   151,
     188,   184,   152,   153,   154,    74,   193,   155,    75,    76,
     195,    74,   194,   198,    75,   203,    74,   209,    77,    78,
      79,    74,   222,   214,    77,    78,    79,    74,   225,   218,
      75,   231,   232,    15,    77,    78,    79,    22,    36,   136,
      77,    78,    79,    28,    80,    81,    82,    83,    84,    85,
      80,    81,    82,    83,    84,    85,    81,    82,    83,    84,
      85,    81,    82,    83,    84,    85,   199,    81,    82,    83,
      84,    85,    67,   124,   205,    70,    71,   224,   185,   217,
       0,   135,    91,   228,    88,   208,   107,     0,    92,     0,
      93,    94,    95,    96,    97,    98,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   200,   146,     0,   147,
     148,   149,   150,   151,    91,     0,   152,   153,   154,     0,
      92,   155,    93,    94,    95,    96,    97,    98,    91,     0,
       0,     0,     0,     0,    92,     0,    93,    94,    95,    96,
      97,    98,    91,     0,     0,     0,     0,     0,    92,     0,
       0,     0,    95,    96,    97,    98,   147,   148,   149,   150,
     151,     0,     0,   152,   153,   154,     0,     0,   155
};

static const short int yycheck[] =
{
      27,   156,    72,    54,    55,     3,     4,     5,     6,     7,
       8,     9,     3,     6,    10,     4,     7,    11,   198,     3,
      18,     5,    18,     7,    22,     9,    10,    18,    11,    22,
     210,    16,    10,     3,    18,     5,    11,     7,    16,     9,
      91,    92,    93,    94,    95,    96,    97,    98,    18,   200,
       3,     4,     5,    42,     7,    49,     9,    55,    43,    11,
      58,   216,    60,    61,     3,    18,     5,    11,     7,     0,
       9,    55,    99,    26,    46,    47,    60,    61,     3,    18,
       5,   236,     7,   234,     9,    55,    30,    26,     3,     9,
      60,    61,     3,    18,     3,    20,    20,    12,    51,     3,
     151,    12,    55,    12,     3,    13,    20,    60,    61,    41,
      18,     3,    51,     5,   141,     7,    55,     9,    14,    33,
      16,    60,    61,   150,   194,    19,    18,    15,    22,    15,
      55,    10,    17,     9,   161,    60,    61,     3,    62,    63,
      64,    20,    17,     3,    23,    24,    60,    61,    62,    63,
      64,    18,   179,     9,    33,    34,    35,     9,   185,    12,
      37,    38,    39,    55,    41,    37,    38,    39,    60,    61,
      17,     9,    12,     9,    15,     3,   203,   198,    10,    36,
      59,    60,    61,    62,    63,    64,     7,   214,    12,   210,
      43,   218,    45,    46,    47,    48,    49,     3,    10,    52,
      53,    54,    12,    43,    57,    45,    46,    47,    48,    49,
      10,   232,    52,    53,    54,    16,    20,    57,    10,    43,
      10,    45,    46,    47,    48,    49,    10,     7,    52,    53,
      54,    56,     4,    57,     4,    45,    46,    47,    48,    49,
      17,     4,    52,    53,    54,    20,     6,    57,    23,    24,
      17,    20,     9,    11,    23,     9,    20,     7,    33,    34,
      35,    20,    10,    19,    33,    34,    35,    20,    17,    21,
      23,    10,    50,     5,    33,    34,    35,    11,    24,   102,
      33,    34,    35,    16,    59,    60,    61,    62,    63,    64,
      59,    60,    61,    62,    63,    64,    60,    61,    62,    63,
      64,    60,    61,    62,    63,    64,   179,    60,    61,    62,
      63,    64,    42,    10,   192,    45,    46,   210,   155,   201,
      -1,   100,    19,   214,    54,   196,    69,    -1,    25,    -1,
      27,    28,    29,    30,    31,    32,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    11,    43,    -1,    45,
      46,    47,    48,    49,    19,    -1,    52,    53,    54,    -1,
      25,    57,    27,    28,    29,    30,    31,    32,    19,    -1,
      -1,    -1,    -1,    -1,    25,    -1,    27,    28,    29,    30,
      31,    32,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,
      -1,    -1,    29,    30,    31,    32,    45,    46,    47,    48,
      49,    -1,    -1,    52,    53,    54,    -1,    -1,    57
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    37,    38,    39,    67,    68,    69,    70,    71,    75,
      11,    11,    11,     0,    41,    69,    82,    83,     3,    72,
      73,    74,    72,     3,    76,    77,    78,     9,    83,    15,
      12,    73,    17,    12,    15,    12,    77,     9,    79,     3,
       5,     7,     9,    18,    55,    60,    61,   113,   114,   115,
     116,   117,     3,     4,     9,    26,    51,   111,   112,   113,
       4,    42,     3,    80,    81,    17,    18,   115,     3,     9,
     115,   115,    17,    84,    20,    23,    24,    33,    34,    35,
      59,    60,    61,    62,    63,    64,     9,   112,   115,   112,
       9,    19,    25,    27,    28,    29,    30,    31,    32,     9,
      15,    10,    16,     3,    10,     3,     7,   114,    80,    85,
      10,    36,   115,   115,   115,   115,   115,   115,   115,   115,
     115,   115,   115,     7,    10,     3,   112,   112,   112,   112,
     112,   112,   112,   112,   113,   111,    81,    10,    10,    10,
      11,    30,    86,    10,    10,    10,    43,    45,    46,    47,
      48,    49,    52,    53,    54,    57,    87,    88,    91,    92,
     101,   102,   105,   106,   107,   108,     4,     6,     7,     8,
      22,    58,    94,    95,    96,    99,   113,     7,    56,    93,
     113,   112,     4,     4,     4,   102,    12,    88,    17,     7,
     103,   113,    13,     6,     9,    17,    16,    43,    11,    94,
      11,   110,   113,     9,   104,    95,    80,   100,    96,     7,
      89,    90,    91,   108,    19,    97,    87,   104,    21,    10,
     113,    14,    10,    12,    90,    17,    20,    98,   110,    12,
     113,    10,    50,   109,    11,   108,    87,    12
};

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
    {								\
      yyerror (&yylloc, lexer, resultAST, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (0)


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (N)								\
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (0)
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
              (Loc).first_line, (Loc).first_column,	\
              (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, &yylloc, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, &yylloc, lexer)
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

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr,					\
                  Type, Value, Location);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short int *bottom, short int *top)
#else
static void
yy_stack_print (bottom, top)
    short int *bottom;
    short int *top;
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
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname[yyr1[yyrule]]);
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
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
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
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

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
  const char *yys = yystr;

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
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      size_t yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

#endif /* YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep, yylocationp)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;
  (void) yylocationp;

  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");

# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
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
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;
  (void) yylocationp;

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {
      case 3: /* "\"identifier\"" */
        { delete (yyvaluep->m_str); };
        break;
      case 4: /* "\"string\"" */
        { delete (yyvaluep->m_str); };
        break;
      case 5: /* "\"integer\"" */
        { delete (yyvaluep->m_int); };
        break;
      case 6: /* "\"section name\"" */
        { delete (yyvaluep->m_str); };
        break;
      case 7: /* "\"source name\"" */
        { delete (yyvaluep->m_str); };
        break;
      case 8: /* "\"binary object\"" */
        { delete (yyvaluep->m_blob); };
        break;
      case 36: /* "\"integer size\"" */
        { delete (yyvaluep->m_int); };
        break;

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
int yyparse (ElftosbLexer * lexer, CommandFileASTNode ** resultAST);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */






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
yyparse (ElftosbLexer * lexer, CommandFileASTNode ** resultAST)
#else
int
yyparse (lexer, resultAST)
    ElftosbLexer * lexer;
    CommandFileASTNode ** resultAST;
#endif
#endif
{
  /* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;
/* Location data for the look-ahead symbol.  */
YYLTYPE yylloc;

  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short int yyssa[YYINITDEPTH];
  short int *yyss = yyssa;
  short int *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;

  /* The location stack.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
  /* The locations where the error started and ended. */
  YYLTYPE yyerror_range[2];

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

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
  yylsp = yyls;
#if YYLTYPE_IS_TRIVIAL
  /* Initialize the default location before parsing starts.  */
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 0;
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

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short int *yyss1 = yyss;
	YYLTYPE *yyls1 = yyls;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);
	yyls = yyls1;
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short int *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);
	YYSTACK_RELOCATE (yyls);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

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
/* Read a look-ahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to look-ahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
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
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
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

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
  *++yylsp = yylloc;

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

  /* Default location. */
  YYLLOC_DEFAULT (yyloc, yylsp - yylen, yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
    {
							CommandFileASTNode * commandFile = new CommandFileASTNode();
							commandFile->setBlocks(dynamic_cast<ListASTNode*>((yyvsp[-1].m_ast)));
							commandFile->setSections(dynamic_cast<ListASTNode*>((yyvsp[0].m_ast)));
							commandFile->setLocation((yylsp[-1]), (yylsp[0]));
							*resultAST = commandFile;
						;}
    break;

  case 3:
    {
							ListASTNode * list = new ListASTNode();
							list->appendNode((yyvsp[0].m_ast));
							(yyval.m_ast) = list;
						;}
    break;

  case 4:
    {
							dynamic_cast<ListASTNode*>((yyvsp[-1].m_ast))->appendNode((yyvsp[0].m_ast));
							(yyval.m_ast) = (yyvsp[-1].m_ast);
						;}
    break;

  case 5:
    { (yyval.m_ast) = (yyvsp[0].m_ast); ;}
    break;

  case 6:
    { (yyval.m_ast) = (yyvsp[0].m_ast); ;}
    break;

  case 7:
    { (yyval.m_ast) = (yyvsp[0].m_ast); ;}
    break;

  case 8:
    {
								(yyval.m_ast) = new OptionsBlockASTNode(dynamic_cast<ListASTNode *>((yyvsp[-1].m_ast)));
							;}
    break;

  case 9:
    {
								(yyval.m_ast) = new ConstantsBlockASTNode(dynamic_cast<ListASTNode *>((yyvsp[-1].m_ast)));
							;}
    break;

  case 10:
    {
								ListASTNode * list = new ListASTNode();
								list->appendNode((yyvsp[0].m_ast));
								(yyval.m_ast) = list;
							;}
    break;

  case 11:
    {
								dynamic_cast<ListASTNode*>((yyvsp[-1].m_ast))->appendNode((yyvsp[0].m_ast));
								(yyval.m_ast) = (yyvsp[-1].m_ast);
							;}
    break;

  case 12:
    { (yyval.m_ast) = (yyvsp[-1].m_ast); ;}
    break;

  case 13:
    { (yyval.m_ast) = NULL; ;}
    break;

  case 14:
    {
								(yyval.m_ast) = new AssignmentASTNode((yyvsp[-2].m_str), (yyvsp[0].m_ast));
								(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
							;}
    break;

  case 15:
    {
							(yyval.m_ast) = new SourcesBlockASTNode(dynamic_cast<ListASTNode *>((yyvsp[-1].m_ast)));
						;}
    break;

  case 16:
    {
							ListASTNode * list = new ListASTNode();
							list->appendNode((yyvsp[0].m_ast));
							(yyval.m_ast) = list;
						;}
    break;

  case 17:
    {
							dynamic_cast<ListASTNode*>((yyvsp[-1].m_ast))->appendNode((yyvsp[0].m_ast));
							(yyval.m_ast) = (yyvsp[-1].m_ast);
						;}
    break;

  case 18:
    {
								// tell the lexer that this is the name of a source file
								SourceDefASTNode * node = dynamic_cast<SourceDefASTNode*>((yyvsp[-2].m_ast));
								if ((yyvsp[-1].m_ast))
								{
									node->setAttributes(dynamic_cast<ListASTNode*>((yyvsp[-1].m_ast)));
								}
								node->setLocation(node->getLocation(), (yylsp[0]));
								lexer->addSourceName(node->getName());
								(yyval.m_ast) = (yyvsp[-2].m_ast);
							;}
    break;

  case 19:
    { (yyval.m_ast) = NULL; ;}
    break;

  case 20:
    {
								(yyval.m_ast) = new PathSourceDefASTNode((yyvsp[-2].m_str), (yyvsp[0].m_str));
								(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
							;}
    break;

  case 21:
    {
								(yyval.m_ast) = new ExternSourceDefASTNode((yyvsp[-5].m_str), dynamic_cast<ExprASTNode*>((yyvsp[-1].m_ast)));
								(yyval.m_ast)->setLocation((yylsp[-5]), (yylsp[0]));
							;}
    break;

  case 22:
    { (yyval.m_ast) = (yyvsp[-1].m_ast); ;}
    break;

  case 23:
    { (yyval.m_ast) = NULL; ;}
    break;

  case 24:
    {
								ListASTNode * list = new ListASTNode();
								list->appendNode((yyvsp[0].m_ast));
								(yyval.m_ast) = list;
							;}
    break;

  case 25:
    {
								dynamic_cast<ListASTNode*>((yyvsp[-2].m_ast))->appendNode((yyvsp[0].m_ast));
								(yyval.m_ast) = (yyvsp[-2].m_ast);
							;}
    break;

  case 26:
    {
								(yyval.m_ast) = new AssignmentASTNode((yyvsp[-2].m_str), (yyvsp[0].m_ast));
								(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
							;}
    break;

  case 27:
    {
								ListASTNode * list = new ListASTNode();
								list->appendNode((yyvsp[0].m_ast));
								(yyval.m_ast) = list;
							;}
    break;

  case 28:
    {
								dynamic_cast<ListASTNode*>((yyvsp[-1].m_ast))->appendNode((yyvsp[0].m_ast));
								(yyval.m_ast) = (yyvsp[-1].m_ast);
							;}
    break;

  case 29:
    {
								SectionContentsASTNode * sectionNode = dynamic_cast<SectionContentsASTNode*>((yyvsp[0].m_ast));
								if (sectionNode)
								{
									ExprASTNode * exprNode = dynamic_cast<ExprASTNode*>((yyvsp[-3].m_ast));
									sectionNode->setSectionNumberExpr(exprNode);
									sectionNode->setOptions(dynamic_cast<ListASTNode*>((yyvsp[-2].m_ast)));
									sectionNode->setLocation((yylsp[-5]), sectionNode->getLocation());
								}
								(yyval.m_ast) = (yyvsp[0].m_ast);
							;}
    break;

  case 30:
    {
								(yyval.m_ast) = (yyvsp[0].m_ast);
							;}
    break;

  case 31:
    {
								(yyval.m_ast) = NULL;
							;}
    break;

  case 32:
    {
								(yyval.m_ast) = (yyvsp[0].m_ast);
							;}
    break;

  case 33:
    {
								(yyval.m_ast) = NULL;
							;}
    break;

  case 34:
    {
								DataSectionContentsASTNode * dataSection = new DataSectionContentsASTNode((yyvsp[-1].m_ast));
								dataSection->setLocation((yylsp[-2]), (yylsp[0]));
								(yyval.m_ast) = dataSection;
							;}
    break;

  case 35:
    {
								ListASTNode * listNode = dynamic_cast<ListASTNode*>((yyvsp[-1].m_ast));
								(yyval.m_ast) = new BootableSectionContentsASTNode(listNode);
								(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
							;}
    break;

  case 36:
    {
								ListASTNode * list = new ListASTNode();
								list->appendNode((yyvsp[0].m_ast));
								(yyval.m_ast) = list;
							;}
    break;

  case 37:
    {
								dynamic_cast<ListASTNode*>((yyvsp[-1].m_ast))->appendNode((yyvsp[0].m_ast));
								(yyval.m_ast) = (yyvsp[-1].m_ast);
							;}
    break;

  case 38:
    { (yyval.m_ast) = (yyvsp[-1].m_ast); ;}
    break;

  case 39:
    { (yyval.m_ast) = (yyvsp[0].m_ast); ;}
    break;

  case 40:
    { (yyval.m_ast) = (yyvsp[0].m_ast); ;}
    break;

  case 41:
    { (yyval.m_ast) = NULL; ;}
    break;

  case 42:
    {
								ListASTNode * list = new ListASTNode();
								list->appendNode((yyvsp[0].m_ast));
								(yyval.m_ast) = list;
							;}
    break;

  case 43:
    {
								dynamic_cast<ListASTNode*>((yyvsp[-1].m_ast))->appendNode((yyvsp[0].m_ast));
								(yyval.m_ast) = (yyvsp[-1].m_ast);
							;}
    break;

  case 44:
    { (yyval.m_ast) = (yyvsp[-1].m_ast); ;}
    break;

  case 45:
    { (yyval.m_ast) = (yyvsp[0].m_ast); ;}
    break;

  case 46:
    { (yyval.m_ast) = NULL; ;}
    break;

  case 47:
    { (yyval.m_ast) = (yyvsp[0].m_ast); ;}
    break;

  case 48:
    { (yyval.m_ast) = (yyvsp[0].m_ast); ;}
    break;

  case 49:
    { (yyval.m_ast) = (yyvsp[0].m_ast); ;}
    break;

  case 50:
    { (yyval.m_ast) = (yyvsp[0].m_ast); ;}
    break;

  case 51:
    {
								LoadStatementASTNode * stmt = new LoadStatementASTNode();
								stmt->setData((yyvsp[-1].m_ast));
								stmt->setTarget((yyvsp[0].m_ast));
								// set dcd load flag if the "dcd" keyword was present.
								if ((yyvsp[-2].m_num))
								{
									stmt->setDCDLoad(true);
								}
								// set char locations for the statement
								if ((yyvsp[0].m_ast))
								{
									stmt->setLocation((yylsp[-3]), (yylsp[0]));
								}
								else
								{
									stmt->setLocation((yylsp[-3]), (yylsp[-1]));
								}
								(yyval.m_ast) = stmt;
							;}
    break;

  case 52:
    {
								if (!elftosb::g_enableHABSupport)
								{
									yyerror(&yylloc, lexer, resultAST, "HAB features not supported with the selected family");
									YYABORT;
								}
								
								(yyval.m_num) = 1;
							;}
    break;

  case 53:
    { (yyval.m_num) = 0; ;}
    break;

  case 54:
    {
								(yyval.m_ast) = (yyvsp[0].m_ast);
							;}
    break;

  case 55:
    {
								(yyval.m_ast) = new StringConstASTNode((yyvsp[0].m_str));
								(yyval.m_ast)->setLocation((yylsp[0]));
							;}
    break;

  case 56:
    {
								(yyval.m_ast) = new SourceASTNode((yyvsp[0].m_str));
								(yyval.m_ast)->setLocation((yylsp[0]));
							;}
    break;

  case 57:
    {
								(yyval.m_ast) = new SectionMatchListASTNode(dynamic_cast<ListASTNode*>((yyvsp[0].m_ast)));
								(yyval.m_ast)->setLocation((yylsp[0]));
							;}
    break;

  case 58:
    {
								(yyval.m_ast) = new SectionMatchListASTNode(dynamic_cast<ListASTNode*>((yyvsp[-2].m_ast)), (yyvsp[0].m_str));
								(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
							;}
    break;

  case 59:
    {
								(yyval.m_ast) = new SectionMatchListASTNode(dynamic_cast<ListASTNode*>((yyvsp[-1].m_ast)), (yyvsp[-3].m_str));
								(yyval.m_ast)->setLocation((yylsp[-3]), (yylsp[0]));
							;}
    break;

  case 60:
    {
								(yyval.m_ast) = new BlobConstASTNode((yyvsp[0].m_blob));
								(yyval.m_ast)->setLocation((yylsp[0]));
							;}
    break;

  case 61:
    {
							;}
    break;

  case 62:
    {
								ListASTNode * list = new ListASTNode();
								list->appendNode((yyvsp[0].m_ast));
								(yyval.m_ast) = list;
							;}
    break;

  case 63:
    {
								dynamic_cast<ListASTNode*>((yyvsp[-2].m_ast))->appendNode((yyvsp[0].m_ast));
								(yyval.m_ast) = (yyvsp[-2].m_ast);
							;}
    break;

  case 64:
    {
								(yyval.m_ast) = new SectionASTNode((yyvsp[0].m_str), SectionASTNode::kInclude);
								(yyval.m_ast)->setLocation((yylsp[0]));
							;}
    break;

  case 65:
    {
								(yyval.m_ast) = new SectionASTNode((yyvsp[0].m_str), SectionASTNode::kExclude);
								(yyval.m_ast)->setLocation((yylsp[-1]), (yylsp[0]));
							;}
    break;

  case 66:
    {
								(yyval.m_ast) = (yyvsp[0].m_ast);
							;}
    break;

  case 67:
    {
								(yyval.m_ast) = new NaturalLocationASTNode();
//								$$->setLocation();
							;}
    break;

  case 68:
    {
								(yyval.m_ast) = new NaturalLocationASTNode();
								(yyval.m_ast)->setLocation((yylsp[0]));
							;}
    break;

  case 69:
    {
								(yyval.m_ast) = (yyvsp[0].m_ast);
							;}
    break;

  case 70:
    {
								IVTConstASTNode * ivt = new IVTConstASTNode();
								if ((yyvsp[-1].m_ast))
								{
									ivt->setFieldAssignments(dynamic_cast<ListASTNode*>((yyvsp[-1].m_ast)));
								}
								ivt->setLocation((yylsp[-3]), (yylsp[0]));
								(yyval.m_ast) = ivt;
							;}
    break;

  case 71:
    { (yyval.m_ast) = (yyvsp[0].m_ast); ;}
    break;

  case 72:
    { (yyval.m_ast) = NULL; ;}
    break;

  case 73:
    {
								CallStatementASTNode * stmt = new CallStatementASTNode();
								switch ((yyvsp[-2].m_num))
								{
									case 1:
										stmt->setCallType(CallStatementASTNode::kCallType);
										break;
									case 2:
										stmt->setCallType(CallStatementASTNode::kJumpType);
										break;
									default:
										yyerror(&yylloc, lexer, resultAST, "invalid call_or_jump value");
										YYABORT;
										break;
								}
								stmt->setTarget((yyvsp[-1].m_ast));
								stmt->setArgument((yyvsp[0].m_ast));
								stmt->setIsHAB(false);
								if ((yyvsp[0].m_ast))
								{
									stmt->setLocation((yylsp[-2]), (yylsp[0]));
								}
								else
								{
									stmt->setLocation((yylsp[-2]), (yylsp[-1]));
								}
								(yyval.m_ast) = stmt;
							;}
    break;

  case 74:
    {
								if (!elftosb::g_enableHABSupport)
								{
									yyerror(&yylloc, lexer, resultAST, "HAB features not supported with the selected family");
									YYABORT;
								}
								
								CallStatementASTNode * stmt = new CallStatementASTNode();
								switch ((yyvsp[-2].m_num))
								{
									case 1:
										stmt->setCallType(CallStatementASTNode::kCallType);
										break;
									case 2:
										stmt->setCallType(CallStatementASTNode::kJumpType);
										break;
									default:
										yyerror(&yylloc, lexer, resultAST, "invalid call_or_jump value");
										YYABORT;
										break;
								}
								stmt->setTarget((yyvsp[-1].m_ast));
								stmt->setArgument((yyvsp[0].m_ast));
								stmt->setIsHAB(true);
								if ((yyvsp[0].m_ast))
								{
									stmt->setLocation((yylsp[-3]), (yylsp[0]));
								}
								else
								{
									stmt->setLocation((yylsp[-3]), (yylsp[-1]));
								}
								(yyval.m_ast) = stmt;
							;}
    break;

  case 75:
    { (yyval.m_num) = 1; ;}
    break;

  case 76:
    { (yyval.m_num) = 2; ;}
    break;

  case 77:
    {
								(yyval.m_ast) = new SymbolASTNode(NULL, (yyvsp[0].m_str));
								(yyval.m_ast)->setLocation((yylsp[0]));
							;}
    break;

  case 78:
    {
								(yyval.m_ast) = new AddressRangeASTNode((yyvsp[0].m_ast), NULL);
								(yyval.m_ast)->setLocation((yyvsp[0].m_ast));
							;}
    break;

  case 79:
    { (yyval.m_ast) = (yyvsp[-1].m_ast); ;}
    break;

  case 80:
    { (yyval.m_ast) = NULL; ;}
    break;

  case 81:
    { (yyval.m_ast) = NULL; ;}
    break;

  case 82:
    {
								(yyval.m_ast) = new FromStatementASTNode((yyvsp[-3].m_str), dynamic_cast<ListASTNode*>((yyvsp[-1].m_ast)));
								(yyval.m_ast)->setLocation((yylsp[-4]), (yylsp[0]));
							;}
    break;

  case 83:
    {
								(yyval.m_ast) = new ModeStatementASTNode(dynamic_cast<ExprASTNode*>((yyvsp[0].m_ast)));
								(yyval.m_ast)->setLocation((yylsp[-1]), (yylsp[0]));
							;}
    break;

  case 84:
    {
								(yyval.m_ast) = new MessageStatementASTNode(MessageStatementASTNode::kInfo, (yyvsp[0].m_str));
								(yyval.m_ast)->setLocation((yylsp[-1]), (yylsp[0]));
							;}
    break;

  case 85:
    {
								(yyval.m_ast) = new MessageStatementASTNode(MessageStatementASTNode::kWarning, (yyvsp[0].m_str));
								(yyval.m_ast)->setLocation((yylsp[-1]), (yylsp[0]));
							;}
    break;

  case 86:
    {
								(yyval.m_ast) = new MessageStatementASTNode(MessageStatementASTNode::kError, (yyvsp[0].m_str));
								(yyval.m_ast)->setLocation((yylsp[-1]), (yylsp[0]));
							;}
    break;

  case 87:
    {
								IfStatementASTNode * ifStmt = new IfStatementASTNode();
								ifStmt->setConditionExpr(dynamic_cast<ExprASTNode*>((yyvsp[-4].m_ast)));
								ifStmt->setIfStatements(dynamic_cast<ListASTNode*>((yyvsp[-2].m_ast)));
								ifStmt->setElseStatements(dynamic_cast<ListASTNode*>((yyvsp[0].m_ast)));
								ifStmt->setLocation((yylsp[-5]), (yylsp[0]));
								(yyval.m_ast) = ifStmt;
							;}
    break;

  case 88:
    {
								(yyval.m_ast) = (yyvsp[-1].m_ast);
							;}
    break;

  case 89:
    {
								ListASTNode * list = new ListASTNode();
								list->appendNode((yyvsp[0].m_ast));
								(yyval.m_ast) = list;
								(yyval.m_ast)->setLocation((yylsp[-1]), (yylsp[0]));
							;}
    break;

  case 90:
    { (yyval.m_ast) = NULL; ;}
    break;

  case 91:
    {
								(yyval.m_ast) = new AddressRangeASTNode((yyvsp[0].m_ast), NULL);
								(yyval.m_ast)->setLocation((yyvsp[0].m_ast));
							;}
    break;

  case 92:
    {
								(yyval.m_ast) = new AddressRangeASTNode((yyvsp[-2].m_ast), (yyvsp[0].m_ast));
								(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
							;}
    break;

  case 93:
    {
								(yyval.m_ast) = (yyvsp[0].m_ast);
							;}
    break;

  case 94:
    {
								(yyval.m_ast) = new StringConstASTNode((yyvsp[0].m_str));
								(yyval.m_ast)->setLocation((yylsp[0]));
							;}
    break;

  case 95:
    {
							(yyval.m_ast) = (yyvsp[0].m_ast);
						;}
    break;

  case 96:
    {
							ExprASTNode * left = dynamic_cast<ExprASTNode*>((yyvsp[-2].m_ast));
							ExprASTNode * right = dynamic_cast<ExprASTNode*>((yyvsp[0].m_ast));
							(yyval.m_ast) = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kLessThan, right);
							(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
						;}
    break;

  case 97:
    {
							ExprASTNode * left = dynamic_cast<ExprASTNode*>((yyvsp[-2].m_ast));
							ExprASTNode * right = dynamic_cast<ExprASTNode*>((yyvsp[0].m_ast));
							(yyval.m_ast) = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kGreaterThan, right);
							(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
						;}
    break;

  case 98:
    {
							ExprASTNode * left = dynamic_cast<ExprASTNode*>((yyvsp[-2].m_ast));
							ExprASTNode * right = dynamic_cast<ExprASTNode*>((yyvsp[0].m_ast));
							(yyval.m_ast) = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kGreaterThanEqual, right);
							(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
						;}
    break;

  case 99:
    {
							ExprASTNode * left = dynamic_cast<ExprASTNode*>((yyvsp[-2].m_ast));
							ExprASTNode * right = dynamic_cast<ExprASTNode*>((yyvsp[0].m_ast));
							(yyval.m_ast) = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kLessThanEqual, right);
							(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
						;}
    break;

  case 100:
    {
							ExprASTNode * left = dynamic_cast<ExprASTNode*>((yyvsp[-2].m_ast));
							ExprASTNode * right = dynamic_cast<ExprASTNode*>((yyvsp[0].m_ast));
							(yyval.m_ast) = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kEqual, right);
							(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
						;}
    break;

  case 101:
    {
							ExprASTNode * left = dynamic_cast<ExprASTNode*>((yyvsp[-2].m_ast));
							ExprASTNode * right = dynamic_cast<ExprASTNode*>((yyvsp[0].m_ast));
							(yyval.m_ast) = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kNotEqual, right);
							(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
						;}
    break;

  case 102:
    {
							ExprASTNode * left = dynamic_cast<ExprASTNode*>((yyvsp[-2].m_ast));
							ExprASTNode * right = dynamic_cast<ExprASTNode*>((yyvsp[0].m_ast));
							(yyval.m_ast) = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kBooleanAnd, right);
							(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
						;}
    break;

  case 103:
    {
							ExprASTNode * left = dynamic_cast<ExprASTNode*>((yyvsp[-2].m_ast));
							ExprASTNode * right = dynamic_cast<ExprASTNode*>((yyvsp[0].m_ast));
							(yyval.m_ast) = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kBooleanOr, right);
							(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
						;}
    break;

  case 104:
    {
							(yyval.m_ast) = new BooleanNotExprASTNode(dynamic_cast<ExprASTNode*>((yyvsp[0].m_ast)));
							(yyval.m_ast)->setLocation((yylsp[-1]), (yylsp[0]));
						;}
    break;

  case 105:
    {
							(yyval.m_ast) = new SourceFileFunctionASTNode((yyvsp[-3].m_str), (yyvsp[-1].m_str));
							(yyval.m_ast)->setLocation((yylsp[-3]), (yylsp[0]));
						;}
    break;

  case 106:
    {
							(yyval.m_ast) = (yyvsp[-1].m_ast);
							(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
						;}
    break;

  case 107:
    {
							(yyval.m_ast) = new DefinedOperatorASTNode((yyvsp[-1].m_str));
							(yyval.m_ast)->setLocation((yylsp[-3]), (yylsp[0]));
						;}
    break;

  case 108:
    { (yyval.m_ast) = (yyvsp[0].m_ast); ;}
    break;

  case 109:
    {
								(yyval.m_ast) = new SymbolASTNode((yyvsp[0].m_str), (yyvsp[-2].m_str));
								(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
							;}
    break;

  case 110:
    {
								(yyval.m_ast) = new SymbolASTNode((yyvsp[0].m_str));
								(yyval.m_ast)->setLocation((yylsp[-1]), (yylsp[0]));
							;}
    break;

  case 111:
    {
								(yyval.m_ast) = (yyvsp[0].m_ast);
							;}
    break;

  case 112:
    {
								(yyval.m_ast) = new VariableExprASTNode((yyvsp[0].m_str));
								(yyval.m_ast)->setLocation((yylsp[0]));
							;}
    break;

  case 113:
    {
								(yyval.m_ast) = new SymbolRefExprASTNode(dynamic_cast<SymbolASTNode*>((yyvsp[0].m_ast)));
								(yyval.m_ast)->setLocation((yylsp[0]));
							;}
    break;

  case 114:
    {
								ExprASTNode * left = dynamic_cast<ExprASTNode*>((yyvsp[-2].m_ast));
								ExprASTNode * right = dynamic_cast<ExprASTNode*>((yyvsp[0].m_ast));
								(yyval.m_ast) = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kAdd, right);
								(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
							;}
    break;

  case 115:
    {
								ExprASTNode * left = dynamic_cast<ExprASTNode*>((yyvsp[-2].m_ast));
								ExprASTNode * right = dynamic_cast<ExprASTNode*>((yyvsp[0].m_ast));
								(yyval.m_ast) = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kSubtract, right);
								(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
							;}
    break;

  case 116:
    {
								ExprASTNode * left = dynamic_cast<ExprASTNode*>((yyvsp[-2].m_ast));
								ExprASTNode * right = dynamic_cast<ExprASTNode*>((yyvsp[0].m_ast));
								(yyval.m_ast) = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kMultiply, right);
								(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
							;}
    break;

  case 117:
    {
								ExprASTNode * left = dynamic_cast<ExprASTNode*>((yyvsp[-2].m_ast));
								ExprASTNode * right = dynamic_cast<ExprASTNode*>((yyvsp[0].m_ast));
								(yyval.m_ast) = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kDivide, right);
								(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
							;}
    break;

  case 118:
    {
								ExprASTNode * left = dynamic_cast<ExprASTNode*>((yyvsp[-2].m_ast));
								ExprASTNode * right = dynamic_cast<ExprASTNode*>((yyvsp[0].m_ast));
								(yyval.m_ast) = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kModulus, right);
								(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
							;}
    break;

  case 119:
    {
								ExprASTNode * left = dynamic_cast<ExprASTNode*>((yyvsp[-2].m_ast));
								ExprASTNode * right = dynamic_cast<ExprASTNode*>((yyvsp[0].m_ast));
								(yyval.m_ast) = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kPower, right);
								(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
							;}
    break;

  case 120:
    {
								ExprASTNode * left = dynamic_cast<ExprASTNode*>((yyvsp[-2].m_ast));
								ExprASTNode * right = dynamic_cast<ExprASTNode*>((yyvsp[0].m_ast));
								(yyval.m_ast) = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kBitwiseAnd, right);
								(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
							;}
    break;

  case 121:
    {
								ExprASTNode * left = dynamic_cast<ExprASTNode*>((yyvsp[-2].m_ast));
								ExprASTNode * right = dynamic_cast<ExprASTNode*>((yyvsp[0].m_ast));
								(yyval.m_ast) = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kBitwiseOr, right);
								(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
							;}
    break;

  case 122:
    {
								ExprASTNode * left = dynamic_cast<ExprASTNode*>((yyvsp[-2].m_ast));
								ExprASTNode * right = dynamic_cast<ExprASTNode*>((yyvsp[0].m_ast));
								(yyval.m_ast) = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kBitwiseXor, right);
								(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
							;}
    break;

  case 123:
    {
								ExprASTNode * left = dynamic_cast<ExprASTNode*>((yyvsp[-2].m_ast));
								ExprASTNode * right = dynamic_cast<ExprASTNode*>((yyvsp[0].m_ast));
								(yyval.m_ast) = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kShiftLeft, right);
								(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
							;}
    break;

  case 124:
    {
								ExprASTNode * left = dynamic_cast<ExprASTNode*>((yyvsp[-2].m_ast));
								ExprASTNode * right = dynamic_cast<ExprASTNode*>((yyvsp[0].m_ast));
								(yyval.m_ast) = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kShiftRight, right);
								(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
							;}
    break;

  case 125:
    {
								(yyval.m_ast) = (yyvsp[0].m_ast);
							;}
    break;

  case 126:
    {
								(yyval.m_ast) = new IntSizeExprASTNode(dynamic_cast<ExprASTNode*>((yyvsp[-2].m_ast)), (yyvsp[0].m_int)->getWordSize());
								(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
							;}
    break;

  case 127:
    {
								(yyval.m_ast) = (yyvsp[-1].m_ast);
								(yyval.m_ast)->setLocation((yylsp[-2]), (yylsp[0]));
							;}
    break;

  case 128:
    {
							(yyval.m_ast) = new SizeofOperatorASTNode(dynamic_cast<SymbolASTNode*>((yyvsp[-1].m_ast)));
							(yyval.m_ast)->setLocation((yylsp[-3]), (yylsp[0]));
						;}
    break;

  case 129:
    {
							(yyval.m_ast) = new SizeofOperatorASTNode((yyvsp[-1].m_str));
							(yyval.m_ast)->setLocation((yylsp[-3]), (yylsp[0]));
						;}
    break;

  case 130:
    {
							(yyval.m_ast) = new SizeofOperatorASTNode((yyvsp[-1].m_str));
							(yyval.m_ast)->setLocation((yylsp[-3]), (yylsp[0]));
						;}
    break;

  case 131:
    {
								(yyval.m_ast) = (yyvsp[0].m_ast);
							;}
    break;

  case 132:
    {
								(yyval.m_ast) = new NegativeExprASTNode(dynamic_cast<ExprASTNode*>((yyvsp[0].m_ast)));
								(yyval.m_ast)->setLocation((yylsp[-1]), (yylsp[0]));
							;}
    break;

  case 133:
    {
								(yyval.m_ast) = new IntConstExprASTNode((yyvsp[0].m_int)->getValue(), (yyvsp[0].m_int)->getWordSize());
								(yyval.m_ast)->setLocation((yylsp[0]));
							;}
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */

  yyvsp -= yylen;
  yyssp -= yylen;
  yylsp -= yylen;

  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

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
	  int yytype = YYTRANSLATE (yychar);
	  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
	  YYSIZE_T yysize = yysize0;
	  YYSIZE_T yysize1;
	  int yysize_overflow = 0;
	  char *yymsg = 0;
#	  define YYERROR_VERBOSE_ARGS_MAXIMUM 5
	  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
	  int yyx;

#if 0
	  /* This is so xgettext sees the translatable formats that are
	     constructed on the fly.  */
	  YY_("syntax error, unexpected %s");
	  YY_("syntax error, unexpected %s, expecting %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s or %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
#endif
	  char *yyfmt;
	  char const *yyf;
	  static char const yyunexpected[] = "syntax error, unexpected %s";
	  static char const yyexpecting[] = ", expecting %s";
	  static char const yyor[] = " or %s";
	  char yyformat[sizeof yyunexpected
			+ sizeof yyexpecting - 1
			+ ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
			   * (sizeof yyor - 1))];
	  char const *yyprefix = yyexpecting;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 1;

	  yyarg[0] = yytname[yytype];
	  yyfmt = yystpcpy (yyformat, yyunexpected);

	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
		  {
		    yycount = 1;
		    yysize = yysize0;
		    yyformat[sizeof yyunexpected - 1] = '\0';
		    break;
		  }
		yyarg[yycount++] = yytname[yyx];
		yysize1 = yysize + yytnamerr (0, yytname[yyx]);
		yysize_overflow |= yysize1 < yysize;
		yysize = yysize1;
		yyfmt = yystpcpy (yyfmt, yyprefix);
		yyprefix = yyor;
	      }

	  yyf = YY_(yyformat);
	  yysize1 = yysize + yystrlen (yyf);
	  yysize_overflow |= yysize1 < yysize;
	  yysize = yysize1;

	  if (!yysize_overflow && yysize <= YYSTACK_ALLOC_MAXIMUM)
	    yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg)
	    {
	      /* Avoid sprintf, as that infringes on the user's name space.
		 Don't have undefined behavior even if the translation
		 produced a string with the wrong number of "%s"s.  */
	      char *yyp = yymsg;
	      int yyi = 0;
	      while ((*yyp = *yyf))
		{
		  if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		    {
		      yyp += yytnamerr (yyp, yyarg[yyi++]);
		      yyf += 2;
		    }
		  else
		    {
		      yyp++;
		      yyf++;
		    }
		}
	      yyerror (&yylloc, lexer, resultAST, yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    {
	      yyerror (&yylloc, lexer, resultAST, YY_("syntax error"));
	      goto yyexhaustedlab;
	    }
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror (&yylloc, lexer, resultAST, YY_("syntax error"));
    }

  yyerror_range[0] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
        {
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
        }
      else
	{
	  yydestruct ("Error: discarding", yytoken, &yylval, &yylloc);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (0)
     goto yyerrorlab;

  yyerror_range[0] = yylsp[1-yylen];
  yylsp -= yylen;
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

      yyerror_range[0] = *yylsp;
      yydestruct ("Error: popping", yystos[yystate], yyvsp, yylsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;

  yyerror_range[1] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the look-ahead.  YYLOC is available though. */
  YYLLOC_DEFAULT (yyloc, yyerror_range - 1, 2);
  *++yylsp = yyloc;

  /* Shift the error token. */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

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
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (&yylloc, lexer, resultAST, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, &yylloc);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yylsp);
      YYPOPSTACK;
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}




/* code goes here */

static int yylex(YYSTYPE * lvalp, YYLTYPE * yylloc, ElftosbLexer * lexer)
{
	int token = lexer->yylex();
	*yylloc = lexer->getLocation();
	lexer->getSymbolValue(lvalp);
	return token;
}

static void yyerror(YYLTYPE * yylloc, ElftosbLexer * lexer, CommandFileASTNode ** resultAST, const char * error)
{
	throw syntax_error(format_string("line %d: %s\n", yylloc->m_firstLine, error));
}


