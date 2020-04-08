/* A Bison parser, made by GNU Bison 3.0.5.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018 Free Software Foundation, Inc.

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
#define YYBISON_VERSION "3.0.5"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 1 "rcparse.y" /* yacc.c:339  */
 /* rcparse.y -- parser for Windows rc files
   Copyright (C) 1997-2020 Free Software Foundation, Inc.
   Written by Ian Lance Taylor, Cygnus Support.
   Extended by Kai Tietz, Onevision.

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
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */


/* This is a parser for Windows rc files.  It is based on the parser
   by Gunther Ebert <gunther.ebert@ixos-leipzig.de>.  */

#include "sysdep.h"
#include "bfd.h"
#include "bucomm.h"
#include "libiberty.h"
#include "windres.h"
#include "safe-ctype.h"

/* The current language.  */

static unsigned short language;

/* The resource information during a sub statement.  */

static rc_res_res_info sub_res_info;

/* Dialog information.  This is built by the nonterminals styles and
   controls.  */

static rc_dialog dialog;

/* This is used when building a style.  It is modified by the
   nonterminal styleexpr.  */

static unsigned long style;

/* These are used when building a control.  They are set before using
   control_params.  */

static rc_uint_type base_style;
static rc_uint_type default_style;
static rc_res_id class;
static rc_res_id res_text_field;
static unichar null_unichar;

/* This is used for COMBOBOX, LISTBOX and EDITTEXT which
   do not allow resource 'text' field in control definition. */
static const rc_res_id res_null_text = { 1, {{0, &null_unichar}}};


#line 132 "rcparse.c" /* yacc.c:339  */

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
#ifndef YY_YY_RCPARSE_H_INCLUDED
# define YY_YY_RCPARSE_H_INCLUDED
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
    BEG = 258,
    END = 259,
    ACCELERATORS = 260,
    VIRTKEY = 261,
    ASCII = 262,
    NOINVERT = 263,
    SHIFT = 264,
    CONTROL = 265,
    ALT = 266,
    BITMAP = 267,
    CURSOR = 268,
    DIALOG = 269,
    DIALOGEX = 270,
    EXSTYLE = 271,
    CAPTION = 272,
    CLASS = 273,
    STYLE = 274,
    AUTO3STATE = 275,
    AUTOCHECKBOX = 276,
    AUTORADIOBUTTON = 277,
    CHECKBOX = 278,
    COMBOBOX = 279,
    CTEXT = 280,
    DEFPUSHBUTTON = 281,
    EDITTEXT = 282,
    GROUPBOX = 283,
    LISTBOX = 284,
    LTEXT = 285,
    PUSHBOX = 286,
    PUSHBUTTON = 287,
    RADIOBUTTON = 288,
    RTEXT = 289,
    SCROLLBAR = 290,
    STATE3 = 291,
    USERBUTTON = 292,
    BEDIT = 293,
    HEDIT = 294,
    IEDIT = 295,
    FONT = 296,
    ICON = 297,
    ANICURSOR = 298,
    ANIICON = 299,
    DLGINCLUDE = 300,
    DLGINIT = 301,
    FONTDIR = 302,
    HTML = 303,
    MANIFEST = 304,
    PLUGPLAY = 305,
    VXD = 306,
    TOOLBAR = 307,
    BUTTON = 308,
    LANGUAGE = 309,
    CHARACTERISTICS = 310,
    VERSIONK = 311,
    MENU = 312,
    MENUEX = 313,
    MENUITEM = 314,
    SEPARATOR = 315,
    POPUP = 316,
    CHECKED = 317,
    GRAYED = 318,
    HELP = 319,
    INACTIVE = 320,
    MENUBARBREAK = 321,
    MENUBREAK = 322,
    MESSAGETABLE = 323,
    RCDATA = 324,
    STRINGTABLE = 325,
    VERSIONINFO = 326,
    FILEVERSION = 327,
    PRODUCTVERSION = 328,
    FILEFLAGSMASK = 329,
    FILEFLAGS = 330,
    FILEOS = 331,
    FILETYPE = 332,
    FILESUBTYPE = 333,
    BLOCKSTRINGFILEINFO = 334,
    BLOCKVARFILEINFO = 335,
    VALUE = 336,
    BLOCK = 337,
    MOVEABLE = 338,
    FIXED = 339,
    PURE = 340,
    IMPURE = 341,
    PRELOAD = 342,
    LOADONCALL = 343,
    DISCARDABLE = 344,
    NOT = 345,
    QUOTEDUNISTRING = 346,
    QUOTEDSTRING = 347,
    STRING = 348,
    NUMBER = 349,
    SIZEDUNISTRING = 350,
    SIZEDSTRING = 351,
    IGNORED_TOKEN = 352,
    NEG = 353
  };
#endif
/* Tokens.  */
#define BEG 258
#define END 259
#define ACCELERATORS 260
#define VIRTKEY 261
#define ASCII 262
#define NOINVERT 263
#define SHIFT 264
#define CONTROL 265
#define ALT 266
#define BITMAP 267
#define CURSOR 268
#define DIALOG 269
#define DIALOGEX 270
#define EXSTYLE 271
#define CAPTION 272
#define CLASS 273
#define STYLE 274
#define AUTO3STATE 275
#define AUTOCHECKBOX 276
#define AUTORADIOBUTTON 277
#define CHECKBOX 278
#define COMBOBOX 279
#define CTEXT 280
#define DEFPUSHBUTTON 281
#define EDITTEXT 282
#define GROUPBOX 283
#define LISTBOX 284
#define LTEXT 285
#define PUSHBOX 286
#define PUSHBUTTON 287
#define RADIOBUTTON 288
#define RTEXT 289
#define SCROLLBAR 290
#define STATE3 291
#define USERBUTTON 292
#define BEDIT 293
#define HEDIT 294
#define IEDIT 295
#define FONT 296
#define ICON 297
#define ANICURSOR 298
#define ANIICON 299
#define DLGINCLUDE 300
#define DLGINIT 301
#define FONTDIR 302
#define HTML 303
#define MANIFEST 304
#define PLUGPLAY 305
#define VXD 306
#define TOOLBAR 307
#define BUTTON 308
#define LANGUAGE 309
#define CHARACTERISTICS 310
#define VERSIONK 311
#define MENU 312
#define MENUEX 313
#define MENUITEM 314
#define SEPARATOR 315
#define POPUP 316
#define CHECKED 317
#define GRAYED 318
#define HELP 319
#define INACTIVE 320
#define MENUBARBREAK 321
#define MENUBREAK 322
#define MESSAGETABLE 323
#define RCDATA 324
#define STRINGTABLE 325
#define VERSIONINFO 326
#define FILEVERSION 327
#define PRODUCTVERSION 328
#define FILEFLAGSMASK 329
#define FILEFLAGS 330
#define FILEOS 331
#define FILETYPE 332
#define FILESUBTYPE 333
#define BLOCKSTRINGFILEINFO 334
#define BLOCKVARFILEINFO 335
#define VALUE 336
#define BLOCK 337
#define MOVEABLE 338
#define FIXED 339
#define PURE 340
#define IMPURE 341
#define PRELOAD 342
#define LOADONCALL 343
#define DISCARDABLE 344
#define NOT 345
#define QUOTEDUNISTRING 346
#define QUOTEDSTRING 347
#define STRING 348
#define NUMBER 349
#define SIZEDUNISTRING 350
#define SIZEDSTRING 351
#define IGNORED_TOKEN 352
#define NEG 353

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 68 "rcparse.y" /* yacc.c:355  */

  rc_accelerator acc;
  rc_accelerator *pacc;
  rc_dialog_control *dialog_control;
  rc_menuitem *menuitem;
  struct
  {
    rc_rcdata_item *first;
    rc_rcdata_item *last;
  } rcdata;
  rc_rcdata_item *rcdata_item;
  rc_fixed_versioninfo *fixver;
  rc_ver_info *verinfo;
  rc_ver_stringtable *verstringtable;
  rc_ver_stringinfo *verstring;
  rc_ver_varinfo *vervar;
  rc_toolbar_item *toobar_item;
  rc_res_id id;
  rc_res_res_info res_info;
  struct
  {
    rc_uint_type on;
    rc_uint_type off;
  } memflags;
  struct
  {
    rc_uint_type val;
    /* Nonzero if this number was explicitly specified as long.  */
    int dword;
  } i;
  rc_uint_type il;
  rc_uint_type is;
  const char *s;
  struct
  {
    rc_uint_type length;
    const char *s;
  } ss;
  unichar *uni;
  struct
  {
    rc_uint_type length;
    const unichar *s;
  } suni;

#line 414 "rcparse.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_RCPARSE_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 431 "rcparse.c" /* yacc.c:358  */

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
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   830

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  112
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  102
/* YYNRULES -- Number of rules.  */
#define YYNRULES  276
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  520

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   353

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,   105,   100,     2,
     110,   111,   103,   101,   108,   102,     2,   104,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   109,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    99,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    98,     2,   106,     2,     2,     2,
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
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,   107
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   178,   178,   180,   181,   182,   183,   184,   185,   186,
     187,   188,   189,   190,   191,   192,   193,   194,   200,   211,
     214,   235,   240,   252,   272,   282,   286,   291,   298,   302,
     307,   311,   315,   319,   328,   340,   354,   352,   379,   377,
     406,   404,   436,   439,   445,   447,   453,   457,   462,   466,
     470,   483,   498,   513,   528,   532,   536,   540,   546,   548,
     560,   559,   572,   571,   584,   583,   596,   595,   611,   610,
     623,   622,   636,   647,   657,   656,   669,   668,   681,   680,
     693,   692,   705,   704,   719,   724,   730,   736,   743,   742,
     758,   757,   770,   769,   782,   781,   793,   792,   805,   804,
     817,   816,   829,   828,   841,   840,   854,   852,   873,   884,
     895,   907,   918,   921,   925,   930,   940,   943,   953,   952,
     959,   958,   965,   964,   972,   984,   997,  1006,  1017,  1020,
    1037,  1041,  1045,  1053,  1056,  1060,  1067,  1071,  1075,  1079,
    1083,  1087,  1096,  1107,  1110,  1127,  1131,  1135,  1139,  1143,
    1147,  1151,  1155,  1165,  1178,  1178,  1190,  1194,  1201,  1209,
    1217,  1225,  1234,  1243,  1252,  1262,  1261,  1266,  1268,  1273,
    1278,  1286,  1290,  1295,  1300,  1305,  1310,  1315,  1320,  1325,
    1330,  1341,  1348,  1358,  1364,  1365,  1384,  1409,  1420,  1425,
    1432,  1439,  1444,  1449,  1454,  1459,  1474,  1477,  1481,  1489,
    1492,  1500,  1503,  1511,  1514,  1523,  1528,  1537,  1541,  1551,
    1556,  1560,  1571,  1577,  1583,  1588,  1593,  1604,  1609,  1621,
    1626,  1638,  1643,  1648,  1653,  1658,  1663,  1668,  1678,  1682,
    1690,  1695,  1710,  1714,  1723,  1727,  1739,  1744,  1760,  1764,
    1776,  1780,  1802,  1806,  1810,  1814,  1821,  1825,  1835,  1838,
    1847,  1856,  1865,  1869,  1873,  1878,  1883,  1888,  1893,  1898,
    1903,  1908,  1913,  1918,  1929,  1938,  1949,  1953,  1957,  1962,
    1967,  1972,  1978,  1983,  1988,  1993,  1998
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "BEG", "END", "ACCELERATORS", "VIRTKEY",
  "ASCII", "NOINVERT", "SHIFT", "CONTROL", "ALT", "BITMAP", "CURSOR",
  "DIALOG", "DIALOGEX", "EXSTYLE", "CAPTION", "CLASS", "STYLE",
  "AUTO3STATE", "AUTOCHECKBOX", "AUTORADIOBUTTON", "CHECKBOX", "COMBOBOX",
  "CTEXT", "DEFPUSHBUTTON", "EDITTEXT", "GROUPBOX", "LISTBOX", "LTEXT",
  "PUSHBOX", "PUSHBUTTON", "RADIOBUTTON", "RTEXT", "SCROLLBAR", "STATE3",
  "USERBUTTON", "BEDIT", "HEDIT", "IEDIT", "FONT", "ICON", "ANICURSOR",
  "ANIICON", "DLGINCLUDE", "DLGINIT", "FONTDIR", "HTML", "MANIFEST",
  "PLUGPLAY", "VXD", "TOOLBAR", "BUTTON", "LANGUAGE", "CHARACTERISTICS",
  "VERSIONK", "MENU", "MENUEX", "MENUITEM", "SEPARATOR", "POPUP",
  "CHECKED", "GRAYED", "HELP", "INACTIVE", "MENUBARBREAK", "MENUBREAK",
  "MESSAGETABLE", "RCDATA", "STRINGTABLE", "VERSIONINFO", "FILEVERSION",
  "PRODUCTVERSION", "FILEFLAGSMASK", "FILEFLAGS", "FILEOS", "FILETYPE",
  "FILESUBTYPE", "BLOCKSTRINGFILEINFO", "BLOCKVARFILEINFO", "VALUE",
  "BLOCK", "MOVEABLE", "FIXED", "PURE", "IMPURE", "PRELOAD", "LOADONCALL",
  "DISCARDABLE", "NOT", "QUOTEDUNISTRING", "QUOTEDSTRING", "STRING",
  "NUMBER", "SIZEDUNISTRING", "SIZEDSTRING", "IGNORED_TOKEN", "'|'", "'^'",
  "'&'", "'+'", "'-'", "'*'", "'/'", "'%'", "'~'", "NEG", "','", "'='",
  "'('", "')'", "$accept", "input", "accelerator", "acc_entries",
  "acc_entry", "acc_event", "acc_options", "acc_option", "bitmap",
  "cursor", "dialog", "$@1", "$@2", "$@3", "exstyle", "styles", "controls",
  "control", "$@4", "$@5", "$@6", "$@7", "$@8", "$@9", "$@10", "$@11",
  "$@12", "$@13", "$@14", "$@15", "$@16", "$@17", "$@18", "$@19", "$@20",
  "$@21", "$@22", "$@23", "$@24", "control_params", "cresid", "optresidc",
  "resid", "opt_control_data", "control_styleexpr", "$@25",
  "icon_styleexpr", "$@26", "control_params_styleexpr", "$@27", "font",
  "icon", "language", "menu", "menuitems", "menuitem", "menuitem_flags",
  "menuitem_flag", "menuex", "menuexitems", "menuexitem", "messagetable",
  "optrcdata_data", "$@28", "optrcdata_data_int", "rcdata_data",
  "stringtable", "$@29", "string_data", "rcdata_id", "user", "toolbar",
  "toolbar_data", "versioninfo", "fixedverinfo", "verblocks",
  "verstringtables", "vervals", "vertrans", "id", "resname", "resref",
  "suboptions", "memflags_move_discard", "memflags_move", "memflag",
  "file_name", "res_unicode_string_concat", "res_unicode_string",
  "res_unicode_sizedstring", "res_unicode_sizedstring_concat",
  "sizedstring", "sizedunistring", "styleexpr", "parennumber",
  "optcnumexpr", "cnumexpr", "numexpr", "sizednumexpr", "cposnumexpr",
  "posnumexpr", "sizedposnumexpr", YY_NULLPTR
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
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   124,    94,
      38,    43,    45,    42,    47,    37,   126,   353,    44,    61,
      40,    41
};
# endif

#define YYPACT_NINF -446

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-446)))

#define YYTABLE_NINF -231

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -446,    75,  -446,   317,  -446,  -446,  -446,  -446,  -446,  -446,
     317,   317,  -446,  -446,  -446,  -446,  -446,  -446,  -446,  -446,
    -446,  -446,  -446,  -446,  -446,  -446,   463,  -446,  -446,  -446,
     589,  -446,   317,   317,   317,   -93,   626,   209,  -446,   437,
    -446,  -446,  -446,  -446,  -446,  -446,  -446,  -446,  -446,  -446,
    -446,  -446,  -446,  -446,  -446,  -446,  -446,  -446,  -446,  -446,
    -446,  -446,  -446,   317,   317,   317,   317,   317,   317,   317,
     317,  -446,  -446,   526,   317,  -446,   317,   317,   317,   317,
     317,   317,   317,   317,  -446,   317,   317,   317,  -446,  -446,
    -446,  -446,  -446,  -446,  -446,  -446,  -446,   267,   675,   675,
     275,   275,   675,   675,   491,   404,   441,   675,   168,   256,
     719,   379,   397,   213,   213,  -446,  -446,  -446,  -446,  -446,
     719,   379,   397,   213,   213,  -446,  -446,  -446,  -446,   -93,
    -446,  -446,  -446,  -446,  -446,  -446,  -446,  -446,  -446,   -65,
     144,   144,  -446,  -446,   -93,  -446,  -446,  -446,  -446,   317,
     317,   317,   317,   317,   317,   317,  -446,  -446,    18,  -446,
      21,   317,   -93,   -93,    31,   140,   155,   126,   -93,   -93,
    -446,  -446,  -446,  -446,  -446,    47,   177,  -446,  -446,   212,
    -446,  -446,  -446,   -34,  -446,  -446,   -93,   -93,  -446,  -446,
     -36,    -5,  -446,  -446,   -25,    -5,  -446,  -446,   119,   131,
     -93,  -446,   -93,  -446,  -446,  -446,  -446,    54,    68,    84,
     626,     2,  -446,     2,    68,    84,   144,    87,   -93,   -93,
      25,  -446,    95,  -446,    -5,  -446,    95,    62,  -446,   102,
     -93,   -93,   177,  -446,  -446,     2,  -446,  -446,   552,  -446,
     -93,  -446,   306,  -446,  -446,  -446,    76,   -93,  -446,     8,
       6,    -5,  -446,  -446,    68,    84,   626,  -446,  -446,  -446,
    -446,  -446,  -446,   167,  -446,  -446,  -446,  -446,  -446,   271,
    -446,  -446,  -446,  -446,  -446,  -446,  -446,   763,  -446,   -93,
     161,  -446,    11,  -446,   197,    -5,   552,  -446,   374,   548,
    -446,   178,  -446,  -446,  -446,   190,  -446,   -93,  -446,     3,
    -446,  -446,   317,    -5,   306,   -47,   317,   317,   317,   317,
     306,  -446,   565,  -446,  -446,   194,   201,    -1,  -446,   -93,
     639,  -446,    -5,  -446,    -5,   143,   -33,  -446,   317,   110,
    -446,   105,   -93,  -446,  -446,  -446,   676,  -446,  -446,  -446,
    -446,    -5,  -446,  -446,   311,   311,   311,   311,   311,  -446,
     311,   311,  -446,   311,  -446,   311,   311,   311,   311,   311,
    -446,   311,   306,   311,   311,   311,   306,  -446,  -446,   104,
     -42,    -5,  -446,  -446,   713,   207,    99,   317,   113,    -5,
    -446,  -446,  -446,  -446,  -446,   317,  -446,  -446,   317,  -446,
     317,  -446,  -446,  -446,  -446,  -446,   317,  -446,   115,   317,
     120,  -446,  -446,  -446,   317,  -446,   -33,  -446,    95,  -446,
    -446,    -5,   152,  -446,   317,   317,   317,   317,  -446,   -93,
     317,   317,  -446,   317,  -446,   317,   317,   317,   317,   317,
    -446,   317,  -446,   153,  -446,   317,   317,   317,   -93,  -446,
     -93,    -5,   311,   159,  -446,  -446,  -446,  -446,   -93,  -446,
    -446,  -446,  -446,  -446,  -446,  -446,  -446,  -446,   317,  -446,
    -446,  -446,   -93,   -93,  -446,  -446,   -93,   -93,   173,    15,
    -446,   -47,   -93,   -93,   317,  -446,  -446,   -93,   110,   -93,
      27,   180,   244,    29,   -93,  -446,  -446,   -93,   317,  -446,
    -446,  -446,   -93,   -93,   -47,   273,   -93,   192,   -47,   273,
     -93,   273,   -93,   110,  -446,   273,   317,   110,  -446,   273,
    -446,   273,  -446,   193,  -446,  -446,  -446,   -47,   -75,  -446
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       2,     0,     1,     0,   212,   232,   233,   208,   266,    17,
       0,     0,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    15,    14,    16,     0,   206,   207,   205,
     265,   252,     0,     0,     0,     0,   251,     0,   268,     0,
     212,   219,   217,   219,   219,   217,   217,   179,   180,   177,
     178,   172,   174,   175,   176,   212,   212,   212,   219,   173,
     188,   212,   171,     0,     0,     0,     0,     0,     0,     0,
       0,   255,   254,     0,     0,   126,     0,     0,     0,     0,
       0,     0,     0,     0,   165,     0,     0,     0,   221,   222,
     223,   224,   225,   226,   227,   213,   267,     0,     0,     0,
      42,    42,     0,     0,     0,     0,     0,     0,     0,     0,
     276,   275,   274,   272,   273,   269,   270,   271,   253,   250,
     263,   262,   261,   259,   260,   256,   257,   258,   167,     0,
     214,   216,    19,   228,   229,   220,    34,   218,    35,     0,
       0,     0,   124,   125,     0,   128,   143,   153,   196,     0,
       0,     0,     0,     0,     0,     0,   154,   182,     0,   215,
       0,     0,     0,     0,     0,     0,     0,     0,   248,   248,
     191,   192,   193,   194,   195,     0,   156,   170,   166,     0,
      18,    23,    20,     0,    24,    43,     0,     0,   184,   127,
       0,     0,   129,   142,     0,     0,   144,   187,     0,     0,
     248,   249,   248,   181,   240,   238,   155,   157,   158,   159,
     160,     0,   236,   168,   235,   234,     0,    21,     0,     0,
       0,   131,     0,   230,   133,   148,   145,     0,   199,     0,
     248,   248,   164,   239,   241,   169,   237,   264,     0,    36,
      38,   183,     0,   186,   231,   133,     0,   146,   143,     0,
       0,     0,   189,   190,   161,   162,   163,    28,    29,    30,
      31,    32,    33,    22,    25,    44,    44,    40,   185,   130,
     128,   136,   137,   138,   139,   140,   141,     0,   135,   248,
       0,   143,     0,   197,     0,   203,     0,    27,     0,     0,
      44,     0,   134,   147,   149,     0,   143,   248,   201,     0,
      26,    58,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    58,     0,   132,   150,     0,     0,     0,   198,     0,
       0,    48,    45,    46,    49,   207,     0,   246,     0,    47,
     242,     0,     0,    55,    57,    54,     0,    58,   151,   143,
     200,     0,   204,    37,   112,   112,   112,   112,   112,    70,
     112,   112,    78,   112,    90,   112,   112,   112,   112,   112,
     102,   112,     0,   112,   112,   112,     0,    59,   243,     0,
       0,     0,    56,    39,     0,     0,     0,     0,     0,   115,
     114,    60,    62,    64,    68,     0,    74,    76,     0,    80,
       0,    92,    94,    96,    98,   100,     0,   104,   210,     0,
       0,    66,    82,    88,     0,   247,     0,   244,    50,    41,
     152,     0,     0,   113,     0,     0,     0,     0,    71,     0,
       0,     0,    79,     0,    91,     0,     0,     0,     0,     0,
     103,     0,   211,     0,   209,     0,     0,     0,     0,   245,
      51,   202,     0,     0,    61,    63,    65,    69,     0,    75,
      77,    81,    93,    95,    97,    99,   101,   105,     0,    67,
      83,    89,     0,    52,   111,   118,     0,     0,     0,   116,
      53,     0,     0,     0,     0,   154,    84,     0,   119,     0,
     116,     0,     0,   116,     0,   122,   108,   248,     0,   117,
     120,    85,   248,   248,     0,   116,   249,     0,     0,   116,
     249,   116,   249,   123,   109,   116,     0,   121,    86,   116,
      72,   116,   110,     0,    87,    73,   106,     0,   248,   107
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -446,  -446,  -446,  -446,  -446,  -446,  -446,  -236,  -446,  -446,
    -446,  -446,  -446,  -446,   184,  -262,  -273,  -446,  -446,  -446,
    -446,  -446,  -446,  -446,  -446,  -446,  -446,  -446,  -446,  -446,
    -446,  -446,  -446,  -446,  -446,  -446,  -446,  -446,  -446,   219,
    -446,   442,  -123,   274,  -446,  -446,  -446,  -446,  -446,  -446,
    -446,  -446,  -446,  -446,    77,  -446,   101,    88,  -446,  -239,
    -446,  -446,  -109,  -446,  -446,  -446,  -446,  -446,  -446,  -446,
    -446,  -446,  -446,  -446,  -446,  -446,  -446,  -446,  -446,   -24,
    -245,     4,   169,   211,   270,   710,   175,  -178,     5,  -173,
     157,  -156,  -122,  -445,  -325,  -161,   -30,    -3,    26,  -446,
      20,  -446
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    12,   160,   182,   183,   263,   264,    13,    14,
      15,   265,   266,   290,   140,   288,   320,   367,   414,   415,
     416,   435,   417,   385,   420,   421,   388,   423,   436,   437,
     390,   425,   426,   427,   428,   429,   396,   431,   517,   418,
     443,   377,   378,   476,   466,   471,   492,   498,   487,   494,
      16,    17,    18,    19,   165,   192,   246,   278,    20,   166,
     196,    21,   175,   176,   206,   207,    22,   128,   158,    61,
      23,    24,   220,    25,   108,   167,   250,   317,   299,    26,
      27,   399,    37,    99,    98,    95,   136,   379,   223,   212,
     213,   214,   215,   329,   330,   200,   201,   419,    36,   217,
     380,    30
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      35,   368,    62,   340,   289,    75,    28,   318,   202,   280,
     283,   281,   222,   224,   296,    74,   226,   227,   475,   177,
     208,    29,   178,   370,   221,   180,   478,   287,   312,   241,
     475,    28,   475,    74,   188,   225,    38,    39,   336,   230,
     236,   231,   295,   326,   161,   407,    29,   327,   406,   503,
     300,   203,   327,   507,   209,     5,     6,   315,    71,    72,
      73,   327,   236,   328,   374,   248,     5,     6,   328,   252,
     253,   119,   518,   285,   216,     2,   254,   328,   242,   270,
     341,   439,   129,   130,   131,   243,     5,     6,   284,   110,
     111,   112,   113,   114,   115,   116,   117,   204,   205,   159,
     375,   144,   120,   121,   122,   123,   124,   125,   126,   127,
     255,    74,    31,   181,   164,     8,    74,   398,   293,    74,
      32,   398,   228,    74,    33,   322,   324,    10,    34,     3,
     197,    11,   186,   187,   229,   485,   316,   490,   271,   272,
     273,   274,   275,   276,   189,     4,   168,   169,   170,   171,
     172,   173,   174,     5,     6,   179,   218,   219,   185,   193,
     162,   163,   232,   376,   233,   294,     5,     6,     7,     8,
      74,   148,     9,   257,   258,   259,   260,   261,   262,   234,
     184,    10,   313,   251,   277,    11,     5,     6,   239,   240,
       5,     6,   245,   408,   314,   238,   247,   249,   338,   190,
     298,   191,   210,    74,   339,   198,   199,   411,   370,    97,
     267,   410,    84,   371,   194,   405,   195,   279,   268,   282,
     194,   413,   195,   432,   104,   105,   106,   244,   434,   244,
     109,   244,   244,   441,  -230,  -230,   237,   190,     8,   191,
     149,   150,   151,   152,   153,   154,   155,    28,   489,   194,
      10,   195,   297,   194,    11,   195,   102,   103,   256,   156,
     442,   458,    29,    85,    86,    87,   194,   465,   195,   319,
     132,    31,   204,   205,   138,   286,   475,   142,   143,    32,
     323,   474,   147,    33,   157,   141,   335,    34,   488,   342,
     244,   139,    88,    89,    90,    91,    92,    93,    94,   321,
     506,   516,   372,   331,   332,   333,   334,   204,   205,   325,
      85,    86,    87,   100,   101,    28,    81,    82,    83,   464,
     211,    85,    86,    87,    29,   369,   495,   244,   107,   244,
      29,   499,   501,   271,   272,   273,   274,   275,   276,    88,
      89,    90,    91,    92,    93,    94,   269,   291,   133,   134,
      88,    89,    90,    91,    92,    93,    94,   519,    88,    89,
      90,    91,    92,    93,    94,   292,   482,    28,   235,     0,
     404,    28,     0,     0,   412,     0,     0,   301,   440,   277,
       0,   244,   400,     0,   244,     0,   400,     0,     0,   448,
     302,   303,   304,   305,     0,     0,   433,     5,     6,     7,
       8,   438,     5,     6,     0,     8,     0,   145,   462,     0,
     463,    31,    10,   244,     0,   306,    11,    10,   467,    32,
       0,    11,     0,    33,     0,     0,     0,    34,   307,   308,
     309,   310,   469,   470,     0,     0,   472,   473,     0,   477,
       0,     0,   479,   480,   146,     0,   244,   483,     0,   484,
       0,     0,     0,     0,   493,   468,     0,   496,    85,    86,
      87,     0,   500,   502,     0,     0,   505,     0,    40,     0,
     509,   481,   511,     0,     0,    41,    42,    43,    44,    78,
      79,    80,    81,    82,    83,   497,     0,    88,    89,    90,
      91,    92,    93,    94,     0,    85,    86,    87,    79,    80,
      81,    82,    83,   513,    45,    46,    47,    48,    49,    50,
       0,    51,    52,    53,    54,    55,     0,     0,     0,     0,
      56,    57,     0,     0,    88,    89,    90,    91,    92,    93,
      94,    58,    59,     0,    60,    76,    77,    78,    79,    80,
      81,    82,    83,     0,     0,    85,    86,    87,    96,     0,
       0,   311,     0,     0,     5,     6,     7,     8,   257,   258,
     259,   260,   261,   262,   302,   303,   304,   305,   337,    10,
       0,     0,     0,    11,    88,    89,    90,    91,    92,    93,
      94,   302,   303,   304,   305,    31,     0,     0,     0,   306,
       0,     0,     0,    32,     0,     0,     0,    33,     0,     0,
       0,    34,   307,   308,   309,   310,   306,   422,     0,   424,
       0,     0,     0,     0,     0,   430,     0,     0,     0,   307,
     308,   309,   310,     0,    76,    77,    78,    79,    80,    81,
      82,    83,     0,   444,   445,   446,   447,   118,     0,   449,
     450,     0,   451,   343,   452,   453,   454,   455,   456,   344,
     457,     0,     0,     0,   459,   460,   461,     0,     0,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,   355,
     356,   357,   358,   359,   360,   361,   362,   363,   364,   365,
     373,   366,     0,     0,     0,     0,   344,    63,    64,    65,
      66,    67,    68,    69,    70,     0,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   409,   366,     0,
       0,     0,     0,   344,    76,    77,    78,    79,    80,    81,
      82,    83,     0,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,   355,   356,   357,   358,   359,   360,   361,
     362,   363,   364,   365,   486,   366,     0,   491,    88,    89,
      90,    91,    92,    93,    94,     0,     0,   133,   134,   504,
       0,     0,     0,   508,     0,   510,     0,     0,     0,   512,
       0,     0,     0,   514,     0,   515,     0,   381,   382,   383,
     384,     0,   386,   387,     0,   389,     0,   391,   392,   393,
     394,   395,     0,   397,     0,   401,   402,   403,   135,   137,
     135,   135,   137,   137,     0,     0,     0,   135,    77,    78,
      79,    80,    81,    82,    83,   271,   272,   273,   274,   275,
     276
};

static const yytype_int16 yycheck[] =
{
       3,   326,    26,     4,   266,    35,     1,     4,   169,   248,
       4,     3,   190,   191,     3,   108,   194,   195,     3,     1,
     176,     1,     4,    98,    60,     4,   471,   263,   290,     4,
       3,    26,     3,   108,     3,    60,    10,    11,   311,   200,
     213,   202,   281,    90,   109,   370,    26,    94,    90,   494,
     286,     4,    94,   498,   176,    91,    92,   296,    32,    33,
      34,    94,   235,   110,   337,     3,    91,    92,   110,   230,
     231,    74,   517,   251,   108,     0,   232,   110,    53,     3,
      81,   406,    85,    86,    87,    60,    91,    92,    82,    63,
      64,    65,    66,    67,    68,    69,    70,    95,    96,   129,
     339,   104,    76,    77,    78,    79,    80,    81,    82,    83,
     232,   108,    94,    92,   144,    94,   108,   362,   279,   108,
     102,   366,     3,   108,   106,   303,   304,   106,   110,    54,
       4,   110,   162,   163,     3,   108,   297,   108,    62,    63,
      64,    65,    66,    67,     4,    70,   149,   150,   151,   152,
     153,   154,   155,    91,    92,   158,   186,   187,   161,     4,
     140,   141,   108,   341,    96,     4,    91,    92,    93,    94,
     108,     3,    97,     6,     7,     8,     9,    10,    11,    95,
     160,   106,     4,    81,   108,   110,    91,    92,   218,   219,
      91,    92,   222,   371,     4,   108,   226,   227,     4,    59,
       3,    61,   176,   108,     3,    79,    80,   108,    98,    40,
     240,     4,     3,   108,    59,   111,    61,   247,   242,   249,
      59,   108,    61,   108,    55,    56,    57,   222,   108,   224,
      61,   226,   227,   411,    91,    92,   216,    59,    94,    61,
      72,    73,    74,    75,    76,    77,    78,   242,     4,    59,
     106,    61,   282,    59,   110,    61,    45,    46,   232,     3,
     108,   108,   242,    54,    55,    56,    59,   108,    61,   299,
       3,    94,    95,    96,    99,   108,     3,   102,   103,   102,
     304,   108,   107,   106,   109,   101,   310,   110,   108,   319,
     285,    16,    83,    84,    85,    86,    87,    88,    89,   302,
     108,   108,   332,   306,   307,   308,   309,    95,    96,   304,
      54,    55,    56,    43,    44,   310,   103,   104,   105,   442,
     108,    54,    55,    56,   304,   328,   487,   322,    58,   324,
     310,   492,   493,    62,    63,    64,    65,    66,    67,    83,
      84,    85,    86,    87,    88,    89,   245,   270,    92,    93,
      83,    84,    85,    86,    87,    88,    89,   518,    83,    84,
      85,    86,    87,    88,    89,   277,   475,   362,   211,    -1,
     366,   366,    -1,    -1,   377,    -1,    -1,     3,   408,   108,
      -1,   376,   362,    -1,   379,    -1,   366,    -1,    -1,   419,
      16,    17,    18,    19,    -1,    -1,   399,    91,    92,    93,
      94,   404,    91,    92,    -1,    94,    -1,     3,   438,    -1,
     440,    94,   106,   408,    -1,    41,   110,   106,   448,   102,
      -1,   110,    -1,   106,    -1,    -1,    -1,   110,    54,    55,
      56,    57,   462,   463,    -1,    -1,   466,   467,    -1,   469,
      -1,    -1,   472,   473,     3,    -1,   441,   477,    -1,   479,
      -1,    -1,    -1,    -1,   484,   458,    -1,   487,    54,    55,
      56,    -1,   492,   493,    -1,    -1,   496,    -1,     5,    -1,
     500,   474,   502,    -1,    -1,    12,    13,    14,    15,   100,
     101,   102,   103,   104,   105,   488,    -1,    83,    84,    85,
      86,    87,    88,    89,    -1,    54,    55,    56,   101,   102,
     103,   104,   105,   506,    41,    42,    43,    44,    45,    46,
      -1,    48,    49,    50,    51,    52,    -1,    -1,    -1,    -1,
      57,    58,    -1,    -1,    83,    84,    85,    86,    87,    88,
      89,    68,    69,    -1,    71,    98,    99,   100,   101,   102,
     103,   104,   105,    -1,    -1,    54,    55,    56,   111,    -1,
      -1,     3,    -1,    -1,    91,    92,    93,    94,     6,     7,
       8,     9,    10,    11,    16,    17,    18,    19,     3,   106,
      -1,    -1,    -1,   110,    83,    84,    85,    86,    87,    88,
      89,    16,    17,    18,    19,    94,    -1,    -1,    -1,    41,
      -1,    -1,    -1,   102,    -1,    -1,    -1,   106,    -1,    -1,
      -1,   110,    54,    55,    56,    57,    41,   388,    -1,   390,
      -1,    -1,    -1,    -1,    -1,   396,    -1,    -1,    -1,    54,
      55,    56,    57,    -1,    98,    99,   100,   101,   102,   103,
     104,   105,    -1,   414,   415,   416,   417,   111,    -1,   420,
     421,    -1,   423,     4,   425,   426,   427,   428,   429,    10,
     431,    -1,    -1,    -1,   435,   436,   437,    -1,    -1,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
       4,    42,    -1,    -1,    -1,    -1,    10,    98,    99,   100,
     101,   102,   103,   104,   105,    -1,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,     4,    42,    -1,
      -1,    -1,    -1,    10,    98,    99,   100,   101,   102,   103,
     104,   105,    -1,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,   480,    42,    -1,   483,    83,    84,
      85,    86,    87,    88,    89,    -1,    -1,    92,    93,   495,
      -1,    -1,    -1,   499,    -1,   501,    -1,    -1,    -1,   505,
      -1,    -1,    -1,   509,    -1,   511,    -1,   345,   346,   347,
     348,    -1,   350,   351,    -1,   353,    -1,   355,   356,   357,
     358,   359,    -1,   361,    -1,   363,   364,   365,    98,    99,
     100,   101,   102,   103,    -1,    -1,    -1,   107,    99,   100,
     101,   102,   103,   104,   105,    62,    63,    64,    65,    66,
      67
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   113,     0,    54,    70,    91,    92,    93,    94,    97,
     106,   110,   114,   120,   121,   122,   162,   163,   164,   165,
     170,   173,   178,   182,   183,   185,   191,   192,   200,   212,
     213,    94,   102,   106,   110,   209,   210,   194,   210,   210,
       5,    12,    13,    14,    15,    41,    42,    43,    44,    45,
      46,    48,    49,    50,    51,    52,    57,    58,    68,    69,
      71,   181,   191,    98,    99,   100,   101,   102,   103,   104,
     105,   210,   210,   210,   108,   208,    98,    99,   100,   101,
     102,   103,   104,   105,     3,    54,    55,    56,    83,    84,
      85,    86,    87,    88,    89,   197,   111,   194,   196,   195,
     196,   196,   195,   195,   194,   194,   194,   196,   186,   194,
     210,   210,   210,   210,   210,   210,   210,   210,   111,   209,
     210,   210,   210,   210,   210,   210,   210,   210,   179,   209,
     209,   209,     3,    92,    93,   197,   198,   197,   198,    16,
     126,   126,   198,   198,   209,     3,     3,   198,     3,    72,
      73,    74,    75,    76,    77,    78,     3,   198,   180,   208,
     115,   109,   212,   212,   208,   166,   171,   187,   209,   209,
     209,   209,   209,   209,   209,   174,   175,     1,     4,   209,
       4,    92,   116,   117,   212,   209,   208,   208,     3,     4,
      59,    61,   167,     4,    59,    61,   172,     4,    79,    80,
     207,   208,   207,     4,    95,    96,   176,   177,   203,   204,
     210,   108,   201,   202,   203,   204,   108,   211,   208,   208,
     184,    60,   199,   200,   199,    60,   199,   199,     3,     3,
     207,   207,   108,    96,    95,   202,   201,   212,   108,   208,
     208,     4,    53,    60,   200,   208,   168,   208,     3,   208,
     188,    81,   207,   207,   203,   204,   210,     6,     7,     8,
       9,    10,    11,   118,   119,   123,   124,   208,   191,   168,
       3,    62,    63,    64,    65,    66,    67,   108,   169,   208,
     171,     3,   208,     4,    82,   199,   108,   119,   127,   127,
     125,   166,   169,   207,     4,   171,     3,   208,     3,   190,
     119,     3,    16,    17,    18,    19,    41,    54,    55,    56,
      57,     3,   127,     4,     4,   171,   207,   189,     4,   208,
     128,   209,   199,   191,   199,   200,    90,    94,   110,   205,
     206,   209,   209,   209,   209,   191,   128,     3,     4,     3,
       4,    81,   208,     4,    10,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    42,   129,   206,   209,
      98,   108,   208,     4,   128,   171,   199,   153,   154,   199,
     212,   153,   153,   153,   153,   135,   153,   153,   138,   153,
     142,   153,   153,   153,   153,   153,   148,   153,   192,   193,
     212,   153,   153,   153,   193,   111,    90,   206,   199,     4,
       4,   108,   209,   108,   130,   131,   132,   134,   151,   209,
     136,   137,   151,   139,   151,   143,   144,   145,   146,   147,
     151,   149,   108,   209,   108,   133,   140,   141,   209,   206,
     208,   199,   108,   152,   151,   151,   151,   151,   208,   151,
     151,   151,   151,   151,   151,   151,   151,   151,   108,   151,
     151,   151,   208,   208,   154,   108,   156,   208,   209,   208,
     208,   157,   208,   208,   108,     3,   155,   208,   205,   208,
     208,   209,   174,   208,   208,   108,   155,   160,   108,     4,
     108,   155,   158,   208,   161,   207,   208,   209,   159,   207,
     208,   207,   208,   205,   155,   208,   108,   205,   155,   208,
     155,   208,   155,   209,   155,   155,   108,   150,   205,   207
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   112,   113,   113,   113,   113,   113,   113,   113,   113,
     113,   113,   113,   113,   113,   113,   113,   113,   114,   115,
     115,   116,   116,   117,   117,   118,   118,   118,   119,   119,
     119,   119,   119,   119,   120,   121,   123,   122,   124,   122,
     125,   122,   126,   126,   127,   127,   127,   127,   127,   127,
     127,   127,   127,   127,   127,   127,   127,   127,   128,   128,
     130,   129,   131,   129,   132,   129,   133,   129,   134,   129,
     135,   129,   129,   129,   136,   129,   137,   129,   138,   129,
     139,   129,   140,   129,   129,   129,   129,   129,   141,   129,
     142,   129,   143,   129,   144,   129,   145,   129,   146,   129,
     147,   129,   148,   129,   149,   129,   150,   129,   151,   151,
     151,   152,   153,   153,   154,   154,   155,   155,   157,   156,
     159,   158,   161,   160,   162,   163,   164,   165,   166,   166,
     167,   167,   167,   168,   168,   168,   169,   169,   169,   169,
     169,   169,   170,   171,   171,   172,   172,   172,   172,   172,
     172,   172,   172,   173,   175,   174,   176,   176,   177,   177,
     177,   177,   177,   177,   177,   179,   178,   180,   180,   180,
     180,   181,   181,   181,   181,   181,   181,   181,   181,   181,
     181,   182,   182,   183,   184,   184,   184,   185,   186,   186,
     186,   186,   186,   186,   186,   186,   187,   187,   187,   188,
     188,   189,   189,   190,   190,   191,   191,   192,   192,   193,
     193,   193,   194,   194,   194,   194,   194,   195,   195,   196,
     196,   197,   197,   197,   197,   197,   197,   197,   198,   198,
     199,   199,   200,   200,   201,   201,   202,   202,   203,   203,
     204,   204,   205,   205,   205,   205,   206,   206,   207,   207,
     208,   209,   210,   210,   210,   210,   210,   210,   210,   210,
     210,   210,   210,   210,   211,   212,   213,   213,   213,   213,
     213,   213,   213,   213,   213,   213,   213
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     6,     0,
       2,     2,     4,     1,     1,     1,     3,     2,     1,     1,
       1,     1,     1,     1,     4,     4,     0,    13,     0,    13,
       0,    14,     0,     3,     0,     3,     3,     3,     3,     3,
       5,     6,     7,     8,     3,     3,     4,     3,     0,     2,
       0,     4,     0,     4,     0,     4,     0,     4,     0,     4,
       0,     3,    11,    12,     0,     4,     0,     4,     0,     3,
       0,     4,     0,     4,     6,     8,    10,    11,     0,     4,
       0,     3,     0,     4,     0,     4,     0,     4,     0,     4,
       0,     4,     0,     3,     0,     4,     0,    15,     6,     8,
       9,     2,     0,     2,     1,     1,     0,     3,     0,     3,
       0,     3,     0,     3,     4,     4,     3,     6,     0,     2,
       4,     2,     6,     0,     3,     2,     1,     1,     1,     1,
       1,     1,     6,     0,     2,     2,     3,     5,     2,     5,
       6,     7,     9,     4,     0,     2,     0,     1,     1,     1,
       1,     3,     3,     3,     2,     0,     6,     0,     3,     4,
       2,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     6,     4,     8,     0,     3,     2,     6,     0,     6,
       6,     3,     3,     3,     3,     3,     0,     5,     7,     0,
       5,     0,     5,     0,     3,     1,     1,     1,     1,     2,
       1,     2,     0,     2,     3,     4,     3,     0,     2,     0,
       2,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     2,     1,     1,     1,     1,     1,     2,     1,     2,
       1,     2,     1,     2,     3,     4,     1,     3,     0,     1,
       2,     1,     1,     3,     2,     2,     3,     3,     3,     3,
       3,     3,     3,     3,     2,     1,     1,     3,     2,     3,
       3,     3,     3,     3,     3,     3,     3
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
    default: /* Avoid compiler warnings. */
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
        case 18:
#line 201 "rcparse.y" /* yacc.c:1648  */
    {
	    define_accelerator ((yyvsp[-5].id), &(yyvsp[-3].res_info), (yyvsp[-1].pacc));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 1979 "rcparse.c" /* yacc.c:1648  */
    break;

  case 19:
#line 211 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.pacc) = NULL;
	  }
#line 1987 "rcparse.c" /* yacc.c:1648  */
    break;

  case 20:
#line 215 "rcparse.y" /* yacc.c:1648  */
    {
	    rc_accelerator *a;

	    a = (rc_accelerator *) res_alloc (sizeof *a);
	    *a = (yyvsp[0].acc);
	    if ((yyvsp[-1].pacc) == NULL)
	      (yyval.pacc) = a;
	    else
	      {
		rc_accelerator **pp;

		for (pp = &(yyvsp[-1].pacc)->next; *pp != NULL; pp = &(*pp)->next)
		  ;
		*pp = a;
		(yyval.pacc) = (yyvsp[-1].pacc);
	      }
	  }
#line 2009 "rcparse.c" /* yacc.c:1648  */
    break;

  case 21:
#line 236 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.acc) = (yyvsp[-1].acc);
	    (yyval.acc).id = (yyvsp[0].il);
	  }
#line 2018 "rcparse.c" /* yacc.c:1648  */
    break;

  case 22:
#line 241 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.acc) = (yyvsp[-3].acc);
	    (yyval.acc).id = (yyvsp[-2].il);
	    (yyval.acc).flags |= (yyvsp[0].is);
	    if (((yyval.acc).flags & ACC_VIRTKEY) == 0
		&& ((yyval.acc).flags & (ACC_SHIFT | ACC_CONTROL)) != 0)
	      rcparse_warning (_("inappropriate modifiers for non-VIRTKEY"));
	  }
#line 2031 "rcparse.c" /* yacc.c:1648  */
    break;

  case 23:
#line 253 "rcparse.y" /* yacc.c:1648  */
    {
	    const char *s = (yyvsp[0].s);
	    char ch;

	    (yyval.acc).next = NULL;
	    (yyval.acc).id = 0;
	    ch = *s;
	    if (ch != '^')
	      (yyval.acc).flags = 0;
	    else
	      {
		(yyval.acc).flags = ACC_CONTROL | ACC_VIRTKEY;
		++s;
		ch = TOUPPER (s[0]);
	      }
	    (yyval.acc).key = ch;
	    if (s[1] != '\0')
	      rcparse_warning (_("accelerator should only be one character"));
	  }
#line 2055 "rcparse.c" /* yacc.c:1648  */
    break;

  case 24:
#line 273 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.acc).next = NULL;
	    (yyval.acc).flags = 0;
	    (yyval.acc).id = 0;
	    (yyval.acc).key = (yyvsp[0].il);
	  }
#line 2066 "rcparse.c" /* yacc.c:1648  */
    break;

  case 25:
#line 283 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.is) = (yyvsp[0].is);
	  }
#line 2074 "rcparse.c" /* yacc.c:1648  */
    break;

  case 26:
#line 287 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.is) = (yyvsp[-2].is) | (yyvsp[0].is);
	  }
#line 2082 "rcparse.c" /* yacc.c:1648  */
    break;

  case 27:
#line 292 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.is) = (yyvsp[-1].is) | (yyvsp[0].is);
	  }
#line 2090 "rcparse.c" /* yacc.c:1648  */
    break;

  case 28:
#line 299 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.is) = ACC_VIRTKEY;
	  }
#line 2098 "rcparse.c" /* yacc.c:1648  */
    break;

  case 29:
#line 303 "rcparse.y" /* yacc.c:1648  */
    {
	    /* This is just the absence of VIRTKEY.  */
	    (yyval.is) = 0;
	  }
#line 2107 "rcparse.c" /* yacc.c:1648  */
    break;

  case 30:
#line 308 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.is) = ACC_NOINVERT;
	  }
#line 2115 "rcparse.c" /* yacc.c:1648  */
    break;

  case 31:
#line 312 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.is) = ACC_SHIFT;
	  }
#line 2123 "rcparse.c" /* yacc.c:1648  */
    break;

  case 32:
#line 316 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.is) = ACC_CONTROL;
	  }
#line 2131 "rcparse.c" /* yacc.c:1648  */
    break;

  case 33:
#line 320 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.is) = ACC_ALT;
	  }
#line 2139 "rcparse.c" /* yacc.c:1648  */
    break;

  case 34:
#line 329 "rcparse.y" /* yacc.c:1648  */
    {
	    define_bitmap ((yyvsp[-3].id), &(yyvsp[-1].res_info), (yyvsp[0].s));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 2150 "rcparse.c" /* yacc.c:1648  */
    break;

  case 35:
#line 341 "rcparse.y" /* yacc.c:1648  */
    {
	    define_cursor ((yyvsp[-3].id), &(yyvsp[-1].res_info), (yyvsp[0].s));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 2161 "rcparse.c" /* yacc.c:1648  */
    break;

  case 36:
#line 354 "rcparse.y" /* yacc.c:1648  */
    {
	      memset (&dialog, 0, sizeof dialog);
	      dialog.x = (yyvsp[-3].il);
	      dialog.y = (yyvsp[-2].il);
	      dialog.width = (yyvsp[-1].il);
	      dialog.height = (yyvsp[0].il);
	      dialog.style = WS_POPUP | WS_BORDER | WS_SYSMENU;
	      dialog.exstyle = (yyvsp[-4].il);
	      dialog.menu.named = 1;
	      dialog.class.named = 1;
	      dialog.font = NULL;
	      dialog.ex = NULL;
	      dialog.controls = NULL;
	      sub_res_info = (yyvsp[-5].res_info);
	      style = 0;
	    }
#line 2182 "rcparse.c" /* yacc.c:1648  */
    break;

  case 37:
#line 371 "rcparse.y" /* yacc.c:1648  */
    {
	    define_dialog ((yyvsp[-12].id), &sub_res_info, &dialog);
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 2193 "rcparse.c" /* yacc.c:1648  */
    break;

  case 38:
#line 379 "rcparse.y" /* yacc.c:1648  */
    {
	      memset (&dialog, 0, sizeof dialog);
	      dialog.x = (yyvsp[-3].il);
	      dialog.y = (yyvsp[-2].il);
	      dialog.width = (yyvsp[-1].il);
	      dialog.height = (yyvsp[0].il);
	      dialog.style = WS_POPUP | WS_BORDER | WS_SYSMENU;
	      dialog.exstyle = (yyvsp[-4].il);
	      dialog.menu.named = 1;
	      dialog.class.named = 1;
	      dialog.font = NULL;
	      dialog.ex = ((rc_dialog_ex *)
			   res_alloc (sizeof (rc_dialog_ex)));
	      memset (dialog.ex, 0, sizeof (rc_dialog_ex));
	      dialog.controls = NULL;
	      sub_res_info = (yyvsp[-5].res_info);
	      style = 0;
	    }
#line 2216 "rcparse.c" /* yacc.c:1648  */
    break;

  case 39:
#line 398 "rcparse.y" /* yacc.c:1648  */
    {
	    define_dialog ((yyvsp[-12].id), &sub_res_info, &dialog);
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 2227 "rcparse.c" /* yacc.c:1648  */
    break;

  case 40:
#line 406 "rcparse.y" /* yacc.c:1648  */
    {
	      memset (&dialog, 0, sizeof dialog);
	      dialog.x = (yyvsp[-4].il);
	      dialog.y = (yyvsp[-3].il);
	      dialog.width = (yyvsp[-2].il);
	      dialog.height = (yyvsp[-1].il);
	      dialog.style = WS_POPUP | WS_BORDER | WS_SYSMENU;
	      dialog.exstyle = (yyvsp[-5].il);
	      dialog.menu.named = 1;
	      dialog.class.named = 1;
	      dialog.font = NULL;
	      dialog.ex = ((rc_dialog_ex *)
			   res_alloc (sizeof (rc_dialog_ex)));
	      memset (dialog.ex, 0, sizeof (rc_dialog_ex));
	      dialog.ex->help = (yyvsp[0].il);
	      dialog.controls = NULL;
	      sub_res_info = (yyvsp[-6].res_info);
	      style = 0;
	    }
#line 2251 "rcparse.c" /* yacc.c:1648  */
    break;

  case 41:
#line 426 "rcparse.y" /* yacc.c:1648  */
    {
	    define_dialog ((yyvsp[-13].id), &sub_res_info, &dialog);
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 2262 "rcparse.c" /* yacc.c:1648  */
    break;

  case 42:
#line 436 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.il) = 0;
	  }
#line 2270 "rcparse.c" /* yacc.c:1648  */
    break;

  case 43:
#line 440 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.il) = (yyvsp[0].il);
	  }
#line 2278 "rcparse.c" /* yacc.c:1648  */
    break;

  case 45:
#line 448 "rcparse.y" /* yacc.c:1648  */
    {
	    dialog.style |= WS_CAPTION;
	    style |= WS_CAPTION;
	    dialog.caption = (yyvsp[0].uni);
	  }
#line 2288 "rcparse.c" /* yacc.c:1648  */
    break;

  case 46:
#line 454 "rcparse.y" /* yacc.c:1648  */
    {
	    dialog.class = (yyvsp[0].id);
	  }
#line 2296 "rcparse.c" /* yacc.c:1648  */
    break;

  case 47:
#line 459 "rcparse.y" /* yacc.c:1648  */
    {
	    dialog.style = style;
	  }
#line 2304 "rcparse.c" /* yacc.c:1648  */
    break;

  case 48:
#line 463 "rcparse.y" /* yacc.c:1648  */
    {
	    dialog.exstyle = (yyvsp[0].il);
	  }
#line 2312 "rcparse.c" /* yacc.c:1648  */
    break;

  case 49:
#line 467 "rcparse.y" /* yacc.c:1648  */
    {
	    res_unistring_to_id (& dialog.class, (yyvsp[0].uni));
	  }
#line 2320 "rcparse.c" /* yacc.c:1648  */
    break;

  case 50:
#line 471 "rcparse.y" /* yacc.c:1648  */
    {
	    dialog.style |= DS_SETFONT;
	    style |= DS_SETFONT;
	    dialog.pointsize = (yyvsp[-2].il);
	    dialog.font = (yyvsp[0].uni);
	    if (dialog.ex != NULL)
	      {
		dialog.ex->weight = 0;
		dialog.ex->italic = 0;
		dialog.ex->charset = 1;
	      }
	  }
#line 2337 "rcparse.c" /* yacc.c:1648  */
    break;

  case 51:
#line 484 "rcparse.y" /* yacc.c:1648  */
    {
	    dialog.style |= DS_SETFONT;
	    style |= DS_SETFONT;
	    dialog.pointsize = (yyvsp[-3].il);
	    dialog.font = (yyvsp[-1].uni);
	    if (dialog.ex == NULL)
	      rcparse_warning (_("extended FONT requires DIALOGEX"));
	    else
	      {
		dialog.ex->weight = (yyvsp[0].il);
		dialog.ex->italic = 0;
		dialog.ex->charset = 1;
	      }
	  }
#line 2356 "rcparse.c" /* yacc.c:1648  */
    break;

  case 52:
#line 499 "rcparse.y" /* yacc.c:1648  */
    {
	    dialog.style |= DS_SETFONT;
	    style |= DS_SETFONT;
	    dialog.pointsize = (yyvsp[-4].il);
	    dialog.font = (yyvsp[-2].uni);
	    if (dialog.ex == NULL)
	      rcparse_warning (_("extended FONT requires DIALOGEX"));
	    else
	      {
		dialog.ex->weight = (yyvsp[-1].il);
		dialog.ex->italic = (yyvsp[0].il);
		dialog.ex->charset = 1;
	      }
	  }
#line 2375 "rcparse.c" /* yacc.c:1648  */
    break;

  case 53:
#line 514 "rcparse.y" /* yacc.c:1648  */
    {
	    dialog.style |= DS_SETFONT;
	    style |= DS_SETFONT;
	    dialog.pointsize = (yyvsp[-5].il);
	    dialog.font = (yyvsp[-3].uni);
	    if (dialog.ex == NULL)
	      rcparse_warning (_("extended FONT requires DIALOGEX"));
	    else
	      {
		dialog.ex->weight = (yyvsp[-2].il);
		dialog.ex->italic = (yyvsp[-1].il);
		dialog.ex->charset = (yyvsp[0].il);
	      }
	  }
#line 2394 "rcparse.c" /* yacc.c:1648  */
    break;

  case 54:
#line 529 "rcparse.y" /* yacc.c:1648  */
    {
	    dialog.menu = (yyvsp[0].id);
	  }
#line 2402 "rcparse.c" /* yacc.c:1648  */
    break;

  case 55:
#line 533 "rcparse.y" /* yacc.c:1648  */
    {
	    sub_res_info.characteristics = (yyvsp[0].il);
	  }
#line 2410 "rcparse.c" /* yacc.c:1648  */
    break;

  case 56:
#line 537 "rcparse.y" /* yacc.c:1648  */
    {
	    sub_res_info.language = (yyvsp[-1].il) | ((yyvsp[0].il) << SUBLANG_SHIFT);
	  }
#line 2418 "rcparse.c" /* yacc.c:1648  */
    break;

  case 57:
#line 541 "rcparse.y" /* yacc.c:1648  */
    {
	    sub_res_info.version = (yyvsp[0].il);
	  }
#line 2426 "rcparse.c" /* yacc.c:1648  */
    break;

  case 59:
#line 549 "rcparse.y" /* yacc.c:1648  */
    {
	    rc_dialog_control **pp;

	    for (pp = &dialog.controls; *pp != NULL; pp = &(*pp)->next)
	      ;
	    *pp = (yyvsp[0].dialog_control);
	  }
#line 2438 "rcparse.c" /* yacc.c:1648  */
    break;

  case 60:
#line 560 "rcparse.y" /* yacc.c:1648  */
    {
	      default_style = BS_AUTO3STATE | WS_TABSTOP;
	      base_style = BS_AUTO3STATE;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2450 "rcparse.c" /* yacc.c:1648  */
    break;

  case 61:
#line 568 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2458 "rcparse.c" /* yacc.c:1648  */
    break;

  case 62:
#line 572 "rcparse.y" /* yacc.c:1648  */
    {
	      default_style = BS_AUTOCHECKBOX | WS_TABSTOP;
	      base_style = BS_AUTOCHECKBOX;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2470 "rcparse.c" /* yacc.c:1648  */
    break;

  case 63:
#line 580 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2478 "rcparse.c" /* yacc.c:1648  */
    break;

  case 64:
#line 584 "rcparse.y" /* yacc.c:1648  */
    {
	      default_style = BS_AUTORADIOBUTTON | WS_TABSTOP;
	      base_style = BS_AUTORADIOBUTTON;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2490 "rcparse.c" /* yacc.c:1648  */
    break;

  case 65:
#line 592 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2498 "rcparse.c" /* yacc.c:1648  */
    break;

  case 66:
#line 596 "rcparse.y" /* yacc.c:1648  */
    {
	      default_style = ES_LEFT | WS_BORDER | WS_TABSTOP;
	      base_style = ES_LEFT | WS_BORDER | WS_TABSTOP;
	      class.named = 0;
	      class.u.id = CTL_EDIT;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2510 "rcparse.c" /* yacc.c:1648  */
    break;

  case 67:
#line 604 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	    if (dialog.ex == NULL)
	      rcparse_warning (_("BEDIT requires DIALOGEX"));
	    res_string_to_id (&(yyval.dialog_control)->class, "BEDIT");
	  }
#line 2521 "rcparse.c" /* yacc.c:1648  */
    break;

  case 68:
#line 611 "rcparse.y" /* yacc.c:1648  */
    {
	      default_style = BS_CHECKBOX | WS_TABSTOP;
	      base_style = BS_CHECKBOX | WS_TABSTOP;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2533 "rcparse.c" /* yacc.c:1648  */
    break;

  case 69:
#line 619 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2541 "rcparse.c" /* yacc.c:1648  */
    break;

  case 70:
#line 623 "rcparse.y" /* yacc.c:1648  */
    {
	      /* This is as per MSDN documentation.  With some (???)
		 versions of MS rc.exe their is no default style.  */
	      default_style = CBS_SIMPLE | WS_TABSTOP;
	      base_style = 0;
	      class.named = 0;
	      class.u.id = CTL_COMBOBOX;
	      res_text_field = res_null_text;
	    }
#line 2555 "rcparse.c" /* yacc.c:1648  */
    break;

  case 71:
#line 633 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2563 "rcparse.c" /* yacc.c:1648  */
    break;

  case 72:
#line 638 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = define_control ((yyvsp[-9].id), (yyvsp[-8].il), (yyvsp[-5].il), (yyvsp[-4].il), (yyvsp[-3].il), (yyvsp[-2].il), (yyvsp[-7].id), style, (yyvsp[-1].il));
	    if ((yyvsp[0].rcdata_item) != NULL)
	      {
		if (dialog.ex == NULL)
		  rcparse_warning (_("control data requires DIALOGEX"));
		(yyval.dialog_control)->data = (yyvsp[0].rcdata_item);
	      }
	  }
#line 2577 "rcparse.c" /* yacc.c:1648  */
    break;

  case 73:
#line 649 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = define_control ((yyvsp[-10].id), (yyvsp[-9].il), (yyvsp[-6].il), (yyvsp[-5].il), (yyvsp[-4].il), (yyvsp[-3].il), (yyvsp[-8].id), style, (yyvsp[-2].il));
	    if (dialog.ex == NULL)
	      rcparse_warning (_("help ID requires DIALOGEX"));
	    (yyval.dialog_control)->help = (yyvsp[-1].il);
	    (yyval.dialog_control)->data = (yyvsp[0].rcdata_item);
	  }
#line 2589 "rcparse.c" /* yacc.c:1648  */
    break;

  case 74:
#line 657 "rcparse.y" /* yacc.c:1648  */
    {
	      default_style = SS_CENTER | WS_GROUP;
	      base_style = SS_CENTER;
	      class.named = 0;
	      class.u.id = CTL_STATIC;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2601 "rcparse.c" /* yacc.c:1648  */
    break;

  case 75:
#line 665 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2609 "rcparse.c" /* yacc.c:1648  */
    break;

  case 76:
#line 669 "rcparse.y" /* yacc.c:1648  */
    {
	      default_style = BS_DEFPUSHBUTTON | WS_TABSTOP;
	      base_style = BS_DEFPUSHBUTTON | WS_TABSTOP;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2621 "rcparse.c" /* yacc.c:1648  */
    break;

  case 77:
#line 677 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2629 "rcparse.c" /* yacc.c:1648  */
    break;

  case 78:
#line 681 "rcparse.y" /* yacc.c:1648  */
    {
	      default_style = ES_LEFT | WS_BORDER | WS_TABSTOP;
	      base_style = ES_LEFT | WS_BORDER | WS_TABSTOP;
	      class.named = 0;
	      class.u.id = CTL_EDIT;
	      res_text_field = res_null_text;
	    }
#line 2641 "rcparse.c" /* yacc.c:1648  */
    break;

  case 79:
#line 689 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2649 "rcparse.c" /* yacc.c:1648  */
    break;

  case 80:
#line 693 "rcparse.y" /* yacc.c:1648  */
    {
	      default_style = BS_GROUPBOX;
	      base_style = BS_GROUPBOX;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2661 "rcparse.c" /* yacc.c:1648  */
    break;

  case 81:
#line 701 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2669 "rcparse.c" /* yacc.c:1648  */
    break;

  case 82:
#line 705 "rcparse.y" /* yacc.c:1648  */
    {
	      default_style = ES_LEFT | WS_BORDER | WS_TABSTOP;
	      base_style = ES_LEFT | WS_BORDER | WS_TABSTOP;
	      class.named = 0;
	      class.u.id = CTL_EDIT;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2681 "rcparse.c" /* yacc.c:1648  */
    break;

  case 83:
#line 713 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	    if (dialog.ex == NULL)
	      rcparse_warning (_("IEDIT requires DIALOGEX"));
	    res_string_to_id (&(yyval.dialog_control)->class, "HEDIT");
	  }
#line 2692 "rcparse.c" /* yacc.c:1648  */
    break;

  case 84:
#line 720 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = define_icon_control ((yyvsp[-4].id), (yyvsp[-3].il), (yyvsp[-2].il), (yyvsp[-1].il), 0, 0, 0, (yyvsp[0].rcdata_item),
				      dialog.ex);
          }
#line 2701 "rcparse.c" /* yacc.c:1648  */
    break;

  case 85:
#line 726 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = define_icon_control ((yyvsp[-6].id), (yyvsp[-5].il), (yyvsp[-4].il), (yyvsp[-3].il), 0, 0, 0, (yyvsp[0].rcdata_item),
				      dialog.ex);
          }
#line 2710 "rcparse.c" /* yacc.c:1648  */
    break;

  case 86:
#line 732 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = define_icon_control ((yyvsp[-8].id), (yyvsp[-7].il), (yyvsp[-6].il), (yyvsp[-5].il), style, (yyvsp[-1].il), 0, (yyvsp[0].rcdata_item),
				      dialog.ex);
          }
#line 2719 "rcparse.c" /* yacc.c:1648  */
    break;

  case 87:
#line 738 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = define_icon_control ((yyvsp[-9].id), (yyvsp[-8].il), (yyvsp[-7].il), (yyvsp[-6].il), style, (yyvsp[-2].il), (yyvsp[-1].il), (yyvsp[0].rcdata_item),
				      dialog.ex);
          }
#line 2728 "rcparse.c" /* yacc.c:1648  */
    break;

  case 88:
#line 743 "rcparse.y" /* yacc.c:1648  */
    {
	      default_style = ES_LEFT | WS_BORDER | WS_TABSTOP;
	      base_style = ES_LEFT | WS_BORDER | WS_TABSTOP;
	      class.named = 0;
	      class.u.id = CTL_EDIT;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2740 "rcparse.c" /* yacc.c:1648  */
    break;

  case 89:
#line 751 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	    if (dialog.ex == NULL)
	      rcparse_warning (_("IEDIT requires DIALOGEX"));
	    res_string_to_id (&(yyval.dialog_control)->class, "IEDIT");
	  }
#line 2751 "rcparse.c" /* yacc.c:1648  */
    break;

  case 90:
#line 758 "rcparse.y" /* yacc.c:1648  */
    {
	      default_style = LBS_NOTIFY | WS_BORDER;
	      base_style = LBS_NOTIFY | WS_BORDER;
	      class.named = 0;
	      class.u.id = CTL_LISTBOX;
	      res_text_field = res_null_text;
	    }
#line 2763 "rcparse.c" /* yacc.c:1648  */
    break;

  case 91:
#line 766 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2771 "rcparse.c" /* yacc.c:1648  */
    break;

  case 92:
#line 770 "rcparse.y" /* yacc.c:1648  */
    {
	      default_style = SS_LEFT | WS_GROUP;
	      base_style = SS_LEFT;
	      class.named = 0;
	      class.u.id = CTL_STATIC;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2783 "rcparse.c" /* yacc.c:1648  */
    break;

  case 93:
#line 778 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2791 "rcparse.c" /* yacc.c:1648  */
    break;

  case 94:
#line 782 "rcparse.y" /* yacc.c:1648  */
    {
	      default_style = BS_PUSHBOX | WS_TABSTOP;
	      base_style = BS_PUSHBOX;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	    }
#line 2802 "rcparse.c" /* yacc.c:1648  */
    break;

  case 95:
#line 789 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2810 "rcparse.c" /* yacc.c:1648  */
    break;

  case 96:
#line 793 "rcparse.y" /* yacc.c:1648  */
    {
	      default_style = BS_PUSHBUTTON | WS_TABSTOP;
	      base_style = BS_PUSHBUTTON | WS_TABSTOP;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2822 "rcparse.c" /* yacc.c:1648  */
    break;

  case 97:
#line 801 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2830 "rcparse.c" /* yacc.c:1648  */
    break;

  case 98:
#line 805 "rcparse.y" /* yacc.c:1648  */
    {
	      default_style = BS_RADIOBUTTON | WS_TABSTOP;
	      base_style = BS_RADIOBUTTON;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2842 "rcparse.c" /* yacc.c:1648  */
    break;

  case 99:
#line 813 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2850 "rcparse.c" /* yacc.c:1648  */
    break;

  case 100:
#line 817 "rcparse.y" /* yacc.c:1648  */
    {
	      default_style = SS_RIGHT | WS_GROUP;
	      base_style = SS_RIGHT;
	      class.named = 0;
	      class.u.id = CTL_STATIC;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2862 "rcparse.c" /* yacc.c:1648  */
    break;

  case 101:
#line 825 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2870 "rcparse.c" /* yacc.c:1648  */
    break;

  case 102:
#line 829 "rcparse.y" /* yacc.c:1648  */
    {
	      default_style = SBS_HORZ;
	      base_style = 0;
	      class.named = 0;
	      class.u.id = CTL_SCROLLBAR;
	      res_text_field = res_null_text;
	    }
#line 2882 "rcparse.c" /* yacc.c:1648  */
    break;

  case 103:
#line 837 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2890 "rcparse.c" /* yacc.c:1648  */
    break;

  case 104:
#line 841 "rcparse.y" /* yacc.c:1648  */
    {
	      default_style = BS_3STATE | WS_TABSTOP;
	      base_style = BS_3STATE;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2902 "rcparse.c" /* yacc.c:1648  */
    break;

  case 105:
#line 849 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2910 "rcparse.c" /* yacc.c:1648  */
    break;

  case 106:
#line 854 "rcparse.y" /* yacc.c:1648  */
    { style = WS_CHILD | WS_VISIBLE; }
#line 2916 "rcparse.c" /* yacc.c:1648  */
    break;

  case 107:
#line 856 "rcparse.y" /* yacc.c:1648  */
    {
	    rc_res_id cid;
	    cid.named = 0;
	    cid.u.id = CTL_BUTTON;
	    (yyval.dialog_control) = define_control ((yyvsp[-13].id), (yyvsp[-12].il), (yyvsp[-10].il), (yyvsp[-8].il), (yyvsp[-6].il), (yyvsp[-4].il), cid,
				 style, (yyvsp[0].il));
	  }
#line 2928 "rcparse.c" /* yacc.c:1648  */
    break;

  case 108:
#line 874 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = define_control (res_text_field, (yyvsp[-5].il), (yyvsp[-4].il), (yyvsp[-3].il), (yyvsp[-2].il), (yyvsp[-1].il), class,
				 default_style | WS_CHILD | WS_VISIBLE, 0);
	    if ((yyvsp[0].rcdata_item) != NULL)
	      {
		if (dialog.ex == NULL)
		  rcparse_warning (_("control data requires DIALOGEX"));
		(yyval.dialog_control)->data = (yyvsp[0].rcdata_item);
	      }
	  }
#line 2943 "rcparse.c" /* yacc.c:1648  */
    break;

  case 109:
#line 886 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = define_control (res_text_field, (yyvsp[-7].il), (yyvsp[-6].il), (yyvsp[-5].il), (yyvsp[-4].il), (yyvsp[-3].il), class, style, (yyvsp[-1].il));
	    if ((yyvsp[0].rcdata_item) != NULL)
	      {
		if (dialog.ex == NULL)
		  rcparse_warning (_("control data requires DIALOGEX"));
		(yyval.dialog_control)->data = (yyvsp[0].rcdata_item);
	      }
	  }
#line 2957 "rcparse.c" /* yacc.c:1648  */
    break;

  case 110:
#line 897 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.dialog_control) = define_control (res_text_field, (yyvsp[-8].il), (yyvsp[-7].il), (yyvsp[-6].il), (yyvsp[-5].il), (yyvsp[-4].il), class, style, (yyvsp[-2].il));
	    if (dialog.ex == NULL)
	      rcparse_warning (_("help ID requires DIALOGEX"));
	    (yyval.dialog_control)->help = (yyvsp[-1].il);
	    (yyval.dialog_control)->data = (yyvsp[0].rcdata_item);
	  }
#line 2969 "rcparse.c" /* yacc.c:1648  */
    break;

  case 111:
#line 908 "rcparse.y" /* yacc.c:1648  */
    {
	    if ((yyvsp[0].id).named)
	      res_unistring_to_id (&(yyval.id), (yyvsp[0].id).u.n.name);
	    else
	      (yyval.id)=(yyvsp[0].id);
	  }
#line 2980 "rcparse.c" /* yacc.c:1648  */
    break;

  case 112:
#line 918 "rcparse.y" /* yacc.c:1648  */
    {
	    res_string_to_id (&(yyval.id), "");
	  }
#line 2988 "rcparse.c" /* yacc.c:1648  */
    break;

  case 113:
#line 921 "rcparse.y" /* yacc.c:1648  */
    { (yyval.id)=(yyvsp[-1].id); }
#line 2994 "rcparse.c" /* yacc.c:1648  */
    break;

  case 114:
#line 926 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.id).named = 0;
	    (yyval.id).u.id = (yyvsp[0].il);
	  }
#line 3003 "rcparse.c" /* yacc.c:1648  */
    break;

  case 115:
#line 931 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.id).named = 1;
	    (yyval.id).u.n.name = (yyvsp[0].uni);
	    (yyval.id).u.n.length = unichar_len ((yyvsp[0].uni));
	  }
#line 3013 "rcparse.c" /* yacc.c:1648  */
    break;

  case 116:
#line 940 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.rcdata_item) = NULL;
	  }
#line 3021 "rcparse.c" /* yacc.c:1648  */
    break;

  case 117:
#line 944 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.rcdata_item) = (yyvsp[-1].rcdata).first;
	  }
#line 3029 "rcparse.c" /* yacc.c:1648  */
    break;

  case 118:
#line 953 "rcparse.y" /* yacc.c:1648  */
    { style = WS_CHILD | WS_VISIBLE; }
#line 3035 "rcparse.c" /* yacc.c:1648  */
    break;

  case 120:
#line 959 "rcparse.y" /* yacc.c:1648  */
    { style = SS_ICON | WS_CHILD | WS_VISIBLE; }
#line 3041 "rcparse.c" /* yacc.c:1648  */
    break;

  case 122:
#line 965 "rcparse.y" /* yacc.c:1648  */
    { style = base_style | WS_CHILD | WS_VISIBLE; }
#line 3047 "rcparse.c" /* yacc.c:1648  */
    break;

  case 124:
#line 973 "rcparse.y" /* yacc.c:1648  */
    {
	    define_font ((yyvsp[-3].id), &(yyvsp[-1].res_info), (yyvsp[0].s));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 3058 "rcparse.c" /* yacc.c:1648  */
    break;

  case 125:
#line 985 "rcparse.y" /* yacc.c:1648  */
    {
	    define_icon ((yyvsp[-3].id), &(yyvsp[-1].res_info), (yyvsp[0].s));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 3069 "rcparse.c" /* yacc.c:1648  */
    break;

  case 126:
#line 998 "rcparse.y" /* yacc.c:1648  */
    {
	    language = (yyvsp[-1].il) | ((yyvsp[0].il) << SUBLANG_SHIFT);
	  }
#line 3077 "rcparse.c" /* yacc.c:1648  */
    break;

  case 127:
#line 1007 "rcparse.y" /* yacc.c:1648  */
    {
	    define_menu ((yyvsp[-5].id), &(yyvsp[-3].res_info), (yyvsp[-1].menuitem));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 3088 "rcparse.c" /* yacc.c:1648  */
    break;

  case 128:
#line 1017 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.menuitem) = NULL;
	  }
#line 3096 "rcparse.c" /* yacc.c:1648  */
    break;

  case 129:
#line 1021 "rcparse.y" /* yacc.c:1648  */
    {
	    if ((yyvsp[-1].menuitem) == NULL)
	      (yyval.menuitem) = (yyvsp[0].menuitem);
	    else
	      {
		rc_menuitem **pp;

		for (pp = &(yyvsp[-1].menuitem)->next; *pp != NULL; pp = &(*pp)->next)
		  ;
		*pp = (yyvsp[0].menuitem);
		(yyval.menuitem) = (yyvsp[-1].menuitem);
	      }
	  }
#line 3114 "rcparse.c" /* yacc.c:1648  */
    break;

  case 130:
#line 1038 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.menuitem) = define_menuitem ((yyvsp[-2].uni), (yyvsp[-1].il), (yyvsp[0].is), 0, 0, NULL);
	  }
#line 3122 "rcparse.c" /* yacc.c:1648  */
    break;

  case 131:
#line 1042 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.menuitem) = define_menuitem (NULL, 0, 0, 0, 0, NULL);
	  }
#line 3130 "rcparse.c" /* yacc.c:1648  */
    break;

  case 132:
#line 1046 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.menuitem) = define_menuitem ((yyvsp[-4].uni), 0, (yyvsp[-3].is), 0, 0, (yyvsp[-1].menuitem));
	  }
#line 3138 "rcparse.c" /* yacc.c:1648  */
    break;

  case 133:
#line 1053 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.is) = 0;
	  }
#line 3146 "rcparse.c" /* yacc.c:1648  */
    break;

  case 134:
#line 1057 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.is) = (yyvsp[-2].is) | (yyvsp[0].is);
	  }
#line 3154 "rcparse.c" /* yacc.c:1648  */
    break;

  case 135:
#line 1061 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.is) = (yyvsp[-1].is) | (yyvsp[0].is);
	  }
#line 3162 "rcparse.c" /* yacc.c:1648  */
    break;

  case 136:
#line 1068 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.is) = MENUITEM_CHECKED;
	  }
#line 3170 "rcparse.c" /* yacc.c:1648  */
    break;

  case 137:
#line 1072 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.is) = MENUITEM_GRAYED;
	  }
#line 3178 "rcparse.c" /* yacc.c:1648  */
    break;

  case 138:
#line 1076 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.is) = MENUITEM_HELP;
	  }
#line 3186 "rcparse.c" /* yacc.c:1648  */
    break;

  case 139:
#line 1080 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.is) = MENUITEM_INACTIVE;
	  }
#line 3194 "rcparse.c" /* yacc.c:1648  */
    break;

  case 140:
#line 1084 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.is) = MENUITEM_MENUBARBREAK;
	  }
#line 3202 "rcparse.c" /* yacc.c:1648  */
    break;

  case 141:
#line 1088 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.is) = MENUITEM_MENUBREAK;
	  }
#line 3210 "rcparse.c" /* yacc.c:1648  */
    break;

  case 142:
#line 1097 "rcparse.y" /* yacc.c:1648  */
    {
	    define_menu ((yyvsp[-5].id), &(yyvsp[-3].res_info), (yyvsp[-1].menuitem));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 3221 "rcparse.c" /* yacc.c:1648  */
    break;

  case 143:
#line 1107 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.menuitem) = NULL;
	  }
#line 3229 "rcparse.c" /* yacc.c:1648  */
    break;

  case 144:
#line 1111 "rcparse.y" /* yacc.c:1648  */
    {
	    if ((yyvsp[-1].menuitem) == NULL)
	      (yyval.menuitem) = (yyvsp[0].menuitem);
	    else
	      {
		rc_menuitem **pp;

		for (pp = &(yyvsp[-1].menuitem)->next; *pp != NULL; pp = &(*pp)->next)
		  ;
		*pp = (yyvsp[0].menuitem);
		(yyval.menuitem) = (yyvsp[-1].menuitem);
	      }
	  }
#line 3247 "rcparse.c" /* yacc.c:1648  */
    break;

  case 145:
#line 1128 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.menuitem) = define_menuitem ((yyvsp[0].uni), 0, 0, 0, 0, NULL);
	  }
#line 3255 "rcparse.c" /* yacc.c:1648  */
    break;

  case 146:
#line 1132 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.menuitem) = define_menuitem ((yyvsp[-1].uni), (yyvsp[0].il), 0, 0, 0, NULL);
	  }
#line 3263 "rcparse.c" /* yacc.c:1648  */
    break;

  case 147:
#line 1136 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.menuitem) = define_menuitem ((yyvsp[-3].uni), (yyvsp[-2].il), (yyvsp[-1].il), (yyvsp[0].il), 0, NULL);
	  }
#line 3271 "rcparse.c" /* yacc.c:1648  */
    break;

  case 148:
#line 1140 "rcparse.y" /* yacc.c:1648  */
    {
 	    (yyval.menuitem) = define_menuitem (NULL, 0, 0, 0, 0, NULL);
 	  }
#line 3279 "rcparse.c" /* yacc.c:1648  */
    break;

  case 149:
#line 1144 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.menuitem) = define_menuitem ((yyvsp[-3].uni), 0, 0, 0, 0, (yyvsp[-1].menuitem));
	  }
#line 3287 "rcparse.c" /* yacc.c:1648  */
    break;

  case 150:
#line 1148 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.menuitem) = define_menuitem ((yyvsp[-4].uni), (yyvsp[-3].il), 0, 0, 0, (yyvsp[-1].menuitem));
	  }
#line 3295 "rcparse.c" /* yacc.c:1648  */
    break;

  case 151:
#line 1152 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.menuitem) = define_menuitem ((yyvsp[-5].uni), (yyvsp[-4].il), (yyvsp[-3].il), 0, 0, (yyvsp[-1].menuitem));
	  }
#line 3303 "rcparse.c" /* yacc.c:1648  */
    break;

  case 152:
#line 1157 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.menuitem) = define_menuitem ((yyvsp[-7].uni), (yyvsp[-6].il), (yyvsp[-5].il), (yyvsp[-4].il), (yyvsp[-3].il), (yyvsp[-1].menuitem));
	  }
#line 3311 "rcparse.c" /* yacc.c:1648  */
    break;

  case 153:
#line 1166 "rcparse.y" /* yacc.c:1648  */
    {
	    define_messagetable ((yyvsp[-3].id), &(yyvsp[-1].res_info), (yyvsp[0].s));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 3322 "rcparse.c" /* yacc.c:1648  */
    break;

  case 154:
#line 1178 "rcparse.y" /* yacc.c:1648  */
    {
	    rcparse_rcdata ();
	  }
#line 3330 "rcparse.c" /* yacc.c:1648  */
    break;

  case 155:
#line 1182 "rcparse.y" /* yacc.c:1648  */
    {
	    rcparse_normal ();
	    (yyval.rcdata) = (yyvsp[0].rcdata);
	  }
#line 3339 "rcparse.c" /* yacc.c:1648  */
    break;

  case 156:
#line 1190 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.rcdata).first = NULL;
	    (yyval.rcdata).last = NULL;
	  }
#line 3348 "rcparse.c" /* yacc.c:1648  */
    break;

  case 157:
#line 1195 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.rcdata) = (yyvsp[0].rcdata);
	  }
#line 3356 "rcparse.c" /* yacc.c:1648  */
    break;

  case 158:
#line 1202 "rcparse.y" /* yacc.c:1648  */
    {
	    rc_rcdata_item *ri;

	    ri = define_rcdata_string ((yyvsp[0].ss).s, (yyvsp[0].ss).length);
	    (yyval.rcdata).first = ri;
	    (yyval.rcdata).last = ri;
	  }
#line 3368 "rcparse.c" /* yacc.c:1648  */
    break;

  case 159:
#line 1210 "rcparse.y" /* yacc.c:1648  */
    {
	    rc_rcdata_item *ri;

	    ri = define_rcdata_unistring ((yyvsp[0].suni).s, (yyvsp[0].suni).length);
	    (yyval.rcdata).first = ri;
	    (yyval.rcdata).last = ri;
	  }
#line 3380 "rcparse.c" /* yacc.c:1648  */
    break;

  case 160:
#line 1218 "rcparse.y" /* yacc.c:1648  */
    {
	    rc_rcdata_item *ri;

	    ri = define_rcdata_number ((yyvsp[0].i).val, (yyvsp[0].i).dword);
	    (yyval.rcdata).first = ri;
	    (yyval.rcdata).last = ri;
	  }
#line 3392 "rcparse.c" /* yacc.c:1648  */
    break;

  case 161:
#line 1226 "rcparse.y" /* yacc.c:1648  */
    {
	    rc_rcdata_item *ri;

	    ri = define_rcdata_string ((yyvsp[0].ss).s, (yyvsp[0].ss).length);
	    (yyval.rcdata).first = (yyvsp[-2].rcdata).first;
	    (yyvsp[-2].rcdata).last->next = ri;
	    (yyval.rcdata).last = ri;
	  }
#line 3405 "rcparse.c" /* yacc.c:1648  */
    break;

  case 162:
#line 1235 "rcparse.y" /* yacc.c:1648  */
    {
	    rc_rcdata_item *ri;

	    ri = define_rcdata_unistring ((yyvsp[0].suni).s, (yyvsp[0].suni).length);
	    (yyval.rcdata).first = (yyvsp[-2].rcdata).first;
	    (yyvsp[-2].rcdata).last->next = ri;
	    (yyval.rcdata).last = ri;
	  }
#line 3418 "rcparse.c" /* yacc.c:1648  */
    break;

  case 163:
#line 1244 "rcparse.y" /* yacc.c:1648  */
    {
	    rc_rcdata_item *ri;

	    ri = define_rcdata_number ((yyvsp[0].i).val, (yyvsp[0].i).dword);
	    (yyval.rcdata).first = (yyvsp[-2].rcdata).first;
	    (yyvsp[-2].rcdata).last->next = ri;
	    (yyval.rcdata).last = ri;
	  }
#line 3431 "rcparse.c" /* yacc.c:1648  */
    break;

  case 164:
#line 1253 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.rcdata)=(yyvsp[-1].rcdata);
	  }
#line 3439 "rcparse.c" /* yacc.c:1648  */
    break;

  case 165:
#line 1262 "rcparse.y" /* yacc.c:1648  */
    { sub_res_info = (yyvsp[-1].res_info); rcparse_rcdata (); }
#line 3445 "rcparse.c" /* yacc.c:1648  */
    break;

  case 166:
#line 1263 "rcparse.y" /* yacc.c:1648  */
    { rcparse_normal (); }
#line 3451 "rcparse.c" /* yacc.c:1648  */
    break;

  case 168:
#line 1269 "rcparse.y" /* yacc.c:1648  */
    {
	    define_stringtable (&sub_res_info, (yyvsp[-1].il), (yyvsp[0].suni).s, (yyvsp[0].suni).length);
	    rcparse_discard_strings ();
	  }
#line 3460 "rcparse.c" /* yacc.c:1648  */
    break;

  case 169:
#line 1274 "rcparse.y" /* yacc.c:1648  */
    {
	    define_stringtable (&sub_res_info, (yyvsp[-2].il), (yyvsp[0].suni).s, (yyvsp[0].suni).length);
	    rcparse_discard_strings ();
	  }
#line 3469 "rcparse.c" /* yacc.c:1648  */
    break;

  case 170:
#line 1279 "rcparse.y" /* yacc.c:1648  */
    {
	    rcparse_warning (_("invalid stringtable resource."));
	    abort ();
	  }
#line 3478 "rcparse.c" /* yacc.c:1648  */
    break;

  case 171:
#line 1287 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.id)=(yyvsp[0].id);
	  }
#line 3486 "rcparse.c" /* yacc.c:1648  */
    break;

  case 172:
#line 1291 "rcparse.y" /* yacc.c:1648  */
    {
	  (yyval.id).named = 0;
	  (yyval.id).u.id = 23;
	}
#line 3495 "rcparse.c" /* yacc.c:1648  */
    break;

  case 173:
#line 1296 "rcparse.y" /* yacc.c:1648  */
    {
          (yyval.id).named = 0;
          (yyval.id).u.id = RT_RCDATA;
        }
#line 3504 "rcparse.c" /* yacc.c:1648  */
    break;

  case 174:
#line 1301 "rcparse.y" /* yacc.c:1648  */
    {
          (yyval.id).named = 0;
          (yyval.id).u.id = RT_MANIFEST;
        }
#line 3513 "rcparse.c" /* yacc.c:1648  */
    break;

  case 175:
#line 1306 "rcparse.y" /* yacc.c:1648  */
    {
          (yyval.id).named = 0;
          (yyval.id).u.id = RT_PLUGPLAY;
        }
#line 3522 "rcparse.c" /* yacc.c:1648  */
    break;

  case 176:
#line 1311 "rcparse.y" /* yacc.c:1648  */
    {
          (yyval.id).named = 0;
          (yyval.id).u.id = RT_VXD;
        }
#line 3531 "rcparse.c" /* yacc.c:1648  */
    break;

  case 177:
#line 1316 "rcparse.y" /* yacc.c:1648  */
    {
          (yyval.id).named = 0;
          (yyval.id).u.id = RT_DLGINCLUDE;
        }
#line 3540 "rcparse.c" /* yacc.c:1648  */
    break;

  case 178:
#line 1321 "rcparse.y" /* yacc.c:1648  */
    {
          (yyval.id).named = 0;
          (yyval.id).u.id = RT_DLGINIT;
        }
#line 3549 "rcparse.c" /* yacc.c:1648  */
    break;

  case 179:
#line 1326 "rcparse.y" /* yacc.c:1648  */
    {
          (yyval.id).named = 0;
          (yyval.id).u.id = RT_ANICURSOR;
        }
#line 3558 "rcparse.c" /* yacc.c:1648  */
    break;

  case 180:
#line 1331 "rcparse.y" /* yacc.c:1648  */
    {
          (yyval.id).named = 0;
          (yyval.id).u.id = RT_ANIICON;
        }
#line 3567 "rcparse.c" /* yacc.c:1648  */
    break;

  case 181:
#line 1342 "rcparse.y" /* yacc.c:1648  */
    {
	    define_user_data ((yyvsp[-5].id), (yyvsp[-4].id), &(yyvsp[-3].res_info), (yyvsp[-1].rcdata).first);
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 3578 "rcparse.c" /* yacc.c:1648  */
    break;

  case 182:
#line 1349 "rcparse.y" /* yacc.c:1648  */
    {
	    define_user_file ((yyvsp[-3].id), (yyvsp[-2].id), &(yyvsp[-1].res_info), (yyvsp[0].s));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 3589 "rcparse.c" /* yacc.c:1648  */
    break;

  case 183:
#line 1359 "rcparse.y" /* yacc.c:1648  */
    {
	  define_toolbar ((yyvsp[-7].id), &(yyvsp[-5].res_info), (yyvsp[-4].il), (yyvsp[-3].il), (yyvsp[-1].toobar_item));
	}
#line 3597 "rcparse.c" /* yacc.c:1648  */
    break;

  case 184:
#line 1364 "rcparse.y" /* yacc.c:1648  */
    { (yyval.toobar_item)= NULL; }
#line 3603 "rcparse.c" /* yacc.c:1648  */
    break;

  case 185:
#line 1366 "rcparse.y" /* yacc.c:1648  */
    {
	  rc_toolbar_item *c,*n;
	  c = (yyvsp[-2].toobar_item);
	  n= (rc_toolbar_item *)
	      res_alloc (sizeof (rc_toolbar_item));
	  if (c != NULL)
	    while (c->next != NULL)
	      c = c->next;
	  n->prev = c;
	  n->next = NULL;
	  if (c != NULL)
	    c->next = n;
	  n->id = (yyvsp[0].id);
	  if ((yyvsp[-2].toobar_item) == NULL)
	    (yyval.toobar_item) = n;
	  else
	    (yyval.toobar_item) = (yyvsp[-2].toobar_item);
	}
#line 3626 "rcparse.c" /* yacc.c:1648  */
    break;

  case 186:
#line 1385 "rcparse.y" /* yacc.c:1648  */
    {
	  rc_toolbar_item *c,*n;
	  c = (yyvsp[-1].toobar_item);
	  n= (rc_toolbar_item *)
	      res_alloc (sizeof (rc_toolbar_item));
	  if (c != NULL)
	    while (c->next != NULL)
	      c = c->next;
	  n->prev = c;
	  n->next = NULL;
	  if (c != NULL)
	    c->next = n;
	  n->id.named = 0;
	  n->id.u.id = 0;
	  if ((yyvsp[-1].toobar_item) == NULL)
	    (yyval.toobar_item) = n;
	  else
	    (yyval.toobar_item) = (yyvsp[-1].toobar_item);
	}
#line 3650 "rcparse.c" /* yacc.c:1648  */
    break;

  case 187:
#line 1410 "rcparse.y" /* yacc.c:1648  */
    {
	    define_versioninfo ((yyvsp[-5].id), language, (yyvsp[-3].fixver), (yyvsp[-1].verinfo));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 3661 "rcparse.c" /* yacc.c:1648  */
    break;

  case 188:
#line 1420 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.fixver) = ((rc_fixed_versioninfo *)
		  res_alloc (sizeof (rc_fixed_versioninfo)));
	    memset ((yyval.fixver), 0, sizeof (rc_fixed_versioninfo));
	  }
#line 3671 "rcparse.c" /* yacc.c:1648  */
    break;

  case 189:
#line 1427 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyvsp[-5].fixver)->file_version_ms = ((yyvsp[-3].il) << 16) | ((yyvsp[-2].il) & 0xffff);
	    (yyvsp[-5].fixver)->file_version_ls = ((yyvsp[-1].il) << 16) | ((yyvsp[0].il) & 0xffff);
	    (yyval.fixver) = (yyvsp[-5].fixver);
	  }
#line 3681 "rcparse.c" /* yacc.c:1648  */
    break;

  case 190:
#line 1434 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyvsp[-5].fixver)->product_version_ms = ((yyvsp[-3].il) << 16) | ((yyvsp[-2].il) & 0xffff);
	    (yyvsp[-5].fixver)->product_version_ls = ((yyvsp[-1].il) << 16) | ((yyvsp[0].il) & 0xffff);
	    (yyval.fixver) = (yyvsp[-5].fixver);
	  }
#line 3691 "rcparse.c" /* yacc.c:1648  */
    break;

  case 191:
#line 1440 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyvsp[-2].fixver)->file_flags_mask = (yyvsp[0].il);
	    (yyval.fixver) = (yyvsp[-2].fixver);
	  }
#line 3700 "rcparse.c" /* yacc.c:1648  */
    break;

  case 192:
#line 1445 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyvsp[-2].fixver)->file_flags = (yyvsp[0].il);
	    (yyval.fixver) = (yyvsp[-2].fixver);
	  }
#line 3709 "rcparse.c" /* yacc.c:1648  */
    break;

  case 193:
#line 1450 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyvsp[-2].fixver)->file_os = (yyvsp[0].il);
	    (yyval.fixver) = (yyvsp[-2].fixver);
	  }
#line 3718 "rcparse.c" /* yacc.c:1648  */
    break;

  case 194:
#line 1455 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyvsp[-2].fixver)->file_type = (yyvsp[0].il);
	    (yyval.fixver) = (yyvsp[-2].fixver);
	  }
#line 3727 "rcparse.c" /* yacc.c:1648  */
    break;

  case 195:
#line 1460 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyvsp[-2].fixver)->file_subtype = (yyvsp[0].il);
	    (yyval.fixver) = (yyvsp[-2].fixver);
	  }
#line 3736 "rcparse.c" /* yacc.c:1648  */
    break;

  case 196:
#line 1474 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.verinfo) = NULL;
	  }
#line 3744 "rcparse.c" /* yacc.c:1648  */
    break;

  case 197:
#line 1478 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.verinfo) = append_ver_stringfileinfo ((yyvsp[-4].verinfo), (yyvsp[-1].verstringtable));
	  }
#line 3752 "rcparse.c" /* yacc.c:1648  */
    break;

  case 198:
#line 1482 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.verinfo) = append_ver_varfileinfo ((yyvsp[-6].verinfo), (yyvsp[-2].uni), (yyvsp[-1].vervar));
	  }
#line 3760 "rcparse.c" /* yacc.c:1648  */
    break;

  case 199:
#line 1489 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.verstringtable) = NULL;
	  }
#line 3768 "rcparse.c" /* yacc.c:1648  */
    break;

  case 200:
#line 1493 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.verstringtable) = append_ver_stringtable ((yyvsp[-4].verstringtable), (yyvsp[-3].s), (yyvsp[-1].verstring));
	  }
#line 3776 "rcparse.c" /* yacc.c:1648  */
    break;

  case 201:
#line 1500 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.verstring) = NULL;
	  }
#line 3784 "rcparse.c" /* yacc.c:1648  */
    break;

  case 202:
#line 1504 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.verstring) = append_verval ((yyvsp[-4].verstring), (yyvsp[-2].uni), (yyvsp[0].uni));
	  }
#line 3792 "rcparse.c" /* yacc.c:1648  */
    break;

  case 203:
#line 1511 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.vervar) = NULL;
	  }
#line 3800 "rcparse.c" /* yacc.c:1648  */
    break;

  case 204:
#line 1515 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.vervar) = append_vertrans ((yyvsp[-2].vervar), (yyvsp[-1].il), (yyvsp[0].il));
	  }
#line 3808 "rcparse.c" /* yacc.c:1648  */
    break;

  case 205:
#line 1524 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.id).named = 0;
	    (yyval.id).u.id = (yyvsp[0].il);
	  }
#line 3817 "rcparse.c" /* yacc.c:1648  */
    break;

  case 206:
#line 1529 "rcparse.y" /* yacc.c:1648  */
    {
	    res_unistring_to_id (&(yyval.id), (yyvsp[0].uni));
	  }
#line 3825 "rcparse.c" /* yacc.c:1648  */
    break;

  case 207:
#line 1538 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.uni) = (yyvsp[0].uni);
	  }
#line 3833 "rcparse.c" /* yacc.c:1648  */
    break;

  case 208:
#line 1542 "rcparse.y" /* yacc.c:1648  */
    {
	    unichar *h = NULL;
	    unicode_from_ascii ((rc_uint_type *) NULL, &h, (yyvsp[0].s));
	    (yyval.uni) = h;
	  }
#line 3843 "rcparse.c" /* yacc.c:1648  */
    break;

  case 209:
#line 1552 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.id).named = 0;
	    (yyval.id).u.id = (yyvsp[-1].il);
	  }
#line 3852 "rcparse.c" /* yacc.c:1648  */
    break;

  case 210:
#line 1557 "rcparse.y" /* yacc.c:1648  */
    {
	    res_unistring_to_id (&(yyval.id), (yyvsp[0].uni));
	  }
#line 3860 "rcparse.c" /* yacc.c:1648  */
    break;

  case 211:
#line 1561 "rcparse.y" /* yacc.c:1648  */
    {
	    res_unistring_to_id (&(yyval.id), (yyvsp[-1].uni));
	  }
#line 3868 "rcparse.c" /* yacc.c:1648  */
    break;

  case 212:
#line 1571 "rcparse.y" /* yacc.c:1648  */
    {
	    memset (&(yyval.res_info), 0, sizeof (rc_res_res_info));
	    (yyval.res_info).language = language;
	    /* FIXME: Is this the right default?  */
	    (yyval.res_info).memflags = MEMFLAG_MOVEABLE | MEMFLAG_PURE | MEMFLAG_DISCARDABLE;
	  }
#line 3879 "rcparse.c" /* yacc.c:1648  */
    break;

  case 213:
#line 1578 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.res_info) = (yyvsp[-1].res_info);
	    (yyval.res_info).memflags |= (yyvsp[0].memflags).on;
	    (yyval.res_info).memflags &=~ (yyvsp[0].memflags).off;
	  }
#line 3889 "rcparse.c" /* yacc.c:1648  */
    break;

  case 214:
#line 1584 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.res_info) = (yyvsp[-2].res_info);
	    (yyval.res_info).characteristics = (yyvsp[0].il);
	  }
#line 3898 "rcparse.c" /* yacc.c:1648  */
    break;

  case 215:
#line 1589 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.res_info) = (yyvsp[-3].res_info);
	    (yyval.res_info).language = (yyvsp[-1].il) | ((yyvsp[0].il) << SUBLANG_SHIFT);
	  }
#line 3907 "rcparse.c" /* yacc.c:1648  */
    break;

  case 216:
#line 1594 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.res_info) = (yyvsp[-2].res_info);
	    (yyval.res_info).version = (yyvsp[0].il);
	  }
#line 3916 "rcparse.c" /* yacc.c:1648  */
    break;

  case 217:
#line 1604 "rcparse.y" /* yacc.c:1648  */
    {
	    memset (&(yyval.res_info), 0, sizeof (rc_res_res_info));
	    (yyval.res_info).language = language;
	    (yyval.res_info).memflags = MEMFLAG_MOVEABLE | MEMFLAG_DISCARDABLE;
	  }
#line 3926 "rcparse.c" /* yacc.c:1648  */
    break;

  case 218:
#line 1610 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.res_info) = (yyvsp[-1].res_info);
	    (yyval.res_info).memflags |= (yyvsp[0].memflags).on;
	    (yyval.res_info).memflags &=~ (yyvsp[0].memflags).off;
	  }
#line 3936 "rcparse.c" /* yacc.c:1648  */
    break;

  case 219:
#line 1621 "rcparse.y" /* yacc.c:1648  */
    {
	    memset (&(yyval.res_info), 0, sizeof (rc_res_res_info));
	    (yyval.res_info).language = language;
	    (yyval.res_info).memflags = MEMFLAG_MOVEABLE | MEMFLAG_PURE | MEMFLAG_DISCARDABLE;
	  }
#line 3946 "rcparse.c" /* yacc.c:1648  */
    break;

  case 220:
#line 1627 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.res_info) = (yyvsp[-1].res_info);
	    (yyval.res_info).memflags |= (yyvsp[0].memflags).on;
	    (yyval.res_info).memflags &=~ (yyvsp[0].memflags).off;
	  }
#line 3956 "rcparse.c" /* yacc.c:1648  */
    break;

  case 221:
#line 1639 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.memflags).on = MEMFLAG_MOVEABLE;
	    (yyval.memflags).off = 0;
	  }
#line 3965 "rcparse.c" /* yacc.c:1648  */
    break;

  case 222:
#line 1644 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.memflags).on = 0;
	    (yyval.memflags).off = MEMFLAG_MOVEABLE;
	  }
#line 3974 "rcparse.c" /* yacc.c:1648  */
    break;

  case 223:
#line 1649 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.memflags).on = MEMFLAG_PURE;
	    (yyval.memflags).off = 0;
	  }
#line 3983 "rcparse.c" /* yacc.c:1648  */
    break;

  case 224:
#line 1654 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.memflags).on = 0;
	    (yyval.memflags).off = MEMFLAG_PURE;
	  }
#line 3992 "rcparse.c" /* yacc.c:1648  */
    break;

  case 225:
#line 1659 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.memflags).on = MEMFLAG_PRELOAD;
	    (yyval.memflags).off = 0;
	  }
#line 4001 "rcparse.c" /* yacc.c:1648  */
    break;

  case 226:
#line 1664 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.memflags).on = 0;
	    (yyval.memflags).off = MEMFLAG_PRELOAD;
	  }
#line 4010 "rcparse.c" /* yacc.c:1648  */
    break;

  case 227:
#line 1669 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.memflags).on = MEMFLAG_DISCARDABLE;
	    (yyval.memflags).off = 0;
	  }
#line 4019 "rcparse.c" /* yacc.c:1648  */
    break;

  case 228:
#line 1679 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.s) = (yyvsp[0].s);
	  }
#line 4027 "rcparse.c" /* yacc.c:1648  */
    break;

  case 229:
#line 1683 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.s) = (yyvsp[0].s);
	  }
#line 4035 "rcparse.c" /* yacc.c:1648  */
    break;

  case 230:
#line 1691 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.uni) = (yyvsp[0].uni);
	  }
#line 4043 "rcparse.c" /* yacc.c:1648  */
    break;

  case 231:
#line 1696 "rcparse.y" /* yacc.c:1648  */
    {
	    rc_uint_type l1 = unichar_len ((yyvsp[-1].uni));
	    rc_uint_type l2 = unichar_len ((yyvsp[0].uni));
	    unichar *h = (unichar *) res_alloc ((l1 + l2 + 1) * sizeof (unichar));
	    if (l1 != 0)
	      memcpy (h, (yyvsp[-1].uni), l1 * sizeof (unichar));
	    if (l2 != 0)
	      memcpy (h + l1, (yyvsp[0].uni), l2  * sizeof (unichar));
	    h[l1 + l2] = 0;
	    (yyval.uni) = h;
	  }
#line 4059 "rcparse.c" /* yacc.c:1648  */
    break;

  case 232:
#line 1711 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.uni) = unichar_dup ((yyvsp[0].uni));
	  }
#line 4067 "rcparse.c" /* yacc.c:1648  */
    break;

  case 233:
#line 1715 "rcparse.y" /* yacc.c:1648  */
    {
	    unichar *h = NULL;
	    unicode_from_ascii ((rc_uint_type *) NULL, &h, (yyvsp[0].s));
	    (yyval.uni) = h;
	  }
#line 4077 "rcparse.c" /* yacc.c:1648  */
    break;

  case 234:
#line 1724 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.suni) = (yyvsp[0].suni);
	  }
#line 4085 "rcparse.c" /* yacc.c:1648  */
    break;

  case 235:
#line 1728 "rcparse.y" /* yacc.c:1648  */
    {
	    unichar *h = NULL;
	    rc_uint_type l = 0;
	    unicode_from_ascii_len (&l, &h, (yyvsp[0].ss).s, (yyvsp[0].ss).length);
	    (yyval.suni).s = h;
	    (yyval.suni).length = l;
	  }
#line 4097 "rcparse.c" /* yacc.c:1648  */
    break;

  case 236:
#line 1740 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.suni) = (yyvsp[0].suni);
	  }
#line 4105 "rcparse.c" /* yacc.c:1648  */
    break;

  case 237:
#line 1745 "rcparse.y" /* yacc.c:1648  */
    {
	    rc_uint_type l1 = (yyvsp[-1].suni).length;
	    rc_uint_type l2 = (yyvsp[0].suni).length;
	    unichar *h = (unichar *) res_alloc ((l1 + l2 + 1) * sizeof (unichar));
	    if (l1 != 0)
	      memcpy (h, (yyvsp[-1].suni).s, l1 * sizeof (unichar));
	    if (l2 != 0)
	      memcpy (h + l1, (yyvsp[0].suni).s, l2  * sizeof (unichar));
	    h[l1 + l2] = 0;
	    (yyval.suni).length = l1 + l2;
	    (yyval.suni).s = h;
	  }
#line 4122 "rcparse.c" /* yacc.c:1648  */
    break;

  case 238:
#line 1761 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.ss) = (yyvsp[0].ss);
	  }
#line 4130 "rcparse.c" /* yacc.c:1648  */
    break;

  case 239:
#line 1765 "rcparse.y" /* yacc.c:1648  */
    {
	    rc_uint_type l = (yyvsp[-1].ss).length + (yyvsp[0].ss).length;
	    char *h = (char *) res_alloc (l);
	    memcpy (h, (yyvsp[-1].ss).s, (yyvsp[-1].ss).length);
	    memcpy (h + (yyvsp[-1].ss).length, (yyvsp[0].ss).s, (yyvsp[0].ss).length);
	    (yyval.ss).s = h;
	    (yyval.ss).length = l;
	  }
#line 4143 "rcparse.c" /* yacc.c:1648  */
    break;

  case 240:
#line 1777 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.suni) = (yyvsp[0].suni);
	  }
#line 4151 "rcparse.c" /* yacc.c:1648  */
    break;

  case 241:
#line 1781 "rcparse.y" /* yacc.c:1648  */
    {
	    rc_uint_type l = (yyvsp[-1].suni).length + (yyvsp[0].suni).length;
	    unichar *h = (unichar *) res_alloc (l * sizeof (unichar));
	    memcpy (h, (yyvsp[-1].suni).s, (yyvsp[-1].suni).length * sizeof (unichar));
	    memcpy (h + (yyvsp[-1].suni).length, (yyvsp[0].suni).s, (yyvsp[0].suni).length  * sizeof (unichar));
	    (yyval.suni).s = h;
	    (yyval.suni).length = l;
	  }
#line 4164 "rcparse.c" /* yacc.c:1648  */
    break;

  case 242:
#line 1803 "rcparse.y" /* yacc.c:1648  */
    {
	    style |= (yyvsp[0].il);
	  }
#line 4172 "rcparse.c" /* yacc.c:1648  */
    break;

  case 243:
#line 1807 "rcparse.y" /* yacc.c:1648  */
    {
	    style &=~ (yyvsp[0].il);
	  }
#line 4180 "rcparse.c" /* yacc.c:1648  */
    break;

  case 244:
#line 1811 "rcparse.y" /* yacc.c:1648  */
    {
	    style |= (yyvsp[0].il);
	  }
#line 4188 "rcparse.c" /* yacc.c:1648  */
    break;

  case 245:
#line 1815 "rcparse.y" /* yacc.c:1648  */
    {
	    style &=~ (yyvsp[0].il);
	  }
#line 4196 "rcparse.c" /* yacc.c:1648  */
    break;

  case 246:
#line 1822 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.il) = (yyvsp[0].i).val;
	  }
#line 4204 "rcparse.c" /* yacc.c:1648  */
    break;

  case 247:
#line 1826 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.il) = (yyvsp[-1].il);
	  }
#line 4212 "rcparse.c" /* yacc.c:1648  */
    break;

  case 248:
#line 1835 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.il) = 0;
	  }
#line 4220 "rcparse.c" /* yacc.c:1648  */
    break;

  case 249:
#line 1839 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.il) = (yyvsp[0].il);
	  }
#line 4228 "rcparse.c" /* yacc.c:1648  */
    break;

  case 250:
#line 1848 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.il) = (yyvsp[0].il);
	  }
#line 4236 "rcparse.c" /* yacc.c:1648  */
    break;

  case 251:
#line 1857 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.il) = (yyvsp[0].i).val;
	  }
#line 4244 "rcparse.c" /* yacc.c:1648  */
    break;

  case 252:
#line 1866 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.i) = (yyvsp[0].i);
	  }
#line 4252 "rcparse.c" /* yacc.c:1648  */
    break;

  case 253:
#line 1870 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.i) = (yyvsp[-1].i);
	  }
#line 4260 "rcparse.c" /* yacc.c:1648  */
    break;

  case 254:
#line 1874 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.i).val = ~ (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[0].i).dword;
	  }
#line 4269 "rcparse.c" /* yacc.c:1648  */
    break;

  case 255:
#line 1879 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.i).val = - (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[0].i).dword;
	  }
#line 4278 "rcparse.c" /* yacc.c:1648  */
    break;

  case 256:
#line 1884 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.i).val = (yyvsp[-2].i).val * (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4287 "rcparse.c" /* yacc.c:1648  */
    break;

  case 257:
#line 1889 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.i).val = (yyvsp[-2].i).val / ((yyvsp[0].i).val ? (yyvsp[0].i).val : 1);
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4296 "rcparse.c" /* yacc.c:1648  */
    break;

  case 258:
#line 1894 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.i).val = (yyvsp[-2].i).val % ((yyvsp[0].i).val ? (yyvsp[0].i).val : 1);
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4305 "rcparse.c" /* yacc.c:1648  */
    break;

  case 259:
#line 1899 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.i).val = (yyvsp[-2].i).val + (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4314 "rcparse.c" /* yacc.c:1648  */
    break;

  case 260:
#line 1904 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.i).val = (yyvsp[-2].i).val - (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4323 "rcparse.c" /* yacc.c:1648  */
    break;

  case 261:
#line 1909 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.i).val = (yyvsp[-2].i).val & (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4332 "rcparse.c" /* yacc.c:1648  */
    break;

  case 262:
#line 1914 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.i).val = (yyvsp[-2].i).val ^ (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4341 "rcparse.c" /* yacc.c:1648  */
    break;

  case 263:
#line 1919 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.i).val = (yyvsp[-2].i).val | (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4350 "rcparse.c" /* yacc.c:1648  */
    break;

  case 264:
#line 1930 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.il) = (yyvsp[0].il);
	  }
#line 4358 "rcparse.c" /* yacc.c:1648  */
    break;

  case 265:
#line 1939 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.il) = (yyvsp[0].i).val;
	  }
#line 4366 "rcparse.c" /* yacc.c:1648  */
    break;

  case 266:
#line 1950 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.i) = (yyvsp[0].i);
	  }
#line 4374 "rcparse.c" /* yacc.c:1648  */
    break;

  case 267:
#line 1954 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.i) = (yyvsp[-1].i);
	  }
#line 4382 "rcparse.c" /* yacc.c:1648  */
    break;

  case 268:
#line 1958 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.i).val = ~ (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[0].i).dword;
	  }
#line 4391 "rcparse.c" /* yacc.c:1648  */
    break;

  case 269:
#line 1963 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.i).val = (yyvsp[-2].i).val * (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4400 "rcparse.c" /* yacc.c:1648  */
    break;

  case 270:
#line 1968 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.i).val = (yyvsp[-2].i).val / ((yyvsp[0].i).val ? (yyvsp[0].i).val : 1);
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4409 "rcparse.c" /* yacc.c:1648  */
    break;

  case 271:
#line 1973 "rcparse.y" /* yacc.c:1648  */
    {
	    /* PR 17512: file: 89105a25.  */
	    (yyval.i).val = (yyvsp[-2].i).val % ((yyvsp[0].i).val ? (yyvsp[0].i).val : 1);
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4419 "rcparse.c" /* yacc.c:1648  */
    break;

  case 272:
#line 1979 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.i).val = (yyvsp[-2].i).val + (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4428 "rcparse.c" /* yacc.c:1648  */
    break;

  case 273:
#line 1984 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.i).val = (yyvsp[-2].i).val - (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4437 "rcparse.c" /* yacc.c:1648  */
    break;

  case 274:
#line 1989 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.i).val = (yyvsp[-2].i).val & (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4446 "rcparse.c" /* yacc.c:1648  */
    break;

  case 275:
#line 1994 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.i).val = (yyvsp[-2].i).val ^ (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4455 "rcparse.c" /* yacc.c:1648  */
    break;

  case 276:
#line 1999 "rcparse.y" /* yacc.c:1648  */
    {
	    (yyval.i).val = (yyvsp[-2].i).val | (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4464 "rcparse.c" /* yacc.c:1648  */
    break;


#line 4468 "rcparse.c" /* yacc.c:1648  */
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
#line 2005 "rcparse.y" /* yacc.c:1907  */


/* Set the language from the command line.  */

void
rcparse_set_language (int lang)
{
  language = lang;
}
