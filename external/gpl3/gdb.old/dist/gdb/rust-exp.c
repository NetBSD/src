/* original parser id follows */
/* yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93" */
/* (use YYMAJOR/YYMINOR for ifdefs dependent on parser version) */

#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9

#define YYEMPTY        (-1)
#define yyclearin      (yychar = YYEMPTY)
#define yyerrok        (yyerrflag = 0)
#define YYRECOVERING() (yyerrflag != 0)
#define YYENOMEM       (-2)
#define YYEOF          0
#undef YYBTYACC
#define YYBTYACC 0
#define YYDEBUGSTR YYPREFIX "debug"
#define YYPREFIX "yy"

#define YYPURE 0

#line 23 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"

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

#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#line 196 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
typedef union
{
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
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
#line 229 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"

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

#line 262 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.c"

/* compatibility with bison */
#ifdef YYPARSE_PARAM
/* compatibility with FreeBSD */
# ifdef YYPARSE_PARAM_TYPE
#  define YYPARSE_DECL() yyparse(YYPARSE_PARAM_TYPE YYPARSE_PARAM)
# else
#  define YYPARSE_DECL() yyparse(void *YYPARSE_PARAM)
# endif
#else
# define YYPARSE_DECL() yyparse(void)
#endif

/* Parameters sent to lex. */
#ifdef YYLEX_PARAM
# define YYLEX_DECL() yylex(void *YYLEX_PARAM)
# define YYLEX yylex(YYLEX_PARAM)
#else
# define YYLEX_DECL() yylex(void)
# define YYLEX yylex()
#endif

#if !(defined(yylex) || defined(YYSTATE))
int YYLEX_DECL();
#endif

/* Parameters sent to yyerror. */
#ifndef YYERROR_DECL
#define YYERROR_DECL() yyerror(const char *s)
#endif
#ifndef YYERROR_CALL
#define YYERROR_CALL(msg) yyerror(msg)
#endif

extern int YYPARSE_DECL();

#define GDBVAR 257
#define IDENT 258
#define COMPLETE 259
#define INTEGER 260
#define DECIMAL_INTEGER 261
#define STRING 262
#define BYTESTRING 263
#define FLOAT 264
#define COMPOUND_ASSIGN 265
#define KW_AS 266
#define KW_IF 267
#define KW_TRUE 268
#define KW_FALSE 269
#define KW_SUPER 270
#define KW_SELF 271
#define KW_MUT 272
#define KW_EXTERN 273
#define KW_CONST 274
#define KW_FN 275
#define KW_SIZEOF 276
#define DOTDOT 277
#define OROR 278
#define ANDAND 279
#define EQEQ 280
#define NOTEQ 281
#define LTEQ 282
#define GTEQ 283
#define LSH 284
#define RSH 285
#define COLONCOLON 286
#define ARROW 287
#define UNARY 288
#define YYERRCODE 256
typedef int YYINT;
static const YYINT yylhs[] = {                           -1,
    0,   11,   11,   11,   11,   11,   11,   11,   11,   11,
   11,   11,   11,   11,   23,   24,   25,   32,   32,   31,
   31,   31,   26,   26,   26,   26,   27,   27,   27,   27,
   10,   10,   10,   10,   10,   10,   10,   12,   12,   12,
   13,   14,   14,   14,   14,   14,   14,   14,   15,   15,
   15,   15,   16,   16,   16,   16,   16,   16,   16,   16,
   16,   16,   16,   16,   16,   16,   16,   16,   16,   16,
   16,   17,   18,   19,   20,   28,   28,   29,   29,   30,
   21,   33,   33,    9,    9,   22,   22,   22,    2,    2,
    2,    2,    2,    3,    3,    3,    3,    4,    4,    4,
    4,    4,    6,    6,    5,    5,    5,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    7,    7,    8,    8,
};
static const YYINT yylen[] = {                            2,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    5,    2,    4,    2,    3,    0,
    1,    5,    4,    3,    6,    5,    2,    3,    2,    1,
    1,    1,    1,    1,    1,    1,    1,    3,    3,    3,
    4,    2,    2,    2,    2,    2,    3,    4,    1,    1,
    1,    1,    3,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    1,    3,    0,    1,    3,
    2,    0,    2,    2,    3,    1,    1,    1,    1,    3,
    3,    2,    2,    1,    3,    5,    5,    1,    3,    3,
    2,    2,    1,    3,    1,    4,    4,    1,    5,    5,
    4,    2,    3,    3,    6,    3,    0,    1,    1,    3,
};
static const YYINT yydefred[] = {                         0,
   87,   94,   31,   32,   34,   35,   33,   36,   37,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    2,    0,    7,    9,   11,   12,
   49,   50,   51,   52,   13,   14,    3,    4,    5,    6,
    8,   10,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   16,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   81,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   24,    0,   75,
    0,    0,    0,   21,   95,    0,    0,  103,    0,    0,
    0,    0,    0,    0,    0,    0,   72,  108,   98,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   38,   39,   40,    0,    0,    0,   84,
    0,    0,   48,    0,   23,    0,    0,    0,    0,    0,
   17,  119,    0,    0,  102,    0,  101,    0,  112,    0,
    0,    0,    0,    0,    0,    0,    0,   41,   80,   85,
    0,   26,   15,    0,   97,   96,    0,   99,    0,    0,
  113,  114,    0,  116,  104,    0,  100,   25,    0,  120,
    0,  111,    0,    0,  107,  106,   22,    0,  109,  110,
  115,
};
#if defined(YYDESTRUCT_CALL) || defined(YYSTYPE_TOSTRING)
static const YYINT yystos[] = {                           0,
  257,  258,  260,  261,  262,  263,  264,  268,  269,  271,
  273,  276,  277,  286,   38,   43,   45,   42,   91,   40,
   33,  290,  292,  293,  300,  301,  302,  303,  304,  305,
  306,  307,  308,  309,  310,  311,  312,  313,  314,  315,
  316,  317,  323,  286,  293,   40,  301,  293,  272,  301,
  301,  301,  301,  272,  301,  318,   41,  301,  301,  123,
  286,  265,  266,  277,  278,  279,  280,  281,  282,  283,
  284,  285,   61,   60,   62,  124,   94,   38,   64,   43,
   45,   42,   47,   37,   91,   46,   40,  320,  270,  299,
  293,  301,  301,  301,  318,   59,   44,   93,   44,   41,
  258,  277,  321,  322,  258,   60,  301,  258,  271,  273,
  275,  286,   38,   42,   91,   40,  291,  294,  295,  296,
  323,  301,  301,  301,  301,  301,  301,  301,  301,  301,
  301,  301,  301,  301,  301,  301,  301,  301,  301,  301,
  301,  301,  301,  258,  259,  261,  301,  318,  319,  286,
  270,  293,   41,   59,   93,  301,  301,  319,   58,  301,
  125,  291,  298,  286,  295,   40,  295,   91,  291,  272,
  274,  291,  297,  298,  286,   60,  299,   93,   41,  286,
  301,   93,   41,  301,  285,   62,   44,  295,  297,  291,
  291,  291,   59,   41,  258,  298,  295,   93,   44,  291,
   41,   93,  260,  261,  285,   62,  321,  287,   93,   93,
  291,
};
#endif /* YYDESTRUCT_CALL || YYSTYPE_TOSTRING */
static const YYINT yydgoto[] = {                         22,
  162,   23,   24,  118,  119,  120,  173,  174,   90,   25,
  147,   27,   28,   29,   30,   31,   32,   33,   34,   35,
   36,   37,   38,   39,   40,   41,   42,  148,  149,   88,
  103,  104,   43,
};
static const YYINT yysindex[] = {                      1658,
    0,    0,    0,    0,    0,    0,    0,    0,    0, -283,
 -194,   46, 1689, -194, 1565, 1658, 1658, 1658, 1603, 1637,
 1658,    0,  -22, -206,    0, 2180,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0, -183, -194, -206, 1658, 2349, -206, 1658,   -9,
   -9,   -9,   -9, 1658, 1931,  -10,    0, 1721,   -9, -180,
  -43, 1658,  198, 1689, 1658, 1658, 1658, 1658, 1658, 1658,
 1658, 1658, 1658, 1658, 1658, 1658, 1658, 1658, 1658, 1658,
 1658, 1658, 1658, 1658, 1658, -208, 1658,    0, -182, -246,
 -206, 1970,   -9, 1998,   -8, 1658, 1658,    0, 1658,    0,
   49, 1658,   -7,    0,    0,  198, 2349,    0, -170, -137,
   91, -137,  465, -239,  198,  198,    0,    0,    0,  -51,
 -183, 2349, 2387, 2429, 1047, 1047, 1047, 1047,  345,  345,
 2349, 1047, 1047, 2457,  -32,  592,  621,  385,  385,    9,
    9,    9, 2035,    0,    0,    0, 2180,  101,  106,    0,
 -138, -206,    0, 1658,    0, 2063, 2180,  108, 1658, 2180,
    0,    0,  -44, -137,    0,  198,    0,  198,    0,  198,
  198,  105,  109,  121,  -92,  198, -230,    0,    0,    0,
 2100,    0,    0, 2138,    0,    0,  198,    0,  126,  -36,
    0,    0, -235,    0,    0,  -42,    0,    0, -180,    0,
 -119,    0,   76,   77,    0,    0,    0,  198,    0,    0,
    0,
};
static const YYINT yyrindex[] = {                       -96,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  163,
    0,    0, 1003,    0,  -96,  -96,  -96,  -96,  -96,  -96,
  -96,    0,  421,    1,    0,  171,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  -86,   29,  -96,  411,   68,  -96,  488,
  516,  555,  583,  -96,   10,    0,    0,    0,  841,   60,
    0,  -96,  -96, 1137,  -96,  -96,  -96,  -96,  -96,  -96,
  -96,  -96,  -96,  -96,  -96,  -96,  -96,  -96,  -96,  -96,
  -96,  -96,  -96,  -96,  -96,    0,  -37,    0,    0,    0,
   96,    0,  869,   10,    0,  -96,  -96,    0,  -37,    0,
    0,  -96,    0,    0,    0,  -96,  232,    0,    0,    0,
    0,    0,  -96,    0,  -96,  -25,    0,    0,    0,  449,
    0,  564, 1519,   58,  478,  718,  764, 1408, 1274, 1311,
  261, 1441, 1477, 1304, 1386, 1350, 1265, 1015, 1177,  880,
  908,  975,    0,    0,    0,    0,   40,  145,    0,    0,
    0,  135,    0,  -96,    0,    0,  -14,    0,  -96,   63,
    0,    0,    0,  -86,    0,  -25,    0,  -96,    0,  -96,
  -96,    0,    0,  161,    0,  -96,    0,    0,    0,    0,
    0,    0,    0,   87,    0,    0,  -96,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   60,    0,
    0,    0,    0,    0,    0,    0,    0,  -96,    0,    0,
    0,
};
#if YYBTYACC
static const YYINT yycindex[] = {                         0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,
};
#endif
static const YYINT yygindex[] = {                         0,
 1452,    0,    8,    0,   34,    0,   47,  -99,   93,    0,
 2522,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    2,  117,    0,
   18,    0, 1318,
};
#define YYTABLESIZE 2742
static const YYINT yytable[] = {                        187,
   89,  187,   44,   78,   84,   78,  163,   87,  176,   82,
   80,    2,   81,   86,   83,  117,  106,  186,   45,  206,
   56,   48,  193,  151,  203,  204,   77,  108,   93,   77,
   87,   79,  170,   97,  171,   97,   86,   89,   89,  151,
   89,   89,   89,   89,   89,   89,   89,   89,   87,  144,
  145,   91,  146,   76,   86,   95,  202,   65,   85,   89,
   89,   89,   89,    2,   89,   93,   93,   92,   93,   93,
   93,   93,   93,   93,   93,   93,  196,  101,   77,   61,
   76,   85,   98,   76,  155,   46,   89,   93,   93,   93,
   93,   89,   93,   89,   89,   90,  102,  152,   65,   85,
   60,   65,   76,  150,   92,   92,  159,   92,   92,   92,
   92,   92,   92,   92,   92,  164,   65,  161,   65,   93,
  108,   93,   93,   89,   89,   89,   92,   92,   92,   92,
  166,   92,   90,   90,   91,   90,   90,   90,   90,   90,
   90,   90,   90,  165,   97,  167,  179,  180,  183,  194,
   65,   93,   93,   93,   90,   90,   90,   90,   92,   90,
   92,   92,   88,  193,  187,  195,  201,  208,  209,  210,
    1,   91,   91,   82,   91,   91,   91,   91,   91,   91,
   91,   91,   65,   83,   20,   79,   90,   18,   90,   90,
   92,   92,   92,   91,   91,   91,   91,  188,   91,   88,
   88,  118,   88,   88,   88,   88,   88,   88,   88,   88,
  197,   19,  189,  177,  105,  158,  207,    0,   90,   90,
   90,   88,   88,   88,   88,   91,   88,   91,   91,    0,
    0,   74,   82,   63,  175,  113,    0,  116,    0,  114,
  185,    0,  205,    0,   82,    0,    0,    0,    0,    0,
    0,   71,   72,   88,    0,   88,   88,   91,   91,   91,
   73,    0,    0,    0,    0,   89,   89,    0,    0,    0,
    0,    0,   74,    0,   63,   74,    0,   89,   89,   89,
   89,   89,   89,   89,   89,   89,   88,   88,  115,    0,
   74,    0,    0,   93,   93,    0,    0,    0,    0,    0,
    0,   73,    0,    0,   73,   93,   93,   93,   93,   93,
   93,   93,   93,   93,    0,    0,    0,    0,    0,   73,
    0,    0,   65,    0,   74,    0,    0,    0,    0,    0,
    0,    0,   92,   92,   65,   65,   65,    0,    0,    0,
    0,    0,    0,    0,   92,   92,   92,   92,   92,   92,
   92,   92,   92,   73,    0,    0,   74,    0,    0,    0,
   90,   90,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   90,   90,   90,   90,   90,   90,   90,   90,
   90,   84,    0,    0,   87,   73,   82,   80,    0,   81,
   86,   83,    0,    0,    0,    0,    0,    0,    0,   91,
   91,    0,    0,    0,    0,    0,    0,    0,   79,    0,
   29,   91,   91,   91,   91,   91,   91,   91,   91,   91,
   86,   84,    0,    0,   87,    0,   82,   88,   88,    0,
   86,   83,    0,    0,    0,   85,    0,    0,    0,   88,
   88,   88,   88,   88,   88,   88,   88,   88,  105,    0,
    0,   29,    0,    0,   29,  108,    0,   86,   86,    0,
   86,   86,   86,   86,   86,   86,   86,   86,  109,   29,
  110,    0,  111,    0,    0,   85,    0,   66,    0,   86,
   86,   86,   86,  112,   86,  105,  105,   46,  105,  105,
  105,  105,  105,  105,  105,  105,    0,    0,    0,    0,
    0,    0,  113,   29,  116,    0,  114,  105,   74,  105,
  105,   86,  105,   86,   86,   42,    0,    0,   66,    0,
    0,   66,    0,    0,   46,   46,    0,    0,   46,   46,
   46,   46,   46,    0,   46,   29,   66,   73,   66,  105,
    0,  105,  105,    0,   86,   86,   46,   46,   46,   46,
    0,   46,   42,   42,   43,  168,   42,   42,   42,   42,
   42,    0,   42,   28,    0,    0,    0,    0,    0,    0,
   66,    0,  105,  105,   42,   42,   42,   42,    0,   42,
   46,   46,   45,    0,    0,    0,    0,    0,    0,    0,
    0,   43,   43,    0,    0,   43,   43,   43,   43,   43,
    0,   43,   66,    0,   28,    0,    0,   28,   42,   42,
   63,   46,   46,   43,   43,   43,   43,    0,   43,   45,
   45,    0,   28,   45,   45,   45,   45,   45,   84,   45,
    0,   87,    0,   82,   80,    0,   81,   86,   83,   42,
   42,   45,   45,   45,   45,    0,   45,   43,   43,    0,
   63,    0,    0,    0,    0,   79,   28,   84,    0,    0,
   87,    0,   82,   80,    0,   81,   86,   83,    0,    0,
    0,    0,    0,    0,    0,   45,   45,    0,   43,   43,
    0,    0,   85,    0,    0,   86,   86,    0,   28,    0,
    0,    0,    0,    0,    0,    0,    0,   86,   86,   86,
   86,   86,   86,   86,   86,   86,   45,   45,    0,    0,
    0,   85,    0,  105,  105,    0,    0,   67,    0,    0,
    0,    0,  108,    0,    0,  105,  105,  105,  105,  105,
  105,  105,  105,  105,    0,  109,    0,  110,    0,  111,
    0,    0,   66,    0,    0,    0,    0,    0,    0,    0,
  112,    0,   46,   46,   66,   66,   66,    0,   67,    0,
    0,   67,    0,   68,   46,   46,   46,   46,   46,   46,
   46,   46,   46,    0,    0,    0,   67,    0,   67,    0,
   42,   42,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   42,   42,   42,   42,   42,   42,   42,   42,
   42,    0,    0,    0,   68,    0,    0,   68,    0,    0,
   67,    0,    0,    0,    0,    0,    0,    0,    0,   43,
   43,    0,   68,    0,   68,    0,    0,    0,    0,    0,
    0,   43,   43,   43,   43,   43,   43,   43,   43,   43,
   44,    0,   67,    0,    0,    0,    0,   45,   45,    0,
    0,    0,    0,    0,    0,    0,   68,   63,    0,   45,
   45,   45,   45,   45,   45,   45,   45,   45,   47,    0,
    0,    0,    0,    0,    0,   71,   72,   44,   44,   53,
    0,   44,   44,   44,   44,   44,   63,   44,   68,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   44,
   44,   44,   44,    0,   44,   47,   47,   55,    0,   47,
   47,   47,   47,   47,    0,   47,   53,   53,    0,    0,
   53,   53,   53,   53,   53,    0,   53,   47,   47,   47,
   47,    0,   47,   44,   44,    0,    0,    0,   53,   53,
   53,   53,    0,   53,   55,   55,    0,    0,   55,   55,
   55,   55,   55,    0,   55,    0,    0,    0,    0,    0,
    0,   47,   47,    0,   44,   44,   55,   55,   55,   55,
    0,   55,   53,   53,   56,    0,    0,    0,    0,    0,
    0,    0,   67,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   47,   47,   67,   67,   67,    0,    0,    0,
   55,   55,   30,   53,   53,    0,    0,    0,    0,    0,
    0,   56,   56,    0,   62,   56,   56,   56,   56,   56,
    0,   56,    0,    0,    0,    0,    0,    0,   68,    0,
    0,   55,   55,   56,   56,   56,   56,    0,   56,   30,
   68,   68,   68,   30,    0,    0,   30,    0,   30,   30,
    0,    0,   62,    0,    0,   62,    0,   62,   62,   62,
    0,   30,   30,   30,   30,    0,   30,   56,   56,    0,
    0,    0,    0,   62,   62,   62,   62,    0,   62,    0,
    0,    0,    0,   84,   78,    0,   87,    0,   82,   80,
    0,   81,   86,   83,    0,   30,   30,    0,   56,   56,
    0,    0,    0,    0,    0,   44,   44,   62,   62,    0,
   79,    0,    0,    0,    0,    0,    0,   44,   44,   44,
   44,   44,   44,   44,   44,   44,   30,   30,    0,    0,
    0,    0,    0,   47,   47,    0,   27,   85,   62,   62,
   77,    0,    0,    0,   53,   47,   47,   47,   47,   47,
   47,   47,   47,   47,    0,    0,   53,   53,   53,   53,
   53,   53,   53,   53,   53,    0,    0,    0,    0,    0,
   76,    0,   55,   27,    0,    0,   63,   27,    0,    0,
   27,    0,   27,   27,   55,   55,   55,   55,   55,   55,
   55,   55,   55,    0,    0,   27,   27,   27,   27,    0,
   27,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   63,    0,    0,   63,    0,   63,
   63,   63,    0,    0,    0,    0,    0,    0,    0,   27,
   27,    0,    0,    0,    0,   63,   63,   63,   63,   56,
   63,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   56,   56,   56,   56,   56,   56,   56,   56,   56,
   27,   27,    0,    0,   54,    0,    0,   30,   30,   63,
   63,    0,   82,   70,    0,    0,    0,    0,    0,   62,
   30,   30,   30,   30,   30,   30,   30,   30,    0,    0,
    0,   62,   62,   62,   62,   62,   62,   62,   62,   62,
   63,   63,   54,   60,    0,   54,    0,    0,   54,    0,
   71,   70,   63,    0,   70,    0,    0,   70,    0,    0,
    0,    0,    0,   54,   54,   54,   54,    0,   54,    0,
   71,   72,   70,   70,   70,   70,    0,    0,    0,    0,
    0,    0,    0,    0,   60,    0,    0,   60,   71,   59,
    0,   71,    0,    0,   71,    0,    0,   54,   54,    0,
    0,    0,   60,   60,   60,   60,   70,   70,    0,   71,
   71,   71,   71,    0,    0,    0,    0,    0,    0,    0,
  121,    0,    0,    0,    0,   61,    0,   59,   54,   54,
   59,    0,    0,   59,    0,    0,   60,   70,   70,    0,
    0,   27,   27,   71,   71,    0,   82,   69,   59,   59,
   59,   59,    0,    0,   27,   27,   27,   27,   27,   27,
   27,   27,    0,  121,    0,    0,   61,   60,   60,   61,
  121,    0,  121,  121,   71,   71,    0,    0,    0,    0,
   57,   63,   59,   59,   61,   61,   61,   61,   69,    0,
    0,   69,    0,   63,   63,   63,   63,   63,   63,   63,
   63,   63,    0,    0,    0,    0,   69,    0,   69,    0,
    0,    0,    0,   59,   59,    0,   58,    0,   61,   61,
    0,   57,    0,  121,   57,  121,    0,  121,  121,    0,
    0,    0,    0,  121,    0,    0,    0,    0,    0,   57,
   69,   57,    0,    0,  121,    0,    0,    0,    0,   61,
   61,    0,    0,    0,  117,    0,    0,   58,   64,    0,
   58,    0,    0,    0,    0,  121,    0,    0,    0,   54,
    0,    0,   69,   57,    0,   58,    0,   58,   70,    0,
    0,   54,   54,   54,   54,   54,   54,   54,   54,   54,
   70,   70,   70,   70,   70,   70,   70,   70,   70,   64,
    0,    0,   64,    0,  169,   57,  172,    0,   60,   58,
    0,    0,    0,    0,    0,   71,    0,   64,    0,   64,
   60,   60,   60,   60,   60,   60,   60,   71,   71,   71,
   71,   71,   71,   71,   71,   71,    0,   21,    0,    0,
    0,   58,   15,    0,   20,    0,   18,   16,    0,   17,
    0,   64,    0,    0,   59,    0,    0,    0,    0,  190,
    0,  191,  192,    0,    0,    0,   59,   59,   59,   59,
   59,   59,   59,    0,    0,   21,    0,    0,  200,    0,
   15,    0,   20,   64,   18,   16,    0,   17,    0,    0,
   61,    0,    0,    0,    0,   19,    0,    0,    0,  211,
    0,    0,   61,   61,   61,   61,   61,   61,   61,   21,
    0,    0,   69,    0,   15,    0,   20,   57,   18,   16,
    0,   17,    0,    0,   69,   69,   69,    0,    0,    0,
   21,    0,    0,   19,    0,   15,    0,   20,    0,   18,
   16,    0,   17,    0,    0,   57,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   57,   57,   57,
    0,   21,    0,    0,    0,    0,   15,   19,   20,    0,
   18,   16,    0,   17,    0,    0,    0,    0,    0,    0,
    0,   58,    0,    0,    0,    0,    0,    0,   19,    0,
    0,    0,    0,   58,   58,   58,    0,   84,   78,    0,
   87,  100,   82,   80,   99,   81,   86,   83,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   19,
   74,   73,   75,   64,   79,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   64,   64,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   85,    0,    0,   77,    0,    0,    0,    0,    0,
    0,    1,    2,    0,    3,    4,    5,    6,    7,    0,
    0,    0,    8,    9,    0,   10,   49,   11,    0,    0,
   12,   13,    0,    0,   76,    0,    0,    0,    0,    0,
   14,    0,    0,    0,    0,    0,    0,    0,    0,    1,
    2,    0,    3,    4,    5,    6,    7,    0,    0,    0,
    8,    9,    0,   10,   54,   11,    0,    0,   12,   13,
    0,    0,    0,    0,    0,    0,    0,    0,   14,    0,
    0,    0,    0,    1,    2,    0,    3,    4,    5,    6,
    7,    0,    0,    0,    8,    9,    0,   10,    0,   11,
    0,    0,   12,   13,    1,    2,    0,    3,    4,    5,
    6,    7,   14,    0,    0,    8,    9,    0,   10,    0,
   11,    0,    0,   12,   13,    0,    0,    0,    0,    0,
    0,    0,    0,   14,    0,    1,    2,    0,    3,    4,
    5,    6,    7,    0,    0,    0,    8,    9,    0,   10,
    0,   11,    0,    0,   12,    0,    0,   84,   78,    0,
   87,    0,   82,   80,   14,   81,   86,   83,    0,    0,
    0,    0,    0,    0,    0,   62,   63,    0,    0,   96,
   74,   73,   75,    0,   79,    0,    0,   64,   65,   66,
   67,   68,   69,   70,   71,   72,   84,   78,    0,   87,
  153,   82,   80,    0,   81,   86,   83,    0,    0,    0,
    0,   85,    0,    0,   77,    0,    0,    0,    0,   74,
   73,   75,    0,   79,   84,   78,    0,   87,    0,   82,
   80,    0,   81,   86,   83,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   76,    0,  154,   74,   73,   75,
   85,   79,    0,   77,    0,    0,    0,    0,    0,    0,
    0,   84,   78,    0,   87,    0,   82,   80,    0,   81,
   86,   83,    0,    0,    0,    0,    0,    0,   85,    0,
    0,   77,    0,   76,   74,   73,   75,    0,   79,   84,
   78,    0,   87,    0,   82,   80,    0,   81,   86,   83,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   76,   74,   73,   75,   85,   79,  178,   77,    0,
    0,    0,    0,    0,    0,    0,   84,   78,    0,   87,
    0,   82,   80,    0,   81,   86,   83,    0,    0,    0,
    0,    0,    0,   85,    0,  182,   77,    0,   76,   74,
   73,   75,    0,   79,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   84,   78,    0,   87,    0,   82,
   80,  199,   81,   86,   83,    0,   76,    0,    0,    0,
   85,    0,  198,   77,    0,   62,   63,   74,   73,   75,
    0,   79,    0,    0,    0,    0,    0,   64,   65,   66,
   67,   68,   69,   70,   71,   72,   84,   78,    0,   87,
    0,   82,   80,   76,   81,   86,   83,    0,   85,    0,
    0,   77,    0,    0,   62,   63,    0,    0,    0,   74,
   73,   75,    0,   79,    0,    0,   64,   65,   66,   67,
   68,   69,   70,   71,   72,    0,    0,    0,    0,    0,
    0,   76,   62,   63,    0,    0,    0,    0,    0,    0,
   85,    0,    0,   77,   64,   65,   66,   67,   68,   69,
   70,   71,   72,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   62,
   63,    0,    0,   76,    0,    0,    0,    0,    0,    0,
    0,   64,   65,   66,   67,   68,   69,   70,   71,   72,
    0,    0,    0,    0,    0,    0,    0,   62,   63,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   64,
   65,   66,   67,   68,   69,   70,   71,   72,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   62,   63,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   64,   65,   66,   67,
   68,   69,   70,   71,   72,   84,   78,    0,   87,    0,
   82,   80,    0,   81,   86,   83,    0,    0,    0,    0,
    0,    0,   62,   63,    0,    0,    0,    0,   74,   73,
   75,    0,   79,    0,   64,   65,   66,   67,   68,   69,
   70,   71,   72,   84,   78,    0,   87,    0,   82,   80,
    0,   81,   86,   83,    0,    0,    0,    0,    0,   85,
    0,    0,   77,    0,   62,   63,   74,    0,   75,    0,
   79,    0,    0,    0,    0,    0,   64,   65,   66,   67,
   68,   69,   70,   71,   72,   84,   78,    0,   87,    0,
   82,   80,   76,   81,   86,   83,    0,   85,    0,    0,
   77,    0,    0,    0,    0,    0,    0,    0,   74,    0,
   75,    0,   79,   84,   78,    0,   87,    0,   82,   80,
    0,   81,   86,   83,    0,    0,    0,    0,    0,    0,
   76,    0,    0,    0,    0,    0,    0,    0,    0,   85,
   79,   26,   77,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   47,    0,   50,   51,   52,   53,
   55,   58,   59,    0,    0,    0,    0,   85,    0,    0,
   77,    0,   76,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   92,    0,    0,
   93,    0,    0,    0,    0,   94,    0,    0,    0,    0,
    0,    0,    0,  107,    0,  122,  123,  124,  125,  126,
  127,  128,  129,  130,  131,  132,  133,  134,  135,  136,
  137,  138,  139,  140,  141,  142,  143,    0,    0,    0,
    0,    0,    0,   62,   63,    0,    0,  156,  157,    0,
    0,    0,    0,  160,    0,    0,   65,   66,   67,   68,
   69,   70,   71,   72,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   63,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   66,   67,   68,   69,   70,
   71,   72,    0,    0,    0,  181,    0,    0,    0,    0,
  184,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   63,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   67,   68,
   69,   70,   71,   72,    0,    0,    0,    0,    0,    0,
    0,    0,   63,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   71,   72,
};
static const YYINT yycheck[] = {                         44,
    0,   44,  286,   41,   37,   38,  106,   40,   60,   42,
   43,  258,   45,   46,   47,   41,   60,   62,   11,   62,
   19,   14,   59,  270,  260,  261,   41,  258,    0,   44,
   40,   64,  272,   44,  274,   44,   46,   37,   38,  270,
   40,   41,   42,   43,   44,   45,   46,   47,   40,  258,
  259,   44,  261,   44,   46,   54,   93,    0,   91,   59,
   60,   61,   62,  258,   64,   37,   38,    0,   40,   41,
   42,   43,   44,   45,   46,   47,  176,  258,   93,  286,
   41,   91,   93,   44,   93,   40,  270,   59,   60,   61,
   62,   91,   64,   93,   94,    0,  277,   90,   41,   91,
  123,   44,   93,  286,   37,   38,   58,   40,   41,   42,
   43,   44,   45,   46,   47,  286,   59,  125,   61,   91,
  258,   93,   94,  123,  124,  125,   59,   60,   61,   62,
   40,   64,   37,   38,    0,   40,   41,   42,   43,   44,
   45,   46,   47,  110,   44,  112,   41,  286,   41,   41,
   93,  123,  124,  125,   59,   60,   61,   62,   91,   64,
   93,   94,    0,   59,   44,  258,   41,  287,   93,   93,
    0,   37,   38,  270,   40,   41,   42,   43,   44,   45,
   46,   47,  125,  270,  125,   41,   91,  125,   93,   94,
  123,  124,  125,   59,   60,   61,   62,  164,   64,   37,
   38,   41,   40,   41,   42,   43,   44,   45,   46,   47,
  177,  125,  166,  121,  258,   99,  199,   -1,  123,  124,
  125,   59,   60,   61,   62,   91,   64,   93,   94,   -1,
   -1,    0,  270,  266,  286,   38,   -1,   40,   -1,   42,
  285,   -1,  285,   -1,  270,   -1,   -1,   -1,   -1,   -1,
   -1,  284,  285,   91,   -1,   93,   94,  123,  124,  125,
    0,   -1,   -1,   -1,   -1,  265,  266,   -1,   -1,   -1,
   -1,   -1,   41,   -1,  266,   44,   -1,  277,  278,  279,
  280,  281,  282,  283,  284,  285,  124,  125,   91,   -1,
   59,   -1,   -1,  265,  266,   -1,   -1,   -1,   -1,   -1,
   -1,   41,   -1,   -1,   44,  277,  278,  279,  280,  281,
  282,  283,  284,  285,   -1,   -1,   -1,   -1,   -1,   59,
   -1,   -1,  265,   -1,   93,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,  265,  266,  277,  278,  279,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  277,  278,  279,  280,  281,  282,
  283,  284,  285,   93,   -1,   -1,  125,   -1,   -1,   -1,
  265,  266,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,  277,  278,  279,  280,  281,  282,  283,  284,
  285,   37,   -1,   -1,   40,  125,   42,   43,   -1,   45,
   46,   47,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  265,
  266,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   64,   -1,
    0,  277,  278,  279,  280,  281,  282,  283,  284,  285,
    0,   37,   -1,   -1,   40,   -1,   42,  265,  266,   -1,
   46,   47,   -1,   -1,   -1,   91,   -1,   -1,   -1,  277,
  278,  279,  280,  281,  282,  283,  284,  285,    0,   -1,
   -1,   41,   -1,   -1,   44,  258,   -1,   37,   38,   -1,
   40,   41,   42,   43,   44,   45,   46,   47,  271,   59,
  273,   -1,  275,   -1,   -1,   91,   -1,    0,   -1,   59,
   60,   61,   62,  286,   64,   37,   38,    0,   40,   41,
   42,   43,   44,   45,   46,   47,   -1,   -1,   -1,   -1,
   -1,   -1,   38,   93,   40,   -1,   42,   59,  277,   61,
   62,   91,   64,   93,   94,    0,   -1,   -1,   41,   -1,
   -1,   44,   -1,   -1,   37,   38,   -1,   -1,   41,   42,
   43,   44,   45,   -1,   47,  125,   59,  277,   61,   91,
   -1,   93,   94,   -1,  124,  125,   59,   60,   61,   62,
   -1,   64,   37,   38,    0,   91,   41,   42,   43,   44,
   45,   -1,   47,    0,   -1,   -1,   -1,   -1,   -1,   -1,
   93,   -1,  124,  125,   59,   60,   61,   62,   -1,   64,
   93,   94,    0,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   37,   38,   -1,   -1,   41,   42,   43,   44,   45,
   -1,   47,  125,   -1,   41,   -1,   -1,   44,   93,   94,
  266,  124,  125,   59,   60,   61,   62,   -1,   64,   37,
   38,   -1,   59,   41,   42,   43,   44,   45,   37,   47,
   -1,   40,   -1,   42,   43,   -1,   45,   46,   47,  124,
  125,   59,   60,   61,   62,   -1,   64,   93,   94,   -1,
  266,   -1,   -1,   -1,   -1,   64,   93,   37,   -1,   -1,
   40,   -1,   42,   43,   -1,   45,   46,   47,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   93,   94,   -1,  124,  125,
   -1,   -1,   91,   -1,   -1,  265,  266,   -1,  125,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  277,  278,  279,
  280,  281,  282,  283,  284,  285,  124,  125,   -1,   -1,
   -1,   91,   -1,  265,  266,   -1,   -1,    0,   -1,   -1,
   -1,   -1,  258,   -1,   -1,  277,  278,  279,  280,  281,
  282,  283,  284,  285,   -1,  271,   -1,  273,   -1,  275,
   -1,   -1,  265,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
  286,   -1,  265,  266,  277,  278,  279,   -1,   41,   -1,
   -1,   44,   -1,    0,  277,  278,  279,  280,  281,  282,
  283,  284,  285,   -1,   -1,   -1,   59,   -1,   61,   -1,
  265,  266,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,  277,  278,  279,  280,  281,  282,  283,  284,
  285,   -1,   -1,   -1,   41,   -1,   -1,   44,   -1,   -1,
   93,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  265,
  266,   -1,   59,   -1,   61,   -1,   -1,   -1,   -1,   -1,
   -1,  277,  278,  279,  280,  281,  282,  283,  284,  285,
    0,   -1,  125,   -1,   -1,   -1,   -1,  265,  266,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   93,  266,   -1,  277,
  278,  279,  280,  281,  282,  283,  284,  285,    0,   -1,
   -1,   -1,   -1,   -1,   -1,  284,  285,   37,   38,    0,
   -1,   41,   42,   43,   44,   45,  266,   47,  125,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   59,
   60,   61,   62,   -1,   64,   37,   38,    0,   -1,   41,
   42,   43,   44,   45,   -1,   47,   37,   38,   -1,   -1,
   41,   42,   43,   44,   45,   -1,   47,   59,   60,   61,
   62,   -1,   64,   93,   94,   -1,   -1,   -1,   59,   60,
   61,   62,   -1,   64,   37,   38,   -1,   -1,   41,   42,
   43,   44,   45,   -1,   47,   -1,   -1,   -1,   -1,   -1,
   -1,   93,   94,   -1,  124,  125,   59,   60,   61,   62,
   -1,   64,   93,   94,    0,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,  265,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,  124,  125,  277,  278,  279,   -1,   -1,   -1,
   93,   94,    0,  124,  125,   -1,   -1,   -1,   -1,   -1,
   -1,   37,   38,   -1,    0,   41,   42,   43,   44,   45,
   -1,   47,   -1,   -1,   -1,   -1,   -1,   -1,  265,   -1,
   -1,  124,  125,   59,   60,   61,   62,   -1,   64,   37,
  277,  278,  279,   41,   -1,   -1,   44,   -1,   46,   47,
   -1,   -1,   38,   -1,   -1,   41,   -1,   43,   44,   45,
   -1,   59,   60,   61,   62,   -1,   64,   93,   94,   -1,
   -1,   -1,   -1,   59,   60,   61,   62,   -1,   64,   -1,
   -1,   -1,   -1,   37,   38,   -1,   40,   -1,   42,   43,
   -1,   45,   46,   47,   -1,   93,   94,   -1,  124,  125,
   -1,   -1,   -1,   -1,   -1,  265,  266,   93,   94,   -1,
   64,   -1,   -1,   -1,   -1,   -1,   -1,  277,  278,  279,
  280,  281,  282,  283,  284,  285,  124,  125,   -1,   -1,
   -1,   -1,   -1,  265,  266,   -1,    0,   91,  124,  125,
   94,   -1,   -1,   -1,  265,  277,  278,  279,  280,  281,
  282,  283,  284,  285,   -1,   -1,  277,  278,  279,  280,
  281,  282,  283,  284,  285,   -1,   -1,   -1,   -1,   -1,
  124,   -1,  265,   37,   -1,   -1,    0,   41,   -1,   -1,
   44,   -1,   46,   47,  277,  278,  279,  280,  281,  282,
  283,  284,  285,   -1,   -1,   59,   60,   61,   62,   -1,
   64,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   38,   -1,   -1,   41,   -1,   43,
   44,   45,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   93,
   94,   -1,   -1,   -1,   -1,   59,   60,   61,   62,  265,
   64,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  277,  278,  279,  280,  281,  282,  283,  284,  285,
  124,  125,   -1,   -1,    0,   -1,   -1,  265,  266,   93,
   94,   -1,  270,    0,   -1,   -1,   -1,   -1,   -1,  265,
  278,  279,  280,  281,  282,  283,  284,  285,   -1,   -1,
   -1,  277,  278,  279,  280,  281,  282,  283,  284,  285,
  124,  125,   38,    0,   -1,   41,   -1,   -1,   44,   -1,
    0,   38,  266,   -1,   41,   -1,   -1,   44,   -1,   -1,
   -1,   -1,   -1,   59,   60,   61,   62,   -1,   64,   -1,
  284,  285,   59,   60,   61,   62,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   41,   -1,   -1,   44,   38,    0,
   -1,   41,   -1,   -1,   44,   -1,   -1,   93,   94,   -1,
   -1,   -1,   59,   60,   61,   62,   93,   94,   -1,   59,
   60,   61,   62,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   63,   -1,   -1,   -1,   -1,    0,   -1,   38,  124,  125,
   41,   -1,   -1,   44,   -1,   -1,   93,  124,  125,   -1,
   -1,  265,  266,   93,   94,   -1,  270,    0,   59,   60,
   61,   62,   -1,   -1,  278,  279,  280,  281,  282,  283,
  284,  285,   -1,  106,   -1,   -1,   41,  124,  125,   44,
  113,   -1,  115,  116,  124,  125,   -1,   -1,   -1,   -1,
    0,  265,   93,   94,   59,   60,   61,   62,   41,   -1,
   -1,   44,   -1,  277,  278,  279,  280,  281,  282,  283,
  284,  285,   -1,   -1,   -1,   -1,   59,   -1,   61,   -1,
   -1,   -1,   -1,  124,  125,   -1,    0,   -1,   93,   94,
   -1,   41,   -1,  166,   44,  168,   -1,  170,  171,   -1,
   -1,   -1,   -1,  176,   -1,   -1,   -1,   -1,   -1,   59,
   93,   61,   -1,   -1,  187,   -1,   -1,   -1,   -1,  124,
  125,   -1,   -1,   -1,   63,   -1,   -1,   41,    0,   -1,
   44,   -1,   -1,   -1,   -1,  208,   -1,   -1,   -1,  265,
   -1,   -1,  125,   93,   -1,   59,   -1,   61,  265,   -1,
   -1,  277,  278,  279,  280,  281,  282,  283,  284,  285,
  277,  278,  279,  280,  281,  282,  283,  284,  285,   41,
   -1,   -1,   44,   -1,  113,  125,  115,   -1,  265,   93,
   -1,   -1,   -1,   -1,   -1,  265,   -1,   59,   -1,   61,
  277,  278,  279,  280,  281,  282,  283,  277,  278,  279,
  280,  281,  282,  283,  284,  285,   -1,   33,   -1,   -1,
   -1,  125,   38,   -1,   40,   -1,   42,   43,   -1,   45,
   -1,   93,   -1,   -1,  265,   -1,   -1,   -1,   -1,  168,
   -1,  170,  171,   -1,   -1,   -1,  277,  278,  279,  280,
  281,  282,  283,   -1,   -1,   33,   -1,   -1,  187,   -1,
   38,   -1,   40,  125,   42,   43,   -1,   45,   -1,   -1,
  265,   -1,   -1,   -1,   -1,   91,   -1,   -1,   -1,  208,
   -1,   -1,  277,  278,  279,  280,  281,  282,  283,   33,
   -1,   -1,  265,   -1,   38,   -1,   40,   41,   42,   43,
   -1,   45,   -1,   -1,  277,  278,  279,   -1,   -1,   -1,
   33,   -1,   -1,   91,   -1,   38,   -1,   40,   -1,   42,
   43,   -1,   45,   -1,   -1,  265,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  277,  278,  279,
   -1,   33,   -1,   -1,   -1,   -1,   38,   91,   40,   -1,
   42,   43,   -1,   45,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  265,   -1,   -1,   -1,   -1,   -1,   -1,   91,   -1,
   -1,   -1,   -1,  277,  278,  279,   -1,   37,   38,   -1,
   40,   41,   42,   43,   44,   45,   46,   47,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   91,
   60,   61,   62,  265,   64,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,  277,  278,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   91,   -1,   -1,   94,   -1,   -1,   -1,   -1,   -1,
   -1,  257,  258,   -1,  260,  261,  262,  263,  264,   -1,
   -1,   -1,  268,  269,   -1,  271,  272,  273,   -1,   -1,
  276,  277,   -1,   -1,  124,   -1,   -1,   -1,   -1,   -1,
  286,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  257,
  258,   -1,  260,  261,  262,  263,  264,   -1,   -1,   -1,
  268,  269,   -1,  271,  272,  273,   -1,   -1,  276,  277,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  286,   -1,
   -1,   -1,   -1,  257,  258,   -1,  260,  261,  262,  263,
  264,   -1,   -1,   -1,  268,  269,   -1,  271,   -1,  273,
   -1,   -1,  276,  277,  257,  258,   -1,  260,  261,  262,
  263,  264,  286,   -1,   -1,  268,  269,   -1,  271,   -1,
  273,   -1,   -1,  276,  277,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,  286,   -1,  257,  258,   -1,  260,  261,
  262,  263,  264,   -1,   -1,   -1,  268,  269,   -1,  271,
   -1,  273,   -1,   -1,  276,   -1,   -1,   37,   38,   -1,
   40,   -1,   42,   43,  286,   45,   46,   47,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,  265,  266,   -1,   -1,   59,
   60,   61,   62,   -1,   64,   -1,   -1,  277,  278,  279,
  280,  281,  282,  283,  284,  285,   37,   38,   -1,   40,
   41,   42,   43,   -1,   45,   46,   47,   -1,   -1,   -1,
   -1,   91,   -1,   -1,   94,   -1,   -1,   -1,   -1,   60,
   61,   62,   -1,   64,   37,   38,   -1,   40,   -1,   42,
   43,   -1,   45,   46,   47,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  124,   -1,   59,   60,   61,   62,
   91,   64,   -1,   94,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   37,   38,   -1,   40,   -1,   42,   43,   -1,   45,
   46,   47,   -1,   -1,   -1,   -1,   -1,   -1,   91,   -1,
   -1,   94,   -1,  124,   60,   61,   62,   -1,   64,   37,
   38,   -1,   40,   -1,   42,   43,   -1,   45,   46,   47,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  124,   60,   61,   62,   91,   64,   93,   94,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   37,   38,   -1,   40,
   -1,   42,   43,   -1,   45,   46,   47,   -1,   -1,   -1,
   -1,   -1,   -1,   91,   -1,   93,   94,   -1,  124,   60,
   61,   62,   -1,   64,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   37,   38,   -1,   40,   -1,   42,
   43,   44,   45,   46,   47,   -1,  124,   -1,   -1,   -1,
   91,   -1,   93,   94,   -1,  265,  266,   60,   61,   62,
   -1,   64,   -1,   -1,   -1,   -1,   -1,  277,  278,  279,
  280,  281,  282,  283,  284,  285,   37,   38,   -1,   40,
   -1,   42,   43,  124,   45,   46,   47,   -1,   91,   -1,
   -1,   94,   -1,   -1,  265,  266,   -1,   -1,   -1,   60,
   61,   62,   -1,   64,   -1,   -1,  277,  278,  279,  280,
  281,  282,  283,  284,  285,   -1,   -1,   -1,   -1,   -1,
   -1,  124,  265,  266,   -1,   -1,   -1,   -1,   -1,   -1,
   91,   -1,   -1,   94,  277,  278,  279,  280,  281,  282,
  283,  284,  285,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  265,
  266,   -1,   -1,  124,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  277,  278,  279,  280,  281,  282,  283,  284,  285,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  265,  266,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  277,
  278,  279,  280,  281,  282,  283,  284,  285,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  265,  266,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,  277,  278,  279,  280,
  281,  282,  283,  284,  285,   37,   38,   -1,   40,   -1,
   42,   43,   -1,   45,   46,   47,   -1,   -1,   -1,   -1,
   -1,   -1,  265,  266,   -1,   -1,   -1,   -1,   60,   61,
   62,   -1,   64,   -1,  277,  278,  279,  280,  281,  282,
  283,  284,  285,   37,   38,   -1,   40,   -1,   42,   43,
   -1,   45,   46,   47,   -1,   -1,   -1,   -1,   -1,   91,
   -1,   -1,   94,   -1,  265,  266,   60,   -1,   62,   -1,
   64,   -1,   -1,   -1,   -1,   -1,  277,  278,  279,  280,
  281,  282,  283,  284,  285,   37,   38,   -1,   40,   -1,
   42,   43,  124,   45,   46,   47,   -1,   91,   -1,   -1,
   94,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   60,   -1,
   62,   -1,   64,   37,   38,   -1,   40,   -1,   42,   43,
   -1,   45,   46,   47,   -1,   -1,   -1,   -1,   -1,   -1,
  124,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   91,
   64,    0,   94,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   13,   -1,   15,   16,   17,   18,
   19,   20,   21,   -1,   -1,   -1,   -1,   91,   -1,   -1,
   94,   -1,  124,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   46,   -1,   -1,
   49,   -1,   -1,   -1,   -1,   54,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   62,   -1,   64,   65,   66,   67,   68,
   69,   70,   71,   72,   73,   74,   75,   76,   77,   78,
   79,   80,   81,   82,   83,   84,   85,   -1,   -1,   -1,
   -1,   -1,   -1,  265,  266,   -1,   -1,   96,   97,   -1,
   -1,   -1,   -1,  102,   -1,   -1,  278,  279,  280,  281,
  282,  283,  284,  285,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,  266,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,  279,  280,  281,  282,  283,
  284,  285,   -1,   -1,   -1,  154,   -1,   -1,   -1,   -1,
  159,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  266,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  280,  281,
  282,  283,  284,  285,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,  266,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
  284,  285,
};
#if YYBTYACC
static const YYINT yyctable[] = {                        -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,
};
#endif
#define YYFINAL 22
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 288
#define YYUNDFTOKEN 324
#define YYTRANSLATE(a) ((a) > YYMAXTOKEN ? YYUNDFTOKEN : (a))
#if YYDEBUG
static const char *const yyname[] = {

"$end",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'!'",0,
0,0,"'%'","'&'",0,"'('","')'","'*'","'+'","','","'-'","'.'","'/'",0,0,0,0,0,0,0,
0,0,0,"':'","';'","'<'","'='","'>'",0,"'@'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,"'['",0,"']'","'^'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,"'{'","'|'","'}'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"error","GDBVAR","IDENT",
"COMPLETE","INTEGER","DECIMAL_INTEGER","STRING","BYTESTRING","FLOAT",
"COMPOUND_ASSIGN","KW_AS","KW_IF","KW_TRUE","KW_FALSE","KW_SUPER","KW_SELF",
"KW_MUT","KW_EXTERN","KW_CONST","KW_FN","KW_SIZEOF","DOTDOT","OROR","ANDAND",
"EQEQ","NOTEQ","LTEQ","GTEQ","LSH","RSH","COLONCOLON","ARROW","UNARY","$accept",
"start","type","path_for_expr","identifier_path_for_expr","path_for_type",
"identifier_path_for_type","just_identifiers_for_type","maybe_type_list",
"type_list","super_path","literal","expr","field_expr","idx_expr","unop_expr",
"binop_expr","binop_expr_expr","type_cast_expr","assignment_expr",
"compound_assignment_expr","paren_expr","call_expr","path_expr","tuple_expr",
"unit_expr","struct_expr","array_expr","range_expr","expr_list",
"maybe_expr_list","paren_expr_list","struct_expr_list","struct_expr_tail",
"maybe_self_path","illegal-symbol",
};
static const char *const yyrule[] = {
"$accept : start",
"start : expr",
"expr : literal",
"expr : path_expr",
"expr : tuple_expr",
"expr : unit_expr",
"expr : struct_expr",
"expr : field_expr",
"expr : array_expr",
"expr : idx_expr",
"expr : range_expr",
"expr : unop_expr",
"expr : binop_expr",
"expr : paren_expr",
"expr : call_expr",
"tuple_expr : '(' expr ',' maybe_expr_list ')'",
"unit_expr : '(' ')'",
"struct_expr : path_for_expr '{' struct_expr_list '}'",
"struct_expr_tail : DOTDOT expr",
"struct_expr_tail : IDENT ':' expr",
"struct_expr_list :",
"struct_expr_list : struct_expr_tail",
"struct_expr_list : IDENT ':' expr ',' struct_expr_list",
"array_expr : '[' KW_MUT expr_list ']'",
"array_expr : '[' expr_list ']'",
"array_expr : '[' KW_MUT expr ';' expr ']'",
"array_expr : '[' expr ';' expr ']'",
"range_expr : expr DOTDOT",
"range_expr : expr DOTDOT expr",
"range_expr : DOTDOT expr",
"range_expr : DOTDOT",
"literal : INTEGER",
"literal : DECIMAL_INTEGER",
"literal : FLOAT",
"literal : STRING",
"literal : BYTESTRING",
"literal : KW_TRUE",
"literal : KW_FALSE",
"field_expr : expr '.' IDENT",
"field_expr : expr '.' COMPLETE",
"field_expr : expr '.' DECIMAL_INTEGER",
"idx_expr : expr '[' expr ']'",
"unop_expr : '+' expr",
"unop_expr : '-' expr",
"unop_expr : '!' expr",
"unop_expr : '*' expr",
"unop_expr : '&' expr",
"unop_expr : '&' KW_MUT expr",
"unop_expr : KW_SIZEOF '(' expr ')'",
"binop_expr : binop_expr_expr",
"binop_expr : type_cast_expr",
"binop_expr : assignment_expr",
"binop_expr : compound_assignment_expr",
"binop_expr_expr : expr '*' expr",
"binop_expr_expr : expr '@' expr",
"binop_expr_expr : expr '/' expr",
"binop_expr_expr : expr '%' expr",
"binop_expr_expr : expr '<' expr",
"binop_expr_expr : expr '>' expr",
"binop_expr_expr : expr '&' expr",
"binop_expr_expr : expr '|' expr",
"binop_expr_expr : expr '^' expr",
"binop_expr_expr : expr '+' expr",
"binop_expr_expr : expr '-' expr",
"binop_expr_expr : expr OROR expr",
"binop_expr_expr : expr ANDAND expr",
"binop_expr_expr : expr EQEQ expr",
"binop_expr_expr : expr NOTEQ expr",
"binop_expr_expr : expr LTEQ expr",
"binop_expr_expr : expr GTEQ expr",
"binop_expr_expr : expr LSH expr",
"binop_expr_expr : expr RSH expr",
"type_cast_expr : expr KW_AS type",
"assignment_expr : expr '=' expr",
"compound_assignment_expr : expr COMPOUND_ASSIGN expr",
"paren_expr : '(' expr ')'",
"expr_list : expr",
"expr_list : expr_list ',' expr",
"maybe_expr_list :",
"maybe_expr_list : expr_list",
"paren_expr_list : '(' maybe_expr_list ')'",
"call_expr : expr paren_expr_list",
"maybe_self_path :",
"maybe_self_path : KW_SELF COLONCOLON",
"super_path : KW_SUPER COLONCOLON",
"super_path : super_path KW_SUPER COLONCOLON",
"path_expr : path_for_expr",
"path_expr : GDBVAR",
"path_expr : KW_SELF",
"path_for_expr : identifier_path_for_expr",
"path_for_expr : KW_SELF COLONCOLON identifier_path_for_expr",
"path_for_expr : maybe_self_path super_path identifier_path_for_expr",
"path_for_expr : COLONCOLON identifier_path_for_expr",
"path_for_expr : KW_EXTERN identifier_path_for_expr",
"identifier_path_for_expr : IDENT",
"identifier_path_for_expr : identifier_path_for_expr COLONCOLON IDENT",
"identifier_path_for_expr : identifier_path_for_expr COLONCOLON '<' type_list '>'",
"identifier_path_for_expr : identifier_path_for_expr COLONCOLON '<' type_list RSH",
"path_for_type : identifier_path_for_type",
"path_for_type : KW_SELF COLONCOLON identifier_path_for_type",
"path_for_type : maybe_self_path super_path identifier_path_for_type",
"path_for_type : COLONCOLON identifier_path_for_type",
"path_for_type : KW_EXTERN identifier_path_for_type",
"just_identifiers_for_type : IDENT",
"just_identifiers_for_type : just_identifiers_for_type COLONCOLON IDENT",
"identifier_path_for_type : just_identifiers_for_type",
"identifier_path_for_type : just_identifiers_for_type '<' type_list '>'",
"identifier_path_for_type : just_identifiers_for_type '<' type_list RSH",
"type : path_for_type",
"type : '[' type ';' INTEGER ']'",
"type : '[' type ';' DECIMAL_INTEGER ']'",
"type : '&' '[' type ']'",
"type : '&' type",
"type : '*' KW_MUT type",
"type : '*' KW_CONST type",
"type : KW_FN '(' maybe_type_list ')' ARROW type",
"type : '(' maybe_type_list ')'",
"maybe_type_list :",
"maybe_type_list : type_list",
"type_list : type",
"type_list : type_list ',' type",

};
#endif

#if YYDEBUG
int      yydebug;
#endif

int      yyerrflag;
int      yychar;
YYSTYPE  yyval;
YYSTYPE  yylval;
int      yynerrs;

#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
YYLTYPE  yyloc; /* position returned by actions */
YYLTYPE  yylloc; /* position from the lexer */
#endif

#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
#ifndef YYLLOC_DEFAULT
#define YYLLOC_DEFAULT(loc, rhs, n) \
do \
{ \
    if (n == 0) \
    { \
        (loc).first_line   = YYRHSLOC(rhs, 0).last_line; \
        (loc).first_column = YYRHSLOC(rhs, 0).last_column; \
        (loc).last_line    = YYRHSLOC(rhs, 0).last_line; \
        (loc).last_column  = YYRHSLOC(rhs, 0).last_column; \
    } \
    else \
    { \
        (loc).first_line   = YYRHSLOC(rhs, 1).first_line; \
        (loc).first_column = YYRHSLOC(rhs, 1).first_column; \
        (loc).last_line    = YYRHSLOC(rhs, n).last_line; \
        (loc).last_column  = YYRHSLOC(rhs, n).last_column; \
    } \
} while (0)
#endif /* YYLLOC_DEFAULT */
#endif /* defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED) */
#if YYBTYACC

#ifndef YYLVQUEUEGROWTH
#define YYLVQUEUEGROWTH 32
#endif
#endif /* YYBTYACC */

/* define the initial stack-sizes */
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH  YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 10000
#define YYMAXDEPTH  10000
#endif
#endif

#ifndef YYINITSTACKSIZE
#define YYINITSTACKSIZE 200
#endif

typedef struct {
    unsigned stacksize;
    YYINT    *s_base;
    YYINT    *s_mark;
    YYINT    *s_last;
    YYSTYPE  *l_base;
    YYSTYPE  *l_mark;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    YYLTYPE  *p_base;
    YYLTYPE  *p_mark;
#endif
} YYSTACKDATA;
#if YYBTYACC

struct YYParseState_s
{
    struct YYParseState_s *save;    /* Previously saved parser state */
    YYSTACKDATA            yystack; /* saved parser stack */
    int                    state;   /* saved parser state */
    int                    errflag; /* saved error recovery status */
    int                    lexeme;  /* saved index of the conflict lexeme in the lexical queue */
    YYINT                  ctry;    /* saved index in yyctable[] for this conflict */
};
typedef struct YYParseState_s YYParseState;
#endif /* YYBTYACC */
/* variables for the parser stack */
static YYSTACKDATA yystack;
#if YYBTYACC

/* Current parser state */
static YYParseState *yyps = 0;

/* yypath != NULL: do the full parse, starting at *yypath parser state. */
static YYParseState *yypath = 0;

/* Base of the lexical value queue */
static YYSTYPE *yylvals = 0;

/* Current position at lexical value queue */
static YYSTYPE *yylvp = 0;

/* End position of lexical value queue */
static YYSTYPE *yylve = 0;

/* The last allocated position at the lexical value queue */
static YYSTYPE *yylvlim = 0;

#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
/* Base of the lexical position queue */
static YYLTYPE *yylpsns = 0;

/* Current position at lexical position queue */
static YYLTYPE *yylpp = 0;

/* End position of lexical position queue */
static YYLTYPE *yylpe = 0;

/* The last allocated position at the lexical position queue */
static YYLTYPE *yylplim = 0;
#endif

/* Current position at lexical token queue */
static YYINT  *yylexp = 0;

static YYINT  *yylexemes = 0;
#endif /* YYBTYACC */
#line 853 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"

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
#line 3506 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.c"

/* For use in generated program */
#define yydepth (int)(yystack.s_mark - yystack.s_base)
#if YYBTYACC
#define yytrial (yyps->save)
#endif /* YYBTYACC */

#if YYDEBUG
#include <stdio.h>	/* needed for printf */
#endif

#include <stdlib.h>	/* needed for malloc, etc */
#include <string.h>	/* needed for memset */

/* allocate initial stack or double stack size, up to YYMAXDEPTH */
static int yygrowstack(YYSTACKDATA *data)
{
    int i;
    unsigned newsize;
    YYINT *newss;
    YYSTYPE *newvs;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    YYLTYPE *newps;
#endif

    if ((newsize = data->stacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return YYENOMEM;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;

    i = (int) (data->s_mark - data->s_base);
    newss = (YYINT *)realloc(data->s_base, newsize * sizeof(*newss));
    if (newss == 0)
        return YYENOMEM;

    data->s_base = newss;
    data->s_mark = newss + i;

    newvs = (YYSTYPE *)realloc(data->l_base, newsize * sizeof(*newvs));
    if (newvs == 0)
        return YYENOMEM;

    data->l_base = newvs;
    data->l_mark = newvs + i;

#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    newps = (YYLTYPE *)realloc(data->p_base, newsize * sizeof(*newps));
    if (newps == 0)
        return YYENOMEM;

    data->p_base = newps;
    data->p_mark = newps + i;
#endif

    data->stacksize = newsize;
    data->s_last = data->s_base + newsize - 1;

#if YYDEBUG
    if (yydebug)
        fprintf(stderr, "%sdebug: stack size increased to %d\n", YYPREFIX, newsize);
#endif
    return 0;
}

#if YYPURE || defined(YY_NO_LEAKS)
static void yyfreestack(YYSTACKDATA *data)
{
    free(data->s_base);
    free(data->l_base);
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    free(data->p_base);
#endif
    memset(data, 0, sizeof(*data));
}
#else
#define yyfreestack(data) /* nothing */
#endif /* YYPURE || defined(YY_NO_LEAKS) */
#if YYBTYACC

static YYParseState *
yyNewState(unsigned size)
{
    YYParseState *p = (YYParseState *) malloc(sizeof(YYParseState));
    if (p == NULL) return NULL;

    p->yystack.stacksize = size;
    if (size == 0)
    {
        p->yystack.s_base = NULL;
        p->yystack.l_base = NULL;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        p->yystack.p_base = NULL;
#endif
        return p;
    }
    p->yystack.s_base    = (YYINT *) malloc(size * sizeof(YYINT));
    if (p->yystack.s_base == NULL) return NULL;
    p->yystack.l_base    = (YYSTYPE *) malloc(size * sizeof(YYSTYPE));
    if (p->yystack.l_base == NULL) return NULL;
    memset(p->yystack.l_base, 0, size * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    p->yystack.p_base    = (YYLTYPE *) malloc(size * sizeof(YYLTYPE));
    if (p->yystack.p_base == NULL) return NULL;
    memset(p->yystack.p_base, 0, size * sizeof(YYLTYPE));
#endif

    return p;
}

static void
yyFreeState(YYParseState *p)
{
    yyfreestack(&p->yystack);
    free(p);
}
#endif /* YYBTYACC */

#define YYABORT  goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR  goto yyerrlab
#if YYBTYACC
#define YYVALID        do { if (yyps->save)            goto yyvalid; } while(0)
#define YYVALID_NESTED do { if (yyps->save && \
                                yyps->save->save == 0) goto yyvalid; } while(0)
#endif /* YYBTYACC */

int
YYPARSE_DECL()
{
    int yym, yyn, yystate, yyresult;
#if YYBTYACC
    int yynewerrflag;
    YYParseState *yyerrctx = NULL;
#endif /* YYBTYACC */
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    YYLTYPE  yyerror_loc_range[3]; /* position of error start/end (0 unused) */
#endif
#if YYDEBUG
    const char *yys;

    if ((yys = getenv("YYDEBUG")) != 0)
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
    if (yydebug)
        fprintf(stderr, "%sdebug[<# of symbols on state stack>]\n", YYPREFIX);
#endif
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    memset(yyerror_loc_range, 0, sizeof(yyerror_loc_range));
#endif

#if YYBTYACC
    yyps = yyNewState(0); if (yyps == 0) goto yyenomem;
    yyps->save = 0;
#endif /* YYBTYACC */
    yym = 0;
    yyn = 0;
    yynerrs = 0;
    yyerrflag = 0;
    yychar = YYEMPTY;
    yystate = 0;

#if YYPURE
    memset(&yystack, 0, sizeof(yystack));
#endif

    if (yystack.s_base == NULL && yygrowstack(&yystack) == YYENOMEM) goto yyoverflow;
    yystack.s_mark = yystack.s_base;
    yystack.l_mark = yystack.l_base;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yystack.p_mark = yystack.p_base;
#endif
    yystate = 0;
    *yystack.s_mark = 0;

yyloop:
    if ((yyn = yydefred[yystate]) != 0) goto yyreduce;
    if (yychar < 0)
    {
#if YYBTYACC
        do {
        if (yylvp < yylve)
        {
            /* we're currently re-reading tokens */
            yylval = *yylvp++;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            yylloc = *yylpp++;
#endif
            yychar = *yylexp++;
            break;
        }
        if (yyps->save)
        {
            /* in trial mode; save scanner results for future parse attempts */
            if (yylvp == yylvlim)
            {   /* Enlarge lexical value queue */
                size_t p = (size_t) (yylvp - yylvals);
                size_t s = (size_t) (yylvlim - yylvals);

                s += YYLVQUEUEGROWTH;
                if ((yylexemes = (YYINT *)realloc(yylexemes, s * sizeof(YYINT))) == NULL) goto yyenomem;
                if ((yylvals   = (YYSTYPE *)realloc(yylvals, s * sizeof(YYSTYPE))) == NULL) goto yyenomem;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                if ((yylpsns   = (YYLTYPE *)realloc(yylpsns, s * sizeof(YYLTYPE))) == NULL) goto yyenomem;
#endif
                yylvp   = yylve = yylvals + p;
                yylvlim = yylvals + s;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                yylpp   = yylpe = yylpsns + p;
                yylplim = yylpsns + s;
#endif
                yylexp  = yylexemes + p;
            }
            *yylexp = (YYINT) YYLEX;
            *yylvp++ = yylval;
            yylve++;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            *yylpp++ = yylloc;
            yylpe++;
#endif
            yychar = *yylexp++;
            break;
        }
        /* normal operation, no conflict encountered */
#endif /* YYBTYACC */
        yychar = YYLEX;
#if YYBTYACC
        } while (0);
#endif /* YYBTYACC */
        if (yychar < 0) yychar = YYEOF;
#if YYDEBUG
        if (yydebug)
        {
            if ((yys = yyname[YYTRANSLATE(yychar)]) == NULL) yys = yyname[YYUNDFTOKEN];
            fprintf(stderr, "%s[%d]: state %d, reading token %d (%s)",
                            YYDEBUGSTR, yydepth, yystate, yychar, yys);
#ifdef YYSTYPE_TOSTRING
#if YYBTYACC
            if (!yytrial)
#endif /* YYBTYACC */
                fprintf(stderr, " <%s>", YYSTYPE_TOSTRING(yychar, yylval));
#endif
            fputc('\n', stderr);
        }
#endif
    }
#if YYBTYACC

    /* Do we have a conflict? */
    if (((yyn = yycindex[yystate]) != 0) && (yyn += yychar) >= 0 &&
        yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) yychar)
    {
        YYINT ctry;

        if (yypath)
        {
            YYParseState *save;
#if YYDEBUG
            if (yydebug)
                fprintf(stderr, "%s[%d]: CONFLICT in state %d: following successful trial parse\n",
                                YYDEBUGSTR, yydepth, yystate);
#endif
            /* Switch to the next conflict context */
            save = yypath;
            yypath = save->save;
            save->save = NULL;
            ctry = save->ctry;
            if (save->state != yystate) YYABORT;
            yyFreeState(save);

        }
        else
        {

            /* Unresolved conflict - start/continue trial parse */
            YYParseState *save;
#if YYDEBUG
            if (yydebug)
            {
                fprintf(stderr, "%s[%d]: CONFLICT in state %d. ", YYDEBUGSTR, yydepth, yystate);
                if (yyps->save)
                    fputs("ALREADY in conflict, continuing trial parse.\n", stderr);
                else
                    fputs("Starting trial parse.\n", stderr);
            }
#endif
            save                  = yyNewState((unsigned)(yystack.s_mark - yystack.s_base + 1));
            if (save == NULL) goto yyenomem;
            save->save            = yyps->save;
            save->state           = yystate;
            save->errflag         = yyerrflag;
            save->yystack.s_mark  = save->yystack.s_base + (yystack.s_mark - yystack.s_base);
            memcpy (save->yystack.s_base, yystack.s_base, (size_t) (yystack.s_mark - yystack.s_base + 1) * sizeof(YYINT));
            save->yystack.l_mark  = save->yystack.l_base + (yystack.l_mark - yystack.l_base);
            memcpy (save->yystack.l_base, yystack.l_base, (size_t) (yystack.l_mark - yystack.l_base + 1) * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            save->yystack.p_mark  = save->yystack.p_base + (yystack.p_mark - yystack.p_base);
            memcpy (save->yystack.p_base, yystack.p_base, (size_t) (yystack.p_mark - yystack.p_base + 1) * sizeof(YYLTYPE));
#endif
            ctry                  = yytable[yyn];
            if (yyctable[ctry] == -1)
            {
#if YYDEBUG
                if (yydebug && yychar >= YYEOF)
                    fprintf(stderr, "%s[%d]: backtracking 1 token\n", YYDEBUGSTR, yydepth);
#endif
                ctry++;
            }
            save->ctry = ctry;
            if (yyps->save == NULL)
            {
                /* If this is a first conflict in the stack, start saving lexemes */
                if (!yylexemes)
                {
                    yylexemes = (YYINT *) malloc((YYLVQUEUEGROWTH) * sizeof(YYINT));
                    if (yylexemes == NULL) goto yyenomem;
                    yylvals   = (YYSTYPE *) malloc((YYLVQUEUEGROWTH) * sizeof(YYSTYPE));
                    if (yylvals == NULL) goto yyenomem;
                    yylvlim   = yylvals + YYLVQUEUEGROWTH;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                    yylpsns   = (YYLTYPE *) malloc((YYLVQUEUEGROWTH) * sizeof(YYLTYPE));
                    if (yylpsns == NULL) goto yyenomem;
                    yylplim   = yylpsns + YYLVQUEUEGROWTH;
#endif
                }
                if (yylvp == yylve)
                {
                    yylvp  = yylve = yylvals;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                    yylpp  = yylpe = yylpsns;
#endif
                    yylexp = yylexemes;
                    if (yychar >= YYEOF)
                    {
                        *yylve++ = yylval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                        *yylpe++ = yylloc;
#endif
                        *yylexp  = (YYINT) yychar;
                        yychar   = YYEMPTY;
                    }
                }
            }
            if (yychar >= YYEOF)
            {
                yylvp--;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                yylpp--;
#endif
                yylexp--;
                yychar = YYEMPTY;
            }
            save->lexeme = (int) (yylvp - yylvals);
            yyps->save   = save;
        }
        if (yytable[yyn] == ctry)
        {
#if YYDEBUG
            if (yydebug)
                fprintf(stderr, "%s[%d]: state %d, shifting to state %d\n",
                                YYDEBUGSTR, yydepth, yystate, yyctable[ctry]);
#endif
            if (yychar < 0)
            {
                yylvp++;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                yylpp++;
#endif
                yylexp++;
            }
            if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM)
                goto yyoverflow;
            yystate = yyctable[ctry];
            *++yystack.s_mark = (YYINT) yystate;
            *++yystack.l_mark = yylval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            *++yystack.p_mark = yylloc;
#endif
            yychar  = YYEMPTY;
            if (yyerrflag > 0) --yyerrflag;
            goto yyloop;
        }
        else
        {
            yyn = yyctable[ctry];
            goto yyreduce;
        }
    } /* End of code dealing with conflicts */
#endif /* YYBTYACC */
    if (((yyn = yysindex[yystate]) != 0) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) yychar)
    {
#if YYDEBUG
        if (yydebug)
            fprintf(stderr, "%s[%d]: state %d, shifting to state %d\n",
                            YYDEBUGSTR, yydepth, yystate, yytable[yyn]);
#endif
        if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM) goto yyoverflow;
        yystate = yytable[yyn];
        *++yystack.s_mark = yytable[yyn];
        *++yystack.l_mark = yylval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        *++yystack.p_mark = yylloc;
#endif
        yychar = YYEMPTY;
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if (((yyn = yyrindex[yystate]) != 0) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag != 0) goto yyinrecovery;
#if YYBTYACC

    yynewerrflag = 1;
    goto yyerrhandler;
    goto yyerrlab; /* redundant goto avoids 'unused label' warning */

yyerrlab:
    /* explicit YYERROR from an action -- pop the rhs of the rule reduced
     * before looking for error recovery */
    yystack.s_mark -= yym;
    yystate = *yystack.s_mark;
    yystack.l_mark -= yym;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yystack.p_mark -= yym;
#endif

    yynewerrflag = 0;
yyerrhandler:
    while (yyps->save)
    {
        int ctry;
        YYParseState *save = yyps->save;
#if YYDEBUG
        if (yydebug)
            fprintf(stderr, "%s[%d]: ERROR in state %d, CONFLICT BACKTRACKING to state %d, %d tokens\n",
                            YYDEBUGSTR, yydepth, yystate, yyps->save->state,
                    (int)(yylvp - yylvals - yyps->save->lexeme));
#endif
        /* Memorize most forward-looking error state in case it's really an error. */
        if (yyerrctx == NULL || yyerrctx->lexeme < yylvp - yylvals)
        {
            /* Free old saved error context state */
            if (yyerrctx) yyFreeState(yyerrctx);
            /* Create and fill out new saved error context state */
            yyerrctx                 = yyNewState((unsigned)(yystack.s_mark - yystack.s_base + 1));
            if (yyerrctx == NULL) goto yyenomem;
            yyerrctx->save           = yyps->save;
            yyerrctx->state          = yystate;
            yyerrctx->errflag        = yyerrflag;
            yyerrctx->yystack.s_mark = yyerrctx->yystack.s_base + (yystack.s_mark - yystack.s_base);
            memcpy (yyerrctx->yystack.s_base, yystack.s_base, (size_t) (yystack.s_mark - yystack.s_base + 1) * sizeof(YYINT));
            yyerrctx->yystack.l_mark = yyerrctx->yystack.l_base + (yystack.l_mark - yystack.l_base);
            memcpy (yyerrctx->yystack.l_base, yystack.l_base, (size_t) (yystack.l_mark - yystack.l_base + 1) * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            yyerrctx->yystack.p_mark = yyerrctx->yystack.p_base + (yystack.p_mark - yystack.p_base);
            memcpy (yyerrctx->yystack.p_base, yystack.p_base, (size_t) (yystack.p_mark - yystack.p_base + 1) * sizeof(YYLTYPE));
#endif
            yyerrctx->lexeme         = (int) (yylvp - yylvals);
        }
        yylvp          = yylvals   + save->lexeme;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        yylpp          = yylpsns   + save->lexeme;
#endif
        yylexp         = yylexemes + save->lexeme;
        yychar         = YYEMPTY;
        yystack.s_mark = yystack.s_base + (save->yystack.s_mark - save->yystack.s_base);
        memcpy (yystack.s_base, save->yystack.s_base, (size_t) (yystack.s_mark - yystack.s_base + 1) * sizeof(YYINT));
        yystack.l_mark = yystack.l_base + (save->yystack.l_mark - save->yystack.l_base);
        memcpy (yystack.l_base, save->yystack.l_base, (size_t) (yystack.l_mark - yystack.l_base + 1) * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        yystack.p_mark = yystack.p_base + (save->yystack.p_mark - save->yystack.p_base);
        memcpy (yystack.p_base, save->yystack.p_base, (size_t) (yystack.p_mark - yystack.p_base + 1) * sizeof(YYLTYPE));
#endif
        ctry           = ++save->ctry;
        yystate        = save->state;
        /* We tried shift, try reduce now */
        if ((yyn = yyctable[ctry]) >= 0) goto yyreduce;
        yyps->save     = save->save;
        save->save     = NULL;
        yyFreeState(save);

        /* Nothing left on the stack -- error */
        if (!yyps->save)
        {
#if YYDEBUG
            if (yydebug)
                fprintf(stderr, "%sdebug[%d,trial]: trial parse FAILED, entering ERROR mode\n",
                                YYPREFIX, yydepth);
#endif
            /* Restore state as it was in the most forward-advanced error */
            yylvp          = yylvals   + yyerrctx->lexeme;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            yylpp          = yylpsns   + yyerrctx->lexeme;
#endif
            yylexp         = yylexemes + yyerrctx->lexeme;
            yychar         = yylexp[-1];
            yylval         = yylvp[-1];
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            yylloc         = yylpp[-1];
#endif
            yystack.s_mark = yystack.s_base + (yyerrctx->yystack.s_mark - yyerrctx->yystack.s_base);
            memcpy (yystack.s_base, yyerrctx->yystack.s_base, (size_t) (yystack.s_mark - yystack.s_base + 1) * sizeof(YYINT));
            yystack.l_mark = yystack.l_base + (yyerrctx->yystack.l_mark - yyerrctx->yystack.l_base);
            memcpy (yystack.l_base, yyerrctx->yystack.l_base, (size_t) (yystack.l_mark - yystack.l_base + 1) * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            yystack.p_mark = yystack.p_base + (yyerrctx->yystack.p_mark - yyerrctx->yystack.p_base);
            memcpy (yystack.p_base, yyerrctx->yystack.p_base, (size_t) (yystack.p_mark - yystack.p_base + 1) * sizeof(YYLTYPE));
#endif
            yystate        = yyerrctx->state;
            yyFreeState(yyerrctx);
            yyerrctx       = NULL;
        }
        yynewerrflag = 1;
    }
    if (yynewerrflag == 0) goto yyinrecovery;
#endif /* YYBTYACC */

    YYERROR_CALL("syntax error");
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yyerror_loc_range[1] = yylloc; /* lookahead position is error start position */
#endif

#if !YYBTYACC
    goto yyerrlab; /* redundant goto avoids 'unused label' warning */
yyerrlab:
#endif
    ++yynerrs;

yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if (((yyn = yysindex[*yystack.s_mark]) != 0) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    fprintf(stderr, "%s[%d]: state %d, error recovery shifting to state %d\n",
                                    YYDEBUGSTR, yydepth, *yystack.s_mark, yytable[yyn]);
#endif
                if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM) goto yyoverflow;
                yystate = yytable[yyn];
                *++yystack.s_mark = yytable[yyn];
                *++yystack.l_mark = yylval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                /* lookahead position is error end position */
                yyerror_loc_range[2] = yylloc;
                YYLLOC_DEFAULT(yyloc, yyerror_loc_range, 2); /* position of error span */
                *++yystack.p_mark = yyloc;
#endif
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    fprintf(stderr, "%s[%d]: error recovery discarding state %d\n",
                                    YYDEBUGSTR, yydepth, *yystack.s_mark);
#endif
                if (yystack.s_mark <= yystack.s_base) goto yyabort;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                /* the current TOS position is the error start position */
                yyerror_loc_range[1] = *yystack.p_mark;
#endif
#if defined(YYDESTRUCT_CALL)
#if YYBTYACC
                if (!yytrial)
#endif /* YYBTYACC */
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                    YYDESTRUCT_CALL("error: discarding state",
                                    yystos[*yystack.s_mark], yystack.l_mark, yystack.p_mark);
#else
                    YYDESTRUCT_CALL("error: discarding state",
                                    yystos[*yystack.s_mark], yystack.l_mark);
#endif /* defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED) */
#endif /* defined(YYDESTRUCT_CALL) */
                --yystack.s_mark;
                --yystack.l_mark;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                --yystack.p_mark;
#endif
            }
        }
    }
    else
    {
        if (yychar == YYEOF) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            if ((yys = yyname[YYTRANSLATE(yychar)]) == NULL) yys = yyname[YYUNDFTOKEN];
            fprintf(stderr, "%s[%d]: state %d, error recovery discarding token %d (%s)\n",
                            YYDEBUGSTR, yydepth, yystate, yychar, yys);
        }
#endif
#if defined(YYDESTRUCT_CALL)
#if YYBTYACC
        if (!yytrial)
#endif /* YYBTYACC */
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            YYDESTRUCT_CALL("error: discarding token", yychar, &yylval, &yylloc);
#else
            YYDESTRUCT_CALL("error: discarding token", yychar, &yylval);
#endif /* defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED) */
#endif /* defined(YYDESTRUCT_CALL) */
        yychar = YYEMPTY;
        goto yyloop;
    }

yyreduce:
    yym = yylen[yyn];
#if YYDEBUG
    if (yydebug)
    {
        fprintf(stderr, "%s[%d]: state %d, reducing by rule %d (%s)",
                        YYDEBUGSTR, yydepth, yystate, yyn, yyrule[yyn]);
#ifdef YYSTYPE_TOSTRING
#if YYBTYACC
        if (!yytrial)
#endif /* YYBTYACC */
            if (yym > 0)
            {
                int i;
                fputc('<', stderr);
                for (i = yym; i > 0; i--)
                {
                    if (i != yym) fputs(", ", stderr);
                    fputs(YYSTYPE_TOSTRING(yystos[yystack.s_mark[1-i]],
                                           yystack.l_mark[1-i]), stderr);
                }
                fputc('>', stderr);
            }
#endif
        fputc('\n', stderr);
    }
#endif
    if (yym > 0)
        yyval = yystack.l_mark[1-yym];
    else
        memset(&yyval, 0, sizeof yyval);
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)

    /* Perform position reduction */
    memset(&yyloc, 0, sizeof(yyloc));
#if YYBTYACC
    if (!yytrial)
#endif /* YYBTYACC */
    {
        YYLLOC_DEFAULT(yyloc, &yystack.p_mark[-yym], yym);
        /* just in case YYERROR is invoked within the action, save
           the start of the rhs as the error start position */
        yyerror_loc_range[1] = yystack.p_mark[1-yym];
    }
#endif

    switch (yyn)
    {
case 1:
#line 354 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{
		  /* If we are completing and see a valid parse,
		     rust_ast will already have been set.  */
		  if (rust_ast == NULL)
		    rust_ast = yystack.l_mark[0].op;
		}
break;
case 15:
#line 383 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{
		  VEC_safe_insert (rust_op_ptr, *yystack.l_mark[-1].params, 0, yystack.l_mark[-3].op);
		  error (_("Tuple expressions not supported yet"));
		}
break;
case 16:
#line 391 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{
		  struct typed_val_int val;

		  val.type
		    = language_lookup_primitive_type (parse_language (pstate),
						      parse_gdbarch (pstate),
						      "()");
		  val.val = 0;
		  yyval.op = ast_literal (val);
		}
break;
case 17:
#line 408 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_struct (yystack.l_mark[-3].op, yystack.l_mark[-1].field_inits); }
break;
case 18:
#line 413 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{
		  struct set_field sf;

		  sf.name.ptr = NULL;
		  sf.name.length = 0;
		  sf.init = yystack.l_mark[0].op;

		  yyval.one_field_init = sf;
		}
break;
case 19:
#line 423 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{
		  struct set_field sf;

		  sf.name = yystack.l_mark[-2].sval;
		  sf.init = yystack.l_mark[0].op;
		  yyval.one_field_init = sf;
		}
break;
case 20:
#line 434 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{
		  VEC (set_field) **result
		    = OBSTACK_ZALLOC (&work_obstack, VEC (set_field) *);
		  yyval.field_inits = result;
		}
break;
case 21:
#line 440 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{
		  VEC (set_field) **result
		    = OBSTACK_ZALLOC (&work_obstack, VEC (set_field) *);

		  make_cleanup (VEC_cleanup (set_field), result);
		  VEC_safe_push (set_field, *result, &yystack.l_mark[0].one_field_init);

		  yyval.field_inits = result;
		}
break;
case 22:
#line 450 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{
		  struct set_field sf;

		  sf.name = yystack.l_mark[-4].sval;
		  sf.init = yystack.l_mark[-2].op;
		  VEC_safe_push (set_field, *yystack.l_mark[0].field_inits, &sf);
		  yyval.field_inits = yystack.l_mark[0].field_inits;
		}
break;
case 23:
#line 462 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_call_ish (OP_ARRAY, NULL, yystack.l_mark[-1].params); }
break;
case 24:
#line 464 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_call_ish (OP_ARRAY, NULL, yystack.l_mark[-1].params); }
break;
case 25:
#line 466 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (OP_RUST_ARRAY, yystack.l_mark[-3].op, yystack.l_mark[-1].op); }
break;
case 26:
#line 468 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (OP_RUST_ARRAY, yystack.l_mark[-3].op, yystack.l_mark[-1].op); }
break;
case 27:
#line 473 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_range (yystack.l_mark[-1].op, NULL); }
break;
case 28:
#line 475 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_range (yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 29:
#line 477 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_range (NULL, yystack.l_mark[0].op); }
break;
case 30:
#line 479 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_range (NULL, NULL); }
break;
case 31:
#line 484 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_literal (yystack.l_mark[0].typed_val_int); }
break;
case 32:
#line 486 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_literal (yystack.l_mark[0].typed_val_int); }
break;
case 33:
#line 488 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_dliteral (yystack.l_mark[0].typed_val_float); }
break;
case 34:
#line 490 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{
		  const struct rust_op *str = ast_string (yystack.l_mark[0].sval);
		  VEC (set_field) **fields;
		  struct set_field field;
		  struct typed_val_int val;
		  struct stoken token;

		  fields = OBSTACK_ZALLOC (&work_obstack, VEC (set_field) *);
		  make_cleanup (VEC_cleanup (set_field), fields);

		  /* Wrap the raw string in the &str struct.  */
		  field.name.ptr = "data_ptr";
		  field.name.length = strlen (field.name.ptr);
		  field.init = ast_unary (UNOP_ADDR, ast_string (yystack.l_mark[0].sval));
		  VEC_safe_push (set_field, *fields, &field);

		  val.type = rust_type ("usize");
		  val.val = yystack.l_mark[0].sval.length;

		  field.name.ptr = "length";
		  field.name.length = strlen (field.name.ptr);
		  field.init = ast_literal (val);
		  VEC_safe_push (set_field, *fields, &field);

		  token.ptr = "&str";
		  token.length = strlen (token.ptr);
		  yyval.op = ast_struct (ast_path (token, NULL), fields);
		}
break;
case 35:
#line 519 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_string (yystack.l_mark[0].sval); }
break;
case 36:
#line 521 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{
		  struct typed_val_int val;

		  val.type = language_bool_type (parse_language (pstate),
						 parse_gdbarch (pstate));
		  val.val = 1;
		  yyval.op = ast_literal (val);
		}
break;
case 37:
#line 530 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{
		  struct typed_val_int val;

		  val.type = language_bool_type (parse_language (pstate),
						 parse_gdbarch (pstate));
		  val.val = 0;
		  yyval.op = ast_literal (val);
		}
break;
case 38:
#line 542 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_structop (yystack.l_mark[-2].op, yystack.l_mark[0].sval.ptr, 0); }
break;
case 39:
#line 544 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{
		  yyval.op = ast_structop (yystack.l_mark[-2].op, yystack.l_mark[0].sval.ptr, 1);
		  rust_ast = yyval.op;
		}
break;
case 40:
#line 549 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_structop_anonymous (yystack.l_mark[-2].op, yystack.l_mark[0].typed_val_int); }
break;
case 41:
#line 554 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_SUBSCRIPT, yystack.l_mark[-3].op, yystack.l_mark[-1].op); }
break;
case 42:
#line 559 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_unary (UNOP_PLUS, yystack.l_mark[0].op); }
break;
case 43:
#line 562 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_unary (UNOP_NEG, yystack.l_mark[0].op); }
break;
case 44:
#line 565 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{
		  /* Note that we provide a Rust-specific evaluator
		     override for UNOP_COMPLEMENT, so it can do the
		     right thing for both bool and integral
		     values.  */
		  yyval.op = ast_unary (UNOP_COMPLEMENT, yystack.l_mark[0].op);
		}
break;
case 45:
#line 574 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_unary (UNOP_IND, yystack.l_mark[0].op); }
break;
case 46:
#line 577 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_unary (UNOP_ADDR, yystack.l_mark[0].op); }
break;
case 47:
#line 580 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_unary (UNOP_ADDR, yystack.l_mark[0].op); }
break;
case 48:
#line 582 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_unary (UNOP_SIZEOF, yystack.l_mark[-1].op); }
break;
case 53:
#line 594 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_MUL, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 54:
#line 597 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_REPEAT, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 55:
#line 600 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_DIV, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 56:
#line 603 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_REM, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 57:
#line 606 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_LESS, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 58:
#line 609 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_GTR, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 59:
#line 612 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_BITWISE_AND, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 60:
#line 615 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_BITWISE_IOR, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 61:
#line 618 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_BITWISE_XOR, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 62:
#line 621 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_ADD, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 63:
#line 624 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_SUB, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 64:
#line 627 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_LOGICAL_OR, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 65:
#line 630 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_LOGICAL_AND, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 66:
#line 633 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_EQUAL, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 67:
#line 636 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_NOTEQUAL, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 68:
#line 639 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_LEQ, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 69:
#line 642 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_GEQ, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 70:
#line 645 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_LSH, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 71:
#line 648 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_RSH, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 72:
#line 653 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_cast (yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 73:
#line 658 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_ASSIGN, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 74:
#line 663 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_compound_assignment (yystack.l_mark[-1].opcode, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 75:
#line 669 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = yystack.l_mark[-1].op; }
break;
case 76:
#line 674 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{
		  yyval.params = OBSTACK_ZALLOC (&work_obstack, VEC (rust_op_ptr) *);
		  make_cleanup (VEC_cleanup (rust_op_ptr), yyval.params);
		  VEC_safe_push (rust_op_ptr, *yyval.params, yystack.l_mark[0].op);
		}
break;
case 77:
#line 680 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{
		  VEC_safe_push (rust_op_ptr, *yystack.l_mark[-2].params, yystack.l_mark[0].op);
		  yyval.params = yystack.l_mark[-2].params;
		}
break;
case 78:
#line 688 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{
		  /* The result can't be NULL.  */
		  yyval.params = OBSTACK_ZALLOC (&work_obstack, VEC (rust_op_ptr) *);
		  make_cleanup (VEC_cleanup (rust_op_ptr), yyval.params);
		}
break;
case 79:
#line 694 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.params = yystack.l_mark[0].params; }
break;
case 80:
#line 701 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.params = yystack.l_mark[-1].params; }
break;
case 81:
#line 706 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_call_ish (OP_FUNCALL, yystack.l_mark[-1].op, yystack.l_mark[0].params); }
break;
case 84:
#line 716 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.depth = 1; }
break;
case 85:
#line 718 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.depth = yystack.l_mark[-2].depth + 1; }
break;
case 86:
#line 723 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = yystack.l_mark[0].op; }
break;
case 87:
#line 725 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_path (yystack.l_mark[0].sval, NULL); }
break;
case 88:
#line 727 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_path (make_stoken ("self"), NULL); }
break;
case 90:
#line 733 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = super_name (yystack.l_mark[0].op, 0); }
break;
case 91:
#line 735 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = super_name (yystack.l_mark[0].op, yystack.l_mark[-1].depth); }
break;
case 92:
#line 737 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = crate_name (yystack.l_mark[0].op); }
break;
case 93:
#line 739 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{
		  /* This is a gdb extension to make it possible to
		     refer to items in other crates.  It just bypasses
		     adding the current crate to the front of the
		     name.  */
		  yyval.op = ast_path (rust_concat3 ("::", yystack.l_mark[0].op->left.sval.ptr, NULL),
				 yystack.l_mark[0].op->right.params);
		}
break;
case 94:
#line 751 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_path (yystack.l_mark[0].sval, NULL); }
break;
case 95:
#line 753 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{
		  yyval.op = ast_path (rust_concat3 (yystack.l_mark[-2].op->left.sval.ptr, "::",
					       yystack.l_mark[0].sval.ptr),
				 NULL);
		}
break;
case 96:
#line 759 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_path (yystack.l_mark[-4].op->left.sval, yystack.l_mark[-1].params); }
break;
case 97:
#line 761 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{
		  yyval.op = ast_path (yystack.l_mark[-4].op->left.sval, yystack.l_mark[-1].params);
		  rust_push_back ('>');
		}
break;
case 99:
#line 770 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = super_name (yystack.l_mark[0].op, 0); }
break;
case 100:
#line 772 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = super_name (yystack.l_mark[0].op, yystack.l_mark[-1].depth); }
break;
case 101:
#line 774 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = crate_name (yystack.l_mark[0].op); }
break;
case 102:
#line 776 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{
		  /* This is a gdb extension to make it possible to
		     refer to items in other crates.  It just bypasses
		     adding the current crate to the front of the
		     name.  */
		  yyval.op = ast_path (rust_concat3 ("::", yystack.l_mark[0].op->left.sval.ptr, NULL),
				 yystack.l_mark[0].op->right.params);
		}
break;
case 103:
#line 788 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_path (yystack.l_mark[0].sval, NULL); }
break;
case 104:
#line 790 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{
		  yyval.op = ast_path (rust_concat3 (yystack.l_mark[-2].op->left.sval.ptr, "::",
					       yystack.l_mark[0].sval.ptr),
				 NULL);
		}
break;
case 106:
#line 800 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_path (yystack.l_mark[-3].op->left.sval, yystack.l_mark[-1].params); }
break;
case 107:
#line 802 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{
		  yyval.op = ast_path (yystack.l_mark[-3].op->left.sval, yystack.l_mark[-1].params);
		  rust_push_back ('>');
		}
break;
case 109:
#line 811 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_array_type (yystack.l_mark[-3].op, yystack.l_mark[-1].typed_val_int); }
break;
case 110:
#line 813 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_array_type (yystack.l_mark[-3].op, yystack.l_mark[-1].typed_val_int); }
break;
case 111:
#line 815 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_slice_type (yystack.l_mark[-1].op); }
break;
case 112:
#line 817 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_reference_type (yystack.l_mark[0].op); }
break;
case 113:
#line 819 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_pointer_type (yystack.l_mark[0].op, 1); }
break;
case 114:
#line 821 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_pointer_type (yystack.l_mark[0].op, 0); }
break;
case 115:
#line 823 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_function_type (yystack.l_mark[0].op, yystack.l_mark[-3].params); }
break;
case 116:
#line 825 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.op = ast_tuple_type (yystack.l_mark[-1].params); }
break;
case 117:
#line 830 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.params = NULL; }
break;
case 118:
#line 832 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{ yyval.params = yystack.l_mark[0].params; }
break;
case 119:
#line 837 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{
		  VEC (rust_op_ptr) **result
		    = OBSTACK_ZALLOC (&work_obstack, VEC (rust_op_ptr) *);

		  make_cleanup (VEC_cleanup (rust_op_ptr), result);
		  VEC_safe_push (rust_op_ptr, *result, yystack.l_mark[0].op);
		  yyval.params = result;
		}
break;
case 120:
#line 846 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.y"
	{
		  VEC_safe_push (rust_op_ptr, *yystack.l_mark[-2].params, yystack.l_mark[0].op);
		  yyval.params = yystack.l_mark[-2].params;
		}
break;
#line 4713 "/p/netbsd/cvsroot/src/external/gpl3/gdb.old/lib/libgdb/../../dist/gdb/rust-exp.c"
    default:
        break;
    }
    yystack.s_mark -= yym;
    yystate = *yystack.s_mark;
    yystack.l_mark -= yym;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yystack.p_mark -= yym;
#endif
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
        {
            fprintf(stderr, "%s[%d]: after reduction, ", YYDEBUGSTR, yydepth);
#ifdef YYSTYPE_TOSTRING
#if YYBTYACC
            if (!yytrial)
#endif /* YYBTYACC */
                fprintf(stderr, "result is <%s>, ", YYSTYPE_TOSTRING(yystos[YYFINAL], yyval));
#endif
            fprintf(stderr, "shifting from state 0 to final state %d\n", YYFINAL);
        }
#endif
        yystate = YYFINAL;
        *++yystack.s_mark = YYFINAL;
        *++yystack.l_mark = yyval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        *++yystack.p_mark = yyloc;
#endif
        if (yychar < 0)
        {
#if YYBTYACC
            do {
            if (yylvp < yylve)
            {
                /* we're currently re-reading tokens */
                yylval = *yylvp++;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                yylloc = *yylpp++;
#endif
                yychar = *yylexp++;
                break;
            }
            if (yyps->save)
            {
                /* in trial mode; save scanner results for future parse attempts */
                if (yylvp == yylvlim)
                {   /* Enlarge lexical value queue */
                    size_t p = (size_t) (yylvp - yylvals);
                    size_t s = (size_t) (yylvlim - yylvals);

                    s += YYLVQUEUEGROWTH;
                    if ((yylexemes = (YYINT *)realloc(yylexemes, s * sizeof(YYINT))) == NULL)
                        goto yyenomem;
                    if ((yylvals   = (YYSTYPE *)realloc(yylvals, s * sizeof(YYSTYPE))) == NULL)
                        goto yyenomem;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                    if ((yylpsns   = (YYLTYPE *)realloc(yylpsns, s * sizeof(YYLTYPE))) == NULL)
                        goto yyenomem;
#endif
                    yylvp   = yylve = yylvals + p;
                    yylvlim = yylvals + s;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                    yylpp   = yylpe = yylpsns + p;
                    yylplim = yylpsns + s;
#endif
                    yylexp  = yylexemes + p;
                }
                *yylexp = (YYINT) YYLEX;
                *yylvp++ = yylval;
                yylve++;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                *yylpp++ = yylloc;
                yylpe++;
#endif
                yychar = *yylexp++;
                break;
            }
            /* normal operation, no conflict encountered */
#endif /* YYBTYACC */
            yychar = YYLEX;
#if YYBTYACC
            } while (0);
#endif /* YYBTYACC */
            if (yychar < 0) yychar = YYEOF;
#if YYDEBUG
            if (yydebug)
            {
                if ((yys = yyname[YYTRANSLATE(yychar)]) == NULL) yys = yyname[YYUNDFTOKEN];
                fprintf(stderr, "%s[%d]: state %d, reading token %d (%s)\n",
                                YYDEBUGSTR, yydepth, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == YYEOF) goto yyaccept;
        goto yyloop;
    }
    if (((yyn = yygindex[yym]) != 0) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
    {
        fprintf(stderr, "%s[%d]: after reduction, ", YYDEBUGSTR, yydepth);
#ifdef YYSTYPE_TOSTRING
#if YYBTYACC
        if (!yytrial)
#endif /* YYBTYACC */
            fprintf(stderr, "result is <%s>, ", YYSTYPE_TOSTRING(yystos[yystate], yyval));
#endif
        fprintf(stderr, "shifting from state %d to state %d\n", *yystack.s_mark, yystate);
    }
#endif
    if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM) goto yyoverflow;
    *++yystack.s_mark = (YYINT) yystate;
    *++yystack.l_mark = yyval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    *++yystack.p_mark = yyloc;
#endif
    goto yyloop;
#if YYBTYACC

    /* Reduction declares that this path is valid. Set yypath and do a full parse */
yyvalid:
    if (yypath) YYABORT;
    while (yyps->save)
    {
        YYParseState *save = yyps->save;
        yyps->save = save->save;
        save->save = yypath;
        yypath = save;
    }
#if YYDEBUG
    if (yydebug)
        fprintf(stderr, "%s[%d]: state %d, CONFLICT trial successful, backtracking to state %d, %d tokens\n",
                        YYDEBUGSTR, yydepth, yystate, yypath->state, (int)(yylvp - yylvals - yypath->lexeme));
#endif
    if (yyerrctx)
    {
        yyFreeState(yyerrctx);
        yyerrctx = NULL;
    }
    yylvp          = yylvals + yypath->lexeme;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yylpp          = yylpsns + yypath->lexeme;
#endif
    yylexp         = yylexemes + yypath->lexeme;
    yychar         = YYEMPTY;
    yystack.s_mark = yystack.s_base + (yypath->yystack.s_mark - yypath->yystack.s_base);
    memcpy (yystack.s_base, yypath->yystack.s_base, (size_t) (yystack.s_mark - yystack.s_base + 1) * sizeof(YYINT));
    yystack.l_mark = yystack.l_base + (yypath->yystack.l_mark - yypath->yystack.l_base);
    memcpy (yystack.l_base, yypath->yystack.l_base, (size_t) (yystack.l_mark - yystack.l_base + 1) * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yystack.p_mark = yystack.p_base + (yypath->yystack.p_mark - yypath->yystack.p_base);
    memcpy (yystack.p_base, yypath->yystack.p_base, (size_t) (yystack.p_mark - yystack.p_base + 1) * sizeof(YYLTYPE));
#endif
    yystate        = yypath->state;
    goto yyloop;
#endif /* YYBTYACC */

yyoverflow:
    YYERROR_CALL("yacc stack overflow");
#if YYBTYACC
    goto yyabort_nomem;
yyenomem:
    YYERROR_CALL("memory exhausted");
yyabort_nomem:
#endif /* YYBTYACC */
    yyresult = 2;
    goto yyreturn;

yyabort:
    yyresult = 1;
    goto yyreturn;

yyaccept:
#if YYBTYACC
    if (yyps->save) goto yyvalid;
#endif /* YYBTYACC */
    yyresult = 0;

yyreturn:
#if defined(YYDESTRUCT_CALL)
    if (yychar != YYEOF && yychar != YYEMPTY)
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        YYDESTRUCT_CALL("cleanup: discarding token", yychar, &yylval, &yylloc);
#else
        YYDESTRUCT_CALL("cleanup: discarding token", yychar, &yylval);
#endif /* defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED) */

    {
        YYSTYPE *pv;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        YYLTYPE *pp;

        for (pv = yystack.l_base, pp = yystack.p_base; pv <= yystack.l_mark; ++pv, ++pp)
             YYDESTRUCT_CALL("cleanup: discarding state",
                             yystos[*(yystack.s_base + (pv - yystack.l_base))], pv, pp);
#else
        for (pv = yystack.l_base; pv <= yystack.l_mark; ++pv)
             YYDESTRUCT_CALL("cleanup: discarding state",
                             yystos[*(yystack.s_base + (pv - yystack.l_base))], pv);
#endif /* defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED) */
    }
#endif /* defined(YYDESTRUCT_CALL) */

#if YYBTYACC
    if (yyerrctx)
    {
        yyFreeState(yyerrctx);
        yyerrctx = NULL;
    }
    while (yyps)
    {
        YYParseState *save = yyps;
        yyps = save->save;
        save->save = NULL;
        yyFreeState(save);
    }
    while (yypath)
    {
        YYParseState *save = yypath;
        yypath = save->save;
        save->save = NULL;
        yyFreeState(save);
    }
#endif /* YYBTYACC */
    yyfreestack(&yystack);
    return (yyresult);
}
