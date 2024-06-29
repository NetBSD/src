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
#line 1 "defparse.y"
 /* defparse.y - parser for .def files */

/* Copyright (C) 1995-2022 Free Software Foundation, Inc.

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
#include "bfd.h"
#include "libiberty.h"
#include "dlltool.h"

#line 98 "defparse.c"

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
#ifndef YY_YY_DEFPARSE_H_INCLUDED
# define YY_YY_DEFPARSE_H_INCLUDED
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
    STACKSIZE = 261,               /* STACKSIZE  */
    HEAPSIZE = 262,                /* HEAPSIZE  */
    CODE = 263,                    /* CODE  */
    DATA = 264,                    /* DATA  */
    SECTIONS = 265,                /* SECTIONS  */
    EXPORTS = 266,                 /* EXPORTS  */
    IMPORTS = 267,                 /* IMPORTS  */
    VERSIONK = 268,                /* VERSIONK  */
    BASE = 269,                    /* BASE  */
    CONSTANT = 270,                /* CONSTANT  */
    READ = 271,                    /* READ  */
    WRITE = 272,                   /* WRITE  */
    EXECUTE = 273,                 /* EXECUTE  */
    SHARED = 274,                  /* SHARED  */
    NONSHARED = 275,               /* NONSHARED  */
    NONAME = 276,                  /* NONAME  */
    PRIVATE = 277,                 /* PRIVATE  */
    SINGLE = 278,                  /* SINGLE  */
    MULTIPLE = 279,                /* MULTIPLE  */
    INITINSTANCE = 280,            /* INITINSTANCE  */
    INITGLOBAL = 281,              /* INITGLOBAL  */
    TERMINSTANCE = 282,            /* TERMINSTANCE  */
    TERMGLOBAL = 283,              /* TERMGLOBAL  */
    EQUAL = 284,                   /* EQUAL  */
    ID = 285,                      /* ID  */
    NUMBER = 286                   /* NUMBER  */
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
#define STACKSIZE 261
#define HEAPSIZE 262
#define CODE 263
#define DATA 264
#define SECTIONS 265
#define EXPORTS 266
#define IMPORTS 267
#define VERSIONK 268
#define BASE 269
#define CONSTANT 270
#define READ 271
#define WRITE 272
#define EXECUTE 273
#define SHARED 274
#define NONSHARED 275
#define NONAME 276
#define PRIVATE 277
#define SINGLE 278
#define MULTIPLE 279
#define INITINSTANCE 280
#define INITGLOBAL 281
#define TERMINSTANCE 282
#define TERMGLOBAL 283
#define EQUAL 284
#define ID 285
#define NUMBER 286

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 28 "defparse.y"

  char *id;
  const char *id_const;
  int number;

#line 219 "defparse.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_DEFPARSE_H_INCLUDED  */
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
  YYSYMBOL_STACKSIZE = 6,                  /* STACKSIZE  */
  YYSYMBOL_HEAPSIZE = 7,                   /* HEAPSIZE  */
  YYSYMBOL_CODE = 8,                       /* CODE  */
  YYSYMBOL_DATA = 9,                       /* DATA  */
  YYSYMBOL_SECTIONS = 10,                  /* SECTIONS  */
  YYSYMBOL_EXPORTS = 11,                   /* EXPORTS  */
  YYSYMBOL_IMPORTS = 12,                   /* IMPORTS  */
  YYSYMBOL_VERSIONK = 13,                  /* VERSIONK  */
  YYSYMBOL_BASE = 14,                      /* BASE  */
  YYSYMBOL_CONSTANT = 15,                  /* CONSTANT  */
  YYSYMBOL_READ = 16,                      /* READ  */
  YYSYMBOL_WRITE = 17,                     /* WRITE  */
  YYSYMBOL_EXECUTE = 18,                   /* EXECUTE  */
  YYSYMBOL_SHARED = 19,                    /* SHARED  */
  YYSYMBOL_NONSHARED = 20,                 /* NONSHARED  */
  YYSYMBOL_NONAME = 21,                    /* NONAME  */
  YYSYMBOL_PRIVATE = 22,                   /* PRIVATE  */
  YYSYMBOL_SINGLE = 23,                    /* SINGLE  */
  YYSYMBOL_MULTIPLE = 24,                  /* MULTIPLE  */
  YYSYMBOL_INITINSTANCE = 25,              /* INITINSTANCE  */
  YYSYMBOL_INITGLOBAL = 26,                /* INITGLOBAL  */
  YYSYMBOL_TERMINSTANCE = 27,              /* TERMINSTANCE  */
  YYSYMBOL_TERMGLOBAL = 28,                /* TERMGLOBAL  */
  YYSYMBOL_EQUAL = 29,                     /* EQUAL  */
  YYSYMBOL_ID = 30,                        /* ID  */
  YYSYMBOL_NUMBER = 31,                    /* NUMBER  */
  YYSYMBOL_32_ = 32,                       /* '.'  */
  YYSYMBOL_33_ = 33,                       /* '='  */
  YYSYMBOL_34_ = 34,                       /* ','  */
  YYSYMBOL_35_ = 35,                       /* '@'  */
  YYSYMBOL_YYACCEPT = 36,                  /* $accept  */
  YYSYMBOL_start = 37,                     /* start  */
  YYSYMBOL_command = 38,                   /* command  */
  YYSYMBOL_explist = 39,                   /* explist  */
  YYSYMBOL_expline = 40,                   /* expline  */
  YYSYMBOL_implist = 41,                   /* implist  */
  YYSYMBOL_impline = 42,                   /* impline  */
  YYSYMBOL_seclist = 43,                   /* seclist  */
  YYSYMBOL_secline = 44,                   /* secline  */
  YYSYMBOL_attr_list = 45,                 /* attr_list  */
  YYSYMBOL_opt_comma = 46,                 /* opt_comma  */
  YYSYMBOL_opt_number = 47,                /* opt_number  */
  YYSYMBOL_attr = 48,                      /* attr  */
  YYSYMBOL_opt_CONSTANT = 49,              /* opt_CONSTANT  */
  YYSYMBOL_opt_NONAME = 50,                /* opt_NONAME  */
  YYSYMBOL_opt_DATA = 51,                  /* opt_DATA  */
  YYSYMBOL_opt_PRIVATE = 52,               /* opt_PRIVATE  */
  YYSYMBOL_keyword_as_name = 53,           /* keyword_as_name  */
  YYSYMBOL_opt_name2 = 54,                 /* opt_name2  */
  YYSYMBOL_opt_name = 55,                  /* opt_name  */
  YYSYMBOL_opt_ordinal = 56,               /* opt_ordinal  */
  YYSYMBOL_opt_import_name = 57,           /* opt_import_name  */
  YYSYMBOL_opt_equal_name = 58,            /* opt_equal_name  */
  YYSYMBOL_opt_base = 59,                  /* opt_base  */
  YYSYMBOL_option_list = 60,               /* option_list  */
  YYSYMBOL_option = 61                     /* option  */
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
#define YYFINAL  66
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   141

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  36
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  26
/* YYNRULES -- Number of rules.  */
#define YYNRULES  98
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  139

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
       2,     2,     2,     2,    34,     2,    32,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    33,     2,     2,    35,     2,     2,     2,     2,     2,
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
static const yytype_uint8 yyrline[] =
{
       0,    48,    48,    49,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    68,    70,    74,    79,
      80,    84,    86,    88,    90,    92,    94,    96,    98,   103,
     104,   108,   112,   113,   117,   118,   120,   121,   125,   126,
     127,   128,   129,   130,   131,   135,   136,   140,   141,   145,
     146,   150,   151,   154,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   182,   185,   186,
     192,   198,   204,   211,   212,   216,   217,   221,   222,   226,
     227,   230,   231,   234,   236,   240,   241,   242,   243
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
  "DESCRIPTION", "STACKSIZE", "HEAPSIZE", "CODE", "DATA", "SECTIONS",
  "EXPORTS", "IMPORTS", "VERSIONK", "BASE", "CONSTANT", "READ", "WRITE",
  "EXECUTE", "SHARED", "NONSHARED", "NONAME", "PRIVATE", "SINGLE",
  "MULTIPLE", "INITINSTANCE", "INITGLOBAL", "TERMINSTANCE", "TERMGLOBAL",
  "EQUAL", "ID", "NUMBER", "'.'", "'='", "','", "'@'", "$accept", "start",
  "command", "explist", "expline", "implist", "impline", "seclist",
  "secline", "attr_list", "opt_comma", "opt_number", "attr",
  "opt_CONSTANT", "opt_NONAME", "opt_DATA", "opt_PRIVATE",
  "keyword_as_name", "opt_name2", "opt_name", "opt_ordinal",
  "opt_import_name", "opt_equal_name", "opt_base", "option_list", "option", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-96)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-36)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int8 yypact[] =
{
      38,    61,    61,   -22,    -1,     8,    39,    39,    -7,   -96,
      23,    59,    92,   -96,   -96,   -96,   -96,   -96,   -96,   -96,
     -96,   -96,   -96,   -96,   -96,   -96,   -96,   -96,   -96,   -96,
     -96,   -96,   -96,   -96,   -96,   -96,   -96,   -96,   -96,    62,
      61,    79,   -96,    96,    96,   -96,    80,    80,   -96,   -96,
     -96,   -96,   -96,   -96,   -96,   -13,   -96,   -13,    39,    -7,
     -96,    82,     1,    23,   -96,    81,   -96,   -96,    61,    79,
     -96,    61,    83,   -96,   -96,    84,   -96,   -96,   -96,    39,
     -13,   -96,    85,   -96,     5,    87,   -96,    88,   -96,   -96,
      89,   -12,   -96,   -96,    61,    86,   -20,    93,    91,   -96,
     -96,    -8,   -96,    94,   103,    61,    30,   -96,   -96,    76,
     -96,   -96,   -96,   -96,   -96,   -96,   -96,   111,   -96,    93,
      93,     0,    93,   -96,   118,   -96,   -96,    78,   -96,   -96,
     -96,   106,    93,    93,   -96,    93,   -96,   -96,   -96
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,    84,    84,     0,     0,     0,     0,     0,     0,    16,
       0,     0,     0,     3,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    67,    68,    69,    70,
      71,    65,    66,    72,    73,    74,    75,    76,    77,    78,
       0,     0,    83,    92,    92,     7,    37,    37,    38,    39,
      40,    41,    42,    43,    44,    10,    33,    11,     0,    12,
      30,     6,     0,    13,    20,    14,     1,     2,     0,    79,
      80,     0,     0,     4,    93,     0,     8,     9,    34,     0,
      31,    29,    90,    17,     0,     0,    19,     0,    82,    81,
       0,     5,    36,    32,     0,    86,    88,    88,     0,    15,
      91,     0,    89,     0,    48,     0,     0,    27,    28,     0,
      95,    96,    97,    98,    94,    85,    47,    46,    87,    88,
      88,    88,    88,    45,    50,    25,    26,     0,    23,    24,
      49,    52,    88,    88,    51,    88,    21,    22,    18
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -96,   -96,   117,   -96,   -96,   -96,    67,   -96,    72,    -6,
      41,    90,    54,   -96,   -96,   -96,   -96,    95,   -40,   132,
     -96,   -95,   -96,    97,   -96,   -96
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,    12,    13,    61,    83,    63,    64,    59,    60,    55,
      79,    76,    56,   124,   117,   131,   135,    41,    42,    43,
     104,   107,    95,    73,    91,   114
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      70,    57,   108,   -35,   -35,   -35,   -35,   -35,    45,   105,
     -35,   -35,   106,   -35,   -35,   -35,   -35,   110,   111,   112,
     113,    78,    78,    58,   125,   126,   128,   129,    88,   105,
      46,    89,   127,    84,    85,    96,    97,   136,   137,    47,
     138,     1,     2,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    80,    62,   102,    48,    49,    50,    51,    52,
     119,   120,    53,    54,    14,   118,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      65,    39,    66,    40,    68,     1,     2,     3,     4,     5,
       6,     7,     8,     9,    10,    11,   121,   122,   132,   133,
      72,    71,    82,    87,    75,    92,    90,    98,    94,    99,
     100,   103,   105,   109,   116,   115,   123,   130,   134,    67,
      86,    81,   101,    93,    44,    69,     0,    77,     0,     0,
       0,    74
};

static const yytype_int16 yycheck[] =
{
      40,     7,    97,    16,    17,    18,    19,    20,    30,    29,
      23,    24,    32,    25,    26,    27,    28,    25,    26,    27,
      28,    34,    34,    30,   119,   120,   121,   122,    68,    29,
      31,    71,    32,    32,    33,    30,    31,   132,   133,    31,
     135,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    58,    30,    94,    16,    17,    18,    19,    20,
      30,    31,    23,    24,     3,   105,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      31,    30,     0,    32,    32,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    30,    31,    30,    31,
      14,    32,    30,    32,    34,    31,    33,    30,    33,    31,
      31,    35,    29,    32,    21,    31,    15,     9,    22,    12,
      63,    59,    91,    79,     2,    40,    -1,    47,    -1,    -1,
      -1,    44
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    37,    38,     3,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    30,
      32,    53,    54,    55,    55,    30,    31,    31,    16,    17,
      18,    19,    20,    23,    24,    45,    48,    45,    30,    43,
      44,    39,    30,    41,    42,    31,     0,    38,    32,    53,
      54,    32,    14,    59,    59,    34,    47,    47,    34,    46,
      45,    44,    30,    40,    32,    33,    42,    32,    54,    54,
      33,    60,    31,    48,    33,    58,    30,    31,    30,    31,
      31,    46,    54,    35,    56,    29,    32,    57,    57,    32,
      25,    26,    27,    28,    61,    31,    21,    50,    54,    30,
      31,    30,    31,    15,    49,    57,    57,    32,    57,    57,
       9,    51,    30,    31,    22,    52,    57,    57,    57
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    36,    37,    37,    38,    38,    38,    38,    38,    38,
      38,    38,    38,    38,    38,    38,    39,    39,    40,    41,
      41,    42,    42,    42,    42,    42,    42,    42,    42,    43,
      43,    44,    45,    45,    46,    46,    47,    47,    48,    48,
      48,    48,    48,    48,    48,    49,    49,    50,    50,    51,
      51,    52,    52,    53,    53,    53,    53,    53,    53,    53,
      53,    53,    53,    53,    53,    53,    53,    53,    53,    53,
      53,    53,    53,    53,    53,    53,    53,    53,    54,    54,
      54,    54,    54,    55,    55,    56,    56,    57,    57,    58,
      58,    59,    59,    60,    60,    61,    61,    61,    61
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     1,     3,     4,     2,     2,     3,     3,
       2,     2,     2,     2,     2,     4,     0,     2,     8,     2,
       1,     8,     8,     6,     6,     6,     6,     4,     4,     2,
       1,     2,     3,     1,     1,     0,     2,     0,     1,     1,
       1,     1,     1,     1,     1,     1,     0,     1,     0,     1,
       0,     1,     0,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       2,     3,     3,     1,     0,     2,     0,     2,     0,     2,
       0,     3,     0,     0,     3,     1,     1,     1,     1
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
#line 53 "defparse.y"
                                       { def_name ((yyvsp[-1].id), (yyvsp[0].number)); }
#line 1355 "defparse.c"
    break;

  case 5: /* command: LIBRARY opt_name opt_base option_list  */
#line 54 "defparse.y"
                                                      { def_library ((yyvsp[-2].id), (yyvsp[-1].number)); }
#line 1361 "defparse.c"
    break;

  case 7: /* command: DESCRIPTION ID  */
#line 56 "defparse.y"
                               { def_description ((yyvsp[0].id));}
#line 1367 "defparse.c"
    break;

  case 8: /* command: STACKSIZE NUMBER opt_number  */
#line 57 "defparse.y"
                                            { def_stacksize ((yyvsp[-1].number), (yyvsp[0].number));}
#line 1373 "defparse.c"
    break;

  case 9: /* command: HEAPSIZE NUMBER opt_number  */
#line 58 "defparse.y"
                                           { def_heapsize ((yyvsp[-1].number), (yyvsp[0].number));}
#line 1379 "defparse.c"
    break;

  case 10: /* command: CODE attr_list  */
#line 59 "defparse.y"
                               { def_code ((yyvsp[0].number));}
#line 1385 "defparse.c"
    break;

  case 11: /* command: DATA attr_list  */
#line 60 "defparse.y"
                                { def_data ((yyvsp[0].number));}
#line 1391 "defparse.c"
    break;

  case 14: /* command: VERSIONK NUMBER  */
#line 63 "defparse.y"
                                { def_version ((yyvsp[0].number),0);}
#line 1397 "defparse.c"
    break;

  case 15: /* command: VERSIONK NUMBER '.' NUMBER  */
#line 64 "defparse.y"
                                           { def_version ((yyvsp[-2].number),(yyvsp[0].number));}
#line 1403 "defparse.c"
    break;

  case 18: /* expline: ID opt_equal_name opt_ordinal opt_NONAME opt_CONSTANT opt_DATA opt_PRIVATE opt_import_name  */
#line 76 "defparse.y"
                        { def_exports ((yyvsp[-7].id), (yyvsp[-6].id), (yyvsp[-5].number), (yyvsp[-4].number), (yyvsp[-3].number), (yyvsp[-2].number), (yyvsp[-1].number), (yyvsp[0].id));}
#line 1409 "defparse.c"
    break;

  case 21: /* impline: ID '=' ID '.' ID '.' ID opt_import_name  */
#line 85 "defparse.y"
                 { def_import ((yyvsp[-7].id),(yyvsp[-5].id),(yyvsp[-3].id),(yyvsp[-1].id), 0, (yyvsp[0].id)); }
#line 1415 "defparse.c"
    break;

  case 22: /* impline: ID '=' ID '.' ID '.' NUMBER opt_import_name  */
#line 87 "defparse.y"
                 { def_import ((yyvsp[-7].id),(yyvsp[-5].id),(yyvsp[-3].id), 0,(yyvsp[-1].number), (yyvsp[0].id)); }
#line 1421 "defparse.c"
    break;

  case 23: /* impline: ID '=' ID '.' ID opt_import_name  */
#line 89 "defparse.y"
                 { def_import ((yyvsp[-5].id),(yyvsp[-3].id), 0,(yyvsp[-1].id), 0, (yyvsp[0].id)); }
#line 1427 "defparse.c"
    break;

  case 24: /* impline: ID '=' ID '.' NUMBER opt_import_name  */
#line 91 "defparse.y"
                 { def_import ((yyvsp[-5].id),(yyvsp[-3].id), 0, 0,(yyvsp[-1].number), (yyvsp[0].id)); }
#line 1433 "defparse.c"
    break;

  case 25: /* impline: ID '.' ID '.' ID opt_import_name  */
#line 93 "defparse.y"
                 { def_import ( 0,(yyvsp[-5].id),(yyvsp[-3].id),(yyvsp[-1].id), 0, (yyvsp[0].id)); }
#line 1439 "defparse.c"
    break;

  case 26: /* impline: ID '.' ID '.' NUMBER opt_import_name  */
#line 95 "defparse.y"
                 { def_import ( 0,(yyvsp[-5].id),(yyvsp[-3].id), 0,(yyvsp[-1].number), (yyvsp[0].id)); }
#line 1445 "defparse.c"
    break;

  case 27: /* impline: ID '.' ID opt_import_name  */
#line 97 "defparse.y"
                 { def_import ( 0,(yyvsp[-3].id), 0,(yyvsp[-1].id), 0, (yyvsp[0].id)); }
#line 1451 "defparse.c"
    break;

  case 28: /* impline: ID '.' NUMBER opt_import_name  */
#line 99 "defparse.y"
                 { def_import ( 0,(yyvsp[-3].id), 0, 0,(yyvsp[-1].number), (yyvsp[0].id)); }
#line 1457 "defparse.c"
    break;

  case 31: /* secline: ID attr_list  */
#line 108 "defparse.y"
                     { def_section ((yyvsp[-1].id),(yyvsp[0].number));}
#line 1463 "defparse.c"
    break;

  case 36: /* opt_number: ',' NUMBER  */
#line 120 "defparse.y"
                       { (yyval.number)=(yyvsp[0].number);}
#line 1469 "defparse.c"
    break;

  case 37: /* opt_number: %empty  */
#line 121 "defparse.y"
                   { (yyval.number)=-1;}
#line 1475 "defparse.c"
    break;

  case 38: /* attr: READ  */
#line 125 "defparse.y"
                     { (yyval.number) = 1; }
#line 1481 "defparse.c"
    break;

  case 39: /* attr: WRITE  */
#line 126 "defparse.y"
                      { (yyval.number) = 2; }
#line 1487 "defparse.c"
    break;

  case 40: /* attr: EXECUTE  */
#line 127 "defparse.y"
                        { (yyval.number) = 4; }
#line 1493 "defparse.c"
    break;

  case 41: /* attr: SHARED  */
#line 128 "defparse.y"
                       { (yyval.number) = 8; }
#line 1499 "defparse.c"
    break;

  case 42: /* attr: NONSHARED  */
#line 129 "defparse.y"
                          { (yyval.number) = 0; }
#line 1505 "defparse.c"
    break;

  case 43: /* attr: SINGLE  */
#line 130 "defparse.y"
                       { (yyval.number) = 0; }
#line 1511 "defparse.c"
    break;

  case 44: /* attr: MULTIPLE  */
#line 131 "defparse.y"
                         { (yyval.number) = 0; }
#line 1517 "defparse.c"
    break;

  case 45: /* opt_CONSTANT: CONSTANT  */
#line 135 "defparse.y"
                         {(yyval.number)=1;}
#line 1523 "defparse.c"
    break;

  case 46: /* opt_CONSTANT: %empty  */
#line 136 "defparse.y"
                         {(yyval.number)=0;}
#line 1529 "defparse.c"
    break;

  case 47: /* opt_NONAME: NONAME  */
#line 140 "defparse.y"
                       {(yyval.number)=1;}
#line 1535 "defparse.c"
    break;

  case 48: /* opt_NONAME: %empty  */
#line 141 "defparse.y"
                         {(yyval.number)=0;}
#line 1541 "defparse.c"
    break;

  case 49: /* opt_DATA: DATA  */
#line 145 "defparse.y"
                     { (yyval.number) = 1; }
#line 1547 "defparse.c"
    break;

  case 50: /* opt_DATA: %empty  */
#line 146 "defparse.y"
                     { (yyval.number) = 0; }
#line 1553 "defparse.c"
    break;

  case 51: /* opt_PRIVATE: PRIVATE  */
#line 150 "defparse.y"
                        { (yyval.number) = 1; }
#line 1559 "defparse.c"
    break;

  case 52: /* opt_PRIVATE: %empty  */
#line 151 "defparse.y"
                        { (yyval.number) = 0; }
#line 1565 "defparse.c"
    break;

  case 53: /* keyword_as_name: NAME  */
#line 154 "defparse.y"
                      { (yyval.id_const) = "NAME"; }
#line 1571 "defparse.c"
    break;

  case 54: /* keyword_as_name: DESCRIPTION  */
#line 159 "defparse.y"
                      { (yyval.id_const) = "DESCRIPTION"; }
#line 1577 "defparse.c"
    break;

  case 55: /* keyword_as_name: STACKSIZE  */
#line 160 "defparse.y"
                    { (yyval.id_const) = "STACKSIZE"; }
#line 1583 "defparse.c"
    break;

  case 56: /* keyword_as_name: HEAPSIZE  */
#line 161 "defparse.y"
                   { (yyval.id_const) = "HEAPSIZE"; }
#line 1589 "defparse.c"
    break;

  case 57: /* keyword_as_name: CODE  */
#line 162 "defparse.y"
               { (yyval.id_const) = "CODE"; }
#line 1595 "defparse.c"
    break;

  case 58: /* keyword_as_name: DATA  */
#line 163 "defparse.y"
               { (yyval.id_const) = "DATA"; }
#line 1601 "defparse.c"
    break;

  case 59: /* keyword_as_name: SECTIONS  */
#line 164 "defparse.y"
                   { (yyval.id_const) = "SECTIONS"; }
#line 1607 "defparse.c"
    break;

  case 60: /* keyword_as_name: EXPORTS  */
#line 165 "defparse.y"
                  { (yyval.id_const) = "EXPORTS"; }
#line 1613 "defparse.c"
    break;

  case 61: /* keyword_as_name: IMPORTS  */
#line 166 "defparse.y"
                  { (yyval.id_const) = "IMPORTS"; }
#line 1619 "defparse.c"
    break;

  case 62: /* keyword_as_name: VERSIONK  */
#line 167 "defparse.y"
                   { (yyval.id_const) = "VERSION"; }
#line 1625 "defparse.c"
    break;

  case 63: /* keyword_as_name: BASE  */
#line 168 "defparse.y"
               { (yyval.id_const) = "BASE"; }
#line 1631 "defparse.c"
    break;

  case 64: /* keyword_as_name: CONSTANT  */
#line 169 "defparse.y"
                   { (yyval.id_const) = "CONSTANT"; }
#line 1637 "defparse.c"
    break;

  case 65: /* keyword_as_name: NONAME  */
#line 170 "defparse.y"
                 { (yyval.id_const) = "NONAME"; }
#line 1643 "defparse.c"
    break;

  case 66: /* keyword_as_name: PRIVATE  */
#line 171 "defparse.y"
                  { (yyval.id_const) = "PRIVATE"; }
#line 1649 "defparse.c"
    break;

  case 67: /* keyword_as_name: READ  */
#line 172 "defparse.y"
               { (yyval.id_const) = "READ"; }
#line 1655 "defparse.c"
    break;

  case 68: /* keyword_as_name: WRITE  */
#line 173 "defparse.y"
                { (yyval.id_const) = "WRITE"; }
#line 1661 "defparse.c"
    break;

  case 69: /* keyword_as_name: EXECUTE  */
#line 174 "defparse.y"
                  { (yyval.id_const) = "EXECUTE"; }
#line 1667 "defparse.c"
    break;

  case 70: /* keyword_as_name: SHARED  */
#line 175 "defparse.y"
                 { (yyval.id_const) = "SHARED"; }
#line 1673 "defparse.c"
    break;

  case 71: /* keyword_as_name: NONSHARED  */
#line 176 "defparse.y"
                    { (yyval.id_const) = "NONSHARED"; }
#line 1679 "defparse.c"
    break;

  case 72: /* keyword_as_name: SINGLE  */
#line 177 "defparse.y"
                 { (yyval.id_const) = "SINGLE"; }
#line 1685 "defparse.c"
    break;

  case 73: /* keyword_as_name: MULTIPLE  */
#line 178 "defparse.y"
                   { (yyval.id_const) = "MULTIPLE"; }
#line 1691 "defparse.c"
    break;

  case 74: /* keyword_as_name: INITINSTANCE  */
#line 179 "defparse.y"
                       { (yyval.id_const) = "INITINSTANCE"; }
#line 1697 "defparse.c"
    break;

  case 75: /* keyword_as_name: INITGLOBAL  */
#line 180 "defparse.y"
                     { (yyval.id_const) = "INITGLOBAL"; }
#line 1703 "defparse.c"
    break;

  case 76: /* keyword_as_name: TERMINSTANCE  */
#line 181 "defparse.y"
                       { (yyval.id_const) = "TERMINSTANCE"; }
#line 1709 "defparse.c"
    break;

  case 77: /* keyword_as_name: TERMGLOBAL  */
#line 182 "defparse.y"
                     { (yyval.id_const) = "TERMGLOBAL"; }
#line 1715 "defparse.c"
    break;

  case 78: /* opt_name2: ID  */
#line 185 "defparse.y"
              { (yyval.id) = (yyvsp[0].id); }
#line 1721 "defparse.c"
    break;

  case 79: /* opt_name2: '.' keyword_as_name  */
#line 187 "defparse.y"
          {
	    char *name = xmalloc (strlen ((yyvsp[0].id_const)) + 2);
	    sprintf (name, ".%s", (yyvsp[0].id_const));
	    (yyval.id) = name;
	  }
#line 1731 "defparse.c"
    break;

  case 80: /* opt_name2: '.' opt_name2  */
#line 193 "defparse.y"
          {
	    char *name = xmalloc (strlen ((yyvsp[0].id)) + 2);
	    sprintf (name, ".%s", (yyvsp[0].id));
	    (yyval.id) = name;
	  }
#line 1741 "defparse.c"
    break;

  case 81: /* opt_name2: keyword_as_name '.' opt_name2  */
#line 199 "defparse.y"
          {
	    char *name = xmalloc (strlen ((yyvsp[-2].id_const)) + 1 + strlen ((yyvsp[0].id)) + 1);
	    sprintf (name, "%s.%s", (yyvsp[-2].id_const), (yyvsp[0].id));
	    (yyval.id) = name;
	  }
#line 1751 "defparse.c"
    break;

  case 82: /* opt_name2: ID '.' opt_name2  */
#line 205 "defparse.y"
          {
	    char *name = xmalloc (strlen ((yyvsp[-2].id)) + 1 + strlen ((yyvsp[0].id)) + 1);
	    sprintf (name, "%s.%s", (yyvsp[-2].id), (yyvsp[0].id));
	    (yyval.id) = name;
	  }
#line 1761 "defparse.c"
    break;

  case 83: /* opt_name: opt_name2  */
#line 211 "defparse.y"
                    { (yyval.id) =(yyvsp[0].id); }
#line 1767 "defparse.c"
    break;

  case 84: /* opt_name: %empty  */
#line 212 "defparse.y"
                        { (yyval.id)=""; }
#line 1773 "defparse.c"
    break;

  case 85: /* opt_ordinal: '@' NUMBER  */
#line 216 "defparse.y"
                         { (yyval.number)=(yyvsp[0].number);}
#line 1779 "defparse.c"
    break;

  case 86: /* opt_ordinal: %empty  */
#line 217 "defparse.y"
                         { (yyval.number)=-1;}
#line 1785 "defparse.c"
    break;

  case 87: /* opt_import_name: EQUAL opt_name2  */
#line 221 "defparse.y"
                                { (yyval.id) = (yyvsp[0].id); }
#line 1791 "defparse.c"
    break;

  case 88: /* opt_import_name: %empty  */
#line 222 "defparse.y"
                        { (yyval.id) = 0; }
#line 1797 "defparse.c"
    break;

  case 89: /* opt_equal_name: '=' opt_name2  */
#line 226 "defparse.y"
                        { (yyval.id) = (yyvsp[0].id); }
#line 1803 "defparse.c"
    break;

  case 90: /* opt_equal_name: %empty  */
#line 227 "defparse.y"
                        { (yyval.id) =  0; }
#line 1809 "defparse.c"
    break;

  case 91: /* opt_base: BASE '=' NUMBER  */
#line 230 "defparse.y"
                                { (yyval.number)= (yyvsp[0].number);}
#line 1815 "defparse.c"
    break;

  case 92: /* opt_base: %empty  */
#line 231 "defparse.y"
                { (yyval.number)=-1;}
#line 1821 "defparse.c"
    break;


#line 1825 "defparse.c"

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

