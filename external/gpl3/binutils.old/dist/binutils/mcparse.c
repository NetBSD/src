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
#line 1 "mcparse.y"
 /* mcparse.y -- parser for Windows mc files
  Copyright (C) 2007-2022 Free Software Foundation, Inc.

  Parser for Windows mc files
  Written by Kai Tietz, Onevision.

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
  Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA
  02110-1301, USA.  */

/* This is a parser for Windows rc files.  It is based on the parser
   by Gunther Ebert <gunther.ebert@ixos-leipzig.de>.  */

#include "sysdep.h"
#include "bfd.h"
#include "bucomm.h"
#include "libiberty.h"
#include "windmc.h"
#include "safe-ctype.h"

static rc_uint_type mc_last_id = 0;
static rc_uint_type mc_sefa_val = 0;
static unichar *mc_last_symbol = NULL;
static const mc_keyword *mc_cur_severity = NULL;
static const mc_keyword *mc_cur_facility = NULL;
static mc_node *cur_node = NULL;


#line 113 "mcparse.c"

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
#ifndef YY_YY_MCPARSE_H_INCLUDED
# define YY_YY_MCPARSE_H_INCLUDED
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
    NL = 258,                      /* NL  */
    MCIDENT = 259,                 /* MCIDENT  */
    MCFILENAME = 260,              /* MCFILENAME  */
    MCLINE = 261,                  /* MCLINE  */
    MCCOMMENT = 262,               /* MCCOMMENT  */
    MCTOKEN = 263,                 /* MCTOKEN  */
    MCENDLINE = 264,               /* MCENDLINE  */
    MCLANGUAGENAMES = 265,         /* MCLANGUAGENAMES  */
    MCFACILITYNAMES = 266,         /* MCFACILITYNAMES  */
    MCSEVERITYNAMES = 267,         /* MCSEVERITYNAMES  */
    MCOUTPUTBASE = 268,            /* MCOUTPUTBASE  */
    MCMESSAGEIDTYPEDEF = 269,      /* MCMESSAGEIDTYPEDEF  */
    MCLANGUAGE = 270,              /* MCLANGUAGE  */
    MCMESSAGEID = 271,             /* MCMESSAGEID  */
    MCSEVERITY = 272,              /* MCSEVERITY  */
    MCFACILITY = 273,              /* MCFACILITY  */
    MCSYMBOLICNAME = 274,          /* MCSYMBOLICNAME  */
    MCNUMBER = 275                 /* MCNUMBER  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define NL 258
#define MCIDENT 259
#define MCFILENAME 260
#define MCLINE 261
#define MCCOMMENT 262
#define MCTOKEN 263
#define MCENDLINE 264
#define MCLANGUAGENAMES 265
#define MCFACILITYNAMES 266
#define MCSEVERITYNAMES 267
#define MCOUTPUTBASE 268
#define MCMESSAGEIDTYPEDEF 269
#define MCLANGUAGE 270
#define MCMESSAGEID 271
#define MCSEVERITY 272
#define MCFACILITY 273
#define MCSYMBOLICNAME 274
#define MCNUMBER 275

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 44 "mcparse.y"

  rc_uint_type ival;
  unichar *ustr;
  const mc_keyword *tok;
  mc_node *nod;

#line 213 "mcparse.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_MCPARSE_H_INCLUDED  */
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_NL = 3,                         /* NL  */
  YYSYMBOL_MCIDENT = 4,                    /* MCIDENT  */
  YYSYMBOL_MCFILENAME = 5,                 /* MCFILENAME  */
  YYSYMBOL_MCLINE = 6,                     /* MCLINE  */
  YYSYMBOL_MCCOMMENT = 7,                  /* MCCOMMENT  */
  YYSYMBOL_MCTOKEN = 8,                    /* MCTOKEN  */
  YYSYMBOL_MCENDLINE = 9,                  /* MCENDLINE  */
  YYSYMBOL_MCLANGUAGENAMES = 10,           /* MCLANGUAGENAMES  */
  YYSYMBOL_MCFACILITYNAMES = 11,           /* MCFACILITYNAMES  */
  YYSYMBOL_MCSEVERITYNAMES = 12,           /* MCSEVERITYNAMES  */
  YYSYMBOL_MCOUTPUTBASE = 13,              /* MCOUTPUTBASE  */
  YYSYMBOL_MCMESSAGEIDTYPEDEF = 14,        /* MCMESSAGEIDTYPEDEF  */
  YYSYMBOL_MCLANGUAGE = 15,                /* MCLANGUAGE  */
  YYSYMBOL_MCMESSAGEID = 16,               /* MCMESSAGEID  */
  YYSYMBOL_MCSEVERITY = 17,                /* MCSEVERITY  */
  YYSYMBOL_MCFACILITY = 18,                /* MCFACILITY  */
  YYSYMBOL_MCSYMBOLICNAME = 19,            /* MCSYMBOLICNAME  */
  YYSYMBOL_MCNUMBER = 20,                  /* MCNUMBER  */
  YYSYMBOL_21_ = 21,                       /* '='  */
  YYSYMBOL_22_ = 22,                       /* '('  */
  YYSYMBOL_23_ = 23,                       /* ')'  */
  YYSYMBOL_24_ = 24,                       /* ':'  */
  YYSYMBOL_25_ = 25,                       /* '+'  */
  YYSYMBOL_YYACCEPT = 26,                  /* $accept  */
  YYSYMBOL_input = 27,                     /* input  */
  YYSYMBOL_entities = 28,                  /* entities  */
  YYSYMBOL_entity = 29,                    /* entity  */
  YYSYMBOL_global_section = 30,            /* global_section  */
  YYSYMBOL_severitymaps = 31,              /* severitymaps  */
  YYSYMBOL_severitymap = 32,               /* severitymap  */
  YYSYMBOL_facilitymaps = 33,              /* facilitymaps  */
  YYSYMBOL_facilitymap = 34,               /* facilitymap  */
  YYSYMBOL_langmaps = 35,                  /* langmaps  */
  YYSYMBOL_langmap = 36,                   /* langmap  */
  YYSYMBOL_alias_name = 37,                /* alias_name  */
  YYSYMBOL_message = 38,                   /* message  */
  YYSYMBOL_39_1 = 39,                      /* $@1  */
  YYSYMBOL_id = 40,                        /* id  */
  YYSYMBOL_vid = 41,                       /* vid  */
  YYSYMBOL_sefasy_def = 42,                /* sefasy_def  */
  YYSYMBOL_severity = 43,                  /* severity  */
  YYSYMBOL_facility = 44,                  /* facility  */
  YYSYMBOL_symbol = 45,                    /* symbol  */
  YYSYMBOL_lang_entities = 46,             /* lang_entities  */
  YYSYMBOL_lang_entity = 47,               /* lang_entity  */
  YYSYMBOL_lines = 48,                     /* lines  */
  YYSYMBOL_comments = 49,                  /* comments  */
  YYSYMBOL_lang = 50,                      /* lang  */
  YYSYMBOL_token = 51,                     /* token  */
  YYSYMBOL_lex_want_nl = 52,               /* lex_want_nl  */
  YYSYMBOL_lex_want_line = 53,             /* lex_want_line  */
  YYSYMBOL_lex_want_filename = 54          /* lex_want_filename  */
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
typedef yytype_int8 yy_state_t;

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
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   114

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  26
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  29
/* YYNRULES -- Number of rules.  */
#define YYNRULES  82
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  125

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   275


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
      22,    23,     2,    25,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    24,     2,
       2,    21,     2,     2,     2,     2,     2,     2,     2,     2,
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
      15,    16,    17,    18,    19,    20
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,    66,    66,    69,    71,    73,    74,    75,    80,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,   102,   106,   110,   117,   118,   119,   123,   127,
     128,   132,   133,   134,   138,   142,   143,   147,   148,   149,
     153,   157,   158,   159,   160,   165,   168,   172,   177,   176,
     190,   191,   192,   196,   199,   203,   207,   212,   219,   225,
     231,   239,   247,   255,   262,   263,   267,   277,   281,   293,
     294,   297,   298,   312,   316,   321,   326,   331,   338,   339,
     343,   347,   351
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
  "\"end of file\"", "error", "\"invalid token\"", "NL", "MCIDENT",
  "MCFILENAME", "MCLINE", "MCCOMMENT", "MCTOKEN", "MCENDLINE",
  "MCLANGUAGENAMES", "MCFACILITYNAMES", "MCSEVERITYNAMES", "MCOUTPUTBASE",
  "MCMESSAGEIDTYPEDEF", "MCLANGUAGE", "MCMESSAGEID", "MCSEVERITY",
  "MCFACILITY", "MCSYMBOLICNAME", "MCNUMBER", "'='", "'('", "')'", "':'",
  "'+'", "$accept", "input", "entities", "entity", "global_section",
  "severitymaps", "severitymap", "facilitymaps", "facilitymap", "langmaps",
  "langmap", "alias_name", "message", "$@1", "id", "vid", "sefasy_def",
  "severity", "facility", "symbol", "lang_entities", "lang_entity",
  "lines", "comments", "lang", "token", "lex_want_nl", "lex_want_line",
  "lex_want_filename", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-34)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-83)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int8 yypact[] =
{
     -34,    62,    70,   -34,   -34,   -34,    15,    22,    30,   -15,
      34,    37,   -34,   -34,   -34,   -34,    56,   -34,    10,   -34,
      12,   -34,    20,    25,   -34,    52,   -34,     0,    80,   -34,
     -34,    71,   -34,    84,   -34,    86,   -34,   -34,   -34,   -34,
     -34,    45,   -34,     1,    68,    74,    76,   -34,   -34,   -34,
     -34,   -34,   -34,     4,   -34,    38,   -34,     6,   -34,    39,
     -34,    29,   -34,    40,   -34,   -34,    93,    94,    99,    43,
      76,   -34,   -34,   -34,   -34,   -34,   -34,    46,   -34,   -34,
     -34,   -34,    47,   -34,   -34,   -34,   -34,    49,   -34,   -34,
     -34,   -34,    83,   -34,     3,   -34,     2,   -34,    81,   -34,
      81,    92,   -34,   -34,    48,   -34,    82,    72,   -34,   -34,
     -34,   104,   105,   108,   -34,   -34,   -34,    73,   -34,   -34,
     -34,   -34,   -34,   -34,   -34
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       3,     0,     0,     1,     8,    71,     0,     0,     0,     0,
       0,     0,     4,     5,     6,    57,     7,    16,     0,    20,
       0,    12,     0,     0,    24,     0,    52,     0,    48,    72,
      15,     0,    19,     0,    11,     0,    21,    23,    22,    51,
      54,     0,    50,     0,     0,     0,     0,    58,    59,    60,
      39,    78,    79,     0,    37,     0,    33,     0,    31,     0,
      27,     0,    25,     0,    56,    55,     0,     0,     0,     0,
      49,    64,    81,    14,    13,    38,    44,     0,    18,    17,
      32,    36,     0,    10,     9,    26,    30,     0,    61,    62,
      63,    77,     0,    65,     0,    43,     0,    35,    45,    29,
      45,     0,    69,    67,     0,    42,     0,     0,    34,    28,
      76,    78,    79,     0,    70,    68,    66,     0,    47,    46,
      74,    73,    75,    41,    40
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -34,   -34,   -34,   -34,   -34,   -34,    50,   -34,    53,   -34,
      59,    13,   -34,   -34,   -34,   -34,   -34,   -34,   -34,   -34,
     -34,    44,   -34,   -34,   -34,   -33,   -34,   -34,   -34
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,     1,     2,    12,    13,    61,    62,    57,    58,    53,
      54,   108,    14,    46,    15,    42,    28,    47,    48,    49,
      70,    71,   104,    16,    72,    55,    92,    94,   106
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int8 yytable[] =
{
      59,    39,    63,   105,   102,    73,    23,    78,    51,   103,
      51,    30,    52,    32,    52,   -53,    17,   -53,   -53,   -53,
      40,    34,    66,    19,    59,    41,   -82,    74,    63,    79,
      83,    21,    31,    51,    33,    24,    18,    52,    26,    76,
      81,    86,    35,    20,    91,    36,    64,    95,    97,   114,
      99,    22,    84,    37,   115,    25,    38,   116,    27,    77,
      82,    87,     3,    29,   -80,    65,    96,    98,   113,   100,
      -2,     4,    50,   118,   123,    51,   119,     5,   124,    52,
       6,     7,     8,     9,    10,    56,    11,    60,    51,    67,
      51,    69,    52,   110,    52,    68,   111,    43,    44,    45,
     112,    88,    89,    90,   101,   107,   117,   120,   121,   122,
      80,    85,    75,   109,    93
};

static const yytype_int8 yycheck[] =
{
      33,     1,    35,     1,     1,     1,    21,     1,     4,     6,
       4,     1,     8,     1,     8,    15,     1,    17,    18,    19,
      20,     1,    21,     1,    57,    25,    24,    23,    61,    23,
       1,     1,    22,     4,    22,     1,    21,     8,     1,     1,
       1,     1,    22,    21,     1,    20,     1,     1,     1,     1,
       1,    21,    23,     1,     6,    21,     4,     9,    21,    21,
      21,    21,     0,     7,    21,    20,    20,    20,   101,    20,
       0,     1,     1,     1,     1,     4,     4,     7,     5,     8,
      10,    11,    12,    13,    14,     1,    16,     1,     4,    21,
       4,    15,     8,     1,     8,    21,     4,    17,    18,    19,
       8,     8,     8,     4,    21,    24,    24,     3,     3,     1,
      57,    61,    53,   100,    70
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    27,    28,     0,     1,     7,    10,    11,    12,    13,
      14,    16,    29,    30,    38,    40,    49,     1,    21,     1,
      21,     1,    21,    21,     1,    21,     1,    21,    42,     7,
       1,    22,     1,    22,     1,    22,    20,     1,     4,     1,
      20,    25,    41,    17,    18,    19,    39,    43,    44,    45,
       1,     4,     8,    35,    36,    51,     1,    33,    34,    51,
       1,    31,    32,    51,     1,    20,    21,    21,    21,    15,
      46,    47,    50,     1,    23,    36,     1,    21,     1,    23,
      34,     1,    21,     1,    23,    32,     1,    21,     8,     8,
       4,     1,    52,    47,    53,     1,    20,     1,    20,     1,
      20,    21,     1,     6,    48,     1,    54,    24,    37,    37,
       1,     4,     8,    51,     1,     6,     9,    24,     1,     4,
       3,     3,     1,     1,     5
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    26,    27,    28,    28,    29,    29,    29,    29,    30,
      30,    30,    30,    30,    30,    30,    30,    30,    30,    30,
      30,    30,    30,    30,    30,    31,    31,    31,    32,    32,
      32,    33,    33,    33,    34,    34,    34,    35,    35,    35,
      36,    36,    36,    36,    36,    37,    37,    37,    39,    38,
      40,    40,    40,    41,    41,    41,    41,    42,    42,    42,
      42,    43,    44,    45,    46,    46,    47,    48,    48,    48,
      48,    49,    49,    50,    50,    50,    50,    50,    51,    51,
      52,    53,    54
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     0,     2,     1,     1,     1,     1,     5,
       5,     3,     2,     5,     5,     3,     2,     5,     5,     3,
       2,     3,     3,     3,     2,     1,     2,     1,     4,     3,
       2,     1,     2,     1,     4,     3,     2,     1,     2,     1,
       6,     6,     4,     3,     2,     0,     2,     2,     0,     4,
       3,     3,     2,     0,     1,     2,     2,     0,     2,     2,
       2,     3,     3,     3,     1,     2,     4,     1,     2,     1,
       2,     1,     2,     5,     5,     5,     4,     2,     1,     1,
       0,     0,     0
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
  case 7: /* entity: comments  */
#line 76 "mcparse.y"
          {
	    cur_node = mc_add_node ();
	    cur_node->user_text = (yyvsp[0].ustr);
	  }
#line 1332 "mcparse.c"
    break;

  case 8: /* entity: error  */
#line 80 "mcparse.y"
                { mc_fatal ("syntax error"); }
#line 1338 "mcparse.c"
    break;

  case 10: /* global_section: MCSEVERITYNAMES '=' '(' severitymaps error  */
#line 85 "mcparse.y"
                                                     { mc_fatal ("missing ')' in SeverityNames"); }
#line 1344 "mcparse.c"
    break;

  case 11: /* global_section: MCSEVERITYNAMES '=' error  */
#line 86 "mcparse.y"
                                    { mc_fatal ("missing '(' in SeverityNames"); }
#line 1350 "mcparse.c"
    break;

  case 12: /* global_section: MCSEVERITYNAMES error  */
#line 87 "mcparse.y"
                                { mc_fatal ("missing '=' for SeverityNames"); }
#line 1356 "mcparse.c"
    break;

  case 14: /* global_section: MCLANGUAGENAMES '=' '(' langmaps error  */
#line 89 "mcparse.y"
                                                 { mc_fatal ("missing ')' in LanguageNames"); }
#line 1362 "mcparse.c"
    break;

  case 15: /* global_section: MCLANGUAGENAMES '=' error  */
#line 90 "mcparse.y"
                                    { mc_fatal ("missing '(' in LanguageNames"); }
#line 1368 "mcparse.c"
    break;

  case 16: /* global_section: MCLANGUAGENAMES error  */
#line 91 "mcparse.y"
                                { mc_fatal ("missing '=' for LanguageNames"); }
#line 1374 "mcparse.c"
    break;

  case 18: /* global_section: MCFACILITYNAMES '=' '(' facilitymaps error  */
#line 93 "mcparse.y"
                                                     { mc_fatal ("missing ')' in FacilityNames"); }
#line 1380 "mcparse.c"
    break;

  case 19: /* global_section: MCFACILITYNAMES '=' error  */
#line 94 "mcparse.y"
                                    { mc_fatal ("missing '(' in FacilityNames"); }
#line 1386 "mcparse.c"
    break;

  case 20: /* global_section: MCFACILITYNAMES error  */
#line 95 "mcparse.y"
                                { mc_fatal ("missing '=' for FacilityNames"); }
#line 1392 "mcparse.c"
    break;

  case 21: /* global_section: MCOUTPUTBASE '=' MCNUMBER  */
#line 97 "mcparse.y"
          {
	    if ((yyvsp[0].ival) != 10 && (yyvsp[0].ival) != 16)
	      mc_fatal ("OutputBase allows 10 or 16 as value");
	    mcset_out_values_are_decimal = ((yyvsp[0].ival) == 10 ? 1 : 0);
	  }
#line 1402 "mcparse.c"
    break;

  case 22: /* global_section: MCMESSAGEIDTYPEDEF '=' MCIDENT  */
#line 103 "mcparse.y"
          {
	    mcset_msg_id_typedef = (yyvsp[0].ustr);
	  }
#line 1410 "mcparse.c"
    break;

  case 23: /* global_section: MCMESSAGEIDTYPEDEF '=' error  */
#line 107 "mcparse.y"
          {
	    mc_fatal ("MessageIdTypedef expects an identifier");
	  }
#line 1418 "mcparse.c"
    break;

  case 24: /* global_section: MCMESSAGEIDTYPEDEF error  */
#line 111 "mcparse.y"
          {
	    mc_fatal ("missing '=' for MessageIdTypedef");
	  }
#line 1426 "mcparse.c"
    break;

  case 27: /* severitymaps: error  */
#line 119 "mcparse.y"
                { mc_fatal ("severity ident missing"); }
#line 1432 "mcparse.c"
    break;

  case 28: /* severitymap: token '=' MCNUMBER alias_name  */
#line 124 "mcparse.y"
          {
	    mc_add_keyword ((yyvsp[-3].ustr), MCTOKEN, "severity", (yyvsp[-1].ival), (yyvsp[0].ustr));
	  }
#line 1440 "mcparse.c"
    break;

  case 29: /* severitymap: token '=' error  */
#line 127 "mcparse.y"
                          { mc_fatal ("severity number missing"); }
#line 1446 "mcparse.c"
    break;

  case 30: /* severitymap: token error  */
#line 128 "mcparse.y"
                      { mc_fatal ("severity missing '='"); }
#line 1452 "mcparse.c"
    break;

  case 33: /* facilitymaps: error  */
#line 134 "mcparse.y"
                { mc_fatal ("missing ident in FacilityNames"); }
#line 1458 "mcparse.c"
    break;

  case 34: /* facilitymap: token '=' MCNUMBER alias_name  */
#line 139 "mcparse.y"
          {
	    mc_add_keyword ((yyvsp[-3].ustr), MCTOKEN, "facility", (yyvsp[-1].ival), (yyvsp[0].ustr));
	  }
#line 1466 "mcparse.c"
    break;

  case 35: /* facilitymap: token '=' error  */
#line 142 "mcparse.y"
                          { mc_fatal ("facility number missing"); }
#line 1472 "mcparse.c"
    break;

  case 36: /* facilitymap: token error  */
#line 143 "mcparse.y"
                      { mc_fatal ("facility missing '='"); }
#line 1478 "mcparse.c"
    break;

  case 39: /* langmaps: error  */
#line 149 "mcparse.y"
                { mc_fatal ("missing ident in LanguageNames"); }
#line 1484 "mcparse.c"
    break;

  case 40: /* langmap: token '=' MCNUMBER lex_want_filename ':' MCFILENAME  */
#line 154 "mcparse.y"
          {
	    mc_add_keyword ((yyvsp[-5].ustr), MCTOKEN, "language", (yyvsp[-3].ival), (yyvsp[0].ustr));
	  }
#line 1492 "mcparse.c"
    break;

  case 41: /* langmap: token '=' MCNUMBER lex_want_filename ':' error  */
#line 157 "mcparse.y"
                                                         { mc_fatal ("missing filename in LanguageNames"); }
#line 1498 "mcparse.c"
    break;

  case 42: /* langmap: token '=' MCNUMBER error  */
#line 158 "mcparse.y"
                                   { mc_fatal ("missing ':' in LanguageNames"); }
#line 1504 "mcparse.c"
    break;

  case 43: /* langmap: token '=' error  */
#line 159 "mcparse.y"
                          { mc_fatal ("missing language code in LanguageNames"); }
#line 1510 "mcparse.c"
    break;

  case 44: /* langmap: token error  */
#line 160 "mcparse.y"
                      { mc_fatal ("missing '=' for LanguageNames"); }
#line 1516 "mcparse.c"
    break;

  case 45: /* alias_name: %empty  */
#line 165 "mcparse.y"
          {
	    (yyval.ustr) = NULL;
	  }
#line 1524 "mcparse.c"
    break;

  case 46: /* alias_name: ':' MCIDENT  */
#line 169 "mcparse.y"
          {
	    (yyval.ustr) = (yyvsp[0].ustr);
	  }
#line 1532 "mcparse.c"
    break;

  case 47: /* alias_name: ':' error  */
#line 172 "mcparse.y"
                    { mc_fatal ("illegal token in identifier"); (yyval.ustr) = NULL; }
#line 1538 "mcparse.c"
    break;

  case 48: /* $@1: %empty  */
#line 177 "mcparse.y"
          {
	    cur_node = mc_add_node ();
	    cur_node->symbol = mc_last_symbol;
	    cur_node->facility = mc_cur_facility;
	    cur_node->severity = mc_cur_severity;
	    cur_node->id = ((yyvsp[-1].ival) & 0xffffUL);
	    cur_node->vid = ((yyvsp[-1].ival) & 0xffffUL) | mc_sefa_val;
	    cur_node->id_typecast = mcset_msg_id_typedef;
	    mc_last_id = (yyvsp[-1].ival);
	  }
#line 1553 "mcparse.c"
    break;

  case 50: /* id: MCMESSAGEID '=' vid  */
#line 190 "mcparse.y"
                              { (yyval.ival) = (yyvsp[0].ival); }
#line 1559 "mcparse.c"
    break;

  case 51: /* id: MCMESSAGEID '=' error  */
#line 191 "mcparse.y"
                                { mc_fatal ("missing number in MessageId"); (yyval.ival) = 0; }
#line 1565 "mcparse.c"
    break;

  case 52: /* id: MCMESSAGEID error  */
#line 192 "mcparse.y"
                            { mc_fatal ("missing '=' for MessageId"); (yyval.ival) = 0; }
#line 1571 "mcparse.c"
    break;

  case 53: /* vid: %empty  */
#line 196 "mcparse.y"
          {
	    (yyval.ival) = ++mc_last_id;
	  }
#line 1579 "mcparse.c"
    break;

  case 54: /* vid: MCNUMBER  */
#line 200 "mcparse.y"
          {
	    (yyval.ival) = (yyvsp[0].ival);
	  }
#line 1587 "mcparse.c"
    break;

  case 55: /* vid: '+' MCNUMBER  */
#line 204 "mcparse.y"
          {
	    (yyval.ival) = mc_last_id + (yyvsp[0].ival);
	  }
#line 1595 "mcparse.c"
    break;

  case 56: /* vid: '+' error  */
#line 207 "mcparse.y"
                    { mc_fatal ("missing number after MessageId '+'"); }
#line 1601 "mcparse.c"
    break;

  case 57: /* sefasy_def: %empty  */
#line 212 "mcparse.y"
          {
	    (yyval.ival) = 0;
	    mc_sefa_val = (mcset_custom_bit ? 1 : 0) << 29;
	    mc_last_symbol = NULL;
	    mc_cur_severity = NULL;
	    mc_cur_facility = NULL;
	  }
#line 1613 "mcparse.c"
    break;

  case 58: /* sefasy_def: sefasy_def severity  */
#line 220 "mcparse.y"
          {
	    if ((yyvsp[-1].ival) & 1)
	      mc_warn (_("duplicate definition of Severity"));
	    (yyval.ival) = (yyvsp[-1].ival) | 1;
	  }
#line 1623 "mcparse.c"
    break;

  case 59: /* sefasy_def: sefasy_def facility  */
#line 226 "mcparse.y"
          {
	    if ((yyvsp[-1].ival) & 2)
	      mc_warn (_("duplicate definition of Facility"));
	    (yyval.ival) = (yyvsp[-1].ival) | 2;
	  }
#line 1633 "mcparse.c"
    break;

  case 60: /* sefasy_def: sefasy_def symbol  */
#line 232 "mcparse.y"
          {
	    if ((yyvsp[-1].ival) & 4)
	      mc_warn (_("duplicate definition of SymbolicName"));
	    (yyval.ival) = (yyvsp[-1].ival) | 4;
	  }
#line 1643 "mcparse.c"
    break;

  case 61: /* severity: MCSEVERITY '=' MCTOKEN  */
#line 240 "mcparse.y"
          {
	    mc_sefa_val &= ~ (0x3UL << 30);
	    mc_sefa_val |= (((yyvsp[0].tok)->nval & 0x3UL) << 30);
	    mc_cur_severity = (yyvsp[0].tok);
	  }
#line 1653 "mcparse.c"
    break;

  case 62: /* facility: MCFACILITY '=' MCTOKEN  */
#line 248 "mcparse.y"
          {
	    mc_sefa_val &= ~ (0xfffUL << 16);
	    mc_sefa_val |= (((yyvsp[0].tok)->nval & 0xfffUL) << 16);
	    mc_cur_facility = (yyvsp[0].tok);
	  }
#line 1663 "mcparse.c"
    break;

  case 63: /* symbol: MCSYMBOLICNAME '=' MCIDENT  */
#line 256 "mcparse.y"
        {
	  mc_last_symbol = (yyvsp[0].ustr);
	}
#line 1671 "mcparse.c"
    break;

  case 66: /* lang_entity: lang lex_want_line lines MCENDLINE  */
#line 268 "mcparse.y"
          {
	    mc_node_lang *h;
	    h = mc_add_node_lang (cur_node, (yyvsp[-3].tok), cur_node->vid);
	    h->message = (yyvsp[-1].ustr);
	    if (mcset_max_message_length != 0 && unichar_len (h->message) > mcset_max_message_length)
	      mc_warn ("message length to long");
	  }
#line 1683 "mcparse.c"
    break;

  case 67: /* lines: MCLINE  */
#line 278 "mcparse.y"
          {
	    (yyval.ustr) = (yyvsp[0].ustr);
	  }
#line 1691 "mcparse.c"
    break;

  case 68: /* lines: lines MCLINE  */
#line 282 "mcparse.y"
          {
	    unichar *h;
	    rc_uint_type l1,l2;
	    l1 = unichar_len ((yyvsp[-1].ustr));
	    l2 = unichar_len ((yyvsp[0].ustr));
	    h = (unichar *) res_alloc ((l1 + l2 + 1) * sizeof (unichar));
	    if (l1) memcpy (h, (yyvsp[-1].ustr), l1 * sizeof (unichar));
	    if (l2) memcpy (&h[l1], (yyvsp[0].ustr), l2 * sizeof (unichar));
	    h[l1 + l2] = 0;
	    (yyval.ustr) = h;
	  }
#line 1707 "mcparse.c"
    break;

  case 69: /* lines: error  */
#line 293 "mcparse.y"
                { mc_fatal ("missing end of message text"); (yyval.ustr) = NULL; }
#line 1713 "mcparse.c"
    break;

  case 70: /* lines: lines error  */
#line 294 "mcparse.y"
                      { mc_fatal ("missing end of message text"); (yyval.ustr) = (yyvsp[-1].ustr); }
#line 1719 "mcparse.c"
    break;

  case 71: /* comments: MCCOMMENT  */
#line 297 "mcparse.y"
                    { (yyval.ustr) = (yyvsp[0].ustr); }
#line 1725 "mcparse.c"
    break;

  case 72: /* comments: comments MCCOMMENT  */
#line 299 "mcparse.y"
          {
	    unichar *h;
	    rc_uint_type l1,l2;
	    l1 = unichar_len ((yyvsp[-1].ustr));
	    l2 = unichar_len ((yyvsp[0].ustr));
	    h = (unichar *) res_alloc ((l1 + l2 + 1) * sizeof (unichar));
	    if (l1) memcpy (h, (yyvsp[-1].ustr), l1 * sizeof (unichar));
	    if (l2) memcpy (&h[l1], (yyvsp[0].ustr), l2 * sizeof (unichar));
	    h[l1 + l2] = 0;
	    (yyval.ustr) = h;
	  }
#line 1741 "mcparse.c"
    break;

  case 73: /* lang: MCLANGUAGE lex_want_nl '=' MCTOKEN NL  */
#line 313 "mcparse.y"
          {
	    (yyval.tok) = (yyvsp[-1].tok);
	  }
#line 1749 "mcparse.c"
    break;

  case 74: /* lang: MCLANGUAGE lex_want_nl '=' MCIDENT NL  */
#line 317 "mcparse.y"
          {
	    (yyval.tok) = NULL;
	    mc_fatal (_("undeclared language identifier"));
	  }
#line 1758 "mcparse.c"
    break;

  case 75: /* lang: MCLANGUAGE lex_want_nl '=' token error  */
#line 322 "mcparse.y"
          {
	    (yyval.tok) = NULL;
	    mc_fatal ("missing newline after Language");
	  }
#line 1767 "mcparse.c"
    break;

  case 76: /* lang: MCLANGUAGE lex_want_nl '=' error  */
#line 327 "mcparse.y"
          {
	    (yyval.tok) = NULL;
	    mc_fatal ("missing ident for Language");
	  }
#line 1776 "mcparse.c"
    break;

  case 77: /* lang: MCLANGUAGE error  */
#line 332 "mcparse.y"
          {
	    (yyval.tok) = NULL;
	    mc_fatal ("missing '=' for Language");
	  }
#line 1785 "mcparse.c"
    break;

  case 78: /* token: MCIDENT  */
#line 338 "mcparse.y"
                { (yyval.ustr) = (yyvsp[0].ustr); }
#line 1791 "mcparse.c"
    break;

  case 79: /* token: MCTOKEN  */
#line 339 "mcparse.y"
                   { (yyval.ustr) = (yyvsp[0].tok)->usz; }
#line 1797 "mcparse.c"
    break;

  case 80: /* lex_want_nl: %empty  */
#line 343 "mcparse.y"
                        { mclex_want_nl = 1; }
#line 1803 "mcparse.c"
    break;

  case 81: /* lex_want_line: %empty  */
#line 347 "mcparse.y"
                        { mclex_want_line = 1; }
#line 1809 "mcparse.c"
    break;

  case 82: /* lex_want_filename: %empty  */
#line 351 "mcparse.y"
                        { mclex_want_filename = 1; }
#line 1815 "mcparse.c"
    break;


#line 1819 "mcparse.c"

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

#line 354 "mcparse.y"


/* Something else.  */
