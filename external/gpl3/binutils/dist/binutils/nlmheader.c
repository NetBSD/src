/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

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
#define YYBISON_VERSION "3.0.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 1 "nlmheader.y" /* yacc.c:339  */
/* nlmheader.y - parse NLM header specification keywords.
     Copyright (C) 1993-2018 Free Software Foundation, Inc.

     This file is part of GNU Binutils.

     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation; either version 3 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
     MA 02110-1301, USA.  */

/* Written by Ian Lance Taylor <ian@cygnus.com>.

   This bison file parses the commands recognized by the NetWare NLM
   linker, except for lists of object files.  It stores the
   information in global variables.

   This implementation is based on the description in the NetWare Tool
   Maker Specification manual, edition 1.0.  */

#include "sysdep.h"
#include "safe-ctype.h"
#include "bfd.h"
#include "nlm/common.h"
#include "nlm/internal.h"
#include "bucomm.h"
#include "nlmconv.h"

/* Information is stored in the structures pointed to by these
   variables.  */

Nlm_Internal_Fixed_Header *fixed_hdr;
Nlm_Internal_Variable_Header *var_hdr;
Nlm_Internal_Version_Header *version_hdr;
Nlm_Internal_Copyright_Header *copyright_hdr;
Nlm_Internal_Extended_Header *extended_hdr;

/* Procedure named by CHECK.  */
char *check_procedure;
/* File named by CUSTOM.  */
char *custom_file;
/* Whether to generate debugging information (DEBUG).  */
bfd_boolean debug_info;
/* Procedure named by EXIT.  */
char *exit_procedure;
/* Exported symbols (EXPORT).  */
struct string_list *export_symbols;
/* List of files from INPUT.  */
struct string_list *input_files;
/* Map file name (MAP, FULLMAP).  */
char *map_file;
/* Whether a full map has been requested (FULLMAP).  */
bfd_boolean full_map;
/* File named by HELP.  */
char *help_file;
/* Imported symbols (IMPORT).  */
struct string_list *import_symbols;
/* File named by MESSAGES.  */
char *message_file;
/* Autoload module list (MODULE).  */
struct string_list *modules;
/* File named by OUTPUT.  */
char *output_file;
/* File named by SHARELIB.  */
char *sharelib_file;
/* Start procedure name (START).  */
char *start_procedure;
/* VERBOSE.  */
bfd_boolean verbose;
/* RPC description file (XDCDATA).  */
char *rpc_file;

/* The number of serious errors that have occurred.  */
int parse_errors;

/* The current symbol prefix when reading a list of import or export
   symbols.  */
static char *symbol_prefix;

/* Parser error message handler.  */
#define yyerror(msg) nlmheader_error (msg);

/* Local functions.  */
static int yylex (void);
static void nlmlex_file_push (const char *);
static bfd_boolean nlmlex_file_open (const char *);
static int nlmlex_buf_init (void);
static char nlmlex_buf_add (int);
static long nlmlex_get_number (const char *);
static void nlmheader_identify (void);
static void nlmheader_warn (const char *, int);
static void nlmheader_error (const char *);
static struct string_list * string_list_cons (char *, struct string_list *);
static struct string_list * string_list_append (struct string_list *,
						struct string_list *);
static struct string_list * string_list_append1 (struct string_list *,
						 char *);
static char *xstrdup (const char *);


#line 176 "nlmheader.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
#ifndef YY_YY_NLMHEADER_H_INCLUDED
# define YY_YY_NLMHEADER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    CHECK = 258,
    CODESTART = 259,
    COPYRIGHT = 260,
    CUSTOM = 261,
    DATE = 262,
    DEBUG_K = 263,
    DESCRIPTION = 264,
    EXIT = 265,
    EXPORT = 266,
    FLAG_ON = 267,
    FLAG_OFF = 268,
    FULLMAP = 269,
    HELP = 270,
    IMPORT = 271,
    INPUT = 272,
    MAP = 273,
    MESSAGES = 274,
    MODULE = 275,
    MULTIPLE = 276,
    OS_DOMAIN = 277,
    OUTPUT = 278,
    PSEUDOPREEMPTION = 279,
    REENTRANT = 280,
    SCREENNAME = 281,
    SHARELIB = 282,
    STACK = 283,
    START = 284,
    SYNCHRONIZE = 285,
    THREADNAME = 286,
    TYPE = 287,
    VERBOSE = 288,
    VERSIONK = 289,
    XDCDATA = 290,
    STRING = 291,
    QUOTED_STRING = 292
  };
#endif
/* Tokens.  */
#define CHECK 258
#define CODESTART 259
#define COPYRIGHT 260
#define CUSTOM 261
#define DATE 262
#define DEBUG_K 263
#define DESCRIPTION 264
#define EXIT 265
#define EXPORT 266
#define FLAG_ON 267
#define FLAG_OFF 268
#define FULLMAP 269
#define HELP 270
#define IMPORT 271
#define INPUT 272
#define MAP 273
#define MESSAGES 274
#define MODULE 275
#define MULTIPLE 276
#define OS_DOMAIN 277
#define OUTPUT 278
#define PSEUDOPREEMPTION 279
#define REENTRANT 280
#define SCREENNAME 281
#define SHARELIB 282
#define STACK 283
#define START 284
#define SYNCHRONIZE 285
#define THREADNAME 286
#define TYPE 287
#define VERBOSE 288
#define VERSIONK 289
#define XDCDATA 290
#define STRING 291
#define QUOTED_STRING 292

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 112 "nlmheader.y" /* yacc.c:355  */

  char *string;
  struct string_list *list;

#line 295 "nlmheader.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_NLMHEADER_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 312 "nlmheader.c" /* yacc.c:358  */

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
#else
typedef signed char yytype_int8;
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
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
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
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
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

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  64
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   73

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  40
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  11
/* YYNRULES -- Number of rules.  */
#define YYNRULES  52
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  82

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   292

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      38,    39,     2,     2,     2,     2,     2,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   143,   143,   148,   150,   156,   160,   165,   182,   186,
     204,   208,   224,   229,   228,   236,   241,   246,   251,   256,
     261,   260,   268,   272,   276,   280,   284,   288,   292,   296,
     303,   307,   311,   327,   331,   336,   340,   344,   360,   365,
     369,   393,   409,   419,   422,   433,   437,   441,   445,   454,
     465,   482,   485
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "CHECK", "CODESTART", "COPYRIGHT",
  "CUSTOM", "DATE", "DEBUG_K", "DESCRIPTION", "EXIT", "EXPORT", "FLAG_ON",
  "FLAG_OFF", "FULLMAP", "HELP", "IMPORT", "INPUT", "MAP", "MESSAGES",
  "MODULE", "MULTIPLE", "OS_DOMAIN", "OUTPUT", "PSEUDOPREEMPTION",
  "REENTRANT", "SCREENNAME", "SHARELIB", "STACK", "START", "SYNCHRONIZE",
  "THREADNAME", "TYPE", "VERBOSE", "VERSIONK", "XDCDATA", "STRING",
  "QUOTED_STRING", "'('", "')'", "$accept", "file", "commands", "command",
  "$@1", "$@2", "symbol_list_opt", "symbol_list", "symbol_prefix",
  "symbol", "string_list", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,    40,    41
};
# endif

#define YYPACT_NINF -20

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-20)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int8 yypact[] =
{
      -3,    -1,     1,     2,     4,     5,   -20,     6,     8,   -20,
       9,    10,    11,    12,   -20,    13,    14,    16,    13,   -20,
     -20,    17,   -20,   -20,    18,    20,    21,    22,   -20,    23,
      25,   -20,    26,    27,    38,   -20,    -3,   -20,   -20,   -20,
     -20,    28,   -20,   -20,    -2,   -20,   -20,   -20,   -20,    -2,
      13,   -20,   -20,   -20,   -20,   -20,   -20,   -20,   -20,   -20,
     -20,   -20,    30,   -20,   -20,   -20,    31,   -20,    32,   -20,
      -2,   -20,   -20,   -20,   -20,    33,   -20,     3,   -20,   -20,
     -20,   -20
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       3,     0,     0,     0,     0,     0,    10,     0,     0,    13,
       0,     0,    17,     0,    20,    51,    23,     0,    51,    27,
      28,     0,    30,    31,     0,     0,     0,     0,    36,     0,
       0,    39,     0,     0,     0,     2,     3,     5,     6,     7,
       8,     0,    11,    12,    43,    15,    16,    18,    19,    43,
      51,    22,    24,    25,    26,    29,    32,    33,    34,    35,
      37,    38,     0,    42,     1,     4,     0,    50,     0,    14,
      44,    46,    45,    21,    52,    41,     9,     0,    48,    47,
      40,    49
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -20,   -20,    34,   -20,   -20,   -20,    24,   -20,   -19,   -16,
      15
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,    34,    35,    36,    44,    49,    69,    70,    71,    72,
      51
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
       1,     2,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    54,    67,    37,    68,    38,    64,    39,
      40,    41,    81,    42,    43,    45,    46,    47,    48,    50,
      52,    78,    53,    55,    79,    56,    57,    58,    59,     0,
      60,    61,    62,    63,    66,    74,    75,    76,    77,    80,
      65,     0,     0,    73
};

static const yytype_int8 yycheck[] =
{
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    18,    36,    36,    38,    36,     0,    37,
      36,    36,    39,    37,    36,    36,    36,    36,    36,    36,
      36,    70,    36,    36,    70,    37,    36,    36,    36,    -1,
      37,    36,    36,    36,    36,    50,    36,    36,    36,    36,
      36,    -1,    -1,    49
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    41,    42,    43,    36,    36,    37,
      36,    36,    37,    36,    44,    36,    36,    36,    36,    45,
      36,    50,    36,    36,    50,    36,    37,    36,    36,    36,
      37,    36,    36,    36,     0,    42,    36,    36,    38,    46,
      47,    48,    49,    46,    50,    36,    36,    36,    48,    49,
      36,    39
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    40,    41,    42,    42,    43,    43,    43,    43,    43,
      43,    43,    43,    44,    43,    43,    43,    43,    43,    43,
      45,    43,    43,    43,    43,    43,    43,    43,    43,    43,
      43,    43,    43,    43,    43,    43,    43,    43,    43,    43,
      43,    43,    43,    46,    46,    47,    47,    47,    47,    48,
      49,    50,    50
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     0,     2,     2,     2,     2,     2,     4,
       1,     2,     2,     0,     3,     2,     2,     1,     2,     2,
       0,     3,     2,     1,     2,     2,     2,     1,     1,     2,
       1,     1,     2,     2,     2,     2,     1,     2,     2,     1,
       4,     3,     2,     0,     1,     1,     1,     2,     2,     3,
       1,     0,     2
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
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
#ifndef YYINITDEPTH
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
static YYSIZE_T
yystrlen (const char *yystr)
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
static char *
yystpcpy (char *yydest, const char *yysrc)
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

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
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
  int yytoken = 0;
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

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
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
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
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
      if (yytable_value_is_error (yyn))
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
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

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
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 5:
#line 157 "nlmheader.y" /* yacc.c:1646  */
    {
	    check_procedure = (yyvsp[0].string);
	  }
#line 1448 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 6:
#line 161 "nlmheader.y" /* yacc.c:1646  */
    {
	    nlmheader_warn (_("CODESTART is not implemented; sorry"), -1);
	    free ((yyvsp[0].string));
	  }
#line 1457 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 7:
#line 166 "nlmheader.y" /* yacc.c:1646  */
    {
	    int len;

	    strncpy (copyright_hdr->stamp, "CoPyRiGhT=", 10);
	    len = strlen ((yyvsp[0].string));
	    if (len >= NLM_MAX_COPYRIGHT_MESSAGE_LENGTH)
	      {
		nlmheader_warn (_("copyright string is too long"),
				NLM_MAX_COPYRIGHT_MESSAGE_LENGTH - 1);
		len = NLM_MAX_COPYRIGHT_MESSAGE_LENGTH - 1;
	      }
	    copyright_hdr->copyrightMessageLength = len;
	    strncpy (copyright_hdr->copyrightMessage, (yyvsp[0].string), len);
	    copyright_hdr->copyrightMessage[len] = '\0';
	    free ((yyvsp[0].string));
	  }
#line 1478 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 8:
#line 183 "nlmheader.y" /* yacc.c:1646  */
    {
	    custom_file = (yyvsp[0].string);
	  }
#line 1486 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 9:
#line 187 "nlmheader.y" /* yacc.c:1646  */
    {
	    /* We don't set the version stamp here, because we use the
	       version stamp to detect whether the required VERSION
	       keyword was given.  */
	    version_hdr->month = nlmlex_get_number ((yyvsp[-2].string));
	    version_hdr->day = nlmlex_get_number ((yyvsp[-1].string));
	    version_hdr->year = nlmlex_get_number ((yyvsp[0].string));
	    free ((yyvsp[-2].string));
	    free ((yyvsp[-1].string));
	    free ((yyvsp[0].string));
	    if (version_hdr->month < 1 || version_hdr->month > 12)
	      nlmheader_warn (_("illegal month"), -1);
	    if (version_hdr->day < 1 || version_hdr->day > 31)
	      nlmheader_warn (_("illegal day"), -1);
	    if (version_hdr->year < 1900 || version_hdr->year > 3000)
	      nlmheader_warn (_("illegal year"), -1);
	  }
#line 1508 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 10:
#line 205 "nlmheader.y" /* yacc.c:1646  */
    {
	    debug_info = TRUE;
	  }
#line 1516 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 11:
#line 209 "nlmheader.y" /* yacc.c:1646  */
    {
	    int len;

	    len = strlen ((yyvsp[0].string));
	    if (len > NLM_MAX_DESCRIPTION_LENGTH)
	      {
		nlmheader_warn (_("description string is too long"),
				NLM_MAX_DESCRIPTION_LENGTH);
		len = NLM_MAX_DESCRIPTION_LENGTH;
	      }
	    var_hdr->descriptionLength = len;
	    strncpy (var_hdr->descriptionText, (yyvsp[0].string), len);
	    var_hdr->descriptionText[len] = '\0';
	    free ((yyvsp[0].string));
	  }
#line 1536 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 12:
#line 225 "nlmheader.y" /* yacc.c:1646  */
    {
	    exit_procedure = (yyvsp[0].string);
	  }
#line 1544 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 13:
#line 229 "nlmheader.y" /* yacc.c:1646  */
    {
	    symbol_prefix = NULL;
	  }
#line 1552 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 14:
#line 233 "nlmheader.y" /* yacc.c:1646  */
    {
	    export_symbols = string_list_append (export_symbols, (yyvsp[0].list));
	  }
#line 1560 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 15:
#line 237 "nlmheader.y" /* yacc.c:1646  */
    {
	    fixed_hdr->flags |= nlmlex_get_number ((yyvsp[0].string));
	    free ((yyvsp[0].string));
	  }
#line 1569 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 16:
#line 242 "nlmheader.y" /* yacc.c:1646  */
    {
	    fixed_hdr->flags &=~ nlmlex_get_number ((yyvsp[0].string));
	    free ((yyvsp[0].string));
	  }
#line 1578 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 17:
#line 247 "nlmheader.y" /* yacc.c:1646  */
    {
	    map_file = "";
	    full_map = TRUE;
	  }
#line 1587 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 18:
#line 252 "nlmheader.y" /* yacc.c:1646  */
    {
	    map_file = (yyvsp[0].string);
	    full_map = TRUE;
	  }
#line 1596 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 19:
#line 257 "nlmheader.y" /* yacc.c:1646  */
    {
	    help_file = (yyvsp[0].string);
	  }
#line 1604 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 20:
#line 261 "nlmheader.y" /* yacc.c:1646  */
    {
	    symbol_prefix = NULL;
	  }
#line 1612 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 21:
#line 265 "nlmheader.y" /* yacc.c:1646  */
    {
	    import_symbols = string_list_append (import_symbols, (yyvsp[0].list));
	  }
#line 1620 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 22:
#line 269 "nlmheader.y" /* yacc.c:1646  */
    {
	    input_files = string_list_append (input_files, (yyvsp[0].list));
	  }
#line 1628 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 23:
#line 273 "nlmheader.y" /* yacc.c:1646  */
    {
	    map_file = "";
	  }
#line 1636 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 24:
#line 277 "nlmheader.y" /* yacc.c:1646  */
    {
	    map_file = (yyvsp[0].string);
	  }
#line 1644 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 25:
#line 281 "nlmheader.y" /* yacc.c:1646  */
    {
	    message_file = (yyvsp[0].string);
	  }
#line 1652 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 26:
#line 285 "nlmheader.y" /* yacc.c:1646  */
    {
	    modules = string_list_append (modules, (yyvsp[0].list));
	  }
#line 1660 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 27:
#line 289 "nlmheader.y" /* yacc.c:1646  */
    {
	    fixed_hdr->flags |= 0x2;
	  }
#line 1668 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 28:
#line 293 "nlmheader.y" /* yacc.c:1646  */
    {
	    fixed_hdr->flags |= 0x10;
	  }
#line 1676 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 29:
#line 297 "nlmheader.y" /* yacc.c:1646  */
    {
	    if (output_file == NULL)
	      output_file = (yyvsp[0].string);
	    else
	      nlmheader_warn (_("ignoring duplicate OUTPUT statement"), -1);
	  }
#line 1687 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 30:
#line 304 "nlmheader.y" /* yacc.c:1646  */
    {
	    fixed_hdr->flags |= 0x8;
	  }
#line 1695 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 31:
#line 308 "nlmheader.y" /* yacc.c:1646  */
    {
	    fixed_hdr->flags |= 0x1;
	  }
#line 1703 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 32:
#line 312 "nlmheader.y" /* yacc.c:1646  */
    {
	    int len;

	    len = strlen ((yyvsp[0].string));
	    if (len >= NLM_MAX_SCREEN_NAME_LENGTH)
	      {
		nlmheader_warn (_("screen name is too long"),
				NLM_MAX_SCREEN_NAME_LENGTH);
		len = NLM_MAX_SCREEN_NAME_LENGTH;
	      }
	    var_hdr->screenNameLength = len;
	    strncpy (var_hdr->screenName, (yyvsp[0].string), len);
	    var_hdr->screenName[NLM_MAX_SCREEN_NAME_LENGTH] = '\0';
	    free ((yyvsp[0].string));
	  }
#line 1723 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 33:
#line 328 "nlmheader.y" /* yacc.c:1646  */
    {
	    sharelib_file = (yyvsp[0].string);
	  }
#line 1731 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 34:
#line 332 "nlmheader.y" /* yacc.c:1646  */
    {
	    var_hdr->stackSize = nlmlex_get_number ((yyvsp[0].string));
	    free ((yyvsp[0].string));
	  }
#line 1740 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 35:
#line 337 "nlmheader.y" /* yacc.c:1646  */
    {
	    start_procedure = (yyvsp[0].string);
	  }
#line 1748 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 36:
#line 341 "nlmheader.y" /* yacc.c:1646  */
    {
	    fixed_hdr->flags |= 0x4;
	  }
#line 1756 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 37:
#line 345 "nlmheader.y" /* yacc.c:1646  */
    {
	    int len;

	    len = strlen ((yyvsp[0].string));
	    if (len >= NLM_MAX_THREAD_NAME_LENGTH)
	      {
		nlmheader_warn (_("thread name is too long"),
				NLM_MAX_THREAD_NAME_LENGTH);
		len = NLM_MAX_THREAD_NAME_LENGTH;
	      }
	    var_hdr->threadNameLength = len;
	    strncpy (var_hdr->threadName, (yyvsp[0].string), len);
	    var_hdr->threadName[len] = '\0';
	    free ((yyvsp[0].string));
	  }
#line 1776 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 38:
#line 361 "nlmheader.y" /* yacc.c:1646  */
    {
	    fixed_hdr->moduleType = nlmlex_get_number ((yyvsp[0].string));
	    free ((yyvsp[0].string));
	  }
#line 1785 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 39:
#line 366 "nlmheader.y" /* yacc.c:1646  */
    {
	    verbose = TRUE;
	  }
#line 1793 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 40:
#line 370 "nlmheader.y" /* yacc.c:1646  */
    {
	    long val;

	    strncpy (version_hdr->stamp, "VeRsIoN#", 8);
	    version_hdr->majorVersion = nlmlex_get_number ((yyvsp[-2].string));
	    val = nlmlex_get_number ((yyvsp[-1].string));
	    if (val < 0 || val > 99)
	      nlmheader_warn (_("illegal minor version number (must be between 0 and 99)"),
			      -1);
	    else
	      version_hdr->minorVersion = val;
	    val = nlmlex_get_number ((yyvsp[0].string));
	    if (val < 0)
	      nlmheader_warn (_("illegal revision number (must be between 0 and 26)"),
			      -1);
	    else if (val > 26)
	      version_hdr->revision = 0;
	    else
	      version_hdr->revision = val;
	    free ((yyvsp[-2].string));
	    free ((yyvsp[-1].string));
	    free ((yyvsp[0].string));
	  }
#line 1821 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 41:
#line 394 "nlmheader.y" /* yacc.c:1646  */
    {
	    long val;

	    strncpy (version_hdr->stamp, "VeRsIoN#", 8);
	    version_hdr->majorVersion = nlmlex_get_number ((yyvsp[-1].string));
	    val = nlmlex_get_number ((yyvsp[0].string));
	    if (val < 0 || val > 99)
	      nlmheader_warn (_("illegal minor version number (must be between 0 and 99)"),
			      -1);
	    else
	      version_hdr->minorVersion = val;
	    version_hdr->revision = 0;
	    free ((yyvsp[-1].string));
	    free ((yyvsp[0].string));
	  }
#line 1841 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 42:
#line 410 "nlmheader.y" /* yacc.c:1646  */
    {
	    rpc_file = (yyvsp[0].string);
	  }
#line 1849 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 43:
#line 419 "nlmheader.y" /* yacc.c:1646  */
    {
	    (yyval.list) = NULL;
	  }
#line 1857 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 44:
#line 423 "nlmheader.y" /* yacc.c:1646  */
    {
	    (yyval.list) = (yyvsp[0].list);
	  }
#line 1865 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 45:
#line 434 "nlmheader.y" /* yacc.c:1646  */
    {
	    (yyval.list) = string_list_cons ((yyvsp[0].string), NULL);
	  }
#line 1873 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 46:
#line 438 "nlmheader.y" /* yacc.c:1646  */
    {
	    (yyval.list) = NULL;
	  }
#line 1881 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 47:
#line 442 "nlmheader.y" /* yacc.c:1646  */
    {
	    (yyval.list) = string_list_append1 ((yyvsp[-1].list), (yyvsp[0].string));
	  }
#line 1889 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 48:
#line 446 "nlmheader.y" /* yacc.c:1646  */
    {
	    (yyval.list) = (yyvsp[-1].list);
	  }
#line 1897 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 49:
#line 455 "nlmheader.y" /* yacc.c:1646  */
    {
	    if (symbol_prefix != NULL)
	      free (symbol_prefix);
	    symbol_prefix = (yyvsp[-1].string);
	  }
#line 1907 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 50:
#line 466 "nlmheader.y" /* yacc.c:1646  */
    {
	    if (symbol_prefix == NULL)
	      (yyval.string) = (yyvsp[0].string);
	    else
	      {
		(yyval.string) = xmalloc (strlen (symbol_prefix) + strlen ((yyvsp[0].string)) + 2);
		sprintf ((yyval.string), "%s@%s", symbol_prefix, (yyvsp[0].string));
		free ((yyvsp[0].string));
	      }
	  }
#line 1922 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 51:
#line 482 "nlmheader.y" /* yacc.c:1646  */
    {
	    (yyval.list) = NULL;
	  }
#line 1930 "nlmheader.c" /* yacc.c:1646  */
    break;

  case 52:
#line 486 "nlmheader.y" /* yacc.c:1646  */
    {
	    (yyval.list) = string_list_cons ((yyvsp[-1].string), (yyvsp[0].list));
	  }
#line 1938 "nlmheader.c" /* yacc.c:1646  */
    break;


#line 1942 "nlmheader.c" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
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
                      yytoken, &yylval);
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

  /* Do not reclaim the symbols of the rule whose action triggered
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
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
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
                  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


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

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp);
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
  return yyresult;
}
#line 491 "nlmheader.y" /* yacc.c:1906  */


/* If strerror is just a macro, we want to use the one from libiberty
   since it will handle undefined values.  */
#undef strerror
extern char *strerror (int);

/* The lexer is simple, too simple for flex.  Keywords are only
   recognized at the start of lines.  Everything else must be an
   argument.  A comma is treated as whitespace.  */

/* The states the lexer can be in.  */

enum lex_state
{
  /* At the beginning of a line.  */
  BEGINNING_OF_LINE,
  /* In the middle of a line.  */
  IN_LINE
};

/* We need to keep a stack of files to handle file inclusion.  */

struct input
{
  /* The file to read from.  */
  FILE *file;
  /* The name of the file.  */
  char *name;
  /* The current line number.  */
  int lineno;
  /* The current state.  */
  enum lex_state state;
  /* The next file on the stack.  */
  struct input *next;
};

/* The current input file.  */

static struct input current;

/* The character which introduces comments.  */
#define COMMENT_CHAR '#'

/* Start the lexer going on the main input file.  */

bfd_boolean
nlmlex_file (const char *name)
{
  current.next = NULL;
  return nlmlex_file_open (name);
}

/* Start the lexer going on a subsidiary input file.  */

static void
nlmlex_file_push (const char *name)
{
  struct input *push;

  push = (struct input *) xmalloc (sizeof (struct input));
  *push = current;
  if (nlmlex_file_open (name))
    current.next = push;
  else
    {
      current = *push;
      free (push);
    }
}

/* Start lexing from a file.  */

static bfd_boolean
nlmlex_file_open (const char *name)
{
  current.file = fopen (name, "r");
  if (current.file == NULL)
    {
      fprintf (stderr, "%s:%s: %s\n", program_name, name, strerror (errno));
      ++parse_errors;
      return FALSE;
    }
  current.name = xstrdup (name);
  current.lineno = 1;
  current.state = BEGINNING_OF_LINE;
  return TRUE;
}

/* Table used to turn keywords into tokens.  */

struct keyword_tokens_struct
{
  const char *keyword;
  int token;
};

static struct keyword_tokens_struct keyword_tokens[] =
{
  { "CHECK", CHECK },
  { "CODESTART", CODESTART },
  { "COPYRIGHT", COPYRIGHT },
  { "CUSTOM", CUSTOM },
  { "DATE", DATE },
  { "DEBUG", DEBUG_K },
  { "DESCRIPTION", DESCRIPTION },
  { "EXIT", EXIT },
  { "EXPORT", EXPORT },
  { "FLAG_ON", FLAG_ON },
  { "FLAG_OFF", FLAG_OFF },
  { "FULLMAP", FULLMAP },
  { "HELP", HELP },
  { "IMPORT", IMPORT },
  { "INPUT", INPUT },
  { "MAP", MAP },
  { "MESSAGES", MESSAGES },
  { "MODULE", MODULE },
  { "MULTIPLE", MULTIPLE },
  { "OS_DOMAIN", OS_DOMAIN },
  { "OUTPUT", OUTPUT },
  { "PSEUDOPREEMPTION", PSEUDOPREEMPTION },
  { "REENTRANT", REENTRANT },
  { "SCREENNAME", SCREENNAME },
  { "SHARELIB", SHARELIB },
  { "STACK", STACK },
  { "STACKSIZE", STACK },
  { "START", START },
  { "SYNCHRONIZE", SYNCHRONIZE },
  { "THREADNAME", THREADNAME },
  { "TYPE", TYPE },
  { "VERBOSE", VERBOSE },
  { "VERSION", VERSIONK },
  { "XDCDATA", XDCDATA }
};

#define KEYWORD_COUNT (sizeof (keyword_tokens) / sizeof (keyword_tokens[0]))

/* The lexer accumulates strings in these variables.  */
static char *lex_buf;
static int lex_size;
static int lex_pos;

/* Start accumulating strings into the buffer.  */
#define BUF_INIT() \
  ((void) (lex_buf != NULL ? lex_pos = 0 : nlmlex_buf_init ()))

static int
nlmlex_buf_init (void)
{
  lex_size = 10;
  lex_buf = xmalloc (lex_size + 1);
  lex_pos = 0;
  return 0;
}

/* Finish a string in the buffer.  */
#define BUF_FINISH() ((void) (lex_buf[lex_pos] = '\0'))

/* Accumulate a character into the buffer.  */
#define BUF_ADD(c) \
  ((void) (lex_pos < lex_size \
	   ? lex_buf[lex_pos++] = (c) \
	   : nlmlex_buf_add (c)))

static char
nlmlex_buf_add (int c)
{
  if (lex_pos >= lex_size)
    {
      lex_size *= 2;
      lex_buf = xrealloc (lex_buf, lex_size + 1);
    }

  return lex_buf[lex_pos++] = c;
}

/* The lexer proper.  This is called by the bison generated parsing
   code.  */

static int
yylex (void)
{
  int c;

tail_recurse:

  c = getc (current.file);

  /* Commas are treated as whitespace characters.  */
  while (ISSPACE (c) || c == ',')
    {
      current.state = IN_LINE;
      if (c == '\n')
	{
	  ++current.lineno;
	  current.state = BEGINNING_OF_LINE;
	}
      c = getc (current.file);
    }

  /* At the end of the file we either pop to the previous file or
     finish up.  */
  if (c == EOF)
    {
      fclose (current.file);
      free (current.name);
      if (current.next == NULL)
	return 0;
      else
	{
	  struct input *next;

	  next = current.next;
	  current = *next;
	  free (next);
	  goto tail_recurse;
	}
    }

  /* A comment character always means to drop everything until the
     next newline.  */
  if (c == COMMENT_CHAR)
    {
      do
	{
	  c = getc (current.file);
	}
      while (c != '\n');
      ++current.lineno;
      current.state = BEGINNING_OF_LINE;
      goto tail_recurse;
    }

  /* An '@' introduces an include file.  */
  if (c == '@')
    {
      do
	{
	  c = getc (current.file);
	  if (c == '\n')
	    ++current.lineno;
	}
      while (ISSPACE (c));
      BUF_INIT ();
      while (! ISSPACE (c) && c != EOF)
	{
	  BUF_ADD (c);
	  c = getc (current.file);
	}
      BUF_FINISH ();

      ungetc (c, current.file);

      nlmlex_file_push (lex_buf);
      goto tail_recurse;
    }

  /* A non-space character at the start of a line must be the start of
     a keyword.  */
  if (current.state == BEGINNING_OF_LINE)
    {
      BUF_INIT ();
      while (ISALNUM (c) || c == '_')
	{
	  BUF_ADD (TOUPPER (c));
	  c = getc (current.file);
	}
      BUF_FINISH ();

      if (c != EOF && ! ISSPACE (c) && c != ',')
	{
	  nlmheader_identify ();
	  fprintf (stderr, _("%s:%d: illegal character in keyword: %c\n"),
		   current.name, current.lineno, c);
	}
      else
	{
	  unsigned int i;

	  for (i = 0; i < KEYWORD_COUNT; i++)
	    {
	      if (lex_buf[0] == keyword_tokens[i].keyword[0]
		  && strcmp (lex_buf, keyword_tokens[i].keyword) == 0)
		{
		  /* Pushing back the final whitespace avoids worrying
		     about \n here.  */
		  ungetc (c, current.file);
		  current.state = IN_LINE;
		  return keyword_tokens[i].token;
		}
	    }

	  nlmheader_identify ();
	  fprintf (stderr, _("%s:%d: unrecognized keyword: %s\n"),
		   current.name, current.lineno, lex_buf);
	}

      ++parse_errors;
      /* Treat the rest of this line as a comment.  */
      ungetc (COMMENT_CHAR, current.file);
      goto tail_recurse;
    }

  /* Parentheses just represent themselves.  */
  if (c == '(' || c == ')')
    return c;

  /* Handle quoted strings.  */
  if (c == '"' || c == '\'')
    {
      int quote;
      int start_lineno;

      quote = c;
      start_lineno = current.lineno;

      c = getc (current.file);
      BUF_INIT ();
      while (c != quote && c != EOF)
	{
	  BUF_ADD (c);
	  if (c == '\n')
	    ++current.lineno;
	  c = getc (current.file);
	}
      BUF_FINISH ();

      if (c == EOF)
	{
	  nlmheader_identify ();
	  fprintf (stderr, _("%s:%d: end of file in quoted string\n"),
		   current.name, start_lineno);
	  ++parse_errors;
	}

      /* FIXME: Possible memory leak.  */
      yylval.string = xstrdup (lex_buf);
      return QUOTED_STRING;
    }

  /* Gather a generic argument.  */
  BUF_INIT ();
  while (! ISSPACE (c)
	 && c != ','
	 && c != COMMENT_CHAR
	 && c != '('
	 && c != ')')
    {
      BUF_ADD (c);
      c = getc (current.file);
    }
  BUF_FINISH ();

  ungetc (c, current.file);

  /* FIXME: Possible memory leak.  */
  yylval.string = xstrdup (lex_buf);
  return STRING;
}

/* Get a number from a string.  */

static long
nlmlex_get_number (const char *s)
{
  long ret;
  char *send;

  ret = strtol (s, &send, 10);
  if (*send != '\0')
    nlmheader_warn (_("bad number"), -1);
  return ret;
}

/* Prefix the nlmconv warnings with a note as to where they come from.
   We don't use program_name on every warning, because then some
   versions of the emacs next-error function can't recognize the line
   number.  */

static void
nlmheader_identify (void)
{
  static int done;

  if (! done)
    {
      fprintf (stderr, _("%s: problems in NLM command language input:\n"),
	       program_name);
      done = 1;
    }
}

/* Issue a warning.  */

static void
nlmheader_warn (const char *s, int imax)
{
  nlmheader_identify ();
  fprintf (stderr, "%s:%d: %s", current.name, current.lineno, s);
  if (imax != -1)
    fprintf (stderr, " (max %d)", imax);
  fprintf (stderr, "\n");
}

/* Report an error.  */

static void
nlmheader_error (const char *s)
{
  nlmheader_warn (s, -1);
  ++parse_errors;
}

/* Add a string to a string list.  */

static struct string_list *
string_list_cons (char *s, struct string_list *l)
{
  struct string_list *ret;

  ret = (struct string_list *) xmalloc (sizeof (struct string_list));
  ret->next = l;
  ret->string = s;
  return ret;
}

/* Append a string list to another string list.  */

static struct string_list *
string_list_append (struct string_list *l1, struct string_list *l2)
{
  register struct string_list **pp;

  for (pp = &l1; *pp != NULL; pp = &(*pp)->next)
    ;
  *pp = l2;
  return l1;
}

/* Append a string to a string list.  */

static struct string_list *
string_list_append1 (struct string_list *l, char *s)
{
  struct string_list *n;
  register struct string_list **pp;

  n = (struct string_list *) xmalloc (sizeof (struct string_list));
  n->next = NULL;
  n->string = s;
  for (pp = &l; *pp != NULL; pp = &(*pp)->next)
    ;
  *pp = n;
  return l;
}

/* Duplicate a string in memory.  */

static char *
xstrdup (const char *s)
{
  unsigned long len;
  char *ret;

  len = strlen (s);
  ret = xmalloc (len + 1);
  strcpy (ret, s);
  return ret;
}
