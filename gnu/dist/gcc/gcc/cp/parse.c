/* A Bison parser, made from parse.y
   by GNU bison 1.35.  */

#define YYBISON 1  /* Identify Bison output.  */

# define	IDENTIFIER	257
# define	tTYPENAME	258
# define	SELFNAME	259
# define	PFUNCNAME	260
# define	SCSPEC	261
# define	TYPESPEC	262
# define	CV_QUALIFIER	263
# define	CONSTANT	264
# define	VAR_FUNC_NAME	265
# define	STRING	266
# define	ELLIPSIS	267
# define	SIZEOF	268
# define	ENUM	269
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
# define	RETURN_KEYWORD	280
# define	GOTO	281
# define	ASM_KEYWORD	282
# define	TYPEOF	283
# define	ALIGNOF	284
# define	SIGOF	285
# define	ATTRIBUTE	286
# define	EXTENSION	287
# define	LABEL	288
# define	REALPART	289
# define	IMAGPART	290
# define	VA_ARG	291
# define	AGGR	292
# define	VISSPEC	293
# define	DELETE	294
# define	NEW	295
# define	THIS	296
# define	OPERATOR	297
# define	CXX_TRUE	298
# define	CXX_FALSE	299
# define	NAMESPACE	300
# define	TYPENAME_KEYWORD	301
# define	USING	302
# define	LEFT_RIGHT	303
# define	TEMPLATE	304
# define	TYPEID	305
# define	DYNAMIC_CAST	306
# define	STATIC_CAST	307
# define	REINTERPRET_CAST	308
# define	CONST_CAST	309
# define	SCOPE	310
# define	EXPORT	311
# define	EMPTY	312
# define	PTYPENAME	313
# define	NSNAME	314
# define	THROW	315
# define	ASSIGN	316
# define	OROR	317
# define	ANDAND	318
# define	MIN_MAX	319
# define	EQCOMPARE	320
# define	ARITHCOMPARE	321
# define	LSHIFT	322
# define	RSHIFT	323
# define	POINTSAT_STAR	324
# define	DOT_STAR	325
# define	UNARY	326
# define	PLUSPLUS	327
# define	MINUSMINUS	328
# define	HYPERUNARY	329
# define	POINTSAT	330
# define	TRY	331
# define	CATCH	332
# define	EXTERN_LANG_STRING	333
# define	ALL	334
# define	PRE_PARSED_CLASS_DECL	335
# define	DEFARG	336
# define	DEFARG_MARKER	337
# define	PRE_PARSED_FUNCTION_DECL	338
# define	TYPENAME_DEFN	339
# define	IDENTIFIER_DEFN	340
# define	PTYPENAME_DEFN	341
# define	END_OF_LINE	342
# define	END_OF_SAVED_INPUT	343

#line 30 "parse.y"

#include "config.h"

#include "system.h"

#include "tree.h"
#include "input.h"
#include "flags.h"
#include "cp-tree.h"
#include "decl.h"
#include "lex.h"
#include "c-pragma.h"		/* For YYDEBUG definition.  */
#include "output.h"
#include "except.h"
#include "toplev.h"
#include "ggc.h"

/* Like YYERROR but do call yyerror.  */
#define YYERROR1 { yyerror ("syntax error"); YYERROR; }

/* Like the default stack expander, except (1) use realloc when possible,
   (2) impose no hard maxiumum on stack size, (3) REALLY do not use alloca.

   Irritatingly, YYSTYPE is defined after this %{ %} block, so we cannot
   give malloced_yyvs its proper type.  This is ok since all we need from
   it is to be able to free it.  */

static short *malloced_yyss;
static void *malloced_yyvs;
static int class_template_ok_as_expr;

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
#define OP0(NODE) (TREE_OPERAND (NODE, 0))
#define OP1(NODE) (TREE_OPERAND (NODE, 1))

/* Contains the statement keyword (if/while/do) to include in an
   error message if the user supplies an empty conditional expression.  */
static const char *cond_stmt_keyword;

/* List of types and structure classes of the current declaration.  */
static GTY(()) tree current_declspecs;

/* List of prefix attributes in effect.
   Prefix attributes are parsed by the reserved_declspecs and declmods
   rules.  They create a list that contains *both* declspecs and attrs.  */
/* ??? It is not clear yet that all cases where an attribute can now appear in
   a declspec list have been updated.  */
static GTY(()) tree prefix_attributes;

/* When defining an enumeration, this is the type of the enumeration.  */
static GTY(()) tree current_enum_type;

/* When parsing a conversion operator name, this is the scope of the
   operator itself.  */
static GTY(()) tree saved_scopes;

static tree empty_parms PARAMS ((void));
static tree parse_decl0 PARAMS ((tree, tree, tree, tree, int));
static tree parse_decl PARAMS ((tree, tree, int));
static void parse_end_decl PARAMS ((tree, tree, tree));
static tree parse_field0 PARAMS ((tree, tree, tree, tree, tree, tree));
static tree parse_field PARAMS ((tree, tree, tree, tree));
static tree parse_bitfield0 PARAMS ((tree, tree, tree, tree, tree));
static tree parse_bitfield PARAMS ((tree, tree, tree));
static tree parse_method PARAMS ((tree, tree, tree));
static void frob_specs PARAMS ((tree, tree));
static void check_class_key PARAMS ((tree, tree));
static tree parse_scoped_id PARAMS ((tree));
static tree parse_xref_tag (tree, tree, int);
static tree parse_handle_class_head (tree, tree, tree, int, int *);
static void parse_decl_instantiation (tree, tree, tree);
static int parse_begin_function_definition (tree, tree);
static tree parse_finish_call_expr (tree, tree, int);

/* Cons up an empty parameter list.  */
static inline tree
empty_parms ()
{
  tree parms;

#ifndef NO_IMPLICIT_EXTERN_C
  if (in_system_header && current_class_type == NULL
      && current_lang_name == lang_name_c)
    parms = NULL_TREE;
  else
#endif
  parms = void_list_node;
  return parms;
}

/* Record the decl-specifiers, attributes and type lookups from the
   decl-specifier-seq in a declaration.  */

static void
frob_specs (specs_attrs, lookups)
     tree specs_attrs, lookups;
{
  save_type_access_control (lookups);
  split_specs_attrs (specs_attrs, &current_declspecs, &prefix_attributes);
  if (current_declspecs
      && TREE_CODE (current_declspecs) != TREE_LIST)
    current_declspecs = build_tree_list (NULL_TREE, current_declspecs);
  if (have_extern_spec)
    {
      /* We have to indicate that there is an "extern", but that it
         was part of a language specifier.  For instance,

    	    extern "C" typedef int (*Ptr) ();

         is well formed.  */
      current_declspecs = tree_cons (error_mark_node,
				     get_identifier ("extern"),
				     current_declspecs);
      have_extern_spec = false;
    }
}

static tree
parse_decl (declarator, attributes, initialized)
     tree declarator, attributes;
     int initialized;
{
  return start_decl (declarator, current_declspecs, initialized,
		     attributes, prefix_attributes);
}

static tree
parse_decl0 (declarator, specs_attrs, lookups, attributes, initialized)
     tree declarator, specs_attrs, lookups, attributes;
     int initialized;
{
  frob_specs (specs_attrs, lookups);
  return parse_decl (declarator, attributes, initialized);
}

static void
parse_end_decl (decl, init, asmspec)
     tree decl, init, asmspec;
{
  /* If decl is NULL_TREE, then this was a variable declaration using
     () syntax for the initializer, so we handled it in grokdeclarator.  */
  if (decl)
    decl_type_access_control (decl);
  cp_finish_decl (decl, init, asmspec, init ? LOOKUP_ONLYCONVERTING : 0);
}

static tree
parse_field (declarator, attributes, asmspec, init)
     tree declarator, attributes, asmspec, init;
{
  tree d = grokfield (declarator, current_declspecs, init, asmspec,
		      chainon (attributes, prefix_attributes));
  decl_type_access_control (d);
  return d;
}

static tree
parse_field0 (declarator, specs_attrs, lookups, attributes, asmspec, init)
     tree declarator, specs_attrs, lookups, attributes, asmspec, init;
{
  frob_specs (specs_attrs, lookups);
  return parse_field (declarator, attributes, asmspec, init);
}

static tree
parse_bitfield (declarator, attributes, width)
     tree declarator, attributes, width;
{
  tree d = grokbitfield (declarator, current_declspecs, width);
  cplus_decl_attributes (&d, chainon (attributes, prefix_attributes), 0);
  decl_type_access_control (d);
  return d;
}

static tree
parse_bitfield0 (declarator, specs_attrs, lookups, attributes, width)
     tree declarator, specs_attrs, lookups, attributes, width;
{
  frob_specs (specs_attrs, lookups);
  return parse_bitfield (declarator, attributes, width);
}

static tree
parse_method (declarator, specs_attrs, lookups)
     tree declarator, specs_attrs, lookups;
{
  tree d;
  frob_specs (specs_attrs, lookups);
  d = start_method (current_declspecs, declarator, prefix_attributes);
  decl_type_access_control (d);
  return d;
}

static void
check_class_key (key, aggr)
     tree key;
     tree aggr;
{
  if (TREE_CODE (key) == TREE_LIST)
    key = TREE_VALUE (key);
  if ((key == union_type_node) != (TREE_CODE (aggr) == UNION_TYPE))
    pedwarn ("`%s' tag used in naming `%#T'",
	     key == union_type_node ? "union"
	     : key == record_type_node ? "struct" : "class", aggr);
}


#line 271 "parse.y"
#ifndef YYSTYPE
typedef union { GTY(())
  long itype;
  tree ttype;
  char *strtype;
  enum tree_code code;
  flagged_type_tree ftype;
  struct unparsed_text *pi;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
#line 478 "parse.y"

/* Tell yyparse how to print a token's value, if yydebug is set.  */
#define YYPRINT(FILE,YYCHAR,YYLVAL) yyprint(FILE,YYCHAR,YYLVAL)
extern void yyprint			PARAMS ((FILE *, int, YYSTYPE));
#ifndef YYDEBUG
# define YYDEBUG 0
#endif



#define	YYFINAL		1823
#define	YYFLAG		-32768
#define	YYNTBASE	114

/* YYTRANSLATE(YYLEX) -- Bison token number corresponding to YYLEX. */
#define YYTRANSLATE(x) ((unsigned)(x) <= 343 ? yytranslate[x] : 410)

/* YYTRANSLATE[YYLEX] -- Bison token number corresponding to YYLEX. */
static const char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   112,     2,     2,     2,    85,    73,     2,
      95,   110,    83,    81,    62,    82,    94,    84,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    65,    63,
      77,    67,    78,    68,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    96,     2,   113,    72,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    61,    71,   111,    91,     2,     2,     2,
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
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    64,    66,    69,    70,    74,
      75,    76,    79,    80,    86,    87,    88,    89,    90,    92,
      93,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109
};

#if YYDEBUG
static const short yyprhs[] =
{
       0,     0,     1,     3,     4,     7,    10,    12,    13,    14,
      15,    17,    19,    20,    23,    26,    28,    29,    33,    35,
      41,    46,    52,    57,    58,    65,    66,    72,    74,    77,
      79,    82,    83,    90,    93,    97,   101,   105,   109,   114,
     115,   121,   124,   128,   130,   132,   135,   138,   140,   143,
     144,   150,   154,   156,   158,   160,   164,   166,   167,   170,
     173,   177,   179,   183,   185,   189,   191,   195,   198,   201,
     204,   206,   208,   214,   219,   222,   225,   229,   233,   236,
     239,   243,   247,   250,   253,   256,   259,   262,   265,   267,
     269,   271,   273,   274,   276,   279,   280,   282,   283,   290,
     294,   298,   302,   303,   312,   318,   319,   329,   336,   337,
     346,   352,   353,   363,   370,   373,   376,   378,   381,   383,
     390,   399,   404,   411,   418,   423,   426,   428,   431,   434,
     436,   439,   441,   444,   447,   452,   455,   456,   460,   461,
     462,   464,   468,   471,   472,   474,   476,   478,   483,   486,
     488,   490,   492,   494,   496,   498,   500,   502,   504,   506,
     508,   510,   511,   518,   519,   526,   527,   533,   534,   540,
     541,   549,   550,   558,   559,   566,   567,   574,   575,   576,
     582,   588,   590,   592,   598,   604,   605,   607,   609,   610,
     612,   614,   618,   619,   622,   624,   626,   629,   631,   635,
     637,   639,   641,   643,   645,   647,   649,   651,   655,   657,
     661,   662,   664,   666,   667,   675,   677,   679,   683,   688,
     692,   696,   700,   704,   708,   710,   712,   714,   717,   720,
     723,   726,   729,   732,   735,   740,   743,   748,   751,   755,
     759,   764,   769,   775,   781,   788,   791,   796,   802,   805,
     808,   812,   816,   820,   822,   826,   829,   833,   838,   840,
     843,   849,   851,   855,   859,   863,   867,   871,   875,   879,
     883,   887,   891,   895,   899,   903,   907,   911,   915,   919,
     923,   927,   933,   937,   941,   943,   946,   948,   952,   956,
     960,   964,   968,   972,   976,   980,   984,   988,   992,   996,
    1000,  1004,  1008,  1012,  1016,  1020,  1026,  1030,  1034,  1036,
    1039,  1043,  1047,  1049,  1051,  1053,  1055,  1057,  1058,  1064,
    1070,  1076,  1082,  1088,  1090,  1092,  1094,  1096,  1099,  1101,
    1104,  1107,  1111,  1116,  1121,  1123,  1125,  1127,  1131,  1133,
    1135,  1137,  1139,  1141,  1145,  1149,  1153,  1154,  1159,  1164,
    1167,  1172,  1175,  1182,  1187,  1190,  1193,  1195,  1200,  1202,
    1210,  1218,  1226,  1234,  1239,  1244,  1247,  1250,  1253,  1255,
    1260,  1263,  1266,  1272,  1276,  1279,  1282,  1288,  1292,  1298,
    1302,  1307,  1314,  1317,  1319,  1322,  1324,  1327,  1329,  1331,
    1332,  1335,  1338,  1342,  1346,  1350,  1353,  1356,  1359,  1361,
    1363,  1365,  1368,  1371,  1374,  1377,  1379,  1381,  1383,  1385,
    1388,  1391,  1395,  1399,  1403,  1408,  1410,  1413,  1416,  1418,
    1420,  1423,  1426,  1429,  1431,  1434,  1437,  1441,  1443,  1446,
    1449,  1451,  1453,  1455,  1457,  1459,  1461,  1463,  1468,  1473,
    1478,  1483,  1485,  1487,  1489,  1491,  1495,  1497,  1501,  1503,
    1507,  1508,  1513,  1514,  1521,  1525,  1526,  1531,  1533,  1537,
    1541,  1542,  1547,  1551,  1552,  1554,  1556,  1559,  1566,  1568,
    1572,  1573,  1575,  1580,  1587,  1592,  1594,  1596,  1598,  1600,
    1602,  1606,  1607,  1610,  1612,  1615,  1619,  1624,  1626,  1628,
    1632,  1637,  1641,  1647,  1651,  1655,  1659,  1660,  1664,  1668,
    1672,  1673,  1676,  1679,  1680,  1687,  1688,  1694,  1697,  1700,
    1703,  1704,  1705,  1706,  1718,  1720,  1721,  1723,  1724,  1726,
    1728,  1731,  1734,  1737,  1740,  1743,  1746,  1750,  1755,  1759,
    1762,  1766,  1771,  1773,  1776,  1778,  1781,  1784,  1787,  1790,
    1794,  1798,  1801,  1802,  1805,  1809,  1811,  1816,  1818,  1822,
    1824,  1826,  1829,  1832,  1836,  1840,  1841,  1843,  1847,  1850,
    1853,  1855,  1858,  1861,  1864,  1867,  1870,  1873,  1876,  1878,
    1881,  1884,  1888,  1890,  1893,  1896,  1901,  1906,  1909,  1911,
    1917,  1922,  1924,  1925,  1927,  1931,  1932,  1934,  1938,  1940,
    1942,  1944,  1946,  1951,  1956,  1961,  1966,  1971,  1975,  1980,
    1985,  1990,  1995,  1999,  2002,  2004,  2006,  2010,  2012,  2016,
    2019,  2021,  2028,  2029,  2032,  2034,  2037,  2039,  2042,  2046,
    2050,  2052,  2056,  2058,  2061,  2065,  2069,  2072,  2075,  2079,
    2081,  2086,  2091,  2095,  2099,  2102,  2104,  2106,  2109,  2111,
    2113,  2116,  2119,  2121,  2124,  2128,  2132,  2135,  2138,  2142,
    2144,  2148,  2152,  2155,  2158,  2162,  2164,  2169,  2173,  2178,
    2182,  2184,  2187,  2190,  2193,  2196,  2199,  2202,  2205,  2207,
    2210,  2215,  2220,  2223,  2225,  2227,  2229,  2231,  2234,  2239,
    2243,  2247,  2250,  2253,  2256,  2259,  2261,  2264,  2267,  2270,
    2273,  2277,  2279,  2282,  2286,  2291,  2294,  2297,  2300,  2303,
    2306,  2309,  2314,  2317,  2319,  2322,  2325,  2329,  2331,  2335,
    2338,  2342,  2345,  2348,  2352,  2354,  2358,  2363,  2365,  2368,
    2372,  2375,  2378,  2380,  2384,  2387,  2390,  2392,  2395,  2399,
    2401,  2405,  2412,  2417,  2422,  2426,  2432,  2436,  2440,  2444,
    2447,  2449,  2451,  2454,  2457,  2460,  2461,  2463,  2465,  2468,
    2472,  2473,  2478,  2480,  2481,  2482,  2488,  2490,  2491,  2495,
    2497,  2500,  2502,  2505,  2506,  2511,  2513,  2514,  2515,  2521,
    2522,  2523,  2531,  2532,  2533,  2534,  2535,  2548,  2549,  2550,
    2558,  2559,  2565,  2566,  2574,  2575,  2580,  2583,  2586,  2589,
    2593,  2600,  2609,  2620,  2629,  2642,  2653,  2664,  2669,  2673,
    2676,  2679,  2681,  2683,  2685,  2687,  2689,  2690,  2691,  2697,
    2698,  2699,  2705,  2707,  2710,  2711,  2712,  2713,  2719,  2721,
    2723,  2727,  2731,  2734,  2737,  2740,  2743,  2746,  2748,  2751,
    2752,  2754,  2755,  2757,  2759,  2760,  2762,  2764,  2768,  2773,
    2781,  2783,  2787,  2788,  2790,  2792,  2794,  2797,  2800,  2803,
    2805,  2808,  2811,  2812,  2816,  2818,  2820,  2822,  2825,  2828,
    2831,  2836,  2839,  2842,  2845,  2848,  2851,  2854,  2856,  2859,
    2861,  2864,  2866,  2868,  2869,  2870,  2872,  2878,  2882,  2883,
    2887,  2888,  2889,  2894,  2897,  2899,  2901,  2903,  2907,  2908,
    2912,  2916,  2920,  2922,  2923,  2927,  2931,  2935,  2939,  2943,
    2947,  2951,  2955,  2959,  2963,  2967,  2971,  2975,  2979,  2983,
    2987,  2991,  2995,  2999,  3003,  3007,  3011,  3015,  3020,  3024,
    3028,  3032,  3036,  3041,  3045,  3049,  3055,  3061,  3066,  3070
};
static const short yyrhs[] =
{
      -1,   115,     0,     0,   116,   122,     0,   115,   122,     0,
     115,     0,     0,     0,     0,    33,     0,    28,     0,     0,
     123,   124,     0,   155,   152,     0,   149,     0,     0,    57,
     125,   146,     0,   146,     0,   121,    95,    12,   110,    63,
       0,   136,    61,   117,   111,     0,   136,   118,   155,   119,
     152,     0,   136,   118,   149,   119,     0,     0,    46,   172,
      61,   126,   117,   111,     0,     0,    46,    61,   127,   117,
     111,     0,   128,     0,   130,    63,     0,   132,     0,   120,
     124,     0,     0,    46,   172,    67,   129,   135,    63,     0,
      48,   318,     0,    48,   332,   318,     0,    48,   332,   217,
       0,    48,   134,   172,     0,    48,   332,   172,     0,    48,
     332,   134,   172,     0,     0,    48,    46,   133,   135,    63,
       0,    60,    56,     0,   134,    60,    56,     0,   217,     0,
     318,     0,   332,   318,     0,   332,   217,     0,    99,     0,
     136,    99,     0,     0,    50,    77,   138,   141,    78,     0,
      50,    77,    78,     0,   137,     0,   139,     0,   145,     0,
     141,    62,   145,     0,   172,     0,     0,   278,   142,     0,
      47,   142,     0,   137,   278,   142,     0,   143,     0,   143,
      67,   232,     0,   396,     0,   396,    67,   212,     0,   144,
       0,   144,    67,   193,     0,   140,   147,     0,   140,     1,
       0,   155,   152,     0,   148,     0,   146,     0,   136,   118,
     155,   119,   152,     0,   136,   118,   148,   119,     0,   120,
     147,     0,   246,    63,     0,   236,   245,    63,     0,   233,
     244,    63,     0,   270,    63,     0,   246,    63,     0,   236,
     245,    63,     0,   233,   244,    63,     0,   236,    63,     0,
     175,    63,     0,   233,    63,     0,     1,    63,     0,     1,
     111,     0,     1,   109,     0,    63,     0,   399,     0,   227,
       0,   166,     0,     0,   165,     0,   165,    63,     0,     0,
     109,     0,     0,   168,   150,   409,    61,   154,   203,     0,
     161,   151,   153,     0,   161,   151,   367,     0,   161,   151,
       1,     0,     0,   323,     5,    95,   157,   387,   110,   305,
     402,     0,   323,     5,    49,   305,   402,     0,     0,   332,
     323,     5,    95,   158,   387,   110,   305,   402,     0,   332,
     323,     5,    49,   305,   402,     0,     0,   323,   188,    95,
     159,   387,   110,   305,   402,     0,   323,   188,    49,   305,
     402,     0,     0,   332,   323,   188,    95,   160,   387,   110,
     305,   402,     0,   332,   323,   188,    49,   305,   402,     0,
     233,   230,     0,   236,   315,     0,   315,     0,   236,   156,
       0,   156,     0,     5,    95,   387,   110,   305,   402,     0,
      95,     5,   110,    95,   387,   110,   305,   402,     0,     5,
      49,   305,   402,     0,    95,     5,   110,    49,   305,   402,
       0,   188,    95,   387,   110,   305,   402,     0,   188,    49,
     305,   402,     0,   236,   162,     0,   162,     0,   233,   230,
       0,   236,   315,     0,   315,     0,   236,   156,     0,   156,
       0,    26,     3,     0,   164,   263,     0,   164,    95,   205,
     110,     0,   164,    49,     0,     0,    65,   167,   169,     0,
       0,     0,   171,     0,   169,    62,   171,     0,   169,     1,
       0,     0,   173,     0,   311,     0,   325,     0,   170,    95,
     205,   110,     0,   170,    49,     0,     1,     0,     3,     0,
       4,     0,     5,     0,    59,     0,    60,     0,     3,     0,
      59,     0,    60,     0,   106,     0,   105,     0,   107,     0,
       0,    50,   184,   242,    63,   176,   185,     0,     0,    50,
     184,   233,   230,   177,   185,     0,     0,    50,   184,   315,
     178,   185,     0,     0,    50,   184,   156,   179,   185,     0,
       0,     7,    50,   184,   242,    63,   180,   185,     0,     0,
       7,    50,   184,   233,   230,   181,   185,     0,     0,     7,
      50,   184,   315,   182,   185,     0,     0,     7,    50,   184,
     156,   183,   185,     0,     0,     0,    59,    77,   191,   190,
     189,     0,     4,    77,   191,   190,   189,     0,   188,     0,
     186,     0,   172,    77,   191,    78,   189,     0,     5,    77,
     191,   190,   189,     0,     0,    78,     0,    80,     0,     0,
     192,     0,   193,     0,   192,    62,   193,     0,     0,   194,
     195,     0,   232,     0,    59,     0,   332,    59,     0,   212,
       0,   323,    50,   172,     0,    82,     0,    81,     0,    89,
       0,    90,     0,   112,     0,   204,     0,   211,     0,    49,
       0,    95,   197,   110,     0,    49,     0,    95,   201,   110,
       0,     0,   201,     0,     1,     0,     0,   377,   230,   247,
     256,    67,   202,   264,     0,   197,     0,   111,     0,   340,
     338,   111,     0,   340,   338,     1,   111,     0,   340,     1,
     111,     0,   211,    62,   211,     0,   211,    62,     1,     0,
     204,    62,   211,     0,   204,    62,     1,     0,   211,     0,
     204,     0,   222,     0,   120,   210,     0,    83,   210,     0,
      73,   210,     0,    91,   210,     0,   196,   210,     0,    70,
     172,     0,   239,   206,     0,   239,    95,   232,   110,     0,
     240,   206,     0,   240,    95,   232,   110,     0,   224,   304,
       0,   224,   304,   208,     0,   224,   207,   304,     0,   224,
     207,   304,   208,     0,   224,    95,   232,   110,     0,   224,
      95,   232,   110,   208,     0,   224,   207,    95,   232,   110,
       0,   224,   207,    95,   232,   110,   208,     0,   225,   210,
       0,   225,    96,   113,   210,     0,   225,    96,   197,   113,
     210,     0,    35,   210,     0,    36,   210,     0,    95,   205,
     110,     0,    61,   205,   111,     0,    95,   205,   110,     0,
      49,     0,    95,   242,   110,     0,    67,   264,     0,    95,
     232,   110,     0,   209,    95,   232,   110,     0,   206,     0,
     209,   206,     0,   209,    61,   265,   276,   111,     0,   210,
       0,   211,    86,   211,     0,   211,    87,   211,     0,   211,
      81,   211,     0,   211,    82,   211,     0,   211,    83,   211,
       0,   211,    84,   211,     0,   211,    85,   211,     0,   211,
      79,   211,     0,   211,    80,   211,     0,   211,    76,   211,
       0,   211,    77,   211,     0,   211,    78,   211,     0,   211,
      75,   211,     0,   211,    74,   211,     0,   211,    73,   211,
       0,   211,    71,   211,     0,   211,    72,   211,     0,   211,
      70,   211,     0,   211,    69,   211,     0,   211,    68,   382,
      65,   211,     0,   211,    67,   211,     0,   211,    66,   211,
       0,    64,     0,    64,   211,     0,   210,     0,   212,    86,
     212,     0,   212,    87,   212,     0,   212,    81,   212,     0,
     212,    82,   212,     0,   212,    83,   212,     0,   212,    84,
     212,     0,   212,    85,   212,     0,   212,    79,   212,     0,
     212,    80,   212,     0,   212,    76,   212,     0,   212,    77,
     212,     0,   212,    75,   212,     0,   212,    74,   212,     0,
     212,    73,   212,     0,   212,    71,   212,     0,   212,    72,
     212,     0,   212,    70,   212,     0,   212,    69,   212,     0,
     212,    68,   382,    65,   212,     0,   212,    67,   212,     0,
     212,    66,   212,     0,    64,     0,    64,   212,     0,    91,
     397,   172,     0,    91,   397,   186,     0,   215,     0,   408,
       0,     3,     0,    59,     0,    60,     0,     0,     6,    77,
     214,   191,   190,     0,   408,    77,   214,   191,   190,     0,
      50,   172,    77,   191,   190,     0,    50,     6,    77,   191,
     190,     0,    50,   408,    77,   191,   190,     0,   213,     0,
       4,     0,     5,     0,   219,     0,   257,   219,     0,   213,
       0,    83,   218,     0,    73,   218,     0,    95,   218,   110,
       0,     3,    77,   191,   190,     0,    60,    77,   192,   190,
       0,   317,     0,   213,     0,   220,     0,    95,   218,   110,
       0,   213,     0,    10,     0,   226,     0,    12,     0,    11,
       0,    95,   197,   110,     0,    95,   218,   110,     0,    95,
       1,   110,     0,     0,    95,   223,   343,   110,     0,   213,
      95,   205,   110,     0,   213,    49,     0,   222,    95,   205,
     110,     0,   222,    49,     0,    37,    95,   211,    62,   232,
     110,     0,   222,    96,   197,   113,     0,   222,    89,     0,
     222,    90,     0,    42,     0,     9,    95,   205,   110,     0,
     321,     0,    52,    77,   232,    78,    95,   197,   110,     0,
      53,    77,   232,    78,    95,   197,   110,     0,    54,    77,
     232,    78,    95,   197,   110,     0,    55,    77,   232,    78,
      95,   197,   110,     0,    51,    95,   197,   110,     0,    51,
      95,   232,   110,     0,   332,     3,     0,   332,   215,     0,
     332,   408,     0,   320,     0,   320,    95,   205,   110,     0,
     320,    49,     0,   228,   216,     0,   228,   216,    95,   205,
     110,     0,   228,   216,    49,     0,   228,   217,     0,   228,
     320,     0,   228,   217,    95,   205,   110,     0,   228,   217,
      49,     0,   228,   320,    95,   205,   110,     0,   228,   320,
      49,     0,   228,    91,     8,    49,     0,   228,     8,    56,
      91,     8,    49,     0,   228,     1,     0,    41,     0,   332,
      41,     0,    40,     0,   332,   225,     0,    44,     0,    45,
       0,     0,   222,    94,     0,   222,    93,     0,   242,   244,
      63,     0,   233,   244,    63,     0,   236,   245,    63,     0,
     233,    63,     0,   236,    63,     0,   120,   229,     0,   309,
       0,   315,     0,    49,     0,   231,    49,     0,   237,   336,
       0,   306,   336,     0,   242,   336,     0,   237,     0,   306,
       0,   237,     0,   234,     0,   236,   242,     0,   242,   235,
       0,   242,   238,   235,     0,   236,   242,   235,     0,   236,
     242,   238,     0,   236,   242,   238,   235,     0,     7,     0,
     235,   243,     0,   235,     7,     0,   306,     0,     7,     0,
     236,     9,     0,   236,     7,     0,   236,   257,     0,   242,
       0,   306,   242,     0,   242,   238,     0,   306,   242,   238,
       0,   243,     0,   238,   243,     0,   238,   257,     0,   257,
       0,    14,     0,    30,     0,    29,     0,   270,     0,     8,
       0,   312,     0,   241,    95,   197,   110,     0,   241,    95,
     232,   110,     0,    31,    95,   197,   110,     0,    31,    95,
     232,   110,     0,     8,     0,     9,     0,   270,     0,   252,
       0,   244,    62,   248,     0,   253,     0,   245,    62,   248,
       0,   254,     0,   246,    62,   248,     0,     0,   121,    95,
      12,   110,     0,     0,   230,   247,   256,    67,   249,   264,
       0,   230,   247,   256,     0,     0,   256,    67,   251,   264,
       0,   256,     0,   230,   247,   250,     0,   315,   247,   250,
       0,     0,   315,   247,   255,   250,     0,   156,   247,   256,
       0,     0,   257,     0,   258,     0,   257,   258,     0,    32,
      95,    95,   259,   110,   110,     0,   260,     0,   259,    62,
     260,     0,     0,   261,     0,   261,    95,     3,   110,     0,
     261,    95,     3,    62,   205,   110,     0,   261,    95,   205,
     110,     0,   172,     0,     7,     0,     8,     0,     9,     0,
     172,     0,   262,    62,   172,     0,     0,    67,   264,     0,
     211,     0,    61,   111,     0,    61,   265,   111,     0,    61,
     265,    62,   111,     0,     1,     0,   264,     0,   265,    62,
     264,     0,    96,   211,   113,   264,     0,   172,    65,   264,
       0,   265,    62,   172,    65,   264,     0,   104,   151,   153,
       0,   104,   151,   367,     0,   104,   151,     1,     0,     0,
     267,   266,   152,     0,   103,   211,   109,     0,   103,     1,
     109,     0,     0,   269,   268,     0,   269,     1,     0,     0,
      15,   172,    61,   271,   301,   111,     0,     0,    15,    61,
     272,   301,   111,     0,    15,   172,     0,    15,   330,     0,
      47,   325,     0,     0,     0,     0,   282,   283,    61,   273,
     288,   111,   256,   274,   269,   275,   267,     0,   281,     0,
       0,    62,     0,     0,    62,     0,    38,     0,   278,     7,
       0,   278,     8,     0,   278,     9,     0,   278,    38,     0,
     278,   257,     0,   278,   172,     0,   278,   323,   172,     0,
     278,   332,   323,   172,     0,   278,   332,   172,     0,   278,
     187,     0,   278,   323,   187,     0,   278,   332,   323,   187,
       0,   279,     0,   278,   174,     0,   280,     0,   279,    61,
       0,   279,    65,     0,   280,    61,     0,   280,    65,     0,
     278,   174,    61,     0,   278,   174,    65,     0,   278,    61,
       0,     0,    65,   397,     0,    65,   397,   284,     0,   285,
       0,   284,    62,   397,   285,     0,   286,     0,   287,   397,
     286,     0,   325,     0,   311,     0,    39,   397,     0,     7,
     397,     0,   287,    39,   397,     0,   287,     7,   397,     0,
       0,   290,     0,   288,   289,   290,     0,   288,   289,     0,
      39,    65,     0,   291,     0,   290,   291,     0,   292,    63,
       0,   292,   111,     0,   163,    65,     0,   163,    97,     0,
     163,    26,     0,   163,    61,     0,    63,     0,   120,   291,
       0,   140,   291,     0,   140,   233,    63,     0,   399,     0,
     233,   293,     0,   236,   294,     0,   315,   247,   256,   263,
       0,   156,   247,   256,   263,     0,    65,   211,     0,     1,
       0,   236,   162,   247,   256,   263,     0,   162,   247,   256,
     263,     0,   130,     0,     0,   295,     0,   293,    62,   296,
       0,     0,   298,     0,   294,    62,   300,     0,   297,     0,
     298,     0,   299,     0,   300,     0,   309,   247,   256,   263,
       0,     4,    65,   211,   256,     0,   315,   247,   256,   263,
       0,   156,   247,   256,   263,     0,     3,    65,   211,   256,
       0,    65,   211,   256,     0,   309,   247,   256,   263,     0,
       4,    65,   211,   256,     0,   315,   247,   256,   263,     0,
       3,    65,   211,   256,     0,    65,   211,   256,     0,   302,
     277,     0,   277,     0,   303,     0,   302,    62,   303,     0,
     172,     0,   172,    67,   211,     0,   377,   333,     0,   377,
       0,    95,   232,   110,    96,   197,   113,     0,     0,   305,
       9,     0,     9,     0,   306,     9,     0,   257,     0,   306,
     257,     0,    95,   205,   110,     0,    95,   387,   110,     0,
      49,     0,    95,     1,   110,     0,   309,     0,   257,   309,
       0,    83,   306,   308,     0,    73,   306,   308,     0,    83,
     308,     0,    73,   308,     0,   331,   305,   308,     0,   310,
       0,   310,   307,   305,   402,     0,   310,    96,   197,   113,
       0,   310,    96,   113,     0,    95,   308,   110,     0,   323,
     322,     0,   322,     0,   322,     0,   332,   322,     0,   311,
       0,   313,     0,   332,   313,     0,   323,   322,     0,   315,
       0,   257,   315,     0,    83,   306,   314,     0,    73,   306,
     314,     0,    83,   314,     0,    73,   314,     0,   331,   305,
     314,     0,   221,     0,    83,   306,   314,     0,    73,   306,
     314,     0,    83,   316,     0,    73,   316,     0,   331,   305,
     314,     0,   317,     0,   221,   307,   305,   402,     0,    95,
     316,   110,     0,   221,    96,   197,   113,     0,   221,    96,
     113,     0,   319,     0,   332,   319,     0,   332,   213,     0,
     323,   220,     0,   323,   217,     0,   323,   216,     0,   323,
     213,     0,   323,   216,     0,   319,     0,   332,   319,     0,
     242,    95,   205,   110,     0,   242,    95,   218,   110,     0,
     242,   231,     0,     4,     0,     5,     0,   186,     0,   324,
       0,   323,   324,     0,   323,    50,   329,    56,     0,   323,
       3,    56,     0,   323,    59,    56,     0,     4,    56,     0,
       5,    56,     0,    60,    56,     0,   186,    56,     0,   326,
       0,   332,   326,     0,   327,   172,     0,   327,   186,     0,
     327,   329,     0,   327,    50,   329,     0,   328,     0,   327,
     328,     0,   327,   329,    56,     0,   327,    50,   329,    56,
       0,     4,    56,     0,     5,    56,     0,   186,    56,     0,
      59,    56,     0,     3,    56,     0,    60,    56,     0,   172,
      77,   191,   190,     0,   332,   322,     0,   313,     0,   332,
     313,     0,   323,    83,     0,   332,   323,    83,     0,    56,
       0,    83,   305,   333,     0,    83,   305,     0,    73,   305,
     333,     0,    73,   305,     0,   331,   305,     0,   331,   305,
     333,     0,   334,     0,    96,   197,   113,     0,   334,    96,
     197,   113,     0,   336,     0,   257,   336,     0,    83,   306,
     335,     0,    83,   335,     0,    83,   306,     0,    83,     0,
      73,   306,   335,     0,    73,   335,     0,    73,   306,     0,
      73,     0,   331,   305,     0,   331,   305,   335,     0,   337,
       0,    95,   335,   110,     0,   337,    95,   387,   110,   305,
     402,     0,   337,    49,   305,   402,     0,   337,    96,   197,
     113,     0,   337,    96,   113,     0,    95,   388,   110,   305,
     402,     0,   209,   305,   402,     0,   231,   305,   402,     0,
      96,   197,   113,     0,    96,   113,     0,   351,     0,   339,
       0,   338,   351,     0,   338,   339,     0,     1,    63,     0,
       0,   341,     0,   342,     0,   341,   342,     0,    34,   262,
      63,     0,     0,   409,    61,   344,   203,     0,   343,     0,
       0,     0,    16,   347,   199,   348,   349,     0,   345,     0,
       0,   350,   409,   352,     0,   345,     0,   409,   352,     0,
     229,     0,   197,    63,     0,     0,   346,    17,   353,   349,
       0,   346,     0,     0,     0,    18,   354,   199,   355,   349,
       0,     0,     0,    19,   356,   349,    18,   357,   198,    63,
       0,     0,     0,     0,     0,    20,   358,    95,   380,   359,
     200,    63,   360,   382,   110,   361,   349,     0,     0,     0,
      21,   362,    95,   201,   110,   363,   349,     0,     0,    22,
     211,    65,   364,   351,     0,     0,    22,   211,    13,   211,
      65,   365,   351,     0,     0,    23,    65,   366,   351,     0,
      24,    63,     0,    25,    63,     0,    26,    63,     0,    26,
     197,    63,     0,   121,   381,    95,    12,   110,    63,     0,
     121,   381,    95,    12,    65,   383,   110,    63,     0,   121,
     381,    95,    12,    65,   383,    65,   383,   110,    63,     0,
     121,   381,    95,    12,    56,   383,   110,    63,     0,   121,
     381,    95,    12,    65,   383,    65,   383,    65,   386,   110,
      63,     0,   121,   381,    95,    12,    56,   383,    65,   386,
     110,    63,     0,   121,   381,    95,    12,    65,   383,    56,
     386,   110,    63,     0,    27,    83,   197,    63,     0,    27,
     172,    63,     0,   379,   351,     0,   379,   111,     0,    63,
       0,   370,     0,   132,     0,   131,     0,   128,     0,     0,
       0,    97,   368,   153,   369,   373,     0,     0,     0,    97,
     371,   345,   372,   373,     0,   374,     0,   373,   374,     0,
       0,     0,     0,    98,   375,   378,   376,   345,     0,   237,
       0,   306,     0,    95,    13,   110,     0,    95,   396,   110,
       0,     3,    65,     0,    59,    65,     0,     4,    65,     0,
       5,    65,     0,   382,    63,     0,   229,     0,    61,   203,
       0,     0,     9,     0,     0,   197,     0,     1,     0,     0,
     384,     0,   385,     0,   384,    62,   385,     0,    12,    95,
     197,   110,     0,    96,   172,   113,    12,    95,   197,   110,
       0,    12,     0,   386,    62,    12,     0,     0,   388,     0,
     232,     0,   392,     0,   393,    13,     0,   392,    13,     0,
     232,    13,     0,    13,     0,   392,    65,     0,   232,    65,
       0,     0,    67,   390,   391,     0,   102,     0,   264,     0,
     394,     0,   396,   389,     0,   393,   395,     0,   393,   398,
       0,   393,   398,    67,   264,     0,   392,    62,     0,   232,
      62,     0,   234,   230,     0,   237,   230,     0,   242,   230,
       0,   234,   336,     0,   234,     0,   236,   315,     0,   396,
       0,   396,   389,     0,   394,     0,   232,     0,     0,     0,
     315,     0,     3,   400,     3,   401,    63,     0,    77,   191,
     190,     0,     0,    95,   205,   110,     0,     0,     0,    64,
      95,   404,   110,     0,    64,    49,     0,   232,     0,     1,
       0,   403,     0,   404,    62,   403,     0,     0,    83,   305,
     405,     0,    73,   305,   405,     0,   331,   305,   405,     0,
      43,     0,     0,   406,    83,   407,     0,   406,    84,   407,
       0,   406,    85,   407,     0,   406,    81,   407,     0,   406,
      82,   407,     0,   406,    73,   407,     0,   406,    71,   407,
       0,   406,    72,   407,     0,   406,    91,   407,     0,   406,
      62,   407,     0,   406,    76,   407,     0,   406,    77,   407,
       0,   406,    78,   407,     0,   406,    75,   407,     0,   406,
      66,   407,     0,   406,    67,   407,     0,   406,    79,   407,
       0,   406,    80,   407,     0,   406,    89,   407,     0,   406,
      90,   407,     0,   406,    70,   407,     0,   406,    69,   407,
       0,   406,   112,   407,     0,   406,    68,    65,   407,     0,
     406,    74,   407,     0,   406,    93,   407,     0,   406,    86,
     407,     0,   406,    49,   407,     0,   406,    96,   113,   407,
       0,   406,    41,   407,     0,   406,    40,   407,     0,   406,
      41,    96,   113,   407,     0,   406,    40,    96,   113,   407,
       0,   406,   377,   405,   407,     0,   406,     1,   407,     0,
       0
};

#endif

#if YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined. */
static const short yyrline[] =
{
       0,   485,   488,   496,   496,   500,   504,   506,   509,   513,
     517,   523,   527,   527,   535,   538,   541,   541,   545,   547,
     549,   551,   553,   555,   555,   559,   559,   563,   564,   566,
     567,   571,   571,   583,   586,   588,   592,   595,   597,   601,
     601,   616,   623,   632,   634,   635,   637,   641,   644,   650,
     650,   657,   663,   665,   668,   671,   675,   678,   682,   685,
     689,   694,   704,   706,   708,   710,   712,   719,   722,   726,
     729,   731,   733,   736,   739,   743,   745,   747,   749,   759,
     761,   763,   765,   767,   768,   775,   776,   777,   779,   780,
     783,   785,   788,   790,   791,   794,   796,   802,   802,   813,
     816,   818,   822,   822,   827,   831,   831,   835,   839,   839,
     843,   847,   847,   851,   857,   862,   865,   868,   871,   879,
     882,   885,   887,   889,   891,   897,   906,   909,   911,   913,
     916,   918,   923,   930,   933,   935,   939,   939,   948,   955,
     961,   966,   977,   980,   988,   991,   994,   999,  1003,  1006,
    1011,  1013,  1014,  1015,  1016,  1019,  1021,  1022,  1025,  1027,
    1028,  1031,  1031,  1036,  1036,  1040,  1040,  1043,  1043,  1046,
    1046,  1051,  1051,  1057,  1057,  1061,  1061,  1067,  1071,  1079,
    1083,  1086,  1089,  1091,  1096,  1102,  1112,  1114,  1122,  1125,
    1128,  1131,  1135,  1135,  1144,  1147,  1153,  1159,  1160,  1172,
    1175,  1177,  1179,  1181,  1185,  1188,  1191,  1196,  1200,  1205,
    1209,  1212,  1213,  1217,  1217,  1240,  1243,  1245,  1246,  1247,
    1250,  1254,  1257,  1259,  1263,  1266,  1269,  1273,  1276,  1278,
    1280,  1282,  1285,  1287,  1290,  1294,  1297,  1304,  1307,  1310,
    1313,  1316,  1321,  1324,  1327,  1331,  1333,  1337,  1341,  1343,
    1347,  1350,  1355,  1358,  1360,  1365,  1375,  1380,  1386,  1388,
    1390,  1403,  1406,  1408,  1410,  1412,  1414,  1416,  1418,  1420,
    1422,  1424,  1426,  1428,  1430,  1432,  1434,  1436,  1438,  1440,
    1442,  1444,  1446,  1450,  1452,  1454,  1458,  1461,  1463,  1465,
    1467,  1469,  1471,  1473,  1475,  1477,  1479,  1481,  1483,  1485,
    1487,  1489,  1491,  1493,  1495,  1497,  1499,  1503,  1505,  1507,
    1511,  1514,  1516,  1517,  1518,  1519,  1520,  1523,  1536,  1544,
    1553,  1556,  1558,  1563,  1565,  1566,  1569,  1571,  1579,  1581,
    1583,  1585,  1589,  1592,  1596,  1600,  1601,  1602,  1606,  1614,
    1615,  1616,  1626,  1628,  1630,  1633,  1635,  1635,  1650,  1652,
    1654,  1656,  1658,  1661,  1663,  1665,  1668,  1670,  1681,  1682,
    1686,  1690,  1694,  1698,  1700,  1704,  1706,  1708,  1716,  1725,
    1727,  1729,  1731,  1733,  1735,  1737,  1739,  1741,  1743,  1745,
    1748,  1750,  1752,  1796,  1799,  1803,  1806,  1810,  1813,  1817,
    1825,  1828,  1835,  1841,  1845,  1847,  1852,  1854,  1861,  1863,
    1867,  1871,  1877,  1881,  1884,  1888,  1891,  1901,  1904,  1908,
    1912,  1915,  1918,  1921,  1924,  1930,  1936,  1938,  1959,  1962,
    1967,  1972,  1980,  1990,  1994,  1997,  2000,  2005,  2008,  2010,
    2012,  2016,  2020,  2024,  2032,  2035,  2037,  2039,  2043,  2047,
    2062,  2081,  2084,  2086,  2089,  2091,  2095,  2097,  2101,  2103,
    2107,  2110,  2114,  2114,  2120,  2133,  2133,  2141,  2147,  2152,
    2157,  2157,  2166,  2173,  2176,  2180,  2183,  2187,  2192,  2195,
    2199,  2202,  2204,  2206,  2208,  2215,  2217,  2218,  2219,  2223,
    2226,  2230,  2233,  2240,  2242,  2245,  2248,  2251,  2257,  2260,
    2263,  2265,  2267,  2271,  2277,  2282,  2288,  2290,  2295,  2298,
    2302,  2304,  2306,  2310,  2310,  2320,  2320,  2329,  2332,  2335,
    2341,  2341,  2341,  2341,  2394,  2402,  2404,  2407,  2409,  2414,
    2416,  2418,  2420,  2422,  2424,  2428,  2434,  2439,  2444,  2451,
    2457,  2462,  2469,  2477,  2483,  2490,  2500,  2508,  2514,  2520,
    2528,  2536,  2549,  2552,  2555,  2559,  2561,  2565,  2568,  2572,
    2576,  2580,  2582,  2586,  2597,  2611,  2612,  2613,  2614,  2617,
    2626,  2633,  2641,  2643,  2648,  2650,  2652,  2654,  2656,  2658,
    2661,  2671,  2676,  2680,  2705,  2711,  2713,  2715,  2717,  2728,
    2733,  2735,  2741,  2744,  2751,  2761,  2764,  2771,  2781,  2783,
    2786,  2788,  2791,  2795,  2800,  2804,  2807,  2810,  2815,  2818,
    2822,  2825,  2827,  2831,  2833,  2840,  2842,  2845,  2848,  2853,
    2857,  2862,  2872,  2875,  2879,  2883,  2886,  2889,  2898,  2901,
    2903,  2905,  2911,  2913,  2922,  2925,  2927,  2929,  2931,  2935,
    2938,  2941,  2943,  2945,  2947,  2951,  2954,  2965,  2975,  2977,
    2978,  2982,  2990,  2992,  3000,  3003,  3005,  3007,  3009,  3013,
    3016,  3019,  3021,  3023,  3025,  3029,  3032,  3035,  3037,  3039,
    3041,  3043,  3045,  3049,  3056,  3060,  3065,  3069,  3074,  3076,
    3080,  3083,  3085,  3089,  3091,  3092,  3095,  3097,  3099,  3103,
    3106,  3113,  3124,  3130,  3136,  3140,  3142,  3146,  3160,  3162,
    3164,  3168,  3176,  3189,  3192,  3199,  3212,  3218,  3220,  3221,
    3222,  3230,  3235,  3244,  3245,  3249,  3252,  3258,  3264,  3267,
    3269,  3271,  3273,  3277,  3281,  3285,  3288,  3292,  3294,  3303,
    3306,  3308,  3310,  3312,  3314,  3316,  3318,  3320,  3324,  3328,
    3332,  3336,  3338,  3340,  3342,  3344,  3346,  3348,  3350,  3352,
    3360,  3362,  3363,  3364,  3367,  3373,  3375,  3380,  3382,  3385,
    3396,  3396,  3404,  3409,  3409,  3409,  3420,  3422,  3422,  3430,
    3432,  3436,  3440,  3442,  3442,  3450,  3453,  3453,  3453,  3463,
    3463,  3463,  3473,  3473,  3473,  3473,  3473,  3484,  3484,  3484,
    3491,  3491,  3495,  3495,  3499,  3499,  3503,  3505,  3507,  3509,
    3511,  3516,  3519,  3522,  3525,  3528,  3531,  3534,  3540,  3542,
    3544,  3548,  3551,  3553,  3555,  3558,  3562,  3562,  3562,  3571,
    3571,  3571,  3580,  3582,  3583,  3594,  3594,  3594,  3603,  3605,
    3608,  3625,  3633,  3636,  3638,  3640,  3644,  3647,  3648,  3656,
    3659,  3662,  3665,  3666,  3672,  3675,  3678,  3680,  3684,  3687,
    3693,  3696,  3706,  3711,  3712,  3719,  3722,  3725,  3727,  3730,
    3732,  3742,  3756,  3756,  3763,  3765,  3769,  3773,  3776,  3779,
    3781,  3785,  3787,  3794,  3800,  3803,  3807,  3810,  3813,  3818,
    3822,  3827,  3829,  3832,  3837,  3843,  3859,  3867,  3870,  3873,
    3876,  3879,  3882,  3884,  3888,  3894,  3898,  3901,  3905,  3908,
    3910,  3912,  3918,  3931,  3940,  3943,  3945,  3947,  3949,  3951,
    3953,  3955,  3957,  3959,  3961,  3963,  3965,  3967,  3969,  3971,
    3973,  3975,  3977,  3979,  3981,  3983,  3985,  3987,  3989,  3991,
    3993,  3995,  3997,  3999,  4001,  4003,  4005,  4007,  4009,  4016
};
#endif


#if (YYDEBUG) || defined YYERROR_VERBOSE

/* YYTNAME[TOKEN_NUM] -- String name of the token TOKEN_NUM. */
static const char *const yytname[] =
{
  "$", "error", "$undefined.", "IDENTIFIER", "tTYPENAME", "SELFNAME", 
  "PFUNCNAME", "SCSPEC", "TYPESPEC", "CV_QUALIFIER", "CONSTANT", 
  "VAR_FUNC_NAME", "STRING", "ELLIPSIS", "SIZEOF", "ENUM", "IF", "ELSE", 
  "WHILE", "DO", "FOR", "SWITCH", "CASE", "DEFAULT", "BREAK", "CONTINUE", 
  "RETURN_KEYWORD", "GOTO", "ASM_KEYWORD", "TYPEOF", "ALIGNOF", "SIGOF", 
  "ATTRIBUTE", "EXTENSION", "LABEL", "REALPART", "IMAGPART", "VA_ARG", 
  "AGGR", "VISSPEC", "DELETE", "NEW", "THIS", "OPERATOR", "CXX_TRUE", 
  "CXX_FALSE", "NAMESPACE", "TYPENAME_KEYWORD", "USING", "LEFT_RIGHT", 
  "TEMPLATE", "TYPEID", "DYNAMIC_CAST", "STATIC_CAST", "REINTERPRET_CAST", 
  "CONST_CAST", "SCOPE", "EXPORT", "EMPTY", "PTYPENAME", "NSNAME", "'{'", 
  "','", "';'", "THROW", "':'", "ASSIGN", "'='", "'?'", "OROR", "ANDAND", 
  "'|'", "'^'", "'&'", "MIN_MAX", "EQCOMPARE", "ARITHCOMPARE", "'<'", 
  "'>'", "LSHIFT", "RSHIFT", "'+'", "'-'", "'*'", "'/'", "'%'", 
  "POINTSAT_STAR", "DOT_STAR", "UNARY", "PLUSPLUS", "MINUSMINUS", "'~'", 
  "HYPERUNARY", "POINTSAT", "'.'", "'('", "'['", "TRY", "CATCH", 
  "EXTERN_LANG_STRING", "ALL", "PRE_PARSED_CLASS_DECL", "DEFARG", 
  "DEFARG_MARKER", "PRE_PARSED_FUNCTION_DECL", "TYPENAME_DEFN", 
  "IDENTIFIER_DEFN", "PTYPENAME_DEFN", "END_OF_LINE", 
  "END_OF_SAVED_INPUT", "')'", "'}'", "'!'", "']'", "program", "extdefs", 
  "@1", "extdefs_opt", ".hush_warning", ".warning_ok", "extension", 
  "asm_keyword", "lang_extdef", "@2", "extdef", "@3", "@4", "@5", 
  "namespace_alias", "@6", "using_decl", "namespace_using_decl", 
  "using_directive", "@7", "namespace_qualifier", "any_id", 
  "extern_lang_string", "template_parm_header", "@8", 
  "template_spec_header", "template_header", "template_parm_list", 
  "maybe_identifier", "template_type_parm", "template_template_parm", 
  "template_parm", "template_def", "template_extdef", "template_datadef", 
  "datadef", "ctor_initializer_opt", "maybe_return_init", 
  "eat_saved_input", "function_body", "@9", "fndef", 
  "constructor_declarator", "@10", "@11", "@12", "@13", "fn.def1", 
  "component_constructor_declarator", "fn_def2", "return_id", 
  "return_init", "base_init", "@14", "begin_function_body_", 
  "member_init_list", "begin_member_init", "member_init", "identifier", 
  "notype_identifier", "identifier_defn", "explicit_instantiation", "@15", 
  "@16", "@17", "@18", "@19", "@20", "@21", "@22", 
  "begin_explicit_instantiation", "end_explicit_instantiation", 
  "template_type", "apparent_template_type", "self_template_type", 
  "finish_template_type_", "template_close_bracket", 
  "template_arg_list_opt", "template_arg_list", "template_arg", "@23", 
  "template_arg_1", "unop", "expr", "paren_expr_or_null", 
  "paren_cond_or_null", "xcond", "condition", "@24", "compstmtend", 
  "nontrivial_exprlist", "nonnull_exprlist", "unary_expr", 
  "new_placement", "new_initializer", "regcast_or_absdcl", "cast_expr", 
  "expr_no_commas", "expr_no_comma_rangle", "notype_unqualified_id", 
  "do_id", "template_id", "object_template_id", "unqualified_id", 
  "expr_or_declarator_intern", "expr_or_declarator", 
  "notype_template_declarator", "direct_notype_declarator", "primary", 
  "@25", "new", "delete", "boolean_literal", "nodecls", "object", "decl", 
  "declarator", "fcast_or_absdcl", "type_id", "typed_declspecs", 
  "typed_declspecs1", "reserved_declspecs", "declmods", "typed_typespecs", 
  "reserved_typespecquals", "sizeof", "alignof", "typeof", "typespec", 
  "typespecqual_reserved", "initdecls", "notype_initdecls", 
  "nomods_initdecls", "maybeasm", "initdcl", "@26", "initdcl0_innards", 
  "@27", "initdcl0", "notype_initdcl0", "nomods_initdcl0", "@28", 
  "maybe_attribute", "attributes", "attribute", "attribute_list", 
  "attrib", "any_word", "identifiers_or_typenames", "maybe_init", "init", 
  "initlist", "pending_inline", "pending_inlines", "defarg_again", 
  "pending_defargs", "structsp", "@29", "@30", "@31", "@32", "@33", 
  "maybecomma", "maybecomma_warn", "aggr", "class_head", 
  "class_head_apparent_template", "class_head_decl", "class_head_defn", 
  "maybe_base_class_list", "base_class_list", "base_class", 
  "base_class_1", "base_class_access_list", "opt.component_decl_list", 
  "access_specifier", "component_decl_list", "component_decl", 
  "component_decl_1", "components", "notype_components", 
  "component_declarator0", "component_declarator", 
  "after_type_component_declarator0", "notype_component_declarator0", 
  "after_type_component_declarator", "notype_component_declarator", 
  "enumlist_opt", "enumlist", "enumerator", "new_type_id", 
  "cv_qualifiers", "nonempty_cv_qualifiers", "maybe_parmlist", 
  "after_type_declarator_intern", "after_type_declarator", 
  "direct_after_type_declarator", "nonnested_type", "complete_type_name", 
  "nested_type", "notype_declarator_intern", "notype_declarator", 
  "complex_notype_declarator", "complex_direct_notype_declarator", 
  "qualified_id", "notype_qualified_id", "overqualified_id", 
  "functional_cast", "type_name", "nested_name_specifier", 
  "nested_name_specifier_1", "typename_sub", "typename_sub0", 
  "typename_sub1", "typename_sub2", "explicit_template_type", 
  "complex_type_name", "ptr_to_mem", "global_scope", "new_declarator", 
  "direct_new_declarator", "absdcl_intern", "absdcl", 
  "direct_abstract_declarator", "stmts", "errstmt", "maybe_label_decls", 
  "label_decls", "label_decl", "compstmt_or_stmtexpr", "@34", "compstmt", 
  "simple_if", "@35", "@36", "implicitly_scoped_stmt", "@37", "stmt", 
  "simple_stmt", "@38", "@39", "@40", "@41", "@42", "@43", "@44", "@45", 
  "@46", "@47", "@48", "@49", "@50", "@51", "function_try_block", "@52", 
  "@53", "try_block", "@54", "@55", "handler_seq", "handler", "@56", 
  "@57", "type_specifier_seq", "handler_args", "label_colon", 
  "for.init.statement", "maybe_cv_qualifier", "xexpr", "asm_operands", 
  "nonnull_asm_operands", "asm_operand", "asm_clobbers", "parmlist", 
  "complex_parmlist", "defarg", "@58", "defarg1", "parms", "parms_comma", 
  "named_parm", "full_parm", "parm", "see_typename", "bad_parm", 
  "bad_decl", "template_arg_list_ignore", "arg_list_ignore", 
  "exception_specification_opt", "ansi_raise_identifier", 
  "ansi_raise_identifiers", "conversion_declarator", "operator", 
  "unoperator", "operator_name", "save_lineno", 0
};
#endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives. */
static const short yyr1[] =
{
       0,   114,   114,   116,   115,   115,   117,   117,   118,   119,
     120,   121,   123,   122,   124,   124,   125,   124,   124,   124,
     124,   124,   124,   126,   124,   127,   124,   124,   124,   124,
     124,   129,   128,   130,   130,   130,   131,   131,   131,   133,
     132,   134,   134,   135,   135,   135,   135,   136,   136,   138,
     137,   139,   140,   140,   141,   141,   142,   142,   143,   143,
     144,   145,   145,   145,   145,   145,   145,   146,   146,   147,
     147,   147,   147,   147,   147,   148,   148,   148,   148,   149,
     149,   149,   149,   149,   149,   149,   149,   149,   149,   149,
     150,   150,   151,   151,   151,   152,   152,   154,   153,   155,
     155,   155,   157,   156,   156,   158,   156,   156,   159,   156,
     156,   160,   156,   156,   161,   161,   161,   161,   161,   162,
     162,   162,   162,   162,   162,   163,   163,   163,   163,   163,
     163,   163,   164,   165,   165,   165,   167,   166,   168,   169,
     169,   169,   169,   170,   170,   170,   170,   171,   171,   171,
     172,   172,   172,   172,   172,   173,   173,   173,   174,   174,
     174,   176,   175,   177,   175,   178,   175,   179,   175,   180,
     175,   181,   175,   182,   175,   183,   175,   184,   185,   186,
     186,   186,   187,   187,   188,   189,   190,   190,   191,   191,
     192,   192,   194,   193,   195,   195,   195,   195,   195,   196,
     196,   196,   196,   196,   197,   197,   198,   198,   199,   199,
     200,   200,   200,   202,   201,   201,   203,   203,   203,   203,
     204,   204,   204,   204,   205,   205,   206,   206,   206,   206,
     206,   206,   206,   206,   206,   206,   206,   206,   206,   206,
     206,   206,   206,   206,   206,   206,   206,   206,   206,   206,
     207,   207,   208,   208,   208,   208,   209,   209,   210,   210,
     210,   211,   211,   211,   211,   211,   211,   211,   211,   211,
     211,   211,   211,   211,   211,   211,   211,   211,   211,   211,
     211,   211,   211,   211,   211,   211,   212,   212,   212,   212,
     212,   212,   212,   212,   212,   212,   212,   212,   212,   212,
     212,   212,   212,   212,   212,   212,   212,   212,   212,   212,
     213,   213,   213,   213,   213,   213,   213,   214,   215,   215,
     216,   216,   216,   217,   217,   217,   218,   218,   219,   219,
     219,   219,   220,   220,   221,   221,   221,   221,   222,   222,
     222,   222,   222,   222,   222,   222,   223,   222,   222,   222,
     222,   222,   222,   222,   222,   222,   222,   222,   222,   222,
     222,   222,   222,   222,   222,   222,   222,   222,   222,   222,
     222,   222,   222,   222,   222,   222,   222,   222,   222,   222,
     222,   222,   222,   224,   224,   225,   225,   226,   226,   227,
     228,   228,   229,   229,   229,   229,   229,   229,   230,   230,
     231,   231,   232,   232,   232,   232,   232,   233,   233,   234,
     234,   234,   234,   234,   234,   235,   235,   235,   236,   236,
     236,   236,   236,   237,   237,   237,   237,   238,   238,   238,
     238,   239,   240,   241,   242,   242,   242,   242,   242,   242,
     242,   243,   243,   243,   244,   244,   245,   245,   246,   246,
     247,   247,   249,   248,   248,   251,   250,   250,   252,   253,
     255,   254,   254,   256,   256,   257,   257,   258,   259,   259,
     260,   260,   260,   260,   260,   261,   261,   261,   261,   262,
     262,   263,   263,   264,   264,   264,   264,   264,   265,   265,
     265,   265,   265,   266,   266,   266,   267,   267,   268,   268,
     269,   269,   269,   271,   270,   272,   270,   270,   270,   270,
     273,   274,   275,   270,   270,   276,   276,   277,   277,   278,
     278,   278,   278,   278,   278,   279,   279,   279,   279,   280,
     280,   280,   281,   281,   281,   282,   282,   282,   282,   282,
     282,   282,   283,   283,   283,   284,   284,   285,   285,   286,
     286,   287,   287,   287,   287,   288,   288,   288,   288,   289,
     290,   290,   291,   291,   291,   291,   291,   291,   291,   291,
     291,   291,   291,   292,   292,   292,   292,   292,   292,   292,
     292,   292,   293,   293,   293,   294,   294,   294,   295,   295,
     296,   296,   297,   297,   298,   298,   298,   298,   299,   299,
     300,   300,   300,   301,   301,   302,   302,   303,   303,   304,
     304,   304,   305,   305,   306,   306,   306,   306,   307,   307,
     307,   307,   308,   308,   309,   309,   309,   309,   309,   309,
     310,   310,   310,   310,   310,   310,   311,   311,   312,   312,
     312,   313,   314,   314,   315,   315,   315,   315,   315,   315,
     316,   316,   316,   316,   316,   316,   317,   317,   317,   317,
     317,   317,   317,   317,   318,   318,   319,   319,   320,   320,
     321,   321,   321,   322,   322,   322,   323,   323,   323,   323,
     323,   324,   324,   324,   324,   325,   325,   326,   326,   326,
     326,   327,   327,   327,   327,   328,   328,   328,   328,   328,
     328,   329,   330,   330,   330,   331,   331,   332,   333,   333,
     333,   333,   333,   333,   333,   334,   334,   335,   335,   336,
     336,   336,   336,   336,   336,   336,   336,   336,   336,   336,
     337,   337,   337,   337,   337,   337,   337,   337,   337,   337,
     338,   338,   338,   338,   339,   340,   340,   341,   341,   342,
     344,   343,   345,   347,   348,   346,   349,   350,   349,   351,
     351,   352,   352,   353,   352,   352,   354,   355,   352,   356,
     357,   352,   358,   359,   360,   361,   352,   362,   363,   352,
     364,   352,   365,   352,   366,   352,   352,   352,   352,   352,
     352,   352,   352,   352,   352,   352,   352,   352,   352,   352,
     352,   352,   352,   352,   352,   352,   368,   369,   367,   371,
     372,   370,   373,   373,   373,   375,   376,   374,   377,   377,
     378,   378,   379,   379,   379,   379,   380,   380,   380,   381,
     381,   382,   382,   382,   383,   383,   384,   384,   385,   385,
     386,   386,   387,   387,   387,   388,   388,   388,   388,   388,
     388,   388,   390,   389,   391,   391,   392,   392,   392,   392,
     392,   393,   393,   394,   394,   394,   394,   394,   394,   395,
     395,   396,   396,   397,   398,   398,   399,   400,   400,   401,
     401,   402,   402,   402,   403,   403,   404,   404,   405,   405,
     405,   405,   406,   407,   408,   408,   408,   408,   408,   408,
     408,   408,   408,   408,   408,   408,   408,   408,   408,   408,
     408,   408,   408,   408,   408,   408,   408,   408,   408,   408,
     408,   408,   408,   408,   408,   408,   408,   408,   408,   409
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN. */
static const short yyr2[] =
{
       0,     0,     1,     0,     2,     2,     1,     0,     0,     0,
       1,     1,     0,     2,     2,     1,     0,     3,     1,     5,
       4,     5,     4,     0,     6,     0,     5,     1,     2,     1,
       2,     0,     6,     2,     3,     3,     3,     3,     4,     0,
       5,     2,     3,     1,     1,     2,     2,     1,     2,     0,
       5,     3,     1,     1,     1,     3,     1,     0,     2,     2,
       3,     1,     3,     1,     3,     1,     3,     2,     2,     2,
       1,     1,     5,     4,     2,     2,     3,     3,     2,     2,
       3,     3,     2,     2,     2,     2,     2,     2,     1,     1,
       1,     1,     0,     1,     2,     0,     1,     0,     6,     3,
       3,     3,     0,     8,     5,     0,     9,     6,     0,     8,
       5,     0,     9,     6,     2,     2,     1,     2,     1,     6,
       8,     4,     6,     6,     4,     2,     1,     2,     2,     1,
       2,     1,     2,     2,     4,     2,     0,     3,     0,     0,
       1,     3,     2,     0,     1,     1,     1,     4,     2,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     0,     6,     0,     6,     0,     5,     0,     5,     0,
       7,     0,     7,     0,     6,     0,     6,     0,     0,     5,
       5,     1,     1,     5,     5,     0,     1,     1,     0,     1,
       1,     3,     0,     2,     1,     1,     2,     1,     3,     1,
       1,     1,     1,     1,     1,     1,     1,     3,     1,     3,
       0,     1,     1,     0,     7,     1,     1,     3,     4,     3,
       3,     3,     3,     3,     1,     1,     1,     2,     2,     2,
       2,     2,     2,     2,     4,     2,     4,     2,     3,     3,
       4,     4,     5,     5,     6,     2,     4,     5,     2,     2,
       3,     3,     3,     1,     3,     2,     3,     4,     1,     2,
       5,     1,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     5,     3,     3,     1,     2,     1,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     5,     3,     3,     1,     2,
       3,     3,     1,     1,     1,     1,     1,     0,     5,     5,
       5,     5,     5,     1,     1,     1,     1,     2,     1,     2,
       2,     3,     4,     4,     1,     1,     1,     3,     1,     1,
       1,     1,     1,     3,     3,     3,     0,     4,     4,     2,
       4,     2,     6,     4,     2,     2,     1,     4,     1,     7,
       7,     7,     7,     4,     4,     2,     2,     2,     1,     4,
       2,     2,     5,     3,     2,     2,     5,     3,     5,     3,
       4,     6,     2,     1,     2,     1,     2,     1,     1,     0,
       2,     2,     3,     3,     3,     2,     2,     2,     1,     1,
       1,     2,     2,     2,     2,     1,     1,     1,     1,     2,
       2,     3,     3,     3,     4,     1,     2,     2,     1,     1,
       2,     2,     2,     1,     2,     2,     3,     1,     2,     2,
       1,     1,     1,     1,     1,     1,     1,     4,     4,     4,
       4,     1,     1,     1,     1,     3,     1,     3,     1,     3,
       0,     4,     0,     6,     3,     0,     4,     1,     3,     3,
       0,     4,     3,     0,     1,     1,     2,     6,     1,     3,
       0,     1,     4,     6,     4,     1,     1,     1,     1,     1,
       3,     0,     2,     1,     2,     3,     4,     1,     1,     3,
       4,     3,     5,     3,     3,     3,     0,     3,     3,     3,
       0,     2,     2,     0,     6,     0,     5,     2,     2,     2,
       0,     0,     0,    11,     1,     0,     1,     0,     1,     1,
       2,     2,     2,     2,     2,     2,     3,     4,     3,     2,
       3,     4,     1,     2,     1,     2,     2,     2,     2,     3,
       3,     2,     0,     2,     3,     1,     4,     1,     3,     1,
       1,     2,     2,     3,     3,     0,     1,     3,     2,     2,
       1,     2,     2,     2,     2,     2,     2,     2,     1,     2,
       2,     3,     1,     2,     2,     4,     4,     2,     1,     5,
       4,     1,     0,     1,     3,     0,     1,     3,     1,     1,
       1,     1,     4,     4,     4,     4,     4,     3,     4,     4,
       4,     4,     3,     2,     1,     1,     3,     1,     3,     2,
       1,     6,     0,     2,     1,     2,     1,     2,     3,     3,
       1,     3,     1,     2,     3,     3,     2,     2,     3,     1,
       4,     4,     3,     3,     2,     1,     1,     2,     1,     1,
       2,     2,     1,     2,     3,     3,     2,     2,     3,     1,
       3,     3,     2,     2,     3,     1,     4,     3,     4,     3,
       1,     2,     2,     2,     2,     2,     2,     2,     1,     2,
       4,     4,     2,     1,     1,     1,     1,     2,     4,     3,
       3,     2,     2,     2,     2,     1,     2,     2,     2,     2,
       3,     1,     2,     3,     4,     2,     2,     2,     2,     2,
       2,     4,     2,     1,     2,     2,     3,     1,     3,     2,
       3,     2,     2,     3,     1,     3,     4,     1,     2,     3,
       2,     2,     1,     3,     2,     2,     1,     2,     3,     1,
       3,     6,     4,     4,     3,     5,     3,     3,     3,     2,
       1,     1,     2,     2,     2,     0,     1,     1,     2,     3,
       0,     4,     1,     0,     0,     5,     1,     0,     3,     1,
       2,     1,     2,     0,     4,     1,     0,     0,     5,     0,
       0,     7,     0,     0,     0,     0,    12,     0,     0,     7,
       0,     5,     0,     7,     0,     4,     2,     2,     2,     3,
       6,     8,    10,     8,    12,    10,    10,     4,     3,     2,
       2,     1,     1,     1,     1,     1,     0,     0,     5,     0,
       0,     5,     1,     2,     0,     0,     0,     5,     1,     1,
       3,     3,     2,     2,     2,     2,     2,     1,     2,     0,
       1,     0,     1,     1,     0,     1,     1,     3,     4,     7,
       1,     3,     0,     1,     1,     1,     2,     2,     2,     1,
       2,     2,     0,     3,     1,     1,     1,     2,     2,     2,
       4,     2,     2,     2,     2,     2,     2,     1,     2,     1,
       2,     1,     1,     0,     0,     1,     5,     3,     0,     3,
       0,     0,     4,     2,     1,     1,     1,     3,     0,     3,
       3,     3,     1,     0,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     4,     3,     3,
       3,     3,     4,     3,     3,     5,     5,     4,     3,     0
};

/* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
   doesn't specify something else to do.  Zero means the default is an
   error. */
static const short yydefact[] =
{
       3,    12,    12,     5,     0,     4,     0,   314,   673,   674,
       0,   419,   435,   614,     0,    11,   433,     0,     0,    10,
     519,   892,     0,     0,     0,   177,   707,    16,   315,   316,
      88,     0,     0,   873,     0,    47,     0,     0,    13,    27,
       0,    29,     8,    52,    53,     0,    18,    15,    95,   118,
      92,     0,   675,   181,   335,   312,   336,   649,     0,   408,
       0,   407,     0,   423,     0,   448,   616,   465,   434,     0,
     532,   534,   514,   542,   418,   638,   436,   639,   116,   334,
     660,   636,     0,   676,   612,     0,    89,     0,   313,    85,
      87,    86,   192,     0,   681,   192,   682,   192,   317,   177,
     150,   151,   152,   153,   154,   505,   507,     0,   703,     0,
     508,     0,     0,     0,   151,   152,   153,   154,    25,     0,
       0,     0,     0,     0,     0,     0,   509,   685,     0,   691,
       0,     0,     0,    39,     0,     0,    33,     0,     0,    49,
       0,     0,   192,   683,   192,   314,   616,     0,   647,   642,
       0,     0,     0,   646,     0,     0,     0,     0,   335,     0,
     326,     0,     0,     0,   334,   612,    30,     0,    28,     3,
      48,     0,    68,   419,     0,     0,     8,    71,    67,    70,
      95,     0,     0,     0,   434,    96,    14,     0,   463,     0,
       0,   481,    93,    83,   684,   620,     0,     0,   612,    84,
       0,     0,     0,   114,     0,   444,   398,   629,   399,   635,
       0,   612,   421,   420,    82,   117,   409,     0,   446,   422,
     115,     0,   415,   441,   442,   410,   425,   427,   430,   443,
       0,    79,   466,   520,   521,   522,   523,   541,   159,   158,
     160,   525,   533,   182,   529,   524,     0,     0,   535,   536,
     537,   538,   873,     0,   615,   424,   617,     0,   460,   314,
     674,     0,   315,   705,   181,   666,   667,   663,   641,   677,
       0,   314,   316,   662,   640,   661,   637,     0,   893,   893,
     893,   893,   893,   893,   893,     0,   893,   893,   893,   893,
     893,   893,   893,   893,   893,   893,   893,   893,   893,   893,
     893,   893,   893,   893,   893,   893,   893,   893,     0,   893,
     818,   423,   819,   888,   317,     0,   189,   190,     0,   880,
       0,     0,   192,     0,   517,   503,     0,     0,     0,   704,
     702,   614,   339,   342,   341,   431,   432,     0,     0,     0,
     385,   383,   356,   387,   388,     0,     0,     0,     0,     0,
     284,     0,     0,   200,   199,     0,   201,   202,     0,     0,
     203,     0,     0,     0,   204,   258,     0,   261,   205,   338,
     226,     0,     0,   340,     0,     0,   405,     0,     0,   423,
     406,   668,   368,   358,     0,     0,   470,     3,    23,    31,
     699,   695,   696,   698,   700,   697,   150,   151,   152,     0,
     153,   154,   687,   688,   692,   689,   686,     0,   314,   324,
     325,   323,   665,   664,    35,    34,    51,     0,   167,     0,
       0,   423,   165,    17,     0,     0,   192,   643,   617,   645,
       0,   644,   151,   152,   310,   311,   330,   616,     0,   653,
     329,     0,   652,     0,   337,   315,   316,     0,     0,     0,
     328,   327,   657,     0,     0,    12,     0,   177,     9,     9,
      74,     0,    69,     0,     0,    75,    78,     0,   462,   464,
     132,   101,   806,    99,   389,   100,   135,     0,     0,   133,
      94,     0,   849,   225,     0,   224,   844,   867,     0,   405,
     423,   406,     0,   843,   845,   874,   856,     0,     0,   659,
       0,     0,   881,   616,     0,   627,   622,     0,   626,     0,
       0,     0,     0,     0,   612,   463,     0,    81,     0,   612,
     634,     0,   412,   413,     0,    80,   463,     0,     0,   417,
     416,   411,   428,   429,   450,   449,   192,   539,   540,   150,
     153,   526,   530,   528,     0,   543,   510,   426,   463,   679,
     612,   102,     0,     0,     0,     0,   680,   612,   108,   613,
       0,   648,   674,   706,   181,   928,     0,   924,     0,   923,
     921,   903,   908,   909,   893,   915,   914,   900,   901,   899,
     918,   907,   904,   905,   906,   910,   911,   897,   898,   894,
     895,   896,   920,   912,   913,   902,   919,   893,   916,   425,
     612,   612,     0,   612,     0,   893,   192,   186,   187,   332,
     192,   315,   308,   193,   286,   197,   194,     0,     0,     0,
       0,   185,   185,     0,   175,     0,   423,   173,   518,   607,
     604,     0,   517,   605,   517,     0,     0,   248,   249,     0,
       0,     0,     0,     0,     0,   285,   232,   229,   228,   230,
       0,     0,     0,     0,     0,   338,     0,   929,     0,   227,
     231,   439,     0,     0,     0,   259,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   349,
       0,   351,   354,   355,   391,   390,     0,     0,     0,     0,
       0,   237,   610,     0,   245,   382,     0,     0,   873,   371,
     374,   375,     0,     0,   440,   400,   726,   722,     0,     0,
     612,   612,   612,   402,   729,     0,   233,     0,   235,     0,
     672,   404,     0,     0,   403,   370,     0,   365,   384,   366,
     386,   669,     0,   367,   476,   477,   478,   475,     0,   468,
     471,     0,     3,     0,   690,   192,   693,     0,    43,    44,
       0,    57,     0,     0,     0,    61,    65,    54,   872,   423,
      57,   871,    63,   178,   163,   161,   178,   185,   333,     0,
     651,   650,   337,     0,   654,     0,    20,    22,    95,     9,
       9,    77,    76,     0,   138,   136,   929,    91,    90,   487,
       0,   483,   482,     0,   621,   618,   848,   862,   851,   726,
     722,     0,   863,   612,   866,   868,     0,     0,   864,     0,
     865,   619,   847,   861,   850,   846,   875,   858,   869,   859,
     852,   857,   658,     0,   672,     0,   656,   623,   617,   625,
     624,   616,     0,     0,     0,     0,     0,     0,   612,   633,
       0,   458,   457,   445,   632,     0,   881,     0,   628,   414,
     447,   459,   437,   438,   463,     0,   527,   531,   673,   674,
     873,   873,   675,   544,   545,   547,   873,   550,   549,     0,
       0,   461,   881,   842,   192,   192,   678,   192,   881,   842,
     612,   105,   612,   111,   893,   893,   917,   922,   888,   888,
     888,     0,   927,     0,   191,   309,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   196,     0,
     876,   180,   184,   318,   178,   171,   169,   178,     0,   506,
     518,   603,     0,     0,     0,     0,     0,     0,   423,     0,
       0,     0,   345,     0,   343,   344,     0,     0,   256,   223,
     222,   314,   673,   674,   315,   316,     0,     0,   488,   515,
       0,   221,   220,   283,   282,   833,   832,     0,   280,   279,
     277,   278,   276,   275,   274,   271,   272,   273,   269,   270,
     264,   265,   266,   267,   268,   262,   263,     0,     0,     0,
       0,     0,     0,     0,   239,   253,     0,     0,   238,   612,
     612,     0,   612,   609,   714,     0,     0,     0,     0,     0,
     373,     0,   377,     0,   379,     0,   616,   725,   724,   717,
     721,   720,   872,     0,     0,   739,     0,     0,   881,   401,
     881,   727,   612,   842,     0,     0,     0,   726,   722,     0,
       0,   612,     0,   616,     0,     0,     0,     0,   470,     0,
       0,    26,     0,     0,   694,     0,    40,    46,    45,    59,
      56,    49,    57,     0,    50,     0,   192,    58,   525,     0,
     168,   178,   178,   166,   179,   332,   331,    19,    21,    73,
      95,   451,   807,     0,     0,   484,     0,   134,   616,   725,
     721,   726,   722,     0,   616,   636,     0,   612,   727,     0,
     726,   722,     0,   338,     0,   668,     0,   870,     0,     0,
     883,     0,     0,     0,     0,   455,   631,   630,   454,   185,
     552,   551,   873,   873,   873,     0,   578,   674,     0,   568,
       0,     0,     0,   581,     0,   131,   126,     0,   181,   582,
     585,     0,     0,   560,     0,   129,   572,   104,     0,     0,
       0,     0,   110,     0,   881,   842,   881,   842,   926,   925,
     890,   889,   891,   319,   307,   306,     0,   304,   303,   301,
     302,   300,   299,   298,   296,   297,   294,   295,   289,   290,
     291,   292,   293,   287,   288,   198,   879,   176,   178,   178,
     174,   608,   606,   504,   357,     0,   363,   364,     0,     0,
       0,     0,   344,   347,   750,     0,     0,     0,     0,   257,
       0,   348,   350,   353,   251,   250,   241,     0,   240,   255,
       0,     0,   711,   709,     0,   712,     0,   246,     0,     0,
     192,   380,     0,     0,     0,   718,   617,   723,   719,   730,
     612,   738,   736,   737,     0,   728,   881,     0,   734,     0,
     234,   236,   670,   671,   726,   722,     0,   369,   469,   467,
     314,     0,    24,    32,   701,    60,    55,    62,    66,    64,
     164,   162,    72,   814,   149,   155,   156,   157,     0,     0,
     140,   144,   145,   146,    97,     0,   485,   617,   725,   721,
     726,   722,     0,   612,   641,   727,     0,     0,   671,   365,
     366,   669,   367,   860,   854,   855,   853,   885,   884,   886,
       0,     0,     0,     0,   617,     0,     0,   452,   183,     0,
     554,   553,   548,   612,   842,   577,     0,   569,   582,   570,
     463,   463,   566,   567,   564,   565,   612,   842,   314,   673,
       0,   450,   127,   573,   583,   588,   589,   450,   450,     0,
       0,   450,   125,   574,   586,   450,     0,   463,     0,   561,
     562,   563,   463,   612,   321,   320,   322,   612,   107,     0,
     113,     0,     0,   172,   170,     0,     0,     0,     0,     0,
     745,     0,   491,     0,   489,   260,   281,     0,   242,   243,
     252,   254,   710,   708,   715,   713,     0,   247,     0,     0,
     372,   376,   378,   881,   732,   612,   733,     0,   472,   474,
     815,   808,   812,   142,     0,   148,     0,   745,   486,   725,
     721,     0,   727,   344,     0,   882,   616,   456,     0,   546,
     881,     0,     0,   571,   481,   481,   881,     0,     0,     0,
     463,   463,     0,   463,   463,     0,   463,     0,   559,   511,
       0,   481,   881,   881,   612,   612,   305,   352,     0,     0,
       0,     0,     0,   216,   751,     0,   746,   747,   490,     0,
       0,   244,   716,   381,   320,   735,   881,     0,     0,   813,
     141,     0,    98,   726,   722,     0,   617,     0,   887,   453,
     121,   612,   612,   842,   576,   580,   124,   612,   463,   463,
     597,   481,   314,   673,     0,   584,   590,   591,   450,   450,
     481,   481,     0,   481,   587,   500,   575,   103,   109,   881,
     881,   359,   360,   361,   362,   479,     0,     0,     0,   741,
     752,   759,   740,     0,   748,   492,   611,   731,   473,     0,
     816,   147,   616,   881,   881,     0,   881,   596,   593,   595,
       0,     0,   463,   463,   463,   592,   594,   579,     0,   106,
     112,     0,   749,   744,   219,     0,   217,   743,   742,   314,
     673,   674,   753,   766,   769,   772,   777,     0,     0,     0,
       0,     0,     0,     0,     0,   315,   801,   809,     0,   829,
     805,   804,   803,     0,   761,     0,     0,   423,   765,   760,
     802,   929,     0,     0,   929,   119,   122,   612,   123,   463,
     463,   602,   481,   481,   502,     0,   501,   496,   480,   218,
     822,   824,   825,     0,     0,   757,     0,     0,     0,   784,
     786,   787,   788,     0,     0,     0,     0,     0,     0,     0,
     823,   929,   397,   830,     0,   762,   395,   450,     0,   396,
       0,   450,     0,     0,   763,   800,   799,   820,   821,   817,
     881,   601,   599,   598,   600,     0,     0,   513,   208,     0,
     754,   767,   756,     0,   929,     0,     0,     0,   780,   929,
     789,     0,   798,    41,   154,    36,   154,     0,    37,   810,
       0,   393,   394,     0,     0,     0,   392,   757,   120,   499,
     498,    92,    95,   215,     0,   423,     0,   757,   757,   770,
       0,   745,   827,   773,     0,     0,     0,   929,   785,   797,
      42,    38,   814,     0,   764,     0,   497,   209,   450,   755,
     768,     0,   758,   828,     0,   826,   778,   782,   781,   811,
     834,   834,     0,   495,   493,   494,   463,   206,     0,     0,
     212,     0,   211,   757,   929,     0,     0,     0,   835,   836,
       0,   790,     0,     0,   771,   774,   779,   783,     0,     0,
       0,     0,     0,     0,   834,     0,   213,   207,     0,     0,
       0,   840,     0,   793,   837,     0,     0,   791,     0,     0,
     838,     0,     0,     0,     0,     0,     0,   214,   775,     0,
     841,   795,   796,     0,   792,   757,     0,     0,   776,   839,
     794,     0,     0,     0
};

static const short yydefgoto[] =
{
    1821,   455,     2,   456,   171,   787,   361,   187,     3,     4,
      38,   141,   752,   387,    39,   753,  1143,  1601,    41,   407,
    1648,   757,    42,    43,   417,    44,  1144,   764,  1069,   765,
     766,   767,    46,   178,   179,    47,   796,   190,   186,   473,
    1427,    48,    49,   883,  1165,   889,  1167,    50,  1146,  1147,
     191,   192,   797,  1093,   474,  1288,  1289,  1290,   629,  1291,
     242,    51,  1082,  1081,   776,   773,  1199,  1198,   937,   934,
     140,  1080,    52,   244,    53,   931,   609,   315,   316,   317,
     318,   613,   362,   654,  1759,  1680,  1761,  1714,  1798,  1474,
     364,  1050,   365,   700,  1008,   366,   367,   368,   615,   369,
     322,    55,   266,   758,   436,   160,    56,    57,   370,   657,
     371,   372,   373,   798,   374,  1604,   534,   721,  1032,  1149,
     487,   225,   488,   489,   226,   377,   378,    62,   501,   227,
     204,   217,    64,   515,   535,  1438,   851,  1326,   205,   218,
      65,   548,   852,    66,    67,   748,   749,   750,  1536,   479,
     968,   969,  1712,  1677,  1626,  1568,    68,   634,   324,   880,
    1525,  1627,  1218,   630,    69,    70,    71,    72,    73,   253,
     873,   874,   875,   876,  1151,  1368,  1152,  1153,  1154,  1353,
    1363,  1354,  1515,  1355,  1356,  1516,  1517,   631,   632,   633,
     701,  1038,   491,   198,   513,   506,   207,    75,    76,    77,
     148,   149,   163,    79,   136,   381,   382,   383,    81,   384,
      83,   878,   127,   128,   129,   554,   110,    84,   385,  1013,
    1014,  1033,  1029,   724,  1538,  1539,  1475,  1476,  1477,  1540,
    1390,  1541,  1608,  1633,  1717,  1683,  1684,  1542,  1609,  1707,
    1634,  1718,  1635,  1741,  1636,  1744,  1788,  1815,  1637,  1763,
    1727,  1764,  1689,   475,   794,  1283,  1610,  1651,  1732,  1421,
    1422,  1488,  1614,  1716,  1550,  1611,  1723,  1654,   977,  1767,
    1768,  1769,  1792,   492,  1034,   831,  1119,  1316,   494,   495,
     496,   827,   497,   154,   829,  1156,    93,   620,   836,  1319,
    1320,   605,    87,   565,    88,   957
};

static const short yypact[] =
{
     147,   205,-32768,-32768,  5953,-32768,   195,   122,   327,   370,
     168,   315,-32768,-32768,  1752,-32768,-32768,   221,   287,-32768,
  -32768,-32768,  1820,  1020,   755,   233,-32768,-32768,   310,   458,
  -32768,  2748,  2748,-32768,  2954,-32768,  5953,   322,-32768,-32768,
     388,-32768,    42,-32768,-32768,  3465,-32768,-32768,   345,   761,
     435,   422,   449,-32768,-32768,-32768,-32768,   360,   513,-32768,
    7624,-32768,   417,  2020,   650,-32768,   489,-32768,-32768,  1925,
     497,   558,-32768,   479,  4523,-32768,-32768,-32768,   629,-32768,
  -32768,-32768,  2397,-32768,-32768,  1339,-32768, 10626,   470,-32768,
  -32768,-32768,   617,   592,-32768,   617,-32768,   617,-32768,-32768,
  -32768,   327,   370,   310,   597,-32768,   553,   449,-32768,  1918,
  -32768,   463, 11675,   564,-32768,-32768,-32768,-32768,-32768,    62,
     599,   693,   710,   725,   625,   665,-32768,-32768,  2419,-32768,
    1548,   327,   370,-32768,   310,   597,-32768,  1578,  2664,   658,
    7982,   689,   617,-32768,-32768,   677,  3514,  3358,-32768,-32768,
    2493,  2690,  3358,-32768,  2132,  5108,  5108,  2954,   663,   694,
  -32768,   360,  1444,   697,   716,-32768,-32768,   817,-32768,   721,
  -32768,  4172,-32768,-32768,   233,  2033,   777,-32768,-32768,-32768,
     345,  5722,  8147,   862,   790,-32768,-32768,   763,   489,   878,
     119,   627,   820,-32768,-32768,-32768,  8838, 10720,-32768,-32768,
    5309,  5309,  7191,   629,   932,-32768,-32768,   431,-32768,-32768,
    2821,-32768,-32768,-32768,-32768,-32768,  2020,   967,-32768,   489,
     629, 11675,-32768,-32768,-32768,  1052,  2020,-32768,   489,-32768,
    5722,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,   808,   560,   449,-32768,   489,  2482,  2358,-32768,-32768,
  -32768,-32768,-32768,   840,-32768,  2189,   489,   463,-32768,   753,
     736,  1729,   756,-32768,    64,-32768,-32768,-32768,-32768,-32768,
    5591,-32768,   597,-32768,-32768,-32768,-32768,  2905,-32768,   824,
     835,-32768,-32768,-32768,-32768,   870,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,   829,-32768,
  -32768,  2189,  4523,   902,-32768,   691,   892,-32768, 11768,   865,
     691,   691,   617,  7982,  1126,-32768,   909,  2557,   756,-32768,
  -32768,   879,-32768,-32768,-32768,-32768,-32768, 12885, 12885,   893,
  -32768,-32768,-32768,-32768,-32768,   896,   920,   924,   926,   928,
   12233,  2557, 12885,-32768,-32768, 12885,-32768,-32768, 12885,  9435,
  -32768, 12885, 12885,   897,   950,-32768, 12326,-32768,  9045,   172,
    1138,  4515, 12419,-32768,  2973,   911,  1123, 12978, 13071,  2499,
    6531,-32768,   237,-32768,  3735,  4417,  2515,   721,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,   599,   693,   710,  2557,
     725,   625,   957,   665,-32768,   987,-32768,  1450,   909,   327,
     370,-32768,-32768,-32768,-32768,-32768,-32768,  5433,-32768,  5722,
    5531,  3216,-32768,-32768,   691,   782,   617,-32768,  3514,-32768,
    3183,-32768,   964,   996,-32768,-32768,-32768,  1444,  3358,-32768,
  -32768,  3358,-32768,   943,-32768,-32768,-32768,  1444,  1444,  1444,
  -32768,-32768,-32768,  5591,   954,   959,   974,-32768,-32768,-32768,
  -32768,  7982,-32768,   976,  1006,-32768,-32768,  1063,-32768,   489,
  -32768,-32768,-32768,-32768,  1018,-32768,-32768, 10091, 12233,-32768,
  -32768,   979,-32768,   950,   983,  9045,   152,  1812,  8147,  1812,
    3870,  7172,   991,-32768,   284,  5854,  1030,  1072,   879,-32768,
    1013,   250,    91,  7415,  6592,-32768,-32768,  6592,-32768,  6828,
    6828,  7191,  7685,  1004,-32768,   489,  5722,-32768, 10813,-32768,
  -32768,  7300,  1052,  2020,  5722,-32768,   489,  1032,  1036,-32768,
  -32768,  1052,-32768,   489,  1112,-32768,  1074,-32768,-32768,   909,
     756,   808,-32768,-32768,  2482,  2241,-32768,  2189,   489,-32768,
  -32768,-32768,  1073,  1077,  1100,  1080,-32768,-32768,-32768,-32768,
    3514,-32768,   872,-32768,   391,-32768,  1048,-32768,  1055,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,  2189,
  -32768,-32768,   398,-32768,   585,-32768,   617,-32768,-32768,  1168,
  -32768,  1096, 12513,-32768,-32768,  8550,-32768,  3850,  4697, 12233,
    1115,-32768,-32768,   691,-32768,  5722,  4000,-32768,-32768,  1114,
  -32768,  1081,  1129,-32768,  1126,   957, 12233,-32768,-32768, 12233,
   11675,  6983,  6983,  6983,  6983, 13163,-32768,-32768,-32768,-32768,
    1084, 12606, 12606,  9435,  1087,   175,  1089,-32768,  1090,-32768,
  -32768,-32768, 10370,  9528,  9435,-32768, 10463, 12233, 12233, 10184,
   12233, 12233, 12233, 12233, 12233, 12233, 12233, 12233, 12233, 12233,
   12233, 12233, 12233, 12233, 12233, 12233, 12233, 12233, 12233,-32768,
   12233,-32768,-32768,-32768,-32768,-32768, 12233, 12233, 12233, 11675,
    4954,   681,   410, 10906,-32768,-32768,  1156,  1729,  1206,   393,
     460,   505,  4152,   585,-32768,-32768,   843,   843,  5205, 10999,
    1121,  1173,-32768,-32768,   636,  9435,-32768,  9435,-32768, 11393,
     955,-32768,  1668,   463,-32768,-32768, 12233,-32768,-32768,-32768,
  -32768,-32768,    77,   470,-32768,-32768,-32768,-32768,   126,-32768,
    1122,  1113,   721,  1450,  1167,   617,-32768,  1162,-32768,-32768,
    2664,  2601,  1153,  1199,   509,  1174,  1176,-32768,-32768,  4613,
    1925,-32768,  1178,-32768,-32768,-32768,-32768,-32768,-32768,   691,
  -32768,-32768,  1136,  1140,-32768,  1188,-32768,-32768,   345,-32768,
  -32768,-32768,-32768,  1145,-32768,-32768,-32768,-32768,-32768,-32768,
    6118, 13163,-32768,  1147,-32768,-32768,-32768,-32768,-32768,  2211,
    2211,  6235,-32768,-32768,-32768,-32768,  2821,  1339,-32768, 11487,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,  1072,  1191,
  -32768,-32768,-32768, 11861,  1173,   535,-32768,-32768,  7415,-32768,
  -32768,  7685,  6592,  6592,  8154,  8154,  7685,  1668,-32768,-32768,
    7300,-32768,  1194,-32768,-32768,  1154,    91,  7415,-32768,  1052,
  -32768,-32768,-32768,-32768,   489,  1201,   808,-32768,   693,   710,
  -32768,-32768,   665,  1220,-32768,-32768,   191,-32768,-32768,  2854,
    4307,-32768,    91,  6367,   617,   617,-32768,   617,    91,  6367,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,  2704,  2704,
    2704,  1687,-32768,   691,-32768,  8550, 12513, 12513, 10184, 12513,
   12513, 12513, 12513, 12513, 12513, 12513, 12513, 12513, 12513, 12513,
   12513, 12513, 12513, 12513, 12513, 12513, 12513,  1729,   310,  1182,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768, 12233,-32768,
    2557,-32768,  1155,  1183, 13118,  1184,  1185,  1208,  3686,  1224,
    1225,  1228,-32768,  1197,-32768,-32768,  1198,  1248,-32768,-32768,
   13163,  1246,   780,   786,    53,   387, 12233,  1247,-32768,  1253,
    1209,-32768, 13163, 13163, 13163,-32768,-32768,  1251,  8437,  8415,
    8488,  6974,  3567,  5996,  6259,  3405,  3405,  3405,  1359,  1359,
    1049,  1049,   600,   600,   600,-32768,-32768,  1210,  1213,  1205,
    1215,  1214,  1219,  6983,   681,-32768, 10091, 12233,-32768,-32768,
  -32768, 12233,-32768,-32768,  1234, 12885,  1221,  1245,  1260,  1289,
  -32768, 12233,-32768, 12233,-32768, 12233,  1022,  1204,-32768,-32768,
    1204,-32768,   144,  1229,  1230,-32768,  1237,  6983,    91,-32768,
      91,  2264,-32768,  6367, 11092,  1231,  1241,  8990,  8990,  6685,
    1242, 12326,  1250,  1906,  3800,  4417,  1995,  1256,  2515,  1258,
   12699,-32768,  1244,  1294,-32768,   691,-32768,-32768,-32768,-32768,
  -32768,-32768,  3623,  5433,-32768,  6983,-32768,-32768,   985, 12513,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
     345,-32768,-32768,   779,  1302,-32768,    57,-32768,  2099,  2273,
    2273,  2377,  2377,  6235,  3296,   325,  2821,-32768,  2507,  3892,
   11581, 11581,  8687,   381,  1266,   443,  1586,-32768, 10091,  9624,
  -32768,  2118,   532,   532,  1888,-32768,-32768,-32768,  1303,-32768,
  -32768,-32768,-32768,-32768,-32768,  2806,-32768,   937,   547,-32768,
   12233,  8212,  6403,-32768,  6403,   220,   220,   367,   569,  5082,
    4717,    66,  5089,-32768,   178,   220,-32768,-32768,  1268,   691,
     691,   691,-32768,  1271,    91,  6367,    91,  6367,-32768,-32768,
  -32768,-32768,-32768,-32768,  8550,  8550,  1322,  8372,  5634,  6030,
    7410,  4203,  6059,  6230,  4003,  4003,  1566,  1566,  1186,  1186,
    1001,  1001,  1001,-32768,-32768,  1077,-32768,-32768,-32768,-32768,
  -32768, 13163,-32768,-32768,-32768,  6983,-32768,-32768,  1295,  1296,
    1297,  1299,  1136,-32768,-32768,  8463, 10091,  9719,  1285,-32768,
   12233,-32768,-32768,-32768,-32768,-32768,   605,  1287,-32768,-32768,
    1300,   186,   863,   863,  1291,   863, 12233,-32768, 12885,  1397,
     617,-32768,  1305,  1306,  1308,-32768,  1022,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,  1022,-32768,    91,  1313,-32768,  1311,
  -32768,-32768,-32768,-32768,  3087,  3087,  6889,-32768,-32768,-32768,
     153,  1315,-32768,-32768,-32768,-32768,-32768,-32768,-32768,  8550,
  -32768,-32768,-32768,  1329,-32768,   599,   725,   625,   101,   575,
  -32768,-32768,-32768,-32768,-32768,  9812,-32768,  2099,  2273,  2273,
    3212,  3212,  7075,-32768,   521,  2507,  2099,  1327,   483,   613,
     624,   754,   266,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
     199,  3384,  3384,  3472,  3472,  3472, 10091,-32768,-32768,  2241,
  -32768,-32768,-32768,-32768,  6367, 13163,   303,-32768,  2140,-32768,
     489,   489,-32768,-32768,-32768,-32768,-32768,  6367,   145,   852,
   12233,  1112,-32768,  1351,-32768,-32768,-32768,   413,   437,  2397,
    2690,   537,   220,  1370,-32768,   649,  1383,   489,  5494,-32768,
  -32768,-32768,   489,-32768,-32768,  1393,-32768,-32768,-32768,  1347,
  -32768,  1350, 12513,-32768,-32768,  1360, 12233, 12233, 12233, 12233,
      61, 10091,-32768,  1396,-32768,-32768, 13163, 12233,-32768,   605,
  -32768,-32768,-32768,-32768,-32768,-32768,  1364,-32768,  1423,   691,
  -32768,-32768,-32768,    91,-32768,-32768,-32768, 12233,-32768,-32768,
  -32768,  1329,-32768,-32768,   351,-32768, 12233,    61,-32768,  2631,
    2631,  1668,  3062,   770,  2118,-32768,  3472,-32768, 10091,-32768,
      91,  1363,   607,-32768,  1412,  1412,    91,  1371, 12233, 12233,
    8338,   489,  5264,   489,   489,  4628,   489,  6779,-32768,-32768,
    5783,  1412,    91,    91,-32768,-32768,  8550,-32768,  1376,  1381,
    1382,  1386,  2557,-32768,-32768,  9247,  1467,-32768,-32768, 10091,
    1394,-32768,-32768,-32768,-32768,-32768,    91,  1398,  1416,-32768,
  -32768,  1402,-32768,  3688,  3688,  5607,  1379,  1379,-32768,-32768,
  -32768,-32768,-32768,  6367,-32768,-32768,-32768,-32768,  8338,  8338,
  -32768,  1412,   146,   931, 12233,-32768,-32768,-32768,  1112,  1112,
    1412,  1412,   872,  1412,-32768,-32768,-32768,-32768,-32768,    91,
      91,-32768,-32768,-32768,-32768,-32768,  1045,   257,  9135,-32768,
  -32768,-32768,-32768, 11203,-32768,-32768,-32768,-32768,-32768,  6625,
  -32768,-32768,  1379,    91,    91,  1403,    91,-32768,-32768,-32768,
   12233, 12233,  8338,   489,   489,-32768,-32768,-32768,  4828,-32768,
  -32768,  2557,-32768,-32768,-32768,   260,-32768,-32768,-32768,  1449,
     933,   972,-32768,-32768,-32768,-32768,-32768, 12233,  1451,  1452,
    1455, 11954,   577,  2557,   917,   850,-32768,-32768, 12047,  1510,
  -32768,-32768,-32768,  1457,-32768,  4383,  7936,  7718,  1507,-32768,
  -32768,  1415,  1418,  1419,-32768,-32768,-32768,-32768,-32768,  8338,
    8338,-32768,  1412,  1412,-32768, 10556,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,   701,   701,  1469,  1437,  1438,  8264,-32768,
  -32768,-32768,-32768,  1474, 12233,  1475,  1479,  1491,  2864,  2869,
  -32768,-32768,-32768,-32768,  1454,-32768,-32768,  1112,  1047,-32768,
    1050,  1112, 12140,  1059,-32768,-32768,-32768,-32768,-32768,-32768,
      91,-32768,-32768,-32768,-32768,  1441,  8888,  1458,-32768, 11675,
  -32768,-32768,-32768,  1537,-32768,  9342, 11675, 12233,-32768,-32768,
  -32768,  1493,-32768,-32768,  1502,-32768,  1491,  2864,-32768,-32768,
    1551,-32768,-32768, 12792, 12792,  9905,-32768,  1469,-32768,-32768,
  -32768,   435,   345,-32768,  1456,   759,  5722,  1469,  1469,-32768,
   11298,    61,-32768,-32768,  1504,  1463, 13141,-32768,-32768,-32768,
  -32768,-32768,  1329,   419,-32768,   201,-32768,-32768,  1112,-32768,
  -32768,   703,-32768,-32768,  9998,-32768,-32768,-32768,-32768,  1329,
      82,    82,  1511,-32768,-32768,-32768,   489,-32768, 12233,  1517,
  -32768,  1523,-32768,  1469,-32768,  1470,  2557,   334,  1525,-32768,
     512,-32768,  1521,  1485,-32768,-32768,-32768,-32768, 12233,  1486,
    1588,  1547,    82,  1588,    82,  1555,-32768,-32768, 10277,  1501,
    1602,-32768,   263,-32768,-32768,   286,   475,-32768, 10091,  1514,
  -32768,  1530,  1608,  1567,  1573,  1588,  1577,-32768,-32768, 12233,
  -32768,-32768,-32768,   295,-32768,  1469,  1531,  1593,-32768,-32768,
  -32768,  1657,  1660,-32768
};

static const short yypgoto[] =
{
  -32768,  1662,-32768,  -325,  1487,  -395,    25,    -3,  1663,-32768,
    1632,-32768,-32768,-32768, -1463,-32768,   304,-32768, -1451,-32768,
      26,   921,    43,  -385,-32768,-32768,   149,-32768,  -705,-32768,
  -32768,   606,    38,  1512,  1227,  1515,-32768,   -17,  -176,  -785,
  -32768,   -11,   -58,-32768,-32768,-32768,-32768,-32768,   545,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,   258,   -14,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
    1598,  -691,  7680,  -175,   -46,  -569,  -290,   131,  1556,  -573,
  -32768,-32768,-32768,   215,-32768,    65,-32768, -1588,-32768, -1355,
    1389,   -70,  -326,-32768,  -954,  7558,  3682,  6841,  3227,  5251,
    1391,  -365,   -56,  -110,  1263,  -123,   -61,   137,-32768,-32768,
  -32768,  -344,-32768,-32768,-32768, -1488,    10,  -353,  2176,    21,
      11,  -139,    18,    63,  -210,-32768,-32768,-32768,    -1,   -98,
    -171,  -177,     1,   -30,  -236,-32768,  -410,-32768,-32768,-32768,
  -32768,-32768,    13,  1343,  2724,-32768,   651,-32768,-32768, -1185,
    -439,   907,-32768,-32768,-32768,-32768,    52,-32768,-32768,-32768,
  -32768,-32768,-32768,  1082,  -399,-32768,-32768,-32768,-32768,-32768,
  -32768,   384,   580,-32768,-32768,-32768,   348, -1068,-32768,-32768,
  -32768,-32768,-32768,-32768,   570,-32768,   264,  1091,-32768,   784,
    1029,  2591,   100,  1529,  2691,  1177,-32768,  -534,-32768,   140,
      90,   801,  -132,   173,   -95,  5765,  1356,-32768,  6769,  2565,
    2184,   -16,  -114,-32768,  1611,   -49,-32768,  6300,  3924,  -930,
  -32768,  2324,   993,-32768,-32768,   202,-32768,-32768,   268,  1093,
  -32768, -1468,-32768,-32768,-32768, -1611,-32768, -1445,    33,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,    19,-32768,-32768,-32768,-32768,-32768,    27,
   -1361,-32768,-32768,   -29,-32768,-32768,-32768,-32768,  -891, -1642,
  -32768,   -22, -1662,  -814,  -154,   938,-32768,-32768,-32768,-32768,
    -405,-32768,  -404,  -238,-32768,   108,-32768,-32768,   788,   331,
  -32768,   157,-32768,  4206,  -212,  -718
};


#define	YYLAST		13250


static const short yytable[] =
{
     106,    37,   215,    63,   462,   464,   523,   126,   119,  1092,
     463,   877,   771,   772,   545,    59,   406,  1176,   770,   188,
     739,   267,    60,   439,   442,    58,   730,   413,   414,    36,
     621,   622,   763,    37,   180,    63,   264,   904,   802,   451,
     665,   740,   493,   415,    63,   547,   183,    59,   258,   555,
    1228,   726,   728,   932,    60,   241,    59,    58,   313,   216,
    1489,    36,   751,   182,   788,  1077,   181,    61,   203,  1158,
     175,   542,  1492,   255,  1337,  1163,  1339,   522,  1094,   405,
    1600,   412,   418,   177,  1369,  1083,   311,   531,   176,   267,
     771,   828,  1602,  1578,  1765,  1472,  1734,   184,  1725,    61,
     559,   599,  1423,   169,    74,  1366,  1739,  1740,    61,  1770,
    1652,   379,    86,   557,   402,   229,   861,   340,  -153,  1295,
     471,  1795,   153,   388,   215,  -878,   484,   530,   532,   389,
     142,   147,   152,    26,   777,   778,    74,   730,   881,   421,
     434,   170,  1796,  1813,    86,    74,  1669,    -1,   834,   267,
     310,    59,  1776,    45,   108,   835,  1762,   806,   420,   558,
     459,   419,  -137,  1424,   180,   806,  1666,  1682,  1296,   599,
      63,   161,  1473,   743,    63,   376,   183,  1367,  1766,   423,
    -138,   216,    59,  1699,  -138,    45,    59,   312,  1058,    60,
     526,   203,    58,   182,    45,   490,   181,  1722,  1133,    92,
     175,   468,  1753,    61,  1818,    -2,   807,   164,  1084,   808,
    1448,  1560,   380,   177,   807,  1417,   472,   808,   176,  -872,
     379,   689,   426,   426,   689,   274,   320,   184,   321,  1257,
    1134,   564,   541,   543,    61,   715,  1059,   429,    61,  1682,
      74,  1370,   431,  1197,  1728,    98,  1200,   553,    15,  1682,
    1682,   329,  -450,   739,   958,   438,   441,  1600,    89,  1504,
    1505,  1434,  -138,  1418,   710,   624,  -138,   690,   229,  1602,
     690,    74,  1398,   424,   740,    74,  1526,   229,   229,    86,
     853,   833,  1748,  -450,   376,  -328,   735,  -450,   860,  1371,
      45,   153,   161,   161,   161,  1682,  1401,   822,   472,   715,
     504,   507,  1402,  1403,    90,  1405,    91,   229,    40,  1435,
     139,   255,   759,   635,   451,  -313,   112,   379,   709,  1777,
    1573,   380,   626,  1573,    45,  1802,  1559,   363,   164,   164,
     164,  -450,   736,   933,    59,  1565,  1566,   646,  1567,   161,
      40,   420,   702,   314,   625,   833,   823,  1682,  1802,   824,
     754,  1379,  1284,  1381,  1285,   868,   869,  1802,   379,    96,
     561,  -313,  -313,   229,  1072,    99,  1743,  1275,  1574,   867,
     311,  1629,   747,  1803,  -635,   164,  -313,   439,   442,   255,
      97,   376,   113,    94,   859,   635,    61,   142,  1489,   451,
    1280,  1281,  1369,  1342,  1089,  1090,  1804,   274,   740,  1780,
    -143,   326,   131,   132,    95,  1817,   743,    26,   803,   195,
    1286,  1287,   500,  1442,   131,   132,   769,   167,   380,   216,
    -635,  -635,   376,    74,   530,   532,    96,  1062,  1343,   774,
    -335,   229,  1344,   530,   310,  -635,   527,  1673,  1674,  -398,
     892,    15,  1020,   143,  1781,  1481,  -143,    97,   327,   532,
     790,   168,  -154,   623,   185,   196,   197,   328,   135,   380,
      63,   189,   183,  -399,  1345,    15,    26,     8,     9,   134,
     135,   312,    59,   229,  -398,  1750,  -335,  -335,  -398,   182,
     195,   263,   181,  1009,  1751,   193,   893,   216,  1021,  1085,
     255,  -328,  -660,  1010,   769,   555,  -337,   812,  -399,   818,
     820,   532,  -399,  1278,   864,   194,  1011,  1383,  1384,  1022,
    -398,  -337,   221,   184,   143,  -337,   145,     8,     9,    10,
    1441,    18,   134,   135,    61,   274,   196,   518,   780,  1752,
     866,   781,  -337,  1447,  -399,   144,     8,     9,  -660,  -660,
    1805,   254,   229,   784,   252,  -337,  -337,   314,  -337,   929,
    -337,   131,   132,  -660,  1024,  1023,    21,   779,   248,  1292,
    1328,    74,   249,  -130,    18,    15,   943,  1229,  1783,    26,
    -634,  1073,    28,    29,   229,   229,   199,  1784,  -337,  -337,
     100,   114,   115,   229,  1120,  1806,   200,  1074,    26,   131,
     132,   134,   135,  -337,   429,   319,   201,   431,  -130,   229,
    1025,   877,  -130,    26,    33,  1321,   134,   135,   202,   842,
     843,   561,  1088,  1173,   325,  1322,  -634,  -634,  1346,   250,
     997,   537,  1785,   251,  1425,   538,   998,  1323,  1000,  1001,
    1121,  -634,  1130,  1131,  -130,   935,   116,   117,  1135,   379,
     948,   948,   948,   948,   134,   135,   161,   161,   161,   967,
    1067,   229,   379,   143,  1005,   390,  1502,    15,   759,   386,
    1644,  -450,  -314,   379,  1347,  1068,  1057,   865,   771,   772,
    1426,   702,  1006,  -312,   770,  -128,   476,    15,   229,  1313,
    1315,   394,   164,   164,   164,  1042,   687,   688,   763,  1555,
     739,  -450,  -450,  1018,   477,  -188,  -450,  -188,   379,   311,
    1007,  1397,  1503,   376,   376,   376,   376,   376,  -314,  -314,
    -128,   740,   230,   231,  -128,   555,   376,   769,   451,  -312,
    -312,   395,   478,  -314,   379,   665,   379,   376,   490,   493,
    1005,  1043,  1044,   855,  -312,   493,   416,   903,   599,   174,
     380,   380,   380,   380,   380,   126,  -128,  1070,  1006,   391,
    1678,  1310,  1757,   380,   426,   267,  1078,  1543,   274,   131,
     132,   530,   376,   310,   380,   406,   392,   223,   224,   607,
      95,   608,   740,  -328,    14,  1274,  1007,  1392,  1394,   820,
    1284,   393,  1285,   868,   869,   550,   967,    97,   376,    15,
     376,    18,    96,  -450,  1724,   877,  1679,    20,  1758,   380,
     312,   133,   142,  -661,   444,    78,    23,   452,   715,   549,
     769,    26,   556,    97,   134,   135,  1027,  1030,   490,  -337,
    1543,   229,  1145,  -450,  -450,   380,  -655,   380,  -143,   454,
     426,   551,    -7,   142,  1148,    26,    94,    78,  1286,  1287,
    -139,  -139,    96,   743,   610,  -151,    78,   131,   132,  -661,
    -661,  -152,    13,   466,   833,   945,  1394,    95,   467,   208,
     607,   220,   608,    97,  -661,  -337,  -337,   131,   132,  1374,
    1375,  1376,   559,   274,  -143,    18,   170,  1128,   834,    63,
    -331,   470,   769,   480,   976,   536,  1065,  1437,   769,   493,
    1292,    59,   715,  1543,  1329,  1330,  1331,  1799,  1150,    26,
     153,   546,   134,   135,  1312,  1142,   131,   132,    94,  1099,
    1100,   229,   999,  1195,  1282,  1650,   716,  1449,  1016,    26,
     566,   890,   134,   135,   230,   465,   717,   142,    96,    95,
     451,   568,   780,   781,  1036,   574,  1009,  1230,   718,   719,
     784,   422,   597,    61,  1122,  1123,  1010,   427,   161,    97,
    1754,  1242,  1478,  1243,   610,  1244,   161,   274,    26,  1011,
     619,   134,   135,   133,  -612,   549,  1720,   891,  -612,   439,
     442,  1543,    78,    26,   636,   600,    78,  1647,   439,   442,
      74,   451,   208,   220,   164,   601,  1333,    94,   639,    94,
    1271,   640,   164,    96,   516,   517,  1561,   641,  1631,  1499,
     229,   642,   948,   643,  1039,   644,  1231,   661,    95,  1543,
      95,   493,   662,   493,    97,  1159,  1160,  -612,  1161,  -612,
    -612,   714,  -612,   120,   121,   122,   131,   132,    96,   524,
     525,   208,  1334,  -612,   755,  -612,   948,  1632,   516,   791,
    1545,    95,   769,   756,   747,   267,  1543,   -56,   490,    97,
    -612,  -612,   -56,   782,    18,  1170,  1171,  1172,  1070,   529,
     223,   224,   536,   -56,   785,  -612,   376,    14,   524,   792,
      -6,   715,   769,    97,   948,   793,    26,  1293,    26,   123,
     124,   134,   135,   795,  1145,   786,  1145,   925,   926,   804,
      20,  1351,  1361,   805,  1145,   716,  1148,  -871,  1148,    23,
     376,   821,   769,   380,  1148,   717,  1148,  1571,  1572,   516,
    1701,   490,   524,  1702,   849,  1340,  1341,   718,   719,  1484,
     948,   516,  1706,   976,   627,  1372,   832,   131,   132,   100,
     114,   115,   684,   685,   686,   687,   688,   380,   376,   830,
      15,    63,   862,    63,   771,  1613,   863,  1027,  1030,   216,
     884,    63,  -188,    59,   885,    59,   886,   887,  -195,  1352,
    1150,   894,  1150,    59,   769,  1338,   769,  1142,   895,  1142,
    1150,  -877,   715,   142,  -195,   380,  -195,  1142,   930,    26,
     493,   938,   134,   135,   376,   116,   117,   691,   628,   429,
     431,   940,   939,   493,   952,   274,   716,   954,   561,   955,
     958,  1298,  1299,  1393,   948,    61,   717,    61,   131,   132,
    1298,  1299,  1017,   254,  1019,    61,  1037,  1060,   718,   719,
     208,   380,  1039,  1064,  1061,  1066,  1234,   692,   693,   427,
    1071,   694,   695,   696,   697,   206,    18,    20,   161,   161,
     161,  1075,    74,  1076,    74,  1079,  -331,   161,   161,   161,
    1086,  1087,    74,   715,   834,  1091,   274,  1097,  1118,  1259,
      26,  1125,    78,   134,   135,   769,  1203,  1126,   376,   922,
     923,   924,   925,   926,   164,   164,   164,   716,   161,  1129,
    1351,  1393,  1132,   164,   164,   164,  1208,   717,   208,   815,
     208,   208,  1196,  1204,  1206,  1207,   826,   159,   267,   718,
     719,   769,  1209,  1210,   427,   380,  1211,  1212,  1213,  1214,
    1145,  -150,  1216,   264,   164,  1217,  1220,   208,  1223,  1219,
    1221,  1451,  1148,  1222,  1225,   208,  1224,  1453,  1454,  1226,
    1236,  1451,  1456,   769,  1238,  1454,  1239,  1240,  1241,  1249,
    1250,  1260,   271,     8,     9,    10,   769,  1487,  1352,   493,
    1251,  1261,  1262,  1444,  1445,  1272,  1491,  1273,   206,  1807,
    1263,   427,   834,  1294,  1027,  1030,  1267,    63,  1269,   723,
    1327,  1409,   731,   734,   146,   146,  1308,   162,  1373,    59,
    1459,  1377,    21,     8,     9,  1461,  1150,  1382,   780,   781,
    1386,  1387,  1388,  1142,  1389,   784,  1395,  1399,    28,   272,
    1429,  1430,  1145,   219,  1404,  1408,   228,   206,  1293,   564,
    1400,    18,   245,  1452,  1148,  1410,  1411,   256,  1412,   440,
     443,  1122,  1123,  1415,  1416,  1419,   208,  1420,   715,  1660,
      33,    61,  1457,   948,  1658,    26,  1663,  1433,   134,   135,
     682,   683,   684,   685,   686,   687,   688,   271,  1458,  -701,
      10,  1406,  1493,   271,   409,   410,    10,  1464,  1535,    63,
    1465,  1479,  1494,  1510,  1511,   159,  1520,  1521,    74,  1523,
    1467,    59,  1483,  1501,  1495,   719,    18,  1482,  1150,   477,
     814,  1507,   723,   731,   734,  1142,  1531,    21,  1563,  1564,
     428,  1532,  1533,    21,   769,   428,  1534,   376,   437,   437,
     162,  1472,   769,   445,   446,   599,    26,  1546,  1548,    28,
     272,  1549,  1551,  1617,  1630,  1640,  1639,   447,  1641,  1653,
    1655,  1557,  1558,    61,  1664,   219,  1665,   448,  1667,  1668,
    -929,   469,  1685,  1686,   380,    33,  1736,  1690,  1692,   449,
    1599,    33,  1607,   503,   503,   512,   389,  1693,   769,  1700,
    1709,   120,   121,   122,    59,  1719,  1729,  1628,  1730,   228,
      74,  1606,  1711,  1733,  1605,  1778,  1737,  1745,  1598,   533,
     208,   439,   442,  1746,  1771,  1621,  1622,  1623,  1645,  1646,
    1774,   408,   409,   410,    10,   483,  1775,  1782,  1786,  1309,
       8,     9,    10,  1429,  1430,  1787,   206,  1607,   228,  1790,
    1791,  1468,  1469,  1470,  1471,   216,    61,   123,   124,    59,
    1793,  1800,  1480,   560,  1801,  1657,  1606,  1657,  1797,  1605,
    1810,    21,   656,  1598,  1808,  1809,   340,   738,   261,    21,
    1811,   526,  1671,  1672,  1695,  1698,  1812,   262,   272,   427,
    1814,  1819,    26,    74,  1127,    28,   272,   920,   921,   922,
     923,   924,   925,   926,   228,   256,  1820,  1822,   427,   229,
    1823,    61,     1,   461,   206,     5,   206,   206,   166,    33,
    1157,   326,     8,     9,  1063,  1697,  1162,    33,  1715,  1276,
     837,  1155,  1490,  1731,  1607,  1715,   458,   460,   789,   837,
     326,   131,   132,   206,  1735,  1362,    59,   323,    74,  1681,
     425,   206,   437,  1606,   379,   606,  1605,  1096,  1756,  1268,
    1598,   440,   783,  1439,   941,  1332,  1460,  1599,   327,  1607,
    1364,  1524,   228,   256,  1202,   942,  1738,   328,   135,  1004,
     711,    59,   100,   114,   115,   552,   519,   327,  1606,   404,
    1577,  1605,   310,  1715,  1544,  1598,   328,   135,    61,   310,
     956,   263,  1779,  1742,  1755,   100,   101,   102,  1603,  1749,
    1794,     0,   731,   219,   228,  1498,  1117,   229,   376,  1772,
     563,     0,    21,   440,   443,     0,     0,     0,     0,   312,
       0,   428,     0,    61,   428,    74,   312,     0,   116,   117,
     162,   162,   162,     0,     0,     0,   560,     0,     0,   161,
       0,     0,   206,   842,   843,   380,  1643,   310,    26,     0,
       0,   103,   104,   105,     0,   145,     8,     9,    10,     0,
      74,     0,     0,   100,   114,   115,  1252,     0,  1253,     0,
       0,   219,     0,   228,   256,   164,     0,     0,     0,     0,
     161,   161,   161,     0,   312,     0,     0,   838,     0,     0,
     838,     0,   841,   841,   512,    21,     0,     0,   469,  1691,
       0,   715,     0,     0,   857,     0,   533,   483,    26,   469,
       0,    28,    29,     0,     0,     0,   164,   164,   164,   116,
     117,   118,     0,     0,     0,   809,     0,     0,     0,     0,
     533,   469,     8,     9,  1713,   810,     0,   559,     0,   427,
     976,  1713,     0,    33,     0,     0,     0,   811,   719,   271,
     131,   132,    10,     0,     0,   440,   953,     0,     0,     0,
      18,   326,     8,     9,     0,     0,     0,   656,   100,   101,
     102,     0,   233,   234,   235,  1603,     0,     0,    18,     0,
       0,   731,   533,  1155,    26,  1155,   206,   134,   135,    21,
    1358,  1365,  1378,  1155,  1380,   715,     0,    18,     0,  1713,
       0,  1321,    26,   236,     0,    28,   272,     0,   327,   228,
       0,  1322,     0,  1773,     0,     0,     0,   328,   135,  1264,
       0,    26,     0,  1323,   103,   104,   237,     0,   656,  1265,
     656,     0,  1052,  1789,   162,   162,   437,    33,   326,     8,
       9,  1266,   719,   976,     0,     0,     0,   437,   483,     0,
       0,     0,     0,     0,     0,   837,     0,     0,   837,  1245,
       0,     0,     0,     0,  1816,   483,     0,   222,   223,   224,
     238,   239,   240,     0,   837,    14,   145,     8,     9,    10,
     173,    12,    13,     0,  1414,   327,  1245,     0,    14,     0,
       0,     0,    18,     0,   328,   135,     0,     0,    20,  1026,
    1026,  1026,    16,     0,    17,    18,    19,    23,   437,     0,
     437,    20,  1053,     0,   159,     0,    21,     0,   563,   483,
      23,     0,  1114,   174,     0,   483,     0,   483,   483,    26,
       0,  1245,    28,    29,     0,     0,  1052,  1245,   427,     0,
       0,     0,   145,     8,     9,    10,    31,   427,   440,   783,
       0,     0,   228,   245,     0,     0,    32,     0,   483,  1317,
       0,     0,     8,     9,    33,   483,    12,    13,    34,     0,
       0,    18,    35,    14,     0,   100,   432,   433,     0,  1358,
       0,     0,    21,  1348,  1349,     9,    10,    16,   715,    17,
      18,     0,  1098,  1098,  1104,    26,    20,     0,    28,    29,
       0,     0,  1104,     0,     0,    23,     0,     0,     0,  1155,
       0,     0,   809,     0,    26,     0,   162,   134,   135,     0,
       0,     0,   810,    21,     0,   838,   838,   841,   841,   512,
      33,   103,   117,   857,   811,   719,    26,   223,   224,    28,
      29,  1485,     0,  1443,    14,  1350,     0,   469,   483,     0,
       0,     0,     0,   200,   145,     8,     9,    10,     0,     0,
      13,    18,   483,   201,     0,     0,     0,    20,  1500,     0,
       0,    33,     0,     0,  1506,   202,    23,     0,     0,  1245,
       0,     0,     0,    18,   120,   868,   869,  1245,   870,     0,
    1527,  1528,     0,  1519,    21,     0,     0,     0,  1519,     0,
     715,  1155,     0,     0,     0,     0,   269,    26,   131,   132,
      28,    29,     0,   559,  1547,   837,   145,     8,     9,    10,
     871,   837,   254,     0,   809,     0,     0,     0,   375,     0,
    1245,   228,     0,   269,   810,     0,    18,    26,     0,  1245,
     123,   124,    33,     0,     0,    18,   811,   719,     0,     0,
       0,   440,   953,   715,     0,     0,    21,  1569,  1570,     0,
      26,   269,   715,   134,   135,     0,  1357,     0,     0,    26,
       0,     0,    28,    29,   269,     0,     0,   716,     0,     0,
       0,  1615,  1616,     0,  1618,     0,   809,   717,     0,     0,
       0,     0,     0,     0,     0,     0,   810,     0,     0,   718,
     719,   100,   101,   102,    33,   440,   443,     0,   811,   719,
    1246,     0,   486,  1246,   440,  1307,     0,     0,     0,     0,
     145,     8,     9,    10,  1254,     0,    13,     0,     0,     0,
    1053,  1053,  1053,     0,   269,     0,   483,   528,     0,     0,
     259,     8,   260,    10,   159,     0,   208,  1661,   208,    18,
     483,     0,   483,     0,   483,   245,     0,   103,   104,     0,
      21,     0,   396,   397,   398,     0,   715,     0,     0,     0,
     269,     0,     0,    26,     0,     0,    28,    29,     0,     0,
      21,     0,  1297,  1297,  1104,  1104,  1104,   261,     0,   483,
    1101,  1306,     0,  1104,  1104,  1104,   262,    29,  1708,     0,
    1102,   269,     0,     0,     0,  1324,  1324,  1325,    33,   399,
       0,     0,  1103,   719,   837,     0,     0,     0,   400,   401,
     263,     0,     0,   837,   162,   539,   101,   102,    33,  1245,
    1245,     0,     0,   219,   616,     0,   259,   131,   132,    10,
       0,   837,   837,   131,   132,     0,     0,   223,   224,     0,
     145,     8,     9,    10,    14,  1357,   559,   208,   100,   114,
     115,     0,   744,   745,   746,     0,     0,     0,   440,   783,
       0,    18,   327,     0,     0,   658,    21,    20,     0,    18,
       0,   540,   104,   261,     0,  1245,    23,     0,   715,     0,
      21,     0,   262,    29,     0,    26,   715,     0,   134,   135,
     100,   114,   115,    26,   440,   783,    28,    29,   269,    82,
       0,     0,   716,     0,   116,   117,   263,     0,     0,   109,
     809,     0,   717,     0,    33,     0,     0,     0,     0,   137,
     810,     0,     0,   768,   729,   719,   150,   150,    33,   150,
       0,    82,   811,   719,   396,   397,   398,  1053,  1053,  1053,
      82,     0,     0,   837,   269,     0,   116,   117,     0,     0,
       0,     0,     0,   210,     0,    82,     0,     0,     0,  1518,
       0,     0,     0,     0,   246,     8,     9,     0,     0,   109,
     254,  1297,  1297,  1104,  1104,  1104,     0,     0,  1306,     0,
     277,     0,   109,     0,     0,     0,     0,    26,     0,     0,
     400,   401,     0,    18,  1436,  1436,  1325,   271,   409,   410,
      10,   768,     0,   837,   837,   270,   109,     0,     0,     0,
     715,     0,     0,   469,   469,     0,     0,    26,     0,     0,
     134,   135,     0,   271,   131,   132,    10,     0,     0,     0,
       0,     0,     0,   137,  1493,    82,     0,    21,   131,   132,
     469,   150,   150,   559,  1494,   469,   430,   150,     0,     0,
     150,   150,   150,    28,   272,     0,  1495,   719,   269,   837,
       0,     0,     0,    21,     0,     0,    82,     0,     0,     0,
      82,     0,     0,     0,     0,     0,   210,    82,     0,    28,
     272,   145,   131,   132,    10,    33,   453,    13,     0,     0,
      26,     0,     0,   134,   135,   210,   210,   210,     0,     0,
       0,     0,  1496,  1496,     0,  1497,     0,   600,     0,     0,
      18,    33,   206,     0,   206,     0,   269,   601,     0,   502,
     232,    21,     0,   469,   469,   210,   469,   469,     0,   469,
       0,   269,   521,     0,    26,     0,   483,    28,    29,   120,
     868,   869,   544,     0,     0,   483,   946,   947,   949,   950,
     951,    31,   109,     0,   259,     8,     9,    10,     0,   658,
       0,    32,     0,     0,     0,   150,  1552,  1552,  1552,    33,
     970,     0,     0,    34,     0,     0,     0,     0,     0,     0,
       0,   469,   469,     0,     0,     0,     0,   120,   868,   869,
       0,     0,    26,     0,    21,   123,   124,   100,   114,   115,
     232,   261,   100,   114,   115,  1002,     0,   109,   602,     0,
     262,    29,     0,   617,     0,     0,   232,     0,    82,     0,
       0,   505,   508,   206,     0,     0,   269,     0,     0,     0,
       0,  1045,     0,  1046,   263,   469,   469,   469,   408,     8,
     562,    10,    33,   123,   124,     0,   269,     0,     0,     0,
       0,     0,     0,   116,  1694,  1114,     0,     0,   116,  1696,
       0,     0,     0,     0,     0,     0,   109,     0,     0,   712,
       0,   602,     0,   232,   602,   732,     0,     0,    21,   219,
     228,     0,   232,     0,     0,   261,     0,   145,   131,   132,
      10,     0,   469,   469,   262,   272,     0,   440,  1307,   232,
       0,     0,   137,     0,   705,     0,   271,   409,   410,    10,
     232,   706,   109,     0,   210,   109,    18,     0,   563,     0,
       0,     0,     0,   150,     0,     0,    33,    21,     0,     0,
     269,     0,     0,   150,     0,   512,   150,     0,     0,     0,
      26,     0,     0,    28,    29,     0,    21,     0,   150,     0,
       0,     0,     0,   707,     0,     0,    82,   155,     0,    26,
       0,   269,    28,   272,     0,     0,     0,   156,     0,     0,
    1028,  1031,     0,     0,     0,    33,   841,   841,   841,   157,
       0,   483,   210,   816,   210,   210,   732,     0,   228,   486,
     816,     0,     0,     0,   708,   486,     8,     9,   210,   210,
       0,   559,   210,     0,   210,   210,   210,   847,     0,     0,
       0,   210,     0,     0,     0,   269,   210,     0,     0,   210,
     271,   131,   132,    10,    18,     0,    13,     0,     0,   469,
       0,     0,     0,     0,     0,   850,     0,     0,     0,     0,
     856,   715,     0,     0,     0,     0,     0,     0,    26,    18,
       0,   134,   135,     0,     0,   150,     0,     0,     0,     0,
      21,     0,     0,  1028,  1031,  1493,   715,     0,     0,     0,
       0,   882,     0,    26,     0,  1494,    28,   272,   888,     0,
       0,     0,   232,     0,     0,     0,     0,  1495,   719,     0,
    1264,   232,     0,     0,     0,     0,     0,     0,     0,   901,
    1265,     0,     0,     0,     0,     0,     0,     0,    33,  1227,
       0,     0,  1266,   719,     0,     0,   408,   131,   132,    10,
     210,   898,   899,   232,   900,   839,     0,     0,   840,     0,
     505,   508,     0,     0,     0,     0,   109,   109,   109,   109,
       0,     0,   858,   970,     0,   271,     8,     9,    10,   486,
       0,    13,     0,   222,   223,   224,    21,   232,     0,     0,
       0,    14,     0,   261,     0,     0,   232,     0,   269,     0,
     269,     0,   262,   272,    18,     0,     0,     0,    18,   768,
       0,  1277,     0,     0,    20,    21,     0,   232,     0,     0,
       0,   715,     0,    23,     0,   109,   563,   602,    26,     0,
       0,    28,   272,     0,    33,     0,     0,     0,   712,   775,
       0,   602,   602,   732,   232,  1300,     0,     0,     0,     0,
     269,     0,     0,   269,  1054,  1301,     0,  1318,  1056,   271,
       8,     9,    10,    33,     0,     0,     0,  1302,   719,     0,
       0,     0,  1040,  1041,     0,     0,     0,     0,   137,     0,
       0,  1040,     0,     0,     0,   137,     0,     0,    18,     0,
       0,     0,     0,     0,   210,   246,     0,     0,     0,    21,
       0,   486,     0,   486,     0,   715,     0,     0,     0,     0,
       0,  1247,    26,     0,  1248,    28,   272,     0,     0,     0,
       0,   145,   131,   132,    10,  1255,     0,   254,     0,  1300,
       0,  1028,  1031,     0,   210,   210,  1106,     0,     0,  1301,
       0,  1385,  1109,     0,  1106,     0,     0,    33,     8,     9,
      18,  1302,   719,    13,     0,     0,     0,     0,     0,     0,
       0,    21,     0,   210,  1108,     0,   847,   210,   210,   847,
     847,   847,     0,     0,    26,   210,    18,    28,    29,     0,
       0,     0,   210,  1247,  1248,  1028,  1031,     0,     0,     0,
       0,    31,  1255,     0,  1028,  1031,     0,     0,     0,  1124,
      26,    32,     0,   134,   135,    82,     0,     0,   109,    33,
       0,     0,     0,    34,   109,     0,     0,  1321,     0,     0,
       0,     0,     0,   602,   602,   602,   172,  1322,   145,     8,
       9,    10,   173,    12,    13,     0,     8,     9,     0,  1323,
      14,  1164,     0,  1166,   680,   681,   682,   683,   684,   685,
     686,   687,   688,     0,    16,     0,    17,    18,    19,     0,
     505,   508,     0,    20,    18,     0,     0,     0,    21,     0,
     486,     0,    23,   602,     0,   174,     0,   145,   131,   132,
      10,    26,     0,   486,    28,    29,     0,     0,    26,     0,
       0,   134,   135,   839,   840,   505,   508,     0,    31,     0,
       0,   858,     0,   269,     0,  1321,    18,     0,    32,     0,
       0,     0,     0,     0,     0,  1322,    33,    21,     0,     0,
      34,     0,   232,     0,    35,   232,     0,  1323,   109,     0,
      26,     0,     0,    28,    29,     0,     0,     0,     0,     0,
       0,   232,     0,     0,     0,     0,     0,    31,  1028,  1031,
       0,   602,   602,     0,     0,   602,     0,    32,     0,     0,
    1232,  1233,   109,  1235,     0,    33,   602,     0,   109,    34,
    1318,     0,  1054,  1054,  1054,   269,     0,     0,   602,     0,
    1109,     0,  1247,  1248,  1028,  1031,   100,   114,   115,  1255,
     233,   234,   235,  1256,     0,     0,     0,     0,   109,   269,
     109,   675,   676,   677,   678,   679,   680,   681,   682,   683,
     684,   685,   686,   687,   688,    18,     0,     0,     0,     0,
       0,   236,     0,   210,   210,   210,   210,   210,  1106,   847,
       0,     0,     0,   210,     0,  1106,  1106,  1106,     0,   486,
       0,  1109,   116,   117,     0,     0,   109,   847,   847,   847,
     131,   132,     8,     9,   223,   224,     0,    13,  1305,     0,
       0,    14,     0,   137,     0,     0,   150,    82,     0,    82,
       0,     0,     0,     0,  1359,    82,     0,    82,    18,     0,
      18,     0,     0,     0,    20,   768,     0,     0,     0,     0,
     109,     0,   109,    23,     0,   715,     0,   715,   408,     8,
       9,    10,    26,     0,    26,   134,   135,   134,   135,     0,
     232,     0,     0,  1247,  1248,     0,  1255,     0,     0,   716,
       0,  1493,     0,     0,     0,     0,     0,     0,     0,   717,
     109,  1494,     0,     0,     0,     0,     0,   232,    21,     0,
       0,   718,   719,  1495,   719,   261,     0,     0,     0,     0,
     839,   840,   505,   508,   262,   272,     0,   602,   602,   858,
     602,   505,   508,   408,     8,     9,    10,     0,     0,     0,
       0,   602,     0,   839,   840,   858,     0,  1028,  1031,   602,
       0,     0,   232,     0,     0,     0,    33,     0,   232,   602,
     602,   732,     0,     0,     0,     0,     0,     0,     0,   905,
       0,  1413,     0,    21,     0,     0,     0,     0,     0,     0,
     261,     0,     0,   408,     8,     9,    10,     0,     0,   262,
     272,     0,   210,   210,   210,   847,   847,  1431,     0,     0,
     210,   210,     0,   145,     8,     9,    10,   222,   223,   224,
       0,   658,     0,   263,     0,    14,   847,   847,   847,   847,
     847,    33,     0,    21,  1432,   408,     8,     9,    10,   109,
     927,     0,    18,  1359,     0,     0,     0,     0,    20,   262,
     272,     0,   109,    21,     0,     0,     0,    23,     0,   715,
       0,     0,     0,     0,  1440,  1455,    26,     0,    85,    28,
      29,     0,     0,    82,     0,    21,     0,  1446,   111,     0,
       0,    33,   261,   809,     0,     0,     0,   130,   138,     0,
       0,   262,   272,   810,     0,   151,   151,     0,   151,     0,
      85,    33,     0,     0,  1462,   819,   719,     0,  1463,    85,
     232,     0,     0,     0,     0,   563,     0,     0,   232,     0,
       0,     0,   151,    33,    85,     0,     0,     0,     0,   839,
     840,   505,   508,   247,   847,   847,   858,   847,   257,   109,
     614,   847,     0,     0,     0,     0,  1486,   222,   223,   224,
       0,   257,   505,   508,     0,    14,     0,   210,     0,   637,
     638,   232,   150,     0,     0,    82,     0,     0,     0,     0,
     232,     0,    18,     0,   647,     0,     0,   648,    20,     0,
     649,     0,     0,   659,   660,     0,     0,    23,   232,   232,
       0,     0,     0,     0,   704,  1529,  1530,     0,   847,   847,
    1431,   847,   847,   936,    85,     0,     0,     0,   109,     0,
     151,   151,     0,     0,     0,     0,   151,     0,     0,   151,
     151,   151,   918,   919,   920,   921,   922,   923,   924,   925,
     926,     0,  1553,  1554,     0,    85,     0,     0,  1556,    85,
       0,     0,     0,     0,     0,   151,    85,     0,     0,     0,
       0,     0,     0,     0,   109,     0,     0,   847,     0,     0,
     839,   840,     0,   858,   151,   151,   151,     0,     0,     0,
       0,     0,     0,  1174,  1175,     0,  1177,  1178,  1179,  1180,
    1181,  1182,  1183,  1184,  1185,  1186,  1187,  1188,  1189,  1190,
    1191,  1192,  1193,  1194,   151,   408,   131,   132,    10,     0,
     232,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     210,   816,   210,     6,     0,     7,     8,     9,    10,    11,
      12,    13,     0,     0,   505,   508,     0,    14,     0,     0,
       0,     0,     0,     0,   151,    21,     0,     0,     0,     0,
       0,    16,   261,    17,    18,     0,     0,     0,  1670,     0,
      20,   262,   272,     0,     0,    21,     0,     0,     0,    23,
     232,   232,   457,     0,     0,     0,     0,  1106,    26,     0,
       0,    28,    29,     0,     0,    30,   257,   604,     0,     0,
       0,     0,   618,    33,     0,    31,     0,    85,     0,     0,
       0,     0,     0,     0,     0,    32,     0,     0,     0,     0,
       0,     0,     0,    33,     0,     0,     0,    34,  1106,  1106,
    1106,     0,     0,     0,     0,     0,   232,   914,   915,   916,
     917,   210,   918,   919,   920,   921,   922,   923,   924,   925,
     926,     0,     0,     0,   614,   257,     0,     0,   713,     0,
     604,     0,     0,   604,   733,     0,  1279,     0,  1136,   742,
       7,     8,  1137,    10,   173,    12,    13,     0,     0,     0,
       0,     0,    14,     0,     0,     0,     0,     0,     0,     0,
       0,   760,     0,   647,   648,     0,    16,     0,    17,    18,
      19,   257,     0,   151,   257,    20,  -555,     0,     0,     0,
      21,     0,   151,     0,    23,  1138,     0,   174,     0,     0,
       0,     0,   151,    26,     0,   151,    28,    29,     0,     0,
    1139,     0,  1140,     0,     0,     0,     0,   151,     0,     0,
      31,     0,     0,     0,     0,    85,   145,     8,     9,    10,
      32,     0,     0,     0,   505,   508,     0,     0,    33,     0,
       0,     0,  1141,     0,     0,     0,     0,     0,     0,     0,
       0,   151,   817,   151,   151,   733,     0,     0,  -555,   817,
     737,     8,     9,    10,     0,     0,    21,   151,   151,     0,
       0,   151,     0,   151,   151,   151,   604,     0,     0,    26,
     151,     0,    28,    29,     0,   151,  1656,     0,   151,     0,
       0,     0,     0,     0,     0,     0,   200,   340,   738,     0,
      21,     0,     0,     0,     0,     0,   201,     0,     0,   879,
       0,     0,     0,    26,    33,     0,   134,   135,   202,     0,
       0,     0,     0,     0,   151,   567,   569,   570,   571,   572,
     573,     0,   575,   576,   577,   578,   579,   580,   581,   582,
     583,   584,   585,   586,   587,   588,   589,   590,   591,   592,
     593,   594,   595,   596,     0,   598,     0,     0,     0,     8,
       9,     0,     0,    12,    13,     0,     0,     8,     9,     0,
      14,    12,   254,     0,     0,     0,     0,     0,    14,     0,
       0,     0,   742,     0,    16,     0,    17,    18,     0,   151,
       0,     0,    16,    20,    17,    18,     0,     0,     0,     0,
       0,    20,    23,     0,     0,   257,   257,   257,   257,     0,
      23,    26,     0,     0,   134,   135,   698,     0,     0,    26,
       0,     0,   134,   135,     0,     0,     0,     0,   614,   614,
       0,   614,   614,   614,   614,   614,   614,   614,   614,   614,
     614,   614,   614,   614,   614,   614,   614,   614,   614,  1466,
     699,     0,     0,     0,     0,     0,   145,     8,     9,    10,
     222,   223,   224,     0,   257,     0,   604,     0,    14,     0,
       0,   408,   131,  1522,    10,     0,     0,     0,     0,     0,
     604,   604,   733,     0,     0,    18,     0,     0,     0,     0,
       0,    20,     0,  1055,     0,     0,    21,     0,     0,     0,
      23,     0,   715,     0,     0,     0,   742,     0,     0,    26,
       0,    21,    28,    29,     0,     0,     0,   760,   261,     0,
       0,     0,     0,     0,     0,   130,   809,   262,   272,     0,
       0,     0,     0,   151,   247,     0,   810,  1237,     0,     0,
     737,     8,     9,    10,    33,     0,     0,     0,   811,   719,
       0,   563,     0,     0,     0,     0,     0,     0,     0,    33,
    1348,     8,  1137,    10,   212,    12,   213,     0,     0,   647,
     648,     0,    14,   151,   151,   817,     0,   340,   738,     0,
      21,     0,     0,  1116,     0,     0,    16,     0,    17,    18,
       0,     0,     0,    26,     0,    20,   928,   135,     0,     0,
      21,   614,   151,     0,    23,   604,   151,   151,   604,   604,
     604,     0,     0,    26,   151,     0,    28,    29,     0,     0,
     896,   151,  1350,     0,     0,     0,     0,     0,     0,     0,
      31,     0,   647,   648,     0,     0,     0,     0,     0,     0,
      32,     0,     0,   897,    85,     0,     0,   257,    33,     0,
       0,   902,  1141,   257,     0,     0,     0,     0,     0,     0,
       0,     0,   604,   604,   604,     0,     0,     0,     0,  1624,
       0,  -512,  -512,  -512,  -512,  -512,  -512,  -512,     0,     0,
       0,  -512,     0,  -512,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  -512,     0,  -512,     0,     0,     0,
    -512,     0,     0,     0,     0,     0,  -512,     0,     0,     0,
       0,  -512,   604,     0,     0,  -512,     0,  -512,     0,     0,
       0,     0,     0,     0,  -512,     0,     0,  -512,  -512,  -512,
    -512,  -512,     0,  -512,  -512,  -512,  -512,  -512,  -512,  -512,
    -512,  -512,  -512,  -512,  -512,  -512,  -512,  -512,  -512,  -512,
    -512,  -512,  -512,  -512,  -512,  -512,     0,  -512,  -512,  -512,
    1407,  -512,  -512,  -512,  -512,  -512,     0,   257,     0,     0,
       0,  1625,  -512,     0,     0,     0,     0,  -512,  -512,  -512,
       0,  -512,     0,     0,     0,     0,     0,     0,     0,     0,
     604,   604,     0,     0,   604,     0,     0,     0,     8,     9,
       0,   257,    12,    13,     0,   604,     0,   257,     0,    14,
       0,  1055,  1055,  1055,     0,     0,     0,   604,     0,   742,
       0,     0,     0,    16,     0,    17,    18,     0,     0,     0,
       0,     0,    20,     0,     0,     0,     0,   257,     0,   257,
       0,    23,     0,     0,     0,     0,     0,     0,     0,     0,
      26,     0,     0,   134,   135,     0,     0,   879,     0,     0,
       0,     0,   151,   151,   151,   151,   151,   817,   604,     0,
       0,     0,   151,     0,  1116,  1116,  1116,     0,     0,     0,
     742,     0,     0,     0,     0,   257,   604,   604,   604,  1003,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   879,
       0,     0,   138,     0,   614,   151,    85,     0,    85,     0,
       0,     0,     0,  1360,    85,     0,    85,     0,     0,     0,
       0,     0,     0,     0,     0,  1348,  1349,     9,    10,   257,
    1136,   257,     7,     8,  1137,    10,   173,    12,    13,     0,
    1168,  1169,     0,     0,    14,     0,     0,     0,     0,     0,
       0,   145,   131,   132,    10,     0,     0,    13,    16,     0,
      17,    18,    19,     0,     0,    21,     0,    20,  -556,   257,
       0,     0,    21,     0,     0,     0,    23,  1138,    26,   174,
      18,    28,    29,     0,     0,    26,     0,  1350,    28,    29,
       0,    21,  1139,     0,  1140,   200,   604,   604,     0,   604,
       0,     0,    31,     0,    26,   201,     0,    28,    29,     0,
     604,     0,    32,    33,     0,     0,     0,   202,   604,     0,
      33,   155,     0,     0,  1141,     0,     0,     0,   604,   604,
     733,   156,     0,     0,     0,     0,     0,     0,     0,    33,
    -556,     0,     0,   157,     0,     0,     0,     0,     0,     8,
       9,     0,   173,    12,    13,     0,     0,     0,   482,     0,
      14,   151,   151,   151,   604,   604,   733,     0,     0,   151,
     151,     0,     0,     0,    16,     0,    17,    18,     0,     0,
       0,     0,     0,    20,     0,   604,   604,   604,   604,   604,
       0,     0,    23,   879,   715,    54,     0,     0,   257,     0,
       0,    26,  1360,     0,   134,   135,     0,  1512,  1513,     9,
      10,   257,     0,     0,     0,     0,     0,     0,   716,     0,
     659,     0,    54,    54,     0,   158,     0,    54,   717,     0,
       0,     0,    85,     0,     0,     0,    54,     0,     0,     0,
     718,   719,     0,     0,     0,     0,     0,    21,     0,    54,
       0,    54,   145,     8,     9,    10,     0,     0,    13,     0,
      26,     0,     0,    28,    29,     0,     0,     0,     0,  1514,
       0,     0,     0,   265,     0,     0,   273,   200,     0,     0,
       0,    18,     0,     0,     0,     0,     0,   201,   879,     0,
       0,     0,    21,   604,   604,    33,   604,     0,   257,   202,
     604,     0,     0,     0,     0,    26,     0,     0,    28,    29,
       0,     0,     0,     0,     0,     0,   151,     0,     0,     0,
       0,   151,   200,     0,    85,   647,   648,     0,   411,   411,
       0,    54,   201,     0,     0,     0,     0,    54,    54,     0,
      33,   265,   273,    54,   202,     0,   158,   158,   158,     0,
       0,     0,     0,   450,     0,     0,     0,   604,   604,   733,
     604,   604,    54,     0,     0,     0,    54,   257,     0,     0,
       0,     0,    54,    54,     0,     0,     0,     8,     9,     0,
     173,    12,    13,     0,     0,     0,     0,     0,    14,     0,
       0,    54,    54,   158,     0,     0,     0,     0,     0,     0,
       0,   265,    16,     0,    17,    18,     0,     0,     0,     0,
       0,    20,     0,   257,     0,     0,   604,     0,     0,     0,
     761,    54,     0,   762,     0,     0,     0,     0,     0,    26,
       0,     0,   134,   135,     0,  1136,     0,     7,     8,  1137,
      10,   173,    12,    13,     0,     0,     0,     0,     0,    14,
       0,     0,     0,     0,     0,     0,     0,     0,  1649,     0,
       0,    54,     0,    16,     0,    17,    18,    19,   265,   151,
     817,   151,    20,  -558,     0,     8,     9,    21,   212,    12,
     213,    23,  1138,     0,   174,     0,    14,     0,     0,     0,
      26,     0,     0,    28,    29,     0,     0,  1139,     0,  1140,
      16,     0,    17,    18,     0,     0,     0,    31,     0,    20,
       0,     0,     0,     0,    54,     0,     0,    32,    23,     0,
       0,     0,     0,     0,     0,    33,  1116,    26,     0,  1141,
     134,   135,     0,     0,   145,   131,   132,    10,     0,     0,
     559,     0,     0,     0,     0,  -558,     0,     0,     0,     0,
     655,     8,     9,     0,   173,    12,    13,     0,     0,     0,
     482,     0,    14,    18,     0,   411,     0,  1116,  1116,  1116,
       0,     0,     0,     0,    21,   265,    16,     0,    17,    18,
     151,     0,     0,     0,     0,    20,     0,    26,     0,     0,
      28,    29,     0,     0,    23,     0,   715,     0,   411,     0,
       0,     0,     0,    26,    31,     0,   134,   135,     0,     0,
      54,     0,     0,     0,    32,     0,     0,     0,     0,    54,
    1493,   265,    33,     0,     0,     0,    34,     0,   450,    54,
    1494,     0,    54,     0,     0,     0,     0,     0,   450,   450,
     450,     0,  1495,   719,    54,   911,   912,   913,   914,   915,
     916,   917,    54,   918,   919,   920,   921,   922,   923,   924,
     925,   926,     0,     0,     0,   145,     8,     9,    10,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    54,    54,
      54,    54,     0,     0,     0,     0,    54,     0,     0,     0,
       0,     0,     0,     0,    54,    54,     0,     0,    54,     0,
     158,   158,   158,   450,     0,    21,     0,    54,     0,    80,
       0,     0,    54,     0,     0,    54,     0,     0,    26,     0,
       0,    28,    29,     0,  1136,     0,     7,     8,  1137,    10,
     173,    12,    13,     0,     0,   200,    80,    80,    14,    80,
       0,    80,     0,     0,     0,   201,     0,     0,     0,     0,
      80,    54,    16,    33,    17,    18,    19,   202,     0,     0,
       0,    20,  -557,    80,     0,    80,    21,     0,     0,     0,
      23,  1138,     0,   174,     0,     0,     0,     0,     0,    26,
       0,     0,    28,    29,     0,     0,  1139,     0,  1140,     0,
     275,     0,     0,     0,     0,     0,    31,   145,     8,     9,
      10,   173,    12,    13,     0,     0,    32,   825,   265,    14,
       0,     0,     0,     0,    33,     0,    54,     0,  1141,     0,
       0,     0,     0,    16,     0,    17,    18,     0,     0,     0,
       0,     0,    20,     0,  -557,     0,     0,    21,     0,     0,
       0,    23,   655,   655,   655,    80,     0,     0,     0,     0,
      26,    80,    80,    28,    29,   655,   275,    80,     0,     0,
      80,    80,    80,     0,     0,     0,     0,    31,     0,     0,
       0,     0,     0,     0,     0,     0,    80,    32,     0,     0,
      80,     0,     0,     0,     0,    33,    80,    80,     0,    34,
       0,     0,     0,     0,     6,     0,     7,     8,     9,    10,
      11,    12,    13,   265,     0,    80,    80,    80,    14,     0,
       0,     0,     0,     0,     0,     0,   655,     0,   655,     0,
     655,    15,    16,     0,    17,    18,    19,     0,     0,     0,
       0,    20,     0,     0,     0,    80,    21,     0,     0,    22,
      23,    24,     0,    25,   411,     0,     0,     0,     0,    26,
      27,   411,    28,    29,     0,     0,    30,     0,     0,     0,
      54,     0,     0,     0,     0,     0,    31,     0,     0,     0,
       0,     0,     0,     0,     0,    80,    32,     0,     0,     0,
       0,     0,     0,     0,    33,     0,     0,     0,    34,     0,
       0,     0,    35,     0,     0,     0,     0,     0,     0,     0,
      54,    54,   158,     0,     0,     0,     0,   265,   273,     0,
    1113,   676,   677,   678,   679,   680,   681,   682,   683,   684,
     685,   686,   687,   688,   655,     0,     0,     0,    80,    54,
       0,     0,   450,    54,    54,   450,   450,   450,     0,     0,
       0,    54,   912,   913,   914,   915,   916,   917,    54,   918,
     919,   920,   921,   922,   923,   924,   925,   926,     0,   799,
       0,   961,   962,   963,    10,     0,    12,   498,   332,   333,
     334,    54,   335,    14,   915,   916,   917,     0,   918,   919,
     920,   921,   922,   923,   924,   925,   926,    16,   336,    17,
     741,    19,     0,   337,   338,   339,    20,     0,   340,   341,
     342,    21,   343,   344,     0,    23,     0,     0,     0,   345,
     346,   347,   348,   349,    26,     0,     0,   964,   965,   800,
       0,     0,   350,     0,    80,     0,     0,     0,   351,     0,
       0,   352,     0,    80,     0,     0,     0,     0,     0,   353,
     354,   355,     0,    80,     0,     0,    80,   356,   357,   358,
       0,     0,     0,   359,   966,     0,     0,     0,    80,     0,
       0,     0,     0,     0,     0,     0,    80,     0,     0,  1095,
     360,     0,     0,     0,     0,     0,     0,     0,   145,     8,
       9,    10,   173,    12,    13,     0,     0,     0,   482,     0,
      14,     0,    80,    80,    80,    80,     0,     0,     0,     0,
      80,     0,     0,     0,    16,     0,    17,    18,    80,    80,
       0,     0,    80,    20,    80,    80,    80,     0,    21,     0,
       0,    80,    23,     0,   715,     0,    80,     0,     0,    80,
       0,    26,     0,     0,    28,    29,     0,     0,   655,   655,
     655,     0,     0,     0,   450,   265,   916,   917,  1101,   918,
     919,   920,   921,   922,   923,   924,   925,   926,  1102,     0,
       0,     0,     0,     0,     0,    80,    33,     0,     0,     0,
    1103,   719,     0,     0,   165,   677,   678,   679,   680,   681,
     682,   683,   684,   685,   686,   687,   688,     0,     0,    54,
      54,    54,   158,   158,   158,   450,     0,   265,   211,    54,
     265,  1113,  1113,  1113,     0,     0,     0,   273,     0,     0,
       0,     8,     9,     0,   173,    12,    13,     0,     0,     0,
     482,     0,    14,   741,     0,     0,     0,     0,     0,     0,
      80,     0,   158,    54,     0,    54,    16,     0,    17,    18,
      54,    54,     0,    54,  1136,    20,     7,     8,  1137,    10,
     173,    12,    13,     0,    23,     0,     0,     0,    14,     0,
       0,     0,     0,    26,     0,     0,   134,   135,     0,     0,
       0,     0,    16,     0,    17,    18,    19,     0,     0,     0,
       0,    20,     0,     0,     0,     0,    21,     0,     0,     0,
      23,  1138,     0,   174,     0,   165,   165,   165,     0,    26,
       0,     0,    28,    29,     0,     0,  1139,     0,  1140,     0,
       0,     0,     0,     0,     0,     0,    31,     0,   741,     0,
       0,   211,     0,     0,     0,     0,    32,     0,     0,     0,
       0,     0,     0,     0,    33,     0,     0,     0,  1141,     0,
     211,   211,   514,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   450,   450,   450,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     211,     0,     0,     0,    80,     8,     9,     0,     0,    12,
     254,     0,     0,     0,     0,     0,    14,     0,    54,    54,
      54,   450,   450,   450,     0,     0,    54,    54,     0,     0,
      16,     0,    17,    18,     0,     0,     0,     0,     0,    20,
       0,     0,     0,     0,    80,    80,    80,     0,    23,     0,
     715,     0,   275,     0,  1115,     0,     0,    26,     0,    54,
     134,   135,     0,     0,     0,   145,     8,     9,    10,     0,
       0,   254,     0,    80,   716,     0,     0,    80,    80,     0,
     265,   273,     0,   603,   717,    80,     0,     0,     0,    54,
       0,     0,    80,     0,    18,     0,   718,   719,     0,     8,
       9,     0,   173,    12,    13,    21,     0,     0,  1612,     0,
      14,     0,     0,     0,     0,    80,     0,     0,    26,     0,
       0,    28,    29,     0,    16,     0,    17,    18,     0,     0,
       0,     0,     0,    20,     0,   200,     0,     0,     0,     0,
       0,     0,    23,     0,     0,   201,   722,     0,     0,   722,
     722,    26,     0,    33,   134,   135,   650,   202,   271,     8,
       9,    10,   173,    12,   331,   332,   333,   334,   482,   335,
      14,     0,     0,    54,     0,     0,   265,     0,    54,     0,
       0,    54,     0,     0,    16,   336,    17,    18,    19,   211,
     337,   338,   339,    20,     0,   340,   341,   342,    21,   343,
     344,     0,    23,     0,   715,     0,   345,   346,   347,   348,
     349,    26,     0,     0,    28,   272,  -346,     0,     0,   350,
       0,     0,     0,     0,     0,   351,     0,     0,  1047,     0,
       0,     0,     0,     0,     0,     0,   353,   354,  1048,     0,
       0,     0,     0,     0,   356,   357,   358,     0,     0,     0,
    1049,   719,  1512,   131,   132,    10,     0,   813,     0,   813,
     813,   722,     0,     0,     0,     0,     0,   360,     0,     0,
       0,     0,     0,   211,   211,     0,     0,   211,     0,   514,
     514,   514,   848,     0,     0,     0,   211,     0,     0,     0,
     741,   211,    21,     0,   211,     0,     0,   209,     0,     0,
       0,   145,     8,     9,    10,    26,     0,    13,    28,    29,
       0,     0,     0,     0,  1514,     0,     0,     0,     0,     0,
       0,   268,    31,     0,   276,     0,    54,    54,    54,     0,
      18,     0,    32,    80,    80,    80,    80,    80,    80,     0,
      33,    21,     0,    80,    34,  1115,  1115,  1115,   268,     0,
     330,  1311,     0,     0,    26,     0,     0,    28,    29,     0,
       0,     0,   271,     8,     9,    10,   173,    12,    13,     0,
       0,   509,   482,     0,    14,     0,    80,    80,     0,    80,
       0,   510,     0,  1113,    80,    80,     0,    80,    16,    33,
      17,    18,     0,   511,     0,   211,     0,    20,     0,     0,
       0,     0,    21,     0,     0,     0,    23,     0,   715,     0,
       0,     0,     0,     0,     0,    26,     0,     0,    28,   272,
     209,     0,     0,     0,  1113,  1113,  1113,     0,     0,     0,
       0,     0,  1264,     0,     0,     0,     0,    54,     0,   209,
     209,   209,  1265,     0,     0,     0,     0,     0,     0,   520,
      33,     0,     0,     0,  1266,   719,     0,     8,     9,     0,
       0,    12,    13,     0,     0,     0,     0,     0,    14,   209,
       0,     0,  1012,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    16,     0,    17,    18,   722,   722,   722,     0,
       0,    20,     0,     0,     0,     0,   276,     0,     0,   722,
      23,     0,     0,     0,     0,     0,     0,   485,     0,    26,
       0,     0,   134,   135,     0,     0,   268,   674,   675,   676,
     677,   678,   679,   680,   681,   682,   683,   684,   685,   686,
     687,   688,    80,    80,    80,     0,     0,     0,     0,   813,
      80,    80,     0,     0,     0,     0,     0,     0,   271,     8,
       9,    10,   173,    12,    13,     0,     0,     0,   482,     0,
      14,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    80,    16,     0,    17,    18,     0,   813,
     813,  1107,     0,    20,     0,     0,     0,     0,    21,  1107,
       0,     0,    23,     0,   715,   275,     0,     0,     0,     0,
       0,    26,     0,    80,    28,   272,     0,     0,   211,     0,
       0,   848,   211,   211,   848,   848,   848,     0,  1300,     0,
     211,     0,     0,   268,   276,     0,     0,   211,  1301,     0,
       0,     0,     0,     0,     0,     0,    33,     0,     0,     0,
    1302,   719,     0,     0,     0,  -418,     8,     9,  -418,  -418,
      12,   254,     0,     0,     0,     0,     0,    14,   209,     0,
       0,   645,     0,     0,   145,     8,     9,    10,   603,   603,
     603,    16,     0,    17,    18,     0,     0,     0,     0,     0,
      20,     0,     0,     0,     0,  -418,     0,    80,     0,    23,
       0,   715,    80,    18,     0,    80,     0,     0,    26,     0,
       0,   134,   135,     0,    21,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   716,     0,    26,   722,     0,
      28,    29,     0,     0,     0,   717,   209,     0,   209,   209,
       0,     0,     0,  -418,   509,     0,     0,   718,   719,     0,
       0,     0,   209,   209,   510,     0,   209,     0,   209,   209,
     209,   209,    33,     0,     0,   209,   511,     0,     0,     0,
     209,     0,     0,   209,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   145,     8,     9,    10,     0,     0,   559,
       0,     0,     0,     0,     0,     0,     0,     0,   801,   485,
       0,     0,     0,     0,     0,     0,   722,   722,     0,     0,
     722,     0,    18,     0,     0,     0,     0,     0,     0,     0,
       0,   722,     0,    21,     0,     0,     0,   722,   722,   722,
       0,     0,     0,   722,     0,     0,    26,     0,     0,    28,
      29,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      80,    80,    80,   200,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   201,     0,     0,   268,   276,     0,     0,
       0,    33,     0,     0,   209,   202,     0,     0,   813,   813,
     813,  1107,  1107,  1107,  1303,     0,     0,     0,   813,     0,
    1107,  1107,  1107,     0,     0,     0,     0,     0,   145,     8,
       9,    10,   848,   848,   848,     0,     0,  1115,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   165,     0,     0,     0,     0,     0,    18,     0,   211,
       0,     0,     0,     0,     0,     0,     0,     0,    21,     0,
     485,     0,     0,     0,     0,     0,     0,     0,  1115,  1115,
    1115,    26,     0,     0,    28,    29,     0,   485,     0,     0,
     944,    80,     0,   913,   914,   915,   916,   917,   200,   918,
     919,   920,   921,   922,   923,   924,   925,   926,   201,     0,
       0,   268,   276,   960,   801,     0,    33,   972,   973,   974,
     202,   978,   979,   980,   981,   982,   983,   984,   985,   986,
     987,   988,   989,   990,   991,   992,   993,   994,   995,   996,
       0,   485,  1012,  1012,     0,  1012,     0,   485,   209,   485,
     485,     0,     0,     0,     0,     0,   722,     0,     0,     0,
       0,     0,     0,     0,   722,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   722,   722,   722,     0,     0,     0,
     485,     0,     0,     0,     0,     0,     0,   485,   209,   209,
    1105,     0,     0,     0,     0,   268,   276,     0,  1105,     0,
       0,     0,     0,     0,     0,     0,     0,   813,   813,   813,
    1303,  1303,  1303,     0,     0,   813,   813,   209,     0,     0,
     209,   209,   209,   209,   209,   209,   520,     0,     0,   209,
       0,   848,   848,   848,   848,   848,   209,   145,     8,     9,
      10,   212,    12,   213,     0,     0,     0,     0,   211,    14,
       0,   801,     0,     0,     0,     0,     0,     0,   276,     0,
       0,     0,     0,    16,     0,    17,    18,     0,     0,     0,
     485,     0,    20,     0,     0,     0,     0,    21,     0,     0,
       0,    23,     0,     0,   485,     0,     0,     0,     0,     0,
      26,     0,     0,    28,    29,     0,     0,   214,   271,     8,
       9,    10,     0,     0,   107,     0,     0,    31,     0,     0,
       0,     0,     0,   125,   107,     0,     0,    32,     0,     0,
       0,   107,   107,     0,   107,    33,     0,    18,     0,    34,
       0,   145,     8,     9,    10,   222,   223,   224,    21,  1303,
    1303,     0,  1303,    14,     0,     0,   848,     0,     0,     0,
       0,    26,     0,     0,    28,   272,     0,     0,     0,   243,
      18,     0,   211,     0,     0,     0,    20,     0,   844,     0,
       0,    21,     0,     0,     0,    23,     0,   715,   845,     0,
       0,     0,     0,     0,    26,     0,    33,    28,    29,  1201,
     846,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   200,     0,  1303,  1303,  1303,  1303,  1303,     0,     0,
       0,   201,     0,     0,     0,     0,     0,  1215,   403,    33,
     125,     0,     0,  1662,     0,     0,     0,   107,   107,     0,
       0,     0,     0,   268,   276,   268,   107,   107,     0,     0,
     107,   107,   107,     0,   435,   107,   107,   107,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   801,   485,     0,
       0,     0,  1303,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   485,     0,   485,     0,   485,   209,   209,   209,
     209,   209,  1105,   209,     0,  1304,     0,   209,   268,  1105,
    1105,  1105,     0,     0,     0,   276,     0,     0,     0,     0,
       0,   209,   209,   209,     0,     0,     0,     0,     0,     0,
       0,   485,     0,     0,     0,   211,     0,   211,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   209,     0,
       0,     0,     0,     0,     0,     0,   243,   107,     0,     0,
       0,     0,     0,     0,   720,     0,     0,   720,   720,   145,
       8,     9,    10,   212,    12,   213,     0,     0,     0,     0,
     107,    14,     0,     0,     0,     0,     0,     0,     0,   801,
     801,     0,   514,     0,     0,    16,     0,    17,    18,     0,
       0,     0,     0,     0,    20,     0,     0,     0,     0,    21,
       0,  1335,     0,    23,     0,   145,     8,     9,    10,   173,
      12,    13,    26,   107,     0,    28,    29,    14,     0,  1659,
       0,     0,     0,   514,   514,   514,     0,     0,     0,    31,
       0,    16,     0,    17,    18,     0,   211,     0,     0,    32,
      20,     0,     0,     0,     0,    21,     0,    33,     0,    23,
       0,    34,     0,     0,     0,     0,     0,     0,    26,     0,
       0,    28,    29,     0,     0,   720,     0,   720,   720,   720,
       0,     0,     0,     0,   107,    31,   107,   801,   801,   107,
       0,  1396,     0,     0,     0,    32,   209,   209,   209,   209,
     209,  1105,     0,    33,   209,   209,     0,    34,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   107,     0,     0,
     209,   209,   209,   209,   209,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   209,   107,     0,
     107,     0,     0,     0,     0,     0,     0,     0,   107,     0,
       0,   107,     0,     0,     0,     0,     0,     0,   520,     0,
       0,     0,     0,   107,     0,     0,   801,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     145,     8,     9,    10,   212,    12,   213,   271,     8,     9,
      10,     0,    14,    13,     0,     0,     0,   801,     0,     0,
       0,     0,     0,     0,     0,     0,    16,     0,    17,    18,
       0,     0,     0,     0,     0,    20,    18,     0,     0,     0,
      21,  1450,     0,     0,    23,     0,     0,    21,   209,   209,
    1304,   209,     0,    26,     0,   209,    28,    29,     0,     0,
      26,     0,     0,    28,   272,   145,   131,  1336,    10,     0,
      31,   209,     0,     0,   243,   872,     0,   844,     0,     0,
      32,     0,   801,     0,     0,     0,     0,   845,    33,     0,
     107,     0,    34,     0,    18,    33,     0,     0,     0,   846,
       0,     0,     0,     0,     0,    21,     0,     0,   485,     0,
       0,     0,   209,   209,  1105,   209,   209,   485,    26,     0,
       0,    28,    29,     0,   720,   720,   720,  1687,     0,   801,
       0,     0,   107,     0,   107,   155,     0,  1051,     0,  1508,
    1509,     0,     0,     0,     0,   156,     0,     0,     0,     0,
       0,     0,     0,    33,     0,     0,     0,   157,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     801,   209,     0,     0,     0,     0,     0,   720,     0,  1688,
     667,   668,   669,   670,   671,   672,   673,   674,   675,   676,
     677,   678,   679,   680,   681,   682,   683,   684,   685,   686,
     687,   688,     0,     0,     0,  1562,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   720,   720,   720,
      18,     0,     0,     0,   209,     0,   209,  1051,     0,     0,
       0,     0,   107,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   107,   107,     0,     0,   107,   107,     0,     0,
       0,  1619,  1620,     0,   667,   668,   669,   670,   671,   672,
     673,   674,   675,   676,   677,   678,   679,   680,   681,   682,
     683,   684,   685,   686,   687,   688,     0,     0,  1638,     0,
       0,  1105,     0,   107,     0,     0,     0,     0,     0,     0,
     107,   125,   910,   911,   912,   913,   914,   915,   916,   917,
     243,   918,   919,   920,   921,   922,   923,   924,   925,   926,
       0,     0,     0,     0,     0,     0,  1676,     0,     0,     0,
       0,     0,  1105,  1105,  1105,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   209,   672,   673,   674,   675,
     676,   677,   678,   679,   680,   681,   682,   683,   684,   685,
     686,   687,   688,   485,     0,     0,   720,   671,   672,   673,
     674,   675,   676,   677,   678,   679,   680,   681,   682,   683,
     684,   685,   686,   687,   688,     0,     0,     0,  1726,   667,
     668,   669,   670,   671,   672,   673,   674,   675,   676,   677,
     678,   679,   680,   681,   682,   683,   684,   685,   686,   687,
     688,     0,     0,     0,     0,     0,     0,     0,     0,   872,
     673,   674,   675,   676,   677,   678,   679,   680,   681,   682,
     683,   684,   685,   686,   687,   688,  1391,     0,   107,   107,
     107,   107,     0,     0,   720,   720,     0,     0,   720,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   720,
       0,     0,     0,     0,     0,  1051,  1051,  1051,     0,     0,
       0,   720,     0,     0,     0,     0,   906,   907,   908,   909,
     910,   911,   912,   913,   914,   915,   916,   917,   107,   918,
     919,   920,   921,   922,   923,   924,   925,   926,     0,   801,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   720,   720,   720,   720,
     720,   720,   720,     0,     0,     0,   720,     0,  1051,  1051,
    1051,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   650,     0,
     145,     8,     9,    10,   173,    12,   331,   332,   333,   334,
     482,   335,    14,     0,     0,     0,   107,   107,     0,     0,
     107,     0,     0,     0,     0,     0,    16,   336,    17,    18,
      19,   107,   337,   338,   339,    20,     0,   340,   341,   342,
      21,   343,   344,   107,    23,     0,   715,     0,   345,   346,
     347,   348,   349,    26,     0,     0,    28,    29,  -346,     0,
       0,   350,     0,     0,     0,     0,     0,   351,     0,     0,
    1110,     0,     0,     0,     0,     0,     0,     0,   353,   354,
    1111,     0,     0,   872,     0,     0,   356,   357,   358,     0,
       0,     0,  1112,   719,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   360,
       0,     0,     0,     0,   720,     0,     0,     0,     0,     0,
       0,     0,   720,     0,     0,   872,     0,     0,   107,     0,
       0,   107,   720,   720,   720,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   481,
       0,   271,     8,     9,    10,   173,    12,   331,   332,   333,
     334,   482,   335,    14,     0,   720,   720,   720,   720,   720,
     720,     0,     0,   720,   720,     0,     0,    16,   336,    17,
      18,    19,     0,   337,   338,   339,    20,     0,   340,   341,
     342,    21,   343,   344,     0,    23,     0,     0,     0,   345,
     346,   347,   348,   349,    26,     0,     0,    28,   272,     0,
       0,     0,   350,     0,     0,     0,     0,     0,   351,     0,
       0,   352,   107,   107,     0,   107,     0,     0,     0,   353,
     354,   355,     0,     0,     0,     0,   107,   356,   357,   358,
       0,     0,     0,   359,   107,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   107,   107,     0,     0,  -842,     0,
     360,     0,     0,     0,   667,   668,   669,   670,   671,   672,
     673,   674,   675,   676,   677,   678,   679,   680,   681,   682,
     683,   684,   685,   686,   687,   688,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   720,   720,     0,
     720,     0,     0,   271,     8,     9,    10,  1710,    12,   331,
     332,   333,   334,     0,   335,    14,     0,     0,     0,   872,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    16,
     336,    17,    18,    19,     0,   337,   338,   339,    20,     0,
     340,   341,   342,    21,   343,   344,     0,    23,     0,   715,
     107,   345,   346,   347,   348,   349,    26,     0,     0,    28,
     272,   720,   720,   720,   720,   720,     0,     0,     0,     0,
     351,     0,     0,  1047,     0,     0,     0,     0,     0,     0,
       0,   353,   354,  1048,     0,     0,     0,     0,     0,   356,
     357,   358,     0,     0,     0,  1049,   719,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   360,     0,   872,     0,     0,   666,     0,     0,
     720,   667,   668,   669,   670,   671,   672,   673,   674,   675,
     676,   677,   678,   679,   680,   681,   682,   683,   684,   685,
     686,   687,   688,     0,     0,   107,  1575,   107,  -929,  -929,
    -929,  -929,  -929,  -929,  -929,  -929,  -929,  -929,     0,  -929,
    -929,  -929,     0,  -929,  -929,  -929,  -929,  -929,  -929,  -929,
    -929,  -929,  -929,  -929,  -929,  -929,  -929,  -929,  -929,     0,
    -929,  -929,  -929,  -929,     0,  -929,  -929,  -929,  -929,  -929,
    -929,  -929,  -929,  -929,     0,     0,  -929,  -929,  -929,  -929,
    -929,  -929,     0,     0,  -929,  -929,  -929,     0,  -929,  -929,
       0,     0,     0,     0,     0,  -929,     0,     0,  -929,     0,
       0,     0,     0,     0,     0,     0,  -929,  -929,  -929,     0,
       0,     0,     0,     0,  -929,  -929,  -929,     0,     0,     0,
    -929,     0,  -929,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1576,  -929,  1537,     0,
    -929,  -929,  -929,  -929,  -929,  -929,  -929,  -929,  -929,  -929,
       0,  -929,  -929,  -929,     0,  -929,  -929,  -929,  -929,  -929,
    -929,  -929,  -929,  -929,  -929,  -929,  -929,  -929,  -929,  -929,
    -929,     0,  -929,  -929,  -929,  -929,     0,  -929,  -929,  -929,
    -929,  -929,  -929,  -929,  -929,  -929,     0,     0,  -929,  -929,
    -929,  -929,  -929,  -929,     0,     0,  -929,  -929,  -929,     0,
    -929,  -929,     0,     0,     0,     0,     0,  -929,     0,     0,
    -929,     0,     0,     0,     0,     0,     0,     0,  -929,  -929,
    -929,     0,     0,     0,     0,     0,  -929,  -929,  -929,     0,
       0,     0,  -929,   975,  -929,   271,     8,     9,    10,   173,
      12,   331,   332,   333,   334,     0,   335,    14,     0,  -929,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    16,   336,    17,    18,    19,     0,   337,   338,   339,
      20,     0,   340,   341,   342,    21,   343,   344,     0,    23,
       0,     0,     0,   345,   346,   347,   348,   349,    26,     0,
       0,    28,   272,  1721,     0,  -831,   350,     0,     0,     0,
       0,     0,   351,     0,     0,   352,     0,     0,     0,     0,
       0,     0,     0,   353,   354,   355,     0,     0,     0,     0,
       0,   356,   357,   358,     0,     0,   650,   359,   271,     8,
       9,    10,     0,    12,   331,   332,   333,   334,     0,   335,
      14,     0,     0,     0,   360,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    16,   336,    17,    18,    19,     0,
     337,   338,   339,    20,     0,   340,   341,   342,    21,   343,
     344,     0,    23,     0,     0,     0,   345,   346,   347,   348,
     349,    26,     0,     0,    28,   272,  -346,     0,     0,   350,
       0,     0,     0,     0,     0,   351,     0,     0,   651,     0,
       0,     0,     0,     0,     0,     0,   353,   354,   652,     0,
       0,     0,     0,     0,   356,   357,   358,     0,     0,   799,
     653,   961,   962,   963,    10,     0,    12,   498,   332,   333,
     334,     0,   335,    14,     0,     0,     0,   360,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    16,   336,    17,
       0,    19,     0,   337,   338,   339,    20,     0,   340,   341,
     342,    21,   343,   344,     0,    23,     0,     0,     0,   345,
     346,   347,   348,   349,    26,     0,     0,   964,   965,   800,
       0,     0,   350,     0,     0,     0,     0,     0,   351,     0,
       0,   352,     0,     0,     0,     0,     0,     0,     0,   353,
     354,   355,     0,     0,     0,     0,     0,   356,   357,   358,
       0,     0,     0,   359,   966,   799,     0,   271,     8,     9,
      10,     0,    12,   498,   332,   333,   334,     0,   335,    14,
     360,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    16,   336,    17,     0,    19,     0,   337,
     338,   339,    20,     0,   340,   341,   342,    21,   343,   344,
       0,    23,     0,     0,     0,   345,   346,   347,   348,   349,
      26,     0,     0,    28,   272,   800,     0,     0,   350,     0,
       0,     0,     0,     0,   351,     0,     0,   352,     0,     0,
       0,     0,     0,     0,     0,   353,   354,   355,     0,     0,
       0,     0,     0,   356,   357,   358,     0,     0,     0,   359,
     799,     0,   961,   962,   963,    10,  1314,    12,   498,   332,
     333,   334,     0,   335,    14,     0,   360,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    16,   336,
      17,     0,    19,     0,   337,   338,   339,    20,     0,   340,
     341,   342,    21,   343,   344,     0,    23,     0,     0,     0,
     345,   346,   347,   348,   349,    26,     0,     0,   964,   965,
     800,     0,     0,   350,     0,     0,     0,     0,     0,   351,
       0,     0,   352,     0,     0,     0,     0,     0,     0,     0,
     353,   354,   355,     0,     0,     0,     0,     0,   356,   357,
     358,     0,     0,   799,   359,   961,   962,   963,    10,     0,
      12,   498,   332,   333,   334,     0,   335,    14,     0,     0,
    -516,   360,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    16,   336,    17,     0,    19,     0,   337,   338,   339,
      20,     0,   340,   341,   342,    21,   343,   344,     0,    23,
       0,     0,     0,   345,   346,   347,   348,   349,    26,     0,
       0,   964,   965,   800,     0,     0,   350,     0,     0,     0,
       0,     0,   351,     0,     0,   352,     0,     0,     0,     0,
       0,     0,     0,   353,   354,   355,     0,     0,     0,     0,
       0,   356,   357,   358,     0,     0,   650,   359,   145,     8,
       9,    10,     0,    12,   331,   332,   333,   334,     0,   335,
      14,     0,     0,  1428,   360,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    16,   336,    17,    18,    19,     0,
     337,   338,   339,    20,     0,   340,   341,   342,    21,   343,
     344,     0,    23,     0,     0,     0,   345,   346,   347,   348,
     349,    26,     0,     0,    28,    29,  -346,     0,     0,   350,
       0,     0,     0,     0,     0,   351,     0,     0,  1703,     0,
       0,     0,     0,     0,     0,     0,   353,   354,  1704,     0,
       0,     0,     0,     0,   356,   357,   358,     0,     0,  1760,
    1705,   271,     8,     9,    10,     0,    12,   331,   332,   333,
     334,     0,   335,    14,     0,     0,     0,   360,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    16,   336,    17,
      18,    19,     0,   337,   338,   339,    20,     0,   340,   341,
     342,    21,   343,   344,     0,    23,     0,     0,     0,   345,
     346,   347,   348,   349,    26,     0,     0,    28,   272,     0,
       0,  -210,   350,     0,     0,     0,     0,     0,   351,     0,
       0,   352,     0,     0,     0,     0,     0,     0,     0,   353,
     354,   355,     0,     0,     0,     0,     0,   356,   357,   358,
       0,     0,   799,   359,   271,     8,     9,    10,     0,    12,
     498,   332,   333,   334,     0,   335,    14,     0,     0,     0,
     360,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      16,   336,    17,     0,    19,     0,   337,   338,   339,    20,
       0,   340,   341,   342,    21,   343,   344,     0,    23,     0,
       0,     0,   345,   346,   347,   348,   349,    26,     0,     0,
      28,   272,   800,     0,     0,   350,     0,     0,     0,     0,
       0,   351,     0,     0,   352,     0,     0,     0,     0,     0,
       0,     0,   353,   354,   355,     0,     0,     0,     0,     0,
     356,   357,   358,     0,     0,   975,   359,   271,     8,     9,
      10,     0,    12,   498,   332,   333,   334,     0,   335,    14,
       0,     0,     0,   360,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    16,   336,    17,     0,    19,     0,   337,
     338,   339,    20,     0,   340,   341,   342,    21,   343,   344,
       0,    23,     0,     0,     0,   345,   346,   347,   348,   349,
      26,     0,     0,    28,   272,     0,     0,     0,   350,  -831,
       0,     0,     0,     0,   351,     0,     0,   352,     0,     0,
       0,     0,     0,     0,     0,   353,   354,   355,     0,     0,
       0,     0,     0,   356,   357,   358,     0,     0,   975,   359,
     271,     8,     9,    10,     0,    12,   498,   332,   333,   334,
       0,   335,    14,     0,     0,     0,   360,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    16,   336,    17,     0,
      19,     0,   337,   338,   339,    20,     0,   340,   341,   342,
      21,   343,   344,     0,    23,     0,     0,     0,   345,   346,
     347,   348,   349,    26,     0,     0,    28,   272,     0,     0,
       0,   350,     0,     0,     0,     0,     0,   351,     0,     0,
     352,     0,     0,     0,     0,     0,     0,     0,   353,   354,
     355,     0,     0,     0,     0,     0,   356,   357,   358,     0,
       0,   959,   359,   271,     8,     9,    10,     0,    12,   498,
     332,   333,   334,     0,   335,    14,     0,  -831,     0,   360,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    16,
     336,    17,     0,    19,     0,   337,   338,   339,    20,     0,
     340,   341,   342,    21,   343,   344,     0,    23,     0,     0,
       0,   345,   346,   347,   348,   349,    26,     0,     0,    28,
     272,     0,     0,     0,   350,     0,     0,     0,     0,     0,
     351,     0,     0,   352,     0,     0,     0,     0,     0,     0,
       0,   353,   354,   355,     0,     0,     0,     0,     0,   356,
     357,   358,     0,     0,   971,   359,   271,     8,     9,    10,
       0,    12,   498,   332,   333,   334,     0,   335,    14,     0,
       0,     0,   360,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    16,   336,    17,     0,    19,     0,   337,   338,
     339,    20,     0,   340,   341,   342,    21,   343,   344,     0,
      23,     0,     0,     0,   345,   346,   347,   348,   349,    26,
       0,     0,    28,   272,     0,     0,     0,   350,     0,     0,
       0,     0,     0,   351,     0,     0,   352,     0,     0,     0,
       0,     0,     0,     0,   353,   354,   355,     0,     0,     0,
       0,     0,   356,   357,   358,     0,     0,  1675,   359,   271,
       8,     9,    10,     0,    12,   498,   332,   333,   334,     0,
     335,    14,     0,     0,     0,   360,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    16,   336,    17,     0,    19,
       0,   337,   338,   339,    20,     0,   340,   341,   342,    21,
     343,   344,     0,    23,     0,     0,     0,   345,   346,   347,
     348,   349,    26,     0,     0,    28,   272,     0,     0,     0,
     350,     0,     0,     0,     0,     0,   351,   278,     0,   352,
       8,     9,     0,     0,    12,    13,     0,   353,   354,   355,
       0,    14,     0,     0,     0,   356,   357,   358,     0,     0,
       0,   359,     0,     0,     0,    16,     0,    17,    18,     0,
       0,     0,     0,     0,    20,     0,   279,   280,   360,     0,
       0,     0,     0,    23,     0,   281,     0,     0,     0,     0,
       0,     0,    26,     0,     0,   134,   135,     0,   282,     0,
       0,     0,   283,   284,   285,   286,   287,   288,   289,   290,
     291,   292,   293,   294,   295,   296,   297,   298,   299,   300,
     301,   302,   303,     0,     0,   304,   305,   306,     0,   307,
       0,     0,   308,   271,     8,     9,    10,     0,    12,   498,
     332,   333,   334,     0,   335,    14,     0,     0,   309,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    16,
     336,    17,     0,    19,     0,   337,   338,   339,    20,     0,
     340,   341,   342,    21,   343,   344,     0,    23,     0,     0,
       0,   345,   346,   347,   348,   349,    26,     0,     0,    28,
     272,     0,     0,     0,   350,     0,     0,     0,     0,     0,
     351,     0,     0,   352,     0,     0,     0,     0,     0,     0,
       0,   353,   354,   355,     0,     0,     0,     0,     0,   356,
     357,   358,     0,     0,     0,   359,   271,     8,     9,    10,
       0,    12,   498,   332,   333,   334,     0,   335,    14,     0,
       0,     0,   360,   499,     0,     0,     0,     0,     0,     0,
       0,     0,    16,   336,    17,     0,    19,     0,   337,   338,
     339,    20,     0,   340,   341,   342,    21,   343,   344,     0,
      23,     0,     0,     0,   345,   346,   347,   348,   349,    26,
       0,     0,    28,   272,     0,     0,     0,   350,     0,     0,
       0,     0,     0,   351,     0,     0,   352,     0,     0,     0,
       0,     0,     0,     0,   353,   354,   355,     0,     0,     0,
       0,     0,   356,   357,   358,     0,     0,     0,   359,   271,
       8,     9,    10,     0,    12,   498,   332,   333,   334,     0,
     335,    14,     0,     0,     0,   360,   854,     0,     0,     0,
       0,     0,     0,     0,     0,    16,   336,    17,     0,    19,
       0,   337,   338,   339,    20,     0,   340,   341,   342,    21,
     343,   344,     0,    23,     0,     0,     0,   345,   346,   347,
     348,   349,    26,     0,     0,    28,   272,     0,     0,     0,
     350,     0,     0,     0,     0,     0,   351,     0,     0,   352,
       0,     0,     0,     0,     0,     0,     0,   353,   354,   355,
       0,     0,     0,     0,     0,   356,   357,   358,     0,     0,
       0,   359,   271,     8,     9,    10,     0,    12,   498,   332,
     333,   334,     0,   335,    14,     0,     0,     0,   360,  1015,
       0,     0,     0,     0,     0,     0,     0,     0,    16,   336,
      17,     0,    19,     0,   337,   338,   339,    20,     0,   340,
     341,   342,    21,   343,   344,     0,    23,     0,     0,     0,
     345,   346,   347,   348,   349,    26,     0,     0,    28,   272,
       0,     0,     0,   350,     0,     0,     0,     0,     0,   351,
       0,     0,   352,     0,     0,     0,     0,     0,     0,     0,
     353,   354,   355,     0,     0,     0,     0,     0,   356,   357,
     358,     0,     0,     0,   359,   271,     8,     9,    10,     0,
      12,   498,   332,   333,   334,     0,   335,    14,     0,     0,
       0,   360,  1035,     0,     0,     0,     0,     0,     0,     0,
       0,    16,   336,    17,     0,    19,     0,   337,   338,   339,
      20,     0,   340,   341,   342,    21,   343,   344,     0,    23,
       0,     0,     0,   345,   346,   347,   348,   349,    26,     0,
       0,    28,   272,     0,     0,     0,   350,     0,     0,     0,
       0,     0,   351,     0,     0,   352,     0,     0,     0,     0,
       0,     0,     0,   353,   354,   355,     0,     0,     0,     0,
       0,   356,   357,   358,     0,     0,     0,   359,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   360,  1258,  1579,  1580,  1581,    10,
     173,    12,   331,   332,   333,   334,     0,   335,    14,  1582,
       0,  1583,  1584,  1585,  1586,  1587,  1588,  1589,  1590,  1591,
    1592,    15,    16,   336,    17,    18,    19,     0,   337,   338,
     339,    20,     0,   340,   341,   342,    21,   343,   344,  1593,
      23,  1594,     0,     0,   345,   346,   347,   348,   349,    26,
       0,     0,  1595,   272,  1214,     0,  1596,   350,     0,     0,
       0,     0,     0,   351,     0,     0,   352,     0,     0,     0,
       0,     0,     0,     0,   353,   354,   355,     0,     0,     0,
       0,     0,   356,   357,   358,     0,     0,     0,   359,     0,
    1597,  1579,  1580,  1581,    10,   173,    12,   331,   332,   333,
     334,     0,   335,    14,  1582,   360,  1583,  1584,  1585,  1586,
    1587,  1588,  1589,  1590,  1591,  1592,    15,    16,   336,    17,
      18,    19,     0,   337,   338,   339,    20,     0,   340,   341,
     342,    21,   343,   344,  1593,    23,  1594,     0,     0,   345,
     346,   347,   348,   349,    26,     0,     0,  1595,   272,     0,
       0,  1596,   350,     0,     0,     0,     0,     0,   351,     0,
       0,   352,     0,     0,     0,     0,     0,     0,     0,   353,
     354,   355,     0,     0,     0,     0,     0,   356,   357,   358,
       0,     0,     0,   359,     0,  1597,   271,     8,     9,    10,
     173,    12,   331,   332,   333,   334,   482,   335,    14,     0,
     360,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    16,   336,    17,    18,    19,     0,   337,   338,
     339,    20,     0,   340,   341,   342,    21,   343,   344,     0,
      23,     0,   715,     0,   345,   346,   347,   348,   349,    26,
       0,     0,    28,   272,     0,     0,     0,   350,     0,     0,
       0,     0,     0,   351,     0,     0,  1047,     0,     0,     0,
       0,     0,     0,     0,   353,   354,  1048,     0,     0,     0,
       0,     0,   356,   357,   358,     0,     0,     0,  1049,   719,
     145,     8,     9,    10,   173,    12,   331,   332,   333,   334,
     482,   335,    14,     0,     0,   360,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    16,   336,    17,    18,
      19,     0,   337,   338,   339,    20,     0,   340,   341,   342,
      21,   343,   344,     0,    23,     0,   715,     0,   345,   346,
     347,   348,   349,    26,     0,     0,    28,    29,     0,     0,
       0,   350,     0,     0,     0,     0,     0,   351,     0,     0,
    1110,     0,     0,     0,     0,     0,     0,     0,   353,   354,
    1111,     0,     0,     0,     0,     0,   356,   357,   358,     0,
       0,     0,  1112,   719,   145,     8,     9,    10,     0,    12,
     331,   332,   333,   334,     0,   335,    14,     0,     0,   360,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      16,   336,    17,    18,    19,     0,   337,   338,   339,    20,
       0,   340,   341,   342,    21,   343,   344,     0,    23,     0,
     715,     0,   345,   346,   347,   348,   349,    26,     0,     0,
      28,    29,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   351,     0,     0,  1110,     0,     0,     0,     0,     0,
       0,     0,   353,   354,  1111,     0,     0,     0,     0,     0,
     356,   357,   358,     0,     0,     0,  1112,   719,   271,     8,
       9,    10,     0,    12,   331,   332,   333,   334,     0,   335,
      14,     0,     0,   360,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    16,   336,    17,    18,    19,     0,
     337,   338,   339,    20,     0,   340,   341,   342,    21,   343,
     344,     0,    23,     0,     0,     0,   345,   346,   347,   348,
     349,    26,     0,     0,    28,   272,     0,     0,     0,   350,
       0,     0,     0,     0,     0,   351,     0,     0,   352,     0,
       0,     0,     0,     0,     0,     0,   353,   354,   355,     0,
       0,     0,     0,     0,   356,   357,   358,     0,     0,     0,
     359,   271,     8,     9,    10,     0,    12,   331,   332,   333,
     334,     0,   335,    14,     0,     0,     0,   360,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    16,   336,    17,
      18,    19,     0,   337,   338,   339,    20,     0,   340,   341,
     342,    21,   343,   344,     0,    23,     0,     0,     0,   345,
     346,   347,   348,   349,    26,     0,     0,   611,   272,     0,
       0,     0,   612,     0,     0,     0,     0,     0,   351,     0,
       0,   352,     0,     0,     0,     0,     0,     0,     0,   353,
     354,   355,     0,     0,     0,     0,     0,   356,   357,   358,
       0,     0,     0,   359,   271,     8,     9,    10,     0,    12,
     498,   332,   333,   334,     0,   335,    14,     0,     0,     0,
     360,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      16,   336,    17,    18,    19,     0,   337,   338,   339,    20,
       0,   340,   341,   342,    21,   343,   344,     0,    23,     0,
       0,     0,   345,   346,   347,   348,   349,    26,     0,     0,
      28,   272,     0,     0,     0,   350,     0,     0,     0,     0,
       0,   351,     0,     0,   651,     0,     0,     0,     0,     0,
       0,     0,   353,   354,   652,     0,     0,     0,     0,     0,
     356,   357,   358,     0,     0,     0,   653,   271,     8,     9,
      10,     0,    12,   498,   332,   333,   334,     0,   335,    14,
       0,     0,     0,   360,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    16,   336,    17,     0,    19,     0,   337,
     338,   339,    20,     0,   340,   341,   342,    21,   343,   344,
       0,    23,     0,     0,     0,   345,   346,   347,   348,   349,
      26,     0,     0,    28,   272,     0,     0,  1642,   350,     0,
       0,     0,     0,     0,   351,     0,     0,   352,     0,     0,
       0,     0,     0,     0,     0,   353,   354,   355,     0,     0,
       0,     0,     0,   356,   357,   358,     0,     0,     0,   359,
     271,     8,     9,    10,   173,    12,   331,   332,   333,   334,
       0,   335,    14,     0,     0,     0,   360,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    16,   336,    17,    18,
      19,     0,   337,   338,   339,    20,     0,   340,   341,   342,
      21,   343,   344,     0,    23,     0,     0,     0,   345,   346,
     347,   348,   349,    26,     0,     0,    28,   272,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   351,     0,     0,
     352,     0,     0,     0,     0,     0,     0,     0,   353,   354,
     355,     0,     0,     0,     0,     0,   356,   357,   358,     0,
       0,     0,   359,   145,     8,     9,    10,     0,    12,   498,
     332,   333,   334,     0,   335,    14,     0,     0,     0,   360,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    16,
     336,    17,    18,    19,     0,   337,   338,   339,    20,     0,
     340,   341,   342,    21,   343,   344,     0,    23,     0,     0,
       0,   345,   346,   347,   348,   349,    26,     0,     0,    28,
      29,     0,     0,     0,   350,     0,     0,     0,     0,     0,
     351,     0,     0,  1703,     0,     0,     0,     0,     0,     0,
       0,   353,   354,  1704,     0,     0,     0,     0,     0,   356,
     357,   358,     0,     0,     0,  1705,   271,     8,     9,    10,
       0,    12,   498,   332,   333,   334,     0,   335,    14,     0,
       0,     0,   360,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    16,   336,    17,     0,    19,     0,   337,   338,
     339,    20,     0,   340,   341,   342,    21,   343,   344,     0,
      23,     0,     0,     0,   345,   346,   347,   348,   349,    26,
       0,     0,    28,   272,     0,     0,     0,   350,     0,     0,
       0,     0,     0,   351,     0,     0,   352,     0,     0,     0,
       0,     0,     0,     0,   353,   354,   355,     0,     0,     0,
       0,     0,   356,   357,   358,     0,     0,     0,   359,   271,
       8,     9,    10,     0,    12,   498,   332,   333,   334,     0,
     335,    14,     0,     0,     0,   360,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    16,   336,    17,     0,    19,
       0,   337,   338,   339,    20,     0,   340,   341,   342,    21,
     343,   344,     0,    23,     0,     0,     0,   345,   346,   347,
     348,   349,    26,     0,     0,    28,   272,   663,     0,     0,
       0,     0,     0,     0,     0,     0,   351,     0,     0,   352,
       0,     0,     0,     0,     0,     0,     0,   353,   354,   355,
       0,     0,     0,     0,     0,   356,   357,   358,     0,     0,
       0,   664,   271,     8,     9,    10,     0,    12,   498,   332,
     333,   334,     0,   335,    14,     0,     0,     0,   360,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    16,   336,
      17,     0,    19,     0,   337,   338,   339,    20,     0,   340,
     341,   342,    21,   343,   344,     0,    23,     0,     0,     0,
     345,   346,   347,   348,   349,    26,     0,     0,    28,   272,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   351,
       0,     0,   352,     0,     0,     0,     0,     0,     0,     0,
     353,   354,   355,     0,     0,     0,     0,     0,   356,   357,
     358,     0,     0,     0,   359,   703,   271,     8,     9,    10,
       0,    12,   498,   332,   333,   334,     0,   335,    14,     0,
       0,   360,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    16,   336,    17,     0,    19,     0,   337,   338,
     339,    20,     0,   340,   341,   342,    21,   343,   344,     0,
      23,     0,     0,     0,   345,   346,   347,   348,   349,    26,
       0,     0,    28,   272,     0,     0,     0,   612,     0,     0,
       0,     0,     0,   351,     0,     0,   352,     0,     0,     0,
       0,     0,     0,     0,   353,   354,   355,     0,     0,     0,
       0,     0,   356,   357,   358,     0,     0,     0,   359,   271,
       8,     9,    10,     0,    12,   498,   332,   333,   334,     0,
     335,    14,     0,     0,     0,   360,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    16,   336,    17,    18,    19,
       0,   337,   338,   339,    20,     0,   340,   341,   342,    21,
     343,   344,     0,    23,     0,     0,     0,   345,   346,   347,
     348,   349,    26,     0,     0,    28,   272,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   351,     0,     0,   651,
       0,     0,     0,     0,     0,     0,     0,   353,   354,   652,
       0,     0,     0,     0,     0,   356,   357,   358,     0,     0,
       0,   653,  1270,     8,     9,    10,     0,    12,   498,   332,
     333,   334,     0,   335,    14,     0,     0,     0,   360,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    16,   336,
      17,     0,    19,     0,   337,   338,   339,    20,     0,   340,
     341,   342,    21,   343,   344,     0,    23,     0,     0,     0,
     345,   346,   347,   348,   349,    26,     0,     0,    28,   272,
       0,     0,     0,   350,     0,     0,     0,     0,     0,   351,
       0,     0,   352,     0,     0,     0,     0,     0,     0,     0,
     353,   354,   355,     0,     0,     0,     0,     0,   356,   357,
     358,     0,     0,     0,   359,   145,     8,     9,    10,     0,
      12,   331,   332,   333,   334,     0,   335,    14,     0,     0,
       0,   360,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    16,   336,    17,    18,    19,     0,   337,   338,   339,
      20,     0,   340,   341,   342,    21,   343,   344,     0,    23,
       0,     0,     0,   345,   346,   347,   348,   349,    26,     0,
       0,    28,    29,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   351,     0,     0,  1703,     0,     0,     0,     0,
       0,     0,     0,   353,   354,  1704,     0,     0,     0,     0,
       0,   356,   357,   358,     0,     0,     0,  1705,   271,     8,
       9,    10,     0,    12,   498,   332,   333,   334,     0,   335,
      14,     0,     0,     0,   360,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    16,   336,    17,     0,    19,     0,
     337,   338,   339,    20,     0,   340,   341,   342,    21,   343,
     344,     0,    23,     0,     0,     0,   345,   346,   347,   348,
     349,    26,     0,     0,    28,   272,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   351,     0,     0,   352,     0,
       0,     0,     0,     0,     0,     0,   353,   354,   355,     0,
       0,     0,     0,     0,   356,   357,   358,     0,     0,     0,
     359,   271,     8,     9,    10,     0,    12,   498,   332,   333,
     334,     0,   335,    14,     0,     0,     0,   360,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    16,   336,    17,
       0,    19,     0,   337,   338,   339,    20,     0,   340,   341,
     342,    21,   343,   344,     0,    23,     0,     0,     0,   345,
     346,   347,   348,   349,    26,     0,     0,    28,   272,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   351,     0,
       0,   352,     0,     0,     0,     0,     0,     0,     0,   353,
     354,   355,     0,     0,     0,     0,     0,   356,   357,   358,
       0,     0,     0,   725,   271,     8,     9,    10,     0,    12,
     498,   332,   333,   334,     0,   335,    14,     0,     0,     0,
     360,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      16,   336,    17,     0,    19,     0,   337,   338,   339,    20,
       0,   340,   341,   342,    21,   343,   344,     0,    23,     0,
       0,     0,   345,   346,   347,   348,   349,    26,     0,     0,
      28,   272,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   351,     0,     0,   352,     0,     0,     0,     0,     0,
       0,     0,   353,   354,   355,     0,     0,     0,     0,     0,
     356,   357,   358,     0,     0,     0,   727,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    1205,     0,     0,   360,   667,   668,   669,   670,   671,   672,
     673,   674,   675,   676,   677,   678,   679,   680,   681,   682,
     683,   684,   685,   686,   687,   688,  1747,   667,   668,   669,
     670,   671,   672,   673,   674,   675,   676,   677,   678,   679,
     680,   681,   682,   683,   684,   685,   686,   687,   688,   667,
     668,   669,   670,   671,   672,   673,   674,   675,   676,   677,
     678,   679,   680,   681,   682,   683,   684,   685,   686,   687,
     688
};

static const short yycheck[] =
{
      14,     4,    60,     4,   180,   182,   216,    23,    22,   794,
     181,   545,   417,   417,   252,     4,   130,   908,   417,    49,
     385,    82,     4,   155,   156,     4,   379,   137,   138,     4,
     320,   321,   417,    36,    45,    36,    82,   610,   477,   162,
     366,   385,   196,   138,    45,   255,    45,    36,    78,   261,
    1004,   377,   378,   622,    36,    69,    45,    36,    87,    60,
    1421,    36,   387,    45,   459,   770,    45,     4,    58,   883,
      45,   246,  1427,    74,  1142,   889,  1144,   216,   796,   128,
    1543,   137,   140,    45,  1152,   776,    87,   226,    45,   150,
     495,   495,  1543,  1538,    12,    34,  1707,    45,  1686,    36,
       9,   311,     1,    61,     4,    39,  1717,  1718,    45,  1751,
    1598,   112,     4,    49,   128,    63,   526,    40,    65,    62,
       1,  1783,    32,    61,   182,     3,   196,   225,   226,    67,
      77,    31,    32,    56,   424,   425,    36,   490,   548,   140,
     154,    99,  1784,  1805,    36,    45,  1614,     0,   501,   210,
      87,   140,  1763,     4,    14,    64,  1744,    13,   140,    95,
     171,   140,    61,    62,   175,    13,  1611,  1635,   111,   379,
     171,    34,   111,   385,   175,   112,   175,   111,    96,   141,
      61,   182,   171,  1651,    65,    36,   175,    87,    62,   171,
     220,   181,   171,   175,    45,   196,   175,  1685,     7,    77,
     175,   188,     1,   140,  1815,     0,    62,    34,   777,    65,
      65,    65,   112,   175,    62,    62,    97,    65,   175,    67,
     221,    49,    77,    77,    49,    85,    95,   175,    97,  1043,
      39,   277,   246,   247,   171,    49,   110,   147,   175,  1707,
     140,    63,   152,   934,  1689,    77,   937,   261,    28,  1717,
    1718,   111,    32,   618,   110,   155,   156,  1720,    63,  1444,
    1445,    62,    61,   110,   374,   323,    65,    95,   216,  1720,
      95,   171,  1226,   142,   618,   175,  1461,   225,   226,   171,
     516,    95,  1727,    63,   221,   110,    49,    67,   524,   111,
     141,   201,   155,   156,   157,  1763,   110,    13,    97,    49,
     200,   201,  1232,  1233,   109,  1235,   111,   255,     4,   110,
      77,   312,   407,   327,   437,    49,    95,   318,   374,  1764,
      63,   221,   323,    63,   175,    62,  1511,   112,   155,   156,
     157,   111,    95,   623,   323,  1520,  1521,   351,  1523,   202,
      36,   323,   371,    77,   323,    95,    62,  1815,    62,    65,
     399,  1165,     1,  1167,     3,     4,     5,    62,   359,    56,
     270,    95,    96,   311,   763,    50,  1721,  1072,   111,   544,
     371,   111,   386,   110,    49,   202,   110,   509,   510,   380,
      77,   318,    95,    56,   523,   399,   323,    77,  1749,   512,
    1081,  1082,  1460,    26,   789,   790,   110,   257,   742,    65,
      49,     3,     4,     5,    77,   110,   618,    56,   478,    49,
      59,    60,   197,   110,     4,     5,   417,    95,   318,   420,
      95,    96,   359,   323,   522,   523,    56,   752,    61,   419,
      49,   379,    65,   531,   371,   110,   221,  1622,  1623,    26,
      49,    28,    49,    56,   110,  1399,    95,    77,    50,   547,
     461,    63,    65,   322,   109,    95,    96,    59,    60,   359,
     461,    26,   461,    26,    97,    28,    56,     4,     5,    59,
      60,   371,   461,   421,    61,    56,    95,    96,    65,   461,
      49,    83,   461,    73,    65,    63,    95,   488,    95,   779,
     491,   110,    49,    83,   495,   707,    13,   487,    61,   489,
     490,   599,    65,  1076,   534,    56,    96,  1198,  1199,    49,
      97,    28,    95,   461,    56,    32,     3,     4,     5,     6,
    1334,    32,    59,    60,   461,   385,    95,    96,   438,   110,
     544,   441,    49,  1347,    97,    77,     4,     5,    95,    96,
      65,     9,   490,   453,    65,    62,    63,    77,    65,   619,
      67,     4,     5,   110,    49,    95,    43,   426,    61,  1093,
    1129,   461,    65,    26,    32,    28,   636,  1006,    56,    56,
      49,    62,    59,    60,   522,   523,    63,    65,    95,    96,
       3,     4,     5,   531,    49,   110,    73,    78,    56,     4,
       5,    59,    60,   110,   504,     3,    83,   507,    61,   547,
      95,  1135,    65,    56,    91,    73,    59,    60,    95,   509,
     510,   521,   788,   903,    61,    83,    95,    96,    49,    61,
     690,    61,   110,    65,    49,    65,   696,    95,   698,   699,
      95,   110,   870,   871,    97,   625,    59,    60,   876,   640,
     641,   642,   643,   644,    59,    60,   509,   510,   511,   663,
     760,   599,   653,    56,    49,    56,    49,    28,   753,    95,
      83,    32,    49,   664,    95,   760,   736,   536,  1073,  1073,
      95,   700,    67,    49,  1073,    26,    49,    28,   626,  1118,
    1119,    56,   509,   510,   511,    49,    86,    87,  1073,  1503,
    1055,    62,    63,   707,    67,    78,    67,    80,   699,   700,
      95,    96,    95,   640,   641,   642,   643,   644,    95,    96,
      61,  1055,    62,    63,    65,   927,   653,   718,   841,    95,
      96,    56,    95,   110,   725,  1051,   727,   664,   729,   883,
      49,    95,    96,   518,   110,   889,    78,   606,   948,    50,
     640,   641,   642,   643,   644,   761,    97,   761,    67,    56,
      49,  1116,    49,   653,    77,   816,   770,  1475,   618,     4,
       5,   859,   699,   700,   664,   879,    56,     8,     9,    78,
      77,    80,  1116,   110,    15,  1065,    95,  1216,  1217,   769,
       1,    56,     3,     4,     5,    49,   800,    77,   725,    28,
     727,    32,    56,    32,  1685,  1329,    95,    38,    95,   699,
     700,    46,    77,    49,   110,     4,    47,   110,    49,    56,
     811,    56,    56,    77,    59,    60,   716,   717,   819,    49,
    1538,   769,   880,    62,    63,   725,   110,   727,    49,    12,
      77,    95,   111,    77,   880,    56,    56,    36,    59,    60,
      61,    62,    56,  1055,    62,    65,    45,     4,     5,    95,
      96,    65,     9,    63,    95,   640,  1295,    77,    95,    58,
      78,    60,    80,    77,   110,    95,    96,     4,     5,  1159,
    1160,  1161,     9,   733,    95,    32,    99,   864,  1231,   880,
     110,     3,   883,    63,   669,    77,   755,  1326,   889,  1043,
    1424,   880,    49,  1611,  1132,  1133,  1134,  1788,   880,    56,
     810,    61,    59,    60,  1116,   880,     4,     5,    56,   809,
     810,   859,   697,   927,  1090,    65,    73,    65,   703,    56,
      96,    49,    59,    60,    62,    63,    83,    77,    56,    77,
    1053,    96,   842,   843,   719,    65,    73,  1007,    95,    96,
     850,   140,   113,   880,   844,   845,    83,   146,   811,    77,
    1735,  1021,  1391,  1023,    62,  1025,   819,   817,    56,    96,
      95,    59,    60,    46,     9,    56,  1684,    95,    13,  1101,
    1102,  1689,   171,    56,    95,    73,   175,    60,  1110,  1111,
     880,  1104,   181,   182,   811,    83,    49,    56,    95,    56,
    1060,    95,   819,    56,    62,    63,    65,    77,    65,  1438,
     948,    77,  1003,    77,    49,    77,  1007,   110,    77,  1727,
      77,  1165,    62,  1167,    77,   884,   885,    62,   887,    64,
      65,   110,    67,     3,     4,     5,     4,     5,    56,    62,
      63,   230,    95,    78,    77,    80,  1037,    65,    62,    63,
    1479,    77,  1043,    56,  1058,  1106,  1764,    62,  1049,    77,
      95,    96,    67,   110,    32,   898,   899,   900,  1072,     7,
       8,     9,    77,    78,   110,   110,  1003,    15,    62,    63,
     111,    49,  1073,    77,  1075,    12,    56,  1093,    56,    59,
      60,    59,    60,    65,  1142,   111,  1144,    86,    87,   110,
      38,  1149,  1150,   110,  1152,    73,  1142,    67,  1144,    47,
    1037,   110,  1103,  1003,  1150,    83,  1152,    62,    63,    62,
      63,  1112,    62,    63,   110,  1145,  1146,    95,    96,  1409,
    1121,    62,    63,   908,   323,  1155,   113,     4,     5,     3,
       4,     5,    83,    84,    85,    86,    87,  1037,  1075,    67,
      28,  1142,   110,  1144,  1549,  1549,   110,  1047,  1048,  1150,
      77,  1152,    78,  1142,    77,  1144,    56,    77,    62,  1149,
    1142,   113,  1144,  1152,  1165,  1144,  1167,  1142,   113,  1144,
    1152,     3,    49,    77,    78,  1075,    80,  1152,    63,    56,
    1334,    67,    59,    60,  1121,    59,    60,    49,    62,  1099,
    1100,    62,   111,  1347,   110,  1055,    73,   110,  1108,   110,
     110,  1101,  1102,  1217,  1205,  1142,    83,  1144,     4,     5,
    1110,  1111,    56,     9,     8,  1152,    95,    95,    95,    96,
     419,  1121,    49,    56,   111,    63,  1011,    89,    90,   428,
      77,    93,    94,    95,    96,    58,    32,    38,  1101,  1102,
    1103,    67,  1142,    67,  1144,    67,   110,  1110,  1111,  1112,
     110,    63,  1152,    49,  1607,   110,  1116,   110,    67,  1044,
      56,    67,   461,    59,    60,  1266,   111,   113,  1205,    83,
      84,    85,    86,    87,  1101,  1102,  1103,    73,  1141,    78,
    1338,  1295,    62,  1110,  1111,  1112,    78,    83,   487,   488,
     489,   490,   110,   110,   110,   110,   495,    34,  1359,    95,
      96,  1302,    78,    78,   503,  1205,    78,   110,   110,    61,
    1368,    65,    65,  1359,  1141,    62,    65,   516,   113,   110,
     110,  1351,  1368,   110,   110,   524,   111,  1357,  1358,   110,
      96,  1361,  1362,  1334,   113,  1365,    91,    77,    49,   110,
     110,   110,     3,     4,     5,     6,  1347,  1417,  1338,  1503,
     113,   110,   110,  1340,  1341,   111,  1426,    63,   181,  1798,
     110,   560,  1715,    61,  1264,  1265,   110,  1368,   110,   376,
      67,  1240,   379,   380,    31,    32,   110,    34,   110,  1368,
    1367,   110,    43,     4,     5,  1372,  1368,    65,  1298,  1299,
      95,    95,    95,  1368,    95,  1305,   111,   110,    59,    60,
    1300,  1301,  1460,    60,   113,     8,    63,   230,  1424,  1455,
     110,    32,    69,    62,  1460,   110,   110,    74,   110,   156,
     157,  1321,  1322,   110,   113,   110,   625,    98,    49,  1606,
      91,  1368,    62,  1434,  1605,    56,  1607,   110,    59,    60,
      81,    82,    83,    84,    85,    86,    87,     3,    65,    56,
       6,  1236,    73,     3,     4,     5,     6,   110,  1472,  1460,
     110,    65,    83,  1450,  1451,   202,  1453,  1454,  1368,  1456,
     110,  1460,    49,   110,    95,    96,    32,   113,  1460,    67,
     487,   110,   489,   490,   491,  1460,   110,    43,  1518,  1519,
     147,   110,   110,    43,  1495,   152,   110,  1434,   155,   156,
     157,    34,  1503,    59,    60,  1715,    56,   113,   110,    59,
      60,    95,   110,   110,    65,    63,    65,    73,    63,     9,
      63,  1508,  1509,  1460,    17,   182,   111,    83,   110,   110,
      61,   188,    95,    95,  1434,    91,  1712,    63,    63,    95,
    1543,    91,  1543,   200,   201,   202,    67,    56,  1549,    95,
     109,     3,     4,     5,  1543,    18,    63,  1571,    56,   216,
    1460,  1543,   104,    12,  1543,    95,   110,    63,  1543,   226,
     769,  1703,  1704,   110,    63,  1562,  1563,  1564,  1592,  1593,
      63,     3,     4,     5,     6,   196,    63,    62,    67,     3,
       4,     5,     6,  1493,  1494,   110,   419,  1598,   255,   113,
      12,  1386,  1387,  1388,  1389,  1606,  1543,    59,    60,  1598,
      63,   110,  1397,   270,    12,  1605,  1598,  1607,    63,  1598,
      12,    43,   359,  1598,   110,    95,    40,    41,    50,    43,
      63,  1661,  1619,  1620,  1648,  1649,    63,    59,    60,   838,
      63,   110,    56,  1543,   856,    59,    60,    81,    82,    83,
      84,    85,    86,    87,   311,   312,    63,     0,   857,  1607,
       0,  1598,     0,   176,   487,     2,   489,   490,    36,    91,
     882,     3,     4,     5,   753,  1649,   888,    91,  1679,  1073,
     503,   880,  1424,  1697,  1685,  1686,   171,   175,   461,   512,
       3,     4,     5,   516,  1711,  1150,  1685,    99,  1598,  1634,
     144,   524,   359,  1685,  1705,   314,  1685,   800,  1738,  1058,
    1685,   448,   449,  1329,   632,  1135,  1368,  1720,    50,  1720,
    1150,  1457,   379,   380,   940,   634,  1716,    59,    60,   700,
     374,  1720,     3,     4,     5,     6,   207,    50,  1720,   128,
    1538,  1720,  1679,  1744,  1476,  1720,    59,    60,  1685,  1686,
     657,    83,  1766,  1720,  1735,     3,     4,     5,  1543,  1732,
    1782,    -1,   769,   420,   421,  1434,   828,  1715,  1705,  1756,
      83,    -1,    43,   510,   511,    -1,    -1,    -1,    -1,  1679,
      -1,   438,    -1,  1720,   441,  1685,  1686,    -1,    59,    60,
     447,   448,   449,    -1,    -1,    -1,   453,    -1,    -1,  1662,
      -1,    -1,   625,  1703,  1704,  1705,  1591,  1744,    56,    -1,
      -1,    59,    60,    61,    -1,     3,     4,     5,     6,    -1,
    1720,    -1,    -1,     3,     4,     5,  1038,    -1,  1040,    -1,
      -1,   488,    -1,   490,   491,  1662,    -1,    -1,    -1,    -1,
    1703,  1704,  1705,    -1,  1744,    -1,    -1,   504,    -1,    -1,
     507,    -1,   509,   510,   511,    43,    -1,    -1,   515,  1644,
      -1,    49,    -1,    -1,   521,    -1,   523,   478,    56,   526,
      -1,    59,    60,    -1,    -1,    -1,  1703,  1704,  1705,    59,
      60,    61,    -1,    -1,    -1,    73,    -1,    -1,    -1,    -1,
     547,   548,     4,     5,  1679,    83,    -1,     9,    -1,  1098,
    1685,  1686,    -1,    91,    -1,    -1,    -1,    95,    96,     3,
       4,     5,     6,    -1,    -1,   652,   653,    -1,    -1,    -1,
      32,     3,     4,     5,    -1,    -1,    -1,   664,     3,     4,
       5,    -1,     7,     8,     9,  1720,    -1,    -1,    32,    -1,
      -1,   948,   599,  1142,    56,  1144,   769,    59,    60,    43,
    1149,  1150,  1164,  1152,  1166,    49,    -1,    32,    -1,  1744,
      -1,    73,    56,    38,    -1,    59,    60,    -1,    50,   626,
      -1,    83,    -1,  1758,    -1,    -1,    -1,    59,    60,    73,
      -1,    56,    -1,    95,    59,    60,    61,    -1,   725,    83,
     727,    -1,   729,  1778,   651,   652,   653,    91,     3,     4,
       5,    95,    96,  1788,    -1,    -1,    -1,   664,   619,    -1,
      -1,    -1,    -1,    -1,    -1,   838,    -1,    -1,   841,  1026,
      -1,    -1,    -1,    -1,  1809,   636,    -1,     7,     8,     9,
     105,   106,   107,    -1,   857,    15,     3,     4,     5,     6,
       7,     8,     9,    -1,  1256,    50,  1053,    -1,    15,    -1,
      -1,    -1,    32,    -1,    59,    60,    -1,    -1,    38,   716,
     717,   718,    29,    -1,    31,    32,    33,    47,   725,    -1,
     727,    38,   729,    -1,   811,    -1,    43,    -1,    83,   690,
      47,    -1,   819,    50,    -1,   696,    -1,   698,   699,    56,
      -1,  1098,    59,    60,    -1,    -1,   833,  1104,  1297,    -1,
      -1,    -1,     3,     4,     5,     6,    73,  1306,   845,   846,
      -1,    -1,   769,   770,    -1,    -1,    83,    -1,   729,     1,
      -1,    -1,     4,     5,    91,   736,     8,     9,    95,    -1,
      -1,    32,    99,    15,    -1,     3,     4,     5,    -1,  1338,
      -1,    -1,    43,     3,     4,     5,     6,    29,    49,    31,
      32,    -1,   809,   810,   811,    56,    38,    -1,    59,    60,
      -1,    -1,   819,    -1,    -1,    47,    -1,    -1,    -1,  1368,
      -1,    -1,    73,    -1,    56,    -1,   833,    59,    60,    -1,
      -1,    -1,    83,    43,    -1,   842,   843,   844,   845,   846,
      91,    59,    60,   850,    95,    96,    56,     8,     9,    59,
      60,  1413,    -1,    63,    15,    65,    -1,   864,   819,    -1,
      -1,    -1,    -1,    73,     3,     4,     5,     6,    -1,    -1,
       9,    32,   833,    83,    -1,    -1,    -1,    38,  1440,    -1,
      -1,    91,    -1,    -1,  1446,    95,    47,    -1,    -1,  1246,
      -1,    -1,    -1,    32,     3,     4,     5,  1254,     7,    -1,
    1462,  1463,    -1,  1452,    43,    -1,    -1,    -1,  1457,    -1,
      49,  1460,    -1,    -1,    -1,    -1,    82,    56,     4,     5,
      59,    60,    -1,     9,  1486,  1098,     3,     4,     5,     6,
      39,  1104,     9,    -1,    73,    -1,    -1,    -1,   112,    -1,
    1297,   948,    -1,   109,    83,    -1,    32,    56,    -1,  1306,
      59,    60,    91,    -1,    -1,    32,    95,    96,    -1,    -1,
      -1,  1048,  1049,    49,    -1,    -1,    43,  1529,  1530,    -1,
      56,   137,    49,    59,    60,    -1,  1149,    -1,    -1,    56,
      -1,    -1,    59,    60,   150,    -1,    -1,    73,    -1,    -1,
      -1,  1553,  1554,    -1,  1556,    -1,    73,    83,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    83,    -1,    -1,    95,
      96,     3,     4,     5,    91,  1102,  1103,    -1,    95,    96,
    1027,    -1,   196,  1030,  1111,  1112,    -1,    -1,    -1,    -1,
       3,     4,     5,     6,  1041,    -1,     9,    -1,    -1,    -1,
    1047,  1048,  1049,    -1,   210,    -1,  1007,   221,    -1,    -1,
       3,     4,     5,     6,  1141,    -1,  1605,  1606,  1607,    32,
    1021,    -1,  1023,    -1,  1025,  1072,    -1,    59,    60,    -1,
      43,    -1,     3,     4,     5,    -1,    49,    -1,    -1,    -1,
     246,    -1,    -1,    56,    -1,    -1,    59,    60,    -1,    -1,
      43,    -1,  1099,  1100,  1101,  1102,  1103,    50,    -1,  1060,
      73,  1108,    -1,  1110,  1111,  1112,    59,    60,  1670,    -1,
      83,   277,    -1,    -1,    -1,  1122,  1123,  1124,    91,    50,
      -1,    -1,    95,    96,  1297,    -1,    -1,    -1,    59,    60,
      83,    -1,    -1,  1306,  1141,     3,     4,     5,    91,  1496,
    1497,    -1,    -1,  1150,   318,    -1,     3,     4,     5,     6,
      -1,  1324,  1325,     4,     5,    -1,    -1,     8,     9,    -1,
       3,     4,     5,     6,    15,  1338,     9,  1716,     3,     4,
       5,    -1,     7,     8,     9,    -1,    -1,    -1,  1265,  1266,
      -1,    32,    50,    -1,    -1,   359,    43,    38,    -1,    32,
      -1,    59,    60,    50,    -1,  1552,    47,    -1,    49,    -1,
      43,    -1,    59,    60,    -1,    56,    49,    -1,    59,    60,
       3,     4,     5,    56,  1301,  1302,    59,    60,   384,     4,
      -1,    -1,    73,    -1,    59,    60,    83,    -1,    -1,    14,
      73,    -1,    83,    -1,    91,    -1,    -1,    -1,    -1,    24,
      83,    -1,    -1,   417,    95,    96,    31,    32,    91,    34,
      -1,    36,    95,    96,     3,     4,     5,  1264,  1265,  1266,
      45,    -1,    -1,  1436,   430,    -1,    59,    60,    -1,    -1,
      -1,    -1,    -1,    58,    -1,    60,    -1,    -1,    -1,  1452,
      -1,    -1,    -1,    -1,    69,     4,     5,    -1,    -1,    74,
       9,  1298,  1299,  1300,  1301,  1302,    -1,    -1,  1305,    -1,
      85,    -1,    87,    -1,    -1,    -1,    -1,    56,    -1,    -1,
      59,    60,    -1,    32,  1321,  1322,  1323,     3,     4,     5,
       6,   495,    -1,  1496,  1497,    84,   111,    -1,    -1,    -1,
      49,    -1,    -1,  1340,  1341,    -1,    -1,    56,    -1,    -1,
      59,    60,    -1,     3,     4,     5,     6,    -1,    -1,    -1,
      -1,    -1,    -1,   138,    73,   140,    -1,    43,     4,     5,
    1367,   146,   147,     9,    83,  1372,   151,   152,    -1,    -1,
     155,   156,   157,    59,    60,    -1,    95,    96,   544,  1552,
      -1,    -1,    -1,    43,    -1,    -1,   171,    -1,    -1,    -1,
     175,    -1,    -1,    -1,    -1,    -1,   181,   182,    -1,    59,
      60,     3,     4,     5,     6,    91,   165,     9,    -1,    -1,
      56,    -1,    -1,    59,    60,   200,   201,   202,    -1,    -1,
      -1,    -1,  1429,  1430,    -1,  1432,    -1,    73,    -1,    -1,
      32,    91,  1605,    -1,  1607,    -1,   602,    83,    -1,   198,
      66,    43,    -1,  1450,  1451,   230,  1453,  1454,    -1,  1456,
      -1,   617,   211,    -1,    56,    -1,  1417,    59,    60,     3,
       4,     5,   247,    -1,    -1,  1426,   640,   641,   642,   643,
     644,    73,   257,    -1,     3,     4,     5,     6,    -1,   653,
      -1,    83,    -1,    -1,    -1,   270,  1493,  1494,  1495,    91,
     664,    -1,    -1,    95,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1508,  1509,    -1,    -1,    -1,    -1,     3,     4,     5,
      -1,    -1,    56,    -1,    43,    59,    60,     3,     4,     5,
     146,    50,     3,     4,     5,   699,    -1,   312,   313,    -1,
      59,    60,    -1,   318,    -1,    -1,   162,    -1,   323,    -1,
      -1,   200,   201,  1716,    -1,    -1,   712,    -1,    -1,    -1,
      -1,   725,    -1,   727,    83,  1562,  1563,  1564,     3,     4,
       5,     6,    91,    59,    60,    -1,   732,    -1,    -1,    -1,
      -1,    -1,    -1,    59,    60,  1662,    -1,    -1,    59,    60,
      -1,    -1,    -1,    -1,    -1,    -1,   371,    -1,    -1,   374,
      -1,   376,    -1,   219,   379,   380,    -1,    -1,    43,  1606,
    1607,    -1,   228,    -1,    -1,    50,    -1,     3,     4,     5,
       6,    -1,  1619,  1620,    59,    60,    -1,  1704,  1705,   245,
      -1,    -1,   407,    -1,     1,    -1,     3,     4,     5,     6,
     256,     8,   417,    -1,   419,   420,    32,    -1,    83,    -1,
      -1,    -1,    -1,   428,    -1,    -1,    91,    43,    -1,    -1,
     816,    -1,    -1,   438,    -1,  1662,   441,    -1,    -1,    -1,
      56,    -1,    -1,    59,    60,    -1,    43,    -1,   453,    -1,
      -1,    -1,    -1,    50,    -1,    -1,   461,    73,    -1,    56,
      -1,   847,    59,    60,    -1,    -1,    -1,    83,    -1,    -1,
     716,   717,    -1,    -1,    -1,    91,  1703,  1704,  1705,    95,
      -1,  1662,   487,   488,   489,   490,   491,    -1,  1715,   883,
     495,    -1,    -1,    -1,    91,   889,     4,     5,   503,   504,
      -1,     9,   507,    -1,   509,   510,   511,   512,    -1,    -1,
      -1,   516,    -1,    -1,    -1,   901,   521,    -1,    -1,   524,
       3,     4,     5,     6,    32,    -1,     9,    -1,    -1,  1756,
      -1,    -1,    -1,    -1,    -1,   514,    -1,    -1,    -1,    -1,
     519,    49,    -1,    -1,    -1,    -1,    -1,    -1,    56,    32,
      -1,    59,    60,    -1,    -1,   560,    -1,    -1,    -1,    -1,
      43,    -1,    -1,   809,   810,    73,    49,    -1,    -1,    -1,
      -1,   550,    -1,    56,    -1,    83,    59,    60,   557,    -1,
      -1,    -1,   428,    -1,    -1,    -1,    -1,    95,    96,    -1,
      73,   437,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   604,
      83,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    91,  1003,
      -1,    -1,    95,    96,    -1,    -1,     3,     4,     5,     6,
     625,   600,   601,   469,   603,   504,    -1,    -1,   507,    -1,
     509,   510,    -1,    -1,    -1,    -1,   641,   642,   643,   644,
      -1,    -1,   521,  1037,    -1,     3,     4,     5,     6,  1043,
      -1,     9,    -1,     7,     8,     9,    43,   503,    -1,    -1,
      -1,    15,    -1,    50,    -1,    -1,   512,    -1,  1054,    -1,
    1056,    -1,    59,    60,    32,    -1,    -1,    -1,    32,  1073,
      -1,  1075,    -1,    -1,    38,    43,    -1,   533,    -1,    -1,
      -1,    49,    -1,    47,    -1,   700,    83,   702,    56,    -1,
      -1,    59,    60,    -1,    91,    -1,    -1,    -1,   713,    63,
      -1,   716,   717,   718,   560,    73,    -1,    -1,    -1,    -1,
    1106,    -1,    -1,  1109,   729,    83,    -1,  1121,   733,     3,
       4,     5,     6,    91,    -1,    -1,    -1,    95,    96,    -1,
      -1,    -1,   721,   722,    -1,    -1,    -1,    -1,   753,    -1,
      -1,   730,    -1,    -1,    -1,   760,    -1,    -1,    32,    -1,
      -1,    -1,    -1,    -1,   769,   770,    -1,    -1,    -1,    43,
      -1,  1165,    -1,  1167,    -1,    49,    -1,    -1,    -1,    -1,
      -1,  1027,    56,    -1,  1030,    59,    60,    -1,    -1,    -1,
      -1,     3,     4,     5,     6,  1041,    -1,     9,    -1,    73,
      -1,  1047,  1048,    -1,   809,   810,   811,    -1,    -1,    83,
      -1,  1205,   817,    -1,   819,    -1,    -1,    91,     4,     5,
      32,    95,    96,     9,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    43,    -1,   838,   813,    -1,   841,   842,   843,   844,
     845,   846,    -1,    -1,    56,   850,    32,    59,    60,    -1,
      -1,    -1,   857,  1099,  1100,  1101,  1102,    -1,    -1,    -1,
      -1,    73,  1108,    -1,  1110,  1111,    -1,    -1,    -1,   848,
      56,    83,    -1,    59,    60,   880,    -1,    -1,   883,    91,
      -1,    -1,    -1,    95,   889,    -1,    -1,    73,    -1,    -1,
      -1,    -1,    -1,   898,   899,   900,     1,    83,     3,     4,
       5,     6,     7,     8,     9,    -1,     4,     5,    -1,    95,
      15,   890,    -1,   892,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    -1,    29,    -1,    31,    32,    33,    -1,
     809,   810,    -1,    38,    32,    -1,    -1,    -1,    43,    -1,
    1334,    -1,    47,   948,    -1,    50,    -1,     3,     4,     5,
       6,    56,    -1,  1347,    59,    60,    -1,    -1,    56,    -1,
      -1,    59,    60,   842,   843,   844,   845,    -1,    73,    -1,
      -1,   850,    -1,  1359,    -1,    73,    32,    -1,    83,    -1,
      -1,    -1,    -1,    -1,    -1,    83,    91,    43,    -1,    -1,
      95,    -1,   838,    -1,    99,   841,    -1,    95,  1003,    -1,
      56,    -1,    -1,    59,    60,    -1,    -1,    -1,    -1,    -1,
      -1,   857,    -1,    -1,    -1,    -1,    -1,    73,  1264,  1265,
      -1,  1026,  1027,    -1,    -1,  1030,    -1,    83,    -1,    -1,
    1009,  1010,  1037,  1012,    -1,    91,  1041,    -1,  1043,    95,
    1434,    -1,  1047,  1048,  1049,  1431,    -1,    -1,  1053,    -1,
    1055,    -1,  1298,  1299,  1300,  1301,     3,     4,     5,  1305,
       7,     8,     9,  1042,    -1,    -1,    -1,    -1,  1073,  1455,
    1075,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    32,    -1,    -1,    -1,    -1,
      -1,    38,    -1,  1098,  1099,  1100,  1101,  1102,  1103,  1104,
      -1,    -1,    -1,  1108,    -1,  1110,  1111,  1112,    -1,  1503,
      -1,  1116,    59,    60,    -1,    -1,  1121,  1122,  1123,  1124,
       4,     5,     4,     5,     8,     9,    -1,     9,  1107,    -1,
      -1,    15,    -1,  1138,    -1,    -1,  1141,  1142,    -1,  1144,
      -1,    -1,    -1,    -1,  1149,  1150,    -1,  1152,    32,    -1,
      32,    -1,    -1,    -1,    38,  1549,    -1,    -1,    -1,    -1,
    1165,    -1,  1167,    47,    -1,    49,    -1,    49,     3,     4,
       5,     6,    56,    -1,    56,    59,    60,    59,    60,    -1,
    1026,    -1,    -1,  1429,  1430,    -1,  1432,    -1,    -1,    73,
      -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    83,
    1205,    83,    -1,    -1,    -1,    -1,    -1,  1053,    43,    -1,
      -1,    95,    96,    95,    96,    50,    -1,    -1,    -1,    -1,
    1099,  1100,  1101,  1102,    59,    60,    -1,  1232,  1233,  1108,
    1235,  1110,  1111,     3,     4,     5,     6,    -1,    -1,    -1,
      -1,  1246,    -1,  1122,  1123,  1124,    -1,  1493,  1494,  1254,
      -1,    -1,  1098,    -1,    -1,    -1,    91,    -1,  1104,  1264,
    1265,  1266,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   612,
      -1,  1250,    -1,    43,    -1,    -1,    -1,    -1,    -1,    -1,
      50,    -1,    -1,     3,     4,     5,     6,    -1,    -1,    59,
      60,    -1,  1297,  1298,  1299,  1300,  1301,  1302,    -1,    -1,
    1305,  1306,    -1,     3,     4,     5,     6,     7,     8,     9,
      -1,  1705,    -1,    83,    -1,    15,  1321,  1322,  1323,  1324,
    1325,    91,    -1,    43,  1303,     3,     4,     5,     6,  1334,
      50,    -1,    32,  1338,    -1,    -1,    -1,    -1,    38,    59,
      60,    -1,  1347,    43,    -1,    -1,    -1,    47,    -1,    49,
      -1,    -1,    -1,    -1,  1333,  1360,    56,    -1,     4,    59,
      60,    -1,    -1,  1368,    -1,    43,    -1,  1346,    14,    -1,
      -1,    91,    50,    73,    -1,    -1,    -1,    23,    24,    -1,
      -1,    59,    60,    83,    -1,    31,    32,    -1,    34,    -1,
      36,    91,    -1,    -1,  1373,    95,    96,    -1,  1377,    45,
    1246,    -1,    -1,    -1,    -1,    83,    -1,    -1,  1254,    -1,
      -1,    -1,    58,    91,    60,    -1,    -1,    -1,    -1,  1298,
    1299,  1300,  1301,    69,  1429,  1430,  1305,  1432,    74,  1434,
     318,  1436,    -1,    -1,    -1,    -1,  1415,     7,     8,     9,
      -1,    87,  1321,  1322,    -1,    15,    -1,  1452,    -1,   337,
     338,  1297,  1457,    -1,    -1,  1460,    -1,    -1,    -1,    -1,
    1306,    -1,    32,    -1,   352,    -1,    -1,   355,    38,    -1,
     358,    -1,    -1,   361,   362,    -1,    -1,    47,  1324,  1325,
      -1,    -1,    -1,    -1,   372,  1464,  1465,    -1,  1493,  1494,
    1495,  1496,  1497,    63,   140,    -1,    -1,    -1,  1503,    -1,
     146,   147,    -1,    -1,    -1,    -1,   152,    -1,    -1,   155,
     156,   157,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    -1,  1501,  1502,    -1,   171,    -1,    -1,  1507,   175,
      -1,    -1,    -1,    -1,    -1,   181,   182,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1549,    -1,    -1,  1552,    -1,    -1,
    1429,  1430,    -1,  1432,   200,   201,   202,    -1,    -1,    -1,
      -1,    -1,    -1,   906,   907,    -1,   909,   910,   911,   912,
     913,   914,   915,   916,   917,   918,   919,   920,   921,   922,
     923,   924,   925,   926,   230,     3,     4,     5,     6,    -1,
    1436,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1605,  1606,  1607,     1,    -1,     3,     4,     5,     6,     7,
       8,     9,    -1,    -1,  1493,  1494,    -1,    15,    -1,    -1,
      -1,    -1,    -1,    -1,   270,    43,    -1,    -1,    -1,    -1,
      -1,    29,    50,    31,    32,    -1,    -1,    -1,  1617,    -1,
      38,    59,    60,    -1,    -1,    43,    -1,    -1,    -1,    47,
    1496,  1497,    50,    -1,    -1,    -1,    -1,  1662,    56,    -1,
      -1,    59,    60,    -1,    -1,    63,   312,   313,    -1,    -1,
      -1,    -1,   318,    91,    -1,    73,    -1,   323,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    83,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    91,    -1,    -1,    -1,    95,  1703,  1704,
    1705,    -1,    -1,    -1,    -1,    -1,  1552,    74,    75,    76,
      77,  1716,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    -1,    -1,    -1,   612,   371,    -1,    -1,   374,    -1,
     376,    -1,    -1,   379,   380,    -1,  1079,    -1,     1,   385,
       3,     4,     5,     6,     7,     8,     9,    -1,    -1,    -1,
      -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   407,    -1,   651,   652,    -1,    29,    -1,    31,    32,
      33,   417,    -1,   419,   420,    38,    39,    -1,    -1,    -1,
      43,    -1,   428,    -1,    47,    48,    -1,    50,    -1,    -1,
      -1,    -1,   438,    56,    -1,   441,    59,    60,    -1,    -1,
      63,    -1,    65,    -1,    -1,    -1,    -1,   453,    -1,    -1,
      73,    -1,    -1,    -1,    -1,   461,     3,     4,     5,     6,
      83,    -1,    -1,    -1,  1703,  1704,    -1,    -1,    91,    -1,
      -1,    -1,    95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   487,   488,   489,   490,   491,    -1,    -1,   111,   495,
       3,     4,     5,     6,    -1,    -1,    43,   503,   504,    -1,
      -1,   507,    -1,   509,   510,   511,   512,    -1,    -1,    56,
     516,    -1,    59,    60,    -1,   521,    63,    -1,   524,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    73,    40,    41,    -1,
      43,    -1,    -1,    -1,    -1,    -1,    83,    -1,    -1,   545,
      -1,    -1,    -1,    56,    91,    -1,    59,    60,    95,    -1,
      -1,    -1,    -1,    -1,   560,   279,   280,   281,   282,   283,
     284,    -1,   286,   287,   288,   289,   290,   291,   292,   293,
     294,   295,   296,   297,   298,   299,   300,   301,   302,   303,
     304,   305,   306,   307,    -1,   309,    -1,    -1,    -1,     4,
       5,    -1,    -1,     8,     9,    -1,    -1,     4,     5,    -1,
      15,     8,     9,    -1,    -1,    -1,    -1,    -1,    15,    -1,
      -1,    -1,   618,    -1,    29,    -1,    31,    32,    -1,   625,
      -1,    -1,    29,    38,    31,    32,    -1,    -1,    -1,    -1,
      -1,    38,    47,    -1,    -1,   641,   642,   643,   644,    -1,
      47,    56,    -1,    -1,    59,    60,    61,    -1,    -1,    56,
      -1,    -1,    59,    60,    -1,    -1,    -1,    -1,   906,   907,
      -1,   909,   910,   911,   912,   913,   914,   915,   916,   917,
     918,   919,   920,   921,   922,   923,   924,   925,   926,  1382,
      95,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,     6,
       7,     8,     9,    -1,   700,    -1,   702,    -1,    15,    -1,
      -1,     3,     4,     5,     6,    -1,    -1,    -1,    -1,    -1,
     716,   717,   718,    -1,    -1,    32,    -1,    -1,    -1,    -1,
      -1,    38,    -1,   729,    -1,    -1,    43,    -1,    -1,    -1,
      47,    -1,    49,    -1,    -1,    -1,   742,    -1,    -1,    56,
      -1,    43,    59,    60,    -1,    -1,    -1,   753,    50,    -1,
      -1,    -1,    -1,    -1,    -1,   761,    73,    59,    60,    -1,
      -1,    -1,    -1,   769,   770,    -1,    83,  1015,    -1,    -1,
       3,     4,     5,     6,    91,    -1,    -1,    -1,    95,    96,
      -1,    83,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    91,
       3,     4,     5,     6,     7,     8,     9,    -1,    -1,  1047,
    1048,    -1,    15,   809,   810,   811,    -1,    40,    41,    -1,
      43,    -1,    -1,   819,    -1,    -1,    29,    -1,    31,    32,
      -1,    -1,    -1,    56,    -1,    38,    59,    60,    -1,    -1,
      43,  1079,   838,    -1,    47,   841,   842,   843,   844,   845,
     846,    -1,    -1,    56,   850,    -1,    59,    60,    -1,    -1,
     574,   857,    65,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      73,    -1,  1110,  1111,    -1,    -1,    -1,    -1,    -1,    -1,
      83,    -1,    -1,   597,   880,    -1,    -1,   883,    91,    -1,
      -1,   605,    95,   889,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   898,   899,   900,    -1,    -1,    -1,    -1,     1,
      -1,     3,     4,     5,     6,     7,     8,     9,    -1,    -1,
      -1,    13,    -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    26,    -1,    28,    -1,    -1,    -1,
      32,    -1,    -1,    -1,    -1,    -1,    38,    -1,    -1,    -1,
      -1,    43,   948,    -1,    -1,    47,    -1,    49,    -1,    -1,
      -1,    -1,    -1,    -1,    56,    -1,    -1,    59,    60,    61,
      62,    63,    -1,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    -1,    89,    90,    91,
    1238,    93,    94,    95,    96,    97,    -1,  1003,    -1,    -1,
      -1,   103,   104,    -1,    -1,    -1,    -1,   109,   110,   111,
      -1,   113,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1026,  1027,    -1,    -1,  1030,    -1,    -1,    -1,     4,     5,
      -1,  1037,     8,     9,    -1,  1041,    -1,  1043,    -1,    15,
      -1,  1047,  1048,  1049,    -1,    -1,    -1,  1053,    -1,  1055,
      -1,    -1,    -1,    29,    -1,    31,    32,    -1,    -1,    -1,
      -1,    -1,    38,    -1,    -1,    -1,    -1,  1073,    -1,  1075,
      -1,    47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      56,    -1,    -1,    59,    60,    -1,    -1,  1093,    -1,    -1,
      -1,    -1,  1098,  1099,  1100,  1101,  1102,  1103,  1104,    -1,
      -1,    -1,  1108,    -1,  1110,  1111,  1112,    -1,    -1,    -1,
    1116,    -1,    -1,    -1,    -1,  1121,  1122,  1123,  1124,    95,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1135,
      -1,    -1,  1138,    -1,  1382,  1141,  1142,    -1,  1144,    -1,
      -1,    -1,    -1,  1149,  1150,    -1,  1152,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     3,     4,     5,     6,  1165,
       1,  1167,     3,     4,     5,     6,     7,     8,     9,    -1,
     894,   895,    -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,
      -1,     3,     4,     5,     6,    -1,    -1,     9,    29,    -1,
      31,    32,    33,    -1,    -1,    43,    -1,    38,    39,  1205,
      -1,    -1,    43,    -1,    -1,    -1,    47,    48,    56,    50,
      32,    59,    60,    -1,    -1,    56,    -1,    65,    59,    60,
      -1,    43,    63,    -1,    65,    73,  1232,  1233,    -1,  1235,
      -1,    -1,    73,    -1,    56,    83,    -1,    59,    60,    -1,
    1246,    -1,    83,    91,    -1,    -1,    -1,    95,  1254,    -1,
      91,    73,    -1,    -1,    95,    -1,    -1,    -1,  1264,  1265,
    1266,    83,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    91,
     111,    -1,    -1,    95,    -1,    -1,    -1,    -1,    -1,     4,
       5,    -1,     7,     8,     9,    -1,    -1,    -1,    13,    -1,
      15,  1297,  1298,  1299,  1300,  1301,  1302,    -1,    -1,  1305,
    1306,    -1,    -1,    -1,    29,    -1,    31,    32,    -1,    -1,
      -1,    -1,    -1,    38,    -1,  1321,  1322,  1323,  1324,  1325,
      -1,    -1,    47,  1329,    49,     4,    -1,    -1,  1334,    -1,
      -1,    56,  1338,    -1,    59,    60,    -1,     3,     4,     5,
       6,  1347,    -1,    -1,    -1,    -1,    -1,    -1,    73,    -1,
    1598,    -1,    31,    32,    -1,    34,    -1,    36,    83,    -1,
      -1,    -1,  1368,    -1,    -1,    -1,    45,    -1,    -1,    -1,
      95,    96,    -1,    -1,    -1,    -1,    -1,    43,    -1,    58,
      -1,    60,     3,     4,     5,     6,    -1,    -1,     9,    -1,
      56,    -1,    -1,    59,    60,    -1,    -1,    -1,    -1,    65,
      -1,    -1,    -1,    82,    -1,    -1,    85,    73,    -1,    -1,
      -1,    32,    -1,    -1,    -1,    -1,    -1,    83,  1424,    -1,
      -1,    -1,    43,  1429,  1430,    91,  1432,    -1,  1434,    95,
    1436,    -1,    -1,    -1,    -1,    56,    -1,    -1,    59,    60,
      -1,    -1,    -1,    -1,    -1,    -1,  1452,    -1,    -1,    -1,
      -1,  1457,    73,    -1,  1460,  1703,  1704,    -1,   137,   138,
      -1,   140,    83,    -1,    -1,    -1,    -1,   146,   147,    -1,
      91,   150,   151,   152,    95,    -1,   155,   156,   157,    -1,
      -1,    -1,    -1,   162,    -1,    -1,    -1,  1493,  1494,  1495,
    1496,  1497,   171,    -1,    -1,    -1,   175,  1503,    -1,    -1,
      -1,    -1,   181,   182,    -1,    -1,    -1,     4,     5,    -1,
       7,     8,     9,    -1,    -1,    -1,    -1,    -1,    15,    -1,
      -1,   200,   201,   202,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   210,    29,    -1,    31,    32,    -1,    -1,    -1,    -1,
      -1,    38,    -1,  1549,    -1,    -1,  1552,    -1,    -1,    -1,
      47,   230,    -1,    50,    -1,    -1,    -1,    -1,    -1,    56,
      -1,    -1,    59,    60,    -1,     1,    -1,     3,     4,     5,
       6,     7,     8,     9,    -1,    -1,    -1,    -1,    -1,    15,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1594,    -1,
      -1,   270,    -1,    29,    -1,    31,    32,    33,   277,  1605,
    1606,  1607,    38,    39,    -1,     4,     5,    43,     7,     8,
       9,    47,    48,    -1,    50,    -1,    15,    -1,    -1,    -1,
      56,    -1,    -1,    59,    60,    -1,    -1,    63,    -1,    65,
      29,    -1,    31,    32,    -1,    -1,    -1,    73,    -1,    38,
      -1,    -1,    -1,    -1,   323,    -1,    -1,    83,    47,    -1,
      -1,    -1,    -1,    -1,    -1,    91,  1662,    56,    -1,    95,
      59,    60,    -1,    -1,     3,     4,     5,     6,    -1,    -1,
       9,    -1,    -1,    -1,    -1,   111,    -1,    -1,    -1,    -1,
     359,     4,     5,    -1,     7,     8,     9,    -1,    -1,    -1,
      13,    -1,    15,    32,    -1,   374,    -1,  1703,  1704,  1705,
      -1,    -1,    -1,    -1,    43,   384,    29,    -1,    31,    32,
    1716,    -1,    -1,    -1,    -1,    38,    -1,    56,    -1,    -1,
      59,    60,    -1,    -1,    47,    -1,    49,    -1,   407,    -1,
      -1,    -1,    -1,    56,    73,    -1,    59,    60,    -1,    -1,
     419,    -1,    -1,    -1,    83,    -1,    -1,    -1,    -1,   428,
      73,   430,    91,    -1,    -1,    -1,    95,    -1,   437,   438,
      83,    -1,   441,    -1,    -1,    -1,    -1,    -1,   447,   448,
     449,    -1,    95,    96,   453,    71,    72,    73,    74,    75,
      76,    77,   461,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    -1,    -1,    -1,     3,     4,     5,     6,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   487,   488,
     489,   490,    -1,    -1,    -1,    -1,   495,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   503,   504,    -1,    -1,   507,    -1,
     509,   510,   511,   512,    -1,    43,    -1,   516,    -1,     4,
      -1,    -1,   521,    -1,    -1,   524,    -1,    -1,    56,    -1,
      -1,    59,    60,    -1,     1,    -1,     3,     4,     5,     6,
       7,     8,     9,    -1,    -1,    73,    31,    32,    15,    34,
      -1,    36,    -1,    -1,    -1,    83,    -1,    -1,    -1,    -1,
      45,   560,    29,    91,    31,    32,    33,    95,    -1,    -1,
      -1,    38,    39,    58,    -1,    60,    43,    -1,    -1,    -1,
      47,    48,    -1,    50,    -1,    -1,    -1,    -1,    -1,    56,
      -1,    -1,    59,    60,    -1,    -1,    63,    -1,    65,    -1,
      85,    -1,    -1,    -1,    -1,    -1,    73,     3,     4,     5,
       6,     7,     8,     9,    -1,    -1,    83,    13,   617,    15,
      -1,    -1,    -1,    -1,    91,    -1,   625,    -1,    95,    -1,
      -1,    -1,    -1,    29,    -1,    31,    32,    -1,    -1,    -1,
      -1,    -1,    38,    -1,   111,    -1,    -1,    43,    -1,    -1,
      -1,    47,   651,   652,   653,   140,    -1,    -1,    -1,    -1,
      56,   146,   147,    59,    60,   664,   151,   152,    -1,    -1,
     155,   156,   157,    -1,    -1,    -1,    -1,    73,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   171,    83,    -1,    -1,
     175,    -1,    -1,    -1,    -1,    91,   181,   182,    -1,    95,
      -1,    -1,    -1,    -1,     1,    -1,     3,     4,     5,     6,
       7,     8,     9,   712,    -1,   200,   201,   202,    15,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   725,    -1,   727,    -1,
     729,    28,    29,    -1,    31,    32,    33,    -1,    -1,    -1,
      -1,    38,    -1,    -1,    -1,   230,    43,    -1,    -1,    46,
      47,    48,    -1,    50,   753,    -1,    -1,    -1,    -1,    56,
      57,   760,    59,    60,    -1,    -1,    63,    -1,    -1,    -1,
     769,    -1,    -1,    -1,    -1,    -1,    73,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   270,    83,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    91,    -1,    -1,    -1,    95,    -1,
      -1,    -1,    99,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     809,   810,   811,    -1,    -1,    -1,    -1,   816,   817,    -1,
     819,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,   833,    -1,    -1,    -1,   323,   838,
      -1,    -1,   841,   842,   843,   844,   845,   846,    -1,    -1,
      -1,   850,    72,    73,    74,    75,    76,    77,   857,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    -1,     1,
      -1,     3,     4,     5,     6,    -1,     8,     9,    10,    11,
      12,   880,    14,    15,    75,    76,    77,    -1,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    29,    30,    31,
     385,    33,    -1,    35,    36,    37,    38,    -1,    40,    41,
      42,    43,    44,    45,    -1,    47,    -1,    -1,    -1,    51,
      52,    53,    54,    55,    56,    -1,    -1,    59,    60,    61,
      -1,    -1,    64,    -1,   419,    -1,    -1,    -1,    70,    -1,
      -1,    73,    -1,   428,    -1,    -1,    -1,    -1,    -1,    81,
      82,    83,    -1,   438,    -1,    -1,   441,    89,    90,    91,
      -1,    -1,    -1,    95,    96,    -1,    -1,    -1,   453,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   461,    -1,    -1,   111,
     112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,
       5,     6,     7,     8,     9,    -1,    -1,    -1,    13,    -1,
      15,    -1,   487,   488,   489,   490,    -1,    -1,    -1,    -1,
     495,    -1,    -1,    -1,    29,    -1,    31,    32,   503,   504,
      -1,    -1,   507,    38,   509,   510,   511,    -1,    43,    -1,
      -1,   516,    47,    -1,    49,    -1,   521,    -1,    -1,   524,
      -1,    56,    -1,    -1,    59,    60,    -1,    -1,  1047,  1048,
    1049,    -1,    -1,    -1,  1053,  1054,    76,    77,    73,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    83,    -1,
      -1,    -1,    -1,    -1,    -1,   560,    91,    -1,    -1,    -1,
      95,    96,    -1,    -1,    34,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    -1,    -1,  1098,
    1099,  1100,  1101,  1102,  1103,  1104,    -1,  1106,    58,  1108,
    1109,  1110,  1111,  1112,    -1,    -1,    -1,  1116,    -1,    -1,
      -1,     4,     5,    -1,     7,     8,     9,    -1,    -1,    -1,
      13,    -1,    15,   618,    -1,    -1,    -1,    -1,    -1,    -1,
     625,    -1,  1141,  1142,    -1,  1144,    29,    -1,    31,    32,
    1149,  1150,    -1,  1152,     1,    38,     3,     4,     5,     6,
       7,     8,     9,    -1,    47,    -1,    -1,    -1,    15,    -1,
      -1,    -1,    -1,    56,    -1,    -1,    59,    60,    -1,    -1,
      -1,    -1,    29,    -1,    31,    32,    33,    -1,    -1,    -1,
      -1,    38,    -1,    -1,    -1,    -1,    43,    -1,    -1,    -1,
      47,    48,    -1,    50,    -1,   155,   156,   157,    -1,    56,
      -1,    -1,    59,    60,    -1,    -1,    63,    -1,    65,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    73,    -1,   713,    -1,
      -1,   181,    -1,    -1,    -1,    -1,    83,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    91,    -1,    -1,    -1,    95,    -1,
     200,   201,   202,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1264,  1265,  1266,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     230,    -1,    -1,    -1,   769,     4,     5,    -1,    -1,     8,
       9,    -1,    -1,    -1,    -1,    -1,    15,    -1,  1297,  1298,
    1299,  1300,  1301,  1302,    -1,    -1,  1305,  1306,    -1,    -1,
      29,    -1,    31,    32,    -1,    -1,    -1,    -1,    -1,    38,
      -1,    -1,    -1,    -1,   809,   810,   811,    -1,    47,    -1,
      49,    -1,   817,    -1,   819,    -1,    -1,    56,    -1,  1338,
      59,    60,    -1,    -1,    -1,     3,     4,     5,     6,    -1,
      -1,     9,    -1,   838,    73,    -1,    -1,   842,   843,    -1,
    1359,  1360,    -1,   313,    83,   850,    -1,    -1,    -1,  1368,
      -1,    -1,   857,    -1,    32,    -1,    95,    96,    -1,     4,
       5,    -1,     7,     8,     9,    43,    -1,    -1,    13,    -1,
      15,    -1,    -1,    -1,    -1,   880,    -1,    -1,    56,    -1,
      -1,    59,    60,    -1,    29,    -1,    31,    32,    -1,    -1,
      -1,    -1,    -1,    38,    -1,    73,    -1,    -1,    -1,    -1,
      -1,    -1,    47,    -1,    -1,    83,   376,    -1,    -1,   379,
     380,    56,    -1,    91,    59,    60,     1,    95,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    -1,    -1,  1452,    -1,    -1,  1455,    -1,  1457,    -1,
      -1,  1460,    -1,    -1,    29,    30,    31,    32,    33,   419,
      35,    36,    37,    38,    -1,    40,    41,    42,    43,    44,
      45,    -1,    47,    -1,    49,    -1,    51,    52,    53,    54,
      55,    56,    -1,    -1,    59,    60,    61,    -1,    -1,    64,
      -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    73,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    81,    82,    83,    -1,
      -1,    -1,    -1,    -1,    89,    90,    91,    -1,    -1,    -1,
      95,    96,     3,     4,     5,     6,    -1,   487,    -1,   489,
     490,   491,    -1,    -1,    -1,    -1,    -1,   112,    -1,    -1,
      -1,    -1,    -1,   503,   504,    -1,    -1,   507,    -1,   509,
     510,   511,   512,    -1,    -1,    -1,   516,    -1,    -1,    -1,
    1055,   521,    43,    -1,   524,    -1,    -1,    58,    -1,    -1,
      -1,     3,     4,     5,     6,    56,    -1,     9,    59,    60,
      -1,    -1,    -1,    -1,    65,    -1,    -1,    -1,    -1,    -1,
      -1,    82,    73,    -1,    85,    -1,  1605,  1606,  1607,    -1,
      32,    -1,    83,  1098,  1099,  1100,  1101,  1102,  1103,    -1,
      91,    43,    -1,  1108,    95,  1110,  1111,  1112,   109,    -1,
     111,  1116,    -1,    -1,    56,    -1,    -1,    59,    60,    -1,
      -1,    -1,     3,     4,     5,     6,     7,     8,     9,    -1,
      -1,    73,    13,    -1,    15,    -1,  1141,  1142,    -1,  1144,
      -1,    83,    -1,  1662,  1149,  1150,    -1,  1152,    29,    91,
      31,    32,    -1,    95,    -1,   625,    -1,    38,    -1,    -1,
      -1,    -1,    43,    -1,    -1,    -1,    47,    -1,    49,    -1,
      -1,    -1,    -1,    -1,    -1,    56,    -1,    -1,    59,    60,
     181,    -1,    -1,    -1,  1703,  1704,  1705,    -1,    -1,    -1,
      -1,    -1,    73,    -1,    -1,    -1,    -1,  1716,    -1,   200,
     201,   202,    83,    -1,    -1,    -1,    -1,    -1,    -1,   210,
      91,    -1,    -1,    -1,    95,    96,    -1,     4,     5,    -1,
      -1,     8,     9,    -1,    -1,    -1,    -1,    -1,    15,   230,
      -1,    -1,   702,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    29,    -1,    31,    32,   716,   717,   718,    -1,
      -1,    38,    -1,    -1,    -1,    -1,   257,    -1,    -1,   729,
      47,    -1,    -1,    -1,    -1,    -1,    -1,   196,    -1,    56,
      -1,    -1,    59,    60,    -1,    -1,   277,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,  1297,  1298,  1299,    -1,    -1,    -1,    -1,   769,
    1305,  1306,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,
       5,     6,     7,     8,     9,    -1,    -1,    -1,    13,    -1,
      15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1338,    29,    -1,    31,    32,    -1,   809,
     810,   811,    -1,    38,    -1,    -1,    -1,    -1,    43,   819,
      -1,    -1,    47,    -1,    49,  1360,    -1,    -1,    -1,    -1,
      -1,    56,    -1,  1368,    59,    60,    -1,    -1,   838,    -1,
      -1,   841,   842,   843,   844,   845,   846,    -1,    73,    -1,
     850,    -1,    -1,   384,   385,    -1,    -1,   857,    83,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    91,    -1,    -1,    -1,
      95,    96,    -1,    -1,    -1,     3,     4,     5,     6,     7,
       8,     9,    -1,    -1,    -1,    -1,    -1,    15,   419,    -1,
      -1,   350,    -1,    -1,     3,     4,     5,     6,   898,   899,
     900,    29,    -1,    31,    32,    -1,    -1,    -1,    -1,    -1,
      38,    -1,    -1,    -1,    -1,    43,    -1,  1452,    -1,    47,
      -1,    49,  1457,    32,    -1,  1460,    -1,    -1,    56,    -1,
      -1,    59,    60,    -1,    43,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    73,    -1,    56,   948,    -1,
      59,    60,    -1,    -1,    -1,    83,   487,    -1,   489,   490,
      -1,    -1,    -1,    91,    73,    -1,    -1,    95,    96,    -1,
      -1,    -1,   503,   504,    83,    -1,   507,    -1,   509,   510,
     511,   512,    91,    -1,    -1,   516,    95,    -1,    -1,    -1,
     521,    -1,    -1,   524,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     3,     4,     5,     6,    -1,    -1,     9,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   477,   478,
      -1,    -1,    -1,    -1,    -1,    -1,  1026,  1027,    -1,    -1,
    1030,    -1,    32,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1041,    -1,    43,    -1,    -1,    -1,  1047,  1048,  1049,
      -1,    -1,    -1,  1053,    -1,    -1,    56,    -1,    -1,    59,
      60,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1605,  1606,  1607,    73,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    83,    -1,    -1,   617,   618,    -1,    -1,
      -1,    91,    -1,    -1,   625,    95,    -1,    -1,  1098,  1099,
    1100,  1101,  1102,  1103,  1104,    -1,    -1,    -1,  1108,    -1,
    1110,  1111,  1112,    -1,    -1,    -1,    -1,    -1,     3,     4,
       5,     6,  1122,  1123,  1124,    -1,    -1,  1662,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1141,    -1,    -1,    -1,    -1,    -1,    32,    -1,  1149,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    43,    -1,
     619,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1703,  1704,
    1705,    56,    -1,    -1,    59,    60,    -1,   636,    -1,    -1,
     639,  1716,    -1,    73,    74,    75,    76,    77,    73,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    83,    -1,
      -1,   732,   733,   662,   663,    -1,    91,   666,   667,   668,
      95,   670,   671,   672,   673,   674,   675,   676,   677,   678,
     679,   680,   681,   682,   683,   684,   685,   686,   687,   688,
      -1,   690,  1232,  1233,    -1,  1235,    -1,   696,   769,   698,
     699,    -1,    -1,    -1,    -1,    -1,  1246,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1254,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1264,  1265,  1266,    -1,    -1,    -1,
     729,    -1,    -1,    -1,    -1,    -1,    -1,   736,   809,   810,
     811,    -1,    -1,    -1,    -1,   816,   817,    -1,   819,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1297,  1298,  1299,
    1300,  1301,  1302,    -1,    -1,  1305,  1306,   838,    -1,    -1,
     841,   842,   843,   844,   845,   846,   847,    -1,    -1,   850,
      -1,  1321,  1322,  1323,  1324,  1325,   857,     3,     4,     5,
       6,     7,     8,     9,    -1,    -1,    -1,    -1,  1338,    15,
      -1,   800,    -1,    -1,    -1,    -1,    -1,    -1,   879,    -1,
      -1,    -1,    -1,    29,    -1,    31,    32,    -1,    -1,    -1,
     819,    -1,    38,    -1,    -1,    -1,    -1,    43,    -1,    -1,
      -1,    47,    -1,    -1,   833,    -1,    -1,    -1,    -1,    -1,
      56,    -1,    -1,    59,    60,    -1,    -1,    63,     3,     4,
       5,     6,    -1,    -1,    14,    -1,    -1,    73,    -1,    -1,
      -1,    -1,    -1,    23,    24,    -1,    -1,    83,    -1,    -1,
      -1,    31,    32,    -1,    34,    91,    -1,    32,    -1,    95,
      -1,     3,     4,     5,     6,     7,     8,     9,    43,  1429,
    1430,    -1,  1432,    15,    -1,    -1,  1436,    -1,    -1,    -1,
      -1,    56,    -1,    -1,    59,    60,    -1,    -1,    -1,    69,
      32,    -1,  1452,    -1,    -1,    -1,    38,    -1,    73,    -1,
      -1,    43,    -1,    -1,    -1,    47,    -1,    49,    83,    -1,
      -1,    -1,    -1,    -1,    56,    -1,    91,    59,    60,   938,
      95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    73,    -1,  1493,  1494,  1495,  1496,  1497,    -1,    -1,
      -1,    83,    -1,    -1,    -1,    -1,    -1,   966,   128,    91,
     130,    -1,    -1,    95,    -1,    -1,    -1,   137,   138,    -1,
      -1,    -1,    -1,  1054,  1055,  1056,   146,   147,    -1,    -1,
     150,   151,   152,    -1,   154,   155,   156,   157,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1006,  1007,    -1,
      -1,    -1,  1552,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1021,    -1,  1023,    -1,  1025,  1098,  1099,  1100,
    1101,  1102,  1103,  1104,    -1,  1106,    -1,  1108,  1109,  1110,
    1111,  1112,    -1,    -1,    -1,  1116,    -1,    -1,    -1,    -1,
      -1,  1122,  1123,  1124,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1060,    -1,    -1,    -1,  1605,    -1,  1607,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1149,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   246,   247,    -1,    -1,
      -1,    -1,    -1,    -1,   376,    -1,    -1,   379,   380,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,
     270,    15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1118,
    1119,    -1,  1662,    -1,    -1,    29,    -1,    31,    32,    -1,
      -1,    -1,    -1,    -1,    38,    -1,    -1,    -1,    -1,    43,
      -1,  1140,    -1,    47,    -1,     3,     4,     5,     6,     7,
       8,     9,    56,   313,    -1,    59,    60,    15,    -1,    63,
      -1,    -1,    -1,  1703,  1704,  1705,    -1,    -1,    -1,    73,
      -1,    29,    -1,    31,    32,    -1,  1716,    -1,    -1,    83,
      38,    -1,    -1,    -1,    -1,    43,    -1,    91,    -1,    47,
      -1,    95,    -1,    -1,    -1,    -1,    -1,    -1,    56,    -1,
      -1,    59,    60,    -1,    -1,   487,    -1,   489,   490,   491,
      -1,    -1,    -1,    -1,   374,    73,   376,  1216,  1217,   379,
      -1,  1220,    -1,    -1,    -1,    83,  1297,  1298,  1299,  1300,
    1301,  1302,    -1,    91,  1305,  1306,    -1,    95,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   407,    -1,    -1,
    1321,  1322,  1323,  1324,  1325,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1338,   428,    -1,
     430,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   438,    -1,
      -1,   441,    -1,    -1,    -1,    -1,    -1,    -1,  1359,    -1,
      -1,    -1,    -1,   453,    -1,    -1,  1295,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       3,     4,     5,     6,     7,     8,     9,     3,     4,     5,
       6,    -1,    15,     9,    -1,    -1,    -1,  1326,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    29,    -1,    31,    32,
      -1,    -1,    -1,    -1,    -1,    38,    32,    -1,    -1,    -1,
      43,  1350,    -1,    -1,    47,    -1,    -1,    43,  1429,  1430,
    1431,  1432,    -1,    56,    -1,  1436,    59,    60,    -1,    -1,
      56,    -1,    -1,    59,    60,     3,     4,     5,     6,    -1,
      73,  1452,    -1,    -1,   544,   545,    -1,    73,    -1,    -1,
      83,    -1,  1391,    -1,    -1,    -1,    -1,    83,    91,    -1,
     560,    -1,    95,    -1,    32,    91,    -1,    -1,    -1,    95,
      -1,    -1,    -1,    -1,    -1,    43,    -1,    -1,  1417,    -1,
      -1,    -1,  1493,  1494,  1495,  1496,  1497,  1426,    56,    -1,
      -1,    59,    60,    -1,   716,   717,   718,    13,    -1,  1438,
      -1,    -1,   602,    -1,   604,    73,    -1,   729,    -1,  1448,
    1449,    -1,    -1,    -1,    -1,    83,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    91,    -1,    -1,    -1,    95,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1479,  1552,    -1,    -1,    -1,    -1,    -1,   769,    -1,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    -1,    -1,    -1,  1514,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   809,   810,   811,
      32,    -1,    -1,    -1,  1605,    -1,  1607,   819,    -1,    -1,
      -1,    -1,   702,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   712,   713,    -1,    -1,   716,   717,    -1,    -1,
      -1,  1560,  1561,    -1,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    -1,    -1,  1587,    -1,
      -1,  1662,    -1,   753,    -1,    -1,    -1,    -1,    -1,    -1,
     760,   761,    70,    71,    72,    73,    74,    75,    76,    77,
     770,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      -1,    -1,    -1,    -1,    -1,    -1,  1625,    -1,    -1,    -1,
      -1,    -1,  1703,  1704,  1705,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1716,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,  1662,    -1,    -1,   948,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    -1,    -1,    -1,  1687,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   879,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,   113,    -1,   898,   899,
     900,   901,    -1,    -1,  1026,  1027,    -1,    -1,  1030,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1041,
      -1,    -1,    -1,    -1,    -1,  1047,  1048,  1049,    -1,    -1,
      -1,  1053,    -1,    -1,    -1,    -1,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,   948,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    -1,  1798,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1098,  1099,  1100,  1101,
    1102,  1103,  1104,    -1,    -1,    -1,  1108,    -1,  1110,  1111,
    1112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    -1,    -1,    -1,  1026,  1027,    -1,    -1,
    1030,    -1,    -1,    -1,    -1,    -1,    29,    30,    31,    32,
      33,  1041,    35,    36,    37,    38,    -1,    40,    41,    42,
      43,    44,    45,  1053,    47,    -1,    49,    -1,    51,    52,
      53,    54,    55,    56,    -1,    -1,    59,    60,    61,    -1,
      -1,    64,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,
      73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,    82,
      83,    -1,    -1,  1093,    -1,    -1,    89,    90,    91,    -1,
      -1,    -1,    95,    96,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   112,
      -1,    -1,    -1,    -1,  1246,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1254,    -1,    -1,  1135,    -1,    -1,  1138,    -1,
      -1,  1141,  1264,  1265,  1266,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     1,
      -1,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    -1,  1297,  1298,  1299,  1300,  1301,
    1302,    -1,    -1,  1305,  1306,    -1,    -1,    29,    30,    31,
      32,    33,    -1,    35,    36,    37,    38,    -1,    40,    41,
      42,    43,    44,    45,    -1,    47,    -1,    -1,    -1,    51,
      52,    53,    54,    55,    56,    -1,    -1,    59,    60,    -1,
      -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,    70,    -1,
      -1,    73,  1232,  1233,    -1,  1235,    -1,    -1,    -1,    81,
      82,    83,    -1,    -1,    -1,    -1,  1246,    89,    90,    91,
      -1,    -1,    -1,    95,  1254,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1264,  1265,    -1,    -1,   110,    -1,
     112,    -1,    -1,    -1,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1429,  1430,    -1,
    1432,    -1,    -1,     3,     4,     5,     6,   109,     8,     9,
      10,    11,    12,    -1,    14,    15,    -1,    -1,    -1,  1329,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    29,
      30,    31,    32,    33,    -1,    35,    36,    37,    38,    -1,
      40,    41,    42,    43,    44,    45,    -1,    47,    -1,    49,
    1360,    51,    52,    53,    54,    55,    56,    -1,    -1,    59,
      60,  1493,  1494,  1495,  1496,  1497,    -1,    -1,    -1,    -1,
      70,    -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    81,    82,    83,    -1,    -1,    -1,    -1,    -1,    89,
      90,    91,    -1,    -1,    -1,    95,    96,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   112,    -1,  1424,    -1,    -1,    62,    -1,    -1,
    1552,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    -1,    -1,  1455,     1,  1457,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    -1,    14,
      15,    16,    -1,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    -1,
      35,    36,    37,    38,    -1,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    -1,    -1,    51,    52,    53,    54,
      55,    56,    -1,    -1,    59,    60,    61,    -1,    63,    64,
      -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    73,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    81,    82,    83,    -1,
      -1,    -1,    -1,    -1,    89,    90,    91,    -1,    -1,    -1,
      95,    -1,    97,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   111,   112,     1,    -1,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      -1,    14,    15,    16,    -1,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    -1,    35,    36,    37,    38,    -1,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    -1,    -1,    51,    52,
      53,    54,    55,    56,    -1,    -1,    59,    60,    61,    -1,
      63,    64,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,
      73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,    82,
      83,    -1,    -1,    -1,    -1,    -1,    89,    90,    91,    -1,
      -1,    -1,    95,     1,    97,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    -1,    14,    15,    -1,   112,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    29,    30,    31,    32,    33,    -1,    35,    36,    37,
      38,    -1,    40,    41,    42,    43,    44,    45,    -1,    47,
      -1,    -1,    -1,    51,    52,    53,    54,    55,    56,    -1,
      -1,    59,    60,    61,    -1,    63,    64,    -1,    -1,    -1,
      -1,    -1,    70,    -1,    -1,    73,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    81,    82,    83,    -1,    -1,    -1,    -1,
      -1,    89,    90,    91,    -1,    -1,     1,    95,     3,     4,
       5,     6,    -1,     8,     9,    10,    11,    12,    -1,    14,
      15,    -1,    -1,    -1,   112,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    29,    30,    31,    32,    33,    -1,
      35,    36,    37,    38,    -1,    40,    41,    42,    43,    44,
      45,    -1,    47,    -1,    -1,    -1,    51,    52,    53,    54,
      55,    56,    -1,    -1,    59,    60,    61,    -1,    -1,    64,
      -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    73,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    81,    82,    83,    -1,
      -1,    -1,    -1,    -1,    89,    90,    91,    -1,    -1,     1,
      95,     3,     4,     5,     6,    -1,     8,     9,    10,    11,
      12,    -1,    14,    15,    -1,    -1,    -1,   112,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    29,    30,    31,
      -1,    33,    -1,    35,    36,    37,    38,    -1,    40,    41,
      42,    43,    44,    45,    -1,    47,    -1,    -1,    -1,    51,
      52,    53,    54,    55,    56,    -1,    -1,    59,    60,    61,
      -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,    70,    -1,
      -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,
      82,    83,    -1,    -1,    -1,    -1,    -1,    89,    90,    91,
      -1,    -1,    -1,    95,    96,     1,    -1,     3,     4,     5,
       6,    -1,     8,     9,    10,    11,    12,    -1,    14,    15,
     112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    29,    30,    31,    -1,    33,    -1,    35,
      36,    37,    38,    -1,    40,    41,    42,    43,    44,    45,
      -1,    47,    -1,    -1,    -1,    51,    52,    53,    54,    55,
      56,    -1,    -1,    59,    60,    61,    -1,    -1,    64,    -1,
      -1,    -1,    -1,    -1,    70,    -1,    -1,    73,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    81,    82,    83,    -1,    -1,
      -1,    -1,    -1,    89,    90,    91,    -1,    -1,    -1,    95,
       1,    -1,     3,     4,     5,     6,   102,     8,     9,    10,
      11,    12,    -1,    14,    15,    -1,   112,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    29,    30,
      31,    -1,    33,    -1,    35,    36,    37,    38,    -1,    40,
      41,    42,    43,    44,    45,    -1,    47,    -1,    -1,    -1,
      51,    52,    53,    54,    55,    56,    -1,    -1,    59,    60,
      61,    -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,    70,
      -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      81,    82,    83,    -1,    -1,    -1,    -1,    -1,    89,    90,
      91,    -1,    -1,     1,    95,     3,     4,     5,     6,    -1,
       8,     9,    10,    11,    12,    -1,    14,    15,    -1,    -1,
     111,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    29,    30,    31,    -1,    33,    -1,    35,    36,    37,
      38,    -1,    40,    41,    42,    43,    44,    45,    -1,    47,
      -1,    -1,    -1,    51,    52,    53,    54,    55,    56,    -1,
      -1,    59,    60,    61,    -1,    -1,    64,    -1,    -1,    -1,
      -1,    -1,    70,    -1,    -1,    73,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    81,    82,    83,    -1,    -1,    -1,    -1,
      -1,    89,    90,    91,    -1,    -1,     1,    95,     3,     4,
       5,     6,    -1,     8,     9,    10,    11,    12,    -1,    14,
      15,    -1,    -1,   111,   112,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    29,    30,    31,    32,    33,    -1,
      35,    36,    37,    38,    -1,    40,    41,    42,    43,    44,
      45,    -1,    47,    -1,    -1,    -1,    51,    52,    53,    54,
      55,    56,    -1,    -1,    59,    60,    61,    -1,    -1,    64,
      -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    73,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    81,    82,    83,    -1,
      -1,    -1,    -1,    -1,    89,    90,    91,    -1,    -1,     1,
      95,     3,     4,     5,     6,    -1,     8,     9,    10,    11,
      12,    -1,    14,    15,    -1,    -1,    -1,   112,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    29,    30,    31,
      32,    33,    -1,    35,    36,    37,    38,    -1,    40,    41,
      42,    43,    44,    45,    -1,    47,    -1,    -1,    -1,    51,
      52,    53,    54,    55,    56,    -1,    -1,    59,    60,    -1,
      -1,    63,    64,    -1,    -1,    -1,    -1,    -1,    70,    -1,
      -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,
      82,    83,    -1,    -1,    -1,    -1,    -1,    89,    90,    91,
      -1,    -1,     1,    95,     3,     4,     5,     6,    -1,     8,
       9,    10,    11,    12,    -1,    14,    15,    -1,    -1,    -1,
     112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      29,    30,    31,    -1,    33,    -1,    35,    36,    37,    38,
      -1,    40,    41,    42,    43,    44,    45,    -1,    47,    -1,
      -1,    -1,    51,    52,    53,    54,    55,    56,    -1,    -1,
      59,    60,    61,    -1,    -1,    64,    -1,    -1,    -1,    -1,
      -1,    70,    -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    81,    82,    83,    -1,    -1,    -1,    -1,    -1,
      89,    90,    91,    -1,    -1,     1,    95,     3,     4,     5,
       6,    -1,     8,     9,    10,    11,    12,    -1,    14,    15,
      -1,    -1,    -1,   112,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    29,    30,    31,    -1,    33,    -1,    35,
      36,    37,    38,    -1,    40,    41,    42,    43,    44,    45,
      -1,    47,    -1,    -1,    -1,    51,    52,    53,    54,    55,
      56,    -1,    -1,    59,    60,    -1,    -1,    -1,    64,    65,
      -1,    -1,    -1,    -1,    70,    -1,    -1,    73,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    81,    82,    83,    -1,    -1,
      -1,    -1,    -1,    89,    90,    91,    -1,    -1,     1,    95,
       3,     4,     5,     6,    -1,     8,     9,    10,    11,    12,
      -1,    14,    15,    -1,    -1,    -1,   112,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    29,    30,    31,    -1,
      33,    -1,    35,    36,    37,    38,    -1,    40,    41,    42,
      43,    44,    45,    -1,    47,    -1,    -1,    -1,    51,    52,
      53,    54,    55,    56,    -1,    -1,    59,    60,    -1,    -1,
      -1,    64,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,
      73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,    82,
      83,    -1,    -1,    -1,    -1,    -1,    89,    90,    91,    -1,
      -1,     1,    95,     3,     4,     5,     6,    -1,     8,     9,
      10,    11,    12,    -1,    14,    15,    -1,   110,    -1,   112,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    29,
      30,    31,    -1,    33,    -1,    35,    36,    37,    38,    -1,
      40,    41,    42,    43,    44,    45,    -1,    47,    -1,    -1,
      -1,    51,    52,    53,    54,    55,    56,    -1,    -1,    59,
      60,    -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,
      70,    -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    81,    82,    83,    -1,    -1,    -1,    -1,    -1,    89,
      90,    91,    -1,    -1,     1,    95,     3,     4,     5,     6,
      -1,     8,     9,    10,    11,    12,    -1,    14,    15,    -1,
      -1,    -1,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    29,    30,    31,    -1,    33,    -1,    35,    36,
      37,    38,    -1,    40,    41,    42,    43,    44,    45,    -1,
      47,    -1,    -1,    -1,    51,    52,    53,    54,    55,    56,
      -1,    -1,    59,    60,    -1,    -1,    -1,    64,    -1,    -1,
      -1,    -1,    -1,    70,    -1,    -1,    73,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    81,    82,    83,    -1,    -1,    -1,
      -1,    -1,    89,    90,    91,    -1,    -1,     1,    95,     3,
       4,     5,     6,    -1,     8,     9,    10,    11,    12,    -1,
      14,    15,    -1,    -1,    -1,   112,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    29,    30,    31,    -1,    33,
      -1,    35,    36,    37,    38,    -1,    40,    41,    42,    43,
      44,    45,    -1,    47,    -1,    -1,    -1,    51,    52,    53,
      54,    55,    56,    -1,    -1,    59,    60,    -1,    -1,    -1,
      64,    -1,    -1,    -1,    -1,    -1,    70,     1,    -1,    73,
       4,     5,    -1,    -1,     8,     9,    -1,    81,    82,    83,
      -1,    15,    -1,    -1,    -1,    89,    90,    91,    -1,    -1,
      -1,    95,    -1,    -1,    -1,    29,    -1,    31,    32,    -1,
      -1,    -1,    -1,    -1,    38,    -1,    40,    41,   112,    -1,
      -1,    -1,    -1,    47,    -1,    49,    -1,    -1,    -1,    -1,
      -1,    -1,    56,    -1,    -1,    59,    60,    -1,    62,    -1,
      -1,    -1,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    -1,    -1,    89,    90,    91,    -1,    93,
      -1,    -1,    96,     3,     4,     5,     6,    -1,     8,     9,
      10,    11,    12,    -1,    14,    15,    -1,    -1,   112,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    29,
      30,    31,    -1,    33,    -1,    35,    36,    37,    38,    -1,
      40,    41,    42,    43,    44,    45,    -1,    47,    -1,    -1,
      -1,    51,    52,    53,    54,    55,    56,    -1,    -1,    59,
      60,    -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,
      70,    -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    81,    82,    83,    -1,    -1,    -1,    -1,    -1,    89,
      90,    91,    -1,    -1,    -1,    95,     3,     4,     5,     6,
      -1,     8,     9,    10,    11,    12,    -1,    14,    15,    -1,
      -1,    -1,   112,   113,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    29,    30,    31,    -1,    33,    -1,    35,    36,
      37,    38,    -1,    40,    41,    42,    43,    44,    45,    -1,
      47,    -1,    -1,    -1,    51,    52,    53,    54,    55,    56,
      -1,    -1,    59,    60,    -1,    -1,    -1,    64,    -1,    -1,
      -1,    -1,    -1,    70,    -1,    -1,    73,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    81,    82,    83,    -1,    -1,    -1,
      -1,    -1,    89,    90,    91,    -1,    -1,    -1,    95,     3,
       4,     5,     6,    -1,     8,     9,    10,    11,    12,    -1,
      14,    15,    -1,    -1,    -1,   112,   113,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    29,    30,    31,    -1,    33,
      -1,    35,    36,    37,    38,    -1,    40,    41,    42,    43,
      44,    45,    -1,    47,    -1,    -1,    -1,    51,    52,    53,
      54,    55,    56,    -1,    -1,    59,    60,    -1,    -1,    -1,
      64,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    73,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,    82,    83,
      -1,    -1,    -1,    -1,    -1,    89,    90,    91,    -1,    -1,
      -1,    95,     3,     4,     5,     6,    -1,     8,     9,    10,
      11,    12,    -1,    14,    15,    -1,    -1,    -1,   112,   113,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    29,    30,
      31,    -1,    33,    -1,    35,    36,    37,    38,    -1,    40,
      41,    42,    43,    44,    45,    -1,    47,    -1,    -1,    -1,
      51,    52,    53,    54,    55,    56,    -1,    -1,    59,    60,
      -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,    70,
      -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      81,    82,    83,    -1,    -1,    -1,    -1,    -1,    89,    90,
      91,    -1,    -1,    -1,    95,     3,     4,     5,     6,    -1,
       8,     9,    10,    11,    12,    -1,    14,    15,    -1,    -1,
      -1,   112,   113,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    29,    30,    31,    -1,    33,    -1,    35,    36,    37,
      38,    -1,    40,    41,    42,    43,    44,    45,    -1,    47,
      -1,    -1,    -1,    51,    52,    53,    54,    55,    56,    -1,
      -1,    59,    60,    -1,    -1,    -1,    64,    -1,    -1,    -1,
      -1,    -1,    70,    -1,    -1,    73,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    81,    82,    83,    -1,    -1,    -1,    -1,
      -1,    89,    90,    91,    -1,    -1,    -1,    95,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   112,   113,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    -1,    14,    15,    16,
      -1,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    -1,    35,    36,
      37,    38,    -1,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    -1,    -1,    51,    52,    53,    54,    55,    56,
      -1,    -1,    59,    60,    61,    -1,    63,    64,    -1,    -1,
      -1,    -1,    -1,    70,    -1,    -1,    73,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    81,    82,    83,    -1,    -1,    -1,
      -1,    -1,    89,    90,    91,    -1,    -1,    -1,    95,    -1,
      97,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    -1,    14,    15,    16,   112,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    -1,    35,    36,    37,    38,    -1,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    -1,    -1,    51,
      52,    53,    54,    55,    56,    -1,    -1,    59,    60,    -1,
      -1,    63,    64,    -1,    -1,    -1,    -1,    -1,    70,    -1,
      -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,
      82,    83,    -1,    -1,    -1,    -1,    -1,    89,    90,    91,
      -1,    -1,    -1,    95,    -1,    97,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    -1,
     112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    29,    30,    31,    32,    33,    -1,    35,    36,
      37,    38,    -1,    40,    41,    42,    43,    44,    45,    -1,
      47,    -1,    49,    -1,    51,    52,    53,    54,    55,    56,
      -1,    -1,    59,    60,    -1,    -1,    -1,    64,    -1,    -1,
      -1,    -1,    -1,    70,    -1,    -1,    73,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    81,    82,    83,    -1,    -1,    -1,
      -1,    -1,    89,    90,    91,    -1,    -1,    -1,    95,    96,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    -1,    -1,   112,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    29,    30,    31,    32,
      33,    -1,    35,    36,    37,    38,    -1,    40,    41,    42,
      43,    44,    45,    -1,    47,    -1,    49,    -1,    51,    52,
      53,    54,    55,    56,    -1,    -1,    59,    60,    -1,    -1,
      -1,    64,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,
      73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,    82,
      83,    -1,    -1,    -1,    -1,    -1,    89,    90,    91,    -1,
      -1,    -1,    95,    96,     3,     4,     5,     6,    -1,     8,
       9,    10,    11,    12,    -1,    14,    15,    -1,    -1,   112,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      29,    30,    31,    32,    33,    -1,    35,    36,    37,    38,
      -1,    40,    41,    42,    43,    44,    45,    -1,    47,    -1,
      49,    -1,    51,    52,    53,    54,    55,    56,    -1,    -1,
      59,    60,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    70,    -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    81,    82,    83,    -1,    -1,    -1,    -1,    -1,
      89,    90,    91,    -1,    -1,    -1,    95,    96,     3,     4,
       5,     6,    -1,     8,     9,    10,    11,    12,    -1,    14,
      15,    -1,    -1,   112,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    29,    30,    31,    32,    33,    -1,
      35,    36,    37,    38,    -1,    40,    41,    42,    43,    44,
      45,    -1,    47,    -1,    -1,    -1,    51,    52,    53,    54,
      55,    56,    -1,    -1,    59,    60,    -1,    -1,    -1,    64,
      -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    73,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    81,    82,    83,    -1,
      -1,    -1,    -1,    -1,    89,    90,    91,    -1,    -1,    -1,
      95,     3,     4,     5,     6,    -1,     8,     9,    10,    11,
      12,    -1,    14,    15,    -1,    -1,    -1,   112,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    29,    30,    31,
      32,    33,    -1,    35,    36,    37,    38,    -1,    40,    41,
      42,    43,    44,    45,    -1,    47,    -1,    -1,    -1,    51,
      52,    53,    54,    55,    56,    -1,    -1,    59,    60,    -1,
      -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,    70,    -1,
      -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,
      82,    83,    -1,    -1,    -1,    -1,    -1,    89,    90,    91,
      -1,    -1,    -1,    95,     3,     4,     5,     6,    -1,     8,
       9,    10,    11,    12,    -1,    14,    15,    -1,    -1,    -1,
     112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      29,    30,    31,    32,    33,    -1,    35,    36,    37,    38,
      -1,    40,    41,    42,    43,    44,    45,    -1,    47,    -1,
      -1,    -1,    51,    52,    53,    54,    55,    56,    -1,    -1,
      59,    60,    -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,
      -1,    70,    -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    81,    82,    83,    -1,    -1,    -1,    -1,    -1,
      89,    90,    91,    -1,    -1,    -1,    95,     3,     4,     5,
       6,    -1,     8,     9,    10,    11,    12,    -1,    14,    15,
      -1,    -1,    -1,   112,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    29,    30,    31,    -1,    33,    -1,    35,
      36,    37,    38,    -1,    40,    41,    42,    43,    44,    45,
      -1,    47,    -1,    -1,    -1,    51,    52,    53,    54,    55,
      56,    -1,    -1,    59,    60,    -1,    -1,    63,    64,    -1,
      -1,    -1,    -1,    -1,    70,    -1,    -1,    73,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    81,    82,    83,    -1,    -1,
      -1,    -1,    -1,    89,    90,    91,    -1,    -1,    -1,    95,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      -1,    14,    15,    -1,    -1,    -1,   112,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    29,    30,    31,    32,
      33,    -1,    35,    36,    37,    38,    -1,    40,    41,    42,
      43,    44,    45,    -1,    47,    -1,    -1,    -1,    51,    52,
      53,    54,    55,    56,    -1,    -1,    59,    60,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,
      73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,    82,
      83,    -1,    -1,    -1,    -1,    -1,    89,    90,    91,    -1,
      -1,    -1,    95,     3,     4,     5,     6,    -1,     8,     9,
      10,    11,    12,    -1,    14,    15,    -1,    -1,    -1,   112,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    29,
      30,    31,    32,    33,    -1,    35,    36,    37,    38,    -1,
      40,    41,    42,    43,    44,    45,    -1,    47,    -1,    -1,
      -1,    51,    52,    53,    54,    55,    56,    -1,    -1,    59,
      60,    -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,
      70,    -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    81,    82,    83,    -1,    -1,    -1,    -1,    -1,    89,
      90,    91,    -1,    -1,    -1,    95,     3,     4,     5,     6,
      -1,     8,     9,    10,    11,    12,    -1,    14,    15,    -1,
      -1,    -1,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    29,    30,    31,    -1,    33,    -1,    35,    36,
      37,    38,    -1,    40,    41,    42,    43,    44,    45,    -1,
      47,    -1,    -1,    -1,    51,    52,    53,    54,    55,    56,
      -1,    -1,    59,    60,    -1,    -1,    -1,    64,    -1,    -1,
      -1,    -1,    -1,    70,    -1,    -1,    73,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    81,    82,    83,    -1,    -1,    -1,
      -1,    -1,    89,    90,    91,    -1,    -1,    -1,    95,     3,
       4,     5,     6,    -1,     8,     9,    10,    11,    12,    -1,
      14,    15,    -1,    -1,    -1,   112,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    29,    30,    31,    -1,    33,
      -1,    35,    36,    37,    38,    -1,    40,    41,    42,    43,
      44,    45,    -1,    47,    -1,    -1,    -1,    51,    52,    53,
      54,    55,    56,    -1,    -1,    59,    60,    61,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    73,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,    82,    83,
      -1,    -1,    -1,    -1,    -1,    89,    90,    91,    -1,    -1,
      -1,    95,     3,     4,     5,     6,    -1,     8,     9,    10,
      11,    12,    -1,    14,    15,    -1,    -1,    -1,   112,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    29,    30,
      31,    -1,    33,    -1,    35,    36,    37,    38,    -1,    40,
      41,    42,    43,    44,    45,    -1,    47,    -1,    -1,    -1,
      51,    52,    53,    54,    55,    56,    -1,    -1,    59,    60,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,
      -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      81,    82,    83,    -1,    -1,    -1,    -1,    -1,    89,    90,
      91,    -1,    -1,    -1,    95,    96,     3,     4,     5,     6,
      -1,     8,     9,    10,    11,    12,    -1,    14,    15,    -1,
      -1,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    29,    30,    31,    -1,    33,    -1,    35,    36,
      37,    38,    -1,    40,    41,    42,    43,    44,    45,    -1,
      47,    -1,    -1,    -1,    51,    52,    53,    54,    55,    56,
      -1,    -1,    59,    60,    -1,    -1,    -1,    64,    -1,    -1,
      -1,    -1,    -1,    70,    -1,    -1,    73,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    81,    82,    83,    -1,    -1,    -1,
      -1,    -1,    89,    90,    91,    -1,    -1,    -1,    95,     3,
       4,     5,     6,    -1,     8,     9,    10,    11,    12,    -1,
      14,    15,    -1,    -1,    -1,   112,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    29,    30,    31,    32,    33,
      -1,    35,    36,    37,    38,    -1,    40,    41,    42,    43,
      44,    45,    -1,    47,    -1,    -1,    -1,    51,    52,    53,
      54,    55,    56,    -1,    -1,    59,    60,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    73,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,    82,    83,
      -1,    -1,    -1,    -1,    -1,    89,    90,    91,    -1,    -1,
      -1,    95,     3,     4,     5,     6,    -1,     8,     9,    10,
      11,    12,    -1,    14,    15,    -1,    -1,    -1,   112,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    29,    30,
      31,    -1,    33,    -1,    35,    36,    37,    38,    -1,    40,
      41,    42,    43,    44,    45,    -1,    47,    -1,    -1,    -1,
      51,    52,    53,    54,    55,    56,    -1,    -1,    59,    60,
      -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,    70,
      -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      81,    82,    83,    -1,    -1,    -1,    -1,    -1,    89,    90,
      91,    -1,    -1,    -1,    95,     3,     4,     5,     6,    -1,
       8,     9,    10,    11,    12,    -1,    14,    15,    -1,    -1,
      -1,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    29,    30,    31,    32,    33,    -1,    35,    36,    37,
      38,    -1,    40,    41,    42,    43,    44,    45,    -1,    47,
      -1,    -1,    -1,    51,    52,    53,    54,    55,    56,    -1,
      -1,    59,    60,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    70,    -1,    -1,    73,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    81,    82,    83,    -1,    -1,    -1,    -1,
      -1,    89,    90,    91,    -1,    -1,    -1,    95,     3,     4,
       5,     6,    -1,     8,     9,    10,    11,    12,    -1,    14,
      15,    -1,    -1,    -1,   112,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    29,    30,    31,    -1,    33,    -1,
      35,    36,    37,    38,    -1,    40,    41,    42,    43,    44,
      45,    -1,    47,    -1,    -1,    -1,    51,    52,    53,    54,
      55,    56,    -1,    -1,    59,    60,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    73,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    81,    82,    83,    -1,
      -1,    -1,    -1,    -1,    89,    90,    91,    -1,    -1,    -1,
      95,     3,     4,     5,     6,    -1,     8,     9,    10,    11,
      12,    -1,    14,    15,    -1,    -1,    -1,   112,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    29,    30,    31,
      -1,    33,    -1,    35,    36,    37,    38,    -1,    40,    41,
      42,    43,    44,    45,    -1,    47,    -1,    -1,    -1,    51,
      52,    53,    54,    55,    56,    -1,    -1,    59,    60,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,
      -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,
      82,    83,    -1,    -1,    -1,    -1,    -1,    89,    90,    91,
      -1,    -1,    -1,    95,     3,     4,     5,     6,    -1,     8,
       9,    10,    11,    12,    -1,    14,    15,    -1,    -1,    -1,
     112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      29,    30,    31,    -1,    33,    -1,    35,    36,    37,    38,
      -1,    40,    41,    42,    43,    44,    45,    -1,    47,    -1,
      -1,    -1,    51,    52,    53,    54,    55,    56,    -1,    -1,
      59,    60,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    70,    -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    81,    82,    83,    -1,    -1,    -1,    -1,    -1,
      89,    90,    91,    -1,    -1,    -1,    95,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      62,    -1,    -1,   112,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87
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
#line 487 "parse.y"
{ finish_translation_unit (); ;
    break;}
case 2:
#line 489 "parse.y"
{ finish_translation_unit (); ;
    break;}
case 3:
#line 497 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 4:
#line 499 "parse.y"
{ yyval.ttype = NULL_TREE; ggc_collect (); ;
    break;}
case 5:
#line 501 "parse.y"
{ yyval.ttype = NULL_TREE; ggc_collect (); ;
    break;}
case 8:
#line 510 "parse.y"
{ have_extern_spec = true;
		  yyval.ttype = NULL_TREE; ;
    break;}
case 9:
#line 514 "parse.y"
{ have_extern_spec = false; ;
    break;}
case 10:
#line 519 "parse.y"
{ yyval.itype = pedantic;
		  pedantic = 0; ;
    break;}
case 12:
#line 528 "parse.y"
{ if (pending_lang_change) do_pending_lang_change();
		  type_lookups = NULL_TREE; ;
    break;}
case 13:
#line 531 "parse.y"
{ if (! toplevel_bindings_p ())
		  pop_everything (); ;
    break;}
case 14:
#line 537 "parse.y"
{ do_pending_inlines (); ;
    break;}
case 15:
#line 539 "parse.y"
{ do_pending_inlines (); ;
    break;}
case 16:
#line 542 "parse.y"
{ warning ("keyword `export' not implemented, and will be ignored"); ;
    break;}
case 17:
#line 544 "parse.y"
{ do_pending_inlines (); ;
    break;}
case 18:
#line 546 "parse.y"
{ do_pending_inlines (); ;
    break;}
case 19:
#line 548 "parse.y"
{ assemble_asm (yyvsp[-2].ttype); ;
    break;}
case 20:
#line 550 "parse.y"
{ pop_lang_context (); ;
    break;}
case 21:
#line 552 "parse.y"
{ do_pending_inlines (); pop_lang_context (); ;
    break;}
case 22:
#line 554 "parse.y"
{ do_pending_inlines (); pop_lang_context (); ;
    break;}
case 23:
#line 556 "parse.y"
{ push_namespace (yyvsp[-1].ttype); ;
    break;}
case 24:
#line 558 "parse.y"
{ pop_namespace (); ;
    break;}
case 25:
#line 560 "parse.y"
{ push_namespace (NULL_TREE); ;
    break;}
case 26:
#line 562 "parse.y"
{ pop_namespace (); ;
    break;}
case 28:
#line 565 "parse.y"
{ do_toplevel_using_decl (yyvsp[-1].ttype); ;
    break;}
case 30:
#line 568 "parse.y"
{ pedantic = yyvsp[-1].itype; ;
    break;}
case 31:
#line 573 "parse.y"
{ begin_only_namespace_names (); ;
    break;}
case 32:
#line 575 "parse.y"
{
		  end_only_namespace_names ();
		  if (lastiddecl)
		    yyvsp[-1].ttype = lastiddecl;
		  do_namespace_alias (yyvsp[-4].ttype, yyvsp[-1].ttype);
		;
    break;}
case 33:
#line 585 "parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 34:
#line 587 "parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 35:
#line 589 "parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 36:
#line 594 "parse.y"
{ yyval.ttype = build_nt (SCOPE_REF, yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 37:
#line 596 "parse.y"
{ yyval.ttype = build_nt (SCOPE_REF, global_namespace, yyvsp[0].ttype); ;
    break;}
case 38:
#line 598 "parse.y"
{ yyval.ttype = build_nt (SCOPE_REF, yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 39:
#line 603 "parse.y"
{ begin_only_namespace_names (); ;
    break;}
case 40:
#line 605 "parse.y"
{
		  end_only_namespace_names ();
		  /* If no declaration was found, the using-directive is
		     invalid. Since that was not reported, we need the
		     identifier for the error message. */
		  if (TREE_CODE (yyvsp[-1].ttype) == IDENTIFIER_NODE && lastiddecl)
		    yyvsp[-1].ttype = lastiddecl;
		  do_using_directive (yyvsp[-1].ttype);
		;
    break;}
case 41:
#line 618 "parse.y"
{
		  if (TREE_CODE (yyval.ttype) == IDENTIFIER_NODE)
		    yyval.ttype = lastiddecl;
		  got_scope = yyval.ttype;
		;
    break;}
case 42:
#line 624 "parse.y"
{
		  yyval.ttype = yyvsp[-1].ttype;
		  if (TREE_CODE (yyval.ttype) == IDENTIFIER_NODE)
		    yyval.ttype = lastiddecl;
		  got_scope = yyval.ttype;
		;
    break;}
case 45:
#line 636 "parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 46:
#line 638 "parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 47:
#line 643 "parse.y"
{ push_lang_context (yyvsp[0].ttype); ;
    break;}
case 48:
#line 645 "parse.y"
{ if (current_lang_name != yyvsp[0].ttype)
		    error ("use of linkage spec `%D' is different from previous spec `%D'", yyvsp[0].ttype, current_lang_name);
		  pop_lang_context (); push_lang_context (yyvsp[0].ttype); ;
    break;}
case 49:
#line 652 "parse.y"
{ begin_template_parm_list (); ;
    break;}
case 50:
#line 654 "parse.y"
{ yyval.ttype = end_template_parm_list (yyvsp[-1].ttype); ;
    break;}
case 51:
#line 659 "parse.y"
{ begin_specialization();
		  yyval.ttype = NULL_TREE; ;
    break;}
case 54:
#line 670 "parse.y"
{ yyval.ttype = process_template_parm (NULL_TREE, yyvsp[0].ttype); ;
    break;}
case 55:
#line 672 "parse.y"
{ yyval.ttype = process_template_parm (yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 56:
#line 677 "parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 57:
#line 679 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 58:
#line 684 "parse.y"
{ yyval.ttype = finish_template_type_parm (yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 59:
#line 686 "parse.y"
{ yyval.ttype = finish_template_type_parm (class_type_node, yyvsp[0].ttype); ;
    break;}
case 60:
#line 691 "parse.y"
{ yyval.ttype = finish_template_template_parm (yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 61:
#line 703 "parse.y"
{ yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ttype); ;
    break;}
case 62:
#line 705 "parse.y"
{ yyval.ttype = build_tree_list (groktypename (yyvsp[0].ftype.t), yyvsp[-2].ttype); ;
    break;}
case 63:
#line 707 "parse.y"
{ yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ftype.t); ;
    break;}
case 64:
#line 709 "parse.y"
{ yyval.ttype = build_tree_list (yyvsp[0].ttype, yyvsp[-2].ftype.t); ;
    break;}
case 65:
#line 711 "parse.y"
{ yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ttype); ;
    break;}
case 66:
#line 713 "parse.y"
{
		  yyvsp[0].ttype = check_template_template_default_arg (yyvsp[0].ttype);
		  yyval.ttype = build_tree_list (yyvsp[0].ttype, yyvsp[-2].ttype);
		;
    break;}
case 67:
#line 721 "parse.y"
{ finish_template_decl (yyvsp[-1].ttype); ;
    break;}
case 68:
#line 723 "parse.y"
{ finish_template_decl (yyvsp[-1].ttype); ;
    break;}
case 69:
#line 728 "parse.y"
{ do_pending_inlines (); ;
    break;}
case 70:
#line 730 "parse.y"
{ do_pending_inlines (); ;
    break;}
case 71:
#line 732 "parse.y"
{ do_pending_inlines (); ;
    break;}
case 72:
#line 734 "parse.y"
{ do_pending_inlines ();
		  pop_lang_context (); ;
    break;}
case 73:
#line 737 "parse.y"
{ do_pending_inlines ();
		  pop_lang_context (); ;
    break;}
case 74:
#line 740 "parse.y"
{ pedantic = yyvsp[-1].itype; ;
    break;}
case 76:
#line 746 "parse.y"
{;
    break;}
case 77:
#line 748 "parse.y"
{ note_list_got_semicolon (yyvsp[-2].ftype.t); ;
    break;}
case 78:
#line 750 "parse.y"
{
		  if (yyvsp[-1].ftype.t != error_mark_node)
                    {
		      maybe_process_partial_specialization (yyvsp[-1].ftype.t);
		      note_got_semicolon (yyvsp[-1].ftype.t);
	            }
                ;
    break;}
case 80:
#line 762 "parse.y"
{;
    break;}
case 81:
#line 764 "parse.y"
{ note_list_got_semicolon (yyvsp[-2].ftype.t); ;
    break;}
case 82:
#line 766 "parse.y"
{ pedwarn ("empty declaration"); ;
    break;}
case 84:
#line 769 "parse.y"
{
		  tree t, attrs;
		  split_specs_attrs (yyvsp[-1].ftype.t, &t, &attrs);
		  shadow_tag (t);
		  note_list_got_semicolon (yyvsp[-1].ftype.t);
		;
    break;}
case 87:
#line 778 "parse.y"
{ end_input (); ;
    break;}
case 97:
#line 804 "parse.y"
{ yyval.ttype = begin_compound_stmt (/*has_no_scope=*/1); ;
    break;}
case 98:
#line 806 "parse.y"
{
		  STMT_LINENO (yyvsp[-1].ttype) = yyvsp[-3].itype;
		  finish_compound_stmt (/*has_no_scope=*/1, yyvsp[-1].ttype);
		  finish_function_body (yyvsp[-5].ttype);
		;
    break;}
case 99:
#line 815 "parse.y"
{ expand_body (finish_function (0)); ;
    break;}
case 100:
#line 817 "parse.y"
{ expand_body (finish_function (0)); ;
    break;}
case 101:
#line 819 "parse.y"
{ ;
    break;}
case 102:
#line 824 "parse.y"
{ yyval.ttype = begin_constructor_declarator (yyvsp[-2].ttype, yyvsp[-1].ttype); ;
    break;}
case 103:
#line 826 "parse.y"
{ yyval.ttype = make_call_declarator (yyvsp[-4].ttype, yyvsp[-3].ttype, yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 104:
#line 828 "parse.y"
{ yyval.ttype = begin_constructor_declarator (yyvsp[-4].ttype, yyvsp[-3].ttype);
		  yyval.ttype = make_call_declarator (yyval.ttype, empty_parms (), yyvsp[-1].ttype, yyvsp[0].ttype);
		;
    break;}
case 105:
#line 832 "parse.y"
{ yyval.ttype = begin_constructor_declarator (yyvsp[-2].ttype, yyvsp[-1].ttype); ;
    break;}
case 106:
#line 834 "parse.y"
{ yyval.ttype = make_call_declarator (yyvsp[-4].ttype, yyvsp[-3].ttype, yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 107:
#line 836 "parse.y"
{ yyval.ttype = begin_constructor_declarator (yyvsp[-4].ttype, yyvsp[-3].ttype);
		  yyval.ttype = make_call_declarator (yyval.ttype, empty_parms (), yyvsp[-1].ttype, yyvsp[0].ttype);
		;
    break;}
case 108:
#line 840 "parse.y"
{ yyval.ttype = begin_constructor_declarator (yyvsp[-2].ttype, yyvsp[-1].ttype); ;
    break;}
case 109:
#line 842 "parse.y"
{ yyval.ttype = make_call_declarator (yyvsp[-4].ttype, yyvsp[-3].ttype, yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 110:
#line 844 "parse.y"
{ yyval.ttype = begin_constructor_declarator (yyvsp[-4].ttype, yyvsp[-3].ttype);
		  yyval.ttype = make_call_declarator (yyval.ttype, empty_parms (), yyvsp[-1].ttype, yyvsp[0].ttype);
		;
    break;}
case 111:
#line 848 "parse.y"
{ yyval.ttype = begin_constructor_declarator (yyvsp[-2].ttype, yyvsp[-1].ttype); ;
    break;}
case 112:
#line 850 "parse.y"
{ yyval.ttype = make_call_declarator (yyvsp[-4].ttype, yyvsp[-3].ttype, yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 113:
#line 852 "parse.y"
{ yyval.ttype = begin_constructor_declarator (yyvsp[-4].ttype, yyvsp[-3].ttype);
		  yyval.ttype = make_call_declarator (yyval.ttype, empty_parms (), yyvsp[-1].ttype, yyvsp[0].ttype);
		;
    break;}
case 114:
#line 859 "parse.y"
{ check_for_new_type ("return type", yyvsp[-1].ftype);
		  if (!parse_begin_function_definition (yyvsp[-1].ftype.t, yyvsp[0].ttype))
		    YYERROR1; ;
    break;}
case 115:
#line 863 "parse.y"
{ if (!parse_begin_function_definition (yyvsp[-1].ftype.t, yyvsp[0].ttype))
		    YYERROR1; ;
    break;}
case 116:
#line 866 "parse.y"
{ if (!parse_begin_function_definition (NULL_TREE, yyvsp[0].ttype))
		    YYERROR1; ;
    break;}
case 117:
#line 869 "parse.y"
{ if (!parse_begin_function_definition (yyvsp[-1].ftype.t, yyvsp[0].ttype))
		    YYERROR1; ;
    break;}
case 118:
#line 872 "parse.y"
{ if (!parse_begin_function_definition (NULL_TREE, yyvsp[0].ttype))
		    YYERROR1; ;
    break;}
case 119:
#line 881 "parse.y"
{ yyval.ttype = make_call_declarator (yyvsp[-5].ttype, yyvsp[-3].ttype, yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 120:
#line 884 "parse.y"
{ yyval.ttype = make_call_declarator (yyvsp[-6].ttype, yyvsp[-3].ttype, yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 121:
#line 886 "parse.y"
{ yyval.ttype = make_call_declarator (yyvsp[-3].ttype, empty_parms (), yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 122:
#line 888 "parse.y"
{ yyval.ttype = make_call_declarator (yyvsp[-4].ttype, empty_parms (), yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 123:
#line 890 "parse.y"
{ yyval.ttype = make_call_declarator (yyvsp[-5].ttype, yyvsp[-3].ttype, yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 124:
#line 892 "parse.y"
{ yyval.ttype = make_call_declarator (yyvsp[-3].ttype, empty_parms (), yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 125:
#line 899 "parse.y"
{ yyval.ttype = parse_method (yyvsp[0].ttype, yyvsp[-1].ftype.t, yyvsp[-1].ftype.lookups);
		 rest_of_mdef:
		  if (! yyval.ttype)
		    YYERROR1;
		  if (yychar == YYEMPTY)
		    yychar = YYLEX;
		  snarf_method (yyval.ttype); ;
    break;}
case 126:
#line 907 "parse.y"
{ yyval.ttype = parse_method (yyvsp[0].ttype, NULL_TREE, NULL_TREE);
		  goto rest_of_mdef; ;
    break;}
case 127:
#line 910 "parse.y"
{ yyval.ttype = parse_method (yyvsp[0].ttype, yyvsp[-1].ftype.t, yyvsp[-1].ftype.lookups); goto rest_of_mdef;;
    break;}
case 128:
#line 912 "parse.y"
{ yyval.ttype = parse_method (yyvsp[0].ttype, yyvsp[-1].ftype.t, yyvsp[-1].ftype.lookups); goto rest_of_mdef;;
    break;}
case 129:
#line 914 "parse.y"
{ yyval.ttype = parse_method (yyvsp[0].ttype, NULL_TREE, NULL_TREE);
		  goto rest_of_mdef; ;
    break;}
case 130:
#line 917 "parse.y"
{ yyval.ttype = parse_method (yyvsp[0].ttype, yyvsp[-1].ftype.t, yyvsp[-1].ftype.lookups); goto rest_of_mdef;;
    break;}
case 131:
#line 919 "parse.y"
{ yyval.ttype = parse_method (yyvsp[0].ttype, NULL_TREE, NULL_TREE);
		  goto rest_of_mdef; ;
    break;}
case 132:
#line 925 "parse.y"
{
		  yyval.ttype = yyvsp[0].ttype;
		;
    break;}
case 133:
#line 932 "parse.y"
{ finish_named_return_value (yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 134:
#line 934 "parse.y"
{ finish_named_return_value (yyval.ttype, yyvsp[-1].ttype); ;
    break;}
case 135:
#line 936 "parse.y"
{ finish_named_return_value (yyval.ttype, NULL_TREE); ;
    break;}
case 136:
#line 940 "parse.y"
{ begin_mem_initializers (); ;
    break;}
case 137:
#line 941 "parse.y"
{
		  if (yyvsp[0].ftype.new_type_flag == 0)
		    error ("no base or member initializers given following ':'");
		  finish_mem_initializers (yyvsp[0].ftype.t);
		;
    break;}
case 138:
#line 950 "parse.y"
{
		  yyval.ttype = begin_function_body ();
		;
    break;}
case 139:
#line 957 "parse.y"
{
		  yyval.ftype.new_type_flag = 0;
		  yyval.ftype.t = NULL_TREE;
		;
    break;}
case 140:
#line 962 "parse.y"
{
		  yyval.ftype.new_type_flag = 1;
		  yyval.ftype.t = yyvsp[0].ttype;
		;
    break;}
case 141:
#line 967 "parse.y"
{
		  if (yyvsp[0].ttype)
		    {
		      yyval.ftype.new_type_flag = 1;
		      TREE_CHAIN (yyvsp[0].ttype) = yyvsp[-2].ftype.t;
		      yyval.ftype.t = yyvsp[0].ttype;
		    }
		  else
		    yyval.ftype = yyvsp[-2].ftype;
		;
    break;}
case 143:
#line 982 "parse.y"
{
 		  if (current_class_name)
		    pedwarn ("anachronistic old style base class initializer");
		  yyval.ttype = expand_member_init (NULL_TREE);
		  in_base_initializer = yyval.ttype && !DECL_P (yyval.ttype);
		;
    break;}
case 144:
#line 989 "parse.y"
{ yyval.ttype = expand_member_init (yyvsp[0].ttype);
		  in_base_initializer = yyval.ttype && !DECL_P (yyval.ttype); ;
    break;}
case 145:
#line 992 "parse.y"
{ yyval.ttype = expand_member_init (yyvsp[0].ttype);
		  in_base_initializer = yyval.ttype && !DECL_P (yyval.ttype); ;
    break;}
case 146:
#line 995 "parse.y"
{ yyval.ttype = expand_member_init (yyvsp[0].ttype);
		  in_base_initializer = yyval.ttype && !DECL_P (yyval.ttype); ;
    break;}
case 147:
#line 1001 "parse.y"
{ in_base_initializer = 0;
		  yyval.ttype = yyvsp[-3].ttype ? build_tree_list (yyvsp[-3].ttype, yyvsp[-1].ttype) : NULL_TREE; ;
    break;}
case 148:
#line 1004 "parse.y"
{ in_base_initializer = 0;
		  yyval.ttype = yyvsp[-1].ttype ? build_tree_list (yyvsp[-1].ttype, void_type_node) : NULL_TREE; ;
    break;}
case 149:
#line 1007 "parse.y"
{ in_base_initializer = 0;
		  yyval.ttype = NULL_TREE; ;
    break;}
case 161:
#line 1033 "parse.y"
{ do_type_instantiation (yyvsp[-1].ftype.t, NULL_TREE, 1);
		  yyungetc (';', 1); ;
    break;}
case 163:
#line 1037 "parse.y"
{ tree specs = strip_attrs (yyvsp[-1].ftype.t);
		  parse_decl_instantiation (specs, yyvsp[0].ttype, NULL_TREE); ;
    break;}
case 165:
#line 1041 "parse.y"
{ parse_decl_instantiation (NULL_TREE, yyvsp[0].ttype, NULL_TREE); ;
    break;}
case 167:
#line 1044 "parse.y"
{ parse_decl_instantiation (NULL_TREE, yyvsp[0].ttype, NULL_TREE); ;
    break;}
case 169:
#line 1047 "parse.y"
{ do_type_instantiation (yyvsp[-1].ftype.t, yyvsp[-4].ttype, 1);
		  yyungetc (';', 1); ;
    break;}
case 170:
#line 1050 "parse.y"
{;
    break;}
case 171:
#line 1053 "parse.y"
{ tree specs = strip_attrs (yyvsp[-1].ftype.t);
		  parse_decl_instantiation (specs, yyvsp[0].ttype, yyvsp[-4].ttype); ;
    break;}
case 172:
#line 1056 "parse.y"
{;
    break;}
case 173:
#line 1058 "parse.y"
{ parse_decl_instantiation (NULL_TREE, yyvsp[0].ttype, yyvsp[-3].ttype); ;
    break;}
case 174:
#line 1060 "parse.y"
{;
    break;}
case 175:
#line 1062 "parse.y"
{ parse_decl_instantiation (NULL_TREE, yyvsp[0].ttype, yyvsp[-3].ttype); ;
    break;}
case 176:
#line 1064 "parse.y"
{;
    break;}
case 177:
#line 1068 "parse.y"
{ begin_explicit_instantiation(); ;
    break;}
case 178:
#line 1072 "parse.y"
{ end_explicit_instantiation(); ;
    break;}
case 179:
#line 1082 "parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 180:
#line 1085 "parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 183:
#line 1093 "parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 184:
#line 1099 "parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 185:
#line 1103 "parse.y"
{
		  if (yychar == YYEMPTY)
		    yychar = YYLEX;

		  yyval.ttype = finish_template_type (yyvsp[-3].ttype, yyvsp[-1].ttype,
					     yychar == SCOPE);
		;
    break;}
case 187:
#line 1115 "parse.y"
{
		  /* Handle `Class<Class<Type>>' without space in the `>>' */
		  pedwarn ("`>>' should be `> >' in template class name");
		  yyungetc ('>', 1);
		;
    break;}
case 188:
#line 1124 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 190:
#line 1130 "parse.y"
{ yyval.ttype = build_tree_list (NULL_TREE, yyval.ttype); ;
    break;}
case 191:
#line 1132 "parse.y"
{ yyval.ttype = chainon (yyval.ttype, build_tree_list (NULL_TREE, yyvsp[0].ttype)); ;
    break;}
case 192:
#line 1136 "parse.y"
{ ++class_template_ok_as_expr; ;
    break;}
case 193:
#line 1138 "parse.y"
{ 
		  --class_template_ok_as_expr; 
		  yyval.ttype = yyvsp[0].ttype; 
		;
    break;}
case 194:
#line 1146 "parse.y"
{ yyval.ttype = groktypename (yyvsp[0].ftype.t); ;
    break;}
case 195:
#line 1148 "parse.y"
{
		  yyval.ttype = lastiddecl;
		  if (DECL_TEMPLATE_TEMPLATE_PARM_P (yyval.ttype))
		    yyval.ttype = TREE_TYPE (yyval.ttype);
		;
    break;}
case 196:
#line 1154 "parse.y"
{
		  yyval.ttype = lastiddecl;
		  if (DECL_TEMPLATE_TEMPLATE_PARM_P (yyval.ttype))
		    yyval.ttype = TREE_TYPE (yyval.ttype);
		;
    break;}
case 198:
#line 1161 "parse.y"
{
		  if (!processing_template_decl)
		    {
		      error ("use of template qualifier outside template");
		      yyval.ttype = error_mark_node;
		    }
		  else
		    yyval.ttype = make_unbound_class_template (yyvsp[-2].ttype, yyvsp[0].ttype, tf_error | tf_parsing);
		;
    break;}
case 199:
#line 1174 "parse.y"
{ yyval.code = NEGATE_EXPR; ;
    break;}
case 200:
#line 1176 "parse.y"
{ yyval.code = CONVERT_EXPR; ;
    break;}
case 201:
#line 1178 "parse.y"
{ yyval.code = PREINCREMENT_EXPR; ;
    break;}
case 202:
#line 1180 "parse.y"
{ yyval.code = PREDECREMENT_EXPR; ;
    break;}
case 203:
#line 1182 "parse.y"
{ yyval.code = TRUTH_NOT_EXPR; ;
    break;}
case 204:
#line 1187 "parse.y"
{ yyval.ttype = build_x_compound_expr (yyval.ttype); ;
    break;}
case 206:
#line 1193 "parse.y"
{ error ("ISO C++ forbids an empty condition for `%s'",
			 cond_stmt_keyword);
		  yyval.ttype = integer_zero_node; ;
    break;}
case 207:
#line 1197 "parse.y"
{ yyval.ttype = yyvsp[-1].ttype; ;
    break;}
case 208:
#line 1202 "parse.y"
{ error ("ISO C++ forbids an empty condition for `%s'",
			 cond_stmt_keyword);
		  yyval.ttype = integer_zero_node; ;
    break;}
case 209:
#line 1206 "parse.y"
{ yyval.ttype = yyvsp[-1].ttype; ;
    break;}
case 210:
#line 1211 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 212:
#line 1214 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 213:
#line 1219 "parse.y"
{ {
		  tree d;
		  for (d = getdecls (); d; d = TREE_CHAIN (d))
		    if (TREE_CODE (d) == TYPE_DECL) {
		      tree s = TREE_TYPE (d);
		      if (TREE_CODE (s) == RECORD_TYPE)
			error ("definition of class `%T' in condition", s);
		      else if (TREE_CODE (s) == ENUMERAL_TYPE)
			error ("definition of enum `%T' in condition", s);
		    }
		  }
		  current_declspecs = yyvsp[-4].ftype.t;
		  yyval.ttype = parse_decl (yyvsp[-3].ttype, yyvsp[-1].ttype, 1);
		;
    break;}
case 214:
#line 1234 "parse.y"
{
		  parse_end_decl (yyvsp[-1].ttype, yyvsp[0].ttype, yyvsp[-3].ttype);
		  yyval.ttype = convert_from_reference (yyvsp[-1].ttype);
		  if (TREE_CODE (TREE_TYPE (yyval.ttype)) == ARRAY_TYPE)
		    error ("definition of array `%#D' in condition", yyval.ttype);
		;
    break;}
case 220:
#line 1252 "parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyval.ttype,
		                  build_tree_list (NULL_TREE, yyvsp[0].ttype)); ;
    break;}
case 221:
#line 1255 "parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyval.ttype,
		                  build_tree_list (NULL_TREE, error_mark_node)); ;
    break;}
case 222:
#line 1258 "parse.y"
{ chainon (yyval.ttype, build_tree_list (NULL_TREE, yyvsp[0].ttype)); ;
    break;}
case 223:
#line 1260 "parse.y"
{ chainon (yyval.ttype, build_tree_list (NULL_TREE, error_mark_node)); ;
    break;}
case 224:
#line 1265 "parse.y"
{ yyval.ttype = build_tree_list (NULL_TREE, yyval.ttype); ;
    break;}
case 226:
#line 1271 "parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 227:
#line 1274 "parse.y"
{ yyval.ttype = yyvsp[0].ttype;
		  pedantic = yyvsp[-1].itype; ;
    break;}
case 228:
#line 1277 "parse.y"
{ yyval.ttype = build_x_indirect_ref (yyvsp[0].ttype, "unary *"); ;
    break;}
case 229:
#line 1279 "parse.y"
{ yyval.ttype = build_x_unary_op (ADDR_EXPR, yyvsp[0].ttype); ;
    break;}
case 230:
#line 1281 "parse.y"
{ yyval.ttype = build_x_unary_op (BIT_NOT_EXPR, yyvsp[0].ttype); ;
    break;}
case 231:
#line 1283 "parse.y"
{ yyval.ttype = finish_unary_op_expr (yyvsp[-1].code, yyvsp[0].ttype); ;
    break;}
case 232:
#line 1286 "parse.y"
{ yyval.ttype = finish_label_address_expr (yyvsp[0].ttype); ;
    break;}
case 233:
#line 1288 "parse.y"
{ yyval.ttype = finish_sizeof (yyvsp[0].ttype);
		  skip_evaluation--; ;
    break;}
case 234:
#line 1291 "parse.y"
{ yyval.ttype = finish_sizeof (groktypename (yyvsp[-1].ftype.t));
		  check_for_new_type ("sizeof", yyvsp[-1].ftype);
		  skip_evaluation--; ;
    break;}
case 235:
#line 1295 "parse.y"
{ yyval.ttype = finish_alignof (yyvsp[0].ttype);
		  skip_evaluation--; ;
    break;}
case 236:
#line 1298 "parse.y"
{ yyval.ttype = finish_alignof (groktypename (yyvsp[-1].ftype.t));
		  check_for_new_type ("alignof", yyvsp[-1].ftype);
		  skip_evaluation--; ;
    break;}
case 237:
#line 1305 "parse.y"
{ yyval.ttype = build_new (NULL_TREE, yyvsp[0].ftype.t, NULL_TREE, yyvsp[-1].itype);
		  check_for_new_type ("new", yyvsp[0].ftype); ;
    break;}
case 238:
#line 1308 "parse.y"
{ yyval.ttype = build_new (NULL_TREE, yyvsp[-1].ftype.t, yyvsp[0].ttype, yyvsp[-2].itype);
		  check_for_new_type ("new", yyvsp[-1].ftype); ;
    break;}
case 239:
#line 1311 "parse.y"
{ yyval.ttype = build_new (yyvsp[-1].ttype, yyvsp[0].ftype.t, NULL_TREE, yyvsp[-2].itype);
		  check_for_new_type ("new", yyvsp[0].ftype); ;
    break;}
case 240:
#line 1314 "parse.y"
{ yyval.ttype = build_new (yyvsp[-2].ttype, yyvsp[-1].ftype.t, yyvsp[0].ttype, yyvsp[-3].itype);
		  check_for_new_type ("new", yyvsp[-1].ftype); ;
    break;}
case 241:
#line 1318 "parse.y"
{ yyval.ttype = build_new (NULL_TREE, groktypename(yyvsp[-1].ftype.t),
				  NULL_TREE, yyvsp[-3].itype);
		  check_for_new_type ("new", yyvsp[-1].ftype); ;
    break;}
case 242:
#line 1322 "parse.y"
{ yyval.ttype = build_new (NULL_TREE, groktypename(yyvsp[-2].ftype.t), yyvsp[0].ttype, yyvsp[-4].itype);
		  check_for_new_type ("new", yyvsp[-2].ftype); ;
    break;}
case 243:
#line 1325 "parse.y"
{ yyval.ttype = build_new (yyvsp[-3].ttype, groktypename(yyvsp[-1].ftype.t), NULL_TREE, yyvsp[-4].itype);
		  check_for_new_type ("new", yyvsp[-1].ftype); ;
    break;}
case 244:
#line 1328 "parse.y"
{ yyval.ttype = build_new (yyvsp[-4].ttype, groktypename(yyvsp[-2].ftype.t), yyvsp[0].ttype, yyvsp[-5].itype);
		  check_for_new_type ("new", yyvsp[-2].ftype); ;
    break;}
case 245:
#line 1332 "parse.y"
{ yyval.ttype = delete_sanity (yyvsp[0].ttype, NULL_TREE, 0, yyvsp[-1].itype); ;
    break;}
case 246:
#line 1334 "parse.y"
{ yyval.ttype = delete_sanity (yyvsp[0].ttype, NULL_TREE, 1, yyvsp[-3].itype);
		  if (yychar == YYEMPTY)
		    yychar = YYLEX; ;
    break;}
case 247:
#line 1338 "parse.y"
{ yyval.ttype = delete_sanity (yyvsp[0].ttype, yyvsp[-2].ttype, 2, yyvsp[-4].itype);
		  if (yychar == YYEMPTY)
		    yychar = YYLEX; ;
    break;}
case 248:
#line 1342 "parse.y"
{ yyval.ttype = build_x_unary_op (REALPART_EXPR, yyvsp[0].ttype); ;
    break;}
case 249:
#line 1344 "parse.y"
{ yyval.ttype = build_x_unary_op (IMAGPART_EXPR, yyvsp[0].ttype); ;
    break;}
case 250:
#line 1349 "parse.y"
{ yyval.ttype = yyvsp[-1].ttype; ;
    break;}
case 251:
#line 1351 "parse.y"
{ pedwarn ("old style placement syntax, use () instead");
		  yyval.ttype = yyvsp[-1].ttype; ;
    break;}
case 252:
#line 1357 "parse.y"
{ yyval.ttype = yyvsp[-1].ttype; ;
    break;}
case 253:
#line 1359 "parse.y"
{ yyval.ttype = void_zero_node; ;
    break;}
case 254:
#line 1361 "parse.y"
{
		  error ("`%T' is not a valid expression", yyvsp[-1].ftype.t);
		  yyval.ttype = error_mark_node;
		;
    break;}
case 255:
#line 1366 "parse.y"
{
		  /* This was previously allowed as an extension, but
		     was removed in G++ 3.3.  */
		  error ("initialization of new expression with `='");
		  yyval.ttype = error_mark_node;
		;
    break;}
case 256:
#line 1377 "parse.y"
{ yyvsp[-1].ftype.t = finish_parmlist (build_tree_list (NULL_TREE, yyvsp[-1].ftype.t), 0);
		  yyval.ttype = make_call_declarator (NULL_TREE, yyvsp[-1].ftype.t, NULL_TREE, NULL_TREE);
		  check_for_new_type ("cast", yyvsp[-1].ftype); ;
    break;}
case 257:
#line 1381 "parse.y"
{ yyvsp[-1].ftype.t = finish_parmlist (build_tree_list (NULL_TREE, yyvsp[-1].ftype.t), 0);
		  yyval.ttype = make_call_declarator (yyval.ttype, yyvsp[-1].ftype.t, NULL_TREE, NULL_TREE);
		  check_for_new_type ("cast", yyvsp[-1].ftype); ;
    break;}
case 259:
#line 1389 "parse.y"
{ yyval.ttype = reparse_absdcl_as_casts (yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 260:
#line 1391 "parse.y"
{
		  tree init = build_nt (CONSTRUCTOR, NULL_TREE,
					nreverse (yyvsp[-2].ttype));
		  if (pedantic)
		    pedwarn ("ISO C++ forbids compound literals");
		  /* Indicate that this was a C99 compound literal.  */
		  TREE_HAS_CONSTRUCTOR (init) = 1;

		  yyval.ttype = reparse_absdcl_as_casts (yyval.ttype, init);
		;
    break;}
case 262:
#line 1407 "parse.y"
{ yyval.ttype = build_x_binary_op (MEMBER_REF, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 263:
#line 1409 "parse.y"
{ yyval.ttype = build_m_component_ref (yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 264:
#line 1411 "parse.y"
{ yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 265:
#line 1413 "parse.y"
{ yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 266:
#line 1415 "parse.y"
{ yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 267:
#line 1417 "parse.y"
{ yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 268:
#line 1419 "parse.y"
{ yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 269:
#line 1421 "parse.y"
{ yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 270:
#line 1423 "parse.y"
{ yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 271:
#line 1425 "parse.y"
{ yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 272:
#line 1427 "parse.y"
{ yyval.ttype = build_x_binary_op (LT_EXPR, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 273:
#line 1429 "parse.y"
{ yyval.ttype = build_x_binary_op (GT_EXPR, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 274:
#line 1431 "parse.y"
{ yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 275:
#line 1433 "parse.y"
{ yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 276:
#line 1435 "parse.y"
{ yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 277:
#line 1437 "parse.y"
{ yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 278:
#line 1439 "parse.y"
{ yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 279:
#line 1441 "parse.y"
{ yyval.ttype = build_x_binary_op (TRUTH_ANDIF_EXPR, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 280:
#line 1443 "parse.y"
{ yyval.ttype = build_x_binary_op (TRUTH_ORIF_EXPR, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 281:
#line 1445 "parse.y"
{ yyval.ttype = build_x_conditional_expr (yyval.ttype, yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 282:
#line 1447 "parse.y"
{ yyval.ttype = build_x_modify_expr (yyval.ttype, NOP_EXPR, yyvsp[0].ttype);
		  if (yyval.ttype != error_mark_node)
                    C_SET_EXP_ORIGINAL_CODE (yyval.ttype, MODIFY_EXPR); ;
    break;}
case 283:
#line 1451 "parse.y"
{ yyval.ttype = build_x_modify_expr (yyval.ttype, yyvsp[-1].code, yyvsp[0].ttype); ;
    break;}
case 284:
#line 1453 "parse.y"
{ yyval.ttype = build_throw (NULL_TREE); ;
    break;}
case 285:
#line 1455 "parse.y"
{ yyval.ttype = build_throw (yyvsp[0].ttype); ;
    break;}
case 287:
#line 1462 "parse.y"
{ yyval.ttype = build_x_binary_op (MEMBER_REF, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 288:
#line 1464 "parse.y"
{ yyval.ttype = build_m_component_ref (yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 289:
#line 1466 "parse.y"
{ yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 290:
#line 1468 "parse.y"
{ yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 291:
#line 1470 "parse.y"
{ yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 292:
#line 1472 "parse.y"
{ yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 293:
#line 1474 "parse.y"
{ yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 294:
#line 1476 "parse.y"
{ yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 295:
#line 1478 "parse.y"
{ yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 296:
#line 1480 "parse.y"
{ yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 297:
#line 1482 "parse.y"
{ yyval.ttype = build_x_binary_op (LT_EXPR, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 298:
#line 1484 "parse.y"
{ yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 299:
#line 1486 "parse.y"
{ yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 300:
#line 1488 "parse.y"
{ yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 301:
#line 1490 "parse.y"
{ yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 302:
#line 1492 "parse.y"
{ yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 303:
#line 1494 "parse.y"
{ yyval.ttype = build_x_binary_op (TRUTH_ANDIF_EXPR, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 304:
#line 1496 "parse.y"
{ yyval.ttype = build_x_binary_op (TRUTH_ORIF_EXPR, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 305:
#line 1498 "parse.y"
{ yyval.ttype = build_x_conditional_expr (yyval.ttype, yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 306:
#line 1500 "parse.y"
{ yyval.ttype = build_x_modify_expr (yyval.ttype, NOP_EXPR, yyvsp[0].ttype);
		  if (yyval.ttype != error_mark_node)
                    C_SET_EXP_ORIGINAL_CODE (yyval.ttype, MODIFY_EXPR); ;
    break;}
case 307:
#line 1504 "parse.y"
{ yyval.ttype = build_x_modify_expr (yyval.ttype, yyvsp[-1].code, yyvsp[0].ttype); ;
    break;}
case 308:
#line 1506 "parse.y"
{ yyval.ttype = build_throw (NULL_TREE); ;
    break;}
case 309:
#line 1508 "parse.y"
{ yyval.ttype = build_throw (yyvsp[0].ttype); ;
    break;}
case 310:
#line 1513 "parse.y"
{ yyval.ttype = build_nt (BIT_NOT_EXPR, yyvsp[0].ttype); ;
    break;}
case 311:
#line 1515 "parse.y"
{ yyval.ttype = build_nt (BIT_NOT_EXPR, yyvsp[0].ttype); ;
    break;}
case 317:
#line 1524 "parse.y"
{
		  /* If lastiddecl is a BASELINK we're in an
		     expression like S::f<int>, so don't
		     do_identifier; we only do that for unqualified
		     identifiers.  */
	          if (!lastiddecl || !BASELINK_P (lastiddecl))
		    yyval.ttype = do_identifier (yyvsp[-1].ttype, 3, NULL_TREE);
		  else
		    yyval.ttype = yyvsp[-1].ttype;
		;
    break;}
case 318:
#line 1538 "parse.y"
{ 
		  tree template_name = yyvsp[-2].ttype;
		  if (TREE_CODE (template_name) == COMPONENT_REF)
		    template_name = TREE_OPERAND (template_name, 1);
		  yyval.ttype = lookup_template_function (template_name, yyvsp[-1].ttype); 
		;
    break;}
case 319:
#line 1545 "parse.y"
{ 
		  tree template_name = yyvsp[-2].ttype;
		  if (TREE_CODE (template_name) == COMPONENT_REF)
		    template_name = TREE_OPERAND (template_name, 1);
		  yyval.ttype = lookup_template_function (template_name, yyvsp[-1].ttype); 
		;
    break;}
case 320:
#line 1555 "parse.y"
{ yyval.ttype = lookup_template_function (yyvsp[-3].ttype, yyvsp[-1].ttype); ;
    break;}
case 321:
#line 1557 "parse.y"
{ yyval.ttype = lookup_template_function (yyvsp[-3].ttype, yyvsp[-1].ttype); ;
    break;}
case 322:
#line 1560 "parse.y"
{ yyval.ttype = lookup_template_function (yyvsp[-3].ttype, yyvsp[-1].ttype); ;
    break;}
case 327:
#line 1572 "parse.y"
{
		  /* Provide support for '(' attributes '*' declarator ')'
		     etc */
		  yyval.ttype = tree_cons (yyvsp[-1].ttype, yyvsp[0].ttype, NULL_TREE);
		;
    break;}
case 329:
#line 1582 "parse.y"
{ yyval.ttype = build_nt (INDIRECT_REF, yyvsp[0].ttype); ;
    break;}
case 330:
#line 1584 "parse.y"
{ yyval.ttype = build_nt (ADDR_EXPR, yyvsp[0].ttype); ;
    break;}
case 331:
#line 1586 "parse.y"
{ yyval.ttype = yyvsp[-1].ttype; ;
    break;}
case 332:
#line 1591 "parse.y"
{ yyval.ttype = lookup_template_function (yyvsp[-3].ttype, yyvsp[-1].ttype); ;
    break;}
case 333:
#line 1593 "parse.y"
{ yyval.ttype = lookup_template_function (yyvsp[-3].ttype, yyvsp[-1].ttype); ;
    break;}
case 337:
#line 1603 "parse.y"
{ yyval.ttype = finish_decl_parsing (yyvsp[-1].ttype); ;
    break;}
case 338:
#line 1608 "parse.y"
{
		  if (TREE_CODE (yyvsp[0].ttype) == BIT_NOT_EXPR)
		    yyval.ttype = build_x_unary_op (BIT_NOT_EXPR, TREE_OPERAND (yyvsp[0].ttype, 0));
		  else
		    yyval.ttype = finish_id_expr (yyvsp[0].ttype);
		;
    break;}
case 341:
#line 1617 "parse.y"
{
		  yyval.ttype = fix_string_type (yyval.ttype);
		  /* fix_string_type doesn't set up TYPE_MAIN_VARIANT of
		     a const array the way we want, so fix it.  */
		  if (flag_const_strings)
		    TREE_TYPE (yyval.ttype) = build_cplus_array_type
		      (TREE_TYPE (TREE_TYPE (yyval.ttype)),
		       TYPE_DOMAIN (TREE_TYPE (yyval.ttype)));
		;
    break;}
case 342:
#line 1627 "parse.y"
{ yyval.ttype = finish_fname (yyvsp[0].ttype); ;
    break;}
case 343:
#line 1629 "parse.y"
{ yyval.ttype = finish_parenthesized_expr (yyvsp[-1].ttype); ;
    break;}
case 344:
#line 1631 "parse.y"
{ yyvsp[-1].ttype = reparse_decl_as_expr (NULL_TREE, yyvsp[-1].ttype);
		  yyval.ttype = finish_parenthesized_expr (yyvsp[-1].ttype); ;
    break;}
case 345:
#line 1634 "parse.y"
{ yyval.ttype = error_mark_node; ;
    break;}
case 346:
#line 1636 "parse.y"
{ if (!at_function_scope_p ())
		    {
		      error ("braced-group within expression allowed only inside a function");
		      YYERROR;
		    }
		  if (pedantic)
		    pedwarn ("ISO C++ forbids braced-groups within expressions");
		  yyval.ttype = begin_stmt_expr ();
		;
    break;}
case 347:
#line 1646 "parse.y"
{ yyval.ttype = finish_stmt_expr (yyvsp[-2].ttype); ;
    break;}
case 348:
#line 1651 "parse.y"
{ yyval.ttype = parse_finish_call_expr (yyvsp[-3].ttype, yyvsp[-1].ttype, 1); ;
    break;}
case 349:
#line 1653 "parse.y"
{ yyval.ttype = parse_finish_call_expr (yyvsp[-1].ttype, NULL_TREE, 1); ;
    break;}
case 350:
#line 1655 "parse.y"
{ yyval.ttype = parse_finish_call_expr (yyvsp[-3].ttype, yyvsp[-1].ttype, 0); ;
    break;}
case 351:
#line 1657 "parse.y"
{ yyval.ttype = parse_finish_call_expr (yyvsp[-1].ttype, NULL_TREE, 0); ;
    break;}
case 352:
#line 1659 "parse.y"
{ yyval.ttype = build_x_va_arg (yyvsp[-3].ttype, groktypename (yyvsp[-1].ftype.t));
		  check_for_new_type ("__builtin_va_arg", yyvsp[-1].ftype); ;
    break;}
case 353:
#line 1662 "parse.y"
{ yyval.ttype = grok_array_decl (yyval.ttype, yyvsp[-1].ttype); ;
    break;}
case 354:
#line 1664 "parse.y"
{ yyval.ttype = finish_increment_expr (yyvsp[-1].ttype, POSTINCREMENT_EXPR); ;
    break;}
case 355:
#line 1666 "parse.y"
{ yyval.ttype = finish_increment_expr (yyvsp[-1].ttype, POSTDECREMENT_EXPR); ;
    break;}
case 356:
#line 1669 "parse.y"
{ yyval.ttype = finish_this_expr (); ;
    break;}
case 357:
#line 1671 "parse.y"
{
		  /* This is a C cast in C++'s `functional' notation
		     using the "implicit int" extension so that:
		     `const (3)' is equivalent to `const int (3)'.  */
		  tree type;

		  type = hash_tree_cons (NULL_TREE, yyvsp[-3].ttype, NULL_TREE);
		  type = groktypename (build_tree_list (type, NULL_TREE));
		  yyval.ttype = build_functional_cast (type, yyvsp[-1].ttype);
		;
    break;}
case 359:
#line 1683 "parse.y"
{ tree type = groktypename (yyvsp[-4].ftype.t);
		  check_for_new_type ("dynamic_cast", yyvsp[-4].ftype);
		  yyval.ttype = build_dynamic_cast (type, yyvsp[-1].ttype); ;
    break;}
case 360:
#line 1687 "parse.y"
{ tree type = groktypename (yyvsp[-4].ftype.t);
		  check_for_new_type ("static_cast", yyvsp[-4].ftype);
		  yyval.ttype = build_static_cast (type, yyvsp[-1].ttype); ;
    break;}
case 361:
#line 1691 "parse.y"
{ tree type = groktypename (yyvsp[-4].ftype.t);
		  check_for_new_type ("reinterpret_cast", yyvsp[-4].ftype);
		  yyval.ttype = build_reinterpret_cast (type, yyvsp[-1].ttype); ;
    break;}
case 362:
#line 1695 "parse.y"
{ tree type = groktypename (yyvsp[-4].ftype.t);
		  check_for_new_type ("const_cast", yyvsp[-4].ftype);
		  yyval.ttype = build_const_cast (type, yyvsp[-1].ttype); ;
    break;}
case 363:
#line 1699 "parse.y"
{ yyval.ttype = build_typeid (yyvsp[-1].ttype); ;
    break;}
case 364:
#line 1701 "parse.y"
{ tree type = groktypename (yyvsp[-1].ftype.t);
		  check_for_new_type ("typeid", yyvsp[-1].ftype);
		  yyval.ttype = get_typeid (type); ;
    break;}
case 365:
#line 1705 "parse.y"
{ yyval.ttype = parse_scoped_id (yyvsp[0].ttype); ;
    break;}
case 366:
#line 1707 "parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 367:
#line 1709 "parse.y"
{
		  got_scope = NULL_TREE;
		  if (TREE_CODE (yyvsp[0].ttype) == IDENTIFIER_NODE)
		    yyval.ttype = parse_scoped_id (yyvsp[0].ttype);
		  else
		    yyval.ttype = yyvsp[0].ttype;
		;
    break;}
case 368:
#line 1717 "parse.y"
{ yyval.ttype = build_offset_ref (OP0 (yyval.ttype), OP1 (yyval.ttype));
		  if (!class_template_ok_as_expr 
		      && DECL_CLASS_TEMPLATE_P (yyval.ttype))
		    {
		      error ("invalid use of template `%D'", yyval.ttype); 
		      yyval.ttype = error_mark_node;
		    }
                ;
    break;}
case 369:
#line 1726 "parse.y"
{ yyval.ttype = parse_finish_call_expr (yyvsp[-3].ttype, yyvsp[-1].ttype, 0); ;
    break;}
case 370:
#line 1728 "parse.y"
{ yyval.ttype = parse_finish_call_expr (yyvsp[-1].ttype, NULL_TREE, 0); ;
    break;}
case 371:
#line 1730 "parse.y"
{ yyval.ttype = finish_class_member_access_expr (yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 372:
#line 1732 "parse.y"
{ yyval.ttype = finish_object_call_expr (yyvsp[-3].ttype, yyvsp[-4].ttype, yyvsp[-1].ttype); ;
    break;}
case 373:
#line 1734 "parse.y"
{ yyval.ttype = finish_object_call_expr (yyvsp[-1].ttype, yyvsp[-2].ttype, NULL_TREE); ;
    break;}
case 374:
#line 1736 "parse.y"
{ yyval.ttype = finish_class_member_access_expr (yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 375:
#line 1738 "parse.y"
{ yyval.ttype = finish_class_member_access_expr (yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 376:
#line 1740 "parse.y"
{ yyval.ttype = finish_object_call_expr (yyvsp[-3].ttype, yyvsp[-4].ttype, yyvsp[-1].ttype); ;
    break;}
case 377:
#line 1742 "parse.y"
{ yyval.ttype = finish_object_call_expr (yyvsp[-1].ttype, yyvsp[-2].ttype, NULL_TREE); ;
    break;}
case 378:
#line 1744 "parse.y"
{ yyval.ttype = finish_qualified_object_call_expr (yyvsp[-3].ttype, yyvsp[-4].ttype, yyvsp[-1].ttype); ;
    break;}
case 379:
#line 1746 "parse.y"
{ yyval.ttype = finish_qualified_object_call_expr (yyvsp[-1].ttype, yyvsp[-2].ttype, NULL_TREE); ;
    break;}
case 380:
#line 1749 "parse.y"
{ yyval.ttype = finish_pseudo_destructor_call_expr (yyvsp[-3].ttype, NULL_TREE, yyvsp[-1].ttype); ;
    break;}
case 381:
#line 1751 "parse.y"
{ yyval.ttype = finish_pseudo_destructor_call_expr (yyvsp[-5].ttype, yyvsp[-4].ttype, yyvsp[-1].ttype); ;
    break;}
case 382:
#line 1753 "parse.y"
{
		  yyval.ttype = error_mark_node;
		;
    break;}
case 383:
#line 1798 "parse.y"
{ yyval.itype = 0; ;
    break;}
case 384:
#line 1800 "parse.y"
{ got_scope = NULL_TREE; yyval.itype = 1; ;
    break;}
case 385:
#line 1805 "parse.y"
{ yyval.itype = 0; ;
    break;}
case 386:
#line 1807 "parse.y"
{ got_scope = NULL_TREE; yyval.itype = 1; ;
    break;}
case 387:
#line 1812 "parse.y"
{ yyval.ttype = boolean_true_node; ;
    break;}
case 388:
#line 1814 "parse.y"
{ yyval.ttype = boolean_false_node; ;
    break;}
case 389:
#line 1819 "parse.y"
{
		  if (DECL_CONSTRUCTOR_P (current_function_decl))
		    finish_mem_initializers (NULL_TREE);
		;
    break;}
case 390:
#line 1827 "parse.y"
{ got_object = TREE_TYPE (yyval.ttype); ;
    break;}
case 391:
#line 1829 "parse.y"
{
		  yyval.ttype = build_x_arrow (yyval.ttype);
		  got_object = TREE_TYPE (yyval.ttype);
		;
    break;}
case 392:
#line 1837 "parse.y"
{
		  if (yyvsp[-2].ftype.t && IS_AGGR_TYPE_CODE (TREE_CODE (yyvsp[-2].ftype.t)))
		    note_got_semicolon (yyvsp[-2].ftype.t);
		;
    break;}
case 393:
#line 1842 "parse.y"
{
		  note_list_got_semicolon (yyvsp[-2].ftype.t);
		;
    break;}
case 394:
#line 1846 "parse.y"
{;
    break;}
case 395:
#line 1848 "parse.y"
{
		  shadow_tag (yyvsp[-1].ftype.t);
		  note_list_got_semicolon (yyvsp[-1].ftype.t);
		;
    break;}
case 396:
#line 1853 "parse.y"
{ warning ("empty declaration"); ;
    break;}
case 397:
#line 1855 "parse.y"
{ pedantic = yyvsp[-1].itype; ;
    break;}
case 400:
#line 1869 "parse.y"
{ yyval.ttype = make_call_declarator (NULL_TREE, empty_parms (),
					     NULL_TREE, NULL_TREE); ;
    break;}
case 401:
#line 1872 "parse.y"
{ yyval.ttype = make_call_declarator (yyval.ttype, empty_parms (), NULL_TREE,
					     NULL_TREE); ;
    break;}
case 402:
#line 1879 "parse.y"
{ yyval.ftype.t = build_tree_list (yyvsp[-1].ftype.t, yyvsp[0].ttype);
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;
    break;}
case 403:
#line 1882 "parse.y"
{ yyval.ftype.t = build_tree_list (yyvsp[-1].ftype.t, yyvsp[0].ttype);
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;
    break;}
case 404:
#line 1885 "parse.y"
{ yyval.ftype.t = build_tree_list (build_tree_list (NULL_TREE, yyvsp[-1].ftype.t),
					  yyvsp[0].ttype);
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;
    break;}
case 405:
#line 1889 "parse.y"
{ yyval.ftype.t = build_tree_list (yyvsp[0].ftype.t, NULL_TREE);
		  yyval.ftype.new_type_flag = yyvsp[0].ftype.new_type_flag;  ;
    break;}
case 406:
#line 1892 "parse.y"
{ yyval.ftype.t = build_tree_list (yyvsp[0].ftype.t, NULL_TREE);
		  yyval.ftype.new_type_flag = yyvsp[0].ftype.new_type_flag; ;
    break;}
case 407:
#line 1903 "parse.y"
{ yyval.ftype.lookups = type_lookups; ;
    break;}
case 408:
#line 1905 "parse.y"
{ yyval.ftype.lookups = type_lookups; ;
    break;}
case 409:
#line 1910 "parse.y"
{ yyval.ftype.t = tree_cons (NULL_TREE, yyvsp[0].ftype.t, yyvsp[-1].ftype.t);
		  yyval.ftype.new_type_flag = yyvsp[0].ftype.new_type_flag; ;
    break;}
case 410:
#line 1913 "parse.y"
{ yyval.ftype.t = tree_cons (NULL_TREE, yyvsp[-1].ftype.t, yyvsp[0].ttype);
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;
    break;}
case 411:
#line 1916 "parse.y"
{ yyval.ftype.t = tree_cons (NULL_TREE, yyvsp[-2].ftype.t, chainon (yyvsp[-1].ttype, yyvsp[0].ttype));
		  yyval.ftype.new_type_flag = yyvsp[-2].ftype.new_type_flag; ;
    break;}
case 412:
#line 1919 "parse.y"
{ yyval.ftype.t = tree_cons (NULL_TREE, yyvsp[-1].ftype.t, chainon (yyvsp[0].ttype, yyvsp[-2].ftype.t));
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;
    break;}
case 413:
#line 1922 "parse.y"
{ yyval.ftype.t = tree_cons (NULL_TREE, yyvsp[-1].ftype.t, chainon (yyvsp[0].ttype, yyvsp[-2].ftype.t));
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;
    break;}
case 414:
#line 1925 "parse.y"
{ yyval.ftype.t = tree_cons (NULL_TREE, yyvsp[-2].ftype.t,
				    chainon (yyvsp[-1].ttype, chainon (yyvsp[0].ttype, yyvsp[-3].ftype.t)));
		  yyval.ftype.new_type_flag = yyvsp[-2].ftype.new_type_flag; ;
    break;}
case 415:
#line 1932 "parse.y"
{ if (extra_warnings)
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyval.ttype));
		  yyval.ttype = build_tree_list (NULL_TREE, yyval.ttype); ;
    break;}
case 416:
#line 1937 "parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ftype.t, yyval.ttype); ;
    break;}
case 417:
#line 1939 "parse.y"
{ if (extra_warnings)
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyval.ttype); ;
    break;}
case 418:
#line 1961 "parse.y"
{ yyval.ftype.lookups = NULL_TREE; TREE_STATIC (yyval.ftype.t) = 1; ;
    break;}
case 419:
#line 1963 "parse.y"
{
		  yyval.ftype.t = hash_tree_cons (NULL_TREE, yyvsp[0].ttype, NULL_TREE);
		  yyval.ftype.new_type_flag = 0; yyval.ftype.lookups = NULL_TREE;
		;
    break;}
case 420:
#line 1968 "parse.y"
{
		  yyval.ftype.t = hash_tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ftype.t);
		  TREE_STATIC (yyval.ftype.t) = 1;
		;
    break;}
case 421:
#line 1973 "parse.y"
{
		  if (extra_warnings && TREE_STATIC (yyval.ftype.t))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ftype.t = hash_tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ftype.t);
		  TREE_STATIC (yyval.ftype.t) = TREE_STATIC (yyvsp[-1].ftype.t);
		;
    break;}
case 422:
#line 1981 "parse.y"
{ yyval.ftype.t = hash_tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ftype.t); ;
    break;}
case 423:
#line 1992 "parse.y"
{ yyval.ftype.t = build_tree_list (NULL_TREE, yyvsp[0].ftype.t);
		  yyval.ftype.new_type_flag = yyvsp[0].ftype.new_type_flag; ;
    break;}
case 424:
#line 1995 "parse.y"
{ yyval.ftype.t = tree_cons (NULL_TREE, yyvsp[0].ftype.t, yyvsp[-1].ftype.t);
		  yyval.ftype.new_type_flag = yyvsp[0].ftype.new_type_flag; ;
    break;}
case 425:
#line 1998 "parse.y"
{ yyval.ftype.t = tree_cons (NULL_TREE, yyvsp[-1].ftype.t, yyvsp[0].ttype);
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;
    break;}
case 426:
#line 2001 "parse.y"
{ yyval.ftype.t = tree_cons (NULL_TREE, yyvsp[-1].ftype.t, chainon (yyvsp[0].ttype, yyvsp[-2].ftype.t));
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;
    break;}
case 427:
#line 2007 "parse.y"
{ yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ftype.t); ;
    break;}
case 428:
#line 2009 "parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ftype.t, yyvsp[-1].ttype); ;
    break;}
case 429:
#line 2011 "parse.y"
{ yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ttype); ;
    break;}
case 430:
#line 2013 "parse.y"
{ yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, NULL_TREE); ;
    break;}
case 431:
#line 2017 "parse.y"
{ skip_evaluation++; ;
    break;}
case 432:
#line 2021 "parse.y"
{ skip_evaluation++; ;
    break;}
case 433:
#line 2025 "parse.y"
{ skip_evaluation++; ;
    break;}
case 434:
#line 2034 "parse.y"
{ yyval.ftype.lookups = NULL_TREE; ;
    break;}
case 435:
#line 2036 "parse.y"
{ yyval.ftype.t = yyvsp[0].ttype; yyval.ftype.new_type_flag = 0; yyval.ftype.lookups = NULL_TREE; ;
    break;}
case 436:
#line 2038 "parse.y"
{ yyval.ftype.t = yyvsp[0].ttype; yyval.ftype.new_type_flag = 0; yyval.ftype.lookups = NULL_TREE; ;
    break;}
case 437:
#line 2040 "parse.y"
{ yyval.ftype.t = finish_typeof (yyvsp[-1].ttype);
		  yyval.ftype.new_type_flag = 0; yyval.ftype.lookups = NULL_TREE;
		  skip_evaluation--; ;
    break;}
case 438:
#line 2044 "parse.y"
{ yyval.ftype.t = groktypename (yyvsp[-1].ftype.t);
		  yyval.ftype.new_type_flag = 0; yyval.ftype.lookups = NULL_TREE;
		  skip_evaluation--; ;
    break;}
case 439:
#line 2048 "parse.y"
{ tree type = TREE_TYPE (yyvsp[-1].ttype);

                  yyval.ftype.new_type_flag = 0; yyval.ftype.lookups = NULL_TREE;
		  if (IS_AGGR_TYPE (type))
		    {
		      sorry ("sigof type specifier");
		      yyval.ftype.t = type;
		    }
		  else
		    {
		      error ("`sigof' applied to non-aggregate expression");
		      yyval.ftype.t = error_mark_node;
		    }
		;
    break;}
case 440:
#line 2063 "parse.y"
{ tree type = groktypename (yyvsp[-1].ftype.t);

                  yyval.ftype.new_type_flag = 0; yyval.ftype.lookups = NULL_TREE;
		  if (IS_AGGR_TYPE (type))
		    {
		      sorry ("sigof type specifier");
		      yyval.ftype.t = type;
		    }
		  else
		    {
		      error("`sigof' applied to non-aggregate type");
		      yyval.ftype.t = error_mark_node;
		    }
		;
    break;}
case 441:
#line 2083 "parse.y"
{ yyval.ftype.t = yyvsp[0].ttype; yyval.ftype.new_type_flag = 0; ;
    break;}
case 442:
#line 2085 "parse.y"
{ yyval.ftype.t = yyvsp[0].ttype; yyval.ftype.new_type_flag = 0; ;
    break;}
case 445:
#line 2092 "parse.y"
{ check_multiple_declarators (); ;
    break;}
case 447:
#line 2098 "parse.y"
{ check_multiple_declarators (); ;
    break;}
case 449:
#line 2104 "parse.y"
{ check_multiple_declarators (); ;
    break;}
case 450:
#line 2109 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 451:
#line 2111 "parse.y"
{ yyval.ttype = yyvsp[-1].ttype; ;
    break;}
case 452:
#line 2116 "parse.y"
{ yyval.ttype = parse_decl (yyvsp[-3].ttype, yyvsp[-1].ttype, 1); ;
    break;}
case 453:
#line 2119 "parse.y"
{ parse_end_decl (yyvsp[-1].ttype, yyvsp[0].ttype, yyvsp[-4].ttype); ;
    break;}
case 454:
#line 2121 "parse.y"
{
		  yyval.ttype = parse_decl (yyvsp[-2].ttype, yyvsp[0].ttype, 0);
		  parse_end_decl (yyval.ttype, NULL_TREE, yyvsp[-1].ttype);
		;
    break;}
case 455:
#line 2135 "parse.y"
{ yyval.ttype = parse_decl0 (yyvsp[-3].ttype, yyvsp[-4].ftype.t,
					   yyvsp[-4].ftype.lookups, yyvsp[-1].ttype, 1); ;
    break;}
case 456:
#line 2140 "parse.y"
{ parse_end_decl (yyvsp[-1].ttype, yyvsp[0].ttype, yyvsp[-4].ttype); ;
    break;}
case 457:
#line 2142 "parse.y"
{ tree d = parse_decl0 (yyvsp[-2].ttype, yyvsp[-3].ftype.t,
					yyvsp[-3].ftype.lookups, yyvsp[0].ttype, 0);
		  parse_end_decl (d, NULL_TREE, yyvsp[-1].ttype); ;
    break;}
case 458:
#line 2149 "parse.y"
{;
    break;}
case 459:
#line 2154 "parse.y"
{;
    break;}
case 460:
#line 2159 "parse.y"
{ /* Set things up as initdcl0_innards expects.  */
	      yyval.ttype = yyvsp[0].ttype;
	      yyvsp[0].ttype = yyvsp[-1].ttype;
              yyvsp[-1].ftype.t = NULL_TREE;
	      yyvsp[-1].ftype.lookups = NULL_TREE; ;
    break;}
case 461:
#line 2165 "parse.y"
{;
    break;}
case 462:
#line 2167 "parse.y"
{ tree d = parse_decl0 (yyvsp[-2].ttype, NULL_TREE, NULL_TREE, yyvsp[0].ttype, 0);
		  parse_end_decl (d, NULL_TREE, yyvsp[-1].ttype); ;
    break;}
case 463:
#line 2175 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 464:
#line 2177 "parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 465:
#line 2182 "parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 466:
#line 2184 "parse.y"
{ yyval.ttype = chainon (yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 467:
#line 2189 "parse.y"
{ yyval.ttype = yyvsp[-2].ttype; ;
    break;}
case 468:
#line 2194 "parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 469:
#line 2196 "parse.y"
{ yyval.ttype = chainon (yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 470:
#line 2201 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 471:
#line 2203 "parse.y"
{ yyval.ttype = build_tree_list (yyvsp[0].ttype, NULL_TREE); ;
    break;}
case 472:
#line 2205 "parse.y"
{ yyval.ttype = build_tree_list (yyvsp[-3].ttype, build_tree_list (NULL_TREE, yyvsp[-1].ttype)); ;
    break;}
case 473:
#line 2207 "parse.y"
{ yyval.ttype = build_tree_list (yyvsp[-5].ttype, tree_cons (NULL_TREE, yyvsp[-3].ttype, yyvsp[-1].ttype)); ;
    break;}
case 474:
#line 2209 "parse.y"
{ yyval.ttype = build_tree_list (yyvsp[-3].ttype, yyvsp[-1].ttype); ;
    break;}
case 479:
#line 2225 "parse.y"
{ yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ttype); ;
    break;}
case 480:
#line 2227 "parse.y"
{ yyval.ttype = chainon (yyvsp[-2].ttype, build_tree_list (NULL_TREE, yyvsp[0].ttype)); ;
    break;}
case 481:
#line 2232 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 482:
#line 2234 "parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 484:
#line 2243 "parse.y"
{ yyval.ttype = build_nt (CONSTRUCTOR, NULL_TREE, NULL_TREE);
		  TREE_HAS_CONSTRUCTOR (yyval.ttype) = 1; ;
    break;}
case 485:
#line 2246 "parse.y"
{ yyval.ttype = build_nt (CONSTRUCTOR, NULL_TREE, nreverse (yyvsp[-1].ttype));
		  TREE_HAS_CONSTRUCTOR (yyval.ttype) = 1; ;
    break;}
case 486:
#line 2249 "parse.y"
{ yyval.ttype = build_nt (CONSTRUCTOR, NULL_TREE, nreverse (yyvsp[-2].ttype));
		  TREE_HAS_CONSTRUCTOR (yyval.ttype) = 1; ;
    break;}
case 487:
#line 2252 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 488:
#line 2259 "parse.y"
{ yyval.ttype = build_tree_list (NULL_TREE, yyval.ttype); ;
    break;}
case 489:
#line 2261 "parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyval.ttype); ;
    break;}
case 490:
#line 2264 "parse.y"
{ yyval.ttype = build_tree_list (yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 491:
#line 2266 "parse.y"
{ yyval.ttype = build_tree_list (yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 492:
#line 2268 "parse.y"
{ yyval.ttype = tree_cons (yyvsp[-2].ttype, yyvsp[0].ttype, yyval.ttype); ;
    break;}
case 493:
#line 2273 "parse.y"
{
		  expand_body (finish_function (2));
		  process_next_inline (yyvsp[-2].pi);
		;
    break;}
case 494:
#line 2278 "parse.y"
{
		  expand_body (finish_function (2));
                  process_next_inline (yyvsp[-2].pi);
		;
    break;}
case 495:
#line 2283 "parse.y"
{
		  finish_function (2);
		  process_next_inline (yyvsp[-2].pi); ;
    break;}
case 498:
#line 2297 "parse.y"
{ replace_defarg (yyvsp[-2].ttype, yyvsp[-1].ttype); ;
    break;}
case 499:
#line 2299 "parse.y"
{ replace_defarg (yyvsp[-2].ttype, error_mark_node); ;
    break;}
case 501:
#line 2305 "parse.y"
{ do_pending_defargs (); ;
    break;}
case 502:
#line 2307 "parse.y"
{ do_pending_defargs (); ;
    break;}
case 503:
#line 2312 "parse.y"
{ yyval.ttype = current_enum_type;
		  current_enum_type = start_enum (yyvsp[-1].ttype); ;
    break;}
case 504:
#line 2315 "parse.y"
{ yyval.ftype.t = current_enum_type;
		  finish_enum (current_enum_type);
		  yyval.ftype.new_type_flag = 1;
		  current_enum_type = yyvsp[-2].ttype;
		  check_for_missing_semicolon (yyval.ftype.t); ;
    break;}
case 505:
#line 2321 "parse.y"
{ yyval.ttype = current_enum_type;
		  current_enum_type = start_enum (make_anon_name ()); ;
    break;}
case 506:
#line 2324 "parse.y"
{ yyval.ftype.t = current_enum_type;
		  finish_enum (current_enum_type);
		  yyval.ftype.new_type_flag = 1;
		  current_enum_type = yyvsp[-2].ttype;
		  check_for_missing_semicolon (yyval.ftype.t); ;
    break;}
case 507:
#line 2330 "parse.y"
{ yyval.ftype.t = parse_xref_tag (enum_type_node, yyvsp[0].ttype, 1);
		  yyval.ftype.new_type_flag = 0; ;
    break;}
case 508:
#line 2333 "parse.y"
{ yyval.ftype.t = parse_xref_tag (enum_type_node, yyvsp[0].ttype, 1);
		  yyval.ftype.new_type_flag = 0; ;
    break;}
case 509:
#line 2336 "parse.y"
{ yyval.ftype.t = yyvsp[0].ttype;
		  yyval.ftype.new_type_flag = 0;
		  if (!processing_template_decl)
		    pedwarn ("using `typename' outside of template"); ;
    break;}
case 510:
#line 2342 "parse.y"
{
		  if (yyvsp[-1].ttype && yyvsp[-2].ftype.t != error_mark_node)
		    {
		      tree type = TREE_TYPE (yyvsp[-2].ftype.t);

		      if (TREE_CODE (type) == TYPENAME_TYPE)
			{
			  if (IMPLICIT_TYPENAME_P (type))
			    /* In a definition of a member class template,
			       we will get here with an implicit typename,
			       a TYPENAME_TYPE with a type. */
			    type = TREE_TYPE (type);
			  else
			    {
			      error ("qualified name does not name a class");
			      type = error_mark_node;
			    }
			}
		      maybe_process_partial_specialization (type);
		      xref_basetypes (type, yyvsp[-1].ttype);
		    }
		  yyvsp[-2].ftype.t = begin_class_definition (TREE_TYPE (yyvsp[-2].ftype.t));
		  check_class_key (current_aggr, yyvsp[-2].ftype.t);
                  current_aggr = NULL_TREE; ;
    break;}
case 511:
#line 2367 "parse.y"
{
		  int semi;
		  tree t;

		  if (yychar == YYEMPTY)
		    yychar = YYLEX;
		  semi = yychar == ';';

		  t = finish_class_definition (yyvsp[-6].ftype.t, yyvsp[0].ttype, semi, yyvsp[-6].ftype.new_type_flag);
		  yyval.ttype = t;

		  /* restore current_aggr */
		  current_aggr = TREE_CODE (t) != RECORD_TYPE
				 ? union_type_node
				 : CLASSTYPE_DECLARED_CLASS (t)
				 ? class_type_node : record_type_node;
		;
    break;}
case 512:
#line 2385 "parse.y"
{
		  done_pending_defargs ();
		  begin_inline_definitions ();
		;
    break;}
case 513:
#line 2390 "parse.y"
{
		  yyval.ftype.t = yyvsp[-3].ttype;
		  yyval.ftype.new_type_flag = 1;
		;
    break;}
case 514:
#line 2395 "parse.y"
{
		  yyval.ftype.t = TREE_TYPE (yyvsp[0].ftype.t);
		  yyval.ftype.new_type_flag = yyvsp[0].ftype.new_type_flag;
		  check_class_key (current_aggr, yyval.ftype.t);
		;
    break;}
case 518:
#line 2410 "parse.y"
{ if (pedantic && !in_system_header)
		    pedwarn ("comma at end of enumerator list"); ;
    break;}
case 520:
#line 2417 "parse.y"
{ error ("storage class specifier `%s' not allowed after struct or class", IDENTIFIER_POINTER (yyvsp[0].ttype)); ;
    break;}
case 521:
#line 2419 "parse.y"
{ error ("type specifier `%s' not allowed after struct or class", IDENTIFIER_POINTER (yyvsp[0].ttype)); ;
    break;}
case 522:
#line 2421 "parse.y"
{ error ("type qualifier `%s' not allowed after struct or class", IDENTIFIER_POINTER (yyvsp[0].ttype)); ;
    break;}
case 523:
#line 2423 "parse.y"
{ error ("no body nor ';' separates two class, struct or union declarations"); ;
    break;}
case 524:
#line 2425 "parse.y"
{ yyval.ttype = build_tree_list (yyvsp[0].ttype, yyvsp[-1].ttype); ;
    break;}
case 525:
#line 2430 "parse.y"
{
		  current_aggr = yyvsp[-1].ttype;
		  yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ttype);
		;
    break;}
case 526:
#line 2435 "parse.y"
{
		  current_aggr = yyvsp[-2].ttype;
		  yyval.ttype = build_tree_list (yyvsp[-1].ttype, yyvsp[0].ttype);
		;
    break;}
case 527:
#line 2440 "parse.y"
{
		  current_aggr = yyvsp[-3].ttype;
		  yyval.ttype = build_tree_list (yyvsp[-1].ttype, yyvsp[0].ttype);
		;
    break;}
case 528:
#line 2445 "parse.y"
{
		  current_aggr = yyvsp[-2].ttype;
		  yyval.ttype = build_tree_list (global_namespace, yyvsp[0].ttype);
		;
    break;}
case 529:
#line 2453 "parse.y"
{
		  current_aggr = yyvsp[-1].ttype;
		  yyval.ttype = yyvsp[0].ttype;
		;
    break;}
case 530:
#line 2458 "parse.y"
{
		  current_aggr = yyvsp[-2].ttype;
		  yyval.ttype = yyvsp[0].ttype;
		;
    break;}
case 531:
#line 2463 "parse.y"
{
		  current_aggr = yyvsp[-3].ttype;
		  yyval.ttype = yyvsp[0].ttype;
		;
    break;}
case 532:
#line 2471 "parse.y"
{
		  yyval.ftype.t = parse_handle_class_head (current_aggr,
						  TREE_PURPOSE (yyvsp[0].ttype), 
						  TREE_VALUE (yyvsp[0].ttype),
						  0, &yyval.ftype.new_type_flag);
		;
    break;}
case 533:
#line 2478 "parse.y"
{
		  current_aggr = yyvsp[-1].ttype;
		  yyval.ftype.t = TYPE_MAIN_DECL (parse_xref_tag (current_aggr, yyvsp[0].ttype, 0));
		  yyval.ftype.new_type_flag = 1;
		;
    break;}
case 534:
#line 2484 "parse.y"
{
		  yyval.ftype.t = yyvsp[0].ttype;
		  yyval.ftype.new_type_flag = 0;
		;
    break;}
case 535:
#line 2492 "parse.y"
{
		  yyungetc ('{', 1);
		  yyval.ftype.t = parse_handle_class_head (current_aggr,
						  TREE_PURPOSE (yyvsp[-1].ttype), 
						  TREE_VALUE (yyvsp[-1].ttype),
						  1, 
						  &yyval.ftype.new_type_flag);
		;
    break;}
case 536:
#line 2501 "parse.y"
{
		  yyungetc (':', 1);
		  yyval.ftype.t = parse_handle_class_head (current_aggr,
						  TREE_PURPOSE (yyvsp[-1].ttype), 
						  TREE_VALUE (yyvsp[-1].ttype),
						  1, &yyval.ftype.new_type_flag);
		;
    break;}
case 537:
#line 2509 "parse.y"
{
		  yyungetc ('{', 1);
		  yyval.ftype.t = handle_class_head_apparent_template
			   (yyvsp[-1].ttype, &yyval.ftype.new_type_flag);
		;
    break;}
case 538:
#line 2515 "parse.y"
{
		  yyungetc (':', 1);
		  yyval.ftype.t = handle_class_head_apparent_template
			   (yyvsp[-1].ttype, &yyval.ftype.new_type_flag);
		;
    break;}
case 539:
#line 2521 "parse.y"
{
		  yyungetc ('{', 1);
		  current_aggr = yyvsp[-2].ttype;
		  yyval.ftype.t = parse_handle_class_head (current_aggr,
						  NULL_TREE, yyvsp[-1].ttype,
						  1, &yyval.ftype.new_type_flag);
		;
    break;}
case 540:
#line 2529 "parse.y"
{
		  yyungetc (':', 1);
		  current_aggr = yyvsp[-2].ttype;
		  yyval.ftype.t = parse_handle_class_head (current_aggr,
						  NULL_TREE, yyvsp[-1].ttype,
						  1, &yyval.ftype.new_type_flag);
		;
    break;}
case 541:
#line 2537 "parse.y"
{
		  current_aggr = yyvsp[-1].ttype;
		  yyval.ftype.t = TYPE_MAIN_DECL (parse_xref_tag (yyvsp[-1].ttype, 
							 make_anon_name (), 
							 0));
		  yyval.ftype.new_type_flag = 0;
		  CLASSTYPE_DECLARED_CLASS (TREE_TYPE (yyval.ftype.t))
		    = yyvsp[-1].ttype == class_type_node;
		  yyungetc ('{', 1);
		;
    break;}
case 542:
#line 2551 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 543:
#line 2553 "parse.y"
{ error ("no bases given following `:'");
		  yyval.ttype = NULL_TREE; ;
    break;}
case 544:
#line 2556 "parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 546:
#line 2562 "parse.y"
{ yyval.ttype = chainon (yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 547:
#line 2567 "parse.y"
{ yyval.ttype = finish_base_specifier (access_default_node, yyvsp[0].ttype); ;
    break;}
case 548:
#line 2569 "parse.y"
{ yyval.ttype = finish_base_specifier (yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 549:
#line 2574 "parse.y"
{ if (!TYPE_P (yyval.ttype))
		    yyval.ttype = error_mark_node; ;
    break;}
case 550:
#line 2577 "parse.y"
{ yyval.ttype = TREE_TYPE (yyval.ttype); ;
    break;}
case 552:
#line 2583 "parse.y"
{ if (yyvsp[-1].ttype != ridpointers[(int)RID_VIRTUAL])
		    error ("`%D' access", yyvsp[-1].ttype);
		  yyval.ttype = access_default_virtual_node; ;
    break;}
case 553:
#line 2587 "parse.y"
{
		  if (yyvsp[-2].ttype != access_default_virtual_node)
		    error ("multiple access specifiers");
		  else if (yyvsp[-1].ttype == access_public_node)
		    yyval.ttype = access_public_virtual_node;
		  else if (yyvsp[-1].ttype == access_protected_node)
		    yyval.ttype = access_protected_virtual_node;
		  else /* $2 == access_private_node */
		    yyval.ttype = access_private_virtual_node;
		;
    break;}
case 554:
#line 2598 "parse.y"
{ if (yyvsp[-1].ttype != ridpointers[(int)RID_VIRTUAL])
		    error ("`%D' access", yyvsp[-1].ttype);
		  else if (yyval.ttype == access_public_node)
		    yyval.ttype = access_public_virtual_node;
		  else if (yyval.ttype == access_protected_node)
		    yyval.ttype = access_protected_virtual_node;
		  else if (yyval.ttype == access_private_node)
		    yyval.ttype = access_private_virtual_node;
		  else
		    error ("multiple `virtual' specifiers");
		;
    break;}
case 559:
#line 2619 "parse.y"
{
		  current_access_specifier = yyvsp[-1].ttype;
                ;
    break;}
case 560:
#line 2628 "parse.y"
{
		  finish_member_declaration (yyvsp[0].ttype);
		  current_aggr = NULL_TREE;
		  reset_type_access_control ();
		;
    break;}
case 561:
#line 2634 "parse.y"
{
		  finish_member_declaration (yyvsp[0].ttype);
		  current_aggr = NULL_TREE;
		  reset_type_access_control ();
		;
    break;}
case 563:
#line 2644 "parse.y"
{ error ("missing ';' before right brace");
		  yyungetc ('}', 0); ;
    break;}
case 564:
#line 2649 "parse.y"
{ yyval.ttype = finish_method (yyval.ttype); ;
    break;}
case 565:
#line 2651 "parse.y"
{ yyval.ttype = finish_method (yyval.ttype); ;
    break;}
case 566:
#line 2653 "parse.y"
{ yyval.ttype = finish_method (yyval.ttype); ;
    break;}
case 567:
#line 2655 "parse.y"
{ yyval.ttype = finish_method (yyval.ttype); ;
    break;}
case 568:
#line 2657 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 569:
#line 2659 "parse.y"
{ yyval.ttype = yyvsp[0].ttype;
		  pedantic = yyvsp[-1].itype; ;
    break;}
case 570:
#line 2662 "parse.y"
{
		  if (yyvsp[0].ttype)
		    yyval.ttype = finish_member_template_decl (yyvsp[0].ttype);
		  else
		    /* The component was already processed.  */
		    yyval.ttype = NULL_TREE;

		  finish_template_decl (yyvsp[-1].ttype);
		;
    break;}
case 571:
#line 2672 "parse.y"
{
		  yyval.ttype = finish_member_class_template (yyvsp[-1].ftype.t);
		  finish_template_decl (yyvsp[-2].ttype);
		;
    break;}
case 572:
#line 2677 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 573:
#line 2685 "parse.y"
{
		  /* Most of the productions for component_decl only
		     allow the creation of one new member, so we call
		     finish_member_declaration in component_decl_list.
		     For this rule and the next, however, there can be
		     more than one member, e.g.:

		       int i, j;

		     and we need the first member to be fully
		     registered before the second is processed.
		     Therefore, the rules for components take care of
		     this processing.  To avoid registering the
		     components more than once, we send NULL_TREE up
		     here; that lets finish_member_declaration know
		     that there is nothing to do.  */
		  if (!yyvsp[0].itype)
		    grok_x_components (yyvsp[-1].ftype.t);
		  yyval.ttype = NULL_TREE;
		;
    break;}
case 574:
#line 2706 "parse.y"
{
		  if (!yyvsp[0].itype)
		    grok_x_components (yyvsp[-1].ftype.t);
		  yyval.ttype = NULL_TREE;
		;
    break;}
case 575:
#line 2712 "parse.y"
{ yyval.ttype = grokfield (yyval.ttype, NULL_TREE, yyvsp[0].ttype, yyvsp[-2].ttype, yyvsp[-1].ttype); ;
    break;}
case 576:
#line 2714 "parse.y"
{ yyval.ttype = grokfield (yyval.ttype, NULL_TREE, yyvsp[0].ttype, yyvsp[-2].ttype, yyvsp[-1].ttype); ;
    break;}
case 577:
#line 2716 "parse.y"
{ yyval.ttype = grokbitfield (NULL_TREE, NULL_TREE, yyvsp[0].ttype); ;
    break;}
case 578:
#line 2718 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 579:
#line 2729 "parse.y"
{ tree specs, attrs;
		  split_specs_attrs (yyvsp[-4].ftype.t, &specs, &attrs);
		  yyval.ttype = grokfield (yyvsp[-3].ttype, specs, yyvsp[0].ttype, yyvsp[-2].ttype,
				  chainon (yyvsp[-1].ttype, attrs)); ;
    break;}
case 580:
#line 2734 "parse.y"
{ yyval.ttype = grokfield (yyval.ttype, NULL_TREE, yyvsp[0].ttype, yyvsp[-2].ttype, yyvsp[-1].ttype); ;
    break;}
case 581:
#line 2736 "parse.y"
{ yyval.ttype = do_class_using_decl (yyvsp[0].ttype); ;
    break;}
case 582:
#line 2743 "parse.y"
{ yyval.itype = 0; ;
    break;}
case 583:
#line 2745 "parse.y"
{
		  if (PROCESSING_REAL_TEMPLATE_DECL_P ())
		    yyvsp[0].ttype = finish_member_template_decl (yyvsp[0].ttype);
		  finish_member_declaration (yyvsp[0].ttype);
		  yyval.itype = 1;
		;
    break;}
case 584:
#line 2752 "parse.y"
{
		  check_multiple_declarators ();
		  if (PROCESSING_REAL_TEMPLATE_DECL_P ())
		    yyvsp[0].ttype = finish_member_template_decl (yyvsp[0].ttype);
		  finish_member_declaration (yyvsp[0].ttype);
		  yyval.itype = 2;
		;
    break;}
case 585:
#line 2763 "parse.y"
{ yyval.itype = 0; ;
    break;}
case 586:
#line 2765 "parse.y"
{
		  if (PROCESSING_REAL_TEMPLATE_DECL_P ())
		    yyvsp[0].ttype = finish_member_template_decl (yyvsp[0].ttype);
		  finish_member_declaration (yyvsp[0].ttype);
		  yyval.itype = 1;
		;
    break;}
case 587:
#line 2772 "parse.y"
{
		  check_multiple_declarators ();
		  if (PROCESSING_REAL_TEMPLATE_DECL_P ())
		    yyvsp[0].ttype = finish_member_template_decl (yyvsp[0].ttype);
		  finish_member_declaration (yyvsp[0].ttype);
		  yyval.itype = 2;
		;
    break;}
case 592:
#line 2793 "parse.y"
{ yyval.ttype = parse_field0 (yyvsp[-3].ttype, yyvsp[-4].ftype.t, yyvsp[-4].ftype.lookups,
				     yyvsp[-1].ttype, yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 593:
#line 2796 "parse.y"
{ yyval.ttype = parse_bitfield0 (yyvsp[-3].ttype, yyvsp[-4].ftype.t, yyvsp[-4].ftype.lookups,
					yyvsp[0].ttype, yyvsp[-1].ttype); ;
    break;}
case 594:
#line 2802 "parse.y"
{ yyval.ttype = parse_field0 (yyvsp[-3].ttype, yyvsp[-4].ftype.t, yyvsp[-4].ftype.lookups,
				     yyvsp[-1].ttype, yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 595:
#line 2805 "parse.y"
{ yyval.ttype = parse_field0 (yyvsp[-3].ttype, yyvsp[-4].ftype.t, yyvsp[-4].ftype.lookups,
				     yyvsp[-1].ttype, yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 596:
#line 2808 "parse.y"
{ yyval.ttype = parse_bitfield0 (yyvsp[-3].ttype, yyvsp[-4].ftype.t, yyvsp[-4].ftype.lookups,
					yyvsp[0].ttype, yyvsp[-1].ttype); ;
    break;}
case 597:
#line 2811 "parse.y"
{ yyval.ttype = parse_bitfield0 (NULL_TREE, yyvsp[-3].ftype.t,
					yyvsp[-3].ftype.lookups, yyvsp[0].ttype, yyvsp[-1].ttype); ;
    break;}
case 598:
#line 2817 "parse.y"
{ yyval.ttype = parse_field (yyvsp[-3].ttype, yyvsp[-1].ttype, yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 599:
#line 2819 "parse.y"
{ yyval.ttype = parse_bitfield (yyvsp[-3].ttype, yyvsp[0].ttype, yyvsp[-1].ttype); ;
    break;}
case 600:
#line 2824 "parse.y"
{ yyval.ttype = parse_field (yyvsp[-3].ttype, yyvsp[-1].ttype, yyvsp[-2].ttype, yyvsp[0].ttype); ;
    break;}
case 601:
#line 2826 "parse.y"
{ yyval.ttype = parse_bitfield (yyvsp[-3].ttype, yyvsp[0].ttype, yyvsp[-1].ttype); ;
    break;}
case 602:
#line 2828 "parse.y"
{ yyval.ttype = parse_bitfield (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype); ;
    break;}
case 607:
#line 2847 "parse.y"
{ build_enumerator (yyvsp[0].ttype, NULL_TREE, current_enum_type); ;
    break;}
case 608:
#line 2849 "parse.y"
{ build_enumerator (yyvsp[-2].ttype, yyvsp[0].ttype, current_enum_type); ;
    break;}
case 609:
#line 2855 "parse.y"
{ yyval.ftype.t = build_tree_list (yyvsp[-1].ftype.t, yyvsp[0].ttype);
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;
    break;}
case 610:
#line 2858 "parse.y"
{ yyval.ftype.t = build_tree_list (yyvsp[0].ftype.t, NULL_TREE);
		  yyval.ftype.new_type_flag = yyvsp[0].ftype.new_type_flag; ;
    break;}
case 611:
#line 2863 "parse.y"
{
		  if (pedantic)
		    pedwarn ("ISO C++ forbids array dimensions with parenthesized type in new");
		  yyval.ftype.t = build_nt (ARRAY_REF, TREE_VALUE (yyvsp[-4].ftype.t), yyvsp[-1].ttype);
		  yyval.ftype.t = build_tree_list (TREE_PURPOSE (yyvsp[-4].ftype.t), yyval.ftype.t);
		  yyval.ftype.new_type_flag = yyvsp[-4].ftype.new_type_flag;
		;
    break;}
case 612:
#line 2874 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 613:
#line 2876 "parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyval.ttype); ;
    break;}
case 614:
#line 2881 "parse.y"
{ yyval.ftype.t = hash_tree_cons (NULL_TREE, yyvsp[0].ttype, NULL_TREE);
		  yyval.ftype.new_type_flag = 0; ;
    break;}
case 615:
#line 2884 "parse.y"
{ yyval.ftype.t = hash_tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ftype.t);
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;
    break;}
case 616:
#line 2887 "parse.y"
{ yyval.ftype.t = hash_tree_cons (yyvsp[0].ttype, NULL_TREE, NULL_TREE);
		  yyval.ftype.new_type_flag = 0; ;
    break;}
case 617:
#line 2890 "parse.y"
{ yyval.ftype.t = hash_tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ftype.t);
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;
    break;}
case 618:
#line 2900 "parse.y"
{ yyval.ttype = yyvsp[-1].ttype; ;
    break;}
case 619:
#line 2902 "parse.y"
{ yyval.ttype = yyvsp[-1].ttype; ;
    break;}
case 620:
#line 2904 "parse.y"
{ yyval.ttype = empty_parms (); ;
    break;}
case 621:
#line 2906 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 623:
#line 2914 "parse.y"
{
		  /* Provide support for '(' attributes '*' declarator ')'
		     etc */
		  yyval.ttype = tree_cons (yyvsp[-1].ttype, yyvsp[0].ttype, NULL_TREE);
		;
    break;}
case 624:
#line 2924 "parse.y"
{ yyval.ttype = make_pointer_declarator (yyvsp[-1].ftype.t, yyvsp[0].ttype); ;
    break;}
case 625:
#line 2926 "parse.y"
{ yyval.ttype = make_reference_declarator (yyvsp[-1].ftype.t, yyvsp[0].ttype); ;
    break;}
case 626:
#line 2928 "parse.y"
{ yyval.ttype = make_pointer_declarator (NULL_TREE, yyvsp[0].ttype); ;
    break;}
case 627:
#line 2930 "parse.y"
{ yyval.ttype = make_reference_declarator (NULL_TREE, yyvsp[0].ttype); ;
    break;}
case 628:
#line 2932 "parse.y"
{ tree arg = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype);
		  yyval.ttype = build_nt (SCOPE_REF, yyvsp[-2].ttype, arg);
		;
    break;}
case 630:
#line 2940 "parse.y"
{ yyval.ttype = make_call_declarator (yyval.ttype, yyvsp[-2].ttype, yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 631:
#line 2942 "parse.y"
{ yyval.ttype = build_nt (ARRAY_REF, yyval.ttype, yyvsp[-1].ttype); ;
    break;}
case 632:
#line 2944 "parse.y"
{ yyval.ttype = build_nt (ARRAY_REF, yyval.ttype, NULL_TREE); ;
    break;}
case 633:
#line 2946 "parse.y"
{ yyval.ttype = yyvsp[-1].ttype; ;
    break;}
case 634:
#line 2948 "parse.y"
{ push_nested_class (yyvsp[-1].ttype, 3);
		  yyval.ttype = build_nt (SCOPE_REF, yyval.ttype, yyvsp[0].ttype);
		  TREE_COMPLEXITY (yyval.ttype) = current_class_depth; ;
    break;}
case 636:
#line 2956 "parse.y"
{
		  if (TREE_CODE (yyvsp[0].ttype) == IDENTIFIER_NODE)
		    {
		      yyval.ttype = lookup_name (yyvsp[0].ttype, 1);
		      maybe_note_name_used_in_class (yyvsp[0].ttype, yyval.ttype);
		    }
		  else
		    yyval.ttype = yyvsp[0].ttype;
		;
    break;}
case 637:
#line 2966 "parse.y"
{
		  if (TREE_CODE (yyvsp[0].ttype) == IDENTIFIER_NODE)
		    yyval.ttype = IDENTIFIER_GLOBAL_VALUE (yyvsp[0].ttype);
		  else
		    yyval.ttype = yyvsp[0].ttype;
		  got_scope = NULL_TREE;
		;
    break;}
case 640:
#line 2979 "parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 641:
#line 2984 "parse.y"
{ yyval.ttype = get_type_decl (yyvsp[0].ttype); ;
    break;}
case 643:
#line 2993 "parse.y"
{
		  /* Provide support for '(' attributes '*' declarator ')'
		     etc */
		  yyval.ttype = tree_cons (yyvsp[-1].ttype, yyvsp[0].ttype, NULL_TREE);
		;
    break;}
case 644:
#line 3002 "parse.y"
{ yyval.ttype = make_pointer_declarator (yyvsp[-1].ftype.t, yyvsp[0].ttype); ;
    break;}
case 645:
#line 3004 "parse.y"
{ yyval.ttype = make_reference_declarator (yyvsp[-1].ftype.t, yyvsp[0].ttype); ;
    break;}
case 646:
#line 3006 "parse.y"
{ yyval.ttype = make_pointer_declarator (NULL_TREE, yyvsp[0].ttype); ;
    break;}
case 647:
#line 3008 "parse.y"
{ yyval.ttype = make_reference_declarator (NULL_TREE, yyvsp[0].ttype); ;
    break;}
case 648:
#line 3010 "parse.y"
{ tree arg = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype);
		  yyval.ttype = build_nt (SCOPE_REF, yyvsp[-2].ttype, arg);
		;
    break;}
case 650:
#line 3018 "parse.y"
{ yyval.ttype = make_pointer_declarator (yyvsp[-1].ftype.t, yyvsp[0].ttype); ;
    break;}
case 651:
#line 3020 "parse.y"
{ yyval.ttype = make_reference_declarator (yyvsp[-1].ftype.t, yyvsp[0].ttype); ;
    break;}
case 652:
#line 3022 "parse.y"
{ yyval.ttype = make_pointer_declarator (NULL_TREE, yyvsp[0].ttype); ;
    break;}
case 653:
#line 3024 "parse.y"
{ yyval.ttype = make_reference_declarator (NULL_TREE, yyvsp[0].ttype); ;
    break;}
case 654:
#line 3026 "parse.y"
{ tree arg = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype);
		  yyval.ttype = build_nt (SCOPE_REF, yyvsp[-2].ttype, arg);
		;
    break;}
case 656:
#line 3034 "parse.y"
{ yyval.ttype = make_call_declarator (yyval.ttype, yyvsp[-2].ttype, yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 657:
#line 3036 "parse.y"
{ yyval.ttype = yyvsp[-1].ttype; ;
    break;}
case 658:
#line 3038 "parse.y"
{ yyval.ttype = build_nt (ARRAY_REF, yyval.ttype, yyvsp[-1].ttype); ;
    break;}
case 659:
#line 3040 "parse.y"
{ yyval.ttype = build_nt (ARRAY_REF, yyval.ttype, NULL_TREE); ;
    break;}
case 660:
#line 3042 "parse.y"
{ enter_scope_of (yyvsp[0].ttype); ;
    break;}
case 661:
#line 3044 "parse.y"
{ enter_scope_of (yyvsp[0].ttype); yyval.ttype = yyvsp[0].ttype;;
    break;}
case 662:
#line 3046 "parse.y"
{ yyval.ttype = build_nt (SCOPE_REF, global_namespace, yyvsp[0].ttype);
		  enter_scope_of (yyval.ttype);
		;
    break;}
case 663:
#line 3050 "parse.y"
{ got_scope = NULL_TREE;
		  yyval.ttype = build_nt (SCOPE_REF, yyvsp[-1].ttype, yyvsp[0].ttype);
		  enter_scope_of (yyval.ttype);
		;
    break;}
case 664:
#line 3058 "parse.y"
{ got_scope = NULL_TREE;
		  yyval.ttype = build_nt (SCOPE_REF, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 665:
#line 3061 "parse.y"
{ got_scope = NULL_TREE;
 		  yyval.ttype = build_nt (SCOPE_REF, yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 666:
#line 3067 "parse.y"
{ got_scope = NULL_TREE;
		  yyval.ttype = build_nt (SCOPE_REF, yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 667:
#line 3070 "parse.y"
{ got_scope = NULL_TREE;
		  yyval.ttype = build_nt (SCOPE_REF, yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 669:
#line 3077 "parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 670:
#line 3082 "parse.y"
{ yyval.ttype = build_functional_cast (yyvsp[-3].ftype.t, yyvsp[-1].ttype); ;
    break;}
case 671:
#line 3084 "parse.y"
{ yyval.ttype = reparse_decl_as_expr (yyvsp[-3].ftype.t, yyvsp[-1].ttype); ;
    break;}
case 672:
#line 3086 "parse.y"
{ yyval.ttype = reparse_absdcl_as_expr (yyvsp[-1].ftype.t, yyvsp[0].ttype); ;
    break;}
case 677:
#line 3098 "parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 678:
#line 3100 "parse.y"
{ got_scope = yyval.ttype
		    = make_typename_type (yyvsp[-3].ttype, yyvsp[-1].ttype, tf_error | tf_parsing); ;
    break;}
case 679:
#line 3104 "parse.y"
{ got_scope = yyval.ttype
		    = make_typename_type (yyvsp[-2].ttype, yyvsp[-1].ttype, tf_error | tf_parsing); ;
    break;}
case 680:
#line 3107 "parse.y"
{ got_scope = yyval.ttype
		    = make_typename_type (yyvsp[-2].ttype, yyvsp[-1].ttype, tf_error | tf_parsing); ;
    break;}
case 681:
#line 3115 "parse.y"
{
		  if (TREE_CODE (yyvsp[-1].ttype) == IDENTIFIER_NODE)
		    {
		      yyval.ttype = lastiddecl;
		      maybe_note_name_used_in_class (yyvsp[-1].ttype, yyval.ttype);
		    }
		  got_scope = yyval.ttype =
		    complete_type (TYPE_MAIN_VARIANT (TREE_TYPE (yyval.ttype)));
		;
    break;}
case 682:
#line 3125 "parse.y"
{
		  if (TREE_CODE (yyvsp[-1].ttype) == IDENTIFIER_NODE)
		    yyval.ttype = lastiddecl;
		  got_scope = yyval.ttype = TREE_TYPE (yyval.ttype);
		;
    break;}
case 683:
#line 3131 "parse.y"
{
		  if (TREE_CODE (yyval.ttype) == IDENTIFIER_NODE)
		    yyval.ttype = lastiddecl;
		  got_scope = yyval.ttype;
		;
    break;}
case 684:
#line 3137 "parse.y"
{ got_scope = yyval.ttype = complete_type (TREE_TYPE (yyvsp[-1].ttype)); ;
    break;}
case 686:
#line 3143 "parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 687:
#line 3148 "parse.y"
{
		  if (TYPE_P (yyvsp[-1].ttype))
		    yyval.ttype = make_typename_type (yyvsp[-1].ttype, yyvsp[0].ttype, tf_error | tf_parsing);
		  else if (TREE_CODE (yyvsp[0].ttype) == IDENTIFIER_NODE)
		    error ("`%T' is not a class or namespace", yyvsp[0].ttype);
		  else
		    {
		      yyval.ttype = yyvsp[0].ttype;
		      if (TREE_CODE (yyval.ttype) == TYPE_DECL)
			yyval.ttype = TREE_TYPE (yyval.ttype);
		    }
		;
    break;}
case 688:
#line 3161 "parse.y"
{ yyval.ttype = TREE_TYPE (yyvsp[0].ttype); ;
    break;}
case 689:
#line 3163 "parse.y"
{ yyval.ttype = make_typename_type (yyvsp[-1].ttype, yyvsp[0].ttype, tf_error | tf_parsing); ;
    break;}
case 690:
#line 3165 "parse.y"
{ yyval.ttype = make_typename_type (yyvsp[-2].ttype, yyvsp[0].ttype, tf_error | tf_parsing); ;
    break;}
case 691:
#line 3170 "parse.y"
{
		  if (TREE_CODE (yyvsp[0].ttype) == IDENTIFIER_NODE)
		    error ("`%T' is not a class or namespace", yyvsp[0].ttype);
		  else if (TREE_CODE (yyvsp[0].ttype) == TYPE_DECL)
		    yyval.ttype = TREE_TYPE (yyvsp[0].ttype);
		;
    break;}
case 692:
#line 3177 "parse.y"
{
		  if (TYPE_P (yyvsp[-1].ttype))
		    yyval.ttype = make_typename_type (yyvsp[-1].ttype, yyvsp[0].ttype, tf_error | tf_parsing);
		  else if (TREE_CODE (yyvsp[0].ttype) == IDENTIFIER_NODE)
		    error ("`%T' is not a class or namespace", yyvsp[0].ttype);
		  else
		    {
		      yyval.ttype = yyvsp[0].ttype;
		      if (TREE_CODE (yyval.ttype) == TYPE_DECL)
			yyval.ttype = TREE_TYPE (yyval.ttype);
		    }
		;
    break;}
case 693:
#line 3190 "parse.y"
{ got_scope = yyval.ttype
		    = make_typename_type (yyvsp[-2].ttype, yyvsp[-1].ttype, tf_error | tf_parsing); ;
    break;}
case 694:
#line 3193 "parse.y"
{ got_scope = yyval.ttype
		    = make_typename_type (yyvsp[-3].ttype, yyvsp[-1].ttype, tf_error | tf_parsing); ;
    break;}
case 695:
#line 3201 "parse.y"
{
		  if (TREE_CODE (yyvsp[-1].ttype) != TYPE_DECL)
		    yyval.ttype = lastiddecl;

		  /* Retrieve the type for the identifier, which might involve
		     some computation. */
		  got_scope = complete_type (TREE_TYPE (yyval.ttype));

		  if (yyval.ttype == error_mark_node)
		    error ("`%T' is not a class or namespace", yyvsp[-1].ttype);
		;
    break;}
case 696:
#line 3213 "parse.y"
{
		  if (TREE_CODE (yyvsp[-1].ttype) != TYPE_DECL)
		    yyval.ttype = lastiddecl;
		  got_scope = complete_type (TREE_TYPE (yyval.ttype));
		;
    break;}
case 697:
#line 3219 "parse.y"
{ got_scope = yyval.ttype = complete_type (TREE_TYPE (yyval.ttype)); ;
    break;}
case 700:
#line 3223 "parse.y"
{
		  if (TREE_CODE (yyval.ttype) == IDENTIFIER_NODE)
		    yyval.ttype = lastiddecl;
		  got_scope = yyval.ttype;
		;
    break;}
case 701:
#line 3232 "parse.y"
{ yyval.ttype = build_min_nt (TEMPLATE_ID_EXPR, yyvsp[-3].ttype, yyvsp[-1].ttype); ;
    break;}
case 702:
#line 3237 "parse.y"
{
		  if (TREE_CODE (yyvsp[0].ttype) == IDENTIFIER_NODE)
		    yyval.ttype = IDENTIFIER_GLOBAL_VALUE (yyvsp[0].ttype);
		  else
		    yyval.ttype = yyvsp[0].ttype;
		  got_scope = NULL_TREE;
		;
    break;}
case 704:
#line 3246 "parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 705:
#line 3251 "parse.y"
{ got_scope = NULL_TREE; ;
    break;}
case 706:
#line 3253 "parse.y"
{ yyval.ttype = yyvsp[-1].ttype; got_scope = NULL_TREE; ;
    break;}
case 707:
#line 3260 "parse.y"
{ got_scope = void_type_node; ;
    break;}
case 708:
#line 3266 "parse.y"
{ yyval.ttype = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 709:
#line 3268 "parse.y"
{ yyval.ttype = make_pointer_declarator (yyvsp[0].ttype, NULL_TREE); ;
    break;}
case 710:
#line 3270 "parse.y"
{ yyval.ttype = make_reference_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 711:
#line 3272 "parse.y"
{ yyval.ttype = make_reference_declarator (yyvsp[0].ttype, NULL_TREE); ;
    break;}
case 712:
#line 3274 "parse.y"
{ tree arg = make_pointer_declarator (yyvsp[0].ttype, NULL_TREE);
		  yyval.ttype = build_nt (SCOPE_REF, yyvsp[-1].ttype, arg);
		;
    break;}
case 713:
#line 3278 "parse.y"
{ tree arg = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype);
		  yyval.ttype = build_nt (SCOPE_REF, yyvsp[-2].ttype, arg);
		;
    break;}
case 715:
#line 3287 "parse.y"
{ yyval.ttype = build_nt (ARRAY_REF, NULL_TREE, yyvsp[-1].ttype); ;
    break;}
case 716:
#line 3289 "parse.y"
{ yyval.ttype = build_nt (ARRAY_REF, yyval.ttype, yyvsp[-1].ttype); ;
    break;}
case 718:
#line 3295 "parse.y"
{
		  /* Provide support for '(' attributes '*' declarator ')'
		     etc */
		  yyval.ttype = tree_cons (yyvsp[-1].ttype, yyvsp[0].ttype, NULL_TREE);
		;
    break;}
case 719:
#line 3305 "parse.y"
{ yyval.ttype = make_pointer_declarator (yyvsp[-1].ftype.t, yyvsp[0].ttype); ;
    break;}
case 720:
#line 3307 "parse.y"
{ yyval.ttype = make_pointer_declarator (NULL_TREE, yyvsp[0].ttype); ;
    break;}
case 721:
#line 3309 "parse.y"
{ yyval.ttype = make_pointer_declarator (yyvsp[0].ftype.t, NULL_TREE); ;
    break;}
case 722:
#line 3311 "parse.y"
{ yyval.ttype = make_pointer_declarator (NULL_TREE, NULL_TREE); ;
    break;}
case 723:
#line 3313 "parse.y"
{ yyval.ttype = make_reference_declarator (yyvsp[-1].ftype.t, yyvsp[0].ttype); ;
    break;}
case 724:
#line 3315 "parse.y"
{ yyval.ttype = make_reference_declarator (NULL_TREE, yyvsp[0].ttype); ;
    break;}
case 725:
#line 3317 "parse.y"
{ yyval.ttype = make_reference_declarator (yyvsp[0].ftype.t, NULL_TREE); ;
    break;}
case 726:
#line 3319 "parse.y"
{ yyval.ttype = make_reference_declarator (NULL_TREE, NULL_TREE); ;
    break;}
case 727:
#line 3321 "parse.y"
{ tree arg = make_pointer_declarator (yyvsp[0].ttype, NULL_TREE);
		  yyval.ttype = build_nt (SCOPE_REF, yyvsp[-1].ttype, arg);
		;
    break;}
case 728:
#line 3325 "parse.y"
{ tree arg = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype);
		  yyval.ttype = build_nt (SCOPE_REF, yyvsp[-2].ttype, arg);
		;
    break;}
case 730:
#line 3334 "parse.y"
{ yyval.ttype = yyvsp[-1].ttype; ;
    break;}
case 731:
#line 3337 "parse.y"
{ yyval.ttype = make_call_declarator (yyval.ttype, yyvsp[-3].ttype, yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 732:
#line 3339 "parse.y"
{ yyval.ttype = make_call_declarator (yyval.ttype, empty_parms (), yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 733:
#line 3341 "parse.y"
{ yyval.ttype = build_nt (ARRAY_REF, yyval.ttype, yyvsp[-1].ttype); ;
    break;}
case 734:
#line 3343 "parse.y"
{ yyval.ttype = build_nt (ARRAY_REF, yyval.ttype, NULL_TREE); ;
    break;}
case 735:
#line 3345 "parse.y"
{ yyval.ttype = make_call_declarator (NULL_TREE, yyvsp[-3].ttype, yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 736:
#line 3347 "parse.y"
{ set_quals_and_spec (yyval.ttype, yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 737:
#line 3349 "parse.y"
{ set_quals_and_spec (yyval.ttype, yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 738:
#line 3351 "parse.y"
{ yyval.ttype = build_nt (ARRAY_REF, NULL_TREE, yyvsp[-1].ttype); ;
    break;}
case 739:
#line 3353 "parse.y"
{ yyval.ttype = build_nt (ARRAY_REF, NULL_TREE, NULL_TREE); ;
    break;}
case 746:
#line 3376 "parse.y"
{ if (pedantic)
		    pedwarn ("ISO C++ forbids label declarations"); ;
    break;}
case 749:
#line 3387 "parse.y"
{
		  while (yyvsp[-1].ttype)
		    {
		      finish_label_decl (TREE_VALUE (yyvsp[-1].ttype));
		      yyvsp[-1].ttype = TREE_CHAIN (yyvsp[-1].ttype);
		    }
		;
    break;}
case 750:
#line 3398 "parse.y"
{ yyval.ttype = begin_compound_stmt (0); ;
    break;}
case 751:
#line 3400 "parse.y"
{ STMT_LINENO (yyvsp[-1].ttype) = yyvsp[-3].itype;
		  finish_compound_stmt (0, yyvsp[-1].ttype); ;
    break;}
case 752:
#line 3406 "parse.y"
{ last_expr_type = NULL_TREE; ;
    break;}
case 753:
#line 3411 "parse.y"
{ yyval.ttype = begin_if_stmt ();
		  cond_stmt_keyword = "if"; ;
    break;}
case 754:
#line 3414 "parse.y"
{ finish_if_stmt_cond (yyvsp[0].ttype, yyvsp[-1].ttype); ;
    break;}
case 755:
#line 3416 "parse.y"
{ yyval.ttype = yyvsp[-3].ttype;
		  finish_then_clause (yyvsp[-3].ttype); ;
    break;}
case 757:
#line 3423 "parse.y"
{ yyval.ttype = begin_compound_stmt (0); ;
    break;}
case 758:
#line 3425 "parse.y"
{ STMT_LINENO (yyvsp[-2].ttype) = yyvsp[-1].itype;
		  if (yyvsp[0].ttype) STMT_LINENO (yyvsp[0].ttype) = yyvsp[-1].itype;
		  finish_compound_stmt (0, yyvsp[-2].ttype); ;
    break;}
case 760:
#line 3433 "parse.y"
{ if (yyvsp[0].ttype) STMT_LINENO (yyvsp[0].ttype) = yyvsp[-1].itype; ;
    break;}
case 761:
#line 3438 "parse.y"
{ finish_stmt ();
		  yyval.ttype = NULL_TREE; ;
    break;}
case 762:
#line 3441 "parse.y"
{ yyval.ttype = finish_expr_stmt (yyvsp[-1].ttype); ;
    break;}
case 763:
#line 3443 "parse.y"
{ begin_else_clause (); ;
    break;}
case 764:
#line 3445 "parse.y"
{
		  yyval.ttype = yyvsp[-3].ttype;
		  finish_else_clause (yyvsp[-3].ttype);
		  finish_if_stmt ();
		;
    break;}
case 765:
#line 3451 "parse.y"
{ yyval.ttype = yyvsp[0].ttype;
		  finish_if_stmt (); ;
    break;}
case 766:
#line 3454 "parse.y"
{
		  yyval.ttype = begin_while_stmt ();
		  cond_stmt_keyword = "while";
		;
    break;}
case 767:
#line 3459 "parse.y"
{ finish_while_stmt_cond (yyvsp[0].ttype, yyvsp[-1].ttype); ;
    break;}
case 768:
#line 3461 "parse.y"
{ yyval.ttype = yyvsp[-3].ttype;
		  finish_while_stmt (yyvsp[-3].ttype); ;
    break;}
case 769:
#line 3464 "parse.y"
{ yyval.ttype = begin_do_stmt (); ;
    break;}
case 770:
#line 3466 "parse.y"
{
		  finish_do_body (yyvsp[-2].ttype);
		  cond_stmt_keyword = "do";
		;
    break;}
case 771:
#line 3471 "parse.y"
{ yyval.ttype = yyvsp[-5].ttype;
		  finish_do_stmt (yyvsp[-1].ttype, yyvsp[-5].ttype); ;
    break;}
case 772:
#line 3474 "parse.y"
{ yyval.ttype = begin_for_stmt (); ;
    break;}
case 773:
#line 3476 "parse.y"
{ finish_for_init_stmt (yyvsp[-2].ttype); ;
    break;}
case 774:
#line 3478 "parse.y"
{ finish_for_cond (yyvsp[-1].ttype, yyvsp[-5].ttype); ;
    break;}
case 775:
#line 3480 "parse.y"
{ finish_for_expr (yyvsp[-1].ttype, yyvsp[-8].ttype); ;
    break;}
case 776:
#line 3482 "parse.y"
{ yyval.ttype = yyvsp[-10].ttype;
		  finish_for_stmt (yyvsp[-10].ttype); ;
    break;}
case 777:
#line 3485 "parse.y"
{ yyval.ttype = begin_switch_stmt (); ;
    break;}
case 778:
#line 3487 "parse.y"
{ finish_switch_cond (yyvsp[-1].ttype, yyvsp[-3].ttype); ;
    break;}
case 779:
#line 3489 "parse.y"
{ yyval.ttype = yyvsp[-5].ttype;
		  finish_switch_stmt (yyvsp[-5].ttype); ;
    break;}
case 780:
#line 3492 "parse.y"
{ yyval.ttype = finish_case_label (yyvsp[-1].ttype, NULL_TREE); ;
    break;}
case 781:
#line 3494 "parse.y"
{ yyval.ttype = yyvsp[-1].ttype; ;
    break;}
case 782:
#line 3496 "parse.y"
{ yyval.ttype = finish_case_label (yyvsp[-3].ttype, yyvsp[-1].ttype); ;
    break;}
case 783:
#line 3498 "parse.y"
{ yyval.ttype = yyvsp[-1].ttype; ;
    break;}
case 784:
#line 3500 "parse.y"
{ yyval.ttype = finish_case_label (NULL_TREE, NULL_TREE); ;
    break;}
case 785:
#line 3502 "parse.y"
{ yyval.ttype = yyvsp[-1].ttype; ;
    break;}
case 786:
#line 3504 "parse.y"
{ yyval.ttype = finish_break_stmt (); ;
    break;}
case 787:
#line 3506 "parse.y"
{ yyval.ttype = finish_continue_stmt (); ;
    break;}
case 788:
#line 3508 "parse.y"
{ yyval.ttype = finish_return_stmt (NULL_TREE); ;
    break;}
case 789:
#line 3510 "parse.y"
{ yyval.ttype = finish_return_stmt (yyvsp[-1].ttype); ;
    break;}
case 790:
#line 3512 "parse.y"
{ yyval.ttype = finish_asm_stmt (yyvsp[-4].ttype, yyvsp[-2].ttype, NULL_TREE, NULL_TREE,
					NULL_TREE);
		  ASM_INPUT_P (yyval.ttype) = 1; ;
    break;}
case 791:
#line 3517 "parse.y"
{ yyval.ttype = finish_asm_stmt (yyvsp[-6].ttype, yyvsp[-4].ttype, yyvsp[-2].ttype, NULL_TREE, NULL_TREE); ;
    break;}
case 792:
#line 3521 "parse.y"
{ yyval.ttype = finish_asm_stmt (yyvsp[-8].ttype, yyvsp[-6].ttype, yyvsp[-4].ttype, yyvsp[-2].ttype, NULL_TREE); ;
    break;}
case 793:
#line 3523 "parse.y"
{ yyval.ttype = finish_asm_stmt (yyvsp[-6].ttype, yyvsp[-4].ttype, NULL_TREE, yyvsp[-2].ttype, NULL_TREE); ;
    break;}
case 794:
#line 3527 "parse.y"
{ yyval.ttype = finish_asm_stmt (yyvsp[-10].ttype, yyvsp[-8].ttype, yyvsp[-6].ttype, yyvsp[-4].ttype, yyvsp[-2].ttype); ;
    break;}
case 795:
#line 3530 "parse.y"
{ yyval.ttype = finish_asm_stmt (yyvsp[-8].ttype, yyvsp[-6].ttype, NULL_TREE, yyvsp[-4].ttype, yyvsp[-2].ttype); ;
    break;}
case 796:
#line 3533 "parse.y"
{ yyval.ttype = finish_asm_stmt (yyvsp[-8].ttype, yyvsp[-6].ttype, yyvsp[-4].ttype, NULL_TREE, yyvsp[-2].ttype); ;
    break;}
case 797:
#line 3535 "parse.y"
{
		  if (pedantic)
		    pedwarn ("ISO C++ forbids computed gotos");
		  yyval.ttype = finish_goto_stmt (yyvsp[-1].ttype);
		;
    break;}
case 798:
#line 3541 "parse.y"
{ yyval.ttype = finish_goto_stmt (yyvsp[-1].ttype); ;
    break;}
case 799:
#line 3543 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 800:
#line 3545 "parse.y"
{ error ("label must be followed by statement");
		  yyungetc ('}', 0);
		  yyval.ttype = NULL_TREE; ;
    break;}
case 801:
#line 3549 "parse.y"
{ finish_stmt ();
		  yyval.ttype = NULL_TREE; ;
    break;}
case 802:
#line 3552 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 803:
#line 3554 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 804:
#line 3556 "parse.y"
{ do_local_using_decl (yyvsp[0].ttype);
		  yyval.ttype = NULL_TREE; ;
    break;}
case 805:
#line 3559 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 806:
#line 3564 "parse.y"
{ yyval.ttype = begin_function_try_block (); ;
    break;}
case 807:
#line 3566 "parse.y"
{ finish_function_try_block (yyvsp[-1].ttype); ;
    break;}
case 808:
#line 3568 "parse.y"
{ finish_function_handler_sequence (yyvsp[-3].ttype); ;
    break;}
case 809:
#line 3573 "parse.y"
{ yyval.ttype = begin_try_block (); ;
    break;}
case 810:
#line 3575 "parse.y"
{ finish_try_block (yyvsp[-1].ttype); ;
    break;}
case 811:
#line 3577 "parse.y"
{ finish_handler_sequence (yyvsp[-3].ttype); ;
    break;}
case 814:
#line 3584 "parse.y"
{ /* Generate a fake handler block to avoid later aborts. */
		  tree fake_handler = begin_handler ();
		  finish_handler_parms (NULL_TREE, fake_handler);
		  finish_handler (fake_handler);
		  yyval.ttype = fake_handler;

		  error ("must have at least one catch per try block");
		;
    break;}
case 815:
#line 3596 "parse.y"
{ yyval.ttype = begin_handler (); ;
    break;}
case 816:
#line 3598 "parse.y"
{ finish_handler_parms (yyvsp[0].ttype, yyvsp[-1].ttype); ;
    break;}
case 817:
#line 3600 "parse.y"
{ finish_handler (yyvsp[-3].ttype); ;
    break;}
case 820:
#line 3610 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 821:
#line 3626 "parse.y"
{
		  check_for_new_type ("inside exception declarations", yyvsp[-1].ftype);
		  yyval.ttype = start_handler_parms (TREE_PURPOSE (yyvsp[-1].ftype.t),
					    TREE_VALUE (yyvsp[-1].ftype.t));
		;
    break;}
case 822:
#line 3635 "parse.y"
{ finish_label_stmt (yyvsp[-1].ttype); ;
    break;}
case 823:
#line 3637 "parse.y"
{ finish_label_stmt (yyvsp[-1].ttype); ;
    break;}
case 824:
#line 3639 "parse.y"
{ finish_label_stmt (yyvsp[-1].ttype); ;
    break;}
case 825:
#line 3641 "parse.y"
{ finish_label_stmt (yyvsp[-1].ttype); ;
    break;}
case 826:
#line 3646 "parse.y"
{ finish_expr_stmt (yyvsp[-1].ttype); ;
    break;}
case 828:
#line 3649 "parse.y"
{ if (pedantic)
		    pedwarn ("ISO C++ forbids compound statements inside for initializations");
		;
    break;}
case 829:
#line 3658 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 831:
#line 3664 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 833:
#line 3667 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 834:
#line 3674 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 837:
#line 3681 "parse.y"
{ yyval.ttype = chainon (yyval.ttype, yyvsp[0].ttype); ;
    break;}
case 838:
#line 3686 "parse.y"
{ yyval.ttype = build_tree_list (build_tree_list (NULL_TREE, yyvsp[-3].ttype), yyvsp[-1].ttype); ;
    break;}
case 839:
#line 3688 "parse.y"
{ yyvsp[-5].ttype = build_string (IDENTIFIER_LENGTH (yyvsp[-5].ttype),
				     IDENTIFIER_POINTER (yyvsp[-5].ttype));
		  yyval.ttype = build_tree_list (build_tree_list (yyvsp[-5].ttype, yyvsp[-3].ttype), yyvsp[-1].ttype); ;
    break;}
case 840:
#line 3695 "parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, NULL_TREE);;
    break;}
case 841:
#line 3697 "parse.y"
{ yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-2].ttype); ;
    break;}
case 842:
#line 3708 "parse.y"
{
		  yyval.ttype = empty_parms();
		;
    break;}
case 844:
#line 3713 "parse.y"
{ yyval.ttype = finish_parmlist (build_tree_list (NULL_TREE, yyvsp[0].ftype.t), 0);
		  check_for_new_type ("inside parameter list", yyvsp[0].ftype); ;
    break;}
case 845:
#line 3721 "parse.y"
{ yyval.ttype = finish_parmlist (yyval.ttype, 0); ;
    break;}
case 846:
#line 3723 "parse.y"
{ yyval.ttype = finish_parmlist (yyvsp[-1].ttype, 1); ;
    break;}
case 847:
#line 3726 "parse.y"
{ yyval.ttype = finish_parmlist (yyvsp[-1].ttype, 1); ;
    break;}
case 848:
#line 3728 "parse.y"
{ yyval.ttype = finish_parmlist (build_tree_list (NULL_TREE,
							 yyvsp[-1].ftype.t), 1); ;
    break;}
case 849:
#line 3731 "parse.y"
{ yyval.ttype = finish_parmlist (NULL_TREE, 1); ;
    break;}
case 850:
#line 3733 "parse.y"
{
		  /* This helps us recover from really nasty
		     parse errors, for example, a missing right
		     parenthesis.  */
		  yyerror ("possibly missing ')'");
		  yyval.ttype = finish_parmlist (yyvsp[-1].ttype, 0);
		  yyungetc (':', 0);
		  yychar = ')';
		;
    break;}
case 851:
#line 3743 "parse.y"
{
		  /* This helps us recover from really nasty
		     parse errors, for example, a missing right
		     parenthesis.  */
		  yyerror ("possibly missing ')'");
		  yyval.ttype = finish_parmlist (build_tree_list (NULL_TREE,
							 yyvsp[-1].ftype.t), 0);
		  yyungetc (':', 0);
		  yychar = ')';
		;
    break;}
case 852:
#line 3758 "parse.y"
{ maybe_snarf_defarg (); ;
    break;}
case 853:
#line 3760 "parse.y"
{ yyval.ttype = yyvsp[0].ttype; ;
    break;}
case 856:
#line 3771 "parse.y"
{ check_for_new_type ("in a parameter list", yyvsp[0].ftype);
		  yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ftype.t); ;
    break;}
case 857:
#line 3774 "parse.y"
{ check_for_new_type ("in a parameter list", yyvsp[-1].ftype);
		  yyval.ttype = build_tree_list (yyvsp[0].ttype, yyvsp[-1].ftype.t); ;
    break;}
case 858:
#line 3777 "parse.y"
{ check_for_new_type ("in a parameter list", yyvsp[0].ftype);
		  yyval.ttype = chainon (yyval.ttype, yyvsp[0].ftype.t); ;
    break;}
case 859:
#line 3780 "parse.y"
{ yyval.ttype = chainon (yyval.ttype, build_tree_list (NULL_TREE, yyvsp[0].ttype)); ;
    break;}
case 860:
#line 3782 "parse.y"
{ yyval.ttype = chainon (yyval.ttype, build_tree_list (yyvsp[0].ttype, yyvsp[-2].ttype)); ;
    break;}
case 862:
#line 3788 "parse.y"
{ check_for_new_type ("in a parameter list", yyvsp[-1].ftype);
		  yyval.ttype = build_tree_list (NULL_TREE, yyvsp[-1].ftype.t); ;
    break;}
case 863:
#line 3798 "parse.y"
{ yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag;
		  yyval.ftype.t = build_tree_list (yyvsp[-1].ftype.t, yyvsp[0].ttype); ;
    break;}
case 864:
#line 3801 "parse.y"
{ yyval.ftype.t = build_tree_list (yyvsp[-1].ftype.t, yyvsp[0].ttype);
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;
    break;}
case 865:
#line 3804 "parse.y"
{ yyval.ftype.t = build_tree_list (build_tree_list (NULL_TREE, yyvsp[-1].ftype.t),
					  yyvsp[0].ttype);
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;
    break;}
case 866:
#line 3808 "parse.y"
{ yyval.ftype.t = build_tree_list (yyvsp[-1].ftype.t, yyvsp[0].ttype);
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;
    break;}
case 867:
#line 3811 "parse.y"
{ yyval.ftype.t = build_tree_list (yyvsp[0].ftype.t, NULL_TREE);
		  yyval.ftype.new_type_flag = yyvsp[0].ftype.new_type_flag; ;
    break;}
case 868:
#line 3814 "parse.y"
{ yyval.ftype.t = build_tree_list (yyvsp[-1].ftype.t, yyvsp[0].ttype);
		  yyval.ftype.new_type_flag = 0; ;
    break;}
case 869:
#line 3820 "parse.y"
{ yyval.ftype.t = build_tree_list (NULL_TREE, yyvsp[0].ftype.t);
		  yyval.ftype.new_type_flag = yyvsp[0].ftype.new_type_flag;  ;
    break;}
case 870:
#line 3823 "parse.y"
{ yyval.ftype.t = build_tree_list (yyvsp[0].ttype, yyvsp[-1].ftype.t);
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag;  ;
    break;}
case 873:
#line 3834 "parse.y"
{ see_typename (); ;
    break;}
case 874:
#line 3839 "parse.y"
{
		  error ("type specifier omitted for parameter");
		  yyval.ttype = build_tree_list (integer_type_node, NULL_TREE);
		;
    break;}
case 875:
#line 3844 "parse.y"
{
		  if (TREE_CODE (yyval.ttype) == SCOPE_REF)
		    {
		      if (TREE_CODE (TREE_OPERAND (yyval.ttype, 0)) == TEMPLATE_TYPE_PARM
			  || TREE_CODE (TREE_OPERAND (yyval.ttype, 0)) == BOUND_TEMPLATE_TEMPLATE_PARM)
			error ("`%E' is not a type, use `typename %E' to make it one", yyval.ttype, yyval.ttype);
		      else
			error ("no type `%D' in `%T'", TREE_OPERAND (yyval.ttype, 1), TREE_OPERAND (yyval.ttype, 0));
		    }
		  else
		    error ("type specifier omitted for parameter `%E'", yyval.ttype);
		  yyval.ttype = build_tree_list (integer_type_node, yyval.ttype);
		;
    break;}
case 876:
#line 3861 "parse.y"
{
                  error("'%D' is used as a type, but is not defined as a type.", yyvsp[-4].ttype);
                  yyvsp[-2].ttype = error_mark_node;
		;
    break;}
case 877:
#line 3869 "parse.y"
{ ;
    break;}
case 879:
#line 3875 "parse.y"
{ ;
    break;}
case 881:
#line 3881 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 882:
#line 3883 "parse.y"
{ yyval.ttype = yyvsp[-1].ttype; ;
    break;}
case 883:
#line 3885 "parse.y"
{ yyval.ttype = empty_except_spec; ;
    break;}
case 884:
#line 3890 "parse.y"
{
		  check_for_new_type ("exception specifier", yyvsp[0].ftype);
		  yyval.ttype = groktypename (yyvsp[0].ftype.t);
		;
    break;}
case 885:
#line 3895 "parse.y"
{ yyval.ttype = error_mark_node; ;
    break;}
case 886:
#line 3900 "parse.y"
{ yyval.ttype = add_exception_specifier (NULL_TREE, yyvsp[0].ttype, 1); ;
    break;}
case 887:
#line 3902 "parse.y"
{ yyval.ttype = add_exception_specifier (yyvsp[-2].ttype, yyvsp[0].ttype, 1); ;
    break;}
case 888:
#line 3907 "parse.y"
{ yyval.ttype = NULL_TREE; ;
    break;}
case 889:
#line 3909 "parse.y"
{ yyval.ttype = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 890:
#line 3911 "parse.y"
{ yyval.ttype = make_reference_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;
    break;}
case 891:
#line 3913 "parse.y"
{ tree arg = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype);
		  yyval.ttype = build_nt (SCOPE_REF, yyvsp[-2].ttype, arg);
		;
    break;}
case 892:
#line 3920 "parse.y"
{
	  saved_scopes = tree_cons (got_scope, got_object, saved_scopes);
	  TREE_LANG_FLAG_0 (saved_scopes) = looking_for_typename;
	  /* We look for conversion-type-id's in both the class and current
	     scopes, just as for ID in 'ptr->ID::'.  */
	  looking_for_typename = 1;
	  got_object = got_scope;
          got_scope = NULL_TREE;
	;
    break;}
case 893:
#line 3932 "parse.y"
{ got_scope = TREE_PURPOSE (saved_scopes);
          got_object = TREE_VALUE (saved_scopes);
	  looking_for_typename = TREE_LANG_FLAG_0 (saved_scopes);
          saved_scopes = TREE_CHAIN (saved_scopes);
	  yyval.ttype = got_scope;
	;
    break;}
case 894:
#line 3942 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (MULT_EXPR)); ;
    break;}
case 895:
#line 3944 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (TRUNC_DIV_EXPR)); ;
    break;}
case 896:
#line 3946 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (TRUNC_MOD_EXPR)); ;
    break;}
case 897:
#line 3948 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (PLUS_EXPR)); ;
    break;}
case 898:
#line 3950 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (MINUS_EXPR)); ;
    break;}
case 899:
#line 3952 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (BIT_AND_EXPR)); ;
    break;}
case 900:
#line 3954 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (BIT_IOR_EXPR)); ;
    break;}
case 901:
#line 3956 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (BIT_XOR_EXPR)); ;
    break;}
case 902:
#line 3958 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (BIT_NOT_EXPR)); ;
    break;}
case 903:
#line 3960 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (COMPOUND_EXPR)); ;
    break;}
case 904:
#line 3962 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (yyvsp[-1].code)); ;
    break;}
case 905:
#line 3964 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (LT_EXPR)); ;
    break;}
case 906:
#line 3966 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (GT_EXPR)); ;
    break;}
case 907:
#line 3968 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (yyvsp[-1].code)); ;
    break;}
case 908:
#line 3970 "parse.y"
{ yyval.ttype = frob_opname (ansi_assopname (yyvsp[-1].code)); ;
    break;}
case 909:
#line 3972 "parse.y"
{ yyval.ttype = frob_opname (ansi_assopname (NOP_EXPR)); ;
    break;}
case 910:
#line 3974 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (yyvsp[-1].code)); ;
    break;}
case 911:
#line 3976 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (yyvsp[-1].code)); ;
    break;}
case 912:
#line 3978 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (POSTINCREMENT_EXPR)); ;
    break;}
case 913:
#line 3980 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (PREDECREMENT_EXPR)); ;
    break;}
case 914:
#line 3982 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (TRUTH_ANDIF_EXPR)); ;
    break;}
case 915:
#line 3984 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (TRUTH_ORIF_EXPR)); ;
    break;}
case 916:
#line 3986 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (TRUTH_NOT_EXPR)); ;
    break;}
case 917:
#line 3988 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (COND_EXPR)); ;
    break;}
case 918:
#line 3990 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (yyvsp[-1].code)); ;
    break;}
case 919:
#line 3992 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (COMPONENT_REF)); ;
    break;}
case 920:
#line 3994 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (MEMBER_REF)); ;
    break;}
case 921:
#line 3996 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (CALL_EXPR)); ;
    break;}
case 922:
#line 3998 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (ARRAY_REF)); ;
    break;}
case 923:
#line 4000 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (NEW_EXPR)); ;
    break;}
case 924:
#line 4002 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (DELETE_EXPR)); ;
    break;}
case 925:
#line 4004 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (VEC_NEW_EXPR)); ;
    break;}
case 926:
#line 4006 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (VEC_DELETE_EXPR)); ;
    break;}
case 927:
#line 4008 "parse.y"
{ yyval.ttype = frob_opname (grokoptypename (yyvsp[-2].ftype.t, yyvsp[-1].ttype, yyvsp[0].ttype)); ;
    break;}
case 928:
#line 4010 "parse.y"
{ yyval.ttype = frob_opname (ansi_opname (ERROR_MARK)); ;
    break;}
case 929:
#line 4017 "parse.y"
{ if (yychar == YYEMPTY)
		    yychar = YYLEX;
		  yyval.itype = lineno; ;
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
#line 4021 "parse.y"


#ifdef SPEW_DEBUG
const char *
debug_yytranslate (value)
    int value;
{
  return yytname[YYTRANSLATE (value)];
}
#endif

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

/* Return the value corresponding to TOKEN in the global scope.  */

static tree
parse_scoped_id (token)
     tree token;
{
  cxx_binding binding;
 
  cxx_binding_clear (&binding);
  if (!qualified_lookup_using_namespace (token, global_namespace, &binding, 0))
    binding.value = NULL;
  if (yychar == YYEMPTY)
    yychar = yylex();

  return do_scoped_id (token, binding.value);
}

/* AGGR may be either a type node (like class_type_node) or a
   TREE_LIST whose TREE_PURPOSE is a list of attributes and whose
   TREE_VALUE is a type node.  Set *TAG_KIND and *ATTRIBUTES to
   represent the information encoded.  */

static void
parse_split_aggr (tree aggr, enum tag_types *tag_kind, tree *attributes)
{
  if (TREE_CODE (aggr) == TREE_LIST) 
    {
      *attributes = TREE_PURPOSE (aggr);
      aggr = TREE_VALUE (aggr);
    }
  else
    *attributes = NULL_TREE;
  *tag_kind = (enum tag_types) tree_low_cst (aggr, 1);
}

/* Like xref_tag, except that the AGGR may be either a type node (like
   class_type_node) or a TREE_LIST whose TREE_PURPOSE is a list of
   attributes and whose TREE_VALUE is a type node.  */

static tree
parse_xref_tag (tree aggr, tree name, int globalize)
{
  tree attributes;
  enum tag_types tag_kind;
  parse_split_aggr (aggr, &tag_kind, &attributes);
  return xref_tag (tag_kind, name, attributes, globalize);
}

/* Like handle_class_head, but AGGR may be as for parse_xref_tag.  */

static tree
parse_handle_class_head (tree aggr, tree scope, tree id, 
			 int defn_p, int *new_type_p)
{
  tree attributes;
  enum tag_types tag_kind;
  parse_split_aggr (aggr, &tag_kind, &attributes);
  return handle_class_head (tag_kind, scope, id, attributes, 
			    defn_p, new_type_p);
}

/* Like do_decl_instantiation, but the declarator has not yet been
   parsed.  */

static void
parse_decl_instantiation (tree declspecs, tree declarator, tree storage)
{
  tree decl = grokdeclarator (declarator, declspecs, NORMAL, 0, NULL);
  do_decl_instantiation (decl, storage);
}

/* Like begin_function_definition, but SPECS_ATTRS is a combined list
   containing both a decl-specifier-seq and attributes.  */

static int
parse_begin_function_definition (tree specs_attrs, tree declarator)
{
  tree specs;
  tree attrs;
  
  split_specs_attrs (specs_attrs, &specs, &attrs);
  return begin_function_definition (specs, attrs, declarator);
}

/* Like finish_call_expr, but the name for FN has not yet been
   resolved.  */

static tree
parse_finish_call_expr (tree fn, tree args, int koenig)
{
  bool disallow_virtual;

  if (TREE_CODE (fn) == OFFSET_REF)
    return build_offset_ref_call_from_tree (fn, args);

  if (TREE_CODE (fn) == SCOPE_REF)
    {
      tree scope = TREE_OPERAND (fn, 0);
      tree name = TREE_OPERAND (fn, 1);

      if (scope == error_mark_node || name == error_mark_node)
	return error_mark_node;
      if (!processing_template_decl)
	fn = resolve_scoped_fn_name (scope, name);
      disallow_virtual = true;
    }
  else
    disallow_virtual = false;

  if (koenig && TREE_CODE (fn) == IDENTIFIER_NODE)
    {
      tree f;
      
      /* Do the Koenig lookup.  */
      fn = do_identifier (fn, 2, args);
      /* If name lookup didn't find any matching declarations, we've
	 got an unbound identifier.  */
      if (TREE_CODE (fn) == IDENTIFIER_NODE)
	{
	  /* For some reason, do_identifier does not resolve
	     conversion operator names if the only matches would be
	     template conversion operators.  So, we do it here.  */
	  if (IDENTIFIER_TYPENAME_P (fn) && current_class_type)
	    {
	      f = lookup_member (current_class_type, fn,
				 /*protect=*/1, /*want_type=*/0);
	      if (f)
		return finish_call_expr (f, args,
					 /*disallow_virtual=*/false);
	    }
	  /* If the name still could not be resolved, then the program
	     is ill-formed.  */
	  if (TREE_CODE (fn) == IDENTIFIER_NODE)
	    {
	      unqualified_name_lookup_error (fn);
	      return error_mark_node;
	    }
	}
      else if (TREE_CODE (fn) == FUNCTION_DECL
	       || DECL_FUNCTION_TEMPLATE_P (fn)
	       || TREE_CODE (fn) == OVERLOAD)
	{
	  tree scope = DECL_CONTEXT (get_first_fn (fn));
	  if (scope && TYPE_P (scope))
	    {
	      tree access_scope;

	      if (DERIVED_FROM_P (scope, current_class_type)
		  && current_class_ref)
		{
		  fn = build_baselink (lookup_base (current_class_type,
						    scope,
						    ba_any,
						    NULL),
				       TYPE_BINFO (current_class_type),
				       fn,
				       /*optype=*/NULL_TREE);
		  return finish_object_call_expr (fn,
						  current_class_ref,
						  args);
		}


	      access_scope = current_class_type;
	      while (!DERIVED_FROM_P (scope, access_scope))
		{
		  access_scope = TYPE_CONTEXT (access_scope);
		  while (DECL_P (access_scope))
		    access_scope = DECL_CONTEXT (access_scope);
		}
	      
	      fn = build_baselink (NULL_TREE,
				   TYPE_BINFO (access_scope),
				   fn,
				   /*optype=*/NULL_TREE);
	    }
	}
    }

  if (TREE_CODE (fn) == COMPONENT_REF)
    /* If the parser sees `(x->y)(bar)' we get here because the
       parentheses confuse the parser.  Treat this like 
       `x->y(bar)'.  */
    return finish_object_call_expr (TREE_OPERAND (fn, 1),
				    TREE_OPERAND (fn, 0),
				    args);

  if (processing_template_decl)
    return build_nt (CALL_EXPR, fn, args, NULL_TREE);

  return build_call_from_tree (fn, args, disallow_virtual);
}

#include "gt-cp-parse.h"
