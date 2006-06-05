/* A Bison parser, made by GNU Bison 1.875.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

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
     IDENTIFIER = 258,
     TYPENAME = 259,
     SCSPEC = 260,
     STATIC = 261,
     TYPESPEC = 262,
     TYPE_QUAL = 263,
     CONSTANT = 264,
     STRING = 265,
     ELLIPSIS = 266,
     SIZEOF = 267,
     ENUM = 268,
     STRUCT = 269,
     UNION = 270,
     IF = 271,
     ELSE = 272,
     WHILE = 273,
     DO = 274,
     FOR = 275,
     SWITCH = 276,
     CASE = 277,
     DEFAULT = 278,
     BREAK = 279,
     CONTINUE = 280,
     RETURN = 281,
     GOTO = 282,
     ASM_KEYWORD = 283,
     TYPEOF = 284,
     ALIGNOF = 285,
     ATTRIBUTE = 286,
     EXTENSION = 287,
     LABEL = 288,
     REALPART = 289,
     IMAGPART = 290,
     VA_ARG = 291,
     CHOOSE_EXPR = 292,
     TYPES_COMPATIBLE_P = 293,
     PTR_VALUE = 294,
     PTR_BASE = 295,
     PTR_EXTENT = 296,
     STRING_FUNC_NAME = 297,
     VAR_FUNC_NAME = 298,
     ASSIGN = 299,
     OROR = 300,
     ANDAND = 301,
     EQCOMPARE = 302,
     ARITHCOMPARE = 303,
     RSHIFT = 304,
     LSHIFT = 305,
     MINUSMINUS = 306,
     PLUSPLUS = 307,
     UNARY = 308,
     HYPERUNARY = 309,
     POINTSAT = 310,
     INTERFACE = 311,
     IMPLEMENTATION = 312,
     END = 313,
     SELECTOR = 314,
     DEFS = 315,
     ENCODE = 316,
     CLASSNAME = 317,
     PUBLIC = 318,
     PRIVATE = 319,
     PROTECTED = 320,
     PROTOCOL = 321,
     OBJECTNAME = 322,
     CLASS = 323,
     ALIAS = 324
   };
#endif
#define IDENTIFIER 258
#define TYPENAME 259
#define SCSPEC 260
#define STATIC 261
#define TYPESPEC 262
#define TYPE_QUAL 263
#define CONSTANT 264
#define STRING 265
#define ELLIPSIS 266
#define SIZEOF 267
#define ENUM 268
#define STRUCT 269
#define UNION 270
#define IF 271
#define ELSE 272
#define WHILE 273
#define DO 274
#define FOR 275
#define SWITCH 276
#define CASE 277
#define DEFAULT 278
#define BREAK 279
#define CONTINUE 280
#define RETURN 281
#define GOTO 282
#define ASM_KEYWORD 283
#define TYPEOF 284
#define ALIGNOF 285
#define ATTRIBUTE 286
#define EXTENSION 287
#define LABEL 288
#define REALPART 289
#define IMAGPART 290
#define VA_ARG 291
#define CHOOSE_EXPR 292
#define TYPES_COMPATIBLE_P 293
#define PTR_VALUE 294
#define PTR_BASE 295
#define PTR_EXTENT 296
#define STRING_FUNC_NAME 297
#define VAR_FUNC_NAME 298
#define ASSIGN 299
#define OROR 300
#define ANDAND 301
#define EQCOMPARE 302
#define ARITHCOMPARE 303
#define RSHIFT 304
#define LSHIFT 305
#define MINUSMINUS 306
#define PLUSPLUS 307
#define UNARY 308
#define HYPERUNARY 309
#define POINTSAT 310
#define INTERFACE 311
#define IMPLEMENTATION 312
#define END 313
#define SELECTOR 314
#define DEFS 315
#define ENCODE 316
#define CLASSNAME 317
#define PUBLIC 318
#define PRIVATE 319
#define PROTECTED 320
#define PROTOCOL 321
#define OBJECTNAME 322
#define CLASS 323
#define ALIAS 324




/* Copy the first part of user declarations.  */
#line 33 "objc-parse.y"

#include "config.h"
#include "system.h"
#include "tree.h"
#include "input.h"
#include "cpplib.h"
#include "intl.h"
#include "timevar.h"
#include "c-pragma.h"		/* For YYDEBUG definition, and parse_in.  */
#include "c-tree.h"
#include "flags.h"
#include "output.h"
#include "toplev.h"
#include "ggc.h"

#ifdef MULTIBYTE_CHARS
#include <locale.h>
#endif

#include "objc-act.h"

/* Like YYERROR but do call yyerror.  */
#define YYERROR1 { yyerror ("syntax error"); YYERROR; }

/* Like the default stack expander, except (1) use realloc when possible,
   (2) impose no hard maxiumum on stack size, (3) REALLY do not use alloca.

   Irritatingly, YYSTYPE is defined after this %{ %} block, so we cannot
   give malloced_yyvs its proper type.  This is ok since all we need from
   it is to be able to free it.  */

static short *malloced_yyss;
static void *malloced_yyvs;

#define yyoverflow(MSG, SS, SSSIZE, VS, VSSIZE, YYSSZ)			\
do {									\
  size_t newsize;							\
  short *newss;								\
  YYSTYPE *newvs;							\
  newsize = *(YYSSZ) *= 2;						\
  if (malloced_yyss)							\
    {									\
      newss = (short *)							\
	really_call_realloc (*(SS), newsize * sizeof (short));		\
      newvs = (YYSTYPE *)						\
	really_call_realloc (*(VS), newsize * sizeof (YYSTYPE));	\
    }									\
  else									\
    {									\
      newss = (short *) really_call_malloc (newsize * sizeof (short));	\
      newvs = (YYSTYPE *) really_call_malloc (newsize * sizeof (YYSTYPE)); \
      if (newss)							\
        memcpy (newss, *(SS), (SSSIZE));				\
      if (newvs)							\
        memcpy (newvs, *(VS), (VSSIZE));				\
    }									\
  if (!newss || !newvs)							\
    {									\
      yyerror (MSG);							\
      return 2;								\
    }									\
  *(SS) = newss;							\
  *(VS) = newvs;							\
  malloced_yyss = newss;						\
  malloced_yyvs = (void *) newvs;					\
} while (0)


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
#line 103 "objc-parse.y"
typedef union YYSTYPE {long itype; tree ttype; enum tree_code code;
	const char *filename; int lineno; } YYSTYPE;
/* Line 191 of yacc.c.  */
#line 284 "op19726.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */
#line 248 "objc-parse.y"

/* Number of statements (loosely speaking) and compound statements
   seen so far.  */
static int stmt_count;
static int compstmt_count;

/* Input file and line number of the end of the body of last simple_if;
   used by the stmt-rule immediately after simple_if returns.  */
static const char *if_stmt_file;
static int if_stmt_line;

/* List of types and structure classes of the current declaration.  */
static GTY(()) tree current_declspecs;
static GTY(()) tree prefix_attributes;

/* List of all the attributes applying to the identifier currently being
   declared; includes prefix_attributes and possibly some more attributes
   just after a comma.  */
static GTY(()) tree all_prefix_attributes;

/* Stack of saved values of current_declspecs, prefix_attributes and
   all_prefix_attributes.  */
static GTY(()) tree declspec_stack;

/* PUSH_DECLSPEC_STACK is called from setspecs; POP_DECLSPEC_STACK
   should be called from the productions making use of setspecs.  */
#define PUSH_DECLSPEC_STACK						 \
  do {									 \
    declspec_stack = tree_cons (build_tree_list (prefix_attributes,	 \
						 all_prefix_attributes), \
				current_declspecs,			 \
				declspec_stack);			 \
  } while (0)

#define POP_DECLSPEC_STACK						\
  do {									\
    current_declspecs = TREE_VALUE (declspec_stack);			\
    prefix_attributes = TREE_PURPOSE (TREE_PURPOSE (declspec_stack));	\
    all_prefix_attributes = TREE_VALUE (TREE_PURPOSE (declspec_stack));	\
    declspec_stack = TREE_CHAIN (declspec_stack);			\
  } while (0)

/* For __extension__, save/restore the warning flags which are
   controlled by __extension__.  */
#define SAVE_EXT_FLAGS()			\
	size_int (pedantic			\
		  | (warn_pointer_arith << 1)	\
		  | (warn_traditional << 2)	\
		  | (flag_iso << 3))

#define RESTORE_EXT_FLAGS(tval)			\
  do {						\
    int val = tree_low_cst (tval, 0);		\
    pedantic = val & 1;				\
    warn_pointer_arith = (val >> 1) & 1;	\
    warn_traditional = (val >> 2) & 1;		\
    flag_iso = (val >> 3) & 1;			\
  } while (0)

/* Objective-C specific parser/lexer information */

static enum tree_code objc_inherit_code;
static int objc_pq_context = 0, objc_public_flag = 0;

/* The following flag is needed to contextualize ObjC lexical analysis.
   In some cases (e.g., 'int NSObject;'), it is undesirable to bind
   an identifier to an ObjC class, even if a class with that name
   exists.  */
static int objc_need_raw_identifier;
#define OBJC_NEED_RAW_IDENTIFIER(VAL)	objc_need_raw_identifier = VAL


static bool parsing_iso_function_signature;

/* Tell yyparse how to print a token's value, if yydebug is set.  */

#define YYPRINT(FILE,YYCHAR,YYLVAL) yyprint(FILE,YYCHAR,YYLVAL)

static void yyprint	  PARAMS ((FILE *, int, YYSTYPE));
static void yyerror	  PARAMS ((const char *));
static int yylexname	  PARAMS ((void));
static int yylexstring	  PARAMS ((void));
static inline int _yylex  PARAMS ((void));
static int  yylex	  PARAMS ((void));
static void init_reswords PARAMS ((void));

  /* Initialisation routine for this file.  */
void
c_parse_init ()
{
  init_reswords ();
}



/* Line 214 of yacc.c.  */
#line 390 "op19726.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

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
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYSTYPE_IS_TRIVIAL)))

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
#define YYFINAL  4
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   5111

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  93
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  266
/* YYNRULES -- Number of rules. */
#define YYNRULES  721
/* YYNRULES -- Number of states. */
#define YYNSTATES  1165

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   324

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    88,     2,     2,     2,    61,    52,     2,
      67,    84,    59,    57,    89,    58,    66,    60,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    47,    85,
       2,    44,     2,    46,    92,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    68,     2,    91,    51,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    90,    50,    86,    87,     2,     2,     2,
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
      35,    36,    37,    38,    39,    40,    41,    42,    43,    45,
      48,    49,    53,    54,    55,    56,    62,    63,    64,    65,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short yyprhs[] =
{
       0,     0,     3,     4,     6,     7,    10,    11,    15,    17,
      19,    21,    23,    29,    32,    36,    41,    46,    49,    52,
      55,    57,    58,    59,    69,    74,    75,    76,    86,    91,
      92,    93,   102,   106,   108,   110,   112,   114,   116,   118,
     120,   122,   124,   126,   128,   130,   131,   133,   135,   139,
     141,   144,   147,   150,   153,   156,   161,   164,   169,   172,
     175,   177,   179,   181,   183,   188,   190,   194,   198,   202,
     206,   210,   214,   218,   222,   226,   230,   234,   238,   239,
     244,   245,   250,   251,   252,   260,   261,   267,   271,   275,
     277,   279,   281,   283,   284,   292,   296,   300,   304,   308,
     313,   320,   329,   336,   341,   345,   349,   352,   355,   357,
     359,   361,   363,   365,   368,   372,   374,   375,   377,   381,
     383,   385,   388,   391,   396,   401,   404,   407,   411,   412,
     414,   419,   424,   428,   432,   435,   438,   440,   443,   446,
     449,   452,   455,   457,   460,   462,   465,   468,   471,   474,
     477,   480,   482,   485,   488,   491,   494,   497,   500,   503,
     506,   509,   512,   515,   518,   521,   524,   527,   530,   532,
     535,   538,   541,   544,   547,   550,   553,   556,   559,   562,
     565,   568,   571,   574,   577,   580,   583,   586,   589,   592,
     595,   598,   601,   604,   607,   610,   613,   616,   619,   622,
     625,   628,   631,   634,   637,   640,   643,   646,   649,   652,
     655,   658,   661,   664,   666,   668,   670,   672,   674,   676,
     678,   680,   682,   684,   686,   688,   690,   692,   694,   696,
     698,   700,   702,   704,   706,   708,   710,   712,   714,   716,
     718,   720,   722,   724,   726,   728,   730,   732,   734,   736,
     738,   740,   742,   744,   746,   748,   750,   752,   754,   756,
     758,   760,   762,   764,   766,   768,   770,   772,   774,   776,
     777,   779,   781,   783,   785,   787,   789,   791,   793,   796,
     799,   801,   806,   811,   813,   818,   820,   825,   826,   831,
     832,   839,   843,   844,   851,   855,   856,   858,   860,   863,
     870,   872,   876,   877,   879,   884,   891,   896,   898,   900,
     902,   904,   906,   908,   910,   911,   916,   918,   919,   922,
     924,   928,   932,   935,   936,   941,   943,   944,   949,   951,
     953,   955,   958,   961,   967,   971,   972,   973,   981,   982,
     983,   991,   993,   995,  1000,  1004,  1007,  1011,  1013,  1015,
    1017,  1019,  1023,  1026,  1028,  1030,  1034,  1037,  1041,  1045,
    1050,  1054,  1059,  1063,  1066,  1068,  1070,  1073,  1075,  1078,
    1080,  1083,  1084,  1092,  1098,  1099,  1107,  1113,  1114,  1123,
    1124,  1132,  1135,  1138,  1141,  1142,  1144,  1145,  1147,  1149,
    1152,  1153,  1157,  1160,  1165,  1169,  1174,  1178,  1180,  1182,
    1185,  1187,  1192,  1194,  1199,  1204,  1211,  1217,  1222,  1229,
    1235,  1237,  1241,  1243,  1245,  1249,  1250,  1254,  1255,  1257,
    1258,  1260,  1263,  1265,  1267,  1269,  1273,  1276,  1280,  1285,
    1289,  1292,  1295,  1297,  1302,  1306,  1311,  1317,  1323,  1325,
    1327,  1329,  1331,  1333,  1336,  1339,  1342,  1345,  1347,  1350,
    1353,  1356,  1358,  1361,  1364,  1367,  1370,  1372,  1375,  1377,
    1379,  1381,  1383,  1386,  1387,  1388,  1389,  1390,  1391,  1393,
    1395,  1398,  1402,  1404,  1407,  1409,  1411,  1417,  1419,  1421,
    1424,  1427,  1430,  1433,  1434,  1440,  1441,  1446,  1447,  1448,
    1450,  1453,  1457,  1461,  1465,  1466,  1471,  1473,  1477,  1478,
    1479,  1487,  1493,  1496,  1497,  1498,  1499,  1500,  1513,  1514,
    1521,  1524,  1526,  1528,  1531,  1535,  1538,  1541,  1544,  1548,
    1555,  1564,  1575,  1588,  1592,  1597,  1599,  1603,  1609,  1612,
    1618,  1619,  1621,  1622,  1624,  1625,  1627,  1629,  1633,  1638,
    1646,  1648,  1652,  1653,  1657,  1660,  1661,  1662,  1669,  1672,
    1673,  1675,  1677,  1681,  1683,  1687,  1692,  1697,  1701,  1706,
    1710,  1715,  1720,  1724,  1729,  1733,  1735,  1736,  1740,  1742,
    1745,  1747,  1751,  1753,  1757,  1759,  1761,  1763,  1765,  1767,
    1769,  1771,  1773,  1777,  1781,  1786,  1787,  1788,  1799,  1800,
    1807,  1808,  1809,  1822,  1823,  1832,  1833,  1840,  1843,  1844,
    1853,  1858,  1859,  1869,  1875,  1876,  1883,  1887,  1888,  1890,
    1894,  1898,  1900,  1902,  1904,  1906,  1907,  1911,  1914,  1918,
    1922,  1924,  1925,  1927,  1932,  1934,  1938,  1941,  1943,  1945,
    1946,  1947,  1948,  1956,  1957,  1958,  1961,  1963,  1965,  1968,
    1969,  1973,  1975,  1977,  1978,  1979,  1985,  1990,  1992,  1998,
    2001,  2002,  2005,  2006,  2008,  2010,  2012,  2015,  2018,  2023,
    2026,  2029,  2031,  2035,  2038,  2041,  2043,  2044,  2047,  2048,
    2052,  2054,  2056,  2059,  2061,  2063,  2065,  2067,  2069,  2071,
    2073,  2075,  2077,  2079,  2081,  2083,  2085,  2087,  2089,  2091,
    2093,  2095,  2097,  2099,  2101,  2103,  2105,  2107,  2109,  2111,
    2118,  2122,  2128,  2131,  2133,  2135,  2137,  2140,  2142,  2146,
    2149,  2151,  2153,  2158,  2160,  2162,  2164,  2167,  2170,  2172,
    2177,  2182
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short yyrhs[] =
{
      94,     0,    -1,    -1,    95,    -1,    -1,    96,    98,    -1,
      -1,    95,    97,    98,    -1,    99,    -1,   101,    -1,   100,
      -1,   296,    -1,    28,    67,   110,    84,    85,    -1,   295,
      98,    -1,   133,   167,    85,    -1,   153,   133,   167,    85,
      -1,   152,   133,   166,    85,    -1,   159,    85,    -1,     1,
      85,    -1,     1,    86,    -1,    85,    -1,    -1,    -1,   152,
     133,   196,   102,   127,   103,   256,   257,   245,    -1,   152,
     133,   196,     1,    -1,    -1,    -1,   153,   133,   201,   104,
     127,   105,   256,   257,   245,    -1,   153,   133,   201,     1,
      -1,    -1,    -1,   133,   201,   106,   127,   107,   256,   257,
     245,    -1,   133,   201,     1,    -1,     3,    -1,     4,    -1,
      81,    -1,    76,    -1,    52,    -1,    58,    -1,    57,    -1,
      63,    -1,    62,    -1,    87,    -1,    88,    -1,   112,    -1,
      -1,   112,    -1,   118,    -1,   112,    89,   118,    -1,   124,
      -1,    59,   117,    -1,   295,   117,    -1,   109,   117,    -1,
      49,   108,    -1,   114,   113,    -1,   114,    67,   222,    84,
      -1,   115,   113,    -1,   115,    67,   222,    84,    -1,    34,
     117,    -1,    35,   117,    -1,    12,    -1,    30,    -1,    29,
      -1,   113,    -1,    67,   222,    84,   117,    -1,   117,    -1,
     118,    57,   118,    -1,   118,    58,   118,    -1,   118,    59,
     118,    -1,   118,    60,   118,    -1,   118,    61,   118,    -1,
     118,    56,   118,    -1,   118,    55,   118,    -1,   118,    54,
     118,    -1,   118,    53,   118,    -1,   118,    52,   118,    -1,
     118,    50,   118,    -1,   118,    51,   118,    -1,    -1,   118,
      49,   119,   118,    -1,    -1,   118,    48,   120,   118,    -1,
      -1,    -1,   118,    46,   121,   110,    47,   122,   118,    -1,
      -1,   118,    46,   123,    47,   118,    -1,   118,    44,   118,
      -1,   118,    45,   118,    -1,     3,    -1,     9,    -1,    10,
      -1,    43,    -1,    -1,    67,   222,    84,    90,   125,   182,
      86,    -1,    67,   110,    84,    -1,    67,     1,    84,    -1,
     249,   247,    84,    -1,   249,     1,    84,    -1,   124,    67,
     111,    84,    -1,    36,    67,   118,    89,   222,    84,    -1,
      37,    67,   118,    89,   118,    89,   118,    84,    -1,    38,
      67,   222,    89,   222,    84,    -1,   124,    68,   110,    91,
      -1,   124,    66,   108,    -1,   124,    69,   108,    -1,   124,
      63,    -1,   124,    62,    -1,   352,    -1,   356,    -1,   357,
      -1,   358,    -1,   126,    -1,    92,    10,    -1,   126,    92,
      10,    -1,   128,    -1,    -1,   130,    -1,   256,   257,   131,
      -1,   129,    -1,   237,    -1,   130,   129,    -1,   129,   237,
      -1,   154,   133,   166,    85,    -1,   155,   133,   167,    85,
      -1,   154,    85,    -1,   155,    85,    -1,   256,   257,   135,
      -1,    -1,   173,    -1,   152,   133,   166,    85,    -1,   153,
     133,   167,    85,    -1,   152,   133,   190,    -1,   153,   133,
     193,    -1,   159,    85,    -1,   295,   135,    -1,     8,    -1,
     136,     8,    -1,   137,     8,    -1,   136,   174,    -1,   138,
       8,    -1,   139,     8,    -1,   174,    -1,   138,   174,    -1,
     161,    -1,   140,     8,    -1,   141,     8,    -1,   140,   163,
      -1,   141,   163,    -1,   136,   161,    -1,   137,   161,    -1,
     162,    -1,   140,   174,    -1,   140,   164,    -1,   141,   164,
      -1,   136,   162,    -1,   137,   162,    -1,   142,     8,    -1,
     143,     8,    -1,   142,   163,    -1,   143,   163,    -1,   138,
     161,    -1,   139,   161,    -1,   142,   174,    -1,   142,   164,
      -1,   143,   164,    -1,   138,   162,    -1,   139,   162,    -1,
     179,    -1,   144,     8,    -1,   145,     8,    -1,   136,   179,
      -1,   137,   179,    -1,   144,   179,    -1,   145,   179,    -1,
     144,   174,    -1,   146,     8,    -1,   147,     8,    -1,   138,
     179,    -1,   139,   179,    -1,   146,   179,    -1,   147,   179,
      -1,   146,   174,    -1,   148,     8,    -1,   149,     8,    -1,
     148,   163,    -1,   149,   163,    -1,   144,   161,    -1,   145,
     161,    -1,   140,   179,    -1,   141,   179,    -1,   148,   179,
      -1,   149,   179,    -1,   148,   174,    -1,   148,   164,    -1,
     149,   164,    -1,   144,   162,    -1,   145,   162,    -1,   150,
       8,    -1,   151,     8,    -1,   150,   163,    -1,   151,   163,
      -1,   146,   161,    -1,   147,   161,    -1,   142,   179,    -1,
     143,   179,    -1,   150,   179,    -1,   151,   179,    -1,   150,
     174,    -1,   150,   164,    -1,   151,   164,    -1,   146,   162,
      -1,   147,   162,    -1,   140,    -1,   141,    -1,   142,    -1,
     143,    -1,   148,    -1,   149,    -1,   150,    -1,   151,    -1,
     136,    -1,   137,    -1,   138,    -1,   139,    -1,   144,    -1,
     145,    -1,   146,    -1,   147,    -1,   140,    -1,   141,    -1,
     148,    -1,   149,    -1,   136,    -1,   137,    -1,   144,    -1,
     145,    -1,   140,    -1,   141,    -1,   142,    -1,   143,    -1,
     136,    -1,   137,    -1,   138,    -1,   139,    -1,   140,    -1,
     141,    -1,   142,    -1,   143,    -1,   136,    -1,   137,    -1,
     138,    -1,   139,    -1,   136,    -1,   137,    -1,   138,    -1,
     139,    -1,   140,    -1,   141,    -1,   142,    -1,   143,    -1,
     144,    -1,   145,    -1,   146,    -1,   147,    -1,   148,    -1,
     149,    -1,   150,    -1,   151,    -1,    -1,   157,    -1,   163,
      -1,   165,    -1,   164,    -1,     7,    -1,   210,    -1,   205,
      -1,     4,    -1,    76,   312,    -1,    81,   312,    -1,   313,
      -1,   116,    67,   110,    84,    -1,   116,    67,   222,    84,
      -1,   169,    -1,   166,    89,   134,   169,    -1,   171,    -1,
     167,    89,   134,   171,    -1,    -1,    28,    67,    10,    84,
      -1,    -1,   196,   168,   173,    44,   170,   180,    -1,   196,
     168,   173,    -1,    -1,   201,   168,   173,    44,   172,   180,
      -1,   201,   168,   173,    -1,    -1,   174,    -1,   175,    -1,
     174,   175,    -1,    31,    67,    67,   176,    84,    84,    -1,
     177,    -1,   176,    89,   177,    -1,    -1,   178,    -1,   178,
      67,     3,    84,    -1,   178,    67,     3,    89,   112,    84,
      -1,   178,    67,   111,    84,    -1,   108,    -1,   179,    -1,
       7,    -1,     8,    -1,     6,    -1,     5,    -1,   118,    -1,
      -1,    90,   181,   182,    86,    -1,     1,    -1,    -1,   183,
     211,    -1,   184,    -1,   183,    89,   184,    -1,   188,    44,
     186,    -1,   189,   186,    -1,    -1,   108,    47,   185,   186,
      -1,   186,    -1,    -1,    90,   187,   182,    86,    -1,   118,
      -1,     1,    -1,   189,    -1,   188,   189,    -1,    66,   108,
      -1,    68,   118,    11,   118,    91,    -1,    68,   118,    91,
      -1,    -1,    -1,   196,   191,   127,   192,   256,   257,   250,
      -1,    -1,    -1,   201,   194,   127,   195,   256,   257,   250,
      -1,   197,    -1,   201,    -1,    67,   173,   197,    84,    -1,
     197,    67,   290,    -1,   197,   230,    -1,    59,   160,   197,
      -1,     4,    -1,    81,    -1,   199,    -1,   200,    -1,   199,
      67,   290,    -1,   199,   230,    -1,     4,    -1,    81,    -1,
     200,    67,   290,    -1,   200,   230,    -1,    59,   160,   199,
      -1,    59,   160,   200,    -1,    67,   173,   200,    84,    -1,
     201,    67,   290,    -1,    67,   173,   201,    84,    -1,    59,
     160,   201,    -1,   201,   230,    -1,     3,    -1,    14,    -1,
      14,   174,    -1,    15,    -1,    15,   174,    -1,    13,    -1,
      13,   174,    -1,    -1,   202,   108,    90,   206,   213,    86,
     173,    -1,   202,    90,   213,    86,   173,    -1,    -1,   203,
     108,    90,   207,   213,    86,   173,    -1,   203,    90,   213,
      86,   173,    -1,    -1,   204,   108,    90,   208,   220,   212,
      86,   173,    -1,    -1,   204,    90,   209,   220,   212,    86,
     173,    -1,   202,   108,    -1,   203,   108,    -1,   204,   108,
      -1,    -1,    89,    -1,    -1,    89,    -1,   214,    -1,   214,
     215,    -1,    -1,   214,   215,    85,    -1,   214,    85,    -1,
      74,    67,    76,    84,    -1,   156,   133,   216,    -1,   156,
     133,   256,   257,    -1,   157,   133,   217,    -1,   157,    -1,
       1,    -1,   295,   215,    -1,   218,    -1,   216,    89,   134,
     218,    -1,   219,    -1,   217,    89,   134,   219,    -1,   256,
     257,   196,   173,    -1,   256,   257,   196,    47,   118,   173,
      -1,   256,   257,    47,   118,   173,    -1,   256,   257,   201,
     173,    -1,   256,   257,   201,    47,   118,   173,    -1,   256,
     257,    47,   118,   173,    -1,   221,    -1,   220,    89,   221,
      -1,     1,    -1,   108,    -1,   108,    44,   118,    -1,    -1,
     158,   223,   224,    -1,    -1,   226,    -1,    -1,   226,    -1,
     227,   174,    -1,   228,    -1,   227,    -1,   229,    -1,    59,
     160,   227,    -1,    59,   160,    -1,    59,   160,   228,    -1,
      67,   173,   226,    84,    -1,   229,    67,   280,    -1,   229,
     230,    -1,    67,   280,    -1,   230,    -1,    68,   160,   110,
      91,    -1,    68,   160,    91,    -1,    68,   160,    59,    91,
      -1,    68,     6,   160,   110,    91,    -1,    68,   157,     6,
     110,    91,    -1,   232,    -1,   233,    -1,   234,    -1,   235,
      -1,   260,    -1,   232,   260,    -1,   233,   260,    -1,   234,
     260,    -1,   235,   260,    -1,   132,    -1,   232,   132,    -1,
     233,   132,    -1,   235,   132,    -1,   261,    -1,   232,   261,
      -1,   233,   261,    -1,   234,   261,    -1,   235,   261,    -1,
     237,    -1,   236,   237,    -1,   232,    -1,   233,    -1,   234,
      -1,   235,    -1,     1,    85,    -1,    -1,    -1,    -1,    -1,
      -1,   243,    -1,   244,    -1,   243,   244,    -1,    33,   294,
      85,    -1,   250,    -1,     1,   250,    -1,    90,    -1,    86,
      -1,   238,   242,   248,    86,   239,    -1,   231,    -1,     1,
      -1,    67,    90,    -1,   246,   247,    -1,   252,   259,    -1,
     252,     1,    -1,    -1,    16,   253,    67,   110,    84,    -1,
      -1,    19,   255,   259,    18,    -1,    -1,    -1,   260,    -1,
     261,   258,    -1,   240,   258,   241,    -1,   256,   257,   272,
      -1,   256,   257,   273,    -1,    -1,   251,    17,   263,   259,
      -1,   251,    -1,   251,    17,     1,    -1,    -1,    -1,    18,
     264,    67,   110,    84,   265,   259,    -1,   254,    67,   110,
      84,    85,    -1,   254,     1,    -1,    -1,    -1,    -1,    -1,
      20,   266,    67,   271,   267,   275,    85,   268,   275,    84,
     269,   259,    -1,    -1,    21,    67,   110,    84,   270,   259,
      -1,   275,    85,    -1,   135,    -1,   250,    -1,   110,    85,
      -1,   240,   262,   241,    -1,    24,    85,    -1,    25,    85,
      -1,    26,    85,    -1,    26,   110,    85,    -1,    28,   274,
      67,   110,    84,    85,    -1,    28,   274,    67,   110,    47,
     276,    84,    85,    -1,    28,   274,    67,   110,    47,   276,
      47,   276,    84,    85,    -1,    28,   274,    67,   110,    47,
     276,    47,   276,    47,   279,    84,    85,    -1,    27,   108,
      85,    -1,    27,    59,   110,    85,    -1,    85,    -1,    22,
     118,    47,    -1,    22,   118,    11,   118,    47,    -1,    23,
      47,    -1,   108,   256,   257,    47,   173,    -1,    -1,     8,
      -1,    -1,   110,    -1,    -1,   277,    -1,   278,    -1,   277,
      89,   278,    -1,    10,    67,   110,    84,    -1,    68,   108,
      91,    10,    67,   110,    84,    -1,    10,    -1,   279,    89,
      10,    -1,    -1,   173,   281,   282,    -1,   285,    84,    -1,
      -1,    -1,   286,    85,   283,   173,   284,   282,    -1,     1,
      84,    -1,    -1,    11,    -1,   286,    -1,   286,    89,    11,
      -1,   288,    -1,   286,    89,   287,    -1,   152,   133,   198,
     173,    -1,   152,   133,   201,   173,    -1,   152,   133,   225,
      -1,   153,   133,   201,   173,    -1,   153,   133,   225,    -1,
     154,   289,   198,   173,    -1,   154,   289,   201,   173,    -1,
     154,   289,   225,    -1,   155,   289,   201,   173,    -1,   155,
     289,   225,    -1,   133,    -1,    -1,   173,   291,   292,    -1,
     282,    -1,   293,    84,    -1,     3,    -1,   293,    89,     3,
      -1,   108,    -1,   294,    89,   108,    -1,    32,    -1,   300,
      -1,   298,    -1,   299,    -1,   310,    -1,   321,    -1,    72,
      -1,   108,    -1,   297,    89,   108,    -1,    82,   297,    85,
      -1,    83,   108,   108,    85,    -1,    -1,    -1,    70,   108,
     312,    90,   301,   314,    86,   302,   325,    72,    -1,    -1,
      70,   108,   312,   303,   325,    72,    -1,    -1,    -1,    70,
     108,    47,   108,   312,    90,   304,   314,    86,   305,   325,
      72,    -1,    -1,    70,   108,    47,   108,   312,   306,   325,
      72,    -1,    -1,    71,   108,    90,   307,   314,    86,    -1,
      71,   108,    -1,    -1,    71,   108,    47,   108,    90,   308,
     314,    86,    -1,    71,   108,    47,   108,    -1,    -1,    70,
     108,    67,   108,    84,   312,   309,   325,    72,    -1,    71,
     108,    67,   108,    84,    -1,    -1,    80,   108,   312,   311,
     325,    72,    -1,    80,   297,    85,    -1,    -1,   313,    -1,
      54,   297,    54,    -1,   314,   315,   316,    -1,   316,    -1,
      78,    -1,    79,    -1,    77,    -1,    -1,   316,   317,    85,
      -1,   316,    85,    -1,   156,   133,   318,    -1,   157,   133,
     318,    -1,     1,    -1,    -1,   319,    -1,   318,    89,   134,
     319,    -1,   196,    -1,   196,    47,   118,    -1,    47,   118,
      -1,    57,    -1,    58,    -1,    -1,    -1,    -1,   320,   322,
     333,   323,   334,   324,   245,    -1,    -1,    -1,   326,   327,
      -1,   330,    -1,   100,    -1,   327,   330,    -1,    -1,   327,
     328,   100,    -1,    85,    -1,     1,    -1,    -1,    -1,   320,
     331,   333,   332,   329,    -1,    67,   222,    84,   342,    -1,
     342,    -1,    67,   222,    84,   343,   340,    -1,   343,   340,
      -1,    -1,    85,   335,    -1,    -1,   336,    -1,   337,    -1,
     237,    -1,   336,   337,    -1,   337,   237,    -1,   152,   133,
     338,    85,    -1,   152,    85,    -1,   153,    85,    -1,   339,
      -1,   338,    89,   339,    -1,   198,   173,    -1,   201,   173,
      -1,   225,    -1,    -1,    89,    11,    -1,    -1,    89,   341,
     285,    -1,   344,    -1,   346,    -1,   343,   346,    -1,     3,
      -1,     4,    -1,    76,    -1,    81,    -1,   345,    -1,    13,
      -1,    14,    -1,    15,    -1,    16,    -1,    17,    -1,    18,
      -1,    19,    -1,    20,    -1,    21,    -1,    22,    -1,    23,
      -1,    24,    -1,    25,    -1,    26,    -1,    27,    -1,    28,
      -1,    12,    -1,    29,    -1,    30,    -1,     7,    -1,     8,
      -1,   344,    47,    67,   222,    84,   108,    -1,   344,    47,
     108,    -1,    47,    67,   222,    84,   108,    -1,    47,   108,
      -1,   344,    -1,   348,    -1,   350,    -1,   348,   350,    -1,
     112,    -1,   344,    47,   349,    -1,    47,   349,    -1,   110,
      -1,    76,    -1,    68,   351,   347,    91,    -1,   344,    -1,
     354,    -1,   355,    -1,   354,   355,    -1,   344,    47,    -1,
      47,    -1,    73,    67,   353,    84,    -1,    80,    67,   108,
      84,    -1,    75,    67,   222,    84,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   345,   345,   349,   368,   368,   369,   369,   373,   378,
     379,   380,   381,   389,   394,   401,   403,   405,   407,   408,
     409,   416,   421,   415,   427,   430,   435,   429,   441,   444,
     449,   443,   455,   460,   461,   462,   463,   466,   468,   470,
     473,   475,   477,   479,   483,   489,   490,   494,   496,   501,
     502,   505,   508,   512,   514,   520,   523,   526,   529,   531,
     536,   540,   544,   548,   549,   554,   555,   557,   559,   561,
     563,   565,   567,   569,   571,   573,   575,   577,   580,   579,
     587,   586,   594,   598,   593,   604,   603,   614,   621,   633,
     639,   640,   642,   645,   644,   657,   662,   664,   680,   687,
     689,   692,   702,   712,   714,   721,   730,   732,   734,   736,
     738,   740,   742,   749,   751,   756,   764,   770,   777,   782,
     783,   784,   785,   793,   795,   797,   800,   809,   818,   828,
     833,   835,   837,   839,   841,   843,   900,   903,   906,   912,
     918,   921,   927,   930,   936,   939,   942,   945,   948,   951,
     954,   960,   963,   966,   969,   972,   975,   981,   984,   987,
     990,   993,   996,  1002,  1005,  1008,  1011,  1014,  1020,  1023,
    1026,  1029,  1035,  1041,  1047,  1056,  1062,  1065,  1068,  1074,
    1080,  1086,  1095,  1101,  1104,  1107,  1110,  1113,  1116,  1119,
    1125,  1131,  1137,  1146,  1149,  1152,  1155,  1158,  1164,  1167,
    1170,  1173,  1176,  1179,  1182,  1188,  1194,  1200,  1209,  1212,
    1215,  1218,  1221,  1228,  1229,  1230,  1231,  1232,  1233,  1234,
    1235,  1239,  1240,  1241,  1242,  1243,  1244,  1245,  1246,  1250,
    1251,  1252,  1253,  1257,  1258,  1259,  1260,  1264,  1265,  1266,
    1267,  1271,  1272,  1273,  1274,  1278,  1279,  1280,  1281,  1282,
    1283,  1284,  1285,  1289,  1290,  1291,  1292,  1293,  1294,  1295,
    1296,  1297,  1298,  1299,  1300,  1301,  1302,  1303,  1304,  1310,
    1311,  1337,  1338,  1342,  1346,  1348,  1352,  1356,  1360,  1362,
    1367,  1369,  1371,  1378,  1379,  1383,  1384,  1389,  1390,  1396,
    1395,  1403,  1412,  1411,  1419,  1428,  1429,  1434,  1436,  1441,
    1446,  1448,  1454,  1455,  1457,  1459,  1461,  1469,  1470,  1471,
    1472,  1476,  1477,  1483,  1485,  1484,  1488,  1495,  1497,  1501,
    1502,  1508,  1511,  1515,  1514,  1520,  1525,  1524,  1528,  1530,
    1534,  1535,  1539,  1541,  1545,  1551,  1564,  1550,  1582,  1595,
    1581,  1615,  1616,  1622,  1624,  1629,  1631,  1633,  1634,  1642,
    1643,  1647,  1652,  1654,  1655,  1659,  1664,  1666,  1668,  1670,
    1678,  1683,  1685,  1687,  1689,  1693,  1695,  1700,  1702,  1707,
    1709,  1721,  1720,  1726,  1731,  1730,  1734,  1739,  1738,  1744,
    1743,  1751,  1753,  1755,  1763,  1765,  1768,  1770,  1776,  1778,
    1784,  1785,  1787,  1791,  1807,  1810,  1820,  1823,  1828,  1830,
    1836,  1837,  1842,  1843,  1848,  1851,  1855,  1861,  1864,  1868,
    1879,  1880,  1885,  1891,  1893,  1899,  1898,  1907,  1908,  1913,
    1916,  1920,  1927,  1928,  1932,  1933,  1938,  1940,  1945,  1947,
    1949,  1951,  1953,  1960,  1962,  1964,  1966,  1969,  1980,  1981,
    1982,  1986,  1990,  1991,  1992,  1993,  1994,  1998,  1999,  2002,
    2003,  2007,  2008,  2009,  2010,  2011,  2015,  2016,  2020,  2021,
    2022,  2023,  2026,  2030,  2039,  2044,  2062,  2076,  2078,  2084,
    2085,  2089,  2103,  2105,  2108,  2112,  2114,  2122,  2123,  2127,
    2145,  2153,  2158,  2171,  2170,  2185,  2184,  2204,  2210,  2216,
    2217,  2222,  2228,  2242,  2252,  2251,  2259,  2271,  2282,  2285,
    2281,  2291,  2294,  2297,  2301,  2304,  2308,  2296,  2312,  2311,
    2319,  2321,  2327,  2329,  2332,  2336,  2339,  2342,  2345,  2348,
    2352,  2356,  2361,  2365,  2377,  2383,  2391,  2394,  2397,  2400,
    2417,  2419,  2425,  2426,  2432,  2433,  2437,  2438,  2443,  2445,
    2452,  2454,  2465,  2464,  2475,  2477,  2485,  2476,  2489,  2496,
    2497,  2507,  2511,  2516,  2518,  2525,  2530,  2535,  2538,  2544,
    2552,  2557,  2562,  2565,  2571,  2577,  2587,  2586,  2597,  2598,
    2616,  2618,  2624,  2626,  2631,  2642,  2643,  2644,  2645,  2646,
    2647,  2662,  2664,  2669,  2676,  2684,  2690,  2683,  2701,  2700,
    2714,  2720,  2713,  2731,  2730,  2744,  2743,  2755,  2764,  2763,
    2775,  2784,  2783,  2796,  2807,  2806,  2821,  2829,  2832,  2836,
    2846,  2847,  2851,  2852,  2853,  2858,  2861,  2862,  2880,  2883,
    2886,  2892,  2893,  2894,  2898,  2905,  2911,  2921,  2923,  2929,
    2935,  2944,  2928,  2957,  2959,  2959,  2963,  2964,  2965,  2966,
    2966,  2970,  2971,  2976,  2981,  2975,  2993,  2998,  3003,  3008,
    3017,  3019,  3025,  3027,  3031,  3032,  3033,  3034,  3038,  3040,
    3042,  3047,  3049,  3057,  3061,  3065,  3071,  3074,  3080,  3079,
    3092,  3096,  3098,  3105,  3106,  3107,  3108,  3109,  3113,  3113,
    3113,  3113,  3113,  3113,  3113,  3113,  3114,  3114,  3114,  3114,
    3114,  3114,  3115,  3115,  3115,  3115,  3115,  3116,  3116,  3120,
    3125,  3130,  3135,  3142,  3143,  3147,  3148,  3156,  3168,  3172,
    3179,  3180,  3187,  3192,  3193,  3197,  3198,  3205,  3209,  3216,
    3223,  3232
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "IDENTIFIER", "TYPENAME", "SCSPEC", 
  "STATIC", "TYPESPEC", "TYPE_QUAL", "CONSTANT", "STRING", "ELLIPSIS", 
  "SIZEOF", "ENUM", "STRUCT", "UNION", "IF", "ELSE", "WHILE", "DO", "FOR", 
  "SWITCH", "CASE", "DEFAULT", "BREAK", "CONTINUE", "RETURN", "GOTO", 
  "ASM_KEYWORD", "TYPEOF", "ALIGNOF", "ATTRIBUTE", "EXTENSION", "LABEL", 
  "REALPART", "IMAGPART", "VA_ARG", "CHOOSE_EXPR", "TYPES_COMPATIBLE_P", 
  "PTR_VALUE", "PTR_BASE", "PTR_EXTENT", "STRING_FUNC_NAME", 
  "VAR_FUNC_NAME", "'='", "ASSIGN", "'?'", "':'", "OROR", "ANDAND", "'|'", 
  "'^'", "'&'", "EQCOMPARE", "ARITHCOMPARE", "RSHIFT", "LSHIFT", "'+'", 
  "'-'", "'*'", "'/'", "'%'", "MINUSMINUS", "PLUSPLUS", "UNARY", 
  "HYPERUNARY", "'.'", "'('", "'['", "POINTSAT", "INTERFACE", 
  "IMPLEMENTATION", "END", "SELECTOR", "DEFS", "ENCODE", "CLASSNAME", 
  "PUBLIC", "PRIVATE", "PROTECTED", "PROTOCOL", "OBJECTNAME", "CLASS", 
  "ALIAS", "')'", "';'", "'}'", "'~'", "'!'", "','", "'{'", "']'", "'@'", 
  "$accept", "program", "extdefs", "@1", "@2", "extdef", "extdef_1", 
  "datadef", "fndef", "@3", "@4", "@5", "@6", "@7", "@8", "identifier", 
  "unop", "expr", "exprlist", "nonnull_exprlist", "unary_expr", "sizeof", 
  "alignof", "typeof", "cast_expr", "expr_no_commas", "@9", "@10", "@11", 
  "@12", "@13", "primary", "@14", "objc_string", "old_style_parm_decls", 
  "old_style_parm_decls_1", "lineno_datadecl", "datadecls", "datadecl", 
  "lineno_decl", "setspecs", "maybe_resetattrs", "decl", 
  "declspecs_nosc_nots_nosa_noea", "declspecs_nosc_nots_nosa_ea", 
  "declspecs_nosc_nots_sa_noea", "declspecs_nosc_nots_sa_ea", 
  "declspecs_nosc_ts_nosa_noea", "declspecs_nosc_ts_nosa_ea", 
  "declspecs_nosc_ts_sa_noea", "declspecs_nosc_ts_sa_ea", 
  "declspecs_sc_nots_nosa_noea", "declspecs_sc_nots_nosa_ea", 
  "declspecs_sc_nots_sa_noea", "declspecs_sc_nots_sa_ea", 
  "declspecs_sc_ts_nosa_noea", "declspecs_sc_ts_nosa_ea", 
  "declspecs_sc_ts_sa_noea", "declspecs_sc_ts_sa_ea", "declspecs_ts", 
  "declspecs_nots", "declspecs_ts_nosa", "declspecs_nots_nosa", 
  "declspecs_nosc_ts", "declspecs_nosc_nots", "declspecs_nosc", 
  "declspecs", "maybe_type_quals_attrs", "typespec_nonattr", 
  "typespec_attr", "typespec_reserved_nonattr", "typespec_reserved_attr", 
  "typespec_nonreserved_nonattr", "initdecls", "notype_initdecls", 
  "maybeasm", "initdcl", "@15", "notype_initdcl", "@16", 
  "maybe_attribute", "attributes", "attribute", "attribute_list", 
  "attrib", "any_word", "scspec", "init", "@17", "initlist_maybe_comma", 
  "initlist1", "initelt", "@18", "initval", "@19", "designator_list", 
  "designator", "nested_function", "@20", "@21", "notype_nested_function", 
  "@22", "@23", "declarator", "after_type_declarator", "parm_declarator", 
  "parm_declarator_starttypename", "parm_declarator_nostarttypename", 
  "notype_declarator", "struct_head", "union_head", "enum_head", 
  "structsp_attr", "@24", "@25", "@26", "@27", "structsp_nonattr", 
  "maybecomma", "maybecomma_warn", "component_decl_list", 
  "component_decl_list2", "component_decl", "components", 
  "components_notype", "component_declarator", 
  "component_notype_declarator", "enumlist", "enumerator", "typename", 
  "@28", "absdcl", "absdcl_maybe_attribute", "absdcl1", "absdcl1_noea", 
  "absdcl1_ea", "direct_absdcl1", "array_declarator", "stmts_and_decls", 
  "lineno_stmt_decl_or_labels_ending_stmt", 
  "lineno_stmt_decl_or_labels_ending_decl", 
  "lineno_stmt_decl_or_labels_ending_label", 
  "lineno_stmt_decl_or_labels_ending_error", "lineno_stmt_decl_or_labels", 
  "errstmt", "pushlevel", "poplevel", "c99_block_start", "c99_block_end", 
  "maybe_label_decls", "label_decls", "label_decl", "compstmt_or_error", 
  "compstmt_start", "compstmt_nostart", "compstmt_contents_nonempty", 
  "compstmt_primary_start", "compstmt", "simple_if", "if_prefix", "@29", 
  "do_stmt_start", "@30", "save_filename", "save_lineno", 
  "lineno_labeled_stmt", "c99_block_lineno_labeled_stmt", "lineno_stmt", 
  "lineno_label", "select_or_iter_stmt", "@31", "@32", "@33", "@34", 
  "@35", "@36", "@37", "@38", "for_init_stmt", "stmt", "label", 
  "maybe_type_qual", "xexpr", "asm_operands", "nonnull_asm_operands", 
  "asm_operand", "asm_clobbers", "parmlist", "@39", "parmlist_1", "@40", 
  "@41", "parmlist_2", "parms", "parm", "firstparm", "setspecs_fp", 
  "parmlist_or_identifiers", "@42", "parmlist_or_identifiers_1", 
  "identifiers", "identifiers_or_typenames", "extension", "objcdef", 
  "identifier_list", "classdecl", "aliasdecl", "classdef", "@43", "@44", 
  "@45", "@46", "@47", "@48", "@49", "@50", "@51", "protocoldef", "@52", 
  "protocolrefs", "non_empty_protocolrefs", "ivar_decl_list", 
  "visibility_spec", "ivar_decls", "ivar_decl", "ivars", 
  "ivar_declarator", "methodtype", "methoddef", "@53", "@54", "@55", 
  "methodprotolist", "@56", "methodprotolist2", "@57", "semi_or_error", 
  "methodproto", "@58", "@59", "methoddecl", "optarglist", "myxdecls", 
  "mydecls", "mydecl", "myparms", "myparm", "optparmlist", "@60", 
  "unaryselector", "keywordselector", "selector", "reservedwords", 
  "keyworddecl", "messageargs", "keywordarglist", "keywordexpr", 
  "keywordarg", "receiver", "objcmessageexpr", "selectorarg", 
  "keywordnamelist", "keywordname", "objcselectorexpr", 
  "objcprotocolexpr", "objcencodeexpr", 0
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
     295,   296,   297,   298,    61,   299,    63,    58,   300,   301,
     124,    94,    38,   302,   303,   304,   305,    43,    45,    42,
      47,    37,   306,   307,   308,   309,    46,    40,    91,   310,
     311,   312,   313,   314,   315,   316,   317,   318,   319,   320,
     321,   322,   323,   324,    41,    59,   125,   126,    33,    44,
     123,    93,    64
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned short yyr1[] =
{
       0,    93,    94,    94,    96,    95,    97,    95,    98,    99,
      99,    99,    99,    99,   100,   100,   100,   100,   100,   100,
     100,   102,   103,   101,   101,   104,   105,   101,   101,   106,
     107,   101,   101,   108,   108,   108,   108,   109,   109,   109,
     109,   109,   109,   109,   110,   111,   111,   112,   112,   113,
     113,   113,   113,   113,   113,   113,   113,   113,   113,   113,
     114,   115,   116,   117,   117,   118,   118,   118,   118,   118,
     118,   118,   118,   118,   118,   118,   118,   118,   119,   118,
     120,   118,   121,   122,   118,   123,   118,   118,   118,   124,
     124,   124,   124,   125,   124,   124,   124,   124,   124,   124,
     124,   124,   124,   124,   124,   124,   124,   124,   124,   124,
     124,   124,   124,   126,   126,   127,   128,   128,   129,   130,
     130,   130,   130,   131,   131,   131,   131,   132,   133,   134,
     135,   135,   135,   135,   135,   135,   136,   136,   136,   137,
     138,   138,   139,   139,   140,   140,   140,   140,   140,   140,
     140,   141,   141,   141,   141,   141,   141,   142,   142,   142,
     142,   142,   142,   143,   143,   143,   143,   143,   144,   144,
     144,   144,   144,   144,   144,   145,   146,   146,   146,   146,
     146,   146,   147,   148,   148,   148,   148,   148,   148,   148,
     148,   148,   148,   149,   149,   149,   149,   149,   150,   150,
     150,   150,   150,   150,   150,   150,   150,   150,   151,   151,
     151,   151,   151,   152,   152,   152,   152,   152,   152,   152,
     152,   153,   153,   153,   153,   153,   153,   153,   153,   154,
     154,   154,   154,   155,   155,   155,   155,   156,   156,   156,
     156,   157,   157,   157,   157,   158,   158,   158,   158,   158,
     158,   158,   158,   159,   159,   159,   159,   159,   159,   159,
     159,   159,   159,   159,   159,   159,   159,   159,   159,   160,
     160,   161,   161,   162,   163,   163,   164,   165,   165,   165,
     165,   165,   165,   166,   166,   167,   167,   168,   168,   170,
     169,   169,   172,   171,   171,   173,   173,   174,   174,   175,
     176,   176,   177,   177,   177,   177,   177,   178,   178,   178,
     178,   179,   179,   180,   181,   180,   180,   182,   182,   183,
     183,   184,   184,   185,   184,   184,   187,   186,   186,   186,
     188,   188,   189,   189,   189,   191,   192,   190,   194,   195,
     193,   196,   196,   197,   197,   197,   197,   197,   197,   198,
     198,   199,   199,   199,   199,   200,   200,   200,   200,   200,
     201,   201,   201,   201,   201,   202,   202,   203,   203,   204,
     204,   206,   205,   205,   207,   205,   205,   208,   205,   209,
     205,   210,   210,   210,   211,   211,   212,   212,   213,   213,
     214,   214,   214,   214,   215,   215,   215,   215,   215,   215,
     216,   216,   217,   217,   218,   218,   218,   219,   219,   219,
     220,   220,   220,   221,   221,   223,   222,   224,   224,   225,
     225,   225,   226,   226,   227,   227,   228,   228,   229,   229,
     229,   229,   229,   230,   230,   230,   230,   230,   231,   231,
     231,   231,   232,   232,   232,   232,   232,   233,   233,   233,
     233,   234,   234,   234,   234,   234,   235,   235,   236,   236,
     236,   236,   237,   238,   239,   240,   241,   242,   242,   243,
     243,   244,   245,   245,   246,   247,   247,   248,   248,   249,
     250,   251,   251,   253,   252,   255,   254,   256,   257,   258,
     258,   259,   260,   261,   263,   262,   262,   262,   264,   265,
     262,   262,   262,   266,   267,   268,   269,   262,   270,   262,
     271,   271,   272,   272,   272,   272,   272,   272,   272,   272,
     272,   272,   272,   272,   272,   272,   273,   273,   273,   273,
     274,   274,   275,   275,   276,   276,   277,   277,   278,   278,
     279,   279,   281,   280,   282,   283,   284,   282,   282,   285,
     285,   285,   285,   286,   286,   287,   287,   287,   287,   287,
     288,   288,   288,   288,   288,   289,   291,   290,   292,   292,
     293,   293,   294,   294,   295,   296,   296,   296,   296,   296,
     296,   297,   297,   298,   299,   301,   302,   300,   303,   300,
     304,   305,   300,   306,   300,   307,   300,   300,   308,   300,
     300,   309,   300,   300,   311,   310,   310,   312,   312,   313,
     314,   314,   315,   315,   315,   316,   316,   316,   317,   317,
     317,   318,   318,   318,   319,   319,   319,   320,   320,   322,
     323,   324,   321,   325,   326,   325,   327,   327,   327,   328,
     327,   329,   329,   331,   332,   330,   333,   333,   333,   333,
     334,   334,   335,   335,   336,   336,   336,   336,   337,   337,
     337,   338,   338,   339,   339,   339,   340,   340,   341,   340,
     342,   343,   343,   344,   344,   344,   344,   344,   345,   345,
     345,   345,   345,   345,   345,   345,   345,   345,   345,   345,
     345,   345,   345,   345,   345,   345,   345,   345,   345,   346,
     346,   346,   346,   347,   347,   348,   348,   349,   350,   350,
     351,   351,   352,   353,   353,   354,   354,   355,   355,   356,
     357,   358
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     0,     1,     0,     2,     0,     3,     1,     1,
       1,     1,     5,     2,     3,     4,     4,     2,     2,     2,
       1,     0,     0,     9,     4,     0,     0,     9,     4,     0,
       0,     8,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     0,     1,     1,     3,     1,
       2,     2,     2,     2,     2,     4,     2,     4,     2,     2,
       1,     1,     1,     1,     4,     1,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     0,     4,
       0,     4,     0,     0,     7,     0,     5,     3,     3,     1,
       1,     1,     1,     0,     7,     3,     3,     3,     3,     4,
       6,     8,     6,     4,     3,     3,     2,     2,     1,     1,
       1,     1,     1,     2,     3,     1,     0,     1,     3,     1,
       1,     2,     2,     4,     4,     2,     2,     3,     0,     1,
       4,     4,     3,     3,     2,     2,     1,     2,     2,     2,
       2,     2,     1,     2,     1,     2,     2,     2,     2,     2,
       2,     1,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     1,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     0,
       1,     1,     1,     1,     1,     1,     1,     1,     2,     2,
       1,     4,     4,     1,     4,     1,     4,     0,     4,     0,
       6,     3,     0,     6,     3,     0,     1,     1,     2,     6,
       1,     3,     0,     1,     4,     6,     4,     1,     1,     1,
       1,     1,     1,     1,     0,     4,     1,     0,     2,     1,
       3,     3,     2,     0,     4,     1,     0,     4,     1,     1,
       1,     2,     2,     5,     3,     0,     0,     7,     0,     0,
       7,     1,     1,     4,     3,     2,     3,     1,     1,     1,
       1,     3,     2,     1,     1,     3,     2,     3,     3,     4,
       3,     4,     3,     2,     1,     1,     2,     1,     2,     1,
       2,     0,     7,     5,     0,     7,     5,     0,     8,     0,
       7,     2,     2,     2,     0,     1,     0,     1,     1,     2,
       0,     3,     2,     4,     3,     4,     3,     1,     1,     2,
       1,     4,     1,     4,     4,     6,     5,     4,     6,     5,
       1,     3,     1,     1,     3,     0,     3,     0,     1,     0,
       1,     2,     1,     1,     1,     3,     2,     3,     4,     3,
       2,     2,     1,     4,     3,     4,     5,     5,     1,     1,
       1,     1,     1,     2,     2,     2,     2,     1,     2,     2,
       2,     1,     2,     2,     2,     2,     1,     2,     1,     1,
       1,     1,     2,     0,     0,     0,     0,     0,     1,     1,
       2,     3,     1,     2,     1,     1,     5,     1,     1,     2,
       2,     2,     2,     0,     5,     0,     4,     0,     0,     1,
       2,     3,     3,     3,     0,     4,     1,     3,     0,     0,
       7,     5,     2,     0,     0,     0,     0,    12,     0,     6,
       2,     1,     1,     2,     3,     2,     2,     2,     3,     6,
       8,    10,    12,     3,     4,     1,     3,     5,     2,     5,
       0,     1,     0,     1,     0,     1,     1,     3,     4,     7,
       1,     3,     0,     3,     2,     0,     0,     6,     2,     0,
       1,     1,     3,     1,     3,     4,     4,     3,     4,     3,
       4,     4,     3,     4,     3,     1,     0,     3,     1,     2,
       1,     3,     1,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     3,     3,     4,     0,     0,    10,     0,     6,
       0,     0,    12,     0,     8,     0,     6,     2,     0,     8,
       4,     0,     9,     5,     0,     6,     3,     0,     1,     3,
       3,     1,     1,     1,     1,     0,     3,     2,     3,     3,
       1,     0,     1,     4,     1,     3,     2,     1,     1,     0,
       0,     0,     7,     0,     0,     2,     1,     1,     2,     0,
       3,     1,     1,     0,     0,     5,     4,     1,     5,     2,
       0,     2,     0,     1,     1,     1,     2,     2,     4,     2,
       2,     1,     3,     2,     2,     1,     0,     2,     0,     3,
       1,     1,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     6,
       3,     5,     2,     1,     1,     1,     2,     1,     3,     2,
       1,     1,     4,     1,     1,     1,     2,     2,     1,     4,
       4,     4
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned short yydefact[] =
{
       4,     0,     6,     0,     1,     0,     0,   277,   312,   311,
     274,   136,   369,   365,   367,     0,    62,     0,   574,     0,
     627,   628,     0,     0,   580,   607,     0,   607,     0,     0,
      20,     5,     8,    10,     9,     0,     0,   221,   222,   223,
     224,   213,   214,   215,   216,   225,   226,   227,   228,   217,
     218,   219,   220,   128,   128,     0,   144,   151,   271,   273,
     272,   142,   297,   168,     0,     0,     0,   276,   275,     0,
      11,   576,   577,   575,   578,   280,   629,   579,     7,    18,
      19,   370,   366,   368,     0,     0,    33,    34,    36,    35,
     581,     0,   607,   597,   278,   608,   607,     0,   279,     0,
       0,     0,   364,   269,   295,     0,   285,     0,   137,   149,
     155,   139,   171,   138,   150,   156,   172,   140,   161,   166,
     143,   178,   141,   162,   167,   179,   145,   147,   153,   152,
     189,   146,   148,   154,   190,   157,   159,   164,   163,   204,
     158,   160,   165,   205,   169,   187,   196,   175,   173,   170,
     188,   197,   174,   176,   202,   211,   182,   180,   177,   203,
     212,   181,   183,   185,   194,   193,   191,   184,   186,   195,
     192,   198,   200,   209,   208,   206,   199,   201,   210,   207,
       0,     0,    17,   298,   390,   381,   390,   382,   379,   383,
      13,     0,    89,    90,    91,    60,    61,     0,     0,     0,
       0,     0,    92,     0,    37,    39,    38,     0,    41,    40,
       0,     0,     0,     0,     0,    42,    43,     0,     0,     0,
      44,    63,     0,     0,    65,    47,    49,   112,     0,     0,
     108,   109,   110,   111,   302,   609,     0,     0,     0,   588,
       0,     0,   595,   604,   606,   583,     0,     0,   249,   250,
     251,   252,   245,   246,   247,   248,   415,     0,   241,   242,
     243,   244,   270,     0,     0,   296,    14,   295,    32,     0,
     295,   269,     0,   295,   363,   347,   269,   295,   348,     0,
     283,     0,   341,   342,     0,     0,     0,     0,     0,   371,
       0,   374,     0,   377,   673,   674,   697,   698,   694,   678,
     679,   680,   681,   682,   683,   684,   685,   686,   687,   688,
     689,   690,   691,   692,   693,   695,   696,     0,     0,   675,
     676,   630,   647,   666,   670,   677,   671,    58,    59,     0,
       0,     0,    53,    50,     0,   479,     0,     0,   711,   710,
       0,     0,     0,     0,   113,    52,     0,     0,     0,    54,
       0,    56,     0,     0,    82,    80,    78,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   107,
     106,     0,    45,     0,     0,     0,     0,   475,   467,     0,
      51,   309,   310,   307,     0,   300,   303,   308,   582,   607,
       0,   585,   634,   600,     0,   615,   634,   584,   281,   417,
     282,   362,     0,     0,   129,     0,   566,   360,   269,   270,
       0,     0,    30,   115,     0,   487,   120,   488,   294,     0,
       0,    16,   295,    24,     0,   295,   295,   345,    15,    28,
       0,     0,   295,   398,   392,   241,   242,   243,   244,   237,
     238,   239,   240,   128,   128,   389,     0,   390,   295,   390,
     412,   413,   386,   410,     0,     0,   702,     0,   650,   668,
     649,     0,   672,     0,     0,     0,     0,    96,    95,     0,
       0,   703,     0,   704,   705,   718,   713,     0,   714,   715,
       0,     0,    12,    48,     0,     0,    87,    88,     0,     0,
       0,     0,    76,    77,    75,    74,    73,    72,    71,    66,
      67,    68,    69,    70,   104,     0,    46,     0,   105,   114,
      98,     0,     0,   468,   469,    97,     0,   302,    45,   593,
     607,   615,     0,     0,   598,   603,     0,     0,     0,   269,
     295,   416,   418,   423,   422,   424,   432,   361,   286,   287,
       0,     0,     0,     0,     0,   434,     0,   462,   487,   122,
     121,     0,   292,   346,     0,     0,    22,   291,   344,    26,
       0,   373,   487,   487,   391,   399,     0,   376,     0,     0,
     387,     0,   386,     0,     0,     0,   631,   667,   549,     0,
     700,     0,     0,     0,    93,    64,   707,   709,     0,   712,
       0,   706,   717,   719,     0,   716,   721,   720,    55,    57,
       0,     0,    81,    79,    99,   103,   572,     0,   478,   447,
     477,   487,   487,   487,   487,     0,   456,     0,   488,   442,
     451,   470,   299,   301,    89,     0,   590,   634,   601,     0,
     589,   637,     0,   128,   128,   643,   639,   636,   615,   614,
     612,   613,   596,   615,   620,   617,   128,   128,     0,   605,
     426,   542,   431,   295,   430,   288,     0,   570,   550,   233,
     234,   229,   230,   235,   236,   231,   232,   128,   128,   568,
       0,   551,   553,   567,     0,     0,     0,   435,   433,   488,
     118,   128,   128,     0,   343,   284,   287,   487,   289,   487,
     393,   394,   400,   488,   396,   402,   488,   295,   295,   414,
     411,   295,     0,     0,   646,   666,   221,   222,   223,   224,
     213,   214,   215,   216,   225,   226,   227,   228,   217,   218,
     219,   220,   128,     0,   655,   651,   653,     0,     0,   669,
     551,     0,     0,     0,     0,     0,   708,    83,    86,   471,
       0,   448,   443,   452,   449,   444,   453,   488,   445,   454,
     450,   446,   455,   457,   464,   465,   304,     0,   306,   615,
       0,   634,   586,     0,     0,     0,     0,   638,     0,     0,
     621,   621,   616,   425,   427,     0,     0,   542,   429,   548,
     565,   419,   419,   544,   545,     0,   569,     0,   436,   437,
       0,   125,     0,   126,     0,   316,   314,   313,   293,   488,
       0,   488,   295,   395,   295,     0,   372,   375,   380,   295,
     701,   648,   659,   419,   660,   656,   657,     0,   474,   632,
     463,   472,     0,   100,     0,   102,   329,    89,     0,     0,
     326,     0,   328,     0,   384,   319,   325,     0,     0,     0,
     573,   465,   476,   277,     0,     0,     0,     0,     0,     0,
     530,   607,   607,   525,   487,     0,   127,   128,   128,     0,
       0,   512,   492,   493,     0,     0,     0,   594,     0,   634,
     644,   640,   599,     0,   624,   618,   622,   619,   428,   543,
     353,   269,   295,   354,   295,   349,   350,   295,   562,   420,
     423,   269,   295,   295,   564,   295,   552,   128,   128,   554,
     571,    31,     0,     0,     0,     0,   290,     0,   487,     0,
     295,   487,     0,   295,   378,   295,   295,   665,     0,   661,
     473,   480,   699,     0,   332,    47,     0,   323,    94,     0,
     318,     0,     0,   331,   322,    84,     0,   528,   515,   516,
     517,     0,     0,     0,   531,     0,   488,   513,     0,     0,
     134,   483,   498,   485,   503,     0,   496,     0,     0,   466,
     135,   305,   591,   602,     0,     0,   626,     0,   295,   426,
     542,   560,   295,   352,   295,   356,   561,   421,   426,   542,
     563,   546,   419,   419,   123,   124,     0,    23,    27,   401,
     488,   295,     0,   404,   403,   295,     0,   407,   663,   664,
     658,   419,   101,     0,   334,     0,     0,   320,   321,     0,
       0,   526,   518,     0,   523,     0,     0,     0,   132,   335,
       0,   133,   338,     0,     0,   465,     0,     0,     0,   482,
     487,   481,   502,     0,   514,   634,   587,   642,   641,   645,
     625,     0,   357,   358,     0,   351,   355,     0,   295,   295,
     557,   295,   559,   315,     0,   406,   295,   409,   295,   662,
       0,   327,   324,     0,   524,     0,   295,   130,     0,   131,
       0,     0,     0,     0,   532,     0,   497,   465,   466,   489,
     487,     0,     0,   623,   359,   547,   555,   556,   558,   405,
     408,   333,   527,   534,     0,   529,   336,   339,     0,     0,
     486,   533,   511,   504,     0,   508,   495,   491,   490,     0,
     592,     0,     0,     0,   535,   536,   519,   487,   487,   484,
     499,   532,   510,   465,   501,     0,     0,   534,     0,     0,
     488,   488,   465,     0,   509,     0,     0,     0,   520,   537,
       0,     0,   500,   505,   538,     0,     0,     0,   337,   340,
     532,     0,   540,     0,   521,     0,     0,     0,     0,   506,
     539,   522,   541,   465,   507
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,     1,     2,     3,     5,    31,    32,    33,    34,   424,
     687,   430,   689,   272,   548,   831,   218,   336,   505,   220,
     221,   222,   223,    35,   224,   225,   491,   490,   488,   839,
     489,   226,   735,   227,   412,   413,   414,   415,   680,   609,
      36,   403,   856,   248,   249,   250,   251,   252,   253,   254,
     255,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,   667,   668,   443,   262,   256,    55,   263,    56,    57,
      58,    59,    60,   279,   105,   273,   280,   800,   106,   683,
     404,   265,    62,   384,   385,   386,    63,   798,   904,   833,
     834,   835,  1006,   836,   926,   837,   838,  1018,  1068,  1117,
    1021,  1070,  1118,   686,   282,   915,   885,   886,   283,    64,
      65,    66,    67,   447,   449,   454,   292,    68,   930,   571,
     287,   288,   445,   691,   694,   692,   695,   452,   453,   257,
     399,   531,   917,   889,   890,   534,   535,   274,   610,   611,
     612,   613,   614,   615,   416,   378,   842,  1030,  1034,   512,
     513,   514,   819,   820,   379,   617,   228,   821,   956,   957,
    1023,   958,  1025,   417,   551,  1078,  1031,  1079,  1080,   959,
    1077,  1024,  1132,  1026,  1121,  1150,  1163,  1123,  1103,   862,
     863,   945,  1104,  1113,  1114,  1115,  1153,   652,   776,   669,
     895,  1047,   670,   671,   899,   672,   781,   407,   541,   673,
     674,   607,   229,    70,    91,    71,    72,    73,   521,   869,
     392,   759,  1035,   627,   395,   638,   761,    74,   396,    94,
      75,   526,   643,   527,   648,   875,   876,    76,    77,   191,
     458,   728,   522,   523,   636,   766,  1039,   637,   765,   965,
     321,   576,   725,   726,   727,   918,   919,   460,   578,   322,
     323,   324,   325,   326,   472,   473,   587,   474,   340,   230,
     477,   478,   479,   231,   232,   233
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -1026
static const short yypact[] =
{
     107,   153,   178,  3928, -1026,  3928,   335, -1026, -1026, -1026,
   -1026, -1026,   135,   135,   135,   147, -1026,   158, -1026,   423,
   -1026, -1026,   423,   423, -1026,   187,   423,   187,   423,   423,
   -1026, -1026, -1026, -1026, -1026,   191,    83,  4067,  4194,  4110,
    4207,   457,   697,   516,  1261,  4152,  4249,  4165,  4291,   951,
    1396,  1360,  1414, -1026, -1026,   182, -1026, -1026, -1026, -1026,
   -1026,   135, -1026, -1026,   246,   362,   401, -1026, -1026,  3928,
   -1026, -1026, -1026, -1026, -1026, -1026, -1026, -1026, -1026, -1026,
   -1026,   135,   135,   135,  3452,   210, -1026, -1026, -1026, -1026,
   -1026,    57,   473,    53, -1026, -1026,    51,   258, -1026,   296,
     423,  3041, -1026,   307,   135,   389, -1026,  1483, -1026, -1026,
   -1026,   135, -1026, -1026, -1026, -1026, -1026, -1026, -1026, -1026,
     135, -1026, -1026, -1026, -1026, -1026, -1026, -1026, -1026,   135,
   -1026, -1026, -1026, -1026, -1026, -1026, -1026, -1026,   135, -1026,
   -1026, -1026, -1026, -1026, -1026, -1026, -1026,   135, -1026, -1026,
   -1026, -1026, -1026, -1026, -1026, -1026,   135, -1026, -1026, -1026,
   -1026, -1026, -1026, -1026, -1026,   135, -1026, -1026, -1026, -1026,
   -1026, -1026, -1026, -1026,   135, -1026, -1026, -1026, -1026, -1026,
     712,    83, -1026, -1026,   206,   193,   206,   198, -1026,   259,
   -1026,  4379, -1026, -1026, -1026, -1026, -1026,  3452,  3452,   287,
     294,   317, -1026,   423, -1026, -1026, -1026,  3452, -1026, -1026,
    2381,  3115,   333,   343,   380, -1026, -1026,   392,  3452,   366,
     369, -1026,  3519,  3586, -1026,  5050,   744,   374,  2021,  3452,
   -1026, -1026, -1026, -1026,   558, -1026,   423,   423,   423,   386,
     423,   423, -1026, -1026, -1026, -1026,   396,   405,  4770,  4815,
    4789,  4834,   988,   828,   991,  1047, -1026,   413,   315,   504,
     349,   506, -1026,    83,    83,   135, -1026,   135, -1026,   442,
     135,   228,  1308,   135, -1026, -1026,   307,   135, -1026,   421,
   -1026,  3742,   308,   458,   486,  1904,   488,   474,  3869, -1026,
     491, -1026,   227, -1026, -1026, -1026, -1026, -1026, -1026, -1026,
   -1026, -1026, -1026, -1026, -1026, -1026, -1026, -1026, -1026, -1026,
   -1026, -1026, -1026, -1026, -1026, -1026, -1026,   642,  4802, -1026,
   -1026, -1026, -1026,  3786,   532, -1026, -1026, -1026, -1026,  3452,
    3452,  4802, -1026, -1026,   508, -1026,   518,   524, -1026, -1026,
    4415,  4460,  4802,   423, -1026, -1026,   531,  3452,  2381, -1026,
    2381, -1026,  3452,  3452,   576, -1026, -1026,  3452,  3452,  3452,
    3452,  3452,  3452,  3452,  3452,  3452,  3452,  3452,  3452, -1026,
   -1026,   423,  3452,  3452,   423,   615,   544, -1026,   600,   556,
   -1026, -1026, -1026, -1026,    38, -1026,   594, -1026, -1026,   187,
     565, -1026,   602,   592,   604, -1026,   602, -1026, -1026,   188,
   -1026,   458,   245,    83, -1026,   676, -1026, -1026,   307,   688,
    3184,   628, -1026, -1026,  2569,    68, -1026, -1026,   670,   712,
     712, -1026,   135, -1026,  1308,   135,   135, -1026, -1026, -1026,
    1308,   641,   135, -1026, -1026,  4770,  4815,  4789,  4834,   988,
     828,   991,  1047, -1026,   354,   634,  4309,   206,   135,   206,
   -1026,   681,   640, -1026,   227,  4802, -1026,   647,   653,   731,
   -1026,   532, -1026,   861,  4909,  4935,   656, -1026, -1026,  3251,
    3452,   699,   657,  4415, -1026, -1026,   702,   672,  4460, -1026,
     690,   701, -1026,  5050,   707,   710,  5050,  5050,  3452,   725,
    3452,  3452,  1587,  1943,  1896,  1322,  1273,  1089,  1089,   134,
     134, -1026, -1026, -1026, -1026,   713,   369,   695, -1026, -1026,
   -1026,   423,  2111,   600, -1026, -1026,   716,   558,  3653,   691,
     187, -1026,   724,  3960, -1026, -1026,   339,  1111,   745,   307,
     135, -1026, -1026, -1026, -1026,   465, -1026, -1026, -1026,    88,
     736,  1622,  3452,  3452,  3318, -1026,   730, -1026, -1026, -1026,
   -1026,  4667, -1026,   308,   329,   712, -1026,   780, -1026, -1026,
     743, -1026, -1026, -1026, -1026, -1026,   742, -1026,   746,  3452,
     423,   747,   640,   754,  4496,  1578, -1026, -1026,  4524,  4802,
   -1026,  4802,  3452,  4802, -1026, -1026,   369, -1026,  3452, -1026,
     699, -1026, -1026, -1026,   702, -1026, -1026, -1026,   741,   741,
     797,  3452,  1799,  1673, -1026, -1026, -1026,   509,   628, -1026,
   -1026,    61,    76,    87,   111,   845, -1026,   763, -1026, -1026,
   -1026, -1026, -1026, -1026,   230,   773, -1026,   602, -1026,   467,
   -1026, -1026,    83, -1026, -1026, -1026,   444, -1026, -1026, -1026,
   -1026, -1026, -1026, -1026, -1026, -1026, -1026, -1026,   785, -1026,
     188,   188, -1026,   135, -1026, -1026,   787, -1026, -1026,  4542,
    4679,  1003,  1265,  4554,  4699,  1433,  1448, -1026, -1026, -1026,
     789,   514, -1026, -1026,   319,   783,   790, -1026, -1026, -1026,
   -1026,   799,   804,  2628, -1026, -1026,   863, -1026, -1026, -1026,
   -1026,   805, -1026, -1026,   807, -1026, -1026,   135,   135,  5050,
   -1026,   135,   821,   423, -1026,  3786,  4542,  4679,  4584,  4732,
    1003,  1265,  1452,  1487,  4554,  4699,  4635,  4758,  1433,  1448,
    1666,  1677,   826,   831, -1026, -1026,  4648,  2511,    74, -1026,
     830,   833,   838,  4955,   839,  1748, -1026, -1026,  2436, -1026,
     423, -1026, -1026, -1026, -1026, -1026, -1026, -1026, -1026, -1026,
   -1026, -1026, -1026, -1026, -1026,  2787, -1026,  3452, -1026, -1026,
     855,   602, -1026,   712,    83,  4379,  4025, -1026,   680,  3834,
     365,   365, -1026, -1026, -1026,   847,  4054, -1026, -1026, -1026,
   -1026,   583,   267, -1026, -1026,  1534, -1026,   930, -1026, -1026,
      74, -1026,   712, -1026,    83, -1026, -1026,  5050, -1026, -1026,
    2628, -1026,   135,   426,   135,   420, -1026, -1026, -1026,   135,
   -1026, -1026, -1026,   583, -1026, -1026, -1026,   844, -1026, -1026,
     853, -1026,   423, -1026,  3452, -1026, -1026,   894,   423,  3115,
   -1026,   897,  5050,   862,   858, -1026, -1026,   265,  2559,  3452,
   -1026,  2951, -1026,   902,  3452,   903,   867,   868,  3385,   515,
     947,    81,   199, -1026, -1026,   875, -1026, -1026, -1026,   876,
    1088, -1026, -1026, -1026,  2877,   352,   822, -1026,   898,   602,
   -1026, -1026, -1026,  3452,   922,   883, -1026,   883, -1026, -1026,
   -1026,   307,   135, -1026,   135,   501,   517,   171, -1026, -1026,
     135,   307,   135,   171, -1026,   135, -1026, -1026, -1026, -1026,
   -1026, -1026,   567,   574,  1748,    74, -1026,    74, -1026,  3452,
     102, -1026,  3452,   176, -1026,   135,   171, -1026,   581, -1026,
   -1026, -1026, -1026,  4977, -1026,  4298,  1748, -1026, -1026,  2471,
   -1026,  2697,  3452, -1026, -1026,  2436,  4873, -1026, -1026, -1026,
   -1026,   889,  3452,   890, -1026,   909, -1026, -1026,   712,    83,
   -1026, -1026, -1026, -1026, -1026,   913,   964,  2201,    78, -1026,
   -1026, -1026, -1026, -1026,   911,    89,  5050,  3452,   135,   583,
     292, -1026,   135, -1026,   135, -1026, -1026,   135,   267,   267,
   -1026, -1026,   583,   267, -1026, -1026,   904, -1026, -1026, -1026,
   -1026,  5014,  3452, -1026, -1026,  5014,  3452, -1026, -1026, -1026,
   -1026,   583, -1026,  3452, -1026,   907,  2697, -1026, -1026,  4298,
    3452, -1026, -1026,   915, -1026,  3452,   967,   582, -1026,   591,
     596, -1026,   734,   948,   953, -1026,   954,  3452,  2291, -1026,
   -1026, -1026, -1026,  3452, -1026,   602, -1026, -1026, -1026, -1026,
    5050,   365,   501,   517,   347, -1026, -1026,  4054,   135,   171,
   -1026,   171, -1026, -1026,   426, -1026,  5014, -1026,  5014, -1026,
    4891, -1026, -1026,  5032, -1026,    84,   135, -1026,  1308, -1026,
    1308,  3452,  3452,  1005,  2877,   941, -1026, -1026, -1026, -1026,
   -1026,   944,   957, -1026, -1026, -1026, -1026, -1026, -1026, -1026,
   -1026, -1026, -1026,    93,   946, -1026, -1026, -1026,   949,   972,
   -1026, -1026, -1026, -1026,   950, -1026, -1026, -1026, -1026,   973,
   -1026,   965,   423,   129,   968, -1026, -1026, -1026, -1026, -1026,
   -1026,  3452, -1026, -1026, -1026,  3452,   977,    93,   974,    93,
   -1026, -1026, -1026,   984, -1026,   989,  1062,   179, -1026, -1026,
     844,   844, -1026, -1026, -1026,  1007,  1065,   992, -1026, -1026,
    3452,  3452, -1026,   372, -1026,   994,   998,  1000,  1076, -1026,
   -1026, -1026, -1026, -1026, -1026
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
   -1026, -1026, -1026, -1026, -1026,   108, -1026,  -487, -1026, -1026,
   -1026, -1026, -1026, -1026, -1026,   -19, -1026,    95,   571,  -351,
     312, -1026, -1026, -1026,  -158,   935, -1026, -1026, -1026, -1026,
   -1026, -1026, -1026, -1026,  -410, -1026,   675, -1026, -1026,  -128,
     345,  -414,  -770,    29,    49,    82,   103,    90,   121,   160,
     166,  -437,   186,  -540,  -499,   192,   212,  -486,  -478,  -445,
    -384,   540,   541,  -498,  -230, -1026,  -682,  -238,   815,  1167,
    1180,  1300, -1026,  -728,  -176,  -275,   539, -1026,   692, -1026,
     477,    14,   122, -1026,   580, -1026,  1001,   298, -1026,  -803,
   -1026,   172, -1026,  -794, -1026, -1026,   263, -1026, -1026, -1026,
   -1026, -1026, -1026,  -156,   278,  -725,   136,  -262,   357, -1026,
   -1026, -1026, -1026, -1026, -1026, -1026, -1026, -1026, -1026,   542,
    -119, -1026,   667, -1026, -1026,   208,   209,   673,   559,   -66,
   -1026, -1026,  -690,  -397,  -278,  -628, -1026,   101, -1026, -1026,
   -1026, -1026, -1026, -1026,  -383, -1026, -1026,  -681,    55, -1026,
   -1026,   621,  -718, -1026,   316, -1026, -1026,  -707, -1026, -1026,
   -1026, -1026, -1026,   569,  -251,    72,  -975,  -413,  -403, -1026,
   -1026, -1026, -1026, -1026, -1026, -1026, -1026, -1026, -1026, -1026,
   -1026, -1026, -1025,     8, -1026,    12, -1026,   500, -1026,  -753,
   -1026, -1026,   577,   579, -1026, -1026,   490,  -415, -1026, -1026,
   -1026, -1026,    13, -1026,   290, -1026, -1026, -1026, -1026, -1026,
   -1026, -1026, -1026, -1026, -1026, -1026, -1026, -1026, -1026,   -26,
     -12,  -484, -1026,   511, -1026,   388,   119,  -455, -1026, -1026,
   -1026, -1026,  -366, -1026, -1026, -1026, -1026,   525, -1026, -1026,
     398, -1026, -1026, -1026,   445, -1026,   175,   468, -1026,   598,
     605,  -298, -1026,  -311, -1026, -1026,   597,   711, -1026, -1026,
   -1026, -1026,   708, -1026, -1026, -1026
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -655
static const short yytable[] =
{
      90,    98,   532,    92,    93,   284,   425,    96,   555,    90,
     100,   558,   462,    95,   556,    95,    69,    61,    69,    61,
     559,   506,   774,   879,   281,   461,    81,    82,    83,   646,
     528,   549,    37,   410,    37,   716,   631,   629,   419,   327,
     328,   409,   471,   476,   934,   185,   187,   189,   861,   333,
    1073,   111,    38,   120,    38,   129,   884,   138,   444,   147,
     345,   156,  -458,   165,   902,   174,   239,   290,   635,  -117,
     243,   380,   901,   859,   860,   817,   717,  -459,   633,  1032,
      95,   246,    69,    61,    95,    39,   102,    39,  -460,   720,
    1037,   888,   894,    41,   960,    41,  1133,   721,    37,   619,
     240,   986,  1106,  1111,   663,    19,    40,    -2,    40,   620,
     920,   235,  -461,    78,   663,    61,   269,    61,    38,   586,
     241,   533,   516,  1005,    42,  1155,    42,   517,   -36,   616,
     722,  1093,   258,    17,   861,    19,  -581,  1008,   714,   634,
    -581,   663,   103,   242,   337,  1033,   236,  -438,  1134,   992,
     104,    39,   259,     4,   768,   270,   271,  1142,  -117,    41,
     860,  1112,  -439,    43,   818,    43,    17,   506,  1094,    44,
     542,    44,    40,  -440,  1038,   590,  1127,   190,    -3,   219,
     594,   635,   859,   183,   332,   260,   716,   987,  1164,   988,
      42,   723,   724,   366,   367,   368,   247,  -441,   742,   745,
     748,   751,    17,   183,   183,   183,   261,    17,   743,   746,
     749,   752,  1062,  1128,    84,   383,   444,   388,   389,   390,
    1017,   393,   394,   996,    61,    85,  1146,   717,   450,    43,
      86,    87,   753,   183,   408,    44,    11,   586,   270,   271,
     720,    19,   183,   270,   271,   716,   -35,   529,   721,    86,
      87,   183,   457,    19,   775,   530,   271,  1048,   101,    17,
     183,   760,   111,  1147,   120,   466,   129,   182,   138,   183,
     102,   646,   111,   451,   120,   866,   480,   234,   183,   871,
     286,   722,   484,   289,   485,    61,   717,   183,   291,   714,
      61,   650,  1050,  1052,  1085,   102,   183,   647,   456,   720,
     258,   446,    61,    88,  1102,   258,   339,   721,    89,   931,
     857,   585,   270,   271,   756,    11,    97,   435,    99,   757,
     259,   633,    88,   108,   481,   259,   891,    89,   566,   537,
     568,   828,    61,   932,   892,   271,   184,   436,    17,   663,
     897,   774,   723,   244,   816,    61,    17,   236,   714,   293,
     774,   881,   504,   260,   329,   508,    61,   117,   260,   882,
     271,   330,    61,   519,    61,    86,    87,   755,   102,   275,
     437,   858,   773,   533,   261,   426,   271,    95,   439,   261,
      17,   245,   634,   427,   331,   236,   333,   183,   908,   573,
     911,   438,   859,   107,   462,   868,   426,   271,   180,   181,
     341,   898,   344,   786,    86,    87,   865,   461,   787,   440,
     342,   425,   873,   684,   974,   271,   639,   640,   641,   857,
      79,    80,    61,   102,   276,   642,    86,    87,   790,   102,
     275,  1084,   277,  1148,  1149,   451,   961,   258,    88,  -397,
    -397,   347,   803,    89,   580,   805,   278,   343,   441,   111,
     346,   120,   186,   129,   442,   138,  1157,   259,   347,   446,
      61,  1158,     8,     9,    10,   126,   375,   912,   507,    61,
      12,    13,    14,   909,   266,   435,   391,    88,   267,   103,
     858,   397,    89,   741,   744,   276,   750,   104,    17,   398,
     260,   188,   606,   277,   628,   436,   841,   400,   383,    88,
     536,    20,    21,   964,    89,   546,   421,   278,    95,   405,
     422,   261,   113,   731,   122,   732,  -635,   734,    86,    87,
     237,     8,     9,    10,   135,   270,   271,    19,   437,    12,
      13,    14,   653,   271,   349,   351,   439,    61,   285,   647,
     238,    61,  -257,    61,   639,   640,   641,    17,   905,   438,
     907,   451,    37,   762,  1041,   431,   435,  1045,   258,  1046,
     432,    86,    87,     8,     9,   381,   382,   440,   972,   271,
     659,   428,    38,   775,   942,   267,   436,   448,   259,   463,
     659,   264,   775,   600,   974,   271,   102,   880,   284,    61,
     660,    88,   467,    61,   739,    61,    89,    61,   740,   784,
     660,  -259,   468,   785,   706,    39,   441,   659,   469,   437,
     663,   260,   442,    41,   874,   874,   482,   439,   903,   269,
     401,   402,  -287,   -85,   707,   509,    40,   660,   510,   857,
     438,   661,   261,   511,    88,  -287,   654,   675,   676,    89,
     515,   661,   881,   969,    42,    86,    87,   910,   440,   520,
     882,   271,   984,   978,   427,   427,   422,   708,  1096,   985,
    1097,   518,   662,   267,   883,   710,  1000,  1067,   661,  1082,
    1001,   422,   662,   111,  -633,   129,  -287,   147,   709,   165,
    -287,  1069,   524,    43,   810,   267,   540,   441,   525,    44,
     858,   773,   533,   442,   543,  1016,   711,   553,   554,   662,
     773,   533,     8,     9,    10,   131,   380,  1043,  1044,   455,
      12,    13,    14,   547,   552,   102,   275,   560,    88,   564,
     111,   840,   120,    89,   129,   569,   138,   664,   147,   570,
     156,   574,   165,   665,   174,   712,   854,   664,   575,  1054,
      61,   713,   577,   665,   425,   583,   588,   406,   589,   592,
     418,   536,   536,   666,   420,   706,   593,   639,   640,   641,
     539,   715,   269,   666,   664,  -287,   872,   718,   864,    61,
     665,   276,   601,  1020,   596,   707,   401,   402,  -287,   277,
      61,   626,  -258,    61,    37,   597,   605,   719,   562,   563,
     666,   598,  1019,   278,   599,    37,   630,   604,   435,    61,
     622,   270,   271,   922,    38,   659,   369,   370,   708,   924,
     371,   372,   373,   374,   706,    38,   710,   649,   436,  -287,
     655,   678,   854,  -287,   688,   660,    98,   690,   697,   709,
     943,   584,   698,   701,   707,    10,   131,    39,   703,    95,
      95,    12,    13,    14,   737,    41,   411,   711,    39,   754,
     855,   437,   109,   114,   118,   123,    41,   758,    40,   439,
     145,   150,   154,   159,    86,    87,   661,   708,   632,    40,
     772,   779,   438,   783,   788,   710,    42,   864,    61,  1140,
    1141,   789,   536,   536,   791,   874,   712,    42,   709,   793,
     440,   269,   713,    37,   802,    61,   804,   662,   910,   639,
     640,   641,   557,   406,   977,    61,   711,   809,   962,   561,
     258,   812,   715,    38,   536,    43,   814,   822,   718,   785,
     258,    44,   823,   825,   339,   567,    43,   867,   579,   441,
     259,   878,    44,   900,   818,   442,   855,    88,   719,   377,
     259,   -33,    89,   941,   927,   712,    39,   929,   928,   -34,
     937,   713,   938,   939,    41,   944,     8,     9,    10,   162,
     947,   950,   664,   260,    12,    13,    14,    40,   665,   967,
     963,   715,   968,   260,  1012,  1014,  1015,   718,   763,   764,
    1027,  1028,    17,  1036,   261,    42,   973,   975,   666,   539,
    1053,   770,   771,  1061,   261,    10,   126,   719,    10,   135,
    1064,    12,    13,    14,    12,    13,    14,   651,     8,     9,
      10,   126,   780,   780,  1066,  1071,    12,    13,    14,    17,
    1072,  1074,    17,  1100,    43,  1105,   792,   794,  1109,  1110,
      44,  1116,  1125,  1119,    17,  1122,  -265,  1013,   112,   116,
     121,   125,   130,   134,   139,   143,   148,   152,   157,   161,
     166,   170,   175,   179,    10,   140,  1120,  1129,  1124,  1138,
      12,    13,    14,   109,   114,   118,   123,   813,  1136,  1143,
     536,   536,  1145,  1144,  1151,  1152,   659,  1154,  1159,   536,
     536,   618,  1160,   536,   536,  1161,  1162,   864,    61,   625,
     550,   681,   682,  1126,   685,   538,   660,   623,   906,   183,
     933,  1007,   536,    37,   951,  1042,   952,   953,   954,   955,
    1065,   632,   644,   565,   702,     7,   989,   679,    10,    11,
     994,   539,  1075,    38,    12,    13,    14,   572,  1081,   700,
     777,   693,   696,  1107,   621,  1137,   921,   661,   887,   893,
      16,  1139,    17,   973,   975,   975,   364,   365,   366,   367,
     368,   539,  1108,   778,   769,   729,    39,   730,   782,   877,
    1083,   767,   913,   870,    41,    19,  1098,  1099,   662,  1101,
     916,   815,   704,   811,   806,   807,  1059,    40,   808,   705,
     618,   618,   747,   618,   591,   736,   595,    25,  -611,  -611,
    -611,     0,    27,     0,     0,    42,   645,  -611,     0,     0,
       0,     0,   948,   949,   110,   115,   119,   124,     0,     0,
       0,     0,   146,   151,   155,   160,  1101,     0,     0,     0,
    1135,   127,   132,   136,   141,     0,     0,     0,     0,   163,
     168,   172,   177,   664,    43,   387,     0,     0,     0,   665,
      44,     0,   982,   983,     0,  1101,  1156,     0,     0,     0,
     109,   114,   118,   123,     0,     0,   799,     0,   801,   666,
       0,     0,     0,     0,   464,   465,     8,     9,    10,   140,
       8,     9,    10,   131,    12,    13,    14,     0,    12,    13,
      14,     0,   483,     0,     0,     0,   914,   486,   487,     0,
       0,     0,   492,   493,   494,   495,   496,   497,   498,   499,
     500,   501,   502,   503,     0,     0,  1022,     0,     0,   411,
       0,     0,  -487,  -487,  -487,  -487,  -487,     0,     0,     0,
       0,  -487,  -487,  -487,     0,     0,   401,   402,   362,   363,
     364,   365,   366,   367,   368,   401,   402,  -487,     0,  1049,
    1051,   128,   133,   137,   142,     0,  -260,     0,     0,   164,
     169,   173,   178,     0,     0,     0,     0,     0,   916,   970,
       0,   971,  -487,     0,   976,     8,     9,    10,   171,   979,
     980,     0,   981,    12,    13,    14,   361,   362,   363,   364,
     365,   366,   367,   368,  -487,     0,     0,   993,     0,  -487,
     997,    17,   998,   999,     0,     0,     0,     0,  -116,     0,
       0,     8,     9,    10,   167,     0,     0,     0,     0,    12,
      13,    14,     0,     0,     0,   110,   115,   119,   124,     8,
       9,    10,   176,   946,     0,   602,   603,    12,    13,    14,
       0,     0,   127,   132,   136,   141,     0,     0,     8,     9,
      10,   162,     0,     0,     0,  -267,    12,    13,    14,   406,
       0,   406,     0,     8,     9,    10,   167,     8,     9,    10,
     135,    12,    13,    14,    17,    12,    13,    14,  1055,     0,
       0,     0,  1057,     0,   109,   114,     0,   990,   145,   150,
     696,  -266,     0,    17,   268,     0,     0,   -29,   -29,   -29,
     -29,   -29,     8,     9,    10,   140,   -29,   -29,   -29,  -268,
      12,    13,    14,     0,   699,     0,     0,     0,     0,     0,
       0,   269,   -29,     0,  -287,     0,     0,   733,   387,     0,
       0,   109,   114,   118,   123,  1086,  1087,  -287,  1088,   145,
     150,   154,   159,  1089,     0,  1090,   738,   -29,     7,     8,
       9,    10,    11,  1095,     0,   896,     0,    12,    13,    14,
     270,   271,   128,   133,   137,   142,     0,     0,     0,   -29,
       0,     0,     0,    16,   -29,    17,     0,     0,  -287,     0,
       0,     0,  -287,   -29,     0,     0,     0,     0,     0,   411,
       0,     0,     7,     8,     9,    10,    11,     0,    19,     0,
       0,    12,    13,    14,     0,     0,     0,     0,     0,   747,
       0,     0,   110,   115,   119,   124,     0,    16,     0,    17,
      25,     0,     0,     0,     0,    27,     0,     0,   797,   127,
     132,   136,   141,   656,     0,   657,     7,     8,     9,    10,
      11,     0,    19,   658,     0,    12,    13,    14,   358,   359,
     360,   361,   362,   363,   364,   365,   366,   367,   368,   747,
       0,    16,     0,     0,    25,     0,     0,     0,     0,    27,
     112,   116,   130,   134,   148,   152,   166,   170,  -652,     0,
     832,     8,     9,    10,   171,     0,    19,     0,     0,    12,
      13,    14,     8,     9,    10,   176,  1130,  1131,     0,     0,
      12,    13,    14,     0,     0,     0,     0,    17,    25,     0,
       0,     0,     0,    27,     0,     0,  -549,   112,   116,   121,
     125,   130,   134,   139,   143,   148,   152,   157,   161,   166,
     170,   175,   179,   357,   358,   359,   360,   361,   362,   363,
     364,   365,   366,   367,   368,   797,     0,     0,     0,   128,
     133,   137,   142,     0,     0,     0,     0,     0,     0,   826,
       0,   827,    87,     0,     0,     0,     0,   193,   194,   923,
     195,     0,     0,     0,   925,     0,     0,     0,     0,     0,
       0,     0,     0,   832,   935,     0,     0,     0,   196,   936,
      18,     0,   197,   198,   199,   200,   201,     0,     0,     0,
       0,   202,     0,     0,     0,     0,     0,   203,     0,     0,
     204,     0,     0,     0,     0,   205,   206,   207,   966,     0,
     208,   209,     0,     0,   828,   210,   829,     0,     0,     0,
       0,   212,     0,   213,    88,     0,   110,   115,   214,    89,
     146,   151,     0,     0,  -317,   215,   216,     0,   830,   832,
     217,   127,   132,     0,   991,   163,   168,   995,   356,   357,
     358,   359,   360,   361,   362,   363,   364,   365,   366,   367,
     368,   832,     0,     0,   832,     0,   832,  1009,     0,     0,
       0,     0,     0,   110,   115,   119,   124,     0,     0,     0,
       0,   146,   151,   155,   160,     0,     0,     0,     0,     0,
     127,   132,   136,   141,     0,     0,     0,     0,   163,   168,
     172,   177,  1040,     0,     0,   429,     0,     0,   -25,   -25,
     -25,   -25,   -25,     0,     0,     0,     0,   -25,   -25,   -25,
       0,     0,     0,     0,     0,     0,     0,  1056,     0,     0,
       0,  1058,   269,   -25,     0,  -287,     0,     0,  1060,     0,
       0,   832,     0,     0,     0,  1063,     0,     0,  -287,   360,
     361,   362,   363,   364,   365,   366,   367,   368,   -25,     0,
       0,   128,   133,     0,     0,   164,   169,     0,     0,     0,
       0,   270,   271,     0,     0,     0,     0,     0,     0,     0,
     -25,     0,     0,     0,     0,   -25,     0,     0,     0,  -287,
       0,     0,     0,  -287,   -25,   359,   360,   361,   362,   363,
     364,   365,   366,   367,   368,     0,     0,     0,     0,     0,
     128,   133,   137,   142,     0,     0,     0,     0,   164,   169,
     173,   178,   376,     0,  -463,  -463,  -463,  -463,  -463,  -463,
    -463,  -463,     0,  -463,  -463,  -463,  -463,  -463,     0,  -463,
    -463,  -463,  -463,  -463,  -463,  -463,  -463,  -463,  -463,  -463,
    -463,  -463,  -463,  -463,  -463,  -463,  -463,  -463,  -463,  -463,
       0,     0,     0,     0,  -463,     0,     0,     0,     0,     0,
    -463,     0,     0,  -463,     0,  -463,     0,     0,  -463,  -463,
    -463,     0,     0,  -463,  -463,     0,     0,     0,  -463,  -463,
       0,     0,     0,     0,  -463,     0,  -463,  -463,     0,     0,
       0,  -463,  -463,     0,     0,     0,  -463,   377,  -463,  -463,
       0,  -463,   608,  -463,  -487,  -487,  -487,  -487,  -487,  -487,
    -487,  -487,     0,  -487,  -487,  -487,  -487,  -487,     0,  -487,
    -487,  -487,  -487,  -487,  -487,  -487,  -487,  -487,  -487,  -487,
    -487,  -487,  -487,  -487,     0,  -487,  -487,  -487,  -487,  -487,
       0,     0,     0,     0,  -487,     0,     0,     0,     0,     0,
    -487,     0,     0,  -487,     0,  -487,     0,     0,  -487,  -487,
    -487,     0,     0,  -487,  -487,     0,     0,     0,  -487,  -487,
       0,     0,     0,     0,  -487,     0,  -487,  -487,     0,     0,
       0,  -487,  -487,     0,     0,     0,  -487,     0,  -487,  -487,
       0,  -487,  1029,  -487,  -465,  -465,     0,     0,     0,     0,
    -465,  -465,     0,  -465,     0,     0,     0,  -465,     0,  -465,
    -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,
       0,  -465,     0,  -465,     0,  -465,  -465,  -465,  -465,  -465,
       0,     0,     0,     0,  -465,     0,     0,     0,     0,     0,
    -465,     0,     0,  -465,     0,     0,     0,     0,  -465,  -465,
    -465,     0,     0,  -465,  -465,     0,     0,     0,  -465,  -465,
       0,     0,     0,     0,  -465,     0,  -465,  -465,     0,     0,
       0,  -465,  -465,     0,     0,     0,  -465,     0,  -465,  -465,
       0,  -465,  1076,  -465,  -494,  -494,     0,     0,     0,     0,
    -494,  -494,     0,  -494,     0,     0,     0,  -494,     0,  -494,
    -494,  -494,  -494,  -494,  -494,  -494,  -494,  -494,  -494,  -494,
       0,  -494,     0,  -494,     0,  -494,  -494,  -494,  -494,  -494,
       0,     0,     0,     0,  -494,     0,     0,     0,     0,     0,
    -494,     0,     0,  -494,     0,     0,     0,     0,  -494,  -494,
    -494,     0,     0,  -494,  -494,     0,     0,     0,  -494,  -494,
       0,     0,     0,     0,  -494,     0,  -494,  -494,     0,     0,
       0,  -494,  -494,     0,     0,     0,  -494,     0,  -494,  -494,
       0,  -494,   334,  -494,   192,     7,     0,     0,    10,    11,
     193,   194,     0,   195,    12,    13,    14,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      16,   196,    17,    18,     0,   197,   198,   199,   200,   201,
       0,     0,     0,     0,   202,     0,     0,     0,     0,     0,
     203,     0,     0,   204,     0,    19,     0,     0,   205,   206,
     207,     0,     0,   208,   209,     0,     0,     0,   210,   211,
       0,     0,     0,     0,   212,     0,   213,    25,     0,     0,
       0,   214,    27,     0,     0,     0,     0,     0,   215,   216,
       0,   335,   826,   217,   827,    87,     0,     0,     0,     0,
     193,   194,   354,   195,   355,   356,   357,   358,   359,   360,
     361,   362,   363,   364,   365,   366,   367,   368,     0,     0,
       0,   196,     0,    18,     0,   197,   198,   199,   200,   201,
       0,     0,   411,     0,   202,  -654,  -654,  -654,  -654,  -654,
     203,     0,     0,   204,  -654,  -654,  -654,     0,   205,   206,
     207,     0,     0,   208,   209,     0,     0,   828,   210,   829,
    -654,     0,  -654,     0,   212,     0,   213,    88,     0,     0,
       0,   214,    89,     0,     0,     0,     0,  -385,   215,   216,
     826,   830,   192,   217,     0,  -654,     0,     0,   193,   194,
     411,   195,     0,  -119,  -119,  -119,  -119,  -119,     0,     0,
       0,     0,  -119,  -119,  -119,     0,     0,  -654,     0,   196,
       0,    18,  -654,   197,   198,   199,   200,   201,  -119,     0,
       0,  -654,   202,  -330,     0,     0,     0,     0,   203,     0,
       0,   204,     0,     0,     0,     0,   205,   206,   207,     0,
       0,   208,   209,  -119,     0,  -330,   210,   211,     0,   795,
       0,   192,   212,     0,   213,     0,     0,   193,   194,   214,
     195,     0,     0,     0,     0,  -119,   215,   216,     0,   830,
    -119,   217,     0,     0,     0,     0,     0,     0,   196,  -119,
      18,     0,   197,   198,   199,   200,   201,     0,     0,     0,
       0,   202,     0,     0,     0,     0,     0,   203,     0,     0,
     204,     0,     0,     0,     0,   205,   206,   207,     0,     0,
     208,   209,     0,     0,     0,   210,   211,     0,   826,     0,
     192,   212,     0,   213,     0,     0,   193,   194,   214,   195,
       0,     0,     0,     0,     0,   215,   216,     0,   796,     0,
     217,     0,     0,     0,     0,     0,     0,   196,     0,    18,
       0,   197,   198,   199,   200,   201,     0,     0,     0,     0,
     202,     0,     0,     0,     0,     0,   203,     0,     0,   204,
       0,     0,     0,     0,   205,   206,   207,     0,     0,   208,
     209,     0,     0,     0,   210,   211,     0,     0,     0,     0,
     212,     0,   213,     0,     0,     0,     0,   214,     0,     0,
       0,     0,     0,     0,   215,   216,     0,   830,     0,   217,
     827,   843,     8,     9,    10,    11,   193,   194,     0,   195,
      12,    13,    14,     0,     0,     0,     0,     0,     0,   844,
     845,   846,   847,   848,   849,   850,    16,   196,    17,    18,
       0,   197,   198,   199,   200,   201,     0,     0,     0,     0,
     202,     0,     0,     0,     0,     0,   203,     0,     0,   204,
       0,    19,     0,     0,   205,   206,   207,     0,     0,   208,
     209,     0,     0,     0,   210,   211,     0,     0,     0,     0,
     212,     0,   213,   851,     0,     0,     0,   214,   852,     0,
       0,     0,   853,     0,   215,   216,     0,   818,     0,   217,
     192,     7,     8,     9,    10,    11,   193,   194,     0,   195,
      12,    13,    14,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    16,   196,    17,    18,
       0,   197,   198,   199,   200,   201,     0,     0,     0,     0,
     202,     0,     0,     0,     0,     0,   203,     0,     0,   204,
       0,    19,     0,     0,   205,   206,   207,     0,     0,   208,
     209,     0,     0,     0,   210,   211,     0,     0,     0,     0,
     212,     0,   213,    25,   827,    87,     0,   214,    27,     0,
     193,   194,     0,   195,   215,   216,     0,     0,     0,   217,
       0,     0,     0,   844,   845,   846,   847,   848,   849,   850,
       0,   196,     0,    18,     0,   197,   198,   199,   200,   201,
       0,     0,     0,     0,   202,     0,     0,     0,     0,     0,
     203,     0,     0,   204,     0,     0,     0,     0,   205,   206,
     207,     0,     0,   208,   209,     0,     0,     0,   210,   211,
       0,     0,     0,     0,   212,     0,   213,    88,     0,     0,
       0,   214,    89,     0,     0,     0,   853,     0,   215,   216,
       0,   818,     0,   217,   192,     7,     0,     0,    10,    11,
     193,   194,     0,   195,    12,    13,    14,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      16,   196,    17,    18,     0,   197,   198,   199,   200,   201,
       0,     0,     0,     0,   202,     0,     0,     0,     0,     0,
     203,     0,     0,   204,     0,    19,     0,     0,   205,   206,
     207,     0,     0,   208,   209,     0,     0,     0,   210,   211,
       0,     0,     0,     0,   212,     0,   213,    25,   192,     0,
       0,   214,    27,     0,   193,   194,     0,   195,   215,   216,
       0,     0,     0,   217,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   196,     0,    18,     0,   197,
     198,   199,   200,   201,     0,     0,     0,     0,   202,     0,
       0,     0,     0,     0,   203,     0,     0,   204,     0,     0,
       0,     0,   205,   206,   207,     0,     0,   208,   209,     0,
       0,     0,   210,   211,     0,     0,     0,   192,   212,     0,
     213,   338,     0,   193,   194,   214,   195,     0,     0,     0,
       0,     0,   215,   216,     0,     0,     0,   217,     0,     0,
       0,     0,     0,     0,   196,     0,    18,     0,   197,   198,
     199,   200,   201,     0,     0,     0,     0,   202,     0,     0,
       0,     0,     0,   203,     0,     0,   204,     0,     0,     0,
       0,   205,   206,   544,     0,     0,   208,   209,     0,     0,
       0,   210,   211,     0,   192,     0,     0,   212,     0,   213,
     193,   194,     0,   195,   214,     0,     0,     0,     0,     0,
       0,   215,   216,     0,     0,   545,   217,     0,     0,     0,
       0,   196,     0,    18,     0,   197,   198,   199,   200,   201,
       0,     0,     0,     0,   202,     0,     0,     0,     0,     0,
     203,     0,     0,   204,     0,     0,     0,     0,   205,   206,
     207,     0,     0,   208,   209,     0,     0,     0,   210,   211,
       0,   192,     0,     0,   212,     0,   213,   193,   194,     0,
     195,   214,     0,     0,     0,     0,     0,     0,   215,   216,
       0,   584,     0,   217,     0,     0,     0,     0,   196,     0,
      18,     0,   197,   198,   199,   200,   201,     0,     0,     0,
       0,   202,     0,     0,     0,     0,     0,   203,     0,     0,
     204,     0,     0,     0,     0,   205,   206,   207,     0,     0,
     208,   209,     0,     0,     0,   210,   211,     0,   192,     0,
       0,   212,     0,   213,   193,   194,     0,   195,   214,     0,
       0,     0,     0,     0,     0,   215,   216,     0,     0,   677,
     217,     0,     0,     0,     0,   196,     0,    18,     0,   197,
     198,   199,   200,   201,     0,     0,     0,     0,   202,     0,
       0,     0,     0,     0,   203,     0,     0,   204,     0,     0,
       0,     0,   205,   206,   207,     0,     0,   208,   209,     0,
       0,     0,   210,   211,     0,   192,     0,     0,   212,     0,
     213,   193,   194,     0,   195,   214,     0,     0,     0,     0,
     940,     0,   215,   216,     0,     0,     0,   217,     0,     0,
       0,     0,   196,     0,    18,     0,   197,   198,   199,   200,
     201,     0,     0,     0,     0,   202,     0,     0,     0,     0,
       0,   203,     0,     0,   204,     0,     0,     0,     0,   205,
     206,   207,     0,     0,   208,   209,     0,     0,     0,   210,
     211,     0,   192,     0,     0,   212,     0,   213,   193,   194,
       0,   195,   214,     0,     0,     0,     0,     0,     0,   215,
     216,     0,     0,     0,   217,     0,     0,     0,     0,   196,
       0,    18,     0,   197,   198,   199,   200,   201,     0,     0,
       0,     0,   202,     0,     0,     0,     0,     0,   203,     0,
       0,   204,     0,     0,     0,     0,   205,   206,   207,     0,
       0,   208,   209,     0,     0,     0,   348,   211,     0,   192,
       0,     0,   212,     0,   213,   193,   194,     0,   195,   214,
       0,     0,     0,     0,     0,     0,   215,   216,     0,     0,
       0,   217,     0,     0,     0,     0,   196,     0,    18,     0,
     197,   198,   199,   200,   201,     0,     0,     0,     0,   202,
       0,     0,     0,     0,     0,   203,     0,     0,   204,     0,
       0,     0,     0,   205,   206,   207,     0,     0,   208,   209,
       0,     0,     0,   350,   211,     0,   624,     0,     0,   212,
       0,   213,   193,   194,     0,   195,   214,     0,     0,     0,
       0,     0,     0,   215,   216,     0,     0,     0,   217,     0,
       0,     0,     0,   196,     0,    18,     0,   197,   198,   199,
     200,   201,     0,     0,     0,     0,   202,     0,     0,     0,
       0,     0,   203,     0,     0,   204,     0,     0,     0,     0,
     205,   206,   207,     0,     0,   208,   209,     0,     0,     0,
     210,   211,     0,     0,     0,     0,   212,     0,   213,     0,
       0,     0,     0,   214,     0,     0,     0,     0,     0,     0,
     215,   216,     0,   423,     0,   217,   -21,   -21,   -21,   -21,
     -21,     0,     0,     0,     0,   -21,   -21,   -21,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     269,   -21,     0,  -287,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  -287,     0,     0,   294,
     295,     0,     0,   296,   297,     0,   -21,     0,   298,   299,
     300,   301,   302,   303,   304,   305,   306,   307,   308,   309,
     310,   311,   312,   313,   314,   315,   316,     0,   -21,     0,
       0,     0,     0,   -21,     0,     0,     0,  -287,     0,     0,
       0,  -287,   -21,   317,     0,   644,     0,     0,     7,     0,
       0,    10,    11,     0,     0,     0,     0,    12,    13,    14,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   319,    16,     0,    17,     0,   320,     0,     0,
     433,     0,     0,     7,     0,   459,    10,    11,     0,     0,
       0,     0,    12,    13,    14,     0,     0,     0,    19,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    16,     0,
      17,    18,     0,     0,     0,     0,     0,     0,     0,     0,
      25,  -610,  -610,  -610,     0,    27,     0,     0,     0,   645,
    -610,     0,     0,    19,     0,     0,     0,     0,     0,     6,
       0,  -128,     7,     8,     9,    10,    11,     0,     0,     0,
       0,    12,    13,    14,     0,    25,     0,     0,     0,     0,
      27,     0,     0,     0,   434,  -388,    15,    16,     0,    17,
      18,     6,     0,  -128,     7,     8,     9,    10,    11,     0,
       0,     0,     0,    12,    13,    14,     0,     0,     0,     0,
       0,     0,    19,     0,     0,    20,    21,  -128,     0,    16,
       0,    17,     0,     0,     0,  -128,     0,     0,    22,    23,
      24,     0,     0,     0,    25,     0,     0,     0,    26,    27,
      28,    29,     0,    30,    19,     0,     0,    20,    21,  -128,
       0,     0,     0,     0,     0,     0,     6,  -128,  -128,     7,
       8,     9,    10,    11,     0,     0,    25,     0,    12,    13,
      14,    27,     0,     0,     0,    30,     0,     0,     0,     0,
       0,     0,     0,     0,    16,   656,    17,     0,     7,     8,
       9,    10,    11,     0,     0,   658,     0,    12,    13,    14,
       0,     7,     8,     9,    10,   108,     0,     0,     0,    19,
      12,    13,    14,    16,  -128,     0,     0,     0,     0,     0,
       0,     0,  -128,     0,     0,     0,    16,     0,    17,     0,
       0,    25,     0,     0,     0,     0,    27,     0,    19,     0,
      30,     0,     0,     0,     7,     8,     9,    10,   117,     0,
       0,    19,     0,    12,    13,    14,     0,     0,     0,     0,
      25,     0,     0,     0,     0,    27,     0,     0,  -549,    16,
       0,    17,     0,    25,     0,     0,     0,     0,    27,     0,
       0,     0,  -253,     0,     0,     0,     7,     8,     9,    10,
     144,     0,     0,     0,    19,    12,    13,    14,     0,     7,
       8,     9,    10,   153,     0,     0,     0,     0,    12,    13,
      14,    16,     0,    17,     0,     0,    25,     0,     0,     0,
       0,    27,     0,     0,    16,  -255,    17,     0,     7,     8,
       9,    10,   113,     0,     0,     0,    19,    12,    13,    14,
       0,     7,     8,     9,    10,   122,     0,     0,     0,    19,
      12,    13,    14,    16,     0,     0,     0,     0,    25,     0,
       0,     0,     0,    27,     0,     0,    16,  -261,     0,     0,
       0,    25,     0,     0,     0,     0,    27,     0,    19,     0,
    -263,     0,     0,     7,     8,     9,    10,   149,     0,     0,
       0,    19,    12,    13,    14,     0,     0,     0,     0,     0,
      25,     0,     0,     0,     0,    27,     0,     0,    16,  -254,
       0,     0,     0,    25,     0,     0,     0,     0,    27,     0,
       0,     0,  -256,     0,     0,     7,     8,     9,    10,   158,
       0,     0,     0,    19,    12,    13,    14,     0,     0,  1003,
     433,     0,     0,     7,     0,     0,    10,    11,     0,     0,
      16,     0,    12,    13,    14,    25,     0,     0,     0,     0,
      27,     0,     0,     0,  -262,     0,     0,     0,    16,     0,
      17,    18,   352,   353,   354,    19,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   368,
       0,     0,     0,    19,     0,     0,     0,    25,     0,     0,
       0,     0,    27,     0,     0,     0,  -264,     0,     0,     0,
       0,     0,   294,   295,     0,    25,   296,   297,     0,  1004,
      27,   298,   299,   300,   301,   302,   303,   304,   305,   306,
     307,   308,   309,   310,   311,   312,   313,   314,   315,   316,
       0,     0,     0,     0,     0,     0,     0,     0,   294,   295,
       0,     0,   296,   297,     0,     0,   317,   298,   299,   300,
     301,   302,   303,   304,   305,   306,   307,   308,   309,   310,
     311,   312,   313,   314,   315,   316,   318,     0,     0,     0,
       0,     0,     0,     0,     0,   319,     0,     0,     0,     0,
     320,     0,   470,   294,   295,     0,     0,   296,   297,     0,
       0,     0,   298,   299,   300,   301,   302,   303,   304,   305,
     306,   307,   308,   309,   310,   311,   312,   313,   314,   315,
     316,   319,     0,     0,     0,     0,   320,     0,     0,   294,
     295,     0,     0,   296,   297,     0,     0,   475,   298,   299,
     300,   301,   302,   303,   304,   305,   306,   307,   308,   309,
     310,   311,   312,   313,   314,   315,   316,     0,     7,     8,
       9,    10,    11,     0,     0,   658,   319,    12,    13,    14,
       0,   320,     0,   317,     0,     0,     7,     8,     9,    10,
     108,     0,     0,    16,     0,    12,    13,    14,     7,     8,
       9,    10,   144,     0,     0,     0,     0,    12,    13,    14,
       0,    16,   319,    17,     0,     0,     0,   320,    19,     0,
       0,     0,     0,    16,     0,    17,     0,     0,     7,     8,
       9,    10,   117,     0,     0,     0,    19,    12,    13,    14,
      25,     0,     0,     0,     0,    27,     0,     0,    19,     0,
       0,     0,     0,    16,     0,    17,     0,     0,    25,     0,
       0,     0,     0,    27,     0,     0,     0,     0,     0,     0,
      25,     0,     0,     0,     0,    27,     0,     0,    19,     7,
       8,     9,    10,   153,     0,     0,     0,     0,    12,    13,
      14,     0,     7,     8,     9,    10,    11,     0,     0,     0,
      25,    12,    13,    14,    16,    27,    17,     0,     0,     0,
       0,     7,     8,     9,    10,    11,     0,    16,     0,    17,
      12,    13,    14,     7,     8,     9,    10,   113,     0,    19,
       0,     0,    12,    13,    14,     0,    16,     0,     0,     0,
       0,     0,    19,     7,     8,     9,    10,   149,    16,     0,
       0,    25,    12,    13,    14,     0,    27,     0,     0,     0,
       0,    19,     0,     0,    25,     0,     0,     0,    16,    27,
       0,     0,     0,    19,     0,     0,     7,     8,     9,    10,
     122,     0,     0,    25,     0,    12,    13,    14,    27,     0,
       0,     0,     0,    19,     0,    25,     0,     0,     0,     0,
      27,    16,     7,     8,     9,    10,   158,     0,     0,     0,
       0,    12,    13,    14,     7,    25,     0,    10,   108,     0,
      27,     0,     0,    12,    13,    14,    19,    16,     0,     0,
       0,     0,     0,     7,     0,     0,    10,   117,     0,    16,
       0,    17,    12,    13,    14,     0,     7,     0,    25,    10,
      11,     0,    19,    27,     0,    12,    13,    14,    16,     7,
      17,     0,    10,   113,    19,     0,     0,     0,    12,    13,
      14,    16,     0,    17,    25,     0,     0,     0,     7,    27,
       0,    10,   122,    19,    16,     0,    25,    12,    13,    14,
       0,    27,     0,     0,     0,     0,    19,     0,     0,     0,
       0,     0,     0,    16,     0,    25,     0,     0,     0,    19,
      27,     0,     0,     0,     0,     0,     0,     0,    25,     0,
       0,     0,     0,    27,  1010,     0,     0,     0,    19,     0,
       0,    25,     0,     0,     0,     0,    27,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      25,     0,     0,     0,     0,    27,     0,   352,   353,   354,
    1011,   355,   356,   357,   358,   359,   360,   361,   362,   363,
     364,   365,   366,   367,   368,   352,   353,   354,     0,   355,
     356,   357,   358,   359,   360,   361,   362,   363,   364,   365,
     366,   367,   368,   352,   353,   354,     0,   355,   356,   357,
     358,   359,   360,   361,   362,   363,   364,   365,   366,   367,
     368,     0,     0,     0,     0,     0,     0,     0,     0,   352,
     353,   354,  1091,   355,   356,   357,   358,   359,   360,   361,
     362,   363,   364,   365,   366,   367,   368,     0,   581,   352,
     353,   354,     0,   355,   356,   357,   358,   359,   360,   361,
     362,   363,   364,   365,   366,   367,   368,     0,     0,     0,
       0,   352,   353,   354,   582,   355,   356,   357,   358,   359,
     360,   361,   362,   363,   364,   365,   366,   367,   368,     0,
       0,     0,     0,     0,   824,    17,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   352,   353,
     354,  1002,   355,   356,   357,   358,   359,   360,   361,   362,
     363,   364,   365,   366,   367,   368,   352,   353,   354,  1092,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   352,   353,   354,     0,   355,   356,
     357,   358,   359,   360,   361,   362,   363,   364,   365,   366,
     367,   368
};

static const short yycheck[] =
{
      19,    27,   399,    22,    23,   181,   281,    26,   422,    28,
      29,   426,   323,    25,   424,    27,     3,     3,     5,     5,
     430,   372,   650,   776,   180,   323,    12,    13,    14,   527,
     396,   414,     3,   271,     5,   575,   523,   521,   276,   197,
     198,   271,   340,   341,   838,    64,    65,    66,   755,   207,
    1025,    37,     3,    39,     5,    41,   781,    43,   288,    45,
     218,    47,     1,    49,   792,    51,    92,   186,   523,     1,
      96,   229,   790,   755,   755,     1,   575,     1,   523,     1,
      92,   100,    69,    69,    96,     3,     3,     5,     1,   575,
       1,   781,   782,     3,   864,     5,  1121,   575,    69,   512,
      47,   904,  1077,    10,   541,    54,     3,     0,     5,   512,
     817,    54,     1,     5,   551,   101,    28,   103,    69,   470,
      67,   399,    84,   926,     3,  1150,     5,    89,    47,   512,
     575,    47,   103,    31,   841,    54,    85,   931,   575,   523,
      89,   578,    59,    90,   210,    67,    89,    86,  1123,    47,
      67,    69,   103,     0,   638,    67,    68,  1132,    90,    69,
     841,    68,    86,     3,    90,     5,    31,   518,    84,     3,
     408,     5,    69,    86,    85,   473,    47,    69,     0,    84,
     478,   636,   864,    61,   203,   103,   726,   905,  1163,   907,
      69,   575,   575,    59,    60,    61,   101,    86,   611,   612,
     613,   614,    31,    81,    82,    83,   103,    31,   611,   612,
     613,   614,  1006,    84,    67,   234,   446,   236,   237,   238,
     948,   240,   241,    47,   210,    67,    47,   726,     1,    69,
       3,     4,   615,   111,     6,    69,     8,   588,    67,    68,
     726,    54,   120,    67,    68,   785,    47,    59,   726,     3,
       4,   129,   318,    54,   651,    67,    68,   982,    67,    31,
     138,   627,   248,    84,   250,   331,   252,    85,   254,   147,
       3,   769,   258,   292,   260,   759,   342,    67,   156,   766,
      74,   726,   348,    90,   350,   271,   785,   165,    90,   726,
     276,   529,   982,   983,  1047,     3,   174,   527,   317,   785,
     271,   288,   288,    76,  1074,   276,   211,   785,    81,    44,
     755,   469,    67,    68,    84,     8,    26,   288,    28,    89,
     271,   766,    76,     8,   343,   276,    59,    81,   447,    84,
     449,    66,   318,    68,    67,    68,    90,   288,    31,   776,
     785,   969,   726,    85,   727,   331,    31,    89,   785,    90,
     978,    59,   371,   271,    67,   374,   342,     8,   276,    67,
      68,    67,   348,   389,   350,     3,     4,   618,     3,     4,
     288,   755,   650,   651,   271,    67,    68,   389,   288,   276,
      31,    85,   766,   282,    67,    89,   544,   265,   802,   455,
     804,   288,  1074,    36,   705,   761,    67,    68,    53,    54,
      67,   785,    10,    84,     3,     4,   757,   705,    89,   288,
      67,   686,    47,    84,    67,    68,    77,    78,    79,   864,
      85,    86,   408,     3,    59,    86,     3,     4,   679,     3,
       4,    84,    67,  1140,  1141,   454,    84,   408,    76,    85,
      86,    89,   693,    81,   463,   696,    81,    67,   288,   435,
      84,   437,    90,   439,   288,   441,    84,   408,    89,   446,
     446,    89,     5,     6,     7,     8,    92,    47,   373,   455,
      13,    14,    15,    47,    85,   446,    90,    76,    89,    59,
     864,    85,    81,   611,   612,    59,   614,    67,    31,    84,
     408,    90,   511,    67,   520,   446,   747,    84,   517,    76,
     399,    57,    58,   869,    81,   410,    85,    81,   520,    67,
      89,   408,     8,   579,     8,   581,    72,   583,     3,     4,
      47,     5,     6,     7,     8,    67,    68,    54,   446,    13,
      14,    15,    67,    68,   222,   223,   446,   523,   181,   769,
      67,   527,    85,   529,    77,    78,    79,    31,   799,   446,
     801,   570,   523,    86,   968,    67,   527,   972,   529,   974,
      86,     3,     4,     5,     6,     7,     8,   446,    67,    68,
     541,    85,   523,   970,    59,    89,   527,    86,   529,    47,
     551,   104,   979,   488,    67,    68,     3,     4,   764,   575,
     541,    76,    84,   579,    85,   581,    81,   583,    89,    85,
     551,    85,    84,    89,   575,   523,   446,   578,    84,   527,
    1047,   529,   446,   523,   770,   771,    85,   527,   794,    28,
     263,   264,    31,    47,   575,    10,   523,   578,    84,  1074,
     527,   541,   529,    33,    76,    44,   535,   542,   543,    81,
      84,   551,    59,   881,   523,     3,     4,   803,   527,    84,
      67,    68,    85,   891,   553,   554,    89,   575,  1068,    85,
    1070,    67,   541,    89,    81,   575,    85,    85,   578,  1035,
      89,    89,   551,   659,    72,   661,    85,   663,   575,   665,
      89,    85,    90,   523,   703,    89,    10,   527,    84,   523,
    1074,   969,   970,   527,     6,   946,   575,   419,   420,   578,
     978,   979,     5,     6,     7,     8,   864,   969,   970,    67,
      13,    14,    15,    85,    44,     3,     4,    76,    76,    85,
     706,   740,   708,    81,   710,    44,   712,   541,   714,    89,
     716,    84,   718,   541,   720,   575,   755,   551,    85,   990,
     726,   575,    11,   551,  1019,    89,    47,   270,    91,    47,
     273,   650,   651,   541,   277,   726,    84,    77,    78,    79,
     403,   575,    28,   551,   578,    31,    86,   575,   755,   755,
     578,    59,    47,   949,    84,   726,   419,   420,    44,    67,
     766,    90,    85,   769,   755,    84,    91,   575,   443,   444,
     578,    84,   948,    81,    84,   766,    72,    84,   769,   785,
      84,    67,    68,   822,   755,   776,    62,    63,   726,   828,
      66,    67,    68,    69,   785,   766,   726,    72,   769,    85,
      84,    91,   841,    89,    44,   776,   852,    84,    86,   726,
     849,    90,    86,    86,   785,     7,     8,   755,    84,   851,
     852,    13,    14,    15,    47,   755,     1,   726,   766,    86,
     755,   769,    37,    38,    39,    40,   766,    84,   755,   769,
      45,    46,    47,    48,     3,     4,   776,   785,   523,   766,
      85,    84,   769,    84,    91,   785,   755,   864,   864,  1130,
    1131,    91,   781,   782,    85,  1041,   726,   766,   785,    85,
     769,    28,   726,   864,    89,   881,    89,   776,  1054,    77,
      78,    79,   425,   426,   890,   891,   785,    86,    86,   432,
     881,    85,   726,   864,   813,   755,    85,    84,   726,    89,
     891,   755,    84,    84,   829,   448,   766,    72,    67,   769,
     881,    84,   766,     3,    90,   769,   841,    76,   726,    86,
     891,    47,    81,   848,    47,   785,   864,    89,    86,    47,
      47,   785,    85,    85,   864,     8,     5,     6,     7,     8,
      85,    85,   776,   881,    13,    14,    15,   864,   776,    47,
      72,   785,    89,   891,    85,    85,    67,   785,   633,   634,
      67,    17,    31,    72,   881,   864,   885,   886,   776,   632,
      86,   646,   647,    86,   891,     7,     8,   785,     7,     8,
      85,    13,    14,    15,    13,    14,    15,   530,     5,     6,
       7,     8,   667,   668,    47,    67,    13,    14,    15,    31,
      67,    67,    31,    18,   864,    84,   681,   682,    84,    72,
     864,    85,    67,    84,    31,    85,    85,   942,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,     7,     8,    84,    89,    85,    85,
      13,    14,    15,   248,   249,   250,   251,   722,    91,    85,
     969,   970,    10,    84,    67,    10,  1047,    85,    84,   978,
     979,   512,    84,   982,   983,    85,    10,  1074,  1074,   518,
     415,   551,   551,  1112,   555,   403,  1047,   517,   800,   977,
     837,   929,  1001,  1074,    16,   969,    18,    19,    20,    21,
    1015,   766,     1,   446,   572,     4,   908,   548,     7,     8,
     911,   764,  1027,  1074,    13,    14,    15,   454,  1033,   570,
     653,   562,   563,  1078,   513,  1127,   820,  1047,   781,   782,
      29,  1129,    31,  1042,  1043,  1044,    57,    58,    59,    60,
      61,   794,  1080,   653,   643,   578,  1074,   578,   668,   771,
    1041,   636,   805,   765,  1074,    54,  1071,  1072,  1047,  1074,
     813,   726,   574,   705,   697,   698,  1001,  1074,   701,   574,
     611,   612,   613,   614,   473,   588,   478,    76,    77,    78,
      79,    -1,    81,    -1,    -1,  1074,    85,    86,    -1,    -1,
      -1,    -1,   857,   858,    37,    38,    39,    40,    -1,    -1,
      -1,    -1,    45,    46,    47,    48,  1121,    -1,    -1,    -1,
    1125,    41,    42,    43,    44,    -1,    -1,    -1,    -1,    49,
      50,    51,    52,  1047,  1074,   234,    -1,    -1,    -1,  1047,
    1074,    -1,   897,   898,    -1,  1150,  1151,    -1,    -1,    -1,
     435,   436,   437,   438,    -1,    -1,   687,    -1,   689,  1047,
      -1,    -1,    -1,    -1,   329,   330,     5,     6,     7,     8,
       5,     6,     7,     8,    13,    14,    15,    -1,    13,    14,
      15,    -1,   347,    -1,    -1,    -1,   809,   352,   353,    -1,
      -1,    -1,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,    -1,    -1,   949,    -1,    -1,     1,
      -1,    -1,     4,     5,     6,     7,     8,    -1,    -1,    -1,
      -1,    13,    14,    15,    -1,    -1,   969,   970,    55,    56,
      57,    58,    59,    60,    61,   978,   979,    29,    -1,   982,
     983,    41,    42,    43,    44,    -1,    85,    -1,    -1,    49,
      50,    51,    52,    -1,    -1,    -1,    -1,    -1,  1001,   882,
      -1,   884,    54,    -1,   887,     5,     6,     7,     8,   892,
     893,    -1,   895,    13,    14,    15,    54,    55,    56,    57,
      58,    59,    60,    61,    76,    -1,    -1,   910,    -1,    81,
     913,    31,   915,   916,    -1,    -1,    -1,    -1,    90,    -1,
      -1,     5,     6,     7,     8,    -1,    -1,    -1,    -1,    13,
      14,    15,    -1,    -1,    -1,   248,   249,   250,   251,     5,
       6,     7,     8,   854,    -1,   490,   491,    13,    14,    15,
      -1,    -1,   252,   253,   254,   255,    -1,    -1,     5,     6,
       7,     8,    -1,    -1,    -1,    85,    13,    14,    15,   972,
      -1,   974,    -1,     5,     6,     7,     8,     5,     6,     7,
       8,    13,    14,    15,    31,    13,    14,    15,   991,    -1,
      -1,    -1,   995,    -1,   659,   660,    -1,   908,   663,   664,
     911,    85,    -1,    31,     1,    -1,    -1,     4,     5,     6,
       7,     8,     5,     6,     7,     8,    13,    14,    15,    85,
      13,    14,    15,    -1,   569,    -1,    -1,    -1,    -1,    -1,
      -1,    28,    29,    -1,    31,    -1,    -1,   582,   517,    -1,
      -1,   706,   707,   708,   709,  1048,  1049,    44,  1051,   714,
     715,   716,   717,  1056,    -1,  1058,   601,    54,     4,     5,
       6,     7,     8,  1066,    -1,    11,    -1,    13,    14,    15,
      67,    68,   252,   253,   254,   255,    -1,    -1,    -1,    76,
      -1,    -1,    -1,    29,    81,    31,    -1,    -1,    85,    -1,
      -1,    -1,    89,    90,    -1,    -1,    -1,    -1,    -1,     1,
      -1,    -1,     4,     5,     6,     7,     8,    -1,    54,    -1,
      -1,    13,    14,    15,    -1,    -1,    -1,    -1,    -1,  1030,
      -1,    -1,   435,   436,   437,   438,    -1,    29,    -1,    31,
      76,    -1,    -1,    -1,    -1,    81,    -1,    -1,   683,   439,
     440,   441,   442,     1,    -1,     3,     4,     5,     6,     7,
       8,    -1,    54,    11,    -1,    13,    14,    15,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,  1080,
      -1,    29,    -1,    -1,    76,    -1,    -1,    -1,    -1,    81,
     659,   660,   661,   662,   663,   664,   665,   666,    90,    -1,
     735,     5,     6,     7,     8,    -1,    54,    -1,    -1,    13,
      14,    15,     5,     6,     7,     8,  1117,  1118,    -1,    -1,
      13,    14,    15,    -1,    -1,    -1,    -1,    31,    76,    -1,
      -1,    -1,    -1,    81,    -1,    -1,    84,   706,   707,   708,
     709,   710,   711,   712,   713,   714,   715,   716,   717,   718,
     719,   720,   721,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,   800,    -1,    -1,    -1,   439,
     440,   441,   442,    -1,    -1,    -1,    -1,    -1,    -1,     1,
      -1,     3,     4,    -1,    -1,    -1,    -1,     9,    10,   824,
      12,    -1,    -1,    -1,   829,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   838,   839,    -1,    -1,    -1,    30,   844,
      32,    -1,    34,    35,    36,    37,    38,    -1,    -1,    -1,
      -1,    43,    -1,    -1,    -1,    -1,    -1,    49,    -1,    -1,
      52,    -1,    -1,    -1,    -1,    57,    58,    59,   873,    -1,
      62,    63,    -1,    -1,    66,    67,    68,    -1,    -1,    -1,
      -1,    73,    -1,    75,    76,    -1,   659,   660,    80,    81,
     663,   664,    -1,    -1,    86,    87,    88,    -1,    90,   904,
      92,   661,   662,    -1,   909,   665,   666,   912,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,   926,    -1,    -1,   929,    -1,   931,   932,    -1,    -1,
      -1,    -1,    -1,   706,   707,   708,   709,    -1,    -1,    -1,
      -1,   714,   715,   716,   717,    -1,    -1,    -1,    -1,    -1,
     710,   711,   712,   713,    -1,    -1,    -1,    -1,   718,   719,
     720,   721,   967,    -1,    -1,     1,    -1,    -1,     4,     5,
       6,     7,     8,    -1,    -1,    -1,    -1,    13,    14,    15,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   992,    -1,    -1,
      -1,   996,    28,    29,    -1,    31,    -1,    -1,  1003,    -1,
      -1,  1006,    -1,    -1,    -1,  1010,    -1,    -1,    44,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    54,    -1,
      -1,   661,   662,    -1,    -1,   665,   666,    -1,    -1,    -1,
      -1,    67,    68,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      76,    -1,    -1,    -1,    -1,    81,    -1,    -1,    -1,    85,
      -1,    -1,    -1,    89,    90,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    -1,    -1,    -1,    -1,    -1,
     710,   711,   712,   713,    -1,    -1,    -1,    -1,   718,   719,
     720,   721,     1,    -1,     3,     4,     5,     6,     7,     8,
       9,    10,    -1,    12,    13,    14,    15,    16,    -1,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      -1,    -1,    -1,    -1,    43,    -1,    -1,    -1,    -1,    -1,
      49,    -1,    -1,    52,    -1,    54,    -1,    -1,    57,    58,
      59,    -1,    -1,    62,    63,    -1,    -1,    -1,    67,    68,
      -1,    -1,    -1,    -1,    73,    -1,    75,    76,    -1,    -1,
      -1,    80,    81,    -1,    -1,    -1,    85,    86,    87,    88,
      -1,    90,     1,    92,     3,     4,     5,     6,     7,     8,
       9,    10,    -1,    12,    13,    14,    15,    16,    -1,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    -1,    34,    35,    36,    37,    38,
      -1,    -1,    -1,    -1,    43,    -1,    -1,    -1,    -1,    -1,
      49,    -1,    -1,    52,    -1,    54,    -1,    -1,    57,    58,
      59,    -1,    -1,    62,    63,    -1,    -1,    -1,    67,    68,
      -1,    -1,    -1,    -1,    73,    -1,    75,    76,    -1,    -1,
      -1,    80,    81,    -1,    -1,    -1,    85,    -1,    87,    88,
      -1,    90,     1,    92,     3,     4,    -1,    -1,    -1,    -1,
       9,    10,    -1,    12,    -1,    -1,    -1,    16,    -1,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      -1,    30,    -1,    32,    -1,    34,    35,    36,    37,    38,
      -1,    -1,    -1,    -1,    43,    -1,    -1,    -1,    -1,    -1,
      49,    -1,    -1,    52,    -1,    -1,    -1,    -1,    57,    58,
      59,    -1,    -1,    62,    63,    -1,    -1,    -1,    67,    68,
      -1,    -1,    -1,    -1,    73,    -1,    75,    76,    -1,    -1,
      -1,    80,    81,    -1,    -1,    -1,    85,    -1,    87,    88,
      -1,    90,     1,    92,     3,     4,    -1,    -1,    -1,    -1,
       9,    10,    -1,    12,    -1,    -1,    -1,    16,    -1,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      -1,    30,    -1,    32,    -1,    34,    35,    36,    37,    38,
      -1,    -1,    -1,    -1,    43,    -1,    -1,    -1,    -1,    -1,
      49,    -1,    -1,    52,    -1,    -1,    -1,    -1,    57,    58,
      59,    -1,    -1,    62,    63,    -1,    -1,    -1,    67,    68,
      -1,    -1,    -1,    -1,    73,    -1,    75,    76,    -1,    -1,
      -1,    80,    81,    -1,    -1,    -1,    85,    -1,    87,    88,
      -1,    90,     1,    92,     3,     4,    -1,    -1,     7,     8,
       9,    10,    -1,    12,    13,    14,    15,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      29,    30,    31,    32,    -1,    34,    35,    36,    37,    38,
      -1,    -1,    -1,    -1,    43,    -1,    -1,    -1,    -1,    -1,
      49,    -1,    -1,    52,    -1,    54,    -1,    -1,    57,    58,
      59,    -1,    -1,    62,    63,    -1,    -1,    -1,    67,    68,
      -1,    -1,    -1,    -1,    73,    -1,    75,    76,    -1,    -1,
      -1,    80,    81,    -1,    -1,    -1,    -1,    -1,    87,    88,
      -1,    90,     1,    92,     3,     4,    -1,    -1,    -1,    -1,
       9,    10,    46,    12,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    -1,    -1,
      -1,    30,    -1,    32,    -1,    34,    35,    36,    37,    38,
      -1,    -1,     1,    -1,    43,     4,     5,     6,     7,     8,
      49,    -1,    -1,    52,    13,    14,    15,    -1,    57,    58,
      59,    -1,    -1,    62,    63,    -1,    -1,    66,    67,    68,
      29,    -1,    31,    -1,    73,    -1,    75,    76,    -1,    -1,
      -1,    80,    81,    -1,    -1,    -1,    -1,    86,    87,    88,
       1,    90,     3,    92,    -1,    54,    -1,    -1,     9,    10,
       1,    12,    -1,     4,     5,     6,     7,     8,    -1,    -1,
      -1,    -1,    13,    14,    15,    -1,    -1,    76,    -1,    30,
      -1,    32,    81,    34,    35,    36,    37,    38,    29,    -1,
      -1,    90,    43,    44,    -1,    -1,    -1,    -1,    49,    -1,
      -1,    52,    -1,    -1,    -1,    -1,    57,    58,    59,    -1,
      -1,    62,    63,    54,    -1,    66,    67,    68,    -1,     1,
      -1,     3,    73,    -1,    75,    -1,    -1,     9,    10,    80,
      12,    -1,    -1,    -1,    -1,    76,    87,    88,    -1,    90,
      81,    92,    -1,    -1,    -1,    -1,    -1,    -1,    30,    90,
      32,    -1,    34,    35,    36,    37,    38,    -1,    -1,    -1,
      -1,    43,    -1,    -1,    -1,    -1,    -1,    49,    -1,    -1,
      52,    -1,    -1,    -1,    -1,    57,    58,    59,    -1,    -1,
      62,    63,    -1,    -1,    -1,    67,    68,    -1,     1,    -1,
       3,    73,    -1,    75,    -1,    -1,     9,    10,    80,    12,
      -1,    -1,    -1,    -1,    -1,    87,    88,    -1,    90,    -1,
      92,    -1,    -1,    -1,    -1,    -1,    -1,    30,    -1,    32,
      -1,    34,    35,    36,    37,    38,    -1,    -1,    -1,    -1,
      43,    -1,    -1,    -1,    -1,    -1,    49,    -1,    -1,    52,
      -1,    -1,    -1,    -1,    57,    58,    59,    -1,    -1,    62,
      63,    -1,    -1,    -1,    67,    68,    -1,    -1,    -1,    -1,
      73,    -1,    75,    -1,    -1,    -1,    -1,    80,    -1,    -1,
      -1,    -1,    -1,    -1,    87,    88,    -1,    90,    -1,    92,
       3,     4,     5,     6,     7,     8,     9,    10,    -1,    12,
      13,    14,    15,    -1,    -1,    -1,    -1,    -1,    -1,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      -1,    34,    35,    36,    37,    38,    -1,    -1,    -1,    -1,
      43,    -1,    -1,    -1,    -1,    -1,    49,    -1,    -1,    52,
      -1,    54,    -1,    -1,    57,    58,    59,    -1,    -1,    62,
      63,    -1,    -1,    -1,    67,    68,    -1,    -1,    -1,    -1,
      73,    -1,    75,    76,    -1,    -1,    -1,    80,    81,    -1,
      -1,    -1,    85,    -1,    87,    88,    -1,    90,    -1,    92,
       3,     4,     5,     6,     7,     8,     9,    10,    -1,    12,
      13,    14,    15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    29,    30,    31,    32,
      -1,    34,    35,    36,    37,    38,    -1,    -1,    -1,    -1,
      43,    -1,    -1,    -1,    -1,    -1,    49,    -1,    -1,    52,
      -1,    54,    -1,    -1,    57,    58,    59,    -1,    -1,    62,
      63,    -1,    -1,    -1,    67,    68,    -1,    -1,    -1,    -1,
      73,    -1,    75,    76,     3,     4,    -1,    80,    81,    -1,
       9,    10,    -1,    12,    87,    88,    -1,    -1,    -1,    92,
      -1,    -1,    -1,    22,    23,    24,    25,    26,    27,    28,
      -1,    30,    -1,    32,    -1,    34,    35,    36,    37,    38,
      -1,    -1,    -1,    -1,    43,    -1,    -1,    -1,    -1,    -1,
      49,    -1,    -1,    52,    -1,    -1,    -1,    -1,    57,    58,
      59,    -1,    -1,    62,    63,    -1,    -1,    -1,    67,    68,
      -1,    -1,    -1,    -1,    73,    -1,    75,    76,    -1,    -1,
      -1,    80,    81,    -1,    -1,    -1,    85,    -1,    87,    88,
      -1,    90,    -1,    92,     3,     4,    -1,    -1,     7,     8,
       9,    10,    -1,    12,    13,    14,    15,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      29,    30,    31,    32,    -1,    34,    35,    36,    37,    38,
      -1,    -1,    -1,    -1,    43,    -1,    -1,    -1,    -1,    -1,
      49,    -1,    -1,    52,    -1,    54,    -1,    -1,    57,    58,
      59,    -1,    -1,    62,    63,    -1,    -1,    -1,    67,    68,
      -1,    -1,    -1,    -1,    73,    -1,    75,    76,     3,    -1,
      -1,    80,    81,    -1,     9,    10,    -1,    12,    87,    88,
      -1,    -1,    -1,    92,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    30,    -1,    32,    -1,    34,
      35,    36,    37,    38,    -1,    -1,    -1,    -1,    43,    -1,
      -1,    -1,    -1,    -1,    49,    -1,    -1,    52,    -1,    -1,
      -1,    -1,    57,    58,    59,    -1,    -1,    62,    63,    -1,
      -1,    -1,    67,    68,    -1,    -1,    -1,     3,    73,    -1,
      75,    76,    -1,     9,    10,    80,    12,    -1,    -1,    -1,
      -1,    -1,    87,    88,    -1,    -1,    -1,    92,    -1,    -1,
      -1,    -1,    -1,    -1,    30,    -1,    32,    -1,    34,    35,
      36,    37,    38,    -1,    -1,    -1,    -1,    43,    -1,    -1,
      -1,    -1,    -1,    49,    -1,    -1,    52,    -1,    -1,    -1,
      -1,    57,    58,    59,    -1,    -1,    62,    63,    -1,    -1,
      -1,    67,    68,    -1,     3,    -1,    -1,    73,    -1,    75,
       9,    10,    -1,    12,    80,    -1,    -1,    -1,    -1,    -1,
      -1,    87,    88,    -1,    -1,    91,    92,    -1,    -1,    -1,
      -1,    30,    -1,    32,    -1,    34,    35,    36,    37,    38,
      -1,    -1,    -1,    -1,    43,    -1,    -1,    -1,    -1,    -1,
      49,    -1,    -1,    52,    -1,    -1,    -1,    -1,    57,    58,
      59,    -1,    -1,    62,    63,    -1,    -1,    -1,    67,    68,
      -1,     3,    -1,    -1,    73,    -1,    75,     9,    10,    -1,
      12,    80,    -1,    -1,    -1,    -1,    -1,    -1,    87,    88,
      -1,    90,    -1,    92,    -1,    -1,    -1,    -1,    30,    -1,
      32,    -1,    34,    35,    36,    37,    38,    -1,    -1,    -1,
      -1,    43,    -1,    -1,    -1,    -1,    -1,    49,    -1,    -1,
      52,    -1,    -1,    -1,    -1,    57,    58,    59,    -1,    -1,
      62,    63,    -1,    -1,    -1,    67,    68,    -1,     3,    -1,
      -1,    73,    -1,    75,     9,    10,    -1,    12,    80,    -1,
      -1,    -1,    -1,    -1,    -1,    87,    88,    -1,    -1,    91,
      92,    -1,    -1,    -1,    -1,    30,    -1,    32,    -1,    34,
      35,    36,    37,    38,    -1,    -1,    -1,    -1,    43,    -1,
      -1,    -1,    -1,    -1,    49,    -1,    -1,    52,    -1,    -1,
      -1,    -1,    57,    58,    59,    -1,    -1,    62,    63,    -1,
      -1,    -1,    67,    68,    -1,     3,    -1,    -1,    73,    -1,
      75,     9,    10,    -1,    12,    80,    -1,    -1,    -1,    -1,
      85,    -1,    87,    88,    -1,    -1,    -1,    92,    -1,    -1,
      -1,    -1,    30,    -1,    32,    -1,    34,    35,    36,    37,
      38,    -1,    -1,    -1,    -1,    43,    -1,    -1,    -1,    -1,
      -1,    49,    -1,    -1,    52,    -1,    -1,    -1,    -1,    57,
      58,    59,    -1,    -1,    62,    63,    -1,    -1,    -1,    67,
      68,    -1,     3,    -1,    -1,    73,    -1,    75,     9,    10,
      -1,    12,    80,    -1,    -1,    -1,    -1,    -1,    -1,    87,
      88,    -1,    -1,    -1,    92,    -1,    -1,    -1,    -1,    30,
      -1,    32,    -1,    34,    35,    36,    37,    38,    -1,    -1,
      -1,    -1,    43,    -1,    -1,    -1,    -1,    -1,    49,    -1,
      -1,    52,    -1,    -1,    -1,    -1,    57,    58,    59,    -1,
      -1,    62,    63,    -1,    -1,    -1,    67,    68,    -1,     3,
      -1,    -1,    73,    -1,    75,     9,    10,    -1,    12,    80,
      -1,    -1,    -1,    -1,    -1,    -1,    87,    88,    -1,    -1,
      -1,    92,    -1,    -1,    -1,    -1,    30,    -1,    32,    -1,
      34,    35,    36,    37,    38,    -1,    -1,    -1,    -1,    43,
      -1,    -1,    -1,    -1,    -1,    49,    -1,    -1,    52,    -1,
      -1,    -1,    -1,    57,    58,    59,    -1,    -1,    62,    63,
      -1,    -1,    -1,    67,    68,    -1,     3,    -1,    -1,    73,
      -1,    75,     9,    10,    -1,    12,    80,    -1,    -1,    -1,
      -1,    -1,    -1,    87,    88,    -1,    -1,    -1,    92,    -1,
      -1,    -1,    -1,    30,    -1,    32,    -1,    34,    35,    36,
      37,    38,    -1,    -1,    -1,    -1,    43,    -1,    -1,    -1,
      -1,    -1,    49,    -1,    -1,    52,    -1,    -1,    -1,    -1,
      57,    58,    59,    -1,    -1,    62,    63,    -1,    -1,    -1,
      67,    68,    -1,    -1,    -1,    -1,    73,    -1,    75,    -1,
      -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,
      87,    88,    -1,     1,    -1,    92,     4,     5,     6,     7,
       8,    -1,    -1,    -1,    -1,    13,    14,    15,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      28,    29,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    44,    -1,    -1,     3,
       4,    -1,    -1,     7,     8,    -1,    54,    -1,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    -1,    76,    -1,
      -1,    -1,    -1,    81,    -1,    -1,    -1,    85,    -1,    -1,
      -1,    89,    90,    47,    -1,     1,    -1,    -1,     4,    -1,
      -1,     7,     8,    -1,    -1,    -1,    -1,    13,    14,    15,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    76,    29,    -1,    31,    -1,    81,    -1,    -1,
       1,    -1,    -1,     4,    -1,    89,     7,     8,    -1,    -1,
      -1,    -1,    13,    14,    15,    -1,    -1,    -1,    54,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    29,    -1,
      31,    32,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      76,    77,    78,    79,    -1,    81,    -1,    -1,    -1,    85,
      86,    -1,    -1,    54,    -1,    -1,    -1,    -1,    -1,     1,
      -1,     3,     4,     5,     6,     7,     8,    -1,    -1,    -1,
      -1,    13,    14,    15,    -1,    76,    -1,    -1,    -1,    -1,
      81,    -1,    -1,    -1,    85,    86,    28,    29,    -1,    31,
      32,     1,    -1,     3,     4,     5,     6,     7,     8,    -1,
      -1,    -1,    -1,    13,    14,    15,    -1,    -1,    -1,    -1,
      -1,    -1,    54,    -1,    -1,    57,    58,    59,    -1,    29,
      -1,    31,    -1,    -1,    -1,    67,    -1,    -1,    70,    71,
      72,    -1,    -1,    -1,    76,    -1,    -1,    -1,    80,    81,
      82,    83,    -1,    85,    54,    -1,    -1,    57,    58,    59,
      -1,    -1,    -1,    -1,    -1,    -1,     1,    67,     3,     4,
       5,     6,     7,     8,    -1,    -1,    76,    -1,    13,    14,
      15,    81,    -1,    -1,    -1,    85,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    29,     1,    31,    -1,     4,     5,
       6,     7,     8,    -1,    -1,    11,    -1,    13,    14,    15,
      -1,     4,     5,     6,     7,     8,    -1,    -1,    -1,    54,
      13,    14,    15,    29,    59,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    67,    -1,    -1,    -1,    29,    -1,    31,    -1,
      -1,    76,    -1,    -1,    -1,    -1,    81,    -1,    54,    -1,
      85,    -1,    -1,    -1,     4,     5,     6,     7,     8,    -1,
      -1,    54,    -1,    13,    14,    15,    -1,    -1,    -1,    -1,
      76,    -1,    -1,    -1,    -1,    81,    -1,    -1,    84,    29,
      -1,    31,    -1,    76,    -1,    -1,    -1,    -1,    81,    -1,
      -1,    -1,    85,    -1,    -1,    -1,     4,     5,     6,     7,
       8,    -1,    -1,    -1,    54,    13,    14,    15,    -1,     4,
       5,     6,     7,     8,    -1,    -1,    -1,    -1,    13,    14,
      15,    29,    -1,    31,    -1,    -1,    76,    -1,    -1,    -1,
      -1,    81,    -1,    -1,    29,    85,    31,    -1,     4,     5,
       6,     7,     8,    -1,    -1,    -1,    54,    13,    14,    15,
      -1,     4,     5,     6,     7,     8,    -1,    -1,    -1,    54,
      13,    14,    15,    29,    -1,    -1,    -1,    -1,    76,    -1,
      -1,    -1,    -1,    81,    -1,    -1,    29,    85,    -1,    -1,
      -1,    76,    -1,    -1,    -1,    -1,    81,    -1,    54,    -1,
      85,    -1,    -1,     4,     5,     6,     7,     8,    -1,    -1,
      -1,    54,    13,    14,    15,    -1,    -1,    -1,    -1,    -1,
      76,    -1,    -1,    -1,    -1,    81,    -1,    -1,    29,    85,
      -1,    -1,    -1,    76,    -1,    -1,    -1,    -1,    81,    -1,
      -1,    -1,    85,    -1,    -1,     4,     5,     6,     7,     8,
      -1,    -1,    -1,    54,    13,    14,    15,    -1,    -1,    11,
       1,    -1,    -1,     4,    -1,    -1,     7,     8,    -1,    -1,
      29,    -1,    13,    14,    15,    76,    -1,    -1,    -1,    -1,
      81,    -1,    -1,    -1,    85,    -1,    -1,    -1,    29,    -1,
      31,    32,    44,    45,    46,    54,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      -1,    -1,    -1,    54,    -1,    -1,    -1,    76,    -1,    -1,
      -1,    -1,    81,    -1,    -1,    -1,    85,    -1,    -1,    -1,
      -1,    -1,     3,     4,    -1,    76,     7,     8,    -1,    91,
      81,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,
      -1,    -1,     7,     8,    -1,    -1,    47,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    67,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    76,    -1,    -1,    -1,    -1,
      81,    -1,    47,     3,     4,    -1,    -1,     7,     8,    -1,
      -1,    -1,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    76,    -1,    -1,    -1,    -1,    81,    -1,    -1,     3,
       4,    -1,    -1,     7,     8,    -1,    -1,    47,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    -1,     4,     5,
       6,     7,     8,    -1,    -1,    11,    76,    13,    14,    15,
      -1,    81,    -1,    47,    -1,    -1,     4,     5,     6,     7,
       8,    -1,    -1,    29,    -1,    13,    14,    15,     4,     5,
       6,     7,     8,    -1,    -1,    -1,    -1,    13,    14,    15,
      -1,    29,    76,    31,    -1,    -1,    -1,    81,    54,    -1,
      -1,    -1,    -1,    29,    -1,    31,    -1,    -1,     4,     5,
       6,     7,     8,    -1,    -1,    -1,    54,    13,    14,    15,
      76,    -1,    -1,    -1,    -1,    81,    -1,    -1,    54,    -1,
      -1,    -1,    -1,    29,    -1,    31,    -1,    -1,    76,    -1,
      -1,    -1,    -1,    81,    -1,    -1,    -1,    -1,    -1,    -1,
      76,    -1,    -1,    -1,    -1,    81,    -1,    -1,    54,     4,
       5,     6,     7,     8,    -1,    -1,    -1,    -1,    13,    14,
      15,    -1,     4,     5,     6,     7,     8,    -1,    -1,    -1,
      76,    13,    14,    15,    29,    81,    31,    -1,    -1,    -1,
      -1,     4,     5,     6,     7,     8,    -1,    29,    -1,    31,
      13,    14,    15,     4,     5,     6,     7,     8,    -1,    54,
      -1,    -1,    13,    14,    15,    -1,    29,    -1,    -1,    -1,
      -1,    -1,    54,     4,     5,     6,     7,     8,    29,    -1,
      -1,    76,    13,    14,    15,    -1,    81,    -1,    -1,    -1,
      -1,    54,    -1,    -1,    76,    -1,    -1,    -1,    29,    81,
      -1,    -1,    -1,    54,    -1,    -1,     4,     5,     6,     7,
       8,    -1,    -1,    76,    -1,    13,    14,    15,    81,    -1,
      -1,    -1,    -1,    54,    -1,    76,    -1,    -1,    -1,    -1,
      81,    29,     4,     5,     6,     7,     8,    -1,    -1,    -1,
      -1,    13,    14,    15,     4,    76,    -1,     7,     8,    -1,
      81,    -1,    -1,    13,    14,    15,    54,    29,    -1,    -1,
      -1,    -1,    -1,     4,    -1,    -1,     7,     8,    -1,    29,
      -1,    31,    13,    14,    15,    -1,     4,    -1,    76,     7,
       8,    -1,    54,    81,    -1,    13,    14,    15,    29,     4,
      31,    -1,     7,     8,    54,    -1,    -1,    -1,    13,    14,
      15,    29,    -1,    31,    76,    -1,    -1,    -1,     4,    81,
      -1,     7,     8,    54,    29,    -1,    76,    13,    14,    15,
      -1,    81,    -1,    -1,    -1,    -1,    54,    -1,    -1,    -1,
      -1,    -1,    -1,    29,    -1,    76,    -1,    -1,    -1,    54,
      81,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    76,    -1,
      -1,    -1,    -1,    81,    11,    -1,    -1,    -1,    54,    -1,
      -1,    76,    -1,    -1,    -1,    -1,    81,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      76,    -1,    -1,    -1,    -1,    81,    -1,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    44,    45,    46,    -1,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    44,    45,    46,    -1,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    44,
      45,    46,    91,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    -1,    89,    44,
      45,    46,    -1,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    -1,    -1,    -1,
      -1,    44,    45,    46,    89,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    -1,
      -1,    -1,    -1,    -1,    89,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    44,    45,
      46,    84,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    44,    45,    46,    -1,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned short yystos[] =
{
       0,    94,    95,    96,     0,    97,     1,     4,     5,     6,
       7,     8,    13,    14,    15,    28,    29,    31,    32,    54,
      57,    58,    70,    71,    72,    76,    80,    81,    82,    83,
      85,    98,    99,   100,   101,   116,   133,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   148,
     149,   150,   151,   152,   153,   159,   161,   162,   163,   164,
     165,   174,   175,   179,   202,   203,   204,   205,   210,   295,
     296,   298,   299,   300,   310,   313,   320,   321,    98,    85,
      86,   174,   174,   174,    67,    67,     3,     4,    76,    81,
     108,   297,   108,   108,   312,   313,   108,   297,   312,   297,
     108,    67,     3,    59,    67,   167,   171,   201,     8,   161,
     162,   174,   179,     8,   161,   162,   179,     8,   161,   162,
     174,   179,     8,   161,   162,   179,     8,   163,   164,   174,
     179,     8,   163,   164,   179,     8,   163,   164,   174,   179,
       8,   163,   164,   179,     8,   161,   162,   174,   179,     8,
     161,   162,   179,     8,   161,   162,   174,   179,     8,   161,
     162,   179,     8,   163,   164,   174,   179,     8,   163,   164,
     179,     8,   163,   164,   174,   179,     8,   163,   164,   179,
     133,   133,    85,   175,    90,   108,    90,   108,    90,   108,
      98,   322,     3,     9,    10,    12,    30,    34,    35,    36,
      37,    38,    43,    49,    52,    57,    58,    59,    62,    63,
      67,    68,    73,    75,    80,    87,    88,    92,   109,   110,
     112,   113,   114,   115,   117,   118,   124,   126,   249,   295,
     352,   356,   357,   358,    67,    54,    89,    47,    67,   312,
      47,    67,    90,   312,    85,    85,   108,   110,   136,   137,
     138,   139,   140,   141,   142,   143,   158,   222,   136,   137,
     138,   139,   157,   160,   173,   174,    85,    89,     1,    28,
      67,    68,   106,   168,   230,     4,    59,    67,    81,   166,
     169,   196,   197,   201,   167,   201,    74,   213,   214,    90,
     213,    90,   209,    90,     3,     4,     7,     8,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    47,    67,    76,
      81,   333,   342,   343,   344,   345,   346,   117,   117,    67,
      67,    67,   108,   117,     1,    90,   110,   222,    76,   110,
     351,    67,    67,    67,    10,   117,    84,    89,    67,   113,
      67,   113,    44,    45,    46,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    66,    67,    68,    69,    92,     1,    86,   238,   247,
     117,     7,     8,   108,   176,   177,   178,   179,   108,   108,
     108,    90,   303,   108,   108,   307,   311,    85,    84,   223,
      84,   201,   201,   134,   173,    67,   173,   290,     6,   157,
     160,     1,   127,   128,   129,   130,   237,   256,   173,   160,
     173,    85,    89,     1,   102,   168,    67,   230,    85,     1,
     104,    67,    86,     1,    85,   136,   137,   138,   139,   140,
     141,   142,   143,   156,   157,   215,   295,   206,    86,   207,
       1,   108,   220,   221,   208,    67,   108,   222,   323,    89,
     340,   344,   346,    47,   118,   118,   222,    84,    84,    84,
      47,   344,   347,   348,   350,    47,   344,   353,   354,   355,
     222,   108,    85,   118,   222,   222,   118,   118,   121,   123,
     120,   119,   118,   118,   118,   118,   118,   118,   118,   118,
     118,   118,   118,   118,   108,   111,   112,   110,   108,    10,
      84,    33,   242,   243,   244,    84,    84,    89,    67,   312,
      84,   301,   325,   326,    90,    84,   314,   316,   325,    59,
      67,   224,   226,   227,   228,   229,   230,    84,   171,   201,
      10,   291,   160,     6,    59,    91,   110,    85,   107,   237,
     129,   257,    44,   197,   197,   134,   127,   173,   290,   127,
      76,   173,   133,   133,    85,   215,   213,   173,   213,    44,
      89,   212,   220,   222,    84,    85,   334,    11,   341,    67,
     108,    89,    89,    89,    90,   117,   112,   349,    47,    91,
     344,   350,    47,    84,   344,   355,    84,    84,    84,    84,
     110,    47,   118,   118,    84,    91,   108,   294,     1,   132,
     231,   232,   233,   234,   235,   236,   237,   248,   256,   260,
     261,   244,    84,   177,     3,   111,    90,   306,   312,   314,
      72,   100,   133,   152,   153,   320,   327,   330,   308,    77,
      78,    79,    86,   315,     1,    85,   156,   157,   317,    72,
     160,   173,   280,    67,   230,    84,     1,     3,    11,   136,
     137,   140,   141,   144,   145,   148,   149,   154,   155,   282,
     285,   286,   288,   292,   293,   110,   110,    91,    91,   256,
     131,   154,   155,   172,    84,   169,   196,   103,    44,   105,
      84,   216,   218,   256,   217,   219,   256,    86,    86,   118,
     221,    86,   212,    84,   342,   343,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   147,   148,   149,
     150,   151,   152,   153,   237,   335,   336,   337,   324,   285,
     286,   222,   222,   118,   222,   125,   349,    47,   118,    85,
      89,   132,   260,   261,   132,   260,   261,   256,   260,   261,
     132,   260,   261,   237,    86,   257,    84,    89,    84,   304,
     325,   309,    86,   133,   133,   331,   328,   330,   314,   316,
     133,   133,    85,   227,   228,   226,   281,   173,   280,    84,
     133,   289,   289,    84,    85,    89,    84,    89,    91,    91,
     257,    85,   133,    85,   133,     1,    90,   118,   180,   256,
     170,   256,    89,   257,    89,   257,   173,   173,   173,    86,
     108,   340,    85,   133,    85,   337,   237,     1,    90,   245,
     246,   250,    84,    84,    89,    84,     1,     3,    66,    68,
      90,   108,   118,   182,   183,   184,   186,   188,   189,   122,
     108,   257,   239,     4,    22,    23,    24,    25,    26,    27,
      28,    76,    81,    85,   108,   110,   135,   152,   153,   159,
     240,   250,   272,   273,   295,   112,   314,    72,   325,   302,
     333,   100,    86,    47,   196,   318,   319,   318,    84,   282,
       4,    59,    67,    81,   198,   199,   200,   201,   225,   226,
     227,    59,    67,   201,   225,   283,    11,   152,   153,   287,
       3,   245,   166,   167,   181,   257,   180,   257,   134,    47,
     196,   134,    47,   201,   173,   198,   201,   225,   338,   339,
     250,   247,   108,   118,   108,   118,   187,    47,    86,    89,
     211,    44,    68,   189,   186,   118,   118,    47,    85,    85,
      85,   110,    59,   108,     8,   274,   256,    85,   133,   133,
      85,    16,    18,    19,    20,    21,   251,   252,   254,   262,
     135,    84,    86,    72,   325,   332,   118,    47,    89,   160,
     173,   173,    67,   230,    67,   230,   173,   174,   160,   173,
     173,   173,   133,   133,    85,    85,   182,   245,   245,   218,
     256,   118,    47,   173,   219,   118,    47,   173,   173,   173,
      85,    89,    84,    11,    91,   182,   185,   184,   186,   118,
      11,    47,    85,   110,    85,    67,   257,   166,   190,   196,
     167,   193,   201,   253,   264,   255,   266,    67,    17,     1,
     240,   259,     1,    67,   241,   305,    72,     1,    85,   329,
     118,   134,   199,   200,   200,   290,   290,   284,   198,   201,
     225,   201,   225,    86,   257,   173,   118,   173,   118,   339,
     118,    86,   186,   118,    85,   110,    47,    85,   191,    85,
     194,    67,    67,   259,    67,   110,     1,   263,   258,   260,
     261,   110,   325,   319,    84,   282,   173,   173,   173,   173,
     173,    91,    47,    47,    84,   173,   127,   127,   110,   110,
      18,   110,   135,   271,   275,    84,   259,   241,   258,    84,
      72,    10,    68,   276,   277,   278,    85,   192,   195,    84,
      84,   267,    85,   270,    85,    67,   108,    47,    84,    89,
     256,   256,   265,   275,   259,   110,    91,   276,    85,   278,
     257,   257,   259,    85,    84,    10,    47,    84,   250,   250,
     268,    67,    10,   279,    85,   275,   110,    84,    89,    84,
      84,    85,    10,   269,   259
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
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
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
| TOP (cinluded).                                                   |
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
  unsigned int yylineno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylineno);
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

#if YYMAXDEPTH == 0
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
     to reallocate them elsewhere.  */

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
	/* Give user a chance to reallocate the stack. Use copies of
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
#line 345 "objc-parse.y"
    { if (pedantic)
		    pedwarn ("ISO C forbids an empty source file");
		  finish_file ();
		;}
    break;

  case 3:
#line 350 "objc-parse.y"
    {
		  /* In case there were missing closebraces,
		     get us back to the global binding level.  */
		  while (! global_bindings_p ())
		    poplevel (0, 0, 0);
		  /* __FUNCTION__ is defined at file scope ("").  This
		     call may not be necessary as my tests indicate it
		     still works without it.  */
		  finish_fname_decls ();
                  finish_file ();
		;}
    break;

  case 4:
#line 368 "objc-parse.y"
    {yyval.ttype = NULL_TREE; ;}
    break;

  case 6:
#line 369 "objc-parse.y"
    {yyval.ttype = NULL_TREE; ggc_collect(); ;}
    break;

  case 8:
#line 374 "objc-parse.y"
    { parsing_iso_function_signature = false; ;}
    break;

  case 12:
#line 382 "objc-parse.y"
    { STRIP_NOPS (yyvsp[-2].ttype);
		  if ((TREE_CODE (yyvsp[-2].ttype) == ADDR_EXPR
		       && TREE_CODE (TREE_OPERAND (yyvsp[-2].ttype, 0)) == STRING_CST)
		      || TREE_CODE (yyvsp[-2].ttype) == STRING_CST)
		    assemble_asm (yyvsp[-2].ttype);
		  else
		    error ("argument of `asm' is not a constant string"); ;}
    break;

  case 13:
#line 390 "objc-parse.y"
    { RESTORE_EXT_FLAGS (yyvsp[-1].ttype); ;}
    break;

  case 14:
#line 395 "objc-parse.y"
    { if (pedantic)
		    error ("ISO C forbids data definition with no type or storage class");
		  else
		    warning ("data definition has no type or storage class");

		  POP_DECLSPEC_STACK; ;}
    break;

  case 15:
#line 402 "objc-parse.y"
    { POP_DECLSPEC_STACK; ;}
    break;

  case 16:
#line 404 "objc-parse.y"
    { POP_DECLSPEC_STACK; ;}
    break;

  case 17:
#line 406 "objc-parse.y"
    { shadow_tag (yyvsp[-1].ttype); ;}
    break;

  case 20:
#line 410 "objc-parse.y"
    { if (pedantic)
		    pedwarn ("ISO C does not allow extra `;' outside of a function"); ;}
    break;

  case 21:
#line 416 "objc-parse.y"
    { if (! start_function (current_declspecs, yyvsp[0].ttype,
					all_prefix_attributes))
		    YYERROR1;
		;}
    break;

  case 22:
#line 421 "objc-parse.y"
    { store_parm_decls (); ;}
    break;

  case 23:
#line 423 "objc-parse.y"
    { DECL_SOURCE_FILE (current_function_decl) = yyvsp[-2].filename;
		  DECL_SOURCE_LINE (current_function_decl) = yyvsp[-1].lineno;
		  finish_function (0, 1);
		  POP_DECLSPEC_STACK; ;}
    break;

  case 24:
#line 428 "objc-parse.y"
    { POP_DECLSPEC_STACK; ;}
    break;

  case 25:
#line 430 "objc-parse.y"
    { if (! start_function (current_declspecs, yyvsp[0].ttype,
					all_prefix_attributes))
		    YYERROR1;
		;}
    break;

  case 26:
#line 435 "objc-parse.y"
    { store_parm_decls (); ;}
    break;

  case 27:
#line 437 "objc-parse.y"
    { DECL_SOURCE_FILE (current_function_decl) = yyvsp[-2].filename;
		  DECL_SOURCE_LINE (current_function_decl) = yyvsp[-1].lineno;
		  finish_function (0, 1);
		  POP_DECLSPEC_STACK; ;}
    break;

  case 28:
#line 442 "objc-parse.y"
    { POP_DECLSPEC_STACK; ;}
    break;

  case 29:
#line 444 "objc-parse.y"
    { if (! start_function (NULL_TREE, yyvsp[0].ttype,
					all_prefix_attributes))
		    YYERROR1;
		;}
    break;

  case 30:
#line 449 "objc-parse.y"
    { store_parm_decls (); ;}
    break;

  case 31:
#line 451 "objc-parse.y"
    { DECL_SOURCE_FILE (current_function_decl) = yyvsp[-2].filename;
		  DECL_SOURCE_LINE (current_function_decl) = yyvsp[-1].lineno;
		  finish_function (0, 1);
		  POP_DECLSPEC_STACK; ;}
    break;

  case 32:
#line 456 "objc-parse.y"
    { POP_DECLSPEC_STACK; ;}
    break;

  case 37:
#line 467 "objc-parse.y"
    { yyval.code = ADDR_EXPR; ;}
    break;

  case 38:
#line 469 "objc-parse.y"
    { yyval.code = NEGATE_EXPR; ;}
    break;

  case 39:
#line 471 "objc-parse.y"
    { yyval.code = CONVERT_EXPR;
		;}
    break;

  case 40:
#line 474 "objc-parse.y"
    { yyval.code = PREINCREMENT_EXPR; ;}
    break;

  case 41:
#line 476 "objc-parse.y"
    { yyval.code = PREDECREMENT_EXPR; ;}
    break;

  case 42:
#line 478 "objc-parse.y"
    { yyval.code = BIT_NOT_EXPR; ;}
    break;

  case 43:
#line 480 "objc-parse.y"
    { yyval.code = TRUTH_NOT_EXPR; ;}
    break;

  case 44:
#line 484 "objc-parse.y"
    { yyval.ttype = build_compound_expr (yyvsp[0].ttype); ;}
    break;

  case 45:
#line 489 "objc-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 47:
#line 495 "objc-parse.y"
    { yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ttype); ;}
    break;

  case 48:
#line 497 "objc-parse.y"
    { chainon (yyvsp[-2].ttype, build_tree_list (NULL_TREE, yyvsp[0].ttype)); ;}
    break;

  case 50:
#line 503 "objc-parse.y"
    { yyval.ttype = build_indirect_ref (yyvsp[0].ttype, "unary *"); ;}
    break;

  case 51:
#line 506 "objc-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  RESTORE_EXT_FLAGS (yyvsp[-1].ttype); ;}
    break;

  case 52:
#line 509 "objc-parse.y"
    { yyval.ttype = build_unary_op (yyvsp[-1].code, yyvsp[0].ttype, 0);
		  overflow_warning (yyval.ttype); ;}
    break;

  case 53:
#line 513 "objc-parse.y"
    { yyval.ttype = finish_label_address_expr (yyvsp[0].ttype); ;}
    break;

  case 54:
#line 515 "objc-parse.y"
    { skip_evaluation--;
		  if (TREE_CODE (yyvsp[0].ttype) == COMPONENT_REF
		      && DECL_C_BIT_FIELD (TREE_OPERAND (yyvsp[0].ttype, 1)))
		    error ("`sizeof' applied to a bit-field");
		  yyval.ttype = c_sizeof (TREE_TYPE (yyvsp[0].ttype)); ;}
    break;

  case 55:
#line 521 "objc-parse.y"
    { skip_evaluation--;
		  yyval.ttype = c_sizeof (groktypename (yyvsp[-1].ttype)); ;}
    break;

  case 56:
#line 524 "objc-parse.y"
    { skip_evaluation--;
		  yyval.ttype = c_alignof_expr (yyvsp[0].ttype); ;}
    break;

  case 57:
#line 527 "objc-parse.y"
    { skip_evaluation--;
		  yyval.ttype = c_alignof (groktypename (yyvsp[-1].ttype)); ;}
    break;

  case 58:
#line 530 "objc-parse.y"
    { yyval.ttype = build_unary_op (REALPART_EXPR, yyvsp[0].ttype, 0); ;}
    break;

  case 59:
#line 532 "objc-parse.y"
    { yyval.ttype = build_unary_op (IMAGPART_EXPR, yyvsp[0].ttype, 0); ;}
    break;

  case 60:
#line 536 "objc-parse.y"
    { skip_evaluation++; ;}
    break;

  case 61:
#line 540 "objc-parse.y"
    { skip_evaluation++; ;}
    break;

  case 62:
#line 544 "objc-parse.y"
    { skip_evaluation++; ;}
    break;

  case 64:
#line 550 "objc-parse.y"
    { yyval.ttype = c_cast_expr (yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 66:
#line 556 "objc-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 67:
#line 558 "objc-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 68:
#line 560 "objc-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 69:
#line 562 "objc-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 70:
#line 564 "objc-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 71:
#line 566 "objc-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 72:
#line 568 "objc-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 73:
#line 570 "objc-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 74:
#line 572 "objc-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 75:
#line 574 "objc-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 76:
#line 576 "objc-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 77:
#line 578 "objc-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 78:
#line 580 "objc-parse.y"
    { yyvsp[-1].ttype = c_common_truthvalue_conversion
		    (default_conversion (yyvsp[-1].ttype));
		  skip_evaluation += yyvsp[-1].ttype == boolean_false_node; ;}
    break;

  case 79:
#line 584 "objc-parse.y"
    { skip_evaluation -= yyvsp[-3].ttype == boolean_false_node;
		  yyval.ttype = parser_build_binary_op (TRUTH_ANDIF_EXPR, yyvsp[-3].ttype, yyvsp[0].ttype); ;}
    break;

  case 80:
#line 587 "objc-parse.y"
    { yyvsp[-1].ttype = c_common_truthvalue_conversion
		    (default_conversion (yyvsp[-1].ttype));
		  skip_evaluation += yyvsp[-1].ttype == boolean_true_node; ;}
    break;

  case 81:
#line 591 "objc-parse.y"
    { skip_evaluation -= yyvsp[-3].ttype == boolean_true_node;
		  yyval.ttype = parser_build_binary_op (TRUTH_ORIF_EXPR, yyvsp[-3].ttype, yyvsp[0].ttype); ;}
    break;

  case 82:
#line 594 "objc-parse.y"
    { yyvsp[-1].ttype = c_common_truthvalue_conversion
		    (default_conversion (yyvsp[-1].ttype));
		  skip_evaluation += yyvsp[-1].ttype == boolean_false_node; ;}
    break;

  case 83:
#line 598 "objc-parse.y"
    { skip_evaluation += ((yyvsp[-4].ttype == boolean_true_node)
				      - (yyvsp[-4].ttype == boolean_false_node)); ;}
    break;

  case 84:
#line 601 "objc-parse.y"
    { skip_evaluation -= yyvsp[-6].ttype == boolean_true_node;
		  yyval.ttype = build_conditional_expr (yyvsp[-6].ttype, yyvsp[-3].ttype, yyvsp[0].ttype); ;}
    break;

  case 85:
#line 604 "objc-parse.y"
    { if (pedantic)
		    pedwarn ("ISO C forbids omitting the middle term of a ?: expression");
		  /* Make sure first operand is calculated only once.  */
		  yyvsp[0].ttype = save_expr (yyvsp[-1].ttype);
		  yyvsp[-1].ttype = c_common_truthvalue_conversion
		    (default_conversion (yyvsp[0].ttype));
		  skip_evaluation += yyvsp[-1].ttype == boolean_true_node; ;}
    break;

  case 86:
#line 612 "objc-parse.y"
    { skip_evaluation -= yyvsp[-4].ttype == boolean_true_node;
		  yyval.ttype = build_conditional_expr (yyvsp[-4].ttype, yyvsp[-3].ttype, yyvsp[0].ttype); ;}
    break;

  case 87:
#line 615 "objc-parse.y"
    { char class;
		  yyval.ttype = build_modify_expr (yyvsp[-2].ttype, NOP_EXPR, yyvsp[0].ttype);
		  class = TREE_CODE_CLASS (TREE_CODE (yyval.ttype));
		  if (IS_EXPR_CODE_CLASS (class))
		    C_SET_EXP_ORIGINAL_CODE (yyval.ttype, MODIFY_EXPR);
		;}
    break;

  case 88:
#line 622 "objc-parse.y"
    { char class;
		  yyval.ttype = build_modify_expr (yyvsp[-2].ttype, yyvsp[-1].code, yyvsp[0].ttype);
		  /* This inhibits warnings in
		     c_common_truthvalue_conversion.  */
		  class = TREE_CODE_CLASS (TREE_CODE (yyval.ttype));
		  if (IS_EXPR_CODE_CLASS (class))
		    C_SET_EXP_ORIGINAL_CODE (yyval.ttype, ERROR_MARK);
		;}
    break;

  case 89:
#line 634 "objc-parse.y"
    {
		  if (yychar == YYEMPTY)
		    yychar = YYLEX;
		  yyval.ttype = build_external_ref (yyvsp[0].ttype, yychar == '(');
		;}
    break;

  case 91:
#line 641 "objc-parse.y"
    { yyval.ttype = fix_string_type (yyval.ttype); ;}
    break;

  case 92:
#line 643 "objc-parse.y"
    { yyval.ttype = fname_decl (C_RID_CODE (yyval.ttype), yyval.ttype); ;}
    break;

  case 93:
#line 645 "objc-parse.y"
    { start_init (NULL_TREE, NULL, 0);
		  yyvsp[-2].ttype = groktypename (yyvsp[-2].ttype);
		  really_start_incremental_init (yyvsp[-2].ttype); ;}
    break;

  case 94:
#line 649 "objc-parse.y"
    { tree constructor = pop_init_level (0);
		  tree type = yyvsp[-5].ttype;
		  finish_init ();

		  if (pedantic && ! flag_isoc99)
		    pedwarn ("ISO C89 forbids compound literals");
		  yyval.ttype = build_compound_literal (type, constructor);
		;}
    break;

  case 95:
#line 658 "objc-parse.y"
    { char class = TREE_CODE_CLASS (TREE_CODE (yyvsp[-1].ttype));
		  if (IS_EXPR_CODE_CLASS (class))
		    C_SET_EXP_ORIGINAL_CODE (yyvsp[-1].ttype, ERROR_MARK);
		  yyval.ttype = yyvsp[-1].ttype; ;}
    break;

  case 96:
#line 663 "objc-parse.y"
    { yyval.ttype = error_mark_node; ;}
    break;

  case 97:
#line 665 "objc-parse.y"
    { tree saved_last_tree;

		   if (pedantic)
		     pedwarn ("ISO C forbids braced-groups within expressions");
		  pop_label_level ();

		  saved_last_tree = COMPOUND_BODY (yyvsp[-2].ttype);
		  RECHAIN_STMTS (yyvsp[-2].ttype, COMPOUND_BODY (yyvsp[-2].ttype));
		  last_tree = saved_last_tree;
		  TREE_CHAIN (last_tree) = NULL_TREE;
		  if (!last_expr_type)
		    last_expr_type = void_type_node;
		  yyval.ttype = build1 (STMT_EXPR, last_expr_type, yyvsp[-2].ttype);
		  TREE_SIDE_EFFECTS (yyval.ttype) = 1;
		;}
    break;

  case 98:
#line 681 "objc-parse.y"
    {
		  pop_label_level ();
		  last_tree = COMPOUND_BODY (yyvsp[-2].ttype);
		  TREE_CHAIN (last_tree) = NULL_TREE;
		  yyval.ttype = error_mark_node;
		;}
    break;

  case 99:
#line 688 "objc-parse.y"
    { yyval.ttype = build_function_call (yyvsp[-3].ttype, yyvsp[-1].ttype); ;}
    break;

  case 100:
#line 690 "objc-parse.y"
    { yyval.ttype = build_va_arg (yyvsp[-3].ttype, groktypename (yyvsp[-1].ttype)); ;}
    break;

  case 101:
#line 693 "objc-parse.y"
    {
                  tree c;

                  c = fold (yyvsp[-5].ttype);
                  STRIP_NOPS (c);
                  if (TREE_CODE (c) != INTEGER_CST)
                    error ("first argument to __builtin_choose_expr not a constant");
                  yyval.ttype = integer_zerop (c) ? yyvsp[-1].ttype : yyvsp[-3].ttype;
		;}
    break;

  case 102:
#line 703 "objc-parse.y"
    {
		  tree e1, e2;

		  e1 = TYPE_MAIN_VARIANT (groktypename (yyvsp[-3].ttype));
		  e2 = TYPE_MAIN_VARIANT (groktypename (yyvsp[-1].ttype));

		  yyval.ttype = comptypes (e1, e2)
		    ? build_int_2 (1, 0) : build_int_2 (0, 0);
		;}
    break;

  case 103:
#line 713 "objc-parse.y"
    { yyval.ttype = build_array_ref (yyvsp[-3].ttype, yyvsp[-1].ttype); ;}
    break;

  case 104:
#line 715 "objc-parse.y"
    {
		    if (!is_public (yyvsp[-2].ttype, yyvsp[0].ttype))
		      yyval.ttype = error_mark_node;
		    else
		      yyval.ttype = build_component_ref (yyvsp[-2].ttype, yyvsp[0].ttype);
		;}
    break;

  case 105:
#line 722 "objc-parse.y"
    {
                  tree expr = build_indirect_ref (yyvsp[-2].ttype, "->");

		      if (!is_public (expr, yyvsp[0].ttype))
			yyval.ttype = error_mark_node;
		      else
			yyval.ttype = build_component_ref (expr, yyvsp[0].ttype);
		;}
    break;

  case 106:
#line 731 "objc-parse.y"
    { yyval.ttype = build_unary_op (POSTINCREMENT_EXPR, yyvsp[-1].ttype, 0); ;}
    break;

  case 107:
#line 733 "objc-parse.y"
    { yyval.ttype = build_unary_op (POSTDECREMENT_EXPR, yyvsp[-1].ttype, 0); ;}
    break;

  case 108:
#line 735 "objc-parse.y"
    { yyval.ttype = build_message_expr (yyvsp[0].ttype); ;}
    break;

  case 109:
#line 737 "objc-parse.y"
    { yyval.ttype = build_selector_expr (yyvsp[0].ttype); ;}
    break;

  case 110:
#line 739 "objc-parse.y"
    { yyval.ttype = build_protocol_expr (yyvsp[0].ttype); ;}
    break;

  case 111:
#line 741 "objc-parse.y"
    { yyval.ttype = build_encode_expr (yyvsp[0].ttype); ;}
    break;

  case 112:
#line 743 "objc-parse.y"
    { yyval.ttype = build_objc_string_object (yyvsp[0].ttype); ;}
    break;

  case 113:
#line 750 "objc-parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 114:
#line 752 "objc-parse.y"
    { yyval.ttype = chainon (yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 115:
#line 757 "objc-parse.y"
    {
	  parsing_iso_function_signature = false; /* Reset after decls.  */
	;}
    break;

  case 116:
#line 764 "objc-parse.y"
    {
	  if (warn_traditional && !in_system_header
	      && parsing_iso_function_signature)
	    warning ("traditional C rejects ISO C style function definitions");
	  parsing_iso_function_signature = false; /* Reset after warning.  */
	;}
    break;

  case 118:
#line 778 "objc-parse.y"
    { ;}
    break;

  case 123:
#line 794 "objc-parse.y"
    { POP_DECLSPEC_STACK; ;}
    break;

  case 124:
#line 796 "objc-parse.y"
    { POP_DECLSPEC_STACK; ;}
    break;

  case 125:
#line 798 "objc-parse.y"
    { shadow_tag_warned (yyvsp[-1].ttype, 1);
		  pedwarn ("empty declaration"); ;}
    break;

  case 126:
#line 801 "objc-parse.y"
    { pedwarn ("empty declaration"); ;}
    break;

  case 127:
#line 810 "objc-parse.y"
    { ;}
    break;

  case 128:
#line 818 "objc-parse.y"
    { pending_xref_error ();
		  PUSH_DECLSPEC_STACK;
		  split_specs_attrs (yyvsp[0].ttype,
				     &current_declspecs, &prefix_attributes);
		  all_prefix_attributes = prefix_attributes; ;}
    break;

  case 129:
#line 829 "objc-parse.y"
    { all_prefix_attributes = chainon (yyvsp[0].ttype, prefix_attributes); ;}
    break;

  case 130:
#line 834 "objc-parse.y"
    { POP_DECLSPEC_STACK; ;}
    break;

  case 131:
#line 836 "objc-parse.y"
    { POP_DECLSPEC_STACK; ;}
    break;

  case 132:
#line 838 "objc-parse.y"
    { POP_DECLSPEC_STACK; ;}
    break;

  case 133:
#line 840 "objc-parse.y"
    { POP_DECLSPEC_STACK; ;}
    break;

  case 134:
#line 842 "objc-parse.y"
    { shadow_tag (yyvsp[-1].ttype); ;}
    break;

  case 135:
#line 844 "objc-parse.y"
    { RESTORE_EXT_FLAGS (yyvsp[-1].ttype); ;}
    break;

  case 136:
#line 901 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, NULL_TREE);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 137:
#line 904 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 138:
#line 907 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 139:
#line 913 "objc-parse.y"
    { yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 140:
#line 919 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 141:
#line 922 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 142:
#line 928 "objc-parse.y"
    { yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, NULL_TREE);
		  TREE_STATIC (yyval.ttype) = 0; ;}
    break;

  case 143:
#line 931 "objc-parse.y"
    { yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 144:
#line 937 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, NULL_TREE);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 145:
#line 940 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 146:
#line 943 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 147:
#line 946 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 148:
#line 949 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 149:
#line 952 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 150:
#line 955 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 151:
#line 961 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, NULL_TREE);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 152:
#line 964 "objc-parse.y"
    { yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 153:
#line 967 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 154:
#line 970 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 155:
#line 973 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 156:
#line 976 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 157:
#line 982 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 158:
#line 985 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 159:
#line 988 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 160:
#line 991 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 161:
#line 994 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 162:
#line 997 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 163:
#line 1003 "objc-parse.y"
    { yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 164:
#line 1006 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 165:
#line 1009 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 166:
#line 1012 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 167:
#line 1015 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 168:
#line 1021 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, NULL_TREE);
		  TREE_STATIC (yyval.ttype) = 0; ;}
    break;

  case 169:
#line 1024 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 170:
#line 1027 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 171:
#line 1030 "objc-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 172:
#line 1036 "objc-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 173:
#line 1042 "objc-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 174:
#line 1048 "objc-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 175:
#line 1057 "objc-parse.y"
    { yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 176:
#line 1063 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 177:
#line 1066 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 178:
#line 1069 "objc-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 179:
#line 1075 "objc-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 180:
#line 1081 "objc-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 181:
#line 1087 "objc-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 182:
#line 1096 "objc-parse.y"
    { yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 183:
#line 1102 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 184:
#line 1105 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 185:
#line 1108 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 186:
#line 1111 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 187:
#line 1114 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 188:
#line 1117 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 189:
#line 1120 "objc-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 190:
#line 1126 "objc-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 191:
#line 1132 "objc-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 192:
#line 1138 "objc-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 193:
#line 1147 "objc-parse.y"
    { yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 194:
#line 1150 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 195:
#line 1153 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 196:
#line 1156 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 197:
#line 1159 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 198:
#line 1165 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 199:
#line 1168 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 200:
#line 1171 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 201:
#line 1174 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 202:
#line 1177 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 203:
#line 1180 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 204:
#line 1183 "objc-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 205:
#line 1189 "objc-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 206:
#line 1195 "objc-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 207:
#line 1201 "objc-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 208:
#line 1210 "objc-parse.y"
    { yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 209:
#line 1213 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 210:
#line 1216 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 211:
#line 1219 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 212:
#line 1222 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 269:
#line 1310 "objc-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 270:
#line 1312 "objc-parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 274:
#line 1347 "objc-parse.y"
    { OBJC_NEED_RAW_IDENTIFIER (1);	;}
    break;

  case 277:
#line 1357 "objc-parse.y"
    { /* For a typedef name, record the meaning, not the name.
		     In case of `foo foo, bar;'.  */
		  yyval.ttype = lookup_name (yyvsp[0].ttype); ;}
    break;

  case 278:
#line 1361 "objc-parse.y"
    { yyval.ttype = get_static_reference (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 279:
#line 1363 "objc-parse.y"
    { yyval.ttype = get_object_reference (yyvsp[0].ttype); ;}
    break;

  case 280:
#line 1368 "objc-parse.y"
    { yyval.ttype = get_object_reference (yyvsp[0].ttype); ;}
    break;

  case 281:
#line 1370 "objc-parse.y"
    { skip_evaluation--; yyval.ttype = TREE_TYPE (yyvsp[-1].ttype); ;}
    break;

  case 282:
#line 1372 "objc-parse.y"
    { skip_evaluation--; yyval.ttype = groktypename (yyvsp[-1].ttype); ;}
    break;

  case 287:
#line 1389 "objc-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 288:
#line 1391 "objc-parse.y"
    { yyval.ttype = yyvsp[-1].ttype; ;}
    break;

  case 289:
#line 1396 "objc-parse.y"
    { yyval.ttype = start_decl (yyvsp[-3].ttype, current_declspecs, 1,
					  chainon (yyvsp[-1].ttype, all_prefix_attributes));
		  start_init (yyval.ttype, yyvsp[-2].ttype, global_bindings_p ()); ;}
    break;

  case 290:
#line 1401 "objc-parse.y"
    { finish_init ();
		  finish_decl (yyvsp[-1].ttype, yyvsp[0].ttype, yyvsp[-4].ttype); ;}
    break;

  case 291:
#line 1404 "objc-parse.y"
    { tree d = start_decl (yyvsp[-2].ttype, current_declspecs, 0,
				       chainon (yyvsp[0].ttype, all_prefix_attributes));
		  finish_decl (d, NULL_TREE, yyvsp[-1].ttype);
                ;}
    break;

  case 292:
#line 1412 "objc-parse.y"
    { yyval.ttype = start_decl (yyvsp[-3].ttype, current_declspecs, 1,
					  chainon (yyvsp[-1].ttype, all_prefix_attributes));
		  start_init (yyval.ttype, yyvsp[-2].ttype, global_bindings_p ()); ;}
    break;

  case 293:
#line 1417 "objc-parse.y"
    { finish_init ();
		  finish_decl (yyvsp[-1].ttype, yyvsp[0].ttype, yyvsp[-4].ttype); ;}
    break;

  case 294:
#line 1420 "objc-parse.y"
    { tree d = start_decl (yyvsp[-2].ttype, current_declspecs, 0,
				       chainon (yyvsp[0].ttype, all_prefix_attributes));
		  finish_decl (d, NULL_TREE, yyvsp[-1].ttype); ;}
    break;

  case 295:
#line 1428 "objc-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 296:
#line 1430 "objc-parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 297:
#line 1435 "objc-parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 298:
#line 1437 "objc-parse.y"
    { yyval.ttype = chainon (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 299:
#line 1442 "objc-parse.y"
    { yyval.ttype = yyvsp[-2].ttype; ;}
    break;

  case 300:
#line 1447 "objc-parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 301:
#line 1449 "objc-parse.y"
    { yyval.ttype = chainon (yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 302:
#line 1454 "objc-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 303:
#line 1456 "objc-parse.y"
    { yyval.ttype = build_tree_list (yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 304:
#line 1458 "objc-parse.y"
    { yyval.ttype = build_tree_list (yyvsp[-3].ttype, build_tree_list (NULL_TREE, yyvsp[-1].ttype)); ;}
    break;

  case 305:
#line 1460 "objc-parse.y"
    { yyval.ttype = build_tree_list (yyvsp[-5].ttype, tree_cons (NULL_TREE, yyvsp[-3].ttype, yyvsp[-1].ttype)); ;}
    break;

  case 306:
#line 1462 "objc-parse.y"
    { yyval.ttype = build_tree_list (yyvsp[-3].ttype, yyvsp[-1].ttype); ;}
    break;

  case 314:
#line 1485 "objc-parse.y"
    { really_start_incremental_init (NULL_TREE); ;}
    break;

  case 315:
#line 1487 "objc-parse.y"
    { yyval.ttype = pop_init_level (0); ;}
    break;

  case 316:
#line 1489 "objc-parse.y"
    { yyval.ttype = error_mark_node; ;}
    break;

  case 317:
#line 1495 "objc-parse.y"
    { if (pedantic)
		    pedwarn ("ISO C forbids empty initializer braces"); ;}
    break;

  case 321:
#line 1509 "objc-parse.y"
    { if (pedantic && ! flag_isoc99)
		    pedwarn ("ISO C89 forbids specifying subobject to initialize"); ;}
    break;

  case 322:
#line 1512 "objc-parse.y"
    { if (pedantic)
		    pedwarn ("obsolete use of designated initializer without `='"); ;}
    break;

  case 323:
#line 1515 "objc-parse.y"
    { set_init_label (yyvsp[-1].ttype);
		  if (pedantic)
		    pedwarn ("obsolete use of designated initializer with `:'"); ;}
    break;

  case 324:
#line 1519 "objc-parse.y"
    {;}
    break;

  case 326:
#line 1525 "objc-parse.y"
    { push_init_level (0); ;}
    break;

  case 327:
#line 1527 "objc-parse.y"
    { process_init_element (pop_init_level (0)); ;}
    break;

  case 328:
#line 1529 "objc-parse.y"
    { process_init_element (yyvsp[0].ttype); ;}
    break;

  case 332:
#line 1540 "objc-parse.y"
    { set_init_label (yyvsp[0].ttype); ;}
    break;

  case 333:
#line 1542 "objc-parse.y"
    { set_init_index (yyvsp[-3].ttype, yyvsp[-1].ttype);
		  if (pedantic)
		    pedwarn ("ISO C forbids specifying range of elements to initialize"); ;}
    break;

  case 334:
#line 1546 "objc-parse.y"
    { set_init_index (yyvsp[-1].ttype, NULL_TREE); ;}
    break;

  case 335:
#line 1551 "objc-parse.y"
    { if (pedantic)
		    pedwarn ("ISO C forbids nested functions");

		  push_function_context ();
		  if (! start_function (current_declspecs, yyvsp[0].ttype,
					all_prefix_attributes))
		    {
		      pop_function_context ();
		      YYERROR1;
		    }
		  parsing_iso_function_signature = false; /* Don't warn about nested functions.  */
		;}
    break;

  case 336:
#line 1564 "objc-parse.y"
    { store_parm_decls (); ;}
    break;

  case 337:
#line 1572 "objc-parse.y"
    { tree decl = current_function_decl;
		  DECL_SOURCE_FILE (decl) = yyvsp[-2].filename;
		  DECL_SOURCE_LINE (decl) = yyvsp[-1].lineno;
		  finish_function (1, 1);
		  pop_function_context ();
		  add_decl_stmt (decl); ;}
    break;

  case 338:
#line 1582 "objc-parse.y"
    { if (pedantic)
		    pedwarn ("ISO C forbids nested functions");

		  push_function_context ();
		  if (! start_function (current_declspecs, yyvsp[0].ttype,
					all_prefix_attributes))
		    {
		      pop_function_context ();
		      YYERROR1;
		    }
		  parsing_iso_function_signature = false; /* Don't warn about nested functions.  */
		;}
    break;

  case 339:
#line 1595 "objc-parse.y"
    { store_parm_decls (); ;}
    break;

  case 340:
#line 1603 "objc-parse.y"
    { tree decl = current_function_decl;
		  DECL_SOURCE_FILE (decl) = yyvsp[-2].filename;
		  DECL_SOURCE_LINE (decl) = yyvsp[-1].lineno;
		  finish_function (1, 1);
		  pop_function_context ();
		  add_decl_stmt (decl); ;}
    break;

  case 343:
#line 1623 "objc-parse.y"
    { yyval.ttype = yyvsp[-2].ttype ? tree_cons (yyvsp[-2].ttype, yyvsp[-1].ttype, NULL_TREE) : yyvsp[-1].ttype; ;}
    break;

  case 344:
#line 1625 "objc-parse.y"
    { yyval.ttype = build_nt (CALL_EXPR, yyvsp[-2].ttype, yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 345:
#line 1630 "objc-parse.y"
    { yyval.ttype = set_array_declarator_type (yyvsp[0].ttype, yyvsp[-1].ttype, 0); ;}
    break;

  case 346:
#line 1632 "objc-parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 351:
#line 1648 "objc-parse.y"
    { yyval.ttype = build_nt (CALL_EXPR, yyvsp[-2].ttype, yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 352:
#line 1653 "objc-parse.y"
    { yyval.ttype = set_array_declarator_type (yyvsp[0].ttype, yyvsp[-1].ttype, 0); ;}
    break;

  case 355:
#line 1660 "objc-parse.y"
    { yyval.ttype = build_nt (CALL_EXPR, yyvsp[-2].ttype, yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 356:
#line 1665 "objc-parse.y"
    { yyval.ttype = set_array_declarator_type (yyvsp[0].ttype, yyvsp[-1].ttype, 0); ;}
    break;

  case 357:
#line 1667 "objc-parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 358:
#line 1669 "objc-parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 359:
#line 1671 "objc-parse.y"
    { yyval.ttype = yyvsp[-2].ttype ? tree_cons (yyvsp[-2].ttype, yyvsp[-1].ttype, NULL_TREE) : yyvsp[-1].ttype; ;}
    break;

  case 360:
#line 1679 "objc-parse.y"
    { yyval.ttype = build_nt (CALL_EXPR, yyvsp[-2].ttype, yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 361:
#line 1684 "objc-parse.y"
    { yyval.ttype = yyvsp[-2].ttype ? tree_cons (yyvsp[-2].ttype, yyvsp[-1].ttype, NULL_TREE) : yyvsp[-1].ttype; ;}
    break;

  case 362:
#line 1686 "objc-parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 363:
#line 1688 "objc-parse.y"
    { yyval.ttype = set_array_declarator_type (yyvsp[0].ttype, yyvsp[-1].ttype, 0); ;}
    break;

  case 365:
#line 1694 "objc-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 366:
#line 1696 "objc-parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 367:
#line 1701 "objc-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 368:
#line 1703 "objc-parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 369:
#line 1708 "objc-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 370:
#line 1710 "objc-parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 371:
#line 1721 "objc-parse.y"
    { yyval.ttype = start_struct (RECORD_TYPE, yyvsp[-1].ttype);
		  /* Start scope of tag before parsing components.  */
		;}
    break;

  case 372:
#line 1725 "objc-parse.y"
    { yyval.ttype = finish_struct (yyvsp[-3].ttype, yyvsp[-2].ttype, chainon (yyvsp[-6].ttype, yyvsp[0].ttype)); ;}
    break;

  case 373:
#line 1727 "objc-parse.y"
    { yyval.ttype = finish_struct (start_struct (RECORD_TYPE, NULL_TREE),
				      yyvsp[-2].ttype, chainon (yyvsp[-4].ttype, yyvsp[0].ttype));
		;}
    break;

  case 374:
#line 1731 "objc-parse.y"
    { yyval.ttype = start_struct (UNION_TYPE, yyvsp[-1].ttype); ;}
    break;

  case 375:
#line 1733 "objc-parse.y"
    { yyval.ttype = finish_struct (yyvsp[-3].ttype, yyvsp[-2].ttype, chainon (yyvsp[-6].ttype, yyvsp[0].ttype)); ;}
    break;

  case 376:
#line 1735 "objc-parse.y"
    { yyval.ttype = finish_struct (start_struct (UNION_TYPE, NULL_TREE),
				      yyvsp[-2].ttype, chainon (yyvsp[-4].ttype, yyvsp[0].ttype));
		;}
    break;

  case 377:
#line 1739 "objc-parse.y"
    { yyval.ttype = start_enum (yyvsp[-1].ttype); ;}
    break;

  case 378:
#line 1741 "objc-parse.y"
    { yyval.ttype = finish_enum (yyvsp[-4].ttype, nreverse (yyvsp[-3].ttype),
				    chainon (yyvsp[-7].ttype, yyvsp[0].ttype)); ;}
    break;

  case 379:
#line 1744 "objc-parse.y"
    { yyval.ttype = start_enum (NULL_TREE); ;}
    break;

  case 380:
#line 1746 "objc-parse.y"
    { yyval.ttype = finish_enum (yyvsp[-4].ttype, nreverse (yyvsp[-3].ttype),
				    chainon (yyvsp[-6].ttype, yyvsp[0].ttype)); ;}
    break;

  case 381:
#line 1752 "objc-parse.y"
    { yyval.ttype = xref_tag (RECORD_TYPE, yyvsp[0].ttype); ;}
    break;

  case 382:
#line 1754 "objc-parse.y"
    { yyval.ttype = xref_tag (UNION_TYPE, yyvsp[0].ttype); ;}
    break;

  case 383:
#line 1756 "objc-parse.y"
    { yyval.ttype = xref_tag (ENUMERAL_TYPE, yyvsp[0].ttype);
		  /* In ISO C, enumerated types can be referred to
		     only if already defined.  */
		  if (pedantic && !COMPLETE_TYPE_P (yyval.ttype))
		    pedwarn ("ISO C forbids forward references to `enum' types"); ;}
    break;

  case 387:
#line 1771 "objc-parse.y"
    { if (pedantic && ! flag_isoc99)
		    pedwarn ("comma at end of enumerator list"); ;}
    break;

  case 388:
#line 1777 "objc-parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 389:
#line 1779 "objc-parse.y"
    { yyval.ttype = chainon (yyvsp[-1].ttype, yyvsp[0].ttype);
		  pedwarn ("no semicolon at end of struct or union"); ;}
    break;

  case 390:
#line 1784 "objc-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 391:
#line 1786 "objc-parse.y"
    { yyval.ttype = chainon (yyvsp[-2].ttype, yyvsp[-1].ttype); ;}
    break;

  case 392:
#line 1788 "objc-parse.y"
    { if (pedantic)
		    pedwarn ("extra semicolon in struct or union specified"); ;}
    break;

  case 393:
#line 1792 "objc-parse.y"
    {
		  tree interface = lookup_interface (yyvsp[-1].ttype);

		  if (interface)
		    yyval.ttype = get_class_ivars (interface);
		  else
		    {
		      error ("cannot find interface declaration for `%s'",
			     IDENTIFIER_POINTER (yyvsp[-1].ttype));
		      yyval.ttype = NULL_TREE;
		    }
		;}
    break;

  case 394:
#line 1808 "objc-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  POP_DECLSPEC_STACK; ;}
    break;

  case 395:
#line 1811 "objc-parse.y"
    {
		  /* Support for unnamed structs or unions as members of
		     structs or unions (which is [a] useful and [b] supports
		     MS P-SDK).  */
		  if (pedantic)
		    pedwarn ("ISO C doesn't support unnamed structs/unions");

		  yyval.ttype = grokfield(yyvsp[-1].filename, yyvsp[0].lineno, NULL, current_declspecs, NULL_TREE);
		  POP_DECLSPEC_STACK; ;}
    break;

  case 396:
#line 1821 "objc-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  POP_DECLSPEC_STACK; ;}
    break;

  case 397:
#line 1824 "objc-parse.y"
    { if (pedantic)
		    pedwarn ("ISO C forbids member declarations with no members");
		  shadow_tag(yyvsp[0].ttype);
		  yyval.ttype = NULL_TREE; ;}
    break;

  case 398:
#line 1829 "objc-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 399:
#line 1831 "objc-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  RESTORE_EXT_FLAGS (yyvsp[-1].ttype); ;}
    break;

  case 401:
#line 1838 "objc-parse.y"
    { yyval.ttype = chainon (yyvsp[-3].ttype, yyvsp[0].ttype); ;}
    break;

  case 403:
#line 1844 "objc-parse.y"
    { yyval.ttype = chainon (yyvsp[-3].ttype, yyvsp[0].ttype); ;}
    break;

  case 404:
#line 1849 "objc-parse.y"
    { yyval.ttype = grokfield (yyvsp[-3].filename, yyvsp[-2].lineno, yyvsp[-1].ttype, current_declspecs, NULL_TREE);
		  decl_attributes (&yyval.ttype, chainon (yyvsp[0].ttype, all_prefix_attributes), 0); ;}
    break;

  case 405:
#line 1853 "objc-parse.y"
    { yyval.ttype = grokfield (yyvsp[-5].filename, yyvsp[-4].lineno, yyvsp[-3].ttype, current_declspecs, yyvsp[-1].ttype);
		  decl_attributes (&yyval.ttype, chainon (yyvsp[0].ttype, all_prefix_attributes), 0); ;}
    break;

  case 406:
#line 1856 "objc-parse.y"
    { yyval.ttype = grokfield (yyvsp[-4].filename, yyvsp[-3].lineno, NULL_TREE, current_declspecs, yyvsp[-1].ttype);
		  decl_attributes (&yyval.ttype, chainon (yyvsp[0].ttype, all_prefix_attributes), 0); ;}
    break;

  case 407:
#line 1862 "objc-parse.y"
    { yyval.ttype = grokfield (yyvsp[-3].filename, yyvsp[-2].lineno, yyvsp[-1].ttype, current_declspecs, NULL_TREE);
		  decl_attributes (&yyval.ttype, chainon (yyvsp[0].ttype, all_prefix_attributes), 0); ;}
    break;

  case 408:
#line 1866 "objc-parse.y"
    { yyval.ttype = grokfield (yyvsp[-5].filename, yyvsp[-4].lineno, yyvsp[-3].ttype, current_declspecs, yyvsp[-1].ttype);
		  decl_attributes (&yyval.ttype, chainon (yyvsp[0].ttype, all_prefix_attributes), 0); ;}
    break;

  case 409:
#line 1869 "objc-parse.y"
    { yyval.ttype = grokfield (yyvsp[-4].filename, yyvsp[-3].lineno, NULL_TREE, current_declspecs, yyvsp[-1].ttype);
		  decl_attributes (&yyval.ttype, chainon (yyvsp[0].ttype, all_prefix_attributes), 0); ;}
    break;

  case 411:
#line 1881 "objc-parse.y"
    { if (yyvsp[-2].ttype == error_mark_node)
		    yyval.ttype = yyvsp[-2].ttype;
		  else
		    yyval.ttype = chainon (yyvsp[0].ttype, yyvsp[-2].ttype); ;}
    break;

  case 412:
#line 1886 "objc-parse.y"
    { yyval.ttype = error_mark_node; ;}
    break;

  case 413:
#line 1892 "objc-parse.y"
    { yyval.ttype = build_enumerator (yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 414:
#line 1894 "objc-parse.y"
    { yyval.ttype = build_enumerator (yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 415:
#line 1899 "objc-parse.y"
    { pending_xref_error ();
		  yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 416:
#line 1902 "objc-parse.y"
    { yyval.ttype = build_tree_list (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 417:
#line 1907 "objc-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 419:
#line 1913 "objc-parse.y"
    { yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 NULL_TREE),
					all_prefix_attributes); ;}
    break;

  case 420:
#line 1917 "objc-parse.y"
    { yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 yyvsp[0].ttype),
					all_prefix_attributes); ;}
    break;

  case 421:
#line 1921 "objc-parse.y"
    { yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 yyvsp[-1].ttype),
					chainon (yyvsp[0].ttype, all_prefix_attributes)); ;}
    break;

  case 425:
#line 1934 "objc-parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 426:
#line 1939 "objc-parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 427:
#line 1941 "objc-parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 428:
#line 1946 "objc-parse.y"
    { yyval.ttype = yyvsp[-2].ttype ? tree_cons (yyvsp[-2].ttype, yyvsp[-1].ttype, NULL_TREE) : yyvsp[-1].ttype; ;}
    break;

  case 429:
#line 1948 "objc-parse.y"
    { yyval.ttype = build_nt (CALL_EXPR, yyvsp[-2].ttype, yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 430:
#line 1950 "objc-parse.y"
    { yyval.ttype = set_array_declarator_type (yyvsp[0].ttype, yyvsp[-1].ttype, 1); ;}
    break;

  case 431:
#line 1952 "objc-parse.y"
    { yyval.ttype = build_nt (CALL_EXPR, NULL_TREE, yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 432:
#line 1954 "objc-parse.y"
    { yyval.ttype = set_array_declarator_type (yyvsp[0].ttype, NULL_TREE, 1); ;}
    break;

  case 433:
#line 1961 "objc-parse.y"
    { yyval.ttype = build_array_declarator (yyvsp[-1].ttype, yyvsp[-2].ttype, 0, 0); ;}
    break;

  case 434:
#line 1963 "objc-parse.y"
    { yyval.ttype = build_array_declarator (NULL_TREE, yyvsp[-1].ttype, 0, 0); ;}
    break;

  case 435:
#line 1965 "objc-parse.y"
    { yyval.ttype = build_array_declarator (NULL_TREE, yyvsp[-2].ttype, 0, 1); ;}
    break;

  case 436:
#line 1967 "objc-parse.y"
    { yyval.ttype = build_array_declarator (yyvsp[-1].ttype, yyvsp[-2].ttype, 1, 0); ;}
    break;

  case 437:
#line 1970 "objc-parse.y"
    { yyval.ttype = build_array_declarator (yyvsp[-1].ttype, yyvsp[-3].ttype, 1, 0); ;}
    break;

  case 440:
#line 1983 "objc-parse.y"
    {
		  pedwarn ("deprecated use of label at end of compound statement");
		;}
    break;

  case 448:
#line 2000 "objc-parse.y"
    { if (pedantic && !flag_isoc99)
		    pedwarn ("ISO C89 forbids mixed declarations and code"); ;}
    break;

  case 463:
#line 2030 "objc-parse.y"
    { pushlevel (0);
		  clear_last_expr ();
		  add_scope_stmt (/*begin_p=*/1, /*partial_p=*/0);
		  if (objc_method_context)
		    add_objc_decls ();
		;}
    break;

  case 464:
#line 2039 "objc-parse.y"
    { yyval.ttype = add_scope_stmt (/*begin_p=*/0, /*partial_p=*/0); ;}
    break;

  case 465:
#line 2044 "objc-parse.y"
    { if (flag_isoc99)
		    {
		      yyval.ttype = c_begin_compound_stmt ();
		      pushlevel (0);
		      clear_last_expr ();
		      add_scope_stmt (/*begin_p=*/1, /*partial_p=*/0);
		      if (objc_method_context)
			add_objc_decls ();
		    }
		  else
		    yyval.ttype = NULL_TREE;
		;}
    break;

  case 466:
#line 2062 "objc-parse.y"
    { if (flag_isoc99)
		    {
		      tree scope_stmt = add_scope_stmt (/*begin_p=*/0, /*partial_p=*/0);
		      yyval.ttype = poplevel (kept_level_p (), 0, 0);
		      SCOPE_STMT_BLOCK (TREE_PURPOSE (scope_stmt))
			= SCOPE_STMT_BLOCK (TREE_VALUE (scope_stmt))
			= yyval.ttype;
		    }
		  else
		    yyval.ttype = NULL_TREE; ;}
    break;

  case 468:
#line 2079 "objc-parse.y"
    { if (pedantic)
		    pedwarn ("ISO C forbids label declarations"); ;}
    break;

  case 471:
#line 2090 "objc-parse.y"
    { tree link;
		  for (link = yyvsp[-1].ttype; link; link = TREE_CHAIN (link))
		    {
		      tree label = shadow_label (TREE_VALUE (link));
		      C_DECLARED_LABEL_FLAG (label) = 1;
		      add_decl_stmt (label);
		    }
		;}
    break;

  case 472:
#line 2104 "objc-parse.y"
    {;}
    break;

  case 474:
#line 2108 "objc-parse.y"
    { compstmt_count++;
                      yyval.ttype = c_begin_compound_stmt (); ;}
    break;

  case 475:
#line 2113 "objc-parse.y"
    { yyval.ttype = convert (void_type_node, integer_zero_node); ;}
    break;

  case 476:
#line 2115 "objc-parse.y"
    { yyval.ttype = poplevel (kept_level_p (), 1, 0);
		  SCOPE_STMT_BLOCK (TREE_PURPOSE (yyvsp[0].ttype))
		    = SCOPE_STMT_BLOCK (TREE_VALUE (yyvsp[0].ttype))
		    = yyval.ttype; ;}
    break;

  case 479:
#line 2128 "objc-parse.y"
    { if (current_function_decl == 0)
		    {
		      error ("braced-group within expression allowed only inside a function");
		      YYERROR;
		    }
		  /* We must force a BLOCK for this level
		     so that, if it is not expanded later,
		     there is a way to turn off the entire subtree of blocks
		     that are contained in it.  */
		  keep_next_level ();
		  push_label_level ();
		  compstmt_count++;
		  yyval.ttype = add_stmt (build_stmt (COMPOUND_STMT, last_tree));
		  last_expr_type = NULL_TREE;
		;}
    break;

  case 480:
#line 2146 "objc-parse.y"
    { RECHAIN_STMTS (yyvsp[-1].ttype, COMPOUND_BODY (yyvsp[-1].ttype));
		  last_expr_type = NULL_TREE;
                  yyval.ttype = yyvsp[-1].ttype; ;}
    break;

  case 481:
#line 2154 "objc-parse.y"
    { c_finish_then (); ;}
    break;

  case 483:
#line 2171 "objc-parse.y"
    { yyval.ttype = c_begin_if_stmt (); ;}
    break;

  case 484:
#line 2173 "objc-parse.y"
    { c_expand_start_cond (c_common_truthvalue_conversion (yyvsp[-1].ttype),
				       compstmt_count,yyvsp[-3].ttype);
		  yyval.itype = stmt_count;
		  if_stmt_file = yyvsp[-7].filename;
		  if_stmt_line = yyvsp[-6].lineno; ;}
    break;

  case 485:
#line 2185 "objc-parse.y"
    { stmt_count++;
		  compstmt_count++;
		  yyval.ttype
		    = add_stmt (build_stmt (DO_STMT, NULL_TREE,
					    NULL_TREE));
		  /* In the event that a parse error prevents
		     parsing the complete do-statement, set the
		     condition now.  Otherwise, we can get crashes at
		     RTL-generation time.  */
		  DO_COND (yyval.ttype) = error_mark_node; ;}
    break;

  case 486:
#line 2196 "objc-parse.y"
    { yyval.ttype = yyvsp[-2].ttype;
		  RECHAIN_STMTS (yyval.ttype, DO_BODY (yyval.ttype)); ;}
    break;

  case 487:
#line 2204 "objc-parse.y"
    { if (yychar == YYEMPTY)
		    yychar = YYLEX;
		  yyval.filename = input_filename; ;}
    break;

  case 488:
#line 2210 "objc-parse.y"
    { if (yychar == YYEMPTY)
		    yychar = YYLEX;
		  yyval.lineno = lineno; ;}
    break;

  case 491:
#line 2223 "objc-parse.y"
    { if (flag_isoc99)
		    RECHAIN_STMTS (yyvsp[-2].ttype, COMPOUND_BODY (yyvsp[-2].ttype)); ;}
    break;

  case 492:
#line 2229 "objc-parse.y"
    { if (yyvsp[0].ttype)
		    {
		      STMT_LINENO (yyvsp[0].ttype) = yyvsp[-1].lineno;
		      /* ??? We currently have no way of recording
			 the filename for a statement.  This probably
			 matters little in practice at the moment,
			 but I suspect that problems will occur when
			 doing inlining at the tree level.  */
		    }
		;}
    break;

  case 493:
#line 2243 "objc-parse.y"
    { if (yyvsp[0].ttype)
		    {
		      STMT_LINENO (yyvsp[0].ttype) = yyvsp[-1].lineno;
		    }
		;}
    break;

  case 494:
#line 2252 "objc-parse.y"
    { c_expand_start_else ();
		  yyvsp[-1].itype = stmt_count; ;}
    break;

  case 495:
#line 2255 "objc-parse.y"
    { c_finish_else ();
		  c_expand_end_cond ();
		  if (extra_warnings && stmt_count == yyvsp[-3].itype)
		    warning ("empty body in an else-statement"); ;}
    break;

  case 496:
#line 2260 "objc-parse.y"
    { c_expand_end_cond ();
		  /* This warning is here instead of in simple_if, because we
		     do not want a warning if an empty if is followed by an
		     else statement.  Increment stmt_count so we don't
		     give a second error if this is a nested `if'.  */
		  if (extra_warnings && stmt_count++ == yyvsp[0].itype)
		    warning_with_file_and_line (if_stmt_file, if_stmt_line,
						"empty body in an if-statement"); ;}
    break;

  case 497:
#line 2272 "objc-parse.y"
    { c_expand_end_cond (); ;}
    break;

  case 498:
#line 2282 "objc-parse.y"
    { stmt_count++;
		  yyval.ttype = c_begin_while_stmt (); ;}
    break;

  case 499:
#line 2285 "objc-parse.y"
    { yyvsp[-1].ttype = c_common_truthvalue_conversion (yyvsp[-1].ttype);
		  c_finish_while_stmt_cond
		    (c_common_truthvalue_conversion (yyvsp[-1].ttype), yyvsp[-3].ttype);
		  yyval.ttype = add_stmt (yyvsp[-3].ttype); ;}
    break;

  case 500:
#line 2290 "objc-parse.y"
    { RECHAIN_STMTS (yyvsp[-1].ttype, WHILE_BODY (yyvsp[-1].ttype)); ;}
    break;

  case 501:
#line 2293 "objc-parse.y"
    { DO_COND (yyvsp[-4].ttype) = c_common_truthvalue_conversion (yyvsp[-2].ttype); ;}
    break;

  case 502:
#line 2295 "objc-parse.y"
    { ;}
    break;

  case 503:
#line 2297 "objc-parse.y"
    { yyval.ttype = build_stmt (FOR_STMT, NULL_TREE, NULL_TREE,
					  NULL_TREE, NULL_TREE);
		  add_stmt (yyval.ttype); ;}
    break;

  case 504:
#line 2301 "objc-parse.y"
    { stmt_count++;
		  RECHAIN_STMTS (yyvsp[-2].ttype, FOR_INIT_STMT (yyvsp[-2].ttype)); ;}
    break;

  case 505:
#line 2304 "objc-parse.y"
    { if (yyvsp[-1].ttype)
		    FOR_COND (yyvsp[-5].ttype)
		      = c_common_truthvalue_conversion (yyvsp[-1].ttype); ;}
    break;

  case 506:
#line 2308 "objc-parse.y"
    { FOR_EXPR (yyvsp[-8].ttype) = yyvsp[-1].ttype; ;}
    break;

  case 507:
#line 2310 "objc-parse.y"
    { RECHAIN_STMTS (yyvsp[-10].ttype, FOR_BODY (yyvsp[-10].ttype)); ;}
    break;

  case 508:
#line 2312 "objc-parse.y"
    { stmt_count++;
		  yyval.ttype = c_start_case (yyvsp[-1].ttype); ;}
    break;

  case 509:
#line 2315 "objc-parse.y"
    { c_finish_case (); ;}
    break;

  case 510:
#line 2320 "objc-parse.y"
    { add_stmt (build_stmt (EXPR_STMT, yyvsp[-1].ttype)); ;}
    break;

  case 511:
#line 2322 "objc-parse.y"
    { check_for_loop_decls (); ;}
    break;

  case 512:
#line 2328 "objc-parse.y"
    { stmt_count++; yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 513:
#line 2330 "objc-parse.y"
    { stmt_count++;
		  yyval.ttype = c_expand_expr_stmt (yyvsp[-1].ttype); ;}
    break;

  case 514:
#line 2333 "objc-parse.y"
    { if (flag_isoc99)
		    RECHAIN_STMTS (yyvsp[-2].ttype, COMPOUND_BODY (yyvsp[-2].ttype));
		  yyval.ttype = NULL_TREE; ;}
    break;

  case 515:
#line 2337 "objc-parse.y"
    { stmt_count++;
		  yyval.ttype = add_stmt (build_break_stmt ()); ;}
    break;

  case 516:
#line 2340 "objc-parse.y"
    { stmt_count++;
		  yyval.ttype = add_stmt (build_continue_stmt ()); ;}
    break;

  case 517:
#line 2343 "objc-parse.y"
    { stmt_count++;
		  yyval.ttype = c_expand_return (NULL_TREE); ;}
    break;

  case 518:
#line 2346 "objc-parse.y"
    { stmt_count++;
		  yyval.ttype = c_expand_return (yyvsp[-1].ttype); ;}
    break;

  case 519:
#line 2349 "objc-parse.y"
    { stmt_count++;
		  yyval.ttype = simple_asm_stmt (yyvsp[-2].ttype); ;}
    break;

  case 520:
#line 2353 "objc-parse.y"
    { stmt_count++;
		  yyval.ttype = build_asm_stmt (yyvsp[-6].ttype, yyvsp[-4].ttype, yyvsp[-2].ttype, NULL_TREE, NULL_TREE); ;}
    break;

  case 521:
#line 2358 "objc-parse.y"
    { stmt_count++;
		  yyval.ttype = build_asm_stmt (yyvsp[-8].ttype, yyvsp[-6].ttype, yyvsp[-4].ttype, yyvsp[-2].ttype, NULL_TREE); ;}
    break;

  case 522:
#line 2363 "objc-parse.y"
    { stmt_count++;
		  yyval.ttype = build_asm_stmt (yyvsp[-10].ttype, yyvsp[-8].ttype, yyvsp[-6].ttype, yyvsp[-4].ttype, yyvsp[-2].ttype); ;}
    break;

  case 523:
#line 2366 "objc-parse.y"
    { tree decl;
		  stmt_count++;
		  decl = lookup_label (yyvsp[-1].ttype);
		  if (decl != 0)
		    {
		      TREE_USED (decl) = 1;
		      yyval.ttype = add_stmt (build_stmt (GOTO_STMT, decl));
		    }
		  else
		    yyval.ttype = NULL_TREE;
		;}
    break;

  case 524:
#line 2378 "objc-parse.y"
    { if (pedantic)
		    pedwarn ("ISO C forbids `goto *expr;'");
		  stmt_count++;
		  yyvsp[-1].ttype = convert (ptr_type_node, yyvsp[-1].ttype);
		  yyval.ttype = add_stmt (build_stmt (GOTO_STMT, yyvsp[-1].ttype)); ;}
    break;

  case 525:
#line 2384 "objc-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 526:
#line 2392 "objc-parse.y"
    { stmt_count++;
		  yyval.ttype = do_case (yyvsp[-1].ttype, NULL_TREE); ;}
    break;

  case 527:
#line 2395 "objc-parse.y"
    { stmt_count++;
		  yyval.ttype = do_case (yyvsp[-3].ttype, yyvsp[-1].ttype); ;}
    break;

  case 528:
#line 2398 "objc-parse.y"
    { stmt_count++;
		  yyval.ttype = do_case (NULL_TREE, NULL_TREE); ;}
    break;

  case 529:
#line 2401 "objc-parse.y"
    { tree label = define_label (yyvsp[-3].filename, yyvsp[-2].lineno, yyvsp[-4].ttype);
		  stmt_count++;
		  if (label)
		    {
		      decl_attributes (&label, yyvsp[0].ttype, 0);
		      yyval.ttype = add_stmt (build_stmt (LABEL_STMT, label));
		    }
		  else
		    yyval.ttype = NULL_TREE;
		;}
    break;

  case 530:
#line 2417 "objc-parse.y"
    { emit_line_note (input_filename, lineno);
		  yyval.ttype = NULL_TREE; ;}
    break;

  case 531:
#line 2420 "objc-parse.y"
    { emit_line_note (input_filename, lineno); ;}
    break;

  case 532:
#line 2425 "objc-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 534:
#line 2432 "objc-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 537:
#line 2439 "objc-parse.y"
    { yyval.ttype = chainon (yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 538:
#line 2444 "objc-parse.y"
    { yyval.ttype = build_tree_list (build_tree_list (NULL_TREE, yyvsp[-3].ttype), yyvsp[-1].ttype); ;}
    break;

  case 539:
#line 2446 "objc-parse.y"
    { yyvsp[-5].ttype = build_string (IDENTIFIER_LENGTH (yyvsp[-5].ttype),
				     IDENTIFIER_POINTER (yyvsp[-5].ttype));
		  yyval.ttype = build_tree_list (build_tree_list (yyvsp[-5].ttype, yyvsp[-3].ttype), yyvsp[-1].ttype); ;}
    break;

  case 540:
#line 2453 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 541:
#line 2455 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-2].ttype); ;}
    break;

  case 542:
#line 2465 "objc-parse.y"
    { pushlevel (0);
		  clear_parm_order ();
		  declare_parm_level (0); ;}
    break;

  case 543:
#line 2469 "objc-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  parmlist_tags_warning ();
		  poplevel (0, 0, 0); ;}
    break;

  case 545:
#line 2477 "objc-parse.y"
    { tree parm;
		  if (pedantic)
		    pedwarn ("ISO C forbids forward parameter declarations");
		  /* Mark the forward decls as such.  */
		  for (parm = getdecls (); parm; parm = TREE_CHAIN (parm))
		    TREE_ASM_WRITTEN (parm) = 1;
		  clear_parm_order (); ;}
    break;

  case 546:
#line 2485 "objc-parse.y"
    { /* Dummy action so attributes are in known place
		     on parser stack.  */ ;}
    break;

  case 547:
#line 2488 "objc-parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 548:
#line 2490 "objc-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, NULL_TREE, NULL_TREE); ;}
    break;

  case 549:
#line 2496 "objc-parse.y"
    { yyval.ttype = get_parm_info (0); ;}
    break;

  case 550:
#line 2498 "objc-parse.y"
    { yyval.ttype = get_parm_info (0);
		  /* Gcc used to allow this as an extension.  However, it does
		     not work for all targets, and thus has been disabled.
		     Also, since func (...) and func () are indistinguishable,
		     it caused problems with the code in expand_builtin which
		     tries to verify that BUILT_IN_NEXT_ARG is being used
		     correctly.  */
		  error ("ISO C requires a named argument before `...'");
		;}
    break;

  case 551:
#line 2508 "objc-parse.y"
    { yyval.ttype = get_parm_info (1);
		  parsing_iso_function_signature = true;
		;}
    break;

  case 552:
#line 2512 "objc-parse.y"
    { yyval.ttype = get_parm_info (0); ;}
    break;

  case 553:
#line 2517 "objc-parse.y"
    { push_parm_decl (yyvsp[0].ttype); ;}
    break;

  case 554:
#line 2519 "objc-parse.y"
    { push_parm_decl (yyvsp[0].ttype); ;}
    break;

  case 555:
#line 2526 "objc-parse.y"
    { yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 yyvsp[-1].ttype),
					chainon (yyvsp[0].ttype, all_prefix_attributes));
		  POP_DECLSPEC_STACK; ;}
    break;

  case 556:
#line 2531 "objc-parse.y"
    { yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 yyvsp[-1].ttype),
					chainon (yyvsp[0].ttype, all_prefix_attributes));
		  POP_DECLSPEC_STACK; ;}
    break;

  case 557:
#line 2536 "objc-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  POP_DECLSPEC_STACK; ;}
    break;

  case 558:
#line 2539 "objc-parse.y"
    { yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 yyvsp[-1].ttype),
					chainon (yyvsp[0].ttype, all_prefix_attributes));
		  POP_DECLSPEC_STACK; ;}
    break;

  case 559:
#line 2545 "objc-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  POP_DECLSPEC_STACK; ;}
    break;

  case 560:
#line 2553 "objc-parse.y"
    { yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 yyvsp[-1].ttype),
					chainon (yyvsp[0].ttype, all_prefix_attributes));
		  POP_DECLSPEC_STACK; ;}
    break;

  case 561:
#line 2558 "objc-parse.y"
    { yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 yyvsp[-1].ttype),
					chainon (yyvsp[0].ttype, all_prefix_attributes));
		  POP_DECLSPEC_STACK; ;}
    break;

  case 562:
#line 2563 "objc-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  POP_DECLSPEC_STACK; ;}
    break;

  case 563:
#line 2566 "objc-parse.y"
    { yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 yyvsp[-1].ttype),
					chainon (yyvsp[0].ttype, all_prefix_attributes));
		  POP_DECLSPEC_STACK; ;}
    break;

  case 564:
#line 2572 "objc-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  POP_DECLSPEC_STACK; ;}
    break;

  case 565:
#line 2578 "objc-parse.y"
    { prefix_attributes = chainon (prefix_attributes, yyvsp[-3].ttype);
		  all_prefix_attributes = prefix_attributes; ;}
    break;

  case 566:
#line 2587 "objc-parse.y"
    { pushlevel (0);
		  clear_parm_order ();
		  declare_parm_level (1); ;}
    break;

  case 567:
#line 2591 "objc-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  parmlist_tags_warning ();
		  poplevel (0, 0, 0); ;}
    break;

  case 569:
#line 2599 "objc-parse.y"
    { tree t;
		  for (t = yyvsp[-1].ttype; t; t = TREE_CHAIN (t))
		    if (TREE_VALUE (t) == NULL_TREE)
		      error ("`...' in old-style identifier list");
		  yyval.ttype = tree_cons (NULL_TREE, NULL_TREE, yyvsp[-1].ttype);

		  /* Make sure we have a parmlist after attributes.  */
		  if (yyvsp[-3].ttype != 0
		      && (TREE_CODE (yyval.ttype) != TREE_LIST
			  || TREE_PURPOSE (yyval.ttype) == 0
			  || TREE_CODE (TREE_PURPOSE (yyval.ttype)) != PARM_DECL))
		    YYERROR1;
		;}
    break;

  case 570:
#line 2617 "objc-parse.y"
    { yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ttype); ;}
    break;

  case 571:
#line 2619 "objc-parse.y"
    { yyval.ttype = chainon (yyvsp[-2].ttype, build_tree_list (NULL_TREE, yyvsp[0].ttype)); ;}
    break;

  case 572:
#line 2625 "objc-parse.y"
    { yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ttype); ;}
    break;

  case 573:
#line 2627 "objc-parse.y"
    { yyval.ttype = chainon (yyvsp[-2].ttype, build_tree_list (NULL_TREE, yyvsp[0].ttype)); ;}
    break;

  case 574:
#line 2632 "objc-parse.y"
    { yyval.ttype = SAVE_EXT_FLAGS();
		  pedantic = 0;
		  warn_pointer_arith = 0;
		  warn_traditional = 0;
		  flag_iso = 0; ;}
    break;

  case 580:
#line 2648 "objc-parse.y"
    {
		  if (objc_implementation_context)
                    {
		      finish_class (objc_implementation_context);
		      objc_ivar_chain = NULL_TREE;
		      objc_implementation_context = NULL_TREE;
		    }
		  else
		    warning ("`@end' must appear in an implementation context");
		;}
    break;

  case 581:
#line 2663 "objc-parse.y"
    { yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ttype); ;}
    break;

  case 582:
#line 2665 "objc-parse.y"
    { yyval.ttype = chainon (yyvsp[-2].ttype, build_tree_list (NULL_TREE, yyvsp[0].ttype)); ;}
    break;

  case 583:
#line 2670 "objc-parse.y"
    {
		  objc_declare_class (yyvsp[-1].ttype);
		;}
    break;

  case 584:
#line 2677 "objc-parse.y"
    {
		  objc_declare_alias (yyvsp[-2].ttype, yyvsp[-1].ttype);
		;}
    break;

  case 585:
#line 2684 "objc-parse.y"
    {
		  objc_interface_context = objc_ivar_context
		    = start_class (CLASS_INTERFACE_TYPE, yyvsp[-2].ttype, NULL_TREE, yyvsp[-1].ttype);
                  objc_public_flag = 0;
		;}
    break;

  case 586:
#line 2690 "objc-parse.y"
    {
                  continue_class (objc_interface_context);
		;}
    break;

  case 587:
#line 2695 "objc-parse.y"
    {
		  finish_class (objc_interface_context);
		  objc_interface_context = NULL_TREE;
		;}
    break;

  case 588:
#line 2701 "objc-parse.y"
    {
		  objc_interface_context
		    = start_class (CLASS_INTERFACE_TYPE, yyvsp[-1].ttype, NULL_TREE, yyvsp[0].ttype);
                  continue_class (objc_interface_context);
		;}
    break;

  case 589:
#line 2708 "objc-parse.y"
    {
		  finish_class (objc_interface_context);
		  objc_interface_context = NULL_TREE;
		;}
    break;

  case 590:
#line 2714 "objc-parse.y"
    {
		  objc_interface_context = objc_ivar_context
		    = start_class (CLASS_INTERFACE_TYPE, yyvsp[-4].ttype, yyvsp[-2].ttype, yyvsp[-1].ttype);
                  objc_public_flag = 0;
		;}
    break;

  case 591:
#line 2720 "objc-parse.y"
    {
                  continue_class (objc_interface_context);
		;}
    break;

  case 592:
#line 2725 "objc-parse.y"
    {
		  finish_class (objc_interface_context);
		  objc_interface_context = NULL_TREE;
		;}
    break;

  case 593:
#line 2731 "objc-parse.y"
    {
		  objc_interface_context
		    = start_class (CLASS_INTERFACE_TYPE, yyvsp[-3].ttype, yyvsp[-1].ttype, yyvsp[0].ttype);
                  continue_class (objc_interface_context);
		;}
    break;

  case 594:
#line 2738 "objc-parse.y"
    {
		  finish_class (objc_interface_context);
		  objc_interface_context = NULL_TREE;
		;}
    break;

  case 595:
#line 2744 "objc-parse.y"
    {
		  objc_implementation_context = objc_ivar_context
		    = start_class (CLASS_IMPLEMENTATION_TYPE, yyvsp[-1].ttype, NULL_TREE, NULL_TREE);
                  objc_public_flag = 0;
		;}
    break;

  case 596:
#line 2750 "objc-parse.y"
    {
                  objc_ivar_chain
		    = continue_class (objc_implementation_context);
		;}
    break;

  case 597:
#line 2756 "objc-parse.y"
    {
		  objc_implementation_context
		    = start_class (CLASS_IMPLEMENTATION_TYPE, yyvsp[0].ttype, NULL_TREE, NULL_TREE);
                  objc_ivar_chain
		    = continue_class (objc_implementation_context);
		;}
    break;

  case 598:
#line 2764 "objc-parse.y"
    {
		  objc_implementation_context = objc_ivar_context
		    = start_class (CLASS_IMPLEMENTATION_TYPE, yyvsp[-3].ttype, yyvsp[-1].ttype, NULL_TREE);
                  objc_public_flag = 0;
		;}
    break;

  case 599:
#line 2770 "objc-parse.y"
    {
                  objc_ivar_chain
		    = continue_class (objc_implementation_context);
		;}
    break;

  case 600:
#line 2776 "objc-parse.y"
    {
		  objc_implementation_context
		    = start_class (CLASS_IMPLEMENTATION_TYPE, yyvsp[-2].ttype, yyvsp[0].ttype, NULL_TREE);
                  objc_ivar_chain
		    = continue_class (objc_implementation_context);
		;}
    break;

  case 601:
#line 2784 "objc-parse.y"
    {
		  objc_interface_context
		    = start_class (CATEGORY_INTERFACE_TYPE, yyvsp[-4].ttype, yyvsp[-2].ttype, yyvsp[0].ttype);
                  continue_class (objc_interface_context);
		;}
    break;

  case 602:
#line 2791 "objc-parse.y"
    {
		  finish_class (objc_interface_context);
		  objc_interface_context = NULL_TREE;
		;}
    break;

  case 603:
#line 2797 "objc-parse.y"
    {
		  objc_implementation_context
		    = start_class (CATEGORY_IMPLEMENTATION_TYPE, yyvsp[-3].ttype, yyvsp[-1].ttype, NULL_TREE);
                  objc_ivar_chain
		    = continue_class (objc_implementation_context);
		;}
    break;

  case 604:
#line 2807 "objc-parse.y"
    {
		  objc_pq_context = 1;
		  objc_interface_context
		    = start_protocol(PROTOCOL_INTERFACE_TYPE, yyvsp[-1].ttype, yyvsp[0].ttype);
		;}
    break;

  case 605:
#line 2813 "objc-parse.y"
    {
		  objc_pq_context = 0;
		  finish_protocol(objc_interface_context);
		  objc_interface_context = NULL_TREE;
		;}
    break;

  case 606:
#line 2822 "objc-parse.y"
    {
		  objc_declare_protocols (yyvsp[-1].ttype);
		;}
    break;

  case 607:
#line 2829 "objc-parse.y"
    {
		  yyval.ttype = NULL_TREE;
		;}
    break;

  case 609:
#line 2837 "objc-parse.y"
    {
		  if (yyvsp[-2].code == LT_EXPR && yyvsp[0].code == GT_EXPR)
		    yyval.ttype = yyvsp[-1].ttype;
		  else
		    YYERROR1;
		;}
    break;

  case 612:
#line 2851 "objc-parse.y"
    { objc_public_flag = 2; ;}
    break;

  case 613:
#line 2852 "objc-parse.y"
    { objc_public_flag = 0; ;}
    break;

  case 614:
#line 2853 "objc-parse.y"
    { objc_public_flag = 1; ;}
    break;

  case 615:
#line 2858 "objc-parse.y"
    {
                  yyval.ttype = NULL_TREE;
                ;}
    break;

  case 617:
#line 2863 "objc-parse.y"
    {
                  if (pedantic)
		    pedwarn ("extra semicolon in struct or union specified");
                ;}
    break;

  case 618:
#line 2881 "objc-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  POP_DECLSPEC_STACK; ;}
    break;

  case 619:
#line 2884 "objc-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  POP_DECLSPEC_STACK; ;}
    break;

  case 620:
#line 2887 "objc-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 621:
#line 2892 "objc-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 624:
#line 2899 "objc-parse.y"
    {
		  yyval.ttype = add_instance_variable (objc_ivar_context,
					      objc_public_flag,
					      yyvsp[0].ttype, current_declspecs,
					      NULL_TREE);
                ;}
    break;

  case 625:
#line 2906 "objc-parse.y"
    {
		  yyval.ttype = add_instance_variable (objc_ivar_context,
					      objc_public_flag,
					      yyvsp[-2].ttype, current_declspecs, yyvsp[0].ttype);
                ;}
    break;

  case 626:
#line 2912 "objc-parse.y"
    {
		  yyval.ttype = add_instance_variable (objc_ivar_context,
					      objc_public_flag,
					      NULL_TREE,
					      current_declspecs, yyvsp[0].ttype);
                ;}
    break;

  case 627:
#line 2922 "objc-parse.y"
    { objc_inherit_code = CLASS_METHOD_DECL; ;}
    break;

  case 628:
#line 2924 "objc-parse.y"
    { objc_inherit_code = INSTANCE_METHOD_DECL; ;}
    break;

  case 629:
#line 2929 "objc-parse.y"
    {
		  objc_pq_context = 1;
		  if (!objc_implementation_context)
		    fatal_error ("method definition not in class context");
		;}
    break;

  case 630:
#line 2935 "objc-parse.y"
    {
		  objc_pq_context = 0;
		  if (objc_inherit_code == CLASS_METHOD_DECL)
		    add_class_method (objc_implementation_context, yyvsp[0].ttype);
		  else
		    add_instance_method (objc_implementation_context, yyvsp[0].ttype);
		  start_method_def (yyvsp[0].ttype);
		;}
    break;

  case 631:
#line 2944 "objc-parse.y"
    {
		  continue_method_def ();
		;}
    break;

  case 632:
#line 2948 "objc-parse.y"
    {
		  finish_method_def ();
		;}
    break;

  case 634:
#line 2959 "objc-parse.y"
    {yyval.ttype = NULL_TREE; ;}
    break;

  case 639:
#line 2966 "objc-parse.y"
    {yyval.ttype = NULL_TREE; ;}
    break;

  case 643:
#line 2976 "objc-parse.y"
    {
		  /* Remember protocol qualifiers in prototypes.  */
		  objc_pq_context = 1;
		;}
    break;

  case 644:
#line 2981 "objc-parse.y"
    {
		  /* Forget protocol qualifiers here.  */
		  objc_pq_context = 0;
		  if (objc_inherit_code == CLASS_METHOD_DECL)
		    add_class_method (objc_interface_context, yyvsp[0].ttype);
		  else
		    add_instance_method (objc_interface_context, yyvsp[0].ttype);
		;}
    break;

  case 646:
#line 2994 "objc-parse.y"
    {
		  yyval.ttype = build_method_decl (objc_inherit_code, yyvsp[-2].ttype, yyvsp[0].ttype, NULL_TREE);
		;}
    break;

  case 647:
#line 2999 "objc-parse.y"
    {
		  yyval.ttype = build_method_decl (objc_inherit_code, NULL_TREE, yyvsp[0].ttype, NULL_TREE);
		;}
    break;

  case 648:
#line 3004 "objc-parse.y"
    {
		  yyval.ttype = build_method_decl (objc_inherit_code, yyvsp[-3].ttype, yyvsp[-1].ttype, yyvsp[0].ttype);
		;}
    break;

  case 649:
#line 3009 "objc-parse.y"
    {
		  yyval.ttype = build_method_decl (objc_inherit_code, NULL_TREE, yyvsp[-1].ttype, yyvsp[0].ttype);
		;}
    break;

  case 658:
#line 3039 "objc-parse.y"
    { POP_DECLSPEC_STACK; ;}
    break;

  case 659:
#line 3041 "objc-parse.y"
    { shadow_tag (yyvsp[-1].ttype); ;}
    break;

  case 660:
#line 3043 "objc-parse.y"
    { pedwarn ("empty declaration"); ;}
    break;

  case 661:
#line 3048 "objc-parse.y"
    { push_parm_decl (yyvsp[0].ttype); ;}
    break;

  case 662:
#line 3050 "objc-parse.y"
    { push_parm_decl (yyvsp[0].ttype); ;}
    break;

  case 663:
#line 3058 "objc-parse.y"
    { yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 yyvsp[-1].ttype),
					chainon (yyvsp[0].ttype, all_prefix_attributes)); ;}
    break;

  case 664:
#line 3062 "objc-parse.y"
    { yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 yyvsp[-1].ttype),
					chainon (yyvsp[0].ttype, all_prefix_attributes)); ;}
    break;

  case 665:
#line 3066 "objc-parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 666:
#line 3071 "objc-parse.y"
    {
	    	  yyval.ttype = NULL_TREE;
		;}
    break;

  case 667:
#line 3075 "objc-parse.y"
    {
		  /* oh what a kludge! */
		  yyval.ttype = objc_ellipsis_node;
		;}
    break;

  case 668:
#line 3080 "objc-parse.y"
    {
		  pushlevel (0);
		;}
    break;

  case 669:
#line 3084 "objc-parse.y"
    {
	  	  /* returns a tree list node generated by get_parm_info */
		  yyval.ttype = yyvsp[0].ttype;
		  poplevel (0, 0, 0);
		;}
    break;

  case 672:
#line 3099 "objc-parse.y"
    {
		  yyval.ttype = chainon (yyvsp[-1].ttype, yyvsp[0].ttype);
		;}
    break;

  case 699:
#line 3121 "objc-parse.y"
    {
		  yyval.ttype = build_keyword_decl (yyvsp[-5].ttype, yyvsp[-2].ttype, yyvsp[0].ttype);
		;}
    break;

  case 700:
#line 3126 "objc-parse.y"
    {
		  yyval.ttype = build_keyword_decl (yyvsp[-2].ttype, NULL_TREE, yyvsp[0].ttype);
		;}
    break;

  case 701:
#line 3131 "objc-parse.y"
    {
		  yyval.ttype = build_keyword_decl (NULL_TREE, yyvsp[-2].ttype, yyvsp[0].ttype);
		;}
    break;

  case 702:
#line 3136 "objc-parse.y"
    {
		  yyval.ttype = build_keyword_decl (NULL_TREE, NULL_TREE, yyvsp[0].ttype);
		;}
    break;

  case 706:
#line 3149 "objc-parse.y"
    {
		  yyval.ttype = chainon (yyvsp[-1].ttype, yyvsp[0].ttype);
		;}
    break;

  case 707:
#line 3157 "objc-parse.y"
    {
		  if (TREE_CHAIN (yyvsp[0].ttype) == NULL_TREE)
		    /* just return the expr., remove a level of indirection */
		    yyval.ttype = TREE_VALUE (yyvsp[0].ttype);
                  else
		    /* we have a comma expr., we will collapse later */
		    yyval.ttype = yyvsp[0].ttype;
		;}
    break;

  case 708:
#line 3169 "objc-parse.y"
    {
		  yyval.ttype = build_tree_list (yyvsp[-2].ttype, yyvsp[0].ttype);
		;}
    break;

  case 709:
#line 3173 "objc-parse.y"
    {
		  yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ttype);
		;}
    break;

  case 711:
#line 3181 "objc-parse.y"
    {
		  yyval.ttype = get_class_reference (yyvsp[0].ttype);
		;}
    break;

  case 712:
#line 3188 "objc-parse.y"
    { yyval.ttype = build_tree_list (yyvsp[-2].ttype, yyvsp[-1].ttype); ;}
    break;

  case 716:
#line 3199 "objc-parse.y"
    {
		  yyval.ttype = chainon (yyvsp[-1].ttype, yyvsp[0].ttype);
		;}
    break;

  case 717:
#line 3206 "objc-parse.y"
    {
		  yyval.ttype = build_tree_list (yyvsp[-1].ttype, NULL_TREE);
		;}
    break;

  case 718:
#line 3210 "objc-parse.y"
    {
		  yyval.ttype = build_tree_list (NULL_TREE, NULL_TREE);
		;}
    break;

  case 719:
#line 3217 "objc-parse.y"
    {
		  yyval.ttype = yyvsp[-1].ttype;
		;}
    break;

  case 720:
#line 3224 "objc-parse.y"
    {
		  yyval.ttype = yyvsp[-1].ttype;
		;}
    break;

  case 721:
#line 3233 "objc-parse.y"
    {
		  yyval.ttype = groktypename (yyvsp[-1].ttype);
		;}
    break;


    }

/* Line 991 of yacc.c.  */
#line 6583 "op19726.c"

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
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("syntax error, unexpected ") + 1;
	  yysize += yystrlen (yytname[yytype]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
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

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
	  /* Pop the error token.  */
          YYPOPSTACK;
	  /* Pop the rest of the stack.  */
	  while (yyss < yyssp)
	    {
	      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
	      yydestruct (yystos[*yyssp], yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab2;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
yyerrlab1:

  /* Suppress GCC warning that yyerrlab1 is unused when no action
     invokes YYERROR.  */
#if defined (__GNUC_MINOR__) && 2093 <= (__GNUC__ * 1000 + __GNUC_MINOR__) \
    && !defined __cplusplus
  __attribute__ ((__unused__))
#endif


  goto yyerrlab2;


/*---------------------------------------------------------------.
| yyerrlab2 -- pop states until the error token can be shifted.  |
`---------------------------------------------------------------*/
yyerrlab2:
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
      yyvsp--;
      yystate = *--yyssp;

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


#line 3238 "objc-parse.y"


/* yylex() is a thin wrapper around c_lex(), all it does is translate
   cpplib.h's token codes into yacc's token codes.  */

static enum cpp_ttype last_token;

/* The reserved keyword table.  */
struct resword
{
  const char *word;
  ENUM_BITFIELD(rid) rid : 16;
  unsigned int disable   : 16;
};

/* Disable mask.  Keywords are disabled if (reswords[i].disable & mask) is
   _true_.  */
#define D_C89	0x01	/* not in C89 */
#define D_EXT	0x02	/* GCC extension */
#define D_EXT89	0x04	/* GCC extension incorporated in C99 */
#define D_OBJC	0x08	/* Objective C only */

static const struct resword reswords[] =
{
  { "_Bool",		RID_BOOL,	0 },
  { "_Complex",		RID_COMPLEX,	0 },
  { "__FUNCTION__",	RID_FUNCTION_NAME, 0 },
  { "__PRETTY_FUNCTION__", RID_PRETTY_FUNCTION_NAME, 0 },
  { "__alignof",	RID_ALIGNOF,	0 },
  { "__alignof__",	RID_ALIGNOF,	0 },
  { "__asm",		RID_ASM,	0 },
  { "__asm__",		RID_ASM,	0 },
  { "__attribute",	RID_ATTRIBUTE,	0 },
  { "__attribute__",	RID_ATTRIBUTE,	0 },
  { "__bounded",	RID_BOUNDED,	0 },
  { "__bounded__",	RID_BOUNDED,	0 },
  { "__builtin_choose_expr", RID_CHOOSE_EXPR, 0 },
  { "__builtin_types_compatible_p", RID_TYPES_COMPATIBLE_P, 0 },
  { "__builtin_va_arg",	RID_VA_ARG,	0 },
  { "__complex",	RID_COMPLEX,	0 },
  { "__complex__",	RID_COMPLEX,	0 },
  { "__const",		RID_CONST,	0 },
  { "__const__",	RID_CONST,	0 },
  { "__extension__",	RID_EXTENSION,	0 },
  { "__func__",		RID_C99_FUNCTION_NAME, 0 },
  { "__imag",		RID_IMAGPART,	0 },
  { "__imag__",		RID_IMAGPART,	0 },
  { "__inline",		RID_INLINE,	0 },
  { "__inline__",	RID_INLINE,	0 },
  { "__label__",	RID_LABEL,	0 },
  { "__ptrbase",	RID_PTRBASE,	0 },
  { "__ptrbase__",	RID_PTRBASE,	0 },
  { "__ptrextent",	RID_PTREXTENT,	0 },
  { "__ptrextent__",	RID_PTREXTENT,	0 },
  { "__ptrvalue",	RID_PTRVALUE,	0 },
  { "__ptrvalue__",	RID_PTRVALUE,	0 },
  { "__real",		RID_REALPART,	0 },
  { "__real__",		RID_REALPART,	0 },
  { "__restrict",	RID_RESTRICT,	0 },
  { "__restrict__",	RID_RESTRICT,	0 },
  { "__signed",		RID_SIGNED,	0 },
  { "__signed__",	RID_SIGNED,	0 },
  { "__thread",		RID_THREAD,	0 },
  { "__typeof",		RID_TYPEOF,	0 },
  { "__typeof__",	RID_TYPEOF,	0 },
  { "__unbounded",	RID_UNBOUNDED,	0 },
  { "__unbounded__",	RID_UNBOUNDED,	0 },
  { "__volatile",	RID_VOLATILE,	0 },
  { "__volatile__",	RID_VOLATILE,	0 },
  { "asm",		RID_ASM,	D_EXT },
  { "auto",		RID_AUTO,	0 },
  { "break",		RID_BREAK,	0 },
  { "case",		RID_CASE,	0 },
  { "char",		RID_CHAR,	0 },
  { "const",		RID_CONST,	0 },
  { "continue",		RID_CONTINUE,	0 },
  { "default",		RID_DEFAULT,	0 },
  { "do",		RID_DO,		0 },
  { "double",		RID_DOUBLE,	0 },
  { "else",		RID_ELSE,	0 },
  { "enum",		RID_ENUM,	0 },
  { "extern",		RID_EXTERN,	0 },
  { "float",		RID_FLOAT,	0 },
  { "for",		RID_FOR,	0 },
  { "goto",		RID_GOTO,	0 },
  { "if",		RID_IF,		0 },
  { "inline",		RID_INLINE,	D_EXT89 },
  { "int",		RID_INT,	0 },
  { "long",		RID_LONG,	0 },
  { "register",		RID_REGISTER,	0 },
  { "restrict",		RID_RESTRICT,	D_C89 },
  { "return",		RID_RETURN,	0 },
  { "short",		RID_SHORT,	0 },
  { "signed",		RID_SIGNED,	0 },
  { "sizeof",		RID_SIZEOF,	0 },
  { "static",		RID_STATIC,	0 },
  { "struct",		RID_STRUCT,	0 },
  { "switch",		RID_SWITCH,	0 },
  { "typedef",		RID_TYPEDEF,	0 },
  { "typeof",		RID_TYPEOF,	D_EXT },
  { "union",		RID_UNION,	0 },
  { "unsigned",		RID_UNSIGNED,	0 },
  { "void",		RID_VOID,	0 },
  { "volatile",		RID_VOLATILE,	0 },
  { "while",		RID_WHILE,	0 },
  { "id",		RID_ID,			D_OBJC },

  /* These objc keywords are recognized only immediately after
     an '@'.  */
  { "class",		RID_AT_CLASS,		D_OBJC },
  { "compatibility_alias", RID_AT_ALIAS,	D_OBJC },
  { "defs",		RID_AT_DEFS,		D_OBJC },
  { "encode",		RID_AT_ENCODE,		D_OBJC },
  { "end",		RID_AT_END,		D_OBJC },
  { "implementation",	RID_AT_IMPLEMENTATION,	D_OBJC },
  { "interface",	RID_AT_INTERFACE,	D_OBJC },
  { "private",		RID_AT_PRIVATE,		D_OBJC },
  { "protected",	RID_AT_PROTECTED,	D_OBJC },
  { "protocol",		RID_AT_PROTOCOL,	D_OBJC },
  { "public",		RID_AT_PUBLIC,		D_OBJC },
  { "selector",		RID_AT_SELECTOR,	D_OBJC },

  /* These are recognized only in protocol-qualifier context
     (see above) */
  { "bycopy",		RID_BYCOPY,		D_OBJC },
  { "byref",		RID_BYREF,		D_OBJC },
  { "in",		RID_IN,			D_OBJC },
  { "inout",		RID_INOUT,		D_OBJC },
  { "oneway",		RID_ONEWAY,		D_OBJC },
  { "out",		RID_OUT,		D_OBJC },
};
#define N_reswords (sizeof reswords / sizeof (struct resword))

/* Table mapping from RID_* constants to yacc token numbers.
   Unfortunately we have to have entries for all the keywords in all
   three languages.  */
static const short rid_to_yy[RID_MAX] =
{
  /* RID_STATIC */	STATIC,
  /* RID_UNSIGNED */	TYPESPEC,
  /* RID_LONG */	TYPESPEC,
  /* RID_CONST */	TYPE_QUAL,
  /* RID_EXTERN */	SCSPEC,
  /* RID_REGISTER */	SCSPEC,
  /* RID_TYPEDEF */	SCSPEC,
  /* RID_SHORT */	TYPESPEC,
  /* RID_INLINE */	SCSPEC,
  /* RID_VOLATILE */	TYPE_QUAL,
  /* RID_SIGNED */	TYPESPEC,
  /* RID_AUTO */	SCSPEC,
  /* RID_RESTRICT */	TYPE_QUAL,

  /* C extensions */
  /* RID_BOUNDED */	TYPE_QUAL,
  /* RID_UNBOUNDED */	TYPE_QUAL,
  /* RID_COMPLEX */	TYPESPEC,
  /* RID_THREAD */	SCSPEC,

  /* C++ */
  /* RID_FRIEND */	0,
  /* RID_VIRTUAL */	0,
  /* RID_EXPLICIT */	0,
  /* RID_EXPORT */	0,
  /* RID_MUTABLE */	0,

  /* ObjC */
  /* RID_IN */		TYPE_QUAL,
  /* RID_OUT */		TYPE_QUAL,
  /* RID_INOUT */	TYPE_QUAL,
  /* RID_BYCOPY */	TYPE_QUAL,
  /* RID_BYREF */	TYPE_QUAL,
  /* RID_ONEWAY */	TYPE_QUAL,

  /* C */
  /* RID_INT */		TYPESPEC,
  /* RID_CHAR */	TYPESPEC,
  /* RID_FLOAT */	TYPESPEC,
  /* RID_DOUBLE */	TYPESPEC,
  /* RID_VOID */	TYPESPEC,
  /* RID_ENUM */	ENUM,
  /* RID_STRUCT */	STRUCT,
  /* RID_UNION */	UNION,
  /* RID_IF */		IF,
  /* RID_ELSE */	ELSE,
  /* RID_WHILE */	WHILE,
  /* RID_DO */		DO,
  /* RID_FOR */		FOR,
  /* RID_SWITCH */	SWITCH,
  /* RID_CASE */	CASE,
  /* RID_DEFAULT */	DEFAULT,
  /* RID_BREAK */	BREAK,
  /* RID_CONTINUE */	CONTINUE,
  /* RID_RETURN */	RETURN,
  /* RID_GOTO */	GOTO,
  /* RID_SIZEOF */	SIZEOF,

  /* C extensions */
  /* RID_ASM */		ASM_KEYWORD,
  /* RID_TYPEOF */	TYPEOF,
  /* RID_ALIGNOF */	ALIGNOF,
  /* RID_ATTRIBUTE */	ATTRIBUTE,
  /* RID_VA_ARG */	VA_ARG,
  /* RID_EXTENSION */	EXTENSION,
  /* RID_IMAGPART */	IMAGPART,
  /* RID_REALPART */	REALPART,
  /* RID_LABEL */	LABEL,
  /* RID_PTRBASE */	PTR_BASE,
  /* RID_PTREXTENT */	PTR_EXTENT,
  /* RID_PTRVALUE */	PTR_VALUE,

  /* RID_CHOOSE_EXPR */			CHOOSE_EXPR,
  /* RID_TYPES_COMPATIBLE_P */		TYPES_COMPATIBLE_P,

  /* RID_FUNCTION_NAME */		STRING_FUNC_NAME,
  /* RID_PRETTY_FUNCTION_NAME */	STRING_FUNC_NAME,
  /* RID_C99_FUNCTION_NAME */		VAR_FUNC_NAME,

  /* C++ */
  /* RID_BOOL */	TYPESPEC,
  /* RID_WCHAR */	0,
  /* RID_CLASS */	0,
  /* RID_PUBLIC */	0,
  /* RID_PRIVATE */	0,
  /* RID_PROTECTED */	0,
  /* RID_TEMPLATE */	0,
  /* RID_NULL */	0,
  /* RID_CATCH */	0,
  /* RID_DELETE */	0,
  /* RID_FALSE */	0,
  /* RID_NAMESPACE */	0,
  /* RID_NEW */		0,
  /* RID_OPERATOR */	0,
  /* RID_THIS */	0,
  /* RID_THROW */	0,
  /* RID_TRUE */	0,
  /* RID_TRY */		0,
  /* RID_TYPENAME */	0,
  /* RID_TYPEID */	0,
  /* RID_USING */	0,

  /* casts */
  /* RID_CONSTCAST */	0,
  /* RID_DYNCAST */	0,
  /* RID_REINTCAST */	0,
  /* RID_STATCAST */	0,

  /* Objective C */
  /* RID_ID */			OBJECTNAME,
  /* RID_AT_ENCODE */		ENCODE,
  /* RID_AT_END */		END,
  /* RID_AT_CLASS */		CLASS,
  /* RID_AT_ALIAS */		ALIAS,
  /* RID_AT_DEFS */		DEFS,
  /* RID_AT_PRIVATE */		PRIVATE,
  /* RID_AT_PROTECTED */	PROTECTED,
  /* RID_AT_PUBLIC */		PUBLIC,
  /* RID_AT_PROTOCOL */		PROTOCOL,
  /* RID_AT_SELECTOR */		SELECTOR,
  /* RID_AT_INTERFACE */	INTERFACE,
  /* RID_AT_IMPLEMENTATION */	IMPLEMENTATION
};

static void
init_reswords ()
{
  unsigned int i;
  tree id;
  int mask = (flag_isoc99 ? 0 : D_C89)
	      | (flag_no_asm ? (flag_isoc99 ? D_EXT : D_EXT|D_EXT89) : 0);

  if (!flag_objc)
     mask |= D_OBJC;

  /* It is not necessary to register ridpointers as a GC root, because
     all the trees it points to are permanently interned in the
     get_identifier hash anyway.  */
  ridpointers = (tree *) xcalloc ((int) RID_MAX, sizeof (tree));
  for (i = 0; i < N_reswords; i++)
    {
      /* If a keyword is disabled, do not enter it into the table
	 and so create a canonical spelling that isn't a keyword.  */
      if (reswords[i].disable & mask)
	continue;

      id = get_identifier (reswords[i].word);
      C_RID_CODE (id) = reswords[i].rid;
      C_IS_RESERVED_WORD (id) = 1;
      ridpointers [(int) reswords[i].rid] = id;
    }
}

#define NAME(type) cpp_type2name (type)

static void
yyerror (msgid)
     const char *msgid;
{
  const char *string = _(msgid);

  if (last_token == CPP_EOF)
    error ("%s at end of input", string);
  else if (last_token == CPP_CHAR || last_token == CPP_WCHAR)
    {
      unsigned int val = TREE_INT_CST_LOW (yylval.ttype);
      const char *const ell = (last_token == CPP_CHAR) ? "" : "L";
      if (val <= UCHAR_MAX && ISGRAPH (val))
	error ("%s before %s'%c'", string, ell, val);
      else
	error ("%s before %s'\\x%x'", string, ell, val);
    }
  else if (last_token == CPP_STRING
	   || last_token == CPP_WSTRING)
    error ("%s before string constant", string);
  else if (last_token == CPP_NUMBER)
    error ("%s before numeric constant", string);
  else if (last_token == CPP_NAME)
    error ("%s before \"%s\"", string, IDENTIFIER_POINTER (yylval.ttype));
  else
    error ("%s before '%s' token", string, NAME(last_token));
}

static int
yylexname ()
{
  tree decl;

  int objc_force_identifier = objc_need_raw_identifier;
  OBJC_NEED_RAW_IDENTIFIER (0);

  if (C_IS_RESERVED_WORD (yylval.ttype))
    {
      enum rid rid_code = C_RID_CODE (yylval.ttype);

      /* Turn non-typedefed refs to "id" into plain identifiers; this
	 allows constructs like "void foo(id id);" to work.  */
      if (rid_code == RID_ID)
      {
	decl = lookup_name (yylval.ttype);
	if (decl == NULL_TREE || TREE_CODE (decl) != TYPE_DECL)
	  return IDENTIFIER;
      }

      if (!OBJC_IS_AT_KEYWORD (rid_code)
	  && (!OBJC_IS_PQ_KEYWORD (rid_code) || objc_pq_context))
      {
	int yycode = rid_to_yy[(int) rid_code];
	if (yycode == STRING_FUNC_NAME)
	  {
	    /* __FUNCTION__ and __PRETTY_FUNCTION__ get converted
	       to string constants.  */
	    const char *name = fname_string (rid_code);

	    yylval.ttype = build_string (strlen (name) + 1, name);
	    C_ARTIFICIAL_STRING_P (yylval.ttype) = 1;
	    last_token = CPP_STRING;  /* so yyerror won't choke */
	    return STRING;
	  }

	/* Return the canonical spelling for this keyword.  */
	yylval.ttype = ridpointers[(int) rid_code];
	return yycode;
      }
    }

  decl = lookup_name (yylval.ttype);
  if (decl)
    {
      if (TREE_CODE (decl) == TYPE_DECL)
	return TYPENAME;
    }
  else
    {
      tree objc_interface_decl = is_class_name (yylval.ttype);
      /* ObjC class names are in the same namespace as variables and
	 typedefs, and hence are shadowed by local declarations.  */
      if (objc_interface_decl
	  && (global_bindings_p ()
	      || (!objc_force_identifier && !decl)))
	{
	  yylval.ttype = objc_interface_decl;
	  return CLASSNAME;
	}
    }

  return IDENTIFIER;
}

/* Concatenate strings before returning them to the parser.  This isn't quite
   as good as having it done in the lexer, but it's better than nothing.  */

static int
yylexstring ()
{
  enum cpp_ttype next_type;
  tree orig = yylval.ttype;

  next_type = c_lex (&yylval.ttype);
  if (next_type == CPP_STRING
      || next_type == CPP_WSTRING
      || (next_type == CPP_NAME && yylexname () == STRING))
    {
      varray_type strings;


      VARRAY_TREE_INIT (strings, 32, "strings");
      VARRAY_PUSH_TREE (strings, orig);

      do
	{
	  VARRAY_PUSH_TREE (strings, yylval.ttype);
	  next_type = c_lex (&yylval.ttype);
	}
      while (next_type == CPP_STRING
	     || next_type == CPP_WSTRING
	     || (next_type == CPP_NAME && yylexname () == STRING));

      yylval.ttype = combine_strings (strings);
    }
  else
    yylval.ttype = orig;

  /* We will have always read one token too many.  */
  _cpp_backup_tokens (parse_in, 1);

  return STRING;
}

static inline int
_yylex ()
{
 get_next:
  last_token = c_lex (&yylval.ttype);
  switch (last_token)
    {
    case CPP_EQ:					return '=';
    case CPP_NOT:					return '!';
    case CPP_GREATER:	yylval.code = GT_EXPR;		return ARITHCOMPARE;
    case CPP_LESS:	yylval.code = LT_EXPR;		return ARITHCOMPARE;
    case CPP_PLUS:	yylval.code = PLUS_EXPR;	return '+';
    case CPP_MINUS:	yylval.code = MINUS_EXPR;	return '-';
    case CPP_MULT:	yylval.code = MULT_EXPR;	return '*';
    case CPP_DIV:	yylval.code = TRUNC_DIV_EXPR;	return '/';
    case CPP_MOD:	yylval.code = TRUNC_MOD_EXPR;	return '%';
    case CPP_AND:	yylval.code = BIT_AND_EXPR;	return '&';
    case CPP_OR:	yylval.code = BIT_IOR_EXPR;	return '|';
    case CPP_XOR:	yylval.code = BIT_XOR_EXPR;	return '^';
    case CPP_RSHIFT:	yylval.code = RSHIFT_EXPR;	return RSHIFT;
    case CPP_LSHIFT:	yylval.code = LSHIFT_EXPR;	return LSHIFT;

    case CPP_COMPL:					return '~';
    case CPP_AND_AND:					return ANDAND;
    case CPP_OR_OR:					return OROR;
    case CPP_QUERY:					return '?';
    case CPP_OPEN_PAREN:				return '(';
    case CPP_EQ_EQ:	yylval.code = EQ_EXPR;		return EQCOMPARE;
    case CPP_NOT_EQ:	yylval.code = NE_EXPR;		return EQCOMPARE;
    case CPP_GREATER_EQ:yylval.code = GE_EXPR;		return ARITHCOMPARE;
    case CPP_LESS_EQ:	yylval.code = LE_EXPR;		return ARITHCOMPARE;

    case CPP_PLUS_EQ:	yylval.code = PLUS_EXPR;	return ASSIGN;
    case CPP_MINUS_EQ:	yylval.code = MINUS_EXPR;	return ASSIGN;
    case CPP_MULT_EQ:	yylval.code = MULT_EXPR;	return ASSIGN;
    case CPP_DIV_EQ:	yylval.code = TRUNC_DIV_EXPR;	return ASSIGN;
    case CPP_MOD_EQ:	yylval.code = TRUNC_MOD_EXPR;	return ASSIGN;
    case CPP_AND_EQ:	yylval.code = BIT_AND_EXPR;	return ASSIGN;
    case CPP_OR_EQ:	yylval.code = BIT_IOR_EXPR;	return ASSIGN;
    case CPP_XOR_EQ:	yylval.code = BIT_XOR_EXPR;	return ASSIGN;
    case CPP_RSHIFT_EQ:	yylval.code = RSHIFT_EXPR;	return ASSIGN;
    case CPP_LSHIFT_EQ:	yylval.code = LSHIFT_EXPR;	return ASSIGN;

    case CPP_OPEN_SQUARE:				return '[';
    case CPP_CLOSE_SQUARE:				return ']';
    case CPP_OPEN_BRACE:				return '{';
    case CPP_CLOSE_BRACE:				return '}';
    case CPP_ELLIPSIS:					return ELLIPSIS;

    case CPP_PLUS_PLUS:					return PLUSPLUS;
    case CPP_MINUS_MINUS:				return MINUSMINUS;
    case CPP_DEREF:					return POINTSAT;
    case CPP_DOT:					return '.';

      /* The following tokens may affect the interpretation of any
	 identifiers following, if doing Objective-C.  */
    case CPP_COLON:		OBJC_NEED_RAW_IDENTIFIER (0);	return ':';
    case CPP_COMMA:		OBJC_NEED_RAW_IDENTIFIER (0);	return ',';
    case CPP_CLOSE_PAREN:	OBJC_NEED_RAW_IDENTIFIER (0);	return ')';
    case CPP_SEMICOLON:		OBJC_NEED_RAW_IDENTIFIER (0);	return ';';

    case CPP_EOF:
      return 0;

    case CPP_NAME:
      {
	int ret = yylexname ();
	if (ret == STRING)
	  return yylexstring ();
	else
	  return ret;
      }

    case CPP_NUMBER:
    case CPP_CHAR:
    case CPP_WCHAR:
      return CONSTANT;

    case CPP_STRING:
    case CPP_WSTRING:
      return yylexstring ();

      /* This token is Objective-C specific.  It gives the next token
	 special significance.  */
    case CPP_ATSIGN:
      {
	tree after_at;
	enum cpp_ttype after_at_type;

	after_at_type = c_lex (&after_at);

	if (after_at_type == CPP_NAME
	    && C_IS_RESERVED_WORD (after_at)
	    && OBJC_IS_AT_KEYWORD (C_RID_CODE (after_at)))
	  {
	    yylval.ttype = after_at;
	    last_token = after_at_type;
	    return rid_to_yy [(int) C_RID_CODE (after_at)];
	  }
	_cpp_backup_tokens (parse_in, 1);
	return '@';
      }

      /* These tokens are C++ specific (and will not be generated
         in C mode, but let's be cautious).  */
    case CPP_SCOPE:
    case CPP_DEREF_STAR:
    case CPP_DOT_STAR:
    case CPP_MIN_EQ:
    case CPP_MAX_EQ:
    case CPP_MIN:
    case CPP_MAX:
      /* These tokens should not survive translation phase 4.  */
    case CPP_HASH:
    case CPP_PASTE:
      error ("syntax error at '%s' token", NAME(last_token));
      goto get_next;

    default:
      abort ();
    }
  /* NOTREACHED */
}

static int
yylex()
{
  int r;
  timevar_push (TV_LEX);
  r = _yylex();
  timevar_pop (TV_LEX);
  return r;
}

/* Function used when yydebug is set, to print a token in more detail.  */

static void
yyprint (file, yychar, yyl)
     FILE *file;
     int yychar;
     YYSTYPE yyl;
{
  tree t = yyl.ttype;

  fprintf (file, " [%s]", NAME(last_token));

  switch (yychar)
    {
    case IDENTIFIER:
    case TYPENAME:
    case OBJECTNAME:
    case TYPESPEC:
    case TYPE_QUAL:
    case SCSPEC:
    case STATIC:
      if (IDENTIFIER_POINTER (t))
	fprintf (file, " `%s'", IDENTIFIER_POINTER (t));
      break;

    case CONSTANT:
      fprintf (file, " %s", GET_MODE_NAME (TYPE_MODE (TREE_TYPE (t))));
      if (TREE_CODE (t) == INTEGER_CST)
	fprintf (file,
#if HOST_BITS_PER_WIDE_INT == 64
#if HOST_BITS_PER_WIDE_INT == HOST_BITS_PER_INT
		 " 0x%x%016x",
#else
#if HOST_BITS_PER_WIDE_INT == HOST_BITS_PER_LONG
		 " 0x%lx%016lx",
#else
		 " 0x%llx%016llx",
#endif
#endif
#else
#if HOST_BITS_PER_WIDE_INT != HOST_BITS_PER_INT
		 " 0x%lx%08lx",
#else
		 " 0x%x%08x",
#endif
#endif
		 TREE_INT_CST_HIGH (t), TREE_INT_CST_LOW (t));
      break;
    }
}

/* This is not the ideal place to put these, but we have to get them out
   of c-lex.c because cp/lex.c has its own versions.  */

/* Free malloced parser stacks if necessary.  */

void
free_parser_stacks ()
{
  if (malloced_yyss)
    {
      free (malloced_yyss);
      free (malloced_yyvs);
    }
}

#include "gt-c-parse.h"


