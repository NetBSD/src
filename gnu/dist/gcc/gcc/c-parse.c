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
#line 34 "c-parse.y"

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
#line 103 "c-parse.y"
typedef union YYSTYPE {long itype; tree ttype; enum tree_code code;
	const char *filename; int lineno; } YYSTYPE;
/* Line 191 of yacc.c.  */
#line 283 "c-p15339.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */
#line 237 "c-parse.y"

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


#define OBJC_NEED_RAW_IDENTIFIER(VAL)	/* nothing */

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
#line 379 "c-p15339.c"

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
#define YYLAST   3120

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  92
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  202
/* YYNRULES -- Number of rules. */
#define YYNRULES  558
/* YYNRULES -- Number of states. */
#define YYNSTATES  896

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
       2,    44,     2,    46,     2,     2,     2,     2,     2,     2,
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
      19,    21,    27,    30,    34,    39,    44,    47,    50,    53,
      55,    56,    57,    67,    72,    73,    74,    84,    89,    90,
      91,   100,   104,   106,   108,   110,   112,   114,   116,   118,
     120,   122,   124,   125,   127,   129,   133,   135,   138,   141,
     144,   147,   150,   155,   158,   163,   166,   169,   171,   173,
     175,   177,   182,   184,   188,   192,   196,   200,   204,   208,
     212,   216,   220,   224,   228,   232,   233,   238,   239,   244,
     245,   246,   254,   255,   261,   265,   269,   271,   273,   275,
     277,   278,   286,   290,   294,   298,   302,   307,   314,   323,
     330,   335,   339,   343,   346,   349,   351,   352,   354,   358,
     360,   362,   365,   368,   373,   378,   381,   384,   388,   389,
     391,   396,   401,   405,   409,   412,   415,   417,   420,   423,
     426,   429,   432,   434,   437,   439,   442,   445,   448,   451,
     454,   457,   459,   462,   465,   468,   471,   474,   477,   480,
     483,   486,   489,   492,   495,   498,   501,   504,   507,   509,
     512,   515,   518,   521,   524,   527,   530,   533,   536,   539,
     542,   545,   548,   551,   554,   557,   560,   563,   566,   569,
     572,   575,   578,   581,   584,   587,   590,   593,   596,   599,
     602,   605,   608,   611,   614,   617,   620,   623,   626,   629,
     632,   635,   638,   641,   643,   645,   647,   649,   651,   653,
     655,   657,   659,   661,   663,   665,   667,   669,   671,   673,
     675,   677,   679,   681,   683,   685,   687,   689,   691,   693,
     695,   697,   699,   701,   703,   705,   707,   709,   711,   713,
     715,   717,   719,   721,   723,   725,   727,   729,   731,   733,
     735,   737,   739,   741,   743,   745,   747,   749,   751,   753,
     754,   756,   758,   760,   762,   764,   766,   768,   770,   775,
     780,   782,   787,   789,   794,   795,   800,   801,   808,   812,
     813,   820,   824,   825,   827,   829,   832,   839,   841,   845,
     846,   848,   853,   860,   865,   867,   869,   871,   873,   875,
     877,   879,   880,   885,   887,   888,   891,   893,   897,   901,
     904,   905,   910,   912,   913,   918,   920,   922,   924,   927,
     930,   936,   940,   941,   942,   950,   951,   952,   960,   962,
     964,   969,   973,   976,   980,   982,   984,   986,   990,   993,
     995,   999,  1002,  1006,  1010,  1015,  1019,  1024,  1028,  1031,
    1033,  1035,  1038,  1040,  1043,  1045,  1048,  1049,  1057,  1063,
    1064,  1072,  1078,  1079,  1088,  1089,  1097,  1100,  1103,  1106,
    1107,  1109,  1110,  1112,  1114,  1117,  1118,  1122,  1125,  1129,
    1134,  1138,  1140,  1142,  1145,  1147,  1152,  1154,  1159,  1164,
    1171,  1177,  1182,  1189,  1195,  1197,  1201,  1203,  1205,  1209,
    1210,  1214,  1215,  1217,  1218,  1220,  1223,  1225,  1227,  1229,
    1233,  1236,  1240,  1245,  1249,  1252,  1255,  1257,  1262,  1266,
    1271,  1277,  1283,  1285,  1287,  1289,  1291,  1293,  1296,  1299,
    1302,  1305,  1307,  1310,  1313,  1316,  1318,  1321,  1324,  1327,
    1330,  1332,  1335,  1337,  1339,  1341,  1343,  1346,  1347,  1348,
    1349,  1350,  1351,  1353,  1355,  1358,  1362,  1364,  1367,  1369,
    1371,  1377,  1379,  1381,  1384,  1387,  1390,  1393,  1394,  1400,
    1401,  1406,  1407,  1408,  1410,  1413,  1417,  1421,  1425,  1426,
    1431,  1433,  1437,  1438,  1439,  1447,  1453,  1456,  1457,  1458,
    1459,  1460,  1473,  1474,  1481,  1484,  1486,  1488,  1491,  1495,
    1498,  1501,  1504,  1508,  1515,  1524,  1535,  1548,  1552,  1557,
    1559,  1563,  1569,  1572,  1578,  1579,  1581,  1582,  1584,  1585,
    1587,  1589,  1593,  1598,  1606,  1608,  1612,  1613,  1617,  1620,
    1621,  1622,  1629,  1632,  1633,  1635,  1637,  1641,  1643,  1647,
    1652,  1657,  1661,  1666,  1670,  1675,  1680,  1684,  1689,  1693,
    1695,  1696,  1700,  1702,  1705,  1707,  1711,  1713,  1717
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short yyrhs[] =
{
      93,     0,    -1,    -1,    94,    -1,    -1,    95,    97,    -1,
      -1,    94,    96,    97,    -1,    98,    -1,   100,    -1,    99,
      -1,    28,    67,   109,    84,    85,    -1,   293,    97,    -1,
     131,   165,    85,    -1,   151,   131,   165,    85,    -1,   150,
     131,   164,    85,    -1,   157,    85,    -1,     1,    85,    -1,
       1,    86,    -1,    85,    -1,    -1,    -1,   150,   131,   194,
     101,   125,   102,   254,   255,   243,    -1,   150,   131,   194,
       1,    -1,    -1,    -1,   151,   131,   199,   103,   125,   104,
     254,   255,   243,    -1,   151,   131,   199,     1,    -1,    -1,
      -1,   131,   199,   105,   125,   106,   254,   255,   243,    -1,
     131,   199,     1,    -1,     3,    -1,     4,    -1,    52,    -1,
      58,    -1,    57,    -1,    63,    -1,    62,    -1,    87,    -1,
      88,    -1,   111,    -1,    -1,   111,    -1,   117,    -1,   111,
      89,   117,    -1,   123,    -1,    59,   116,    -1,   293,   116,
      -1,   108,   116,    -1,    49,   107,    -1,   113,   112,    -1,
     113,    67,   220,    84,    -1,   114,   112,    -1,   114,    67,
     220,    84,    -1,    34,   116,    -1,    35,   116,    -1,    12,
      -1,    30,    -1,    29,    -1,   112,    -1,    67,   220,    84,
     116,    -1,   116,    -1,   117,    57,   117,    -1,   117,    58,
     117,    -1,   117,    59,   117,    -1,   117,    60,   117,    -1,
     117,    61,   117,    -1,   117,    56,   117,    -1,   117,    55,
     117,    -1,   117,    54,   117,    -1,   117,    53,   117,    -1,
     117,    52,   117,    -1,   117,    50,   117,    -1,   117,    51,
     117,    -1,    -1,   117,    49,   118,   117,    -1,    -1,   117,
      48,   119,   117,    -1,    -1,    -1,   117,    46,   120,   109,
      47,   121,   117,    -1,    -1,   117,    46,   122,    47,   117,
      -1,   117,    44,   117,    -1,   117,    45,   117,    -1,     3,
      -1,     9,    -1,    10,    -1,    43,    -1,    -1,    67,   220,
      84,    90,   124,   180,    86,    -1,    67,   109,    84,    -1,
      67,     1,    84,    -1,   247,   245,    84,    -1,   247,     1,
      84,    -1,   123,    67,   110,    84,    -1,    36,    67,   117,
      89,   220,    84,    -1,    37,    67,   117,    89,   117,    89,
     117,    84,    -1,    38,    67,   220,    89,   220,    84,    -1,
     123,    68,   109,    91,    -1,   123,    66,   107,    -1,   123,
      69,   107,    -1,   123,    63,    -1,   123,    62,    -1,   126,
      -1,    -1,   128,    -1,   254,   255,   129,    -1,   127,    -1,
     235,    -1,   128,   127,    -1,   127,   235,    -1,   152,   131,
     164,    85,    -1,   153,   131,   165,    85,    -1,   152,    85,
      -1,   153,    85,    -1,   254,   255,   133,    -1,    -1,   171,
      -1,   150,   131,   164,    85,    -1,   151,   131,   165,    85,
      -1,   150,   131,   188,    -1,   151,   131,   191,    -1,   157,
      85,    -1,   293,   133,    -1,     8,    -1,   134,     8,    -1,
     135,     8,    -1,   134,   172,    -1,   136,     8,    -1,   137,
       8,    -1,   172,    -1,   136,   172,    -1,   159,    -1,   138,
       8,    -1,   139,     8,    -1,   138,   161,    -1,   139,   161,
      -1,   134,   159,    -1,   135,   159,    -1,   160,    -1,   138,
     172,    -1,   138,   162,    -1,   139,   162,    -1,   134,   160,
      -1,   135,   160,    -1,   140,     8,    -1,   141,     8,    -1,
     140,   161,    -1,   141,   161,    -1,   136,   159,    -1,   137,
     159,    -1,   140,   172,    -1,   140,   162,    -1,   141,   162,
      -1,   136,   160,    -1,   137,   160,    -1,   177,    -1,   142,
       8,    -1,   143,     8,    -1,   134,   177,    -1,   135,   177,
      -1,   142,   177,    -1,   143,   177,    -1,   142,   172,    -1,
     144,     8,    -1,   145,     8,    -1,   136,   177,    -1,   137,
     177,    -1,   144,   177,    -1,   145,   177,    -1,   144,   172,
      -1,   146,     8,    -1,   147,     8,    -1,   146,   161,    -1,
     147,   161,    -1,   142,   159,    -1,   143,   159,    -1,   138,
     177,    -1,   139,   177,    -1,   146,   177,    -1,   147,   177,
      -1,   146,   172,    -1,   146,   162,    -1,   147,   162,    -1,
     142,   160,    -1,   143,   160,    -1,   148,     8,    -1,   149,
       8,    -1,   148,   161,    -1,   149,   161,    -1,   144,   159,
      -1,   145,   159,    -1,   140,   177,    -1,   141,   177,    -1,
     148,   177,    -1,   149,   177,    -1,   148,   172,    -1,   148,
     162,    -1,   149,   162,    -1,   144,   160,    -1,   145,   160,
      -1,   138,    -1,   139,    -1,   140,    -1,   141,    -1,   146,
      -1,   147,    -1,   148,    -1,   149,    -1,   134,    -1,   135,
      -1,   136,    -1,   137,    -1,   142,    -1,   143,    -1,   144,
      -1,   145,    -1,   138,    -1,   139,    -1,   146,    -1,   147,
      -1,   134,    -1,   135,    -1,   142,    -1,   143,    -1,   138,
      -1,   139,    -1,   140,    -1,   141,    -1,   134,    -1,   135,
      -1,   136,    -1,   137,    -1,   138,    -1,   139,    -1,   140,
      -1,   141,    -1,   134,    -1,   135,    -1,   136,    -1,   137,
      -1,   134,    -1,   135,    -1,   136,    -1,   137,    -1,   138,
      -1,   139,    -1,   140,    -1,   141,    -1,   142,    -1,   143,
      -1,   144,    -1,   145,    -1,   146,    -1,   147,    -1,   148,
      -1,   149,    -1,    -1,   155,    -1,   161,    -1,   163,    -1,
     162,    -1,     7,    -1,   208,    -1,   203,    -1,     4,    -1,
     115,    67,   109,    84,    -1,   115,    67,   220,    84,    -1,
     167,    -1,   164,    89,   132,   167,    -1,   169,    -1,   165,
      89,   132,   169,    -1,    -1,    28,    67,    10,    84,    -1,
      -1,   194,   166,   171,    44,   168,   178,    -1,   194,   166,
     171,    -1,    -1,   199,   166,   171,    44,   170,   178,    -1,
     199,   166,   171,    -1,    -1,   172,    -1,   173,    -1,   172,
     173,    -1,    31,    67,    67,   174,    84,    84,    -1,   175,
      -1,   174,    89,   175,    -1,    -1,   176,    -1,   176,    67,
       3,    84,    -1,   176,    67,     3,    89,   111,    84,    -1,
     176,    67,   110,    84,    -1,   107,    -1,   177,    -1,     7,
      -1,     8,    -1,     6,    -1,     5,    -1,   117,    -1,    -1,
      90,   179,   180,    86,    -1,     1,    -1,    -1,   181,   209,
      -1,   182,    -1,   181,    89,   182,    -1,   186,    44,   184,
      -1,   187,   184,    -1,    -1,   107,    47,   183,   184,    -1,
     184,    -1,    -1,    90,   185,   180,    86,    -1,   117,    -1,
       1,    -1,   187,    -1,   186,   187,    -1,    66,   107,    -1,
      68,   117,    11,   117,    91,    -1,    68,   117,    91,    -1,
      -1,    -1,   194,   189,   125,   190,   254,   255,   248,    -1,
      -1,    -1,   199,   192,   125,   193,   254,   255,   248,    -1,
     195,    -1,   199,    -1,    67,   171,   195,    84,    -1,   195,
      67,   288,    -1,   195,   228,    -1,    59,   158,   195,    -1,
       4,    -1,   197,    -1,   198,    -1,   197,    67,   288,    -1,
     197,   228,    -1,     4,    -1,   198,    67,   288,    -1,   198,
     228,    -1,    59,   158,   197,    -1,    59,   158,   198,    -1,
      67,   171,   198,    84,    -1,   199,    67,   288,    -1,    67,
     171,   199,    84,    -1,    59,   158,   199,    -1,   199,   228,
      -1,     3,    -1,    14,    -1,    14,   172,    -1,    15,    -1,
      15,   172,    -1,    13,    -1,    13,   172,    -1,    -1,   200,
     107,    90,   204,   211,    86,   171,    -1,   200,    90,   211,
      86,   171,    -1,    -1,   201,   107,    90,   205,   211,    86,
     171,    -1,   201,    90,   211,    86,   171,    -1,    -1,   202,
     107,    90,   206,   218,   210,    86,   171,    -1,    -1,   202,
      90,   207,   218,   210,    86,   171,    -1,   200,   107,    -1,
     201,   107,    -1,   202,   107,    -1,    -1,    89,    -1,    -1,
      89,    -1,   212,    -1,   212,   213,    -1,    -1,   212,   213,
      85,    -1,   212,    85,    -1,   154,   131,   214,    -1,   154,
     131,   254,   255,    -1,   155,   131,   215,    -1,   155,    -1,
       1,    -1,   293,   213,    -1,   216,    -1,   214,    89,   132,
     216,    -1,   217,    -1,   215,    89,   132,   217,    -1,   254,
     255,   194,   171,    -1,   254,   255,   194,    47,   117,   171,
      -1,   254,   255,    47,   117,   171,    -1,   254,   255,   199,
     171,    -1,   254,   255,   199,    47,   117,   171,    -1,   254,
     255,    47,   117,   171,    -1,   219,    -1,   218,    89,   219,
      -1,     1,    -1,   107,    -1,   107,    44,   117,    -1,    -1,
     156,   221,   222,    -1,    -1,   224,    -1,    -1,   224,    -1,
     225,   172,    -1,   226,    -1,   225,    -1,   227,    -1,    59,
     158,   225,    -1,    59,   158,    -1,    59,   158,   226,    -1,
      67,   171,   224,    84,    -1,   227,    67,   278,    -1,   227,
     228,    -1,    67,   278,    -1,   228,    -1,    68,   158,   109,
      91,    -1,    68,   158,    91,    -1,    68,   158,    59,    91,
      -1,    68,     6,   158,   109,    91,    -1,    68,   155,     6,
     109,    91,    -1,   230,    -1,   231,    -1,   232,    -1,   233,
      -1,   258,    -1,   230,   258,    -1,   231,   258,    -1,   232,
     258,    -1,   233,   258,    -1,   130,    -1,   230,   130,    -1,
     231,   130,    -1,   233,   130,    -1,   259,    -1,   230,   259,
      -1,   231,   259,    -1,   232,   259,    -1,   233,   259,    -1,
     235,    -1,   234,   235,    -1,   230,    -1,   231,    -1,   232,
      -1,   233,    -1,     1,    85,    -1,    -1,    -1,    -1,    -1,
      -1,   241,    -1,   242,    -1,   241,   242,    -1,    33,   292,
      85,    -1,   248,    -1,     1,   248,    -1,    90,    -1,    86,
      -1,   236,   240,   246,    86,   237,    -1,   229,    -1,     1,
      -1,    67,    90,    -1,   244,   245,    -1,   250,   257,    -1,
     250,     1,    -1,    -1,    16,   251,    67,   109,    84,    -1,
      -1,    19,   253,   257,    18,    -1,    -1,    -1,   258,    -1,
     259,   256,    -1,   238,   256,   239,    -1,   254,   255,   270,
      -1,   254,   255,   271,    -1,    -1,   249,    17,   261,   257,
      -1,   249,    -1,   249,    17,     1,    -1,    -1,    -1,    18,
     262,    67,   109,    84,   263,   257,    -1,   252,    67,   109,
      84,    85,    -1,   252,     1,    -1,    -1,    -1,    -1,    -1,
      20,   264,    67,   269,   265,   273,    85,   266,   273,    84,
     267,   257,    -1,    -1,    21,    67,   109,    84,   268,   257,
      -1,   273,    85,    -1,   133,    -1,   248,    -1,   109,    85,
      -1,   238,   260,   239,    -1,    24,    85,    -1,    25,    85,
      -1,    26,    85,    -1,    26,   109,    85,    -1,    28,   272,
      67,   109,    84,    85,    -1,    28,   272,    67,   109,    47,
     274,    84,    85,    -1,    28,   272,    67,   109,    47,   274,
      47,   274,    84,    85,    -1,    28,   272,    67,   109,    47,
     274,    47,   274,    47,   277,    84,    85,    -1,    27,   107,
      85,    -1,    27,    59,   109,    85,    -1,    85,    -1,    22,
     117,    47,    -1,    22,   117,    11,   117,    47,    -1,    23,
      47,    -1,   107,   254,   255,    47,   171,    -1,    -1,     8,
      -1,    -1,   109,    -1,    -1,   275,    -1,   276,    -1,   275,
      89,   276,    -1,    10,    67,   109,    84,    -1,    68,   107,
      91,    10,    67,   109,    84,    -1,    10,    -1,   277,    89,
      10,    -1,    -1,   171,   279,   280,    -1,   283,    84,    -1,
      -1,    -1,   284,    85,   281,   171,   282,   280,    -1,     1,
      84,    -1,    -1,    11,    -1,   284,    -1,   284,    89,    11,
      -1,   286,    -1,   284,    89,   285,    -1,   150,   131,   196,
     171,    -1,   150,   131,   199,   171,    -1,   150,   131,   223,
      -1,   151,   131,   199,   171,    -1,   151,   131,   223,    -1,
     152,   287,   196,   171,    -1,   152,   287,   199,   171,    -1,
     152,   287,   223,    -1,   153,   287,   199,   171,    -1,   153,
     287,   223,    -1,   131,    -1,    -1,   171,   289,   290,    -1,
     280,    -1,   291,    84,    -1,     3,    -1,   291,    89,     3,
      -1,   107,    -1,   292,    89,   107,    -1,    32,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   324,   324,   328,   347,   347,   348,   348,   352,   357,
     358,   359,   367,   372,   379,   381,   383,   385,   386,   387,
     394,   399,   393,   405,   408,   413,   407,   419,   422,   427,
     421,   433,   438,   439,   442,   444,   446,   451,   453,   455,
     457,   461,   467,   468,   472,   474,   479,   480,   483,   486,
     490,   492,   498,   501,   504,   507,   509,   514,   518,   522,
     526,   527,   532,   533,   535,   537,   539,   541,   543,   545,
     547,   549,   551,   553,   555,   558,   557,   565,   564,   572,
     576,   571,   582,   581,   592,   599,   611,   617,   618,   620,
     623,   622,   635,   640,   642,   658,   665,   667,   670,   680,
     690,   692,   696,   702,   704,   710,   718,   724,   731,   736,
     737,   738,   739,   747,   749,   751,   754,   763,   772,   782,
     787,   789,   791,   793,   795,   797,   854,   857,   860,   866,
     872,   875,   881,   884,   890,   893,   896,   899,   902,   905,
     908,   914,   917,   920,   923,   926,   929,   935,   938,   941,
     944,   947,   950,   956,   959,   962,   965,   968,   974,   977,
     980,   983,   989,   995,  1001,  1010,  1016,  1019,  1022,  1028,
    1034,  1040,  1049,  1055,  1058,  1061,  1064,  1067,  1070,  1073,
    1079,  1085,  1091,  1100,  1103,  1106,  1109,  1112,  1118,  1121,
    1124,  1127,  1130,  1133,  1136,  1142,  1148,  1154,  1163,  1166,
    1169,  1172,  1175,  1182,  1183,  1184,  1185,  1186,  1187,  1188,
    1189,  1193,  1194,  1195,  1196,  1197,  1198,  1199,  1200,  1204,
    1205,  1206,  1207,  1211,  1212,  1213,  1214,  1218,  1219,  1220,
    1221,  1225,  1226,  1227,  1228,  1232,  1233,  1234,  1235,  1236,
    1237,  1238,  1239,  1243,  1244,  1245,  1246,  1247,  1248,  1249,
    1250,  1251,  1252,  1253,  1254,  1255,  1256,  1257,  1258,  1264,
    1265,  1291,  1292,  1296,  1300,  1302,  1306,  1310,  1314,  1316,
    1323,  1324,  1328,  1329,  1334,  1335,  1341,  1340,  1348,  1357,
    1356,  1364,  1373,  1374,  1379,  1381,  1386,  1391,  1393,  1399,
    1400,  1402,  1404,  1406,  1414,  1415,  1416,  1417,  1421,  1422,
    1428,  1430,  1429,  1433,  1440,  1442,  1446,  1447,  1453,  1456,
    1460,  1459,  1465,  1470,  1469,  1473,  1475,  1479,  1480,  1484,
    1486,  1490,  1496,  1509,  1495,  1527,  1540,  1526,  1560,  1561,
    1567,  1569,  1574,  1576,  1578,  1586,  1587,  1591,  1596,  1598,
    1602,  1607,  1609,  1611,  1613,  1621,  1626,  1628,  1630,  1632,
    1636,  1638,  1643,  1645,  1650,  1652,  1664,  1663,  1669,  1674,
    1673,  1677,  1682,  1681,  1687,  1686,  1694,  1696,  1698,  1706,
    1708,  1711,  1713,  1719,  1721,  1727,  1728,  1730,  1736,  1739,
    1749,  1752,  1757,  1759,  1765,  1766,  1771,  1772,  1777,  1780,
    1784,  1790,  1793,  1797,  1808,  1809,  1814,  1820,  1822,  1828,
    1827,  1836,  1837,  1842,  1845,  1849,  1856,  1857,  1861,  1862,
    1867,  1869,  1874,  1876,  1878,  1880,  1882,  1889,  1891,  1893,
    1895,  1898,  1909,  1910,  1911,  1915,  1919,  1920,  1921,  1922,
    1923,  1927,  1928,  1931,  1932,  1936,  1937,  1938,  1939,  1940,
    1944,  1945,  1949,  1950,  1951,  1952,  1955,  1959,  1966,  1971,
    1987,  2001,  2003,  2009,  2010,  2014,  2028,  2030,  2033,  2037,
    2039,  2047,  2048,  2052,  2070,  2078,  2083,  2096,  2095,  2110,
    2109,  2129,  2135,  2141,  2142,  2147,  2153,  2167,  2177,  2176,
    2184,  2196,  2207,  2210,  2206,  2216,  2219,  2222,  2226,  2229,
    2233,  2221,  2237,  2236,  2244,  2246,  2252,  2254,  2257,  2261,
    2264,  2267,  2270,  2273,  2277,  2281,  2286,  2290,  2302,  2308,
    2316,  2319,  2322,  2325,  2342,  2344,  2350,  2351,  2357,  2358,
    2362,  2363,  2368,  2370,  2377,  2379,  2390,  2389,  2400,  2402,
    2410,  2401,  2414,  2421,  2422,  2432,  2436,  2441,  2443,  2450,
    2455,  2460,  2463,  2469,  2477,  2482,  2487,  2490,  2496,  2502,
    2512,  2511,  2522,  2523,  2541,  2543,  2549,  2551,  2556
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
  "ALIAS", "')'", "';'", "'}'", "'~'", "'!'", "','", "'{'", "']'", 
  "$accept", "program", "extdefs", "@1", "@2", "extdef", "extdef_1", 
  "datadef", "fndef", "@3", "@4", "@5", "@6", "@7", "@8", "identifier", 
  "unop", "expr", "exprlist", "nonnull_exprlist", "unary_expr", "sizeof", 
  "alignof", "typeof", "cast_expr", "expr_no_commas", "@9", "@10", "@11", 
  "@12", "@13", "primary", "@14", "old_style_parm_decls", 
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
  "identifiers", "identifiers_or_typenames", "extension", 0
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
     123,    93
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned short yyr1[] =
{
       0,    92,    93,    93,    95,    94,    96,    94,    97,    98,
      98,    98,    98,    99,    99,    99,    99,    99,    99,    99,
     101,   102,   100,   100,   103,   104,   100,   100,   105,   106,
     100,   100,   107,   107,   108,   108,   108,   108,   108,   108,
     108,   109,   110,   110,   111,   111,   112,   112,   112,   112,
     112,   112,   112,   112,   112,   112,   112,   113,   114,   115,
     116,   116,   117,   117,   117,   117,   117,   117,   117,   117,
     117,   117,   117,   117,   117,   118,   117,   119,   117,   120,
     121,   117,   122,   117,   117,   117,   123,   123,   123,   123,
     124,   123,   123,   123,   123,   123,   123,   123,   123,   123,
     123,   123,   123,   123,   123,   125,   126,   126,   127,   128,
     128,   128,   128,   129,   129,   129,   129,   130,   131,   132,
     133,   133,   133,   133,   133,   133,   134,   134,   134,   135,
     136,   136,   137,   137,   138,   138,   138,   138,   138,   138,
     138,   139,   139,   139,   139,   139,   139,   140,   140,   140,
     140,   140,   140,   141,   141,   141,   141,   141,   142,   142,
     142,   142,   142,   142,   142,   143,   144,   144,   144,   144,
     144,   144,   145,   146,   146,   146,   146,   146,   146,   146,
     146,   146,   146,   147,   147,   147,   147,   147,   148,   148,
     148,   148,   148,   148,   148,   148,   148,   148,   149,   149,
     149,   149,   149,   150,   150,   150,   150,   150,   150,   150,
     150,   151,   151,   151,   151,   151,   151,   151,   151,   152,
     152,   152,   152,   153,   153,   153,   153,   154,   154,   154,
     154,   155,   155,   155,   155,   156,   156,   156,   156,   156,
     156,   156,   156,   157,   157,   157,   157,   157,   157,   157,
     157,   157,   157,   157,   157,   157,   157,   157,   157,   158,
     158,   159,   159,   160,   161,   161,   162,   163,   163,   163,
     164,   164,   165,   165,   166,   166,   168,   167,   167,   170,
     169,   169,   171,   171,   172,   172,   173,   174,   174,   175,
     175,   175,   175,   175,   176,   176,   176,   176,   177,   177,
     178,   179,   178,   178,   180,   180,   181,   181,   182,   182,
     183,   182,   182,   185,   184,   184,   184,   186,   186,   187,
     187,   187,   189,   190,   188,   192,   193,   191,   194,   194,
     195,   195,   195,   195,   195,   196,   196,   197,   197,   197,
     198,   198,   198,   198,   198,   199,   199,   199,   199,   199,
     200,   200,   201,   201,   202,   202,   204,   203,   203,   205,
     203,   203,   206,   203,   207,   203,   208,   208,   208,   209,
     209,   210,   210,   211,   211,   212,   212,   212,   213,   213,
     213,   213,   213,   213,   214,   214,   215,   215,   216,   216,
     216,   217,   217,   217,   218,   218,   218,   219,   219,   221,
     220,   222,   222,   223,   223,   223,   224,   224,   225,   225,
     226,   226,   227,   227,   227,   227,   227,   228,   228,   228,
     228,   228,   229,   229,   229,   229,   230,   230,   230,   230,
     230,   231,   231,   231,   231,   232,   232,   232,   232,   232,
     233,   233,   234,   234,   234,   234,   235,   236,   237,   238,
     239,   240,   240,   241,   241,   242,   243,   243,   244,   245,
     245,   246,   246,   247,   248,   249,   249,   251,   250,   253,
     252,   254,   255,   256,   256,   257,   258,   259,   261,   260,
     260,   260,   262,   263,   260,   260,   260,   264,   265,   266,
     267,   260,   268,   260,   269,   269,   270,   270,   270,   270,
     270,   270,   270,   270,   270,   270,   270,   270,   270,   270,
     271,   271,   271,   271,   272,   272,   273,   273,   274,   274,
     275,   275,   276,   276,   277,   277,   279,   278,   280,   281,
     282,   280,   280,   283,   283,   283,   283,   284,   284,   285,
     285,   285,   285,   285,   286,   286,   286,   286,   286,   287,
     289,   288,   290,   290,   291,   291,   292,   292,   293
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     0,     1,     0,     2,     0,     3,     1,     1,
       1,     5,     2,     3,     4,     4,     2,     2,     2,     1,
       0,     0,     9,     4,     0,     0,     9,     4,     0,     0,
       8,     3,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     0,     1,     1,     3,     1,     2,     2,     2,
       2,     2,     4,     2,     4,     2,     2,     1,     1,     1,
       1,     4,     1,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     0,     4,     0,     4,     0,
       0,     7,     0,     5,     3,     3,     1,     1,     1,     1,
       0,     7,     3,     3,     3,     3,     4,     6,     8,     6,
       4,     3,     3,     2,     2,     1,     0,     1,     3,     1,
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
       1,     1,     1,     1,     1,     1,     1,     1,     4,     4,
       1,     4,     1,     4,     0,     4,     0,     6,     3,     0,
       6,     3,     0,     1,     1,     2,     6,     1,     3,     0,
       1,     4,     6,     4,     1,     1,     1,     1,     1,     1,
       1,     0,     4,     1,     0,     2,     1,     3,     3,     2,
       0,     4,     1,     0,     4,     1,     1,     1,     2,     2,
       5,     3,     0,     0,     7,     0,     0,     7,     1,     1,
       4,     3,     2,     3,     1,     1,     1,     3,     2,     1,
       3,     2,     3,     3,     4,     3,     4,     3,     2,     1,
       1,     2,     1,     2,     1,     2,     0,     7,     5,     0,
       7,     5,     0,     8,     0,     7,     2,     2,     2,     0,
       1,     0,     1,     1,     2,     0,     3,     2,     3,     4,
       3,     1,     1,     2,     1,     4,     1,     4,     4,     6,
       5,     4,     6,     5,     1,     3,     1,     1,     3,     0,
       3,     0,     1,     0,     1,     2,     1,     1,     1,     3,
       2,     3,     4,     3,     2,     2,     1,     4,     3,     4,
       5,     5,     1,     1,     1,     1,     1,     2,     2,     2,
       2,     1,     2,     2,     2,     1,     2,     2,     2,     2,
       1,     2,     1,     1,     1,     1,     2,     0,     0,     0,
       0,     0,     1,     1,     2,     3,     1,     2,     1,     1,
       5,     1,     1,     2,     2,     2,     2,     0,     5,     0,
       4,     0,     0,     1,     2,     3,     3,     3,     0,     4,
       1,     3,     0,     0,     7,     5,     2,     0,     0,     0,
       0,    12,     0,     6,     2,     1,     1,     2,     3,     2,
       2,     2,     3,     6,     8,    10,    12,     3,     4,     1,
       3,     5,     2,     5,     0,     1,     0,     1,     0,     1,
       1,     3,     4,     7,     1,     3,     0,     3,     2,     0,
       0,     6,     2,     0,     1,     1,     3,     1,     3,     4,
       4,     3,     4,     3,     4,     4,     3,     4,     3,     1,
       0,     3,     1,     2,     1,     3,     1,     3,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned short yydefact[] =
{
       4,     0,     6,     0,     1,     0,     0,   267,   299,   298,
     264,   126,   354,   350,   352,     0,    59,     0,   558,    19,
       5,     8,    10,     9,     0,     0,   211,   212,   213,   214,
     203,   204,   205,   206,   215,   216,   217,   218,   207,   208,
     209,   210,   118,   118,     0,   134,   141,   261,   263,   262,
     132,   284,   158,     0,     0,     0,   266,   265,     0,     7,
      17,    18,   355,   351,   353,     0,     0,     0,   349,   259,
     282,     0,   272,     0,   127,   139,   145,   129,   161,   128,
     140,   146,   162,   130,   151,   156,   133,   168,   131,   152,
     157,   169,   135,   137,   143,   142,   179,   136,   138,   144,
     180,   147,   149,   154,   153,   194,   148,   150,   155,   195,
     159,   177,   186,   165,   163,   160,   178,   187,   164,   166,
     192,   201,   172,   170,   167,   193,   202,   171,   173,   175,
     184,   183,   181,   174,   176,   185,   182,   188,   190,   199,
     198,   196,   189,   191,   200,   197,     0,     0,    16,   285,
      32,    33,   375,   366,   375,   367,   364,   368,    12,    86,
      87,    88,    57,    58,     0,     0,     0,     0,     0,    89,
       0,    34,    36,    35,     0,    38,    37,     0,    39,    40,
       0,     0,    41,    60,     0,     0,    62,    44,    46,     0,
       0,   289,     0,   239,   240,   241,   242,   235,   236,   237,
     238,   399,     0,   231,   232,   233,   234,   260,     0,     0,
     283,    13,   282,    31,     0,   282,   259,     0,   282,   348,
     334,   259,   282,     0,   270,     0,   328,   329,     0,     0,
       0,     0,   356,     0,   359,     0,   362,    55,    56,     0,
       0,     0,    50,    47,     0,   463,     0,     0,    49,     0,
       0,     0,    51,     0,    53,     0,     0,    79,    77,    75,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   104,   103,     0,    42,     0,     0,     0,   459,
     451,     0,    48,   296,   297,   294,     0,   287,   290,   295,
     268,   401,   269,   347,     0,     0,   119,     0,   550,   345,
     259,   260,     0,     0,    29,   105,     0,   471,   110,   472,
     281,     0,     0,    15,   282,    23,     0,   282,   282,   332,
      14,    27,     0,   282,   382,   377,   231,   232,   233,   234,
     227,   228,   229,   230,   118,   118,   374,     0,   375,   282,
     375,   396,   397,   371,   394,     0,     0,     0,     0,    93,
      92,     0,    11,    45,     0,     0,    84,    85,     0,     0,
       0,     0,    73,    74,    72,    71,    70,    69,    68,    63,
      64,    65,    66,    67,   101,     0,    43,     0,   102,    95,
       0,     0,   452,   453,    94,     0,   289,    42,   259,   282,
     400,   402,   407,   406,   408,   416,   346,   273,   274,     0,
       0,     0,     0,     0,   418,     0,   446,   471,   112,   111,
       0,   279,   333,     0,     0,    21,   278,   331,    25,   358,
     471,   471,   376,   383,     0,   361,     0,     0,   372,     0,
     371,     0,     0,     0,    90,    61,    52,    54,     0,     0,
      78,    76,    96,   100,   556,     0,   462,   431,   461,   471,
     471,   471,   471,     0,   440,     0,   472,   426,   435,   454,
     286,   288,    86,     0,   410,   526,   415,   282,   414,   275,
       0,   554,   534,   223,   224,   219,   220,   225,   226,   221,
     222,   118,   118,   552,     0,   535,   537,   551,     0,     0,
       0,   419,   417,   472,   108,   118,   118,     0,   330,   271,
     274,   471,   276,   471,   378,   384,   472,   380,   386,   472,
     282,   282,   398,   395,   282,     0,     0,     0,     0,     0,
      80,    83,   455,     0,   432,   427,   436,   433,   428,   437,
     472,   429,   438,   434,   430,   439,   441,   448,   449,   291,
       0,   293,   409,   411,     0,     0,   526,   413,   532,   549,
     403,   403,   528,   529,     0,   553,     0,   420,   421,     0,
     115,     0,   116,     0,   303,   301,   300,   280,   472,     0,
     472,   282,   379,   282,     0,   357,   360,   365,   282,    97,
       0,    99,   316,    86,     0,     0,   313,     0,   315,     0,
     369,   306,   312,     0,     0,     0,   557,   449,   460,   267,
       0,     0,     0,     0,     0,     0,   514,   509,   458,   471,
       0,   117,   118,   118,     0,     0,   447,   496,   476,   477,
       0,     0,   412,   527,   339,   259,   282,   282,   335,   336,
     282,   546,   404,   407,   259,   282,   282,   548,   282,   536,
     211,   212,   213,   214,   203,   204,   205,   206,   215,   216,
     217,   218,   207,   208,   209,   210,   118,   118,   538,   555,
       0,    30,   456,     0,     0,     0,     0,   277,     0,   471,
       0,   282,   471,     0,   282,   363,     0,   319,     0,     0,
     310,    91,     0,   305,     0,   318,   309,    81,     0,   512,
     499,   500,   501,     0,     0,     0,   515,     0,   472,   497,
       0,     0,   124,   467,   482,   469,   487,     0,   480,     0,
       0,   450,   464,   125,   292,   410,   526,   544,   282,   338,
     282,   341,   545,   405,   410,   526,   547,   530,   403,   403,
     457,   113,   114,     0,    22,    26,   385,   472,   282,     0,
     388,   387,   282,     0,   391,    98,     0,   321,     0,     0,
     307,   308,     0,   510,   502,     0,   507,     0,     0,     0,
     122,   322,     0,   123,   325,     0,     0,   449,     0,     0,
       0,   466,   471,   465,   486,     0,   498,   342,   343,     0,
     337,   340,     0,   282,   282,   541,   282,   543,   302,     0,
     390,   282,   393,   282,     0,   314,   311,     0,   508,     0,
     282,   120,     0,   121,     0,     0,     0,     0,   516,     0,
     481,   449,   450,   473,   471,     0,   344,   531,   539,   540,
     542,   389,   392,   320,   511,   518,     0,   513,   323,   326,
       0,     0,   470,   517,   495,   488,     0,   492,   479,   475,
     474,     0,     0,     0,     0,   519,   520,   503,   471,   471,
     468,   483,   516,   494,   449,   485,     0,     0,   518,     0,
       0,   472,   472,   449,     0,   493,     0,     0,     0,   504,
     521,     0,     0,   484,   489,   522,     0,     0,     0,   324,
     327,   516,     0,   524,     0,   505,     0,     0,     0,     0,
     490,   523,   506,   525,   449,   491
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,     1,     2,     3,     5,    20,    21,    22,    23,   316,
     501,   322,   503,   217,   407,   587,   180,   246,   375,   182,
     183,   184,   185,    24,   186,   187,   361,   360,   358,   595,
     359,   188,   519,   304,   305,   306,   307,   494,   447,    25,
     295,   611,   193,   194,   195,   196,   197,   198,   199,   200,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
     481,   482,   334,   207,   201,    44,   208,    45,    46,    47,
      48,    49,   223,    71,   218,   224,   569,    72,   497,   296,
     210,    51,   286,   287,   288,    52,   567,   665,   589,   590,
     591,   749,   592,   679,   593,   594,   760,   802,   848,   763,
     804,   849,   500,   226,   627,   628,   629,   227,    53,    54,
      55,    56,   338,   340,   345,   235,    57,   683,   429,   230,
     231,   336,   504,   507,   505,   508,   343,   344,   202,   291,
     390,   631,   632,   392,   393,   394,   219,   448,   449,   450,
     451,   452,   453,   308,   280,   598,   772,   776,   381,   382,
     383,   661,   616,   281,   455,   189,   662,   708,   709,   765,
     710,   767,   309,   410,   812,   773,   813,   814,   711,   811,
     766,   863,   768,   852,   881,   894,   854,   835,   618,   619,
     697,   836,   844,   845,   846,   884,   466,   545,   483,   638,
     782,   484,   485,   658,   486,   550,   299,   400,   487,   488,
     445,   190
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -735
static const short yypact[] =
{
      66,   112,   119,  1512,  -735,  1512,   221,  -735,  -735,  -735,
    -735,  -735,   105,   105,   105,    91,  -735,   106,  -735,  -735,
    -735,  -735,  -735,  -735,   123,   183,   513,   387,   974,  1004,
     830,   488,  1143,  1677,  1260,  1174,  1293,  1402,  1579,  1764,
    1681,  1768,  -735,  -735,    70,  -735,  -735,  -735,  -735,  -735,
     105,  -735,  -735,    88,   110,   157,  -735,  -735,  1512,  -735,
    -735,  -735,   105,   105,   105,  2651,   160,  2569,  -735,   137,
     105,   182,  -735,   962,  -735,  -735,  -735,   105,  -735,  -735,
    -735,  -735,  -735,  -735,  -735,  -735,   105,  -735,  -735,  -735,
    -735,  -735,  -735,  -735,  -735,   105,  -735,  -735,  -735,  -735,
    -735,  -735,  -735,  -735,   105,  -735,  -735,  -735,  -735,  -735,
    -735,  -735,  -735,   105,  -735,  -735,  -735,  -735,  -735,  -735,
    -735,  -735,   105,  -735,  -735,  -735,  -735,  -735,  -735,  -735,
    -735,   105,  -735,  -735,  -735,  -735,  -735,  -735,  -735,  -735,
     105,  -735,  -735,  -735,  -735,  -735,   222,   183,  -735,  -735,
    -735,  -735,  -735,   141,  -735,   144,  -735,   159,  -735,  -735,
    -735,  -735,  -735,  -735,  2651,  2651,   208,   219,   277,  -735,
     495,  -735,  -735,  -735,  2651,  -735,  -735,  1362,  -735,  -735,
    2651,   272,   273,  -735,  2712,  2753,  -735,  3043,   745,  1613,
    2651,   896,   303,   844,   775,  2521,  2300,   787,   681,  1041,
     758,  -735,   320,   375,   406,   391,   420,  -735,   183,   183,
     105,  -735,   105,  -735,   416,   105,   274,   455,   105,  -735,
    -735,   137,   105,   279,  -735,  1056,    98,   448,   318,  1493,
     403,   738,  -735,   405,  -735,   590,  -735,  -735,  -735,  2651,
    2651,  2722,  -735,  -735,   425,  -735,   445,   457,  -735,   463,
    2651,  1362,  -735,  1362,  -735,  2651,  2651,   506,  -735,  -735,
    2651,  2651,  2651,  2651,  2651,  2651,  2651,  2651,  2651,  2651,
    2651,  2651,  -735,  -735,   495,  2651,  2651,   495,   477,  -735,
     531,   501,  -735,  -735,  -735,  -735,   350,  -735,   505,  -735,
    -735,   419,  -735,   448,   172,   183,  -735,   577,  -735,  -735,
     137,   595,  2202,   519,  -735,  -735,   826,    51,  -735,  -735,
     576,   222,   222,  -735,   105,  -735,   455,   105,   105,  -735,
    -735,  -735,   455,   105,  -735,  -735,   844,   775,  2521,  2300,
     787,   681,  1041,   758,  -735,   492,   539,  1829,  -735,   105,
    -735,  -735,   584,   541,  -735,   590,  2876,  2902,   554,  -735,
    -735,  2443,  -735,  3043,   563,   567,  3043,  3043,  2651,   608,
    2651,  2651,  2405,  2533,  1298,  1036,  1900,   717,   717,   266,
     266,  -735,  -735,  -735,  -735,   573,   273,   579,  -735,  -735,
     495,  1701,   531,  -735,  -735,   599,   896,  2794,   137,   105,
    -735,  -735,  -735,  -735,   571,  -735,  -735,  -735,   142,   601,
    1466,  2651,  2651,  2243,  -735,   600,  -735,  -735,  -735,  -735,
    1187,  -735,    98,   193,   222,  -735,   653,  -735,  -735,  -735,
    -735,  -735,  -735,  -735,   614,  -735,   616,  2651,   495,   619,
     541,  2722,  2651,  2722,  -735,  -735,   627,   627,   662,  2651,
    3006,  2963,  -735,  -735,  -735,   351,   519,  -735,  -735,   101,
     108,   130,   136,   705,  -735,   632,  -735,  -735,  -735,  -735,
    -735,  -735,   365,   636,   419,   419,  -735,   105,  -735,  -735,
     645,  -735,  -735,  2088,  1562,   883,  1497,  2118,  2181,  1857,
    1653,  -735,  -735,  -735,   646,   358,  -735,  -735,   382,   640,
     641,  -735,  -735,  -735,  -735,   652,   665,  2078,  -735,  -735,
     710,  -735,  -735,  -735,   658,  -735,  -735,   666,  -735,  -735,
     105,   105,  3043,  -735,   105,   668,   680,  2922,   684,  1195,
    -735,  3059,  -735,   495,  -735,  -735,  -735,  -735,  -735,  -735,
    -735,  -735,  -735,  -735,  -735,  -735,  -735,  -735,  2313,  -735,
    2651,  -735,  -735,  -735,   701,   602,  -735,  -735,  -735,  -735,
     281,   233,  -735,  -735,  1431,  -735,   783,  -735,  -735,    64,
    -735,   222,  -735,   183,  -735,  -735,  3043,  -735,  -735,  2078,
    -735,   105,   586,   105,   330,  -735,  -735,  -735,   105,  -735,
    2651,  -735,  -735,   750,   495,  2651,  -735,   751,  3043,   719,
     720,  -735,  -735,   316,  2010,  2651,  -735,  2382,  -735,   768,
    2651,   772,   736,   737,  2610,   125,   818,  -735,  -735,  -735,
     757,  -735,  -735,  -735,   762,   549,   767,  -735,  -735,  -735,
    2508,   396,  -735,  -735,  -735,   137,   105,   105,   592,   597,
     197,  -735,  -735,   105,   137,   105,   197,  -735,   105,  -735,
    2088,  1562,  2212,  2422,   883,  1497,  1917,  1741,  2118,  2181,
    2353,  3039,  1857,  1653,  2149,  1982,  -735,  -735,  -735,  -735,
     764,  -735,  -735,   446,   462,  1195,    64,  -735,    64,  -735,
    2651,   322,  -735,  2651,   298,  -735,  2944,  -735,  2814,  1195,
    -735,  -735,  1942,  -735,  2141,  -735,  -735,  3059,  2839,  -735,
    -735,  -735,  -735,   777,  2651,   778,  -735,   797,  -735,  -735,
     222,   183,  -735,  -735,  -735,  -735,  -735,   798,   849,  1789,
      71,  -735,  -735,  -735,  -735,   281,   356,  -735,   105,  -735,
     105,  -735,  -735,   105,   233,   233,  -735,  -735,   281,   233,
    -735,  -735,  -735,   786,  -735,  -735,  -735,  -735,  2981,  2651,
    -735,  -735,  2981,  2651,  -735,  -735,  2651,  -735,   788,  2141,
    -735,  -735,  2651,  -735,  -735,   799,  -735,  2651,   839,   465,
    -735,   659,   499,  -735,  1070,   840,   843,  -735,   846,  2651,
    1877,  -735,  -735,  -735,  -735,  2651,  -735,   592,   597,   304,
    -735,  -735,   602,   105,   197,  -735,   197,  -735,  -735,   586,
    -735,  2981,  -735,  2981,  2858,  -735,  -735,  3025,  -735,    50,
     105,  -735,   455,  -735,   455,  2651,  2651,   867,  2508,   827,
    -735,  -735,  -735,  -735,  -735,   835,  -735,  -735,  -735,  -735,
    -735,  -735,  -735,  -735,  -735,    72,   836,  -735,  -735,  -735,
     860,   861,  -735,  -735,  -735,  -735,   837,  -735,  -735,  -735,
    -735,   862,   879,   495,    68,   865,  -735,  -735,  -735,  -735,
    -735,  -735,  2651,  -735,  -735,  -735,  2651,   864,    72,   874,
      72,  -735,  -735,  -735,   875,  -735,   868,   951,    80,  -735,
    -735,   764,   764,  -735,  -735,  -735,   895,   954,   880,  -735,
    -735,  2651,  2651,  -735,   441,  -735,   887,   888,   889,   963,
    -735,  -735,  -735,  -735,  -735,  -735
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
    -735,  -735,  -735,  -735,  -735,    84,  -735,  -735,  -735,  -735,
    -735,  -735,  -735,  -735,  -735,    96,  -735,   -65,   596,  -244,
     489,  -735,  -735,  -735,  -106,   863,  -735,  -735,  -735,  -735,
    -735,  -735,  -735,  -312,  -735,   677,  -735,  -735,   162,   -31,
    -210,  -570,    -2,    38,    41,    42,    17,    21,    20,    48,
    -373,  -371,   431,   432,  -337,  -330,   438,   440,  -461,  -453,
     585,   587,  -735,  -133,  -735,  -517,  -167,   687,   699,   838,
     910,  -735,  -504,  -138,  -212,   582,  -735,   703,  -735,   282,
       2,    58,  -735,   613,  -735,   283,   435,  -735,  -260,  -735,
     319,  -735,  -508,  -735,  -735,   414,  -735,  -735,  -735,  -735,
    -735,  -735,  -140,   366,   293,   307,   -34,   -17,  -735,  -735,
    -735,  -735,  -735,  -735,  -735,  -735,  -735,  -735,   593,   -64,
    -735,   694,  -735,  -735,   363,   362,   695,   611,   -21,  -735,
    -735,  -516,  -272,  -348,  -416,  -735,   493,  -735,  -735,  -735,
    -735,  -735,  -735,  -218,  -735,  -735,  -451,   229,  -735,  -735,
     660,  -211,  -735,   429,  -735,  -735,  -521,  -735,  -735,  -735,
    -735,  -735,  -326,  -324,   232,  -706,    56,    61,  -735,  -735,
    -735,  -735,  -735,  -735,  -735,  -735,  -735,  -735,  -735,  -735,
    -735,  -734,   192,  -735,   198,  -735,   607,  -735,  -512,  -735,
    -735,  -735,  -735,  -735,  -735,   594,  -300,  -735,  -735,  -735,
    -735,    59
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -534
static const short yytable[] =
{
     181,    26,   192,    26,   415,    50,   225,    50,    73,   228,
     418,   146,   147,   317,    62,    63,    64,   617,   417,   391,
      30,   614,    30,    32,    31,    32,    31,   477,    77,   478,
      86,   376,    95,   623,   104,   637,   113,   477,   122,   478,
     131,    27,   140,    27,    28,    29,    28,    29,   543,   302,
     713,    33,  -107,    33,   311,   456,    26,   663,   237,   238,
      50,   807,    58,   479,    58,   660,    -2,   203,   243,    50,
     480,    50,   774,   479,   248,    30,   617,   612,    32,    31,
     480,   493,   842,   301,   282,   613,   686,   615,   408,    59,
     233,   150,   151,   656,   506,   509,    27,   825,   335,    28,
      29,   657,  -442,   614,   414,   838,    33,   204,   149,  -443,
     205,   206,     4,   150,   151,   858,   542,    58,   864,    -3,
     149,   149,   149,   456,   456,   530,   456,   877,   150,   151,
     229,  -444,   538,   401,   826,   149,    17,  -445,   775,   730,
     843,  -107,   158,   376,   149,    11,   615,   886,   865,   153,
     155,   157,   859,   149,   608,   148,   247,   873,    65,   612,
     150,   151,   149,   454,   878,   318,   216,   613,    17,   559,
     214,   149,   477,    66,   478,   568,   751,   570,   152,    50,
     149,   648,   572,   649,   694,   574,    68,  -422,   895,   149,
      67,   293,   294,   544,  -423,    77,   759,    86,   149,    95,
     154,   104,   633,   633,   335,    77,   597,    86,   479,   215,
     216,   377,   785,   787,   203,   480,  -424,   652,    50,   203,
     348,   464,  -425,    50,   653,    68,   220,   191,    17,   326,
     354,   232,   355,    50,   234,   536,    68,   405,   834,   215,
     216,   796,    69,    50,   666,   435,   668,   156,   330,   236,
      70,   332,   331,    50,   204,    50,   396,   205,   206,   204,
     318,   216,   205,   206,   215,   216,   242,   211,   149,   327,
     817,   212,   328,   329,   424,   239,   426,   498,   398,   333,
     300,   221,    11,   698,    68,   624,   240,   285,   317,   222,
     337,   614,   634,   438,   293,   294,   621,   243,   203,   543,
     635,   216,    50,   420,   421,    17,    60,    61,   543,    78,
      82,    87,    91,    96,   100,   105,   109,   114,   118,   123,
     127,   132,   136,   141,   145,   269,   270,   271,    77,    17,
      86,   342,    95,    68,   104,   326,   489,   490,   204,    50,
     625,   205,   206,   737,   241,   743,   509,   612,   626,   216,
     879,   880,   209,    17,   330,   613,   249,   332,   331,    68,
     684,   669,   250,   672,   313,   215,   216,   542,   314,   739,
     374,   720,   216,   378,   758,   327,   542,   673,   328,   329,
     633,   633,   584,    74,   585,   333,   203,   290,   816,    69,
      50,     7,     8,     9,    10,    79,   337,    70,   473,    83,
      12,    13,    14,   320,   292,   733,    17,   212,   473,   477,
     516,   478,   518,   789,    79,   625,    16,   475,   780,   748,
     781,   476,    17,   626,   216,   664,   204,   475,    88,   205,
     206,   476,   671,    50,   385,    50,   522,   457,   474,   386,
     523,   342,   458,   553,   544,   479,   530,   554,   474,   539,
     549,   549,   480,   544,   540,   734,   303,   735,   715,  -471,
    -471,  -471,  -471,  -471,   561,   563,   555,   724,  -471,  -471,
    -471,   556,  -244,   610,   289,    77,   444,    95,   388,   113,
     714,   131,   285,   297,  -471,   250,   389,   216,   530,   323,
     828,   339,   829,     8,     9,    10,    97,   298,   150,   151,
     310,    12,    13,    14,   312,   525,   528,   531,   534,   349,
     526,   529,   532,   535,   282,   215,   216,     7,     8,     9,
      10,    74,   861,   862,   342,   888,    12,    13,    14,   350,
     889,   731,   610,   630,   636,   314,    26,   871,   872,   693,
      50,   351,    16,   473,    17,  -106,   398,   732,   352,   317,
     801,   212,   640,   -82,   314,    30,    50,   674,    32,    31,
     761,   379,   475,   762,   380,   703,   476,   704,   705,   706,
     707,   644,   387,  -248,   646,   645,    27,  -381,  -381,    28,
      29,   700,   701,   474,   803,   384,    33,   399,   212,    68,
     220,   341,   641,   150,   151,   642,   643,   620,  -243,   416,
     298,   402,   647,   470,   406,   419,     7,     8,     9,    10,
      11,   524,   527,   472,   533,    12,    13,    14,    26,   596,
     411,   425,    50,   203,   422,   728,   729,    50,   427,   755,
     428,    16,   203,   670,   609,   723,    50,    30,   467,   216,
      32,    31,    77,   433,    86,   221,    95,   436,   104,   671,
     113,   437,   122,   222,   131,   439,   140,   442,    27,   718,
     216,    28,    29,   204,   720,   216,   205,   206,    33,   289,
     443,   465,   204,   252,   254,   205,   206,   412,   413,   620,
     677,   778,   779,   460,   764,   469,  -533,   214,    10,    97,
    -274,   492,   799,   609,    12,    13,    14,   502,   293,   294,
     510,   695,   511,  -274,   809,   514,   303,   293,   294,   520,
     815,   784,   786,    75,    80,    84,    89,   434,   537,   319,
     541,   111,   116,   120,   125,    76,    81,    85,    90,   548,
     552,   557,   558,   112,   117,   121,   126,   560,   214,   324,
     830,   831,     7,   833,  -274,    10,    11,   571,  -274,   546,
     562,    12,    13,    14,   578,   573,    78,    82,    96,   100,
     114,   118,   132,   136,   579,    10,   106,    16,   581,    17,
      18,    12,    13,    14,   267,   268,   269,   270,   271,     7,
     473,   149,    10,    79,   395,   622,   659,   833,    12,    13,
      14,   866,   575,   576,    10,    92,   577,   -32,   680,   475,
      12,    13,    14,   476,    16,   681,    26,   272,   273,   682,
      50,   274,   275,   276,   277,   -33,   833,   887,    17,   689,
     474,   690,   691,   325,  -373,    30,   696,   303,    32,    31,
    -109,  -109,  -109,  -109,  -109,     8,     9,    10,    92,  -109,
    -109,  -109,   699,    12,    13,    14,    27,   702,     7,    28,
      29,    10,    74,   279,   608,  -109,    33,    12,    13,    14,
     675,    17,   754,   756,   757,   769,   770,   620,    93,    98,
     102,   107,   788,    16,   795,    17,   129,   134,   138,   143,
      75,    80,    84,    89,   798,   832,   800,   468,     8,     9,
      10,    92,    76,    81,    85,    90,    12,    13,    14,   150,
     151,     8,     9,   283,   284,   319,   319,   805,   716,   717,
     806,   837,   722,   808,    17,  -247,  -109,   725,   726,   841,
     727,   847,   853,    78,    82,    87,    91,    96,   100,   105,
     109,   114,   118,   123,   127,   132,   136,   141,   145,   857,
      94,    99,   103,   108,   850,   851,   856,   855,   130,   135,
     139,   144,   875,   740,   860,   867,   744,   395,   395,   869,
     874,   876,   882,   213,   883,   885,   -28,   -28,   -28,   -28,
     -28,   890,   891,   893,   892,   -28,   -28,   -28,     7,     8,
       9,    10,    83,   463,   409,   650,   651,    12,    13,    14,
     214,   -28,   654,  -274,   655,   495,   499,   496,   397,   461,
     298,   750,   298,    16,   667,    17,  -274,   685,     7,     8,
       9,    10,    88,    75,    80,    84,    89,    12,    13,    14,
     790,   783,   777,   515,   792,    76,    81,    85,    90,   215,
     216,   423,   736,    16,   741,    93,    98,   102,   107,   513,
     430,   839,   459,   395,   395,   712,   840,  -274,    10,   101,
     868,  -274,   -28,     0,    12,    13,    14,   315,   870,  -245,
     -20,   -20,   -20,   -20,   -20,   818,   819,     0,   820,   -20,
     -20,   -20,    17,   821,   547,   822,   551,     0,     0,     0,
       0,     0,   827,     0,   214,   -20,     0,  -274,     0,  -246,
     264,   265,   266,   267,   268,   269,   270,   271,   214,     0,
    -274,  -274,   346,   347,     0,     0,     0,    94,    99,   103,
     108,     0,     0,   353,  -274,     0,     0,     0,   356,   357,
       0,   719,   721,   362,   363,   364,   365,   366,   367,   368,
     369,   370,   371,   372,   373,     0,     0,   215,   216,     0,
       0,  -274,     0,     0,     0,  -274,   -20,     0,     8,     9,
      10,   101,     0,     0,     0,  -274,    12,    13,    14,  -274,
      75,    80,     0,     0,   111,   116,     0,     0,    93,    98,
     102,   107,    76,    81,    17,     0,   112,   117,     7,     8,
       9,    10,   115,     0,     0,     0,     0,    12,    13,    14,
       0,     7,     8,     9,    10,    11,   582,     0,   583,   151,
      12,    13,    14,    16,   160,   161,     0,   162,   395,   395,
       0,     0,     0,     0,     0,     0,    16,   395,   395,     0,
       0,   395,   395,   440,   441,   163,     0,    18,  -249,   164,
     165,   166,   167,   168,     0,     0,     0,     0,   169,     0,
      94,    99,   103,   108,   170,     0,     0,   171,     0,     0,
       0,     0,   172,   173,   174,     0,     0,   175,   176,  -252,
       0,   584,   177,   585,     7,     8,     9,    10,   110,     0,
     719,   721,   721,    12,    13,    14,     0,     0,     0,     0,
       0,  -304,   178,   179,     0,   586,     0,     0,     0,    16,
     512,    17,     0,     0,     0,   517,     0,     7,     8,     9,
      10,   119,   521,     0,     0,     0,    12,    13,    14,     0,
       0,     0,     0,    93,    98,     0,     0,   129,   134,     0,
       0,     0,    16,     0,    17,     0,     0,    75,    80,    84,
      89,     0,     0,     0,     0,   111,   116,   120,   125,    76,
      81,    85,    90,     0,     0,  -251,     0,   112,   117,   121,
     126,   263,   264,   265,   266,   267,   268,   269,   270,   271,
     566,     0,     0,   244,     0,   159,     7,     0,     0,    10,
      11,   160,   161,     0,   162,    12,    13,    14,  -253,     0,
       0,     0,   588,     0,     0,    94,    99,     0,     0,   130,
     135,    16,   163,    17,    18,     0,   164,   165,   166,   167,
     168,     0,     0,     0,     0,   169,     7,     8,     9,    10,
     124,   170,     0,     0,   171,    12,    13,    14,     0,   172,
     173,   174,     0,     0,   175,   176,     0,     0,     0,   177,
       0,    16,   566,     0,     0,     7,     8,     9,    10,    11,
       0,     0,   639,   676,    12,    13,    14,     0,   678,   178,
     179,     0,   245,     0,     0,     0,     0,   588,   687,     0,
      16,     0,    17,   688,     0,     0,     0,   470,     0,   471,
       7,     8,     9,    10,    11,     0,     0,   472,     0,    12,
      13,    14,    93,    98,   102,   107,     0,  -254,     0,     0,
     129,   134,   138,   143,   321,    16,     0,   -24,   -24,   -24,
     -24,   -24,     8,     9,    10,    97,   -24,   -24,   -24,     0,
      12,    13,    14,     6,     0,  -118,     7,     8,     9,    10,
      11,   214,   -24,     0,  -274,    12,    13,    14,   588,     0,
       0,     0,     0,   738,     0,     0,   742,  -274,     0,     0,
      15,    16,   588,    17,    18,   588,     0,   588,     0,     0,
    -533,     0,     0,     0,    94,    99,   103,   108,     0,     0,
     215,   216,   130,   135,   139,   144,     7,     8,     9,    10,
      79,  -118,     0,     0,     0,    12,    13,    14,  -274,  -118,
       0,     0,  -274,   -24,     8,     9,    10,   128,     0,     0,
       0,    16,    12,    13,    14,     0,     0,    19,     0,     0,
       0,     0,   791,     0,     0,     0,   793,     0,     0,   794,
      17,     0,   588,     0,   278,   797,  -447,  -447,  -447,  -447,
    -447,  -447,  -447,  -447,     0,  -447,  -447,  -447,  -447,  -447,
       0,  -447,  -447,  -447,  -447,  -447,  -447,  -447,  -447,  -447,
    -447,  -447,  -447,  -447,  -447,  -447,  -447,  -447,  -447,  -447,
    -447,  -447,     0,     0,     0,     0,  -447,     0,     8,     9,
      10,   133,  -447,     0,  -255,  -447,    12,    13,    14,     0,
    -447,  -447,  -447,     0,     0,  -447,  -447,     0,     0,     0,
    -447,     0,     8,     9,    10,   106,     8,     9,    10,   137,
      12,    13,    14,     0,    12,    13,    14,     0,  -447,   279,
    -447,  -447,   446,  -447,  -471,  -471,  -471,  -471,  -471,  -471,
    -471,  -471,    17,  -471,  -471,  -471,  -471,  -471,     0,  -471,
    -471,  -471,  -471,  -471,  -471,  -471,  -471,  -471,  -471,  -471,
    -471,  -471,  -471,  -471,     0,  -471,  -471,  -471,  -471,  -471,
       0,     0,     0,     0,  -471,     0,     8,     9,    10,   106,
    -471,     0,     0,  -471,    12,    13,    14,     0,  -471,  -471,
    -471,     0,  -250,  -471,  -471,     0,  -257,     0,  -471,     8,
       9,    10,   133,     8,     9,    10,   142,    12,    13,    14,
       0,    12,    13,    14,     0,     0,  -471,     0,  -471,  -471,
     771,  -471,  -449,  -449,     0,     0,     0,     0,  -449,  -449,
       0,  -449,     0,     0,     0,  -449,     0,  -449,  -449,  -449,
    -449,  -449,  -449,  -449,  -449,  -449,  -449,  -449,     0,  -449,
       0,  -449,     0,  -449,  -449,  -449,  -449,  -449,     0,     0,
     324,     0,  -449,     7,     0,     0,    10,    11,  -449,     0,
       0,  -449,    12,    13,    14,     0,  -449,  -449,  -449,  -256,
       0,  -449,  -449,  -258,     0,     0,  -449,     0,    16,     0,
      17,    18,     8,     9,    10,   128,     0,     0,     0,     0,
      12,    13,    14,     0,  -449,     0,  -449,  -449,   810,  -449,
    -478,  -478,     0,     0,     0,     0,  -478,  -478,    17,  -478,
       0,     0,     0,  -478,     0,  -478,  -478,  -478,  -478,  -478,
    -478,  -478,  -478,  -478,  -478,  -478,     0,  -478,     0,  -478,
       0,  -478,  -478,  -478,  -478,  -478,     0,     0,     0,     0,
    -478,     0,     8,     9,    10,   101,  -478,     0,     0,  -478,
      12,    13,    14,     0,  -478,  -478,  -478,     0,     0,  -478,
    -478,     0,     0,   582,  -478,   583,   151,     0,    17,     0,
       0,   160,   161,     0,   162,   265,   266,   267,   268,   269,
     270,   271,  -478,     0,  -478,  -478,     0,  -478,     0,     0,
       0,     0,   163,     0,    18,     0,   164,   165,   166,   167,
     168,     0,     0,     0,     0,   169,     0,     8,     9,    10,
     142,   170,     0,     0,   171,    12,    13,    14,     0,   172,
     173,   174,     0,     0,   175,   176,     0,     0,   584,   177,
     585,   582,     0,   159,     0,     0,     0,     0,     0,   160,
     161,     0,   162,     0,     0,     0,     0,     0,  -370,   178,
     179,     0,   586,     0,     0,     0,     0,     0,     0,     0,
     163,     0,    18,     0,   164,   165,   166,   167,   168,     0,
       0,     0,     0,   169,  -317,     0,     0,     0,     0,   170,
       0,     0,   171,     0,     0,     0,     0,   172,   173,   174,
       0,     0,   175,   176,     0,     0,  -317,   177,  -317,   564,
       0,   159,     0,     0,     0,     0,     0,   160,   161,     0,
     162,     0,     7,     8,     9,    10,    74,   178,   179,     0,
     586,    12,    13,    14,     0,     0,     0,     0,   163,     0,
      18,     0,   164,   165,   166,   167,   168,    16,     0,    17,
       0,   169,     7,     8,     9,    10,   110,   170,     0,     0,
     171,    12,    13,    14,     0,   172,   173,   174,     0,     0,
     175,   176,   582,     0,   159,   177,     0,    16,     0,    17,
     160,   161,     0,   162,     8,     9,    10,   137,     0,     0,
       0,     0,    12,    13,    14,   178,   179,     0,   565,     0,
       0,   163,     0,    18,     0,   164,   165,   166,   167,   168,
      17,     0,     0,     0,   169,     7,     8,     9,    10,   115,
     170,     0,     0,   171,    12,    13,    14,     0,   172,   173,
     174,     0,     0,   175,   176,   159,     0,     0,   177,     0,
      16,   160,   161,     0,   162,     0,     7,     8,     9,    10,
      83,     0,     0,     0,     0,    12,    13,    14,   178,   179,
       0,   586,   163,     0,    18,     0,   164,   165,   166,   167,
     168,    16,     0,    17,     0,   169,   159,     0,     0,     0,
       0,   170,   160,   161,   171,   162,     0,     0,     0,   172,
     173,   403,     0,     0,   175,   176,     0,     0,     0,   177,
       0,     0,     0,   163,     0,    18,     0,   164,   165,   166,
     167,   168,     0,     0,     0,     0,   169,     0,     0,   178,
     179,     0,   170,   404,     0,   171,     0,     0,     0,     0,
     172,   173,   174,     0,     7,   175,   176,    10,    88,     0,
     177,     0,     0,    12,    13,    14,   583,   599,     8,     9,
      10,    11,   160,   161,     0,   162,    12,    13,    14,    16,
     178,   179,     0,     0,   491,   600,   601,   602,   603,   604,
     605,   606,    16,   163,    17,    18,     0,   164,   165,   166,
     167,   168,     0,     0,     0,     0,   169,     7,     8,     9,
      10,   119,   170,     0,     0,   171,    12,    13,    14,     0,
     172,   173,   174,     0,     0,   175,   176,     0,     0,     0,
     177,     0,    16,     0,    17,   583,   151,     0,     0,     0,
       0,   160,   161,     0,   162,     0,     0,     0,   607,     0,
     178,   179,     0,   608,   600,   601,   602,   603,   604,   605,
     606,     0,   163,     0,    18,     0,   164,   165,   166,   167,
     168,     0,     0,     0,     0,   169,     7,     8,     9,    10,
      88,   170,     0,     0,   171,    12,    13,    14,     0,   172,
     173,   174,     0,     0,   175,   176,   159,     0,     0,   177,
       0,    16,   160,   161,     0,   162,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   607,     0,   178,
     179,     0,   608,   163,     0,    18,     0,   164,   165,   166,
     167,   168,     0,     0,     0,     0,   169,     0,     0,     0,
       0,     0,   170,     0,     0,   171,     0,     0,     0,     0,
     172,   173,   174,     0,     0,   175,   176,     0,     0,     0,
     177,   159,     7,     8,     9,    10,    11,   160,   161,     0,
     162,    12,    13,    14,     0,     7,     0,     0,    10,    83,
     178,   179,     0,   434,    12,    13,    14,    16,   163,    17,
      18,     0,   164,   165,   166,   167,   168,     0,     0,     0,
      16,   169,    17,     0,     0,     0,     0,   170,     0,     0,
     171,     0,     0,     0,     0,   172,   173,   174,     0,     0,
     175,   176,   159,     7,     0,   177,    10,    11,   160,   161,
       0,   162,    12,    13,    14,   262,   263,   264,   265,   266,
     267,   268,   269,   270,   271,   178,   179,     0,    16,   163,
      17,    18,     0,   164,   165,   166,   167,   168,     0,     0,
       0,     0,   169,   159,     0,     0,     0,     0,   170,   160,
     161,   171,   162,     0,     0,     0,   172,   173,   174,     0,
       0,   175,   176,     0,     0,     0,   177,     0,     0,     0,
     163,     0,    18,     0,   164,   165,   166,   167,   168,     0,
       0,     0,     0,   169,   159,     0,   178,   179,     0,   170,
     160,   161,   171,   162,     0,     0,     0,   172,   173,   174,
       0,     0,   175,   176,     0,     0,     0,   177,     0,     0,
       0,   163,     0,    18,     0,   164,   165,   166,   167,   168,
       0,     0,     0,     0,   169,   692,     0,   178,   179,     0,
     170,     0,     0,   171,     0,     0,     0,     0,   172,   173,
     174,     0,     0,   175,   176,   159,     0,     0,   177,     0,
       0,   160,   161,     0,   162,     0,     7,     0,     0,    10,
      11,     0,     0,     0,     0,    12,    13,    14,   178,   179,
       0,     0,   163,     0,    18,     0,   164,   165,   166,   167,
     168,    16,     0,    17,     0,   169,   159,     0,     0,     0,
       0,   170,   160,   161,   171,   162,     0,     0,     0,   172,
     173,   174,     0,     0,   175,   176,     0,     0,     0,   251,
       0,     0,     0,   163,     0,    18,     0,   164,   165,   166,
     167,   168,     0,     0,     0,     0,   169,   462,     0,   178,
     179,     0,   170,   160,   161,   171,   162,     0,     0,     0,
     172,   173,   174,     0,     0,   175,   176,     0,     0,     0,
     253,     0,     0,     0,   163,   746,    18,     0,   164,   165,
     166,   167,   168,     0,     0,     0,     0,   169,     0,     0,
     178,   179,     0,   170,     0,     0,   171,     0,     0,     0,
     752,   172,   173,   174,     0,     0,   175,   176,   255,   256,
     257,   177,   258,   259,   260,   261,   262,   263,   264,   265,
     266,   267,   268,   269,   270,   271,     0,     0,     0,     0,
       0,   178,   179,   255,   256,   257,   753,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
     271,     0,   255,   256,   257,   747,   258,   259,   260,   261,
     262,   263,   264,   265,   266,   267,   268,   269,   270,   271,
     255,   256,   257,     0,   258,   259,   260,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   270,   271,     0,     0,
       0,     0,     0,     0,     0,     0,   255,   256,   257,   823,
     258,   259,   260,   261,   262,   263,   264,   265,   266,   267,
     268,   269,   270,   271,     0,   431,   255,   256,   257,     0,
     258,   259,   260,   261,   262,   263,   264,   265,   266,   267,
     268,   269,   270,   271,     0,     0,     0,     0,   255,   256,
     257,   432,   258,   259,   260,   261,   262,   263,   264,   265,
     266,   267,   268,   269,   270,   271,     0,     0,     0,     0,
       0,   580,    17,   260,   261,   262,   263,   264,   265,   266,
     267,   268,   269,   270,   271,   255,   256,   257,   745,   258,
     259,   260,   261,   262,   263,   264,   265,   266,   267,   268,
     269,   270,   271,     7,     8,     9,    10,   124,     0,     0,
       0,     0,    12,    13,    14,   259,   260,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   270,   271,    16,   255,
     256,   257,   824,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   255,   256,   257,
       0,   258,   259,   260,   261,   262,   263,   264,   265,   266,
     267,   268,   269,   270,   271,   257,     0,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
     271
};

static const short yycheck[] =
{
      65,     3,    67,     5,   316,     3,   146,     5,    25,   147,
     322,    42,    43,   225,    12,    13,    14,   538,   318,   291,
       3,   538,     5,     3,     3,     5,     5,   400,    26,   400,
      28,   275,    30,   545,    32,   551,    34,   410,    36,   410,
      38,     3,    40,     5,     3,     3,     5,     5,   464,   216,
     620,     3,     1,     5,   221,   381,    58,   561,   164,   165,
      58,   767,     3,   400,     5,     1,     0,    69,   174,    67,
     400,    69,     1,   410,   180,    58,   597,   538,    58,    58,
     410,   407,    10,   216,   190,   538,   594,   538,   306,     5,
     154,     3,     4,   554,   420,   421,    58,    47,   231,    58,
      58,   554,     1,   620,   314,   811,    58,    69,    50,     1,
      69,    69,     0,     3,     4,    47,   464,    58,   852,     0,
      62,    63,    64,   449,   450,   451,   452,    47,     3,     4,
     147,     1,   456,   300,    84,    77,    31,     1,    67,   660,
      68,    90,    58,   387,    86,     8,   597,   881,   854,    53,
      54,    55,    84,    95,    90,    85,   177,   863,    67,   620,
       3,     4,   104,   381,    84,    67,    68,   620,    31,   493,
      28,   113,   545,    67,   545,   501,   684,   503,    90,   177,
     122,   554,   506,   554,    59,   509,     3,    86,   894,   131,
      67,   208,   209,   465,    86,   193,   700,   195,   140,   197,
      90,   199,   550,   551,   337,   203,   530,   205,   545,    67,
      68,   276,   728,   729,   216,   545,    86,   554,   216,   221,
     241,   388,    86,   221,   554,     3,     4,    67,    31,   231,
     251,    90,   253,   231,    90,   453,     3,   302,   808,    67,
      68,   749,    59,   241,   568,   351,   570,    90,   231,    90,
      67,   231,   231,   251,   216,   253,    84,   216,   216,   221,
      67,    68,   221,   221,    67,    68,   170,    85,   210,   231,
     782,    89,   231,   231,   338,    67,   340,    84,   295,   231,
       6,    59,     8,   609,     3,     4,    67,   191,   500,    67,
     231,   808,    59,   358,   311,   312,   540,   403,   300,   715,
      67,    68,   300,   334,   335,    31,    85,    86,   724,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    59,    60,    61,   326,    31,
     328,   235,   330,     3,   332,   337,   401,   402,   300,   337,
      59,   300,   300,   669,    67,    47,   672,   808,    67,    68,
     871,   872,    70,    31,   337,   808,    84,   337,   337,     3,
      44,   571,    89,   573,    85,    67,    68,   715,    89,    47,
     274,    67,    68,   277,   698,   337,   724,    47,   337,   337,
     728,   729,    66,     8,    68,   337,   388,    84,    84,    59,
     388,     4,     5,     6,     7,     8,   337,    67,   400,     8,
      13,    14,    15,    85,    84,   665,    31,    89,   410,   782,
     431,   782,   433,   737,     8,    59,    29,   400,   718,   679,
     720,   400,    31,    67,    68,   563,   388,   410,     8,   388,
     388,   410,   572,   431,    84,   433,    85,   381,   400,    89,
      89,   345,   381,    85,   716,   782,   772,    89,   410,    84,
     481,   482,   782,   725,    89,   666,     1,   668,   625,     4,
       5,     6,     7,     8,   495,   496,    84,   634,    13,    14,
      15,    89,    85,   538,   191,   473,   380,   475,    59,   477,
      84,   479,   386,    67,    29,    89,    67,    68,   814,    86,
     802,    86,   804,     5,     6,     7,     8,   215,     3,     4,
     218,    13,    14,    15,   222,   449,   450,   451,   452,    84,
     449,   450,   451,   452,   620,    67,    68,     4,     5,     6,
       7,     8,   848,   849,   428,    84,    13,    14,    15,    84,
      89,    85,   597,   550,   551,    89,   538,   861,   862,   604,
     538,    84,    29,   545,    31,    90,   563,    85,    85,   761,
      85,    89,   554,    47,    89,   538,   554,   574,   538,   538,
     700,    84,   545,   701,    33,    16,   545,    18,    19,    20,
      21,   554,    67,    85,   554,   554,   538,    85,    86,   538,
     538,   612,   613,   545,    85,    84,   538,    10,    89,     3,
       4,     1,   554,     3,     4,   554,   554,   538,    85,   317,
     318,     6,   554,     1,    85,   323,     4,     5,     6,     7,
       8,   449,   450,    11,   452,    13,    14,    15,   620,   523,
      44,   339,   620,   625,    85,   656,   657,   625,    44,   694,
      89,    29,   634,    47,   538,   633,   634,   620,    67,    68,
     620,   620,   640,    89,   642,    59,   644,    84,   646,   789,
     648,    84,   650,    67,   652,    47,   654,    84,   620,    67,
      68,   620,   620,   625,    67,    68,   625,   625,   620,   386,
      91,   389,   634,   184,   185,   634,   634,   311,   312,   620,
     584,   715,   716,    84,   701,    84,    84,    28,     7,     8,
      31,    91,   757,   597,    13,    14,    15,    44,   715,   716,
      86,   605,    86,    44,   769,    86,     1,   724,   725,    47,
     775,   728,   729,    26,    27,    28,    29,    90,    86,   226,
      84,    34,    35,    36,    37,    26,    27,    28,    29,    84,
      84,    91,    91,    34,    35,    36,    37,    85,    28,     1,
     805,   806,     4,   808,    85,     7,     8,    89,    89,   467,
      85,    13,    14,    15,    86,    89,   473,   474,   475,   476,
     477,   478,   479,   480,    84,     7,     8,    29,    84,    31,
      32,    13,    14,    15,    57,    58,    59,    60,    61,     4,
     782,   723,     7,     8,   291,    84,     3,   852,    13,    14,
      15,   856,   510,   511,     7,     8,   514,    47,    47,   782,
      13,    14,    15,   782,    29,    86,   808,    62,    63,    89,
     808,    66,    67,    68,    69,    47,   881,   882,    31,    47,
     782,    85,    85,    85,    86,   808,     8,     1,   808,   808,
       4,     5,     6,     7,     8,     5,     6,     7,     8,    13,
      14,    15,    85,    13,    14,    15,   808,    85,     4,   808,
     808,     7,     8,    86,    90,    29,   808,    13,    14,    15,
     578,    31,    85,    85,    67,    67,    17,   808,    30,    31,
      32,    33,    86,    29,    86,    31,    38,    39,    40,    41,
     193,   194,   195,   196,    85,    18,    47,   394,     5,     6,
       7,     8,   193,   194,   195,   196,    13,    14,    15,     3,
       4,     5,     6,     7,     8,   412,   413,    67,   626,   627,
      67,    84,   630,    67,    31,    85,    90,   635,   636,    84,
     638,    85,    85,   640,   641,   642,   643,   644,   645,   646,
     647,   648,   649,   650,   651,   652,   653,   654,   655,   843,
      30,    31,    32,    33,    84,    84,    67,    85,    38,    39,
      40,    41,    84,   671,    89,    91,   674,   464,   465,    85,
      85,    10,    67,     1,    10,    85,     4,     5,     6,     7,
       8,    84,    84,    10,    85,    13,    14,    15,     4,     5,
       6,     7,     8,   387,   307,   554,   554,    13,    14,    15,
      28,    29,   554,    31,   554,   410,   414,   410,   295,   386,
     718,   682,   720,    29,   569,    31,    44,   593,     4,     5,
       6,     7,     8,   326,   327,   328,   329,    13,    14,    15,
     738,   728,   715,   430,   742,   326,   327,   328,   329,    67,
      68,   337,   669,    29,   672,   197,   198,   199,   200,   428,
     345,   812,   382,   550,   551,   616,   814,    85,     7,     8,
     858,    89,    90,    -1,    13,    14,    15,     1,   860,    85,
       4,     5,     6,     7,     8,   783,   784,    -1,   786,    13,
      14,    15,    31,   791,   467,   793,   482,    -1,    -1,    -1,
      -1,    -1,   800,    -1,    28,    29,    -1,    31,    -1,    85,
      54,    55,    56,    57,    58,    59,    60,    61,    28,    -1,
      44,    31,   239,   240,    -1,    -1,    -1,   197,   198,   199,
     200,    -1,    -1,   250,    44,    -1,    -1,    -1,   255,   256,
      -1,   628,   629,   260,   261,   262,   263,   264,   265,   266,
     267,   268,   269,   270,   271,    -1,    -1,    67,    68,    -1,
      -1,    85,    -1,    -1,    -1,    89,    90,    -1,     5,     6,
       7,     8,    -1,    -1,    -1,    85,    13,    14,    15,    89,
     473,   474,    -1,    -1,   477,   478,    -1,    -1,   330,   331,
     332,   333,   473,   474,    31,    -1,   477,   478,     4,     5,
       6,     7,     8,    -1,    -1,    -1,    -1,    13,    14,    15,
      -1,     4,     5,     6,     7,     8,     1,    -1,     3,     4,
      13,    14,    15,    29,     9,    10,    -1,    12,   715,   716,
      -1,    -1,    -1,    -1,    -1,    -1,    29,   724,   725,    -1,
      -1,   728,   729,   360,   361,    30,    -1,    32,    85,    34,
      35,    36,    37,    38,    -1,    -1,    -1,    -1,    43,    -1,
     330,   331,   332,   333,    49,    -1,    -1,    52,    -1,    -1,
      -1,    -1,    57,    58,    59,    -1,    -1,    62,    63,    85,
      -1,    66,    67,    68,     4,     5,     6,     7,     8,    -1,
     777,   778,   779,    13,    14,    15,    -1,    -1,    -1,    -1,
      -1,    86,    87,    88,    -1,    90,    -1,    -1,    -1,    29,
     427,    31,    -1,    -1,    -1,   432,    -1,     4,     5,     6,
       7,     8,   439,    -1,    -1,    -1,    13,    14,    15,    -1,
      -1,    -1,    -1,   475,   476,    -1,    -1,   479,   480,    -1,
      -1,    -1,    29,    -1,    31,    -1,    -1,   640,   641,   642,
     643,    -1,    -1,    -1,    -1,   648,   649,   650,   651,   640,
     641,   642,   643,    -1,    -1,    85,    -1,   648,   649,   650,
     651,    53,    54,    55,    56,    57,    58,    59,    60,    61,
     497,    -1,    -1,     1,    -1,     3,     4,    -1,    -1,     7,
       8,     9,    10,    -1,    12,    13,    14,    15,    85,    -1,
      -1,    -1,   519,    -1,    -1,   475,   476,    -1,    -1,   479,
     480,    29,    30,    31,    32,    -1,    34,    35,    36,    37,
      38,    -1,    -1,    -1,    -1,    43,     4,     5,     6,     7,
       8,    49,    -1,    -1,    52,    13,    14,    15,    -1,    57,
      58,    59,    -1,    -1,    62,    63,    -1,    -1,    -1,    67,
      -1,    29,   569,    -1,    -1,     4,     5,     6,     7,     8,
      -1,    -1,    11,   580,    13,    14,    15,    -1,   585,    87,
      88,    -1,    90,    -1,    -1,    -1,    -1,   594,   595,    -1,
      29,    -1,    31,   600,    -1,    -1,    -1,     1,    -1,     3,
       4,     5,     6,     7,     8,    -1,    -1,    11,    -1,    13,
      14,    15,   644,   645,   646,   647,    -1,    85,    -1,    -1,
     652,   653,   654,   655,     1,    29,    -1,     4,     5,     6,
       7,     8,     5,     6,     7,     8,    13,    14,    15,    -1,
      13,    14,    15,     1,    -1,     3,     4,     5,     6,     7,
       8,    28,    29,    -1,    31,    13,    14,    15,   665,    -1,
      -1,    -1,    -1,   670,    -1,    -1,   673,    44,    -1,    -1,
      28,    29,   679,    31,    32,   682,    -1,   684,    -1,    -1,
      84,    -1,    -1,    -1,   644,   645,   646,   647,    -1,    -1,
      67,    68,   652,   653,   654,   655,     4,     5,     6,     7,
       8,    59,    -1,    -1,    -1,    13,    14,    15,    85,    67,
      -1,    -1,    89,    90,     5,     6,     7,     8,    -1,    -1,
      -1,    29,    13,    14,    15,    -1,    -1,    85,    -1,    -1,
      -1,    -1,   739,    -1,    -1,    -1,   743,    -1,    -1,   746,
      31,    -1,   749,    -1,     1,   752,     3,     4,     5,     6,
       7,     8,     9,    10,    -1,    12,    13,    14,    15,    16,
      -1,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    -1,    -1,    -1,    -1,    43,    -1,     5,     6,
       7,     8,    49,    -1,    85,    52,    13,    14,    15,    -1,
      57,    58,    59,    -1,    -1,    62,    63,    -1,    -1,    -1,
      67,    -1,     5,     6,     7,     8,     5,     6,     7,     8,
      13,    14,    15,    -1,    13,    14,    15,    -1,    85,    86,
      87,    88,     1,    90,     3,     4,     5,     6,     7,     8,
       9,    10,    31,    12,    13,    14,    15,    16,    -1,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    -1,    34,    35,    36,    37,    38,
      -1,    -1,    -1,    -1,    43,    -1,     5,     6,     7,     8,
      49,    -1,    -1,    52,    13,    14,    15,    -1,    57,    58,
      59,    -1,    85,    62,    63,    -1,    85,    -1,    67,     5,
       6,     7,     8,     5,     6,     7,     8,    13,    14,    15,
      -1,    13,    14,    15,    -1,    -1,    85,    -1,    87,    88,
       1,    90,     3,     4,    -1,    -1,    -1,    -1,     9,    10,
      -1,    12,    -1,    -1,    -1,    16,    -1,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    -1,    30,
      -1,    32,    -1,    34,    35,    36,    37,    38,    -1,    -1,
       1,    -1,    43,     4,    -1,    -1,     7,     8,    49,    -1,
      -1,    52,    13,    14,    15,    -1,    57,    58,    59,    85,
      -1,    62,    63,    85,    -1,    -1,    67,    -1,    29,    -1,
      31,    32,     5,     6,     7,     8,    -1,    -1,    -1,    -1,
      13,    14,    15,    -1,    85,    -1,    87,    88,     1,    90,
       3,     4,    -1,    -1,    -1,    -1,     9,    10,    31,    12,
      -1,    -1,    -1,    16,    -1,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    -1,    30,    -1,    32,
      -1,    34,    35,    36,    37,    38,    -1,    -1,    -1,    -1,
      43,    -1,     5,     6,     7,     8,    49,    -1,    -1,    52,
      13,    14,    15,    -1,    57,    58,    59,    -1,    -1,    62,
      63,    -1,    -1,     1,    67,     3,     4,    -1,    31,    -1,
      -1,     9,    10,    -1,    12,    55,    56,    57,    58,    59,
      60,    61,    85,    -1,    87,    88,    -1,    90,    -1,    -1,
      -1,    -1,    30,    -1,    32,    -1,    34,    35,    36,    37,
      38,    -1,    -1,    -1,    -1,    43,    -1,     5,     6,     7,
       8,    49,    -1,    -1,    52,    13,    14,    15,    -1,    57,
      58,    59,    -1,    -1,    62,    63,    -1,    -1,    66,    67,
      68,     1,    -1,     3,    -1,    -1,    -1,    -1,    -1,     9,
      10,    -1,    12,    -1,    -1,    -1,    -1,    -1,    86,    87,
      88,    -1,    90,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      30,    -1,    32,    -1,    34,    35,    36,    37,    38,    -1,
      -1,    -1,    -1,    43,    44,    -1,    -1,    -1,    -1,    49,
      -1,    -1,    52,    -1,    -1,    -1,    -1,    57,    58,    59,
      -1,    -1,    62,    63,    -1,    -1,    66,    67,    68,     1,
      -1,     3,    -1,    -1,    -1,    -1,    -1,     9,    10,    -1,
      12,    -1,     4,     5,     6,     7,     8,    87,    88,    -1,
      90,    13,    14,    15,    -1,    -1,    -1,    -1,    30,    -1,
      32,    -1,    34,    35,    36,    37,    38,    29,    -1,    31,
      -1,    43,     4,     5,     6,     7,     8,    49,    -1,    -1,
      52,    13,    14,    15,    -1,    57,    58,    59,    -1,    -1,
      62,    63,     1,    -1,     3,    67,    -1,    29,    -1,    31,
       9,    10,    -1,    12,     5,     6,     7,     8,    -1,    -1,
      -1,    -1,    13,    14,    15,    87,    88,    -1,    90,    -1,
      -1,    30,    -1,    32,    -1,    34,    35,    36,    37,    38,
      31,    -1,    -1,    -1,    43,     4,     5,     6,     7,     8,
      49,    -1,    -1,    52,    13,    14,    15,    -1,    57,    58,
      59,    -1,    -1,    62,    63,     3,    -1,    -1,    67,    -1,
      29,     9,    10,    -1,    12,    -1,     4,     5,     6,     7,
       8,    -1,    -1,    -1,    -1,    13,    14,    15,    87,    88,
      -1,    90,    30,    -1,    32,    -1,    34,    35,    36,    37,
      38,    29,    -1,    31,    -1,    43,     3,    -1,    -1,    -1,
      -1,    49,     9,    10,    52,    12,    -1,    -1,    -1,    57,
      58,    59,    -1,    -1,    62,    63,    -1,    -1,    -1,    67,
      -1,    -1,    -1,    30,    -1,    32,    -1,    34,    35,    36,
      37,    38,    -1,    -1,    -1,    -1,    43,    -1,    -1,    87,
      88,    -1,    49,    91,    -1,    52,    -1,    -1,    -1,    -1,
      57,    58,    59,    -1,     4,    62,    63,     7,     8,    -1,
      67,    -1,    -1,    13,    14,    15,     3,     4,     5,     6,
       7,     8,     9,    10,    -1,    12,    13,    14,    15,    29,
      87,    88,    -1,    -1,    91,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    -1,    34,    35,    36,
      37,    38,    -1,    -1,    -1,    -1,    43,     4,     5,     6,
       7,     8,    49,    -1,    -1,    52,    13,    14,    15,    -1,
      57,    58,    59,    -1,    -1,    62,    63,    -1,    -1,    -1,
      67,    -1,    29,    -1,    31,     3,     4,    -1,    -1,    -1,
      -1,     9,    10,    -1,    12,    -1,    -1,    -1,    85,    -1,
      87,    88,    -1,    90,    22,    23,    24,    25,    26,    27,
      28,    -1,    30,    -1,    32,    -1,    34,    35,    36,    37,
      38,    -1,    -1,    -1,    -1,    43,     4,     5,     6,     7,
       8,    49,    -1,    -1,    52,    13,    14,    15,    -1,    57,
      58,    59,    -1,    -1,    62,    63,     3,    -1,    -1,    67,
      -1,    29,     9,    10,    -1,    12,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    85,    -1,    87,
      88,    -1,    90,    30,    -1,    32,    -1,    34,    35,    36,
      37,    38,    -1,    -1,    -1,    -1,    43,    -1,    -1,    -1,
      -1,    -1,    49,    -1,    -1,    52,    -1,    -1,    -1,    -1,
      57,    58,    59,    -1,    -1,    62,    63,    -1,    -1,    -1,
      67,     3,     4,     5,     6,     7,     8,     9,    10,    -1,
      12,    13,    14,    15,    -1,     4,    -1,    -1,     7,     8,
      87,    88,    -1,    90,    13,    14,    15,    29,    30,    31,
      32,    -1,    34,    35,    36,    37,    38,    -1,    -1,    -1,
      29,    43,    31,    -1,    -1,    -1,    -1,    49,    -1,    -1,
      52,    -1,    -1,    -1,    -1,    57,    58,    59,    -1,    -1,
      62,    63,     3,     4,    -1,    67,     7,     8,     9,    10,
      -1,    12,    13,    14,    15,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    87,    88,    -1,    29,    30,
      31,    32,    -1,    34,    35,    36,    37,    38,    -1,    -1,
      -1,    -1,    43,     3,    -1,    -1,    -1,    -1,    49,     9,
      10,    52,    12,    -1,    -1,    -1,    57,    58,    59,    -1,
      -1,    62,    63,    -1,    -1,    -1,    67,    -1,    -1,    -1,
      30,    -1,    32,    -1,    34,    35,    36,    37,    38,    -1,
      -1,    -1,    -1,    43,     3,    -1,    87,    88,    -1,    49,
       9,    10,    52,    12,    -1,    -1,    -1,    57,    58,    59,
      -1,    -1,    62,    63,    -1,    -1,    -1,    67,    -1,    -1,
      -1,    30,    -1,    32,    -1,    34,    35,    36,    37,    38,
      -1,    -1,    -1,    -1,    43,    85,    -1,    87,    88,    -1,
      49,    -1,    -1,    52,    -1,    -1,    -1,    -1,    57,    58,
      59,    -1,    -1,    62,    63,     3,    -1,    -1,    67,    -1,
      -1,     9,    10,    -1,    12,    -1,     4,    -1,    -1,     7,
       8,    -1,    -1,    -1,    -1,    13,    14,    15,    87,    88,
      -1,    -1,    30,    -1,    32,    -1,    34,    35,    36,    37,
      38,    29,    -1,    31,    -1,    43,     3,    -1,    -1,    -1,
      -1,    49,     9,    10,    52,    12,    -1,    -1,    -1,    57,
      58,    59,    -1,    -1,    62,    63,    -1,    -1,    -1,    67,
      -1,    -1,    -1,    30,    -1,    32,    -1,    34,    35,    36,
      37,    38,    -1,    -1,    -1,    -1,    43,     3,    -1,    87,
      88,    -1,    49,     9,    10,    52,    12,    -1,    -1,    -1,
      57,    58,    59,    -1,    -1,    62,    63,    -1,    -1,    -1,
      67,    -1,    -1,    -1,    30,    11,    32,    -1,    34,    35,
      36,    37,    38,    -1,    -1,    -1,    -1,    43,    -1,    -1,
      87,    88,    -1,    49,    -1,    -1,    52,    -1,    -1,    -1,
      11,    57,    58,    59,    -1,    -1,    62,    63,    44,    45,
      46,    67,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    -1,    -1,    -1,    -1,
      -1,    87,    88,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    -1,    44,    45,    46,    91,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      44,    45,    46,    -1,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    44,    45,    46,    91,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    -1,    89,    44,    45,    46,    -1,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    -1,    -1,    -1,    -1,    44,    45,
      46,    89,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    -1,    -1,    -1,    -1,
      -1,    89,    31,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    44,    45,    46,    84,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,     4,     5,     6,     7,     8,    -1,    -1,
      -1,    -1,    13,    14,    15,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    29,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    44,    45,    46,
      -1,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    46,    -1,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned short yystos[] =
{
       0,    93,    94,    95,     0,    96,     1,     4,     5,     6,
       7,     8,    13,    14,    15,    28,    29,    31,    32,    85,
      97,    98,    99,   100,   115,   131,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,   157,   159,   160,   161,   162,   163,
     172,   173,   177,   200,   201,   202,   203,   208,   293,    97,
      85,    86,   172,   172,   172,    67,    67,    67,     3,    59,
      67,   165,   169,   199,     8,   159,   160,   172,   177,     8,
     159,   160,   177,     8,   159,   160,   172,   177,     8,   159,
     160,   177,     8,   161,   162,   172,   177,     8,   161,   162,
     177,     8,   161,   162,   172,   177,     8,   161,   162,   177,
       8,   159,   160,   172,   177,     8,   159,   160,   177,     8,
     159,   160,   172,   177,     8,   159,   160,   177,     8,   161,
     162,   172,   177,     8,   161,   162,   177,     8,   161,   162,
     172,   177,     8,   161,   162,   177,   131,   131,    85,   173,
       3,     4,    90,   107,    90,   107,    90,   107,    97,     3,
       9,    10,    12,    30,    34,    35,    36,    37,    38,    43,
      49,    52,    57,    58,    59,    62,    63,    67,    87,    88,
     108,   109,   111,   112,   113,   114,   116,   117,   123,   247,
     293,    67,   109,   134,   135,   136,   137,   138,   139,   140,
     141,   156,   220,   134,   135,   136,   137,   155,   158,   171,
     172,    85,    89,     1,    28,    67,    68,   105,   166,   228,
       4,    59,    67,   164,   167,   194,   195,   199,   165,   199,
     211,   212,    90,   211,    90,   207,    90,   116,   116,    67,
      67,    67,   107,   116,     1,    90,   109,   220,   116,    84,
      89,    67,   112,    67,   112,    44,    45,    46,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    66,    67,    68,    69,     1,    86,
     236,   245,   116,     7,     8,   107,   174,   175,   176,   177,
      84,   221,    84,   199,   199,   132,   171,    67,   171,   288,
       6,   155,   158,     1,   125,   126,   127,   128,   235,   254,
     171,   158,   171,    85,    89,     1,   101,   166,    67,   228,
      85,     1,   103,    86,     1,    85,   134,   135,   136,   137,
     138,   139,   140,   141,   154,   155,   213,   293,   204,    86,
     205,     1,   107,   218,   219,   206,   117,   117,   220,    84,
      84,    84,    85,   117,   220,   220,   117,   117,   120,   122,
     119,   118,   117,   117,   117,   117,   117,   117,   117,   117,
     117,   117,   117,   117,   107,   110,   111,   109,   107,    84,
      33,   240,   241,   242,    84,    84,    89,    67,    59,    67,
     222,   224,   225,   226,   227,   228,    84,   169,   199,    10,
     289,   158,     6,    59,    91,   109,    85,   106,   235,   127,
     255,    44,   195,   195,   132,   125,   171,   288,   125,   171,
     131,   131,    85,   213,   211,   171,   211,    44,    89,   210,
     218,    89,    89,    89,    90,   116,    84,    84,   109,    47,
     117,   117,    84,    91,   107,   292,     1,   130,   229,   230,
     231,   232,   233,   234,   235,   246,   254,   258,   259,   242,
      84,   175,     3,   110,   158,   171,   278,    67,   228,    84,
       1,     3,    11,   134,   135,   138,   139,   142,   143,   146,
     147,   152,   153,   280,   283,   284,   286,   290,   291,   109,
     109,    91,    91,   254,   129,   152,   153,   170,    84,   167,
     194,   102,    44,   104,   214,   216,   254,   215,   217,   254,
      86,    86,   117,   219,    86,   210,   220,   117,   220,   124,
      47,   117,    85,    89,   130,   258,   259,   130,   258,   259,
     254,   258,   259,   130,   258,   259,   235,    86,   255,    84,
      89,    84,   225,   226,   224,   279,   171,   278,    84,   131,
     287,   287,    84,    85,    89,    84,    89,    91,    91,   255,
      85,   131,    85,   131,     1,    90,   117,   178,   254,   168,
     254,    89,   255,    89,   255,   171,   171,   171,    86,    84,
      89,    84,     1,     3,    66,    68,    90,   107,   117,   180,
     181,   182,   184,   186,   187,   121,   107,   255,   237,     4,
      22,    23,    24,    25,    26,    27,    28,    85,    90,   107,
     109,   133,   150,   151,   157,   238,   244,   248,   270,   271,
     293,   111,    84,   280,     4,    59,    67,   196,   197,   198,
     199,   223,   224,   225,    59,    67,   199,   223,   281,    11,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   147,   148,   149,   150,   151,   285,     3,
       1,   243,   248,   164,   165,   179,   255,   178,   255,   132,
      47,   194,   132,    47,   199,   171,   117,   107,   117,   185,
      47,    86,    89,   209,    44,   187,   184,   117,   117,    47,
      85,    85,    85,   109,    59,   107,     8,   272,   254,    85,
     131,   131,    85,    16,    18,    19,    20,    21,   249,   250,
     252,   260,   245,   133,    84,   158,   171,   171,    67,   228,
      67,   228,   171,   172,   158,   171,   171,   171,   131,   131,
     248,    85,    85,   180,   243,   243,   216,   254,   117,    47,
     171,   217,   117,    47,   171,    84,    11,    91,   180,   183,
     182,   184,    11,    47,    85,   109,    85,    67,   255,   164,
     188,   194,   165,   191,   199,   251,   262,   253,   264,    67,
      17,     1,   238,   257,     1,    67,   239,   197,   198,   198,
     288,   288,   282,   196,   199,   223,   199,   223,    86,   255,
     171,   117,   171,   117,   117,    86,   184,   117,    85,   109,
      47,    85,   189,    85,   192,    67,    67,   257,    67,   109,
       1,   261,   256,   258,   259,   109,    84,   280,   171,   171,
     171,   171,   171,    91,    47,    47,    84,   171,   125,   125,
     109,   109,    18,   109,   133,   269,   273,    84,   257,   239,
     256,    84,    10,    68,   274,   275,   276,    85,   190,   193,
      84,    84,   265,    85,   268,    85,    67,   107,    47,    84,
      89,   254,   254,   263,   273,   257,   109,    91,   274,    85,
     276,   255,   255,   257,    85,    84,    10,    47,    84,   248,
     248,   266,    67,    10,   277,    85,   273,   109,    84,    89,
      84,    84,    85,    10,   267,   257
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
#line 324 "c-parse.y"
    { if (pedantic)
		    pedwarn ("ISO C forbids an empty source file");
		  finish_file ();
		;}
    break;

  case 3:
#line 329 "c-parse.y"
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
#line 347 "c-parse.y"
    {yyval.ttype = NULL_TREE; ;}
    break;

  case 6:
#line 348 "c-parse.y"
    {yyval.ttype = NULL_TREE; ggc_collect(); ;}
    break;

  case 8:
#line 353 "c-parse.y"
    { parsing_iso_function_signature = false; ;}
    break;

  case 11:
#line 360 "c-parse.y"
    { STRIP_NOPS (yyvsp[-2].ttype);
		  if ((TREE_CODE (yyvsp[-2].ttype) == ADDR_EXPR
		       && TREE_CODE (TREE_OPERAND (yyvsp[-2].ttype, 0)) == STRING_CST)
		      || TREE_CODE (yyvsp[-2].ttype) == STRING_CST)
		    assemble_asm (yyvsp[-2].ttype);
		  else
		    error ("argument of `asm' is not a constant string"); ;}
    break;

  case 12:
#line 368 "c-parse.y"
    { RESTORE_EXT_FLAGS (yyvsp[-1].ttype); ;}
    break;

  case 13:
#line 373 "c-parse.y"
    { if (pedantic)
		    error ("ISO C forbids data definition with no type or storage class");
		  else
		    warning ("data definition has no type or storage class");

		  POP_DECLSPEC_STACK; ;}
    break;

  case 14:
#line 380 "c-parse.y"
    { POP_DECLSPEC_STACK; ;}
    break;

  case 15:
#line 382 "c-parse.y"
    { POP_DECLSPEC_STACK; ;}
    break;

  case 16:
#line 384 "c-parse.y"
    { shadow_tag (yyvsp[-1].ttype); ;}
    break;

  case 19:
#line 388 "c-parse.y"
    { if (pedantic)
		    pedwarn ("ISO C does not allow extra `;' outside of a function"); ;}
    break;

  case 20:
#line 394 "c-parse.y"
    { if (! start_function (current_declspecs, yyvsp[0].ttype,
					all_prefix_attributes))
		    YYERROR1;
		;}
    break;

  case 21:
#line 399 "c-parse.y"
    { store_parm_decls (); ;}
    break;

  case 22:
#line 401 "c-parse.y"
    { DECL_SOURCE_FILE (current_function_decl) = yyvsp[-2].filename;
		  DECL_SOURCE_LINE (current_function_decl) = yyvsp[-1].lineno;
		  finish_function (0, 1);
		  POP_DECLSPEC_STACK; ;}
    break;

  case 23:
#line 406 "c-parse.y"
    { POP_DECLSPEC_STACK; ;}
    break;

  case 24:
#line 408 "c-parse.y"
    { if (! start_function (current_declspecs, yyvsp[0].ttype,
					all_prefix_attributes))
		    YYERROR1;
		;}
    break;

  case 25:
#line 413 "c-parse.y"
    { store_parm_decls (); ;}
    break;

  case 26:
#line 415 "c-parse.y"
    { DECL_SOURCE_FILE (current_function_decl) = yyvsp[-2].filename;
		  DECL_SOURCE_LINE (current_function_decl) = yyvsp[-1].lineno;
		  finish_function (0, 1);
		  POP_DECLSPEC_STACK; ;}
    break;

  case 27:
#line 420 "c-parse.y"
    { POP_DECLSPEC_STACK; ;}
    break;

  case 28:
#line 422 "c-parse.y"
    { if (! start_function (NULL_TREE, yyvsp[0].ttype,
					all_prefix_attributes))
		    YYERROR1;
		;}
    break;

  case 29:
#line 427 "c-parse.y"
    { store_parm_decls (); ;}
    break;

  case 30:
#line 429 "c-parse.y"
    { DECL_SOURCE_FILE (current_function_decl) = yyvsp[-2].filename;
		  DECL_SOURCE_LINE (current_function_decl) = yyvsp[-1].lineno;
		  finish_function (0, 1);
		  POP_DECLSPEC_STACK; ;}
    break;

  case 31:
#line 434 "c-parse.y"
    { POP_DECLSPEC_STACK; ;}
    break;

  case 34:
#line 443 "c-parse.y"
    { yyval.code = ADDR_EXPR; ;}
    break;

  case 35:
#line 445 "c-parse.y"
    { yyval.code = NEGATE_EXPR; ;}
    break;

  case 36:
#line 447 "c-parse.y"
    { yyval.code = CONVERT_EXPR;
  if (warn_traditional && !in_system_header)
    warning ("traditional C rejects the unary plus operator");
		;}
    break;

  case 37:
#line 452 "c-parse.y"
    { yyval.code = PREINCREMENT_EXPR; ;}
    break;

  case 38:
#line 454 "c-parse.y"
    { yyval.code = PREDECREMENT_EXPR; ;}
    break;

  case 39:
#line 456 "c-parse.y"
    { yyval.code = BIT_NOT_EXPR; ;}
    break;

  case 40:
#line 458 "c-parse.y"
    { yyval.code = TRUTH_NOT_EXPR; ;}
    break;

  case 41:
#line 462 "c-parse.y"
    { yyval.ttype = build_compound_expr (yyvsp[0].ttype); ;}
    break;

  case 42:
#line 467 "c-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 44:
#line 473 "c-parse.y"
    { yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ttype); ;}
    break;

  case 45:
#line 475 "c-parse.y"
    { chainon (yyvsp[-2].ttype, build_tree_list (NULL_TREE, yyvsp[0].ttype)); ;}
    break;

  case 47:
#line 481 "c-parse.y"
    { yyval.ttype = build_indirect_ref (yyvsp[0].ttype, "unary *"); ;}
    break;

  case 48:
#line 484 "c-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  RESTORE_EXT_FLAGS (yyvsp[-1].ttype); ;}
    break;

  case 49:
#line 487 "c-parse.y"
    { yyval.ttype = build_unary_op (yyvsp[-1].code, yyvsp[0].ttype, 0);
		  overflow_warning (yyval.ttype); ;}
    break;

  case 50:
#line 491 "c-parse.y"
    { yyval.ttype = finish_label_address_expr (yyvsp[0].ttype); ;}
    break;

  case 51:
#line 493 "c-parse.y"
    { skip_evaluation--;
		  if (TREE_CODE (yyvsp[0].ttype) == COMPONENT_REF
		      && DECL_C_BIT_FIELD (TREE_OPERAND (yyvsp[0].ttype, 1)))
		    error ("`sizeof' applied to a bit-field");
		  yyval.ttype = c_sizeof (TREE_TYPE (yyvsp[0].ttype)); ;}
    break;

  case 52:
#line 499 "c-parse.y"
    { skip_evaluation--;
		  yyval.ttype = c_sizeof (groktypename (yyvsp[-1].ttype)); ;}
    break;

  case 53:
#line 502 "c-parse.y"
    { skip_evaluation--;
		  yyval.ttype = c_alignof_expr (yyvsp[0].ttype); ;}
    break;

  case 54:
#line 505 "c-parse.y"
    { skip_evaluation--;
		  yyval.ttype = c_alignof (groktypename (yyvsp[-1].ttype)); ;}
    break;

  case 55:
#line 508 "c-parse.y"
    { yyval.ttype = build_unary_op (REALPART_EXPR, yyvsp[0].ttype, 0); ;}
    break;

  case 56:
#line 510 "c-parse.y"
    { yyval.ttype = build_unary_op (IMAGPART_EXPR, yyvsp[0].ttype, 0); ;}
    break;

  case 57:
#line 514 "c-parse.y"
    { skip_evaluation++; ;}
    break;

  case 58:
#line 518 "c-parse.y"
    { skip_evaluation++; ;}
    break;

  case 59:
#line 522 "c-parse.y"
    { skip_evaluation++; ;}
    break;

  case 61:
#line 528 "c-parse.y"
    { yyval.ttype = c_cast_expr (yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 63:
#line 534 "c-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 64:
#line 536 "c-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 65:
#line 538 "c-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 66:
#line 540 "c-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 67:
#line 542 "c-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 68:
#line 544 "c-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 69:
#line 546 "c-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 70:
#line 548 "c-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 71:
#line 550 "c-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 72:
#line 552 "c-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 73:
#line 554 "c-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 74:
#line 556 "c-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 75:
#line 558 "c-parse.y"
    { yyvsp[-1].ttype = c_common_truthvalue_conversion
		    (default_conversion (yyvsp[-1].ttype));
		  skip_evaluation += yyvsp[-1].ttype == boolean_false_node; ;}
    break;

  case 76:
#line 562 "c-parse.y"
    { skip_evaluation -= yyvsp[-3].ttype == boolean_false_node;
		  yyval.ttype = parser_build_binary_op (TRUTH_ANDIF_EXPR, yyvsp[-3].ttype, yyvsp[0].ttype); ;}
    break;

  case 77:
#line 565 "c-parse.y"
    { yyvsp[-1].ttype = c_common_truthvalue_conversion
		    (default_conversion (yyvsp[-1].ttype));
		  skip_evaluation += yyvsp[-1].ttype == boolean_true_node; ;}
    break;

  case 78:
#line 569 "c-parse.y"
    { skip_evaluation -= yyvsp[-3].ttype == boolean_true_node;
		  yyval.ttype = parser_build_binary_op (TRUTH_ORIF_EXPR, yyvsp[-3].ttype, yyvsp[0].ttype); ;}
    break;

  case 79:
#line 572 "c-parse.y"
    { yyvsp[-1].ttype = c_common_truthvalue_conversion
		    (default_conversion (yyvsp[-1].ttype));
		  skip_evaluation += yyvsp[-1].ttype == boolean_false_node; ;}
    break;

  case 80:
#line 576 "c-parse.y"
    { skip_evaluation += ((yyvsp[-4].ttype == boolean_true_node)
				      - (yyvsp[-4].ttype == boolean_false_node)); ;}
    break;

  case 81:
#line 579 "c-parse.y"
    { skip_evaluation -= yyvsp[-6].ttype == boolean_true_node;
		  yyval.ttype = build_conditional_expr (yyvsp[-6].ttype, yyvsp[-3].ttype, yyvsp[0].ttype); ;}
    break;

  case 82:
#line 582 "c-parse.y"
    { if (pedantic)
		    pedwarn ("ISO C forbids omitting the middle term of a ?: expression");
		  /* Make sure first operand is calculated only once.  */
		  yyvsp[0].ttype = save_expr (yyvsp[-1].ttype);
		  yyvsp[-1].ttype = c_common_truthvalue_conversion
		    (default_conversion (yyvsp[0].ttype));
		  skip_evaluation += yyvsp[-1].ttype == boolean_true_node; ;}
    break;

  case 83:
#line 590 "c-parse.y"
    { skip_evaluation -= yyvsp[-4].ttype == boolean_true_node;
		  yyval.ttype = build_conditional_expr (yyvsp[-4].ttype, yyvsp[-3].ttype, yyvsp[0].ttype); ;}
    break;

  case 84:
#line 593 "c-parse.y"
    { char class;
		  yyval.ttype = build_modify_expr (yyvsp[-2].ttype, NOP_EXPR, yyvsp[0].ttype);
		  class = TREE_CODE_CLASS (TREE_CODE (yyval.ttype));
		  if (IS_EXPR_CODE_CLASS (class))
		    C_SET_EXP_ORIGINAL_CODE (yyval.ttype, MODIFY_EXPR);
		;}
    break;

  case 85:
#line 600 "c-parse.y"
    { char class;
		  yyval.ttype = build_modify_expr (yyvsp[-2].ttype, yyvsp[-1].code, yyvsp[0].ttype);
		  /* This inhibits warnings in
		     c_common_truthvalue_conversion.  */
		  class = TREE_CODE_CLASS (TREE_CODE (yyval.ttype));
		  if (IS_EXPR_CODE_CLASS (class))
		    C_SET_EXP_ORIGINAL_CODE (yyval.ttype, ERROR_MARK);
		;}
    break;

  case 86:
#line 612 "c-parse.y"
    {
		  if (yychar == YYEMPTY)
		    yychar = YYLEX;
		  yyval.ttype = build_external_ref (yyvsp[0].ttype, yychar == '(');
		;}
    break;

  case 88:
#line 619 "c-parse.y"
    { yyval.ttype = fix_string_type (yyval.ttype); ;}
    break;

  case 89:
#line 621 "c-parse.y"
    { yyval.ttype = fname_decl (C_RID_CODE (yyval.ttype), yyval.ttype); ;}
    break;

  case 90:
#line 623 "c-parse.y"
    { start_init (NULL_TREE, NULL, 0);
		  yyvsp[-2].ttype = groktypename (yyvsp[-2].ttype);
		  really_start_incremental_init (yyvsp[-2].ttype); ;}
    break;

  case 91:
#line 627 "c-parse.y"
    { tree constructor = pop_init_level (0);
		  tree type = yyvsp[-5].ttype;
		  finish_init ();

		  if (pedantic && ! flag_isoc99)
		    pedwarn ("ISO C89 forbids compound literals");
		  yyval.ttype = build_compound_literal (type, constructor);
		;}
    break;

  case 92:
#line 636 "c-parse.y"
    { char class = TREE_CODE_CLASS (TREE_CODE (yyvsp[-1].ttype));
		  if (IS_EXPR_CODE_CLASS (class))
		    C_SET_EXP_ORIGINAL_CODE (yyvsp[-1].ttype, ERROR_MARK);
		  yyval.ttype = yyvsp[-1].ttype; ;}
    break;

  case 93:
#line 641 "c-parse.y"
    { yyval.ttype = error_mark_node; ;}
    break;

  case 94:
#line 643 "c-parse.y"
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

  case 95:
#line 659 "c-parse.y"
    {
		  pop_label_level ();
		  last_tree = COMPOUND_BODY (yyvsp[-2].ttype);
		  TREE_CHAIN (last_tree) = NULL_TREE;
		  yyval.ttype = error_mark_node;
		;}
    break;

  case 96:
#line 666 "c-parse.y"
    { yyval.ttype = build_function_call (yyvsp[-3].ttype, yyvsp[-1].ttype); ;}
    break;

  case 97:
#line 668 "c-parse.y"
    { yyval.ttype = build_va_arg (yyvsp[-3].ttype, groktypename (yyvsp[-1].ttype)); ;}
    break;

  case 98:
#line 671 "c-parse.y"
    {
                  tree c;

                  c = fold (yyvsp[-5].ttype);
                  STRIP_NOPS (c);
                  if (TREE_CODE (c) != INTEGER_CST)
                    error ("first argument to __builtin_choose_expr not a constant");
                  yyval.ttype = integer_zerop (c) ? yyvsp[-1].ttype : yyvsp[-3].ttype;
		;}
    break;

  case 99:
#line 681 "c-parse.y"
    {
		  tree e1, e2;

		  e1 = TYPE_MAIN_VARIANT (groktypename (yyvsp[-3].ttype));
		  e2 = TYPE_MAIN_VARIANT (groktypename (yyvsp[-1].ttype));

		  yyval.ttype = comptypes (e1, e2)
		    ? build_int_2 (1, 0) : build_int_2 (0, 0);
		;}
    break;

  case 100:
#line 691 "c-parse.y"
    { yyval.ttype = build_array_ref (yyvsp[-3].ttype, yyvsp[-1].ttype); ;}
    break;

  case 101:
#line 693 "c-parse.y"
    {
		      yyval.ttype = build_component_ref (yyvsp[-2].ttype, yyvsp[0].ttype);
		;}
    break;

  case 102:
#line 697 "c-parse.y"
    {
                  tree expr = build_indirect_ref (yyvsp[-2].ttype, "->");

			yyval.ttype = build_component_ref (expr, yyvsp[0].ttype);
		;}
    break;

  case 103:
#line 703 "c-parse.y"
    { yyval.ttype = build_unary_op (POSTINCREMENT_EXPR, yyvsp[-1].ttype, 0); ;}
    break;

  case 104:
#line 705 "c-parse.y"
    { yyval.ttype = build_unary_op (POSTDECREMENT_EXPR, yyvsp[-1].ttype, 0); ;}
    break;

  case 105:
#line 711 "c-parse.y"
    {
	  parsing_iso_function_signature = false; /* Reset after decls.  */
	;}
    break;

  case 106:
#line 718 "c-parse.y"
    {
	  if (warn_traditional && !in_system_header
	      && parsing_iso_function_signature)
	    warning ("traditional C rejects ISO C style function definitions");
	  parsing_iso_function_signature = false; /* Reset after warning.  */
	;}
    break;

  case 108:
#line 732 "c-parse.y"
    { ;}
    break;

  case 113:
#line 748 "c-parse.y"
    { POP_DECLSPEC_STACK; ;}
    break;

  case 114:
#line 750 "c-parse.y"
    { POP_DECLSPEC_STACK; ;}
    break;

  case 115:
#line 752 "c-parse.y"
    { shadow_tag_warned (yyvsp[-1].ttype, 1);
		  pedwarn ("empty declaration"); ;}
    break;

  case 116:
#line 755 "c-parse.y"
    { pedwarn ("empty declaration"); ;}
    break;

  case 117:
#line 764 "c-parse.y"
    { ;}
    break;

  case 118:
#line 772 "c-parse.y"
    { pending_xref_error ();
		  PUSH_DECLSPEC_STACK;
		  split_specs_attrs (yyvsp[0].ttype,
				     &current_declspecs, &prefix_attributes);
		  all_prefix_attributes = prefix_attributes; ;}
    break;

  case 119:
#line 783 "c-parse.y"
    { all_prefix_attributes = chainon (yyvsp[0].ttype, prefix_attributes); ;}
    break;

  case 120:
#line 788 "c-parse.y"
    { POP_DECLSPEC_STACK; ;}
    break;

  case 121:
#line 790 "c-parse.y"
    { POP_DECLSPEC_STACK; ;}
    break;

  case 122:
#line 792 "c-parse.y"
    { POP_DECLSPEC_STACK; ;}
    break;

  case 123:
#line 794 "c-parse.y"
    { POP_DECLSPEC_STACK; ;}
    break;

  case 124:
#line 796 "c-parse.y"
    { shadow_tag (yyvsp[-1].ttype); ;}
    break;

  case 125:
#line 798 "c-parse.y"
    { RESTORE_EXT_FLAGS (yyvsp[-1].ttype); ;}
    break;

  case 126:
#line 855 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, NULL_TREE);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 127:
#line 858 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 128:
#line 861 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 129:
#line 867 "c-parse.y"
    { yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 130:
#line 873 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 131:
#line 876 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 132:
#line 882 "c-parse.y"
    { yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, NULL_TREE);
		  TREE_STATIC (yyval.ttype) = 0; ;}
    break;

  case 133:
#line 885 "c-parse.y"
    { yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 134:
#line 891 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, NULL_TREE);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 135:
#line 894 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 136:
#line 897 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 137:
#line 900 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 138:
#line 903 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 139:
#line 906 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 140:
#line 909 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 141:
#line 915 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, NULL_TREE);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 142:
#line 918 "c-parse.y"
    { yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 143:
#line 921 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 144:
#line 924 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 145:
#line 927 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 146:
#line 930 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 147:
#line 936 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 148:
#line 939 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 149:
#line 942 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 150:
#line 945 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 151:
#line 948 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 152:
#line 951 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 153:
#line 957 "c-parse.y"
    { yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 154:
#line 960 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 155:
#line 963 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 156:
#line 966 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 157:
#line 969 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 158:
#line 975 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, NULL_TREE);
		  TREE_STATIC (yyval.ttype) = 0; ;}
    break;

  case 159:
#line 978 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 160:
#line 981 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 161:
#line 984 "c-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 162:
#line 990 "c-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 163:
#line 996 "c-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 164:
#line 1002 "c-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 165:
#line 1011 "c-parse.y"
    { yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 166:
#line 1017 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 167:
#line 1020 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 168:
#line 1023 "c-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 169:
#line 1029 "c-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 170:
#line 1035 "c-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 171:
#line 1041 "c-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 172:
#line 1050 "c-parse.y"
    { yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 173:
#line 1056 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 174:
#line 1059 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 175:
#line 1062 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 176:
#line 1065 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 177:
#line 1068 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 178:
#line 1071 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 179:
#line 1074 "c-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 180:
#line 1080 "c-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 181:
#line 1086 "c-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 182:
#line 1092 "c-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 183:
#line 1101 "c-parse.y"
    { yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 184:
#line 1104 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 185:
#line 1107 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 186:
#line 1110 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 187:
#line 1113 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 188:
#line 1119 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 189:
#line 1122 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 190:
#line 1125 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 191:
#line 1128 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 192:
#line 1131 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 193:
#line 1134 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 194:
#line 1137 "c-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 195:
#line 1143 "c-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 196:
#line 1149 "c-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 197:
#line 1155 "c-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 198:
#line 1164 "c-parse.y"
    { yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 199:
#line 1167 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 200:
#line 1170 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 201:
#line 1173 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 202:
#line 1176 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 259:
#line 1264 "c-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 260:
#line 1266 "c-parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 264:
#line 1301 "c-parse.y"
    { OBJC_NEED_RAW_IDENTIFIER (1);	;}
    break;

  case 267:
#line 1311 "c-parse.y"
    { /* For a typedef name, record the meaning, not the name.
		     In case of `foo foo, bar;'.  */
		  yyval.ttype = lookup_name (yyvsp[0].ttype); ;}
    break;

  case 268:
#line 1315 "c-parse.y"
    { skip_evaluation--; yyval.ttype = TREE_TYPE (yyvsp[-1].ttype); ;}
    break;

  case 269:
#line 1317 "c-parse.y"
    { skip_evaluation--; yyval.ttype = groktypename (yyvsp[-1].ttype); ;}
    break;

  case 274:
#line 1334 "c-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 275:
#line 1336 "c-parse.y"
    { yyval.ttype = yyvsp[-1].ttype; ;}
    break;

  case 276:
#line 1341 "c-parse.y"
    { yyval.ttype = start_decl (yyvsp[-3].ttype, current_declspecs, 1,
					  chainon (yyvsp[-1].ttype, all_prefix_attributes));
		  start_init (yyval.ttype, yyvsp[-2].ttype, global_bindings_p ()); ;}
    break;

  case 277:
#line 1346 "c-parse.y"
    { finish_init ();
		  finish_decl (yyvsp[-1].ttype, yyvsp[0].ttype, yyvsp[-4].ttype); ;}
    break;

  case 278:
#line 1349 "c-parse.y"
    { tree d = start_decl (yyvsp[-2].ttype, current_declspecs, 0,
				       chainon (yyvsp[0].ttype, all_prefix_attributes));
		  finish_decl (d, NULL_TREE, yyvsp[-1].ttype);
                ;}
    break;

  case 279:
#line 1357 "c-parse.y"
    { yyval.ttype = start_decl (yyvsp[-3].ttype, current_declspecs, 1,
					  chainon (yyvsp[-1].ttype, all_prefix_attributes));
		  start_init (yyval.ttype, yyvsp[-2].ttype, global_bindings_p ()); ;}
    break;

  case 280:
#line 1362 "c-parse.y"
    { finish_init ();
		  finish_decl (yyvsp[-1].ttype, yyvsp[0].ttype, yyvsp[-4].ttype); ;}
    break;

  case 281:
#line 1365 "c-parse.y"
    { tree d = start_decl (yyvsp[-2].ttype, current_declspecs, 0,
				       chainon (yyvsp[0].ttype, all_prefix_attributes));
		  finish_decl (d, NULL_TREE, yyvsp[-1].ttype); ;}
    break;

  case 282:
#line 1373 "c-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 283:
#line 1375 "c-parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 284:
#line 1380 "c-parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 285:
#line 1382 "c-parse.y"
    { yyval.ttype = chainon (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 286:
#line 1387 "c-parse.y"
    { yyval.ttype = yyvsp[-2].ttype; ;}
    break;

  case 287:
#line 1392 "c-parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 288:
#line 1394 "c-parse.y"
    { yyval.ttype = chainon (yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 289:
#line 1399 "c-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 290:
#line 1401 "c-parse.y"
    { yyval.ttype = build_tree_list (yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 291:
#line 1403 "c-parse.y"
    { yyval.ttype = build_tree_list (yyvsp[-3].ttype, build_tree_list (NULL_TREE, yyvsp[-1].ttype)); ;}
    break;

  case 292:
#line 1405 "c-parse.y"
    { yyval.ttype = build_tree_list (yyvsp[-5].ttype, tree_cons (NULL_TREE, yyvsp[-3].ttype, yyvsp[-1].ttype)); ;}
    break;

  case 293:
#line 1407 "c-parse.y"
    { yyval.ttype = build_tree_list (yyvsp[-3].ttype, yyvsp[-1].ttype); ;}
    break;

  case 301:
#line 1430 "c-parse.y"
    { really_start_incremental_init (NULL_TREE); ;}
    break;

  case 302:
#line 1432 "c-parse.y"
    { yyval.ttype = pop_init_level (0); ;}
    break;

  case 303:
#line 1434 "c-parse.y"
    { yyval.ttype = error_mark_node; ;}
    break;

  case 304:
#line 1440 "c-parse.y"
    { if (pedantic)
		    pedwarn ("ISO C forbids empty initializer braces"); ;}
    break;

  case 308:
#line 1454 "c-parse.y"
    { if (pedantic && ! flag_isoc99)
		    pedwarn ("ISO C89 forbids specifying subobject to initialize"); ;}
    break;

  case 309:
#line 1457 "c-parse.y"
    { if (pedantic)
		    pedwarn ("obsolete use of designated initializer without `='"); ;}
    break;

  case 310:
#line 1460 "c-parse.y"
    { set_init_label (yyvsp[-1].ttype);
		  if (pedantic)
		    pedwarn ("obsolete use of designated initializer with `:'"); ;}
    break;

  case 311:
#line 1464 "c-parse.y"
    {;}
    break;

  case 313:
#line 1470 "c-parse.y"
    { push_init_level (0); ;}
    break;

  case 314:
#line 1472 "c-parse.y"
    { process_init_element (pop_init_level (0)); ;}
    break;

  case 315:
#line 1474 "c-parse.y"
    { process_init_element (yyvsp[0].ttype); ;}
    break;

  case 319:
#line 1485 "c-parse.y"
    { set_init_label (yyvsp[0].ttype); ;}
    break;

  case 320:
#line 1487 "c-parse.y"
    { set_init_index (yyvsp[-3].ttype, yyvsp[-1].ttype);
		  if (pedantic)
		    pedwarn ("ISO C forbids specifying range of elements to initialize"); ;}
    break;

  case 321:
#line 1491 "c-parse.y"
    { set_init_index (yyvsp[-1].ttype, NULL_TREE); ;}
    break;

  case 322:
#line 1496 "c-parse.y"
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

  case 323:
#line 1509 "c-parse.y"
    { store_parm_decls (); ;}
    break;

  case 324:
#line 1517 "c-parse.y"
    { tree decl = current_function_decl;
		  DECL_SOURCE_FILE (decl) = yyvsp[-2].filename;
		  DECL_SOURCE_LINE (decl) = yyvsp[-1].lineno;
		  finish_function (1, 1);
		  pop_function_context ();
		  add_decl_stmt (decl); ;}
    break;

  case 325:
#line 1527 "c-parse.y"
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

  case 326:
#line 1540 "c-parse.y"
    { store_parm_decls (); ;}
    break;

  case 327:
#line 1548 "c-parse.y"
    { tree decl = current_function_decl;
		  DECL_SOURCE_FILE (decl) = yyvsp[-2].filename;
		  DECL_SOURCE_LINE (decl) = yyvsp[-1].lineno;
		  finish_function (1, 1);
		  pop_function_context ();
		  add_decl_stmt (decl); ;}
    break;

  case 330:
#line 1568 "c-parse.y"
    { yyval.ttype = yyvsp[-2].ttype ? tree_cons (yyvsp[-2].ttype, yyvsp[-1].ttype, NULL_TREE) : yyvsp[-1].ttype; ;}
    break;

  case 331:
#line 1570 "c-parse.y"
    { yyval.ttype = build_nt (CALL_EXPR, yyvsp[-2].ttype, yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 332:
#line 1575 "c-parse.y"
    { yyval.ttype = set_array_declarator_type (yyvsp[0].ttype, yyvsp[-1].ttype, 0); ;}
    break;

  case 333:
#line 1577 "c-parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 337:
#line 1592 "c-parse.y"
    { yyval.ttype = build_nt (CALL_EXPR, yyvsp[-2].ttype, yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 338:
#line 1597 "c-parse.y"
    { yyval.ttype = set_array_declarator_type (yyvsp[0].ttype, yyvsp[-1].ttype, 0); ;}
    break;

  case 340:
#line 1603 "c-parse.y"
    { yyval.ttype = build_nt (CALL_EXPR, yyvsp[-2].ttype, yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 341:
#line 1608 "c-parse.y"
    { yyval.ttype = set_array_declarator_type (yyvsp[0].ttype, yyvsp[-1].ttype, 0); ;}
    break;

  case 342:
#line 1610 "c-parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 343:
#line 1612 "c-parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 344:
#line 1614 "c-parse.y"
    { yyval.ttype = yyvsp[-2].ttype ? tree_cons (yyvsp[-2].ttype, yyvsp[-1].ttype, NULL_TREE) : yyvsp[-1].ttype; ;}
    break;

  case 345:
#line 1622 "c-parse.y"
    { yyval.ttype = build_nt (CALL_EXPR, yyvsp[-2].ttype, yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 346:
#line 1627 "c-parse.y"
    { yyval.ttype = yyvsp[-2].ttype ? tree_cons (yyvsp[-2].ttype, yyvsp[-1].ttype, NULL_TREE) : yyvsp[-1].ttype; ;}
    break;

  case 347:
#line 1629 "c-parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 348:
#line 1631 "c-parse.y"
    { yyval.ttype = set_array_declarator_type (yyvsp[0].ttype, yyvsp[-1].ttype, 0); ;}
    break;

  case 350:
#line 1637 "c-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 351:
#line 1639 "c-parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 352:
#line 1644 "c-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 353:
#line 1646 "c-parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 354:
#line 1651 "c-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 355:
#line 1653 "c-parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 356:
#line 1664 "c-parse.y"
    { yyval.ttype = start_struct (RECORD_TYPE, yyvsp[-1].ttype);
		  /* Start scope of tag before parsing components.  */
		;}
    break;

  case 357:
#line 1668 "c-parse.y"
    { yyval.ttype = finish_struct (yyvsp[-3].ttype, yyvsp[-2].ttype, chainon (yyvsp[-6].ttype, yyvsp[0].ttype)); ;}
    break;

  case 358:
#line 1670 "c-parse.y"
    { yyval.ttype = finish_struct (start_struct (RECORD_TYPE, NULL_TREE),
				      yyvsp[-2].ttype, chainon (yyvsp[-4].ttype, yyvsp[0].ttype));
		;}
    break;

  case 359:
#line 1674 "c-parse.y"
    { yyval.ttype = start_struct (UNION_TYPE, yyvsp[-1].ttype); ;}
    break;

  case 360:
#line 1676 "c-parse.y"
    { yyval.ttype = finish_struct (yyvsp[-3].ttype, yyvsp[-2].ttype, chainon (yyvsp[-6].ttype, yyvsp[0].ttype)); ;}
    break;

  case 361:
#line 1678 "c-parse.y"
    { yyval.ttype = finish_struct (start_struct (UNION_TYPE, NULL_TREE),
				      yyvsp[-2].ttype, chainon (yyvsp[-4].ttype, yyvsp[0].ttype));
		;}
    break;

  case 362:
#line 1682 "c-parse.y"
    { yyval.ttype = start_enum (yyvsp[-1].ttype); ;}
    break;

  case 363:
#line 1684 "c-parse.y"
    { yyval.ttype = finish_enum (yyvsp[-4].ttype, nreverse (yyvsp[-3].ttype),
				    chainon (yyvsp[-7].ttype, yyvsp[0].ttype)); ;}
    break;

  case 364:
#line 1687 "c-parse.y"
    { yyval.ttype = start_enum (NULL_TREE); ;}
    break;

  case 365:
#line 1689 "c-parse.y"
    { yyval.ttype = finish_enum (yyvsp[-4].ttype, nreverse (yyvsp[-3].ttype),
				    chainon (yyvsp[-6].ttype, yyvsp[0].ttype)); ;}
    break;

  case 366:
#line 1695 "c-parse.y"
    { yyval.ttype = xref_tag (RECORD_TYPE, yyvsp[0].ttype); ;}
    break;

  case 367:
#line 1697 "c-parse.y"
    { yyval.ttype = xref_tag (UNION_TYPE, yyvsp[0].ttype); ;}
    break;

  case 368:
#line 1699 "c-parse.y"
    { yyval.ttype = xref_tag (ENUMERAL_TYPE, yyvsp[0].ttype);
		  /* In ISO C, enumerated types can be referred to
		     only if already defined.  */
		  if (pedantic && !COMPLETE_TYPE_P (yyval.ttype))
		    pedwarn ("ISO C forbids forward references to `enum' types"); ;}
    break;

  case 372:
#line 1714 "c-parse.y"
    { if (pedantic && ! flag_isoc99)
		    pedwarn ("comma at end of enumerator list"); ;}
    break;

  case 373:
#line 1720 "c-parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 374:
#line 1722 "c-parse.y"
    { yyval.ttype = chainon (yyvsp[-1].ttype, yyvsp[0].ttype);
		  pedwarn ("no semicolon at end of struct or union"); ;}
    break;

  case 375:
#line 1727 "c-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 376:
#line 1729 "c-parse.y"
    { yyval.ttype = chainon (yyvsp[-2].ttype, yyvsp[-1].ttype); ;}
    break;

  case 377:
#line 1731 "c-parse.y"
    { if (pedantic)
		    pedwarn ("extra semicolon in struct or union specified"); ;}
    break;

  case 378:
#line 1737 "c-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  POP_DECLSPEC_STACK; ;}
    break;

  case 379:
#line 1740 "c-parse.y"
    {
		  /* Support for unnamed structs or unions as members of
		     structs or unions (which is [a] useful and [b] supports
		     MS P-SDK).  */
		  if (pedantic)
		    pedwarn ("ISO C doesn't support unnamed structs/unions");

		  yyval.ttype = grokfield(yyvsp[-1].filename, yyvsp[0].lineno, NULL, current_declspecs, NULL_TREE);
		  POP_DECLSPEC_STACK; ;}
    break;

  case 380:
#line 1750 "c-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  POP_DECLSPEC_STACK; ;}
    break;

  case 381:
#line 1753 "c-parse.y"
    { if (pedantic)
		    pedwarn ("ISO C forbids member declarations with no members");
		  shadow_tag(yyvsp[0].ttype);
		  yyval.ttype = NULL_TREE; ;}
    break;

  case 382:
#line 1758 "c-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 383:
#line 1760 "c-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  RESTORE_EXT_FLAGS (yyvsp[-1].ttype); ;}
    break;

  case 385:
#line 1767 "c-parse.y"
    { yyval.ttype = chainon (yyvsp[-3].ttype, yyvsp[0].ttype); ;}
    break;

  case 387:
#line 1773 "c-parse.y"
    { yyval.ttype = chainon (yyvsp[-3].ttype, yyvsp[0].ttype); ;}
    break;

  case 388:
#line 1778 "c-parse.y"
    { yyval.ttype = grokfield (yyvsp[-3].filename, yyvsp[-2].lineno, yyvsp[-1].ttype, current_declspecs, NULL_TREE);
		  decl_attributes (&yyval.ttype, chainon (yyvsp[0].ttype, all_prefix_attributes), 0); ;}
    break;

  case 389:
#line 1782 "c-parse.y"
    { yyval.ttype = grokfield (yyvsp[-5].filename, yyvsp[-4].lineno, yyvsp[-3].ttype, current_declspecs, yyvsp[-1].ttype);
		  decl_attributes (&yyval.ttype, chainon (yyvsp[0].ttype, all_prefix_attributes), 0); ;}
    break;

  case 390:
#line 1785 "c-parse.y"
    { yyval.ttype = grokfield (yyvsp[-4].filename, yyvsp[-3].lineno, NULL_TREE, current_declspecs, yyvsp[-1].ttype);
		  decl_attributes (&yyval.ttype, chainon (yyvsp[0].ttype, all_prefix_attributes), 0); ;}
    break;

  case 391:
#line 1791 "c-parse.y"
    { yyval.ttype = grokfield (yyvsp[-3].filename, yyvsp[-2].lineno, yyvsp[-1].ttype, current_declspecs, NULL_TREE);
		  decl_attributes (&yyval.ttype, chainon (yyvsp[0].ttype, all_prefix_attributes), 0); ;}
    break;

  case 392:
#line 1795 "c-parse.y"
    { yyval.ttype = grokfield (yyvsp[-5].filename, yyvsp[-4].lineno, yyvsp[-3].ttype, current_declspecs, yyvsp[-1].ttype);
		  decl_attributes (&yyval.ttype, chainon (yyvsp[0].ttype, all_prefix_attributes), 0); ;}
    break;

  case 393:
#line 1798 "c-parse.y"
    { yyval.ttype = grokfield (yyvsp[-4].filename, yyvsp[-3].lineno, NULL_TREE, current_declspecs, yyvsp[-1].ttype);
		  decl_attributes (&yyval.ttype, chainon (yyvsp[0].ttype, all_prefix_attributes), 0); ;}
    break;

  case 395:
#line 1810 "c-parse.y"
    { if (yyvsp[-2].ttype == error_mark_node)
		    yyval.ttype = yyvsp[-2].ttype;
		  else
		    yyval.ttype = chainon (yyvsp[0].ttype, yyvsp[-2].ttype); ;}
    break;

  case 396:
#line 1815 "c-parse.y"
    { yyval.ttype = error_mark_node; ;}
    break;

  case 397:
#line 1821 "c-parse.y"
    { yyval.ttype = build_enumerator (yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 398:
#line 1823 "c-parse.y"
    { yyval.ttype = build_enumerator (yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 399:
#line 1828 "c-parse.y"
    { pending_xref_error ();
		  yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 400:
#line 1831 "c-parse.y"
    { yyval.ttype = build_tree_list (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 401:
#line 1836 "c-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 403:
#line 1842 "c-parse.y"
    { yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 NULL_TREE),
					all_prefix_attributes); ;}
    break;

  case 404:
#line 1846 "c-parse.y"
    { yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 yyvsp[0].ttype),
					all_prefix_attributes); ;}
    break;

  case 405:
#line 1850 "c-parse.y"
    { yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 yyvsp[-1].ttype),
					chainon (yyvsp[0].ttype, all_prefix_attributes)); ;}
    break;

  case 409:
#line 1863 "c-parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 410:
#line 1868 "c-parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 411:
#line 1870 "c-parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 412:
#line 1875 "c-parse.y"
    { yyval.ttype = yyvsp[-2].ttype ? tree_cons (yyvsp[-2].ttype, yyvsp[-1].ttype, NULL_TREE) : yyvsp[-1].ttype; ;}
    break;

  case 413:
#line 1877 "c-parse.y"
    { yyval.ttype = build_nt (CALL_EXPR, yyvsp[-2].ttype, yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 414:
#line 1879 "c-parse.y"
    { yyval.ttype = set_array_declarator_type (yyvsp[0].ttype, yyvsp[-1].ttype, 1); ;}
    break;

  case 415:
#line 1881 "c-parse.y"
    { yyval.ttype = build_nt (CALL_EXPR, NULL_TREE, yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 416:
#line 1883 "c-parse.y"
    { yyval.ttype = set_array_declarator_type (yyvsp[0].ttype, NULL_TREE, 1); ;}
    break;

  case 417:
#line 1890 "c-parse.y"
    { yyval.ttype = build_array_declarator (yyvsp[-1].ttype, yyvsp[-2].ttype, 0, 0); ;}
    break;

  case 418:
#line 1892 "c-parse.y"
    { yyval.ttype = build_array_declarator (NULL_TREE, yyvsp[-1].ttype, 0, 0); ;}
    break;

  case 419:
#line 1894 "c-parse.y"
    { yyval.ttype = build_array_declarator (NULL_TREE, yyvsp[-2].ttype, 0, 1); ;}
    break;

  case 420:
#line 1896 "c-parse.y"
    { yyval.ttype = build_array_declarator (yyvsp[-1].ttype, yyvsp[-2].ttype, 1, 0); ;}
    break;

  case 421:
#line 1899 "c-parse.y"
    { yyval.ttype = build_array_declarator (yyvsp[-1].ttype, yyvsp[-3].ttype, 1, 0); ;}
    break;

  case 424:
#line 1912 "c-parse.y"
    {
		  pedwarn ("deprecated use of label at end of compound statement");
		;}
    break;

  case 432:
#line 1929 "c-parse.y"
    { if (pedantic && !flag_isoc99)
		    pedwarn ("ISO C89 forbids mixed declarations and code"); ;}
    break;

  case 447:
#line 1959 "c-parse.y"
    { pushlevel (0);
		  clear_last_expr ();
		  add_scope_stmt (/*begin_p=*/1, /*partial_p=*/0);
		;}
    break;

  case 448:
#line 1966 "c-parse.y"
    { yyval.ttype = add_scope_stmt (/*begin_p=*/0, /*partial_p=*/0); ;}
    break;

  case 449:
#line 1971 "c-parse.y"
    { if (flag_isoc99)
		    {
		      yyval.ttype = c_begin_compound_stmt ();
		      pushlevel (0);
		      clear_last_expr ();
		      add_scope_stmt (/*begin_p=*/1, /*partial_p=*/0);
		    }
		  else
		    yyval.ttype = NULL_TREE;
		;}
    break;

  case 450:
#line 1987 "c-parse.y"
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

  case 452:
#line 2004 "c-parse.y"
    { if (pedantic)
		    pedwarn ("ISO C forbids label declarations"); ;}
    break;

  case 455:
#line 2015 "c-parse.y"
    { tree link;
		  for (link = yyvsp[-1].ttype; link; link = TREE_CHAIN (link))
		    {
		      tree label = shadow_label (TREE_VALUE (link));
		      C_DECLARED_LABEL_FLAG (label) = 1;
		      add_decl_stmt (label);
		    }
		;}
    break;

  case 456:
#line 2029 "c-parse.y"
    {;}
    break;

  case 458:
#line 2033 "c-parse.y"
    { compstmt_count++;
                      yyval.ttype = c_begin_compound_stmt (); ;}
    break;

  case 459:
#line 2038 "c-parse.y"
    { yyval.ttype = convert (void_type_node, integer_zero_node); ;}
    break;

  case 460:
#line 2040 "c-parse.y"
    { yyval.ttype = poplevel (kept_level_p (), 1, 0);
		  SCOPE_STMT_BLOCK (TREE_PURPOSE (yyvsp[0].ttype))
		    = SCOPE_STMT_BLOCK (TREE_VALUE (yyvsp[0].ttype))
		    = yyval.ttype; ;}
    break;

  case 463:
#line 2053 "c-parse.y"
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

  case 464:
#line 2071 "c-parse.y"
    { RECHAIN_STMTS (yyvsp[-1].ttype, COMPOUND_BODY (yyvsp[-1].ttype));
		  last_expr_type = NULL_TREE;
                  yyval.ttype = yyvsp[-1].ttype; ;}
    break;

  case 465:
#line 2079 "c-parse.y"
    { c_finish_then (); ;}
    break;

  case 467:
#line 2096 "c-parse.y"
    { yyval.ttype = c_begin_if_stmt (); ;}
    break;

  case 468:
#line 2098 "c-parse.y"
    { c_expand_start_cond (c_common_truthvalue_conversion (yyvsp[-1].ttype),
				       compstmt_count,yyvsp[-3].ttype);
		  yyval.itype = stmt_count;
		  if_stmt_file = yyvsp[-7].filename;
		  if_stmt_line = yyvsp[-6].lineno; ;}
    break;

  case 469:
#line 2110 "c-parse.y"
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

  case 470:
#line 2121 "c-parse.y"
    { yyval.ttype = yyvsp[-2].ttype;
		  RECHAIN_STMTS (yyval.ttype, DO_BODY (yyval.ttype)); ;}
    break;

  case 471:
#line 2129 "c-parse.y"
    { if (yychar == YYEMPTY)
		    yychar = YYLEX;
		  yyval.filename = input_filename; ;}
    break;

  case 472:
#line 2135 "c-parse.y"
    { if (yychar == YYEMPTY)
		    yychar = YYLEX;
		  yyval.lineno = lineno; ;}
    break;

  case 475:
#line 2148 "c-parse.y"
    { if (flag_isoc99)
		    RECHAIN_STMTS (yyvsp[-2].ttype, COMPOUND_BODY (yyvsp[-2].ttype)); ;}
    break;

  case 476:
#line 2154 "c-parse.y"
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

  case 477:
#line 2168 "c-parse.y"
    { if (yyvsp[0].ttype)
		    {
		      STMT_LINENO (yyvsp[0].ttype) = yyvsp[-1].lineno;
		    }
		;}
    break;

  case 478:
#line 2177 "c-parse.y"
    { c_expand_start_else ();
		  yyvsp[-1].itype = stmt_count; ;}
    break;

  case 479:
#line 2180 "c-parse.y"
    { c_finish_else ();
		  c_expand_end_cond ();
		  if (extra_warnings && stmt_count == yyvsp[-3].itype)
		    warning ("empty body in an else-statement"); ;}
    break;

  case 480:
#line 2185 "c-parse.y"
    { c_expand_end_cond ();
		  /* This warning is here instead of in simple_if, because we
		     do not want a warning if an empty if is followed by an
		     else statement.  Increment stmt_count so we don't
		     give a second error if this is a nested `if'.  */
		  if (extra_warnings && stmt_count++ == yyvsp[0].itype)
		    warning_with_file_and_line (if_stmt_file, if_stmt_line,
						"empty body in an if-statement"); ;}
    break;

  case 481:
#line 2197 "c-parse.y"
    { c_expand_end_cond (); ;}
    break;

  case 482:
#line 2207 "c-parse.y"
    { stmt_count++;
		  yyval.ttype = c_begin_while_stmt (); ;}
    break;

  case 483:
#line 2210 "c-parse.y"
    { yyvsp[-1].ttype = c_common_truthvalue_conversion (yyvsp[-1].ttype);
		  c_finish_while_stmt_cond
		    (c_common_truthvalue_conversion (yyvsp[-1].ttype), yyvsp[-3].ttype);
		  yyval.ttype = add_stmt (yyvsp[-3].ttype); ;}
    break;

  case 484:
#line 2215 "c-parse.y"
    { RECHAIN_STMTS (yyvsp[-1].ttype, WHILE_BODY (yyvsp[-1].ttype)); ;}
    break;

  case 485:
#line 2218 "c-parse.y"
    { DO_COND (yyvsp[-4].ttype) = c_common_truthvalue_conversion (yyvsp[-2].ttype); ;}
    break;

  case 486:
#line 2220 "c-parse.y"
    { ;}
    break;

  case 487:
#line 2222 "c-parse.y"
    { yyval.ttype = build_stmt (FOR_STMT, NULL_TREE, NULL_TREE,
					  NULL_TREE, NULL_TREE);
		  add_stmt (yyval.ttype); ;}
    break;

  case 488:
#line 2226 "c-parse.y"
    { stmt_count++;
		  RECHAIN_STMTS (yyvsp[-2].ttype, FOR_INIT_STMT (yyvsp[-2].ttype)); ;}
    break;

  case 489:
#line 2229 "c-parse.y"
    { if (yyvsp[-1].ttype)
		    FOR_COND (yyvsp[-5].ttype)
		      = c_common_truthvalue_conversion (yyvsp[-1].ttype); ;}
    break;

  case 490:
#line 2233 "c-parse.y"
    { FOR_EXPR (yyvsp[-8].ttype) = yyvsp[-1].ttype; ;}
    break;

  case 491:
#line 2235 "c-parse.y"
    { RECHAIN_STMTS (yyvsp[-10].ttype, FOR_BODY (yyvsp[-10].ttype)); ;}
    break;

  case 492:
#line 2237 "c-parse.y"
    { stmt_count++;
		  yyval.ttype = c_start_case (yyvsp[-1].ttype); ;}
    break;

  case 493:
#line 2240 "c-parse.y"
    { c_finish_case (); ;}
    break;

  case 494:
#line 2245 "c-parse.y"
    { add_stmt (build_stmt (EXPR_STMT, yyvsp[-1].ttype)); ;}
    break;

  case 495:
#line 2247 "c-parse.y"
    { check_for_loop_decls (); ;}
    break;

  case 496:
#line 2253 "c-parse.y"
    { stmt_count++; yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 497:
#line 2255 "c-parse.y"
    { stmt_count++;
		  yyval.ttype = c_expand_expr_stmt (yyvsp[-1].ttype); ;}
    break;

  case 498:
#line 2258 "c-parse.y"
    { if (flag_isoc99)
		    RECHAIN_STMTS (yyvsp[-2].ttype, COMPOUND_BODY (yyvsp[-2].ttype));
		  yyval.ttype = NULL_TREE; ;}
    break;

  case 499:
#line 2262 "c-parse.y"
    { stmt_count++;
		  yyval.ttype = add_stmt (build_break_stmt ()); ;}
    break;

  case 500:
#line 2265 "c-parse.y"
    { stmt_count++;
		  yyval.ttype = add_stmt (build_continue_stmt ()); ;}
    break;

  case 501:
#line 2268 "c-parse.y"
    { stmt_count++;
		  yyval.ttype = c_expand_return (NULL_TREE); ;}
    break;

  case 502:
#line 2271 "c-parse.y"
    { stmt_count++;
		  yyval.ttype = c_expand_return (yyvsp[-1].ttype); ;}
    break;

  case 503:
#line 2274 "c-parse.y"
    { stmt_count++;
		  yyval.ttype = simple_asm_stmt (yyvsp[-2].ttype); ;}
    break;

  case 504:
#line 2278 "c-parse.y"
    { stmt_count++;
		  yyval.ttype = build_asm_stmt (yyvsp[-6].ttype, yyvsp[-4].ttype, yyvsp[-2].ttype, NULL_TREE, NULL_TREE); ;}
    break;

  case 505:
#line 2283 "c-parse.y"
    { stmt_count++;
		  yyval.ttype = build_asm_stmt (yyvsp[-8].ttype, yyvsp[-6].ttype, yyvsp[-4].ttype, yyvsp[-2].ttype, NULL_TREE); ;}
    break;

  case 506:
#line 2288 "c-parse.y"
    { stmt_count++;
		  yyval.ttype = build_asm_stmt (yyvsp[-10].ttype, yyvsp[-8].ttype, yyvsp[-6].ttype, yyvsp[-4].ttype, yyvsp[-2].ttype); ;}
    break;

  case 507:
#line 2291 "c-parse.y"
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

  case 508:
#line 2303 "c-parse.y"
    { if (pedantic)
		    pedwarn ("ISO C forbids `goto *expr;'");
		  stmt_count++;
		  yyvsp[-1].ttype = convert (ptr_type_node, yyvsp[-1].ttype);
		  yyval.ttype = add_stmt (build_stmt (GOTO_STMT, yyvsp[-1].ttype)); ;}
    break;

  case 509:
#line 2309 "c-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 510:
#line 2317 "c-parse.y"
    { stmt_count++;
		  yyval.ttype = do_case (yyvsp[-1].ttype, NULL_TREE); ;}
    break;

  case 511:
#line 2320 "c-parse.y"
    { stmt_count++;
		  yyval.ttype = do_case (yyvsp[-3].ttype, yyvsp[-1].ttype); ;}
    break;

  case 512:
#line 2323 "c-parse.y"
    { stmt_count++;
		  yyval.ttype = do_case (NULL_TREE, NULL_TREE); ;}
    break;

  case 513:
#line 2326 "c-parse.y"
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

  case 514:
#line 2342 "c-parse.y"
    { emit_line_note (input_filename, lineno);
		  yyval.ttype = NULL_TREE; ;}
    break;

  case 515:
#line 2345 "c-parse.y"
    { emit_line_note (input_filename, lineno); ;}
    break;

  case 516:
#line 2350 "c-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 518:
#line 2357 "c-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 521:
#line 2364 "c-parse.y"
    { yyval.ttype = chainon (yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 522:
#line 2369 "c-parse.y"
    { yyval.ttype = build_tree_list (build_tree_list (NULL_TREE, yyvsp[-3].ttype), yyvsp[-1].ttype); ;}
    break;

  case 523:
#line 2371 "c-parse.y"
    { yyvsp[-5].ttype = build_string (IDENTIFIER_LENGTH (yyvsp[-5].ttype),
				     IDENTIFIER_POINTER (yyvsp[-5].ttype));
		  yyval.ttype = build_tree_list (build_tree_list (yyvsp[-5].ttype, yyvsp[-3].ttype), yyvsp[-1].ttype); ;}
    break;

  case 524:
#line 2378 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 525:
#line 2380 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-2].ttype); ;}
    break;

  case 526:
#line 2390 "c-parse.y"
    { pushlevel (0);
		  clear_parm_order ();
		  declare_parm_level (0); ;}
    break;

  case 527:
#line 2394 "c-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  parmlist_tags_warning ();
		  poplevel (0, 0, 0); ;}
    break;

  case 529:
#line 2402 "c-parse.y"
    { tree parm;
		  if (pedantic)
		    pedwarn ("ISO C forbids forward parameter declarations");
		  /* Mark the forward decls as such.  */
		  for (parm = getdecls (); parm; parm = TREE_CHAIN (parm))
		    TREE_ASM_WRITTEN (parm) = 1;
		  clear_parm_order (); ;}
    break;

  case 530:
#line 2410 "c-parse.y"
    { /* Dummy action so attributes are in known place
		     on parser stack.  */ ;}
    break;

  case 531:
#line 2413 "c-parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 532:
#line 2415 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, NULL_TREE, NULL_TREE); ;}
    break;

  case 533:
#line 2421 "c-parse.y"
    { yyval.ttype = get_parm_info (0); ;}
    break;

  case 534:
#line 2423 "c-parse.y"
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

  case 535:
#line 2433 "c-parse.y"
    { yyval.ttype = get_parm_info (1);
		  parsing_iso_function_signature = true;
		;}
    break;

  case 536:
#line 2437 "c-parse.y"
    { yyval.ttype = get_parm_info (0); ;}
    break;

  case 537:
#line 2442 "c-parse.y"
    { push_parm_decl (yyvsp[0].ttype); ;}
    break;

  case 538:
#line 2444 "c-parse.y"
    { push_parm_decl (yyvsp[0].ttype); ;}
    break;

  case 539:
#line 2451 "c-parse.y"
    { yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 yyvsp[-1].ttype),
					chainon (yyvsp[0].ttype, all_prefix_attributes));
		  POP_DECLSPEC_STACK; ;}
    break;

  case 540:
#line 2456 "c-parse.y"
    { yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 yyvsp[-1].ttype),
					chainon (yyvsp[0].ttype, all_prefix_attributes));
		  POP_DECLSPEC_STACK; ;}
    break;

  case 541:
#line 2461 "c-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  POP_DECLSPEC_STACK; ;}
    break;

  case 542:
#line 2464 "c-parse.y"
    { yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 yyvsp[-1].ttype),
					chainon (yyvsp[0].ttype, all_prefix_attributes));
		  POP_DECLSPEC_STACK; ;}
    break;

  case 543:
#line 2470 "c-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  POP_DECLSPEC_STACK; ;}
    break;

  case 544:
#line 2478 "c-parse.y"
    { yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 yyvsp[-1].ttype),
					chainon (yyvsp[0].ttype, all_prefix_attributes));
		  POP_DECLSPEC_STACK; ;}
    break;

  case 545:
#line 2483 "c-parse.y"
    { yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 yyvsp[-1].ttype),
					chainon (yyvsp[0].ttype, all_prefix_attributes));
		  POP_DECLSPEC_STACK; ;}
    break;

  case 546:
#line 2488 "c-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  POP_DECLSPEC_STACK; ;}
    break;

  case 547:
#line 2491 "c-parse.y"
    { yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 yyvsp[-1].ttype),
					chainon (yyvsp[0].ttype, all_prefix_attributes));
		  POP_DECLSPEC_STACK; ;}
    break;

  case 548:
#line 2497 "c-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  POP_DECLSPEC_STACK; ;}
    break;

  case 549:
#line 2503 "c-parse.y"
    { prefix_attributes = chainon (prefix_attributes, yyvsp[-3].ttype);
		  all_prefix_attributes = prefix_attributes; ;}
    break;

  case 550:
#line 2512 "c-parse.y"
    { pushlevel (0);
		  clear_parm_order ();
		  declare_parm_level (1); ;}
    break;

  case 551:
#line 2516 "c-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  parmlist_tags_warning ();
		  poplevel (0, 0, 0); ;}
    break;

  case 553:
#line 2524 "c-parse.y"
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

  case 554:
#line 2542 "c-parse.y"
    { yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ttype); ;}
    break;

  case 555:
#line 2544 "c-parse.y"
    { yyval.ttype = chainon (yyvsp[-2].ttype, build_tree_list (NULL_TREE, yyvsp[0].ttype)); ;}
    break;

  case 556:
#line 2550 "c-parse.y"
    { yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ttype); ;}
    break;

  case 557:
#line 2552 "c-parse.y"
    { yyval.ttype = chainon (yyvsp[-2].ttype, build_tree_list (NULL_TREE, yyvsp[0].ttype)); ;}
    break;

  case 558:
#line 2557 "c-parse.y"
    { yyval.ttype = SAVE_EXT_FLAGS();
		  pedantic = 0;
		  warn_pointer_arith = 0;
		  warn_traditional = 0;
		  flag_iso = 0; ;}
    break;


    }

/* Line 991 of yacc.c.  */
#line 5255 "c-p15339.c"

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


#line 2564 "c-parse.y"


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


  if (C_IS_RESERVED_WORD (yylval.ttype))
    {
      enum rid rid_code = C_RID_CODE (yylval.ttype);

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

      static int last_lineno = 0;
      static const char *last_input_filename = 0;
      if (warn_traditional && !in_system_header
	  && (lineno != last_lineno || !last_input_filename ||
	      strcmp (last_input_filename, input_filename)))
	{
	  warning ("traditional C rejects string concatenation");
	  last_lineno = lineno;
	  last_input_filename = input_filename;
	}

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


