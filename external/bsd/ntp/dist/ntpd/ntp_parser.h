/* A Bison parser, made by GNU Bison 2.6.2.  */

/* Bison interface for Yacc-like parsers in C
   
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
/* Line 2049 of yacc.c  */
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


/* Line 2049 of yacc.c  */
#line 266 "ntp_parser.h"
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
