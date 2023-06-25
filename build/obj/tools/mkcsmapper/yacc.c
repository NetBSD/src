#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif
/* original parser id follows */
/* yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93" */
/* (use YYMAJOR/YYMINOR for ifdefs dependent on parser version) */

#define YYBYACC 1
#define YYMAJOR 2
#define YYMINOR 0

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

#line 4 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"
/*-
 * Copyright (c)2003, 2006 Citrus Project,
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if !defined(lint)
__RCSID("$NetBSD: yacc.y,v 1.11 2016/06/28 09:22:16 wiz Exp $");
#endif /* not lint */

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include "ldef.h"

#ifndef __packed
#define __packed
#endif

#include "citrus_namespace.h"
#include "citrus_types.h"
#include "citrus_mapper_std_file.h"
#include "citrus_region.h"
#include "citrus_db_factory.h"
#include "citrus_db_hash.h"
#include "citrus_lookup_factory.h"
#include "citrus_pivot_factory.h"

int			debug = 0;
static char		*output = NULL;
static void		*table = NULL;
static size_t		table_size;
static char		*map_name;
static int		map_type;
static u_int32_t	dst_invalid, dst_ilseq, oob_mode, dst_unit_bits;
static void		(*putfunc)(void *, size_t, u_int32_t) = 0;

static u_int32_t	src_next;

static u_int32_t	done_flag = 0;
#define DF_TYPE			0x00000001
#define DF_NAME			0x00000002
#define DF_SRC_ZONE		0x00000004
#define DF_DST_INVALID		0x00000008
#define DF_DST_ILSEQ		0x00000010
#define DF_DST_UNIT_BITS	0x00000020
#define DF_OOB_MODE		0x00000040

static linear_zone_t	rowcol[_CITRUS_MAPPER_STD_ROWCOL_MAX];
static size_t		rowcol_len = 0;
static u_int32_t	rowcol_bits = 0, rowcol_mask = 0;

static void	dump_file(void);
static void	setup_map(void);
static void	set_type(int);
static void	set_name(char *);
static void	set_src_zone(u_int32_t);
static void	set_dst_invalid(u_int32_t);
static void	set_dst_ilseq(u_int32_t);
static void	set_dst_unit_bits(u_int32_t);
static void	set_oob_mode(u_int32_t);
static int	check_src(u_int32_t, u_int32_t);
static void	store(const linear_zone_t *, u_int32_t, int);
static void	put8(void *, size_t, u_int32_t);
static void	put16(void *, size_t, u_int32_t);
static void	put32(void *, size_t, u_int32_t);
static void	set_range(u_int32_t, u_int32_t);
static void	set_src(linear_zone_t *, u_int32_t, u_int32_t);
#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#line 107 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"
typedef union {
	u_int32_t	i_value;
	char		*s_value;
	linear_zone_t	lz_value;
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
#line 138 "yacc.c"

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

#if !(defined(yylex) || defined(YYSTATE))
int YYLEX_DECL();
#endif

/* Parameters sent to yyerror. */
#ifndef YYERROR_DECL
#define YYERROR_DECL() yyerror(const char *s)
#endif
#ifndef YYERROR_CALL
#define YYERROR_CALL(msg) yyerror(msg)
#endif

extern int YYPARSE_DECL();

#define R_TYPE 257
#define R_NAME 258
#define R_SRC_ZONE 259
#define R_DST_UNIT_BITS 260
#define R_DST_INVALID 261
#define R_DST_ILSEQ 262
#define R_BEGIN_MAP 263
#define R_END_MAP 264
#define R_INVALID 265
#define R_ROWCOL 266
#define R_ILSEQ 267
#define R_OOB_MODE 268
#define R_LN 269
#define L_IMM 270
#define L_STRING 271
#define YYERRCODE 256
typedef int YYINT;
static const YYINT yylhs[] = {                           -1,
    0,    6,    6,    6,    6,    6,    6,    6,    6,    6,
    9,   10,    3,   16,   17,   17,   11,    5,    5,   12,
   13,   14,   15,    4,    4,    7,   18,   19,   19,   20,
   20,    2,    2,    2,    1,    1,    1,    1,    8,    8,
};
static const YYINT yylen[] = {                            2,
    3,    0,    2,    2,    2,    2,    2,    2,    2,    2,
    2,    2,    1,    3,    0,    3,    2,    1,    6,    2,
    2,    2,    2,    1,    1,    3,    2,    0,    3,    3,
    4,    1,    1,    1,    0,    1,    3,    2,    1,    2,
};
static const YYINT yydefred[] = {                         2,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    3,    0,    4,    5,    6,    7,    8,    9,   10,   28,
   13,   12,   11,    0,   17,    0,   22,   20,   21,   39,
    0,   24,   25,   23,    0,    0,    0,    0,   40,   26,
    0,    0,    0,    0,   14,    0,    0,   38,    0,    0,
   15,   37,   33,   34,    0,   30,    0,   31,    0,    0,
   16,
};
#if defined(YYDESTRUCT_CALL) || defined(YYSTYPE_TOSTRING)
static const YYINT yystos[] = {                           0,
  273,  279,  257,  258,  259,  260,  261,  262,  263,  268,
  269,  280,  282,  283,  284,  285,  286,  287,  288,  291,
  266,  276,  271,  270,  278,  289,  270,  270,  270,  269,
  281,  265,  267,  277,  281,  292,   45,   47,  269,  264,
  270,   45,  274,  293,  270,  289,   45,  270,   61,  281,
   47,  270,  265,  267,  270,  275,  290,   45,  270,  289,
   47,
};
#endif /* YYDESTRUCT_CALL || YYSTYPE_TOSTRING */
static const YYINT yydgoto[] = {                          1,
   43,   56,   22,   34,   25,    2,   12,   31,   13,   14,
   15,   16,   17,   18,   19,   26,   57,   20,   36,   44,
};
static const YYINT yysindex[] = {                         0,
    0, -250, -261, -265, -256, -255, -249, -246, -237, -245,
    0, -237,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  -12,    0,  -13,    0,    0,    0,    0,
 -224,    0,    0,    0, -224,  -43, -223, -256,    0,    0,
    1, -222,   -7, -237,    0,    5, -215,    0, -214, -224,
    0,    0,    0,    0,   12,    0, -212,    0,  -12,   13,
    0,
};
static const YYINT yyrindex[] = {                         0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0, -232,    0,    0,    0,    0,
  -45,    0,    0,    0,   59,    2,    0,    0,    0,    0,
    3,    0,    0,    0,    0,    0,    0,    0,    0,  -44,
    0,    0,    0,    0, -208,    0,    0,    0, -219,    0,
    0,
};
#if YYBTYACC
static const YYINT yycindex[] = {                         0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,
};
#endif
static const YYINT yygindex[] = {                         0,
    0,    0,    0,    0,    0,    0,    0,   -9,    0,    0,
    0,    0,    0,    0,    0,  -34,    0,    0,    0,    0,
};
#define YYTABLESIZE 227
static const YYINT yytable[] = {                         27,
   29,   42,   35,   46,   21,   23,    3,    4,    5,    6,
    7,    8,    9,   24,   27,   27,   29,   10,   11,   32,
   28,   33,   60,   29,   18,   18,   18,   18,   18,   18,
   18,   30,   37,   38,   50,   18,   18,   19,   19,   19,
   19,   19,   19,   19,   39,   47,   45,   48,   19,   19,
   53,   51,   54,   49,   52,   55,   58,   59,    1,   61,
   32,    0,   35,   36,    0,    0,    0,    0,    0,    0,
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
    0,    0,    0,    0,    0,    0,    0,    0,   27,   29,
   40,    0,    0,    0,   27,   29,   41,
};
static const YYINT yycheck[] = {                         45,
   45,   45,   12,   38,  266,  271,  257,  258,  259,  260,
  261,  262,  263,  270,  270,   61,   61,  268,  269,  265,
  270,  267,   57,  270,  257,  258,  259,  260,  261,  262,
  263,  269,   45,   47,   44,  268,  269,  257,  258,  259,
  260,  261,  262,  263,  269,   45,  270,  270,  268,  269,
  265,   47,  267,   61,  270,  270,   45,  270,    0,   47,
  269,   -1,   61,   61,   -1,   -1,   -1,   -1,   -1,   -1,
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
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  264,  264,
  264,   -1,   -1,   -1,  270,  270,  270,
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
   -1,   -1,   -1,   -1,   -1,   -1,
};
#endif
#define YYFINAL 1
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 271
#define YYUNDFTOKEN 294
#define YYTRANSLATE(a) ((a) > YYMAXTOKEN ? YYUNDFTOKEN : (a))
#if YYDEBUG
static const char *const yyname[] = {

"$end",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,"'-'",0,"'/'",0,0,0,0,0,0,0,0,0,0,0,0,0,"'='",0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"error","R_TYPE","R_NAME",
"R_SRC_ZONE","R_DST_UNIT_BITS","R_DST_INVALID","R_DST_ILSEQ","R_BEGIN_MAP",
"R_END_MAP","R_INVALID","R_ROWCOL","R_ILSEQ","R_OOB_MODE","R_LN","L_IMM",
"L_STRING","$accept","file","src","dst","types","oob_mode_sel","zone",
"property","mapping","lns","name","type","src_zone","dst_invalid","dst_ilseq",
"dst_unit_bits","oob_mode","range","ranges","begin_map","map_elems","map_elem",
"illegal-symbol",
};
static const char *const yyrule[] = {
"$accept : file",
"file : property mapping lns",
"property :",
"property : property R_LN",
"property : property name",
"property : property type",
"property : property src_zone",
"property : property dst_invalid",
"property : property dst_ilseq",
"property : property dst_unit_bits",
"property : property oob_mode",
"name : R_NAME L_STRING",
"type : R_TYPE types",
"types : R_ROWCOL",
"range : L_IMM '-' L_IMM",
"ranges :",
"ranges : ranges range '/'",
"src_zone : R_SRC_ZONE zone",
"zone : range",
"zone : range '/' range '/' ranges L_IMM",
"dst_invalid : R_DST_INVALID L_IMM",
"dst_ilseq : R_DST_ILSEQ L_IMM",
"dst_unit_bits : R_DST_UNIT_BITS L_IMM",
"oob_mode : R_OOB_MODE oob_mode_sel",
"oob_mode_sel : R_INVALID",
"oob_mode_sel : R_ILSEQ",
"mapping : begin_map map_elems R_END_MAP",
"begin_map : R_BEGIN_MAP lns",
"map_elems :",
"map_elems : map_elems map_elem lns",
"map_elem : src '=' dst",
"map_elem : src '=' L_IMM '-'",
"dst : L_IMM",
"dst : R_INVALID",
"dst : R_ILSEQ",
"src :",
"src : L_IMM",
"src : L_IMM '-' L_IMM",
"src : '-' L_IMM",
"lns : R_LN",
"lns : lns R_LN",

};
#endif

#if YYDEBUG
int      yydebug;
#endif

int      yyerrflag;
int      yychar;
YYSTYPE  yyval;
YYSTYPE  yylval;
int      yynerrs;

#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
YYLTYPE  yyloc; /* position returned by actions */
YYLTYPE  yylloc; /* position from the lexer */
#endif

#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
#ifndef YYLLOC_DEFAULT
#define YYLLOC_DEFAULT(loc, rhs, n) \
do \
{ \
    if (n == 0) \
    { \
        (loc).first_line   = YYRHSLOC(rhs, 0).last_line; \
        (loc).first_column = YYRHSLOC(rhs, 0).last_column; \
        (loc).last_line    = YYRHSLOC(rhs, 0).last_line; \
        (loc).last_column  = YYRHSLOC(rhs, 0).last_column; \
    } \
    else \
    { \
        (loc).first_line   = YYRHSLOC(rhs, 1).first_line; \
        (loc).first_column = YYRHSLOC(rhs, 1).first_column; \
        (loc).last_line    = YYRHSLOC(rhs, n).last_line; \
        (loc).last_column  = YYRHSLOC(rhs, n).last_column; \
    } \
} while (0)
#endif /* YYLLOC_DEFAULT */
#endif /* defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED) */
#if YYBTYACC

#ifndef YYLVQUEUEGROWTH
#define YYLVQUEUEGROWTH 32
#endif
#endif /* YYBTYACC */

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

#ifndef YYINITSTACKSIZE
#define YYINITSTACKSIZE 200
#endif

typedef struct {
    unsigned stacksize;
    YYINT    *s_base;
    YYINT    *s_mark;
    YYINT    *s_last;
    YYSTYPE  *l_base;
    YYSTYPE  *l_mark;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    YYLTYPE  *p_base;
    YYLTYPE  *p_mark;
#endif
} YYSTACKDATA;
#if YYBTYACC

struct YYParseState_s
{
    struct YYParseState_s *save;    /* Previously saved parser state */
    YYSTACKDATA            yystack; /* saved parser stack */
    int                    state;   /* saved parser state */
    int                    errflag; /* saved error recovery status */
    int                    lexeme;  /* saved index of the conflict lexeme in the lexical queue */
    YYINT                  ctry;    /* saved index in yyctable[] for this conflict */
};
typedef struct YYParseState_s YYParseState;
#endif /* YYBTYACC */
/* variables for the parser stack */
static YYSTACKDATA yystack;
#if YYBTYACC

/* Current parser state */
static YYParseState *yyps = 0;

/* yypath != NULL: do the full parse, starting at *yypath parser state. */
static YYParseState *yypath = 0;

/* Base of the lexical value queue */
static YYSTYPE *yylvals = 0;

/* Current position at lexical value queue */
static YYSTYPE *yylvp = 0;

/* End position of lexical value queue */
static YYSTYPE *yylve = 0;

/* The last allocated position at the lexical value queue */
static YYSTYPE *yylvlim = 0;

#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
/* Base of the lexical position queue */
static YYLTYPE *yylpsns = 0;

/* Current position at lexical position queue */
static YYLTYPE *yylpp = 0;

/* End position of lexical position queue */
static YYLTYPE *yylpe = 0;

/* The last allocated position at the lexical position queue */
static YYLTYPE *yylplim = 0;
#endif

/* Current position at lexical token queue */
static YYINT  *yylexp = 0;

static YYINT  *yylexemes = 0;
#endif /* YYBTYACC */
#line 206 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"

static void
warning(const char *s)
{
	fprintf(stderr, "%s in %d\n", s, line_number);
}

int
yyerror(const char *s)
{
	warning(s);
	exit(1);
}

void
put8(void *ptr, size_t ofs, u_int32_t val)
{
	*((u_int8_t *)ptr + ofs) = val;
}

void
put16(void *ptr, size_t ofs, u_int32_t val)
{
	u_int16_t oval = htons(val);
	memcpy((u_int16_t *)ptr + ofs, &oval, 2);
}

void
put32(void *ptr, size_t ofs, u_int32_t val)
{
	u_int32_t oval = htonl(val);
	memcpy((u_int32_t *)ptr + ofs, &oval, 4);
}

static void
alloc_table(void)
{
	size_t i;
	u_int32_t val = 0;
	linear_zone_t *p;

	i = rowcol_len;
	p = &rowcol[--i];
	table_size = p->width;
	while (i > 0) {
		p = &rowcol[--i];
		table_size *= p->width;
	}
	table = (void *)malloc(table_size * dst_unit_bits / 8);
	if (table == NULL) {
		perror("malloc");
		exit(1);
	}

	switch (oob_mode) {
	case _CITRUS_MAPPER_STD_OOB_NONIDENTICAL:
		val = dst_invalid;
		break;
	case _CITRUS_MAPPER_STD_OOB_ILSEQ:
		val = dst_ilseq;
		break;
	default:
		_DIAGASSERT(0);
	}
	for (i = 0; i < table_size; i++)
		(*putfunc)(table, i, val);
}

static void
setup_map(void)
{

	if ((done_flag & DF_SRC_ZONE)==0) {
		fprintf(stderr, "SRC_ZONE is mandatory.\n");
		exit(1);
	}
	if ((done_flag & DF_DST_UNIT_BITS)==0) {
		fprintf(stderr, "DST_UNIT_BITS is mandatory.\n");
		exit(1);
	}

	if ((done_flag & DF_DST_INVALID) == 0)
		dst_invalid = 0xFFFFFFFF;
	if ((done_flag & DF_DST_ILSEQ) == 0)
		dst_ilseq = 0xFFFFFFFE;
	if ((done_flag & DF_OOB_MODE) == 0)
		oob_mode = _CITRUS_MAPPER_STD_OOB_NONIDENTICAL;

	alloc_table();
}

static void
create_rowcol_info(struct _region *r)
{
	void *ptr;
	size_t ofs, i, len;

	ofs = 0;
	ptr = malloc(_CITRUS_MAPPER_STD_ROWCOL_INFO_SIZE);
	if (ptr == NULL)
		err(EXIT_FAILURE, "malloc");
	put32(ptr, ofs, rowcol_bits); ofs++;
	put32(ptr, ofs, dst_invalid); ofs++;

	/* XXX: keep backward compatibility */
	switch (rowcol_len) {
	case 1:
		put32(ptr, ofs, 0); ofs++;
		put32(ptr, ofs, 0); ofs++;
	/*FALLTHROUGH*/
	case 2:
		len = 0;
		break;
	default:
		len = rowcol_len;
	}
	for (i = 0; i < rowcol_len; ++i) {
		put32(ptr, ofs, rowcol[i].begin); ofs++;
		put32(ptr, ofs, rowcol[i].end); ofs++;
	}
	put32(ptr, ofs, dst_unit_bits); ofs++;
	put32(ptr, ofs, len); ofs++;

	_region_init(r, ptr, ofs * 4);
}


static void
create_rowcol_ext_ilseq_info(struct _region *r)
{
	void *ptr;
	size_t ofs;

	ofs = 0;
	ptr = malloc(_CITRUS_MAPPER_STD_ROWCOL_EXT_ILSEQ_SIZE);
	if (ptr==NULL)
		err(EXIT_FAILURE, "malloc");

	put32(ptr, ofs, oob_mode); ofs++;
	put32(ptr, ofs, dst_ilseq); ofs++;

	_region_init(r, ptr, _CITRUS_MAPPER_STD_ROWCOL_EXT_ILSEQ_SIZE);
}

#define CHKERR(ret, func, a)						\
do {									\
	ret = func a;							\
	if (ret)							\
		errx(EXIT_FAILURE, "%s: %s", #func, strerror(ret));	\
} while (/*CONSTCOND*/0)

static void
dump_file(void)
{
	FILE *fp;
	int ret;
	struct _db_factory *df;
	struct _region data;
	void *serialized;
	size_t size;

	/*
	 * build database
	 */
	CHKERR(ret, _db_factory_create, (&df, _db_hash_std, NULL));

	/* store type */
	CHKERR(ret, _db_factory_addstr_by_s,
	       (df, _CITRUS_MAPPER_STD_SYM_TYPE,
		_CITRUS_MAPPER_STD_TYPE_ROWCOL));

	/* store info */
	create_rowcol_info(&data);
	CHKERR(ret, _db_factory_add_by_s,
	       (df, _CITRUS_MAPPER_STD_SYM_INFO, &data, 1));

	/* ilseq extension */
	create_rowcol_ext_ilseq_info(&data);
	CHKERR(ret, _db_factory_add_by_s,
	       (df, _CITRUS_MAPPER_STD_SYM_ROWCOL_EXT_ILSEQ, &data, 1));

	/* store table */
	_region_init(&data, table, table_size*dst_unit_bits/8);
	CHKERR(ret, _db_factory_add_by_s,
	       (df, _CITRUS_MAPPER_STD_SYM_TABLE, &data, 1));

	/*
	 * dump database to file
	 */
	if (output)
		fp = fopen(output, "wb");
	else
		fp = stdout;

	if (fp == NULL) {
		perror("fopen");
		exit(1);
	}

	/* dump database body */
	size = _db_factory_calc_size(df);
	serialized = malloc(size);
	_region_init(&data, serialized, size);
	CHKERR(ret, _db_factory_serialize,
	       (df, _CITRUS_MAPPER_STD_MAGIC, &data));
	if (fwrite(serialized, size, 1, fp) != 1)
		err(EXIT_FAILURE, "fwrite");

	fclose(fp);
}

static void
/*ARGSUSED*/
set_type(int type)
{

	if (done_flag & DF_TYPE) {
		warning("TYPE is duplicated. ignored this one");
		return;
	}

	map_type = type;

	done_flag |= DF_TYPE;
}
static void
/*ARGSUSED*/
set_name(char *str)
{

	if (done_flag & DF_NAME) {
		warning("NAME is duplicated. ignored this one");
		return;
	}

	map_name = str;

	done_flag |= DF_NAME;
}
static void
set_src_zone(u_int32_t val)
{
	size_t i;
	linear_zone_t *p;

	if (done_flag & DF_SRC_ZONE) {
		warning("SRC_ZONE is duplicated. ignored this one");
		return;
	}
	rowcol_bits = val;

	/* sanity check */
	switch (rowcol_bits) {
	case 8: case 16: case 32:
		if (rowcol_len <= 32 / rowcol_bits)
			break;
	/*FALLTHROUGH*/
	default: 
		goto bad;
	}
	rowcol_mask = 1 << (rowcol_bits - 1);
	rowcol_mask |= rowcol_mask - 1;
	for (i = 0; i < rowcol_len; ++i) {
		p = &rowcol[i];
		_DIAGASSERT(p->begin <= p->end);
		if (p->end > rowcol_mask)
			goto bad;
	}
	done_flag |= DF_SRC_ZONE;
	return;

bad:
	yyerror("Illegal argument for SRC_ZONE");
}
static void
set_dst_invalid(u_int32_t val)
{

	if (done_flag & DF_DST_INVALID) {
		warning("DST_INVALID is duplicated. ignored this one");
		return;
	}

	dst_invalid = val;

	done_flag |= DF_DST_INVALID;
}
static void
set_dst_ilseq(u_int32_t val)
{

	if (done_flag & DF_DST_ILSEQ) {
		warning("DST_ILSEQ is duplicated. ignored this one");
		return;
	}

	dst_ilseq = val;

	done_flag |= DF_DST_ILSEQ;
}
static void
set_oob_mode(u_int32_t val)
{

	if (done_flag & DF_OOB_MODE) {
		warning("OOB_MODE is duplicated. ignored this one");
		return;
	}

	oob_mode = val;

	done_flag |= DF_OOB_MODE;
}
static void
set_dst_unit_bits(u_int32_t val)
{

	if (done_flag & DF_DST_UNIT_BITS) {
		warning("DST_UNIT_BITS is duplicated. ignored this one");
		return;
	}

	switch (val) {
	case 8:
		putfunc = &put8;
		dst_unit_bits = val;
		break;
	case 16:
		putfunc = &put16;
		dst_unit_bits = val;
		break;
	case 32:
		putfunc = &put32;
		dst_unit_bits = val;
		break;
	default:
		yyerror("Illegal argument for DST_UNIT_BITS");
	}
	done_flag |= DF_DST_UNIT_BITS;
}
static int
check_src(u_int32_t begin, u_int32_t end)
{
	size_t i;
	linear_zone_t *p;
	u_int32_t m, n;

	if (begin > end)
		return 1;
	if (begin < end) {
		m = begin & ~rowcol_mask;
		n = end & ~rowcol_mask;
		if (m != n)
			return 1;
	}
	for (i = rowcol_len * rowcol_bits, p = &rowcol[0]; i > 0; ++p) {
		i -= rowcol_bits;
		m = (begin >> i) & rowcol_mask;
		if (m < p->begin || m > p->end)
			return 1;
	}
	if (begin < end) {
		n = end & rowcol_mask;
		_DIAGASSERT(p > rowcol);
		--p;
		if (n < p->begin || n > p->end)
			return 1;
	}
	return 0;
}
static void
store(const linear_zone_t *lz, u_int32_t dst, int inc)
{
	size_t i, ofs;
	linear_zone_t *p;
	u_int32_t n;

	ofs = 0;
	for (i = rowcol_len * rowcol_bits, p = &rowcol[0]; i > 0; ++p) {
		i -= rowcol_bits;
		n = ((lz->begin >> i) & rowcol_mask) - p->begin;
		ofs = (ofs * p->width) + n;
	}
	n = lz->width;
	while (n-- > 0) {
		(*putfunc)(table, ofs++, dst);
		if (inc)
			dst++;
	}
}
static void
set_range(u_int32_t begin, u_int32_t end)
{
	linear_zone_t *p;

	if (rowcol_len >= _CITRUS_MAPPER_STD_ROWCOL_MAX)
		goto bad;
	p = &rowcol[rowcol_len++];

	if (begin > end)
		goto bad;
	p->begin = begin, p->end = end;
	p->width = end - begin + 1;

	return;

bad:
	yyerror("Illegal argument for SRC_ZONE");
}
static void
set_src(linear_zone_t *lz, u_int32_t begin, u_int32_t end)
{
	_DIAGASSERT(lz != NULL);

	if (check_src(begin, end) != 0)
		yyerror("illegal zone");

	lz->begin = begin, lz->end = end;
	lz->width = end - begin + 1;

	src_next = end + 1;
}

static void
do_mkdb(FILE *in)
{
	int ret;
	FILE *out;

        /* dump DB to file */
	if (output)
		out = fopen(output, "wb");
	else
		out = stdout;

	if (out==NULL)
		err(EXIT_FAILURE, "fopen");

	ret = _lookup_factory_convert(out, in);
	fclose(out);
	if (ret && output)
		unlink(output); /* dump failure */
}

static void
do_mkpv(FILE *in)
{
	int ret;
	FILE *out;

        /* dump pivot to file */
	if (output)
		out = fopen(output, "wb");
	else
		out = stdout;

	if (out == NULL)
		err(EXIT_FAILURE, "fopen");

	ret = _pivot_factory_convert(out, in);
	fclose(out);
	if (ret && output)
		unlink(output); /* dump failure */
	if (ret)
 		errc(EXIT_FAILURE, ret, "");
}

__dead static void
usage(void)
{
	fprintf(stderr, "Usage: %s [-d] [-m|-p] [-o outfile] [infile]\n",
	    getprogname());
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	int ch;
	extern char *optarg;
	extern int optind;
	FILE *in = NULL;
	int mkdb = 0, mkpv = 0;

	while ((ch=getopt(argc, argv, "do:mp")) != EOF) {
		switch (ch) {
		case 'd':
			debug=1;
			break;
		case 'o':
			output = strdup(optarg);
			break;
		case 'm':
			mkdb = 1;
			break;
		case 'p':
			mkpv = 1;
			break;
		default:
			usage();
		}
	}

	argc-=optind;
	argv+=optind;
	switch (argc) {
	case 0:
		in = stdin;
		break;
	case 1:
		in = fopen(argv[0], "r");
		if (!in)
			err(EXIT_FAILURE, "%s", argv[0]);
		break;
	default:
		usage();
	}

	if (mkdb)
		do_mkdb(in);
	else if (mkpv)
		do_mkpv(in);
	else {
		yyin = in;
		yyparse();
	}

	return (0);
}
#line 1066 "yacc.c"

/* For use in generated program */
#define yydepth (int)(yystack.s_mark - yystack.s_base)
#if YYBTYACC
#define yytrial (yyps->save)
#endif /* YYBTYACC */

#if YYDEBUG
#include <stdio.h>	/* needed for printf */
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
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    YYLTYPE *newps;
#endif

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

#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    newps = (YYLTYPE *)realloc(data->p_base, newsize * sizeof(*newps));
    if (newps == 0)
        return YYENOMEM;

    data->p_base = newps;
    data->p_mark = newps + i;
#endif

    data->stacksize = newsize;
    data->s_last = data->s_base + newsize - 1;

#if YYDEBUG
    if (yydebug)
        fprintf(stderr, "%sdebug: stack size increased to %d\n", YYPREFIX, newsize);
#endif
    return 0;
}

#if YYPURE || defined(YY_NO_LEAKS)
static void yyfreestack(YYSTACKDATA *data)
{
    free(data->s_base);
    free(data->l_base);
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    free(data->p_base);
#endif
    memset(data, 0, sizeof(*data));
}
#else
#define yyfreestack(data) /* nothing */
#endif /* YYPURE || defined(YY_NO_LEAKS) */
#if YYBTYACC

static YYParseState *
yyNewState(unsigned size)
{
    YYParseState *p = (YYParseState *) malloc(sizeof(YYParseState));
    if (p == NULL) return NULL;

    p->yystack.stacksize = size;
    if (size == 0)
    {
        p->yystack.s_base = NULL;
        p->yystack.l_base = NULL;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        p->yystack.p_base = NULL;
#endif
        return p;
    }
    p->yystack.s_base    = (YYINT *) malloc(size * sizeof(YYINT));
    if (p->yystack.s_base == NULL) return NULL;
    p->yystack.l_base    = (YYSTYPE *) malloc(size * sizeof(YYSTYPE));
    if (p->yystack.l_base == NULL) return NULL;
    memset(p->yystack.l_base, 0, size * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    p->yystack.p_base    = (YYLTYPE *) malloc(size * sizeof(YYLTYPE));
    if (p->yystack.p_base == NULL) return NULL;
    memset(p->yystack.p_base, 0, size * sizeof(YYLTYPE));
#endif

    return p;
}

static void
yyFreeState(YYParseState *p)
{
    yyfreestack(&p->yystack);
    free(p);
}
#endif /* YYBTYACC */

#define YYABORT  goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR  goto yyerrlab
#if YYBTYACC
#define YYVALID        do { if (yyps->save)            goto yyvalid; } while(0)
#define YYVALID_NESTED do { if (yyps->save && \
                                yyps->save->save == 0) goto yyvalid; } while(0)
#endif /* YYBTYACC */

int
YYPARSE_DECL()
{
    int yym, yyn, yystate, yyresult;
#if YYBTYACC
    int yynewerrflag;
    YYParseState *yyerrctx = NULL;
#endif /* YYBTYACC */
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    YYLTYPE  yyerror_loc_range[3]; /* position of error start/end (0 unused) */
#endif
#if YYDEBUG
    const char *yys;

    if ((yys = getenv("YYDEBUG")) != 0)
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
    if (yydebug)
        fprintf(stderr, "%sdebug[<# of symbols on state stack>]\n", YYPREFIX);
#endif
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    memset(yyerror_loc_range, 0, sizeof(yyerror_loc_range));
#endif

#if YYBTYACC
    yyps = yyNewState(0); if (yyps == 0) goto yyenomem;
    yyps->save = 0;
#endif /* YYBTYACC */
    yym = 0;
    yyn = 0;
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
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yystack.p_mark = yystack.p_base;
#endif
    yystate = 0;
    *yystack.s_mark = 0;

yyloop:
    if ((yyn = yydefred[yystate]) != 0) goto yyreduce;
    if (yychar < 0)
    {
#if YYBTYACC
        do {
        if (yylvp < yylve)
        {
            /* we're currently re-reading tokens */
            yylval = *yylvp++;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            yylloc = *yylpp++;
#endif
            yychar = *yylexp++;
            break;
        }
        if (yyps->save)
        {
            /* in trial mode; save scanner results for future parse attempts */
            if (yylvp == yylvlim)
            {   /* Enlarge lexical value queue */
                size_t p = (size_t) (yylvp - yylvals);
                size_t s = (size_t) (yylvlim - yylvals);

                s += YYLVQUEUEGROWTH;
                if ((yylexemes = (YYINT *)realloc(yylexemes, s * sizeof(YYINT))) == NULL) goto yyenomem;
                if ((yylvals   = (YYSTYPE *)realloc(yylvals, s * sizeof(YYSTYPE))) == NULL) goto yyenomem;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                if ((yylpsns   = (YYLTYPE *)realloc(yylpsns, s * sizeof(YYLTYPE))) == NULL) goto yyenomem;
#endif
                yylvp   = yylve = yylvals + p;
                yylvlim = yylvals + s;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                yylpp   = yylpe = yylpsns + p;
                yylplim = yylpsns + s;
#endif
                yylexp  = yylexemes + p;
            }
            *yylexp = (YYINT) YYLEX;
            *yylvp++ = yylval;
            yylve++;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            *yylpp++ = yylloc;
            yylpe++;
#endif
            yychar = *yylexp++;
            break;
        }
        /* normal operation, no conflict encountered */
#endif /* YYBTYACC */
        yychar = YYLEX;
#if YYBTYACC
        } while (0);
#endif /* YYBTYACC */
        if (yychar < 0) yychar = YYEOF;
#if YYDEBUG
        if (yydebug)
        {
            if ((yys = yyname[YYTRANSLATE(yychar)]) == NULL) yys = yyname[YYUNDFTOKEN];
            fprintf(stderr, "%s[%d]: state %d, reading token %d (%s)",
                            YYDEBUGSTR, yydepth, yystate, yychar, yys);
#ifdef YYSTYPE_TOSTRING
#if YYBTYACC
            if (!yytrial)
#endif /* YYBTYACC */
                fprintf(stderr, " <%s>", YYSTYPE_TOSTRING(yychar, yylval));
#endif
            fputc('\n', stderr);
        }
#endif
    }
#if YYBTYACC

    /* Do we have a conflict? */
    if (((yyn = yycindex[yystate]) != 0) && (yyn += yychar) >= 0 &&
        yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) yychar)
    {
        YYINT ctry;

        if (yypath)
        {
            YYParseState *save;
#if YYDEBUG
            if (yydebug)
                fprintf(stderr, "%s[%d]: CONFLICT in state %d: following successful trial parse\n",
                                YYDEBUGSTR, yydepth, yystate);
#endif
            /* Switch to the next conflict context */
            save = yypath;
            yypath = save->save;
            save->save = NULL;
            ctry = save->ctry;
            if (save->state != yystate) YYABORT;
            yyFreeState(save);

        }
        else
        {

            /* Unresolved conflict - start/continue trial parse */
            YYParseState *save;
#if YYDEBUG
            if (yydebug)
            {
                fprintf(stderr, "%s[%d]: CONFLICT in state %d. ", YYDEBUGSTR, yydepth, yystate);
                if (yyps->save)
                    fputs("ALREADY in conflict, continuing trial parse.\n", stderr);
                else
                    fputs("Starting trial parse.\n", stderr);
            }
#endif
            save                  = yyNewState((unsigned)(yystack.s_mark - yystack.s_base + 1));
            if (save == NULL) goto yyenomem;
            save->save            = yyps->save;
            save->state           = yystate;
            save->errflag         = yyerrflag;
            save->yystack.s_mark  = save->yystack.s_base + (yystack.s_mark - yystack.s_base);
            memcpy (save->yystack.s_base, yystack.s_base, (size_t) (yystack.s_mark - yystack.s_base + 1) * sizeof(YYINT));
            save->yystack.l_mark  = save->yystack.l_base + (yystack.l_mark - yystack.l_base);
            memcpy (save->yystack.l_base, yystack.l_base, (size_t) (yystack.l_mark - yystack.l_base + 1) * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            save->yystack.p_mark  = save->yystack.p_base + (yystack.p_mark - yystack.p_base);
            memcpy (save->yystack.p_base, yystack.p_base, (size_t) (yystack.p_mark - yystack.p_base + 1) * sizeof(YYLTYPE));
#endif
            ctry                  = yytable[yyn];
            if (yyctable[ctry] == -1)
            {
#if YYDEBUG
                if (yydebug && yychar >= YYEOF)
                    fprintf(stderr, "%s[%d]: backtracking 1 token\n", YYDEBUGSTR, yydepth);
#endif
                ctry++;
            }
            save->ctry = ctry;
            if (yyps->save == NULL)
            {
                /* If this is a first conflict in the stack, start saving lexemes */
                if (!yylexemes)
                {
                    yylexemes = (YYINT *) malloc((YYLVQUEUEGROWTH) * sizeof(YYINT));
                    if (yylexemes == NULL) goto yyenomem;
                    yylvals   = (YYSTYPE *) malloc((YYLVQUEUEGROWTH) * sizeof(YYSTYPE));
                    if (yylvals == NULL) goto yyenomem;
                    yylvlim   = yylvals + YYLVQUEUEGROWTH;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                    yylpsns   = (YYLTYPE *) malloc((YYLVQUEUEGROWTH) * sizeof(YYLTYPE));
                    if (yylpsns == NULL) goto yyenomem;
                    yylplim   = yylpsns + YYLVQUEUEGROWTH;
#endif
                }
                if (yylvp == yylve)
                {
                    yylvp  = yylve = yylvals;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                    yylpp  = yylpe = yylpsns;
#endif
                    yylexp = yylexemes;
                    if (yychar >= YYEOF)
                    {
                        *yylve++ = yylval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                        *yylpe++ = yylloc;
#endif
                        *yylexp  = (YYINT) yychar;
                        yychar   = YYEMPTY;
                    }
                }
            }
            if (yychar >= YYEOF)
            {
                yylvp--;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                yylpp--;
#endif
                yylexp--;
                yychar = YYEMPTY;
            }
            save->lexeme = (int) (yylvp - yylvals);
            yyps->save   = save;
        }
        if (yytable[yyn] == ctry)
        {
#if YYDEBUG
            if (yydebug)
                fprintf(stderr, "%s[%d]: state %d, shifting to state %d\n",
                                YYDEBUGSTR, yydepth, yystate, yyctable[ctry]);
#endif
            if (yychar < 0)
            {
                yylvp++;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                yylpp++;
#endif
                yylexp++;
            }
            if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM)
                goto yyoverflow;
            yystate = yyctable[ctry];
            *++yystack.s_mark = (YYINT) yystate;
            *++yystack.l_mark = yylval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            *++yystack.p_mark = yylloc;
#endif
            yychar  = YYEMPTY;
            if (yyerrflag > 0) --yyerrflag;
            goto yyloop;
        }
        else
        {
            yyn = yyctable[ctry];
            goto yyreduce;
        }
    } /* End of code dealing with conflicts */
#endif /* YYBTYACC */
    if (((yyn = yysindex[yystate]) != 0) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) yychar)
    {
#if YYDEBUG
        if (yydebug)
            fprintf(stderr, "%s[%d]: state %d, shifting to state %d\n",
                            YYDEBUGSTR, yydepth, yystate, yytable[yyn]);
#endif
        if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM) goto yyoverflow;
        yystate = yytable[yyn];
        *++yystack.s_mark = yytable[yyn];
        *++yystack.l_mark = yylval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        *++yystack.p_mark = yylloc;
#endif
        yychar = YYEMPTY;
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if (((yyn = yyrindex[yystate]) != 0) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag != 0) goto yyinrecovery;
#if YYBTYACC

    yynewerrflag = 1;
    goto yyerrhandler;
    goto yyerrlab; /* redundant goto avoids 'unused label' warning */

yyerrlab:
    /* explicit YYERROR from an action -- pop the rhs of the rule reduced
     * before looking for error recovery */
    yystack.s_mark -= yym;
    yystate = *yystack.s_mark;
    yystack.l_mark -= yym;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yystack.p_mark -= yym;
#endif

    yynewerrflag = 0;
yyerrhandler:
    while (yyps->save)
    {
        int ctry;
        YYParseState *save = yyps->save;
#if YYDEBUG
        if (yydebug)
            fprintf(stderr, "%s[%d]: ERROR in state %d, CONFLICT BACKTRACKING to state %d, %d tokens\n",
                            YYDEBUGSTR, yydepth, yystate, yyps->save->state,
                    (int)(yylvp - yylvals - yyps->save->lexeme));
#endif
        /* Memorize most forward-looking error state in case it's really an error. */
        if (yyerrctx == NULL || yyerrctx->lexeme < yylvp - yylvals)
        {
            /* Free old saved error context state */
            if (yyerrctx) yyFreeState(yyerrctx);
            /* Create and fill out new saved error context state */
            yyerrctx                 = yyNewState((unsigned)(yystack.s_mark - yystack.s_base + 1));
            if (yyerrctx == NULL) goto yyenomem;
            yyerrctx->save           = yyps->save;
            yyerrctx->state          = yystate;
            yyerrctx->errflag        = yyerrflag;
            yyerrctx->yystack.s_mark = yyerrctx->yystack.s_base + (yystack.s_mark - yystack.s_base);
            memcpy (yyerrctx->yystack.s_base, yystack.s_base, (size_t) (yystack.s_mark - yystack.s_base + 1) * sizeof(YYINT));
            yyerrctx->yystack.l_mark = yyerrctx->yystack.l_base + (yystack.l_mark - yystack.l_base);
            memcpy (yyerrctx->yystack.l_base, yystack.l_base, (size_t) (yystack.l_mark - yystack.l_base + 1) * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            yyerrctx->yystack.p_mark = yyerrctx->yystack.p_base + (yystack.p_mark - yystack.p_base);
            memcpy (yyerrctx->yystack.p_base, yystack.p_base, (size_t) (yystack.p_mark - yystack.p_base + 1) * sizeof(YYLTYPE));
#endif
            yyerrctx->lexeme         = (int) (yylvp - yylvals);
        }
        yylvp          = yylvals   + save->lexeme;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        yylpp          = yylpsns   + save->lexeme;
#endif
        yylexp         = yylexemes + save->lexeme;
        yychar         = YYEMPTY;
        yystack.s_mark = yystack.s_base + (save->yystack.s_mark - save->yystack.s_base);
        memcpy (yystack.s_base, save->yystack.s_base, (size_t) (yystack.s_mark - yystack.s_base + 1) * sizeof(YYINT));
        yystack.l_mark = yystack.l_base + (save->yystack.l_mark - save->yystack.l_base);
        memcpy (yystack.l_base, save->yystack.l_base, (size_t) (yystack.l_mark - yystack.l_base + 1) * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        yystack.p_mark = yystack.p_base + (save->yystack.p_mark - save->yystack.p_base);
        memcpy (yystack.p_base, save->yystack.p_base, (size_t) (yystack.p_mark - yystack.p_base + 1) * sizeof(YYLTYPE));
#endif
        ctry           = ++save->ctry;
        yystate        = save->state;
        /* We tried shift, try reduce now */
        if ((yyn = yyctable[ctry]) >= 0) goto yyreduce;
        yyps->save     = save->save;
        save->save     = NULL;
        yyFreeState(save);

        /* Nothing left on the stack -- error */
        if (!yyps->save)
        {
#if YYDEBUG
            if (yydebug)
                fprintf(stderr, "%sdebug[%d,trial]: trial parse FAILED, entering ERROR mode\n",
                                YYPREFIX, yydepth);
#endif
            /* Restore state as it was in the most forward-advanced error */
            yylvp          = yylvals   + yyerrctx->lexeme;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            yylpp          = yylpsns   + yyerrctx->lexeme;
#endif
            yylexp         = yylexemes + yyerrctx->lexeme;
            yychar         = yylexp[-1];
            yylval         = yylvp[-1];
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            yylloc         = yylpp[-1];
#endif
            yystack.s_mark = yystack.s_base + (yyerrctx->yystack.s_mark - yyerrctx->yystack.s_base);
            memcpy (yystack.s_base, yyerrctx->yystack.s_base, (size_t) (yystack.s_mark - yystack.s_base + 1) * sizeof(YYINT));
            yystack.l_mark = yystack.l_base + (yyerrctx->yystack.l_mark - yyerrctx->yystack.l_base);
            memcpy (yystack.l_base, yyerrctx->yystack.l_base, (size_t) (yystack.l_mark - yystack.l_base + 1) * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            yystack.p_mark = yystack.p_base + (yyerrctx->yystack.p_mark - yyerrctx->yystack.p_base);
            memcpy (yystack.p_base, yyerrctx->yystack.p_base, (size_t) (yystack.p_mark - yystack.p_base + 1) * sizeof(YYLTYPE));
#endif
            yystate        = yyerrctx->state;
            yyFreeState(yyerrctx);
            yyerrctx       = NULL;
        }
        yynewerrflag = 1;
    }
    if (yynewerrflag == 0) goto yyinrecovery;
#endif /* YYBTYACC */

    YYERROR_CALL("syntax error");
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yyerror_loc_range[1] = yylloc; /* lookahead position is error start position */
#endif

#if !YYBTYACC
    goto yyerrlab; /* redundant goto avoids 'unused label' warning */
yyerrlab:
#endif
    ++yynerrs;

yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if (((yyn = yysindex[*yystack.s_mark]) != 0) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    fprintf(stderr, "%s[%d]: state %d, error recovery shifting to state %d\n",
                                    YYDEBUGSTR, yydepth, *yystack.s_mark, yytable[yyn]);
#endif
                if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM) goto yyoverflow;
                yystate = yytable[yyn];
                *++yystack.s_mark = yytable[yyn];
                *++yystack.l_mark = yylval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                /* lookahead position is error end position */
                yyerror_loc_range[2] = yylloc;
                YYLLOC_DEFAULT(yyloc, yyerror_loc_range, 2); /* position of error span */
                *++yystack.p_mark = yyloc;
#endif
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    fprintf(stderr, "%s[%d]: error recovery discarding state %d\n",
                                    YYDEBUGSTR, yydepth, *yystack.s_mark);
#endif
                if (yystack.s_mark <= yystack.s_base) goto yyabort;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                /* the current TOS position is the error start position */
                yyerror_loc_range[1] = *yystack.p_mark;
#endif
#if defined(YYDESTRUCT_CALL)
#if YYBTYACC
                if (!yytrial)
#endif /* YYBTYACC */
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                    YYDESTRUCT_CALL("error: discarding state",
                                    yystos[*yystack.s_mark], yystack.l_mark, yystack.p_mark);
#else
                    YYDESTRUCT_CALL("error: discarding state",
                                    yystos[*yystack.s_mark], yystack.l_mark);
#endif /* defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED) */
#endif /* defined(YYDESTRUCT_CALL) */
                --yystack.s_mark;
                --yystack.l_mark;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                --yystack.p_mark;
#endif
            }
        }
    }
    else
    {
        if (yychar == YYEOF) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            if ((yys = yyname[YYTRANSLATE(yychar)]) == NULL) yys = yyname[YYUNDFTOKEN];
            fprintf(stderr, "%s[%d]: state %d, error recovery discarding token %d (%s)\n",
                            YYDEBUGSTR, yydepth, yystate, yychar, yys);
        }
#endif
#if defined(YYDESTRUCT_CALL)
#if YYBTYACC
        if (!yytrial)
#endif /* YYBTYACC */
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            YYDESTRUCT_CALL("error: discarding token", yychar, &yylval, &yylloc);
#else
            YYDESTRUCT_CALL("error: discarding token", yychar, &yylval);
#endif /* defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED) */
#endif /* defined(YYDESTRUCT_CALL) */
        yychar = YYEMPTY;
        goto yyloop;
    }

yyreduce:
    yym = yylen[yyn];
#if YYDEBUG
    if (yydebug)
    {
        fprintf(stderr, "%s[%d]: state %d, reducing by rule %d (%s)",
                        YYDEBUGSTR, yydepth, yystate, yyn, yyrule[yyn]);
#ifdef YYSTYPE_TOSTRING
#if YYBTYACC
        if (!yytrial)
#endif /* YYBTYACC */
            if (yym > 0)
            {
                int i;
                fputc('<', stderr);
                for (i = yym; i > 0; i--)
                {
                    if (i != yym) fputs(", ", stderr);
                    fputs(YYSTYPE_TOSTRING(yystos[yystack.s_mark[1-i]],
                                           yystack.l_mark[1-i]), stderr);
                }
                fputc('>', stderr);
            }
#endif
        fputc('\n', stderr);
    }
#endif
    if (yym > 0)
        yyval = yystack.l_mark[1-yym];
    else
        memset(&yyval, 0, sizeof yyval);
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)

    /* Perform position reduction */
    memset(&yyloc, 0, sizeof(yyloc));
#if YYBTYACC
    if (!yytrial)
#endif /* YYBTYACC */
    {
        YYLLOC_DEFAULT(yyloc, &yystack.p_mark[-yym], yym);
        /* just in case YYERROR is invoked within the action, save
           the start of the rhs as the error start position */
        yyerror_loc_range[1] = yystack.p_mark[1-yym];
    }
#endif

    switch (yyn)
    {
case 1:
#line 127 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"
	{ dump_file(); }
#line 1 ""
break;
case 11:
#line 139 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"
	{ set_name(yystack.l_mark[0].s_value); yystack.l_mark[0].s_value = NULL; }
#line 1 ""
break;
case 12:
#line 140 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"
	{ set_type(yystack.l_mark[0].i_value); }
#line 1 ""
break;
case 13:
#line 141 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"
	{ yyval.i_value = R_ROWCOL; }
#line 1 ""
break;
case 14:
#line 142 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"
	{ set_range(yystack.l_mark[-2].i_value, yystack.l_mark[0].i_value); }
#line 1 ""
break;
case 17:
#line 147 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"
	{ set_src_zone(yystack.l_mark[0].i_value); }
#line 1 ""
break;
case 18:
#line 148 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"
	{
			yyval.i_value = 32;
		}
#line 1 ""
break;
case 19:
#line 151 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"
	{
			yyval.i_value = yystack.l_mark[0].i_value;
		}
#line 1 ""
break;
case 20:
#line 155 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"
	{ set_dst_invalid(yystack.l_mark[0].i_value); }
#line 1 ""
break;
case 21:
#line 156 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"
	{ set_dst_ilseq(yystack.l_mark[0].i_value); }
#line 1 ""
break;
case 22:
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"
	{ set_dst_unit_bits(yystack.l_mark[0].i_value); }
#line 1 ""
break;
case 23:
#line 158 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"
	{ set_oob_mode(yystack.l_mark[0].i_value); }
#line 1 ""
break;
case 24:
#line 160 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"
	{ yyval.i_value = _CITRUS_MAPPER_STD_OOB_NONIDENTICAL; }
#line 1 ""
break;
case 25:
#line 161 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"
	{ yyval.i_value = _CITRUS_MAPPER_STD_OOB_ILSEQ; }
#line 1 ""
break;
case 27:
#line 164 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"
	{ setup_map(); }
#line 1 ""
break;
case 30:
#line 170 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"
	{ store(&yystack.l_mark[-2].lz_value, yystack.l_mark[0].i_value, 0); }
#line 1 ""
break;
case 31:
#line 172 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"
	{ store(&yystack.l_mark[-3].lz_value, yystack.l_mark[-1].i_value, 1); }
#line 1 ""
break;
case 32:
#line 174 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"
	{
			yyval.i_value = yystack.l_mark[0].i_value;
		}
#line 1 ""
break;
case 33:
#line 178 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"
	{
			yyval.i_value = dst_invalid;
		}
#line 1 ""
break;
case 34:
#line 182 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"
	{
			yyval.i_value = dst_ilseq;
		}
#line 1 ""
break;
case 35:
#line 187 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"
	{
			set_src(&yyval.lz_value, src_next, src_next);
		}
#line 1 ""
break;
case 36:
#line 191 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"
	{
			set_src(&yyval.lz_value, yystack.l_mark[0].i_value, yystack.l_mark[0].i_value);
		}
#line 1 ""
break;
case 37:
#line 195 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"
	{
			set_src(&yyval.lz_value, yystack.l_mark[-2].i_value, yystack.l_mark[0].i_value);
		}
#line 1 ""
break;
case 38:
#line 199 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/mkcsmapper/../../usr.bin/mkcsmapper/yacc.y"
	{
			set_src(&yyval.lz_value, src_next, yystack.l_mark[0].i_value);
		}
#line 1 ""
break;
#line 1874 "yacc.c"
    default:
        break;
    }
    yystack.s_mark -= yym;
    yystate = *yystack.s_mark;
    yystack.l_mark -= yym;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yystack.p_mark -= yym;
#endif
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
        {
            fprintf(stderr, "%s[%d]: after reduction, ", YYDEBUGSTR, yydepth);
#ifdef YYSTYPE_TOSTRING
#if YYBTYACC
            if (!yytrial)
#endif /* YYBTYACC */
                fprintf(stderr, "result is <%s>, ", YYSTYPE_TOSTRING(yystos[YYFINAL], yyval));
#endif
            fprintf(stderr, "shifting from state 0 to final state %d\n", YYFINAL);
        }
#endif
        yystate = YYFINAL;
        *++yystack.s_mark = YYFINAL;
        *++yystack.l_mark = yyval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        *++yystack.p_mark = yyloc;
#endif
        if (yychar < 0)
        {
#if YYBTYACC
            do {
            if (yylvp < yylve)
            {
                /* we're currently re-reading tokens */
                yylval = *yylvp++;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                yylloc = *yylpp++;
#endif
                yychar = *yylexp++;
                break;
            }
            if (yyps->save)
            {
                /* in trial mode; save scanner results for future parse attempts */
                if (yylvp == yylvlim)
                {   /* Enlarge lexical value queue */
                    size_t p = (size_t) (yylvp - yylvals);
                    size_t s = (size_t) (yylvlim - yylvals);

                    s += YYLVQUEUEGROWTH;
                    if ((yylexemes = (YYINT *)realloc(yylexemes, s * sizeof(YYINT))) == NULL)
                        goto yyenomem;
                    if ((yylvals   = (YYSTYPE *)realloc(yylvals, s * sizeof(YYSTYPE))) == NULL)
                        goto yyenomem;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                    if ((yylpsns   = (YYLTYPE *)realloc(yylpsns, s * sizeof(YYLTYPE))) == NULL)
                        goto yyenomem;
#endif
                    yylvp   = yylve = yylvals + p;
                    yylvlim = yylvals + s;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                    yylpp   = yylpe = yylpsns + p;
                    yylplim = yylpsns + s;
#endif
                    yylexp  = yylexemes + p;
                }
                *yylexp = (YYINT) YYLEX;
                *yylvp++ = yylval;
                yylve++;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                *yylpp++ = yylloc;
                yylpe++;
#endif
                yychar = *yylexp++;
                break;
            }
            /* normal operation, no conflict encountered */
#endif /* YYBTYACC */
            yychar = YYLEX;
#if YYBTYACC
            } while (0);
#endif /* YYBTYACC */
            if (yychar < 0) yychar = YYEOF;
#if YYDEBUG
            if (yydebug)
            {
                if ((yys = yyname[YYTRANSLATE(yychar)]) == NULL) yys = yyname[YYUNDFTOKEN];
                fprintf(stderr, "%s[%d]: state %d, reading token %d (%s)\n",
                                YYDEBUGSTR, yydepth, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == YYEOF) goto yyaccept;
        goto yyloop;
    }
    if (((yyn = yygindex[yym]) != 0) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
    {
        fprintf(stderr, "%s[%d]: after reduction, ", YYDEBUGSTR, yydepth);
#ifdef YYSTYPE_TOSTRING
#if YYBTYACC
        if (!yytrial)
#endif /* YYBTYACC */
            fprintf(stderr, "result is <%s>, ", YYSTYPE_TOSTRING(yystos[yystate], yyval));
#endif
        fprintf(stderr, "shifting from state %d to state %d\n", *yystack.s_mark, yystate);
    }
#endif
    if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM) goto yyoverflow;
    *++yystack.s_mark = (YYINT) yystate;
    *++yystack.l_mark = yyval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    *++yystack.p_mark = yyloc;
#endif
    goto yyloop;
#if YYBTYACC

    /* Reduction declares that this path is valid. Set yypath and do a full parse */
yyvalid:
    if (yypath) YYABORT;
    while (yyps->save)
    {
        YYParseState *save = yyps->save;
        yyps->save = save->save;
        save->save = yypath;
        yypath = save;
    }
#if YYDEBUG
    if (yydebug)
        fprintf(stderr, "%s[%d]: state %d, CONFLICT trial successful, backtracking to state %d, %d tokens\n",
                        YYDEBUGSTR, yydepth, yystate, yypath->state, (int)(yylvp - yylvals - yypath->lexeme));
#endif
    if (yyerrctx)
    {
        yyFreeState(yyerrctx);
        yyerrctx = NULL;
    }
    yylvp          = yylvals + yypath->lexeme;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yylpp          = yylpsns + yypath->lexeme;
#endif
    yylexp         = yylexemes + yypath->lexeme;
    yychar         = YYEMPTY;
    yystack.s_mark = yystack.s_base + (yypath->yystack.s_mark - yypath->yystack.s_base);
    memcpy (yystack.s_base, yypath->yystack.s_base, (size_t) (yystack.s_mark - yystack.s_base + 1) * sizeof(YYINT));
    yystack.l_mark = yystack.l_base + (yypath->yystack.l_mark - yypath->yystack.l_base);
    memcpy (yystack.l_base, yypath->yystack.l_base, (size_t) (yystack.l_mark - yystack.l_base + 1) * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yystack.p_mark = yystack.p_base + (yypath->yystack.p_mark - yypath->yystack.p_base);
    memcpy (yystack.p_base, yypath->yystack.p_base, (size_t) (yystack.p_mark - yystack.p_base + 1) * sizeof(YYLTYPE));
#endif
    yystate        = yypath->state;
    goto yyloop;
#endif /* YYBTYACC */

yyoverflow:
    YYERROR_CALL("yacc stack overflow");
#if YYBTYACC
    goto yyabort_nomem;
yyenomem:
    YYERROR_CALL("memory exhausted");
yyabort_nomem:
#endif /* YYBTYACC */
    yyresult = 2;
    goto yyreturn;

yyabort:
    yyresult = 1;
    goto yyreturn;

yyaccept:
#if YYBTYACC
    if (yyps->save) goto yyvalid;
#endif /* YYBTYACC */
    yyresult = 0;

yyreturn:
#if defined(YYDESTRUCT_CALL)
    if (yychar != YYEOF && yychar != YYEMPTY)
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        YYDESTRUCT_CALL("cleanup: discarding token", yychar, &yylval, &yylloc);
#else
        YYDESTRUCT_CALL("cleanup: discarding token", yychar, &yylval);
#endif /* defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED) */

    {
        YYSTYPE *pv;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        YYLTYPE *pp;

        for (pv = yystack.l_base, pp = yystack.p_base; pv <= yystack.l_mark; ++pv, ++pp)
             YYDESTRUCT_CALL("cleanup: discarding state",
                             yystos[*(yystack.s_base + (pv - yystack.l_base))], pv, pp);
#else
        for (pv = yystack.l_base; pv <= yystack.l_mark; ++pv)
             YYDESTRUCT_CALL("cleanup: discarding state",
                             yystos[*(yystack.s_base + (pv - yystack.l_base))], pv);
#endif /* defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED) */
    }
#endif /* defined(YYDESTRUCT_CALL) */

#if YYBTYACC
    if (yyerrctx)
    {
        yyFreeState(yyerrctx);
        yyerrctx = NULL;
    }
    while (yyps)
    {
        YYParseState *save = yyps;
        yyps = save->save;
        save->save = NULL;
        yyFreeState(save);
    }
    while (yypath)
    {
        YYParseState *save = yypath;
        yypath = save->save;
        save->save = NULL;
        yyFreeState(save);
    }
#endif /* YYBTYACC */
    yyfreestack(&yystack);
    return (yyresult);
}
