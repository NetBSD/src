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


#include <ctype.h>
#include <unordered_map>
#include "expression.h"
#include "value.h"
#include "parser-defs.h"
#include "language.h"
#include "ada-lang.h"
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

using namespace expr;

/* A convenience typedef.  */
typedef std::unique_ptr<ada_assign_operation> ada_assign_up;

/* Data that must be held for the duration of a parse.  */

struct ada_parse_state
{
  explicit ada_parse_state (const char *expr)
    : m_original_expr (expr)
  {
  }

  std::string find_completion_bounds ();

  const gdb_mpz *push_integer (gdb_mpz &&val)
  {
    auto &result = m_int_storage.emplace_back (new gdb_mpz (std::move (val)));
    return result.get ();
  }

  /* The components being constructed during this parse.  */
  std::vector<ada_component_up> components;

  /* The associations being constructed during this parse.  */
  std::vector<ada_association_up> associations;

  /* The stack of currently active assignment expressions.  This is used
     to implement '@', the target name symbol.  */
  std::vector<ada_assign_up> assignments;

  /* Track currently active iterated assignment names.  */
  std::unordered_map<std::string, std::vector<ada_index_var_operation *>>
       iterated_associations;

  auto_obstack temp_space;

  /* Depth of parentheses, used by the lexer.  */
  int paren_depth = 0;

  /* When completing, we'll return a special character at the end of the
     input, to signal the completion position to the lexer.  This is
     done because flex does not have a generally useful way to detect
     EOF in a pattern.  This variable records whether the special
     character has been emitted.  */
  bool returned_complete = false;

private:

  /* We don't have a good way to manage non-POD data in Yacc, so store
     values here.  The storage here is only valid for the duration of
     the parse.  */
  std::vector<std::unique_ptr<gdb_mpz>> m_int_storage;

  /* The original expression string.  */
  const char *m_original_expr;
};

/* The current Ada parser object.  */

static ada_parse_state *ada_parser;

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

static struct type *type_for_char (struct parser_state *, ULONGEST);

static struct type *type_system_address (struct parser_state *);

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
      callee_t = ada_check_typedef (callee_v->type ());
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

/* Create a new ada_component_up of the indicated type and arguments,
   and push it on the global 'components' vector.  */
template<typename T, typename... Arg>
void
push_component (Arg... args)
{
  ada_parser->components.emplace_back (new T (std::forward<Arg> (args)...));
}

/* Examine the final element of the 'components' vector, and return it
   as a pointer to an ada_choices_component.  The caller is
   responsible for ensuring that the final element is in fact an
   ada_choices_component.  */
static ada_choices_component *
choice_component ()
{
  ada_component *last = ada_parser->components.back ().get ();
  return gdb::checked_static_cast<ada_choices_component *> (last);
}

/* Pop the most recent component from the global stack, and return
   it.  */
static ada_component_up
pop_component ()
{
  ada_component_up result = std::move (ada_parser->components.back ());
  ada_parser->components.pop_back ();
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

/* Create a new ada_association_up of the indicated type and
   arguments, and push it on the global 'associations' vector.  */
template<typename T, typename... Arg>
void
push_association (Arg... args)
{
  ada_parser->associations.emplace_back (new T (std::forward<Arg> (args)...));
}

/* Pop the most recent association from the global stack, and return
   it.  */
static ada_association_up
pop_association ()
{
  ada_association_up result = std::move (ada_parser->associations.back ());
  ada_parser->associations.pop_back ();
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


#line 500 "ada-exp.c.tmp"

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
    WITH = 264,                    /* WITH  */
    DELTA = 265,                   /* DELTA  */
    COLONCOLON = 266,              /* COLONCOLON  */
    STRING = 267,                  /* STRING  */
    NAME = 268,                    /* NAME  */
    DOT_ID = 269,                  /* DOT_ID  */
    TICK_COMPLETE = 270,           /* TICK_COMPLETE  */
    DOT_COMPLETE = 271,            /* DOT_COMPLETE  */
    NAME_COMPLETE = 272,           /* NAME_COMPLETE  */
    DOLLAR_VARIABLE = 273,         /* DOLLAR_VARIABLE  */
    ASSIGN = 274,                  /* ASSIGN  */
    _AND_ = 275,                   /* _AND_  */
    OR = 276,                      /* OR  */
    XOR = 277,                     /* XOR  */
    THEN = 278,                    /* THEN  */
    ELSE = 279,                    /* ELSE  */
    NOTEQUAL = 280,                /* NOTEQUAL  */
    LEQ = 281,                     /* LEQ  */
    GEQ = 282,                     /* GEQ  */
    IN = 283,                      /* IN  */
    DOTDOT = 284,                  /* DOTDOT  */
    UNARY = 285,                   /* UNARY  */
    MOD = 286,                     /* MOD  */
    REM = 287,                     /* REM  */
    STARSTAR = 288,                /* STARSTAR  */
    ABS = 289,                     /* ABS  */
    NOT = 290,                     /* NOT  */
    VAR = 291,                     /* VAR  */
    ARROW = 292,                   /* ARROW  */
    TICK_ACCESS = 293,             /* TICK_ACCESS  */
    TICK_ADDRESS = 294,            /* TICK_ADDRESS  */
    TICK_FIRST = 295,              /* TICK_FIRST  */
    TICK_LAST = 296,               /* TICK_LAST  */
    TICK_LENGTH = 297,             /* TICK_LENGTH  */
    TICK_MAX = 298,                /* TICK_MAX  */
    TICK_MIN = 299,                /* TICK_MIN  */
    TICK_MODULUS = 300,            /* TICK_MODULUS  */
    TICK_POS = 301,                /* TICK_POS  */
    TICK_RANGE = 302,              /* TICK_RANGE  */
    TICK_SIZE = 303,               /* TICK_SIZE  */
    TICK_TAG = 304,                /* TICK_TAG  */
    TICK_VAL = 305,                /* TICK_VAL  */
    TICK_ENUM_REP = 306,           /* TICK_ENUM_REP  */
    TICK_ENUM_VAL = 307,           /* TICK_ENUM_VAL  */
    NEW = 308,                     /* NEW  */
    OTHERS = 309,                  /* OTHERS  */
    FOR = 310                      /* FOR  */
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
#define WITH 264
#define DELTA 265
#define COLONCOLON 266
#define STRING 267
#define NAME 268
#define DOT_ID 269
#define TICK_COMPLETE 270
#define DOT_COMPLETE 271
#define NAME_COMPLETE 272
#define DOLLAR_VARIABLE 273
#define ASSIGN 274
#define _AND_ 275
#define OR 276
#define XOR 277
#define THEN 278
#define ELSE 279
#define NOTEQUAL 280
#define LEQ 281
#define GEQ 282
#define IN 283
#define DOTDOT 284
#define UNARY 285
#define MOD 286
#define REM 287
#define STARSTAR 288
#define ABS 289
#define NOT 290
#define VAR 291
#define ARROW 292
#define TICK_ACCESS 293
#define TICK_ADDRESS 294
#define TICK_FIRST 295
#define TICK_LAST 296
#define TICK_LENGTH 297
#define TICK_MAX 298
#define TICK_MIN 299
#define TICK_MODULUS 300
#define TICK_POS 301
#define TICK_RANGE 302
#define TICK_SIZE 303
#define TICK_TAG 304
#define TICK_VAL 305
#define TICK_ENUM_REP 306
#define TICK_ENUM_VAL 307
#define NEW 308
#define OTHERS 309
#define FOR 310

/* Value type.  */
#if ! defined ada_exp_YYSTYPE && ! defined ada_exp_YYSTYPE_IS_DECLARED
union ada_exp_YYSTYPE
{
#line 466 "ada-exp.y"

    LONGEST lval;
    struct {
      const gdb_mpz *val;
      struct type *type;
    } typed_val;
    struct {
      LONGEST val;
      struct type *type;
    } typed_char;
    struct {
      gdb_byte val[16];
      struct type *type;
    } typed_val_float;
    struct type *tval;
    struct stoken sval;
    const struct block *bval;
    struct internalvar *ivar;
  

#line 681 "ada-exp.c.tmp"

};
typedef union ada_exp_YYSTYPE ada_exp_YYSTYPE;
# define ada_exp_YYSTYPE_IS_TRIVIAL 1
# define ada_exp_YYSTYPE_IS_DECLARED 1
#endif


extern ada_exp_YYSTYPE yylval;


int yyparse (void);



/* Symbol kind.  */
enum ada_exp_yysymbol_kind_t
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
  YYSYMBOL_WITH = 9,                       /* WITH  */
  YYSYMBOL_DELTA = 10,                     /* DELTA  */
  YYSYMBOL_COLONCOLON = 11,                /* COLONCOLON  */
  YYSYMBOL_STRING = 12,                    /* STRING  */
  YYSYMBOL_NAME = 13,                      /* NAME  */
  YYSYMBOL_DOT_ID = 14,                    /* DOT_ID  */
  YYSYMBOL_TICK_COMPLETE = 15,             /* TICK_COMPLETE  */
  YYSYMBOL_DOT_COMPLETE = 16,              /* DOT_COMPLETE  */
  YYSYMBOL_NAME_COMPLETE = 17,             /* NAME_COMPLETE  */
  YYSYMBOL_DOLLAR_VARIABLE = 18,           /* DOLLAR_VARIABLE  */
  YYSYMBOL_ASSIGN = 19,                    /* ASSIGN  */
  YYSYMBOL__AND_ = 20,                     /* _AND_  */
  YYSYMBOL_OR = 21,                        /* OR  */
  YYSYMBOL_XOR = 22,                       /* XOR  */
  YYSYMBOL_THEN = 23,                      /* THEN  */
  YYSYMBOL_ELSE = 24,                      /* ELSE  */
  YYSYMBOL_25_ = 25,                       /* '='  */
  YYSYMBOL_NOTEQUAL = 26,                  /* NOTEQUAL  */
  YYSYMBOL_27_ = 27,                       /* '<'  */
  YYSYMBOL_28_ = 28,                       /* '>'  */
  YYSYMBOL_LEQ = 29,                       /* LEQ  */
  YYSYMBOL_GEQ = 30,                       /* GEQ  */
  YYSYMBOL_IN = 31,                        /* IN  */
  YYSYMBOL_DOTDOT = 32,                    /* DOTDOT  */
  YYSYMBOL_33_ = 33,                       /* '@'  */
  YYSYMBOL_34_ = 34,                       /* '+'  */
  YYSYMBOL_35_ = 35,                       /* '-'  */
  YYSYMBOL_36_ = 36,                       /* '&'  */
  YYSYMBOL_UNARY = 37,                     /* UNARY  */
  YYSYMBOL_38_ = 38,                       /* '*'  */
  YYSYMBOL_39_ = 39,                       /* '/'  */
  YYSYMBOL_MOD = 40,                       /* MOD  */
  YYSYMBOL_REM = 41,                       /* REM  */
  YYSYMBOL_STARSTAR = 42,                  /* STARSTAR  */
  YYSYMBOL_ABS = 43,                       /* ABS  */
  YYSYMBOL_NOT = 44,                       /* NOT  */
  YYSYMBOL_VAR = 45,                       /* VAR  */
  YYSYMBOL_ARROW = 46,                     /* ARROW  */
  YYSYMBOL_47_ = 47,                       /* '|'  */
  YYSYMBOL_TICK_ACCESS = 48,               /* TICK_ACCESS  */
  YYSYMBOL_TICK_ADDRESS = 49,              /* TICK_ADDRESS  */
  YYSYMBOL_TICK_FIRST = 50,                /* TICK_FIRST  */
  YYSYMBOL_TICK_LAST = 51,                 /* TICK_LAST  */
  YYSYMBOL_TICK_LENGTH = 52,               /* TICK_LENGTH  */
  YYSYMBOL_TICK_MAX = 53,                  /* TICK_MAX  */
  YYSYMBOL_TICK_MIN = 54,                  /* TICK_MIN  */
  YYSYMBOL_TICK_MODULUS = 55,              /* TICK_MODULUS  */
  YYSYMBOL_TICK_POS = 56,                  /* TICK_POS  */
  YYSYMBOL_TICK_RANGE = 57,                /* TICK_RANGE  */
  YYSYMBOL_TICK_SIZE = 58,                 /* TICK_SIZE  */
  YYSYMBOL_TICK_TAG = 59,                  /* TICK_TAG  */
  YYSYMBOL_TICK_VAL = 60,                  /* TICK_VAL  */
  YYSYMBOL_TICK_ENUM_REP = 61,             /* TICK_ENUM_REP  */
  YYSYMBOL_TICK_ENUM_VAL = 62,             /* TICK_ENUM_VAL  */
  YYSYMBOL_63_ = 63,                       /* '.'  */
  YYSYMBOL_64_ = 64,                       /* '('  */
  YYSYMBOL_65_ = 65,                       /* '['  */
  YYSYMBOL_NEW = 66,                       /* NEW  */
  YYSYMBOL_OTHERS = 67,                    /* OTHERS  */
  YYSYMBOL_FOR = 68,                       /* FOR  */
  YYSYMBOL_69_ = 69,                       /* ';'  */
  YYSYMBOL_70_ = 70,                       /* ')'  */
  YYSYMBOL_71_ = 71,                       /* '\''  */
  YYSYMBOL_72_ = 72,                       /* ','  */
  YYSYMBOL_73_ = 73,                       /* '{'  */
  YYSYMBOL_74_ = 74,                       /* '}'  */
  YYSYMBOL_75_ = 75,                       /* ']'  */
  YYSYMBOL_YYACCEPT = 76,                  /* $accept  */
  YYSYMBOL_start = 77,                     /* start  */
  YYSYMBOL_exp1 = 78,                      /* exp1  */
  YYSYMBOL_79_1 = 79,                      /* $@1  */
  YYSYMBOL_primary = 80,                   /* primary  */
  YYSYMBOL_simple_exp = 81,                /* simple_exp  */
  YYSYMBOL_arglist = 82,                   /* arglist  */
  YYSYMBOL_relation = 83,                  /* relation  */
  YYSYMBOL_exp = 84,                       /* exp  */
  YYSYMBOL_and_exp = 85,                   /* and_exp  */
  YYSYMBOL_and_then_exp = 86,              /* and_then_exp  */
  YYSYMBOL_or_exp = 87,                    /* or_exp  */
  YYSYMBOL_or_else_exp = 88,               /* or_else_exp  */
  YYSYMBOL_xor_exp = 89,                   /* xor_exp  */
  YYSYMBOL_tick_arglist = 90,              /* tick_arglist  */
  YYSYMBOL_type_prefix = 91,               /* type_prefix  */
  YYSYMBOL_opt_type_prefix = 92,           /* opt_type_prefix  */
  YYSYMBOL_var_or_type = 93,               /* var_or_type  */
  YYSYMBOL_block = 94,                     /* block  */
  YYSYMBOL_aggregate = 95,                 /* aggregate  */
  YYSYMBOL_aggregate_component_list = 96,  /* aggregate_component_list  */
  YYSYMBOL_positional_list = 97,           /* positional_list  */
  YYSYMBOL_component_groups = 98,          /* component_groups  */
  YYSYMBOL_others = 99,                    /* others  */
  YYSYMBOL_component_group = 100,          /* component_group  */
  YYSYMBOL_101_2 = 101,                    /* $@2  */
  YYSYMBOL_component_associations = 102    /* component_associations  */
};
typedef enum ada_exp_yysymbol_kind_t ada_exp_yysymbol_kind_t;




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
         || (defined ada_exp_YYSTYPE_IS_TRIVIAL && ada_exp_YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union ada_exp_yyalloc
{
  yy_state_t yyss_alloc;
  ada_exp_YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union ada_exp_yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (ada_exp_YYSTYPE)) \
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
#define YYFINAL  60
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   849

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  76
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  27
/* YYNRULES -- Number of rules.  */
#define YYNRULES  126
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  250

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   310


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (ada_exp_yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    36,    71,
      64,    70,    38,    34,    72,    35,    63,    39,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    69,
      27,    25,    28,     2,    33,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    65,     2,    75,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    73,    47,    74,     2,     2,     2,     2,
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
      26,    29,    30,    31,    32,    37,    40,    41,    42,    43,
      44,    45,    46,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    66,    67,
      68
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   534,   534,   538,   539,   542,   541,   565,   578,   592,
     594,   609,   621,   623,   631,   642,   648,   652,   659,   670,
     673,   677,   693,   700,   704,   707,   709,   711,   713,   717,
     730,   734,   738,   742,   746,   750,   754,   758,   762,   766,
     769,   773,   777,   781,   783,   790,   798,   801,   809,   820,
     824,   828,   832,   833,   834,   835,   836,   837,   841,   844,
     850,   853,   859,   862,   868,   870,   874,   877,   890,   892,
     894,   898,   904,   910,   916,   918,   920,   922,   924,   926,
     932,   938,   944,   954,   956,   961,   970,   973,   977,   984,
     990,  1001,  1009,  1016,  1021,  1028,  1032,  1034,  1040,  1042,
    1048,  1056,  1067,  1069,  1074,  1083,  1094,  1095,  1101,  1106,
    1112,  1121,  1122,  1123,  1127,  1134,  1140,  1139,  1174,  1180,
    1186,  1195,  1200,  1205,  1219,  1221,  1223
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (ada_exp_yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (ada_exp_yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "INT", "NULL_PTR",
  "CHARLIT", "FLOAT", "TRUEKEYWORD", "FALSEKEYWORD", "WITH", "DELTA",
  "COLONCOLON", "STRING", "NAME", "DOT_ID", "TICK_COMPLETE",
  "DOT_COMPLETE", "NAME_COMPLETE", "DOLLAR_VARIABLE", "ASSIGN", "_AND_",
  "OR", "XOR", "THEN", "ELSE", "'='", "NOTEQUAL", "'<'", "'>'", "LEQ",
  "GEQ", "IN", "DOTDOT", "'@'", "'+'", "'-'", "'&'", "UNARY", "'*'", "'/'",
  "MOD", "REM", "STARSTAR", "ABS", "NOT", "VAR", "ARROW", "'|'",
  "TICK_ACCESS", "TICK_ADDRESS", "TICK_FIRST", "TICK_LAST", "TICK_LENGTH",
  "TICK_MAX", "TICK_MIN", "TICK_MODULUS", "TICK_POS", "TICK_RANGE",
  "TICK_SIZE", "TICK_TAG", "TICK_VAL", "TICK_ENUM_REP", "TICK_ENUM_VAL",
  "'.'", "'('", "'['", "NEW", "OTHERS", "FOR", "';'", "')'", "'\\''",
  "','", "'{'", "'}'", "']'", "$accept", "start", "exp1", "$@1", "primary",
  "simple_exp", "arglist", "relation", "exp", "and_exp", "and_then_exp",
  "or_exp", "or_else_exp", "xor_exp", "tick_arglist", "type_prefix",
  "opt_type_prefix", "var_or_type", "block", "aggregate",
  "aggregate_component_list", "positional_list", "component_groups",
  "others", "component_group", "$@2", "component_associations", YY_NULLPTRPTR
};

static const char *
yysymbol_name (ada_exp_yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-109)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-86)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     447,  -109,  -109,  -109,  -109,  -109,  -109,  -109,    18,  -109,
    -109,  -109,   447,   447,   573,   573,   447,   447,   278,    20,
      52,    16,   -35,   568,   742,   112,  -109,    28,    34,    56,
      88,    33,    60,    -7,   244,    58,  -109,  -109,  -109,   638,
      62,    62,    -2,    -2,    62,    62,     4,    -6,    97,   -43,
     679,    -4,    42,   278,  -109,  -109,    44,  -109,  -109,    50,
    -109,   447,  -109,  -109,  -109,  -109,  -109,  -109,    54,    54,
      54,  -109,  -109,   320,   447,   447,   447,   447,   447,   447,
     447,   447,   447,   447,   447,   447,   447,   447,   447,   447,
     447,    98,   362,   405,   447,   447,   107,   447,   113,   447,
    -109,    72,    75,    76,    77,    79,    80,   320,    81,    19,
    -109,   447,   489,   447,   121,  -109,   447,   447,   489,   149,
    -109,  -109,    89,  -109,   278,   573,  -109,   447,   157,  -109,
    -109,  -109,    10,   702,     9,  -109,    90,   807,   807,   807,
     807,   807,   807,   599,   786,   126,   115,    62,    62,    62,
     122,   122,   122,   122,   122,   447,   447,  -109,   447,  -109,
    -109,  -109,   447,  -109,   447,  -109,   447,   447,   447,   447,
     447,   447,   722,    35,   447,  -109,  -109,  -109,   755,  -109,
    -109,  -109,   770,  -109,  -109,   278,  -109,  -109,    -2,  -109,
      93,   447,   447,  -109,   531,  -109,    54,   447,   620,   797,
     205,  -109,  -109,  -109,  -109,   103,   105,   106,   111,   117,
     108,   447,  -109,   114,   489,   447,   489,   128,   123,  -109,
    -109,     3,    11,  -109,  -109,   807,    54,   447,  -109,  -109,
    -109,   447,   447,  -109,   209,  -109,  -109,  -109,  -109,  -109,
    -109,   447,  -109,   807,   132,   133,  -109,  -109,  -109,  -109
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
      87,    88,    91,    89,    90,    93,    94,    92,    96,    97,
      16,    18,    87,    87,    87,    87,    87,    87,    87,     0,
       0,     0,     2,    19,    39,    52,     3,    53,    54,    55,
      56,    57,    86,     0,    15,     0,    17,   102,   100,    19,
      21,    20,   125,   124,    23,    22,    96,     0,     0,     0,
      39,     3,     0,    87,   106,   111,   112,   115,    95,     0,
       1,    87,     7,    70,     8,     5,    68,    69,    83,    83,
      83,    74,    75,    87,    87,    87,    87,    87,    87,    87,
      87,    87,    87,    87,    87,    87,    87,    87,    87,    87,
      87,     0,    87,    87,    87,    87,     0,    87,     0,    87,
      82,     0,     0,     0,     0,     0,     0,    87,     0,    98,
      99,    87,    87,    87,     0,    14,    87,    87,    87,     0,
     109,   105,   107,   108,    87,    87,     4,    87,     0,    71,
      72,    73,    96,    39,     0,    25,     0,    40,    41,    50,
      51,    42,    49,    19,     0,    15,    35,    36,    38,    37,
      31,    32,    34,    33,    30,    87,    87,    58,    87,    62,
      66,    59,    87,    63,    87,    67,    87,    87,    87,    87,
      87,    87,    39,     0,    87,   103,   101,   118,     0,   121,
     114,   116,     0,   119,   122,    87,   110,   113,    29,     6,
       0,    87,    87,     9,    87,   126,    83,    87,    19,     0,
      15,    60,    64,    61,    65,     0,     0,     0,     0,     0,
       0,    87,    10,     0,    87,    87,    87,     0,     0,    84,
      26,     0,    96,    27,    44,    43,    83,    87,    79,    80,
      81,    87,    87,    78,     0,    11,   117,   120,   123,   104,
      12,    87,    47,    46,     0,     0,    13,    28,    77,    76
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -109,  -109,   159,  -109,    17,     7,    99,   -86,     0,  -109,
    -109,  -109,  -109,  -109,   -68,  -109,  -109,   -17,  -109,  -109,
      22,  -109,   -25,  -109,  -109,  -109,  -108
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,    21,    22,   127,    39,    24,   134,    25,   135,    27,
      28,    29,    30,    31,   129,    32,    33,    34,    35,    36,
      52,    53,    54,    55,    56,   214,    57
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      26,   130,   131,    59,   179,   119,   157,   159,   160,   161,
     184,   163,    62,   165,    64,    37,    60,    23,    51,    40,
      41,    37,    37,    44,    45,    50,    61,   115,   123,    37,
     175,    42,    43,    58,    61,    23,    82,    83,    84,    85,
     113,    86,    87,    88,    89,    90,   104,   105,    95,   106,
     111,   112,    38,   122,    96,    99,   191,   241,    38,    38,
      50,   126,    73,    74,   145,     8,    38,   176,   120,     9,
     201,   109,   202,   240,   136,   110,   203,    97,   204,   193,
     133,   194,   137,   138,   139,   140,   141,   142,   144,   146,
     147,   148,   149,   150,   151,   152,   153,   154,   143,   187,
      86,    87,    88,    89,    90,   212,   236,   194,   238,    98,
     114,   177,   121,   180,   172,   100,   124,   183,   128,   178,
     101,   102,   103,   182,   125,   178,   -45,   189,   224,   155,
     162,   178,    92,    93,    94,   -45,   166,   164,   200,   167,
     168,   169,   188,   170,   171,   174,   -45,   -45,   -45,    83,
      84,    85,   181,    86,    87,    88,    89,    90,   242,   185,
     190,   186,   199,   219,    90,   195,   205,   206,   207,   208,
     209,   210,   198,   228,   213,   229,   230,    49,   233,   -85,
     -85,   -85,   -85,   231,   235,   217,   -85,   -85,   -85,   232,
     107,   220,    50,   239,   223,   -45,   -45,   108,   -45,   221,
     120,   -45,   248,   249,   225,   -48,   173,   218,     0,     0,
       0,     0,     0,     0,   -48,   237,     0,     0,   234,     0,
       0,   178,     0,   178,     0,   -48,   -48,   -48,     0,     0,
       0,   244,   245,     0,   243,     0,     0,     0,     0,     0,
       0,   247,    82,    83,    84,    85,     0,    86,    87,    88,
      89,    90,     0,     0,     0,     0,     0,     0,   -85,   -85,
     -85,   -85,     0,     0,     0,   -85,   -85,   -85,     0,   107,
       0,     0,     0,     0,   -48,   -48,   108,   -48,     0,   246,
     -48,     1,     2,     3,     4,     5,     6,     0,     0,     0,
       7,    46,     0,     0,     0,     9,    10,   -85,   -85,   -85,
     -85,     0,     0,     0,   -85,   -85,   -85,     0,   107,     0,
       0,    11,    12,    13,    14,   108,    15,     0,     0,     0,
       0,    16,    17,     1,     2,     3,     4,     5,     6,     0,
       0,     0,     7,   132,     0,     0,     0,     9,    10,     0,
       0,     0,    18,     0,    19,    47,    48,     0,     0,     0,
       0,    20,     0,    11,    12,    13,    14,     0,    15,     0,
       0,     0,     0,    16,    17,     1,     2,     3,     4,     5,
       6,     0,     0,     0,     7,     8,     0,     0,     0,     9,
      10,     0,     0,     0,    18,   156,    19,     0,     0,     0,
     -24,     0,   -24,    20,     0,    11,    12,    13,    14,     0,
      15,     0,     0,     0,     0,    16,    17,     0,     1,     2,
       3,     4,     5,     6,     0,     0,     0,     7,     8,     0,
       0,     0,     9,    10,     0,     0,    18,     0,    19,   158,
       0,     0,     0,     0,     0,    20,     0,     0,    11,    12,
      13,    14,     0,    15,     0,     0,     0,     0,    16,    17,
       1,     2,     3,     4,     5,     6,     0,     0,     0,     7,
       8,     0,     0,     0,     9,    10,     0,     0,     0,    18,
       0,    19,     0,     0,     0,     0,     0,     0,    20,     0,
      11,    12,    13,    14,     0,    15,     0,     0,     0,     0,
      16,    17,     1,     2,     3,     4,     5,     6,     0,     0,
       0,     7,    46,     0,     0,     0,     9,    10,     0,     0,
       0,    18,     0,    19,     0,     0,     0,     0,     0,     0,
      20,     0,    11,    12,    13,    14,     0,    15,     0,     0,
       0,     0,    16,    17,     1,     2,     3,     4,     5,     6,
       0,     0,     0,     7,   222,     0,     0,     0,     9,    10,
       0,     0,     0,    18,     0,    19,     0,     0,     0,     0,
       0,     0,    20,     0,    11,    12,    13,    14,     0,    15,
       0,     0,     0,     0,    16,    17,     1,     2,     3,     4,
       5,     6,    62,    63,    64,     7,     8,    65,     0,     0,
       9,    10,     0,     0,     0,    18,     0,    19,     0,     0,
       0,     0,     0,     0,    20,     0,    11,     0,     0,    14,
       0,    15,     0,    62,    63,    64,    66,    67,    68,    69,
      70,     0,     0,     0,     0,     0,    71,    72,     0,     0,
       0,     0,    73,    74,    62,    63,    64,    18,     0,    19,
       0,     0,     0,     0,     0,     0,    20,    66,    67,    68,
      69,    70,    62,    63,    64,     0,   196,    71,    72,     0,
       0,     0,     0,    73,    74,     0,     0,     0,    66,    67,
      68,    69,    70,     0,     0,     0,     0,   226,    71,    72,
       0,     0,     0,     0,    73,    74,    66,    67,    68,    69,
      70,     0,     0,     0,     0,     0,    71,    72,     0,     0,
       0,     0,    73,    74,    75,    76,    77,    78,    79,    80,
      81,   116,    82,    83,    84,    85,     0,    86,    87,    88,
      89,    90,     0,    91,     0,   117,   118,    75,    76,    77,
      78,    79,    80,    81,   192,    82,    83,    84,    85,     0,
      86,    87,    88,    89,    90,     0,    91,    75,    76,    77,
      78,    79,    80,    81,   211,    82,    83,    84,    85,     0,
      86,    87,    88,    89,    90,     0,    91,    75,    76,    77,
      78,    79,    80,    81,     0,    82,    83,    84,    85,     0,
      86,    87,    88,    89,    90,     0,    91,   116,    82,    83,
      84,    85,     0,    86,    87,    88,    89,    90,     0,     0,
       0,   117,   118,    82,    83,    84,    85,     0,    86,    87,
      88,    89,    90,     0,     0,     0,   215,   216,   197,    82,
      83,    84,    85,     0,    86,    87,    88,    89,    90,   227,
      82,    83,    84,    85,     0,    86,    87,    88,    89,    90,
      82,    83,    84,    85,     0,    86,    87,    88,    89,    90
};

static const yytype_int16 yycheck[] =
{
       0,    69,    70,    20,   112,     9,    92,    93,    94,    95,
     118,    97,    14,    99,    16,    11,     0,     0,    18,    12,
      13,    11,    11,    16,    17,    18,    69,    70,    53,    11,
      11,    14,    15,    13,    69,    18,    33,    34,    35,    36,
      46,    38,    39,    40,    41,    42,    53,    54,    20,    56,
      46,    47,    48,    53,    20,    22,    46,    46,    48,    48,
      53,    61,    64,    65,    81,    13,    48,    48,    72,    17,
     156,    13,   158,    70,    74,    17,   162,    21,   164,    70,
      73,    72,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    81,   124,
      38,    39,    40,    41,    42,    70,   214,    72,   216,    21,
      13,   111,    70,   113,   107,    55,    72,   117,    64,   112,
      60,    61,    62,   116,    74,   118,     0,   127,   196,    31,
      23,   124,    20,    21,    22,     9,    64,    24,   155,    64,
      64,    64,   125,    64,    64,    64,    20,    21,    22,    34,
      35,    36,    31,    38,    39,    40,    41,    42,   226,    10,
       3,    72,   155,    70,    42,    75,   166,   167,   168,   169,
     170,   171,   155,    70,   174,    70,    70,    18,    70,    53,
      54,    55,    56,    72,    70,   185,    60,    61,    62,    72,
      64,   191,   185,    70,   194,    69,    70,    71,    72,   192,
      72,    75,    70,    70,   197,     0,   107,   185,    -1,    -1,
      -1,    -1,    -1,    -1,     9,   215,    -1,    -1,   211,    -1,
      -1,   214,    -1,   216,    -1,    20,    21,    22,    -1,    -1,
      -1,   231,   232,    -1,   227,    -1,    -1,    -1,    -1,    -1,
      -1,   241,    33,    34,    35,    36,    -1,    38,    39,    40,
      41,    42,    -1,    -1,    -1,    -1,    -1,    -1,    53,    54,
      55,    56,    -1,    -1,    -1,    60,    61,    62,    -1,    64,
      -1,    -1,    -1,    -1,    69,    70,    71,    72,    -1,    70,
      75,     3,     4,     5,     6,     7,     8,    -1,    -1,    -1,
      12,    13,    -1,    -1,    -1,    17,    18,    53,    54,    55,
      56,    -1,    -1,    -1,    60,    61,    62,    -1,    64,    -1,
      -1,    33,    34,    35,    36,    71,    38,    -1,    -1,    -1,
      -1,    43,    44,     3,     4,     5,     6,     7,     8,    -1,
      -1,    -1,    12,    13,    -1,    -1,    -1,    17,    18,    -1,
      -1,    -1,    64,    -1,    66,    67,    68,    -1,    -1,    -1,
      -1,    73,    -1,    33,    34,    35,    36,    -1,    38,    -1,
      -1,    -1,    -1,    43,    44,     3,     4,     5,     6,     7,
       8,    -1,    -1,    -1,    12,    13,    -1,    -1,    -1,    17,
      18,    -1,    -1,    -1,    64,    23,    66,    -1,    -1,    -1,
      70,    -1,    72,    73,    -1,    33,    34,    35,    36,    -1,
      38,    -1,    -1,    -1,    -1,    43,    44,    -1,     3,     4,
       5,     6,     7,     8,    -1,    -1,    -1,    12,    13,    -1,
      -1,    -1,    17,    18,    -1,    -1,    64,    -1,    66,    24,
      -1,    -1,    -1,    -1,    -1,    73,    -1,    -1,    33,    34,
      35,    36,    -1,    38,    -1,    -1,    -1,    -1,    43,    44,
       3,     4,     5,     6,     7,     8,    -1,    -1,    -1,    12,
      13,    -1,    -1,    -1,    17,    18,    -1,    -1,    -1,    64,
      -1,    66,    -1,    -1,    -1,    -1,    -1,    -1,    73,    -1,
      33,    34,    35,    36,    -1,    38,    -1,    -1,    -1,    -1,
      43,    44,     3,     4,     5,     6,     7,     8,    -1,    -1,
      -1,    12,    13,    -1,    -1,    -1,    17,    18,    -1,    -1,
      -1,    64,    -1,    66,    -1,    -1,    -1,    -1,    -1,    -1,
      73,    -1,    33,    34,    35,    36,    -1,    38,    -1,    -1,
      -1,    -1,    43,    44,     3,     4,     5,     6,     7,     8,
      -1,    -1,    -1,    12,    13,    -1,    -1,    -1,    17,    18,
      -1,    -1,    -1,    64,    -1,    66,    -1,    -1,    -1,    -1,
      -1,    -1,    73,    -1,    33,    34,    35,    36,    -1,    38,
      -1,    -1,    -1,    -1,    43,    44,     3,     4,     5,     6,
       7,     8,    14,    15,    16,    12,    13,    19,    -1,    -1,
      17,    18,    -1,    -1,    -1,    64,    -1,    66,    -1,    -1,
      -1,    -1,    -1,    -1,    73,    -1,    33,    -1,    -1,    36,
      -1,    38,    -1,    14,    15,    16,    48,    49,    50,    51,
      52,    -1,    -1,    -1,    -1,    -1,    58,    59,    -1,    -1,
      -1,    -1,    64,    65,    14,    15,    16,    64,    -1,    66,
      -1,    -1,    -1,    -1,    -1,    -1,    73,    48,    49,    50,
      51,    52,    14,    15,    16,    -1,    57,    58,    59,    -1,
      -1,    -1,    -1,    64,    65,    -1,    -1,    -1,    48,    49,
      50,    51,    52,    -1,    -1,    -1,    -1,    57,    58,    59,
      -1,    -1,    -1,    -1,    64,    65,    48,    49,    50,    51,
      52,    -1,    -1,    -1,    -1,    -1,    58,    59,    -1,    -1,
      -1,    -1,    64,    65,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    -1,    38,    39,    40,
      41,    42,    -1,    44,    -1,    46,    47,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    -1,
      38,    39,    40,    41,    42,    -1,    44,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    -1,
      38,    39,    40,    41,    42,    -1,    44,    25,    26,    27,
      28,    29,    30,    31,    -1,    33,    34,    35,    36,    -1,
      38,    39,    40,    41,    42,    -1,    44,    32,    33,    34,
      35,    36,    -1,    38,    39,    40,    41,    42,    -1,    -1,
      -1,    46,    47,    33,    34,    35,    36,    -1,    38,    39,
      40,    41,    42,    -1,    -1,    -1,    46,    47,    32,    33,
      34,    35,    36,    -1,    38,    39,    40,    41,    42,    32,
      33,    34,    35,    36,    -1,    38,    39,    40,    41,    42,
      33,    34,    35,    36,    -1,    38,    39,    40,    41,    42
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,    12,    13,    17,
      18,    33,    34,    35,    36,    38,    43,    44,    64,    66,
      73,    77,    78,    80,    81,    83,    84,    85,    86,    87,
      88,    89,    91,    92,    93,    94,    95,    11,    48,    80,
      81,    81,    80,    80,    81,    81,    13,    67,    68,    78,
      81,    84,    96,    97,    98,    99,   100,   102,    13,    93,
       0,    69,    14,    15,    16,    19,    48,    49,    50,    51,
      52,    58,    59,    64,    65,    25,    26,    27,    28,    29,
      30,    31,    33,    34,    35,    36,    38,    39,    40,    41,
      42,    44,    20,    21,    22,    20,    20,    21,    21,    22,
      55,    60,    61,    62,    53,    54,    56,    64,    71,    13,
      17,    46,    47,    46,    13,    70,    32,    46,    47,     9,
      72,    70,    84,    98,    72,    74,    84,    79,    64,    90,
      90,    90,    13,    81,    82,    84,    84,    81,    81,    81,
      81,    81,    81,    80,    81,    93,    81,    81,    81,    81,
      81,    81,    81,    81,    81,    31,    23,    83,    24,    83,
      83,    83,    23,    83,    24,    83,    64,    64,    64,    64,
      64,    64,    81,    82,    64,    11,    48,    84,    81,   102,
      84,    31,    81,    84,   102,    10,    72,    98,    80,    84,
       3,    46,    32,    70,    72,    75,    57,    32,    80,    81,
      93,    83,    83,    83,    83,    84,    84,    84,    84,    84,
      84,    32,    70,    84,   101,    46,    47,    84,    96,    70,
      84,    81,    13,    84,    90,    81,    57,    32,    70,    70,
      70,    72,    72,    70,    81,    70,   102,    84,   102,    70,
      70,    46,    90,    81,    84,    84,    70,    84,    70,    70
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    76,    77,    78,    78,    79,    78,    80,    80,    80,
      80,    80,    80,    80,    80,    80,    80,    80,    80,    81,
      81,    81,    81,    81,    82,    82,    82,    82,    82,    80,
      81,    81,    81,    81,    81,    81,    81,    81,    81,    83,
      83,    83,    83,    83,    83,    83,    83,    83,    83,    83,
      83,    83,    84,    84,    84,    84,    84,    84,    85,    85,
      86,    86,    87,    87,    88,    88,    89,    89,    80,    80,
      80,    80,    80,    80,    80,    80,    80,    80,    80,    80,
      80,    80,    80,    90,    90,    91,    92,    92,    80,    80,
      80,    80,    80,    80,    80,    80,    93,    93,    93,    93,
      93,    93,    94,    94,    95,    95,    96,    96,    96,    97,
      97,    98,    98,    98,    99,   100,   101,   100,   102,   102,
     102,   102,   102,   102,    80,    80,    80
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     3,     0,     4,     2,     2,     4,
       4,     5,     6,     6,     3,     1,     1,     1,     1,     1,
       2,     2,     2,     2,     0,     1,     3,     3,     5,     4,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     1,
       3,     3,     3,     5,     5,     3,     6,     6,     4,     3,
       3,     3,     1,     1,     1,     1,     1,     1,     3,     3,
       4,     4,     3,     3,     4,     4,     3,     3,     2,     2,
       2,     3,     3,     3,     2,     2,     7,     7,     5,     5,
       5,     5,     2,     0,     3,     1,     1,     0,     1,     1,
       1,     1,     1,     1,     1,     2,     1,     1,     2,     2,
       2,     3,     2,     3,     6,     3,     1,     2,     2,     2,
       3,     1,     1,     3,     3,     1,     0,     5,     3,     3,
       5,     3,     3,     5,     2,     2,     4
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
                       ada_exp_yysymbol_kind_t yykind, ada_exp_YYSTYPE const * const yyvaluep)
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
                 ada_exp_yysymbol_kind_t yykind, ada_exp_YYSTYPE const * const yyvaluep)
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
yy_reduce_print (yy_state_t *yyssp, ada_exp_YYSTYPE *yyvsp,
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
            ada_exp_yysymbol_kind_t yykind, ada_exp_YYSTYPE *yyvaluep)
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
ada_exp_YYSTYPE yylval;
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
    ada_exp_YYSTYPE yyvsa[YYINITDEPTH];
    ada_exp_YYSTYPE *yyvs = yyvsa;
    ada_exp_YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  ada_exp_yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  ada_exp_YYSTYPE yyval;



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
        ada_exp_YYSTYPE *yyvs1 = yyvs;

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
        union ada_exp_yyalloc *yyptr =
          YY_CAST (union ada_exp_yyalloc *,
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
#line 540 "ada-exp.y"
                        { ada_wrap2<comma_operation> (BINOP_COMMA); }
#line 2049 "ada-exp.c.tmp"
    break;

  case 5: /* $@1: %empty  */
#line 542 "ada-exp.y"
                        {
			  ada_parser->assignments.emplace_back
			    (new ada_assign_operation (ada_pop (), nullptr));
			}
#line 2058 "ada-exp.c.tmp"
    break;

  case 6: /* exp1: primary ASSIGN $@1 exp  */
#line 547 "ada-exp.y"
                        {
			  ada_assign_up assign
			    = std::move (ada_parser->assignments.back ());
			  ada_parser->assignments.pop_back ();
			  value *lhs_val = (assign->eval_for_resolution
					    (pstate->expout.get ()));

			  operation_up rhs = pstate->pop ();
			  rhs = resolve (std::move (rhs), true,
					 lhs_val->type ());

			  assign->set_rhs (std::move (rhs));
			  pstate->push (std::move (assign));
			}
#line 2077 "ada-exp.c.tmp"
    break;

  case 7: /* primary: primary DOT_ID  */
#line 566 "ada-exp.y"
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
#line 2092 "ada-exp.c.tmp"
    break;

  case 8: /* primary: primary DOT_COMPLETE  */
#line 579 "ada-exp.y"
                        {
			  /* This is done even for ".all", because
			     that might be a prefix.  */
			  operation_up arg = ada_pop ();
			  ada_structop_operation *str_op
			    = (new ada_structop_operation
			       (std::move (arg), copy_name ((yyvsp[0].sval))));
			  str_op->set_prefix (ada_parser->find_completion_bounds ());
			  pstate->push (operation_up (str_op));
			  pstate->mark_struct_expression (str_op);
			}
#line 2108 "ada-exp.c.tmp"
    break;

  case 9: /* primary: primary '(' arglist ')'  */
#line 593 "ada-exp.y"
                        { ada_funcall ((yyvsp[-1].lval)); }
#line 2114 "ada-exp.c.tmp"
    break;

  case 10: /* primary: var_or_type '(' arglist ')'  */
#line 595 "ada-exp.y"
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
#line 2131 "ada-exp.c.tmp"
    break;

  case 11: /* primary: var_or_type '\'' '(' exp ')'  */
#line 610 "ada-exp.y"
                        {
			  if ((yyvsp[-4].tval) == NULL)
			    error (_("Type required for qualification"));
			  operation_up arg = ada_pop (true,
						      check_typedef ((yyvsp[-4].tval)));
			  pstate->push_new<ada_qual_operation>
			    (std::move (arg), (yyvsp[-4].tval));
			}
#line 2144 "ada-exp.c.tmp"
    break;

  case 12: /* primary: primary '(' simple_exp DOTDOT simple_exp ')'  */
#line 622 "ada-exp.y"
                        { ada_wrap3<ada_ternop_slice_operation> (); }
#line 2150 "ada-exp.c.tmp"
    break;

  case 13: /* primary: var_or_type '(' simple_exp DOTDOT simple_exp ')'  */
#line 624 "ada-exp.y"
                        { if ((yyvsp[-5].tval) == NULL) 
			    ada_wrap3<ada_ternop_slice_operation> ();
			  else
			    error (_("Cannot slice a type"));
			}
#line 2160 "ada-exp.c.tmp"
    break;

  case 14: /* primary: '(' exp1 ')'  */
#line 631 "ada-exp.y"
                                { }
#line 2166 "ada-exp.c.tmp"
    break;

  case 15: /* primary: var_or_type  */
#line 643 "ada-exp.y"
                        { if ((yyvsp[0].tval) != NULL)
			    pstate->push_new<type_operation> ((yyvsp[0].tval));
			}
#line 2174 "ada-exp.c.tmp"
    break;

  case 16: /* primary: DOLLAR_VARIABLE  */
#line 649 "ada-exp.y"
                        { pstate->push_dollar ((yyvsp[0].sval)); }
#line 2180 "ada-exp.c.tmp"
    break;

  case 17: /* primary: aggregate  */
#line 653 "ada-exp.y"
                        {
			  pstate->push_new<ada_aggregate_operation>
			    (pop_component ());
			}
#line 2189 "ada-exp.c.tmp"
    break;

  case 18: /* primary: '@'  */
#line 660 "ada-exp.y"
                        {
			  if (ada_parser->assignments.empty ())
			    error (_("the target name symbol ('@') may only "
				     "appear in an assignment context"));
			  ada_assign_operation *current
			    = ada_parser->assignments.back ().get ();
			  pstate->push_new<ada_target_operation> (current);
			}
#line 2202 "ada-exp.c.tmp"
    break;

  case 20: /* simple_exp: '-' simple_exp  */
#line 674 "ada-exp.y"
                        { ada_wrap_overload<ada_neg_operation> (UNOP_NEG); }
#line 2208 "ada-exp.c.tmp"
    break;

  case 21: /* simple_exp: '+' simple_exp  */
#line 678 "ada-exp.y"
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
#line 2226 "ada-exp.c.tmp"
    break;

  case 22: /* simple_exp: NOT simple_exp  */
#line 694 "ada-exp.y"
                        {
			  ada_wrap_overload<unary_logical_not_operation>
			    (UNOP_LOGICAL_NOT);
			}
#line 2235 "ada-exp.c.tmp"
    break;

  case 23: /* simple_exp: ABS simple_exp  */
#line 701 "ada-exp.y"
                        { ada_wrap_overload<ada_abs_operation> (UNOP_ABS); }
#line 2241 "ada-exp.c.tmp"
    break;

  case 24: /* arglist: %empty  */
#line 704 "ada-exp.y"
                        { (yyval.lval) = 0; }
#line 2247 "ada-exp.c.tmp"
    break;

  case 25: /* arglist: exp  */
#line 708 "ada-exp.y"
                        { (yyval.lval) = 1; }
#line 2253 "ada-exp.c.tmp"
    break;

  case 26: /* arglist: NAME ARROW exp  */
#line 710 "ada-exp.y"
                        { (yyval.lval) = 1; }
#line 2259 "ada-exp.c.tmp"
    break;

  case 27: /* arglist: arglist ',' exp  */
#line 712 "ada-exp.y"
                        { (yyval.lval) = (yyvsp[-2].lval) + 1; }
#line 2265 "ada-exp.c.tmp"
    break;

  case 28: /* arglist: arglist ',' NAME ARROW exp  */
#line 714 "ada-exp.y"
                        { (yyval.lval) = (yyvsp[-4].lval) + 1; }
#line 2271 "ada-exp.c.tmp"
    break;

  case 29: /* primary: '{' var_or_type '}' primary  */
#line 719 "ada-exp.y"
                        { 
			  if ((yyvsp[-2].tval) == NULL)
			    error (_("Type required within braces in coercion"));
			  operation_up arg = ada_pop ();
			  pstate->push_new<unop_memval_operation>
			    (std::move (arg), (yyvsp[-2].tval));
			}
#line 2283 "ada-exp.c.tmp"
    break;

  case 30: /* simple_exp: simple_exp STARSTAR simple_exp  */
#line 731 "ada-exp.y"
                        { ada_wrap2<ada_binop_exp_operation> (BINOP_EXP); }
#line 2289 "ada-exp.c.tmp"
    break;

  case 31: /* simple_exp: simple_exp '*' simple_exp  */
#line 735 "ada-exp.y"
                        { ada_wrap2<ada_binop_mul_operation> (BINOP_MUL); }
#line 2295 "ada-exp.c.tmp"
    break;

  case 32: /* simple_exp: simple_exp '/' simple_exp  */
#line 739 "ada-exp.y"
                        { ada_wrap2<ada_binop_div_operation> (BINOP_DIV); }
#line 2301 "ada-exp.c.tmp"
    break;

  case 33: /* simple_exp: simple_exp REM simple_exp  */
#line 743 "ada-exp.y"
                        { ada_wrap2<ada_binop_rem_operation> (BINOP_REM); }
#line 2307 "ada-exp.c.tmp"
    break;

  case 34: /* simple_exp: simple_exp MOD simple_exp  */
#line 747 "ada-exp.y"
                        { ada_wrap2<ada_binop_mod_operation> (BINOP_MOD); }
#line 2313 "ada-exp.c.tmp"
    break;

  case 35: /* simple_exp: simple_exp '@' simple_exp  */
#line 751 "ada-exp.y"
                        { ada_wrap2<repeat_operation> (BINOP_REPEAT); }
#line 2319 "ada-exp.c.tmp"
    break;

  case 36: /* simple_exp: simple_exp '+' simple_exp  */
#line 755 "ada-exp.y"
                        { ada_wrap_op<ada_binop_addsub_operation> (BINOP_ADD); }
#line 2325 "ada-exp.c.tmp"
    break;

  case 37: /* simple_exp: simple_exp '&' simple_exp  */
#line 759 "ada-exp.y"
                        { ada_wrap2<ada_concat_operation> (BINOP_CONCAT); }
#line 2331 "ada-exp.c.tmp"
    break;

  case 38: /* simple_exp: simple_exp '-' simple_exp  */
#line 763 "ada-exp.y"
                        { ada_wrap_op<ada_binop_addsub_operation> (BINOP_SUB); }
#line 2337 "ada-exp.c.tmp"
    break;

  case 40: /* relation: simple_exp '=' simple_exp  */
#line 770 "ada-exp.y"
                        { ada_wrap_op<ada_binop_equal_operation> (BINOP_EQUAL); }
#line 2343 "ada-exp.c.tmp"
    break;

  case 41: /* relation: simple_exp NOTEQUAL simple_exp  */
#line 774 "ada-exp.y"
                        { ada_wrap_op<ada_binop_equal_operation> (BINOP_NOTEQUAL); }
#line 2349 "ada-exp.c.tmp"
    break;

  case 42: /* relation: simple_exp LEQ simple_exp  */
#line 778 "ada-exp.y"
                        { ada_un_wrap2<leq_operation> (BINOP_LEQ); }
#line 2355 "ada-exp.c.tmp"
    break;

  case 43: /* relation: simple_exp IN simple_exp DOTDOT simple_exp  */
#line 782 "ada-exp.y"
                        { ada_wrap3<ada_ternop_range_operation> (); }
#line 2361 "ada-exp.c.tmp"
    break;

  case 44: /* relation: simple_exp IN primary TICK_RANGE tick_arglist  */
#line 784 "ada-exp.y"
                        {
			  operation_up rhs = ada_pop ();
			  operation_up lhs = ada_pop ();
			  pstate->push_new<ada_binop_in_bounds_operation>
			    (std::move (lhs), std::move (rhs), (yyvsp[0].lval));
			}
#line 2372 "ada-exp.c.tmp"
    break;

  case 45: /* relation: simple_exp IN var_or_type  */
#line 791 "ada-exp.y"
                        { 
			  if ((yyvsp[0].tval) == NULL)
			    error (_("Right operand of 'in' must be type"));
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_unop_range_operation>
			    (std::move (arg), (yyvsp[0].tval));
			}
#line 2384 "ada-exp.c.tmp"
    break;

  case 46: /* relation: simple_exp NOT IN simple_exp DOTDOT simple_exp  */
#line 799 "ada-exp.y"
                        { ada_wrap3<ada_ternop_range_operation> ();
			  ada_wrap<unary_logical_not_operation> (); }
#line 2391 "ada-exp.c.tmp"
    break;

  case 47: /* relation: simple_exp NOT IN primary TICK_RANGE tick_arglist  */
#line 802 "ada-exp.y"
                        {
			  operation_up rhs = ada_pop ();
			  operation_up lhs = ada_pop ();
			  pstate->push_new<ada_binop_in_bounds_operation>
			    (std::move (lhs), std::move (rhs), (yyvsp[0].lval));
			  ada_wrap<unary_logical_not_operation> ();
			}
#line 2403 "ada-exp.c.tmp"
    break;

  case 48: /* relation: simple_exp NOT IN var_or_type  */
#line 810 "ada-exp.y"
                        { 
			  if ((yyvsp[0].tval) == NULL)
			    error (_("Right operand of 'in' must be type"));
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_unop_range_operation>
			    (std::move (arg), (yyvsp[0].tval));
			  ada_wrap<unary_logical_not_operation> ();
			}
#line 2416 "ada-exp.c.tmp"
    break;

  case 49: /* relation: simple_exp GEQ simple_exp  */
#line 821 "ada-exp.y"
                        { ada_un_wrap2<geq_operation> (BINOP_GEQ); }
#line 2422 "ada-exp.c.tmp"
    break;

  case 50: /* relation: simple_exp '<' simple_exp  */
#line 825 "ada-exp.y"
                        { ada_un_wrap2<less_operation> (BINOP_LESS); }
#line 2428 "ada-exp.c.tmp"
    break;

  case 51: /* relation: simple_exp '>' simple_exp  */
#line 829 "ada-exp.y"
                        { ada_un_wrap2<gtr_operation> (BINOP_GTR); }
#line 2434 "ada-exp.c.tmp"
    break;

  case 58: /* and_exp: relation _AND_ relation  */
#line 842 "ada-exp.y"
                        { ada_wrap2<bitwise_and_operation>
			    (BINOP_BITWISE_AND); }
#line 2441 "ada-exp.c.tmp"
    break;

  case 59: /* and_exp: and_exp _AND_ relation  */
#line 845 "ada-exp.y"
                        { ada_wrap2<bitwise_and_operation>
			    (BINOP_BITWISE_AND); }
#line 2448 "ada-exp.c.tmp"
    break;

  case 60: /* and_then_exp: relation _AND_ THEN relation  */
#line 851 "ada-exp.y"
                        { ada_wrap2<logical_and_operation>
			    (BINOP_LOGICAL_AND); }
#line 2455 "ada-exp.c.tmp"
    break;

  case 61: /* and_then_exp: and_then_exp _AND_ THEN relation  */
#line 854 "ada-exp.y"
                        { ada_wrap2<logical_and_operation>
			    (BINOP_LOGICAL_AND); }
#line 2462 "ada-exp.c.tmp"
    break;

  case 62: /* or_exp: relation OR relation  */
#line 860 "ada-exp.y"
                        { ada_wrap2<bitwise_ior_operation>
			    (BINOP_BITWISE_IOR); }
#line 2469 "ada-exp.c.tmp"
    break;

  case 63: /* or_exp: or_exp OR relation  */
#line 863 "ada-exp.y"
                        { ada_wrap2<bitwise_ior_operation>
			    (BINOP_BITWISE_IOR); }
#line 2476 "ada-exp.c.tmp"
    break;

  case 64: /* or_else_exp: relation OR ELSE relation  */
#line 869 "ada-exp.y"
                        { ada_wrap2<logical_or_operation> (BINOP_LOGICAL_OR); }
#line 2482 "ada-exp.c.tmp"
    break;

  case 65: /* or_else_exp: or_else_exp OR ELSE relation  */
#line 871 "ada-exp.y"
                        { ada_wrap2<logical_or_operation> (BINOP_LOGICAL_OR); }
#line 2488 "ada-exp.c.tmp"
    break;

  case 66: /* xor_exp: relation XOR relation  */
#line 875 "ada-exp.y"
                        { ada_wrap2<bitwise_xor_operation>
			    (BINOP_BITWISE_XOR); }
#line 2495 "ada-exp.c.tmp"
    break;

  case 67: /* xor_exp: xor_exp XOR relation  */
#line 878 "ada-exp.y"
                        { ada_wrap2<bitwise_xor_operation>
			    (BINOP_BITWISE_XOR); }
#line 2502 "ada-exp.c.tmp"
    break;

  case 68: /* primary: primary TICK_ACCESS  */
#line 891 "ada-exp.y"
                        { ada_addrof (); }
#line 2508 "ada-exp.c.tmp"
    break;

  case 69: /* primary: primary TICK_ADDRESS  */
#line 893 "ada-exp.y"
                        { ada_addrof (type_system_address (pstate)); }
#line 2514 "ada-exp.c.tmp"
    break;

  case 70: /* primary: primary TICK_COMPLETE  */
#line 895 "ada-exp.y"
                        {
			  pstate->mark_completion (make_tick_completer ((yyvsp[0].sval)));
			}
#line 2522 "ada-exp.c.tmp"
    break;

  case 71: /* primary: primary TICK_FIRST tick_arglist  */
#line 899 "ada-exp.y"
                        {
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_unop_atr_operation>
			    (std::move (arg), OP_ATR_FIRST, (yyvsp[0].lval));
			}
#line 2532 "ada-exp.c.tmp"
    break;

  case 72: /* primary: primary TICK_LAST tick_arglist  */
#line 905 "ada-exp.y"
                        {
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_unop_atr_operation>
			    (std::move (arg), OP_ATR_LAST, (yyvsp[0].lval));
			}
#line 2542 "ada-exp.c.tmp"
    break;

  case 73: /* primary: primary TICK_LENGTH tick_arglist  */
#line 911 "ada-exp.y"
                        {
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_unop_atr_operation>
			    (std::move (arg), OP_ATR_LENGTH, (yyvsp[0].lval));
			}
#line 2552 "ada-exp.c.tmp"
    break;

  case 74: /* primary: primary TICK_SIZE  */
#line 917 "ada-exp.y"
                        { ada_wrap<ada_atr_size_operation> (); }
#line 2558 "ada-exp.c.tmp"
    break;

  case 75: /* primary: primary TICK_TAG  */
#line 919 "ada-exp.y"
                        { ada_wrap<ada_atr_tag_operation> (); }
#line 2564 "ada-exp.c.tmp"
    break;

  case 76: /* primary: opt_type_prefix TICK_MIN '(' exp ',' exp ')'  */
#line 921 "ada-exp.y"
                        { ada_wrap2<ada_binop_min_operation> (BINOP_MIN); }
#line 2570 "ada-exp.c.tmp"
    break;

  case 77: /* primary: opt_type_prefix TICK_MAX '(' exp ',' exp ')'  */
#line 923 "ada-exp.y"
                        { ada_wrap2<ada_binop_max_operation> (BINOP_MAX); }
#line 2576 "ada-exp.c.tmp"
    break;

  case 78: /* primary: opt_type_prefix TICK_POS '(' exp ')'  */
#line 925 "ada-exp.y"
                        { ada_wrap<ada_pos_operation> (); }
#line 2582 "ada-exp.c.tmp"
    break;

  case 79: /* primary: type_prefix TICK_VAL '(' exp ')'  */
#line 927 "ada-exp.y"
                        {
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_atr_val_operation>
			    ((yyvsp[-4].tval), std::move (arg));
			}
#line 2592 "ada-exp.c.tmp"
    break;

  case 80: /* primary: type_prefix TICK_ENUM_REP '(' exp ')'  */
#line 933 "ada-exp.y"
                        {
			  operation_up arg = ada_pop (true, (yyvsp[-4].tval));
			  pstate->push_new<ada_atr_enum_rep_operation>
			    ((yyvsp[-4].tval), std::move (arg));
			}
#line 2602 "ada-exp.c.tmp"
    break;

  case 81: /* primary: type_prefix TICK_ENUM_VAL '(' exp ')'  */
#line 939 "ada-exp.y"
                        {
			  operation_up arg = ada_pop (true, (yyvsp[-4].tval));
			  pstate->push_new<ada_atr_enum_val_operation>
			    ((yyvsp[-4].tval), std::move (arg));
			}
#line 2612 "ada-exp.c.tmp"
    break;

  case 82: /* primary: type_prefix TICK_MODULUS  */
#line 945 "ada-exp.y"
                        {
			  struct type *type_arg = check_typedef ((yyvsp[-1].tval));
			  if (!ada_is_modular_type (type_arg))
			    error (_("'modulus must be applied to modular type"));
			  write_int (pstate, ada_modulus (type_arg),
				     type_arg->target_type ());
			}
#line 2624 "ada-exp.c.tmp"
    break;

  case 83: /* tick_arglist: %empty  */
#line 955 "ada-exp.y"
                        { (yyval.lval) = 1; }
#line 2630 "ada-exp.c.tmp"
    break;

  case 84: /* tick_arglist: '(' INT ')'  */
#line 957 "ada-exp.y"
                        { (yyval.lval) = (yyvsp[-1].typed_val).val->as_integer<LONGEST> (); }
#line 2636 "ada-exp.c.tmp"
    break;

  case 85: /* type_prefix: var_or_type  */
#line 962 "ada-exp.y"
                        { 
			  if ((yyvsp[0].tval) == NULL)
			    error (_("Prefix must be type"));
			  (yyval.tval) = (yyvsp[0].tval);
			}
#line 2646 "ada-exp.c.tmp"
    break;

  case 86: /* opt_type_prefix: type_prefix  */
#line 971 "ada-exp.y"
                        { (yyval.tval) = (yyvsp[0].tval); }
#line 2652 "ada-exp.c.tmp"
    break;

  case 87: /* opt_type_prefix: %empty  */
#line 973 "ada-exp.y"
                        { (yyval.tval) = parse_type (pstate)->builtin_void; }
#line 2658 "ada-exp.c.tmp"
    break;

  case 88: /* primary: INT  */
#line 978 "ada-exp.y"
                        {
			  pstate->push_new<long_const_operation> ((yyvsp[0].typed_val).type, *(yyvsp[0].typed_val).val);
			  ada_wrap<ada_wrapped_operation> ();
			}
#line 2667 "ada-exp.c.tmp"
    break;

  case 89: /* primary: CHARLIT  */
#line 985 "ada-exp.y"
                        {
			  pstate->push_new<ada_char_operation> ((yyvsp[0].typed_char).type, (yyvsp[0].typed_char).val);
			}
#line 2675 "ada-exp.c.tmp"
    break;

  case 90: /* primary: FLOAT  */
#line 991 "ada-exp.y"
                        {
			  float_data data;
			  std::copy (std::begin ((yyvsp[0].typed_val_float).val), std::end ((yyvsp[0].typed_val_float).val),
				     std::begin (data));
			  pstate->push_new<float_const_operation>
			    ((yyvsp[0].typed_val_float).type, data);
			  ada_wrap<ada_wrapped_operation> ();
			}
#line 2688 "ada-exp.c.tmp"
    break;

  case 91: /* primary: NULL_PTR  */
#line 1002 "ada-exp.y"
                        {
			  struct type *null_ptr_type
			    = lookup_pointer_type (parse_type (pstate)->builtin_int0);
			  write_int (pstate, 0, null_ptr_type);
			}
#line 2698 "ada-exp.c.tmp"
    break;

  case 92: /* primary: STRING  */
#line 1010 "ada-exp.y"
                        { 
			  pstate->push_new<ada_string_operation>
			    (copy_name ((yyvsp[0].sval)));
			}
#line 2707 "ada-exp.c.tmp"
    break;

  case 93: /* primary: TRUEKEYWORD  */
#line 1017 "ada-exp.y"
                        {
			  write_int (pstate, 1,
				     parse_type (pstate)->builtin_bool);
			}
#line 2716 "ada-exp.c.tmp"
    break;

  case 94: /* primary: FALSEKEYWORD  */
#line 1022 "ada-exp.y"
                        {
			  write_int (pstate, 0,
				     parse_type (pstate)->builtin_bool);
			}
#line 2725 "ada-exp.c.tmp"
    break;

  case 95: /* primary: NEW NAME  */
#line 1029 "ada-exp.y"
                        { error (_("NEW not implemented.")); }
#line 2731 "ada-exp.c.tmp"
    break;

  case 96: /* var_or_type: NAME  */
#line 1033 "ada-exp.y"
                                { (yyval.tval) = write_var_or_type (pstate, NULL, (yyvsp[0].sval)); }
#line 2737 "ada-exp.c.tmp"
    break;

  case 97: /* var_or_type: NAME_COMPLETE  */
#line 1035 "ada-exp.y"
                                {
				  (yyval.tval) = write_var_or_type_completion (pstate,
								     NULL,
								     (yyvsp[0].sval));
				}
#line 2747 "ada-exp.c.tmp"
    break;

  case 98: /* var_or_type: block NAME  */
#line 1041 "ada-exp.y"
                                { (yyval.tval) = write_var_or_type (pstate, (yyvsp[-1].bval), (yyvsp[0].sval)); }
#line 2753 "ada-exp.c.tmp"
    break;

  case 99: /* var_or_type: block NAME_COMPLETE  */
#line 1043 "ada-exp.y"
                                {
				  (yyval.tval) = write_var_or_type_completion (pstate,
								     (yyvsp[-1].bval),
								     (yyvsp[0].sval));
				}
#line 2763 "ada-exp.c.tmp"
    break;

  case 100: /* var_or_type: NAME TICK_ACCESS  */
#line 1049 "ada-exp.y"
                        { 
			  (yyval.tval) = write_var_or_type (pstate, NULL, (yyvsp[-1].sval));
			  if ((yyval.tval) == NULL)
			    ada_addrof ();
			  else
			    (yyval.tval) = lookup_pointer_type ((yyval.tval));
			}
#line 2775 "ada-exp.c.tmp"
    break;

  case 101: /* var_or_type: block NAME TICK_ACCESS  */
#line 1057 "ada-exp.y"
                        { 
			  (yyval.tval) = write_var_or_type (pstate, (yyvsp[-2].bval), (yyvsp[-1].sval));
			  if ((yyval.tval) == NULL)
			    ada_addrof ();
			  else
			    (yyval.tval) = lookup_pointer_type ((yyval.tval));
			}
#line 2787 "ada-exp.c.tmp"
    break;

  case 102: /* block: NAME COLONCOLON  */
#line 1068 "ada-exp.y"
                        { (yyval.bval) = block_lookup (NULL, (yyvsp[-1].sval).ptr); }
#line 2793 "ada-exp.c.tmp"
    break;

  case 103: /* block: block NAME COLONCOLON  */
#line 1070 "ada-exp.y"
                        { (yyval.bval) = block_lookup ((yyvsp[-2].bval), (yyvsp[-1].sval).ptr); }
#line 2799 "ada-exp.c.tmp"
    break;

  case 104: /* aggregate: '(' exp WITH DELTA aggregate_component_list ')'  */
#line 1075 "ada-exp.y"
                        {
			  std::vector<ada_component_up> components
			    = pop_components ((yyvsp[-1].lval));
			  operation_up base = ada_pop ();

			  push_component<ada_aggregate_component>
			    (std::move (base), std::move (components));
			}
#line 2812 "ada-exp.c.tmp"
    break;

  case 105: /* aggregate: '(' aggregate_component_list ')'  */
#line 1084 "ada-exp.y"
                        {
			  std::vector<ada_component_up> components
			    = pop_components ((yyvsp[-1].lval));

			  push_component<ada_aggregate_component>
			    (std::move (components));
			}
#line 2824 "ada-exp.c.tmp"
    break;

  case 106: /* aggregate_component_list: component_groups  */
#line 1094 "ada-exp.y"
                                         { (yyval.lval) = (yyvsp[0].lval); }
#line 2830 "ada-exp.c.tmp"
    break;

  case 107: /* aggregate_component_list: positional_list exp  */
#line 1096 "ada-exp.y"
                        {
			  push_component<ada_positional_component>
			    ((yyvsp[-1].lval), ada_pop ());
			  (yyval.lval) = (yyvsp[-1].lval) + 1;
			}
#line 2840 "ada-exp.c.tmp"
    break;

  case 108: /* aggregate_component_list: positional_list component_groups  */
#line 1102 "ada-exp.y"
                                         { (yyval.lval) = (yyvsp[-1].lval) + (yyvsp[0].lval); }
#line 2846 "ada-exp.c.tmp"
    break;

  case 109: /* positional_list: exp ','  */
#line 1107 "ada-exp.y"
                        {
			  push_component<ada_positional_component>
			    (0, ada_pop ());
			  (yyval.lval) = 1;
			}
#line 2856 "ada-exp.c.tmp"
    break;

  case 110: /* positional_list: positional_list exp ','  */
#line 1113 "ada-exp.y"
                        {
			  push_component<ada_positional_component>
			    ((yyvsp[-2].lval), ada_pop ());
			  (yyval.lval) = (yyvsp[-2].lval) + 1; 
			}
#line 2866 "ada-exp.c.tmp"
    break;

  case 111: /* component_groups: others  */
#line 1121 "ada-exp.y"
                                         { (yyval.lval) = 1; }
#line 2872 "ada-exp.c.tmp"
    break;

  case 112: /* component_groups: component_group  */
#line 1122 "ada-exp.y"
                                         { (yyval.lval) = 1; }
#line 2878 "ada-exp.c.tmp"
    break;

  case 113: /* component_groups: component_group ',' component_groups  */
#line 1124 "ada-exp.y"
                                         { (yyval.lval) = (yyvsp[0].lval) + 1; }
#line 2884 "ada-exp.c.tmp"
    break;

  case 114: /* others: OTHERS ARROW exp  */
#line 1128 "ada-exp.y"
                        {
			  push_component<ada_others_component> (ada_pop ());
			}
#line 2892 "ada-exp.c.tmp"
    break;

  case 115: /* component_group: component_associations  */
#line 1135 "ada-exp.y"
                        {
			  ada_choices_component *choices = choice_component ();
			  choices->set_associations (pop_associations ((yyvsp[0].lval)));
			}
#line 2901 "ada-exp.c.tmp"
    break;

  case 116: /* $@2: %empty  */
#line 1140 "ada-exp.y"
                        {
			  std::string name = copy_name ((yyvsp[-1].sval));

			  auto iter = ada_parser->iterated_associations.find (name);
			  if (iter != ada_parser->iterated_associations.end ())
			    error (_("Nested use of index parameter '%s'"),
				   name.c_str ());

			  ada_parser->iterated_associations[name] = {};
			}
#line 2916 "ada-exp.c.tmp"
    break;

  case 117: /* component_group: FOR NAME IN $@2 component_associations  */
#line 1151 "ada-exp.y"
                        {
			  std::string name = copy_name ((yyvsp[-3].sval));

			  ada_choices_component *choices = choice_component ();
			  choices->set_associations (pop_associations ((yyvsp[0].lval)));

			  auto iter = ada_parser->iterated_associations.find (name);
			  gdb_assert (iter != ada_parser->iterated_associations.end ());
			  for (ada_index_var_operation *var : iter->second)
			    var->set_choices (choices);

			  ada_parser->iterated_associations.erase (name);

			  choices->set_name (std::move (name));
			}
#line 2936 "ada-exp.c.tmp"
    break;

  case 118: /* component_associations: NAME ARROW exp  */
#line 1175 "ada-exp.y"
                        {
			  push_component<ada_choices_component> (ada_pop ());
			  write_name_assoc (pstate, (yyvsp[-2].sval));
			  (yyval.lval) = 1;
			}
#line 2946 "ada-exp.c.tmp"
    break;

  case 119: /* component_associations: simple_exp ARROW exp  */
#line 1181 "ada-exp.y"
                        {
			  push_component<ada_choices_component> (ada_pop ());
			  push_association<ada_name_association> (ada_pop ());
			  (yyval.lval) = 1;
			}
#line 2956 "ada-exp.c.tmp"
    break;

  case 120: /* component_associations: simple_exp DOTDOT simple_exp ARROW exp  */
#line 1187 "ada-exp.y"
                        {
			  push_component<ada_choices_component> (ada_pop ());
			  operation_up rhs = ada_pop ();
			  operation_up lhs = ada_pop ();
			  push_association<ada_discrete_range_association>
			    (std::move (lhs), std::move (rhs));
			  (yyval.lval) = 1;
			}
#line 2969 "ada-exp.c.tmp"
    break;

  case 121: /* component_associations: NAME '|' component_associations  */
#line 1196 "ada-exp.y"
                        {
			  write_name_assoc (pstate, (yyvsp[-2].sval));
			  (yyval.lval) = (yyvsp[0].lval) + 1;
			}
#line 2978 "ada-exp.c.tmp"
    break;

  case 122: /* component_associations: simple_exp '|' component_associations  */
#line 1201 "ada-exp.y"
                        {
			  push_association<ada_name_association> (ada_pop ());
			  (yyval.lval) = (yyvsp[0].lval) + 1;
			}
#line 2987 "ada-exp.c.tmp"
    break;

  case 123: /* component_associations: simple_exp DOTDOT simple_exp '|' component_associations  */
#line 1207 "ada-exp.y"
                        {
			  operation_up rhs = ada_pop ();
			  operation_up lhs = ada_pop ();
			  push_association<ada_discrete_range_association>
			    (std::move (lhs), std::move (rhs));
			  (yyval.lval) = (yyvsp[0].lval) + 1;
			}
#line 2999 "ada-exp.c.tmp"
    break;

  case 124: /* primary: '*' primary  */
#line 1220 "ada-exp.y"
                        { ada_wrap<ada_unop_ind_operation> (); }
#line 3005 "ada-exp.c.tmp"
    break;

  case 125: /* primary: '&' primary  */
#line 1222 "ada-exp.y"
                        { ada_addrof (); }
#line 3011 "ada-exp.c.tmp"
    break;

  case 126: /* primary: primary '[' exp ']'  */
#line 1224 "ada-exp.y"
                        {
			  ada_wrap2<subscript_operation> (BINOP_SUBSCRIPT);
			  ada_wrap<ada_wrapped_operation> ();
			}
#line 3020 "ada-exp.c.tmp"
    break;


#line 3024 "ada-exp.c.tmp"

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
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (ada_exp_yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

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

#line 1230 "ada-exp.y"


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

/* The following kludge was found necessary to prevent conflicts between */
/* defs.h and non-standard stdlib.h files.  */
#define qsort __qsort__dummy
#include "ada-lex.c"

int
ada_parse (struct parser_state *par_state)
{
  /* Setting up the parser state.  */
  scoped_restore pstate_restore = make_scoped_restore (&pstate, par_state);
  gdb_assert (par_state != NULL);

  ada_parse_state parser (par_state->lexptr);
  scoped_restore parser_restore = make_scoped_restore (&ada_parser, &parser);

  scoped_restore restore_yydebug = make_scoped_restore (&yydebug,
							par_state->debug);

  lexer_init (yyin);		/* (Re-)initialize lexer.  */

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
  pstate->parse_error (msg);
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

  name = obstack_strndup (&ada_parser->temp_space, renamed_entity,
			  renamed_entity_len);
  ada_lookup_encoded_symbol (name, orig_left_context, SEARCH_VFT, &sym_info);
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
	[[fallthrough]];
      case 'S':
	renaming_expr += 1;
	if (isdigit (*renaming_expr))
	  {
	    char *next;
	    long val = strtol (renaming_expr, &next, 10);
	    if (next == renaming_expr)
	      goto BadEncoding;
	    renaming_expr = next;
	    write_int (par_state, val, parse_type (par_state)->builtin_int);
	  }
	else
	  {
	    const char *end;
	    char *index_name;
	    struct block_symbol index_sym_info;

	    end = strchr (renaming_expr, 'X');
	    if (end == NULL)
	      end = renaming_expr + strlen (renaming_expr);

	    index_name = obstack_strndup (&ada_parser->temp_space,
					  renaming_expr,
					  end - renaming_expr);
	    renaming_expr = end;

	    ada_lookup_encoded_symbol (index_name, orig_left_context,
				       SEARCH_VFT, &index_sym_info);
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
    = ada_lookup_symbol_list (name, context, SEARCH_FUNCTION_DOMAIN);

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
      sym = ada_lookup_symbol (expanded_name, NULL, SEARCH_TYPE_DOMAIN).symbol;
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
  struct symbol *sym = new (&ada_parser->temp_space) symbol ();

  sym->set_domain (UNDEF_DOMAIN);
  sym->set_linkage_name (obstack_strndup (&ada_parser->temp_space, name, len));
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

  std::string name_storage = ada_encode (name0.ptr);

  if (block == nullptr)
    {
      auto iter = ada_parser->iterated_associations.find (name_storage);
      if (iter != ada_parser->iterated_associations.end ())
	{
	  auto op = std::make_unique<ada_index_var_operation> ();
	  iter->second.push_back (op.get ());
	  par_state->push (std::move (op));
	  return nullptr;
	}

      block = par_state->expression_context_block;
    }

  name_len = name_storage.size ();
  encoded_name = obstack_strndup (&ada_parser->temp_space,
				  name_storage.c_str (),
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
	    = ada_lookup_symbol_list (decoded_name.c_str (), block,
				      SEARCH_VFT);

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
		  = (char *) obstack_alloc (&ada_parser->temp_space,
					    alloc_len);
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
		objfile = block->objfile ();

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

std::string
ada_parse_state::find_completion_bounds ()
{
  const char *end = pstate->lexptr;
  /* First the end of the prefix.  Here we stop at the token start or
     at '.' or space.  */
  for (; end > m_original_expr && end[-1] != '.' && !isspace (end[-1]); --end)
    {
      /* Nothing.  */
    }
  /* Now find the start of the prefix.  */
  const char *ptr = end;
  /* Here we allow '.'.  */
  for (;
       ptr > m_original_expr && (ptr[-1] == '.'
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
  op->set_prefix (ada_parser->find_completion_bounds ());
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
				  SEARCH_VFT);

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
type_system_address (struct parser_state *par_state)
{
  struct type *type 
    = language_lookup_primitive_type (par_state->language (),
				      par_state->gdbarch (),
				      "system__address");
  return  type != NULL ? type : parse_type (par_state)->builtin_data_ptr;
}
