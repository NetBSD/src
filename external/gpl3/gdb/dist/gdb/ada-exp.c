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
#line 36 "ada-exp.y"


#include "defs.h"
#include <ctype.h>
#include "expression.h"
#include "value.h"
#include "parser-defs.h"
#include "language.h"
#include "ada-lang.h"
#include "bfd.h" /* Required by objfiles.h.  */
#include "symfile.h" /* Required by objfiles.h.  */
#include "objfiles.h" /* For have_full_symbols and have_partial_symbols */
#include "frame.h"
#include "block.h"
#include "ada-exp.h"

#define parse_type(ps) builtin_type (ps->gdbarch ())

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror,
   etc).  */
#define GDB_YY_REMAP_PREFIX ada_
#include "yy-remap.h"

struct name_info {
  struct symbol *sym;
  struct minimal_symbol *msym;
  const struct block *block;
  struct stoken stoken;
};

/* The state of the parser, used internally when we are parsing the
   expression.  */

static struct parser_state *pstate = NULL;

/* The original expression string.  */
static const char *original_expr;

int yyparse (void);

static int yylex (void);

static void yyerror (const char *);

static void write_int (struct parser_state *, LONGEST, struct type *);

static void write_object_renaming (struct parser_state *,
				   const struct block *, const char *, int,
				   const char *, int);

static struct type* write_var_or_type (struct parser_state *,
				       const struct block *, struct stoken);
static struct type *write_var_or_type_completion (struct parser_state *,
						  const struct block *,
						  struct stoken);

static void write_name_assoc (struct parser_state *, struct stoken);

static const struct block *block_lookup (const struct block *, const char *);

static void write_ambiguous_var (struct parser_state *,
				 const struct block *, const char *, int);

static struct type *type_int (struct parser_state *);

static struct type *type_long (struct parser_state *);

static struct type *type_long_long (struct parser_state *);

static struct type *type_long_double (struct parser_state *);

static struct type *type_for_char (struct parser_state *, ULONGEST);

static struct type *type_boolean (struct parser_state *);

static struct type *type_system_address (struct parser_state *);

static std::string find_completion_bounds (struct parser_state *);

using namespace expr;

/* Handle Ada type resolution for OP.  DEPROCEDURE_P and CONTEXT_TYPE
   are passed to the resolve method, if called.  */
static operation_up
resolve (operation_up &&op, bool deprocedure_p, struct type *context_type)
{
  operation_up result = std::move (op);
  ada_resolvable *res = dynamic_cast<ada_resolvable *> (result.get ());
  if (res != nullptr)
    return res->replace (std::move (result),
			 pstate->expout.get (),
			 deprocedure_p,
			 pstate->parse_completion,
			 pstate->block_tracker,
			 context_type);
  return result;
}

/* Like parser_state::pop, but handles Ada type resolution.
   DEPROCEDURE_P and CONTEXT_TYPE are passed to the resolve method, if
   called.  */
static operation_up
ada_pop (bool deprocedure_p = true, struct type *context_type = nullptr)
{
  /* Of course it's ok to call parser_state::pop here... */
  return resolve (pstate->pop (), deprocedure_p, context_type);
}

/* Like parser_state::wrap, but use ada_pop to pop the value.  */
template<typename T>
void
ada_wrap ()
{
  operation_up arg = ada_pop ();
  pstate->push_new<T> (std::move (arg));
}

/* Create and push an address-of operation, as appropriate for Ada.
   If TYPE is not NULL, the resulting operation will be wrapped in a
   cast to TYPE.  */
static void
ada_addrof (struct type *type = nullptr)
{
  operation_up arg = ada_pop (false);
  operation_up addr = make_operation<unop_addr_operation> (std::move (arg));
  operation_up wrapped
    = make_operation<ada_wrapped_operation> (std::move (addr));
  if (type != nullptr)
    wrapped = make_operation<unop_cast_operation> (std::move (wrapped), type);
  pstate->push (std::move (wrapped));
}

/* Handle operator overloading.  Either returns a function all
   operation wrapping the arguments, or it returns null, leaving the
   caller to construct the appropriate operation.  If RHS is null, a
   unary operator is assumed.  */
static operation_up
maybe_overload (enum exp_opcode op, operation_up &lhs, operation_up &rhs)
{
  struct value *args[2];

  int nargs = 1;
  args[0] = lhs->evaluate (nullptr, pstate->expout.get (),
			   EVAL_AVOID_SIDE_EFFECTS);
  if (rhs == nullptr)
    args[1] = nullptr;
  else
    {
      args[1] = rhs->evaluate (nullptr, pstate->expout.get (),
			       EVAL_AVOID_SIDE_EFFECTS);
      ++nargs;
    }

  block_symbol fn = ada_find_operator_symbol (op, pstate->parse_completion,
					      nargs, args);
  if (fn.symbol == nullptr)
    return {};

  if (symbol_read_needs_frame (fn.symbol))
    pstate->block_tracker->update (fn.block, INNERMOST_BLOCK_FOR_SYMBOLS);
  operation_up callee = make_operation<ada_var_value_operation> (fn);

  std::vector<operation_up> argvec;
  argvec.push_back (std::move (lhs));
  if (rhs != nullptr)
    argvec.push_back (std::move (rhs));
  return make_operation<ada_funcall_operation> (std::move (callee),
						std::move (argvec));
}

/* Like parser_state::wrap, but use ada_pop to pop the value, and
   handle unary overloading.  */
template<typename T>
void
ada_wrap_overload (enum exp_opcode op)
{
  operation_up arg = ada_pop ();
  operation_up empty;

  operation_up call = maybe_overload (op, arg, empty);
  if (call == nullptr)
    call = make_operation<T> (std::move (arg));
  pstate->push (std::move (call));
}

/* A variant of parser_state::wrap2 that uses ada_pop to pop both
   operands, and then pushes a new Ada-wrapped operation of the
   template type T.  */
template<typename T>
void
ada_un_wrap2 (enum exp_opcode op)
{
  operation_up rhs = ada_pop ();
  operation_up lhs = ada_pop ();

  operation_up wrapped = maybe_overload (op, lhs, rhs);
  if (wrapped == nullptr)
    {
      wrapped = make_operation<T> (std::move (lhs), std::move (rhs));
      wrapped = make_operation<ada_wrapped_operation> (std::move (wrapped));
    }
  pstate->push (std::move (wrapped));
}

/* A variant of parser_state::wrap2 that uses ada_pop to pop both
   operands.  Unlike ada_un_wrap2, ada_wrapped_operation is not
   used.  */
template<typename T>
void
ada_wrap2 (enum exp_opcode op)
{
  operation_up rhs = ada_pop ();
  operation_up lhs = ada_pop ();
  operation_up call = maybe_overload (op, lhs, rhs);
  if (call == nullptr)
    call = make_operation<T> (std::move (lhs), std::move (rhs));
  pstate->push (std::move (call));
}

/* A variant of parser_state::wrap2 that uses ada_pop to pop both
   operands.  OP is also passed to the constructor of the new binary
   operation.  */
template<typename T>
void
ada_wrap_op (enum exp_opcode op)
{
  operation_up rhs = ada_pop ();
  operation_up lhs = ada_pop ();
  operation_up call = maybe_overload (op, lhs, rhs);
  if (call == nullptr)
    call = make_operation<T> (op, std::move (lhs), std::move (rhs));
  pstate->push (std::move (call));
}

/* Pop three operands using ada_pop, then construct a new ternary
   operation of type T and push it.  */
template<typename T>
void
ada_wrap3 ()
{
  operation_up rhs = ada_pop ();
  operation_up mid = ada_pop ();
  operation_up lhs = ada_pop ();
  pstate->push_new<T> (std::move (lhs), std::move (mid), std::move (rhs));
}

/* Pop NARGS operands, then a callee operand, and use these to
   construct and push a new Ada function call operation.  */
static void
ada_funcall (int nargs)
{
  /* We use the ordinary pop here, because we're going to do
     resolution in a separate step, in order to handle array
     indices.  */
  std::vector<operation_up> args = pstate->pop_vector (nargs);
  /* Call parser_state::pop here, because we don't want to
     function-convert the callee slot of a call we're already
     constructing.  */
  operation_up callee = pstate->pop ();

  ada_var_value_operation *vvo
    = dynamic_cast<ada_var_value_operation *> (callee.get ());
  int array_arity = 0;
  struct type *callee_t = nullptr;
  if (vvo == nullptr
      || vvo->get_symbol ()->domain () != UNDEF_DOMAIN)
    {
      struct value *callee_v = callee->evaluate (nullptr,
						 pstate->expout.get (),
						 EVAL_AVOID_SIDE_EFFECTS);
      callee_t = ada_check_typedef (value_type (callee_v));
      array_arity = ada_array_arity (callee_t);
    }

  for (int i = 0; i < nargs; ++i)
    {
      struct type *subtype = nullptr;
      if (i < array_arity)
	subtype = ada_index_type (callee_t, i + 1, "array type");
      args[i] = resolve (std::move (args[i]), true, subtype);
    }

  std::unique_ptr<ada_funcall_operation> funcall
    (new ada_funcall_operation (std::move (callee), std::move (args)));
  funcall->resolve (pstate->expout.get (), true, pstate->parse_completion,
		    pstate->block_tracker, nullptr);
  pstate->push (std::move (funcall));
}

/* The components being constructed during this parse.  */
static std::vector<ada_component_up> components;

/* Create a new ada_component_up of the indicated type and arguments,
   and push it on the global 'components' vector.  */
template<typename T, typename... Arg>
void
push_component (Arg... args)
{
  components.emplace_back (new T (std::forward<Arg> (args)...));
}

/* Examine the final element of the 'components' vector, and return it
   as a pointer to an ada_choices_component.  The caller is
   responsible for ensuring that the final element is in fact an
   ada_choices_component.  */
static ada_choices_component *
choice_component ()
{
  ada_component *last = components.back ().get ();
  return gdb::checked_static_cast<ada_choices_component *> (last);
}

/* Pop the most recent component from the global stack, and return
   it.  */
static ada_component_up
pop_component ()
{
  ada_component_up result = std::move (components.back ());
  components.pop_back ();
  return result;
}

/* Pop the N most recent components from the global stack, and return
   them in a vector.  */
static std::vector<ada_component_up>
pop_components (int n)
{
  std::vector<ada_component_up> result (n);
  for (int i = 1; i <= n; ++i)
    result[n - i] = pop_component ();
  return result;
}

/* The associations being constructed during this parse.  */
static std::vector<ada_association_up> associations;

/* Create a new ada_association_up of the indicated type and
   arguments, and push it on the global 'associations' vector.  */
template<typename T, typename... Arg>
void
push_association (Arg... args)
{
  associations.emplace_back (new T (std::forward<Arg> (args)...));
}

/* Pop the most recent association from the global stack, and return
   it.  */
static ada_association_up
pop_association ()
{
  ada_association_up result = std::move (associations.back ());
  associations.pop_back ();
  return result;
}

/* Pop the N most recent associations from the global stack, and
   return them in a vector.  */
static std::vector<ada_association_up>
pop_associations (int n)
{
  std::vector<ada_association_up> result (n);
  for (int i = 1; i <= n; ++i)
    result[n - i] = pop_association ();
  return result;
}

/* Expression completer for attributes.  */
struct ada_tick_completer : public expr_completion_base
{
  explicit ada_tick_completer (std::string &&name)
    : m_name (std::move (name))
  {
  }

  bool complete (struct expression *exp,
		 completion_tracker &tracker) override;

private:

  std::string m_name;
};

/* Make a new ada_tick_completer and wrap it in a unique pointer.  */
static std::unique_ptr<expr_completion_base>
make_tick_completer (struct stoken tok)
{
  return (std::unique_ptr<expr_completion_base>
	  (new ada_tick_completer (std::string (tok.ptr, tok.length))));
}


#line 463 "ada-exp.c.tmp"

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
    NULL_PTR = 259,                /* NULL_PTR  */
    CHARLIT = 260,                 /* CHARLIT  */
    FLOAT = 261,                   /* FLOAT  */
    TRUEKEYWORD = 262,             /* TRUEKEYWORD  */
    FALSEKEYWORD = 263,            /* FALSEKEYWORD  */
    COLONCOLON = 264,              /* COLONCOLON  */
    STRING = 265,                  /* STRING  */
    NAME = 266,                    /* NAME  */
    DOT_ID = 267,                  /* DOT_ID  */
    TICK_COMPLETE = 268,           /* TICK_COMPLETE  */
    DOT_COMPLETE = 269,            /* DOT_COMPLETE  */
    NAME_COMPLETE = 270,           /* NAME_COMPLETE  */
    DOLLAR_VARIABLE = 271,         /* DOLLAR_VARIABLE  */
    ASSIGN = 272,                  /* ASSIGN  */
    _AND_ = 273,                   /* _AND_  */
    OR = 274,                      /* OR  */
    XOR = 275,                     /* XOR  */
    THEN = 276,                    /* THEN  */
    ELSE = 277,                    /* ELSE  */
    NOTEQUAL = 278,                /* NOTEQUAL  */
    LEQ = 279,                     /* LEQ  */
    GEQ = 280,                     /* GEQ  */
    IN = 281,                      /* IN  */
    DOTDOT = 282,                  /* DOTDOT  */
    UNARY = 283,                   /* UNARY  */
    MOD = 284,                     /* MOD  */
    REM = 285,                     /* REM  */
    STARSTAR = 286,                /* STARSTAR  */
    ABS = 287,                     /* ABS  */
    NOT = 288,                     /* NOT  */
    VAR = 289,                     /* VAR  */
    ARROW = 290,                   /* ARROW  */
    TICK_ACCESS = 291,             /* TICK_ACCESS  */
    TICK_ADDRESS = 292,            /* TICK_ADDRESS  */
    TICK_FIRST = 293,              /* TICK_FIRST  */
    TICK_LAST = 294,               /* TICK_LAST  */
    TICK_LENGTH = 295,             /* TICK_LENGTH  */
    TICK_MAX = 296,                /* TICK_MAX  */
    TICK_MIN = 297,                /* TICK_MIN  */
    TICK_MODULUS = 298,            /* TICK_MODULUS  */
    TICK_POS = 299,                /* TICK_POS  */
    TICK_RANGE = 300,              /* TICK_RANGE  */
    TICK_SIZE = 301,               /* TICK_SIZE  */
    TICK_TAG = 302,                /* TICK_TAG  */
    TICK_VAL = 303,                /* TICK_VAL  */
    NEW = 304,                     /* NEW  */
    OTHERS = 305                   /* OTHERS  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define INT 258
#define NULL_PTR 259
#define CHARLIT 260
#define FLOAT 261
#define TRUEKEYWORD 262
#define FALSEKEYWORD 263
#define COLONCOLON 264
#define STRING 265
#define NAME 266
#define DOT_ID 267
#define TICK_COMPLETE 268
#define DOT_COMPLETE 269
#define NAME_COMPLETE 270
#define DOLLAR_VARIABLE 271
#define ASSIGN 272
#define _AND_ 273
#define OR 274
#define XOR 275
#define THEN 276
#define ELSE 277
#define NOTEQUAL 278
#define LEQ 279
#define GEQ 280
#define IN 281
#define DOTDOT 282
#define UNARY 283
#define MOD 284
#define REM 285
#define STARSTAR 286
#define ABS 287
#define NOT 288
#define VAR 289
#define ARROW 290
#define TICK_ACCESS 291
#define TICK_ADDRESS 292
#define TICK_FIRST 293
#define TICK_LAST 294
#define TICK_LENGTH 295
#define TICK_MAX 296
#define TICK_MIN 297
#define TICK_MODULUS 298
#define TICK_POS 299
#define TICK_RANGE 300
#define TICK_SIZE 301
#define TICK_TAG 302
#define TICK_VAL 303
#define NEW 304
#define OTHERS 305

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 429 "ada-exp.y"

    LONGEST lval;
    struct {
      LONGEST val;
      struct type *type;
    } typed_val;
    struct {
      gdb_byte val[16];
      struct type *type;
    } typed_val_float;
    struct type *tval;
    struct stoken sval;
    const struct block *bval;
    struct internalvar *ivar;
  

#line 630 "ada-exp.c.tmp"

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
  YYSYMBOL_NULL_PTR = 4,                   /* NULL_PTR  */
  YYSYMBOL_CHARLIT = 5,                    /* CHARLIT  */
  YYSYMBOL_FLOAT = 6,                      /* FLOAT  */
  YYSYMBOL_TRUEKEYWORD = 7,                /* TRUEKEYWORD  */
  YYSYMBOL_FALSEKEYWORD = 8,               /* FALSEKEYWORD  */
  YYSYMBOL_COLONCOLON = 9,                 /* COLONCOLON  */
  YYSYMBOL_STRING = 10,                    /* STRING  */
  YYSYMBOL_NAME = 11,                      /* NAME  */
  YYSYMBOL_DOT_ID = 12,                    /* DOT_ID  */
  YYSYMBOL_TICK_COMPLETE = 13,             /* TICK_COMPLETE  */
  YYSYMBOL_DOT_COMPLETE = 14,              /* DOT_COMPLETE  */
  YYSYMBOL_NAME_COMPLETE = 15,             /* NAME_COMPLETE  */
  YYSYMBOL_DOLLAR_VARIABLE = 16,           /* DOLLAR_VARIABLE  */
  YYSYMBOL_ASSIGN = 17,                    /* ASSIGN  */
  YYSYMBOL__AND_ = 18,                     /* _AND_  */
  YYSYMBOL_OR = 19,                        /* OR  */
  YYSYMBOL_XOR = 20,                       /* XOR  */
  YYSYMBOL_THEN = 21,                      /* THEN  */
  YYSYMBOL_ELSE = 22,                      /* ELSE  */
  YYSYMBOL_23_ = 23,                       /* '='  */
  YYSYMBOL_NOTEQUAL = 24,                  /* NOTEQUAL  */
  YYSYMBOL_25_ = 25,                       /* '<'  */
  YYSYMBOL_26_ = 26,                       /* '>'  */
  YYSYMBOL_LEQ = 27,                       /* LEQ  */
  YYSYMBOL_GEQ = 28,                       /* GEQ  */
  YYSYMBOL_IN = 29,                        /* IN  */
  YYSYMBOL_DOTDOT = 30,                    /* DOTDOT  */
  YYSYMBOL_31_ = 31,                       /* '@'  */
  YYSYMBOL_32_ = 32,                       /* '+'  */
  YYSYMBOL_33_ = 33,                       /* '-'  */
  YYSYMBOL_34_ = 34,                       /* '&'  */
  YYSYMBOL_UNARY = 35,                     /* UNARY  */
  YYSYMBOL_36_ = 36,                       /* '*'  */
  YYSYMBOL_37_ = 37,                       /* '/'  */
  YYSYMBOL_MOD = 38,                       /* MOD  */
  YYSYMBOL_REM = 39,                       /* REM  */
  YYSYMBOL_STARSTAR = 40,                  /* STARSTAR  */
  YYSYMBOL_ABS = 41,                       /* ABS  */
  YYSYMBOL_NOT = 42,                       /* NOT  */
  YYSYMBOL_VAR = 43,                       /* VAR  */
  YYSYMBOL_ARROW = 44,                     /* ARROW  */
  YYSYMBOL_45_ = 45,                       /* '|'  */
  YYSYMBOL_TICK_ACCESS = 46,               /* TICK_ACCESS  */
  YYSYMBOL_TICK_ADDRESS = 47,              /* TICK_ADDRESS  */
  YYSYMBOL_TICK_FIRST = 48,                /* TICK_FIRST  */
  YYSYMBOL_TICK_LAST = 49,                 /* TICK_LAST  */
  YYSYMBOL_TICK_LENGTH = 50,               /* TICK_LENGTH  */
  YYSYMBOL_TICK_MAX = 51,                  /* TICK_MAX  */
  YYSYMBOL_TICK_MIN = 52,                  /* TICK_MIN  */
  YYSYMBOL_TICK_MODULUS = 53,              /* TICK_MODULUS  */
  YYSYMBOL_TICK_POS = 54,                  /* TICK_POS  */
  YYSYMBOL_TICK_RANGE = 55,                /* TICK_RANGE  */
  YYSYMBOL_TICK_SIZE = 56,                 /* TICK_SIZE  */
  YYSYMBOL_TICK_TAG = 57,                  /* TICK_TAG  */
  YYSYMBOL_TICK_VAL = 58,                  /* TICK_VAL  */
  YYSYMBOL_59_ = 59,                       /* '.'  */
  YYSYMBOL_60_ = 60,                       /* '('  */
  YYSYMBOL_61_ = 61,                       /* '['  */
  YYSYMBOL_NEW = 62,                       /* NEW  */
  YYSYMBOL_OTHERS = 63,                    /* OTHERS  */
  YYSYMBOL_64_ = 64,                       /* ';'  */
  YYSYMBOL_65_ = 65,                       /* ')'  */
  YYSYMBOL_66_ = 66,                       /* '\''  */
  YYSYMBOL_67_ = 67,                       /* ','  */
  YYSYMBOL_68_ = 68,                       /* '{'  */
  YYSYMBOL_69_ = 69,                       /* '}'  */
  YYSYMBOL_70_ = 70,                       /* ']'  */
  YYSYMBOL_YYACCEPT = 71,                  /* $accept  */
  YYSYMBOL_start = 72,                     /* start  */
  YYSYMBOL_exp1 = 73,                      /* exp1  */
  YYSYMBOL_primary = 74,                   /* primary  */
  YYSYMBOL_simple_exp = 75,                /* simple_exp  */
  YYSYMBOL_arglist = 76,                   /* arglist  */
  YYSYMBOL_relation = 77,                  /* relation  */
  YYSYMBOL_exp = 78,                       /* exp  */
  YYSYMBOL_and_exp = 79,                   /* and_exp  */
  YYSYMBOL_and_then_exp = 80,              /* and_then_exp  */
  YYSYMBOL_or_exp = 81,                    /* or_exp  */
  YYSYMBOL_or_else_exp = 82,               /* or_else_exp  */
  YYSYMBOL_xor_exp = 83,                   /* xor_exp  */
  YYSYMBOL_tick_arglist = 84,              /* tick_arglist  */
  YYSYMBOL_type_prefix = 85,               /* type_prefix  */
  YYSYMBOL_opt_type_prefix = 86,           /* opt_type_prefix  */
  YYSYMBOL_var_or_type = 87,               /* var_or_type  */
  YYSYMBOL_block = 88,                     /* block  */
  YYSYMBOL_aggregate = 89,                 /* aggregate  */
  YYSYMBOL_aggregate_component_list = 90,  /* aggregate_component_list  */
  YYSYMBOL_positional_list = 91,           /* positional_list  */
  YYSYMBOL_component_groups = 92,          /* component_groups  */
  YYSYMBOL_others = 93,                    /* others  */
  YYSYMBOL_component_group = 94,           /* component_group  */
  YYSYMBOL_component_associations = 95     /* component_associations  */
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
#define YYFINAL  58
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   801

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  71
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  25
/* YYNRULES -- Number of rules.  */
#define YYNRULES  119
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  230

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   305


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
       2,     2,     2,     2,     2,     2,     2,     2,    34,    66,
      60,    65,    36,    32,    67,    33,    59,    37,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    64,
      25,    23,    26,     2,    31,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    61,     2,    70,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    68,    45,    69,     2,     2,     2,     2,
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
      15,    16,    17,    18,    19,    20,    21,    22,    24,    27,
      28,    29,    30,    35,    38,    39,    40,    41,    42,    43,
      44,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    62,    63
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   491,   491,   495,   496,   498,   514,   527,   541,   543,
     558,   570,   572,   580,   591,   597,   601,   608,   611,   615,
     631,   638,   642,   645,   647,   649,   651,   655,   668,   672,
     676,   680,   684,   688,   692,   696,   700,   704,   707,   711,
     715,   719,   721,   728,   736,   739,   747,   758,   762,   766,
     770,   771,   772,   773,   774,   775,   779,   782,   788,   791,
     797,   800,   806,   808,   812,   815,   828,   830,   832,   836,
     842,   848,   854,   856,   858,   860,   862,   864,   870,   880,
     882,   887,   896,   899,   903,   907,   913,   924,   932,   939,
     941,   945,   949,   951,   957,   959,   965,   973,   984,   986,
     991,  1002,  1003,  1009,  1014,  1020,  1029,  1030,  1031,  1035,
    1042,  1055,  1061,  1067,  1076,  1081,  1086,  1100,  1102,  1104
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
  "\"end of file\"", "error", "\"invalid token\"", "INT", "NULL_PTR",
  "CHARLIT", "FLOAT", "TRUEKEYWORD", "FALSEKEYWORD", "COLONCOLON",
  "STRING", "NAME", "DOT_ID", "TICK_COMPLETE", "DOT_COMPLETE",
  "NAME_COMPLETE", "DOLLAR_VARIABLE", "ASSIGN", "_AND_", "OR", "XOR",
  "THEN", "ELSE", "'='", "NOTEQUAL", "'<'", "'>'", "LEQ", "GEQ", "IN",
  "DOTDOT", "'@'", "'+'", "'-'", "'&'", "UNARY", "'*'", "'/'", "MOD",
  "REM", "STARSTAR", "ABS", "NOT", "VAR", "ARROW", "'|'", "TICK_ACCESS",
  "TICK_ADDRESS", "TICK_FIRST", "TICK_LAST", "TICK_LENGTH", "TICK_MAX",
  "TICK_MIN", "TICK_MODULUS", "TICK_POS", "TICK_RANGE", "TICK_SIZE",
  "TICK_TAG", "TICK_VAL", "'.'", "'('", "'['", "NEW", "OTHERS", "';'",
  "')'", "'\\''", "','", "'{'", "'}'", "']'", "$accept", "start", "exp1",
  "primary", "simple_exp", "arglist", "relation", "exp", "and_exp",
  "and_then_exp", "or_exp", "or_else_exp", "xor_exp", "tick_arglist",
  "type_prefix", "opt_type_prefix", "var_or_type", "block", "aggregate",
  "aggregate_component_list", "positional_list", "component_groups",
  "others", "component_group", "component_associations", YY_NULLPTRPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-106)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-82)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     421,  -106,  -106,  -106,  -106,  -106,  -106,  -106,     6,  -106,
    -106,   421,   421,   541,   541,   421,   421,   274,    45,    17,
      61,    10,   566,   723,    26,  -106,    48,    52,    57,    60,
      76,   -32,    49,    99,    32,  -106,  -106,  -106,   621,     2,
       2,    -7,    -7,     2,     2,     4,    54,   -29,   660,    35,
      39,   274,  -106,  -106,    38,  -106,  -106,    37,  -106,   421,
    -106,  -106,  -106,   421,  -106,  -106,    51,    51,    51,  -106,
    -106,   260,   421,   421,   421,   421,   421,   421,   421,   421,
     421,   421,   421,   421,   421,   421,   421,   421,   421,    79,
     340,   381,   421,   421,    92,   421,    94,   421,  -106,    71,
      72,    73,    75,   260,    77,    22,  -106,   421,   461,   421,
    -106,   421,   421,   461,  -106,  -106,    47,  -106,   274,   541,
    -106,  -106,   114,  -106,  -106,  -106,    16,   683,   -38,  -106,
      66,   553,   553,   553,   553,   553,   553,   582,   534,   171,
     761,     2,     2,     2,    98,    98,    98,    98,    98,   421,
     421,  -106,   421,  -106,  -106,  -106,   421,  -106,   421,  -106,
     421,   421,   421,   421,   703,   -10,   421,  -106,  -106,  -106,
     736,  -106,  -106,   326,  -106,  -106,  -106,  -106,    -7,    89,
     421,   421,  -106,   501,  -106,    51,   421,   605,   752,   192,
    -106,  -106,  -106,  -106,    93,    97,   100,   103,   421,  -106,
     105,   421,   461,  -106,  -106,    90,    21,  -106,  -106,   553,
      51,   421,  -106,   421,   421,  -106,   109,  -106,  -106,  -106,
    -106,   421,  -106,   553,   107,   108,  -106,  -106,  -106,  -106
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
      83,    84,    87,    85,    86,    89,    90,    88,    92,    93,
      15,    83,    83,    83,    83,    83,    83,    83,     0,     0,
       0,     2,    17,    37,    50,     3,    51,    52,    53,    54,
      55,    82,     0,    14,     0,    16,    98,    96,    17,    19,
      18,   118,   117,    21,    20,    92,     0,     0,    37,     3,
       0,    83,   101,   106,   107,   110,    91,     0,     1,    83,
       6,    68,     7,    83,    66,    67,    79,    79,    79,    72,
      73,    83,    83,    83,    83,    83,    83,    83,    83,    83,
      83,    83,    83,    83,    83,    83,    83,    83,    83,     0,
      83,    83,    83,    83,     0,    83,     0,    83,    78,     0,
       0,     0,     0,    83,     0,    94,    95,    83,    83,    83,
      13,    83,    83,    83,   104,   100,   102,   103,    83,    83,
       4,     5,     0,    69,    70,    71,    92,    37,     0,    23,
       0,    38,    39,    48,    49,    40,    47,    17,     0,    14,
      33,    34,    36,    35,    29,    30,    32,    31,    28,    83,
      83,    56,    83,    60,    64,    57,    83,    61,    83,    65,
      83,    83,    83,    83,    37,     0,    83,    99,    97,   111,
       0,   114,   109,     0,   112,   115,   105,   108,    27,     0,
      83,    83,     8,    83,   119,    79,    83,    17,     0,    14,
      58,    62,    59,    63,     0,     0,     0,     0,    83,     9,
       0,    83,    83,    80,    24,     0,    92,    25,    42,    41,
      79,    83,    77,    83,    83,    76,     0,    10,   113,   116,
      11,    83,    45,    44,     0,     0,    12,    26,    75,    74
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -106,  -106,   158,    20,     7,    74,   -81,     0,  -106,  -106,
    -106,  -106,  -106,   -66,  -106,  -106,   -15,  -106,  -106,  -106,
    -106,   -45,  -106,  -106,  -105
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,    20,    21,    38,    23,   128,    24,   129,    26,    27,
      28,    29,    30,   123,    31,    32,    33,    34,    35,    50,
      51,    52,    53,    54,    55
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      25,   124,   125,   171,    57,    60,   117,    62,   175,   151,
     153,   154,   155,    36,   157,    36,   159,    49,    39,    40,
      22,    98,    43,    44,    48,    36,    99,   182,     8,   183,
      36,   167,     9,    41,    42,    59,   110,    22,    84,    85,
      86,    87,    88,   105,    90,    91,    92,   106,   107,   108,
      37,   116,    37,    71,    72,   199,    56,   183,    48,   120,
     180,    58,    37,   121,   139,   221,    93,    37,   168,   190,
      94,   191,   130,   177,    59,   192,    95,   193,   127,    96,
     131,   132,   133,   134,   135,   136,   138,   140,   141,   142,
     143,   144,   145,   146,   147,   148,    97,   219,   109,   137,
     100,   101,   114,   102,   115,   118,   119,   169,   149,   172,
     164,   122,   174,   156,   176,   170,   158,   179,   173,   208,
     170,    80,    81,    82,    83,   170,    84,    85,    86,    87,
      88,   160,   161,   162,   189,   163,   184,   166,    88,   178,
      80,    81,    82,    83,   222,    84,    85,    86,    87,    88,
     -81,   -81,   -81,   -81,   203,   220,   188,   -81,   212,   103,
     194,   195,   196,   197,   213,   104,   200,   214,   215,   187,
     217,   -43,   228,   229,   226,    47,     0,   165,     0,     0,
     204,     0,     0,   207,     0,     0,     0,     0,   205,   -43,
     -43,   -43,   -46,   209,     0,     0,     0,     0,     0,     0,
       0,   218,     0,     0,     0,   216,     0,     0,     0,   170,
     -46,   -46,   -46,   224,   225,     0,     0,     0,   223,     0,
       0,   227,   -81,   -81,   -81,   -81,     0,     0,     0,   -81,
       0,   103,     0,     0,     0,   -43,   -43,   104,   -43,     0,
       0,   -43,     0,   -81,   -81,   -81,   -81,     0,     0,     0,
     -81,     0,   103,     0,     0,     0,   -46,   -46,   104,   -46,
       0,     0,   -46,     1,     2,     3,     4,     5,     6,     0,
       7,   126,     0,     0,     0,     9,    10,     1,     2,     3,
       4,     5,     6,     0,     7,    45,     0,     0,     0,     9,
      10,     0,    11,    12,    13,     0,    14,     0,     0,     0,
       0,    15,    16,     0,     0,     0,    11,    12,    13,     0,
      14,     0,     0,     0,     0,    15,    16,     0,     0,     0,
      17,     0,    18,     0,     0,   -22,     0,   -22,    19,     0,
       0,     0,     0,     0,    17,     0,    18,    46,     0,     0,
       0,     0,    19,     1,     2,     3,     4,     5,     6,     0,
       7,     8,     0,     0,     0,     9,    10,    80,    81,    82,
      83,   150,    84,    85,    86,    87,    88,     0,     0,     0,
     201,   202,    11,    12,    13,     0,    14,     0,     0,     0,
       0,    15,    16,     0,     1,     2,     3,     4,     5,     6,
       0,     7,     8,     0,     0,     0,     9,    10,     0,     0,
      17,     0,    18,   152,     0,     0,     0,     0,    19,     0,
       0,     0,     0,    11,    12,    13,     0,    14,     0,     0,
       0,     0,    15,    16,     1,     2,     3,     4,     5,     6,
       0,     7,     8,     0,     0,     0,     9,    10,     0,     0,
       0,    17,     0,    18,     0,     0,     0,     0,     0,    19,
       0,     0,     0,    11,    12,    13,     0,    14,     0,     0,
       0,     0,    15,    16,     1,     2,     3,     4,     5,     6,
       0,     7,    45,     0,     0,     0,     9,    10,     0,     0,
       0,    17,     0,    18,     0,     0,     0,     0,     0,    19,
       0,     0,     0,    11,    12,    13,     0,    14,     0,     0,
       0,     0,    15,    16,     1,     2,     3,     4,     5,     6,
       0,     7,   206,     0,     0,     0,     9,    10,     0,     0,
       0,    17,     0,    18,     0,     0,     0,     0,     0,    19,
       0,     0,     0,    11,    12,    13,     0,    14,     0,     0,
       0,     0,    15,    16,     1,     2,     3,     4,     5,     6,
       0,     7,     8,     0,     0,     0,     9,    10,     0,     0,
       0,    17,     0,    18,   186,    80,    81,    82,    83,    19,
      84,    85,    86,    87,    88,    13,     0,    14,    60,    61,
      62,     0,     0,    63,    80,    81,    82,    83,     0,    84,
      85,    86,    87,    88,    60,    61,    62,     0,     0,     0,
       0,    17,     0,    18,     0,     0,     0,     0,     0,    19,
       0,     0,    64,    65,    66,    67,    68,    60,    61,    62,
       0,     0,    69,    70,     0,     0,    71,    72,    64,    65,
      66,    67,    68,    60,    61,    62,     0,   185,    69,    70,
       0,     0,    71,    72,     0,     0,     0,     0,     0,     0,
       0,    64,    65,    66,    67,    68,     0,     0,     0,     0,
     210,    69,    70,     0,     0,    71,    72,    64,    65,    66,
      67,    68,     0,     0,     0,     0,     0,    69,    70,     0,
       0,    71,    72,    73,    74,    75,    76,    77,    78,    79,
     111,    80,    81,    82,    83,     0,    84,    85,    86,    87,
      88,     0,    89,     0,   112,   113,    73,    74,    75,    76,
      77,    78,    79,   181,    80,    81,    82,    83,     0,    84,
      85,    86,    87,    88,     0,    89,    73,    74,    75,    76,
      77,    78,    79,   198,    80,    81,    82,    83,     0,    84,
      85,    86,    87,    88,     0,    89,    73,    74,    75,    76,
      77,    78,    79,     0,    80,    81,    82,    83,     0,    84,
      85,    86,    87,    88,     0,    89,   111,    80,    81,    82,
      83,     0,    84,    85,    86,    87,    88,     0,     0,     0,
     112,   113,   211,    80,    81,    82,    83,     0,    84,    85,
      86,    87,    88,    81,    82,    83,     0,    84,    85,    86,
      87,    88
};

static const yytype_int16 yycheck[] =
{
       0,    67,    68,   108,    19,    12,    51,    14,   113,    90,
      91,    92,    93,     9,    95,     9,    97,    17,    11,    12,
       0,    53,    15,    16,    17,     9,    58,    65,    11,    67,
       9,     9,    15,    13,    14,    64,    65,    17,    36,    37,
      38,    39,    40,    11,    18,    19,    20,    15,    44,    45,
      46,    51,    46,    60,    61,    65,    11,    67,    51,    59,
      44,     0,    46,    63,    79,    44,    18,    46,    46,   150,
      18,   152,    72,   118,    64,   156,    19,   158,    71,    19,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    20,   202,    44,    79,
      51,    52,    67,    54,    65,    67,    69,   107,    29,   109,
     103,    60,   112,    21,    67,   108,    22,     3,   111,   185,
     113,    31,    32,    33,    34,   118,    36,    37,    38,    39,
      40,    60,    60,    60,   149,    60,    70,    60,    40,   119,
      31,    32,    33,    34,   210,    36,    37,    38,    39,    40,
      51,    52,    53,    54,    65,    65,   149,    58,    65,    60,
     160,   161,   162,   163,    67,    66,   166,    67,    65,   149,
      65,     0,    65,    65,    65,    17,    -1,   103,    -1,    -1,
     180,    -1,    -1,   183,    -1,    -1,    -1,    -1,   181,    18,
      19,    20,     0,   186,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   201,    -1,    -1,    -1,   198,    -1,    -1,    -1,   202,
      18,    19,    20,   213,   214,    -1,    -1,    -1,   211,    -1,
      -1,   221,    51,    52,    53,    54,    -1,    -1,    -1,    58,
      -1,    60,    -1,    -1,    -1,    64,    65,    66,    67,    -1,
      -1,    70,    -1,    51,    52,    53,    54,    -1,    -1,    -1,
      58,    -1,    60,    -1,    -1,    -1,    64,    65,    66,    67,
      -1,    -1,    70,     3,     4,     5,     6,     7,     8,    -1,
      10,    11,    -1,    -1,    -1,    15,    16,     3,     4,     5,
       6,     7,     8,    -1,    10,    11,    -1,    -1,    -1,    15,
      16,    -1,    32,    33,    34,    -1,    36,    -1,    -1,    -1,
      -1,    41,    42,    -1,    -1,    -1,    32,    33,    34,    -1,
      36,    -1,    -1,    -1,    -1,    41,    42,    -1,    -1,    -1,
      60,    -1,    62,    -1,    -1,    65,    -1,    67,    68,    -1,
      -1,    -1,    -1,    -1,    60,    -1,    62,    63,    -1,    -1,
      -1,    -1,    68,     3,     4,     5,     6,     7,     8,    -1,
      10,    11,    -1,    -1,    -1,    15,    16,    31,    32,    33,
      34,    21,    36,    37,    38,    39,    40,    -1,    -1,    -1,
      44,    45,    32,    33,    34,    -1,    36,    -1,    -1,    -1,
      -1,    41,    42,    -1,     3,     4,     5,     6,     7,     8,
      -1,    10,    11,    -1,    -1,    -1,    15,    16,    -1,    -1,
      60,    -1,    62,    22,    -1,    -1,    -1,    -1,    68,    -1,
      -1,    -1,    -1,    32,    33,    34,    -1,    36,    -1,    -1,
      -1,    -1,    41,    42,     3,     4,     5,     6,     7,     8,
      -1,    10,    11,    -1,    -1,    -1,    15,    16,    -1,    -1,
      -1,    60,    -1,    62,    -1,    -1,    -1,    -1,    -1,    68,
      -1,    -1,    -1,    32,    33,    34,    -1,    36,    -1,    -1,
      -1,    -1,    41,    42,     3,     4,     5,     6,     7,     8,
      -1,    10,    11,    -1,    -1,    -1,    15,    16,    -1,    -1,
      -1,    60,    -1,    62,    -1,    -1,    -1,    -1,    -1,    68,
      -1,    -1,    -1,    32,    33,    34,    -1,    36,    -1,    -1,
      -1,    -1,    41,    42,     3,     4,     5,     6,     7,     8,
      -1,    10,    11,    -1,    -1,    -1,    15,    16,    -1,    -1,
      -1,    60,    -1,    62,    -1,    -1,    -1,    -1,    -1,    68,
      -1,    -1,    -1,    32,    33,    34,    -1,    36,    -1,    -1,
      -1,    -1,    41,    42,     3,     4,     5,     6,     7,     8,
      -1,    10,    11,    -1,    -1,    -1,    15,    16,    -1,    -1,
      -1,    60,    -1,    62,    30,    31,    32,    33,    34,    68,
      36,    37,    38,    39,    40,    34,    -1,    36,    12,    13,
      14,    -1,    -1,    17,    31,    32,    33,    34,    -1,    36,
      37,    38,    39,    40,    12,    13,    14,    -1,    -1,    -1,
      -1,    60,    -1,    62,    -1,    -1,    -1,    -1,    -1,    68,
      -1,    -1,    46,    47,    48,    49,    50,    12,    13,    14,
      -1,    -1,    56,    57,    -1,    -1,    60,    61,    46,    47,
      48,    49,    50,    12,    13,    14,    -1,    55,    56,    57,
      -1,    -1,    60,    61,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    46,    47,    48,    49,    50,    -1,    -1,    -1,    -1,
      55,    56,    57,    -1,    -1,    60,    61,    46,    47,    48,
      49,    50,    -1,    -1,    -1,    -1,    -1,    56,    57,    -1,
      -1,    60,    61,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    -1,    36,    37,    38,    39,
      40,    -1,    42,    -1,    44,    45,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    -1,    36,
      37,    38,    39,    40,    -1,    42,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    -1,    36,
      37,    38,    39,    40,    -1,    42,    23,    24,    25,    26,
      27,    28,    29,    -1,    31,    32,    33,    34,    -1,    36,
      37,    38,    39,    40,    -1,    42,    30,    31,    32,    33,
      34,    -1,    36,    37,    38,    39,    40,    -1,    -1,    -1,
      44,    45,    30,    31,    32,    33,    34,    -1,    36,    37,
      38,    39,    40,    32,    33,    34,    -1,    36,    37,    38,
      39,    40
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,    10,    11,    15,
      16,    32,    33,    34,    36,    41,    42,    60,    62,    68,
      72,    73,    74,    75,    77,    78,    79,    80,    81,    82,
      83,    85,    86,    87,    88,    89,     9,    46,    74,    75,
      75,    74,    74,    75,    75,    11,    63,    73,    75,    78,
      90,    91,    92,    93,    94,    95,    11,    87,     0,    64,
      12,    13,    14,    17,    46,    47,    48,    49,    50,    56,
      57,    60,    61,    23,    24,    25,    26,    27,    28,    29,
      31,    32,    33,    34,    36,    37,    38,    39,    40,    42,
      18,    19,    20,    18,    18,    19,    19,    20,    53,    58,
      51,    52,    54,    60,    66,    11,    15,    44,    45,    44,
      65,    30,    44,    45,    67,    65,    78,    92,    67,    69,
      78,    78,    60,    84,    84,    84,    11,    75,    76,    78,
      78,    75,    75,    75,    75,    75,    75,    74,    75,    87,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    29,
      21,    77,    22,    77,    77,    77,    21,    77,    22,    77,
      60,    60,    60,    60,    75,    76,    60,     9,    46,    78,
      75,    95,    78,    75,    78,    95,    67,    92,    74,     3,
      44,    30,    65,    67,    70,    55,    30,    74,    75,    87,
      77,    77,    77,    77,    78,    78,    78,    78,    30,    65,
      78,    44,    45,    65,    78,    75,    11,    78,    84,    75,
      55,    30,    65,    67,    67,    65,    75,    65,    78,    95,
      65,    44,    84,    75,    78,    78,    65,    78,    65,    65
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    71,    72,    73,    73,    73,    74,    74,    74,    74,
      74,    74,    74,    74,    74,    74,    74,    75,    75,    75,
      75,    75,    76,    76,    76,    76,    76,    74,    75,    75,
      75,    75,    75,    75,    75,    75,    75,    77,    77,    77,
      77,    77,    77,    77,    77,    77,    77,    77,    77,    77,
      78,    78,    78,    78,    78,    78,    79,    79,    80,    80,
      81,    81,    82,    82,    83,    83,    74,    74,    74,    74,
      74,    74,    74,    74,    74,    74,    74,    74,    74,    84,
      84,    85,    86,    86,    74,    74,    74,    74,    74,    74,
      74,    74,    87,    87,    87,    87,    87,    87,    88,    88,
      89,    90,    90,    90,    91,    91,    92,    92,    92,    93,
      94,    95,    95,    95,    95,    95,    95,    74,    74,    74
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     3,     3,     2,     2,     4,     4,
       5,     6,     6,     3,     1,     1,     1,     1,     2,     2,
       2,     2,     0,     1,     3,     3,     5,     4,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     1,     3,     3,
       3,     5,     5,     3,     6,     6,     4,     3,     3,     3,
       1,     1,     1,     1,     1,     1,     3,     3,     4,     4,
       3,     3,     4,     4,     3,     3,     2,     2,     2,     3,
       3,     3,     2,     2,     7,     7,     5,     5,     2,     0,
       3,     1,     1,     0,     1,     1,     1,     1,     1,     1,
       1,     2,     1,     1,     2,     2,     2,     3,     2,     3,
       3,     1,     2,     2,     2,     3,     1,     1,     3,     3,
       1,     3,     3,     5,     3,     3,     5,     2,     2,     4
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
  case 4: /* exp1: exp1 ';' exp  */
#line 497 "ada-exp.y"
                        { ada_wrap2<comma_operation> (BINOP_COMMA); }
#line 1972 "ada-exp.c.tmp"
    break;

  case 5: /* exp1: primary ASSIGN exp  */
#line 499 "ada-exp.y"
                        {
			  operation_up rhs = pstate->pop ();
			  operation_up lhs = ada_pop ();
			  value *lhs_val
			    = lhs->evaluate (nullptr, pstate->expout.get (),
					     EVAL_AVOID_SIDE_EFFECTS);
			  rhs = resolve (std::move (rhs), true,
					 value_type (lhs_val));
			  pstate->push_new<ada_assign_operation>
			    (std::move (lhs), std::move (rhs));
			}
#line 1988 "ada-exp.c.tmp"
    break;

  case 6: /* primary: primary DOT_ID  */
#line 515 "ada-exp.y"
                        {
			  if (strcmp ((yyvsp[0].sval).ptr, "all") == 0)
			    ada_wrap<ada_unop_ind_operation> ();
			  else
			    {
			      operation_up arg = ada_pop ();
			      pstate->push_new<ada_structop_operation>
				(std::move (arg), copy_name ((yyvsp[0].sval)));
			    }
			}
#line 2003 "ada-exp.c.tmp"
    break;

  case 7: /* primary: primary DOT_COMPLETE  */
#line 528 "ada-exp.y"
                        {
			  /* This is done even for ".all", because
			     that might be a prefix.  */
			  operation_up arg = ada_pop ();
			  ada_structop_operation *str_op
			    = (new ada_structop_operation
			       (std::move (arg), copy_name ((yyvsp[0].sval))));
			  str_op->set_prefix (find_completion_bounds (pstate));
			  pstate->push (operation_up (str_op));
			  pstate->mark_struct_expression (str_op);
			}
#line 2019 "ada-exp.c.tmp"
    break;

  case 8: /* primary: primary '(' arglist ')'  */
#line 542 "ada-exp.y"
                        { ada_funcall ((yyvsp[-1].lval)); }
#line 2025 "ada-exp.c.tmp"
    break;

  case 9: /* primary: var_or_type '(' arglist ')'  */
#line 544 "ada-exp.y"
                        {
			  if ((yyvsp[-3].tval) != NULL)
			    {
			      if ((yyvsp[-1].lval) != 1)
				error (_("Invalid conversion"));
			      operation_up arg = ada_pop ();
			      pstate->push_new<unop_cast_operation>
				(std::move (arg), (yyvsp[-3].tval));
			    }
			  else
			    ada_funcall ((yyvsp[-1].lval));
			}
#line 2042 "ada-exp.c.tmp"
    break;

  case 10: /* primary: var_or_type '\'' '(' exp ')'  */
#line 559 "ada-exp.y"
                        {
			  if ((yyvsp[-4].tval) == NULL)
			    error (_("Type required for qualification"));
			  operation_up arg = ada_pop (true,
						      check_typedef ((yyvsp[-4].tval)));
			  pstate->push_new<ada_qual_operation>
			    (std::move (arg), (yyvsp[-4].tval));
			}
#line 2055 "ada-exp.c.tmp"
    break;

  case 11: /* primary: primary '(' simple_exp DOTDOT simple_exp ')'  */
#line 571 "ada-exp.y"
                        { ada_wrap3<ada_ternop_slice_operation> (); }
#line 2061 "ada-exp.c.tmp"
    break;

  case 12: /* primary: var_or_type '(' simple_exp DOTDOT simple_exp ')'  */
#line 573 "ada-exp.y"
                        { if ((yyvsp[-5].tval) == NULL) 
			    ada_wrap3<ada_ternop_slice_operation> ();
			  else
			    error (_("Cannot slice a type"));
			}
#line 2071 "ada-exp.c.tmp"
    break;

  case 13: /* primary: '(' exp1 ')'  */
#line 580 "ada-exp.y"
                                { }
#line 2077 "ada-exp.c.tmp"
    break;

  case 14: /* primary: var_or_type  */
#line 592 "ada-exp.y"
                        { if ((yyvsp[0].tval) != NULL)
			    pstate->push_new<type_operation> ((yyvsp[0].tval));
			}
#line 2085 "ada-exp.c.tmp"
    break;

  case 15: /* primary: DOLLAR_VARIABLE  */
#line 598 "ada-exp.y"
                        { pstate->push_dollar ((yyvsp[0].sval)); }
#line 2091 "ada-exp.c.tmp"
    break;

  case 16: /* primary: aggregate  */
#line 602 "ada-exp.y"
                        {
			  pstate->push_new<ada_aggregate_operation>
			    (pop_component ());
			}
#line 2100 "ada-exp.c.tmp"
    break;

  case 18: /* simple_exp: '-' simple_exp  */
#line 612 "ada-exp.y"
                        { ada_wrap_overload<ada_neg_operation> (UNOP_NEG); }
#line 2106 "ada-exp.c.tmp"
    break;

  case 19: /* simple_exp: '+' simple_exp  */
#line 616 "ada-exp.y"
                        {
			  operation_up arg = ada_pop ();
			  operation_up empty;

			  /* If an overloaded operator was found, use
			     it.  Otherwise, unary + has no effect and
			     the argument can be pushed instead.  */
			  operation_up call = maybe_overload (UNOP_PLUS, arg,
							      empty);
			  if (call != nullptr)
			    arg = std::move (call);
			  pstate->push (std::move (arg));
			}
#line 2124 "ada-exp.c.tmp"
    break;

  case 20: /* simple_exp: NOT simple_exp  */
#line 632 "ada-exp.y"
                        {
			  ada_wrap_overload<unary_logical_not_operation>
			    (UNOP_LOGICAL_NOT);
			}
#line 2133 "ada-exp.c.tmp"
    break;

  case 21: /* simple_exp: ABS simple_exp  */
#line 639 "ada-exp.y"
                        { ada_wrap_overload<ada_abs_operation> (UNOP_ABS); }
#line 2139 "ada-exp.c.tmp"
    break;

  case 22: /* arglist: %empty  */
#line 642 "ada-exp.y"
                        { (yyval.lval) = 0; }
#line 2145 "ada-exp.c.tmp"
    break;

  case 23: /* arglist: exp  */
#line 646 "ada-exp.y"
                        { (yyval.lval) = 1; }
#line 2151 "ada-exp.c.tmp"
    break;

  case 24: /* arglist: NAME ARROW exp  */
#line 648 "ada-exp.y"
                        { (yyval.lval) = 1; }
#line 2157 "ada-exp.c.tmp"
    break;

  case 25: /* arglist: arglist ',' exp  */
#line 650 "ada-exp.y"
                        { (yyval.lval) = (yyvsp[-2].lval) + 1; }
#line 2163 "ada-exp.c.tmp"
    break;

  case 26: /* arglist: arglist ',' NAME ARROW exp  */
#line 652 "ada-exp.y"
                        { (yyval.lval) = (yyvsp[-4].lval) + 1; }
#line 2169 "ada-exp.c.tmp"
    break;

  case 27: /* primary: '{' var_or_type '}' primary  */
#line 657 "ada-exp.y"
                        { 
			  if ((yyvsp[-2].tval) == NULL)
			    error (_("Type required within braces in coercion"));
			  operation_up arg = ada_pop ();
			  pstate->push_new<unop_memval_operation>
			    (std::move (arg), (yyvsp[-2].tval));
			}
#line 2181 "ada-exp.c.tmp"
    break;

  case 28: /* simple_exp: simple_exp STARSTAR simple_exp  */
#line 669 "ada-exp.y"
                        { ada_wrap2<ada_binop_exp_operation> (BINOP_EXP); }
#line 2187 "ada-exp.c.tmp"
    break;

  case 29: /* simple_exp: simple_exp '*' simple_exp  */
#line 673 "ada-exp.y"
                        { ada_wrap2<ada_binop_mul_operation> (BINOP_MUL); }
#line 2193 "ada-exp.c.tmp"
    break;

  case 30: /* simple_exp: simple_exp '/' simple_exp  */
#line 677 "ada-exp.y"
                        { ada_wrap2<ada_binop_div_operation> (BINOP_DIV); }
#line 2199 "ada-exp.c.tmp"
    break;

  case 31: /* simple_exp: simple_exp REM simple_exp  */
#line 681 "ada-exp.y"
                        { ada_wrap2<ada_binop_rem_operation> (BINOP_REM); }
#line 2205 "ada-exp.c.tmp"
    break;

  case 32: /* simple_exp: simple_exp MOD simple_exp  */
#line 685 "ada-exp.y"
                        { ada_wrap2<ada_binop_mod_operation> (BINOP_MOD); }
#line 2211 "ada-exp.c.tmp"
    break;

  case 33: /* simple_exp: simple_exp '@' simple_exp  */
#line 689 "ada-exp.y"
                        { ada_wrap2<repeat_operation> (BINOP_REPEAT); }
#line 2217 "ada-exp.c.tmp"
    break;

  case 34: /* simple_exp: simple_exp '+' simple_exp  */
#line 693 "ada-exp.y"
                        { ada_wrap_op<ada_binop_addsub_operation> (BINOP_ADD); }
#line 2223 "ada-exp.c.tmp"
    break;

  case 35: /* simple_exp: simple_exp '&' simple_exp  */
#line 697 "ada-exp.y"
                        { ada_wrap2<ada_concat_operation> (BINOP_CONCAT); }
#line 2229 "ada-exp.c.tmp"
    break;

  case 36: /* simple_exp: simple_exp '-' simple_exp  */
#line 701 "ada-exp.y"
                        { ada_wrap_op<ada_binop_addsub_operation> (BINOP_SUB); }
#line 2235 "ada-exp.c.tmp"
    break;

  case 38: /* relation: simple_exp '=' simple_exp  */
#line 708 "ada-exp.y"
                        { ada_wrap_op<ada_binop_equal_operation> (BINOP_EQUAL); }
#line 2241 "ada-exp.c.tmp"
    break;

  case 39: /* relation: simple_exp NOTEQUAL simple_exp  */
#line 712 "ada-exp.y"
                        { ada_wrap_op<ada_binop_equal_operation> (BINOP_NOTEQUAL); }
#line 2247 "ada-exp.c.tmp"
    break;

  case 40: /* relation: simple_exp LEQ simple_exp  */
#line 716 "ada-exp.y"
                        { ada_un_wrap2<leq_operation> (BINOP_LEQ); }
#line 2253 "ada-exp.c.tmp"
    break;

  case 41: /* relation: simple_exp IN simple_exp DOTDOT simple_exp  */
#line 720 "ada-exp.y"
                        { ada_wrap3<ada_ternop_range_operation> (); }
#line 2259 "ada-exp.c.tmp"
    break;

  case 42: /* relation: simple_exp IN primary TICK_RANGE tick_arglist  */
#line 722 "ada-exp.y"
                        {
			  operation_up rhs = ada_pop ();
			  operation_up lhs = ada_pop ();
			  pstate->push_new<ada_binop_in_bounds_operation>
			    (std::move (lhs), std::move (rhs), (yyvsp[0].lval));
			}
#line 2270 "ada-exp.c.tmp"
    break;

  case 43: /* relation: simple_exp IN var_or_type  */
#line 729 "ada-exp.y"
                        { 
			  if ((yyvsp[0].tval) == NULL)
			    error (_("Right operand of 'in' must be type"));
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_unop_range_operation>
			    (std::move (arg), (yyvsp[0].tval));
			}
#line 2282 "ada-exp.c.tmp"
    break;

  case 44: /* relation: simple_exp NOT IN simple_exp DOTDOT simple_exp  */
#line 737 "ada-exp.y"
                        { ada_wrap3<ada_ternop_range_operation> ();
			  ada_wrap<unary_logical_not_operation> (); }
#line 2289 "ada-exp.c.tmp"
    break;

  case 45: /* relation: simple_exp NOT IN primary TICK_RANGE tick_arglist  */
#line 740 "ada-exp.y"
                        {
			  operation_up rhs = ada_pop ();
			  operation_up lhs = ada_pop ();
			  pstate->push_new<ada_binop_in_bounds_operation>
			    (std::move (lhs), std::move (rhs), (yyvsp[0].lval));
			  ada_wrap<unary_logical_not_operation> ();
			}
#line 2301 "ada-exp.c.tmp"
    break;

  case 46: /* relation: simple_exp NOT IN var_or_type  */
#line 748 "ada-exp.y"
                        { 
			  if ((yyvsp[0].tval) == NULL)
			    error (_("Right operand of 'in' must be type"));
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_unop_range_operation>
			    (std::move (arg), (yyvsp[0].tval));
			  ada_wrap<unary_logical_not_operation> ();
			}
#line 2314 "ada-exp.c.tmp"
    break;

  case 47: /* relation: simple_exp GEQ simple_exp  */
#line 759 "ada-exp.y"
                        { ada_un_wrap2<geq_operation> (BINOP_GEQ); }
#line 2320 "ada-exp.c.tmp"
    break;

  case 48: /* relation: simple_exp '<' simple_exp  */
#line 763 "ada-exp.y"
                        { ada_un_wrap2<less_operation> (BINOP_LESS); }
#line 2326 "ada-exp.c.tmp"
    break;

  case 49: /* relation: simple_exp '>' simple_exp  */
#line 767 "ada-exp.y"
                        { ada_un_wrap2<gtr_operation> (BINOP_GTR); }
#line 2332 "ada-exp.c.tmp"
    break;

  case 56: /* and_exp: relation _AND_ relation  */
#line 780 "ada-exp.y"
                        { ada_wrap2<ada_bitwise_and_operation>
			    (BINOP_BITWISE_AND); }
#line 2339 "ada-exp.c.tmp"
    break;

  case 57: /* and_exp: and_exp _AND_ relation  */
#line 783 "ada-exp.y"
                        { ada_wrap2<ada_bitwise_and_operation>
			    (BINOP_BITWISE_AND); }
#line 2346 "ada-exp.c.tmp"
    break;

  case 58: /* and_then_exp: relation _AND_ THEN relation  */
#line 789 "ada-exp.y"
                        { ada_wrap2<logical_and_operation>
			    (BINOP_LOGICAL_AND); }
#line 2353 "ada-exp.c.tmp"
    break;

  case 59: /* and_then_exp: and_then_exp _AND_ THEN relation  */
#line 792 "ada-exp.y"
                        { ada_wrap2<logical_and_operation>
			    (BINOP_LOGICAL_AND); }
#line 2360 "ada-exp.c.tmp"
    break;

  case 60: /* or_exp: relation OR relation  */
#line 798 "ada-exp.y"
                        { ada_wrap2<ada_bitwise_ior_operation>
			    (BINOP_BITWISE_IOR); }
#line 2367 "ada-exp.c.tmp"
    break;

  case 61: /* or_exp: or_exp OR relation  */
#line 801 "ada-exp.y"
                        { ada_wrap2<ada_bitwise_ior_operation>
			    (BINOP_BITWISE_IOR); }
#line 2374 "ada-exp.c.tmp"
    break;

  case 62: /* or_else_exp: relation OR ELSE relation  */
#line 807 "ada-exp.y"
                        { ada_wrap2<logical_or_operation> (BINOP_LOGICAL_OR); }
#line 2380 "ada-exp.c.tmp"
    break;

  case 63: /* or_else_exp: or_else_exp OR ELSE relation  */
#line 809 "ada-exp.y"
                        { ada_wrap2<logical_or_operation> (BINOP_LOGICAL_OR); }
#line 2386 "ada-exp.c.tmp"
    break;

  case 64: /* xor_exp: relation XOR relation  */
#line 813 "ada-exp.y"
                        { ada_wrap2<ada_bitwise_xor_operation>
			    (BINOP_BITWISE_XOR); }
#line 2393 "ada-exp.c.tmp"
    break;

  case 65: /* xor_exp: xor_exp XOR relation  */
#line 816 "ada-exp.y"
                        { ada_wrap2<ada_bitwise_xor_operation>
			    (BINOP_BITWISE_XOR); }
#line 2400 "ada-exp.c.tmp"
    break;

  case 66: /* primary: primary TICK_ACCESS  */
#line 829 "ada-exp.y"
                        { ada_addrof (); }
#line 2406 "ada-exp.c.tmp"
    break;

  case 67: /* primary: primary TICK_ADDRESS  */
#line 831 "ada-exp.y"
                        { ada_addrof (type_system_address (pstate)); }
#line 2412 "ada-exp.c.tmp"
    break;

  case 68: /* primary: primary TICK_COMPLETE  */
#line 833 "ada-exp.y"
                        {
			  pstate->mark_completion (make_tick_completer ((yyvsp[0].sval)));
			}
#line 2420 "ada-exp.c.tmp"
    break;

  case 69: /* primary: primary TICK_FIRST tick_arglist  */
#line 837 "ada-exp.y"
                        {
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_unop_atr_operation>
			    (std::move (arg), OP_ATR_FIRST, (yyvsp[0].lval));
			}
#line 2430 "ada-exp.c.tmp"
    break;

  case 70: /* primary: primary TICK_LAST tick_arglist  */
#line 843 "ada-exp.y"
                        {
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_unop_atr_operation>
			    (std::move (arg), OP_ATR_LAST, (yyvsp[0].lval));
			}
#line 2440 "ada-exp.c.tmp"
    break;

  case 71: /* primary: primary TICK_LENGTH tick_arglist  */
#line 849 "ada-exp.y"
                        {
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_unop_atr_operation>
			    (std::move (arg), OP_ATR_LENGTH, (yyvsp[0].lval));
			}
#line 2450 "ada-exp.c.tmp"
    break;

  case 72: /* primary: primary TICK_SIZE  */
#line 855 "ada-exp.y"
                        { ada_wrap<ada_atr_size_operation> (); }
#line 2456 "ada-exp.c.tmp"
    break;

  case 73: /* primary: primary TICK_TAG  */
#line 857 "ada-exp.y"
                        { ada_wrap<ada_atr_tag_operation> (); }
#line 2462 "ada-exp.c.tmp"
    break;

  case 74: /* primary: opt_type_prefix TICK_MIN '(' exp ',' exp ')'  */
#line 859 "ada-exp.y"
                        { ada_wrap2<ada_binop_min_operation> (BINOP_MIN); }
#line 2468 "ada-exp.c.tmp"
    break;

  case 75: /* primary: opt_type_prefix TICK_MAX '(' exp ',' exp ')'  */
#line 861 "ada-exp.y"
                        { ada_wrap2<ada_binop_max_operation> (BINOP_MAX); }
#line 2474 "ada-exp.c.tmp"
    break;

  case 76: /* primary: opt_type_prefix TICK_POS '(' exp ')'  */
#line 863 "ada-exp.y"
                        { ada_wrap<ada_pos_operation> (); }
#line 2480 "ada-exp.c.tmp"
    break;

  case 77: /* primary: type_prefix TICK_VAL '(' exp ')'  */
#line 865 "ada-exp.y"
                        {
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_atr_val_operation>
			    ((yyvsp[-4].tval), std::move (arg));
			}
#line 2490 "ada-exp.c.tmp"
    break;

  case 78: /* primary: type_prefix TICK_MODULUS  */
#line 871 "ada-exp.y"
                        {
			  struct type *type_arg = check_typedef ((yyvsp[-1].tval));
			  if (!ada_is_modular_type (type_arg))
			    error (_("'modulus must be applied to modular type"));
			  write_int (pstate, ada_modulus (type_arg),
				     type_arg->target_type ());
			}
#line 2502 "ada-exp.c.tmp"
    break;

  case 79: /* tick_arglist: %empty  */
#line 881 "ada-exp.y"
                        { (yyval.lval) = 1; }
#line 2508 "ada-exp.c.tmp"
    break;

  case 80: /* tick_arglist: '(' INT ')'  */
#line 883 "ada-exp.y"
                        { (yyval.lval) = (yyvsp[-1].typed_val).val; }
#line 2514 "ada-exp.c.tmp"
    break;

  case 81: /* type_prefix: var_or_type  */
#line 888 "ada-exp.y"
                        { 
			  if ((yyvsp[0].tval) == NULL)
			    error (_("Prefix must be type"));
			  (yyval.tval) = (yyvsp[0].tval);
			}
#line 2524 "ada-exp.c.tmp"
    break;

  case 82: /* opt_type_prefix: type_prefix  */
#line 897 "ada-exp.y"
                        { (yyval.tval) = (yyvsp[0].tval); }
#line 2530 "ada-exp.c.tmp"
    break;

  case 83: /* opt_type_prefix: %empty  */
#line 899 "ada-exp.y"
                        { (yyval.tval) = parse_type (pstate)->builtin_void; }
#line 2536 "ada-exp.c.tmp"
    break;

  case 84: /* primary: INT  */
#line 904 "ada-exp.y"
                        { write_int (pstate, (LONGEST) (yyvsp[0].typed_val).val, (yyvsp[0].typed_val).type); }
#line 2542 "ada-exp.c.tmp"
    break;

  case 85: /* primary: CHARLIT  */
#line 908 "ada-exp.y"
                        {
			  pstate->push_new<ada_char_operation> ((yyvsp[0].typed_val).type, (yyvsp[0].typed_val).val);
			}
#line 2550 "ada-exp.c.tmp"
    break;

  case 86: /* primary: FLOAT  */
#line 914 "ada-exp.y"
                        {
			  float_data data;
			  std::copy (std::begin ((yyvsp[0].typed_val_float).val), std::end ((yyvsp[0].typed_val_float).val),
				     std::begin (data));
			  pstate->push_new<float_const_operation>
			    ((yyvsp[0].typed_val_float).type, data);
			  ada_wrap<ada_wrapped_operation> ();
			}
#line 2563 "ada-exp.c.tmp"
    break;

  case 87: /* primary: NULL_PTR  */
#line 925 "ada-exp.y"
                        {
			  struct type *null_ptr_type
			    = lookup_pointer_type (parse_type (pstate)->builtin_int0);
			  write_int (pstate, 0, null_ptr_type);
			}
#line 2573 "ada-exp.c.tmp"
    break;

  case 88: /* primary: STRING  */
#line 933 "ada-exp.y"
                        { 
			  pstate->push_new<ada_string_operation>
			    (copy_name ((yyvsp[0].sval)));
			}
#line 2582 "ada-exp.c.tmp"
    break;

  case 89: /* primary: TRUEKEYWORD  */
#line 940 "ada-exp.y"
                        { write_int (pstate, 1, type_boolean (pstate)); }
#line 2588 "ada-exp.c.tmp"
    break;

  case 90: /* primary: FALSEKEYWORD  */
#line 942 "ada-exp.y"
                        { write_int (pstate, 0, type_boolean (pstate)); }
#line 2594 "ada-exp.c.tmp"
    break;

  case 91: /* primary: NEW NAME  */
#line 946 "ada-exp.y"
                        { error (_("NEW not implemented.")); }
#line 2600 "ada-exp.c.tmp"
    break;

  case 92: /* var_or_type: NAME  */
#line 950 "ada-exp.y"
                                { (yyval.tval) = write_var_or_type (pstate, NULL, (yyvsp[0].sval)); }
#line 2606 "ada-exp.c.tmp"
    break;

  case 93: /* var_or_type: NAME_COMPLETE  */
#line 952 "ada-exp.y"
                                {
				  (yyval.tval) = write_var_or_type_completion (pstate,
								     NULL,
								     (yyvsp[0].sval));
				}
#line 2616 "ada-exp.c.tmp"
    break;

  case 94: /* var_or_type: block NAME  */
#line 958 "ada-exp.y"
                                { (yyval.tval) = write_var_or_type (pstate, (yyvsp[-1].bval), (yyvsp[0].sval)); }
#line 2622 "ada-exp.c.tmp"
    break;

  case 95: /* var_or_type: block NAME_COMPLETE  */
#line 960 "ada-exp.y"
                                {
				  (yyval.tval) = write_var_or_type_completion (pstate,
								     (yyvsp[-1].bval),
								     (yyvsp[0].sval));
				}
#line 2632 "ada-exp.c.tmp"
    break;

  case 96: /* var_or_type: NAME TICK_ACCESS  */
#line 966 "ada-exp.y"
                        { 
			  (yyval.tval) = write_var_or_type (pstate, NULL, (yyvsp[-1].sval));
			  if ((yyval.tval) == NULL)
			    ada_addrof ();
			  else
			    (yyval.tval) = lookup_pointer_type ((yyval.tval));
			}
#line 2644 "ada-exp.c.tmp"
    break;

  case 97: /* var_or_type: block NAME TICK_ACCESS  */
#line 974 "ada-exp.y"
                        { 
			  (yyval.tval) = write_var_or_type (pstate, (yyvsp[-2].bval), (yyvsp[-1].sval));
			  if ((yyval.tval) == NULL)
			    ada_addrof ();
			  else
			    (yyval.tval) = lookup_pointer_type ((yyval.tval));
			}
#line 2656 "ada-exp.c.tmp"
    break;

  case 98: /* block: NAME COLONCOLON  */
#line 985 "ada-exp.y"
                        { (yyval.bval) = block_lookup (NULL, (yyvsp[-1].sval).ptr); }
#line 2662 "ada-exp.c.tmp"
    break;

  case 99: /* block: block NAME COLONCOLON  */
#line 987 "ada-exp.y"
                        { (yyval.bval) = block_lookup ((yyvsp[-2].bval), (yyvsp[-1].sval).ptr); }
#line 2668 "ada-exp.c.tmp"
    break;

  case 100: /* aggregate: '(' aggregate_component_list ')'  */
#line 992 "ada-exp.y"
                        {
			  std::vector<ada_component_up> components
			    = pop_components ((yyvsp[-1].lval));

			  push_component<ada_aggregate_component>
			    (std::move (components));
			}
#line 2680 "ada-exp.c.tmp"
    break;

  case 101: /* aggregate_component_list: component_groups  */
#line 1002 "ada-exp.y"
                                         { (yyval.lval) = (yyvsp[0].lval); }
#line 2686 "ada-exp.c.tmp"
    break;

  case 102: /* aggregate_component_list: positional_list exp  */
#line 1004 "ada-exp.y"
                        {
			  push_component<ada_positional_component>
			    ((yyvsp[-1].lval), ada_pop ());
			  (yyval.lval) = (yyvsp[-1].lval) + 1;
			}
#line 2696 "ada-exp.c.tmp"
    break;

  case 103: /* aggregate_component_list: positional_list component_groups  */
#line 1010 "ada-exp.y"
                                         { (yyval.lval) = (yyvsp[-1].lval) + (yyvsp[0].lval); }
#line 2702 "ada-exp.c.tmp"
    break;

  case 104: /* positional_list: exp ','  */
#line 1015 "ada-exp.y"
                        {
			  push_component<ada_positional_component>
			    (0, ada_pop ());
			  (yyval.lval) = 1;
			}
#line 2712 "ada-exp.c.tmp"
    break;

  case 105: /* positional_list: positional_list exp ','  */
#line 1021 "ada-exp.y"
                        {
			  push_component<ada_positional_component>
			    ((yyvsp[-2].lval), ada_pop ());
			  (yyval.lval) = (yyvsp[-2].lval) + 1; 
			}
#line 2722 "ada-exp.c.tmp"
    break;

  case 106: /* component_groups: others  */
#line 1029 "ada-exp.y"
                                         { (yyval.lval) = 1; }
#line 2728 "ada-exp.c.tmp"
    break;

  case 107: /* component_groups: component_group  */
#line 1030 "ada-exp.y"
                                         { (yyval.lval) = 1; }
#line 2734 "ada-exp.c.tmp"
    break;

  case 108: /* component_groups: component_group ',' component_groups  */
#line 1032 "ada-exp.y"
                                         { (yyval.lval) = (yyvsp[0].lval) + 1; }
#line 2740 "ada-exp.c.tmp"
    break;

  case 109: /* others: OTHERS ARROW exp  */
#line 1036 "ada-exp.y"
                        {
			  push_component<ada_others_component> (ada_pop ());
			}
#line 2748 "ada-exp.c.tmp"
    break;

  case 110: /* component_group: component_associations  */
#line 1043 "ada-exp.y"
                        {
			  ada_choices_component *choices = choice_component ();
			  choices->set_associations (pop_associations ((yyvsp[0].lval)));
			}
#line 2757 "ada-exp.c.tmp"
    break;

  case 111: /* component_associations: NAME ARROW exp  */
#line 1056 "ada-exp.y"
                        {
			  push_component<ada_choices_component> (ada_pop ());
			  write_name_assoc (pstate, (yyvsp[-2].sval));
			  (yyval.lval) = 1;
			}
#line 2767 "ada-exp.c.tmp"
    break;

  case 112: /* component_associations: simple_exp ARROW exp  */
#line 1062 "ada-exp.y"
                        {
			  push_component<ada_choices_component> (ada_pop ());
			  push_association<ada_name_association> (ada_pop ());
			  (yyval.lval) = 1;
			}
#line 2777 "ada-exp.c.tmp"
    break;

  case 113: /* component_associations: simple_exp DOTDOT simple_exp ARROW exp  */
#line 1068 "ada-exp.y"
                        {
			  push_component<ada_choices_component> (ada_pop ());
			  operation_up rhs = ada_pop ();
			  operation_up lhs = ada_pop ();
			  push_association<ada_discrete_range_association>
			    (std::move (lhs), std::move (rhs));
			  (yyval.lval) = 1;
			}
#line 2790 "ada-exp.c.tmp"
    break;

  case 114: /* component_associations: NAME '|' component_associations  */
#line 1077 "ada-exp.y"
                        {
			  write_name_assoc (pstate, (yyvsp[-2].sval));
			  (yyval.lval) = (yyvsp[0].lval) + 1;
			}
#line 2799 "ada-exp.c.tmp"
    break;

  case 115: /* component_associations: simple_exp '|' component_associations  */
#line 1082 "ada-exp.y"
                        {
			  push_association<ada_name_association> (ada_pop ());
			  (yyval.lval) = (yyvsp[0].lval) + 1;
			}
#line 2808 "ada-exp.c.tmp"
    break;

  case 116: /* component_associations: simple_exp DOTDOT simple_exp '|' component_associations  */
#line 1088 "ada-exp.y"
                        {
			  operation_up rhs = ada_pop ();
			  operation_up lhs = ada_pop ();
			  push_association<ada_discrete_range_association>
			    (std::move (lhs), std::move (rhs));
			  (yyval.lval) = (yyvsp[0].lval) + 1;
			}
#line 2820 "ada-exp.c.tmp"
    break;

  case 117: /* primary: '*' primary  */
#line 1101 "ada-exp.y"
                        { ada_wrap<ada_unop_ind_operation> (); }
#line 2826 "ada-exp.c.tmp"
    break;

  case 118: /* primary: '&' primary  */
#line 1103 "ada-exp.y"
                        { ada_addrof (); }
#line 2832 "ada-exp.c.tmp"
    break;

  case 119: /* primary: primary '[' exp ']'  */
#line 1105 "ada-exp.y"
                        {
			  ada_wrap2<subscript_operation> (BINOP_SUBSCRIPT);
			  ada_wrap<ada_wrapped_operation> ();
			}
#line 2841 "ada-exp.c.tmp"
    break;


#line 2845 "ada-exp.c.tmp"

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

#line 1111 "ada-exp.y"


/* yylex defined in ada-lex.c: Reads one token, getting characters */
/* through lexptr.  */

/* Remap normal flex interface names (yylex) as well as gratuitiously */
/* global symbol names, so we can have multiple flex-generated parsers */
/* in gdb.  */

/* (See note above on previous definitions for YACC.) */

#define yy_create_buffer ada_yy_create_buffer
#define yy_delete_buffer ada_yy_delete_buffer
#define yy_init_buffer ada_yy_init_buffer
#define yy_load_buffer_state ada_yy_load_buffer_state
#define yy_switch_to_buffer ada_yy_switch_to_buffer
#define yyrestart ada_yyrestart
#define yytext ada_yytext

static struct obstack temp_parse_space;

/* The following kludge was found necessary to prevent conflicts between */
/* defs.h and non-standard stdlib.h files.  */
#define qsort __qsort__dummy
#include "ada-lex.c"

int
ada_parse (struct parser_state *par_state)
{
  /* Setting up the parser state.  */
  scoped_restore pstate_restore = make_scoped_restore (&pstate);
  gdb_assert (par_state != NULL);
  pstate = par_state;
  original_expr = par_state->lexptr;

  scoped_restore restore_yydebug = make_scoped_restore (&yydebug,
							parser_debug);

  lexer_init (yyin);		/* (Re-)initialize lexer.  */
  obstack_free (&temp_parse_space, NULL);
  obstack_init (&temp_parse_space);
  components.clear ();
  associations.clear ();

  int result = yyparse ();
  if (!result)
    {
      struct type *context_type = nullptr;
      if (par_state->void_context_p)
	context_type = parse_type (par_state)->builtin_void;
      pstate->set_operation (ada_pop (true, context_type));
    }
  return result;
}

static void
yyerror (const char *msg)
{
  error (_("Error in expression, near `%s'."), pstate->lexptr);
}

/* Emit expression to access an instance of SYM, in block BLOCK (if
   non-NULL).  */

static void
write_var_from_sym (struct parser_state *par_state, block_symbol sym)
{
  if (symbol_read_needs_frame (sym.symbol))
    par_state->block_tracker->update (sym.block, INNERMOST_BLOCK_FOR_SYMBOLS);

  par_state->push_new<ada_var_value_operation> (sym);
}

/* Write integer or boolean constant ARG of type TYPE.  */

static void
write_int (struct parser_state *par_state, LONGEST arg, struct type *type)
{
  pstate->push_new<long_const_operation> (type, arg);
  ada_wrap<ada_wrapped_operation> ();
}

/* Emit expression corresponding to the renamed object named 
   designated by RENAMED_ENTITY[0 .. RENAMED_ENTITY_LEN-1] in the
   context of ORIG_LEFT_CONTEXT, to which is applied the operations
   encoded by RENAMING_EXPR.  MAX_DEPTH is the maximum number of
   cascaded renamings to allow.  If ORIG_LEFT_CONTEXT is null, it
   defaults to the currently selected block. ORIG_SYMBOL is the 
   symbol that originally encoded the renaming.  It is needed only
   because its prefix also qualifies any index variables used to index
   or slice an array.  It should not be necessary once we go to the
   new encoding entirely (FIXME pnh 7/20/2007).  */

static void
write_object_renaming (struct parser_state *par_state,
		       const struct block *orig_left_context,
		       const char *renamed_entity, int renamed_entity_len,
		       const char *renaming_expr, int max_depth)
{
  char *name;
  enum { SIMPLE_INDEX, LOWER_BOUND, UPPER_BOUND } slice_state;
  struct block_symbol sym_info;

  if (max_depth <= 0)
    error (_("Could not find renamed symbol"));

  if (orig_left_context == NULL)
    orig_left_context = get_selected_block (NULL);

  name = obstack_strndup (&temp_parse_space, renamed_entity,
			  renamed_entity_len);
  ada_lookup_encoded_symbol (name, orig_left_context, VAR_DOMAIN, &sym_info);
  if (sym_info.symbol == NULL)
    error (_("Could not find renamed variable: %s"), ada_decode (name).c_str ());
  else if (sym_info.symbol->aclass () == LOC_TYPEDEF)
    /* We have a renaming of an old-style renaming symbol.  Don't
       trust the block information.  */
    sym_info.block = orig_left_context;

  {
    const char *inner_renamed_entity;
    int inner_renamed_entity_len;
    const char *inner_renaming_expr;

    switch (ada_parse_renaming (sym_info.symbol, &inner_renamed_entity,
				&inner_renamed_entity_len,
				&inner_renaming_expr))
      {
      case ADA_NOT_RENAMING:
	write_var_from_sym (par_state, sym_info);
	break;
      case ADA_OBJECT_RENAMING:
	write_object_renaming (par_state, sym_info.block,
			       inner_renamed_entity, inner_renamed_entity_len,
			       inner_renaming_expr, max_depth - 1);
	break;
      default:
	goto BadEncoding;
      }
  }

  slice_state = SIMPLE_INDEX;
  while (*renaming_expr == 'X')
    {
      renaming_expr += 1;

      switch (*renaming_expr) {
      case 'A':
	renaming_expr += 1;
	ada_wrap<ada_unop_ind_operation> ();
	break;
      case 'L':
	slice_state = LOWER_BOUND;
	/* FALLTHROUGH */
      case 'S':
	renaming_expr += 1;
	if (isdigit (*renaming_expr))
	  {
	    char *next;
	    long val = strtol (renaming_expr, &next, 10);
	    if (next == renaming_expr)
	      goto BadEncoding;
	    renaming_expr = next;
	    write_int (par_state, val, type_int (par_state));
	  }
	else
	  {
	    const char *end;
	    char *index_name;
	    struct block_symbol index_sym_info;

	    end = strchr (renaming_expr, 'X');
	    if (end == NULL)
	      end = renaming_expr + strlen (renaming_expr);

	    index_name = obstack_strndup (&temp_parse_space, renaming_expr,
					  end - renaming_expr);
	    renaming_expr = end;

	    ada_lookup_encoded_symbol (index_name, orig_left_context,
				       VAR_DOMAIN, &index_sym_info);
	    if (index_sym_info.symbol == NULL)
	      error (_("Could not find %s"), index_name);
	    else if (index_sym_info.symbol->aclass () == LOC_TYPEDEF)
	      /* Index is an old-style renaming symbol.  */
	      index_sym_info.block = orig_left_context;
	    write_var_from_sym (par_state, index_sym_info);
	  }
	if (slice_state == SIMPLE_INDEX)
	  ada_funcall (1);
	else if (slice_state == LOWER_BOUND)
	  slice_state = UPPER_BOUND;
	else if (slice_state == UPPER_BOUND)
	  {
	    ada_wrap3<ada_ternop_slice_operation> ();
	    slice_state = SIMPLE_INDEX;
	  }
	break;

      case 'R':
	{
	  const char *end;

	  renaming_expr += 1;

	  if (slice_state != SIMPLE_INDEX)
	    goto BadEncoding;
	  end = strchr (renaming_expr, 'X');
	  if (end == NULL)
	    end = renaming_expr + strlen (renaming_expr);

	  operation_up arg = ada_pop ();
	  pstate->push_new<ada_structop_operation>
	    (std::move (arg), std::string (renaming_expr,
					   end - renaming_expr));
	  renaming_expr = end;
	  break;
	}

      default:
	goto BadEncoding;
      }
    }
  if (slice_state == SIMPLE_INDEX)
    return;

 BadEncoding:
  error (_("Internal error in encoding of renaming declaration"));
}

static const struct block*
block_lookup (const struct block *context, const char *raw_name)
{
  const char *name;
  struct symtab *symtab;
  const struct block *result = NULL;

  std::string name_storage;
  if (raw_name[0] == '\'')
    {
      raw_name += 1;
      name = raw_name;
    }
  else
    {
      name_storage = ada_encode (raw_name);
      name = name_storage.c_str ();
    }

  std::vector<struct block_symbol> syms
    = ada_lookup_symbol_list (name, context, VAR_DOMAIN);

  if (context == NULL
      && (syms.empty () || syms[0].symbol->aclass () != LOC_BLOCK))
    symtab = lookup_symtab (name);
  else
    symtab = NULL;

  if (symtab != NULL)
    result = symtab->compunit ()->blockvector ()->static_block ();
  else if (syms.empty () || syms[0].symbol->aclass () != LOC_BLOCK)
    {
      if (context == NULL)
	error (_("No file or function \"%s\"."), raw_name);
      else
	error (_("No function \"%s\" in specified context."), raw_name);
    }
  else
    {
      if (syms.size () > 1)
	warning (_("Function name \"%s\" ambiguous here"), raw_name);
      result = syms[0].symbol->value_block ();
    }

  return result;
}

static struct symbol*
select_possible_type_sym (const std::vector<struct block_symbol> &syms)
{
  int i;
  int preferred_index;
  struct type *preferred_type;
	  
  preferred_index = -1; preferred_type = NULL;
  for (i = 0; i < syms.size (); i += 1)
    switch (syms[i].symbol->aclass ())
      {
      case LOC_TYPEDEF:
	if (ada_prefer_type (syms[i].symbol->type (), preferred_type))
	  {
	    preferred_index = i;
	    preferred_type = syms[i].symbol->type ();
	  }
	break;
      case LOC_REGISTER:
      case LOC_ARG:
      case LOC_REF_ARG:
      case LOC_REGPARM_ADDR:
      case LOC_LOCAL:
      case LOC_COMPUTED:
	return NULL;
      default:
	break;
      }
  if (preferred_type == NULL)
    return NULL;
  return syms[preferred_index].symbol;
}

static struct type*
find_primitive_type (struct parser_state *par_state, const char *name)
{
  struct type *type;
  type = language_lookup_primitive_type (par_state->language (),
					 par_state->gdbarch (),
					 name);
  if (type == NULL && strcmp ("system__address", name) == 0)
    type = type_system_address (par_state);

  if (type != NULL)
    {
      /* Check to see if we have a regular definition of this
	 type that just didn't happen to have been read yet.  */
      struct symbol *sym;
      char *expanded_name = 
	(char *) alloca (strlen (name) + sizeof ("standard__"));
      strcpy (expanded_name, "standard__");
      strcat (expanded_name, name);
      sym = ada_lookup_symbol (expanded_name, NULL, VAR_DOMAIN).symbol;
      if (sym != NULL && sym->aclass () == LOC_TYPEDEF)
	type = sym->type ();
    }

  return type;
}

static int
chop_selector (const char *name, int end)
{
  int i;
  for (i = end - 1; i > 0; i -= 1)
    if (name[i] == '.' || (name[i] == '_' && name[i+1] == '_'))
      return i;
  return -1;
}

/* If NAME is a string beginning with a separator (either '__', or
   '.'), chop this separator and return the result; else, return
   NAME.  */

static const char *
chop_separator (const char *name)
{
  if (*name == '.')
   return name + 1;

  if (name[0] == '_' && name[1] == '_')
    return name + 2;

  return name;
}

/* Given that SELS is a string of the form (<sep><identifier>)*, where
   <sep> is '__' or '.', write the indicated sequence of
   STRUCTOP_STRUCT expression operators.  Returns a pointer to the
   last operation that was pushed.  */
static ada_structop_operation *
write_selectors (struct parser_state *par_state, const char *sels)
{
  ada_structop_operation *result = nullptr;
  while (*sels != '\0')
    {
      const char *p = chop_separator (sels);
      sels = p;
      while (*sels != '\0' && *sels != '.' 
	     && (sels[0] != '_' || sels[1] != '_'))
	sels += 1;
      operation_up arg = ada_pop ();
      result = new ada_structop_operation (std::move (arg),
					   std::string (p, sels - p));
      pstate->push (operation_up (result));
    }
  return result;
}

/* Write a variable access (OP_VAR_VALUE) to ambiguous encoded name
   NAME[0..LEN-1], in block context BLOCK, to be resolved later.  Writes
   a temporary symbol that is valid until the next call to ada_parse.
   */
static void
write_ambiguous_var (struct parser_state *par_state,
		     const struct block *block, const char *name, int len)
{
  struct symbol *sym = new (&temp_parse_space) symbol ();

  sym->set_domain (UNDEF_DOMAIN);
  sym->set_linkage_name (obstack_strndup (&temp_parse_space, name, len));
  sym->set_language (language_ada, nullptr);

  block_symbol bsym { sym, block };
  par_state->push_new<ada_var_value_operation> (bsym);
}

/* A convenient wrapper around ada_get_field_index that takes
   a non NUL-terminated FIELD_NAME0 and a FIELD_NAME_LEN instead
   of a NUL-terminated field name.  */

static int
ada_nget_field_index (const struct type *type, const char *field_name0,
		      int field_name_len, int maybe_missing)
{
  char *field_name = (char *) alloca ((field_name_len + 1) * sizeof (char));

  strncpy (field_name, field_name0, field_name_len);
  field_name[field_name_len] = '\0';
  return ada_get_field_index (type, field_name, maybe_missing);
}

/* If encoded_field_name is the name of a field inside symbol SYM,
   then return the type of that field.  Otherwise, return NULL.

   This function is actually recursive, so if ENCODED_FIELD_NAME
   doesn't match one of the fields of our symbol, then try to see
   if ENCODED_FIELD_NAME could not be a succession of field names
   (in other words, the user entered an expression of the form
   TYPE_NAME.FIELD1.FIELD2.FIELD3), in which case we evaluate
   each field name sequentially to obtain the desired field type.
   In case of failure, we return NULL.  */

static struct type *
get_symbol_field_type (struct symbol *sym, const char *encoded_field_name)
{
  const char *field_name = encoded_field_name;
  const char *subfield_name;
  struct type *type = sym->type ();
  int fieldno;

  if (type == NULL || field_name == NULL)
    return NULL;
  type = check_typedef (type);

  while (field_name[0] != '\0')
    {
      field_name = chop_separator (field_name);

      fieldno = ada_get_field_index (type, field_name, 1);
      if (fieldno >= 0)
	return type->field (fieldno).type ();

      subfield_name = field_name;
      while (*subfield_name != '\0' && *subfield_name != '.' 
	     && (subfield_name[0] != '_' || subfield_name[1] != '_'))
	subfield_name += 1;

      if (subfield_name[0] == '\0')
	return NULL;

      fieldno = ada_nget_field_index (type, field_name,
				      subfield_name - field_name, 1);
      if (fieldno < 0)
	return NULL;

      type = type->field (fieldno).type ();
      field_name = subfield_name;
    }

  return NULL;
}

/* Look up NAME0 (an unencoded identifier or dotted name) in BLOCK (or 
   expression_block_context if NULL).  If it denotes a type, return
   that type.  Otherwise, write expression code to evaluate it as an
   object and return NULL. In this second case, NAME0 will, in general,
   have the form <name>(.<selector_name>)*, where <name> is an object
   or renaming encoded in the debugging data.  Calls error if no
   prefix <name> matches a name in the debugging data (i.e., matches
   either a complete name or, as a wild-card match, the final 
   identifier).  */

static struct type*
write_var_or_type (struct parser_state *par_state,
		   const struct block *block, struct stoken name0)
{
  int depth;
  char *encoded_name;
  int name_len;

  if (block == NULL)
    block = par_state->expression_context_block;

  std::string name_storage = ada_encode (name0.ptr);
  name_len = name_storage.size ();
  encoded_name = obstack_strndup (&temp_parse_space, name_storage.c_str (),
				  name_len);
  for (depth = 0; depth < MAX_RENAMING_CHAIN_LENGTH; depth += 1)
    {
      int tail_index;
      
      tail_index = name_len;
      while (tail_index > 0)
	{
	  struct symbol *type_sym;
	  struct symbol *renaming_sym;
	  const char* renaming;
	  int renaming_len;
	  const char* renaming_expr;
	  int terminator = encoded_name[tail_index];

	  encoded_name[tail_index] = '\0';
	  /* In order to avoid double-encoding, we want to only pass
	     the decoded form to lookup functions.  */
	  std::string decoded_name = ada_decode (encoded_name);
	  encoded_name[tail_index] = terminator;

	  std::vector<struct block_symbol> syms
	    = ada_lookup_symbol_list (decoded_name.c_str (), block, VAR_DOMAIN);

	  type_sym = select_possible_type_sym (syms);

	  if (type_sym != NULL)
	    renaming_sym = type_sym;
	  else if (syms.size () == 1)
	    renaming_sym = syms[0].symbol;
	  else 
	    renaming_sym = NULL;

	  switch (ada_parse_renaming (renaming_sym, &renaming,
				      &renaming_len, &renaming_expr))
	    {
	    case ADA_NOT_RENAMING:
	      break;
	    case ADA_PACKAGE_RENAMING:
	    case ADA_EXCEPTION_RENAMING:
	    case ADA_SUBPROGRAM_RENAMING:
	      {
		int alloc_len = renaming_len + name_len - tail_index + 1;
		char *new_name
		  = (char *) obstack_alloc (&temp_parse_space, alloc_len);
		strncpy (new_name, renaming, renaming_len);
		strcpy (new_name + renaming_len, encoded_name + tail_index);
		encoded_name = new_name;
		name_len = renaming_len + name_len - tail_index;
		goto TryAfterRenaming;
	      }	
	    case ADA_OBJECT_RENAMING:
	      write_object_renaming (par_state, block, renaming, renaming_len,
				     renaming_expr, MAX_RENAMING_CHAIN_LENGTH);
	      write_selectors (par_state, encoded_name + tail_index);
	      return NULL;
	    default:
	      internal_error (_("impossible value from ada_parse_renaming"));
	    }

	  if (type_sym != NULL)
	    {
	      struct type *field_type;
	      
	      if (tail_index == name_len)
		return type_sym->type ();

	      /* We have some extraneous characters after the type name.
		 If this is an expression "TYPE_NAME.FIELD0.[...].FIELDN",
		 then try to get the type of FIELDN.  */
	      field_type
		= get_symbol_field_type (type_sym, encoded_name + tail_index);
	      if (field_type != NULL)
		return field_type;
	      else 
		error (_("Invalid attempt to select from type: \"%s\"."),
		       name0.ptr);
	    }
	  else if (tail_index == name_len && syms.empty ())
	    {
	      struct type *type = find_primitive_type (par_state,
						       encoded_name);

	      if (type != NULL)
		return type;
	    }

	  if (syms.size () == 1)
	    {
	      write_var_from_sym (par_state, syms[0]);
	      write_selectors (par_state, encoded_name + tail_index);
	      return NULL;
	    }
	  else if (syms.empty ())
	    {
	      struct objfile *objfile = nullptr;
	      if (block != nullptr)
		objfile = block_objfile (block);

	      struct bound_minimal_symbol msym
		= ada_lookup_simple_minsym (decoded_name.c_str (), objfile);
	      if (msym.minsym != NULL)
		{
		  par_state->push_new<ada_var_msym_value_operation> (msym);
		  /* Maybe cause error here rather than later? FIXME? */
		  write_selectors (par_state, encoded_name + tail_index);
		  return NULL;
		}

	      if (tail_index == name_len
		  && strncmp (encoded_name, "standard__", 
			      sizeof ("standard__") - 1) == 0)
		error (_("No definition of \"%s\" found."), name0.ptr);

	      tail_index = chop_selector (encoded_name, tail_index);
	    } 
	  else
	    {
	      write_ambiguous_var (par_state, block, encoded_name,
				   tail_index);
	      write_selectors (par_state, encoded_name + tail_index);
	      return NULL;
	    }
	}

      if (!have_full_symbols () && !have_partial_symbols () && block == NULL)
	error (_("No symbol table is loaded.  Use the \"file\" command."));
      if (block == par_state->expression_context_block)
	error (_("No definition of \"%s\" in current context."), name0.ptr);
      else
	error (_("No definition of \"%s\" in specified context."), name0.ptr);
      
    TryAfterRenaming: ;
    }

  error (_("Could not find renamed symbol \"%s\""), name0.ptr);

}

/* Because ada_completer_word_break_characters does not contain '.' --
   and it cannot easily be added, this breaks other completions -- we
   have to recreate the completion word-splitting here, so that we can
   provide a prefix that is then used when completing field names.
   Without this, an attempt like "complete print abc.d" will give a
   result like "print def" rather than "print abc.def".  */

static std::string
find_completion_bounds (struct parser_state *par_state)
{
  const char *end = pstate->lexptr;
  /* First the end of the prefix.  Here we stop at the token start or
     at '.' or space.  */
  for (; end > original_expr && end[-1] != '.' && !isspace (end[-1]); --end)
    {
      /* Nothing.  */
    }
  /* Now find the start of the prefix.  */
  const char *ptr = end;
  /* Here we allow '.'.  */
  for (;
       ptr > original_expr && (ptr[-1] == '.'
			       || ptr[-1] == '_'
			       || (ptr[-1] >= 'a' && ptr[-1] <= 'z')
			       || (ptr[-1] >= 'A' && ptr[-1] <= 'Z')
			       || (ptr[-1] & 0xff) >= 0x80);
       --ptr)
    {
      /* Nothing.  */
    }
  /* ... except, skip leading spaces.  */
  ptr = skip_spaces (ptr);

  return std::string (ptr, end);
}

/* A wrapper for write_var_or_type that is used specifically when
   completion is requested for the last of a sequence of
   identifiers.  */

static struct type *
write_var_or_type_completion (struct parser_state *par_state,
			      const struct block *block, struct stoken name0)
{
  int tail_index = chop_selector (name0.ptr, name0.length);
  /* If there's no separator, just defer to ordinary symbol
     completion.  */
  if (tail_index == -1)
    return write_var_or_type (par_state, block, name0);

  std::string copy (name0.ptr, tail_index);
  struct type *type = write_var_or_type (par_state, block,
					 { copy.c_str (),
					   (int) copy.length () });
  /* For completion purposes, it's enough that we return a type
     here.  */
  if (type != nullptr)
    return type;

  ada_structop_operation *op = write_selectors (par_state,
						name0.ptr + tail_index);
  op->set_prefix (find_completion_bounds (par_state));
  par_state->mark_struct_expression (op);
  return nullptr;
}

/* Write a left side of a component association (e.g., NAME in NAME =>
   exp).  If NAME has the form of a selected component, write it as an
   ordinary expression.  If it is a simple variable that unambiguously
   corresponds to exactly one symbol that does not denote a type or an
   object renaming, also write it normally as an OP_VAR_VALUE.
   Otherwise, write it as an OP_NAME.

   Unfortunately, we don't know at this point whether NAME is supposed
   to denote a record component name or the value of an array index.
   Therefore, it is not appropriate to disambiguate an ambiguous name
   as we normally would, nor to replace a renaming with its referent.
   As a result, in the (one hopes) rare case that one writes an
   aggregate such as (R => 42) where R renames an object or is an
   ambiguous name, one must write instead ((R) => 42). */
   
static void
write_name_assoc (struct parser_state *par_state, struct stoken name)
{
  if (strchr (name.ptr, '.') == NULL)
    {
      std::vector<struct block_symbol> syms
	= ada_lookup_symbol_list (name.ptr,
				  par_state->expression_context_block,
				  VAR_DOMAIN);

      if (syms.size () != 1 || syms[0].symbol->aclass () == LOC_TYPEDEF)
	pstate->push_new<ada_string_operation> (copy_name (name));
      else
	write_var_from_sym (par_state, syms[0]);
    }
  else
    if (write_var_or_type (par_state, NULL, name) != NULL)
      error (_("Invalid use of type."));

  push_association<ada_name_association> (ada_pop ());
}

static struct type *
type_int (struct parser_state *par_state)
{
  return parse_type (par_state)->builtin_int;
}

static struct type *
type_long (struct parser_state *par_state)
{
  return parse_type (par_state)->builtin_long;
}

static struct type *
type_long_long (struct parser_state *par_state)
{
  return parse_type (par_state)->builtin_long_long;
}

static struct type *
type_long_double (struct parser_state *par_state)
{
  return parse_type (par_state)->builtin_long_double;
}

static struct type *
type_for_char (struct parser_state *par_state, ULONGEST value)
{
  if (value <= 0xff)
    return language_string_char_type (par_state->language (),
				      par_state->gdbarch ());
  else if (value <= 0xffff)
    return language_lookup_primitive_type (par_state->language (),
					   par_state->gdbarch (),
					   "wide_character");
  return language_lookup_primitive_type (par_state->language (),
					 par_state->gdbarch (),
					 "wide_wide_character");
}

static struct type *
type_boolean (struct parser_state *par_state)
{
  return parse_type (par_state)->builtin_bool;
}

static struct type *
type_system_address (struct parser_state *par_state)
{
  struct type *type 
    = language_lookup_primitive_type (par_state->language (),
				      par_state->gdbarch (),
				      "system__address");
  return  type != NULL ? type : parse_type (par_state)->builtin_data_ptr;
}

void _initialize_ada_exp ();
void
_initialize_ada_exp ()
{
  obstack_init (&temp_parse_space);
}
