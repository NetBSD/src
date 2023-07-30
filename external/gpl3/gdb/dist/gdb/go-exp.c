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
#line 52 "go-exp.y"


#include "defs.h"
#include <ctype.h>
#include "expression.h"
#include "value.h"
#include "parser-defs.h"
#include "language.h"
#include "c-lang.h"
#include "go-lang.h"
#include "bfd.h" /* Required by objfiles.h.  */
#include "symfile.h" /* Required by objfiles.h.  */
#include "objfiles.h" /* For have_full_symbols and have_partial_symbols */
#include "charset.h"
#include "block.h"
#include "expop.h"

#define parse_type(ps) builtin_type (ps->gdbarch ())

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror,
   etc).  */
#define GDB_YY_REMAP_PREFIX go_
#include "yy-remap.h"

/* The state of the parser, used internally when we are parsing the
   expression.  */

static struct parser_state *pstate = NULL;

int yyparse (void);

static int yylex (void);

static void yyerror (const char *);


#line 108 "go-exp.c.tmp"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTRPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTRPTR nullptr
#   else
#    define YY_NULLPTRPTR 0
#   endif
#  else
#   define YY_NULLPTRPTR ((void*)0)
#  endif
# endif


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
    INT = 258,                     /* INT  */
    FLOAT = 259,                   /* FLOAT  */
    RAW_STRING = 260,              /* RAW_STRING  */
    STRING = 261,                  /* STRING  */
    CHAR = 262,                    /* CHAR  */
    NAME = 263,                    /* NAME  */
    TYPENAME = 264,                /* TYPENAME  */
    COMPLETE = 265,                /* COMPLETE  */
    NAME_OR_INT = 266,             /* NAME_OR_INT  */
    TRUE_KEYWORD = 267,            /* TRUE_KEYWORD  */
    FALSE_KEYWORD = 268,           /* FALSE_KEYWORD  */
    STRUCT_KEYWORD = 269,          /* STRUCT_KEYWORD  */
    INTERFACE_KEYWORD = 270,       /* INTERFACE_KEYWORD  */
    TYPE_KEYWORD = 271,            /* TYPE_KEYWORD  */
    CHAN_KEYWORD = 272,            /* CHAN_KEYWORD  */
    SIZEOF_KEYWORD = 273,          /* SIZEOF_KEYWORD  */
    LEN_KEYWORD = 274,             /* LEN_KEYWORD  */
    CAP_KEYWORD = 275,             /* CAP_KEYWORD  */
    NEW_KEYWORD = 276,             /* NEW_KEYWORD  */
    IOTA_KEYWORD = 277,            /* IOTA_KEYWORD  */
    NIL_KEYWORD = 278,             /* NIL_KEYWORD  */
    CONST_KEYWORD = 279,           /* CONST_KEYWORD  */
    DOTDOTDOT = 280,               /* DOTDOTDOT  */
    ENTRY = 281,                   /* ENTRY  */
    ERROR = 282,                   /* ERROR  */
    BYTE_KEYWORD = 283,            /* BYTE_KEYWORD  */
    DOLLAR_VARIABLE = 284,         /* DOLLAR_VARIABLE  */
    ASSIGN_MODIFY = 285,           /* ASSIGN_MODIFY  */
    ABOVE_COMMA = 286,             /* ABOVE_COMMA  */
    OROR = 287,                    /* OROR  */
    ANDAND = 288,                  /* ANDAND  */
    ANDNOT = 289,                  /* ANDNOT  */
    EQUAL = 290,                   /* EQUAL  */
    NOTEQUAL = 291,                /* NOTEQUAL  */
    LEQ = 292,                     /* LEQ  */
    GEQ = 293,                     /* GEQ  */
    LSH = 294,                     /* LSH  */
    RSH = 295,                     /* RSH  */
    UNARY = 296,                   /* UNARY  */
    INCREMENT = 297,               /* INCREMENT  */
    DECREMENT = 298,               /* DECREMENT  */
    LEFT_ARROW = 299               /* LEFT_ARROW  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define INT 258
#define FLOAT 259
#define RAW_STRING 260
#define STRING 261
#define CHAR 262
#define NAME 263
#define TYPENAME 264
#define COMPLETE 265
#define NAME_OR_INT 266
#define TRUE_KEYWORD 267
#define FALSE_KEYWORD 268
#define STRUCT_KEYWORD 269
#define INTERFACE_KEYWORD 270
#define TYPE_KEYWORD 271
#define CHAN_KEYWORD 272
#define SIZEOF_KEYWORD 273
#define LEN_KEYWORD 274
#define CAP_KEYWORD 275
#define NEW_KEYWORD 276
#define IOTA_KEYWORD 277
#define NIL_KEYWORD 278
#define CONST_KEYWORD 279
#define DOTDOTDOT 280
#define ENTRY 281
#define ERROR 282
#define BYTE_KEYWORD 283
#define DOLLAR_VARIABLE 284
#define ASSIGN_MODIFY 285
#define ABOVE_COMMA 286
#define OROR 287
#define ANDAND 288
#define ANDNOT 289
#define EQUAL 290
#define NOTEQUAL 291
#define LEQ 292
#define GEQ 293
#define LSH 294
#define RSH 295
#define UNARY 296
#define INCREMENT 297
#define DECREMENT 298
#define LEFT_ARROW 299

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 94 "go-exp.y"

    LONGEST lval;
    struct {
      LONGEST val;
      struct type *type;
    } typed_val_int;
    struct {
      gdb_byte val[16];
      struct type *type;
    } typed_val_float;
    struct stoken sval;
    struct symtoken ssym;
    struct type *tval;
    struct typed_stoken tsval;
    struct ttype tsym;
    int voidval;
    enum exp_opcode opcode;
    struct internalvar *ivar;
    struct stoken_vector svec;
  

#line 268 "go-exp.c.tmp"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);



/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_INT = 3,                        /* INT  */
  YYSYMBOL_FLOAT = 4,                      /* FLOAT  */
  YYSYMBOL_RAW_STRING = 5,                 /* RAW_STRING  */
  YYSYMBOL_STRING = 6,                     /* STRING  */
  YYSYMBOL_CHAR = 7,                       /* CHAR  */
  YYSYMBOL_NAME = 8,                       /* NAME  */
  YYSYMBOL_TYPENAME = 9,                   /* TYPENAME  */
  YYSYMBOL_COMPLETE = 10,                  /* COMPLETE  */
  YYSYMBOL_NAME_OR_INT = 11,               /* NAME_OR_INT  */
  YYSYMBOL_TRUE_KEYWORD = 12,              /* TRUE_KEYWORD  */
  YYSYMBOL_FALSE_KEYWORD = 13,             /* FALSE_KEYWORD  */
  YYSYMBOL_STRUCT_KEYWORD = 14,            /* STRUCT_KEYWORD  */
  YYSYMBOL_INTERFACE_KEYWORD = 15,         /* INTERFACE_KEYWORD  */
  YYSYMBOL_TYPE_KEYWORD = 16,              /* TYPE_KEYWORD  */
  YYSYMBOL_CHAN_KEYWORD = 17,              /* CHAN_KEYWORD  */
  YYSYMBOL_SIZEOF_KEYWORD = 18,            /* SIZEOF_KEYWORD  */
  YYSYMBOL_LEN_KEYWORD = 19,               /* LEN_KEYWORD  */
  YYSYMBOL_CAP_KEYWORD = 20,               /* CAP_KEYWORD  */
  YYSYMBOL_NEW_KEYWORD = 21,               /* NEW_KEYWORD  */
  YYSYMBOL_IOTA_KEYWORD = 22,              /* IOTA_KEYWORD  */
  YYSYMBOL_NIL_KEYWORD = 23,               /* NIL_KEYWORD  */
  YYSYMBOL_CONST_KEYWORD = 24,             /* CONST_KEYWORD  */
  YYSYMBOL_DOTDOTDOT = 25,                 /* DOTDOTDOT  */
  YYSYMBOL_ENTRY = 26,                     /* ENTRY  */
  YYSYMBOL_ERROR = 27,                     /* ERROR  */
  YYSYMBOL_BYTE_KEYWORD = 28,              /* BYTE_KEYWORD  */
  YYSYMBOL_DOLLAR_VARIABLE = 29,           /* DOLLAR_VARIABLE  */
  YYSYMBOL_ASSIGN_MODIFY = 30,             /* ASSIGN_MODIFY  */
  YYSYMBOL_31_ = 31,                       /* ','  */
  YYSYMBOL_ABOVE_COMMA = 32,               /* ABOVE_COMMA  */
  YYSYMBOL_33_ = 33,                       /* '='  */
  YYSYMBOL_34_ = 34,                       /* '?'  */
  YYSYMBOL_OROR = 35,                      /* OROR  */
  YYSYMBOL_ANDAND = 36,                    /* ANDAND  */
  YYSYMBOL_37_ = 37,                       /* '|'  */
  YYSYMBOL_38_ = 38,                       /* '^'  */
  YYSYMBOL_39_ = 39,                       /* '&'  */
  YYSYMBOL_ANDNOT = 40,                    /* ANDNOT  */
  YYSYMBOL_EQUAL = 41,                     /* EQUAL  */
  YYSYMBOL_NOTEQUAL = 42,                  /* NOTEQUAL  */
  YYSYMBOL_43_ = 43,                       /* '<'  */
  YYSYMBOL_44_ = 44,                       /* '>'  */
  YYSYMBOL_LEQ = 45,                       /* LEQ  */
  YYSYMBOL_GEQ = 46,                       /* GEQ  */
  YYSYMBOL_LSH = 47,                       /* LSH  */
  YYSYMBOL_RSH = 48,                       /* RSH  */
  YYSYMBOL_49_ = 49,                       /* '@'  */
  YYSYMBOL_50_ = 50,                       /* '+'  */
  YYSYMBOL_51_ = 51,                       /* '-'  */
  YYSYMBOL_52_ = 52,                       /* '*'  */
  YYSYMBOL_53_ = 53,                       /* '/'  */
  YYSYMBOL_54_ = 54,                       /* '%'  */
  YYSYMBOL_UNARY = 55,                     /* UNARY  */
  YYSYMBOL_INCREMENT = 56,                 /* INCREMENT  */
  YYSYMBOL_DECREMENT = 57,                 /* DECREMENT  */
  YYSYMBOL_LEFT_ARROW = 58,                /* LEFT_ARROW  */
  YYSYMBOL_59_ = 59,                       /* '.'  */
  YYSYMBOL_60_ = 60,                       /* '['  */
  YYSYMBOL_61_ = 61,                       /* '('  */
  YYSYMBOL_62_ = 62,                       /* '!'  */
  YYSYMBOL_63_ = 63,                       /* ']'  */
  YYSYMBOL_64_ = 64,                       /* ')'  */
  YYSYMBOL_65_ = 65,                       /* '{'  */
  YYSYMBOL_66_ = 66,                       /* '}'  */
  YYSYMBOL_67_ = 67,                       /* ':'  */
  YYSYMBOL_YYACCEPT = 68,                  /* $accept  */
  YYSYMBOL_start = 69,                     /* start  */
  YYSYMBOL_type_exp = 70,                  /* type_exp  */
  YYSYMBOL_exp1 = 71,                      /* exp1  */
  YYSYMBOL_exp = 72,                       /* exp  */
  YYSYMBOL_73_1 = 73,                      /* $@1  */
  YYSYMBOL_lcurly = 74,                    /* lcurly  */
  YYSYMBOL_arglist = 75,                   /* arglist  */
  YYSYMBOL_rcurly = 76,                    /* rcurly  */
  YYSYMBOL_string_exp = 77,                /* string_exp  */
  YYSYMBOL_variable = 78,                  /* variable  */
  YYSYMBOL_type = 79,                      /* type  */
  YYSYMBOL_name_not_typename = 80          /* name_not_typename  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;


/* Second part of user prologue.  */
#line 115 "go-exp.y"

/* YYSTYPE gets defined by %union.  */
static int parse_number (struct parser_state *,
			 const char *, int, int, YYSTYPE *);

using namespace expr;

#line 382 "go-exp.c.tmp"


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

/* The parser invokes alloca or xmalloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
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
       && ! ((defined YYMALLOC || defined xmalloc) \
             && (defined YYFREE || defined xfree)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC xmalloc
#   if ! defined xmalloc && ! defined EXIT_SUCCESS
void *xmalloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE xfree
#   if ! defined xfree && ! defined EXIT_SUCCESS
void xfree (void *); /* INFRINGES ON USER NAME SPACE */
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
#define YYFINAL  40
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   451

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  68
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  13
/* YYNRULES -- Number of rules.  */
#define YYNRULES  69
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  122

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   299


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
       2,     2,     2,    62,     2,     2,     2,    54,    39,     2,
      61,    64,    52,    50,    31,    51,    59,    53,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    67,     2,
      43,    33,    44,    34,    49,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    60,     2,    63,    38,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    65,    37,    66,     2,     2,     2,     2,
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
      25,    26,    27,    28,    29,    30,    32,    35,    36,    40,
      41,    42,    45,    46,    47,    48,    55,    56,    57,    58
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   194,   194,   195,   198,   203,   204,   209,   213,   217,
     221,   225,   229,   233,   237,   243,   250,   260,   269,   276,
     273,   286,   290,   293,   297,   301,   305,   312,   319,   325,
     329,   333,   337,   341,   345,   349,   353,   357,   361,   365,
     369,   373,   377,   381,   385,   389,   393,   397,   401,   412,
     416,   425,   432,   441,   452,   461,   464,   470,   482,   489,
     506,   524,   536,   540,   544,   558,   603,   605,   612,   625
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
  "\"end of file\"", "error", "\"invalid token\"", "INT", "FLOAT",
  "RAW_STRING", "STRING", "CHAR", "NAME", "TYPENAME", "COMPLETE",
  "NAME_OR_INT", "TRUE_KEYWORD", "FALSE_KEYWORD", "STRUCT_KEYWORD",
  "INTERFACE_KEYWORD", "TYPE_KEYWORD", "CHAN_KEYWORD", "SIZEOF_KEYWORD",
  "LEN_KEYWORD", "CAP_KEYWORD", "NEW_KEYWORD", "IOTA_KEYWORD",
  "NIL_KEYWORD", "CONST_KEYWORD", "DOTDOTDOT", "ENTRY", "ERROR",
  "BYTE_KEYWORD", "DOLLAR_VARIABLE", "ASSIGN_MODIFY", "','", "ABOVE_COMMA",
  "'='", "'?'", "OROR", "ANDAND", "'|'", "'^'", "'&'", "ANDNOT", "EQUAL",
  "NOTEQUAL", "'<'", "'>'", "LEQ", "GEQ", "LSH", "RSH", "'@'", "'+'",
  "'-'", "'*'", "'/'", "'%'", "UNARY", "INCREMENT", "DECREMENT",
  "LEFT_ARROW", "'.'", "'['", "'('", "'!'", "']'", "')'", "'{'", "'}'",
  "':'", "$accept", "start", "type_exp", "exp1", "exp", "$@1", "lcurly",
  "arglist", "rcurly", "string_exp", "variable", "type",
  "name_not_typename", YY_NULLPTRPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-42)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      49,   -42,   -42,   -42,   -42,   -42,   -42,   -42,   -42,   -42,
     -38,   -42,   -42,    49,    49,    49,    49,    49,    49,    49,
     -42,    27,   -42,    34,   187,    45,    16,   -42,    11,    53,
      49,   -35,    11,   -35,   -35,   -35,   -35,    11,    38,   -35,
     -42,    49,    49,    49,    49,    49,    49,    49,    49,    49,
      49,    49,    49,    49,    49,    49,    49,    49,    49,    49,
      49,    49,    49,    49,   -42,   -42,    -3,    49,   -42,    45,
       9,    74,    49,   -42,   123,    10,   -42,   187,   187,   187,
      88,   241,   266,   290,   313,   334,   353,   353,   368,   368,
     368,   368,   -41,   -41,   380,   390,   390,   -35,   -35,   -35,
     -42,    72,    33,    49,   -42,   -42,    49,   -42,   155,   -42,
     -42,    49,   -42,   -42,   187,    39,   -35,   -42,   215,    49,
     -42,   187
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,    51,    54,    59,    52,    69,    67,    53,    62,    63,
       0,    68,    56,     0,     0,     0,     0,     0,     0,     0,
      21,     0,     3,     2,     5,     0,    61,    55,     4,    65,
       0,    12,     0,     8,    10,     9,     7,    66,     0,    11,
       1,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    13,    14,     0,     0,    19,     0,
       0,     0,     0,    64,     0,     0,    28,     6,    50,    49,
       0,    47,    46,    45,    44,    43,    37,    38,    41,    42,
      39,    40,    35,    36,    29,    33,    34,    30,    31,    32,
      17,    15,     0,    22,    66,    25,     0,    60,     0,    58,
      57,     0,    16,    18,    23,     0,    26,    27,    48,     0,
      20,    24
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -42,   -42,   -42,    -4,   -13,   -42,   -42,   -42,   -42,   -42,
     -42,    51,    17
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,    21,    22,    23,    24,   103,    25,   115,   106,    26,
      27,    32,    29
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int8 yytable[] =
{
      31,    33,    34,    35,    36,     5,    39,   100,    58,    59,
      60,    61,    62,    63,    38,    64,    65,    74,    66,    67,
      68,    64,    65,    30,    66,    67,    68,    40,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,    28,     1,     2,     6,     3,     4,     5,     6,   108,
       7,     8,     9,   102,    41,    41,    71,    10,    37,    41,
     119,    72,    72,    11,   110,   105,    70,    11,    12,    73,
     107,    75,   112,   101,     0,     0,     0,    13,    14,     0,
     114,     0,     0,   116,     0,     0,   113,    69,   118,    15,
      16,    17,    76,   120,     0,     0,   121,     0,     0,     0,
      18,    19,     0,     0,    20,     0,     0,     0,    42,     0,
     104,    43,    44,    45,    46,    47,    48,    49,     0,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,     0,    64,    65,     0,    66,    67,    68,
       0,     0,     0,    42,     0,   111,    43,    44,    45,    46,
      47,    48,    49,     0,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,     0,    64,
      65,     0,    66,    67,    68,    42,     0,   109,    43,    44,
      45,    46,    47,    48,    49,     0,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
       0,    64,    65,     0,    66,    67,    68,    42,     0,   117,
      43,    44,    45,    46,    47,    48,    49,     0,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,     0,    64,    65,     0,    66,    67,    68,    44,
      45,    46,    47,    48,    49,     0,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
       0,    64,    65,     0,    66,    67,    68,    46,    47,    48,
      49,     0,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,     0,    64,    65,     0,
      66,    67,    68,    47,    48,    49,     0,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,     0,    64,    65,     0,    66,    67,    68,    48,    49,
       0,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,     0,    64,    65,     0,    66,
      67,    68,    49,     0,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,     0,    64,
      65,     0,    66,    67,    68,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,     0,
      64,    65,     0,    66,    67,    68,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,     0,    64,
      65,     0,    66,    67,    68,    56,    57,    58,    59,    60,
      61,    62,    63,     0,    64,    65,     0,    66,    67,    68,
      59,    60,    61,    62,    63,     0,    64,    65,     0,    66,
      67,    68,    61,    62,    63,     0,    64,    65,     0,    66,
      67,    68
};

static const yytype_int8 yycheck[] =
{
      13,    14,    15,    16,    17,     8,    19,    10,    49,    50,
      51,    52,    53,    54,    18,    56,    57,    30,    59,    60,
      61,    56,    57,    61,    59,    60,    61,     0,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,     0,     3,     4,     9,     6,     7,     8,     9,    72,
      11,    12,    13,    67,    31,    31,    50,    18,    17,    31,
      31,    61,    61,    28,    64,    66,    25,    28,    29,    26,
       6,    30,    10,    66,    -1,    -1,    -1,    38,    39,    -1,
     103,    -1,    -1,   106,    -1,    -1,    63,    52,   111,    50,
      51,    52,    64,    64,    -1,    -1,   119,    -1,    -1,    -1,
      61,    62,    -1,    -1,    65,    -1,    -1,    -1,    30,    -1,
      69,    33,    34,    35,    36,    37,    38,    39,    -1,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    -1,    56,    57,    -1,    59,    60,    61,
      -1,    -1,    -1,    30,    -1,    67,    33,    34,    35,    36,
      37,    38,    39,    -1,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    -1,    56,
      57,    -1,    59,    60,    61,    30,    -1,    64,    33,    34,
      35,    36,    37,    38,    39,    -1,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      -1,    56,    57,    -1,    59,    60,    61,    30,    -1,    64,
      33,    34,    35,    36,    37,    38,    39,    -1,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    -1,    56,    57,    -1,    59,    60,    61,    34,
      35,    36,    37,    38,    39,    -1,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      -1,    56,    57,    -1,    59,    60,    61,    36,    37,    38,
      39,    -1,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    -1,    56,    57,    -1,
      59,    60,    61,    37,    38,    39,    -1,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    -1,    56,    57,    -1,    59,    60,    61,    38,    39,
      -1,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    -1,    56,    57,    -1,    59,
      60,    61,    39,    -1,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    -1,    56,
      57,    -1,    59,    60,    61,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    -1,
      56,    57,    -1,    59,    60,    61,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    -1,    56,
      57,    -1,    59,    60,    61,    47,    48,    49,    50,    51,
      52,    53,    54,    -1,    56,    57,    -1,    59,    60,    61,
      50,    51,    52,    53,    54,    -1,    56,    57,    -1,    59,
      60,    61,    52,    53,    54,    -1,    56,    57,    -1,    59,
      60,    61
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     6,     7,     8,     9,    11,    12,    13,
      18,    28,    29,    38,    39,    50,    51,    52,    61,    62,
      65,    69,    70,    71,    72,    74,    77,    78,    79,    80,
      61,    72,    79,    72,    72,    72,    72,    79,    71,    72,
       0,    31,    30,    33,    34,    35,    36,    37,    38,    39,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    56,    57,    59,    60,    61,    52,
      79,    50,    61,    26,    72,    79,    64,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      10,    80,    71,    73,    79,    66,    76,     6,    72,    64,
      64,    67,    10,    63,    72,    75,    72,    64,    72,    31,
      64,    72
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    68,    69,    69,    70,    71,    71,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    73,
      72,    74,    75,    75,    75,    76,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    77,
      77,    72,    72,    72,    78,    78,    79,    79,    79,    80
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     3,     2,     2,     2,
       2,     2,     2,     2,     2,     3,     4,     3,     4,     0,
       5,     1,     0,     1,     3,     1,     4,     4,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     5,     3,
       3,     1,     1,     1,     1,     1,     1,     4,     4,     1,
       3,     1,     1,     1,     2,     1,     2,     1,     1,     1
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
       to xreallocate them elsewhere.  */

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
        /* Give user a chance to xreallocate the stack.  Use copies of
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
  case 4: /* type_exp: type  */
#line 199 "go-exp.y"
                        { pstate->push_new<type_operation> ((yyvsp[0].tval)); }
#line 1485 "go-exp.c.tmp"
    break;

  case 6: /* exp1: exp1 ',' exp  */
#line 205 "go-exp.y"
                        { pstate->wrap2<comma_operation> (); }
#line 1491 "go-exp.c.tmp"
    break;

  case 7: /* exp: '*' exp  */
#line 210 "go-exp.y"
                        { pstate->wrap<unop_ind_operation> (); }
#line 1497 "go-exp.c.tmp"
    break;

  case 8: /* exp: '&' exp  */
#line 214 "go-exp.y"
                        { pstate->wrap<unop_addr_operation> (); }
#line 1503 "go-exp.c.tmp"
    break;

  case 9: /* exp: '-' exp  */
#line 218 "go-exp.y"
                        { pstate->wrap<unary_neg_operation> (); }
#line 1509 "go-exp.c.tmp"
    break;

  case 10: /* exp: '+' exp  */
#line 222 "go-exp.y"
                        { pstate->wrap<unary_plus_operation> (); }
#line 1515 "go-exp.c.tmp"
    break;

  case 11: /* exp: '!' exp  */
#line 226 "go-exp.y"
                        { pstate->wrap<unary_logical_not_operation> (); }
#line 1521 "go-exp.c.tmp"
    break;

  case 12: /* exp: '^' exp  */
#line 230 "go-exp.y"
                        { pstate->wrap<unary_complement_operation> (); }
#line 1527 "go-exp.c.tmp"
    break;

  case 13: /* exp: exp INCREMENT  */
#line 234 "go-exp.y"
                        { pstate->wrap<postinc_operation> (); }
#line 1533 "go-exp.c.tmp"
    break;

  case 14: /* exp: exp DECREMENT  */
#line 238 "go-exp.y"
                        { pstate->wrap<postdec_operation> (); }
#line 1539 "go-exp.c.tmp"
    break;

  case 15: /* exp: exp '.' name_not_typename  */
#line 244 "go-exp.y"
                        {
			  pstate->push_new<structop_operation>
			    (pstate->pop (), copy_name ((yyvsp[0].ssym).stoken));
			}
#line 1548 "go-exp.c.tmp"
    break;

  case 16: /* exp: exp '.' name_not_typename COMPLETE  */
#line 251 "go-exp.y"
                        {
			  structop_base_operation *op
			    = new structop_operation (pstate->pop (),
						      copy_name ((yyvsp[-1].ssym).stoken));
			  pstate->mark_struct_expression (op);
			  pstate->push (operation_up (op));
			}
#line 1560 "go-exp.c.tmp"
    break;

  case 17: /* exp: exp '.' COMPLETE  */
#line 261 "go-exp.y"
                        {
			  structop_base_operation *op
			    = new structop_operation (pstate->pop (), "");
			  pstate->mark_struct_expression (op);
			  pstate->push (operation_up (op));
			}
#line 1571 "go-exp.c.tmp"
    break;

  case 18: /* exp: exp '[' exp1 ']'  */
#line 270 "go-exp.y"
                        { pstate->wrap2<subscript_operation> (); }
#line 1577 "go-exp.c.tmp"
    break;

  case 19: /* $@1: %empty  */
#line 276 "go-exp.y"
                        { pstate->start_arglist (); }
#line 1583 "go-exp.c.tmp"
    break;

  case 20: /* exp: exp '(' $@1 arglist ')'  */
#line 278 "go-exp.y"
                        {
			  std::vector<operation_up> args
			    = pstate->pop_vector (pstate->end_arglist ());
			  pstate->push_new<funcall_operation>
			    (pstate->pop (), std::move (args));
			}
#line 1594 "go-exp.c.tmp"
    break;

  case 21: /* lcurly: '{'  */
#line 287 "go-exp.y"
                        { pstate->start_arglist (); }
#line 1600 "go-exp.c.tmp"
    break;

  case 23: /* arglist: exp  */
#line 294 "go-exp.y"
                        { pstate->arglist_len = 1; }
#line 1606 "go-exp.c.tmp"
    break;

  case 24: /* arglist: arglist ',' exp  */
#line 298 "go-exp.y"
                        { pstate->arglist_len++; }
#line 1612 "go-exp.c.tmp"
    break;

  case 25: /* rcurly: '}'  */
#line 302 "go-exp.y"
                        { (yyval.lval) = pstate->end_arglist () - 1; }
#line 1618 "go-exp.c.tmp"
    break;

  case 26: /* exp: lcurly type rcurly exp  */
#line 306 "go-exp.y"
                        {
			  pstate->push_new<unop_memval_operation>
			    (pstate->pop (), (yyvsp[-2].tval));
			}
#line 1627 "go-exp.c.tmp"
    break;

  case 27: /* exp: type '(' exp ')'  */
#line 313 "go-exp.y"
                        {
			  pstate->push_new<unop_cast_operation>
			    (pstate->pop (), (yyvsp[-3].tval));
			}
#line 1636 "go-exp.c.tmp"
    break;

  case 28: /* exp: '(' exp1 ')'  */
#line 320 "go-exp.y"
                        { }
#line 1642 "go-exp.c.tmp"
    break;

  case 29: /* exp: exp '@' exp  */
#line 326 "go-exp.y"
                        { pstate->wrap2<repeat_operation> (); }
#line 1648 "go-exp.c.tmp"
    break;

  case 30: /* exp: exp '*' exp  */
#line 330 "go-exp.y"
                        { pstate->wrap2<mul_operation> (); }
#line 1654 "go-exp.c.tmp"
    break;

  case 31: /* exp: exp '/' exp  */
#line 334 "go-exp.y"
                        { pstate->wrap2<div_operation> (); }
#line 1660 "go-exp.c.tmp"
    break;

  case 32: /* exp: exp '%' exp  */
#line 338 "go-exp.y"
                        { pstate->wrap2<rem_operation> (); }
#line 1666 "go-exp.c.tmp"
    break;

  case 33: /* exp: exp '+' exp  */
#line 342 "go-exp.y"
                        { pstate->wrap2<add_operation> (); }
#line 1672 "go-exp.c.tmp"
    break;

  case 34: /* exp: exp '-' exp  */
#line 346 "go-exp.y"
                        { pstate->wrap2<sub_operation> (); }
#line 1678 "go-exp.c.tmp"
    break;

  case 35: /* exp: exp LSH exp  */
#line 350 "go-exp.y"
                        { pstate->wrap2<lsh_operation> (); }
#line 1684 "go-exp.c.tmp"
    break;

  case 36: /* exp: exp RSH exp  */
#line 354 "go-exp.y"
                        { pstate->wrap2<rsh_operation> (); }
#line 1690 "go-exp.c.tmp"
    break;

  case 37: /* exp: exp EQUAL exp  */
#line 358 "go-exp.y"
                        { pstate->wrap2<equal_operation> (); }
#line 1696 "go-exp.c.tmp"
    break;

  case 38: /* exp: exp NOTEQUAL exp  */
#line 362 "go-exp.y"
                        { pstate->wrap2<notequal_operation> (); }
#line 1702 "go-exp.c.tmp"
    break;

  case 39: /* exp: exp LEQ exp  */
#line 366 "go-exp.y"
                        { pstate->wrap2<leq_operation> (); }
#line 1708 "go-exp.c.tmp"
    break;

  case 40: /* exp: exp GEQ exp  */
#line 370 "go-exp.y"
                        { pstate->wrap2<geq_operation> (); }
#line 1714 "go-exp.c.tmp"
    break;

  case 41: /* exp: exp '<' exp  */
#line 374 "go-exp.y"
                        { pstate->wrap2<less_operation> (); }
#line 1720 "go-exp.c.tmp"
    break;

  case 42: /* exp: exp '>' exp  */
#line 378 "go-exp.y"
                        { pstate->wrap2<gtr_operation> (); }
#line 1726 "go-exp.c.tmp"
    break;

  case 43: /* exp: exp '&' exp  */
#line 382 "go-exp.y"
                        { pstate->wrap2<bitwise_and_operation> (); }
#line 1732 "go-exp.c.tmp"
    break;

  case 44: /* exp: exp '^' exp  */
#line 386 "go-exp.y"
                        { pstate->wrap2<bitwise_xor_operation> (); }
#line 1738 "go-exp.c.tmp"
    break;

  case 45: /* exp: exp '|' exp  */
#line 390 "go-exp.y"
                        { pstate->wrap2<bitwise_ior_operation> (); }
#line 1744 "go-exp.c.tmp"
    break;

  case 46: /* exp: exp ANDAND exp  */
#line 394 "go-exp.y"
                        { pstate->wrap2<logical_and_operation> (); }
#line 1750 "go-exp.c.tmp"
    break;

  case 47: /* exp: exp OROR exp  */
#line 398 "go-exp.y"
                        { pstate->wrap2<logical_or_operation> (); }
#line 1756 "go-exp.c.tmp"
    break;

  case 48: /* exp: exp '?' exp ':' exp  */
#line 402 "go-exp.y"
                        {
			  operation_up last = pstate->pop ();
			  operation_up mid = pstate->pop ();
			  operation_up first = pstate->pop ();
			  pstate->push_new<ternop_cond_operation>
			    (std::move (first), std::move (mid),
			     std::move (last));
			}
#line 1769 "go-exp.c.tmp"
    break;

  case 49: /* exp: exp '=' exp  */
#line 413 "go-exp.y"
                        { pstate->wrap2<assign_operation> (); }
#line 1775 "go-exp.c.tmp"
    break;

  case 50: /* exp: exp ASSIGN_MODIFY exp  */
#line 417 "go-exp.y"
                        {
			  operation_up rhs = pstate->pop ();
			  operation_up lhs = pstate->pop ();
			  pstate->push_new<assign_modify_operation>
			    ((yyvsp[-1].opcode), std::move (lhs), std::move (rhs));
			}
#line 1786 "go-exp.c.tmp"
    break;

  case 51: /* exp: INT  */
#line 426 "go-exp.y"
                        {
			  pstate->push_new<long_const_operation>
			    ((yyvsp[0].typed_val_int).type, (yyvsp[0].typed_val_int).val);
			}
#line 1795 "go-exp.c.tmp"
    break;

  case 52: /* exp: CHAR  */
#line 433 "go-exp.y"
                        {
			  struct stoken_vector vec;
			  vec.len = 1;
			  vec.tokens = &(yyvsp[0].tsval);
			  pstate->push_c_string ((yyvsp[0].tsval).type, &vec);
			}
#line 1806 "go-exp.c.tmp"
    break;

  case 53: /* exp: NAME_OR_INT  */
#line 442 "go-exp.y"
                        { YYSTYPE val;
			  parse_number (pstate, (yyvsp[0].ssym).stoken.ptr,
					(yyvsp[0].ssym).stoken.length, 0, &val);
			  pstate->push_new<long_const_operation>
			    (val.typed_val_int.type,
			     val.typed_val_int.val);
			}
#line 1818 "go-exp.c.tmp"
    break;

  case 54: /* exp: FLOAT  */
#line 453 "go-exp.y"
                        {
			  float_data data;
			  std::copy (std::begin ((yyvsp[0].typed_val_float).val), std::end ((yyvsp[0].typed_val_float).val),
				     std::begin (data));
			  pstate->push_new<float_const_operation> ((yyvsp[0].typed_val_float).type, data);
			}
#line 1829 "go-exp.c.tmp"
    break;

  case 56: /* exp: DOLLAR_VARIABLE  */
#line 465 "go-exp.y"
                        {
			  pstate->push_dollar ((yyvsp[0].sval));
			}
#line 1837 "go-exp.c.tmp"
    break;

  case 57: /* exp: SIZEOF_KEYWORD '(' type ')'  */
#line 471 "go-exp.y"
                        {
			  /* TODO(dje): Go objects in structs.  */
			  /* TODO(dje): What's the right type here?  */
			  struct type *size_type
			    = parse_type (pstate)->builtin_unsigned_int;
			  (yyvsp[-1].tval) = check_typedef ((yyvsp[-1].tval));
			  pstate->push_new<long_const_operation>
			    (size_type, (LONGEST) (yyvsp[-1].tval)->length ());
			}
#line 1851 "go-exp.c.tmp"
    break;

  case 58: /* exp: SIZEOF_KEYWORD '(' exp ')'  */
#line 483 "go-exp.y"
                        {
			  /* TODO(dje): Go objects in structs.  */
			  pstate->wrap<unop_sizeof_operation> ();
			}
#line 1860 "go-exp.c.tmp"
    break;

  case 59: /* string_exp: STRING  */
#line 490 "go-exp.y"
                        {
			  /* We copy the string here, and not in the
			     lexer, to guarantee that we do not leak a
			     string.  */
			  /* Note that we NUL-terminate here, but just
			     for convenience.  */
			  struct typed_stoken *vec = XNEW (struct typed_stoken);
			  (yyval.svec).len = 1;
			  (yyval.svec).tokens = vec;

			  vec->type = (yyvsp[0].tsval).type;
			  vec->length = (yyvsp[0].tsval).length;
			  vec->ptr = (char *) xmalloc ((yyvsp[0].tsval).length + 1);
			  memcpy (vec->ptr, (yyvsp[0].tsval).ptr, (yyvsp[0].tsval).length + 1);
			}
#line 1880 "go-exp.c.tmp"
    break;

  case 60: /* string_exp: string_exp '+' STRING  */
#line 507 "go-exp.y"
                        {
			  /* Note that we NUL-terminate here, but just
			     for convenience.  */
			  char *p;
			  ++(yyval.svec).len;
			  (yyval.svec).tokens = XRESIZEVEC (struct typed_stoken,
						  (yyval.svec).tokens, (yyval.svec).len);

			  p = (char *) xmalloc ((yyvsp[0].tsval).length + 1);
			  memcpy (p, (yyvsp[0].tsval).ptr, (yyvsp[0].tsval).length + 1);

			  (yyval.svec).tokens[(yyval.svec).len - 1].type = (yyvsp[0].tsval).type;
			  (yyval.svec).tokens[(yyval.svec).len - 1].length = (yyvsp[0].tsval).length;
			  (yyval.svec).tokens[(yyval.svec).len - 1].ptr = p;
			}
#line 1900 "go-exp.c.tmp"
    break;

  case 61: /* exp: string_exp  */
#line 525 "go-exp.y"
                        {
			  int i;

			  /* Always utf8.  */
			  pstate->push_c_string (0, &(yyvsp[0].svec));
			  for (i = 0; i < (yyvsp[0].svec).len; ++i)
			    xfree ((yyvsp[0].svec).tokens[i].ptr);
			  xfree ((yyvsp[0].svec).tokens);
			}
#line 1914 "go-exp.c.tmp"
    break;

  case 62: /* exp: TRUE_KEYWORD  */
#line 537 "go-exp.y"
                        { pstate->push_new<bool_operation> ((yyvsp[0].lval)); }
#line 1920 "go-exp.c.tmp"
    break;

  case 63: /* exp: FALSE_KEYWORD  */
#line 541 "go-exp.y"
                        { pstate->push_new<bool_operation> ((yyvsp[0].lval)); }
#line 1926 "go-exp.c.tmp"
    break;

  case 64: /* variable: name_not_typename ENTRY  */
#line 545 "go-exp.y"
                        { struct symbol *sym = (yyvsp[-1].ssym).sym.symbol;

			  if (sym == NULL
			      || !sym->is_argument ()
			      || !symbol_read_needs_frame (sym))
			    error (_("@entry can be used only for function "
				     "parameters, not for \"%s\""),
				   copy_name ((yyvsp[-1].ssym).stoken).c_str ());

			  pstate->push_new<var_entry_value_operation> (sym);
			}
#line 1942 "go-exp.c.tmp"
    break;

  case 65: /* variable: name_not_typename  */
#line 559 "go-exp.y"
                        { struct block_symbol sym = (yyvsp[0].ssym).sym;

			  if (sym.symbol)
			    {
			      if (symbol_read_needs_frame (sym.symbol))
				pstate->block_tracker->update (sym);

			      pstate->push_new<var_value_operation> (sym);
			    }
			  else if ((yyvsp[0].ssym).is_a_field_of_this)
			    {
			      /* TODO(dje): Can we get here?
				 E.g., via a mix of c++ and go?  */
			      gdb_assert_not_reached ("go with `this' field");
			    }
			  else
			    {
			      struct bound_minimal_symbol msymbol;
			      std::string arg = copy_name ((yyvsp[0].ssym).stoken);

			      msymbol =
				lookup_bound_minimal_symbol (arg.c_str ());
			      if (msymbol.minsym != NULL)
				pstate->push_new<var_msym_value_operation>
				  (msymbol);
			      else if (!have_full_symbols ()
				       && !have_partial_symbols ())
				error (_("No symbol table is loaded.  "
				       "Use the \"file\" command."));
			      else
				error (_("No symbol \"%s\" in current context."),
				       arg.c_str ());
			    }
			}
#line 1981 "go-exp.c.tmp"
    break;

  case 66: /* type: '*' type  */
#line 604 "go-exp.y"
                        { (yyval.tval) = lookup_pointer_type ((yyvsp[0].tval)); }
#line 1987 "go-exp.c.tmp"
    break;

  case 67: /* type: TYPENAME  */
#line 606 "go-exp.y"
                        { (yyval.tval) = (yyvsp[0].tsym).type; }
#line 1993 "go-exp.c.tmp"
    break;

  case 68: /* type: BYTE_KEYWORD  */
#line 613 "go-exp.y"
                        { (yyval.tval) = builtin_go_type (pstate->gdbarch ())
			    ->builtin_uint8; }
#line 2000 "go-exp.c.tmp"
    break;


#line 2004 "go-exp.c.tmp"

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

#line 635 "go-exp.y"


/* Take care of parsing a number (anything that starts with a digit).
   Set yylval and return the token type; update lexptr.
   LEN is the number of characters in it.  */

/* FIXME: Needs some error checking for the float case.  */
/* FIXME(dje): IWBN to use c-exp.y's parse_number if we could.
   That will require moving the guts into a function that we both call
   as our YYSTYPE is different than c-exp.y's  */

static int
parse_number (struct parser_state *par_state,
	      const char *p, int len, int parsed_float, YYSTYPE *putithere)
{
  ULONGEST n = 0;
  ULONGEST prevn = 0;

  int i = 0;
  int c;
  int base = input_radix;
  int unsigned_p = 0;

  /* Number of "L" suffixes encountered.  */
  int long_p = 0;

  /* We have found a "L" or "U" suffix.  */
  int found_suffix = 0;

  if (parsed_float)
    {
      const struct builtin_go_type *builtin_go_types
	= builtin_go_type (par_state->gdbarch ());

      /* Handle suffixes: 'f' for float32, 'l' for long double.
	 FIXME: This appears to be an extension -- do we want this?  */
      if (len >= 1 && tolower (p[len - 1]) == 'f')
	{
	  putithere->typed_val_float.type
	    = builtin_go_types->builtin_float32;
	  len--;
	}
      else if (len >= 1 && tolower (p[len - 1]) == 'l')
	{
	  putithere->typed_val_float.type
	    = parse_type (par_state)->builtin_long_double;
	  len--;
	}
      /* Default type for floating-point literals is float64.  */
      else
	{
	  putithere->typed_val_float.type
	    = builtin_go_types->builtin_float64;
	}

      if (!parse_float (p, len,
			putithere->typed_val_float.type,
			putithere->typed_val_float.val))
	return ERROR;
      return FLOAT;
    }

  /* Handle base-switching prefixes 0x, 0t, 0d, 0.  */
  if (p[0] == '0' && len > 1)
    switch (p[1])
      {
      case 'x':
      case 'X':
	if (len >= 3)
	  {
	    p += 2;
	    base = 16;
	    len -= 2;
	  }
	break;

      case 'b':
      case 'B':
	if (len >= 3)
	  {
	    p += 2;
	    base = 2;
	    len -= 2;
	  }
	break;

      case 't':
      case 'T':
      case 'd':
      case 'D':
	if (len >= 3)
	  {
	    p += 2;
	    base = 10;
	    len -= 2;
	  }
	break;

      default:
	base = 8;
	break;
      }

  while (len-- > 0)
    {
      c = *p++;
      if (c >= 'A' && c <= 'Z')
	c += 'a' - 'A';
      if (c != 'l' && c != 'u')
	n *= base;
      if (c >= '0' && c <= '9')
	{
	  if (found_suffix)
	    return ERROR;
	  n += i = c - '0';
	}
      else
	{
	  if (base > 10 && c >= 'a' && c <= 'f')
	    {
	      if (found_suffix)
		return ERROR;
	      n += i = c - 'a' + 10;
	    }
	  else if (c == 'l')
	    {
	      ++long_p;
	      found_suffix = 1;
	    }
	  else if (c == 'u')
	    {
	      unsigned_p = 1;
	      found_suffix = 1;
	    }
	  else
	    return ERROR;	/* Char not a digit */
	}
      if (i >= base)
	return ERROR;		/* Invalid digit in this base.  */

      if (c != 'l' && c != 'u')
	{
	  /* Test for overflow.  */
	  if (n == 0 && prevn == 0)
	    ;
	  else if (prevn >= n)
	    error (_("Numeric constant too large."));
	}
      prevn = n;
    }

  /* An integer constant is an int, a long, or a long long.  An L
     suffix forces it to be long; an LL suffix forces it to be long
     long.  If not forced to a larger size, it gets the first type of
     the above that it fits in.  To figure out whether it fits, we
     shift it right and see whether anything remains.  Note that we
     can't shift sizeof (LONGEST) * HOST_CHAR_BIT bits or more in one
     operation, because many compilers will warn about such a shift
     (which always produces a zero result).  Sometimes gdbarch_int_bit
     or gdbarch_long_bit will be that big, sometimes not.  To deal with
     the case where it is we just always shift the value more than
     once, with fewer bits each time.  */

  int int_bits = gdbarch_int_bit (par_state->gdbarch ());
  int long_bits = gdbarch_long_bit (par_state->gdbarch ());
  int long_long_bits = gdbarch_long_long_bit (par_state->gdbarch ());
  bool have_signed = !unsigned_p;
  bool have_int = long_p == 0;
  bool have_long = long_p <= 1;
  if (have_int && have_signed && fits_in_type (1, n, int_bits, true))
    putithere->typed_val_int.type = parse_type (par_state)->builtin_int;
  else if (have_int && fits_in_type (1, n, int_bits, false))
    putithere->typed_val_int.type
      = parse_type (par_state)->builtin_unsigned_int;
  else if (have_long && have_signed && fits_in_type (1, n, long_bits, true))
    putithere->typed_val_int.type = parse_type (par_state)->builtin_long;
  else if (have_long && fits_in_type (1, n, long_bits, false))
    putithere->typed_val_int.type
      = parse_type (par_state)->builtin_unsigned_long;
  else if (have_signed && fits_in_type (1, n, long_long_bits, true))
    putithere->typed_val_int.type
      = parse_type (par_state)->builtin_long_long;
  else if (fits_in_type (1, n, long_long_bits, false))
    putithere->typed_val_int.type
      = parse_type (par_state)->builtin_unsigned_long_long;
  else
    error (_("Numeric constant too large."));
  putithere->typed_val_int.val = n;

   return INT;
}

/* Temporary obstack used for holding strings.  */
static struct obstack tempbuf;
static int tempbuf_init;

/* Parse a string or character literal from TOKPTR.  The string or
   character may be wide or unicode.  *OUTPTR is set to just after the
   end of the literal in the input string.  The resulting token is
   stored in VALUE.  This returns a token value, either STRING or
   CHAR, depending on what was parsed.  *HOST_CHARS is set to the
   number of host characters in the literal.  */

static int
parse_string_or_char (const char *tokptr, const char **outptr,
		      struct typed_stoken *value, int *host_chars)
{
  int quote;

  /* Build the gdb internal form of the input string in tempbuf.  Note
     that the buffer is null byte terminated *only* for the
     convenience of debugging gdb itself and printing the buffer
     contents when the buffer contains no embedded nulls.  Gdb does
     not depend upon the buffer being null byte terminated, it uses
     the length string instead.  This allows gdb to handle C strings
     (as well as strings in other languages) with embedded null
     bytes */

  if (!tempbuf_init)
    tempbuf_init = 1;
  else
    obstack_free (&tempbuf, NULL);
  obstack_init (&tempbuf);

  /* Skip the quote.  */
  quote = *tokptr;
  ++tokptr;

  *host_chars = 0;

  while (*tokptr)
    {
      char c = *tokptr;
      if (c == '\\')
	{
	  ++tokptr;
	  *host_chars += c_parse_escape (&tokptr, &tempbuf);
	}
      else if (c == quote)
	break;
      else
	{
	  obstack_1grow (&tempbuf, c);
	  ++tokptr;
	  /* FIXME: this does the wrong thing with multi-byte host
	     characters.  We could use mbrlen here, but that would
	     make "set host-charset" a bit less useful.  */
	  ++*host_chars;
	}
    }

  if (*tokptr != quote)
    {
      if (quote == '"')
	error (_("Unterminated string in expression."));
      else
	error (_("Unmatched single quote."));
    }
  ++tokptr;

  value->type = (int) C_STRING | (quote == '\'' ? C_CHAR : 0); /*FIXME*/
  value->ptr = (char *) obstack_base (&tempbuf);
  value->length = obstack_object_size (&tempbuf);

  *outptr = tokptr;

  return quote == '\'' ? CHAR : STRING;
}

struct token
{
  const char *oper;
  int token;
  enum exp_opcode opcode;
};

static const struct token tokentab3[] =
  {
    {">>=", ASSIGN_MODIFY, BINOP_RSH},
    {"<<=", ASSIGN_MODIFY, BINOP_LSH},
    /*{"&^=", ASSIGN_MODIFY, BINOP_BITWISE_ANDNOT}, TODO */
    {"...", DOTDOTDOT, OP_NULL},
  };

static const struct token tokentab2[] =
  {
    {"+=", ASSIGN_MODIFY, BINOP_ADD},
    {"-=", ASSIGN_MODIFY, BINOP_SUB},
    {"*=", ASSIGN_MODIFY, BINOP_MUL},
    {"/=", ASSIGN_MODIFY, BINOP_DIV},
    {"%=", ASSIGN_MODIFY, BINOP_REM},
    {"|=", ASSIGN_MODIFY, BINOP_BITWISE_IOR},
    {"&=", ASSIGN_MODIFY, BINOP_BITWISE_AND},
    {"^=", ASSIGN_MODIFY, BINOP_BITWISE_XOR},
    {"++", INCREMENT, OP_NULL},
    {"--", DECREMENT, OP_NULL},
    /*{"->", RIGHT_ARROW, OP_NULL}, Doesn't exist in Go.  */
    {"<-", LEFT_ARROW, OP_NULL},
    {"&&", ANDAND, OP_NULL},
    {"||", OROR, OP_NULL},
    {"<<", LSH, OP_NULL},
    {">>", RSH, OP_NULL},
    {"==", EQUAL, OP_NULL},
    {"!=", NOTEQUAL, OP_NULL},
    {"<=", LEQ, OP_NULL},
    {">=", GEQ, OP_NULL},
    /*{"&^", ANDNOT, OP_NULL}, TODO */
  };

/* Identifier-like tokens.  */
static const struct token ident_tokens[] =
  {
    {"true", TRUE_KEYWORD, OP_NULL},
    {"false", FALSE_KEYWORD, OP_NULL},
    {"nil", NIL_KEYWORD, OP_NULL},
    {"const", CONST_KEYWORD, OP_NULL},
    {"struct", STRUCT_KEYWORD, OP_NULL},
    {"type", TYPE_KEYWORD, OP_NULL},
    {"interface", INTERFACE_KEYWORD, OP_NULL},
    {"chan", CHAN_KEYWORD, OP_NULL},
    {"byte", BYTE_KEYWORD, OP_NULL}, /* An alias of uint8.  */
    {"len", LEN_KEYWORD, OP_NULL},
    {"cap", CAP_KEYWORD, OP_NULL},
    {"new", NEW_KEYWORD, OP_NULL},
    {"iota", IOTA_KEYWORD, OP_NULL},
  };

/* This is set if a NAME token appeared at the very end of the input
   string, with no whitespace separating the name from the EOF.  This
   is used only when parsing to do field name completion.  */
static int saw_name_at_eof;

/* This is set if the previously-returned token was a structure
   operator -- either '.' or ARROW.  This is used only when parsing to
   do field name completion.  */
static int last_was_structop;

/* Depth of parentheses.  */
static int paren_depth;

/* Read one token, getting characters through lexptr.  */

static int
lex_one_token (struct parser_state *par_state)
{
  int c;
  int namelen;
  const char *tokstart;
  int saw_structop = last_was_structop;

  last_was_structop = 0;

 retry:

  par_state->prev_lexptr = par_state->lexptr;

  tokstart = par_state->lexptr;
  /* See if it is a special token of length 3.  */
  for (const auto &token : tokentab3)
    if (strncmp (tokstart, token.oper, 3) == 0)
      {
	par_state->lexptr += 3;
	yylval.opcode = token.opcode;
	return token.token;
      }

  /* See if it is a special token of length 2.  */
  for (const auto &token : tokentab2)
    if (strncmp (tokstart, token.oper, 2) == 0)
      {
	par_state->lexptr += 2;
	yylval.opcode = token.opcode;
	/* NOTE: -> doesn't exist in Go, so we don't need to watch for
	   setting last_was_structop here.  */
	return token.token;
      }

  switch (c = *tokstart)
    {
    case 0:
      if (saw_name_at_eof)
	{
	  saw_name_at_eof = 0;
	  return COMPLETE;
	}
      else if (saw_structop)
	return COMPLETE;
      else
	return 0;

    case ' ':
    case '\t':
    case '\n':
      par_state->lexptr++;
      goto retry;

    case '[':
    case '(':
      paren_depth++;
      par_state->lexptr++;
      return c;

    case ']':
    case ')':
      if (paren_depth == 0)
	return 0;
      paren_depth--;
      par_state->lexptr++;
      return c;

    case ',':
      if (pstate->comma_terminates
	  && paren_depth == 0)
	return 0;
      par_state->lexptr++;
      return c;

    case '.':
      /* Might be a floating point number.  */
      if (par_state->lexptr[1] < '0' || par_state->lexptr[1] > '9')
	{
	  if (pstate->parse_completion)
	    last_was_structop = 1;
	  goto symbol;		/* Nope, must be a symbol. */
	}
      /* FALL THRU.  */

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      {
	/* It's a number.  */
	int got_dot = 0, got_e = 0, toktype;
	const char *p = tokstart;
	int hex = input_radix > 10;

	if (c == '0' && (p[1] == 'x' || p[1] == 'X'))
	  {
	    p += 2;
	    hex = 1;
	  }

	for (;; ++p)
	  {
	    /* This test includes !hex because 'e' is a valid hex digit
	       and thus does not indicate a floating point number when
	       the radix is hex.  */
	    if (!hex && !got_e && (*p == 'e' || *p == 'E'))
	      got_dot = got_e = 1;
	    /* This test does not include !hex, because a '.' always indicates
	       a decimal floating point number regardless of the radix.  */
	    else if (!got_dot && *p == '.')
	      got_dot = 1;
	    else if (got_e && (p[-1] == 'e' || p[-1] == 'E')
		     && (*p == '-' || *p == '+'))
	      /* This is the sign of the exponent, not the end of the
		 number.  */
	      continue;
	    /* We will take any letters or digits.  parse_number will
	       complain if past the radix, or if L or U are not final.  */
	    else if ((*p < '0' || *p > '9')
		     && ((*p < 'a' || *p > 'z')
				  && (*p < 'A' || *p > 'Z')))
	      break;
	  }
	toktype = parse_number (par_state, tokstart, p - tokstart,
				got_dot|got_e, &yylval);
	if (toktype == ERROR)
	  {
	    char *err_copy = (char *) alloca (p - tokstart + 1);

	    memcpy (err_copy, tokstart, p - tokstart);
	    err_copy[p - tokstart] = 0;
	    error (_("Invalid number \"%s\"."), err_copy);
	  }
	par_state->lexptr = p;
	return toktype;
      }

    case '@':
      {
	const char *p = &tokstart[1];
	size_t len = strlen ("entry");

	while (isspace (*p))
	  p++;
	if (strncmp (p, "entry", len) == 0 && !isalnum (p[len])
	    && p[len] != '_')
	  {
	    par_state->lexptr = &p[len];
	    return ENTRY;
	  }
      }
      /* FALLTHRU */
    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
    case '|':
    case '&':
    case '^':
    case '~':
    case '!':
    case '<':
    case '>':
    case '?':
    case ':':
    case '=':
    case '{':
    case '}':
    symbol:
      par_state->lexptr++;
      return c;

    case '\'':
    case '"':
    case '`':
      {
	int host_len;
	int result = parse_string_or_char (tokstart, &par_state->lexptr,
					   &yylval.tsval, &host_len);
	if (result == CHAR)
	  {
	    if (host_len == 0)
	      error (_("Empty character constant."));
	    else if (host_len > 2 && c == '\'')
	      {
		++tokstart;
		namelen = par_state->lexptr - tokstart - 1;
		goto tryname;
	      }
	    else if (host_len > 1)
	      error (_("Invalid character constant."));
	  }
	return result;
      }
    }

  if (!(c == '_' || c == '$'
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
    /* We must have come across a bad character (e.g. ';').  */
    error (_("Invalid character '%c' in expression."), c);

  /* It's a name.  See how long it is.  */
  namelen = 0;
  for (c = tokstart[namelen];
       (c == '_' || c == '$' || (c >= '0' && c <= '9')
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));)
    {
      c = tokstart[++namelen];
    }

  /* The token "if" terminates the expression and is NOT removed from
     the input stream.  It doesn't count if it appears in the
     expansion of a macro.  */
  if (namelen == 2
      && tokstart[0] == 'i'
      && tokstart[1] == 'f')
    {
      return 0;
    }

  /* For the same reason (breakpoint conditions), "thread N"
     terminates the expression.  "thread" could be an identifier, but
     an identifier is never followed by a number without intervening
     punctuation.
     Handle abbreviations of these, similarly to
     breakpoint.c:find_condition_and_thread.
     TODO: Watch for "goroutine" here?  */
  if (namelen >= 1
      && strncmp (tokstart, "thread", namelen) == 0
      && (tokstart[namelen] == ' ' || tokstart[namelen] == '\t'))
    {
      const char *p = tokstart + namelen + 1;

      while (*p == ' ' || *p == '\t')
	p++;
      if (*p >= '0' && *p <= '9')
	return 0;
    }

  par_state->lexptr += namelen;

  tryname:

  yylval.sval.ptr = tokstart;
  yylval.sval.length = namelen;

  /* Catch specific keywords.  */
  std::string copy = copy_name (yylval.sval);
  for (const auto &token : ident_tokens)
    if (copy == token.oper)
      {
	/* It is ok to always set this, even though we don't always
	   strictly need to.  */
	yylval.opcode = token.opcode;
	return token.token;
      }

  if (*tokstart == '$')
    return DOLLAR_VARIABLE;

  if (pstate->parse_completion && *par_state->lexptr == '\0')
    saw_name_at_eof = 1;
  return NAME;
}

/* An object of this type is pushed on a FIFO by the "outer" lexer.  */
struct token_and_value
{
  int token;
  YYSTYPE value;
};

/* A FIFO of tokens that have been read but not yet returned to the
   parser.  */
static std::vector<token_and_value> token_fifo;

/* Non-zero if the lexer should return tokens from the FIFO.  */
static int popping;

/* Temporary storage for yylex; this holds symbol names as they are
   built up.  */
static auto_obstack name_obstack;

/* Build "package.name" in name_obstack.
   For convenience of the caller, the name is NUL-terminated,
   but the NUL is not included in the recorded length.  */

static struct stoken
build_packaged_name (const char *package, int package_len,
		     const char *name, int name_len)
{
  struct stoken result;

  name_obstack.clear ();
  obstack_grow (&name_obstack, package, package_len);
  obstack_grow_str (&name_obstack, ".");
  obstack_grow (&name_obstack, name, name_len);
  obstack_grow (&name_obstack, "", 1);
  result.ptr = (char *) obstack_base (&name_obstack);
  result.length = obstack_object_size (&name_obstack) - 1;

  return result;
}

/* Return non-zero if NAME is a package name.
   BLOCK is the scope in which to interpret NAME; this can be NULL
   to mean the global scope.  */

static int
package_name_p (const char *name, const struct block *block)
{
  struct symbol *sym;
  struct field_of_this_result is_a_field_of_this;

  sym = lookup_symbol (name, block, STRUCT_DOMAIN, &is_a_field_of_this).symbol;

  if (sym
      && sym->aclass () == LOC_TYPEDEF
      && sym->type ()->code () == TYPE_CODE_MODULE)
    return 1;

  return 0;
}

/* Classify a (potential) function in the "unsafe" package.
   We fold these into "keywords" to keep things simple, at least until
   something more complex is warranted.  */

static int
classify_unsafe_function (struct stoken function_name)
{
  std::string copy = copy_name (function_name);

  if (copy == "Sizeof")
    {
      yylval.sval = function_name;
      return SIZEOF_KEYWORD;
    }

  error (_("Unknown function in `unsafe' package: %s"), copy.c_str ());
}

/* Classify token(s) "name1.name2" where name1 is known to be a package.
   The contents of the token are in `yylval'.
   Updates yylval and returns the new token type.

   The result is one of NAME, NAME_OR_INT, or TYPENAME.  */

static int
classify_packaged_name (const struct block *block)
{
  struct block_symbol sym;
  struct field_of_this_result is_a_field_of_this;

  std::string copy = copy_name (yylval.sval);

  sym = lookup_symbol (copy.c_str (), block, VAR_DOMAIN, &is_a_field_of_this);

  if (sym.symbol)
    {
      yylval.ssym.sym = sym;
      yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;
    }

  return NAME;
}

/* Classify a NAME token.
   The contents of the token are in `yylval'.
   Updates yylval and returns the new token type.
   BLOCK is the block in which lookups start; this can be NULL
   to mean the global scope.

   The result is one of NAME, NAME_OR_INT, or TYPENAME.  */

static int
classify_name (struct parser_state *par_state, const struct block *block)
{
  struct type *type;
  struct block_symbol sym;
  struct field_of_this_result is_a_field_of_this;

  std::string copy = copy_name (yylval.sval);

  /* Try primitive types first so they win over bad/weird debug info.  */
  type = language_lookup_primitive_type (par_state->language (),
					 par_state->gdbarch (),
					 copy.c_str ());
  if (type != NULL)
    {
      /* NOTE: We take advantage of the fact that yylval coming in was a
	 NAME, and that struct ttype is a compatible extension of struct
	 stoken, so yylval.tsym.stoken is already filled in.  */
      yylval.tsym.type = type;
      return TYPENAME;
    }

  /* TODO: What about other types?  */

  sym = lookup_symbol (copy.c_str (), block, VAR_DOMAIN, &is_a_field_of_this);

  if (sym.symbol)
    {
      yylval.ssym.sym = sym;
      yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;
      return NAME;
    }

  /* If we didn't find a symbol, look again in the current package.
     This is to, e.g., make "p global_var" work without having to specify
     the package name.  We intentionally only looks for objects in the
     current package.  */

  {
    char *current_package_name = go_block_package_name (block);

    if (current_package_name != NULL)
      {
	struct stoken sval =
	  build_packaged_name (current_package_name,
			       strlen (current_package_name),
			       copy.c_str (), copy.size ());

	xfree (current_package_name);
	sym = lookup_symbol (sval.ptr, block, VAR_DOMAIN,
			     &is_a_field_of_this);
	if (sym.symbol)
	  {
	    yylval.ssym.stoken = sval;
	    yylval.ssym.sym = sym;
	    yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;
	    return NAME;
	  }
      }
  }

  /* Input names that aren't symbols but ARE valid hex numbers, when
     the input radix permits them, can be names or numbers depending
     on the parse.  Note we support radixes > 16 here.  */
  if ((copy[0] >= 'a' && copy[0] < 'a' + input_radix - 10)
      || (copy[0] >= 'A' && copy[0] < 'A' + input_radix - 10))
    {
      YYSTYPE newlval;	/* Its value is ignored.  */
      int hextype = parse_number (par_state, copy.c_str (),
				  yylval.sval.length, 0, &newlval);
      if (hextype == INT)
	{
	  yylval.ssym.sym.symbol = NULL;
	  yylval.ssym.sym.block = NULL;
	  yylval.ssym.is_a_field_of_this = 0;
	  return NAME_OR_INT;
	}
    }

  yylval.ssym.sym.symbol = NULL;
  yylval.ssym.sym.block = NULL;
  yylval.ssym.is_a_field_of_this = 0;
  return NAME;
}

/* This is taken from c-exp.y mostly to get something working.
   The basic structure has been kept because we may yet need some of it.  */

static int
yylex (void)
{
  token_and_value current, next;

  if (popping && !token_fifo.empty ())
    {
      token_and_value tv = token_fifo[0];
      token_fifo.erase (token_fifo.begin ());
      yylval = tv.value;
      /* There's no need to fall through to handle package.name
	 as that can never happen here.  In theory.  */
      return tv.token;
    }
  popping = 0;

  current.token = lex_one_token (pstate);

  /* TODO: Need a way to force specifying name1 as a package.
     .name1.name2 ?  */

  if (current.token != NAME)
    return current.token;

  /* See if we have "name1 . name2".  */

  current.value = yylval;
  next.token = lex_one_token (pstate);
  next.value = yylval;

  if (next.token == '.')
    {
      token_and_value name2;

      name2.token = lex_one_token (pstate);
      name2.value = yylval;

      if (name2.token == NAME)
	{
	  /* Ok, we have "name1 . name2".  */
	  std::string copy = copy_name (current.value.sval);

	  if (copy == "unsafe")
	    {
	      popping = 1;
	      return classify_unsafe_function (name2.value.sval);
	    }

	  if (package_name_p (copy.c_str (), pstate->expression_context_block))
	    {
	      popping = 1;
	      yylval.sval = build_packaged_name (current.value.sval.ptr,
						 current.value.sval.length,
						 name2.value.sval.ptr,
						 name2.value.sval.length);
	      return classify_packaged_name (pstate->expression_context_block);
	    }
	}

      token_fifo.push_back (next);
      token_fifo.push_back (name2);
    }
  else
    token_fifo.push_back (next);

  /* If we arrive here we don't have a package-qualified name.  */

  popping = 1;
  yylval = current.value;
  return classify_name (pstate, pstate->expression_context_block);
}

/* See language.h.  */

int
go_language::parser (struct parser_state *par_state) const
{
  /* Setting up the parser state.  */
  scoped_restore pstate_restore = make_scoped_restore (&pstate);
  gdb_assert (par_state != NULL);
  pstate = par_state;

  scoped_restore restore_yydebug = make_scoped_restore (&yydebug,
							parser_debug);

  /* Initialize some state used by the lexer.  */
  last_was_structop = 0;
  saw_name_at_eof = 0;
  paren_depth = 0;

  token_fifo.clear ();
  popping = 0;
  name_obstack.clear ();

  int result = yyparse ();
  if (!result)
    pstate->set_operation (pstate->pop ());
  return result;
}

static void
yyerror (const char *msg)
{
  if (pstate->prev_lexptr)
    pstate->lexptr = pstate->prev_lexptr;

  error (_("A %s in expression, near `%s'."), msg, pstate->lexptr);
}
