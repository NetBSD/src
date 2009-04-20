
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 1 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"

/* Parse a string into an internal time stamp.

   Copyright (C) 1999, 2000, 2002, 2003, 2004, 2005 Free Software
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
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Originally written by Steven M. Bellovin <smb@research.att.com> while
   at the University of North Carolina at Chapel Hill.  Later tweaked by
   a couple of people on Usenet.  Completely overhauled by Rich $alz
   <rsalz@bbn.com> and Jim Berets <jberets@bbn.com> in August, 1990.

   Modified by Paul Eggert <eggert@twinsun.com> in August 1999 to do
   the right thing about local DST.  Also modified by Paul Eggert
   <eggert@cs.ucla.edu> in February 2004 to support
   nanosecond-resolution time stamps, and in October 2004 to support
   TZ strings in dates.  */

/* FIXME: Check for arithmetic overflow in all cases, not just
   some of them.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "getdate.h"

/* There's no need to extend the stack, so there's no need to involve
   alloca.  */
#define YYSTACK_USE_ALLOCA 0

/* Tell Bison how much stack space is needed.  20 should be plenty for
   this grammar, which is not right recursive.  Beware setting it too
   high, since that might cause problems on machines whose
   implementations have lame stack-overflow checking.  */
#define YYMAXDEPTH 20
#define YYINITDEPTH YYMAXDEPTH

/* Since the code of getdate.y is not included in the Emacs executable
   itself, there is no need to #define static in this file.  Even if
   the code were included in the Emacs executable, it probably
   wouldn't do any harm to #undef it here; this will only cause
   problems if we try to write to a static variable, which I don't
   think this code needs to do.  */
#ifdef emacs
# undef static
#endif

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "setenv.h"
#include "xalloc.h"

#if STDC_HEADERS || (! defined isascii && ! HAVE_ISASCII)
# define IN_CTYPE_DOMAIN(c) 1
#else
# define IN_CTYPE_DOMAIN(c) isascii (c)
#endif

#define ISSPACE(c) (IN_CTYPE_DOMAIN (c) && isspace (c))
#define ISALPHA(c) (IN_CTYPE_DOMAIN (c) && isalpha (c))
#define ISLOWER(c) (IN_CTYPE_DOMAIN (c) && islower (c))

/* ISDIGIT differs from isdigit, as follows:
   - Its arg may be any int or unsigned int; it need not be an unsigned char.
   - It's guaranteed to evaluate its argument exactly once.
   - It's typically faster.
   POSIX says that only '0' through '9' are digits.  Prefer ISDIGIT to
   isdigit unless it's important to use the locale's definition
   of `digit' even when the host does not conform to POSIX.  */
#define ISDIGIT(c) ((unsigned int) (c) - '0' <= 9)

#if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 8) || __STRICT_ANSI__
# define __attribute__(x)
#endif

#ifndef ATTRIBUTE_UNUSED
# define ATTRIBUTE_UNUSED __attribute__ ((__unused__))
#endif

/* Shift A right by B bits portably, by dividing A by 2**B and
   truncating towards minus infinity.  A and B should be free of side
   effects, and B should be in the range 0 <= B <= INT_BITS - 2, where
   INT_BITS is the number of useful bits in an int.  GNU code can
   assume that INT_BITS is at least 32.

   ISO C99 says that A >> B is implementation-defined if A < 0.  Some
   implementations (e.g., UNICOS 9.0 on a Cray Y-MP EL) don't shift
   right in the usual way when A < 0, so SHR falls back on division if
   ordinary A >> B doesn't seem to be the usual signed shift.  */
#define SHR(a, b)	\
  (-1 >> 1 == -1	\
   ? (a) >> (b)		\
   : (a) / (1 << (b)) - ((a) % (1 << (b)) < 0))

#define EPOCH_YEAR 1970
#define TM_YEAR_BASE 1900

#define HOUR(x) ((x) * 60)

/* An integer value, and the number of digits in its textual
   representation.  */
typedef struct
{
  bool negative;
  long int value;
  size_t digits;
} textint;

/* An entry in the lexical lookup table.  */
typedef struct
{
  char const *name;
  int type;
  int value;
} table;

/* Meridian: am, pm, or 24-hour style.  */
enum { MERam, MERpm, MER24 };

enum { BILLION = 1000000000, LOG10_BILLION = 9 };

/* Information passed to and from the parser.  */
typedef struct
{
  /* The input string remaining to be parsed. */
  const char *input;

  /* N, if this is the Nth Tuesday.  */
  long int day_ordinal;

  /* Day of week; Sunday is 0.  */
  int day_number;

  /* tm_isdst flag for the local zone.  */
  int local_isdst;

  /* Time zone, in minutes east of UTC.  */
  long int time_zone;

  /* Style used for time.  */
  int meridian;

  /* Gregorian year, month, day, hour, minutes, seconds, and nanoseconds.  */
  textint year;
  long int month;
  long int day;
  long int hour;
  long int minutes;
  struct timespec seconds; /* includes nanoseconds */

  /* Relative year, month, day, hour, minutes, seconds, and nanoseconds.  */
  long int rel_year;
  long int rel_month;
  long int rel_day;
  long int rel_hour;
  long int rel_minutes;
  long int rel_seconds;
  long int rel_ns;

  /* Presence or counts of nonterminals of various flavors parsed so far.  */
  bool timespec_seen;
  bool rels_seen;
  size_t dates_seen;
  size_t days_seen;
  size_t local_zones_seen;
  size_t dsts_seen;
  size_t times_seen;
  size_t zones_seen;

  /* Table of local time zone abbrevations, terminated by a null entry.  */
  table local_time_zone_table[3];
} parser_control;

union YYSTYPE;
static int yylex (union YYSTYPE *, parser_control *);
static int yyerror (parser_control *, char *);
static long int time_zone_hhmm (textint, long int);



/* Line 189 of yacc.c  */
#line 272 "getdate.c"

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

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     tAGO = 258,
     tDST = 259,
     tDAY = 260,
     tDAY_UNIT = 261,
     tDAYZONE = 262,
     tHOUR_UNIT = 263,
     tLOCAL_ZONE = 264,
     tMERIDIAN = 265,
     tMINUTE_UNIT = 266,
     tMONTH = 267,
     tMONTH_UNIT = 268,
     tORDINAL = 269,
     tSEC_UNIT = 270,
     tYEAR_UNIT = 271,
     tZONE = 272,
     tSNUMBER = 273,
     tUNUMBER = 274,
     tSDECIMAL_NUMBER = 275,
     tUDECIMAL_NUMBER = 276
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 209 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"

  long int intval;
  textint textintval;
  struct timespec timespec;



/* Line 214 of yacc.c  */
#line 337 "getdate.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 349 "getdate.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

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

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
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
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  12
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   92

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  27
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  20
/* YYNRULES -- Number of rules.  */
#define YYNRULES  80
/* YYNRULES -- Number of states.  */
#define YYNSTATES  101

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   276

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    25,     2,    23,    26,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    24,     2,
       2,     2,     2,     2,    22,     2,     2,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     5,     7,    10,    11,    14,    16,    18,
      20,    22,    24,    26,    28,    30,    36,    39,    44,    50,
      57,    65,    67,    70,    72,    75,    79,    81,    84,    86,
      89,    92,    95,    99,   105,   109,   113,   117,   120,   125,
     128,   132,   135,   137,   140,   143,   145,   148,   151,   153,
     156,   159,   161,   164,   167,   169,   172,   175,   177,   180,
     183,   186,   189,   191,   193,   196,   199,   202,   205,   208,
     211,   213,   215,   217,   219,   221,   223,   225,   226,   229,
     230
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      28,     0,    -1,    29,    -1,    30,    -1,    22,    41,    -1,
      -1,    30,    31,    -1,    33,    -1,    34,    -1,    35,    -1,
      37,    -1,    36,    -1,    38,    -1,    32,    -1,    44,    -1,
      21,    23,    21,    23,    21,    -1,    19,    10,    -1,    19,
      24,    19,    46,    -1,    19,    24,    19,    18,    45,    -1,
      19,    24,    19,    24,    43,    46,    -1,    19,    24,    19,
      24,    43,    18,    45,    -1,     9,    -1,     9,     4,    -1,
      17,    -1,    17,    40,    -1,    17,    18,    45,    -1,     7,
      -1,    17,     4,    -1,     5,    -1,     5,    25,    -1,    14,
       5,    -1,    19,     5,    -1,    19,    26,    19,    -1,    19,
      26,    19,    26,    19,    -1,    19,    18,    18,    -1,    19,
      12,    18,    -1,    12,    18,    18,    -1,    12,    19,    -1,
      12,    19,    25,    19,    -1,    19,    12,    -1,    19,    12,
      19,    -1,    39,     3,    -1,    39,    -1,    14,    16,    -1,
      19,    16,    -1,    16,    -1,    14,    13,    -1,    19,    13,
      -1,    13,    -1,    14,     6,    -1,    19,     6,    -1,     6,
      -1,    14,     8,    -1,    19,     8,    -1,     8,    -1,    14,
      11,    -1,    19,    11,    -1,    11,    -1,    14,    15,    -1,
      19,    15,    -1,    20,    15,    -1,    21,    15,    -1,    15,
      -1,    40,    -1,    18,    16,    -1,    18,    13,    -1,    18,
       6,    -1,    18,     8,    -1,    18,    11,    -1,    18,    15,
      -1,    42,    -1,    43,    -1,    20,    -1,    18,    -1,    21,
      -1,    19,    -1,    19,    -1,    -1,    24,    19,    -1,    -1,
      10,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   230,   230,   231,   235,   242,   244,   248,   250,   252,
     254,   256,   258,   260,   266,   269,   296,   304,   312,   322,
     329,   341,   346,   354,   356,   358,   360,   362,   367,   372,
     377,   382,   390,   395,   415,   422,   430,   438,   443,   449,
     454,   463,   473,   477,   479,   481,   483,   485,   487,   489,
     491,   493,   495,   497,   499,   501,   503,   505,   507,   509,
     511,   513,   515,   517,   521,   523,   525,   527,   529,   531,
     535,   535,   538,   539,   544,   545,   550,   588,   589,   595,
     596
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "tAGO", "tDST", "tDAY", "tDAY_UNIT",
  "tDAYZONE", "tHOUR_UNIT", "tLOCAL_ZONE", "tMERIDIAN", "tMINUTE_UNIT",
  "tMONTH", "tMONTH_UNIT", "tORDINAL", "tSEC_UNIT", "tYEAR_UNIT", "tZONE",
  "tSNUMBER", "tUNUMBER", "tSDECIMAL_NUMBER", "tUDECIMAL_NUMBER", "'@'",
  "'.'", "':'", "','", "'/'", "$accept", "spec", "timespec", "items",
  "item", "cvsstamp", "time", "local_zone", "zone", "day", "date", "rel",
  "relunit", "relunit_snumber", "seconds", "signed_seconds",
  "unsigned_seconds", "number", "o_colon_minutes", "o_merid", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,    64,    46,    58,    44,    47
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    27,    28,    28,    29,    30,    30,    31,    31,    31,
      31,    31,    31,    31,    31,    32,    33,    33,    33,    33,
      33,    34,    34,    35,    35,    35,    35,    35,    36,    36,
      36,    36,    37,    37,    37,    37,    37,    37,    37,    37,
      37,    38,    38,    39,    39,    39,    39,    39,    39,    39,
      39,    39,    39,    39,    39,    39,    39,    39,    39,    39,
      39,    39,    39,    39,    40,    40,    40,    40,    40,    40,
      41,    41,    42,    42,    43,    43,    44,    45,    45,    46,
      46
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     2,     0,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     5,     2,     4,     5,     6,
       7,     1,     2,     1,     2,     3,     1,     2,     1,     2,
       2,     2,     3,     5,     3,     3,     3,     2,     4,     2,
       3,     2,     1,     2,     2,     1,     2,     2,     1,     2,
       2,     1,     2,     2,     1,     2,     2,     1,     2,     2,
       2,     2,     1,     1,     2,     2,     2,     2,     2,     2,
       1,     1,     1,     1,     1,     1,     1,     0,     2,     0,
       1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       5,     0,     0,     2,     3,    73,    75,    72,    74,     4,
      70,    71,     1,    28,    51,    26,    54,    21,    57,     0,
      48,     0,    62,    45,    23,     0,    76,     0,     0,     6,
      13,     7,     8,     9,    11,    10,    12,    42,    63,    14,
      29,    22,     0,    37,    30,    49,    52,    55,    46,    58,
      43,    27,    77,    24,    66,    67,    68,    65,    69,    64,
      31,    50,    53,    16,    56,    39,    47,    59,    44,     0,
       0,     0,    60,    61,     0,    41,    36,     0,     0,    25,
      35,    40,    34,    79,    32,     0,    38,    78,    80,    77,
       0,    17,     0,     0,    18,    79,    33,    15,    77,    19,
      20
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     2,     3,     4,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,     9,    10,    11,    39,    79,    91
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -76
static const yytype_int8 yypact[] =
{
     -13,    45,    67,   -76,    23,   -76,   -76,   -76,   -76,   -76,
     -76,   -76,   -76,    47,   -76,   -76,   -76,    69,   -76,     8,
     -76,    40,   -76,   -76,    -2,    46,    -5,    56,    35,   -76,
     -76,   -76,   -76,   -76,   -76,   -76,   -76,    71,   -76,   -76,
     -76,   -76,    57,    52,   -76,   -76,   -76,   -76,   -76,   -76,
     -76,   -76,     9,   -76,   -76,   -76,   -76,   -76,   -76,   -76,
     -76,   -76,   -76,   -76,   -76,    51,   -76,   -76,   -76,    58,
      59,    60,   -76,   -76,    61,   -76,   -76,    62,    64,   -76,
     -76,   -76,   -76,    -6,    54,    63,   -76,   -76,   -76,    65,
      28,   -76,    66,    70,   -76,    50,   -76,   -76,    65,   -76,
     -76
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -76,   -76,   -76,   -76,   -76,   -76,   -76,   -76,   -76,   -76,
     -76,   -76,   -76,    68,   -76,   -76,    -3,   -76,   -75,   -11
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      60,    61,    51,    62,    88,    63,    64,    65,    66,     1,
      67,    68,    89,    69,    94,    54,    52,    55,    90,    70,
      56,    71,    57,   100,    58,    59,    42,    43,    13,    14,
      15,    16,    17,    78,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    44,    45,     6,    46,     8,
      73,    47,    54,    48,    55,    49,    50,    56,    74,    57,
      88,    58,    59,     5,     6,     7,     8,    12,    98,    80,
      81,    72,    40,    41,    75,    76,    82,    77,    83,    84,
      92,    86,    85,    87,    99,    96,    93,    95,     0,    78,
       0,    97,    53
};

static const yytype_int8 yycheck[] =
{
       5,     6,     4,     8,    10,    10,    11,    12,    13,    22,
      15,    16,    18,    18,    89,     6,    18,     8,    24,    24,
      11,    26,    13,    98,    15,    16,    18,    19,     5,     6,
       7,     8,     9,    24,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,     5,     6,    19,     8,    21,
      15,    11,     6,    13,     8,    15,    16,    11,    23,    13,
      10,    15,    16,    18,    19,    20,    21,     0,    18,    18,
      19,    15,    25,     4,     3,    18,    18,    25,    19,    19,
      26,    19,    21,    19,    95,    19,    23,    90,    -1,    24,
      -1,    21,    24
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    22,    28,    29,    30,    18,    19,    20,    21,    41,
      42,    43,     0,     5,     6,     7,     8,     9,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    44,
      25,     4,    18,    19,     5,     6,     8,    11,    13,    15,
      16,     4,    18,    40,     6,     8,    11,    13,    15,    16,
       5,     6,     8,    10,    11,    12,    13,    15,    16,    18,
      24,    26,    15,    15,    23,     3,    18,    25,    24,    45,
      18,    19,    18,    19,    19,    21,    19,    19,    10,    18,
      24,    46,    26,    23,    45,    43,    19,    21,    18,    46,
      45
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
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (pc, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
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
    while (YYID (0))
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
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, pc)
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
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, pc); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, parser_control *pc)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, pc)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    parser_control *pc;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (pc);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, parser_control *pc)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, pc)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    parser_control *pc;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, pc);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule, parser_control *pc)
#else
static void
yy_reduce_print (yyvsp, yyrule, pc)
    YYSTYPE *yyvsp;
    int yyrule;
    parser_control *pc;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       , pc);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule, pc); \
} while (YYID (0))

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
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
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
      YYSIZE_T yyn = 0;
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

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
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
      int yychecklim = YYLAST - yyn + 1;
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
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
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
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, parser_control *pc)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, pc)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    parser_control *pc;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (pc);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (parser_control *pc);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */





/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (parser_control *pc)
#else
int
yyparse (pc)
    parser_control *pc;
#endif
#endif
{
/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

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
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

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
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
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

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

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

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

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
        case 4:

/* Line 1455 of yacc.c  */
#line 236 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    {
	pc->seconds = (yyvsp[(2) - (2)].timespec);
	pc->timespec_seen = true;
      ;}
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 249 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->times_seen++; ;}
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 251 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->local_zones_seen++; ;}
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 253 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->zones_seen++; ;}
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 255 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->dates_seen++; ;}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 257 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->days_seen++; ;}
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 259 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rels_seen = true; ;}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 261 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    {
        pc->dates_seen++;
        pc->zones_seen++;
        pc->times_seen++;
      ;}
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 270 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    {
    int i;
    pc->year.negative = 0;
    pc->year.value = (yyvsp[(1) - (5)].timespec).tv_sec;

    if (pc->year.value < 70)
      pc->year.value += 2000;
    else if (pc->year.value < 100)
      pc->year.value += 1900;

    for (i = pc->year.value, pc->year.digits = 0; i; i /= 10, pc->year.digits++)
      continue;
    if (pc->year.digits == 0)
      pc->year.digits++;

    pc->month = (yyvsp[(1) - (5)].timespec).tv_nsec / 10000000;
    pc->day = (yyvsp[(3) - (5)].timespec).tv_sec;
    pc->hour = (yyvsp[(3) - (5)].timespec).tv_nsec / 10000000;
    pc->minutes = (yyvsp[(5) - (5)].timespec).tv_sec;
    pc->seconds.tv_sec = (yyvsp[(5) - (5)].timespec).tv_nsec / 10000000;
    pc->seconds.tv_nsec = 0;
    pc->meridian = MER24;
    pc->time_zone = 0;
  ;}
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 297 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    {
	pc->hour = (yyvsp[(1) - (2)].textintval).value;
	pc->minutes = 0;
	pc->seconds.tv_sec = 0;
	pc->seconds.tv_nsec = 0;
	pc->meridian = (yyvsp[(2) - (2)].intval);
      ;}
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 305 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    {
	pc->hour = (yyvsp[(1) - (4)].textintval).value;
	pc->minutes = (yyvsp[(3) - (4)].textintval).value;
	pc->seconds.tv_sec = 0;
	pc->seconds.tv_nsec = 0;
	pc->meridian = (yyvsp[(4) - (4)].intval);
      ;}
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 313 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    {
	pc->hour = (yyvsp[(1) - (5)].textintval).value;
	pc->minutes = (yyvsp[(3) - (5)].textintval).value;
	pc->seconds.tv_sec = 0;
	pc->seconds.tv_nsec = 0;
	pc->meridian = MER24;
	pc->zones_seen++;
	pc->time_zone = time_zone_hhmm ((yyvsp[(4) - (5)].textintval), (yyvsp[(5) - (5)].intval));
      ;}
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 323 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    {
	pc->hour = (yyvsp[(1) - (6)].textintval).value;
	pc->minutes = (yyvsp[(3) - (6)].textintval).value;
	pc->seconds = (yyvsp[(5) - (6)].timespec);
	pc->meridian = (yyvsp[(6) - (6)].intval);
      ;}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 330 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    {
	pc->hour = (yyvsp[(1) - (7)].textintval).value;
	pc->minutes = (yyvsp[(3) - (7)].textintval).value;
	pc->seconds = (yyvsp[(5) - (7)].timespec);
	pc->meridian = MER24;
	pc->zones_seen++;
	pc->time_zone = time_zone_hhmm ((yyvsp[(6) - (7)].textintval), (yyvsp[(7) - (7)].intval));
      ;}
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 342 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    {
	pc->local_isdst = (yyvsp[(1) - (1)].intval);
	pc->dsts_seen += (0 < (yyvsp[(1) - (1)].intval));
      ;}
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 347 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    {
	pc->local_isdst = 1;
	pc->dsts_seen += (0 < (yyvsp[(1) - (2)].intval)) + 1;
      ;}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 355 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->time_zone = (yyvsp[(1) - (1)].intval); ;}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 357 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->time_zone = (yyvsp[(1) - (2)].intval); pc->rels_seen = true; ;}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 359 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->time_zone = (yyvsp[(1) - (3)].intval) + time_zone_hhmm ((yyvsp[(2) - (3)].textintval), (yyvsp[(3) - (3)].intval)); ;}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 361 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->time_zone = (yyvsp[(1) - (1)].intval) + 60; ;}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 363 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->time_zone = (yyvsp[(1) - (2)].intval) + 60; ;}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 368 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    {
	pc->day_ordinal = 1;
	pc->day_number = (yyvsp[(1) - (1)].intval);
      ;}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 373 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    {
	pc->day_ordinal = 1;
	pc->day_number = (yyvsp[(1) - (2)].intval);
      ;}
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 378 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    {
	pc->day_ordinal = (yyvsp[(1) - (2)].intval);
	pc->day_number = (yyvsp[(2) - (2)].intval);
      ;}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 383 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    {
	pc->day_ordinal = (yyvsp[(1) - (2)].textintval).value;
	pc->day_number = (yyvsp[(2) - (2)].intval);
      ;}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 391 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    {
	pc->month = (yyvsp[(1) - (3)].textintval).value;
	pc->day = (yyvsp[(3) - (3)].textintval).value;
      ;}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 396 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    {
	/* Interpret as YYYY/MM/DD if the first value has 4 or more digits,
	   otherwise as MM/DD/YY.
	   The goal in recognizing YYYY/MM/DD is solely to support legacy
	   machine-generated dates like those in an RCS log listing.  If
	   you want portability, use the ISO 8601 format.  */
	if (4 <= (yyvsp[(1) - (5)].textintval).digits)
	  {
	    pc->year = (yyvsp[(1) - (5)].textintval);
	    pc->month = (yyvsp[(3) - (5)].textintval).value;
	    pc->day = (yyvsp[(5) - (5)].textintval).value;
	  }
	else
	  {
	    pc->month = (yyvsp[(1) - (5)].textintval).value;
	    pc->day = (yyvsp[(3) - (5)].textintval).value;
	    pc->year = (yyvsp[(5) - (5)].textintval);
	  }
      ;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 416 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    {
	/* ISO 8601 format.  YYYY-MM-DD.  */
	pc->year = (yyvsp[(1) - (3)].textintval);
	pc->month = -(yyvsp[(2) - (3)].textintval).value;
	pc->day = -(yyvsp[(3) - (3)].textintval).value;
      ;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 423 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    {
	/* e.g. 17-JUN-1992.  */
	pc->day = (yyvsp[(1) - (3)].textintval).value;
	pc->month = (yyvsp[(2) - (3)].intval);
	pc->year.value = -(yyvsp[(3) - (3)].textintval).value;
	pc->year.digits = (yyvsp[(3) - (3)].textintval).digits;
      ;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 431 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    {
	/* e.g. JUN-17-1992.  */
	pc->month = (yyvsp[(1) - (3)].intval);
	pc->day = -(yyvsp[(2) - (3)].textintval).value;
	pc->year.value = -(yyvsp[(3) - (3)].textintval).value;
	pc->year.digits = (yyvsp[(3) - (3)].textintval).digits;
      ;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 439 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    {
	pc->month = (yyvsp[(1) - (2)].intval);
	pc->day = (yyvsp[(2) - (2)].textintval).value;
      ;}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 444 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    {
	pc->month = (yyvsp[(1) - (4)].intval);
	pc->day = (yyvsp[(2) - (4)].textintval).value;
	pc->year = (yyvsp[(4) - (4)].textintval);
      ;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 450 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    {
	pc->day = (yyvsp[(1) - (2)].textintval).value;
	pc->month = (yyvsp[(2) - (2)].intval);
      ;}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 455 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    {
	pc->day = (yyvsp[(1) - (3)].textintval).value;
	pc->month = (yyvsp[(2) - (3)].intval);
	pc->year = (yyvsp[(3) - (3)].textintval);
      ;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 464 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    {
	pc->rel_ns = -pc->rel_ns;
	pc->rel_seconds = -pc->rel_seconds;
	pc->rel_minutes = -pc->rel_minutes;
	pc->rel_hour = -pc->rel_hour;
	pc->rel_day = -pc->rel_day;
	pc->rel_month = -pc->rel_month;
	pc->rel_year = -pc->rel_year;
      ;}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 478 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rel_year += (yyvsp[(1) - (2)].intval) * (yyvsp[(2) - (2)].intval); ;}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 480 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rel_year += (yyvsp[(1) - (2)].textintval).value * (yyvsp[(2) - (2)].intval); ;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 482 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rel_year += (yyvsp[(1) - (1)].intval); ;}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 484 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rel_month += (yyvsp[(1) - (2)].intval) * (yyvsp[(2) - (2)].intval); ;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 486 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rel_month += (yyvsp[(1) - (2)].textintval).value * (yyvsp[(2) - (2)].intval); ;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 488 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rel_month += (yyvsp[(1) - (1)].intval); ;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 490 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rel_day += (yyvsp[(1) - (2)].intval) * (yyvsp[(2) - (2)].intval); ;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 492 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rel_day += (yyvsp[(1) - (2)].textintval).value * (yyvsp[(2) - (2)].intval); ;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 494 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rel_day += (yyvsp[(1) - (1)].intval); ;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 496 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rel_hour += (yyvsp[(1) - (2)].intval) * (yyvsp[(2) - (2)].intval); ;}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 498 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rel_hour += (yyvsp[(1) - (2)].textintval).value * (yyvsp[(2) - (2)].intval); ;}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 500 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rel_hour += (yyvsp[(1) - (1)].intval); ;}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 502 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rel_minutes += (yyvsp[(1) - (2)].intval) * (yyvsp[(2) - (2)].intval); ;}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 504 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rel_minutes += (yyvsp[(1) - (2)].textintval).value * (yyvsp[(2) - (2)].intval); ;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 506 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rel_minutes += (yyvsp[(1) - (1)].intval); ;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 508 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rel_seconds += (yyvsp[(1) - (2)].intval) * (yyvsp[(2) - (2)].intval); ;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 510 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rel_seconds += (yyvsp[(1) - (2)].textintval).value * (yyvsp[(2) - (2)].intval); ;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 512 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rel_seconds += (yyvsp[(1) - (2)].timespec).tv_sec * (yyvsp[(2) - (2)].intval); pc->rel_ns += (yyvsp[(1) - (2)].timespec).tv_nsec * (yyvsp[(2) - (2)].intval); ;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 514 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rel_seconds += (yyvsp[(1) - (2)].timespec).tv_sec * (yyvsp[(2) - (2)].intval); pc->rel_ns += (yyvsp[(1) - (2)].timespec).tv_nsec * (yyvsp[(2) - (2)].intval); ;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 516 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rel_seconds += (yyvsp[(1) - (1)].intval); ;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 522 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rel_year += (yyvsp[(1) - (2)].textintval).value * (yyvsp[(2) - (2)].intval); ;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 524 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rel_month += (yyvsp[(1) - (2)].textintval).value * (yyvsp[(2) - (2)].intval); ;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 526 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rel_day += (yyvsp[(1) - (2)].textintval).value * (yyvsp[(2) - (2)].intval); ;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 528 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rel_hour += (yyvsp[(1) - (2)].textintval).value * (yyvsp[(2) - (2)].intval); ;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 530 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rel_minutes += (yyvsp[(1) - (2)].textintval).value * (yyvsp[(2) - (2)].intval); ;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 532 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { pc->rel_seconds += (yyvsp[(1) - (2)].textintval).value * (yyvsp[(2) - (2)].intval); ;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 540 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { (yyval.timespec).tv_sec = (yyvsp[(1) - (1)].textintval).value; (yyval.timespec).tv_nsec = 0; ;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 546 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { (yyval.timespec).tv_sec = (yyvsp[(1) - (1)].textintval).value; (yyval.timespec).tv_nsec = 0; ;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 551 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    {
	if (pc->dates_seen && ! pc->year.digits
	    && ! pc->rels_seen && (pc->times_seen || 2 < (yyvsp[(1) - (1)].textintval).digits))
	  pc->year = (yyvsp[(1) - (1)].textintval);
	else
	  {
	    if (4 < (yyvsp[(1) - (1)].textintval).digits)
	      {
		pc->dates_seen++;
		pc->day = (yyvsp[(1) - (1)].textintval).value % 100;
		pc->month = ((yyvsp[(1) - (1)].textintval).value / 100) % 100;
		pc->year.value = (yyvsp[(1) - (1)].textintval).value / 10000;
		pc->year.digits = (yyvsp[(1) - (1)].textintval).digits - 4;
	      }
	    else
	      {
		pc->times_seen++;
		if ((yyvsp[(1) - (1)].textintval).digits <= 2)
		  {
		    pc->hour = (yyvsp[(1) - (1)].textintval).value;
		    pc->minutes = 0;
		  }
		else
		  {
		    pc->hour = (yyvsp[(1) - (1)].textintval).value / 100;
		    pc->minutes = (yyvsp[(1) - (1)].textintval).value % 100;
		  }
		pc->seconds.tv_sec = 0;
		pc->seconds.tv_nsec = 0;
		pc->meridian = MER24;
	      }
	  }
      ;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 588 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { (yyval.intval) = -1; ;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 590 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { (yyval.intval) = (yyvsp[(2) - (2)].textintval).value; ;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 595 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { (yyval.intval) = MER24; ;}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 597 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"
    { (yyval.intval) = (yyvsp[(1) - (1)].intval); ;}
    break;



/* Line 1455 of yacc.c  */
#line 2294 "getdate.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
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
#if ! YYERROR_VERBOSE
      yyerror (pc, YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (pc, yymsg);
	  }
	else
	  {
	    yyerror (pc, YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, pc);
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

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
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


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, pc);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
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

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (pc, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, pc);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, pc);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 600 "/usr/src/external/gpl2/xcvs/dist/lib/getdate.y"


static table const meridian_table[] =
{
  { "AM",   tMERIDIAN, MERam },
  { "A.M.", tMERIDIAN, MERam },
  { "PM",   tMERIDIAN, MERpm },
  { "P.M.", tMERIDIAN, MERpm },
  { NULL, 0, 0 }
};

static table const dst_table[] =
{
  { "DST", tDST, 0 }
};

static table const month_and_day_table[] =
{
  { "JANUARY",	tMONTH,	 1 },
  { "FEBRUARY",	tMONTH,	 2 },
  { "MARCH",	tMONTH,	 3 },
  { "APRIL",	tMONTH,	 4 },
  { "MAY",	tMONTH,	 5 },
  { "JUNE",	tMONTH,	 6 },
  { "JULY",	tMONTH,	 7 },
  { "AUGUST",	tMONTH,	 8 },
  { "SEPTEMBER",tMONTH,	 9 },
  { "SEPT",	tMONTH,	 9 },
  { "OCTOBER",	tMONTH,	10 },
  { "NOVEMBER",	tMONTH,	11 },
  { "DECEMBER",	tMONTH,	12 },
  { "SUNDAY",	tDAY,	 0 },
  { "MONDAY",	tDAY,	 1 },
  { "TUESDAY",	tDAY,	 2 },
  { "TUES",	tDAY,	 2 },
  { "WEDNESDAY",tDAY,	 3 },
  { "WEDNES",	tDAY,	 3 },
  { "THURSDAY",	tDAY,	 4 },
  { "THUR",	tDAY,	 4 },
  { "THURS",	tDAY,	 4 },
  { "FRIDAY",	tDAY,	 5 },
  { "SATURDAY",	tDAY,	 6 },
  { NULL, 0, 0 }
};

static table const time_units_table[] =
{
  { "YEAR",	tYEAR_UNIT,	 1 },
  { "MONTH",	tMONTH_UNIT,	 1 },
  { "FORTNIGHT",tDAY_UNIT,	14 },
  { "WEEK",	tDAY_UNIT,	 7 },
  { "DAY",	tDAY_UNIT,	 1 },
  { "HOUR",	tHOUR_UNIT,	 1 },
  { "MINUTE",	tMINUTE_UNIT,	 1 },
  { "MIN",	tMINUTE_UNIT,	 1 },
  { "SECOND",	tSEC_UNIT,	 1 },
  { "SEC",	tSEC_UNIT,	 1 },
  { NULL, 0, 0 }
};

/* Assorted relative-time words. */
static table const relative_time_table[] =
{
  { "TOMORROW",	tDAY_UNIT,	 1 },
  { "YESTERDAY",tDAY_UNIT,	-1 },
  { "TODAY",	tDAY_UNIT,	 0 },
  { "NOW",	tDAY_UNIT,	 0 },
  { "LAST",	tORDINAL,	-1 },
  { "THIS",	tORDINAL,	 0 },
  { "NEXT",	tORDINAL,	 1 },
  { "FIRST",	tORDINAL,	 1 },
/*{ "SECOND",	tORDINAL,	 2 }, */
  { "THIRD",	tORDINAL,	 3 },
  { "FOURTH",	tORDINAL,	 4 },
  { "FIFTH",	tORDINAL,	 5 },
  { "SIXTH",	tORDINAL,	 6 },
  { "SEVENTH",	tORDINAL,	 7 },
  { "EIGHTH",	tORDINAL,	 8 },
  { "NINTH",	tORDINAL,	 9 },
  { "TENTH",	tORDINAL,	10 },
  { "ELEVENTH",	tORDINAL,	11 },
  { "TWELFTH",	tORDINAL,	12 },
  { "AGO",	tAGO,		 1 },
  { NULL, 0, 0 }
};

/* The universal time zone table.  These labels can be used even for
   time stamps that would not otherwise be valid, e.g., GMT time
   stamps in London during summer.  */
static table const universal_time_zone_table[] =
{
  { "GMT",	tZONE,     HOUR ( 0) },	/* Greenwich Mean */
  { "UT",	tZONE,     HOUR ( 0) },	/* Universal (Coordinated) */
  { "UTC",	tZONE,     HOUR ( 0) },
  { NULL, 0, 0 }
};

/* The time zone table.  This table is necessarily incomplete, as time
   zone abbreviations are ambiguous; e.g. Australians interpret "EST"
   as Eastern time in Australia, not as US Eastern Standard Time.
   You cannot rely on getdate to handle arbitrary time zone
   abbreviations; use numeric abbreviations like `-0500' instead.  */
static table const time_zone_table[] =
{
  { "WET",	tZONE,     HOUR ( 0) },	/* Western European */
  { "WEST",	tDAYZONE,  HOUR ( 0) },	/* Western European Summer */
  { "BST",	tDAYZONE,  HOUR ( 0) },	/* British Summer */
  { "ART",	tZONE,	  -HOUR ( 3) },	/* Argentina */
  { "BRT",	tZONE,	  -HOUR ( 3) },	/* Brazil */
  { "BRST",	tDAYZONE, -HOUR ( 3) },	/* Brazil Summer */
  { "NST",	tZONE,	 -(HOUR ( 3) + 30) },	/* Newfoundland Standard */
  { "NDT",	tDAYZONE,-(HOUR ( 3) + 30) },	/* Newfoundland Daylight */
  { "AST",	tZONE,    -HOUR ( 4) },	/* Atlantic Standard */
  { "ADT",	tDAYZONE, -HOUR ( 4) },	/* Atlantic Daylight */
  { "CLT",	tZONE,    -HOUR ( 4) },	/* Chile */
  { "CLST",	tDAYZONE, -HOUR ( 4) },	/* Chile Summer */
  { "EST",	tZONE,    -HOUR ( 5) },	/* Eastern Standard */
  { "EDT",	tDAYZONE, -HOUR ( 5) },	/* Eastern Daylight */
  { "CST",	tZONE,    -HOUR ( 6) },	/* Central Standard */
  { "CDT",	tDAYZONE, -HOUR ( 6) },	/* Central Daylight */
  { "MST",	tZONE,    -HOUR ( 7) },	/* Mountain Standard */
  { "MDT",	tDAYZONE, -HOUR ( 7) },	/* Mountain Daylight */
  { "PST",	tZONE,    -HOUR ( 8) },	/* Pacific Standard */
  { "PDT",	tDAYZONE, -HOUR ( 8) },	/* Pacific Daylight */
  { "AKST",	tZONE,    -HOUR ( 9) },	/* Alaska Standard */
  { "AKDT",	tDAYZONE, -HOUR ( 9) },	/* Alaska Daylight */
  { "HST",	tZONE,    -HOUR (10) },	/* Hawaii Standard */
  { "HAST",	tZONE,	  -HOUR (10) },	/* Hawaii-Aleutian Standard */
  { "HADT",	tDAYZONE, -HOUR (10) },	/* Hawaii-Aleutian Daylight */
  { "SST",	tZONE,    -HOUR (12) },	/* Samoa Standard */
  { "WAT",	tZONE,     HOUR ( 1) },	/* West Africa */
  { "CET",	tZONE,     HOUR ( 1) },	/* Central European */
  { "CEST",	tDAYZONE,  HOUR ( 1) },	/* Central European Summer */
  { "MET",	tZONE,     HOUR ( 1) },	/* Middle European */
  { "MEZ",	tZONE,     HOUR ( 1) },	/* Middle European */
  { "MEST",	tDAYZONE,  HOUR ( 1) },	/* Middle European Summer */
  { "MESZ",	tDAYZONE,  HOUR ( 1) },	/* Middle European Summer */
  { "EET",	tZONE,     HOUR ( 2) },	/* Eastern European */
  { "EEST",	tDAYZONE,  HOUR ( 2) },	/* Eastern European Summer */
  { "CAT",	tZONE,	   HOUR ( 2) },	/* Central Africa */
  { "SAST",	tZONE,	   HOUR ( 2) },	/* South Africa Standard */
  { "EAT",	tZONE,	   HOUR ( 3) },	/* East Africa */
  { "MSK",	tZONE,	   HOUR ( 3) },	/* Moscow */
  { "MSD",	tDAYZONE,  HOUR ( 3) },	/* Moscow Daylight */
  { "IST",	tZONE,	  (HOUR ( 5) + 30) },	/* India Standard */
  { "SGT",	tZONE,     HOUR ( 8) },	/* Singapore */
  { "KST",	tZONE,     HOUR ( 9) },	/* Korea Standard */
  { "JST",	tZONE,     HOUR ( 9) },	/* Japan Standard */
  { "GST",	tZONE,     HOUR (10) },	/* Guam Standard */
  { "NZST",	tZONE,     HOUR (12) },	/* New Zealand Standard */
  { "NZDT",	tDAYZONE,  HOUR (12) },	/* New Zealand Daylight */
  { NULL, 0, 0 }
};

/* Military time zone table. */
static table const military_table[] =
{
  { "A", tZONE,	-HOUR ( 1) },
  { "B", tZONE,	-HOUR ( 2) },
  { "C", tZONE,	-HOUR ( 3) },
  { "D", tZONE,	-HOUR ( 4) },
  { "E", tZONE,	-HOUR ( 5) },
  { "F", tZONE,	-HOUR ( 6) },
  { "G", tZONE,	-HOUR ( 7) },
  { "H", tZONE,	-HOUR ( 8) },
  { "I", tZONE,	-HOUR ( 9) },
  { "K", tZONE,	-HOUR (10) },
  { "L", tZONE,	-HOUR (11) },
  { "M", tZONE,	-HOUR (12) },
  { "N", tZONE,	 HOUR ( 1) },
  { "O", tZONE,	 HOUR ( 2) },
  { "P", tZONE,	 HOUR ( 3) },
  { "Q", tZONE,	 HOUR ( 4) },
  { "R", tZONE,	 HOUR ( 5) },
  { "S", tZONE,	 HOUR ( 6) },
  { "T", tZONE,	 HOUR ( 7) },
  { "U", tZONE,	 HOUR ( 8) },
  { "V", tZONE,	 HOUR ( 9) },
  { "W", tZONE,	 HOUR (10) },
  { "X", tZONE,	 HOUR (11) },
  { "Y", tZONE,	 HOUR (12) },
  { "Z", tZONE,	 HOUR ( 0) },
  { NULL, 0, 0 }
};



/* Convert a time zone expressed as HH:MM into an integer count of
   minutes.  If MM is negative, then S is of the form HHMM and needs
   to be picked apart; otherwise, S is of the form HH.  */

static long int
time_zone_hhmm (textint s, long int mm)
{
  if (mm < 0)
    return (s.value / 100) * 60 + s.value % 100;
  else
    return s.value * 60 + (s.negative ? -mm : mm);
}

static int
to_hour (long int hours, int meridian)
{
  switch (meridian)
    {
    default: /* Pacify GCC.  */
    case MER24:
      return 0 <= hours && hours < 24 ? hours : -1;
    case MERam:
      return 0 < hours && hours < 12 ? hours : hours == 12 ? 0 : -1;
    case MERpm:
      return 0 < hours && hours < 12 ? hours + 12 : hours == 12 ? 12 : -1;
    }
}

static long int
to_year (textint textyear)
{
  long int year = textyear.value;

  if (year < 0)
    year = -year;

  /* XPG4 suggests that years 00-68 map to 2000-2068, and
     years 69-99 map to 1969-1999.  */
  else if (textyear.digits == 2)
    year += year < 69 ? 2000 : 1900;

  return year;
}

static table const *
lookup_zone (parser_control const *pc, char const *name)
{
  table const *tp;

  for (tp = universal_time_zone_table; tp->name; tp++)
    if (strcmp (name, tp->name) == 0)
      return tp;

  /* Try local zone abbreviations before those in time_zone_table, as
     the local ones are more likely to be right.  */
  for (tp = pc->local_time_zone_table; tp->name; tp++)
    if (strcmp (name, tp->name) == 0)
      return tp;

  for (tp = time_zone_table; tp->name; tp++)
    if (strcmp (name, tp->name) == 0)
      return tp;

  return NULL;
}

#if ! HAVE_TM_GMTOFF
/* Yield the difference between *A and *B,
   measured in seconds, ignoring leap seconds.
   The body of this function is taken directly from the GNU C Library;
   see src/strftime.c.  */
static long int
tm_diff (struct tm const *a, struct tm const *b)
{
  /* Compute intervening leap days correctly even if year is negative.
     Take care to avoid int overflow in leap day calculations.  */
  int a4 = SHR (a->tm_year, 2) + SHR (TM_YEAR_BASE, 2) - ! (a->tm_year & 3);
  int b4 = SHR (b->tm_year, 2) + SHR (TM_YEAR_BASE, 2) - ! (b->tm_year & 3);
  int a100 = a4 / 25 - (a4 % 25 < 0);
  int b100 = b4 / 25 - (b4 % 25 < 0);
  int a400 = SHR (a100, 2);
  int b400 = SHR (b100, 2);
  int intervening_leap_days = (a4 - b4) - (a100 - b100) + (a400 - b400);
  long int ayear = a->tm_year;
  long int years = ayear - b->tm_year;
  long int days = (365 * years + intervening_leap_days
		   + (a->tm_yday - b->tm_yday));
  return (60 * (60 * (24 * days + (a->tm_hour - b->tm_hour))
		+ (a->tm_min - b->tm_min))
	  + (a->tm_sec - b->tm_sec));
}
#endif /* ! HAVE_TM_GMTOFF */

static table const *
lookup_word (parser_control const *pc, char *word)
{
  char *p;
  char *q;
  size_t wordlen;
  table const *tp;
  bool period_found;
  bool abbrev;

  /* Make it uppercase.  */
  for (p = word; *p; p++)
    {
      unsigned char ch = *p;
      if (ISLOWER (ch))
	*p = toupper (ch);
    }

  for (tp = meridian_table; tp->name; tp++)
    if (strcmp (word, tp->name) == 0)
      return tp;

  /* See if we have an abbreviation for a month. */
  wordlen = strlen (word);
  abbrev = wordlen == 3 || (wordlen == 4 && word[3] == '.');

  for (tp = month_and_day_table; tp->name; tp++)
    if ((abbrev ? strncmp (word, tp->name, 3) : strcmp (word, tp->name)) == 0)
      return tp;

  if ((tp = lookup_zone (pc, word)))
    return tp;

  if (strcmp (word, dst_table[0].name) == 0)
    return dst_table;

  for (tp = time_units_table; tp->name; tp++)
    if (strcmp (word, tp->name) == 0)
      return tp;

  /* Strip off any plural and try the units table again. */
  if (word[wordlen - 1] == 'S')
    {
      word[wordlen - 1] = '\0';
      for (tp = time_units_table; tp->name; tp++)
	if (strcmp (word, tp->name) == 0)
	  return tp;
      word[wordlen - 1] = 'S';	/* For "this" in relative_time_table.  */
    }

  for (tp = relative_time_table; tp->name; tp++)
    if (strcmp (word, tp->name) == 0)
      return tp;

  /* Military time zones. */
  if (wordlen == 1)
    for (tp = military_table; tp->name; tp++)
      if (word[0] == tp->name[0])
	return tp;

  /* Drop out any periods and try the time zone table again. */
  for (period_found = false, p = q = word; (*p = *q); q++)
    if (*q == '.')
      period_found = true;
    else
      p++;
  if (period_found && (tp = lookup_zone (pc, word)))
    return tp;

  return NULL;
}

static int
yylex (YYSTYPE *lvalp, parser_control *pc)
{
  unsigned char c;
  size_t count;

  for (;;)
    {
      while (c = *pc->input, ISSPACE (c))
	pc->input++;

      if (ISDIGIT (c) || c == '-' || c == '+')
	{
	  char const *p;
	  int sign;
	  unsigned long int value;
	  if (c == '-' || c == '+')
	    {
	      sign = c == '-' ? -1 : 1;
	      while (c = *++pc->input, ISSPACE (c))
		continue;
	      if (! ISDIGIT (c))
		/* skip the '-' sign */
		continue;
	    }
	  else
	    sign = 0;
	  p = pc->input;
	  for (value = 0; ; value *= 10)
	    {
	      unsigned long int value1 = value + (c - '0');
	      if (value1 < value)
		return '?';
	      value = value1;
	      c = *++p;
	      if (! ISDIGIT (c))
		break;
	      if (ULONG_MAX / 10 < value)
		return '?';
	    }
	  if ((c == '.' || c == ',') && ISDIGIT (p[1]))
	    {
	      time_t s;
	      int ns;
	      int digits;
	      unsigned long int value1;

	      /* Check for overflow when converting value to time_t.  */
	      if (sign < 0)
		{
		  s = - value;
		  if (0 < s)
		    return '?';
		  value1 = -s;
		}
	      else
		{
		  s = value;
		  if (s < 0)
		    return '?';
		  value1 = s;
		}
	      if (value != value1)
		return '?';

	      /* Accumulate fraction, to ns precision.  */
	      p++;
	      ns = *p++ - '0';
	      for (digits = 2; digits <= LOG10_BILLION; digits++)
		{
		  ns *= 10;
		  if (ISDIGIT (*p))
		    ns += *p++ - '0';
		}

	      /* Skip excess digits, truncating toward -Infinity.  */
	      if (sign < 0)
		for (; ISDIGIT (*p); p++)
		  if (*p != '0')
		    {
		      ns++;
		      break;
		    }
	      while (ISDIGIT (*p))
		p++;

	      /* Adjust to the timespec convention, which is that
		 tv_nsec is always a positive offset even if tv_sec is
		 negative.  */
	      if (sign < 0 && ns)
		{
		  s--;
		  if (! (s < 0))
		    return '?';
		  ns = BILLION - ns;
		}

	      lvalp->timespec.tv_sec = s;
	      lvalp->timespec.tv_nsec = ns;
	      pc->input = p;
	      return sign ? tSDECIMAL_NUMBER : tUDECIMAL_NUMBER;
	    }
	  else
	    {
	      lvalp->textintval.negative = sign < 0;
	      if (sign < 0)
		{
		  lvalp->textintval.value = - value;
		  if (0 < lvalp->textintval.value)
		    return '?';
		}
	      else
		{
		  lvalp->textintval.value = value;
		  if (lvalp->textintval.value < 0)
		    return '?';
		}
	      lvalp->textintval.digits = p - pc->input;
	      pc->input = p;
	      return sign ? tSNUMBER : tUNUMBER;
	    }
	}

      if (ISALPHA (c))
	{
	  char buff[20];
	  char *p = buff;
	  table const *tp;

	  do
	    {
	      if (p < buff + sizeof buff - 1)
		*p++ = c;
	      c = *++pc->input;
	    }
	  while (ISALPHA (c) || c == '.');

	  *p = '\0';
	  tp = lookup_word (pc, buff);
	  if (! tp)
	    return '?';
	  lvalp->intval = tp->value;
	  return tp->type;
	}

      if (c != '(')
	return *pc->input++;
      count = 0;
      do
	{
	  c = *pc->input++;
	  if (c == '\0')
	    return c;
	  if (c == '(')
	    count++;
	  else if (c == ')')
	    count--;
	}
      while (count != 0);
    }
}

/* Do nothing if the parser reports an error.  */
static int
yyerror (parser_control *pc ATTRIBUTE_UNUSED, char *s ATTRIBUTE_UNUSED)
{
  return 0;
}

/* If *TM0 is the old and *TM1 is the new value of a struct tm after
   passing it to mktime, return true if it's OK that mktime returned T.
   It's not OK if *TM0 has out-of-range members.  */

static bool
mktime_ok (struct tm const *tm0, struct tm const *tm1, time_t t)
{
  if (t == (time_t) -1)
    {
      /* Guard against falsely reporting an error when parsing a time
	 stamp that happens to equal (time_t) -1, on a host that
	 supports such a time stamp.  */
      tm1 = localtime (&t);
      if (!tm1)
	return false;
    }

  return ! ((tm0->tm_sec ^ tm1->tm_sec)
	    | (tm0->tm_min ^ tm1->tm_min)
	    | (tm0->tm_hour ^ tm1->tm_hour)
	    | (tm0->tm_mday ^ tm1->tm_mday)
	    | (tm0->tm_mon ^ tm1->tm_mon)
	    | (tm0->tm_year ^ tm1->tm_year));
}

/* A reasonable upper bound for the size of ordinary TZ strings.
   Use heap allocation if TZ's length exceeds this.  */
enum { TZBUFSIZE = 100 };

/* Return a copy of TZ, stored in TZBUF if it fits, and heap-allocated
   otherwise.  */
static char *
get_tz (char tzbuf[TZBUFSIZE])
{
  char *tz = getenv ("TZ");
  if (tz)
    {
      size_t tzsize = strlen (tz) + 1;
      tz = (tzsize <= TZBUFSIZE
	    ? memcpy (tzbuf, tz, tzsize)
	    : xmemdup (tz, tzsize));
    }
  return tz;
}

/* Parse a date/time string, storing the resulting time value into *RESULT.
   The string itself is pointed to by P.  Return true if successful.
   P can be an incomplete or relative time specification; if so, use
   *NOW as the basis for the returned time.  */
bool
get_date (struct timespec *result, char const *p, struct timespec const *now)
{
  time_t Start;
  long int Start_ns;
  struct tm const *tmp;
  struct tm tm;
  struct tm tm0;
  parser_control pc;
  struct timespec gettime_buffer;
  unsigned char c;
  bool tz_was_altered = false;
  char *tz0 = NULL;
  char tz0buf[TZBUFSIZE];
  bool ok = true;

  if (! now)
    {
      gettime (&gettime_buffer);
      now = &gettime_buffer;
    }

  Start = now->tv_sec;
  Start_ns = now->tv_nsec;

  tmp = localtime (&now->tv_sec);
  if (! tmp)
    return false;

  while (c = *p, ISSPACE (c))
    p++;

  if (strncmp (p, "TZ=\"", 4) == 0)
    {
      char const *tzbase = p + 4;
      size_t tzsize = 1;
      char const *s;

      for (s = tzbase; *s; s++, tzsize++)
	if (*s == '\\')
	  {
	    s++;
	    if (! (*s == '\\' || *s == '"'))
	      break;
	  }
	else if (*s == '"')
	  {
	    char *z;
	    char *tz1;
	    char tz1buf[TZBUFSIZE];
	    bool large_tz = TZBUFSIZE < tzsize;
	    bool setenv_ok;
	    tz0 = get_tz (tz0buf);
	    z = tz1 = large_tz ? xmalloc (tzsize) : tz1buf;
	    for (s = tzbase; *s != '"'; s++)
	      *z++ = *(s += *s == '\\');
	    *z = '\0';
	    setenv_ok = setenv ("TZ", tz1, 1) == 0;
	    if (large_tz)
	      free (tz1);
	    if (!setenv_ok)
	      goto fail;
	    tz_was_altered = true;
	    p = s + 1;
	  }
    }

  pc.input = p;
  pc.year.value = tmp->tm_year;
  pc.year.value += TM_YEAR_BASE;
  pc.year.digits = 0;
  pc.month = tmp->tm_mon + 1;
  pc.day = tmp->tm_mday;
  pc.hour = tmp->tm_hour;
  pc.minutes = tmp->tm_min;
  pc.seconds.tv_sec = tmp->tm_sec;
  pc.seconds.tv_nsec = Start_ns;
  tm.tm_isdst = tmp->tm_isdst;

  pc.meridian = MER24;
  pc.rel_ns = 0;
  pc.rel_seconds = 0;
  pc.rel_minutes = 0;
  pc.rel_hour = 0;
  pc.rel_day = 0;
  pc.rel_month = 0;
  pc.rel_year = 0;
  pc.timespec_seen = false;
  pc.rels_seen = false;
  pc.dates_seen = 0;
  pc.days_seen = 0;
  pc.times_seen = 0;
  pc.local_zones_seen = 0;
  pc.dsts_seen = 0;
  pc.zones_seen = 0;

#if HAVE_STRUCT_TM_TM_ZONE
  pc.local_time_zone_table[0].name = tmp->tm_zone;
  pc.local_time_zone_table[0].type = tLOCAL_ZONE;
  pc.local_time_zone_table[0].value = tmp->tm_isdst;
  pc.local_time_zone_table[1].name = NULL;

  /* Probe the names used in the next three calendar quarters, looking
     for a tm_isdst different from the one we already have.  */
  {
    int quarter;
    for (quarter = 1; quarter <= 3; quarter++)
      {
	time_t probe = Start + quarter * (90 * 24 * 60 * 60);
	struct tm const *probe_tm = localtime (&probe);
	if (probe_tm && probe_tm->tm_zone
	    && probe_tm->tm_isdst != pc.local_time_zone_table[0].value)
	  {
	      {
		pc.local_time_zone_table[1].name = probe_tm->tm_zone;
		pc.local_time_zone_table[1].type = tLOCAL_ZONE;
		pc.local_time_zone_table[1].value = probe_tm->tm_isdst;
		pc.local_time_zone_table[2].name = NULL;
	      }
	    break;
	  }
      }
  }
#else
#if HAVE_TZNAME
  {
# ifndef tzname
    extern char *tzname[];
# endif
    int i;
    for (i = 0; i < 2; i++)
      {
	pc.local_time_zone_table[i].name = tzname[i];
	pc.local_time_zone_table[i].type = tLOCAL_ZONE;
	pc.local_time_zone_table[i].value = i;
      }
    pc.local_time_zone_table[i].name = NULL;
  }
#else
  pc.local_time_zone_table[0].name = NULL;
#endif
#endif

  if (pc.local_time_zone_table[0].name && pc.local_time_zone_table[1].name
      && ! strcmp (pc.local_time_zone_table[0].name,
		   pc.local_time_zone_table[1].name))
    {
      /* This locale uses the same abbrevation for standard and
	 daylight times.  So if we see that abbreviation, we don't
	 know whether it's daylight time.  */
      pc.local_time_zone_table[0].value = -1;
      pc.local_time_zone_table[1].name = NULL;
    }

  if (yyparse (&pc) != 0)
    goto fail;

  if (pc.timespec_seen)
    *result = pc.seconds;
  else
    {
      if (1 < (pc.times_seen | pc.dates_seen | pc.days_seen | pc.dsts_seen
	       | (pc.local_zones_seen + pc.zones_seen)))
	goto fail;

      tm.tm_year = to_year (pc.year) - TM_YEAR_BASE;
      tm.tm_mon = pc.month - 1;
      tm.tm_mday = pc.day;
      if (pc.times_seen || (pc.rels_seen && ! pc.dates_seen && ! pc.days_seen))
	{
	  tm.tm_hour = to_hour (pc.hour, pc.meridian);
	  if (tm.tm_hour < 0)
	    goto fail;
	  tm.tm_min = pc.minutes;
	  tm.tm_sec = pc.seconds.tv_sec;
	}
      else
	{
	  tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
	  pc.seconds.tv_nsec = 0;
	}

      /* Let mktime deduce tm_isdst if we have an absolute time stamp.  */
      if (!pc.rels_seen)
	tm.tm_isdst = -1;

      /* But if the input explicitly specifies local time with or without
	 DST, give mktime that information.  */
      if (pc.local_zones_seen)
	tm.tm_isdst = pc.local_isdst;

      tm0 = tm;

      Start = mktime (&tm);

      if (! mktime_ok (&tm0, &tm, Start))
	{
	  if (! pc.zones_seen)
	    goto fail;
	  else
	    {
	      /* Guard against falsely reporting errors near the time_t
		 boundaries when parsing times in other time zones.  For
		 example, suppose the input string "1969-12-31 23:00:00 -0100",
		 the current time zone is 8 hours ahead of UTC, and the min
		 time_t value is 1970-01-01 00:00:00 UTC.  Then the min
		 localtime value is 1970-01-01 08:00:00, and mktime will
		 therefore fail on 1969-12-31 23:00:00.  To work around the
		 problem, set the time zone to 1 hour behind UTC temporarily
		 by setting TZ="XXX1:00" and try mktime again.  */

	      long int time_zone = pc.time_zone;
	      long int abs_time_zone = time_zone < 0 ? - time_zone : time_zone;
	      long int abs_time_zone_hour = abs_time_zone / 60;
	      int abs_time_zone_min = abs_time_zone % 60;
	      char tz1buf[sizeof "XXX+0:00"
			  + sizeof pc.time_zone * CHAR_BIT / 3];
	      if (!tz_was_altered)
		tz0 = get_tz (tz0buf);
	      sprintf (tz1buf, "XXX%s%ld:%02d", "-" + (time_zone < 0),
		       abs_time_zone_hour, abs_time_zone_min);
	      if (setenv ("TZ", tz1buf, 1) != 0)
		goto fail;
	      tz_was_altered = true;
	      tm = tm0;
	      Start = mktime (&tm);
	      if (! mktime_ok (&tm0, &tm, Start))
		goto fail;
	    }
	}

      if (pc.days_seen && ! pc.dates_seen)
	{
	  tm.tm_mday += ((pc.day_number - tm.tm_wday + 7) % 7
			 + 7 * (pc.day_ordinal - (0 < pc.day_ordinal)));
	  tm.tm_isdst = -1;
	  Start = mktime (&tm);
	  if (Start == (time_t) -1)
	    goto fail;
	}

      if (pc.zones_seen)
	{
	  long int delta = pc.time_zone * 60;
	  time_t t1;
#ifdef HAVE_TM_GMTOFF
	  delta -= tm.tm_gmtoff;
#else
	  time_t t = Start;
	  struct tm const *gmt = gmtime (&t);
	  if (! gmt)
	    goto fail;
	  delta -= tm_diff (&tm, gmt);
#endif
	  t1 = Start - delta;
	  if ((Start < t1) != (delta < 0))
	    goto fail;	/* time_t overflow */
	  Start = t1;
	}

      /* Add relative date.  */
      if (pc.rel_year | pc.rel_month | pc.rel_day)
	{
	  int year = tm.tm_year + pc.rel_year;
	  int month = tm.tm_mon + pc.rel_month;
	  int day = tm.tm_mday + pc.rel_day;
	  if (((year < tm.tm_year) ^ (pc.rel_year < 0))
	      | ((month < tm.tm_mon) ^ (pc.rel_month < 0))
	      | ((day < tm.tm_mday) ^ (pc.rel_day < 0)))
	    goto fail;
	  tm.tm_year = year;
	  tm.tm_mon = month;
	  tm.tm_mday = day;
	  Start = mktime (&tm);
	  if (Start == (time_t) -1)
	    goto fail;
	}

      /* Add relative hours, minutes, and seconds.  On hosts that support
	 leap seconds, ignore the possibility of leap seconds; e.g.,
	 "+ 10 minutes" adds 600 seconds, even if one of them is a
	 leap second.  Typically this is not what the user wants, but it's
	 too hard to do it the other way, because the time zone indicator
	 must be applied before relative times, and if mktime is applied
	 again the time zone will be lost.  */
      {
	long int sum_ns = pc.seconds.tv_nsec + pc.rel_ns;
	long int normalized_ns = (sum_ns % BILLION + BILLION) % BILLION;
	time_t t0 = Start;
	long int d1 = 60 * 60 * pc.rel_hour;
	time_t t1 = t0 + d1;
	long int d2 = 60 * pc.rel_minutes;
	time_t t2 = t1 + d2;
	long int d3 = pc.rel_seconds;
	time_t t3 = t2 + d3;
	long int d4 = (sum_ns - normalized_ns) / BILLION;
	time_t t4 = t3 + d4;

	if ((d1 / (60 * 60) ^ pc.rel_hour)
	    | (d2 / 60 ^ pc.rel_minutes)
	    | ((t1 < t0) ^ (d1 < 0))
	    | ((t2 < t1) ^ (d2 < 0))
	    | ((t3 < t2) ^ (d3 < 0))
	    | ((t4 < t3) ^ (d4 < 0)))
	  goto fail;

	result->tv_sec = t4;
	result->tv_nsec = normalized_ns;
      }
    }

  goto done;

 fail:
  ok = false;
 done:
  if (tz_was_altered)
    ok &= (tz0 ? setenv ("TZ", tz0, 1) : unsetenv ("TZ")) == 0;
  if (tz0 != tz0buf)
    free (tz0);
  return ok;
}

#if TEST

int
main (int ac, char **av)
{
  char buff[BUFSIZ];

  printf ("Enter date, or blank line to exit.\n\t> ");
  fflush (stdout);

  buff[BUFSIZ - 1] = '\0';
  while (fgets (buff, BUFSIZ - 1, stdin) && buff[0])
    {
      struct timespec d;
      struct tm const *tm;
      if (! get_date (&d, buff, NULL))
	printf ("Bad format - couldn't convert.\n");
      else if (! (tm = localtime (&d.tv_sec)))
	{
	  long int sec = d.tv_sec;
	  printf ("localtime (%ld) failed\n", sec);
	}
      else
	{
	  int ns = d.tv_nsec;
	  printf ("%04ld-%02d-%02d %02d:%02d:%02d.%09d\n",
		  tm->tm_year + 1900L, tm->tm_mon + 1, tm->tm_mday,
		  tm->tm_hour, tm->tm_min, tm->tm_sec, ns);
	}
      printf ("\t> ");
      fflush (stdout);
    }
  return 0;
}
#endif /* TEST */

