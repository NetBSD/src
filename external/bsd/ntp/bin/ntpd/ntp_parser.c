/* original parser id follows */
/* yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93" */
/* (use YYMAJOR/YYMINOR for ifdefs dependent on parser version) */

#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9

#define YYEMPTY        (-1)
#define yyclearin      (yychar = YYEMPTY)
#define yyerrok        (yyerrflag = 0)
#define YYRECOVERING() (yyerrflag != 0)
#define YYENOMEM       (-2)
#define YYEOF          0
#undef YYBTYACC
#define YYBTYACC 0
#define YYDEBUGSTR YYPREFIX "debug"
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
#line 53 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
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
#line 81 "ntp_parser.c"

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
#define T_Unconfig 428
#define T_Unpeer 429
#define T_Version 430
#define T_WanderThreshold 431
#define T_Week 432
#define T_Wildcard 433
#define T_Xleave 434
#define T_Year 435
#define T_Flag 436
#define T_EOC 437
#define T_Simulate 438
#define T_Beep_Delay 439
#define T_Sim_Duration 440
#define T_Server_Offset 441
#define T_Duration 442
#define T_Freq_Offset 443
#define T_Wander 444
#define T_Jitter 445
#define T_Prop_Delay 446
#define T_Proc_Delay 447
#define YYERRCODE 256
typedef int YYINT;
static const YYINT yylhs[] = {                           -1,
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
   61,   99,   66,   66,   65,   64,   64,   64,   64,   64,
   64,   64,   64,   64,   64,  100,  100,  100,  100,  100,
  100,  100,  100,  100,  100,  100,  100,  100,  100,   35,
   35,   35,   36,   36,   37,   37,   37,   38,   38,  102,
  102,  102,   74,   63,   63,   72,   72,   71,   71,   34,
   34,   33,   29,   29,   30,   30,   42,   42,   42,   42,
   28,   28,   28,   52,    9,    9,    8,    8,    8,    8,
    8,    8,    8,   24,   24,   25,   25,   26,   26,   27,
   58,   58,    5,    5,    6,    6,    6,   43,   43,  101,
  103,   76,   76,   75,   77,   77,   78,   78,   79,   80,
   81,   83,   83,   82,   85,   85,   86,   84,   84,   84,
   84,   84,
};
static const YYINT yylen[] = {                            2,
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
    1,    1,    1,    1,    1,    1,    1,    2,    2,    2,
    2,    3,    1,    2,    2,    2,    2,    3,    2,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    2,    0,    4,    1,    0,    0,    2,    2,    2,    2,
    1,    1,    3,    3,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    2,    2,    1,    1,    1,    1,    1,
    1,    1,    1,    2,    1,    2,    1,    1,    1,    5,
    2,    1,    2,    1,    1,    1,    1,    1,    1,    5,
    1,    3,    2,    3,    1,    1,    2,    1,    5,    4,
    3,    2,    1,    6,    3,    2,    3,    1,    1,    1,
    1,    1,
};
static const YYINT yydefred[] = {                         0,
    0,    0,   23,   57,  230,    0,   70,    0,    0,    0,
  233,    0,  223,    0,    0,  235,    0,  255,    0,    0,
  236,  234,    0,  238,   24,    0,    0,    0,    0,  256,
  231,    0,   22,    0,  237,   21,    0,    0,    0,    0,
    0,  239,   20,    0,    0,    0,  232,    0,    0,    0,
    0,    0,   55,   56,  291,    0,    0,  216,    0,    0,
    0,    0,    0,  217,    0,    0,    0,    6,    7,    8,
    9,   10,   11,   12,   13,   14,   15,   16,   17,   18,
    0,    4,   61,   62,    0,  194,  195,  196,  197,  200,
  198,  199,  201,  191,  192,  193,    0,  153,  154,  155,
  151,    0,    0,    0,  224,    0,  100,  101,  102,  103,
  107,  104,  105,  106,  108,   28,   29,   27,    0,    0,
   25,    0,   64,   65,  252,  251,    0,  284,    0,   60,
  159,  160,  161,  162,  163,  164,  165,  166,  157,    0,
    0,    0,   69,  282,    0,   66,  267,  268,  269,  270,
  271,  272,  273,  266,    0,  133,    0,    0,  133,  133,
    0,   67,  187,  185,  186,    0,  183,    0,    0,  227,
   99,    0,   96,  206,  207,  208,  209,  210,  211,  212,
  213,  214,  215,    0,  204,    0,   90,   85,    0,   86,
   94,   92,   93,   91,   89,   87,   88,   81,    0,    0,
    0,  246,  278,    0,    0,  277,  279,  275,    0,   30,
  263,  262,  261,    0,  289,  288,  218,  219,  220,  221,
   54,    0,    3,    0,   78,   74,   75,   76,   77,    0,
   71,    0,  190,  152,  150,  241,    0,    0,  177,  178,
  179,  180,    0,    0,  175,  176,  169,    0,    0,    0,
   26,  222,  250,  283,  158,  156,  281,  265,    0,  133,
  133,    0,    0,    0,  184,  182,    0,   98,  205,  203,
  287,  285,  286,   84,   83,   82,   80,    0,    0,  276,
  274,    0,  257,  258,  259,  254,  260,  253,    2,  295,
  296,    0,    0,    0,   73,   72,  117,  116,    0,  114,
  115,    0,  113,  109,  112,  173,  174,  172,  171,  170,
  168,  135,  136,  137,  138,  139,  140,  141,  142,  143,
  144,  145,  146,  147,  148,  149,  134,    0,    0,  133,
    0,    0,    0,  247,    0,   36,   37,   38,   53,   46,
   48,   47,   50,   39,   40,   41,   42,   49,   51,   43,
   31,   32,   35,   33,    0,   34,    0,  293,    0,    0,
    0,  298,    0,    0,  110,  124,  120,  122,  118,  119,
  121,  123,  111,    0,  244,  243,  249,  248,    0,   44,
   45,   52,    0,  292,  290,  297,    0,  294,  280,  301,
    0,    0,    0,    0,  303,    0,    0,    0,  299,  302,
  300,    0,    0,  308,  309,  310,  311,  312,    0,    0,
    0,    0,  304,    0,  306,  307,  305,
};
#if defined(YYDESTRUCT_CALL) || defined(YYSTYPE_TOSTRING)
static const YYINT yystos[] = {                           0,
  256,  264,  268,  269,  270,  276,  277,  283,  284,  287,
  289,  291,  292,  295,  304,  308,  314,  316,  325,  326,
  329,  330,  334,  335,  338,  339,  347,  360,  361,  362,
  367,  376,  380,  382,  384,  385,  394,  395,  396,  397,
  398,  399,  400,  401,  404,  406,  415,  420,  421,  422,
  424,  425,  428,  429,  438,  449,  456,  478,  479,  484,
  485,  486,  487,  501,  522,  536,  537,  538,  539,  540,
  541,  542,  543,  544,  545,  546,  547,  548,  549,  550,
  552,  437,  315,  315,  460,  262,  266,  272,  323,  357,
  358,  374,  405,  508,  509,  510,  511,  265,  353,  358,
  462,  463,  464,  412,  551,  511,  274,  278,  336,  381,
  389,  392,  414,  419,  505,  320,  322,  412,  452,  453,
  480,  412,  412,  412,  412,  482,  483,  452,  454,  315,
  310,  311,  312,  313,  341,  343,  345,  351,  488,  489,
  490,  454,  412,  412,  507,  315,  261,  262,  279,  318,
  348,  413,  418,  457,  458,  281,  320,  322,  402,  452,
  480,  315,  296,  349,  403,  502,  503,  504,  412,  523,
  505,  506,  412,  260,  285,  303,  306,  379,  407,  408,
  409,  410,  415,  513,  514,  515,  267,  273,  275,  302,
  342,  344,  350,  352,  355,  377,  378,  516,  517,  518,
  519,  480,  315,   40,  474,  475,  476,  315,  473,  452,
  288,  309,  333,  477,  286,  315,  492,  315,  412,  412,
  452,  537,  437,  123,  282,  305,  308,  390,  391,  397,
  459,  461,  508,  315,  462,  286,  467,  257,  297,  298,
  299,  300,  393,  411,  416,  417,  469,  470,  471,  472,
  412,  537,  482,  452,  315,  488,  412,  457,  451,  281,
  281,  451,  451,  340,  315,  503,   61,  505,  492,  514,
  293,  315,  423,  455,  492,  315,  516,  521,  315,  475,
  315,  496,  259,  319,  321,  412,  433,  491,  437,  439,
  440,  524,  525,  526,  315,  412,  283,  291,  294,  332,
  363,  426,  465,  466,  481,  412,  412,  315,  455,  492,
  469,  301,  309,  327,  328,  331,  337,  364,  365,  368,
  369,  371,  372,  373,  375,  430,  450,  451,  451,  480,
  412,  316,  386,  520,  290,  263,  271,  307,  308,  324,
  346,  354,  356,  370,  387,  388,  423,  425,  430,  434,
  493,  494,  495,  497,  498,  499,  500,  437,  400,  524,
  527,  528,  530,   61,  412,  258,  280,  359,  366,  383,
  432,  435,  468,  451,  281,  512,  480,  315,  315,  315,
  427,  412,   61,  437,  125,  528,  123,  492,   41,  452,
  441,  529,   61,  442,  531,  532,  492,   61,  125,  531,
  437,  492,  123,  443,  444,  445,  446,  447,  533,  534,
  535,   61,  125,  535,  437,  492,  437,
};
#endif /* YYDESTRUCT_CALL || YYSTYPE_TOSTRING */
static const YYINT yydgoto[] = {                         56,
  327,  259,  128,  120,  129,  274,   57,  154,  155,  231,
   85,  232,  101,  102,  103,  303,  304,  237,  373,  247,
  248,  249,  250,  209,  205,  206,  207,  214,   58,   59,
  121,  305,  126,  127,   60,   61,   62,   63,  139,  140,
  141,  288,  217,  351,  352,  353,  282,  354,  355,  356,
  357,   64,  166,  167,  168,  115,  172,  145,   94,   95,
   96,   97,  376,  184,  185,  186,  198,  199,  200,  201,
  334,  278,   65,  170,  292,  293,  294,  361,  362,  392,
  363,  395,  396,  409,  410,  411,   66,   67,   68,   69,
   70,   71,   72,   73,   74,   75,   76,   77,   78,   79,
   80,  105,   81,
};
static const YYINT yysindex[] = {                      -231,
 -422, -286,    0,    0,    0, -280,    0, -218, -238, -376,
    0, -218,    0,   53, -300,    0, -369,    0, -365, -361,
    0,    0, -347,    0,    0, -300, -249,  -53, -300,    0,
    0, -344,    0, -340,    0,    0, -241, -186, -265, -235,
 -279,    0,    0, -331,   53, -330,    0, -197,  153, -328,
  -35, -228,    0,    0,    0,    0, -300,    0, -247, -256,
 -215, -315, -311,    0, -300,  -21, -335,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  -10,    0,    0,    0, -106,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0, -218,    0,    0,    0,
    0, -194, -238, -164,    0, -218,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0, -208, -289,
    0,  -21,    0,    0,    0,    0, -347,    0, -300,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0, -187,
  -53, -300,    0,    0, -278,    0,    0,    0,    0,    0,
    0,    0,    0,    0, -186,    0, -148, -146,    0,    0,
 -199,    0,    0,    0,    0, -171,    0, -279,   85,    0,
    0,   53,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0, -256,    0, -197,    0,    0, -281,    0,
    0,    0,    0,    0,    0,    0,    0,    0, -256, -167,
  153,    0,    0, -165,  -35,    0,    0,    0, -163,    0,
    0,    0,    0, -252,    0,    0,    0,    0,    0,    0,
    0, -282,    0, -408,    0,    0,    0,    0,    0, -158,
    0, -254,    0,    0,    0,    0,  -30, -253,    0,    0,
    0,    0, -251, -144,    0,    0,    0, -281, -256, -208,
    0,    0,    0,    0,    0,    0,    0,    0,  121,    0,
    0,  121,  121, -328,    0,    0, -234,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0, -290, -118,    0,
    0,   36,    0,    0,    0,    0,    0,    0,    0,    0,
    0, -263, -322,  116,    0,    0,    0,    0, -233,    0,
    0, -240,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  121,  121,    0,
  -98, -328, -127,    0, -119,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0, -302,    0, -212,    0,  140, -232,
 -121,    0,   81, -256,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  121,    0,    0,    0,    0,  165,    0,
    0,    0, -300,    0,    0,    0, -227,    0,    0,    0,
  155, -225, -256,  158,    0, -117, -211, -256,    0,    0,
    0,  106, -222,    0,    0,    0,    0,    0,  169, -123,
 -206, -256,    0, -204,    0,    0,    0,
};
static const YYINT yyrindex[] = {                      -202,
    0,    0,    0,    0,    0,    0,    0,    0,    0, -201,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    1,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0, -200,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0, -198,    0,    0,    0,
    0,    0, -196, -195,    0, -193,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0, -202,    0,    0,    0,    0, -192,    0, -191,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
 -185, -183,    0,    0, -172,    0,    0,    0,    0,    0,
    0,    0,    0,    0, -170,    0, -174, -162,    0,    0,
  109,    0,    0,    0,    0,    0,    0, -168,    0,    0,
    0, -161,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0, -160,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
 -159,    0,    0,    0, -156,    0,    0,    0, -155,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0, -151,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0, -143,
    0,    0,    0,    0,    0,    0,    0,    0, -140,    0,
    0, -137, -136,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0, -134,    0,    0,
    0, -131,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0, -126, -125,    0,
 -122,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0, -109,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,
};
#if YYBTYACC
static const YYINT yycindex[] = {                         0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,
};
#endif
static const YYINT yygindex[] = {                         0,
    0, -150,  -15,    0,  211,    3,    0,  117,    0,    0,
    0,    0,  170,    0,    0,    0,    0,    0,    0,   25,
    0,    0,    0,    0,    0,   84,    0,    0,    0,    0,
  -36,    0,  183,    0,    0,    0,    0,    0,  175,    0,
    0,    0, -178,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  151,    0,  -34,    0,    0,  -78,    0,
    0,  317,    0,    0,  144,    0,  131,    0,    0,    0,
    0,    0,    0,    0,   41,    0,    0,    0,  -26,    0,
    0,  -60,    0,    0,    0,  -73,    0,  -43,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,
};
#define YYTABLESIZE 551
static const YYINT yytable[] = {                        119,
    1,  413,  161,  385,  204,  269,  283,  399,  262,  263,
  171,  271,  380,  202,   82,  156,  163,  366,  233,  116,
  275,  117,  222,  160,    1,  332,   98,  233,   83,  215,
  290,  291,    2,  272,   84,  104,    3,    4,    5,  367,
  211,  210,  122,   86,    6,    7,  123,   87,  238,  221,
  124,    8,    9,   88,  157,   10,  158,   11,  216,   12,
   13,  212,  174,   14,  125,  130,  284,  143,  285,  164,
  310,  144,   15,  146,  147,  148,   16,  359,  252,  162,
  169,  173,   17,  118,   18,  213,  208,  175,  239,  240,
  241,  242,  149,   19,   20,  333,  219,   21,   22,  218,
  220,  223,   23,   24,   89,  176,   25,   26,  177,  328,
  329,  118,  224,  254,   99,   27,  290,  291,  368,  100,
  234,  236,  251,  165,  381,  369,  254,  255,   28,   29,
   30,  150,  260,  257,  261,   31,  159,  268,   90,   91,
  264,  273,  370,  265,   32,  267,  118,  276,   33,  279,
   34,  281,   35,   36,  289,   92,  295,  296,  306,  286,
  307,  151,   37,   38,   39,   40,   41,   42,   43,   44,
  308,  335,   45,  358,   46,  225,  364,  331,  365,  374,
  287,  178,  375,   47,  243,  388,   93,  378,   48,   49,
   50,  371,   51,   52,  372,  379,   53,   54,  226,  382,
  383,  227,  244,  387,  384,  389,   55,  245,  246,  179,
  180,  181,  182,  391,  397,  393,  394,  183,  398,  402,
  404,  405,  406,  407,  408,  401,  152,  330,  403,  412,
  415,  153,  417,  416,    5,  242,   63,   28,  189,  142,
  125,  240,    2,  188,  225,   58,    3,    4,    5,   29,
  309,  126,  297,   59,    6,    7,  131,  132,  133,  134,
  298,    8,    9,  299,  226,   10,  264,   11,  181,   12,
   13,  258,  235,   14,  311,   95,  202,   79,  359,  203,
   68,  229,   15,  228,  229,   97,   16,  135,  280,  136,
  230,  137,   17,  167,   18,  377,  129,  138,  336,  132,
  127,  300,  228,   19,   20,   19,  337,   21,   22,  253,
  130,  131,   23,   24,  245,  256,   25,   26,  266,  404,
  405,  406,  407,  408,  394,   27,  107,  128,  106,  270,
  108,  277,  301,  360,  386,  400,  414,    0,   28,   29,
   30,    0,  338,  339,    0,   31,    0,    0,    0,    0,
    0,    0,    0,    0,   32,    0,    0,    0,   33,  340,
   34,    0,   35,   36,    0,    0,    0,  390,    0,    0,
    0,    0,   37,   38,   39,   40,   41,   42,   43,   44,
    0,  341,   45,    0,   46,    0,    0,    0,  109,  342,
    0,  343,    0,   47,    0,  302,    0,    0,   48,   49,
   50,    0,   51,   52,    0,  344,   53,   54,    0,   25,
    0,    0,    0,    0,    0,    0,   55,   25,    0,  187,
    0,  312,  345,  346,    0,  188,    0,  189,    0,  313,
    0,    0,    0,  110,    0,   25,   25,    5,    0,   25,
    0,  111,    0,    0,  112,   25,    0,  314,  315,    0,
    0,  316,    0,    0,  190,    0,    0,  317,  347,    0,
  348,    0,    0,    0,    0,  349,  113,    0,    0,  350,
    0,  114,   25,   25,    0,    0,   25,   25,    0,   25,
   25,   25,    0,   25,  318,  319,    0,    0,  320,  321,
    0,  322,  323,  324,  191,  325,  192,    0,    0,    0,
    0,    0,  193,    0,  194,    0,    0,  195,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  196,
  197,    0,    0,    0,    0,    0,    0,    0,   25,    0,
    0,    0,    0,    0,    0,   25,    0,    0,    0,    0,
  326,
};
static const YYINT yycheck[] = {                         15,
    0,  125,   39,  125,   40,  184,  259,  125,  159,  160,
   45,  293,  315,   50,  437,  281,  296,  258,   97,  320,
  199,  322,   66,   39,  256,  316,  265,  106,  315,  286,
  439,  440,  264,  315,  315,  412,  268,  269,  270,  280,
  288,   57,  412,  262,  276,  277,  412,  266,  257,   65,
  412,  283,  284,  272,  320,  287,  322,  289,  315,  291,
  292,  309,  260,  295,  412,  315,  319,  412,  321,  349,
  249,  412,  304,  315,  261,  262,  308,  400,  122,  315,
  412,  412,  314,  412,  316,  333,  315,  285,  297,  298,
  299,  300,  279,  325,  326,  386,  412,  329,  330,  315,
  412,  437,  334,  335,  323,  303,  338,  339,  306,  260,
  261,  412,  123,  129,  353,  347,  439,  440,  359,  358,
  315,  286,  412,  403,  427,  366,  142,  315,  360,  361,
  362,  318,  281,  412,  281,  367,  402,  172,  357,  358,
  340,  423,  383,  315,  376,   61,  412,  315,  380,  315,
  382,  315,  384,  385,  437,  374,  315,  412,  412,  412,
  412,  348,  394,  395,  396,  397,  398,  399,  400,  401,
  315,  290,  404,  437,  406,  282,   61,  412,  412,  330,
  433,  379,  281,  415,  393,  364,  405,  315,  420,  421,
  422,  432,  424,  425,  435,  315,  428,  429,  305,  412,
   61,  308,  411,  123,  437,   41,  438,  416,  417,  407,
  408,  409,  410,  441,  393,   61,  442,  415,   61,  398,
  443,  444,  445,  446,  447,  437,  413,  264,  123,   61,
  437,  418,  437,  412,  437,  437,  437,  412,  437,   29,
  437,  437,  264,  437,  437,  437,  268,  269,  270,  412,
  248,  437,  283,  437,  276,  277,  310,  311,  312,  313,
  291,  283,  284,  294,  437,  287,  437,  289,  437,  291,
  292,  155,  103,  295,  250,  437,  437,  437,  400,  315,
  437,  437,  304,  390,  391,  437,  308,  341,  205,  343,
  397,  345,  314,  437,  316,  332,  437,  351,  263,  437,
  437,  332,  437,  325,  326,  437,  271,  329,  330,  127,
  437,  437,  334,  335,  437,  141,  338,  339,  168,  443,
  444,  445,  446,  447,  442,  347,  274,  437,   12,  186,
  278,  201,  363,  293,  361,  396,  410,   -1,  360,  361,
  362,   -1,  307,  308,   -1,  367,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  376,   -1,   -1,   -1,  380,  324,
  382,   -1,  384,  385,   -1,   -1,   -1,  383,   -1,   -1,
   -1,   -1,  394,  395,  396,  397,  398,  399,  400,  401,
   -1,  346,  404,   -1,  406,   -1,   -1,   -1,  336,  354,
   -1,  356,   -1,  415,   -1,  426,   -1,   -1,  420,  421,
  422,   -1,  424,  425,   -1,  370,  428,  429,   -1,  301,
   -1,   -1,   -1,   -1,   -1,   -1,  438,  309,   -1,  267,
   -1,  301,  387,  388,   -1,  273,   -1,  275,   -1,  309,
   -1,   -1,   -1,  381,   -1,  327,  328,  437,   -1,  331,
   -1,  389,   -1,   -1,  392,  337,   -1,  327,  328,   -1,
   -1,  331,   -1,   -1,  302,   -1,   -1,  337,  423,   -1,
  425,   -1,   -1,   -1,   -1,  430,  414,   -1,   -1,  434,
   -1,  419,  364,  365,   -1,   -1,  368,  369,   -1,  371,
  372,  373,   -1,  375,  364,  365,   -1,   -1,  368,  369,
   -1,  371,  372,  373,  342,  375,  344,   -1,   -1,   -1,
   -1,   -1,  350,   -1,  352,   -1,   -1,  355,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  377,
  378,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  430,   -1,
   -1,   -1,   -1,   -1,   -1,  437,   -1,   -1,   -1,   -1,
  430,
};
#if YYBTYACC
static const YYINT yyctable[] = {                        -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
};
#endif
#define YYFINAL 56
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 447
#define YYUNDFTOKEN 553
#define YYTRANSLATE(a) ((a) > YYMAXTOKEN ? YYUNDFTOKEN : (a))
#undef yytname
#define yytname yyname
static const char *const yyname[] = {

"$end",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,"'('","')'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'='",0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,"'{'",0,"'}'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"error","T_Abbrev",
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
"T_Ttl","T_Type","T_U_int","T_Unconfig","T_Unpeer","T_Version",
"T_WanderThreshold","T_Week","T_Wildcard","T_Xleave","T_Year","T_Flag","T_EOC",
"T_Simulate","T_Beep_Delay","T_Sim_Duration","T_Server_Offset","T_Duration",
"T_Freq_Offset","T_Wander","T_Jitter","T_Prop_Delay","T_Proc_Delay","$accept",
"configuration","access_control_flag","ac_flag_list","address","address_fam",
"address_list","boolean","client_type","counter_set_keyword","counter_set_list",
"crypto_command","crypto_command_list","crypto_str_keyword","discard_option",
"discard_option_keyword","discard_option_list","enable_disable",
"filegen_option","filegen_option_list","filegen_type","fudge_factor",
"fudge_factor_bool_keyword","fudge_factor_dbl_keyword","fudge_factor_list",
"integer_list","integer_list_range","integer_list_range_elt","integer_range",
"nic_rule_action","interface_command","interface_nic","ip_address",
"link_nolink","log_config_command","log_config_list","misc_cmd_dbl_keyword",
"misc_cmd_int_keyword","misc_cmd_str_keyword","misc_cmd_str_lcl_keyword",
"mru_option","mru_option_keyword","mru_option_list","nic_rule_class","number",
"option","option_flag","option_flag_keyword","option_list","option_int",
"option_int_keyword","option_str","option_str_keyword","reset_command",
"rlimit_option_keyword","rlimit_option","rlimit_option_list","stat",
"stats_list","string_list","system_option","system_option_flag_keyword",
"system_option_local_flag_keyword","system_option_list","t_default_or_zero",
"tinker_option_keyword","tinker_option","tinker_option_list","tos_option",
"tos_option_dbl_keyword","tos_option_int_keyword","tos_option_list",
"trap_option","trap_option_list","unpeer_keyword","variable_assign",
"sim_init_statement","sim_init_statement_list","sim_init_keyword",
"sim_server_list","sim_server","sim_server_offset","sim_server_name","sim_act",
"sim_act_list","sim_act_keyword","sim_act_stmt_list","sim_act_stmt",
"command_list","command","server_command","unpeer_command","other_mode_command",
"authentication_command","monitoring_command","access_control_command",
"orphan_mode_command","fudge_command","rlimit_command","system_option_command",
"tinker_command","miscellaneous_command","simulate_command","drift_parm",
"sim_conf_start","illegal-symbol",
};
#if YYDEBUG
static const char *const yyrule[] = {
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
#define YYSTACKSIZE 10000
#define YYMAXDEPTH  10000
#endif
#endif

#define YYINITSTACKSIZE 200

typedef struct {
    unsigned stacksize;
    YYINT    *s_base;
    YYINT    *s_mark;
    YYINT    *s_last;
    YYSTYPE  *l_base;
    YYSTYPE  *l_mark;
} YYSTACKDATA;
/* variables for the parser stack */
static YYSTACKDATA yystack;
#line 1607 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"

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

#line 1287 "ntp_parser.c"

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
    YYINT *newss;
    YYSTYPE *newvs;

    if ((newsize = data->stacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return YYENOMEM;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;

    i = (int) (data->s_mark - data->s_base);
    newss = (YYINT *)realloc(data->s_base, newsize * sizeof(*newss));
    if (newss == 0)
        return YYENOMEM;

    data->s_base = newss;
    data->s_mark = newss + i;

    newvs = (YYSTYPE *)realloc(data->l_base, newsize * sizeof(*newvs));
    if (newvs == 0)
        return YYENOMEM;

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

    if (yystack.s_base == NULL && yygrowstack(&yystack) == YYENOMEM) goto yyoverflow;
    yystack.s_mark = yystack.s_base;
    yystack.l_mark = yystack.l_base;
    yystate = 0;
    *yystack.s_mark = 0;

yyloop:
    if ((yyn = yydefred[yystate]) != 0) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = YYLEX) < 0) yychar = YYEOF;
#if YYDEBUG
        if (yydebug)
        {
            yys = yyname[YYTRANSLATE(yychar)];
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
        if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM)
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

    YYERROR_CALL("syntax error");

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
                if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM)
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
        if (yychar == YYEOF) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = yyname[YYTRANSLATE(yychar)];
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
#line 375 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
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
#line 411 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			peer_node *my_node;

			my_node = create_peer_node(yystack.l_mark[-2].Integer, yystack.l_mark[-1].Address_node, yystack.l_mark[0].Attr_val_fifo);
			APPEND_G_FIFO(cfgt.peers, my_node);
		}
break;
case 26:
#line 430 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Address_node = create_address_node(yystack.l_mark[0].String, yystack.l_mark[-1].Integer); }
break;
case 27:
#line 435 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Address_node = create_address_node(yystack.l_mark[0].String, AF_UNSPEC); }
break;
case 28:
#line 440 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Integer = AF_INET; }
break;
case 29:
#line 442 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Integer = AF_INET6; }
break;
case 30:
#line 447 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val_fifo = NULL; }
break;
case 31:
#line 449 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 35:
#line 463 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(T_Flag, yystack.l_mark[0].Integer); }
break;
case 44:
#line 479 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 45:
#line 481 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_uval(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 52:
#line 495 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_sval(yystack.l_mark[-1].Integer, yystack.l_mark[0].String); }
break;
case 54:
#line 509 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			unpeer_node *my_node;

			my_node = create_unpeer_node(yystack.l_mark[0].Address_node);
			if (my_node)
				APPEND_G_FIFO(cfgt.unpeers, my_node);
		}
break;
case 57:
#line 530 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ cfgt.broadcastclient = 1; }
break;
case 58:
#line 532 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.manycastserver, yystack.l_mark[0].Address_fifo); }
break;
case 59:
#line 534 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.multicastclient, yystack.l_mark[0].Address_fifo); }
break;
case 60:
#line 536 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ cfgt.mdnstries = yystack.l_mark[0].Integer; }
break;
case 61:
#line 547 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			attr_val *atrv;

			atrv = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer);
			APPEND_G_FIFO(cfgt.vars, atrv);
		}
break;
case 62:
#line 554 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ cfgt.auth.control_key = yystack.l_mark[0].Integer; }
break;
case 63:
#line 556 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			cfgt.auth.cryptosw++;
			CONCAT_G_FIFOS(cfgt.auth.crypto_cmd_list, yystack.l_mark[0].Attr_val_fifo);
		}
break;
case 64:
#line 561 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ cfgt.auth.keys = yystack.l_mark[0].String; }
break;
case 65:
#line 563 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ cfgt.auth.keysdir = yystack.l_mark[0].String; }
break;
case 66:
#line 565 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ cfgt.auth.request_key = yystack.l_mark[0].Integer; }
break;
case 67:
#line 567 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ cfgt.auth.revoke = yystack.l_mark[0].Integer; }
break;
case 68:
#line 569 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			cfgt.auth.trusted_key_list = yystack.l_mark[0].Attr_val_fifo;

			/* if (!cfgt.auth.trusted_key_list)*/
			/* 	cfgt.auth.trusted_key_list = $2;*/
			/* else*/
			/* 	LINK_SLIST(cfgt.auth.trusted_key_list, $2, link);*/
		}
break;
case 69:
#line 578 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ cfgt.auth.ntp_signd_socket = yystack.l_mark[0].String; }
break;
case 70:
#line 583 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val_fifo = NULL; }
break;
case 71:
#line 585 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 72:
#line 593 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_sval(yystack.l_mark[-1].Integer, yystack.l_mark[0].String); }
break;
case 73:
#line 595 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
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
#line 620 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.orphan_cmds, yystack.l_mark[0].Attr_val_fifo); }
break;
case 80:
#line 625 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 81:
#line 630 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 82:
#line 638 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-1].Integer, (double)yystack.l_mark[0].Integer); }
break;
case 83:
#line 640 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-1].Integer, yystack.l_mark[0].Double); }
break;
case 84:
#line 642 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-1].Integer, (double)yystack.l_mark[0].Integer); }
break;
case 95:
#line 668 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.stats_list, yystack.l_mark[0].Int_fifo); }
break;
case 96:
#line 670 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
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
#line 679 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			filegen_node *fgn;

			fgn = create_filegen_node(yystack.l_mark[-1].Integer, yystack.l_mark[0].Attr_val_fifo);
			APPEND_G_FIFO(cfgt.filegen_opts, fgn);
		}
break;
case 98:
#line 689 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Int_fifo = yystack.l_mark[-1].Int_fifo;
			APPEND_G_FIFO(yyval.Int_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 99:
#line 694 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Int_fifo = NULL;
			APPEND_G_FIFO(yyval.Int_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 108:
#line 713 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val_fifo = NULL; }
break;
case 109:
#line 715 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 110:
#line 723 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
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
#line 733 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
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
#line 742 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
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
#line 757 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(T_Flag, yystack.l_mark[0].Integer); }
break;
case 125:
#line 787 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			CONCAT_G_FIFOS(cfgt.discard_opts, yystack.l_mark[0].Attr_val_fifo);
		}
break;
case 126:
#line 791 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			CONCAT_G_FIFOS(cfgt.mru_opts, yystack.l_mark[0].Attr_val_fifo);
		}
break;
case 127:
#line 795 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			restrict_node *rn;

			rn = create_restrict_node(yystack.l_mark[-1].Address_node, NULL, yystack.l_mark[0].Int_fifo,
						  lex_current()->curpos.nline);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
break;
case 128:
#line 803 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			restrict_node *rn;

			rn = create_restrict_node(yystack.l_mark[-3].Address_node, yystack.l_mark[-1].Address_node, yystack.l_mark[0].Int_fifo,
						  lex_current()->curpos.nline);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
break;
case 129:
#line 811 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			restrict_node *rn;

			rn = create_restrict_node(NULL, NULL, yystack.l_mark[0].Int_fifo,
						  lex_current()->curpos.nline);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
break;
case 130:
#line 819 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
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
#line 834 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
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
#line 849 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			restrict_node *	rn;

			APPEND_G_FIFO(yystack.l_mark[0].Int_fifo, create_int_node(yystack.l_mark[-1].Integer));
			rn = create_restrict_node(
				NULL, NULL, yystack.l_mark[0].Int_fifo, lex_current()->curpos.nline);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
break;
case 133:
#line 861 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Int_fifo = NULL; }
break;
case 134:
#line 863 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Int_fifo = yystack.l_mark[-1].Int_fifo;
			APPEND_G_FIFO(yyval.Int_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 150:
#line 889 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 151:
#line 894 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 152:
#line 902 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 156:
#line 913 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 157:
#line 918 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 158:
#line 926 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 167:
#line 946 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			addr_opts_node *aon;

			aon = create_addr_opts_node(yystack.l_mark[-1].Address_node, yystack.l_mark[0].Attr_val_fifo);
			APPEND_G_FIFO(cfgt.fudge, aon);
		}
break;
case 168:
#line 956 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 169:
#line 961 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 170:
#line 969 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-1].Integer, yystack.l_mark[0].Double); }
break;
case 171:
#line 971 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 172:
#line 973 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 173:
#line 975 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_sval(yystack.l_mark[-1].Integer, yystack.l_mark[0].String); }
break;
case 174:
#line 977 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_sval(yystack.l_mark[-1].Integer, yystack.l_mark[0].String); }
break;
case 181:
#line 998 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.rlimit, yystack.l_mark[0].Attr_val_fifo); }
break;
case 182:
#line 1003 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 183:
#line 1008 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 184:
#line 1016 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 188:
#line 1032 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.enable_opts, yystack.l_mark[0].Attr_val_fifo); }
break;
case 189:
#line 1034 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.disable_opts, yystack.l_mark[0].Attr_val_fifo); }
break;
case 190:
#line 1039 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 191:
#line 1044 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 192:
#line 1052 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(T_Flag, yystack.l_mark[0].Integer); }
break;
case 193:
#line 1054 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
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
case 202:
#line 1089 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.tinker, yystack.l_mark[0].Attr_val_fifo); }
break;
case 203:
#line 1094 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 204:
#line 1099 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 205:
#line 1107 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-1].Integer, yystack.l_mark[0].Double); }
break;
case 218:
#line 1132 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			attr_val *av;

			av = create_attr_dval(yystack.l_mark[-1].Integer, yystack.l_mark[0].Double);
			APPEND_G_FIFO(cfgt.vars, av);
		}
break;
case 219:
#line 1139 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			attr_val *av;

			av = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer);
			APPEND_G_FIFO(cfgt.vars, av);
		}
break;
case 220:
#line 1146 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			attr_val *av;

			av = create_attr_sval(yystack.l_mark[-1].Integer, yystack.l_mark[0].String);
			APPEND_G_FIFO(cfgt.vars, av);
		}
break;
case 221:
#line 1153 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
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
case 222:
#line 1169 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
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
case 223:
#line 1188 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ lex_flush_stack(); }
break;
case 224:
#line 1190 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ /* see drift_parm below for actions */ }
break;
case 225:
#line 1192 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.logconfig, yystack.l_mark[0].Attr_val_fifo); }
break;
case 226:
#line 1194 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.phone, yystack.l_mark[0].String_fifo); }
break;
case 227:
#line 1196 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ APPEND_G_FIFO(cfgt.setvar, yystack.l_mark[0].Set_var); }
break;
case 228:
#line 1198 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			addr_opts_node *aon;

			aon = create_addr_opts_node(yystack.l_mark[-1].Address_node, yystack.l_mark[0].Attr_val_fifo);
			APPEND_G_FIFO(cfgt.trap, aon);
		}
break;
case 229:
#line 1205 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.ttl, yystack.l_mark[0].Attr_val_fifo); }
break;
case 234:
#line 1220 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
#ifndef LEAP_SMEAR
			yyerror("Built without LEAP_SMEAR support.");
#endif
		}
break;
case 240:
#line 1240 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			attr_val *av;

			av = create_attr_sval(T_Driftfile, yystack.l_mark[0].String);
			APPEND_G_FIFO(cfgt.vars, av);
		}
break;
case 241:
#line 1247 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			attr_val *av;

			av = create_attr_sval(T_Driftfile, yystack.l_mark[-1].String);
			APPEND_G_FIFO(cfgt.vars, av);
			av = create_attr_dval(T_WanderThreshold, yystack.l_mark[0].Double);
			APPEND_G_FIFO(cfgt.vars, av);
		}
break;
case 242:
#line 1256 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			attr_val *av;

			av = create_attr_sval(T_Driftfile, "");
			APPEND_G_FIFO(cfgt.vars, av);
		}
break;
case 243:
#line 1266 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Set_var = create_setvar_node(yystack.l_mark[-3].String, yystack.l_mark[-1].String, yystack.l_mark[0].Integer); }
break;
case 245:
#line 1272 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Integer = 0; }
break;
case 246:
#line 1277 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val_fifo = NULL; }
break;
case 247:
#line 1279 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 248:
#line 1287 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival(yystack.l_mark[-1].Integer, yystack.l_mark[0].Integer); }
break;
case 249:
#line 1289 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val = create_attr_sval(yystack.l_mark[-1].Integer, estrdup(yystack.l_mark[0].Address_node->address));
			destroy_address_node(yystack.l_mark[0].Address_node);
		}
break;
case 250:
#line 1297 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 251:
#line 1302 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 252:
#line 1310 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
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
case 253:
#line 1335 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			nic_rule_node *nrn;

			nrn = create_nic_rule_node(yystack.l_mark[0].Integer, NULL, yystack.l_mark[-1].Integer);
			APPEND_G_FIFO(cfgt.nic_rules, nrn);
		}
break;
case 254:
#line 1342 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			nic_rule_node *nrn;

			nrn = create_nic_rule_node(0, yystack.l_mark[0].String, yystack.l_mark[-1].Integer);
			APPEND_G_FIFO(cfgt.nic_rules, nrn);
		}
break;
case 264:
#line 1370 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ CONCAT_G_FIFOS(cfgt.reset_counters, yystack.l_mark[0].Int_fifo); }
break;
case 265:
#line 1375 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Int_fifo = yystack.l_mark[-1].Int_fifo;
			APPEND_G_FIFO(yyval.Int_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 266:
#line 1380 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Int_fifo = NULL;
			APPEND_G_FIFO(yyval.Int_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 274:
#line 1404 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 275:
#line 1409 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, create_int_node(yystack.l_mark[0].Integer));
		}
break;
case 276:
#line 1417 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-1].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 277:
#line 1422 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[0].Attr_val);
		}
break;
case 278:
#line 1430 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_ival('i', yystack.l_mark[0].Integer); }
break;
case 280:
#line 1436 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_rangeval('-', yystack.l_mark[-3].Integer, yystack.l_mark[-1].Integer); }
break;
case 281:
#line 1441 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.String_fifo = yystack.l_mark[-1].String_fifo;
			APPEND_G_FIFO(yyval.String_fifo, create_string_node(yystack.l_mark[0].String));
		}
break;
case 282:
#line 1446 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.String_fifo = NULL;
			APPEND_G_FIFO(yyval.String_fifo, create_string_node(yystack.l_mark[0].String));
		}
break;
case 283:
#line 1454 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Address_fifo = yystack.l_mark[-1].Address_fifo;
			APPEND_G_FIFO(yyval.Address_fifo, yystack.l_mark[0].Address_node);
		}
break;
case 284:
#line 1459 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Address_fifo = NULL;
			APPEND_G_FIFO(yyval.Address_fifo, yystack.l_mark[0].Address_node);
		}
break;
case 285:
#line 1467 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			if (yystack.l_mark[0].Integer != 0 && yystack.l_mark[0].Integer != 1) {
				yyerror("Integer value is not boolean (0 or 1). Assuming 1");
				yyval.Integer = 1;
			} else {
				yyval.Integer = yystack.l_mark[0].Integer;
			}
		}
break;
case 286:
#line 1475 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Integer = 1; }
break;
case 287:
#line 1476 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Integer = 0; }
break;
case 288:
#line 1480 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Double = (double)yystack.l_mark[0].Integer; }
break;
case 290:
#line 1491 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			sim_node *sn;

			sn =  create_sim_node(yystack.l_mark[-2].Attr_val_fifo, yystack.l_mark[-1].Sim_server_fifo);
			APPEND_G_FIFO(cfgt.sim_details, sn);

			/* Revert from ; to \n for end-of-command */
			old_config_style = 1;
		}
break;
case 291:
#line 1508 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ old_config_style = 0; }
break;
case 292:
#line 1513 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-2].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[-1].Attr_val);
		}
break;
case 293:
#line 1518 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[-1].Attr_val);
		}
break;
case 294:
#line 1526 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-2].Integer, yystack.l_mark[0].Double); }
break;
case 297:
#line 1536 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Sim_server_fifo = yystack.l_mark[-1].Sim_server_fifo;
			APPEND_G_FIFO(yyval.Sim_server_fifo, yystack.l_mark[0].Sim_server);
		}
break;
case 298:
#line 1541 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Sim_server_fifo = NULL;
			APPEND_G_FIFO(yyval.Sim_server_fifo, yystack.l_mark[0].Sim_server);
		}
break;
case 299:
#line 1549 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Sim_server = ONLY_SIM(create_sim_server(yystack.l_mark[-4].Address_node, yystack.l_mark[-2].Double, yystack.l_mark[-1].Sim_script_fifo)); }
break;
case 300:
#line 1554 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Double = yystack.l_mark[-1].Double; }
break;
case 301:
#line 1559 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Address_node = yystack.l_mark[0].Address_node; }
break;
case 302:
#line 1564 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Sim_script_fifo = yystack.l_mark[-1].Sim_script_fifo;
			APPEND_G_FIFO(yyval.Sim_script_fifo, yystack.l_mark[0].Sim_script);
		}
break;
case 303:
#line 1569 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Sim_script_fifo = NULL;
			APPEND_G_FIFO(yyval.Sim_script_fifo, yystack.l_mark[0].Sim_script);
		}
break;
case 304:
#line 1577 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Sim_script = ONLY_SIM(create_sim_script_info(yystack.l_mark[-3].Double, yystack.l_mark[-1].Attr_val_fifo)); }
break;
case 305:
#line 1582 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = yystack.l_mark[-2].Attr_val_fifo;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[-1].Attr_val);
		}
break;
case 306:
#line 1587 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{
			yyval.Attr_val_fifo = NULL;
			APPEND_G_FIFO(yyval.Attr_val_fifo, yystack.l_mark[-1].Attr_val);
		}
break;
case 307:
#line 1595 "/net/quasar/src-5/NetBSD/src/external/bsd/ntp/dist/ntpd/ntp_parser.y"
	{ yyval.Attr_val = create_attr_dval(yystack.l_mark[-2].Integer, yystack.l_mark[0].Double); }
break;
#line 2468 "ntp_parser.c"
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
            if ((yychar = YYLEX) < 0) yychar = YYEOF;
#if YYDEBUG
            if (yydebug)
            {
                yys = yyname[YYTRANSLATE(yychar)];
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == YYEOF) goto yyaccept;
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
    if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM)
    {
        goto yyoverflow;
    }
    *++yystack.s_mark = (YYINT) yystate;
    *++yystack.l_mark = yyval;
    goto yyloop;

yyoverflow:
    YYERROR_CALL("yacc stack overflow");

yyabort:
    yyfreestack(&yystack);
    return (1);

yyaccept:
    yyfreestack(&yystack);
    return (0);
}
