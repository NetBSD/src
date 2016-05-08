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
#define T_Beacon 267
#define T_Broadcast 268
#define T_Broadcastclient 269
#define T_Broadcastdelay 270
#define T_Burst 271
#define T_Calibrate 272
#define T_Ceiling 273
#define T_Clockstats 274
#define T_Cohort 275
#define T_ControlKey 276
#define T_Crypto 277
#define T_Cryptostats 278
#define T_Ctl 279
#define T_Day 280
#define T_Default 281
#define T_Digest 282
#define T_Disable 283
#define T_Discard 284
#define T_Dispersion 285
#define T_Double 286
#define T_Driftfile 287
#define T_Drop 288
#define T_Dscp 289
#define T_Ellipsis 290
#define T_Enable 291
#define T_End 292
#define T_False 293
#define T_File 294
#define T_Filegen 295
#define T_Filenum 296
#define T_Flag1 297
#define T_Flag2 298
#define T_Flag3 299
#define T_Flag4 300
#define T_Flake 301
#define T_Floor 302
#define T_Freq 303
#define T_Fudge 304
#define T_Host 305
#define T_Huffpuff 306
#define T_Iburst 307
#define T_Ident 308
#define T_Ignore 309
#define T_Incalloc 310
#define T_Incmem 311
#define T_Initalloc 312
#define T_Initmem 313
#define T_Includefile 314
#define T_Integer 315
#define T_Interface 316
#define T_Intrange 317
#define T_Io 318
#define T_Ipv4 319
#define T_Ipv4_flag 320
#define T_Ipv6 321
#define T_Ipv6_flag 322
#define T_Kernel 323
#define T_Key 324
#define T_Keys 325
#define T_Keysdir 326
#define T_Kod 327
#define T_Mssntp 328
#define T_Leapfile 329
#define T_Leapsmearinterval 330
#define T_Limited 331
#define T_Link 332
#define T_Listen 333
#define T_Logconfig 334
#define T_Logfile 335
#define T_Loopstats 336
#define T_Lowpriotrap 337
#define T_Manycastclient 338
#define T_Manycastserver 339
#define T_Mask 340
#define T_Maxage 341
#define T_Maxclock 342
#define T_Maxdepth 343
#define T_Maxdist 344
#define T_Maxmem 345
#define T_Maxpoll 346
#define T_Mdnstries 347
#define T_Mem 348
#define T_Memlock 349
#define T_Minclock 350
#define T_Mindepth 351
#define T_Mindist 352
#define T_Minimum 353
#define T_Minpoll 354
#define T_Minsane 355
#define T_Mode 356
#define T_Mode7 357
#define T_Monitor 358
#define T_Month 359
#define T_Mru 360
#define T_Multicastclient 361
#define T_Nic 362
#define T_Nolink 363
#define T_Nomodify 364
#define T_Nomrulist 365
#define T_None 366
#define T_Nonvolatile 367
#define T_Nopeer 368
#define T_Noquery 369
#define T_Noselect 370
#define T_Noserve 371
#define T_Notrap 372
#define T_Notrust 373
#define T_Ntp 374
#define T_Ntpport 375
#define T_NtpSignDsocket 376
#define T_Orphan 377
#define T_Orphanwait 378
#define T_Panic 379
#define T_Peer 380
#define T_Peerstats 381
#define T_Phone 382
#define T_Pid 383
#define T_Pidfile 384
#define T_Pool 385
#define T_Port 386
#define T_Preempt 387
#define T_Prefer 388
#define T_Protostats 389
#define T_Pw 390
#define T_Randfile 391
#define T_Rawstats 392
#define T_Refid 393
#define T_Requestkey 394
#define T_Reset 395
#define T_Restrict 396
#define T_Revoke 397
#define T_Rlimit 398
#define T_Saveconfigdir 399
#define T_Server 400
#define T_Setvar 401
#define T_Source 402
#define T_Stacksize 403
#define T_Statistics 404
#define T_Stats 405
#define T_Statsdir 406
#define T_Step 407
#define T_Stepback 408
#define T_Stepfwd 409
#define T_Stepout 410
#define T_Stratum 411
#define T_String 412
#define T_Sys 413
#define T_Sysstats 414
#define T_Tick 415
#define T_Time1 416
#define T_Time2 417
#define T_Timer 418
#define T_Timingstats 419
#define T_Tinker 420
#define T_Tos 421
#define T_Trap 422
#define T_True 423
#define T_Trustedkey 424
#define T_Ttl 425
#define T_Type 426
#define T_U_int 427
#define T_UEcrypto 428
#define T_UEcryptonak 429
#define T_UEdigest 430
#define T_Unconfig 431
#define T_Unpeer 432
#define T_Version 433
#define T_WanderThreshold 434
#define T_Week 435
#define T_Wildcard 436
#define T_Xleave 437
#define T_Year 438
#define T_Flag 439
#define T_EOC 440
#define T_Simulate 441
#define T_Beep_Delay 442
#define T_Sim_Duration 443
#define T_Server_Offset 444
#define T_Duration 445
#define T_Freq_Offset 446
#define T_Wander 447
#define T_Jitter 448
#define T_Prop_Delay 449
#define T_Proc_Delay 450
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
   68,   68,   68,   68,   93,   93,   93,   57,   57,   56,
   56,   56,   56,   56,   56,   56,   56,   18,   18,   17,
   17,   17,   17,   32,   32,   16,   16,   19,   19,   19,
   19,   19,   19,   19,   94,   94,   94,   94,   94,   94,
   94,   94,    2,    2,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,   15,
   15,   13,   14,   14,   14,   41,   41,   39,   40,   40,
   40,   40,   40,   40,   40,   40,   96,   23,   23,   20,
   20,   20,   20,   20,   22,   22,   21,   21,   21,   21,
   97,   55,   55,   54,   53,   53,   53,   98,   98,   62,
   62,   59,   59,   60,   60,   60,   60,   60,   60,   61,
   61,   61,   61,   61,   99,   66,   66,   65,   64,   64,
   64,   64,   64,   64,   64,   64,   64,   64,  100,  100,
  100,  100,  100,  100,  100,  100,  100,  100,  100,  100,
  100,  100,   35,   35,   35,   36,   36,   37,   37,   38,
   38,   38,  102,  102,  102,   74,   63,   63,   72,   72,
   71,   71,   34,   34,   33,   29,   29,   30,   30,   42,
   42,   42,   42,   28,   28,   28,   52,    9,    9,    8,
    8,    8,    8,    8,    8,    8,   24,   24,   25,   25,
   26,   26,   27,   58,   58,    5,    5,    6,    6,    6,
   43,   43,  101,  103,   76,   76,   75,   77,   77,   78,
   78,   79,   80,   81,   83,   83,   82,   85,   85,   86,
   84,   84,   84,   84,   84,
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
    1,    1,    1,    1,    2,    2,    3,    2,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    0,    2,    2,
    2,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    2,    2,    3,    5,    3,    4,
    4,    3,    0,    2,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    2,
    1,    2,    1,    1,    1,    2,    1,    2,    1,    1,
    1,    1,    1,    1,    1,    1,    3,    2,    1,    2,
    2,    2,    2,    2,    1,    1,    1,    1,    1,    1,
    2,    2,    1,    2,    1,    1,    1,    2,    2,    2,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    2,    2,    1,    2,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    2,    2,    2,    2,    3,    1,    2,    2,    2,    2,
    3,    2,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    2,    0,    4,    1,    0,    0,    2,
    2,    2,    2,    1,    1,    3,    3,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    2,    2,    1,    1,
    1,    1,    1,    1,    1,    1,    2,    1,    2,    1,
    1,    1,    5,    2,    1,    2,    1,    1,    1,    1,
    1,    1,    5,    1,    3,    2,    3,    1,    1,    2,
    1,    5,    4,    3,    2,    1,    6,    3,    2,    3,
    1,    1,    1,    1,    1,
};
static const short yydefred[] = {                         0,
    0,    0,   23,   57,  233,    0,   70,    0,    0,    0,
  236,    0,  226,    0,    0,  238,    0,  258,    0,    0,
  239,  237,    0,  240,   24,    0,    0,    0,    0,  259,
  234,    0,   22,    0,  241,   21,    0,    0,    0,    0,
    0,  242,   20,    0,    0,    0,  235,    0,    0,    0,
    0,    0,   55,   56,  294,    0,    0,  219,    0,    0,
    0,    0,    0,  220,    0,    0,    0,    6,    7,    8,
    9,   10,   11,   12,   13,   14,   15,   16,   17,   18,
    0,    4,   61,   62,    0,  194,  195,  196,  197,  200,
  198,  199,  201,  202,  203,  204,  191,  192,  193,    0,
  153,  154,  155,  151,    0,    0,    0,  227,    0,  100,
  101,  102,  103,  107,  104,  105,  106,  108,   28,   29,
   27,    0,    0,   25,    0,   64,   65,  255,  254,    0,
  287,    0,   60,  159,  160,  161,  162,  163,  164,  165,
  166,  157,    0,    0,    0,   69,  285,    0,   66,  270,
  271,  272,  273,  274,  275,  276,  269,    0,  133,    0,
    0,  133,  133,    0,   67,  187,  185,  186,    0,  183,
    0,    0,  230,   99,    0,   96,  209,  210,  211,  212,
  213,  214,  215,  216,  217,  218,    0,  207,    0,   90,
   85,    0,   86,   94,   92,   93,   91,   89,   87,   88,
   81,    0,    0,    0,  249,  281,    0,    0,  280,  282,
  278,    0,   30,  266,  265,  264,    0,  292,  291,  221,
  222,  223,  224,   54,    0,    3,    0,   78,   74,   75,
   76,   77,    0,   71,    0,  190,  152,  150,  244,    0,
    0,  177,  178,  179,  180,    0,    0,  175,  176,  169,
    0,    0,    0,   26,  225,  253,  286,  158,  156,  284,
  268,    0,  133,  133,    0,    0,    0,  184,  182,    0,
   98,  208,  206,  290,  288,  289,   84,   83,   82,   80,
    0,    0,  279,  277,    0,  260,  261,  262,  257,  263,
  256,    2,  298,  299,    0,    0,    0,   73,   72,  117,
  116,    0,  114,  115,    0,  113,  109,  112,  173,  174,
  172,  171,  170,  168,  135,  136,  137,  138,  139,  140,
  141,  142,  143,  144,  145,  146,  147,  148,  149,  134,
    0,    0,  133,    0,    0,    0,  250,    0,   36,   37,
   38,   53,   46,   48,   47,   50,   39,   40,   41,   42,
   49,   51,   43,   31,   32,   35,   33,    0,   34,    0,
  296,    0,    0,    0,  301,    0,    0,  110,  124,  120,
  122,  118,  119,  121,  123,  111,    0,  247,  246,  252,
  251,    0,   44,   45,   52,    0,  295,  293,  300,    0,
  297,  283,  304,    0,    0,    0,    0,  306,    0,    0,
    0,  302,  305,  303,    0,    0,  311,  312,  313,  314,
  315,    0,    0,    0,    0,  307,    0,  309,  310,  308,
};
static const short yydgoto[] = {                         56,
  330,  262,  131,  123,  132,  277,   57,  157,  158,  234,
   85,  235,  104,  105,  106,  306,  307,  240,  376,  250,
  251,  252,  253,  212,  208,  209,  210,  217,   58,   59,
  124,  308,  129,  130,   60,   61,   62,   63,  142,  143,
  144,  291,  220,  354,  355,  356,  285,  357,  358,  359,
  360,   64,  169,  170,  171,  118,  175,  148,   97,   98,
   99,  100,  379,  187,  188,  189,  201,  202,  203,  204,
  337,  281,   65,  173,  295,  296,  297,  364,  365,  395,
  366,  398,  399,  412,  413,  414,   66,   67,   68,   69,
   70,   71,   72,   73,   74,   75,   76,   77,   78,   79,
   80,  108,   81,
};
static const short yysindex[] = {                      -225,
 -407, -269,    0,    0,    0, -267,    0,   38, -242, -371,
    0,   38,    0, -258, -286,    0, -359,    0, -358, -355,
    0,    0, -352,    0,    0, -286, -254,  118, -286,    0,
    0, -349,    0, -347,    0,    0, -235, -185, -234, -233,
 -274,    0,    0, -331, -258, -325,    0,   12,  172, -322,
  -35, -223,    0,    0,    0,    0, -286,    0, -260, -246,
 -222, -317, -316,    0, -286,  -18, -343,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  -25,    0,    0,    0, -180,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   38,
    0,    0,    0,    0, -209, -242, -179,    0,   38,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0, -154, -300,    0,  -18,    0,    0,    0,    0, -352,
    0, -286,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0, -195,  118, -286,    0,    0, -288,    0,    0,
    0,    0,    0,    0,    0,    0,    0, -185,    0, -143,
 -142,    0,    0, -213,    0,    0,    0,    0, -168,    0,
 -274,   87,    0,    0, -258,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0, -246,    0,   12,    0,
    0, -283,    0,    0,    0,    0,    0,    0,    0,    0,
    0, -246, -166,  172,    0,    0, -165,  -35,    0,    0,
    0, -163,    0,    0,    0,    0, -247,    0,    0,    0,
    0,    0,    0,    0, -287,    0, -405,    0,    0,    0,
    0,    0, -161,    0, -248,    0,    0,    0,    0, -264,
 -232,    0,    0,    0,    0, -230, -157,    0,    0,    0,
 -283, -246, -154,    0,    0,    0,    0,    0,    0,    0,
    0,  123,    0,    0,  123,  123, -322,    0,    0, -229,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
 -301, -124,    0,    0,  -86,    0,    0,    0,    0,    0,
    0,    0,    0,    0, -273, -387,  130,    0,    0,    0,
    0, -220,    0,    0, -251,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  123,  123,    0,  -88, -322, -107,    0, -106,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0, -306,    0, -218,
    0,  137, -228, -121,    0,   90, -246,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  123,    0,    0,    0,
    0,  173,    0,    0,    0, -286,    0,    0,    0, -226,
    0,    0,    0,  158, -221, -246,  162,    0, -117, -215,
 -246,    0,    0,    0,  103, -245,    0,    0,    0,    0,
    0,  166, -123, -211, -246,    0, -210,    0,    0,    0,
};
static const short yyrindex[] = {                      -208,
    0,    0,    0,    0,    0,    0,    0,    0,    0, -205,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    1,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0, -204,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0, -203,
    0,    0,    0,    0,    0, -200, -199,    0, -198,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0, -208,    0,    0,    0,    0, -197,
    0, -196,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0, -193, -192,    0,    0, -191,    0,    0,
    0,    0,    0,    0,    0,    0,    0, -187,    0, -167,
 -158,    0,    0,  107,    0,    0,    0,    0,    0,    0,
 -184,    0,    0,    0, -176,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0, -173,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0, -164,    0,    0,    0, -162,    0,    0,
    0, -159,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0, -155,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0, -153,    0,    0,    0,    0,    0,    0,    0,
    0, -152,    0,    0, -151, -149,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
 -148,    0,    0,    0, -147,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
 -146, -137,    0, -135,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0, -134,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
};
static const short yygindex[] = {                         0,
    0, -145,  -15,    0,  226,   10,    0,  117,    0,    0,
    0,    0,  176,    0,    0,    0,    0,    0,    0,   30,
    0,    0,    0,    0,    0,  101,    0,    0,    0,    0,
  -36,    0,  165,    0,    0,    0,    0,    0,  169,    0,
    0,    0, -181,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  143,    0,  -34,    0,    0,  -74,    0,
    0,  307,    0,    0,  133,    0,  126,    0,    0,    0,
    0,    0,    0,    0,   35,    0,    0,    0,  -32,    0,
    0,  -66,    0,    0,    0,  -79,    0,  -41,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,
};
#define YYTABLESIZE 556
static const short yytable[] = {                        122,
    1,  416,  164,  388,  207,  272,  369,  402,  383,  274,
  174,  286,  362,  205,  335,  110,  265,  266,  300,  111,
  278,  166,  101,  163,  225,  236,  301,  214,  370,  302,
    1,  275,   82,  119,  236,  120,  293,  294,    2,  218,
  107,  213,    3,    4,    5,   83,  159,   84,  215,  224,
    6,    7,  125,  126,  293,  294,  127,    8,    9,  128,
  133,   10,  146,   11,  147,   12,   13,  303,  219,   14,
  313,  287,  216,  288,  167,  150,  151,  112,   15,  149,
  172,  165,   16,  255,  336,  160,  176,  161,   17,  121,
   18,  211,  221,  152,  222,  223,  226,  227,  304,   19,
   20,  228,  241,   21,   22,  237,  239,  371,   23,   24,
  102,  254,   25,   26,  372,  103,  257,  331,  332,  258,
  384,   27,  113,  260,  229,  121,  267,  230,  168,  257,
  114,  373,  153,  115,   28,   29,   30,  263,  264,  276,
  271,   31,  242,  243,  244,  245,  268,  270,  279,  282,
   32,  284,  292,  298,   33,  116,   34,  311,   35,   36,
  117,  305,  154,  299,  289,  338,  361,  162,   37,   38,
   39,   40,   41,   42,   43,   44,  339,  121,   45,  309,
   46,  310,  334,  374,  340,  391,  375,  377,  290,   47,
  367,  368,  378,  385,   48,   49,   50,  386,   51,   52,
  407,  408,  409,  410,  411,   53,   54,  381,  382,  231,
  232,  387,  390,  392,  400,   55,  233,  394,  396,  405,
  341,  342,  401,  397,  404,  406,  415,  155,  418,  420,
  333,    5,  156,  419,  245,   63,  189,  343,  246,  125,
  243,  188,  228,   58,   28,    2,  126,   59,  229,    3,
    4,    5,  267,   29,  145,  181,  247,    6,    7,  344,
  312,  248,  249,   95,    8,    9,  205,  345,   10,  346,
   11,  177,   12,   13,  261,   79,   14,   68,  362,  206,
  232,  238,  314,  347,   97,   15,  167,  129,  132,   16,
  127,  231,   19,  130,  256,   17,  178,   18,  380,   86,
  348,  349,  131,   87,  248,  128,   19,   20,  283,   88,
   21,   22,  259,  269,  179,   23,   24,  180,  109,   25,
   26,  273,  407,  408,  409,  410,  411,  397,   27,  280,
  363,  389,  403,  417,    0,    0,  350,    0,  351,    0,
    0,   28,   29,   30,    0,    0,  352,    0,   31,    0,
  353,    0,    0,    0,    0,    0,    0,   32,    0,    0,
   89,   33,    0,   34,    0,   35,   36,    0,    0,    0,
  393,    0,    0,    0,    0,   37,   38,   39,   40,   41,
   42,   43,   44,    0,    0,   45,    0,   46,    0,    0,
  181,    0,    0,    0,   90,   91,   47,    0,    0,    0,
    0,   48,   49,   50,    0,   51,   52,   25,    0,    0,
    0,   92,   53,   54,    0,   25,    0,    0,  182,  183,
  184,  185,   55,  315,    0,    0,  186,  134,  135,  136,
  137,  316,    0,   25,   25,    0,    0,   25,  190,    0,
    5,    0,   93,   25,  191,    0,  192,    0,    0,  317,
  318,    0,    0,  319,    0,    0,    0,    0,  138,  320,
  139,    0,  140,    0,    0,   94,   95,   96,  141,    0,
   25,   25,    0,  193,   25,   25,    0,   25,   25,   25,
    0,   25,    0,    0,    0,    0,  321,  322,    0,    0,
  323,  324,    0,  325,  326,  327,    0,  328,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  194,    0,  195,    0,    0,    0,    0,
    0,  196,    0,  197,    0,    0,  198,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   25,
    0,    0,    0,    0,    0,    0,   25,    0,  199,  200,
    0,    0,    0,    0,    0,  329,
};
static const short yycheck[] = {                         15,
    0,  125,   39,  125,   40,  187,  258,  125,  315,  293,
   45,  259,  400,   50,  316,  274,  162,  163,  283,  278,
  202,  296,  265,   39,   66,  100,  291,  288,  280,  294,
  256,  315,  440,  320,  109,  322,  442,  443,  264,  286,
  412,   57,  268,  269,  270,  315,  281,  315,  309,   65,
  276,  277,  412,  412,  442,  443,  412,  283,  284,  412,
  315,  287,  412,  289,  412,  291,  292,  332,  315,  295,
  252,  319,  333,  321,  349,  261,  262,  336,  304,  315,
  412,  315,  308,  125,  386,  320,  412,  322,  314,  412,
  316,  315,  315,  279,  412,  412,  440,  123,  363,  325,
  326,  282,  257,  329,  330,  315,  286,  359,  334,  335,
  353,  412,  338,  339,  366,  358,  132,  263,  264,  315,
  427,  347,  381,  412,  305,  412,  340,  308,  403,  145,
  389,  383,  318,  392,  360,  361,  362,  281,  281,  423,
  175,  367,  297,  298,  299,  300,  315,   61,  315,  315,
  376,  315,  440,  315,  380,  414,  382,  315,  384,  385,
  419,  426,  348,  412,  412,  290,  440,  402,  394,  395,
  396,  397,  398,  399,  400,  401,  263,  412,  404,  412,
  406,  412,  412,  435,  271,  367,  438,  333,  436,  415,
   61,  412,  281,  412,  420,  421,  422,   61,  424,  425,
  446,  447,  448,  449,  450,  431,  432,  315,  315,  390,
  391,  440,  123,   41,  396,  441,  397,  444,   61,  401,
  307,  308,   61,  445,  440,  123,   61,  413,  440,  440,
  267,  440,  418,  415,  440,  440,  440,  324,  393,  440,
  440,  440,  440,  440,  412,  264,  440,  440,  440,  268,
  269,  270,  440,  412,   29,  440,  411,  276,  277,  346,
  251,  416,  417,  440,  283,  284,  440,  354,  287,  356,
  289,  260,  291,  292,  158,  440,  295,  440,  400,  315,
  440,  106,  253,  370,  440,  304,  440,  440,  440,  308,
  440,  440,  440,  440,  130,  314,  285,  316,  335,  262,
  387,  388,  440,  266,  440,  440,  325,  326,  208,  272,
  329,  330,  144,  171,  303,  334,  335,  306,   12,  338,
  339,  189,  446,  447,  448,  449,  450,  445,  347,  204,
  296,  364,  399,  413,   -1,   -1,  423,   -1,  425,   -1,
   -1,  360,  361,  362,   -1,   -1,  433,   -1,  367,   -1,
  437,   -1,   -1,   -1,   -1,   -1,   -1,  376,   -1,   -1,
  323,  380,   -1,  382,   -1,  384,  385,   -1,   -1,   -1,
  386,   -1,   -1,   -1,   -1,  394,  395,  396,  397,  398,
  399,  400,  401,   -1,   -1,  404,   -1,  406,   -1,   -1,
  379,   -1,   -1,   -1,  357,  358,  415,   -1,   -1,   -1,
   -1,  420,  421,  422,   -1,  424,  425,  301,   -1,   -1,
   -1,  374,  431,  432,   -1,  309,   -1,   -1,  407,  408,
  409,  410,  441,  301,   -1,   -1,  415,  310,  311,  312,
  313,  309,   -1,  327,  328,   -1,   -1,  331,  267,   -1,
  440,   -1,  405,  337,  273,   -1,  275,   -1,   -1,  327,
  328,   -1,   -1,  331,   -1,   -1,   -1,   -1,  341,  337,
  343,   -1,  345,   -1,   -1,  428,  429,  430,  351,   -1,
  364,  365,   -1,  302,  368,  369,   -1,  371,  372,  373,
   -1,  375,   -1,   -1,   -1,   -1,  364,  365,   -1,   -1,
  368,  369,   -1,  371,  372,  373,   -1,  375,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,  342,   -1,  344,   -1,   -1,   -1,   -1,
   -1,  350,   -1,  352,   -1,   -1,  355,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  433,
   -1,   -1,   -1,   -1,   -1,   -1,  440,   -1,  377,  378,
   -1,   -1,   -1,   -1,   -1,  433,
};
#define YYFINAL 56
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 450
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
"T_Average","T_Bclient","T_Beacon","T_Broadcast","T_Broadcastclient",
"T_Broadcastdelay","T_Burst","T_Calibrate","T_Ceiling","T_Clockstats",
"T_Cohort","T_ControlKey","T_Crypto","T_Cryptostats","T_Ctl","T_Day",
"T_Default","T_Digest","T_Disable","T_Discard","T_Dispersion","T_Double",
"T_Driftfile","T_Drop","T_Dscp","T_Ellipsis","T_Enable","T_End","T_False",
"T_File","T_Filegen","T_Filenum","T_Flag1","T_Flag2","T_Flag3","T_Flag4",
"T_Flake","T_Floor","T_Freq","T_Fudge","T_Host","T_Huffpuff","T_Iburst",
"T_Ident","T_Ignore","T_Incalloc","T_Incmem","T_Initalloc","T_Initmem",
"T_Includefile","T_Integer","T_Interface","T_Intrange","T_Io","T_Ipv4",
"T_Ipv4_flag","T_Ipv6","T_Ipv6_flag","T_Kernel","T_Key","T_Keys","T_Keysdir",
"T_Kod","T_Mssntp","T_Leapfile","T_Leapsmearinterval","T_Limited","T_Link",
"T_Listen","T_Logconfig","T_Logfile","T_Loopstats","T_Lowpriotrap",
"T_Manycastclient","T_Manycastserver","T_Mask","T_Maxage","T_Maxclock",
"T_Maxdepth","T_Maxdist","T_Maxmem","T_Maxpoll","T_Mdnstries","T_Mem",
"T_Memlock","T_Minclock","T_Mindepth","T_Mindist","T_Minimum","T_Minpoll",
"T_Minsane","T_Mode","T_Mode7","T_Monitor","T_Month","T_Mru",
"T_Multicastclient","T_Nic","T_Nolink","T_Nomodify","T_Nomrulist","T_None",
"T_Nonvolatile","T_Nopeer","T_Noquery","T_Noselect","T_Noserve","T_Notrap",
"T_Notrust","T_Ntp","T_Ntpport","T_NtpSignDsocket","T_Orphan","T_Orphanwait",
"T_Panic","T_Peer","T_Peerstats","T_Phone","T_Pid","T_Pidfile","T_Pool",
"T_Port","T_Preempt","T_Prefer","T_Protostats","T_Pw","T_Randfile","T_Rawstats",
"T_Refid","T_Requestkey","T_Reset","T_Restrict","T_Revoke","T_Rlimit",
"T_Saveconfigdir","T_Server","T_Setvar","T_Source","T_Stacksize","T_Statistics",
"T_Stats","T_Statsdir","T_Step","T_Stepback","T_Stepfwd","T_Stepout",
"T_Stratum","T_String","T_Sys","T_Sysstats","T_Tick","T_Time1","T_Time2",
"T_Timer","T_Timingstats","T_Tinker","T_Tos","T_Trap","T_True","T_Trustedkey",
"T_Ttl","T_Type","T_U_int","T_UEcrypto","T_UEcryptonak","T_UEdigest",
"T_Unconfig","T_Unpeer","T_Version","T_WanderThreshold","T_Week","T_Wildcard",
"T_Xleave","T_Year","T_Flag","T_EOC","T_Simulate","T_Beep_Delay",
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
"tos_option_int_keyword : T_Ceiling",
"tos_option_int_keyword : T_Floor",
"tos_option_int_keyword : T_Orphan",
"tos_option_int_keyword : T_Orphanwait",
"tos_option_int_keyword : T_Minsane",
"tos_option_int_keyword : T_Beacon",
"tos_option_dbl_keyword : T_Mindist",
"tos_option_dbl_keyword : T_Maxdist",
"tos_option_dbl_keyword : T_Minclock",
"tos_option_dbl_keyword : T_Maxclock",
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
"access_control_command : T_Restrict ip_address T_Mask ip_address ac_flag_list",
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
#line 1631 "../../dist/ntpd/ntp_parser.y"

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

#line 1116 "ntp_parser.c"

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
#line 378 "../../dist/ntpd/ntp_parser.y"
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
#line 414 "../../dist/ntpd/ntp_parser.y"
	{
			peer_node *my_node;

			my_node = create_peer_node(yystack.l_mark[-2].Integer, yystack.l_mark[-1].Address_node, yystack.l_mark[0].Attr_val_fifo);
			APPEND_G_FIFO(cfgt.peers, my_node);
		}
break;
case 26:
#line 433 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Address_node = create_address_node(yystack.l_mark[0].String, yystack.l_mark[-1].Integer); }
break;
case 27:
#line 438 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Address_node = create_address_node(yystack.l_mark[0].String, AF_UNSPEC); }
break;
case 28:
#line 443 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Integer = AF_INET; }
break;
case 29:
#line 445 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Integer = AF_INET6; }
break;
case 30:
#line 450 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val_fifo = NULL; }
break;
case 31:
#line 452 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 35:
#line 466 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(T_Flag, yystack.l_mark[0].Integer); }
break;
case 44:
#line 482 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 45:
#line 484 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_uval(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 52:
#line 498 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_sval(yystack.l_mark[-1].Integer, yystack.l_mark[0].String); }
break;
case 54:
#line 512 "../../dist/ntpd/ntp_parser.y"
	{
			unpeer_node *my_node;

			my_node = create_unpeer_node(yystack.l_mark[0].Address_node);
			if (my_node)
				APPEND_G_FIFO(cfgt.unpeers, my_node);
		}
break;
case 57:
#line 533 "../../dist/ntpd/ntp_parser.y"
	{ cfgt.broadcastclient = 1; }
break;
case 58:
#line 535 "../../dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.manycastserver, yystack.l_mark[0].Address_fifo); }
break;
case 59:
#line 537 "../../dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.multicastclient, yystack.l_mark[0].Address_fifo); }
break;
case 60:
#line 539 "../../dist/ntpd/ntp_parser.y"
	{ cfgt.mdnstries = yystack.l_mark[0].Integer; }
break;
case 61:
#line 550 "../../dist/ntpd/ntp_parser.y"
	{
			attr_val *atrv;

			atrv = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer);
			APPEND_G_FIFO(cfgt.vars, atrv);
		}
break;
case 62:
#line 557 "../../dist/ntpd/ntp_parser.y"
	{ cfgt.auth.control_key = yystack.l_mark[0].Integer; }
break;
case 63:
#line 559 "../../dist/ntpd/ntp_parser.y"
	{
			cfgt.auth.cryptosw++;
			CONCAT_G_FIFOS(cfgt.auth.crypto_cmd_list, yystack.l_mark[0].Attr_val_fifo);
		}
break;
case 64:
#line 564 "../../dist/ntpd/ntp_parser.y"
	{ cfgt.auth.keys = yystack.l_mark[0].String; }
break;
case 65:
#line 566 "../../dist/ntpd/ntp_parser.y"
	{ cfgt.auth.keysdir = yystack.l_mark[0].String; }
break;
case 66:
#line 568 "../../dist/ntpd/ntp_parser.y"
	{ cfgt.auth.request_key = yystack.l_mark[0].Integer; }
break;
case 67:
#line 570 "../../dist/ntpd/ntp_parser.y"
	{ cfgt.auth.revoke = yystack.l_mark[0].Integer; }
break;
case 68:
#line 572 "../../dist/ntpd/ntp_parser.y"
	{
			cfgt.auth.trusted_key_list = yystack.l_mark[0].Attr_val_fifo;

			/* if (!cfgt.auth.trusted_key_list)*/
			/* 	cfgt.auth.trusted_key_list = $2;*/
			/* else*/
			/* 	LINK_SLIST(cfgt.auth.trusted_key_list, $2, link);*/
		}
break;
case 69:
#line 581 "../../dist/ntpd/ntp_parser.y"
	{ cfgt.auth.ntp_signd_socket = yystack.l_mark[0].String; }
break;
case 70:
#line 586 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val_fifo = NULL; }
break;
case 71:
#line 588 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 72:
#line 596 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_sval(yystack.l_mark[-1].Integer, yystack.l_mark[0].String); }
break;
case 73:
#line 598 "../../dist/ntpd/ntp_parser.y"
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
#line 623 "../../dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.orphan_cmds, yystack.l_mark[0].Attr_val_fifo); }
break;
case 80:
#line 628 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 81:
#line 633 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 82:
#line 641 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-1].Integer, (double)yystack.l_mark[0].Integer); }
break;
case 83:
#line 643 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-1].Integer, yystack.l_mark[0].Double); }
break;
case 84:
#line 645 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-1].Integer, (double)yystack.l_mark[0].Integer); }
break;
case 95:
#line 671 "../../dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.stats_list, yystack.l_mark[0].Int_fifo); }
break;
case 96:
#line 673 "../../dist/ntpd/ntp_parser.y"
	{
			if (lex_from_file()) {
				cfgt.stats_dir = yystack.l_mark[0].String;
			} else {
				YYFREE(yystack.l_mark[0].String);
				yyerror("statsdir remote configuration ignored");
			}
		}
break;
case 97:
#line 682 "../../dist/ntpd/ntp_parser.y"
	{
			filegen_node *fgn;

			fgn = create_filegen_node(yystack.l_mark[-1].Integer, yystack.l_mark[0].Attr_val_fifo);
			APPEND_G_FIFO(cfgt.filegen_opts, fgn);
		}
break;
case 98:
#line 692 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Int_fifo = yystack.l_mark[-1].Int_fifo;
			APPEND_G_FIFO(yyval.Int_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 99:
#line 697 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Int_fifo = NULL;
			APPEND_G_FIFO(yyval.Int_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 108:
#line 716 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val_fifo = NULL; }
break;
case 109:
#line 718 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 110:
#line 726 "../../dist/ntpd/ntp_parser.y"
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
case 111:
#line 736 "../../dist/ntpd/ntp_parser.y"
	{
			if (lex_from_file()) {
				yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer);
			} else {
				yyval.Attr_val = NULL;
				yyerror("filegen type remote config ignored");
			}
		}
break;
case 112:
#line 745 "../../dist/ntpd/ntp_parser.y"
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
case 113:
#line 760 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(T_Flag, yystack.l_mark[0].Integer); }
break;
case 125:
#line 790 "../../dist/ntpd/ntp_parser.y"
	{
			CONCAT_G_FIFOS(cfgt.discard_opts, yystack.l_mark[0].Attr_val_fifo);
		}
break;
case 126:
#line 794 "../../dist/ntpd/ntp_parser.y"
	{
			CONCAT_G_FIFOS(cfgt.mru_opts, yystack.l_mark[0].Attr_val_fifo);
		}
break;
case 127:
#line 798 "../../dist/ntpd/ntp_parser.y"
	{
			restrict_node *rn;

			rn = create_restrict_node(yystack.l_mark[-1].Address_node, NULL, yystack.l_mark[0].Int_fifo,
						  lex_current()->curpos.nline);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
break;
case 128:
#line 806 "../../dist/ntpd/ntp_parser.y"
	{
			restrict_node *rn;

			rn = create_restrict_node(yystack.l_mark[-3].Address_node, yystack.l_mark[-1].Address_node, yystack.l_mark[0].Int_fifo,
						  lex_current()->curpos.nline);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
break;
case 129:
#line 814 "../../dist/ntpd/ntp_parser.y"
	{
			restrict_node *rn;

			rn = create_restrict_node(NULL, NULL, yystack.l_mark[0].Int_fifo,
						  lex_current()->curpos.nline);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
break;
case 130:
#line 822 "../../dist/ntpd/ntp_parser.y"
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
case 131:
#line 837 "../../dist/ntpd/ntp_parser.y"
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
case 132:
#line 852 "../../dist/ntpd/ntp_parser.y"
	{
			restrict_node *	rn;

			APPEND_G_FIFO(yystack.l_mark[0].Int_fifo, create_int_node(yystack.l_mark[-1].Integer));
			rn = create_restrict_node(
				NULL, NULL, yystack.l_mark[0].Int_fifo, lex_current()->curpos.nline);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
break;
case 133:
#line 864 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Int_fifo = NULL; }
break;
case 134:
#line 866 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Int_fifo = yystack.l_mark[-1].Int_fifo;
			APPEND_G_FIFO(yyval.Int_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 150:
#line 892 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 151:
#line 897 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 152:
#line 905 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 156:
#line 916 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 157:
#line 921 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 158:
#line 929 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 167:
#line 949 "../../dist/ntpd/ntp_parser.y"
	{
			addr_opts_node *aon;

			aon = create_addr_opts_node(yystack.l_mark[-1].Address_node, yystack.l_mark[0].Attr_val_fifo);
			APPEND_G_FIFO(cfgt.fudge, aon);
		}
break;
case 168:
#line 959 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 169:
#line 964 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 170:
#line 972 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-1].Integer, yystack.l_mark[0].Double); }
break;
case 171:
#line 974 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 172:
#line 976 "../../dist/ntpd/ntp_parser.y"
	{
			if (yystack.l_mark[0].Integer >= 0 && yystack.l_mark[0].Integer <= 16) {
				yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer);
			} else {
				yyval.Attr_val = NULL;
				yyerror("fudge factor: stratum value not in [0..16], ignored");
			}
		}
break;
case 173:
#line 985 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_sval(yystack.l_mark[-1].Integer, yystack.l_mark[0].String); }
break;
case 174:
#line 987 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_sval(yystack.l_mark[-1].Integer, yystack.l_mark[0].String); }
break;
case 181:
#line 1008 "../../dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.rlimit, yystack.l_mark[0].Attr_val_fifo); }
break;
case 182:
#line 1013 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 183:
#line 1018 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 184:
#line 1026 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 188:
#line 1042 "../../dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.enable_opts, yystack.l_mark[0].Attr_val_fifo); }
break;
case 189:
#line 1044 "../../dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.disable_opts, yystack.l_mark[0].Attr_val_fifo); }
break;
case 190:
#line 1049 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 191:
#line 1054 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 192:
#line 1062 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(T_Flag, yystack.l_mark[0].Integer); }
break;
case 193:
#line 1064 "../../dist/ntpd/ntp_parser.y"
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
case 205:
#line 1102 "../../dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.tinker, yystack.l_mark[0].Attr_val_fifo); }
break;
case 206:
#line 1107 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 207:
#line 1112 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 208:
#line 1120 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-1].Integer, yystack.l_mark[0].Double); }
break;
case 221:
#line 1145 "../../dist/ntpd/ntp_parser.y"
	{
			attr_val *av;

			av = create_attr_dval(yystack.l_mark[-1].Integer, yystack.l_mark[0].Double);
			APPEND_G_FIFO(cfgt.vars, av);
		}
break;
case 222:
#line 1152 "../../dist/ntpd/ntp_parser.y"
	{
			attr_val *av;

			av = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer);
			APPEND_G_FIFO(cfgt.vars, av);
		}
break;
case 223:
#line 1159 "../../dist/ntpd/ntp_parser.y"
	{
			attr_val *av;

			av = create_attr_sval(yystack.l_mark[-1].Integer, yystack.l_mark[0].String);
			APPEND_G_FIFO(cfgt.vars, av);
		}
break;
case 224:
#line 1166 "../../dist/ntpd/ntp_parser.y"
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
case 225:
#line 1182 "../../dist/ntpd/ntp_parser.y"
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
case 226:
#line 1201 "../../dist/ntpd/ntp_parser.y"
	{ lex_flush_stack(); }
break;
case 227:
#line 1203 "../../dist/ntpd/ntp_parser.y"
	{ /* see drift_parm below for actions */ }
break;
case 228:
#line 1205 "../../dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.logconfig, yystack.l_mark[0].Attr_val_fifo); }
break;
case 229:
#line 1207 "../../dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.phone, yystack.l_mark[0].String_fifo); }
break;
case 230:
#line 1209 "../../dist/ntpd/ntp_parser.y"
	{ APPEND_G_FIFO(cfgt.setvar, yystack.l_mark[0].Set_var); }
break;
case 231:
#line 1211 "../../dist/ntpd/ntp_parser.y"
	{
			addr_opts_node *aon;

			aon = create_addr_opts_node(yystack.l_mark[-1].Address_node, yystack.l_mark[0].Attr_val_fifo);
			APPEND_G_FIFO(cfgt.trap, aon);
		}
break;
case 232:
#line 1218 "../../dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.ttl, yystack.l_mark[0].Attr_val_fifo); }
break;
case 237:
#line 1233 "../../dist/ntpd/ntp_parser.y"
	{
#ifndef LEAP_SMEAR
			yyerror("Built without LEAP_SMEAR support.");
#endif
		}
break;
case 243:
#line 1253 "../../dist/ntpd/ntp_parser.y"
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
case 244:
#line 1264 "../../dist/ntpd/ntp_parser.y"
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
case 245:
#line 1277 "../../dist/ntpd/ntp_parser.y"
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
case 246:
#line 1290 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Set_var = create_setvar_node(yystack.l_mark[-3].String, yystack.l_mark[-1].String, yystack.l_mark[0].Integer); }
break;
case 248:
#line 1296 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Integer = 0; }
break;
case 249:
#line 1301 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val_fifo = NULL; }
break;
case 250:
#line 1303 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 251:
#line 1311 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 252:
#line 1313 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val = create_attr_sval(yystack.l_mark[-1].Integer, estrdup(yystack.l_mark[0].Address_node->address));
			destroy_address_node(yystack.l_mark[0].Address_node);
		}
break;
case 253:
#line 1321 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 254:
#line 1326 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 255:
#line 1334 "../../dist/ntpd/ntp_parser.y"
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
case 256:
#line 1359 "../../dist/ntpd/ntp_parser.y"
	{
			nic_rule_node *nrn;

			nrn = create_nic_rule_node(yystack.l_mark[0].Integer, NULL, yystack.l_mark[-1].Integer);
			APPEND_G_FIFO(cfgt.nic_rules, nrn);
		}
break;
case 257:
#line 1366 "../../dist/ntpd/ntp_parser.y"
	{
			nic_rule_node *nrn;

			nrn = create_nic_rule_node(0, yystack.l_mark[0].String, yystack.l_mark[-1].Integer);
			APPEND_G_FIFO(cfgt.nic_rules, nrn);
		}
break;
case 267:
#line 1394 "../../dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.reset_counters, yystack.l_mark[0].Int_fifo); }
break;
case 268:
#line 1399 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Int_fifo = yystack.l_mark[-1].Int_fifo;
			APPEND_G_FIFO(yyval.Int_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 269:
#line 1404 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Int_fifo = NULL;
			APPEND_G_FIFO(yyval.Int_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 277:
#line 1428 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 278:
#line 1433 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 279:
#line 1441 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 280:
#line 1446 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 281:
#line 1454 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival('i', yystack.l_mark[0].Integer); }
break;
case 283:
#line 1460 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_rangeval('-', yystack.l_mark[-3].Integer, yystack.l_mark[-1].Integer); }
break;
case 284:
#line 1465 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.String_fifo = yystack.l_mark[-1].String_fifo;
			APPEND_G_FIFO(yyval.String_fifo, create_string_node(yystack.l_mark[0].String));
		}
break;
case 285:
#line 1470 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.String_fifo = NULL;
			APPEND_G_FIFO(yyval.String_fifo, create_string_node(yystack.l_mark[0].String));
		}
break;
case 286:
#line 1478 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Address_fifo = yystack.l_mark[-1].Address_fifo;
			APPEND_G_FIFO(yyval.Address_fifo, yystack.l_mark[0].Address_node);
		}
break;
case 287:
#line 1483 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Address_fifo = NULL;
			APPEND_G_FIFO(yyval.Address_fifo, yystack.l_mark[0].Address_node);
		}
break;
case 288:
#line 1491 "../../dist/ntpd/ntp_parser.y"
	{
			if (yystack.l_mark[0].Integer != 0 && yystack.l_mark[0].Integer != 1) {
				yyerror("Integer value is not boolean (0 or 1). Assuming 1");
				yyval.Integer = 1;
			} else {
				yyval.Integer = yystack.l_mark[0].Integer;
			}
		}
break;
case 289:
#line 1499 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Integer = 1; }
break;
case 290:
#line 1500 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Integer = 0; }
break;
case 291:
#line 1504 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Double = (double)yystack.l_mark[0].Integer; }
break;
case 293:
#line 1515 "../../dist/ntpd/ntp_parser.y"
	{
			sim_node *sn;

			sn =  create_sim_node(yystack.l_mark[-2].Attr_val_fifo, yystack.l_mark[-1].Sim_server_fifo);
			APPEND_G_FIFO(cfgt.sim_details, sn);

			/* Revert from ; to \n for end-of-command */
			old_config_style = 1;
		}
break;
case 294:
#line 1532 "../../dist/ntpd/ntp_parser.y"
	{ old_config_style = 0; }
break;
case 295:
#line 1537 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-2].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[-1].Attr_val);
		}
break;
case 296:
#line 1542 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[-1].Attr_val);
		}
break;
case 297:
#line 1550 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-2].Integer, yystack.l_mark[0].Double); }
break;
case 300:
#line 1560 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Sim_server_fifo = yystack.l_mark[-1].Sim_server_fifo;
			APPEND_G_FIFO(yyval.Sim_server_fifo, yystack.l_mark[0].Sim_server);
		}
break;
case 301:
#line 1565 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Sim_server_fifo = NULL;
			APPEND_G_FIFO(yyval.Sim_server_fifo, yystack.l_mark[0].Sim_server);
		}
break;
case 302:
#line 1573 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Sim_server = ONLY_SIM(create_sim_server(yystack.l_mark[-4].Address_node, yystack.l_mark[-2].Double, yystack.l_mark[-1].Sim_script_fifo)); }
break;
case 303:
#line 1578 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Double = yystack.l_mark[-1].Double; }
break;
case 304:
#line 1583 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Address_node = yystack.l_mark[0].Address_node; }
break;
case 305:
#line 1588 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Sim_script_fifo = yystack.l_mark[-1].Sim_script_fifo;
			APPEND_G_FIFO(yyval.Sim_script_fifo, yystack.l_mark[0].Sim_script);
		}
break;
case 306:
#line 1593 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Sim_script_fifo = NULL;
			APPEND_G_FIFO(yyval.Sim_script_fifo, yystack.l_mark[0].Sim_script);
		}
break;
case 307:
#line 1601 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Sim_script = ONLY_SIM(create_sim_script_info(yystack.l_mark[-3].Double, yystack.l_mark[-1].Attr_val_fifo)); }
break;
case 308:
#line 1606 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-2].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[-1].Attr_val);
		}
break;
case 309:
#line 1611 "../../dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[-1].Attr_val);
		}
break;
case 310:
#line 1619 "../../dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-2].Integer, yystack.l_mark[0].Double); }
break;
#line 2315 "ntp_parser.c"
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
