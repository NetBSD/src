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
#line 36 "c-exp.y"


#include "defs.h"
#include <ctype.h>
#include "expression.h"
#include "value.h"
#include "parser-defs.h"
#include "language.h"
#include "c-lang.h"
#include "c-support.h"
#include "bfd.h" /* Required by objfiles.h.  */
#include "symfile.h" /* Required by objfiles.h.  */
#include "objfiles.h" /* For have_full_symbols and have_partial_symbols */
#include "charset.h"
#include "block.h"
#include "cp-support.h"
#include "macroscope.h"
#include "objc-lang.h"
#include "typeprint.h"
#include "cp-abi.h"
#include "type-stack.h"
#include "target-float.h"
#include "c-exp.h"

#define parse_type(ps) builtin_type (ps->gdbarch ())

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror,
   etc).  */
#define GDB_YY_REMAP_PREFIX c_
#include "yy-remap.h"

/* The state of the parser, used internally when we are parsing the
   expression.  */

static struct parser_state *pstate = NULL;

/* Data that must be held for the duration of a parse.  */

struct c_parse_state
{
  /* These are used to hold type lists and type stacks that are
     allocated during the parse.  */
  std::vector<std::unique_ptr<std::vector<struct type *>>> type_lists;
  std::vector<std::unique_ptr<struct type_stack>> type_stacks;

  /* Storage for some strings allocated during the parse.  */
  std::vector<gdb::unique_xmalloc_ptr<char>> strings;

  /* When we find that lexptr (the global var defined in parse.c) is
     pointing at a macro invocation, we expand the invocation, and call
     scan_macro_expansion to save the old lexptr here and point lexptr
     into the expanded text.  When we reach the end of that, we call
     end_macro_expansion to pop back to the value we saved here.  The
     macro expansion code promises to return only fully-expanded text,
     so we don't need to "push" more than one level.

     This is disgusting, of course.  It would be cleaner to do all macro
     expansion beforehand, and then hand that to lexptr.  But we don't
     really know where the expression ends.  Remember, in a command like

     (gdb) break *ADDRESS if CONDITION

     we evaluate ADDRESS in the scope of the current frame, but we
     evaluate CONDITION in the scope of the breakpoint's location.  So
     it's simply wrong to try to macro-expand the whole thing at once.  */
  const char *macro_original_text = nullptr;

  /* We save all intermediate macro expansions on this obstack for the
     duration of a single parse.  The expansion text may sometimes have
     to live past the end of the expansion, due to yacc lookahead.
     Rather than try to be clever about saving the data for a single
     token, we simply keep it all and delete it after parsing has
     completed.  */
  auto_obstack expansion_obstack;

  /* The type stack.  */
  struct type_stack type_stack;
};

/* This is set and cleared in c_parse.  */

static struct c_parse_state *cpstate;

int yyparse (void);

static int yylex (void);

static void yyerror (const char *);

static int type_aggregate_p (struct type *);

using namespace expr;

#line 165 "c-exp.c.tmp"

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
    COMPLEX_INT = 259,             /* COMPLEX_INT  */
    FLOAT = 260,                   /* FLOAT  */
    COMPLEX_FLOAT = 261,           /* COMPLEX_FLOAT  */
    STRING = 262,                  /* STRING  */
    NSSTRING = 263,                /* NSSTRING  */
    SELECTOR = 264,                /* SELECTOR  */
    CHAR = 265,                    /* CHAR  */
    NAME = 266,                    /* NAME  */
    UNKNOWN_CPP_NAME = 267,        /* UNKNOWN_CPP_NAME  */
    COMPLETE = 268,                /* COMPLETE  */
    TYPENAME = 269,                /* TYPENAME  */
    CLASSNAME = 270,               /* CLASSNAME  */
    OBJC_LBRAC = 271,              /* OBJC_LBRAC  */
    NAME_OR_INT = 272,             /* NAME_OR_INT  */
    OPERATOR = 273,                /* OPERATOR  */
    STRUCT = 274,                  /* STRUCT  */
    CLASS = 275,                   /* CLASS  */
    UNION = 276,                   /* UNION  */
    ENUM = 277,                    /* ENUM  */
    SIZEOF = 278,                  /* SIZEOF  */
    ALIGNOF = 279,                 /* ALIGNOF  */
    UNSIGNED = 280,                /* UNSIGNED  */
    COLONCOLON = 281,              /* COLONCOLON  */
    TEMPLATE = 282,                /* TEMPLATE  */
    ERROR = 283,                   /* ERROR  */
    NEW = 284,                     /* NEW  */
    DELETE = 285,                  /* DELETE  */
    REINTERPRET_CAST = 286,        /* REINTERPRET_CAST  */
    DYNAMIC_CAST = 287,            /* DYNAMIC_CAST  */
    STATIC_CAST = 288,             /* STATIC_CAST  */
    CONST_CAST = 289,              /* CONST_CAST  */
    ENTRY = 290,                   /* ENTRY  */
    TYPEOF = 291,                  /* TYPEOF  */
    DECLTYPE = 292,                /* DECLTYPE  */
    TYPEID = 293,                  /* TYPEID  */
    SIGNED_KEYWORD = 294,          /* SIGNED_KEYWORD  */
    LONG = 295,                    /* LONG  */
    SHORT = 296,                   /* SHORT  */
    INT_KEYWORD = 297,             /* INT_KEYWORD  */
    CONST_KEYWORD = 298,           /* CONST_KEYWORD  */
    VOLATILE_KEYWORD = 299,        /* VOLATILE_KEYWORD  */
    DOUBLE_KEYWORD = 300,          /* DOUBLE_KEYWORD  */
    RESTRICT = 301,                /* RESTRICT  */
    ATOMIC = 302,                  /* ATOMIC  */
    FLOAT_KEYWORD = 303,           /* FLOAT_KEYWORD  */
    COMPLEX = 304,                 /* COMPLEX  */
    DOLLAR_VARIABLE = 305,         /* DOLLAR_VARIABLE  */
    ASSIGN_MODIFY = 306,           /* ASSIGN_MODIFY  */
    TRUEKEYWORD = 307,             /* TRUEKEYWORD  */
    FALSEKEYWORD = 308,            /* FALSEKEYWORD  */
    ABOVE_COMMA = 309,             /* ABOVE_COMMA  */
    OROR = 310,                    /* OROR  */
    ANDAND = 311,                  /* ANDAND  */
    EQUAL = 312,                   /* EQUAL  */
    NOTEQUAL = 313,                /* NOTEQUAL  */
    LEQ = 314,                     /* LEQ  */
    GEQ = 315,                     /* GEQ  */
    LSH = 316,                     /* LSH  */
    RSH = 317,                     /* RSH  */
    UNARY = 318,                   /* UNARY  */
    INCREMENT = 319,               /* INCREMENT  */
    DECREMENT = 320,               /* DECREMENT  */
    ARROW = 321,                   /* ARROW  */
    ARROW_STAR = 322,              /* ARROW_STAR  */
    DOT_STAR = 323,                /* DOT_STAR  */
    BLOCKNAME = 324,               /* BLOCKNAME  */
    FILENAME = 325,                /* FILENAME  */
    DOTDOTDOT = 326                /* DOTDOTDOT  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define INT 258
#define COMPLEX_INT 259
#define FLOAT 260
#define COMPLEX_FLOAT 261
#define STRING 262
#define NSSTRING 263
#define SELECTOR 264
#define CHAR 265
#define NAME 266
#define UNKNOWN_CPP_NAME 267
#define COMPLETE 268
#define TYPENAME 269
#define CLASSNAME 270
#define OBJC_LBRAC 271
#define NAME_OR_INT 272
#define OPERATOR 273
#define STRUCT 274
#define CLASS 275
#define UNION 276
#define ENUM 277
#define SIZEOF 278
#define ALIGNOF 279
#define UNSIGNED 280
#define COLONCOLON 281
#define TEMPLATE 282
#define ERROR 283
#define NEW 284
#define DELETE 285
#define REINTERPRET_CAST 286
#define DYNAMIC_CAST 287
#define STATIC_CAST 288
#define CONST_CAST 289
#define ENTRY 290
#define TYPEOF 291
#define DECLTYPE 292
#define TYPEID 293
#define SIGNED_KEYWORD 294
#define LONG 295
#define SHORT 296
#define INT_KEYWORD 297
#define CONST_KEYWORD 298
#define VOLATILE_KEYWORD 299
#define DOUBLE_KEYWORD 300
#define RESTRICT 301
#define ATOMIC 302
#define FLOAT_KEYWORD 303
#define COMPLEX 304
#define DOLLAR_VARIABLE 305
#define ASSIGN_MODIFY 306
#define TRUEKEYWORD 307
#define FALSEKEYWORD 308
#define ABOVE_COMMA 309
#define OROR 310
#define ANDAND 311
#define EQUAL 312
#define NOTEQUAL 313
#define LEQ 314
#define GEQ 315
#define LSH 316
#define RSH 317
#define UNARY 318
#define INCREMENT 319
#define DECREMENT 320
#define ARROW 321
#define ARROW_STAR 322
#define DOT_STAR 323
#define BLOCKNAME 324
#define FILENAME 325
#define DOTDOTDOT 326

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 135 "c-exp.y"

    LONGEST lval;
    struct {
      LONGEST val;
      struct type *type;
    } typed_val_int;
    struct {
      gdb_byte val[16];
      struct type *type;
    } typed_val_float;
    struct type *tval;
    struct stoken sval;
    struct typed_stoken tsval;
    struct ttype tsym;
    struct symtoken ssym;
    int voidval;
    const struct block *bval;
    enum exp_opcode opcode;

    struct stoken_vector svec;
    std::vector<struct type *> *tvec;

    struct type_stack *type_stack;

    struct objc_class_str theclass;
  

#line 385 "c-exp.c.tmp"

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
  YYSYMBOL_COMPLEX_INT = 4,                /* COMPLEX_INT  */
  YYSYMBOL_FLOAT = 5,                      /* FLOAT  */
  YYSYMBOL_COMPLEX_FLOAT = 6,              /* COMPLEX_FLOAT  */
  YYSYMBOL_STRING = 7,                     /* STRING  */
  YYSYMBOL_NSSTRING = 8,                   /* NSSTRING  */
  YYSYMBOL_SELECTOR = 9,                   /* SELECTOR  */
  YYSYMBOL_CHAR = 10,                      /* CHAR  */
  YYSYMBOL_NAME = 11,                      /* NAME  */
  YYSYMBOL_UNKNOWN_CPP_NAME = 12,          /* UNKNOWN_CPP_NAME  */
  YYSYMBOL_COMPLETE = 13,                  /* COMPLETE  */
  YYSYMBOL_TYPENAME = 14,                  /* TYPENAME  */
  YYSYMBOL_CLASSNAME = 15,                 /* CLASSNAME  */
  YYSYMBOL_OBJC_LBRAC = 16,                /* OBJC_LBRAC  */
  YYSYMBOL_NAME_OR_INT = 17,               /* NAME_OR_INT  */
  YYSYMBOL_OPERATOR = 18,                  /* OPERATOR  */
  YYSYMBOL_STRUCT = 19,                    /* STRUCT  */
  YYSYMBOL_CLASS = 20,                     /* CLASS  */
  YYSYMBOL_UNION = 21,                     /* UNION  */
  YYSYMBOL_ENUM = 22,                      /* ENUM  */
  YYSYMBOL_SIZEOF = 23,                    /* SIZEOF  */
  YYSYMBOL_ALIGNOF = 24,                   /* ALIGNOF  */
  YYSYMBOL_UNSIGNED = 25,                  /* UNSIGNED  */
  YYSYMBOL_COLONCOLON = 26,                /* COLONCOLON  */
  YYSYMBOL_TEMPLATE = 27,                  /* TEMPLATE  */
  YYSYMBOL_ERROR = 28,                     /* ERROR  */
  YYSYMBOL_NEW = 29,                       /* NEW  */
  YYSYMBOL_DELETE = 30,                    /* DELETE  */
  YYSYMBOL_REINTERPRET_CAST = 31,          /* REINTERPRET_CAST  */
  YYSYMBOL_DYNAMIC_CAST = 32,              /* DYNAMIC_CAST  */
  YYSYMBOL_STATIC_CAST = 33,               /* STATIC_CAST  */
  YYSYMBOL_CONST_CAST = 34,                /* CONST_CAST  */
  YYSYMBOL_ENTRY = 35,                     /* ENTRY  */
  YYSYMBOL_TYPEOF = 36,                    /* TYPEOF  */
  YYSYMBOL_DECLTYPE = 37,                  /* DECLTYPE  */
  YYSYMBOL_TYPEID = 38,                    /* TYPEID  */
  YYSYMBOL_SIGNED_KEYWORD = 39,            /* SIGNED_KEYWORD  */
  YYSYMBOL_LONG = 40,                      /* LONG  */
  YYSYMBOL_SHORT = 41,                     /* SHORT  */
  YYSYMBOL_INT_KEYWORD = 42,               /* INT_KEYWORD  */
  YYSYMBOL_CONST_KEYWORD = 43,             /* CONST_KEYWORD  */
  YYSYMBOL_VOLATILE_KEYWORD = 44,          /* VOLATILE_KEYWORD  */
  YYSYMBOL_DOUBLE_KEYWORD = 45,            /* DOUBLE_KEYWORD  */
  YYSYMBOL_RESTRICT = 46,                  /* RESTRICT  */
  YYSYMBOL_ATOMIC = 47,                    /* ATOMIC  */
  YYSYMBOL_FLOAT_KEYWORD = 48,             /* FLOAT_KEYWORD  */
  YYSYMBOL_COMPLEX = 49,                   /* COMPLEX  */
  YYSYMBOL_DOLLAR_VARIABLE = 50,           /* DOLLAR_VARIABLE  */
  YYSYMBOL_ASSIGN_MODIFY = 51,             /* ASSIGN_MODIFY  */
  YYSYMBOL_TRUEKEYWORD = 52,               /* TRUEKEYWORD  */
  YYSYMBOL_FALSEKEYWORD = 53,              /* FALSEKEYWORD  */
  YYSYMBOL_54_ = 54,                       /* ','  */
  YYSYMBOL_ABOVE_COMMA = 55,               /* ABOVE_COMMA  */
  YYSYMBOL_56_ = 56,                       /* '='  */
  YYSYMBOL_57_ = 57,                       /* '?'  */
  YYSYMBOL_OROR = 58,                      /* OROR  */
  YYSYMBOL_ANDAND = 59,                    /* ANDAND  */
  YYSYMBOL_60_ = 60,                       /* '|'  */
  YYSYMBOL_61_ = 61,                       /* '^'  */
  YYSYMBOL_62_ = 62,                       /* '&'  */
  YYSYMBOL_EQUAL = 63,                     /* EQUAL  */
  YYSYMBOL_NOTEQUAL = 64,                  /* NOTEQUAL  */
  YYSYMBOL_65_ = 65,                       /* '<'  */
  YYSYMBOL_66_ = 66,                       /* '>'  */
  YYSYMBOL_LEQ = 67,                       /* LEQ  */
  YYSYMBOL_GEQ = 68,                       /* GEQ  */
  YYSYMBOL_LSH = 69,                       /* LSH  */
  YYSYMBOL_RSH = 70,                       /* RSH  */
  YYSYMBOL_71_ = 71,                       /* '@'  */
  YYSYMBOL_72_ = 72,                       /* '+'  */
  YYSYMBOL_73_ = 73,                       /* '-'  */
  YYSYMBOL_74_ = 74,                       /* '*'  */
  YYSYMBOL_75_ = 75,                       /* '/'  */
  YYSYMBOL_76_ = 76,                       /* '%'  */
  YYSYMBOL_UNARY = 77,                     /* UNARY  */
  YYSYMBOL_INCREMENT = 78,                 /* INCREMENT  */
  YYSYMBOL_DECREMENT = 79,                 /* DECREMENT  */
  YYSYMBOL_ARROW = 80,                     /* ARROW  */
  YYSYMBOL_ARROW_STAR = 81,                /* ARROW_STAR  */
  YYSYMBOL_82_ = 82,                       /* '.'  */
  YYSYMBOL_DOT_STAR = 83,                  /* DOT_STAR  */
  YYSYMBOL_84_ = 84,                       /* '['  */
  YYSYMBOL_85_ = 85,                       /* '('  */
  YYSYMBOL_BLOCKNAME = 86,                 /* BLOCKNAME  */
  YYSYMBOL_FILENAME = 87,                  /* FILENAME  */
  YYSYMBOL_DOTDOTDOT = 88,                 /* DOTDOTDOT  */
  YYSYMBOL_89_ = 89,                       /* ')'  */
  YYSYMBOL_90_ = 90,                       /* '!'  */
  YYSYMBOL_91_ = 91,                       /* '~'  */
  YYSYMBOL_92_ = 92,                       /* ']'  */
  YYSYMBOL_93_ = 93,                       /* ':'  */
  YYSYMBOL_94_ = 94,                       /* '{'  */
  YYSYMBOL_95_ = 95,                       /* '}'  */
  YYSYMBOL_YYACCEPT = 96,                  /* $accept  */
  YYSYMBOL_start = 97,                     /* start  */
  YYSYMBOL_type_exp = 98,                  /* type_exp  */
  YYSYMBOL_exp1 = 99,                      /* exp1  */
  YYSYMBOL_exp = 100,                      /* exp  */
  YYSYMBOL_101_1 = 101,                    /* $@1  */
  YYSYMBOL_102_2 = 102,                    /* $@2  */
  YYSYMBOL_103_3 = 103,                    /* $@3  */
  YYSYMBOL_msglist = 104,                  /* msglist  */
  YYSYMBOL_msgarglist = 105,               /* msgarglist  */
  YYSYMBOL_msgarg = 106,                   /* msgarg  */
  YYSYMBOL_107_4 = 107,                    /* $@4  */
  YYSYMBOL_108_5 = 108,                    /* $@5  */
  YYSYMBOL_lcurly = 109,                   /* lcurly  */
  YYSYMBOL_arglist = 110,                  /* arglist  */
  YYSYMBOL_function_method = 111,          /* function_method  */
  YYSYMBOL_function_method_void = 112,     /* function_method_void  */
  YYSYMBOL_function_method_void_or_typelist = 113, /* function_method_void_or_typelist  */
  YYSYMBOL_rcurly = 114,                   /* rcurly  */
  YYSYMBOL_string_exp = 115,               /* string_exp  */
  YYSYMBOL_block = 116,                    /* block  */
  YYSYMBOL_variable = 117,                 /* variable  */
  YYSYMBOL_qualified_name = 118,           /* qualified_name  */
  YYSYMBOL_const_or_volatile = 119,        /* const_or_volatile  */
  YYSYMBOL_single_qualifier = 120,         /* single_qualifier  */
  YYSYMBOL_qualifier_seq_noopt = 121,      /* qualifier_seq_noopt  */
  YYSYMBOL_qualifier_seq = 122,            /* qualifier_seq  */
  YYSYMBOL_ptr_operator = 123,             /* ptr_operator  */
  YYSYMBOL_124_6 = 124,                    /* $@6  */
  YYSYMBOL_125_7 = 125,                    /* $@7  */
  YYSYMBOL_ptr_operator_ts = 126,          /* ptr_operator_ts  */
  YYSYMBOL_abs_decl = 127,                 /* abs_decl  */
  YYSYMBOL_direct_abs_decl = 128,          /* direct_abs_decl  */
  YYSYMBOL_array_mod = 129,                /* array_mod  */
  YYSYMBOL_func_mod = 130,                 /* func_mod  */
  YYSYMBOL_type = 131,                     /* type  */
  YYSYMBOL_scalar_type = 132,              /* scalar_type  */
  YYSYMBOL_typebase = 133,                 /* typebase  */
  YYSYMBOL_type_name = 134,                /* type_name  */
  YYSYMBOL_parameter_typelist = 135,       /* parameter_typelist  */
  YYSYMBOL_nonempty_typelist = 136,        /* nonempty_typelist  */
  YYSYMBOL_ptype = 137,                    /* ptype  */
  YYSYMBOL_conversion_type_id = 138,       /* conversion_type_id  */
  YYSYMBOL_conversion_declarator = 139,    /* conversion_declarator  */
  YYSYMBOL_const_and_volatile = 140,       /* const_and_volatile  */
  YYSYMBOL_const_or_volatile_noopt = 141,  /* const_or_volatile_noopt  */
  YYSYMBOL_oper = 142,                     /* oper  */
  YYSYMBOL_field_name = 143,               /* field_name  */
  YYSYMBOL_name = 144,                     /* name  */
  YYSYMBOL_name_not_typename = 145         /* name_not_typename  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;


/* Second part of user prologue.  */
#line 162 "c-exp.y"

/* YYSTYPE gets defined by %union */
static int parse_number (struct parser_state *par_state,
			 const char *, int, int, YYSTYPE *);
static struct stoken operator_stoken (const char *);
static struct stoken typename_stoken (const char *);
static void check_parameter_typelist (std::vector<struct type *> *);

#if defined(YYBISON) && YYBISON < 30800
static void c_print_token (FILE *file, int type, YYSTYPE value);
#define YYPRINT(FILE, TYPE, VALUE) c_print_token (FILE, TYPE, VALUE)
#endif

#line 570 "c-exp.c.tmp"


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
typedef yytype_int16 yy_state_t;

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
#define YYFINAL  177
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1742

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  96
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  50
/* YYNRULES -- Number of rules.  */
#define YYNRULES  285
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  442

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   326


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
       2,     2,     2,    90,     2,     2,     2,    76,    62,     2,
      85,    89,    74,    72,    54,    73,    82,    75,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    93,     2,
      65,    56,    66,    57,    71,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    84,     2,    92,    61,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    94,    60,    95,    91,     2,     2,     2,
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
      45,    46,    47,    48,    49,    50,    51,    52,    53,    55,
      58,    59,    63,    64,    67,    68,    69,    70,    77,    78,
      79,    80,    81,    83,    86,    87,    88
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   280,   280,   281,   284,   288,   292,   296,   303,   304,
     309,   313,   317,   321,   325,   335,   339,   343,   347,   351,
     355,   359,   363,   367,   371,   378,   388,   397,   404,   414,
     422,   426,   438,   448,   457,   464,   474,   482,   486,   490,
     500,   499,   519,   518,   530,   529,   535,   537,   540,   541,
     544,   546,   548,   555,   552,   568,   577,   576,   595,   599,
     602,   606,   610,   625,   635,   642,   643,   646,   653,   656,
     665,   669,   679,   685,   689,   693,   697,   701,   705,   709,
     713,   717,   727,   737,   747,   757,   767,   777,   781,   785,
     789,   805,   821,   838,   848,   857,   864,   877,   886,   897,
     906,   929,   932,   938,   945,   964,   968,   972,   976,   983,
    1000,  1018,  1050,  1060,  1066,  1074,  1082,  1088,  1101,  1114,
    1131,  1142,  1158,  1167,  1168,  1179,  1251,  1252,  1256,  1258,
    1260,  1262,  1264,  1269,  1277,  1278,  1282,  1283,  1288,  1287,
    1291,  1290,  1293,  1295,  1297,  1299,  1303,  1310,  1312,  1313,
    1316,  1318,  1326,  1334,  1341,  1349,  1351,  1353,  1355,  1359,
    1364,  1376,  1383,  1386,  1389,  1392,  1395,  1398,  1401,  1404,
    1407,  1410,  1413,  1416,  1419,  1422,  1425,  1428,  1431,  1434,
    1437,  1440,  1443,  1446,  1449,  1452,  1455,  1458,  1461,  1466,
    1471,  1476,  1479,  1482,  1485,  1501,  1503,  1505,  1509,  1514,
    1520,  1526,  1531,  1537,  1543,  1548,  1554,  1560,  1564,  1569,
    1578,  1583,  1585,  1589,  1590,  1597,  1604,  1614,  1616,  1625,
    1634,  1641,  1642,  1649,  1653,  1654,  1657,  1658,  1661,  1665,
    1667,  1671,  1673,  1675,  1677,  1679,  1681,  1683,  1685,  1687,
    1689,  1691,  1693,  1695,  1697,  1699,  1701,  1703,  1705,  1707,
    1709,  1749,  1751,  1753,  1755,  1757,  1759,  1761,  1763,  1765,
    1767,  1769,  1771,  1773,  1775,  1777,  1779,  1781,  1804,  1805,
    1806,  1807,  1808,  1809,  1810,  1811,  1814,  1815,  1816,  1817,
    1818,  1819,  1822,  1823,  1831,  1844
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
  "\"end of file\"", "error", "\"invalid token\"", "INT", "COMPLEX_INT",
  "FLOAT", "COMPLEX_FLOAT", "STRING", "NSSTRING", "SELECTOR", "CHAR",
  "NAME", "UNKNOWN_CPP_NAME", "COMPLETE", "TYPENAME", "CLASSNAME",
  "OBJC_LBRAC", "NAME_OR_INT", "OPERATOR", "STRUCT", "CLASS", "UNION",
  "ENUM", "SIZEOF", "ALIGNOF", "UNSIGNED", "COLONCOLON", "TEMPLATE",
  "ERROR", "NEW", "DELETE", "REINTERPRET_CAST", "DYNAMIC_CAST",
  "STATIC_CAST", "CONST_CAST", "ENTRY", "TYPEOF", "DECLTYPE", "TYPEID",
  "SIGNED_KEYWORD", "LONG", "SHORT", "INT_KEYWORD", "CONST_KEYWORD",
  "VOLATILE_KEYWORD", "DOUBLE_KEYWORD", "RESTRICT", "ATOMIC",
  "FLOAT_KEYWORD", "COMPLEX", "DOLLAR_VARIABLE", "ASSIGN_MODIFY",
  "TRUEKEYWORD", "FALSEKEYWORD", "','", "ABOVE_COMMA", "'='", "'?'",
  "OROR", "ANDAND", "'|'", "'^'", "'&'", "EQUAL", "NOTEQUAL", "'<'", "'>'",
  "LEQ", "GEQ", "LSH", "RSH", "'@'", "'+'", "'-'", "'*'", "'/'", "'%'",
  "UNARY", "INCREMENT", "DECREMENT", "ARROW", "ARROW_STAR", "'.'",
  "DOT_STAR", "'['", "'('", "BLOCKNAME", "FILENAME", "DOTDOTDOT", "')'",
  "'!'", "'~'", "']'", "':'", "'{'", "'}'", "$accept", "start", "type_exp",
  "exp1", "exp", "$@1", "$@2", "$@3", "msglist", "msgarglist", "msgarg",
  "$@4", "$@5", "lcurly", "arglist", "function_method",
  "function_method_void", "function_method_void_or_typelist", "rcurly",
  "string_exp", "block", "variable", "qualified_name", "const_or_volatile",
  "single_qualifier", "qualifier_seq_noopt", "qualifier_seq",
  "ptr_operator", "$@6", "$@7", "ptr_operator_ts", "abs_decl",
  "direct_abs_decl", "array_mod", "func_mod", "type", "scalar_type",
  "typebase", "type_name", "parameter_typelist", "nonempty_typelist",
  "ptype", "conversion_type_id", "conversion_declarator",
  "const_and_volatile", "const_or_volatile_noopt", "oper", "field_name",
  "name", "name_not_typename", YY_NULLPTRPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-225)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-128)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     408,  -225,  -225,  -225,  -225,  -225,  -225,   -24,  -225,  -225,
     -11,    30,   592,  -225,   855,   241,   270,   278,   288,   684,
       4,    87,    29,   208,    35,    44,    53,    59,    45,    46,
      49,   199,   589,    83,  -225,  -225,  -225,  -225,  -225,  -225,
    -225,   312,  -225,  -225,  -225,   776,    68,   776,   776,   776,
     776,   776,   408,   117,  -225,   776,   776,  -225,   149,  -225,
     101,  1380,   408,   130,  -225,   141,   184,   170,  -225,  -225,
    -225,  1671,  -225,  -225,    73,   201,  -225,   163,   208,  -225,
     147,    30,  -225,  1380,  -225,   118,    -2,    11,  -225,  -225,
    -225,  -225,  -225,  -225,  -225,  -225,  -225,  -225,  -225,  -225,
    -225,  -225,  -225,  -225,  -225,  -225,  -225,  -225,  -225,  -225,
    -225,  -225,  -225,   119,   123,  -225,  -225,   772,  -225,  -225,
    -225,  -225,  -225,  -225,  -225,  -225,   202,  -225,   215,  -225,
     219,  -225,   221,    30,   408,   647,  1635,  -225,     6,   172,
    -225,  -225,  -225,  -225,  -225,   171,  1635,  1635,  1635,  1635,
     500,   776,   408,   150,  -225,  -225,   193,   200,   121,  -225,
    -225,   205,   206,  -225,  -225,   647,  -225,  -225,   647,   647,
     647,   647,   647,   162,   -30,   647,   647,  -225,   776,   776,
     776,   776,   776,   776,   776,   776,   776,   776,   776,   776,
     776,   776,   776,   776,   776,   776,   776,   776,   776,   776,
     776,   776,  -225,  -225,    51,   776,   232,   776,   776,  1022,
     161,  1380,   -29,   208,  -225,   208,  -225,    73,    73,    19,
     -32,   187,  -225,    20,   966,   188,     1,  -225,    13,  -225,
    -225,  -225,   176,   776,   208,   242,    40,    40,    40,  -225,
     175,   177,   178,   186,  -225,  -225,    88,  -225,  -225,  -225,
    -225,  -225,   190,   204,   255,  -225,  -225,  1671,   243,   246,
     247,   249,  1128,   209,  1164,   218,  1200,   266,  -225,  -225,
    -225,   277,   279,  -225,  -225,  -225,   776,  -225,  1380,   -22,
    1380,  1380,   891,   317,  1441,  1475,  1502,  1536,  1563,  1563,
     485,   485,   485,   485,   577,   577,   669,   287,   287,   647,
     647,   647,  -225,    30,  -225,  -225,  -225,  -225,  -225,  -225,
    -225,   208,  -225,   309,  -225,   120,  -225,   208,  -225,   311,
     120,   -20,    34,   776,  -225,   236,   272,  -225,   776,   776,
    -225,  -225,   302,  -225,   237,  -225,   188,   188,    73,   238,
    -225,  -225,   245,   250,  -225,    13,  1058,  -225,  -225,  -225,
      -5,  -225,   208,   776,   776,   239,    40,  -225,   251,   244,
     248,  -225,  -225,  -225,  -225,  -225,  -225,  -225,  -225,  -225,
     280,   253,   258,   262,   263,  -225,  -225,  -225,  -225,  -225,
    -225,  -225,  -225,   647,  -225,   776,   337,  -225,   345,  -225,
    -225,   331,   351,  -225,  -225,  -225,    21,    61,  1094,   647,
    1380,  -225,    73,  -225,  -225,  -225,  -225,    73,  -225,  -225,
    1380,  1380,  -225,  -225,   251,   776,  -225,  -225,  -225,   776,
     776,   776,   776,  1414,  -225,  -225,  -225,  -225,  -225,  -225,
    -225,  -225,  -225,  1380,  1236,  1272,  1308,  1344,  -225,  -225,
    -225,  -225
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       0,    95,    96,    99,   100,   109,   112,     0,    97,   282,
     285,   195,     0,    98,     0,     0,     0,     0,     0,     0,
       0,   192,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   194,   163,   164,   162,   128,   129,   188,   131,   130,
     189,     0,   102,   113,   114,     0,     0,     0,     0,     0,
       0,     0,     0,   283,   116,     0,     0,    58,     0,     3,
       2,     8,    59,    64,    66,     0,   111,     0,   101,   123,
     134,     0,     4,   196,   221,   161,   284,   125,     0,    56,
       0,    40,    42,    44,   195,     0,   231,   232,   250,   261,
     247,   258,   257,   244,   242,   243,   253,   254,   248,   249,
     255,   256,   251,   252,   237,   238,   239,   240,   241,   259,
     260,   263,   262,     0,     0,   246,   245,   224,   267,   276,
     280,   199,   278,   279,   277,   281,   198,   202,   201,   205,
     204,   208,   207,     0,     0,    22,     0,   213,   215,   216,
     214,   191,   285,   283,   124,     0,     0,     0,     0,     0,
       0,     0,     0,   215,   216,   193,   171,   167,   172,   165,
     190,   186,   184,   182,   197,    11,   132,   133,    13,    12,
      10,    16,    17,     0,     0,    14,    15,     1,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    18,    19,     0,     0,     0,     0,     0,    53,
       0,    60,     0,     0,   110,     0,   134,   211,   212,     0,
     144,   142,   140,     0,     0,   146,   148,   222,   149,   152,
     154,   118,     0,    59,     0,   120,     0,     0,     0,   266,
       0,     0,     0,     0,   265,   264,   224,   223,   200,   203,
     206,   209,     0,     0,   178,   169,   185,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   176,   168,   170,
     166,   180,   175,   173,   187,   183,     0,    72,     9,     0,
      94,    93,     0,    91,    90,    89,    88,    87,    81,    82,
      85,    86,    83,    84,    79,    80,    73,    77,    78,    74,
      75,    76,    26,   278,   275,   274,   272,   273,   271,   269,
     270,     0,    29,    24,   268,    30,    33,     0,    36,    31,
      37,     0,    55,    59,   219,     0,   217,    68,     0,     0,
      69,    67,   119,   135,     0,   156,   145,   143,   137,     0,
     155,   159,     0,     0,   138,   147,     0,   151,   153,   103,
       0,   121,     0,     0,     0,     0,    47,    48,    46,     0,
       0,   235,   233,   236,   234,   138,   225,   104,    23,   179,
       0,     0,     0,     0,     0,     5,     6,     7,    21,    20,
     177,   181,   174,    71,    39,     0,    27,    25,    34,    32,
      38,   229,   230,    63,   228,   126,     0,   127,     0,    70,
      61,   158,   136,   141,   157,   150,   160,   137,    57,   122,
      52,    51,    41,    49,     0,     0,    43,    45,   210,     0,
       0,     0,     0,    92,    28,    35,   226,   227,    54,    62,
     218,   220,   139,    50,     0,     0,     0,     0,   105,   107,
     106,   108
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -225,  -225,     5,    15,   -12,  -225,  -225,  -225,  -124,  -225,
      48,  -225,  -225,  -225,  -220,  -225,  -225,  -225,   194,  -225,
    -225,  -225,  -151,     8,   -70,   -72,    14,  -114,  -225,  -225,
    -225,   212,   197,  -224,  -222,  -122,   396,    17,   407,   234,
    -225,  -225,  -225,   213,  -225,  -225,    -7,   256,     3,   441
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    58,   173,   174,    61,   236,   237,   238,   355,   356,
     357,   323,   233,    62,   212,    63,    64,    65,   328,    66,
      67,    68,    69,   393,    70,    71,   403,   225,   407,   338,
     226,   227,   228,   229,   230,    72,    73,    74,   141,   343,
     326,    75,   118,   247,   394,   395,    76,   313,   358,    77
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      83,   216,   218,   246,   347,    59,   348,   135,   125,   125,
     125,   125,   252,   350,   240,    60,   125,   219,   126,   128,
     130,   132,   334,   339,   178,   329,   145,   242,   263,   219,
     221,   117,   178,   165,   178,   168,   169,   170,   171,   172,
       9,   142,   222,   175,   176,   218,   254,    14,   255,   329,
     211,   119,   120,   312,   122,   318,    80,   123,    14,   277,
    -127,    78,   119,   120,   302,   303,   327,   210,   123,    14,
     384,   125,   390,   125,    79,   329,   304,   391,   392,   166,
     167,   232,   241,   235,   408,   223,   224,   324,   217,   136,
     305,   306,   307,   308,   353,   243,   309,   223,   346,   310,
     146,   137,   324,   396,   391,   392,   336,   337,   161,   147,
     428,   335,   340,   359,   360,   143,    35,    36,   148,    38,
      39,   347,   162,   348,   149,   163,   124,   138,   139,   140,
     150,   151,   246,   354,   152,   370,   179,   124,   262,   264,
     266,   253,   311,  -115,    46,   218,   271,   220,   333,   177,
     221,   258,   259,   260,   261,   178,   -65,   265,   119,   120,
     272,   122,   365,   273,   123,    14,   278,   213,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   294,   295,   296,   297,   298,   299,   300,   301,
     267,   214,   268,   315,   279,   320,   215,   125,   231,   125,
     204,   205,   206,   207,   208,   209,   125,   314,   125,   314,
     239,   244,   245,   137,   256,   248,   331,   219,   332,   119,
     120,   211,   122,   321,   324,   123,    14,   125,   249,   125,
     125,   125,   250,   124,   251,   269,   257,   351,   234,   153,
     154,   140,   270,   119,   120,   316,   303,   274,   275,   123,
      14,   276,   119,   120,   121,   122,   327,   304,   123,    14,
     220,   222,   344,   221,   383,   349,   402,   361,   352,   362,
     363,   305,   306,   307,   308,   222,   431,   309,   364,   367,
     310,   119,   120,   127,   122,   223,   224,   123,    14,   119,
     120,   129,   122,   368,   124,   123,    14,   369,   376,   119,
     120,   131,   122,   179,   125,   123,    14,   378,   380,   371,
     125,   211,   372,   373,   386,   374,   399,   400,   124,   381,
     388,   382,   387,   317,   389,   397,   398,   124,  -117,   401,
     404,   412,   333,   179,   405,   402,   416,    21,   419,   406,
     417,   410,   411,   420,   415,   125,   418,   421,   422,   125,
     424,    31,    32,    33,    34,   409,   124,    37,   425,   414,
      40,   199,   200,   201,   124,   202,   203,   204,   205,   206,
     207,   208,   209,   423,   124,   426,   184,   185,   186,   187,
     188,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,   201,   427,   202,   203,   204,   205,   206,
     207,   208,   209,   433,   413,   429,   330,   434,   435,   436,
     437,     1,     2,     3,     4,     5,     6,     7,     8,     9,
      10,   432,    11,   345,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,   342,   164,   155,    24,
      25,    26,    27,   325,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,   366,
      43,    44,   319,   144,     0,     0,     0,     0,     0,     0,
      45,     0,     0,     0,     0,     0,     0,     0,     0,    46,
      47,    48,    49,     0,     0,     0,    50,    51,     0,     0,
       0,     0,     0,    52,    53,    54,     0,     0,    55,    56,
       0,   179,    57,     1,     2,     3,     4,     5,     6,     7,
       8,     9,    10,     0,    11,     0,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,     0,     0,
       0,    24,    25,    26,    27,     0,     0,     0,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,     0,    43,    44,   194,   195,   196,   197,   198,   199,
     200,   201,    45,   202,   203,   204,   205,   206,   207,   208,
     209,    46,    47,    48,    49,     0,     0,     0,    50,    51,
       0,     0,     0,     0,     0,    52,    53,    54,     0,     0,
      55,    56,     0,   179,    57,     1,     2,     3,     4,     5,
       6,     7,     8,     9,    10,     0,    81,    82,    12,    13,
      14,     0,     0,     0,   156,    19,    20,     0,    22,     0,
       0,     0,     0,    24,    25,    26,    27,     0,   157,   158,
      30,   159,     0,     0,   160,     0,     0,     0,     0,     0,
       0,     0,    42,     0,    43,    44,     0,     0,   196,   197,
     198,   199,   200,   201,    45,   202,   203,   204,   205,   206,
     207,   208,   209,   179,    47,    48,    49,     0,     0,     0,
      50,    51,     0,     0,     0,     0,     0,    52,    53,    54,
       0,     0,    55,    56,     0,   179,    57,     1,     2,     3,
       4,     5,     6,     7,     8,     9,    10,     0,   133,     0,
      12,    13,    14,     0,     0,     0,     0,    19,    20,     0,
      22,     0,     0,     0,     0,    24,    25,    26,    27,     0,
       0,     0,    30,     0,     0,   202,   203,   204,   205,   206,
     207,   208,   209,     0,    42,     0,    43,    44,     0,     0,
       0,   197,   198,   199,   200,   201,    45,   202,   203,   204,
     205,   206,   207,   208,   209,     0,    47,    48,    49,     0,
       0,     0,    50,    51,     0,     0,     0,     0,     0,   134,
      53,    54,     0,     0,    55,    56,     0,     0,    57,     1,
       2,     3,     4,     5,     6,     7,     8,     9,    10,     0,
     133,     0,    12,    13,    14,     0,     0,     0,     0,    19,
      20,     0,    22,     0,     0,     0,     0,    24,    25,    26,
      27,     0,     0,     0,    30,    35,    36,     0,    38,    39,
       0,     0,     0,     0,     0,     0,    42,     0,    43,    44,
       0,   220,     0,     0,   221,     0,     0,     0,    45,     0,
       0,     0,     0,    46,     0,     0,   222,     0,    47,    48,
      49,     0,     0,     0,    50,    51,     0,     0,     0,     0,
       0,    52,    53,    54,     0,     0,    55,    56,     0,    84,
      57,    85,     0,     0,    15,    16,    17,    18,     0,     0,
      21,     0,    23,     0,    86,    87,     0,     0,     0,     0,
       0,     0,     0,     0,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,     0,    88,   179,     0,    89,
       0,    90,     0,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,    46,   104,   105,   106,
     107,   108,     0,   109,   110,   111,   112,     0,     0,   113,
     114,     0,   180,     0,     0,   115,   116,   181,   182,   183,
     184,   185,   186,   187,   188,   189,   190,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,   201,     0,   202,
     203,   204,   205,   206,   207,   208,   209,     0,     0,     0,
      84,     0,   219,     0,   385,    15,    16,    17,    18,     0,
       0,    21,     0,    23,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   220,     0,     0,   221,     0,
       0,     0,     0,     0,     0,     0,    84,    46,     0,     0,
     222,    15,    16,    17,    18,     0,     0,    21,     0,    23,
     223,   224,     0,     0,     0,   341,     0,     0,     0,     0,
       0,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    84,     0,     0,     0,     0,    15,    16,    17,
      18,     0,     0,    21,     0,    23,     0,     0,     0,     0,
       0,     0,     0,    46,     0,     0,     0,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    84,     0,
       0,   322,     0,    15,    16,    17,    18,     0,     0,    21,
       0,    23,     0,     0,     0,     0,     0,     0,     0,    46,
       0,     0,     0,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,   179,     0,     0,   341,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    46,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   180,
     179,     0,   430,     0,   181,   182,   183,   184,   185,   186,
     187,   188,   189,   190,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200,   201,     0,   202,   203,   204,   205,
     206,   207,   208,   209,     0,   180,   179,   375,     0,     0,
     181,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,   192,   193,   194,   195,   196,   197,   198,   199,   200,
     201,     0,   202,   203,   204,   205,   206,   207,   208,   209,
       0,   180,   179,   377,     0,     0,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,   201,     0,   202,   203,
     204,   205,   206,   207,   208,   209,     0,   180,   179,   379,
       0,     0,   181,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   191,   192,   193,   194,   195,   196,   197,   198,
     199,   200,   201,     0,   202,   203,   204,   205,   206,   207,
     208,   209,     0,   180,   179,   438,     0,     0,   181,   182,
     183,   184,   185,   186,   187,   188,   189,   190,   191,   192,
     193,   194,   195,   196,   197,   198,   199,   200,   201,     0,
     202,   203,   204,   205,   206,   207,   208,   209,     0,   180,
     179,   439,     0,     0,   181,   182,   183,   184,   185,   186,
     187,   188,   189,   190,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200,   201,     0,   202,   203,   204,   205,
     206,   207,   208,   209,     0,   180,   179,   440,     0,     0,
     181,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,   192,   193,   194,   195,   196,   197,   198,   199,   200,
     201,     0,   202,   203,   204,   205,   206,   207,   208,   209,
     179,   180,     0,   441,     0,     0,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,   201,   179,   202,   203,
     204,   205,   206,   207,   208,   209,     0,     0,     0,     0,
       0,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,   192,   193,   194,   195,   196,   197,   198,   199,   200,
     201,   179,   202,   203,   204,   205,   206,   207,   208,   209,
       0,   185,   186,   187,   188,   189,   190,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,   201,   179,   202,
     203,   204,   205,   206,   207,   208,   209,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   186,   187,   188,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,   201,   179,   202,   203,   204,   205,   206,   207,   208,
     209,     0,     0,     0,   187,   188,   189,   190,   191,   192,
     193,   194,   195,   196,   197,   198,   199,   200,   201,   179,
     202,   203,   204,   205,   206,   207,   208,   209,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   188,
     189,   190,   191,   192,   193,   194,   195,   196,   197,   198,
     199,   200,   201,     0,   202,   203,   204,   205,   206,   207,
     208,   209,     0,     0,     0,     0,     0,     0,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,   201,
       0,   202,   203,   204,   205,   206,   207,   208,   209,    84,
       0,     0,     0,     0,    15,    16,    17,    18,     0,     0,
      21,     0,    23,     0,     0,     0,     0,     0,     0,     0,
       0,    28,    29,     0,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    84,     0,     0,     0,     0,
      15,    16,    17,    18,     0,     0,    21,     0,    23,     0,
       0,     0,     0,     0,     0,     0,    46,     0,     0,     0,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    46
};

static const yytype_int16 yycheck[] =
{
      12,    71,    74,   117,   228,     0,   228,    19,    15,    16,
      17,    18,   134,   233,    16,     0,    23,    16,    15,    16,
      17,    18,     3,     3,    54,    54,    23,    16,   150,    16,
      62,    14,    54,    45,    54,    47,    48,    49,    50,    51,
      11,    12,    74,    55,    56,   117,    40,    18,    42,    54,
      62,    11,    12,   204,    14,   206,    26,    17,    18,    89,
      26,    85,    11,    12,    13,    14,    95,    62,    17,    18,
      92,    78,    92,    80,    85,    54,    25,    43,    44,    11,
      12,    78,    84,    80,    89,    84,    85,   209,    71,    85,
      39,    40,    41,    42,    54,    84,    45,    84,    85,    48,
      65,    14,   224,   323,    43,    44,   220,   221,    25,    65,
      89,    92,    92,   237,   238,    86,    43,    44,    65,    46,
      47,   345,    39,   345,    65,    42,    86,    40,    41,    42,
      85,    85,   246,    93,    85,   257,    16,    86,   150,   151,
     152,   136,    91,    26,    71,   217,    25,    59,   218,     0,
      62,   146,   147,   148,   149,    54,    26,   152,    11,    12,
      39,    14,    74,    42,    17,    18,   178,    26,   180,   181,
     182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,   201,
      40,     7,    42,   205,   179,   207,    26,   204,    35,   206,
      80,    81,    82,    83,    84,    85,   213,   204,   215,   206,
      92,    92,    89,    14,    42,    13,   213,    16,   215,    11,
      12,   233,    14,   208,   346,    17,    18,   234,    13,   236,
     237,   238,    13,    86,    13,    42,    65,   234,    91,    40,
      41,    42,    42,    11,    12,    13,    14,    42,    42,    17,
      18,    89,    11,    12,    13,    14,    95,    25,    17,    18,
      59,    74,    74,    62,   276,    89,   338,    92,    26,    92,
      92,    39,    40,    41,    42,    74,   398,    45,    92,    89,
      48,    11,    12,    13,    14,    84,    85,    17,    18,    11,
      12,    13,    14,    89,    86,    17,    18,    42,    89,    11,
      12,    13,    14,    16,   311,    17,    18,    89,    42,    66,
     317,   323,    66,    66,   311,    66,   328,   329,    86,    42,
     317,    42,    13,    91,    13,    89,    54,    86,    26,    92,
      92,    92,   402,    16,    89,   407,    92,    25,    85,    89,
      92,   353,   354,    85,    93,   352,    66,    85,    85,   356,
      13,    39,    40,    41,    42,   352,    86,    45,    13,   356,
      48,    74,    75,    76,    86,    78,    79,    80,    81,    82,
      83,    84,    85,   385,    86,    44,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    43,    78,    79,    80,    81,    82,
      83,    84,    85,   415,   356,   397,   212,   419,   420,   421,
     422,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,   407,    14,   226,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,   224,    41,    31,    31,
      32,    33,    34,   209,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,   246,
      52,    53,   206,    22,    -1,    -1,    -1,    -1,    -1,    -1,
      62,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    71,
      72,    73,    74,    -1,    -1,    -1,    78,    79,    -1,    -1,
      -1,    -1,    -1,    85,    86,    87,    -1,    -1,    90,    91,
      -1,    16,    94,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    -1,    14,    -1,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    -1,    -1,
      -1,    31,    32,    33,    34,    -1,    -1,    -1,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    -1,    52,    53,    69,    70,    71,    72,    73,    74,
      75,    76,    62,    78,    79,    80,    81,    82,    83,    84,
      85,    71,    72,    73,    74,    -1,    -1,    -1,    78,    79,
      -1,    -1,    -1,    -1,    -1,    85,    86,    87,    -1,    -1,
      90,    91,    -1,    16,    94,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    -1,    14,    15,    16,    17,
      18,    -1,    -1,    -1,    25,    23,    24,    -1,    26,    -1,
      -1,    -1,    -1,    31,    32,    33,    34,    -1,    39,    40,
      38,    42,    -1,    -1,    45,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    50,    -1,    52,    53,    -1,    -1,    71,    72,
      73,    74,    75,    76,    62,    78,    79,    80,    81,    82,
      83,    84,    85,    16,    72,    73,    74,    -1,    -1,    -1,
      78,    79,    -1,    -1,    -1,    -1,    -1,    85,    86,    87,
      -1,    -1,    90,    91,    -1,    16,    94,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    -1,    14,    -1,
      16,    17,    18,    -1,    -1,    -1,    -1,    23,    24,    -1,
      26,    -1,    -1,    -1,    -1,    31,    32,    33,    34,    -1,
      -1,    -1,    38,    -1,    -1,    78,    79,    80,    81,    82,
      83,    84,    85,    -1,    50,    -1,    52,    53,    -1,    -1,
      -1,    72,    73,    74,    75,    76,    62,    78,    79,    80,
      81,    82,    83,    84,    85,    -1,    72,    73,    74,    -1,
      -1,    -1,    78,    79,    -1,    -1,    -1,    -1,    -1,    85,
      86,    87,    -1,    -1,    90,    91,    -1,    -1,    94,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    -1,
      14,    -1,    16,    17,    18,    -1,    -1,    -1,    -1,    23,
      24,    -1,    26,    -1,    -1,    -1,    -1,    31,    32,    33,
      34,    -1,    -1,    -1,    38,    43,    44,    -1,    46,    47,
      -1,    -1,    -1,    -1,    -1,    -1,    50,    -1,    52,    53,
      -1,    59,    -1,    -1,    62,    -1,    -1,    -1,    62,    -1,
      -1,    -1,    -1,    71,    -1,    -1,    74,    -1,    72,    73,
      74,    -1,    -1,    -1,    78,    79,    -1,    -1,    -1,    -1,
      -1,    85,    86,    87,    -1,    -1,    90,    91,    -1,    14,
      94,    16,    -1,    -1,    19,    20,    21,    22,    -1,    -1,
      25,    -1,    27,    -1,    29,    30,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    -1,    51,    16,    -1,    54,
      -1,    56,    -1,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    -1,    78,    79,    80,    81,    -1,    -1,    84,
      85,    -1,    51,    -1,    -1,    90,    91,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    -1,    78,
      79,    80,    81,    82,    83,    84,    85,    -1,    -1,    -1,
      14,    -1,    16,    -1,    93,    19,    20,    21,    22,    -1,
      -1,    25,    -1,    27,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    59,    -1,    -1,    62,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    14,    71,    -1,    -1,
      74,    19,    20,    21,    22,    -1,    -1,    25,    -1,    27,
      84,    85,    -1,    -1,    -1,    89,    -1,    -1,    -1,    -1,
      -1,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    14,    -1,    -1,    -1,    -1,    19,    20,    21,
      22,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    71,    -1,    -1,    -1,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    14,    -1,
      -1,    89,    -1,    19,    20,    21,    22,    -1,    -1,    25,
      -1,    27,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    71,
      -1,    -1,    -1,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    16,    -1,    -1,    89,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    71,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    51,
      16,    -1,    88,    -1,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    -1,    78,    79,    80,    81,
      82,    83,    84,    85,    -1,    51,    16,    89,    -1,    -1,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    -1,    78,    79,    80,    81,    82,    83,    84,    85,
      -1,    51,    16,    89,    -1,    -1,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    -1,    78,    79,
      80,    81,    82,    83,    84,    85,    -1,    51,    16,    89,
      -1,    -1,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    -1,    78,    79,    80,    81,    82,    83,
      84,    85,    -1,    51,    16,    89,    -1,    -1,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    -1,
      78,    79,    80,    81,    82,    83,    84,    85,    -1,    51,
      16,    89,    -1,    -1,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    -1,    78,    79,    80,    81,
      82,    83,    84,    85,    -1,    51,    16,    89,    -1,    -1,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    -1,    78,    79,    80,    81,    82,    83,    84,    85,
      16,    51,    -1,    89,    -1,    -1,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    16,    78,    79,
      80,    81,    82,    83,    84,    85,    -1,    -1,    -1,    -1,
      -1,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    16,    78,    79,    80,    81,    82,    83,    84,    85,
      -1,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    16,    78,
      79,    80,    81,    82,    83,    84,    85,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    16,    78,    79,    80,    81,    82,    83,    84,
      85,    -1,    -1,    -1,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    16,
      78,    79,    80,    81,    82,    83,    84,    85,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    -1,    78,    79,    80,    81,    82,    83,
      84,    85,    -1,    -1,    -1,    -1,    -1,    -1,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      -1,    78,    79,    80,    81,    82,    83,    84,    85,    14,
      -1,    -1,    -1,    -1,    19,    20,    21,    22,    -1,    -1,
      25,    -1,    27,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    36,    37,    -1,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    14,    -1,    -1,    -1,    -1,
      19,    20,    21,    22,    -1,    -1,    25,    -1,    27,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    71,    -1,    -1,    -1,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    71
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    14,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    31,    32,    33,    34,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    52,    53,    62,    71,    72,    73,    74,
      78,    79,    85,    86,    87,    90,    91,    94,    97,    98,
      99,   100,   109,   111,   112,   113,   115,   116,   117,   118,
     120,   121,   131,   132,   133,   137,   142,   145,    85,    85,
      26,    14,    15,   100,    14,    16,    29,    30,    51,    54,
      56,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    72,    73,    74,    75,    76,    78,
      79,    80,    81,    84,    85,    90,    91,   133,   138,    11,
      12,    13,    14,    17,    86,   142,   144,    13,   144,    13,
     144,    13,   144,    14,    85,   100,    85,    14,    40,    41,
      42,   134,    12,    86,   145,   144,    65,    65,    65,    65,
      85,    85,    85,    40,    41,   134,    25,    39,    40,    42,
      45,    25,    39,    42,   132,   100,    11,    12,   100,   100,
     100,   100,   100,    98,    99,   100,   100,     0,    54,    16,
      51,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    78,    79,    80,    81,    82,    83,    84,    85,
      98,   100,   110,    26,     7,    26,   120,   133,   121,    16,
      59,    62,    74,    84,    85,   123,   126,   127,   128,   129,
     130,    35,   144,   108,    91,   144,   101,   102,   103,    92,
      16,    84,    16,    84,    92,    89,   123,   139,    13,    13,
      13,    13,   131,    98,    40,    42,    42,    65,    98,    98,
      98,    98,   100,   131,   100,    98,   100,    40,    42,    42,
      42,    25,    39,    42,    42,    42,    89,    89,   100,    99,
     100,   100,   100,   100,   100,   100,   100,   100,   100,   100,
     100,   100,   100,   100,   100,   100,   100,   100,   100,   100,
     100,   100,    13,    14,    25,    39,    40,    41,    42,    45,
      48,    91,   118,   143,   144,   100,    13,    91,   118,   143,
     100,    99,    89,   107,   131,   135,   136,    95,   114,    54,
     114,   144,   144,   120,     3,    92,   123,   123,   125,     3,
      92,    89,   127,   135,    74,   128,    85,   129,   130,    89,
     110,   144,    26,    54,    93,   104,   105,   106,   144,   104,
     104,    92,    92,    92,    92,    74,   139,    89,    89,    42,
     131,    66,    66,    66,    66,    89,    89,    89,    89,    89,
      42,    42,    42,   100,    92,    93,   144,    13,   144,    13,
      92,    43,    44,   119,   140,   141,   110,    89,    54,   100,
     100,    92,   121,   122,    92,    89,    89,   124,    89,   144,
     100,   100,    92,   106,   144,    93,    92,    92,    66,    85,
      85,    85,    85,   100,    13,    13,    44,    43,    89,   119,
      88,   131,   122,   100,   100,   100,   100,   100,    89,    89,
      89,    89
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,    96,    97,    97,    98,    98,    98,    98,    99,    99,
     100,   100,   100,   100,   100,   100,   100,   100,   100,   100,
     100,   100,   100,   100,   100,   100,   100,   100,   100,   100,
     100,   100,   100,   100,   100,   100,   100,   100,   100,   100,
     101,   100,   102,   100,   103,   100,   104,   104,   105,   105,
     106,   106,   106,   107,   100,   100,   108,   100,   109,   110,
     110,   110,   111,   112,   100,   113,   113,   100,   114,   100,
     100,   100,   100,   100,   100,   100,   100,   100,   100,   100,
     100,   100,   100,   100,   100,   100,   100,   100,   100,   100,
     100,   100,   100,   100,   100,   100,   100,   100,   100,   100,
     100,   100,   100,   100,   100,   100,   100,   100,   100,   115,
     115,   100,   100,   100,   100,   116,   116,   116,   117,   117,
     118,   118,   118,   117,   117,   117,   119,   119,   120,   120,
     120,   120,   120,   120,   121,   121,   122,   122,   124,   123,
     125,   123,   123,   123,   123,   123,   126,   127,   127,   127,
     128,   128,   128,   128,   128,   129,   129,   129,   129,   130,
     130,   131,   132,   132,   132,   132,   132,   132,   132,   132,
     132,   132,   132,   132,   132,   132,   132,   132,   132,   132,
     132,   132,   132,   132,   132,   132,   132,   132,   132,   132,
     132,   132,   132,   132,   132,   133,   133,   133,   133,   133,
     133,   133,   133,   133,   133,   133,   133,   133,   133,   133,
     133,   133,   133,   134,   134,   134,   134,   135,   135,   136,
     136,   137,   137,   138,   139,   139,   140,   140,   141,   141,
     141,   142,   142,   142,   142,   142,   142,   142,   142,   142,
     142,   142,   142,   142,   142,   142,   142,   142,   142,   142,
     142,   142,   142,   142,   142,   142,   142,   142,   142,   142,
     142,   142,   142,   142,   142,   142,   142,   142,   143,   143,
     143,   143,   143,   143,   143,   143,   144,   144,   144,   144,
     144,   144,   145,   145,   145,   145
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     1,     4,     4,     4,     1,     3,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       4,     4,     2,     4,     3,     4,     3,     4,     5,     3,
       3,     3,     4,     3,     4,     5,     3,     3,     4,     4,
       0,     5,     0,     5,     0,     5,     1,     1,     1,     2,
       3,     2,     2,     0,     5,     3,     0,     5,     1,     0,
       1,     3,     5,     4,     1,     1,     1,     3,     1,     3,
       4,     4,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     5,     3,     3,     1,     1,     1,     1,     1,
       1,     1,     1,     4,     4,     7,     7,     7,     7,     1,
       2,     1,     1,     1,     1,     1,     1,     3,     2,     3,
       3,     4,     5,     1,     2,     1,     1,     0,     1,     1,
       1,     1,     2,     2,     1,     2,     1,     0,     0,     4,
       0,     3,     1,     2,     1,     2,     1,     2,     1,     1,
       3,     2,     1,     2,     1,     2,     2,     3,     3,     2,
       3,     1,     1,     1,     1,     2,     3,     2,     3,     3,
       3,     2,     2,     3,     4,     3,     3,     4,     3,     4,
       3,     4,     2,     3,     2,     3,     2,     3,     1,     1,
       2,     2,     1,     2,     1,     1,     1,     2,     2,     2,
       3,     2,     2,     3,     2,     2,     3,     2,     2,     3,
       5,     2,     2,     1,     1,     1,     1,     1,     3,     1,
       3,     1,     2,     2,     0,     2,     2,     2,     1,     1,
       1,     2,     2,     4,     4,     4,     4,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     3,     3,     3,     2,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1
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
#line 285 "c-exp.y"
                        {
			  pstate->push_new<type_operation> ((yyvsp[0].tval));
			}
#line 2117 "c-exp.c.tmp"
    break;

  case 5: /* type_exp: TYPEOF '(' exp ')'  */
#line 289 "c-exp.y"
                        {
			  pstate->wrap<typeof_operation> ();
			}
#line 2125 "c-exp.c.tmp"
    break;

  case 6: /* type_exp: TYPEOF '(' type ')'  */
#line 293 "c-exp.y"
                        {
			  pstate->push_new<type_operation> ((yyvsp[-1].tval));
			}
#line 2133 "c-exp.c.tmp"
    break;

  case 7: /* type_exp: DECLTYPE '(' exp ')'  */
#line 297 "c-exp.y"
                        {
			  pstate->wrap<decltype_operation> ();
			}
#line 2141 "c-exp.c.tmp"
    break;

  case 9: /* exp1: exp1 ',' exp  */
#line 305 "c-exp.y"
                        { pstate->wrap2<comma_operation> (); }
#line 2147 "c-exp.c.tmp"
    break;

  case 10: /* exp: '*' exp  */
#line 310 "c-exp.y"
                        { pstate->wrap<unop_ind_operation> (); }
#line 2153 "c-exp.c.tmp"
    break;

  case 11: /* exp: '&' exp  */
#line 314 "c-exp.y"
                        { pstate->wrap<unop_addr_operation> (); }
#line 2159 "c-exp.c.tmp"
    break;

  case 12: /* exp: '-' exp  */
#line 318 "c-exp.y"
                        { pstate->wrap<unary_neg_operation> (); }
#line 2165 "c-exp.c.tmp"
    break;

  case 13: /* exp: '+' exp  */
#line 322 "c-exp.y"
                        { pstate->wrap<unary_plus_operation> (); }
#line 2171 "c-exp.c.tmp"
    break;

  case 14: /* exp: '!' exp  */
#line 326 "c-exp.y"
                        {
			  if (pstate->language ()->la_language
			      == language_opencl)
			    pstate->wrap<opencl_not_operation> ();
			  else
			    pstate->wrap<unary_logical_not_operation> ();
			}
#line 2183 "c-exp.c.tmp"
    break;

  case 15: /* exp: '~' exp  */
#line 336 "c-exp.y"
                        { pstate->wrap<unary_complement_operation> (); }
#line 2189 "c-exp.c.tmp"
    break;

  case 16: /* exp: INCREMENT exp  */
#line 340 "c-exp.y"
                        { pstate->wrap<preinc_operation> (); }
#line 2195 "c-exp.c.tmp"
    break;

  case 17: /* exp: DECREMENT exp  */
#line 344 "c-exp.y"
                        { pstate->wrap<predec_operation> (); }
#line 2201 "c-exp.c.tmp"
    break;

  case 18: /* exp: exp INCREMENT  */
#line 348 "c-exp.y"
                        { pstate->wrap<postinc_operation> (); }
#line 2207 "c-exp.c.tmp"
    break;

  case 19: /* exp: exp DECREMENT  */
#line 352 "c-exp.y"
                        { pstate->wrap<postdec_operation> (); }
#line 2213 "c-exp.c.tmp"
    break;

  case 20: /* exp: TYPEID '(' exp ')'  */
#line 356 "c-exp.y"
                        { pstate->wrap<typeid_operation> (); }
#line 2219 "c-exp.c.tmp"
    break;

  case 21: /* exp: TYPEID '(' type_exp ')'  */
#line 360 "c-exp.y"
                        { pstate->wrap<typeid_operation> (); }
#line 2225 "c-exp.c.tmp"
    break;

  case 22: /* exp: SIZEOF exp  */
#line 364 "c-exp.y"
                        { pstate->wrap<unop_sizeof_operation> (); }
#line 2231 "c-exp.c.tmp"
    break;

  case 23: /* exp: ALIGNOF '(' type_exp ')'  */
#line 368 "c-exp.y"
                        { pstate->wrap<unop_alignof_operation> (); }
#line 2237 "c-exp.c.tmp"
    break;

  case 24: /* exp: exp ARROW field_name  */
#line 372 "c-exp.y"
                        {
			  pstate->push_new<structop_ptr_operation>
			    (pstate->pop (), copy_name ((yyvsp[0].sval)));
			}
#line 2246 "c-exp.c.tmp"
    break;

  case 25: /* exp: exp ARROW field_name COMPLETE  */
#line 379 "c-exp.y"
                        {
			  structop_base_operation *op
			    = new structop_ptr_operation (pstate->pop (),
							  copy_name ((yyvsp[-1].sval)));
			  pstate->mark_struct_expression (op);
			  pstate->push (operation_up (op));
			}
#line 2258 "c-exp.c.tmp"
    break;

  case 26: /* exp: exp ARROW COMPLETE  */
#line 389 "c-exp.y"
                        {
			  structop_base_operation *op
			    = new structop_ptr_operation (pstate->pop (), "");
			  pstate->mark_struct_expression (op);
			  pstate->push (operation_up (op));
			}
#line 2269 "c-exp.c.tmp"
    break;

  case 27: /* exp: exp ARROW '~' name  */
#line 398 "c-exp.y"
                        {
			  pstate->push_new<structop_ptr_operation>
			    (pstate->pop (), "~" + copy_name ((yyvsp[0].sval)));
			}
#line 2278 "c-exp.c.tmp"
    break;

  case 28: /* exp: exp ARROW '~' name COMPLETE  */
#line 405 "c-exp.y"
                        {
			  structop_base_operation *op
			    = new structop_ptr_operation (pstate->pop (),
							  "~" + copy_name ((yyvsp[-1].sval)));
			  pstate->mark_struct_expression (op);
			  pstate->push (operation_up (op));
			}
#line 2290 "c-exp.c.tmp"
    break;

  case 29: /* exp: exp ARROW qualified_name  */
#line 415 "c-exp.y"
                        { /* exp->type::name becomes exp->*(&type::name) */
			  /* Note: this doesn't work if name is a
			     static member!  FIXME */
			  pstate->wrap<unop_addr_operation> ();
			  pstate->wrap2<structop_mptr_operation> (); }
#line 2300 "c-exp.c.tmp"
    break;

  case 30: /* exp: exp ARROW_STAR exp  */
#line 423 "c-exp.y"
                        { pstate->wrap2<structop_mptr_operation> (); }
#line 2306 "c-exp.c.tmp"
    break;

  case 31: /* exp: exp '.' field_name  */
#line 427 "c-exp.y"
                        {
			  if (pstate->language ()->la_language
			      == language_opencl)
			    pstate->push_new<opencl_structop_operation>
			      (pstate->pop (), copy_name ((yyvsp[0].sval)));
			  else
			    pstate->push_new<structop_operation>
			      (pstate->pop (), copy_name ((yyvsp[0].sval)));
			}
#line 2320 "c-exp.c.tmp"
    break;

  case 32: /* exp: exp '.' field_name COMPLETE  */
#line 439 "c-exp.y"
                        {
			  structop_base_operation *op
			    = new structop_operation (pstate->pop (),
						      copy_name ((yyvsp[-1].sval)));
			  pstate->mark_struct_expression (op);
			  pstate->push (operation_up (op));
			}
#line 2332 "c-exp.c.tmp"
    break;

  case 33: /* exp: exp '.' COMPLETE  */
#line 449 "c-exp.y"
                        {
			  structop_base_operation *op
			    = new structop_operation (pstate->pop (), "");
			  pstate->mark_struct_expression (op);
			  pstate->push (operation_up (op));
			}
#line 2343 "c-exp.c.tmp"
    break;

  case 34: /* exp: exp '.' '~' name  */
#line 458 "c-exp.y"
                        {
			  pstate->push_new<structop_operation>
			    (pstate->pop (), "~" + copy_name ((yyvsp[0].sval)));
			}
#line 2352 "c-exp.c.tmp"
    break;

  case 35: /* exp: exp '.' '~' name COMPLETE  */
#line 465 "c-exp.y"
                        {
			  structop_base_operation *op
			    = new structop_operation (pstate->pop (),
						      "~" + copy_name ((yyvsp[-1].sval)));
			  pstate->mark_struct_expression (op);
			  pstate->push (operation_up (op));
			}
#line 2364 "c-exp.c.tmp"
    break;

  case 36: /* exp: exp '.' qualified_name  */
#line 475 "c-exp.y"
                        { /* exp.type::name becomes exp.*(&type::name) */
			  /* Note: this doesn't work if name is a
			     static member!  FIXME */
			  pstate->wrap<unop_addr_operation> ();
			  pstate->wrap2<structop_member_operation> (); }
#line 2374 "c-exp.c.tmp"
    break;

  case 37: /* exp: exp DOT_STAR exp  */
#line 483 "c-exp.y"
                        { pstate->wrap2<structop_member_operation> (); }
#line 2380 "c-exp.c.tmp"
    break;

  case 38: /* exp: exp '[' exp1 ']'  */
#line 487 "c-exp.y"
                        { pstate->wrap2<subscript_operation> (); }
#line 2386 "c-exp.c.tmp"
    break;

  case 39: /* exp: exp OBJC_LBRAC exp1 ']'  */
#line 491 "c-exp.y"
                        { pstate->wrap2<subscript_operation> (); }
#line 2392 "c-exp.c.tmp"
    break;

  case 40: /* $@1: %empty  */
#line 500 "c-exp.y"
                        {
			  CORE_ADDR theclass;

			  std::string copy = copy_name ((yyvsp[0].tsym).stoken);
			  theclass = lookup_objc_class (pstate->gdbarch (),
							copy.c_str ());
			  if (theclass == 0)
			    error (_("%s is not an ObjC Class"),
				   copy.c_str ());
			  pstate->push_new<long_const_operation>
			    (parse_type (pstate)->builtin_int,
			     (LONGEST) theclass);
			  start_msglist();
			}
#line 2411 "c-exp.c.tmp"
    break;

  case 41: /* exp: OBJC_LBRAC TYPENAME $@1 msglist ']'  */
#line 515 "c-exp.y"
                        { end_msglist (pstate); }
#line 2417 "c-exp.c.tmp"
    break;

  case 42: /* $@2: %empty  */
#line 519 "c-exp.y"
                        {
			  pstate->push_new<long_const_operation>
			    (parse_type (pstate)->builtin_int,
			     (LONGEST) (yyvsp[0].theclass).theclass);
			  start_msglist();
			}
#line 2428 "c-exp.c.tmp"
    break;

  case 43: /* exp: OBJC_LBRAC CLASSNAME $@2 msglist ']'  */
#line 526 "c-exp.y"
                        { end_msglist (pstate); }
#line 2434 "c-exp.c.tmp"
    break;

  case 44: /* $@3: %empty  */
#line 530 "c-exp.y"
                        { start_msglist(); }
#line 2440 "c-exp.c.tmp"
    break;

  case 45: /* exp: OBJC_LBRAC exp $@3 msglist ']'  */
#line 532 "c-exp.y"
                        { end_msglist (pstate); }
#line 2446 "c-exp.c.tmp"
    break;

  case 46: /* msglist: name  */
#line 536 "c-exp.y"
                        { add_msglist(&(yyvsp[0].sval), 0); }
#line 2452 "c-exp.c.tmp"
    break;

  case 50: /* msgarg: name ':' exp  */
#line 545 "c-exp.y"
                        { add_msglist(&(yyvsp[-2].sval), 1); }
#line 2458 "c-exp.c.tmp"
    break;

  case 51: /* msgarg: ':' exp  */
#line 547 "c-exp.y"
                        { add_msglist(0, 1);   }
#line 2464 "c-exp.c.tmp"
    break;

  case 52: /* msgarg: ',' exp  */
#line 549 "c-exp.y"
                        { add_msglist(0, 0);   }
#line 2470 "c-exp.c.tmp"
    break;

  case 53: /* $@4: %empty  */
#line 555 "c-exp.y"
                        { pstate->start_arglist (); }
#line 2476 "c-exp.c.tmp"
    break;

  case 54: /* exp: exp '(' $@4 arglist ')'  */
#line 557 "c-exp.y"
                        {
			  std::vector<operation_up> args
			    = pstate->pop_vector (pstate->end_arglist ());
			  pstate->push_new<funcall_operation>
			    (pstate->pop (), std::move (args));
			}
#line 2487 "c-exp.c.tmp"
    break;

  case 55: /* exp: exp '(' ')'  */
#line 569 "c-exp.y"
                        {
			  pstate->push_new<funcall_operation>
			    (pstate->pop (), std::vector<operation_up> ());
			}
#line 2496 "c-exp.c.tmp"
    break;

  case 56: /* $@5: %empty  */
#line 577 "c-exp.y"
                        {
			  /* This could potentially be a an argument defined
			     lookup function (Koenig).  */
			  /* This is to save the value of arglist_len
			     being accumulated by an outer function call.  */
			  pstate->start_arglist ();
			}
#line 2508 "c-exp.c.tmp"
    break;

  case 57: /* exp: UNKNOWN_CPP_NAME '(' $@5 arglist ')'  */
#line 585 "c-exp.y"
                        {
			  std::vector<operation_up> args
			    = pstate->pop_vector (pstate->end_arglist ());
			  pstate->push_new<adl_func_operation>
			    (copy_name ((yyvsp[-4].ssym).stoken),
			     pstate->expression_context_block,
			     std::move (args));
			}
#line 2521 "c-exp.c.tmp"
    break;

  case 58: /* lcurly: '{'  */
#line 596 "c-exp.y"
                        { pstate->start_arglist (); }
#line 2527 "c-exp.c.tmp"
    break;

  case 60: /* arglist: exp  */
#line 603 "c-exp.y"
                        { pstate->arglist_len = 1; }
#line 2533 "c-exp.c.tmp"
    break;

  case 61: /* arglist: arglist ',' exp  */
#line 607 "c-exp.y"
                        { pstate->arglist_len++; }
#line 2539 "c-exp.c.tmp"
    break;

  case 62: /* function_method: exp '(' parameter_typelist ')' const_or_volatile  */
#line 611 "c-exp.y"
                        {
			  std::vector<struct type *> *type_list = (yyvsp[-2].tvec);
			  /* Save the const/volatile qualifiers as
			     recorded by the const_or_volatile
			     production's actions.  */
			  type_instance_flags flags
			    = (cpstate->type_stack
			       .follow_type_instance_flags ());
			  pstate->push_new<type_instance_operation>
			    (flags, std::move (*type_list),
			     pstate->pop ());
			}
#line 2556 "c-exp.c.tmp"
    break;

  case 63: /* function_method_void: exp '(' ')' const_or_volatile  */
#line 626 "c-exp.y"
                       {
			  type_instance_flags flags
			    = (cpstate->type_stack
			       .follow_type_instance_flags ());
			  pstate->push_new<type_instance_operation>
			    (flags, std::vector<type *> (), pstate->pop ());
		       }
#line 2568 "c-exp.c.tmp"
    break;

  case 67: /* exp: function_method_void_or_typelist COLONCOLON name  */
#line 647 "c-exp.y"
                        {
			  pstate->push_new<func_static_var_operation>
			    (pstate->pop (), copy_name ((yyvsp[0].sval)));
			}
#line 2577 "c-exp.c.tmp"
    break;

  case 68: /* rcurly: '}'  */
#line 654 "c-exp.y"
                        { (yyval.lval) = pstate->end_arglist () - 1; }
#line 2583 "c-exp.c.tmp"
    break;

  case 69: /* exp: lcurly arglist rcurly  */
#line 657 "c-exp.y"
                        {
			  std::vector<operation_up> args
			    = pstate->pop_vector ((yyvsp[0].lval) + 1);
			  pstate->push_new<array_operation> (0, (yyvsp[0].lval),
							     std::move (args));
			}
#line 2594 "c-exp.c.tmp"
    break;

  case 70: /* exp: lcurly type_exp rcurly exp  */
#line 666 "c-exp.y"
                        { pstate->wrap2<unop_memval_type_operation> (); }
#line 2600 "c-exp.c.tmp"
    break;

  case 71: /* exp: '(' type_exp ')' exp  */
#line 670 "c-exp.y"
                        {
			  if (pstate->language ()->la_language
			      == language_opencl)
			    pstate->wrap2<opencl_cast_type_operation> ();
			  else
			    pstate->wrap2<unop_cast_type_operation> ();
			}
#line 2612 "c-exp.c.tmp"
    break;

  case 72: /* exp: '(' exp1 ')'  */
#line 680 "c-exp.y"
                        { }
#line 2618 "c-exp.c.tmp"
    break;

  case 73: /* exp: exp '@' exp  */
#line 686 "c-exp.y"
                        { pstate->wrap2<repeat_operation> (); }
#line 2624 "c-exp.c.tmp"
    break;

  case 74: /* exp: exp '*' exp  */
#line 690 "c-exp.y"
                        { pstate->wrap2<mul_operation> (); }
#line 2630 "c-exp.c.tmp"
    break;

  case 75: /* exp: exp '/' exp  */
#line 694 "c-exp.y"
                        { pstate->wrap2<div_operation> (); }
#line 2636 "c-exp.c.tmp"
    break;

  case 76: /* exp: exp '%' exp  */
#line 698 "c-exp.y"
                        { pstate->wrap2<rem_operation> (); }
#line 2642 "c-exp.c.tmp"
    break;

  case 77: /* exp: exp '+' exp  */
#line 702 "c-exp.y"
                        { pstate->wrap2<add_operation> (); }
#line 2648 "c-exp.c.tmp"
    break;

  case 78: /* exp: exp '-' exp  */
#line 706 "c-exp.y"
                        { pstate->wrap2<sub_operation> (); }
#line 2654 "c-exp.c.tmp"
    break;

  case 79: /* exp: exp LSH exp  */
#line 710 "c-exp.y"
                        { pstate->wrap2<lsh_operation> (); }
#line 2660 "c-exp.c.tmp"
    break;

  case 80: /* exp: exp RSH exp  */
#line 714 "c-exp.y"
                        { pstate->wrap2<rsh_operation> (); }
#line 2666 "c-exp.c.tmp"
    break;

  case 81: /* exp: exp EQUAL exp  */
#line 718 "c-exp.y"
                        {
			  if (pstate->language ()->la_language
			      == language_opencl)
			    pstate->wrap2<opencl_equal_operation> ();
			  else
			    pstate->wrap2<equal_operation> ();
			}
#line 2678 "c-exp.c.tmp"
    break;

  case 82: /* exp: exp NOTEQUAL exp  */
#line 728 "c-exp.y"
                        {
			  if (pstate->language ()->la_language
			      == language_opencl)
			    pstate->wrap2<opencl_notequal_operation> ();
			  else
			    pstate->wrap2<notequal_operation> ();
			}
#line 2690 "c-exp.c.tmp"
    break;

  case 83: /* exp: exp LEQ exp  */
#line 738 "c-exp.y"
                        {
			  if (pstate->language ()->la_language
			      == language_opencl)
			    pstate->wrap2<opencl_leq_operation> ();
			  else
			    pstate->wrap2<leq_operation> ();
			}
#line 2702 "c-exp.c.tmp"
    break;

  case 84: /* exp: exp GEQ exp  */
#line 748 "c-exp.y"
                        {
			  if (pstate->language ()->la_language
			      == language_opencl)
			    pstate->wrap2<opencl_geq_operation> ();
			  else
			    pstate->wrap2<geq_operation> ();
			}
#line 2714 "c-exp.c.tmp"
    break;

  case 85: /* exp: exp '<' exp  */
#line 758 "c-exp.y"
                        {
			  if (pstate->language ()->la_language
			      == language_opencl)
			    pstate->wrap2<opencl_less_operation> ();
			  else
			    pstate->wrap2<less_operation> ();
			}
#line 2726 "c-exp.c.tmp"
    break;

  case 86: /* exp: exp '>' exp  */
#line 768 "c-exp.y"
                        {
			  if (pstate->language ()->la_language
			      == language_opencl)
			    pstate->wrap2<opencl_gtr_operation> ();
			  else
			    pstate->wrap2<gtr_operation> ();
			}
#line 2738 "c-exp.c.tmp"
    break;

  case 87: /* exp: exp '&' exp  */
#line 778 "c-exp.y"
                        { pstate->wrap2<bitwise_and_operation> (); }
#line 2744 "c-exp.c.tmp"
    break;

  case 88: /* exp: exp '^' exp  */
#line 782 "c-exp.y"
                        { pstate->wrap2<bitwise_xor_operation> (); }
#line 2750 "c-exp.c.tmp"
    break;

  case 89: /* exp: exp '|' exp  */
#line 786 "c-exp.y"
                        { pstate->wrap2<bitwise_ior_operation> (); }
#line 2756 "c-exp.c.tmp"
    break;

  case 90: /* exp: exp ANDAND exp  */
#line 790 "c-exp.y"
                        {
			  if (pstate->language ()->la_language
			      == language_opencl)
			    {
			      operation_up rhs = pstate->pop ();
			      operation_up lhs = pstate->pop ();
			      pstate->push_new<opencl_logical_binop_operation>
				(BINOP_LOGICAL_AND, std::move (lhs),
				 std::move (rhs));
			    }
			  else
			    pstate->wrap2<logical_and_operation> ();
			}
#line 2774 "c-exp.c.tmp"
    break;

  case 91: /* exp: exp OROR exp  */
#line 806 "c-exp.y"
                        {
			  if (pstate->language ()->la_language
			      == language_opencl)
			    {
			      operation_up rhs = pstate->pop ();
			      operation_up lhs = pstate->pop ();
			      pstate->push_new<opencl_logical_binop_operation>
				(BINOP_LOGICAL_OR, std::move (lhs),
				 std::move (rhs));
			    }
			  else
			    pstate->wrap2<logical_or_operation> ();
			}
#line 2792 "c-exp.c.tmp"
    break;

  case 92: /* exp: exp '?' exp ':' exp  */
#line 822 "c-exp.y"
                        {
			  operation_up last = pstate->pop ();
			  operation_up mid = pstate->pop ();
			  operation_up first = pstate->pop ();
			  if (pstate->language ()->la_language
			      == language_opencl)
			    pstate->push_new<opencl_ternop_cond_operation>
			      (std::move (first), std::move (mid),
			       std::move (last));
			  else
			    pstate->push_new<ternop_cond_operation>
			      (std::move (first), std::move (mid),
			       std::move (last));
			}
#line 2811 "c-exp.c.tmp"
    break;

  case 93: /* exp: exp '=' exp  */
#line 839 "c-exp.y"
                        {
			  if (pstate->language ()->la_language
			      == language_opencl)
			    pstate->wrap2<opencl_assign_operation> ();
			  else
			    pstate->wrap2<assign_operation> ();
			}
#line 2823 "c-exp.c.tmp"
    break;

  case 94: /* exp: exp ASSIGN_MODIFY exp  */
#line 849 "c-exp.y"
                        {
			  operation_up rhs = pstate->pop ();
			  operation_up lhs = pstate->pop ();
			  pstate->push_new<assign_modify_operation>
			    ((yyvsp[-1].opcode), std::move (lhs), std::move (rhs));
			}
#line 2834 "c-exp.c.tmp"
    break;

  case 95: /* exp: INT  */
#line 858 "c-exp.y"
                        {
			  pstate->push_new<long_const_operation>
			    ((yyvsp[0].typed_val_int).type, (yyvsp[0].typed_val_int).val);
			}
#line 2843 "c-exp.c.tmp"
    break;

  case 96: /* exp: COMPLEX_INT  */
#line 865 "c-exp.y"
                        {
			  operation_up real
			    = (make_operation<long_const_operation>
			       ((yyvsp[0].typed_val_int).type->target_type (), 0));
			  operation_up imag
			    = (make_operation<long_const_operation>
			       ((yyvsp[0].typed_val_int).type->target_type (), (yyvsp[0].typed_val_int).val));
			  pstate->push_new<complex_operation>
			    (std::move (real), std::move (imag), (yyvsp[0].typed_val_int).type);
			}
#line 2858 "c-exp.c.tmp"
    break;

  case 97: /* exp: CHAR  */
#line 878 "c-exp.y"
                        {
			  struct stoken_vector vec;
			  vec.len = 1;
			  vec.tokens = &(yyvsp[0].tsval);
			  pstate->push_c_string ((yyvsp[0].tsval).type, &vec);
			}
#line 2869 "c-exp.c.tmp"
    break;

  case 98: /* exp: NAME_OR_INT  */
#line 887 "c-exp.y"
                        { YYSTYPE val;
			  parse_number (pstate, (yyvsp[0].ssym).stoken.ptr,
					(yyvsp[0].ssym).stoken.length, 0, &val);
			  pstate->push_new<long_const_operation>
			    (val.typed_val_int.type,
			     val.typed_val_int.val);
			}
#line 2881 "c-exp.c.tmp"
    break;

  case 99: /* exp: FLOAT  */
#line 898 "c-exp.y"
                        {
			  float_data data;
			  std::copy (std::begin ((yyvsp[0].typed_val_float).val), std::end ((yyvsp[0].typed_val_float).val),
				     std::begin (data));
			  pstate->push_new<float_const_operation> ((yyvsp[0].typed_val_float).type, data);
			}
#line 2892 "c-exp.c.tmp"
    break;

  case 100: /* exp: COMPLEX_FLOAT  */
#line 907 "c-exp.y"
                        {
			  struct type *underlying = (yyvsp[0].typed_val_float).type->target_type ();

			  float_data val;
			  target_float_from_host_double (val.data (),
							 underlying, 0);
			  operation_up real
			    = (make_operation<float_const_operation>
			       (underlying, val));

			  std::copy (std::begin ((yyvsp[0].typed_val_float).val), std::end ((yyvsp[0].typed_val_float).val),
				     std::begin (val));
			  operation_up imag
			    = (make_operation<float_const_operation>
			       (underlying, val));

			  pstate->push_new<complex_operation>
			    (std::move (real), std::move (imag),
			     (yyvsp[0].typed_val_float).type);
			}
#line 2917 "c-exp.c.tmp"
    break;

  case 102: /* exp: DOLLAR_VARIABLE  */
#line 933 "c-exp.y"
                        {
			  pstate->push_dollar ((yyvsp[0].sval));
			}
#line 2925 "c-exp.c.tmp"
    break;

  case 103: /* exp: SELECTOR '(' name ')'  */
#line 939 "c-exp.y"
                        {
			  pstate->push_new<objc_selector_operation>
			    (copy_name ((yyvsp[-1].sval)));
			}
#line 2934 "c-exp.c.tmp"
    break;

  case 104: /* exp: SIZEOF '(' type ')'  */
#line 946 "c-exp.y"
                        { struct type *type = (yyvsp[-1].tval);
			  struct type *int_type
			    = lookup_signed_typename (pstate->language (),
						      "int");
			  type = check_typedef (type);

			    /* $5.3.3/2 of the C++ Standard (n3290 draft)
			       says of sizeof:  "When applied to a reference
			       or a reference type, the result is the size of
			       the referenced type."  */
			  if (TYPE_IS_REFERENCE (type))
			    type = check_typedef (type->target_type ());

			  pstate->push_new<long_const_operation>
			    (int_type, type->length ());
			}
#line 2955 "c-exp.c.tmp"
    break;

  case 105: /* exp: REINTERPRET_CAST '<' type_exp '>' '(' exp ')'  */
#line 965 "c-exp.y"
                        { pstate->wrap2<reinterpret_cast_operation> (); }
#line 2961 "c-exp.c.tmp"
    break;

  case 106: /* exp: STATIC_CAST '<' type_exp '>' '(' exp ')'  */
#line 969 "c-exp.y"
                        { pstate->wrap2<unop_cast_type_operation> (); }
#line 2967 "c-exp.c.tmp"
    break;

  case 107: /* exp: DYNAMIC_CAST '<' type_exp '>' '(' exp ')'  */
#line 973 "c-exp.y"
                        { pstate->wrap2<dynamic_cast_operation> (); }
#line 2973 "c-exp.c.tmp"
    break;

  case 108: /* exp: CONST_CAST '<' type_exp '>' '(' exp ')'  */
#line 977 "c-exp.y"
                        { /* We could do more error checking here, but
			     it doesn't seem worthwhile.  */
			  pstate->wrap2<unop_cast_type_operation> (); }
#line 2981 "c-exp.c.tmp"
    break;

  case 109: /* string_exp: STRING  */
#line 984 "c-exp.y"
                        {
			  /* We copy the string here, and not in the
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
#line 3001 "c-exp.c.tmp"
    break;

  case 110: /* string_exp: string_exp STRING  */
#line 1001 "c-exp.y"
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
#line 3021 "c-exp.c.tmp"
    break;

  case 111: /* exp: string_exp  */
#line 1019 "c-exp.y"
                        {
			  int i;
			  c_string_type type = C_STRING;

			  for (i = 0; i < (yyvsp[0].svec).len; ++i)
			    {
			      switch ((yyvsp[0].svec).tokens[i].type)
				{
				case C_STRING:
				  break;
				case C_WIDE_STRING:
				case C_STRING_16:
				case C_STRING_32:
				  if (type != C_STRING
				      && type != (yyvsp[0].svec).tokens[i].type)
				    error (_("Undefined string concatenation."));
				  type = (enum c_string_type_values) (yyvsp[0].svec).tokens[i].type;
				  break;
				default:
				  /* internal error */
				  internal_error ("unrecognized type in string concatenation");
				}
			    }

			  pstate->push_c_string (type, &(yyvsp[0].svec));
			  for (i = 0; i < (yyvsp[0].svec).len; ++i)
			    xfree ((yyvsp[0].svec).tokens[i].ptr);
			  xfree ((yyvsp[0].svec).tokens);
			}
#line 3055 "c-exp.c.tmp"
    break;

  case 112: /* exp: NSSTRING  */
#line 1053 "c-exp.y"
                        {
			  pstate->push_new<objc_nsstring_operation>
			    (copy_name ((yyvsp[0].sval)));
			}
#line 3064 "c-exp.c.tmp"
    break;

  case 113: /* exp: TRUEKEYWORD  */
#line 1061 "c-exp.y"
                        { pstate->push_new<long_const_operation>
			    (parse_type (pstate)->builtin_bool, 1);
			}
#line 3072 "c-exp.c.tmp"
    break;

  case 114: /* exp: FALSEKEYWORD  */
#line 1067 "c-exp.y"
                        { pstate->push_new<long_const_operation>
			    (parse_type (pstate)->builtin_bool, 0);
			}
#line 3080 "c-exp.c.tmp"
    break;

  case 115: /* block: BLOCKNAME  */
#line 1075 "c-exp.y"
                        {
			  if ((yyvsp[0].ssym).sym.symbol)
			    (yyval.bval) = (yyvsp[0].ssym).sym.symbol->value_block ();
			  else
			    error (_("No file or function \"%s\"."),
				   copy_name ((yyvsp[0].ssym).stoken).c_str ());
			}
#line 3092 "c-exp.c.tmp"
    break;

  case 116: /* block: FILENAME  */
#line 1083 "c-exp.y"
                        {
			  (yyval.bval) = (yyvsp[0].bval);
			}
#line 3100 "c-exp.c.tmp"
    break;

  case 117: /* block: block COLONCOLON name  */
#line 1089 "c-exp.y"
                        {
			  std::string copy = copy_name ((yyvsp[0].sval));
			  struct symbol *tem
			    = lookup_symbol (copy.c_str (), (yyvsp[-2].bval),
					     VAR_DOMAIN, NULL).symbol;

			  if (!tem || tem->aclass () != LOC_BLOCK)
			    error (_("No function \"%s\" in specified context."),
				   copy.c_str ());
			  (yyval.bval) = tem->value_block (); }
#line 3115 "c-exp.c.tmp"
    break;

  case 118: /* variable: name_not_typename ENTRY  */
#line 1102 "c-exp.y"
                        { struct symbol *sym = (yyvsp[-1].ssym).sym.symbol;

			  if (sym == NULL || !sym->is_argument ()
			      || !symbol_read_needs_frame (sym))
			    error (_("@entry can be used only for function "
				     "parameters, not for \"%s\""),
				   copy_name ((yyvsp[-1].ssym).stoken).c_str ());

			  pstate->push_new<var_entry_value_operation> (sym);
			}
#line 3130 "c-exp.c.tmp"
    break;

  case 119: /* variable: block COLONCOLON name  */
#line 1115 "c-exp.y"
                        {
			  std::string copy = copy_name ((yyvsp[0].sval));
			  struct block_symbol sym
			    = lookup_symbol (copy.c_str (), (yyvsp[-2].bval),
					     VAR_DOMAIN, NULL);

			  if (sym.symbol == 0)
			    error (_("No symbol \"%s\" in specified context."),
				   copy.c_str ());
			  if (symbol_read_needs_frame (sym.symbol))
			    pstate->block_tracker->update (sym);

			  pstate->push_new<var_value_operation> (sym);
			}
#line 3149 "c-exp.c.tmp"
    break;

  case 120: /* qualified_name: TYPENAME COLONCOLON name  */
#line 1132 "c-exp.y"
                        {
			  struct type *type = (yyvsp[-2].tsym).type;
			  type = check_typedef (type);
			  if (!type_aggregate_p (type))
			    error (_("`%s' is not defined as an aggregate type."),
				   TYPE_SAFE_NAME (type));

			  pstate->push_new<scope_operation> (type,
							     copy_name ((yyvsp[0].sval)));
			}
#line 3164 "c-exp.c.tmp"
    break;

  case 121: /* qualified_name: TYPENAME COLONCOLON '~' name  */
#line 1143 "c-exp.y"
                        {
			  struct type *type = (yyvsp[-3].tsym).type;

			  type = check_typedef (type);
			  if (!type_aggregate_p (type))
			    error (_("`%s' is not defined as an aggregate type."),
				   TYPE_SAFE_NAME (type));
			  std::string name = "~" + std::string ((yyvsp[0].sval).ptr,
								(yyvsp[0].sval).length);

			  /* Check for valid destructor name.  */
			  destructor_name_p (name.c_str (), (yyvsp[-3].tsym).type);
			  pstate->push_new<scope_operation> (type,
							     std::move (name));
			}
#line 3184 "c-exp.c.tmp"
    break;

  case 122: /* qualified_name: TYPENAME COLONCOLON name COLONCOLON name  */
#line 1159 "c-exp.y"
                        {
			  std::string copy = copy_name ((yyvsp[-2].sval));
			  error (_("No type \"%s\" within class "
				   "or namespace \"%s\"."),
				 copy.c_str (), TYPE_SAFE_NAME ((yyvsp[-4].tsym).type));
			}
#line 3195 "c-exp.c.tmp"
    break;

  case 124: /* variable: COLONCOLON name_not_typename  */
#line 1169 "c-exp.y"
                        {
			  std::string name = copy_name ((yyvsp[0].ssym).stoken);
			  struct block_symbol sym
			    = lookup_symbol (name.c_str (),
					     (const struct block *) NULL,
					     VAR_DOMAIN, NULL);
			  pstate->push_symbol (name.c_str (), sym);
			}
#line 3208 "c-exp.c.tmp"
    break;

  case 125: /* variable: name_not_typename  */
#line 1180 "c-exp.y"
                        { struct block_symbol sym = (yyvsp[0].ssym).sym;

			  if (sym.symbol)
			    {
			      if (symbol_read_needs_frame (sym.symbol))
				pstate->block_tracker->update (sym);

			      /* If we found a function, see if it's
				 an ifunc resolver that has the same
				 address as the ifunc symbol itself.
				 If so, prefer the ifunc symbol.  */

			      bound_minimal_symbol resolver
				= find_gnu_ifunc (sym.symbol);
			      if (resolver.minsym != NULL)
				pstate->push_new<var_msym_value_operation>
				  (resolver);
			      else
				pstate->push_new<var_value_operation> (sym);
			    }
			  else if ((yyvsp[0].ssym).is_a_field_of_this)
			    {
			      /* C++: it hangs off of `this'.  Must
				 not inadvertently convert from a method call
				 to data ref.  */
			      pstate->block_tracker->update (sym);
			      operation_up thisop
				= make_operation<op_this_operation> ();
			      pstate->push_new<structop_ptr_operation>
				(std::move (thisop), copy_name ((yyvsp[0].ssym).stoken));
			    }
			  else
			    {
			      std::string arg = copy_name ((yyvsp[0].ssym).stoken);

			      bound_minimal_symbol msymbol
				= lookup_bound_minimal_symbol (arg.c_str ());
			      if (msymbol.minsym == NULL)
				{
				  if (!have_full_symbols () && !have_partial_symbols ())
				    error (_("No symbol table is loaded.  Use the \"file\" command."));
				  else
				    error (_("No symbol \"%s\" in current context."),
					   arg.c_str ());
				}

			      /* This minsym might be an alias for
				 another function.  See if we can find
				 the debug symbol for the target, and
				 if so, use it instead, since it has
				 return type / prototype info.  This
				 is important for example for "p
				 *__errno_location()".  */
			      symbol *alias_target
				= ((msymbol.minsym->type () != mst_text_gnu_ifunc
				    && msymbol.minsym->type () != mst_data_gnu_ifunc)
				   ? find_function_alias_target (msymbol)
				   : NULL);
			      if (alias_target != NULL)
				{
				  block_symbol bsym { alias_target,
				    alias_target->value_block () };
				  pstate->push_new<var_value_operation> (bsym);
				}
			      else
				pstate->push_new<var_msym_value_operation>
				  (msymbol);
			    }
			}
#line 3282 "c-exp.c.tmp"
    break;

  case 128: /* single_qualifier: CONST_KEYWORD  */
#line 1257 "c-exp.y"
                        { cpstate->type_stack.insert (tp_const); }
#line 3288 "c-exp.c.tmp"
    break;

  case 129: /* single_qualifier: VOLATILE_KEYWORD  */
#line 1259 "c-exp.y"
                        { cpstate->type_stack.insert (tp_volatile); }
#line 3294 "c-exp.c.tmp"
    break;

  case 130: /* single_qualifier: ATOMIC  */
#line 1261 "c-exp.y"
                        { cpstate->type_stack.insert (tp_atomic); }
#line 3300 "c-exp.c.tmp"
    break;

  case 131: /* single_qualifier: RESTRICT  */
#line 1263 "c-exp.y"
                        { cpstate->type_stack.insert (tp_restrict); }
#line 3306 "c-exp.c.tmp"
    break;

  case 132: /* single_qualifier: '@' NAME  */
#line 1265 "c-exp.y"
                {
		  cpstate->type_stack.insert (pstate,
					      copy_name ((yyvsp[0].ssym).stoken).c_str ());
		}
#line 3315 "c-exp.c.tmp"
    break;

  case 133: /* single_qualifier: '@' UNKNOWN_CPP_NAME  */
#line 1270 "c-exp.y"
                {
		  cpstate->type_stack.insert (pstate,
					      copy_name ((yyvsp[0].ssym).stoken).c_str ());
		}
#line 3324 "c-exp.c.tmp"
    break;

  case 138: /* $@6: %empty  */
#line 1288 "c-exp.y"
                        { cpstate->type_stack.insert (tp_pointer); }
#line 3330 "c-exp.c.tmp"
    break;

  case 140: /* $@7: %empty  */
#line 1291 "c-exp.y"
                        { cpstate->type_stack.insert (tp_pointer); }
#line 3336 "c-exp.c.tmp"
    break;

  case 142: /* ptr_operator: '&'  */
#line 1294 "c-exp.y"
                        { cpstate->type_stack.insert (tp_reference); }
#line 3342 "c-exp.c.tmp"
    break;

  case 143: /* ptr_operator: '&' ptr_operator  */
#line 1296 "c-exp.y"
                        { cpstate->type_stack.insert (tp_reference); }
#line 3348 "c-exp.c.tmp"
    break;

  case 144: /* ptr_operator: ANDAND  */
#line 1298 "c-exp.y"
                        { cpstate->type_stack.insert (tp_rvalue_reference); }
#line 3354 "c-exp.c.tmp"
    break;

  case 145: /* ptr_operator: ANDAND ptr_operator  */
#line 1300 "c-exp.y"
                        { cpstate->type_stack.insert (tp_rvalue_reference); }
#line 3360 "c-exp.c.tmp"
    break;

  case 146: /* ptr_operator_ts: ptr_operator  */
#line 1304 "c-exp.y"
                        {
			  (yyval.type_stack) = cpstate->type_stack.create ();
			  cpstate->type_stacks.emplace_back ((yyval.type_stack));
			}
#line 3369 "c-exp.c.tmp"
    break;

  case 147: /* abs_decl: ptr_operator_ts direct_abs_decl  */
#line 1311 "c-exp.y"
                        { (yyval.type_stack) = (yyvsp[0].type_stack)->append ((yyvsp[-1].type_stack)); }
#line 3375 "c-exp.c.tmp"
    break;

  case 150: /* direct_abs_decl: '(' abs_decl ')'  */
#line 1317 "c-exp.y"
                        { (yyval.type_stack) = (yyvsp[-1].type_stack); }
#line 3381 "c-exp.c.tmp"
    break;

  case 151: /* direct_abs_decl: direct_abs_decl array_mod  */
#line 1319 "c-exp.y"
                        {
			  cpstate->type_stack.push ((yyvsp[-1].type_stack));
			  cpstate->type_stack.push ((yyvsp[0].lval));
			  cpstate->type_stack.push (tp_array);
			  (yyval.type_stack) = cpstate->type_stack.create ();
			  cpstate->type_stacks.emplace_back ((yyval.type_stack));
			}
#line 3393 "c-exp.c.tmp"
    break;

  case 152: /* direct_abs_decl: array_mod  */
#line 1327 "c-exp.y"
                        {
			  cpstate->type_stack.push ((yyvsp[0].lval));
			  cpstate->type_stack.push (tp_array);
			  (yyval.type_stack) = cpstate->type_stack.create ();
			  cpstate->type_stacks.emplace_back ((yyval.type_stack));
			}
#line 3404 "c-exp.c.tmp"
    break;

  case 153: /* direct_abs_decl: direct_abs_decl func_mod  */
#line 1335 "c-exp.y"
                        {
			  cpstate->type_stack.push ((yyvsp[-1].type_stack));
			  cpstate->type_stack.push ((yyvsp[0].tvec));
			  (yyval.type_stack) = cpstate->type_stack.create ();
			  cpstate->type_stacks.emplace_back ((yyval.type_stack));
			}
#line 3415 "c-exp.c.tmp"
    break;

  case 154: /* direct_abs_decl: func_mod  */
#line 1342 "c-exp.y"
                        {
			  cpstate->type_stack.push ((yyvsp[0].tvec));
			  (yyval.type_stack) = cpstate->type_stack.create ();
			  cpstate->type_stacks.emplace_back ((yyval.type_stack));
			}
#line 3425 "c-exp.c.tmp"
    break;

  case 155: /* array_mod: '[' ']'  */
#line 1350 "c-exp.y"
                        { (yyval.lval) = -1; }
#line 3431 "c-exp.c.tmp"
    break;

  case 156: /* array_mod: OBJC_LBRAC ']'  */
#line 1352 "c-exp.y"
                        { (yyval.lval) = -1; }
#line 3437 "c-exp.c.tmp"
    break;

  case 157: /* array_mod: '[' INT ']'  */
#line 1354 "c-exp.y"
                        { (yyval.lval) = (yyvsp[-1].typed_val_int).val; }
#line 3443 "c-exp.c.tmp"
    break;

  case 158: /* array_mod: OBJC_LBRAC INT ']'  */
#line 1356 "c-exp.y"
                        { (yyval.lval) = (yyvsp[-1].typed_val_int).val; }
#line 3449 "c-exp.c.tmp"
    break;

  case 159: /* func_mod: '(' ')'  */
#line 1360 "c-exp.y"
                        {
			  (yyval.tvec) = new std::vector<struct type *>;
			  cpstate->type_lists.emplace_back ((yyval.tvec));
			}
#line 3458 "c-exp.c.tmp"
    break;

  case 160: /* func_mod: '(' parameter_typelist ')'  */
#line 1365 "c-exp.y"
                        { (yyval.tvec) = (yyvsp[-1].tvec); }
#line 3464 "c-exp.c.tmp"
    break;

  case 162: /* scalar_type: INT_KEYWORD  */
#line 1384 "c-exp.y"
                        { (yyval.tval) = lookup_signed_typename (pstate->language (),
						       "int"); }
#line 3471 "c-exp.c.tmp"
    break;

  case 163: /* scalar_type: LONG  */
#line 1387 "c-exp.y"
                        { (yyval.tval) = lookup_signed_typename (pstate->language (),
						       "long"); }
#line 3478 "c-exp.c.tmp"
    break;

  case 164: /* scalar_type: SHORT  */
#line 1390 "c-exp.y"
                        { (yyval.tval) = lookup_signed_typename (pstate->language (),
						       "short"); }
#line 3485 "c-exp.c.tmp"
    break;

  case 165: /* scalar_type: LONG INT_KEYWORD  */
#line 1393 "c-exp.y"
                        { (yyval.tval) = lookup_signed_typename (pstate->language (),
						       "long"); }
#line 3492 "c-exp.c.tmp"
    break;

  case 166: /* scalar_type: LONG SIGNED_KEYWORD INT_KEYWORD  */
#line 1396 "c-exp.y"
                        { (yyval.tval) = lookup_signed_typename (pstate->language (),
						       "long"); }
#line 3499 "c-exp.c.tmp"
    break;

  case 167: /* scalar_type: LONG SIGNED_KEYWORD  */
#line 1399 "c-exp.y"
                        { (yyval.tval) = lookup_signed_typename (pstate->language (),
						       "long"); }
#line 3506 "c-exp.c.tmp"
    break;

  case 168: /* scalar_type: SIGNED_KEYWORD LONG INT_KEYWORD  */
#line 1402 "c-exp.y"
                        { (yyval.tval) = lookup_signed_typename (pstate->language (),
						       "long"); }
#line 3513 "c-exp.c.tmp"
    break;

  case 169: /* scalar_type: UNSIGNED LONG INT_KEYWORD  */
#line 1405 "c-exp.y"
                        { (yyval.tval) = lookup_unsigned_typename (pstate->language (),
							 "long"); }
#line 3520 "c-exp.c.tmp"
    break;

  case 170: /* scalar_type: LONG UNSIGNED INT_KEYWORD  */
#line 1408 "c-exp.y"
                        { (yyval.tval) = lookup_unsigned_typename (pstate->language (),
							 "long"); }
#line 3527 "c-exp.c.tmp"
    break;

  case 171: /* scalar_type: LONG UNSIGNED  */
#line 1411 "c-exp.y"
                        { (yyval.tval) = lookup_unsigned_typename (pstate->language (),
							 "long"); }
#line 3534 "c-exp.c.tmp"
    break;

  case 172: /* scalar_type: LONG LONG  */
#line 1414 "c-exp.y"
                        { (yyval.tval) = lookup_signed_typename (pstate->language (),
						       "long long"); }
#line 3541 "c-exp.c.tmp"
    break;

  case 173: /* scalar_type: LONG LONG INT_KEYWORD  */
#line 1417 "c-exp.y"
                        { (yyval.tval) = lookup_signed_typename (pstate->language (),
						       "long long"); }
#line 3548 "c-exp.c.tmp"
    break;

  case 174: /* scalar_type: LONG LONG SIGNED_KEYWORD INT_KEYWORD  */
#line 1420 "c-exp.y"
                        { (yyval.tval) = lookup_signed_typename (pstate->language (),
						       "long long"); }
#line 3555 "c-exp.c.tmp"
    break;

  case 175: /* scalar_type: LONG LONG SIGNED_KEYWORD  */
#line 1423 "c-exp.y"
                        { (yyval.tval) = lookup_signed_typename (pstate->language (),
						       "long long"); }
#line 3562 "c-exp.c.tmp"
    break;

  case 176: /* scalar_type: SIGNED_KEYWORD LONG LONG  */
#line 1426 "c-exp.y"
                        { (yyval.tval) = lookup_signed_typename (pstate->language (),
						       "long long"); }
#line 3569 "c-exp.c.tmp"
    break;

  case 177: /* scalar_type: SIGNED_KEYWORD LONG LONG INT_KEYWORD  */
#line 1429 "c-exp.y"
                        { (yyval.tval) = lookup_signed_typename (pstate->language (),
						       "long long"); }
#line 3576 "c-exp.c.tmp"
    break;

  case 178: /* scalar_type: UNSIGNED LONG LONG  */
#line 1432 "c-exp.y"
                        { (yyval.tval) = lookup_unsigned_typename (pstate->language (),
							 "long long"); }
#line 3583 "c-exp.c.tmp"
    break;

  case 179: /* scalar_type: UNSIGNED LONG LONG INT_KEYWORD  */
#line 1435 "c-exp.y"
                        { (yyval.tval) = lookup_unsigned_typename (pstate->language (),
							 "long long"); }
#line 3590 "c-exp.c.tmp"
    break;

  case 180: /* scalar_type: LONG LONG UNSIGNED  */
#line 1438 "c-exp.y"
                        { (yyval.tval) = lookup_unsigned_typename (pstate->language (),
							 "long long"); }
#line 3597 "c-exp.c.tmp"
    break;

  case 181: /* scalar_type: LONG LONG UNSIGNED INT_KEYWORD  */
#line 1441 "c-exp.y"
                        { (yyval.tval) = lookup_unsigned_typename (pstate->language (),
							 "long long"); }
#line 3604 "c-exp.c.tmp"
    break;

  case 182: /* scalar_type: SHORT INT_KEYWORD  */
#line 1444 "c-exp.y"
                        { (yyval.tval) = lookup_signed_typename (pstate->language (),
						       "short"); }
#line 3611 "c-exp.c.tmp"
    break;

  case 183: /* scalar_type: SHORT SIGNED_KEYWORD INT_KEYWORD  */
#line 1447 "c-exp.y"
                        { (yyval.tval) = lookup_signed_typename (pstate->language (),
						       "short"); }
#line 3618 "c-exp.c.tmp"
    break;

  case 184: /* scalar_type: SHORT SIGNED_KEYWORD  */
#line 1450 "c-exp.y"
                        { (yyval.tval) = lookup_signed_typename (pstate->language (),
						       "short"); }
#line 3625 "c-exp.c.tmp"
    break;

  case 185: /* scalar_type: UNSIGNED SHORT INT_KEYWORD  */
#line 1453 "c-exp.y"
                        { (yyval.tval) = lookup_unsigned_typename (pstate->language (),
							 "short"); }
#line 3632 "c-exp.c.tmp"
    break;

  case 186: /* scalar_type: SHORT UNSIGNED  */
#line 1456 "c-exp.y"
                        { (yyval.tval) = lookup_unsigned_typename (pstate->language (),
							 "short"); }
#line 3639 "c-exp.c.tmp"
    break;

  case 187: /* scalar_type: SHORT UNSIGNED INT_KEYWORD  */
#line 1459 "c-exp.y"
                        { (yyval.tval) = lookup_unsigned_typename (pstate->language (),
							 "short"); }
#line 3646 "c-exp.c.tmp"
    break;

  case 188: /* scalar_type: DOUBLE_KEYWORD  */
#line 1462 "c-exp.y"
                        { (yyval.tval) = lookup_typename (pstate->language (),
						"double",
						NULL,
						0); }
#line 3655 "c-exp.c.tmp"
    break;

  case 189: /* scalar_type: FLOAT_KEYWORD  */
#line 1467 "c-exp.y"
                        { (yyval.tval) = lookup_typename (pstate->language (),
						"float",
						NULL,
						0); }
#line 3664 "c-exp.c.tmp"
    break;

  case 190: /* scalar_type: LONG DOUBLE_KEYWORD  */
#line 1472 "c-exp.y"
                        { (yyval.tval) = lookup_typename (pstate->language (),
						"long double",
						NULL,
						0); }
#line 3673 "c-exp.c.tmp"
    break;

  case 191: /* scalar_type: UNSIGNED type_name  */
#line 1477 "c-exp.y"
                        { (yyval.tval) = lookup_unsigned_typename (pstate->language (),
							 (yyvsp[0].tsym).type->name ()); }
#line 3680 "c-exp.c.tmp"
    break;

  case 192: /* scalar_type: UNSIGNED  */
#line 1480 "c-exp.y"
                        { (yyval.tval) = lookup_unsigned_typename (pstate->language (),
							 "int"); }
#line 3687 "c-exp.c.tmp"
    break;

  case 193: /* scalar_type: SIGNED_KEYWORD type_name  */
#line 1483 "c-exp.y"
                        { (yyval.tval) = lookup_signed_typename (pstate->language (),
						       (yyvsp[0].tsym).type->name ()); }
#line 3694 "c-exp.c.tmp"
    break;

  case 194: /* scalar_type: SIGNED_KEYWORD  */
#line 1486 "c-exp.y"
                        { (yyval.tval) = lookup_signed_typename (pstate->language (),
						       "int"); }
#line 3701 "c-exp.c.tmp"
    break;

  case 195: /* typebase: TYPENAME  */
#line 1502 "c-exp.y"
                        { (yyval.tval) = (yyvsp[0].tsym).type; }
#line 3707 "c-exp.c.tmp"
    break;

  case 196: /* typebase: scalar_type  */
#line 1504 "c-exp.y"
                        { (yyval.tval) = (yyvsp[0].tval); }
#line 3713 "c-exp.c.tmp"
    break;

  case 197: /* typebase: COMPLEX scalar_type  */
#line 1506 "c-exp.y"
                        {
			  (yyval.tval) = init_complex_type (nullptr, (yyvsp[0].tval));
			}
#line 3721 "c-exp.c.tmp"
    break;

  case 198: /* typebase: STRUCT name  */
#line 1510 "c-exp.y"
                        { (yyval.tval)
			    = lookup_struct (copy_name ((yyvsp[0].sval)).c_str (),
					     pstate->expression_context_block);
			}
#line 3730 "c-exp.c.tmp"
    break;

  case 199: /* typebase: STRUCT COMPLETE  */
#line 1515 "c-exp.y"
                        {
			  pstate->mark_completion_tag (TYPE_CODE_STRUCT,
						       "", 0);
			  (yyval.tval) = NULL;
			}
#line 3740 "c-exp.c.tmp"
    break;

  case 200: /* typebase: STRUCT name COMPLETE  */
#line 1521 "c-exp.y"
                        {
			  pstate->mark_completion_tag (TYPE_CODE_STRUCT,
						       (yyvsp[-1].sval).ptr, (yyvsp[-1].sval).length);
			  (yyval.tval) = NULL;
			}
#line 3750 "c-exp.c.tmp"
    break;

  case 201: /* typebase: CLASS name  */
#line 1527 "c-exp.y"
                        { (yyval.tval) = lookup_struct
			    (copy_name ((yyvsp[0].sval)).c_str (),
			     pstate->expression_context_block);
			}
#line 3759 "c-exp.c.tmp"
    break;

  case 202: /* typebase: CLASS COMPLETE  */
#line 1532 "c-exp.y"
                        {
			  pstate->mark_completion_tag (TYPE_CODE_STRUCT,
						       "", 0);
			  (yyval.tval) = NULL;
			}
#line 3769 "c-exp.c.tmp"
    break;

  case 203: /* typebase: CLASS name COMPLETE  */
#line 1538 "c-exp.y"
                        {
			  pstate->mark_completion_tag (TYPE_CODE_STRUCT,
						       (yyvsp[-1].sval).ptr, (yyvsp[-1].sval).length);
			  (yyval.tval) = NULL;
			}
#line 3779 "c-exp.c.tmp"
    break;

  case 204: /* typebase: UNION name  */
#line 1544 "c-exp.y"
                        { (yyval.tval)
			    = lookup_union (copy_name ((yyvsp[0].sval)).c_str (),
					    pstate->expression_context_block);
			}
#line 3788 "c-exp.c.tmp"
    break;

  case 205: /* typebase: UNION COMPLETE  */
#line 1549 "c-exp.y"
                        {
			  pstate->mark_completion_tag (TYPE_CODE_UNION,
						       "", 0);
			  (yyval.tval) = NULL;
			}
#line 3798 "c-exp.c.tmp"
    break;

  case 206: /* typebase: UNION name COMPLETE  */
#line 1555 "c-exp.y"
                        {
			  pstate->mark_completion_tag (TYPE_CODE_UNION,
						       (yyvsp[-1].sval).ptr, (yyvsp[-1].sval).length);
			  (yyval.tval) = NULL;
			}
#line 3808 "c-exp.c.tmp"
    break;

  case 207: /* typebase: ENUM name  */
#line 1561 "c-exp.y"
                        { (yyval.tval) = lookup_enum (copy_name ((yyvsp[0].sval)).c_str (),
					    pstate->expression_context_block);
			}
#line 3816 "c-exp.c.tmp"
    break;

  case 208: /* typebase: ENUM COMPLETE  */
#line 1565 "c-exp.y"
                        {
			  pstate->mark_completion_tag (TYPE_CODE_ENUM, "", 0);
			  (yyval.tval) = NULL;
			}
#line 3825 "c-exp.c.tmp"
    break;

  case 209: /* typebase: ENUM name COMPLETE  */
#line 1570 "c-exp.y"
                        {
			  pstate->mark_completion_tag (TYPE_CODE_ENUM, (yyvsp[-1].sval).ptr,
						       (yyvsp[-1].sval).length);
			  (yyval.tval) = NULL;
			}
#line 3835 "c-exp.c.tmp"
    break;

  case 210: /* typebase: TEMPLATE name '<' type '>'  */
#line 1579 "c-exp.y"
                        { (yyval.tval) = lookup_template_type
			    (copy_name((yyvsp[-3].sval)).c_str (), (yyvsp[-1].tval),
			     pstate->expression_context_block);
			}
#line 3844 "c-exp.c.tmp"
    break;

  case 211: /* typebase: qualifier_seq_noopt typebase  */
#line 1584 "c-exp.y"
                        { (yyval.tval) = cpstate->type_stack.follow_types ((yyvsp[0].tval)); }
#line 3850 "c-exp.c.tmp"
    break;

  case 212: /* typebase: typebase qualifier_seq_noopt  */
#line 1586 "c-exp.y"
                        { (yyval.tval) = cpstate->type_stack.follow_types ((yyvsp[-1].tval)); }
#line 3856 "c-exp.c.tmp"
    break;

  case 214: /* type_name: INT_KEYWORD  */
#line 1591 "c-exp.y"
                {
		  (yyval.tsym).stoken.ptr = "int";
		  (yyval.tsym).stoken.length = 3;
		  (yyval.tsym).type = lookup_signed_typename (pstate->language (),
						    "int");
		}
#line 3867 "c-exp.c.tmp"
    break;

  case 215: /* type_name: LONG  */
#line 1598 "c-exp.y"
                {
		  (yyval.tsym).stoken.ptr = "long";
		  (yyval.tsym).stoken.length = 4;
		  (yyval.tsym).type = lookup_signed_typename (pstate->language (),
						    "long");
		}
#line 3878 "c-exp.c.tmp"
    break;

  case 216: /* type_name: SHORT  */
#line 1605 "c-exp.y"
                {
		  (yyval.tsym).stoken.ptr = "short";
		  (yyval.tsym).stoken.length = 5;
		  (yyval.tsym).type = lookup_signed_typename (pstate->language (),
						    "short");
		}
#line 3889 "c-exp.c.tmp"
    break;

  case 217: /* parameter_typelist: nonempty_typelist  */
#line 1615 "c-exp.y"
                        { check_parameter_typelist ((yyvsp[0].tvec)); }
#line 3895 "c-exp.c.tmp"
    break;

  case 218: /* parameter_typelist: nonempty_typelist ',' DOTDOTDOT  */
#line 1617 "c-exp.y"
                        {
			  (yyvsp[-2].tvec)->push_back (NULL);
			  check_parameter_typelist ((yyvsp[-2].tvec));
			  (yyval.tvec) = (yyvsp[-2].tvec);
			}
#line 3905 "c-exp.c.tmp"
    break;

  case 219: /* nonempty_typelist: type  */
#line 1626 "c-exp.y"
                {
		  std::vector<struct type *> *typelist
		    = new std::vector<struct type *>;
		  cpstate->type_lists.emplace_back (typelist);

		  typelist->push_back ((yyvsp[0].tval));
		  (yyval.tvec) = typelist;
		}
#line 3918 "c-exp.c.tmp"
    break;

  case 220: /* nonempty_typelist: nonempty_typelist ',' type  */
#line 1635 "c-exp.y"
                {
		  (yyvsp[-2].tvec)->push_back ((yyvsp[0].tval));
		  (yyval.tvec) = (yyvsp[-2].tvec);
		}
#line 3927 "c-exp.c.tmp"
    break;

  case 222: /* ptype: ptype abs_decl  */
#line 1643 "c-exp.y"
                {
		  cpstate->type_stack.push ((yyvsp[0].type_stack));
		  (yyval.tval) = cpstate->type_stack.follow_types ((yyvsp[-1].tval));
		}
#line 3936 "c-exp.c.tmp"
    break;

  case 223: /* conversion_type_id: typebase conversion_declarator  */
#line 1650 "c-exp.y"
                { (yyval.tval) = cpstate->type_stack.follow_types ((yyvsp[-1].tval)); }
#line 3942 "c-exp.c.tmp"
    break;

  case 228: /* const_or_volatile_noopt: const_and_volatile  */
#line 1662 "c-exp.y"
                        { cpstate->type_stack.insert (tp_const);
			  cpstate->type_stack.insert (tp_volatile);
			}
#line 3950 "c-exp.c.tmp"
    break;

  case 229: /* const_or_volatile_noopt: CONST_KEYWORD  */
#line 1666 "c-exp.y"
                        { cpstate->type_stack.insert (tp_const); }
#line 3956 "c-exp.c.tmp"
    break;

  case 230: /* const_or_volatile_noopt: VOLATILE_KEYWORD  */
#line 1668 "c-exp.y"
                        { cpstate->type_stack.insert (tp_volatile); }
#line 3962 "c-exp.c.tmp"
    break;

  case 231: /* oper: OPERATOR NEW  */
#line 1672 "c-exp.y"
                        { (yyval.sval) = operator_stoken (" new"); }
#line 3968 "c-exp.c.tmp"
    break;

  case 232: /* oper: OPERATOR DELETE  */
#line 1674 "c-exp.y"
                        { (yyval.sval) = operator_stoken (" delete"); }
#line 3974 "c-exp.c.tmp"
    break;

  case 233: /* oper: OPERATOR NEW '[' ']'  */
#line 1676 "c-exp.y"
                        { (yyval.sval) = operator_stoken (" new[]"); }
#line 3980 "c-exp.c.tmp"
    break;

  case 234: /* oper: OPERATOR DELETE '[' ']'  */
#line 1678 "c-exp.y"
                        { (yyval.sval) = operator_stoken (" delete[]"); }
#line 3986 "c-exp.c.tmp"
    break;

  case 235: /* oper: OPERATOR NEW OBJC_LBRAC ']'  */
#line 1680 "c-exp.y"
                        { (yyval.sval) = operator_stoken (" new[]"); }
#line 3992 "c-exp.c.tmp"
    break;

  case 236: /* oper: OPERATOR DELETE OBJC_LBRAC ']'  */
#line 1682 "c-exp.y"
                        { (yyval.sval) = operator_stoken (" delete[]"); }
#line 3998 "c-exp.c.tmp"
    break;

  case 237: /* oper: OPERATOR '+'  */
#line 1684 "c-exp.y"
                        { (yyval.sval) = operator_stoken ("+"); }
#line 4004 "c-exp.c.tmp"
    break;

  case 238: /* oper: OPERATOR '-'  */
#line 1686 "c-exp.y"
                        { (yyval.sval) = operator_stoken ("-"); }
#line 4010 "c-exp.c.tmp"
    break;

  case 239: /* oper: OPERATOR '*'  */
#line 1688 "c-exp.y"
                        { (yyval.sval) = operator_stoken ("*"); }
#line 4016 "c-exp.c.tmp"
    break;

  case 240: /* oper: OPERATOR '/'  */
#line 1690 "c-exp.y"
                        { (yyval.sval) = operator_stoken ("/"); }
#line 4022 "c-exp.c.tmp"
    break;

  case 241: /* oper: OPERATOR '%'  */
#line 1692 "c-exp.y"
                        { (yyval.sval) = operator_stoken ("%"); }
#line 4028 "c-exp.c.tmp"
    break;

  case 242: /* oper: OPERATOR '^'  */
#line 1694 "c-exp.y"
                        { (yyval.sval) = operator_stoken ("^"); }
#line 4034 "c-exp.c.tmp"
    break;

  case 243: /* oper: OPERATOR '&'  */
#line 1696 "c-exp.y"
                        { (yyval.sval) = operator_stoken ("&"); }
#line 4040 "c-exp.c.tmp"
    break;

  case 244: /* oper: OPERATOR '|'  */
#line 1698 "c-exp.y"
                        { (yyval.sval) = operator_stoken ("|"); }
#line 4046 "c-exp.c.tmp"
    break;

  case 245: /* oper: OPERATOR '~'  */
#line 1700 "c-exp.y"
                        { (yyval.sval) = operator_stoken ("~"); }
#line 4052 "c-exp.c.tmp"
    break;

  case 246: /* oper: OPERATOR '!'  */
#line 1702 "c-exp.y"
                        { (yyval.sval) = operator_stoken ("!"); }
#line 4058 "c-exp.c.tmp"
    break;

  case 247: /* oper: OPERATOR '='  */
#line 1704 "c-exp.y"
                        { (yyval.sval) = operator_stoken ("="); }
#line 4064 "c-exp.c.tmp"
    break;

  case 248: /* oper: OPERATOR '<'  */
#line 1706 "c-exp.y"
                        { (yyval.sval) = operator_stoken ("<"); }
#line 4070 "c-exp.c.tmp"
    break;

  case 249: /* oper: OPERATOR '>'  */
#line 1708 "c-exp.y"
                        { (yyval.sval) = operator_stoken (">"); }
#line 4076 "c-exp.c.tmp"
    break;

  case 250: /* oper: OPERATOR ASSIGN_MODIFY  */
#line 1710 "c-exp.y"
                        { const char *op = " unknown";
			  switch ((yyvsp[0].opcode))
			    {
			    case BINOP_RSH:
			      op = ">>=";
			      break;
			    case BINOP_LSH:
			      op = "<<=";
			      break;
			    case BINOP_ADD:
			      op = "+=";
			      break;
			    case BINOP_SUB:
			      op = "-=";
			      break;
			    case BINOP_MUL:
			      op = "*=";
			      break;
			    case BINOP_DIV:
			      op = "/=";
			      break;
			    case BINOP_REM:
			      op = "%=";
			      break;
			    case BINOP_BITWISE_IOR:
			      op = "|=";
			      break;
			    case BINOP_BITWISE_AND:
			      op = "&=";
			      break;
			    case BINOP_BITWISE_XOR:
			      op = "^=";
			      break;
			    default:
			      break;
			    }

			  (yyval.sval) = operator_stoken (op);
			}
#line 4120 "c-exp.c.tmp"
    break;

  case 251: /* oper: OPERATOR LSH  */
#line 1750 "c-exp.y"
                        { (yyval.sval) = operator_stoken ("<<"); }
#line 4126 "c-exp.c.tmp"
    break;

  case 252: /* oper: OPERATOR RSH  */
#line 1752 "c-exp.y"
                        { (yyval.sval) = operator_stoken (">>"); }
#line 4132 "c-exp.c.tmp"
    break;

  case 253: /* oper: OPERATOR EQUAL  */
#line 1754 "c-exp.y"
                        { (yyval.sval) = operator_stoken ("=="); }
#line 4138 "c-exp.c.tmp"
    break;

  case 254: /* oper: OPERATOR NOTEQUAL  */
#line 1756 "c-exp.y"
                        { (yyval.sval) = operator_stoken ("!="); }
#line 4144 "c-exp.c.tmp"
    break;

  case 255: /* oper: OPERATOR LEQ  */
#line 1758 "c-exp.y"
                        { (yyval.sval) = operator_stoken ("<="); }
#line 4150 "c-exp.c.tmp"
    break;

  case 256: /* oper: OPERATOR GEQ  */
#line 1760 "c-exp.y"
                        { (yyval.sval) = operator_stoken (">="); }
#line 4156 "c-exp.c.tmp"
    break;

  case 257: /* oper: OPERATOR ANDAND  */
#line 1762 "c-exp.y"
                        { (yyval.sval) = operator_stoken ("&&"); }
#line 4162 "c-exp.c.tmp"
    break;

  case 258: /* oper: OPERATOR OROR  */
#line 1764 "c-exp.y"
                        { (yyval.sval) = operator_stoken ("||"); }
#line 4168 "c-exp.c.tmp"
    break;

  case 259: /* oper: OPERATOR INCREMENT  */
#line 1766 "c-exp.y"
                        { (yyval.sval) = operator_stoken ("++"); }
#line 4174 "c-exp.c.tmp"
    break;

  case 260: /* oper: OPERATOR DECREMENT  */
#line 1768 "c-exp.y"
                        { (yyval.sval) = operator_stoken ("--"); }
#line 4180 "c-exp.c.tmp"
    break;

  case 261: /* oper: OPERATOR ','  */
#line 1770 "c-exp.y"
                        { (yyval.sval) = operator_stoken (","); }
#line 4186 "c-exp.c.tmp"
    break;

  case 262: /* oper: OPERATOR ARROW_STAR  */
#line 1772 "c-exp.y"
                        { (yyval.sval) = operator_stoken ("->*"); }
#line 4192 "c-exp.c.tmp"
    break;

  case 263: /* oper: OPERATOR ARROW  */
#line 1774 "c-exp.y"
                        { (yyval.sval) = operator_stoken ("->"); }
#line 4198 "c-exp.c.tmp"
    break;

  case 264: /* oper: OPERATOR '(' ')'  */
#line 1776 "c-exp.y"
                        { (yyval.sval) = operator_stoken ("()"); }
#line 4204 "c-exp.c.tmp"
    break;

  case 265: /* oper: OPERATOR '[' ']'  */
#line 1778 "c-exp.y"
                        { (yyval.sval) = operator_stoken ("[]"); }
#line 4210 "c-exp.c.tmp"
    break;

  case 266: /* oper: OPERATOR OBJC_LBRAC ']'  */
#line 1780 "c-exp.y"
                        { (yyval.sval) = operator_stoken ("[]"); }
#line 4216 "c-exp.c.tmp"
    break;

  case 267: /* oper: OPERATOR conversion_type_id  */
#line 1782 "c-exp.y"
                        {
			  string_file buf;
			  c_print_type ((yyvsp[0].tval), NULL, &buf, -1, 0,
					pstate->language ()->la_language,
					&type_print_raw_options);
			  std::string name = buf.release ();

			  /* This also needs canonicalization.  */
			  gdb::unique_xmalloc_ptr<char> canon
			    = cp_canonicalize_string (name.c_str ());
			  if (canon != nullptr)
			    name = canon.get ();
			  (yyval.sval) = operator_stoken ((" " + name).c_str ());
			}
#line 4235 "c-exp.c.tmp"
    break;

  case 269: /* field_name: DOUBLE_KEYWORD  */
#line 1805 "c-exp.y"
                               { (yyval.sval) = typename_stoken ("double"); }
#line 4241 "c-exp.c.tmp"
    break;

  case 270: /* field_name: FLOAT_KEYWORD  */
#line 1806 "c-exp.y"
                              { (yyval.sval) = typename_stoken ("float"); }
#line 4247 "c-exp.c.tmp"
    break;

  case 271: /* field_name: INT_KEYWORD  */
#line 1807 "c-exp.y"
                            { (yyval.sval) = typename_stoken ("int"); }
#line 4253 "c-exp.c.tmp"
    break;

  case 272: /* field_name: LONG  */
#line 1808 "c-exp.y"
                     { (yyval.sval) = typename_stoken ("long"); }
#line 4259 "c-exp.c.tmp"
    break;

  case 273: /* field_name: SHORT  */
#line 1809 "c-exp.y"
                      { (yyval.sval) = typename_stoken ("short"); }
#line 4265 "c-exp.c.tmp"
    break;

  case 274: /* field_name: SIGNED_KEYWORD  */
#line 1810 "c-exp.y"
                               { (yyval.sval) = typename_stoken ("signed"); }
#line 4271 "c-exp.c.tmp"
    break;

  case 275: /* field_name: UNSIGNED  */
#line 1811 "c-exp.y"
                         { (yyval.sval) = typename_stoken ("unsigned"); }
#line 4277 "c-exp.c.tmp"
    break;

  case 276: /* name: NAME  */
#line 1814 "c-exp.y"
                     { (yyval.sval) = (yyvsp[0].ssym).stoken; }
#line 4283 "c-exp.c.tmp"
    break;

  case 277: /* name: BLOCKNAME  */
#line 1815 "c-exp.y"
                          { (yyval.sval) = (yyvsp[0].ssym).stoken; }
#line 4289 "c-exp.c.tmp"
    break;

  case 278: /* name: TYPENAME  */
#line 1816 "c-exp.y"
                         { (yyval.sval) = (yyvsp[0].tsym).stoken; }
#line 4295 "c-exp.c.tmp"
    break;

  case 279: /* name: NAME_OR_INT  */
#line 1817 "c-exp.y"
                             { (yyval.sval) = (yyvsp[0].ssym).stoken; }
#line 4301 "c-exp.c.tmp"
    break;

  case 280: /* name: UNKNOWN_CPP_NAME  */
#line 1818 "c-exp.y"
                                  { (yyval.sval) = (yyvsp[0].ssym).stoken; }
#line 4307 "c-exp.c.tmp"
    break;

  case 281: /* name: oper  */
#line 1819 "c-exp.y"
                     { (yyval.sval) = (yyvsp[0].sval); }
#line 4313 "c-exp.c.tmp"
    break;

  case 284: /* name_not_typename: oper  */
#line 1832 "c-exp.y"
                        {
			  struct field_of_this_result is_a_field_of_this;

			  (yyval.ssym).stoken = (yyvsp[0].sval);
			  (yyval.ssym).sym
			    = lookup_symbol ((yyvsp[0].sval).ptr,
					     pstate->expression_context_block,
					     VAR_DOMAIN,
					     &is_a_field_of_this);
			  (yyval.ssym).is_a_field_of_this
			    = is_a_field_of_this.type != NULL;
			}
#line 4330 "c-exp.c.tmp"
    break;


#line 4334 "c-exp.c.tmp"

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

#line 1847 "c-exp.y"


/* Returns a stoken of the operator name given by OP (which does not
   include the string "operator").  */

static struct stoken
operator_stoken (const char *op)
{
  struct stoken st = { NULL, 0 };
  char *buf;

  st.length = CP_OPERATOR_LEN + strlen (op);
  buf = (char *) xmalloc (st.length + 1);
  strcpy (buf, CP_OPERATOR_STR);
  strcat (buf, op);
  st.ptr = buf;

  /* The toplevel (c_parse) will free the memory allocated here.  */
  cpstate->strings.emplace_back (buf);
  return st;
};

/* Returns a stoken of the type named TYPE.  */

static struct stoken
typename_stoken (const char *type)
{
  struct stoken st = { type, 0 };
  st.length = strlen (type);
  return st;
};

/* Return true if the type is aggregate-like.  */

static int
type_aggregate_p (struct type *type)
{
  return (type->code () == TYPE_CODE_STRUCT
	  || type->code () == TYPE_CODE_UNION
	  || type->code () == TYPE_CODE_NAMESPACE
	  || (type->code () == TYPE_CODE_ENUM
	      && type->is_declared_class ()));
}

/* Validate a parameter typelist.  */

static void
check_parameter_typelist (std::vector<struct type *> *params)
{
  struct type *type;
  int ix;

  for (ix = 0; ix < params->size (); ++ix)
    {
      type = (*params)[ix];
      if (type != NULL && check_typedef (type)->code () == TYPE_CODE_VOID)
	{
	  if (ix == 0)
	    {
	      if (params->size () == 1)
		{
		  /* Ok.  */
		  break;
		}
	      error (_("parameter types following 'void'"));
	    }
	  else
	    error (_("'void' invalid as parameter type"));
	}
    }
}

/* Take care of parsing a number (anything that starts with a digit).
   Set yylval and return the token type; update lexptr.
   LEN is the number of characters in it.  */

/*** Needs some error checking for the float case ***/

static int
parse_number (struct parser_state *par_state,
	      const char *buf, int len, int parsed_float, YYSTYPE *putithere)
{
  ULONGEST n = 0;
  ULONGEST prevn = 0;

  int i = 0;
  int c;
  int base = input_radix;
  int unsigned_p = 0;

  /* Number of "L" suffixes encountered.  */
  int long_p = 0;

  /* Imaginary number.  */
  bool imaginary_p = false;

  /* We have found a "L" or "U" (or "i") suffix.  */
  int found_suffix = 0;

  char *p;

  p = (char *) alloca (len);
  memcpy (p, buf, len);

  if (parsed_float)
    {
      if (len >= 1 && p[len - 1] == 'i')
	{
	  imaginary_p = true;
	  --len;
	}

      /* Handle suffixes for decimal floating-point: "df", "dd" or "dl".  */
      if (len >= 2 && p[len - 2] == 'd' && p[len - 1] == 'f')
	{
	  putithere->typed_val_float.type
	    = parse_type (par_state)->builtin_decfloat;
	  len -= 2;
	}
      else if (len >= 2 && p[len - 2] == 'd' && p[len - 1] == 'd')
	{
	  putithere->typed_val_float.type
	    = parse_type (par_state)->builtin_decdouble;
	  len -= 2;
	}
      else if (len >= 2 && p[len - 2] == 'd' && p[len - 1] == 'l')
	{
	  putithere->typed_val_float.type
	    = parse_type (par_state)->builtin_declong;
	  len -= 2;
	}
      /* Handle suffixes: 'f' for float, 'l' for long double.  */
      else if (len >= 1 && TOLOWER (p[len - 1]) == 'f')
	{
	  putithere->typed_val_float.type
	    = parse_type (par_state)->builtin_float;
	  len -= 1;
	}
      else if (len >= 1 && TOLOWER (p[len - 1]) == 'l')
	{
	  putithere->typed_val_float.type
	    = parse_type (par_state)->builtin_long_double;
	  len -= 1;
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

      if (imaginary_p)
	putithere->typed_val_float.type
	  = init_complex_type (nullptr, putithere->typed_val_float.type);

      return imaginary_p ? COMPLEX_FLOAT : FLOAT;
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
      if (c != 'l' && c != 'u' && c != 'i')
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
	  else if (c == 'i')
	    {
	      imaginary_p = true;
	      found_suffix = 1;
	    }
	  else
	    return ERROR;	/* Char not a digit */
	}
      if (i >= base)
	return ERROR;		/* Invalid digit in this base */

      if (c != 'l' && c != 'u' && c != 'i')
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
  bool have_signed
    /* No 'u' suffix.  */
    = !unsigned_p;
  bool have_unsigned
    = ((/* 'u' suffix.  */
	unsigned_p)
       || (/* Not a decimal.  */
	   base != 10)
       || (/* Allowed as a convenience, in case decimal doesn't fit in largest
	      signed type.  */
	   !fits_in_type (1, n, long_long_bits, true)));
  bool have_int
    /* No 'l' or 'll' suffix.  */
    = long_p == 0;
  bool have_long
    /* No 'll' suffix.  */
    = long_p <= 1;
  if (have_int && have_signed && fits_in_type (1, n, int_bits, true))
    putithere->typed_val_int.type = parse_type (par_state)->builtin_int;
  else if (have_int && have_unsigned && fits_in_type (1, n, int_bits, false))
    putithere->typed_val_int.type
      = parse_type (par_state)->builtin_unsigned_int;
  else if (have_long && have_signed && fits_in_type (1, n, long_bits, true))
    putithere->typed_val_int.type = parse_type (par_state)->builtin_long;
  else if (have_long && have_unsigned && fits_in_type (1, n, long_bits, false))
    putithere->typed_val_int.type
      = parse_type (par_state)->builtin_unsigned_long;
  else if (have_signed && fits_in_type (1, n, long_long_bits, true))
    putithere->typed_val_int.type
      = parse_type (par_state)->builtin_long_long;
  else if (have_unsigned && fits_in_type (1, n, long_long_bits, false))
    putithere->typed_val_int.type
      = parse_type (par_state)->builtin_unsigned_long_long;
  else
    error (_("Numeric constant too large."));
  putithere->typed_val_int.val = n;

   if (imaginary_p)
     putithere->typed_val_int.type
       = init_complex_type (nullptr, putithere->typed_val_int.type);

   return imaginary_p ? COMPLEX_INT : INT;
}

/* Temporary obstack used for holding strings.  */
static struct obstack tempbuf;
static int tempbuf_init;

/* Parse a C escape sequence.  The initial backslash of the sequence
   is at (*PTR)[-1].  *PTR will be updated to point to just after the
   last character of the sequence.  If OUTPUT is not NULL, the
   translated form of the escape sequence will be written there.  If
   OUTPUT is NULL, no output is written and the call will only affect
   *PTR.  If an escape sequence is expressed in target bytes, then the
   entire sequence will simply be copied to OUTPUT.  Return 1 if any
   character was emitted, 0 otherwise.  */

int
c_parse_escape (const char **ptr, struct obstack *output)
{
  const char *tokptr = *ptr;
  int result = 1;

  /* Some escape sequences undergo character set conversion.  Those we
     translate here.  */
  switch (*tokptr)
    {
      /* Hex escapes do not undergo character set conversion, so keep
	 the escape sequence for later.  */
    case 'x':
      if (output)
	obstack_grow_str (output, "\\x");
      ++tokptr;
      if (!ISXDIGIT (*tokptr))
	error (_("\\x escape without a following hex digit"));
      while (ISXDIGIT (*tokptr))
	{
	  if (output)
	    obstack_1grow (output, *tokptr);
	  ++tokptr;
	}
      break;

      /* Octal escapes do not undergo character set conversion, so
	 keep the escape sequence for later.  */
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
      {
	int i;
	if (output)
	  obstack_grow_str (output, "\\");
	for (i = 0;
	     i < 3 && ISDIGIT (*tokptr) && *tokptr != '8' && *tokptr != '9';
	     ++i)
	  {
	    if (output)
	      obstack_1grow (output, *tokptr);
	    ++tokptr;
	  }
      }
      break;

      /* We handle UCNs later.  We could handle them here, but that
	 would mean a spurious error in the case where the UCN could
	 be converted to the target charset but not the host
	 charset.  */
    case 'u':
    case 'U':
      {
	char c = *tokptr;
	int i, len = c == 'U' ? 8 : 4;
	if (output)
	  {
	    obstack_1grow (output, '\\');
	    obstack_1grow (output, *tokptr);
	  }
	++tokptr;
	if (!ISXDIGIT (*tokptr))
	  error (_("\\%c escape without a following hex digit"), c);
	for (i = 0; i < len && ISXDIGIT (*tokptr); ++i)
	  {
	    if (output)
	      obstack_1grow (output, *tokptr);
	    ++tokptr;
	  }
      }
      break;

      /* We must pass backslash through so that it does not
	 cause quoting during the second expansion.  */
    case '\\':
      if (output)
	obstack_grow_str (output, "\\\\");
      ++tokptr;
      break;

      /* Escapes which undergo conversion.  */
    case 'a':
      if (output)
	obstack_1grow (output, '\a');
      ++tokptr;
      break;
    case 'b':
      if (output)
	obstack_1grow (output, '\b');
      ++tokptr;
      break;
    case 'f':
      if (output)
	obstack_1grow (output, '\f');
      ++tokptr;
      break;
    case 'n':
      if (output)
	obstack_1grow (output, '\n');
      ++tokptr;
      break;
    case 'r':
      if (output)
	obstack_1grow (output, '\r');
      ++tokptr;
      break;
    case 't':
      if (output)
	obstack_1grow (output, '\t');
      ++tokptr;
      break;
    case 'v':
      if (output)
	obstack_1grow (output, '\v');
      ++tokptr;
      break;

      /* GCC extension.  */
    case 'e':
      if (output)
	obstack_1grow (output, HOST_ESCAPE_CHAR);
      ++tokptr;
      break;

      /* Backslash-newline expands to nothing at all.  */
    case '\n':
      ++tokptr;
      result = 0;
      break;

      /* A few escapes just expand to the character itself.  */
    case '\'':
    case '\"':
    case '?':
      /* GCC extensions.  */
    case '(':
    case '{':
    case '[':
    case '%':
      /* Unrecognized escapes turn into the character itself.  */
    default:
      if (output)
	obstack_1grow (output, *tokptr);
      ++tokptr;
      break;
    }
  *ptr = tokptr;
  return result;
}

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
  c_string_type type;
  int is_objc = 0;

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

  /* Record the string type.  */
  if (*tokptr == 'L')
    {
      type = C_WIDE_STRING;
      ++tokptr;
    }
  else if (*tokptr == 'u')
    {
      type = C_STRING_16;
      ++tokptr;
    }
  else if (*tokptr == 'U')
    {
      type = C_STRING_32;
      ++tokptr;
    }
  else if (*tokptr == '@')
    {
      /* An Objective C string.  */
      is_objc = 1;
      type = C_STRING;
      ++tokptr;
    }
  else
    type = C_STRING;

  /* Skip the quote.  */
  quote = *tokptr;
  if (quote == '\'')
    type |= C_CHAR;
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

  value->type = type;
  value->ptr = (char *) obstack_base (&tempbuf);
  value->length = obstack_object_size (&tempbuf);

  *outptr = tokptr;

  return quote == '"' ? (is_objc ? NSSTRING : STRING) : CHAR;
}

/* This is used to associate some attributes with a token.  */

enum token_flag
{
  /* If this bit is set, the token is C++-only.  */

  FLAG_CXX = 1,

  /* If this bit is set, the token is C-only.  */

  FLAG_C = 2,

  /* If this bit is set, the token is conditional: if there is a
     symbol of the same name, then the token is a symbol; otherwise,
     the token is a keyword.  */

  FLAG_SHADOW = 4
};
DEF_ENUM_FLAGS_TYPE (enum token_flag, token_flags);

struct token
{
  const char *oper;
  int token;
  enum exp_opcode opcode;
  token_flags flags;
};

static const struct token tokentab3[] =
  {
    {">>=", ASSIGN_MODIFY, BINOP_RSH, 0},
    {"<<=", ASSIGN_MODIFY, BINOP_LSH, 0},
    {"->*", ARROW_STAR, OP_NULL, FLAG_CXX},
    {"...", DOTDOTDOT, OP_NULL, 0}
  };

static const struct token tokentab2[] =
  {
    {"+=", ASSIGN_MODIFY, BINOP_ADD, 0},
    {"-=", ASSIGN_MODIFY, BINOP_SUB, 0},
    {"*=", ASSIGN_MODIFY, BINOP_MUL, 0},
    {"/=", ASSIGN_MODIFY, BINOP_DIV, 0},
    {"%=", ASSIGN_MODIFY, BINOP_REM, 0},
    {"|=", ASSIGN_MODIFY, BINOP_BITWISE_IOR, 0},
    {"&=", ASSIGN_MODIFY, BINOP_BITWISE_AND, 0},
    {"^=", ASSIGN_MODIFY, BINOP_BITWISE_XOR, 0},
    {"++", INCREMENT, OP_NULL, 0},
    {"--", DECREMENT, OP_NULL, 0},
    {"->", ARROW, OP_NULL, 0},
    {"&&", ANDAND, OP_NULL, 0},
    {"||", OROR, OP_NULL, 0},
    /* "::" is *not* only C++: gdb overrides its meaning in several
       different ways, e.g., 'filename'::func, function::variable.  */
    {"::", COLONCOLON, OP_NULL, 0},
    {"<<", LSH, OP_NULL, 0},
    {">>", RSH, OP_NULL, 0},
    {"==", EQUAL, OP_NULL, 0},
    {"!=", NOTEQUAL, OP_NULL, 0},
    {"<=", LEQ, OP_NULL, 0},
    {">=", GEQ, OP_NULL, 0},
    {".*", DOT_STAR, OP_NULL, FLAG_CXX}
  };

/* Identifier-like tokens.  Only type-specifiers than can appear in
   multi-word type names (for example 'double' can appear in 'long
   double') need to be listed here.  type-specifiers that are only ever
   single word (like 'char') are handled by the classify_name function.  */
static const struct token ident_tokens[] =
  {
    {"unsigned", UNSIGNED, OP_NULL, 0},
    {"template", TEMPLATE, OP_NULL, FLAG_CXX},
    {"volatile", VOLATILE_KEYWORD, OP_NULL, 0},
    {"struct", STRUCT, OP_NULL, 0},
    {"signed", SIGNED_KEYWORD, OP_NULL, 0},
    {"sizeof", SIZEOF, OP_NULL, 0},
    {"_Alignof", ALIGNOF, OP_NULL, 0},
    {"alignof", ALIGNOF, OP_NULL, FLAG_CXX},
    {"double", DOUBLE_KEYWORD, OP_NULL, 0},
    {"float", FLOAT_KEYWORD, OP_NULL, 0},
    {"false", FALSEKEYWORD, OP_NULL, FLAG_CXX},
    {"class", CLASS, OP_NULL, FLAG_CXX},
    {"union", UNION, OP_NULL, 0},
    {"short", SHORT, OP_NULL, 0},
    {"const", CONST_KEYWORD, OP_NULL, 0},
    {"restrict", RESTRICT, OP_NULL, FLAG_C | FLAG_SHADOW},
    {"__restrict__", RESTRICT, OP_NULL, 0},
    {"__restrict", RESTRICT, OP_NULL, 0},
    {"_Atomic", ATOMIC, OP_NULL, 0},
    {"enum", ENUM, OP_NULL, 0},
    {"long", LONG, OP_NULL, 0},
    {"_Complex", COMPLEX, OP_NULL, 0},
    {"__complex__", COMPLEX, OP_NULL, 0},

    {"true", TRUEKEYWORD, OP_NULL, FLAG_CXX},
    {"int", INT_KEYWORD, OP_NULL, 0},
    {"new", NEW, OP_NULL, FLAG_CXX},
    {"delete", DELETE, OP_NULL, FLAG_CXX},
    {"operator", OPERATOR, OP_NULL, FLAG_CXX},

    {"and", ANDAND, OP_NULL, FLAG_CXX},
    {"and_eq", ASSIGN_MODIFY, BINOP_BITWISE_AND, FLAG_CXX},
    {"bitand", '&', OP_NULL, FLAG_CXX},
    {"bitor", '|', OP_NULL, FLAG_CXX},
    {"compl", '~', OP_NULL, FLAG_CXX},
    {"not", '!', OP_NULL, FLAG_CXX},
    {"not_eq", NOTEQUAL, OP_NULL, FLAG_CXX},
    {"or", OROR, OP_NULL, FLAG_CXX},
    {"or_eq", ASSIGN_MODIFY, BINOP_BITWISE_IOR, FLAG_CXX},
    {"xor", '^', OP_NULL, FLAG_CXX},
    {"xor_eq", ASSIGN_MODIFY, BINOP_BITWISE_XOR, FLAG_CXX},

    {"const_cast", CONST_CAST, OP_NULL, FLAG_CXX },
    {"dynamic_cast", DYNAMIC_CAST, OP_NULL, FLAG_CXX },
    {"static_cast", STATIC_CAST, OP_NULL, FLAG_CXX },
    {"reinterpret_cast", REINTERPRET_CAST, OP_NULL, FLAG_CXX },

    {"__typeof__", TYPEOF, OP_TYPEOF, 0 },
    {"__typeof", TYPEOF, OP_TYPEOF, 0 },
    {"typeof", TYPEOF, OP_TYPEOF, FLAG_SHADOW },
    {"__decltype", DECLTYPE, OP_DECLTYPE, FLAG_CXX },
    {"decltype", DECLTYPE, OP_DECLTYPE, FLAG_CXX | FLAG_SHADOW },

    {"typeid", TYPEID, OP_TYPEID, FLAG_CXX}
  };


static void
scan_macro_expansion (const char *expansion)
{
  /* We'd better not be trying to push the stack twice.  */
  gdb_assert (! cpstate->macro_original_text);

  /* Copy to the obstack.  */
  const char *copy = obstack_strdup (&cpstate->expansion_obstack, expansion);

  /* Save the old lexptr value, so we can return to it when we're done
     parsing the expanded text.  */
  cpstate->macro_original_text = pstate->lexptr;
  pstate->lexptr = copy;
}

static int
scanning_macro_expansion (void)
{
  return cpstate->macro_original_text != 0;
}

static void
finished_macro_expansion (void)
{
  /* There'd better be something to pop back to.  */
  gdb_assert (cpstate->macro_original_text);

  /* Pop back to the original text.  */
  pstate->lexptr = cpstate->macro_original_text;
  cpstate->macro_original_text = 0;
}

/* Return true iff the token represents a C++ cast operator.  */

static int
is_cast_operator (const char *token, int len)
{
  return (! strncmp (token, "dynamic_cast", len)
	  || ! strncmp (token, "static_cast", len)
	  || ! strncmp (token, "reinterpret_cast", len)
	  || ! strncmp (token, "const_cast", len));
}

/* The scope used for macro expansion.  */
static struct macro_scope *expression_macro_scope;

/* This is set if a NAME token appeared at the very end of the input
   string, with no whitespace separating the name from the EOF.  This
   is used only when parsing to do field name completion.  */
static int saw_name_at_eof;

/* This is set if the previously-returned token was a structure
   operator -- either '.' or ARROW.  */
static bool last_was_structop;

/* Depth of parentheses.  */
static int paren_depth;

/* Read one token, getting characters through lexptr.  */

static int
lex_one_token (struct parser_state *par_state, bool *is_quoted_name)
{
  int c;
  int namelen;
  const char *tokstart;
  bool saw_structop = last_was_structop;

  last_was_structop = false;
  *is_quoted_name = false;

 retry:

  /* Check if this is a macro invocation that we need to expand.  */
  if (! scanning_macro_expansion ())
    {
      gdb::unique_xmalloc_ptr<char> expanded
	= macro_expand_next (&pstate->lexptr, *expression_macro_scope);

      if (expanded != nullptr)
	scan_macro_expansion (expanded.get ());
    }

  pstate->prev_lexptr = pstate->lexptr;

  tokstart = pstate->lexptr;
  /* See if it is a special token of length 3.  */
  for (const auto &token : tokentab3)
    if (strncmp (tokstart, token.oper, 3) == 0)
      {
	if ((token.flags & FLAG_CXX) != 0
	    && par_state->language ()->la_language != language_cplus)
	  break;
	gdb_assert ((token.flags & FLAG_C) == 0);

	pstate->lexptr += 3;
	yylval.opcode = token.opcode;
	return token.token;
      }

  /* See if it is a special token of length 2.  */
  for (const auto &token : tokentab2)
    if (strncmp (tokstart, token.oper, 2) == 0)
      {
	if ((token.flags & FLAG_CXX) != 0
	    && par_state->language ()->la_language != language_cplus)
	  break;
	gdb_assert ((token.flags & FLAG_C) == 0);

	pstate->lexptr += 2;
	yylval.opcode = token.opcode;
	if (token.token == ARROW)
	  last_was_structop = 1;
	return token.token;
      }

  switch (c = *tokstart)
    {
    case 0:
      /* If we were just scanning the result of a macro expansion,
	 then we need to resume scanning the original text.
	 If we're parsing for field name completion, and the previous
	 token allows such completion, return a COMPLETE token.
	 Otherwise, we were already scanning the original text, and
	 we're really done.  */
      if (scanning_macro_expansion ())
	{
	  finished_macro_expansion ();
	  goto retry;
	}
      else if (saw_name_at_eof)
	{
	  saw_name_at_eof = 0;
	  return COMPLETE;
	}
      else if (par_state->parse_completion && saw_structop)
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
      if (par_state->language ()->la_language == language_objc
	  && c == '[')
	return OBJC_LBRAC;
      return c;

    case ']':
    case ')':
      if (paren_depth == 0)
	return 0;
      paren_depth--;
      pstate->lexptr++;
      return c;

    case ',':
      if (pstate->comma_terminates
	  && paren_depth == 0
	  && ! scanning_macro_expansion ())
	return 0;
      pstate->lexptr++;
      return c;

    case '.':
      /* Might be a floating point number.  */
      if (pstate->lexptr[1] < '0' || pstate->lexptr[1] > '9')
	{
	  last_was_structop = true;
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
	int got_dot = 0, got_e = 0, got_p = 0, toktype;
	const char *p = tokstart;
	int hex = input_radix > 10;

	if (c == '0' && (p[1] == 'x' || p[1] == 'X'))
	  {
	    p += 2;
	    hex = 1;
	  }
	else if (c == '0' && (p[1]=='t' || p[1]=='T' || p[1]=='d' || p[1]=='D'))
	  {
	    p += 2;
	    hex = 0;
	  }

	for (;; ++p)
	  {
	    /* This test includes !hex because 'e' is a valid hex digit
	       and thus does not indicate a floating point number when
	       the radix is hex.  */
	    if (!hex && !got_e && !got_p && (*p == 'e' || *p == 'E'))
	      got_dot = got_e = 1;
	    else if (!got_e && !got_p && (*p == 'p' || *p == 'P'))
	      got_dot = got_p = 1;
	    /* This test does not include !hex, because a '.' always indicates
	       a decimal floating point number regardless of the radix.  */
	    else if (!got_dot && *p == '.')
	      got_dot = 1;
	    else if (((got_e && (p[-1] == 'e' || p[-1] == 'E'))
		      || (got_p && (p[-1] == 'p' || p[-1] == 'P')))
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
				got_dot | got_e | got_p, &yylval);
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

	if (par_state->language ()->la_language == language_objc)
	  {
	    size_t len = strlen ("selector");

	    if (strncmp (p, "selector", len) == 0
		&& (p[len] == '\0' || ISSPACE (p[len])))
	      {
		pstate->lexptr = p + len;
		return SELECTOR;
	      }
	    else if (*p == '"')
	      goto parse_string;
	  }

	while (ISSPACE (*p))
	  p++;
	size_t len = strlen ("entry");
	if (strncmp (p, "entry", len) == 0 && !c_ident_is_alnum (p[len])
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

    case 'L':
    case 'u':
    case 'U':
      if (tokstart[1] != '"' && tokstart[1] != '\'')
	break;
      /* Fall through.  */
    case '\'':
    case '"':

    parse_string:
      {
	int host_len;
	int result = parse_string_or_char (tokstart, &pstate->lexptr,
					   &yylval.tsval, &host_len);
	if (result == CHAR)
	  {
	    if (host_len == 0)
	      error (_("Empty character constant."));
	    else if (host_len > 2 && c == '\'')
	      {
		++tokstart;
		namelen = pstate->lexptr - tokstart - 1;
		*is_quoted_name = true;

		goto tryname;
	      }
	    else if (host_len > 1)
	      error (_("Invalid character constant."));
	  }
	return result;
      }
    }

  if (!(c == '_' || c == '$' || c_ident_is_alpha (c)))
    /* We must have come across a bad character (e.g. ';').  */
    error (_("Invalid character '%c' in expression."), c);

  /* It's a name.  See how long it is.  */
  namelen = 0;
  for (c = tokstart[namelen];
       (c == '_' || c == '$' || c_ident_is_alnum (c) || c == '<');)
    {
      /* Template parameter lists are part of the name.
	 FIXME: This mishandles `print $a<4&&$a>3'.  */

      if (c == '<')
	{
	  if (! is_cast_operator (tokstart, namelen))
	    {
	      /* Scan ahead to get rest of the template specification.  Note
		 that we look ahead only when the '<' adjoins non-whitespace
		 characters; for comparison expressions, e.g. "a < b > c",
		 there must be spaces before the '<', etc. */
	      const char *p = find_template_name_end (tokstart + namelen);

	      if (p)
		namelen = p - tokstart;
	    }
	  break;
	}
      c = tokstart[++namelen];
    }

  /* The token "if" terminates the expression and is NOT removed from
     the input stream.  It doesn't count if it appears in the
     expansion of a macro.  */
  if (namelen == 2
      && tokstart[0] == 'i'
      && tokstart[1] == 'f'
      && ! scanning_macro_expansion ())
    {
      return 0;
    }

  /* For the same reason (breakpoint conditions), "thread N"
     terminates the expression.  "thread" could be an identifier, but
     an identifier is never followed by a number without intervening
     punctuation.  "task" is similar.  Handle abbreviations of these,
     similarly to breakpoint.c:find_condition_and_thread.  */
  if (namelen >= 1
      && (strncmp (tokstart, "thread", namelen) == 0
	  || strncmp (tokstart, "task", namelen) == 0)
      && (tokstart[namelen] == ' ' || tokstart[namelen] == '\t')
      && ! scanning_macro_expansion ())
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
	if ((token.flags & FLAG_CXX) != 0
	    && par_state->language ()->la_language != language_cplus)
	  break;
	if ((token.flags & FLAG_C) != 0
	    && par_state->language ()->la_language != language_c
	    && par_state->language ()->la_language != language_objc)
	  break;

	if ((token.flags & FLAG_SHADOW) != 0)
	  {
	    struct field_of_this_result is_a_field_of_this;

	    if (lookup_symbol (copy.c_str (),
			       pstate->expression_context_block,
			       VAR_DOMAIN,
			       (par_state->language ()->la_language
				== language_cplus ? &is_a_field_of_this
				: NULL)).symbol
		!= NULL)
	      {
		/* The keyword is shadowed.  */
		break;
	      }
	  }

	/* It is ok to always set this, even though we don't always
	   strictly need to.  */
	yylval.opcode = token.opcode;
	return token.token;
      }

  if (*tokstart == '$')
    return DOLLAR_VARIABLE;

  if (pstate->parse_completion && *pstate->lexptr == '\0')
    saw_name_at_eof = 1;

  yylval.ssym.stoken = yylval.sval;
  yylval.ssym.sym.symbol = NULL;
  yylval.ssym.sym.block = NULL;
  yylval.ssym.is_a_field_of_this = 0;
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

/* Temporary storage for c_lex; this holds symbol names as they are
   built up.  */
static auto_obstack name_obstack;

/* Classify a NAME token.  The contents of the token are in `yylval'.
   Updates yylval and returns the new token type.  BLOCK is the block
   in which lookups start; this can be NULL to mean the global scope.
   IS_QUOTED_NAME is non-zero if the name token was originally quoted
   in single quotes.  IS_AFTER_STRUCTOP is true if this name follows
   a structure operator -- either '.' or ARROW  */

static int
classify_name (struct parser_state *par_state, const struct block *block,
	       bool is_quoted_name, bool is_after_structop)
{
  struct block_symbol bsym;
  struct field_of_this_result is_a_field_of_this;

  std::string copy = copy_name (yylval.sval);

  /* Initialize this in case we *don't* use it in this call; that way
     we can refer to it unconditionally below.  */
  memset (&is_a_field_of_this, 0, sizeof (is_a_field_of_this));

  bsym = lookup_symbol (copy.c_str (), block, VAR_DOMAIN,
			par_state->language ()->name_of_this ()
			? &is_a_field_of_this : NULL);

  if (bsym.symbol && bsym.symbol->aclass () == LOC_BLOCK)
    {
      yylval.ssym.sym = bsym;
      yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;
      return BLOCKNAME;
    }
  else if (!bsym.symbol)
    {
      /* If we found a field of 'this', we might have erroneously
	 found a constructor where we wanted a type name.  Handle this
	 case by noticing that we found a constructor and then look up
	 the type tag instead.  */
      if (is_a_field_of_this.type != NULL
	  && is_a_field_of_this.fn_field != NULL
	  && TYPE_FN_FIELD_CONSTRUCTOR (is_a_field_of_this.fn_field->fn_fields,
					0))
	{
	  struct field_of_this_result inner_is_a_field_of_this;

	  bsym = lookup_symbol (copy.c_str (), block, STRUCT_DOMAIN,
				&inner_is_a_field_of_this);
	  if (bsym.symbol != NULL)
	    {
	      yylval.tsym.type = bsym.symbol->type ();
	      return TYPENAME;
	    }
	}

      /* If we found a field on the "this" object, or we are looking
	 up a field on a struct, then we want to prefer it over a
	 filename.  However, if the name was quoted, then it is better
	 to check for a filename or a block, since this is the only
	 way the user has of requiring the extension to be used.  */
      if ((is_a_field_of_this.type == NULL && !is_after_structop) 
	  || is_quoted_name)
	{
	  /* See if it's a file name. */
	  struct symtab *symtab;

	  symtab = lookup_symtab (copy.c_str ());
	  if (symtab)
	    {
	      yylval.bval
		= symtab->compunit ()->blockvector ()->static_block ();

	      return FILENAME;
	    }
	}
    }

  if (bsym.symbol && bsym.symbol->aclass () == LOC_TYPEDEF)
    {
      yylval.tsym.type = bsym.symbol->type ();
      return TYPENAME;
    }

  /* See if it's an ObjC classname.  */
  if (par_state->language ()->la_language == language_objc && !bsym.symbol)
    {
      CORE_ADDR Class = lookup_objc_class (par_state->gdbarch (),
					   copy.c_str ());
      if (Class)
	{
	  struct symbol *sym;

	  yylval.theclass.theclass = Class;
	  sym = lookup_struct_typedef (copy.c_str (),
				       par_state->expression_context_block, 1);
	  if (sym)
	    yylval.theclass.type = sym->type ();
	  return CLASSNAME;
	}
    }

  /* Input names that aren't symbols but ARE valid hex numbers, when
     the input radix permits them, can be names or numbers depending
     on the parse.  Note we support radixes > 16 here.  */
  if (!bsym.symbol
      && ((copy[0] >= 'a' && copy[0] < 'a' + input_radix - 10)
	  || (copy[0] >= 'A' && copy[0] < 'A' + input_radix - 10)))
    {
      YYSTYPE newlval;	/* Its value is ignored.  */
      int hextype = parse_number (par_state, copy.c_str (), yylval.sval.length,
				  0, &newlval);

      if (hextype == INT)
	{
	  yylval.ssym.sym = bsym;
	  yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;
	  return NAME_OR_INT;
	}
    }

  /* Any other kind of symbol */
  yylval.ssym.sym = bsym;
  yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;

  if (bsym.symbol == NULL
      && par_state->language ()->la_language == language_cplus
      && is_a_field_of_this.type == NULL
      && lookup_minimal_symbol (copy.c_str (), NULL, NULL).minsym == NULL)
    return UNKNOWN_CPP_NAME;

  return NAME;
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
    return classify_name (par_state, block, false, false);

  type = check_typedef (context);
  if (!type_aggregate_p (type))
    return ERROR;

  std::string copy = copy_name (yylval.ssym.stoken);
  /* N.B. We assume the symbol can only be in VAR_DOMAIN.  */
  yylval.ssym.sym = cp_lookup_nested_symbol (type, copy.c_str (), block,
					     VAR_DOMAIN);

  /* If no symbol was found, search for a matching base class named
     COPY.  This will allow users to enter qualified names of class members
     relative to the `this' pointer.  */
  if (yylval.ssym.sym.symbol == NULL)
    {
      struct type *base_type = cp_find_type_baseclass_by_name (type,
							       copy.c_str ());

      if (base_type != NULL)
	{
	  yylval.tsym.type = base_type;
	  return TYPENAME;
	}

      return ERROR;
    }

  switch (yylval.ssym.sym.symbol->aclass ())
    {
    case LOC_BLOCK:
    case LOC_LABEL:
      /* cp_lookup_nested_symbol might have accidentally found a constructor
	 named COPY when we really wanted a base class of the same name.
	 Double-check this case by looking for a base class.  */
      {
	struct type *base_type
	  = cp_find_type_baseclass_by_name (type, copy.c_str ());

	if (base_type != NULL)
	  {
	    yylval.tsym.type = base_type;
	    return TYPENAME;
	  }
      }
      return ERROR;

    case LOC_TYPEDEF:
      yylval.tsym.type = yylval.ssym.sym.symbol->type ();
      return TYPENAME;

    default:
      return NAME;
    }
  internal_error (_("not reached"));
}

/* The outer level of a two-level lexer.  This calls the inner lexer
   to return tokens.  It then either returns these tokens, or
   aggregates them into a larger token.  This lets us work around a
   problem in our parsing approach, where the parser could not
   distinguish between qualified names and qualified types at the
   right point.

   This approach is still not ideal, because it mishandles template
   types.  See the comment in lex_one_token for an example.  However,
   this is still an improvement over the earlier approach, and will
   suffice until we move to better parsing technology.  */

static int
yylex (void)
{
  token_and_value current;
  int first_was_coloncolon, last_was_coloncolon;
  struct type *context_type = NULL;
  int last_to_examine, next_to_examine, checkpoint;
  const struct block *search_block;
  bool is_quoted_name, last_lex_was_structop;

  if (popping && !token_fifo.empty ())
    goto do_pop;
  popping = 0;

  last_lex_was_structop = last_was_structop;

  /* Read the first token and decide what to do.  Most of the
     subsequent code is C++-only; but also depends on seeing a "::" or
     name-like token.  */
  current.token = lex_one_token (pstate, &is_quoted_name);
  if (current.token == NAME)
    current.token = classify_name (pstate, pstate->expression_context_block,
				   is_quoted_name, last_lex_was_structop);
  if (pstate->language ()->la_language != language_cplus
      || (current.token != TYPENAME && current.token != COLONCOLON
	  && current.token != FILENAME))
    return current.token;

  /* Read any sequence of alternating "::" and name-like tokens into
     the token FIFO.  */
  current.value = yylval;
  token_fifo.push_back (current);
  last_was_coloncolon = current.token == COLONCOLON;
  while (1)
    {
      bool ignore;

      /* We ignore quoted names other than the very first one.
	 Subsequent ones do not have any special meaning.  */
      current.token = lex_one_token (pstate, &ignore);
      current.value = yylval;
      token_fifo.push_back (current);

      if ((last_was_coloncolon && current.token != NAME)
	  || (!last_was_coloncolon && current.token != COLONCOLON))
	break;
      last_was_coloncolon = !last_was_coloncolon;
    }
  popping = 1;

  /* We always read one extra token, so compute the number of tokens
     to examine accordingly.  */
  last_to_examine = token_fifo.size () - 2;
  next_to_examine = 0;

  current = token_fifo[next_to_examine];
  ++next_to_examine;

  name_obstack.clear ();
  checkpoint = 0;
  if (current.token == FILENAME)
    search_block = current.value.bval;
  else if (current.token == COLONCOLON)
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

  first_was_coloncolon = current.token == COLONCOLON;
  last_was_coloncolon = first_was_coloncolon;

  while (next_to_examine <= last_to_examine)
    {
      token_and_value next;

      next = token_fifo[next_to_examine];
      ++next_to_examine;

      if (next.token == NAME && last_was_coloncolon)
	{
	  int classification;

	  yylval = next.value;
	  classification = classify_inner_name (pstate, search_block,
						context_type);
	  /* We keep going until we either run out of names, or until
	     we have a qualified name which is not a type.  */
	  if (classification != TYPENAME && classification != NAME)
	    break;

	  /* Accept up to this token.  */
	  checkpoint = next_to_examine;

	  /* Update the partial name we are constructing.  */
	  if (context_type != NULL)
	    {
	      /* We don't want to put a leading "::" into the name.  */
	      obstack_grow_str (&name_obstack, "::");
	    }
	  obstack_grow (&name_obstack, next.value.sval.ptr,
			next.value.sval.length);

	  yylval.sval.ptr = (const char *) obstack_base (&name_obstack);
	  yylval.sval.length = obstack_object_size (&name_obstack);
	  current.value = yylval;
	  current.token = classification;

	  last_was_coloncolon = 0;

	  if (classification == NAME)
	    break;

	  context_type = yylval.tsym.type;
	}
      else if (next.token == COLONCOLON && !last_was_coloncolon)
	last_was_coloncolon = 1;
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
      current.value.sval.ptr
	= obstack_strndup (&cpstate->expansion_obstack,
			   current.value.sval.ptr,
			   current.value.sval.length);

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
c_parse (struct parser_state *par_state)
{
  /* Setting up the parser state.  */
  scoped_restore pstate_restore = make_scoped_restore (&pstate);
  gdb_assert (par_state != NULL);
  pstate = par_state;

  c_parse_state cstate;
  scoped_restore cstate_restore = make_scoped_restore (&cpstate, &cstate);

  gdb::unique_xmalloc_ptr<struct macro_scope> macro_scope;

  if (par_state->expression_context_block)
    macro_scope
      = sal_macro_scope (find_pc_line (par_state->expression_context_pc, 0));
  else
    macro_scope = default_macro_scope ();
  if (! macro_scope)
    macro_scope = user_macro_scope ();

  scoped_restore restore_macro_scope
    = make_scoped_restore (&expression_macro_scope, macro_scope.get ());

  scoped_restore restore_yydebug = make_scoped_restore (&yydebug,
							parser_debug);

  /* Initialize some state used by the lexer.  */
  last_was_structop = false;
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

#if defined(YYBISON) && YYBISON < 30800


/* This is called via the YYPRINT macro when parser debugging is
   enabled.  It prints a token's value.  */

static void
c_print_token (FILE *file, int type, YYSTYPE value)
{
  switch (type)
    {
    case INT:
      parser_fprintf (file, "typed_val_int<%s, %s>",
		      TYPE_SAFE_NAME (value.typed_val_int.type),
		      pulongest (value.typed_val_int.val));
      break;

    case CHAR:
    case STRING:
      {
	char *copy = (char *) alloca (value.tsval.length + 1);

	memcpy (copy, value.tsval.ptr, value.tsval.length);
	copy[value.tsval.length] = '\0';

	parser_fprintf (file, "tsval<type=%d, %s>", value.tsval.type, copy);
      }
      break;

    case NSSTRING:
    case DOLLAR_VARIABLE:
      parser_fprintf (file, "sval<%s>", copy_name (value.sval).c_str ());
      break;

    case TYPENAME:
      parser_fprintf (file, "tsym<type=%s, name=%s>",
		      TYPE_SAFE_NAME (value.tsym.type),
		      copy_name (value.tsym.stoken).c_str ());
      break;

    case NAME:
    case UNKNOWN_CPP_NAME:
    case NAME_OR_INT:
    case BLOCKNAME:
      parser_fprintf (file, "ssym<name=%s, sym=%s, field_of_this=%d>",
		       copy_name (value.ssym.stoken).c_str (),
		       (value.ssym.sym.symbol == NULL
			? "(null)" : value.ssym.sym.symbol->print_name ()),
		       value.ssym.is_a_field_of_this);
      break;

    case FILENAME:
      parser_fprintf (file, "bval<%s>", host_address_to_string (value.bval));
      break;
    }
}

#endif

static void
yyerror (const char *msg)
{
  if (pstate->prev_lexptr)
    pstate->lexptr = pstate->prev_lexptr;

  error (_("A %s in expression, near `%s'."), msg, pstate->lexptr);
}
