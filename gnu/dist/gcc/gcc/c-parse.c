/* A Bison parser, made from c-parse.y
   by GNU bison 1.35.  */

#define YYBISON 1  /* Identify Bison output.  */

# define	IDENTIFIER	257
# define	TYPENAME	258
# define	SCSPEC	259
# define	STATIC	260
# define	TYPESPEC	261
# define	TYPE_QUAL	262
# define	CONSTANT	263
# define	STRING	264
# define	ELLIPSIS	265
# define	SIZEOF	266
# define	ENUM	267
# define	STRUCT	268
# define	UNION	269
# define	IF	270
# define	ELSE	271
# define	WHILE	272
# define	DO	273
# define	FOR	274
# define	SWITCH	275
# define	CASE	276
# define	DEFAULT	277
# define	BREAK	278
# define	CONTINUE	279
# define	RETURN	280
# define	GOTO	281
# define	ASM_KEYWORD	282
# define	TYPEOF	283
# define	ALIGNOF	284
# define	ATTRIBUTE	285
# define	EXTENSION	286
# define	LABEL	287
# define	REALPART	288
# define	IMAGPART	289
# define	VA_ARG	290
# define	CHOOSE_EXPR	291
# define	TYPES_COMPATIBLE_P	292
# define	PTR_VALUE	293
# define	PTR_BASE	294
# define	PTR_EXTENT	295
# define	STRING_FUNC_NAME	296
# define	VAR_FUNC_NAME	297
# define	ASSIGN	298
# define	OROR	299
# define	ANDAND	300
# define	EQCOMPARE	301
# define	ARITHCOMPARE	302
# define	LSHIFT	303
# define	RSHIFT	304
# define	UNARY	305
# define	PLUSPLUS	306
# define	MINUSMINUS	307
# define	HYPERUNARY	308
# define	POINTSAT	309
# define	INTERFACE	310
# define	IMPLEMENTATION	311
# define	END	312
# define	SELECTOR	313
# define	DEFS	314
# define	ENCODE	315
# define	CLASSNAME	316
# define	PUBLIC	317
# define	PRIVATE	318
# define	PROTECTED	319
# define	PROTOCOL	320
# define	OBJECTNAME	321
# define	CLASS	322
# define	ALIAS	323

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

#line 103 "c-parse.y"
#ifndef YYSTYPE
typedef union {long itype; tree ttype; enum tree_code code;
	const char *filename; int lineno; } yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
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

#ifndef YYDEBUG
# define YYDEBUG 0
#endif



#define	YYFINAL		896
#define	YYFLAG		-32768
#define	YYNTBASE	92

/* YYTRANSLATE(YYLEX) -- Bison token number corresponding to YYLEX. */
#define YYTRANSLATE(x) ((unsigned)(x) <= 323 ? yytranslate[x] : 293)

/* YYTRANSLATE[YYLEX] -- Bison token number corresponding to YYLEX. */
static const char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    88,     2,     2,     2,    61,    52,     2,
      68,    84,    59,    57,    89,    58,    67,    60,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    47,    85,
       2,    45,     2,    46,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    69,     2,    91,    51,     2,     2,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     1,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    48,
      49,    53,    54,    55,    56,    62,    63,    64,    65,    66,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83
};

#if YYDEBUG
static const short yyprhs[] =
{
       0,     0,     1,     3,     4,     7,     8,    12,    14,    16,
      18,    24,    27,    31,    36,    41,    44,    47,    50,    52,
      53,    54,    64,    69,    70,    71,    81,    86,    87,    88,
      97,   101,   103,   105,   107,   109,   111,   113,   115,   117,
     119,   121,   122,   124,   126,   130,   132,   135,   138,   141,
     144,   147,   152,   155,   160,   163,   166,   168,   170,   172,
     174,   179,   181,   185,   189,   193,   197,   201,   205,   209,
     213,   217,   221,   225,   229,   230,   235,   236,   241,   242,
     243,   251,   252,   258,   262,   266,   268,   270,   272,   274,
     275,   283,   287,   291,   295,   299,   304,   311,   320,   327,
     332,   336,   340,   343,   346,   348,   349,   351,   355,   357,
     359,   362,   365,   370,   375,   378,   381,   385,   386,   388,
     393,   398,   402,   406,   409,   412,   414,   417,   420,   423,
     426,   429,   431,   434,   436,   439,   442,   445,   448,   451,
     454,   456,   459,   462,   465,   468,   471,   474,   477,   480,
     483,   486,   489,   492,   495,   498,   501,   504,   506,   509,
     512,   515,   518,   521,   524,   527,   530,   533,   536,   539,
     542,   545,   548,   551,   554,   557,   560,   563,   566,   569,
     572,   575,   578,   581,   584,   587,   590,   593,   596,   599,
     602,   605,   608,   611,   614,   617,   620,   623,   626,   629,
     632,   635,   638,   640,   642,   644,   646,   648,   650,   652,
     654,   656,   658,   660,   662,   664,   666,   668,   670,   672,
     674,   676,   678,   680,   682,   684,   686,   688,   690,   692,
     694,   696,   698,   700,   702,   704,   706,   708,   710,   712,
     714,   716,   718,   720,   722,   724,   726,   728,   730,   732,
     734,   736,   738,   740,   742,   744,   746,   748,   750,   751,
     753,   755,   757,   759,   761,   763,   765,   767,   772,   777,
     779,   784,   786,   791,   792,   797,   798,   805,   809,   810,
     817,   821,   822,   824,   826,   829,   836,   838,   842,   843,
     845,   850,   857,   862,   864,   866,   868,   870,   872,   874,
     876,   877,   882,   884,   885,   888,   890,   894,   898,   901,
     902,   907,   909,   910,   915,   917,   919,   921,   924,   927,
     933,   937,   938,   939,   947,   948,   949,   957,   959,   961,
     966,   970,   973,   977,   979,   981,   983,   987,   990,   992,
     996,   999,  1003,  1007,  1012,  1016,  1021,  1025,  1028,  1030,
    1032,  1035,  1037,  1040,  1042,  1045,  1046,  1054,  1060,  1061,
    1069,  1075,  1076,  1085,  1086,  1094,  1097,  1100,  1103,  1104,
    1106,  1107,  1109,  1111,  1114,  1115,  1119,  1122,  1126,  1131,
    1135,  1137,  1139,  1142,  1144,  1149,  1151,  1156,  1161,  1168,
    1174,  1179,  1186,  1192,  1194,  1198,  1200,  1202,  1206,  1207,
    1211,  1212,  1214,  1215,  1217,  1220,  1222,  1224,  1226,  1230,
    1233,  1237,  1242,  1246,  1249,  1252,  1254,  1259,  1263,  1268,
    1274,  1280,  1282,  1284,  1286,  1288,  1290,  1293,  1296,  1299,
    1302,  1304,  1307,  1310,  1313,  1315,  1318,  1321,  1324,  1327,
    1329,  1332,  1334,  1336,  1338,  1340,  1343,  1344,  1345,  1346,
    1347,  1348,  1350,  1352,  1355,  1359,  1361,  1364,  1366,  1368,
    1374,  1376,  1378,  1381,  1384,  1387,  1390,  1391,  1397,  1398,
    1403,  1404,  1405,  1407,  1410,  1414,  1418,  1422,  1423,  1428,
    1430,  1434,  1435,  1436,  1444,  1450,  1453,  1454,  1455,  1456,
    1457,  1470,  1471,  1478,  1481,  1483,  1485,  1488,  1492,  1495,
    1498,  1501,  1505,  1512,  1521,  1532,  1545,  1549,  1554,  1556,
    1560,  1566,  1569,  1575,  1576,  1578,  1579,  1581,  1582,  1584,
    1586,  1590,  1595,  1603,  1605,  1609,  1610,  1614,  1617,  1618,
    1619,  1626,  1629,  1630,  1632,  1634,  1638,  1640,  1644,  1649,
    1654,  1658,  1663,  1667,  1672,  1677,  1681,  1686,  1690,  1692,
    1693,  1697,  1699,  1702,  1704,  1708,  1710,  1714
};
static const short yyrhs[] =
{
      -1,    93,     0,     0,    94,    96,     0,     0,    93,    95,
      96,     0,    97,     0,    99,     0,    98,     0,    28,    68,
     108,    84,    85,     0,   292,    96,     0,   130,   164,    85,
       0,   150,   130,   164,    85,     0,   149,   130,   163,    85,
       0,   156,    85,     0,     1,    85,     0,     1,    86,     0,
      85,     0,     0,     0,   149,   130,   193,   100,   124,   101,
     253,   254,   242,     0,   149,   130,   193,     1,     0,     0,
       0,   150,   130,   198,   102,   124,   103,   253,   254,   242,
       0,   150,   130,   198,     1,     0,     0,     0,   130,   198,
     104,   124,   105,   253,   254,   242,     0,   130,   198,     1,
       0,     3,     0,     4,     0,    52,     0,    58,     0,    57,
       0,    63,     0,    64,     0,    87,     0,    88,     0,   110,
       0,     0,   110,     0,   116,     0,   110,    89,   116,     0,
     122,     0,    59,   115,     0,   292,   115,     0,   107,   115,
       0,    49,   106,     0,   112,   111,     0,   112,    68,   219,
      84,     0,   113,   111,     0,   113,    68,   219,    84,     0,
      34,   115,     0,    35,   115,     0,    12,     0,    30,     0,
      29,     0,   111,     0,    68,   219,    84,   115,     0,   115,
       0,   116,    57,   116,     0,   116,    58,   116,     0,   116,
      59,   116,     0,   116,    60,   116,     0,   116,    61,   116,
       0,   116,    55,   116,     0,   116,    56,   116,     0,   116,
      54,   116,     0,   116,    53,   116,     0,   116,    52,   116,
       0,   116,    50,   116,     0,   116,    51,   116,     0,     0,
     116,    49,   117,   116,     0,     0,   116,    48,   118,   116,
       0,     0,     0,   116,    46,   119,   108,    47,   120,   116,
       0,     0,   116,    46,   121,    47,   116,     0,   116,    45,
     116,     0,   116,    44,   116,     0,     3,     0,     9,     0,
      10,     0,    43,     0,     0,    68,   219,    84,    90,   123,
     179,    86,     0,    68,   108,    84,     0,    68,     1,    84,
       0,   246,   244,    84,     0,   246,     1,    84,     0,   122,
      68,   109,    84,     0,    36,    68,   116,    89,   219,    84,
       0,    37,    68,   116,    89,   116,    89,   116,    84,     0,
      38,    68,   219,    89,   219,    84,     0,   122,    69,   108,
      91,     0,   122,    67,   106,     0,   122,    66,   106,     0,
     122,    63,     0,   122,    64,     0,   125,     0,     0,   127,
       0,   253,   254,   128,     0,   126,     0,   234,     0,   127,
     126,     0,   126,   234,     0,   151,   130,   163,    85,     0,
     152,   130,   164,    85,     0,   151,    85,     0,   152,    85,
       0,   253,   254,   132,     0,     0,   170,     0,   149,   130,
     163,    85,     0,   150,   130,   164,    85,     0,   149,   130,
     187,     0,   150,   130,   190,     0,   156,    85,     0,   292,
     132,     0,     8,     0,   133,     8,     0,   134,     8,     0,
     133,   171,     0,   135,     8,     0,   136,     8,     0,   171,
       0,   135,   171,     0,   158,     0,   137,     8,     0,   138,
       8,     0,   137,   160,     0,   138,   160,     0,   133,   158,
       0,   134,   158,     0,   159,     0,   137,   171,     0,   137,
     161,     0,   138,   161,     0,   133,   159,     0,   134,   159,
       0,   139,     8,     0,   140,     8,     0,   139,   160,     0,
     140,   160,     0,   135,   158,     0,   136,   158,     0,   139,
     171,     0,   139,   161,     0,   140,   161,     0,   135,   159,
       0,   136,   159,     0,   176,     0,   141,     8,     0,   142,
       8,     0,   133,   176,     0,   134,   176,     0,   141,   176,
       0,   142,   176,     0,   141,   171,     0,   143,     8,     0,
     144,     8,     0,   135,   176,     0,   136,   176,     0,   143,
     176,     0,   144,   176,     0,   143,   171,     0,   145,     8,
       0,   146,     8,     0,   145,   160,     0,   146,   160,     0,
     141,   158,     0,   142,   158,     0,   137,   176,     0,   138,
     176,     0,   145,   176,     0,   146,   176,     0,   145,   171,
       0,   145,   161,     0,   146,   161,     0,   141,   159,     0,
     142,   159,     0,   147,     8,     0,   148,     8,     0,   147,
     160,     0,   148,   160,     0,   143,   158,     0,   144,   158,
       0,   139,   176,     0,   140,   176,     0,   147,   176,     0,
     148,   176,     0,   147,   171,     0,   147,   161,     0,   148,
     161,     0,   143,   159,     0,   144,   159,     0,   137,     0,
     138,     0,   139,     0,   140,     0,   145,     0,   146,     0,
     147,     0,   148,     0,   133,     0,   134,     0,   135,     0,
     136,     0,   141,     0,   142,     0,   143,     0,   144,     0,
     137,     0,   138,     0,   145,     0,   146,     0,   133,     0,
     134,     0,   141,     0,   142,     0,   137,     0,   138,     0,
     139,     0,   140,     0,   133,     0,   134,     0,   135,     0,
     136,     0,   137,     0,   138,     0,   139,     0,   140,     0,
     133,     0,   134,     0,   135,     0,   136,     0,   133,     0,
     134,     0,   135,     0,   136,     0,   137,     0,   138,     0,
     139,     0,   140,     0,   141,     0,   142,     0,   143,     0,
     144,     0,   145,     0,   146,     0,   147,     0,   148,     0,
       0,   154,     0,   160,     0,   162,     0,   161,     0,     7,
       0,   207,     0,   202,     0,     4,     0,   114,    68,   108,
      84,     0,   114,    68,   219,    84,     0,   166,     0,   163,
      89,   131,   166,     0,   168,     0,   164,    89,   131,   168,
       0,     0,    28,    68,    10,    84,     0,     0,   193,   165,
     170,    45,   167,   177,     0,   193,   165,   170,     0,     0,
     198,   165,   170,    45,   169,   177,     0,   198,   165,   170,
       0,     0,   171,     0,   172,     0,   171,   172,     0,    31,
      68,    68,   173,    84,    84,     0,   174,     0,   173,    89,
     174,     0,     0,   175,     0,   175,    68,     3,    84,     0,
     175,    68,     3,    89,   110,    84,     0,   175,    68,   109,
      84,     0,   106,     0,   176,     0,     7,     0,     8,     0,
       6,     0,     5,     0,   116,     0,     0,    90,   178,   179,
      86,     0,     1,     0,     0,   180,   208,     0,   181,     0,
     180,    89,   181,     0,   185,    45,   183,     0,   186,   183,
       0,     0,   106,    47,   182,   183,     0,   183,     0,     0,
      90,   184,   179,    86,     0,   116,     0,     1,     0,   186,
       0,   185,   186,     0,    67,   106,     0,    69,   116,    11,
     116,    91,     0,    69,   116,    91,     0,     0,     0,   193,
     188,   124,   189,   253,   254,   247,     0,     0,     0,   198,
     191,   124,   192,   253,   254,   247,     0,   194,     0,   198,
       0,    68,   170,   194,    84,     0,   194,    68,   287,     0,
     194,   227,     0,    59,   157,   194,     0,     4,     0,   196,
       0,   197,     0,   196,    68,   287,     0,   196,   227,     0,
       4,     0,   197,    68,   287,     0,   197,   227,     0,    59,
     157,   196,     0,    59,   157,   197,     0,    68,   170,   197,
      84,     0,   198,    68,   287,     0,    68,   170,   198,    84,
       0,    59,   157,   198,     0,   198,   227,     0,     3,     0,
      14,     0,    14,   171,     0,    15,     0,    15,   171,     0,
      13,     0,    13,   171,     0,     0,   199,   106,    90,   203,
     210,    86,   170,     0,   199,    90,   210,    86,   170,     0,
       0,   200,   106,    90,   204,   210,    86,   170,     0,   200,
      90,   210,    86,   170,     0,     0,   201,   106,    90,   205,
     217,   209,    86,   170,     0,     0,   201,    90,   206,   217,
     209,    86,   170,     0,   199,   106,     0,   200,   106,     0,
     201,   106,     0,     0,    89,     0,     0,    89,     0,   211,
       0,   211,   212,     0,     0,   211,   212,    85,     0,   211,
      85,     0,   153,   130,   213,     0,   153,   130,   253,   254,
       0,   154,   130,   214,     0,   154,     0,     1,     0,   292,
     212,     0,   215,     0,   213,    89,   131,   215,     0,   216,
       0,   214,    89,   131,   216,     0,   253,   254,   193,   170,
       0,   253,   254,   193,    47,   116,   170,     0,   253,   254,
      47,   116,   170,     0,   253,   254,   198,   170,     0,   253,
     254,   198,    47,   116,   170,     0,   253,   254,    47,   116,
     170,     0,   218,     0,   217,    89,   218,     0,     1,     0,
     106,     0,   106,    45,   116,     0,     0,   155,   220,   221,
       0,     0,   223,     0,     0,   223,     0,   224,   171,     0,
     225,     0,   224,     0,   226,     0,    59,   157,   224,     0,
      59,   157,     0,    59,   157,   225,     0,    68,   170,   223,
      84,     0,   226,    68,   277,     0,   226,   227,     0,    68,
     277,     0,   227,     0,    69,   157,   108,    91,     0,    69,
     157,    91,     0,    69,   157,    59,    91,     0,    69,     6,
     157,   108,    91,     0,    69,   154,     6,   108,    91,     0,
     229,     0,   230,     0,   231,     0,   232,     0,   257,     0,
     229,   257,     0,   230,   257,     0,   231,   257,     0,   232,
     257,     0,   129,     0,   229,   129,     0,   230,   129,     0,
     232,   129,     0,   258,     0,   229,   258,     0,   230,   258,
       0,   231,   258,     0,   232,   258,     0,   234,     0,   233,
     234,     0,   229,     0,   230,     0,   231,     0,   232,     0,
       1,    85,     0,     0,     0,     0,     0,     0,   240,     0,
     241,     0,   240,   241,     0,    33,   291,    85,     0,   247,
       0,     1,   247,     0,    90,     0,    86,     0,   235,   239,
     245,    86,   236,     0,   228,     0,     1,     0,    68,    90,
       0,   243,   244,     0,   249,   256,     0,   249,     1,     0,
       0,    16,   250,    68,   108,    84,     0,     0,    19,   252,
     256,    18,     0,     0,     0,   257,     0,   258,   255,     0,
     237,   255,   238,     0,   253,   254,   269,     0,   253,   254,
     270,     0,     0,   248,    17,   260,   256,     0,   248,     0,
     248,    17,     1,     0,     0,     0,    18,   261,    68,   108,
      84,   262,   256,     0,   251,    68,   108,    84,    85,     0,
     251,     1,     0,     0,     0,     0,     0,    20,   263,    68,
     268,   264,   272,    85,   265,   272,    84,   266,   256,     0,
       0,    21,    68,   108,    84,   267,   256,     0,   272,    85,
       0,   132,     0,   247,     0,   108,    85,     0,   237,   259,
     238,     0,    24,    85,     0,    25,    85,     0,    26,    85,
       0,    26,   108,    85,     0,    28,   271,    68,   108,    84,
      85,     0,    28,   271,    68,   108,    47,   273,    84,    85,
       0,    28,   271,    68,   108,    47,   273,    47,   273,    84,
      85,     0,    28,   271,    68,   108,    47,   273,    47,   273,
      47,   276,    84,    85,     0,    27,   106,    85,     0,    27,
      59,   108,    85,     0,    85,     0,    22,   116,    47,     0,
      22,   116,    11,   116,    47,     0,    23,    47,     0,   106,
     253,   254,    47,   170,     0,     0,     8,     0,     0,   108,
       0,     0,   274,     0,   275,     0,   274,    89,   275,     0,
      10,    68,   108,    84,     0,    69,   106,    91,    10,    68,
     108,    84,     0,    10,     0,   276,    89,    10,     0,     0,
     170,   278,   279,     0,   282,    84,     0,     0,     0,   283,
      85,   280,   170,   281,   279,     0,     1,    84,     0,     0,
      11,     0,   283,     0,   283,    89,    11,     0,   285,     0,
     283,    89,   284,     0,   149,   130,   195,   170,     0,   149,
     130,   198,   170,     0,   149,   130,   222,     0,   150,   130,
     198,   170,     0,   150,   130,   222,     0,   151,   286,   195,
     170,     0,   151,   286,   198,   170,     0,   151,   286,   222,
       0,   152,   286,   198,   170,     0,   152,   286,   222,     0,
     130,     0,     0,   170,   288,   289,     0,   279,     0,   290,
      84,     0,     3,     0,   290,    89,     3,     0,   106,     0,
     291,    89,   106,     0,    32,     0
};

#endif

#if YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined. */
static const short yyrline[] =
{
       0,   323,   328,   346,   346,   348,   348,   351,   356,   358,
     359,   367,   371,   379,   381,   383,   385,   386,   387,   392,
     392,   392,   405,   407,   407,   407,   419,   421,   421,   421,
     433,   437,   439,   442,   444,   446,   451,   453,   455,   457,
     461,   465,   468,   471,   474,   478,   480,   483,   486,   490,
     492,   498,   501,   504,   507,   509,   513,   517,   521,   525,
     527,   531,   533,   535,   537,   539,   541,   543,   545,   547,
     549,   551,   553,   555,   557,   557,   564,   564,   571,   571,
     571,   581,   581,   592,   599,   610,   617,   618,   620,   622,
     622,   635,   640,   642,   658,   665,   667,   670,   680,   690,
     692,   696,   702,   704,   709,   716,   724,   730,   735,   737,
     738,   739,   746,   749,   751,   754,   762,   771,   781,   786,
     789,   791,   793,   795,   797,   853,   857,   860,   865,   871,
     875,   880,   884,   889,   893,   896,   899,   902,   905,   908,
     913,   917,   920,   923,   926,   929,   934,   938,   941,   944,
     947,   950,   955,   959,   962,   965,   968,   973,   977,   980,
     983,   989,   995,  1001,  1009,  1015,  1019,  1022,  1028,  1034,
    1040,  1048,  1054,  1058,  1061,  1064,  1067,  1070,  1073,  1079,
    1085,  1091,  1099,  1103,  1106,  1109,  1112,  1117,  1121,  1124,
    1127,  1130,  1133,  1136,  1142,  1148,  1154,  1162,  1166,  1169,
    1172,  1175,  1181,  1183,  1184,  1185,  1186,  1187,  1188,  1189,
    1192,  1194,  1195,  1196,  1197,  1198,  1199,  1200,  1203,  1205,
    1206,  1207,  1210,  1212,  1213,  1214,  1217,  1219,  1220,  1221,
    1224,  1226,  1227,  1228,  1231,  1233,  1234,  1235,  1236,  1237,
    1238,  1239,  1242,  1244,  1245,  1246,  1247,  1248,  1249,  1250,
    1251,  1252,  1253,  1254,  1255,  1256,  1257,  1258,  1262,  1265,
    1290,  1292,  1295,  1299,  1302,  1305,  1309,  1314,  1316,  1322,
    1324,  1327,  1329,  1332,  1335,  1339,  1339,  1348,  1355,  1355,
    1364,  1371,  1374,  1378,  1381,  1385,  1390,  1393,  1397,  1400,
    1402,  1404,  1406,  1413,  1415,  1416,  1417,  1420,  1422,  1427,
    1429,  1429,  1433,  1438,  1442,  1445,  1447,  1452,  1456,  1459,
    1459,  1465,  1468,  1468,  1473,  1475,  1478,  1480,  1483,  1486,
    1490,  1494,  1494,  1494,  1525,  1525,  1525,  1559,  1561,  1566,
    1569,  1574,  1576,  1578,  1585,  1587,  1590,  1596,  1598,  1601,
    1607,  1609,  1611,  1613,  1620,  1626,  1628,  1630,  1632,  1635,
    1638,  1642,  1645,  1649,  1652,  1662,  1662,  1669,  1673,  1673,
    1677,  1681,  1681,  1686,  1686,  1693,  1696,  1698,  1706,  1708,
    1711,  1713,  1718,  1721,  1726,  1728,  1730,  1735,  1739,  1749,
    1752,  1757,  1759,  1764,  1766,  1770,  1772,  1776,  1780,  1784,
    1789,  1793,  1797,  1807,  1809,  1814,  1819,  1822,  1826,  1826,
    1834,  1837,  1840,  1845,  1849,  1855,  1857,  1860,  1862,  1866,
    1869,  1873,  1876,  1878,  1880,  1882,  1888,  1891,  1893,  1895,
    1898,  1908,  1910,  1911,  1915,  1918,  1920,  1921,  1922,  1923,
    1926,  1928,  1931,  1932,  1935,  1937,  1938,  1939,  1940,  1943,
    1945,  1948,  1950,  1951,  1952,  1955,  1958,  1965,  1970,  1986,
    2001,  2003,  2008,  2010,  2013,  2027,  2030,  2033,  2037,  2039,
    2046,  2048,  2051,  2069,  2076,  2082,  2085,  2085,  2107,  2107,
    2127,  2133,  2139,  2141,  2145,  2151,  2165,  2174,  2174,  2183,
    2195,  2205,  2205,  2205,  2215,  2218,  2220,  2220,  2220,  2220,
    2220,  2235,  2235,  2242,  2245,  2250,  2253,  2256,  2260,  2263,
    2266,  2269,  2272,  2276,  2280,  2285,  2289,  2301,  2307,  2315,
    2318,  2321,  2324,  2339,  2343,  2347,  2350,  2355,  2357,  2360,
    2362,  2366,  2369,  2375,  2378,  2387,  2387,  2398,  2400,  2400,
    2400,  2413,  2419,  2421,  2431,  2435,  2439,  2442,  2448,  2454,
    2459,  2462,  2468,  2475,  2481,  2486,  2489,  2495,  2500,  2509,
    2509,  2520,  2522,  2539,  2542,  2547,  2550,  2554
};
#endif


#if (YYDEBUG) || defined YYERROR_VERBOSE

/* YYTNAME[TOKEN_NUM] -- String name of the token TOKEN_NUM. */
static const char *const yytname[] =
{
  "$", "error", "$undefined.", "IDENTIFIER", "TYPENAME", "SCSPEC", "STATIC", 
  "TYPESPEC", "TYPE_QUAL", "CONSTANT", "STRING", "ELLIPSIS", "SIZEOF", 
  "ENUM", "STRUCT", "UNION", "IF", "ELSE", "WHILE", "DO", "FOR", "SWITCH", 
  "CASE", "DEFAULT", "BREAK", "CONTINUE", "RETURN", "GOTO", "ASM_KEYWORD", 
  "TYPEOF", "ALIGNOF", "ATTRIBUTE", "EXTENSION", "LABEL", "REALPART", 
  "IMAGPART", "VA_ARG", "CHOOSE_EXPR", "TYPES_COMPATIBLE_P", "PTR_VALUE", 
  "PTR_BASE", "PTR_EXTENT", "STRING_FUNC_NAME", "VAR_FUNC_NAME", "ASSIGN", 
  "'='", "'?'", "':'", "OROR", "ANDAND", "'|'", "'^'", "'&'", "EQCOMPARE", 
  "ARITHCOMPARE", "LSHIFT", "RSHIFT", "'+'", "'-'", "'*'", "'/'", "'%'", 
  "UNARY", "PLUSPLUS", "MINUSMINUS", "HYPERUNARY", "POINTSAT", "'.'", 
  "'('", "'['", "INTERFACE", "IMPLEMENTATION", "END", "SELECTOR", "DEFS", 
  "ENCODE", "CLASSNAME", "PUBLIC", "PRIVATE", "PROTECTED", "PROTOCOL", 
  "OBJECTNAME", "CLASS", "ALIAS", "')'", "';'", "'}'", "'~'", "'!'", 
  "','", "'{'", "']'", "program", "extdefs", "@1", "@2", "extdef", 
  "extdef_1", "datadef", "fndef", "@3", "@4", "@5", "@6", "@7", "@8", 
  "identifier", "unop", "expr", "exprlist", "nonnull_exprlist", 
  "unary_expr", "sizeof", "alignof", "typeof", "cast_expr", 
  "expr_no_commas", "@9", "@10", "@11", "@12", "@13", "primary", "@14", 
  "old_style_parm_decls", "old_style_parm_decls_1", "lineno_datadecl", 
  "datadecls", "datadecl", "lineno_decl", "setspecs", "maybe_resetattrs", 
  "decl", "declspecs_nosc_nots_nosa_noea", "declspecs_nosc_nots_nosa_ea", 
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

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives. */
static const short yyr1[] =
{
       0,    92,    92,    94,    93,    95,    93,    96,    97,    97,
      97,    97,    98,    98,    98,    98,    98,    98,    98,   100,
     101,    99,    99,   102,   103,    99,    99,   104,   105,    99,
      99,   106,   106,   107,   107,   107,   107,   107,   107,   107,
     108,   109,   109,   110,   110,   111,   111,   111,   111,   111,
     111,   111,   111,   111,   111,   111,   112,   113,   114,   115,
     115,   116,   116,   116,   116,   116,   116,   116,   116,   116,
     116,   116,   116,   116,   117,   116,   118,   116,   119,   120,
     116,   121,   116,   116,   116,   122,   122,   122,   122,   123,
     122,   122,   122,   122,   122,   122,   122,   122,   122,   122,
     122,   122,   122,   122,   124,   125,   125,   126,   127,   127,
     127,   127,   128,   128,   128,   128,   129,   130,   131,   132,
     132,   132,   132,   132,   132,   133,   133,   133,   134,   135,
     135,   136,   136,   137,   137,   137,   137,   137,   137,   137,
     138,   138,   138,   138,   138,   138,   139,   139,   139,   139,
     139,   139,   140,   140,   140,   140,   140,   141,   141,   141,
     141,   141,   141,   141,   142,   143,   143,   143,   143,   143,
     143,   144,   145,   145,   145,   145,   145,   145,   145,   145,
     145,   145,   146,   146,   146,   146,   146,   147,   147,   147,
     147,   147,   147,   147,   147,   147,   147,   148,   148,   148,
     148,   148,   149,   149,   149,   149,   149,   149,   149,   149,
     150,   150,   150,   150,   150,   150,   150,   150,   151,   151,
     151,   151,   152,   152,   152,   152,   153,   153,   153,   153,
     154,   154,   154,   154,   155,   155,   155,   155,   155,   155,
     155,   155,   156,   156,   156,   156,   156,   156,   156,   156,
     156,   156,   156,   156,   156,   156,   156,   156,   157,   157,
     158,   158,   159,   160,   160,   161,   162,   162,   162,   163,
     163,   164,   164,   165,   165,   167,   166,   166,   169,   168,
     168,   170,   170,   171,   171,   172,   173,   173,   174,   174,
     174,   174,   174,   175,   175,   175,   175,   176,   176,   177,
     178,   177,   177,   179,   179,   180,   180,   181,   181,   182,
     181,   181,   184,   183,   183,   183,   185,   185,   186,   186,
     186,   188,   189,   187,   191,   192,   190,   193,   193,   194,
     194,   194,   194,   194,   195,   195,   196,   196,   196,   197,
     197,   197,   197,   197,   198,   198,   198,   198,   198,   199,
     199,   200,   200,   201,   201,   203,   202,   202,   204,   202,
     202,   205,   202,   206,   202,   207,   207,   207,   208,   208,
     209,   209,   210,   210,   211,   211,   211,   212,   212,   212,
     212,   212,   212,   213,   213,   214,   214,   215,   215,   215,
     216,   216,   216,   217,   217,   217,   218,   218,   220,   219,
     221,   221,   222,   222,   222,   223,   223,   224,   224,   225,
     225,   226,   226,   226,   226,   226,   227,   227,   227,   227,
     227,   228,   228,   228,   228,   229,   229,   229,   229,   229,
     230,   230,   230,   230,   231,   231,   231,   231,   231,   232,
     232,   233,   233,   233,   233,   234,   235,   236,   237,   238,
     239,   239,   240,   240,   241,   242,   242,   243,   244,   244,
     245,   245,   246,   247,   248,   248,   250,   249,   252,   251,
     253,   254,   255,   255,   256,   257,   258,   260,   259,   259,
     259,   261,   262,   259,   259,   259,   263,   264,   265,   266,
     259,   267,   259,   268,   268,   269,   269,   269,   269,   269,
     269,   269,   269,   269,   269,   269,   269,   269,   269,   270,
     270,   270,   270,   271,   271,   272,   272,   273,   273,   274,
     274,   275,   275,   276,   276,   278,   277,   279,   280,   281,
     279,   279,   282,   282,   282,   282,   283,   283,   284,   284,
     284,   284,   284,   285,   285,   285,   285,   285,   286,   288,
     287,   289,   289,   290,   290,   291,   291,   292
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN. */
static const short yyr2[] =
{
       0,     0,     1,     0,     2,     0,     3,     1,     1,     1,
       5,     2,     3,     4,     4,     2,     2,     2,     1,     0,
       0,     9,     4,     0,     0,     9,     4,     0,     0,     8,
       3,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     0,     1,     1,     3,     1,     2,     2,     2,     2,
       2,     4,     2,     4,     2,     2,     1,     1,     1,     1,
       4,     1,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     0,     4,     0,     4,     0,     0,
       7,     0,     5,     3,     3,     1,     1,     1,     1,     0,
       7,     3,     3,     3,     3,     4,     6,     8,     6,     4,
       3,     3,     2,     2,     1,     0,     1,     3,     1,     1,
       2,     2,     4,     4,     2,     2,     3,     0,     1,     4,
       4,     3,     3,     2,     2,     1,     2,     2,     2,     2,
       2,     1,     2,     1,     2,     2,     2,     2,     2,     2,
       1,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     1,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     0,     1,
       1,     1,     1,     1,     1,     1,     1,     4,     4,     1,
       4,     1,     4,     0,     4,     0,     6,     3,     0,     6,
       3,     0,     1,     1,     2,     6,     1,     3,     0,     1,
       4,     6,     4,     1,     1,     1,     1,     1,     1,     1,
       0,     4,     1,     0,     2,     1,     3,     3,     2,     0,
       4,     1,     0,     4,     1,     1,     1,     2,     2,     5,
       3,     0,     0,     7,     0,     0,     7,     1,     1,     4,
       3,     2,     3,     1,     1,     1,     3,     2,     1,     3,
       2,     3,     3,     4,     3,     4,     3,     2,     1,     1,
       2,     1,     2,     1,     2,     0,     7,     5,     0,     7,
       5,     0,     8,     0,     7,     2,     2,     2,     0,     1,
       0,     1,     1,     2,     0,     3,     2,     3,     4,     3,
       1,     1,     2,     1,     4,     1,     4,     4,     6,     5,
       4,     6,     5,     1,     3,     1,     1,     3,     0,     3,
       0,     1,     0,     1,     2,     1,     1,     1,     3,     2,
       3,     4,     3,     2,     2,     1,     4,     3,     4,     5,
       5,     1,     1,     1,     1,     1,     2,     2,     2,     2,
       1,     2,     2,     2,     1,     2,     2,     2,     2,     1,
       2,     1,     1,     1,     1,     2,     0,     0,     0,     0,
       0,     1,     1,     2,     3,     1,     2,     1,     1,     5,
       1,     1,     2,     2,     2,     2,     0,     5,     0,     4,
       0,     0,     1,     2,     3,     3,     3,     0,     4,     1,
       3,     0,     0,     7,     5,     2,     0,     0,     0,     0,
      12,     0,     6,     2,     1,     1,     2,     3,     2,     2,
       2,     3,     6,     8,    10,    12,     3,     4,     1,     3,
       5,     2,     5,     0,     1,     0,     1,     0,     1,     1,
       3,     4,     7,     1,     3,     0,     3,     2,     0,     0,
       6,     2,     0,     1,     1,     3,     1,     3,     4,     4,
       3,     4,     3,     4,     4,     3,     4,     3,     1,     0,
       3,     1,     2,     1,     3,     1,     3,     1
};

/* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
   doesn't specify something else to do.  Zero means the default is an
   error. */
static const short yydefact[] =
{
       3,     5,     0,     0,     0,   266,   298,   297,   263,   125,
     353,   349,   351,     0,    58,     0,   557,    18,     4,     7,
       9,     8,     0,     0,   210,   211,   212,   213,   202,   203,
     204,   205,   214,   215,   216,   217,   206,   207,   208,   209,
     117,   117,     0,   133,   140,   260,   262,   261,   131,   283,
     157,     0,     0,     0,   265,   264,     0,     6,    16,    17,
     354,   350,   352,     0,     0,     0,   348,   258,   281,     0,
     271,     0,   126,   138,   144,   128,   160,   127,   139,   145,
     161,   129,   150,   155,   132,   167,   130,   151,   156,   168,
     134,   136,   142,   141,   178,   135,   137,   143,   179,   146,
     148,   153,   152,   193,   147,   149,   154,   194,   158,   176,
     185,   164,   162,   159,   177,   186,   163,   165,   191,   200,
     171,   169,   166,   192,   201,   170,   172,   174,   183,   182,
     180,   173,   175,   184,   181,   187,   189,   198,   197,   195,
     188,   190,   199,   196,     0,     0,    15,   284,    31,    32,
     374,   365,   374,   366,   363,   367,    11,    85,    86,    87,
      56,    57,     0,     0,     0,     0,     0,    88,     0,    33,
      35,    34,     0,    36,    37,     0,    38,    39,     0,     0,
      40,    59,     0,     0,    61,    43,    45,     0,     0,   288,
       0,   238,   239,   240,   241,   234,   235,   236,   237,   398,
       0,   230,   231,   232,   233,   259,     0,     0,   282,    12,
     281,    30,     0,   281,   258,     0,   281,   347,   333,   258,
     281,     0,   269,     0,   327,   328,     0,     0,     0,     0,
     355,     0,   358,     0,   361,    54,    55,     0,     0,     0,
      49,    46,     0,   462,     0,     0,    48,     0,     0,     0,
      50,     0,    52,     0,     0,    78,    76,    74,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     102,   103,     0,     0,    41,     0,     0,   458,   450,     0,
      47,   295,   296,   293,     0,   286,   289,   294,   267,   400,
     268,   346,     0,     0,   118,     0,   549,   344,   258,   259,
       0,     0,    28,   104,     0,   470,   109,   471,   280,     0,
       0,    14,   281,    22,     0,   281,   281,   331,    13,    26,
       0,   281,   381,   376,   230,   231,   232,   233,   226,   227,
     228,   229,   117,   117,   373,     0,   374,   281,   374,   395,
     396,   370,   393,     0,     0,     0,     0,    92,    91,     0,
      10,    44,     0,     0,    84,    83,     0,     0,     0,     0,
      72,    73,    71,    70,    69,    67,    68,    62,    63,    64,
      65,    66,   101,   100,     0,    42,     0,    94,     0,     0,
     451,   452,    93,     0,   288,    41,   258,   281,   399,   401,
     406,   405,   407,   415,   345,   272,   273,     0,     0,     0,
       0,     0,   417,     0,   445,   470,   111,   110,     0,   278,
     332,     0,     0,    20,   277,   330,    24,   357,   470,   470,
     375,   382,     0,   360,     0,     0,   371,     0,   370,     0,
       0,     0,    89,    60,    51,    53,     0,     0,    77,    75,
      95,    99,   555,     0,   461,   430,   460,   470,   470,   470,
     470,     0,   439,     0,   471,   425,   434,   453,   285,   287,
      85,     0,   409,   525,   414,   281,   413,   274,     0,   553,
     533,   222,   223,   218,   219,   224,   225,   220,   221,   117,
     117,   551,     0,   534,   536,   550,     0,     0,     0,   418,
     416,   471,   107,   117,   117,     0,   329,   270,   273,   470,
     275,   470,   377,   383,   471,   379,   385,   471,   281,   281,
     397,   394,   281,     0,     0,     0,     0,     0,    79,    82,
     454,     0,   431,   426,   435,   432,   427,   436,   471,   428,
     437,   433,   429,   438,   440,   447,   448,   290,     0,   292,
     408,   410,     0,     0,   525,   412,   531,   548,   402,   402,
     527,   528,     0,   552,     0,   419,   420,     0,   114,     0,
     115,     0,   302,   300,   299,   279,   471,     0,   471,   281,
     378,   281,     0,   356,   359,   364,   281,    96,     0,    98,
     315,    85,     0,     0,   312,     0,   314,     0,   368,   305,
     311,     0,     0,     0,   556,   448,   459,   266,     0,     0,
       0,     0,     0,     0,   513,   508,   457,   470,     0,   116,
     117,   117,     0,     0,   446,   495,   475,   476,     0,     0,
     411,   526,   338,   258,   281,   281,   334,   335,   281,   545,
     403,   406,   258,   281,   281,   547,   281,   535,   210,   211,
     212,   213,   202,   203,   204,   205,   214,   215,   216,   217,
     206,   207,   208,   209,   117,   117,   537,   554,     0,    29,
     455,     0,     0,     0,     0,   276,     0,   470,     0,   281,
     470,     0,   281,   362,     0,   318,     0,     0,   309,    90,
       0,   304,     0,   317,   308,    80,     0,   511,   498,   499,
     500,     0,     0,     0,   514,     0,   471,   496,     0,     0,
     123,   466,   481,   468,   486,     0,   479,     0,     0,   449,
     463,   124,   291,   409,   525,   543,   281,   337,   281,   340,
     544,   404,   409,   525,   546,   529,   402,   402,   456,   112,
     113,     0,    21,    25,   384,   471,   281,     0,   387,   386,
     281,     0,   390,    97,     0,   320,     0,     0,   306,   307,
       0,   509,   501,     0,   506,     0,     0,     0,   121,   321,
       0,   122,   324,     0,     0,   448,     0,     0,     0,   465,
     470,   464,   485,     0,   497,   341,   342,     0,   336,   339,
       0,   281,   281,   540,   281,   542,   301,     0,   389,   281,
     392,   281,     0,   313,   310,     0,   507,     0,   281,   119,
       0,   120,     0,     0,     0,     0,   515,     0,   480,   448,
     449,   472,   470,     0,   343,   530,   538,   539,   541,   388,
     391,   319,   510,   517,     0,   512,   322,   325,     0,     0,
     469,   516,   494,   487,     0,   491,   478,   474,   473,     0,
       0,     0,     0,   518,   519,   502,   470,   470,   467,   482,
     515,   493,   448,   484,     0,     0,   517,     0,     0,   471,
     471,   448,     0,   492,     0,     0,     0,   503,   520,     0,
       0,   483,   488,   521,     0,     0,     0,   323,   326,   515,
       0,   523,     0,   504,     0,     0,     0,     0,   489,   522,
     505,   524,   448,   490,     0,     0,     0
};

static const short yydefgoto[] =
{
     894,     1,     2,     3,    18,    19,    20,    21,   314,   499,
     320,   501,   215,   405,   585,   178,   244,   374,   180,   181,
     182,   183,    22,   184,   185,   359,   358,   356,   593,   357,
     186,   517,   302,   303,   304,   305,   492,   445,    23,   293,
     609,   191,   192,   193,   194,   195,   196,   197,   198,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,   479,
     480,   332,   205,   199,    42,   206,    43,    44,    45,    46,
      47,   221,    69,   216,   222,   567,    70,   495,   294,   208,
      49,   284,   285,   286,    50,   565,   663,   587,   588,   589,
     747,   590,   677,   591,   592,   758,   800,   846,   761,   802,
     847,   498,   224,   625,   626,   627,   225,    51,    52,    53,
      54,   336,   338,   343,   233,    55,   681,   427,   228,   229,
     334,   502,   505,   503,   506,   341,   342,   200,   289,   388,
     629,   630,   390,   391,   392,   217,   446,   447,   448,   449,
     450,   451,   306,   278,   596,   770,   774,   379,   380,   381,
     659,   614,   279,   453,   187,   660,   706,   707,   763,   708,
     765,   307,   408,   810,   771,   811,   812,   709,   809,   764,
     861,   766,   850,   879,   892,   852,   833,   616,   617,   695,
     834,   842,   843,   844,   882,   464,   543,   481,   636,   780,
     482,   483,   656,   484,   548,   297,   398,   485,   486,   443,
     188
};

static const short yypact[] =
{
      92,   118,  2854,  2854,   195,-32768,-32768,-32768,-32768,-32768,
     100,   100,   100,    72,-32768,   104,-32768,-32768,-32768,-32768,
  -32768,-32768,   156,   143,   982,  1751,  1012,  1839,   510,   304,
    1266,  1162,  1262,  2106,  1556,  2343,  1293,  1755,  1448,  1843,
  -32768,-32768,    64,-32768,-32768,-32768,-32768,-32768,   100,-32768,
  -32768,   119,   130,   141,-32768,-32768,  2854,-32768,-32768,-32768,
     100,   100,   100,  2644,   162,  2562,-32768,   144,   100,    25,
  -32768,   880,-32768,-32768,-32768,   100,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,   100,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,   100,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,   100,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,   100,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
     100,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,   100,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,   100,-32768,
  -32768,-32768,-32768,-32768,   319,   143,-32768,-32768,-32768,-32768,
  -32768,    80,-32768,   155,-32768,   164,-32768,-32768,-32768,-32768,
  -32768,-32768,  2644,  2644,   204,   246,   257,-32768,   557,-32768,
  -32768,-32768,  2644,-32768,-32768,  1353,-32768,-32768,  2644,   254,
     259,-32768,  2706,  2747,-32768,  3142,   467,  1588,  2644,  1148,
     267,  1501,  1114,  1635,  2044,  1081,   446,  1126,   675,-32768,
     289,   220,   376,   282,   398,-32768,   143,   143,   100,-32768,
     100,-32768,   341,   100,   226,   491,   100,-32768,-32768,   144,
     100,   128,-32768,  1041,   497,   545,   297,   946,   326,  1178,
  -32768,   345,-32768,   499,-32768,-32768,-32768,  2644,  2644,  2419,
  -32768,-32768,   357,-32768,   360,   367,-32768,   391,  2644,  1353,
  -32768,  1353,-32768,  2644,  2644,   411,-32768,-32768,  2644,  2644,
    2644,  2644,  2644,  2644,  2644,  2644,  2644,  2644,  2644,  2644,
  -32768,-32768,   557,   557,  2644,  2644,   417,-32768,   476,   427,
  -32768,-32768,-32768,-32768,   324,-32768,   451,-32768,-32768,   380,
  -32768,   545,   292,   143,-32768,   511,-32768,-32768,   144,   521,
    2192,   444,-32768,-32768,   767,    86,-32768,-32768,   493,   319,
     319,-32768,   100,-32768,   491,   100,   100,-32768,-32768,-32768,
     491,   100,-32768,-32768,  1501,  1114,  1635,  2044,  1081,   446,
    1126,   675,-32768,   533,   463,  1393,-32768,   100,-32768,-32768,
     506,   474,-32768,   499,  2932,  2958,   480,-32768,-32768,  2434,
  -32768,  3142,   488,   490,  3142,  3142,  2644,   536,  2644,  2644,
    2668,  2871,  2525,   924,  1145,   547,   547,   531,   531,-32768,
  -32768,-32768,-32768,-32768,   509,   259,   508,-32768,   557,  1686,
     476,-32768,-32768,   513,  1148,  2788,   144,   100,-32768,-32768,
  -32768,-32768,   564,-32768,-32768,-32768,   303,   527,  1210,  2644,
    2644,  2233,-32768,   529,-32768,-32768,-32768,-32768,   723,-32768,
     497,   296,   319,-32768,   583,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,   544,-32768,   548,  2644,   557,   554,   474,  2419,
    2644,  2419,-32768,-32768,   553,   553,   598,  2644,  3171,  2155,
  -32768,-32768,-32768,   358,   444,-32768,-32768,    65,    73,    95,
      96,   646,-32768,   563,-32768,-32768,-32768,-32768,-32768,-32768,
     336,   567,   380,   380,-32768,   100,-32768,-32768,   571,-32768,
  -32768,  1536,  2170,  1101,  1045,  2904,  2512,  1521,  1814,-32768,
  -32768,-32768,   575,   393,-32768,-32768,   337,   570,   572,-32768,
  -32768,-32768,-32768,   585,   590,  2066,-32768,-32768,   649,-32768,
  -32768,-32768,   596,-32768,-32768,   602,-32768,-32768,   100,   100,
    3142,-32768,   100,   593,   603,  2978,   618,  1928,-32768,  3158,
  -32768,   557,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,  2303,-32768,  2644,-32768,
  -32768,-32768,   619,   387,-32768,-32768,-32768,-32768,   415,   147,
  -32768,-32768,   539,-32768,   729,-32768,-32768,    90,-32768,   319,
  -32768,   143,-32768,-32768,  3142,-32768,-32768,  2066,-32768,   100,
     236,   100,   298,-32768,-32768,-32768,   100,-32768,  2644,-32768,
  -32768,   688,   557,  2644,-32768,   701,  3142,   664,   662,-32768,
  -32768,   239,  1435,  2644,-32768,  2372,-32768,   708,  2644,   709,
     672,   673,  2603,   133,   752,-32768,-32768,-32768,   676,-32768,
  -32768,-32768,   679,   726,   683,-32768,-32768,-32768,  2500,   373,
  -32768,-32768,-32768,   144,   100,   100,   589,   604,   328,-32768,
  -32768,   100,   144,   100,   328,-32768,   100,-32768,  1536,  2170,
    3095,  3126,  1101,  1045,  1902,  2144,  2904,  2512,  3107,  3138,
    1521,  1814,  1968,  2474,-32768,-32768,-32768,-32768,   680,-32768,
  -32768,   400,   405,  1928,    90,-32768,    90,-32768,  2644,   247,
  -32768,  2644,   153,-32768,  3000,-32768,  2843,  1928,-32768,-32768,
    1997,-32768,  2130,-32768,-32768,  3158,  2896,-32768,-32768,-32768,
  -32768,   691,  2644,   694,-32768,   697,-32768,-32768,   319,   143,
  -32768,-32768,-32768,-32768,-32768,   715,   768,  1774,   125,-32768,
  -32768,-32768,-32768,   415,   200,-32768,   100,-32768,   100,-32768,
  -32768,   100,   147,   147,-32768,-32768,   415,   147,-32768,-32768,
  -32768,   698,-32768,-32768,-32768,-32768,  3037,  2644,-32768,-32768,
    3037,  2644,-32768,-32768,  2644,-32768,   702,  2130,-32768,-32768,
    2644,-32768,-32768,   705,-32768,  2644,   745,   423,-32768,   379,
     437,-32768,   258,   727,   730,-32768,   732,  2644,  1862,-32768,
  -32768,-32768,-32768,  2644,-32768,   589,   604,   354,-32768,-32768,
     387,   100,   328,-32768,   328,-32768,-32768,   236,-32768,  3037,
  -32768,  3037,  2914,-32768,-32768,  3124,-32768,    77,   100,-32768,
     491,-32768,   491,  2644,  2644,   776,  2500,   713,-32768,-32768,
  -32768,-32768,-32768,   717,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,    58,   719,-32768,-32768,-32768,   721,   725,
  -32768,-32768,-32768,-32768,   728,-32768,-32768,-32768,-32768,   733,
     743,   557,    81,   731,-32768,-32768,-32768,-32768,-32768,-32768,
    2644,-32768,-32768,-32768,  2644,   724,    58,   736,    58,-32768,
  -32768,-32768,   738,-32768,   742,   804,   126,-32768,-32768,   680,
     680,-32768,-32768,-32768,   760,   820,   746,-32768,-32768,  2644,
    2644,-32768,   381,-32768,   750,   755,   756,   830,-32768,-32768,
  -32768,-32768,-32768,-32768,   842,   847,-32768
};

static const short yypgoto[] =
{
  -32768,-32768,-32768,-32768,   113,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,   102,-32768,   -63,   464,  -256,   443,
  -32768,-32768,-32768,  -105,   990,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,  -309,-32768,   546,-32768,-32768,   129,   123,  -271,
    -557,    42,    44,    46,    48,     6,    19,    21,    23,  -381,
    -369,   301,   306,  -365,  -314,   317,   320,  -532,  -451,   447,
     465,-32768,  -121,-32768,  -501,  -160,   674,   811,   868,   903,
  -32768,  -494,  -132,  -222,   459,-32768,   581,-32768,   294,     4,
      28,-32768,   494,-32768,   687,   308,-32768,  -348,-32768,   197,
  -32768,  -522,-32768,-32768,   288,-32768,-32768,-32768,-32768,-32768,
  -32768,  -134,   384,   154,   169,   -18,    40,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,   455,   -72,-32768,
     555,-32768,-32768,   222,   221,   549,   475,  -144,-32768,-32768,
    -485,  -277,  -406,  -425,-32768,   397,-32768,-32768,-32768,-32768,
  -32768,-32768,  -232,-32768,-32768,  -437,    93,-32768,-32768,   520,
    -197,-32768,   300,-32768,-32768,  -517,-32768,-32768,-32768,-32768,
  -32768,   414,  -372,    98,  -684,  -259,   -80,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
    -652,    56,-32768,    59,-32768,   456,-32768,  -488,-32768,-32768,
  -32768,-32768,-32768,-32768,   422,  -313,-32768,-32768,-32768,-32768,
      50
};


#define	YYLAST		3232


static const short yytable[] =
{
     179,   315,   190,   415,   610,   413,    48,    48,    28,    28,
     223,   416,   389,   226,    60,    61,    62,   475,   375,   615,
     654,    29,    29,    30,    30,    31,    31,   475,    75,   476,
      84,   245,    93,   477,   102,   612,   111,   541,   120,   476,
     129,   412,   138,   477,    24,    24,    25,    25,    26,    26,
      27,    27,    56,    56,   300,   621,   540,   235,   236,   309,
      48,   711,    28,    71,   635,   661,  -441,   241,   840,    48,
     684,    48,   406,   246,  -442,    29,   147,    30,   615,    31,
     231,   805,   536,   280,   478,   611,   610,  -106,   147,   147,
     147,   658,    -1,   299,   478,   346,  -443,  -444,    24,   613,
      25,   655,    26,   147,    27,   352,    56,   353,   333,   201,
     209,   202,   147,   203,   210,   204,    57,   612,    -2,   557,
     455,   147,   148,   149,   823,   836,   772,   841,   856,   375,
     147,    15,   570,   148,   149,   572,   148,   149,   399,   147,
      63,   728,   631,   631,   148,   149,    66,   452,   147,   146,
      66,  -421,     9,   151,   153,   155,   595,   147,   613,  -422,
     749,   824,   475,   144,   145,   857,   147,   611,   863,   156,
     230,   646,    64,   875,   476,    15,  -106,   871,   477,    48,
     606,  -423,  -424,   647,    15,   227,   542,   650,   523,   526,
     529,   532,   692,   773,   664,    75,   666,    84,   862,    93,
     741,   102,    67,    66,   757,    75,   632,    84,   893,   150,
     876,    68,   376,   311,   333,   633,   214,   312,    48,   534,
     152,   213,   214,    48,    65,   794,   462,   884,    72,   478,
     189,   154,   298,    48,     9,   328,   147,   403,   651,    66,
     218,   783,   785,    48,   433,   232,   291,   292,   329,   832,
     330,    15,   331,    48,   234,    48,   201,    15,   202,   623,
     203,   201,   204,   202,   422,   203,   424,   204,   624,   214,
     240,   324,   237,   325,   610,   326,   315,   327,    15,   335,
      58,    59,   619,   668,   682,   514,   212,   516,   541,  -273,
      81,   283,   815,   436,   737,   219,   241,   541,   667,   456,
     670,    66,    48,  -273,   220,   612,   582,   540,   583,     6,
       7,     8,    95,    15,   238,   731,   540,    10,    11,    12,
     631,   631,    66,   218,   756,   239,   213,   214,    75,   746,
      84,   212,    93,   396,   102,   340,   487,   488,   247,    48,
     201,   328,   202,  -273,   203,   671,   204,  -273,   248,   291,
     292,   288,   877,   878,   329,   611,   330,    67,   331,    15,
     213,   214,   207,   787,   316,   214,    68,   524,   527,   530,
     533,   213,   214,   290,   372,   373,   394,   324,   219,   325,
     496,   326,   318,   327,    77,   335,   210,   220,   468,  -247,
      48,     5,     6,     7,     8,     9,   213,   214,   470,   475,
      10,    11,    12,   778,   473,   779,    86,   212,   383,   295,
    -273,   476,   321,   384,   473,   477,    14,   474,    66,   622,
     537,   553,   718,   214,  -273,   538,   554,   474,   201,   662,
     202,   337,   203,    48,   204,    48,   669,   542,   814,   386,
     471,   347,   472,   520,   348,   340,   542,   521,   387,   214,
     471,   349,   472,     8,    95,   418,   419,   712,   -81,    10,
      11,    12,   248,   713,  -273,   886,   478,   732,  -273,   733,
     887,  -532,   722,   608,   623,    75,   350,    93,   551,   111,
     442,   129,   552,   624,   214,   729,   283,   869,   870,   312,
     730,   826,   301,   827,   210,  -470,  -470,  -470,  -470,  -470,
     339,   377,   148,   149,  -470,  -470,  -470,   296,   799,   378,
     308,   382,   312,   280,   310,     6,     7,     8,    90,   385,
    -470,   397,   801,    10,    11,    12,   210,   400,   340,   404,
     270,   271,   608,   272,   273,   274,   275,   315,   409,   691,
      48,    15,    28,     5,     6,     7,     8,     9,   420,   473,
     637,   425,    10,    11,    12,    29,    48,    30,   642,    31,
     148,   149,   474,   426,   759,   316,   214,   760,    14,   431,
      15,   643,   434,   644,   435,   645,   522,   525,    24,   531,
      25,  -105,    26,   437,    27,   471,   618,   472,   628,   634,
     267,   268,   269,   440,   638,  -246,   639,   458,   640,   441,
     641,   396,   547,   547,   265,   266,   267,   268,   269,   414,
     296,   467,   672,   213,   214,   417,   559,   561,  -380,  -380,
     490,   317,    48,   594,    28,   250,   252,    48,   500,   753,
     508,   423,   465,   214,   509,   721,    48,    29,   607,    30,
     512,    31,    75,   432,    84,   518,    93,   301,   102,   535,
     111,   539,   120,   669,   129,   546,   138,   716,   214,   550,
      24,   555,    25,   556,    26,   201,    27,   202,   618,   203,
     558,   204,   718,   214,   201,   560,   202,   212,   203,   576,
     204,   463,     8,   104,   675,   569,   393,   577,    10,    11,
      12,   571,   797,   410,   411,   776,   777,   607,    73,    78,
      82,    87,   579,   620,   807,   693,   109,   114,   118,   123,
     813,    76,    80,    85,    89,    94,    98,   103,   107,   112,
     116,   121,   125,   130,   134,   139,   143,     5,     6,     7,
       8,     9,   657,   698,   699,   -31,    10,    11,    12,   762,
     828,   829,   701,   831,   702,   703,   704,   705,   678,   147,
     679,   680,    14,   291,   292,   -32,   687,   688,   689,   544,
     694,   697,   291,   292,   700,   755,   782,   784,   301,   277,
     606,  -108,  -108,  -108,  -108,  -108,   752,   726,   727,   754,
    -108,  -108,  -108,   767,   786,   768,   473,   831,   793,   466,
     796,   864,   798,   454,   830,   803,  -108,   835,   804,   474,
     806,   839,   573,   574,   845,   848,   575,   317,   317,   849,
      48,   854,    28,   851,   874,   865,   831,   885,   853,   491,
     858,   867,   471,   872,   472,    29,   873,    30,   880,    31,
     881,   883,   504,   507,   888,    74,    79,    83,    88,   889,
     891,   890,   895,   110,   115,   119,   124,   896,    24,   461,
      25,   407,    26,   648,    27,   493,   618,  -108,   649,   393,
     393,   454,   454,   528,   454,    73,    78,    82,    87,   652,
     673,   497,   653,   494,   395,   665,   287,   748,   459,   683,
     781,   211,   775,   513,   -27,   -27,   -27,   -27,   -27,   734,
     421,   739,   428,   -27,   -27,   -27,    91,    96,   100,   105,
     457,   511,   549,   837,   127,   132,   136,   141,   212,   -27,
     838,  -273,   866,   566,   710,   568,     0,   868,   714,   715,
       0,   545,   720,     0,     0,  -273,     0,   723,   724,     0,
     725,    92,    97,   101,   106,     0,     0,     0,     0,   128,
     133,   137,   142,   855,     0,   393,   393,   319,   213,   214,
     -23,   -23,   -23,   -23,   -23,     0,     0,     0,     0,   -23,
     -23,   -23,     0,   738,     0,  -273,   742,     0,     0,  -273,
     -27,     0,     0,     0,   212,   -23,     0,  -273,   262,   263,
     264,   265,   266,   267,   268,   269,     5,     6,     7,     8,
      72,  -273,     0,     0,     0,    10,    11,    12,    73,    78,
      82,    87,    74,    79,    83,    88,     0,     0,     0,     0,
     296,    14,   296,    15,   213,   214,     5,     6,     7,     8,
      81,   696,     0,   717,   719,    10,    11,    12,     0,     0,
     788,  -273,     0,     0,   790,  -273,   -23,     0,     0,     0,
       0,    14,   313,    15,     0,   -19,   -19,   -19,   -19,   -19,
       6,     7,     8,    95,   -19,   -19,   -19,     0,    10,    11,
      12,     0,     0,    91,    96,   100,   105,  -242,     0,   212,
     -19,   287,  -273,     0,     0,   816,   817,     0,   818,     0,
       0,   735,     0,   819,   507,   820,  -273,     0,     8,    90,
       0,     0,   825,     0,    10,    11,    12,  -244,    92,    97,
     101,   106,     0,     0,     0,     0,     6,     7,     8,    90,
     393,   393,    15,     0,    10,    11,    12,     0,     5,   393,
     393,     8,    77,   393,   393,     0,  -273,    10,    11,    12,
    -273,   -19,    15,     8,    99,    74,    79,    83,    88,    10,
      11,    12,     0,    14,     0,    73,    78,     0,     0,   109,
     114,   148,   149,     6,     7,   281,   282,    15,    76,    80,
      94,    98,   112,   116,   130,   134,     0,     6,     7,     8,
     104,     0,   717,   719,   719,    10,    11,    12,     0,   322,
       0,     0,     5,     0,   528,     8,     9,     0,     0,     0,
       0,    10,    11,    12,     0,     0,    91,    96,   100,   105,
     263,   264,   265,   266,   267,   268,   269,    14,     0,    15,
      16,   468,     0,   469,     5,     6,     7,     8,     9,     0,
       0,   470,     0,    10,    11,    12,   528,   344,   345,     0,
       0,    92,    97,   101,   106,     0,     0,     0,   351,    14,
       0,     0,     0,   354,   355,     0,     0,  -249,   360,   361,
     362,   363,   364,   365,   366,   367,   368,   369,   370,   371,
     859,   860,     0,   323,  -372,     0,     5,     6,     7,     8,
     108,     6,     7,     8,    99,    10,    11,    12,     0,    10,
      11,    12,    74,    79,     0,     0,   110,   115,     0,     0,
       0,    14,     0,    15,  -532,     0,     0,    15,     6,     7,
       8,   126,     0,     0,     0,     0,    10,    11,    12,     0,
       0,     0,    73,    78,    82,    87,     0,     0,     0,     0,
     109,   114,   118,   123,    15,    76,    80,    85,    89,    94,
      98,   103,   107,   112,   116,   121,   125,   130,   134,   139,
     143,    91,    96,     0,     0,   127,   132,  -250,   438,   439,
       0,  -248,     0,     0,   242,     0,   157,     5,     0,     0,
       8,     9,   158,   159,     0,   160,    10,    11,    12,     0,
       0,     0,     0,     0,     0,     0,    92,    97,  -254,     0,
     128,   133,    14,   161,    15,    16,     0,   162,   163,   164,
     165,   166,     0,     0,   322,     0,   167,     5,     0,     0,
       8,     9,   168,     0,     0,   169,    10,    11,    12,     0,
     170,   171,   172,     0,     0,   510,   173,   174,     0,     0,
     515,   175,    14,     0,    15,    16,     0,   519,     0,     0,
       0,     0,     0,     0,     0,     0,   580,     0,   157,     0,
     176,   177,     0,   243,   158,   159,     0,   160,     0,    74,
      79,    83,    88,     6,     7,     8,   135,   110,   115,   119,
     124,    10,    11,    12,     0,   161,     0,    16,     0,   162,
     163,   164,   165,   166,     0,     0,     0,     0,   167,    15,
    -316,     0,     0,     0,   168,   564,     0,   169,     0,     0,
       0,     0,   170,   171,   172,     0,     0,     0,   173,   174,
       0,     0,  -316,   175,  -316,     5,     0,   586,     8,    72,
      91,    96,   100,   105,    10,    11,    12,     0,   127,   132,
     136,   141,   176,   177,     0,   584,     6,     7,     8,   126,
      14,     0,    15,  -256,    10,    11,    12,     0,     0,     0,
       5,     6,     7,     8,    72,    92,    97,   101,   106,    10,
      11,    12,    15,   128,   133,   137,   142,   564,     0,     0,
       5,     6,     7,     8,   117,    14,     0,    15,   674,    10,
      11,    12,     0,   676,     0,     0,     0,     0,     0,     0,
       0,     0,   586,   685,     0,    14,     0,    15,   686,   276,
       0,  -446,  -446,  -446,  -446,  -446,  -446,  -446,  -446,     0,
    -446,  -446,  -446,  -446,  -446,     0,  -446,  -446,  -446,  -446,
    -446,  -446,  -446,  -446,  -446,  -446,  -446,  -446,  -446,  -446,
    -446,  -446,  -446,  -446,  -446,  -446,  -446,     0,     0,     0,
       0,  -446,     0,     0,     0,     0,     0,  -446,     0,     5,
    -446,  -252,     8,    81,     0,  -446,  -446,  -446,    10,    11,
      12,  -446,  -446,   586,     0,     0,  -446,     0,   736,     0,
       0,   740,     0,     0,    14,     0,    15,   586,     0,     0,
     586,     0,   586,  -446,   277,  -446,  -446,     0,  -446,     0,
       0,     0,     0,     0,     0,     0,     0,   444,     0,  -470,
    -470,  -470,  -470,  -470,  -470,  -470,  -470,     0,  -470,  -470,
    -470,  -470,  -470,     0,  -470,  -470,  -470,  -470,  -470,  -470,
    -470,  -470,  -470,  -470,  -470,  -470,  -470,  -470,  -470,     0,
    -470,  -470,  -470,  -470,  -470,     0,     0,   789,     0,  -470,
       0,   791,     0,     0,   792,  -470,     0,   586,  -470,     0,
     795,     0,     0,  -470,  -470,  -470,     0,     0,     0,  -470,
    -470,     0,     0,     0,  -470,     5,     6,     7,     8,    77,
       6,     7,     8,   131,    10,    11,    12,     0,    10,    11,
      12,  -470,     0,  -470,  -470,   769,  -470,  -448,  -448,     0,
      14,     0,     0,  -448,  -448,     0,  -448,     0,     0,     0,
    -448,     0,  -448,  -448,  -448,  -448,  -448,  -448,  -448,  -448,
    -448,  -448,  -448,     0,  -448,     0,  -448,     0,  -448,  -448,
    -448,  -448,  -448,     0,     0,     0,     0,  -448,     0,     6,
       7,     8,   131,  -448,     0,     0,  -448,    10,    11,    12,
       0,  -448,  -448,  -448,     0,     0,  -243,  -448,  -448,     0,
    -255,     0,  -448,     5,     6,     7,     8,    86,     6,     7,
       8,   140,    10,    11,    12,     0,    10,    11,    12,  -448,
       0,  -448,  -448,   808,  -448,  -477,  -477,     0,    14,     0,
       0,  -477,  -477,     0,  -477,     0,     0,     0,  -477,     0,
    -477,  -477,  -477,  -477,  -477,  -477,  -477,  -477,  -477,  -477,
    -477,     0,  -477,     0,  -477,     0,  -477,  -477,  -477,  -477,
    -477,     0,     0,     0,     0,  -477,     0,     6,     7,     8,
      99,  -477,     0,     0,  -477,    10,    11,    12,     0,  -477,
    -477,  -477,     0,     0,  -245,  -477,  -477,     0,  -257,   580,
    -477,   581,   149,    15,     0,     0,     0,   158,   159,     0,
     160,     0,     0,     0,     0,     0,     0,  -477,     0,  -477,
    -477,     0,  -477,     0,     0,     0,     0,     0,   161,     0,
      16,     0,   162,   163,   164,   165,   166,     0,     0,     0,
       0,   167,     0,     6,     7,     8,   135,   168,     0,     0,
     169,    10,    11,    12,     0,   170,   171,   172,     0,     0,
       0,   173,   174,     0,     0,   582,   175,   583,   580,    15,
     581,   149,     0,     0,     0,     0,   158,   159,     0,   160,
       0,     0,     0,     0,  -303,   176,   177,     0,   584,     0,
       0,     0,     0,     0,     0,     0,     0,   161,     0,    16,
       0,   162,   163,   164,   165,   166,     0,     0,     0,     0,
     167,     0,     0,     0,     0,     0,   168,     0,     5,   169,
       0,     8,    86,     0,   170,   171,   172,    10,    11,    12,
     173,   174,     0,     0,   582,   175,   583,   562,     0,   157,
       0,     0,     0,    14,     0,   158,   159,     0,   160,     0,
       0,     0,     0,  -369,   176,   177,     0,   584,     0,     0,
       0,     0,     0,     0,     0,     0,   161,     0,    16,     0,
     162,   163,   164,   165,   166,     0,     0,     0,     0,   167,
       5,     6,     7,     8,   113,   168,     0,     0,   169,    10,
      11,    12,     0,   170,   171,   172,     0,     0,     0,   173,
     174,   580,     0,   157,   175,    14,     0,     0,     0,   158,
     159,     0,   160,     0,     0,     0,     0,     0,     0,     6,
       7,     8,   104,   176,   177,     0,   563,    10,    11,    12,
     161,     0,    16,     0,   162,   163,   164,   165,   166,     0,
       0,     0,     0,   167,     5,     6,     7,     8,    77,   168,
       0,     0,   169,    10,    11,    12,     0,   170,   171,   172,
       0,  -251,     0,   173,   174,   157,     0,     0,   175,    14,
       0,   158,   159,     0,   160,   258,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   176,   177,     0,
     584,     0,   161,     0,    16,     0,   162,   163,   164,   165,
     166,     0,     0,     0,     0,   167,   157,     0,     0,     0,
       0,   168,   158,   159,   169,   160,     0,     0,     0,   170,
     171,   401,     0,     0,     0,   173,   174,     0,     0,     0,
     175,     0,     0,   161,     0,    16,     0,   162,   163,   164,
     165,   166,     0,     0,     0,     0,   167,     0,     0,   176,
     177,     0,   168,   402,     0,   169,     0,     0,     0,     0,
     170,   171,   172,     0,     0,     0,   173,   174,     0,     0,
       0,   175,     0,     0,     0,     0,   581,   597,     6,     7,
       8,     9,   158,   159,     0,   160,    10,    11,    12,     0,
     176,   177,     0,     0,   489,   598,   599,   600,   601,   602,
     603,   604,    14,   161,    15,    16,     0,   162,   163,   164,
     165,   166,     0,     0,     0,     0,   167,     5,     6,     7,
       8,   122,   168,     0,     0,   169,    10,    11,    12,     0,
     170,   171,   172,     0,     0,     0,   173,   174,     0,     0,
       0,   175,    14,     0,     0,   581,   149,     0,     0,     0,
       0,   158,   159,     0,   160,     0,     0,     0,   605,     0,
     176,   177,     0,   606,   598,   599,   600,   601,   602,   603,
     604,     0,   161,     0,    16,     0,   162,   163,   164,   165,
     166,     0,     0,     0,     0,   167,     0,     0,     0,     0,
       0,   168,     0,     5,   169,     0,     8,     9,  -253,   170,
     171,   172,    10,    11,    12,   173,   174,   157,     0,     0,
     175,     0,     0,   158,   159,     0,   160,     0,    14,     0,
      15,     0,     0,     0,     0,     0,     0,   605,     0,   176,
     177,     0,   606,     0,   161,     0,    16,     0,   162,   163,
     164,   165,   166,     0,     0,     0,     0,   167,     0,     6,
       7,     8,   140,   168,     0,     0,   169,    10,    11,    12,
       0,   170,   171,   172,     0,     0,     0,   173,   174,     0,
       0,     0,   175,   157,     5,     6,     7,     8,     9,   158,
     159,     0,   160,    10,    11,    12,     5,     6,     7,     8,
     113,   176,   177,     0,   432,    10,    11,    12,     0,    14,
     161,    15,    16,     0,   162,   163,   164,   165,   166,     0,
       0,    14,     0,   167,     0,     0,     0,     0,     0,   168,
       0,     0,   169,     0,     0,     0,     0,   170,   171,   172,
       0,     0,     0,   173,   174,   157,     5,     0,   175,     8,
       9,   158,   159,     0,   160,    10,    11,    12,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   176,   177,     0,
       0,    14,   161,    15,    16,     0,   162,   163,   164,   165,
     166,     0,     0,     0,     0,   167,   157,     0,     0,     0,
       0,   168,   158,   159,   169,   160,     0,     0,     0,   170,
     171,   172,     0,     0,     0,   173,   174,     0,     0,     0,
     175,     0,     0,   161,     0,    16,     0,   162,   163,   164,
     165,   166,     0,     0,     0,     0,   167,   157,     0,   176,
     177,     0,   168,   158,   159,   169,   160,     0,     0,     0,
     170,   171,   172,     0,     0,     0,   173,   174,     0,     0,
       0,   175,     0,     0,   161,     0,    16,     0,   162,   163,
     164,   165,   166,     0,     0,     0,     0,   167,   690,     0,
     176,   177,     0,   168,     0,     0,   169,     0,     0,     0,
       0,   170,   171,   172,     0,     0,     0,   173,   174,   157,
       0,     0,   175,     0,     0,   158,   159,     0,   160,   259,
     260,   261,   262,   263,   264,   265,   266,   267,   268,   269,
       0,   176,   177,     0,     0,     0,   161,     0,    16,     0,
     162,   163,   164,   165,   166,     0,     0,     0,     0,   167,
     157,     0,     0,     0,     0,   168,   158,   159,   169,   160,
       0,     0,     0,   170,   171,   172,     0,     0,     0,   173,
     174,     0,     0,     0,   249,     0,     0,   161,     0,    16,
       0,   162,   163,   164,   165,   166,     0,     0,     0,     0,
     167,   460,     0,   176,   177,     0,   168,   158,   159,   169,
     160,     0,     0,     0,   170,   171,   172,     0,     0,     0,
     173,   174,     0,     0,     0,   251,     0,     0,   161,     0,
      16,     0,   162,   163,   164,   165,   166,     0,     0,     0,
       0,   167,     0,     0,   176,   177,     0,   168,     0,     0,
     169,     0,     0,     0,     0,   170,   171,   172,     0,     0,
       0,   173,   174,     0,   744,     4,   175,  -117,     5,     6,
       7,     8,     9,     0,     0,     0,     0,    10,    11,    12,
       0,     0,     0,     0,     0,   176,   177,     0,     0,     0,
       0,     0,    13,    14,     0,    15,    16,   253,   254,   255,
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,     0,     0,   750,     5,     6,
       7,     8,   108,  -117,     0,     0,     0,    10,    11,    12,
       0,     0,  -117,   260,   261,   262,   263,   264,   265,   266,
     267,   268,   269,    14,   745,    15,     0,     0,     0,    17,
     253,   254,   255,   751,   256,   257,   258,   259,   260,   261,
     262,   263,   264,   265,   266,   267,   268,   269,   253,   254,
     255,     0,   256,   257,   258,   259,   260,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   253,   254,   255,     0,
     256,   257,   258,   259,   260,   261,   262,   263,   264,   265,
     266,   267,   268,   269,     0,     0,     0,     0,     0,     0,
       0,     0,   253,   254,   255,   821,   256,   257,   258,   259,
     260,   261,   262,   263,   264,   265,   266,   267,   268,   269,
       0,   429,   253,   254,   255,     0,   256,   257,   258,   259,
     260,   261,   262,   263,   264,   265,   266,   267,   268,   269,
       0,     0,     0,     0,   253,   254,   255,   430,   256,   257,
     258,   259,   260,   261,   262,   263,   264,   265,   266,   267,
     268,   269,     0,     0,     0,     0,     0,   578,    15,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   253,   254,   255,   743,   256,   257,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   268,   269,     5,
       6,     7,     8,    81,     0,     0,     0,     0,    10,    11,
      12,     5,     6,     7,     8,   117,     0,     0,     0,     0,
      10,    11,    12,     0,    14,     0,    15,     0,     0,     0,
       5,     6,     7,     8,    86,     0,    14,     0,    15,    10,
      11,    12,     5,     6,     7,     8,   122,     0,     0,     0,
       0,    10,    11,    12,     0,    14,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    14,   253,   254,
     255,   822,   256,   257,   258,   259,   260,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   253,   254,   255,     0,
     256,   257,   258,   259,   260,   261,   262,   263,   264,   265,
     266,   267,   268,   269,   255,     0,   256,   257,   258,   259,
     260,   261,   262,   263,   264,   265,   266,   267,   268,   269,
     257,   258,   259,   260,   261,   262,   263,   264,   265,   266,
     267,   268,   269
};

static const short yycheck[] =
{
      63,   223,    65,   316,   536,   314,     2,     3,     2,     3,
     144,   320,   289,   145,    10,    11,    12,   398,   274,   536,
     552,     2,     3,     2,     3,     2,     3,   408,    24,   398,
      26,   175,    28,   398,    30,   536,    32,   462,    34,   408,
      36,   312,    38,   408,     2,     3,     2,     3,     2,     3,
       2,     3,     2,     3,   214,   543,   462,   162,   163,   219,
      56,   618,    56,    23,   549,   559,     1,   172,    10,    65,
     592,    67,   304,   178,     1,    56,    48,    56,   595,    56,
     152,   765,   454,   188,   398,   536,   618,     1,    60,    61,
      62,     1,     0,   214,   408,   239,     1,     1,    56,   536,
      56,   552,    56,    75,    56,   249,    56,   251,   229,    67,
      85,    67,    84,    67,    89,    67,     3,   618,     0,   491,
     379,    93,     3,     4,    47,   809,     1,    69,    47,   385,
     102,    31,   504,     3,     4,   507,     3,     4,   298,   111,
      68,   658,   548,   549,     3,     4,     3,   379,   120,    85,
       3,    86,     8,    51,    52,    53,   528,   129,   595,    86,
     682,    84,   543,    40,    41,    84,   138,   618,   852,    56,
      90,   552,    68,    47,   543,    31,    90,   861,   543,   175,
      90,    86,    86,   552,    31,   145,   463,   552,   447,   448,
     449,   450,    59,    68,   566,   191,   568,   193,   850,   195,
      47,   197,    59,     3,   698,   201,    59,   203,   892,    90,
      84,    68,   275,    85,   335,    68,    69,    89,   214,   451,
      90,    68,    69,   219,    68,   747,   386,   879,     8,   543,
      68,    90,     6,   229,     8,   229,   208,   300,   552,     3,
       4,   726,   727,   239,   349,    90,   206,   207,   229,   806,
     229,    31,   229,   249,    90,   251,   214,    31,   214,    59,
     214,   219,   214,   219,   336,   219,   338,   219,    68,    69,
     168,   229,    68,   229,   806,   229,   498,   229,    31,   229,
      85,    86,   538,    47,    45,   429,    28,   431,   713,    31,
       8,   189,   780,   356,    47,    59,   401,   722,   569,   379,
     571,     3,   298,    45,    68,   806,    67,   713,    69,     5,
       6,     7,     8,    31,    68,   663,   722,    13,    14,    15,
     726,   727,     3,     4,   696,    68,    68,    69,   324,   677,
     326,    28,   328,   293,   330,   233,   399,   400,    84,   335,
     298,   335,   298,    85,   298,    47,   298,    89,    89,   309,
     310,    84,   869,   870,   335,   806,   335,    59,   335,    31,
      68,    69,    68,   735,    68,    69,    68,   447,   448,   449,
     450,    68,    69,    84,   272,   273,    84,   335,    59,   335,
      84,   335,    85,   335,     8,   335,    89,    68,     1,    85,
     386,     4,     5,     6,     7,     8,    68,    69,    11,   780,
      13,    14,    15,   716,   398,   718,     8,    28,    84,    68,
      31,   780,    86,    89,   408,   780,    29,   398,     3,     4,
      84,    84,    68,    69,    45,    89,    89,   408,   386,   561,
     386,    86,   386,   429,   386,   431,   570,   714,    84,    59,
     398,    84,   398,    85,    84,   343,   723,    89,    68,    69,
     408,    84,   408,     7,     8,   332,   333,    84,    47,    13,
      14,    15,    89,   623,    85,    84,   780,   664,    89,   666,
      89,    84,   632,   536,    59,   471,    85,   473,    85,   475,
     378,   477,    89,    68,    69,    85,   384,   859,   860,    89,
      85,   800,     1,   802,    89,     4,     5,     6,     7,     8,
       1,    84,     3,     4,    13,    14,    15,   213,    85,    33,
     216,    84,    89,   618,   220,     5,     6,     7,     8,    68,
      29,    10,    85,    13,    14,    15,    89,     6,   426,    85,
      63,    64,   595,    66,    67,    68,    69,   759,    45,   602,
     536,    31,   536,     4,     5,     6,     7,     8,    85,   543,
      11,    45,    13,    14,    15,   536,   552,   536,   552,   536,
       3,     4,   543,    89,   698,    68,    69,   699,    29,    89,
      31,   552,    84,   552,    84,   552,   447,   448,   536,   450,
     536,    90,   536,    47,   536,   543,   536,   543,   548,   549,
      59,    60,    61,    84,   552,    85,   552,    84,   552,    91,
     552,   561,   479,   480,    57,    58,    59,    60,    61,   315,
     316,    84,   572,    68,    69,   321,   493,   494,    85,    86,
      91,   224,   618,   521,   618,   182,   183,   623,    45,   692,
      86,   337,    68,    69,    86,   631,   632,   618,   536,   618,
      86,   618,   638,    90,   640,    47,   642,     1,   644,    86,
     646,    84,   648,   787,   650,    84,   652,    68,    69,    84,
     618,    91,   618,    91,   618,   623,   618,   623,   618,   623,
      85,   623,    68,    69,   632,    85,   632,    28,   632,    86,
     632,   387,     7,     8,   582,    89,   289,    84,    13,    14,
      15,    89,   755,   309,   310,   713,   714,   595,    24,    25,
      26,    27,    84,    84,   767,   603,    32,    33,    34,    35,
     773,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,     4,     5,     6,
       7,     8,     3,   610,   611,    47,    13,    14,    15,   699,
     803,   804,    16,   806,    18,    19,    20,    21,    47,   721,
      86,    89,    29,   713,   714,    47,    47,    85,    85,   465,
       8,    85,   722,   723,    85,    68,   726,   727,     1,    86,
      90,     4,     5,     6,     7,     8,    85,   654,   655,    85,
      13,    14,    15,    68,    86,    17,   780,   850,    86,   392,
      85,   854,    47,   379,    18,    68,    29,    84,    68,   780,
      68,    84,   508,   509,    85,    84,   512,   410,   411,    84,
     806,    68,   806,    85,    10,    91,   879,   880,    85,   405,
      89,    85,   780,    85,   780,   806,    84,   806,    68,   806,
      10,    85,   418,   419,    84,    24,    25,    26,    27,    84,
      10,    85,     0,    32,    33,    34,    35,     0,   806,   385,
     806,   305,   806,   552,   806,   408,   806,    90,   552,   462,
     463,   447,   448,   449,   450,   191,   192,   193,   194,   552,
     576,   412,   552,   408,   293,   567,   189,   680,   384,   591,
     726,     1,   713,   428,     4,     5,     6,     7,     8,   667,
     335,   670,   343,    13,    14,    15,    28,    29,    30,    31,
     380,   426,   480,   810,    36,    37,    38,    39,    28,    29,
     812,    31,   856,   499,   614,   501,    -1,   858,   624,   625,
      -1,   465,   628,    -1,    -1,    45,    -1,   633,   634,    -1,
     636,    28,    29,    30,    31,    -1,    -1,    -1,    -1,    36,
      37,    38,    39,   841,    -1,   548,   549,     1,    68,    69,
       4,     5,     6,     7,     8,    -1,    -1,    -1,    -1,    13,
      14,    15,    -1,   669,    -1,    85,   672,    -1,    -1,    89,
      90,    -1,    -1,    -1,    28,    29,    -1,    31,    54,    55,
      56,    57,    58,    59,    60,    61,     4,     5,     6,     7,
       8,    45,    -1,    -1,    -1,    13,    14,    15,   324,   325,
     326,   327,   191,   192,   193,   194,    -1,    -1,    -1,    -1,
     716,    29,   718,    31,    68,    69,     4,     5,     6,     7,
       8,   607,    -1,   626,   627,    13,    14,    15,    -1,    -1,
     736,    85,    -1,    -1,   740,    89,    90,    -1,    -1,    -1,
      -1,    29,     1,    31,    -1,     4,     5,     6,     7,     8,
       5,     6,     7,     8,    13,    14,    15,    -1,    13,    14,
      15,    -1,    -1,   195,   196,   197,   198,    85,    -1,    28,
      29,   384,    31,    -1,    -1,   781,   782,    -1,   784,    -1,
      -1,   667,    -1,   789,   670,   791,    45,    -1,     7,     8,
      -1,    -1,   798,    -1,    13,    14,    15,    85,   195,   196,
     197,   198,    -1,    -1,    -1,    -1,     5,     6,     7,     8,
     713,   714,    31,    -1,    13,    14,    15,    -1,     4,   722,
     723,     7,     8,   726,   727,    -1,    85,    13,    14,    15,
      89,    90,    31,     7,     8,   324,   325,   326,   327,    13,
      14,    15,    -1,    29,    -1,   471,   472,    -1,    -1,   475,
     476,     3,     4,     5,     6,     7,     8,    31,   471,   472,
     473,   474,   475,   476,   477,   478,    -1,     5,     6,     7,
       8,    -1,   775,   776,   777,    13,    14,    15,    -1,     1,
      -1,    -1,     4,    -1,   770,     7,     8,    -1,    -1,    -1,
      -1,    13,    14,    15,    -1,    -1,   328,   329,   330,   331,
      55,    56,    57,    58,    59,    60,    61,    29,    -1,    31,
      32,     1,    -1,     3,     4,     5,     6,     7,     8,    -1,
      -1,    11,    -1,    13,    14,    15,   812,   237,   238,    -1,
      -1,   328,   329,   330,   331,    -1,    -1,    -1,   248,    29,
      -1,    -1,    -1,   253,   254,    -1,    -1,    85,   258,   259,
     260,   261,   262,   263,   264,   265,   266,   267,   268,   269,
     846,   847,    -1,    85,    86,    -1,     4,     5,     6,     7,
       8,     5,     6,     7,     8,    13,    14,    15,    -1,    13,
      14,    15,   471,   472,    -1,    -1,   475,   476,    -1,    -1,
      -1,    29,    -1,    31,    84,    -1,    -1,    31,     5,     6,
       7,     8,    -1,    -1,    -1,    -1,    13,    14,    15,    -1,
      -1,    -1,   638,   639,   640,   641,    -1,    -1,    -1,    -1,
     646,   647,   648,   649,    31,   638,   639,   640,   641,   642,
     643,   644,   645,   646,   647,   648,   649,   650,   651,   652,
     653,   473,   474,    -1,    -1,   477,   478,    85,   358,   359,
      -1,    85,    -1,    -1,     1,    -1,     3,     4,    -1,    -1,
       7,     8,     9,    10,    -1,    12,    13,    14,    15,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   473,   474,    85,    -1,
     477,   478,    29,    30,    31,    32,    -1,    34,    35,    36,
      37,    38,    -1,    -1,     1,    -1,    43,     4,    -1,    -1,
       7,     8,    49,    -1,    -1,    52,    13,    14,    15,    -1,
      57,    58,    59,    -1,    -1,   425,    63,    64,    -1,    -1,
     430,    68,    29,    -1,    31,    32,    -1,   437,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,     3,    -1,
      87,    88,    -1,    90,     9,    10,    -1,    12,    -1,   638,
     639,   640,   641,     5,     6,     7,     8,   646,   647,   648,
     649,    13,    14,    15,    -1,    30,    -1,    32,    -1,    34,
      35,    36,    37,    38,    -1,    -1,    -1,    -1,    43,    31,
      45,    -1,    -1,    -1,    49,   495,    -1,    52,    -1,    -1,
      -1,    -1,    57,    58,    59,    -1,    -1,    -1,    63,    64,
      -1,    -1,    67,    68,    69,     4,    -1,   517,     7,     8,
     642,   643,   644,   645,    13,    14,    15,    -1,   650,   651,
     652,   653,    87,    88,    -1,    90,     5,     6,     7,     8,
      29,    -1,    31,    85,    13,    14,    15,    -1,    -1,    -1,
       4,     5,     6,     7,     8,   642,   643,   644,   645,    13,
      14,    15,    31,   650,   651,   652,   653,   567,    -1,    -1,
       4,     5,     6,     7,     8,    29,    -1,    31,   578,    13,
      14,    15,    -1,   583,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   592,   593,    -1,    29,    -1,    31,   598,     1,
      -1,     3,     4,     5,     6,     7,     8,     9,    10,    -1,
      12,    13,    14,    15,    16,    -1,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    -1,    -1,    -1,
      -1,    43,    -1,    -1,    -1,    -1,    -1,    49,    -1,     4,
      52,    85,     7,     8,    -1,    57,    58,    59,    13,    14,
      15,    63,    64,   663,    -1,    -1,    68,    -1,   668,    -1,
      -1,   671,    -1,    -1,    29,    -1,    31,   677,    -1,    -1,
     680,    -1,   682,    85,    86,    87,    88,    -1,    90,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,     3,
       4,     5,     6,     7,     8,     9,    10,    -1,    12,    13,
      14,    15,    16,    -1,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    -1,
      34,    35,    36,    37,    38,    -1,    -1,   737,    -1,    43,
      -1,   741,    -1,    -1,   744,    49,    -1,   747,    52,    -1,
     750,    -1,    -1,    57,    58,    59,    -1,    -1,    -1,    63,
      64,    -1,    -1,    -1,    68,     4,     5,     6,     7,     8,
       5,     6,     7,     8,    13,    14,    15,    -1,    13,    14,
      15,    85,    -1,    87,    88,     1,    90,     3,     4,    -1,
      29,    -1,    -1,     9,    10,    -1,    12,    -1,    -1,    -1,
      16,    -1,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    -1,    30,    -1,    32,    -1,    34,    35,
      36,    37,    38,    -1,    -1,    -1,    -1,    43,    -1,     5,
       6,     7,     8,    49,    -1,    -1,    52,    13,    14,    15,
      -1,    57,    58,    59,    -1,    -1,    85,    63,    64,    -1,
      85,    -1,    68,     4,     5,     6,     7,     8,     5,     6,
       7,     8,    13,    14,    15,    -1,    13,    14,    15,    85,
      -1,    87,    88,     1,    90,     3,     4,    -1,    29,    -1,
      -1,     9,    10,    -1,    12,    -1,    -1,    -1,    16,    -1,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    -1,    30,    -1,    32,    -1,    34,    35,    36,    37,
      38,    -1,    -1,    -1,    -1,    43,    -1,     5,     6,     7,
       8,    49,    -1,    -1,    52,    13,    14,    15,    -1,    57,
      58,    59,    -1,    -1,    85,    63,    64,    -1,    85,     1,
      68,     3,     4,    31,    -1,    -1,    -1,     9,    10,    -1,
      12,    -1,    -1,    -1,    -1,    -1,    -1,    85,    -1,    87,
      88,    -1,    90,    -1,    -1,    -1,    -1,    -1,    30,    -1,
      32,    -1,    34,    35,    36,    37,    38,    -1,    -1,    -1,
      -1,    43,    -1,     5,     6,     7,     8,    49,    -1,    -1,
      52,    13,    14,    15,    -1,    57,    58,    59,    -1,    -1,
      -1,    63,    64,    -1,    -1,    67,    68,    69,     1,    31,
       3,     4,    -1,    -1,    -1,    -1,     9,    10,    -1,    12,
      -1,    -1,    -1,    -1,    86,    87,    88,    -1,    90,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    30,    -1,    32,
      -1,    34,    35,    36,    37,    38,    -1,    -1,    -1,    -1,
      43,    -1,    -1,    -1,    -1,    -1,    49,    -1,     4,    52,
      -1,     7,     8,    -1,    57,    58,    59,    13,    14,    15,
      63,    64,    -1,    -1,    67,    68,    69,     1,    -1,     3,
      -1,    -1,    -1,    29,    -1,     9,    10,    -1,    12,    -1,
      -1,    -1,    -1,    86,    87,    88,    -1,    90,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    30,    -1,    32,    -1,
      34,    35,    36,    37,    38,    -1,    -1,    -1,    -1,    43,
       4,     5,     6,     7,     8,    49,    -1,    -1,    52,    13,
      14,    15,    -1,    57,    58,    59,    -1,    -1,    -1,    63,
      64,     1,    -1,     3,    68,    29,    -1,    -1,    -1,     9,
      10,    -1,    12,    -1,    -1,    -1,    -1,    -1,    -1,     5,
       6,     7,     8,    87,    88,    -1,    90,    13,    14,    15,
      30,    -1,    32,    -1,    34,    35,    36,    37,    38,    -1,
      -1,    -1,    -1,    43,     4,     5,     6,     7,     8,    49,
      -1,    -1,    52,    13,    14,    15,    -1,    57,    58,    59,
      -1,    85,    -1,    63,    64,     3,    -1,    -1,    68,    29,
      -1,     9,    10,    -1,    12,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    87,    88,    -1,
      90,    -1,    30,    -1,    32,    -1,    34,    35,    36,    37,
      38,    -1,    -1,    -1,    -1,    43,     3,    -1,    -1,    -1,
      -1,    49,     9,    10,    52,    12,    -1,    -1,    -1,    57,
      58,    59,    -1,    -1,    -1,    63,    64,    -1,    -1,    -1,
      68,    -1,    -1,    30,    -1,    32,    -1,    34,    35,    36,
      37,    38,    -1,    -1,    -1,    -1,    43,    -1,    -1,    87,
      88,    -1,    49,    91,    -1,    52,    -1,    -1,    -1,    -1,
      57,    58,    59,    -1,    -1,    -1,    63,    64,    -1,    -1,
      -1,    68,    -1,    -1,    -1,    -1,     3,     4,     5,     6,
       7,     8,     9,    10,    -1,    12,    13,    14,    15,    -1,
      87,    88,    -1,    -1,    91,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    -1,    34,    35,    36,
      37,    38,    -1,    -1,    -1,    -1,    43,     4,     5,     6,
       7,     8,    49,    -1,    -1,    52,    13,    14,    15,    -1,
      57,    58,    59,    -1,    -1,    -1,    63,    64,    -1,    -1,
      -1,    68,    29,    -1,    -1,     3,     4,    -1,    -1,    -1,
      -1,     9,    10,    -1,    12,    -1,    -1,    -1,    85,    -1,
      87,    88,    -1,    90,    22,    23,    24,    25,    26,    27,
      28,    -1,    30,    -1,    32,    -1,    34,    35,    36,    37,
      38,    -1,    -1,    -1,    -1,    43,    -1,    -1,    -1,    -1,
      -1,    49,    -1,     4,    52,    -1,     7,     8,    85,    57,
      58,    59,    13,    14,    15,    63,    64,     3,    -1,    -1,
      68,    -1,    -1,     9,    10,    -1,    12,    -1,    29,    -1,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    85,    -1,    87,
      88,    -1,    90,    -1,    30,    -1,    32,    -1,    34,    35,
      36,    37,    38,    -1,    -1,    -1,    -1,    43,    -1,     5,
       6,     7,     8,    49,    -1,    -1,    52,    13,    14,    15,
      -1,    57,    58,    59,    -1,    -1,    -1,    63,    64,    -1,
      -1,    -1,    68,     3,     4,     5,     6,     7,     8,     9,
      10,    -1,    12,    13,    14,    15,     4,     5,     6,     7,
       8,    87,    88,    -1,    90,    13,    14,    15,    -1,    29,
      30,    31,    32,    -1,    34,    35,    36,    37,    38,    -1,
      -1,    29,    -1,    43,    -1,    -1,    -1,    -1,    -1,    49,
      -1,    -1,    52,    -1,    -1,    -1,    -1,    57,    58,    59,
      -1,    -1,    -1,    63,    64,     3,     4,    -1,    68,     7,
       8,     9,    10,    -1,    12,    13,    14,    15,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    87,    88,    -1,
      -1,    29,    30,    31,    32,    -1,    34,    35,    36,    37,
      38,    -1,    -1,    -1,    -1,    43,     3,    -1,    -1,    -1,
      -1,    49,     9,    10,    52,    12,    -1,    -1,    -1,    57,
      58,    59,    -1,    -1,    -1,    63,    64,    -1,    -1,    -1,
      68,    -1,    -1,    30,    -1,    32,    -1,    34,    35,    36,
      37,    38,    -1,    -1,    -1,    -1,    43,     3,    -1,    87,
      88,    -1,    49,     9,    10,    52,    12,    -1,    -1,    -1,
      57,    58,    59,    -1,    -1,    -1,    63,    64,    -1,    -1,
      -1,    68,    -1,    -1,    30,    -1,    32,    -1,    34,    35,
      36,    37,    38,    -1,    -1,    -1,    -1,    43,    85,    -1,
      87,    88,    -1,    49,    -1,    -1,    52,    -1,    -1,    -1,
      -1,    57,    58,    59,    -1,    -1,    -1,    63,    64,     3,
      -1,    -1,    68,    -1,    -1,     9,    10,    -1,    12,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      -1,    87,    88,    -1,    -1,    -1,    30,    -1,    32,    -1,
      34,    35,    36,    37,    38,    -1,    -1,    -1,    -1,    43,
       3,    -1,    -1,    -1,    -1,    49,     9,    10,    52,    12,
      -1,    -1,    -1,    57,    58,    59,    -1,    -1,    -1,    63,
      64,    -1,    -1,    -1,    68,    -1,    -1,    30,    -1,    32,
      -1,    34,    35,    36,    37,    38,    -1,    -1,    -1,    -1,
      43,     3,    -1,    87,    88,    -1,    49,     9,    10,    52,
      12,    -1,    -1,    -1,    57,    58,    59,    -1,    -1,    -1,
      63,    64,    -1,    -1,    -1,    68,    -1,    -1,    30,    -1,
      32,    -1,    34,    35,    36,    37,    38,    -1,    -1,    -1,
      -1,    43,    -1,    -1,    87,    88,    -1,    49,    -1,    -1,
      52,    -1,    -1,    -1,    -1,    57,    58,    59,    -1,    -1,
      -1,    63,    64,    -1,    11,     1,    68,     3,     4,     5,
       6,     7,     8,    -1,    -1,    -1,    -1,    13,    14,    15,
      -1,    -1,    -1,    -1,    -1,    87,    88,    -1,    -1,    -1,
      -1,    -1,    28,    29,    -1,    31,    32,    44,    45,    46,
      -1,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    -1,    -1,    11,     4,     5,
       6,     7,     8,    59,    -1,    -1,    -1,    13,    14,    15,
      -1,    -1,    68,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    29,    91,    31,    -1,    -1,    -1,    85,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    44,    45,
      46,    -1,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    44,    45,    46,    -1,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    44,    45,    46,    91,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      -1,    89,    44,    45,    46,    -1,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      -1,    -1,    -1,    -1,    44,    45,    46,    89,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    -1,    -1,    -1,    -1,    -1,    89,    31,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    44,    45,    46,    84,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,     4,
       5,     6,     7,     8,    -1,    -1,    -1,    -1,    13,    14,
      15,     4,     5,     6,     7,     8,    -1,    -1,    -1,    -1,
      13,    14,    15,    -1,    29,    -1,    31,    -1,    -1,    -1,
       4,     5,     6,     7,     8,    -1,    29,    -1,    31,    13,
      14,    15,     4,     5,     6,     7,     8,    -1,    -1,    -1,
      -1,    13,    14,    15,    -1,    29,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    29,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    44,    45,    46,    -1,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    46,    -1,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/share/bison/bison.simple"

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

#line 315 "/usr/share/bison/bison.simple"


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

case 1:
#line 324 "c-parse.y"
{ if (pedantic)
		    pedwarn ("ISO C forbids an empty source file");
		  finish_file ();
		;
    break;}
case 2:
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
		;
    break;}
case 3:
#line 347 "c-parse.y"
{yyval.ttype = NULL_TREE; ;
    break;}
case 5:
#line 348 "c-parse.y"
{yyval.ttype = NULL_TREE; ggc_collect(); ;
    break;}
case 7:
#line 353 "c-parse.y"
{ parsing_iso_function_signature = false; ;
    break;}
case 10:
#line 360 "c-parse.y"
{ STRIP_NOPS (yyvsp[-2].ttype);
		  if ((TREE_CODE (yyvsp[-2].ttype) == ADDR_EXPR
		       && TREE_CODE (TREE_OPERAND (yyvsp[-2].ttype, 0)) == STRING_CST)
		      || TREE_CODE (yyvsp[-2].ttype) == STRING_CST)
		    assemble_asm (yyvsp[-2].ttype);
		  else
		    error ("argument of `asm' is not a constant string"); ;
    break;}
case 11:
#line 368 "c-parse.y"
{ RESTORE_EXT_FLAGS (yyvsp[-1].ttype); ;
    break;}
case 12:
#line 373 "c-parse.y"
{ if (pedantic)
		    error ("ISO C forbids data definition with no type or storage class");
		  else
		    warning ("data definition has no type or storage class");

		  POP_DECLSPEC_STACK; ;
    break;}
case 13:
#line 380 "c-parse.y"
{ POP_DECLSPEC_STACK; ;
    break;}
case 14:
#line 382 "c-parse.y"
{ POP_DECLSPEC_STACK; ;
    break;}
case 15:
#line 384 "c-parse.y"
{ shadow_tag (yyvsp[-1].ttype); ;
    break;}
case 18:
#line 388 "c-parse.y"
{ if (pedantic)
		    pedwarn ("ISO C does not allow extra `;' outside of a function"); ;
    break;}
case 19:
#line 394 "c-parse.y"
{ if (! start_function (current_declspecs, yyvsp[0].ttype,
					all_prefix_attributes))
		    YYERROR1;
		;
    break;}
case 20:
#line 399 "c-parse.y"
{ store_parm_decls (); ;
    break;}
case 21:
#line 401 "c-parse.y"
{ DECL_SOURCE_FILE (current_function_decl) = yyvsp[-2].filename;
		  DECL_SOURCE_LINE (current_function_decl) = yyvsp[-1].lineno;
		  finish_function (0, 1);
		  POP_DECLSPEC_STACK; ;
    break;}
case 22:
#line 406 "c-parse.y"
{ POP_DECLSPEC_STACK; ;
    break;}
case 23:
#line 408 "c-parse.y"
{ if (! start_function (current_declspecs, yyvsp[0].ttype,
					all_prefix_attributes))
		    YYERROR1;
		;
    break;}
case 24:
#line 413 "c-parse.y"
{ store_parm_decls (); ;
    break;}
case 25:
#line 415 "c-parse.y"
{ DECL_SOURCE_FILE (current_function_decl) = yyvsp[-2].filename;
		  DECL_SOURCE_LINE (current_function_decl) = yyvsp[-1].lineno;
		  finish_function (0, 1);
		  POP_DECLSPEC_STACK; ;
    break;}
case 26:
#line 420 "c-parse.y"
{ POP_DECLSPEC_STACK; ;
    break;}
case 27:
#line 422 "c-parse.y"
{ if (! start_function (NULL_TREE, yyvsp[0].ttype,
					all_prefix_attributes))
		    YYERROR1;
		;
    break;}
case 28:
#line 427 "c-parse.y"
{ store_parm_decls (); ;
    break;}
case 29:
#line 429 "c-parse.y"
{ DECL_SOURCE_FILE (current_function_decl) = yyvsp[-2].filename;
		  DECL_SOURCE_LINE (current_function_decl) = yyvsp[-1].lineno;
		  finish_function (0, 1);
		  POP_DECLSPEC_STACK; ;
    break;}
case 30:
#line 434 "c-parse.y"
{ POP_DECLSPEC_STACK; ;
    break;}
case 33:
#line 443 "c-parse.y"
{ yyval.code = ADDR_EXPR; ;
    break;}
case 34:
#line 445 "c-parse.y"
{ yyval.code = NEGATE_EXPR; ;
    break;}
case 35:
#line 447 "c-parse.y"
{ yyval.code = CONVERT_EXPR;
  if (warn_traditional && !in_system_header)
    warning ("traditional C rejects the unary plus operator");
		;
    break;}
case 36:
#line 452 "c-parse.y"
{ yyval.code = PREINCREMENT_EXPR; ;
    break;}
case 37:
#line 454 "c-parse.y"
{ yyval.code = PREDECREMENT_EXPR; ;
    break;}
case 38:
#line 456 "c-parse.y"
{ yyval.code = BIT_NOT_EXPR; ;
    break;}
case 39:
#line 458 "c-parse.y"
{ yyval.code = TRUTH_NOT_EXPR; ;
    break;}
case 40:
#line 462 "c-parse.y"
{ yyval.ttype = build_compound_expr (yyvsp[0].ttype); ;
    break;}
case 41:
#line 467 "c-parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 43:
#line 473 "c-parse.y"
{ yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ttype); ;
    break;}
case 44:
#line 475 "c-parse.y"
{ chainon (yyvsp[-2].ttype, build_tree_list (NULL_TREE, yyvsp[0].ttype)); ;
    break;}
case 46:
#line 481 "c-parse.y"
{ yyval.ttype = build_indirect_ref (yyvsp[0].ttype, "unary *"); ;
    break;}
case 47:
#line 484 "c-parse.y"
{ yyval.ttype = yyvsp[0].ttype;
		  RESTORE_EXT_FLAGS (yyvsp[-1].ttype); ;
    break;}
case 48:
#line 487 "c-parse.y"
{ yyval.ttype = build_unary_op (yyvsp[-1].code, yyvsp[0].ttype, 0);
		  overflow_warning (yyval.ttype); ;
    break;}
case 49:
#line 491 "c-parse.y"
{ yyval.ttype = finish_label_address_expr (yyvsp[0].ttype); ;
    break;}
case 50:
#line 493 "c-parse.y"
{ skip_evaluation--;
		  if (TREE_CODE (yyvsp[0].ttype) == COMPONENT_REF
		      && DECL_C_BIT_FIELD (TREE_OPERAND (yyvsp[0].ttype, 1)))
		    error ("`sizeof' applied to a bit-field");
		  yyval.ttype = c_sizeof (TREE_TYPE (yyvsp[0].ttype)); ;
    break;}
case 51:
#line 499 "c-parse.y"
{ skip_evaluation--;
		  yyval.ttype = c_sizeof (groktypename (yyvsp[-1].ttype)); ;
    break;}
case 52:
#line 502 "c-parse.y"
{ skip_evaluation--;
		  yyval.ttype = c_alignof_expr (yyvsp[0].ttype); ;
    break;}
case 53:
#line 505 "c-parse.y"
{ skip_evaluation--;
		  yyval.ttype = c_alignof (groktypename (yyvsp[-1].ttype)); ;
    break;}
case 54:
#line 508 "c-parse.y"
{ yyval.ttype = build_unary_op (REALPART_EXPR, yyvsp[0].ttype, 0); ;
    break;}
case 55:
#line 510 "c-parse.y"
{ yyval.ttype = build_unary_op (IMAGPART_EXPR, yyvsp[0].ttype, 0); ;
    break;}
case 56:
#line 514 "c-parse.y"
{ skip_evaluation++; ;
    break;}
case 57:
#line 518 "c-parse.y"
{ skip_evaluation++; ;
    break;}
case 58:
#line 522 "c-parse.y"
{ skip_evaluation++; ;
    break;}
case 60:
#line 528 "c-parse.y"
{ yyval.ttype = c_cast_expr (yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 62:
#line 534 "c-parse.y"
{ yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 63:
#line 536 "c-parse.y"
{ yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 64:
#line 538 "c-parse.y"
{ yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 65:
#line 540 "c-parse.y"
{ yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 66:
#line 542 "c-parse.y"
{ yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 67:
#line 544 "c-parse.y"
{ yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 68:
#line 546 "c-parse.y"
{ yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 69:
#line 548 "c-parse.y"
{ yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 70:
#line 550 "c-parse.y"
{ yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 71:
#line 552 "c-parse.y"
{ yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 72:
#line 554 "c-parse.y"
{ yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 73:
#line 556 "c-parse.y"
{ yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 74:
#line 558 "c-parse.y"
{ yyvsp[-1].ttype = c_common_truthvalue_conversion
		    (default_conversion (yyvsp[-1].ttype));
		  skip_evaluation += yyvsp[-1].ttype == boolean_false_node; ;
    break;}
case 75:
#line 562 "c-parse.y"
{ skip_evaluation -= yyvsp[-3].ttype == boolean_false_node;
		  yyval.ttype = parser_build_binary_op (TRUTH_ANDIF_EXPR, yyvsp[-3].ttype, yyvsp[0].ttype); ;
    break;}
case 76:
#line 565 "c-parse.y"
{ yyvsp[-1].ttype = c_common_truthvalue_conversion
		    (default_conversion (yyvsp[-1].ttype));
		  skip_evaluation += yyvsp[-1].ttype == boolean_true_node; ;
    break;}
case 77:
#line 569 "c-parse.y"
{ skip_evaluation -= yyvsp[-3].ttype == boolean_true_node;
		  yyval.ttype = parser_build_binary_op (TRUTH_ORIF_EXPR, yyvsp[-3].ttype, yyvsp[0].ttype); ;
    break;}
case 78:
#line 572 "c-parse.y"
{ yyvsp[-1].ttype = c_common_truthvalue_conversion
		    (default_conversion (yyvsp[-1].ttype));
		  skip_evaluation += yyvsp[-1].ttype == boolean_false_node; ;
    break;}
case 79:
#line 576 "c-parse.y"
{ skip_evaluation += ((yyvsp[-4].ttype == boolean_true_node)
				      - (yyvsp[-4].ttype == boolean_false_node)); ;
    break;}
case 80:
#line 579 "c-parse.y"
{ skip_evaluation -= yyvsp[-6].ttype == boolean_true_node;
		  yyval.ttype = build_conditional_expr (yyvsp[-6].ttype, yyvsp[-3].ttype, yyvsp[0].ttype); ;
    break;}
case 81:
#line 582 "c-parse.y"
{ if (pedantic)
		    pedwarn ("ISO C forbids omitting the middle term of a ?: expression");
		  /* Make sure first operand is calculated only once.  */
		  yyvsp[0].ttype = save_expr (yyvsp[-1].ttype);
		  yyvsp[-1].ttype = c_common_truthvalue_conversion
		    (default_conversion (yyvsp[0].ttype));
		  skip_evaluation += yyvsp[-1].ttype == boolean_true_node; ;
    break;}
case 82:
#line 590 "c-parse.y"
{ skip_evaluation -= yyvsp[-4].ttype == boolean_true_node;
		  yyval.ttype = build_conditional_expr (yyvsp[-4].ttype, yyvsp[-3].ttype, yyvsp[0].ttype); ;
    break;}
case 83:
#line 593 "c-parse.y"
{ char class;
		  yyval.ttype = build_modify_expr (yyvsp[-2].ttype, NOP_EXPR, yyvsp[0].ttype);
		  class = TREE_CODE_CLASS (TREE_CODE (yyval.ttype));
		  if (IS_EXPR_CODE_CLASS (class))
		    C_SET_EXP_ORIGINAL_CODE (yyval.ttype, MODIFY_EXPR);
		;
    break;}
case 84:
#line 600 "c-parse.y"
{ char class;
		  yyval.ttype = build_modify_expr (yyvsp[-2].ttype, yyvsp[-1].code, yyvsp[0].ttype);
		  /* This inhibits warnings in
		     c_common_truthvalue_conversion.  */
		  class = TREE_CODE_CLASS (TREE_CODE (yyval.ttype));
		  if (IS_EXPR_CODE_CLASS (class))
		    C_SET_EXP_ORIGINAL_CODE (yyval.ttype, ERROR_MARK);
		;
    break;}
case 85:
#line 612 "c-parse.y"
{
		  if (yychar == YYEMPTY)
		    yychar = YYLEX;
		  yyval.ttype = build_external_ref (yyvsp[0].ttype, yychar == '(');
		;
    break;}
case 87:
#line 619 "c-parse.y"
{ yyval.ttype = fix_string_type (yyval.ttype); ;
    break;}
case 88:
#line 621 "c-parse.y"
{ yyval.ttype = fname_decl (C_RID_CODE (yyval.ttype), yyval.ttype); ;
    break;}
case 89:
#line 623 "c-parse.y"
{ start_init (NULL_TREE, NULL, 0);
		  yyvsp[-2].ttype = groktypename (yyvsp[-2].ttype);
		  really_start_incremental_init (yyvsp[-2].ttype); ;
    break;}
case 90:
#line 627 "c-parse.y"
{ tree constructor = pop_init_level (0);
		  tree type = yyvsp[-5].ttype;
		  finish_init ();

		  if (pedantic && ! flag_isoc99)
		    pedwarn ("ISO C89 forbids compound literals");
		  yyval.ttype = build_compound_literal (type, constructor);
		;
    break;}
case 91:
#line 636 "c-parse.y"
{ char class = TREE_CODE_CLASS (TREE_CODE (yyvsp[-1].ttype));
		  if (IS_EXPR_CODE_CLASS (class))
		    C_SET_EXP_ORIGINAL_CODE (yyvsp[-1].ttype, ERROR_MARK);
		  yyval.ttype = yyvsp[-1].ttype; ;
    break;}
case 92:
#line 641 "c-parse.y"
{ yyval.ttype = error_mark_node; ;
    break;}
case 93:
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
		;
    break;}
case 94:
#line 659 "c-parse.y"
{
		  pop_label_level ();
		  last_tree = COMPOUND_BODY (yyvsp[-2].ttype);
		  TREE_CHAIN (last_tree) = NULL_TREE;
		  yyval.ttype = error_mark_node;
		;
    break;}
case 95:
#line 666 "c-parse.y"
{ yyval.ttype = build_function_call (yyvsp[-3].ttype, yyvsp[-1].ttype); ;
    break;}
case 96:
#line 668 "c-parse.y"
{ yyval.ttype = build_va_arg (yyvsp[-3].ttype, groktypename (yyvsp[-1].ttype)); ;
    break;}
case 97:
#line 671 "c-parse.y"
{
                  tree c;

                  c = fold (yyvsp[-5].ttype);
                  STRIP_NOPS (c);
                  if (TREE_CODE (c) != INTEGER_CST)
                    error ("first argument to __builtin_choose_expr not a constant");
                  yyval.ttype = integer_zerop (c) ? yyvsp[-1].ttype : yyvsp[-3].ttype;
		;
    break;}
case 98:
#line 681 "c-parse.y"
{
		  tree e1, e2;

		  e1 = TYPE_MAIN_VARIANT (groktypename (yyvsp[-3].ttype));
		  e2 = TYPE_MAIN_VARIANT (groktypename (yyvsp[-1].ttype));

		  yyval.ttype = comptypes (e1, e2)
		    ? build_int_2 (1, 0) : build_int_2 (0, 0);
		;
    break;}
case 99:
#line 691 "c-parse.y"
{ yyval.ttype = build_array_ref (yyvsp[-3].ttype, yyvsp[-1].ttype); ;
    break;}
case 100:
#line 693 "c-parse.y"
{
		      yyval.ttype = build_component_ref (yyvsp[-2].ttype, yyvsp[0].ttype);
		;
    break;}
case 101:
#line 697 "c-parse.y"
{
                  tree expr = build_indirect_ref (yyvsp[-2].ttype, "->");

			yyval.ttype = build_component_ref (expr, yyvsp[0].ttype);
		;
    break;}
case 102:
#line 703 "c-parse.y"
{ yyval.ttype = build_unary_op (POSTINCREMENT_EXPR, yyvsp[-1].ttype, 0); ;
    break;}
case 103:
#line 705 "c-parse.y"
{ yyval.ttype = build_unary_op (POSTDECREMENT_EXPR, yyvsp[-1].ttype, 0); ;
    break;}
case 104:
#line 711 "c-parse.y"
{
	  parsing_iso_function_signature = false; /* Reset after decls.  */
	;
    break;}
case 105:
#line 718 "c-parse.y"
{
	  if (warn_traditional && !in_system_header
	      && parsing_iso_function_signature)
	    warning ("traditional C rejects ISO C style function definitions");
	  parsing_iso_function_signature = false; /* Reset after warning.  */
	;
    break;}
case 107:
#line 732 "c-parse.y"
{ ;
    break;}
case 112:
#line 748 "c-parse.y"
{ POP_DECLSPEC_STACK; ;
    break;}
case 113:
#line 750 "c-parse.y"
{ POP_DECLSPEC_STACK; ;
    break;}
case 114:
#line 752 "c-parse.y"
{ shadow_tag_warned (yyvsp[-1].ttype, 1);
		  pedwarn ("empty declaration"); ;
    break;}
case 115:
#line 755 "c-parse.y"
{ pedwarn ("empty declaration"); ;
    break;}
case 116:
#line 764 "c-parse.y"
{ ;
    break;}
case 117:
#line 772 "c-parse.y"
{ pending_xref_error ();
		  PUSH_DECLSPEC_STACK;
		  split_specs_attrs (yyvsp[0].ttype,
				     &current_declspecs, &prefix_attributes);
		  all_prefix_attributes = prefix_attributes; ;
    break;}
case 118:
#line 783 "c-parse.y"
{ all_prefix_attributes = chainon (yyvsp[0].ttype, prefix_attributes); ;
    break;}
case 119:
#line 788 "c-parse.y"
{ POP_DECLSPEC_STACK; ;
    break;}
case 120:
#line 790 "c-parse.y"
{ POP_DECLSPEC_STACK; ;
    break;}
case 121:
#line 792 "c-parse.y"
{ POP_DECLSPEC_STACK; ;
    break;}
case 122:
#line 794 "c-parse.y"
{ POP_DECLSPEC_STACK; ;
    break;}
case 123:
#line 796 "c-parse.y"
{ shadow_tag (yyvsp[-1].ttype); ;
    break;}
case 124:
#line 798 "c-parse.y"
{ RESTORE_EXT_FLAGS (yyvsp[-1].ttype); ;
    break;}
case 125:
#line 855 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, NULL_TREE);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 126:
#line 858 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 127:
#line 861 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 128:
#line 867 "c-parse.y"
{ yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;
    break;}
case 129:
#line 873 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 130:
#line 876 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 131:
#line 882 "c-parse.y"
{ yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, NULL_TREE);
		  TREE_STATIC (yyval.ttype) = 0; ;
    break;}
case 132:
#line 885 "c-parse.y"
{ yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;
    break;}
case 133:
#line 891 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, NULL_TREE);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 134:
#line 894 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 135:
#line 897 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 136:
#line 900 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 137:
#line 903 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 138:
#line 906 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 139:
#line 909 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 140:
#line 915 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, NULL_TREE);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 141:
#line 918 "c-parse.y"
{ yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;
    break;}
case 142:
#line 921 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 143:
#line 924 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 144:
#line 927 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 145:
#line 930 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 146:
#line 936 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 147:
#line 939 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 148:
#line 942 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 149:
#line 945 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 150:
#line 948 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 151:
#line 951 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 152:
#line 957 "c-parse.y"
{ yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;
    break;}
case 153:
#line 960 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 154:
#line 963 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 155:
#line 966 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 156:
#line 969 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 157:
#line 975 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, NULL_TREE);
		  TREE_STATIC (yyval.ttype) = 0; ;
    break;}
case 158:
#line 978 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 159:
#line 981 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 160:
#line 984 "c-parse.y"
{ if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;
    break;}
case 161:
#line 990 "c-parse.y"
{ if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;
    break;}
case 162:
#line 996 "c-parse.y"
{ if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;
    break;}
case 163:
#line 1002 "c-parse.y"
{ if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;
    break;}
case 164:
#line 1011 "c-parse.y"
{ yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;
    break;}
case 165:
#line 1017 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 166:
#line 1020 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 167:
#line 1023 "c-parse.y"
{ if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;
    break;}
case 168:
#line 1029 "c-parse.y"
{ if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;
    break;}
case 169:
#line 1035 "c-parse.y"
{ if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;
    break;}
case 170:
#line 1041 "c-parse.y"
{ if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;
    break;}
case 171:
#line 1050 "c-parse.y"
{ yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;
    break;}
case 172:
#line 1056 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 173:
#line 1059 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 174:
#line 1062 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 175:
#line 1065 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 176:
#line 1068 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 177:
#line 1071 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 178:
#line 1074 "c-parse.y"
{ if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;
    break;}
case 179:
#line 1080 "c-parse.y"
{ if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;
    break;}
case 180:
#line 1086 "c-parse.y"
{ if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;
    break;}
case 181:
#line 1092 "c-parse.y"
{ if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;
    break;}
case 182:
#line 1101 "c-parse.y"
{ yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;
    break;}
case 183:
#line 1104 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 184:
#line 1107 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 185:
#line 1110 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 186:
#line 1113 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 187:
#line 1119 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 188:
#line 1122 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 189:
#line 1125 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 190:
#line 1128 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 191:
#line 1131 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 192:
#line 1134 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 193:
#line 1137 "c-parse.y"
{ if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;
    break;}
case 194:
#line 1143 "c-parse.y"
{ if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;
    break;}
case 195:
#line 1149 "c-parse.y"
{ if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;
    break;}
case 196:
#line 1155 "c-parse.y"
{ if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;
    break;}
case 197:
#line 1164 "c-parse.y"
{ yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;
    break;}
case 198:
#line 1167 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 199:
#line 1170 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 200:
#line 1173 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 201:
#line 1176 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;
    break;}
case 258:
#line 1264 "c-parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 259:
#line 1266 "c-parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 263:
#line 1301 "c-parse.y"
{ OBJC_NEED_RAW_IDENTIFIER (1);	;
    break;}
case 266:
#line 1311 "c-parse.y"
{ /* For a typedef name, record the meaning, not the name.
		     In case of `foo foo, bar;'.  */
		  yyval.ttype = lookup_name (yyvsp[0].ttype); ;
    break;}
case 267:
#line 1315 "c-parse.y"
{ skip_evaluation--; yyval.ttype = TREE_TYPE (yyvsp[-1].ttype); ;
    break;}
case 268:
#line 1317 "c-parse.y"
{ skip_evaluation--; yyval.ttype = groktypename (yyvsp[-1].ttype); ;
    break;}
case 273:
#line 1334 "c-parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 274:
#line 1336 "c-parse.y"
{ yyval.ttype = yyvsp[-1].ttype; ;
    break;}
case 275:
#line 1341 "c-parse.y"
{ yyval.ttype = start_decl (yyvsp[-3].ttype, current_declspecs, 1,
					  chainon (yyvsp[-1].ttype, all_prefix_attributes));
		  start_init (yyval.ttype, yyvsp[-2].ttype, global_bindings_p ()); ;
    break;}
case 276:
#line 1346 "c-parse.y"
{ finish_init ();
		  finish_decl (yyvsp[-1].ttype, yyvsp[0].ttype, yyvsp[-4].ttype); ;
    break;}
case 277:
#line 1349 "c-parse.y"
{ tree d = start_decl (yyvsp[-2].ttype, current_declspecs, 0,
				       chainon (yyvsp[0].ttype, all_prefix_attributes));
		  finish_decl (d, NULL_TREE, yyvsp[-1].ttype);
                ;
    break;}
case 278:
#line 1357 "c-parse.y"
{ yyval.ttype = start_decl (yyvsp[-3].ttype, current_declspecs, 1,
					  chainon (yyvsp[-1].ttype, all_prefix_attributes));
		  start_init (yyval.ttype, yyvsp[-2].ttype, global_bindings_p ()); ;
    break;}
case 279:
#line 1362 "c-parse.y"
{ finish_init ();
		  finish_decl (yyvsp[-1].ttype, yyvsp[0].ttype, yyvsp[-4].ttype); ;
    break;}
case 280:
#line 1365 "c-parse.y"
{ tree d = start_decl (yyvsp[-2].ttype, current_declspecs, 0,
				       chainon (yyvsp[0].ttype, all_prefix_attributes));
		  finish_decl (d, NULL_TREE, yyvsp[-1].ttype); ;
    break;}
case 281:
#line 1373 "c-parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 282:
#line 1375 "c-parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 283:
#line 1380 "c-parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 284:
#line 1382 "c-parse.y"
{ yyval.ttype = chainon (yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 285:
#line 1387 "c-parse.y"
{ yyval.ttype = yyvsp[-2].ttype; ;
    break;}
case 286:
#line 1392 "c-parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 287:
#line 1394 "c-parse.y"
{ yyval.ttype = chainon (yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 288:
#line 1399 "c-parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 289:
#line 1401 "c-parse.y"
{ yyval.ttype = build_tree_list (yyvsp[0].ttype, NULL_TREE); ;
    break;}
case 290:
#line 1403 "c-parse.y"
{ yyval.ttype = build_tree_list (yyvsp[-3].ttype, build_tree_list (NULL_TREE, yyvsp[-1].ttype)); ;
    break;}
case 291:
#line 1405 "c-parse.y"
{ yyval.ttype = build_tree_list (yyvsp[-5].ttype, tree_cons (NULL_TREE, yyvsp[-3].ttype, yyvsp[-1].ttype)); ;
    break;}
case 292:
#line 1407 "c-parse.y"
{ yyval.ttype = build_tree_list (yyvsp[-3].ttype, yyvsp[-1].ttype); ;
    break;}
case 300:
#line 1430 "c-parse.y"
{ really_start_incremental_init (NULL_TREE); ;
    break;}
case 301:
#line 1432 "c-parse.y"
{ yyval.ttype = pop_init_level (0); ;
    break;}
case 302:
#line 1434 "c-parse.y"
{ yyval.ttype = error_mark_node; ;
    break;}
case 303:
#line 1440 "c-parse.y"
{ if (pedantic)
		    pedwarn ("ISO C forbids empty initializer braces"); ;
    break;}
case 307:
#line 1454 "c-parse.y"
{ if (pedantic && ! flag_isoc99)
		    pedwarn ("ISO C89 forbids specifying subobject to initialize"); ;
    break;}
case 308:
#line 1457 "c-parse.y"
{ if (pedantic)
		    pedwarn ("obsolete use of designated initializer without `='"); ;
    break;}
case 309:
#line 1460 "c-parse.y"
{ set_init_label (yyvsp[-1].ttype);
		  if (pedantic)
		    pedwarn ("obsolete use of designated initializer with `:'"); ;
    break;}
case 310:
#line 1464 "c-parse.y"
{;
    break;}
case 312:
#line 1470 "c-parse.y"
{ push_init_level (0); ;
    break;}
case 313:
#line 1472 "c-parse.y"
{ process_init_element (pop_init_level (0)); ;
    break;}
case 314:
#line 1474 "c-parse.y"
{ process_init_element (yyvsp[0].ttype); ;
    break;}
case 318:
#line 1485 "c-parse.y"
{ set_init_label (yyvsp[0].ttype); ;
    break;}
case 319:
#line 1487 "c-parse.y"
{ set_init_index (yyvsp[-3].ttype, yyvsp[-1].ttype);
		  if (pedantic)
		    pedwarn ("ISO C forbids specifying range of elements to initialize"); ;
    break;}
case 320:
#line 1491 "c-parse.y"
{ set_init_index (yyvsp[-1].ttype, NULL_TREE); ;
    break;}
case 321:
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
		;
    break;}
case 322:
#line 1509 "c-parse.y"
{ store_parm_decls (); ;
    break;}
case 323:
#line 1517 "c-parse.y"
{ tree decl = current_function_decl;
		  DECL_SOURCE_FILE (decl) = yyvsp[-2].filename;
		  DECL_SOURCE_LINE (decl) = yyvsp[-1].lineno;
		  finish_function (1, 1);
		  pop_function_context ();
		  add_decl_stmt (decl); ;
    break;}
case 324:
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
		;
    break;}
case 325:
#line 1540 "c-parse.y"
{ store_parm_decls (); ;
    break;}
case 326:
#line 1548 "c-parse.y"
{ tree decl = current_function_decl;
		  DECL_SOURCE_FILE (decl) = yyvsp[-2].filename;
		  DECL_SOURCE_LINE (decl) = yyvsp[-1].lineno;
		  finish_function (1, 1);
		  pop_function_context ();
		  add_decl_stmt (decl); ;
    break;}
case 329:
#line 1568 "c-parse.y"
{ yyval.ttype = yyvsp[-2].ttype ? tree_cons (yyvsp[-2].ttype, yyvsp[-1].ttype, NULL_TREE) : yyvsp[-1].ttype; ;
    break;}
case 330:
#line 1570 "c-parse.y"
{ yyval.ttype = build_nt (CALL_EXPR, yyvsp[-2].ttype, yyvsp[0].ttype, NULL_TREE); ;
    break;}
case 331:
#line 1575 "c-parse.y"
{ yyval.ttype = set_array_declarator_type (yyvsp[0].ttype, yyvsp[-1].ttype, 0); ;
    break;}
case 332:
#line 1577 "c-parse.y"
{ yyval.ttype = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 336:
#line 1592 "c-parse.y"
{ yyval.ttype = build_nt (CALL_EXPR, yyvsp[-2].ttype, yyvsp[0].ttype, NULL_TREE); ;
    break;}
case 337:
#line 1597 "c-parse.y"
{ yyval.ttype = set_array_declarator_type (yyvsp[0].ttype, yyvsp[-1].ttype, 0); ;
    break;}
case 339:
#line 1603 "c-parse.y"
{ yyval.ttype = build_nt (CALL_EXPR, yyvsp[-2].ttype, yyvsp[0].ttype, NULL_TREE); ;
    break;}
case 340:
#line 1608 "c-parse.y"
{ yyval.ttype = set_array_declarator_type (yyvsp[0].ttype, yyvsp[-1].ttype, 0); ;
    break;}
case 341:
#line 1610 "c-parse.y"
{ yyval.ttype = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 342:
#line 1612 "c-parse.y"
{ yyval.ttype = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 343:
#line 1614 "c-parse.y"
{ yyval.ttype = yyvsp[-2].ttype ? tree_cons (yyvsp[-2].ttype, yyvsp[-1].ttype, NULL_TREE) : yyvsp[-1].ttype; ;
    break;}
case 344:
#line 1622 "c-parse.y"
{ yyval.ttype = build_nt (CALL_EXPR, yyvsp[-2].ttype, yyvsp[0].ttype, NULL_TREE); ;
    break;}
case 345:
#line 1627 "c-parse.y"
{ yyval.ttype = yyvsp[-2].ttype ? tree_cons (yyvsp[-2].ttype, yyvsp[-1].ttype, NULL_TREE) : yyvsp[-1].ttype; ;
    break;}
case 346:
#line 1629 "c-parse.y"
{ yyval.ttype = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 347:
#line 1631 "c-parse.y"
{ yyval.ttype = set_array_declarator_type (yyvsp[0].ttype, yyvsp[-1].ttype, 0); ;
    break;}
case 349:
#line 1637 "c-parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 350:
#line 1639 "c-parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 351:
#line 1644 "c-parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 352:
#line 1646 "c-parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 353:
#line 1651 "c-parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 354:
#line 1653 "c-parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 355:
#line 1664 "c-parse.y"
{ yyval.ttype = start_struct (RECORD_TYPE, yyvsp[-1].ttype);
		  /* Start scope of tag before parsing components.  */
		;
    break;}
case 356:
#line 1668 "c-parse.y"
{ yyval.ttype = finish_struct (yyvsp[-3].ttype, yyvsp[-2].ttype, chainon (yyvsp[-6].ttype, yyvsp[0].ttype)); ;
    break;}
case 357:
#line 1670 "c-parse.y"
{ yyval.ttype = finish_struct (start_struct (RECORD_TYPE, NULL_TREE),
				      yyvsp[-2].ttype, chainon (yyvsp[-4].ttype, yyvsp[0].ttype));
		;
    break;}
case 358:
#line 1674 "c-parse.y"
{ yyval.ttype = start_struct (UNION_TYPE, yyvsp[-1].ttype); ;
    break;}
case 359:
#line 1676 "c-parse.y"
{ yyval.ttype = finish_struct (yyvsp[-3].ttype, yyvsp[-2].ttype, chainon (yyvsp[-6].ttype, yyvsp[0].ttype)); ;
    break;}
case 360:
#line 1678 "c-parse.y"
{ yyval.ttype = finish_struct (start_struct (UNION_TYPE, NULL_TREE),
				      yyvsp[-2].ttype, chainon (yyvsp[-4].ttype, yyvsp[0].ttype));
		;
    break;}
case 361:
#line 1682 "c-parse.y"
{ yyval.ttype = start_enum (yyvsp[-1].ttype); ;
    break;}
case 362:
#line 1684 "c-parse.y"
{ yyval.ttype = finish_enum (yyvsp[-4].ttype, nreverse (yyvsp[-3].ttype),
				    chainon (yyvsp[-7].ttype, yyvsp[0].ttype)); ;
    break;}
case 363:
#line 1687 "c-parse.y"
{ yyval.ttype = start_enum (NULL_TREE); ;
    break;}
case 364:
#line 1689 "c-parse.y"
{ yyval.ttype = finish_enum (yyvsp[-4].ttype, nreverse (yyvsp[-3].ttype),
				    chainon (yyvsp[-6].ttype, yyvsp[0].ttype)); ;
    break;}
case 365:
#line 1695 "c-parse.y"
{ yyval.ttype = xref_tag (RECORD_TYPE, yyvsp[0].ttype); ;
    break;}
case 366:
#line 1697 "c-parse.y"
{ yyval.ttype = xref_tag (UNION_TYPE, yyvsp[0].ttype); ;
    break;}
case 367:
#line 1699 "c-parse.y"
{ yyval.ttype = xref_tag (ENUMERAL_TYPE, yyvsp[0].ttype);
		  /* In ISO C, enumerated types can be referred to
		     only if already defined.  */
		  if (pedantic && !COMPLETE_TYPE_P (yyval.ttype))
		    pedwarn ("ISO C forbids forward references to `enum' types"); ;
    break;}
case 371:
#line 1714 "c-parse.y"
{ if (pedantic && ! flag_isoc99)
		    pedwarn ("comma at end of enumerator list"); ;
    break;}
case 372:
#line 1720 "c-parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 373:
#line 1722 "c-parse.y"
{ yyval.ttype = chainon (yyvsp[-1].ttype, yyvsp[0].ttype);
		  pedwarn ("no semicolon at end of struct or union"); ;
    break;}
case 374:
#line 1727 "c-parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 375:
#line 1729 "c-parse.y"
{ yyval.ttype = chainon (yyvsp[-2].ttype, yyvsp[-1].ttype); ;
    break;}
case 376:
#line 1731 "c-parse.y"
{ if (pedantic)
		    pedwarn ("extra semicolon in struct or union specified"); ;
    break;}
case 377:
#line 1737 "c-parse.y"
{ yyval.ttype = yyvsp[0].ttype;
		  POP_DECLSPEC_STACK; ;
    break;}
case 378:
#line 1740 "c-parse.y"
{
		  /* Support for unnamed structs or unions as members of
		     structs or unions (which is [a] useful and [b] supports
		     MS P-SDK).  */
		  if (pedantic)
		    pedwarn ("ISO C doesn't support unnamed structs/unions");

		  yyval.ttype = grokfield(yyvsp[-1].filename, yyvsp[0].lineno, NULL, current_declspecs, NULL_TREE);
		  POP_DECLSPEC_STACK; ;
    break;}
case 379:
#line 1750 "c-parse.y"
{ yyval.ttype = yyvsp[0].ttype;
		  POP_DECLSPEC_STACK; ;
    break;}
case 380:
#line 1753 "c-parse.y"
{ if (pedantic)
		    pedwarn ("ISO C forbids member declarations with no members");
		  shadow_tag(yyvsp[0].ttype);
		  yyval.ttype = NULL_TREE; ;
    break;}
case 381:
#line 1758 "c-parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 382:
#line 1760 "c-parse.y"
{ yyval.ttype = yyvsp[0].ttype;
		  RESTORE_EXT_FLAGS (yyvsp[-1].ttype); ;
    break;}
case 384:
#line 1767 "c-parse.y"
{ yyval.ttype = chainon (yyvsp[-3].ttype, yyvsp[0].ttype); ;
    break;}
case 386:
#line 1773 "c-parse.y"
{ yyval.ttype = chainon (yyvsp[-3].ttype, yyvsp[0].ttype); ;
    break;}
case 387:
#line 1778 "c-parse.y"
{ yyval.ttype = grokfield (yyvsp[-3].filename, yyvsp[-2].lineno, yyvsp[-1].ttype, current_declspecs, NULL_TREE);
		  decl_attributes (&yyval.ttype, chainon (yyvsp[0].ttype, all_prefix_attributes), 0); ;
    break;}
case 388:
#line 1782 "c-parse.y"
{ yyval.ttype = grokfield (yyvsp[-5].filename, yyvsp[-4].lineno, yyvsp[-3].ttype, current_declspecs, yyvsp[-1].ttype);
		  decl_attributes (&yyval.ttype, chainon (yyvsp[0].ttype, all_prefix_attributes), 0); ;
    break;}
case 389:
#line 1785 "c-parse.y"
{ yyval.ttype = grokfield (yyvsp[-4].filename, yyvsp[-3].lineno, NULL_TREE, current_declspecs, yyvsp[-1].ttype);
		  decl_attributes (&yyval.ttype, chainon (yyvsp[0].ttype, all_prefix_attributes), 0); ;
    break;}
case 390:
#line 1791 "c-parse.y"
{ yyval.ttype = grokfield (yyvsp[-3].filename, yyvsp[-2].lineno, yyvsp[-1].ttype, current_declspecs, NULL_TREE);
		  decl_attributes (&yyval.ttype, chainon (yyvsp[0].ttype, all_prefix_attributes), 0); ;
    break;}
case 391:
#line 1795 "c-parse.y"
{ yyval.ttype = grokfield (yyvsp[-5].filename, yyvsp[-4].lineno, yyvsp[-3].ttype, current_declspecs, yyvsp[-1].ttype);
		  decl_attributes (&yyval.ttype, chainon (yyvsp[0].ttype, all_prefix_attributes), 0); ;
    break;}
case 392:
#line 1798 "c-parse.y"
{ yyval.ttype = grokfield (yyvsp[-4].filename, yyvsp[-3].lineno, NULL_TREE, current_declspecs, yyvsp[-1].ttype);
		  decl_attributes (&yyval.ttype, chainon (yyvsp[0].ttype, all_prefix_attributes), 0); ;
    break;}
case 394:
#line 1810 "c-parse.y"
{ if (yyvsp[-2].ttype == error_mark_node)
		    yyval.ttype = yyvsp[-2].ttype;
		  else
		    yyval.ttype = chainon (yyvsp[0].ttype, yyvsp[-2].ttype); ;
    break;}
case 395:
#line 1815 "c-parse.y"
{ yyval.ttype = error_mark_node; ;
    break;}
case 396:
#line 1821 "c-parse.y"
{ yyval.ttype = build_enumerator (yyvsp[0].ttype, NULL_TREE); ;
    break;}
case 397:
#line 1823 "c-parse.y"
{ yyval.ttype = build_enumerator (yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 398:
#line 1828 "c-parse.y"
{ pending_xref_error ();
		  yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 399:
#line 1831 "c-parse.y"
{ yyval.ttype = build_tree_list (yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 400:
#line 1836 "c-parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 402:
#line 1842 "c-parse.y"
{ yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 NULL_TREE),
					all_prefix_attributes); ;
    break;}
case 403:
#line 1846 "c-parse.y"
{ yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 yyvsp[0].ttype),
					all_prefix_attributes); ;
    break;}
case 404:
#line 1850 "c-parse.y"
{ yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 yyvsp[-1].ttype),
					chainon (yyvsp[0].ttype, all_prefix_attributes)); ;
    break;}
case 408:
#line 1863 "c-parse.y"
{ yyval.ttype = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 409:
#line 1868 "c-parse.y"
{ yyval.ttype = make_pointer_declarator (yyvsp[0].ttype, NULL_TREE); ;
    break;}
case 410:
#line 1870 "c-parse.y"
{ yyval.ttype = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 411:
#line 1875 "c-parse.y"
{ yyval.ttype = yyvsp[-2].ttype ? tree_cons (yyvsp[-2].ttype, yyvsp[-1].ttype, NULL_TREE) : yyvsp[-1].ttype; ;
    break;}
case 412:
#line 1877 "c-parse.y"
{ yyval.ttype = build_nt (CALL_EXPR, yyvsp[-2].ttype, yyvsp[0].ttype, NULL_TREE); ;
    break;}
case 413:
#line 1879 "c-parse.y"
{ yyval.ttype = set_array_declarator_type (yyvsp[0].ttype, yyvsp[-1].ttype, 1); ;
    break;}
case 414:
#line 1881 "c-parse.y"
{ yyval.ttype = build_nt (CALL_EXPR, NULL_TREE, yyvsp[0].ttype, NULL_TREE); ;
    break;}
case 415:
#line 1883 "c-parse.y"
{ yyval.ttype = set_array_declarator_type (yyvsp[0].ttype, NULL_TREE, 1); ;
    break;}
case 416:
#line 1890 "c-parse.y"
{ yyval.ttype = build_array_declarator (yyvsp[-1].ttype, yyvsp[-2].ttype, 0, 0); ;
    break;}
case 417:
#line 1892 "c-parse.y"
{ yyval.ttype = build_array_declarator (NULL_TREE, yyvsp[-1].ttype, 0, 0); ;
    break;}
case 418:
#line 1894 "c-parse.y"
{ yyval.ttype = build_array_declarator (NULL_TREE, yyvsp[-2].ttype, 0, 1); ;
    break;}
case 419:
#line 1896 "c-parse.y"
{ yyval.ttype = build_array_declarator (yyvsp[-1].ttype, yyvsp[-2].ttype, 1, 0); ;
    break;}
case 420:
#line 1899 "c-parse.y"
{ yyval.ttype = build_array_declarator (yyvsp[-1].ttype, yyvsp[-3].ttype, 1, 0); ;
    break;}
case 423:
#line 1912 "c-parse.y"
{
		  pedwarn ("deprecated use of label at end of compound statement");
		;
    break;}
case 431:
#line 1929 "c-parse.y"
{ if (pedantic && !flag_isoc99)
		    pedwarn ("ISO C89 forbids mixed declarations and code"); ;
    break;}
case 446:
#line 1959 "c-parse.y"
{ pushlevel (0);
		  clear_last_expr ();
		  add_scope_stmt (/*begin_p=*/1, /*partial_p=*/0);
		;
    break;}
case 447:
#line 1966 "c-parse.y"
{ yyval.ttype = add_scope_stmt (/*begin_p=*/0, /*partial_p=*/0); ;
    break;}
case 448:
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
		;
    break;}
case 449:
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
		    yyval.ttype = NULL_TREE; ;
    break;}
case 451:
#line 2004 "c-parse.y"
{ if (pedantic)
		    pedwarn ("ISO C forbids label declarations"); ;
    break;}
case 454:
#line 2015 "c-parse.y"
{ tree link;
		  for (link = yyvsp[-1].ttype; link; link = TREE_CHAIN (link))
		    {
		      tree label = shadow_label (TREE_VALUE (link));
		      C_DECLARED_LABEL_FLAG (label) = 1;
		      add_decl_stmt (label);
		    }
		;
    break;}
case 455:
#line 2029 "c-parse.y"
{;
    break;}
case 457:
#line 2033 "c-parse.y"
{ compstmt_count++;
                      yyval.ttype = c_begin_compound_stmt (); ;
    break;}
case 458:
#line 2038 "c-parse.y"
{ yyval.ttype = convert (void_type_node, integer_zero_node); ;
    break;}
case 459:
#line 2040 "c-parse.y"
{ yyval.ttype = poplevel (kept_level_p (), 1, 0);
		  SCOPE_STMT_BLOCK (TREE_PURPOSE (yyvsp[0].ttype))
		    = SCOPE_STMT_BLOCK (TREE_VALUE (yyvsp[0].ttype))
		    = yyval.ttype; ;
    break;}
case 462:
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
		;
    break;}
case 463:
#line 2070 "c-parse.y"
{ RECHAIN_STMTS (yyvsp[-1].ttype, COMPOUND_BODY (yyvsp[-1].ttype));
		  last_expr_type = NULL_TREE;
                  yyval.ttype = yyvsp[-1].ttype; ;
    break;}
case 464:
#line 2078 "c-parse.y"
{ c_finish_then (); ;
    break;}
case 466:
#line 2095 "c-parse.y"
{ yyval.ttype = c_begin_if_stmt (); ;
    break;}
case 467:
#line 2097 "c-parse.y"
{ c_expand_start_cond (c_common_truthvalue_conversion (yyvsp[-1].ttype),
				       compstmt_count,yyvsp[-3].ttype);
		  yyval.itype = stmt_count;
		  if_stmt_file = yyvsp[-7].filename;
		  if_stmt_line = yyvsp[-6].lineno; ;
    break;}
case 468:
#line 2109 "c-parse.y"
{ stmt_count++;
		  compstmt_count++;
		  yyval.ttype
		    = add_stmt (build_stmt (DO_STMT, NULL_TREE,
					    NULL_TREE));
		  /* In the event that a parse error prevents
		     parsing the complete do-statement, set the
		     condition now.  Otherwise, we can get crashes at
		     RTL-generation time.  */
		  DO_COND (yyval.ttype) = error_mark_node; ;
    break;}
case 469:
#line 2120 "c-parse.y"
{ yyval.ttype = yyvsp[-2].ttype;
		  RECHAIN_STMTS (yyval.ttype, DO_BODY (yyval.ttype)); ;
    break;}
case 470:
#line 2128 "c-parse.y"
{ if (yychar == YYEMPTY)
		    yychar = YYLEX;
		  yyval.filename = input_filename; ;
    break;}
case 471:
#line 2134 "c-parse.y"
{ if (yychar == YYEMPTY)
		    yychar = YYLEX;
		  yyval.lineno = lineno; ;
    break;}
case 474:
#line 2147 "c-parse.y"
{ if (flag_isoc99)
		    RECHAIN_STMTS (yyvsp[-2].ttype, COMPOUND_BODY (yyvsp[-2].ttype)); ;
    break;}
case 475:
#line 2153 "c-parse.y"
{ if (yyvsp[0].ttype)
		    {
		      STMT_LINENO (yyvsp[0].ttype) = yyvsp[-1].lineno;
		      /* ??? We currently have no way of recording
			 the filename for a statement.  This probably
			 matters little in practice at the moment,
			 but I suspect that problems will occur when
			 doing inlining at the tree level.  */
		    }
		;
    break;}
case 476:
#line 2167 "c-parse.y"
{ if (yyvsp[0].ttype)
		    {
		      STMT_LINENO (yyvsp[0].ttype) = yyvsp[-1].lineno;
		    }
		;
    break;}
case 477:
#line 2176 "c-parse.y"
{ c_expand_start_else ();
		  yyvsp[-1].itype = stmt_count; ;
    break;}
case 478:
#line 2179 "c-parse.y"
{ c_finish_else ();
		  c_expand_end_cond ();
		  if (extra_warnings && stmt_count == yyvsp[-3].itype)
		    warning ("empty body in an else-statement"); ;
    break;}
case 479:
#line 2184 "c-parse.y"
{ c_expand_end_cond ();
		  /* This warning is here instead of in simple_if, because we
		     do not want a warning if an empty if is followed by an
		     else statement.  Increment stmt_count so we don't
		     give a second error if this is a nested `if'.  */
		  if (extra_warnings && stmt_count++ == yyvsp[0].itype)
		    warning_with_file_and_line (if_stmt_file, if_stmt_line,
						"empty body in an if-statement"); ;
    break;}
case 480:
#line 2196 "c-parse.y"
{ c_expand_end_cond (); ;
    break;}
case 481:
#line 2206 "c-parse.y"
{ stmt_count++;
		  yyval.ttype = c_begin_while_stmt (); ;
    break;}
case 482:
#line 2209 "c-parse.y"
{ yyvsp[-1].ttype = c_common_truthvalue_conversion (yyvsp[-1].ttype);
		  c_finish_while_stmt_cond
		    (c_common_truthvalue_conversion (yyvsp[-1].ttype), yyvsp[-3].ttype);
		  yyval.ttype = add_stmt (yyvsp[-3].ttype); ;
    break;}
case 483:
#line 2214 "c-parse.y"
{ RECHAIN_STMTS (yyvsp[-1].ttype, WHILE_BODY (yyvsp[-1].ttype)); ;
    break;}
case 484:
#line 2217 "c-parse.y"
{ DO_COND (yyvsp[-4].ttype) = c_common_truthvalue_conversion (yyvsp[-2].ttype); ;
    break;}
case 485:
#line 2219 "c-parse.y"
{ ;
    break;}
case 486:
#line 2221 "c-parse.y"
{ yyval.ttype = build_stmt (FOR_STMT, NULL_TREE, NULL_TREE,
					  NULL_TREE, NULL_TREE);
		  add_stmt (yyval.ttype); ;
    break;}
case 487:
#line 2225 "c-parse.y"
{ stmt_count++;
		  RECHAIN_STMTS (yyvsp[-2].ttype, FOR_INIT_STMT (yyvsp[-2].ttype)); ;
    break;}
case 488:
#line 2228 "c-parse.y"
{ if (yyvsp[-1].ttype)
		    FOR_COND (yyvsp[-5].ttype)
		      = c_common_truthvalue_conversion (yyvsp[-1].ttype); ;
    break;}
case 489:
#line 2232 "c-parse.y"
{ FOR_EXPR (yyvsp[-8].ttype) = yyvsp[-1].ttype; ;
    break;}
case 490:
#line 2234 "c-parse.y"
{ RECHAIN_STMTS (yyvsp[-10].ttype, FOR_BODY (yyvsp[-10].ttype)); ;
    break;}
case 491:
#line 2236 "c-parse.y"
{ stmt_count++;
		  yyval.ttype = c_start_case (yyvsp[-1].ttype); ;
    break;}
case 492:
#line 2239 "c-parse.y"
{ c_finish_case (); ;
    break;}
case 493:
#line 2244 "c-parse.y"
{ add_stmt (build_stmt (EXPR_STMT, yyvsp[-1].ttype)); ;
    break;}
case 494:
#line 2246 "c-parse.y"
{ check_for_loop_decls (); ;
    break;}
case 495:
#line 2252 "c-parse.y"
{ stmt_count++; yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 496:
#line 2254 "c-parse.y"
{ stmt_count++;
		  yyval.ttype = c_expand_expr_stmt (yyvsp[-1].ttype); ;
    break;}
case 497:
#line 2257 "c-parse.y"
{ if (flag_isoc99)
		    RECHAIN_STMTS (yyvsp[-2].ttype, COMPOUND_BODY (yyvsp[-2].ttype));
		  yyval.ttype = NULL_TREE; ;
    break;}
case 498:
#line 2261 "c-parse.y"
{ stmt_count++;
		  yyval.ttype = add_stmt (build_break_stmt ()); ;
    break;}
case 499:
#line 2264 "c-parse.y"
{ stmt_count++;
		  yyval.ttype = add_stmt (build_continue_stmt ()); ;
    break;}
case 500:
#line 2267 "c-parse.y"
{ stmt_count++;
		  yyval.ttype = c_expand_return (NULL_TREE); ;
    break;}
case 501:
#line 2270 "c-parse.y"
{ stmt_count++;
		  yyval.ttype = c_expand_return (yyvsp[-1].ttype); ;
    break;}
case 502:
#line 2273 "c-parse.y"
{ stmt_count++;
		  yyval.ttype = simple_asm_stmt (yyvsp[-2].ttype); ;
    break;}
case 503:
#line 2277 "c-parse.y"
{ stmt_count++;
		  yyval.ttype = build_asm_stmt (yyvsp[-6].ttype, yyvsp[-4].ttype, yyvsp[-2].ttype, NULL_TREE, NULL_TREE); ;
    break;}
case 504:
#line 2282 "c-parse.y"
{ stmt_count++;
		  yyval.ttype = build_asm_stmt (yyvsp[-8].ttype, yyvsp[-6].ttype, yyvsp[-4].ttype, yyvsp[-2].ttype, NULL_TREE); ;
    break;}
case 505:
#line 2287 "c-parse.y"
{ stmt_count++;
		  yyval.ttype = build_asm_stmt (yyvsp[-10].ttype, yyvsp[-8].ttype, yyvsp[-6].ttype, yyvsp[-4].ttype, yyvsp[-2].ttype); ;
    break;}
case 506:
#line 2290 "c-parse.y"
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
		;
    break;}
case 507:
#line 2302 "c-parse.y"
{ if (pedantic)
		    pedwarn ("ISO C forbids `goto *expr;'");
		  stmt_count++;
		  yyvsp[-1].ttype = convert (ptr_type_node, yyvsp[-1].ttype);
		  yyval.ttype = add_stmt (build_stmt (GOTO_STMT, yyvsp[-1].ttype)); ;
    break;}
case 508:
#line 2308 "c-parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 509:
#line 2316 "c-parse.y"
{ stmt_count++;
		  yyval.ttype = do_case (yyvsp[-1].ttype, NULL_TREE); ;
    break;}
case 510:
#line 2319 "c-parse.y"
{ stmt_count++;
		  yyval.ttype = do_case (yyvsp[-3].ttype, yyvsp[-1].ttype); ;
    break;}
case 511:
#line 2322 "c-parse.y"
{ stmt_count++;
		  yyval.ttype = do_case (NULL_TREE, NULL_TREE); ;
    break;}
case 512:
#line 2325 "c-parse.y"
{ tree label = define_label (yyvsp[-3].filename, yyvsp[-2].lineno, yyvsp[-4].ttype);
		  stmt_count++;
		  if (label)
		    {
		      decl_attributes (&label, yyvsp[0].ttype, 0);
		      yyval.ttype = add_stmt (build_stmt (LABEL_STMT, label));
		    }
		  else
		    yyval.ttype = NULL_TREE;
		;
    break;}
case 513:
#line 2341 "c-parse.y"
{ emit_line_note (input_filename, lineno);
		  yyval.ttype = NULL_TREE; ;
    break;}
case 514:
#line 2344 "c-parse.y"
{ emit_line_note (input_filename, lineno); ;
    break;}
case 515:
#line 2349 "c-parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 517:
#line 2356 "c-parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 520:
#line 2363 "c-parse.y"
{ yyval.ttype = chainon (yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 521:
#line 2368 "c-parse.y"
{ yyval.ttype = build_tree_list (build_tree_list (NULL_TREE, yyvsp[-3].ttype), yyvsp[-1].ttype); ;
    break;}
case 522:
#line 2370 "c-parse.y"
{ yyvsp[-5].ttype = build_string (IDENTIFIER_LENGTH (yyvsp[-5].ttype),
				     IDENTIFIER_POINTER (yyvsp[-5].ttype));
		  yyval.ttype = build_tree_list (build_tree_list (yyvsp[-5].ttype, yyvsp[-3].ttype), yyvsp[-1].ttype); ;
    break;}
case 523:
#line 2377 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, NULL_TREE); ;
    break;}
case 524:
#line 2379 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-2].ttype); ;
    break;}
case 525:
#line 2389 "c-parse.y"
{ pushlevel (0);
		  clear_parm_order ();
		  declare_parm_level (0); ;
    break;}
case 526:
#line 2393 "c-parse.y"
{ yyval.ttype = yyvsp[0].ttype;
		  parmlist_tags_warning ();
		  poplevel (0, 0, 0); ;
    break;}
case 528:
#line 2401 "c-parse.y"
{ tree parm;
		  if (pedantic)
		    pedwarn ("ISO C forbids forward parameter declarations");
		  /* Mark the forward decls as such.  */
		  for (parm = getdecls (); parm; parm = TREE_CHAIN (parm))
		    TREE_ASM_WRITTEN (parm) = 1;
		  clear_parm_order (); ;
    break;}
case 529:
#line 2409 "c-parse.y"
{ /* Dummy action so attributes are in known place
		     on parser stack.  */ ;
    break;}
case 530:
#line 2412 "c-parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 531:
#line 2414 "c-parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, NULL_TREE, NULL_TREE); ;
    break;}
case 532:
#line 2420 "c-parse.y"
{ yyval.ttype = get_parm_info (0); ;
    break;}
case 533:
#line 2422 "c-parse.y"
{ yyval.ttype = get_parm_info (0);
		  /* Gcc used to allow this as an extension.  However, it does
		     not work for all targets, and thus has been disabled.
		     Also, since func (...) and func () are indistinguishable,
		     it caused problems with the code in expand_builtin which
		     tries to verify that BUILT_IN_NEXT_ARG is being used
		     correctly.  */
		  error ("ISO C requires a named argument before `...'");
		;
    break;}
case 534:
#line 2432 "c-parse.y"
{ yyval.ttype = get_parm_info (1);
		  parsing_iso_function_signature = true;
		;
    break;}
case 535:
#line 2436 "c-parse.y"
{ yyval.ttype = get_parm_info (0); ;
    break;}
case 536:
#line 2441 "c-parse.y"
{ push_parm_decl (yyvsp[0].ttype); ;
    break;}
case 537:
#line 2443 "c-parse.y"
{ push_parm_decl (yyvsp[0].ttype); ;
    break;}
case 538:
#line 2450 "c-parse.y"
{ yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 yyvsp[-1].ttype),
					chainon (yyvsp[0].ttype, all_prefix_attributes));
		  POP_DECLSPEC_STACK; ;
    break;}
case 539:
#line 2455 "c-parse.y"
{ yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 yyvsp[-1].ttype),
					chainon (yyvsp[0].ttype, all_prefix_attributes));
		  POP_DECLSPEC_STACK; ;
    break;}
case 540:
#line 2460 "c-parse.y"
{ yyval.ttype = yyvsp[0].ttype;
		  POP_DECLSPEC_STACK; ;
    break;}
case 541:
#line 2463 "c-parse.y"
{ yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 yyvsp[-1].ttype),
					chainon (yyvsp[0].ttype, all_prefix_attributes));
		  POP_DECLSPEC_STACK; ;
    break;}
case 542:
#line 2469 "c-parse.y"
{ yyval.ttype = yyvsp[0].ttype;
		  POP_DECLSPEC_STACK; ;
    break;}
case 543:
#line 2477 "c-parse.y"
{ yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 yyvsp[-1].ttype),
					chainon (yyvsp[0].ttype, all_prefix_attributes));
		  POP_DECLSPEC_STACK; ;
    break;}
case 544:
#line 2482 "c-parse.y"
{ yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 yyvsp[-1].ttype),
					chainon (yyvsp[0].ttype, all_prefix_attributes));
		  POP_DECLSPEC_STACK; ;
    break;}
case 545:
#line 2487 "c-parse.y"
{ yyval.ttype = yyvsp[0].ttype;
		  POP_DECLSPEC_STACK; ;
    break;}
case 546:
#line 2490 "c-parse.y"
{ yyval.ttype = build_tree_list (build_tree_list (current_declspecs,
							 yyvsp[-1].ttype),
					chainon (yyvsp[0].ttype, all_prefix_attributes));
		  POP_DECLSPEC_STACK; ;
    break;}
case 547:
#line 2496 "c-parse.y"
{ yyval.ttype = yyvsp[0].ttype;
		  POP_DECLSPEC_STACK; ;
    break;}
case 548:
#line 2502 "c-parse.y"
{ prefix_attributes = chainon (prefix_attributes, yyvsp[-3].ttype);
		  all_prefix_attributes = prefix_attributes; ;
    break;}
case 549:
#line 2511 "c-parse.y"
{ pushlevel (0);
		  clear_parm_order ();
		  declare_parm_level (1); ;
    break;}
case 550:
#line 2515 "c-parse.y"
{ yyval.ttype = yyvsp[0].ttype;
		  parmlist_tags_warning ();
		  poplevel (0, 0, 0); ;
    break;}
case 552:
#line 2523 "c-parse.y"
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
		;
    break;}
case 553:
#line 2541 "c-parse.y"
{ yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ttype); ;
    break;}
case 554:
#line 2543 "c-parse.y"
{ yyval.ttype = chainon (yyvsp[-2].ttype, build_tree_list (NULL_TREE, yyvsp[0].ttype)); ;
    break;}
case 555:
#line 2549 "c-parse.y"
{ yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ttype); ;
    break;}
case 556:
#line 2551 "c-parse.y"
{ yyval.ttype = chainon (yyvsp[-2].ttype, build_tree_list (NULL_TREE, yyvsp[0].ttype)); ;
    break;}
case 557:
#line 2556 "c-parse.y"
{ yyval.ttype = SAVE_EXT_FLAGS();
		  pedantic = 0;
		  warn_pointer_arith = 0;
		  warn_traditional = 0;
		  flag_iso = 0; ;
    break;}
}

#line 705 "/usr/share/bison/bison.simple"


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
#line 2563 "c-parse.y"


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
