#ifndef lint
#if __GNUC__ - 0 >= 4 || (__GNUC__ - 0 == 3 && __GNUC_MINOR__ >= 1)
__attribute__((__used__))
#endif
static const char yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93";
#endif

#ifdef _LIBC
#include "namespace.h"
#endif
#include <stdlib.h>
#include <string.h>

#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9

#define YYEMPTY        (-1)
#define yyclearin      (yychar = YYEMPTY)
#define yyerrok        (yyerrflag = 0)
#define YYRECOVERING() (yyerrflag != 0)

#define YYPREFIX "yy"

#define YYPURE 0

#line 14 "../../dist/ntpd/ntp_parser.y"
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

  #define YYMALLOC	emalloc
  #define YYFREE	free
  #define YYERROR_VERBOSE
  #define YYMAXDEPTH	1000	/* stop the madness sooner */
  void yyerror(const char *msg);

  #ifdef SIM
  #  define ONLY_SIM(a)	(a)
  #else
  #  define ONLY_SIM(a)	NULL
  #endif
#line 53 "../../dist/ntpd/ntp_parser.y"
#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
typedef union {
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
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
#line 85 "ntp_parser.c"

/* compatibility with bison */
#ifdef YYPARSE_PARAM
/* compatibility with FreeBSD */
# ifdef YYPARSE_PARAM_TYPE
#  define YYPARSE_DECL() yyparse(YYPARSE_PARAM_TYPE YYPARSE_PARAM)
# else
#  define YYPARSE_DECL() yyparse(void *YYPARSE_PARAM)
# endif
#else
# define YYPARSE_DECL() yyparse(void)
#endif

/* Parameters sent to lex. */
#ifdef YYLEX_PARAM
# define YYLEX_DECL() yylex(void *YYLEX_PARAM)
# define YYLEX yylex(YYLEX_PARAM)
#else
# define YYLEX_DECL() yylex(void)
# define YYLEX yylex()
#endif

/* Parameters sent to yyerror. */
#ifndef YYERROR_DECL
#define YYERROR_DECL() yyerror(const char *s)
#endif
#ifndef YYERROR_CALL
#define YYERROR_CALL(msg) yyerror(msg)
#endif

extern int YYPARSE_DECL();


#define T_Abbrev 257
#define T_Age 258
#define T_All 259
#define T_Allan 260
#define T_Allpeers 261
#define T_Auth 262
#define T_Autokey 263
#define T_Automax 264
#define T_Average 265
#define T_Bclient 266
#define T_Bcpollbstep 267
#define T_Beacon 268
#define T_Broadcast 269
#define T_Broadcastclient 270
#define T_Broadcastdelay 271
#define T_Burst 272
#define T_Calibrate 273
#define T_Ceiling 274
#define T_Clockstats 275
#define T_Cohort 276
#define T_ControlKey 277
#define T_Crypto 278
#define T_Cryptostats 279
#define T_Ctl 280
#define T_Day 281
#define T_Default 282
#define T_Digest 283
#define T_Disable 284
#define T_Discard 285
#define T_Dispersion 286
#define T_Double 287
#define T_Driftfile 288
#define T_Drop 289
#define T_Dscp 290
#define T_Ellipsis 291
#define T_Enable 292
#define T_End 293
#define T_False 294
#define T_File 295
#define T_Filegen 296
#define T_Filenum 297
#define T_Flag1 298
#define T_Flag2 299
#define T_Flag3 300
#define T_Flag4 301
#define T_Flake 302
#define T_Floor 303
#define T_Freq 304
#define T_Fudge 305
#define T_Host 306
#define T_Huffpuff 307
#define T_Iburst 308
#define T_Ident 309
#define T_Ignore 310
#define T_Incalloc 311
#define T_Incmem 312
#define T_Initalloc 313
#define T_Initmem 314
#define T_Includefile 315
#define T_Integer 316
#define T_Interface 317
#define T_Intrange 318
#define T_Io 319
#define T_Ipv4 320
#define T_Ipv4_flag 321
#define T_Ipv6 322
#define T_Ipv6_flag 323
#define T_Kernel 324
#define T_Key 325
#define T_Keys 326
#define T_Keysdir 327
#define T_Kod 328
#define T_Mssntp 329
#define T_Leapfile 330
#define T_Leapsmearinterval 331
#define T_Limited 332
#define T_Link 333
#define T_Listen 334
#define T_Logconfig 335
#define T_Logfile 336
#define T_Loopstats 337
#define T_Lowpriotrap 338
#define T_Manycastclient 339
#define T_Manycastserver 340
#define T_Mask 341
#define T_Maxage 342
#define T_Maxclock 343
#define T_Maxdepth 344
#define T_Maxdist 345
#define T_Maxmem 346
#define T_Maxpoll 347
#define T_Mdnstries 348
#define T_Mem 349
#define T_Memlock 350
#define T_Minclock 351
#define T_Mindepth 352
#define T_Mindist 353
#define T_Minimum 354
#define T_Minpoll 355
#define T_Minsane 356
#define T_Mode 357
#define T_Mode7 358
#define T_Monitor 359
#define T_Month 360
#define T_Mru 361
#define T_Multicastclient 362
#define T_Nic 363
#define T_Nolink 364
#define T_Nomodify 365
#define T_Nomrulist 366
#define T_None 367
#define T_Nonvolatile 368
#define T_Nopeer 369
#define T_Noquery 370
#define T_Noselect 371
#define T_Noserve 372
#define T_Notrap 373
#define T_Notrust 374
#define T_Ntp 375
#define T_Ntpport 376
#define T_NtpSignDsocket 377
#define T_Orphan 378
#define T_Orphanwait 379
#define T_PCEdigest 380
#define T_Panic 381
#define T_Peer 382
#define T_Peerstats 383
#define T_Phone 384
#define T_Pid 385
#define T_Pidfile 386
#define T_Pool 387
#define T_Port 388
#define T_Preempt 389
#define T_Prefer 390
#define T_Protostats 391
#define T_Pw 392
#define T_Randfile 393
#define T_Rawstats 394
#define T_Refid 395
#define T_Requestkey 396
#define T_Reset 397
#define T_Restrict 398
#define T_Revoke 399
#define T_Rlimit 400
#define T_Saveconfigdir 401
#define T_Server 402
#define T_Setvar 403
#define T_Source 404
#define T_Stacksize 405
#define T_Statistics 406
#define T_Stats 407
#define T_Statsdir 408
#define T_Step 409
#define T_Stepback 410
#define T_Stepfwd 411
#define T_Stepout 412
#define T_Stratum 413
#define T_String 414
#define T_Sys 415
#define T_Sysstats 416
#define T_Tick 417
#define T_Time1 418
#define T_Time2 419
#define T_Timer 420
#define T_Timingstats 421
#define T_Tinker 422
#define T_Tos 423
#define T_Trap 424
#define T_True 425
#define T_Trustedkey 426
#define T_Ttl 427
#define T_Type 428
#define T_U_int 429
#define T_UEcrypto 430
#define T_UEcryptonak 431
#define T_UEdigest 432
#define T_Unconfig 433
#define T_Unpeer 434
#define T_Version 435
#define T_WanderThreshold 436
#define T_Week 437
#define T_Wildcard 438
#define T_Xleave 439
#define T_Year 440
#define T_Flag 441
#define T_EOC 442
#define T_Simulate 443
#define T_Beep_Delay 444
#define T_Sim_Duration 445
#define T_Server_Offset 446
#define T_Duration 447
#define T_Freq_Offset 448
#define T_Wander 449
#define T_Jitter 450
#define T_Prop_Delay 451
#define T_Proc_Delay 452
#define YYERRCODE 256
static const short yylhs[] = {                           -1,
    0,   87,   87,   87,   88,   88,   88,   88,   88,   88,
   88,   88,   88,   88,   88,   88,   88,   88,   89,    7,
    7,    7,    7,    7,    3,    3,   31,    4,    4,   47,
   47,   44,   44,   44,   45,   46,   46,   46,   46,   46,
   46,   46,   46,   48,   48,   49,   49,   49,   49,   49,
   49,   50,   51,   90,   73,   73,   91,   91,   91,   91,
   92,   92,   92,   92,   92,   92,   92,   92,   92,   11,
   11,   10,   10,   12,   12,   12,   12,   12,   95,   70,
   70,   67,   67,   67,   69,   69,   69,   69,   69,   69,
   69,   69,   69,   68,   68,   93,   93,   93,   57,   57,
   56,   56,   56,   56,   56,   56,   56,   56,   18,   18,
   17,   17,   17,   17,   32,   32,   16,   16,   19,   19,
   19,   19,   19,   19,   19,   94,   94,   94,   94,   94,
   94,   94,   94,    2,    2,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
   15,   15,   13,   14,   14,   14,   41,   41,   39,   40,
   40,   40,   40,   40,   40,   40,   40,   96,   23,   23,
   20,   20,   20,   20,   20,   22,   22,   21,   21,   21,
   21,   97,   55,   55,   54,   53,   53,   53,   98,   98,
   62,   62,   59,   59,   60,   60,   60,   60,   60,   60,
   61,   61,   61,   61,   61,   61,   99,   66,   66,   65,
   64,   64,   64,   64,   64,   64,   64,   64,   64,   64,
  100,  100,  100,  100,  100,  100,  100,  100,  100,  100,
  100,  100,  100,  100,   35,   35,   35,   36,   36,   37,
   37,   38,   38,   38,  102,  102,  102,   74,   63,   63,
   72,   72,   71,   71,   34,   34,   33,   29,   29,   30,
   30,   42,   42,   42,   42,   28,   28,   28,   52,    9,
    9,    8,    8,    8,    8,    8,    8,    8,   24,   24,
   25,   25,   26,   26,   27,   58,   58,    5,    5,    6,
    6,    6,   43,   43,  101,  103,   76,   76,   75,   77,
   77,   78,   78,   79,   80,   81,   83,   83,   82,   85,
   85,   86,   84,   84,   84,   84,   84,
};
static const short yylen[] = {                            2,
    1,    3,    2,    2,    0,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    3,    1,
    1,    1,    1,    1,    1,    2,    1,    1,    1,    0,
    2,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    2,    2,    1,    1,    1,    1,    1,
    1,    2,    1,    2,    1,    1,    1,    2,    2,    2,
    2,    2,    2,    2,    2,    2,    2,    2,    2,    0,
    2,    2,    2,    1,    1,    1,    1,    1,    2,    2,
    1,    2,    2,    2,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    2,    2,    3,    2,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    0,    2,
    2,    2,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    2,    2,    3,    5,    3,
    4,    4,    3,    0,    2,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    2,    1,    2,    1,    1,    1,    2,    1,    2,    1,
    1,    1,    1,    1,    1,    1,    1,    3,    2,    1,
    2,    2,    2,    2,    2,    1,    1,    1,    1,    1,
    1,    2,    2,    1,    2,    1,    1,    1,    2,    2,
    2,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    2,    2,    1,    2,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    2,    2,    2,    2,    3,    1,    2,    2,
    2,    2,    3,    2,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    2,    0,    4,    1,    0,
    0,    2,    2,    2,    2,    1,    1,    3,    3,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    2,    2,
    1,    1,    1,    1,    1,    1,    1,    1,    2,    1,
    2,    1,    1,    1,    5,    2,    1,    2,    1,    1,
    1,    1,    1,    1,    5,    1,    3,    2,    3,    1,
    1,    2,    1,    5,    4,    3,    2,    1,    6,    3,
    2,    3,    1,    1,    1,    1,    1,
};
static const short yydefred[] = {                         0,
    0,    0,   23,   57,  235,    0,   70,    0,    0,    0,
  238,    0,  228,    0,    0,  240,    0,  260,    0,    0,
  241,  239,    0,  242,   24,    0,    0,    0,    0,  261,
  236,    0,   22,    0,  243,   21,    0,    0,    0,    0,
    0,  244,   20,    0,    0,    0,  237,    0,    0,    0,
    0,    0,   55,   56,  296,    0,    0,  221,    0,    0,
    0,    0,    0,  222,    0,    0,    0,    6,    7,    8,
    9,   10,   11,   12,   13,   14,   15,   16,   17,   18,
    0,    4,   61,   62,    0,  195,  196,  197,  198,  201,
  199,  200,  202,  203,  204,  205,  206,  192,  193,  194,
    0,  154,  155,  156,  152,    0,    0,    0,  229,    0,
  101,  102,  103,  104,  108,  105,  106,  107,  109,   28,
   29,   27,    0,    0,   25,    0,   64,   65,  257,  256,
    0,  289,    0,   60,  160,  161,  162,  163,  164,  165,
  166,  167,  158,    0,    0,    0,   69,  287,    0,   66,
  272,  273,  274,  275,  276,  277,  278,  271,    0,  134,
    0,    0,  134,    0,   67,  188,  186,  187,    0,  184,
    0,    0,  232,  100,    0,   97,  211,  212,  213,  214,
  215,  216,  217,  218,  219,  220,    0,  209,    0,   85,
   86,   87,    0,   88,   89,   95,   90,   94,   91,   92,
   93,   81,    0,    0,    0,  251,  283,    0,    0,  282,
  284,  280,    0,   30,  268,  267,  266,    0,  294,  293,
  223,  224,  225,  226,   54,    0,    3,    0,   78,   74,
   75,   76,   77,    0,   71,    0,  191,  153,  151,  246,
    0,    0,  178,  179,  180,  181,    0,    0,  176,  177,
  170,    0,    0,    0,   26,  227,  255,  288,  159,  157,
  286,  270,    0,  134,  134,    0,    0,    0,  185,  183,
    0,   99,  210,  208,  292,  290,  291,   84,   83,   82,
   80,    0,    0,  281,  279,    0,  262,  263,  264,  259,
  265,  258,    2,  300,  301,    0,    0,    0,   73,   72,
  118,  117,    0,  115,  116,    0,  114,  110,  113,  174,
  175,  173,  172,  171,  169,  136,  137,  138,  139,  140,
  141,  142,  143,  144,  145,  146,  147,  148,  149,  150,
  135,    0,    0,  134,    0,    0,    0,  252,    0,   36,
   37,   38,   53,   46,   48,   47,   50,   39,   40,   41,
   42,   49,   51,   43,   31,   32,   35,   33,    0,   34,
    0,  298,    0,    0,    0,  303,    0,    0,  111,  125,
  121,  123,  119,  120,  122,  124,  112,    0,  249,  248,
  254,  253,    0,   44,   45,   52,    0,  297,  295,  302,
    0,  299,  285,  306,    0,    0,    0,    0,  308,    0,
    0,    0,  304,  307,  305,    0,    0,  313,  314,  315,
  316,  317,    0,    0,    0,    0,  309,    0,  311,  312,
  310,
};
static const short yydgoto[] = {                         56,
  331,  263,  132,  124,  133,  278,   57,  158,  159,  235,
   85,  236,  105,  106,  107,  307,  308,  241,  377,  251,
  252,  253,  254,  213,  209,  210,  211,  218,   58,   59,
  125,  309,  130,  131,   60,   61,   62,   63,  143,  144,
  145,  292,  221,  355,  356,  357,  286,  358,  359,  360,
  361,   64,  169,  170,  171,  119,  175,  149,   98,   99,
  100,  101,  380,  187,  188,  189,  202,  203,  204,  205,
  338,  282,   65,  173,  296,  297,  298,  365,  366,  396,
  367,  399,  400,  413,  414,  415,   66,   67,   68,   69,
   70,   71,   72,   73,   74,   75,   76,   77,   78,   79,
   80,  109,   81,
};
static const short yysindex[] = {                      -225,
 -414, -287,    0,    0,    0, -286,    0,   39, -256, -381,
    0,   39,    0, -188, -302,    0, -378,    0, -377, -373,
    0,    0, -360,    0,    0, -302, -260,  -87, -302,    0,
    0, -357,    0, -353,    0,    0, -252, -186, -235, -250,
 -254,    0,    0, -352, -188, -337,    0,   46,  185, -332,
  -34, -223,    0,    0,    0,    0, -302,    0, -255, -267,
 -221, -317, -315,    0, -302,  -17, -342,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  -19,    0,    0,    0, -271,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   39,    0,    0,    0,    0, -209, -256, -179,    0,   39,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0, -174, -305,    0,  -17,    0,    0,    0,    0,
 -360,    0, -302,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0, -203,  -87, -302,    0,    0, -295,    0,
    0,    0,    0,    0,    0,    0,    0,    0, -186,    0,
 -153, -150,    0, -207,    0,    0,    0,    0, -177,    0,
 -254,   79,    0,    0, -188,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0, -267,    0,   46,    0,
    0,    0, -284,    0,    0,    0,    0,    0,    0,    0,
    0,    0, -267, -166,  185,    0,    0, -163,  -34,    0,
    0,    0, -162,    0,    0,    0,    0, -248,    0,    0,
    0,    0,    0,    0,    0, -300,    0, -419,    0,    0,
    0,    0,    0, -161,    0, -258,    0,    0,    0,    0,
 -244, -249,    0,    0,    0,    0, -246, -158,    0,    0,
    0, -284, -267, -174,    0,    0,    0,    0,    0,    0,
    0,    0,  113,    0,    0,  113, -332,  113,    0,    0,
 -234,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0, -303, -127,    0,    0,  -14,    0,    0,    0,    0,
    0,    0,    0,    0,    0, -272, -375,  121,    0,    0,
    0,    0, -227,    0,    0, -200,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  113,  113,    0,  -93, -332, -125,    0, -122,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0, -299,    0,
 -218,    0,  132, -242, -121,    0,   81, -267,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  113,    0,    0,
    0,    0,  164,    0,    0,    0, -302,    0,    0,    0,
 -239,    0,    0,    0,  149, -236, -267,  151,    0, -117,
 -229, -267,    0,    0,    0,   91, -304,    0,    0,    0,
    0,    0,  154, -123, -226, -267,    0, -219,    0,    0,
    0,
};
static const short yyrindex[] = {                      -212,
    0,    0,    0,    0,    0,    0,    0,    0,    0, -211,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    1,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0, -210,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
 -204,    0,    0,    0,    0,    0, -201, -199,    0, -196,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0, -212,    0,    0,    0,    0,
 -194,    0, -192,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0, -191, -178,    0,    0, -176,    0,
    0,    0,    0,    0,    0,    0,    0,    0, -173,    0,
 -195, -172,    0,   64,    0,    0,    0,    0,    0,    0,
 -170,    0,    0,    0, -168,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0, -165,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0, -164,    0,    0,    0, -159,    0,
    0,    0, -157,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
 -156,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0, -155,    0,    0,    0,    0,    0,    0,
    0,    0, -152,    0,    0, -151,    0, -149,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0, -146,    0,    0,    0, -145,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0, -143, -140,    0, -139,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0, -138,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,
};
static const short yygindex[] = {                         0,
    0, -148,  -15,    0,  206,    4,    0,  103,    0,    0,
    0,    0,  156,    0,    0,    0,    0,    0,    0,   16,
    0,    0,    0,    0,    0,   71,    0,    0,    0,    0,
  -47,    0,  153,    0,    0,    0,    0,    0,  162,    0,
    0,    0, -180,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  137,    0,  -40,    0,    0,  -88,    0,
    0,  303,    0,    0,  127,    0,  112,    0,    0,    0,
    0,    0,    0,    0,   23,    0,    0,    0,  -44,    0,
    0,  -76,    0,    0,    0,  -80,    0,  -48,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,
};
#define YYTABLESIZE 564
static const short yytable[] = {                        123,
    1,  417,  206,  389,  174,  208,  273,  403,  102,  275,
  287,  229,  237,  336,  266,  268,  384,  226,  120,  219,
  121,  237,  279,  164,  294,  295,  363,   82,   83,   84,
    1,  276,  108,  215,  230,  126,  127,  231,    2,  301,
  128,  214,  166,    3,    4,    5,  160,  302,  220,  225,
  303,    6,    7,  129,  216,  134,  147,  370,    8,    9,
  148,  172,   10,  150,   11,  165,   12,   13,  294,  295,
   14,  288,  314,  289,  151,  152,  176,  256,  217,   15,
  371,  122,  242,   16,  337,  161,  111,  162,  304,   17,
  112,   18,  212,  153,  222,  167,  223,  103,  224,  227,
   19,   20,  104,  228,   21,   22,  238,  240,  255,   23,
   24,  122,  259,   25,   26,  332,  333,  258,  261,  305,
  232,  233,   27,  243,  244,  245,  246,  234,  264,  385,
  258,  265,  154,  267,  272,   28,   29,   30,  269,  271,
  277,  293,   31,  408,  409,  410,  411,  412,  113,  280,
  168,   32,  283,  285,  299,  300,   33,  312,   34,  372,
   35,   36,  155,  339,  310,  290,  373,  311,  163,  362,
   37,   38,   39,   40,   41,   42,   43,   44,  122,  335,
   45,  368,   46,  306,  374,  378,  369,  392,  379,  291,
  382,   47,  387,  383,  114,  386,   48,   49,   50,  388,
   51,   52,  115,  391,  393,  116,  395,   53,   54,  397,
  398,  402,  405,  407,  416,  419,  401,   55,   28,  334,
  247,  406,  421,  135,  136,  137,  138,  117,  156,    5,
  247,   63,  118,  157,  146,  420,  375,  190,  248,  376,
  126,   29,  245,  249,  250,  189,    2,  230,  340,   58,
  127,    3,    4,    5,  139,  313,  140,  341,  141,    6,
    7,  262,  239,   59,  142,  231,    8,    9,  269,  315,
   10,  182,   11,   96,   12,   13,  207,   79,   14,  284,
  363,  207,   68,  257,  234,   98,  168,   15,  381,  130,
  133,   16,  128,  342,  343,  233,   19,   17,  131,   18,
   86,  132,  250,  129,   87,  177,  260,  270,   19,   20,
  344,   88,   21,   22,  110,  274,  281,   23,   24,  364,
  390,   25,   26,  404,  408,  409,  410,  411,  412,  398,
   27,  178,  345,  418,    0,    0,    0,    0,    0,    0,
  346,    0,  347,   28,   29,   30,    0,    0,    0,  179,
   31,    0,  180,    0,    0,    0,  348,    0,    0,   32,
    0,    0,   89,    0,   33,  134,   34,    0,   35,   36,
    0,  394,    0,  134,  349,  350,    0,    0,   37,   38,
   39,   40,   41,   42,   43,   44,    0,    0,   45,    0,
   46,  134,  134,    0,    0,  134,   90,   91,    0,   47,
    0,  134,    0,    0,   48,   49,   50,    0,   51,   52,
  351,    0,  352,   92,  316,   53,   54,    0,   93,    0,
  353,    0,  317,    0,  354,   55,  181,    0,  134,  134,
    0,    0,  134,  134,    0,  134,  134,  134,    0,  134,
  318,  319,    5,    0,  320,   94,    0,    0,    0,    0,
  321,  190,  191,    0,  182,  183,  184,  185,  192,    0,
  193,    0,  186,    0,    0,    0,    0,    0,   95,   96,
   97,    0,    0,    0,    0,    0,    0,  322,  323,    0,
    0,  324,  325,    0,  326,  327,  328,  194,  329,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  134,    0,
    0,    0,    0,    0,    0,  134,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  195,    0,  196,
    0,    0,    0,    0,    0,  197,    0,  198,    0,    0,
  199,    0,    0,    0,    0,    0,    0,  330,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,  200,  201,
};
static const short yycheck[] = {                         15,
    0,  125,   50,  125,   45,   40,  187,  125,  265,  294,
  259,  283,  101,  317,  163,  164,  316,   66,  321,  287,
  323,  110,  203,   39,  444,  445,  402,  442,  316,  316,
  256,  316,  414,  289,  306,  414,  414,  309,  264,  284,
  414,   57,  297,  269,  270,  271,  282,  292,  316,   65,
  295,  277,  278,  414,  310,  316,  414,  258,  284,  285,
  414,  414,  288,  316,  290,  316,  292,  293,  444,  445,
  296,  320,  253,  322,  261,  262,  414,  126,  334,  305,
  281,  414,  257,  309,  388,  321,  275,  323,  333,  315,
  279,  317,  316,  280,  316,  350,  414,  354,  414,  442,
  326,  327,  359,  123,  330,  331,  316,  287,  414,  335,
  336,  414,  316,  339,  340,  264,  265,  133,  414,  364,
  392,  393,  348,  298,  299,  300,  301,  399,  282,  429,
  146,  282,  319,  341,  175,  361,  362,  363,  316,   61,
  425,  442,  368,  448,  449,  450,  451,  452,  337,  316,
  405,  377,  316,  316,  316,  414,  382,  316,  384,  360,
  386,  387,  349,  291,  414,  414,  367,  414,  404,  442,
  396,  397,  398,  399,  400,  401,  402,  403,  414,  414,
  406,   61,  408,  428,  385,  334,  414,  368,  282,  438,
  316,  417,   61,  316,  383,  414,  422,  423,  424,  442,
  426,  427,  391,  123,   41,  394,  446,  433,  434,   61,
  447,   61,  442,  123,   61,  442,  397,  443,  414,  267,
  395,  402,  442,  311,  312,  313,  314,  416,  415,  442,
  442,  442,  421,  420,   29,  416,  437,  442,  413,  440,
  442,  414,  442,  418,  419,  442,  264,  442,  263,  442,
  442,  269,  270,  271,  342,  252,  344,  272,  346,  277,
  278,  159,  107,  442,  352,  442,  284,  285,  442,  254,
  288,  442,  290,  442,  292,  293,  442,  442,  296,  209,
  402,  316,  442,  131,  442,  442,  442,  305,  336,  442,
  442,  309,  442,  308,  309,  442,  442,  315,  442,  317,
  262,  442,  442,  442,  266,  260,  145,  171,  326,  327,
  325,  273,  330,  331,   12,  189,  205,  335,  336,  297,
  365,  339,  340,  400,  448,  449,  450,  451,  452,  447,
  348,  286,  347,  414,   -1,   -1,   -1,   -1,   -1,   -1,
  355,   -1,  357,  361,  362,  363,   -1,   -1,   -1,  304,
  368,   -1,  307,   -1,   -1,   -1,  371,   -1,   -1,  377,
   -1,   -1,  324,   -1,  382,  302,  384,   -1,  386,  387,
   -1,  387,   -1,  310,  389,  390,   -1,   -1,  396,  397,
  398,  399,  400,  401,  402,  403,   -1,   -1,  406,   -1,
  408,  328,  329,   -1,   -1,  332,  358,  359,   -1,  417,
   -1,  338,   -1,   -1,  422,  423,  424,   -1,  426,  427,
  425,   -1,  427,  375,  302,  433,  434,   -1,  380,   -1,
  435,   -1,  310,   -1,  439,  443,  381,   -1,  365,  366,
   -1,   -1,  369,  370,   -1,  372,  373,  374,   -1,  376,
  328,  329,  442,   -1,  332,  407,   -1,   -1,   -1,   -1,
  338,  267,  268,   -1,  409,  410,  411,  412,  274,   -1,
  276,   -1,  417,   -1,   -1,   -1,   -1,   -1,  430,  431,
  432,   -1,   -1,   -1,   -1,   -1,   -1,  365,  366,   -1,
   -1,  369,  370,   -1,  372,  373,  374,  303,  376,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  435,   -1,
   -1,   -1,   -1,   -1,   -1,  442,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  343,   -1,  345,
   -1,   -1,   -1,   -1,   -1,  351,   -1,  353,   -1,   -1,
  356,   -1,   -1,   -1,   -1,   -1,   -1,  435,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,  378,  379,
};
#define YYFINAL 56
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 452
#define YYTRANSLATE(a) ((a) > YYMAXTOKEN ? (YYMAXTOKEN + 1) : (a))
static const char *yytname[] = {

"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,"'('","')'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'='",0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'{'",0,"'}'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"T_Abbrev",
"T_Age","T_All","T_Allan","T_Allpeers","T_Auth","T_Autokey","T_Automax",
"T_Average","T_Bclient","T_Bcpollbstep","T_Beacon","T_Broadcast",
"T_Broadcastclient","T_Broadcastdelay","T_Burst","T_Calibrate","T_Ceiling",
"T_Clockstats","T_Cohort","T_ControlKey","T_Crypto","T_Cryptostats","T_Ctl",
"T_Day","T_Default","T_Digest","T_Disable","T_Discard","T_Dispersion",
"T_Double","T_Driftfile","T_Drop","T_Dscp","T_Ellipsis","T_Enable","T_End",
"T_False","T_File","T_Filegen","T_Filenum","T_Flag1","T_Flag2","T_Flag3",
"T_Flag4","T_Flake","T_Floor","T_Freq","T_Fudge","T_Host","T_Huffpuff",
"T_Iburst","T_Ident","T_Ignore","T_Incalloc","T_Incmem","T_Initalloc",
"T_Initmem","T_Includefile","T_Integer","T_Interface","T_Intrange","T_Io",
"T_Ipv4","T_Ipv4_flag","T_Ipv6","T_Ipv6_flag","T_Kernel","T_Key","T_Keys",
"T_Keysdir","T_Kod","T_Mssntp","T_Leapfile","T_Leapsmearinterval","T_Limited",
"T_Link","T_Listen","T_Logconfig","T_Logfile","T_Loopstats","T_Lowpriotrap",
"T_Manycastclient","T_Manycastserver","T_Mask","T_Maxage","T_Maxclock",
"T_Maxdepth","T_Maxdist","T_Maxmem","T_Maxpoll","T_Mdnstries","T_Mem",
"T_Memlock","T_Minclock","T_Mindepth","T_Mindist","T_Minimum","T_Minpoll",
"T_Minsane","T_Mode","T_Mode7","T_Monitor","T_Month","T_Mru",
"T_Multicastclient","T_Nic","T_Nolink","T_Nomodify","T_Nomrulist","T_None",
"T_Nonvolatile","T_Nopeer","T_Noquery","T_Noselect","T_Noserve","T_Notrap",
"T_Notrust","T_Ntp","T_Ntpport","T_NtpSignDsocket","T_Orphan","T_Orphanwait",
"T_PCEdigest","T_Panic","T_Peer","T_Peerstats","T_Phone","T_Pid","T_Pidfile",
"T_Pool","T_Port","T_Preempt","T_Prefer","T_Protostats","T_Pw","T_Randfile",
"T_Rawstats","T_Refid","T_Requestkey","T_Reset","T_Restrict","T_Revoke",
"T_Rlimit","T_Saveconfigdir","T_Server","T_Setvar","T_Source","T_Stacksize",
"T_Statistics","T_Stats","T_Statsdir","T_Step","T_Stepback","T_Stepfwd",
"T_Stepout","T_Stratum","T_String","T_Sys","T_Sysstats","T_Tick","T_Time1",
"T_Time2","T_Timer","T_Timingstats","T_Tinker","T_Tos","T_Trap","T_True",
"T_Trustedkey","T_Ttl","T_Type","T_U_int","T_UEcrypto","T_UEcryptonak",
"T_UEdigest","T_Unconfig","T_Unpeer","T_Version","T_WanderThreshold","T_Week",
"T_Wildcard","T_Xleave","T_Year","T_Flag","T_EOC","T_Simulate","T_Beep_Delay",
"T_Sim_Duration","T_Server_Offset","T_Duration","T_Freq_Offset","T_Wander",
"T_Jitter","T_Prop_Delay","T_Proc_Delay","illegal-token",
};
#if YYDEBUG
static const char *yyrule[] = {
"$accept : configuration",
"configuration : command_list",
"command_list : command_list command T_EOC",
"command_list : command T_EOC",
"command_list : error T_EOC",
"command :",
"command : server_command",
"command : unpeer_command",
"command : other_mode_command",
"command : authentication_command",
"command : monitoring_command",
"command : access_control_command",
"command : orphan_mode_command",
"command : fudge_command",
"command : rlimit_command",
"command : system_option_command",
"command : tinker_command",
"command : miscellaneous_command",
"command : simulate_command",
"server_command : client_type address option_list",
"client_type : T_Server",
"client_type : T_Pool",
"client_type : T_Peer",
"client_type : T_Broadcast",
"client_type : T_Manycastclient",
"address : ip_address",
"address : address_fam T_String",
"ip_address : T_String",
"address_fam : T_Ipv4_flag",
"address_fam : T_Ipv6_flag",
"option_list :",
"option_list : option_list option",
"option : option_flag",
"option : option_int",
"option : option_str",
"option_flag : option_flag_keyword",
"option_flag_keyword : T_Autokey",
"option_flag_keyword : T_Burst",
"option_flag_keyword : T_Iburst",
"option_flag_keyword : T_Noselect",
"option_flag_keyword : T_Preempt",
"option_flag_keyword : T_Prefer",
"option_flag_keyword : T_True",
"option_flag_keyword : T_Xleave",
"option_int : option_int_keyword T_Integer",
"option_int : option_int_keyword T_U_int",
"option_int_keyword : T_Key",
"option_int_keyword : T_Minpoll",
"option_int_keyword : T_Maxpoll",
"option_int_keyword : T_Ttl",
"option_int_keyword : T_Mode",
"option_int_keyword : T_Version",
"option_str : option_str_keyword T_String",
"option_str_keyword : T_Ident",
"unpeer_command : unpeer_keyword address",
"unpeer_keyword : T_Unconfig",
"unpeer_keyword : T_Unpeer",
"other_mode_command : T_Broadcastclient",
"other_mode_command : T_Manycastserver address_list",
"other_mode_command : T_Multicastclient address_list",
"other_mode_command : T_Mdnstries T_Integer",
"authentication_command : T_Automax T_Integer",
"authentication_command : T_ControlKey T_Integer",
"authentication_command : T_Crypto crypto_command_list",
"authentication_command : T_Keys T_String",
"authentication_command : T_Keysdir T_String",
"authentication_command : T_Requestkey T_Integer",
"authentication_command : T_Revoke T_Integer",
"authentication_command : T_Trustedkey integer_list_range",
"authentication_command : T_NtpSignDsocket T_String",
"crypto_command_list :",
"crypto_command_list : crypto_command_list crypto_command",
"crypto_command : crypto_str_keyword T_String",
"crypto_command : T_Revoke T_Integer",
"crypto_str_keyword : T_Host",
"crypto_str_keyword : T_Ident",
"crypto_str_keyword : T_Pw",
"crypto_str_keyword : T_Randfile",
"crypto_str_keyword : T_Digest",
"orphan_mode_command : T_Tos tos_option_list",
"tos_option_list : tos_option_list tos_option",
"tos_option_list : tos_option",
"tos_option : tos_option_int_keyword T_Integer",
"tos_option : tos_option_dbl_keyword number",
"tos_option : T_Cohort boolean",
"tos_option_int_keyword : T_Bcpollbstep",
"tos_option_int_keyword : T_Beacon",
"tos_option_int_keyword : T_Ceiling",
"tos_option_int_keyword : T_Floor",
"tos_option_int_keyword : T_Maxclock",
"tos_option_int_keyword : T_Minclock",
"tos_option_int_keyword : T_Minsane",
"tos_option_int_keyword : T_Orphan",
"tos_option_int_keyword : T_Orphanwait",
"tos_option_dbl_keyword : T_Mindist",
"tos_option_dbl_keyword : T_Maxdist",
"monitoring_command : T_Statistics stats_list",
"monitoring_command : T_Statsdir T_String",
"monitoring_command : T_Filegen stat filegen_option_list",
"stats_list : stats_list stat",
"stats_list : stat",
"stat : T_Clockstats",
"stat : T_Cryptostats",
"stat : T_Loopstats",
"stat : T_Peerstats",
"stat : T_Rawstats",
"stat : T_Sysstats",
"stat : T_Timingstats",
"stat : T_Protostats",
"filegen_option_list :",
"filegen_option_list : filegen_option_list filegen_option",
"filegen_option : T_File T_String",
"filegen_option : T_Type filegen_type",
"filegen_option : link_nolink",
"filegen_option : enable_disable",
"link_nolink : T_Link",
"link_nolink : T_Nolink",
"enable_disable : T_Enable",
"enable_disable : T_Disable",
"filegen_type : T_None",
"filegen_type : T_Pid",
"filegen_type : T_Day",
"filegen_type : T_Week",
"filegen_type : T_Month",
"filegen_type : T_Year",
"filegen_type : T_Age",
"access_control_command : T_Discard discard_option_list",
"access_control_command : T_Mru mru_option_list",
"access_control_command : T_Restrict address ac_flag_list",
"access_control_command : T_Restrict address T_Mask ip_address ac_flag_list",
"access_control_command : T_Restrict T_Default ac_flag_list",
"access_control_command : T_Restrict T_Ipv4_flag T_Default ac_flag_list",
"access_control_command : T_Restrict T_Ipv6_flag T_Default ac_flag_list",
"access_control_command : T_Restrict T_Source ac_flag_list",
"ac_flag_list :",
"ac_flag_list : ac_flag_list access_control_flag",
"access_control_flag : T_Flake",
"access_control_flag : T_Ignore",
"access_control_flag : T_Kod",
"access_control_flag : T_Mssntp",
"access_control_flag : T_Limited",
"access_control_flag : T_Lowpriotrap",
"access_control_flag : T_Nomodify",
"access_control_flag : T_Nomrulist",
"access_control_flag : T_Nopeer",
"access_control_flag : T_Noquery",
"access_control_flag : T_Noserve",
"access_control_flag : T_Notrap",
"access_control_flag : T_Notrust",
"access_control_flag : T_Ntpport",
"access_control_flag : T_Version",
"discard_option_list : discard_option_list discard_option",
"discard_option_list : discard_option",
"discard_option : discard_option_keyword T_Integer",
"discard_option_keyword : T_Average",
"discard_option_keyword : T_Minimum",
"discard_option_keyword : T_Monitor",
"mru_option_list : mru_option_list mru_option",
"mru_option_list : mru_option",
"mru_option : mru_option_keyword T_Integer",
"mru_option_keyword : T_Incalloc",
"mru_option_keyword : T_Incmem",
"mru_option_keyword : T_Initalloc",
"mru_option_keyword : T_Initmem",
"mru_option_keyword : T_Maxage",
"mru_option_keyword : T_Maxdepth",
"mru_option_keyword : T_Maxmem",
"mru_option_keyword : T_Mindepth",
"fudge_command : T_Fudge address fudge_factor_list",
"fudge_factor_list : fudge_factor_list fudge_factor",
"fudge_factor_list : fudge_factor",
"fudge_factor : fudge_factor_dbl_keyword number",
"fudge_factor : fudge_factor_bool_keyword boolean",
"fudge_factor : T_Stratum T_Integer",
"fudge_factor : T_Abbrev T_String",
"fudge_factor : T_Refid T_String",
"fudge_factor_dbl_keyword : T_Time1",
"fudge_factor_dbl_keyword : T_Time2",
"fudge_factor_bool_keyword : T_Flag1",
"fudge_factor_bool_keyword : T_Flag2",
"fudge_factor_bool_keyword : T_Flag3",
"fudge_factor_bool_keyword : T_Flag4",
"rlimit_command : T_Rlimit rlimit_option_list",
"rlimit_option_list : rlimit_option_list rlimit_option",
"rlimit_option_list : rlimit_option",
"rlimit_option : rlimit_option_keyword T_Integer",
"rlimit_option_keyword : T_Memlock",
"rlimit_option_keyword : T_Stacksize",
"rlimit_option_keyword : T_Filenum",
"system_option_command : T_Enable system_option_list",
"system_option_command : T_Disable system_option_list",
"system_option_list : system_option_list system_option",
"system_option_list : system_option",
"system_option : system_option_flag_keyword",
"system_option : system_option_local_flag_keyword",
"system_option_flag_keyword : T_Auth",
"system_option_flag_keyword : T_Bclient",
"system_option_flag_keyword : T_Calibrate",
"system_option_flag_keyword : T_Kernel",
"system_option_flag_keyword : T_Monitor",
"system_option_flag_keyword : T_Ntp",
"system_option_local_flag_keyword : T_Mode7",
"system_option_local_flag_keyword : T_PCEdigest",
"system_option_local_flag_keyword : T_Stats",
"system_option_local_flag_keyword : T_UEcrypto",
"system_option_local_flag_keyword : T_UEcryptonak",
"system_option_local_flag_keyword : T_UEdigest",
"tinker_command : T_Tinker tinker_option_list",
"tinker_option_list : tinker_option_list tinker_option",
"tinker_option_list : tinker_option",
"tinker_option : tinker_option_keyword number",
"tinker_option_keyword : T_Allan",
"tinker_option_keyword : T_Dispersion",
"tinker_option_keyword : T_Freq",
"tinker_option_keyword : T_Huffpuff",
"tinker_option_keyword : T_Panic",
"tinker_option_keyword : T_Step",
"tinker_option_keyword : T_Stepback",
"tinker_option_keyword : T_Stepfwd",
"tinker_option_keyword : T_Stepout",
"tinker_option_keyword : T_Tick",
"miscellaneous_command : interface_command",
"miscellaneous_command : reset_command",
"miscellaneous_command : misc_cmd_dbl_keyword number",
"miscellaneous_command : misc_cmd_int_keyword T_Integer",
"miscellaneous_command : misc_cmd_str_keyword T_String",
"miscellaneous_command : misc_cmd_str_lcl_keyword T_String",
"miscellaneous_command : T_Includefile T_String command",
"miscellaneous_command : T_End",
"miscellaneous_command : T_Driftfile drift_parm",
"miscellaneous_command : T_Logconfig log_config_list",
"miscellaneous_command : T_Phone string_list",
"miscellaneous_command : T_Setvar variable_assign",
"miscellaneous_command : T_Trap ip_address trap_option_list",
"miscellaneous_command : T_Ttl integer_list",
"misc_cmd_dbl_keyword : T_Broadcastdelay",
"misc_cmd_dbl_keyword : T_Nonvolatile",
"misc_cmd_dbl_keyword : T_Tick",
"misc_cmd_int_keyword : T_Dscp",
"misc_cmd_int_keyword : T_Leapsmearinterval",
"misc_cmd_str_keyword : T_Ident",
"misc_cmd_str_keyword : T_Leapfile",
"misc_cmd_str_lcl_keyword : T_Logfile",
"misc_cmd_str_lcl_keyword : T_Pidfile",
"misc_cmd_str_lcl_keyword : T_Saveconfigdir",
"drift_parm : T_String",
"drift_parm : T_String T_Double",
"drift_parm :",
"variable_assign : T_String '=' T_String t_default_or_zero",
"t_default_or_zero : T_Default",
"t_default_or_zero :",
"trap_option_list :",
"trap_option_list : trap_option_list trap_option",
"trap_option : T_Port T_Integer",
"trap_option : T_Interface ip_address",
"log_config_list : log_config_list log_config_command",
"log_config_list : log_config_command",
"log_config_command : T_String",
"interface_command : interface_nic nic_rule_action nic_rule_class",
"interface_command : interface_nic nic_rule_action T_String",
"interface_nic : T_Interface",
"interface_nic : T_Nic",
"nic_rule_class : T_All",
"nic_rule_class : T_Ipv4",
"nic_rule_class : T_Ipv6",
"nic_rule_class : T_Wildcard",
"nic_rule_action : T_Listen",
"nic_rule_action : T_Ignore",
"nic_rule_action : T_Drop",
"reset_command : T_Reset counter_set_list",
"counter_set_list : counter_set_list counter_set_keyword",
"counter_set_list : counter_set_keyword",
"counter_set_keyword : T_Allpeers",
"counter_set_keyword : T_Auth",
"counter_set_keyword : T_Ctl",
"counter_set_keyword : T_Io",
"counter_set_keyword : T_Mem",
"counter_set_keyword : T_Sys",
"counter_set_keyword : T_Timer",
"integer_list : integer_list T_Integer",
"integer_list : T_Integer",
"integer_list_range : integer_list_range integer_list_range_elt",
"integer_list_range : integer_list_range_elt",
"integer_list_range_elt : T_Integer",
"integer_list_range_elt : integer_range",
"integer_range : '(' T_Integer T_Ellipsis T_Integer ')'",
"string_list : string_list T_String",
"string_list : T_String",
"address_list : address_list address",
"address_list : address",
"boolean : T_Integer",
"boolean : T_True",
"boolean : T_False",
"number : T_Integer",
"number : T_Double",
"simulate_command : sim_conf_start '{' sim_init_statement_list sim_server_list '}'",
"sim_conf_start : T_Simulate",
"sim_init_statement_list : sim_init_statement_list sim_init_statement T_EOC",
"sim_init_statement_list : sim_init_statement T_EOC",
"sim_init_statement : sim_init_keyword '=' number",
"sim_init_keyword : T_Beep_Delay",
"sim_init_keyword : T_Sim_Duration",
"sim_server_list : sim_server_list sim_server",
"sim_server_list : sim_server",
"sim_server : sim_server_name '{' sim_server_offset sim_act_list '}'",
"sim_server_offset : T_Server_Offset '=' number T_EOC",
"sim_server_name : T_Server '=' address",
"sim_act_list : sim_act_list sim_act",
"sim_act_list : sim_act",
"sim_act : T_Duration '=' number '{' sim_act_stmt_list '}'",
"sim_act_stmt_list : sim_act_stmt_list sim_act_stmt T_EOC",
"sim_act_stmt_list : sim_act_stmt T_EOC",
"sim_act_stmt : sim_act_keyword '=' number",
"sim_act_keyword : T_Freq_Offset",
"sim_act_keyword : T_Wander",
"sim_act_keyword : T_Jitter",
"sim_act_keyword : T_Prop_Delay",
"sim_act_keyword : T_Proc_Delay",

};
#endif

int      yydebug;
int      yynerrs;

int      yyerrflag;
int      yychar;
YYSTYPE  yyval;
YYSTYPE  yylval;

/* define the initial stack-sizes */
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH  YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 500
#define YYMAXDEPTH  500
#endif
#endif

#define YYINITSTACKSIZE 500

typedef struct {
    unsigned stacksize;
    short    *s_base;
    short    *s_mark;
    short    *s_last;
    YYSTYPE  *l_base;
    YYSTYPE  *l_mark;
} YYSTACKDATA;
/* variables for the parser stack */
static YYSTACKDATA yystack;
#line 1635 "../../dist/ntpd/ntp_parser.y"

void
yyerror(
	const char *msg
	)
{
	int retval;
	struct FILE_INFO * ip_ctx;

	ip_ctx = lex_current();
	ip_ctx->errpos = ip_ctx->tokpos;

	msyslog(LOG_ERR, "line %d column %d %s",
		ip_ctx->errpos.nline, ip_ctx->errpos.ncol, msg);
	if (!lex_from_file()) {
		/* Save the error message in the correct buffer */
		retval = snprintf(remote_config.err_msg + remote_config.err_pos,
				  MAXLINE - remote_config.err_pos,
				  "column %d %s",
				  ip_ctx->errpos.ncol, msg);

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

#line 1125 "ntp_parser.c"

#if YYDEBUG
#include <stdio.h>		/* needed for printf */
#endif

#include <stdlib.h>	/* needed for malloc, etc */
#include <string.h>	/* needed for memset */

/* allocate initial stack or double stack size, up to YYMAXDEPTH */
static int yygrowstack(YYSTACKDATA *data)
{
    int i;
    unsigned newsize;
    short *newss;
    YYSTYPE *newvs;

    if ((newsize = data->stacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return -1;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;

    i = (int) (data->s_mark - data->s_base);
    newss = (short *)realloc(data->s_base, newsize * sizeof(*newss));
    if (newss == 0)
        return -1;

    data->s_base = newss;
    data->s_mark = newss + i;

    newvs = (YYSTYPE *)realloc(data->l_base, newsize * sizeof(*newvs));
    if (newvs == 0)
        return -1;

    data->l_base = newvs;
    data->l_mark = newvs + i;

    data->stacksize = newsize;
    data->s_last = data->s_base + newsize - 1;
    return 0;
}

#if YYPURE || defined(YY_NO_LEAKS)
static void yyfreestack(YYSTACKDATA *data)
{
    free(data->s_base);
    free(data->l_base);
    memset(data, 0, sizeof(*data));
}
#else
#define yyfreestack(data) /* nothing */
#endif

#define YYABORT  goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR  goto yyerrlab

int
YYPARSE_DECL()
{
    int yym, yyn, yystate;
#if YYDEBUG
    const char *yys;

    if ((yys = getenv("YYDEBUG")) != 0)
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = YYEMPTY;
    yystate = 0;

#if YYPURE
    memset(&yystack, 0, sizeof(yystack));
#endif

    if (yystack.s_base == NULL && yygrowstack(&yystack)) goto yyoverflow;
    yystack.s_mark = yystack.s_base;
    yystack.l_mark = yystack.l_base;
    yystate = 0;
    *yystack.s_mark = 0;

yyloop:
    if ((yyn = yydefred[yystate]) != 0) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = YYLEX) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = yytname[YYTRANSLATE(yychar)];
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack))
        {
            goto yyoverflow;
        }
        yystate = yytable[yyn];
        *++yystack.s_mark = yytable[yyn];
        *++yystack.l_mark = yylval;
        yychar = YYEMPTY;
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;

    yyerror("syntax error");

    goto yyerrlab;

yyerrlab:
    ++yynerrs;

yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yystack.s_mark]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yystack.s_mark, yytable[yyn]);
#endif
                if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack))
                {
                    goto yyoverflow;
                }
                yystate = yytable[yyn];
                *++yystack.s_mark = yytable[yyn];
                *++yystack.l_mark = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yystack.s_mark);
#endif
                if (yystack.s_mark <= yystack.s_base) goto yyabort;
                --yystack.s_mark;
                --yystack.l_mark;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = yytname[YYTRANSLATE(yychar)];
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = YYEMPTY;
        goto yyloop;
    }

yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    if (yym)
        yyval = yystack.l_mark[1-yym];
    else
        memset(&yyval, 0, sizeof yyval);
    switch (yyn)
    {
case 4:
#line 380 "../../dist/ntpd/ntp_parser.y"
	{
			/* I will need to incorporate much more fine grained
			 * error messages. The following should suffice for
			 * the time being.
			 */
			struct FILE_INFO * ip_ctx = lex_current();
			msyslog(LOG_ERR,
				"syntax error in %s line %d, column %d",
				ip_ctx->fname,
				ip_ctx->errpos.nline,
				ip_ctx->errpos.ncol);
		}
break;
case 19:
#line 416 "../../dist/ntpd/ntp_parser.y"
	{
			peer_node *my_node;

			my_node = create_peer_node(yystack.l_mark[-2].Integer, yystack.l_mark[-1].Address_node, yystack.l_mark[0].Attr_val_fifo);
			APPEND_G_FIFO(cfgt.peers, my_node);
		}
break;
case 26:
#line 435 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Address_node = create_address_node(yystack.l_mark[0].String, yystack.l_mark[-1].Integer); }
break;
case 27:
#line 440 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Address_node = create_address_node(yystack.l_mark[0].String, AF_UNSPEC); }
break;
case 28:
#line 445 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Integer = AF_INET; }
break;
case 29:
#line 447 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Integer = AF_INET6; }
break;
case 30:
#line 452 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val_fifo = NULL; }
break;
case 31:
#line 454 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 35:
#line 468 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(T_Flag, yystack.l_mark[0].Integer); }
break;
case 44:
#line 484 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 45:
#line 486 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_uval(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 52:
#line 500 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_sval(yystack.l_mark[-1].Integer, yystack.l_mark[0].String); }
break;
case 54:
#line 514 "../../dist/ntpd/ntp_parser.y"
	{
			unpeer_node *my_node;

			my_node = create_unpeer_node(yystack.l_mark[0].Address_node);
			if (my_node)
				APPEND_G_FIFO(cfgt.unpeers, my_node);
		}
break;
case 57:
#line 535 "../../dist/ntpd/ntp_parser.y"
	{ cfgt.broadcastclient = 1; }
break;
case 58:
#line 537 "../../dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.manycastserver, yystack.l_mark[0].Address_fifo); }
break;
case 59:
#line 539 "../../dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.multicastclient, yystack.l_mark[0].Address_fifo); }
break;
case 60:
#line 541 "../../dist/ntpd/ntp_parser.y"
	{ cfgt.mdnstries = yystack.l_mark[0].Integer; }
break;
case 61:
#line 552 "../../dist/ntpd/ntp_parser.y"
	{
			attr_val *atrv;

			atrv = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer);
			APPEND_G_FIFO(cfgt.vars, atrv);
		}
break;
case 62:
#line 559 "../../dist/ntpd/ntp_parser.y"
	{ cfgt.auth.control_key = yystack.l_mark[0].Integer; }
break;
case 63:
#line 561 "../../dist/ntpd/ntp_parser.y"
	{
			cfgt.auth.cryptosw++;
			CONCAT_G_FIFOS(cfgt.auth.crypto_cmd_list, yystack.l_mark[0].Attr_val_fifo);
		}
break;
case 64:
#line 566 "../../dist/ntpd/ntp_parser.y"
	{ cfgt.auth.keys = yystack.l_mark[0].String; }
break;
case 65:
#line 568 "../../dist/ntpd/ntp_parser.y"
	{ cfgt.auth.keysdir = yystack.l_mark[0].String; }
break;
case 66:
#line 570 "../../dist/ntpd/ntp_parser.y"
	{ cfgt.auth.request_key = yystack.l_mark[0].Integer; }
break;
case 67:
#line 572 "../../dist/ntpd/ntp_parser.y"
	{ cfgt.auth.revoke = yystack.l_mark[0].Integer; }
break;
case 68:
#line 574 "../../dist/ntpd/ntp_parser.y"
	{
			cfgt.auth.trusted_key_list = yystack.l_mark[0].Attr_val_fifo;

			/* if (!cfgt.auth.trusted_key_list)*/
			/* 	cfgt.auth.trusted_key_list = $2;*/
			/* else*/
			/* 	LINK_SLIST(cfgt.auth.trusted_key_list, $2, link);*/
		}
break;
case 69:
#line 583 "../../dist/ntpd/ntp_parser.y"
	{ cfgt.auth.ntp_signd_socket = yystack.l_mark[0].String; }
break;
case 70:
#line 588 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val_fifo = NULL; }
break;
case 71:
#line 590 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 72:
#line 598 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_sval(yystack.l_mark[-1].Integer, yystack.l_mark[0].String); }
break;
case 73:
#line 600 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val = NULL;
			cfgt.auth.revoke = yystack.l_mark[0].Integer;
			msyslog(LOG_WARNING,
				"'crypto revoke %d' is deprecated, "
				"please use 'revoke %d' instead.",
				cfgt.auth.revoke, cfgt.auth.revoke);
		}
break;
case 79:
#line 625 "../../dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.orphan_cmds, yystack.l_mark[0].Attr_val_fifo); }
break;
case 80:
#line 630 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 81:
#line 635 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 82:
#line 643 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-1].Integer, (double)yystack.l_mark[0].Integer); }
break;
case 83:
#line 645 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-1].Integer, yystack.l_mark[0].Double); }
break;
case 84:
#line 647 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-1].Integer, (double)yystack.l_mark[0].Integer); }
break;
case 96:
#line 674 "../../dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.stats_list, yystack.l_mark[0].Int_fifo); }
break;
case 97:
#line 676 "../../dist/ntpd/ntp_parser.y"
	{
			if (lex_from_file()) {
				cfgt.stats_dir = yystack.l_mark[0].String;
			} else {
				YYFREE(yystack.l_mark[0].String);
				yyerror("statsdir remote configuration ignored");
			}
		}
break;
case 98:
#line 685 "../../dist/ntpd/ntp_parser.y"
	{
			filegen_node *fgn;

			fgn = create_filegen_node(yystack.l_mark[-1].Integer, yystack.l_mark[0].Attr_val_fifo);
			APPEND_G_FIFO(cfgt.filegen_opts, fgn);
		}
break;
case 99:
#line 695 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Int_fifo = yystack.l_mark[-1].Int_fifo;
			APPEND_G_FIFO(yyval.Int_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 100:
#line 700 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Int_fifo = NULL;
			APPEND_G_FIFO(yyval.Int_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 109:
#line 719 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val_fifo = NULL; }
break;
case 110:
#line 721 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 111:
#line 729 "../../dist/ntpd/ntp_parser.y"
	{
			if (lex_from_file()) {
				yyval.Attr_val = create_attr_sval(yystack.l_mark[-1].Integer, yystack.l_mark[0].String);
			} else {
				yyval.Attr_val = NULL;
				YYFREE(yystack.l_mark[0].String);
				yyerror("filegen file remote config ignored");
			}
		}
break;
case 112:
#line 739 "../../dist/ntpd/ntp_parser.y"
	{
			if (lex_from_file()) {
				yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer);
			} else {
				yyval.Attr_val = NULL;
				yyerror("filegen type remote config ignored");
			}
		}
break;
case 113:
#line 748 "../../dist/ntpd/ntp_parser.y"
	{
			const char *err;

			if (lex_from_file()) {
				yyval.Attr_val = create_attr_ival(T_Flag, yystack.l_mark[0].Integer);
			} else {
				yyval.Attr_val = NULL;
				if (T_Link == yystack.l_mark[0].Integer)
					err = "filegen link remote config ignored";
				else
					err = "filegen nolink remote config ignored";
				yyerror(err);
			}
		}
break;
case 114:
#line 763 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(T_Flag, yystack.l_mark[0].Integer); }
break;
case 126:
#line 793 "../../dist/ntpd/ntp_parser.y"
	{
			CONCAT_G_FIFOS(cfgt.discard_opts, yystack.l_mark[0].Attr_val_fifo);
		}
break;
case 127:
#line 797 "../../dist/ntpd/ntp_parser.y"
	{
			CONCAT_G_FIFOS(cfgt.mru_opts, yystack.l_mark[0].Attr_val_fifo);
		}
break;
case 128:
#line 801 "../../dist/ntpd/ntp_parser.y"
	{
			restrict_node *rn;

			rn = create_restrict_node(yystack.l_mark[-1].Address_node, NULL, yystack.l_mark[0].Int_fifo,
						  lex_current()->curpos.nline);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
break;
case 129:
#line 809 "../../dist/ntpd/ntp_parser.y"
	{
			restrict_node *rn;

			rn = create_restrict_node(yystack.l_mark[-3].Address_node, yystack.l_mark[-1].Address_node, yystack.l_mark[0].Int_fifo,
						  lex_current()->curpos.nline);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
break;
case 130:
#line 817 "../../dist/ntpd/ntp_parser.y"
	{
			restrict_node *rn;

			rn = create_restrict_node(NULL, NULL, yystack.l_mark[0].Int_fifo,
						  lex_current()->curpos.nline);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
break;
case 131:
#line 825 "../../dist/ntpd/ntp_parser.y"
	{
			restrict_node *rn;

			rn = create_restrict_node(
				create_address_node(
					estrdup("0.0.0.0"),
					AF_INET),
				create_address_node(
					estrdup("0.0.0.0"),
					AF_INET),
				yystack.l_mark[0].Int_fifo,
				lex_current()->curpos.nline);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
break;
case 132:
#line 840 "../../dist/ntpd/ntp_parser.y"
	{
			restrict_node *rn;

			rn = create_restrict_node(
				create_address_node(
					estrdup("::"),
					AF_INET6),
				create_address_node(
					estrdup("::"),
					AF_INET6),
				yystack.l_mark[0].Int_fifo,
				lex_current()->curpos.nline);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
break;
case 133:
#line 855 "../../dist/ntpd/ntp_parser.y"
	{
			restrict_node *	rn;

			APPEND_G_FIFO(yystack.l_mark[0].Int_fifo, create_int_node(yystack.l_mark[-1].Integer));
			rn = create_restrict_node(
				NULL, NULL, yystack.l_mark[0].Int_fifo, lex_current()->curpos.nline);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
break;
case 134:
#line 867 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Int_fifo = NULL; }
break;
case 135:
#line 869 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Int_fifo = yystack.l_mark[-1].Int_fifo;
			APPEND_G_FIFO(yyval.Int_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 151:
#line 895 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 152:
#line 900 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 153:
#line 908 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 157:
#line 919 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 158:
#line 924 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 159:
#line 932 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 168:
#line 952 "../../dist/ntpd/ntp_parser.y"
	{
			addr_opts_node *aon;

			aon = create_addr_opts_node(yystack.l_mark[-1].Address_node, yystack.l_mark[0].Attr_val_fifo);
			APPEND_G_FIFO(cfgt.fudge, aon);
		}
break;
case 169:
#line 962 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 170:
#line 967 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 171:
#line 975 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-1].Integer, yystack.l_mark[0].Double); }
break;
case 172:
#line 977 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 173:
#line 979 "../../dist/ntpd/ntp_parser.y"
	{
			if (yystack.l_mark[0].Integer >= 0 && yystack.l_mark[0].Integer <= 16) {
				yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer);
			} else {
				yyval.Attr_val = NULL;
				yyerror("fudge factor: stratum value not in [0..16], ignored");
			}
		}
break;
case 174:
#line 988 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_sval(yystack.l_mark[-1].Integer, yystack.l_mark[0].String); }
break;
case 175:
#line 990 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_sval(yystack.l_mark[-1].Integer, yystack.l_mark[0].String); }
break;
case 182:
#line 1011 "../../dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.rlimit, yystack.l_mark[0].Attr_val_fifo); }
break;
case 183:
#line 1016 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 184:
#line 1021 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 185:
#line 1029 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 189:
#line 1045 "../../dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.enable_opts, yystack.l_mark[0].Attr_val_fifo); }
break;
case 190:
#line 1047 "../../dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.disable_opts, yystack.l_mark[0].Attr_val_fifo); }
break;
case 191:
#line 1052 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 192:
#line 1057 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 193:
#line 1065 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(T_Flag, yystack.l_mark[0].Integer); }
break;
case 194:
#line 1067 "../../dist/ntpd/ntp_parser.y"
	{
			if (lex_from_file()) {
				yyval.Attr_val = create_attr_ival(T_Flag, yystack.l_mark[0].Integer);
			} else {
				char err_str[128];

				yyval.Attr_val = NULL;
				snprintf(err_str, sizeof(err_str),
					 "enable/disable %s remote configuration ignored",
					 keyword(yystack.l_mark[0].Integer));
				yyerror(err_str);
			}
		}
break;
case 207:
#line 1106 "../../dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.tinker, yystack.l_mark[0].Attr_val_fifo); }
break;
case 208:
#line 1111 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 209:
#line 1116 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 210:
#line 1124 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-1].Integer, yystack.l_mark[0].Double); }
break;
case 223:
#line 1149 "../../dist/ntpd/ntp_parser.y"
	{
			attr_val *av;

			av = create_attr_dval(yystack.l_mark[-1].Integer, yystack.l_mark[0].Double);
			APPEND_G_FIFO(cfgt.vars, av);
		}
break;
case 224:
#line 1156 "../../dist/ntpd/ntp_parser.y"
	{
			attr_val *av;

			av = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer);
			APPEND_G_FIFO(cfgt.vars, av);
		}
break;
case 225:
#line 1163 "../../dist/ntpd/ntp_parser.y"
	{
			attr_val *av;

			av = create_attr_sval(yystack.l_mark[-1].Integer, yystack.l_mark[0].String);
			APPEND_G_FIFO(cfgt.vars, av);
		}
break;
case 226:
#line 1170 "../../dist/ntpd/ntp_parser.y"
	{
			char error_text[64];
			attr_val *av;

			if (lex_from_file()) {
				av = create_attr_sval(yystack.l_mark[-1].Integer, yystack.l_mark[0].String);
				APPEND_G_FIFO(cfgt.vars, av);
			} else {
				YYFREE(yystack.l_mark[0].String);
				snprintf(error_text, sizeof(error_text),
					 "%s remote config ignored",
					 keyword(yystack.l_mark[-1].Integer));
				yyerror(error_text);
			}
		}
break;
case 227:
#line 1186 "../../dist/ntpd/ntp_parser.y"
	{
			if (!lex_from_file()) {
				YYFREE(yystack.l_mark[-1].String); /* avoid leak */
				yyerror("remote includefile ignored");
				break;
			}
			if (lex_level() > MAXINCLUDELEVEL) {
				fprintf(stderr, "getconfig: Maximum include file level exceeded.\n");
				msyslog(LOG_ERR, "getconfig: Maximum include file level exceeded.");
			} else {
				const char * path = FindConfig(yystack.l_mark[-1].String); /* might return $2! */
				if (!lex_push_file(path, "r")) {
					fprintf(stderr, "getconfig: Couldn't open <%s>\n", path);
					msyslog(LOG_ERR, "getconfig: Couldn't open <%s>", path);
				}
			}
			YYFREE(yystack.l_mark[-1].String); /* avoid leak */
		}
break;
case 228:
#line 1205 "../../dist/ntpd/ntp_parser.y"
	{ lex_flush_stack(); }
break;
case 229:
#line 1207 "../../dist/ntpd/ntp_parser.y"
	{ /* see drift_parm below for actions */ }
break;
case 230:
#line 1209 "../../dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.logconfig, yystack.l_mark[0].Attr_val_fifo); }
break;
case 231:
#line 1211 "../../dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.phone, yystack.l_mark[0].String_fifo); }
break;
case 232:
#line 1213 "../../dist/ntpd/ntp_parser.y"
	{ APPEND_G_FIFO(cfgt.setvar, yystack.l_mark[0].Set_var); }
break;
case 233:
#line 1215 "../../dist/ntpd/ntp_parser.y"
	{
			addr_opts_node *aon;

			aon = create_addr_opts_node(yystack.l_mark[-1].Address_node, yystack.l_mark[0].Attr_val_fifo);
			APPEND_G_FIFO(cfgt.trap, aon);
		}
break;
case 234:
#line 1222 "../../dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.ttl, yystack.l_mark[0].Attr_val_fifo); }
break;
case 239:
#line 1237 "../../dist/ntpd/ntp_parser.y"
	{
#ifndef LEAP_SMEAR
			yyerror("Built without LEAP_SMEAR support.");
#endif
		}
break;
case 245:
#line 1257 "../../dist/ntpd/ntp_parser.y"
	{
			if (lex_from_file()) {
				attr_val *av;
				av = create_attr_sval(T_Driftfile, yystack.l_mark[0].String);
				APPEND_G_FIFO(cfgt.vars, av);
			} else {
				YYFREE(yystack.l_mark[0].String);
				yyerror("driftfile remote configuration ignored");
			}
		}
break;
case 246:
#line 1268 "../../dist/ntpd/ntp_parser.y"
	{
			if (lex_from_file()) {
				attr_val *av;
				av = create_attr_sval(T_Driftfile, yystack.l_mark[-1].String);
				APPEND_G_FIFO(cfgt.vars, av);
				av = create_attr_dval(T_WanderThreshold, yystack.l_mark[0].Double);
				APPEND_G_FIFO(cfgt.vars, av);
			} else {
				YYFREE(yystack.l_mark[-1].String);
				yyerror("driftfile remote configuration ignored");
			}
		}
break;
case 247:
#line 1281 "../../dist/ntpd/ntp_parser.y"
	{
			if (lex_from_file()) {
				attr_val *av;
				av = create_attr_sval(T_Driftfile, estrdup(""));
				APPEND_G_FIFO(cfgt.vars, av);
			} else {
				yyerror("driftfile remote configuration ignored");
			}
		}
break;
case 248:
#line 1294 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Set_var = create_setvar_node(yystack.l_mark[-3].String, yystack.l_mark[-1].String, yystack.l_mark[0].Integer); }
break;
case 250:
#line 1300 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Integer = 0; }
break;
case 251:
#line 1305 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val_fifo = NULL; }
break;
case 252:
#line 1307 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 253:
#line 1315 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 254:
#line 1317 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val = create_attr_sval(yystack.l_mark[-1].Integer, estrdup(yystack.l_mark[0].Address_node->address));
			destroy_address_node(yystack.l_mark[0].Address_node);
		}
break;
case 255:
#line 1325 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 256:
#line 1330 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 257:
#line 1338 "../../dist/ntpd/ntp_parser.y"
	{
			char	prefix;
			char *	type;

			switch (yystack.l_mark[0].String[0]) {

			case '+':
			case '-':
			case '=':
				prefix = yystack.l_mark[0].String[0];
				type = yystack.l_mark[0].String + 1;
				break;

			default:
				prefix = '=';
				type = yystack.l_mark[0].String;
			}

			yyval.Attr_val = create_attr_sval(prefix, estrdup(type));
			YYFREE(yystack.l_mark[0].String);
		}
break;
case 258:
#line 1363 "../../dist/ntpd/ntp_parser.y"
	{
			nic_rule_node *nrn;

			nrn = create_nic_rule_node(yystack.l_mark[0].Integer, NULL, yystack.l_mark[-1].Integer);
			APPEND_G_FIFO(cfgt.nic_rules, nrn);
		}
break;
case 259:
#line 1370 "../../dist/ntpd/ntp_parser.y"
	{
			nic_rule_node *nrn;

			nrn = create_nic_rule_node(0, yystack.l_mark[0].String, yystack.l_mark[-1].Integer);
			APPEND_G_FIFO(cfgt.nic_rules, nrn);
		}
break;
case 269:
#line 1398 "../../dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.reset_counters, yystack.l_mark[0].Int_fifo); }
break;
case 270:
#line 1403 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Int_fifo = yystack.l_mark[-1].Int_fifo;
			APPEND_G_FIFO(yyval.Int_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 271:
#line 1408 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Int_fifo = NULL;
			APPEND_G_FIFO(yyval.Int_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 279:
#line 1432 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 280:
#line 1437 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 281:
#line 1445 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 282:
#line 1450 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 283:
#line 1458 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival('i', yystack.l_mark[0].Integer); }
break;
case 285:
#line 1464 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_rangeval('-', yystack.l_mark[-3].Integer, yystack.l_mark[-1].Integer); }
break;
case 286:
#line 1469 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.String_fifo = yystack.l_mark[-1].String_fifo;
			APPEND_G_FIFO(yyval.String_fifo, create_string_node(yystack.l_mark[0].String));
		}
break;
case 287:
#line 1474 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.String_fifo = NULL;
			APPEND_G_FIFO(yyval.String_fifo, create_string_node(yystack.l_mark[0].String));
		}
break;
case 288:
#line 1482 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Address_fifo = yystack.l_mark[-1].Address_fifo;
			APPEND_G_FIFO(yyval.Address_fifo, yystack.l_mark[0].Address_node);
		}
break;
case 289:
#line 1487 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Address_fifo = NULL;
			APPEND_G_FIFO(yyval.Address_fifo, yystack.l_mark[0].Address_node);
		}
break;
case 290:
#line 1495 "../../dist/ntpd/ntp_parser.y"
	{
			if (yystack.l_mark[0].Integer != 0 && yystack.l_mark[0].Integer != 1) {
				yyerror("Integer value is not boolean (0 or 1). Assuming 1");
				yyval.Integer = 1;
			} else {
				yyval.Integer = yystack.l_mark[0].Integer;
			}
		}
break;
case 291:
#line 1503 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Integer = 1; }
break;
case 292:
#line 1504 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Integer = 0; }
break;
case 293:
#line 1508 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Double = (double)yystack.l_mark[0].Integer; }
break;
case 295:
#line 1519 "../../dist/ntpd/ntp_parser.y"
	{
			sim_node *sn;

			sn =  create_sim_node(yystack.l_mark[-2].Attr_val_fifo, yystack.l_mark[-1].Sim_server_fifo);
			APPEND_G_FIFO(cfgt.sim_details, sn);

			/* Revert from ; to \n for end-of-command */
			old_config_style = 1;
		}
break;
case 296:
#line 1536 "../../dist/ntpd/ntp_parser.y"
	{ old_config_style = 0; }
break;
case 297:
#line 1541 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-2].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[-1].Attr_val);
		}
break;
case 298:
#line 1546 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[-1].Attr_val);
		}
break;
case 299:
#line 1554 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-2].Integer, yystack.l_mark[0].Double); }
break;
case 302:
#line 1564 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Sim_server_fifo = yystack.l_mark[-1].Sim_server_fifo;
			APPEND_G_FIFO(yyval.Sim_server_fifo, yystack.l_mark[0].Sim_server);
		}
break;
case 303:
#line 1569 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Sim_server_fifo = NULL;
			APPEND_G_FIFO(yyval.Sim_server_fifo, yystack.l_mark[0].Sim_server);
		}
break;
case 304:
#line 1577 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Sim_server = ONLY_SIM(create_sim_server(yystack.l_mark[-4].Address_node, yystack.l_mark[-2].Double, yystack.l_mark[-1].Sim_script_fifo)); }
break;
case 305:
#line 1582 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Double = yystack.l_mark[-1].Double; }
break;
case 306:
#line 1587 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Address_node = yystack.l_mark[0].Address_node; }
break;
case 307:
#line 1592 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Sim_script_fifo = yystack.l_mark[-1].Sim_script_fifo;
			APPEND_G_FIFO(yyval.Sim_script_fifo, yystack.l_mark[0].Sim_script);
		}
break;
case 308:
#line 1597 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Sim_script_fifo = NULL;
			APPEND_G_FIFO(yyval.Sim_script_fifo, yystack.l_mark[0].Sim_script);
		}
break;
case 309:
#line 1605 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Sim_script = ONLY_SIM(create_sim_script_info(yystack.l_mark[-3].Double, yystack.l_mark[-1].Attr_val_fifo)); }
break;
case 310:
#line 1610 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-2].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[-1].Attr_val);
		}
break;
case 311:
#line 1615 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[-1].Attr_val);
		}
break;
case 312:
#line 1623 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-2].Integer, yystack.l_mark[0].Double); }
break;
#line 2324 "ntp_parser.c"
    }
    yystack.s_mark -= yym;
    yystate = *yystack.s_mark;
    yystack.l_mark -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yystack.s_mark = YYFINAL;
        *++yystack.l_mark = yyval;
        if (yychar < 0)
        {
            if ((yychar = YYLEX) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = yytname[YYTRANSLATE(yychar)];
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yystack.s_mark, yystate);
#endif
    if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack))
    {
        goto yyoverflow;
    }
    *++yystack.s_mark = (short) yystate;
    *++yystack.l_mark = yyval;
    goto yyloop;

yyoverflow:
    yyerror("yacc stack overflow");

yyabort:
    yyfreestack(&yystack);
    return (1);

yyaccept:
    yyfreestack(&yystack);
    return (0);
}
