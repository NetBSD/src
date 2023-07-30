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
#line 44 "p-exp.y"


#include "defs.h"
#include <ctype.h>
#include "expression.h"
#include "value.h"
#include "parser-defs.h"
#include "language.h"
#include "p-lang.h"
#include "bfd.h" /* Required by objfiles.h.  */
#include "symfile.h" /* Required by objfiles.h.  */
#include "objfiles.h" /* For have_full_symbols and have_partial_symbols.  */
#include "block.h"
#include "completer.h"
#include "expop.h"

#define parse_type(ps) builtin_type (ps->gdbarch ())

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror,
   etc).  */
#define GDB_YY_REMAP_PREFIX pascal_
#include "yy-remap.h"

/* The state of the parser, used internally when we are parsing the
   expression.  */

static struct parser_state *pstate = NULL;

/* Depth of parentheses.  */
static int paren_depth;

int yyparse (void);

static int yylex (void);

static void yyerror (const char *);

static char *uptok (const char *, int);

using namespace expr;

#line 113 "p-exp.c.tmp"

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
    STRING = 260,                  /* STRING  */
    FIELDNAME = 261,               /* FIELDNAME  */
    COMPLETE = 262,                /* COMPLETE  */
    NAME = 263,                    /* NAME  */
    TYPENAME = 264,                /* TYPENAME  */
    NAME_OR_INT = 265,             /* NAME_OR_INT  */
    STRUCT = 266,                  /* STRUCT  */
    CLASS = 267,                   /* CLASS  */
    SIZEOF = 268,                  /* SIZEOF  */
    COLONCOLON = 269,              /* COLONCOLON  */
    ERROR = 270,                   /* ERROR  */
    DOLLAR_VARIABLE = 271,         /* DOLLAR_VARIABLE  */
    THIS = 272,                    /* THIS  */
    TRUEKEYWORD = 273,             /* TRUEKEYWORD  */
    FALSEKEYWORD = 274,            /* FALSEKEYWORD  */
    ABOVE_COMMA = 275,             /* ABOVE_COMMA  */
    ASSIGN = 276,                  /* ASSIGN  */
    NOT = 277,                     /* NOT  */
    OR = 278,                      /* OR  */
    XOR = 279,                     /* XOR  */
    ANDAND = 280,                  /* ANDAND  */
    NOTEQUAL = 281,                /* NOTEQUAL  */
    LEQ = 282,                     /* LEQ  */
    GEQ = 283,                     /* GEQ  */
    LSH = 284,                     /* LSH  */
    RSH = 285,                     /* RSH  */
    DIV = 286,                     /* DIV  */
    MOD = 287,                     /* MOD  */
    UNARY = 288,                   /* UNARY  */
    INCREMENT = 289,               /* INCREMENT  */
    DECREMENT = 290,               /* DECREMENT  */
    ARROW = 291,                   /* ARROW  */
    BLOCKNAME = 292                /* BLOCKNAME  */
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
#define STRING 260
#define FIELDNAME 261
#define COMPLETE 262
#define NAME 263
#define TYPENAME 264
#define NAME_OR_INT 265
#define STRUCT 266
#define CLASS 267
#define SIZEOF 268
#define COLONCOLON 269
#define ERROR 270
#define DOLLAR_VARIABLE 271
#define THIS 272
#define TRUEKEYWORD 273
#define FALSEKEYWORD 274
#define ABOVE_COMMA 275
#define ASSIGN 276
#define NOT 277
#define OR 278
#define XOR 279
#define ANDAND 280
#define NOTEQUAL 281
#define LEQ 282
#define GEQ 283
#define LSH 284
#define RSH 285
#define DIV 286
#define MOD 287
#define UNARY 288
#define INCREMENT 289
#define DECREMENT 290
#define ARROW 291
#define BLOCKNAME 292

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 91 "p-exp.y"

    LONGEST lval;
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
    struct stoken sval;
    struct ttype tsym;
    struct symtoken ssym;
    int voidval;
    const struct block *bval;
    enum exp_opcode opcode;
    struct internalvar *ivar;

    struct type **tvec;
    int *ivec;
  

#line 262 "p-exp.c.tmp"

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
  YYSYMBOL_STRING = 5,                     /* STRING  */
  YYSYMBOL_FIELDNAME = 6,                  /* FIELDNAME  */
  YYSYMBOL_COMPLETE = 7,                   /* COMPLETE  */
  YYSYMBOL_NAME = 8,                       /* NAME  */
  YYSYMBOL_TYPENAME = 9,                   /* TYPENAME  */
  YYSYMBOL_NAME_OR_INT = 10,               /* NAME_OR_INT  */
  YYSYMBOL_STRUCT = 11,                    /* STRUCT  */
  YYSYMBOL_CLASS = 12,                     /* CLASS  */
  YYSYMBOL_SIZEOF = 13,                    /* SIZEOF  */
  YYSYMBOL_COLONCOLON = 14,                /* COLONCOLON  */
  YYSYMBOL_ERROR = 15,                     /* ERROR  */
  YYSYMBOL_DOLLAR_VARIABLE = 16,           /* DOLLAR_VARIABLE  */
  YYSYMBOL_THIS = 17,                      /* THIS  */
  YYSYMBOL_TRUEKEYWORD = 18,               /* TRUEKEYWORD  */
  YYSYMBOL_FALSEKEYWORD = 19,              /* FALSEKEYWORD  */
  YYSYMBOL_20_ = 20,                       /* ','  */
  YYSYMBOL_ABOVE_COMMA = 21,               /* ABOVE_COMMA  */
  YYSYMBOL_ASSIGN = 22,                    /* ASSIGN  */
  YYSYMBOL_NOT = 23,                       /* NOT  */
  YYSYMBOL_OR = 24,                        /* OR  */
  YYSYMBOL_XOR = 25,                       /* XOR  */
  YYSYMBOL_ANDAND = 26,                    /* ANDAND  */
  YYSYMBOL_27_ = 27,                       /* '='  */
  YYSYMBOL_NOTEQUAL = 28,                  /* NOTEQUAL  */
  YYSYMBOL_29_ = 29,                       /* '<'  */
  YYSYMBOL_30_ = 30,                       /* '>'  */
  YYSYMBOL_LEQ = 31,                       /* LEQ  */
  YYSYMBOL_GEQ = 32,                       /* GEQ  */
  YYSYMBOL_LSH = 33,                       /* LSH  */
  YYSYMBOL_RSH = 34,                       /* RSH  */
  YYSYMBOL_DIV = 35,                       /* DIV  */
  YYSYMBOL_MOD = 36,                       /* MOD  */
  YYSYMBOL_37_ = 37,                       /* '@'  */
  YYSYMBOL_38_ = 38,                       /* '+'  */
  YYSYMBOL_39_ = 39,                       /* '-'  */
  YYSYMBOL_40_ = 40,                       /* '*'  */
  YYSYMBOL_41_ = 41,                       /* '/'  */
  YYSYMBOL_UNARY = 42,                     /* UNARY  */
  YYSYMBOL_INCREMENT = 43,                 /* INCREMENT  */
  YYSYMBOL_DECREMENT = 44,                 /* DECREMENT  */
  YYSYMBOL_ARROW = 45,                     /* ARROW  */
  YYSYMBOL_46_ = 46,                       /* '.'  */
  YYSYMBOL_47_ = 47,                       /* '['  */
  YYSYMBOL_48_ = 48,                       /* '('  */
  YYSYMBOL_49_ = 49,                       /* '^'  */
  YYSYMBOL_BLOCKNAME = 50,                 /* BLOCKNAME  */
  YYSYMBOL_51_ = 51,                       /* ')'  */
  YYSYMBOL_52_ = 52,                       /* ']'  */
  YYSYMBOL_YYACCEPT = 53,                  /* $accept  */
  YYSYMBOL_start = 54,                     /* start  */
  YYSYMBOL_55_1 = 55,                      /* $@1  */
  YYSYMBOL_normal_start = 56,              /* normal_start  */
  YYSYMBOL_type_exp = 57,                  /* type_exp  */
  YYSYMBOL_exp1 = 58,                      /* exp1  */
  YYSYMBOL_exp = 59,                       /* exp  */
  YYSYMBOL_field_exp = 60,                 /* field_exp  */
  YYSYMBOL_61_2 = 61,                      /* $@2  */
  YYSYMBOL_62_3 = 62,                      /* $@3  */
  YYSYMBOL_arglist = 63,                   /* arglist  */
  YYSYMBOL_64_4 = 64,                      /* $@4  */
  YYSYMBOL_block = 65,                     /* block  */
  YYSYMBOL_variable = 66,                  /* variable  */
  YYSYMBOL_qualified_name = 67,            /* qualified_name  */
  YYSYMBOL_ptype = 68,                     /* ptype  */
  YYSYMBOL_type = 69,                      /* type  */
  YYSYMBOL_typebase = 70,                  /* typebase  */
  YYSYMBOL_name = 71,                      /* name  */
  YYSYMBOL_name_not_typename = 72          /* name_not_typename  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;


/* Second part of user prologue.  */
#line 115 "p-exp.y"

/* YYSTYPE gets defined by %union */
static int parse_number (struct parser_state *,
			 const char *, int, int, YYSTYPE *);

static struct type *current_type;
static int leftdiv_is_integer;
static void push_current_type (void);
static void pop_current_type (void);
static int search_field;

#line 372 "p-exp.c.tmp"


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
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   377

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  53
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  20
/* YYNRULES -- Number of rules.  */
#define YYNRULES  77
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  126

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   292


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
      48,    51,    40,    38,    20,    39,    46,    41,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      29,    27,    30,     2,    37,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    47,     2,    52,    49,     2,     2,     2,     2,     2,
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
      15,    16,    17,    18,    19,    21,    22,    23,    24,    25,
      26,    28,    31,    32,    33,    34,    35,    36,    42,    43,
      44,    45,    50
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   196,   196,   196,   204,   205,   208,   214,   215,   220,
     226,   232,   236,   240,   244,   249,   253,   271,   287,   296,
     307,   305,   330,   327,   344,   345,   347,   351,   365,   371,
     375,   375,   396,   400,   404,   408,   412,   416,   420,   427,
     434,   441,   448,   455,   462,   466,   470,   474,   478,   485,
     492,   500,   512,   521,   524,   550,   559,   563,   585,   612,
     631,   644,   658,   672,   673,   684,   742,   753,   757,   759,
     761,   766,   776,   777,   778,   779,   782,   783
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
  "STRING", "FIELDNAME", "COMPLETE", "NAME", "TYPENAME", "NAME_OR_INT",
  "STRUCT", "CLASS", "SIZEOF", "COLONCOLON", "ERROR", "DOLLAR_VARIABLE",
  "THIS", "TRUEKEYWORD", "FALSEKEYWORD", "','", "ABOVE_COMMA", "ASSIGN",
  "NOT", "OR", "XOR", "ANDAND", "'='", "NOTEQUAL", "'<'", "'>'", "LEQ",
  "GEQ", "LSH", "RSH", "DIV", "MOD", "'@'", "'+'", "'-'", "'*'", "'/'",
  "UNARY", "INCREMENT", "DECREMENT", "ARROW", "'.'", "'['", "'('", "'^'",
  "BLOCKNAME", "')'", "']'", "$accept", "start", "$@1", "normal_start",
  "type_exp", "exp1", "exp", "field_exp", "$@2", "$@3", "arglist", "$@4",
  "block", "variable", "qualified_name", "ptype", "type", "typebase",
  "name", "name_not_typename", YY_NULLPTRPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-44)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-61)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     -44,    20,    90,   -44,   -44,   -44,   -44,   -44,   -44,   -44,
       7,     7,   -41,     7,   -44,   -44,   -44,   -44,    90,    90,
      90,   -39,   -24,    90,    10,    13,   -44,   -44,     8,   231,
       4,    21,   -44,   -44,   -44,   -14,    41,   -44,   -44,   -44,
     -44,   -44,   -44,   -44,    90,   -44,    35,   -14,    35,    35,
      90,    90,     5,   -44,    90,    90,    90,    90,    90,    90,
      90,    90,    90,    90,    90,    90,    90,    90,    90,    90,
      90,    90,   -44,   -44,   -44,   -44,   -44,   -44,   -44,    23,
       7,    90,     7,   119,   -43,   147,   175,   -44,   231,   231,
     256,   280,   303,   324,   324,    31,    31,    31,    31,    76,
      76,    76,    76,   328,   328,    35,    90,    90,    90,   -44,
      44,   203,   -44,   -44,   -44,   -44,   -44,    35,     9,   231,
      11,   -44,   -44,    90,   -44,   231
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       2,     0,     0,     1,    50,    52,    57,    76,    69,    51,
       0,     0,     0,     0,    54,    58,    48,    49,     0,     0,
       0,     0,     0,     0,     0,    77,     3,     5,     4,     7,
       0,     0,    53,    63,    67,     6,    66,    65,    72,    74,
      75,    73,    70,    71,     0,    64,    12,     0,    10,    11,
       0,     0,     0,    68,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    30,    15,    20,    22,     9,    16,    19,    17,
       0,     0,     0,     0,     0,     0,     0,    28,     8,    47,
      46,    45,    44,    38,    39,    42,    43,    40,    41,    36,
      37,    32,    33,    34,    35,    29,     0,     0,    24,    18,
      61,     0,    62,    56,    55,    13,    14,    31,     0,    25,
       0,    27,    21,     0,    23,    26
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -44,   -44,   -44,   -44,   -44,   -20,   -18,   -44,   -44,   -44,
     -44,   -44,   -44,   -44,   -44,   -44,    16,    50,    -7,   -44
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,     1,     2,    26,    27,    28,    29,    30,   107,   108,
     120,   106,    31,    32,    33,    34,    47,    36,    42,    37
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int8 yytable[] =
{
      46,    48,    49,    52,    43,    81,    45,    44,   114,    50,
      77,    78,    38,    39,    40,    38,    39,    40,    35,     8,
       3,    10,    11,    79,    51,    54,    83,   -59,    54,    54,
     109,   123,    85,    86,    81,    80,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,    41,    82,    87,    41,   -60,    24,
      84,   122,   124,   111,    65,    66,    67,    68,     0,    69,
      70,    71,    72,   110,    53,   112,     0,    73,    74,    75,
      76,    73,    74,    75,    76,     0,     0,   118,   117,     0,
     119,     0,     0,     4,     5,     6,     0,     0,     7,     8,
       9,    10,    11,    12,    13,   125,    14,    15,    16,    17,
       0,     0,     0,    18,    69,    70,    71,    72,     0,     0,
       0,     0,    73,    74,    75,    76,     0,    19,     0,    20,
       0,     0,     0,    21,    22,     0,     0,     0,    23,    24,
      25,    55,     0,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,     0,    69,    70,    71,
      72,     0,     0,     0,     0,    73,    74,    75,    76,    55,
     113,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,     0,    69,    70,    71,    72,     0,
       0,     0,     0,    73,    74,    75,    76,    55,   115,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,     0,    69,    70,    71,    72,     0,     0,     0,
       0,    73,    74,    75,    76,    55,   116,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
       0,    69,    70,    71,    72,     0,     0,     0,     0,    73,
      74,    75,    76,    55,   121,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,     0,    69,
      70,    71,    72,     0,     0,     0,     0,    73,    74,    75,
      76,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,     0,    69,    70,    71,    72,     0,     0,
       0,     0,    73,    74,    75,    76,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,     0,    69,    70,
      71,    72,     0,     0,     0,     0,    73,    74,    75,    76,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
       0,    69,    70,    71,    72,     0,     0,     0,     0,    73,
      74,    75,    76,    61,    62,    63,    64,    65,    66,    67,
      68,     0,    69,    70,    71,    72,     0,     0,    71,    72,
      73,    74,    75,    76,    73,    74,    75,    76
};

static const yytype_int8 yycheck[] =
{
      18,    19,    20,    23,    11,    48,    13,    48,    51,    48,
       6,     7,     8,     9,    10,     8,     9,    10,     2,     9,
       0,    11,    12,    30,    48,    20,    44,    14,    20,    20,
       7,    20,    50,    51,    48,    14,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    50,    14,    51,    50,    14,    49,
      44,    52,    51,    81,    33,    34,    35,    36,    -1,    38,
      39,    40,    41,    80,    24,    82,    -1,    46,    47,    48,
      49,    46,    47,    48,    49,    -1,    -1,   107,   106,    -1,
     108,    -1,    -1,     3,     4,     5,    -1,    -1,     8,     9,
      10,    11,    12,    13,    14,   123,    16,    17,    18,    19,
      -1,    -1,    -1,    23,    38,    39,    40,    41,    -1,    -1,
      -1,    -1,    46,    47,    48,    49,    -1,    37,    -1,    39,
      -1,    -1,    -1,    43,    44,    -1,    -1,    -1,    48,    49,
      50,    22,    -1,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    -1,    38,    39,    40,
      41,    -1,    -1,    -1,    -1,    46,    47,    48,    49,    22,
      51,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    -1,    38,    39,    40,    41,    -1,
      -1,    -1,    -1,    46,    47,    48,    49,    22,    51,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    -1,    38,    39,    40,    41,    -1,    -1,    -1,
      -1,    46,    47,    48,    49,    22,    51,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      -1,    38,    39,    40,    41,    -1,    -1,    -1,    -1,    46,
      47,    48,    49,    22,    51,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    -1,    38,
      39,    40,    41,    -1,    -1,    -1,    -1,    46,    47,    48,
      49,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    -1,    38,    39,    40,    41,    -1,    -1,
      -1,    -1,    46,    47,    48,    49,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    -1,    38,    39,
      40,    41,    -1,    -1,    -1,    -1,    46,    47,    48,    49,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      -1,    38,    39,    40,    41,    -1,    -1,    -1,    -1,    46,
      47,    48,    49,    29,    30,    31,    32,    33,    34,    35,
      36,    -1,    38,    39,    40,    41,    -1,    -1,    40,    41,
      46,    47,    48,    49,    46,    47,    48,    49
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    54,    55,     0,     3,     4,     5,     8,     9,    10,
      11,    12,    13,    14,    16,    17,    18,    19,    23,    37,
      39,    43,    44,    48,    49,    50,    56,    57,    58,    59,
      60,    65,    66,    67,    68,    69,    70,    72,     8,     9,
      10,    50,    71,    71,    48,    71,    59,    69,    59,    59,
      48,    48,    58,    70,    20,    22,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    38,
      39,    40,    41,    46,    47,    48,    49,     6,     7,    71,
      14,    48,    14,    59,    69,    59,    59,    51,    59,    59,
      59,    59,    59,    59,    59,    59,    59,    59,    59,    59,
      59,    59,    59,    59,    59,    59,    64,    61,    62,     7,
      71,    59,    71,    51,    51,    51,    51,    59,    58,    59,
      63,    51,    52,    20,    51,    59
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    53,    55,    54,    56,    56,    57,    58,    58,    59,
      59,    59,    59,    59,    59,    60,    59,    59,    59,    59,
      61,    59,    62,    59,    63,    63,    63,    59,    59,    59,
      64,    59,    59,    59,    59,    59,    59,    59,    59,    59,
      59,    59,    59,    59,    59,    59,    59,    59,    59,    59,
      59,    59,    59,    59,    59,    59,    59,    59,    59,    65,
      65,    66,    67,    66,    66,    66,    68,    69,    70,    70,
      70,    70,    71,    71,    71,    71,    72,    72
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     1,     1,     1,     1,     3,     2,
       2,     2,     2,     4,     4,     2,     2,     2,     3,     2,
       0,     5,     0,     5,     0,     1,     3,     4,     3,     3,
       0,     4,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     1,     1,
       1,     1,     1,     1,     1,     4,     4,     1,     1,     1,
       3,     3,     3,     1,     2,     1,     1,     1,     2,     1,
       2,     2,     1,     1,     1,     1,     1,     1
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
  case 2: /* $@1: %empty  */
#line 196 "p-exp.y"
                { current_type = NULL;
		  search_field = 0;
		  leftdiv_is_integer = 0;
		}
#line 1462 "p-exp.c.tmp"
    break;

  case 3: /* start: $@1 normal_start  */
#line 200 "p-exp.y"
                             {}
#line 1468 "p-exp.c.tmp"
    break;

  case 6: /* type_exp: type  */
#line 209 "p-exp.y"
                        {
			  pstate->push_new<type_operation> ((yyvsp[0].tval));
			  current_type = (yyvsp[0].tval); }
#line 1476 "p-exp.c.tmp"
    break;

  case 8: /* exp1: exp1 ',' exp  */
#line 216 "p-exp.y"
                        { pstate->wrap2<comma_operation> (); }
#line 1482 "p-exp.c.tmp"
    break;

  case 9: /* exp: exp '^'  */
#line 221 "p-exp.y"
                        { pstate->wrap<unop_ind_operation> ();
			  if (current_type)
			    current_type = current_type->target_type (); }
#line 1490 "p-exp.c.tmp"
    break;

  case 10: /* exp: '@' exp  */
#line 227 "p-exp.y"
                        { pstate->wrap<unop_addr_operation> ();
			  if (current_type)
			    current_type = TYPE_POINTER_TYPE (current_type); }
#line 1498 "p-exp.c.tmp"
    break;

  case 11: /* exp: '-' exp  */
#line 233 "p-exp.y"
                        { pstate->wrap<unary_neg_operation> (); }
#line 1504 "p-exp.c.tmp"
    break;

  case 12: /* exp: NOT exp  */
#line 237 "p-exp.y"
                        { pstate->wrap<unary_logical_not_operation> (); }
#line 1510 "p-exp.c.tmp"
    break;

  case 13: /* exp: INCREMENT '(' exp ')'  */
#line 241 "p-exp.y"
                        { pstate->wrap<preinc_operation> (); }
#line 1516 "p-exp.c.tmp"
    break;

  case 14: /* exp: DECREMENT '(' exp ')'  */
#line 245 "p-exp.y"
                        { pstate->wrap<predec_operation> (); }
#line 1522 "p-exp.c.tmp"
    break;

  case 15: /* field_exp: exp '.'  */
#line 250 "p-exp.y"
                        { search_field = 1; }
#line 1528 "p-exp.c.tmp"
    break;

  case 16: /* exp: field_exp FIELDNAME  */
#line 254 "p-exp.y"
                        {
			  pstate->push_new<structop_operation>
			    (pstate->pop (), copy_name ((yyvsp[0].sval)));
			  search_field = 0;
			  if (current_type)
			    {
			      while (current_type->code ()
				     == TYPE_CODE_PTR)
				current_type =
				  current_type->target_type ();
			      current_type = lookup_struct_elt_type (
				current_type, (yyvsp[0].sval).ptr, 0);
			    }
			 }
#line 1547 "p-exp.c.tmp"
    break;

  case 17: /* exp: field_exp name  */
#line 272 "p-exp.y"
                        {
			  pstate->push_new<structop_operation>
			    (pstate->pop (), copy_name ((yyvsp[0].sval)));
			  search_field = 0;
			  if (current_type)
			    {
			      while (current_type->code ()
				     == TYPE_CODE_PTR)
				current_type =
				  current_type->target_type ();
			      current_type = lookup_struct_elt_type (
				current_type, (yyvsp[0].sval).ptr, 0);
			    }
			}
#line 1566 "p-exp.c.tmp"
    break;

  case 18: /* exp: field_exp name COMPLETE  */
#line 288 "p-exp.y"
                        {
			  structop_base_operation *op
			    = new structop_ptr_operation (pstate->pop (),
							  copy_name ((yyvsp[-1].sval)));
			  pstate->mark_struct_expression (op);
			  pstate->push (operation_up (op));
			}
#line 1578 "p-exp.c.tmp"
    break;

  case 19: /* exp: field_exp COMPLETE  */
#line 297 "p-exp.y"
                        {
			  structop_base_operation *op
			    = new structop_ptr_operation (pstate->pop (), "");
			  pstate->mark_struct_expression (op);
			  pstate->push (operation_up (op));
			}
#line 1589 "p-exp.c.tmp"
    break;

  case 20: /* $@2: %empty  */
#line 307 "p-exp.y"
                        { const char *arrayname;
			  int arrayfieldindex
			    = pascal_is_string_type (current_type, NULL, NULL,
						     NULL, NULL, &arrayname);
			  if (arrayfieldindex)
			    {
			      current_type
				= (current_type
				   ->field (arrayfieldindex - 1).type ());
			      pstate->push_new<structop_operation>
				(pstate->pop (), arrayname);
			    }
			  push_current_type ();  }
#line 1607 "p-exp.c.tmp"
    break;

  case 21: /* exp: exp '[' $@2 exp1 ']'  */
#line 321 "p-exp.y"
                        { pop_current_type ();
			  pstate->wrap2<subscript_operation> ();
			  if (current_type)
			    current_type = current_type->target_type (); }
#line 1616 "p-exp.c.tmp"
    break;

  case 22: /* $@3: %empty  */
#line 330 "p-exp.y"
                        { push_current_type ();
			  pstate->start_arglist (); }
#line 1623 "p-exp.c.tmp"
    break;

  case 23: /* exp: exp '(' $@3 arglist ')'  */
#line 333 "p-exp.y"
                        {
			  std::vector<operation_up> args
			    = pstate->pop_vector (pstate->end_arglist ());
			  pstate->push_new<funcall_operation>
			    (pstate->pop (), std::move (args));
			  pop_current_type ();
			  if (current_type)
			    current_type = current_type->target_type ();
			}
#line 1637 "p-exp.c.tmp"
    break;

  case 25: /* arglist: exp  */
#line 346 "p-exp.y"
                        { pstate->arglist_len = 1; }
#line 1643 "p-exp.c.tmp"
    break;

  case 26: /* arglist: arglist ',' exp  */
#line 348 "p-exp.y"
                        { pstate->arglist_len++; }
#line 1649 "p-exp.c.tmp"
    break;

  case 27: /* exp: type '(' exp ')'  */
#line 352 "p-exp.y"
                        { if (current_type)
			    {
			      /* Allow automatic dereference of classes.  */
			      if ((current_type->code () == TYPE_CODE_PTR)
				  && (current_type->target_type ()->code () == TYPE_CODE_STRUCT)
				  && (((yyvsp[-3].tval))->code () == TYPE_CODE_STRUCT))
				pstate->wrap<unop_ind_operation> ();
			    }
			  pstate->push_new<unop_cast_operation>
			    (pstate->pop (), (yyvsp[-3].tval));
			  current_type = (yyvsp[-3].tval); }
#line 1665 "p-exp.c.tmp"
    break;

  case 28: /* exp: '(' exp1 ')'  */
#line 366 "p-exp.y"
                        { }
#line 1671 "p-exp.c.tmp"
    break;

  case 29: /* exp: exp '*' exp  */
#line 372 "p-exp.y"
                        { pstate->wrap2<mul_operation> (); }
#line 1677 "p-exp.c.tmp"
    break;

  case 30: /* $@4: %empty  */
#line 375 "p-exp.y"
                        {
			  if (current_type && is_integral_type (current_type))
			    leftdiv_is_integer = 1;
			}
#line 1686 "p-exp.c.tmp"
    break;

  case 31: /* exp: exp '/' $@4 exp  */
#line 380 "p-exp.y"
                        {
			  if (leftdiv_is_integer && current_type
			      && is_integral_type (current_type))
			    {
			      pstate->push_new<unop_cast_operation>
				(pstate->pop (),
				 parse_type (pstate)->builtin_long_double);
			      current_type
				= parse_type (pstate)->builtin_long_double;
			      leftdiv_is_integer = 0;
			    }

			  pstate->wrap2<div_operation> ();
			}
#line 1705 "p-exp.c.tmp"
    break;

  case 32: /* exp: exp DIV exp  */
#line 397 "p-exp.y"
                        { pstate->wrap2<intdiv_operation> (); }
#line 1711 "p-exp.c.tmp"
    break;

  case 33: /* exp: exp MOD exp  */
#line 401 "p-exp.y"
                        { pstate->wrap2<rem_operation> (); }
#line 1717 "p-exp.c.tmp"
    break;

  case 34: /* exp: exp '+' exp  */
#line 405 "p-exp.y"
                        { pstate->wrap2<add_operation> (); }
#line 1723 "p-exp.c.tmp"
    break;

  case 35: /* exp: exp '-' exp  */
#line 409 "p-exp.y"
                        { pstate->wrap2<sub_operation> (); }
#line 1729 "p-exp.c.tmp"
    break;

  case 36: /* exp: exp LSH exp  */
#line 413 "p-exp.y"
                        { pstate->wrap2<lsh_operation> (); }
#line 1735 "p-exp.c.tmp"
    break;

  case 37: /* exp: exp RSH exp  */
#line 417 "p-exp.y"
                        { pstate->wrap2<rsh_operation> (); }
#line 1741 "p-exp.c.tmp"
    break;

  case 38: /* exp: exp '=' exp  */
#line 421 "p-exp.y"
                        {
			  pstate->wrap2<equal_operation> ();
			  current_type = parse_type (pstate)->builtin_bool;
			}
#line 1750 "p-exp.c.tmp"
    break;

  case 39: /* exp: exp NOTEQUAL exp  */
#line 428 "p-exp.y"
                        {
			  pstate->wrap2<notequal_operation> ();
			  current_type = parse_type (pstate)->builtin_bool;
			}
#line 1759 "p-exp.c.tmp"
    break;

  case 40: /* exp: exp LEQ exp  */
#line 435 "p-exp.y"
                        {
			  pstate->wrap2<leq_operation> ();
			  current_type = parse_type (pstate)->builtin_bool;
			}
#line 1768 "p-exp.c.tmp"
    break;

  case 41: /* exp: exp GEQ exp  */
#line 442 "p-exp.y"
                        {
			  pstate->wrap2<geq_operation> ();
			  current_type = parse_type (pstate)->builtin_bool;
			}
#line 1777 "p-exp.c.tmp"
    break;

  case 42: /* exp: exp '<' exp  */
#line 449 "p-exp.y"
                        {
			  pstate->wrap2<less_operation> ();
			  current_type = parse_type (pstate)->builtin_bool;
			}
#line 1786 "p-exp.c.tmp"
    break;

  case 43: /* exp: exp '>' exp  */
#line 456 "p-exp.y"
                        {
			  pstate->wrap2<gtr_operation> ();
			  current_type = parse_type (pstate)->builtin_bool;
			}
#line 1795 "p-exp.c.tmp"
    break;

  case 44: /* exp: exp ANDAND exp  */
#line 463 "p-exp.y"
                        { pstate->wrap2<bitwise_and_operation> (); }
#line 1801 "p-exp.c.tmp"
    break;

  case 45: /* exp: exp XOR exp  */
#line 467 "p-exp.y"
                        { pstate->wrap2<bitwise_xor_operation> (); }
#line 1807 "p-exp.c.tmp"
    break;

  case 46: /* exp: exp OR exp  */
#line 471 "p-exp.y"
                        { pstate->wrap2<bitwise_ior_operation> (); }
#line 1813 "p-exp.c.tmp"
    break;

  case 47: /* exp: exp ASSIGN exp  */
#line 475 "p-exp.y"
                        { pstate->wrap2<assign_operation> (); }
#line 1819 "p-exp.c.tmp"
    break;

  case 48: /* exp: TRUEKEYWORD  */
#line 479 "p-exp.y"
                        {
			  pstate->push_new<bool_operation> ((yyvsp[0].lval));
			  current_type = parse_type (pstate)->builtin_bool;
			}
#line 1828 "p-exp.c.tmp"
    break;

  case 49: /* exp: FALSEKEYWORD  */
#line 486 "p-exp.y"
                        {
			  pstate->push_new<bool_operation> ((yyvsp[0].lval));
			  current_type = parse_type (pstate)->builtin_bool;
			}
#line 1837 "p-exp.c.tmp"
    break;

  case 50: /* exp: INT  */
#line 493 "p-exp.y"
                        {
			  pstate->push_new<long_const_operation>
			    ((yyvsp[0].typed_val_int).type, (yyvsp[0].typed_val_int).val);
			  current_type = (yyvsp[0].typed_val_int).type;
			}
#line 1847 "p-exp.c.tmp"
    break;

  case 51: /* exp: NAME_OR_INT  */
#line 501 "p-exp.y"
                        { YYSTYPE val;
			  parse_number (pstate, (yyvsp[0].ssym).stoken.ptr,
					(yyvsp[0].ssym).stoken.length, 0, &val);
			  pstate->push_new<long_const_operation>
			    (val.typed_val_int.type,
			     val.typed_val_int.val);
			  current_type = val.typed_val_int.type;
			}
#line 1860 "p-exp.c.tmp"
    break;

  case 52: /* exp: FLOAT  */
#line 513 "p-exp.y"
                        {
			  float_data data;
			  std::copy (std::begin ((yyvsp[0].typed_val_float).val), std::end ((yyvsp[0].typed_val_float).val),
				     std::begin (data));
			  pstate->push_new<float_const_operation> ((yyvsp[0].typed_val_float).type, data);
			}
#line 1871 "p-exp.c.tmp"
    break;

  case 54: /* exp: DOLLAR_VARIABLE  */
#line 525 "p-exp.y"
                        {
			  pstate->push_dollar ((yyvsp[0].sval));

			  /* $ is the normal prefix for pascal
			     hexadecimal values but this conflicts
			     with the GDB use for debugger variables
			     so in expression to enter hexadecimal
			     values we still need to use C syntax with
			     0xff */
			  std::string tmp ((yyvsp[0].sval).ptr, (yyvsp[0].sval).length);
			  /* Handle current_type.  */
			  struct internalvar *intvar
			    = lookup_only_internalvar (tmp.c_str () + 1);
			  if (intvar != nullptr)
			    {
			      scoped_value_mark mark;

			      value *val
				= value_of_internalvar (pstate->gdbarch (),
							intvar);
			      current_type = value_type (val);
			    }
 			}
#line 1899 "p-exp.c.tmp"
    break;

  case 55: /* exp: SIZEOF '(' type ')'  */
#line 551 "p-exp.y"
                        {
			  current_type = parse_type (pstate)->builtin_int;
			  (yyvsp[-1].tval) = check_typedef ((yyvsp[-1].tval));
			  pstate->push_new<long_const_operation>
			    (parse_type (pstate)->builtin_int,
			     (yyvsp[-1].tval)->length ()); }
#line 1910 "p-exp.c.tmp"
    break;

  case 56: /* exp: SIZEOF '(' exp ')'  */
#line 560 "p-exp.y"
                        { pstate->wrap<unop_sizeof_operation> ();
			  current_type = parse_type (pstate)->builtin_int; }
#line 1917 "p-exp.c.tmp"
    break;

  case 57: /* exp: STRING  */
#line 564 "p-exp.y"
                        { /* C strings are converted into array constants with
			     an explicit null byte added at the end.  Thus
			     the array upper bound is the string length.
			     There is no such thing in C as a completely empty
			     string.  */
			  const char *sp = (yyvsp[0].sval).ptr; int count = (yyvsp[0].sval).length;

			  std::vector<operation_up> args (count + 1);
			  for (int i = 0; i < count; ++i)
			    args[i] = (make_operation<long_const_operation>
				       (parse_type (pstate)->builtin_char,
					*sp++));
			  args[count] = (make_operation<long_const_operation>
					 (parse_type (pstate)->builtin_char,
					  '\0'));
			  pstate->push_new<array_operation>
			    (0, (yyvsp[0].sval).length, std::move (args));
			}
#line 1940 "p-exp.c.tmp"
    break;

  case 58: /* exp: THIS  */
#line 586 "p-exp.y"
                        {
			  struct value * this_val;
			  struct type * this_type;
			  pstate->push_new<op_this_operation> ();
			  /* We need type of this.  */
			  this_val
			    = value_of_this_silent (pstate->language ());
			  if (this_val)
			    this_type = value_type (this_val);
			  else
			    this_type = NULL;
			  if (this_type)
			    {
			      if (this_type->code () == TYPE_CODE_PTR)
				{
				  this_type = this_type->target_type ();
				  pstate->wrap<unop_ind_operation> ();
				}
			    }

			  current_type = this_type;
			}
#line 1967 "p-exp.c.tmp"
    break;

  case 59: /* block: BLOCKNAME  */
#line 613 "p-exp.y"
                        {
			  if ((yyvsp[0].ssym).sym.symbol != 0)
			      (yyval.bval) = (yyvsp[0].ssym).sym.symbol->value_block ();
			  else
			    {
			      std::string copy = copy_name ((yyvsp[0].ssym).stoken);
			      struct symtab *tem =
				  lookup_symtab (copy.c_str ());
			      if (tem)
				(yyval.bval) = (tem->compunit ()->blockvector ()
				      ->static_block ());
			      else
				error (_("No file or function \"%s\"."),
				       copy.c_str ());
			    }
			}
#line 1988 "p-exp.c.tmp"
    break;

  case 60: /* block: block COLONCOLON name  */
#line 632 "p-exp.y"
                        {
			  std::string copy = copy_name ((yyvsp[0].sval));
			  struct symbol *tem
			    = lookup_symbol (copy.c_str (), (yyvsp[-2].bval),
					     VAR_DOMAIN, NULL).symbol;

			  if (!tem || tem->aclass () != LOC_BLOCK)
			    error (_("No function \"%s\" in specified context."),
				   copy.c_str ());
			  (yyval.bval) = tem->value_block (); }
#line 2003 "p-exp.c.tmp"
    break;

  case 61: /* variable: block COLONCOLON name  */
#line 645 "p-exp.y"
                        { struct block_symbol sym;

			  std::string copy = copy_name ((yyvsp[0].sval));
			  sym = lookup_symbol (copy.c_str (), (yyvsp[-2].bval),
					       VAR_DOMAIN, NULL);
			  if (sym.symbol == 0)
			    error (_("No symbol \"%s\" in specified context."),
				   copy.c_str ());

			  pstate->push_new<var_value_operation> (sym);
			}
#line 2019 "p-exp.c.tmp"
    break;

  case 62: /* qualified_name: typebase COLONCOLON name  */
#line 659 "p-exp.y"
                        {
			  struct type *type = (yyvsp[-2].tval);

			  if (type->code () != TYPE_CODE_STRUCT
			      && type->code () != TYPE_CODE_UNION)
			    error (_("`%s' is not defined as an aggregate type."),
				   type->name ());

			  pstate->push_new<scope_operation>
			    (type, copy_name ((yyvsp[0].sval)));
			}
#line 2035 "p-exp.c.tmp"
    break;

  case 64: /* variable: COLONCOLON name  */
#line 674 "p-exp.y"
                        {
			  std::string name = copy_name ((yyvsp[0].sval));

			  struct block_symbol sym
			    = lookup_symbol (name.c_str (), nullptr,
					     VAR_DOMAIN, nullptr);
			  pstate->push_symbol (name.c_str (), sym);
			}
#line 2048 "p-exp.c.tmp"
    break;

  case 65: /* variable: name_not_typename  */
#line 685 "p-exp.y"
                        { struct block_symbol sym = (yyvsp[0].ssym).sym;

			  if (sym.symbol)
			    {
			      if (symbol_read_needs_frame (sym.symbol))
				pstate->block_tracker->update (sym);

			      pstate->push_new<var_value_operation> (sym);
			      current_type = sym.symbol->type (); }
			  else if ((yyvsp[0].ssym).is_a_field_of_this)
			    {
			      struct value * this_val;
			      struct type * this_type;
			      /* Object pascal: it hangs off of `this'.  Must
				 not inadvertently convert from a method call
				 to data ref.  */
			      pstate->block_tracker->update (sym);
			      operation_up thisop
				= make_operation<op_this_operation> ();
			      pstate->push_new<structop_operation>
				(std::move (thisop), copy_name ((yyvsp[0].ssym).stoken));
			      /* We need type of this.  */
			      this_val
				= value_of_this_silent (pstate->language ());
			      if (this_val)
				this_type = value_type (this_val);
			      else
				this_type = NULL;
			      if (this_type)
				current_type = lookup_struct_elt_type (
				  this_type,
				  copy_name ((yyvsp[0].ssym).stoken).c_str (), 0);
			      else
				current_type = NULL;
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
#line 2107 "p-exp.c.tmp"
    break;

  case 68: /* typebase: '^' typebase  */
#line 758 "p-exp.y"
                        { (yyval.tval) = lookup_pointer_type ((yyvsp[0].tval)); }
#line 2113 "p-exp.c.tmp"
    break;

  case 69: /* typebase: TYPENAME  */
#line 760 "p-exp.y"
                        { (yyval.tval) = (yyvsp[0].tsym).type; }
#line 2119 "p-exp.c.tmp"
    break;

  case 70: /* typebase: STRUCT name  */
#line 762 "p-exp.y"
                        { (yyval.tval)
			    = lookup_struct (copy_name ((yyvsp[0].sval)).c_str (),
					     pstate->expression_context_block);
			}
#line 2128 "p-exp.c.tmp"
    break;

  case 71: /* typebase: CLASS name  */
#line 767 "p-exp.y"
                        { (yyval.tval)
			    = lookup_struct (copy_name ((yyvsp[0].sval)).c_str (),
					     pstate->expression_context_block);
			}
#line 2137 "p-exp.c.tmp"
    break;

  case 72: /* name: NAME  */
#line 776 "p-exp.y"
                     { (yyval.sval) = (yyvsp[0].ssym).stoken; }
#line 2143 "p-exp.c.tmp"
    break;

  case 73: /* name: BLOCKNAME  */
#line 777 "p-exp.y"
                          { (yyval.sval) = (yyvsp[0].ssym).stoken; }
#line 2149 "p-exp.c.tmp"
    break;

  case 74: /* name: TYPENAME  */
#line 778 "p-exp.y"
                         { (yyval.sval) = (yyvsp[0].tsym).stoken; }
#line 2155 "p-exp.c.tmp"
    break;

  case 75: /* name: NAME_OR_INT  */
#line 779 "p-exp.y"
                             { (yyval.sval) = (yyvsp[0].ssym).stoken; }
#line 2161 "p-exp.c.tmp"
    break;


#line 2165 "p-exp.c.tmp"

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

#line 793 "p-exp.y"


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
      /* Handle suffixes: 'f' for float, 'l' for long double.
	 FIXME: This appears to be an extension -- do we want this?  */
      if (len >= 1 && tolower (p[len - 1]) == 'f')
	{
	  putithere->typed_val_float.type
	    = parse_type (par_state)->builtin_float;
	  len--;
	}
      else if (len >= 1 && tolower (p[len - 1]) == 'l')
	{
	  putithere->typed_val_float.type
	    = parse_type (par_state)->builtin_long_double;
	  len--;
	}
      /* Default type for floating-point literals is double.  */
      else
	{
	  putithere->typed_val_float.type
	    = parse_type (par_state)->builtin_double;
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
	  if (prevn == 0 && n == 0)
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


struct type_push
{
  struct type *stored;
  struct type_push *next;
};

static struct type_push *tp_top = NULL;

static void
push_current_type (void)
{
  struct type_push *tpnew;
  tpnew = (struct type_push *) xmalloc (sizeof (struct type_push));
  tpnew->next = tp_top;
  tpnew->stored = current_type;
  current_type = NULL;
  tp_top = tpnew;
}

static void
pop_current_type (void)
{
  struct type_push *tp = tp_top;
  if (tp)
    {
      current_type = tp->stored;
      tp_top = tp->next;
      xfree (tp);
    }
}

struct token
{
  const char *oper;
  int token;
  enum exp_opcode opcode;
};

static const struct token tokentab3[] =
  {
    {"shr", RSH, OP_NULL},
    {"shl", LSH, OP_NULL},
    {"and", ANDAND, OP_NULL},
    {"div", DIV, OP_NULL},
    {"not", NOT, OP_NULL},
    {"mod", MOD, OP_NULL},
    {"inc", INCREMENT, OP_NULL},
    {"dec", DECREMENT, OP_NULL},
    {"xor", XOR, OP_NULL}
  };

static const struct token tokentab2[] =
  {
    {"or", OR, OP_NULL},
    {"<>", NOTEQUAL, OP_NULL},
    {"<=", LEQ, OP_NULL},
    {">=", GEQ, OP_NULL},
    {":=", ASSIGN, OP_NULL},
    {"::", COLONCOLON, OP_NULL} };

/* Allocate uppercased var: */
/* make an uppercased copy of tokstart.  */
static char *
uptok (const char *tokstart, int namelen)
{
  int i;
  char *uptokstart = (char *)xmalloc(namelen+1);
  for (i = 0;i <= namelen;i++)
    {
      if ((tokstart[i]>='a' && tokstart[i]<='z'))
	uptokstart[i] = tokstart[i]-('a'-'A');
      else
	uptokstart[i] = tokstart[i];
    }
  uptokstart[namelen]='\0';
  return uptokstart;
}

/* Read one token, getting characters through lexptr.  */

static int
yylex (void)
{
  int c;
  int namelen;
  const char *tokstart;
  char *uptokstart;
  const char *tokptr;
  int explen, tempbufindex;
  static char *tempbuf;
  static int tempbufsize;

 retry:

  pstate->prev_lexptr = pstate->lexptr;

  tokstart = pstate->lexptr;
  explen = strlen (pstate->lexptr);

  /* See if it is a special token of length 3.  */
  if (explen > 2)
    for (const auto &token : tokentab3)
      if (strncasecmp (tokstart, token.oper, 3) == 0
	  && (!isalpha (token.oper[0]) || explen == 3
	      || (!isalpha (tokstart[3])
		  && !isdigit (tokstart[3]) && tokstart[3] != '_')))
	{
	  pstate->lexptr += 3;
	  yylval.opcode = token.opcode;
	  return token.token;
	}

  /* See if it is a special token of length 2.  */
  if (explen > 1)
    for (const auto &token : tokentab2)
      if (strncasecmp (tokstart, token.oper, 2) == 0
	  && (!isalpha (token.oper[0]) || explen == 2
	      || (!isalpha (tokstart[2])
		  && !isdigit (tokstart[2]) && tokstart[2] != '_')))
	{
	  pstate->lexptr += 2;
	  yylval.opcode = token.opcode;
	  return token.token;
	}

  switch (c = *tokstart)
    {
    case 0:
      if (search_field && pstate->parse_completion)
	return COMPLETE;
      else
       return 0;

    case ' ':
    case '\t':
    case '\n':
      pstate->lexptr++;
      goto retry;

    case '\'':
      /* We either have a character constant ('0' or '\177' for example)
	 or we have a quoted symbol reference ('foo(int,int)' in object pascal
	 for example).  */
      pstate->lexptr++;
      c = *pstate->lexptr++;
      if (c == '\\')
	c = parse_escape (pstate->gdbarch (), &pstate->lexptr);
      else if (c == '\'')
	error (_("Empty character constant."));

      yylval.typed_val_int.val = c;
      yylval.typed_val_int.type = parse_type (pstate)->builtin_char;

      c = *pstate->lexptr++;
      if (c != '\'')
	{
	  namelen = skip_quoted (tokstart) - tokstart;
	  if (namelen > 2)
	    {
	      pstate->lexptr = tokstart + namelen;
	      if (pstate->lexptr[-1] != '\'')
		error (_("Unmatched single quote."));
	      namelen -= 2;
	      tokstart++;
	      uptokstart = uptok(tokstart,namelen);
	      goto tryname;
	    }
	  error (_("Invalid character constant."));
	}
      return INT;

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
	{
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
	else if (c == '0' && (p[1]=='t' || p[1]=='T'
			      || p[1]=='d' || p[1]=='D'))
	  {
	    p += 2;
	    hex = 0;
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
	toktype = parse_number (pstate, tokstart,
				p - tokstart, got_dot | got_e, &yylval);
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

    case '"':

      /* Build the gdb internal form of the input string in tempbuf,
	 translating any standard C escape forms seen.  Note that the
	 buffer is null byte terminated *only* for the convenience of
	 debugging gdb itself and printing the buffer contents when
	 the buffer contains no embedded nulls.  Gdb does not depend
	 upon the buffer being null byte terminated, it uses the length
	 string instead.  This allows gdb to handle C strings (as well
	 as strings in other languages) with embedded null bytes.  */

      tokptr = ++tokstart;
      tempbufindex = 0;

      do {
	/* Grow the static temp buffer if necessary, including allocating
	   the first one on demand.  */
	if (tempbufindex + 1 >= tempbufsize)
	  {
	    tempbuf = (char *) xrealloc (tempbuf, tempbufsize += 64);
	  }

	switch (*tokptr)
	  {
	  case '\0':
	  case '"':
	    /* Do nothing, loop will terminate.  */
	    break;
	  case '\\':
	    ++tokptr;
	    c = parse_escape (pstate->gdbarch (), &tokptr);
	    if (c == -1)
	      {
		continue;
	      }
	    tempbuf[tempbufindex++] = c;
	    break;
	  default:
	    tempbuf[tempbufindex++] = *tokptr++;
	    break;
	  }
      } while ((*tokptr != '"') && (*tokptr != '\0'));
      if (*tokptr++ != '"')
	{
	  error (_("Unterminated string in expression."));
	}
      tempbuf[tempbufindex] = '\0';	/* See note above.  */
      yylval.sval.ptr = tempbuf;
      yylval.sval.length = tempbufindex;
      pstate->lexptr = tokptr;
      return (STRING);
    }

  if (!(c == '_' || c == '$'
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
    /* We must have come across a bad character (e.g. ';').  */
    error (_("Invalid character '%c' in expression."), c);

  /* It's a name.  See how long it is.  */
  namelen = 0;
  for (c = tokstart[namelen];
       (c == '_' || c == '$' || (c >= '0' && c <= '9')
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '<');)
    {
      /* Template parameter lists are part of the name.
	 FIXME: This mishandles `print $a<4&&$a>3'.  */
      if (c == '<')
	{
	  int i = namelen;
	  int nesting_level = 1;
	  while (tokstart[++i])
	    {
	      if (tokstart[i] == '<')
		nesting_level++;
	      else if (tokstart[i] == '>')
		{
		  if (--nesting_level == 0)
		    break;
		}
	    }
	  if (tokstart[i] == '>')
	    namelen = i;
	  else
	    break;
	}

      /* do NOT uppercase internals because of registers !!!  */
      c = tokstart[++namelen];
    }

  uptokstart = uptok(tokstart,namelen);

  /* The token "if" terminates the expression and is NOT
     removed from the input stream.  */
  if (namelen == 2 && uptokstart[0] == 'I' && uptokstart[1] == 'F')
    {
      xfree (uptokstart);
      return 0;
    }

  pstate->lexptr += namelen;

  tryname:

  /* Catch specific keywords.  Should be done with a data structure.  */
  switch (namelen)
    {
    case 6:
      if (strcmp (uptokstart, "OBJECT") == 0)
	{
	  xfree (uptokstart);
	  return CLASS;
	}
      if (strcmp (uptokstart, "RECORD") == 0)
	{
	  xfree (uptokstart);
	  return STRUCT;
	}
      if (strcmp (uptokstart, "SIZEOF") == 0)
	{
	  xfree (uptokstart);
	  return SIZEOF;
	}
      break;
    case 5:
      if (strcmp (uptokstart, "CLASS") == 0)
	{
	  xfree (uptokstart);
	  return CLASS;
	}
      if (strcmp (uptokstart, "FALSE") == 0)
	{
	  yylval.lval = 0;
	  xfree (uptokstart);
	  return FALSEKEYWORD;
	}
      break;
    case 4:
      if (strcmp (uptokstart, "TRUE") == 0)
	{
	  yylval.lval = 1;
	  xfree (uptokstart);
  	  return TRUEKEYWORD;
	}
      if (strcmp (uptokstart, "SELF") == 0)
	{
	  /* Here we search for 'this' like
	     inserted in FPC stabs debug info.  */
	  static const char this_name[] = "this";

	  if (lookup_symbol (this_name, pstate->expression_context_block,
			     VAR_DOMAIN, NULL).symbol)
	    {
	      xfree (uptokstart);
	      return THIS;
	    }
	}
      break;
    default:
      break;
    }

  yylval.sval.ptr = tokstart;
  yylval.sval.length = namelen;

  if (*tokstart == '$')
    {
      xfree (uptokstart);
      return DOLLAR_VARIABLE;
    }

  /* Use token-type BLOCKNAME for symbols that happen to be defined as
     functions or symtabs.  If this is not so, then ...
     Use token-type TYPENAME for symbols that happen to be defined
     currently as names of types; NAME for other symbols.
     The caller is not constrained to care about the distinction.  */
  {
    std::string tmp = copy_name (yylval.sval);
    struct symbol *sym;
    struct field_of_this_result is_a_field_of_this;
    int is_a_field = 0;
    int hextype;

    is_a_field_of_this.type = NULL;
    if (search_field && current_type)
      is_a_field = (lookup_struct_elt_type (current_type,
					    tmp.c_str (), 1) != NULL);
    if (is_a_field)
      sym = NULL;
    else
      sym = lookup_symbol (tmp.c_str (), pstate->expression_context_block,
			   VAR_DOMAIN, &is_a_field_of_this).symbol;
    /* second chance uppercased (as Free Pascal does).  */
    if (!sym && is_a_field_of_this.type == NULL && !is_a_field)
      {
       for (int i = 0; i <= namelen; i++)
	 {
	   if ((tmp[i] >= 'a' && tmp[i] <= 'z'))
	     tmp[i] -= ('a'-'A');
	 }
       if (search_field && current_type)
	 is_a_field = (lookup_struct_elt_type (current_type,
					       tmp.c_str (), 1) != NULL);
       if (is_a_field)
	 sym = NULL;
       else
	 sym = lookup_symbol (tmp.c_str (), pstate->expression_context_block,
			      VAR_DOMAIN, &is_a_field_of_this).symbol;
      }
    /* Third chance Capitalized (as GPC does).  */
    if (!sym && is_a_field_of_this.type == NULL && !is_a_field)
      {
       for (int i = 0; i <= namelen; i++)
	 {
	   if (i == 0)
	     {
	      if ((tmp[i] >= 'a' && tmp[i] <= 'z'))
		tmp[i] -= ('a'-'A');
	     }
	   else
	   if ((tmp[i] >= 'A' && tmp[i] <= 'Z'))
	     tmp[i] -= ('A'-'a');
	  }
       if (search_field && current_type)
	 is_a_field = (lookup_struct_elt_type (current_type,
					       tmp.c_str (), 1) != NULL);
       if (is_a_field)
	 sym = NULL;
       else
	 sym = lookup_symbol (tmp.c_str (), pstate->expression_context_block,
			      VAR_DOMAIN, &is_a_field_of_this).symbol;
      }

    if (is_a_field || (is_a_field_of_this.type != NULL))
      {
	tempbuf = (char *) xrealloc (tempbuf, namelen + 1);
	strncpy (tempbuf, tmp.c_str (), namelen);
	tempbuf [namelen] = 0;
	yylval.sval.ptr = tempbuf;
	yylval.sval.length = namelen;
	yylval.ssym.sym.symbol = NULL;
	yylval.ssym.sym.block = NULL;
	xfree (uptokstart);
	yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;
	if (is_a_field)
	  return FIELDNAME;
	else
	  return NAME;
      }
    /* Call lookup_symtab, not lookup_partial_symtab, in case there are
       no psymtabs (coff, xcoff, or some future change to blow away the
       psymtabs once once symbols are read).  */
    if ((sym && sym->aclass () == LOC_BLOCK)
	|| lookup_symtab (tmp.c_str ()))
      {
	yylval.ssym.sym.symbol = sym;
	yylval.ssym.sym.block = NULL;
	yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;
	xfree (uptokstart);
	return BLOCKNAME;
      }
    if (sym && sym->aclass () == LOC_TYPEDEF)
	{
#if 1
	  /* Despite the following flaw, we need to keep this code enabled.
	     Because we can get called from check_stub_method, if we don't
	     handle nested types then it screws many operations in any
	     program which uses nested types.  */
	  /* In "A::x", if x is a member function of A and there happens
	     to be a type (nested or not, since the stabs don't make that
	     distinction) named x, then this code incorrectly thinks we
	     are dealing with nested types rather than a member function.  */

	  const char *p;
	  const char *namestart;
	  struct symbol *best_sym;

	  /* Look ahead to detect nested types.  This probably should be
	     done in the grammar, but trying seemed to introduce a lot
	     of shift/reduce and reduce/reduce conflicts.  It's possible
	     that it could be done, though.  Or perhaps a non-grammar, but
	     less ad hoc, approach would work well.  */

	  /* Since we do not currently have any way of distinguishing
	     a nested type from a non-nested one (the stabs don't tell
	     us whether a type is nested), we just ignore the
	     containing type.  */

	  p = pstate->lexptr;
	  best_sym = sym;
	  while (1)
	    {
	      /* Skip whitespace.  */
	      while (*p == ' ' || *p == '\t' || *p == '\n')
		++p;
	      if (*p == ':' && p[1] == ':')
		{
		  /* Skip the `::'.  */
		  p += 2;
		  /* Skip whitespace.  */
		  while (*p == ' ' || *p == '\t' || *p == '\n')
		    ++p;
		  namestart = p;
		  while (*p == '_' || *p == '$' || (*p >= '0' && *p <= '9')
			 || (*p >= 'a' && *p <= 'z')
			 || (*p >= 'A' && *p <= 'Z'))
		    ++p;
		  if (p != namestart)
		    {
		      struct symbol *cur_sym;
		      /* As big as the whole rest of the expression, which is
			 at least big enough.  */
		      char *ncopy
			= (char *) alloca (tmp.size () + strlen (namestart)
					   + 3);
		      char *tmp1;

		      tmp1 = ncopy;
		      memcpy (tmp1, tmp.c_str (), tmp.size ());
		      tmp1 += tmp.size ();
		      memcpy (tmp1, "::", 2);
		      tmp1 += 2;
		      memcpy (tmp1, namestart, p - namestart);
		      tmp1[p - namestart] = '\0';
		      cur_sym
			= lookup_symbol (ncopy,
					 pstate->expression_context_block,
					 VAR_DOMAIN, NULL).symbol;
		      if (cur_sym)
			{
			  if (cur_sym->aclass () == LOC_TYPEDEF)
			    {
			      best_sym = cur_sym;
			      pstate->lexptr = p;
			    }
			  else
			    break;
			}
		      else
			break;
		    }
		  else
		    break;
		}
	      else
		break;
	    }

	  yylval.tsym.type = best_sym->type ();
#else /* not 0 */
	  yylval.tsym.type = sym->type ();
#endif /* not 0 */
	  xfree (uptokstart);
	  return TYPENAME;
	}
    yylval.tsym.type
      = language_lookup_primitive_type (pstate->language (),
					pstate->gdbarch (), tmp.c_str ());
    if (yylval.tsym.type != NULL)
      {
	xfree (uptokstart);
	return TYPENAME;
      }

    /* Input names that aren't symbols but ARE valid hex numbers,
       when the input radix permits them, can be names or numbers
       depending on the parse.  Note we support radixes > 16 here.  */
    if (!sym
	&& ((tokstart[0] >= 'a' && tokstart[0] < 'a' + input_radix - 10)
	    || (tokstart[0] >= 'A' && tokstart[0] < 'A' + input_radix - 10)))
      {
 	YYSTYPE newlval;	/* Its value is ignored.  */
	hextype = parse_number (pstate, tokstart, namelen, 0, &newlval);
	if (hextype == INT)
	  {
	    yylval.ssym.sym.symbol = sym;
	    yylval.ssym.sym.block = NULL;
	    yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;
	    xfree (uptokstart);
	    return NAME_OR_INT;
	  }
      }

    xfree(uptokstart);
    /* Any other kind of symbol.  */
    yylval.ssym.sym.symbol = sym;
    yylval.ssym.sym.block = NULL;
    return NAME;
  }
}

/* See language.h.  */

int
pascal_language::parser (struct parser_state *par_state) const
{
  /* Setting up the parser state.  */
  scoped_restore pstate_restore = make_scoped_restore (&pstate);
  gdb_assert (par_state != NULL);
  pstate = par_state;
  paren_depth = 0;

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
