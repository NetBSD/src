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
#line 39 "d-exp.y"


#include "defs.h"
#include <ctype.h>
#include "expression.h"
#include "value.h"
#include "parser-defs.h"
#include "language.h"
#include "c-lang.h"
#include "d-lang.h"
#include "bfd.h" /* Required by objfiles.h.  */
#include "symfile.h" /* Required by objfiles.h.  */
#include "objfiles.h" /* For have_full_symbols and have_partial_symbols */
#include "charset.h"
#include "block.h"
#include "type-stack.h"
#include "expop.h"

#define parse_type(ps) builtin_type (ps->gdbarch ())
#define parse_d_type(ps) builtin_d_type (ps->gdbarch ())

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror,
   etc).  */
#define GDB_YY_REMAP_PREFIX d_
#include "yy-remap.h"

/* The state of the parser, used internally when we are parsing the
   expression.  */

static struct parser_state *pstate = NULL;

/* The current type stack.  */
static struct type_stack *type_stack;

int yyparse (void);

static int yylex (void);

static void yyerror (const char *);

static int type_aggregate_p (struct type *);

using namespace expr;


#line 117 "d-exp.c.tmp"

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
    IDENTIFIER = 258,              /* IDENTIFIER  */
    UNKNOWN_NAME = 259,            /* UNKNOWN_NAME  */
    TYPENAME = 260,                /* TYPENAME  */
    COMPLETE = 261,                /* COMPLETE  */
    NAME_OR_INT = 262,             /* NAME_OR_INT  */
    INTEGER_LITERAL = 263,         /* INTEGER_LITERAL  */
    FLOAT_LITERAL = 264,           /* FLOAT_LITERAL  */
    CHARACTER_LITERAL = 265,       /* CHARACTER_LITERAL  */
    STRING_LITERAL = 266,          /* STRING_LITERAL  */
    ENTRY = 267,                   /* ENTRY  */
    ERROR = 268,                   /* ERROR  */
    TRUE_KEYWORD = 269,            /* TRUE_KEYWORD  */
    FALSE_KEYWORD = 270,           /* FALSE_KEYWORD  */
    NULL_KEYWORD = 271,            /* NULL_KEYWORD  */
    SUPER_KEYWORD = 272,           /* SUPER_KEYWORD  */
    CAST_KEYWORD = 273,            /* CAST_KEYWORD  */
    SIZEOF_KEYWORD = 274,          /* SIZEOF_KEYWORD  */
    TYPEOF_KEYWORD = 275,          /* TYPEOF_KEYWORD  */
    TYPEID_KEYWORD = 276,          /* TYPEID_KEYWORD  */
    INIT_KEYWORD = 277,            /* INIT_KEYWORD  */
    IMMUTABLE_KEYWORD = 278,       /* IMMUTABLE_KEYWORD  */
    CONST_KEYWORD = 279,           /* CONST_KEYWORD  */
    SHARED_KEYWORD = 280,          /* SHARED_KEYWORD  */
    STRUCT_KEYWORD = 281,          /* STRUCT_KEYWORD  */
    UNION_KEYWORD = 282,           /* UNION_KEYWORD  */
    CLASS_KEYWORD = 283,           /* CLASS_KEYWORD  */
    INTERFACE_KEYWORD = 284,       /* INTERFACE_KEYWORD  */
    ENUM_KEYWORD = 285,            /* ENUM_KEYWORD  */
    TEMPLATE_KEYWORD = 286,        /* TEMPLATE_KEYWORD  */
    DELEGATE_KEYWORD = 287,        /* DELEGATE_KEYWORD  */
    FUNCTION_KEYWORD = 288,        /* FUNCTION_KEYWORD  */
    DOLLAR_VARIABLE = 289,         /* DOLLAR_VARIABLE  */
    ASSIGN_MODIFY = 290,           /* ASSIGN_MODIFY  */
    OROR = 291,                    /* OROR  */
    ANDAND = 292,                  /* ANDAND  */
    EQUAL = 293,                   /* EQUAL  */
    NOTEQUAL = 294,                /* NOTEQUAL  */
    LEQ = 295,                     /* LEQ  */
    GEQ = 296,                     /* GEQ  */
    LSH = 297,                     /* LSH  */
    RSH = 298,                     /* RSH  */
    HATHAT = 299,                  /* HATHAT  */
    IDENTITY = 300,                /* IDENTITY  */
    NOTIDENTITY = 301,             /* NOTIDENTITY  */
    INCREMENT = 302,               /* INCREMENT  */
    DECREMENT = 303,               /* DECREMENT  */
    DOTDOT = 304                   /* DOTDOT  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define IDENTIFIER 258
#define UNKNOWN_NAME 259
#define TYPENAME 260
#define COMPLETE 261
#define NAME_OR_INT 262
#define INTEGER_LITERAL 263
#define FLOAT_LITERAL 264
#define CHARACTER_LITERAL 265
#define STRING_LITERAL 266
#define ENTRY 267
#define ERROR 268
#define TRUE_KEYWORD 269
#define FALSE_KEYWORD 270
#define NULL_KEYWORD 271
#define SUPER_KEYWORD 272
#define CAST_KEYWORD 273
#define SIZEOF_KEYWORD 274
#define TYPEOF_KEYWORD 275
#define TYPEID_KEYWORD 276
#define INIT_KEYWORD 277
#define IMMUTABLE_KEYWORD 278
#define CONST_KEYWORD 279
#define SHARED_KEYWORD 280
#define STRUCT_KEYWORD 281
#define UNION_KEYWORD 282
#define CLASS_KEYWORD 283
#define INTERFACE_KEYWORD 284
#define ENUM_KEYWORD 285
#define TEMPLATE_KEYWORD 286
#define DELEGATE_KEYWORD 287
#define FUNCTION_KEYWORD 288
#define DOLLAR_VARIABLE 289
#define ASSIGN_MODIFY 290
#define OROR 291
#define ANDAND 292
#define EQUAL 293
#define NOTEQUAL 294
#define LEQ 295
#define GEQ 296
#define LSH 297
#define RSH 298
#define HATHAT 299
#define IDENTITY 300
#define NOTIDENTITY 301
#define INCREMENT 302
#define DECREMENT 303
#define DOTDOT 304

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 90 "d-exp.y"

    struct {
      LONGEST val;
      struct type *type;
    } typed_val_int;
    struct {
      gdb_byte val[16];
      struct type *type;
    } typed_val_float;
    struct symbol *sym;
    struct type *tval;
    struct typed_stoken tsval;
    struct stoken sval;
    struct ttype tsym;
    struct symtoken ssym;
    int ival;
    int voidval;
    enum exp_opcode opcode;
    struct stoken_vector svec;
  

#line 287 "d-exp.c.tmp"

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
  YYSYMBOL_IDENTIFIER = 3,                 /* IDENTIFIER  */
  YYSYMBOL_UNKNOWN_NAME = 4,               /* UNKNOWN_NAME  */
  YYSYMBOL_TYPENAME = 5,                   /* TYPENAME  */
  YYSYMBOL_COMPLETE = 6,                   /* COMPLETE  */
  YYSYMBOL_NAME_OR_INT = 7,                /* NAME_OR_INT  */
  YYSYMBOL_INTEGER_LITERAL = 8,            /* INTEGER_LITERAL  */
  YYSYMBOL_FLOAT_LITERAL = 9,              /* FLOAT_LITERAL  */
  YYSYMBOL_CHARACTER_LITERAL = 10,         /* CHARACTER_LITERAL  */
  YYSYMBOL_STRING_LITERAL = 11,            /* STRING_LITERAL  */
  YYSYMBOL_ENTRY = 12,                     /* ENTRY  */
  YYSYMBOL_ERROR = 13,                     /* ERROR  */
  YYSYMBOL_TRUE_KEYWORD = 14,              /* TRUE_KEYWORD  */
  YYSYMBOL_FALSE_KEYWORD = 15,             /* FALSE_KEYWORD  */
  YYSYMBOL_NULL_KEYWORD = 16,              /* NULL_KEYWORD  */
  YYSYMBOL_SUPER_KEYWORD = 17,             /* SUPER_KEYWORD  */
  YYSYMBOL_CAST_KEYWORD = 18,              /* CAST_KEYWORD  */
  YYSYMBOL_SIZEOF_KEYWORD = 19,            /* SIZEOF_KEYWORD  */
  YYSYMBOL_TYPEOF_KEYWORD = 20,            /* TYPEOF_KEYWORD  */
  YYSYMBOL_TYPEID_KEYWORD = 21,            /* TYPEID_KEYWORD  */
  YYSYMBOL_INIT_KEYWORD = 22,              /* INIT_KEYWORD  */
  YYSYMBOL_IMMUTABLE_KEYWORD = 23,         /* IMMUTABLE_KEYWORD  */
  YYSYMBOL_CONST_KEYWORD = 24,             /* CONST_KEYWORD  */
  YYSYMBOL_SHARED_KEYWORD = 25,            /* SHARED_KEYWORD  */
  YYSYMBOL_STRUCT_KEYWORD = 26,            /* STRUCT_KEYWORD  */
  YYSYMBOL_UNION_KEYWORD = 27,             /* UNION_KEYWORD  */
  YYSYMBOL_CLASS_KEYWORD = 28,             /* CLASS_KEYWORD  */
  YYSYMBOL_INTERFACE_KEYWORD = 29,         /* INTERFACE_KEYWORD  */
  YYSYMBOL_ENUM_KEYWORD = 30,              /* ENUM_KEYWORD  */
  YYSYMBOL_TEMPLATE_KEYWORD = 31,          /* TEMPLATE_KEYWORD  */
  YYSYMBOL_DELEGATE_KEYWORD = 32,          /* DELEGATE_KEYWORD  */
  YYSYMBOL_FUNCTION_KEYWORD = 33,          /* FUNCTION_KEYWORD  */
  YYSYMBOL_DOLLAR_VARIABLE = 34,           /* DOLLAR_VARIABLE  */
  YYSYMBOL_ASSIGN_MODIFY = 35,             /* ASSIGN_MODIFY  */
  YYSYMBOL_36_ = 36,                       /* ','  */
  YYSYMBOL_37_ = 37,                       /* '='  */
  YYSYMBOL_38_ = 38,                       /* '?'  */
  YYSYMBOL_OROR = 39,                      /* OROR  */
  YYSYMBOL_ANDAND = 40,                    /* ANDAND  */
  YYSYMBOL_41_ = 41,                       /* '|'  */
  YYSYMBOL_42_ = 42,                       /* '^'  */
  YYSYMBOL_43_ = 43,                       /* '&'  */
  YYSYMBOL_EQUAL = 44,                     /* EQUAL  */
  YYSYMBOL_NOTEQUAL = 45,                  /* NOTEQUAL  */
  YYSYMBOL_46_ = 46,                       /* '<'  */
  YYSYMBOL_47_ = 47,                       /* '>'  */
  YYSYMBOL_LEQ = 48,                       /* LEQ  */
  YYSYMBOL_GEQ = 49,                       /* GEQ  */
  YYSYMBOL_LSH = 50,                       /* LSH  */
  YYSYMBOL_RSH = 51,                       /* RSH  */
  YYSYMBOL_52_ = 52,                       /* '+'  */
  YYSYMBOL_53_ = 53,                       /* '-'  */
  YYSYMBOL_54_ = 54,                       /* '*'  */
  YYSYMBOL_55_ = 55,                       /* '/'  */
  YYSYMBOL_56_ = 56,                       /* '%'  */
  YYSYMBOL_HATHAT = 57,                    /* HATHAT  */
  YYSYMBOL_IDENTITY = 58,                  /* IDENTITY  */
  YYSYMBOL_NOTIDENTITY = 59,               /* NOTIDENTITY  */
  YYSYMBOL_INCREMENT = 60,                 /* INCREMENT  */
  YYSYMBOL_DECREMENT = 61,                 /* DECREMENT  */
  YYSYMBOL_62_ = 62,                       /* '.'  */
  YYSYMBOL_63_ = 63,                       /* '['  */
  YYSYMBOL_64_ = 64,                       /* '('  */
  YYSYMBOL_DOTDOT = 65,                    /* DOTDOT  */
  YYSYMBOL_66_ = 66,                       /* ':'  */
  YYSYMBOL_67_ = 67,                       /* '~'  */
  YYSYMBOL_68_ = 68,                       /* '!'  */
  YYSYMBOL_69_ = 69,                       /* ')'  */
  YYSYMBOL_70_ = 70,                       /* ']'  */
  YYSYMBOL_YYACCEPT = 71,                  /* $accept  */
  YYSYMBOL_start = 72,                     /* start  */
  YYSYMBOL_Expression = 73,                /* Expression  */
  YYSYMBOL_CommaExpression = 74,           /* CommaExpression  */
  YYSYMBOL_AssignExpression = 75,          /* AssignExpression  */
  YYSYMBOL_ConditionalExpression = 76,     /* ConditionalExpression  */
  YYSYMBOL_OrOrExpression = 77,            /* OrOrExpression  */
  YYSYMBOL_AndAndExpression = 78,          /* AndAndExpression  */
  YYSYMBOL_OrExpression = 79,              /* OrExpression  */
  YYSYMBOL_XorExpression = 80,             /* XorExpression  */
  YYSYMBOL_AndExpression = 81,             /* AndExpression  */
  YYSYMBOL_CmpExpression = 82,             /* CmpExpression  */
  YYSYMBOL_EqualExpression = 83,           /* EqualExpression  */
  YYSYMBOL_IdentityExpression = 84,        /* IdentityExpression  */
  YYSYMBOL_RelExpression = 85,             /* RelExpression  */
  YYSYMBOL_ShiftExpression = 86,           /* ShiftExpression  */
  YYSYMBOL_AddExpression = 87,             /* AddExpression  */
  YYSYMBOL_MulExpression = 88,             /* MulExpression  */
  YYSYMBOL_UnaryExpression = 89,           /* UnaryExpression  */
  YYSYMBOL_CastExpression = 90,            /* CastExpression  */
  YYSYMBOL_PowExpression = 91,             /* PowExpression  */
  YYSYMBOL_PostfixExpression = 92,         /* PostfixExpression  */
  YYSYMBOL_ArgumentList = 93,              /* ArgumentList  */
  YYSYMBOL_ArgumentList_opt = 94,          /* ArgumentList_opt  */
  YYSYMBOL_CallExpression = 95,            /* CallExpression  */
  YYSYMBOL_96_1 = 96,                      /* $@1  */
  YYSYMBOL_IndexExpression = 97,           /* IndexExpression  */
  YYSYMBOL_SliceExpression = 98,           /* SliceExpression  */
  YYSYMBOL_PrimaryExpression = 99,         /* PrimaryExpression  */
  YYSYMBOL_ArrayLiteral = 100,             /* ArrayLiteral  */
  YYSYMBOL_IdentifierExp = 101,            /* IdentifierExp  */
  YYSYMBOL_StringExp = 102,                /* StringExp  */
  YYSYMBOL_TypeExp = 103,                  /* TypeExp  */
  YYSYMBOL_BasicType2 = 104,               /* BasicType2  */
  YYSYMBOL_BasicType = 105                 /* BasicType  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;


/* Second part of user prologue.  */
#line 111 "d-exp.y"

/* YYSTYPE gets defined by %union */
static int parse_number (struct parser_state *, const char *,
			 int, int, YYSTYPE *);

#line 424 "d-exp.c.tmp"


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
#define YYFINAL  70
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   200

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  71
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  35
/* YYNRULES -- Number of rules.  */
#define YYNRULES  104
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  169

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   304


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
       2,     2,     2,    68,     2,     2,     2,    56,    43,     2,
      64,    69,    54,    52,    36,    53,    62,    55,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    66,     2,
      46,    37,    47,    38,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    63,     2,    70,    42,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    41,     2,    67,     2,     2,     2,
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
      35,    39,    40,    44,    45,    48,    49,    50,    51,    57,
      58,    59,    60,    61,    65
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   184,   184,   185,   191,   195,   196,   201,   202,   204,
     214,   215,   227,   228,   233,   234,   239,   240,   245,   246,
     251,   252,   257,   258,   259,   260,   264,   266,   271,   273,
     278,   280,   282,   284,   289,   290,   292,   297,   298,   300,
     302,   307,   308,   310,   312,   316,   318,   320,   322,   324,
     326,   328,   330,   332,   334,   335,   339,   343,   348,   349,
     354,   355,   362,   367,   374,   376,   378,   380,   381,   382,
     386,   388,   394,   395,   400,   399,   411,   425,   427,   439,
     441,   480,   514,   516,   521,   525,   527,   529,   531,   538,
     543,   549,   556,   561,   566,   570,   585,   603,   605,   607,
     614,   616,   618,   621,   627
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
  "\"end of file\"", "error", "\"invalid token\"", "IDENTIFIER",
  "UNKNOWN_NAME", "TYPENAME", "COMPLETE", "NAME_OR_INT", "INTEGER_LITERAL",
  "FLOAT_LITERAL", "CHARACTER_LITERAL", "STRING_LITERAL", "ENTRY", "ERROR",
  "TRUE_KEYWORD", "FALSE_KEYWORD", "NULL_KEYWORD", "SUPER_KEYWORD",
  "CAST_KEYWORD", "SIZEOF_KEYWORD", "TYPEOF_KEYWORD", "TYPEID_KEYWORD",
  "INIT_KEYWORD", "IMMUTABLE_KEYWORD", "CONST_KEYWORD", "SHARED_KEYWORD",
  "STRUCT_KEYWORD", "UNION_KEYWORD", "CLASS_KEYWORD", "INTERFACE_KEYWORD",
  "ENUM_KEYWORD", "TEMPLATE_KEYWORD", "DELEGATE_KEYWORD",
  "FUNCTION_KEYWORD", "DOLLAR_VARIABLE", "ASSIGN_MODIFY", "','", "'='",
  "'?'", "OROR", "ANDAND", "'|'", "'^'", "'&'", "EQUAL", "NOTEQUAL", "'<'",
  "'>'", "LEQ", "GEQ", "LSH", "RSH", "'+'", "'-'", "'*'", "'/'", "'%'",
  "HATHAT", "IDENTITY", "NOTIDENTITY", "INCREMENT", "DECREMENT", "'.'",
  "'['", "'('", "DOTDOT", "':'", "'~'", "'!'", "')'", "']'", "$accept",
  "start", "Expression", "CommaExpression", "AssignExpression",
  "ConditionalExpression", "OrOrExpression", "AndAndExpression",
  "OrExpression", "XorExpression", "AndExpression", "CmpExpression",
  "EqualExpression", "IdentityExpression", "RelExpression",
  "ShiftExpression", "AddExpression", "MulExpression", "UnaryExpression",
  "CastExpression", "PowExpression", "PostfixExpression", "ArgumentList",
  "ArgumentList_opt", "CallExpression", "$@1", "IndexExpression",
  "SliceExpression", "PrimaryExpression", "ArrayLiteral", "IdentifierExp",
  "StringExp", "TypeExp", "BasicType2", "BasicType", YY_NULLPTRPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-91)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     132,   -91,   -91,   -91,   -91,   -91,   -91,   -91,   -91,   -91,
     -91,   -43,   -27,   -91,   132,   132,   132,   132,   132,   132,
     132,   132,   132,   132,    30,   -91,   -91,    17,    -8,     0,
      14,    21,    35,    12,   -91,   -91,   -91,   -91,    -1,   -34,
     -31,   -91,   -91,   -91,    43,   -91,   -91,   -91,   -91,   -91,
     -91,    72,    22,   -37,     6,   132,   -91,    22,   -91,   -91,
     -91,   -91,   -91,   -91,    49,    16,    19,   -49,   -91,   -91,
     -91,   132,   132,   132,   132,   132,   132,   132,   132,   132,
     132,   132,   132,   132,   132,   132,   132,   132,   132,   132,
     132,   132,   132,   132,   132,   132,   132,   -91,   -91,     9,
      58,   -91,   -91,    13,   -37,    81,   -91,     6,    24,    25,
     132,   -91,   -91,   132,   -91,   -91,   -91,    29,    14,    21,
      35,    12,   -91,    -9,    -9,    -9,    -9,    -9,    -9,   -34,
     -34,    -9,    -9,   -31,   -31,   -31,   -91,   -91,   -91,   -91,
      84,   -91,   -91,   -91,    26,   -30,   132,   -91,   -91,   -91,
      28,    27,   132,   -91,   -91,   -91,   132,   -91,   132,   -91,
      33,   -37,   -91,   -91,   -91,    38,   -91,   -91,   -91
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,    94,   104,    83,    87,    88,    89,    95,    85,    86,
      84,     0,     0,    82,     0,     0,     0,     0,     0,     0,
      72,     0,     0,     0,     0,     2,     4,     5,     7,    10,
      12,    14,    16,    18,    20,    23,    24,    25,    22,    34,
      37,    41,    54,    55,    58,    67,    68,    69,    60,    91,
      80,    90,     3,    98,     0,     0,    45,     0,    50,    49,
      48,    46,    47,    70,    73,     0,     0,     0,    52,    51,
       1,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    65,    66,     0,
       0,    74,    96,     0,   100,     0,    99,     0,     0,     0,
       0,    93,    79,    97,     6,     9,     8,     0,    13,    15,
      17,    19,    21,    26,    27,    30,    32,    31,    33,    35,
      36,    28,    29,    38,    39,    40,    42,    43,    44,    59,
      62,    61,    64,    77,    70,     0,    72,    53,    81,   101,
       0,     0,     0,    92,    71,    57,     0,    63,     0,    76,
       0,   102,    97,    56,    11,     0,    75,   103,    78
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -91,   -91,     1,    42,   -13,   -47,   -91,    39,    40,    46,
      37,    41,   -91,   -91,   -91,    73,   -35,   -56,   -14,   -91,
     -91,   -91,    31,   -22,   -91,   -91,   -91,   -91,   -91,   -91,
      48,   -91,    10,   -90,   -91
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    64,    65,    45,   146,    46,    47,    48,    49,
      50,    51,    57,   106,    53
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      56,    58,    59,    60,    61,    62,   110,    63,    68,    69,
      52,     2,   140,   103,   149,   141,     1,   104,    90,    91,
     113,    54,    66,    93,    94,    95,   105,    72,   142,    73,
      70,    67,   147,    92,   133,   134,   135,    55,    74,    75,
     159,    86,    87,    80,    81,    82,    83,    84,    85,    86,
      87,   129,   130,    71,    76,    79,   109,    88,    89,   115,
     116,     1,    77,     2,   108,     3,     4,     5,     6,     7,
     107,   167,     8,     9,    10,   117,    11,    78,    12,   136,
     137,   138,   139,   102,   103,   110,   111,   144,   112,   150,
     157,   158,    13,   152,   153,   156,   162,   154,   161,   155,
      96,    14,   166,    97,    98,    99,   100,   101,   168,   164,
      15,    16,    17,   114,   118,   121,   119,   151,    18,    19,
     122,    20,    21,   120,   160,    22,    23,     0,   143,     0,
       0,   145,     0,    63,     0,     1,     0,     2,   163,     3,
       4,     5,     6,     7,     0,   165,     8,     9,    10,     0,
      11,   148,    12,   123,   124,   125,   126,   127,   128,     0,
       0,   131,   132,     0,     0,     0,    13,     0,     0,     0,
       0,     0,     0,     0,     0,    14,     0,     0,     0,     0,
       0,     0,     0,     0,    15,    16,    17,     0,     0,     0,
       0,     0,    18,    19,     0,    20,    21,     0,     0,    22,
      23
};

static const yytype_int16 yycheck[] =
{
      14,    15,    16,    17,    18,    19,    36,    20,    22,    23,
       0,     5,     3,    62,   104,     6,     3,    54,    52,    53,
      69,    64,    21,    54,    55,    56,    63,    35,    19,    37,
       0,    21,    19,    67,    90,    91,    92,    64,    38,    39,
      70,    50,    51,    44,    45,    46,    47,    48,    49,    50,
      51,    86,    87,    36,    40,    43,    55,    58,    59,    72,
      73,     3,    41,     5,    54,     7,     8,     9,    10,    11,
      64,   161,    14,    15,    16,    74,    18,    42,    20,    93,
      94,    95,    96,    11,    62,    36,    70,   100,    69,     8,
       6,    65,    34,    69,    69,    66,    69,   110,    70,   113,
      57,    43,    69,    60,    61,    62,    63,    64,    70,   156,
      52,    53,    54,    71,    75,    78,    76,   107,    60,    61,
      79,    63,    64,    77,   146,    67,    68,    -1,    70,    -1,
      -1,   100,    -1,   146,    -1,     3,    -1,     5,   152,     7,
       8,     9,    10,    11,    -1,   158,    14,    15,    16,    -1,
      18,   103,    20,    80,    81,    82,    83,    84,    85,    -1,
      -1,    88,    89,    -1,    -1,    -1,    34,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    43,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    52,    53,    54,    -1,    -1,    -1,
      -1,    -1,    60,    61,    -1,    63,    64,    -1,    -1,    67,
      68
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     5,     7,     8,     9,    10,    11,    14,    15,
      16,    18,    20,    34,    43,    52,    53,    54,    60,    61,
      63,    64,    67,    68,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    95,    97,    98,    99,   100,
     101,   102,   103,   105,    64,    64,    89,   103,    89,    89,
      89,    89,    89,    75,    93,    94,    73,   103,    89,    89,
       0,    36,    35,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    58,    59,
      52,    53,    67,    54,    55,    56,    57,    60,    61,    62,
      63,    64,    11,    62,    54,    63,   104,    64,   103,    73,
      36,    70,    69,    69,    74,    75,    75,    73,    78,    79,
      80,    81,    82,    86,    86,    86,    86,    86,    86,    87,
      87,    86,    86,    88,    88,    88,    89,    89,    89,    89,
       3,     6,    19,    70,    75,    93,    96,    19,   101,   104,
       8,   103,    69,    69,    75,    89,    66,     6,    65,    70,
      94,    70,    69,    89,    76,    75,    69,   104,    70
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    71,    72,    72,    73,    74,    74,    75,    75,    75,
      76,    76,    77,    77,    78,    78,    79,    79,    80,    80,
      81,    81,    82,    82,    82,    82,    83,    83,    84,    84,
      85,    85,    85,    85,    86,    86,    86,    87,    87,    87,
      87,    88,    88,    88,    88,    89,    89,    89,    89,    89,
      89,    89,    89,    89,    89,    89,    90,    90,    91,    91,
      92,    92,    92,    92,    92,    92,    92,    92,    92,    92,
      93,    93,    94,    94,    96,    95,    97,    98,    98,    99,
      99,    99,    99,    99,    99,    99,    99,    99,    99,    99,
      99,    99,    99,   100,   101,   102,   102,   103,   103,   103,
     104,   104,   104,   104,   105
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     3,     1,     3,     3,
       1,     5,     1,     3,     1,     3,     1,     3,     1,     3,
       1,     3,     1,     1,     1,     1,     3,     3,     3,     3,
       3,     3,     3,     3,     1,     3,     3,     1,     3,     3,
       3,     1,     3,     3,     3,     2,     2,     2,     2,     2,
       2,     2,     2,     3,     1,     1,     5,     4,     1,     3,
       1,     3,     3,     4,     3,     2,     2,     1,     1,     1,
       1,     3,     0,     1,     0,     5,     4,     3,     6,     3,
       1,     3,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     4,     3,     1,     1,     2,     3,     1,     2,
       1,     2,     3,     4,     1
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
  case 6: /* CommaExpression: AssignExpression ',' CommaExpression  */
#line 197 "d-exp.y"
                { pstate->wrap2<comma_operation> (); }
#line 1514 "d-exp.c.tmp"
    break;

  case 8: /* AssignExpression: ConditionalExpression '=' AssignExpression  */
#line 203 "d-exp.y"
                { pstate->wrap2<assign_operation> (); }
#line 1520 "d-exp.c.tmp"
    break;

  case 9: /* AssignExpression: ConditionalExpression ASSIGN_MODIFY AssignExpression  */
#line 205 "d-exp.y"
                {
		  operation_up rhs = pstate->pop ();
		  operation_up lhs = pstate->pop ();
		  pstate->push_new<assign_modify_operation>
		    ((yyvsp[-1].opcode), std::move (lhs), std::move (rhs));
		}
#line 1531 "d-exp.c.tmp"
    break;

  case 11: /* ConditionalExpression: OrOrExpression '?' Expression ':' ConditionalExpression  */
#line 216 "d-exp.y"
                {
		  operation_up last = pstate->pop ();
		  operation_up mid = pstate->pop ();
		  operation_up first = pstate->pop ();
		  pstate->push_new<ternop_cond_operation>
		    (std::move (first), std::move (mid),
		     std::move (last));
		}
#line 1544 "d-exp.c.tmp"
    break;

  case 13: /* OrOrExpression: OrOrExpression OROR AndAndExpression  */
#line 229 "d-exp.y"
                { pstate->wrap2<logical_or_operation> (); }
#line 1550 "d-exp.c.tmp"
    break;

  case 15: /* AndAndExpression: AndAndExpression ANDAND OrExpression  */
#line 235 "d-exp.y"
                { pstate->wrap2<logical_and_operation> (); }
#line 1556 "d-exp.c.tmp"
    break;

  case 17: /* OrExpression: OrExpression '|' XorExpression  */
#line 241 "d-exp.y"
                { pstate->wrap2<bitwise_ior_operation> (); }
#line 1562 "d-exp.c.tmp"
    break;

  case 19: /* XorExpression: XorExpression '^' AndExpression  */
#line 247 "d-exp.y"
                { pstate->wrap2<bitwise_xor_operation> (); }
#line 1568 "d-exp.c.tmp"
    break;

  case 21: /* AndExpression: AndExpression '&' CmpExpression  */
#line 253 "d-exp.y"
                { pstate->wrap2<bitwise_and_operation> (); }
#line 1574 "d-exp.c.tmp"
    break;

  case 26: /* EqualExpression: ShiftExpression EQUAL ShiftExpression  */
#line 265 "d-exp.y"
                { pstate->wrap2<equal_operation> (); }
#line 1580 "d-exp.c.tmp"
    break;

  case 27: /* EqualExpression: ShiftExpression NOTEQUAL ShiftExpression  */
#line 267 "d-exp.y"
                { pstate->wrap2<notequal_operation> (); }
#line 1586 "d-exp.c.tmp"
    break;

  case 28: /* IdentityExpression: ShiftExpression IDENTITY ShiftExpression  */
#line 272 "d-exp.y"
                { pstate->wrap2<equal_operation> (); }
#line 1592 "d-exp.c.tmp"
    break;

  case 29: /* IdentityExpression: ShiftExpression NOTIDENTITY ShiftExpression  */
#line 274 "d-exp.y"
                { pstate->wrap2<notequal_operation> (); }
#line 1598 "d-exp.c.tmp"
    break;

  case 30: /* RelExpression: ShiftExpression '<' ShiftExpression  */
#line 279 "d-exp.y"
                { pstate->wrap2<less_operation> (); }
#line 1604 "d-exp.c.tmp"
    break;

  case 31: /* RelExpression: ShiftExpression LEQ ShiftExpression  */
#line 281 "d-exp.y"
                { pstate->wrap2<leq_operation> (); }
#line 1610 "d-exp.c.tmp"
    break;

  case 32: /* RelExpression: ShiftExpression '>' ShiftExpression  */
#line 283 "d-exp.y"
                { pstate->wrap2<gtr_operation> (); }
#line 1616 "d-exp.c.tmp"
    break;

  case 33: /* RelExpression: ShiftExpression GEQ ShiftExpression  */
#line 285 "d-exp.y"
                { pstate->wrap2<geq_operation> (); }
#line 1622 "d-exp.c.tmp"
    break;

  case 35: /* ShiftExpression: ShiftExpression LSH AddExpression  */
#line 291 "d-exp.y"
                { pstate->wrap2<lsh_operation> (); }
#line 1628 "d-exp.c.tmp"
    break;

  case 36: /* ShiftExpression: ShiftExpression RSH AddExpression  */
#line 293 "d-exp.y"
                { pstate->wrap2<rsh_operation> (); }
#line 1634 "d-exp.c.tmp"
    break;

  case 38: /* AddExpression: AddExpression '+' MulExpression  */
#line 299 "d-exp.y"
                { pstate->wrap2<add_operation> (); }
#line 1640 "d-exp.c.tmp"
    break;

  case 39: /* AddExpression: AddExpression '-' MulExpression  */
#line 301 "d-exp.y"
                { pstate->wrap2<sub_operation> (); }
#line 1646 "d-exp.c.tmp"
    break;

  case 40: /* AddExpression: AddExpression '~' MulExpression  */
#line 303 "d-exp.y"
                { pstate->wrap2<concat_operation> (); }
#line 1652 "d-exp.c.tmp"
    break;

  case 42: /* MulExpression: MulExpression '*' UnaryExpression  */
#line 309 "d-exp.y"
                { pstate->wrap2<mul_operation> (); }
#line 1658 "d-exp.c.tmp"
    break;

  case 43: /* MulExpression: MulExpression '/' UnaryExpression  */
#line 311 "d-exp.y"
                { pstate->wrap2<div_operation> (); }
#line 1664 "d-exp.c.tmp"
    break;

  case 44: /* MulExpression: MulExpression '%' UnaryExpression  */
#line 313 "d-exp.y"
                { pstate->wrap2<rem_operation> (); }
#line 1670 "d-exp.c.tmp"
    break;

  case 45: /* UnaryExpression: '&' UnaryExpression  */
#line 317 "d-exp.y"
                { pstate->wrap<unop_addr_operation> (); }
#line 1676 "d-exp.c.tmp"
    break;

  case 46: /* UnaryExpression: INCREMENT UnaryExpression  */
#line 319 "d-exp.y"
                { pstate->wrap<preinc_operation> (); }
#line 1682 "d-exp.c.tmp"
    break;

  case 47: /* UnaryExpression: DECREMENT UnaryExpression  */
#line 321 "d-exp.y"
                { pstate->wrap<predec_operation> (); }
#line 1688 "d-exp.c.tmp"
    break;

  case 48: /* UnaryExpression: '*' UnaryExpression  */
#line 323 "d-exp.y"
                { pstate->wrap<unop_ind_operation> (); }
#line 1694 "d-exp.c.tmp"
    break;

  case 49: /* UnaryExpression: '-' UnaryExpression  */
#line 325 "d-exp.y"
                { pstate->wrap<unary_neg_operation> (); }
#line 1700 "d-exp.c.tmp"
    break;

  case 50: /* UnaryExpression: '+' UnaryExpression  */
#line 327 "d-exp.y"
                { pstate->wrap<unary_plus_operation> (); }
#line 1706 "d-exp.c.tmp"
    break;

  case 51: /* UnaryExpression: '!' UnaryExpression  */
#line 329 "d-exp.y"
                { pstate->wrap<unary_logical_not_operation> (); }
#line 1712 "d-exp.c.tmp"
    break;

  case 52: /* UnaryExpression: '~' UnaryExpression  */
#line 331 "d-exp.y"
                { pstate->wrap<unary_complement_operation> (); }
#line 1718 "d-exp.c.tmp"
    break;

  case 53: /* UnaryExpression: TypeExp '.' SIZEOF_KEYWORD  */
#line 333 "d-exp.y"
                { pstate->wrap<unop_sizeof_operation> (); }
#line 1724 "d-exp.c.tmp"
    break;

  case 56: /* CastExpression: CAST_KEYWORD '(' TypeExp ')' UnaryExpression  */
#line 340 "d-exp.y"
                { pstate->wrap2<unop_cast_type_operation> (); }
#line 1730 "d-exp.c.tmp"
    break;

  case 57: /* CastExpression: '(' TypeExp ')' UnaryExpression  */
#line 344 "d-exp.y"
                { pstate->wrap2<unop_cast_type_operation> (); }
#line 1736 "d-exp.c.tmp"
    break;

  case 59: /* PowExpression: PostfixExpression HATHAT UnaryExpression  */
#line 350 "d-exp.y"
                { pstate->wrap2<exp_operation> (); }
#line 1742 "d-exp.c.tmp"
    break;

  case 61: /* PostfixExpression: PostfixExpression '.' COMPLETE  */
#line 356 "d-exp.y"
                {
		  structop_base_operation *op
		    = new structop_ptr_operation (pstate->pop (), "");
		  pstate->mark_struct_expression (op);
		  pstate->push (operation_up (op));
		}
#line 1753 "d-exp.c.tmp"
    break;

  case 62: /* PostfixExpression: PostfixExpression '.' IDENTIFIER  */
#line 363 "d-exp.y"
                {
		  pstate->push_new<structop_operation>
		    (pstate->pop (), copy_name ((yyvsp[0].sval)));
		}
#line 1762 "d-exp.c.tmp"
    break;

  case 63: /* PostfixExpression: PostfixExpression '.' IDENTIFIER COMPLETE  */
#line 368 "d-exp.y"
                {
		  structop_base_operation *op
		    = new structop_operation (pstate->pop (), copy_name ((yyvsp[-1].sval)));
		  pstate->mark_struct_expression (op);
		  pstate->push (operation_up (op));
		}
#line 1773 "d-exp.c.tmp"
    break;

  case 64: /* PostfixExpression: PostfixExpression '.' SIZEOF_KEYWORD  */
#line 375 "d-exp.y"
                { pstate->wrap<unop_sizeof_operation> (); }
#line 1779 "d-exp.c.tmp"
    break;

  case 65: /* PostfixExpression: PostfixExpression INCREMENT  */
#line 377 "d-exp.y"
                { pstate->wrap<postinc_operation> (); }
#line 1785 "d-exp.c.tmp"
    break;

  case 66: /* PostfixExpression: PostfixExpression DECREMENT  */
#line 379 "d-exp.y"
                { pstate->wrap<postdec_operation> (); }
#line 1791 "d-exp.c.tmp"
    break;

  case 70: /* ArgumentList: AssignExpression  */
#line 387 "d-exp.y"
                { pstate->arglist_len = 1; }
#line 1797 "d-exp.c.tmp"
    break;

  case 71: /* ArgumentList: ArgumentList ',' AssignExpression  */
#line 389 "d-exp.y"
                { pstate->arglist_len++; }
#line 1803 "d-exp.c.tmp"
    break;

  case 72: /* ArgumentList_opt: %empty  */
#line 394 "d-exp.y"
                { pstate->arglist_len = 0; }
#line 1809 "d-exp.c.tmp"
    break;

  case 74: /* $@1: %empty  */
#line 400 "d-exp.y"
                { pstate->start_arglist (); }
#line 1815 "d-exp.c.tmp"
    break;

  case 75: /* CallExpression: PostfixExpression '(' $@1 ArgumentList_opt ')'  */
#line 402 "d-exp.y"
                {
		  std::vector<operation_up> args
		    = pstate->pop_vector (pstate->end_arglist ());
		  pstate->push_new<funcall_operation>
		    (pstate->pop (), std::move (args));
		}
#line 1826 "d-exp.c.tmp"
    break;

  case 76: /* IndexExpression: PostfixExpression '[' ArgumentList ']'  */
#line 412 "d-exp.y"
                { if (pstate->arglist_len > 0)
		    {
		      std::vector<operation_up> args
			= pstate->pop_vector (pstate->arglist_len);
		      pstate->push_new<multi_subscript_operation>
			(pstate->pop (), std::move (args));
		    }
		  else
		    pstate->wrap2<subscript_operation> ();
		}
#line 1841 "d-exp.c.tmp"
    break;

  case 77: /* SliceExpression: PostfixExpression '[' ']'  */
#line 426 "d-exp.y"
                { /* Do nothing.  */ }
#line 1847 "d-exp.c.tmp"
    break;

  case 78: /* SliceExpression: PostfixExpression '[' AssignExpression DOTDOT AssignExpression ']'  */
#line 428 "d-exp.y"
                {
		  operation_up last = pstate->pop ();
		  operation_up mid = pstate->pop ();
		  operation_up first = pstate->pop ();
		  pstate->push_new<ternop_slice_operation>
		    (std::move (first), std::move (mid),
		     std::move (last));
		}
#line 1860 "d-exp.c.tmp"
    break;

  case 79: /* PrimaryExpression: '(' Expression ')'  */
#line 440 "d-exp.y"
                { /* Do nothing.  */ }
#line 1866 "d-exp.c.tmp"
    break;

  case 80: /* PrimaryExpression: IdentifierExp  */
#line 442 "d-exp.y"
                { struct bound_minimal_symbol msymbol;
		  std::string copy = copy_name ((yyvsp[0].sval));
		  struct field_of_this_result is_a_field_of_this;
		  struct block_symbol sym;

		  /* Handle VAR, which could be local or global.  */
		  sym = lookup_symbol (copy.c_str (),
				       pstate->expression_context_block,
				       VAR_DOMAIN, &is_a_field_of_this);
		  if (sym.symbol && sym.symbol->aclass () != LOC_TYPEDEF)
		    {
		      if (symbol_read_needs_frame (sym.symbol))
			pstate->block_tracker->update (sym);
		      pstate->push_new<var_value_operation> (sym);
		    }
		  else if (is_a_field_of_this.type != NULL)
		     {
		      /* It hangs off of `this'.  Must not inadvertently convert from a
			 method call to data ref.  */
		      pstate->block_tracker->update (sym);
		      operation_up thisop
			= make_operation<op_this_operation> ();
		      pstate->push_new<structop_ptr_operation>
			(std::move (thisop), std::move (copy));
		    }
		  else
		    {
		      /* Lookup foreign name in global static symbols.  */
		      msymbol = lookup_bound_minimal_symbol (copy.c_str ());
		      if (msymbol.minsym != NULL)
			pstate->push_new<var_msym_value_operation> (msymbol);
		      else if (!have_full_symbols () && !have_partial_symbols ())
			error (_("No symbol table is loaded.  Use the \"file\" command"));
		      else
			error (_("No symbol \"%s\" in current context."),
			       copy.c_str ());
		    }
		  }
#line 1909 "d-exp.c.tmp"
    break;

  case 81: /* PrimaryExpression: TypeExp '.' IdentifierExp  */
#line 481 "d-exp.y"
                        { struct type *type = check_typedef ((yyvsp[-2].tval));

			  /* Check if the qualified name is in the global
			     context.  However if the symbol has not already
			     been resolved, it's not likely to be found.  */
			  if (type->code () == TYPE_CODE_MODULE)
			    {
			      struct block_symbol sym;
			      const char *type_name = TYPE_SAFE_NAME (type);
			      int type_name_len = strlen (type_name);
			      std::string name
				= string_printf ("%.*s.%.*s",
						 type_name_len, type_name,
						 (yyvsp[0].sval).length, (yyvsp[0].sval).ptr);

			      sym =
				lookup_symbol (name.c_str (),
					       (const struct block *) NULL,
					       VAR_DOMAIN, NULL);
			      pstate->push_symbol (name.c_str (), sym);
			    }
			  else
			    {
			      /* Check if the qualified name resolves as a member
				 of an aggregate or an enum type.  */
			      if (!type_aggregate_p (type))
				error (_("`%s' is not defined as an aggregate type."),
				       TYPE_SAFE_NAME (type));

			      pstate->push_new<scope_operation>
				(type, copy_name ((yyvsp[0].sval)));
			    }
			}
#line 1947 "d-exp.c.tmp"
    break;

  case 82: /* PrimaryExpression: DOLLAR_VARIABLE  */
#line 515 "d-exp.y"
                { pstate->push_dollar ((yyvsp[0].sval)); }
#line 1953 "d-exp.c.tmp"
    break;

  case 83: /* PrimaryExpression: NAME_OR_INT  */
#line 517 "d-exp.y"
                { YYSTYPE val;
		  parse_number (pstate, (yyvsp[0].sval).ptr, (yyvsp[0].sval).length, 0, &val);
		  pstate->push_new<long_const_operation>
		    (val.typed_val_int.type, val.typed_val_int.val); }
#line 1962 "d-exp.c.tmp"
    break;

  case 84: /* PrimaryExpression: NULL_KEYWORD  */
#line 522 "d-exp.y"
                { struct type *type = parse_d_type (pstate)->builtin_void;
		  type = lookup_pointer_type (type);
		  pstate->push_new<long_const_operation> (type, 0); }
#line 1970 "d-exp.c.tmp"
    break;

  case 85: /* PrimaryExpression: TRUE_KEYWORD  */
#line 526 "d-exp.y"
                { pstate->push_new<bool_operation> (true); }
#line 1976 "d-exp.c.tmp"
    break;

  case 86: /* PrimaryExpression: FALSE_KEYWORD  */
#line 528 "d-exp.y"
                { pstate->push_new<bool_operation> (false); }
#line 1982 "d-exp.c.tmp"
    break;

  case 87: /* PrimaryExpression: INTEGER_LITERAL  */
#line 530 "d-exp.y"
                { pstate->push_new<long_const_operation> ((yyvsp[0].typed_val_int).type, (yyvsp[0].typed_val_int).val); }
#line 1988 "d-exp.c.tmp"
    break;

  case 88: /* PrimaryExpression: FLOAT_LITERAL  */
#line 532 "d-exp.y"
                {
		  float_data data;
		  std::copy (std::begin ((yyvsp[0].typed_val_float).val), std::end ((yyvsp[0].typed_val_float).val),
			     std::begin (data));
		  pstate->push_new<float_const_operation> ((yyvsp[0].typed_val_float).type, data);
		}
#line 1999 "d-exp.c.tmp"
    break;

  case 89: /* PrimaryExpression: CHARACTER_LITERAL  */
#line 539 "d-exp.y"
                { struct stoken_vector vec;
		  vec.len = 1;
		  vec.tokens = &(yyvsp[0].tsval);
		  pstate->push_c_string (0, &vec); }
#line 2008 "d-exp.c.tmp"
    break;

  case 90: /* PrimaryExpression: StringExp  */
#line 544 "d-exp.y"
                { int i;
		  pstate->push_c_string (0, &(yyvsp[0].svec));
		  for (i = 0; i < (yyvsp[0].svec).len; ++i)
		    xfree ((yyvsp[0].svec).tokens[i].ptr);
		  xfree ((yyvsp[0].svec).tokens); }
#line 2018 "d-exp.c.tmp"
    break;

  case 91: /* PrimaryExpression: ArrayLiteral  */
#line 550 "d-exp.y"
                {
		  std::vector<operation_up> args
		    = pstate->pop_vector ((yyvsp[0].ival));
		  pstate->push_new<array_operation>
		    (0, (yyvsp[0].ival) - 1, std::move (args));
		}
#line 2029 "d-exp.c.tmp"
    break;

  case 92: /* PrimaryExpression: TYPEOF_KEYWORD '(' Expression ')'  */
#line 557 "d-exp.y"
                { pstate->wrap<typeof_operation> (); }
#line 2035 "d-exp.c.tmp"
    break;

  case 93: /* ArrayLiteral: '[' ArgumentList_opt ']'  */
#line 562 "d-exp.y"
                { (yyval.ival) = pstate->arglist_len; }
#line 2041 "d-exp.c.tmp"
    break;

  case 95: /* StringExp: STRING_LITERAL  */
#line 571 "d-exp.y"
                { /* We copy the string here, and not in the
		     lexer, to guarantee that we do not leak a
		     string.  Note that we follow the
		     NUL-termination convention of the
		     lexer.  */
		  struct typed_stoken *vec = XNEW (struct typed_stoken);
		  (yyval.svec).len = 1;
		  (yyval.svec).tokens = vec;

		  vec->type = (yyvsp[0].tsval).type;
		  vec->length = (yyvsp[0].tsval).length;
		  vec->ptr = (char *) xmalloc ((yyvsp[0].tsval).length + 1);
		  memcpy (vec->ptr, (yyvsp[0].tsval).ptr, (yyvsp[0].tsval).length + 1);
		}
#line 2060 "d-exp.c.tmp"
    break;

  case 96: /* StringExp: StringExp STRING_LITERAL  */
#line 586 "d-exp.y"
                { /* Note that we NUL-terminate here, but just
		     for convenience.  */
		  char *p;
		  ++(yyval.svec).len;
		  (yyval.svec).tokens
		    = XRESIZEVEC (struct typed_stoken, (yyval.svec).tokens, (yyval.svec).len);

		  p = (char *) xmalloc ((yyvsp[0].tsval).length + 1);
		  memcpy (p, (yyvsp[0].tsval).ptr, (yyvsp[0].tsval).length + 1);

		  (yyval.svec).tokens[(yyval.svec).len - 1].type = (yyvsp[0].tsval).type;
		  (yyval.svec).tokens[(yyval.svec).len - 1].length = (yyvsp[0].tsval).length;
		  (yyval.svec).tokens[(yyval.svec).len - 1].ptr = p;
		}
#line 2079 "d-exp.c.tmp"
    break;

  case 97: /* TypeExp: '(' TypeExp ')'  */
#line 604 "d-exp.y"
                { /* Do nothing.  */ }
#line 2085 "d-exp.c.tmp"
    break;

  case 98: /* TypeExp: BasicType  */
#line 606 "d-exp.y"
                { pstate->push_new<type_operation> ((yyvsp[0].tval)); }
#line 2091 "d-exp.c.tmp"
    break;

  case 99: /* TypeExp: BasicType BasicType2  */
#line 608 "d-exp.y"
                { (yyval.tval) = type_stack->follow_types ((yyvsp[-1].tval));
		  pstate->push_new<type_operation> ((yyval.tval));
		}
#line 2099 "d-exp.c.tmp"
    break;

  case 100: /* BasicType2: '*'  */
#line 615 "d-exp.y"
                { type_stack->push (tp_pointer); }
#line 2105 "d-exp.c.tmp"
    break;

  case 101: /* BasicType2: '*' BasicType2  */
#line 617 "d-exp.y"
                { type_stack->push (tp_pointer); }
#line 2111 "d-exp.c.tmp"
    break;

  case 102: /* BasicType2: '[' INTEGER_LITERAL ']'  */
#line 619 "d-exp.y"
                { type_stack->push ((yyvsp[-1].typed_val_int).val);
		  type_stack->push (tp_array); }
#line 2118 "d-exp.c.tmp"
    break;

  case 103: /* BasicType2: '[' INTEGER_LITERAL ']' BasicType2  */
#line 622 "d-exp.y"
                { type_stack->push ((yyvsp[-2].typed_val_int).val);
		  type_stack->push (tp_array); }
#line 2125 "d-exp.c.tmp"
    break;

  case 104: /* BasicType: TYPENAME  */
#line 628 "d-exp.y"
                { (yyval.tval) = (yyvsp[0].tsym).type; }
#line 2131 "d-exp.c.tmp"
    break;


#line 2135 "d-exp.c.tmp"

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

#line 631 "d-exp.y"


/* Return true if the type is aggregate-like.  */

static int
type_aggregate_p (struct type *type)
{
  return (type->code () == TYPE_CODE_STRUCT
	  || type->code () == TYPE_CODE_UNION
	  || type->code () == TYPE_CODE_MODULE
	  || (type->code () == TYPE_CODE_ENUM
	      && type->is_declared_class ()));
}

/* Take care of parsing a number (anything that starts with a digit).
   Set yylval and return the token type; update lexptr.
   LEN is the number of characters in it.  */

/*** Needs some error checking for the float case ***/

static int
parse_number (struct parser_state *ps, const char *p,
	      int len, int parsed_float, YYSTYPE *putithere)
{
  ULONGEST n = 0;
  ULONGEST prevn = 0;
  ULONGEST un;

  int i = 0;
  int c;
  int base = input_radix;
  int unsigned_p = 0;
  int long_p = 0;

  /* We have found a "L" or "U" suffix.  */
  int found_suffix = 0;

  ULONGEST high_bit;
  struct type *signed_type;
  struct type *unsigned_type;

  if (parsed_float)
    {
      char *s, *sp;

      /* Strip out all embedded '_' before passing to parse_float.  */
      s = (char *) alloca (len + 1);
      sp = s;
      while (len-- > 0)
	{
	  if (*p != '_')
	    *sp++ = *p;
	  p++;
	}
      *sp = '\0';
      len = strlen (s);

      /* Check suffix for `i' , `fi' or `li' (idouble, ifloat or ireal).  */
      if (len >= 1 && tolower (s[len - 1]) == 'i')
	{
	  if (len >= 2 && tolower (s[len - 2]) == 'f')
	    {
	      putithere->typed_val_float.type
		= parse_d_type (ps)->builtin_ifloat;
	      len -= 2;
	    }
	  else if (len >= 2 && tolower (s[len - 2]) == 'l')
	    {
	      putithere->typed_val_float.type
		= parse_d_type (ps)->builtin_ireal;
	      len -= 2;
	    }
	  else
	    {
	      putithere->typed_val_float.type
		= parse_d_type (ps)->builtin_idouble;
	      len -= 1;
	    }
	}
      /* Check suffix for `f' or `l'' (float or real).  */
      else if (len >= 1 && tolower (s[len - 1]) == 'f')
	{
	  putithere->typed_val_float.type
	    = parse_d_type (ps)->builtin_float;
	  len -= 1;
	}
      else if (len >= 1 && tolower (s[len - 1]) == 'l')
	{
	  putithere->typed_val_float.type
	    = parse_d_type (ps)->builtin_real;
	  len -= 1;
	}
      /* Default type if no suffix.  */
      else
	{
	  putithere->typed_val_float.type
	    = parse_d_type (ps)->builtin_double;
	}

      if (!parse_float (s, len,
			putithere->typed_val_float.type,
			putithere->typed_val_float.val))
	return ERROR;

      return FLOAT_LITERAL;
    }

  /* Handle base-switching prefixes 0x, 0b, 0 */
  if (p[0] == '0')
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

      default:
	base = 8;
	break;
      }

  while (len-- > 0)
    {
      c = *p++;
      if (c == '_')
	continue;	/* Ignore embedded '_'.  */
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
	  else if (c == 'l' && long_p == 0)
	    {
	      long_p = 1;
	      found_suffix = 1;
	    }
	  else if (c == 'u' && unsigned_p == 0)
	    {
	      unsigned_p = 1;
	      found_suffix = 1;
	    }
	  else
	    return ERROR;	/* Char not a digit */
	}
      if (i >= base)
	return ERROR;		/* Invalid digit in this base.  */
      /* Portably test for integer overflow.  */
      if (c != 'l' && c != 'u')
	{
	  ULONGEST n2 = prevn * base;
	  if ((n2 / base != prevn) || (n2 + i < prevn))
	    error (_("Numeric constant too large."));
	}
      prevn = n;
    }

  /* An integer constant is an int or a long.  An L suffix forces it to
     be long, and a U suffix forces it to be unsigned.  To figure out
     whether it fits, we shift it right and see whether anything remains.
     Note that we can't shift sizeof (LONGEST) * HOST_CHAR_BIT bits or
     more in one operation, because many compilers will warn about such a
     shift (which always produces a zero result).  To deal with the case
     where it is we just always shift the value more than once, with fewer
     bits each time.  */
  un = (ULONGEST) n >> 2;
  if (long_p == 0 && (un >> 30) == 0)
    {
      high_bit = ((ULONGEST) 1) << 31;
      signed_type = parse_d_type (ps)->builtin_int;
      /* For decimal notation, keep the sign of the worked out type.  */
      if (base == 10 && !unsigned_p)
	unsigned_type = parse_d_type (ps)->builtin_long;
      else
	unsigned_type = parse_d_type (ps)->builtin_uint;
    }
  else
    {
      int shift;
      if (sizeof (ULONGEST) * HOST_CHAR_BIT < 64)
	/* A long long does not fit in a LONGEST.  */
	shift = (sizeof (ULONGEST) * HOST_CHAR_BIT - 1);
      else
	shift = 63;
      high_bit = (ULONGEST) 1 << shift;
      signed_type = parse_d_type (ps)->builtin_long;
      unsigned_type = parse_d_type (ps)->builtin_ulong;
    }

  putithere->typed_val_int.val = n;

  /* If the high bit of the worked out type is set then this number
     has to be unsigned_type.  */
  if (unsigned_p || (n & high_bit))
    putithere->typed_val_int.type = unsigned_type;
  else
    putithere->typed_val_int.type = signed_type;

  return INTEGER_LITERAL;
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
      if (quote == '"' || quote == '`')
	error (_("Unterminated string in expression."));
      else
	error (_("Unmatched single quote."));
    }
  ++tokptr;

  /* FIXME: should instead use own language string_type enum
     and handle D-specific string suffixes here. */
  if (quote == '\'')
    value->type = C_CHAR;
  else
    value->type = C_STRING;

  value->ptr = (char *) obstack_base (&tempbuf);
  value->length = obstack_object_size (&tempbuf);

  *outptr = tokptr;

  return quote == '\'' ? CHARACTER_LITERAL : STRING_LITERAL;
}

struct token
{
  const char *oper;
  int token;
  enum exp_opcode opcode;
};

static const struct token tokentab3[] =
  {
    {"^^=", ASSIGN_MODIFY, BINOP_EXP},
    {"<<=", ASSIGN_MODIFY, BINOP_LSH},
    {">>=", ASSIGN_MODIFY, BINOP_RSH},
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
    {"&&", ANDAND, OP_NULL},
    {"||", OROR, OP_NULL},
    {"^^", HATHAT, OP_NULL},
    {"<<", LSH, OP_NULL},
    {">>", RSH, OP_NULL},
    {"==", EQUAL, OP_NULL},
    {"!=", NOTEQUAL, OP_NULL},
    {"<=", LEQ, OP_NULL},
    {">=", GEQ, OP_NULL},
    {"..", DOTDOT, OP_NULL},
  };

/* Identifier-like tokens.  */
static const struct token ident_tokens[] =
  {
    {"is", IDENTITY, OP_NULL},
    {"!is", NOTIDENTITY, OP_NULL},

    {"cast", CAST_KEYWORD, OP_NULL},
    {"const", CONST_KEYWORD, OP_NULL},
    {"immutable", IMMUTABLE_KEYWORD, OP_NULL},
    {"shared", SHARED_KEYWORD, OP_NULL},
    {"super", SUPER_KEYWORD, OP_NULL},

    {"null", NULL_KEYWORD, OP_NULL},
    {"true", TRUE_KEYWORD, OP_NULL},
    {"false", FALSE_KEYWORD, OP_NULL},

    {"init", INIT_KEYWORD, OP_NULL},
    {"sizeof", SIZEOF_KEYWORD, OP_NULL},
    {"typeof", TYPEOF_KEYWORD, OP_NULL},
    {"typeid", TYPEID_KEYWORD, OP_NULL},

    {"delegate", DELEGATE_KEYWORD, OP_NULL},
    {"function", FUNCTION_KEYWORD, OP_NULL},
    {"struct", STRUCT_KEYWORD, OP_NULL},
    {"union", UNION_KEYWORD, OP_NULL},
    {"class", CLASS_KEYWORD, OP_NULL},
    {"interface", INTERFACE_KEYWORD, OP_NULL},
    {"enum", ENUM_KEYWORD, OP_NULL},
    {"template", TEMPLATE_KEYWORD, OP_NULL},
  };

/* This is set if a NAME token appeared at the very end of the input
   string, with no whitespace separating the name from the EOF.  This
   is used only when parsing to do field name completion.  */
static int saw_name_at_eof;

/* This is set if the previously-returned token was a structure operator.
   This is used only when parsing to do field name completion.  */
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

  pstate->prev_lexptr = pstate->lexptr;

  tokstart = pstate->lexptr;
  /* See if it is a special token of length 3.  */
  for (const auto &token : tokentab3)
    if (strncmp (tokstart, token.oper, 3) == 0)
      {
	pstate->lexptr += 3;
	yylval.opcode = token.opcode;
	return token.token;
      }

  /* See if it is a special token of length 2.  */
  for (const auto &token : tokentab2)
    if (strncmp (tokstart, token.oper, 2) == 0)
      {
	pstate->lexptr += 2;
	yylval.opcode = token.opcode;
	return token.token;
      }

  switch (c = *tokstart)
    {
    case 0:
      /* If we're parsing for field name completion, and the previous
	 token allows such completion, return a COMPLETE token.
	 Otherwise, we were already scanning the original text, and
	 we're really done.  */
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
      pstate->lexptr++;
      goto retry;

    case '[':
    case '(':
      paren_depth++;
      pstate->lexptr++;
      return c;

    case ']':
    case ')':
      if (paren_depth == 0)
	return 0;
      paren_depth--;
      pstate->lexptr++;
      return c;

    case ',':
      if (pstate->comma_terminates && paren_depth == 0)
	return 0;
      pstate->lexptr++;
      return c;

    case '.':
      /* Might be a floating point number.  */
      if (pstate->lexptr[1] < '0' || pstate->lexptr[1] > '9')
	{
	  if (pstate->parse_completion)
	    last_was_structop = 1;
	  goto symbol;		/* Nope, must be a symbol.  */
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
	    /* Hex exponents start with 'p', because 'e' is a valid hex
	       digit and thus does not indicate a floating point number
	       when the radix is hex.  */
	    if ((!hex && !got_e && tolower (p[0]) == 'e')
		|| (hex && !got_e && tolower (p[0] == 'p')))
	      got_dot = got_e = 1;
	    /* A '.' always indicates a decimal floating point number
	       regardless of the radix.  If we have a '..' then its the
	       end of the number and the beginning of a slice.  */
	    else if (!got_dot && (p[0] == '.' && p[1] != '.'))
		got_dot = 1;
	    /* This is the sign of the exponent, not the end of the number.  */
	    else if (got_e && (tolower (p[-1]) == 'e' || tolower (p[-1]) == 'p')
		     && (*p == '-' || *p == '+'))
	      continue;
	    /* We will take any letters or digits, ignoring any embedded '_'.
	       parse_number will complain if past the radix, or if L or U are
	       not final.  */
	    else if ((*p < '0' || *p > '9') && (*p != '_')
		     && ((*p < 'a' || *p > 'z') && (*p < 'A' || *p > 'Z')))
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
	pstate->lexptr = p;
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
	    pstate->lexptr = &p[len];
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
      pstate->lexptr++;
      return c;

    case '\'':
    case '"':
    case '`':
      {
	int host_len;
	int result = parse_string_or_char (tokstart, &pstate->lexptr,
					   &yylval.tsval, &host_len);
	if (result == CHARACTER_LITERAL)
	  {
	    if (host_len == 0)
	      error (_("Empty character constant."));
	    else if (host_len > 2 && c == '\'')
	      {
		++tokstart;
		namelen = pstate->lexptr - tokstart - 1;
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
    error (_("Invalid character '%c' in expression"), c);

  /* It's a name.  See how long it is.  */
  namelen = 0;
  for (c = tokstart[namelen];
       (c == '_' || c == '$' || (c >= '0' && c <= '9')
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));)
    c = tokstart[++namelen];

  /* The token "if" terminates the expression and is NOT
     removed from the input stream.  */
  if (namelen == 2 && tokstart[0] == 'i' && tokstart[1] == 'f')
    return 0;

  /* For the same reason (breakpoint conditions), "thread N"
     terminates the expression.  "thread" could be an identifier, but
     an identifier is never followed by a number without intervening
     punctuation.  "task" is similar.  Handle abbreviations of these,
     similarly to breakpoint.c:find_condition_and_thread.  */
  if (namelen >= 1
      && (strncmp (tokstart, "thread", namelen) == 0
	  || strncmp (tokstart, "task", namelen) == 0)
      && (tokstart[namelen] == ' ' || tokstart[namelen] == '\t'))
    {
      const char *p = tokstart + namelen + 1;

      while (*p == ' ' || *p == '\t')
	p++;
      if (*p >= '0' && *p <= '9')
	return 0;
    }

  pstate->lexptr += namelen;

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

  yylval.tsym.type
    = language_lookup_primitive_type (par_state->language (),
				      par_state->gdbarch (), copy.c_str ());
  if (yylval.tsym.type != NULL)
    return TYPENAME;

  /* Input names that aren't symbols but ARE valid hex numbers,
     when the input radix permits them, can be names or numbers
     depending on the parse.  Note we support radixes > 16 here.  */
  if ((tokstart[0] >= 'a' && tokstart[0] < 'a' + input_radix - 10)
      || (tokstart[0] >= 'A' && tokstart[0] < 'A' + input_radix - 10))
    {
      YYSTYPE newlval;	/* Its value is ignored.  */
      int hextype = parse_number (par_state, tokstart, namelen, 0, &newlval);
      if (hextype == INTEGER_LITERAL)
	return NAME_OR_INT;
    }

  if (pstate->parse_completion && *pstate->lexptr == '\0')
    saw_name_at_eof = 1;

  return IDENTIFIER;
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

/* Classify an IDENTIFIER token.  The contents of the token are in `yylval'.
   Updates yylval and returns the new token type.  BLOCK is the block
   in which lookups start; this can be NULL to mean the global scope.  */

static int
classify_name (struct parser_state *par_state, const struct block *block)
{
  struct block_symbol sym;
  struct field_of_this_result is_a_field_of_this;

  std::string copy = copy_name (yylval.sval);

  sym = lookup_symbol (copy.c_str (), block, VAR_DOMAIN, &is_a_field_of_this);
  if (sym.symbol && sym.symbol->aclass () == LOC_TYPEDEF)
    {
      yylval.tsym.type = sym.symbol->type ();
      return TYPENAME;
    }
  else if (sym.symbol == NULL)
    {
      /* Look-up first for a module name, then a type.  */
      sym = lookup_symbol (copy.c_str (), block, MODULE_DOMAIN, NULL);
      if (sym.symbol == NULL)
	sym = lookup_symbol (copy.c_str (), block, STRUCT_DOMAIN, NULL);

      if (sym.symbol != NULL)
	{
	  yylval.tsym.type = sym.symbol->type ();
	  return TYPENAME;
	}

      return UNKNOWN_NAME;
    }

  return IDENTIFIER;
}

/* Like classify_name, but used by the inner loop of the lexer, when a
   name might have already been seen.  CONTEXT is the context type, or
   NULL if this is the first component of a name.  */

static int
classify_inner_name (struct parser_state *par_state,
		     const struct block *block, struct type *context)
{
  struct type *type;

  if (context == NULL)
    return classify_name (par_state, block);

  type = check_typedef (context);
  if (!type_aggregate_p (type))
    return ERROR;

  std::string copy = copy_name (yylval.ssym.stoken);
  yylval.ssym.sym = d_lookup_nested_symbol (type, copy.c_str (), block);

  if (yylval.ssym.sym.symbol == NULL)
    return ERROR;

  if (yylval.ssym.sym.symbol->aclass () == LOC_TYPEDEF)
    {
      yylval.tsym.type = yylval.ssym.sym.symbol->type ();
      return TYPENAME;
    }

  return IDENTIFIER;
}

/* The outer level of a two-level lexer.  This calls the inner lexer
   to return tokens.  It then either returns these tokens, or
   aggregates them into a larger token.  This lets us work around a
   problem in our parsing approach, where the parser could not
   distinguish between qualified names and qualified types at the
   right point.  */

static int
yylex (void)
{
  token_and_value current;
  int last_was_dot;
  struct type *context_type = NULL;
  int last_to_examine, next_to_examine, checkpoint;
  const struct block *search_block;

  if (popping && !token_fifo.empty ())
    goto do_pop;
  popping = 0;

  /* Read the first token and decide what to do.  */
  current.token = lex_one_token (pstate);
  if (current.token != IDENTIFIER && current.token != '.')
    return current.token;

  /* Read any sequence of alternating "." and identifier tokens into
     the token FIFO.  */
  current.value = yylval;
  token_fifo.push_back (current);
  last_was_dot = current.token == '.';

  while (1)
    {
      current.token = lex_one_token (pstate);
      current.value = yylval;
      token_fifo.push_back (current);

      if ((last_was_dot && current.token != IDENTIFIER)
	  || (!last_was_dot && current.token != '.'))
	break;

      last_was_dot = !last_was_dot;
    }
  popping = 1;

  /* We always read one extra token, so compute the number of tokens
     to examine accordingly.  */
  last_to_examine = token_fifo.size () - 2;
  next_to_examine = 0;

  current = token_fifo[next_to_examine];
  ++next_to_examine;

  /* If we are not dealing with a typename, now is the time to find out.  */
  if (current.token == IDENTIFIER)
    {
      yylval = current.value;
      current.token = classify_name (pstate, pstate->expression_context_block);
      current.value = yylval;
    }

  /* If the IDENTIFIER is not known, it could be a package symbol,
     first try building up a name until we find the qualified module.  */
  if (current.token == UNKNOWN_NAME)
    {
      name_obstack.clear ();
      obstack_grow (&name_obstack, current.value.sval.ptr,
		    current.value.sval.length);

      last_was_dot = 0;

      while (next_to_examine <= last_to_examine)
	{
	  token_and_value next;

	  next = token_fifo[next_to_examine];
	  ++next_to_examine;

	  if (next.token == IDENTIFIER && last_was_dot)
	    {
	      /* Update the partial name we are constructing.  */
	      obstack_grow_str (&name_obstack, ".");
	      obstack_grow (&name_obstack, next.value.sval.ptr,
			    next.value.sval.length);

	      yylval.sval.ptr = (char *) obstack_base (&name_obstack);
	      yylval.sval.length = obstack_object_size (&name_obstack);

	      current.token = classify_name (pstate,
					     pstate->expression_context_block);
	      current.value = yylval;

	      /* We keep going until we find a TYPENAME.  */
	      if (current.token == TYPENAME)
		{
		  /* Install it as the first token in the FIFO.  */
		  token_fifo[0] = current;
		  token_fifo.erase (token_fifo.begin () + 1,
				    token_fifo.begin () + next_to_examine);
		  break;
		}
	    }
	  else if (next.token == '.' && !last_was_dot)
	    last_was_dot = 1;
	  else
	    {
	      /* We've reached the end of the name.  */
	      break;
	    }
	}

      /* Reset our current token back to the start, if we found nothing
	 this means that we will just jump to do pop.  */
      current = token_fifo[0];
      next_to_examine = 1;
    }
  if (current.token != TYPENAME && current.token != '.')
    goto do_pop;

  name_obstack.clear ();
  checkpoint = 0;
  if (current.token == '.')
    search_block = NULL;
  else
    {
      gdb_assert (current.token == TYPENAME);
      search_block = pstate->expression_context_block;
      obstack_grow (&name_obstack, current.value.sval.ptr,
		    current.value.sval.length);
      context_type = current.value.tsym.type;
      checkpoint = 1;
    }

  last_was_dot = current.token == '.';

  while (next_to_examine <= last_to_examine)
    {
      token_and_value next;

      next = token_fifo[next_to_examine];
      ++next_to_examine;

      if (next.token == IDENTIFIER && last_was_dot)
	{
	  int classification;

	  yylval = next.value;
	  classification = classify_inner_name (pstate, search_block,
						context_type);
	  /* We keep going until we either run out of names, or until
	     we have a qualified name which is not a type.  */
	  if (classification != TYPENAME && classification != IDENTIFIER)
	    break;

	  /* Accept up to this token.  */
	  checkpoint = next_to_examine;

	  /* Update the partial name we are constructing.  */
	  if (context_type != NULL)
	    {
	      /* We don't want to put a leading "." into the name.  */
	      obstack_grow_str (&name_obstack, ".");
	    }
	  obstack_grow (&name_obstack, next.value.sval.ptr,
			next.value.sval.length);

	  yylval.sval.ptr = (char *) obstack_base (&name_obstack);
	  yylval.sval.length = obstack_object_size (&name_obstack);
	  current.value = yylval;
	  current.token = classification;

	  last_was_dot = 0;

	  if (classification == IDENTIFIER)
	    break;

	  context_type = yylval.tsym.type;
	}
      else if (next.token == '.' && !last_was_dot)
	last_was_dot = 1;
      else
	{
	  /* We've reached the end of the name.  */
	  break;
	}
    }

  /* If we have a replacement token, install it as the first token in
     the FIFO, and delete the other constituent tokens.  */
  if (checkpoint > 0)
    {
      token_fifo[0] = current;
      if (checkpoint > 1)
	token_fifo.erase (token_fifo.begin () + 1,
			  token_fifo.begin () + checkpoint);
    }

 do_pop:
  current = token_fifo[0];
  token_fifo.erase (token_fifo.begin ());
  yylval = current.value;
  return current.token;
}

int
d_parse (struct parser_state *par_state)
{
  /* Setting up the parser state.  */
  scoped_restore pstate_restore = make_scoped_restore (&pstate);
  gdb_assert (par_state != NULL);
  pstate = par_state;

  scoped_restore restore_yydebug = make_scoped_restore (&yydebug,
							parser_debug);

  struct type_stack stack;
  scoped_restore restore_type_stack = make_scoped_restore (&type_stack,
							   &stack);

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

