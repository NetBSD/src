/* A Bison parser, made by GNU Bison 3.7.6.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30706

/* Bison version string.  */
#define YYBISON_VERSION "3.7.6"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "zparser.y"

/*
 * zyparser.y -- yacc grammar for (DNS) zone files
 *
 * Copyright (c) 2001-2006, NLnet Labs. All rights reserved.
 *
 * See LICENSE for the license.
 *
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "dname.h"
#include "namedb.h"
#include "zonec.h"

/* these need to be global, otherwise they cannot be used inside yacc */
zparser_type *parser;

#ifdef __cplusplus
extern "C"
#endif /* __cplusplus */
int yywrap(void);

/* this hold the nxt bits */
static uint8_t nxtbits[16];
static int dlv_warn = 1;

/* 256 windows of 256 bits (32 bytes) */
/* still need to reset the bastard somewhere */
static uint8_t nsecbits[NSEC_WINDOW_COUNT][NSEC_WINDOW_BITS_SIZE];

/* hold the highest rcode seen in a NSEC rdata , BUG #106 */
uint16_t nsec_highest_rcode;

void yyerror(const char *message);

#ifdef NSEC3
/* parse nsec3 parameters and add the (first) rdata elements */
static void
nsec3_add_params(const char* hash_algo_str, const char* flag_str,
	const char* iter_str, const char* salt_str, int salt_len);
#endif /* NSEC3 */


#line 121 "zparser.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "zparser.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_T_A = 3,                        /* T_A  */
  YYSYMBOL_T_NS = 4,                       /* T_NS  */
  YYSYMBOL_T_MX = 5,                       /* T_MX  */
  YYSYMBOL_T_TXT = 6,                      /* T_TXT  */
  YYSYMBOL_T_CNAME = 7,                    /* T_CNAME  */
  YYSYMBOL_T_AAAA = 8,                     /* T_AAAA  */
  YYSYMBOL_T_PTR = 9,                      /* T_PTR  */
  YYSYMBOL_T_NXT = 10,                     /* T_NXT  */
  YYSYMBOL_T_KEY = 11,                     /* T_KEY  */
  YYSYMBOL_T_SOA = 12,                     /* T_SOA  */
  YYSYMBOL_T_SIG = 13,                     /* T_SIG  */
  YYSYMBOL_T_SRV = 14,                     /* T_SRV  */
  YYSYMBOL_T_CERT = 15,                    /* T_CERT  */
  YYSYMBOL_T_LOC = 16,                     /* T_LOC  */
  YYSYMBOL_T_MD = 17,                      /* T_MD  */
  YYSYMBOL_T_MF = 18,                      /* T_MF  */
  YYSYMBOL_T_MB = 19,                      /* T_MB  */
  YYSYMBOL_T_MG = 20,                      /* T_MG  */
  YYSYMBOL_T_MR = 21,                      /* T_MR  */
  YYSYMBOL_T_NULL = 22,                    /* T_NULL  */
  YYSYMBOL_T_WKS = 23,                     /* T_WKS  */
  YYSYMBOL_T_HINFO = 24,                   /* T_HINFO  */
  YYSYMBOL_T_MINFO = 25,                   /* T_MINFO  */
  YYSYMBOL_T_RP = 26,                      /* T_RP  */
  YYSYMBOL_T_AFSDB = 27,                   /* T_AFSDB  */
  YYSYMBOL_T_X25 = 28,                     /* T_X25  */
  YYSYMBOL_T_ISDN = 29,                    /* T_ISDN  */
  YYSYMBOL_T_RT = 30,                      /* T_RT  */
  YYSYMBOL_T_NSAP = 31,                    /* T_NSAP  */
  YYSYMBOL_T_NSAP_PTR = 32,                /* T_NSAP_PTR  */
  YYSYMBOL_T_PX = 33,                      /* T_PX  */
  YYSYMBOL_T_GPOS = 34,                    /* T_GPOS  */
  YYSYMBOL_T_EID = 35,                     /* T_EID  */
  YYSYMBOL_T_NIMLOC = 36,                  /* T_NIMLOC  */
  YYSYMBOL_T_ATMA = 37,                    /* T_ATMA  */
  YYSYMBOL_T_NAPTR = 38,                   /* T_NAPTR  */
  YYSYMBOL_T_KX = 39,                      /* T_KX  */
  YYSYMBOL_T_A6 = 40,                      /* T_A6  */
  YYSYMBOL_T_DNAME = 41,                   /* T_DNAME  */
  YYSYMBOL_T_SINK = 42,                    /* T_SINK  */
  YYSYMBOL_T_OPT = 43,                     /* T_OPT  */
  YYSYMBOL_T_APL = 44,                     /* T_APL  */
  YYSYMBOL_T_UINFO = 45,                   /* T_UINFO  */
  YYSYMBOL_T_UID = 46,                     /* T_UID  */
  YYSYMBOL_T_GID = 47,                     /* T_GID  */
  YYSYMBOL_T_UNSPEC = 48,                  /* T_UNSPEC  */
  YYSYMBOL_T_TKEY = 49,                    /* T_TKEY  */
  YYSYMBOL_T_TSIG = 50,                    /* T_TSIG  */
  YYSYMBOL_T_IXFR = 51,                    /* T_IXFR  */
  YYSYMBOL_T_AXFR = 52,                    /* T_AXFR  */
  YYSYMBOL_T_MAILB = 53,                   /* T_MAILB  */
  YYSYMBOL_T_MAILA = 54,                   /* T_MAILA  */
  YYSYMBOL_T_DS = 55,                      /* T_DS  */
  YYSYMBOL_T_DLV = 56,                     /* T_DLV  */
  YYSYMBOL_T_SSHFP = 57,                   /* T_SSHFP  */
  YYSYMBOL_T_RRSIG = 58,                   /* T_RRSIG  */
  YYSYMBOL_T_NSEC = 59,                    /* T_NSEC  */
  YYSYMBOL_T_DNSKEY = 60,                  /* T_DNSKEY  */
  YYSYMBOL_T_SPF = 61,                     /* T_SPF  */
  YYSYMBOL_T_NSEC3 = 62,                   /* T_NSEC3  */
  YYSYMBOL_T_IPSECKEY = 63,                /* T_IPSECKEY  */
  YYSYMBOL_T_DHCID = 64,                   /* T_DHCID  */
  YYSYMBOL_T_NSEC3PARAM = 65,              /* T_NSEC3PARAM  */
  YYSYMBOL_T_TLSA = 66,                    /* T_TLSA  */
  YYSYMBOL_T_URI = 67,                     /* T_URI  */
  YYSYMBOL_T_NID = 68,                     /* T_NID  */
  YYSYMBOL_T_L32 = 69,                     /* T_L32  */
  YYSYMBOL_T_L64 = 70,                     /* T_L64  */
  YYSYMBOL_T_LP = 71,                      /* T_LP  */
  YYSYMBOL_T_EUI48 = 72,                   /* T_EUI48  */
  YYSYMBOL_T_EUI64 = 73,                   /* T_EUI64  */
  YYSYMBOL_T_CAA = 74,                     /* T_CAA  */
  YYSYMBOL_T_CDS = 75,                     /* T_CDS  */
  YYSYMBOL_T_CDNSKEY = 76,                 /* T_CDNSKEY  */
  YYSYMBOL_T_OPENPGPKEY = 77,              /* T_OPENPGPKEY  */
  YYSYMBOL_T_CSYNC = 78,                   /* T_CSYNC  */
  YYSYMBOL_T_ZONEMD = 79,                  /* T_ZONEMD  */
  YYSYMBOL_T_AVC = 80,                     /* T_AVC  */
  YYSYMBOL_T_SMIMEA = 81,                  /* T_SMIMEA  */
  YYSYMBOL_T_SVCB = 82,                    /* T_SVCB  */
  YYSYMBOL_T_HTTPS = 83,                   /* T_HTTPS  */
  YYSYMBOL_DOLLAR_TTL = 84,                /* DOLLAR_TTL  */
  YYSYMBOL_DOLLAR_ORIGIN = 85,             /* DOLLAR_ORIGIN  */
  YYSYMBOL_NL = 86,                        /* NL  */
  YYSYMBOL_SP = 87,                        /* SP  */
  YYSYMBOL_QSTR = 88,                      /* QSTR  */
  YYSYMBOL_STR = 89,                       /* STR  */
  YYSYMBOL_PREV = 90,                      /* PREV  */
  YYSYMBOL_BITLAB = 91,                    /* BITLAB  */
  YYSYMBOL_T_TTL = 92,                     /* T_TTL  */
  YYSYMBOL_T_RRCLASS = 93,                 /* T_RRCLASS  */
  YYSYMBOL_URR = 94,                       /* URR  */
  YYSYMBOL_T_UTYPE = 95,                   /* T_UTYPE  */
  YYSYMBOL_96_ = 96,                       /* '.'  */
  YYSYMBOL_97_ = 97,                       /* '@'  */
  YYSYMBOL_YYACCEPT = 98,                  /* $accept  */
  YYSYMBOL_lines = 99,                     /* lines  */
  YYSYMBOL_line = 100,                     /* line  */
  YYSYMBOL_sp = 101,                       /* sp  */
  YYSYMBOL_str = 102,                      /* str  */
  YYSYMBOL_trail = 103,                    /* trail  */
  YYSYMBOL_ttl_directive = 104,            /* ttl_directive  */
  YYSYMBOL_origin_directive = 105,         /* origin_directive  */
  YYSYMBOL_rr = 106,                       /* rr  */
  YYSYMBOL_owner = 107,                    /* owner  */
  YYSYMBOL_classttl = 108,                 /* classttl  */
  YYSYMBOL_dname = 109,                    /* dname  */
  YYSYMBOL_abs_dname = 110,                /* abs_dname  */
  YYSYMBOL_label = 111,                    /* label  */
  YYSYMBOL_rel_dname = 112,                /* rel_dname  */
  YYSYMBOL_wire_dname = 113,               /* wire_dname  */
  YYSYMBOL_wire_abs_dname = 114,           /* wire_abs_dname  */
  YYSYMBOL_wire_label = 115,               /* wire_label  */
  YYSYMBOL_wire_rel_dname = 116,           /* wire_rel_dname  */
  YYSYMBOL_str_seq = 117,                  /* str_seq  */
  YYSYMBOL_concatenated_str_seq = 118,     /* concatenated_str_seq  */
  YYSYMBOL_nxt_seq = 119,                  /* nxt_seq  */
  YYSYMBOL_nsec_more = 120,                /* nsec_more  */
  YYSYMBOL_nsec_seq = 121,                 /* nsec_seq  */
  YYSYMBOL_str_sp_seq = 122,               /* str_sp_seq  */
  YYSYMBOL_str_dot_seq = 123,              /* str_dot_seq  */
  YYSYMBOL_unquoted_dotted_str = 124,      /* unquoted_dotted_str  */
  YYSYMBOL_dotted_str = 125,               /* dotted_str  */
  YYSYMBOL_type_and_rdata = 126,           /* type_and_rdata  */
  YYSYMBOL_rdata_a = 127,                  /* rdata_a  */
  YYSYMBOL_rdata_domain_name = 128,        /* rdata_domain_name  */
  YYSYMBOL_rdata_soa = 129,                /* rdata_soa  */
  YYSYMBOL_rdata_wks = 130,                /* rdata_wks  */
  YYSYMBOL_rdata_hinfo = 131,              /* rdata_hinfo  */
  YYSYMBOL_rdata_minfo = 132,              /* rdata_minfo  */
  YYSYMBOL_rdata_mx = 133,                 /* rdata_mx  */
  YYSYMBOL_rdata_txt = 134,                /* rdata_txt  */
  YYSYMBOL_rdata_rp = 135,                 /* rdata_rp  */
  YYSYMBOL_rdata_afsdb = 136,              /* rdata_afsdb  */
  YYSYMBOL_rdata_x25 = 137,                /* rdata_x25  */
  YYSYMBOL_rdata_isdn = 138,               /* rdata_isdn  */
  YYSYMBOL_rdata_rt = 139,                 /* rdata_rt  */
  YYSYMBOL_rdata_nsap = 140,               /* rdata_nsap  */
  YYSYMBOL_rdata_px = 141,                 /* rdata_px  */
  YYSYMBOL_rdata_aaaa = 142,               /* rdata_aaaa  */
  YYSYMBOL_rdata_loc = 143,                /* rdata_loc  */
  YYSYMBOL_rdata_nxt = 144,                /* rdata_nxt  */
  YYSYMBOL_rdata_srv = 145,                /* rdata_srv  */
  YYSYMBOL_rdata_naptr = 146,              /* rdata_naptr  */
  YYSYMBOL_rdata_kx = 147,                 /* rdata_kx  */
  YYSYMBOL_rdata_cert = 148,               /* rdata_cert  */
  YYSYMBOL_rdata_apl = 149,                /* rdata_apl  */
  YYSYMBOL_rdata_apl_seq = 150,            /* rdata_apl_seq  */
  YYSYMBOL_rdata_ds = 151,                 /* rdata_ds  */
  YYSYMBOL_rdata_dlv = 152,                /* rdata_dlv  */
  YYSYMBOL_rdata_sshfp = 153,              /* rdata_sshfp  */
  YYSYMBOL_rdata_dhcid = 154,              /* rdata_dhcid  */
  YYSYMBOL_rdata_rrsig = 155,              /* rdata_rrsig  */
  YYSYMBOL_rdata_nsec = 156,               /* rdata_nsec  */
  YYSYMBOL_rdata_nsec3 = 157,              /* rdata_nsec3  */
  YYSYMBOL_rdata_nsec3_param = 158,        /* rdata_nsec3_param  */
  YYSYMBOL_rdata_tlsa = 159,               /* rdata_tlsa  */
  YYSYMBOL_rdata_smimea = 160,             /* rdata_smimea  */
  YYSYMBOL_rdata_dnskey = 161,             /* rdata_dnskey  */
  YYSYMBOL_rdata_ipsec_base = 162,         /* rdata_ipsec_base  */
  YYSYMBOL_rdata_ipseckey = 163,           /* rdata_ipseckey  */
  YYSYMBOL_rdata_nid = 164,                /* rdata_nid  */
  YYSYMBOL_rdata_l32 = 165,                /* rdata_l32  */
  YYSYMBOL_rdata_l64 = 166,                /* rdata_l64  */
  YYSYMBOL_rdata_lp = 167,                 /* rdata_lp  */
  YYSYMBOL_rdata_eui48 = 168,              /* rdata_eui48  */
  YYSYMBOL_rdata_eui64 = 169,              /* rdata_eui64  */
  YYSYMBOL_rdata_uri = 170,                /* rdata_uri  */
  YYSYMBOL_rdata_caa = 171,                /* rdata_caa  */
  YYSYMBOL_rdata_openpgpkey = 172,         /* rdata_openpgpkey  */
  YYSYMBOL_rdata_csync = 173,              /* rdata_csync  */
  YYSYMBOL_rdata_zonemd = 174,             /* rdata_zonemd  */
  YYSYMBOL_svcparam = 175,                 /* svcparam  */
  YYSYMBOL_svcparams = 176,                /* svcparams  */
  YYSYMBOL_rdata_svcb_base = 177,          /* rdata_svcb_base  */
  YYSYMBOL_rdata_svcb = 178,               /* rdata_svcb  */
  YYSYMBOL_rdata_unknown = 179             /* rdata_unknown  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                            \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
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
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1374

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  98
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  82
/* YYNRULES -- Number of rules.  */
#define YYNRULES  261
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  630

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   350


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,    96,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    97,     2,     2,     2,     2,     2,
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
      95
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,    93,    93,    94,    97,    98,    99,   100,   108,   116,
     136,   140,   141,   144,   144,   146,   147,   150,   160,   171,
     177,   184,   189,   196,   200,   205,   210,   215,   222,   223,
     244,   248,   252,   262,   276,   283,   284,   302,   303,   327,
     334,   346,   358,   375,   376,   388,   392,   396,   401,   405,
     410,   414,   418,   429,   430,   435,   444,   456,   465,   476,
     479,   482,   496,   497,   504,   505,   521,   522,   537,   538,
     543,   553,   569,   569,   576,   577,   578,   579,   580,   581,
     586,   587,   593,   594,   595,   596,   597,   598,   604,   605,
     606,   607,   609,   610,   611,   612,   613,   614,   615,   616,
     617,   618,   619,   620,   621,   622,   623,   624,   625,   626,
     627,   628,   629,   630,   631,   632,   633,   634,   635,   636,
     637,   638,   639,   640,   641,   642,   643,   644,   645,   646,
     647,   648,   649,   650,   651,   652,   653,   654,   655,   656,
     657,   658,   659,   660,   661,   662,   663,   664,   665,   666,
     667,   668,   669,   670,   671,   672,   673,   674,   675,   676,
     677,   678,   679,   680,   681,   682,   683,   684,   685,   686,
     687,   688,   689,   690,   691,   692,   693,   694,   695,   696,
     697,   698,   699,   700,   701,   702,   703,   704,   705,   706,
     707,   708,   709,   710,   711,   712,   713,   714,   715,   716,
     717,   729,   735,   742,   755,   762,   769,   777,   784,   791,
     799,   807,   814,   818,   826,   834,   846,   854,   860,   866,
     874,   884,   896,   904,   914,   917,   921,   927,   936,   945,
     954,   960,   975,   985,  1000,  1010,  1019,  1028,  1037,  1082,
    1086,  1090,  1097,  1104,  1111,  1118,  1124,  1131,  1140,  1149,
    1156,  1167,  1176,  1181,  1187,  1188,  1191,  1198,  1202,  1205,
    1211,  1215
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "T_A", "T_NS", "T_MX",
  "T_TXT", "T_CNAME", "T_AAAA", "T_PTR", "T_NXT", "T_KEY", "T_SOA",
  "T_SIG", "T_SRV", "T_CERT", "T_LOC", "T_MD", "T_MF", "T_MB", "T_MG",
  "T_MR", "T_NULL", "T_WKS", "T_HINFO", "T_MINFO", "T_RP", "T_AFSDB",
  "T_X25", "T_ISDN", "T_RT", "T_NSAP", "T_NSAP_PTR", "T_PX", "T_GPOS",
  "T_EID", "T_NIMLOC", "T_ATMA", "T_NAPTR", "T_KX", "T_A6", "T_DNAME",
  "T_SINK", "T_OPT", "T_APL", "T_UINFO", "T_UID", "T_GID", "T_UNSPEC",
  "T_TKEY", "T_TSIG", "T_IXFR", "T_AXFR", "T_MAILB", "T_MAILA", "T_DS",
  "T_DLV", "T_SSHFP", "T_RRSIG", "T_NSEC", "T_DNSKEY", "T_SPF", "T_NSEC3",
  "T_IPSECKEY", "T_DHCID", "T_NSEC3PARAM", "T_TLSA", "T_URI", "T_NID",
  "T_L32", "T_L64", "T_LP", "T_EUI48", "T_EUI64", "T_CAA", "T_CDS",
  "T_CDNSKEY", "T_OPENPGPKEY", "T_CSYNC", "T_ZONEMD", "T_AVC", "T_SMIMEA",
  "T_SVCB", "T_HTTPS", "DOLLAR_TTL", "DOLLAR_ORIGIN", "NL", "SP", "QSTR",
  "STR", "PREV", "BITLAB", "T_TTL", "T_RRCLASS", "URR", "T_UTYPE", "'.'",
  "'@'", "$accept", "lines", "line", "sp", "str", "trail", "ttl_directive",
  "origin_directive", "rr", "owner", "classttl", "dname", "abs_dname",
  "label", "rel_dname", "wire_dname", "wire_abs_dname", "wire_label",
  "wire_rel_dname", "str_seq", "concatenated_str_seq", "nxt_seq",
  "nsec_more", "nsec_seq", "str_sp_seq", "str_dot_seq",
  "unquoted_dotted_str", "dotted_str", "type_and_rdata", "rdata_a",
  "rdata_domain_name", "rdata_soa", "rdata_wks", "rdata_hinfo",
  "rdata_minfo", "rdata_mx", "rdata_txt", "rdata_rp", "rdata_afsdb",
  "rdata_x25", "rdata_isdn", "rdata_rt", "rdata_nsap", "rdata_px",
  "rdata_aaaa", "rdata_loc", "rdata_nxt", "rdata_srv", "rdata_naptr",
  "rdata_kx", "rdata_cert", "rdata_apl", "rdata_apl_seq", "rdata_ds",
  "rdata_dlv", "rdata_sshfp", "rdata_dhcid", "rdata_rrsig", "rdata_nsec",
  "rdata_nsec3", "rdata_nsec3_param", "rdata_tlsa", "rdata_smimea",
  "rdata_dnskey", "rdata_ipsec_base", "rdata_ipseckey", "rdata_nid",
  "rdata_l32", "rdata_l64", "rdata_lp", "rdata_eui48", "rdata_eui64",
  "rdata_uri", "rdata_caa", "rdata_openpgpkey", "rdata_csync",
  "rdata_zonemd", "svcparam", "svcparams", "rdata_svcb_base", "rdata_svcb",
  "rdata_unknown", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_int16 yytoknum[] =
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
     345,   346,   347,   348,   349,   350,    46,    64
};
#endif

#define YYPACT_NINF (-473)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -473,   115,  -473,   -70,   -60,   -60,  -473,  -473,  -473,  -473,
     -51,  -473,  -473,  -473,  -473,    51,  -473,  -473,  -473,  -473,
      98,   -60,  -473,  -473,   -59,  -473,   106,    67,  -473,  -473,
    -473,   -60,   -60,   678,    20,    60,    73,    73,   -81,   -79,
      21,   -60,   -60,   -60,   -60,   -60,   -60,   -60,   -60,   -60,
     -60,   -60,   -60,   -60,   -60,   -60,   -60,   -60,   -60,   -60,
     -60,   -60,   -60,   -60,   -60,   -60,   -60,   -60,   -60,   -60,
     -60,   -60,   -60,    73,   -60,   -60,   -60,   -60,   -60,   -60,
     -60,   -60,   -60,   -60,   -60,   -60,   -60,   -60,   -60,   -60,
     -60,   -60,   -60,   -60,   -60,   -60,   -60,   -60,   -60,   -60,
     -60,   -60,   -60,   -60,   121,  -473,  -473,  -473,   111,  -473,
    -473,  -473,   -60,   -60,   136,    56,   -55,   148,    56,   136,
      56,    56,   -55,    56,   -55,   -55,   -55,   166,    56,    56,
      56,    56,    56,   136,   -55,    56,    56,   -55,   -55,   -55,
     -55,   -55,   -55,   -55,   -55,    56,    31,  -473,   -55,   -55,
     -55,   -55,    78,   -55,   148,   -55,   -55,   -55,   -55,   -55,
     -55,   -55,   -55,   -55,   -55,   -55,   -55,   -55,   -55,   -55,
     -55,   -55,   -55,   148,   -55,   -55,   -55,   -58,    38,  -473,
      20,    20,  -473,  -473,    17,  -473,    30,    73,  -473,  -473,
      73,  -473,  -473,   -60,  -473,  -473,    14,   192,    30,  -473,
    -473,  -473,  -473,    73,  -473,  -473,  -473,  -473,   -60,  -473,
    -473,   -60,  -473,  -473,   -60,  -473,  -473,   -60,  -473,  -473,
     -60,  -473,  -473,   -60,  -473,  -473,  -473,  -473,   -77,  -473,
    -473,  -473,  -473,  -473,  -473,  -473,  -473,  -473,  -473,  -473,
    -473,   -60,  -473,  -473,   -60,  -473,  -473,   -60,  -473,  -473,
     -60,  -473,  -473,   -60,  -473,  -473,    73,  -473,  -473,    73,
    -473,  -473,   -60,  -473,  -473,  -473,    90,  -473,  -473,   -60,
    -473,  -473,   -60,  -473,  -473,   -60,  -473,  -473,  -473,  -473,
    -473,  -473,    73,  -473,   -60,  -473,  -473,   -60,  -473,  -473,
     -60,  -473,  -473,  -473,  -473,  -473,  -473,  -473,   122,  -473,
    -473,    33,  -473,  -473,  -473,  -473,  -473,  -473,   -60,  -473,
    -473,   -60,    73,  -473,  -473,  -473,    73,  -473,  -473,   -60,
    -473,  -473,   -60,  -473,  -473,   -60,  -473,  -473,   -60,  -473,
    -473,   -60,  -473,  -473,   -60,  -473,  -473,   -60,  -473,  -473,
      73,  -473,  -473,    73,  -473,  -473,   -60,  -473,  -473,  -473,
    -473,  -473,  -473,    73,  -473,  -473,   -60,  -473,  -473,   -60,
    -473,  -473,  -473,  -473,   -60,  -473,  -473,   -60,    73,  -473,
    -473,  -473,  -473,  -473,  -473,    48,   106,    57,  -473,  -473,
      67,    30,    14,    44,  -473,  -473,   106,   106,    67,   106,
     106,   106,   126,   204,  -473,   106,   106,    67,    67,    67,
    -473,   204,  -473,    67,   126,  -473,    67,   106,    67,    82,
    -473,   106,   106,   106,  -473,   208,  -473,   126,   106,   106,
     204,  -473,   204,  -473,   106,   106,   106,   176,   176,   176,
      67,  -473,  -473,   106,  -473,   106,   106,   106,    67,    82,
    -473,  -473,    73,  -473,    73,    30,    14,    30,  -473,    73,
     -60,   -60,   -60,   -60,   -60,  -473,  -473,   -60,    73,    73,
      73,    73,    73,    73,  -473,   -60,   -60,    73,  -473,   -60,
     -60,   -60,  -473,   208,   122,  -473,  -473,   -60,   -60,    73,
    -473,   -60,   -60,   -60,    73,    73,    73,    73,   -60,   122,
     -60,   -60,  -473,    69,  -473,    73,   204,  -473,  -473,    30,
     204,  -473,   106,   106,   106,   106,   106,   188,  -473,  -473,
    -473,  -473,  -473,  -473,    67,   106,  -473,   106,   106,   106,
    -473,  -473,   106,   106,  -473,   106,   106,   176,  -473,  -473,
    -473,  -473,   176,  -473,   106,   106,  -473,    82,  -473,    73,
    -473,   -60,   -60,   -60,   -60,   -60,   -77,    73,   -60,   -60,
     -60,    73,   -60,   -60,   -60,   -60,    73,    73,   -60,   -60,
    -473,  -473,   106,   106,   106,    67,   106,  -473,  -473,   106,
     106,   106,  -473,   106,   176,   106,   106,  -473,  -473,   106,
     106,    73,   -60,   -60,    73,    73,   -60,    73,    73,   -60,
    -473,    73,    73,    73,    73,  -473,   106,   106,  -473,  -473,
     106,  -473,  -473,   106,  -473,  -473,  -473,  -473,   -60,   -60,
     -60,   122,   106,   106,    67,  -473,   -60,   -60,    73,   106,
     106,  -473,    73,   -60,  -473,    92,   -60,   106,    73,  -473
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       2,     0,     1,     0,     0,     0,     4,    11,    14,    13,
      22,    34,    30,    31,     3,     0,    33,     7,     8,     9,
      23,     0,    28,    35,    29,    10,     0,     0,     6,     5,
      12,     0,     0,     0,    21,    32,     0,     0,     0,    25,
      24,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    20,    36,    15,     0,    17,
      18,    19,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   146,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    16,
      26,    27,    73,    68,     0,    69,    72,     0,    74,    75,
       0,    76,    77,     0,   100,   101,    46,     0,    45,   102,
     103,    82,    83,     0,   130,   131,    94,    95,     0,   134,
     135,     0,   126,   127,     0,    84,    85,     0,   124,   125,
       0,   136,   137,     0,   142,   143,    54,    53,     0,   132,
     133,    78,    79,    80,    81,    86,    87,    88,    89,    90,
      91,     0,    92,    93,     0,    96,    97,     0,    98,    99,
       0,   108,   109,     0,   110,   111,     0,   112,   113,     0,
     114,   115,     0,   120,   121,    66,     0,   122,   123,     0,
     128,   129,     0,   138,   139,     0,   140,   141,   144,   145,
     225,   147,     0,   148,     0,   149,   150,     0,   151,   152,
       0,   153,   154,   155,   156,    39,    40,    42,     0,    37,
      43,    38,   157,   158,   163,   164,   104,   105,     0,   159,
     160,     0,     0,   116,   117,    64,     0,   118,   119,     0,
     161,   162,     0,   165,   166,     0,   197,   198,     0,   169,
     170,     0,   171,   172,     0,   173,   174,     0,   175,   176,
       0,   177,   178,     0,   179,   180,     0,   181,   182,   183,
     184,   185,   186,     0,   187,   188,     0,   189,   190,     0,
     191,   192,   106,   107,     0,   167,   168,     0,     0,   193,
     194,   195,   196,   199,   200,     0,     0,    70,   201,   202,
       0,    47,    48,     0,   208,   217,     0,     0,     0,     0,
       0,     0,     0,     0,   218,     0,     0,     0,     0,     0,
     211,     0,   212,     0,     0,   215,     0,     0,     0,     0,
     224,     0,     0,     0,    62,     0,   232,    41,     0,     0,
       0,   240,     0,   230,     0,     0,     0,     0,     0,     0,
       0,   245,   246,     0,   249,     0,     0,     0,     0,     0,
     258,   261,     0,    71,     0,    49,    51,    50,    57,     0,
       0,     0,     0,     0,     0,    56,    55,     0,     0,     0,
       0,     0,     0,     0,    67,     0,     0,     0,   226,     0,
       0,     0,    60,     0,     0,    63,    44,     0,     0,     0,
      65,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   256,   253,   254,     0,     0,   260,   207,    52,
       0,   219,     0,     0,     0,     0,     0,     0,   205,   206,
     209,   210,   213,   214,     0,     0,   222,     0,     0,     0,
      59,    61,     0,     0,   239,     0,     0,     0,   241,   242,
     243,   244,     0,   250,     0,     0,   252,     0,   257,     0,
      58,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     255,   259,     0,     0,     0,     0,     0,   204,   216,     0,
       0,     0,   229,     0,     0,     0,     0,   247,   248,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     238,     0,     0,     0,     0,   237,     0,     0,   220,   223,
       0,   227,   228,     0,   234,   235,   251,   236,     0,     0,
       0,     0,     0,     0,     0,   233,     0,     0,     0,     0,
       0,   221,     0,     0,   203,     0,     0,     0,     0,   231
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -473,  -473,  -473,    -1,   504,   746,  -473,  -473,  -473,  -473,
    -473,     0,   135,   138,   155,  -440,  -473,  -230,  -473,  -473,
    -290,  -473,  -255,  -472,   -64,  -473,   -12,    -5,  -473,  -473,
    -107,  -473,  -473,  -473,  -473,  -473,  -147,  -473,  -473,  -473,
    -473,  -473,  -473,  -473,  -473,  -473,  -473,  -473,  -473,  -473,
    -473,  -473,  -473,    52,  -473,  -473,  -473,    70,  -473,  -473,
    -473,  -473,  -473,  -141,  -473,  -473,  -473,  -473,  -473,  -473,
    -473,  -473,  -473,  -473,  -473,  -473,  -473,  -311,  -473,  -473,
      53,   672
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,    14,   108,    16,   109,    17,    18,    19,    20,
      33,   190,    22,    23,    24,   298,   299,   300,   301,   197,
     228,   449,   475,   416,   316,   266,   186,   493,   105,   188,
     191,   215,   242,   245,   248,   194,   199,   251,   254,   257,
     260,   263,   267,   270,   204,   229,   209,   221,   273,   276,
     224,   281,   282,   285,   288,   291,   317,   218,   302,   309,
     320,   323,   365,   212,   312,   313,   329,   332,   335,   338,
     341,   344,   326,   347,   354,   357,   360,   494,   495,   368,
     369,   189
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      15,    21,   521,    26,    27,   107,     7,   306,    30,   107,
       7,   201,   304,   206,   112,    35,    25,   533,   375,   392,
      34,   231,   233,   235,   237,   239,   362,     7,   351,    30,
      39,    40,    30,     8,     9,    28,   184,    35,   278,   184,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   183,     7,   198,   353,    30,    30,   187,
     185,   180,   181,   113,   203,     2,     3,   179,    30,   182,
     183,   208,   178,   214,   374,   184,   377,   185,   241,   417,
     179,    30,   446,   183,   441,   247,   250,    29,    30,   615,
     185,   280,   198,    30,     8,     9,   443,    11,     8,     9,
     184,    11,    12,    13,    30,     8,     9,   536,    11,   107,
       7,   198,    37,    12,    13,    30,     8,     9,   179,    30,
     182,   183,   184,   106,   295,   296,   107,     7,   185,    30,
       8,     9,    38,   376,   381,   626,   404,   476,   295,   296,
      31,    32,   380,    30,     8,     9,   383,   179,    30,     4,
       5,     6,     7,     8,     9,    10,    11,   386,   414,   415,
     387,    12,    13,   388,     8,     9,   389,   546,   520,   390,
     349,   293,   391,    30,   182,   183,   560,   393,     0,   371,
     184,     0,   185,     0,     0,    30,   196,   183,     0,     0,
     395,     0,   184,   396,   185,     0,   397,     0,     0,   398,
       0,     0,   399,    30,     8,     9,     0,     0,   401,     0,
     184,   403,   226,    30,   182,   183,     0,     0,   406,     0,
       0,   407,   185,     0,   408,    30,     8,     9,   107,     7,
     382,   409,     0,   411,   226,     0,   412,     0,     0,   413,
     179,    30,     8,     9,   472,   473,     8,     9,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   418,     0,     0,
     419,   420,     0,     0,     0,   422,     0,     0,   424,     0,
       0,   425,     0,     0,   426,     0,     0,   427,     0,     0,
     428,     0,     0,   429,     0,     0,   430,     0,     0,     0,
       0,     0,     0,     0,     0,   433,     0,     0,     0,     0,
       0,     0,   422,     0,     0,   435,   479,     0,   436,     0,
       0,     0,     0,   437,     0,     0,   438,   439,     0,     0,
     445,   447,     0,     0,     0,     0,     0,     0,     0,     0,
     444,     0,     0,     0,     0,     0,     0,     0,   451,     0,
       0,     0,     0,     0,     0,     0,     0,   459,   460,   461,
       0,     0,     0,   463,   468,     0,   465,     0,   467,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   484,   485,   486,     0,     0,     0,     0,     0,
     487,     0,   539,     0,   499,     0,     0,     0,   492,     0,
       0,   496,     0,     0,     0,     0,     0,     0,   500,   502,
     503,   504,   505,   506,     0,   551,   507,     0,     0,     0,
       0,     0,     0,     0,   514,   515,     0,     0,   517,   518,
     519,     0,     0,     0,     0,     0,   522,   523,   422,     0,
     525,   526,   527,     0,     0,     0,     0,   532,     0,   534,
     535,     0,     0,     0,   537,     0,     0,     0,   581,     0,
       0,     0,   585,     0,     0,     0,   587,   588,     0,     0,
       0,     0,   592,     0,   547,   593,   594,     0,     0,     0,
       0,     0,   556,     0,     0,     0,     0,   557,     0,     0,
      36,     0,     0,     0,     0,     0,     0,   104,   422,     0,
     562,   563,   564,   565,   566,   393,     0,   569,   570,   571,
     422,   573,   574,   575,   576,     0,     0,   579,   580,     0,
       0,     0,     0,   628,     0,   584,     0,     0,     0,   590,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     422,   596,   597,     0,   422,   600,   422,   422,   603,     0,
       0,   422,   422,   422,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   612,   613,   614,
       0,     0,     0,     0,   618,   619,   620,     0,     0,     0,
     193,     0,   625,     0,     0,   627,   211,   422,   217,   220,
     223,   227,     0,     0,     0,     0,     0,     0,   244,     0,
       0,   253,   256,   259,   262,   265,   269,   272,   275,     0,
       0,     0,   284,   287,   290,   217,   297,   211,     0,   308,
     311,   315,   319,   322,   325,   328,   331,   334,   337,   340,
     343,   346,   284,   211,   315,   356,   359,     0,   364,   367,
     367,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
       0,    60,    61,    62,    63,    64,    65,    66,    67,    68,
       0,    69,     0,     0,     0,     0,    70,    71,     0,    72,
       0,     0,    73,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,     0,     0,     0,     0,     8,     9,     0,     0,
       0,     0,     0,   103,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   110,   111,     0,     0,   192,   195,   200,
     202,   205,   207,   210,   213,   216,   219,   222,   225,   230,
     232,   234,   236,   238,   240,   243,   246,   249,   252,   255,
     258,   261,   264,   268,   271,   274,   277,   279,   283,   147,
     286,   289,   292,   294,   303,   305,   307,   310,   314,   318,
     321,   324,   327,   330,   333,   336,   339,   342,   345,   348,
     350,   352,   355,   358,   361,   363,   366,   370,   372,   373,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     442,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     448,   450,     0,   452,   453,   454,   455,   456,     0,   457,
     458,     0,     0,     0,     0,   462,     0,     0,   464,     0,
       0,   466,     0,     0,     0,   469,   470,   471,     0,   474,
       0,   297,   477,   478,   315,     0,   480,     0,   481,   482,
     483,     0,     0,   378,     0,     0,   379,   488,     0,   489,
     490,   491,     0,   384,     0,     0,     0,     0,     0,   385,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   394,     0,     0,   474,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     315,     0,   400,     0,   540,   402,   541,   542,   543,   544,
     545,   227,   405,     0,     0,     0,     0,     0,     0,   548,
       0,   549,   550,   315,     0,     0,   552,   553,   410,   554,
     555,     0,     0,     0,     0,     0,     0,     0,   558,   559,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   421,     0,
       0,     0,   423,     0,     0,     0,   315,   582,   583,     0,
     315,     0,     0,   586,   315,   315,     0,   589,     0,   591,
     315,     0,     0,   315,   315,     0,   431,     0,     0,   432,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   434,
     608,   609,     0,     0,   610,     0,     0,   611,     0,     0,
       0,     0,     0,     0,   440,     0,   616,   617,     0,     0,
       0,     0,     0,   622,   623,     0,     0,     0,     0,   297,
       0,   315,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   497,     0,
     498,     0,     0,     0,     0,   501,     0,     0,     0,     0,
       0,     0,     0,     0,   508,   509,   510,   511,   512,   513,
       0,     0,     0,   516,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   524,     0,     0,     0,     0,
     528,   529,   530,   531,     0,     0,     0,     0,     0,     0,
       0,   538,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   561,     0,     0,     0,     0,
       0,     0,   567,   568,     0,     0,     0,   572,     0,     0,
       0,     0,   577,   578,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   595,     0,     0,
     598,   599,     0,   601,   602,     0,     0,   604,   605,   606,
     607,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   621,     0,     0,     0,   624,     0,
       0,     0,     0,     0,   629
};

static const yytype_int16 yycheck[] =
{
       1,     1,   474,     4,     5,    86,    87,   154,    87,    86,
      87,   118,   153,   120,    93,    96,    86,   489,     1,    96,
      21,   128,   129,   130,   131,   132,   173,    87,   169,    87,
      31,    32,    87,    88,    89,    86,    94,    96,   145,    94,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,    89,    87,   117,   170,    87,    87,   114,
      96,   112,   113,    92,   119,     0,     1,    86,    87,    88,
      89,   121,     1,   123,    86,    94,    96,    96,   133,    96,
      86,    87,    88,    89,    86,   135,   136,    86,    87,   611,
      96,   146,   154,    87,    88,    89,    89,    91,    88,    89,
      94,    91,    96,    97,    87,    88,    89,    88,    91,    86,
      87,   173,    27,    96,    97,    87,    88,    89,    86,    87,
      88,    89,    94,    35,    96,    97,    86,    87,    96,    87,
      88,    89,    27,   184,   196,   625,    96,   417,    96,    97,
      92,    93,   193,    87,    88,    89,   197,    86,    87,    84,
      85,    86,    87,    88,    89,    90,    91,   208,    86,    87,
     211,    96,    97,   214,    88,    89,   217,   507,   473,   220,
     168,   151,   223,    87,    88,    89,   537,   228,    -1,   176,
      94,    -1,    96,    -1,    -1,    87,    88,    89,    -1,    -1,
     241,    -1,    94,   244,    96,    -1,   247,    -1,    -1,   250,
      -1,    -1,   253,    87,    88,    89,    -1,    -1,   259,    -1,
      94,   262,    96,    87,    88,    89,    -1,    -1,   269,    -1,
      -1,   272,    96,    -1,   275,    87,    88,    89,    86,    87,
      88,   282,    -1,   284,    96,    -1,   287,    -1,    -1,   290,
      86,    87,    88,    89,    86,    87,    88,    89,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   308,    -1,    -1,
     311,   312,    -1,    -1,    -1,   316,    -1,    -1,   319,    -1,
      -1,   322,    -1,    -1,   325,    -1,    -1,   328,    -1,    -1,
     331,    -1,    -1,   334,    -1,    -1,   337,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   346,    -1,    -1,    -1,    -1,
      -1,    -1,   353,    -1,    -1,   356,   420,    -1,   359,    -1,
      -1,    -1,    -1,   364,    -1,    -1,   367,   368,    -1,    -1,
     382,   383,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     380,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   388,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   397,   398,   399,
      -1,    -1,    -1,   403,   409,    -1,   406,    -1,   408,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   427,   428,   429,    -1,    -1,    -1,    -1,    -1,
     430,    -1,   496,    -1,   446,    -1,    -1,    -1,   438,    -1,
      -1,   442,    -1,    -1,    -1,    -1,    -1,    -1,   449,   450,
     451,   452,   453,   454,    -1,   519,   457,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   465,   466,    -1,    -1,   469,   470,
     471,    -1,    -1,    -1,    -1,    -1,   477,   478,   479,    -1,
     481,   482,   483,    -1,    -1,    -1,    -1,   488,    -1,   490,
     491,    -1,    -1,    -1,   495,    -1,    -1,    -1,   562,    -1,
      -1,    -1,   566,    -1,    -1,    -1,   570,   571,    -1,    -1,
      -1,    -1,   576,    -1,   514,   579,   580,    -1,    -1,    -1,
      -1,    -1,   527,    -1,    -1,    -1,    -1,   532,    -1,    -1,
      26,    -1,    -1,    -1,    -1,    -1,    -1,    33,   539,    -1,
     541,   542,   543,   544,   545,   546,    -1,   548,   549,   550,
     551,   552,   553,   554,   555,    -1,    -1,   558,   559,    -1,
      -1,    -1,    -1,   627,    -1,   565,    -1,    -1,    -1,   574,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     581,   582,   583,    -1,   585,   586,   587,   588,   589,    -1,
      -1,   592,   593,   594,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   608,   609,   610,
      -1,    -1,    -1,    -1,   614,   616,   617,    -1,    -1,    -1,
     116,    -1,   623,    -1,    -1,   626,   122,   628,   124,   125,
     126,   127,    -1,    -1,    -1,    -1,    -1,    -1,   134,    -1,
      -1,   137,   138,   139,   140,   141,   142,   143,   144,    -1,
      -1,    -1,   148,   149,   150,   151,   152,   153,    -1,   155,
     156,   157,   158,   159,   160,   161,   162,   163,   164,   165,
     166,   167,   168,   169,   170,   171,   172,    -1,   174,   175,
     176,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      -1,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      -1,    33,    -1,    -1,    -1,    -1,    38,    39,    -1,    41,
      -1,    -1,    44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    -1,    -1,    -1,    -1,    88,    89,    -1,    -1,
      -1,    -1,    -1,    95,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    37,    38,    -1,    -1,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,    73,
     148,   149,   150,   151,   152,   153,   154,   155,   156,   157,
     158,   159,   160,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   175,   176,   177,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     376,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     386,   387,    -1,   389,   390,   391,   392,   393,    -1,   395,
     396,    -1,    -1,    -1,    -1,   401,    -1,    -1,   404,    -1,
      -1,   407,    -1,    -1,    -1,   411,   412,   413,    -1,   415,
      -1,   417,   418,   419,   420,    -1,   422,    -1,   424,   425,
     426,    -1,    -1,   187,    -1,    -1,   190,   433,    -1,   435,
     436,   437,    -1,   197,    -1,    -1,    -1,    -1,    -1,   203,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   228,    -1,    -1,   473,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     496,    -1,   256,    -1,   500,   259,   502,   503,   504,   505,
     506,   507,   266,    -1,    -1,    -1,    -1,    -1,    -1,   515,
      -1,   517,   518,   519,    -1,    -1,   522,   523,   282,   525,
     526,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   534,   535,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   312,    -1,
      -1,    -1,   316,    -1,    -1,    -1,   562,   563,   564,    -1,
     566,    -1,    -1,   569,   570,   571,    -1,   573,    -1,   575,
     576,    -1,    -1,   579,   580,    -1,   340,    -1,    -1,   343,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   353,
     596,   597,    -1,    -1,   600,    -1,    -1,   603,    -1,    -1,
      -1,    -1,    -1,    -1,   368,    -1,   612,   613,    -1,    -1,
      -1,    -1,    -1,   619,   620,    -1,    -1,    -1,    -1,   625,
      -1,   627,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   442,    -1,
     444,    -1,    -1,    -1,    -1,   449,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   458,   459,   460,   461,   462,   463,
      -1,    -1,    -1,   467,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   479,    -1,    -1,    -1,    -1,
     484,   485,   486,   487,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   495,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   539,    -1,    -1,    -1,    -1,
      -1,    -1,   546,   547,    -1,    -1,    -1,   551,    -1,    -1,
      -1,    -1,   556,   557,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   581,    -1,    -1,
     584,   585,    -1,   587,   588,    -1,    -1,   591,   592,   593,
     594,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   618,    -1,    -1,    -1,   622,    -1,
      -1,    -1,    -1,    -1,   628
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    99,     0,     1,    84,    85,    86,    87,    88,    89,
      90,    91,    96,    97,   100,   101,   102,   104,   105,   106,
     107,   109,   110,   111,   112,    86,   101,   101,    86,    86,
      87,    92,    93,   108,   101,    96,   102,   110,   112,   101,
     101,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    33,
      38,    39,    41,    44,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    95,   102,   126,   111,    86,   101,   103,
     103,   103,    93,    92,   101,   101,   101,   101,   101,   101,
     101,   101,   101,   101,   101,   101,   101,   101,   101,   101,
     101,   101,   101,   101,   101,   101,   101,   101,   101,   101,
     101,   101,   101,   101,   101,   101,   101,   103,   101,   101,
     101,   101,   101,   101,   101,   101,   101,   101,   101,   101,
     101,   101,   101,   101,   101,   101,   101,   101,   101,   101,
     101,   101,   101,   101,   101,   101,   101,   101,     1,    86,
     101,   101,    88,    89,    94,    96,   124,   125,   127,   179,
     109,   128,   179,   102,   133,   179,    88,   117,   124,   134,
     179,   128,   179,   125,   142,   179,   128,   179,   109,   144,
     179,   102,   161,   179,   109,   129,   179,   102,   155,   179,
     102,   145,   179,   102,   148,   179,    96,   102,   118,   143,
     179,   128,   179,   128,   179,   128,   179,   128,   179,   128,
     179,   125,   130,   179,   102,   131,   179,   109,   132,   179,
     109,   135,   179,   102,   136,   179,   102,   137,   179,   102,
     138,   179,   102,   139,   179,   102,   123,   140,   179,   102,
     141,   179,   102,   146,   179,   102,   147,   179,   128,   179,
     125,   149,   150,   179,   102,   151,   179,   102,   152,   179,
     102,   153,   179,   155,   179,    96,    97,   102,   113,   114,
     115,   116,   156,   179,   161,   179,   134,   179,   102,   157,
     179,   102,   162,   163,   179,   102,   122,   154,   179,   102,
     158,   179,   102,   159,   179,   102,   170,   179,   102,   164,
     179,   102,   165,   179,   102,   166,   179,   102,   167,   179,
     102,   168,   179,   102,   169,   179,   102,   171,   179,   151,
     179,   161,   179,   122,   172,   179,   102,   173,   179,   102,
     174,   179,   134,   179,   102,   160,   179,   102,   177,   178,
     179,   178,   179,   179,    86,     1,   101,    96,   103,   103,
     101,   124,    88,   101,   103,   103,   101,   101,   101,   101,
     101,   101,    96,   101,   103,   101,   101,   101,   101,   101,
     103,   101,   103,   101,    96,   103,   101,   101,   101,   101,
     103,   101,   101,   101,    86,    87,   121,    96,   101,   101,
     101,   103,   101,   103,   101,   101,   101,   101,   101,   101,
     101,   103,   103,   101,   103,   101,   101,   101,   101,   101,
     103,    86,   102,    89,   109,   124,    88,   124,   102,   119,
     102,   109,   102,   102,   102,   102,   102,   102,   102,   109,
     109,   109,   102,   109,   102,   109,   102,   109,   125,   102,
     102,   102,    86,    87,   102,   120,   115,   102,   102,   122,
     102,   102,   102,   102,   125,   125,   125,   109,   102,   102,
     102,   102,   109,   125,   175,   176,   101,   103,   103,   124,
     101,   103,   101,   101,   101,   101,   101,   101,   103,   103,
     103,   103,   103,   103,   101,   101,   103,   101,   101,   101,
     120,   121,   101,   101,   103,   101,   101,   101,   103,   103,
     103,   103,   101,   121,   101,   101,    88,   101,   103,   122,
     102,   102,   102,   102,   102,   102,   118,   109,   102,   102,
     102,   122,   102,   102,   102,   102,   125,   125,   102,   102,
     175,   103,   101,   101,   101,   101,   101,   103,   103,   101,
     101,   101,   103,   101,   101,   101,   101,   103,   103,   101,
     101,   122,   102,   102,   109,   122,   102,   122,   122,   102,
     125,   102,   122,   122,   122,   103,   101,   101,   103,   103,
     101,   103,   103,   101,   103,   103,   103,   103,   102,   102,
     102,   102,   101,   101,   101,   121,   102,   102,   109,   101,
     101,   103,   102,   102,   103,   101,   113,   101,   122,   103
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    98,    99,    99,   100,   100,   100,   100,   100,   100,
     100,   101,   101,   102,   102,   103,   103,   104,   105,   105,
     106,   107,   107,   108,   108,   108,   108,   108,   109,   109,
     110,   110,   110,   111,   111,   112,   112,   113,   113,   114,
     114,   114,   115,   116,   116,   117,   117,   117,   117,   117,
     117,   117,   117,   118,   118,   118,   118,   119,   119,   120,
     120,   120,   121,   121,   122,   122,   123,   123,   124,   124,
     124,   124,   125,   125,   126,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   126,   126,   126,   126,   126,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   150,   151,   152,   153,
     154,   155,   156,   157,   158,   159,   160,   161,   162,   163,
     163,   164,   165,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   175,   176,   176,   177,   178,   178,   179,
     179,   179
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     1,     2,     2,     1,     1,     1,
       2,     1,     2,     1,     1,     1,     2,     4,     4,     4,
       3,     2,     1,     0,     2,     2,     4,     4,     1,     1,
       1,     1,     2,     1,     1,     1,     3,     1,     1,     1,
       1,     2,     1,     1,     3,     1,     1,     2,     2,     3,
       3,     3,     4,     1,     1,     3,     3,     1,     3,     2,
       1,     2,     1,     2,     1,     3,     1,     3,     1,     1,
       2,     3,     1,     1,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     2,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     2,     2,    14,     6,     4,     4,     4,     2,     4,
       4,     2,     2,     4,     4,     2,     6,     2,     2,     4,
       8,    12,     4,     8,     2,     1,     3,     8,     8,     6,
       2,    18,     2,    10,     8,     8,     8,     8,     7,     4,
       2,     4,     4,     4,     4,     2,     2,     6,     6,     2,
       4,     8,     2,     1,     1,     3,     3,     4,     2,     6,
       4,     3
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
# ifndef YY_LOCATION_PRINT
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yykind < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yykind], *yyvaluep);
# endif
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
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






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

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
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
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
      if (yytable_value_is_error (yyn))
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
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
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
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 6: /* line: PREV NL  */
#line 99 "zparser.y"
                        {}
#line 1883 "zparser.c"
    break;

  case 7: /* line: ttl_directive  */
#line 101 "zparser.y"
        {
	    region_free_all(parser->rr_region);
	    parser->current_rr.type = 0;
	    parser->current_rr.rdata_count = 0;
	    parser->current_rr.rdatas = parser->temporary_rdatas;
	    parser->error_occurred = 0;
    }
#line 1895 "zparser.c"
    break;

  case 8: /* line: origin_directive  */
#line 109 "zparser.y"
        {
	    region_free_all(parser->rr_region);
	    parser->current_rr.type = 0;
	    parser->current_rr.rdata_count = 0;
	    parser->current_rr.rdatas = parser->temporary_rdatas;
	    parser->error_occurred = 0;
    }
#line 1907 "zparser.c"
    break;

  case 9: /* line: rr  */
#line 117 "zparser.y"
    {	/* rr should be fully parsed */
	    if (!parser->error_occurred) {
			    parser->current_rr.rdatas
				    =(rdata_atom_type *)region_alloc_array_init(
					    parser->region,
					    parser->current_rr.rdatas,
					    parser->current_rr.rdata_count,
					    sizeof(rdata_atom_type));

			    process_rr();
	    }

	    region_free_all(parser->rr_region);

	    parser->current_rr.type = 0;
	    parser->current_rr.rdata_count = 0;
	    parser->current_rr.rdatas = parser->temporary_rdatas;
	    parser->error_occurred = 0;
    }
#line 1931 "zparser.c"
    break;

  case 17: /* ttl_directive: DOLLAR_TTL sp str trail  */
#line 151 "zparser.y"
    {
	    parser->default_ttl = zparser_ttl2int((yyvsp[-1].data).str, &(parser->error_occurred));
	    if (parser->error_occurred == 1) {
		    parser->default_ttl = DEFAULT_TTL;
			parser->error_occurred = 0;
	    }
    }
#line 1943 "zparser.c"
    break;

  case 18: /* origin_directive: DOLLAR_ORIGIN sp abs_dname trail  */
#line 161 "zparser.y"
    {
	    /* if previous origin is unused, remove it, do not leak it */
	    if(parser->origin != error_domain && parser->origin != (yyvsp[-1].domain)) {
		/* protect $3 from deletion, because deldomain walks up */
		(yyvsp[-1].domain)->usage ++;
	    	domain_table_deldomain(parser->db, parser->origin);
		(yyvsp[-1].domain)->usage --;
	    }
	    parser->origin = (yyvsp[-1].domain);
    }
#line 1958 "zparser.c"
    break;

  case 19: /* origin_directive: DOLLAR_ORIGIN sp rel_dname trail  */
#line 172 "zparser.y"
    {
	    zc_error_prev_line("$ORIGIN directive requires absolute domain name");
    }
#line 1966 "zparser.c"
    break;

  case 20: /* rr: owner classttl type_and_rdata  */
#line 178 "zparser.y"
    {
	    parser->current_rr.owner = (yyvsp[-2].domain);
	    parser->current_rr.type = (yyvsp[0].type);
    }
#line 1975 "zparser.c"
    break;

  case 21: /* owner: dname sp  */
#line 185 "zparser.y"
    {
	    parser->prev_dname = (yyvsp[-1].domain);
	    (yyval.domain) = (yyvsp[-1].domain);
    }
#line 1984 "zparser.c"
    break;

  case 22: /* owner: PREV  */
#line 190 "zparser.y"
    {
	    (yyval.domain) = parser->prev_dname;
    }
#line 1992 "zparser.c"
    break;

  case 23: /* classttl: %empty  */
#line 196 "zparser.y"
    {
	    parser->current_rr.ttl = parser->default_ttl;
	    parser->current_rr.klass = parser->default_class;
    }
#line 2001 "zparser.c"
    break;

  case 24: /* classttl: T_RRCLASS sp  */
#line 201 "zparser.y"
    {
	    parser->current_rr.ttl = parser->default_ttl;
	    parser->current_rr.klass = (yyvsp[-1].klass);
    }
#line 2010 "zparser.c"
    break;

  case 25: /* classttl: T_TTL sp  */
#line 206 "zparser.y"
    {
	    parser->current_rr.ttl = (yyvsp[-1].ttl);
	    parser->current_rr.klass = parser->default_class;
    }
#line 2019 "zparser.c"
    break;

  case 26: /* classttl: T_TTL sp T_RRCLASS sp  */
#line 211 "zparser.y"
    {
	    parser->current_rr.ttl = (yyvsp[-3].ttl);
	    parser->current_rr.klass = (yyvsp[-1].klass);
    }
#line 2028 "zparser.c"
    break;

  case 27: /* classttl: T_RRCLASS sp T_TTL sp  */
#line 216 "zparser.y"
    {
	    parser->current_rr.ttl = (yyvsp[-1].ttl);
	    parser->current_rr.klass = (yyvsp[-3].klass);
    }
#line 2037 "zparser.c"
    break;

  case 29: /* dname: rel_dname  */
#line 224 "zparser.y"
    {
	    if ((yyvsp[0].dname) == error_dname) {
		    (yyval.domain) = error_domain;
	    } else if(parser->origin == error_domain) {
		    zc_error("cannot concatenate origin to domain name, because origin failed to parse");
		    (yyval.domain) = error_domain;
	    } else if ((yyvsp[0].dname)->name_size + domain_dname(parser->origin)->name_size - 1 > MAXDOMAINLEN) {
		    zc_error("domain name exceeds %d character limit", MAXDOMAINLEN);
		    (yyval.domain) = error_domain;
	    } else {
		    (yyval.domain) = domain_table_insert(
			    parser->db->domains,
			    dname_concatenate(
				    parser->rr_region,
				    (yyvsp[0].dname),
				    domain_dname(parser->origin)));
	    }
    }
#line 2060 "zparser.c"
    break;

  case 30: /* abs_dname: '.'  */
#line 245 "zparser.y"
    {
	    (yyval.domain) = parser->db->domains->root;
    }
#line 2068 "zparser.c"
    break;

  case 31: /* abs_dname: '@'  */
#line 249 "zparser.y"
    {
	    (yyval.domain) = parser->origin;
    }
#line 2076 "zparser.c"
    break;

  case 32: /* abs_dname: rel_dname '.'  */
#line 253 "zparser.y"
    {
	    if ((yyvsp[-1].dname) != error_dname) {
		    (yyval.domain) = domain_table_insert(parser->db->domains, (yyvsp[-1].dname));
	    } else {
		    (yyval.domain) = error_domain;
	    }
    }
#line 2088 "zparser.c"
    break;

  case 33: /* label: str  */
#line 263 "zparser.y"
    {
	    if ((yyvsp[0].data).len > MAXLABELLEN) {
		    zc_error("label exceeds %d character limit", MAXLABELLEN);
		    (yyval.dname) = error_dname;
	    } else if ((yyvsp[0].data).len <= 0) {
		    zc_error("zero label length");
		    (yyval.dname) = error_dname;
	    } else {
		    (yyval.dname) = dname_make_from_label(parser->rr_region,
					       (uint8_t *) (yyvsp[0].data).str,
					       (yyvsp[0].data).len);
	    }
    }
#line 2106 "zparser.c"
    break;

  case 34: /* label: BITLAB  */
#line 277 "zparser.y"
    {
	    zc_error("bitlabels are now deprecated. RFC2673 is obsoleted.");
	    (yyval.dname) = error_dname;
    }
#line 2115 "zparser.c"
    break;

  case 36: /* rel_dname: rel_dname '.' label  */
#line 285 "zparser.y"
    {
	    if ((yyvsp[-2].dname) == error_dname || (yyvsp[0].dname) == error_dname) {
		    (yyval.dname) = error_dname;
	    } else if ((yyvsp[-2].dname)->name_size + (yyvsp[0].dname)->name_size - 1 > MAXDOMAINLEN) {
		    zc_error("domain name exceeds %d character limit",
			     MAXDOMAINLEN);
		    (yyval.dname) = error_dname;
	    } else {
		    (yyval.dname) = dname_concatenate(parser->rr_region, (yyvsp[-2].dname), (yyvsp[0].dname));
	    }
    }
#line 2131 "zparser.c"
    break;

  case 38: /* wire_dname: wire_rel_dname  */
#line 304 "zparser.y"
    {
	    /* terminate in root label and copy the origin in there */
	    if(parser->origin && domain_dname(parser->origin)) {
		    (yyval.data).len = (yyvsp[0].data).len + domain_dname(parser->origin)->name_size;
		    if ((yyval.data).len > MAXDOMAINLEN)
			    zc_error("domain name exceeds %d character limit",
				     MAXDOMAINLEN);
		    (yyval.data).str = (char *) region_alloc(parser->rr_region, (yyval.data).len);
		    memmove((yyval.data).str, (yyvsp[0].data).str, (yyvsp[0].data).len);
		    memmove((yyval.data).str + (yyvsp[0].data).len, dname_name(domain_dname(parser->origin)),
			domain_dname(parser->origin)->name_size);
	    } else {
		    (yyval.data).len = (yyvsp[0].data).len + 1;
		    if ((yyval.data).len > MAXDOMAINLEN)
			    zc_error("domain name exceeds %d character limit",
				     MAXDOMAINLEN);
		    (yyval.data).str = (char *) region_alloc(parser->rr_region, (yyval.data).len);
		    memmove((yyval.data).str, (yyvsp[0].data).str, (yyvsp[0].data).len);
		    (yyval.data).str[ (yyvsp[0].data).len ] = 0;
	    }
    }
#line 2157 "zparser.c"
    break;

  case 39: /* wire_abs_dname: '.'  */
#line 328 "zparser.y"
    {
	    char *result = (char *) region_alloc(parser->rr_region, 1);
	    result[0] = 0;
	    (yyval.data).str = result;
	    (yyval.data).len = 1;
    }
#line 2168 "zparser.c"
    break;

  case 40: /* wire_abs_dname: '@'  */
#line 335 "zparser.y"
    {
	    if(parser->origin && domain_dname(parser->origin)) {
		    (yyval.data).len = domain_dname(parser->origin)->name_size;
		    (yyval.data).str = (char *) region_alloc(parser->rr_region, (yyval.data).len);
		    memmove((yyval.data).str, dname_name(domain_dname(parser->origin)), (yyval.data).len);
	    } else {
		    (yyval.data).len = 1;
		    (yyval.data).str = (char *) region_alloc(parser->rr_region, (yyval.data).len);
		    (yyval.data).str[0] = 0;
	    }
    }
#line 2184 "zparser.c"
    break;

  case 41: /* wire_abs_dname: wire_rel_dname '.'  */
#line 347 "zparser.y"
    {
	    (yyval.data).len = (yyvsp[-1].data).len + 1;
	    if ((yyval.data).len > MAXDOMAINLEN)
		    zc_error("domain name exceeds %d character limit",
			     MAXDOMAINLEN);
	    (yyval.data).str = (char *) region_alloc(parser->rr_region, (yyval.data).len);
	    memcpy((yyval.data).str, (yyvsp[-1].data).str, (yyvsp[-1].data).len);
	    (yyval.data).str[(yyvsp[-1].data).len] = 0;
    }
#line 2198 "zparser.c"
    break;

  case 42: /* wire_label: str  */
#line 359 "zparser.y"
    {
	    char *result = (char *) region_alloc(parser->rr_region,
						 (yyvsp[0].data).len + 1);

	    if ((yyvsp[0].data).len > MAXLABELLEN)
		    zc_error("label exceeds %d character limit", MAXLABELLEN);

	    /* make label anyway */
	    result[0] = (yyvsp[0].data).len;
	    memmove(result+1, (yyvsp[0].data).str, (yyvsp[0].data).len);

	    (yyval.data).str = result;
	    (yyval.data).len = (yyvsp[0].data).len + 1;
    }
#line 2217 "zparser.c"
    break;

  case 44: /* wire_rel_dname: wire_rel_dname '.' wire_label  */
#line 377 "zparser.y"
    {
	    (yyval.data).len = (yyvsp[-2].data).len + (yyvsp[0].data).len;
	    if ((yyval.data).len > MAXDOMAINLEN)
		    zc_error("domain name exceeds %d character limit",
			     MAXDOMAINLEN);
	    (yyval.data).str = (char *) region_alloc(parser->rr_region, (yyval.data).len);
	    memmove((yyval.data).str, (yyvsp[-2].data).str, (yyvsp[-2].data).len);
	    memmove((yyval.data).str + (yyvsp[-2].data).len, (yyvsp[0].data).str, (yyvsp[0].data).len);
    }
#line 2231 "zparser.c"
    break;

  case 45: /* str_seq: unquoted_dotted_str  */
#line 389 "zparser.y"
    {
	    zadd_rdata_txt_wireformat(zparser_conv_text(parser->rr_region, (yyvsp[0].data).str, (yyvsp[0].data).len), 1);
    }
#line 2239 "zparser.c"
    break;

  case 46: /* str_seq: QSTR  */
#line 393 "zparser.y"
    {
	    zadd_rdata_txt_wireformat(zparser_conv_text(parser->rr_region, (yyvsp[0].data).str, (yyvsp[0].data).len), 1);
    }
#line 2247 "zparser.c"
    break;

  case 47: /* str_seq: QSTR unquoted_dotted_str  */
#line 397 "zparser.y"
    {
	    zadd_rdata_txt_wireformat(zparser_conv_text(parser->rr_region, (yyvsp[-1].data).str, (yyvsp[-1].data).len), 1);
	    zadd_rdata_txt_wireformat(zparser_conv_text(parser->rr_region, (yyvsp[0].data).str, (yyvsp[0].data).len), 0);
    }
#line 2256 "zparser.c"
    break;

  case 48: /* str_seq: str_seq QSTR  */
#line 402 "zparser.y"
    {
	    zadd_rdata_txt_wireformat(zparser_conv_text(parser->rr_region, (yyvsp[0].data).str, (yyvsp[0].data).len), 0);
    }
#line 2264 "zparser.c"
    break;

  case 49: /* str_seq: str_seq QSTR unquoted_dotted_str  */
#line 406 "zparser.y"
    {
	    zadd_rdata_txt_wireformat(zparser_conv_text(parser->rr_region, (yyvsp[-1].data).str, (yyvsp[-1].data).len), 0);
	    zadd_rdata_txt_wireformat(zparser_conv_text(parser->rr_region, (yyvsp[0].data).str, (yyvsp[0].data).len), 0);
    }
#line 2273 "zparser.c"
    break;

  case 50: /* str_seq: str_seq sp unquoted_dotted_str  */
#line 411 "zparser.y"
    {
	    zadd_rdata_txt_wireformat(zparser_conv_text(parser->rr_region, (yyvsp[0].data).str, (yyvsp[0].data).len), 0);
    }
#line 2281 "zparser.c"
    break;

  case 51: /* str_seq: str_seq sp QSTR  */
#line 415 "zparser.y"
    {
	    zadd_rdata_txt_wireformat(zparser_conv_text(parser->rr_region, (yyvsp[0].data).str, (yyvsp[0].data).len), 0);
    }
#line 2289 "zparser.c"
    break;

  case 52: /* str_seq: str_seq sp QSTR unquoted_dotted_str  */
#line 419 "zparser.y"
    {
	    zadd_rdata_txt_wireformat(zparser_conv_text(parser->rr_region, (yyvsp[-1].data).str, (yyvsp[-1].data).len), 0);
	    zadd_rdata_txt_wireformat(zparser_conv_text(parser->rr_region, (yyvsp[0].data).str, (yyvsp[0].data).len), 0);
    }
#line 2298 "zparser.c"
    break;

  case 54: /* concatenated_str_seq: '.'  */
#line 431 "zparser.y"
    {
	    (yyval.data).len = 1;
	    (yyval.data).str = region_strdup(parser->rr_region, ".");
    }
#line 2307 "zparser.c"
    break;

  case 55: /* concatenated_str_seq: concatenated_str_seq sp str  */
#line 436 "zparser.y"
    {
	    (yyval.data).len = (yyvsp[-2].data).len + (yyvsp[0].data).len + 1;
	    (yyval.data).str = (char *) region_alloc(parser->rr_region, (yyval.data).len + 1);
	    memcpy((yyval.data).str, (yyvsp[-2].data).str, (yyvsp[-2].data).len);
	    memcpy((yyval.data).str + (yyvsp[-2].data).len, " ", 1);
	    memcpy((yyval.data).str + (yyvsp[-2].data).len + 1, (yyvsp[0].data).str, (yyvsp[0].data).len);
	    (yyval.data).str[(yyval.data).len] = '\0';
    }
#line 2320 "zparser.c"
    break;

  case 56: /* concatenated_str_seq: concatenated_str_seq '.' str  */
#line 445 "zparser.y"
    {
	    (yyval.data).len = (yyvsp[-2].data).len + (yyvsp[0].data).len + 1;
	    (yyval.data).str = (char *) region_alloc(parser->rr_region, (yyval.data).len + 1);
	    memcpy((yyval.data).str, (yyvsp[-2].data).str, (yyvsp[-2].data).len);
	    memcpy((yyval.data).str + (yyvsp[-2].data).len, ".", 1);
	    memcpy((yyval.data).str + (yyvsp[-2].data).len + 1, (yyvsp[0].data).str, (yyvsp[0].data).len);
	    (yyval.data).str[(yyval.data).len] = '\0';
    }
#line 2333 "zparser.c"
    break;

  case 57: /* nxt_seq: str  */
#line 457 "zparser.y"
    {
	    uint16_t type = rrtype_from_string((yyvsp[0].data).str);
	    if (type != 0 && type < 128) {
		    set_bit(nxtbits, type);
	    } else {
		    zc_error("bad type %d in NXT record", (int) type);
	    }
    }
#line 2346 "zparser.c"
    break;

  case 58: /* nxt_seq: nxt_seq sp str  */
#line 466 "zparser.y"
    {
	    uint16_t type = rrtype_from_string((yyvsp[0].data).str);
	    if (type != 0 && type < 128) {
		    set_bit(nxtbits, type);
	    } else {
		    zc_error("bad type %d in NXT record", (int) type);
	    }
    }
#line 2359 "zparser.c"
    break;

  case 59: /* nsec_more: SP nsec_more  */
#line 477 "zparser.y"
    {
    }
#line 2366 "zparser.c"
    break;

  case 60: /* nsec_more: NL  */
#line 480 "zparser.y"
    {
    }
#line 2373 "zparser.c"
    break;

  case 61: /* nsec_more: str nsec_seq  */
#line 483 "zparser.y"
    {
	    uint16_t type = rrtype_from_string((yyvsp[-1].data).str);
	    if (type != 0) {
                    if (type > nsec_highest_rcode) {
                            nsec_highest_rcode = type;
                    }
		    set_bitnsec(nsecbits, type);
	    } else {
		    zc_error("bad type %d in NSEC record", (int) type);
	    }
    }
#line 2389 "zparser.c"
    break;

  case 65: /* str_sp_seq: str_sp_seq sp str  */
#line 506 "zparser.y"
    {
	    char *result = (char *) region_alloc(parser->rr_region,
						 (yyvsp[-2].data).len + (yyvsp[0].data).len + 1);
	    memcpy(result, (yyvsp[-2].data).str, (yyvsp[-2].data).len);
	    memcpy(result + (yyvsp[-2].data).len, (yyvsp[0].data).str, (yyvsp[0].data).len);
	    (yyval.data).str = result;
	    (yyval.data).len = (yyvsp[-2].data).len + (yyvsp[0].data).len;
	    (yyval.data).str[(yyval.data).len] = '\0';
    }
#line 2403 "zparser.c"
    break;

  case 67: /* str_dot_seq: str_dot_seq '.' str  */
#line 523 "zparser.y"
    {
	    char *result = (char *) region_alloc(parser->rr_region,
						 (yyvsp[-2].data).len + (yyvsp[0].data).len + 1);
	    memcpy(result, (yyvsp[-2].data).str, (yyvsp[-2].data).len);
	    memcpy(result + (yyvsp[-2].data).len, (yyvsp[0].data).str, (yyvsp[0].data).len);
	    (yyval.data).str = result;
	    (yyval.data).len = (yyvsp[-2].data).len + (yyvsp[0].data).len;
	    (yyval.data).str[(yyval.data).len] = '\0';
    }
#line 2417 "zparser.c"
    break;

  case 69: /* unquoted_dotted_str: '.'  */
#line 539 "zparser.y"
    {
	(yyval.data).str = ".";
	(yyval.data).len = 1;
    }
#line 2426 "zparser.c"
    break;

  case 70: /* unquoted_dotted_str: unquoted_dotted_str '.'  */
#line 544 "zparser.y"
    {
	    char *result = (char *) region_alloc(parser->rr_region,
						 (yyvsp[-1].data).len + 2);
	    memcpy(result, (yyvsp[-1].data).str, (yyvsp[-1].data).len);
	    result[(yyvsp[-1].data).len] = '.';
	    (yyval.data).str = result;
	    (yyval.data).len = (yyvsp[-1].data).len + 1;
	    (yyval.data).str[(yyval.data).len] = '\0';
    }
#line 2440 "zparser.c"
    break;

  case 71: /* unquoted_dotted_str: unquoted_dotted_str '.' STR  */
#line 554 "zparser.y"
    {
	    char *result = (char *) region_alloc(parser->rr_region,
						 (yyvsp[-2].data).len + (yyvsp[0].data).len + 2);
	    memcpy(result, (yyvsp[-2].data).str, (yyvsp[-2].data).len);
	    result[(yyvsp[-2].data).len] = '.';
	    memcpy(result + (yyvsp[-2].data).len + 1, (yyvsp[0].data).str, (yyvsp[0].data).len);
	    (yyval.data).str = result;
	    (yyval.data).len = (yyvsp[-2].data).len + (yyvsp[0].data).len + 1;
	    (yyval.data).str[(yyval.data).len] = '\0';
    }
#line 2455 "zparser.c"
    break;

  case 75: /* type_and_rdata: T_A sp rdata_unknown  */
#line 577 "zparser.y"
                             { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2461 "zparser.c"
    break;

  case 77: /* type_and_rdata: T_NS sp rdata_unknown  */
#line 579 "zparser.y"
                              { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2467 "zparser.c"
    break;

  case 78: /* type_and_rdata: T_MD sp rdata_domain_name  */
#line 580 "zparser.y"
                                  { zc_warning_prev_line("MD is obsolete"); }
#line 2473 "zparser.c"
    break;

  case 79: /* type_and_rdata: T_MD sp rdata_unknown  */
#line 582 "zparser.y"
    {
	    zc_warning_prev_line("MD is obsolete");
	    (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown));
    }
#line 2482 "zparser.c"
    break;

  case 80: /* type_and_rdata: T_MF sp rdata_domain_name  */
#line 586 "zparser.y"
                                  { zc_warning_prev_line("MF is obsolete"); }
#line 2488 "zparser.c"
    break;

  case 81: /* type_and_rdata: T_MF sp rdata_unknown  */
#line 588 "zparser.y"
    {
	    zc_warning_prev_line("MF is obsolete");
	    (yyval.type) = (yyvsp[-2].type);
	    parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown));
    }
#line 2498 "zparser.c"
    break;

  case 83: /* type_and_rdata: T_CNAME sp rdata_unknown  */
#line 594 "zparser.y"
                                 { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2504 "zparser.c"
    break;

  case 85: /* type_and_rdata: T_SOA sp rdata_unknown  */
#line 596 "zparser.y"
                               { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2510 "zparser.c"
    break;

  case 86: /* type_and_rdata: T_MB sp rdata_domain_name  */
#line 597 "zparser.y"
                                  { zc_warning_prev_line("MB is obsolete"); }
#line 2516 "zparser.c"
    break;

  case 87: /* type_and_rdata: T_MB sp rdata_unknown  */
#line 599 "zparser.y"
    {
	    zc_warning_prev_line("MB is obsolete");
	    (yyval.type) = (yyvsp[-2].type);
	    parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown));
    }
#line 2526 "zparser.c"
    break;

  case 89: /* type_and_rdata: T_MG sp rdata_unknown  */
#line 605 "zparser.y"
                              { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2532 "zparser.c"
    break;

  case 91: /* type_and_rdata: T_MR sp rdata_unknown  */
#line 607 "zparser.y"
                              { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2538 "zparser.c"
    break;

  case 93: /* type_and_rdata: T_WKS sp rdata_unknown  */
#line 610 "zparser.y"
                               { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2544 "zparser.c"
    break;

  case 95: /* type_and_rdata: T_PTR sp rdata_unknown  */
#line 612 "zparser.y"
                               { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2550 "zparser.c"
    break;

  case 97: /* type_and_rdata: T_HINFO sp rdata_unknown  */
#line 614 "zparser.y"
                                 { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2556 "zparser.c"
    break;

  case 99: /* type_and_rdata: T_MINFO sp rdata_unknown  */
#line 616 "zparser.y"
                                 { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2562 "zparser.c"
    break;

  case 101: /* type_and_rdata: T_MX sp rdata_unknown  */
#line 618 "zparser.y"
                              { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2568 "zparser.c"
    break;

  case 103: /* type_and_rdata: T_TXT sp rdata_unknown  */
#line 620 "zparser.y"
                               { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2574 "zparser.c"
    break;

  case 105: /* type_and_rdata: T_SPF sp rdata_unknown  */
#line 622 "zparser.y"
                               { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2580 "zparser.c"
    break;

  case 107: /* type_and_rdata: T_AVC sp rdata_unknown  */
#line 624 "zparser.y"
                               { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2586 "zparser.c"
    break;

  case 109: /* type_and_rdata: T_RP sp rdata_unknown  */
#line 626 "zparser.y"
                              { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2592 "zparser.c"
    break;

  case 111: /* type_and_rdata: T_AFSDB sp rdata_unknown  */
#line 628 "zparser.y"
                                 { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2598 "zparser.c"
    break;

  case 113: /* type_and_rdata: T_X25 sp rdata_unknown  */
#line 630 "zparser.y"
                               { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2604 "zparser.c"
    break;

  case 115: /* type_and_rdata: T_ISDN sp rdata_unknown  */
#line 632 "zparser.y"
                                { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2610 "zparser.c"
    break;

  case 117: /* type_and_rdata: T_IPSECKEY sp rdata_unknown  */
#line 634 "zparser.y"
                                    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2616 "zparser.c"
    break;

  case 119: /* type_and_rdata: T_DHCID sp rdata_unknown  */
#line 636 "zparser.y"
                                 { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2622 "zparser.c"
    break;

  case 121: /* type_and_rdata: T_RT sp rdata_unknown  */
#line 638 "zparser.y"
                              { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2628 "zparser.c"
    break;

  case 123: /* type_and_rdata: T_NSAP sp rdata_unknown  */
#line 640 "zparser.y"
                                { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2634 "zparser.c"
    break;

  case 125: /* type_and_rdata: T_SIG sp rdata_unknown  */
#line 642 "zparser.y"
                               { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2640 "zparser.c"
    break;

  case 127: /* type_and_rdata: T_KEY sp rdata_unknown  */
#line 644 "zparser.y"
                               { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2646 "zparser.c"
    break;

  case 129: /* type_and_rdata: T_PX sp rdata_unknown  */
#line 646 "zparser.y"
                              { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2652 "zparser.c"
    break;

  case 131: /* type_and_rdata: T_AAAA sp rdata_unknown  */
#line 648 "zparser.y"
                                { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2658 "zparser.c"
    break;

  case 133: /* type_and_rdata: T_LOC sp rdata_unknown  */
#line 650 "zparser.y"
                               { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2664 "zparser.c"
    break;

  case 135: /* type_and_rdata: T_NXT sp rdata_unknown  */
#line 652 "zparser.y"
                               { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2670 "zparser.c"
    break;

  case 137: /* type_and_rdata: T_SRV sp rdata_unknown  */
#line 654 "zparser.y"
                               { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2676 "zparser.c"
    break;

  case 139: /* type_and_rdata: T_NAPTR sp rdata_unknown  */
#line 656 "zparser.y"
                                 { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2682 "zparser.c"
    break;

  case 141: /* type_and_rdata: T_KX sp rdata_unknown  */
#line 658 "zparser.y"
                              { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2688 "zparser.c"
    break;

  case 143: /* type_and_rdata: T_CERT sp rdata_unknown  */
#line 660 "zparser.y"
                                { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2694 "zparser.c"
    break;

  case 145: /* type_and_rdata: T_DNAME sp rdata_unknown  */
#line 662 "zparser.y"
                                 { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2700 "zparser.c"
    break;

  case 148: /* type_and_rdata: T_APL sp rdata_unknown  */
#line 665 "zparser.y"
                               { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2706 "zparser.c"
    break;

  case 150: /* type_and_rdata: T_DS sp rdata_unknown  */
#line 667 "zparser.y"
                              { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2712 "zparser.c"
    break;

  case 151: /* type_and_rdata: T_DLV sp rdata_dlv  */
#line 668 "zparser.y"
                           { if (dlv_warn) { dlv_warn = 0; zc_warning_prev_line("DLV is experimental"); } }
#line 2718 "zparser.c"
    break;

  case 152: /* type_and_rdata: T_DLV sp rdata_unknown  */
#line 669 "zparser.y"
                               { if (dlv_warn) { dlv_warn = 0; zc_warning_prev_line("DLV is experimental"); } (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2724 "zparser.c"
    break;

  case 154: /* type_and_rdata: T_SSHFP sp rdata_unknown  */
#line 671 "zparser.y"
                                 { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); check_sshfp(); }
#line 2730 "zparser.c"
    break;

  case 156: /* type_and_rdata: T_RRSIG sp rdata_unknown  */
#line 673 "zparser.y"
                                 { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2736 "zparser.c"
    break;

  case 158: /* type_and_rdata: T_NSEC sp rdata_unknown  */
#line 675 "zparser.y"
                                { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2742 "zparser.c"
    break;

  case 160: /* type_and_rdata: T_NSEC3 sp rdata_unknown  */
#line 677 "zparser.y"
                                 { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2748 "zparser.c"
    break;

  case 162: /* type_and_rdata: T_NSEC3PARAM sp rdata_unknown  */
#line 679 "zparser.y"
                                      { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2754 "zparser.c"
    break;

  case 164: /* type_and_rdata: T_DNSKEY sp rdata_unknown  */
#line 681 "zparser.y"
                                  { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2760 "zparser.c"
    break;

  case 166: /* type_and_rdata: T_TLSA sp rdata_unknown  */
#line 683 "zparser.y"
                                { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2766 "zparser.c"
    break;

  case 168: /* type_and_rdata: T_SMIMEA sp rdata_unknown  */
#line 685 "zparser.y"
                                  { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2772 "zparser.c"
    break;

  case 170: /* type_and_rdata: T_NID sp rdata_unknown  */
#line 687 "zparser.y"
                               { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2778 "zparser.c"
    break;

  case 172: /* type_and_rdata: T_L32 sp rdata_unknown  */
#line 689 "zparser.y"
                               { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2784 "zparser.c"
    break;

  case 174: /* type_and_rdata: T_L64 sp rdata_unknown  */
#line 691 "zparser.y"
                               { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2790 "zparser.c"
    break;

  case 176: /* type_and_rdata: T_LP sp rdata_unknown  */
#line 693 "zparser.y"
                              { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2796 "zparser.c"
    break;

  case 178: /* type_and_rdata: T_EUI48 sp rdata_unknown  */
#line 695 "zparser.y"
                                 { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2802 "zparser.c"
    break;

  case 180: /* type_and_rdata: T_EUI64 sp rdata_unknown  */
#line 697 "zparser.y"
                                 { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2808 "zparser.c"
    break;

  case 182: /* type_and_rdata: T_CAA sp rdata_unknown  */
#line 699 "zparser.y"
                               { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2814 "zparser.c"
    break;

  case 184: /* type_and_rdata: T_CDS sp rdata_unknown  */
#line 701 "zparser.y"
                               { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2820 "zparser.c"
    break;

  case 186: /* type_and_rdata: T_CDNSKEY sp rdata_unknown  */
#line 703 "zparser.y"
                                   { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2826 "zparser.c"
    break;

  case 188: /* type_and_rdata: T_OPENPGPKEY sp rdata_unknown  */
#line 705 "zparser.y"
                                      { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2832 "zparser.c"
    break;

  case 190: /* type_and_rdata: T_CSYNC sp rdata_unknown  */
#line 707 "zparser.y"
                                 { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2838 "zparser.c"
    break;

  case 192: /* type_and_rdata: T_ZONEMD sp rdata_unknown  */
#line 709 "zparser.y"
                                  { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2844 "zparser.c"
    break;

  case 194: /* type_and_rdata: T_SVCB sp rdata_unknown  */
#line 711 "zparser.y"
                                { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2850 "zparser.c"
    break;

  case 196: /* type_and_rdata: T_HTTPS sp rdata_unknown  */
#line 713 "zparser.y"
                                 { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2856 "zparser.c"
    break;

  case 198: /* type_and_rdata: T_URI sp rdata_unknown  */
#line 715 "zparser.y"
                               { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2862 "zparser.c"
    break;

  case 199: /* type_and_rdata: T_UTYPE sp rdata_unknown  */
#line 716 "zparser.y"
                                 { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2868 "zparser.c"
    break;

  case 200: /* type_and_rdata: str error NL  */
#line 718 "zparser.y"
    {
	    zc_error_prev_line("unrecognized RR type '%s'", (yyvsp[-2].data).str);
    }
#line 2876 "zparser.c"
    break;

  case 201: /* rdata_a: dotted_str trail  */
#line 730 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_a(parser->region, (yyvsp[-1].data).str));
    }
#line 2884 "zparser.c"
    break;

  case 202: /* rdata_domain_name: dname trail  */
#line 736 "zparser.y"
    {
	    /* convert a single dname record */
	    zadd_rdata_domain((yyvsp[-1].domain));
    }
#line 2893 "zparser.c"
    break;

  case 203: /* rdata_soa: dname sp dname sp str sp str sp str sp str sp str trail  */
#line 743 "zparser.y"
    {
	    /* convert the soa data */
	    zadd_rdata_domain((yyvsp[-13].domain));	/* prim. ns */
	    zadd_rdata_domain((yyvsp[-11].domain));	/* email */
	    zadd_rdata_wireformat(zparser_conv_serial(parser->region, (yyvsp[-9].data).str)); /* serial */
	    zadd_rdata_wireformat(zparser_conv_period(parser->region, (yyvsp[-7].data).str)); /* refresh */
	    zadd_rdata_wireformat(zparser_conv_period(parser->region, (yyvsp[-5].data).str)); /* retry */
	    zadd_rdata_wireformat(zparser_conv_period(parser->region, (yyvsp[-3].data).str)); /* expire */
	    zadd_rdata_wireformat(zparser_conv_period(parser->region, (yyvsp[-1].data).str)); /* minimum */
    }
#line 2908 "zparser.c"
    break;

  case 204: /* rdata_wks: dotted_str sp str sp concatenated_str_seq trail  */
#line 756 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_a(parser->region, (yyvsp[-5].data).str)); /* address */
	    zadd_rdata_wireformat(zparser_conv_services(parser->region, (yyvsp[-3].data).str, (yyvsp[-1].data).str)); /* protocol and services */
    }
#line 2917 "zparser.c"
    break;

  case 205: /* rdata_hinfo: str sp str trail  */
#line 763 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-3].data).str, (yyvsp[-3].data).len)); /* CPU */
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* OS*/
    }
#line 2926 "zparser.c"
    break;

  case 206: /* rdata_minfo: dname sp dname trail  */
#line 770 "zparser.y"
    {
	    /* convert a single dname record */
	    zadd_rdata_domain((yyvsp[-3].domain));
	    zadd_rdata_domain((yyvsp[-1].domain));
    }
#line 2936 "zparser.c"
    break;

  case 207: /* rdata_mx: str sp dname trail  */
#line 778 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str));  /* priority */
	    zadd_rdata_domain((yyvsp[-1].domain));	/* MX host */
    }
#line 2945 "zparser.c"
    break;

  case 208: /* rdata_txt: str_seq trail  */
#line 785 "zparser.y"
    {
	zadd_rdata_txt_clean_wireformat();
    }
#line 2953 "zparser.c"
    break;

  case 209: /* rdata_rp: dname sp dname trail  */
#line 792 "zparser.y"
    {
	    zadd_rdata_domain((yyvsp[-3].domain)); /* mbox d-name */
	    zadd_rdata_domain((yyvsp[-1].domain)); /* txt d-name */
    }
#line 2962 "zparser.c"
    break;

  case 210: /* rdata_afsdb: str sp dname trail  */
#line 800 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str)); /* subtype */
	    zadd_rdata_domain((yyvsp[-1].domain)); /* domain name */
    }
#line 2971 "zparser.c"
    break;

  case 211: /* rdata_x25: str trail  */
#line 808 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* X.25 address. */
    }
#line 2979 "zparser.c"
    break;

  case 212: /* rdata_isdn: str trail  */
#line 815 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* address */
    }
#line 2987 "zparser.c"
    break;

  case 213: /* rdata_isdn: str sp str trail  */
#line 819 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-3].data).str, (yyvsp[-3].data).len)); /* address */
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* sub-address */
    }
#line 2996 "zparser.c"
    break;

  case 214: /* rdata_rt: str sp dname trail  */
#line 827 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str)); /* preference */
	    zadd_rdata_domain((yyvsp[-1].domain)); /* intermediate host */
    }
#line 3005 "zparser.c"
    break;

  case 215: /* rdata_nsap: str_dot_seq trail  */
#line 835 "zparser.y"
    {
	    /* String must start with "0x" or "0X".	 */
	    if (strncasecmp((yyvsp[-1].data).str, "0x", 2) != 0) {
		    zc_error_prev_line("NSAP rdata must start with '0x'");
	    } else {
		    zadd_rdata_wireformat(zparser_conv_hex(parser->region, (yyvsp[-1].data).str + 2, (yyvsp[-1].data).len - 2)); /* NSAP */
	    }
    }
#line 3018 "zparser.c"
    break;

  case 216: /* rdata_px: str sp dname sp dname trail  */
#line 847 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-5].data).str)); /* preference */
	    zadd_rdata_domain((yyvsp[-3].domain)); /* MAP822 */
	    zadd_rdata_domain((yyvsp[-1].domain)); /* MAPX400 */
    }
#line 3028 "zparser.c"
    break;

  case 217: /* rdata_aaaa: dotted_str trail  */
#line 855 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_aaaa(parser->region, (yyvsp[-1].data).str));  /* IPv6 address */
    }
#line 3036 "zparser.c"
    break;

  case 218: /* rdata_loc: concatenated_str_seq trail  */
#line 861 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_loc(parser->region, (yyvsp[-1].data).str)); /* Location */
    }
#line 3044 "zparser.c"
    break;

  case 219: /* rdata_nxt: dname sp nxt_seq trail  */
#line 867 "zparser.y"
    {
	    zadd_rdata_domain((yyvsp[-3].domain)); /* nxt name */
	    zadd_rdata_wireformat(zparser_conv_nxt(parser->region, nxtbits)); /* nxt bitlist */
	    memset(nxtbits, 0, sizeof(nxtbits));
    }
#line 3054 "zparser.c"
    break;

  case 220: /* rdata_srv: str sp str sp str sp dname trail  */
#line 875 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-7].data).str)); /* prio */
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-5].data).str)); /* weight */
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str)); /* port */
	    zadd_rdata_domain((yyvsp[-1].domain)); /* target name */
    }
#line 3065 "zparser.c"
    break;

  case 221: /* rdata_naptr: str sp str sp str sp str sp str sp dname trail  */
#line 885 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-11].data).str)); /* order */
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-9].data).str)); /* preference */
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-7].data).str, (yyvsp[-7].data).len)); /* flags */
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-5].data).str, (yyvsp[-5].data).len)); /* service */
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-3].data).str, (yyvsp[-3].data).len)); /* regexp */
	    zadd_rdata_domain((yyvsp[-1].domain)); /* target name */
    }
#line 3078 "zparser.c"
    break;

  case 222: /* rdata_kx: str sp dname trail  */
#line 897 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str)); /* preference */
	    zadd_rdata_domain((yyvsp[-1].domain)); /* exchanger */
    }
#line 3087 "zparser.c"
    break;

  case 223: /* rdata_cert: str sp str sp str sp str_sp_seq trail  */
#line 905 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_certificate_type(parser->region, (yyvsp[-7].data).str)); /* type */
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-5].data).str)); /* key tag */
	    zadd_rdata_wireformat(zparser_conv_algorithm(parser->region, (yyvsp[-3].data).str)); /* algorithm */
	    zadd_rdata_wireformat(zparser_conv_b64(parser->region, (yyvsp[-1].data).str)); /* certificate or CRL */
    }
#line 3098 "zparser.c"
    break;

  case 225: /* rdata_apl_seq: dotted_str  */
#line 918 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_apl_rdata(parser->region, (yyvsp[0].data).str));
    }
#line 3106 "zparser.c"
    break;

  case 226: /* rdata_apl_seq: rdata_apl_seq sp dotted_str  */
#line 922 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_apl_rdata(parser->region, (yyvsp[0].data).str));
    }
#line 3114 "zparser.c"
    break;

  case 227: /* rdata_ds: str sp str sp str sp str_sp_seq trail  */
#line 928 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-7].data).str)); /* keytag */
	    zadd_rdata_wireformat(zparser_conv_algorithm(parser->region, (yyvsp[-5].data).str)); /* alg */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-3].data).str)); /* type */
	    zadd_rdata_wireformat(zparser_conv_hex(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* hash */
    }
#line 3125 "zparser.c"
    break;

  case 228: /* rdata_dlv: str sp str sp str sp str_sp_seq trail  */
#line 937 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-7].data).str)); /* keytag */
	    zadd_rdata_wireformat(zparser_conv_algorithm(parser->region, (yyvsp[-5].data).str)); /* alg */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-3].data).str)); /* type */
	    zadd_rdata_wireformat(zparser_conv_hex(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* hash */
    }
#line 3136 "zparser.c"
    break;

  case 229: /* rdata_sshfp: str sp str sp str_sp_seq trail  */
#line 946 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-5].data).str)); /* alg */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-3].data).str)); /* fp type */
	    zadd_rdata_wireformat(zparser_conv_hex(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* hash */
	    check_sshfp();
    }
#line 3147 "zparser.c"
    break;

  case 230: /* rdata_dhcid: str_sp_seq trail  */
#line 955 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_b64(parser->region, (yyvsp[-1].data).str)); /* data blob */
    }
#line 3155 "zparser.c"
    break;

  case 231: /* rdata_rrsig: str sp str sp str sp str sp str sp str sp str sp wire_dname sp str_sp_seq trail  */
#line 961 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_rrtype(parser->region, (yyvsp[-17].data).str)); /* rr covered */
	    zadd_rdata_wireformat(zparser_conv_algorithm(parser->region, (yyvsp[-15].data).str)); /* alg */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-13].data).str)); /* # labels */
	    zadd_rdata_wireformat(zparser_conv_period(parser->region, (yyvsp[-11].data).str)); /* # orig TTL */
	    zadd_rdata_wireformat(zparser_conv_time(parser->region, (yyvsp[-9].data).str)); /* sig exp */
	    zadd_rdata_wireformat(zparser_conv_time(parser->region, (yyvsp[-7].data).str)); /* sig inc */
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-5].data).str)); /* key id */
	    zadd_rdata_wireformat(zparser_conv_dns_name(parser->region, 
				(const uint8_t*) (yyvsp[-3].data).str,(yyvsp[-3].data).len)); /* sig name */
	    zadd_rdata_wireformat(zparser_conv_b64(parser->region, (yyvsp[-1].data).str)); /* sig data */
    }
#line 3172 "zparser.c"
    break;

  case 232: /* rdata_nsec: wire_dname nsec_seq  */
#line 976 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_dns_name(parser->region, 
				(const uint8_t*) (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* nsec name */
	    zadd_rdata_wireformat(zparser_conv_nsec(parser->region, nsecbits)); /* nsec bitlist */
	    memset(nsecbits, 0, sizeof(nsecbits));
            nsec_highest_rcode = 0;
    }
#line 3184 "zparser.c"
    break;

  case 233: /* rdata_nsec3: str sp str sp str sp str sp str nsec_seq  */
#line 986 "zparser.y"
    {
#ifdef NSEC3
	    nsec3_add_params((yyvsp[-9].data).str, (yyvsp[-7].data).str, (yyvsp[-5].data).str, (yyvsp[-3].data).str, (yyvsp[-3].data).len);

	    zadd_rdata_wireformat(zparser_conv_b32(parser->region, (yyvsp[-1].data).str)); /* next hashed name */
	    zadd_rdata_wireformat(zparser_conv_nsec(parser->region, nsecbits)); /* nsec bitlist */
	    memset(nsecbits, 0, sizeof(nsecbits));
	    nsec_highest_rcode = 0;
#else
	    zc_error_prev_line("nsec3 not supported");
#endif /* NSEC3 */
    }
#line 3201 "zparser.c"
    break;

  case 234: /* rdata_nsec3_param: str sp str sp str sp str trail  */
#line 1001 "zparser.y"
    {
#ifdef NSEC3
	    nsec3_add_params((yyvsp[-7].data).str, (yyvsp[-5].data).str, (yyvsp[-3].data).str, (yyvsp[-1].data).str, (yyvsp[-1].data).len);
#else
	    zc_error_prev_line("nsec3 not supported");
#endif /* NSEC3 */
    }
#line 3213 "zparser.c"
    break;

  case 235: /* rdata_tlsa: str sp str sp str sp str_sp_seq trail  */
#line 1011 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-7].data).str)); /* usage */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-5].data).str)); /* selector */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-3].data).str)); /* matching type */
	    zadd_rdata_wireformat(zparser_conv_hex(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* ca data */
    }
#line 3224 "zparser.c"
    break;

  case 236: /* rdata_smimea: str sp str sp str sp str_sp_seq trail  */
#line 1020 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-7].data).str)); /* usage */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-5].data).str)); /* selector */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-3].data).str)); /* matching type */
	    zadd_rdata_wireformat(zparser_conv_hex(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* ca data */
    }
#line 3235 "zparser.c"
    break;

  case 237: /* rdata_dnskey: str sp str sp str sp str_sp_seq trail  */
#line 1029 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-7].data).str)); /* flags */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-5].data).str)); /* proto */
	    zadd_rdata_wireformat(zparser_conv_algorithm(parser->region, (yyvsp[-3].data).str)); /* alg */
	    zadd_rdata_wireformat(zparser_conv_b64(parser->region, (yyvsp[-1].data).str)); /* hash */
    }
#line 3246 "zparser.c"
    break;

  case 238: /* rdata_ipsec_base: str sp str sp str sp dotted_str  */
#line 1038 "zparser.y"
    {
	    const dname_type* name = 0;
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-6].data).str)); /* precedence */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-4].data).str)); /* gateway type */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-2].data).str)); /* algorithm */
	    switch(atoi((yyvsp[-4].data).str)) {
		case IPSECKEY_NOGATEWAY: 
			zadd_rdata_wireformat(alloc_rdata_init(parser->region, "", 0));
			break;
		case IPSECKEY_IP4:
			zadd_rdata_wireformat(zparser_conv_a(parser->region, (yyvsp[0].data).str));
			break;
		case IPSECKEY_IP6:
			zadd_rdata_wireformat(zparser_conv_aaaa(parser->region, (yyvsp[0].data).str));
			break;
		case IPSECKEY_DNAME:
			/* convert and insert the dname */
			if(strlen((yyvsp[0].data).str) == 0)
				zc_error_prev_line("IPSECKEY must specify gateway name");
			if(!(name = dname_parse(parser->region, (yyvsp[0].data).str))) {
				zc_error_prev_line("IPSECKEY bad gateway dname %s", (yyvsp[0].data).str);
				break;
			}
			if((yyvsp[0].data).str[strlen((yyvsp[0].data).str)-1] != '.') {
				if(parser->origin == error_domain) {
		    			zc_error("cannot concatenate origin to domain name, because origin failed to parse");
					break;
				} else if(name->name_size + domain_dname(parser->origin)->name_size - 1 > MAXDOMAINLEN) {
					zc_error("ipsec gateway name exceeds %d character limit",
						MAXDOMAINLEN);
					break;
				}
				name = dname_concatenate(parser->rr_region, name, 
					domain_dname(parser->origin));
			}
			zadd_rdata_wireformat(alloc_rdata_init(parser->region,
				dname_name(name), name->name_size));
			break;
		default:
			zc_error_prev_line("unknown IPSECKEY gateway type");
	    }
    }
#line 3293 "zparser.c"
    break;

  case 239: /* rdata_ipseckey: rdata_ipsec_base sp str_sp_seq trail  */
#line 1083 "zparser.y"
    {
	   zadd_rdata_wireformat(zparser_conv_b64(parser->region, (yyvsp[-1].data).str)); /* public key */
    }
#line 3301 "zparser.c"
    break;

  case 241: /* rdata_nid: str sp dotted_str trail  */
#line 1091 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str));  /* preference */
	    zadd_rdata_wireformat(zparser_conv_ilnp64(parser->region, (yyvsp[-1].data).str));  /* NodeID */
    }
#line 3310 "zparser.c"
    break;

  case 242: /* rdata_l32: str sp dotted_str trail  */
#line 1098 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str));  /* preference */
	    zadd_rdata_wireformat(zparser_conv_a(parser->region, (yyvsp[-1].data).str));  /* Locator32 */
    }
#line 3319 "zparser.c"
    break;

  case 243: /* rdata_l64: str sp dotted_str trail  */
#line 1105 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str));  /* preference */
	    zadd_rdata_wireformat(zparser_conv_ilnp64(parser->region, (yyvsp[-1].data).str));  /* Locator64 */
    }
#line 3328 "zparser.c"
    break;

  case 244: /* rdata_lp: str sp dname trail  */
#line 1112 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str));  /* preference */
	    zadd_rdata_domain((yyvsp[-1].domain));  /* FQDN */
    }
#line 3337 "zparser.c"
    break;

  case 245: /* rdata_eui48: str trail  */
#line 1119 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_eui(parser->region, (yyvsp[-1].data).str, 48));
    }
#line 3345 "zparser.c"
    break;

  case 246: /* rdata_eui64: str trail  */
#line 1125 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_eui(parser->region, (yyvsp[-1].data).str, 64));
    }
#line 3353 "zparser.c"
    break;

  case 247: /* rdata_uri: str sp str sp dotted_str trail  */
#line 1132 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-5].data).str)); /* priority */
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str)); /* weight */
	    zadd_rdata_wireformat(zparser_conv_long_text(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* target */
    }
#line 3363 "zparser.c"
    break;

  case 248: /* rdata_caa: str sp str sp dotted_str trail  */
#line 1141 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-5].data).str)); /* Flags */
	    zadd_rdata_wireformat(zparser_conv_tag(parser->region, (yyvsp[-3].data).str, (yyvsp[-3].data).len)); /* Tag */
	    zadd_rdata_wireformat(zparser_conv_long_text(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* Value */
    }
#line 3373 "zparser.c"
    break;

  case 249: /* rdata_openpgpkey: str_sp_seq trail  */
#line 1150 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_b64(parser->region, (yyvsp[-1].data).str));
    }
#line 3381 "zparser.c"
    break;

  case 250: /* rdata_csync: str sp str nsec_seq  */
#line 1157 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_serial(parser->region, (yyvsp[-3].data).str));
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-1].data).str));
	    zadd_rdata_wireformat(zparser_conv_nsec(parser->region, nsecbits)); /* nsec bitlist */
	    memset(nsecbits, 0, sizeof(nsecbits));
            nsec_highest_rcode = 0;
    }
#line 3393 "zparser.c"
    break;

  case 251: /* rdata_zonemd: str sp str sp str sp str_sp_seq trail  */
#line 1168 "zparser.y"
    {
	    zadd_rdata_wireformat(zparser_conv_serial(parser->region, (yyvsp[-7].data).str)); /* serial */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-5].data).str)); /* scheme */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-3].data).str)); /* hash algorithm */
	    zadd_rdata_wireformat(zparser_conv_hex(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* digest */
    }
#line 3404 "zparser.c"
    break;

  case 252: /* svcparam: dotted_str QSTR  */
#line 1177 "zparser.y"
    {
	zadd_rdata_wireformat(zparser_conv_svcbparam(
		parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len, (yyvsp[0].data).str, (yyvsp[0].data).len));
    }
#line 3413 "zparser.c"
    break;

  case 253: /* svcparam: dotted_str  */
#line 1182 "zparser.y"
    {
	zadd_rdata_wireformat(zparser_conv_svcbparam(
		parser->region, (yyvsp[0].data).str, (yyvsp[0].data).len, NULL, 0));
    }
#line 3422 "zparser.c"
    break;

  case 256: /* rdata_svcb_base: str sp dname  */
#line 1192 "zparser.y"
    {
	    /* SvcFieldPriority */
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-2].data).str));
	    /* SvcDomainName */
	    zadd_rdata_domain((yyvsp[0].domain));
    }
#line 3433 "zparser.c"
    break;

  case 257: /* rdata_svcb: rdata_svcb_base sp svcparams trail  */
#line 1199 "zparser.y"
    {
        zadd_rdata_svcb_check_wireformat();
    }
#line 3441 "zparser.c"
    break;

  case 259: /* rdata_unknown: URR sp str sp str_sp_seq trail  */
#line 1206 "zparser.y"
    {
	    /* $2 is the number of octets, currently ignored */
	    (yyval.unknown) = zparser_conv_hex(parser->rr_region, (yyvsp[-1].data).str, (yyvsp[-1].data).len);

    }
#line 3451 "zparser.c"
    break;

  case 260: /* rdata_unknown: URR sp str trail  */
#line 1212 "zparser.y"
    {
	    (yyval.unknown) = zparser_conv_hex(parser->rr_region, "", 0);
    }
#line 3459 "zparser.c"
    break;

  case 261: /* rdata_unknown: URR error NL  */
#line 1216 "zparser.y"
    {
	    (yyval.unknown) = zparser_conv_hex(parser->rr_region, "", 0);
    }
#line 3467 "zparser.c"
    break;


#line 3471 "zparser.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
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
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;

  /* Do not reclaim the symbols of the rule whose action triggered
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
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
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
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

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


#if !defined yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturn;
#endif


/*-------------------------------------------------------.
| yyreturn -- parsing is finished, clean up and return.  |
`-------------------------------------------------------*/
yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 1220 "zparser.y"


int
yywrap(void)
{
	return 1;
}

/*
 * Create the parser.
 */
zparser_type *
zparser_create(region_type *region, region_type *rr_region, namedb_type *db)
{
	zparser_type *result;

	result = (zparser_type *) region_alloc(region, sizeof(zparser_type));
	result->region = region;
	result->rr_region = rr_region;
	result->db = db;

	result->filename = NULL;
	result->current_zone = NULL;
	result->origin = NULL;
	result->prev_dname = NULL;

	result->temporary_rdatas = (rdata_atom_type *) region_alloc_array(
		result->region, MAXRDATALEN, sizeof(rdata_atom_type));

	return result;
}

/*
 * Initialize the parser for a new zone file.
 */
void
zparser_init(const char *filename, uint32_t ttl, uint16_t klass,
	     const dname_type *origin)
{
	memset(nxtbits, 0, sizeof(nxtbits));
	memset(nsecbits, 0, sizeof(nsecbits));
        nsec_highest_rcode = 0;

	parser->default_ttl = ttl;
	parser->default_class = klass;
	parser->current_zone = NULL;
	parser->origin = domain_table_insert(parser->db->domains, origin);
	parser->prev_dname = parser->origin;
	parser->error_occurred = 0;
	parser->errors = 0;
	parser->line = 1;
	parser->filename = filename;
	parser->current_rr.rdata_count = 0;
	parser->current_rr.rdatas = parser->temporary_rdatas;
}

void
yyerror(const char *message)
{
	zc_error("%s", message);
}

static void
error_va_list(unsigned line, const char *fmt, va_list args)
{
	if (parser->filename) {
		char message[MAXSYSLOGMSGLEN];
		vsnprintf(message, sizeof(message), fmt, args);
		log_msg(LOG_ERR, "%s:%u: %s", parser->filename, line, message);
	}
	else log_vmsg(LOG_ERR, fmt, args);

	++parser->errors;
	parser->error_occurred = 1;
}

/* the line counting sux, to say the least
 * with this grose hack we try do give sane
 * numbers back */
void
zc_error_prev_line(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	error_va_list(parser->line - 1, fmt, args);
	va_end(args);
}

void
zc_error(const char *fmt, ...)
{
	/* send an error message to stderr */
	va_list args;
	va_start(args, fmt);
	error_va_list(parser->line, fmt, args);
	va_end(args);
}

static void
warning_va_list(unsigned line, const char *fmt, va_list args)
{
	if (parser->filename) {
		char m[MAXSYSLOGMSGLEN];
		vsnprintf(m, sizeof(m), fmt, args);
		log_msg(LOG_WARNING, "%s:%u: %s", parser->filename, line, m);
	}
	else log_vmsg(LOG_WARNING, fmt, args);
}

void
zc_warning_prev_line(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	warning_va_list(parser->line - 1, fmt, args);
	va_end(args);
}

void
zc_warning(const char *fmt, ... )
{
	va_list args;
	va_start(args, fmt);
	warning_va_list(parser->line, fmt, args);
	va_end(args);
}

#ifdef NSEC3
static void
nsec3_add_params(const char* hashalgo_str, const char* flag_str,
	const char* iter_str, const char* salt_str, int salt_len)
{
	zadd_rdata_wireformat(zparser_conv_byte(parser->region, hashalgo_str));
	zadd_rdata_wireformat(zparser_conv_byte(parser->region, flag_str));
	zadd_rdata_wireformat(zparser_conv_short(parser->region, iter_str));

	/* salt */
	if(strcmp(salt_str, "-") != 0) 
		zadd_rdata_wireformat(zparser_conv_hex_length(parser->region, 
			salt_str, salt_len)); 
	else 
		zadd_rdata_wireformat(alloc_rdata_init(parser->region, "", 1));
}
#endif /* NSEC3 */
