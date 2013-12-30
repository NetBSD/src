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

#line 14 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
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
#line 51 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
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
#line 83 "ntp_parser.c"

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
#define T_Ellipsis 289
#define T_Enable 290
#define T_End 291
#define T_False 292
#define T_File 293
#define T_Filegen 294
#define T_Filenum 295
#define T_Flag1 296
#define T_Flag2 297
#define T_Flag3 298
#define T_Flag4 299
#define T_Flake 300
#define T_Floor 301
#define T_Freq 302
#define T_Fudge 303
#define T_Host 304
#define T_Huffpuff 305
#define T_Iburst 306
#define T_Ident 307
#define T_Ignore 308
#define T_Incalloc 309
#define T_Incmem 310
#define T_Initalloc 311
#define T_Initmem 312
#define T_Includefile 313
#define T_Integer 314
#define T_Interface 315
#define T_Intrange 316
#define T_Io 317
#define T_Ipv4 318
#define T_Ipv4_flag 319
#define T_Ipv6 320
#define T_Ipv6_flag 321
#define T_Kernel 322
#define T_Key 323
#define T_Keys 324
#define T_Keysdir 325
#define T_Kod 326
#define T_Mssntp 327
#define T_Leapfile 328
#define T_Limited 329
#define T_Link 330
#define T_Listen 331
#define T_Logconfig 332
#define T_Logfile 333
#define T_Loopstats 334
#define T_Lowpriotrap 335
#define T_Manycastclient 336
#define T_Manycastserver 337
#define T_Mask 338
#define T_Maxage 339
#define T_Maxclock 340
#define T_Maxdepth 341
#define T_Maxdist 342
#define T_Maxmem 343
#define T_Maxpoll 344
#define T_Mdnstries 345
#define T_Mem 346
#define T_Memlock 347
#define T_Minclock 348
#define T_Mindepth 349
#define T_Mindist 350
#define T_Minimum 351
#define T_Minpoll 352
#define T_Minsane 353
#define T_Mode 354
#define T_Mode7 355
#define T_Monitor 356
#define T_Month 357
#define T_Mru 358
#define T_Multicastclient 359
#define T_Nic 360
#define T_Nolink 361
#define T_Nomodify 362
#define T_Nomrulist 363
#define T_None 364
#define T_Nonvolatile 365
#define T_Nopeer 366
#define T_Noquery 367
#define T_Noselect 368
#define T_Noserve 369
#define T_Notrap 370
#define T_Notrust 371
#define T_Ntp 372
#define T_Ntpport 373
#define T_NtpSignDsocket 374
#define T_Orphan 375
#define T_Orphanwait 376
#define T_Panic 377
#define T_Peer 378
#define T_Peerstats 379
#define T_Phone 380
#define T_Pid 381
#define T_Pidfile 382
#define T_Pool 383
#define T_Port 384
#define T_Preempt 385
#define T_Prefer 386
#define T_Protostats 387
#define T_Pw 388
#define T_Randfile 389
#define T_Rawstats 390
#define T_Refid 391
#define T_Requestkey 392
#define T_Reset 393
#define T_Restrict 394
#define T_Revoke 395
#define T_Rlimit 396
#define T_Saveconfigdir 397
#define T_Server 398
#define T_Setvar 399
#define T_Source 400
#define T_Stacksize 401
#define T_Statistics 402
#define T_Stats 403
#define T_Statsdir 404
#define T_Step 405
#define T_Stepout 406
#define T_Stratum 407
#define T_String 408
#define T_Sys 409
#define T_Sysstats 410
#define T_Tick 411
#define T_Time1 412
#define T_Time2 413
#define T_Timer 414
#define T_Timingstats 415
#define T_Tinker 416
#define T_Tos 417
#define T_Trap 418
#define T_True 419
#define T_Trustedkey 420
#define T_Ttl 421
#define T_Type 422
#define T_U_int 423
#define T_Unconfig 424
#define T_Unpeer 425
#define T_Version 426
#define T_WanderThreshold 427
#define T_Week 428
#define T_Wildcard 429
#define T_Xleave 430
#define T_Year 431
#define T_Flag 432
#define T_EOC 433
#define T_Simulate 434
#define T_Beep_Delay 435
#define T_Sim_Duration 436
#define T_Server_Offset 437
#define T_Duration 438
#define T_Freq_Offset 439
#define T_Wander 440
#define T_Jitter 441
#define T_Prop_Delay 442
#define T_Proc_Delay 443
#define YYERRCODE 256
static const short yylhs[] = {                           -1,
    0,   86,   86,   86,   87,   87,   87,   87,   87,   87,
   87,   87,   87,   87,   87,   87,   87,   87,   88,    7,
    7,    7,    7,    7,    3,    3,   31,    4,    4,   46,
   46,   43,   43,   43,   44,   45,   45,   45,   45,   45,
   45,   45,   45,   47,   47,   48,   48,   48,   48,   48,
   48,   49,   50,   89,   72,   72,   90,   90,   90,   90,
   91,   91,   91,   91,   91,   91,   91,   91,   91,   11,
   11,   10,   10,   12,   12,   12,   12,   12,   94,   69,
   69,   66,   66,   66,   68,   68,   68,   68,   68,   68,
   67,   67,   67,   67,   92,   92,   92,   56,   56,   55,
   55,   55,   55,   55,   55,   55,   55,   18,   18,   17,
   17,   17,   17,   32,   32,   16,   16,   19,   19,   19,
   19,   19,   19,   19,   93,   93,   93,   93,   93,   93,
   93,   93,    2,    2,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,   15,
   15,   13,   14,   14,   14,   40,   40,   38,   39,   39,
   39,   39,   39,   39,   39,   39,   95,   23,   23,   20,
   20,   20,   20,   20,   22,   22,   21,   21,   21,   21,
   96,   54,   54,   53,   52,   52,   52,   97,   97,   61,
   61,   58,   58,   59,   59,   59,   59,   59,   59,   60,
   60,   98,   65,   65,   64,   63,   63,   63,   63,   63,
   63,   63,   63,   99,   99,   99,   99,   99,   99,   99,
   99,   99,   99,   99,   99,   99,   35,   35,   35,   36,
   36,   36,   37,   37,  101,  101,  101,   73,   62,   62,
   71,   71,   70,   70,   34,   34,   33,   29,   29,   30,
   30,   41,   41,   41,   41,   28,   28,   28,   51,    9,
    9,    8,    8,    8,    8,    8,    8,    8,   24,   24,
   25,   25,   26,   26,   27,   57,   57,    5,    5,    6,
    6,    6,   42,   42,  100,  102,   75,   75,   74,   76,
   76,   77,   77,   78,   79,   80,   82,   82,   81,   84,
   84,   85,   83,   83,   83,   83,   83,
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
    1,    2,    2,    1,    2,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    2,    2,    2,    3,    1,
    2,    2,    2,    2,    3,    2,    1,    1,    1,    1,
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
    0,    0,   23,   57,  227,    0,   70,    0,    0,    0,
    0,  220,    0,    0,  230,    0,  250,    0,    0,  231,
    0,  233,   24,    0,    0,    0,    0,  251,  228,    0,
   22,    0,  232,   21,    0,    0,    0,    0,    0,  234,
   20,    0,    0,    0,  229,    0,    0,    0,    0,    0,
   55,   56,  286,    0,    0,  214,    0,    0,    0,    0,
  215,    0,    0,    0,    6,    7,    8,    9,   10,   11,
   12,   13,   14,   15,   16,   17,   18,    0,    4,   61,
   62,    0,  194,  195,  196,  197,  200,  198,  199,  201,
  191,  192,  193,    0,  153,  154,  155,  151,    0,    0,
    0,  221,    0,  100,  101,  102,  103,  107,  104,  105,
  106,  108,   28,   29,   27,    0,    0,   25,    0,   64,
   65,  247,  246,    0,  279,    0,   60,  159,  160,  161,
  162,  163,  164,  165,  166,  157,    0,    0,    0,   69,
  277,    0,   66,  262,  263,  264,  265,  266,  267,  268,
  261,    0,  133,    0,    0,  133,  133,    0,   67,  187,
  185,  186,    0,  183,    0,    0,  224,   99,    0,   96,
  206,  207,  208,  209,  210,  211,  212,  213,    0,  204,
    0,   90,   85,    0,   86,   94,   92,   93,   91,   89,
   87,   88,   81,    0,    0,    0,  241,  273,    0,    0,
  272,  274,  270,    0,   30,  258,  257,  256,    0,  284,
  283,  216,  217,  218,   54,    0,    3,    0,   78,   74,
   75,   76,   77,    0,   71,    0,  190,  152,  150,  236,
    0,    0,  177,  178,  179,  180,    0,    0,  175,  176,
  169,    0,    0,    0,   26,  219,  245,  278,  158,  156,
  276,  260,    0,  133,  133,    0,    0,    0,  184,  182,
    0,   98,  205,  203,  282,  280,  281,   84,   83,   82,
   80,    0,    0,  271,  269,    0,  252,  253,  254,  249,
  255,  248,    2,  290,  291,    0,    0,    0,   73,   72,
  117,  116,    0,  114,  115,    0,  113,  109,  112,  173,
  174,  172,  171,  170,  168,  135,  136,  137,  138,  139,
  140,  141,  142,  143,  144,  145,  146,  147,  148,  149,
  134,    0,    0,  133,    0,    0,    0,  242,    0,   36,
   37,   38,   53,   46,   48,   47,   50,   39,   40,   41,
   42,   49,   51,   43,   31,   32,   35,   33,    0,   34,
    0,  288,    0,    0,    0,  293,    0,    0,  110,  124,
  120,  122,  118,  119,  121,  123,  111,    0,  239,  238,
  244,  243,    0,   44,   45,   52,    0,  287,  285,  292,
    0,  289,  275,  296,    0,    0,    0,    0,  298,    0,
    0,    0,  294,  297,  295,    0,    0,  303,  304,  305,
  306,  307,    0,    0,    0,    0,  299,    0,  301,  302,
  300,
};
static const short yydgoto[] = {                         54,
  321,  253,  125,  117,  126,  268,   55,  151,  152,  225,
   82,  226,   98,   99,  100,  297,  298,  231,  367,  241,
  242,  243,  244,  204,  200,  201,  202,  209,   56,   57,
  118,  299,  123,  124,   58,   59,   60,  136,  137,  138,
  282,  212,  345,  346,  347,  276,  348,  349,  350,  351,
   61,  163,  164,  165,  112,  169,  142,   91,   92,   93,
   94,  370,  179,  180,  181,  193,  194,  195,  196,  328,
  272,   62,  167,  286,  287,  288,  355,  356,  386,  357,
  389,  390,  403,  404,  405,   63,   64,   65,   66,   67,
   68,   69,   70,   71,   72,   73,   74,   75,   76,   77,
  102,   78,
};
static const short yysindex[] = {                      -231,
 -399, -279,    0,    0,    0, -278,    0, -127, -225, -354,
 -127,    0, -257, -297,    0, -353,    0, -351, -350,    0,
 -344,    0,    0, -297, -241,  -59, -297,    0,    0, -343,
    0, -333,    0,    0, -236, -200, -177, -234, -280,    0,
    0, -327, -257, -325,    0, -189,  155, -322,  -36, -227,
    0,    0,    0,    0, -297,    0, -261, -270, -316, -310,
    0, -297,  -22, -334,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  -23,    0,    0,
    0, -197,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0, -127,    0,    0,    0,    0, -211, -225,
 -168,    0, -127,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0, -208, -299,    0,  -22,    0,
    0,    0,    0, -344,    0, -297,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0, -195,  -59, -297,    0,
    0, -285,    0,    0,    0,    0,    0,    0,    0,    0,
    0, -200,    0, -157, -145,    0,    0, -190,    0,    0,
    0,    0, -164,    0, -280,   93,    0,    0, -257,    0,
    0,    0,    0,    0,    0,    0,    0,    0, -270,    0,
 -189,    0,    0, -282,    0,    0,    0,    0,    0,    0,
    0,    0,    0, -270, -159,  155,    0,    0, -155,  -36,
    0,    0,    0, -154,    0,    0,    0,    0, -252,    0,
    0,    0,    0,    0,    0, -276,    0, -406,    0,    0,
    0,    0,    0, -144,    0, -239,    0,    0,    0,    0,
  -63, -233,    0,    0,    0,    0, -232, -142,    0,    0,
    0, -282, -270, -208,    0,    0,    0,    0,    0,    0,
    0,    0,  106,    0,    0,  106,  106, -322,    0,    0,
 -230,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0, -289, -115,    0,    0,   61,    0,    0,    0,    0,
    0,    0,    0,    0,    0, -237, -385,  120,    0,    0,
    0,    0, -207,    0,    0, -249,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  106,  106,    0,  -84, -322, -112,    0, -108,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0, -303,    0,
 -201,    0,  139, -222, -119,    0,   85, -270,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  106,    0,    0,
    0,    0,  171,    0,    0,    0, -297,    0,    0,    0,
 -218,    0,    0,    0,  154, -217, -270,  164,    0, -117,
 -194, -270,    0,    0,    0,  103, -206,    0,    0,    0,
    0,    0,  177, -123, -193, -270,    0, -192,    0,    0,
    0,
};
static const short yyrindex[] = {                      -184,
    0,    0,    0,    0,    0,    0,    0,    0,    0, -176,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    1,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0, -175,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0, -173,    0,    0,    0,    0,    0, -170,
 -169,    0, -167,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0, -184,    0,
    0,    0,    0, -163,    0, -162,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0, -160, -158,    0,
    0, -156,    0,    0,    0,    0,    0,    0,    0,    0,
    0, -150,    0, -165, -152,    0,    0,   92,    0,    0,
    0,    0,    0,    0, -147,    0,    0,    0, -146,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
 -139,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0, -399,    0,    0,    0, -138,
    0,    0,    0, -137,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
 -136,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0, -134,    0,    0,    0,    0,    0,    0,
    0,    0, -133,    0,    0, -132, -129,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0, -128,    0,    0,    0, -126,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0, -125, -124,    0, -121,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0, -120,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,
};
static const short yygindex[] = {                         0,
    0, -114,  -14,    0,  217,   17,    0,  122,    0,    0,
    0,    0,  188,    0,    0,    0,    0,    0,    0,   45,
    0,    0,    0,    0,    0,  125,    0,    0,    0,    0,
  -34,    0,  198,    0,    0,    0,    0,  189,    0,    0,
    0, -174,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,  161,    0,  -31,    0,    0,  -75,    0,    0,
  317,    0,    0,  148,    0,  134,    0,    0,    0,    0,
    0,    0,    0,   44,    0,    0,    0,  -21,    0,    0,
  -57,    0,    0,    0,  -69,    0,  -45,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,
};
#define YYTABLESIZE 532
static const short yytable[] = {                        116,
    1,  407,  158,  199,  263,  379,  277,  393,  360,  265,
  374,  168,  353,  197,  160,  210,  104,  216,  227,  269,
  105,  113,  157,  114,    1,  326,  206,  227,  284,  285,
  361,  266,    2,   79,   80,   81,    3,    4,    5,   95,
  205,  256,  257,  211,    6,    7,  207,  215,  232,  284,
  285,    8,    9,  101,  119,   10,  120,  121,   11,   12,
  144,  145,   13,  122,  140,  278,  161,  279,  304,  208,
  171,   14,  127,  246,  141,   15,  106,  143,  146,  159,
  166,   16,  170,   17,  219,  115,  203,  233,  234,  235,
  236,  213,   18,   19,  327,  172,   20,  214,  217,  218,
   21,   22,  228,  153,   23,   24,  220,  362,  245,  221,
  115,  248,  173,   25,  363,  174,  147,  230,  249,  375,
  162,  107,  251,  254,  248,   96,   26,   27,   28,  108,
   97,  364,  109,   29,   83,  255,  267,  262,   84,  322,
  323,  154,   30,  155,   85,  148,   31,  258,   32,  259,
   33,   34,  110,  261,  270,  280,  283,  111,  273,  275,
   35,   36,   37,   38,   39,   40,   41,   42,  290,  289,
   43,  302,   44,  329,  300,  301,  281,  325,  365,   45,
  358,  366,  237,  382,   46,   47,   48,  175,   49,   50,
  222,  223,   51,   52,   86,  352,  369,  224,  238,  377,
  359,  372,   53,  239,  240,  373,  376,  381,  149,  368,
  378,  383,  391,  150,  387,  176,  177,  396,  385,  291,
  388,  178,  156,  324,  392,  397,  292,   87,   88,  293,
  115,  410,  398,  399,  400,  401,  402,  406,  395,  409,
  411,    2,   28,  139,   89,    3,    4,    5,    5,  128,
  129,  130,  131,    6,    7,   29,  237,   63,  303,  189,
    8,    9,  125,  235,   10,  188,  294,   11,   12,  222,
   58,   13,  126,  252,   59,   90,  223,  198,  353,  132,
   14,  133,  259,  134,   15,  181,   95,  229,  305,  135,
   16,  371,   17,  202,   68,  226,   97,  295,  167,  129,
  132,   18,   19,  127,  225,   20,   19,  130,  131,   21,
   22,  240,  128,   23,   24,  398,  399,  400,  401,  402,
  388,  247,   25,  330,  274,  260,  250,  103,  264,  271,
  354,  331,  394,  380,  408,   26,   27,   28,    0,    0,
    0,    0,   29,    0,    0,    0,    0,    0,    0,    0,
    0,   30,    0,    0,    0,   31,    0,   32,  296,   33,
   34,    0,  384,    0,    0,    0,  332,  333,    0,   35,
   36,   37,   38,   39,   40,   41,   42,    0,    0,   43,
    0,   44,    0,  334,    0,    0,    0,    0,   45,    0,
    0,   25,    0,   46,   47,   48,    0,   49,   50,   25,
    0,   51,   52,    0,  335,  306,    0,    0,    0,    0,
    0,   53,  336,  307,  337,    0,    0,   25,   25,    0,
   25,  182,    0,    0,    0,    0,   25,  183,  338,  184,
    0,  308,  309,    5,  310,    0,    0,    0,    0,    0,
  311,    0,    0,    0,    0,  339,  340,    0,    0,    0,
    0,    0,    0,   25,   25,  185,    0,   25,   25,    0,
   25,   25,   25,    0,   25,    0,    0,  312,  313,    0,
    0,  314,  315,    0,  316,  317,  318,    0,  319,  341,
    0,  342,    0,    0,    0,    0,  343,    0,    0,    0,
  344,    0,    0,    0,  186,    0,  187,    0,    0,    0,
    0,    0,  188,    0,  189,    0,    0,  190,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   25,    0,    0,
    0,    0,    0,    0,   25,    0,    0,    0,    0,  191,
  192,  320,
};
static const short yycheck[] = {                         14,
    0,  125,   37,   40,  179,  125,  259,  125,  258,  292,
  314,   43,  398,   48,  295,  286,  274,   63,   94,  194,
  278,  319,   37,  321,  256,  315,  288,  103,  435,  436,
  280,  314,  264,  433,  314,  314,  268,  269,  270,  265,
   55,  156,  157,  314,  276,  277,  308,   62,  257,  435,
  436,  283,  284,  408,  408,  287,  408,  408,  290,  291,
  261,  262,  294,  408,  408,  318,  347,  320,  243,  331,
  260,  303,  314,  119,  408,  307,  334,  314,  279,  314,
  408,  313,  408,  315,  282,  408,  314,  296,  297,  298,
  299,  408,  324,  325,  384,  285,  328,  408,  433,  123,
  332,  333,  314,  281,  336,  337,  304,  357,  408,  307,
  408,  126,  302,  345,  364,  305,  317,  286,  314,  423,
  401,  379,  408,  281,  139,  351,  358,  359,  360,  387,
  356,  381,  390,  365,  262,  281,  419,  169,  266,  254,
  255,  319,  374,  321,  272,  346,  378,  338,  380,  314,
  382,  383,  410,   61,  314,  408,  433,  415,  314,  314,
  392,  393,  394,  395,  396,  397,  398,  399,  408,  314,
  402,  314,  404,  289,  408,  408,  429,  408,  428,  411,
   61,  431,  391,  358,  416,  417,  418,  377,  420,  421,
  388,  389,  424,  425,  322,  433,  281,  395,  407,   61,
  408,  314,  434,  412,  413,  314,  408,  123,  409,  324,
  433,   41,  387,  414,   61,  405,  406,  392,  437,  283,
  438,  411,  400,  258,   61,  123,  290,  355,  356,  293,
  408,  406,  439,  440,  441,  442,  443,   61,  433,  433,
  433,  264,  408,   27,  372,  268,  269,  270,  433,  309,
  310,  311,  312,  276,  277,  408,  433,  433,  242,  433,
  283,  284,  433,  433,  287,  433,  330,  290,  291,  433,
  433,  294,  433,  152,  433,  403,  433,  314,  398,  339,
  303,  341,  433,  343,  307,  433,  433,  100,  244,  349,
  313,  326,  315,  433,  433,  433,  433,  361,  433,  433,
  433,  324,  325,  433,  433,  328,  433,  433,  433,  332,
  333,  433,  433,  336,  337,  439,  440,  441,  442,  443,
  438,  124,  345,  263,  200,  165,  138,   11,  181,  196,
  287,  271,  390,  355,  404,  358,  359,  360,   -1,   -1,
   -1,   -1,  365,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  374,   -1,   -1,   -1,  378,   -1,  380,  422,  382,
  383,   -1,  377,   -1,   -1,   -1,  306,  307,   -1,  392,
  393,  394,  395,  396,  397,  398,  399,   -1,   -1,  402,
   -1,  404,   -1,  323,   -1,   -1,   -1,   -1,  411,   -1,
   -1,  300,   -1,  416,  417,  418,   -1,  420,  421,  308,
   -1,  424,  425,   -1,  344,  300,   -1,   -1,   -1,   -1,
   -1,  434,  352,  308,  354,   -1,   -1,  326,  327,   -1,
  329,  267,   -1,   -1,   -1,   -1,  335,  273,  368,  275,
   -1,  326,  327,  433,  329,   -1,   -1,   -1,   -1,   -1,
  335,   -1,   -1,   -1,   -1,  385,  386,   -1,   -1,   -1,
   -1,   -1,   -1,  362,  363,  301,   -1,  366,  367,   -1,
  369,  370,  371,   -1,  373,   -1,   -1,  362,  363,   -1,
   -1,  366,  367,   -1,  369,  370,  371,   -1,  373,  419,
   -1,  421,   -1,   -1,   -1,   -1,  426,   -1,   -1,   -1,
  430,   -1,   -1,   -1,  340,   -1,  342,   -1,   -1,   -1,
   -1,   -1,  348,   -1,  350,   -1,   -1,  353,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  426,   -1,   -1,
   -1,   -1,   -1,   -1,  433,   -1,   -1,   -1,   -1,  375,
  376,  426,
};
#define YYFINAL 54
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 443
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
"T_Driftfile","T_Drop","T_Ellipsis","T_Enable","T_End","T_False","T_File",
"T_Filegen","T_Filenum","T_Flag1","T_Flag2","T_Flag3","T_Flag4","T_Flake",
"T_Floor","T_Freq","T_Fudge","T_Host","T_Huffpuff","T_Iburst","T_Ident",
"T_Ignore","T_Incalloc","T_Incmem","T_Initalloc","T_Initmem","T_Includefile",
"T_Integer","T_Interface","T_Intrange","T_Io","T_Ipv4","T_Ipv4_flag","T_Ipv6",
"T_Ipv6_flag","T_Kernel","T_Key","T_Keys","T_Keysdir","T_Kod","T_Mssntp",
"T_Leapfile","T_Limited","T_Link","T_Listen","T_Logconfig","T_Logfile",
"T_Loopstats","T_Lowpriotrap","T_Manycastclient","T_Manycastserver","T_Mask",
"T_Maxage","T_Maxclock","T_Maxdepth","T_Maxdist","T_Maxmem","T_Maxpoll",
"T_Mdnstries","T_Mem","T_Memlock","T_Minclock","T_Mindepth","T_Mindist",
"T_Minimum","T_Minpoll","T_Minsane","T_Mode","T_Mode7","T_Monitor","T_Month",
"T_Mru","T_Multicastclient","T_Nic","T_Nolink","T_Nomodify","T_Nomrulist",
"T_None","T_Nonvolatile","T_Nopeer","T_Noquery","T_Noselect","T_Noserve",
"T_Notrap","T_Notrust","T_Ntp","T_Ntpport","T_NtpSignDsocket","T_Orphan",
"T_Orphanwait","T_Panic","T_Peer","T_Peerstats","T_Phone","T_Pid","T_Pidfile",
"T_Pool","T_Port","T_Preempt","T_Prefer","T_Protostats","T_Pw","T_Randfile",
"T_Rawstats","T_Refid","T_Requestkey","T_Reset","T_Restrict","T_Revoke",
"T_Rlimit","T_Saveconfigdir","T_Server","T_Setvar","T_Source","T_Stacksize",
"T_Statistics","T_Stats","T_Statsdir","T_Step","T_Stepout","T_Stratum",
"T_String","T_Sys","T_Sysstats","T_Tick","T_Time1","T_Time2","T_Timer",
"T_Timingstats","T_Tinker","T_Tos","T_Trap","T_True","T_Trustedkey","T_Ttl",
"T_Type","T_U_int","T_Unconfig","T_Unpeer","T_Version","T_WanderThreshold",
"T_Week","T_Wildcard","T_Xleave","T_Year","T_Flag","T_EOC","T_Simulate",
"T_Beep_Delay","T_Sim_Duration","T_Server_Offset","T_Duration","T_Freq_Offset",
"T_Wander","T_Jitter","T_Prop_Delay","T_Proc_Delay","illegal-token",
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
"tinker_option_keyword : T_Stepout",
"tinker_option_keyword : T_Tick",
"miscellaneous_command : interface_command",
"miscellaneous_command : reset_command",
"miscellaneous_command : misc_cmd_dbl_keyword number",
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
"misc_cmd_str_keyword : T_Ident",
"misc_cmd_str_keyword : T_Leapfile",
"misc_cmd_str_keyword : T_Pidfile",
"misc_cmd_str_lcl_keyword : T_Logfile",
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
#line 1580 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"

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

#line 1093 "ntp_parser.c"

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
case 19:
#line 403 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			peer_node *my_node;

			my_node = create_peer_node(yystack.l_mark[-2].Integer, yystack.l_mark[-1].Address_node, yystack.l_mark[0].Attr_val_fifo);
			APPEND_G_FIFO(cfgt.peers, my_node);
		}
break;
case 26:
#line 422 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Address_node = create_address_node(yystack.l_mark[0].String, yystack.l_mark[-1].Integer); }
break;
case 27:
#line 427 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Address_node = create_address_node(yystack.l_mark[0].String, AF_UNSPEC); }
break;
case 28:
#line 432 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Integer = AF_INET; }
break;
case 29:
#line 434 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Integer = AF_INET6; }
break;
case 30:
#line 439 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val_fifo = NULL; }
break;
case 31:
#line 441 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 35:
#line 455 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(T_Flag, yystack.l_mark[0].Integer); }
break;
case 44:
#line 471 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 45:
#line 473 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_uval(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 52:
#line 487 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_sval(yystack.l_mark[-1].Integer, yystack.l_mark[0].String); }
break;
case 54:
#line 501 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			unpeer_node *my_node;
			
			my_node = create_unpeer_node(yystack.l_mark[0].Address_node);
			if (my_node)
				APPEND_G_FIFO(cfgt.unpeers, my_node);
		}
break;
case 57:
#line 522 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ cfgt.broadcastclient = 1; }
break;
case 58:
#line 524 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.manycastserver, yystack.l_mark[0].Address_fifo); }
break;
case 59:
#line 526 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.multicastclient, yystack.l_mark[0].Address_fifo); }
break;
case 60:
#line 528 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ cfgt.mdnstries = yystack.l_mark[0].Integer; }
break;
case 61:
#line 539 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			attr_val *atrv;
			
			atrv = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer);
			APPEND_G_FIFO(cfgt.vars, atrv);
		}
break;
case 62:
#line 546 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ cfgt.auth.control_key = yystack.l_mark[0].Integer; }
break;
case 63:
#line 548 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ 
			cfgt.auth.cryptosw++;
			CONCAT_G_FIFOS(cfgt.auth.crypto_cmd_list, yystack.l_mark[0].Attr_val_fifo);
		}
break;
case 64:
#line 553 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ cfgt.auth.keys = yystack.l_mark[0].String; }
break;
case 65:
#line 555 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ cfgt.auth.keysdir = yystack.l_mark[0].String; }
break;
case 66:
#line 557 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ cfgt.auth.request_key = yystack.l_mark[0].Integer; }
break;
case 67:
#line 559 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ cfgt.auth.revoke = yystack.l_mark[0].Integer; }
break;
case 68:
#line 561 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			cfgt.auth.trusted_key_list = yystack.l_mark[0].Attr_val_fifo;

			/* if (!cfgt.auth.trusted_key_list)*/
			/* 	cfgt.auth.trusted_key_list = $2;*/
			/* else*/
			/* 	LINK_SLIST(cfgt.auth.trusted_key_list, $2, link);*/
		}
break;
case 69:
#line 570 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ cfgt.auth.ntp_signd_socket = yystack.l_mark[0].String; }
break;
case 70:
#line 575 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val_fifo = NULL; }
break;
case 71:
#line 577 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 72:
#line 585 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_sval(yystack.l_mark[-1].Integer, yystack.l_mark[0].String); }
break;
case 73:
#line 587 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
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
#line 612 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.orphan_cmds, yystack.l_mark[0].Attr_val_fifo); }
break;
case 80:
#line 617 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 81:
#line 622 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{	
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 82:
#line 630 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-1].Integer, (double)yystack.l_mark[0].Integer); }
break;
case 83:
#line 632 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-1].Integer, yystack.l_mark[0].Double); }
break;
case 84:
#line 634 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-1].Integer, (double)yystack.l_mark[0].Integer); }
break;
case 95:
#line 660 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.stats_list, yystack.l_mark[0].Int_fifo); }
break;
case 96:
#line 662 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			if (input_from_file) {
				cfgt.stats_dir = yystack.l_mark[0].String;
			} else {
				YYFREE(yystack.l_mark[0].String);
				yyerror("statsdir remote configuration ignored");
			}
		}
break;
case 97:
#line 671 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			filegen_node *fgn;
			
			fgn = create_filegen_node(yystack.l_mark[-1].Integer, yystack.l_mark[0].Attr_val_fifo);
			APPEND_G_FIFO(cfgt.filegen_opts, fgn);
		}
break;
case 98:
#line 681 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Int_fifo = yystack.l_mark[-1].Int_fifo;
			APPEND_G_FIFO(yyval.Int_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 99:
#line 686 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Int_fifo = NULL;
			APPEND_G_FIFO(yyval.Int_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 108:
#line 705 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val_fifo = NULL; }
break;
case 109:
#line 707 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 110:
#line 715 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			if (input_from_file) {
				yyval.Attr_val = create_attr_sval(yystack.l_mark[-1].Integer, yystack.l_mark[0].String);
			} else {
				yyval.Attr_val = NULL;
				YYFREE(yystack.l_mark[0].String);
				yyerror("filegen file remote config ignored");
			}
		}
break;
case 111:
#line 725 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			if (input_from_file) {
				yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer);
			} else {
				yyval.Attr_val = NULL;
				yyerror("filegen type remote config ignored");
			}
		}
break;
case 112:
#line 734 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			const char *err;
			
			if (input_from_file) {
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
#line 749 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(T_Flag, yystack.l_mark[0].Integer); }
break;
case 125:
#line 779 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			CONCAT_G_FIFOS(cfgt.discard_opts, yystack.l_mark[0].Attr_val_fifo);
		}
break;
case 126:
#line 783 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			CONCAT_G_FIFOS(cfgt.mru_opts, yystack.l_mark[0].Attr_val_fifo);
		}
break;
case 127:
#line 787 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			restrict_node *rn;

			rn = create_restrict_node(yystack.l_mark[-1].Address_node, NULL, yystack.l_mark[0].Int_fifo,
						  ip_file->line_no);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
break;
case 128:
#line 795 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			restrict_node *rn;

			rn = create_restrict_node(yystack.l_mark[-3].Address_node, yystack.l_mark[-1].Address_node, yystack.l_mark[0].Int_fifo,
						  ip_file->line_no);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
break;
case 129:
#line 803 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			restrict_node *rn;

			rn = create_restrict_node(NULL, NULL, yystack.l_mark[0].Int_fifo,
						  ip_file->line_no);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
break;
case 130:
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
				yystack.l_mark[0].Int_fifo, 
				ip_file->line_no);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
break;
case 131:
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
				yystack.l_mark[0].Int_fifo, 
				ip_file->line_no);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
break;
case 132:
#line 841 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			restrict_node *	rn;

			APPEND_G_FIFO(yystack.l_mark[0].Int_fifo, create_int_node(yystack.l_mark[-1].Integer));
			rn = create_restrict_node(
				NULL, NULL, yystack.l_mark[0].Int_fifo, ip_file->line_no);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
break;
case 133:
#line 853 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Int_fifo = NULL; }
break;
case 134:
#line 855 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Int_fifo = yystack.l_mark[-1].Int_fifo;
			APPEND_G_FIFO(yyval.Int_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 150:
#line 881 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 151:
#line 886 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 152:
#line 894 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 156:
#line 905 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 157:
#line 910 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 158:
#line 918 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 167:
#line 938 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			addr_opts_node *aon;
			
			aon = create_addr_opts_node(yystack.l_mark[-1].Address_node, yystack.l_mark[0].Attr_val_fifo);
			APPEND_G_FIFO(cfgt.fudge, aon);
		}
break;
case 168:
#line 948 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 169:
#line 953 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 170:
#line 961 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-1].Integer, yystack.l_mark[0].Double); }
break;
case 171:
#line 963 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 172:
#line 965 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 173:
#line 967 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_sval(yystack.l_mark[-1].Integer, yystack.l_mark[0].String); }
break;
case 174:
#line 969 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_sval(yystack.l_mark[-1].Integer, yystack.l_mark[0].String); }
break;
case 181:
#line 990 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.rlimit, yystack.l_mark[0].Attr_val_fifo); }
break;
case 182:
#line 995 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 183:
#line 1000 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 184:
#line 1008 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 188:
#line 1024 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.enable_opts, yystack.l_mark[0].Attr_val_fifo); }
break;
case 189:
#line 1026 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.disable_opts, yystack.l_mark[0].Attr_val_fifo); }
break;
case 190:
#line 1031 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 191:
#line 1036 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 192:
#line 1044 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(T_Flag, yystack.l_mark[0].Integer); }
break;
case 193:
#line 1046 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ 
			if (input_from_file) {
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
case 202:
#line 1081 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.tinker, yystack.l_mark[0].Attr_val_fifo); }
break;
case 203:
#line 1086 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 204:
#line 1091 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 205:
#line 1099 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-1].Integer, yystack.l_mark[0].Double); }
break;
case 216:
#line 1122 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			attr_val *av;
			
			av = create_attr_dval(yystack.l_mark[-1].Integer, yystack.l_mark[0].Double);
			APPEND_G_FIFO(cfgt.vars, av);
		}
break;
case 217:
#line 1129 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			attr_val *av;
			
			av = create_attr_sval(yystack.l_mark[-1].Integer, yystack.l_mark[0].String);
			APPEND_G_FIFO(cfgt.vars, av);
		}
break;
case 218:
#line 1136 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			char error_text[64];
			attr_val *av;

			if (input_from_file) {
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
case 219:
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
				fp[curr_include_level + 1] = F_OPEN(FindConfig(yystack.l_mark[-1].String), "r");
				if (fp[curr_include_level + 1] == NULL) {
					fprintf(stderr, "getconfig: Couldn't open <%s>\n", FindConfig(yystack.l_mark[-1].String));
					msyslog(LOG_ERR, "getconfig: Couldn't open <%s>", FindConfig(yystack.l_mark[-1].String));
				} else {
					ip_file = fp[++curr_include_level];
				}
			}
		}
break;
case 220:
#line 1171 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			while (curr_include_level != -1)
				FCLOSE(fp[curr_include_level--]);
		}
break;
case 221:
#line 1176 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ /* see drift_parm below for actions */ }
break;
case 222:
#line 1178 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.logconfig, yystack.l_mark[0].Attr_val_fifo); }
break;
case 223:
#line 1180 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.phone, yystack.l_mark[0].String_fifo); }
break;
case 224:
#line 1182 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ APPEND_G_FIFO(cfgt.setvar, yystack.l_mark[0].Set_var); }
break;
case 225:
#line 1184 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			addr_opts_node *aon;
			
			aon = create_addr_opts_node(yystack.l_mark[-1].Address_node, yystack.l_mark[0].Attr_val_fifo);
			APPEND_G_FIFO(cfgt.trap, aon);
		}
break;
case 226:
#line 1191 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.ttl, yystack.l_mark[0].Attr_val_fifo); }
break;
case 235:
#line 1213 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			attr_val *av;
			
			av = create_attr_sval(T_Driftfile, yystack.l_mark[0].String);
			APPEND_G_FIFO(cfgt.vars, av);
		}
break;
case 236:
#line 1220 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			attr_val *av;
			
			av = create_attr_sval(T_Driftfile, yystack.l_mark[-1].String);
			APPEND_G_FIFO(cfgt.vars, av);
			av = create_attr_dval(T_WanderThreshold, yystack.l_mark[0].Double);
			APPEND_G_FIFO(cfgt.vars, av);
		}
break;
case 237:
#line 1229 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			attr_val *av;
			
			av = create_attr_sval(T_Driftfile, "");
			APPEND_G_FIFO(cfgt.vars, av);
		}
break;
case 238:
#line 1239 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Set_var = create_setvar_node(yystack.l_mark[-3].String, yystack.l_mark[-1].String, yystack.l_mark[0].Integer); }
break;
case 240:
#line 1245 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Integer = 0; }
break;
case 241:
#line 1250 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val_fifo = NULL; }
break;
case 242:
#line 1252 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 243:
#line 1260 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 244:
#line 1262 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val = create_attr_sval(yystack.l_mark[-1].Integer, estrdup(yystack.l_mark[0].Address_node->address));
			destroy_address_node(yystack.l_mark[0].Address_node);
		}
break;
case 245:
#line 1270 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 246:
#line 1275 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 247:
#line 1283 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
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
case 248:
#line 1308 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			nic_rule_node *nrn;
			
			nrn = create_nic_rule_node(yystack.l_mark[0].Integer, NULL, yystack.l_mark[-1].Integer);
			APPEND_G_FIFO(cfgt.nic_rules, nrn);
		}
break;
case 249:
#line 1315 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			nic_rule_node *nrn;
			
			nrn = create_nic_rule_node(0, yystack.l_mark[0].String, yystack.l_mark[-1].Integer);
			APPEND_G_FIFO(cfgt.nic_rules, nrn);
		}
break;
case 259:
#line 1343 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.reset_counters, yystack.l_mark[0].Int_fifo); }
break;
case 260:
#line 1348 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Int_fifo = yystack.l_mark[-1].Int_fifo;
			APPEND_G_FIFO(yyval.Int_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 261:
#line 1353 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Int_fifo = NULL;
			APPEND_G_FIFO(yyval.Int_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 269:
#line 1377 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 270:
#line 1382 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 271:
#line 1390 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 272:
#line 1395 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 273:
#line 1403 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival('i', yystack.l_mark[0].Integer); }
break;
case 275:
#line 1409 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_rangeval('-', yystack.l_mark[-3].Integer, yystack.l_mark[-1].Integer); }
break;
case 276:
#line 1414 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.String_fifo = yystack.l_mark[-1].String_fifo;
			APPEND_G_FIFO(yyval.String_fifo, create_string_node(yystack.l_mark[0].String));
		}
break;
case 277:
#line 1419 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.String_fifo = NULL;
			APPEND_G_FIFO(yyval.String_fifo, create_string_node(yystack.l_mark[0].String));
		}
break;
case 278:
#line 1427 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Address_fifo = yystack.l_mark[-1].Address_fifo;
			APPEND_G_FIFO(yyval.Address_fifo, yystack.l_mark[0].Address_node);
		}
break;
case 279:
#line 1432 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Address_fifo = NULL;
			APPEND_G_FIFO(yyval.Address_fifo, yystack.l_mark[0].Address_node);
		}
break;
case 280:
#line 1440 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			if (yystack.l_mark[0].Integer != 0 && yystack.l_mark[0].Integer != 1) {
				yyerror("Integer value is not boolean (0 or 1). Assuming 1");
				yyval.Integer = 1;
			} else {
				yyval.Integer = yystack.l_mark[0].Integer;
			}
		}
break;
case 281:
#line 1448 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Integer = 1; }
break;
case 282:
#line 1449 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Integer = 0; }
break;
case 283:
#line 1453 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Double = (double)yystack.l_mark[0].Integer; }
break;
case 285:
#line 1464 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			sim_node *sn;
			
			sn =  create_sim_node(yystack.l_mark[-2].Attr_val_fifo, yystack.l_mark[-1].Sim_server_fifo);
			APPEND_G_FIFO(cfgt.sim_details, sn);

			/* Revert from ; to \n for end-of-command */
			old_config_style = 1;
		}
break;
case 286:
#line 1481 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ old_config_style = 0; }
break;
case 287:
#line 1486 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-2].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[-1].Attr_val);
		}
break;
case 288:
#line 1491 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[-1].Attr_val);
		}
break;
case 289:
#line 1499 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-2].Integer, yystack.l_mark[0].Double); }
break;
case 292:
#line 1509 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Sim_server_fifo = yystack.l_mark[-1].Sim_server_fifo;
			APPEND_G_FIFO(yyval.Sim_server_fifo, yystack.l_mark[0].Sim_server);
		}
break;
case 293:
#line 1514 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Sim_server_fifo = NULL;
			APPEND_G_FIFO(yyval.Sim_server_fifo, yystack.l_mark[0].Sim_server);
		}
break;
case 294:
#line 1522 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Sim_server = create_sim_server(yystack.l_mark[-4].Address_node, yystack.l_mark[-2].Double, yystack.l_mark[-1].Sim_script_fifo); }
break;
case 295:
#line 1527 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Double = yystack.l_mark[-1].Double; }
break;
case 296:
#line 1532 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Address_node = yystack.l_mark[0].Address_node; }
break;
case 297:
#line 1537 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Sim_script_fifo = yystack.l_mark[-1].Sim_script_fifo;
			APPEND_G_FIFO(yyval.Sim_script_fifo, yystack.l_mark[0].Sim_script);
		}
break;
case 298:
#line 1542 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Sim_script_fifo = NULL;
			APPEND_G_FIFO(yyval.Sim_script_fifo, yystack.l_mark[0].Sim_script);
		}
break;
case 299:
#line 1550 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Sim_script = create_sim_script_info(yystack.l_mark[-3].Double, yystack.l_mark[-1].Attr_val_fifo); }
break;
case 300:
#line 1555 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-2].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[-1].Attr_val);
		}
break;
case 301:
#line 1560 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[-1].Attr_val);
		}
break;
case 302:
#line 1568 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-2].Integer, yystack.l_mark[0].Double); }
break;
#line 2259 "ntp_parser.c"
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
