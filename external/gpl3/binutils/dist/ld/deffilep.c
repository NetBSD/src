/* A Bison parser, made by GNU Bison 3.8.2.  */

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
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "deffilep.y"
 /* deffilep.y - parser for .def files */

/*   Copyright (C) 1995-2024 Free Software Foundation, Inc.

     This file is part of GNU Binutils.

     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation; either version 3 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
     MA 02110-1301, USA.  */

#include "sysdep.h"
#include "libiberty.h"
#include "safe-ctype.h"
#include "bfd.h"
#include "bfdlink.h"
#include "ld.h"
#include "ldmisc.h"
#include "deffile.h"

#define TRACE 0

#define ROUND_UP(a, b) (((a)+((b)-1))&~((b)-1))

#define SYMBOL_LIST_ARRAY_GROW 64

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror, etc),
   as well as gratuitiously global symbol names, so we can have multiple
   yacc generated parsers in ld.  Note that these are only the variables
   produced by yacc.  If other parser generators (bison, byacc, etc) produce
   additional global names that conflict at link time, then those parser
   generators need to be fixed instead of adding those names to this list.  */

#define	yymaxdepth def_maxdepth
#define	yyparse	def_parse
#define	yylex	def_lex
#define	yyerror	def_error
#define	yylval	def_lval
#define	yychar	def_char
#define	yydebug	def_debug
#define	yypact	def_pact
#define	yyr1	def_r1
#define	yyr2	def_r2
#define	yydef	def_def
#define	yychk	def_chk
#define	yypgo	def_pgo
#define	yyact	def_act
#define	yyexca	def_exca
#define yyerrflag def_errflag
#define yynerrs	def_nerrs
#define	yyps	def_ps
#define	yypv	def_pv
#define	yys	def_s
#define	yy_yys	def_yys
#define	yystate	def_state
#define	yytmp	def_tmp
#define	yyv	def_v
#define	yy_yyv	def_yyv
#define	yyval	def_val
#define	yylloc	def_lloc
#define yyreds	def_reds		/* With YYDEBUG defined.  */
#define yytoks	def_toks		/* With YYDEBUG defined.  */
#define yylhs	def_yylhs
#define yylen	def_yylen
#define yydefred def_yydefred
#define yydgoto	def_yydgoto
#define yysindex def_yysindex
#define yyrindex def_yyrindex
#define yygindex def_yygindex
#define yytable	 def_yytable
#define yycheck	 def_yycheck

typedef struct def_pool_str {
  struct def_pool_str *next;
  char data[1];
} def_pool_str;

static def_pool_str *pool_strs = NULL;

static char *def_pool_alloc (size_t sz);
static char *def_pool_strdup (const char *str);
static void def_pool_free (void);

static void def_description (const char *);
static void def_exports (const char *, const char *, int, int, const char *);
static void def_heapsize (int, int);
static void def_import (const char *, const char *, const char *, const char *,
			int, const char *);
static void def_image_name (const char *, bfd_vma, int);
static void def_section (const char *, int);
static void def_section_alt (const char *, const char *);
static void def_stacksize (int, int);
static void def_version (int, int);
static void def_directive (char *);
static void def_aligncomm (char *str, int align);
static void def_exclude_symbols (char *str);
static int def_parse (void);
static void def_error (const char *);
static int def_lex (void);

static int lex_forced_token = 0;
static const char *lex_parse_string = 0;
static const char *lex_parse_string_end = 0;


#line 187 "deffilep.c"

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

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
#ifndef YY_YY_DEFFILEP_H_INCLUDED
# define YY_YY_DEFFILEP_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    NAME = 258,                    /* NAME  */
    LIBRARY = 259,                 /* LIBRARY  */
    DESCRIPTION = 260,             /* DESCRIPTION  */
    STACKSIZE_K = 261,             /* STACKSIZE_K  */
    HEAPSIZE = 262,                /* HEAPSIZE  */
    CODE = 263,                    /* CODE  */
    DATAU = 264,                   /* DATAU  */
    DATAL = 265,                   /* DATAL  */
    SECTIONS = 266,                /* SECTIONS  */
    EXPORTS = 267,                 /* EXPORTS  */
    IMPORTS = 268,                 /* IMPORTS  */
    VERSIONK = 269,                /* VERSIONK  */
    BASE = 270,                    /* BASE  */
    CONSTANTU = 271,               /* CONSTANTU  */
    CONSTANTL = 272,               /* CONSTANTL  */
    PRIVATEU = 273,                /* PRIVATEU  */
    PRIVATEL = 274,                /* PRIVATEL  */
    ALIGNCOMM = 275,               /* ALIGNCOMM  */
    EXCLUDE_SYMBOLS = 276,         /* EXCLUDE_SYMBOLS  */
    READ = 277,                    /* READ  */
    WRITE = 278,                   /* WRITE  */
    EXECUTE = 279,                 /* EXECUTE  */
    SHARED_K = 280,                /* SHARED_K  */
    NONAMEU = 281,                 /* NONAMEU  */
    NONAMEL = 282,                 /* NONAMEL  */
    DIRECTIVE = 283,               /* DIRECTIVE  */
    EQUAL = 284,                   /* EQUAL  */
    ID = 285,                      /* ID  */
    DIGITS = 286                   /* DIGITS  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define NAME 258
#define LIBRARY 259
#define DESCRIPTION 260
#define STACKSIZE_K 261
#define HEAPSIZE 262
#define CODE 263
#define DATAU 264
#define DATAL 265
#define SECTIONS 266
#define EXPORTS 267
#define IMPORTS 268
#define VERSIONK 269
#define BASE 270
#define CONSTANTU 271
#define CONSTANTL 272
#define PRIVATEU 273
#define PRIVATEL 274
#define ALIGNCOMM 275
#define EXCLUDE_SYMBOLS 276
#define READ 277
#define WRITE 278
#define EXECUTE 279
#define SHARED_K 280
#define NONAMEU 281
#define NONAMEL 282
#define DIRECTIVE 283
#define EQUAL 284
#define ID 285
#define DIGITS 286

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 117 "deffilep.y"

  char *id;
  const char *id_const;
  int number;
  bfd_vma vma;
  char *digits;

#line 310 "deffilep.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_DEFFILEP_H_INCLUDED  */
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_NAME = 3,                       /* NAME  */
  YYSYMBOL_LIBRARY = 4,                    /* LIBRARY  */
  YYSYMBOL_DESCRIPTION = 5,                /* DESCRIPTION  */
  YYSYMBOL_STACKSIZE_K = 6,                /* STACKSIZE_K  */
  YYSYMBOL_HEAPSIZE = 7,                   /* HEAPSIZE  */
  YYSYMBOL_CODE = 8,                       /* CODE  */
  YYSYMBOL_DATAU = 9,                      /* DATAU  */
  YYSYMBOL_DATAL = 10,                     /* DATAL  */
  YYSYMBOL_SECTIONS = 11,                  /* SECTIONS  */
  YYSYMBOL_EXPORTS = 12,                   /* EXPORTS  */
  YYSYMBOL_IMPORTS = 13,                   /* IMPORTS  */
  YYSYMBOL_VERSIONK = 14,                  /* VERSIONK  */
  YYSYMBOL_BASE = 15,                      /* BASE  */
  YYSYMBOL_CONSTANTU = 16,                 /* CONSTANTU  */
  YYSYMBOL_CONSTANTL = 17,                 /* CONSTANTL  */
  YYSYMBOL_PRIVATEU = 18,                  /* PRIVATEU  */
  YYSYMBOL_PRIVATEL = 19,                  /* PRIVATEL  */
  YYSYMBOL_ALIGNCOMM = 20,                 /* ALIGNCOMM  */
  YYSYMBOL_EXCLUDE_SYMBOLS = 21,           /* EXCLUDE_SYMBOLS  */
  YYSYMBOL_READ = 22,                      /* READ  */
  YYSYMBOL_WRITE = 23,                     /* WRITE  */
  YYSYMBOL_EXECUTE = 24,                   /* EXECUTE  */
  YYSYMBOL_SHARED_K = 25,                  /* SHARED_K  */
  YYSYMBOL_NONAMEU = 26,                   /* NONAMEU  */
  YYSYMBOL_NONAMEL = 27,                   /* NONAMEL  */
  YYSYMBOL_DIRECTIVE = 28,                 /* DIRECTIVE  */
  YYSYMBOL_EQUAL = 29,                     /* EQUAL  */
  YYSYMBOL_ID = 30,                        /* ID  */
  YYSYMBOL_DIGITS = 31,                    /* DIGITS  */
  YYSYMBOL_32_ = 32,                       /* '.'  */
  YYSYMBOL_33_ = 33,                       /* ','  */
  YYSYMBOL_34_ = 34,                       /* '='  */
  YYSYMBOL_35_ = 35,                       /* '@'  */
  YYSYMBOL_YYACCEPT = 36,                  /* $accept  */
  YYSYMBOL_start = 37,                     /* start  */
  YYSYMBOL_command = 38,                   /* command  */
  YYSYMBOL_explist = 39,                   /* explist  */
  YYSYMBOL_expline = 40,                   /* expline  */
  YYSYMBOL_exp_opt_list = 41,              /* exp_opt_list  */
  YYSYMBOL_exp_opt = 42,                   /* exp_opt  */
  YYSYMBOL_implist = 43,                   /* implist  */
  YYSYMBOL_impline = 44,                   /* impline  */
  YYSYMBOL_seclist = 45,                   /* seclist  */
  YYSYMBOL_secline = 46,                   /* secline  */
  YYSYMBOL_attr_list = 47,                 /* attr_list  */
  YYSYMBOL_opt_comma = 48,                 /* opt_comma  */
  YYSYMBOL_opt_number = 49,                /* opt_number  */
  YYSYMBOL_attr = 50,                      /* attr  */
  YYSYMBOL_keyword_as_name = 51,           /* keyword_as_name  */
  YYSYMBOL_opt_name2 = 52,                 /* opt_name2  */
  YYSYMBOL_opt_name = 53,                  /* opt_name  */
  YYSYMBOL_opt_equalequal_name = 54,       /* opt_equalequal_name  */
  YYSYMBOL_opt_ordinal = 55,               /* opt_ordinal  */
  YYSYMBOL_opt_equal_name = 56,            /* opt_equal_name  */
  YYSYMBOL_opt_base = 57,                  /* opt_base  */
  YYSYMBOL_anylang_id = 58,                /* anylang_id  */
  YYSYMBOL_symbol_list = 59,               /* symbol_list  */
  YYSYMBOL_opt_digits = 60,                /* opt_digits  */
  YYSYMBOL_opt_id = 61,                    /* opt_id  */
  YYSYMBOL_NUMBER = 62,                    /* NUMBER  */
  YYSYMBOL_VMA = 63                        /* VMA  */
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
typedef yytype_uint8 yy_state_t;

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

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
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
#define YYFINAL  73
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   161

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  36
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  28
/* YYNRULES -- Number of rules.  */
#define YYNRULES  104
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  153

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   286


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
       2,     2,     2,     2,    33,     2,    32,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    34,     2,     2,    35,     2,     2,     2,     2,     2,
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
      25,    26,    27,    28,    29,    30,    31
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   142,   142,   143,   147,   148,   149,   150,   151,   152,
     153,   154,   155,   156,   157,   158,   159,   160,   161,   165,
     167,   168,   175,   182,   183,   186,   187,   188,   189,   190,
     191,   192,   193,   196,   197,   201,   203,   205,   207,   209,
     211,   216,   217,   221,   222,   226,   227,   231,   232,   234,
     235,   239,   240,   241,   242,   246,   247,   248,   249,   250,
     251,   252,   253,   254,   255,   256,   257,   258,   265,   266,
     267,   268,   269,   270,   271,   272,   273,   274,   277,   278,
     284,   290,   296,   304,   305,   308,   309,   313,   314,   318,
     319,   322,   323,   326,   327,   333,   342,   343,   344,   347,
     348,   351,   352,   355,   357
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
  "\"end of file\"", "error", "\"invalid token\"", "NAME", "LIBRARY",
  "DESCRIPTION", "STACKSIZE_K", "HEAPSIZE", "CODE", "DATAU", "DATAL",
  "SECTIONS", "EXPORTS", "IMPORTS", "VERSIONK", "BASE", "CONSTANTU",
  "CONSTANTL", "PRIVATEU", "PRIVATEL", "ALIGNCOMM", "EXCLUDE_SYMBOLS",
  "READ", "WRITE", "EXECUTE", "SHARED_K", "NONAMEU", "NONAMEL",
  "DIRECTIVE", "EQUAL", "ID", "DIGITS", "'.'", "','", "'='", "'@'",
  "$accept", "start", "command", "explist", "expline", "exp_opt_list",
  "exp_opt", "implist", "impline", "seclist", "secline", "attr_list",
  "opt_comma", "opt_number", "attr", "keyword_as_name", "opt_name2",
  "opt_name", "opt_equalequal_name", "opt_ordinal", "opt_equal_name",
  "opt_base", "anylang_id", "symbol_list", "opt_digits", "opt_id",
  "NUMBER", "VMA", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-85)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-49)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int8 yypact[] =
{
      93,     5,     5,   -14,     7,     7,    38,    38,    16,     5,
      46,     7,   -27,   -27,    57,    36,   -85,   -85,   -85,   -85,
     -85,   -85,   -85,   -85,   -85,   -85,   -85,   -85,   -85,   -85,
     -85,   -85,   -85,   -85,   -85,   -85,   -85,   -85,   -85,   -85,
      61,     5,    76,   -85,   102,   102,   -85,   -85,    95,    95,
     -85,   -85,   -85,   -85,    87,   -85,    87,   100,    16,   -85,
       5,   -85,    84,   -25,    46,   -85,    97,   -85,   101,    53,
     103,    37,   -85,   -85,   -85,     5,    76,   -85,     5,    98,
     -85,   -85,     7,   -85,   -85,   -85,    38,   -85,    87,   -85,
     -85,     5,    99,   106,   107,   -85,     7,   -85,   109,     7,
     -27,   103,   -85,   -85,   110,   -85,   -85,   -85,     7,   105,
      26,   111,   -85,   -85,   112,   -85,   103,   -85,   -85,   -85,
      56,   114,   115,   -85,    85,   -85,   -85,   -85,   -85,   -85,
     -85,   -85,   -85,   -85,   -85,   105,   105,   -85,   104,    63,
     104,   104,    56,   -85,    96,   -85,   -85,   -85,   -85,   104,
     104,   -85,   -85
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,    84,    84,     0,     0,     0,     0,     0,     0,    19,
       0,     0,     0,     0,     0,     0,     3,    68,    61,    75,
      66,    56,    59,    60,    65,    67,    76,    55,    57,    58,
      71,    72,    63,    73,    77,    64,    74,    69,    70,    62,
      78,     0,     0,    83,    92,    92,     6,   103,    50,    50,
      51,    52,    53,    54,     9,    46,    10,     0,    11,    42,
      12,    20,    90,     0,    13,    34,    14,    93,     0,     0,
      96,    18,    16,     1,     2,     0,    79,    80,     0,     0,
       4,     5,     0,     7,     8,    47,     0,    44,    43,    41,
      21,     0,    88,     0,     0,    33,     0,    94,   100,     0,
       0,    97,    82,    81,     0,    49,    45,    89,     0,    48,
      86,     0,    15,    99,   102,    17,    98,   104,    91,    87,
      24,     0,     0,    40,     0,   101,    95,    29,    30,    27,
      28,    31,    32,    25,    26,    48,    48,    85,    86,    86,
      86,    86,    24,    39,     0,    37,    38,    22,    23,    86,
      86,    35,    36
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -85,   -85,   131,   -85,    88,     8,   -85,   -85,    83,   -85,
      91,    -3,   -84,   108,    65,   113,    -7,   150,   -60,   -85,
     -85,   116,   -12,   -85,   -85,   -85,    -5,   -85
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,    15,    16,    60,    61,   135,   136,    64,    65,    58,
      59,    54,    86,    83,    55,    42,    43,    44,   123,   109,
      92,    80,    69,    71,   114,   126,    48,   118
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      49,    70,    62,    67,    56,    68,    66,    93,    17,    94,
      18,    19,    20,    21,    22,    23,    46,    24,    25,    26,
      27,    28,    29,    30,    31,   120,    32,    33,    34,    35,
      36,    37,    38,    39,    77,    40,    73,    41,    47,     1,
       2,     3,     4,     5,     6,     7,    57,     8,     9,    10,
      11,   141,   142,    62,    88,   121,    12,    13,   122,   101,
      50,    51,    52,    53,    14,   127,   128,    67,   102,    68,
     100,   103,   129,   130,   131,   132,    63,   105,   143,   145,
     146,   147,   133,   134,   107,    98,    99,    72,   116,   151,
     152,   112,   121,    75,   115,   144,     1,     2,     3,     4,
       5,     6,     7,   119,     8,     9,    10,    11,    78,   -48,
     -48,   -48,   -48,    12,    13,   139,    47,    79,    91,   140,
      85,    14,    50,    51,    52,    53,   149,    47,    82,    96,
      87,    97,   104,   121,   108,    98,   110,   111,    85,   150,
     113,   117,   125,   124,   137,   138,    74,    95,    90,    89,
     148,   106,    45,     0,    76,     0,     0,    84,     0,     0,
       0,    81
};

static const yytype_int16 yycheck[] =
{
       5,    13,     9,    30,     7,    32,    11,    32,     3,    34,
       5,     6,     7,     8,     9,    10,    30,    12,    13,    14,
      15,    16,    17,    18,    19,   109,    21,    22,    23,    24,
      25,    26,    27,    28,    41,    30,     0,    32,    31,     3,
       4,     5,     6,     7,     8,     9,    30,    11,    12,    13,
      14,   135,   136,    60,    57,    29,    20,    21,    32,    71,
      22,    23,    24,    25,    28,     9,    10,    30,    75,    32,
      33,    78,    16,    17,    18,    19,    30,    82,   138,   139,
     140,   141,    26,    27,    91,    32,    33,    30,   100,   149,
     150,    96,    29,    32,    99,    32,     3,     4,     5,     6,
       7,     8,     9,   108,    11,    12,    13,    14,    32,    22,
      23,    24,    25,    20,    21,    30,    31,    15,    34,   124,
      33,    28,    22,    23,    24,    25,    30,    31,    33,    32,
      30,    30,    34,    29,    35,    32,    30,    30,    33,   144,
      31,    31,    30,    32,    30,    30,    15,    64,    60,    58,
     142,    86,     2,    -1,    41,    -1,    -1,    49,    -1,    -1,
      -1,    45
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    11,    12,
      13,    14,    20,    21,    28,    37,    38,     3,     5,     6,
       7,     8,     9,    10,    12,    13,    14,    15,    16,    17,
      18,    19,    21,    22,    23,    24,    25,    26,    27,    28,
      30,    32,    51,    52,    53,    53,    30,    31,    62,    62,
      22,    23,    24,    25,    47,    50,    47,    30,    45,    46,
      39,    40,    52,    30,    43,    44,    62,    30,    32,    58,
      58,    59,    30,     0,    38,    32,    51,    52,    32,    15,
      57,    57,    33,    49,    49,    33,    48,    30,    47,    46,
      40,    34,    56,    32,    34,    44,    32,    30,    32,    33,
      33,    58,    52,    52,    34,    62,    50,    52,    35,    55,
      30,    30,    62,    31,    60,    62,    58,    31,    63,    62,
      48,    29,    32,    54,    32,    30,    61,     9,    10,    16,
      17,    18,    19,    26,    27,    41,    42,    30,    30,    30,
      62,    48,    48,    54,    32,    54,    54,    54,    41,    30,
      62,    54,    54
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    36,    37,    37,    38,    38,    38,    38,    38,    38,
      38,    38,    38,    38,    38,    38,    38,    38,    38,    39,
      39,    39,    40,    41,    41,    42,    42,    42,    42,    42,
      42,    42,    42,    43,    43,    44,    44,    44,    44,    44,
      44,    45,    45,    46,    46,    47,    47,    48,    48,    49,
      49,    50,    50,    50,    50,    51,    51,    51,    51,    51,
      51,    51,    51,    51,    51,    51,    51,    51,    51,    51,
      51,    51,    51,    51,    51,    51,    51,    51,    52,    52,
      52,    52,    52,    53,    53,    54,    54,    55,    55,    56,
      56,    57,    57,    58,    58,    58,    59,    59,    59,    60,
      60,    61,    61,    62,    63
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     1,     3,     3,     2,     3,     3,     2,
       2,     2,     2,     2,     2,     4,     2,     4,     2,     0,
       1,     2,     7,     3,     0,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     1,     8,     8,     6,     6,     6,
       4,     2,     1,     2,     2,     3,     1,     1,     0,     2,
       0,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       2,     3,     3,     1,     0,     2,     0,     2,     0,     2,
       0,     3,     0,     1,     2,     4,     1,     2,     3,     1,
       0,     1,     0,     1,     1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


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
    YYNOMEM;
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
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
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
  case 4: /* command: NAME opt_name opt_base  */
#line 147 "deffilep.y"
                                       { def_image_name ((yyvsp[-1].id), (yyvsp[0].vma), 0); }
#line 1462 "deffilep.c"
    break;

  case 5: /* command: LIBRARY opt_name opt_base  */
#line 148 "deffilep.y"
                                          { def_image_name ((yyvsp[-1].id), (yyvsp[0].vma), 1); }
#line 1468 "deffilep.c"
    break;

  case 6: /* command: DESCRIPTION ID  */
#line 149 "deffilep.y"
                               { def_description ((yyvsp[0].id));}
#line 1474 "deffilep.c"
    break;

  case 7: /* command: STACKSIZE_K NUMBER opt_number  */
#line 150 "deffilep.y"
                                              { def_stacksize ((yyvsp[-1].number), (yyvsp[0].number));}
#line 1480 "deffilep.c"
    break;

  case 8: /* command: HEAPSIZE NUMBER opt_number  */
#line 151 "deffilep.y"
                                           { def_heapsize ((yyvsp[-1].number), (yyvsp[0].number));}
#line 1486 "deffilep.c"
    break;

  case 9: /* command: CODE attr_list  */
#line 152 "deffilep.y"
                               { def_section ("CODE", (yyvsp[0].number));}
#line 1492 "deffilep.c"
    break;

  case 10: /* command: DATAU attr_list  */
#line 153 "deffilep.y"
                                 { def_section ("DATA", (yyvsp[0].number));}
#line 1498 "deffilep.c"
    break;

  case 14: /* command: VERSIONK NUMBER  */
#line 157 "deffilep.y"
                                { def_version ((yyvsp[0].number), 0);}
#line 1504 "deffilep.c"
    break;

  case 15: /* command: VERSIONK NUMBER '.' NUMBER  */
#line 158 "deffilep.y"
                                           { def_version ((yyvsp[-2].number), (yyvsp[0].number));}
#line 1510 "deffilep.c"
    break;

  case 16: /* command: DIRECTIVE ID  */
#line 159 "deffilep.y"
                             { def_directive ((yyvsp[0].id));}
#line 1516 "deffilep.c"
    break;

  case 17: /* command: ALIGNCOMM anylang_id ',' NUMBER  */
#line 160 "deffilep.y"
                                                { def_aligncomm ((yyvsp[-2].id), (yyvsp[0].number));}
#line 1522 "deffilep.c"
    break;

  case 22: /* expline: opt_name2 opt_equal_name opt_ordinal opt_comma exp_opt_list opt_comma opt_equalequal_name  */
#line 176 "deffilep.y"
                        { def_exports ((yyvsp[-6].id), (yyvsp[-5].id), (yyvsp[-4].number), (yyvsp[-2].number), (yyvsp[0].id)); }
#line 1528 "deffilep.c"
    break;

  case 23: /* exp_opt_list: exp_opt opt_comma exp_opt_list  */
#line 182 "deffilep.y"
                                               { (yyval.number) = (yyvsp[-2].number) | (yyvsp[0].number); }
#line 1534 "deffilep.c"
    break;

  case 24: /* exp_opt_list: %empty  */
#line 183 "deffilep.y"
                { (yyval.number) = 0; }
#line 1540 "deffilep.c"
    break;

  case 25: /* exp_opt: NONAMEU  */
#line 186 "deffilep.y"
                                { (yyval.number) = 1; }
#line 1546 "deffilep.c"
    break;

  case 26: /* exp_opt: NONAMEL  */
#line 187 "deffilep.y"
                                { (yyval.number) = 1; }
#line 1552 "deffilep.c"
    break;

  case 27: /* exp_opt: CONSTANTU  */
#line 188 "deffilep.y"
                                { (yyval.number) = 2; }
#line 1558 "deffilep.c"
    break;

  case 28: /* exp_opt: CONSTANTL  */
#line 189 "deffilep.y"
                                { (yyval.number) = 2; }
#line 1564 "deffilep.c"
    break;

  case 29: /* exp_opt: DATAU  */
#line 190 "deffilep.y"
                                { (yyval.number) = 4; }
#line 1570 "deffilep.c"
    break;

  case 30: /* exp_opt: DATAL  */
#line 191 "deffilep.y"
                                { (yyval.number) = 4; }
#line 1576 "deffilep.c"
    break;

  case 31: /* exp_opt: PRIVATEU  */
#line 192 "deffilep.y"
                                { (yyval.number) = 8; }
#line 1582 "deffilep.c"
    break;

  case 32: /* exp_opt: PRIVATEL  */
#line 193 "deffilep.y"
                                { (yyval.number) = 8; }
#line 1588 "deffilep.c"
    break;

  case 35: /* impline: ID '=' ID '.' ID '.' ID opt_equalequal_name  */
#line 202 "deffilep.y"
                 { def_import ((yyvsp[-7].id), (yyvsp[-5].id), (yyvsp[-3].id), (yyvsp[-1].id), -1, (yyvsp[0].id)); }
#line 1594 "deffilep.c"
    break;

  case 36: /* impline: ID '=' ID '.' ID '.' NUMBER opt_equalequal_name  */
#line 204 "deffilep.y"
                                 { def_import ((yyvsp[-7].id), (yyvsp[-5].id), (yyvsp[-3].id),  0, (yyvsp[-1].number), (yyvsp[0].id)); }
#line 1600 "deffilep.c"
    break;

  case 37: /* impline: ID '=' ID '.' ID opt_equalequal_name  */
#line 206 "deffilep.y"
                 { def_import ((yyvsp[-5].id), (yyvsp[-3].id),	0, (yyvsp[-1].id), -1, (yyvsp[0].id)); }
#line 1606 "deffilep.c"
    break;

  case 38: /* impline: ID '=' ID '.' NUMBER opt_equalequal_name  */
#line 208 "deffilep.y"
                 { def_import ((yyvsp[-5].id), (yyvsp[-3].id),	0,  0, (yyvsp[-1].number), (yyvsp[0].id)); }
#line 1612 "deffilep.c"
    break;

  case 39: /* impline: ID '.' ID '.' ID opt_equalequal_name  */
#line 210 "deffilep.y"
                 { def_import( 0, (yyvsp[-5].id), (yyvsp[-3].id), (yyvsp[-1].id), -1, (yyvsp[0].id)); }
#line 1618 "deffilep.c"
    break;

  case 40: /* impline: ID '.' ID opt_equalequal_name  */
#line 212 "deffilep.y"
                 { def_import ( 0, (yyvsp[-3].id),	0, (yyvsp[-1].id), -1, (yyvsp[0].id)); }
#line 1624 "deffilep.c"
    break;

  case 43: /* secline: ID attr_list  */
#line 221 "deffilep.y"
                     { def_section ((yyvsp[-1].id), (yyvsp[0].number));}
#line 1630 "deffilep.c"
    break;

  case 44: /* secline: ID ID  */
#line 222 "deffilep.y"
                { def_section_alt ((yyvsp[-1].id), (yyvsp[0].id));}
#line 1636 "deffilep.c"
    break;

  case 45: /* attr_list: attr_list opt_comma attr  */
#line 226 "deffilep.y"
                                 { (yyval.number) = (yyvsp[-2].number) | (yyvsp[0].number); }
#line 1642 "deffilep.c"
    break;

  case 46: /* attr_list: attr  */
#line 227 "deffilep.y"
               { (yyval.number) = (yyvsp[0].number); }
#line 1648 "deffilep.c"
    break;

  case 49: /* opt_number: ',' NUMBER  */
#line 234 "deffilep.y"
                       { (yyval.number)=(yyvsp[0].number);}
#line 1654 "deffilep.c"
    break;

  case 50: /* opt_number: %empty  */
#line 235 "deffilep.y"
                   { (yyval.number)=-1;}
#line 1660 "deffilep.c"
    break;

  case 51: /* attr: READ  */
#line 239 "deffilep.y"
                        { (yyval.number) = 1;}
#line 1666 "deffilep.c"
    break;

  case 52: /* attr: WRITE  */
#line 240 "deffilep.y"
                        { (yyval.number) = 2;}
#line 1672 "deffilep.c"
    break;

  case 53: /* attr: EXECUTE  */
#line 241 "deffilep.y"
                        { (yyval.number)=4;}
#line 1678 "deffilep.c"
    break;

  case 54: /* attr: SHARED_K  */
#line 242 "deffilep.y"
                         { (yyval.number)=8;}
#line 1684 "deffilep.c"
    break;

  case 55: /* keyword_as_name: BASE  */
#line 246 "deffilep.y"
                      { (yyval.id_const) = "BASE"; }
#line 1690 "deffilep.c"
    break;

  case 56: /* keyword_as_name: CODE  */
#line 247 "deffilep.y"
                { (yyval.id_const) = "CODE"; }
#line 1696 "deffilep.c"
    break;

  case 57: /* keyword_as_name: CONSTANTU  */
#line 248 "deffilep.y"
                     { (yyval.id_const) = "CONSTANT"; }
#line 1702 "deffilep.c"
    break;

  case 58: /* keyword_as_name: CONSTANTL  */
#line 249 "deffilep.y"
                     { (yyval.id_const) = "constant"; }
#line 1708 "deffilep.c"
    break;

  case 59: /* keyword_as_name: DATAU  */
#line 250 "deffilep.y"
                 { (yyval.id_const) = "DATA"; }
#line 1714 "deffilep.c"
    break;

  case 60: /* keyword_as_name: DATAL  */
#line 251 "deffilep.y"
                 { (yyval.id_const) = "data"; }
#line 1720 "deffilep.c"
    break;

  case 61: /* keyword_as_name: DESCRIPTION  */
#line 252 "deffilep.y"
                       { (yyval.id_const) = "DESCRIPTION"; }
#line 1726 "deffilep.c"
    break;

  case 62: /* keyword_as_name: DIRECTIVE  */
#line 253 "deffilep.y"
                     { (yyval.id_const) = "DIRECTIVE"; }
#line 1732 "deffilep.c"
    break;

  case 63: /* keyword_as_name: EXCLUDE_SYMBOLS  */
#line 254 "deffilep.y"
                           { (yyval.id_const) = "EXCLUDE_SYMBOLS"; }
#line 1738 "deffilep.c"
    break;

  case 64: /* keyword_as_name: EXECUTE  */
#line 255 "deffilep.y"
                   { (yyval.id_const) = "EXECUTE"; }
#line 1744 "deffilep.c"
    break;

  case 65: /* keyword_as_name: EXPORTS  */
#line 256 "deffilep.y"
                   { (yyval.id_const) = "EXPORTS"; }
#line 1750 "deffilep.c"
    break;

  case 66: /* keyword_as_name: HEAPSIZE  */
#line 257 "deffilep.y"
                    { (yyval.id_const) = "HEAPSIZE"; }
#line 1756 "deffilep.c"
    break;

  case 67: /* keyword_as_name: IMPORTS  */
#line 258 "deffilep.y"
                   { (yyval.id_const) = "IMPORTS"; }
#line 1762 "deffilep.c"
    break;

  case 68: /* keyword_as_name: NAME  */
#line 265 "deffilep.y"
                { (yyval.id_const) = "NAME"; }
#line 1768 "deffilep.c"
    break;

  case 69: /* keyword_as_name: NONAMEU  */
#line 266 "deffilep.y"
                   { (yyval.id_const) = "NONAME"; }
#line 1774 "deffilep.c"
    break;

  case 70: /* keyword_as_name: NONAMEL  */
#line 267 "deffilep.y"
                   { (yyval.id_const) = "noname"; }
#line 1780 "deffilep.c"
    break;

  case 71: /* keyword_as_name: PRIVATEU  */
#line 268 "deffilep.y"
                    { (yyval.id_const) = "PRIVATE"; }
#line 1786 "deffilep.c"
    break;

  case 72: /* keyword_as_name: PRIVATEL  */
#line 269 "deffilep.y"
                    { (yyval.id_const) = "private"; }
#line 1792 "deffilep.c"
    break;

  case 73: /* keyword_as_name: READ  */
#line 270 "deffilep.y"
                { (yyval.id_const) = "READ"; }
#line 1798 "deffilep.c"
    break;

  case 74: /* keyword_as_name: SHARED_K  */
#line 271 "deffilep.y"
                     { (yyval.id_const) = "SHARED"; }
#line 1804 "deffilep.c"
    break;

  case 75: /* keyword_as_name: STACKSIZE_K  */
#line 272 "deffilep.y"
                       { (yyval.id_const) = "STACKSIZE"; }
#line 1810 "deffilep.c"
    break;

  case 76: /* keyword_as_name: VERSIONK  */
#line 273 "deffilep.y"
                    { (yyval.id_const) = "VERSION"; }
#line 1816 "deffilep.c"
    break;

  case 77: /* keyword_as_name: WRITE  */
#line 274 "deffilep.y"
                 { (yyval.id_const) = "WRITE"; }
#line 1822 "deffilep.c"
    break;

  case 78: /* opt_name2: ID  */
#line 277 "deffilep.y"
              { (yyval.id) = (yyvsp[0].id); }
#line 1828 "deffilep.c"
    break;

  case 79: /* opt_name2: '.' keyword_as_name  */
#line 279 "deffilep.y"
          {
	    char *name = xmalloc (strlen ((yyvsp[0].id_const)) + 2);
	    sprintf (name, ".%s", (yyvsp[0].id_const));
	    (yyval.id) = name;
	  }
#line 1838 "deffilep.c"
    break;

  case 80: /* opt_name2: '.' opt_name2  */
#line 285 "deffilep.y"
          {
	    char *name = def_pool_alloc (strlen ((yyvsp[0].id)) + 2);
	    sprintf (name, ".%s", (yyvsp[0].id));
	    (yyval.id) = name;
	  }
#line 1848 "deffilep.c"
    break;

  case 81: /* opt_name2: keyword_as_name '.' opt_name2  */
#line 291 "deffilep.y"
          {
	    char *name = def_pool_alloc (strlen ((yyvsp[-2].id_const)) + 1 + strlen ((yyvsp[0].id)) + 1);
	    sprintf (name, "%s.%s", (yyvsp[-2].id_const), (yyvsp[0].id));
	    (yyval.id) = name;
	  }
#line 1858 "deffilep.c"
    break;

  case 82: /* opt_name2: ID '.' opt_name2  */
#line 297 "deffilep.y"
          {
	    char *name = def_pool_alloc (strlen ((yyvsp[-2].id)) + 1 + strlen ((yyvsp[0].id)) + 1);
	    sprintf (name, "%s.%s", (yyvsp[-2].id), (yyvsp[0].id));
	    (yyval.id) = name;
	  }
#line 1868 "deffilep.c"
    break;

  case 83: /* opt_name: opt_name2  */
#line 304 "deffilep.y"
                    { (yyval.id) = (yyvsp[0].id); }
#line 1874 "deffilep.c"
    break;

  case 84: /* opt_name: %empty  */
#line 305 "deffilep.y"
                        { (yyval.id) = ""; }
#line 1880 "deffilep.c"
    break;

  case 85: /* opt_equalequal_name: EQUAL ID  */
#line 308 "deffilep.y"
                                { (yyval.id) = (yyvsp[0].id); }
#line 1886 "deffilep.c"
    break;

  case 86: /* opt_equalequal_name: %empty  */
#line 309 "deffilep.y"
                                                                { (yyval.id) = 0; }
#line 1892 "deffilep.c"
    break;

  case 87: /* opt_ordinal: '@' NUMBER  */
#line 313 "deffilep.y"
                         { (yyval.number) = (yyvsp[0].number);}
#line 1898 "deffilep.c"
    break;

  case 88: /* opt_ordinal: %empty  */
#line 314 "deffilep.y"
                         { (yyval.number) = -1;}
#line 1904 "deffilep.c"
    break;

  case 89: /* opt_equal_name: '=' opt_name2  */
#line 318 "deffilep.y"
                        { (yyval.id) = (yyvsp[0].id); }
#line 1910 "deffilep.c"
    break;

  case 90: /* opt_equal_name: %empty  */
#line 319 "deffilep.y"
                        { (yyval.id) =	0; }
#line 1916 "deffilep.c"
    break;

  case 91: /* opt_base: BASE '=' VMA  */
#line 322 "deffilep.y"
                        { (yyval.vma) = (yyvsp[0].vma);}
#line 1922 "deffilep.c"
    break;

  case 92: /* opt_base: %empty  */
#line 323 "deffilep.y"
                { (yyval.vma) = (bfd_vma) -1;}
#line 1928 "deffilep.c"
    break;

  case 93: /* anylang_id: ID  */
#line 326 "deffilep.y"
                        { (yyval.id) = (yyvsp[0].id); }
#line 1934 "deffilep.c"
    break;

  case 94: /* anylang_id: '.' ID  */
#line 328 "deffilep.y"
          {
	    char *id = def_pool_alloc (strlen ((yyvsp[0].id)) + 2);
	    sprintf (id, ".%s", (yyvsp[0].id));
	    (yyval.id) = id;
	  }
#line 1944 "deffilep.c"
    break;

  case 95: /* anylang_id: anylang_id '.' opt_digits opt_id  */
#line 334 "deffilep.y"
          {
	    char *id = def_pool_alloc (strlen ((yyvsp[-3].id)) + 1 + strlen ((yyvsp[-1].digits)) + strlen ((yyvsp[0].id)) + 1);
	    sprintf (id, "%s.%s%s", (yyvsp[-3].id), (yyvsp[-1].digits), (yyvsp[0].id));
	    (yyval.id) = id;
	  }
#line 1954 "deffilep.c"
    break;

  case 96: /* symbol_list: anylang_id  */
#line 342 "deffilep.y"
                   { def_exclude_symbols ((yyvsp[0].id)); }
#line 1960 "deffilep.c"
    break;

  case 97: /* symbol_list: symbol_list anylang_id  */
#line 343 "deffilep.y"
                                       { def_exclude_symbols ((yyvsp[0].id)); }
#line 1966 "deffilep.c"
    break;

  case 98: /* symbol_list: symbol_list ',' anylang_id  */
#line 344 "deffilep.y"
                                           { def_exclude_symbols ((yyvsp[0].id)); }
#line 1972 "deffilep.c"
    break;

  case 99: /* opt_digits: DIGITS  */
#line 347 "deffilep.y"
                        { (yyval.digits) = (yyvsp[0].digits); }
#line 1978 "deffilep.c"
    break;

  case 100: /* opt_digits: %empty  */
#line 348 "deffilep.y"
                        { (yyval.digits) = ""; }
#line 1984 "deffilep.c"
    break;

  case 101: /* opt_id: ID  */
#line 351 "deffilep.y"
                        { (yyval.id) = (yyvsp[0].id); }
#line 1990 "deffilep.c"
    break;

  case 102: /* opt_id: %empty  */
#line 352 "deffilep.y"
                        { (yyval.id) = ""; }
#line 1996 "deffilep.c"
    break;

  case 103: /* NUMBER: DIGITS  */
#line 355 "deffilep.y"
                        { (yyval.number) = strtoul ((yyvsp[0].digits), 0, 0); }
#line 2002 "deffilep.c"
    break;

  case 104: /* VMA: DIGITS  */
#line 357 "deffilep.y"
                        { (yyval.vma) = (bfd_vma) strtoull ((yyvsp[0].digits), 0, 0); }
#line 2008 "deffilep.c"
    break;


#line 2012 "deffilep.c"

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
  ++yynerrs;

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
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
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

#line 359 "deffilep.y"


/*****************************************************************************
 API
 *****************************************************************************/

static FILE *the_file;
static const char *def_filename;
static int linenumber;
static def_file *def;
static int saw_newline;

struct directive
  {
    struct directive *next;
    char *name;
    int len;
  };

static struct directive *directives = 0;

def_file *
def_file_empty (void)
{
  def_file *rv = xmalloc (sizeof (def_file));
  memset (rv, 0, sizeof (def_file));
  rv->is_dll = -1;
  rv->base_address = (bfd_vma) -1;
  rv->stack_reserve = rv->stack_commit = -1;
  rv->heap_reserve = rv->heap_commit = -1;
  rv->version_major = rv->version_minor = -1;
  return rv;
}

def_file *
def_file_parse (const char *filename, def_file *add_to)
{
  struct directive *d;

  the_file = fopen (filename, "r");
  def_filename = filename;
  linenumber = 1;
  if (!the_file)
    {
      perror (filename);
      return 0;
    }
  if (add_to)
    {
      def = add_to;
    }
  else
    {
      def = def_file_empty ();
    }

  saw_newline = 1;
  if (def_parse ())
    {
      def_file_free (def);
      fclose (the_file);
      def_pool_free ();
      return 0;
    }

  fclose (the_file);

  while ((d = directives) != NULL)
    {
#if TRACE
      printf ("Adding directive %08x `%s'\n", d->name, d->name);
#endif
      def_file_add_directive (def, d->name, d->len);
      directives = d->next;
      free (d->name);
      free (d);
    }
  def_pool_free ();

  return def;
}

void
def_file_free (def_file *fdef)
{
  int i;
  unsigned int ui;

  if (!fdef)
    return;
  free (fdef->name);
  free (fdef->description);

  if (fdef->section_defs)
    {
      for (i = 0; i < fdef->num_section_defs; i++)
	{
	  free (fdef->section_defs[i].name);
	  free (fdef->section_defs[i].class);
	}
      free (fdef->section_defs);
    }

  for (i = 0; i < fdef->num_exports; i++)
    {
      if (fdef->exports[i].internal_name != fdef->exports[i].name)
        free (fdef->exports[i].internal_name);
      free (fdef->exports[i].name);
      free (fdef->exports[i].its_name);
    }
  free (fdef->exports);

  for (i = 0; i < fdef->num_imports; i++)
    {
      if (fdef->imports[i].internal_name != fdef->imports[i].name)
        free (fdef->imports[i].internal_name);
      free (fdef->imports[i].name);
      free (fdef->imports[i].its_name);
    }
  free (fdef->imports);

  while (fdef->modules)
    {
      def_file_module *m = fdef->modules;

      fdef->modules = fdef->modules->next;
      free (m);
    }

  while (fdef->aligncomms)
    {
      def_file_aligncomm *c = fdef->aligncomms;

      fdef->aligncomms = fdef->aligncomms->next;
      free (c->symbol_name);
      free (c);
    }

  for (ui = 0; ui < fdef->num_exclude_symbols; ui++)
    {
      free (fdef->exclude_symbols[ui].symbol_name);
    }
  free (fdef->exclude_symbols);

  free (fdef);
}

#ifdef DEF_FILE_PRINT
void
def_file_print (FILE *file, def_file *fdef)
{
  int i;

  fprintf (file, ">>>> def_file at 0x%08x\n", fdef);
  if (fdef->name)
    fprintf (file, "  name: %s\n", fdef->name ? fdef->name : "(unspecified)");
  if (fdef->is_dll != -1)
    fprintf (file, "  is dll: %s\n", fdef->is_dll ? "yes" : "no");
  if (fdef->base_address != (bfd_vma) -1)
    fprintf (file, "  base address: 0x%" PRIx64 "\n",
	     (uint64_t) fdef->base_address);
  if (fdef->description)
    fprintf (file, "  description: `%s'\n", fdef->description);
  if (fdef->stack_reserve != -1)
    fprintf (file, "  stack reserve: 0x%08x\n", fdef->stack_reserve);
  if (fdef->stack_commit != -1)
    fprintf (file, "  stack commit: 0x%08x\n", fdef->stack_commit);
  if (fdef->heap_reserve != -1)
    fprintf (file, "  heap reserve: 0x%08x\n", fdef->heap_reserve);
  if (fdef->heap_commit != -1)
    fprintf (file, "  heap commit: 0x%08x\n", fdef->heap_commit);

  if (fdef->num_section_defs > 0)
    {
      fprintf (file, "  section defs:\n");

      for (i = 0; i < fdef->num_section_defs; i++)
	{
	  fprintf (file, "    name: `%s', class: `%s', flags:",
		   fdef->section_defs[i].name, fdef->section_defs[i].class);
	  if (fdef->section_defs[i].flag_read)
	    fprintf (file, " R");
	  if (fdef->section_defs[i].flag_write)
	    fprintf (file, " W");
	  if (fdef->section_defs[i].flag_execute)
	    fprintf (file, " X");
	  if (fdef->section_defs[i].flag_shared)
	    fprintf (file, " S");
	  fprintf (file, "\n");
	}
    }

  if (fdef->num_exports > 0)
    {
      fprintf (file, "  exports:\n");

      for (i = 0; i < fdef->num_exports; i++)
	{
	  fprintf (file, "    name: `%s', int: `%s', ordinal: %d, flags:",
		   fdef->exports[i].name, fdef->exports[i].internal_name,
		   fdef->exports[i].ordinal);
	  if (fdef->exports[i].flag_private)
	    fprintf (file, " P");
	  if (fdef->exports[i].flag_constant)
	    fprintf (file, " C");
	  if (fdef->exports[i].flag_noname)
	    fprintf (file, " N");
	  if (fdef->exports[i].flag_data)
	    fprintf (file, " D");
	  fprintf (file, "\n");
	}
    }

  if (fdef->num_imports > 0)
    {
      fprintf (file, "  imports:\n");

      for (i = 0; i < fdef->num_imports; i++)
	{
	  fprintf (file, "    int: %s, from: `%s', name: `%s', ordinal: %d\n",
		   fdef->imports[i].internal_name,
		   fdef->imports[i].module,
		   fdef->imports[i].name,
		   fdef->imports[i].ordinal);
	}
    }

  if (fdef->version_major != -1)
    fprintf (file, "  version: %d.%d\n", fdef->version_major, fdef->version_minor);

  fprintf (file, "<<<< def_file at 0x%08x\n", fdef);
}
#endif

/* Helper routine to check for identity of string pointers,
   which might be NULL.  */

static int
are_names_equal (const char *s1, const char *s2)
{
  if (!s1 && !s2)
    return 0;
  if (!s1 || !s2)
    return (!s1 ? -1 : 1);
  return strcmp (s1, s2);
}

static int
cmp_export_elem (const def_file_export *e, const char *ex_name,
		 const char *in_name, const char *its_name,
		 int ord)
{
  int r;

  if ((r = are_names_equal (ex_name, e->name)) != 0)
    return r;
  if ((r = are_names_equal (in_name, e->internal_name)) != 0)
    return r;
  if ((r = are_names_equal (its_name, e->its_name)) != 0)
    return r;
  return (ord - e->ordinal);
}

/* Search the position of the identical element, or returns the position
   of the next higher element. If last valid element is smaller, then MAX
   is returned. The max parameter indicates the number of elements in the
   array. On return, *is_ident indicates whether the returned array index
   points at an element which is identical to the one searched for.  */

static unsigned int
find_export_in_list (def_file_export *b, unsigned int max,
		     const char *ex_name, const char *in_name,
		     const char *its_name, int ord, bool *is_ident)
{
  int e;
  unsigned int l, r, p;

  *is_ident = false;
  if (!max)
    return 0;
  if ((e = cmp_export_elem (b, ex_name, in_name, its_name, ord)) <= 0)
    {
      if (!e)
	*is_ident = true;
      return 0;
    }
  if (max == 1)
    return 1;
  if ((e = cmp_export_elem (b + (max - 1), ex_name, in_name, its_name, ord)) > 0)
    return max;
  else if (!e || max == 2)
    {
      if (!e)
	*is_ident = true;
      return max - 1;
    }
  l = 0; r = max - 1;
  while (l < r)
    {
      p = (l + r) / 2;
      e = cmp_export_elem (b + p, ex_name, in_name, its_name, ord);
      if (!e)
	{
	  *is_ident = true;
	  return p;
	}
      else if (e < 0)
	r = p - 1;
      else if (e > 0)
	l = p + 1;
    }
  if ((e = cmp_export_elem (b + l, ex_name, in_name, its_name, ord)) > 0)
    ++l;
  else if (!e)
    *is_ident = true;
  return l;
}

def_file_export *
def_file_add_export (def_file *fdef,
		     const char *external_name,
		     const char *internal_name,
		     int ordinal,
		     const char *its_name,
		     bool *is_dup)
{
  def_file_export *e;
  unsigned int pos;

  if (internal_name && !external_name)
    external_name = internal_name;
  if (external_name && !internal_name)
    internal_name = external_name;

  /* We need to avoid duplicates.  */
  *is_dup = false;
  pos = find_export_in_list (fdef->exports, fdef->num_exports,
		     external_name, internal_name,
		     its_name, ordinal, is_dup);

  if (*is_dup)
    return (fdef->exports + pos);

  if ((unsigned)fdef->num_exports >= fdef->max_exports)
    {
      fdef->max_exports += SYMBOL_LIST_ARRAY_GROW;
      fdef->exports = xrealloc (fdef->exports,
				fdef->max_exports * sizeof (def_file_export));
    }

  e = fdef->exports + pos;
  /* If we're inserting in the middle of the array, we need to move the
     following elements forward.  */
  if (pos != (unsigned)fdef->num_exports)
    memmove (&e[1], e, (sizeof (def_file_export) * (fdef->num_exports - pos)));
  /* Wipe the element for use as a new entry.  */
  memset (e, 0, sizeof (def_file_export));
  e->name = xstrdup (external_name);
  e->internal_name = xstrdup (internal_name);
  e->its_name = (its_name ? xstrdup (its_name) : NULL);
  e->ordinal = ordinal;
  fdef->num_exports++;
  return e;
}

def_file_module *
def_get_module (def_file *fdef, const char *name)
{
  def_file_module *s;

  for (s = fdef->modules; s; s = s->next)
    if (strcmp (s->name, name) == 0)
      return s;

  return NULL;
}

static def_file_module *
def_stash_module (def_file *fdef, const char *name)
{
  def_file_module *s;

  if ((s = def_get_module (fdef, name)) != NULL)
      return s;
  s = xmalloc (sizeof (def_file_module) + strlen (name));
  s->next = fdef->modules;
  fdef->modules = s;
  s->user_data = 0;
  strcpy (s->name, name);
  return s;
}

static int
cmp_import_elem (const def_file_import *e, const char *ex_name,
		 const char *in_name, const char *module,
		 int ord)
{
  int r;

  if ((r = are_names_equal (module, (e->module ? e->module->name : NULL))))
    return r;
  if ((r = are_names_equal (ex_name, e->name)) != 0)
    return r;
  if ((r = are_names_equal (in_name, e->internal_name)) != 0)
    return r;
  if (ord != e->ordinal)
    return (ord < e->ordinal ? -1 : 1);
  return 0;
}

/* Search the position of the identical element, or returns the position
   of the next higher element. If last valid element is smaller, then MAX
   is returned. The max parameter indicates the number of elements in the
   array. On return, *is_ident indicates whether the returned array index
   points at an element which is identical to the one searched for.  */

static unsigned int
find_import_in_list (def_file_import *b, unsigned int max,
		     const char *ex_name, const char *in_name,
		     const char *module, int ord, bool *is_ident)
{
  int e;
  unsigned int l, r, p;

  *is_ident = false;
  if (!max)
    return 0;
  if ((e = cmp_import_elem (b, ex_name, in_name, module, ord)) <= 0)
    {
      if (!e)
	*is_ident = true;
      return 0;
    }
  if (max == 1)
    return 1;
  if ((e = cmp_import_elem (b + (max - 1), ex_name, in_name, module, ord)) > 0)
    return max;
  else if (!e || max == 2)
    {
      if (!e)
	*is_ident = true;
      return max - 1;
    }
  l = 0; r = max - 1;
  while (l < r)
    {
      p = (l + r) / 2;
      e = cmp_import_elem (b + p, ex_name, in_name, module, ord);
      if (!e)
	{
	  *is_ident = true;
	  return p;
	}
      else if (e < 0)
	r = p - 1;
      else if (e > 0)
	l = p + 1;
    }
  if ((e = cmp_import_elem (b + l, ex_name, in_name, module, ord)) > 0)
    ++l;
  else if (!e)
    *is_ident = true;
  return l;
}

static void
fill_in_import (def_file_import *i,
		const char *name,
		def_file_module *module,
		int ordinal,
		const char *internal_name,
		const char *its_name)
{
  memset (i, 0, sizeof (def_file_import));
  if (name)
    i->name = xstrdup (name);
  i->module = module;
  i->ordinal = ordinal;
  if (internal_name)
    i->internal_name = xstrdup (internal_name);
  else
    i->internal_name = i->name;
  i->its_name = (its_name ? xstrdup (its_name) : NULL);
}

def_file_import *
def_file_add_import (def_file *fdef,
		     const char *name,
		     const char *module,
		     int ordinal,
		     const char *internal_name,
		     const char *its_name,
		     bool *is_dup)
{
  def_file_import *i;
  unsigned int pos;

  /* We need to avoid here duplicates.  */
  *is_dup = false;
  pos = find_import_in_list (fdef->imports, fdef->num_imports,
			     name,
			     (!internal_name ? name : internal_name),
			     module, ordinal, is_dup);
  if (*is_dup)
    return fdef->imports + pos;

  if ((unsigned)fdef->num_imports >= fdef->max_imports)
    {
      fdef->max_imports += SYMBOL_LIST_ARRAY_GROW;
      fdef->imports = xrealloc (fdef->imports,
				fdef->max_imports * sizeof (def_file_import));
    }
  i = fdef->imports + pos;
  /* If we're inserting in the middle of the array, we need to move the
     following elements forward.  */
  if (pos != (unsigned)fdef->num_imports)
    memmove (i + 1, i, sizeof (def_file_import) * (fdef->num_imports - pos));

  fill_in_import (i, name, def_stash_module (fdef, module), ordinal,
		  internal_name, its_name);
  fdef->num_imports++;

  return i;
}

int
def_file_add_import_from (def_file *fdef,
			  int num_imports,
			  const char *name,
			  const char *module,
			  int ordinal,
			  const char *internal_name,
			  const char *its_name ATTRIBUTE_UNUSED)
{
  def_file_import *i;
  bool is_dup;
  unsigned int pos;

  /* We need to avoid here duplicates.  */
  is_dup = false;
  pos = find_import_in_list (fdef->imports, fdef->num_imports,
			     name, internal_name ? internal_name : name,
			     module, ordinal, &is_dup);
  if (is_dup)
    return -1;
  if (fdef->imports && pos != (unsigned)fdef->num_imports)
    {
      i = fdef->imports + pos;
      if (i->module && strcmp (i->module->name, module) == 0)
	return -1;
    }

  if ((unsigned)fdef->num_imports + num_imports - 1 >= fdef->max_imports)
    {
      fdef->max_imports = fdef->num_imports + num_imports +
			  SYMBOL_LIST_ARRAY_GROW;

      fdef->imports = xrealloc (fdef->imports,
				fdef->max_imports * sizeof (def_file_import));
    }
  i = fdef->imports + pos;
  /* If we're inserting in the middle of the array, we need to move the
     following elements forward.  */
  if (pos != (unsigned)fdef->num_imports)
    memmove (i + num_imports, i,
	     sizeof (def_file_import) * (fdef->num_imports - pos));

  return pos;
}

def_file_import *
def_file_add_import_at (def_file *fdef,
			int pos,
			const char *name,
			const char *module,
			int ordinal,
			const char *internal_name,
			const char *its_name)
{
  def_file_import *i = fdef->imports + pos;

  fill_in_import (i, name, def_stash_module (fdef, module), ordinal,
		  internal_name, its_name);
  fdef->num_imports++;

  return i;
}

/* Search the position of the identical element, or returns the position
   of the next higher element. If last valid element is smaller, then MAX
   is returned. The max parameter indicates the number of elements in the
   array. On return, *is_ident indicates whether the returned array index
   points at an element which is identical to the one searched for.  */

static unsigned int
find_exclude_in_list (def_file_exclude_symbol *b, unsigned int max,
		      const char *name, bool *is_ident)
{
  int e;
  unsigned int l, r, p;

  *is_ident = false;
  if (!max)
    return 0;
  if ((e = strcmp (b[0].symbol_name, name)) <= 0)
    {
      if (!e)
	*is_ident = true;
      return 0;
    }
  if (max == 1)
    return 1;
  if ((e = strcmp (b[max - 1].symbol_name, name)) > 0)
    return max;
  else if (!e || max == 2)
    {
      if (!e)
	*is_ident = true;
      return max - 1;
    }
  l = 0; r = max - 1;
  while (l < r)
    {
      p = (l + r) / 2;
      e = strcmp (b[p].symbol_name, name);
      if (!e)
	{
	  *is_ident = true;
	  return p;
	}
      else if (e < 0)
	r = p - 1;
      else if (e > 0)
	l = p + 1;
    }
  if ((e = strcmp (b[l].symbol_name, name)) > 0)
    ++l;
  else if (!e)
    *is_ident = true;
  return l;
}

static def_file_exclude_symbol *
def_file_add_exclude_symbol (def_file *fdef, const char *name)
{
  def_file_exclude_symbol *e;
  unsigned pos;
  bool is_dup = false;

  pos = find_exclude_in_list (fdef->exclude_symbols, fdef->num_exclude_symbols,
			      name, &is_dup);

  /* We need to avoid duplicates.  */
  if (is_dup)
    return (fdef->exclude_symbols + pos);

  if (fdef->num_exclude_symbols >= fdef->max_exclude_symbols)
    {
      fdef->max_exclude_symbols += SYMBOL_LIST_ARRAY_GROW;
      fdef->exclude_symbols = xrealloc (fdef->exclude_symbols,
					fdef->max_exclude_symbols * sizeof (def_file_exclude_symbol));
    }

  e = fdef->exclude_symbols + pos;
  /* If we're inserting in the middle of the array, we need to move the
     following elements forward.  */
  if (pos != fdef->num_exclude_symbols)
    memmove (&e[1], e, (sizeof (def_file_exclude_symbol) * (fdef->num_exclude_symbols - pos)));
  /* Wipe the element for use as a new entry.  */
  memset (e, 0, sizeof (def_file_exclude_symbol));
  e->symbol_name = xstrdup (name);
  fdef->num_exclude_symbols++;
  return e;
}

struct
{
  char *param;
  int token;
}
diropts[] =
{
  { "-heap", HEAPSIZE },
  { "-stack", STACKSIZE_K },
  { "-attr", SECTIONS },
  { "-export", EXPORTS },
  { "-aligncomm", ALIGNCOMM },
  { "-exclude-symbols", EXCLUDE_SYMBOLS },
  { 0, 0 }
};

void
def_file_add_directive (def_file *my_def, const char *param, int len)
{
  def_file *save_def = def;
  const char *pend = param + len;
  char * tend = (char *) param;
  int i;

  def = my_def;

  while (param < pend)
    {
      while (param < pend
	     && (ISSPACE (*param) || *param == '\n' || *param == 0))
	param++;

      if (param == pend)
	break;

      /* Scan forward until we encounter any of:
	  - the end of the buffer
	  - the start of a new option
	  - a newline separating options
	  - a NUL separating options.  */
      for (tend = (char *) (param + 1);
	   (tend < pend
	    && !(ISSPACE (tend[-1]) && *tend == '-')
	    && *tend != '\n' && *tend != 0);
	   tend++)
	;

      for (i = 0; diropts[i].param; i++)
	{
	  len = strlen (diropts[i].param);

	  if (tend - param >= len
	      && strncmp (param, diropts[i].param, len) == 0
	      && (param[len] == ':' || param[len] == ' '))
	    {
	      lex_parse_string_end = tend;
	      lex_parse_string = param + len + 1;
	      lex_forced_token = diropts[i].token;
	      saw_newline = 0;
	      if (def_parse ())
		continue;
	      break;
	    }
	}

      if (!diropts[i].param)
	{
	  if (tend < pend)
	    {
	      char saved;

	      saved = * tend;
	      * tend = 0;
	      /* xgettext:c-format */
	      einfo (_("Warning: .drectve `%s' unrecognized\n"), param);
	      * tend = saved;
	    }
	  else
	    {
	      einfo (_("Warning: corrupt .drectve at end of def file\n"));
	    }
	}

      lex_parse_string = 0;
      param = tend;
    }

  def = save_def;
  def_pool_free ();
}

/* Parser Callbacks.  */

static void
def_image_name (const char *name, bfd_vma base, int is_dll)
{
  /* If a LIBRARY or NAME statement is specified without a name, there is nothing
     to do here.  We retain the output filename specified on command line.  */
  if (*name)
    {
      const char* image_name = lbasename (name);

      if (image_name != name)
	einfo (_("%s:%d: Warning: path components stripped from %s, '%s'\n"),
	       def_filename, linenumber, is_dll ? "LIBRARY" : "NAME",
	       name);
      free (def->name);
      /* Append the default suffix, if none specified.  */
      if (strchr (image_name, '.') == 0)
	{
	  const char * suffix = is_dll ? ".dll" : ".exe";

	  def->name = xmalloc (strlen (image_name) + strlen (suffix) + 1);
	  sprintf (def->name, "%s%s", image_name, suffix);
	}
      else
	def->name = xstrdup (image_name);
    }

  /* Honor a BASE address statement, even if LIBRARY string is empty.  */
  def->base_address = base;
  def->is_dll = is_dll;
}

static void
def_description (const char *text)
{
  int len = def->description ? strlen (def->description) : 0;

  len += strlen (text) + 1;
  if (def->description)
    {
      def->description = xrealloc (def->description, len);
      strcat (def->description, text);
    }
  else
    {
      def->description = xmalloc (len);
      strcpy (def->description, text);
    }
}

static void
def_stacksize (int reserve, int commit)
{
  def->stack_reserve = reserve;
  def->stack_commit = commit;
}

static void
def_heapsize (int reserve, int commit)
{
  def->heap_reserve = reserve;
  def->heap_commit = commit;
}

static void
def_section (const char *name, int attr)
{
  def_file_section *s;
  int max_sections = ROUND_UP (def->num_section_defs, 4);

  if (def->num_section_defs >= max_sections)
    {
      max_sections = ROUND_UP (def->num_section_defs+1, 4);

      if (def->section_defs)
	def->section_defs = xrealloc (def->section_defs,
				      max_sections * sizeof (def_file_import));
      else
	def->section_defs = xmalloc (max_sections * sizeof (def_file_import));
    }
  s = def->section_defs + def->num_section_defs;
  memset (s, 0, sizeof (def_file_section));
  s->name = xstrdup (name);
  if (attr & 1)
    s->flag_read = 1;
  if (attr & 2)
    s->flag_write = 1;
  if (attr & 4)
    s->flag_execute = 1;
  if (attr & 8)
    s->flag_shared = 1;

  def->num_section_defs++;
}

static void
def_section_alt (const char *name, const char *attr)
{
  int aval = 0;

  for (; *attr; attr++)
    {
      switch (*attr)
	{
	case 'R':
	case 'r':
	  aval |= 1;
	  break;
	case 'W':
	case 'w':
	  aval |= 2;
	  break;
	case 'X':
	case 'x':
	  aval |= 4;
	  break;
	case 'S':
	case 's':
	  aval |= 8;
	  break;
	}
    }
  def_section (name, aval);
}

static void
def_exports (const char *external_name,
	     const char *internal_name,
	     int ordinal,
	     int flags,
	     const char *its_name)
{
  def_file_export *dfe;
  bool is_dup = false;

  if (!internal_name && external_name)
    internal_name = external_name;
#if TRACE
  printf ("def_exports, ext=%s int=%s\n", external_name, internal_name);
#endif

  dfe = def_file_add_export (def, external_name, internal_name, ordinal,
			     its_name, &is_dup);

  /* We might check here for flag redefinition and warn.  For now we
     ignore duplicates silently.  */
  if (is_dup)
    return;

  if (flags & 1)
    dfe->flag_noname = 1;
  if (flags & 2)
    dfe->flag_constant = 1;
  if (flags & 4)
    dfe->flag_data = 1;
  if (flags & 8)
    dfe->flag_private = 1;
}

static void
def_import (const char *internal_name,
	    const char *module,
	    const char *dllext,
	    const char *name,
	    int ordinal,
	    const char *its_name)
{
  char *buf = 0;
  const char *ext = dllext ? dllext : "dll";
  bool is_dup = false;

  buf = xmalloc (strlen (module) + strlen (ext) + 2);
  sprintf (buf, "%s.%s", module, ext);
  module = buf;

  def_file_add_import (def, name, module, ordinal, internal_name, its_name,
		       &is_dup);
  free (buf);
}

static void
def_version (int major, int minor)
{
  def->version_major = major;
  def->version_minor = minor;
}

static void
def_directive (char *str)
{
  struct directive *d = xmalloc (sizeof (struct directive));

  d->next = directives;
  directives = d;
  d->name = xstrdup (str);
  d->len = strlen (str);
}

static void
def_aligncomm (char *str, int align)
{
  def_file_aligncomm *c, *p;

  p = NULL;
  c = def->aligncomms;
  while (c != NULL)
    {
      int e = strcmp (c->symbol_name, str);
      if (!e)
	{
	  /* Not sure if we want to allow here duplicates with
	     different alignments, but for now we keep them.  */
	  e = (int) c->alignment - align;
	  if (!e)
	    return;
	}
      if (e > 0)
	break;
      c = (p = c)->next;
    }

  c = xmalloc (sizeof (def_file_aligncomm));
  c->symbol_name = xstrdup (str);
  c->alignment = (unsigned int) align;
  if (!p)
    {
      c->next = def->aligncomms;
      def->aligncomms = c;
    }
  else
    {
      c->next = p->next;
      p->next = c;
    }
}

static void
def_exclude_symbols (char *str)
{
  def_file_add_exclude_symbol (def, str);
}

static void
def_error (const char *err)
{
  einfo ("%P: %s:%d: %s\n",
	 def_filename ? def_filename : "<unknown-file>", linenumber, err);
}


/* Lexical Scanner.  */

#undef TRACE
#define TRACE 0

/* Never freed, but always reused as needed, so no real leak.  */
static char *buffer = 0;
static int buflen = 0;
static int bufptr = 0;

static void
put_buf (char c)
{
  if (bufptr == buflen)
    {
      buflen += 50;		/* overly reasonable, eh?  */
      if (buffer)
	buffer = xrealloc (buffer, buflen + 1);
      else
	buffer = xmalloc (buflen + 1);
    }
  buffer[bufptr++] = c;
  buffer[bufptr] = 0;		/* not optimal, but very convenient.  */
}

static struct
{
  char *name;
  int token;
}
tokens[] =
{
  { "BASE", BASE },
  { "CODE", CODE },
  { "CONSTANT", CONSTANTU },
  { "constant", CONSTANTL },
  { "DATA", DATAU },
  { "data", DATAL },
  { "DESCRIPTION", DESCRIPTION },
  { "DIRECTIVE", DIRECTIVE },
  { "EXCLUDE_SYMBOLS", EXCLUDE_SYMBOLS },
  { "EXECUTE", EXECUTE },
  { "EXPORTS", EXPORTS },
  { "HEAPSIZE", HEAPSIZE },
  { "IMPORTS", IMPORTS },
  { "LIBRARY", LIBRARY },
  { "NAME", NAME },
  { "NONAME", NONAMEU },
  { "noname", NONAMEL },
  { "PRIVATE", PRIVATEU },
  { "private", PRIVATEL },
  { "READ", READ },
  { "SECTIONS", SECTIONS },
  { "SEGMENTS", SECTIONS },
  { "SHARED", SHARED_K },
  { "STACKSIZE", STACKSIZE_K },
  { "VERSION", VERSIONK },
  { "WRITE", WRITE },
  { 0, 0 }
};

static int
def_getc (void)
{
  int rv;

  if (lex_parse_string)
    {
      if (lex_parse_string >= lex_parse_string_end)
	rv = EOF;
      else
	rv = *lex_parse_string++;
    }
  else
    {
      rv = fgetc (the_file);
    }
  if (rv == '\n')
    saw_newline = 1;
  return rv;
}

static int
def_ungetc (int c)
{
  if (lex_parse_string)
    {
      lex_parse_string--;
      return c;
    }
  else
    return ungetc (c, the_file);
}

static int
def_lex (void)
{
  int c, i, q;

  if (lex_forced_token)
    {
      i = lex_forced_token;
      lex_forced_token = 0;
#if TRACE
      printf ("lex: forcing token %d\n", i);
#endif
      return i;
    }

  c = def_getc ();

  /* Trim leading whitespace.  */
  while (c != EOF && (c == ' ' || c == '\t') && saw_newline)
    c = def_getc ();

  if (c == EOF)
    {
#if TRACE
      printf ("lex: EOF\n");
#endif
      return 0;
    }

  if (saw_newline && c == ';')
    {
      do
	{
	  c = def_getc ();
	}
      while (c != EOF && c != '\n');
      if (c == '\n')
	return def_lex ();
      return 0;
    }

  /* Must be something else.  */
  saw_newline = 0;

  if (ISDIGIT (c))
    {
      bufptr = 0;
      while (c != EOF && (ISXDIGIT (c) || (c == 'x')))
	{
	  put_buf (c);
	  c = def_getc ();
	}
      if (c != EOF)
	def_ungetc (c);
      yylval.digits = def_pool_strdup (buffer);
#if TRACE
      printf ("lex: `%s' returns DIGITS\n", buffer);
#endif
      return DIGITS;
    }

  if (ISALPHA (c) || strchr ("$:-_?@", c))
    {
      bufptr = 0;
      q = c;
      put_buf (c);
      c = def_getc ();

      if (q == '@')
	{
	  if (ISBLANK (c) ) /* '@' followed by whitespace.  */
	    return (q);
	  else if (ISDIGIT (c)) /* '@' followed by digit.  */
	    {
	      def_ungetc (c);
	      return (q);
	    }
#if TRACE
	  printf ("lex: @ returns itself\n");
#endif
	}

      while (c != EOF && (ISALNUM (c) || strchr ("$:-_?/@<>", c)))
	{
	  put_buf (c);
	  c = def_getc ();
	}
      if (c != EOF)
	def_ungetc (c);
      if (ISALPHA (q)) /* Check for tokens.  */
	{
	  for (i = 0; tokens[i].name; i++)
	    if (strcmp (tokens[i].name, buffer) == 0)
	      {
#if TRACE
	        printf ("lex: `%s' is a string token\n", buffer);
#endif
	        return tokens[i].token;
	      }
	}
#if TRACE
      printf ("lex: `%s' returns ID\n", buffer);
#endif
      yylval.id = def_pool_strdup (buffer);
      return ID;
    }

  if (c == '\'' || c == '"')
    {
      q = c;
      c = def_getc ();
      bufptr = 0;

      while (c != EOF && c != q)
	{
	  put_buf (c);
	  c = def_getc ();
	}
      yylval.id = def_pool_strdup (buffer);
#if TRACE
      printf ("lex: `%s' returns ID\n", buffer);
#endif
      return ID;
    }

  if ( c == '=')
    {
      c = def_getc ();
      if (c == '=')
	{
#if TRACE
	  printf ("lex: `==' returns EQUAL\n");
#endif
	  return EQUAL;
	}
      def_ungetc (c);
#if TRACE
      printf ("lex: `=' returns itself\n");
#endif
      return '=';
    }
  if (c == '.' || c == ',')
    {
#if TRACE
      printf ("lex: `%c' returns itself\n", c);
#endif
      return c;
    }

  if (c == '\n')
    {
      linenumber++;
      saw_newline = 1;
    }

  /*printf ("lex: 0x%02x ignored\n", c); */
  return def_lex ();
}

static char *
def_pool_alloc (size_t sz)
{
  def_pool_str *e;

  e = (def_pool_str *) xmalloc (sizeof (def_pool_str) + sz);
  e->next = pool_strs;
  pool_strs = e;
  return e->data;
}

static char *
def_pool_strdup (const char *str)
{
  char *s;
  size_t len;
  if (!str)
    return NULL;
  len = strlen (str) + 1;
  s = def_pool_alloc (len);
  memcpy (s, str, len);
  return s;
}

static void
def_pool_free (void)
{
  def_pool_str *p;
  while ((p = pool_strs) != NULL)
    {
      pool_strs = p->next;
      free (p);
    }
}
