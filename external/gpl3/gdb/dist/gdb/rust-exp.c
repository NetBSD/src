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

#line 23 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"

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

#line 196 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
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
#line 229 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"

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

#line 262 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.c"

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
#define DOTDOT 276
#define OROR 277
#define ANDAND 278
#define EQEQ 279
#define NOTEQ 280
#define LTEQ 281
#define GTEQ 282
#define LSH 283
#define RSH 284
#define COLONCOLON 285
#define ARROW 286
#define UNARY 287
#define YYERRCODE 256
typedef int YYINT;
static const YYINT yylhs[] = {                           -1,
    0,   11,   11,   11,   11,   11,   11,   11,   11,   11,
   11,   11,   11,   11,   23,   24,   25,   32,   32,   31,
   31,   31,   26,   26,   26,   26,   27,   27,   27,   27,
   10,   10,   10,   10,   10,   10,   10,   12,   12,   12,
   13,   14,   14,   14,   14,   14,   14,   15,   15,   15,
   15,   16,   16,   16,   16,   16,   16,   16,   16,   16,
   16,   16,   16,   16,   16,   16,   16,   16,   16,   16,
   17,   18,   19,   20,   28,   28,   29,   29,   30,   21,
   33,   33,    9,    9,   22,   22,   22,    2,    2,    2,
    2,    2,    3,    3,    3,    3,    4,    4,    4,    4,
    4,    6,    6,    5,    5,    5,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    7,    7,    8,    8,
};
static const YYINT yylen[] = {                            2,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    5,    2,    4,    2,    3,    0,
    1,    5,    4,    3,    6,    5,    2,    3,    2,    1,
    1,    1,    1,    1,    1,    1,    1,    3,    3,    3,
    4,    2,    2,    2,    2,    2,    3,    1,    1,    1,
    1,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    1,    3,    0,    1,    3,    2,
    0,    2,    2,    3,    1,    1,    1,    1,    3,    3,
    2,    2,    1,    3,    5,    5,    1,    3,    3,    2,
    2,    1,    3,    1,    4,    4,    1,    5,    5,    4,
    2,    3,    3,    6,    3,    0,    1,    1,    3,
};
static const YYINT yydefred[] = {                         0,
   86,   93,   31,   32,   34,   35,   33,   36,   37,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    2,    0,    7,    9,   11,   12,   48,
   49,   50,   51,   13,   14,    3,    4,    5,    6,    8,
   10,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   16,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   80,    0,    0,    0,    0,
    0,    0,    0,    0,   24,    0,   74,    0,    0,    0,
   21,   94,    0,    0,  102,    0,    0,    0,    0,    0,
    0,    0,    0,   71,  107,   97,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   38,   39,   40,    0,    0,    0,   83,    0,    0,    0,
   23,    0,    0,    0,    0,    0,   17,  118,    0,    0,
  101,    0,  100,    0,  111,    0,    0,    0,    0,    0,
    0,    0,    0,   41,   79,   84,    0,   26,   15,    0,
   96,   95,    0,   98,    0,    0,  112,  113,    0,  115,
  103,    0,   99,   25,    0,  119,    0,  110,    0,    0,
  106,  105,   22,    0,  108,  109,  114,
};
#if defined(YYDESTRUCT_CALL) || defined(YYSTYPE_TOSTRING)
static const YYINT yystos[] = {                           0,
  257,  258,  260,  261,  262,  263,  264,  268,  269,  271,
  273,  276,  285,   38,   43,   45,   42,   91,   40,   33,
  289,  291,  292,  299,  300,  301,  302,  303,  304,  305,
  306,  307,  308,  309,  310,  311,  312,  313,  314,  315,
  316,  322,  285,  292,  300,  292,  272,  300,  300,  300,
  300,  272,  300,  317,   41,  300,  300,  123,  285,  265,
  266,  276,  277,  278,  279,  280,  281,  282,  283,  284,
   61,   60,   62,  124,   94,   38,   64,   43,   45,   42,
   47,   37,   91,   46,   40,  319,  270,  298,  292,  300,
  300,  317,   59,   44,   93,   44,   41,  258,  276,  320,
  321,  258,   60,  300,  258,  271,  273,  275,  285,   38,
   42,   91,   40,  290,  293,  294,  295,  322,  300,  300,
  300,  300,  300,  300,  300,  300,  300,  300,  300,  300,
  300,  300,  300,  300,  300,  300,  300,  300,  300,  300,
  258,  259,  261,  300,  317,  318,  285,  270,  292,   59,
   93,  300,  300,  318,   58,  300,  125,  290,  297,  285,
  294,   40,  294,   91,  290,  272,  274,  290,  296,  297,
  285,   60,  298,   93,   41,  285,  300,   93,   41,  300,
  284,   62,   44,  294,  296,  290,  290,  290,   59,   41,
  258,  297,  294,   93,   44,  290,   41,   93,  260,  261,
  284,   62,  320,  286,   93,   93,  290,
};
#endif /* YYDESTRUCT_CALL || YYSTYPE_TOSTRING */
static const YYINT yydgoto[] = {                         21,
  158,   22,   23,  115,  116,  117,  169,  170,   88,   24,
  144,   26,   27,   28,   29,   30,   31,   32,   33,   34,
   35,   36,   37,   38,   39,   40,   41,  145,  146,   86,
  100,  101,   42,
};
static const YYINT yysindex[] = {                      1701,
    0,    0,    0,    0,    0,    0,    0,    0,    0, -282,
 -218, 1740, -218,  392, 1701, 1701, 1701, 1395, 1681, 1701,
    0,  -40, -205,    0, 2199,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0, -191, -218, -205, 2290, -205, 1701,   11,   11,   11,
   11, 1701, 1836,   -9,    0, 1753,   11, -194,  -43, 1701,
  -10, 1740, 1701, 1701, 1701, 1701, 1701, 1701, 1701, 1701,
 1701, 1701, 1701, 1701, 1701, 1701, 1701, 1701, 1701, 1701,
 1701, 1701, 1701, -140, 1701,    0, -182, -173, -205,   11,
 2001,   -7, 1701, 1701,    0, 1701,    0,   49, 1701,   -8,
    0,    0,  -10, 2290,    0, -141, -127,  105, -127,  784,
 -248,  -10,  -10,    0,    0,    0,  -49, -191, 2290, 2451,
 2486, 1710, 1710, 1710, 1710,  295,  295, 2290, 1710, 1710,
 2541,  -33,  698,  362,  480,  480,  -13,  -13,  -13, 2036,
    0,    0,    0, 2199,  102,  106,    0, -137, -205, 1701,
    0, 2094, 2199,  108, 1701, 2199,    0,    0,  -44, -127,
    0,  -10,    0,  -10,    0,  -10,  -10,   91,  110,  120,
  -93,  -10, -154,    0,    0,    0, 2129,    0,    0, 2164,
    0,    0,  -10,    0,  125,  -37,    0,    0, -206,    0,
    0,  -42,    0,    0, -194,    0, -119,    0,   75,   76,
    0,    0,    0,  -10,    0,    0,    0,
};
static const YYINT yyrindex[] = {                      -100,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  163,
    0,  988,    0, -100, -100, -100, -100, -100, -100, -100,
    0,  420,    1,    0,  171,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,  -96,   29,  211,   68, -100,  487,  515,  554,
  582, -100,    6,    0,    0,    0,  808,   58,    0, -100,
 -100, 1215, -100, -100, -100, -100, -100, -100, -100, -100,
 -100, -100, -100, -100, -100, -100, -100, -100, -100, -100,
 -100, -100, -100,    0,  -35,    0,    0,    0,   96,  844,
    6,    0, -100, -100,    0,  -35,    0,    0, -100,    0,
    0,    0, -100,  230,    0,    0,    0,    0,    0, -100,
    0, -100,  -26,    0,    0,    0,  448,    0,  410, 1180,
  854,  890,  926, 1576, 1580, 1318, 1328,  232, 1584, 1590,
 1489, 1417, 1410, 1291, 1240, 1282,  880,  916,  952,    0,
    0,    0,    0,  -25,  143,    0,    0,    0,  135, -100,
    0,    0,    8,    0, -100,   60,    0,    0,    0,  -96,
    0,  -26,    0, -100,    0, -100, -100,    0,    0,  145,
    0, -100,    0,    0,    0,    0,    0,    0,    0,   63,
    0,    0, -100,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   58,    0,    0,    0,    0,    0,
    0,    0,    0, -100,    0,    0,    0,
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
    0,    0,    0,    0,    0,    0,    0,
};
#endif
static const YYINT yygindex[] = {                         0,
 1005,    0,   10,    0,  -73,    0,   36,  -95,   84,    0,
 2599,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    7,  116,    0,
   18,    0, 1676,
};
#define YYTABLESIZE 2825
static const YYINT yytable[] = {                        183,
   88,  183,   43,   82,   76,   77,   85,  159,   80,   78,
  172,   79,   84,   81,  116,   75,  103,  182,   75,  202,
   44,  189,   46,  166,   54,  167,   85,  110,   92,  113,
   77,  111,   84,  161,   94,  163,   94,   88,   88,    2,
   88,   88,   88,   88,   88,   88,   88,   88,   76,   75,
   85,   76,   89,  199,  200,  198,   84,   83,   92,   88,
   88,   88,   88,   98,   88,   92,   92,   91,   92,   92,
   92,   92,   92,   92,   92,   92,  192,   83,   87,   59,
  112,   99,   58,   95,    2,  151,  184,   92,   92,   92,
   92,   88,   92,   88,   88,   89,  148,  149,   75,  193,
   76,   83,  147,  105,   91,   91,  155,   91,   91,   91,
   91,   91,   91,   91,   91,  148,  157,  141,  142,   92,
  143,   92,   92,   88,   88,   88,   91,   91,   91,   91,
  105,   91,   89,   89,   90,   89,   89,   89,   89,   89,
   89,   89,   89,  160,  162,   94,  175,  176,  179,  189,
  190,   92,   92,   92,   89,   89,   89,   89,   91,   89,
   91,   91,   87,  183,  191,  197,  204,  205,  206,   81,
    1,   90,   90,   82,   90,   90,   90,   90,   90,   90,
   90,   90,   20,   78,   18,  117,   89,   19,   89,   89,
   91,   91,   91,   90,   90,   90,   90,  185,   90,   87,
   87,  173,   87,   87,   87,   87,   87,   87,   87,   87,
   29,  154,  203,    0,  102,    0,    0,    0,   89,   89,
   89,   87,   87,   87,   87,   90,   87,   90,   90,   73,
    0,   72,   61,    0,   81,  171,    0,    0,    0,  181,
    0,  201,    0,   81,    0,    0,    0,  105,    0,   69,
   70,   29,   61,   87,   29,   87,   87,   90,   90,   90,
  106,    0,  107,    0,  108,   88,   88,    0,    0,   29,
   73,    0,   72,   73,  109,   72,   88,   88,   88,   88,
   88,   88,   88,   88,   88,    0,   87,   87,   73,    0,
   72,    0,    0,   92,   92,    0,    0,    0,    0,    0,
    0,    0,    0,   29,   92,   92,   92,   92,   92,   92,
   92,   92,   92,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   73,    0,   72,    0,    0,    0,    0,    0,
    0,   82,   91,   91,   85,   29,   80,   78,    0,   79,
   84,   81,    0,   91,   91,   91,   91,   91,   91,   91,
   91,   91,    0,    0,   73,    0,   72,    0,   77,    0,
   89,   89,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   89,   89,   89,   89,   89,   89,   89,   89,   89,
    0,    0,    0,    0,    0,   83,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   82,   90,
   90,   85,    0,   80,   78,    0,   79,   84,   81,   28,
   90,   90,   90,   90,   90,   90,   90,   90,   90,   85,
    0,    0,    0,    0,   20,    0,    0,   87,   87,   14,
    0,   19,    0,   17,   15,    0,   16,    0,   87,   87,
   87,   87,   87,   87,   87,   87,   87,  104,    0,    0,
   28,    0,   83,   28,    0,    0,   85,   85,    0,   85,
   85,   85,   85,   85,   85,   85,   85,    0,   28,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   85,   85,
   85,   85,   18,   85,  104,  104,   46,  104,  104,  104,
  104,  104,  104,  104,  104,    0,    0,    0,    0,    0,
    0,    0,   28,    0,    0,   73,  104,   72,  104,  104,
   85,  104,   85,   85,   42,    0,   82,    0,    0,   85,
    0,   80,    0,   46,   46,   84,   81,   46,   46,   46,
   46,   46,    0,   46,   28,    0,    0,    0,  104,    0,
  104,  104,    0,   85,   85,   46,   46,   46,   46,    0,
   46,   42,   42,   43,    0,   42,   42,   42,   42,   42,
   61,   42,    0,    0,    0,    0,    0,    0,    0,    0,
   83,  104,  104,   42,   42,   42,   42,    0,   42,   46,
   46,   45,    0,    0,    0,    0,    0,    0,    0,    0,
   43,   43,    0,    0,   43,   43,   43,   43,   43,    0,
   43,    0,    0,    0,    0,    0,    0,   42,   42,    0,
   46,   46,   43,   43,   43,   43,    0,   43,   45,   45,
    0,    0,   45,   45,   45,   45,   45,   61,   45,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   42,   42,
   45,   45,   45,   45,    0,   45,   43,   43,    1,    2,
    0,    3,    4,    5,    6,    7,    0,    0,    0,    8,
    9,    0,   10,   47,   11,    0,    0,   12,    0,    0,
    0,    0,    0,    0,   45,   45,   13,   43,   43,    0,
    0,    0,    0,    0,   85,   85,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   85,   85,   85,   85,   85,
   85,   85,   85,   85,    0,   45,   45,    0,    0,    0,
    0,    0,  104,  104,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  104,  104,  104,  104,  104,  104,  104,
  104,  104,    0,    0,   82,    0,    0,   85,    0,   80,
   78,    0,   79,   84,   81,   61,    0,    0,    0,    0,
    0,   46,   46,    0,    0,    0,    0,    0,    0,    0,
    0,   77,   46,   46,   46,   46,   46,   46,   46,   46,
   46,    0,    0,    0,    0,    0,    0,    0,    0,   42,
   42,    0,    0,    0,    0,    0,    0,    0,   83,    0,
   42,   42,   42,   42,   42,   42,   42,   42,   42,    0,
    0,    0,    0,    0,    0,    0,    0,   44,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   43,   43,
    0,  110,    0,  113,    0,  111,    0,    0,    0,   43,
   43,   43,   43,   43,   43,   43,   43,   43,    0,    0,
    0,    0,    0,   47,   44,   44,   45,   45,   44,   44,
   44,   44,   44,   64,   44,    0,    0,   45,   45,   45,
   45,   45,   45,   45,   45,   45,   44,   44,   44,   44,
    0,   44,    0,    0,  164,    0,    0,    0,    0,   52,
   47,   47,    0,    0,   47,   47,   47,   47,   47,   65,
   47,    0,    0,    0,   64,    0,    0,   64,    0,    0,
   44,   44,   47,   47,   47,   47,    0,   47,    0,    0,
    0,    0,   64,    0,   64,   54,   52,   52,    0,    0,
   52,   52,   52,   52,   52,   66,   52,    0,    0,    0,
   65,   44,   44,   65,    0,    0,   47,   47,   52,   52,
   52,   52,    0,   52,    0,    0,   64,    0,   65,    0,
   65,   55,   54,   54,    0,    0,   54,   54,   54,   54,
   54,    0,   54,   61,    0,    0,   66,   47,   47,   66,
    0,    0,   52,   52,   54,   54,   54,   54,   64,   54,
   69,   70,   65,    0,   66,    0,   66,   30,   55,   55,
    0,    0,   55,   55,   55,   55,   55,    0,   55,    0,
    0,    0,    0,   52,   52,    0,    0,    0,   54,   54,
   55,   55,   55,   55,   65,   55,    0,    0,   66,    0,
    0,    0,    0,    0,   30,    0,    0,    0,   30,    0,
    0,   30,    0,   30,   30,    0,    0,    0,    0,   54,
   54,  105,    0,    0,   55,   55,   30,   30,   30,   30,
   66,   30,    0,    0,  106,    0,  107,    0,  108,    0,
    0,    0,    0,    0,    0,  114,    0,    0,  109,    0,
    0,    0,   44,   44,    0,   55,   55,    0,    0,    0,
   30,   30,    0,   44,   44,   44,   44,   44,   44,   44,
   44,   44,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   47,   47,
    0,   30,   30,    0,  165,    0,  168,    0,   64,   47,
   47,   47,   47,   47,   47,   47,   47,   47,    0,   64,
   64,   64,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   52,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   65,   52,   52,   52,   52,   52,
   52,   52,   52,   52,    0,   65,   65,   65,  186,    0,
  187,  188,    0,    0,    0,    0,    0,    0,    0,   63,
   54,    0,    0,    0,    0,    0,    0,  196,    0,    0,
   66,   54,   54,   54,   54,   54,   54,   54,   54,   54,
    0,   66,   66,   66,    0,    0,    0,    0,  207,    0,
    0,    0,    0,    0,   27,    0,   55,    0,    0,    0,
   63,    0,    0,   63,    0,    0,    0,   55,   55,   55,
   55,   55,   55,   55,   55,   55,    0,    0,   63,   61,
   63,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   27,   30,   30,    0,   27,    0,   81,   27,    0,
   27,   27,    0,    0,   30,   30,   30,   30,   30,   30,
   30,   30,   63,   27,   27,   27,   27,   61,   27,    0,
   61,   62,   61,   61,   61,    0,    0,    0,    0,    0,
   53,    0,    0,    0,    0,    0,    0,    0,   61,   61,
   61,   61,    0,   61,   63,    0,    0,   27,   27,    0,
    0,    0,    0,    0,    0,    0,    0,   69,    0,   62,
    0,    0,   62,    0,   62,   62,   62,   70,   53,    0,
    0,   53,   61,   61,   53,    0,    0,    0,   27,   27,
   62,   62,   62,   62,    0,   62,    0,    0,    0,   53,
   53,   53,   53,    0,   53,   69,    0,    0,   69,    0,
    0,   69,    0,   61,   61,   70,    0,    0,   70,    0,
    0,   70,    0,    0,   62,   62,   69,   69,   69,   69,
    0,    0,    0,   53,   53,    0,   70,   70,   70,   70,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   62,   62,    0,    0,   58,
   69,   69,    0,    0,   53,   53,   60,    0,    0,    0,
   70,   70,    0,    0,    0,    0,    0,   20,    0,    0,
    0,    0,   14,    0,   19,    0,   17,   15,    0,   16,
    0,   69,   69,    0,   63,    0,    0,   58,    0,    0,
   58,   70,   70,   58,    0,   63,   63,   60,    0,    0,
   60,    0,    0,    0,    0,    0,    0,    0,   58,   58,
   58,   58,    0,    0,    0,   60,   60,   60,   60,   27,
   27,    0,    0,    0,   81,   18,    0,    0,   59,    0,
    0,   27,   27,   27,   27,   27,   27,   27,   27,    0,
    0,    0,   58,   58,   61,    0,    0,    0,    0,   60,
   60,    0,    0,    0,    0,   61,   61,   61,   61,   61,
   61,   61,   61,   61,    0,    0,    0,    0,    0,   59,
    0,    0,   59,   58,   58,    0,    0,    0,    0,    0,
   60,   60,    0,    0,    0,    0,   62,   59,   59,   59,
   59,    0,    0,    0,    0,   53,    0,   62,   62,   62,
   62,   62,   62,   62,   62,   62,   53,   53,   53,   53,
   53,   53,   53,   53,   53,   67,    0,    0,    0,   68,
    0,   59,   69,   56,    0,    0,    0,    0,    0,   57,
    0,    0,   70,   69,   69,   69,   69,   69,   69,   69,
   69,   69,    0,   70,   70,   70,   70,   70,   70,   70,
   70,   70,   59,   59,    0,    0,   67,    0,    0,   67,
   68,    0,    0,   68,   56,    0,    0,   56,    0,    0,
   57,    0,    0,   57,   67,    0,   67,    0,   68,    0,
   68,    0,   56,    0,   56,    0,    0,    0,   57,    0,
   57,    1,    2,    0,    3,    4,    5,    6,    7,    0,
    0,    0,    8,    9,    0,   10,   52,   11,   67,    0,
   12,    0,   68,    0,   58,    0,   56,    0,    0,   13,
    0,   60,   57,    0,    0,   58,   58,   58,   58,   58,
   58,   58,   60,   60,   60,   60,   60,   60,   60,    0,
   67,    0,    0,    0,   68,    0,    0,    0,   56,    0,
    0,    0,    0,   20,   57,    0,    0,    0,   14,    0,
   19,   55,   17,   15,    0,   16,    0,    0,    0,    0,
    0,    0,    0,   20,    0,    0,  118,    0,   14,    0,
   19,    0,   17,   15,    0,   16,   82,   76,    0,   85,
    0,   80,   78,   59,   79,   84,   81,    0,    0,    0,
    0,    0,    0,    0,   59,   59,   59,   59,   59,   59,
   59,   18,   20,   77,    0,    0,    0,   14,  118,   19,
    0,   17,   15,    0,   16,  118,    0,  118,  118,   82,
   76,   18,   85,   97,   80,   78,   96,   79,   84,   81,
   83,    0,    0,   75,    0,    0,    0,    0,    0,    0,
    0,    0,   72,   71,   73,    0,   77,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   18,    0,    0,   74,    0,    0,    0,  118,    0,  118,
   67,  118,  118,   83,   68,    0,   75,  118,   56,    0,
    0,   67,   67,   67,   57,   68,   68,   68,  118,   56,
   56,   56,    0,    0,    0,   57,   57,   57,    0,    0,
    0,    0,   82,   76,    0,   85,   74,   80,   78,  118,
   79,   84,   81,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   93,   72,   71,   73,    0,   77,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   83,    0,    0,   75,
    0,    0,    0,    0,    0,    0,    0,    1,    2,    0,
    3,    4,    5,    6,    7,    0,    0,    0,    8,    9,
    0,   10,    0,   11,    0,    0,   12,    1,    2,   74,
    3,    4,    5,    6,    7,   13,    0,    0,    8,    9,
    0,   10,    0,   11,    0,   61,   12,    0,    0,    0,
    0,    0,    0,    0,    0,   13,    0,    0,    0,    0,
    0,    0,   69,   70,    0,    0,    1,    2,    0,    3,
    4,    5,    6,    7,    0,    0,    0,    8,    9,    0,
   10,    0,   11,    0,    0,    0,    0,   60,   61,    0,
    0,    0,    0,    0,   13,    0,    0,    0,   62,   63,
   64,   65,   66,   67,   68,   69,   70,   82,   76,    0,
   85,    0,   80,   78,    0,   79,   84,   81,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  150,
   72,   71,   73,    0,   77,    0,    0,    0,    0,    0,
    0,    0,   82,   76,    0,   85,    0,   80,   78,    0,
   79,   84,   81,    0,    0,    0,    0,    0,    0,    0,
    0,   83,    0,    0,   75,   72,   71,   73,    0,   77,
   60,   61,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   62,   63,   64,   65,   66,   67,   68,   69,   70,
    0,    0,    0,    0,   74,    0,   83,    0,  174,   75,
   82,   76,    0,   85,    0,   80,   78,    0,   79,   84,
   81,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   72,   71,   73,    0,   77,    0,   74,
    0,    0,    0,    0,    0,   82,   76,    0,   85,    0,
   80,   78,    0,   79,   84,   81,    0,    0,    0,    0,
    0,    0,    0,    0,   83,    0,  178,   75,   72,   71,
   73,    0,   77,    0,    0,    0,    0,    0,    0,    0,
   82,   76,    0,   85,    0,   80,   78,  195,   79,   84,
   81,    0,    0,    0,    0,    0,    0,   74,    0,   83,
    0,  194,   75,   72,   71,   73,    0,   77,    0,    0,
    0,    0,    0,    0,    0,   82,   76,    0,   85,    0,
   80,   78,    0,   79,   84,   81,    0,    0,    0,    0,
    0,    0,   74,    0,   83,    0,    0,   75,   72,   71,
   73,    0,   77,    0,    0,   60,   61,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   62,   63,   64,   65,
   66,   67,   68,   69,   70,    0,    0,   74,    0,   83,
    0,    0,   75,    0,    0,    0,    0,    0,    0,    0,
   60,   61,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   62,   63,   64,   65,   66,   67,   68,   69,   70,
    0,    0,   74,    0,    0,    0,   82,   76,    0,   85,
    0,   80,   78,    0,   79,   84,   81,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   72,
   71,   73,    0,   77,    0,    0,    0,    0,   60,   61,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   62,
   63,   64,   65,   66,   67,   68,   69,   70,    0,    0,
   83,    0,    0,   75,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   60,   61,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   62,   63,   64,   65,   66,   67,
   68,   69,   70,   74,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   60,   61,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   62,
   63,   64,   65,   66,   67,   68,   69,   70,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   60,   61,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   62,   63,   64,   65,   66,   67,
   68,   69,   70,    0,    0,    0,    0,   82,   76,    0,
   85,    0,   80,   78,    0,   79,   84,   81,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   72,    0,   73,    0,   77,    0,    0,    0,    0,    0,
    0,    0,   82,   76,    0,   85,    0,   80,   78,    0,
   79,   84,   81,    0,    0,    0,    0,    0,    0,    0,
    0,   83,    0,    0,   75,   72,    0,   73,    0,   77,
    0,    0,    0,    0,   60,   61,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   63,   64,   65,   66,
   67,   68,   69,   70,   74,    0,   83,   82,   76,   75,
   85,    0,   80,   78,    0,   79,   84,   81,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   25,    0,
    0,    0,    0,    0,   77,    0,    0,    0,    0,   74,
   45,    0,   48,   49,   50,   51,   53,   56,   57,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   83,    0,    0,   75,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   90,    0,    0,    0,    0,
   91,    0,    0,    0,    0,    0,    0,    0,  104,    0,
  119,  120,  121,  122,  123,  124,  125,  126,  127,  128,
  129,  130,  131,  132,  133,  134,  135,  136,  137,  138,
  139,  140,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  152,  153,    0,    0,    0,    0,  156,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   61,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   64,   65,
   66,   67,   68,   69,   70,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  177,    0,
    0,   61,    0,  180,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   65,   66,   67,   68,   69,   70,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   61,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   69,   70,
};
static const YYINT yycheck[] = {                         44,
    0,   44,  285,   37,   38,   41,   40,  103,   42,   43,
   60,   45,   46,   47,   41,   41,   60,   62,   44,   62,
   11,   59,   13,  272,   18,  274,   40,   38,    0,   40,
   64,   42,   46,  107,   44,  109,   44,   37,   38,  258,
   40,   41,   42,   43,   44,   45,   46,   47,   41,   44,
   40,   44,   43,  260,  261,   93,   46,   91,   52,   59,
   60,   61,   62,  258,   64,   37,   38,    0,   40,   41,
   42,   43,   44,   45,   46,   47,  172,   91,  270,  285,
   91,  276,  123,   93,  258,   93,  160,   59,   60,   61,
   62,   91,   64,   93,   94,    0,  270,   88,   93,  173,
   93,   91,  285,  258,   37,   38,   58,   40,   41,   42,
   43,   44,   45,   46,   47,  270,  125,  258,  259,   91,
  261,   93,   94,  123,  124,  125,   59,   60,   61,   62,
  258,   64,   37,   38,    0,   40,   41,   42,   43,   44,
   45,   46,   47,  285,   40,   44,   41,  285,   41,   59,
   41,  123,  124,  125,   59,   60,   61,   62,   91,   64,
   93,   94,    0,   44,  258,   41,  286,   93,   93,  270,
    0,   37,   38,  270,   40,   41,   42,   43,   44,   45,
   46,   47,  125,   41,  125,   41,   91,  125,   93,   94,
  123,  124,  125,   59,   60,   61,   62,  162,   64,   37,
   38,  118,   40,   41,   42,   43,   44,   45,   46,   47,
    0,   96,  195,   -1,  258,   -1,   -1,   -1,  123,  124,
  125,   59,   60,   61,   62,   91,   64,   93,   94,    0,
   -1,    0,  266,   -1,  270,  285,   -1,   -1,   -1,  284,
   -1,  284,   -1,  270,   -1,   -1,   -1,  258,   -1,  283,
  284,   41,  266,   91,   44,   93,   94,  123,  124,  125,
  271,   -1,  273,   -1,  275,  265,  266,   -1,   -1,   59,
   41,   -1,   41,   44,  285,   44,  276,  277,  278,  279,
  280,  281,  282,  283,  284,   -1,  124,  125,   59,   -1,
   59,   -1,   -1,  265,  266,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   93,  276,  277,  278,  279,  280,  281,
  282,  283,  284,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   93,   -1,   93,   -1,   -1,   -1,   -1,   -1,
   -1,   37,  265,  266,   40,  125,   42,   43,   -1,   45,
   46,   47,   -1,  276,  277,  278,  279,  280,  281,  282,
  283,  284,   -1,   -1,  125,   -1,  125,   -1,   64,   -1,
  265,  266,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  276,  277,  278,  279,  280,  281,  282,  283,  284,
   -1,   -1,   -1,   -1,   -1,   91,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   37,  265,
  266,   40,   -1,   42,   43,   -1,   45,   46,   47,    0,
  276,  277,  278,  279,  280,  281,  282,  283,  284,    0,
   -1,   -1,   -1,   -1,   33,   -1,   -1,  265,  266,   38,
   -1,   40,   -1,   42,   43,   -1,   45,   -1,  276,  277,
  278,  279,  280,  281,  282,  283,  284,    0,   -1,   -1,
   41,   -1,   91,   44,   -1,   -1,   37,   38,   -1,   40,
   41,   42,   43,   44,   45,   46,   47,   -1,   59,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   59,   60,
   61,   62,   91,   64,   37,   38,    0,   40,   41,   42,
   43,   44,   45,   46,   47,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   93,   -1,   -1,  276,   59,  276,   61,   62,
   91,   64,   93,   94,    0,   -1,   37,   -1,   -1,   40,
   -1,   42,   -1,   37,   38,   46,   47,   41,   42,   43,
   44,   45,   -1,   47,  125,   -1,   -1,   -1,   91,   -1,
   93,   94,   -1,  124,  125,   59,   60,   61,   62,   -1,
   64,   37,   38,    0,   -1,   41,   42,   43,   44,   45,
  266,   47,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   91,  124,  125,   59,   60,   61,   62,   -1,   64,   93,
   94,    0,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   37,   38,   -1,   -1,   41,   42,   43,   44,   45,   -1,
   47,   -1,   -1,   -1,   -1,   -1,   -1,   93,   94,   -1,
  124,  125,   59,   60,   61,   62,   -1,   64,   37,   38,
   -1,   -1,   41,   42,   43,   44,   45,  266,   47,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  124,  125,
   59,   60,   61,   62,   -1,   64,   93,   94,  257,  258,
   -1,  260,  261,  262,  263,  264,   -1,   -1,   -1,  268,
  269,   -1,  271,  272,  273,   -1,   -1,  276,   -1,   -1,
   -1,   -1,   -1,   -1,   93,   94,  285,  124,  125,   -1,
   -1,   -1,   -1,   -1,  265,  266,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,  276,  277,  278,  279,  280,
  281,  282,  283,  284,   -1,  124,  125,   -1,   -1,   -1,
   -1,   -1,  265,  266,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,  276,  277,  278,  279,  280,  281,  282,
  283,  284,   -1,   -1,   37,   -1,   -1,   40,   -1,   42,
   43,   -1,   45,   46,   47,  266,   -1,   -1,   -1,   -1,
   -1,  265,  266,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   64,  276,  277,  278,  279,  280,  281,  282,  283,
  284,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  265,
  266,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   91,   -1,
  276,  277,  278,  279,  280,  281,  282,  283,  284,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,    0,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  265,  266,
   -1,   38,   -1,   40,   -1,   42,   -1,   -1,   -1,  276,
  277,  278,  279,  280,  281,  282,  283,  284,   -1,   -1,
   -1,   -1,   -1,    0,   37,   38,  265,  266,   41,   42,
   43,   44,   45,    0,   47,   -1,   -1,  276,  277,  278,
  279,  280,  281,  282,  283,  284,   59,   60,   61,   62,
   -1,   64,   -1,   -1,   91,   -1,   -1,   -1,   -1,    0,
   37,   38,   -1,   -1,   41,   42,   43,   44,   45,    0,
   47,   -1,   -1,   -1,   41,   -1,   -1,   44,   -1,   -1,
   93,   94,   59,   60,   61,   62,   -1,   64,   -1,   -1,
   -1,   -1,   59,   -1,   61,    0,   37,   38,   -1,   -1,
   41,   42,   43,   44,   45,    0,   47,   -1,   -1,   -1,
   41,  124,  125,   44,   -1,   -1,   93,   94,   59,   60,
   61,   62,   -1,   64,   -1,   -1,   93,   -1,   59,   -1,
   61,    0,   37,   38,   -1,   -1,   41,   42,   43,   44,
   45,   -1,   47,  266,   -1,   -1,   41,  124,  125,   44,
   -1,   -1,   93,   94,   59,   60,   61,   62,  125,   64,
  283,  284,   93,   -1,   59,   -1,   61,    0,   37,   38,
   -1,   -1,   41,   42,   43,   44,   45,   -1,   47,   -1,
   -1,   -1,   -1,  124,  125,   -1,   -1,   -1,   93,   94,
   59,   60,   61,   62,  125,   64,   -1,   -1,   93,   -1,
   -1,   -1,   -1,   -1,   37,   -1,   -1,   -1,   41,   -1,
   -1,   44,   -1,   46,   47,   -1,   -1,   -1,   -1,  124,
  125,  258,   -1,   -1,   93,   94,   59,   60,   61,   62,
  125,   64,   -1,   -1,  271,   -1,  273,   -1,  275,   -1,
   -1,   -1,   -1,   -1,   -1,   61,   -1,   -1,  285,   -1,
   -1,   -1,  265,  266,   -1,  124,  125,   -1,   -1,   -1,
   93,   94,   -1,  276,  277,  278,  279,  280,  281,  282,
  283,  284,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  265,  266,
   -1,  124,  125,   -1,  110,   -1,  112,   -1,  265,  276,
  277,  278,  279,  280,  281,  282,  283,  284,   -1,  276,
  277,  278,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  265,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  265,  276,  277,  278,  279,  280,
  281,  282,  283,  284,   -1,  276,  277,  278,  164,   -1,
  166,  167,   -1,   -1,   -1,   -1,   -1,   -1,   -1,    0,
  265,   -1,   -1,   -1,   -1,   -1,   -1,  183,   -1,   -1,
  265,  276,  277,  278,  279,  280,  281,  282,  283,  284,
   -1,  276,  277,  278,   -1,   -1,   -1,   -1,  204,   -1,
   -1,   -1,   -1,   -1,    0,   -1,  265,   -1,   -1,   -1,
   41,   -1,   -1,   44,   -1,   -1,   -1,  276,  277,  278,
  279,  280,  281,  282,  283,  284,   -1,   -1,   59,    0,
   61,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   37,  265,  266,   -1,   41,   -1,  270,   44,   -1,
   46,   47,   -1,   -1,  277,  278,  279,  280,  281,  282,
  283,  284,   93,   59,   60,   61,   62,   38,   64,   -1,
   41,    0,   43,   44,   45,   -1,   -1,   -1,   -1,   -1,
    0,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   59,   60,
   61,   62,   -1,   64,  125,   -1,   -1,   93,   94,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,    0,   -1,   38,
   -1,   -1,   41,   -1,   43,   44,   45,    0,   38,   -1,
   -1,   41,   93,   94,   44,   -1,   -1,   -1,  124,  125,
   59,   60,   61,   62,   -1,   64,   -1,   -1,   -1,   59,
   60,   61,   62,   -1,   64,   38,   -1,   -1,   41,   -1,
   -1,   44,   -1,  124,  125,   38,   -1,   -1,   41,   -1,
   -1,   44,   -1,   -1,   93,   94,   59,   60,   61,   62,
   -1,   -1,   -1,   93,   94,   -1,   59,   60,   61,   62,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,  124,  125,   -1,   -1,    0,
   93,   94,   -1,   -1,  124,  125,    0,   -1,   -1,   -1,
   93,   94,   -1,   -1,   -1,   -1,   -1,   33,   -1,   -1,
   -1,   -1,   38,   -1,   40,   -1,   42,   43,   -1,   45,
   -1,  124,  125,   -1,  265,   -1,   -1,   38,   -1,   -1,
   41,  124,  125,   44,   -1,  276,  277,   41,   -1,   -1,
   44,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   59,   60,
   61,   62,   -1,   -1,   -1,   59,   60,   61,   62,  265,
  266,   -1,   -1,   -1,  270,   91,   -1,   -1,    0,   -1,
   -1,  277,  278,  279,  280,  281,  282,  283,  284,   -1,
   -1,   -1,   93,   94,  265,   -1,   -1,   -1,   -1,   93,
   94,   -1,   -1,   -1,   -1,  276,  277,  278,  279,  280,
  281,  282,  283,  284,   -1,   -1,   -1,   -1,   -1,   41,
   -1,   -1,   44,  124,  125,   -1,   -1,   -1,   -1,   -1,
  124,  125,   -1,   -1,   -1,   -1,  265,   59,   60,   61,
   62,   -1,   -1,   -1,   -1,  265,   -1,  276,  277,  278,
  279,  280,  281,  282,  283,  284,  276,  277,  278,  279,
  280,  281,  282,  283,  284,    0,   -1,   -1,   -1,    0,
   -1,   93,  265,    0,   -1,   -1,   -1,   -1,   -1,    0,
   -1,   -1,  265,  276,  277,  278,  279,  280,  281,  282,
  283,  284,   -1,  276,  277,  278,  279,  280,  281,  282,
  283,  284,  124,  125,   -1,   -1,   41,   -1,   -1,   44,
   41,   -1,   -1,   44,   41,   -1,   -1,   44,   -1,   -1,
   41,   -1,   -1,   44,   59,   -1,   61,   -1,   59,   -1,
   61,   -1,   59,   -1,   61,   -1,   -1,   -1,   59,   -1,
   61,  257,  258,   -1,  260,  261,  262,  263,  264,   -1,
   -1,   -1,  268,  269,   -1,  271,  272,  273,   93,   -1,
  276,   -1,   93,   -1,  265,   -1,   93,   -1,   -1,  285,
   -1,  265,   93,   -1,   -1,  276,  277,  278,  279,  280,
  281,  282,  276,  277,  278,  279,  280,  281,  282,   -1,
  125,   -1,   -1,   -1,  125,   -1,   -1,   -1,  125,   -1,
   -1,   -1,   -1,   33,  125,   -1,   -1,   -1,   38,   -1,
   40,   41,   42,   43,   -1,   45,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   33,   -1,   -1,   61,   -1,   38,   -1,
   40,   -1,   42,   43,   -1,   45,   37,   38,   -1,   40,
   -1,   42,   43,  265,   45,   46,   47,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  276,  277,  278,  279,  280,  281,
  282,   91,   33,   64,   -1,   -1,   -1,   38,  103,   40,
   -1,   42,   43,   -1,   45,  110,   -1,  112,  113,   37,
   38,   91,   40,   41,   42,   43,   44,   45,   46,   47,
   91,   -1,   -1,   94,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   60,   61,   62,   -1,   64,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   91,   -1,   -1,  124,   -1,   -1,   -1,  162,   -1,  164,
  265,  166,  167,   91,  265,   -1,   94,  172,  265,   -1,
   -1,  276,  277,  278,  265,  276,  277,  278,  183,  276,
  277,  278,   -1,   -1,   -1,  276,  277,  278,   -1,   -1,
   -1,   -1,   37,   38,   -1,   40,  124,   42,   43,  204,
   45,   46,   47,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   59,   60,   61,   62,   -1,   64,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   91,   -1,   -1,   94,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  257,  258,   -1,
  260,  261,  262,  263,  264,   -1,   -1,   -1,  268,  269,
   -1,  271,   -1,  273,   -1,   -1,  276,  257,  258,  124,
  260,  261,  262,  263,  264,  285,   -1,   -1,  268,  269,
   -1,  271,   -1,  273,   -1,  266,  276,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,  285,   -1,   -1,   -1,   -1,
   -1,   -1,  283,  284,   -1,   -1,  257,  258,   -1,  260,
  261,  262,  263,  264,   -1,   -1,   -1,  268,  269,   -1,
  271,   -1,  273,   -1,   -1,   -1,   -1,  265,  266,   -1,
   -1,   -1,   -1,   -1,  285,   -1,   -1,   -1,  276,  277,
  278,  279,  280,  281,  282,  283,  284,   37,   38,   -1,
   40,   -1,   42,   43,   -1,   45,   46,   47,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   59,
   60,   61,   62,   -1,   64,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   37,   38,   -1,   40,   -1,   42,   43,   -1,
   45,   46,   47,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   91,   -1,   -1,   94,   60,   61,   62,   -1,   64,
  265,  266,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  276,  277,  278,  279,  280,  281,  282,  283,  284,
   -1,   -1,   -1,   -1,  124,   -1,   91,   -1,   93,   94,
   37,   38,   -1,   40,   -1,   42,   43,   -1,   45,   46,
   47,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   60,   61,   62,   -1,   64,   -1,  124,
   -1,   -1,   -1,   -1,   -1,   37,   38,   -1,   40,   -1,
   42,   43,   -1,   45,   46,   47,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   91,   -1,   93,   94,   60,   61,
   62,   -1,   64,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   37,   38,   -1,   40,   -1,   42,   43,   44,   45,   46,
   47,   -1,   -1,   -1,   -1,   -1,   -1,  124,   -1,   91,
   -1,   93,   94,   60,   61,   62,   -1,   64,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   37,   38,   -1,   40,   -1,
   42,   43,   -1,   45,   46,   47,   -1,   -1,   -1,   -1,
   -1,   -1,  124,   -1,   91,   -1,   -1,   94,   60,   61,
   62,   -1,   64,   -1,   -1,  265,  266,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,  276,  277,  278,  279,
  280,  281,  282,  283,  284,   -1,   -1,  124,   -1,   91,
   -1,   -1,   94,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
  265,  266,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  276,  277,  278,  279,  280,  281,  282,  283,  284,
   -1,   -1,  124,   -1,   -1,   -1,   37,   38,   -1,   40,
   -1,   42,   43,   -1,   45,   46,   47,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   60,
   61,   62,   -1,   64,   -1,   -1,   -1,   -1,  265,  266,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  276,
  277,  278,  279,  280,  281,  282,  283,  284,   -1,   -1,
   91,   -1,   -1,   94,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,  265,  266,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  276,  277,  278,  279,  280,  281,
  282,  283,  284,  124,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  265,  266,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  276,
  277,  278,  279,  280,  281,  282,  283,  284,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,  265,  266,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  276,  277,  278,  279,  280,  281,
  282,  283,  284,   -1,   -1,   -1,   -1,   37,   38,   -1,
   40,   -1,   42,   43,   -1,   45,   46,   47,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   60,   -1,   62,   -1,   64,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   37,   38,   -1,   40,   -1,   42,   43,   -1,
   45,   46,   47,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   91,   -1,   -1,   94,   60,   -1,   62,   -1,   64,
   -1,   -1,   -1,   -1,  265,  266,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,  277,  278,  279,  280,
  281,  282,  283,  284,  124,   -1,   91,   37,   38,   94,
   40,   -1,   42,   43,   -1,   45,   46,   47,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,    0,   -1,
   -1,   -1,   -1,   -1,   64,   -1,   -1,   -1,   -1,  124,
   12,   -1,   14,   15,   16,   17,   18,   19,   20,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   91,   -1,   -1,   94,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   47,   -1,   -1,   -1,   -1,
   52,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   60,   -1,
   62,   63,   64,   65,   66,   67,   68,   69,   70,   71,
   72,   73,   74,   75,   76,   77,   78,   79,   80,   81,
   82,   83,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   93,   94,   -1,   -1,   -1,   -1,   99,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,  266,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  278,  279,
  280,  281,  282,  283,  284,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  150,   -1,
   -1,  266,   -1,  155,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  279,  280,  281,  282,  283,  284,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,  266,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,  283,  284,
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
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,
};
#endif
#define YYFINAL 21
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 287
#define YYUNDFTOKEN 323
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
"KW_MUT","KW_EXTERN","KW_CONST","KW_FN","DOTDOT","OROR","ANDAND","EQEQ","NOTEQ",
"LTEQ","GTEQ","LSH","RSH","COLONCOLON","ARROW","UNARY","$accept","start","type",
"path_for_expr","identifier_path_for_expr","path_for_type",
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

int      yydebug;
int      yynerrs;

int      yyerrflag;
int      yychar;
YYSTYPE  yyval;
YYSTYPE  yylval;

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

#define YYINITSTACKSIZE 200

typedef struct {
    unsigned stacksize;
    YYINT    *s_base;
    YYINT    *s_mark;
    YYINT    *s_last;
    YYSTYPE  *l_base;
    YYSTYPE  *l_mark;
} YYSTACKDATA;
/* variables for the parser stack */
static YYSTACKDATA yystack;
#line 851 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"

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
  char *crate = rust_crate_for_block (expression_context_block);
  struct stoken result;

  gdb_assert (name->opcode == OP_VAR_VALUE);

  if (crate == NULL)
    error (_("Could not find crate for current location"));
  result = make_stoken (obconcat (&work_obstack, "::", crate, "::",
				  name->left.sval.ptr, (char *) NULL));
  xfree (crate);

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
      VEC (int) *offsets = NULL;
      unsigned int current_len;
      struct cleanup *cleanup;

      cleanup = make_cleanup (VEC_cleanup (int), &offsets);
      current_len = cp_find_first_component (scope);
      while (scope[current_len] != '\0')
	{
	  VEC_safe_push (int, offsets, current_len);
	  gdb_assert (scope[current_len] == ':');
	  /* The "::".  */
	  current_len += 2;
	  current_len += cp_find_first_component (scope
						  + current_len);
	}

      len = VEC_length (int, offsets);
      if (n_supers >= len)
	error (_("Too many super:: uses from '%s'"), scope);

      offset = VEC_index (int, offsets, len - n_supers);

      do_cleanups (cleanup);
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

static int
ends_raw_string (const char *str, int n)
{
  int i;

  gdb_assert (str[0] == '"');
  for (i = 0; i < n; ++i)
    if (str[i + 1] != '#')
      return 0;
  return 1;
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

static int
space_then_number (const char *string)
{
  const char *p = string;

  while (p[0] == ' ' || p[0] == '\t')
    ++p;
  if (p == string)
    return 0;

  return *p >= '0' && *p <= '9';
}

/* Return true if C can start an identifier.  */

static int
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
  char *type_name = NULL;
  struct type *type;
  int end_index;
  int type_index = -1;
  int i, out;
  char *number;
  struct cleanup *cleanup = make_cleanup (null_cleanup, NULL);

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
  if (type_name == NULL)
    {
      gdb_assert (type_index != -1);
      type_name = xstrndup (lexptr + subexps[type_index].rm_so,
			   (subexps[type_index].rm_eo
			    - subexps[type_index].rm_so));
      make_cleanup (xfree, type_name);
    }

  /* Look up the type.  */
  type = rust_type (type_name);

  /* Copy the text of the number and remove the "_"s.  */
  number = xstrndup (lexptr, end_index);
  make_cleanup (xfree, number);
  for (i = out = 0; number[i]; ++i)
    {
      if (number[i] == '_')
	could_be_decimal = 0;
      else
	number[out++] = number[i];
    }
  number[out] = '\0';

  /* Advance past the match.  */
  lexptr += subexps[0].rm_eo;

  /* Parse the number.  */
  if (is_integer)
    {
      uint64_t value;
      int radix = 10;
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
	      number += 2;
	      could_be_decimal = 0;
	    }
	}

      value = strtoul (number, NULL, radix);
      if (implicit_i32 && value >= ((uint64_t) 1) << 31)
	type = rust_type ("i64");

      rustyylval.typed_val_int.val = value;
      rustyylval.typed_val_int.type = type;
    }
  else
    {
      rustyylval.typed_val_float.dval = strtod (number, NULL);
      rustyylval.typed_val_float.type = type;
    }

  do_cleanups (cleanup);
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

static VEC (type_ptr) *
convert_params_to_types (struct parser_state *state, VEC (rust_op_ptr) *params)
{
  int i;
  const struct rust_op *op;
  VEC (type_ptr) *result = NULL;
  struct cleanup *cleanup = make_cleanup (VEC_cleanup (type_ptr), &result);

  for (i = 0; VEC_iterate (rust_op_ptr, params, i, op); ++i)
    VEC_safe_push (type_ptr, result, convert_ast_to_type (state, op));

  discard_cleanups (cleanup);
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
	VEC (type_ptr) *args
	  = convert_params_to_types (state, *operation->right.params);
	struct cleanup *cleanup
	  = make_cleanup (VEC_cleanup (type_ptr), &args);
	struct type **argtypes = NULL;

	type = convert_ast_to_type (state, operation->left.op);
	if (!VEC_empty (type_ptr, args))
	  argtypes = VEC_address (type_ptr, args);

	result
	  = lookup_function_type_with_arguments (type,
						 VEC_length (type_ptr, args),
						 argtypes);
	result = lookup_pointer_type (result);

	do_cleanups (cleanup);
      }
      break;

    case TYPE_CODE_STRUCT:
      {
	VEC (type_ptr) *args
	  = convert_params_to_types (state, *operation->left.params);
	struct cleanup *cleanup
	  = make_cleanup (VEC_cleanup (type_ptr), &args);
	int i;
	struct type *type;
	const char *name;

	obstack_1grow (&work_obstack, '(');
	for (i = 0; VEC_iterate (type_ptr, args, i, type); ++i)
	  {
	    char *type_name = type_to_string (type);

	    if (i > 0)
	      obstack_1grow (&work_obstack, ',');
	    obstack_grow_str (&work_obstack, type_name);

	    xfree (type_name);
	  }

	obstack_grow_str0 (&work_obstack, ")");
	name = (const char *) obstack_finish (&work_obstack);

	/* We don't allow creating new tuple types (yet), but we do
	   allow looking up existing tuple types.  */
	result = rust_lookup_type (name, expression_context_block);
	if (result == NULL)
	  error (_("could not find tuple type '%s'"), name);

	do_cleanups (cleanup);
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
  VEC (type_ptr) *types;
  struct cleanup *cleanup;
  int i;
  struct type *type;

  gdb_assert (operation->opcode == OP_VAR_VALUE);

  if (operation->right.params == NULL)
    return operation->left.sval.ptr;

  types = convert_params_to_types (state, *operation->right.params);
  cleanup = make_cleanup (VEC_cleanup (type_ptr), &types);

  obstack_grow_str (&work_obstack, operation->left.sval.ptr);
  obstack_1grow (&work_obstack, '<');
  for (i = 0; VEC_iterate (type_ptr, types, i, type); ++i)
    {
      char *type_name = type_to_string (type);

      if (i > 0)
	obstack_1grow (&work_obstack, ',');

      obstack_grow_str (&work_obstack, type_name);
      xfree (type_name);
    }
  obstack_grow_str0 (&work_obstack, ">");

  do_cleanups (cleanup);

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
rustyyerror (char *msg)
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
#line 3449 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.c"

#if YYDEBUG
#include <stdio.h>		/* needed for printf */
#endif

#include <stdlib.h>	/* needed for xmalloc, etc */
#include <string.h>	/* needed for memset */

/* allocate initial stack or double stack size, up to YYMAXDEPTH */
static int yygrowstack(YYSTACKDATA *data)
{
    int i;
    unsigned newsize;
    YYINT *newss;
    YYSTYPE *newvs;

    if ((newsize = data->stacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return YYENOMEM;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;

    i = (int) (data->s_mark - data->s_base);
    newss = (YYINT *)xrealloc(data->s_base, newsize * sizeof(*newss));
    if (newss == 0)
        return YYENOMEM;

    data->s_base = newss;
    data->s_mark = newss + i;

    newvs = (YYSTYPE *)xrealloc(data->l_base, newsize * sizeof(*newvs));
    if (newvs == 0)
        return YYENOMEM;

    data->l_base = newvs;
    data->l_mark = newvs + i;

    data->stacksize = newsize;
    data->s_last = data->s_base + newsize - 1;
    return 0;
}

#if YYPURE || defined(YY_NO_LEAKS)
static void yyfreestack(YYSTACKDATA *data)
{
    xfree(data->s_base);
    xfree(data->l_base);
    memset(data, 0, sizeof(*data));
}
#else
#define yyfreestack(data) /* nothing */
#endif

#define YYABORT  goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR  goto yyerrlab

int
YYPARSE_DECL()
{
    int yym, yyn, yystate;
#if YYDEBUG
    const char *yys;

    if ((yys = getenv("YYDEBUG")) != 0)
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

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
    yystate = 0;
    *yystack.s_mark = 0;

yyloop:
    if ((yyn = yydefred[yystate]) != 0) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = YYLEX) < 0) yychar = YYEOF;
#if YYDEBUG
        if (yydebug)
        {
            yys = yyname[YYTRANSLATE(yychar)];
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM)
        {
            goto yyoverflow;
        }
        yystate = yytable[yyn];
        *++yystack.s_mark = yytable[yyn];
        *++yystack.l_mark = yylval;
        yychar = YYEMPTY;
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;

    YYERROR_CALL("syntax error");

    goto yyerrlab;

yyerrlab:
    ++yynerrs;

yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yystack.s_mark]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yystack.s_mark, yytable[yyn]);
#endif
                if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM)
                {
                    goto yyoverflow;
                }
                yystate = yytable[yyn];
                *++yystack.s_mark = yytable[yyn];
                *++yystack.l_mark = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yystack.s_mark);
#endif
                if (yystack.s_mark <= yystack.s_base) goto yyabort;
                --yystack.s_mark;
                --yystack.l_mark;
            }
        }
    }
    else
    {
        if (yychar == YYEOF) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = yyname[YYTRANSLATE(yychar)];
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = YYEMPTY;
        goto yyloop;
    }

yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    if (yym)
        yyval = yystack.l_mark[1-yym];
    else
        memset(&yyval, 0, sizeof yyval);
    switch (yyn)
    {
case 1:
#line 353 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{
		  /* If we are completing and see a valid parse,
		     rust_ast will already have been set.  */
		  if (rust_ast == NULL)
		    rust_ast = yystack.l_mark[0].op;
		}
break;
case 15:
#line 382 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{
		  VEC_safe_insert (rust_op_ptr, *yystack.l_mark[-1].params, 0, yystack.l_mark[-3].op);
		  error (_("Tuple expressions not supported yet"));
		}
break;
case 16:
#line 390 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
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
#line 407 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_struct (yystack.l_mark[-3].op, yystack.l_mark[-1].field_inits); }
break;
case 18:
#line 412 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{
		  struct set_field sf;

		  sf.name.ptr = NULL;
		  sf.name.length = 0;
		  sf.init = yystack.l_mark[0].op;

		  yyval.one_field_init = sf;
		}
break;
case 19:
#line 422 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{
		  struct set_field sf;

		  sf.name = yystack.l_mark[-2].sval;
		  sf.init = yystack.l_mark[0].op;
		  yyval.one_field_init = sf;
		}
break;
case 20:
#line 433 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{
		  VEC (set_field) **result
		    = OBSTACK_ZALLOC (&work_obstack, VEC (set_field) *);
		  yyval.field_inits = result;
		}
break;
case 21:
#line 439 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{
		  VEC (set_field) **result
		    = OBSTACK_ZALLOC (&work_obstack, VEC (set_field) *);

		  make_cleanup (VEC_cleanup (set_field), result);
		  VEC_safe_push (set_field, *result, &yystack.l_mark[0].one_field_init);

		  yyval.field_inits = result;
		}
break;
case 22:
#line 449 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{
		  struct set_field sf;

		  sf.name = yystack.l_mark[-4].sval;
		  sf.init = yystack.l_mark[-2].op;
		  VEC_safe_push (set_field, *yystack.l_mark[0].field_inits, &sf);
		  yyval.field_inits = yystack.l_mark[0].field_inits;
		}
break;
case 23:
#line 461 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_call_ish (OP_ARRAY, NULL, yystack.l_mark[-1].params); }
break;
case 24:
#line 463 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_call_ish (OP_ARRAY, NULL, yystack.l_mark[-1].params); }
break;
case 25:
#line 465 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (OP_RUST_ARRAY, yystack.l_mark[-3].op, yystack.l_mark[-1].op); }
break;
case 26:
#line 467 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (OP_RUST_ARRAY, yystack.l_mark[-3].op, yystack.l_mark[-1].op); }
break;
case 27:
#line 472 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_range (yystack.l_mark[-1].op, NULL); }
break;
case 28:
#line 474 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_range (yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 29:
#line 476 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_range (NULL, yystack.l_mark[0].op); }
break;
case 30:
#line 478 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_range (NULL, NULL); }
break;
case 31:
#line 483 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_literal (yystack.l_mark[0].typed_val_int); }
break;
case 32:
#line 485 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_literal (yystack.l_mark[0].typed_val_int); }
break;
case 33:
#line 487 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_dliteral (yystack.l_mark[0].typed_val_float); }
break;
case 34:
#line 489 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
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
#line 518 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_string (yystack.l_mark[0].sval); }
break;
case 36:
#line 520 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{
		  struct typed_val_int val;

		  val.type = language_bool_type (parse_language (pstate),
						 parse_gdbarch (pstate));
		  val.val = 1;
		  yyval.op = ast_literal (val);
		}
break;
case 37:
#line 529 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{
		  struct typed_val_int val;

		  val.type = language_bool_type (parse_language (pstate),
						 parse_gdbarch (pstate));
		  val.val = 0;
		  yyval.op = ast_literal (val);
		}
break;
case 38:
#line 541 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_structop (yystack.l_mark[-2].op, yystack.l_mark[0].sval.ptr, 0); }
break;
case 39:
#line 543 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{
		  yyval.op = ast_structop (yystack.l_mark[-2].op, yystack.l_mark[0].sval.ptr, 1);
		  rust_ast = yyval.op;
		}
break;
case 40:
#line 548 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_structop_anonymous (yystack.l_mark[-2].op, yystack.l_mark[0].typed_val_int); }
break;
case 41:
#line 553 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_SUBSCRIPT, yystack.l_mark[-3].op, yystack.l_mark[-1].op); }
break;
case 42:
#line 558 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_unary (UNOP_PLUS, yystack.l_mark[0].op); }
break;
case 43:
#line 561 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_unary (UNOP_NEG, yystack.l_mark[0].op); }
break;
case 44:
#line 564 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{
		  /* Note that we provide a Rust-specific evaluator
		     override for UNOP_COMPLEMENT, so it can do the
		     right thing for both bool and integral
		     values.  */
		  yyval.op = ast_unary (UNOP_COMPLEMENT, yystack.l_mark[0].op);
		}
break;
case 45:
#line 573 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_unary (UNOP_IND, yystack.l_mark[0].op); }
break;
case 46:
#line 576 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_unary (UNOP_ADDR, yystack.l_mark[0].op); }
break;
case 47:
#line 579 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_unary (UNOP_ADDR, yystack.l_mark[0].op); }
break;
case 52:
#line 592 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_MUL, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 53:
#line 595 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_REPEAT, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 54:
#line 598 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_DIV, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 55:
#line 601 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_REM, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 56:
#line 604 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_LESS, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 57:
#line 607 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_GTR, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 58:
#line 610 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_BITWISE_AND, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 59:
#line 613 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_BITWISE_IOR, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 60:
#line 616 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_BITWISE_XOR, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 61:
#line 619 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_ADD, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 62:
#line 622 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_SUB, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 63:
#line 625 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_LOGICAL_OR, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 64:
#line 628 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_LOGICAL_AND, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 65:
#line 631 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_EQUAL, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 66:
#line 634 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_NOTEQUAL, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 67:
#line 637 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_LEQ, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 68:
#line 640 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_GEQ, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 69:
#line 643 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_LSH, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 70:
#line 646 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_RSH, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 71:
#line 651 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_cast (yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 72:
#line 656 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_operation (BINOP_ASSIGN, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 73:
#line 661 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_compound_assignment (yystack.l_mark[-1].opcode, yystack.l_mark[-2].op, yystack.l_mark[0].op); }
break;
case 74:
#line 667 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = yystack.l_mark[-1].op; }
break;
case 75:
#line 672 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{
		  yyval.params = OBSTACK_ZALLOC (&work_obstack, VEC (rust_op_ptr) *);
		  make_cleanup (VEC_cleanup (rust_op_ptr), yyval.params);
		  VEC_safe_push (rust_op_ptr, *yyval.params, yystack.l_mark[0].op);
		}
break;
case 76:
#line 678 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{
		  VEC_safe_push (rust_op_ptr, *yystack.l_mark[-2].params, yystack.l_mark[0].op);
		  yyval.params = yystack.l_mark[-2].params;
		}
break;
case 77:
#line 686 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{
		  /* The result can't be NULL.  */
		  yyval.params = OBSTACK_ZALLOC (&work_obstack, VEC (rust_op_ptr) *);
		  make_cleanup (VEC_cleanup (rust_op_ptr), yyval.params);
		}
break;
case 78:
#line 692 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.params = yystack.l_mark[0].params; }
break;
case 79:
#line 699 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.params = yystack.l_mark[-1].params; }
break;
case 80:
#line 704 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_call_ish (OP_FUNCALL, yystack.l_mark[-1].op, yystack.l_mark[0].params); }
break;
case 83:
#line 714 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.depth = 1; }
break;
case 84:
#line 716 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.depth = yystack.l_mark[-2].depth + 1; }
break;
case 85:
#line 721 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = yystack.l_mark[0].op; }
break;
case 86:
#line 723 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_path (yystack.l_mark[0].sval, NULL); }
break;
case 87:
#line 725 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_path (make_stoken ("self"), NULL); }
break;
case 89:
#line 731 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = super_name (yystack.l_mark[0].op, 0); }
break;
case 90:
#line 733 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = super_name (yystack.l_mark[0].op, yystack.l_mark[-1].depth); }
break;
case 91:
#line 735 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = crate_name (yystack.l_mark[0].op); }
break;
case 92:
#line 737 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{
		  /* This is a gdb extension to make it possible to
		     refer to items in other crates.  It just bypasses
		     adding the current crate to the front of the
		     name.  */
		  yyval.op = ast_path (rust_concat3 ("::", yystack.l_mark[0].op->left.sval.ptr, NULL),
				 yystack.l_mark[0].op->right.params);
		}
break;
case 93:
#line 749 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_path (yystack.l_mark[0].sval, NULL); }
break;
case 94:
#line 751 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{
		  yyval.op = ast_path (rust_concat3 (yystack.l_mark[-2].op->left.sval.ptr, "::",
					       yystack.l_mark[0].sval.ptr),
				 NULL);
		}
break;
case 95:
#line 757 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_path (yystack.l_mark[-4].op->left.sval, yystack.l_mark[-1].params); }
break;
case 96:
#line 759 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{
		  yyval.op = ast_path (yystack.l_mark[-4].op->left.sval, yystack.l_mark[-1].params);
		  rust_push_back ('>');
		}
break;
case 98:
#line 768 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = super_name (yystack.l_mark[0].op, 0); }
break;
case 99:
#line 770 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = super_name (yystack.l_mark[0].op, yystack.l_mark[-1].depth); }
break;
case 100:
#line 772 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = crate_name (yystack.l_mark[0].op); }
break;
case 101:
#line 774 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{
		  /* This is a gdb extension to make it possible to
		     refer to items in other crates.  It just bypasses
		     adding the current crate to the front of the
		     name.  */
		  yyval.op = ast_path (rust_concat3 ("::", yystack.l_mark[0].op->left.sval.ptr, NULL),
				 yystack.l_mark[0].op->right.params);
		}
break;
case 102:
#line 786 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_path (yystack.l_mark[0].sval, NULL); }
break;
case 103:
#line 788 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{
		  yyval.op = ast_path (rust_concat3 (yystack.l_mark[-2].op->left.sval.ptr, "::",
					       yystack.l_mark[0].sval.ptr),
				 NULL);
		}
break;
case 105:
#line 798 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_path (yystack.l_mark[-3].op->left.sval, yystack.l_mark[-1].params); }
break;
case 106:
#line 800 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{
		  yyval.op = ast_path (yystack.l_mark[-3].op->left.sval, yystack.l_mark[-1].params);
		  rust_push_back ('>');
		}
break;
case 108:
#line 809 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_array_type (yystack.l_mark[-3].op, yystack.l_mark[-1].typed_val_int); }
break;
case 109:
#line 811 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_array_type (yystack.l_mark[-3].op, yystack.l_mark[-1].typed_val_int); }
break;
case 110:
#line 813 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_slice_type (yystack.l_mark[-1].op); }
break;
case 111:
#line 815 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_reference_type (yystack.l_mark[0].op); }
break;
case 112:
#line 817 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_pointer_type (yystack.l_mark[0].op, 1); }
break;
case 113:
#line 819 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_pointer_type (yystack.l_mark[0].op, 0); }
break;
case 114:
#line 821 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_function_type (yystack.l_mark[0].op, yystack.l_mark[-3].params); }
break;
case 115:
#line 823 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.op = ast_tuple_type (yystack.l_mark[-1].params); }
break;
case 116:
#line 828 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.params = NULL; }
break;
case 117:
#line 830 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{ yyval.params = yystack.l_mark[0].params; }
break;
case 118:
#line 835 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{
		  VEC (rust_op_ptr) **result
		    = OBSTACK_ZALLOC (&work_obstack, VEC (rust_op_ptr) *);

		  make_cleanup (VEC_cleanup (rust_op_ptr), result);
		  VEC_safe_push (rust_op_ptr, *result, yystack.l_mark[0].op);
		  yyval.params = result;
		}
break;
case 119:
#line 844 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.y"
	{
		  VEC_safe_push (rust_op_ptr, *yystack.l_mark[-2].params, yystack.l_mark[0].op);
		  yyval.params = yystack.l_mark[-2].params;
		}
break;
#line 4184 "/usr/src/tools/gdb/../../external/gpl3/gdb/dist/gdb/rust-exp.c"
    }
    yystack.s_mark -= yym;
    yystate = *yystack.s_mark;
    yystack.l_mark -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yystack.s_mark = YYFINAL;
        *++yystack.l_mark = yyval;
        if (yychar < 0)
        {
            if ((yychar = YYLEX) < 0) yychar = YYEOF;
#if YYDEBUG
            if (yydebug)
            {
                yys = yyname[YYTRANSLATE(yychar)];
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == YYEOF) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yystack.s_mark, yystate);
#endif
    if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM)
    {
        goto yyoverflow;
    }
    *++yystack.s_mark = (YYINT) yystate;
    *++yystack.l_mark = yyval;
    goto yyloop;

yyoverflow:
    YYERROR_CALL("yacc stack overflow");

yyabort:
    yyfreestack(&yystack);
    return (1);

yyaccept:
    yyfreestack(&yystack);
    return (0);
}
