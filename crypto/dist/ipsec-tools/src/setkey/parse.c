/*	$NetBSD: parse.c,v 1.2.2.2 2005/11/21 21:12:34 tron Exp $	*/


/*  A Bison parser, made from parse.y
    by GNU Bison version 1.28  */

#define YYBISON 1  /* Identify Bison output.  */

#define	EOT	257
#define	SLASH	258
#define	BLCL	259
#define	ELCL	260
#define	ADD	261
#define	GET	262
#define	DELETE	263
#define	DELETEALL	264
#define	FLUSH	265
#define	DUMP	266
#define	EXIT	267
#define	PR_ESP	268
#define	PR_AH	269
#define	PR_IPCOMP	270
#define	PR_ESPUDP	271
#define	PR_TCP	272
#define	F_PROTOCOL	273
#define	F_AUTH	274
#define	F_ENC	275
#define	F_REPLAY	276
#define	F_COMP	277
#define	F_RAWCPI	278
#define	F_MODE	279
#define	MODE	280
#define	F_REQID	281
#define	F_EXT	282
#define	EXTENSION	283
#define	NOCYCLICSEQ	284
#define	ALG_AUTH	285
#define	ALG_AUTH_NOKEY	286
#define	ALG_ENC	287
#define	ALG_ENC_NOKEY	288
#define	ALG_ENC_DESDERIV	289
#define	ALG_ENC_DES32IV	290
#define	ALG_ENC_OLD	291
#define	ALG_COMP	292
#define	F_LIFETIME_HARD	293
#define	F_LIFETIME_SOFT	294
#define	F_LIFEBYTE_HARD	295
#define	F_LIFEBYTE_SOFT	296
#define	DECSTRING	297
#define	QUOTEDSTRING	298
#define	HEXSTRING	299
#define	STRING	300
#define	ANY	301
#define	SPDADD	302
#define	SPDDELETE	303
#define	SPDDUMP	304
#define	SPDFLUSH	305
#define	F_POLICY	306
#define	PL_REQUESTS	307
#define	F_AIFLAGS	308
#define	TAGGED	309

#line 32 "parse.y"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <net/pfkeyv2.h>
#ifdef HAVE_NETINET6_IPSEC
#  include <netinet6/ipsec.h>
#else 
#  include <netinet/ipsec.h>
#endif
#include <arpa/inet.h>

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#include "libpfkey.h"
#include "vchar.h"
#include "extern.h"

#define DEFAULT_NATT_PORT	4500

#ifndef UDP_ENCAP_ESPINUDP
#define UDP_ENCAP_ESPINUDP	2
#endif

#define ATOX(c) \
  (isdigit((int)c) ? (c - '0') : \
    (isupper((int)c) ? (c - 'A' + 10) : (c - 'a' + 10)))

u_int32_t p_spi;
u_int p_ext, p_alg_enc, p_alg_auth, p_replay, p_mode;
u_int32_t p_reqid;
u_int p_key_enc_len, p_key_auth_len;
const char *p_key_enc;
const char *p_key_auth;
time_t p_lt_hard, p_lt_soft;
size_t p_lb_hard, p_lb_soft;

static u_int p_natt_type;
static struct addrinfo * p_natt_oa = NULL;

static int p_aiflags = 0, p_aifamily = PF_UNSPEC;

static struct addrinfo *parse_addr __P((char *, char *));
static int fix_portstr __P((vchar_t *, vchar_t *, vchar_t *));
static int setvarbuf __P((char *, int *, struct sadb_ext *, int, 
    const void *, int));
void parse_init __P((void));
void free_buffer __P((void));

int setkeymsg0 __P((struct sadb_msg *, unsigned int, unsigned int, size_t));
static int setkeymsg_spdaddr __P((unsigned int, unsigned int, vchar_t *,
	struct addrinfo *, int, struct addrinfo *, int));
static int setkeymsg_spdaddr_tag __P((unsigned int, char *, vchar_t *));
static int setkeymsg_addr __P((unsigned int, unsigned int,
	struct addrinfo *, struct addrinfo *, int));
static int setkeymsg_add __P((unsigned int, unsigned int,
	struct addrinfo *, struct addrinfo *));

#line 103 "parse.y"
typedef union {
	int num;
	unsigned long ulnum;
	vchar_t val;
	struct addrinfo *res;
} YYSTYPE;
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		160
#define	YYFLAG		-32768
#define	YYNTBASE	56

#define YYTRANSLATE(x) ((unsigned)(x) <= 309 ? yytranslate[x] : 90)

static const char yytranslate[] = {     0,
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
    47,    48,    49,    50,    51,    52,    53,    54,    55
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     1,     4,     6,     8,    10,    12,    14,    16,    18,
    20,    22,    24,    26,    36,    45,    52,    61,    65,    69,
    70,    72,    74,    76,    78,    81,    83,    85,    87,    89,
    91,    93,    98,   101,   104,   107,   111,   113,   116,   118,
   121,   124,   127,   129,   131,   133,   134,   137,   140,   143,
   146,   149,   152,   155,   158,   161,   164,   167,   180,   186,
   199,   202,   205,   206,   209,   211,   213,   215,   218,   219,
   222,   223,   227,   231,   235,   237,   239,   241,   243,   244,
   246,   249,   251
};

static const short yyrhs[] = {    -1,
    56,    57,     0,    58,     0,    61,     0,    59,     0,    60,
     0,    62,     0,    63,     0,    89,     0,    75,     0,    76,
     0,    77,     0,    78,     0,     7,    79,    82,    82,    64,
    65,    73,    66,     3,     0,     9,    79,    81,    81,    64,
    65,    73,     3,     0,    10,    79,    81,    81,    64,     3,
     0,     8,    79,    81,    81,    64,    65,    73,     3,     0,
    11,    64,     3,     0,    12,    64,     3,     0,     0,    14,
     0,    15,     0,    16,     0,    17,     0,    17,    81,     0,
    18,     0,    43,     0,    45,     0,    67,     0,    68,     0,
    69,     0,    21,    70,    20,    71,     0,    21,    70,     0,
    20,    71,     0,    23,    38,     0,    23,    38,    24,     0,
    34,     0,    33,    72,     0,    37,     0,    35,    72,     0,
    36,    72,     0,    31,    72,     0,    32,     0,    44,     0,
    45,     0,     0,    73,    74,     0,    28,    29,     0,    28,
    30,     0,    25,    26,     0,    25,    47,     0,    27,    43,
     0,    22,    43,     0,    39,    43,     0,    40,    43,     0,
    41,    43,     0,    42,    43,     0,    48,    79,    46,    83,
    84,    46,    83,    84,    85,    86,    87,     3,     0,    48,
    55,    44,    87,     3,     0,    49,    79,    46,    83,    84,
    46,    83,    84,    85,    86,    87,     3,     0,    50,     3,
     0,    51,     3,     0,     0,    79,    80,     0,    54,     0,
    46,     0,    46,     0,    46,    84,     0,     0,     4,    43,
     0,     0,     5,    47,     6,     0,     5,    43,     6,     0,
     5,    46,     6,     0,    43,     0,    47,     0,    18,     0,
    46,     0,     0,    46,     0,    52,    88,     0,    53,     0,
    13,     3,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   144,   145,   153,   154,   155,   156,   157,   158,   159,   160,
   161,   162,   163,   169,   181,   200,   212,   227,   237,   246,
   250,   258,   266,   270,   277,   284,   293,   294,   315,   316,
   317,   321,   322,   326,   330,   338,   350,   365,   380,   396,
   417,   441,   466,   479,   483,   513,   514,   518,   519,   520,
   521,   522,   523,   532,   533,   534,   535,   541,   580,   592,
   629,   640,   651,   652,   656,   681,   692,   700,   712,   713,
   717,   726,   735,   746,   753,   754,   755,   758,   780,   785,
   797,   821,   826
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","EOT","SLASH",
"BLCL","ELCL","ADD","GET","DELETE","DELETEALL","FLUSH","DUMP","EXIT","PR_ESP",
"PR_AH","PR_IPCOMP","PR_ESPUDP","PR_TCP","F_PROTOCOL","F_AUTH","F_ENC","F_REPLAY",
"F_COMP","F_RAWCPI","F_MODE","MODE","F_REQID","F_EXT","EXTENSION","NOCYCLICSEQ",
"ALG_AUTH","ALG_AUTH_NOKEY","ALG_ENC","ALG_ENC_NOKEY","ALG_ENC_DESDERIV","ALG_ENC_DES32IV",
"ALG_ENC_OLD","ALG_COMP","F_LIFETIME_HARD","F_LIFETIME_SOFT","F_LIFEBYTE_HARD",
"F_LIFEBYTE_SOFT","DECSTRING","QUOTEDSTRING","HEXSTRING","STRING","ANY","SPDADD",
"SPDDELETE","SPDDUMP","SPDFLUSH","F_POLICY","PL_REQUESTS","F_AIFLAGS","TAGGED",
"commands","command","add_command","delete_command","deleteall_command","get_command",
"flush_command","dump_command","protocol_spec","spi","algorithm_spec","esp_spec",
"ah_spec","ipcomp_spec","enc_alg","auth_alg","key_string","extension_spec","extension",
"spdadd_command","spddelete_command","spddump_command","spdflush_command","ipaddropts",
"ipaddropt","ipaddr","ipandport","prefix","portstr","upper_spec","upper_misc_spec",
"policy_spec","policy_requests","exit_command", NULL
};
#endif

static const short yyr1[] = {     0,
    56,    56,    57,    57,    57,    57,    57,    57,    57,    57,
    57,    57,    57,    58,    59,    60,    61,    62,    63,    64,
    64,    64,    64,    64,    64,    64,    65,    65,    66,    66,
    66,    67,    67,    68,    69,    69,    70,    70,    70,    70,
    70,    71,    71,    72,    72,    73,    73,    74,    74,    74,
    74,    74,    74,    74,    74,    74,    74,    75,    75,    76,
    77,    78,    79,    79,    80,    81,    82,    82,    83,    83,
    84,    84,    84,    84,    85,    85,    85,    85,    86,    86,
    87,    88,    89
};

static const short yyr2[] = {     0,
     0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     9,     8,     6,     8,     3,     3,     0,
     1,     1,     1,     1,     2,     1,     1,     1,     1,     1,
     1,     4,     2,     2,     2,     3,     1,     2,     1,     2,
     2,     2,     1,     1,     1,     0,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,    12,     5,    12,
     2,     2,     0,     2,     1,     1,     1,     2,     0,     2,
     0,     3,     3,     3,     1,     1,     1,     1,     0,     1,
     2,     1,     2
};

static const short yydefact[] = {     1,
     0,    63,    63,    63,    63,    20,    20,     0,    63,    63,
     0,     0,     2,     3,     5,     6,     4,     7,     8,    10,
    11,    12,    13,     9,     0,     0,     0,     0,    21,    22,
    23,    24,    26,     0,     0,    83,     0,     0,     0,    61,
    62,    67,    65,    64,     0,    66,     0,     0,     0,    25,
    18,    19,     0,    69,    69,     0,    68,    20,    20,    20,
    20,     0,     0,     0,    71,    71,     0,     0,     0,     0,
     0,     0,     0,    82,    81,    59,    70,     0,     0,    73,
    74,    72,    27,    28,    46,    46,    46,    16,    69,    69,
     0,     0,     0,    71,    71,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,    29,    30,    31,
    47,    17,    15,     0,     0,     0,    43,    34,     0,    37,
     0,     0,    39,    33,    53,    35,    50,    51,    52,    48,
    49,    54,    55,    56,    57,    14,    77,    75,    78,    76,
    79,    79,    44,    45,    42,    38,    40,    41,     0,    36,
    80,     0,     0,    32,     0,     0,    58,    60,     0,     0
};

static const short yydefgoto[] = {     1,
    13,    14,    15,    16,    17,    18,    19,    34,    85,   107,
   108,   109,   110,   124,   118,   145,    91,   111,    20,    21,
    22,    23,    25,    44,    47,    45,    65,    57,   141,   152,
    63,    75,    24
};

static const short yypact[] = {-32768,
     7,-32768,-32768,-32768,-32768,    32,    32,    67,   -45,-32768,
    86,    88,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,   -41,    14,    14,    14,-32768,-32768,
-32768,    15,-32768,   111,   112,-32768,    74,    33,    34,-32768,
-32768,   114,-32768,-32768,    70,-32768,    15,    15,    15,-32768,
-32768,-32768,   -15,   113,   113,    58,-32768,    32,    32,    32,
    32,    68,   117,    79,   114,   114,   118,   119,   120,    47,
    47,    47,   124,-32768,-32768,-32768,-32768,    77,    82,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,   113,   113,
    44,    -1,     3,   114,   114,    -9,    63,    87,    91,   -18,
    89,    73,    90,    92,    93,    94,   128,-32768,-32768,-32768,
-32768,-32768,-32768,    16,    16,    62,-32768,-32768,    62,-32768,
    62,    62,-32768,   121,-32768,   110,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
    96,    96,-32768,-32768,-32768,-32768,-32768,-32768,    -9,-32768,
-32768,   -15,   -15,-32768,   135,   136,-32768,-32768,   140,-32768
};

static const short yypgoto[] = {-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,    -7,    37,-32768,
-32768,-32768,-32768,-32768,    -6,  -110,    24,-32768,-32768,-32768,
-32768,-32768,    72,-32768,    46,    99,   -54,   -62,    30,     4,
   -40,-32768,-32768
};


#define	YYLAST		146


static const short yytable[] = {    35,
    66,   112,    78,    79,    42,   113,   159,   127,   146,    37,
   147,   148,    43,     2,     3,     4,     5,     6,     7,     8,
    98,   116,   117,   100,    98,   101,   102,   100,   128,   101,
   102,   114,   115,   137,    94,    95,    62,   103,   104,   105,
   106,   103,   104,   105,   106,    29,    30,    31,    32,    33,
    70,    71,    72,    73,     9,    10,    11,    12,   138,    46,
    46,   139,   140,    96,    97,    98,    99,    43,   100,    36,
   101,   102,    48,    49,    26,    27,    28,    50,    54,    55,
    38,    39,   103,   104,   105,   106,    43,    43,    40,    83,
    41,    84,    59,    60,    61,   119,   120,   121,   122,   123,
    67,   130,   131,    68,    69,   143,   144,    86,    87,    92,
    93,   155,   156,    51,    52,    42,    64,    53,    56,    76,
    74,    77,    89,    80,    81,    82,    88,    90,   126,   125,
   136,   129,   132,   150,   133,   134,   135,   157,   158,   160,
   149,   151,   154,    58,   142,   153
};

static const short yycheck[] = {     7,
    55,     3,    65,    66,    46,     3,     0,    26,   119,    55,
   121,   122,    54,     7,     8,     9,    10,    11,    12,    13,
    22,    31,    32,    25,    22,    27,    28,    25,    47,    27,
    28,    94,    95,    18,    89,    90,    52,    39,    40,    41,
    42,    39,    40,    41,    42,    14,    15,    16,    17,    18,
    58,    59,    60,    61,    48,    49,    50,    51,    43,    46,
    46,    46,    47,    20,    21,    22,    23,    54,    25,     3,
    27,    28,    27,    28,     3,     4,     5,    32,    46,    46,
     9,    10,    39,    40,    41,    42,    54,    54,     3,    43,
     3,    45,    47,    48,    49,    33,    34,    35,    36,    37,
    43,    29,    30,    46,    47,    44,    45,    71,    72,    86,
    87,   152,   153,     3,     3,    46,     4,    44,     5,     3,
    53,    43,    46,     6,     6,     6,     3,    46,    38,    43,
     3,    43,    43,    24,    43,    43,    43,     3,     3,     0,
    20,    46,   149,    45,   115,   142
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

case 2:
#line 146 "parse.y"
{
			free_buffer();
			parse_init();
		;
    break;}
case 14:
#line 170 "parse.y"
{
			int status;

			status = setkeymsg_add(SADB_ADD, yyvsp[-4].num, yyvsp[-6].res, yyvsp[-5].res);
			if (status < 0)
				return -1;
		;
    break;}
case 15:
#line 182 "parse.y"
{
			int status;

			if (yyvsp[-5].res->ai_next || yyvsp[-4].res->ai_next) {
				yyerror("multiple address specified");
				return -1;
			}
			if (p_mode != IPSEC_MODE_ANY)
				yyerror("WARNING: mode is obsolete");

			status = setkeymsg_addr(SADB_DELETE, yyvsp[-3].num, yyvsp[-5].res, yyvsp[-4].res, 0);
			if (status < 0)
				return -1;
		;
    break;}
case 16:
#line 201 "parse.y"
{
			int status;

			status = setkeymsg_addr(SADB_DELETE, yyvsp[-1].num, yyvsp[-3].res, yyvsp[-2].res, 1);
			if (status < 0)
				return -1;
		;
    break;}
case 17:
#line 213 "parse.y"
{
			int status;

			if (p_mode != IPSEC_MODE_ANY)
				yyerror("WARNING: mode is obsolete");

			status = setkeymsg_addr(SADB_GET, yyvsp[-3].num, yyvsp[-5].res, yyvsp[-4].res, 0);
			if (status < 0)
				return -1;
		;
    break;}
case 18:
#line 228 "parse.y"
{
			struct sadb_msg msg;
			setkeymsg0(&msg, SADB_FLUSH, yyvsp[-1].num, sizeof(msg));
			sendkeymsg((char *)&msg, sizeof(msg));
		;
    break;}
case 19:
#line 238 "parse.y"
{
			struct sadb_msg msg;
			setkeymsg0(&msg, SADB_DUMP, yyvsp[-1].num, sizeof(msg));
			sendkeymsg((char *)&msg, sizeof(msg));
		;
    break;}
case 20:
#line 247 "parse.y"
{
			yyval.num = SADB_SATYPE_UNSPEC;
		;
    break;}
case 21:
#line 251 "parse.y"
{
			yyval.num = SADB_SATYPE_ESP;
			if (yyvsp[0].num == 1)
				p_ext |= SADB_X_EXT_OLD;
			else
				p_ext &= ~SADB_X_EXT_OLD;
		;
    break;}
case 22:
#line 259 "parse.y"
{
			yyval.num = SADB_SATYPE_AH;
			if (yyvsp[0].num == 1)
				p_ext |= SADB_X_EXT_OLD;
			else
				p_ext &= ~SADB_X_EXT_OLD;
		;
    break;}
case 23:
#line 267 "parse.y"
{
			yyval.num = SADB_X_SATYPE_IPCOMP;
		;
    break;}
case 24:
#line 271 "parse.y"
{
			yyval.num = SADB_SATYPE_ESP;
			p_ext &= ~SADB_X_EXT_OLD;
			p_natt_oa = 0;
			p_natt_type = UDP_ENCAP_ESPINUDP;
		;
    break;}
case 25:
#line 278 "parse.y"
{
			yyval.num = SADB_SATYPE_ESP;
			p_ext &= ~SADB_X_EXT_OLD;
			p_natt_oa = yyvsp[0].res;
			p_natt_type = UDP_ENCAP_ESPINUDP;
		;
    break;}
case 26:
#line 285 "parse.y"
{
#ifdef SADB_X_SATYPE_TCPSIGNATURE
			yyval.num = SADB_X_SATYPE_TCPSIGNATURE;
#endif
		;
    break;}
case 27:
#line 293 "parse.y"
{ p_spi = yyvsp[0].ulnum; ;
    break;}
case 28:
#line 295 "parse.y"
{
			char *ep;
			unsigned long v;

			ep = NULL;
			v = strtoul(yyvsp[0].val.buf, &ep, 16);
			if (!ep || *ep) {
				yyerror("invalid SPI");
				return -1;
			}
			if (v & ~0xffffffff) {
				yyerror("SPI too big.");
				return -1;
			}

			p_spi = v;
		;
    break;}
case 35:
#line 331 "parse.y"
{
			if (yyvsp[0].num < 0) {
				yyerror("unsupported algorithm");
				return -1;
			}
			p_alg_enc = yyvsp[0].num;
		;
    break;}
case 36:
#line 339 "parse.y"
{
			if (yyvsp[-1].num < 0) {
				yyerror("unsupported algorithm");
				return -1;
			}
			p_alg_enc = yyvsp[-1].num;
			p_ext |= SADB_X_EXT_RAWCPI;
		;
    break;}
case 37:
#line 350 "parse.y"
{
			if (yyvsp[0].num < 0) {
				yyerror("unsupported algorithm");
				return -1;
			}
			p_alg_enc = yyvsp[0].num;

			p_key_enc_len = 0;
			p_key_enc = "";
			if (ipsec_check_keylen(SADB_EXT_SUPPORTED_ENCRYPT,
			    p_alg_enc, PFKEY_UNUNIT64(p_key_enc_len)) < 0) {
				yyerror(ipsec_strerror());
				return -1;
			}
		;
    break;}
case 38:
#line 365 "parse.y"
{
			if (yyvsp[-1].num < 0) {
				yyerror("unsupported algorithm");
				return -1;
			}
			p_alg_enc = yyvsp[-1].num;

			p_key_enc_len = yyvsp[0].val.len;
			p_key_enc = yyvsp[0].val.buf;
			if (ipsec_check_keylen(SADB_EXT_SUPPORTED_ENCRYPT,
			    p_alg_enc, PFKEY_UNUNIT64(p_key_enc_len)) < 0) {
				yyerror(ipsec_strerror());
				return -1;
			}
		;
    break;}
case 39:
#line 380 "parse.y"
{
			if (yyvsp[0].num < 0) {
				yyerror("unsupported algorithm");
				return -1;
			}
			yyerror("WARNING: obsolete algorithm");
			p_alg_enc = yyvsp[0].num;

			p_key_enc_len = 0;
			p_key_enc = "";
			if (ipsec_check_keylen(SADB_EXT_SUPPORTED_ENCRYPT,
			    p_alg_enc, PFKEY_UNUNIT64(p_key_enc_len)) < 0) {
				yyerror(ipsec_strerror());
				return -1;
			}
		;
    break;}
case 40:
#line 397 "parse.y"
{
			if (yyvsp[-1].num < 0) {
				yyerror("unsupported algorithm");
				return -1;
			}
			p_alg_enc = yyvsp[-1].num;
			if (p_ext & SADB_X_EXT_OLD) {
				yyerror("algorithm mismatched");
				return -1;
			}
			p_ext |= SADB_X_EXT_DERIV;

			p_key_enc_len = yyvsp[0].val.len;
			p_key_enc = yyvsp[0].val.buf;
			if (ipsec_check_keylen(SADB_EXT_SUPPORTED_ENCRYPT,
			    p_alg_enc, PFKEY_UNUNIT64(p_key_enc_len)) < 0) {
				yyerror(ipsec_strerror());
				return -1;
			}
		;
    break;}
case 41:
#line 418 "parse.y"
{
			if (yyvsp[-1].num < 0) {
				yyerror("unsupported algorithm");
				return -1;
			}
			p_alg_enc = yyvsp[-1].num;
			if (!(p_ext & SADB_X_EXT_OLD)) {
				yyerror("algorithm mismatched");
				return -1;
			}
			p_ext |= SADB_X_EXT_IV4B;

			p_key_enc_len = yyvsp[0].val.len;
			p_key_enc = yyvsp[0].val.buf;
			if (ipsec_check_keylen(SADB_EXT_SUPPORTED_ENCRYPT,
			    p_alg_enc, PFKEY_UNUNIT64(p_key_enc_len)) < 0) {
				yyerror(ipsec_strerror());
				return -1;
			}
		;
    break;}
case 42:
#line 441 "parse.y"
{
			if (yyvsp[-1].num < 0) {
				yyerror("unsupported algorithm");
				return -1;
			}
			p_alg_auth = yyvsp[-1].num;

			p_key_auth_len = yyvsp[0].val.len;
			p_key_auth = yyvsp[0].val.buf;
#ifdef SADB_X_AALG_TCP_MD5
			if (p_alg_auth == SADB_X_AALG_TCP_MD5) {
				if ((p_key_auth_len < 1) || 
				    (p_key_auth_len > 80))
					return -1;
			} else 
#endif
			{
				if (ipsec_check_keylen(SADB_EXT_SUPPORTED_AUTH,
				    p_alg_auth, 
				    PFKEY_UNUNIT64(p_key_auth_len)) < 0) {
					yyerror(ipsec_strerror());
					return -1;
				}
			}
		;
    break;}
case 43:
#line 466 "parse.y"
{
			if (yyvsp[0].num < 0) {
				yyerror("unsupported algorithm");
				return -1;
			}
			p_alg_auth = yyvsp[0].num;

			p_key_auth_len = 0;
			p_key_auth = NULL;
		;
    break;}
case 44:
#line 480 "parse.y"
{
			yyval.val = yyvsp[0].val;
		;
    break;}
case 45:
#line 484 "parse.y"
{
			caddr_t pp_key;
			caddr_t bp;
			caddr_t yp = yyvsp[0].val.buf;
			int l;

			l = strlen(yp) % 2 + strlen(yp) / 2;
			if ((pp_key = malloc(l)) == 0) {
				yyerror("not enough core");
				return -1;
			}
			memset(pp_key, 0, l);

			bp = pp_key;
			if (strlen(yp) % 2) {
				*bp = ATOX(yp[0]);
				yp++, bp++;
			}
			while (*yp) {
				*bp = (ATOX(yp[0]) << 4) | ATOX(yp[1]);
				yp += 2, bp++;
			}

			yyval.val.len = l;
			yyval.val.buf = pp_key;
		;
    break;}
case 48:
#line 518 "parse.y"
{ p_ext |= yyvsp[0].num; ;
    break;}
case 49:
#line 519 "parse.y"
{ p_ext &= ~SADB_X_EXT_CYCSEQ; ;
    break;}
case 50:
#line 520 "parse.y"
{ p_mode = yyvsp[0].num; ;
    break;}
case 51:
#line 521 "parse.y"
{ p_mode = IPSEC_MODE_ANY; ;
    break;}
case 52:
#line 522 "parse.y"
{ p_reqid = yyvsp[0].ulnum; ;
    break;}
case 53:
#line 524 "parse.y"
{
			if ((p_ext & SADB_X_EXT_OLD) != 0) {
				yyerror("replay prevention cannot be used with "
				    "ah/esp-old");
				return -1;
			}
			p_replay = yyvsp[0].ulnum;
		;
    break;}
case 54:
#line 532 "parse.y"
{ p_lt_hard = yyvsp[0].ulnum; ;
    break;}
case 55:
#line 533 "parse.y"
{ p_lt_soft = yyvsp[0].ulnum; ;
    break;}
case 56:
#line 534 "parse.y"
{ p_lb_hard = yyvsp[0].ulnum; ;
    break;}
case 57:
#line 535 "parse.y"
{ p_lb_soft = yyvsp[0].ulnum; ;
    break;}
case 58:
#line 542 "parse.y"
{
			int status;
			struct addrinfo *src, *dst;

#ifdef HAVE_PFKEY_POLICY_PRIORITY
			last_msg_type = SADB_X_SPDADD;
#endif

			/* fixed port fields if ulp is icmpv6 */
			if (yyvsp[-2].val.buf != NULL) {
				if (yyvsp[-3].num != IPPROTO_ICMPV6)
					return -1;
				free(yyvsp[-7].val.buf);
				free(yyvsp[-4].val.buf);
				if (fix_portstr(&yyvsp[-2].val, &yyvsp[-7].val, &yyvsp[-4].val))
					return -1;
			}

			src = parse_addr(yyvsp[-9].val.buf, yyvsp[-7].val.buf);
			dst = parse_addr(yyvsp[-6].val.buf, yyvsp[-4].val.buf);
			if (!src || !dst) {
				/* yyerror is already called */
				return -1;
			}
			if (src->ai_next || dst->ai_next) {
				yyerror("multiple address specified");
				freeaddrinfo(src);
				freeaddrinfo(dst);
				return -1;
			}

			status = setkeymsg_spdaddr(SADB_X_SPDADD, yyvsp[-3].num, &yyvsp[-1].val,
			    src, yyvsp[-8].num, dst, yyvsp[-5].num);
			freeaddrinfo(src);
			freeaddrinfo(dst);
			if (status < 0)
				return -1;
		;
    break;}
case 59:
#line 581 "parse.y"
{
			int status;

			status = setkeymsg_spdaddr_tag(SADB_X_SPDADD,
			    yyvsp[-2].val.buf, &yyvsp[-1].val);
			if (status < 0)
				return -1;
		;
    break;}
case 60:
#line 593 "parse.y"
{
			int status;
			struct addrinfo *src, *dst;

			/* fixed port fields if ulp is icmpv6 */
			if (yyvsp[-2].val.buf != NULL) {
				if (yyvsp[-3].num != IPPROTO_ICMPV6)
					return -1;
				free(yyvsp[-7].val.buf);
				free(yyvsp[-4].val.buf);
				if (fix_portstr(&yyvsp[-2].val, &yyvsp[-7].val, &yyvsp[-4].val))
					return -1;
			}

			src = parse_addr(yyvsp[-9].val.buf, yyvsp[-7].val.buf);
			dst = parse_addr(yyvsp[-6].val.buf, yyvsp[-4].val.buf);
			if (!src || !dst) {
				/* yyerror is already called */
				return -1;
			}
			if (src->ai_next || dst->ai_next) {
				yyerror("multiple address specified");
				freeaddrinfo(src);
				freeaddrinfo(dst);
				return -1;
			}

			status = setkeymsg_spdaddr(SADB_X_SPDDELETE, yyvsp[-3].num, &yyvsp[-1].val,
			    src, yyvsp[-8].num, dst, yyvsp[-5].num);
			freeaddrinfo(src);
			freeaddrinfo(dst);
			if (status < 0)
				return -1;
		;
    break;}
case 61:
#line 631 "parse.y"
{
			struct sadb_msg msg;
			setkeymsg0(&msg, SADB_X_SPDDUMP, SADB_SATYPE_UNSPEC,
			    sizeof(msg));
			sendkeymsg((char *)&msg, sizeof(msg));
		;
    break;}
case 62:
#line 642 "parse.y"
{
			struct sadb_msg msg;
			setkeymsg0(&msg, SADB_X_SPDFLUSH, SADB_SATYPE_UNSPEC,
			    sizeof(msg));
			sendkeymsg((char *)&msg, sizeof(msg));
		;
    break;}
case 65:
#line 657 "parse.y"
{
			char *p;

			for (p = yyvsp[0].val.buf + 1; *p; p++)
				switch (*p) {
				case '4':
					p_aifamily = AF_INET;
					break;
#ifdef INET6
				case '6':
					p_aifamily = AF_INET6;
					break;
#endif
				case 'n':
					p_aiflags = AI_NUMERICHOST;
					break;
				default:
					yyerror("invalid flag");
					return -1;
				}
		;
    break;}
case 66:
#line 682 "parse.y"
{
			yyval.res = parse_addr(yyvsp[0].val.buf, NULL);
			if (yyval.res == NULL) {
				/* yyerror already called by parse_addr */
				return -1;
			}
		;
    break;}
case 67:
#line 693 "parse.y"
{
			yyval.res = parse_addr(yyvsp[0].val.buf, NULL);
			if (yyval.res == NULL) {
				/* yyerror already called by parse_addr */
				return -1;
			}
		;
    break;}
case 68:
#line 701 "parse.y"
{
			yyval.res = parse_addr(yyvsp[-1].val.buf, yyvsp[0].val.buf);
			if (yyval.res == NULL) {
				/* yyerror already called by parse_addr */
				return -1;
			}
		;
    break;}
case 69:
#line 712 "parse.y"
{ yyval.num = -1; ;
    break;}
case 70:
#line 713 "parse.y"
{ yyval.num = yyvsp[0].ulnum; ;
    break;}
case 71:
#line 718 "parse.y"
{
			yyval.val.buf = strdup("0");
			if (!yyval.val.buf) {
				yyerror("insufficient memory");
				return -1;
			}
			yyval.val.len = strlen(yyval.val.buf);
		;
    break;}
case 72:
#line 727 "parse.y"
{
			yyval.val.buf = strdup("0");
			if (!yyval.val.buf) {
				yyerror("insufficient memory");
				return -1;
			}
			yyval.val.len = strlen(yyval.val.buf);
		;
    break;}
case 73:
#line 736 "parse.y"
{
			char buf[20];
			snprintf(buf, sizeof(buf), "%lu", yyvsp[-1].ulnum);
			yyval.val.buf = strdup(buf);
			if (!yyval.val.buf) {
				yyerror("insufficient memory");
				return -1;
			}
			yyval.val.len = strlen(yyval.val.buf);
		;
    break;}
case 74:
#line 747 "parse.y"
{
			yyval.val = yyvsp[-1].val;
		;
    break;}
case 75:
#line 753 "parse.y"
{ yyval.num = yyvsp[0].ulnum; ;
    break;}
case 76:
#line 754 "parse.y"
{ yyval.num = IPSEC_ULPROTO_ANY; ;
    break;}
case 77:
#line 755 "parse.y"
{ 
				yyval.num = IPPROTO_TCP; 
			;
    break;}
case 78:
#line 759 "parse.y"
{
			struct protoent *ent;

			ent = getprotobyname(yyvsp[0].val.buf);
			if (ent)
				yyval.num = ent->p_proto;
			else {
				if (strcmp("icmp6", yyvsp[0].val.buf) == 0) {
					yyval.num = IPPROTO_ICMPV6;
				} else if(strcmp("ip4", yyvsp[0].val.buf) == 0) {
					yyval.num = IPPROTO_IPV4;
				} else {
					yyerror("invalid upper layer protocol");
					return -1;
				}
			}
			endprotoent();
		;
    break;}
case 79:
#line 781 "parse.y"
{
			yyval.val.buf = NULL;
			yyval.val.len = 0;
		;
    break;}
case 80:
#line 786 "parse.y"
{
			yyval.val.buf = strdup(yyvsp[0].val.buf);
			if (!yyval.val.buf) {
				yyerror("insufficient memory");
				return -1;
			}
			yyval.val.len = strlen(yyval.val.buf);
		;
    break;}
case 81:
#line 798 "parse.y"
{
			char *policy;
#ifdef HAVE_PFKEY_POLICY_PRIORITY
			struct sadb_x_policy *xpl;
#endif

			policy = ipsec_set_policy(yyvsp[0].val.buf, yyvsp[0].val.len);
			if (policy == NULL) {
				yyerror(ipsec_strerror());
				return -1;
			}

			yyval.val.buf = policy;
			yyval.val.len = ipsec_get_policylen(policy);

#ifdef HAVE_PFKEY_POLICY_PRIORITY
			xpl = (struct sadb_x_policy *) yyval.val.buf;
			last_priority = xpl->sadb_x_policy_priority;
#endif
		;
    break;}
case 82:
#line 821 "parse.y"
{ yyval.val = yyvsp[0].val; ;
    break;}
case 83:
#line 827 "parse.y"
{
			exit_now = 1;
			YYACCEPT;
		;
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
#line 832 "parse.y"


int
setkeymsg0(msg, type, satype, l)
	struct sadb_msg *msg;
	unsigned int type;
	unsigned int satype;
	size_t l;
{

	msg->sadb_msg_version = PF_KEY_V2;
	msg->sadb_msg_type = type;
	msg->sadb_msg_errno = 0;
	msg->sadb_msg_satype = satype;
	msg->sadb_msg_reserved = 0;
	msg->sadb_msg_seq = 0;
	msg->sadb_msg_pid = getpid();
	msg->sadb_msg_len = PFKEY_UNIT64(l);
	return 0;
}

/* XXX NO BUFFER OVERRUN CHECK! BAD BAD! */
static int
setkeymsg_spdaddr(type, upper, policy, srcs, splen, dsts, dplen)
	unsigned int type;
	unsigned int upper;
	vchar_t *policy;
	struct addrinfo *srcs;
	int splen;
	struct addrinfo *dsts;
	int dplen;
{
	struct sadb_msg *msg;
	char buf[BUFSIZ];
	int l, l0;
	struct sadb_address m_addr;
	struct addrinfo *s, *d;
	int n;
	int plen;
	struct sockaddr *sa;
	int salen;
	struct sadb_x_policy *sp;
#ifdef HAVE_POLICY_FWD
	struct sadb_x_ipsecrequest *ps = NULL;
	int saved_level, saved_id = 0;
#endif

	msg = (struct sadb_msg *)buf;

	if (!srcs || !dsts)
		return -1;

	/* fix up length afterwards */
	setkeymsg0(msg, type, SADB_SATYPE_UNSPEC, 0);
	l = sizeof(struct sadb_msg);

	sp = (struct sadb_x_policy*) (buf + l);
	memcpy(buf + l, policy->buf, policy->len);
	l += policy->len;

	l0 = l;
	n = 0;

	/* do it for all src/dst pairs */
	for (s = srcs; s; s = s->ai_next) {
		for (d = dsts; d; d = d->ai_next) {
			/* rewind pointer */
			l = l0;

			if (s->ai_addr->sa_family != d->ai_addr->sa_family)
				continue;
			switch (s->ai_addr->sa_family) {
			case AF_INET:
				plen = sizeof(struct in_addr) << 3;
				break;
#ifdef INET6
			case AF_INET6:
				plen = sizeof(struct in6_addr) << 3;
				break;
#endif
			default:
				continue;
			}

			/* set src */
			sa = s->ai_addr;
			salen = sysdep_sa_len(s->ai_addr);
			m_addr.sadb_address_len = PFKEY_UNIT64(sizeof(m_addr) +
			    PFKEY_ALIGN8(salen));
			m_addr.sadb_address_exttype = SADB_EXT_ADDRESS_SRC;
			m_addr.sadb_address_proto = upper;
			m_addr.sadb_address_prefixlen =
			    (splen >= 0 ? splen : plen);
			m_addr.sadb_address_reserved = 0;

			setvarbuf(buf, &l, (struct sadb_ext *)&m_addr,
			    sizeof(m_addr), (caddr_t)sa, salen);

			/* set dst */
			sa = d->ai_addr;
			salen = sysdep_sa_len(d->ai_addr);
			m_addr.sadb_address_len = PFKEY_UNIT64(sizeof(m_addr) +
			    PFKEY_ALIGN8(salen));
			m_addr.sadb_address_exttype = SADB_EXT_ADDRESS_DST;
			m_addr.sadb_address_proto = upper;
			m_addr.sadb_address_prefixlen =
			    (dplen >= 0 ? dplen : plen);
			m_addr.sadb_address_reserved = 0;

			setvarbuf(buf, &l, (struct sadb_ext *)&m_addr,
			    sizeof(m_addr), sa, salen);

			msg->sadb_msg_len = PFKEY_UNIT64(l);

			sendkeymsg(buf, l);

#ifdef HAVE_POLICY_FWD
			/* create extra call for FWD policy */
			if (f_rfcmode && sp->sadb_x_policy_dir == IPSEC_DIR_INBOUND) {
				sp->sadb_x_policy_dir = IPSEC_DIR_FWD;
				ps = (struct sadb_x_ipsecrequest*) (sp+1);

				/* if request level is unique, change it to
				 * require for fwd policy */
				/* XXX: currently, only first policy is updated
				 * only. Update following too... */
				saved_level = ps->sadb_x_ipsecrequest_level;
				if (saved_level == IPSEC_LEVEL_UNIQUE) {
					saved_id = ps->sadb_x_ipsecrequest_reqid;
					ps->sadb_x_ipsecrequest_reqid=0;
					ps->sadb_x_ipsecrequest_level=IPSEC_LEVEL_REQUIRE;
				}

				sendkeymsg(buf, l);
				/* restoring for next message */
				sp->sadb_x_policy_dir = IPSEC_DIR_INBOUND;
				if (saved_level == IPSEC_LEVEL_UNIQUE) {
					ps->sadb_x_ipsecrequest_reqid = saved_id;
					ps->sadb_x_ipsecrequest_level = saved_level;
				}
			}
#endif

			n++;
		}
	}

	if (n == 0)
		return -1;
	else
		return 0;
}

static int
setkeymsg_spdaddr_tag(type, tag, policy)
	unsigned int type;
	char *tag;
	vchar_t *policy;
{
	struct sadb_msg *msg;
	char buf[BUFSIZ];
	int l, l0;
#ifdef SADB_X_EXT_TAG
	struct sadb_x_tag m_tag;
#endif
	int n;

	msg = (struct sadb_msg *)buf;

	/* fix up length afterwards */
	setkeymsg0(msg, type, SADB_SATYPE_UNSPEC, 0);
	l = sizeof(struct sadb_msg);

	memcpy(buf + l, policy->buf, policy->len);
	l += policy->len;

	l0 = l;
	n = 0;

#ifdef SADB_X_EXT_TAG
	memset(&m_tag, 0, sizeof(m_tag));
	m_tag.sadb_x_tag_len = PFKEY_UNIT64(sizeof(m_tag));
	m_tag.sadb_x_tag_exttype = SADB_X_EXT_TAG;
	if (strlcpy(m_tag.sadb_x_tag_name, tag,
	    sizeof(m_tag.sadb_x_tag_name)) >= sizeof(m_tag.sadb_x_tag_name))
		return -1;
	memcpy(buf + l, &m_tag, sizeof(m_tag));
	l += sizeof(m_tag);
#endif

	msg->sadb_msg_len = PFKEY_UNIT64(l);

	sendkeymsg(buf, l);

	return 0;
}

/* XXX NO BUFFER OVERRUN CHECK! BAD BAD! */
static int
setkeymsg_addr(type, satype, srcs, dsts, no_spi)
	unsigned int type;
	unsigned int satype;
	struct addrinfo *srcs;
	struct addrinfo *dsts;
	int no_spi;
{
	struct sadb_msg *msg;
	char buf[BUFSIZ];
	int l, l0, len;
	struct sadb_sa m_sa;
	struct sadb_x_sa2 m_sa2;
	struct sadb_address m_addr;
	struct addrinfo *s, *d;
	int n;
	int plen;
	struct sockaddr *sa;
	int salen;

	msg = (struct sadb_msg *)buf;

	if (!srcs || !dsts)
		return -1;

	/* fix up length afterwards */
	setkeymsg0(msg, type, satype, 0);
	l = sizeof(struct sadb_msg);

	if (!no_spi) {
		len = sizeof(struct sadb_sa);
		m_sa.sadb_sa_len = PFKEY_UNIT64(len);
		m_sa.sadb_sa_exttype = SADB_EXT_SA;
		m_sa.sadb_sa_spi = htonl(p_spi);
		m_sa.sadb_sa_replay = p_replay;
		m_sa.sadb_sa_state = 0;
		m_sa.sadb_sa_auth = p_alg_auth;
		m_sa.sadb_sa_encrypt = p_alg_enc;
		m_sa.sadb_sa_flags = p_ext;

		memcpy(buf + l, &m_sa, len);
		l += len;

		len = sizeof(struct sadb_x_sa2);
		m_sa2.sadb_x_sa2_len = PFKEY_UNIT64(len);
		m_sa2.sadb_x_sa2_exttype = SADB_X_EXT_SA2;
		m_sa2.sadb_x_sa2_mode = p_mode;
		m_sa2.sadb_x_sa2_reqid = p_reqid;

		memcpy(buf + l, &m_sa2, len);
		l += len;
	}

	l0 = l;
	n = 0;

	/* do it for all src/dst pairs */
	for (s = srcs; s; s = s->ai_next) {
		for (d = dsts; d; d = d->ai_next) {
			/* rewind pointer */
			l = l0;

			if (s->ai_addr->sa_family != d->ai_addr->sa_family)
				continue;
			switch (s->ai_addr->sa_family) {
			case AF_INET:
				plen = sizeof(struct in_addr) << 3;
				break;
#ifdef INET6
			case AF_INET6:
				plen = sizeof(struct in6_addr) << 3;
				break;
#endif
			default:
				continue;
			}

			/* set src */
			sa = s->ai_addr;
			salen = sysdep_sa_len(s->ai_addr);
			m_addr.sadb_address_len = PFKEY_UNIT64(sizeof(m_addr) +
			    PFKEY_ALIGN8(salen));
			m_addr.sadb_address_exttype = SADB_EXT_ADDRESS_SRC;
			m_addr.sadb_address_proto = IPSEC_ULPROTO_ANY;
			m_addr.sadb_address_prefixlen = plen;
			m_addr.sadb_address_reserved = 0;

			setvarbuf(buf, &l, (struct sadb_ext *)&m_addr,
			    sizeof(m_addr), sa, salen);

			/* set dst */
			sa = d->ai_addr;
			salen = sysdep_sa_len(d->ai_addr);
			m_addr.sadb_address_len = PFKEY_UNIT64(sizeof(m_addr) +
			    PFKEY_ALIGN8(salen));
			m_addr.sadb_address_exttype = SADB_EXT_ADDRESS_DST;
			m_addr.sadb_address_proto = IPSEC_ULPROTO_ANY;
			m_addr.sadb_address_prefixlen = plen;
			m_addr.sadb_address_reserved = 0;

			setvarbuf(buf, &l, (struct sadb_ext *)&m_addr,
			    sizeof(m_addr), sa, salen);

			msg->sadb_msg_len = PFKEY_UNIT64(l);

			sendkeymsg(buf, l);

			n++;
		}
	}

	if (n == 0)
		return -1;
	else
		return 0;
}

#ifdef SADB_X_EXT_NAT_T_TYPE
static u_int16_t get_port (struct addrinfo *addr)
{
	struct sockaddr *s = addr->ai_addr;
	u_int16_t port = 0;

	switch (s->sa_family) {
	case AF_INET:
	  {
		struct sockaddr_in *sin4 = (struct sockaddr_in *)s;
		port = ntohs(sin4->sin_port);
		break;
	  }
	case AF_INET6:
	  {
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)s;
		port = ntohs(sin6->sin6_port);
		break;
	  }
	}

	if (port == 0)
		port = DEFAULT_NATT_PORT;

	return port;
}
#endif

/* XXX NO BUFFER OVERRUN CHECK! BAD BAD! */
static int
setkeymsg_add(type, satype, srcs, dsts)
	unsigned int type;
	unsigned int satype;
	struct addrinfo *srcs;
	struct addrinfo *dsts;
{
	struct sadb_msg *msg;
	char buf[BUFSIZ];
	int l, l0, len;
	struct sadb_sa m_sa;
	struct sadb_x_sa2 m_sa2;
	struct sadb_address m_addr;
	struct addrinfo *s, *d;
	int n;
	int plen;
	struct sockaddr *sa;
	int salen;

	msg = (struct sadb_msg *)buf;

	if (!srcs || !dsts)
		return -1;

	/* fix up length afterwards */
	setkeymsg0(msg, type, satype, 0);
	l = sizeof(struct sadb_msg);

	/* set encryption algorithm, if present. */
	if (satype != SADB_X_SATYPE_IPCOMP && p_key_enc) {
		union {
			struct sadb_key key;
			struct sadb_ext ext;
		} m;

		m.key.sadb_key_len =
			PFKEY_UNIT64(sizeof(m.key)
				   + PFKEY_ALIGN8(p_key_enc_len));
		m.key.sadb_key_exttype = SADB_EXT_KEY_ENCRYPT;
		m.key.sadb_key_bits = p_key_enc_len * 8;
		m.key.sadb_key_reserved = 0;

		setvarbuf(buf, &l, &m.ext, sizeof(m.key),
			p_key_enc, p_key_enc_len);
	}

	/* set authentication algorithm, if present. */
	if (p_key_auth) {
		union {
			struct sadb_key key;
			struct sadb_ext ext;
		} m;

		m.key.sadb_key_len =
			PFKEY_UNIT64(sizeof(m.key)
				   + PFKEY_ALIGN8(p_key_auth_len));
		m.key.sadb_key_exttype = SADB_EXT_KEY_AUTH;
		m.key.sadb_key_bits = p_key_auth_len * 8;
		m.key.sadb_key_reserved = 0;

		setvarbuf(buf, &l, &m.ext, sizeof(m.key),
			p_key_auth, p_key_auth_len);
	}

	/* set lifetime for HARD */
	if (p_lt_hard != 0 || p_lb_hard != 0) {
		struct sadb_lifetime m_lt;
		u_int slen = sizeof(struct sadb_lifetime);

		m_lt.sadb_lifetime_len = PFKEY_UNIT64(slen);
		m_lt.sadb_lifetime_exttype = SADB_EXT_LIFETIME_HARD;
		m_lt.sadb_lifetime_allocations = 0;
		m_lt.sadb_lifetime_bytes = p_lb_hard;
		m_lt.sadb_lifetime_addtime = p_lt_hard;
		m_lt.sadb_lifetime_usetime = 0;

		memcpy(buf + l, &m_lt, slen);
		l += slen;
	}

	/* set lifetime for SOFT */
	if (p_lt_soft != 0 || p_lb_soft != 0) {
		struct sadb_lifetime m_lt;
		u_int slen = sizeof(struct sadb_lifetime);

		m_lt.sadb_lifetime_len = PFKEY_UNIT64(slen);
		m_lt.sadb_lifetime_exttype = SADB_EXT_LIFETIME_SOFT;
		m_lt.sadb_lifetime_allocations = 0;
		m_lt.sadb_lifetime_bytes = p_lb_soft;
		m_lt.sadb_lifetime_addtime = p_lt_soft;
		m_lt.sadb_lifetime_usetime = 0;

		memcpy(buf + l, &m_lt, slen);
		l += slen;
	}

	len = sizeof(struct sadb_sa);
	m_sa.sadb_sa_len = PFKEY_UNIT64(len);
	m_sa.sadb_sa_exttype = SADB_EXT_SA;
	m_sa.sadb_sa_spi = htonl(p_spi);
	m_sa.sadb_sa_replay = p_replay;
	m_sa.sadb_sa_state = 0;
	m_sa.sadb_sa_auth = p_alg_auth;
	m_sa.sadb_sa_encrypt = p_alg_enc;
	m_sa.sadb_sa_flags = p_ext;

	memcpy(buf + l, &m_sa, len);
	l += len;

	len = sizeof(struct sadb_x_sa2);
	m_sa2.sadb_x_sa2_len = PFKEY_UNIT64(len);
	m_sa2.sadb_x_sa2_exttype = SADB_X_EXT_SA2;
	m_sa2.sadb_x_sa2_mode = p_mode;
	m_sa2.sadb_x_sa2_reqid = p_reqid;

	memcpy(buf + l, &m_sa2, len);
	l += len;

#ifdef SADB_X_EXT_NAT_T_TYPE
	if (p_natt_type) {
		struct sadb_x_nat_t_type natt_type;

		len = sizeof(struct sadb_x_nat_t_type);
		memset(&natt_type, 0, len);
		natt_type.sadb_x_nat_t_type_len = PFKEY_UNIT64(len);
		natt_type.sadb_x_nat_t_type_exttype = SADB_X_EXT_NAT_T_TYPE;
		natt_type.sadb_x_nat_t_type_type = p_natt_type;

		memcpy(buf + l, &natt_type, len);
		l += len;

		if (p_natt_oa) {
			sa = p_natt_oa->ai_addr;
			switch (sa->sa_family) {
			case AF_INET:
				plen = sizeof(struct in_addr) << 3;
				break;
#ifdef INET6
			case AF_INET6:
				plen = sizeof(struct in6_addr) << 3;
				break;
#endif
			default:
				return -1;
			}
			salen = sysdep_sa_len(sa);
			m_addr.sadb_address_len = PFKEY_UNIT64(sizeof(m_addr) +
			    PFKEY_ALIGN8(salen));
			m_addr.sadb_address_exttype = SADB_X_EXT_NAT_T_OA;
			m_addr.sadb_address_proto = IPSEC_ULPROTO_ANY;
			m_addr.sadb_address_prefixlen = plen;
			m_addr.sadb_address_reserved = 0;

			setvarbuf(buf, &l, (struct sadb_ext *)&m_addr,
			    sizeof(m_addr), sa, salen);
		}
	}
#endif

	l0 = l;
	n = 0;

	/* do it for all src/dst pairs */
	for (s = srcs; s; s = s->ai_next) {
		for (d = dsts; d; d = d->ai_next) {
			/* rewind pointer */
			l = l0;

			if (s->ai_addr->sa_family != d->ai_addr->sa_family)
				continue;
			switch (s->ai_addr->sa_family) {
			case AF_INET:
				plen = sizeof(struct in_addr) << 3;
				break;
#ifdef INET6
			case AF_INET6:
				plen = sizeof(struct in6_addr) << 3;
				break;
#endif
			default:
				continue;
			}

			/* set src */
			sa = s->ai_addr;
			salen = sysdep_sa_len(s->ai_addr);
			m_addr.sadb_address_len = PFKEY_UNIT64(sizeof(m_addr) +
			    PFKEY_ALIGN8(salen));
			m_addr.sadb_address_exttype = SADB_EXT_ADDRESS_SRC;
			m_addr.sadb_address_proto = IPSEC_ULPROTO_ANY;
			m_addr.sadb_address_prefixlen = plen;
			m_addr.sadb_address_reserved = 0;

			setvarbuf(buf, &l, (struct sadb_ext *)&m_addr,
			    sizeof(m_addr), sa, salen);

			/* set dst */
			sa = d->ai_addr;
			salen = sysdep_sa_len(d->ai_addr);
			m_addr.sadb_address_len = PFKEY_UNIT64(sizeof(m_addr) +
			    PFKEY_ALIGN8(salen));
			m_addr.sadb_address_exttype = SADB_EXT_ADDRESS_DST;
			m_addr.sadb_address_proto = IPSEC_ULPROTO_ANY;
			m_addr.sadb_address_prefixlen = plen;
			m_addr.sadb_address_reserved = 0;

			setvarbuf(buf, &l, (struct sadb_ext *)&m_addr,
			    sizeof(m_addr), sa, salen);

#ifdef SADB_X_EXT_NAT_T_TYPE
			if (p_natt_type) {
				struct sadb_x_nat_t_port natt_port;

				/* NATT_SPORT */
				len = sizeof(struct sadb_x_nat_t_port);
				memset(&natt_port, 0, len);
				natt_port.sadb_x_nat_t_port_len = PFKEY_UNIT64(len);
				natt_port.sadb_x_nat_t_port_exttype =
					SADB_X_EXT_NAT_T_SPORT;
				natt_port.sadb_x_nat_t_port_port = htons(get_port(s));
				
				memcpy(buf + l, &natt_port, len);
				l += len;

				/* NATT_DPORT */
				natt_port.sadb_x_nat_t_port_exttype =
					SADB_X_EXT_NAT_T_DPORT;
				natt_port.sadb_x_nat_t_port_port = htons(get_port(d));
				
				memcpy(buf + l, &natt_port, len);
				l += len;
			}
#endif
			msg->sadb_msg_len = PFKEY_UNIT64(l);

			sendkeymsg(buf, l);

			n++;
		}
	}

	if (n == 0)
		return -1;
	else
		return 0;
}

static struct addrinfo *
parse_addr(host, port)
	char *host;
	char *port;
{
	struct addrinfo hints, *res = NULL;
	int error;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = p_aifamily;
	hints.ai_socktype = SOCK_DGRAM;		/*dummy*/
	hints.ai_protocol = IPPROTO_UDP;	/*dummy*/
	hints.ai_flags = p_aiflags;
	error = getaddrinfo(host, port, &hints, &res);
	if (error != 0) {
		yyerror(gai_strerror(error));
		return NULL;
	}
	return res;
}

static int
fix_portstr(spec, sport, dport)
	vchar_t *spec, *sport, *dport;
{
	const char *p, *p2 = "0";
	char *q;
	u_int l;

	l = 0;
	for (q = spec->buf; *q != ',' && *q != '\0' && l < spec->len; q++, l++)
		;
	if (*q != '\0') {
		if (*q == ',') {
			*q = '\0';
			p2 = ++q;
		}
		for (p = p2; *p != '\0' && l < spec->len; p++, l++)
			;
		if (*p != '\0' || *p2 == '\0') {
			yyerror("invalid an upper layer protocol spec");
			return -1;
		}
	}

	sport->buf = strdup(spec->buf);
	if (!sport->buf) {
		yyerror("insufficient memory");
		return -1;
	}
	sport->len = strlen(sport->buf);
	dport->buf = strdup(p2);
	if (!dport->buf) {
		yyerror("insufficient memory");
		return -1;
	}
	dport->len = strlen(dport->buf);

	return 0;
}

static int
setvarbuf(buf, off, ebuf, elen, vbuf, vlen)
	char *buf;
	int *off;
	struct sadb_ext *ebuf;
	int elen;
	const void *vbuf;
	int vlen;
{
	memset(buf + *off, 0, PFKEY_UNUNIT64(ebuf->sadb_ext_len));
	memcpy(buf + *off, (caddr_t)ebuf, elen);
	memcpy(buf + *off + elen, vbuf, vlen);
	(*off) += PFKEY_ALIGN8(elen + vlen);

	return 0;
}

void
parse_init()
{
	p_spi = 0;

	p_ext = SADB_X_EXT_CYCSEQ;
	p_alg_enc = SADB_EALG_NONE;
	p_alg_auth = SADB_AALG_NONE;
	p_mode = IPSEC_MODE_ANY;
	p_reqid = 0;
	p_replay = 0;
	p_key_enc_len = p_key_auth_len = 0;
	p_key_enc = p_key_auth = 0;
	p_lt_hard = p_lt_soft = 0;
	p_lb_hard = p_lb_soft = 0;

	p_aiflags = 0;
	p_aifamily = PF_UNSPEC;

	/* Clear out any natt OA information */
	if (p_natt_oa)
		freeaddrinfo (p_natt_oa);
	p_natt_oa = NULL;
	p_natt_type = 0;

	return;
}

void
free_buffer()
{
	/* we got tons of memory leaks in the parser anyways, leave them */

	return;
}
