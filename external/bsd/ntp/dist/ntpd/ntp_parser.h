/* A Bison parser, made by GNU Bison 2.4.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
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

/* Line 1685 of yacc.c  */
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



/* Line 1685 of yacc.c  */
#line 400 "ntp_parser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


