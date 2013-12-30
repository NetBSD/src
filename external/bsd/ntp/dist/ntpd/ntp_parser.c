/* A Bison parser, made by GNU Bison 2.6.2.  */

/* Bison implementation for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2012 Free Software Foundation, Inc.
   
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
#define YYBISON_VERSION "2.6.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
/* Line 336 of yacc.c  */
#line 13 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"

  #ifdef HAVE_CONFIG_H
  # include <config.h>
  #endif

  #include "ntp.h"
  #include "ntpd.h"
  #include "ntp_machine.h"
  #include "ntp_stdlib.h"
  #include "ntp_filegen.h"
  #include "ntp_scanner.h"
  #include "ntp_config.h"
  #include "ntp_crypto.h"

  #include "ntpsim.h"		/* HMS: Do we really want this all the time? */
				/* SK: It might be a good idea to always
				   include the simulator code. That way
				   someone can use the same configuration file
				   for both the simulator and the daemon
				*/


  struct FILE_INFO *ip_file;	/* configuration file stream */

  #define YYMALLOC	emalloc
  #define YYFREE	free
  #define YYERROR_VERBOSE
  #define YYMAXDEPTH	1000	/* stop the madness sooner */
  void yyerror(const char *msg);
  extern int input_from_file;	/* else from ntpq :config */

/* Line 336 of yacc.c  */
#line 100 "ntp_parser.c"

# ifndef YY_NULL
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULL nullptr
#  else
#   define YY_NULL 0
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
   by #include "ntp_parser.h".  */
#ifndef YY_NTP_PARSER_H
# define YY_NTP_PARSER_H
/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     T_Abbrev = 258,
     T_Age = 259,
     T_All = 260,
     T_Allan = 261,
     T_Allpeers = 262,
     T_Auth = 263,
     T_Autokey = 264,
     T_Automax = 265,
     T_Average = 266,
     T_Bclient = 267,
     T_Beacon = 268,
     T_Broadcast = 269,
     T_Broadcastclient = 270,
     T_Broadcastdelay = 271,
     T_Burst = 272,
     T_Calibrate = 273,
     T_Ceiling = 274,
     T_Clockstats = 275,
     T_Cohort = 276,
     T_ControlKey = 277,
     T_Crypto = 278,
     T_Cryptostats = 279,
     T_Ctl = 280,
     T_Day = 281,
     T_Default = 282,
     T_Digest = 283,
     T_Disable = 284,
     T_Discard = 285,
     T_Dispersion = 286,
     T_Double = 287,
     T_Driftfile = 288,
     T_Drop = 289,
     T_Ellipsis = 290,
     T_Enable = 291,
     T_End = 292,
     T_False = 293,
     T_File = 294,
     T_Filegen = 295,
     T_Filenum = 296,
     T_Flag1 = 297,
     T_Flag2 = 298,
     T_Flag3 = 299,
     T_Flag4 = 300,
     T_Flake = 301,
     T_Floor = 302,
     T_Freq = 303,
     T_Fudge = 304,
     T_Host = 305,
     T_Huffpuff = 306,
     T_Iburst = 307,
     T_Ident = 308,
     T_Ignore = 309,
     T_Incalloc = 310,
     T_Incmem = 311,
     T_Initalloc = 312,
     T_Initmem = 313,
     T_Includefile = 314,
     T_Integer = 315,
     T_Interface = 316,
     T_Intrange = 317,
     T_Io = 318,
     T_Ipv4 = 319,
     T_Ipv4_flag = 320,
     T_Ipv6 = 321,
     T_Ipv6_flag = 322,
     T_Kernel = 323,
     T_Key = 324,
     T_Keys = 325,
     T_Keysdir = 326,
     T_Kod = 327,
     T_Mssntp = 328,
     T_Leapfile = 329,
     T_Limited = 330,
     T_Link = 331,
     T_Listen = 332,
     T_Logconfig = 333,
     T_Logfile = 334,
     T_Loopstats = 335,
     T_Lowpriotrap = 336,
     T_Manycastclient = 337,
     T_Manycastserver = 338,
     T_Mask = 339,
     T_Maxage = 340,
     T_Maxclock = 341,
     T_Maxdepth = 342,
     T_Maxdist = 343,
     T_Maxmem = 344,
     T_Maxpoll = 345,
     T_Mdnstries = 346,
     T_Mem = 347,
     T_Memlock = 348,
     T_Minclock = 349,
     T_Mindepth = 350,
     T_Mindist = 351,
     T_Minimum = 352,
     T_Minpoll = 353,
     T_Minsane = 354,
     T_Mode = 355,
     T_Mode7 = 356,
     T_Monitor = 357,
     T_Month = 358,
     T_Mru = 359,
     T_Multicastclient = 360,
     T_Nic = 361,
     T_Nolink = 362,
     T_Nomodify = 363,
     T_Nomrulist = 364,
     T_None = 365,
     T_Nonvolatile = 366,
     T_Nopeer = 367,
     T_Noquery = 368,
     T_Noselect = 369,
     T_Noserve = 370,
     T_Notrap = 371,
     T_Notrust = 372,
     T_Ntp = 373,
     T_Ntpport = 374,
     T_NtpSignDsocket = 375,
     T_Orphan = 376,
     T_Orphanwait = 377,
     T_Panic = 378,
     T_Peer = 379,
     T_Peerstats = 380,
     T_Phone = 381,
     T_Pid = 382,
     T_Pidfile = 383,
     T_Pool = 384,
     T_Port = 385,
     T_Preempt = 386,
     T_Prefer = 387,
     T_Protostats = 388,
     T_Pw = 389,
     T_Randfile = 390,
     T_Rawstats = 391,
     T_Refid = 392,
     T_Requestkey = 393,
     T_Reset = 394,
     T_Restrict = 395,
     T_Revoke = 396,
     T_Rlimit = 397,
     T_Saveconfigdir = 398,
     T_Server = 399,
     T_Setvar = 400,
     T_Source = 401,
     T_Stacksize = 402,
     T_Statistics = 403,
     T_Stats = 404,
     T_Statsdir = 405,
     T_Step = 406,
     T_Stepout = 407,
     T_Stratum = 408,
     T_String = 409,
     T_Sys = 410,
     T_Sysstats = 411,
     T_Tick = 412,
     T_Time1 = 413,
     T_Time2 = 414,
     T_Timer = 415,
     T_Timingstats = 416,
     T_Tinker = 417,
     T_Tos = 418,
     T_Trap = 419,
     T_True = 420,
     T_Trustedkey = 421,
     T_Ttl = 422,
     T_Type = 423,
     T_U_int = 424,
     T_Unconfig = 425,
     T_Unpeer = 426,
     T_Version = 427,
     T_WanderThreshold = 428,
     T_Week = 429,
     T_Wildcard = 430,
     T_Xleave = 431,
     T_Year = 432,
     T_Flag = 433,
     T_EOC = 434,
     T_Simulate = 435,
     T_Beep_Delay = 436,
     T_Sim_Duration = 437,
     T_Server_Offset = 438,
     T_Duration = 439,
     T_Freq_Offset = 440,
     T_Wander = 441,
     T_Jitter = 442,
     T_Prop_Delay = 443,
     T_Proc_Delay = 444
   };
#endif


#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 350 of yacc.c  */
#line 51 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"

	char *			String;
	double			Double;
	int			Integer;
	unsigned		U_int;
	gen_fifo *		Generic_fifo;
	attr_val *		Attr_val;
	attr_val_fifo *		Attr_val_fifo;
	int_fifo *		Int_fifo;
	string_fifo *		String_fifo;
	address_node *		Address_node;
	address_fifo *		Address_fifo;
	setvar_node *		Set_var;
	server_info *		Sim_server;
	server_info_fifo *	Sim_server_fifo;
	script_info *		Sim_script;
	script_info_fifo *	Sim_script_fifo;


/* Line 350 of yacc.c  */
#line 352 "ntp_parser.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_NTP_PARSER_H  */

/* Copy the second part of user declarations.  */

/* Line 353 of yacc.c  */
#line 380 "ntp_parser.c"

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
# if defined YYENABLE_NLS && YYENABLE_NLS
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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
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
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
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

# define YYCOPY_NEEDED 1

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
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  205
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   634

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  195
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  104
/* YYNRULES -- Number of rules.  */
#define YYNRULES  308
/* YYNRULES -- Number of states.  */
#define YYNSTATES  413

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   444

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     191,   192,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   190,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   193,     2,   194,     2,     2,     2,     2,
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
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     9,    12,    15,    16,    18,    20,
      22,    24,    26,    28,    30,    32,    34,    36,    38,    40,
      42,    46,    48,    50,    52,    54,    56,    58,    61,    63,
      65,    67,    68,    71,    73,    75,    77,    79,    81,    83,
      85,    87,    89,    91,    93,    95,    98,   101,   103,   105,
     107,   109,   111,   113,   116,   118,   121,   123,   125,   127,
     130,   133,   136,   139,   142,   145,   148,   151,   154,   157,
     160,   163,   164,   167,   170,   173,   175,   177,   179,   181,
     183,   186,   189,   191,   194,   197,   200,   202,   204,   206,
     208,   210,   212,   214,   216,   218,   220,   223,   226,   230,
     233,   235,   237,   239,   241,   243,   245,   247,   249,   251,
     252,   255,   258,   261,   263,   265,   267,   269,   271,   273,
     275,   277,   279,   281,   283,   285,   287,   290,   293,   297,
     303,   307,   312,   317,   321,   322,   325,   327,   329,   331,
     333,   335,   337,   339,   341,   343,   345,   347,   349,   351,
     353,   355,   358,   360,   363,   365,   367,   369,   372,   374,
     377,   379,   381,   383,   385,   387,   389,   391,   393,   397,
     400,   402,   405,   408,   411,   414,   417,   419,   421,   423,
     425,   427,   429,   432,   435,   437,   440,   442,   444,   446,
     449,   452,   455,   457,   459,   461,   463,   465,   467,   469,
     471,   473,   475,   477,   480,   483,   485,   488,   490,   492,
     494,   496,   498,   500,   502,   504,   506,   508,   511,   514,
     517,   521,   523,   526,   529,   532,   535,   539,   542,   544,
     546,   548,   550,   552,   554,   556,   558,   560,   563,   564,
     569,   571,   572,   573,   576,   579,   582,   585,   587,   589,
     593,   597,   599,   601,   603,   605,   607,   609,   611,   613,
     615,   618,   621,   623,   625,   627,   629,   631,   633,   635,
     637,   640,   642,   645,   647,   649,   651,   657,   660,   662,
     665,   667,   669,   671,   673,   675,   677,   683,   685,   689,
     692,   696,   698,   700,   703,   705,   711,   716,   720,   723,
     725,   732,   736,   739,   743,   745,   747,   749,   751
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     196,     0,    -1,   197,    -1,   197,   198,   179,    -1,   198,
     179,    -1,     1,   179,    -1,    -1,   199,    -1,   212,    -1,
     214,    -1,   215,    -1,   224,    -1,   232,    -1,   219,    -1,
     241,    -1,   246,    -1,   250,    -1,   255,    -1,   259,    -1,
     285,    -1,   200,   201,   204,    -1,   144,    -1,   129,    -1,
     124,    -1,    14,    -1,    82,    -1,   202,    -1,   203,   154,
      -1,   154,    -1,    65,    -1,    67,    -1,    -1,   204,   205,
      -1,   206,    -1,   208,    -1,   210,    -1,   207,    -1,     9,
      -1,    17,    -1,    52,    -1,   114,    -1,   131,    -1,   132,
      -1,   165,    -1,   176,    -1,   209,    60,    -1,   209,   169,
      -1,    69,    -1,    98,    -1,    90,    -1,   167,    -1,   100,
      -1,   172,    -1,   211,   154,    -1,    53,    -1,   213,   201,
      -1,   170,    -1,   171,    -1,    15,    -1,    83,   282,    -1,
     105,   282,    -1,    91,    60,    -1,    10,    60,    -1,    22,
      60,    -1,    23,   216,    -1,    70,   154,    -1,    71,   154,
      -1,   138,    60,    -1,   141,    60,    -1,   166,   278,    -1,
     120,   154,    -1,    -1,   216,   217,    -1,   218,   154,    -1,
     141,    60,    -1,    50,    -1,    53,    -1,   134,    -1,   135,
      -1,    28,    -1,   163,   220,    -1,   220,   221,    -1,   221,
      -1,   222,    60,    -1,   223,   284,    -1,    21,   283,    -1,
      19,    -1,    47,    -1,   121,    -1,   122,    -1,    99,    -1,
      13,    -1,    96,    -1,    88,    -1,    94,    -1,    86,    -1,
     148,   225,    -1,   150,   154,    -1,    40,   226,   227,    -1,
     225,   226,    -1,   226,    -1,    20,    -1,    24,    -1,    80,
      -1,   125,    -1,   136,    -1,   156,    -1,   161,    -1,   133,
      -1,    -1,   227,   228,    -1,    39,   154,    -1,   168,   231,
      -1,   229,    -1,   230,    -1,    76,    -1,   107,    -1,    36,
      -1,    29,    -1,   110,    -1,   127,    -1,    26,    -1,   174,
      -1,   103,    -1,   177,    -1,     4,    -1,    30,   235,    -1,
     104,   238,    -1,   140,   201,   233,    -1,   140,   202,    84,
     202,   233,    -1,   140,    27,   233,    -1,   140,    65,    27,
     233,    -1,   140,    67,    27,   233,    -1,   140,   146,   233,
      -1,    -1,   233,   234,    -1,    46,    -1,    54,    -1,    72,
      -1,    73,    -1,    75,    -1,    81,    -1,   108,    -1,   109,
      -1,   112,    -1,   113,    -1,   115,    -1,   116,    -1,   117,
      -1,   119,    -1,   172,    -1,   235,   236,    -1,   236,    -1,
     237,    60,    -1,    11,    -1,    97,    -1,   102,    -1,   238,
     239,    -1,   239,    -1,   240,    60,    -1,    55,    -1,    56,
      -1,    57,    -1,    58,    -1,    85,    -1,    87,    -1,    89,
      -1,    95,    -1,    49,   201,   242,    -1,   242,   243,    -1,
     243,    -1,   244,   284,    -1,   245,   283,    -1,   153,    60,
      -1,     3,   154,    -1,   137,   154,    -1,   158,    -1,   159,
      -1,    42,    -1,    43,    -1,    44,    -1,    45,    -1,   142,
     247,    -1,   247,   248,    -1,   248,    -1,   249,    60,    -1,
      93,    -1,   147,    -1,    41,    -1,    36,   251,    -1,    29,
     251,    -1,   251,   252,    -1,   252,    -1,   253,    -1,   254,
      -1,     8,    -1,    12,    -1,    18,    -1,    68,    -1,   102,
      -1,   118,    -1,   101,    -1,   149,    -1,   162,   256,    -1,
     256,   257,    -1,   257,    -1,   258,   284,    -1,     6,    -1,
      31,    -1,    48,    -1,    51,    -1,   123,    -1,   151,    -1,
     152,    -1,   157,    -1,   270,    -1,   274,    -1,   260,   284,
      -1,   261,   154,    -1,   262,   154,    -1,    59,   154,   198,
      -1,    37,    -1,    33,   263,    -1,    78,   268,    -1,   126,
     281,    -1,   145,   264,    -1,   164,   202,   266,    -1,   167,
     277,    -1,    16,    -1,   111,    -1,   157,    -1,    53,    -1,
      74,    -1,   128,    -1,    79,    -1,   143,    -1,   154,    -1,
     154,    32,    -1,    -1,   154,   190,   154,   265,    -1,    27,
      -1,    -1,    -1,   266,   267,    -1,   130,    60,    -1,    61,
     202,    -1,   268,   269,    -1,   269,    -1,   154,    -1,   271,
     273,   272,    -1,   271,   273,   154,    -1,    61,    -1,   106,
      -1,     5,    -1,    64,    -1,    66,    -1,   175,    -1,    77,
      -1,    54,    -1,    34,    -1,   139,   275,    -1,   275,   276,
      -1,   276,    -1,     7,    -1,     8,    -1,    25,    -1,    63,
      -1,    92,    -1,   155,    -1,   160,    -1,   277,    60,    -1,
      60,    -1,   278,   279,    -1,   279,    -1,    60,    -1,   280,
      -1,   191,    60,    35,    60,   192,    -1,   281,   154,    -1,
     154,    -1,   282,   201,    -1,   201,    -1,    60,    -1,   165,
      -1,    38,    -1,    60,    -1,    32,    -1,   286,   193,   287,
     290,   194,    -1,   180,    -1,   287,   288,   179,    -1,   288,
     179,    -1,   289,   190,   284,    -1,   181,    -1,   182,    -1,
     290,   291,    -1,   291,    -1,   293,   193,   292,   294,   194,
      -1,   183,   190,   284,   179,    -1,   144,   190,   201,    -1,
     294,   295,    -1,   295,    -1,   184,   190,   284,   193,   296,
     194,    -1,   296,   297,   179,    -1,   297,   179,    -1,   298,
     190,   284,    -1,   185,    -1,   186,    -1,   187,    -1,   188,
      -1,   189,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   361,   361,   365,   366,   367,   381,   382,   383,   384,
     385,   386,   387,   388,   389,   390,   391,   392,   393,   394,
     402,   412,   413,   414,   415,   416,   420,   421,   426,   431,
     433,   439,   440,   448,   449,   450,   454,   459,   460,   461,
     462,   463,   464,   465,   466,   470,   472,   477,   478,   479,
     480,   481,   482,   486,   491,   500,   510,   511,   521,   523,
     525,   527,   538,   545,   547,   552,   554,   556,   558,   560,
     569,   575,   576,   584,   586,   598,   599,   600,   601,   602,
     611,   616,   621,   629,   631,   633,   638,   639,   640,   641,
     642,   643,   647,   648,   649,   650,   659,   661,   670,   680,
     685,   693,   694,   695,   696,   697,   698,   699,   700,   705,
     706,   714,   724,   733,   748,   753,   754,   758,   759,   763,
     764,   765,   766,   767,   768,   769,   778,   782,   786,   794,
     802,   810,   825,   840,   853,   854,   862,   863,   864,   865,
     866,   867,   868,   869,   870,   871,   872,   873,   874,   875,
     876,   880,   885,   893,   898,   899,   900,   904,   909,   917,
     922,   923,   924,   925,   926,   927,   928,   929,   937,   947,
     952,   960,   962,   964,   966,   968,   973,   974,   978,   979,
     980,   981,   989,   994,   999,  1007,  1012,  1013,  1014,  1023,
    1025,  1030,  1035,  1043,  1045,  1062,  1063,  1064,  1065,  1066,
    1067,  1071,  1072,  1080,  1085,  1090,  1098,  1103,  1104,  1105,
    1106,  1107,  1108,  1109,  1110,  1119,  1120,  1121,  1128,  1135,
    1151,  1170,  1175,  1177,  1179,  1181,  1183,  1190,  1195,  1196,
    1197,  1201,  1202,  1203,  1207,  1208,  1212,  1219,  1229,  1238,
    1243,  1245,  1250,  1251,  1259,  1261,  1269,  1274,  1282,  1307,
    1314,  1324,  1325,  1329,  1330,  1331,  1332,  1336,  1337,  1338,
    1342,  1347,  1352,  1360,  1361,  1362,  1363,  1364,  1365,  1366,
    1376,  1381,  1389,  1394,  1402,  1404,  1408,  1413,  1418,  1426,
    1431,  1439,  1448,  1449,  1453,  1454,  1463,  1481,  1485,  1490,
    1498,  1503,  1504,  1508,  1513,  1521,  1526,  1531,  1536,  1541,
    1549,  1554,  1559,  1567,  1572,  1573,  1574,  1575,  1576
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "T_Abbrev", "T_Age", "T_All", "T_Allan",
  "T_Allpeers", "T_Auth", "T_Autokey", "T_Automax", "T_Average",
  "T_Bclient", "T_Beacon", "T_Broadcast", "T_Broadcastclient",
  "T_Broadcastdelay", "T_Burst", "T_Calibrate", "T_Ceiling",
  "T_Clockstats", "T_Cohort", "T_ControlKey", "T_Crypto", "T_Cryptostats",
  "T_Ctl", "T_Day", "T_Default", "T_Digest", "T_Disable", "T_Discard",
  "T_Dispersion", "T_Double", "T_Driftfile", "T_Drop", "T_Ellipsis",
  "T_Enable", "T_End", "T_False", "T_File", "T_Filegen", "T_Filenum",
  "T_Flag1", "T_Flag2", "T_Flag3", "T_Flag4", "T_Flake", "T_Floor",
  "T_Freq", "T_Fudge", "T_Host", "T_Huffpuff", "T_Iburst", "T_Ident",
  "T_Ignore", "T_Incalloc", "T_Incmem", "T_Initalloc", "T_Initmem",
  "T_Includefile", "T_Integer", "T_Interface", "T_Intrange", "T_Io",
  "T_Ipv4", "T_Ipv4_flag", "T_Ipv6", "T_Ipv6_flag", "T_Kernel", "T_Key",
  "T_Keys", "T_Keysdir", "T_Kod", "T_Mssntp", "T_Leapfile", "T_Limited",
  "T_Link", "T_Listen", "T_Logconfig", "T_Logfile", "T_Loopstats",
  "T_Lowpriotrap", "T_Manycastclient", "T_Manycastserver", "T_Mask",
  "T_Maxage", "T_Maxclock", "T_Maxdepth", "T_Maxdist", "T_Maxmem",
  "T_Maxpoll", "T_Mdnstries", "T_Mem", "T_Memlock", "T_Minclock",
  "T_Mindepth", "T_Mindist", "T_Minimum", "T_Minpoll", "T_Minsane",
  "T_Mode", "T_Mode7", "T_Monitor", "T_Month", "T_Mru",
  "T_Multicastclient", "T_Nic", "T_Nolink", "T_Nomodify", "T_Nomrulist",
  "T_None", "T_Nonvolatile", "T_Nopeer", "T_Noquery", "T_Noselect",
  "T_Noserve", "T_Notrap", "T_Notrust", "T_Ntp", "T_Ntpport",
  "T_NtpSignDsocket", "T_Orphan", "T_Orphanwait", "T_Panic", "T_Peer",
  "T_Peerstats", "T_Phone", "T_Pid", "T_Pidfile", "T_Pool", "T_Port",
  "T_Preempt", "T_Prefer", "T_Protostats", "T_Pw", "T_Randfile",
  "T_Rawstats", "T_Refid", "T_Requestkey", "T_Reset", "T_Restrict",
  "T_Revoke", "T_Rlimit", "T_Saveconfigdir", "T_Server", "T_Setvar",
  "T_Source", "T_Stacksize", "T_Statistics", "T_Stats", "T_Statsdir",
  "T_Step", "T_Stepout", "T_Stratum", "T_String", "T_Sys", "T_Sysstats",
  "T_Tick", "T_Time1", "T_Time2", "T_Timer", "T_Timingstats", "T_Tinker",
  "T_Tos", "T_Trap", "T_True", "T_Trustedkey", "T_Ttl", "T_Type",
  "T_U_int", "T_Unconfig", "T_Unpeer", "T_Version", "T_WanderThreshold",
  "T_Week", "T_Wildcard", "T_Xleave", "T_Year", "T_Flag", "T_EOC",
  "T_Simulate", "T_Beep_Delay", "T_Sim_Duration", "T_Server_Offset",
  "T_Duration", "T_Freq_Offset", "T_Wander", "T_Jitter", "T_Prop_Delay",
  "T_Proc_Delay", "'='", "'('", "')'", "'{'", "'}'", "$accept",
  "configuration", "command_list", "command", "server_command",
  "client_type", "address", "ip_address", "address_fam", "option_list",
  "option", "option_flag", "option_flag_keyword", "option_int",
  "option_int_keyword", "option_str", "option_str_keyword",
  "unpeer_command", "unpeer_keyword", "other_mode_command",
  "authentication_command", "crypto_command_list", "crypto_command",
  "crypto_str_keyword", "orphan_mode_command", "tos_option_list",
  "tos_option", "tos_option_int_keyword", "tos_option_dbl_keyword",
  "monitoring_command", "stats_list", "stat", "filegen_option_list",
  "filegen_option", "link_nolink", "enable_disable", "filegen_type",
  "access_control_command", "ac_flag_list", "access_control_flag",
  "discard_option_list", "discard_option", "discard_option_keyword",
  "mru_option_list", "mru_option", "mru_option_keyword", "fudge_command",
  "fudge_factor_list", "fudge_factor", "fudge_factor_dbl_keyword",
  "fudge_factor_bool_keyword", "rlimit_command", "rlimit_option_list",
  "rlimit_option", "rlimit_option_keyword", "system_option_command",
  "system_option_list", "system_option", "system_option_flag_keyword",
  "system_option_local_flag_keyword", "tinker_command",
  "tinker_option_list", "tinker_option", "tinker_option_keyword",
  "miscellaneous_command", "misc_cmd_dbl_keyword", "misc_cmd_str_keyword",
  "misc_cmd_str_lcl_keyword", "drift_parm", "variable_assign",
  "t_default_or_zero", "trap_option_list", "trap_option",
  "log_config_list", "log_config_command", "interface_command",
  "interface_nic", "nic_rule_class", "nic_rule_action", "reset_command",
  "counter_set_list", "counter_set_keyword", "integer_list",
  "integer_list_range", "integer_list_range_elt", "integer_range",
  "string_list", "address_list", "boolean", "number", "simulate_command",
  "sim_conf_start", "sim_init_statement_list", "sim_init_statement",
  "sim_init_keyword", "sim_server_list", "sim_server", "sim_server_offset",
  "sim_server_name", "sim_act_list", "sim_act", "sim_act_stmt_list",
  "sim_act_stmt", "sim_act_keyword", YY_NULL
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
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
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   389,   390,   391,   392,   393,   394,
     395,   396,   397,   398,   399,   400,   401,   402,   403,   404,
     405,   406,   407,   408,   409,   410,   411,   412,   413,   414,
     415,   416,   417,   418,   419,   420,   421,   422,   423,   424,
     425,   426,   427,   428,   429,   430,   431,   432,   433,   434,
     435,   436,   437,   438,   439,   440,   441,   442,   443,   444,
      61,    40,    41,   123,   125
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   195,   196,   197,   197,   197,   198,   198,   198,   198,
     198,   198,   198,   198,   198,   198,   198,   198,   198,   198,
     199,   200,   200,   200,   200,   200,   201,   201,   202,   203,
     203,   204,   204,   205,   205,   205,   206,   207,   207,   207,
     207,   207,   207,   207,   207,   208,   208,   209,   209,   209,
     209,   209,   209,   210,   211,   212,   213,   213,   214,   214,
     214,   214,   215,   215,   215,   215,   215,   215,   215,   215,
     215,   216,   216,   217,   217,   218,   218,   218,   218,   218,
     219,   220,   220,   221,   221,   221,   222,   222,   222,   222,
     222,   222,   223,   223,   223,   223,   224,   224,   224,   225,
     225,   226,   226,   226,   226,   226,   226,   226,   226,   227,
     227,   228,   228,   228,   228,   229,   229,   230,   230,   231,
     231,   231,   231,   231,   231,   231,   232,   232,   232,   232,
     232,   232,   232,   232,   233,   233,   234,   234,   234,   234,
     234,   234,   234,   234,   234,   234,   234,   234,   234,   234,
     234,   235,   235,   236,   237,   237,   237,   238,   238,   239,
     240,   240,   240,   240,   240,   240,   240,   240,   241,   242,
     242,   243,   243,   243,   243,   243,   244,   244,   245,   245,
     245,   245,   246,   247,   247,   248,   249,   249,   249,   250,
     250,   251,   251,   252,   252,   253,   253,   253,   253,   253,
     253,   254,   254,   255,   256,   256,   257,   258,   258,   258,
     258,   258,   258,   258,   258,   259,   259,   259,   259,   259,
     259,   259,   259,   259,   259,   259,   259,   259,   260,   260,
     260,   261,   261,   261,   262,   262,   263,   263,   263,   264,
     265,   265,   266,   266,   267,   267,   268,   268,   269,   270,
     270,   271,   271,   272,   272,   272,   272,   273,   273,   273,
     274,   275,   275,   276,   276,   276,   276,   276,   276,   276,
     277,   277,   278,   278,   279,   279,   280,   281,   281,   282,
     282,   283,   283,   283,   284,   284,   285,   286,   287,   287,
     288,   289,   289,   290,   290,   291,   292,   293,   294,   294,
     295,   296,   296,   297,   298,   298,   298,   298,   298
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     3,     2,     2,     0,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       3,     1,     1,     1,     1,     1,     1,     2,     1,     1,
       1,     0,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     2,     2,     1,     1,     1,
       1,     1,     1,     2,     1,     2,     1,     1,     1,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     0,     2,     2,     2,     1,     1,     1,     1,     1,
       2,     2,     1,     2,     2,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     2,     3,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     0,
       2,     2,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     2,     3,     5,
       3,     4,     4,     3,     0,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     2,     1,     2,     1,     1,     1,     2,     1,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     3,     2,
       1,     2,     2,     2,     2,     2,     1,     1,     1,     1,
       1,     1,     2,     2,     1,     2,     1,     1,     1,     2,
       2,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     2,     1,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     2,     2,
       3,     1,     2,     2,     2,     2,     3,     2,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     0,     4,
       1,     0,     0,     2,     2,     2,     2,     1,     1,     3,
       3,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       2,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       2,     1,     2,     1,     1,     1,     5,     2,     1,     2,
       1,     1,     1,     1,     1,     1,     5,     1,     3,     2,
       3,     1,     1,     2,     1,     5,     4,     3,     2,     1,
       6,     3,     2,     3,     1,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,     0,     0,    24,    58,   228,     0,    71,     0,     0,
     238,     0,   221,     0,     0,   231,     0,   251,     0,     0,
     232,     0,   234,    25,     0,     0,     0,     0,   252,   229,
       0,    23,     0,   233,    22,     0,     0,     0,     0,     0,
     235,    21,     0,     0,     0,   230,     0,     0,     0,     0,
       0,    56,    57,   287,     0,     2,     0,     7,     0,     8,
       0,     9,    10,    13,    11,    12,    14,    15,    16,    17,
      18,     0,     0,     0,   215,     0,   216,    19,     0,     5,
      62,    63,    64,   195,   196,   197,   198,   201,   199,   200,
     202,   190,   192,   193,   194,   154,   155,   156,   126,   152,
       0,   236,   222,   189,   101,   102,   103,   104,   108,   105,
     106,   107,   109,    29,    30,    28,     0,    26,     0,     6,
      65,    66,   248,   223,   247,   280,    59,    61,   160,   161,
     162,   163,   164,   165,   166,   167,   127,   158,     0,    60,
      70,   278,   224,    67,   263,   264,   265,   266,   267,   268,
     269,   260,   262,   134,    29,    30,   134,   134,    26,    68,
     188,   186,   187,   182,   184,     0,     0,   225,    96,   100,
      97,   207,   208,   209,   210,   211,   212,   213,   214,   203,
     205,     0,    91,    86,     0,    87,    95,    93,    94,    92,
      90,    88,    89,    80,    82,     0,     0,   242,   274,     0,
      69,   273,   275,   271,   227,     1,     0,     4,    31,    55,
     285,   284,   217,   218,   219,   259,   258,   257,     0,     0,
      79,    75,    76,    77,    78,     0,    72,     0,   191,   151,
     153,   237,    98,     0,   178,   179,   180,   181,     0,     0,
     176,   177,   168,   170,     0,     0,    27,   220,   246,   279,
     157,   159,   277,   261,   130,   134,   134,   133,   128,     0,
     183,   185,     0,    99,   204,   206,   283,   281,   282,    85,
      81,    83,    84,   226,     0,   272,   270,     3,    20,   253,
     254,   255,   250,   256,   249,   291,   292,     0,     0,     0,
      74,    73,   118,   117,     0,   115,   116,     0,   110,   113,
     114,   174,   175,   173,   169,   171,   172,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   148,
     149,   150,   135,   131,   132,   134,   241,     0,     0,   243,
       0,    37,    38,    39,    54,    47,    49,    48,    51,    40,
      41,    42,    43,    50,    52,    44,    32,    33,    36,    34,
       0,    35,     0,     0,     0,     0,   294,     0,   289,     0,
     111,   125,   121,   123,   119,   120,   122,   124,   112,   129,
     240,   239,   245,   244,     0,    45,    46,    53,     0,   288,
     286,   293,     0,   290,   276,   297,     0,     0,     0,     0,
       0,   299,     0,     0,   295,   298,   296,     0,     0,   304,
     305,   306,   307,   308,     0,     0,     0,   300,     0,   302,
       0,   301,   303
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    54,    55,    56,    57,    58,   125,   117,   118,   278,
     346,   347,   348,   349,   350,   351,   352,    59,    60,    61,
      62,    82,   226,   227,    63,   193,   194,   195,   196,    64,
     168,   112,   232,   298,   299,   300,   368,    65,   254,   322,
      98,    99,   100,   136,   137,   138,    66,   242,   243,   244,
     245,    67,   163,   164,   165,    68,    91,    92,    93,    94,
      69,   179,   180,   181,    70,    71,    72,    73,   102,   167,
     371,   273,   329,   123,   124,    74,    75,   284,   218,    76,
     151,   152,   204,   200,   201,   202,   142,   126,   269,   212,
      77,    78,   287,   288,   289,   355,   356,   387,   357,   390,
     391,   404,   405,   406
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -180
static const yytype_int16 yypact[] =
{
      26,  -147,    -9,  -180,  -180,  -180,    -6,  -180,   222,     9,
     -96,   222,  -180,   276,   -41,  -180,   -93,  -180,   -90,   -86,
    -180,   -84,  -180,  -180,   -41,     0,   443,   -41,  -180,  -180,
     -83,  -180,   -82,  -180,  -180,    20,    27,   -20,    21,   -19,
    -180,  -180,   -68,   276,   -66,  -180,   221,   328,   -65,   -55,
      38,  -180,  -180,  -180,    99,   188,   -69,  -180,   -41,  -180,
     -41,  -180,  -180,  -180,  -180,  -180,  -180,  -180,  -180,  -180,
    -180,    -3,   -40,   -39,  -180,    -4,  -180,  -180,   -77,  -180,
    -180,  -180,   254,  -180,  -180,  -180,  -180,  -180,  -180,  -180,
    -180,   222,  -180,  -180,  -180,  -180,  -180,  -180,     9,  -180,
      47,    90,  -180,   222,  -180,  -180,  -180,  -180,  -180,  -180,
    -180,  -180,  -180,  -180,  -180,  -180,    98,  -180,   -31,   347,
    -180,  -180,  -180,   -84,  -180,  -180,   -41,  -180,  -180,  -180,
    -180,  -180,  -180,  -180,  -180,  -180,   443,  -180,    61,   -41,
    -180,  -180,   -30,  -180,  -180,  -180,  -180,  -180,  -180,  -180,
    -180,    27,  -180,  -180,   102,   106,  -180,  -180,    55,  -180,
    -180,  -180,  -180,   -19,  -180,    87,   -42,  -180,   276,  -180,
    -180,  -180,  -180,  -180,  -180,  -180,  -180,  -180,  -180,   221,
    -180,    -3,  -180,  -180,   -27,  -180,  -180,  -180,  -180,  -180,
    -180,  -180,  -180,   328,  -180,    91,    -3,  -180,  -180,    93,
     -55,  -180,  -180,  -180,    96,  -180,   -21,  -180,  -180,  -180,
    -180,  -180,  -180,  -180,  -180,  -180,  -180,  -180,     3,  -144,
    -180,  -180,  -180,  -180,  -180,   100,  -180,    -5,  -180,  -180,
    -180,  -180,   -23,     5,  -180,  -180,  -180,  -180,     7,   103,
    -180,  -180,    98,  -180,    -3,   -27,  -180,  -180,  -180,  -180,
    -180,  -180,  -180,  -180,   462,  -180,  -180,   462,   462,   -65,
    -180,  -180,     8,  -180,  -180,  -180,  -180,  -180,  -180,  -180,
    -180,  -180,  -180,   -47,   138,  -180,  -180,  -180,   425,  -180,
    -180,  -180,  -180,  -180,  -180,  -180,  -180,  -105,    -2,   -15,
    -180,  -180,  -180,  -180,    25,  -180,  -180,    17,  -180,  -180,
    -180,  -180,  -180,  -180,  -180,  -180,  -180,  -180,  -180,  -180,
    -180,  -180,  -180,  -180,  -180,  -180,  -180,  -180,  -180,  -180,
    -180,  -180,  -180,   462,   462,  -180,   154,   -65,   124,  -180,
     125,  -180,  -180,  -180,  -180,  -180,  -180,  -180,  -180,  -180,
    -180,  -180,  -180,  -180,  -180,  -180,  -180,  -180,  -180,  -180,
     -51,  -180,    32,    10,    16,  -116,  -180,     6,  -180,    -3,
    -180,  -180,  -180,  -180,  -180,  -180,  -180,  -180,  -180,   462,
    -180,  -180,  -180,  -180,    15,  -180,  -180,  -180,   -41,  -180,
    -180,  -180,    18,  -180,  -180,  -180,    22,    24,    -3,    23,
    -169,  -180,    36,    -3,  -180,  -180,  -180,    29,   -94,  -180,
    -180,  -180,  -180,  -180,    89,    37,    30,  -180,    40,  -180,
      -3,  -180,  -180
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -180,  -180,  -180,   -37,  -180,  -180,   -14,   -36,  -180,  -180,
    -180,  -180,  -180,  -180,  -180,  -180,  -180,  -180,  -180,  -180,
    -180,  -180,  -180,  -180,  -180,  -180,    33,  -180,  -180,  -180,
    -180,   -33,  -180,  -180,  -180,  -180,  -180,  -180,  -153,  -180,
    -180,   131,  -180,  -180,    97,  -180,  -180,  -180,   -10,  -180,
    -180,  -180,  -180,    73,  -180,  -180,   227,   -72,  -180,  -180,
    -180,  -180,    60,  -180,  -180,  -180,  -180,  -180,  -180,  -180,
    -180,  -180,  -180,  -180,   119,  -180,  -180,  -180,  -180,  -180,
    -180,    92,  -180,  -180,    44,  -180,  -180,   218,     1,  -179,
    -180,  -180,  -180,   -34,  -180,  -180,  -107,  -180,  -180,  -180,
    -140,  -180,  -150,  -180
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -7
static const yytype_int16 yytable[] =
{
     116,   158,   265,   257,   258,   198,   292,   153,   279,   375,
     169,   266,   197,   293,   327,   389,   294,   272,   206,   228,
      95,   361,   160,   157,   113,   394,   114,     1,   353,   210,
     215,   228,    79,   267,   144,   145,     2,   285,   286,   353,
       3,     4,     5,   362,   208,   154,   209,   155,     6,     7,
     216,    80,   146,   295,    81,     8,     9,   211,   101,    10,
     127,   119,    11,    12,   120,   305,    13,   280,   121,   281,
     122,   140,   141,   217,   161,    14,   285,   286,   380,    15,
     143,   159,   247,   328,   296,    16,   166,    17,   170,   115,
     147,   399,   400,   401,   402,   403,    18,    19,   203,   205,
      20,   233,   323,   324,    21,    22,    96,   230,    23,    24,
     207,    97,   249,   115,   213,   214,   219,    25,   376,   148,
     363,   251,   231,   246,   252,   249,   156,   364,   162,   255,
      26,    27,    28,   256,   115,   263,   199,    29,   268,   259,
     234,   235,   236,   237,   365,   297,    30,   261,   262,   291,
      31,   271,    32,   274,    33,    34,   276,   282,   277,   301,
     290,   302,   326,   303,    35,    36,    37,    38,    39,    40,
      41,    42,   369,   330,    43,   359,    44,   358,   283,   360,
     383,   370,   149,    45,   373,   374,   377,   150,    46,    47,
      48,   366,    49,    50,   367,   379,    51,    52,     2,   382,
     378,   386,     3,     4,     5,    -6,    53,   384,   389,   392,
       6,     7,   388,   393,   397,   396,   409,     8,     9,   411,
     410,    10,   398,   325,    11,    12,   270,   171,    13,   229,
      83,   412,   304,   250,    84,   238,   260,    14,   103,   264,
      85,    15,   248,   253,   275,   139,   306,    16,   381,    17,
     395,   239,   172,   354,   408,     0,   240,   241,    18,    19,
       0,     0,    20,     0,     0,     0,    21,    22,     0,   173,
      23,    24,   174,     0,   399,   400,   401,   402,   403,    25,
       0,     0,   220,   407,     0,     0,     0,     0,     0,     0,
      86,   372,    26,    27,    28,     0,   104,     0,     0,    29,
     105,     0,     0,     0,   221,     0,     0,   222,    30,     0,
       0,     0,    31,     0,    32,     0,    33,    34,     0,     0,
       0,     0,     0,    87,    88,     0,    35,    36,    37,    38,
      39,    40,    41,    42,     0,     0,    43,     0,    44,     0,
      89,   182,     0,     0,   175,    45,     0,   183,     0,   184,
      46,    47,    48,     0,    49,    50,   106,     2,    51,    52,
       0,     3,     4,     5,   385,     0,     0,    -6,    53,     6,
       7,    90,   176,   177,     0,   185,     8,     9,   178,     0,
      10,     0,     0,    11,    12,     0,     0,    13,   223,   224,
       0,     0,     0,     0,     0,   225,    14,     0,     0,     0,
      15,   107,     0,     0,     0,     0,    16,     0,    17,   108,
       0,     0,   109,     0,   186,     0,   187,    18,    19,     0,
       0,    20,   188,     0,   189,    21,    22,   190,     0,    23,
      24,     0,   110,     0,   331,     0,     0,   111,    25,     0,
       0,     0,   332,     0,     0,     0,     0,     0,     0,   191,
     192,    26,    27,    28,     0,     0,     0,     0,    29,     0,
       0,     0,     0,     0,     0,     0,     0,    30,     0,     0,
       0,    31,     0,    32,     0,    33,    34,   333,   334,     0,
       0,     0,     0,     0,     0,    35,    36,    37,    38,    39,
      40,    41,    42,     0,   335,    43,     0,    44,   128,   129,
     130,   131,     0,     0,    45,     0,     0,     0,   307,    46,
      47,    48,     0,    49,    50,   336,   308,    51,    52,     0,
       0,     0,     0,   337,     0,   338,     0,    53,   132,     0,
     133,     0,   134,     0,   309,   310,     0,   311,   135,   339,
       0,     0,     0,   312,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   340,   341,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     313,   314,     0,     0,   315,   316,     0,   317,   318,   319,
       0,   320,     0,     0,     0,     0,     0,     0,     0,     0,
     342,     0,   343,     0,     0,     0,     0,   344,     0,     0,
       0,   345,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   321
};

#define yypact_value_is_default(yystate) \
  ((yystate) == (-180))

#define yytable_value_is_error(yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
      14,    37,   181,   156,   157,    60,    29,    27,     5,    60,
      43,    38,    48,    36,    61,   184,    39,   196,    55,    91,
      11,     4,    41,    37,    65,   194,    67,     1,   144,    32,
      34,   103,   179,    60,     7,     8,    10,   181,   182,   144,
      14,    15,    16,    26,    58,    65,    60,    67,    22,    23,
      54,    60,    25,    76,    60,    29,    30,    60,   154,    33,
      60,   154,    36,    37,   154,   244,    40,    64,   154,    66,
     154,   154,   154,    77,    93,    49,   181,   182,   194,    53,
      60,    60,   119,   130,   107,    59,   154,    61,   154,   154,
      63,   185,   186,   187,   188,   189,    70,    71,    60,     0,
      74,     3,   255,   256,    78,    79,    97,    60,    82,    83,
     179,   102,   126,   154,   154,   154,   193,    91,   169,    92,
     103,    60,    32,   154,   154,   139,   146,   110,   147,    27,
     104,   105,   106,    27,   154,   168,   191,   111,   165,    84,
      42,    43,    44,    45,   127,   168,   120,    60,   190,   154,
     124,    60,   126,    60,   128,   129,    60,   154,   179,   154,
      60,   154,   154,    60,   138,   139,   140,   141,   142,   143,
     144,   145,   325,    35,   148,   190,   150,   179,   175,   154,
     359,    27,   155,   157,    60,    60,   154,   160,   162,   163,
     164,   174,   166,   167,   177,   179,   170,   171,    10,   193,
     190,   183,    14,    15,    16,   179,   180,   192,   184,   388,
      22,    23,   190,   190,   393,   179,   179,    29,    30,   179,
     190,    33,   193,   259,    36,    37,   193,     6,    40,    98,
       8,   410,   242,   136,    12,   137,   163,    49,    11,   179,
      18,    53,   123,   151,   200,    27,   245,    59,   355,    61,
     390,   153,    31,   287,   404,    -1,   158,   159,    70,    71,
      -1,    -1,    74,    -1,    -1,    -1,    78,    79,    -1,    48,
      82,    83,    51,    -1,   185,   186,   187,   188,   189,    91,
      -1,    -1,    28,   194,    -1,    -1,    -1,    -1,    -1,    -1,
      68,   327,   104,   105,   106,    -1,    20,    -1,    -1,   111,
      24,    -1,    -1,    -1,    50,    -1,    -1,    53,   120,    -1,
      -1,    -1,   124,    -1,   126,    -1,   128,   129,    -1,    -1,
      -1,    -1,    -1,   101,   102,    -1,   138,   139,   140,   141,
     142,   143,   144,   145,    -1,    -1,   148,    -1,   150,    -1,
     118,    13,    -1,    -1,   123,   157,    -1,    19,    -1,    21,
     162,   163,   164,    -1,   166,   167,    80,    10,   170,   171,
      -1,    14,    15,    16,   378,    -1,    -1,   179,   180,    22,
      23,   149,   151,   152,    -1,    47,    29,    30,   157,    -1,
      33,    -1,    -1,    36,    37,    -1,    -1,    40,   134,   135,
      -1,    -1,    -1,    -1,    -1,   141,    49,    -1,    -1,    -1,
      53,   125,    -1,    -1,    -1,    -1,    59,    -1,    61,   133,
      -1,    -1,   136,    -1,    86,    -1,    88,    70,    71,    -1,
      -1,    74,    94,    -1,    96,    78,    79,    99,    -1,    82,
      83,    -1,   156,    -1,     9,    -1,    -1,   161,    91,    -1,
      -1,    -1,    17,    -1,    -1,    -1,    -1,    -1,    -1,   121,
     122,   104,   105,   106,    -1,    -1,    -1,    -1,   111,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   120,    -1,    -1,
      -1,   124,    -1,   126,    -1,   128,   129,    52,    53,    -1,
      -1,    -1,    -1,    -1,    -1,   138,   139,   140,   141,   142,
     143,   144,   145,    -1,    69,   148,    -1,   150,    55,    56,
      57,    58,    -1,    -1,   157,    -1,    -1,    -1,    46,   162,
     163,   164,    -1,   166,   167,    90,    54,   170,   171,    -1,
      -1,    -1,    -1,    98,    -1,   100,    -1,   180,    85,    -1,
      87,    -1,    89,    -1,    72,    73,    -1,    75,    95,   114,
      -1,    -1,    -1,    81,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   131,   132,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     108,   109,    -1,    -1,   112,   113,    -1,   115,   116,   117,
      -1,   119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     165,    -1,   167,    -1,    -1,    -1,    -1,   172,    -1,    -1,
      -1,   176,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   172
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,     1,    10,    14,    15,    16,    22,    23,    29,    30,
      33,    36,    37,    40,    49,    53,    59,    61,    70,    71,
      74,    78,    79,    82,    83,    91,   104,   105,   106,   111,
     120,   124,   126,   128,   129,   138,   139,   140,   141,   142,
     143,   144,   145,   148,   150,   157,   162,   163,   164,   166,
     167,   170,   171,   180,   196,   197,   198,   199,   200,   212,
     213,   214,   215,   219,   224,   232,   241,   246,   250,   255,
     259,   260,   261,   262,   270,   271,   274,   285,   286,   179,
      60,    60,   216,     8,    12,    18,    68,   101,   102,   118,
     149,   251,   252,   253,   254,    11,    97,   102,   235,   236,
     237,   154,   263,   251,    20,    24,    80,   125,   133,   136,
     156,   161,   226,    65,    67,   154,   201,   202,   203,   154,
     154,   154,   154,   268,   269,   201,   282,    60,    55,    56,
      57,    58,    85,    87,    89,    95,   238,   239,   240,   282,
     154,   154,   281,    60,     7,     8,    25,    63,    92,   155,
     160,   275,   276,    27,    65,    67,   146,   201,   202,    60,
      41,    93,   147,   247,   248,   249,   154,   264,   225,   226,
     154,     6,    31,    48,    51,   123,   151,   152,   157,   256,
     257,   258,    13,    19,    21,    47,    86,    88,    94,    96,
      99,   121,   122,   220,   221,   222,   223,   202,    60,   191,
     278,   279,   280,    60,   277,     0,   198,   179,   201,   201,
      32,    60,   284,   154,   154,    34,    54,    77,   273,   193,
      28,    50,    53,   134,   135,   141,   217,   218,   252,   236,
      60,    32,   227,     3,    42,    43,    44,    45,   137,   153,
     158,   159,   242,   243,   244,   245,   154,   198,   269,   201,
     239,    60,   154,   276,   233,    27,    27,   233,   233,    84,
     248,    60,   190,   226,   257,   284,    38,    60,   165,   283,
     221,    60,   284,   266,    60,   279,    60,   179,   204,     5,
      64,    66,   154,   175,   272,   181,   182,   287,   288,   289,
      60,   154,    29,    36,    39,    76,   107,   168,   228,   229,
     230,   154,   154,    60,   243,   284,   283,    46,    54,    72,
      73,    75,    81,   108,   109,   112,   113,   115,   116,   117,
     119,   172,   234,   233,   233,   202,   154,    61,   130,   267,
      35,     9,    17,    52,    53,    69,    90,    98,   100,   114,
     131,   132,   165,   167,   172,   176,   205,   206,   207,   208,
     209,   210,   211,   144,   288,   290,   291,   293,   179,   190,
     154,     4,    26,   103,   110,   127,   174,   177,   231,   233,
      27,   265,   202,    60,    60,    60,   169,   154,   190,   179,
     194,   291,   193,   284,   192,   201,   183,   292,   190,   184,
     294,   295,   284,   190,   194,   295,   179,   284,   193,   185,
     186,   187,   188,   189,   296,   297,   298,   194,   297,   179,
     190,   179,   284
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
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

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
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (YYID (N))                                                     \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (YYID (0))
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])



/* This macro is provided for backward compatibility. */

#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
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
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
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
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
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
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
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
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
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
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
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
  YYSIZE_T yysize0 = yytnamerr (YY_NULL, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  YYSIZE_T yysize1;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULL;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
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
                yysize1 = yysize + yytnamerr (YY_NULL, yytname[yyx]);
                if (! (yysize <= yysize1
                       && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                  return 2;
                yysize = yysize1;
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

  yysize1 = yysize + yystrlen (yyformat);
  if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
    return 2;
  yysize = yysize1;

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

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
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
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
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
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

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
  if (yypact_value_is_default (yyn))
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
        case 5:
/* Line 1787 of yacc.c  */
#line 368 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			/* I will need to incorporate much more fine grained
			 * error messages. The following should suffice for
			 * the time being.
			 */
			msyslog(LOG_ERR, 
				"syntax error in %s line %d, column %d",
				ip_file->fname,
				ip_file->err_line_no,
				ip_file->err_col_no);
		}
    break;

  case 20:
/* Line 1787 of yacc.c  */
#line 403 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			peer_node *my_node;

			my_node = create_peer_node((yyvsp[(1) - (3)].Integer), (yyvsp[(2) - (3)].Address_node), (yyvsp[(3) - (3)].Attr_val_fifo));
			APPEND_G_FIFO(cfgt.peers, my_node);
		}
    break;

  case 27:
/* Line 1787 of yacc.c  */
#line 422 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Address_node) = create_address_node((yyvsp[(2) - (2)].String), (yyvsp[(1) - (2)].Integer)); }
    break;

  case 28:
/* Line 1787 of yacc.c  */
#line 427 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Address_node) = create_address_node((yyvsp[(1) - (1)].String), AF_UNSPEC); }
    break;

  case 29:
/* Line 1787 of yacc.c  */
#line 432 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Integer) = AF_INET; }
    break;

  case 30:
/* Line 1787 of yacc.c  */
#line 434 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Integer) = AF_INET6; }
    break;

  case 31:
/* Line 1787 of yacc.c  */
#line 439 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val_fifo) = NULL; }
    break;

  case 32:
/* Line 1787 of yacc.c  */
#line 441 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = (yyvsp[(1) - (2)].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[(2) - (2)].Attr_val));
		}
    break;

  case 36:
/* Line 1787 of yacc.c  */
#line 455 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val) = create_attr_ival(T_Flag, (yyvsp[(1) - (1)].Integer)); }
    break;

  case 45:
/* Line 1787 of yacc.c  */
#line 471 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val) = create_attr_ival((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Integer)); }
    break;

  case 46:
/* Line 1787 of yacc.c  */
#line 473 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val) = create_attr_uval((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Integer)); }
    break;

  case 53:
/* Line 1787 of yacc.c  */
#line 487 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val) = create_attr_sval((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].String)); }
    break;

  case 55:
/* Line 1787 of yacc.c  */
#line 501 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			unpeer_node *my_node;
			
			my_node = create_unpeer_node((yyvsp[(2) - (2)].Address_node));
			if (my_node)
				APPEND_G_FIFO(cfgt.unpeers, my_node);
		}
    break;

  case 58:
/* Line 1787 of yacc.c  */
#line 522 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { cfgt.broadcastclient = 1; }
    break;

  case 59:
/* Line 1787 of yacc.c  */
#line 524 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { CONCAT_G_FIFOS(cfgt.manycastserver, (yyvsp[(2) - (2)].Address_fifo)); }
    break;

  case 60:
/* Line 1787 of yacc.c  */
#line 526 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { CONCAT_G_FIFOS(cfgt.multicastclient, (yyvsp[(2) - (2)].Address_fifo)); }
    break;

  case 61:
/* Line 1787 of yacc.c  */
#line 528 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { cfgt.mdnstries = (yyvsp[(2) - (2)].Integer); }
    break;

  case 62:
/* Line 1787 of yacc.c  */
#line 539 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			attr_val *atrv;
			
			atrv = create_attr_ival((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Integer));
			APPEND_G_FIFO(cfgt.vars, atrv);
		}
    break;

  case 63:
/* Line 1787 of yacc.c  */
#line 546 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { cfgt.auth.control_key = (yyvsp[(2) - (2)].Integer); }
    break;

  case 64:
/* Line 1787 of yacc.c  */
#line 548 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { 
			cfgt.auth.cryptosw++;
			CONCAT_G_FIFOS(cfgt.auth.crypto_cmd_list, (yyvsp[(2) - (2)].Attr_val_fifo));
		}
    break;

  case 65:
/* Line 1787 of yacc.c  */
#line 553 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { cfgt.auth.keys = (yyvsp[(2) - (2)].String); }
    break;

  case 66:
/* Line 1787 of yacc.c  */
#line 555 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { cfgt.auth.keysdir = (yyvsp[(2) - (2)].String); }
    break;

  case 67:
/* Line 1787 of yacc.c  */
#line 557 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { cfgt.auth.request_key = (yyvsp[(2) - (2)].Integer); }
    break;

  case 68:
/* Line 1787 of yacc.c  */
#line 559 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { cfgt.auth.revoke = (yyvsp[(2) - (2)].Integer); }
    break;

  case 69:
/* Line 1787 of yacc.c  */
#line 561 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			cfgt.auth.trusted_key_list = (yyvsp[(2) - (2)].Attr_val_fifo);

			// if (!cfgt.auth.trusted_key_list)
			// 	cfgt.auth.trusted_key_list = $2;
			// else
			// 	LINK_SLIST(cfgt.auth.trusted_key_list, $2, link);
		}
    break;

  case 70:
/* Line 1787 of yacc.c  */
#line 570 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { cfgt.auth.ntp_signd_socket = (yyvsp[(2) - (2)].String); }
    break;

  case 71:
/* Line 1787 of yacc.c  */
#line 575 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val_fifo) = NULL; }
    break;

  case 72:
/* Line 1787 of yacc.c  */
#line 577 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = (yyvsp[(1) - (2)].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[(2) - (2)].Attr_val));
		}
    break;

  case 73:
/* Line 1787 of yacc.c  */
#line 585 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val) = create_attr_sval((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].String)); }
    break;

  case 74:
/* Line 1787 of yacc.c  */
#line 587 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val) = NULL;
			cfgt.auth.revoke = (yyvsp[(2) - (2)].Integer);
			msyslog(LOG_WARNING,
				"'crypto revoke %d' is deprecated, "
				"please use 'revoke %d' instead.",
				cfgt.auth.revoke, cfgt.auth.revoke);
		}
    break;

  case 80:
/* Line 1787 of yacc.c  */
#line 612 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { CONCAT_G_FIFOS(cfgt.orphan_cmds, (yyvsp[(2) - (2)].Attr_val_fifo)); }
    break;

  case 81:
/* Line 1787 of yacc.c  */
#line 617 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = (yyvsp[(1) - (2)].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[(2) - (2)].Attr_val));
		}
    break;

  case 82:
/* Line 1787 of yacc.c  */
#line 622 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {	
			(yyval.Attr_val_fifo) = NULL;
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[(1) - (1)].Attr_val));
		}
    break;

  case 83:
/* Line 1787 of yacc.c  */
#line 630 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val) = create_attr_dval((yyvsp[(1) - (2)].Integer), (double)(yyvsp[(2) - (2)].Integer)); }
    break;

  case 84:
/* Line 1787 of yacc.c  */
#line 632 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val) = create_attr_dval((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Double)); }
    break;

  case 85:
/* Line 1787 of yacc.c  */
#line 634 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val) = create_attr_dval((yyvsp[(1) - (2)].Integer), (double)(yyvsp[(2) - (2)].Integer)); }
    break;

  case 96:
/* Line 1787 of yacc.c  */
#line 660 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { CONCAT_G_FIFOS(cfgt.stats_list, (yyvsp[(2) - (2)].Int_fifo)); }
    break;

  case 97:
/* Line 1787 of yacc.c  */
#line 662 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			if (input_from_file) {
				cfgt.stats_dir = (yyvsp[(2) - (2)].String);
			} else {
				YYFREE((yyvsp[(2) - (2)].String));
				yyerror("statsdir remote configuration ignored");
			}
		}
    break;

  case 98:
/* Line 1787 of yacc.c  */
#line 671 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			filegen_node *fgn;
			
			fgn = create_filegen_node((yyvsp[(2) - (3)].Integer), (yyvsp[(3) - (3)].Attr_val_fifo));
			APPEND_G_FIFO(cfgt.filegen_opts, fgn);
		}
    break;

  case 99:
/* Line 1787 of yacc.c  */
#line 681 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Int_fifo) = (yyvsp[(1) - (2)].Int_fifo);
			APPEND_G_FIFO((yyval.Int_fifo), create_int_node((yyvsp[(2) - (2)].Integer)));
		}
    break;

  case 100:
/* Line 1787 of yacc.c  */
#line 686 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Int_fifo) = NULL;
			APPEND_G_FIFO((yyval.Int_fifo), create_int_node((yyvsp[(1) - (1)].Integer)));
		}
    break;

  case 109:
/* Line 1787 of yacc.c  */
#line 705 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val_fifo) = NULL; }
    break;

  case 110:
/* Line 1787 of yacc.c  */
#line 707 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = (yyvsp[(1) - (2)].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[(2) - (2)].Attr_val));
		}
    break;

  case 111:
/* Line 1787 of yacc.c  */
#line 715 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			if (input_from_file) {
				(yyval.Attr_val) = create_attr_sval((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].String));
			} else {
				(yyval.Attr_val) = NULL;
				YYFREE((yyvsp[(2) - (2)].String));
				yyerror("filegen file remote config ignored");
			}
		}
    break;

  case 112:
/* Line 1787 of yacc.c  */
#line 725 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			if (input_from_file) {
				(yyval.Attr_val) = create_attr_ival((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Integer));
			} else {
				(yyval.Attr_val) = NULL;
				yyerror("filegen type remote config ignored");
			}
		}
    break;

  case 113:
/* Line 1787 of yacc.c  */
#line 734 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			const char *err;
			
			if (input_from_file) {
				(yyval.Attr_val) = create_attr_ival(T_Flag, (yyvsp[(1) - (1)].Integer));
			} else {
				(yyval.Attr_val) = NULL;
				if (T_Link == (yyvsp[(1) - (1)].Integer))
					err = "filegen link remote config ignored";
				else
					err = "filegen nolink remote config ignored";
				yyerror(err);
			}
		}
    break;

  case 114:
/* Line 1787 of yacc.c  */
#line 749 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val) = create_attr_ival(T_Flag, (yyvsp[(1) - (1)].Integer)); }
    break;

  case 126:
/* Line 1787 of yacc.c  */
#line 779 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			CONCAT_G_FIFOS(cfgt.discard_opts, (yyvsp[(2) - (2)].Attr_val_fifo));
		}
    break;

  case 127:
/* Line 1787 of yacc.c  */
#line 783 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			CONCAT_G_FIFOS(cfgt.mru_opts, (yyvsp[(2) - (2)].Attr_val_fifo));
		}
    break;

  case 128:
/* Line 1787 of yacc.c  */
#line 787 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			restrict_node *rn;

			rn = create_restrict_node((yyvsp[(2) - (3)].Address_node), NULL, (yyvsp[(3) - (3)].Int_fifo),
						  ip_file->line_no);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
    break;

  case 129:
/* Line 1787 of yacc.c  */
#line 795 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			restrict_node *rn;

			rn = create_restrict_node((yyvsp[(2) - (5)].Address_node), (yyvsp[(4) - (5)].Address_node), (yyvsp[(5) - (5)].Int_fifo),
						  ip_file->line_no);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
    break;

  case 130:
/* Line 1787 of yacc.c  */
#line 803 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			restrict_node *rn;

			rn = create_restrict_node(NULL, NULL, (yyvsp[(3) - (3)].Int_fifo),
						  ip_file->line_no);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
    break;

  case 131:
/* Line 1787 of yacc.c  */
#line 811 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			restrict_node *rn;

			rn = create_restrict_node(
				create_address_node(
					estrdup("0.0.0.0"), 
					AF_INET),
				create_address_node(
					estrdup("0.0.0.0"), 
					AF_INET),
				(yyvsp[(4) - (4)].Int_fifo), 
				ip_file->line_no);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
    break;

  case 132:
/* Line 1787 of yacc.c  */
#line 826 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			restrict_node *rn;
			
			rn = create_restrict_node(
				create_address_node(
					estrdup("::"), 
					AF_INET6),
				create_address_node(
					estrdup("::"), 
					AF_INET6),
				(yyvsp[(4) - (4)].Int_fifo), 
				ip_file->line_no);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
    break;

  case 133:
/* Line 1787 of yacc.c  */
#line 841 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			restrict_node *	rn;

			APPEND_G_FIFO((yyvsp[(3) - (3)].Int_fifo), create_int_node((yyvsp[(2) - (3)].Integer)));
			rn = create_restrict_node(
				NULL, NULL, (yyvsp[(3) - (3)].Int_fifo), ip_file->line_no);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
    break;

  case 134:
/* Line 1787 of yacc.c  */
#line 853 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Int_fifo) = NULL; }
    break;

  case 135:
/* Line 1787 of yacc.c  */
#line 855 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Int_fifo) = (yyvsp[(1) - (2)].Int_fifo);
			APPEND_G_FIFO((yyval.Int_fifo), create_int_node((yyvsp[(2) - (2)].Integer)));
		}
    break;

  case 151:
/* Line 1787 of yacc.c  */
#line 881 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = (yyvsp[(1) - (2)].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[(2) - (2)].Attr_val));
		}
    break;

  case 152:
/* Line 1787 of yacc.c  */
#line 886 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = NULL;
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[(1) - (1)].Attr_val));
		}
    break;

  case 153:
/* Line 1787 of yacc.c  */
#line 894 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val) = create_attr_ival((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Integer)); }
    break;

  case 157:
/* Line 1787 of yacc.c  */
#line 905 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = (yyvsp[(1) - (2)].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[(2) - (2)].Attr_val));
		}
    break;

  case 158:
/* Line 1787 of yacc.c  */
#line 910 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = NULL;
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[(1) - (1)].Attr_val));
		}
    break;

  case 159:
/* Line 1787 of yacc.c  */
#line 918 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val) = create_attr_ival((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Integer)); }
    break;

  case 168:
/* Line 1787 of yacc.c  */
#line 938 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			addr_opts_node *aon;
			
			aon = create_addr_opts_node((yyvsp[(2) - (3)].Address_node), (yyvsp[(3) - (3)].Attr_val_fifo));
			APPEND_G_FIFO(cfgt.fudge, aon);
		}
    break;

  case 169:
/* Line 1787 of yacc.c  */
#line 948 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = (yyvsp[(1) - (2)].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[(2) - (2)].Attr_val));
		}
    break;

  case 170:
/* Line 1787 of yacc.c  */
#line 953 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = NULL;
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[(1) - (1)].Attr_val));
		}
    break;

  case 171:
/* Line 1787 of yacc.c  */
#line 961 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val) = create_attr_dval((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Double)); }
    break;

  case 172:
/* Line 1787 of yacc.c  */
#line 963 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val) = create_attr_ival((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Integer)); }
    break;

  case 173:
/* Line 1787 of yacc.c  */
#line 965 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val) = create_attr_ival((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Integer)); }
    break;

  case 174:
/* Line 1787 of yacc.c  */
#line 967 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val) = create_attr_sval((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].String)); }
    break;

  case 175:
/* Line 1787 of yacc.c  */
#line 969 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val) = create_attr_sval((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].String)); }
    break;

  case 182:
/* Line 1787 of yacc.c  */
#line 990 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { CONCAT_G_FIFOS(cfgt.rlimit, (yyvsp[(2) - (2)].Attr_val_fifo)); }
    break;

  case 183:
/* Line 1787 of yacc.c  */
#line 995 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = (yyvsp[(1) - (2)].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[(2) - (2)].Attr_val));
		}
    break;

  case 184:
/* Line 1787 of yacc.c  */
#line 1000 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = NULL;
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[(1) - (1)].Attr_val));
		}
    break;

  case 185:
/* Line 1787 of yacc.c  */
#line 1008 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val) = create_attr_ival((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Integer)); }
    break;

  case 189:
/* Line 1787 of yacc.c  */
#line 1024 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { CONCAT_G_FIFOS(cfgt.enable_opts, (yyvsp[(2) - (2)].Attr_val_fifo)); }
    break;

  case 190:
/* Line 1787 of yacc.c  */
#line 1026 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { CONCAT_G_FIFOS(cfgt.disable_opts, (yyvsp[(2) - (2)].Attr_val_fifo)); }
    break;

  case 191:
/* Line 1787 of yacc.c  */
#line 1031 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = (yyvsp[(1) - (2)].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[(2) - (2)].Attr_val));
		}
    break;

  case 192:
/* Line 1787 of yacc.c  */
#line 1036 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = NULL;
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[(1) - (1)].Attr_val));
		}
    break;

  case 193:
/* Line 1787 of yacc.c  */
#line 1044 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val) = create_attr_ival(T_Flag, (yyvsp[(1) - (1)].Integer)); }
    break;

  case 194:
/* Line 1787 of yacc.c  */
#line 1046 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { 
			if (input_from_file) {
				(yyval.Attr_val) = create_attr_ival(T_Flag, (yyvsp[(1) - (1)].Integer));
			} else {
				char err_str[128];
				
				(yyval.Attr_val) = NULL;
				snprintf(err_str, sizeof(err_str),
					 "enable/disable %s remote configuration ignored",
					 keyword((yyvsp[(1) - (1)].Integer)));
				yyerror(err_str);
			}
		}
    break;

  case 203:
/* Line 1787 of yacc.c  */
#line 1081 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { CONCAT_G_FIFOS(cfgt.tinker, (yyvsp[(2) - (2)].Attr_val_fifo)); }
    break;

  case 204:
/* Line 1787 of yacc.c  */
#line 1086 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = (yyvsp[(1) - (2)].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[(2) - (2)].Attr_val));
		}
    break;

  case 205:
/* Line 1787 of yacc.c  */
#line 1091 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = NULL;
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[(1) - (1)].Attr_val));
		}
    break;

  case 206:
/* Line 1787 of yacc.c  */
#line 1099 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val) = create_attr_dval((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Double)); }
    break;

  case 217:
/* Line 1787 of yacc.c  */
#line 1122 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			attr_val *av;
			
			av = create_attr_dval((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Double));
			APPEND_G_FIFO(cfgt.vars, av);
		}
    break;

  case 218:
/* Line 1787 of yacc.c  */
#line 1129 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			attr_val *av;
			
			av = create_attr_sval((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].String));
			APPEND_G_FIFO(cfgt.vars, av);
		}
    break;

  case 219:
/* Line 1787 of yacc.c  */
#line 1136 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			char error_text[64];
			attr_val *av;

			if (input_from_file) {
				av = create_attr_sval((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].String));
				APPEND_G_FIFO(cfgt.vars, av);
			} else {
				YYFREE((yyvsp[(2) - (2)].String));
				snprintf(error_text, sizeof(error_text),
					 "%s remote config ignored",
					 keyword((yyvsp[(1) - (2)].Integer)));
				yyerror(error_text);
			}
		}
    break;

  case 220:
/* Line 1787 of yacc.c  */
#line 1152 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			if (!input_from_file) {
				yyerror("remote includefile ignored");
				break;
			}
			if (curr_include_level >= MAXINCLUDELEVEL) {
				fprintf(stderr, "getconfig: Maximum include file level exceeded.\n");
				msyslog(LOG_ERR, "getconfig: Maximum include file level exceeded.");
			} else {
				fp[curr_include_level + 1] = F_OPEN(FindConfig((yyvsp[(2) - (3)].String)), "r");
				if (fp[curr_include_level + 1] == NULL) {
					fprintf(stderr, "getconfig: Couldn't open <%s>\n", FindConfig((yyvsp[(2) - (3)].String)));
					msyslog(LOG_ERR, "getconfig: Couldn't open <%s>", FindConfig((yyvsp[(2) - (3)].String)));
				} else {
					ip_file = fp[++curr_include_level];
				}
			}
		}
    break;

  case 221:
/* Line 1787 of yacc.c  */
#line 1171 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			while (curr_include_level != -1)
				FCLOSE(fp[curr_include_level--]);
		}
    break;

  case 222:
/* Line 1787 of yacc.c  */
#line 1176 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { /* see drift_parm below for actions */ }
    break;

  case 223:
/* Line 1787 of yacc.c  */
#line 1178 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { CONCAT_G_FIFOS(cfgt.logconfig, (yyvsp[(2) - (2)].Attr_val_fifo)); }
    break;

  case 224:
/* Line 1787 of yacc.c  */
#line 1180 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { CONCAT_G_FIFOS(cfgt.phone, (yyvsp[(2) - (2)].String_fifo)); }
    break;

  case 225:
/* Line 1787 of yacc.c  */
#line 1182 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { APPEND_G_FIFO(cfgt.setvar, (yyvsp[(2) - (2)].Set_var)); }
    break;

  case 226:
/* Line 1787 of yacc.c  */
#line 1184 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			addr_opts_node *aon;
			
			aon = create_addr_opts_node((yyvsp[(2) - (3)].Address_node), (yyvsp[(3) - (3)].Attr_val_fifo));
			APPEND_G_FIFO(cfgt.trap, aon);
		}
    break;

  case 227:
/* Line 1787 of yacc.c  */
#line 1191 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { CONCAT_G_FIFOS(cfgt.ttl, (yyvsp[(2) - (2)].Attr_val_fifo)); }
    break;

  case 236:
/* Line 1787 of yacc.c  */
#line 1213 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			attr_val *av;
			
			av = create_attr_sval(T_Driftfile, (yyvsp[(1) - (1)].String));
			APPEND_G_FIFO(cfgt.vars, av);
		}
    break;

  case 237:
/* Line 1787 of yacc.c  */
#line 1220 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			attr_val *av;
			
			av = create_attr_sval(T_Driftfile, (yyvsp[(1) - (2)].String));
			APPEND_G_FIFO(cfgt.vars, av);
			av = create_attr_dval(T_WanderThreshold, (yyvsp[(2) - (2)].Double));
			APPEND_G_FIFO(cfgt.vars, av);
		}
    break;

  case 238:
/* Line 1787 of yacc.c  */
#line 1229 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			attr_val *av;
			
			av = create_attr_sval(T_Driftfile, "");
			APPEND_G_FIFO(cfgt.vars, av);
		}
    break;

  case 239:
/* Line 1787 of yacc.c  */
#line 1239 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Set_var) = create_setvar_node((yyvsp[(1) - (4)].String), (yyvsp[(3) - (4)].String), (yyvsp[(4) - (4)].Integer)); }
    break;

  case 241:
/* Line 1787 of yacc.c  */
#line 1245 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Integer) = 0; }
    break;

  case 242:
/* Line 1787 of yacc.c  */
#line 1250 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val_fifo) = NULL; }
    break;

  case 243:
/* Line 1787 of yacc.c  */
#line 1252 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = (yyvsp[(1) - (2)].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[(2) - (2)].Attr_val));
		}
    break;

  case 244:
/* Line 1787 of yacc.c  */
#line 1260 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val) = create_attr_ival((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Integer)); }
    break;

  case 245:
/* Line 1787 of yacc.c  */
#line 1262 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val) = create_attr_sval((yyvsp[(1) - (2)].Integer), estrdup((yyvsp[(2) - (2)].Address_node)->address));
			destroy_address_node((yyvsp[(2) - (2)].Address_node));
		}
    break;

  case 246:
/* Line 1787 of yacc.c  */
#line 1270 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = (yyvsp[(1) - (2)].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[(2) - (2)].Attr_val));
		}
    break;

  case 247:
/* Line 1787 of yacc.c  */
#line 1275 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = NULL;
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[(1) - (1)].Attr_val));
		}
    break;

  case 248:
/* Line 1787 of yacc.c  */
#line 1283 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			char	prefix;
			char *	type;
			
			switch ((yyvsp[(1) - (1)].String)[0]) {
			
			case '+':
			case '-':
			case '=':
				prefix = (yyvsp[(1) - (1)].String)[0];
				type = (yyvsp[(1) - (1)].String) + 1;
				break;
				
			default:
				prefix = '=';
				type = (yyvsp[(1) - (1)].String);
			}	
			
			(yyval.Attr_val) = create_attr_sval(prefix, estrdup(type));
			YYFREE((yyvsp[(1) - (1)].String));
		}
    break;

  case 249:
/* Line 1787 of yacc.c  */
#line 1308 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			nic_rule_node *nrn;
			
			nrn = create_nic_rule_node((yyvsp[(3) - (3)].Integer), NULL, (yyvsp[(2) - (3)].Integer));
			APPEND_G_FIFO(cfgt.nic_rules, nrn);
		}
    break;

  case 250:
/* Line 1787 of yacc.c  */
#line 1315 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			nic_rule_node *nrn;
			
			nrn = create_nic_rule_node(0, (yyvsp[(3) - (3)].String), (yyvsp[(2) - (3)].Integer));
			APPEND_G_FIFO(cfgt.nic_rules, nrn);
		}
    break;

  case 260:
/* Line 1787 of yacc.c  */
#line 1343 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { CONCAT_G_FIFOS(cfgt.reset_counters, (yyvsp[(2) - (2)].Int_fifo)); }
    break;

  case 261:
/* Line 1787 of yacc.c  */
#line 1348 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Int_fifo) = (yyvsp[(1) - (2)].Int_fifo);
			APPEND_G_FIFO((yyval.Int_fifo), create_int_node((yyvsp[(2) - (2)].Integer)));
		}
    break;

  case 262:
/* Line 1787 of yacc.c  */
#line 1353 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Int_fifo) = NULL;
			APPEND_G_FIFO((yyval.Int_fifo), create_int_node((yyvsp[(1) - (1)].Integer)));
		}
    break;

  case 270:
/* Line 1787 of yacc.c  */
#line 1377 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = (yyvsp[(1) - (2)].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), create_int_node((yyvsp[(2) - (2)].Integer)));
		}
    break;

  case 271:
/* Line 1787 of yacc.c  */
#line 1382 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = NULL;
			APPEND_G_FIFO((yyval.Attr_val_fifo), create_int_node((yyvsp[(1) - (1)].Integer)));
		}
    break;

  case 272:
/* Line 1787 of yacc.c  */
#line 1390 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = (yyvsp[(1) - (2)].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[(2) - (2)].Attr_val));
		}
    break;

  case 273:
/* Line 1787 of yacc.c  */
#line 1395 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = NULL;
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[(1) - (1)].Attr_val));
		}
    break;

  case 274:
/* Line 1787 of yacc.c  */
#line 1403 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val) = create_attr_ival('i', (yyvsp[(1) - (1)].Integer)); }
    break;

  case 276:
/* Line 1787 of yacc.c  */
#line 1409 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val) = create_attr_rangeval('-', (yyvsp[(2) - (5)].Integer), (yyvsp[(4) - (5)].Integer)); }
    break;

  case 277:
/* Line 1787 of yacc.c  */
#line 1414 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.String_fifo) = (yyvsp[(1) - (2)].String_fifo);
			APPEND_G_FIFO((yyval.String_fifo), create_string_node((yyvsp[(2) - (2)].String)));
		}
    break;

  case 278:
/* Line 1787 of yacc.c  */
#line 1419 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.String_fifo) = NULL;
			APPEND_G_FIFO((yyval.String_fifo), create_string_node((yyvsp[(1) - (1)].String)));
		}
    break;

  case 279:
/* Line 1787 of yacc.c  */
#line 1427 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Address_fifo) = (yyvsp[(1) - (2)].Address_fifo);
			APPEND_G_FIFO((yyval.Address_fifo), (yyvsp[(2) - (2)].Address_node));
		}
    break;

  case 280:
/* Line 1787 of yacc.c  */
#line 1432 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Address_fifo) = NULL;
			APPEND_G_FIFO((yyval.Address_fifo), (yyvsp[(1) - (1)].Address_node));
		}
    break;

  case 281:
/* Line 1787 of yacc.c  */
#line 1440 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			if ((yyvsp[(1) - (1)].Integer) != 0 && (yyvsp[(1) - (1)].Integer) != 1) {
				yyerror("Integer value is not boolean (0 or 1). Assuming 1");
				(yyval.Integer) = 1;
			} else {
				(yyval.Integer) = (yyvsp[(1) - (1)].Integer);
			}
		}
    break;

  case 282:
/* Line 1787 of yacc.c  */
#line 1448 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Integer) = 1; }
    break;

  case 283:
/* Line 1787 of yacc.c  */
#line 1449 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Integer) = 0; }
    break;

  case 284:
/* Line 1787 of yacc.c  */
#line 1453 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Double) = (double)(yyvsp[(1) - (1)].Integer); }
    break;

  case 286:
/* Line 1787 of yacc.c  */
#line 1464 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			sim_node *sn;
			
			sn =  create_sim_node((yyvsp[(3) - (5)].Attr_val_fifo), (yyvsp[(4) - (5)].Sim_server_fifo));
			APPEND_G_FIFO(cfgt.sim_details, sn);

			/* Revert from ; to \n for end-of-command */
			old_config_style = 1;
		}
    break;

  case 287:
/* Line 1787 of yacc.c  */
#line 1481 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { old_config_style = 0; }
    break;

  case 288:
/* Line 1787 of yacc.c  */
#line 1486 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = (yyvsp[(1) - (3)].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[(2) - (3)].Attr_val));
		}
    break;

  case 289:
/* Line 1787 of yacc.c  */
#line 1491 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = NULL;
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[(1) - (2)].Attr_val));
		}
    break;

  case 290:
/* Line 1787 of yacc.c  */
#line 1499 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val) = create_attr_dval((yyvsp[(1) - (3)].Integer), (yyvsp[(3) - (3)].Double)); }
    break;

  case 293:
/* Line 1787 of yacc.c  */
#line 1509 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Sim_server_fifo) = (yyvsp[(1) - (2)].Sim_server_fifo);
			APPEND_G_FIFO((yyval.Sim_server_fifo), (yyvsp[(2) - (2)].Sim_server));
		}
    break;

  case 294:
/* Line 1787 of yacc.c  */
#line 1514 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Sim_server_fifo) = NULL;
			APPEND_G_FIFO((yyval.Sim_server_fifo), (yyvsp[(1) - (1)].Sim_server));
		}
    break;

  case 295:
/* Line 1787 of yacc.c  */
#line 1522 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Sim_server) = create_sim_server((yyvsp[(1) - (5)].Address_node), (yyvsp[(3) - (5)].Double), (yyvsp[(4) - (5)].Sim_script_fifo)); }
    break;

  case 296:
/* Line 1787 of yacc.c  */
#line 1527 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Double) = (yyvsp[(3) - (4)].Double); }
    break;

  case 297:
/* Line 1787 of yacc.c  */
#line 1532 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Address_node) = (yyvsp[(3) - (3)].Address_node); }
    break;

  case 298:
/* Line 1787 of yacc.c  */
#line 1537 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Sim_script_fifo) = (yyvsp[(1) - (2)].Sim_script_fifo);
			APPEND_G_FIFO((yyval.Sim_script_fifo), (yyvsp[(2) - (2)].Sim_script));
		}
    break;

  case 299:
/* Line 1787 of yacc.c  */
#line 1542 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Sim_script_fifo) = NULL;
			APPEND_G_FIFO((yyval.Sim_script_fifo), (yyvsp[(1) - (1)].Sim_script));
		}
    break;

  case 300:
/* Line 1787 of yacc.c  */
#line 1550 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Sim_script) = create_sim_script_info((yyvsp[(3) - (6)].Double), (yyvsp[(5) - (6)].Attr_val_fifo)); }
    break;

  case 301:
/* Line 1787 of yacc.c  */
#line 1555 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = (yyvsp[(1) - (3)].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[(2) - (3)].Attr_val));
		}
    break;

  case 302:
/* Line 1787 of yacc.c  */
#line 1560 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    {
			(yyval.Attr_val_fifo) = NULL;
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[(1) - (2)].Attr_val));
		}
    break;

  case 303:
/* Line 1787 of yacc.c  */
#line 1568 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
    { (yyval.Attr_val) = create_attr_dval((yyvsp[(1) - (3)].Integer), (yyvsp[(3) - (3)].Double)); }
    break;


/* Line 1787 of yacc.c  */
#line 3396 "ntp_parser.c"
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
  /* Do not reclaim the symbols of the rule which action triggered
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
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


/* Line 2048 of yacc.c  */
#line 1579 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"


void 
yyerror(
	const char *msg
	)
{
	int retval;

	ip_file->err_line_no = ip_file->prev_token_line_no;
	ip_file->err_col_no = ip_file->prev_token_col_no;
	
	msyslog(LOG_ERR, 
		"line %d column %d %s", 
		ip_file->err_line_no,
		ip_file->err_col_no,
		msg);
	if (!input_from_file) {
		/* Save the error message in the correct buffer */
		retval = snprintf(remote_config.err_msg + remote_config.err_pos,
				  MAXLINE - remote_config.err_pos,
				  "column %d %s",
				  ip_file->err_col_no, msg);

		/* Increment the value of err_pos */
		if (retval > 0)
			remote_config.err_pos += retval;

		/* Increment the number of errors */
		++remote_config.no_errors;
	}
}


/*
 * token_name - convert T_ token integers to text
 *		example: token_name(T_Server) returns "T_Server"
 */
const char *
token_name(
	int token
	)
{
	return yytname[YYTRANSLATE(token)];
}


/* Initial Testing function -- ignore */
#if 0
int main(int argc, char *argv[])
{
	ip_file = FOPEN(argv[1], "r");
	if (!ip_file)
		fprintf(stderr, "ERROR!! Could not open file: %s\n", argv[1]);
	yyparse();
	return 0;
}
#endif

