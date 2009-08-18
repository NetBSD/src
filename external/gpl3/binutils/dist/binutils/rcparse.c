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
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
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




/* Copy the first part of user declarations.  */
#line 1 "rcparse.y"
 /* rcparse.y -- parser for Windows rc files
   Copyright 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2005, 2007
   Free Software Foundation, Inc.
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

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 69 "rcparse.y"
typedef union YYSTYPE {
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
} YYSTYPE;
/* Line 196 of yacc.c.  */
#line 393 "rcparse.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */
#line 405 "rcparse.c"

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
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short int yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short int) + sizeof (YYSTYPE))			\
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
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   835

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  112
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  99
/* YYNRULES -- Number of rules. */
#define YYNRULES  269
/* YYNRULES -- Number of states. */
#define YYNSTATES  515

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   353

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
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
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
{
       0,     0,     3,     4,     7,    10,    13,    16,    19,    22,
      25,    28,    31,    34,    37,    40,    43,    46,    49,    56,
      57,    60,    63,    68,    70,    72,    74,    78,    81,    83,
      85,    87,    89,    91,    93,    98,   103,   104,   118,   119,
     133,   134,   149,   150,   154,   155,   159,   163,   167,   171,
     175,   181,   188,   196,   205,   209,   213,   218,   222,   223,
     226,   227,   232,   233,   238,   239,   244,   245,   250,   251,
     256,   257,   261,   273,   286,   287,   292,   293,   298,   299,
     303,   304,   309,   310,   315,   322,   331,   342,   354,   355,
     360,   361,   365,   366,   371,   372,   377,   378,   383,   384,
     389,   390,   395,   396,   400,   401,   406,   407,   423,   430,
     439,   449,   452,   453,   456,   458,   460,   461,   465,   466,
     470,   471,   475,   476,   480,   485,   490,   494,   501,   502,
     505,   510,   513,   520,   521,   525,   528,   530,   532,   534,
     536,   538,   540,   547,   548,   551,   554,   558,   564,   567,
     573,   580,   588,   598,   603,   604,   607,   608,   610,   612,
     614,   616,   620,   624,   628,   629,   636,   637,   641,   646,
     649,   651,   653,   655,   657,   659,   661,   663,   665,   667,
     669,   676,   681,   690,   691,   695,   698,   705,   706,   713,
     720,   724,   728,   732,   736,   740,   741,   750,   758,   759,
     765,   766,   770,   772,   774,   776,   778,   781,   783,   786,
     787,   790,   794,   799,   803,   804,   807,   808,   811,   813,
     815,   817,   819,   821,   823,   825,   827,   829,   831,   834,
     836,   838,   840,   843,   845,   848,   850,   853,   857,   862,
     864,   868,   869,   871,   874,   876,   878,   882,   885,   888,
     892,   896,   900,   904,   908,   912,   916,   920,   923,   925,
     927,   931,   934,   938,   942,   946,   950,   954,   958,   962
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short int yyrhs[] =
{
     113,     0,    -1,    -1,   113,   114,    -1,   113,   120,    -1,
     113,   121,    -1,   113,   122,    -1,   113,   162,    -1,   113,
     163,    -1,   113,   164,    -1,   113,   165,    -1,   113,   170,
      -1,   113,   173,    -1,   113,   178,    -1,   113,   183,    -1,
     113,   182,    -1,   113,   185,    -1,   113,    97,    -1,   190,
       5,   193,     3,   115,     4,    -1,    -1,   115,   116,    -1,
     117,   208,    -1,   117,   208,   108,   118,    -1,    92,    -1,
     209,    -1,   119,    -1,   118,   108,   119,    -1,   118,   119,
      -1,     6,    -1,     7,    -1,     8,    -1,     9,    -1,    10,
      -1,    11,    -1,   190,    12,   195,   197,    -1,   190,    13,
     194,   197,    -1,    -1,   190,    14,   195,   126,   209,   205,
     205,   205,   123,   127,     3,   128,     4,    -1,    -1,   190,
      15,   195,   126,   209,   205,   205,   205,   124,   127,     3,
     128,     4,    -1,    -1,   190,    15,   195,   126,   209,   205,
     205,   205,   205,   125,   127,     3,   128,     4,    -1,    -1,
      16,   109,   206,    -1,    -1,   127,    17,   198,    -1,   127,
      18,   190,    -1,   127,    19,   202,    -1,   127,    16,   206,
      -1,   127,    18,   198,    -1,   127,    41,   206,   108,   198,
      -1,   127,    41,   206,   108,   198,   205,    -1,   127,    41,
     206,   108,   198,   205,   205,    -1,   127,    41,   206,   108,
     198,   205,   205,   205,    -1,   127,    57,   190,    -1,   127,
      55,   206,    -1,   127,    54,   206,   205,    -1,   127,    56,
     206,    -1,    -1,   128,   129,    -1,    -1,    20,   153,   130,
     151,    -1,    -1,    21,   153,   131,   151,    -1,    -1,    22,
     153,   132,   151,    -1,    -1,    38,   153,   133,   151,    -1,
      -1,    23,   153,   134,   151,    -1,    -1,    24,   135,   151,
      -1,    10,   153,   206,   152,   156,   205,   205,   205,   205,
     204,   155,    -1,    10,   153,   206,   152,   156,   205,   205,
     205,   205,   205,   205,   155,    -1,    -1,    25,   153,   136,
     151,    -1,    -1,    26,   153,   137,   151,    -1,    -1,    27,
     138,   151,    -1,    -1,    28,   153,   139,   151,    -1,    -1,
      39,   153,   140,   151,    -1,    42,   192,   206,   205,   205,
     155,    -1,    42,   192,   206,   205,   205,   205,   205,   155,
      -1,    42,   192,   206,   205,   205,   205,   205,   158,   204,
     155,    -1,    42,   192,   206,   205,   205,   205,   205,   158,
     205,   205,   155,    -1,    -1,    40,   153,   141,   151,    -1,
      -1,    29,   142,   151,    -1,    -1,    30,   153,   143,   151,
      -1,    -1,    31,   153,   144,   151,    -1,    -1,    32,   153,
     145,   151,    -1,    -1,    33,   153,   146,   151,    -1,    -1,
      34,   153,   147,   151,    -1,    -1,    35,   148,   151,    -1,
      -1,    36,   153,   149,   151,    -1,    -1,    37,   192,   206,
     108,   206,   108,   206,   108,   206,   108,   206,   108,   150,
     202,   204,    -1,   206,   205,   205,   205,   205,   155,    -1,
     206,   205,   205,   205,   205,   160,   204,   155,    -1,   206,
     205,   205,   205,   205,   160,   205,   205,   155,    -1,   108,
     154,    -1,    -1,   154,   108,    -1,   209,    -1,   198,    -1,
      -1,     3,   174,     4,    -1,    -1,   108,   157,   202,    -1,
      -1,   108,   159,   202,    -1,    -1,   108,   161,   202,    -1,
     190,    41,   194,   197,    -1,   190,    42,   194,   197,    -1,
      54,   206,   205,    -1,   190,    57,   193,     3,   166,     4,
      -1,    -1,   166,   167,    -1,    59,   198,   205,   168,    -1,
      59,    60,    -1,    61,   198,   168,     3,   166,     4,    -1,
      -1,   168,   108,   169,    -1,   168,   169,    -1,    62,    -1,
      63,    -1,    64,    -1,    65,    -1,    66,    -1,    67,    -1,
     190,    58,   193,     3,   171,     4,    -1,    -1,   171,   172,
      -1,    59,   198,    -1,    59,   198,   205,    -1,    59,   198,
     205,   205,   204,    -1,    59,    60,    -1,    61,   198,     3,
     171,     4,    -1,    61,   198,   205,     3,   171,     4,    -1,
      61,   198,   205,   205,     3,   171,     4,    -1,    61,   198,
     205,   205,   205,   204,     3,   171,     4,    -1,   190,    68,
     195,   197,    -1,    -1,   175,   176,    -1,    -1,   177,    -1,
     200,    -1,   201,    -1,   207,    -1,   177,   108,   200,    -1,
     177,   108,   201,    -1,   177,   108,   207,    -1,    -1,    70,
     193,     3,   179,   180,     4,    -1,    -1,   180,   206,   198,
      -1,   180,   206,   108,   198,    -1,   180,     1,    -1,   190,
      -1,    48,    -1,    69,    -1,    49,    -1,    50,    -1,    51,
      -1,    45,    -1,    46,    -1,    43,    -1,    44,    -1,   190,
     181,   193,     3,   174,     4,    -1,   190,   181,   193,   197,
      -1,   190,    52,   193,   206,   205,     3,   184,     4,    -1,
      -1,   184,    53,   190,    -1,   184,    60,    -1,   190,    71,
     186,     3,   187,     4,    -1,    -1,   186,    72,   206,   205,
     205,   205,    -1,   186,    73,   206,   205,   205,   205,    -1,
     186,    74,   206,    -1,   186,    75,   206,    -1,   186,    76,
     206,    -1,   186,    77,   206,    -1,   186,    78,   206,    -1,
      -1,   187,    79,     3,    82,     3,   188,     4,     4,    -1,
     187,    80,     3,    81,   198,   189,     4,    -1,    -1,   188,
      81,   198,   108,   198,    -1,    -1,   189,   205,   205,    -1,
     209,    -1,   191,    -1,   199,    -1,    93,    -1,   209,   108,
      -1,   191,    -1,   191,   108,    -1,    -1,   193,   196,    -1,
     193,    55,   206,    -1,   193,    54,   206,   205,    -1,   193,
      56,   206,    -1,    -1,   194,   196,    -1,    -1,   195,   196,
      -1,    83,    -1,    84,    -1,    85,    -1,    86,    -1,    87,
      -1,    88,    -1,    89,    -1,    92,    -1,    93,    -1,   199,
      -1,   198,   199,    -1,    91,    -1,    92,    -1,    96,    -1,
     200,    96,    -1,    95,    -1,   201,    95,    -1,   203,    -1,
      90,   203,    -1,   202,    98,   203,    -1,   202,    98,    90,
     203,    -1,    94,    -1,   110,   206,   111,    -1,    -1,   205,
      -1,   108,   206,    -1,   207,    -1,    94,    -1,   110,   207,
     111,    -1,   106,   207,    -1,   102,   207,    -1,   207,   103,
     207,    -1,   207,   104,   207,    -1,   207,   105,   207,    -1,
     207,   101,   207,    -1,   207,   102,   207,    -1,   207,   100,
     207,    -1,   207,    99,   207,    -1,   207,    98,   207,    -1,
     108,   209,    -1,   210,    -1,    94,    -1,   110,   207,   111,
      -1,   106,   207,    -1,   210,   103,   207,    -1,   210,   104,
     207,    -1,   210,   105,   207,    -1,   210,   101,   207,    -1,
     210,   102,   207,    -1,   210,   100,   207,    -1,   210,    99,
     207,    -1,   210,    98,   207,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   177,   177,   179,   180,   181,   182,   183,   184,   185,
     186,   187,   188,   189,   190,   191,   192,   193,   199,   210,
     213,   234,   239,   251,   271,   281,   285,   290,   297,   301,
     306,   310,   314,   318,   327,   339,   353,   351,   378,   376,
     405,   403,   435,   438,   444,   446,   452,   456,   461,   465,
     469,   482,   497,   512,   527,   531,   535,   539,   545,   547,
     559,   558,   571,   570,   583,   582,   595,   594,   610,   609,
     622,   621,   635,   646,   656,   655,   668,   667,   680,   679,
     692,   691,   704,   703,   718,   723,   729,   735,   742,   741,
     757,   756,   769,   768,   781,   780,   792,   791,   804,   803,
     816,   815,   828,   827,   840,   839,   853,   851,   872,   883,
     894,   906,   917,   920,   924,   929,   939,   942,   952,   951,
     958,   957,   964,   963,   971,   983,   996,  1005,  1016,  1019,
    1036,  1040,  1044,  1052,  1055,  1059,  1066,  1070,  1074,  1078,
    1082,  1086,  1095,  1106,  1109,  1126,  1130,  1134,  1138,  1142,
    1146,  1150,  1154,  1164,  1177,  1177,  1189,  1193,  1200,  1208,
    1216,  1224,  1233,  1242,  1257,  1256,  1261,  1263,  1268,  1273,
    1281,  1285,  1290,  1295,  1300,  1305,  1310,  1315,  1320,  1325,
    1336,  1343,  1353,  1359,  1360,  1379,  1404,  1415,  1420,  1426,
    1432,  1437,  1442,  1447,  1452,  1467,  1470,  1474,  1482,  1485,
    1493,  1496,  1505,  1510,  1519,  1523,  1533,  1538,  1542,  1553,
    1559,  1565,  1570,  1575,  1586,  1591,  1603,  1608,  1620,  1625,
    1630,  1635,  1640,  1645,  1650,  1660,  1664,  1672,  1677,  1692,
    1696,  1705,  1709,  1721,  1725,  1747,  1751,  1755,  1759,  1766,
    1770,  1780,  1783,  1792,  1801,  1810,  1814,  1818,  1823,  1828,
    1833,  1838,  1843,  1848,  1853,  1858,  1863,  1874,  1883,  1894,
    1898,  1902,  1907,  1912,  1917,  1922,  1927,  1932,  1937,  1942
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
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
  "cursor", "dialog", "@1", "@2", "@3", "exstyle", "styles", "controls",
  "control", "@4", "@5", "@6", "@7", "@8", "@9", "@10", "@11", "@12",
  "@13", "@14", "@15", "@16", "@17", "@18", "@19", "@20", "@21", "@22",
  "@23", "@24", "control_params", "cresid", "optresidc", "resid",
  "opt_control_data", "control_styleexpr", "@25", "icon_styleexpr", "@26",
  "control_params_styleexpr", "@27", "font", "icon", "language", "menu",
  "menuitems", "menuitem", "menuitem_flags", "menuitem_flag", "menuex",
  "menuexitems", "menuexitem", "messagetable", "optrcdata_data", "@28",
  "optrcdata_data_int", "rcdata_data", "stringtable", "@29", "string_data",
  "rcdata_id", "user", "toolbar", "toolbar_data", "versioninfo",
  "fixedverinfo", "verblocks", "vervals", "vertrans", "id", "resname",
  "resref", "suboptions", "memflags_move_discard", "memflags_move",
  "memflag", "file_name", "res_unicode_string_concat",
  "res_unicode_string", "sizedstring", "sizedunistring", "styleexpr",
  "parennumber", "optcnumexpr", "cnumexpr", "numexpr", "sizednumexpr",
  "cposnumexpr", "posnumexpr", "sizedposnumexpr", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short int yytoknum[] =
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

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
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
     177,   177,   177,   177,   179,   178,   180,   180,   180,   180,
     181,   181,   181,   181,   181,   181,   181,   181,   181,   181,
     182,   182,   183,   184,   184,   184,   185,   186,   186,   186,
     186,   186,   186,   186,   186,   187,   187,   187,   188,   188,
     189,   189,   190,   190,   191,   191,   192,   192,   192,   193,
     193,   193,   193,   193,   194,   194,   195,   195,   196,   196,
     196,   196,   196,   196,   196,   197,   197,   198,   198,   199,
     199,   200,   200,   201,   201,   202,   202,   202,   202,   203,
     203,   204,   204,   205,   206,   207,   207,   207,   207,   207,
     207,   207,   207,   207,   207,   207,   207,   208,   209,   210,
     210,   210,   210,   210,   210,   210,   210,   210,   210,   210
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
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
       1,     3,     3,     3,     0,     6,     0,     3,     4,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       6,     4,     8,     0,     3,     2,     6,     0,     6,     6,
       3,     3,     3,     3,     3,     0,     8,     7,     0,     5,
       0,     3,     1,     1,     1,     1,     2,     1,     2,     0,
       2,     3,     4,     3,     0,     2,     0,     2,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     2,     1,
       1,     1,     2,     1,     2,     1,     2,     3,     4,     1,
       3,     0,     1,     2,     1,     1,     3,     2,     2,     3,
       3,     3,     3,     3,     3,     3,     3,     2,     1,     1,
       3,     2,     3,     3,     3,     3,     3,     3,     3,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned short int yydefact[] =
{
       2,     0,     1,     0,   209,   229,   230,   205,   259,    17,
       0,     0,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    15,    14,    16,     0,   203,   204,   202,
     258,   245,     0,     0,     0,     0,   244,     0,   261,     0,
     209,   216,   214,   216,   216,   214,   214,   178,   179,   176,
     177,   171,   173,   174,   175,   209,   209,   209,   216,   172,
     187,   209,   170,     0,     0,     0,     0,     0,     0,     0,
       0,   248,   247,     0,     0,   126,     0,     0,     0,     0,
       0,     0,     0,     0,   164,     0,     0,     0,   218,   219,
     220,   221,   222,   223,   224,   210,   260,     0,     0,     0,
      42,    42,     0,     0,     0,     0,     0,     0,     0,     0,
     269,   268,   267,   265,   266,   262,   263,   264,   246,   243,
     256,   255,   254,   252,   253,   249,   250,   251,   166,     0,
     211,   213,    19,   225,   226,   217,    34,   215,    35,     0,
       0,     0,   124,   125,     0,   128,   143,   153,   195,     0,
       0,     0,     0,     0,     0,     0,   154,   181,     0,   212,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     190,   191,   192,   193,   194,     0,   156,   169,   165,     0,
      18,    23,    20,     0,    24,    43,     0,     0,   183,   127,
       0,     0,   129,   142,     0,     0,   144,   186,     0,     0,
       0,     0,   180,   233,   231,   155,   157,   158,   159,   160,
       0,   167,   227,     0,    21,     0,     0,     0,   131,     0,
     133,   148,   145,     0,     0,     0,     0,     0,     0,   232,
     234,   168,   228,   257,     0,    36,    38,   182,     0,   185,
     133,     0,   146,   143,     0,     0,     0,   188,   189,   161,
     162,   163,    28,    29,    30,    31,    32,    33,    22,    25,
      44,    44,    40,   184,   130,   128,   136,   137,   138,   139,
     140,   141,     0,   135,   241,     0,   143,     0,   198,   200,
       0,    27,     0,     0,    44,     0,   134,   147,   242,   149,
       0,   143,   241,     0,     0,    26,    58,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    58,     0,   132,   150,
       0,     0,     0,     0,   197,     0,     0,    48,    45,    46,
      49,   204,     0,   239,     0,    47,   235,     0,     0,    55,
      57,    54,     0,    58,   151,   143,   196,     0,   201,    37,
     112,   112,   112,   112,   112,    70,   112,   112,    78,   112,
      90,   112,   112,   112,   112,   112,   102,   112,     0,   112,
     112,   112,     0,    59,   236,     0,     0,     0,    56,    39,
       0,     0,     0,     0,     0,   115,   114,    60,    62,    64,
      68,     0,    74,    76,     0,    80,     0,    92,    94,    96,
      98,   100,     0,   104,   207,     0,     0,    66,    82,    88,
       0,   240,     0,   237,    50,    41,   152,   199,     0,   113,
       0,     0,     0,     0,    71,     0,     0,     0,    79,     0,
      91,     0,     0,     0,     0,     0,   103,     0,   208,     0,
     206,     0,     0,     0,     0,   238,    51,     0,     0,    61,
      63,    65,    69,     0,    75,    77,    81,    93,    95,    97,
      99,   101,   105,     0,    67,    83,    89,     0,    52,   111,
     118,     0,     0,     0,   116,    53,     0,     0,     0,     0,
     154,    84,     0,   119,     0,   116,     0,     0,   116,     0,
     122,   108,   241,     0,   117,   120,    85,   241,   241,     0,
     116,   242,     0,     0,   116,   242,   116,   242,   123,   109,
     116,     0,   121,    86,   116,    72,   116,   110,     0,    87,
      73,   106,     0,   241,   107
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,     1,    12,   160,   182,   183,   258,   259,    13,    14,
      15,   260,   261,   284,   140,   282,   316,   363,   410,   411,
     412,   431,   413,   381,   416,   417,   384,   419,   432,   433,
     386,   421,   422,   423,   424,   425,   392,   427,   512,   414,
     438,   373,   374,   471,   461,   466,   487,   493,   482,   489,
      16,    17,    18,    19,   165,   192,   241,   273,    20,   166,
     196,    21,   175,   176,   205,   206,    22,   128,   158,    61,
      23,    24,   217,    25,   108,   167,   293,   294,    26,    27,
     395,    37,    99,    98,    95,   136,   375,   212,   207,   208,
     325,   326,   287,   288,   415,    36,   214,   376,    30
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -309
static const short int yypact[] =
{
    -309,    68,  -309,   338,  -309,  -309,  -309,  -309,  -309,  -309,
     338,   338,  -309,  -309,  -309,  -309,  -309,  -309,  -309,  -309,
    -309,  -309,  -309,  -309,  -309,  -309,   458,  -309,  -309,  -309,
     605,  -309,   338,   338,   338,   -92,   642,   230,  -309,   534,
    -309,  -309,  -309,  -309,  -309,  -309,  -309,  -309,  -309,  -309,
    -309,  -309,  -309,  -309,  -309,  -309,  -309,  -309,  -309,  -309,
    -309,  -309,  -309,   338,   338,   338,   338,   338,   338,   338,
     338,  -309,  -309,   695,   338,  -309,   338,   338,   338,   338,
     338,   338,   338,   338,  -309,   338,   338,   338,  -309,  -309,
    -309,  -309,  -309,  -309,  -309,  -309,  -309,   329,   724,   724,
     242,   242,   724,   724,   499,   434,   457,   724,   192,   250,
     392,   718,   318,   174,   174,  -309,  -309,  -309,  -309,  -309,
     392,   718,   318,   174,   174,  -309,  -309,  -309,  -309,   -92,
    -309,  -309,  -309,  -309,  -309,  -309,  -309,  -309,  -309,   -81,
     263,   263,  -309,  -309,   -92,  -309,  -309,  -309,  -309,   338,
     338,   338,   338,   338,   338,   338,  -309,  -309,     6,  -309,
      13,   338,   -92,   -92,    48,     8,   105,    35,   -92,   -92,
    -309,  -309,  -309,  -309,  -309,    53,   373,  -309,  -309,   -38,
    -309,  -309,  -309,   -48,  -309,  -309,   -92,   -92,  -309,  -309,
     -36,     7,  -309,  -309,    80,     7,  -309,  -309,    60,   103,
     -92,   -92,  -309,  -309,  -309,  -309,    17,    38,    47,   642,
       7,     7,  -309,   263,    65,   -92,   -92,    -1,  -309,   163,
       7,  -309,   163,    12,    74,    94,   -92,   -92,   373,  -309,
    -309,     7,  -309,  -309,   818,  -309,   -92,  -309,   253,  -309,
    -309,   184,   -92,  -309,     5,   177,     7,  -309,  -309,    38,
      47,   642,  -309,  -309,  -309,  -309,  -309,  -309,    25,  -309,
    -309,  -309,  -309,  -309,   155,  -309,  -309,  -309,  -309,  -309,
    -309,  -309,   768,  -309,   -92,   120,  -309,    10,  -309,     7,
     818,  -309,   556,   562,  -309,   137,  -309,  -309,  -309,  -309,
     141,  -309,   -92,    21,     2,  -309,  -309,   338,     7,   253,
     -46,   338,   338,   338,   338,   253,  -309,   573,  -309,  -309,
     153,   188,   172,     7,  -309,   -92,   655,  -309,     7,  -309,
       7,    40,    27,  -309,   338,    99,  -309,    93,   -92,  -309,
    -309,  -309,   692,  -309,  -309,  -309,  -309,   168,  -309,  -309,
     258,   258,   258,   258,   258,  -309,   258,   258,  -309,   258,
    -309,   258,   258,   258,   258,   258,  -309,   258,   253,   258,
     258,   258,   253,  -309,  -309,    95,    98,     7,  -309,  -309,
     729,   173,     7,   338,   102,     7,  -309,  -309,  -309,  -309,
    -309,   338,  -309,  -309,   338,  -309,   338,  -309,  -309,  -309,
    -309,  -309,   338,  -309,   117,   338,   123,  -309,  -309,  -309,
     338,  -309,    27,  -309,   163,  -309,  -309,     7,   128,  -309,
     338,   338,   338,   338,  -309,   -92,   338,   338,  -309,   338,
    -309,   338,   338,   338,   338,   338,  -309,   338,  -309,   131,
    -309,   338,   338,   338,   -92,  -309,   -92,   258,   132,  -309,
    -309,  -309,  -309,   -92,  -309,  -309,  -309,  -309,  -309,  -309,
    -309,  -309,  -309,   338,  -309,  -309,  -309,   -92,   -92,  -309,
    -309,   -92,   -92,   149,    18,  -309,   -46,   -92,   -92,   338,
    -309,  -309,   -92,    99,   -92,    19,   154,   203,    20,   -92,
    -309,  -309,   -92,   338,  -309,  -309,  -309,   -92,   -92,   -46,
     225,   -92,   165,   -46,   225,   -92,   225,   -92,    99,  -309,
     225,   338,    99,  -309,   225,  -309,   225,  -309,   182,  -309,
    -309,  -309,   -46,   -71,  -309
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -309,  -309,  -309,  -309,  -309,  -309,  -309,  -240,  -309,  -309,
    -309,  -309,  -309,  -309,   144,  -235,  -295,  -309,  -309,  -309,
    -309,  -309,  -309,  -309,  -309,  -309,  -309,  -309,  -309,  -309,
    -309,  -309,  -309,  -309,  -309,  -309,  -309,  -309,  -309,   239,
    -309,   431,  -156,  -100,  -309,  -309,  -309,  -309,  -309,  -309,
    -309,  -309,  -309,  -309,    26,  -309,    56,    39,  -309,  -196,
    -309,  -309,  -173,  -309,  -309,  -309,  -309,  -309,  -309,  -309,
    -309,  -309,  -309,  -309,  -309,  -309,  -309,  -309,   -25,  -265,
     -55,   232,     0,   333,   432,   375,  -129,     4,    82,    84,
    -237,  -308,  -283,   -33,    -3,     9,  -309,     3,  -309
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -228
static const short int yytable[] =
{
      35,    62,    75,   237,    29,    28,   314,   177,   276,   311,
     178,   332,   189,   291,   364,   243,    74,   180,   281,    38,
      39,   470,   470,   470,   218,   312,   283,   366,   161,    29,
      28,   252,   253,   254,   255,   256,   257,    74,   370,   197,
     295,    71,    72,    73,   322,   102,   103,   275,   323,   307,
     211,   188,   238,     5,     6,     5,     6,   202,   403,   239,
     213,   219,   220,   224,   324,   222,   223,   190,     2,   191,
     210,   119,   110,   111,   112,   113,   114,   115,   116,   117,
     290,   231,   129,   130,   131,   120,   121,   122,   123,   124,
     125,   126,   127,   394,   435,   310,   159,   394,     5,     6,
      31,   144,   313,     5,     6,   181,   225,     8,    32,   193,
      74,   164,    33,    74,   198,   199,    34,   279,    74,    10,
      74,   323,     3,    11,   289,   228,    74,   480,   485,   186,
     187,  -227,  -227,   280,   229,   200,   201,   324,     4,   371,
     221,   308,   230,   162,   163,   309,   168,   169,   170,   171,
     172,   173,   174,   215,   216,   179,   245,   334,   185,     5,
       6,     7,     8,   184,   194,     9,   195,   226,   227,   318,
     320,     5,     6,   234,    10,   246,   336,   406,    11,   194,
     278,   195,   235,   236,   337,   209,   240,   265,   402,   242,
     244,   335,   323,   247,   248,   148,   190,   366,   191,   490,
     194,   367,   195,   262,   494,   496,   401,   484,   324,   274,
     409,   277,   194,   263,   195,   232,   233,   266,   267,   268,
     269,   270,   271,   232,   232,   428,   232,   232,   470,   473,
     514,   430,   194,    84,   195,   232,   437,   251,   404,   453,
     460,    29,    28,   407,   292,   141,   266,   267,   268,   269,
     270,   271,   498,   156,     5,     6,   502,   469,   139,     5,
       6,   315,   483,   272,   149,   150,   151,   152,   153,   154,
     155,    74,    97,   501,   319,   513,   372,    81,    82,    83,
     331,   459,   338,   232,    85,    86,    87,   104,   105,   106,
     511,   285,   272,   109,   317,   368,   264,   477,   327,   328,
     329,   330,    29,   321,    85,    86,    87,   400,    29,    28,
     249,   286,   250,    88,    89,    90,    91,    92,    93,    94,
       0,   365,   232,     0,   232,    88,    89,    90,    91,    92,
      93,    94,   132,    88,    89,    90,    91,    92,    93,    94,
       0,   232,   133,   134,     5,     6,     7,     8,     0,     5,
       6,     0,     8,     0,     0,     0,     0,     8,     0,    10,
       0,   396,    28,    11,    10,   396,    28,     0,    11,    10,
     408,   436,     0,    11,     0,   481,   100,   101,   486,   232,
       0,     0,   443,    85,    86,    87,     0,     0,     0,     0,
     499,   107,   429,     0,   503,     0,   505,   434,     0,     0,
     507,   457,     0,   458,   509,     0,   510,     0,   232,     0,
     462,   232,    88,    89,    90,    91,    92,    93,    94,    79,
      80,    81,    82,    83,   464,   465,     0,     0,   467,   468,
       0,   472,    31,     0,   474,   475,     0,   145,     0,   478,
      32,   479,     0,     0,    33,     0,   488,     0,    34,   491,
     463,     0,     0,     0,   495,   497,     0,     0,   500,     0,
     146,     0,   504,    40,   506,     0,   476,    31,   203,   204,
      41,    42,    43,    44,   138,    32,     0,   142,   143,    33,
     492,     0,   147,    34,   157,     0,     0,     0,    85,    86,
      87,    77,    78,    79,    80,    81,    82,    83,   508,    45,
      46,    47,    48,    49,    50,     0,    51,    52,    53,    54,
      55,    85,    86,    87,     0,    56,    57,    88,    89,    90,
      91,    92,    93,    94,     0,     0,    58,    59,     0,    60,
     135,   137,   135,   135,   137,   137,     0,     0,     0,   135,
      88,    89,    90,    91,    92,    93,    94,     0,     0,     5,
       6,     7,     8,    85,    86,    87,     0,     0,     0,   296,
       0,     0,     0,     0,    10,   306,     0,     0,    11,     0,
       0,     0,   297,   298,   299,   300,   333,     0,   297,   298,
     299,   300,    88,    89,    90,    91,    92,    93,    94,   297,
     298,   299,   300,    31,     0,     0,     0,   301,     0,     0,
       0,    32,     0,   301,     0,    33,     0,     0,     0,    34,
     302,   303,   304,   305,   301,     0,   302,   303,   304,   305,
       0,     0,     0,   418,     0,   420,     0,   302,   303,   304,
     305,   426,    76,    77,    78,    79,    80,    81,    82,    83,
       0,     0,     0,     0,     0,    96,     0,     0,     0,   439,
     440,   441,   442,     0,     0,   444,   445,     0,   446,   339,
     447,   448,   449,   450,   451,   340,   452,     0,     0,     0,
     454,   455,   456,     0,     0,   341,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,   355,
     356,   357,   358,   359,   360,   361,   369,   362,     0,     0,
       0,     0,   340,    63,    64,    65,    66,    67,    68,    69,
      70,     0,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   405,   362,     0,     0,     0,     0,   340,
      76,    77,    78,    79,    80,    81,    82,    83,     0,   341,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,   355,   356,   357,   358,   359,   360,   361,
       0,   362,   377,   378,   379,   380,     0,   382,   383,     0,
     385,     0,   387,   388,   389,   390,   391,     0,   393,     0,
     397,   398,   399,    76,    77,    78,    79,    80,    81,    82,
      83,     0,     0,     0,     0,     0,   118,    88,    89,    90,
      91,    92,    93,    94,     0,     0,   133,   134,    78,    79,
      80,    81,    82,    83,   252,   253,   254,   255,   256,   257,
     266,   267,   268,   269,   270,   271
};

static const short int yycheck[] =
{
       3,    26,    35,     4,     1,     1,     4,     1,     3,   292,
       4,   306,     4,     3,   322,     3,   108,     4,   258,    10,
      11,     3,     3,     3,    60,     4,   261,    98,   109,    26,
      26,     6,     7,     8,     9,    10,    11,   108,   333,     4,
     280,    32,    33,    34,    90,    45,    46,   243,    94,   284,
     179,     3,    53,    91,    92,    91,    92,     4,   366,    60,
     108,   190,   191,     3,   110,   194,   195,    59,     0,    61,
     108,    74,    63,    64,    65,    66,    67,    68,    69,    70,
     276,   210,    85,    86,    87,    76,    77,    78,    79,    80,
      81,    82,    83,   358,   402,   291,   129,   362,    91,    92,
      94,   104,    81,    91,    92,    92,     3,    94,   102,     4,
     108,   144,   106,   108,    79,    80,   110,   246,   108,   106,
     108,    94,    54,   110,     4,   108,   108,   108,   108,   162,
     163,    91,    92,   108,    96,   168,   169,   110,    70,   335,
      60,     4,    95,   140,   141,     4,   149,   150,   151,   152,
     153,   154,   155,   186,   187,   158,    82,     4,   161,    91,
      92,    93,    94,   160,    59,    97,    61,   200,   201,   298,
     299,    91,    92,   108,   106,    81,     4,     4,   110,    59,
       3,    61,   215,   216,   313,   176,   219,     3,    90,   222,
     223,     3,    94,   226,   227,     3,    59,    98,    61,   482,
      59,   108,    61,   236,   487,   488,   111,     4,   110,   242,
     108,   244,    59,   238,    61,   211,   213,    62,    63,    64,
      65,    66,    67,   219,   220,   108,   222,   223,     3,   466,
     513,   108,    59,     3,    61,   231,   108,   228,   367,   108,
     108,   238,   238,   372,   277,   101,    62,    63,    64,    65,
      66,    67,   489,     3,    91,    92,   493,   108,    16,    91,
      92,   294,   108,   108,    72,    73,    74,    75,    76,    77,
      78,   108,    40,   108,   299,   512,   108,   103,   104,   105,
     305,   437,   315,   279,    54,    55,    56,    55,    56,    57,
     108,   265,   108,    61,   297,   328,   240,   470,   301,   302,
     303,   304,   299,   299,    54,    55,    56,   362,   305,   305,
     228,   272,   228,    83,    84,    85,    86,    87,    88,    89,
      -1,   324,   318,    -1,   320,    83,    84,    85,    86,    87,
      88,    89,     3,    83,    84,    85,    86,    87,    88,    89,
      -1,   337,    92,    93,    91,    92,    93,    94,    -1,    91,
      92,    -1,    94,    -1,    -1,    -1,    -1,    94,    -1,   106,
      -1,   358,   358,   110,   106,   362,   362,    -1,   110,   106,
     373,   404,    -1,   110,    -1,   475,    43,    44,   478,   375,
      -1,    -1,   415,    54,    55,    56,    -1,    -1,    -1,    -1,
     490,    58,   395,    -1,   494,    -1,   496,   400,    -1,    -1,
     500,   434,    -1,   436,   504,    -1,   506,    -1,   404,    -1,
     443,   407,    83,    84,    85,    86,    87,    88,    89,   101,
     102,   103,   104,   105,   457,   458,    -1,    -1,   461,   462,
      -1,   464,    94,    -1,   467,   468,    -1,     3,    -1,   472,
     102,   474,    -1,    -1,   106,    -1,   479,    -1,   110,   482,
     453,    -1,    -1,    -1,   487,   488,    -1,    -1,   491,    -1,
       3,    -1,   495,     5,   497,    -1,   469,    94,    95,    96,
      12,    13,    14,    15,    99,   102,    -1,   102,   103,   106,
     483,    -1,   107,   110,   109,    -1,    -1,    -1,    54,    55,
      56,    99,   100,   101,   102,   103,   104,   105,   501,    41,
      42,    43,    44,    45,    46,    -1,    48,    49,    50,    51,
      52,    54,    55,    56,    -1,    57,    58,    83,    84,    85,
      86,    87,    88,    89,    -1,    -1,    68,    69,    -1,    71,
      98,    99,   100,   101,   102,   103,    -1,    -1,    -1,   107,
      83,    84,    85,    86,    87,    88,    89,    -1,    -1,    91,
      92,    93,    94,    54,    55,    56,    -1,    -1,    -1,     3,
      -1,    -1,    -1,    -1,   106,     3,    -1,    -1,   110,    -1,
      -1,    -1,    16,    17,    18,    19,     3,    -1,    16,    17,
      18,    19,    83,    84,    85,    86,    87,    88,    89,    16,
      17,    18,    19,    94,    -1,    -1,    -1,    41,    -1,    -1,
      -1,   102,    -1,    41,    -1,   106,    -1,    -1,    -1,   110,
      54,    55,    56,    57,    41,    -1,    54,    55,    56,    57,
      -1,    -1,    -1,   384,    -1,   386,    -1,    54,    55,    56,
      57,   392,    98,    99,   100,   101,   102,   103,   104,   105,
      -1,    -1,    -1,    -1,    -1,   111,    -1,    -1,    -1,   410,
     411,   412,   413,    -1,    -1,   416,   417,    -1,   419,     4,
     421,   422,   423,   424,   425,    10,   427,    -1,    -1,    -1,
     431,   432,   433,    -1,    -1,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,     4,    42,    -1,    -1,
      -1,    -1,    10,    98,    99,   100,   101,   102,   103,   104,
     105,    -1,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,     4,    42,    -1,    -1,    -1,    -1,    10,
      98,    99,   100,   101,   102,   103,   104,   105,    -1,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      -1,    42,   341,   342,   343,   344,    -1,   346,   347,    -1,
     349,    -1,   351,   352,   353,   354,   355,    -1,   357,    -1,
     359,   360,   361,    98,    99,   100,   101,   102,   103,   104,
     105,    -1,    -1,    -1,    -1,    -1,   111,    83,    84,    85,
      86,    87,    88,    89,    -1,    -1,    92,    93,   100,   101,
     102,   103,   104,   105,     6,     7,     8,     9,    10,    11,
      62,    63,    64,    65,    66,    67
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,   113,     0,    54,    70,    91,    92,    93,    94,    97,
     106,   110,   114,   120,   121,   122,   162,   163,   164,   165,
     170,   173,   178,   182,   183,   185,   190,   191,   199,   209,
     210,    94,   102,   106,   110,   206,   207,   193,   207,   207,
       5,    12,    13,    14,    15,    41,    42,    43,    44,    45,
      46,    48,    49,    50,    51,    52,    57,    58,    68,    69,
      71,   181,   190,    98,    99,   100,   101,   102,   103,   104,
     105,   207,   207,   207,   108,   205,    98,    99,   100,   101,
     102,   103,   104,   105,     3,    54,    55,    56,    83,    84,
      85,    86,    87,    88,    89,   196,   111,   193,   195,   194,
     195,   195,   194,   194,   193,   193,   193,   195,   186,   193,
     207,   207,   207,   207,   207,   207,   207,   207,   111,   206,
     207,   207,   207,   207,   207,   207,   207,   207,   179,   206,
     206,   206,     3,    92,    93,   196,   197,   196,   197,    16,
     126,   126,   197,   197,   206,     3,     3,   197,     3,    72,
      73,    74,    75,    76,    77,    78,     3,   197,   180,   205,
     115,   109,   209,   209,   205,   166,   171,   187,   206,   206,
     206,   206,   206,   206,   206,   174,   175,     1,     4,   206,
       4,    92,   116,   117,   209,   206,   205,   205,     3,     4,
      59,    61,   167,     4,    59,    61,   172,     4,    79,    80,
     205,   205,     4,    95,    96,   176,   177,   200,   201,   207,
     108,   198,   199,   108,   208,   205,   205,   184,    60,   198,
     198,    60,   198,   198,     3,     3,   205,   205,   108,    96,
      95,   198,   199,   209,   108,   205,   205,     4,    53,    60,
     205,   168,   205,     3,   205,    82,    81,   205,   205,   200,
     201,   207,     6,     7,     8,     9,    10,    11,   118,   119,
     123,   124,   205,   190,   168,     3,    62,    63,    64,    65,
      66,    67,   108,   169,   205,   171,     3,   205,     3,   198,
     108,   119,   127,   127,   125,   166,   169,   204,   205,     4,
     171,     3,   205,   188,   189,   119,     3,    16,    17,    18,
      19,    41,    54,    55,    56,    57,     3,   127,     4,     4,
     171,   204,     4,    81,     4,   205,   128,   206,   198,   190,
     198,   199,    90,    94,   110,   202,   203,   206,   206,   206,
     206,   190,   128,     3,     4,     3,     4,   198,   205,     4,
      10,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    42,   129,   203,   206,    98,   108,   205,     4,
     128,   171,   108,   153,   154,   198,   209,   153,   153,   153,
     153,   135,   153,   153,   138,   153,   142,   153,   153,   153,
     153,   153,   148,   153,   191,   192,   209,   153,   153,   153,
     192,   111,    90,   203,   198,     4,     4,   198,   206,   108,
     130,   131,   132,   134,   151,   206,   136,   137,   151,   139,
     151,   143,   144,   145,   146,   147,   151,   149,   108,   206,
     108,   133,   140,   141,   206,   203,   205,   108,   152,   151,
     151,   151,   151,   205,   151,   151,   151,   151,   151,   151,
     151,   151,   151,   108,   151,   151,   151,   205,   205,   154,
     108,   156,   205,   206,   205,   205,   157,   205,   205,   108,
       3,   155,   205,   202,   205,   205,   206,   174,   205,   205,
     108,   155,   160,   108,     4,   108,   155,   158,   205,   161,
     204,   205,   206,   159,   204,   205,   204,   205,   202,   155,
     205,   108,   202,   155,   205,   155,   205,   155,   206,   155,
     155,   108,   150,   202,   204
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
      yyerror (YY_("syntax error: cannot back up")); \
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

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr,					\
                  Type, Value);	\
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
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);


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
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

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



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
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
    ;
#endif
#endif
{
  
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
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short int *yyss1 = yyss;


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
	short int *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
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
        case 18:
#line 200 "rcparse.y"
    {
	    define_accelerator ((yyvsp[-5].id), &(yyvsp[-3].res_info), (yyvsp[-1].pacc));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
    break;

  case 19:
#line 210 "rcparse.y"
    {
	    (yyval.pacc) = NULL;
	  }
    break;

  case 20:
#line 214 "rcparse.y"
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
    break;

  case 21:
#line 235 "rcparse.y"
    {
	    (yyval.acc) = (yyvsp[-1].acc);
	    (yyval.acc).id = (yyvsp[0].il);
	  }
    break;

  case 22:
#line 240 "rcparse.y"
    {
	    (yyval.acc) = (yyvsp[-3].acc);
	    (yyval.acc).id = (yyvsp[-2].il);
	    (yyval.acc).flags |= (yyvsp[0].is);
	    if (((yyval.acc).flags & ACC_VIRTKEY) == 0
		&& ((yyval.acc).flags & (ACC_SHIFT | ACC_CONTROL)) != 0)
	      rcparse_warning (_("inappropriate modifiers for non-VIRTKEY"));
	  }
    break;

  case 23:
#line 252 "rcparse.y"
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
    break;

  case 24:
#line 272 "rcparse.y"
    {
	    (yyval.acc).next = NULL;
	    (yyval.acc).flags = 0;
	    (yyval.acc).id = 0;
	    (yyval.acc).key = (yyvsp[0].il);
	  }
    break;

  case 25:
#line 282 "rcparse.y"
    {
	    (yyval.is) = (yyvsp[0].is);
	  }
    break;

  case 26:
#line 286 "rcparse.y"
    {
	    (yyval.is) = (yyvsp[-2].is) | (yyvsp[0].is);
	  }
    break;

  case 27:
#line 291 "rcparse.y"
    {
	    (yyval.is) = (yyvsp[-1].is) | (yyvsp[0].is);
	  }
    break;

  case 28:
#line 298 "rcparse.y"
    {
	    (yyval.is) = ACC_VIRTKEY;
	  }
    break;

  case 29:
#line 302 "rcparse.y"
    {
	    /* This is just the absence of VIRTKEY.  */
	    (yyval.is) = 0;
	  }
    break;

  case 30:
#line 307 "rcparse.y"
    {
	    (yyval.is) = ACC_NOINVERT;
	  }
    break;

  case 31:
#line 311 "rcparse.y"
    {
	    (yyval.is) = ACC_SHIFT;
	  }
    break;

  case 32:
#line 315 "rcparse.y"
    {
	    (yyval.is) = ACC_CONTROL;
	  }
    break;

  case 33:
#line 319 "rcparse.y"
    {
	    (yyval.is) = ACC_ALT;
	  }
    break;

  case 34:
#line 328 "rcparse.y"
    {
	    define_bitmap ((yyvsp[-3].id), &(yyvsp[-1].res_info), (yyvsp[0].s));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
    break;

  case 35:
#line 340 "rcparse.y"
    {
	    define_cursor ((yyvsp[-3].id), &(yyvsp[-1].res_info), (yyvsp[0].s));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
    break;

  case 36:
#line 353 "rcparse.y"
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
    break;

  case 37:
#line 370 "rcparse.y"
    {
	    define_dialog ((yyvsp[-12].id), &sub_res_info, &dialog);
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
    break;

  case 38:
#line 378 "rcparse.y"
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
    break;

  case 39:
#line 397 "rcparse.y"
    {
	    define_dialog ((yyvsp[-12].id), &sub_res_info, &dialog);
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
    break;

  case 40:
#line 405 "rcparse.y"
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
    break;

  case 41:
#line 425 "rcparse.y"
    {
	    define_dialog ((yyvsp[-13].id), &sub_res_info, &dialog);
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
    break;

  case 42:
#line 435 "rcparse.y"
    {
	    (yyval.il) = 0;
	  }
    break;

  case 43:
#line 439 "rcparse.y"
    {
	    (yyval.il) = (yyvsp[0].il);
	  }
    break;

  case 45:
#line 447 "rcparse.y"
    {
	    dialog.style |= WS_CAPTION;
	    style |= WS_CAPTION;
	    dialog.caption = (yyvsp[0].uni);
	  }
    break;

  case 46:
#line 453 "rcparse.y"
    {
	    dialog.class = (yyvsp[0].id);
	  }
    break;

  case 47:
#line 458 "rcparse.y"
    {
	    dialog.style = style;
	  }
    break;

  case 48:
#line 462 "rcparse.y"
    {
	    dialog.exstyle = (yyvsp[0].il);
	  }
    break;

  case 49:
#line 466 "rcparse.y"
    {
	    res_unistring_to_id (& dialog.class, (yyvsp[0].uni));
	  }
    break;

  case 50:
#line 470 "rcparse.y"
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
    break;

  case 51:
#line 483 "rcparse.y"
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
    break;

  case 52:
#line 498 "rcparse.y"
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
    break;

  case 53:
#line 513 "rcparse.y"
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
    break;

  case 54:
#line 528 "rcparse.y"
    {
	    dialog.menu = (yyvsp[0].id);
	  }
    break;

  case 55:
#line 532 "rcparse.y"
    {
	    sub_res_info.characteristics = (yyvsp[0].il);
	  }
    break;

  case 56:
#line 536 "rcparse.y"
    {
	    sub_res_info.language = (yyvsp[-1].il) | ((yyvsp[0].il) << SUBLANG_SHIFT);
	  }
    break;

  case 57:
#line 540 "rcparse.y"
    {
	    sub_res_info.version = (yyvsp[0].il);
	  }
    break;

  case 59:
#line 548 "rcparse.y"
    {
	    rc_dialog_control **pp;

	    for (pp = &dialog.controls; *pp != NULL; pp = &(*pp)->next)
	      ;
	    *pp = (yyvsp[0].dialog_control);
	  }
    break;

  case 60:
#line 559 "rcparse.y"
    {
	      default_style = BS_AUTO3STATE | WS_TABSTOP;
	      base_style = BS_AUTO3STATE;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);	
	    }
    break;

  case 61:
#line 567 "rcparse.y"
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
    break;

  case 62:
#line 571 "rcparse.y"
    {
	      default_style = BS_AUTOCHECKBOX | WS_TABSTOP;
	      base_style = BS_AUTOCHECKBOX;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);	
	    }
    break;

  case 63:
#line 579 "rcparse.y"
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
    break;

  case 64:
#line 583 "rcparse.y"
    {
	      default_style = BS_AUTORADIOBUTTON | WS_TABSTOP;
	      base_style = BS_AUTORADIOBUTTON;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);	
	    }
    break;

  case 65:
#line 591 "rcparse.y"
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
    break;

  case 66:
#line 595 "rcparse.y"
    {
	      default_style = ES_LEFT | WS_BORDER | WS_TABSTOP;
	      base_style = ES_LEFT | WS_BORDER | WS_TABSTOP;
	      class.named = 0;
	      class.u.id = CTL_EDIT;
	      res_text_field = (yyvsp[0].id);	
	    }
    break;

  case 67:
#line 603 "rcparse.y"
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	    if (dialog.ex == NULL)
	      rcparse_warning (_("BEDIT requires DIALOGEX"));
	    res_string_to_id (&(yyval.dialog_control)->class, "BEDIT");
	  }
    break;

  case 68:
#line 610 "rcparse.y"
    {
	      default_style = BS_CHECKBOX | WS_TABSTOP;
	      base_style = BS_CHECKBOX | WS_TABSTOP;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);	
	    }
    break;

  case 69:
#line 618 "rcparse.y"
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
    break;

  case 70:
#line 622 "rcparse.y"
    {
	      /* This is as per MSDN documentation.  With some (???)
		 versions of MS rc.exe their is no default style.  */
	      default_style = CBS_SIMPLE | WS_TABSTOP;
	      base_style = 0;
	      class.named = 0;
	      class.u.id = CTL_COMBOBOX;
	      res_text_field = res_null_text;	
	    }
    break;

  case 71:
#line 632 "rcparse.y"
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
    break;

  case 72:
#line 637 "rcparse.y"
    {
	    (yyval.dialog_control) = define_control ((yyvsp[-9].id), (yyvsp[-8].il), (yyvsp[-5].il), (yyvsp[-4].il), (yyvsp[-3].il), (yyvsp[-2].il), (yyvsp[-7].id), style, (yyvsp[-1].il));
	    if ((yyvsp[0].rcdata_item) != NULL)
	      {
		if (dialog.ex == NULL)
		  rcparse_warning (_("control data requires DIALOGEX"));
		(yyval.dialog_control)->data = (yyvsp[0].rcdata_item);
	      }
	  }
    break;

  case 73:
#line 648 "rcparse.y"
    {
	    (yyval.dialog_control) = define_control ((yyvsp[-10].id), (yyvsp[-9].il), (yyvsp[-6].il), (yyvsp[-5].il), (yyvsp[-4].il), (yyvsp[-3].il), (yyvsp[-8].id), style, (yyvsp[-2].il));
	    if (dialog.ex == NULL)
	      rcparse_warning (_("help ID requires DIALOGEX"));
	    (yyval.dialog_control)->help = (yyvsp[-1].il);
	    (yyval.dialog_control)->data = (yyvsp[0].rcdata_item);
	  }
    break;

  case 74:
#line 656 "rcparse.y"
    {
	      default_style = SS_CENTER | WS_GROUP;
	      base_style = SS_CENTER;
	      class.named = 0;
	      class.u.id = CTL_STATIC;
	      res_text_field = (yyvsp[0].id);	
	    }
    break;

  case 75:
#line 664 "rcparse.y"
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
    break;

  case 76:
#line 668 "rcparse.y"
    {
	      default_style = BS_DEFPUSHBUTTON | WS_TABSTOP;
	      base_style = BS_DEFPUSHBUTTON | WS_TABSTOP;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);	
	    }
    break;

  case 77:
#line 676 "rcparse.y"
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
    break;

  case 78:
#line 680 "rcparse.y"
    {
	      default_style = ES_LEFT | WS_BORDER | WS_TABSTOP;
	      base_style = ES_LEFT | WS_BORDER | WS_TABSTOP;
	      class.named = 0;
	      class.u.id = CTL_EDIT;
	      res_text_field = res_null_text;	
	    }
    break;

  case 79:
#line 688 "rcparse.y"
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
    break;

  case 80:
#line 692 "rcparse.y"
    {
	      default_style = BS_GROUPBOX;
	      base_style = BS_GROUPBOX;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);	
	    }
    break;

  case 81:
#line 700 "rcparse.y"
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
    break;

  case 82:
#line 704 "rcparse.y"
    {
	      default_style = ES_LEFT | WS_BORDER | WS_TABSTOP;
	      base_style = ES_LEFT | WS_BORDER | WS_TABSTOP;
	      class.named = 0;
	      class.u.id = CTL_EDIT;
	      res_text_field = (yyvsp[0].id);	
	    }
    break;

  case 83:
#line 712 "rcparse.y"
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	    if (dialog.ex == NULL)
	      rcparse_warning (_("IEDIT requires DIALOGEX"));
	    res_string_to_id (&(yyval.dialog_control)->class, "HEDIT");
	  }
    break;

  case 84:
#line 719 "rcparse.y"
    {
	    (yyval.dialog_control) = define_icon_control ((yyvsp[-4].id), (yyvsp[-3].il), (yyvsp[-2].il), (yyvsp[-1].il), 0, 0, 0, (yyvsp[0].rcdata_item),
				      dialog.ex);
          }
    break;

  case 85:
#line 725 "rcparse.y"
    {
	    (yyval.dialog_control) = define_icon_control ((yyvsp[-6].id), (yyvsp[-5].il), (yyvsp[-4].il), (yyvsp[-3].il), 0, 0, 0, (yyvsp[0].rcdata_item),
				      dialog.ex);
          }
    break;

  case 86:
#line 731 "rcparse.y"
    {
	    (yyval.dialog_control) = define_icon_control ((yyvsp[-8].id), (yyvsp[-7].il), (yyvsp[-6].il), (yyvsp[-5].il), style, (yyvsp[-1].il), 0, (yyvsp[0].rcdata_item),
				      dialog.ex);
          }
    break;

  case 87:
#line 737 "rcparse.y"
    {
	    (yyval.dialog_control) = define_icon_control ((yyvsp[-9].id), (yyvsp[-8].il), (yyvsp[-7].il), (yyvsp[-6].il), style, (yyvsp[-2].il), (yyvsp[-1].il), (yyvsp[0].rcdata_item),
				      dialog.ex);
          }
    break;

  case 88:
#line 742 "rcparse.y"
    {
	      default_style = ES_LEFT | WS_BORDER | WS_TABSTOP;
	      base_style = ES_LEFT | WS_BORDER | WS_TABSTOP;
	      class.named = 0;
	      class.u.id = CTL_EDIT;
	      res_text_field = (yyvsp[0].id);	
	    }
    break;

  case 89:
#line 750 "rcparse.y"
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	    if (dialog.ex == NULL)
	      rcparse_warning (_("IEDIT requires DIALOGEX"));
	    res_string_to_id (&(yyval.dialog_control)->class, "IEDIT");
	  }
    break;

  case 90:
#line 757 "rcparse.y"
    {
	      default_style = LBS_NOTIFY | WS_BORDER;
	      base_style = LBS_NOTIFY | WS_BORDER;
	      class.named = 0;
	      class.u.id = CTL_LISTBOX;
	      res_text_field = res_null_text;	
	    }
    break;

  case 91:
#line 765 "rcparse.y"
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
    break;

  case 92:
#line 769 "rcparse.y"
    {
	      default_style = SS_LEFT | WS_GROUP;
	      base_style = SS_LEFT;
	      class.named = 0;
	      class.u.id = CTL_STATIC;
	      res_text_field = (yyvsp[0].id);	
	    }
    break;

  case 93:
#line 777 "rcparse.y"
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
    break;

  case 94:
#line 781 "rcparse.y"
    {
	      default_style = BS_PUSHBOX | WS_TABSTOP;
	      base_style = BS_PUSHBOX;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	    }
    break;

  case 95:
#line 788 "rcparse.y"
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
    break;

  case 96:
#line 792 "rcparse.y"
    {
	      default_style = BS_PUSHBUTTON | WS_TABSTOP;
	      base_style = BS_PUSHBUTTON | WS_TABSTOP;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);	
	    }
    break;

  case 97:
#line 800 "rcparse.y"
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
    break;

  case 98:
#line 804 "rcparse.y"
    {
	      default_style = BS_RADIOBUTTON | WS_TABSTOP;
	      base_style = BS_RADIOBUTTON;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);	
	    }
    break;

  case 99:
#line 812 "rcparse.y"
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
    break;

  case 100:
#line 816 "rcparse.y"
    {
	      default_style = SS_RIGHT | WS_GROUP;
	      base_style = SS_RIGHT;
	      class.named = 0;
	      class.u.id = CTL_STATIC;
	      res_text_field = (yyvsp[0].id);	
	    }
    break;

  case 101:
#line 824 "rcparse.y"
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
    break;

  case 102:
#line 828 "rcparse.y"
    {
	      default_style = SBS_HORZ;
	      base_style = 0;
	      class.named = 0;
	      class.u.id = CTL_SCROLLBAR;
	      res_text_field = res_null_text;	
	    }
    break;

  case 103:
#line 836 "rcparse.y"
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
    break;

  case 104:
#line 840 "rcparse.y"
    {
	      default_style = BS_3STATE | WS_TABSTOP;
	      base_style = BS_3STATE;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);	
	    }
    break;

  case 105:
#line 848 "rcparse.y"
    {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
    break;

  case 106:
#line 853 "rcparse.y"
    { style = WS_CHILD | WS_VISIBLE; }
    break;

  case 107:
#line 855 "rcparse.y"
    {
	    rc_res_id cid;
	    cid.named = 0;
	    cid.u.id = CTL_BUTTON;
	    (yyval.dialog_control) = define_control ((yyvsp[-13].id), (yyvsp[-12].il), (yyvsp[-10].il), (yyvsp[-8].il), (yyvsp[-6].il), (yyvsp[-4].il), cid,
				 style, (yyvsp[0].il));
	  }
    break;

  case 108:
#line 873 "rcparse.y"
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
    break;

  case 109:
#line 885 "rcparse.y"
    {
	    (yyval.dialog_control) = define_control (res_text_field, (yyvsp[-7].il), (yyvsp[-6].il), (yyvsp[-5].il), (yyvsp[-4].il), (yyvsp[-3].il), class, style, (yyvsp[-1].il));
	    if ((yyvsp[0].rcdata_item) != NULL)
	      {
		if (dialog.ex == NULL)
		  rcparse_warning (_("control data requires DIALOGEX"));
		(yyval.dialog_control)->data = (yyvsp[0].rcdata_item);
	      }
	  }
    break;

  case 110:
#line 896 "rcparse.y"
    {
	    (yyval.dialog_control) = define_control (res_text_field, (yyvsp[-8].il), (yyvsp[-7].il), (yyvsp[-6].il), (yyvsp[-5].il), (yyvsp[-4].il), class, style, (yyvsp[-2].il));
	    if (dialog.ex == NULL)
	      rcparse_warning (_("help ID requires DIALOGEX"));
	    (yyval.dialog_control)->help = (yyvsp[-1].il);
	    (yyval.dialog_control)->data = (yyvsp[0].rcdata_item);
	  }
    break;

  case 111:
#line 907 "rcparse.y"
    {
	    if ((yyvsp[0].id).named)
	      res_unistring_to_id (&(yyval.id), (yyvsp[0].id).u.n.name);
	    else
	      (yyval.id)=(yyvsp[0].id);
	  }
    break;

  case 112:
#line 917 "rcparse.y"
    {
	    res_string_to_id (&(yyval.id), "");
	  }
    break;

  case 113:
#line 920 "rcparse.y"
    { (yyval.id)=(yyvsp[-1].id); }
    break;

  case 114:
#line 925 "rcparse.y"
    {
	    (yyval.id).named = 0;
	    (yyval.id).u.id = (yyvsp[0].il);
	  }
    break;

  case 115:
#line 930 "rcparse.y"
    {
	    (yyval.id).named = 1;
	    (yyval.id).u.n.name = (yyvsp[0].uni);
	    (yyval.id).u.n.length = unichar_len ((yyvsp[0].uni));
	  }
    break;

  case 116:
#line 939 "rcparse.y"
    {
	    (yyval.rcdata_item) = NULL;
	  }
    break;

  case 117:
#line 943 "rcparse.y"
    {
	    (yyval.rcdata_item) = (yyvsp[-1].rcdata).first;
	  }
    break;

  case 118:
#line 952 "rcparse.y"
    { style = WS_CHILD | WS_VISIBLE; }
    break;

  case 120:
#line 958 "rcparse.y"
    { style = SS_ICON | WS_CHILD | WS_VISIBLE; }
    break;

  case 122:
#line 964 "rcparse.y"
    { style = base_style | WS_CHILD | WS_VISIBLE; }
    break;

  case 124:
#line 972 "rcparse.y"
    {
	    define_font ((yyvsp[-3].id), &(yyvsp[-1].res_info), (yyvsp[0].s));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
    break;

  case 125:
#line 984 "rcparse.y"
    {
	    define_icon ((yyvsp[-3].id), &(yyvsp[-1].res_info), (yyvsp[0].s));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
    break;

  case 126:
#line 997 "rcparse.y"
    {
	    language = (yyvsp[-1].il) | ((yyvsp[0].il) << SUBLANG_SHIFT);
	  }
    break;

  case 127:
#line 1006 "rcparse.y"
    {
	    define_menu ((yyvsp[-5].id), &(yyvsp[-3].res_info), (yyvsp[-1].menuitem));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
    break;

  case 128:
#line 1016 "rcparse.y"
    {
	    (yyval.menuitem) = NULL;
	  }
    break;

  case 129:
#line 1020 "rcparse.y"
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
    break;

  case 130:
#line 1037 "rcparse.y"
    {
	    (yyval.menuitem) = define_menuitem ((yyvsp[-2].uni), (yyvsp[-1].il), (yyvsp[0].is), 0, 0, NULL);
	  }
    break;

  case 131:
#line 1041 "rcparse.y"
    {
	    (yyval.menuitem) = define_menuitem (NULL, 0, 0, 0, 0, NULL);
	  }
    break;

  case 132:
#line 1045 "rcparse.y"
    {
	    (yyval.menuitem) = define_menuitem ((yyvsp[-4].uni), 0, (yyvsp[-3].is), 0, 0, (yyvsp[-1].menuitem));
	  }
    break;

  case 133:
#line 1052 "rcparse.y"
    {
	    (yyval.is) = 0;
	  }
    break;

  case 134:
#line 1056 "rcparse.y"
    {
	    (yyval.is) = (yyvsp[-2].is) | (yyvsp[0].is);
	  }
    break;

  case 135:
#line 1060 "rcparse.y"
    {
	    (yyval.is) = (yyvsp[-1].is) | (yyvsp[0].is);
	  }
    break;

  case 136:
#line 1067 "rcparse.y"
    {
	    (yyval.is) = MENUITEM_CHECKED;
	  }
    break;

  case 137:
#line 1071 "rcparse.y"
    {
	    (yyval.is) = MENUITEM_GRAYED;
	  }
    break;

  case 138:
#line 1075 "rcparse.y"
    {
	    (yyval.is) = MENUITEM_HELP;
	  }
    break;

  case 139:
#line 1079 "rcparse.y"
    {
	    (yyval.is) = MENUITEM_INACTIVE;
	  }
    break;

  case 140:
#line 1083 "rcparse.y"
    {
	    (yyval.is) = MENUITEM_MENUBARBREAK;
	  }
    break;

  case 141:
#line 1087 "rcparse.y"
    {
	    (yyval.is) = MENUITEM_MENUBREAK;
	  }
    break;

  case 142:
#line 1096 "rcparse.y"
    {
	    define_menu ((yyvsp[-5].id), &(yyvsp[-3].res_info), (yyvsp[-1].menuitem));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
    break;

  case 143:
#line 1106 "rcparse.y"
    {
	    (yyval.menuitem) = NULL;
	  }
    break;

  case 144:
#line 1110 "rcparse.y"
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
    break;

  case 145:
#line 1127 "rcparse.y"
    {
	    (yyval.menuitem) = define_menuitem ((yyvsp[0].uni), 0, 0, 0, 0, NULL);
	  }
    break;

  case 146:
#line 1131 "rcparse.y"
    {
	    (yyval.menuitem) = define_menuitem ((yyvsp[-1].uni), (yyvsp[0].il), 0, 0, 0, NULL);
	  }
    break;

  case 147:
#line 1135 "rcparse.y"
    {
	    (yyval.menuitem) = define_menuitem ((yyvsp[-3].uni), (yyvsp[-2].il), (yyvsp[-1].il), (yyvsp[0].il), 0, NULL);
	  }
    break;

  case 148:
#line 1139 "rcparse.y"
    {
 	    (yyval.menuitem) = define_menuitem (NULL, 0, 0, 0, 0, NULL);
 	  }
    break;

  case 149:
#line 1143 "rcparse.y"
    {
	    (yyval.menuitem) = define_menuitem ((yyvsp[-3].uni), 0, 0, 0, 0, (yyvsp[-1].menuitem));
	  }
    break;

  case 150:
#line 1147 "rcparse.y"
    {
	    (yyval.menuitem) = define_menuitem ((yyvsp[-4].uni), (yyvsp[-3].il), 0, 0, 0, (yyvsp[-1].menuitem));
	  }
    break;

  case 151:
#line 1151 "rcparse.y"
    {
	    (yyval.menuitem) = define_menuitem ((yyvsp[-5].uni), (yyvsp[-4].il), (yyvsp[-3].il), 0, 0, (yyvsp[-1].menuitem));
	  }
    break;

  case 152:
#line 1156 "rcparse.y"
    {
	    (yyval.menuitem) = define_menuitem ((yyvsp[-7].uni), (yyvsp[-6].il), (yyvsp[-5].il), (yyvsp[-4].il), (yyvsp[-3].il), (yyvsp[-1].menuitem));
	  }
    break;

  case 153:
#line 1165 "rcparse.y"
    {
	    define_messagetable ((yyvsp[-3].id), &(yyvsp[-1].res_info), (yyvsp[0].s));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
    break;

  case 154:
#line 1177 "rcparse.y"
    {
	    rcparse_rcdata ();
	  }
    break;

  case 155:
#line 1181 "rcparse.y"
    {
	    rcparse_normal ();
	    (yyval.rcdata) = (yyvsp[0].rcdata);
	  }
    break;

  case 156:
#line 1189 "rcparse.y"
    {
	    (yyval.rcdata).first = NULL;
	    (yyval.rcdata).last = NULL;
	  }
    break;

  case 157:
#line 1194 "rcparse.y"
    {
	    (yyval.rcdata) = (yyvsp[0].rcdata);
	  }
    break;

  case 158:
#line 1201 "rcparse.y"
    {
	    rc_rcdata_item *ri;

	    ri = define_rcdata_string ((yyvsp[0].ss).s, (yyvsp[0].ss).length);
	    (yyval.rcdata).first = ri;
	    (yyval.rcdata).last = ri;
	  }
    break;

  case 159:
#line 1209 "rcparse.y"
    {
	    rc_rcdata_item *ri;

	    ri = define_rcdata_unistring ((yyvsp[0].suni).s, (yyvsp[0].suni).length);
	    (yyval.rcdata).first = ri;
	    (yyval.rcdata).last = ri;
	  }
    break;

  case 160:
#line 1217 "rcparse.y"
    {
	    rc_rcdata_item *ri;

	    ri = define_rcdata_number ((yyvsp[0].i).val, (yyvsp[0].i).dword);
	    (yyval.rcdata).first = ri;
	    (yyval.rcdata).last = ri;
	  }
    break;

  case 161:
#line 1225 "rcparse.y"
    {
	    rc_rcdata_item *ri;

	    ri = define_rcdata_string ((yyvsp[0].ss).s, (yyvsp[0].ss).length);
	    (yyval.rcdata).first = (yyvsp[-2].rcdata).first;
	    (yyvsp[-2].rcdata).last->next = ri;
	    (yyval.rcdata).last = ri;
	  }
    break;

  case 162:
#line 1234 "rcparse.y"
    {
	    rc_rcdata_item *ri;

	    ri = define_rcdata_unistring ((yyvsp[0].suni).s, (yyvsp[0].suni).length);
	    (yyval.rcdata).first = (yyvsp[-2].rcdata).first;
	    (yyvsp[-2].rcdata).last->next = ri;
	    (yyval.rcdata).last = ri;
	  }
    break;

  case 163:
#line 1243 "rcparse.y"
    {
	    rc_rcdata_item *ri;

	    ri = define_rcdata_number ((yyvsp[0].i).val, (yyvsp[0].i).dword);
	    (yyval.rcdata).first = (yyvsp[-2].rcdata).first;
	    (yyvsp[-2].rcdata).last->next = ri;
	    (yyval.rcdata).last = ri;
	  }
    break;

  case 164:
#line 1257 "rcparse.y"
    { sub_res_info = (yyvsp[-1].res_info); }
    break;

  case 167:
#line 1264 "rcparse.y"
    {
	    define_stringtable (&sub_res_info, (yyvsp[-1].il), (yyvsp[0].uni));
	    rcparse_discard_strings ();
	  }
    break;

  case 168:
#line 1269 "rcparse.y"
    {
	    define_stringtable (&sub_res_info, (yyvsp[-2].il), (yyvsp[0].uni));
	    rcparse_discard_strings ();
	  }
    break;

  case 169:
#line 1274 "rcparse.y"
    {
	    rcparse_warning (_("invalid stringtable resource."));
	    abort ();
	  }
    break;

  case 170:
#line 1282 "rcparse.y"
    {
	    (yyval.id)=(yyvsp[0].id);
	  }
    break;

  case 171:
#line 1286 "rcparse.y"
    {
	  (yyval.id).named = 0;
	  (yyval.id).u.id = 23;
	}
    break;

  case 172:
#line 1291 "rcparse.y"
    {
          (yyval.id).named = 0;
          (yyval.id).u.id = RT_RCDATA;
        }
    break;

  case 173:
#line 1296 "rcparse.y"
    {
          (yyval.id).named = 0;
          (yyval.id).u.id = RT_MANIFEST;
        }
    break;

  case 174:
#line 1301 "rcparse.y"
    {
          (yyval.id).named = 0;
          (yyval.id).u.id = RT_PLUGPLAY;
        }
    break;

  case 175:
#line 1306 "rcparse.y"
    {
          (yyval.id).named = 0;
          (yyval.id).u.id = RT_VXD;
        }
    break;

  case 176:
#line 1311 "rcparse.y"
    {
          (yyval.id).named = 0;
          (yyval.id).u.id = RT_DLGINCLUDE;
        }
    break;

  case 177:
#line 1316 "rcparse.y"
    {
          (yyval.id).named = 0;
          (yyval.id).u.id = RT_DLGINIT;
        }
    break;

  case 178:
#line 1321 "rcparse.y"
    {
          (yyval.id).named = 0;
          (yyval.id).u.id = RT_ANICURSOR;
        }
    break;

  case 179:
#line 1326 "rcparse.y"
    {
          (yyval.id).named = 0;
          (yyval.id).u.id = RT_ANIICON;
        }
    break;

  case 180:
#line 1337 "rcparse.y"
    {
	    define_user_data ((yyvsp[-5].id), (yyvsp[-4].id), &(yyvsp[-3].res_info), (yyvsp[-1].rcdata).first);
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
    break;

  case 181:
#line 1344 "rcparse.y"
    {
	    define_user_file ((yyvsp[-3].id), (yyvsp[-2].id), &(yyvsp[-1].res_info), (yyvsp[0].s));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
    break;

  case 182:
#line 1354 "rcparse.y"
    {
	  define_toolbar ((yyvsp[-7].id), &(yyvsp[-5].res_info), (yyvsp[-4].il), (yyvsp[-3].il), (yyvsp[-1].toobar_item));
	}
    break;

  case 183:
#line 1359 "rcparse.y"
    { (yyval.toobar_item)= NULL; }
    break;

  case 184:
#line 1361 "rcparse.y"
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
    break;

  case 185:
#line 1380 "rcparse.y"
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
    break;

  case 186:
#line 1405 "rcparse.y"
    {
	    define_versioninfo ((yyvsp[-5].id), language, (yyvsp[-3].fixver), (yyvsp[-1].verinfo));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
    break;

  case 187:
#line 1415 "rcparse.y"
    {
	    (yyval.fixver) = ((rc_fixed_versioninfo *)
		  res_alloc (sizeof (rc_fixed_versioninfo)));
	    memset ((yyval.fixver), 0, sizeof (rc_fixed_versioninfo));
	  }
    break;

  case 188:
#line 1421 "rcparse.y"
    {
	    (yyvsp[-5].fixver)->file_version_ms = ((yyvsp[-3].il) << 16) | (yyvsp[-2].il);
	    (yyvsp[-5].fixver)->file_version_ls = ((yyvsp[-1].il) << 16) | (yyvsp[0].il);
	    (yyval.fixver) = (yyvsp[-5].fixver);
	  }
    break;

  case 189:
#line 1427 "rcparse.y"
    {
	    (yyvsp[-5].fixver)->product_version_ms = ((yyvsp[-3].il) << 16) | (yyvsp[-2].il);
	    (yyvsp[-5].fixver)->product_version_ls = ((yyvsp[-1].il) << 16) | (yyvsp[0].il);
	    (yyval.fixver) = (yyvsp[-5].fixver);
	  }
    break;

  case 190:
#line 1433 "rcparse.y"
    {
	    (yyvsp[-2].fixver)->file_flags_mask = (yyvsp[0].il);
	    (yyval.fixver) = (yyvsp[-2].fixver);
	  }
    break;

  case 191:
#line 1438 "rcparse.y"
    {
	    (yyvsp[-2].fixver)->file_flags = (yyvsp[0].il);
	    (yyval.fixver) = (yyvsp[-2].fixver);
	  }
    break;

  case 192:
#line 1443 "rcparse.y"
    {
	    (yyvsp[-2].fixver)->file_os = (yyvsp[0].il);
	    (yyval.fixver) = (yyvsp[-2].fixver);
	  }
    break;

  case 193:
#line 1448 "rcparse.y"
    {
	    (yyvsp[-2].fixver)->file_type = (yyvsp[0].il);
	    (yyval.fixver) = (yyvsp[-2].fixver);
	  }
    break;

  case 194:
#line 1453 "rcparse.y"
    {
	    (yyvsp[-2].fixver)->file_subtype = (yyvsp[0].il);
	    (yyval.fixver) = (yyvsp[-2].fixver);
	  }
    break;

  case 195:
#line 1467 "rcparse.y"
    {
	    (yyval.verinfo) = NULL;
	  }
    break;

  case 196:
#line 1471 "rcparse.y"
    {
	    (yyval.verinfo) = append_ver_stringfileinfo ((yyvsp[-7].verinfo), (yyvsp[-4].s), (yyvsp[-2].verstring));
	  }
    break;

  case 197:
#line 1475 "rcparse.y"
    {
	    (yyval.verinfo) = append_ver_varfileinfo ((yyvsp[-6].verinfo), (yyvsp[-2].uni), (yyvsp[-1].vervar));
	  }
    break;

  case 198:
#line 1482 "rcparse.y"
    {
	    (yyval.verstring) = NULL;
	  }
    break;

  case 199:
#line 1486 "rcparse.y"
    {
	    (yyval.verstring) = append_verval ((yyvsp[-4].verstring), (yyvsp[-2].uni), (yyvsp[0].uni));
	  }
    break;

  case 200:
#line 1493 "rcparse.y"
    {
	    (yyval.vervar) = NULL;
	  }
    break;

  case 201:
#line 1497 "rcparse.y"
    {
	    (yyval.vervar) = append_vertrans ((yyvsp[-2].vervar), (yyvsp[-1].il), (yyvsp[0].il));
	  }
    break;

  case 202:
#line 1506 "rcparse.y"
    {
	    (yyval.id).named = 0;
	    (yyval.id).u.id = (yyvsp[0].il);
	  }
    break;

  case 203:
#line 1511 "rcparse.y"
    {
	    res_unistring_to_id (&(yyval.id), (yyvsp[0].uni));
	  }
    break;

  case 204:
#line 1520 "rcparse.y"
    {
	    (yyval.uni) = (yyvsp[0].uni);
	  }
    break;

  case 205:
#line 1524 "rcparse.y"
    {
	    unichar *h = NULL;
	    unicode_from_ascii ((rc_uint_type *) NULL, &h, (yyvsp[0].s));
	    (yyval.uni) = h;
	  }
    break;

  case 206:
#line 1534 "rcparse.y"
    {
	    (yyval.id).named = 0;
	    (yyval.id).u.id = (yyvsp[-1].il);
	  }
    break;

  case 207:
#line 1539 "rcparse.y"
    {
	    res_unistring_to_id (&(yyval.id), (yyvsp[0].uni));
	  }
    break;

  case 208:
#line 1543 "rcparse.y"
    {
	    res_unistring_to_id (&(yyval.id), (yyvsp[-1].uni));
	  }
    break;

  case 209:
#line 1553 "rcparse.y"
    {
	    memset (&(yyval.res_info), 0, sizeof (rc_res_res_info));
	    (yyval.res_info).language = language;
	    /* FIXME: Is this the right default?  */
	    (yyval.res_info).memflags = MEMFLAG_MOVEABLE | MEMFLAG_PURE | MEMFLAG_DISCARDABLE;
	  }
    break;

  case 210:
#line 1560 "rcparse.y"
    {
	    (yyval.res_info) = (yyvsp[-1].res_info);
	    (yyval.res_info).memflags |= (yyvsp[0].memflags).on;
	    (yyval.res_info).memflags &=~ (yyvsp[0].memflags).off;
	  }
    break;

  case 211:
#line 1566 "rcparse.y"
    {
	    (yyval.res_info) = (yyvsp[-2].res_info);
	    (yyval.res_info).characteristics = (yyvsp[0].il);
	  }
    break;

  case 212:
#line 1571 "rcparse.y"
    {
	    (yyval.res_info) = (yyvsp[-3].res_info);
	    (yyval.res_info).language = (yyvsp[-1].il) | ((yyvsp[0].il) << SUBLANG_SHIFT);
	  }
    break;

  case 213:
#line 1576 "rcparse.y"
    {
	    (yyval.res_info) = (yyvsp[-2].res_info);
	    (yyval.res_info).version = (yyvsp[0].il);
	  }
    break;

  case 214:
#line 1586 "rcparse.y"
    {
	    memset (&(yyval.res_info), 0, sizeof (rc_res_res_info));
	    (yyval.res_info).language = language;
	    (yyval.res_info).memflags = MEMFLAG_MOVEABLE | MEMFLAG_DISCARDABLE;
	  }
    break;

  case 215:
#line 1592 "rcparse.y"
    {
	    (yyval.res_info) = (yyvsp[-1].res_info);
	    (yyval.res_info).memflags |= (yyvsp[0].memflags).on;
	    (yyval.res_info).memflags &=~ (yyvsp[0].memflags).off;
	  }
    break;

  case 216:
#line 1603 "rcparse.y"
    {
	    memset (&(yyval.res_info), 0, sizeof (rc_res_res_info));
	    (yyval.res_info).language = language;
	    (yyval.res_info).memflags = MEMFLAG_MOVEABLE | MEMFLAG_PURE | MEMFLAG_DISCARDABLE;
	  }
    break;

  case 217:
#line 1609 "rcparse.y"
    {
	    (yyval.res_info) = (yyvsp[-1].res_info);
	    (yyval.res_info).memflags |= (yyvsp[0].memflags).on;
	    (yyval.res_info).memflags &=~ (yyvsp[0].memflags).off;
	  }
    break;

  case 218:
#line 1621 "rcparse.y"
    {
	    (yyval.memflags).on = MEMFLAG_MOVEABLE;
	    (yyval.memflags).off = 0;
	  }
    break;

  case 219:
#line 1626 "rcparse.y"
    {
	    (yyval.memflags).on = 0;
	    (yyval.memflags).off = MEMFLAG_MOVEABLE;
	  }
    break;

  case 220:
#line 1631 "rcparse.y"
    {
	    (yyval.memflags).on = MEMFLAG_PURE;
	    (yyval.memflags).off = 0;
	  }
    break;

  case 221:
#line 1636 "rcparse.y"
    {
	    (yyval.memflags).on = 0;
	    (yyval.memflags).off = MEMFLAG_PURE;
	  }
    break;

  case 222:
#line 1641 "rcparse.y"
    {
	    (yyval.memflags).on = MEMFLAG_PRELOAD;
	    (yyval.memflags).off = 0;
	  }
    break;

  case 223:
#line 1646 "rcparse.y"
    {
	    (yyval.memflags).on = 0;
	    (yyval.memflags).off = MEMFLAG_PRELOAD;
	  }
    break;

  case 224:
#line 1651 "rcparse.y"
    {
	    (yyval.memflags).on = MEMFLAG_DISCARDABLE;
	    (yyval.memflags).off = 0;
	  }
    break;

  case 225:
#line 1661 "rcparse.y"
    {
	    (yyval.s) = (yyvsp[0].s);
	  }
    break;

  case 226:
#line 1665 "rcparse.y"
    {
	    (yyval.s) = (yyvsp[0].s);
	  }
    break;

  case 227:
#line 1673 "rcparse.y"
    {
	    (yyval.uni) = (yyvsp[0].uni);
	  }
    break;

  case 228:
#line 1678 "rcparse.y"
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
    break;

  case 229:
#line 1693 "rcparse.y"
    {
	    (yyval.uni) = unichar_dup ((yyvsp[0].uni));
	  }
    break;

  case 230:
#line 1697 "rcparse.y"
    {
	    unichar *h = NULL;
	    unicode_from_ascii ((rc_uint_type *) NULL, &h, (yyvsp[0].s));
	    (yyval.uni) = h;
	  }
    break;

  case 231:
#line 1706 "rcparse.y"
    {
	    (yyval.ss) = (yyvsp[0].ss);
	  }
    break;

  case 232:
#line 1710 "rcparse.y"
    {
	    rc_uint_type l = (yyvsp[-1].ss).length + (yyvsp[0].ss).length;
	    char *h = (char *) res_alloc (l);
	    memcpy (h, (yyvsp[-1].ss).s, (yyvsp[-1].ss).length);
	    memcpy (h + (yyvsp[-1].ss).length, (yyvsp[0].ss).s, (yyvsp[0].ss).length);
	    (yyval.ss).s = h;
	    (yyval.ss).length = l;
	  }
    break;

  case 233:
#line 1722 "rcparse.y"
    {
	    (yyval.suni) = (yyvsp[0].suni);
	  }
    break;

  case 234:
#line 1726 "rcparse.y"
    {
	    rc_uint_type l = (yyvsp[-1].suni).length + (yyvsp[0].suni).length;
	    unichar *h = (unichar *) res_alloc (l * sizeof (unichar));
	    memcpy (h, (yyvsp[-1].suni).s, (yyvsp[-1].suni).length * sizeof (unichar));
	    memcpy (h + (yyvsp[-1].suni).length, (yyvsp[0].suni).s, (yyvsp[0].suni).length  * sizeof (unichar));
	    (yyval.suni).s = h;
	    (yyval.suni).length = l;
	  }
    break;

  case 235:
#line 1748 "rcparse.y"
    {
	    style |= (yyvsp[0].il);
	  }
    break;

  case 236:
#line 1752 "rcparse.y"
    {
	    style &=~ (yyvsp[0].il);
	  }
    break;

  case 237:
#line 1756 "rcparse.y"
    {
	    style |= (yyvsp[0].il);
	  }
    break;

  case 238:
#line 1760 "rcparse.y"
    {
	    style &=~ (yyvsp[0].il);
	  }
    break;

  case 239:
#line 1767 "rcparse.y"
    {
	    (yyval.il) = (yyvsp[0].i).val;
	  }
    break;

  case 240:
#line 1771 "rcparse.y"
    {
	    (yyval.il) = (yyvsp[-1].il);
	  }
    break;

  case 241:
#line 1780 "rcparse.y"
    {
	    (yyval.il) = 0;
	  }
    break;

  case 242:
#line 1784 "rcparse.y"
    {
	    (yyval.il) = (yyvsp[0].il);
	  }
    break;

  case 243:
#line 1793 "rcparse.y"
    {
	    (yyval.il) = (yyvsp[0].il);
	  }
    break;

  case 244:
#line 1802 "rcparse.y"
    {
	    (yyval.il) = (yyvsp[0].i).val;
	  }
    break;

  case 245:
#line 1811 "rcparse.y"
    {
	    (yyval.i) = (yyvsp[0].i);
	  }
    break;

  case 246:
#line 1815 "rcparse.y"
    {
	    (yyval.i) = (yyvsp[-1].i);
	  }
    break;

  case 247:
#line 1819 "rcparse.y"
    {
	    (yyval.i).val = ~ (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[0].i).dword;
	  }
    break;

  case 248:
#line 1824 "rcparse.y"
    {
	    (yyval.i).val = - (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[0].i).dword;
	  }
    break;

  case 249:
#line 1829 "rcparse.y"
    {
	    (yyval.i).val = (yyvsp[-2].i).val * (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
    break;

  case 250:
#line 1834 "rcparse.y"
    {
	    (yyval.i).val = (yyvsp[-2].i).val / (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
    break;

  case 251:
#line 1839 "rcparse.y"
    {
	    (yyval.i).val = (yyvsp[-2].i).val % (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
    break;

  case 252:
#line 1844 "rcparse.y"
    {
	    (yyval.i).val = (yyvsp[-2].i).val + (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
    break;

  case 253:
#line 1849 "rcparse.y"
    {
	    (yyval.i).val = (yyvsp[-2].i).val - (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
    break;

  case 254:
#line 1854 "rcparse.y"
    {
	    (yyval.i).val = (yyvsp[-2].i).val & (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
    break;

  case 255:
#line 1859 "rcparse.y"
    {
	    (yyval.i).val = (yyvsp[-2].i).val ^ (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
    break;

  case 256:
#line 1864 "rcparse.y"
    {
	    (yyval.i).val = (yyvsp[-2].i).val | (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
    break;

  case 257:
#line 1875 "rcparse.y"
    {
	    (yyval.il) = (yyvsp[0].il);
	  }
    break;

  case 258:
#line 1884 "rcparse.y"
    {
	    (yyval.il) = (yyvsp[0].i).val;
	  }
    break;

  case 259:
#line 1895 "rcparse.y"
    {
	    (yyval.i) = (yyvsp[0].i);
	  }
    break;

  case 260:
#line 1899 "rcparse.y"
    {
	    (yyval.i) = (yyvsp[-1].i);
	  }
    break;

  case 261:
#line 1903 "rcparse.y"
    {
	    (yyval.i).val = ~ (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[0].i).dword;
	  }
    break;

  case 262:
#line 1908 "rcparse.y"
    {
	    (yyval.i).val = (yyvsp[-2].i).val * (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
    break;

  case 263:
#line 1913 "rcparse.y"
    {
	    (yyval.i).val = (yyvsp[-2].i).val / (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
    break;

  case 264:
#line 1918 "rcparse.y"
    {
	    (yyval.i).val = (yyvsp[-2].i).val % (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
    break;

  case 265:
#line 1923 "rcparse.y"
    {
	    (yyval.i).val = (yyvsp[-2].i).val + (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
    break;

  case 266:
#line 1928 "rcparse.y"
    {
	    (yyval.i).val = (yyvsp[-2].i).val - (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
    break;

  case 267:
#line 1933 "rcparse.y"
    {
	    (yyval.i).val = (yyvsp[-2].i).val & (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
    break;

  case 268:
#line 1938 "rcparse.y"
    {
	    (yyval.i).val = (yyvsp[-2].i).val ^ (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
    break;

  case 269:
#line 1943 "rcparse.y"
    {
	    (yyval.i).val = (yyvsp[-2].i).val | (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */
#line 4145 "rcparse.c"

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
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    {
	      yyerror (YY_("syntax error"));
	      goto yyexhaustedlab;
	    }
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror (YY_("syntax error"));
    }



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
	  yydestruct ("Error: discarding", yytoken, &yylval);
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


      yydestruct ("Error: popping", yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


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
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK;
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 1949 "rcparse.y"


/* Set the language from the command line.  */

void
rcparse_set_language (int lang)
{
  language = lang;
}

