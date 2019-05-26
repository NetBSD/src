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
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 30 "rust-exp.y" /* yacc.c:339  */


#include "defs.h"

#include "block.h"
#include "charset.h"
#include "cp-support.h"
#include "gdb_obstack.h"
#include "gdb_regex.h"
#include "rust-lang.h"
#include "parser-defs.h"
#include "common/selftest.h"
#include "value.h"
#include "common/vec.h"

#define GDB_YY_REMAP_PREFIX rust
#include "yy-remap.h"

#define RUSTSTYPE YYSTYPE

struct rust_op;
typedef std::vector<const struct rust_op *> rust_op_vector;

/* A typed integer constant.  */

struct typed_val_int
{
  LONGEST val;
  struct type *type;
};

/* A typed floating point constant.  */

struct typed_val_float
{
  gdb_byte val[16];
  struct type *type;
};

/* An identifier and an expression.  This is used to represent one
   element of a struct initializer.  */

struct set_field
{
  struct stoken name;
  const struct rust_op *init;
};

typedef std::vector<set_field> rust_set_vector;


#line 118 "rust-exp.c.tmp" /* yacc.c:339  */

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
    DOTDOTEQ = 279,
    OROR = 280,
    ANDAND = 281,
    EQEQ = 282,
    NOTEQ = 283,
    LTEQ = 284,
    GTEQ = 285,
    LSH = 286,
    RSH = 287,
    COLONCOLON = 288,
    ARROW = 289,
    UNARY = 290
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
#define DOTDOTEQ 279
#define OROR 280
#define ANDAND 281
#define EQEQ 282
#define NOTEQ 283
#define LTEQ 284
#define GTEQ 285
#define LSH 286
#define RSH 287
#define COLONCOLON 288
#define ARROW 289
#define UNARY 290

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 83 "rust-exp.y" /* yacc.c:355  */

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
  rust_op_vector *params;

  /* A list of field initializers.  */
  rust_set_vector *field_inits;

  /* A single field initializer.  */
  struct set_field one_field_init;

  /* An expression.  */
  const struct rust_op *op;

  /* A plain integer, for example used to count the number of
     "super::" prefixes on a path.  */
  unsigned int depth;

#line 256 "rust-exp.c.tmp" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int yyparse (struct rust_parser *parser);



/* Copy the second part of user declarations.  */
#line 114 "rust-exp.y" /* yacc.c:358  */


struct rust_parser;
static int rustyylex (YYSTYPE *, rust_parser *);
static void rustyyerror (rust_parser *parser, const char *msg);

static void rust_push_back (char c);
static struct stoken make_stoken (const char *);
static struct block_symbol rust_lookup_symbol (const char *name,
					       const struct block *block,
					       const domain_enum domain);

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

/* An instance of this is created before parsing, and destroyed when
   parsing is finished.  */

struct rust_parser
{
  rust_parser (struct parser_state *state)
    : rust_ast (nullptr),
      pstate (state)
  {
  }

  ~rust_parser ()
  {
  }

  /* Create a new rust_set_vector.  The storage for the new vector is
     managed by this class.  */
  rust_set_vector *new_set_vector ()
  {
    rust_set_vector *result = new rust_set_vector;
    set_vectors.push_back (std::unique_ptr<rust_set_vector> (result));
    return result;
  }

  /* Create a new rust_ops_vector.  The storage for the new vector is
     managed by this class.  */
  rust_op_vector *new_op_vector ()
  {
    rust_op_vector *result = new rust_op_vector;
    op_vectors.push_back (std::unique_ptr<rust_op_vector> (result));
    return result;
  }

  /* Return the parser's language.  */
  const struct language_defn *language () const
  {
    return parse_language (pstate);
  }

  /* Return the parser's gdbarch.  */
  struct gdbarch *arch () const
  {
    return parse_gdbarch (pstate);
  }

  /* A helper to look up a Rust type, or fail.  This only works for
     types defined by rust_language_arch_info.  */

  struct type *get_type (const char *name)
  {
    struct type *type;

    type = language_lookup_primitive_type (language (), arch (), name);
    if (type == NULL)
      error (_("Could not find Rust type %s"), name);
    return type;
  }

  const char *copy_name (const char *name, int len);
  struct stoken concat3 (const char *s1, const char *s2, const char *s3);
  const struct rust_op *crate_name (const struct rust_op *name);
  const struct rust_op *super_name (const struct rust_op *ident,
				    unsigned int n_supers);

  int lex_character (YYSTYPE *lvalp);
  int lex_number (YYSTYPE *lvalp);
  int lex_string (YYSTYPE *lvalp);
  int lex_identifier (YYSTYPE *lvalp);

  struct type *rust_lookup_type (const char *name, const struct block *block);
  std::vector<struct type *> convert_params_to_types (rust_op_vector *params);
  struct type *convert_ast_to_type (const struct rust_op *operation);
  const char *convert_name (const struct rust_op *operation);
  void convert_params_to_expression (rust_op_vector *params,
				     const struct rust_op *top);
  void convert_ast_to_expression (const struct rust_op *operation,
				  const struct rust_op *top,
				  bool want_type = false);

  struct rust_op *ast_basic_type (enum type_code typecode);
  const struct rust_op *ast_operation (enum exp_opcode opcode,
				       const struct rust_op *left,
				       const struct rust_op *right);
  const struct rust_op *ast_compound_assignment
  (enum exp_opcode opcode, const struct rust_op *left,
   const struct rust_op *rust_op);
  const struct rust_op *ast_literal (struct typed_val_int val);
  const struct rust_op *ast_dliteral (struct typed_val_float val);
  const struct rust_op *ast_structop (const struct rust_op *left,
				      const char *name,
				      int completing);
  const struct rust_op *ast_structop_anonymous
  (const struct rust_op *left, struct typed_val_int number);
  const struct rust_op *ast_unary (enum exp_opcode opcode,
				   const struct rust_op *expr);
  const struct rust_op *ast_cast (const struct rust_op *expr,
				  const struct rust_op *type);
  const struct rust_op *ast_call_ish (enum exp_opcode opcode,
				      const struct rust_op *expr,
				      rust_op_vector *params);
  const struct rust_op *ast_path (struct stoken name,
				  rust_op_vector *params);
  const struct rust_op *ast_string (struct stoken str);
  const struct rust_op *ast_struct (const struct rust_op *name,
				    rust_set_vector *fields);
  const struct rust_op *ast_range (const struct rust_op *lhs,
				   const struct rust_op *rhs,
				   bool inclusive);
  const struct rust_op *ast_array_type (const struct rust_op *lhs,
					struct typed_val_int val);
  const struct rust_op *ast_slice_type (const struct rust_op *type);
  const struct rust_op *ast_reference_type (const struct rust_op *type);
  const struct rust_op *ast_pointer_type (const struct rust_op *type,
					  int is_mut);
  const struct rust_op *ast_function_type (const struct rust_op *result,
					   rust_op_vector *params);
  const struct rust_op *ast_tuple_type (rust_op_vector *params);


  /* A pointer to this is installed globally.  */
  auto_obstack obstack;

  /* Result of parsing.  Points into obstack.  */
  const struct rust_op *rust_ast;

  /* This keeps track of the various vectors we allocate.  */
  std::vector<std::unique_ptr<rust_set_vector>> set_vectors;
  std::vector<std::unique_ptr<rust_op_vector>> op_vectors;

  /* The parser state gdb gave us.  */
  struct parser_state *pstate;
};

/* Rust AST operations.  We build a tree of these; then lower them to
   gdb expressions when parsing has completed.  */

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
  /* For OP_RANGE, indicates whether the range is inclusive or
     exclusive.  */
  unsigned int inclusive : 1;
  /* Operands of expression.  Which one is used and how depends on the
     particular opcode.  */
  RUSTSTYPE left;
  RUSTSTYPE right;
};


#line 493 "rust-exp.c.tmp" /* yacc.c:358  */

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
#define YYFINAL  62
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1124

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  59
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  35
/* YYNRULES -- Number of rules.  */
#define YYNRULES  125
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  219

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   290

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    58,     2,     2,     2,    46,    40,     2,
      50,    52,    44,    42,    51,    43,    49,    45,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    55,    57,
      36,    35,    37,     2,    41,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    48,     2,    56,    39,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    53,    38,    54,     2,     2,     2,     2,
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
      47
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   432,   432,   445,   446,   447,   448,   449,   450,   451,
     452,   453,   454,   456,   457,   458,   462,   470,   487,   492,
     502,   510,   522,   525,   531,   540,   552,   554,   556,   558,
     563,   565,   567,   569,   571,   573,   578,   580,   582,   584,
     612,   614,   623,   635,   637,   642,   647,   652,   655,   658,
     667,   670,   673,   675,   680,   681,   682,   683,   687,   690,
     693,   696,   699,   702,   705,   708,   711,   714,   717,   720,
     723,   726,   729,   732,   735,   738,   741,   746,   751,   756,
     762,   767,   772,   781,   785,   790,   795,   799,   801,   805,
     807,   812,   814,   816,   821,   822,   824,   826,   828,   842,
     844,   850,   852,   860,   861,   863,   865,   867,   881,   883,
     892,   893,   895,   903,   904,   906,   908,   910,   912,   914,
     916,   918,   924,   925,   930,   936
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
  "KW_EXTERN", "KW_CONST", "KW_FN", "KW_SIZEOF", "DOTDOT", "DOTDOTEQ",
  "OROR", "ANDAND", "EQEQ", "NOTEQ", "LTEQ", "GTEQ", "LSH", "RSH",
  "COLONCOLON", "ARROW", "'='", "'<'", "'>'", "'|'", "'^'", "'&'", "'@'",
  "'+'", "'-'", "'*'", "'/'", "'%'", "UNARY", "'['", "'.'", "'('", "','",
  "')'", "'{'", "'}'", "':'", "']'", "';'", "'!'", "$accept", "start",
  "expr", "tuple_expr", "unit_expr", "struct_expr", "struct_expr_tail",
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
     285,   286,   287,   288,   289,    61,    60,    62,   124,    94,
      38,    64,    43,    45,    42,    47,    37,   290,    91,    46,
      40,    44,    41,   123,   125,    58,    93,    59,    33
};
# endif

#define YYPACT_NINF -181

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-181)))

#define YYTABLE_NINF -123

#define yytable_value_is_error(Yytable_value) \
  (!!((Yytable_value) == (-123)))

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     423,  -181,  -181,  -181,  -181,  -181,  -181,  -181,  -181,  -181,
     -18,    19,     8,   158,   423,    19,   211,   423,   423,   423,
     264,   317,   423,    49,   826,  -181,  -181,  -181,  -181,  -181,
    -181,  -181,  -181,  -181,  -181,  -181,  -181,  -181,  -181,  -181,
    -181,    73,  -181,    57,    85,    19,    85,   423,   866,   866,
      85,   423,    84,    84,    84,    84,   423,   463,   -25,  -181,
     701,    84,  -181,   423,   546,   158,   423,   423,   423,   423,
     423,   423,   423,   423,   423,   423,   423,   423,   423,   423,
     423,   423,   423,   423,   423,   423,   423,   423,    90,   370,
    -181,   103,    89,     5,    -1,    85,   743,    84,   503,    -3,
     423,   423,  -181,   370,  -181,   906,  -181,   108,   109,    96,
     109,   551,   122,   546,    87,    73,  -181,    63,  -181,  -181,
     866,   866,   941,   976,  1011,  1011,  1011,  1011,    80,    80,
     906,  1011,  1011,  1032,  1053,  1074,   144,    -6,    -6,    12,
      12,    12,   581,  -181,  -181,  -181,   826,    97,    95,  -181,
     116,    85,   -43,   423,  -181,    98,  -181,   546,  -181,   423,
    -181,   621,   826,   102,   109,  -181,    87,  -181,   546,  -181,
     546,   546,    93,  -181,   105,   100,    99,   154,   546,  -181,
    -181,  -181,     5,   423,   826,  -181,    -5,   661,  -181,  -181,
    -181,   111,   -27,  -181,  -181,   138,  -181,   546,  -181,  -181,
      13,  -181,   785,  -181,  -181,  -181,   135,  -181,   114,   115,
    -181,  -181,  -181,     5,   546,  -181,  -181,  -181,  -181
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
      87,    92,    99,    36,    37,    39,    40,    38,    41,    42,
      93,     0,     0,    35,    87,     0,    87,    87,    87,    87,
      87,    87,    87,     0,     2,     5,     6,     7,     9,    11,
       3,     8,    10,    12,    13,    54,    55,    56,    57,    14,
      15,     0,     4,    91,    94,    88,    98,    87,    33,    34,
      97,    87,    51,    47,    48,    50,    87,    81,     0,    17,
       0,    49,     1,    87,    87,    30,    87,    87,    87,    87,
      87,    87,    87,    87,    87,    87,    87,    87,    87,    87,
      87,    87,    87,    87,    87,    87,    87,    87,     0,    83,
      86,     0,     0,    22,     0,    95,     0,    52,    81,     0,
      87,    87,    27,    83,    80,    79,   108,     0,     0,     0,
       0,    87,     0,    87,    87,     0,   113,   110,   103,    77,
      31,    32,    69,    70,    71,    72,    73,    74,    75,    76,
      78,    62,    63,    65,    66,    64,    59,    67,    68,    58,
      60,    61,     0,    43,    44,    45,    81,    84,     0,    89,
       0,    96,    21,    87,    23,     0,   100,    87,    53,    87,
      26,     0,    82,     0,    88,   107,    87,   106,    87,   117,
      87,    87,     0,   124,     0,   123,     0,     0,    87,    46,
      85,    90,    22,    87,    19,    18,     0,     0,    29,    16,
     104,     0,     0,   118,   119,     0,   121,    87,   105,   109,
       0,    25,    20,   102,   101,    28,     0,   116,     0,     0,
     125,   112,   111,    22,    87,   114,   115,    24,   120
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -181,  -181,     0,  -181,  -181,  -181,  -181,  -180,  -181,  -181,
    -181,  -181,  -181,  -181,  -181,  -181,  -181,  -181,  -181,  -181,
     -19,    75,  -181,  -181,   -59,    61,  -181,  -181,    -4,  -181,
    -181,   -74,   -54,    18,  -153
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    23,   146,    25,    26,    27,   154,   155,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
     147,   148,    90,    40,    41,    92,    42,    43,    44,   116,
     117,   118,   173,   174,   175
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      24,    58,   201,   156,   186,   115,    64,    46,   182,   152,
     119,    50,   183,    48,    49,    45,    52,    53,    54,    55,
      57,    60,    61,     2,    64,   200,   101,   203,   153,   207,
     195,   102,   204,   217,   165,   157,   167,    99,    84,    85,
      86,    95,    87,    88,    89,   211,   197,    96,   101,    62,
     212,    97,   115,   160,   115,   115,    98,   169,    47,   172,
      87,    88,    89,   105,   197,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   151,    91,
     190,   106,    64,     2,   143,   144,   177,   145,   115,   178,
     161,   162,   198,   106,   107,   150,   108,   115,   109,   115,
      93,   115,   115,   106,   192,   150,   193,   194,    94,   115,
     110,    81,    82,    83,    84,    85,    86,   111,    87,    88,
      89,   112,    87,    88,    89,   113,   149,   114,   115,  -122,
     170,   164,   171,   210,   208,   209,   166,   180,   101,   181,
     195,   197,   185,   184,   189,   115,    64,   196,   199,   187,
     218,     1,     2,   206,     3,     4,     5,     6,     7,   214,
     215,   216,     8,     9,   -87,    10,   176,    11,   163,     0,
      12,  -123,  -123,   202,   191,     0,    82,    83,    84,    85,
      86,    15,    87,    88,    89,     0,     0,     0,    16,     0,
      17,    18,    19,     0,     0,     0,    20,     0,    21,     0,
       0,     0,     0,     0,     1,     2,    22,     3,     4,     5,
       6,     7,     0,     0,     0,     8,     9,     0,    10,    51,
      11,     0,     0,    12,    13,    14,     0,     0,     0,     0,
       0,     0,     0,     0,    15,     0,     0,     0,     0,     0,
       0,    16,     0,    17,    18,    19,     0,     0,     0,    20,
       0,    21,     0,     0,     0,     0,     0,     1,     2,    22,
       3,     4,     5,     6,     7,     0,     0,     0,     8,     9,
       0,    10,    56,    11,     0,     0,    12,    13,    14,     0,
       0,     0,     0,     0,     0,     0,     0,    15,     0,     0,
       0,     0,     0,     0,    16,     0,    17,    18,    19,     0,
       0,     0,    20,     0,    21,     0,     0,     0,     0,     0,
       1,     2,    22,     3,     4,     5,     6,     7,     0,     0,
       0,     8,     9,     0,    10,     0,    11,     0,     0,    12,
      13,    14,     0,     0,     0,     0,     0,     0,     0,     0,
      15,     0,     0,     0,     0,     0,     0,    16,     0,    17,
      18,    19,     0,     0,     0,    20,     0,    21,     0,    59,
       0,     0,     0,     1,     2,    22,     3,     4,     5,     6,
       7,     0,     0,     0,     8,     9,   -87,    10,     0,    11,
       0,     0,    12,    13,    14,     0,     0,     0,     0,     0,
       0,     0,     0,    15,     0,     0,     0,     0,     0,     0,
      16,     0,    17,    18,    19,     0,     0,     0,    20,     0,
      21,     0,     0,     0,     0,     0,     1,     2,    22,     3,
       4,     5,     6,     7,     0,     0,     0,     8,     9,     0,
      10,     0,    11,     0,     0,    12,    13,    14,     0,     0,
       0,     0,     0,     0,     0,     0,    15,     0,     0,     0,
       0,     0,     0,    16,     0,    17,    18,    19,     0,     0,
       0,    20,     0,    21,    63,    64,     0,     0,     0,     0,
       0,    22,     0,     0,     0,     0,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,     0,     0,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
       0,    87,    88,    89,    63,    64,     0,     0,     0,     0,
     100,     0,     0,     0,     0,     0,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,     0,     0,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
     106,    87,    88,    89,     0,   106,     0,     0,     0,     0,
     159,     0,     0,   107,     0,   108,     0,   109,   107,     0,
     108,     0,   109,     0,     0,     0,     0,     0,     0,   110,
       0,     0,     0,     0,   110,     0,   111,     0,     0,     0,
     112,   111,    63,    64,   113,   112,   114,     0,     0,   168,
       0,   114,     0,     0,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,     0,     0,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,     0,    87,
      88,    89,    63,    64,     0,     0,     0,   179,     0,     0,
       0,     0,     0,     0,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,     0,     0,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,     0,    87,
      88,    89,    63,    64,     0,     0,     0,   188,     0,     0,
       0,     0,     0,     0,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,     0,     0,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,     0,    87,
      88,    89,    63,    64,     0,     0,     0,   205,     0,     0,
       0,     0,     0,     0,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,     0,     0,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,     0,    87,
      88,    89,   103,   104,    63,    64,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,     0,     0,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
       0,    87,    88,    89,     0,   158,    63,    64,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,     0,     0,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,     0,    87,    88,    89,   213,    63,    64,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,     0,
       0,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,     0,    87,    88,    89,    63,    64,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  -123,
    -123,    67,    68,    69,    70,    71,    72,    73,    74,     0,
       0,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,     0,    87,    88,    89,    63,    64,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    67,    68,    69,    70,    71,    72,    73,    74,     0,
       0,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    64,    87,    88,    89,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    68,    69,    70,
      71,    72,    73,    74,     0,     0,     0,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    64,    87,
      88,    89,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    69,    70,    71,    72,    73,    74,     0,
       0,     0,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    64,    87,    88,    89,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  -123,  -123,
    -123,  -123,    73,    74,    64,     0,     0,  -123,  -123,    78,
      79,    80,    81,    82,    83,    84,    85,    86,     0,    87,
      88,    89,     0,    73,    74,    64,     0,     0,     0,     0,
       0,    79,    80,    81,    82,    83,    84,    85,    86,     0,
      87,    88,    89,     0,    73,    74,    64,     0,     0,     0,
       0,     0,     0,    80,    81,    82,    83,    84,    85,    86,
       0,    87,    88,    89,     0,    73,    74,     0,     0,     0,
       0,     0,     0,     0,     0,    81,    82,    83,    84,    85,
      86,     0,    87,    88,    89
};

static const yytype_int16 yycheck[] =
{
       0,    20,   182,     4,   157,    64,    12,    11,    51,     4,
      64,    15,    55,    13,    14,    33,    16,    17,    18,    19,
      20,    21,    22,     4,    12,   178,    51,    32,    23,    56,
      57,    56,    37,   213,   108,    36,   110,    56,    44,    45,
      46,    45,    48,    49,    50,    32,    51,    47,    51,     0,
      37,    51,   111,    56,   113,   114,    56,   111,    50,   113,
      48,    49,    50,    63,    51,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    92,    16,
     164,     4,    12,     4,     4,     5,    33,     7,   157,    36,
     100,   101,   176,     4,    17,    16,    19,   166,    21,   168,
      53,   170,   171,     4,   168,    16,   170,   171,    33,   178,
      33,    41,    42,    43,    44,    45,    46,    40,    48,    49,
      50,    44,    48,    49,    50,    48,    33,    50,   197,    52,
      18,    33,    20,   197,     6,     7,    50,    52,    51,    33,
      57,    51,    54,   153,    52,   214,    12,    52,     4,   159,
     214,     3,     4,    52,     6,     7,     8,     9,    10,    34,
      56,    56,    14,    15,    16,    17,   115,    19,   103,    -1,
      22,    23,    24,   183,   166,    -1,    42,    43,    44,    45,
      46,    33,    48,    49,    50,    -1,    -1,    -1,    40,    -1,
      42,    43,    44,    -1,    -1,    -1,    48,    -1,    50,    -1,
      -1,    -1,    -1,    -1,     3,     4,    58,     6,     7,     8,
       9,    10,    -1,    -1,    -1,    14,    15,    -1,    17,    18,
      19,    -1,    -1,    22,    23,    24,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    33,    -1,    -1,    -1,    -1,    -1,
      -1,    40,    -1,    42,    43,    44,    -1,    -1,    -1,    48,
      -1,    50,    -1,    -1,    -1,    -1,    -1,     3,     4,    58,
       6,     7,     8,     9,    10,    -1,    -1,    -1,    14,    15,
      -1,    17,    18,    19,    -1,    -1,    22,    23,    24,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    33,    -1,    -1,
      -1,    -1,    -1,    -1,    40,    -1,    42,    43,    44,    -1,
      -1,    -1,    48,    -1,    50,    -1,    -1,    -1,    -1,    -1,
       3,     4,    58,     6,     7,     8,     9,    10,    -1,    -1,
      -1,    14,    15,    -1,    17,    -1,    19,    -1,    -1,    22,
      23,    24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      33,    -1,    -1,    -1,    -1,    -1,    -1,    40,    -1,    42,
      43,    44,    -1,    -1,    -1,    48,    -1,    50,    -1,    52,
      -1,    -1,    -1,     3,     4,    58,     6,     7,     8,     9,
      10,    -1,    -1,    -1,    14,    15,    16,    17,    -1,    19,
      -1,    -1,    22,    23,    24,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    33,    -1,    -1,    -1,    -1,    -1,    -1,
      40,    -1,    42,    43,    44,    -1,    -1,    -1,    48,    -1,
      50,    -1,    -1,    -1,    -1,    -1,     3,     4,    58,     6,
       7,     8,     9,    10,    -1,    -1,    -1,    14,    15,    -1,
      17,    -1,    19,    -1,    -1,    22,    23,    24,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    33,    -1,    -1,    -1,
      -1,    -1,    -1,    40,    -1,    42,    43,    44,    -1,    -1,
      -1,    48,    -1,    50,    11,    12,    -1,    -1,    -1,    -1,
      -1,    58,    -1,    -1,    -1,    -1,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    -1,    -1,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      -1,    48,    49,    50,    11,    12,    -1,    -1,    -1,    -1,
      57,    -1,    -1,    -1,    -1,    -1,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    -1,    -1,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
       4,    48,    49,    50,    -1,     4,    -1,    -1,    -1,    -1,
      57,    -1,    -1,    17,    -1,    19,    -1,    21,    17,    -1,
      19,    -1,    21,    -1,    -1,    -1,    -1,    -1,    -1,    33,
      -1,    -1,    -1,    -1,    33,    -1,    40,    -1,    -1,    -1,
      44,    40,    11,    12,    48,    44,    50,    -1,    -1,    48,
      -1,    50,    -1,    -1,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    -1,    -1,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    -1,    48,
      49,    50,    11,    12,    -1,    -1,    -1,    56,    -1,    -1,
      -1,    -1,    -1,    -1,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    -1,    -1,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    -1,    48,
      49,    50,    11,    12,    -1,    -1,    -1,    56,    -1,    -1,
      -1,    -1,    -1,    -1,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    -1,    -1,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    -1,    48,
      49,    50,    11,    12,    -1,    -1,    -1,    56,    -1,    -1,
      -1,    -1,    -1,    -1,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    -1,    -1,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    -1,    48,
      49,    50,    51,    52,    11,    12,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    -1,    -1,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      -1,    48,    49,    50,    -1,    52,    11,    12,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    -1,    -1,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    -1,    48,    49,    50,    51,    11,    12,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    -1,
      -1,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    -1,    48,    49,    50,    11,    12,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    -1,
      -1,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    -1,    48,    49,    50,    11,    12,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    25,    26,    27,    28,    29,    30,    31,    32,    -1,
      -1,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    12,    48,    49,    50,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    26,    27,    28,
      29,    30,    31,    32,    -1,    -1,    -1,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    12,    48,
      49,    50,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    27,    28,    29,    30,    31,    32,    -1,
      -1,    -1,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    12,    48,    49,    50,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    27,    28,
      29,    30,    31,    32,    12,    -1,    -1,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    -1,    48,
      49,    50,    -1,    31,    32,    12,    -1,    -1,    -1,    -1,
      -1,    39,    40,    41,    42,    43,    44,    45,    46,    -1,
      48,    49,    50,    -1,    31,    32,    12,    -1,    -1,    -1,
      -1,    -1,    -1,    40,    41,    42,    43,    44,    45,    46,
      -1,    48,    49,    50,    -1,    31,    32,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    41,    42,    43,    44,    45,
      46,    -1,    48,    49,    50
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     6,     7,     8,     9,    10,    14,    15,
      17,    19,    22,    23,    24,    33,    40,    42,    43,    44,
      48,    50,    58,    60,    61,    62,    63,    64,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      82,    83,    85,    86,    87,    33,    87,    50,    61,    61,
      87,    18,    61,    61,    61,    61,    18,    61,    79,    52,
      61,    61,     0,    11,    12,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    48,    49,    50,
      81,    16,    84,    53,    33,    87,    61,    61,    61,    79,
      57,    51,    56,    51,    52,    61,     4,    17,    19,    21,
      33,    40,    44,    48,    50,    83,    88,    89,    90,    91,
      61,    61,    61,    61,    61,    61,    61,    61,    61,    61,
      61,    61,    61,    61,    61,    61,    61,    61,    61,    61,
      61,    61,    61,     4,     5,     7,    61,    79,    80,    33,
      16,    87,     4,    23,    65,    66,     4,    36,    52,    57,
      56,    61,    61,    80,    33,    90,    50,    90,    48,    91,
      18,    20,    91,    91,    92,    93,    84,    33,    36,    56,
      52,    33,    51,    55,    61,    54,    93,    61,    56,    52,
      90,    92,    91,    91,    91,    57,    52,    51,    90,     4,
      93,    66,    61,    32,    37,    56,    52,    56,     6,     7,
      91,    32,    37,    51,    34,    56,    56,    66,    91
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    59,    60,    61,    61,    61,    61,    61,    61,    61,
      61,    61,    61,    61,    61,    61,    62,    63,    64,    65,
      65,    65,    66,    66,    66,    66,    67,    67,    67,    67,
      68,    68,    68,    68,    68,    68,    69,    69,    69,    69,
      69,    69,    69,    70,    70,    70,    71,    72,    72,    72,
      72,    72,    72,    72,    73,    73,    73,    73,    74,    74,
      74,    74,    74,    74,    74,    74,    74,    74,    74,    74,
      74,    74,    74,    74,    74,    74,    74,    75,    76,    77,
      78,    79,    79,    80,    80,    81,    82,    83,    83,    84,
      84,    85,    85,    85,    86,    86,    86,    86,    86,    87,
      87,    87,    87,    88,    88,    88,    88,    88,    89,    89,
      90,    90,    90,    91,    91,    91,    91,    91,    91,    91,
      91,    91,    92,    92,    93,    93
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     5,     2,     4,     2,
       3,     1,     0,     1,     5,     3,     4,     3,     6,     5,
       2,     3,     3,     2,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     3,     3,     3,     4,     2,     2,     2,
       2,     2,     3,     4,     1,     1,     1,     1,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     1,     3,     0,     1,     3,     2,     0,     2,     2,
       3,     1,     1,     1,     1,     3,     3,     2,     2,     1,
       3,     5,     5,     1,     3,     3,     2,     2,     1,     3,
       1,     4,     4,     1,     5,     5,     4,     2,     3,     3,
       6,     3,     0,     1,     1,     3
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
      yyerror (parser, YY_("syntax error: cannot back up")); \
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
                  Type, Value, parser); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, struct rust_parser *parser)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  YYUSE (parser);
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
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, struct rust_parser *parser)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, parser);
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
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule, struct rust_parser *parser)
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
                                              , parser);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, parser); \
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
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, struct rust_parser *parser)
{
  YYUSE (yyvaluep);
  YYUSE (parser);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/*----------.
| yyparse.  |
`----------*/

int
yyparse (struct rust_parser *parser)
{
/* The lookahead symbol.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs;

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
      yychar = yylex (&yylval, parser);
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
#line 433 "rust-exp.y" /* yacc.c:1646  */
    {
		  /* If we are completing and see a valid parse,
		     rust_ast will already have been set.  */
		  if (parser->rust_ast == NULL)
		    parser->rust_ast = (yyvsp[0].op);
		}
#line 1922 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 16:
#line 463 "rust-exp.y" /* yacc.c:1646  */
    {
		  (yyvsp[-1].params)->push_back ((yyvsp[-3].op));
		  error (_("Tuple expressions not supported yet"));
		}
#line 1931 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 17:
#line 471 "rust-exp.y" /* yacc.c:1646  */
    {
		  struct typed_val_int val;

		  val.type
		    = (language_lookup_primitive_type
		       (parser->language (), parser->arch (),
			"()"));
		  val.val = 0;
		  (yyval.op) = parser->ast_literal (val);
		}
#line 1946 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 18:
#line 488 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_struct ((yyvsp[-3].op), (yyvsp[-1].field_inits)); }
#line 1952 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 19:
#line 493 "rust-exp.y" /* yacc.c:1646  */
    {
		  struct set_field sf;

		  sf.name.ptr = NULL;
		  sf.name.length = 0;
		  sf.init = (yyvsp[0].op);

		  (yyval.one_field_init) = sf;
		}
#line 1966 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 20:
#line 503 "rust-exp.y" /* yacc.c:1646  */
    {
		  struct set_field sf;

		  sf.name = (yyvsp[-2].sval);
		  sf.init = (yyvsp[0].op);
		  (yyval.one_field_init) = sf;
		}
#line 1978 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 21:
#line 511 "rust-exp.y" /* yacc.c:1646  */
    {
		  struct set_field sf;

		  sf.name = (yyvsp[0].sval);
		  sf.init = parser->ast_path ((yyvsp[0].sval), NULL);
		  (yyval.one_field_init) = sf;
		}
#line 1990 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 22:
#line 522 "rust-exp.y" /* yacc.c:1646  */
    {
		  (yyval.field_inits) = parser->new_set_vector ();
		}
#line 1998 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 23:
#line 526 "rust-exp.y" /* yacc.c:1646  */
    {
		  rust_set_vector *result = parser->new_set_vector ();
		  result->push_back ((yyvsp[0].one_field_init));
		  (yyval.field_inits) = result;
		}
#line 2008 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 24:
#line 532 "rust-exp.y" /* yacc.c:1646  */
    {
		  struct set_field sf;

		  sf.name = (yyvsp[-4].sval);
		  sf.init = (yyvsp[-2].op);
		  (yyvsp[0].field_inits)->push_back (sf);
		  (yyval.field_inits) = (yyvsp[0].field_inits);
		}
#line 2021 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 25:
#line 541 "rust-exp.y" /* yacc.c:1646  */
    {
		  struct set_field sf;

		  sf.name = (yyvsp[-2].sval);
		  sf.init = parser->ast_path ((yyvsp[-2].sval), NULL);
		  (yyvsp[0].field_inits)->push_back (sf);
		  (yyval.field_inits) = (yyvsp[0].field_inits);
		}
#line 2034 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 26:
#line 553 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_call_ish (OP_ARRAY, NULL, (yyvsp[-1].params)); }
#line 2040 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 27:
#line 555 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_call_ish (OP_ARRAY, NULL, (yyvsp[-1].params)); }
#line 2046 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 28:
#line 557 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_operation (OP_RUST_ARRAY, (yyvsp[-3].op), (yyvsp[-1].op)); }
#line 2052 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 29:
#line 559 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_operation (OP_RUST_ARRAY, (yyvsp[-3].op), (yyvsp[-1].op)); }
#line 2058 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 30:
#line 564 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_range ((yyvsp[-1].op), NULL, false); }
#line 2064 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 31:
#line 566 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_range ((yyvsp[-2].op), (yyvsp[0].op), false); }
#line 2070 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 32:
#line 568 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_range ((yyvsp[-2].op), (yyvsp[0].op), true); }
#line 2076 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 33:
#line 570 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_range (NULL, (yyvsp[0].op), false); }
#line 2082 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 34:
#line 572 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_range (NULL, (yyvsp[0].op), true); }
#line 2088 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 35:
#line 574 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_range (NULL, NULL, false); }
#line 2094 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 36:
#line 579 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_literal ((yyvsp[0].typed_val_int)); }
#line 2100 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 37:
#line 581 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_literal ((yyvsp[0].typed_val_int)); }
#line 2106 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 38:
#line 583 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_dliteral ((yyvsp[0].typed_val_float)); }
#line 2112 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 39:
#line 585 "rust-exp.y" /* yacc.c:1646  */
    {
		  struct set_field field;
		  struct typed_val_int val;
		  struct stoken token;

		  rust_set_vector *fields = parser->new_set_vector ();

		  /* Wrap the raw string in the &str struct.  */
		  field.name.ptr = "data_ptr";
		  field.name.length = strlen (field.name.ptr);
		  field.init = parser->ast_unary (UNOP_ADDR,
						  parser->ast_string ((yyvsp[0].sval)));
		  fields->push_back (field);

		  val.type = parser->get_type ("usize");
		  val.val = (yyvsp[0].sval).length;

		  field.name.ptr = "length";
		  field.name.length = strlen (field.name.ptr);
		  field.init = parser->ast_literal (val);
		  fields->push_back (field);

		  token.ptr = "&str";
		  token.length = strlen (token.ptr);
		  (yyval.op) = parser->ast_struct (parser->ast_path (token, NULL),
					   fields);
		}
#line 2144 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 40:
#line 613 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_string ((yyvsp[0].sval)); }
#line 2150 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 41:
#line 615 "rust-exp.y" /* yacc.c:1646  */
    {
		  struct typed_val_int val;

		  val.type = language_bool_type (parser->language (),
						 parser->arch ());
		  val.val = 1;
		  (yyval.op) = parser->ast_literal (val);
		}
#line 2163 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 42:
#line 624 "rust-exp.y" /* yacc.c:1646  */
    {
		  struct typed_val_int val;

		  val.type = language_bool_type (parser->language (),
						 parser->arch ());
		  val.val = 0;
		  (yyval.op) = parser->ast_literal (val);
		}
#line 2176 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 43:
#line 636 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_structop ((yyvsp[-2].op), (yyvsp[0].sval).ptr, 0); }
#line 2182 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 44:
#line 638 "rust-exp.y" /* yacc.c:1646  */
    {
		  (yyval.op) = parser->ast_structop ((yyvsp[-2].op), (yyvsp[0].sval).ptr, 1);
		  parser->rust_ast = (yyval.op);
		}
#line 2191 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 45:
#line 643 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_structop_anonymous ((yyvsp[-2].op), (yyvsp[0].typed_val_int)); }
#line 2197 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 46:
#line 648 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_operation (BINOP_SUBSCRIPT, (yyvsp[-3].op), (yyvsp[-1].op)); }
#line 2203 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 47:
#line 653 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_unary (UNOP_PLUS, (yyvsp[0].op)); }
#line 2209 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 48:
#line 656 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_unary (UNOP_NEG, (yyvsp[0].op)); }
#line 2215 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 49:
#line 659 "rust-exp.y" /* yacc.c:1646  */
    {
		  /* Note that we provide a Rust-specific evaluator
		     override for UNOP_COMPLEMENT, so it can do the
		     right thing for both bool and integral
		     values.  */
		  (yyval.op) = parser->ast_unary (UNOP_COMPLEMENT, (yyvsp[0].op));
		}
#line 2227 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 50:
#line 668 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_unary (UNOP_IND, (yyvsp[0].op)); }
#line 2233 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 51:
#line 671 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_unary (UNOP_ADDR, (yyvsp[0].op)); }
#line 2239 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 52:
#line 674 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_unary (UNOP_ADDR, (yyvsp[0].op)); }
#line 2245 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 53:
#line 676 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_unary (UNOP_SIZEOF, (yyvsp[-1].op)); }
#line 2251 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 58:
#line 688 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_operation (BINOP_MUL, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2257 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 59:
#line 691 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_operation (BINOP_REPEAT, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2263 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 60:
#line 694 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_operation (BINOP_DIV, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2269 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 61:
#line 697 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_operation (BINOP_REM, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2275 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 62:
#line 700 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_operation (BINOP_LESS, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2281 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 63:
#line 703 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_operation (BINOP_GTR, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2287 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 64:
#line 706 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_operation (BINOP_BITWISE_AND, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2293 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 65:
#line 709 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_operation (BINOP_BITWISE_IOR, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2299 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 66:
#line 712 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_operation (BINOP_BITWISE_XOR, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2305 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 67:
#line 715 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_operation (BINOP_ADD, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2311 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 68:
#line 718 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_operation (BINOP_SUB, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2317 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 69:
#line 721 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_operation (BINOP_LOGICAL_OR, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2323 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 70:
#line 724 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_operation (BINOP_LOGICAL_AND, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2329 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 71:
#line 727 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_operation (BINOP_EQUAL, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2335 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 72:
#line 730 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_operation (BINOP_NOTEQUAL, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2341 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 73:
#line 733 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_operation (BINOP_LEQ, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2347 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 74:
#line 736 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_operation (BINOP_GEQ, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2353 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 75:
#line 739 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_operation (BINOP_LSH, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2359 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 76:
#line 742 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_operation (BINOP_RSH, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2365 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 77:
#line 747 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_cast ((yyvsp[-2].op), (yyvsp[0].op)); }
#line 2371 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 78:
#line 752 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_operation (BINOP_ASSIGN, (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2377 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 79:
#line 757 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_compound_assignment ((yyvsp[-1].opcode), (yyvsp[-2].op), (yyvsp[0].op)); }
#line 2383 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 80:
#line 763 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = (yyvsp[-1].op); }
#line 2389 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 81:
#line 768 "rust-exp.y" /* yacc.c:1646  */
    {
		  (yyval.params) = parser->new_op_vector ();
		  (yyval.params)->push_back ((yyvsp[0].op));
		}
#line 2398 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 82:
#line 773 "rust-exp.y" /* yacc.c:1646  */
    {
		  (yyvsp[-2].params)->push_back ((yyvsp[0].op));
		  (yyval.params) = (yyvsp[-2].params);
		}
#line 2407 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 83:
#line 781 "rust-exp.y" /* yacc.c:1646  */
    {
		  /* The result can't be NULL.  */
		  (yyval.params) = parser->new_op_vector ();
		}
#line 2416 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 84:
#line 786 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.params) = (yyvsp[0].params); }
#line 2422 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 85:
#line 791 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.params) = (yyvsp[-1].params); }
#line 2428 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 86:
#line 796 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_call_ish (OP_FUNCALL, (yyvsp[-1].op), (yyvsp[0].params)); }
#line 2434 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 89:
#line 806 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.depth) = 1; }
#line 2440 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 90:
#line 808 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.depth) = (yyvsp[-2].depth) + 1; }
#line 2446 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 91:
#line 813 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = (yyvsp[0].op); }
#line 2452 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 92:
#line 815 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_path ((yyvsp[0].sval), NULL); }
#line 2458 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 93:
#line 817 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_path (make_stoken ("self"), NULL); }
#line 2464 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 95:
#line 823 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->super_name ((yyvsp[0].op), 0); }
#line 2470 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 96:
#line 825 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->super_name ((yyvsp[0].op), (yyvsp[-1].depth)); }
#line 2476 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 97:
#line 827 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->crate_name ((yyvsp[0].op)); }
#line 2482 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 98:
#line 829 "rust-exp.y" /* yacc.c:1646  */
    {
		  /* This is a gdb extension to make it possible to
		     refer to items in other crates.  It just bypasses
		     adding the current crate to the front of the
		     name.  */
		  (yyval.op) = parser->ast_path (parser->concat3 ("::",
							  (yyvsp[0].op)->left.sval.ptr,
							  NULL),
					 (yyvsp[0].op)->right.params);
		}
#line 2497 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 99:
#line 843 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_path ((yyvsp[0].sval), NULL); }
#line 2503 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 100:
#line 845 "rust-exp.y" /* yacc.c:1646  */
    {
		  (yyval.op) = parser->ast_path (parser->concat3 ((yyvsp[-2].op)->left.sval.ptr,
							  "::", (yyvsp[0].sval).ptr),
					 NULL);
		}
#line 2513 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 101:
#line 851 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_path ((yyvsp[-4].op)->left.sval, (yyvsp[-1].params)); }
#line 2519 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 102:
#line 853 "rust-exp.y" /* yacc.c:1646  */
    {
		  (yyval.op) = parser->ast_path ((yyvsp[-4].op)->left.sval, (yyvsp[-1].params));
		  rust_push_back ('>');
		}
#line 2528 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 104:
#line 862 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->super_name ((yyvsp[0].op), 0); }
#line 2534 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 105:
#line 864 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->super_name ((yyvsp[0].op), (yyvsp[-1].depth)); }
#line 2540 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 106:
#line 866 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->crate_name ((yyvsp[0].op)); }
#line 2546 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 107:
#line 868 "rust-exp.y" /* yacc.c:1646  */
    {
		  /* This is a gdb extension to make it possible to
		     refer to items in other crates.  It just bypasses
		     adding the current crate to the front of the
		     name.  */
		  (yyval.op) = parser->ast_path (parser->concat3 ("::",
							  (yyvsp[0].op)->left.sval.ptr,
							  NULL),
					 (yyvsp[0].op)->right.params);
		}
#line 2561 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 108:
#line 882 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_path ((yyvsp[0].sval), NULL); }
#line 2567 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 109:
#line 884 "rust-exp.y" /* yacc.c:1646  */
    {
		  (yyval.op) = parser->ast_path (parser->concat3 ((yyvsp[-2].op)->left.sval.ptr,
							  "::", (yyvsp[0].sval).ptr),
					 NULL);
		}
#line 2577 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 111:
#line 894 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_path ((yyvsp[-3].op)->left.sval, (yyvsp[-1].params)); }
#line 2583 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 112:
#line 896 "rust-exp.y" /* yacc.c:1646  */
    {
		  (yyval.op) = parser->ast_path ((yyvsp[-3].op)->left.sval, (yyvsp[-1].params));
		  rust_push_back ('>');
		}
#line 2592 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 114:
#line 905 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_array_type ((yyvsp[-3].op), (yyvsp[-1].typed_val_int)); }
#line 2598 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 115:
#line 907 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_array_type ((yyvsp[-3].op), (yyvsp[-1].typed_val_int)); }
#line 2604 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 116:
#line 909 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_slice_type ((yyvsp[-1].op)); }
#line 2610 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 117:
#line 911 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_reference_type ((yyvsp[0].op)); }
#line 2616 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 118:
#line 913 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_pointer_type ((yyvsp[0].op), 1); }
#line 2622 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 119:
#line 915 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_pointer_type ((yyvsp[0].op), 0); }
#line 2628 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 120:
#line 917 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_function_type ((yyvsp[0].op), (yyvsp[-3].params)); }
#line 2634 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 121:
#line 919 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.op) = parser->ast_tuple_type ((yyvsp[-1].params)); }
#line 2640 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 122:
#line 924 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.params) = NULL; }
#line 2646 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 123:
#line 926 "rust-exp.y" /* yacc.c:1646  */
    { (yyval.params) = (yyvsp[0].params); }
#line 2652 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 124:
#line 931 "rust-exp.y" /* yacc.c:1646  */
    {
		  rust_op_vector *result = parser->new_op_vector ();
		  result->push_back ((yyvsp[0].op));
		  (yyval.params) = result;
		}
#line 2662 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 125:
#line 937 "rust-exp.y" /* yacc.c:1646  */
    {
		  (yyvsp[-2].params)->push_back ((yyvsp[0].op));
		  (yyval.params) = (yyvsp[-2].params);
		}
#line 2671 "rust-exp.c.tmp" /* yacc.c:1646  */
    break;


#line 2675 "rust-exp.c.tmp" /* yacc.c:1646  */
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
      yyerror (parser, YY_("syntax error"));
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
        yyerror (parser, yymsgp);
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
                      yytoken, &yylval, parser);
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
                  yystos[yystate], yyvsp, parser);
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
  yyerror (parser, YY_("memory exhausted"));
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
                  yytoken, &yylval, parser);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp, parser);
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
#line 943 "rust-exp.y" /* yacc.c:1906  */


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
  { "..=", DOTDOTEQ, OP_NULL },

  { "::", COLONCOLON, OP_NULL },
  { "..", DOTDOT, OP_NULL },
  { "->", ARROW, OP_NULL }
};

/* Helper function to copy to the name obstack.  */

const char *
rust_parser::copy_name (const char *name, int len)
{
  return (const char *) obstack_copy0 (&obstack, name, len);
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

struct stoken
rust_parser::concat3 (const char *s1, const char *s2, const char *s3)
{
  return make_stoken (obconcat (&obstack, s1, s2, s3, (char *) NULL));
}

/* Return an AST node referring to NAME, but relative to the crate's
   name.  */

const struct rust_op *
rust_parser::crate_name (const struct rust_op *name)
{
  std::string crate = rust_crate_for_block (expression_context_block);
  struct stoken result;

  gdb_assert (name->opcode == OP_VAR_VALUE);

  if (crate.empty ())
    error (_("Could not find crate for current location"));
  result = make_stoken (obconcat (&obstack, "::", crate.c_str (), "::",
				  name->left.sval.ptr, (char *) NULL));

  return ast_path (result, name->right.params);
}

/* Create an AST node referring to a "super::" qualified name.  IDENT
   is the base name and N_SUPERS is how many "super::"s were
   provided.  N_SUPERS can be zero.  */

const struct rust_op *
rust_parser::super_name (const struct rust_op *ident, unsigned int n_supers)
{
  const char *scope = block_scope (expression_context_block);
  int offset;

  gdb_assert (ident->opcode == OP_VAR_VALUE);

  if (scope[0] == '\0')
    error (_("Couldn't find namespace scope for self::"));

  if (n_supers > 0)
    {
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

  obstack_grow (&obstack, "::", 2);
  obstack_grow (&obstack, scope, offset);
  obstack_grow (&obstack, "::", 2);
  obstack_grow0 (&obstack, ident->left.sval.ptr, ident->left.sval.length);

  return ast_path (make_stoken ((const char *) obstack_finish (&obstack)),
		   ident->right.params);
}

/* A helper that updates the innermost block as appropriate.  */

static void
update_innermost_block (struct block_symbol sym)
{
  if (symbol_read_needs_frame (sym.symbol))
    innermost_block.update (sym);
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

int
rust_parser::lex_character (YYSTYPE *lvalp)
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

  lvalp->typed_val_int.val = value;
  lvalp->typed_val_int.type = get_type (is_byte ? "u8" : "char");

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

int
rust_parser::lex_string (YYSTYPE *lvalp)
{
  int is_byte = lexptr[0] == 'b';
  int raw_length;

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
	  obstack_1grow (&obstack, value);

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
	    obstack_1grow (&obstack, value);
	  else
	    convert_between_encodings ("UTF-32", "UTF-8", (gdb_byte *) &value,
				       sizeof (value), sizeof (value),
				       &obstack, translit_none);
	}
      else if (lexptr[0] == '\0')
	error (_("Unexpected EOF in string"));
      else
	{
	  value = lexptr[0] & 0xff;
	  if (is_byte && value > 127)
	    error (_("Non-ASCII value in byte string"));
	  obstack_1grow (&obstack, value);
	  ++lexptr;
	}
    }

  lvalp->sval.length = obstack_object_size (&obstack);
  lvalp->sval.ptr = (const char *) obstack_finish (&obstack);
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

int
rust_parser::lex_identifier (YYSTYPE *lvalp)
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
    lvalp->sval = make_stoken (copy_name (start, length));

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
lex_operator (YYSTYPE *lvalp)
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
      lvalp->opcode = token->opcode;
      return token->value;
    }

  return *lexptr++;
}

/* Lex a number.  */

int
rust_parser::lex_number (YYSTYPE *lvalp)
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
      const char *next = skip_spaces (&lexptr[subexps[0].rm_eo]);

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
  type = get_type (type_name);

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
	type = get_type ("i64");

      lvalp->typed_val_int.val = value;
      lvalp->typed_val_int.type = type;
    }
  else
    {
      lvalp->typed_val_float.type = type;
      bool parsed = parse_float (number.c_str (), number.length (),
				 lvalp->typed_val_float.type,
				 lvalp->typed_val_float.val);
      gdb_assert (parsed);
    }

  return is_integer ? (could_be_decimal ? DECIMAL_INTEGER : INTEGER) : FLOAT;
}

/* The lexer.  */

static int
rustyylex (YYSTYPE *lvalp, rust_parser *parser)
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
	  lvalp->sval = make_stoken ("");
	  return COMPLETE;
	}
      return 0;
    }

  if (lexptr[0] >= '0' && lexptr[0] <= '9')
    return parser->lex_number (lvalp);
  else if (lexptr[0] == 'b' && lexptr[1] == '\'')
    return parser->lex_character (lvalp);
  else if (lexptr[0] == 'b' && lexptr[1] == '"')
    return parser->lex_string (lvalp);
  else if (lexptr[0] == 'b' && starts_raw_string (lexptr + 1))
    return parser->lex_string (lvalp);
  else if (starts_raw_string (lexptr))
    return parser->lex_string (lvalp);
  else if (rust_identifier_start_p (lexptr[0]))
    return parser->lex_identifier (lvalp);
  else if (lexptr[0] == '"')
    return parser->lex_string (lvalp);
  else if (lexptr[0] == '\'')
    return parser->lex_character (lvalp);
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

  return lex_operator (lvalp);
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

const struct rust_op *
rust_parser::ast_operation (enum exp_opcode opcode, const struct rust_op *left,
			    const struct rust_op *right)
{
  struct rust_op *result = OBSTACK_ZALLOC (&obstack, struct rust_op);

  result->opcode = opcode;
  result->left.op = left;
  result->right.op = right;

  return result;
}

/* Make a compound assignment operation.  */

const struct rust_op *
rust_parser::ast_compound_assignment (enum exp_opcode opcode,
				      const struct rust_op *left,
				      const struct rust_op *right)
{
  struct rust_op *result = OBSTACK_ZALLOC (&obstack, struct rust_op);

  result->opcode = opcode;
  result->compound_assignment = 1;
  result->left.op = left;
  result->right.op = right;

  return result;
}

/* Make a typed integer literal operation.  */

const struct rust_op *
rust_parser::ast_literal (struct typed_val_int val)
{
  struct rust_op *result = OBSTACK_ZALLOC (&obstack, struct rust_op);

  result->opcode = OP_LONG;
  result->left.typed_val_int = val;

  return result;
}

/* Make a typed floating point literal operation.  */

const struct rust_op *
rust_parser::ast_dliteral (struct typed_val_float val)
{
  struct rust_op *result = OBSTACK_ZALLOC (&obstack, struct rust_op);

  result->opcode = OP_FLOAT;
  result->left.typed_val_float = val;

  return result;
}

/* Make a unary operation.  */

const struct rust_op *
rust_parser::ast_unary (enum exp_opcode opcode, const struct rust_op *expr)
{
  return ast_operation (opcode, expr, NULL);
}

/* Make a cast operation.  */

const struct rust_op *
rust_parser::ast_cast (const struct rust_op *expr, const struct rust_op *type)
{
  struct rust_op *result = OBSTACK_ZALLOC (&obstack, struct rust_op);

  result->opcode = UNOP_CAST;
  result->left.op = expr;
  result->right.op = type;

  return result;
}

/* Make a call-like operation.  This is nominally a function call, but
   when lowering we may discover that it actually represents the
   creation of a tuple struct.  */

const struct rust_op *
rust_parser::ast_call_ish (enum exp_opcode opcode, const struct rust_op *expr,
			   rust_op_vector *params)
{
  struct rust_op *result = OBSTACK_ZALLOC (&obstack, struct rust_op);

  result->opcode = opcode;
  result->left.op = expr;
  result->right.params = params;

  return result;
}

/* Make a structure creation operation.  */

const struct rust_op *
rust_parser::ast_struct (const struct rust_op *name, rust_set_vector *fields)
{
  struct rust_op *result = OBSTACK_ZALLOC (&obstack, struct rust_op);

  result->opcode = OP_AGGREGATE;
  result->left.op = name;
  result->right.field_inits = fields;

  return result;
}

/* Make an identifier path.  */

const struct rust_op *
rust_parser::ast_path (struct stoken path, rust_op_vector *params)
{
  struct rust_op *result = OBSTACK_ZALLOC (&obstack, struct rust_op);

  result->opcode = OP_VAR_VALUE;
  result->left.sval = path;
  result->right.params = params;

  return result;
}

/* Make a string constant operation.  */

const struct rust_op *
rust_parser::ast_string (struct stoken str)
{
  struct rust_op *result = OBSTACK_ZALLOC (&obstack, struct rust_op);

  result->opcode = OP_STRING;
  result->left.sval = str;

  return result;
}

/* Make a field expression.  */

const struct rust_op *
rust_parser::ast_structop (const struct rust_op *left, const char *name,
			   int completing)
{
  struct rust_op *result = OBSTACK_ZALLOC (&obstack, struct rust_op);

  result->opcode = STRUCTOP_STRUCT;
  result->completing = completing;
  result->left.op = left;
  result->right.sval = make_stoken (name);

  return result;
}

/* Make an anonymous struct operation, like 'x.0'.  */

const struct rust_op *
rust_parser::ast_structop_anonymous (const struct rust_op *left,
				     struct typed_val_int number)
{
  struct rust_op *result = OBSTACK_ZALLOC (&obstack, struct rust_op);

  result->opcode = STRUCTOP_ANONYMOUS;
  result->left.op = left;
  result->right.typed_val_int = number;

  return result;
}

/* Make a range operation.  */

const struct rust_op *
rust_parser::ast_range (const struct rust_op *lhs, const struct rust_op *rhs,
			bool inclusive)
{
  struct rust_op *result = OBSTACK_ZALLOC (&obstack, struct rust_op);

  result->opcode = OP_RANGE;
  result->inclusive = inclusive;
  result->left.op = lhs;
  result->right.op = rhs;

  return result;
}

/* A helper function to make a type-related AST node.  */

struct rust_op *
rust_parser::ast_basic_type (enum type_code typecode)
{
  struct rust_op *result = OBSTACK_ZALLOC (&obstack, struct rust_op);

  result->opcode = OP_TYPE;
  result->typecode = typecode;
  return result;
}

/* Create an AST node describing an array type.  */

const struct rust_op *
rust_parser::ast_array_type (const struct rust_op *lhs,
			     struct typed_val_int val)
{
  struct rust_op *result = ast_basic_type (TYPE_CODE_ARRAY);

  result->left.op = lhs;
  result->right.typed_val_int = val;
  return result;
}

/* Create an AST node describing a reference type.  */

const struct rust_op *
rust_parser::ast_slice_type (const struct rust_op *type)
{
  /* Use TYPE_CODE_COMPLEX just because it is handy.  */
  struct rust_op *result = ast_basic_type (TYPE_CODE_COMPLEX);

  result->left.op = type;
  return result;
}

/* Create an AST node describing a reference type.  */

const struct rust_op *
rust_parser::ast_reference_type (const struct rust_op *type)
{
  struct rust_op *result = ast_basic_type (TYPE_CODE_REF);

  result->left.op = type;
  return result;
}

/* Create an AST node describing a pointer type.  */

const struct rust_op *
rust_parser::ast_pointer_type (const struct rust_op *type, int is_mut)
{
  struct rust_op *result = ast_basic_type (TYPE_CODE_PTR);

  result->left.op = type;
  /* For the time being we ignore is_mut.  */
  return result;
}

/* Create an AST node describing a function type.  */

const struct rust_op *
rust_parser::ast_function_type (const struct rust_op *rtype,
				rust_op_vector *params)
{
  struct rust_op *result = ast_basic_type (TYPE_CODE_FUNC);

  result->left.op = rtype;
  result->right.params = params;
  return result;
}

/* Create an AST node describing a tuple type.  */

const struct rust_op *
rust_parser::ast_tuple_type (rust_op_vector *params)
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

struct type *
rust_parser::rust_lookup_type (const char *name, const struct block *block)
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

  type = lookup_typename (language (), arch (), name, NULL, 1);
  if (type != NULL)
    return type;

  /* Last chance, try a built-in type.  */
  return language_lookup_primitive_type (language (), arch (), name);
}

/* Convert a vector of rust_ops representing types to a vector of
   types.  */

std::vector<struct type *>
rust_parser::convert_params_to_types (rust_op_vector *params)
{
  std::vector<struct type *> result;

  if (params != nullptr)
    {
      for (const rust_op *op : *params)
        result.push_back (convert_ast_to_type (op));
    }

  return result;
}

/* Convert a rust_op representing a type to a struct type *.  */

struct type *
rust_parser::convert_ast_to_type (const struct rust_op *operation)
{
  struct type *type, *result = NULL;

  if (operation->opcode == OP_VAR_VALUE)
    {
      const char *varname = convert_name (operation);

      result = rust_lookup_type (varname, expression_context_block);
      if (result == NULL)
	error (_("No typed name '%s' in current context"), varname);
      return result;
    }

  gdb_assert (operation->opcode == OP_TYPE);

  switch (operation->typecode)
    {
    case TYPE_CODE_ARRAY:
      type = convert_ast_to_type (operation->left.op);
      if (operation->right.typed_val_int.val < 0)
	error (_("Negative array length"));
      result = lookup_array_range_type (type, 0,
					operation->right.typed_val_int.val - 1);
      break;

    case TYPE_CODE_COMPLEX:
      {
	struct type *usize = get_type ("usize");

	type = convert_ast_to_type (operation->left.op);
	result = rust_slice_type ("&[*gdb*]", type, usize);
      }
      break;

    case TYPE_CODE_REF:
    case TYPE_CODE_PTR:
      /* For now we treat &x and *x identically.  */
      type = convert_ast_to_type (operation->left.op);
      result = lookup_pointer_type (type);
      break;

    case TYPE_CODE_FUNC:
      {
	std::vector<struct type *> args
	  (convert_params_to_types (operation->right.params));
	struct type **argtypes = NULL;

	type = convert_ast_to_type (operation->left.op);
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
	  (convert_params_to_types (operation->left.params));
	int i;
	const char *name;

	obstack_1grow (&obstack, '(');
	for (i = 0; i < args.size (); ++i)
	  {
	    std::string type_name = type_to_string (args[i]);

	    if (i > 0)
	      obstack_1grow (&obstack, ',');
	    obstack_grow_str (&obstack, type_name.c_str ());
	  }

	obstack_grow_str0 (&obstack, ")");
	name = (const char *) obstack_finish (&obstack);

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

const char *
rust_parser::convert_name (const struct rust_op *operation)
{
  int i;

  gdb_assert (operation->opcode == OP_VAR_VALUE);

  if (operation->right.params == NULL)
    return operation->left.sval.ptr;

  std::vector<struct type *> types
    (convert_params_to_types (operation->right.params));

  obstack_grow_str (&obstack, operation->left.sval.ptr);
  obstack_1grow (&obstack, '<');
  for (i = 0; i < types.size (); ++i)
    {
      std::string type_name = type_to_string (types[i]);

      if (i > 0)
	obstack_1grow (&obstack, ',');

      obstack_grow_str (&obstack, type_name.c_str ());
    }
  obstack_grow_str0 (&obstack, ">");

  return (const char *) obstack_finish (&obstack);
}

/* A helper function that converts a vec of rust_ops to a gdb
   expression.  */

void
rust_parser::convert_params_to_expression (rust_op_vector *params,
					   const struct rust_op *top)
{
  for (const rust_op *elem : *params)
    convert_ast_to_expression (elem, top);
}

/* Lower a rust_op to a gdb expression.  STATE is the parser state.
   OPERATION is the operation to lower.  TOP is a pointer to the
   top-most operation; it is used to handle the special case where the
   top-most expression is an identifier and can be optionally lowered
   to OP_TYPE.  WANT_TYPE is a flag indicating that, if the expression
   is the name of a type, then emit an OP_TYPE for it (rather than
   erroring).  If WANT_TYPE is set, then the similar TOP handling is
   not done.  */

void
rust_parser::convert_ast_to_expression (const struct rust_op *operation,
					const struct rust_op *top,
					bool want_type)
{
  switch (operation->opcode)
    {
    case OP_LONG:
      write_exp_elt_opcode (pstate, OP_LONG);
      write_exp_elt_type (pstate, operation->left.typed_val_int.type);
      write_exp_elt_longcst (pstate, operation->left.typed_val_int.val);
      write_exp_elt_opcode (pstate, OP_LONG);
      break;

    case OP_FLOAT:
      write_exp_elt_opcode (pstate, OP_FLOAT);
      write_exp_elt_type (pstate, operation->left.typed_val_float.type);
      write_exp_elt_floatcst (pstate, operation->left.typed_val_float.val);
      write_exp_elt_opcode (pstate, OP_FLOAT);
      break;

    case STRUCTOP_STRUCT:
      {
	convert_ast_to_expression (operation->left.op, top);

	if (operation->completing)
	  mark_struct_expression (pstate);
	write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
	write_exp_string (pstate, operation->right.sval);
	write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
      }
      break;

    case STRUCTOP_ANONYMOUS:
      {
	convert_ast_to_expression (operation->left.op, top);

	write_exp_elt_opcode (pstate, STRUCTOP_ANONYMOUS);
	write_exp_elt_longcst (pstate, operation->right.typed_val_int.val);
	write_exp_elt_opcode (pstate, STRUCTOP_ANONYMOUS);
      }
      break;

    case UNOP_SIZEOF:
      convert_ast_to_expression (operation->left.op, top, true);
      write_exp_elt_opcode (pstate, UNOP_SIZEOF);
      break;

    case UNOP_PLUS:
    case UNOP_NEG:
    case UNOP_COMPLEMENT:
    case UNOP_IND:
    case UNOP_ADDR:
      convert_ast_to_expression (operation->left.op, top);
      write_exp_elt_opcode (pstate, operation->opcode);
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
      convert_ast_to_expression (operation->left.op, top);
      convert_ast_to_expression (operation->right.op, top);
      if (operation->compound_assignment)
	{
	  write_exp_elt_opcode (pstate, BINOP_ASSIGN_MODIFY);
	  write_exp_elt_opcode (pstate, operation->opcode);
	  write_exp_elt_opcode (pstate, BINOP_ASSIGN_MODIFY);
	}
      else
	write_exp_elt_opcode (pstate, operation->opcode);

      if (operation->compound_assignment
	  || operation->opcode == BINOP_ASSIGN)
	{
	  struct type *type;

	  type = language_lookup_primitive_type (parse_language (pstate),
						 parse_gdbarch (pstate),
						 "()");

	  write_exp_elt_opcode (pstate, OP_LONG);
	  write_exp_elt_type (pstate, type);
	  write_exp_elt_longcst (pstate, 0);
	  write_exp_elt_opcode (pstate, OP_LONG);

	  write_exp_elt_opcode (pstate, BINOP_COMMA);
	}
      break;

    case UNOP_CAST:
      {
	struct type *type = convert_ast_to_type (operation->right.op);

	convert_ast_to_expression (operation->left.op, top);
	write_exp_elt_opcode (pstate, UNOP_CAST);
	write_exp_elt_type (pstate, type);
	write_exp_elt_opcode (pstate, UNOP_CAST);
      }
      break;

    case OP_FUNCALL:
      {
	if (operation->left.op->opcode == OP_VAR_VALUE)
	  {
	    struct type *type;
	    const char *varname = convert_name (operation->left.op);

	    type = rust_lookup_type (varname, expression_context_block);
	    if (type != NULL)
	      {
		/* This is actually a tuple struct expression, not a
		   call expression.  */
		rust_op_vector *params = operation->right.params;

		if (TYPE_CODE (type) != TYPE_CODE_NAMESPACE)
		  {
		    if (!rust_tuple_struct_type_p (type))
		      error (_("Type %s is not a tuple struct"), varname);

		    for (int i = 0; i < params->size (); ++i)
		      {
			char *cell = get_print_cell ();

			xsnprintf (cell, PRINT_CELL_SIZE, "__%d", i);
			write_exp_elt_opcode (pstate, OP_NAME);
			write_exp_string (pstate, make_stoken (cell));
			write_exp_elt_opcode (pstate, OP_NAME);

			convert_ast_to_expression ((*params)[i], top);
		      }

		    write_exp_elt_opcode (pstate, OP_AGGREGATE);
		    write_exp_elt_type (pstate, type);
		    write_exp_elt_longcst (pstate, 2 * params->size ());
		    write_exp_elt_opcode (pstate, OP_AGGREGATE);
		    break;
		  }
	      }
	  }
	convert_ast_to_expression (operation->left.op, top);
	convert_params_to_expression (operation->right.params, top);
	write_exp_elt_opcode (pstate, OP_FUNCALL);
	write_exp_elt_longcst (pstate, operation->right.params->size ());
	write_exp_elt_longcst (pstate, OP_FUNCALL);
      }
      break;

    case OP_ARRAY:
      gdb_assert (operation->left.op == NULL);
      convert_params_to_expression (operation->right.params, top);
      write_exp_elt_opcode (pstate, OP_ARRAY);
      write_exp_elt_longcst (pstate, 0);
      write_exp_elt_longcst (pstate, operation->right.params->size () - 1);
      write_exp_elt_longcst (pstate, OP_ARRAY);
      break;

    case OP_VAR_VALUE:
      {
	struct block_symbol sym;
	const char *varname;

	if (operation->left.sval.ptr[0] == '$')
	  {
	    write_dollar_variable (pstate, operation->left.sval);
	    break;
	  }

	varname = convert_name (operation);
	sym = rust_lookup_symbol (varname, expression_context_block,
				  VAR_DOMAIN);
	if (sym.symbol != NULL && SYMBOL_CLASS (sym.symbol) != LOC_TYPEDEF)
	  {
	    write_exp_elt_opcode (pstate, OP_VAR_VALUE);
	    write_exp_elt_block (pstate, sym.block);
	    write_exp_elt_sym (pstate, sym.symbol);
	    write_exp_elt_opcode (pstate, OP_VAR_VALUE);
	  }
	else
	  {
	    struct type *type = NULL;

	    if (sym.symbol != NULL)
	      {
		gdb_assert (SYMBOL_CLASS (sym.symbol) == LOC_TYPEDEF);
		type = SYMBOL_TYPE (sym.symbol);
	      }
	    if (type == NULL)
	      type = rust_lookup_type (varname, expression_context_block);
	    if (type == NULL)
	      error (_("No symbol '%s' in current context"), varname);

	    if (!want_type
		&& TYPE_CODE (type) == TYPE_CODE_STRUCT
		&& TYPE_NFIELDS (type) == 0)
	      {
		/* A unit-like struct.  */
		write_exp_elt_opcode (pstate, OP_AGGREGATE);
		write_exp_elt_type (pstate, type);
		write_exp_elt_longcst (pstate, 0);
		write_exp_elt_opcode (pstate, OP_AGGREGATE);
	      }
	    else if (want_type || operation == top)
	      {
		write_exp_elt_opcode (pstate, OP_TYPE);
		write_exp_elt_type (pstate, type);
		write_exp_elt_opcode (pstate, OP_TYPE);
	      }
	    else
	      error (_("Found type '%s', which can't be "
		       "evaluated in this context"),
		     varname);
	  }
      }
      break;

    case OP_AGGREGATE:
      {
	int length;
	rust_set_vector *fields = operation->right.field_inits;
	struct type *type;
	const char *name;

	length = 0;
	for (const set_field &init : *fields)
	  {
	    if (init.name.ptr != NULL)
	      {
		write_exp_elt_opcode (pstate, OP_NAME);
		write_exp_string (pstate, init.name);
		write_exp_elt_opcode (pstate, OP_NAME);
		++length;
	      }

	    convert_ast_to_expression (init.init, top);
	    ++length;

	    if (init.name.ptr == NULL)
	      {
		/* This is handled differently from Ada in our
		   evaluator.  */
		write_exp_elt_opcode (pstate, OP_OTHERS);
	      }
	  }

	name = convert_name (operation->left.op);
	type = rust_lookup_type (name, expression_context_block);
	if (type == NULL)
	  error (_("Could not find type '%s'"), operation->left.sval.ptr);

	if (TYPE_CODE (type) != TYPE_CODE_STRUCT
	    || rust_tuple_type_p (type)
	    || rust_tuple_struct_type_p (type))
	  error (_("Struct expression applied to non-struct type"));

	write_exp_elt_opcode (pstate, OP_AGGREGATE);
	write_exp_elt_type (pstate, type);
	write_exp_elt_longcst (pstate, length);
	write_exp_elt_opcode (pstate, OP_AGGREGATE);
      }
      break;

    case OP_STRING:
      {
	write_exp_elt_opcode (pstate, OP_STRING);
	write_exp_string (pstate, operation->left.sval);
	write_exp_elt_opcode (pstate, OP_STRING);
      }
      break;

    case OP_RANGE:
      {
	enum range_type kind = BOTH_BOUND_DEFAULT;

	if (operation->left.op != NULL)
	  {
	    convert_ast_to_expression (operation->left.op, top);
	    kind = HIGH_BOUND_DEFAULT;
	  }
	if (operation->right.op != NULL)
	  {
	    convert_ast_to_expression (operation->right.op, top);
	    if (kind == BOTH_BOUND_DEFAULT)
	      kind = (operation->inclusive
		      ? LOW_BOUND_DEFAULT : LOW_BOUND_DEFAULT_EXCLUSIVE);
	    else
	      {
		gdb_assert (kind == HIGH_BOUND_DEFAULT);
		kind = (operation->inclusive
			? NONE_BOUND_DEFAULT : NONE_BOUND_DEFAULT_EXCLUSIVE);
	      }
	  }
	else
	  {
	    /* Nothing should make an inclusive range without an upper
	       bound.  */
	    gdb_assert (!operation->inclusive);
	  }

	write_exp_elt_opcode (pstate, OP_RANGE);
	write_exp_elt_longcst (pstate, kind);
	write_exp_elt_opcode (pstate, OP_RANGE);
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

  /* This sets various globals and also clears them on
     destruction.  */
  rust_parser parser (state);

  result = rustyyparse (&parser);

  if (!result || (parse_completion && parser.rust_ast != NULL))
    parser.convert_ast_to_expression (parser.rust_ast, parser.rust_ast);

  return result;
}

/* The parser error handler.  */

static void
rustyyerror (rust_parser *parser, const char *msg)
{
  const char *where = prev_lexptr ? prev_lexptr : lexptr;
  error (_("%s in expression, near `%s'."), msg, where);
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
rust_lex_test_one (rust_parser *parser, const char *input, int expected)
{
  int token;
  RUSTSTYPE result;

  rust_lex_test_init (input);

  token = rustyylex (&result, parser);
  SELF_CHECK (token == expected);

  if (token)
    {
      RUSTSTYPE ignore;
      token = rustyylex (&ignore, parser);
      SELF_CHECK (token == 0);
    }

  return result;
}

/* Test that INPUT lexes as the integer VALUE.  */

static void
rust_lex_int_test (rust_parser *parser, const char *input, int value, int kind)
{
  RUSTSTYPE result = rust_lex_test_one (parser, input, kind);
  SELF_CHECK (result.typed_val_int.val == value);
}

/* Test that INPUT throws an exception with text ERR.  */

static void
rust_lex_exception_test (rust_parser *parser, const char *input,
			 const char *err)
{
  TRY
    {
      /* The "kind" doesn't matter.  */
      rust_lex_test_one (parser, input, DECIMAL_INTEGER);
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
rust_lex_stringish_test (rust_parser *parser, const char *input,
			 const char *value, int kind)
{
  RUSTSTYPE result = rust_lex_test_one (parser, input, kind);
  SELF_CHECK (result.sval.length == strlen (value));
  SELF_CHECK (strncmp (result.sval.ptr, value, result.sval.length) == 0);
}

/* Helper to test that a string parses as a given token sequence.  */

static void
rust_lex_test_sequence (rust_parser *parser, const char *input, int len,
			const int expected[])
{
  int i;

  lexptr = input;
  paren_depth = 0;

  for (i = 0; i < len; ++i)
    {
      RUSTSTYPE ignore;
      int token = rustyylex (&ignore, parser);

      SELF_CHECK (token == expected[i]);
    }
}

/* Tests for an integer-parsing corner case.  */

static void
rust_lex_test_trailing_dot (rust_parser *parser)
{
  const int expected1[] = { DECIMAL_INTEGER, '.', IDENT, '(', ')', 0 };
  const int expected2[] = { INTEGER, '.', IDENT, '(', ')', 0 };
  const int expected3[] = { FLOAT, EQEQ, '(', ')', 0 };
  const int expected4[] = { DECIMAL_INTEGER, DOTDOT, DECIMAL_INTEGER, 0 };

  rust_lex_test_sequence (parser, "23.g()", ARRAY_SIZE (expected1), expected1);
  rust_lex_test_sequence (parser, "23_0.g()", ARRAY_SIZE (expected2),
			  expected2);
  rust_lex_test_sequence (parser, "23.==()", ARRAY_SIZE (expected3),
			  expected3);
  rust_lex_test_sequence (parser, "23..25", ARRAY_SIZE (expected4), expected4);
}

/* Tests of completion.  */

static void
rust_lex_test_completion (rust_parser *parser)
{
  const int expected[] = { IDENT, '.', COMPLETE, 0 };

  parse_completion = 1;

  rust_lex_test_sequence (parser, "something.wha", ARRAY_SIZE (expected),
			  expected);
  rust_lex_test_sequence (parser, "something.", ARRAY_SIZE (expected),
			  expected);

  parse_completion = 0;
}

/* Test pushback.  */

static void
rust_lex_test_push_back (rust_parser *parser)
{
  int token;
  RUSTSTYPE lval;

  rust_lex_test_init (">>=");

  token = rustyylex (&lval, parser);
  SELF_CHECK (token == COMPOUND_ASSIGN);
  SELF_CHECK (lval.opcode == BINOP_RSH);

  rust_push_back ('=');

  token = rustyylex (&lval, parser);
  SELF_CHECK (token == '=');

  token = rustyylex (&lval, parser);
  SELF_CHECK (token == 0);
}

/* Unit test the lexer.  */

static void
rust_lex_tests (void)
{
  int i;

  // Set up dummy "parser", so that rust_type works.
  struct parser_state ps (0, &rust_language_defn, target_gdbarch ());
  rust_parser parser (&ps);

  rust_lex_test_one (&parser, "", 0);
  rust_lex_test_one (&parser, "    \t  \n \r  ", 0);
  rust_lex_test_one (&parser, "thread 23", 0);
  rust_lex_test_one (&parser, "task 23", 0);
  rust_lex_test_one (&parser, "th 104", 0);
  rust_lex_test_one (&parser, "ta 97", 0);

  rust_lex_int_test (&parser, "'z'", 'z', INTEGER);
  rust_lex_int_test (&parser, "'\\xff'", 0xff, INTEGER);
  rust_lex_int_test (&parser, "'\\u{1016f}'", 0x1016f, INTEGER);
  rust_lex_int_test (&parser, "b'z'", 'z', INTEGER);
  rust_lex_int_test (&parser, "b'\\xfe'", 0xfe, INTEGER);
  rust_lex_int_test (&parser, "b'\\xFE'", 0xfe, INTEGER);
  rust_lex_int_test (&parser, "b'\\xfE'", 0xfe, INTEGER);

  /* Test all escapes in both modes.  */
  rust_lex_int_test (&parser, "'\\n'", '\n', INTEGER);
  rust_lex_int_test (&parser, "'\\r'", '\r', INTEGER);
  rust_lex_int_test (&parser, "'\\t'", '\t', INTEGER);
  rust_lex_int_test (&parser, "'\\\\'", '\\', INTEGER);
  rust_lex_int_test (&parser, "'\\0'", '\0', INTEGER);
  rust_lex_int_test (&parser, "'\\''", '\'', INTEGER);
  rust_lex_int_test (&parser, "'\\\"'", '"', INTEGER);

  rust_lex_int_test (&parser, "b'\\n'", '\n', INTEGER);
  rust_lex_int_test (&parser, "b'\\r'", '\r', INTEGER);
  rust_lex_int_test (&parser, "b'\\t'", '\t', INTEGER);
  rust_lex_int_test (&parser, "b'\\\\'", '\\', INTEGER);
  rust_lex_int_test (&parser, "b'\\0'", '\0', INTEGER);
  rust_lex_int_test (&parser, "b'\\''", '\'', INTEGER);
  rust_lex_int_test (&parser, "b'\\\"'", '"', INTEGER);

  rust_lex_exception_test (&parser, "'z", "Unterminated character literal");
  rust_lex_exception_test (&parser, "b'\\x0'", "Not enough hex digits seen");
  rust_lex_exception_test (&parser, "b'\\u{0}'",
			   "Unicode escape in byte literal");
  rust_lex_exception_test (&parser, "'\\x0'", "Not enough hex digits seen");
  rust_lex_exception_test (&parser, "'\\u0'", "Missing '{' in Unicode escape");
  rust_lex_exception_test (&parser, "'\\u{0", "Missing '}' in Unicode escape");
  rust_lex_exception_test (&parser, "'\\u{0000007}", "Overlong hex escape");
  rust_lex_exception_test (&parser, "'\\u{}", "Not enough hex digits seen");
  rust_lex_exception_test (&parser, "'\\Q'", "Invalid escape \\Q in literal");
  rust_lex_exception_test (&parser, "b'\\Q'", "Invalid escape \\Q in literal");

  rust_lex_int_test (&parser, "23", 23, DECIMAL_INTEGER);
  rust_lex_int_test (&parser, "2_344__29", 234429, INTEGER);
  rust_lex_int_test (&parser, "0x1f", 0x1f, INTEGER);
  rust_lex_int_test (&parser, "23usize", 23, INTEGER);
  rust_lex_int_test (&parser, "23i32", 23, INTEGER);
  rust_lex_int_test (&parser, "0x1_f", 0x1f, INTEGER);
  rust_lex_int_test (&parser, "0b1_101011__", 0x6b, INTEGER);
  rust_lex_int_test (&parser, "0o001177i64", 639, INTEGER);

  rust_lex_test_trailing_dot (&parser);

  rust_lex_test_one (&parser, "23.", FLOAT);
  rust_lex_test_one (&parser, "23.99f32", FLOAT);
  rust_lex_test_one (&parser, "23e7", FLOAT);
  rust_lex_test_one (&parser, "23E-7", FLOAT);
  rust_lex_test_one (&parser, "23e+7", FLOAT);
  rust_lex_test_one (&parser, "23.99e+7f64", FLOAT);
  rust_lex_test_one (&parser, "23.82f32", FLOAT);

  rust_lex_stringish_test (&parser, "hibob", "hibob", IDENT);
  rust_lex_stringish_test (&parser, "hibob__93", "hibob__93", IDENT);
  rust_lex_stringish_test (&parser, "thread", "thread", IDENT);

  rust_lex_stringish_test (&parser, "\"string\"", "string", STRING);
  rust_lex_stringish_test (&parser, "\"str\\ting\"", "str\ting", STRING);
  rust_lex_stringish_test (&parser, "\"str\\\"ing\"", "str\"ing", STRING);
  rust_lex_stringish_test (&parser, "r\"str\\ing\"", "str\\ing", STRING);
  rust_lex_stringish_test (&parser, "r#\"str\\ting\"#", "str\\ting", STRING);
  rust_lex_stringish_test (&parser, "r###\"str\\\"ing\"###", "str\\\"ing",
			   STRING);

  rust_lex_stringish_test (&parser, "b\"string\"", "string", BYTESTRING);
  rust_lex_stringish_test (&parser, "b\"\x73tring\"", "string", BYTESTRING);
  rust_lex_stringish_test (&parser, "b\"str\\\"ing\"", "str\"ing", BYTESTRING);
  rust_lex_stringish_test (&parser, "br####\"\\x73tring\"####", "\\x73tring",
			   BYTESTRING);

  for (i = 0; i < ARRAY_SIZE (identifier_tokens); ++i)
    rust_lex_test_one (&parser, identifier_tokens[i].name,
		       identifier_tokens[i].value);

  for (i = 0; i < ARRAY_SIZE (operator_tokens); ++i)
    rust_lex_test_one (&parser, operator_tokens[i].name,
		       operator_tokens[i].value);

  rust_lex_test_completion (&parser);
  rust_lex_test_push_back (&parser);
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
  selftests::register_test ("rust-lex", rust_lex_tests);
#endif
}
