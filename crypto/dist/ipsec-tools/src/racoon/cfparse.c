/*	$NetBSD: cfparse.c,v 1.2.2.2 2005/11/21 21:12:30 tron Exp $	*/


/*  A Bison parser, made from ./cfparse.y
    by GNU Bison version 1.28  */

#define YYBISON 1  /* Identify Bison output.  */

#define	PRIVSEP	257
#define	USER	258
#define	GROUP	259
#define	CHROOT	260
#define	PATH	261
#define	PATHTYPE	262
#define	INCLUDE	263
#define	IDENTIFIER	264
#define	VENDORID	265
#define	LOGGING	266
#define	LOGLEV	267
#define	PADDING	268
#define	PAD_RANDOMIZE	269
#define	PAD_RANDOMIZELEN	270
#define	PAD_MAXLEN	271
#define	PAD_STRICT	272
#define	PAD_EXCLTAIL	273
#define	LISTEN	274
#define	X_ISAKMP	275
#define	X_ISAKMP_NATT	276
#define	X_ADMIN	277
#define	STRICT_ADDRESS	278
#define	ADMINSOCK	279
#define	DISABLED	280
#define	MODECFG	281
#define	CFG_NET4	282
#define	CFG_MASK4	283
#define	CFG_DNS4	284
#define	CFG_NBNS4	285
#define	CFG_AUTH_SOURCE	286
#define	CFG_SYSTEM	287
#define	CFG_RADIUS	288
#define	CFG_PAM	289
#define	CFG_LOCAL	290
#define	CFG_NONE	291
#define	CFG_ACCOUNTING	292
#define	CFG_CONF_SOURCE	293
#define	CFG_MOTD	294
#define	CFG_POOL_SIZE	295
#define	CFG_AUTH_THROTTLE	296
#define	CFG_PFS_GROUP	297
#define	CFG_SAVE_PASSWD	298
#define	RETRY	299
#define	RETRY_COUNTER	300
#define	RETRY_INTERVAL	301
#define	RETRY_PERSEND	302
#define	RETRY_PHASE1	303
#define	RETRY_PHASE2	304
#define	NATT_KA	305
#define	ALGORITHM_CLASS	306
#define	ALGORITHMTYPE	307
#define	STRENGTHTYPE	308
#define	SAINFO	309
#define	FROM	310
#define	REMOTE	311
#define	ANONYMOUS	312
#define	INHERIT	313
#define	EXCHANGE_MODE	314
#define	EXCHANGETYPE	315
#define	DOI	316
#define	DOITYPE	317
#define	SITUATION	318
#define	SITUATIONTYPE	319
#define	CERTIFICATE_TYPE	320
#define	CERTTYPE	321
#define	PEERS_CERTFILE	322
#define	CA_TYPE	323
#define	VERIFY_CERT	324
#define	SEND_CERT	325
#define	SEND_CR	326
#define	IDENTIFIERTYPE	327
#define	MY_IDENTIFIER	328
#define	PEERS_IDENTIFIER	329
#define	VERIFY_IDENTIFIER	330
#define	DNSSEC	331
#define	CERT_X509	332
#define	CERT_PLAINRSA	333
#define	NONCE_SIZE	334
#define	DH_GROUP	335
#define	KEEPALIVE	336
#define	PASSIVE	337
#define	INITIAL_CONTACT	338
#define	NAT_TRAVERSAL	339
#define	NAT_TRAVERSAL_LEVEL	340
#define	PROPOSAL_CHECK	341
#define	PROPOSAL_CHECK_LEVEL	342
#define	GENERATE_POLICY	343
#define	SUPPORT_PROXY	344
#define	PROPOSAL	345
#define	EXEC_PATH	346
#define	EXEC_COMMAND	347
#define	EXEC_SUCCESS	348
#define	EXEC_FAILURE	349
#define	GSS_ID	350
#define	GSS_ID_ENC	351
#define	GSS_ID_ENCTYPE	352
#define	COMPLEX_BUNDLE	353
#define	DPD	354
#define	DPD_DELAY	355
#define	DPD_RETRY	356
#define	DPD_MAXFAIL	357
#define	XAUTH_LOGIN	358
#define	PREFIX	359
#define	PORT	360
#define	PORTANY	361
#define	UL_PROTO	362
#define	ANY	363
#define	IKE_FRAG	364
#define	ESP_FRAG	365
#define	MODE_CFG	366
#define	PFS_GROUP	367
#define	LIFETIME	368
#define	LIFETYPE_TIME	369
#define	LIFETYPE_BYTE	370
#define	STRENGTH	371
#define	SCRIPT	372
#define	PHASE1_UP	373
#define	PHASE1_DOWN	374
#define	NUMBER	375
#define	SWITCH	376
#define	BOOLEAN	377
#define	HEXSTRING	378
#define	QUOTEDSTRING	379
#define	ADDRSTRING	380
#define	UNITTYPE_BYTE	381
#define	UNITTYPE_KBYTES	382
#define	UNITTYPE_MBYTES	383
#define	UNITTYPE_TBYTES	384
#define	UNITTYPE_SEC	385
#define	UNITTYPE_MIN	386
#define	UNITTYPE_HOUR	387
#define	EOS	388
#define	BOC	389
#define	EOC	390
#define	COMMA	391

#line 3 "./cfparse.y"

/*
 * Copyright (C) 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002 and 2003 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/socket.h>

#include <netinet/in.h>
#ifdef HAVE_NETINET6_IPSEC
#  include <netinet6/ipsec.h>
#else
#  include <netinet/ipsec.h>
#endif

#ifdef ENABLE_HYBRID
#include <arpa/inet.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <pwd.h>
#include <grp.h>

#include "var.h"
#include "misc.h"
#include "vmbuf.h"
#include "plog.h"
#include "sockmisc.h"
#include "str2val.h"
#include "genlist.h"
#include "debug.h"

#include "admin.h"
#include "privsep.h"
#include "cfparse_proto.h"
#include "cftoken_proto.h"
#include "algorithm.h"
#include "localconf.h"
#include "policy.h"
#include "sainfo.h"
#include "oakley.h"
#include "pfkey.h"
#include "remoteconf.h"
#include "grabmyaddr.h"
#include "isakmp_var.h"
#include "handler.h"
#include "isakmp.h"
#ifdef ENABLE_HYBRID
#include "isakmp_xauth.h"
#include "isakmp_cfg.h"
#endif
#include "ipsec_doi.h"
#include "strnames.h"
#include "gcmalloc.h"
#ifdef HAVE_GSSAPI
#include "gssapi.h"
#endif
#include "vendorid.h"
#include "rsalist.h"

struct proposalspec {
	time_t lifetime;		/* for isakmp/ipsec */
	int lifebyte;			/* for isakmp/ipsec */
	struct secprotospec *spspec;	/* the head is always current spec. */
	struct proposalspec *next;	/* the tail is the most prefered. */
	struct proposalspec *prev;
};

struct secprotospec {
	int prop_no;
	int trns_no;
	int strength;		/* for isakmp/ipsec */
	int encklen;		/* for isakmp/ipsec */
	time_t lifetime;	/* for isakmp */
	int lifebyte;		/* for isakmp */
	int proto_id;		/* for ipsec (isakmp?) */
	int ipsec_level;	/* for ipsec */
	int encmode;		/* for ipsec */
	int vendorid;		/* for isakmp */
	char *gssid;
	struct sockaddr *remote;
	int algclass[MAXALGCLASS];

	struct secprotospec *next;	/* the tail is the most prefiered. */
	struct secprotospec *prev;
	struct proposalspec *back;
};

static int num2dhgroup[] = {
	0,
	OAKLEY_ATTR_GRP_DESC_MODP768,
	OAKLEY_ATTR_GRP_DESC_MODP1024,
	OAKLEY_ATTR_GRP_DESC_EC2N155,
	OAKLEY_ATTR_GRP_DESC_EC2N185,
	OAKLEY_ATTR_GRP_DESC_MODP1536,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	OAKLEY_ATTR_GRP_DESC_MODP2048,
	OAKLEY_ATTR_GRP_DESC_MODP3072,
	OAKLEY_ATTR_GRP_DESC_MODP4096,
	OAKLEY_ATTR_GRP_DESC_MODP6144,
	OAKLEY_ATTR_GRP_DESC_MODP8192
};

static struct remoteconf *cur_rmconf;
static int tmpalgtype[MAXALGCLASS];
static struct sainfo *cur_sainfo;
static int cur_algclass;

static struct proposalspec *newprspec __P((void));
static void insprspec __P((struct proposalspec *, struct proposalspec **));
static struct secprotospec *newspspec __P((void));
static void insspspec __P((struct secprotospec *, struct proposalspec **));
static void adminsock_conf __P((vchar_t *, vchar_t *, vchar_t *, int));

static int set_isakmp_proposal
	__P((struct remoteconf *, struct proposalspec *));
static void clean_tmpalgtype __P((void));
static int expand_isakmpspec __P((int, int, int *,
	int, int, time_t, int, int, int, char *, struct remoteconf *));
static int listen_addr __P((struct sockaddr *addr, int udp_encap));

void freeetypes (struct etypes **etypes);

#if 0
static int fix_lifebyte __P((u_long));
#endif

#line 171 "./cfparse.y"
typedef union {
	unsigned long num;
	vchar_t *val;
	struct remoteconf *rmconf;
	struct sockaddr *saddr;
	struct sainfoalg *alg;
} YYSTYPE;
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		509
#define	YYFLAG		-32768
#define	YYNTBASE	138

#define YYTRANSLATE(x) ((unsigned)(x) <= 391 ? yytranslate[x] : 289)

static const short yytranslate[] = {     0,
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
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     3,     4,     5,     6,
     7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
    17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
    27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
    37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
    47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
    57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
    67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
    77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
    87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
    97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
   107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
   117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
   127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
   137
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     1,     4,     6,     8,    10,    12,    14,    16,    18,
    20,    22,    24,    26,    28,    30,    35,    36,    39,    40,
    45,    46,    51,    52,    57,    58,    63,    64,    69,    70,
    76,    77,    82,    86,    90,    93,    94,    99,   100,   105,
   109,   111,   113,   118,   119,   122,   123,   128,   129,   134,
   135,   140,   141,   146,   147,   152,   157,   158,   161,   162,
   167,   168,   173,   174,   179,   180,   188,   189,   194,   195,
   200,   201,   205,   208,   209,   211,   216,   217,   220,   221,
   226,   227,   232,   233,   238,   239,   244,   245,   250,   251,
   256,   257,   262,   263,   268,   269,   274,   275,   280,   281,
   286,   287,   292,   293,   298,   299,   304,   305,   310,   311,
   316,   317,   322,   327,   328,   331,   332,   337,   338,   344,
   345,   350,   351,   357,   358,   364,   365,   371,   372,   373,
   382,   384,   387,   393,   396,   397,   401,   402,   405,   406,
   411,   412,   419,   420,   427,   428,   433,   434,   439,   440,
   446,   448,   449,   454,   457,   458,   460,   461,   463,   465,
   467,   469,   471,   472,   474,   475,   482,   483,   488,   492,
   495,   497,   498,   501,   502,   507,   508,   513,   514,   519,
   522,   523,   528,   529,   535,   536,   542,   543,   549,   550,
   555,   556,   561,   562,   567,   568,   573,   574,   580,   581,
   586,   587,   593,   594,   599,   600,   605,   606,   611,   612,
   617,   618,   623,   624,   629,   630,   636,   637,   643,   644,
   649,   650,   655,   656,   661,   662,   667,   668,   673,   674,
   679,   680,   685,   686,   691,   692,   697,   698,   703,   704,
   711,   712,   717,   718,   725,   726,   732,   733,   736,   737,
   743,   744,   749,   751,   753,   754,   756,   758,   759,   762,
   763,   768,   769,   776,   777,   784,   785,   790,   791,   796,
   797,   803,   805,   807,   809,   811,   813,   815
};

static const short yyrhs[] = {    -1,
   138,   139,     0,   140,     0,   148,     0,   152,     0,   153,
     0,   154,     0,   158,     0,   160,     0,   168,     0,   180,
     0,   200,     0,   209,     0,   230,     0,   150,     0,     3,
   135,   141,   136,     0,     0,   141,   142,     0,     0,     4,
   125,   143,   134,     0,     0,     4,   121,   144,   134,     0,
     0,     5,   125,   145,   134,     0,     0,     5,   121,   146,
   134,     0,     0,     6,   125,   147,   134,     0,     0,     7,
     8,   125,   149,   134,     0,     0,    99,   122,   151,   134,
     0,     9,   125,   134,     0,    97,    98,   134,     0,    10,
   155,     0,     0,    11,   156,   125,   134,     0,     0,    73,
   125,   157,   134,     0,    12,   159,   134,     0,   124,     0,
    13,     0,    14,   135,   161,   136,     0,     0,   161,   162,
     0,     0,    15,   122,   163,   134,     0,     0,    16,   122,
   164,   134,     0,     0,    17,   121,   165,   134,     0,     0,
    18,   122,   166,   134,     0,     0,    19,   122,   167,   134,
     0,    20,   135,   169,   136,     0,     0,   169,   170,     0,
     0,    21,   178,   171,   134,     0,     0,    22,   178,   172,
   134,     0,     0,    23,   173,   106,   134,     0,     0,    25,
   125,   125,   125,   121,   174,   134,     0,     0,    25,   125,
   175,   134,     0,     0,    25,    26,   176,   134,     0,     0,
    24,   177,   134,     0,   126,   179,     0,     0,   106,     0,
    27,   135,   181,   136,     0,     0,   181,   182,     0,     0,
    28,   126,   183,   134,     0,     0,    29,   126,   184,   134,
     0,     0,    30,   126,   185,   134,     0,     0,    31,   126,
   186,   134,     0,     0,    32,    33,   187,   134,     0,     0,
    32,    34,   188,   134,     0,     0,    32,    35,   189,   134,
     0,     0,    38,    37,   190,   134,     0,     0,    38,    34,
   191,   134,     0,     0,    38,    35,   192,   134,     0,     0,
    41,   121,   193,   134,     0,     0,    43,   121,   194,   134,
     0,     0,    44,   122,   195,   134,     0,     0,    42,   121,
   196,   134,     0,     0,    39,    36,   197,   134,     0,     0,
    39,    34,   198,   134,     0,     0,    40,   125,   199,   134,
     0,    45,   135,   201,   136,     0,     0,   201,   202,     0,
     0,    46,   121,   203,   134,     0,     0,    47,   121,   287,
   204,   134,     0,     0,    48,   121,   205,   134,     0,     0,
    49,   121,   287,   206,   134,     0,     0,    50,   121,   287,
   207,   134,     0,     0,    51,   121,   287,   208,   134,     0,
     0,     0,    55,   210,   212,   214,   135,   215,   211,   136,
     0,    58,     0,   213,   213,     0,    73,   126,   226,   227,
   228,     0,    73,   125,     0,     0,    56,    73,   278,     0,
     0,   215,   216,     0,     0,   113,   277,   217,   134,     0,
     0,   114,   115,   121,   287,   218,   134,     0,     0,   114,
   116,   121,   288,   219,   134,     0,     0,    52,   220,   223,
   134,     0,     0,    10,    73,   221,   134,     0,     0,    74,
    73,   125,   222,   134,     0,   225,     0,     0,   225,   224,
   137,   223,     0,    53,   229,     0,     0,   105,     0,     0,
   106,     0,   107,     0,   121,     0,   108,     0,   109,     0,
     0,   121,     0,     0,    57,   234,    59,   234,   231,   233,
     0,     0,    57,   234,   232,   233,     0,   135,   235,   136,
     0,    58,   179,     0,   178,     0,     0,   235,   236,     0,
     0,    60,   237,   273,   134,     0,     0,    62,    63,   238,
   134,     0,     0,    64,    65,   239,   134,     0,    66,   274,
     0,     0,    68,   125,   240,   134,     0,     0,    69,    78,
   125,   241,   134,     0,     0,    68,    78,   125,   242,   134,
     0,     0,    68,    79,   125,   243,   134,     0,     0,    68,
    77,   244,   134,     0,     0,    70,   122,   245,   134,     0,
     0,    71,   122,   246,   134,     0,     0,    72,   122,   247,
   134,     0,     0,    74,    73,   278,   248,   134,     0,     0,
   104,   278,   249,   134,     0,     0,    75,    73,   278,   250,
   134,     0,     0,    76,   122,   251,   134,     0,     0,    80,
   121,   252,   134,     0,     0,    81,   253,   277,   134,     0,
     0,    83,   122,   254,   134,     0,     0,   110,   122,   255,
   134,     0,     0,   111,   121,   256,   134,     0,     0,   118,
   125,   119,   257,   134,     0,     0,   118,   125,   120,   258,
   134,     0,     0,   112,   122,   259,   134,     0,     0,    89,
   122,   260,   134,     0,     0,    90,   122,   261,   134,     0,
     0,    84,   122,   262,   134,     0,     0,    85,   122,   263,
   134,     0,     0,    85,    86,   264,   134,     0,     0,   100,
   122,   265,   134,     0,     0,   101,   121,   266,   134,     0,
     0,   102,   121,   267,   134,     0,     0,   103,   121,   268,
   134,     0,     0,   114,   115,   121,   287,   269,   134,     0,
     0,    87,    88,   270,   134,     0,     0,   114,   116,   121,
   288,   271,   134,     0,     0,    91,   272,   135,   279,   136,
     0,     0,   273,    61,     0,     0,    78,   125,   125,   275,
   134,     0,     0,    79,   125,   276,   134,     0,    53,     0,
   121,     0,     0,   126,     0,   125,     0,     0,   279,   280,
     0,     0,   117,   281,    54,   134,     0,     0,   114,   115,
   121,   287,   282,   134,     0,     0,   114,   116,   121,   288,
   283,   134,     0,     0,    81,   277,   284,   134,     0,     0,
    96,   125,   285,   134,     0,     0,    52,    53,   229,   286,
   134,     0,   131,     0,   132,     0,   133,     0,   127,     0,
   128,     0,   129,     0,   130,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   251,   252,   255,   256,   257,   258,   259,   260,   261,   262,
   263,   264,   265,   266,   267,   272,   275,   276,   279,   290,
   290,   291,   291,   302,   302,   303,   303,   304,   308,   324,
   328,   329,   333,   347,   359,   362,   366,   367,   379,   383,
   386,   395,   408,   411,   412,   415,   416,   416,   417,   417,
   418,   418,   419,   419,   420,   424,   427,   428,   431,   436,
   436,   445,   445,   449,   450,   459,   459,   468,   468,   477,
   477,   478,   480,   492,   493,   497,   500,   501,   504,   515,
   515,   526,   526,   537,   537,   548,   548,   557,   557,   570,
   570,   583,   583,   592,   592,   605,   605,   618,   618,   635,
   635,   644,   644,   653,   653,   662,   662,   671,   671,   684,
   684,   695,   699,   702,   703,   706,   711,   711,   716,   716,
   721,   721,   726,   726,   731,   731,   740,   744,   752,   785,
   787,   792,   799,   852,   880,   885,   912,   913,   916,   921,
   921,   926,   926,   938,   938,   941,   942,   947,   947,   952,
   955,   959,   963,   966,  1022,  1023,  1026,  1027,  1028,  1031,
  1032,  1033,  1036,  1037,  1042,  1068,  1068,  1089,  1092,  1163,
  1169,  1179,  1180,  1183,  1187,  1188,  1189,  1189,  1190,  1190,
  1191,  1200,  1200,  1208,  1208,  1215,  1215,  1238,  1238,  1248,
  1248,  1249,  1249,  1250,  1250,  1251,  1251,  1260,  1260,  1275,
  1275,  1292,  1292,  1293,  1293,  1294,  1294,  1299,  1300,  1301,
  1301,  1302,  1302,  1309,  1309,  1313,  1313,  1317,  1317,  1318,
  1318,  1319,  1319,  1320,  1320,  1321,  1321,  1329,  1329,  1337,
  1337,  1345,  1345,  1354,  1354,  1363,  1363,  1372,  1372,  1377,
  1377,  1378,  1378,  1392,  1392,  1401,  1404,  1405,  1428,  1437,
  1437,  1457,  1459,  1467,  1479,  1480,  1481,  1484,  1485,  1488,
  1491,  1492,  1497,  1497,  1509,  1509,  1514,  1514,  1523,  1523,
  1618,  1621,  1622,  1623,  1626,  1627,  1628,  1629
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","PRIVSEP",
"USER","GROUP","CHROOT","PATH","PATHTYPE","INCLUDE","IDENTIFIER","VENDORID",
"LOGGING","LOGLEV","PADDING","PAD_RANDOMIZE","PAD_RANDOMIZELEN","PAD_MAXLEN",
"PAD_STRICT","PAD_EXCLTAIL","LISTEN","X_ISAKMP","X_ISAKMP_NATT","X_ADMIN","STRICT_ADDRESS",
"ADMINSOCK","DISABLED","MODECFG","CFG_NET4","CFG_MASK4","CFG_DNS4","CFG_NBNS4",
"CFG_AUTH_SOURCE","CFG_SYSTEM","CFG_RADIUS","CFG_PAM","CFG_LOCAL","CFG_NONE",
"CFG_ACCOUNTING","CFG_CONF_SOURCE","CFG_MOTD","CFG_POOL_SIZE","CFG_AUTH_THROTTLE",
"CFG_PFS_GROUP","CFG_SAVE_PASSWD","RETRY","RETRY_COUNTER","RETRY_INTERVAL","RETRY_PERSEND",
"RETRY_PHASE1","RETRY_PHASE2","NATT_KA","ALGORITHM_CLASS","ALGORITHMTYPE","STRENGTHTYPE",
"SAINFO","FROM","REMOTE","ANONYMOUS","INHERIT","EXCHANGE_MODE","EXCHANGETYPE",
"DOI","DOITYPE","SITUATION","SITUATIONTYPE","CERTIFICATE_TYPE","CERTTYPE","PEERS_CERTFILE",
"CA_TYPE","VERIFY_CERT","SEND_CERT","SEND_CR","IDENTIFIERTYPE","MY_IDENTIFIER",
"PEERS_IDENTIFIER","VERIFY_IDENTIFIER","DNSSEC","CERT_X509","CERT_PLAINRSA",
"NONCE_SIZE","DH_GROUP","KEEPALIVE","PASSIVE","INITIAL_CONTACT","NAT_TRAVERSAL",
"NAT_TRAVERSAL_LEVEL","PROPOSAL_CHECK","PROPOSAL_CHECK_LEVEL","GENERATE_POLICY",
"SUPPORT_PROXY","PROPOSAL","EXEC_PATH","EXEC_COMMAND","EXEC_SUCCESS","EXEC_FAILURE",
"GSS_ID","GSS_ID_ENC","GSS_ID_ENCTYPE","COMPLEX_BUNDLE","DPD","DPD_DELAY","DPD_RETRY",
"DPD_MAXFAIL","XAUTH_LOGIN","PREFIX","PORT","PORTANY","UL_PROTO","ANY","IKE_FRAG",
"ESP_FRAG","MODE_CFG","PFS_GROUP","LIFETIME","LIFETYPE_TIME","LIFETYPE_BYTE",
"STRENGTH","SCRIPT","PHASE1_UP","PHASE1_DOWN","NUMBER","SWITCH","BOOLEAN","HEXSTRING",
"QUOTEDSTRING","ADDRSTRING","UNITTYPE_BYTE","UNITTYPE_KBYTES","UNITTYPE_MBYTES",
"UNITTYPE_TBYTES","UNITTYPE_SEC","UNITTYPE_MIN","UNITTYPE_HOUR","EOS","BOC",
"EOC","COMMA","statements","statement","privsep_statement","privsep_stmts","privsep_stmt",
"@1","@2","@3","@4","@5","path_statement","@6","special_statement","@7","include_statement",
"gssenc_statement","identifier_statement","identifier_stmt","@8","@9","logging_statement",
"log_level","padding_statement","padding_stmts","padding_stmt","@10","@11","@12",
"@13","@14","listen_statement","listen_stmts","listen_stmt","@15","@16","@17",
"@18","@19","@20","@21","ike_addrinfo_port","ike_port","modecfg_statement","modecfg_stmts",
"modecfg_stmt","@22","@23","@24","@25","@26","@27","@28","@29","@30","@31","@32",
"@33","@34","@35","@36","@37","@38","timer_statement","timer_stmts","timer_stmt",
"@39","@40","@41","@42","@43","@44","sainfo_statement","@45","@46","sainfo_name",
"sainfo_id","sainfo_peer","sainfo_specs","sainfo_spec","@47","@48","@49","@50",
"@51","@52","algorithms","@53","algorithm","prefix","port","ul_proto","keylength",
"remote_statement","@54","@55","remote_specs_block","remote_index","remote_specs",
"remote_spec","@56","@57","@58","@59","@60","@61","@62","@63","@64","@65","@66",
"@67","@68","@69","@70","@71","@72","@73","@74","@75","@76","@77","@78","@79",
"@80","@81","@82","@83","@84","@85","@86","@87","@88","@89","@90","@91","exchange_types",
"cert_spec","@92","@93","dh_group_num","identifierstring","isakmpproposal_specs",
"isakmpproposal_spec","@94","@95","@96","@97","@98","@99","unittype_time","unittype_byte", NULL
};
#endif

static const short yyr1[] = {     0,
   138,   138,   139,   139,   139,   139,   139,   139,   139,   139,
   139,   139,   139,   139,   139,   140,   141,   141,   143,   142,
   144,   142,   145,   142,   146,   142,   147,   142,   149,   148,
   151,   150,   152,   153,   154,   156,   155,   157,   155,   158,
   159,   159,   160,   161,   161,   163,   162,   164,   162,   165,
   162,   166,   162,   167,   162,   168,   169,   169,   171,   170,
   172,   170,   173,   170,   174,   170,   175,   170,   176,   170,
   177,   170,   178,   179,   179,   180,   181,   181,   183,   182,
   184,   182,   185,   182,   186,   182,   187,   182,   188,   182,
   189,   182,   190,   182,   191,   182,   192,   182,   193,   182,
   194,   182,   195,   182,   196,   182,   197,   182,   198,   182,
   199,   182,   200,   201,   201,   203,   202,   204,   202,   205,
   202,   206,   202,   207,   202,   208,   202,   210,   211,   209,
   212,   212,   213,   213,   214,   214,   215,   215,   217,   216,
   218,   216,   219,   216,   220,   216,   221,   216,   222,   216,
   223,   224,   223,   225,   226,   226,   227,   227,   227,   228,
   228,   228,   229,   229,   231,   230,   232,   230,   233,   234,
   234,   235,   235,   237,   236,   238,   236,   239,   236,   236,
   240,   236,   241,   236,   242,   236,   243,   236,   244,   236,
   245,   236,   246,   236,   247,   236,   248,   236,   249,   236,
   250,   236,   251,   236,   252,   236,   253,   236,   254,   236,
   255,   236,   256,   236,   257,   236,   258,   236,   259,   236,
   260,   236,   261,   236,   262,   236,   263,   236,   264,   236,
   265,   236,   266,   236,   267,   236,   268,   236,   269,   236,
   270,   236,   271,   236,   272,   236,   273,   273,   275,   274,
   276,   274,   277,   277,   278,   278,   278,   279,   279,   281,
   280,   282,   280,   283,   280,   284,   280,   285,   280,   286,
   280,   287,   287,   287,   288,   288,   288,   288
};

static const short yyr2[] = {     0,
     0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     4,     0,     2,     0,     4,
     0,     4,     0,     4,     0,     4,     0,     4,     0,     5,
     0,     4,     3,     3,     2,     0,     4,     0,     4,     3,
     1,     1,     4,     0,     2,     0,     4,     0,     4,     0,
     4,     0,     4,     0,     4,     4,     0,     2,     0,     4,
     0,     4,     0,     4,     0,     7,     0,     4,     0,     4,
     0,     3,     2,     0,     1,     4,     0,     2,     0,     4,
     0,     4,     0,     4,     0,     4,     0,     4,     0,     4,
     0,     4,     0,     4,     0,     4,     0,     4,     0,     4,
     0,     4,     0,     4,     0,     4,     0,     4,     0,     4,
     0,     4,     4,     0,     2,     0,     4,     0,     5,     0,
     4,     0,     5,     0,     5,     0,     5,     0,     0,     8,
     1,     2,     5,     2,     0,     3,     0,     2,     0,     4,
     0,     6,     0,     6,     0,     4,     0,     4,     0,     5,
     1,     0,     4,     2,     0,     1,     0,     1,     1,     1,
     1,     1,     0,     1,     0,     6,     0,     4,     3,     2,
     1,     0,     2,     0,     4,     0,     4,     0,     4,     2,
     0,     4,     0,     5,     0,     5,     0,     5,     0,     4,
     0,     4,     0,     4,     0,     4,     0,     5,     0,     4,
     0,     5,     0,     4,     0,     4,     0,     4,     0,     4,
     0,     4,     0,     4,     0,     5,     0,     5,     0,     4,
     0,     4,     0,     4,     0,     4,     0,     4,     0,     4,
     0,     4,     0,     4,     0,     4,     0,     4,     0,     6,
     0,     4,     0,     6,     0,     5,     0,     2,     0,     5,
     0,     4,     1,     1,     0,     1,     1,     0,     2,     0,
     4,     0,     6,     0,     6,     0,     4,     0,     4,     0,
     5,     1,     1,     1,     1,     1,     1,     1
};

static const short yydefact[] = {     1,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
   128,     0,     0,     0,     2,     3,     4,    15,     5,     6,
     7,     8,     9,    10,    11,    12,    13,    14,    17,     0,
     0,    36,     0,    35,    42,    41,     0,    44,    57,    77,
   114,     0,    74,    74,   171,   167,     0,    31,     0,    29,
    33,     0,    38,    40,     0,     0,     0,     0,   131,     0,
   135,     0,    75,   170,    73,     0,     0,    34,     0,     0,
     0,     0,    16,    18,     0,     0,     0,     0,     0,     0,
     0,     0,    43,    45,     0,     0,    63,    71,     0,    56,
    58,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,    76,    78,     0,     0,     0,     0,     0,
     0,   113,   115,   134,   155,     0,     0,   132,   165,   172,
   168,    32,    21,    19,    25,    23,    27,    30,    37,    39,
    46,    48,    50,    52,    54,    59,    61,     0,     0,    69,
    67,    79,    81,    83,    85,    87,    89,    91,    95,    97,
    93,   109,   107,   111,    99,   105,   101,   103,   116,     0,
   120,     0,     0,     0,   156,   157,   255,   137,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,    72,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,   272,   273,   274,   118,     0,
   122,   124,   126,   158,   159,     0,   257,   256,   136,   129,
   166,   174,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,   207,     0,     0,     0,     0,     0,
     0,   245,     0,     0,     0,     0,   255,     0,     0,     0,
     0,     0,   169,   173,    22,    20,    26,    24,    28,    47,
    49,    51,    53,    55,    60,    62,    64,    70,     0,    68,
    80,    82,    84,    86,    88,    90,    92,    96,    98,    94,
   110,   108,   112,   100,   106,   102,   104,   117,     0,   121,
     0,     0,     0,   161,   162,   160,   133,     0,   145,     0,
     0,     0,     0,   138,   247,   176,   178,     0,     0,   180,
   189,     0,     0,   181,     0,   191,   193,   195,   255,   255,
   203,   205,     0,   209,   225,   229,   227,   241,   221,   223,
     0,   231,   233,   235,   237,   199,   211,   213,   219,     0,
     0,     0,    65,   119,   123,   125,   127,   147,     0,     0,
   253,   254,   139,     0,     0,   130,     0,     0,     0,     0,
   251,     0,   185,   187,     0,   183,     0,     0,     0,   197,
   201,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,   258,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,   215,   217,     0,     0,   163,     0,   151,   149,
     0,     0,     0,   248,   175,   177,   179,   249,     0,   190,
     0,     0,   182,     0,   192,   194,   196,     0,     0,   204,
   206,   208,   210,   226,   230,   228,   242,   222,   224,     0,
   232,   234,   236,   238,   200,   212,   214,   220,   239,   275,
   276,   277,   278,   243,     0,     0,    66,   148,   164,   154,
   146,     0,     0,   140,   141,   143,     0,   252,   186,   188,
   184,   198,   202,     0,     0,     0,     0,   260,   246,   259,
     0,     0,   216,   218,     0,   150,     0,     0,   250,   163,
   266,   268,     0,     0,     0,   240,   244,   153,   142,   144,
   270,     0,     0,     0,     0,     0,     0,   267,   269,   262,
   264,   261,   271,     0,     0,   263,   265,     0,     0
};

static const short yydefgoto[] = {     1,
    15,    16,    49,    74,   172,   171,   174,   173,   175,    17,
    75,    18,    69,    19,    20,    21,    34,    52,    77,    22,
    37,    23,    55,    84,   176,   177,   178,   179,   180,    24,
    56,    91,   181,   182,   138,   395,   187,   185,   139,    45,
    64,    25,    57,   105,   188,   189,   190,   191,   192,   193,
   194,   197,   195,   196,   201,   203,   204,   202,   199,   198,
   200,    26,    58,   113,   205,   289,   210,   291,   292,   293,
    27,    42,   303,    61,    62,   117,   220,   304,   401,   477,
   478,   349,   396,   453,   398,   452,   399,   166,   216,   297,
   450,    28,   169,    67,   121,    46,   170,   254,   305,   358,
   359,   365,   414,   411,   412,   362,   367,   368,   369,   418,
   387,   419,   372,   373,   323,   375,   388,   389,   445,   446,
   390,   380,   381,   376,   378,   377,   383,   384,   385,   386,
   471,   379,   472,   331,   357,   310,   457,   409,   353,   219,
   430,   470,   485,   504,   505,   492,   493,   497,   209,   444
};

static const short yypact[] = {-32768,
    40,   -96,    60,   -72,    18,     5,   -62,   -48,   -39,   -35,
-32768,   -28,     4,   -32,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,    16,
   -17,-32768,    32,-32768,-32768,-32768,   -15,-32768,-32768,-32768,
-32768,   -16,    27,    27,-32768,    84,    26,-32768,     0,-32768,
-32768,    39,-32768,-32768,    -8,    -9,    -6,    15,-32768,   -18,
   110,    97,-32768,-32768,-32768,   -28,    36,-32768,    38,   -77,
   -70,    48,-32768,-32768,    41,    42,    43,    61,    62,    53,
    63,    64,-32768,-32768,    65,    65,-32768,-32768,    -7,-32768,
-32768,    67,    68,    69,    71,    45,    35,    52,    73,    66,
    79,    80,    77,-32768,-32768,    81,    82,    83,    85,    86,
    87,-32768,-32768,-32768,   100,   136,    75,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,   105,    88,-32768,
    90,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,   -49,
-32768,   -49,   -49,   -49,-32768,     3,   -12,-32768,    36,    78,
    89,    91,    92,    93,    94,    96,    98,    99,   101,   102,
   103,   104,   107,-32768,   108,    95,   109,   111,   112,   113,
   114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
   124,   125,   126,   127,   128,-32768,-32768,-32768,-32768,   129,
-32768,-32768,-32768,-32768,-32768,   -63,-32768,-32768,-32768,     7,
-32768,-32768,   149,   148,    37,    -3,   138,   142,   143,   144,
   145,   146,   147,   150,-32768,   151,   152,   -45,   133,   153,
   154,-32768,   155,   157,   158,   159,   -12,   160,   162,   163,
     8,   106,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,   165,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,   134,-32768,
   156,   161,   164,-32768,-32768,-32768,-32768,   166,-32768,   171,
   -22,    10,   131,-32768,-32768,-32768,-32768,   167,   168,-32768,
-32768,   169,   172,-32768,   174,-32768,-32768,-32768,   -12,   -12,
-32768,-32768,   -22,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
   135,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,   170,
   175,    12,-32768,-32768,-32768,-32768,-32768,-32768,   181,   176,
-32768,-32768,-32768,   179,   182,-32768,   -33,   173,   177,   180,
-32768,   178,-32768,-32768,   183,-32768,   184,   185,   186,-32768,
-32768,   187,   188,   189,   190,   191,   192,   193,   194,   195,
   196,-32768,   197,   199,   200,   201,   202,   203,   204,   205,
   -49,   -24,-32768,-32768,   206,   207,   221,   209,   208,-32768,
   210,   -49,   -24,-32768,-32768,-32768,-32768,-32768,   212,-32768,
   213,   214,-32768,   215,-32768,-32768,-32768,   216,   217,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,   -25,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,   218,   219,-32768,-32768,-32768,-32768,
-32768,   220,   222,-32768,-32768,-32768,   224,-32768,-32768,-32768,
-32768,-32768,-32768,   228,   -22,   229,    19,-32768,-32768,-32768,
   225,   226,-32768,-32768,   181,-32768,   227,   230,-32768,   221,
-32768,-32768,   234,   241,   233,-32768,-32768,-32768,-32768,-32768,
-32768,   231,   232,   -49,   -24,   235,   236,-32768,-32768,-32768,
-32768,-32768,-32768,   237,   238,-32768,-32768,   272,-32768
};

static const short yypgoto[] = {-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,    70,
   240,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,   242,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,  -258,-32768,-32768,-32768,-32768,-32768,
  -256,-32768,-32768,-32768,   137,   223,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,  -320,  -226,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,  -162,  -383
};


#define	YYLAST		372


static const short yytable[] = {   211,
   212,   213,   374,    70,    71,    72,    78,    79,    80,    81,
    82,    85,    86,    87,    88,    89,   298,    35,   140,   456,
   336,    92,    93,    94,    95,    96,   464,   404,    32,    43,
   351,    97,    98,    99,   100,   101,   102,   103,    29,   508,
   326,    59,     2,   123,   294,   295,     3,   124,     4,     5,
   125,     6,    31,     7,   126,   465,    60,   296,   299,     8,
   106,   107,   108,   109,   110,   111,     9,    30,   149,   150,
   466,   151,    38,   311,   312,   313,   327,   146,   147,   148,
   300,   206,   207,   208,    10,   152,    39,   153,   467,    48,
    33,   468,   370,   371,    11,    40,    12,    44,   352,    41,
   405,    47,   440,   441,   442,   443,   114,   115,   214,   215,
   469,   501,   217,   218,   308,   309,    51,   141,    54,   301,
   302,   314,   340,   341,   354,   355,    90,    83,    36,   104,
   393,   394,    63,   483,   484,    73,    13,   222,    14,   223,
    50,   224,    66,   225,   481,   226,   227,   228,   229,   230,
   112,   231,   232,   233,   136,   137,    53,   234,   235,    68,
   236,   237,   238,    76,   239,   116,   240,   241,   242,    60,
   120,   122,   127,   133,   128,   129,   130,   243,   244,   245,
   246,   247,   131,   132,   134,   135,   155,   248,   249,   250,
    44,   251,   142,   143,   144,   252,   145,   154,   158,   156,
   157,   159,   160,   161,   165,   162,   163,   164,   167,   168,
   183,   306,   307,   253,   186,   315,   488,   319,   320,   269,
   328,   184,   255,   491,   256,   257,   258,   259,   439,   260,
   342,   261,   262,   397,   263,   264,   265,   266,   348,   455,
   267,   268,   270,   350,   271,   272,   273,   274,   275,   276,
   277,   278,   279,   280,   281,   282,   283,   284,   285,   286,
   287,   288,   290,   316,   317,   318,   356,   344,   321,   382,
   322,   509,   324,   325,   329,   330,   332,   333,   334,   335,
   480,   337,   338,    65,   339,   343,   496,     0,   119,   345,
   391,   360,   361,   363,   346,   392,   364,   347,   366,   402,
   400,     0,   403,   118,   408,   221,   406,     0,     0,     0,
   407,   410,     0,     0,     0,     0,   413,   415,   416,   417,
   420,   421,   422,   423,   424,   425,   426,   427,   428,   429,
   431,   500,   432,   433,   434,   435,   436,   437,   438,   447,
   448,   449,   451,   454,  -152,   458,   459,   460,   461,   462,
   463,   473,   474,   482,   494,   476,   475,   479,   486,   487,
   489,   495,     0,   490,   498,   499,     0,     0,   502,   503,
   506,   507
};

static const short yycheck[] = {   162,
   163,   164,   323,     4,     5,     6,    15,    16,    17,    18,
    19,    21,    22,    23,    24,    25,    10,    13,    26,   403,
   247,    28,    29,    30,    31,    32,    52,    61,    11,    58,
    53,    38,    39,    40,    41,    42,    43,    44,   135,     0,
    86,    58,     3,   121,   108,   109,     7,   125,     9,    10,
   121,    12,   125,    14,   125,    81,    73,   121,    52,    20,
    46,    47,    48,    49,    50,    51,    27,     8,    34,    35,
    96,    37,   135,    77,    78,    79,   122,    33,    34,    35,
    74,   131,   132,   133,    45,    34,   135,    36,   114,   122,
    73,   117,   319,   320,    55,   135,    57,   126,   121,   135,
   134,    98,   127,   128,   129,   130,   125,   126,   106,   107,
   136,   495,   125,   126,    78,    79,   134,   125,   134,   113,
   114,   125,   115,   116,   115,   116,   136,   136,   124,   136,
   119,   120,   106,   115,   116,   136,    97,    60,    99,    62,
   125,    64,    59,    66,   465,    68,    69,    70,    71,    72,
   136,    74,    75,    76,    85,    86,   125,    80,    81,   134,
    83,    84,    85,   125,    87,    56,    89,    90,    91,    73,
   135,   134,   125,   121,   134,   134,   134,   100,   101,   102,
   103,   104,   122,   122,   122,   122,   121,   110,   111,   112,
   126,   114,   126,   126,   126,   118,   126,   125,   122,   121,
   121,   121,   121,   121,   105,   121,   121,   121,    73,   135,
   106,    63,    65,   136,   125,    78,   475,    73,    73,   125,
    88,   134,   134,   480,   134,   134,   134,   134,   391,   134,
   125,   134,   134,    53,   134,   134,   134,   134,    73,   402,
   134,   134,   134,    73,   134,   134,   134,   134,   134,   134,
   134,   134,   134,   134,   134,   134,   134,   134,   134,   134,
   134,   134,   134,   122,   122,   122,   136,   134,   122,   135,
   121,     0,   122,   122,   122,   122,   122,   121,   121,   121,
    53,   122,   121,    44,   122,   121,    54,    -1,    66,   134,
   121,   125,   125,   125,   134,   121,   125,   134,   125,   121,
   125,    -1,   121,    62,   125,   169,   134,    -1,    -1,    -1,
   134,   134,    -1,    -1,    -1,    -1,   134,   134,   134,   134,
   134,   134,   134,   134,   134,   134,   134,   134,   134,   134,
   134,   494,   134,   134,   134,   134,   134,   134,   134,   134,
   134,   121,   134,   134,   137,   134,   134,   134,   134,   134,
   134,   134,   134,   125,   121,   134,   137,   134,   134,   134,
   134,   121,    -1,   134,   134,   134,    -1,    -1,   134,   134,
   134,   134
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/lib/bison.simple"
/* This file comes from bison-1.28.  */

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

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

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

#ifndef YYSTACK_USE_ALLOCA
#ifdef alloca
#define YYSTACK_USE_ALLOCA
#else /* alloca not defined */
#ifdef __GNUC__
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi) || (defined (__sun) && defined (__i386))
#define YYSTACK_USE_ALLOCA
#include <alloca.h>
#else /* not sparc */
/* We think this test detects Watcom and Microsoft C.  */
/* This used to test MSDOS, but that is a bad idea
   since that symbol is in the user namespace.  */
#if (defined (_MSDOS) || defined (_MSDOS_)) && !defined (__TURBOC__)
#if 0 /* No need for malloc.h, which pollutes the namespace;
	 instead, just don't use alloca.  */
#include <malloc.h>
#endif
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
/* I don't know what this was needed for, but it pollutes the namespace.
   So I turned it off.   rms, 2 May 1997.  */
/* #include <malloc.h>  */
 #pragma alloca
#define YYSTACK_USE_ALLOCA
#else /* not MSDOS, or __TURBOC__, or _AIX */
#if 0
#ifdef __hpux /* haible@ilog.fr says this works for HPUX 9.05 and up,
		 and on HPUX 10.  Eventually we can turn this on.  */
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#endif /* __hpux */
#endif
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc */
#endif /* not GNU C */
#endif /* alloca not defined */
#endif /* YYSTACK_USE_ALLOCA not defined */

#ifdef YYSTACK_USE_ALLOCA
#define YYSTACK_ALLOC alloca
#else
#define YYSTACK_ALLOC malloc
#endif

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Define __yy_memcpy.  Note that the size argument
   should be passed with type unsigned int, because that is what the non-GCC
   definitions require.  With GCC, __builtin_memcpy takes an arg
   of type size_t, but it can handle unsigned int.  */

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (to, from, count)
     char *to;
     char *from;
     unsigned int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *to, char *from, unsigned int count)
{
  register char *t = to;
  register char *f = from;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 217 "/usr/lib/bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#ifdef __cplusplus
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else /* not __cplusplus */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif /* not __cplusplus */
#else /* not YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* not YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
#ifdef YYPARSE_PARAM
int yyparse (void *);
#else
int yyparse (void);
#endif
#endif

int
yyparse(YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;
  int yyfree_stacks = 0;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  if (yyfree_stacks)
	    {
	      free (yyss);
	      free (yyvs);
#ifdef YYLSP_NEEDED
	      free (yyls);
#endif
	    }
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
#ifndef YYSTACK_USE_ALLOCA
      yyfree_stacks = 1;
#endif
      yyss = (short *) YYSTACK_ALLOC (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss, (char *)yyss1,
		   size * (unsigned int) sizeof (*yyssp));
      yyvs = (YYSTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs, (char *)yyvs1,
		   size * (unsigned int) sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls, (char *)yyls1,
		   size * (unsigned int) sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
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
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
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

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 19:
#line 280 "./cfparse.y"
{
			struct passwd *pw;

			if ((pw = getpwnam(yyvsp[0].val->v)) == NULL) {
				yyerror("unkown user \"%s\"", yyvsp[0].val->v);
				return -1;
			}
			lcconf->uid = pw->pw_uid;
		;
    break;}
case 21:
#line 290 "./cfparse.y"
{ lcconf->uid = yyvsp[0].num; ;
    break;}
case 23:
#line 292 "./cfparse.y"
{
			struct group *gr;

			if ((gr = getgrnam(yyvsp[0].val->v)) == NULL) {
				yyerror("unkown group \"%s\"", yyvsp[0].val->v);
				return -1;
			}
			lcconf->gid = gr->gr_gid;
		;
    break;}
case 25:
#line 302 "./cfparse.y"
{ lcconf->gid = yyvsp[0].num; ;
    break;}
case 27:
#line 303 "./cfparse.y"
{ lcconf->chroot = yyvsp[0].val->v; ;
    break;}
case 29:
#line 309 "./cfparse.y"
{
			if (yyvsp[-1].num >= LC_PATHTYPE_MAX) {
				yyerror("invalid path type %d", yyvsp[-1].num);
				return -1;
			}

			/* free old pathinfo */
			if (lcconf->pathinfo[yyvsp[-1].num])
				racoon_free(lcconf->pathinfo[yyvsp[-1].num]);

			/* set new pathinfo */
			lcconf->pathinfo[yyvsp[-1].num] = strdup(yyvsp[0].val->v);
			vfree(yyvsp[0].val);
		;
    break;}
case 31:
#line 328 "./cfparse.y"
{ lcconf->complex_bundle = yyvsp[0].num; ;
    break;}
case 33:
#line 334 "./cfparse.y"
{
			char path[MAXPATHLEN];

			getpathname(path, sizeof(path),
				LC_PATHTYPE_INCLUDE, yyvsp[-1].val->v);
			vfree(yyvsp[-1].val);
			if (yycf_switch_buffer(path) != 0)
				return -1;
		;
    break;}
case 34:
#line 348 "./cfparse.y"
{
			if (yyvsp[-1].num >= LC_GSSENC_MAX) {
				yyerror("invalid GSS ID encoding %d", yyvsp[-1].num);
				return -1;
			}
			lcconf->gss_id_enc = yyvsp[-1].num;
		;
    break;}
case 36:
#line 363 "./cfparse.y"
{
			/*XXX to be deleted */
		;
    break;}
case 38:
#line 368 "./cfparse.y"
{
			/*XXX to be deleted */
			yyvsp[0].val->l--;	/* nuke '\0' */
			lcconf->ident[yyvsp[-1].num] = yyvsp[0].val;
			if (lcconf->ident[yyvsp[-1].num] == NULL) {
				yyerror("failed to set my ident: %s",
					strerror(errno));
				return -1;
			}
		;
    break;}
case 41:
#line 387 "./cfparse.y"
{
			/*
			 * XXX ignore it because this specification
			 * will be obsoleted.
			 */
			yywarn("see racoon.conf(5), such a log specification will be obsoleted.");
			vfree(yyvsp[0].val);
		;
    break;}
case 42:
#line 396 "./cfparse.y"
{
			/*
			 * set the loglevel by configuration file only when
			 * the command line did not specify any loglevel.
			 */
			if (loglevel <= LLV_BASE)
				loglevel += yyvsp[0].num;
		;
    break;}
case 46:
#line 415 "./cfparse.y"
{ lcconf->pad_random = yyvsp[0].num; ;
    break;}
case 48:
#line 416 "./cfparse.y"
{ lcconf->pad_randomlen = yyvsp[0].num; ;
    break;}
case 50:
#line 417 "./cfparse.y"
{ lcconf->pad_maxsize = yyvsp[0].num; ;
    break;}
case 52:
#line 418 "./cfparse.y"
{ lcconf->pad_strict = yyvsp[0].num; ;
    break;}
case 54:
#line 419 "./cfparse.y"
{ lcconf->pad_excltail = yyvsp[0].num; ;
    break;}
case 59:
#line 432 "./cfparse.y"
{
			listen_addr (yyvsp[0].saddr, 0);
		;
    break;}
case 61:
#line 437 "./cfparse.y"
{
#ifdef ENABLE_NATT
			listen_addr (yyvsp[0].saddr, 1);
#else
			yyerror("NAT-T support not compiled in.");
#endif
		;
    break;}
case 63:
#line 446 "./cfparse.y"
{
			yyerror("admin directive is obsoleted.");
		;
    break;}
case 65:
#line 451 "./cfparse.y"
{
#ifdef ENABLE_ADMINPORT
			adminsock_conf(yyvsp[-3].val, yyvsp[-2].val, yyvsp[-1].val, yyvsp[0].num);
#else
			yywarn("admin port support not compiled in");
#endif
		;
    break;}
case 67:
#line 460 "./cfparse.y"
{
#ifdef ENABLE_ADMINPORT
			adminsock_conf(yyvsp[0].val, NULL, NULL, -1);
#else
			yywarn("admin port support not compiled in");
#endif
		;
    break;}
case 69:
#line 469 "./cfparse.y"
{
#ifdef ENABLE_ADMINPORT
			adminsock_path = NULL;
#else
			yywarn("admin port support not compiled in");
#endif
		;
    break;}
case 71:
#line 477 "./cfparse.y"
{ lcconf->strict_address = TRUE; ;
    break;}
case 73:
#line 481 "./cfparse.y"
{
			char portbuf[10];

			snprintf(portbuf, sizeof(portbuf), "%ld", yyvsp[0].num);
			yyval.saddr = str2saddr(yyvsp[-1].val->v, portbuf);
			vfree(yyvsp[-1].val);
			if (!yyval.saddr)
				return -1;
		;
    break;}
case 74:
#line 492 "./cfparse.y"
{ yyval.num = PORT_ISAKMP; ;
    break;}
case 75:
#line 493 "./cfparse.y"
{ yyval.num = yyvsp[0].num; ;
    break;}
case 79:
#line 505 "./cfparse.y"
{
#ifdef ENABLE_HYBRID
		 if (inet_pton(AF_INET, yyvsp[0].val->v,
		     &isakmp_cfg_config.network4) != 1)
			yyerror("bad IPv4 network address.");
#else
			yyerror("racoon not configured with --enable-hybrid");
#endif
		;
    break;}
case 81:
#line 516 "./cfparse.y"
{
#ifdef ENABLE_HYBRID
			if (inet_pton(AF_INET, yyvsp[0].val->v,
			    &isakmp_cfg_config.netmask4) != 1)
				yyerror("bad IPv4 netmask address.");
#else
			yyerror("racoon not configured with --enable-hybrid");
#endif
		;
    break;}
case 83:
#line 527 "./cfparse.y"
{
#ifdef ENABLE_HYBRID
			if (inet_pton(AF_INET, yyvsp[0].val->v,
			    &isakmp_cfg_config.dns4) != 1)
				yyerror("bad IPv4 DNS address.");
#else
			yyerror("racoon not configured with --enable-hybrid");
#endif
		;
    break;}
case 85:
#line 538 "./cfparse.y"
{
#ifdef ENABLE_HYBRID
			if (inet_pton(AF_INET, yyvsp[0].val->v,
			    &isakmp_cfg_config.nbns4) != 1)
				yyerror("bad IPv4 WINS address.");
#else
			yyerror("racoon not configured with --enable-hybrid");
#endif
		;
    break;}
case 87:
#line 549 "./cfparse.y"
{
#ifdef ENABLE_HYBRID
			isakmp_cfg_config.authsource = ISAKMP_CFG_AUTH_SYSTEM;
#else
			yyerror("racoon not configured with --enable-hybrid");
#endif
		;
    break;}
case 89:
#line 558 "./cfparse.y"
{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBRADIUS
			isakmp_cfg_config.authsource = ISAKMP_CFG_AUTH_RADIUS;
#else /* HAVE_LIBRADIUS */
			yyerror("racoon not configured with --with-libradius");
#endif /* HAVE_LIBRADIUS */
#else /* ENABLE_HYBRID */
			yyerror("racoon not configured with --enable-hybrid");
#endif /* ENABLE_HYBRID */
		;
    break;}
case 91:
#line 571 "./cfparse.y"
{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBPAM
			isakmp_cfg_config.authsource = ISAKMP_CFG_AUTH_PAM;
#else /* HAVE_LIBPAM */
			yyerror("racoon not configured with --with-libpam");
#endif /* HAVE_LIBPAM */
#else /* ENABLE_HYBRID */
			yyerror("racoon not configured with --enable-hybrid");
#endif /* ENABLE_HYBRID */
		;
    break;}
case 93:
#line 584 "./cfparse.y"
{
#ifdef ENABLE_HYBRID
			isakmp_cfg_config.accounting = ISAKMP_CFG_ACCT_NONE;
#else
			yyerror("racoon not configured with --enable-hybrid");
#endif
		;
    break;}
case 95:
#line 593 "./cfparse.y"
{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBRADIUS
			isakmp_cfg_config.accounting = ISAKMP_CFG_ACCT_RADIUS;
#else /* HAVE_LIBRADIUS */
			yyerror("racoon not configured with --with-libradius");
#endif /* HAVE_LIBRADIUS */
#else /* ENABLE_HYBRID */
			yyerror("racoon not configured with --enable-hybrid");
#endif /* ENABLE_HYBRID */
		;
    break;}
case 97:
#line 606 "./cfparse.y"
{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBPAM
			isakmp_cfg_config.accounting = ISAKMP_CFG_ACCT_PAM;
#else /* HAVE_LIBPAM */
			yyerror("racoon not configured with --with-libpam");
#endif /* HAVE_LIBPAM */
#else /* ENABLE_HYBRID */
			yyerror("racoon not configured with --enable-hybrid");
#endif /* ENABLE_HYBRID */
		;
    break;}
case 99:
#line 619 "./cfparse.y"
{
#ifdef ENABLE_HYBRID
			size_t len;

			isakmp_cfg_config.pool_size = yyvsp[0].num;

			len = yyvsp[0].num * sizeof(*isakmp_cfg_config.port_pool);
			isakmp_cfg_config.port_pool = racoon_malloc(len);
			if (isakmp_cfg_config.port_pool == NULL)
				yyerror("cannot allocate memory for pool");
			bzero(isakmp_cfg_config.port_pool, len);
#else /* ENABLE_HYBRID */
			yyerror("racoon not configured with --enable-hybrid");
#endif /* ENABLE_HYBRID */
		;
    break;}
case 101:
#line 636 "./cfparse.y"
{
#ifdef ENABLE_HYBRID
			isakmp_cfg_config.pfs_group = yyvsp[0].num;
#else /* ENABLE_HYBRID */
			yyerror("racoon not configured with --enable-hybrid");
#endif /* ENABLE_HYBRID */
		;
    break;}
case 103:
#line 645 "./cfparse.y"
{
#ifdef ENABLE_HYBRID
			isakmp_cfg_config.save_passwd = yyvsp[0].num;
#else /* ENABLE_HYBRID */
			yyerror("racoon not configured with --enable-hybrid");
#endif /* ENABLE_HYBRID */
		;
    break;}
case 105:
#line 654 "./cfparse.y"
{
#ifdef ENABLE_HYBRID
			isakmp_cfg_config.auth_throttle = yyvsp[0].num;
#else /* ENABLE_HYBRID */
			yyerror("racoon not configured with --enable-hybrid");
#endif /* ENABLE_HYBRID */
		;
    break;}
case 107:
#line 663 "./cfparse.y"
{
#ifdef ENABLE_HYBRID
			isakmp_cfg_config.confsource = ISAKMP_CFG_CONF_LOCAL;
#else /* ENABLE_HYBRID */
			yyerror("racoon not configured with --enable-hybrid");
#endif /* ENABLE_HYBRID */
		;
    break;}
case 109:
#line 672 "./cfparse.y"
{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBRADIUS
			isakmp_cfg_config.confsource = ISAKMP_CFG_CONF_RADIUS;
#else /* HAVE_LIBRADIUS */
			yyerror("racoon not configured with --with-libradius");
#endif /* HAVE_LIBRADIUS */
#else /* ENABLE_HYBRID */
			yyerror("racoon not configured with --enable-hybrid");
#endif /* ENABLE_HYBRID */
		;
    break;}
case 111:
#line 685 "./cfparse.y"
{
#ifdef ENABLE_HYBRID
			strncpy(&isakmp_cfg_config.motd[0], yyvsp[0].val->v, MAXPATHLEN);
			isakmp_cfg_config.motd[MAXPATHLEN] = '\0';
			vfree(yyvsp[0].val);
#else
			yyerror("racoon not configured with --enable-hybrid");
#endif
		;
    break;}
case 116:
#line 707 "./cfparse.y"
{
			lcconf->retry_counter = yyvsp[0].num;
		;
    break;}
case 118:
#line 712 "./cfparse.y"
{
			lcconf->retry_interval = yyvsp[-1].num * yyvsp[0].num;
		;
    break;}
case 120:
#line 717 "./cfparse.y"
{
			lcconf->count_persend = yyvsp[0].num;
		;
    break;}
case 122:
#line 722 "./cfparse.y"
{
			lcconf->retry_checkph1 = yyvsp[-1].num * yyvsp[0].num;
		;
    break;}
case 124:
#line 727 "./cfparse.y"
{
			lcconf->wait_ph2complete = yyvsp[-1].num * yyvsp[0].num;
		;
    break;}
case 126:
#line 732 "./cfparse.y"
{
#ifdef ENABLE_NATT
			lcconf->natt_ka_interval = yyvsp[-1].num * yyvsp[0].num;
#else
			yyerror("NAT-T support not compiled in.");
#endif
		;
    break;}
case 128:
#line 745 "./cfparse.y"
{
			cur_sainfo = newsainfo();
			if (cur_sainfo == NULL) {
				yyerror("failed to allocate sainfo");
				return -1;
			}
		;
    break;}
case 129:
#line 753 "./cfparse.y"
{
			struct sainfo *check;

			/* default */
			if (cur_sainfo->algs[algclass_ipsec_enc] == 0) {
				yyerror("no encryption algorithm at %s",
					sainfo2str(cur_sainfo));
				return -1;
			}
			if (cur_sainfo->algs[algclass_ipsec_auth] == 0) {
				yyerror("no authentication algorithm at %s",
					sainfo2str(cur_sainfo));
				return -1;
			}
			if (cur_sainfo->algs[algclass_ipsec_comp] == 0) {
				yyerror("no compression algorithm at %s",
					sainfo2str(cur_sainfo));
				return -1;
			}

			/* duplicate check */
			check = getsainfo(cur_sainfo->idsrc,
					  cur_sainfo->iddst,
					  cur_sainfo->id_i);
			if (check && (!check->idsrc && !cur_sainfo->idsrc)) {
				yyerror("duplicated sainfo: %s",
					sainfo2str(cur_sainfo));
				return -1;
			}
			inssainfo(cur_sainfo);
		;
    break;}
case 131:
#line 788 "./cfparse.y"
{
			cur_sainfo->idsrc = NULL;
			cur_sainfo->iddst = NULL;
		;
    break;}
case 132:
#line 793 "./cfparse.y"
{
			cur_sainfo->idsrc = yyvsp[-1].val;
			cur_sainfo->iddst = yyvsp[0].val;
		;
    break;}
case 133:
#line 800 "./cfparse.y"
{
			char portbuf[10];
			struct sockaddr *saddr;

			if ((yyvsp[0].num == IPPROTO_ICMP || yyvsp[0].num == IPPROTO_ICMPV6)
			 && (yyvsp[-1].num != IPSEC_PORT_ANY || yyvsp[-1].num != IPSEC_PORT_ANY)) {
				yyerror("port number must be \"any\".");
				return -1;
			}

			snprintf(portbuf, sizeof(portbuf), "%lu", yyvsp[-1].num);
			saddr = str2saddr(yyvsp[-3].val->v, portbuf);
			vfree(yyvsp[-3].val);
			if (saddr == NULL)
				return -1;

			switch (saddr->sa_family) {
			case AF_INET:
				if (yyvsp[0].num == IPPROTO_ICMPV6) {
					yyerror("upper layer protocol mismatched.\n");
					racoon_free(saddr);
					return -1;
				}
				yyval.val = ipsecdoi_sockaddr2id(saddr,
					yyvsp[-2].num == (sizeof(struct in_addr) << 3) &&
						yyvsp[-4].num == IDTYPE_ADDRESS
					  ? ~0 : yyvsp[-2].num,
					yyvsp[0].num);
				break;
#ifdef INET6
			case AF_INET6:
				if (yyvsp[0].num == IPPROTO_ICMP) {
					yyerror("upper layer protocol mismatched.\n");
					racoon_free(saddr);
					return -1;
				}
				yyval.val = ipsecdoi_sockaddr2id(saddr,
					yyvsp[-2].num == (sizeof(struct in6_addr) << 3) &&
						yyvsp[-4].num == IDTYPE_ADDRESS
					  ? ~0 : yyvsp[-2].num,
					yyvsp[0].num);
				break;
#endif
			default:
				yyerror("invalid family: %d", saddr->sa_family);
				yyval.val = NULL;
				break;
			}
			racoon_free(saddr);
			if (yyval.val == NULL)
				return -1;
		;
    break;}
case 134:
#line 853 "./cfparse.y"
{
			struct ipsecdoi_id_b *id_b;

			if (yyvsp[-1].num == IDTYPE_ASN1DN) {
				yyerror("id type forbidden: %d", yyvsp[-1].num);
				yyval.val = NULL;
				return -1;
			}

			yyvsp[0].val->l--;

			yyval.val = vmalloc(sizeof(*id_b) + yyvsp[0].val->l);
			if (yyval.val == NULL) {
				yyerror("failed to allocate identifier");
				return -1;
			}

			id_b = (struct ipsecdoi_id_b *)yyval.val->v;
			id_b->type = idtype2doi(yyvsp[-1].num);

			id_b->proto_id = 0;
			id_b->port = 0;

			memcpy(yyval.val->v + sizeof(*id_b), yyvsp[0].val->v, yyvsp[0].val->l);
		;
    break;}
case 135:
#line 881 "./cfparse.y"
{
			cur_sainfo->id_i = NULL;
		;
    break;}
case 136:
#line 886 "./cfparse.y"
{
			struct ipsecdoi_id_b *id_b;
			vchar_t *idv;

			if (set_identifier(&idv, yyvsp[-1].num, yyvsp[0].val) != 0) {
				yyerror("failed to set identifer.\n");
				return -1;
			}
			cur_sainfo->id_i = vmalloc(sizeof(*id_b) + idv->l);
			if (cur_sainfo->id_i == NULL) {
				yyerror("failed to allocate identifier");
				return -1;
			}

			id_b = (struct ipsecdoi_id_b *)cur_sainfo->id_i->v;
			id_b->type = idtype2doi(yyvsp[-1].num);

			id_b->proto_id = 0;
			id_b->port = 0;

			memcpy(cur_sainfo->id_i->v + sizeof(*id_b),
			       idv->v, idv->l);
			vfree(idv);
		;
    break;}
case 139:
#line 917 "./cfparse.y"
{
			cur_sainfo->pfs_group = yyvsp[0].num;
		;
    break;}
case 141:
#line 922 "./cfparse.y"
{
			cur_sainfo->lifetime = yyvsp[-1].num * yyvsp[0].num;
		;
    break;}
case 143:
#line 927 "./cfparse.y"
{
#if 1
			yyerror("byte lifetime support is deprecated");
			return -1;
#else
			cur_sainfo->lifebyte = fix_lifebyte(yyvsp[-1].num * yyvsp[0].num);
			if (cur_sainfo->lifebyte == 0)
				return -1;
#endif
		;
    break;}
case 145:
#line 938 "./cfparse.y"
{
			cur_algclass = yyvsp[0].num;
		;
    break;}
case 147:
#line 943 "./cfparse.y"
{
			yyerror("it's deprecated to specify a identifier in phase 2");
		;
    break;}
case 149:
#line 948 "./cfparse.y"
{
			yyerror("it's deprecated to specify a identifier in phase 2");
		;
    break;}
case 151:
#line 956 "./cfparse.y"
{
			inssainfoalg(&cur_sainfo->algs[cur_algclass], yyvsp[0].alg);
		;
    break;}
case 152:
#line 960 "./cfparse.y"
{
			inssainfoalg(&cur_sainfo->algs[cur_algclass], yyvsp[0].alg);
		;
    break;}
case 154:
#line 967 "./cfparse.y"
{
			int defklen;

			yyval.alg = newsainfoalg();
			if (yyval.alg == NULL) {
				yyerror("failed to get algorithm allocation");
				return -1;
			}

			yyval.alg->alg = algtype2doi(cur_algclass, yyvsp[-1].num);
			if (yyval.alg->alg == -1) {
				yyerror("algorithm mismatched");
				racoon_free(yyval.alg);
				yyval.alg = NULL;
				return -1;
			}

			defklen = default_keylen(cur_algclass, yyvsp[-1].num);
			if (defklen == 0) {
				if (yyvsp[0].num) {
					yyerror("keylen not allowed");
					racoon_free(yyval.alg);
					yyval.alg = NULL;
					return -1;
				}
			} else {
				if (yyvsp[0].num && check_keylen(cur_algclass, yyvsp[-1].num, yyvsp[0].num) < 0) {
					yyerror("invalid keylen %d", yyvsp[0].num);
					racoon_free(yyval.alg);
					yyval.alg = NULL;
					return -1;
				}
			}

			if (yyvsp[0].num)
				yyval.alg->encklen = yyvsp[0].num;
			else
				yyval.alg->encklen = defklen;

			/* check if it's supported algorithm by kernel */
			if (!(cur_algclass == algclass_ipsec_auth && yyvsp[-1].num == algtype_non_auth)
			 && pk_checkalg(cur_algclass, yyvsp[-1].num, yyval.alg->encklen)) {
				int a = algclass2doi(cur_algclass);
				int b = algtype2doi(cur_algclass, yyvsp[-1].num);
				if (a == IPSECDOI_ATTR_AUTH)
					a = IPSECDOI_PROTO_IPSEC_AH;
				yyerror("algorithm %s not supported by the kernel (missing module?)",
					s_ipsecdoi_trns(a, b));
				racoon_free(yyval.alg);
				yyval.alg = NULL;
				return -1;
			}
		;
    break;}
case 155:
#line 1022 "./cfparse.y"
{ yyval.num = ~0; ;
    break;}
case 156:
#line 1023 "./cfparse.y"
{ yyval.num = yyvsp[0].num; ;
    break;}
case 157:
#line 1026 "./cfparse.y"
{ yyval.num = IPSEC_PORT_ANY; ;
    break;}
case 158:
#line 1027 "./cfparse.y"
{ yyval.num = yyvsp[0].num; ;
    break;}
case 159:
#line 1028 "./cfparse.y"
{ yyval.num = IPSEC_PORT_ANY; ;
    break;}
case 160:
#line 1031 "./cfparse.y"
{ yyval.num = yyvsp[0].num; ;
    break;}
case 161:
#line 1032 "./cfparse.y"
{ yyval.num = yyvsp[0].num; ;
    break;}
case 162:
#line 1033 "./cfparse.y"
{ yyval.num = IPSEC_ULPROTO_ANY; ;
    break;}
case 163:
#line 1036 "./cfparse.y"
{ yyval.num = 0; ;
    break;}
case 164:
#line 1037 "./cfparse.y"
{ yyval.num = yyvsp[0].num; ;
    break;}
case 165:
#line 1043 "./cfparse.y"
{
			struct remoteconf *new;
			struct proposalspec *prspec;

			new = copyrmconf(yyvsp[0].saddr);
			if (new == NULL) {
				yyerror("failed to get remoteconf for %s.", saddr2str (yyvsp[0].saddr));
				return -1;
			}

			new->remote = yyvsp[-2].saddr;
			new->inherited_from = getrmconf_strict(yyvsp[0].saddr, 1);
			new->proposal = NULL;
			new->prhead = NULL;
			cur_rmconf = new;

			prspec = newprspec();
			if (prspec == NULL || !cur_rmconf->inherited_from 
				|| !cur_rmconf->inherited_from->proposal)
				return -1;
			prspec->lifetime = cur_rmconf->inherited_from->proposal->lifetime;
			prspec->lifebyte = cur_rmconf->inherited_from->proposal->lifebyte;
			insprspec(prspec, &cur_rmconf->prhead);
		;
    break;}
case 167:
#line 1069 "./cfparse.y"
{
			struct remoteconf *new;
			struct proposalspec *prspec;

			new = newrmconf();
			if (new == NULL) {
				yyerror("failed to get new remoteconf.");
				return -1;
			}

			new->remote = yyvsp[0].saddr;
			cur_rmconf = new;

			prspec = newprspec();
			if (prspec == NULL)
				return -1;
			prspec->lifetime = oakley_get_defaultlifetime();
			insprspec(prspec, &cur_rmconf->prhead);
		;
    break;}
case 169:
#line 1093 "./cfparse.y"
{
			/* check a exchange mode */
			if (cur_rmconf->etypes == NULL) {
				yyerror("no exchange mode specified.\n");
				return -1;
			}

			if (cur_rmconf->idvtype == IDTYPE_UNDEFINED)
				cur_rmconf->idvtype = IDTYPE_ADDRESS;


			if (cur_rmconf->idvtype == IDTYPE_ASN1DN) {
				if (cur_rmconf->mycertfile) {
					if (cur_rmconf->idv)
						yywarn("Both CERT and ASN1 ID "
						       "are set. Hope this is OK.\n");
					/* TODO: Preparse the DN here */
				} else if (cur_rmconf->idv) {
					/* OK, using asn1dn without X.509. */
				} else {
					yyerror("ASN1 ID not specified "
						"and no CERT defined!\n");
					return -1;
				}
			}
			
			if (cur_rmconf->prhead->spspec == NULL
				&& cur_rmconf->inherited_from
				&& cur_rmconf->inherited_from->prhead) {
				cur_rmconf->prhead->spspec = cur_rmconf->inherited_from->prhead->spspec;
			}
			if (set_isakmp_proposal(cur_rmconf, cur_rmconf->prhead) != 0)
				return -1;

			/* DH group settting if aggressive mode is there. */
			if (check_etypeok(cur_rmconf, ISAKMP_ETYPE_AGG) != NULL) {
				struct isakmpsa *p;
				int b = 0;

				/* DH group */
				for (p = cur_rmconf->proposal; p; p = p->next) {
					if (b == 0 || (b && b == p->dh_group)) {
						b = p->dh_group;
						continue;
					}
					yyerror("DH group must be equal "
						"in all proposals "
						"when aggressive mode is "
						"used.\n");
					return -1;
				}
				cur_rmconf->dh_group = b;

				if (cur_rmconf->dh_group == 0) {
					yyerror("DH group must be set in the proposal.\n");
					return -1;
				}

				/* DH group settting if PFS is required. */
				if (oakley_setdhgroup(cur_rmconf->dh_group,
						&cur_rmconf->dhgrp) < 0) {
					yyerror("failed to set DH value.\n");
					return -1;
				}
			}

			insrmconf(cur_rmconf);
		;
    break;}
case 170:
#line 1164 "./cfparse.y"
{
			yyval.saddr = newsaddr(sizeof(struct sockaddr));
			yyval.saddr->sa_family = AF_UNSPEC;
			((struct sockaddr_in *)yyval.saddr)->sin_port = htons(yyvsp[0].num);
		;
    break;}
case 171:
#line 1170 "./cfparse.y"
{
			yyval.saddr = yyvsp[0].saddr;
			if (yyval.saddr == NULL) {
				yyerror("failed to allocate sockaddr");
				return -1;
			}
		;
    break;}
case 174:
#line 1184 "./cfparse.y"
{
			cur_rmconf->etypes = NULL;
		;
    break;}
case 176:
#line 1188 "./cfparse.y"
{ cur_rmconf->doitype = yyvsp[0].num; ;
    break;}
case 178:
#line 1189 "./cfparse.y"
{ cur_rmconf->sittype = yyvsp[0].num; ;
    break;}
case 181:
#line 1192 "./cfparse.y"
{
			yywarn("This directive without certtype will be removed!\n");
			yywarn("Please use 'peers_certfile x509 \"%s\";' instead\n", yyvsp[0].val->v);
			cur_rmconf->getcert_method = ISAKMP_GETCERT_LOCALFILE;
			cur_rmconf->peerscertfile = strdup(yyvsp[0].val->v);
			vfree(yyvsp[0].val);
		;
    break;}
case 183:
#line 1201 "./cfparse.y"
{
			cur_rmconf->cacerttype = yyvsp[-1].num;
			cur_rmconf->getcacert_method = ISAKMP_GETCERT_LOCALFILE;
			cur_rmconf->cacertfile = strdup(yyvsp[0].val->v);
			vfree(yyvsp[0].val);
		;
    break;}
case 185:
#line 1209 "./cfparse.y"
{
			cur_rmconf->getcert_method = ISAKMP_GETCERT_LOCALFILE;
			cur_rmconf->peerscertfile = strdup(yyvsp[0].val->v);
			vfree(yyvsp[0].val);
		;
    break;}
case 187:
#line 1216 "./cfparse.y"
{
			char path[MAXPATHLEN];
			int ret = 0;

			getpathname(path, sizeof(path),
				LC_PATHTYPE_CERT, yyvsp[0].val->v);
			vfree(yyvsp[0].val);

			if (cur_rmconf->getcert_method == ISAKMP_GETCERT_DNS) {
				yyerror("Different peers_certfile method "
					"already defined: %d!\n",
					cur_rmconf->getcert_method);
				return -1;
			}
			cur_rmconf->getcert_method = ISAKMP_GETCERT_LOCALFILE;
			if (rsa_parse_file(cur_rmconf->rsa_public, path, RSA_TYPE_PUBLIC)) {
				yyerror("Couldn't parse keyfile.\n", path);
				return -1;
			}
			plog(LLV_DEBUG, LOCATION, NULL, "Public PlainRSA keyfile parsed: %s\n", path);
		;
    break;}
case 189:
#line 1239 "./cfparse.y"
{
			if (cur_rmconf->getcert_method) {
				yyerror("Different peers_certfile method already defined!\n");
				return -1;
			}
			cur_rmconf->getcert_method = ISAKMP_GETCERT_DNS;
			cur_rmconf->peerscertfile = NULL;
		;
    break;}
case 191:
#line 1248 "./cfparse.y"
{ cur_rmconf->verify_cert = yyvsp[0].num; ;
    break;}
case 193:
#line 1249 "./cfparse.y"
{ cur_rmconf->send_cert = yyvsp[0].num; ;
    break;}
case 195:
#line 1250 "./cfparse.y"
{ cur_rmconf->send_cr = yyvsp[0].num; ;
    break;}
case 197:
#line 1252 "./cfparse.y"
{
			if (set_identifier(&cur_rmconf->idv, yyvsp[-1].num, yyvsp[0].val) != 0) {
				yyerror("failed to set identifer.\n");
				return -1;
			}
			cur_rmconf->idvtype = yyvsp[-1].num;
		;
    break;}
case 199:
#line 1261 "./cfparse.y"
{
#ifdef ENABLE_HYBRID
			/* formerly identifier type login */
			cur_rmconf->idvtype = IDTYPE_LOGIN;
			if (set_identifier(&cur_rmconf->idv, IDTYPE_LOGIN, yyvsp[0].val) != 0) {
				yyerror("failed to set identifer.\n");
				return -1;
			}
			/* cur_rmconf->use_xauth = 1; */
#else
			yyerror("racoon not configured with --enable-hybrid");
#endif
		;
    break;}
case 201:
#line 1276 "./cfparse.y"
{
			struct idspec  *id;
			id = newidspec();
			if (id == NULL) {
				yyerror("failed to allocate idspec");
				return -1;
			}
			if (set_identifier(&id->id, yyvsp[-1].num, yyvsp[0].val) != 0) {
				yyerror("failed to set identifer.\n");
				racoon_free(id);
				return -1;
			}
			id->idtype = yyvsp[-1].num;
			genlist_append (cur_rmconf->idvl_p, id);
		;
    break;}
case 203:
#line 1292 "./cfparse.y"
{ cur_rmconf->verify_identifier = yyvsp[0].num; ;
    break;}
case 205:
#line 1293 "./cfparse.y"
{ cur_rmconf->nonce_size = yyvsp[0].num; ;
    break;}
case 207:
#line 1295 "./cfparse.y"
{
			yyerror("dh_group cannot be defined here.");
			return -1;
		;
    break;}
case 209:
#line 1300 "./cfparse.y"
{ cur_rmconf->passive = yyvsp[0].num; ;
    break;}
case 211:
#line 1301 "./cfparse.y"
{ cur_rmconf->ike_frag = yyvsp[0].num; ;
    break;}
case 213:
#line 1302 "./cfparse.y"
{ 
#ifdef SADB_X_EXT_NAT_T_FRAG
			cur_rmconf->esp_frag = yyvsp[0].num; 
#else
			yywarn("Your kernel does not support esp_frag");
#endif
		;
    break;}
case 215:
#line 1309 "./cfparse.y"
{ 
			cur_rmconf->script[SCRIPT_PHASE1_UP] = 
			    script_path_add(vdup(yyvsp[-1].val));
		;
    break;}
case 217:
#line 1313 "./cfparse.y"
{ 
			cur_rmconf->script[SCRIPT_PHASE1_DOWN] = 
			    script_path_add(vdup(yyvsp[-1].val));
		;
    break;}
case 219:
#line 1317 "./cfparse.y"
{ cur_rmconf->mode_cfg = yyvsp[0].num; ;
    break;}
case 221:
#line 1318 "./cfparse.y"
{ cur_rmconf->gen_policy = yyvsp[0].num; ;
    break;}
case 223:
#line 1319 "./cfparse.y"
{ cur_rmconf->support_proxy = yyvsp[0].num; ;
    break;}
case 225:
#line 1320 "./cfparse.y"
{ cur_rmconf->ini_contact = yyvsp[0].num; ;
    break;}
case 227:
#line 1322 "./cfparse.y"
{
#ifdef ENABLE_NATT
			cur_rmconf->nat_traversal = yyvsp[0].num;
#else
			yyerror("NAT-T support not compiled in.");
#endif
		;
    break;}
case 229:
#line 1330 "./cfparse.y"
{
#ifdef ENABLE_NATT
			cur_rmconf->nat_traversal = yyvsp[0].num;
#else
			yyerror("NAT-T support not compiled in.");
#endif
		;
    break;}
case 231:
#line 1338 "./cfparse.y"
{
#ifdef ENABLE_DPD
			cur_rmconf->dpd = yyvsp[0].num;
#else
			yyerror("DPD support not compiled in.");
#endif
		;
    break;}
case 233:
#line 1346 "./cfparse.y"
{
#ifdef ENABLE_DPD
			cur_rmconf->dpd_interval = yyvsp[0].num;
#else
			yyerror("DPD support not compiled in.");
#endif
		;
    break;}
case 235:
#line 1355 "./cfparse.y"
{
#ifdef ENABLE_DPD
			cur_rmconf->dpd_retry = yyvsp[0].num;
#else
			yyerror("DPD support not compiled in.");
#endif
		;
    break;}
case 237:
#line 1364 "./cfparse.y"
{
#ifdef ENABLE_DPD
			cur_rmconf->dpd_maxfails = yyvsp[0].num;
#else
			yyerror("DPD support not compiled in.");
#endif
		;
    break;}
case 239:
#line 1373 "./cfparse.y"
{
			cur_rmconf->prhead->lifetime = yyvsp[-1].num * yyvsp[0].num;
		;
    break;}
case 241:
#line 1377 "./cfparse.y"
{ cur_rmconf->pcheck_level = yyvsp[0].num; ;
    break;}
case 243:
#line 1379 "./cfparse.y"
{
#if 1
			yyerror("byte lifetime support is deprecated in Phase1");
			return -1;
#else
			yywarn("the lifetime of bytes in phase 1 "
				"will be ignored at the moment.");
			cur_rmconf->prhead->lifebyte = fix_lifebyte(yyvsp[-1].num * yyvsp[0].num);
			if (cur_rmconf->prhead->lifebyte == 0)
				return -1;
#endif
		;
    break;}
case 245:
#line 1393 "./cfparse.y"
{
			struct secprotospec *spspec;

			spspec = newspspec();
			if (spspec == NULL)
				return -1;
			insspspec(spspec, &cur_rmconf->prhead);
		;
    break;}
case 248:
#line 1406 "./cfparse.y"
{
			struct etypes *new;
			new = racoon_malloc(sizeof(struct etypes));
			if (new == NULL) {
				yyerror("filed to allocate etypes");
				return -1;
			}
			new->type = yyvsp[0].num;
			new->next = NULL;
			if (cur_rmconf->etypes == NULL)
				cur_rmconf->etypes = new;
			else {
				struct etypes *p;
				for (p = cur_rmconf->etypes;
				     p->next != NULL;
				     p = p->next)
					;
				p->next = new;
			}
		;
    break;}
case 249:
#line 1429 "./cfparse.y"
{
			cur_rmconf->certtype = yyvsp[-2].num;
			cur_rmconf->mycertfile = strdup(yyvsp[-1].val->v);
			vfree(yyvsp[-1].val);
			cur_rmconf->myprivfile = strdup(yyvsp[0].val->v);
			vfree(yyvsp[0].val);
		;
    break;}
case 251:
#line 1438 "./cfparse.y"
{
			char path[MAXPATHLEN];
			int ret = 0;

			getpathname(path, sizeof(path),
				LC_PATHTYPE_CERT, yyvsp[0].val->v);
			vfree(yyvsp[0].val);

			cur_rmconf->certtype = yyvsp[-1].num;
			cur_rmconf->send_cr = FALSE;
			cur_rmconf->send_cert = FALSE;
			cur_rmconf->verify_cert = FALSE;
			if (rsa_parse_file(cur_rmconf->rsa_private, path, RSA_TYPE_PRIVATE)) {
				yyerror("Couldn't parse keyfile.\n", path);
				return -1;
			}
			plog(LLV_DEBUG, LOCATION, NULL, "Private PlainRSA keyfile parsed: %s\n", path);
		;
    break;}
case 253:
#line 1460 "./cfparse.y"
{
			yyval.num = algtype2doi(algclass_isakmp_dh, yyvsp[0].num);
			if (yyval.num == -1) {
				yyerror("must be DH group");
				return -1;
			}
		;
    break;}
case 254:
#line 1468 "./cfparse.y"
{
			if (ARRAYLEN(num2dhgroup) > yyvsp[0].num && num2dhgroup[yyvsp[0].num] != 0) {
				yyval.num = num2dhgroup[yyvsp[0].num];
			} else {
				yyerror("must be DH group");
				yyval.num = 0;
				return -1;
			}
		;
    break;}
case 255:
#line 1479 "./cfparse.y"
{ yyval.val = NULL; ;
    break;}
case 256:
#line 1480 "./cfparse.y"
{ yyval.val = yyvsp[0].val; ;
    break;}
case 257:
#line 1481 "./cfparse.y"
{ yyval.val = yyvsp[0].val; ;
    break;}
case 260:
#line 1489 "./cfparse.y"
{
			yyerror("strength directive is obsoleted.");
		;
    break;}
case 262:
#line 1493 "./cfparse.y"
{
			cur_rmconf->prhead->spspec->lifetime = yyvsp[-1].num * yyvsp[0].num;
		;
    break;}
case 264:
#line 1498 "./cfparse.y"
{
#if 1
			yyerror("byte lifetime support is deprecated");
			return -1;
#else
			cur_rmconf->prhead->spspec->lifebyte = fix_lifebyte(yyvsp[-1].num * yyvsp[0].num);
			if (cur_rmconf->prhead->spspec->lifebyte == 0)
				return -1;
#endif
		;
    break;}
case 266:
#line 1510 "./cfparse.y"
{
			cur_rmconf->prhead->spspec->algclass[algclass_isakmp_dh] = yyvsp[0].num;
		;
    break;}
case 268:
#line 1515 "./cfparse.y"
{
			if (cur_rmconf->prhead->spspec->vendorid != VENDORID_GSSAPI) {
				yyerror("wrong Vendor ID for gssapi_id");
				return -1;
			}
			cur_rmconf->prhead->spspec->gssid = strdup(yyvsp[0].val->v);
		;
    break;}
case 270:
#line 1524 "./cfparse.y"
{
			int doi;
			int defklen;

			doi = algtype2doi(yyvsp[-2].num, yyvsp[-1].num);
			if (doi == -1) {
				yyerror("algorithm mismatched 1");
				return -1;
			}

			switch (yyvsp[-2].num) {
			case algclass_isakmp_enc:
			/* reject suppressed algorithms */
#ifndef HAVE_OPENSSL_RC5_H
				if (yyvsp[-1].num == algtype_rc5) {
					yyerror("algorithm %s not supported",
					    s_attr_isakmp_enc(doi));
					return -1;
				}
#endif
#ifndef HAVE_OPENSSL_IDEA_H
				if (yyvsp[-1].num == algtype_idea) {
					yyerror("algorithm %s not supported",
					    s_attr_isakmp_enc(doi));
					return -1;
				}
#endif

				cur_rmconf->prhead->spspec->algclass[algclass_isakmp_enc] = doi;
				defklen = default_keylen(yyvsp[-2].num, yyvsp[-1].num);
				if (defklen == 0) {
					if (yyvsp[0].num) {
						yyerror("keylen not allowed");
						return -1;
					}
				} else {
					if (yyvsp[0].num && check_keylen(yyvsp[-2].num, yyvsp[-1].num, yyvsp[0].num) < 0) {
						yyerror("invalid keylen %d", yyvsp[0].num);
						return -1;
					}
				}
				if (yyvsp[0].num)
					cur_rmconf->prhead->spspec->encklen = yyvsp[0].num;
				else
					cur_rmconf->prhead->spspec->encklen = defklen;
				break;
			case algclass_isakmp_hash:
				cur_rmconf->prhead->spspec->algclass[algclass_isakmp_hash] = doi;
				break;
			case algclass_isakmp_ameth:
				cur_rmconf->prhead->spspec->algclass[algclass_isakmp_ameth] = doi;
				/*
				 * We may have to set the Vendor ID for the
				 * authentication method we're using.
				 */
				switch (yyvsp[-1].num) {
				case algtype_gssapikrb:
					if (cur_rmconf->prhead->spspec->vendorid !=
					    VENDORID_UNKNOWN) {
						yyerror("Vendor ID mismatch "
						    "for auth method");
						return -1;
					}
					/*
					 * For interoperability with Win2k,
					 * we set the Vendor ID to "GSSAPI".
					 */
					cur_rmconf->prhead->spspec->vendorid =
					    VENDORID_GSSAPI;
					break;
				case algtype_rsasig:
					if (cur_rmconf->certtype == ISAKMP_CERT_PLAINRSA) {
						if (rsa_list_count(cur_rmconf->rsa_private) == 0) {
							yyerror ("Private PlainRSA key not set. "
								"Use directive 'certificate_type plainrsa ...'\n");
							return -1;
						}
						if (rsa_list_count(cur_rmconf->rsa_public) == 0) {
							yyerror ("Public PlainRSA keys not set. "
								"Use directive 'peers_certfile plainrsa ...'\n");
							return -1;
						}
					}
					break;
				default:
					break;
				}
				break;
			default:
				yyerror("algorithm mismatched 2");
				return -1;
			}
		;
    break;}
case 272:
#line 1621 "./cfparse.y"
{ yyval.num = 1; ;
    break;}
case 273:
#line 1622 "./cfparse.y"
{ yyval.num = 60; ;
    break;}
case 274:
#line 1623 "./cfparse.y"
{ yyval.num = (60 * 60); ;
    break;}
case 275:
#line 1626 "./cfparse.y"
{ yyval.num = 1; ;
    break;}
case 276:
#line 1627 "./cfparse.y"
{ yyval.num = 1024; ;
    break;}
case 277:
#line 1628 "./cfparse.y"
{ yyval.num = (1024 * 1024); ;
    break;}
case 278:
#line 1629 "./cfparse.y"
{ yyval.num = (1024 * 1024 * 1024); ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 543 "/usr/lib/bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

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

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;

 yyacceptlab:
  /* YYACCEPT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 0;

 yyabortlab:
  /* YYABORT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 1;
}
#line 1631 "./cfparse.y"


static struct proposalspec *
newprspec()
{
	struct proposalspec *new;

	new = racoon_calloc(1, sizeof(*new));
	if (new == NULL)
		yyerror("failed to allocate proposal");

	return new;
}

/*
 * insert into head of list.
 */
static void
insprspec(prspec, head)
	struct proposalspec *prspec;
	struct proposalspec **head;
{
	if (*head != NULL)
		(*head)->prev = prspec;
	prspec->next = *head;
	*head = prspec;
}

static struct secprotospec *
newspspec()
{
	struct secprotospec *new;

	new = racoon_calloc(1, sizeof(*new));
	if (new == NULL) {
		yyerror("failed to allocate spproto");
		return NULL;
	}

	new->encklen = 0;	/*XXX*/

	/*
	 * Default to "uknown" vendor -- we will override this
	 * as necessary.  When we send a Vendor ID payload, an
	 * "unknown" will be translated to a KAME/racoon ID.
	 */
	new->vendorid = VENDORID_UNKNOWN;

	return new;
}

/*
 * insert into head of list.
 */
static void
insspspec(spspec, head)
	struct secprotospec *spspec;
	struct proposalspec **head;
{
	spspec->back = *head;

	if ((*head)->spspec != NULL)
		(*head)->spspec->prev = spspec;
	spspec->next = (*head)->spspec;
	(*head)->spspec = spspec;
}

/* set final acceptable proposal */
static int
set_isakmp_proposal(rmconf, prspec)
	struct remoteconf *rmconf;
	struct proposalspec *prspec;
{
	struct proposalspec *p;
	struct secprotospec *s;
	int prop_no = 1; 
	int trns_no = 1;
	int32_t types[MAXALGCLASS];

	p = prspec;
	if (p->next != 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"multiple proposal definition.\n");
		return -1;
	}

	/* mandatory check */
	if (p->spspec == NULL) {
		yyerror("no remote specification found: %s.\n",
			saddr2str(rmconf->remote));
		return -1;
	}
	for (s = p->spspec; s != NULL; s = s->next) {
		/* XXX need more to check */
		if (s->algclass[algclass_isakmp_enc] == 0) {
			yyerror("encryption algorithm required.");
			return -1;
		}
		if (s->algclass[algclass_isakmp_hash] == 0) {
			yyerror("hash algorithm required.");
			return -1;
		}
		if (s->algclass[algclass_isakmp_dh] == 0) {
			yyerror("DH group required.");
			return -1;
		}
		if (s->algclass[algclass_isakmp_ameth] == 0) {
			yyerror("authentication method required.");
			return -1;
		}
	}

	/* skip to last part */
	for (s = p->spspec; s->next != NULL; s = s->next)
		;

	while (s != NULL) {
		plog(LLV_DEBUG2, LOCATION, NULL,
			"lifetime = %ld\n", (long)
			(s->lifetime ? s->lifetime : p->lifetime));
		plog(LLV_DEBUG2, LOCATION, NULL,
			"lifebyte = %d\n",
			s->lifebyte ? s->lifebyte : p->lifebyte);
		plog(LLV_DEBUG2, LOCATION, NULL,
			"encklen=%d\n", s->encklen);

		memset(types, 0, ARRAYLEN(types));
		types[algclass_isakmp_enc] = s->algclass[algclass_isakmp_enc];
		types[algclass_isakmp_hash] = s->algclass[algclass_isakmp_hash];
		types[algclass_isakmp_dh] = s->algclass[algclass_isakmp_dh];
		types[algclass_isakmp_ameth] =
		    s->algclass[algclass_isakmp_ameth];

		/* expanding spspec */
		clean_tmpalgtype();
		trns_no = expand_isakmpspec(prop_no, trns_no, types,
				algclass_isakmp_enc, algclass_isakmp_ameth + 1,
				s->lifetime ? s->lifetime : p->lifetime,
				s->lifebyte ? s->lifebyte : p->lifebyte,
				s->encklen, s->vendorid, s->gssid,
				rmconf);
		if (trns_no == -1) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to expand isakmp proposal.\n");
			return -1;
		}

		s = s->prev;
	}

	if (rmconf->proposal == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"no proposal found.\n");
		return -1;
	}

	return 0;
}

static void
clean_tmpalgtype()
{
	int i;
	for (i = 0; i < MAXALGCLASS; i++)
		tmpalgtype[i] = 0;	/* means algorithm undefined. */
}

static int
expand_isakmpspec(prop_no, trns_no, types,
		class, last, lifetime, lifebyte, encklen, vendorid, gssid,
		rmconf)
	int prop_no, trns_no;
	int *types, class, last;
	time_t lifetime;
	int lifebyte;
	int encklen;
	int vendorid;
	char *gssid;
	struct remoteconf *rmconf;
{
	struct isakmpsa *new;

	/* debugging */
    {
	int j;
	char tb[10];
	plog(LLV_DEBUG2, LOCATION, NULL,
		"p:%d t:%d\n", prop_no, trns_no);
	for (j = class; j < MAXALGCLASS; j++) {
		snprintf(tb, sizeof(tb), "%d", types[j]);
		plog(LLV_DEBUG2, LOCATION, NULL,
			"%s%s%s%s\n",
			s_algtype(j, types[j]),
			types[j] ? "(" : "",
			tb[0] == '0' ? "" : tb,
			types[j] ? ")" : "");
	}
	plog(LLV_DEBUG2, LOCATION, NULL, "\n");
    }

#define TMPALGTYPE2STR(n) \
	s_algtype(algclass_isakmp_##n, types[algclass_isakmp_##n])
		/* check mandatory values */
		if (types[algclass_isakmp_enc] == 0
		 || types[algclass_isakmp_ameth] == 0
		 || types[algclass_isakmp_hash] == 0
		 || types[algclass_isakmp_dh] == 0) {
			yyerror("few definition of algorithm "
				"enc=%s ameth=%s hash=%s dhgroup=%s.\n",
				TMPALGTYPE2STR(enc),
				TMPALGTYPE2STR(ameth),
				TMPALGTYPE2STR(hash),
				TMPALGTYPE2STR(dh));
			return -1;
		}
#undef TMPALGTYPE2STR

	/* set new sa */
	new = newisakmpsa();
	if (new == NULL) {
		yyerror("failed to allocate isakmp sa");
		return -1;
	}
	new->prop_no = prop_no;
	new->trns_no = trns_no++;
	new->lifetime = lifetime;
	new->lifebyte = lifebyte;
	new->enctype = types[algclass_isakmp_enc];
	new->encklen = encklen;
	new->authmethod = types[algclass_isakmp_ameth];
	new->hashtype = types[algclass_isakmp_hash];
	new->dh_group = types[algclass_isakmp_dh];
	new->vendorid = vendorid;
#ifdef HAVE_GSSAPI
	if (new->authmethod == OAKLEY_ATTR_AUTH_METHOD_GSSAPI_KRB) {
		if (gssid != NULL) {
			new->gssid = vmalloc(strlen(gssid));
			memcpy(new->gssid->v, gssid, new->gssid->l);
			racoon_free(gssid);
		} else {
			/*
			 * Allocate the default ID so that it gets put
			 * into a GSS ID attribute during the Phase 1
			 * exchange.
			 */
			new->gssid = gssapi_get_default_gss_id();
		}
	}
#endif
	insisakmpsa(new, rmconf);

	return trns_no;
}

static int
listen_addr (struct sockaddr *addr, int udp_encap)
{
	struct myaddrs *p;

	p = newmyaddr();
	if (p == NULL) {
		yyerror("failed to allocate myaddrs");
		return -1;
	}
	p->addr = addr;
	if (p->addr == NULL) {
		yyerror("failed to copy sockaddr ");
		delmyaddr(p);
		return -1;
	}
	p->udp_encap = udp_encap;

	insmyaddr(p, &lcconf->myaddrs);

	lcconf->autograbaddr = 0;
	return 0;
}

#if 0
/*
 * fix lifebyte.
 * Must be more than 1024B because its unit is kilobytes.
 * That is defined RFC2407.
 */
static int
fix_lifebyte(t)
	unsigned long t;
{
	if (t < 1024) {
		yyerror("byte size should be more than 1024B.");
		return 0;
	}

	return(t / 1024);
}
#endif

int
cfparse()
{
	int error;

	yycf_init_buffer();

	if (yycf_switch_buffer(lcconf->racoon_conf) != 0)
		return -1;

	error = yyparse();
	if (error != 0) {
		if (yyerrorcount) {
			plog(LLV_ERROR, LOCATION, NULL,
				"fatal parse failure (%d errors)\n",
				yyerrorcount);
		} else {
			plog(LLV_ERROR, LOCATION, NULL,
				"fatal parse failure.\n");
		}
		return -1;
	}

	if (error == 0 && yyerrorcount) {
		plog(LLV_ERROR, LOCATION, NULL,
			"parse error is nothing, but yyerrorcount is %d.\n",
				yyerrorcount);
		exit(1);
	}

	yycf_clean_buffer();

	plog(LLV_DEBUG2, LOCATION, NULL, "parse successed.\n");

	return 0;
}

int
cfreparse()
{
	flushph2();
	flushph1();
	flushrmconf();
	flushsainfo();
	clean_tmpalgtype();
	yycf_init_buffer();

	if (yycf_switch_buffer(lcconf->racoon_conf) != 0)
		return -1;

	return(cfparse());
}

#ifdef ENABLE_ADMINPORT
static void
adminsock_conf(path, owner, group, mode_dec)
	vchar_t *path;
	vchar_t *owner;
	vchar_t *group;
	int mode_dec;
{
	struct passwd *pw = NULL;
	struct group *gr = NULL;
	mode_t mode = 0;
	uid_t uid;
	gid_t gid;
	int isnum;

	adminsock_path = path->v;

	if (owner == NULL)
		return;

	errno = 0;
	uid = atoi(owner->v);
	isnum = !errno;
	if (((pw = getpwnam(owner->v)) == NULL) && !isnum)
		yyerror("User \"%s\" does not exist", owner->v);

	if (pw)
		adminsock_owner = pw->pw_uid;
	else
		adminsock_owner = uid;

	if (group == NULL)
		return;

	errno = 0;
	gid = atoi(group->v);
	isnum = !errno;
	if (((gr = getgrnam(group->v)) == NULL) && !isnum)
		yyerror("Group \"%s\" does not exist", group->v);

	if (gr)
		adminsock_group = gr->gr_gid;
	else
		adminsock_group = gid;

	if (mode_dec == -1)
		return;

	if (mode_dec > 777)
		yyerror("Mode 0%03o is invalid", mode_dec);
	if (mode_dec >= 400) { mode += 0400; mode_dec -= 400; }
	if (mode_dec >= 200) { mode += 0200; mode_dec -= 200; }
	if (mode_dec >= 100) { mode += 0200; mode_dec -= 100; }

	if (mode_dec > 77)
		yyerror("Mode 0%03o is invalid", mode_dec);
	if (mode_dec >= 40) { mode += 040; mode_dec -= 40; }
	if (mode_dec >= 20) { mode += 020; mode_dec -= 20; }
	if (mode_dec >= 10) { mode += 020; mode_dec -= 10; }

	if (mode_dec > 7)
		yyerror("Mode 0%03o is invalid", mode_dec);
	if (mode_dec >= 4) { mode += 04; mode_dec -= 4; }
	if (mode_dec >= 2) { mode += 02; mode_dec -= 2; }
	if (mode_dec >= 1) { mode += 02; mode_dec -= 1; }
	
	adminsock_mode = mode;

	return;
}
#endif
