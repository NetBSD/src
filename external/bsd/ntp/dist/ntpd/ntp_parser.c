/* A Bison parser, made by GNU Bison 2.4.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006,
   2009, 2010 Free Software Foundation, Inc.
   
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
#define YYBISON_VERSION "2.4.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 13 "ntp_parser.y"

  #ifdef HAVE_CONFIG_H
  # include <config.h>
  #endif

  #include "ntp.h"
  #include "ntpd.h"
  #include "ntp_machine.h"
  #include "ntp_stdlib.h"
  #include "ntp_filegen.h"
  #include "ntp_data_structures.h"
  #include "ntp_scanner.h"
  #include "ntp_config.h"
  #include "ntp_crypto.h"

  #include "ntpsim.h"		/* HMS: Do we really want this all the time? */
				/* SK: It might be a good idea to always
				   include the simulator code. That way
				   someone can use the same configuration file
				   for both the simulator and the daemon
				*/


  struct FILE_INFO *ip_file;   /* Pointer to the configuration file stream */

  #define YYMALLOC	emalloc
  #define YYFREE	free
  #define YYERROR_VERBOSE
  #define YYMAXDEPTH	1000   /* stop the madness sooner */
  void yyerror(const char *msg);
  extern int input_from_file;  /* 0=input from ntpq :config */


/* Line 189 of yacc.c  */
#line 106 "ntp_parser.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
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
# define YYTOKEN_TABLE 1
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     T_Age = 258,
     T_All = 259,
     T_Allan = 260,
     T_Auth = 261,
     T_Autokey = 262,
     T_Automax = 263,
     T_Average = 264,
     T_Bclient = 265,
     T_Beacon = 266,
     T_Bias = 267,
     T_Broadcast = 268,
     T_Broadcastclient = 269,
     T_Broadcastdelay = 270,
     T_Burst = 271,
     T_Calibrate = 272,
     T_Calldelay = 273,
     T_Ceiling = 274,
     T_Clockstats = 275,
     T_Cohort = 276,
     T_ControlKey = 277,
     T_Crypto = 278,
     T_Cryptostats = 279,
     T_Day = 280,
     T_Default = 281,
     T_Digest = 282,
     T_Disable = 283,
     T_Discard = 284,
     T_Dispersion = 285,
     T_Double = 286,
     T_Driftfile = 287,
     T_Drop = 288,
     T_Ellipsis = 289,
     T_Enable = 290,
     T_End = 291,
     T_False = 292,
     T_File = 293,
     T_Filegen = 294,
     T_Flag1 = 295,
     T_Flag2 = 296,
     T_Flag3 = 297,
     T_Flag4 = 298,
     T_Flake = 299,
     T_Floor = 300,
     T_Freq = 301,
     T_Fudge = 302,
     T_Host = 303,
     T_Huffpuff = 304,
     T_Iburst = 305,
     T_Ident = 306,
     T_Ignore = 307,
     T_Includefile = 308,
     T_Integer = 309,
     T_Interface = 310,
     T_Ipv4 = 311,
     T_Ipv4_flag = 312,
     T_Ipv6 = 313,
     T_Ipv6_flag = 314,
     T_Kernel = 315,
     T_Key = 316,
     T_Keys = 317,
     T_Keysdir = 318,
     T_Kod = 319,
     T_Mssntp = 320,
     T_Leapfile = 321,
     T_Limited = 322,
     T_Link = 323,
     T_Listen = 324,
     T_Logconfig = 325,
     T_Logfile = 326,
     T_Loopstats = 327,
     T_Lowpriotrap = 328,
     T_Manycastclient = 329,
     T_Manycastserver = 330,
     T_Mask = 331,
     T_Maxclock = 332,
     T_Maxdist = 333,
     T_Maxpoll = 334,
     T_Mdnstries = 335,
     T_Minclock = 336,
     T_Mindist = 337,
     T_Minimum = 338,
     T_Minpoll = 339,
     T_Minsane = 340,
     T_Mode = 341,
     T_Monitor = 342,
     T_Month = 343,
     T_Multicastclient = 344,
     T_Nic = 345,
     T_Nolink = 346,
     T_Nomodify = 347,
     T_None = 348,
     T_Nopeer = 349,
     T_Noquery = 350,
     T_Noselect = 351,
     T_Noserve = 352,
     T_Notrap = 353,
     T_Notrust = 354,
     T_Ntp = 355,
     T_Ntpport = 356,
     T_NtpSignDsocket = 357,
     T_Orphan = 358,
     T_Panic = 359,
     T_Peer = 360,
     T_Peerstats = 361,
     T_Phone = 362,
     T_Pid = 363,
     T_Pidfile = 364,
     T_Pool = 365,
     T_Port = 366,
     T_Preempt = 367,
     T_Prefer = 368,
     T_Protostats = 369,
     T_Pw = 370,
     T_Qos = 371,
     T_Randfile = 372,
     T_Rawstats = 373,
     T_Refid = 374,
     T_Requestkey = 375,
     T_Restrict = 376,
     T_Revoke = 377,
     T_Saveconfigdir = 378,
     T_Server = 379,
     T_Setvar = 380,
     T_Sign = 381,
     T_Statistics = 382,
     T_Stats = 383,
     T_Statsdir = 384,
     T_Step = 385,
     T_Stepout = 386,
     T_Stratum = 387,
     T_String = 388,
     T_Sysstats = 389,
     T_Tick = 390,
     T_Time1 = 391,
     T_Time2 = 392,
     T_Timingstats = 393,
     T_Tinker = 394,
     T_Tos = 395,
     T_Trap = 396,
     T_True = 397,
     T_Trustedkey = 398,
     T_Ttl = 399,
     T_Type = 400,
     T_Unconfig = 401,
     T_Unpeer = 402,
     T_Version = 403,
     T_WanderThreshold = 404,
     T_Week = 405,
     T_Wildcard = 406,
     T_Xleave = 407,
     T_Year = 408,
     T_Flag = 409,
     T_Void = 410,
     T_EOC = 411,
     T_Simulate = 412,
     T_Beep_Delay = 413,
     T_Sim_Duration = 414,
     T_Server_Offset = 415,
     T_Duration = 416,
     T_Freq_Offset = 417,
     T_Wander = 418,
     T_Jitter = 419,
     T_Prop_Delay = 420,
     T_Proc_Delay = 421
   };
#endif
/* Tokens.  */
#define T_Age 258
#define T_All 259
#define T_Allan 260
#define T_Auth 261
#define T_Autokey 262
#define T_Automax 263
#define T_Average 264
#define T_Bclient 265
#define T_Beacon 266
#define T_Bias 267
#define T_Broadcast 268
#define T_Broadcastclient 269
#define T_Broadcastdelay 270
#define T_Burst 271
#define T_Calibrate 272
#define T_Calldelay 273
#define T_Ceiling 274
#define T_Clockstats 275
#define T_Cohort 276
#define T_ControlKey 277
#define T_Crypto 278
#define T_Cryptostats 279
#define T_Day 280
#define T_Default 281
#define T_Digest 282
#define T_Disable 283
#define T_Discard 284
#define T_Dispersion 285
#define T_Double 286
#define T_Driftfile 287
#define T_Drop 288
#define T_Ellipsis 289
#define T_Enable 290
#define T_End 291
#define T_False 292
#define T_File 293
#define T_Filegen 294
#define T_Flag1 295
#define T_Flag2 296
#define T_Flag3 297
#define T_Flag4 298
#define T_Flake 299
#define T_Floor 300
#define T_Freq 301
#define T_Fudge 302
#define T_Host 303
#define T_Huffpuff 304
#define T_Iburst 305
#define T_Ident 306
#define T_Ignore 307
#define T_Includefile 308
#define T_Integer 309
#define T_Interface 310
#define T_Ipv4 311
#define T_Ipv4_flag 312
#define T_Ipv6 313
#define T_Ipv6_flag 314
#define T_Kernel 315
#define T_Key 316
#define T_Keys 317
#define T_Keysdir 318
#define T_Kod 319
#define T_Mssntp 320
#define T_Leapfile 321
#define T_Limited 322
#define T_Link 323
#define T_Listen 324
#define T_Logconfig 325
#define T_Logfile 326
#define T_Loopstats 327
#define T_Lowpriotrap 328
#define T_Manycastclient 329
#define T_Manycastserver 330
#define T_Mask 331
#define T_Maxclock 332
#define T_Maxdist 333
#define T_Maxpoll 334
#define T_Mdnstries 335
#define T_Minclock 336
#define T_Mindist 337
#define T_Minimum 338
#define T_Minpoll 339
#define T_Minsane 340
#define T_Mode 341
#define T_Monitor 342
#define T_Month 343
#define T_Multicastclient 344
#define T_Nic 345
#define T_Nolink 346
#define T_Nomodify 347
#define T_None 348
#define T_Nopeer 349
#define T_Noquery 350
#define T_Noselect 351
#define T_Noserve 352
#define T_Notrap 353
#define T_Notrust 354
#define T_Ntp 355
#define T_Ntpport 356
#define T_NtpSignDsocket 357
#define T_Orphan 358
#define T_Panic 359
#define T_Peer 360
#define T_Peerstats 361
#define T_Phone 362
#define T_Pid 363
#define T_Pidfile 364
#define T_Pool 365
#define T_Port 366
#define T_Preempt 367
#define T_Prefer 368
#define T_Protostats 369
#define T_Pw 370
#define T_Qos 371
#define T_Randfile 372
#define T_Rawstats 373
#define T_Refid 374
#define T_Requestkey 375
#define T_Restrict 376
#define T_Revoke 377
#define T_Saveconfigdir 378
#define T_Server 379
#define T_Setvar 380
#define T_Sign 381
#define T_Statistics 382
#define T_Stats 383
#define T_Statsdir 384
#define T_Step 385
#define T_Stepout 386
#define T_Stratum 387
#define T_String 388
#define T_Sysstats 389
#define T_Tick 390
#define T_Time1 391
#define T_Time2 392
#define T_Timingstats 393
#define T_Tinker 394
#define T_Tos 395
#define T_Trap 396
#define T_True 397
#define T_Trustedkey 398
#define T_Ttl 399
#define T_Type 400
#define T_Unconfig 401
#define T_Unpeer 402
#define T_Version 403
#define T_WanderThreshold 404
#define T_Week 405
#define T_Wildcard 406
#define T_Xleave 407
#define T_Year 408
#define T_Flag 409
#define T_Void 410
#define T_EOC 411
#define T_Simulate 412
#define T_Beep_Delay 413
#define T_Sim_Duration 414
#define T_Server_Offset 415
#define T_Duration 416
#define T_Freq_Offset 417
#define T_Wander 418
#define T_Jitter 419
#define T_Prop_Delay 420
#define T_Proc_Delay 421




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 52 "ntp_parser.y"

    char   *String;
    double  Double;
    int     Integer;
    void   *VoidPtr;
    queue  *Queue;
    struct attr_val *Attr_val;
    struct address_node *Address_node;
    struct setvar_node *Set_var;

    /* Simulation types */
    server_info *Sim_server;
    script_info *Sim_script;



/* Line 214 of yacc.c  */
#line 491 "ntp_parser.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 503 "ntp_parser.c"

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
#define YYFINAL  170
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   646

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  172
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  89
/* YYNRULES -- Number of rules.  */
#define YYNRULES  268
/* YYNRULES -- Number of states.  */
#define YYNSTATES  377

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   421

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     168,   169,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   167,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   170,     2,   171,     2,     2,     2,     2,
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
     165,   166
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     9,    12,    15,    16,    18,    20,
      22,    24,    26,    28,    30,    32,    34,    36,    38,    40,
      44,    47,    49,    51,    53,    55,    57,    59,    62,    64,
      66,    68,    71,    73,    75,    77,    80,    82,    84,    86,
      88,    90,    92,    94,    96,    98,   101,   103,   105,   107,
     109,   111,   113,   116,   118,   120,   122,   125,   128,   131,
     134,   137,   140,   143,   146,   149,   152,   155,   158,   159,
     162,   165,   168,   170,   172,   174,   176,   178,   180,   183,
     186,   188,   191,   194,   197,   199,   201,   203,   205,   207,
     209,   211,   213,   215,   218,   221,   225,   228,   230,   232,
     234,   236,   238,   240,   242,   244,   246,   247,   250,   253,
     256,   258,   260,   262,   264,   266,   268,   270,   272,   274,
     276,   278,   280,   282,   285,   289,   293,   298,   303,   309,
     310,   313,   315,   317,   319,   321,   323,   325,   327,   329,
     331,   333,   335,   337,   339,   341,   344,   346,   349,   351,
     353,   355,   359,   362,   364,   367,   370,   373,   376,   378,
     380,   382,   384,   386,   388,   391,   394,   397,   399,   401,
     403,   405,   407,   409,   411,   413,   415,   418,   421,   423,
     426,   428,   430,   432,   434,   436,   438,   440,   442,   445,
     448,   451,   455,   457,   460,   463,   466,   469,   472,   475,
     479,   482,   484,   486,   488,   490,   492,   494,   496,   498,
     501,   502,   507,   509,   510,   513,   515,   518,   521,   524,
     526,   528,   532,   536,   538,   540,   542,   544,   546,   548,
     550,   552,   554,   557,   559,   562,   564,   566,   568,   574,
     577,   579,   582,   584,   586,   588,   590,   592,   594,   600,
     602,   606,   609,   613,   617,   620,   622,   628,   633,   637,
     640,   642,   649,   653,   656,   660,   664,   668,   672
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     173,     0,    -1,   174,    -1,   174,   175,   156,    -1,   175,
     156,    -1,     1,   156,    -1,    -1,   176,    -1,   187,    -1,
     189,    -1,   190,    -1,   199,    -1,   207,    -1,   194,    -1,
     213,    -1,   218,    -1,   222,    -1,   226,    -1,   249,    -1,
     177,   178,   181,    -1,   177,   178,    -1,   124,    -1,   110,
      -1,   105,    -1,    13,    -1,    74,    -1,   179,    -1,   180,
     133,    -1,   133,    -1,    57,    -1,    59,    -1,   181,   182,
      -1,   182,    -1,   183,    -1,   185,    -1,    12,   248,    -1,
     184,    -1,     7,    -1,    16,    -1,    50,    -1,    96,    -1,
     112,    -1,   113,    -1,   142,    -1,   152,    -1,   186,    54,
      -1,    61,    -1,    84,    -1,    79,    -1,   144,    -1,    86,
      -1,   148,    -1,   188,   178,    -1,   146,    -1,   147,    -1,
      14,    -1,    75,   246,    -1,    89,   246,    -1,    80,    54,
      -1,     8,    54,    -1,    22,    54,    -1,    23,   191,    -1,
      62,   133,    -1,    63,   133,    -1,   120,    54,    -1,   122,
      54,    -1,   143,   242,    -1,   102,   133,    -1,    -1,   191,
     192,    -1,   193,   133,    -1,   122,    54,    -1,    48,    -1,
      51,    -1,   115,    -1,   117,    -1,   126,    -1,    27,    -1,
     140,   195,    -1,   195,   196,    -1,   196,    -1,   197,    54,
      -1,   198,   248,    -1,    21,   247,    -1,    19,    -1,    45,
      -1,   103,    -1,    85,    -1,    11,    -1,    82,    -1,    78,
      -1,    81,    -1,    77,    -1,   127,   200,    -1,   129,   133,
      -1,    39,   201,   202,    -1,   200,   201,    -1,   201,    -1,
      20,    -1,    24,    -1,    72,    -1,   106,    -1,   118,    -1,
     134,    -1,   138,    -1,   114,    -1,    -1,   202,   203,    -1,
      38,   133,    -1,   145,   206,    -1,   204,    -1,   205,    -1,
      68,    -1,    91,    -1,    35,    -1,    28,    -1,    93,    -1,
     108,    -1,    25,    -1,   150,    -1,    88,    -1,   153,    -1,
       3,    -1,    29,   210,    -1,   121,   178,   208,    -1,   121,
      26,   208,    -1,   121,    57,    26,   208,    -1,   121,    59,
      26,   208,    -1,   121,   179,    76,   179,   208,    -1,    -1,
     208,   209,    -1,    44,    -1,    52,    -1,    64,    -1,    65,
      -1,    67,    -1,    73,    -1,    92,    -1,    94,    -1,    95,
      -1,    97,    -1,    98,    -1,    99,    -1,   101,    -1,   148,
      -1,   210,   211,    -1,   211,    -1,   212,    54,    -1,     9,
      -1,    83,    -1,    87,    -1,    47,   178,   214,    -1,   214,
     215,    -1,   215,    -1,   216,   248,    -1,   217,   247,    -1,
     132,    54,    -1,   119,   133,    -1,   136,    -1,   137,    -1,
      40,    -1,    41,    -1,    42,    -1,    43,    -1,    35,   219,
      -1,    28,   219,    -1,   219,   220,    -1,   220,    -1,   221,
      -1,   128,    -1,     6,    -1,    10,    -1,    17,    -1,    60,
      -1,    87,    -1,   100,    -1,   139,   223,    -1,   223,   224,
      -1,   224,    -1,   225,   248,    -1,     5,    -1,    30,    -1,
      46,    -1,    49,    -1,   104,    -1,   130,    -1,   131,    -1,
     237,    -1,   227,   248,    -1,   228,   133,    -1,   229,   133,
      -1,    53,   133,   175,    -1,    36,    -1,    18,    54,    -1,
      32,   230,    -1,    70,   235,    -1,   107,   245,    -1,   125,
     231,    -1,   141,   179,    -1,   141,   179,   233,    -1,   144,
     241,    -1,    15,    -1,   135,    -1,    66,    -1,   109,    -1,
     116,    -1,    71,    -1,   123,    -1,   133,    -1,   133,    31,
      -1,    -1,   133,   167,   133,   232,    -1,    26,    -1,    -1,
     233,   234,    -1,   234,    -1,   111,    54,    -1,    55,   179,
      -1,   235,   236,    -1,   236,    -1,   133,    -1,   238,   240,
     239,    -1,   238,   240,   133,    -1,    55,    -1,    90,    -1,
       4,    -1,    56,    -1,    58,    -1,   151,    -1,    69,    -1,
      52,    -1,    33,    -1,   241,    54,    -1,    54,    -1,   242,
     243,    -1,   243,    -1,    54,    -1,   244,    -1,   168,    54,
      34,    54,   169,    -1,   245,   133,    -1,   133,    -1,   246,
     178,    -1,   178,    -1,    54,    -1,   142,    -1,    37,    -1,
      54,    -1,    31,    -1,   250,   170,   251,   253,   171,    -1,
     157,    -1,   251,   252,   156,    -1,   252,   156,    -1,   158,
     167,   248,    -1,   159,   167,   248,    -1,   253,   254,    -1,
     254,    -1,   256,   170,   255,   257,   171,    -1,   160,   167,
     248,   156,    -1,   124,   167,   178,    -1,   257,   258,    -1,
     258,    -1,   161,   167,   248,   170,   259,   171,    -1,   259,
     260,   156,    -1,   260,   156,    -1,   162,   167,   248,    -1,
     163,   167,   248,    -1,   164,   167,   248,    -1,   165,   167,
     248,    -1,   166,   167,   248,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   321,   321,   325,   326,   327,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   361,
     367,   376,   377,   378,   379,   380,   384,   385,   390,   395,
     397,   402,   403,   407,   408,   409,   414,   419,   420,   421,
     422,   423,   424,   425,   426,   430,   435,   436,   437,   438,
     439,   440,   450,   458,   459,   469,   471,   473,   475,   486,
     488,   490,   495,   497,   499,   501,   503,   505,   511,   512,
     522,   524,   536,   537,   538,   539,   540,   541,   550,   555,
     556,   560,   562,   564,   569,   570,   571,   572,   573,   577,
     578,   579,   580,   589,   591,   600,   608,   609,   613,   614,
     615,   616,   617,   618,   619,   620,   625,   626,   636,   646,
     655,   670,   675,   676,   680,   681,   685,   686,   687,   688,
     689,   690,   691,   700,   704,   709,   714,   727,   740,   749,
     750,   755,   756,   757,   758,   759,   760,   761,   762,   763,
     764,   765,   766,   767,   768,   772,   774,   779,   784,   785,
     786,   795,   800,   802,   807,   809,   811,   813,   818,   819,
     823,   824,   825,   826,   835,   837,   842,   849,   859,   861,
     873,   874,   875,   876,   877,   878,   887,   891,   892,   896,
     901,   902,   903,   904,   905,   906,   907,   916,   917,   924,
     931,   947,   966,   971,   973,   975,   977,   979,   981,   983,
     985,   990,   991,   995,   996,   997,  1001,  1002,  1006,  1008,
    1012,  1016,  1021,  1023,  1027,  1029,  1033,  1034,  1038,  1039,
    1043,  1058,  1063,  1071,  1072,  1076,  1077,  1078,  1079,  1083,
    1084,  1085,  1095,  1096,  1100,  1102,  1107,  1109,  1113,  1118,
    1119,  1123,  1124,  1128,  1137,  1138,  1142,  1143,  1152,  1167,
    1171,  1172,  1176,  1177,  1181,  1182,  1186,  1191,  1195,  1199,
    1200,  1204,  1209,  1210,  1214,  1216,  1218,  1220,  1222
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "T_Age", "T_All", "T_Allan", "T_Auth",
  "T_Autokey", "T_Automax", "T_Average", "T_Bclient", "T_Beacon", "T_Bias",
  "T_Broadcast", "T_Broadcastclient", "T_Broadcastdelay", "T_Burst",
  "T_Calibrate", "T_Calldelay", "T_Ceiling", "T_Clockstats", "T_Cohort",
  "T_ControlKey", "T_Crypto", "T_Cryptostats", "T_Day", "T_Default",
  "T_Digest", "T_Disable", "T_Discard", "T_Dispersion", "T_Double",
  "T_Driftfile", "T_Drop", "T_Ellipsis", "T_Enable", "T_End", "T_False",
  "T_File", "T_Filegen", "T_Flag1", "T_Flag2", "T_Flag3", "T_Flag4",
  "T_Flake", "T_Floor", "T_Freq", "T_Fudge", "T_Host", "T_Huffpuff",
  "T_Iburst", "T_Ident", "T_Ignore", "T_Includefile", "T_Integer",
  "T_Interface", "T_Ipv4", "T_Ipv4_flag", "T_Ipv6", "T_Ipv6_flag",
  "T_Kernel", "T_Key", "T_Keys", "T_Keysdir", "T_Kod", "T_Mssntp",
  "T_Leapfile", "T_Limited", "T_Link", "T_Listen", "T_Logconfig",
  "T_Logfile", "T_Loopstats", "T_Lowpriotrap", "T_Manycastclient",
  "T_Manycastserver", "T_Mask", "T_Maxclock", "T_Maxdist", "T_Maxpoll",
  "T_Mdnstries", "T_Minclock", "T_Mindist", "T_Minimum", "T_Minpoll",
  "T_Minsane", "T_Mode", "T_Monitor", "T_Month", "T_Multicastclient",
  "T_Nic", "T_Nolink", "T_Nomodify", "T_None", "T_Nopeer", "T_Noquery",
  "T_Noselect", "T_Noserve", "T_Notrap", "T_Notrust", "T_Ntp", "T_Ntpport",
  "T_NtpSignDsocket", "T_Orphan", "T_Panic", "T_Peer", "T_Peerstats",
  "T_Phone", "T_Pid", "T_Pidfile", "T_Pool", "T_Port", "T_Preempt",
  "T_Prefer", "T_Protostats", "T_Pw", "T_Qos", "T_Randfile", "T_Rawstats",
  "T_Refid", "T_Requestkey", "T_Restrict", "T_Revoke", "T_Saveconfigdir",
  "T_Server", "T_Setvar", "T_Sign", "T_Statistics", "T_Stats",
  "T_Statsdir", "T_Step", "T_Stepout", "T_Stratum", "T_String",
  "T_Sysstats", "T_Tick", "T_Time1", "T_Time2", "T_Timingstats",
  "T_Tinker", "T_Tos", "T_Trap", "T_True", "T_Trustedkey", "T_Ttl",
  "T_Type", "T_Unconfig", "T_Unpeer", "T_Version", "T_WanderThreshold",
  "T_Week", "T_Wildcard", "T_Xleave", "T_Year", "T_Flag", "T_Void",
  "T_EOC", "T_Simulate", "T_Beep_Delay", "T_Sim_Duration",
  "T_Server_Offset", "T_Duration", "T_Freq_Offset", "T_Wander", "T_Jitter",
  "T_Prop_Delay", "T_Proc_Delay", "'='", "'('", "')'", "'{'", "'}'",
  "$accept", "configuration", "command_list", "command", "server_command",
  "client_type", "address", "ip_address", "address_fam", "option_list",
  "option", "option_flag", "option_flag_keyword", "option_int",
  "option_int_keyword", "unpeer_command", "unpeer_keyword",
  "other_mode_command", "authentication_command", "crypto_command_list",
  "crypto_command", "crypto_str_keyword", "orphan_mode_command",
  "tos_option_list", "tos_option", "tos_option_int_keyword",
  "tos_option_dbl_keyword", "monitoring_command", "stats_list", "stat",
  "filegen_option_list", "filegen_option", "link_nolink", "enable_disable",
  "filegen_type", "access_control_command", "ac_flag_list",
  "access_control_flag", "discard_option_list", "discard_option",
  "discard_option_keyword", "fudge_command", "fudge_factor_list",
  "fudge_factor", "fudge_factor_dbl_keyword", "fudge_factor_bool_keyword",
  "system_option_command", "system_option_list", "system_option",
  "system_option_flag_keyword", "tinker_command", "tinker_option_list",
  "tinker_option", "tinker_option_keyword", "miscellaneous_command",
  "misc_cmd_dbl_keyword", "misc_cmd_str_keyword",
  "misc_cmd_str_lcl_keyword", "drift_parm", "variable_assign",
  "t_default_or_zero", "trap_option_list", "trap_option",
  "log_config_list", "log_config_command", "interface_command",
  "interface_nic", "nic_rule_class", "nic_rule_action", "integer_list",
  "integer_list_range", "integer_list_range_elt", "integer_range",
  "string_list", "address_list", "boolean", "number", "simulate_command",
  "sim_conf_start", "sim_init_statement_list", "sim_init_statement",
  "sim_server_list", "sim_server", "sim_server_offset", "sim_server_name",
  "sim_act_list", "sim_act", "sim_act_stmt_list", "sim_act_stmt", 0
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
     415,   416,   417,   418,   419,   420,   421,    61,    40,    41,
     123,   125
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   172,   173,   174,   174,   174,   175,   175,   175,   175,
     175,   175,   175,   175,   175,   175,   175,   175,   175,   176,
     176,   177,   177,   177,   177,   177,   178,   178,   179,   180,
     180,   181,   181,   182,   182,   182,   183,   184,   184,   184,
     184,   184,   184,   184,   184,   185,   186,   186,   186,   186,
     186,   186,   187,   188,   188,   189,   189,   189,   189,   190,
     190,   190,   190,   190,   190,   190,   190,   190,   191,   191,
     192,   192,   193,   193,   193,   193,   193,   193,   194,   195,
     195,   196,   196,   196,   197,   197,   197,   197,   197,   198,
     198,   198,   198,   199,   199,   199,   200,   200,   201,   201,
     201,   201,   201,   201,   201,   201,   202,   202,   203,   203,
     203,   203,   204,   204,   205,   205,   206,   206,   206,   206,
     206,   206,   206,   207,   207,   207,   207,   207,   207,   208,
     208,   209,   209,   209,   209,   209,   209,   209,   209,   209,
     209,   209,   209,   209,   209,   210,   210,   211,   212,   212,
     212,   213,   214,   214,   215,   215,   215,   215,   216,   216,
     217,   217,   217,   217,   218,   218,   219,   219,   220,   220,
     221,   221,   221,   221,   221,   221,   222,   223,   223,   224,
     225,   225,   225,   225,   225,   225,   225,   226,   226,   226,
     226,   226,   226,   226,   226,   226,   226,   226,   226,   226,
     226,   227,   227,   228,   228,   228,   229,   229,   230,   230,
     230,   231,   232,   232,   233,   233,   234,   234,   235,   235,
     236,   237,   237,   238,   238,   239,   239,   239,   239,   240,
     240,   240,   241,   241,   242,   242,   243,   243,   244,   245,
     245,   246,   246,   247,   247,   247,   248,   248,   249,   250,
     251,   251,   252,   252,   253,   253,   254,   255,   256,   257,
     257,   258,   259,   259,   260,   260,   260,   260,   260
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     3,     2,     2,     0,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     3,
       2,     1,     1,     1,     1,     1,     1,     2,     1,     1,
       1,     2,     1,     1,     1,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     2,     1,     1,     1,     1,
       1,     1,     2,     1,     1,     1,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     0,     2,
       2,     2,     1,     1,     1,     1,     1,     1,     2,     2,
       1,     2,     2,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     2,     3,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     0,     2,     2,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     3,     3,     4,     4,     5,     0,
       2,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     2,     1,     2,     1,     1,
       1,     3,     2,     1,     2,     2,     2,     2,     1,     1,
       1,     1,     1,     1,     2,     2,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     2,     1,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     2,     2,
       2,     3,     1,     2,     2,     2,     2,     2,     2,     3,
       2,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       0,     4,     1,     0,     2,     1,     2,     2,     2,     1,
       1,     3,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     1,     2,     1,     1,     1,     5,     2,
       1,     2,     1,     1,     1,     1,     1,     1,     5,     1,
       3,     2,     3,     3,     2,     1,     5,     4,     3,     2,
       1,     6,     3,     2,     3,     3,     3,     3,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,     0,     0,    24,    55,   201,     0,     0,    68,     0,
       0,   210,     0,   192,     0,     0,     0,   223,     0,     0,
     203,     0,   206,    25,     0,     0,     0,   224,     0,    23,
       0,   204,    22,   205,     0,     0,     0,   207,    21,     0,
       0,     0,   202,     0,     0,     0,     0,     0,    53,    54,
     249,     0,     2,     0,     7,     0,     8,     0,     9,    10,
      13,    11,    12,    14,    15,    16,    17,     0,     0,     0,
     187,     0,    18,     0,     5,    59,   193,    60,    61,   170,
     171,   172,   173,   174,   175,   169,   165,   167,   168,   148,
     149,   150,   123,   146,     0,   208,   194,   164,    98,    99,
     100,   101,   105,   102,   103,   104,   106,    29,    30,    28,
       0,    26,     0,     6,    62,    63,   220,   195,   219,   242,
      56,    58,    57,    67,   240,   196,    64,   129,    29,    30,
     129,    26,    65,     0,   197,    93,    97,    94,   180,   181,
     182,   183,   184,   185,   186,   176,   178,     0,    88,    84,
       0,    85,    92,    90,    91,    89,    87,    86,    78,    80,
       0,     0,   198,   236,     0,    66,   235,   237,   233,   200,
       1,     0,     4,    20,    52,   247,   246,   188,   189,   190,
     231,   230,   229,     0,     0,    77,    72,    73,    74,    75,
       0,    76,    69,     0,   166,   145,   147,   209,    95,   160,
     161,   162,   163,     0,     0,   158,   159,   151,   153,     0,
       0,    27,   191,   218,   241,   239,   125,   129,   129,   124,
       0,     0,    96,   177,   179,   245,   243,   244,    83,    79,
      81,    82,     0,     0,   199,   215,     0,   234,   232,     3,
      37,     0,    38,    39,    46,    48,    47,    50,    40,    41,
      42,    43,    49,    51,    44,    19,    32,    33,    36,    34,
       0,   225,   226,   227,   222,   228,   221,     0,     0,     0,
       0,    71,    70,   115,   114,     0,   112,   113,     0,   107,
     110,   111,   157,   156,   152,   154,   155,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   130,   126,   127,   129,   213,   217,   216,   214,     0,
      35,    31,    45,     0,     0,     0,     0,     0,   255,     0,
     251,   108,   122,   118,   120,   116,   117,   119,   121,   109,
     128,   212,   211,     0,   252,   253,     0,   250,   248,   254,
       0,   238,   258,     0,     0,     0,     0,     0,   260,     0,
       0,   256,   259,   257,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   261,     0,
     263,   264,   265,   266,   267,   268,   262
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    51,    52,    53,    54,    55,   119,   111,   112,   255,
     256,   257,   258,   259,   260,    56,    57,    58,    59,    78,
     192,   193,    60,   158,   159,   160,   161,    61,   135,   106,
     198,   279,   280,   281,   329,    62,   216,   301,    92,    93,
      94,    63,   207,   208,   209,   210,    64,    86,    87,    88,
      65,   145,   146,   147,    66,    67,    68,    69,    96,   134,
     332,   234,   235,   117,   118,    70,    71,   266,   183,   169,
     165,   166,   167,   125,   120,   228,   177,    72,    73,   269,
     270,   317,   318,   344,   319,   347,   348,   361,   362
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -147
static const yytype_int16 yypact[] =
{
     215,  -109,     8,  -147,  -147,  -147,    10,    11,  -147,    -1,
      35,   -85,    -1,  -147,    -6,   -14,   -80,  -147,   -76,   -64,
    -147,   -61,  -147,  -147,   -14,    20,   -14,  -147,   -57,  -147,
     -48,  -147,  -147,  -147,    33,   -18,    34,  -147,  -147,   -41,
      -6,   -40,  -147,     5,   164,   -39,   -47,    44,  -147,  -147,
    -147,   101,   352,   -53,  -147,   -14,  -147,   -14,  -147,  -147,
    -147,  -147,  -147,  -147,  -147,  -147,  -147,     1,   -23,   -20,
    -147,    -2,  -147,   -54,  -147,  -147,  -147,  -147,    22,  -147,
    -147,  -147,  -147,  -147,  -147,  -147,    -1,  -147,  -147,  -147,
    -147,  -147,    35,  -147,    63,    89,  -147,    -1,  -147,  -147,
    -147,  -147,  -147,  -147,  -147,  -147,  -147,  -147,  -147,  -147,
     155,  -147,   -10,   489,  -147,  -147,  -147,   -61,  -147,  -147,
     -14,  -147,   -14,  -147,  -147,    -9,  -147,  -147,    99,   100,
    -147,    62,  -147,   -27,  -147,    -6,  -147,  -147,  -147,  -147,
    -147,  -147,  -147,  -147,  -147,     5,  -147,     1,  -147,  -147,
      -8,  -147,  -147,  -147,  -147,  -147,  -147,  -147,   164,  -147,
      87,     1,   -34,  -147,    88,   -47,  -147,  -147,  -147,    91,
    -147,   -13,  -147,    18,  -147,  -147,  -147,  -147,  -147,  -147,
    -147,  -147,  -147,     0,  -122,  -147,  -147,  -147,  -147,  -147,
      92,  -147,  -147,    14,  -147,  -147,  -147,  -147,   -16,  -147,
    -147,  -147,  -147,    16,    96,  -147,  -147,   155,  -147,     1,
      -8,  -147,  -147,  -147,  -147,  -147,   113,  -147,  -147,   113,
     -39,    19,  -147,  -147,  -147,  -147,  -147,  -147,  -147,  -147,
    -147,  -147,   -39,   104,   -34,  -147,   120,  -147,  -147,  -147,
    -147,     1,  -147,  -147,  -147,  -147,  -147,  -147,  -147,  -147,
    -147,  -147,  -147,  -147,  -147,    18,  -147,  -147,  -147,  -147,
     105,  -147,  -147,  -147,  -147,  -147,  -147,   -12,    -4,   -98,
      13,  -147,  -147,  -147,  -147,    28,  -147,  -147,     3,  -147,
    -147,  -147,  -147,  -147,  -147,  -147,  -147,  -147,  -147,  -147,
    -147,  -147,  -147,  -147,  -147,  -147,  -147,  -147,  -147,  -147,
    -147,  -147,   113,   113,  -147,   138,  -147,  -147,  -147,   117,
    -147,  -147,  -147,     1,     1,     6,    23,  -100,  -147,     2,
    -147,  -147,  -147,  -147,  -147,  -147,  -147,  -147,  -147,  -147,
     113,  -147,  -147,    12,  -147,  -147,   -14,  -147,  -147,  -147,
      24,  -147,  -147,     7,    21,     1,    25,  -138,  -147,    31,
       1,  -147,  -147,  -147,    32,   -82,    26,    27,    36,    39,
      46,    93,    45,     1,     1,     1,     1,     1,  -147,    59,
    -147,  -147,  -147,  -147,  -147,  -147,  -147
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -147,  -147,  -147,   -35,  -147,  -147,   -15,   -32,  -147,  -147,
     -66,  -147,  -147,  -147,  -147,  -147,  -147,  -147,  -147,  -147,
    -147,  -147,  -147,  -147,    64,  -147,  -147,  -147,  -147,   -29,
    -147,  -147,  -147,  -147,  -147,  -147,  -128,  -147,  -147,    98,
    -147,  -147,  -147,    17,  -147,  -147,  -147,   179,   -59,  -147,
    -147,  -147,    80,  -147,  -147,  -147,  -147,  -147,  -147,  -147,
    -147,  -147,    -7,  -147,   109,  -147,  -147,  -147,  -147,  -147,
    -147,    66,  -147,  -147,   206,    29,  -146,  -147,  -147,  -147,
     -33,  -147,   -83,  -147,  -147,  -147,  -112,  -147,  -121
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -7
static const yytype_int16 yytable[] =
{
     110,   224,   219,   131,   261,    79,   322,   163,   127,    80,
     138,   136,   273,   162,    98,   231,    81,   171,    99,   274,
     130,   232,   275,   346,   315,   240,   315,   194,   323,   225,
     241,   180,   175,   351,   242,   139,   267,   268,   194,   128,
     173,   129,   174,   107,    89,   108,   226,    74,    95,   185,
     181,   140,   276,   113,   141,   176,   262,   114,   263,    82,
     267,   268,    75,   285,    76,    77,   100,   182,   243,   115,
     186,   338,   116,   187,   121,   277,   123,   233,   212,   244,
     356,   357,   358,   359,   360,   124,    83,   126,   132,   302,
     303,   324,   133,   137,   109,   310,   325,   245,   168,    84,
     101,   170,   246,   172,   247,   214,   222,   214,   102,   142,
     178,   326,   103,   179,   248,   109,   184,   196,    90,   109,
     197,   164,    91,   211,   215,   217,   218,    85,   104,   278,
     249,   250,   105,   264,   227,   143,   144,   188,   220,   189,
     221,   230,   236,   239,   190,   238,   271,   272,   191,   282,
     283,   265,   305,   327,   309,   313,   328,   287,   307,   312,
     251,   321,   252,   314,   331,   288,   253,   334,   335,   320,
     254,   333,   340,   336,   345,   148,   330,   289,   290,   337,
     291,   341,   346,   149,   343,   150,   292,   353,   304,   311,
     195,    97,   350,   363,   364,   199,   200,   201,   202,   349,
     306,   370,   355,   365,   354,   293,   366,   294,   295,   151,
     296,   297,   298,   367,   299,   376,     1,   371,   372,   373,
     374,   375,   229,     2,   284,   223,   213,   308,     3,     4,
       5,   237,   122,     6,   339,   352,   316,     7,     8,   286,
     369,   152,   153,     9,    10,   154,   155,    11,     0,   156,
      12,    13,     0,     0,    14,   356,   357,   358,   359,   360,
       0,   300,    15,     0,   368,     0,     0,   157,    16,     0,
      17,     0,     0,     0,   203,     0,     0,    18,    19,     0,
       0,    20,     0,     0,     0,    21,    22,   204,     0,    23,
      24,   205,   206,     0,     0,    25,     0,     0,     0,     0,
       0,     0,     0,     0,    26,    27,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    28,     0,     0,
      29,   342,    30,     0,    31,    32,     0,     0,     0,     0,
       0,    33,     0,     0,     0,    34,    35,    36,    37,    38,
      39,     0,    40,     0,    41,     0,     0,     0,     0,     0,
      42,     0,     0,     0,    43,    44,    45,     0,    46,    47,
       2,    48,    49,     0,     0,     3,     4,     5,     0,     0,
       6,    -6,    50,     0,     7,     8,     0,     0,     0,     0,
       9,    10,     0,     0,    11,     0,     0,    12,    13,     0,
       0,    14,     0,     0,     0,     0,     0,     0,     0,    15,
       0,     0,     0,     0,     0,    16,     0,    17,     0,     0,
       0,     0,     0,     0,    18,    19,     0,     0,    20,     0,
       0,     0,    21,    22,     0,     0,    23,    24,     0,     0,
       0,     0,    25,     0,     0,     0,     0,     0,     0,     0,
       0,    26,    27,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    28,     0,     0,    29,     0,    30,
       0,    31,    32,     0,     0,     0,     0,     0,    33,     0,
       0,     0,    34,    35,    36,    37,    38,    39,     0,    40,
       0,    41,     0,     0,     0,     0,     0,    42,     0,     0,
       0,    43,    44,    45,     0,    46,    47,     2,    48,    49,
       0,     0,     3,     4,     5,     0,     0,     6,    -6,    50,
       0,     7,     8,     0,     0,     0,     0,     9,    10,     0,
       0,    11,     0,     0,    12,    13,     0,     0,    14,     0,
       0,     0,     0,     0,     0,     0,    15,     0,     0,     0,
       0,     0,    16,     0,    17,     0,     0,     0,     0,     0,
       0,    18,    19,     0,     0,    20,     0,     0,     0,    21,
      22,     0,     0,    23,    24,     0,     0,     0,     0,    25,
       0,     0,     0,     0,     0,     0,     0,     0,    26,    27,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    28,     0,     0,    29,     0,    30,     0,    31,    32,
       0,     0,     0,     0,     0,    33,     0,     0,     0,    34,
      35,    36,    37,    38,    39,     0,    40,     0,    41,     0,
       0,     0,     0,     0,    42,     0,     0,     0,    43,    44,
      45,     0,    46,    47,     0,    48,    49,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    50
};

static const yytype_int16 yycheck[] =
{
      15,   147,   130,    35,     4,     6,     3,    54,    26,    10,
       5,    40,    28,    45,    20,   161,    17,    52,    24,    35,
      35,    55,    38,   161,   124,     7,   124,    86,    25,    37,
      12,    33,    31,   171,    16,    30,   158,   159,    97,    57,
      55,    59,    57,    57,     9,    59,    54,   156,   133,    27,
      52,    46,    68,   133,    49,    54,    56,   133,    58,    60,
     158,   159,    54,   209,    54,    54,    72,    69,    50,   133,
      48,   171,   133,    51,    54,    91,   133,   111,   113,    61,
     162,   163,   164,   165,   166,   133,    87,    54,    54,   217,
     218,    88,   133,   133,   133,   241,    93,    79,    54,   100,
     106,     0,    84,   156,    86,   120,   135,   122,   114,   104,
     133,   108,   118,   133,    96,   133,   170,    54,    83,   133,
      31,   168,    87,   133,   133,    26,    26,   128,   134,   145,
     112,   113,   138,   133,   142,   130,   131,   115,    76,   117,
     167,    54,    54,   156,   122,    54,    54,   133,   126,   133,
      54,   151,   133,   150,    34,   167,   153,    44,    54,    54,
     142,   133,   144,   167,    26,    52,   148,   313,   314,   156,
     152,    54,   170,   167,   167,    11,   304,    64,    65,   156,
      67,   169,   161,    19,   160,    21,    73,   156,   220,   255,
      92,    12,   167,   167,   167,    40,    41,    42,    43,   345,
     232,   156,   170,   167,   350,    92,   167,    94,    95,    45,
      97,    98,    99,   167,   101,   156,     1,   363,   364,   365,
     366,   367,   158,     8,   207,   145,   117,   234,    13,    14,
      15,   165,    26,    18,   317,   347,   269,    22,    23,   210,
     361,    77,    78,    28,    29,    81,    82,    32,    -1,    85,
      35,    36,    -1,    -1,    39,   162,   163,   164,   165,   166,
      -1,   148,    47,    -1,   171,    -1,    -1,   103,    53,    -1,
      55,    -1,    -1,    -1,   119,    -1,    -1,    62,    63,    -1,
      -1,    66,    -1,    -1,    -1,    70,    71,   132,    -1,    74,
      75,   136,   137,    -1,    -1,    80,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    89,    90,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   102,    -1,    -1,
     105,   336,   107,    -1,   109,   110,    -1,    -1,    -1,    -1,
      -1,   116,    -1,    -1,    -1,   120,   121,   122,   123,   124,
     125,    -1,   127,    -1,   129,    -1,    -1,    -1,    -1,    -1,
     135,    -1,    -1,    -1,   139,   140,   141,    -1,   143,   144,
       8,   146,   147,    -1,    -1,    13,    14,    15,    -1,    -1,
      18,   156,   157,    -1,    22,    23,    -1,    -1,    -1,    -1,
      28,    29,    -1,    -1,    32,    -1,    -1,    35,    36,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    47,
      -1,    -1,    -1,    -1,    -1,    53,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    62,    63,    -1,    -1,    66,    -1,
      -1,    -1,    70,    71,    -1,    -1,    74,    75,    -1,    -1,
      -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    89,    90,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   102,    -1,    -1,   105,    -1,   107,
      -1,   109,   110,    -1,    -1,    -1,    -1,    -1,   116,    -1,
      -1,    -1,   120,   121,   122,   123,   124,   125,    -1,   127,
      -1,   129,    -1,    -1,    -1,    -1,    -1,   135,    -1,    -1,
      -1,   139,   140,   141,    -1,   143,   144,     8,   146,   147,
      -1,    -1,    13,    14,    15,    -1,    -1,    18,   156,   157,
      -1,    22,    23,    -1,    -1,    -1,    -1,    28,    29,    -1,
      -1,    32,    -1,    -1,    35,    36,    -1,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,
      -1,    -1,    53,    -1,    55,    -1,    -1,    -1,    -1,    -1,
      -1,    62,    63,    -1,    -1,    66,    -1,    -1,    -1,    70,
      71,    -1,    -1,    74,    75,    -1,    -1,    -1,    -1,    80,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    89,    90,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   102,    -1,    -1,   105,    -1,   107,    -1,   109,   110,
      -1,    -1,    -1,    -1,    -1,   116,    -1,    -1,    -1,   120,
     121,   122,   123,   124,   125,    -1,   127,    -1,   129,    -1,
      -1,    -1,    -1,    -1,   135,    -1,    -1,    -1,   139,   140,
     141,    -1,   143,   144,    -1,   146,   147,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   157
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,     1,     8,    13,    14,    15,    18,    22,    23,    28,
      29,    32,    35,    36,    39,    47,    53,    55,    62,    63,
      66,    70,    71,    74,    75,    80,    89,    90,   102,   105,
     107,   109,   110,   116,   120,   121,   122,   123,   124,   125,
     127,   129,   135,   139,   140,   141,   143,   144,   146,   147,
     157,   173,   174,   175,   176,   177,   187,   188,   189,   190,
     194,   199,   207,   213,   218,   222,   226,   227,   228,   229,
     237,   238,   249,   250,   156,    54,    54,    54,   191,     6,
      10,    17,    60,    87,   100,   128,   219,   220,   221,     9,
      83,    87,   210,   211,   212,   133,   230,   219,    20,    24,
      72,   106,   114,   118,   134,   138,   201,    57,    59,   133,
     178,   179,   180,   133,   133,   133,   133,   235,   236,   178,
     246,    54,   246,   133,   133,   245,    54,    26,    57,    59,
     178,   179,    54,   133,   231,   200,   201,   133,     5,    30,
      46,    49,   104,   130,   131,   223,   224,   225,    11,    19,
      21,    45,    77,    78,    81,    82,    85,   103,   195,   196,
     197,   198,   179,    54,   168,   242,   243,   244,    54,   241,
       0,   175,   156,   178,   178,    31,    54,   248,   133,   133,
      33,    52,    69,   240,   170,    27,    48,    51,   115,   117,
     122,   126,   192,   193,   220,   211,    54,    31,   202,    40,
      41,    42,    43,   119,   132,   136,   137,   214,   215,   216,
     217,   133,   175,   236,   178,   133,   208,    26,    26,   208,
      76,   167,   201,   224,   248,    37,    54,   142,   247,   196,
      54,   248,    55,   111,   233,   234,    54,   243,    54,   156,
       7,    12,    16,    50,    61,    79,    84,    86,    96,   112,
     113,   142,   144,   148,   152,   181,   182,   183,   184,   185,
     186,     4,    56,    58,   133,   151,   239,   158,   159,   251,
     252,    54,   133,    28,    35,    38,    68,    91,   145,   203,
     204,   205,   133,    54,   215,   248,   247,    44,    52,    64,
      65,    67,    73,    92,    94,    95,    97,    98,    99,   101,
     148,   209,   208,   208,   179,   133,   179,    54,   234,    34,
     248,   182,    54,   167,   167,   124,   252,   253,   254,   256,
     156,   133,     3,    25,    88,    93,   108,   150,   153,   206,
     208,    26,   232,    54,   248,   248,   167,   156,   171,   254,
     170,   169,   178,   160,   255,   167,   161,   257,   258,   248,
     167,   171,   258,   156,   248,   170,   162,   163,   164,   165,
     166,   259,   260,   167,   167,   167,   167,   167,   171,   260,
     156,   248,   248,   248,   248,   248,   156
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
      yyerror (YY_("syntax error: cannot back up")); \
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
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
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

/* Prevent warnings from -Wmissing-prototypes.  */
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


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



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
        case 5:

/* Line 1464 of yacc.c  */
#line 328 "ntp_parser.y"
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

  case 19:

/* Line 1464 of yacc.c  */
#line 362 "ntp_parser.y"
    {
			struct peer_node *my_node =  create_peer_node((yyvsp[(1) - (3)].Integer), (yyvsp[(2) - (3)].Address_node), (yyvsp[(3) - (3)].Queue));
			if (my_node)
				enqueue(cfgt.peers, my_node);
		}
    break;

  case 20:

/* Line 1464 of yacc.c  */
#line 368 "ntp_parser.y"
    {
			struct peer_node *my_node = create_peer_node((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Address_node), NULL);
			if (my_node)
				enqueue(cfgt.peers, my_node);
		}
    break;

  case 27:

/* Line 1464 of yacc.c  */
#line 386 "ntp_parser.y"
    { (yyval.Address_node) = create_address_node((yyvsp[(2) - (2)].String), (yyvsp[(1) - (2)].Integer)); }
    break;

  case 28:

/* Line 1464 of yacc.c  */
#line 391 "ntp_parser.y"
    { (yyval.Address_node) = create_address_node((yyvsp[(1) - (1)].String), 0); }
    break;

  case 29:

/* Line 1464 of yacc.c  */
#line 396 "ntp_parser.y"
    { (yyval.Integer) = AF_INET; }
    break;

  case 30:

/* Line 1464 of yacc.c  */
#line 398 "ntp_parser.y"
    { (yyval.Integer) = AF_INET6; }
    break;

  case 31:

/* Line 1464 of yacc.c  */
#line 402 "ntp_parser.y"
    { (yyval.Queue) = enqueue((yyvsp[(1) - (2)].Queue), (yyvsp[(2) - (2)].Attr_val)); }
    break;

  case 32:

/* Line 1464 of yacc.c  */
#line 403 "ntp_parser.y"
    { (yyval.Queue) = enqueue_in_new_queue((yyvsp[(1) - (1)].Attr_val)); }
    break;

  case 35:

/* Line 1464 of yacc.c  */
#line 410 "ntp_parser.y"
    { (yyval.Attr_val) = create_attr_dval((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Double)); }
    break;

  case 36:

/* Line 1464 of yacc.c  */
#line 415 "ntp_parser.y"
    { (yyval.Attr_val) = create_attr_ival(T_Flag, (yyvsp[(1) - (1)].Integer)); }
    break;

  case 45:

/* Line 1464 of yacc.c  */
#line 431 "ntp_parser.y"
    { (yyval.Attr_val) = create_attr_ival((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Integer)); }
    break;

  case 52:

/* Line 1464 of yacc.c  */
#line 451 "ntp_parser.y"
    {
			struct unpeer_node *my_node = create_unpeer_node((yyvsp[(2) - (2)].Address_node));
			if (my_node)
				enqueue(cfgt.unpeers, my_node);
		}
    break;

  case 55:

/* Line 1464 of yacc.c  */
#line 470 "ntp_parser.y"
    { cfgt.broadcastclient = 1; }
    break;

  case 56:

/* Line 1464 of yacc.c  */
#line 472 "ntp_parser.y"
    { append_queue(cfgt.manycastserver, (yyvsp[(2) - (2)].Queue)); }
    break;

  case 57:

/* Line 1464 of yacc.c  */
#line 474 "ntp_parser.y"
    { append_queue(cfgt.multicastclient, (yyvsp[(2) - (2)].Queue)); }
    break;

  case 58:

/* Line 1464 of yacc.c  */
#line 476 "ntp_parser.y"
    { cfgt.mdnstries = (yyvsp[(2) - (2)].Integer); }
    break;

  case 59:

/* Line 1464 of yacc.c  */
#line 487 "ntp_parser.y"
    { enqueue(cfgt.vars, create_attr_ival((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Integer))); }
    break;

  case 60:

/* Line 1464 of yacc.c  */
#line 489 "ntp_parser.y"
    { cfgt.auth.control_key = (yyvsp[(2) - (2)].Integer); }
    break;

  case 61:

/* Line 1464 of yacc.c  */
#line 491 "ntp_parser.y"
    { 
			cfgt.auth.cryptosw++;
			append_queue(cfgt.auth.crypto_cmd_list, (yyvsp[(2) - (2)].Queue));
		}
    break;

  case 62:

/* Line 1464 of yacc.c  */
#line 496 "ntp_parser.y"
    { cfgt.auth.keys = (yyvsp[(2) - (2)].String); }
    break;

  case 63:

/* Line 1464 of yacc.c  */
#line 498 "ntp_parser.y"
    { cfgt.auth.keysdir = (yyvsp[(2) - (2)].String); }
    break;

  case 64:

/* Line 1464 of yacc.c  */
#line 500 "ntp_parser.y"
    { cfgt.auth.request_key = (yyvsp[(2) - (2)].Integer); }
    break;

  case 65:

/* Line 1464 of yacc.c  */
#line 502 "ntp_parser.y"
    { cfgt.auth.revoke = (yyvsp[(2) - (2)].Integer); }
    break;

  case 66:

/* Line 1464 of yacc.c  */
#line 504 "ntp_parser.y"
    { cfgt.auth.trusted_key_list = (yyvsp[(2) - (2)].Queue); }
    break;

  case 67:

/* Line 1464 of yacc.c  */
#line 506 "ntp_parser.y"
    { cfgt.auth.ntp_signd_socket = (yyvsp[(2) - (2)].String); }
    break;

  case 68:

/* Line 1464 of yacc.c  */
#line 511 "ntp_parser.y"
    { (yyval.Queue) = create_queue(); }
    break;

  case 69:

/* Line 1464 of yacc.c  */
#line 513 "ntp_parser.y"
    { 
			if ((yyvsp[(2) - (2)].Attr_val) != NULL)
				(yyval.Queue) = enqueue((yyvsp[(1) - (2)].Queue), (yyvsp[(2) - (2)].Attr_val));
			else
				(yyval.Queue) = (yyvsp[(1) - (2)].Queue);
		}
    break;

  case 70:

/* Line 1464 of yacc.c  */
#line 523 "ntp_parser.y"
    { (yyval.Attr_val) = create_attr_sval((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].String)); }
    break;

  case 71:

/* Line 1464 of yacc.c  */
#line 525 "ntp_parser.y"
    {
			(yyval.Attr_val) = NULL;
			cfgt.auth.revoke = (yyvsp[(2) - (2)].Integer);
			msyslog(LOG_WARNING,
				"'crypto revoke %d' is deprecated, "
				"please use 'revoke %d' instead.",
				cfgt.auth.revoke, cfgt.auth.revoke);
		}
    break;

  case 78:

/* Line 1464 of yacc.c  */
#line 551 "ntp_parser.y"
    { append_queue(cfgt.orphan_cmds,(yyvsp[(2) - (2)].Queue)); }
    break;

  case 79:

/* Line 1464 of yacc.c  */
#line 555 "ntp_parser.y"
    { (yyval.Queue) = enqueue((yyvsp[(1) - (2)].Queue), (yyvsp[(2) - (2)].Attr_val)); }
    break;

  case 80:

/* Line 1464 of yacc.c  */
#line 556 "ntp_parser.y"
    { (yyval.Queue) = enqueue_in_new_queue((yyvsp[(1) - (1)].Attr_val)); }
    break;

  case 81:

/* Line 1464 of yacc.c  */
#line 561 "ntp_parser.y"
    { (yyval.Attr_val) = create_attr_dval((yyvsp[(1) - (2)].Integer), (double)(yyvsp[(2) - (2)].Integer)); }
    break;

  case 82:

/* Line 1464 of yacc.c  */
#line 563 "ntp_parser.y"
    { (yyval.Attr_val) = create_attr_dval((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Double)); }
    break;

  case 83:

/* Line 1464 of yacc.c  */
#line 565 "ntp_parser.y"
    { (yyval.Attr_val) = create_attr_dval((yyvsp[(1) - (2)].Integer), (double)(yyvsp[(2) - (2)].Integer)); }
    break;

  case 93:

/* Line 1464 of yacc.c  */
#line 590 "ntp_parser.y"
    { append_queue(cfgt.stats_list, (yyvsp[(2) - (2)].Queue)); }
    break;

  case 94:

/* Line 1464 of yacc.c  */
#line 592 "ntp_parser.y"
    {
			if (input_from_file) {
				cfgt.stats_dir = (yyvsp[(2) - (2)].String);
			} else {
				YYFREE((yyvsp[(2) - (2)].String));
				yyerror("statsdir remote configuration ignored");
			}
		}
    break;

  case 95:

/* Line 1464 of yacc.c  */
#line 601 "ntp_parser.y"
    {
			enqueue(cfgt.filegen_opts,
				create_filegen_node((yyvsp[(2) - (3)].Integer), (yyvsp[(3) - (3)].Queue)));
		}
    break;

  case 96:

/* Line 1464 of yacc.c  */
#line 608 "ntp_parser.y"
    { (yyval.Queue) = enqueue((yyvsp[(1) - (2)].Queue), create_ival((yyvsp[(2) - (2)].Integer))); }
    break;

  case 97:

/* Line 1464 of yacc.c  */
#line 609 "ntp_parser.y"
    { (yyval.Queue) = enqueue_in_new_queue(create_ival((yyvsp[(1) - (1)].Integer))); }
    break;

  case 106:

/* Line 1464 of yacc.c  */
#line 625 "ntp_parser.y"
    { (yyval.Queue) = create_queue(); }
    break;

  case 107:

/* Line 1464 of yacc.c  */
#line 627 "ntp_parser.y"
    {
			if ((yyvsp[(2) - (2)].Attr_val) != NULL)
				(yyval.Queue) = enqueue((yyvsp[(1) - (2)].Queue), (yyvsp[(2) - (2)].Attr_val));
			else
				(yyval.Queue) = (yyvsp[(1) - (2)].Queue);
		}
    break;

  case 108:

/* Line 1464 of yacc.c  */
#line 637 "ntp_parser.y"
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

  case 109:

/* Line 1464 of yacc.c  */
#line 647 "ntp_parser.y"
    {
			if (input_from_file) {
				(yyval.Attr_val) = create_attr_ival((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Integer));
			} else {
				(yyval.Attr_val) = NULL;
				yyerror("filegen type remote config ignored");
			}
		}
    break;

  case 110:

/* Line 1464 of yacc.c  */
#line 656 "ntp_parser.y"
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

  case 111:

/* Line 1464 of yacc.c  */
#line 671 "ntp_parser.y"
    { (yyval.Attr_val) = create_attr_ival(T_Flag, (yyvsp[(1) - (1)].Integer)); }
    break;

  case 123:

/* Line 1464 of yacc.c  */
#line 701 "ntp_parser.y"
    {
			append_queue(cfgt.discard_opts, (yyvsp[(2) - (2)].Queue));
		}
    break;

  case 124:

/* Line 1464 of yacc.c  */
#line 705 "ntp_parser.y"
    {
			enqueue(cfgt.restrict_opts,
				create_restrict_node((yyvsp[(2) - (3)].Address_node), NULL, (yyvsp[(3) - (3)].Queue), ip_file->line_no));
		}
    break;

  case 125:

/* Line 1464 of yacc.c  */
#line 710 "ntp_parser.y"
    {
			enqueue(cfgt.restrict_opts,
				create_restrict_node(NULL, NULL, (yyvsp[(3) - (3)].Queue), ip_file->line_no));
		}
    break;

  case 126:

/* Line 1464 of yacc.c  */
#line 715 "ntp_parser.y"
    {
			enqueue(cfgt.restrict_opts,
				create_restrict_node(
					create_address_node(
						estrdup("0.0.0.0"), 
						AF_INET),
					create_address_node(
						estrdup("0.0.0.0"), 
						AF_INET),
					(yyvsp[(4) - (4)].Queue), 
					ip_file->line_no));
		}
    break;

  case 127:

/* Line 1464 of yacc.c  */
#line 728 "ntp_parser.y"
    {
			enqueue(cfgt.restrict_opts,
				create_restrict_node(
					create_address_node(
						estrdup("::"), 
						AF_INET6),
					create_address_node(
						estrdup("::"), 
						AF_INET6),
					(yyvsp[(4) - (4)].Queue), 
					ip_file->line_no));
		}
    break;

  case 128:

/* Line 1464 of yacc.c  */
#line 741 "ntp_parser.y"
    {
			enqueue(cfgt.restrict_opts,
				create_restrict_node((yyvsp[(2) - (5)].Address_node), (yyvsp[(4) - (5)].Address_node), (yyvsp[(5) - (5)].Queue), ip_file->line_no));
		}
    break;

  case 129:

/* Line 1464 of yacc.c  */
#line 749 "ntp_parser.y"
    { (yyval.Queue) = create_queue(); }
    break;

  case 130:

/* Line 1464 of yacc.c  */
#line 751 "ntp_parser.y"
    { (yyval.Queue) = enqueue((yyvsp[(1) - (2)].Queue), create_ival((yyvsp[(2) - (2)].Integer))); }
    break;

  case 145:

/* Line 1464 of yacc.c  */
#line 773 "ntp_parser.y"
    { (yyval.Queue) = enqueue((yyvsp[(1) - (2)].Queue), (yyvsp[(2) - (2)].Attr_val)); }
    break;

  case 146:

/* Line 1464 of yacc.c  */
#line 775 "ntp_parser.y"
    { (yyval.Queue) = enqueue_in_new_queue((yyvsp[(1) - (1)].Attr_val)); }
    break;

  case 147:

/* Line 1464 of yacc.c  */
#line 780 "ntp_parser.y"
    { (yyval.Attr_val) = create_attr_ival((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Integer)); }
    break;

  case 151:

/* Line 1464 of yacc.c  */
#line 796 "ntp_parser.y"
    { enqueue(cfgt.fudge, create_addr_opts_node((yyvsp[(2) - (3)].Address_node), (yyvsp[(3) - (3)].Queue))); }
    break;

  case 152:

/* Line 1464 of yacc.c  */
#line 801 "ntp_parser.y"
    { enqueue((yyvsp[(1) - (2)].Queue), (yyvsp[(2) - (2)].Attr_val)); }
    break;

  case 153:

/* Line 1464 of yacc.c  */
#line 803 "ntp_parser.y"
    { (yyval.Queue) = enqueue_in_new_queue((yyvsp[(1) - (1)].Attr_val)); }
    break;

  case 154:

/* Line 1464 of yacc.c  */
#line 808 "ntp_parser.y"
    { (yyval.Attr_val) = create_attr_dval((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Double)); }
    break;

  case 155:

/* Line 1464 of yacc.c  */
#line 810 "ntp_parser.y"
    { (yyval.Attr_val) = create_attr_ival((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Integer)); }
    break;

  case 156:

/* Line 1464 of yacc.c  */
#line 812 "ntp_parser.y"
    { (yyval.Attr_val) = create_attr_ival((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Integer)); }
    break;

  case 157:

/* Line 1464 of yacc.c  */
#line 814 "ntp_parser.y"
    { (yyval.Attr_val) = create_attr_sval((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].String)); }
    break;

  case 164:

/* Line 1464 of yacc.c  */
#line 836 "ntp_parser.y"
    { append_queue(cfgt.enable_opts, (yyvsp[(2) - (2)].Queue));  }
    break;

  case 165:

/* Line 1464 of yacc.c  */
#line 838 "ntp_parser.y"
    { append_queue(cfgt.disable_opts, (yyvsp[(2) - (2)].Queue));  }
    break;

  case 166:

/* Line 1464 of yacc.c  */
#line 843 "ntp_parser.y"
    {
			if ((yyvsp[(2) - (2)].Attr_val) != NULL)
				(yyval.Queue) = enqueue((yyvsp[(1) - (2)].Queue), (yyvsp[(2) - (2)].Attr_val));
			else
				(yyval.Queue) = (yyvsp[(1) - (2)].Queue);
		}
    break;

  case 167:

/* Line 1464 of yacc.c  */
#line 850 "ntp_parser.y"
    {
			if ((yyvsp[(1) - (1)].Attr_val) != NULL)
				(yyval.Queue) = enqueue_in_new_queue((yyvsp[(1) - (1)].Attr_val));
			else
				(yyval.Queue) = create_queue();
		}
    break;

  case 168:

/* Line 1464 of yacc.c  */
#line 860 "ntp_parser.y"
    { (yyval.Attr_val) = create_attr_ival(T_Flag, (yyvsp[(1) - (1)].Integer)); }
    break;

  case 169:

/* Line 1464 of yacc.c  */
#line 862 "ntp_parser.y"
    { 
			if (input_from_file) {
				(yyval.Attr_val) = create_attr_ival(T_Flag, (yyvsp[(1) - (1)].Integer));
			} else {
				(yyval.Attr_val) = NULL;
				yyerror("enable/disable stats remote config ignored");
			}
		}
    break;

  case 176:

/* Line 1464 of yacc.c  */
#line 887 "ntp_parser.y"
    { append_queue(cfgt.tinker, (yyvsp[(2) - (2)].Queue)); }
    break;

  case 177:

/* Line 1464 of yacc.c  */
#line 891 "ntp_parser.y"
    { (yyval.Queue) = enqueue((yyvsp[(1) - (2)].Queue), (yyvsp[(2) - (2)].Attr_val)); }
    break;

  case 178:

/* Line 1464 of yacc.c  */
#line 892 "ntp_parser.y"
    { (yyval.Queue) = enqueue_in_new_queue((yyvsp[(1) - (1)].Attr_val)); }
    break;

  case 179:

/* Line 1464 of yacc.c  */
#line 897 "ntp_parser.y"
    { (yyval.Attr_val) = create_attr_dval((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Double)); }
    break;

  case 188:

/* Line 1464 of yacc.c  */
#line 918 "ntp_parser.y"
    {
			struct attr_val *av;
			
			av = create_attr_dval((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Double));
			enqueue(cfgt.vars, av);
		}
    break;

  case 189:

/* Line 1464 of yacc.c  */
#line 925 "ntp_parser.y"
    {
			struct attr_val *av;
			
			av = create_attr_sval((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].String));
			enqueue(cfgt.vars, av);
		}
    break;

  case 190:

/* Line 1464 of yacc.c  */
#line 932 "ntp_parser.y"
    {
			char error_text[64];
			struct attr_val *av;

			if (input_from_file) {
				av = create_attr_sval((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].String));
				enqueue(cfgt.vars, av);
			} else {
				YYFREE((yyvsp[(2) - (2)].String));
				snprintf(error_text, sizeof(error_text),
					 "%s remote config ignored",
					 keyword((yyvsp[(1) - (2)].Integer)));
				yyerror(error_text);
			}
		}
    break;

  case 191:

/* Line 1464 of yacc.c  */
#line 948 "ntp_parser.y"
    {
			if (!input_from_file) {
				yyerror("remote includefile ignored");
				break;
			}
			if (curr_include_level >= MAXINCLUDELEVEL) {
				fprintf(stderr, "getconfig: Maximum include file level exceeded.\n");
				msyslog(LOG_ERR, "getconfig: Maximum include file level exceeded.\n");
			} else {
				fp[curr_include_level + 1] = F_OPEN(FindConfig((yyvsp[(2) - (3)].String)), "r");
				if (fp[curr_include_level + 1] == NULL) {
					fprintf(stderr, "getconfig: Couldn't open <%s>\n", FindConfig((yyvsp[(2) - (3)].String)));
					msyslog(LOG_ERR, "getconfig: Couldn't open <%s>\n", FindConfig((yyvsp[(2) - (3)].String)));
				} else {
					ip_file = fp[++curr_include_level];
				}
			}
		}
    break;

  case 192:

/* Line 1464 of yacc.c  */
#line 967 "ntp_parser.y"
    {
			while (curr_include_level != -1)
				FCLOSE(fp[curr_include_level--]);
		}
    break;

  case 193:

/* Line 1464 of yacc.c  */
#line 972 "ntp_parser.y"
    { enqueue(cfgt.vars, create_attr_ival((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Integer))); }
    break;

  case 194:

/* Line 1464 of yacc.c  */
#line 974 "ntp_parser.y"
    { /* Null action, possibly all null parms */ }
    break;

  case 195:

/* Line 1464 of yacc.c  */
#line 976 "ntp_parser.y"
    { append_queue(cfgt.logconfig, (yyvsp[(2) - (2)].Queue)); }
    break;

  case 196:

/* Line 1464 of yacc.c  */
#line 978 "ntp_parser.y"
    { append_queue(cfgt.phone, (yyvsp[(2) - (2)].Queue)); }
    break;

  case 197:

/* Line 1464 of yacc.c  */
#line 980 "ntp_parser.y"
    { enqueue(cfgt.setvar, (yyvsp[(2) - (2)].Set_var)); }
    break;

  case 198:

/* Line 1464 of yacc.c  */
#line 982 "ntp_parser.y"
    { enqueue(cfgt.trap, create_addr_opts_node((yyvsp[(2) - (2)].Address_node), NULL)); }
    break;

  case 199:

/* Line 1464 of yacc.c  */
#line 984 "ntp_parser.y"
    { enqueue(cfgt.trap, create_addr_opts_node((yyvsp[(2) - (3)].Address_node), (yyvsp[(3) - (3)].Queue))); }
    break;

  case 200:

/* Line 1464 of yacc.c  */
#line 986 "ntp_parser.y"
    { append_queue(cfgt.ttl, (yyvsp[(2) - (2)].Queue)); }
    break;

  case 208:

/* Line 1464 of yacc.c  */
#line 1007 "ntp_parser.y"
    { enqueue(cfgt.vars, create_attr_sval(T_Driftfile, (yyvsp[(1) - (1)].String))); }
    break;

  case 209:

/* Line 1464 of yacc.c  */
#line 1009 "ntp_parser.y"
    { enqueue(cfgt.vars, create_attr_dval(T_WanderThreshold, (yyvsp[(2) - (2)].Double)));
			  enqueue(cfgt.vars, create_attr_sval(T_Driftfile, (yyvsp[(1) - (2)].String))); }
    break;

  case 210:

/* Line 1464 of yacc.c  */
#line 1012 "ntp_parser.y"
    { enqueue(cfgt.vars, create_attr_sval(T_Driftfile, NULL)); }
    break;

  case 211:

/* Line 1464 of yacc.c  */
#line 1017 "ntp_parser.y"
    { (yyval.Set_var) = create_setvar_node((yyvsp[(1) - (4)].String), (yyvsp[(3) - (4)].String), (yyvsp[(4) - (4)].Integer)); }
    break;

  case 213:

/* Line 1464 of yacc.c  */
#line 1023 "ntp_parser.y"
    { (yyval.Integer) = 0; }
    break;

  case 214:

/* Line 1464 of yacc.c  */
#line 1028 "ntp_parser.y"
    { (yyval.Queue) = enqueue((yyvsp[(1) - (2)].Queue), (yyvsp[(2) - (2)].Attr_val)); }
    break;

  case 215:

/* Line 1464 of yacc.c  */
#line 1029 "ntp_parser.y"
    { (yyval.Queue) = enqueue_in_new_queue((yyvsp[(1) - (1)].Attr_val)); }
    break;

  case 216:

/* Line 1464 of yacc.c  */
#line 1033 "ntp_parser.y"
    { (yyval.Attr_val) = create_attr_ival((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Integer)); }
    break;

  case 217:

/* Line 1464 of yacc.c  */
#line 1034 "ntp_parser.y"
    { (yyval.Attr_val) = create_attr_pval((yyvsp[(1) - (2)].Integer), (yyvsp[(2) - (2)].Address_node)); }
    break;

  case 218:

/* Line 1464 of yacc.c  */
#line 1038 "ntp_parser.y"
    { (yyval.Queue) = enqueue((yyvsp[(1) - (2)].Queue), (yyvsp[(2) - (2)].Attr_val)); }
    break;

  case 219:

/* Line 1464 of yacc.c  */
#line 1039 "ntp_parser.y"
    { (yyval.Queue) = enqueue_in_new_queue((yyvsp[(1) - (1)].Attr_val)); }
    break;

  case 220:

/* Line 1464 of yacc.c  */
#line 1044 "ntp_parser.y"
    {
			char prefix = (yyvsp[(1) - (1)].String)[0];
			char *type = (yyvsp[(1) - (1)].String) + 1;
			
			if (prefix != '+' && prefix != '-' && prefix != '=') {
				yyerror("Logconfig prefix is not '+', '-' or '='\n");
			}
			else
				(yyval.Attr_val) = create_attr_sval(prefix, estrdup(type));
			YYFREE((yyvsp[(1) - (1)].String));
		}
    break;

  case 221:

/* Line 1464 of yacc.c  */
#line 1059 "ntp_parser.y"
    {
			enqueue(cfgt.nic_rules,
				create_nic_rule_node((yyvsp[(3) - (3)].Integer), NULL, (yyvsp[(2) - (3)].Integer)));
		}
    break;

  case 222:

/* Line 1464 of yacc.c  */
#line 1064 "ntp_parser.y"
    {
			enqueue(cfgt.nic_rules,
				create_nic_rule_node(0, (yyvsp[(3) - (3)].String), (yyvsp[(2) - (3)].Integer)));
		}
    break;

  case 232:

/* Line 1464 of yacc.c  */
#line 1095 "ntp_parser.y"
    { (yyval.Queue) = enqueue((yyvsp[(1) - (2)].Queue), create_ival((yyvsp[(2) - (2)].Integer))); }
    break;

  case 233:

/* Line 1464 of yacc.c  */
#line 1096 "ntp_parser.y"
    { (yyval.Queue) = enqueue_in_new_queue(create_ival((yyvsp[(1) - (1)].Integer))); }
    break;

  case 234:

/* Line 1464 of yacc.c  */
#line 1101 "ntp_parser.y"
    { (yyval.Queue) = enqueue((yyvsp[(1) - (2)].Queue), (yyvsp[(2) - (2)].Attr_val)); }
    break;

  case 235:

/* Line 1464 of yacc.c  */
#line 1103 "ntp_parser.y"
    { (yyval.Queue) = enqueue_in_new_queue((yyvsp[(1) - (1)].Attr_val)); }
    break;

  case 236:

/* Line 1464 of yacc.c  */
#line 1108 "ntp_parser.y"
    { (yyval.Attr_val) = create_attr_ival('i', (yyvsp[(1) - (1)].Integer)); }
    break;

  case 238:

/* Line 1464 of yacc.c  */
#line 1114 "ntp_parser.y"
    { (yyval.Attr_val) = create_attr_shorts('-', (yyvsp[(2) - (5)].Integer), (yyvsp[(4) - (5)].Integer)); }
    break;

  case 239:

/* Line 1464 of yacc.c  */
#line 1118 "ntp_parser.y"
    { (yyval.Queue) = enqueue((yyvsp[(1) - (2)].Queue), create_pval((yyvsp[(2) - (2)].String))); }
    break;

  case 240:

/* Line 1464 of yacc.c  */
#line 1119 "ntp_parser.y"
    { (yyval.Queue) = enqueue_in_new_queue(create_pval((yyvsp[(1) - (1)].String))); }
    break;

  case 241:

/* Line 1464 of yacc.c  */
#line 1123 "ntp_parser.y"
    { (yyval.Queue) = enqueue((yyvsp[(1) - (2)].Queue), (yyvsp[(2) - (2)].Address_node)); }
    break;

  case 242:

/* Line 1464 of yacc.c  */
#line 1124 "ntp_parser.y"
    { (yyval.Queue) = enqueue_in_new_queue((yyvsp[(1) - (1)].Address_node)); }
    break;

  case 243:

/* Line 1464 of yacc.c  */
#line 1129 "ntp_parser.y"
    {
			if ((yyvsp[(1) - (1)].Integer) != 0 && (yyvsp[(1) - (1)].Integer) != 1) {
				yyerror("Integer value is not boolean (0 or 1). Assuming 1");
				(yyval.Integer) = 1;
			} else {
				(yyval.Integer) = (yyvsp[(1) - (1)].Integer);
			}
		}
    break;

  case 244:

/* Line 1464 of yacc.c  */
#line 1137 "ntp_parser.y"
    { (yyval.Integer) = 1; }
    break;

  case 245:

/* Line 1464 of yacc.c  */
#line 1138 "ntp_parser.y"
    { (yyval.Integer) = 0; }
    break;

  case 246:

/* Line 1464 of yacc.c  */
#line 1142 "ntp_parser.y"
    { (yyval.Double) = (double)(yyvsp[(1) - (1)].Integer); }
    break;

  case 248:

/* Line 1464 of yacc.c  */
#line 1153 "ntp_parser.y"
    {
			cfgt.sim_details = create_sim_node((yyvsp[(3) - (5)].Queue), (yyvsp[(4) - (5)].Queue));

			/* Reset the old_config_style variable */
			old_config_style = 1;
		}
    break;

  case 249:

/* Line 1464 of yacc.c  */
#line 1167 "ntp_parser.y"
    { old_config_style = 0; }
    break;

  case 250:

/* Line 1464 of yacc.c  */
#line 1171 "ntp_parser.y"
    { (yyval.Queue) = enqueue((yyvsp[(1) - (3)].Queue), (yyvsp[(2) - (3)].Attr_val)); }
    break;

  case 251:

/* Line 1464 of yacc.c  */
#line 1172 "ntp_parser.y"
    { (yyval.Queue) = enqueue_in_new_queue((yyvsp[(1) - (2)].Attr_val)); }
    break;

  case 252:

/* Line 1464 of yacc.c  */
#line 1176 "ntp_parser.y"
    { (yyval.Attr_val) = create_attr_dval((yyvsp[(1) - (3)].Integer), (yyvsp[(3) - (3)].Double)); }
    break;

  case 253:

/* Line 1464 of yacc.c  */
#line 1177 "ntp_parser.y"
    { (yyval.Attr_val) = create_attr_dval((yyvsp[(1) - (3)].Integer), (yyvsp[(3) - (3)].Double)); }
    break;

  case 254:

/* Line 1464 of yacc.c  */
#line 1181 "ntp_parser.y"
    { (yyval.Queue) = enqueue((yyvsp[(1) - (2)].Queue), (yyvsp[(2) - (2)].Sim_server)); }
    break;

  case 255:

/* Line 1464 of yacc.c  */
#line 1182 "ntp_parser.y"
    { (yyval.Queue) = enqueue_in_new_queue((yyvsp[(1) - (1)].Sim_server)); }
    break;

  case 256:

/* Line 1464 of yacc.c  */
#line 1187 "ntp_parser.y"
    { (yyval.Sim_server) = create_sim_server((yyvsp[(1) - (5)].Address_node), (yyvsp[(3) - (5)].Double), (yyvsp[(4) - (5)].Queue)); }
    break;

  case 257:

/* Line 1464 of yacc.c  */
#line 1191 "ntp_parser.y"
    { (yyval.Double) = (yyvsp[(3) - (4)].Double); }
    break;

  case 258:

/* Line 1464 of yacc.c  */
#line 1195 "ntp_parser.y"
    { (yyval.Address_node) = (yyvsp[(3) - (3)].Address_node); }
    break;

  case 259:

/* Line 1464 of yacc.c  */
#line 1199 "ntp_parser.y"
    { (yyval.Queue) = enqueue((yyvsp[(1) - (2)].Queue), (yyvsp[(2) - (2)].Sim_script)); }
    break;

  case 260:

/* Line 1464 of yacc.c  */
#line 1200 "ntp_parser.y"
    { (yyval.Queue) = enqueue_in_new_queue((yyvsp[(1) - (1)].Sim_script)); }
    break;

  case 261:

/* Line 1464 of yacc.c  */
#line 1205 "ntp_parser.y"
    { (yyval.Sim_script) = create_sim_script_info((yyvsp[(3) - (6)].Double), (yyvsp[(5) - (6)].Queue)); }
    break;

  case 262:

/* Line 1464 of yacc.c  */
#line 1209 "ntp_parser.y"
    { (yyval.Queue) = enqueue((yyvsp[(1) - (3)].Queue), (yyvsp[(2) - (3)].Attr_val)); }
    break;

  case 263:

/* Line 1464 of yacc.c  */
#line 1210 "ntp_parser.y"
    { (yyval.Queue) = enqueue_in_new_queue((yyvsp[(1) - (2)].Attr_val)); }
    break;

  case 264:

/* Line 1464 of yacc.c  */
#line 1215 "ntp_parser.y"
    { (yyval.Attr_val) = create_attr_dval((yyvsp[(1) - (3)].Integer), (yyvsp[(3) - (3)].Double)); }
    break;

  case 265:

/* Line 1464 of yacc.c  */
#line 1217 "ntp_parser.y"
    { (yyval.Attr_val) = create_attr_dval((yyvsp[(1) - (3)].Integer), (yyvsp[(3) - (3)].Double)); }
    break;

  case 266:

/* Line 1464 of yacc.c  */
#line 1219 "ntp_parser.y"
    { (yyval.Attr_val) = create_attr_dval((yyvsp[(1) - (3)].Integer), (yyvsp[(3) - (3)].Double)); }
    break;

  case 267:

/* Line 1464 of yacc.c  */
#line 1221 "ntp_parser.y"
    { (yyval.Attr_val) = create_attr_dval((yyvsp[(1) - (3)].Integer), (yyvsp[(3) - (3)].Double)); }
    break;

  case 268:

/* Line 1464 of yacc.c  */
#line 1223 "ntp_parser.y"
    { (yyval.Attr_val) = create_attr_dval((yyvsp[(1) - (3)].Integer), (yyvsp[(3) - (3)].Double)); }
    break;



/* Line 1464 of yacc.c  */
#line 3367 "ntp_parser.c"
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
      yyerror (YY_("syntax error"));
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
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
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

#if !defined(yyoverflow) || YYERROR_VERBOSE
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
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
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



/* Line 1684 of yacc.c  */
#line 1227 "ntp_parser.y"


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
 * token_name - Convert T_ token integers to text.
 *		Example: token_name(T_Server) returns "T_Server".
 *		see also keyword(T_Server) which returns "server".
 */
const char *
token_name(
	int token
	)
{
	return yytname[YYTRANSLATE(token)];
}


/* Initial Testing function -- ignore
int main(int argc, char *argv[])
{
	ip_file = FOPEN(argv[1], "r");
	if (!ip_file) {
		fprintf(stderr, "ERROR!! Could not open file: %s\n", argv[1]);
	}
	key_scanner = create_keyword_scanner(keyword_list);
	print_keyword_scanner(key_scanner, 0);
	yyparse();
	return 0;
}
*/


