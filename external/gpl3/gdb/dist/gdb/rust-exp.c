/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.0.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 22 "rust-exp.y" /* yacc.c:339  */


#include "defs.h"

#include "block.h"
#include "charset.h"
#include "cp-support.h"
#include "gdb_obstack.h"
#include "gdb_regex.h"
#include "rust-lang.h"
#include "parser-defs.h"
#include "selftest.h"
#include "value.h"
#include "vec.h"

#define GDB_YY_REMAP_PREFIX rust
#include "yy-remap.h"

#define RUSTSTYPE YYSTYPE

extern initialize_file_ftype _initialize_rust_exp;

struct rust_op;
typedef const struct rust_op *rust_op_ptr;
DEF_VEC_P (rust_op_ptr);

/* A typed integer constant.  */

struct typed_val_int
{
  LONGEST val;
  struct type *type;
};

/* A typed floating point constant.  */

struct typed_val_float
{
  DOUBLEST dval;
  struct type *type;
};

/* An identifier and an expression.  This is used to represent one
   element of a struct initializer.  */

struct set_field
{
  struct stoken name;
  const struct rust_op *init;
};

typedef struct set_field set_field;

DEF_VEC_O (set_field);


static int rustyylex (void);
static void rust_push_back (char c);
static const char *rust_copy_name (const char *, int);
static struct stoken rust_concat3 (const char *, const char *, const char *);
static struct stoken make_stoken (const char *);
static struct block_symbol rust_lookup_symbol (const char *name,
					       const struct block *block,
					       const domain_enum domain);
static struct type *rust_lookup_type (const char *name,
				      const struct block *block);
static struct type *rust_type (const char *name);

static const struct rust_op *crate_name (const struct rust_op *name);
static const struct rust_op *super_name (const struct rust_op *name,
					 unsigned int n_supers);

static const struct rust_op *ast_operation (enum exp_opcode opcode,
					    const struct rust_op *left,
					    const struct rust_op *right);
static const struct rust_op *ast_compound_assignment
  (enum exp_opcode opcode, const struct rust_op *left,
   const struct rust_op *rust_op);
static const struct rust_op *ast_literal (struct typed_val_int val);
static const struct rust_op *ast_dliteral (struct typed_val_float val);
static const struct rust_op *ast_structop (const struct rust_op *left,
					   const char *name,
					   int completing);
static const struct rust_op *ast_structop_anonymous
  (const struct rust_op *left, struct typed_val_int number);
static const struct rust_op *ast_unary (enum exp_opcode opcode,
					const struct rust_op *expr);
static const struct rust_op *ast_cast (const struct rust_op *expr,
				       const struct rust_op *type);
static const struct rust_op *ast_call_ish (enum exp_opcode opcode,
					   const struct rust_op *expr,
					   VEC (rust_op_ptr) **params);
static const struct rust_op *ast_path (struct stoken name,
				       VEC (rust_op_ptr) **params);
static const struct rust_op *ast_string (struct stoken str);
static const struct rust_op *ast_struct (const struct rust_op *name,
					 VEC (set_field) **fields);
static const struct rust_op *ast_range (const struct rust_op *lhs,
					const struct rust_op *rhs);
static const struct rust_op *ast_array_type (const struct rust_op *lhs,
					     struct typed_val_int val);
static const struct rust_op *ast_slice_type (const struct rust_op *type);
static const struct rust_op *ast_reference_type (const struct rust_op *type);
static const struct rust_op *ast_pointer_type (const struct rust_op *type,
					       int is_mut);
static const struct rust_op *ast_function_type (const struct rust_op *result,
						VEC (rust_op_ptr) **params);
static const struct rust_op *ast_tuple_type (VEC (rust_op_ptr) **params);

/* The state of the parser, used internally when we are parsing the
   expression.  */

static struct parser_state *pstate = NULL;

/* A regular expression for matching Rust numbers.  This is split up
   since it is very long and this gives us a way to comment the
   sections.  */

static const char *number_regex_text =
  /* subexpression 1: allows use of alternation, otherwise uninteresting */
  "^("
  /* First comes floating point.  */
  /* Recognize number after the decimal point, with optional
     exponent and optional type suffix.
     subexpression 2: allows "?", otherwise uninteresting
     subexpression 3: if present, type suffix
  */
  "[0-9][0-9_]*\\.[0-9][0-9_]*([eE][-+]?[0-9][0-9_]*)?(f32|f64)?"
#define FLOAT_TYPE1 3
  "|"
  /* Recognize exponent without decimal point, with optional type
     suffix.
     subexpression 4: if present, type suffix
  */
#define FLOAT_TYPE2 4
  "[0-9][0-9_]*[eE][-+]?[0-9][0-9_]*(f32|f64)?"
  "|"
  /* "23." is a valid floating point number, but "23.e5" and
     "23.f32" are not.  So, handle the trailing-. case
     separately.  */
  "[0-9][0-9_]*\\."
  "|"
  /* Finally come integers.
     subexpression 5: text of integer
     subexpression 6: if present, type suffix
     subexpression 7: allows use of alternation, otherwise uninteresting
  */
#define INT_TEXT 5
#define INT_TYPE 6
  "(0x[a-fA-F0-9_]+|0o[0-7_]+|0b[01_]+|[0-9][0-9_]*)"
  "([iu](size|8|16|32|64))?"
  ")";
/* The number of subexpressions to allocate space for, including the
   "0th" whole match subexpression.  */
#define NUM_SUBEXPRESSIONS 8

/* The compiled number-matching regex.  */

static regex_t number_regex;

/* True if we're running unit tests.  */

static int unit_testing;

/* Obstack for data temporarily allocated during parsing.  */

static struct obstack work_obstack;

/* Result of parsing.  Points into work_obstack.  */

static const struct rust_op *rust_ast;


#line 240 "rust-exp.c" /* yacc.c:339  */

# ifndef YY_NULLPTRPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTRPTR nullptr
#  else
#   define YY_NULLPTRPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif


/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    GDBVAR = 258,
    IDENT = 259,
    COMPLETE = 260,
    INTEGER = 261,
    DECIMAL_INTEGER = 262,
    STRING = 263,
    BYTESTRING = 264,
    FLOAT = 265,
    COMPOUND_ASSIGN = 266,
    KW_AS = 267,
    KW_IF = 268,
    KW_TRUE = 269,
    KW_FALSE = 270,
    KW_SUPER = 271,
    KW_SELF = 272,
    KW_MUT = 273,
    KW_EXTERN = 274,
    KW_CONST = 275,
    KW_FN = 276,
    KW_SIZEOF = 277,
    DOTDOT = 278,
    OROR = 279,
    ANDAND = 280,
    EQEQ = 281,
    NOTEQ = 282,
    LTEQ = 283,
    GTEQ = 284,
    LSH = 285,
    RSH = 286,
    COLONCOLON = 287,
    ARROW = 288,
    UNARY = 289
  };
#endif
/* Tokens.  */
#define GDBVAR 258
#define IDENT 259
#define COMPLETE 260
#define INTEGER 261
#define DECIMAL_INTEGER 262
#define STRING 263
#define BYTESTRING 264
#define FLOAT 265
#define COMPOUND_ASSIGN 266
#define KW_AS 267
#define KW_IF 268
#define KW_TRUE 269
#define KW_FALSE 270
#define KW_SUPER 271
#define KW_SELF 272
#define KW_MUT 273
#define KW_EXTERN 274
#define KW_CONST 275
#define KW_FN 276
#define KW_SIZEOF 277
#define DOTDOT 278
#define OROR 279
#define ANDAND 280
#define EQEQ 281
#define NOTEQ 282
#define LTEQ 283
#define GTEQ 284
#define LSH 285
#define RSH 286
#define COLONCOLON 287
#define ARROW 288
#define UNARY 289

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 197 "rust-exp.y" /* yacc.c:355  */

  /* A typed integer constant.  */
  struct typed_val_int typed_val_int;

  /* A typed floating point constant.  */
  struct typed_val_float typed_val_float;

  /* An identifier or string.  */
  struct stoken sval;

  /* A token representing an opcode, like "==".  */
  enum exp_opcode opcode;

  /* A list of expressions; for example, the arguments to a function
     call.  */
  VEC (rust_op_ptr) **params;

  /* A list of field initializers.  */
  VEC (set_field) **field_inits;

  /* A single field initializer.  */
  struct set_field one_field_init;

  /* An expression.  */
  const struct rust_op *op;

  /* A plain integer, for example used to count the number of
     "super::" prefixes on a path.  */
  unsigned int depth;

#line 376 "rust-exp.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);



/* Copy the second part of user declarations.  */
#line 228 "rust-exp.y" /* yacc.c:358  */


  /* Rust AST operations.  We build a tree of these; then lower them
     to gdb expressions when parsing has completed.  */

struct rust_op
{
  /* The opcode.  */
  enum exp_opcode opcode;
  /* If OPCODE is OP_TYPE, then this holds information about what type
     is described by this node.  */
  enum type_code typecode;
  /* Indicates whether OPCODE actually represents a compound
     assignment.  For example, if OPCODE is GTGT and this is false,
     then this rust_op represents an ordinary ">>"; but if this is
     true, then this rust_op represents ">>=".  Unused in other
     cases.  */
  unsigned int compound_assignment : 1;
  /* Only used by a field expression; if set, indicates that the field
     name occurred at the end of the expression and is eligible for
     completion.  */
  unsigned int completing : 1;
  /* Operands of expression.  Which one is used and how depends on the
     particular opcode.  */
  RUSTSTYPE left;
  RUSTSTYPE right;
};


#line 422 "rust-exp.c" /* yacc.c:358  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

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

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
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


#if ! defined yyoverflow || YYERROR_VERBOSE

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
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
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
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  60
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1031

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  58
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  35
/* YYNRULES -- Number of rules.  */
#define YYNRULES  121
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  213

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   289

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    57,     2,     2,     2,    45,    39,     2,
      49,    51,    43,    41,    50,    42,    48,    44,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    54,    56,
      35,    34,    36,     2,    40,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    47,     2,    55,    38,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    52,    37,    53,     2,     2,     2,     2,
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
      25,    26,    27,    28,    29,    30,    31,    32,    33,    46
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   353,   353,   366,   367,   368,   369,   370,   371,   372,
     373,   374,   375,   376,   377,   378,   382,   390,   407,   412,
     422,   434,   439,   449,   461,   463,   465,   467,   472,   474,
     476,   478,   483,   485,   487,   489,   518,   520,   529,   541,
     543,   548,   553,   558,   561,   564,   573,   576,   579,   581,
     586,   587,   588,   589,   593,   596,   599,   602,   605,   608,
     611,   614,   617,   620,   623,   626,   629,   632,   635,   638,
     641,   644,   647,   652,   657,   662,   668,   673,   679,   688,
     693,   698,   705,   709,   711,   715,   717,   722,   724,   726,
     731,   732,   734,   736,   738,   750,   752,   758,   760,   768,
     769,   771,   773,   775,   787,   789,   798,   799,   801,   809,
     810,   812,   814,   816,   818,   820,   822,   824,   830,   831,
     836,   845
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "GDBVAR", "IDENT", "COMPLETE", "INTEGER",
  "DECIMAL_INTEGER", "STRING", "BYTESTRING", "FLOAT", "COMPOUND_ASSIGN",
  "KW_AS", "KW_IF", "KW_TRUE", "KW_FALSE", "KW_SUPER", "KW_SELF", "KW_MUT",
  "KW_EXTERN", "KW_CONST", "KW_FN", "KW_SIZEOF", "DOTDOT", "OROR",
  "ANDAND", "EQEQ", "NOTEQ", "LTEQ", "GTEQ", "LSH", "RSH", "COLONCOLON",
  "ARROW", "'='", "'<'", "'>'", "'|'", "'^'", "'&'", "'@'", "'+'", "'-'",
  "'*'", "'/'", "'%'", "UNARY", "'['", "'.'", "'('", "','", "')'", "'{'",
  "'}'", "':'", "']'", "';'", "'!'", "$accept", "start", "expr",
  "tuple_expr", "unit_expr", "struct_expr", "struct_expr_tail",
  "struct_expr_list", "array_expr", "range_expr", "literal", "field_expr",
  "idx_expr", "unop_expr", "binop_expr", "binop_expr_expr",
  "type_cast_expr", "assignment_expr", "compound_assignment_expr",
  "paren_expr", "expr_list", "maybe_expr_list", "paren_expr_list",
  "call_expr", "maybe_self_path", "super_path", "path_expr",
  "path_for_expr", "identifier_path_for_expr", "path_for_type",
  "just_identifiers_for_type", "identifier_path_for_type", "type",
  "maybe_type_list", "type_list", YY_NULLPTRPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,    61,    60,    62,   124,    94,    38,
      64,    43,    45,    42,    47,    37,   289,    91,    46,    40,
      44,    41,   123,   125,    58,    93,    59,    33
};
# endif

#define YYPACT_NINF -151

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-151)))

#define YYTABLE_NINF -119

#define yytable_value_is_error(Yytable_value) \
  (!!((Yytable_value) == (-119)))

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     384,  -151,  -151,  -151,  -151,  -151,  -151,  -151,  -151,  -151,
     -20,    41,    -2,   119,    41,   176,   384,   384,   384,   228,
     280,   384,    56,   742,  -151,  -151,  -151,  -151,  -151,  -151,
    -151,  -151,  -151,  -151,  -151,  -151,  -151,  -151,  -151,  -151,
      42,  -151,     8,    30,    41,    30,   384,   781,    30,   384,
      68,    68,    68,    68,   384,   425,   -28,  -151,   620,    68,
    -151,   384,   209,   119,   384,   384,   384,   384,   384,   384,
     384,   384,   384,   384,   384,   384,   384,   384,   384,   384,
     384,   384,   384,   384,   384,    96,   332,  -151,    53,    10,
       6,     5,    30,   661,    68,   464,     2,   384,   384,  -151,
     332,  -151,   820,  -151,    78,   109,    71,   109,   261,    15,
     209,   396,    42,  -151,    73,  -151,  -151,   781,   854,   888,
     922,   922,   922,   922,   162,   162,   820,   922,   922,   942,
     962,   982,    47,   313,   313,   -11,   -11,   -11,   503,  -151,
    -151,  -151,   742,    74,    70,  -151,    98,    30,    77,   384,
    -151,    86,  -151,   209,  -151,   384,  -151,   542,   742,    89,
     109,  -151,   396,  -151,   209,  -151,   209,   209,    87,  -151,
      93,    95,    27,   142,   209,  -151,  -151,  -151,   384,   742,
    -151,    -8,   581,  -151,  -151,  -151,    99,   -50,  -151,  -151,
     112,  -151,   209,  -151,  -151,    -6,   702,  -151,  -151,  -151,
     114,  -151,    97,   101,  -151,  -151,  -151,     6,   209,  -151,
    -151,  -151,  -151
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
      83,    88,    95,    32,    33,    35,    36,    34,    37,    38,
      89,     0,     0,    31,     0,    83,    83,    83,    83,    83,
      83,    83,     0,     2,     5,     6,     7,     9,    11,     3,
       8,    10,    12,    13,    50,    51,    52,    53,    14,    15,
       0,     4,    87,    90,    84,    94,    83,    30,    93,    83,
      47,    43,    44,    46,    83,    77,     0,    17,     0,    45,
       1,    83,    83,    28,    83,    83,    83,    83,    83,    83,
      83,    83,    83,    83,    83,    83,    83,    83,    83,    83,
      83,    83,    83,    83,    83,     0,    79,    82,     0,     0,
      21,     0,    91,     0,    48,    77,     0,    83,    83,    25,
      79,    76,    75,   104,     0,     0,     0,     0,    83,     0,
      83,    83,     0,   109,   106,    99,    73,    29,    65,    66,
      67,    68,    69,    70,    71,    72,    74,    58,    59,    61,
      62,    60,    55,    63,    64,    54,    56,    57,     0,    39,
      40,    41,    77,    80,     0,    85,     0,    92,     0,    83,
      22,     0,    96,    83,    49,    83,    24,     0,    78,     0,
      84,   103,    83,   102,    83,   113,    83,    83,     0,   120,
       0,   119,     0,     0,    83,    42,    81,    86,    83,    19,
      18,     0,     0,    27,    16,   100,     0,     0,   114,   115,
       0,   117,    83,   101,   105,     0,    20,    98,    97,    26,
       0,   112,     0,     0,   121,   108,   107,    21,    83,   110,
     111,    23,   116
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -151,  -151,     0,  -151,  -151,  -151,  -151,   -53,  -151,  -151,
    -151,  -151,  -151,  -151,  -151,  -151,  -151,  -151,  -151,  -151,
     -15,    57,  -151,  -151,   -60,    51,  -151,  -151,    -3,  -151,
    -151,   -73,   -55,     3,  -150
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    22,   142,    24,    25,    26,   150,   151,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
     143,   144,    87,    39,    40,    89,    41,    42,    43,   113,
     114,   115,   169,   170,   171
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      23,    62,   112,   181,    56,   201,   190,   116,    45,   152,
     148,    48,    44,    47,     2,    50,    51,    52,    53,    55,
      58,    59,    98,   197,   195,   205,   146,    99,   198,   149,
     206,   103,   161,   166,   163,   167,    84,    85,    86,    96,
     153,    92,   192,   146,   192,     2,    93,    46,   112,    94,
     112,   112,    98,   165,    95,   168,    60,   156,    88,    62,
      90,   102,    91,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   145,   147,   185,    79,    80,
      81,    82,    83,   112,    84,    85,    86,   157,   158,   193,
     139,   140,   112,   141,   112,   173,   112,   112,   174,   187,
     160,   188,   189,   103,   112,    84,    85,    86,   202,   203,
     162,   176,     1,     2,    98,     3,     4,     5,     6,     7,
     177,   178,   112,     8,     9,   -83,    10,   204,    11,   180,
     184,    12,  -119,   190,   191,   192,   194,   208,   112,   179,
     200,    14,   209,   212,   211,   182,   210,   159,    15,     0,
      16,    17,    18,   172,     0,   186,    19,     0,    20,     0,
       0,     0,     0,     0,    62,     0,    21,     0,   196,     1,
       2,     0,     3,     4,     5,     6,     7,     0,     0,     0,
       8,     9,     0,    10,    49,    11,     0,     0,    12,    13,
       0,     0,    78,    79,    80,    81,    82,    83,    14,    84,
      85,    86,     0,   103,     0,    15,     0,    16,    17,    18,
       0,     0,     0,    19,     0,    20,   104,     0,   105,     0,
     106,     1,     2,    21,     3,     4,     5,     6,     7,     0,
       0,   107,     8,     9,     0,    10,    54,    11,   108,     0,
      12,    13,   109,     0,     0,     0,   110,     0,   111,     0,
      14,     0,     0,     0,     0,   103,     0,    15,     0,    16,
      17,    18,     0,     0,     0,    19,     0,    20,   104,     0,
     105,     0,   106,     1,     2,    21,     3,     4,     5,     6,
       7,     0,     0,   107,     8,     9,     0,    10,     0,    11,
     108,     0,    12,    13,   109,     0,     0,     0,   164,     0,
     111,     0,    14,     0,     0,     0,     0,     0,     0,    15,
       0,    16,    17,    18,     0,    62,     0,    19,     0,    20,
       0,    57,     0,     0,     0,     1,     2,    21,     3,     4,
       5,     6,     7,     0,     0,     0,     8,     9,   -83,    10,
       0,    11,     0,     0,    12,    13,    81,    82,    83,     0,
      84,    85,    86,     0,    14,     0,     0,     0,     0,     0,
       0,    15,     0,    16,    17,    18,     0,     0,     0,    19,
       0,    20,     0,     0,     0,     0,     0,     1,     2,    21,
       3,     4,     5,     6,     7,     0,     0,     0,     8,     9,
     103,    10,     0,    11,     0,     0,    12,    13,     0,     0,
       0,     0,     0,   104,     0,   105,    14,   106,     0,     0,
       0,     0,     0,    15,     0,    16,    17,    18,   107,     0,
       0,    19,     0,    20,     0,   108,    61,    62,     0,   109,
       0,    21,     0,   110,     0,   111,     0,  -118,    63,    64,
      65,    66,    67,    68,    69,    70,    71,     0,     0,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,     0,    84,    85,    86,    61,    62,     0,     0,     0,
       0,    97,     0,     0,     0,     0,     0,    63,    64,    65,
      66,    67,    68,    69,    70,    71,     0,     0,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
       0,    84,    85,    86,    61,    62,     0,     0,     0,     0,
     155,     0,     0,     0,     0,     0,    63,    64,    65,    66,
      67,    68,    69,    70,    71,     0,     0,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,     0,
      84,    85,    86,    61,    62,     0,     0,     0,   175,     0,
       0,     0,     0,     0,     0,    63,    64,    65,    66,    67,
      68,    69,    70,    71,     0,     0,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,     0,    84,
      85,    86,    61,    62,     0,     0,     0,   183,     0,     0,
       0,     0,     0,     0,    63,    64,    65,    66,    67,    68,
      69,    70,    71,     0,     0,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,     0,    84,    85,
      86,    61,    62,     0,     0,     0,   199,     0,     0,     0,
       0,     0,     0,    63,    64,    65,    66,    67,    68,    69,
      70,    71,     0,     0,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,     0,    84,    85,    86,
     100,   101,    61,    62,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    63,    64,    65,    66,    67,    68,
      69,    70,    71,     0,     0,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,     0,    84,    85,
      86,     0,   154,    61,    62,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    63,    64,    65,    66,    67,
      68,    69,    70,    71,     0,     0,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,     0,    84,
      85,    86,   207,    61,    62,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    63,    64,    65,    66,    67,
      68,    69,    70,    71,     0,     0,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,     0,    84,
      85,    86,    61,    62,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  -119,    64,    65,    66,    67,    68,
      69,    70,    71,     0,     0,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,     0,    84,    85,
      86,    61,    62,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    64,    65,    66,    67,    68,    69,
      70,    71,     0,     0,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    62,    84,    85,    86,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    65,
      66,    67,    68,    69,    70,    71,     0,     0,     0,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      62,    84,    85,    86,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    66,    67,    68,    69,    70,    71,
       0,     0,     0,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    62,    84,    85,    86,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  -119,  -119,
    -119,  -119,    70,    71,    62,     0,     0,  -119,  -119,    75,
      76,    77,    78,    79,    80,    81,    82,    83,     0,    84,
      85,    86,    70,    71,    62,     0,     0,     0,     0,     0,
      76,    77,    78,    79,    80,    81,    82,    83,     0,    84,
      85,    86,    70,    71,    62,     0,     0,     0,     0,     0,
       0,    77,    78,    79,    80,    81,    82,    83,     0,    84,
      85,    86,    70,    71,     0,     0,     0,     0,     0,     0,
       0,     0,    78,    79,    80,    81,    82,    83,     0,    84,
      85,    86
};

static const yytype_int16 yycheck[] =
{
       0,    12,    62,   153,    19,    55,    56,    62,    11,     4,
       4,    14,    32,    13,     4,    15,    16,    17,    18,    19,
      20,    21,    50,    31,   174,    31,    16,    55,    36,    23,
      36,     4,   105,    18,   107,    20,    47,    48,    49,    54,
      35,    44,    50,    16,    50,     4,    46,    49,   108,    49,
     110,   111,    50,   108,    54,   110,     0,    55,    16,    12,
      52,    61,    32,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    32,    89,   160,    41,    42,
      43,    44,    45,   153,    47,    48,    49,    97,    98,   172,
       4,     5,   162,     7,   164,    32,   166,   167,    35,   164,
      32,   166,   167,     4,   174,    47,    48,    49,     6,     7,
      49,    51,     3,     4,    50,     6,     7,     8,     9,    10,
      32,    54,   192,    14,    15,    16,    17,   192,    19,    53,
      51,    22,    23,    56,    51,    50,     4,    33,   208,   149,
      51,    32,    55,   208,   207,   155,    55,   100,    39,    -1,
      41,    42,    43,   112,    -1,   162,    47,    -1,    49,    -1,
      -1,    -1,    -1,    -1,    12,    -1,    57,    -1,   178,     3,
       4,    -1,     6,     7,     8,     9,    10,    -1,    -1,    -1,
      14,    15,    -1,    17,    18,    19,    -1,    -1,    22,    23,
      -1,    -1,    40,    41,    42,    43,    44,    45,    32,    47,
      48,    49,    -1,     4,    -1,    39,    -1,    41,    42,    43,
      -1,    -1,    -1,    47,    -1,    49,    17,    -1,    19,    -1,
      21,     3,     4,    57,     6,     7,     8,     9,    10,    -1,
      -1,    32,    14,    15,    -1,    17,    18,    19,    39,    -1,
      22,    23,    43,    -1,    -1,    -1,    47,    -1,    49,    -1,
      32,    -1,    -1,    -1,    -1,     4,    -1,    39,    -1,    41,
      42,    43,    -1,    -1,    -1,    47,    -1,    49,    17,    -1,
      19,    -1,    21,     3,     4,    57,     6,     7,     8,     9,
      10,    -1,    -1,    32,    14,    15,    -1,    17,    -1,    19,
      39,    -1,    22,    23,    43,    -1,    -1,    -1,    47,    -1,
      49,    -1,    32,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    41,    42,    43,    -1,    12,    -1,    47,    -1,    49,
      -1,    51,    -1,    -1,    -1,     3,     4,    57,     6,     7,
       8,     9,    10,    -1,    -1,    -1,    14,    15,    16,    17,
      -1,    19,    -1,    -1,    22,    23,    43,    44,    45,    -1,
      47,    48,    49,    -1,    32,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    41,    42,    43,    -1,    -1,    -1,    47,
      -1,    49,    -1,    -1,    -1,    -1,    -1,     3,     4,    57,
       6,     7,     8,     9,    10,    -1,    -1,    -1,    14,    15,
       4,    17,    -1,    19,    -1,    -1,    22,    23,    -1,    -1,
      -1,    -1,    -1,    17,    -1,    19,    32,    21,    -1,    -1,
      -1,    -1,    -1,    39,    -1,    41,    42,    43,    32,    -1,
      -1,    47,    -1,    49,    -1,    39,    11,    12,    -1,    43,
      -1,    57,    -1,    47,    -1,    49,    -1,    51,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    -1,    -1,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    -1,    47,    48,    49,    11,    12,    -1,    -1,    -1,
      -1,    56,    -1,    -1,    -1,    -1,    -1,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    -1,    -1,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      -1,    47,    48,    49,    11,    12,    -1,    -1,    -1,    -1,
      56,    -1,    -1,    -1,    -1,    -1,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    -1,    -1,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    -1,
      47,    48,    49,    11,    12,    -1,    -1,    -1,    55,    -1,
      -1,    -1,    -1,    -1,    -1,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    -1,    -1,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    -1,    47,
      48,    49,    11,    12,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    -1,    -1,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    -1,    47,    48,
      49,    11,    12,    -1,    -1,    -1,    55,    -1,    -1,    -1,
      -1,    -1,    -1,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    -1,    -1,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    -1,    47,    48,    49,
      50,    51,    11,    12,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    -1,    -1,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    -1,    47,    48,
      49,    -1,    51,    11,    12,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    -1,    -1,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    -1,    47,
      48,    49,    50,    11,    12,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    -1,    -1,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    -1,    47,
      48,    49,    11,    12,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    -1,    -1,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    -1,    47,    48,
      49,    11,    12,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    24,    25,    26,    27,    28,    29,
      30,    31,    -1,    -1,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    12,    47,    48,    49,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    25,
      26,    27,    28,    29,    30,    31,    -1,    -1,    -1,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      12,    47,    48,    49,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    26,    27,    28,    29,    30,    31,
      -1,    -1,    -1,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    12,    47,    48,    49,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    26,    27,
      28,    29,    30,    31,    12,    -1,    -1,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    -1,    47,
      48,    49,    30,    31,    12,    -1,    -1,    -1,    -1,    -1,
      38,    39,    40,    41,    42,    43,    44,    45,    -1,    47,
      48,    49,    30,    31,    12,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    40,    41,    42,    43,    44,    45,    -1,    47,
      48,    49,    30,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    40,    41,    42,    43,    44,    45,    -1,    47,
      48,    49
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     6,     7,     8,     9,    10,    14,    15,
      17,    19,    22,    23,    32,    39,    41,    42,    43,    47,
      49,    57,    59,    60,    61,    62,    63,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    81,
      82,    84,    85,    86,    32,    86,    49,    60,    86,    18,
      60,    60,    60,    60,    18,    60,    78,    51,    60,    60,
       0,    11,    12,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    47,    48,    49,    80,    16,    83,
      52,    32,    86,    60,    60,    60,    78,    56,    50,    55,
      50,    51,    60,     4,    17,    19,    21,    32,    39,    43,
      47,    49,    82,    87,    88,    89,    90,    60,    60,    60,
      60,    60,    60,    60,    60,    60,    60,    60,    60,    60,
      60,    60,    60,    60,    60,    60,    60,    60,    60,     4,
       5,     7,    60,    78,    79,    32,    16,    86,     4,    23,
      64,    65,     4,    35,    51,    56,    55,    60,    60,    79,
      32,    89,    49,    89,    47,    90,    18,    20,    90,    90,
      91,    92,    83,    32,    35,    55,    51,    32,    54,    60,
      53,    92,    60,    55,    51,    89,    91,    90,    90,    90,
      56,    51,    50,    89,     4,    92,    60,    31,    36,    55,
      51,    55,     6,     7,    90,    31,    36,    50,    33,    55,
      55,    65,    90
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    58,    59,    60,    60,    60,    60,    60,    60,    60,
      60,    60,    60,    60,    60,    60,    61,    62,    63,    64,
      64,    65,    65,    65,    66,    66,    66,    66,    67,    67,
      67,    67,    68,    68,    68,    68,    68,    68,    68,    69,
      69,    69,    70,    71,    71,    71,    71,    71,    71,    71,
      72,    72,    72,    72,    73,    73,    73,    73,    73,    73,
      73,    73,    73,    73,    73,    73,    73,    73,    73,    73,
      73,    73,    73,    74,    75,    76,    77,    78,    78,    79,
      79,    80,    81,    82,    82,    83,    83,    84,    84,    84,
      85,    85,    85,    85,    85,    86,    86,    86,    86,    87,
      87,    87,    87,    87,    88,    88,    89,    89,    89,    90,
      90,    90,    90,    90,    90,    90,    90,    90,    91,    91,
      92,    92
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     5,     2,     4,     2,
       3,     0,     1,     5,     4,     3,     6,     5,     2,     3,
       2,     1,     1,     1,     1,     1,     1,     1,     1,     3,
       3,     3,     4,     2,     2,     2,     2,     2,     3,     4,
       1,     1,     1,     1,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     1,     3,     0,
       1,     3,     2,     0,     2,     2,     3,     1,     1,     1,
       1,     3,     3,     2,     2,     1,     3,     5,     5,     1,
       3,     3,     2,     2,     1,     3,     1,     4,     4,     1,
       5,     5,     4,     2,     3,     3,     6,     3,     0,     1,
       1,     3
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
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

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



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
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
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
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
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
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
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


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTRPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTRPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTRPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
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
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to xreallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to xreallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yystacksize);

        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

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

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
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

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

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
| yyreduce -- Do a reduction.  |
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
        case 2:
#line 354 "rust-exp.y" /* yacc.c:1646  */
    {
		  /* If we are completing and see a valid parse,
		     rust_ast will already have been set.  */
		  if (rust_ast == NULL)
		    rust_ast = (yyvsp[0].op);
		}
#line 1826 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 16:
#line 383 "rust-exp.y" /* yacc.c:1646  */
    {
		  VEC_safe_insert (rust_op_ptr, *(yyvsp[-1].params), 0, (yyvsp[-3].op));
		  error (_("Tuple expressions not supported yet"));
		}
#line 1835 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 17:
#line 391 "rust-exp.y" /* yacc.c:1646  */
    {
		  struct typed_val_int val;

		  val.type
		    = language_lookup_primitive_type (parse_language (pstate),
						      parse_gdbarch (pstate),
						      "()");
		  val.val = 0;
		  (yyval.op) = ast_literal (val);
		}
#line 1850 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 18:
#line 408 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_struct ((yyvsp[-3].op), (yyvsp[-1].field_inits)); }
#line 1856 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 19:
#line 413 "rust-exp.y" /* yacc.c:1646  */
    {
		  struct set_field sf;

		  sf.name.ptr = NULL;
		  sf.name.length = 0;
		  sf.init = (yyvsp[0].op);

		  (yyval.one_field_init) = sf;
		}
#line 1870 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 20:
#line 423 "rust-exp.y" /* yacc.c:1646  */
    {
		  struct set_field sf;

		  sf.name = (yyvsp[-2].sval);
		  sf.init = (yyvsp[0].op);
		  (yyval.one_field_init) = sf;
		}
#line 1882 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 21:
#line 434 "rust-exp.y" /* yacc.c:1646  */
    {
		  VEC (set_field) **result
		    = OBSTACK_ZALLOC (&work_obstack, VEC (set_field) *);
		  (yyval.field_inits) = result;
		}
#line 1892 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 22:
#line 440 "rust-exp.y" /* yacc.c:1646  */
    {
		  VEC (set_field) **result
		    = OBSTACK_ZALLOC (&work_obstack, VEC (set_field) *);

		  make_cleanup (VEC_cleanup (set_field), result);
		  VEC_safe_push (set_field, *result, &(yyvsp[0].one_field_init));

		  (yyval.field_inits) = result;
		}
#line 1906 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 23:
#line 450 "rust-exp.y" /* yacc.c:1646  */
    {
		  struct set_field sf;

		  sf.name = (yyvsp[-4].sval);
		  sf.init = (yyvsp[-2].op);
		  VEC_safe_push (set_field, *(yyvsp[0].field_inits), &sf);
		  (yyval.field_inits) = (yyvsp[0].field_inits);
		}
#line 1919 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 24:
#line 462 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_call_ish (OP_ARRAY, NULL, (yyvsp[-1].params)); }
#line 1925 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 25:
#line 464 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_call_ish (OP_ARRAY, NULL, (yyvsp[-1].params)); }
#line 1931 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 26:
#line 466 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_operation (OP_RUST_ARRAY, (yyvsp[-3].op), (yyvsp[-1].op)); }
#line 1937 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 27:
#line 468 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_operation (OP_RUST_ARRAY, (yyvsp[-3].op), (yyvsp[-1].op)); }
#line 1943 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 28:
#line 473 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_range ((yyvsp[-1].op), NULL); }
#line 1949 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 29:
#line 475 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_range ((yyvsp[-2].op), (yyvsp[0].op)); }
#line 1955 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 30:
#line 477 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_range (NULL, (yyvsp[0].op)); }
#line 1961 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 31:
#line 479 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_range (NULL, NULL); }
#line 1967 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 32:
#line 484 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_literal ((yyvsp[0].typed_val_int)); }
#line 1973 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 33:
#line 486 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_literal ((yyvsp[0].typed_val_int)); }
#line 1979 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 34:
#line 488 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_dliteral ((yyvsp[0].typed_val_float)); }
#line 1985 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 35:
#line 490 "rust-exp.y" /* yacc.c:1646  */
    {
		  const struct rust_op *str = ast_string ((yyvsp[0].sval));
		  VEC (set_field) **fields;
		  struct set_field field;
		  struct typed_val_int val;
		  struct stoken token;

		  fields = OBSTACK_ZALLOC (&work_obstack, VEC (set_field) *);
		  make_cleanup (VEC_cleanup (set_field), fields);

		  /* Wrap the raw string in the &str struct.  */
		  field.name.ptr = "data_ptr";
		  field.name.length = strlen (field.name.ptr);
		  field.init = ast_unary (UNOP_ADDR, ast_string ((yyvsp[0].sval)));
		  VEC_safe_push (set_field, *fields, &field);

		  val.type = rust_type ("usize");
		  val.val = (yyvsp[0].sval).length;

		  field.name.ptr = "length";
		  field.name.length = strlen (field.name.ptr);
		  field.init = ast_literal (val);
		  VEC_safe_push (set_field, *fields, &field);

		  token.ptr = "&str";
		  token.length = strlen (token.ptr);
		  (yyval.op) = ast_struct (ast_path (token, NULL), fields);
		}
#line 2018 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 36:
#line 519 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_string ((yyvsp[0].sval)); }
#line 2024 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 37:
#line 521 "rust-exp.y" /* yacc.c:1646  */
    {
		  struct typed_val_int val;

		  val.type = language_bool_type (parse_language (pstate),
						 parse_gdbarch (pstate));
		  val.val = 1;
		  (yyval.op) = ast_literal (val);
		}
#line 2037 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 38:
#line 530 "rust-exp.y" /* yacc.c:1646  */
    {
		  struct typed_val_int val;

		  val.type = language_bool_type (parse_language (pstate),
						 parse_gdbarch (pstate));
		  val.val = 0;
		  (yyval.op) = ast_literal (val);
		}
#line 2050 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 39:
#line 542 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_structop ((yyvsp[-2].op), (yyvsp[0].sval).ptr, 0); }
#line 2056 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 40:
#line 544 "rust-exp.y" /* yacc.c:1646  */
    {
		  (yyval.op) = ast_structop ((yyvsp[-2].op), (yyvsp[0].sval).ptr, 1);
		  rust_ast = (yyval.op);
		}
#line 2065 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 41:
#line 549 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_structop_anonymous ((yyvsp[-2].op), (yyvsp[0].typed_val_int)); }
#line 2071 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 42:
#line 554 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_operation (BINOP_SUBSCRIPT, (yyvsp[-3].op), (yyvsp[-1].op)); }
#line 2077 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 43:
#line 559 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_unary (UNOP_PLUS, (yyvsp[0].op)); }
#line 2083 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 44:
#line 562 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_unary (UNOP_NEG, (yyvsp[0].op)); }
#line 2089 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 45:
#line 565 "rust-exp.y" /* yacc.c:1646  */
    {
		  /* Note that we provide a Rust-specific evaluator
		     override for UNOP_COMPLEMENT, so it can do the
		     right thing for both bool and integral
		     values.  */
		  (yyval.op) = ast_unary (UNOP_COMPLEMENT, (yyvsp[0].op));
		}
#line 2101 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 46:
#line 574 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_unary (UNOP_IND, (yyvsp[0].op)); }
#line 2107 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 47:
#line 577 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_unary (UNOP_ADDR, (yyvsp[0].op)); }
#line 2113 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 48:
#line 580 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_unary (UNOP_ADDR, (yyvsp[0].op)); }
#line 2119 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 49:
#line 582 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_unary (UNOP_SIZEOF, (yyvsp[-1].op)); }
#line 2125 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 54:
#line 594 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_operation (BINOP_MUL, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2131 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 55:
#line 597 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_operation (BINOP_REPEAT, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2137 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 56:
#line 600 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_operation (BINOP_DIV, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2143 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 57:
#line 603 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_operation (BINOP_REM, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2149 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 58:
#line 606 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_operation (BINOP_LESS, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2155 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 59:
#line 609 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_operation (BINOP_GTR, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2161 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 60:
#line 612 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_operation (BINOP_BITWISE_AND, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2167 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 61:
#line 615 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_operation (BINOP_BITWISE_IOR, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2173 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 62:
#line 618 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_operation (BINOP_BITWISE_XOR, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2179 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 63:
#line 621 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_operation (BINOP_ADD, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2185 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 64:
#line 624 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_operation (BINOP_SUB, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2191 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 65:
#line 627 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_operation (BINOP_LOGICAL_OR, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2197 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 66:
#line 630 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_operation (BINOP_LOGICAL_AND, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2203 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 67:
#line 633 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_operation (BINOP_EQUAL, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2209 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 68:
#line 636 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_operation (BINOP_NOTEQUAL, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2215 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 69:
#line 639 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_operation (BINOP_LEQ, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2221 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 70:
#line 642 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_operation (BINOP_GEQ, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2227 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 71:
#line 645 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_operation (BINOP_LSH, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2233 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 72:
#line 648 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_operation (BINOP_RSH, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2239 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 73:
#line 653 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_cast ((yyvsp[-2].op), (yyvsp[0].op)); }
#line 2245 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 74:
#line 658 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_operation (BINOP_ASSIGN, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2251 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 75:
#line 663 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_compound_assignment ((yyvsp[-1].opcode), (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2257 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 76:
#line 669 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = (yyvsp[-1].op); }
#line 2263 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 77:
#line 674 "rust-exp.y" /* yacc.c:1646  */
    {
		  (yyval.params) = OBSTACK_ZALLOC (&work_obstack, VEC (rust_op_ptr) *);
		  make_cleanup (VEC_cleanup (rust_op_ptr), (yyval.params));
		  VEC_safe_push (rust_op_ptr, *(yyval.params), (yyvsp[0].op));
		}
#line 2273 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 78:
#line 680 "rust-exp.y" /* yacc.c:1646  */
    {
		  VEC_safe_push (rust_op_ptr, *(yyvsp[-2].params), (yyvsp[0].op));
		  (yyval.params) = (yyvsp[-2].params);
		}
#line 2282 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 79:
#line 688 "rust-exp.y" /* yacc.c:1646  */
    {
		  /* The result can't be NULL.  */
		  (yyval.params) = OBSTACK_ZALLOC (&work_obstack, VEC (rust_op_ptr) *);
		  make_cleanup (VEC_cleanup (rust_op_ptr), (yyval.params));
		}
#line 2292 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 80:
#line 694 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.params) = (yyvsp[0].params); }
#line 2298 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 81:
#line 701 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.params) = (yyvsp[-1].params); }
#line 2304 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 82:
#line 706 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_call_ish (OP_FUNCALL, (yyvsp[-1].op), (yyvsp[0].params)); }
#line 2310 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 85:
#line 716 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.depth) = 1; }
#line 2316 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 86:
#line 718 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.depth) = (yyvsp[-2].depth) + 1; }
#line 2322 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 87:
#line 723 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = (yyvsp[0].op); }
#line 2328 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 88:
#line 725 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_path ((yyvsp[0].sval), NULL); }
#line 2334 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 89:
#line 727 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_path (make_stoken ("self"), NULL); }
#line 2340 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 91:
#line 733 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = super_name ((yyvsp[0].op), 0); }
#line 2346 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 92:
#line 735 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = super_name ((yyvsp[0].op), (yyvsp[-1].depth)); }
#line 2352 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 93:
#line 737 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = crate_name ((yyvsp[0].op)); }
#line 2358 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 94:
#line 739 "rust-exp.y" /* yacc.c:1646  */
    {
		  /* This is a gdb extension to make it possible to
		     refer to items in other crates.  It just bypasses
		     adding the current crate to the front of the
		     name.  */
		  (yyval.op) = ast_path (rust_concat3 ("::", (yyvsp[0].op)->left.sval.ptr, NULL),
				 (yyvsp[0].op)->right.params);
		}
#line 2371 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 95:
#line 751 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_path ((yyvsp[0].sval), NULL); }
#line 2377 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 96:
#line 753 "rust-exp.y" /* yacc.c:1646  */
    {
		  (yyval.op) = ast_path (rust_concat3 ((yyvsp[-2].op)->left.sval.ptr, "::",
					       (yyvsp[0].sval).ptr),
				 NULL);
		}
#line 2387 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 97:
#line 759 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_path ((yyvsp[-4].op)->left.sval, (yyvsp[-1].params)); }
#line 2393 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 98:
#line 761 "rust-exp.y" /* yacc.c:1646  */
    {
		  (yyval.op) = ast_path ((yyvsp[-4].op)->left.sval, (yyvsp[-1].params));
		  rust_push_back ('>');
		}
#line 2402 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 100:
#line 770 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = super_name ((yyvsp[0].op), 0); }
#line 2408 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 101:
#line 772 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = super_name ((yyvsp[0].op), (yyvsp[-1].depth)); }
#line 2414 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 102:
#line 774 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = crate_name ((yyvsp[0].op)); }
#line 2420 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 103:
#line 776 "rust-exp.y" /* yacc.c:1646  */
    {
		  /* This is a gdb extension to make it possible to
		     refer to items in other crates.  It just bypasses
		     adding the current crate to the front of the
		     name.  */
		  (yyval.op) = ast_path (rust_concat3 ("::", (yyvsp[0].op)->left.sval.ptr, NULL),
				 (yyvsp[0].op)->right.params);
		}
#line 2433 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 104:
#line 788 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_path ((yyvsp[0].sval), NULL); }
#line 2439 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 105:
#line 790 "rust-exp.y" /* yacc.c:1646  */
    {
		  (yyval.op) = ast_path (rust_concat3 ((yyvsp[-2].op)->left.sval.ptr, "::",
					       (yyvsp[0].sval).ptr),
				 NULL);
		}
#line 2449 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 107:
#line 800 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_path ((yyvsp[-3].op)->left.sval, (yyvsp[-1].params)); }
#line 2455 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 108:
#line 802 "rust-exp.y" /* yacc.c:1646  */
    {
		  (yyval.op) = ast_path ((yyvsp[-3].op)->left.sval, (yyvsp[-1].params));
		  rust_push_back ('>');
		}
#line 2464 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 110:
#line 811 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_array_type ((yyvsp[-3].op), (yyvsp[-1].typed_val_int)); }
#line 2470 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 111:
#line 813 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_array_type ((yyvsp[-3].op), (yyvsp[-1].typed_val_int)); }
#line 2476 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 112:
#line 815 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_slice_type ((yyvsp[-1].op)); }
#line 2482 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 113:
#line 817 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_reference_type ((yyvsp[0].op)); }
#line 2488 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 114:
#line 819 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_pointer_type ((yyvsp[0].op), 1); }
#line 2494 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 115:
#line 821 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_pointer_type ((yyvsp[0].op), 0); }
#line 2500 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 116:
#line 823 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_function_type ((yyvsp[0].op), (yyvsp[-3].params)); }
#line 2506 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 117:
#line 825 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = ast_tuple_type ((yyvsp[-1].params)); }
#line 2512 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 118:
#line 830 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.params) = NULL; }
#line 2518 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 119:
#line 832 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.params) = (yyvsp[0].params); }
#line 2524 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 120:
#line 837 "rust-exp.y" /* yacc.c:1646  */
    {
		  VEC (rust_op_ptr) **result
		    = OBSTACK_ZALLOC (&work_obstack, VEC (rust_op_ptr) *);

		  make_cleanup (VEC_cleanup (rust_op_ptr), result);
		  VEC_safe_push (rust_op_ptr, *result, (yyvsp[0].op));
		  (yyval.params) = result;
		}
#line 2537 "rust-exp.c" /* yacc.c:1646  */
    break;

  case 121:
#line 846 "rust-exp.y" /* yacc.c:1646  */
    {
		  VEC_safe_push (rust_op_ptr, *(yyvsp[-2].params), (yyvsp[0].op));
		  (yyval.params) = (yyvsp[-2].params);
		}
#line 2546 "rust-exp.c" /* yacc.c:1646  */
    break;


#line 2550 "rust-exp.c" /* yacc.c:1646  */
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
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
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

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

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

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
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
                  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

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

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

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
                  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 852 "rust-exp.y" /* yacc.c:1906  */


/* A struct of this type is used to describe a token.  */

struct token_info
{
  const char *name;
  int value;
  enum exp_opcode opcode;
};

/* Identifier tokens.  */

static const struct token_info identifier_tokens[] =
{
  { "as", KW_AS, OP_NULL },
  { "false", KW_FALSE, OP_NULL },
  { "if", 0, OP_NULL },
  { "mut", KW_MUT, OP_NULL },
  { "const", KW_CONST, OP_NULL },
  { "self", KW_SELF, OP_NULL },
  { "super", KW_SUPER, OP_NULL },
  { "true", KW_TRUE, OP_NULL },
  { "extern", KW_EXTERN, OP_NULL },
  { "fn", KW_FN, OP_NULL },
  { "sizeof", KW_SIZEOF, OP_NULL },
};

/* Operator tokens, sorted longest first.  */

static const struct token_info operator_tokens[] =
{
  { ">>=", COMPOUND_ASSIGN, BINOP_RSH },
  { "<<=", COMPOUND_ASSIGN, BINOP_LSH },

  { "<<", LSH, OP_NULL },
  { ">>", RSH, OP_NULL },
  { "&&", ANDAND, OP_NULL },
  { "||", OROR, OP_NULL },
  { "==", EQEQ, OP_NULL },
  { "!=", NOTEQ, OP_NULL },
  { "<=", LTEQ, OP_NULL },
  { ">=", GTEQ, OP_NULL },
  { "+=", COMPOUND_ASSIGN, BINOP_ADD },
  { "-=", COMPOUND_ASSIGN, BINOP_SUB },
  { "*=", COMPOUND_ASSIGN, BINOP_MUL },
  { "/=", COMPOUND_ASSIGN, BINOP_DIV },
  { "%=", COMPOUND_ASSIGN, BINOP_REM },
  { "&=", COMPOUND_ASSIGN, BINOP_BITWISE_AND },
  { "|=", COMPOUND_ASSIGN, BINOP_BITWISE_IOR },
  { "^=", COMPOUND_ASSIGN, BINOP_BITWISE_XOR },

  { "::", COLONCOLON, OP_NULL },
  { "..", DOTDOT, OP_NULL },
  { "->", ARROW, OP_NULL }
};

/* Helper function to copy to the name obstack.  */

static const char *
rust_copy_name (const char *name, int len)
{
  return (const char *) obstack_copy0 (&work_obstack, name, len);
}

/* Helper function to make an stoken from a C string.  */

static struct stoken
make_stoken (const char *p)
{
  struct stoken result;

  result.ptr = p;
  result.length = strlen (result.ptr);
  return result;
}

/* Helper function to concatenate three strings on the name
   obstack.  */

static struct stoken
rust_concat3 (const char *s1, const char *s2, const char *s3)
{
  return make_stoken (obconcat (&work_obstack, s1, s2, s3, (char *) NULL));
}

/* Return an AST node referring to NAME, but relative to the crate's
   name.  */

static const struct rust_op *
crate_name (const struct rust_op *name)
{
  std::string crate = rust_crate_for_block (expression_context_block);
  struct stoken result;

  gdb_assert (name->opcode == OP_VAR_VALUE);

  if (crate.empty ())
    error (_("Could not find crate for current location"));
  result = make_stoken (obconcat (&work_obstack, "::", crate.c_str (), "::",
				  name->left.sval.ptr, (char *) NULL));

  return ast_path (result, name->right.params);
}

/* Create an AST node referring to a "super::" qualified name.  IDENT
   is the base name and N_SUPERS is how many "super::"s were
   provided.  N_SUPERS can be zero.  */

static const struct rust_op *
super_name (const struct rust_op *ident, unsigned int n_supers)
{
  const char *scope = block_scope (expression_context_block);
  int offset;

  gdb_assert (ident->opcode == OP_VAR_VALUE);

  if (scope[0] == '\0')
    error (_("Couldn't find namespace scope for self::"));

  if (n_supers > 0)
    {
      int i;
      int len;
      std::vector<int> offsets;
      unsigned int current_len;

      current_len = cp_find_first_component (scope);
      while (scope[current_len] != '\0')
	{
	  offsets.push_back (current_len);
	  gdb_assert (scope[current_len] == ':');
	  /* The "::".  */
	  current_len += 2;
	  current_len += cp_find_first_component (scope
						  + current_len);
	}

      len = offsets.size ();
      if (n_supers >= len)
	error (_("Too many super:: uses from '%s'"), scope);

      offset = offsets[len - n_supers];
    }
  else
    offset = strlen (scope);

  obstack_grow (&work_obstack, "::", 2);
  obstack_grow (&work_obstack, scope, offset);
  obstack_grow (&work_obstack, "::", 2);
  obstack_grow0 (&work_obstack, ident->left.sval.ptr, ident->left.sval.length);

  return ast_path (make_stoken ((const char *) obstack_finish (&work_obstack)),
		   ident->right.params);
}

/* A helper that updates innermost_block as appropriate.  */

static void
update_innermost_block (struct block_symbol sym)
{
  if (symbol_read_needs_frame (sym.symbol)
      && (innermost_block == NULL
	  || contained_in (sym.block, innermost_block)))
    innermost_block = sym.block;
}

/* A helper to look up a Rust type, or fail.  This only works for
   types defined by rust_language_arch_info.  */

static struct type *
rust_type (const char *name)
{
  struct type *type;

  /* When unit testing, we don't bother checking the types, so avoid a
     possibly-failing lookup here.  */
  if (unit_testing)
    return NULL;

  type = language_lookup_primitive_type (parse_language (pstate),
					 parse_gdbarch (pstate),
					 name);
  if (type == NULL)
    error (_("Could not find Rust type %s"), name);
  return type;
}

/* Lex a hex number with at least MIN digits and at most MAX
   digits.  */

static uint32_t
lex_hex (int min, int max)
{
  uint32_t result = 0;
  int len = 0;
  /* We only want to stop at MAX if we're lexing a byte escape.  */
  int check_max = min == max;

  while ((check_max ? len <= max : 1)
	 && ((lexptr[0] >= 'a' && lexptr[0] <= 'f')
	     || (lexptr[0] >= 'A' && lexptr[0] <= 'F')
	     || (lexptr[0] >= '0' && lexptr[0] <= '9')))
    {
      result *= 16;
      if (lexptr[0] >= 'a' && lexptr[0] <= 'f')
	result = result + 10 + lexptr[0] - 'a';
      else if (lexptr[0] >= 'A' && lexptr[0] <= 'F')
	result = result + 10 + lexptr[0] - 'A';
      else
	result = result + lexptr[0] - '0';
      ++lexptr;
      ++len;
    }

  if (len < min)
    error (_("Not enough hex digits seen"));
  if (len > max)
    {
      gdb_assert (min != max);
      error (_("Overlong hex escape"));
    }

  return result;
}

/* Lex an escape.  IS_BYTE is true if we're lexing a byte escape;
   otherwise we're lexing a character escape.  */

static uint32_t
lex_escape (int is_byte)
{
  uint32_t result;

  gdb_assert (lexptr[0] == '\\');
  ++lexptr;
  switch (lexptr[0])
    {
    case 'x':
      ++lexptr;
      result = lex_hex (2, 2);
      break;

    case 'u':
      if (is_byte)
	error (_("Unicode escape in byte literal"));
      ++lexptr;
      if (lexptr[0] != '{')
	error (_("Missing '{' in Unicode escape"));
      ++lexptr;
      result = lex_hex (1, 6);
      /* Could do range checks here.  */
      if (lexptr[0] != '}')
	error (_("Missing '}' in Unicode escape"));
      ++lexptr;
      break;

    case 'n':
      result = '\n';
      ++lexptr;
      break;
    case 'r':
      result = '\r';
      ++lexptr;
      break;
    case 't':
      result = '\t';
      ++lexptr;
      break;
    case '\\':
      result = '\\';
      ++lexptr;
      break;
    case '0':
      result = '\0';
      ++lexptr;
      break;
    case '\'':
      result = '\'';
      ++lexptr;
      break;
    case '"':
      result = '"';
      ++lexptr;
      break;

    default:
      error (_("Invalid escape \\%c in literal"), lexptr[0]);
    }

  return result;
}

/* Lex a character constant.  */

static int
lex_character (void)
{
  int is_byte = 0;
  uint32_t value;

  if (lexptr[0] == 'b')
    {
      is_byte = 1;
      ++lexptr;
    }
  gdb_assert (lexptr[0] == '\'');
  ++lexptr;
  /* This should handle UTF-8 here.  */
  if (lexptr[0] == '\\')
    value = lex_escape (is_byte);
  else
    {
      value = lexptr[0] & 0xff;
      ++lexptr;
    }

  if (lexptr[0] != '\'')
    error (_("Unterminated character literal"));
  ++lexptr;

  rustyylval.typed_val_int.val = value;
  rustyylval.typed_val_int.type = rust_type (is_byte ? "u8" : "char");

  return INTEGER;
}

/* Return the offset of the double quote if STR looks like the start
   of a raw string, or 0 if STR does not start a raw string.  */

static int
starts_raw_string (const char *str)
{
  const char *save = str;

  if (str[0] != 'r')
    return 0;
  ++str;
  while (str[0] == '#')
    ++str;
  if (str[0] == '"')
    return str - save;
  return 0;
}

/* Return true if STR looks like the end of a raw string that had N
   hashes at the start.  */

static bool
ends_raw_string (const char *str, int n)
{
  int i;

  gdb_assert (str[0] == '"');
  for (i = 0; i < n; ++i)
    if (str[i + 1] != '#')
      return false;
  return true;
}

/* Lex a string constant.  */

static int
lex_string (void)
{
  int is_byte = lexptr[0] == 'b';
  int raw_length;
  int len_in_chars = 0;

  if (is_byte)
    ++lexptr;
  raw_length = starts_raw_string (lexptr);
  lexptr += raw_length;
  gdb_assert (lexptr[0] == '"');
  ++lexptr;

  while (1)
    {
      uint32_t value;

      if (raw_length > 0)
	{
	  if (lexptr[0] == '"' && ends_raw_string (lexptr, raw_length - 1))
	    {
	      /* Exit with lexptr pointing after the final "#".  */
	      lexptr += raw_length;
	      break;
	    }
	  else if (lexptr[0] == '\0')
	    error (_("Unexpected EOF in string"));

	  value = lexptr[0] & 0xff;
	  if (is_byte && value > 127)
	    error (_("Non-ASCII value in raw byte string"));
	  obstack_1grow (&work_obstack, value);

	  ++lexptr;
	}
      else if (lexptr[0] == '"')
	{
	  /* Make sure to skip the quote.  */
	  ++lexptr;
	  break;
	}
      else if (lexptr[0] == '\\')
	{
	  value = lex_escape (is_byte);

	  if (is_byte)
	    obstack_1grow (&work_obstack, value);
	  else
	    convert_between_encodings ("UTF-32", "UTF-8", (gdb_byte *) &value,
				       sizeof (value), sizeof (value),
				       &work_obstack, translit_none);
	}
      else if (lexptr[0] == '\0')
	error (_("Unexpected EOF in string"));
      else
	{
	  value = lexptr[0] & 0xff;
	  if (is_byte && value > 127)
	    error (_("Non-ASCII value in byte string"));
	  obstack_1grow (&work_obstack, value);
	  ++lexptr;
	}
    }

  rustyylval.sval.length = obstack_object_size (&work_obstack);
  rustyylval.sval.ptr = (const char *) obstack_finish (&work_obstack);
  return is_byte ? BYTESTRING : STRING;
}

/* Return true if STRING starts with whitespace followed by a digit.  */

static bool
space_then_number (const char *string)
{
  const char *p = string;

  while (p[0] == ' ' || p[0] == '\t')
    ++p;
  if (p == string)
    return false;

  return *p >= '0' && *p <= '9';
}

/* Return true if C can start an identifier.  */

static bool
rust_identifier_start_p (char c)
{
  return ((c >= 'a' && c <= 'z')
	  || (c >= 'A' && c <= 'Z')
	  || c == '_'
	  || c == '$');
}

/* Lex an identifier.  */

static int
lex_identifier (void)
{
  const char *start = lexptr;
  unsigned int length;
  const struct token_info *token;
  int i;
  int is_gdb_var = lexptr[0] == '$';

  gdb_assert (rust_identifier_start_p (lexptr[0]));

  ++lexptr;

  /* For the time being this doesn't handle Unicode rules.  Non-ASCII
     identifiers are gated anyway.  */
  while ((lexptr[0] >= 'a' && lexptr[0] <= 'z')
	 || (lexptr[0] >= 'A' && lexptr[0] <= 'Z')
	 || lexptr[0] == '_'
	 || (is_gdb_var && lexptr[0] == '$')
	 || (lexptr[0] >= '0' && lexptr[0] <= '9'))
    ++lexptr;


  length = lexptr - start;
  token = NULL;
  for (i = 0; i < ARRAY_SIZE (identifier_tokens); ++i)
    {
      if (length == strlen (identifier_tokens[i].name)
	  && strncmp (identifier_tokens[i].name, start, length) == 0)
	{
	  token = &identifier_tokens[i];
	  break;
	}
    }

  if (token != NULL)
    {
      if (token->value == 0)
	{
	  /* Leave the terminating token alone.  */
	  lexptr = start;
	  return 0;
	}
    }
  else if (token == NULL
	   && (strncmp (start, "thread", length) == 0
	       || strncmp (start, "task", length) == 0)
	   && space_then_number (lexptr))
    {
      /* "task" or "thread" followed by a number terminates the
	 parse, per gdb rules.  */
      lexptr = start;
      return 0;
    }

  if (token == NULL || (parse_completion && lexptr[0] == '\0'))
    rustyylval.sval = make_stoken (rust_copy_name (start, length));

  if (parse_completion && lexptr[0] == '\0')
    {
      /* Prevent rustyylex from returning two COMPLETE tokens.  */
      prev_lexptr = lexptr;
      return COMPLETE;
    }

  if (token != NULL)
    return token->value;
  if (is_gdb_var)
    return GDBVAR;
  return IDENT;
}

/* Lex an operator.  */

static int
lex_operator (void)
{
  const struct token_info *token = NULL;
  int i;

  for (i = 0; i < ARRAY_SIZE (operator_tokens); ++i)
    {
      if (strncmp (operator_tokens[i].name, lexptr,
		   strlen (operator_tokens[i].name)) == 0)
	{
	  lexptr += strlen (operator_tokens[i].name);
	  token = &operator_tokens[i];
	  break;
	}
    }

  if (token != NULL)
    {
      rustyylval.opcode = token->opcode;
      return token->value;
    }

  return *lexptr++;
}

/* Lex a number.  */

static int
lex_number (void)
{
  regmatch_t subexps[NUM_SUBEXPRESSIONS];
  int match;
  int is_integer = 0;
  int could_be_decimal = 1;
  int implicit_i32 = 0;
  const char *type_name = NULL;
  struct type *type;
  int end_index;
  int type_index = -1;
  int i;

  match = regexec (&number_regex, lexptr, ARRAY_SIZE (subexps), subexps, 0);
  /* Failure means the regexp is broken.  */
  gdb_assert (match == 0);

  if (subexps[INT_TEXT].rm_so != -1)
    {
      /* Integer part matched.  */
      is_integer = 1;
      end_index = subexps[INT_TEXT].rm_eo;
      if (subexps[INT_TYPE].rm_so == -1)
	{
	  type_name = "i32";
	  implicit_i32 = 1;
	}
      else
	{
	  type_index = INT_TYPE;
	  could_be_decimal = 0;
	}
    }
  else if (subexps[FLOAT_TYPE1].rm_so != -1)
    {
      /* Found floating point type suffix.  */
      end_index = subexps[FLOAT_TYPE1].rm_so;
      type_index = FLOAT_TYPE1;
    }
  else if (subexps[FLOAT_TYPE2].rm_so != -1)
    {
      /* Found floating point type suffix.  */
      end_index = subexps[FLOAT_TYPE2].rm_so;
      type_index = FLOAT_TYPE2;
    }
  else
    {
      /* Any other floating point match.  */
      end_index = subexps[0].rm_eo;
      type_name = "f64";
    }

  /* We need a special case if the final character is ".".  In this
     case we might need to parse an integer.  For example, "23.f()" is
     a request for a trait method call, not a syntax error involving
     the floating point number "23.".  */
  gdb_assert (subexps[0].rm_eo > 0);
  if (lexptr[subexps[0].rm_eo - 1] == '.')
    {
      const char *next = skip_spaces_const (&lexptr[subexps[0].rm_eo]);

      if (rust_identifier_start_p (*next) || *next == '.')
	{
	  --subexps[0].rm_eo;
	  is_integer = 1;
	  end_index = subexps[0].rm_eo;
	  type_name = "i32";
	  could_be_decimal = 1;
	  implicit_i32 = 1;
	}
    }

  /* Compute the type name if we haven't already.  */
  std::string type_name_holder;
  if (type_name == NULL)
    {
      gdb_assert (type_index != -1);
      type_name_holder = std::string (lexptr + subexps[type_index].rm_so,
				      (subexps[type_index].rm_eo
				       - subexps[type_index].rm_so));
      type_name = type_name_holder.c_str ();
    }

  /* Look up the type.  */
  type = rust_type (type_name);

  /* Copy the text of the number and remove the "_"s.  */
  std::string number;
  for (i = 0; i < end_index && lexptr[i]; ++i)
    {
      if (lexptr[i] == '_')
	could_be_decimal = 0;
      else
	number.push_back (lexptr[i]);
    }

  /* Advance past the match.  */
  lexptr += subexps[0].rm_eo;

  /* Parse the number.  */
  if (is_integer)
    {
      uint64_t value;
      int radix = 10;
      int offset = 0;

      if (number[0] == '0')
	{
	  if (number[1] == 'x')
	    radix = 16;
	  else if (number[1] == 'o')
	    radix = 8;
	  else if (number[1] == 'b')
	    radix = 2;
	  if (radix != 10)
	    {
	      offset = 2;
	      could_be_decimal = 0;
	    }
	}

      value = strtoul (number.c_str () + offset, NULL, radix);
      if (implicit_i32 && value >= ((uint64_t) 1) << 31)
	type = rust_type ("i64");

      rustyylval.typed_val_int.val = value;
      rustyylval.typed_val_int.type = type;
    }
  else
    {
      rustyylval.typed_val_float.dval = strtod (number.c_str (), NULL);
      rustyylval.typed_val_float.type = type;
    }

  return is_integer ? (could_be_decimal ? DECIMAL_INTEGER : INTEGER) : FLOAT;
}

/* The lexer.  */

static int
rustyylex (void)
{
  /* Skip all leading whitespace.  */
  while (lexptr[0] == ' ' || lexptr[0] == '\t' || lexptr[0] == '\r'
	 || lexptr[0] == '\n')
    ++lexptr;

  /* If we hit EOF and we're completing, then return COMPLETE -- maybe
     we're completing an empty string at the end of a field_expr.
     But, we don't want to return two COMPLETE tokens in a row.  */
  if (lexptr[0] == '\0' && lexptr == prev_lexptr)
    return 0;
  prev_lexptr = lexptr;
  if (lexptr[0] == '\0')
    {
      if (parse_completion)
	{
	  rustyylval.sval = make_stoken ("");
	  return COMPLETE;
	}
      return 0;
    }

  if (lexptr[0] >= '0' && lexptr[0] <= '9')
    return lex_number ();
  else if (lexptr[0] == 'b' && lexptr[1] == '\'')
    return lex_character ();
  else if (lexptr[0] == 'b' && lexptr[1] == '"')
    return lex_string ();
  else if (lexptr[0] == 'b' && starts_raw_string (lexptr + 1))
    return lex_string ();
  else if (starts_raw_string (lexptr))
    return lex_string ();
  else if (rust_identifier_start_p (lexptr[0]))
    return lex_identifier ();
  else if (lexptr[0] == '"')
    return lex_string ();
  else if (lexptr[0] == '\'')
    return lex_character ();
  else if (lexptr[0] == '}' || lexptr[0] == ']')
    {
      /* Falls through to lex_operator.  */
      --paren_depth;
    }
  else if (lexptr[0] == '(' || lexptr[0] == '{')
    {
      /* Falls through to lex_operator.  */
      ++paren_depth;
    }
  else if (lexptr[0] == ',' && comma_terminates && paren_depth == 0)
    return 0;

  return lex_operator ();
}

/* Push back a single character to be re-lexed.  */

static void
rust_push_back (char c)
{
  /* Can't be called before any lexing.  */
  gdb_assert (prev_lexptr != NULL);

  --lexptr;
  gdb_assert (*lexptr == c);
}



/* Make an arbitrary operation and fill in the fields.  */

static const struct rust_op *
ast_operation (enum exp_opcode opcode, const struct rust_op *left,
		const struct rust_op *right)
{
  struct rust_op *result = OBSTACK_ZALLOC (&work_obstack, struct rust_op);

  result->opcode = opcode;
  result->left.op = left;
  result->right.op = right;

  return result;
}

/* Make a compound assignment operation.  */

static const struct rust_op *
ast_compound_assignment (enum exp_opcode opcode, const struct rust_op *left,
			  const struct rust_op *right)
{
  struct rust_op *result = OBSTACK_ZALLOC (&work_obstack, struct rust_op);

  result->opcode = opcode;
  result->compound_assignment = 1;
  result->left.op = left;
  result->right.op = right;

  return result;
}

/* Make a typed integer literal operation.  */

static const struct rust_op *
ast_literal (struct typed_val_int val)
{
  struct rust_op *result = OBSTACK_ZALLOC (&work_obstack, struct rust_op);

  result->opcode = OP_LONG;
  result->left.typed_val_int = val;

  return result;
}

/* Make a typed floating point literal operation.  */

static const struct rust_op *
ast_dliteral (struct typed_val_float val)
{
  struct rust_op *result = OBSTACK_ZALLOC (&work_obstack, struct rust_op);

  result->opcode = OP_DOUBLE;
  result->left.typed_val_float = val;

  return result;
}

/* Make a unary operation.  */

static const struct rust_op *
ast_unary (enum exp_opcode opcode, const struct rust_op *expr)
{
  return ast_operation (opcode, expr, NULL);
}

/* Make a cast operation.  */

static const struct rust_op *
ast_cast (const struct rust_op *expr, const struct rust_op *type)
{
  struct rust_op *result = OBSTACK_ZALLOC (&work_obstack, struct rust_op);

  result->opcode = UNOP_CAST;
  result->left.op = expr;
  result->right.op = type;

  return result;
}

/* Make a call-like operation.  This is nominally a function call, but
   when lowering we may discover that it actually represents the
   creation of a tuple struct.  */

static const struct rust_op *
ast_call_ish (enum exp_opcode opcode, const struct rust_op *expr,
	       VEC (rust_op_ptr) **params)
{
  struct rust_op *result = OBSTACK_ZALLOC (&work_obstack, struct rust_op);

  result->opcode = opcode;
  result->left.op = expr;
  result->right.params = params;

  return result;
}

/* Make a structure creation operation.  */

static const struct rust_op *
ast_struct (const struct rust_op *name, VEC (set_field) **fields)
{
  struct rust_op *result = OBSTACK_ZALLOC (&work_obstack, struct rust_op);

  result->opcode = OP_AGGREGATE;
  result->left.op = name;
  result->right.field_inits = fields;

  return result;
}

/* Make an identifier path.  */

static const struct rust_op *
ast_path (struct stoken path, VEC (rust_op_ptr) **params)
{
  struct rust_op *result = OBSTACK_ZALLOC (&work_obstack, struct rust_op);

  result->opcode = OP_VAR_VALUE;
  result->left.sval = path;
  result->right.params = params;

  return result;
}

/* Make a string constant operation.  */

static const struct rust_op *
ast_string (struct stoken str)
{
  struct rust_op *result = OBSTACK_ZALLOC (&work_obstack, struct rust_op);

  result->opcode = OP_STRING;
  result->left.sval = str;

  return result;
}

/* Make a field expression.  */

static const struct rust_op *
ast_structop (const struct rust_op *left, const char *name, int completing)
{
  struct rust_op *result = OBSTACK_ZALLOC (&work_obstack, struct rust_op);

  result->opcode = STRUCTOP_STRUCT;
  result->completing = completing;
  result->left.op = left;
  result->right.sval = make_stoken (name);

  return result;
}

/* Make an anonymous struct operation, like 'x.0'.  */

static const struct rust_op *
ast_structop_anonymous (const struct rust_op *left,
			 struct typed_val_int number)
{
  struct rust_op *result = OBSTACK_ZALLOC (&work_obstack, struct rust_op);

  result->opcode = STRUCTOP_ANONYMOUS;
  result->left.op = left;
  result->right.typed_val_int = number;

  return result;
}

/* Make a range operation.  */

static const struct rust_op *
ast_range (const struct rust_op *lhs, const struct rust_op *rhs)
{
  struct rust_op *result = OBSTACK_ZALLOC (&work_obstack, struct rust_op);

  result->opcode = OP_RANGE;
  result->left.op = lhs;
  result->right.op = rhs;

  return result;
}

/* A helper function to make a type-related AST node.  */

static struct rust_op *
ast_basic_type (enum type_code typecode)
{
  struct rust_op *result = OBSTACK_ZALLOC (&work_obstack, struct rust_op);

  result->opcode = OP_TYPE;
  result->typecode = typecode;
  return result;
}

/* Create an AST node describing an array type.  */

static const struct rust_op *
ast_array_type (const struct rust_op *lhs, struct typed_val_int val)
{
  struct rust_op *result = ast_basic_type (TYPE_CODE_ARRAY);

  result->left.op = lhs;
  result->right.typed_val_int = val;
  return result;
}

/* Create an AST node describing a reference type.  */

static const struct rust_op *
ast_slice_type (const struct rust_op *type)
{
  /* Use TYPE_CODE_COMPLEX just because it is handy.  */
  struct rust_op *result = ast_basic_type (TYPE_CODE_COMPLEX);

  result->left.op = type;
  return result;
}

/* Create an AST node describing a reference type.  */

static const struct rust_op *
ast_reference_type (const struct rust_op *type)
{
  struct rust_op *result = ast_basic_type (TYPE_CODE_REF);

  result->left.op = type;
  return result;
}

/* Create an AST node describing a pointer type.  */

static const struct rust_op *
ast_pointer_type (const struct rust_op *type, int is_mut)
{
  struct rust_op *result = ast_basic_type (TYPE_CODE_PTR);

  result->left.op = type;
  /* For the time being we ignore is_mut.  */
  return result;
}

/* Create an AST node describing a function type.  */

static const struct rust_op *
ast_function_type (const struct rust_op *rtype, VEC (rust_op_ptr) **params)
{
  struct rust_op *result = ast_basic_type (TYPE_CODE_FUNC);

  result->left.op = rtype;
  result->right.params = params;
  return result;
}

/* Create an AST node describing a tuple type.  */

static const struct rust_op *
ast_tuple_type (VEC (rust_op_ptr) **params)
{
  struct rust_op *result = ast_basic_type (TYPE_CODE_STRUCT);

  result->left.params = params;
  return result;
}

/* A helper to appropriately munge NAME and BLOCK depending on the
   presence of a leading "::".  */

static void
munge_name_and_block (const char **name, const struct block **block)
{
  /* If it is a global reference, skip the current block in favor of
     the static block.  */
  if (strncmp (*name, "::", 2) == 0)
    {
      *name += 2;
      *block = block_static_block (*block);
    }
}

/* Like lookup_symbol, but handles Rust namespace conventions, and
   doesn't require field_of_this_result.  */

static struct block_symbol
rust_lookup_symbol (const char *name, const struct block *block,
		    const domain_enum domain)
{
  struct block_symbol result;

  munge_name_and_block (&name, &block);

  result = lookup_symbol (name, block, domain, NULL);
  if (result.symbol != NULL)
    update_innermost_block (result);
  return result;
}

/* Look up a type, following Rust namespace conventions.  */

static struct type *
rust_lookup_type (const char *name, const struct block *block)
{
  struct block_symbol result;
  struct type *type;

  munge_name_and_block (&name, &block);

  result = lookup_symbol (name, block, STRUCT_DOMAIN, NULL);
  if (result.symbol != NULL)
    {
      update_innermost_block (result);
      return SYMBOL_TYPE (result.symbol);
    }

  type = lookup_typename (parse_language (pstate), parse_gdbarch (pstate),
			  name, NULL, 1);
  if (type != NULL)
    return type;

  /* Last chance, try a built-in type.  */
  return language_lookup_primitive_type (parse_language (pstate),
					 parse_gdbarch (pstate),
					 name);
}

static struct type *convert_ast_to_type (struct parser_state *state,
					 const struct rust_op *operation);
static const char *convert_name (struct parser_state *state,
				 const struct rust_op *operation);

/* Convert a vector of rust_ops representing types to a vector of
   types.  */

static std::vector<struct type *>
convert_params_to_types (struct parser_state *state, VEC (rust_op_ptr) *params)
{
  int i;
  const struct rust_op *op;
  std::vector<struct type *> result;

  for (i = 0; VEC_iterate (rust_op_ptr, params, i, op); ++i)
    result.push_back (convert_ast_to_type (state, op));

  return result;
}

/* Convert a rust_op representing a type to a struct type *.  */

static struct type *
convert_ast_to_type (struct parser_state *state,
		     const struct rust_op *operation)
{
  struct type *type, *result = NULL;

  if (operation->opcode == OP_VAR_VALUE)
    {
      const char *varname = convert_name (state, operation);

      result = rust_lookup_type (varname, expression_context_block);
      if (result == NULL)
	error (_("No typed name '%s' in current context"), varname);
      return result;
    }

  gdb_assert (operation->opcode == OP_TYPE);

  switch (operation->typecode)
    {
    case TYPE_CODE_ARRAY:
      type = convert_ast_to_type (state, operation->left.op);
      if (operation->right.typed_val_int.val < 0)
	error (_("Negative array length"));
      result = lookup_array_range_type (type, 0,
					operation->right.typed_val_int.val - 1);
      break;

    case TYPE_CODE_COMPLEX:
      {
	struct type *usize = rust_type ("usize");

	type = convert_ast_to_type (state, operation->left.op);
	result = rust_slice_type ("&[*gdb*]", type, usize);
      }
      break;

    case TYPE_CODE_REF:
    case TYPE_CODE_PTR:
      /* For now we treat &x and *x identically.  */
      type = convert_ast_to_type (state, operation->left.op);
      result = lookup_pointer_type (type);
      break;

    case TYPE_CODE_FUNC:
      {
	std::vector<struct type *> args
	  (convert_params_to_types (state, *operation->right.params));
	struct type **argtypes = NULL;

	type = convert_ast_to_type (state, operation->left.op);
	if (!args.empty ())
	  argtypes = args.data ();

	result
	  = lookup_function_type_with_arguments (type, args.size (),
						 argtypes);
	result = lookup_pointer_type (result);
      }
      break;

    case TYPE_CODE_STRUCT:
      {
	std::vector<struct type *> args
	  (convert_params_to_types (state, *operation->left.params));
	int i;
	struct type *type;
	const char *name;

	obstack_1grow (&work_obstack, '(');
	for (i = 0; i < args.size (); ++i)
	  {
	    std::string type_name = type_to_string (args[i]);

	    if (i > 0)
	      obstack_1grow (&work_obstack, ',');
	    obstack_grow_str (&work_obstack, type_name.c_str ());
	  }

	obstack_grow_str0 (&work_obstack, ")");
	name = (const char *) obstack_finish (&work_obstack);

	/* We don't allow creating new tuple types (yet), but we do
	   allow looking up existing tuple types.  */
	result = rust_lookup_type (name, expression_context_block);
	if (result == NULL)
	  error (_("could not find tuple type '%s'"), name);
      }
      break;

    default:
      gdb_assert_not_reached ("unhandled opcode in convert_ast_to_type");
    }

  gdb_assert (result != NULL);
  return result;
}

/* A helper function to turn a rust_op representing a name into a full
   name.  This applies generic arguments as needed.  The returned name
   is allocated on the work obstack.  */

static const char *
convert_name (struct parser_state *state, const struct rust_op *operation)
{
  int i;

  gdb_assert (operation->opcode == OP_VAR_VALUE);

  if (operation->right.params == NULL)
    return operation->left.sval.ptr;

  std::vector<struct type *> types
    (convert_params_to_types (state, *operation->right.params));

  obstack_grow_str (&work_obstack, operation->left.sval.ptr);
  obstack_1grow (&work_obstack, '<');
  for (i = 0; i < types.size (); ++i)
    {
      std::string type_name = type_to_string (types[i]);

      if (i > 0)
	obstack_1grow (&work_obstack, ',');

      obstack_grow_str (&work_obstack, type_name.c_str ());
    }
  obstack_grow_str0 (&work_obstack, ">");

  return (const char *) obstack_finish (&work_obstack);
}

static void convert_ast_to_expression (struct parser_state *state,
				       const struct rust_op *operation,
				       const struct rust_op *top);

/* A helper function that converts a vec of rust_ops to a gdb
   expression.  */

static void
convert_params_to_expression (struct parser_state *state,
			      VEC (rust_op_ptr) *params,
			      const struct rust_op *top)
{
  int i;
  rust_op_ptr elem;

  for (i = 0; VEC_iterate (rust_op_ptr, params, i, elem); ++i)
    convert_ast_to_expression (state, elem, top);
}

/* Lower a rust_op to a gdb expression.  STATE is the parser state.
   OPERATION is the operation to lower.  TOP is a pointer to the
   top-most operation; it is used to handle the special case where the
   top-most expression is an identifier and can be optionally lowered
   to OP_TYPE.  */

static void
convert_ast_to_expression (struct parser_state *state,
			   const struct rust_op *operation,
			   const struct rust_op *top)
{
  switch (operation->opcode)
    {
    case OP_LONG:
      write_exp_elt_opcode (state, OP_LONG);
      write_exp_elt_type (state, operation->left.typed_val_int.type);
      write_exp_elt_longcst (state, operation->left.typed_val_int.val);
      write_exp_elt_opcode (state, OP_LONG);
      break;

    case OP_DOUBLE:
      write_exp_elt_opcode (state, OP_DOUBLE);
      write_exp_elt_type (state, operation->left.typed_val_float.type);
      write_exp_elt_dblcst (state, operation->left.typed_val_float.dval);
      write_exp_elt_opcode (state, OP_DOUBLE);
      break;

    case STRUCTOP_STRUCT:
      {
	convert_ast_to_expression (state, operation->left.op, top);

	if (operation->completing)
	  mark_struct_expression (state);
	write_exp_elt_opcode (state, STRUCTOP_STRUCT);
	write_exp_string (state, operation->right.sval);
	write_exp_elt_opcode (state, STRUCTOP_STRUCT);
      }
      break;

    case STRUCTOP_ANONYMOUS:
      {
	convert_ast_to_expression (state, operation->left.op, top);

	write_exp_elt_opcode (state, STRUCTOP_ANONYMOUS);
	write_exp_elt_longcst (state, operation->right.typed_val_int.val);
	write_exp_elt_opcode (state, STRUCTOP_ANONYMOUS);
      }
      break;

    case UNOP_PLUS:
    case UNOP_NEG:
    case UNOP_COMPLEMENT:
    case UNOP_IND:
    case UNOP_ADDR:
    case UNOP_SIZEOF:
      convert_ast_to_expression (state, operation->left.op, top);
      write_exp_elt_opcode (state, operation->opcode);
      break;

    case BINOP_SUBSCRIPT:
    case BINOP_MUL:
    case BINOP_REPEAT:
    case BINOP_DIV:
    case BINOP_REM:
    case BINOP_LESS:
    case BINOP_GTR:
    case BINOP_BITWISE_AND:
    case BINOP_BITWISE_IOR:
    case BINOP_BITWISE_XOR:
    case BINOP_ADD:
    case BINOP_SUB:
    case BINOP_LOGICAL_OR:
    case BINOP_LOGICAL_AND:
    case BINOP_EQUAL:
    case BINOP_NOTEQUAL:
    case BINOP_LEQ:
    case BINOP_GEQ:
    case BINOP_LSH:
    case BINOP_RSH:
    case BINOP_ASSIGN:
    case OP_RUST_ARRAY:
      convert_ast_to_expression (state, operation->left.op, top);
      convert_ast_to_expression (state, operation->right.op, top);
      if (operation->compound_assignment)
	{
	  write_exp_elt_opcode (state, BINOP_ASSIGN_MODIFY);
	  write_exp_elt_opcode (state, operation->opcode);
	  write_exp_elt_opcode (state, BINOP_ASSIGN_MODIFY);
	}
      else
	write_exp_elt_opcode (state, operation->opcode);

      if (operation->compound_assignment
	  || operation->opcode == BINOP_ASSIGN)
	{
	  struct type *type;

	  type = language_lookup_primitive_type (parse_language (state),
						 parse_gdbarch (state),
						 "()");

	  write_exp_elt_opcode (state, OP_LONG);
	  write_exp_elt_type (state, type);
	  write_exp_elt_longcst (state, 0);
	  write_exp_elt_opcode (state, OP_LONG);

	  write_exp_elt_opcode (state, BINOP_COMMA);
	}
      break;

    case UNOP_CAST:
      {
	struct type *type = convert_ast_to_type (state, operation->right.op);

	convert_ast_to_expression (state, operation->left.op, top);
	write_exp_elt_opcode (state, UNOP_CAST);
	write_exp_elt_type (state, type);
	write_exp_elt_opcode (state, UNOP_CAST);
      }
      break;

    case OP_FUNCALL:
      {
	if (operation->left.op->opcode == OP_VAR_VALUE)
	  {
	    struct type *type;
	    const char *varname = convert_name (state, operation->left.op);

	    type = rust_lookup_type (varname, expression_context_block);
	    if (type != NULL)
	      {
		/* This is actually a tuple struct expression, not a
		   call expression.  */
		rust_op_ptr elem;
		int i;
		VEC (rust_op_ptr) *params = *operation->right.params;

		if (TYPE_CODE (type) != TYPE_CODE_NAMESPACE)
		  {
		    if (!rust_tuple_struct_type_p (type))
		      error (_("Type %s is not a tuple struct"), varname);

		    for (i = 0;
			 VEC_iterate (rust_op_ptr, params, i, elem);
			 ++i)
		      {
			char *cell = get_print_cell ();

			xsnprintf (cell, PRINT_CELL_SIZE, "__%d", i);
			write_exp_elt_opcode (state, OP_NAME);
			write_exp_string (state, make_stoken (cell));
			write_exp_elt_opcode (state, OP_NAME);

			convert_ast_to_expression (state, elem, top);
		      }

		    write_exp_elt_opcode (state, OP_AGGREGATE);
		    write_exp_elt_type (state, type);
		    write_exp_elt_longcst (state,
					   2 * VEC_length (rust_op_ptr,
							   params));
		    write_exp_elt_opcode (state, OP_AGGREGATE);
		    break;
		  }
	      }
	  }
	convert_ast_to_expression (state, operation->left.op, top);
	convert_params_to_expression (state, *operation->right.params, top);
	write_exp_elt_opcode (state, OP_FUNCALL);
	write_exp_elt_longcst (state, VEC_length (rust_op_ptr,
						  *operation->right.params));
	write_exp_elt_longcst (state, OP_FUNCALL);
      }
      break;

    case OP_ARRAY:
      gdb_assert (operation->left.op == NULL);
      convert_params_to_expression (state, *operation->right.params, top);
      write_exp_elt_opcode (state, OP_ARRAY);
      write_exp_elt_longcst (state, 0);
      write_exp_elt_longcst (state, VEC_length (rust_op_ptr,
						*operation->right.params) - 1);
      write_exp_elt_longcst (state, OP_ARRAY);
      break;

    case OP_VAR_VALUE:
      {
	struct block_symbol sym;
	const char *varname;

	if (operation->left.sval.ptr[0] == '$')
	  {
	    write_dollar_variable (state, operation->left.sval);
	    break;
	  }

	varname = convert_name (state, operation);
	sym = rust_lookup_symbol (varname, expression_context_block,
				  VAR_DOMAIN);
	if (sym.symbol != NULL)
	  {
	    write_exp_elt_opcode (state, OP_VAR_VALUE);
	    write_exp_elt_block (state, sym.block);
	    write_exp_elt_sym (state, sym.symbol);
	    write_exp_elt_opcode (state, OP_VAR_VALUE);
	  }
	else
	  {
	    struct type *type;

	    type = rust_lookup_type (varname, expression_context_block);
	    if (type == NULL)
	      error (_("No symbol '%s' in current context"), varname);

	    if (TYPE_CODE (type) == TYPE_CODE_STRUCT
		&& TYPE_NFIELDS (type) == 0)
	      {
		/* A unit-like struct.  */
		write_exp_elt_opcode (state, OP_AGGREGATE);
		write_exp_elt_type (state, type);
		write_exp_elt_longcst (state, 0);
		write_exp_elt_opcode (state, OP_AGGREGATE);
	      }
	    else if (operation == top)
	      {
		write_exp_elt_opcode (state, OP_TYPE);
		write_exp_elt_type (state, type);
		write_exp_elt_opcode (state, OP_TYPE);
		break;
	      }
	  }
      }
      break;

    case OP_AGGREGATE:
      {
	int i;
	int length;
	struct set_field *init;
	VEC (set_field) *fields = *operation->right.field_inits;
	struct type *type;
	const char *name;

	length = 0;
	for (i = 0; VEC_iterate (set_field, fields, i, init); ++i)
	  {
	    if (init->name.ptr != NULL)
	      {
		write_exp_elt_opcode (state, OP_NAME);
		write_exp_string (state, init->name);
		write_exp_elt_opcode (state, OP_NAME);
		++length;
	      }

	    convert_ast_to_expression (state, init->init, top);
	    ++length;

	    if (init->name.ptr == NULL)
	      {
		/* This is handled differently from Ada in our
		   evaluator.  */
		write_exp_elt_opcode (state, OP_OTHERS);
	      }
	  }

	name = convert_name (state, operation->left.op);
	type = rust_lookup_type (name, expression_context_block);
	if (type == NULL)
	  error (_("Could not find type '%s'"), operation->left.sval.ptr);

	if (TYPE_CODE (type) != TYPE_CODE_STRUCT
	    || rust_tuple_type_p (type)
	    || rust_tuple_struct_type_p (type))
	  error (_("Struct expression applied to non-struct type"));

	write_exp_elt_opcode (state, OP_AGGREGATE);
	write_exp_elt_type (state, type);
	write_exp_elt_longcst (state, length);
	write_exp_elt_opcode (state, OP_AGGREGATE);
      }
      break;

    case OP_STRING:
      {
	write_exp_elt_opcode (state, OP_STRING);
	write_exp_string (state, operation->left.sval);
	write_exp_elt_opcode (state, OP_STRING);
      }
      break;

    case OP_RANGE:
      {
	enum range_type kind = BOTH_BOUND_DEFAULT;

	if (operation->left.op != NULL)
	  {
	    convert_ast_to_expression (state, operation->left.op, top);
	    kind = HIGH_BOUND_DEFAULT;
	  }
	if (operation->right.op != NULL)
	  {
	    convert_ast_to_expression (state, operation->right.op, top);
	    if (kind == BOTH_BOUND_DEFAULT)
	      kind = LOW_BOUND_DEFAULT;
	    else
	      {
		gdb_assert (kind == HIGH_BOUND_DEFAULT);
		kind = NONE_BOUND_DEFAULT;
	      }
	  }
	write_exp_elt_opcode (state, OP_RANGE);
	write_exp_elt_longcst (state, kind);
	write_exp_elt_opcode (state, OP_RANGE);
      }
      break;

    default:
      gdb_assert_not_reached ("unhandled opcode in convert_ast_to_expression");
    }
}



/* The parser as exposed to gdb.  */

int
rust_parse (struct parser_state *state)
{
  int result;
  struct cleanup *cleanup;

  obstack_init (&work_obstack);
  cleanup = make_cleanup_obstack_free (&work_obstack);
  rust_ast = NULL;

  pstate = state;
  result = rustyyparse ();

  if (!result || (parse_completion && rust_ast != NULL))
    {
      const struct rust_op *ast = rust_ast;

      rust_ast = NULL;
      gdb_assert (ast != NULL);
      convert_ast_to_expression (state, ast, ast);
    }

  do_cleanups (cleanup);
  return result;
}

/* The parser error handler.  */

void
rustyyerror (const char *msg)
{
  const char *where = prev_lexptr ? prev_lexptr : lexptr;
  error (_("%s in expression, near `%s'."), (msg ? msg : "Error"), where);
}



#if GDB_SELF_TEST

/* Initialize the lexer for testing.  */

static void
rust_lex_test_init (const char *input)
{
  prev_lexptr = NULL;
  lexptr = input;
  paren_depth = 0;
}

/* A test helper that lexes a string, expecting a single token.  It
   returns the lexer data for this token.  */

static RUSTSTYPE
rust_lex_test_one (const char *input, int expected)
{
  int token;
  RUSTSTYPE result;

  rust_lex_test_init (input);

  token = rustyylex ();
  SELF_CHECK (token == expected);
  result = rustyylval;

  if (token)
    {
      token = rustyylex ();
      SELF_CHECK (token == 0);
    }

  return result;
}

/* Test that INPUT lexes as the integer VALUE.  */

static void
rust_lex_int_test (const char *input, int value, int kind)
{
  RUSTSTYPE result = rust_lex_test_one (input, kind);
  SELF_CHECK (result.typed_val_int.val == value);
}

/* Test that INPUT throws an exception with text ERR.  */

static void
rust_lex_exception_test (const char *input, const char *err)
{
  TRY
    {
      /* The "kind" doesn't matter.  */
      rust_lex_test_one (input, DECIMAL_INTEGER);
      SELF_CHECK (0);
    }
  CATCH (except, RETURN_MASK_ERROR)
    {
      SELF_CHECK (strcmp (except.message, err) == 0);
    }
  END_CATCH
}

/* Test that INPUT lexes as the identifier, string, or byte-string
   VALUE.  KIND holds the expected token kind.  */

static void
rust_lex_stringish_test (const char *input, const char *value, int kind)
{
  RUSTSTYPE result = rust_lex_test_one (input, kind);
  SELF_CHECK (result.sval.length == strlen (value));
  SELF_CHECK (strncmp (result.sval.ptr, value, result.sval.length) == 0);
}

/* Helper to test that a string parses as a given token sequence.  */

static void
rust_lex_test_sequence (const char *input, int len, const int expected[])
{
  int i;

  lexptr = input;
  paren_depth = 0;

  for (i = 0; i < len; ++i)
    {
      int token = rustyylex ();

      SELF_CHECK (token == expected[i]);
    }
}

/* Tests for an integer-parsing corner case.  */

static void
rust_lex_test_trailing_dot (void)
{
  const int expected1[] = { DECIMAL_INTEGER, '.', IDENT, '(', ')', 0 };
  const int expected2[] = { INTEGER, '.', IDENT, '(', ')', 0 };
  const int expected3[] = { FLOAT, EQEQ, '(', ')', 0 };
  const int expected4[] = { DECIMAL_INTEGER, DOTDOT, DECIMAL_INTEGER, 0 };

  rust_lex_test_sequence ("23.g()", ARRAY_SIZE (expected1), expected1);
  rust_lex_test_sequence ("23_0.g()", ARRAY_SIZE (expected2), expected2);
  rust_lex_test_sequence ("23.==()", ARRAY_SIZE (expected3), expected3);
  rust_lex_test_sequence ("23..25", ARRAY_SIZE (expected4), expected4);
}

/* Tests of completion.  */

static void
rust_lex_test_completion (void)
{
  const int expected[] = { IDENT, '.', COMPLETE, 0 };

  parse_completion = 1;

  rust_lex_test_sequence ("something.wha", ARRAY_SIZE (expected), expected);
  rust_lex_test_sequence ("something.", ARRAY_SIZE (expected), expected);

  parse_completion = 0;
}

/* Test pushback.  */

static void
rust_lex_test_push_back (void)
{
  int token;

  rust_lex_test_init (">>=");

  token = rustyylex ();
  SELF_CHECK (token == COMPOUND_ASSIGN);
  SELF_CHECK (rustyylval.opcode == BINOP_RSH);

  rust_push_back ('=');

  token = rustyylex ();
  SELF_CHECK (token == '=');

  token = rustyylex ();
  SELF_CHECK (token == 0);
}

/* Unit test the lexer.  */

static void
rust_lex_tests (void)
{
  int i;

  obstack_init (&work_obstack);
  unit_testing = 1;

  rust_lex_test_one ("", 0);
  rust_lex_test_one ("    \t  \n \r  ", 0);
  rust_lex_test_one ("thread 23", 0);
  rust_lex_test_one ("task 23", 0);
  rust_lex_test_one ("th 104", 0);
  rust_lex_test_one ("ta 97", 0);

  rust_lex_int_test ("'z'", 'z', INTEGER);
  rust_lex_int_test ("'\\xff'", 0xff, INTEGER);
  rust_lex_int_test ("'\\u{1016f}'", 0x1016f, INTEGER);
  rust_lex_int_test ("b'z'", 'z', INTEGER);
  rust_lex_int_test ("b'\\xfe'", 0xfe, INTEGER);
  rust_lex_int_test ("b'\\xFE'", 0xfe, INTEGER);
  rust_lex_int_test ("b'\\xfE'", 0xfe, INTEGER);

  /* Test all escapes in both modes.  */
  rust_lex_int_test ("'\\n'", '\n', INTEGER);
  rust_lex_int_test ("'\\r'", '\r', INTEGER);
  rust_lex_int_test ("'\\t'", '\t', INTEGER);
  rust_lex_int_test ("'\\\\'", '\\', INTEGER);
  rust_lex_int_test ("'\\0'", '\0', INTEGER);
  rust_lex_int_test ("'\\''", '\'', INTEGER);
  rust_lex_int_test ("'\\\"'", '"', INTEGER);

  rust_lex_int_test ("b'\\n'", '\n', INTEGER);
  rust_lex_int_test ("b'\\r'", '\r', INTEGER);
  rust_lex_int_test ("b'\\t'", '\t', INTEGER);
  rust_lex_int_test ("b'\\\\'", '\\', INTEGER);
  rust_lex_int_test ("b'\\0'", '\0', INTEGER);
  rust_lex_int_test ("b'\\''", '\'', INTEGER);
  rust_lex_int_test ("b'\\\"'", '"', INTEGER);

  rust_lex_exception_test ("'z", "Unterminated character literal");
  rust_lex_exception_test ("b'\\x0'", "Not enough hex digits seen");
  rust_lex_exception_test ("b'\\u{0}'", "Unicode escape in byte literal");
  rust_lex_exception_test ("'\\x0'", "Not enough hex digits seen");
  rust_lex_exception_test ("'\\u0'", "Missing '{' in Unicode escape");
  rust_lex_exception_test ("'\\u{0", "Missing '}' in Unicode escape");
  rust_lex_exception_test ("'\\u{0000007}", "Overlong hex escape");
  rust_lex_exception_test ("'\\u{}", "Not enough hex digits seen");
  rust_lex_exception_test ("'\\Q'", "Invalid escape \\Q in literal");
  rust_lex_exception_test ("b'\\Q'", "Invalid escape \\Q in literal");

  rust_lex_int_test ("23", 23, DECIMAL_INTEGER);
  rust_lex_int_test ("2_344__29", 234429, INTEGER);
  rust_lex_int_test ("0x1f", 0x1f, INTEGER);
  rust_lex_int_test ("23usize", 23, INTEGER);
  rust_lex_int_test ("23i32", 23, INTEGER);
  rust_lex_int_test ("0x1_f", 0x1f, INTEGER);
  rust_lex_int_test ("0b1_101011__", 0x6b, INTEGER);
  rust_lex_int_test ("0o001177i64", 639, INTEGER);

  rust_lex_test_trailing_dot ();

  rust_lex_test_one ("23.", FLOAT);
  rust_lex_test_one ("23.99f32", FLOAT);
  rust_lex_test_one ("23e7", FLOAT);
  rust_lex_test_one ("23E-7", FLOAT);
  rust_lex_test_one ("23e+7", FLOAT);
  rust_lex_test_one ("23.99e+7f64", FLOAT);
  rust_lex_test_one ("23.82f32", FLOAT);

  rust_lex_stringish_test ("hibob", "hibob", IDENT);
  rust_lex_stringish_test ("hibob__93", "hibob__93", IDENT);
  rust_lex_stringish_test ("thread", "thread", IDENT);

  rust_lex_stringish_test ("\"string\"", "string", STRING);
  rust_lex_stringish_test ("\"str\\ting\"", "str\ting", STRING);
  rust_lex_stringish_test ("\"str\\\"ing\"", "str\"ing", STRING);
  rust_lex_stringish_test ("r\"str\\ing\"", "str\\ing", STRING);
  rust_lex_stringish_test ("r#\"str\\ting\"#", "str\\ting", STRING);
  rust_lex_stringish_test ("r###\"str\\\"ing\"###", "str\\\"ing", STRING);

  rust_lex_stringish_test ("b\"string\"", "string", BYTESTRING);
  rust_lex_stringish_test ("b\"\x73tring\"", "string", BYTESTRING);
  rust_lex_stringish_test ("b\"str\\\"ing\"", "str\"ing", BYTESTRING);
  rust_lex_stringish_test ("br####\"\\x73tring\"####", "\\x73tring",
			   BYTESTRING);

  for (i = 0; i < ARRAY_SIZE (identifier_tokens); ++i)
    rust_lex_test_one (identifier_tokens[i].name, identifier_tokens[i].value);

  for (i = 0; i < ARRAY_SIZE (operator_tokens); ++i)
    rust_lex_test_one (operator_tokens[i].name, operator_tokens[i].value);

  rust_lex_test_completion ();
  rust_lex_test_push_back ();

  obstack_free (&work_obstack, NULL);
  unit_testing = 0;
}

#endif /* GDB_SELF_TEST */

void
_initialize_rust_exp (void)
{
  int code = regcomp (&number_regex, number_regex_text, REG_EXTENDED);
  /* If the regular expression was incorrect, it was a programming
     error.  */
  gdb_assert (code == 0);

#if GDB_SELF_TEST
  register_self_test (rust_lex_tests);
#endif
}
