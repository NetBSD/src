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
#line 43 "f-exp.y"


#include "defs.h"
#include "expression.h"
#include "value.h"
#include "parser-defs.h"
#include "language.h"
#include "f-lang.h"
#include "bfd.h" /* Required by objfiles.h.  */
#include "symfile.h" /* Required by objfiles.h.  */
#include "objfiles.h" /* For have_full_symbols and have_partial_symbols */
#include "block.h"
#include <ctype.h>
#include <algorithm>
#include "type-stack.h"
#include "f-exp.h"

#define parse_type(ps) builtin_type (ps->gdbarch ())
#define parse_f_type(ps) builtin_f_type (ps->gdbarch ())

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror,
   etc).  */
#define GDB_YY_REMAP_PREFIX f_
#include "yy-remap.h"

/* The state of the parser, used internally when we are parsing the
   expression.  */

static struct parser_state *pstate = NULL;

/* Depth of parentheses.  */
static int paren_depth;

/* The current type stack.  */
static struct type_stack *type_stack;

int yyparse (void);

static int yylex (void);

static void yyerror (const char *);

static void growbuf_by_size (int);

static int match_string_literal (void);

static void push_kind_type (LONGEST val, struct type *type);

static struct type *convert_to_kind_type (struct type *basetype, int kind);

static void wrap_unop_intrinsic (exp_opcode opcode);

static void wrap_binop_intrinsic (exp_opcode opcode);

static void wrap_ternop_intrinsic (exp_opcode opcode);

template<typename T>
static void fortran_wrap2_kind (type *base_type);

template<typename T>
static void fortran_wrap3_kind (type *base_type);

using namespace expr;

#line 136 "f-exp.c.tmp"

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
    STRING_LITERAL = 260,          /* STRING_LITERAL  */
    BOOLEAN_LITERAL = 261,         /* BOOLEAN_LITERAL  */
    NAME = 262,                    /* NAME  */
    TYPENAME = 263,                /* TYPENAME  */
    COMPLETE = 264,                /* COMPLETE  */
    NAME_OR_INT = 265,             /* NAME_OR_INT  */
    SIZEOF = 266,                  /* SIZEOF  */
    KIND = 267,                    /* KIND  */
    ERROR = 268,                   /* ERROR  */
    INT_S1_KEYWORD = 269,          /* INT_S1_KEYWORD  */
    INT_S2_KEYWORD = 270,          /* INT_S2_KEYWORD  */
    INT_KEYWORD = 271,             /* INT_KEYWORD  */
    INT_S4_KEYWORD = 272,          /* INT_S4_KEYWORD  */
    INT_S8_KEYWORD = 273,          /* INT_S8_KEYWORD  */
    LOGICAL_S1_KEYWORD = 274,      /* LOGICAL_S1_KEYWORD  */
    LOGICAL_S2_KEYWORD = 275,      /* LOGICAL_S2_KEYWORD  */
    LOGICAL_KEYWORD = 276,         /* LOGICAL_KEYWORD  */
    LOGICAL_S4_KEYWORD = 277,      /* LOGICAL_S4_KEYWORD  */
    LOGICAL_S8_KEYWORD = 278,      /* LOGICAL_S8_KEYWORD  */
    REAL_KEYWORD = 279,            /* REAL_KEYWORD  */
    REAL_S4_KEYWORD = 280,         /* REAL_S4_KEYWORD  */
    REAL_S8_KEYWORD = 281,         /* REAL_S8_KEYWORD  */
    REAL_S16_KEYWORD = 282,        /* REAL_S16_KEYWORD  */
    COMPLEX_KEYWORD = 283,         /* COMPLEX_KEYWORD  */
    COMPLEX_S4_KEYWORD = 284,      /* COMPLEX_S4_KEYWORD  */
    COMPLEX_S8_KEYWORD = 285,      /* COMPLEX_S8_KEYWORD  */
    COMPLEX_S16_KEYWORD = 286,     /* COMPLEX_S16_KEYWORD  */
    BOOL_AND = 287,                /* BOOL_AND  */
    BOOL_OR = 288,                 /* BOOL_OR  */
    BOOL_NOT = 289,                /* BOOL_NOT  */
    SINGLE = 290,                  /* SINGLE  */
    DOUBLE = 291,                  /* DOUBLE  */
    PRECISION = 292,               /* PRECISION  */
    CHARACTER = 293,               /* CHARACTER  */
    DOLLAR_VARIABLE = 294,         /* DOLLAR_VARIABLE  */
    ASSIGN_MODIFY = 295,           /* ASSIGN_MODIFY  */
    UNOP_INTRINSIC = 296,          /* UNOP_INTRINSIC  */
    BINOP_INTRINSIC = 297,         /* BINOP_INTRINSIC  */
    UNOP_OR_BINOP_INTRINSIC = 298, /* UNOP_OR_BINOP_INTRINSIC  */
    UNOP_OR_BINOP_OR_TERNOP_INTRINSIC = 299, /* UNOP_OR_BINOP_OR_TERNOP_INTRINSIC  */
    ABOVE_COMMA = 300,             /* ABOVE_COMMA  */
    EQUAL = 301,                   /* EQUAL  */
    NOTEQUAL = 302,                /* NOTEQUAL  */
    LESSTHAN = 303,                /* LESSTHAN  */
    GREATERTHAN = 304,             /* GREATERTHAN  */
    LEQ = 305,                     /* LEQ  */
    GEQ = 306,                     /* GEQ  */
    LSH = 307,                     /* LSH  */
    RSH = 308,                     /* RSH  */
    STARSTAR = 309,                /* STARSTAR  */
    UNARY = 310                    /* UNARY  */
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
#define STRING_LITERAL 260
#define BOOLEAN_LITERAL 261
#define NAME 262
#define TYPENAME 263
#define COMPLETE 264
#define NAME_OR_INT 265
#define SIZEOF 266
#define KIND 267
#define ERROR 268
#define INT_S1_KEYWORD 269
#define INT_S2_KEYWORD 270
#define INT_KEYWORD 271
#define INT_S4_KEYWORD 272
#define INT_S8_KEYWORD 273
#define LOGICAL_S1_KEYWORD 274
#define LOGICAL_S2_KEYWORD 275
#define LOGICAL_KEYWORD 276
#define LOGICAL_S4_KEYWORD 277
#define LOGICAL_S8_KEYWORD 278
#define REAL_KEYWORD 279
#define REAL_S4_KEYWORD 280
#define REAL_S8_KEYWORD 281
#define REAL_S16_KEYWORD 282
#define COMPLEX_KEYWORD 283
#define COMPLEX_S4_KEYWORD 284
#define COMPLEX_S8_KEYWORD 285
#define COMPLEX_S16_KEYWORD 286
#define BOOL_AND 287
#define BOOL_OR 288
#define BOOL_NOT 289
#define SINGLE 290
#define DOUBLE 291
#define PRECISION 292
#define CHARACTER 293
#define DOLLAR_VARIABLE 294
#define ASSIGN_MODIFY 295
#define UNOP_INTRINSIC 296
#define BINOP_INTRINSIC 297
#define UNOP_OR_BINOP_INTRINSIC 298
#define UNOP_OR_BINOP_OR_TERNOP_INTRINSIC 299
#define ABOVE_COMMA 300
#define EQUAL 301
#define NOTEQUAL 302
#define LESSTHAN 303
#define GREATERTHAN 304
#define LEQ 305
#define GEQ 306
#define LSH 307
#define RSH 308
#define STARSTAR 309
#define UNARY 310

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 113 "f-exp.y"

    LONGEST lval;
    struct {
      LONGEST val;
      struct type *type;
    } typed_val;
    struct {
      gdb_byte val[16];
      struct type *type;
    } typed_val_float;
    struct symbol *sym;
    struct type *tval;
    struct stoken sval;
    struct ttype tsym;
    struct symtoken ssym;
    int voidval;
    enum exp_opcode opcode;
    struct internalvar *ivar;

    struct type **tvec;
    int *ivec;
  

#line 320 "f-exp.c.tmp"

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
  YYSYMBOL_STRING_LITERAL = 5,             /* STRING_LITERAL  */
  YYSYMBOL_BOOLEAN_LITERAL = 6,            /* BOOLEAN_LITERAL  */
  YYSYMBOL_NAME = 7,                       /* NAME  */
  YYSYMBOL_TYPENAME = 8,                   /* TYPENAME  */
  YYSYMBOL_COMPLETE = 9,                   /* COMPLETE  */
  YYSYMBOL_NAME_OR_INT = 10,               /* NAME_OR_INT  */
  YYSYMBOL_SIZEOF = 11,                    /* SIZEOF  */
  YYSYMBOL_KIND = 12,                      /* KIND  */
  YYSYMBOL_ERROR = 13,                     /* ERROR  */
  YYSYMBOL_INT_S1_KEYWORD = 14,            /* INT_S1_KEYWORD  */
  YYSYMBOL_INT_S2_KEYWORD = 15,            /* INT_S2_KEYWORD  */
  YYSYMBOL_INT_KEYWORD = 16,               /* INT_KEYWORD  */
  YYSYMBOL_INT_S4_KEYWORD = 17,            /* INT_S4_KEYWORD  */
  YYSYMBOL_INT_S8_KEYWORD = 18,            /* INT_S8_KEYWORD  */
  YYSYMBOL_LOGICAL_S1_KEYWORD = 19,        /* LOGICAL_S1_KEYWORD  */
  YYSYMBOL_LOGICAL_S2_KEYWORD = 20,        /* LOGICAL_S2_KEYWORD  */
  YYSYMBOL_LOGICAL_KEYWORD = 21,           /* LOGICAL_KEYWORD  */
  YYSYMBOL_LOGICAL_S4_KEYWORD = 22,        /* LOGICAL_S4_KEYWORD  */
  YYSYMBOL_LOGICAL_S8_KEYWORD = 23,        /* LOGICAL_S8_KEYWORD  */
  YYSYMBOL_REAL_KEYWORD = 24,              /* REAL_KEYWORD  */
  YYSYMBOL_REAL_S4_KEYWORD = 25,           /* REAL_S4_KEYWORD  */
  YYSYMBOL_REAL_S8_KEYWORD = 26,           /* REAL_S8_KEYWORD  */
  YYSYMBOL_REAL_S16_KEYWORD = 27,          /* REAL_S16_KEYWORD  */
  YYSYMBOL_COMPLEX_KEYWORD = 28,           /* COMPLEX_KEYWORD  */
  YYSYMBOL_COMPLEX_S4_KEYWORD = 29,        /* COMPLEX_S4_KEYWORD  */
  YYSYMBOL_COMPLEX_S8_KEYWORD = 30,        /* COMPLEX_S8_KEYWORD  */
  YYSYMBOL_COMPLEX_S16_KEYWORD = 31,       /* COMPLEX_S16_KEYWORD  */
  YYSYMBOL_BOOL_AND = 32,                  /* BOOL_AND  */
  YYSYMBOL_BOOL_OR = 33,                   /* BOOL_OR  */
  YYSYMBOL_BOOL_NOT = 34,                  /* BOOL_NOT  */
  YYSYMBOL_SINGLE = 35,                    /* SINGLE  */
  YYSYMBOL_DOUBLE = 36,                    /* DOUBLE  */
  YYSYMBOL_PRECISION = 37,                 /* PRECISION  */
  YYSYMBOL_CHARACTER = 38,                 /* CHARACTER  */
  YYSYMBOL_DOLLAR_VARIABLE = 39,           /* DOLLAR_VARIABLE  */
  YYSYMBOL_ASSIGN_MODIFY = 40,             /* ASSIGN_MODIFY  */
  YYSYMBOL_UNOP_INTRINSIC = 41,            /* UNOP_INTRINSIC  */
  YYSYMBOL_BINOP_INTRINSIC = 42,           /* BINOP_INTRINSIC  */
  YYSYMBOL_UNOP_OR_BINOP_INTRINSIC = 43,   /* UNOP_OR_BINOP_INTRINSIC  */
  YYSYMBOL_UNOP_OR_BINOP_OR_TERNOP_INTRINSIC = 44, /* UNOP_OR_BINOP_OR_TERNOP_INTRINSIC  */
  YYSYMBOL_45_ = 45,                       /* ','  */
  YYSYMBOL_ABOVE_COMMA = 46,               /* ABOVE_COMMA  */
  YYSYMBOL_47_ = 47,                       /* '='  */
  YYSYMBOL_48_ = 48,                       /* '?'  */
  YYSYMBOL_49_ = 49,                       /* '|'  */
  YYSYMBOL_50_ = 50,                       /* '^'  */
  YYSYMBOL_51_ = 51,                       /* '&'  */
  YYSYMBOL_EQUAL = 52,                     /* EQUAL  */
  YYSYMBOL_NOTEQUAL = 53,                  /* NOTEQUAL  */
  YYSYMBOL_LESSTHAN = 54,                  /* LESSTHAN  */
  YYSYMBOL_GREATERTHAN = 55,               /* GREATERTHAN  */
  YYSYMBOL_LEQ = 56,                       /* LEQ  */
  YYSYMBOL_GEQ = 57,                       /* GEQ  */
  YYSYMBOL_LSH = 58,                       /* LSH  */
  YYSYMBOL_RSH = 59,                       /* RSH  */
  YYSYMBOL_60_ = 60,                       /* '@'  */
  YYSYMBOL_61_ = 61,                       /* '+'  */
  YYSYMBOL_62_ = 62,                       /* '-'  */
  YYSYMBOL_63_ = 63,                       /* '*'  */
  YYSYMBOL_64_ = 64,                       /* '/'  */
  YYSYMBOL_STARSTAR = 65,                  /* STARSTAR  */
  YYSYMBOL_66_ = 66,                       /* '%'  */
  YYSYMBOL_UNARY = 67,                     /* UNARY  */
  YYSYMBOL_68_ = 68,                       /* '('  */
  YYSYMBOL_69_ = 69,                       /* ')'  */
  YYSYMBOL_70_ = 70,                       /* '~'  */
  YYSYMBOL_71_ = 71,                       /* ':'  */
  YYSYMBOL_YYACCEPT = 72,                  /* $accept  */
  YYSYMBOL_start = 73,                     /* start  */
  YYSYMBOL_type_exp = 74,                  /* type_exp  */
  YYSYMBOL_exp = 75,                       /* exp  */
  YYSYMBOL_76_1 = 76,                      /* $@1  */
  YYSYMBOL_77_2 = 77,                      /* $@2  */
  YYSYMBOL_78_3 = 78,                      /* $@3  */
  YYSYMBOL_arglist = 79,                   /* arglist  */
  YYSYMBOL_subrange = 80,                  /* subrange  */
  YYSYMBOL_complexnum = 81,                /* complexnum  */
  YYSYMBOL_variable = 82,                  /* variable  */
  YYSYMBOL_type = 83,                      /* type  */
  YYSYMBOL_ptype = 84,                     /* ptype  */
  YYSYMBOL_abs_decl = 85,                  /* abs_decl  */
  YYSYMBOL_direct_abs_decl = 86,           /* direct_abs_decl  */
  YYSYMBOL_func_mod = 87,                  /* func_mod  */
  YYSYMBOL_typebase = 88,                  /* typebase  */
  YYSYMBOL_nonempty_typelist = 89,         /* nonempty_typelist  */
  YYSYMBOL_name = 90,                      /* name  */
  YYSYMBOL_name_not_typename = 91          /* name_not_typename  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;


/* Second part of user prologue.  */
#line 136 "f-exp.y"

/* YYSTYPE gets defined by %union */
static int parse_number (struct parser_state *, const char *, int,
			 int, YYSTYPE *);

#line 443 "f-exp.c.tmp"


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
#define YYFINAL  69
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   924

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  72
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  20
/* YYNRULES -- Number of rules.  */
#define YYNRULES  113
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  184

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   310


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
       2,     2,     2,     2,     2,     2,     2,    66,    51,     2,
      68,    69,    63,    61,    45,    62,     2,    64,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    71,     2,
       2,    47,     2,    48,    60,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    50,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    49,     2,    70,     2,     2,     2,
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
      46,    52,    53,    54,    55,    56,    57,    58,    59,    65,
      67
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   222,   222,   223,   226,   230,   235,   239,   243,   247,
     251,   255,   259,   269,   268,   279,   285,   292,   291,   312,
     311,   335,   338,   342,   346,   350,   356,   366,   375,   384,
     395,   407,   419,   431,   443,   447,   457,   464,   471,   481,
     493,   497,   501,   505,   509,   513,   517,   521,   525,   529,
     533,   537,   541,   545,   549,   553,   557,   561,   566,   570,
     574,   583,   590,   600,   609,   612,   616,   625,   629,   636,
     644,   647,   648,   699,   701,   703,   705,   707,   710,   712,
     714,   716,   718,   722,   724,   729,   731,   733,   735,   737,
     739,   741,   743,   745,   747,   749,   751,   753,   755,   757,
     759,   761,   763,   765,   767,   769,   771,   773,   775,   780,
     785,   793,   795,   799
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
  "STRING_LITERAL", "BOOLEAN_LITERAL", "NAME", "TYPENAME", "COMPLETE",
  "NAME_OR_INT", "SIZEOF", "KIND", "ERROR", "INT_S1_KEYWORD",
  "INT_S2_KEYWORD", "INT_KEYWORD", "INT_S4_KEYWORD", "INT_S8_KEYWORD",
  "LOGICAL_S1_KEYWORD", "LOGICAL_S2_KEYWORD", "LOGICAL_KEYWORD",
  "LOGICAL_S4_KEYWORD", "LOGICAL_S8_KEYWORD", "REAL_KEYWORD",
  "REAL_S4_KEYWORD", "REAL_S8_KEYWORD", "REAL_S16_KEYWORD",
  "COMPLEX_KEYWORD", "COMPLEX_S4_KEYWORD", "COMPLEX_S8_KEYWORD",
  "COMPLEX_S16_KEYWORD", "BOOL_AND", "BOOL_OR", "BOOL_NOT", "SINGLE",
  "DOUBLE", "PRECISION", "CHARACTER", "DOLLAR_VARIABLE", "ASSIGN_MODIFY",
  "UNOP_INTRINSIC", "BINOP_INTRINSIC", "UNOP_OR_BINOP_INTRINSIC",
  "UNOP_OR_BINOP_OR_TERNOP_INTRINSIC", "','", "ABOVE_COMMA", "'='", "'?'",
  "'|'", "'^'", "'&'", "EQUAL", "NOTEQUAL", "LESSTHAN", "GREATERTHAN",
  "LEQ", "GEQ", "LSH", "RSH", "'@'", "'+'", "'-'", "'*'", "'/'",
  "STARSTAR", "'%'", "UNARY", "'('", "')'", "'~'", "':'", "$accept",
  "start", "type_exp", "exp", "$@1", "$@2", "$@3", "arglist", "subrange",
  "complexnum", "variable", "type", "ptype", "abs_decl", "direct_abs_decl",
  "func_mod", "typebase", "nonempty_typelist", "name", "name_not_typename", YY_NULLPTRPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-103)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     249,  -103,  -103,  -103,  -103,  -103,  -103,  -103,   291,   -62,
    -103,  -103,  -103,  -103,  -103,  -103,  -103,  -103,  -103,  -103,
    -103,  -103,  -103,  -103,  -103,  -103,  -103,  -103,   333,     6,
      18,  -103,  -103,   -61,   -33,   -26,   -21,   333,   333,   333,
     249,   333,    56,  -103,   718,  -103,  -103,  -103,    -6,  -103,
     249,   -20,   333,   -20,  -103,  -103,  -103,  -103,   333,   333,
    -103,  -103,   -20,   -20,   -20,   567,   -16,    -4,   -20,  -103,
     333,   333,   333,   333,   333,   333,   333,   333,   333,   333,
     333,   333,   333,   333,   333,   333,   333,   333,   333,   333,
     333,    60,  -103,    -6,    -2,   432,  -103,    29,  -103,    40,
     373,   605,   681,    88,    88,   333,  -103,  -103,   333,   802,
     753,   718,   718,   821,   839,   856,   733,   733,    53,    53,
      53,    53,   133,   133,    72,    38,    38,    58,    58,    58,
    -103,  -103,  -103,    54,    88,  -103,  -103,  -103,    63,  -103,
    -103,    59,   -40,    -5,  -103,   375,  -103,  -103,   333,   166,
     456,   -18,  -103,    -9,   718,   -20,  -103,    51,   104,  -103,
     812,  -103,   643,   333,   493,   180,    88,  -103,  -103,  -103,
      75,  -103,  -103,   718,   333,   333,   530,   456,  -103,  -103,
     718,   718,   333,   718
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,    61,    63,    68,    67,   113,    85,    62,     0,     0,
      86,    87,    88,    89,    90,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,     0,     0,
       0,    91,    65,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     3,     2,    64,     4,    70,    71,    69,
       0,    11,     0,     9,   107,   105,   108,   106,     0,     0,
      17,    19,     7,     8,     6,     0,     0,     0,    10,     1,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    13,    75,    73,     0,    72,    77,    82,     0,
       0,     0,     0,    21,    21,     0,     5,    35,     0,    57,
      58,    60,    59,    56,    55,    54,    48,    49,    52,    53,
      50,    51,    46,    47,    40,    44,    45,    42,    43,    41,
     111,   112,    39,    37,    21,    76,    80,    74,     0,    83,
     109,     0,     0,     0,    81,    66,    12,    15,     0,    29,
      22,     0,    23,     0,    34,    36,    38,     0,     0,    78,
       0,    84,     0,     0,    28,    27,     0,    18,    20,    14,
       0,   110,    16,    33,     0,     0,    26,    24,    25,    79,
      32,    31,     0,    30
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
    -103,  -103,  -103,     0,  -103,  -103,  -103,  -102,   -41,  -103,
    -103,     4,  -103,    48,  -103,    49,  -103,  -103,  -103,  -103
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,    42,    43,   150,   134,   103,   104,   151,   152,    66,
      45,   140,    47,    96,    97,    98,    48,   142,   133,    49
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      44,   136,   153,     6,    46,   160,    52,    58,    51,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,   166,    53,   161,
      29,    30,   157,    31,    54,    59,   166,    62,    63,    64,
      65,    68,    60,    55,    67,    93,    56,    61,    92,    93,
      65,   167,   100,   107,    99,    57,    69,    94,   101,   102,
     168,    94,    95,   156,   139,   108,    95,   130,   131,   132,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,   128,
     129,     1,     2,     3,     4,     5,   166,   143,     7,     8,
       9,    88,    89,    90,    91,   154,    92,   170,   155,   145,
     158,    83,    84,    85,    86,    87,    88,    89,    90,    91,
     169,    92,    28,    90,    91,   178,    92,    32,   159,    33,
      34,    35,    36,    86,    87,    88,    89,    90,    91,    37,
      92,   135,   137,   141,   179,   155,   144,     0,   162,   164,
      38,    39,     0,     0,     0,     0,    40,     0,    41,   149,
       0,     0,     0,   173,   171,   176,   177,     0,     0,     1,
       2,     3,     4,     5,   180,   181,     7,     8,     9,     0,
       0,     0,   183,     1,     2,     3,     4,     5,     0,     0,
       7,     8,     9,    85,    86,    87,    88,    89,    90,    91,
      28,    92,     0,     0,     0,    32,     0,    33,    34,    35,
      36,     0,     0,     0,    28,     0,     0,    37,     0,    32,
       0,    33,    34,    35,    36,     0,     0,     0,    38,    39,
       0,    37,     0,     0,    40,     0,    41,   163,     0,     0,
       0,     0,    38,    39,     0,     0,     0,     0,    40,     0,
      41,   175,     1,     2,     3,     4,     5,     6,     0,     7,
       8,     9,     0,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,     0,     0,    28,    29,    30,     0,    31,    32,     0,
      33,    34,    35,    36,     1,     2,     3,     4,     5,     0,
      37,     7,     8,     9,     0,     0,     0,     0,     0,     0,
       0,    38,    39,     0,     0,     0,     0,    40,     0,    41,
       0,     0,     0,     0,     0,    28,     0,     0,     0,     0,
      32,     0,    33,    34,    35,    36,     1,     2,     3,     4,
       5,     0,    37,     7,     8,     9,     0,     0,     0,     0,
       0,     0,     0,    38,    39,     0,     0,     0,     0,    50,
       0,    41,     0,     0,     0,     0,     0,    28,     0,     0,
       0,     0,    32,     0,    33,    34,    35,    36,     1,     2,
       3,     4,     5,     0,    37,     7,     8,     9,     0,     0,
       0,     0,     0,     0,     0,    38,    39,     0,     0,     0,
       0,    40,     0,    41,     0,    70,    71,     0,     0,    28,
       0,     0,     0,    72,    32,     0,    33,    34,    35,    36,
      73,     0,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
       6,    92,   146,    40,   138,    41,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,     0,     0,     0,    29,    30,     0,
      31,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    93,     0,     0,     0,     0,    70,    71,
       0,     0,     0,     0,     0,    94,    72,     0,     0,     0,
      95,   139,     0,    73,     0,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,     0,    92,    70,    71,   165,     0,     0,
       0,     0,     0,    72,     0,     0,     0,     0,     0,     0,
      73,     0,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
       0,    92,    70,    71,   174,     0,     0,     0,     0,     0,
      72,     0,     0,     0,     0,     0,     0,    73,     0,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,     0,    92,    70,
      71,   182,     0,     0,     0,     0,     0,    72,     0,     0,
       0,     0,   105,     0,    73,     0,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,     0,    92,   106,    70,    71,     0,
       0,     0,     0,     0,     0,    72,     0,     0,     0,     0,
       0,     0,    73,     0,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,     0,    92,   147,    70,    71,     0,     0,     0,
       0,     0,     0,    72,     0,     0,     0,     0,     0,     0,
      73,     0,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
       0,    92,   172,    70,    71,     0,     0,     0,     0,     0,
       0,    72,     0,     0,     0,     0,   148,     0,    73,     0,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,     0,    92,
      70,    71,     0,     0,     0,     0,     0,     0,    72,     0,
       0,     0,     0,     0,     0,    73,     0,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    70,    92,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
       0,    92,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
       6,    92,     0,     0,     0,     0,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,     0,     0,     0,    29,    30,     0,
      31,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,     0,
      92,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,     0,    92,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,     0,    92,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,     0,    92
};

static const yytype_int16 yycheck[] =
{
       0,     3,   104,     8,     0,    45,    68,    68,     8,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    45,    28,    69,
      35,    36,   134,    38,    28,    68,    45,    37,    38,    39,
      40,    41,    68,    37,    40,    51,    28,    68,    68,    51,
      50,    69,    52,    69,    50,    37,     0,    63,    58,    59,
      69,    63,    68,     9,    69,    69,    68,     7,     8,     9,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,     3,     4,     5,     6,     7,    45,    68,    10,    11,
      12,    63,    64,    65,    66,   105,    68,     3,   108,    69,
      47,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      69,    68,    34,    65,    66,   166,    68,    39,    69,    41,
      42,    43,    44,    61,    62,    63,    64,    65,    66,    51,
      68,    93,    94,    95,    69,   145,    97,    -1,   148,   149,
      62,    63,    -1,    -1,    -1,    -1,    68,    -1,    70,    71,
      -1,    -1,    -1,   163,   160,   165,   166,    -1,    -1,     3,
       4,     5,     6,     7,   174,   175,    10,    11,    12,    -1,
      -1,    -1,   182,     3,     4,     5,     6,     7,    -1,    -1,
      10,    11,    12,    60,    61,    62,    63,    64,    65,    66,
      34,    68,    -1,    -1,    -1,    39,    -1,    41,    42,    43,
      44,    -1,    -1,    -1,    34,    -1,    -1,    51,    -1,    39,
      -1,    41,    42,    43,    44,    -1,    -1,    -1,    62,    63,
      -1,    51,    -1,    -1,    68,    -1,    70,    71,    -1,    -1,
      -1,    -1,    62,    63,    -1,    -1,    -1,    -1,    68,    -1,
      70,    71,     3,     4,     5,     6,     7,     8,    -1,    10,
      11,    12,    -1,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    -1,    -1,    34,    35,    36,    -1,    38,    39,    -1,
      41,    42,    43,    44,     3,     4,     5,     6,     7,    -1,
      51,    10,    11,    12,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    62,    63,    -1,    -1,    -1,    -1,    68,    -1,    70,
      -1,    -1,    -1,    -1,    -1,    34,    -1,    -1,    -1,    -1,
      39,    -1,    41,    42,    43,    44,     3,     4,     5,     6,
       7,    -1,    51,    10,    11,    12,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    62,    63,    -1,    -1,    -1,    -1,    68,
      -1,    70,    -1,    -1,    -1,    -1,    -1,    34,    -1,    -1,
      -1,    -1,    39,    -1,    41,    42,    43,    44,     3,     4,
       5,     6,     7,    -1,    51,    10,    11,    12,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    62,    63,    -1,    -1,    -1,
      -1,    68,    -1,    70,    -1,    32,    33,    -1,    -1,    34,
      -1,    -1,    -1,    40,    39,    -1,    41,    42,    43,    44,
      47,    -1,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
       8,    68,    69,    68,    12,    70,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    -1,    -1,    -1,    35,    36,    -1,
      38,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    51,    -1,    -1,    -1,    -1,    32,    33,
      -1,    -1,    -1,    -1,    -1,    63,    40,    -1,    -1,    -1,
      68,    69,    -1,    47,    -1,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    -1,    68,    32,    33,    71,    -1,    -1,
      -1,    -1,    -1,    40,    -1,    -1,    -1,    -1,    -1,    -1,
      47,    -1,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      -1,    68,    32,    33,    71,    -1,    -1,    -1,    -1,    -1,
      40,    -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    -1,    68,    32,
      33,    71,    -1,    -1,    -1,    -1,    -1,    40,    -1,    -1,
      -1,    -1,    45,    -1,    47,    -1,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    -1,    68,    69,    32,    33,    -1,
      -1,    -1,    -1,    -1,    -1,    40,    -1,    -1,    -1,    -1,
      -1,    -1,    47,    -1,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    -1,    68,    69,    32,    33,    -1,    -1,    -1,
      -1,    -1,    -1,    40,    -1,    -1,    -1,    -1,    -1,    -1,
      47,    -1,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      -1,    68,    69,    32,    33,    -1,    -1,    -1,    -1,    -1,
      -1,    40,    -1,    -1,    -1,    -1,    45,    -1,    47,    -1,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    -1,    68,
      32,    33,    -1,    -1,    -1,    -1,    -1,    -1,    40,    -1,
      -1,    -1,    -1,    -1,    -1,    47,    -1,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    32,    68,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      -1,    68,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
       8,    68,    -1,    -1,    -1,    -1,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    -1,    -1,    -1,    35,    36,    -1,
      38,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    -1,
      68,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    -1,    68,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    -1,    68,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    -1,    68
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,    10,    11,    12,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    34,    35,
      36,    38,    39,    41,    42,    43,    44,    51,    62,    63,
      68,    70,    73,    74,    75,    82,    83,    84,    88,    91,
      68,    75,    68,    75,    28,    37,    28,    37,    68,    68,
      68,    68,    75,    75,    75,    75,    81,    83,    75,     0,
      32,    33,    40,    47,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    68,    51,    63,    68,    85,    86,    87,    83,
      75,    75,    75,    77,    78,    45,    69,    69,    69,    75,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    75,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    75,
       7,     8,     9,    90,    76,    85,     3,    85,    12,    69,
      83,    85,    89,    68,    87,    69,    69,    69,    45,    71,
      75,    79,    80,    79,    75,    75,     9,    79,    47,    69,
      45,    69,    75,    71,    75,    71,    45,    69,    69,    69,
       3,    83,    69,    75,    71,    71,    75,    75,    80,    69,
      75,    75,    71,    75
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    72,    73,    73,    74,    75,    75,    75,    75,    75,
      75,    75,    75,    76,    75,    75,    75,    77,    75,    78,
      75,    79,    79,    79,    79,    79,    80,    80,    80,    80,
      80,    80,    80,    80,    81,    75,    75,    75,    75,    75,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    75,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    75,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    82,
      83,    84,    84,    85,    85,    85,    85,    85,    86,    86,
      86,    86,    86,    87,    87,    88,    88,    88,    88,    88,
      88,    88,    88,    88,    88,    88,    88,    88,    88,    88,
      88,    88,    88,    88,    88,    88,    88,    88,    88,    89,
      89,    90,    90,    91
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     1,     3,     2,     2,     2,     2,
       2,     2,     4,     0,     5,     4,     6,     0,     5,     0,
       5,     0,     1,     1,     3,     3,     3,     2,     2,     1,
       5,     4,     4,     3,     3,     3,     4,     3,     4,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     1,     1,     1,     1,     1,     4,     1,     1,     1,
       1,     1,     2,     1,     2,     1,     2,     1,     3,     5,
       2,     2,     1,     2,     3,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     2,     2,     2,     2,     1,
       3,     1,     1,     1
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
#line 227 "f-exp.y"
                        { pstate->push_new<type_operation> ((yyvsp[0].tval)); }
#line 1679 "f-exp.c.tmp"
    break;

  case 5: /* exp: '(' exp ')'  */
#line 231 "f-exp.y"
                        { }
#line 1685 "f-exp.c.tmp"
    break;

  case 6: /* exp: '*' exp  */
#line 236 "f-exp.y"
                        { pstate->wrap<unop_ind_operation> (); }
#line 1691 "f-exp.c.tmp"
    break;

  case 7: /* exp: '&' exp  */
#line 240 "f-exp.y"
                        { pstate->wrap<unop_addr_operation> (); }
#line 1697 "f-exp.c.tmp"
    break;

  case 8: /* exp: '-' exp  */
#line 244 "f-exp.y"
                        { pstate->wrap<unary_neg_operation> (); }
#line 1703 "f-exp.c.tmp"
    break;

  case 9: /* exp: BOOL_NOT exp  */
#line 248 "f-exp.y"
                        { pstate->wrap<unary_logical_not_operation> (); }
#line 1709 "f-exp.c.tmp"
    break;

  case 10: /* exp: '~' exp  */
#line 252 "f-exp.y"
                        { pstate->wrap<unary_complement_operation> (); }
#line 1715 "f-exp.c.tmp"
    break;

  case 11: /* exp: SIZEOF exp  */
#line 256 "f-exp.y"
                        { pstate->wrap<unop_sizeof_operation> (); }
#line 1721 "f-exp.c.tmp"
    break;

  case 12: /* exp: KIND '(' exp ')'  */
#line 260 "f-exp.y"
                        { pstate->wrap<fortran_kind_operation> (); }
#line 1727 "f-exp.c.tmp"
    break;

  case 13: /* $@1: %empty  */
#line 269 "f-exp.y"
                        { pstate->start_arglist (); }
#line 1733 "f-exp.c.tmp"
    break;

  case 14: /* exp: exp '(' $@1 arglist ')'  */
#line 271 "f-exp.y"
                        {
			  std::vector<operation_up> args
			    = pstate->pop_vector (pstate->end_arglist ());
			  pstate->push_new<fortran_undetermined>
			    (pstate->pop (), std::move (args));
			}
#line 1744 "f-exp.c.tmp"
    break;

  case 15: /* exp: UNOP_INTRINSIC '(' exp ')'  */
#line 280 "f-exp.y"
                        {
			  wrap_unop_intrinsic ((yyvsp[-3].opcode));
			}
#line 1752 "f-exp.c.tmp"
    break;

  case 16: /* exp: BINOP_INTRINSIC '(' exp ',' exp ')'  */
#line 286 "f-exp.y"
                        {
			  wrap_binop_intrinsic ((yyvsp[-5].opcode));
			}
#line 1760 "f-exp.c.tmp"
    break;

  case 17: /* $@2: %empty  */
#line 292 "f-exp.y"
                        { pstate->start_arglist (); }
#line 1766 "f-exp.c.tmp"
    break;

  case 18: /* exp: UNOP_OR_BINOP_INTRINSIC '(' $@2 arglist ')'  */
#line 294 "f-exp.y"
                        {
			  const int n = pstate->end_arglist ();

			  switch (n)
			    {
			    case 1:
			      wrap_unop_intrinsic ((yyvsp[-4].opcode));
			      break;
			    case 2:
			      wrap_binop_intrinsic ((yyvsp[-4].opcode));
			      break;
			    default:
			      gdb_assert_not_reached
				("wrong number of arguments for intrinsics");
			    }
			}
#line 1787 "f-exp.c.tmp"
    break;

  case 19: /* $@3: %empty  */
#line 312 "f-exp.y"
                        { pstate->start_arglist (); }
#line 1793 "f-exp.c.tmp"
    break;

  case 20: /* exp: UNOP_OR_BINOP_OR_TERNOP_INTRINSIC '(' $@3 arglist ')'  */
#line 314 "f-exp.y"
                        {
			  const int n = pstate->end_arglist ();

			  switch (n)
			    {
			    case 1:
			      wrap_unop_intrinsic ((yyvsp[-4].opcode));
			      break;
			    case 2:
			      wrap_binop_intrinsic ((yyvsp[-4].opcode));
			      break;
			    case 3:
			      wrap_ternop_intrinsic ((yyvsp[-4].opcode));
			      break;
			    default:
			      gdb_assert_not_reached
				("wrong number of arguments for intrinsics");
			    }
			}
#line 1817 "f-exp.c.tmp"
    break;

  case 22: /* arglist: exp  */
#line 339 "f-exp.y"
                        { pstate->arglist_len = 1; }
#line 1823 "f-exp.c.tmp"
    break;

  case 23: /* arglist: subrange  */
#line 343 "f-exp.y"
                        { pstate->arglist_len = 1; }
#line 1829 "f-exp.c.tmp"
    break;

  case 24: /* arglist: arglist ',' exp  */
#line 347 "f-exp.y"
                        { pstate->arglist_len++; }
#line 1835 "f-exp.c.tmp"
    break;

  case 25: /* arglist: arglist ',' subrange  */
#line 351 "f-exp.y"
                        { pstate->arglist_len++; }
#line 1841 "f-exp.c.tmp"
    break;

  case 26: /* subrange: exp ':' exp  */
#line 357 "f-exp.y"
                        {
			  operation_up high = pstate->pop ();
			  operation_up low = pstate->pop ();
			  pstate->push_new<fortran_range_operation>
			    (RANGE_STANDARD, std::move (low),
			     std::move (high), operation_up ());
			}
#line 1853 "f-exp.c.tmp"
    break;

  case 27: /* subrange: exp ':'  */
#line 367 "f-exp.y"
                        {
			  operation_up low = pstate->pop ();
			  pstate->push_new<fortran_range_operation>
			    (RANGE_HIGH_BOUND_DEFAULT, std::move (low),
			     operation_up (), operation_up ());
			}
#line 1864 "f-exp.c.tmp"
    break;

  case 28: /* subrange: ':' exp  */
#line 376 "f-exp.y"
                        {
			  operation_up high = pstate->pop ();
			  pstate->push_new<fortran_range_operation>
			    (RANGE_LOW_BOUND_DEFAULT, operation_up (),
			     std::move (high), operation_up ());
			}
#line 1875 "f-exp.c.tmp"
    break;

  case 29: /* subrange: ':'  */
#line 385 "f-exp.y"
                        {
			  pstate->push_new<fortran_range_operation>
			    (RANGE_LOW_BOUND_DEFAULT
			     | RANGE_HIGH_BOUND_DEFAULT,
			     operation_up (), operation_up (),
			     operation_up ());
			}
#line 1887 "f-exp.c.tmp"
    break;

  case 30: /* subrange: exp ':' exp ':' exp  */
#line 396 "f-exp.y"
                        {
			  operation_up stride = pstate->pop ();
			  operation_up high = pstate->pop ();
			  operation_up low = pstate->pop ();
			  pstate->push_new<fortran_range_operation>
			    (RANGE_STANDARD | RANGE_HAS_STRIDE,
			     std::move (low), std::move (high),
			     std::move (stride));
			}
#line 1901 "f-exp.c.tmp"
    break;

  case 31: /* subrange: exp ':' ':' exp  */
#line 408 "f-exp.y"
                        {
			  operation_up stride = pstate->pop ();
			  operation_up low = pstate->pop ();
			  pstate->push_new<fortran_range_operation>
			    (RANGE_HIGH_BOUND_DEFAULT
			     | RANGE_HAS_STRIDE,
			     std::move (low), operation_up (),
			     std::move (stride));
			}
#line 1915 "f-exp.c.tmp"
    break;

  case 32: /* subrange: ':' exp ':' exp  */
#line 420 "f-exp.y"
                        {
			  operation_up stride = pstate->pop ();
			  operation_up high = pstate->pop ();
			  pstate->push_new<fortran_range_operation>
			    (RANGE_LOW_BOUND_DEFAULT
			     | RANGE_HAS_STRIDE,
			     operation_up (), std::move (high),
			     std::move (stride));
			}
#line 1929 "f-exp.c.tmp"
    break;

  case 33: /* subrange: ':' ':' exp  */
#line 432 "f-exp.y"
                        {
			  operation_up stride = pstate->pop ();
			  pstate->push_new<fortran_range_operation>
			    (RANGE_LOW_BOUND_DEFAULT
			     | RANGE_HIGH_BOUND_DEFAULT
			     | RANGE_HAS_STRIDE,
			     operation_up (), operation_up (),
			     std::move (stride));
			}
#line 1943 "f-exp.c.tmp"
    break;

  case 34: /* complexnum: exp ',' exp  */
#line 444 "f-exp.y"
                        { }
#line 1949 "f-exp.c.tmp"
    break;

  case 35: /* exp: '(' complexnum ')'  */
#line 448 "f-exp.y"
                        {
			  operation_up rhs = pstate->pop ();
			  operation_up lhs = pstate->pop ();
			  pstate->push_new<complex_operation>
			    (std::move (lhs), std::move (rhs),
			     parse_f_type (pstate)->builtin_complex_s16);
			}
#line 1961 "f-exp.c.tmp"
    break;

  case 36: /* exp: '(' type ')' exp  */
#line 458 "f-exp.y"
                        {
			  pstate->push_new<unop_cast_operation>
			    (pstate->pop (), (yyvsp[-2].tval));
			}
#line 1970 "f-exp.c.tmp"
    break;

  case 37: /* exp: exp '%' name  */
#line 465 "f-exp.y"
                        {
			  pstate->push_new<fortran_structop_operation>
			    (pstate->pop (), copy_name ((yyvsp[0].sval)));
			}
#line 1979 "f-exp.c.tmp"
    break;

  case 38: /* exp: exp '%' name COMPLETE  */
#line 472 "f-exp.y"
                        {
			  structop_base_operation *op
			    = new fortran_structop_operation (pstate->pop (),
							      copy_name ((yyvsp[-1].sval)));
			  pstate->mark_struct_expression (op);
			  pstate->push (operation_up (op));
			}
#line 1991 "f-exp.c.tmp"
    break;

  case 39: /* exp: exp '%' COMPLETE  */
#line 482 "f-exp.y"
                        {
			  structop_base_operation *op
			    = new fortran_structop_operation (pstate->pop (),
							      "");
			  pstate->mark_struct_expression (op);
			  pstate->push (operation_up (op));
			}
#line 2003 "f-exp.c.tmp"
    break;

  case 40: /* exp: exp '@' exp  */
#line 494 "f-exp.y"
                        { pstate->wrap2<repeat_operation> (); }
#line 2009 "f-exp.c.tmp"
    break;

  case 41: /* exp: exp STARSTAR exp  */
#line 498 "f-exp.y"
                        { pstate->wrap2<exp_operation> (); }
#line 2015 "f-exp.c.tmp"
    break;

  case 42: /* exp: exp '*' exp  */
#line 502 "f-exp.y"
                        { pstate->wrap2<mul_operation> (); }
#line 2021 "f-exp.c.tmp"
    break;

  case 43: /* exp: exp '/' exp  */
#line 506 "f-exp.y"
                        { pstate->wrap2<div_operation> (); }
#line 2027 "f-exp.c.tmp"
    break;

  case 44: /* exp: exp '+' exp  */
#line 510 "f-exp.y"
                        { pstate->wrap2<add_operation> (); }
#line 2033 "f-exp.c.tmp"
    break;

  case 45: /* exp: exp '-' exp  */
#line 514 "f-exp.y"
                        { pstate->wrap2<sub_operation> (); }
#line 2039 "f-exp.c.tmp"
    break;

  case 46: /* exp: exp LSH exp  */
#line 518 "f-exp.y"
                        { pstate->wrap2<lsh_operation> (); }
#line 2045 "f-exp.c.tmp"
    break;

  case 47: /* exp: exp RSH exp  */
#line 522 "f-exp.y"
                        { pstate->wrap2<rsh_operation> (); }
#line 2051 "f-exp.c.tmp"
    break;

  case 48: /* exp: exp EQUAL exp  */
#line 526 "f-exp.y"
                        { pstate->wrap2<equal_operation> (); }
#line 2057 "f-exp.c.tmp"
    break;

  case 49: /* exp: exp NOTEQUAL exp  */
#line 530 "f-exp.y"
                        { pstate->wrap2<notequal_operation> (); }
#line 2063 "f-exp.c.tmp"
    break;

  case 50: /* exp: exp LEQ exp  */
#line 534 "f-exp.y"
                        { pstate->wrap2<leq_operation> (); }
#line 2069 "f-exp.c.tmp"
    break;

  case 51: /* exp: exp GEQ exp  */
#line 538 "f-exp.y"
                        { pstate->wrap2<geq_operation> (); }
#line 2075 "f-exp.c.tmp"
    break;

  case 52: /* exp: exp LESSTHAN exp  */
#line 542 "f-exp.y"
                        { pstate->wrap2<less_operation> (); }
#line 2081 "f-exp.c.tmp"
    break;

  case 53: /* exp: exp GREATERTHAN exp  */
#line 546 "f-exp.y"
                        { pstate->wrap2<gtr_operation> (); }
#line 2087 "f-exp.c.tmp"
    break;

  case 54: /* exp: exp '&' exp  */
#line 550 "f-exp.y"
                        { pstate->wrap2<bitwise_and_operation> (); }
#line 2093 "f-exp.c.tmp"
    break;

  case 55: /* exp: exp '^' exp  */
#line 554 "f-exp.y"
                        { pstate->wrap2<bitwise_xor_operation> (); }
#line 2099 "f-exp.c.tmp"
    break;

  case 56: /* exp: exp '|' exp  */
#line 558 "f-exp.y"
                        { pstate->wrap2<bitwise_ior_operation> (); }
#line 2105 "f-exp.c.tmp"
    break;

  case 57: /* exp: exp BOOL_AND exp  */
#line 562 "f-exp.y"
                        { pstate->wrap2<logical_and_operation> (); }
#line 2111 "f-exp.c.tmp"
    break;

  case 58: /* exp: exp BOOL_OR exp  */
#line 567 "f-exp.y"
                        { pstate->wrap2<logical_or_operation> (); }
#line 2117 "f-exp.c.tmp"
    break;

  case 59: /* exp: exp '=' exp  */
#line 571 "f-exp.y"
                        { pstate->wrap2<assign_operation> (); }
#line 2123 "f-exp.c.tmp"
    break;

  case 60: /* exp: exp ASSIGN_MODIFY exp  */
#line 575 "f-exp.y"
                        {
			  operation_up rhs = pstate->pop ();
			  operation_up lhs = pstate->pop ();
			  pstate->push_new<assign_modify_operation>
			    ((yyvsp[-1].opcode), std::move (lhs), std::move (rhs));
			}
#line 2134 "f-exp.c.tmp"
    break;

  case 61: /* exp: INT  */
#line 584 "f-exp.y"
                        {
			  pstate->push_new<long_const_operation>
			    ((yyvsp[0].typed_val).type, (yyvsp[0].typed_val).val);
			}
#line 2143 "f-exp.c.tmp"
    break;

  case 62: /* exp: NAME_OR_INT  */
#line 591 "f-exp.y"
                        { YYSTYPE val;
			  parse_number (pstate, (yyvsp[0].ssym).stoken.ptr,
					(yyvsp[0].ssym).stoken.length, 0, &val);
			  pstate->push_new<long_const_operation>
			    (val.typed_val.type,
			     val.typed_val.val);
			}
#line 2155 "f-exp.c.tmp"
    break;

  case 63: /* exp: FLOAT  */
#line 601 "f-exp.y"
                        {
			  float_data data;
			  std::copy (std::begin ((yyvsp[0].typed_val_float).val), std::end ((yyvsp[0].typed_val_float).val),
				     std::begin (data));
			  pstate->push_new<float_const_operation> ((yyvsp[0].typed_val_float).type, data);
			}
#line 2166 "f-exp.c.tmp"
    break;

  case 65: /* exp: DOLLAR_VARIABLE  */
#line 613 "f-exp.y"
                        { pstate->push_dollar ((yyvsp[0].sval)); }
#line 2172 "f-exp.c.tmp"
    break;

  case 66: /* exp: SIZEOF '(' type ')'  */
#line 617 "f-exp.y"
                        {
			  (yyvsp[-1].tval) = check_typedef ((yyvsp[-1].tval));
			  pstate->push_new<long_const_operation>
			    (parse_f_type (pstate)->builtin_integer,
			     (yyvsp[-1].tval)->length ());
			}
#line 2183 "f-exp.c.tmp"
    break;

  case 67: /* exp: BOOLEAN_LITERAL  */
#line 626 "f-exp.y"
                        { pstate->push_new<bool_operation> ((yyvsp[0].lval)); }
#line 2189 "f-exp.c.tmp"
    break;

  case 68: /* exp: STRING_LITERAL  */
#line 630 "f-exp.y"
                        {
			  pstate->push_new<string_operation>
			    (copy_name ((yyvsp[0].sval)));
			}
#line 2198 "f-exp.c.tmp"
    break;

  case 69: /* variable: name_not_typename  */
#line 637 "f-exp.y"
                        { struct block_symbol sym = (yyvsp[0].ssym).sym;
			  std::string name = copy_name ((yyvsp[0].ssym).stoken);
			  pstate->push_symbol (name.c_str (), sym);
			}
#line 2207 "f-exp.c.tmp"
    break;

  case 72: /* ptype: typebase abs_decl  */
#line 649 "f-exp.y"
                {
		  /* This is where the interesting stuff happens.  */
		  int done = 0;
		  int array_size;
		  struct type *follow_type = (yyvsp[-1].tval);
		  struct type *range_type;
		  
		  while (!done)
		    switch (type_stack->pop ())
		      {
		      case tp_end:
			done = 1;
			break;
		      case tp_pointer:
			follow_type = lookup_pointer_type (follow_type);
			break;
		      case tp_reference:
			follow_type = lookup_lvalue_reference_type (follow_type);
			break;
		      case tp_array:
			array_size = type_stack->pop_int ();
			if (array_size != -1)
			  {
			    range_type =
			      create_static_range_type ((struct type *) NULL,
							parse_f_type (pstate)
							->builtin_integer,
							0, array_size - 1);
			    follow_type =
			      create_array_type ((struct type *) NULL,
						 follow_type, range_type);
			  }
			else
			  follow_type = lookup_pointer_type (follow_type);
			break;
		      case tp_function:
			follow_type = lookup_function_type (follow_type);
			break;
		      case tp_kind:
			{
			  int kind_val = type_stack->pop_int ();
			  follow_type
			    = convert_to_kind_type (follow_type, kind_val);
			}
			break;
		      }
		  (yyval.tval) = follow_type;
		}
#line 2260 "f-exp.c.tmp"
    break;

  case 73: /* abs_decl: '*'  */
#line 700 "f-exp.y"
                        { type_stack->push (tp_pointer); (yyval.voidval) = 0; }
#line 2266 "f-exp.c.tmp"
    break;

  case 74: /* abs_decl: '*' abs_decl  */
#line 702 "f-exp.y"
                        { type_stack->push (tp_pointer); (yyval.voidval) = (yyvsp[0].voidval); }
#line 2272 "f-exp.c.tmp"
    break;

  case 75: /* abs_decl: '&'  */
#line 704 "f-exp.y"
                        { type_stack->push (tp_reference); (yyval.voidval) = 0; }
#line 2278 "f-exp.c.tmp"
    break;

  case 76: /* abs_decl: '&' abs_decl  */
#line 706 "f-exp.y"
                        { type_stack->push (tp_reference); (yyval.voidval) = (yyvsp[0].voidval); }
#line 2284 "f-exp.c.tmp"
    break;

  case 78: /* direct_abs_decl: '(' abs_decl ')'  */
#line 711 "f-exp.y"
                        { (yyval.voidval) = (yyvsp[-1].voidval); }
#line 2290 "f-exp.c.tmp"
    break;

  case 79: /* direct_abs_decl: '(' KIND '=' INT ')'  */
#line 713 "f-exp.y"
                        { push_kind_type ((yyvsp[-1].typed_val).val, (yyvsp[-1].typed_val).type); }
#line 2296 "f-exp.c.tmp"
    break;

  case 80: /* direct_abs_decl: '*' INT  */
#line 715 "f-exp.y"
                        { push_kind_type ((yyvsp[0].typed_val).val, (yyvsp[0].typed_val).type); }
#line 2302 "f-exp.c.tmp"
    break;

  case 81: /* direct_abs_decl: direct_abs_decl func_mod  */
#line 717 "f-exp.y"
                        { type_stack->push (tp_function); }
#line 2308 "f-exp.c.tmp"
    break;

  case 82: /* direct_abs_decl: func_mod  */
#line 719 "f-exp.y"
                        { type_stack->push (tp_function); }
#line 2314 "f-exp.c.tmp"
    break;

  case 83: /* func_mod: '(' ')'  */
#line 723 "f-exp.y"
                        { (yyval.voidval) = 0; }
#line 2320 "f-exp.c.tmp"
    break;

  case 84: /* func_mod: '(' nonempty_typelist ')'  */
#line 725 "f-exp.y"
                        { xfree ((yyvsp[-1].tvec)); (yyval.voidval) = 0; }
#line 2326 "f-exp.c.tmp"
    break;

  case 85: /* typebase: TYPENAME  */
#line 730 "f-exp.y"
                        { (yyval.tval) = (yyvsp[0].tsym).type; }
#line 2332 "f-exp.c.tmp"
    break;

  case 86: /* typebase: INT_S1_KEYWORD  */
#line 732 "f-exp.y"
                        { (yyval.tval) = parse_f_type (pstate)->builtin_integer_s1; }
#line 2338 "f-exp.c.tmp"
    break;

  case 87: /* typebase: INT_S2_KEYWORD  */
#line 734 "f-exp.y"
                        { (yyval.tval) = parse_f_type (pstate)->builtin_integer_s2; }
#line 2344 "f-exp.c.tmp"
    break;

  case 88: /* typebase: INT_KEYWORD  */
#line 736 "f-exp.y"
                        { (yyval.tval) = parse_f_type (pstate)->builtin_integer; }
#line 2350 "f-exp.c.tmp"
    break;

  case 89: /* typebase: INT_S4_KEYWORD  */
#line 738 "f-exp.y"
                        { (yyval.tval) = parse_f_type (pstate)->builtin_integer; }
#line 2356 "f-exp.c.tmp"
    break;

  case 90: /* typebase: INT_S8_KEYWORD  */
#line 740 "f-exp.y"
                        { (yyval.tval) = parse_f_type (pstate)->builtin_integer_s8; }
#line 2362 "f-exp.c.tmp"
    break;

  case 91: /* typebase: CHARACTER  */
#line 742 "f-exp.y"
                        { (yyval.tval) = parse_f_type (pstate)->builtin_character; }
#line 2368 "f-exp.c.tmp"
    break;

  case 92: /* typebase: LOGICAL_S1_KEYWORD  */
#line 744 "f-exp.y"
                        { (yyval.tval) = parse_f_type (pstate)->builtin_logical_s1; }
#line 2374 "f-exp.c.tmp"
    break;

  case 93: /* typebase: LOGICAL_S2_KEYWORD  */
#line 746 "f-exp.y"
                        { (yyval.tval) = parse_f_type (pstate)->builtin_logical_s2; }
#line 2380 "f-exp.c.tmp"
    break;

  case 94: /* typebase: LOGICAL_KEYWORD  */
#line 748 "f-exp.y"
                        { (yyval.tval) = parse_f_type (pstate)->builtin_logical; }
#line 2386 "f-exp.c.tmp"
    break;

  case 95: /* typebase: LOGICAL_S4_KEYWORD  */
#line 750 "f-exp.y"
                        { (yyval.tval) = parse_f_type (pstate)->builtin_logical; }
#line 2392 "f-exp.c.tmp"
    break;

  case 96: /* typebase: LOGICAL_S8_KEYWORD  */
#line 752 "f-exp.y"
                        { (yyval.tval) = parse_f_type (pstate)->builtin_logical_s8; }
#line 2398 "f-exp.c.tmp"
    break;

  case 97: /* typebase: REAL_KEYWORD  */
#line 754 "f-exp.y"
                        { (yyval.tval) = parse_f_type (pstate)->builtin_real; }
#line 2404 "f-exp.c.tmp"
    break;

  case 98: /* typebase: REAL_S4_KEYWORD  */
#line 756 "f-exp.y"
                        { (yyval.tval) = parse_f_type (pstate)->builtin_real; }
#line 2410 "f-exp.c.tmp"
    break;

  case 99: /* typebase: REAL_S8_KEYWORD  */
#line 758 "f-exp.y"
                        { (yyval.tval) = parse_f_type (pstate)->builtin_real_s8; }
#line 2416 "f-exp.c.tmp"
    break;

  case 100: /* typebase: REAL_S16_KEYWORD  */
#line 760 "f-exp.y"
                        { (yyval.tval) = parse_f_type (pstate)->builtin_real_s16; }
#line 2422 "f-exp.c.tmp"
    break;

  case 101: /* typebase: COMPLEX_KEYWORD  */
#line 762 "f-exp.y"
                        { (yyval.tval) = parse_f_type (pstate)->builtin_complex; }
#line 2428 "f-exp.c.tmp"
    break;

  case 102: /* typebase: COMPLEX_S4_KEYWORD  */
#line 764 "f-exp.y"
                        { (yyval.tval) = parse_f_type (pstate)->builtin_complex; }
#line 2434 "f-exp.c.tmp"
    break;

  case 103: /* typebase: COMPLEX_S8_KEYWORD  */
#line 766 "f-exp.y"
                        { (yyval.tval) = parse_f_type (pstate)->builtin_complex_s8; }
#line 2440 "f-exp.c.tmp"
    break;

  case 104: /* typebase: COMPLEX_S16_KEYWORD  */
#line 768 "f-exp.y"
                        { (yyval.tval) = parse_f_type (pstate)->builtin_complex_s16; }
#line 2446 "f-exp.c.tmp"
    break;

  case 105: /* typebase: SINGLE PRECISION  */
#line 770 "f-exp.y"
                        { (yyval.tval) = parse_f_type (pstate)->builtin_real;}
#line 2452 "f-exp.c.tmp"
    break;

  case 106: /* typebase: DOUBLE PRECISION  */
#line 772 "f-exp.y"
                        { (yyval.tval) = parse_f_type (pstate)->builtin_real_s8;}
#line 2458 "f-exp.c.tmp"
    break;

  case 107: /* typebase: SINGLE COMPLEX_KEYWORD  */
#line 774 "f-exp.y"
                        { (yyval.tval) = parse_f_type (pstate)->builtin_complex;}
#line 2464 "f-exp.c.tmp"
    break;

  case 108: /* typebase: DOUBLE COMPLEX_KEYWORD  */
#line 776 "f-exp.y"
                        { (yyval.tval) = parse_f_type (pstate)->builtin_complex_s8;}
#line 2470 "f-exp.c.tmp"
    break;

  case 109: /* nonempty_typelist: type  */
#line 781 "f-exp.y"
                { (yyval.tvec) = (struct type **) xmalloc (sizeof (struct type *) * 2);
		  (yyval.ivec)[0] = 1;	/* Number of types in vector */
		  (yyval.tvec)[1] = (yyvsp[0].tval);
		}
#line 2479 "f-exp.c.tmp"
    break;

  case 110: /* nonempty_typelist: nonempty_typelist ',' type  */
#line 786 "f-exp.y"
                { int len = sizeof (struct type *) * (++((yyvsp[-2].ivec)[0]) + 1);
		  (yyval.tvec) = (struct type **) xrealloc ((char *) (yyvsp[-2].tvec), len);
		  (yyval.tvec)[(yyval.ivec)[0]] = (yyvsp[0].tval);
		}
#line 2488 "f-exp.c.tmp"
    break;

  case 111: /* name: NAME  */
#line 794 "f-exp.y"
                { (yyval.sval) = (yyvsp[0].ssym).stoken; }
#line 2494 "f-exp.c.tmp"
    break;

  case 112: /* name: TYPENAME  */
#line 796 "f-exp.y"
                { (yyval.sval) = (yyvsp[0].tsym).stoken; }
#line 2500 "f-exp.c.tmp"
    break;


#line 2504 "f-exp.c.tmp"

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

#line 809 "f-exp.y"


/* Called to match intrinsic function calls with one argument to their
   respective implementation and push the operation.  */

static void
wrap_unop_intrinsic (exp_opcode code)
{
  switch (code)
    {
    case UNOP_ABS:
      pstate->wrap<fortran_abs_operation> ();
      break;
    case FORTRAN_FLOOR:
      pstate->wrap<fortran_floor_operation_1arg> ();
      break;
    case FORTRAN_CEILING:
      pstate->wrap<fortran_ceil_operation_1arg> ();
      break;
    case UNOP_FORTRAN_ALLOCATED:
      pstate->wrap<fortran_allocated_operation> ();
      break;
    case UNOP_FORTRAN_RANK:
      pstate->wrap<fortran_rank_operation> ();
      break;
    case UNOP_FORTRAN_SHAPE:
      pstate->wrap<fortran_array_shape_operation> ();
      break;
    case UNOP_FORTRAN_LOC:
      pstate->wrap<fortran_loc_operation> ();
      break;
    case FORTRAN_ASSOCIATED:
      pstate->wrap<fortran_associated_1arg> ();
      break;
    case FORTRAN_ARRAY_SIZE:
      pstate->wrap<fortran_array_size_1arg> ();
      break;
    case FORTRAN_CMPLX:
      pstate->wrap<fortran_cmplx_operation_1arg> ();
      break;
    case FORTRAN_LBOUND:
    case FORTRAN_UBOUND:
      pstate->push_new<fortran_bound_1arg> (code, pstate->pop ());
      break;
    default:
      gdb_assert_not_reached ("unhandled intrinsic");
    }
}

/* Called to match intrinsic function calls with two arguments to their
   respective implementation and push the operation.  */

static void
wrap_binop_intrinsic (exp_opcode code)
{
  switch (code)
    {
    case FORTRAN_FLOOR:
      fortran_wrap2_kind<fortran_floor_operation_2arg>
	(parse_f_type (pstate)->builtin_integer);
      break;
    case FORTRAN_CEILING:
      fortran_wrap2_kind<fortran_ceil_operation_2arg>
	(parse_f_type (pstate)->builtin_integer);
      break;
    case BINOP_MOD:
      pstate->wrap2<fortran_mod_operation> ();
      break;
    case BINOP_FORTRAN_MODULO:
      pstate->wrap2<fortran_modulo_operation> ();
      break;
    case FORTRAN_CMPLX:
      pstate->wrap2<fortran_cmplx_operation_2arg> ();
      break;
    case FORTRAN_ASSOCIATED:
      pstate->wrap2<fortran_associated_2arg> ();
      break;
    case FORTRAN_ARRAY_SIZE:
      pstate->wrap2<fortran_array_size_2arg> ();
      break;
    case FORTRAN_LBOUND:
    case FORTRAN_UBOUND:
      {
	operation_up arg2 = pstate->pop ();
	operation_up arg1 = pstate->pop ();
	pstate->push_new<fortran_bound_2arg> (code, std::move (arg1),
					      std::move (arg2));
      }
      break;
    default:
      gdb_assert_not_reached ("unhandled intrinsic");
    }
}

/* Called to match intrinsic function calls with three arguments to their
   respective implementation and push the operation.  */

static void
wrap_ternop_intrinsic (exp_opcode code)
{
  switch (code)
    {
    case FORTRAN_LBOUND:
    case FORTRAN_UBOUND:
      {
	operation_up kind_arg = pstate->pop ();
	operation_up arg2 = pstate->pop ();
	operation_up arg1 = pstate->pop ();

	value *val = kind_arg->evaluate (nullptr, pstate->expout.get (),
					 EVAL_AVOID_SIDE_EFFECTS);
	gdb_assert (val != nullptr);

	type *follow_type
	  = convert_to_kind_type (parse_f_type (pstate)->builtin_integer,
				  value_as_long (val));

	pstate->push_new<fortran_bound_3arg> (code, std::move (arg1),
					      std::move (arg2), follow_type);
      }
      break;
    case FORTRAN_ARRAY_SIZE:
      fortran_wrap3_kind<fortran_array_size_3arg>
	(parse_f_type (pstate)->builtin_integer);
      break;
    case FORTRAN_CMPLX:
      fortran_wrap3_kind<fortran_cmplx_operation_3arg>
	(parse_f_type (pstate)->builtin_complex);
      break;
    default:
      gdb_assert_not_reached ("unhandled intrinsic");
    }
}

/* A helper that pops two operations (similar to wrap2), evaluates the last one
   assuming it is a kind parameter, and wraps them in some other operation
   pushing it to the stack.  */

template<typename T>
static void
fortran_wrap2_kind (type *base_type)
{
  operation_up kind_arg = pstate->pop ();
  operation_up arg = pstate->pop ();

  value *val = kind_arg->evaluate (nullptr, pstate->expout.get (),
				   EVAL_AVOID_SIDE_EFFECTS);
  gdb_assert (val != nullptr);

  type *follow_type = convert_to_kind_type (base_type, value_as_long (val));

  pstate->push_new<T> (std::move (arg), follow_type);
}

/* A helper that pops three operations, evaluates the last one assuming it is a
   kind parameter, and wraps them in some other operation pushing it to the
   stack.  */

template<typename T>
static void
fortran_wrap3_kind (type *base_type)
{
  operation_up kind_arg = pstate->pop ();
  operation_up arg2 = pstate->pop ();
  operation_up arg1 = pstate->pop ();

  value *val = kind_arg->evaluate (nullptr, pstate->expout.get (),
				   EVAL_AVOID_SIDE_EFFECTS);
  gdb_assert (val != nullptr);

  type *follow_type = convert_to_kind_type (base_type, value_as_long (val));

  pstate->push_new<T> (std::move (arg1), std::move (arg2), follow_type);
}

/* Take care of parsing a number (anything that starts with a digit).
   Set yylval and return the token type; update lexptr.
   LEN is the number of characters in it.  */

/*** Needs some error checking for the float case ***/

static int
parse_number (struct parser_state *par_state,
	      const char *p, int len, int parsed_float, YYSTYPE *putithere)
{
  ULONGEST n = 0;
  ULONGEST prevn = 0;
  int c;
  int base = input_radix;
  int unsigned_p = 0;
  int long_p = 0;
  ULONGEST high_bit;
  struct type *signed_type;
  struct type *unsigned_type;

  if (parsed_float)
    {
      /* It's a float since it contains a point or an exponent.  */
      /* [dD] is not understood as an exponent by parse_float,
	 change it to 'e'.  */
      char *tmp, *tmp2;

      tmp = xstrdup (p);
      for (tmp2 = tmp; *tmp2; ++tmp2)
	if (*tmp2 == 'd' || *tmp2 == 'D')
	  *tmp2 = 'e';

      /* FIXME: Should this use different types?  */
      putithere->typed_val_float.type = parse_f_type (pstate)->builtin_real_s8;
      bool parsed = parse_float (tmp, len,
				 putithere->typed_val_float.type,
				 putithere->typed_val_float.val);
      xfree (tmp);
      return parsed? FLOAT : ERROR;
    }

  /* Handle base-switching prefixes 0x, 0t, 0d, 0 */
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
      if (isupper (c))
	c = tolower (c);
      if (len == 0 && c == 'l')
	long_p = 1;
      else if (len == 0 && c == 'u')
	unsigned_p = 1;
      else
	{
	  int i;
	  if (c >= '0' && c <= '9')
	    i = c - '0';
	  else if (c >= 'a' && c <= 'f')
	    i = c - 'a' + 10;
	  else
	    return ERROR;	/* Char not a digit */
	  if (i >= base)
	    return ERROR;		/* Invalid digit in this base */
	  n *= base;
	  n += i;
	}
      /* Test for overflow.  */
      if (prevn == 0 && n == 0)
	;
      else if (RANGE_CHECK && prevn >= n)
	range_error (_("Overflow on numeric constant."));
      prevn = n;
    }
  
  /* If the number is too big to be an int, or it's got an l suffix
     then it's a long.  Work out if this has to be a long by
     shifting right and seeing if anything remains, and the
     target int size is different to the target long size.
     
     In the expression below, we could have tested
     (n >> gdbarch_int_bit (parse_gdbarch))
     to see if it was zero,
     but too many compilers warn about that, when ints and longs
     are the same size.  So we shift it twice, with fewer bits
     each time, for the same result.  */

  int bits_available;
  if ((gdbarch_int_bit (par_state->gdbarch ())
       != gdbarch_long_bit (par_state->gdbarch ())
       && ((n >> 2)
	   >> (gdbarch_int_bit (par_state->gdbarch ())-2))) /* Avoid
							    shift warning */
      || long_p)
    {
      bits_available = gdbarch_long_bit (par_state->gdbarch ());
      unsigned_type = parse_type (par_state)->builtin_unsigned_long;
      signed_type = parse_type (par_state)->builtin_long;
  }
  else 
    {
      bits_available = gdbarch_int_bit (par_state->gdbarch ());
      unsigned_type = parse_type (par_state)->builtin_unsigned_int;
      signed_type = parse_type (par_state)->builtin_int;
    }    
  high_bit = ((ULONGEST)1) << (bits_available - 1);
  
  if (RANGE_CHECK
      && ((n >> 2) >> (bits_available - 2)))
    range_error (_("Overflow on numeric constant."));

  putithere->typed_val.val = n;
  
  /* If the high bit of the worked out type is set then this number
     has to be unsigned.  */
  
  if (unsigned_p || (n & high_bit)) 
    putithere->typed_val.type = unsigned_type;
  else 
    putithere->typed_val.type = signed_type;
  
  return INT;
}

/* Called to setup the type stack when we encounter a '(kind=N)' type
   modifier, performs some bounds checking on 'N' and then pushes this to
   the type stack followed by the 'tp_kind' marker.  */
static void
push_kind_type (LONGEST val, struct type *type)
{
  int ival;

  if (type->is_unsigned ())
    {
      ULONGEST uval = static_cast <ULONGEST> (val);
      if (uval > INT_MAX)
	error (_("kind value out of range"));
      ival = static_cast <int> (uval);
    }
  else
    {
      if (val > INT_MAX || val < 0)
	error (_("kind value out of range"));
      ival = static_cast <int> (val);
    }

  type_stack->push (ival);
  type_stack->push (tp_kind);
}

/* Called when a type has a '(kind=N)' modifier after it, for example
   'character(kind=1)'.  The BASETYPE is the type described by 'character'
   in our example, and KIND is the integer '1'.  This function returns a
   new type that represents the basetype of a specific kind.  */
static struct type *
convert_to_kind_type (struct type *basetype, int kind)
{
  if (basetype == parse_f_type (pstate)->builtin_character)
    {
      /* Character of kind 1 is a special case, this is the same as the
	 base character type.  */
      if (kind == 1)
	return parse_f_type (pstate)->builtin_character;
    }
  else if (basetype == parse_f_type (pstate)->builtin_complex)
    {
      if (kind == 4)
	return parse_f_type (pstate)->builtin_complex;
      else if (kind == 8)
	return parse_f_type (pstate)->builtin_complex_s8;
      else if (kind == 16)
	return parse_f_type (pstate)->builtin_complex_s16;
    }
  else if (basetype == parse_f_type (pstate)->builtin_real)
    {
      if (kind == 4)
	return parse_f_type (pstate)->builtin_real;
      else if (kind == 8)
	return parse_f_type (pstate)->builtin_real_s8;
      else if (kind == 16)
	return parse_f_type (pstate)->builtin_real_s16;
    }
  else if (basetype == parse_f_type (pstate)->builtin_logical)
    {
      if (kind == 1)
	return parse_f_type (pstate)->builtin_logical_s1;
      else if (kind == 2)
	return parse_f_type (pstate)->builtin_logical_s2;
      else if (kind == 4)
	return parse_f_type (pstate)->builtin_logical;
      else if (kind == 8)
	return parse_f_type (pstate)->builtin_logical_s8;
    }
  else if (basetype == parse_f_type (pstate)->builtin_integer)
    {
      if (kind == 1)
	return parse_f_type (pstate)->builtin_integer_s1;
      else if (kind == 2)
	return parse_f_type (pstate)->builtin_integer_s2;
      else if (kind == 4)
	return parse_f_type (pstate)->builtin_integer;
      else if (kind == 8)
	return parse_f_type (pstate)->builtin_integer_s8;
    }

  error (_("unsupported kind %d for type %s"),
	 kind, TYPE_SAFE_NAME (basetype));

  /* Should never get here.  */
  return nullptr;
}

struct token
{
  /* The string to match against.  */
  const char *oper;

  /* The lexer token to return.  */
  int token;

  /* The expression opcode to embed within the token.  */
  enum exp_opcode opcode;

  /* When this is true the string in OPER is matched exactly including
     case, when this is false OPER is matched case insensitively.  */
  bool case_sensitive;
};

/* List of Fortran operators.  */

static const struct token fortran_operators[] =
{
  { ".and.", BOOL_AND, OP_NULL, false },
  { ".or.", BOOL_OR, OP_NULL, false },
  { ".not.", BOOL_NOT, OP_NULL, false },
  { ".eq.", EQUAL, OP_NULL, false },
  { ".eqv.", EQUAL, OP_NULL, false },
  { ".neqv.", NOTEQUAL, OP_NULL, false },
  { ".xor.", NOTEQUAL, OP_NULL, false },
  { "==", EQUAL, OP_NULL, false },
  { ".ne.", NOTEQUAL, OP_NULL, false },
  { "/=", NOTEQUAL, OP_NULL, false },
  { ".le.", LEQ, OP_NULL, false },
  { "<=", LEQ, OP_NULL, false },
  { ".ge.", GEQ, OP_NULL, false },
  { ">=", GEQ, OP_NULL, false },
  { ".gt.", GREATERTHAN, OP_NULL, false },
  { ">", GREATERTHAN, OP_NULL, false },
  { ".lt.", LESSTHAN, OP_NULL, false },
  { "<", LESSTHAN, OP_NULL, false },
  { "**", STARSTAR, BINOP_EXP, false },
};

/* Holds the Fortran representation of a boolean, and the integer value we
   substitute in when one of the matching strings is parsed.  */
struct f77_boolean_val
{
  /* The string representing a Fortran boolean.  */
  const char *name;

  /* The integer value to replace it with.  */
  int value;
};

/* The set of Fortran booleans.  These are matched case insensitively.  */
static const struct f77_boolean_val boolean_values[]  =
{
  { ".true.", 1 },
  { ".false.", 0 }
};

static const token f_keywords[] =
{
  /* Historically these have always been lowercase only in GDB.  */
  { "character", CHARACTER, OP_NULL, true },
  { "complex", COMPLEX_KEYWORD, OP_NULL, true },
  { "complex_4", COMPLEX_S4_KEYWORD, OP_NULL, true },
  { "complex_8", COMPLEX_S8_KEYWORD, OP_NULL, true },
  { "complex_16", COMPLEX_S16_KEYWORD, OP_NULL, true },
  { "integer_1", INT_S1_KEYWORD, OP_NULL, true },
  { "integer_2", INT_S2_KEYWORD, OP_NULL, true },
  { "integer_4", INT_S4_KEYWORD, OP_NULL, true },
  { "integer", INT_KEYWORD, OP_NULL, true },
  { "integer_8", INT_S8_KEYWORD, OP_NULL, true },
  { "logical_1", LOGICAL_S1_KEYWORD, OP_NULL, true },
  { "logical_2", LOGICAL_S2_KEYWORD, OP_NULL, true },
  { "logical", LOGICAL_KEYWORD, OP_NULL, true },
  { "logical_4", LOGICAL_S4_KEYWORD, OP_NULL, true },
  { "logical_8", LOGICAL_S8_KEYWORD, OP_NULL, true },
  { "real", REAL_KEYWORD, OP_NULL, true },
  { "real_4", REAL_S4_KEYWORD, OP_NULL, true },
  { "real_8", REAL_S8_KEYWORD, OP_NULL, true },
  { "real_16", REAL_S16_KEYWORD, OP_NULL, true },
  { "sizeof", SIZEOF, OP_NULL, true },
  { "single", SINGLE, OP_NULL, true },
  { "double", DOUBLE, OP_NULL, true },
  { "precision", PRECISION, OP_NULL, true },
  /* The following correspond to actual functions in Fortran and are case
     insensitive.  */
  { "kind", KIND, OP_NULL, false },
  { "abs", UNOP_INTRINSIC, UNOP_ABS, false },
  { "mod", BINOP_INTRINSIC, BINOP_MOD, false },
  { "floor", UNOP_OR_BINOP_INTRINSIC, FORTRAN_FLOOR, false },
  { "ceiling", UNOP_OR_BINOP_INTRINSIC, FORTRAN_CEILING, false },
  { "modulo", BINOP_INTRINSIC, BINOP_FORTRAN_MODULO, false },
  { "cmplx", UNOP_OR_BINOP_OR_TERNOP_INTRINSIC, FORTRAN_CMPLX, false },
  { "lbound", UNOP_OR_BINOP_OR_TERNOP_INTRINSIC, FORTRAN_LBOUND, false },
  { "ubound", UNOP_OR_BINOP_OR_TERNOP_INTRINSIC, FORTRAN_UBOUND, false },
  { "allocated", UNOP_INTRINSIC, UNOP_FORTRAN_ALLOCATED, false },
  { "associated", UNOP_OR_BINOP_INTRINSIC, FORTRAN_ASSOCIATED, false },
  { "rank", UNOP_INTRINSIC, UNOP_FORTRAN_RANK, false },
  { "size", UNOP_OR_BINOP_OR_TERNOP_INTRINSIC, FORTRAN_ARRAY_SIZE, false },
  { "shape", UNOP_INTRINSIC, UNOP_FORTRAN_SHAPE, false },
  { "loc", UNOP_INTRINSIC, UNOP_FORTRAN_LOC, false },
};

/* Implementation of a dynamically expandable buffer for processing input
   characters acquired through lexptr and building a value to return in
   yylval.  Ripped off from ch-exp.y */ 

static char *tempbuf;		/* Current buffer contents */
static int tempbufsize;		/* Size of allocated buffer */
static int tempbufindex;	/* Current index into buffer */

#define GROWBY_MIN_SIZE 64	/* Minimum amount to grow buffer by */

#define CHECKBUF(size) \
  do { \
    if (tempbufindex + (size) >= tempbufsize) \
      { \
	growbuf_by_size (size); \
      } \
  } while (0);


/* Grow the static temp buffer if necessary, including allocating the
   first one on demand.  */

static void
growbuf_by_size (int count)
{
  int growby;

  growby = std::max (count, GROWBY_MIN_SIZE);
  tempbufsize += growby;
  if (tempbuf == NULL)
    tempbuf = (char *) xmalloc (tempbufsize);
  else
    tempbuf = (char *) xrealloc (tempbuf, tempbufsize);
}

/* Blatantly ripped off from ch-exp.y. This routine recognizes F77 
   string-literals.
   
   Recognize a string literal.  A string literal is a nonzero sequence
   of characters enclosed in matching single quotes, except that
   a single character inside single quotes is a character literal, which
   we reject as a string literal.  To embed the terminator character inside
   a string, it is simply doubled (I.E. 'this''is''one''string') */

static int
match_string_literal (void)
{
  const char *tokptr = pstate->lexptr;

  for (tempbufindex = 0, tokptr++; *tokptr != '\0'; tokptr++)
    {
      CHECKBUF (1);
      if (*tokptr == *pstate->lexptr)
	{
	  if (*(tokptr + 1) == *pstate->lexptr)
	    tokptr++;
	  else
	    break;
	}
      tempbuf[tempbufindex++] = *tokptr;
    }
  if (*tokptr == '\0'					/* no terminator */
      || tempbufindex == 0)				/* no string */
    return 0;
  else
    {
      tempbuf[tempbufindex] = '\0';
      yylval.sval.ptr = tempbuf;
      yylval.sval.length = tempbufindex;
      pstate->lexptr = ++tokptr;
      return STRING_LITERAL;
    }
}

/* This is set if a NAME token appeared at the very end of the input
   string, with no whitespace separating the name from the EOF.  This
   is used only when parsing to do field name completion.  */
static bool saw_name_at_eof;

/* This is set if the previously-returned token was a structure
   operator '%'.  */
static bool last_was_structop;

/* Read one token, getting characters through lexptr.  */

static int
yylex (void)
{
  int c;
  int namelen;
  unsigned int token;
  const char *tokstart;
  bool saw_structop = last_was_structop;

  last_was_structop = false;

 retry:
 
  pstate->prev_lexptr = pstate->lexptr;
 
  tokstart = pstate->lexptr;

  /* First of all, let us make sure we are not dealing with the
     special tokens .true. and .false. which evaluate to 1 and 0.  */

  if (*pstate->lexptr == '.')
    {
      for (const auto &candidate : boolean_values)
	{
	  if (strncasecmp (tokstart, candidate.name,
			   strlen (candidate.name)) == 0)
	    {
	      pstate->lexptr += strlen (candidate.name);
	      yylval.lval = candidate.value;
	      return BOOLEAN_LITERAL;
	    }
	}
    }

  /* See if it is a Fortran operator.  */
  for (const auto &candidate : fortran_operators)
    if (strncasecmp (tokstart, candidate.oper,
		     strlen (candidate.oper)) == 0)
      {
	gdb_assert (!candidate.case_sensitive);
	pstate->lexptr += strlen (candidate.oper);
	yylval.opcode = candidate.opcode;
	return candidate.token;
      }

  switch (c = *tokstart)
    {
    case 0:
      if (saw_name_at_eof)
	{
	  saw_name_at_eof = false;
	  return COMPLETE;
	}
      else if (pstate->parse_completion && saw_structop)
	return COMPLETE;
      return 0;
      
    case ' ':
    case '\t':
    case '\n':
      pstate->lexptr++;
      goto retry;
      
    case '\'':
      token = match_string_literal ();
      if (token != 0)
	return (token);
      break;
      
    case '(':
      paren_depth++;
      pstate->lexptr++;
      return c;
      
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
	goto symbol;		/* Nope, must be a symbol.  */
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
	int got_dot = 0, got_e = 0, got_d = 0, toktype;
	const char *p = tokstart;
	int hex = input_radix > 10;
	
	if (c == '0' && (p[1] == 'x' || p[1] == 'X'))
	  {
	    p += 2;
	    hex = 1;
	  }
	else if (c == '0' && (p[1]=='t' || p[1]=='T'
			      || p[1]=='d' || p[1]=='D'))
	  {
	    p += 2;
	    hex = 0;
	  }
	
	for (;; ++p)
	  {
	    if (!hex && !got_e && (*p == 'e' || *p == 'E'))
	      got_dot = got_e = 1;
	    else if (!hex && !got_d && (*p == 'd' || *p == 'D'))
	      got_dot = got_d = 1;
	    else if (!hex && !got_dot && *p == '.')
	      got_dot = 1;
	    else if (((got_e && (p[-1] == 'e' || p[-1] == 'E'))
		     || (got_d && (p[-1] == 'd' || p[-1] == 'D')))
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
	toktype = parse_number (pstate, tokstart, p - tokstart,
				got_dot|got_e|got_d,
				&yylval);
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

    case '%':
      last_was_structop = true;
      /* Fall through.  */
    case '+':
    case '-':
    case '*':
    case '/':
    case '|':
    case '&':
    case '^':
    case '~':
    case '!':
    case '@':
    case '<':
    case '>':
    case '[':
    case ']':
    case '?':
    case ':':
    case '=':
    case '{':
    case '}':
    symbol:
      pstate->lexptr++;
      return c;
    }
  
  if (!(c == '_' || c == '$' || c ==':'
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
    /* We must have come across a bad character (e.g. ';').  */
    error (_("Invalid character '%c' in expression."), c);
  
  namelen = 0;
  for (c = tokstart[namelen];
       (c == '_' || c == '$' || c == ':' || (c >= '0' && c <= '9')
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')); 
       c = tokstart[++namelen]);
  
  /* The token "if" terminates the expression and is NOT 
     removed from the input stream.  */
  
  if (namelen == 2 && tokstart[0] == 'i' && tokstart[1] == 'f')
    return 0;
  
  pstate->lexptr += namelen;
  
  /* Catch specific keywords.  */

  for (const auto &keyword : f_keywords)
    if (strlen (keyword.oper) == namelen
	&& ((!keyword.case_sensitive
	     && strncasecmp (tokstart, keyword.oper, namelen) == 0)
	    || (keyword.case_sensitive
		&& strncmp (tokstart, keyword.oper, namelen) == 0)))
      {
	yylval.opcode = keyword.opcode;
	return keyword.token;
      }

  yylval.sval.ptr = tokstart;
  yylval.sval.length = namelen;
  
  if (*tokstart == '$')
    return DOLLAR_VARIABLE;

  /* Use token-type TYPENAME for symbols that happen to be defined
     currently as names of types; NAME for other symbols.
     The caller is not constrained to care about the distinction.  */
  {
    std::string tmp = copy_name (yylval.sval);
    struct block_symbol result;
    const domain_enum lookup_domains[] =
    {
      STRUCT_DOMAIN,
      VAR_DOMAIN,
      MODULE_DOMAIN
    };
    int hextype;

    for (const auto &domain : lookup_domains)
      {
	result = lookup_symbol (tmp.c_str (), pstate->expression_context_block,
				domain, NULL);
	if (result.symbol && result.symbol->aclass () == LOC_TYPEDEF)
	  {
	    yylval.tsym.type = result.symbol->type ();
	    return TYPENAME;
	  }

	if (result.symbol)
	  break;
      }

    yylval.tsym.type
      = language_lookup_primitive_type (pstate->language (),
					pstate->gdbarch (), tmp.c_str ());
    if (yylval.tsym.type != NULL)
      return TYPENAME;
    
    /* Input names that aren't symbols but ARE valid hex numbers,
       when the input radix permits them, can be names or numbers
       depending on the parse.  Note we support radixes > 16 here.  */
    if (!result.symbol
	&& ((tokstart[0] >= 'a' && tokstart[0] < 'a' + input_radix - 10)
	    || (tokstart[0] >= 'A' && tokstart[0] < 'A' + input_radix - 10)))
      {
 	YYSTYPE newlval;	/* Its value is ignored.  */
	hextype = parse_number (pstate, tokstart, namelen, 0, &newlval);
	if (hextype == INT)
	  {
	    yylval.ssym.sym = result;
	    yylval.ssym.is_a_field_of_this = false;
	    return NAME_OR_INT;
	  }
      }

    if (pstate->parse_completion && *pstate->lexptr == '\0')
      saw_name_at_eof = true;

    /* Any other kind of symbol */
    yylval.ssym.sym = result;
    yylval.ssym.is_a_field_of_this = false;
    return NAME;
  }
}

int
f_language::parser (struct parser_state *par_state) const
{
  /* Setting up the parser state.  */
  scoped_restore pstate_restore = make_scoped_restore (&pstate);
  scoped_restore restore_yydebug = make_scoped_restore (&yydebug,
							parser_debug);
  gdb_assert (par_state != NULL);
  pstate = par_state;
  last_was_structop = false;
  saw_name_at_eof = false;
  paren_depth = 0;

  struct type_stack stack;
  scoped_restore restore_type_stack = make_scoped_restore (&type_stack,
							   &stack);

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
