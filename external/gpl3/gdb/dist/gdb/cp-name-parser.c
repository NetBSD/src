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
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 38 "cp-name-parser.y"



#include <unistd.h>
#include "gdbsupport/gdb-safe-ctype.h"
#include "demangle.h"
#include "cp-support.h"
#include "c-support.h"
#include "parser-defs.h"
#include "gdbsupport/selftest.h"

#define GDB_YY_REMAP_PREFIX cpname
#include "yy-remap.h"


#line 87 "cp-name-parser.c.tmp"

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
    NAME = 260,                    /* NAME  */
    STRUCT = 261,                  /* STRUCT  */
    CLASS = 262,                   /* CLASS  */
    UNION = 263,                   /* UNION  */
    ENUM = 264,                    /* ENUM  */
    SIZEOF = 265,                  /* SIZEOF  */
    UNSIGNED = 266,                /* UNSIGNED  */
    COLONCOLON = 267,              /* COLONCOLON  */
    TEMPLATE = 268,                /* TEMPLATE  */
    ERROR = 269,                   /* ERROR  */
    NEW = 270,                     /* NEW  */
    DELETE = 271,                  /* DELETE  */
    OPERATOR = 272,                /* OPERATOR  */
    STATIC_CAST = 273,             /* STATIC_CAST  */
    REINTERPRET_CAST = 274,        /* REINTERPRET_CAST  */
    DYNAMIC_CAST = 275,            /* DYNAMIC_CAST  */
    SIGNED_KEYWORD = 276,          /* SIGNED_KEYWORD  */
    LONG = 277,                    /* LONG  */
    SHORT = 278,                   /* SHORT  */
    INT_KEYWORD = 279,             /* INT_KEYWORD  */
    CONST_KEYWORD = 280,           /* CONST_KEYWORD  */
    VOLATILE_KEYWORD = 281,        /* VOLATILE_KEYWORD  */
    DOUBLE_KEYWORD = 282,          /* DOUBLE_KEYWORD  */
    BOOL = 283,                    /* BOOL  */
    ELLIPSIS = 284,                /* ELLIPSIS  */
    RESTRICT = 285,                /* RESTRICT  */
    VOID = 286,                    /* VOID  */
    FLOAT_KEYWORD = 287,           /* FLOAT_KEYWORD  */
    CHAR = 288,                    /* CHAR  */
    WCHAR_T = 289,                 /* WCHAR_T  */
    ASSIGN_MODIFY = 290,           /* ASSIGN_MODIFY  */
    TRUEKEYWORD = 291,             /* TRUEKEYWORD  */
    FALSEKEYWORD = 292,            /* FALSEKEYWORD  */
    DEMANGLER_SPECIAL = 293,       /* DEMANGLER_SPECIAL  */
    CONSTRUCTION_VTABLE = 294,     /* CONSTRUCTION_VTABLE  */
    CONSTRUCTION_IN = 295,         /* CONSTRUCTION_IN  */
    OROR = 296,                    /* OROR  */
    ANDAND = 297,                  /* ANDAND  */
    EQUAL = 298,                   /* EQUAL  */
    NOTEQUAL = 299,                /* NOTEQUAL  */
    LEQ = 300,                     /* LEQ  */
    GEQ = 301,                     /* GEQ  */
    SPACESHIP = 302,               /* SPACESHIP  */
    LSH = 303,                     /* LSH  */
    RSH = 304,                     /* RSH  */
    UNARY = 305,                   /* UNARY  */
    INCREMENT = 306,               /* INCREMENT  */
    DECREMENT = 307,               /* DECREMENT  */
    ARROW = 308                    /* ARROW  */
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
#define NAME 260
#define STRUCT 261
#define CLASS 262
#define UNION 263
#define ENUM 264
#define SIZEOF 265
#define UNSIGNED 266
#define COLONCOLON 267
#define TEMPLATE 268
#define ERROR 269
#define NEW 270
#define DELETE 271
#define OPERATOR 272
#define STATIC_CAST 273
#define REINTERPRET_CAST 274
#define DYNAMIC_CAST 275
#define SIGNED_KEYWORD 276
#define LONG 277
#define SHORT 278
#define INT_KEYWORD 279
#define CONST_KEYWORD 280
#define VOLATILE_KEYWORD 281
#define DOUBLE_KEYWORD 282
#define BOOL 283
#define ELLIPSIS 284
#define RESTRICT 285
#define VOID 286
#define FLOAT_KEYWORD 287
#define CHAR 288
#define WCHAR_T 289
#define ASSIGN_MODIFY 290
#define TRUEKEYWORD 291
#define FALSEKEYWORD 292
#define DEMANGLER_SPECIAL 293
#define CONSTRUCTION_VTABLE 294
#define CONSTRUCTION_IN 295
#define OROR 296
#define ANDAND 297
#define EQUAL 298
#define NOTEQUAL 299
#define LEQ 300
#define GEQ 301
#define SPACESHIP 302
#define LSH 303
#define RSH 304
#define UNARY 305
#define INCREMENT 306
#define DECREMENT 307
#define ARROW 308

/* Value type.  */
#if ! defined cp_name_parser_YYSTYPE && ! defined cp_name_parser_YYSTYPE_IS_DECLARED
union cp_name_parser_YYSTYPE
{
#line 55 "cp-name-parser.y"

    struct demangle_component *comp;
    struct nested {
      struct demangle_component *comp;
      struct demangle_component **last;
    } nested;
    struct {
      struct demangle_component *comp, *last;
    } nested1;
    struct {
      struct demangle_component *comp, **last;
      struct nested fn;
      struct demangle_component *start;
      int fold_flag;
    } abstract;
    int lval;
    const char *opname;
  

#line 263 "cp-name-parser.c.tmp"

};
typedef union cp_name_parser_YYSTYPE cp_name_parser_YYSTYPE;
# define cp_name_parser_YYSTYPE_IS_TRIVIAL 1
# define cp_name_parser_YYSTYPE_IS_DECLARED 1
#endif




int yyparse (struct cpname_state *state);



/* Symbol kind.  */
enum cp_name_parser_yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_INT = 3,                        /* INT  */
  YYSYMBOL_FLOAT = 4,                      /* FLOAT  */
  YYSYMBOL_NAME = 5,                       /* NAME  */
  YYSYMBOL_STRUCT = 6,                     /* STRUCT  */
  YYSYMBOL_CLASS = 7,                      /* CLASS  */
  YYSYMBOL_UNION = 8,                      /* UNION  */
  YYSYMBOL_ENUM = 9,                       /* ENUM  */
  YYSYMBOL_SIZEOF = 10,                    /* SIZEOF  */
  YYSYMBOL_UNSIGNED = 11,                  /* UNSIGNED  */
  YYSYMBOL_COLONCOLON = 12,                /* COLONCOLON  */
  YYSYMBOL_TEMPLATE = 13,                  /* TEMPLATE  */
  YYSYMBOL_ERROR = 14,                     /* ERROR  */
  YYSYMBOL_NEW = 15,                       /* NEW  */
  YYSYMBOL_DELETE = 16,                    /* DELETE  */
  YYSYMBOL_OPERATOR = 17,                  /* OPERATOR  */
  YYSYMBOL_STATIC_CAST = 18,               /* STATIC_CAST  */
  YYSYMBOL_REINTERPRET_CAST = 19,          /* REINTERPRET_CAST  */
  YYSYMBOL_DYNAMIC_CAST = 20,              /* DYNAMIC_CAST  */
  YYSYMBOL_SIGNED_KEYWORD = 21,            /* SIGNED_KEYWORD  */
  YYSYMBOL_LONG = 22,                      /* LONG  */
  YYSYMBOL_SHORT = 23,                     /* SHORT  */
  YYSYMBOL_INT_KEYWORD = 24,               /* INT_KEYWORD  */
  YYSYMBOL_CONST_KEYWORD = 25,             /* CONST_KEYWORD  */
  YYSYMBOL_VOLATILE_KEYWORD = 26,          /* VOLATILE_KEYWORD  */
  YYSYMBOL_DOUBLE_KEYWORD = 27,            /* DOUBLE_KEYWORD  */
  YYSYMBOL_BOOL = 28,                      /* BOOL  */
  YYSYMBOL_ELLIPSIS = 29,                  /* ELLIPSIS  */
  YYSYMBOL_RESTRICT = 30,                  /* RESTRICT  */
  YYSYMBOL_VOID = 31,                      /* VOID  */
  YYSYMBOL_FLOAT_KEYWORD = 32,             /* FLOAT_KEYWORD  */
  YYSYMBOL_CHAR = 33,                      /* CHAR  */
  YYSYMBOL_WCHAR_T = 34,                   /* WCHAR_T  */
  YYSYMBOL_ASSIGN_MODIFY = 35,             /* ASSIGN_MODIFY  */
  YYSYMBOL_TRUEKEYWORD = 36,               /* TRUEKEYWORD  */
  YYSYMBOL_FALSEKEYWORD = 37,              /* FALSEKEYWORD  */
  YYSYMBOL_DEMANGLER_SPECIAL = 38,         /* DEMANGLER_SPECIAL  */
  YYSYMBOL_CONSTRUCTION_VTABLE = 39,       /* CONSTRUCTION_VTABLE  */
  YYSYMBOL_CONSTRUCTION_IN = 40,           /* CONSTRUCTION_IN  */
  YYSYMBOL_41_ = 41,                       /* ')'  */
  YYSYMBOL_42_ = 42,                       /* ','  */
  YYSYMBOL_43_ = 43,                       /* '='  */
  YYSYMBOL_44_ = 44,                       /* '?'  */
  YYSYMBOL_OROR = 45,                      /* OROR  */
  YYSYMBOL_ANDAND = 46,                    /* ANDAND  */
  YYSYMBOL_47_ = 47,                       /* '|'  */
  YYSYMBOL_48_ = 48,                       /* '^'  */
  YYSYMBOL_49_ = 49,                       /* '&'  */
  YYSYMBOL_EQUAL = 50,                     /* EQUAL  */
  YYSYMBOL_NOTEQUAL = 51,                  /* NOTEQUAL  */
  YYSYMBOL_52_ = 52,                       /* '<'  */
  YYSYMBOL_53_ = 53,                       /* '>'  */
  YYSYMBOL_LEQ = 54,                       /* LEQ  */
  YYSYMBOL_GEQ = 55,                       /* GEQ  */
  YYSYMBOL_SPACESHIP = 56,                 /* SPACESHIP  */
  YYSYMBOL_LSH = 57,                       /* LSH  */
  YYSYMBOL_RSH = 58,                       /* RSH  */
  YYSYMBOL_59_ = 59,                       /* '@'  */
  YYSYMBOL_60_ = 60,                       /* '+'  */
  YYSYMBOL_61_ = 61,                       /* '-'  */
  YYSYMBOL_62_ = 62,                       /* '*'  */
  YYSYMBOL_63_ = 63,                       /* '/'  */
  YYSYMBOL_64_ = 64,                       /* '%'  */
  YYSYMBOL_UNARY = 65,                     /* UNARY  */
  YYSYMBOL_INCREMENT = 66,                 /* INCREMENT  */
  YYSYMBOL_DECREMENT = 67,                 /* DECREMENT  */
  YYSYMBOL_ARROW = 68,                     /* ARROW  */
  YYSYMBOL_69_ = 69,                       /* '.'  */
  YYSYMBOL_70_ = 70,                       /* '['  */
  YYSYMBOL_71_ = 71,                       /* ']'  */
  YYSYMBOL_72_ = 72,                       /* '~'  */
  YYSYMBOL_73_ = 73,                       /* '!'  */
  YYSYMBOL_74_ = 74,                       /* '('  */
  YYSYMBOL_75_ = 75,                       /* ':'  */
  YYSYMBOL_YYACCEPT = 76,                  /* $accept  */
  YYSYMBOL_result = 77,                    /* result  */
  YYSYMBOL_start = 78,                     /* start  */
  YYSYMBOL_start_opt = 79,                 /* start_opt  */
  YYSYMBOL_function = 80,                  /* function  */
  YYSYMBOL_demangler_special = 81,         /* demangler_special  */
  YYSYMBOL_oper = 82,                      /* oper  */
  YYSYMBOL_conversion_op = 83,             /* conversion_op  */
  YYSYMBOL_conversion_op_name = 84,        /* conversion_op_name  */
  YYSYMBOL_unqualified_name = 85,          /* unqualified_name  */
  YYSYMBOL_colon_name = 86,                /* colon_name  */
  YYSYMBOL_name = 87,                      /* name  */
  YYSYMBOL_colon_ext_name = 88,            /* colon_ext_name  */
  YYSYMBOL_colon_ext_only = 89,            /* colon_ext_only  */
  YYSYMBOL_ext_only_name = 90,             /* ext_only_name  */
  YYSYMBOL_nested_name = 91,               /* nested_name  */
  YYSYMBOL_templ = 92,                     /* templ  */
  YYSYMBOL_template_params = 93,           /* template_params  */
  YYSYMBOL_template_arg = 94,              /* template_arg  */
  YYSYMBOL_function_args = 95,             /* function_args  */
  YYSYMBOL_function_arglist = 96,          /* function_arglist  */
  YYSYMBOL_qualifiers_opt = 97,            /* qualifiers_opt  */
  YYSYMBOL_qualifier = 98,                 /* qualifier  */
  YYSYMBOL_qualifiers = 99,                /* qualifiers  */
  YYSYMBOL_int_part = 100,                 /* int_part  */
  YYSYMBOL_int_seq = 101,                  /* int_seq  */
  YYSYMBOL_builtin_type = 102,             /* builtin_type  */
  YYSYMBOL_ptr_operator = 103,             /* ptr_operator  */
  YYSYMBOL_array_indicator = 104,          /* array_indicator  */
  YYSYMBOL_typespec_2 = 105,               /* typespec_2  */
  YYSYMBOL_abstract_declarator = 106,      /* abstract_declarator  */
  YYSYMBOL_direct_abstract_declarator = 107, /* direct_abstract_declarator  */
  YYSYMBOL_abstract_declarator_fn = 108,   /* abstract_declarator_fn  */
  YYSYMBOL_type = 109,                     /* type  */
  YYSYMBOL_declarator = 110,               /* declarator  */
  YYSYMBOL_direct_declarator = 111,        /* direct_declarator  */
  YYSYMBOL_declarator_1 = 112,             /* declarator_1  */
  YYSYMBOL_direct_declarator_1 = 113,      /* direct_declarator_1  */
  YYSYMBOL_exp = 114,                      /* exp  */
  YYSYMBOL_exp1 = 115                      /* exp1  */
};
typedef enum cp_name_parser_yysymbol_kind_t cp_name_parser_yysymbol_kind_t;


/* Second part of user prologue.  */
#line 74 "cp-name-parser.y"


struct cpname_state
{
  cpname_state (const char *input, demangle_parse_info *info)
    : lexptr (input),
      prev_lexptr (input),
      demangle_info (info)
  { }

  /* Un-push a character into the lexer.  This can only un-push the
     previous character in the input string.  */
  void unpush (char c)
  {
    gdb_assert (lexptr[-1] == c);
    --lexptr;
  }

  /* LEXPTR is the current pointer into our lex buffer.  PREV_LEXPTR
     is the start of the last token lexed, only used for diagnostics.
     ERROR_LEXPTR is the first place an error occurred.  GLOBAL_ERRMSG
     is the first error message encountered.  */

  const char *lexptr, *prev_lexptr;
  const char *error_lexptr = nullptr;
  const char *global_errmsg = nullptr;

  demangle_parse_info *demangle_info;

  /* The parse tree created by the parser is stored here after a
     successful parse.  */

  struct demangle_component *global_result = nullptr;

  struct demangle_component *d_grab ();

  /* Helper functions.  These wrap the demangler tree interface,
     handle allocation from our global store, and return the allocated
     component.  */

  struct demangle_component *fill_comp (enum demangle_component_type d_type,
					struct demangle_component *lhs,
					struct demangle_component *rhs);

  struct demangle_component *make_operator (const char *name, int args);

  struct demangle_component *make_dtor (enum gnu_v3_dtor_kinds kind,
					struct demangle_component *name);

  struct demangle_component *make_builtin_type (const char *name);

  struct demangle_component *make_name (const char *name, int len);

  struct demangle_component *d_qualify (struct demangle_component *lhs,
					int qualifiers, int is_method);

  struct demangle_component *d_int_type (int flags);

  struct demangle_component *d_unary (const char *name,
				      struct demangle_component *lhs);

  struct demangle_component *d_binary (const char *name,
				       struct demangle_component *lhs,
				       struct demangle_component *rhs);

  int parse_number (const char *p, int len, int parsed_float, cp_name_parser_YYSTYPE *lvalp);
};

struct demangle_component *
cpname_state::d_grab ()
{
  return obstack_new<demangle_component> (&demangle_info->obstack);
}

/* Flags passed to d_qualify.  */

#define QUAL_CONST 1
#define QUAL_RESTRICT 2
#define QUAL_VOLATILE 4

/* Flags passed to d_int_type.  */

#define INT_CHAR	(1 << 0)
#define INT_SHORT	(1 << 1)
#define INT_LONG	(1 << 2)
#define INT_LLONG	(1 << 3)

#define INT_SIGNED	(1 << 4)
#define INT_UNSIGNED	(1 << 5)

/* Helper functions.  These wrap the demangler tree interface, handle
   allocation from our global store, and return the allocated component.  */

struct demangle_component *
cpname_state::fill_comp (enum demangle_component_type d_type,
			 struct demangle_component *lhs,
			 struct demangle_component *rhs)
{
  struct demangle_component *ret = d_grab ();
  int i;

  i = cplus_demangle_fill_component (ret, d_type, lhs, rhs);
  gdb_assert (i);

  return ret;
}

struct demangle_component *
cpname_state::make_operator (const char *name, int args)
{
  struct demangle_component *ret = d_grab ();
  int i;

  i = cplus_demangle_fill_operator (ret, name, args);
  gdb_assert (i);

  return ret;
}

struct demangle_component *
cpname_state::make_dtor (enum gnu_v3_dtor_kinds kind,
			 struct demangle_component *name)
{
  struct demangle_component *ret = d_grab ();
  int i;

  i = cplus_demangle_fill_dtor (ret, kind, name);
  gdb_assert (i);

  return ret;
}

struct demangle_component *
cpname_state::make_builtin_type (const char *name)
{
  struct demangle_component *ret = d_grab ();
  int i;

  i = cplus_demangle_fill_builtin_type (ret, name);
  gdb_assert (i);

  return ret;
}

struct demangle_component *
cpname_state::make_name (const char *name, int len)
{
  struct demangle_component *ret = d_grab ();
  int i;

  i = cplus_demangle_fill_name (ret, name, len);
  gdb_assert (i);

  return ret;
}

#define d_left(dc) (dc)->u.s_binary.left
#define d_right(dc) (dc)->u.s_binary.right

static int yylex (cp_name_parser_YYSTYPE *, cpname_state *);
static void yyerror (cpname_state *, const char *);

#line 566 "cp-name-parser.c.tmp"


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
         || (defined cp_name_parser_YYSTYPE_IS_TRIVIAL && cp_name_parser_YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union cp_name_parser_yyalloc
{
  yy_state_t yyss_alloc;
  cp_name_parser_YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union cp_name_parser_yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (cp_name_parser_YYSTYPE)) \
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
#define YYFINAL  85
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1151

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  76
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  40
/* YYNRULES -- Number of rules.  */
#define YYNRULES  201
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  332

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   308


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (cp_name_parser_yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    73,     2,     2,     2,    64,    49,     2,
      74,    41,    62,    60,    42,    61,    69,    63,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    75,     2,
      52,    43,    53,    44,    59,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    70,     2,    71,    48,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    47,     2,    72,     2,     2,     2,
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
      35,    36,    37,    38,    39,    40,    45,    46,    50,    51,
      54,    55,    56,    57,    58,    65,    66,    67,    68
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   332,   332,   341,   343,   345,   350,   351,   358,   367,
     374,   377,   394,   397,   416,   418,   422,   428,   434,   440,
     446,   448,   450,   452,   454,   456,   458,   460,   462,   464,
     466,   468,   470,   472,   474,   476,   478,   480,   482,   484,
     486,   488,   490,   492,   494,   496,   498,   500,   502,   504,
     512,   517,   522,   526,   531,   539,   540,   542,   547,   559,
     560,   566,   568,   569,   571,   574,   575,   578,   579,   583,
     585,   588,   592,   597,   601,   610,   612,   619,   622,   633,
     634,   638,   640,   642,   643,   646,   650,   655,   660,   666,
     676,   680,   684,   692,   693,   696,   698,   700,   704,   705,
     712,   714,   716,   718,   720,   722,   726,   727,   731,   733,
     735,   737,   739,   741,   743,   747,   752,   755,   758,   764,
     772,   774,   788,   790,   791,   793,   796,   798,   799,   801,
     804,   806,   808,   810,   815,   818,   823,   830,   834,   845,
     851,   869,   872,   880,   882,   893,   900,   901,   907,   911,
     915,   917,   922,   927,   939,   943,   947,   955,   960,   969,
     973,   978,   983,   987,   993,   999,  1002,  1009,  1011,  1016,
    1020,  1024,  1031,  1047,  1054,  1061,  1080,  1084,  1088,  1092,
    1096,  1100,  1104,  1108,  1112,  1116,  1120,  1124,  1128,  1132,
    1136,  1140,  1144,  1148,  1153,  1157,  1161,  1168,  1172,  1175,
    1184,  1193
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (cp_name_parser_yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (cp_name_parser_yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "INT", "FLOAT", "NAME",
  "STRUCT", "CLASS", "UNION", "ENUM", "SIZEOF", "UNSIGNED", "COLONCOLON",
  "TEMPLATE", "ERROR", "NEW", "DELETE", "OPERATOR", "STATIC_CAST",
  "REINTERPRET_CAST", "DYNAMIC_CAST", "SIGNED_KEYWORD", "LONG", "SHORT",
  "INT_KEYWORD", "CONST_KEYWORD", "VOLATILE_KEYWORD", "DOUBLE_KEYWORD",
  "BOOL", "ELLIPSIS", "RESTRICT", "VOID", "FLOAT_KEYWORD", "CHAR",
  "WCHAR_T", "ASSIGN_MODIFY", "TRUEKEYWORD", "FALSEKEYWORD",
  "DEMANGLER_SPECIAL", "CONSTRUCTION_VTABLE", "CONSTRUCTION_IN", "')'",
  "','", "'='", "'?'", "OROR", "ANDAND", "'|'", "'^'", "'&'", "EQUAL",
  "NOTEQUAL", "'<'", "'>'", "LEQ", "GEQ", "SPACESHIP", "LSH", "RSH", "'@'",
  "'+'", "'-'", "'*'", "'/'", "'%'", "UNARY", "INCREMENT", "DECREMENT",
  "ARROW", "'.'", "'['", "']'", "'~'", "'!'", "'('", "':'", "$accept",
  "result", "start", "start_opt", "function", "demangler_special", "oper",
  "conversion_op", "conversion_op_name", "unqualified_name", "colon_name",
  "name", "colon_ext_name", "colon_ext_only", "ext_only_name",
  "nested_name", "templ", "template_params", "template_arg",
  "function_args", "function_arglist", "qualifiers_opt", "qualifier",
  "qualifiers", "int_part", "int_seq", "builtin_type", "ptr_operator",
  "array_indicator", "typespec_2", "abstract_declarator",
  "direct_abstract_declarator", "abstract_declarator_fn", "type",
  "declarator", "direct_declarator", "declarator_1", "direct_declarator_1",
  "exp", "exp1", YY_NULLPTRPTR
};

static const char *
yysymbol_name (cp_name_parser_yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-218)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     295,    -2,  -218,    52,   558,  -218,   -11,  -218,  -218,  -218,
    -218,  -218,  -218,  -218,  -218,  -218,  -218,  -218,   295,   295,
      18,    38,  -218,  -218,  -218,     1,  -218,    13,  -218,   117,
     -10,  -218,    81,    43,   117,   937,  -218,   367,   117,   334,
    -218,  -218,   338,  -218,   117,  -218,    81,    58,     0,    39,
    -218,  -218,  -218,  -218,  -218,  -218,  -218,  -218,  -218,  -218,
    -218,  -218,  -218,  -218,  -218,  -218,  -218,  -218,  -218,  -218,
    -218,  -218,  -218,  -218,     4,    46,  -218,  -218,    71,    66,
    -218,  -218,  -218,    85,  -218,  -218,   338,    -2,   295,  -218,
    -218,   117,     5,   622,  -218,     6,    43,   122,   497,  -218,
      80,  -218,  -218,   825,   122,     9,  -218,  -218,   139,  -218,
    -218,    58,   117,   117,  -218,  -218,  -218,   121,   761,   622,
    -218,  -218,    80,  -218,    17,   122,   716,  -218,    80,  -218,
      80,  -218,  -218,    89,   125,   127,   131,  -218,  -218,   655,
     504,   460,   504,   420,  -218,     7,  -218,   334,   954,  -218,
    -218,    98,   102,  -218,  -218,  -218,   295,    99,  -218,    22,
    -218,  -218,   119,  -218,    58,   161,   117,   498,    10,   -12,
     498,   498,   162,     9,   117,   139,   295,  -218,   199,  -218,
     193,  -218,  -218,  -218,  -218,   117,  -218,  -218,  -218,    41,
     725,   197,  -218,  -218,   498,  -218,  -218,  -218,   200,  -218,
     913,   913,   913,   913,   295,  -218,   504,    91,    91,    91,
     686,   498,   170,   928,   172,   338,  -218,  -218,  -218,   504,
     504,   504,   504,   504,   504,   504,   504,   504,   504,   504,
     504,   504,   504,   504,   504,   504,   504,   504,   228,   229,
    -218,  -218,  -218,  -218,  -218,   117,  -218,    30,   117,  -218,
     117,   883,  -218,  -218,  -218,    35,   295,  -218,   725,  -218,
     725,   203,    80,   295,   295,   204,   194,   196,   205,   211,
     295,  -218,   504,   504,  -218,  -218,   823,   978,  1001,  1023,
    1044,  1064,  1082,  1082,   493,   493,   493,   493,   683,   683,
     218,   218,    91,    91,    91,  -218,  -218,  -218,  -218,  -218,
    -218,   498,  -218,   219,  -218,  -218,  -218,  -218,  -218,  -218,
    -218,   185,   195,   209,  -218,   232,    91,   954,   504,  -218,
    -218,   464,   464,   464,  -218,   954,   233,   243,   247,  -218,
    -218,  -218
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    62,   102,     0,     0,   101,   104,   105,   100,    97,
      96,   110,   112,    95,   114,   109,   103,   113,     0,     0,
       0,     0,     2,     5,     4,    55,    52,     6,    70,   127,
      11,    67,     0,    64,    98,     0,   106,   108,   123,   146,
       3,    71,     0,    54,   131,    68,     0,     0,    16,    17,
      33,    45,    30,    42,    41,    27,    25,    26,    36,    37,
      31,    32,    38,    39,    40,    34,    35,    20,    21,    22,
      23,    24,    43,    44,    47,     0,    28,    29,     0,     0,
      50,   111,    14,     0,    58,     1,     0,     0,     0,   117,
     116,    93,     0,     0,    12,     0,     0,     6,   141,   140,
     143,    13,   126,     0,     6,    61,    51,    69,    63,    73,
      99,     0,   129,   125,   104,   107,   122,     0,     0,     0,
      65,    59,   155,    66,     0,     6,   134,   147,   136,     8,
     156,   197,   198,     0,     0,     0,     0,   200,   201,     0,
       0,     0,     0,     0,    84,     0,    77,    79,    83,   130,
      53,     0,     0,    46,    49,    48,     0,     0,     7,     0,
     115,    94,     0,   120,     0,   114,    93,     0,     0,     0,
     134,    85,     0,     0,    93,     0,     0,   145,     0,   142,
     138,   139,    10,    72,    74,   133,   128,   124,    60,     0,
     134,   162,   163,     9,     0,   135,   154,   138,   160,   161,
       0,     0,     0,     0,     0,    81,     0,   169,   171,   170,
       0,   146,     0,   165,     0,     0,    75,    76,    80,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      18,    19,    15,    56,    57,    93,   121,     0,    93,    92,
      93,     0,    86,   137,   118,     0,     0,   132,     0,   153,
     134,     0,   149,     0,     0,     0,     0,     0,     0,     0,
       0,   167,     0,     0,   164,    78,     0,   193,   192,   191,
     190,   189,   183,   184,   188,   185,   186,   187,   181,   182,
     179,   180,   176,   177,   178,   194,   195,   119,    91,    90,
      89,    87,   144,     0,   148,   159,   151,   152,   157,   158,
     199,     0,     0,     0,    82,     0,   172,   166,     0,    88,
     150,     0,     0,     0,   168,   196,     0,     0,     0,   173,
     175,   174
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -218,  -218,    33,    14,   -39,  -218,  -218,    -1,  -218,    -4,
    -218,   145,  -178,   -19,     2,    -3,    83,   160,    75,  -218,
     -26,  -157,  -218,   241,   254,  -218,   258,   -20,   -74,    63,
     -25,   -21,   201,   -70,  -217,  -218,   169,  -218,    -5,  -127
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    21,   158,    94,    23,    24,    25,    26,    27,    28,
     120,    29,   122,    30,    31,    32,    33,   145,   146,   169,
      97,   160,    34,    35,    36,    37,    38,   170,    99,    39,
     172,   128,   101,    40,   261,   262,   129,   130,   213,   214
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      46,    79,    43,   144,   104,    45,   100,    98,   162,   249,
      41,   173,   259,   125,   127,   105,    81,   254,    87,   126,
     123,   183,   105,    84,    95,    88,   181,   105,   107,   250,
     251,   106,    79,    22,   118,   105,   124,   148,    85,     4,
     173,   303,   107,   304,    79,   150,   105,   144,   192,   215,
      42,    82,    83,    86,   181,   109,   199,     1,   118,    89,
     216,    42,    90,     1,   103,   217,   153,    80,   174,     4,
     151,   105,   174,   212,   180,    91,   163,   100,    98,   174,
     259,   148,   259,    92,   245,   159,   105,    93,   297,    20,
     168,   298,   245,   299,    20,    95,   191,   245,     4,   190,
      79,   195,   197,   245,   198,   147,   126,   123,    79,   152,
      96,   177,   155,    20,   189,   108,   168,   154,   182,    45,
     107,   125,   218,   124,    20,   156,     1,   126,   123,   108,
     265,   266,   267,   268,   176,   207,   208,   209,   118,   193,
      79,   215,     9,    10,   124,   195,   252,    13,    44,   147,
      92,   184,   243,    20,   103,   107,   171,   244,   106,   238,
     239,   247,   108,   200,    95,   195,   171,    95,    95,   240,
     260,   123,   205,   241,   190,   255,   144,   201,   175,   202,
     112,    96,   171,   203,   121,   107,   127,   124,   307,   242,
     246,    95,    44,    20,   326,   327,   328,    79,    79,    79,
      79,   208,   248,   253,    87,   256,   211,   108,    95,   263,
     148,   272,   264,   274,   276,   277,   278,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   294,   295,   296,   195,   306,   269,   260,   123,
     260,   123,   108,   271,   305,   310,   157,   311,    79,   312,
      96,   108,   314,    96,    96,   124,   185,   124,   313,   321,
     320,    96,   188,   211,   211,   211,   211,   316,   317,   322,
     102,   121,   108,   324,   329,   110,   319,    96,   147,   116,
     235,   236,   237,   323,   330,   149,   238,   239,   331,   302,
     275,   115,   121,   113,    96,   196,   308,   309,    95,   179,
       1,     0,     0,   315,     0,     0,     2,     3,     0,    44,
       0,     0,     4,   325,   301,     0,     5,     6,     7,     8,
       9,    10,    11,    12,     0,    13,    14,    15,    16,    17,
     108,     0,   161,    18,    19,   121,     0,     0,   175,     1,
       0,   131,   132,     1,     0,     0,   117,     0,   133,     2,
       3,   118,     0,   186,   187,     4,   134,   135,   136,     5,
       6,     7,     8,     9,    10,    11,    12,    20,    13,    14,
      15,    16,    17,     0,   137,   138,     0,     0,     2,     0,
      89,     0,     0,    90,    96,     0,     0,   139,     5,   114,
       7,     8,     0,     0,     0,     0,    91,     0,     0,   140,
      16,     0,     0,   121,    92,   121,    20,   161,   119,     0,
     141,   142,   143,     0,     0,   161,     0,     0,     0,     0,
       0,     0,     0,   131,   132,     1,   257,     0,     0,     0,
     133,     2,    47,     0,     0,     0,     0,     0,   134,   135,
     136,     5,     6,     7,     8,     9,    10,    11,    12,     0,
      13,    14,    15,    16,    17,     0,   137,   138,     0,     0,
       0,     0,     0,   131,   132,    84,     0,   131,   132,   210,
     133,     0,     0,     0,   133,     0,     0,     0,   134,   135,
     136,   140,   134,   135,   136,     0,   161,     0,     0,   161,
       0,   161,   206,   142,   143,     0,   137,   138,     0,     0,
     137,   138,    87,    87,     0,     0,     0,   131,   132,   178,
     178,     0,     0,   210,   133,     0,     0,     0,     0,     0,
       0,   140,   134,   135,   136,   140,     0,     0,     0,     0,
       0,     0,   206,   142,   143,     0,   206,   142,   143,     0,
     137,   138,     0,    89,    89,     0,    90,    90,     0,     0,
     231,   232,     0,   233,   234,   235,   236,   237,     0,    91,
      91,   238,   239,     1,     0,   140,     0,    92,    92,     2,
      47,    93,   167,    48,    49,     0,   206,   142,   143,     5,
       6,     7,     8,     9,    10,    11,    12,     0,    13,    14,
      15,    16,    17,    50,     0,     0,     0,     0,     0,     0,
      51,    52,     0,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,     0,    67,    68,
      69,    70,    71,     0,    72,    73,    74,     1,    75,     0,
      76,    77,    78,     2,   164,     0,     0,     0,     0,     0,
       0,     0,     0,     5,     6,     7,     8,     9,    10,    11,
      12,     0,    13,   165,    15,    16,    17,     0,     0,     0,
       1,     0,     0,   166,     0,     0,     2,     3,    89,     0,
       0,    90,     4,     0,     0,     0,     5,     6,     7,     8,
       9,    10,    11,    12,    91,    13,    14,    15,    16,    17,
       0,     1,    92,    18,    19,     0,   167,     2,     3,     0,
       0,     0,     0,     4,     0,     0,     0,     5,     6,     7,
       8,     9,    10,    11,    12,     0,    13,    14,    15,    16,
      17,     1,     0,     0,    18,    19,     0,    20,   117,   204,
       1,     0,     0,   118,     0,     0,     0,   117,     0,     0,
       0,     0,   118,   233,   234,   235,   236,   237,     0,     0,
       0,   238,   239,     0,     0,     0,     0,     0,    20,     0,
     270,     0,    89,     0,     0,    90,     0,     0,     0,     0,
       0,    89,     0,     0,    90,     0,    48,    49,    91,     0,
       0,     0,     0,     0,     0,     0,    92,    91,    20,     0,
     194,     0,     0,     0,     0,    92,    50,    20,     0,   258,
       0,     0,     0,    51,    52,     0,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
       0,    67,    68,    69,    70,    71,     0,    72,    73,    74,
       1,    75,     0,    76,    77,    78,     2,    47,     0,     0,
       0,     0,     0,     0,     0,     0,     5,     6,     7,     8,
       9,    10,    11,    12,     0,    13,   165,    15,    16,    17,
       0,     0,     0,     0,     0,     0,   166,   219,   220,   221,
     222,   223,   224,   225,   226,   227,     0,   228,   229,   230,
     231,   232,     0,   233,   234,   235,   236,   237,     1,     0,
       0,   238,   239,     0,     2,    47,     0,     0,   318,     0,
       0,     0,     0,     0,     5,     6,     7,     8,     9,    10,
      11,    12,   300,    13,    14,    15,    16,    17,     1,     0,
       0,     0,     0,     0,     2,    47,     0,     0,     0,     0,
       0,     0,     0,     0,     5,     6,     7,     8,     9,    10,
      11,    12,     1,    13,    14,    15,    16,    17,     2,   111,
       0,     0,     0,     0,     0,     0,     0,     0,     5,     6,
       7,     8,     0,     0,    11,    12,     0,     0,    14,    15,
      16,    17,   219,   220,   221,   222,   223,   224,   225,   226,
     227,   273,   228,   229,   230,   231,   232,     0,   233,   234,
     235,   236,   237,     0,     0,     0,   238,   239,   219,   220,
     221,   222,   223,   224,   225,   226,   227,     0,   228,   229,
     230,   231,   232,     0,   233,   234,   235,   236,   237,     0,
       0,     0,   238,   239,   221,   222,   223,   224,   225,   226,
     227,     0,   228,   229,   230,   231,   232,     0,   233,   234,
     235,   236,   237,     0,     0,     0,   238,   239,   222,   223,
     224,   225,   226,   227,     0,   228,   229,   230,   231,   232,
       0,   233,   234,   235,   236,   237,     0,     0,     0,   238,
     239,   223,   224,   225,   226,   227,     0,   228,   229,   230,
     231,   232,     0,   233,   234,   235,   236,   237,     0,     0,
       0,   238,   239,   224,   225,   226,   227,     0,   228,   229,
     230,   231,   232,     0,   233,   234,   235,   236,   237,     0,
       0,     0,   238,   239,   225,   226,   227,     0,   228,   229,
     230,   231,   232,     0,   233,   234,   235,   236,   237,     0,
       0,     0,   238,   239,   227,     0,   228,   229,   230,   231,
     232,     0,   233,   234,   235,   236,   237,     0,     0,     0,
     238,   239
};

static const yytype_int16 yycheck[] =
{
       3,     4,     3,    42,    30,     3,    27,    27,     3,   166,
      12,     5,   190,    39,    39,     5,    27,   174,     5,    39,
      39,    12,     5,     5,    27,    12,   100,     5,    32,    41,
      42,    32,    35,     0,    17,     5,    39,    42,     0,    17,
       5,   258,    46,   260,    47,    46,     5,    86,   122,    42,
      52,    18,    19,    52,   128,    12,   130,     5,    17,    46,
      53,    52,    49,     5,    74,    58,    62,     4,    62,    17,
      70,     5,    62,   143,   100,    62,    71,    98,    98,    62,
     258,    86,   260,    70,    62,    88,     5,    74,   245,    72,
      93,   248,    62,   250,    72,    98,   122,    62,    17,   119,
     103,   126,   128,    62,   130,    42,   126,   126,   111,    70,
      27,    97,    41,    72,   117,    32,   119,    71,   104,   117,
     124,   147,   147,   126,    72,    40,     5,   147,   147,    46,
     200,   201,   202,   203,    12,   140,   141,   142,    17,   125,
     143,    42,    25,    26,   147,   170,   171,    30,     3,    86,
      70,    12,    53,    72,    74,   159,    93,    58,   159,    68,
      69,   164,    79,    74,   167,   190,   103,   170,   171,    71,
     190,   190,   139,    71,   194,   178,   215,    52,    95,    52,
      35,    98,   119,    52,    39,   189,   211,   190,   262,   156,
      71,   194,    47,    72,   321,   322,   323,   200,   201,   202,
     203,   206,    41,    41,     5,    12,   143,   124,   211,    12,
     215,    41,    12,    41,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,   229,   230,   231,   232,   233,   234,
     235,   236,   237,     5,     5,   260,   262,   204,   258,   258,
     260,   260,   159,   210,    41,    41,    86,    53,   251,    53,
     167,   168,    41,   170,   171,   258,   111,   260,    53,    74,
      41,   178,   117,   200,   201,   202,   203,   272,   273,    74,
      29,   126,   189,    41,    41,    34,   301,   194,   215,    38,
      62,    63,    64,    74,    41,    44,    68,    69,    41,   256,
     215,    37,   147,    35,   211,   126,   263,   264,   301,    98,
       5,    -1,    -1,   270,    -1,    -1,    11,    12,    -1,   164,
      -1,    -1,    17,   318,   251,    -1,    21,    22,    23,    24,
      25,    26,    27,    28,    -1,    30,    31,    32,    33,    34,
     247,    -1,    91,    38,    39,   190,    -1,    -1,   255,     5,
      -1,     3,     4,     5,    -1,    -1,    12,    -1,    10,    11,
      12,    17,    -1,   112,   113,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    72,    30,    31,
      32,    33,    34,    -1,    36,    37,    -1,    -1,    11,    -1,
      46,    -1,    -1,    49,   301,    -1,    -1,    49,    21,    22,
      23,    24,    -1,    -1,    -1,    -1,    62,    -1,    -1,    61,
      33,    -1,    -1,   258,    70,   260,    72,   166,    74,    -1,
      72,    73,    74,    -1,    -1,   174,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     3,     4,     5,   185,    -1,    -1,    -1,
      10,    11,    12,    -1,    -1,    -1,    -1,    -1,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    -1,
      30,    31,    32,    33,    34,    -1,    36,    37,    -1,    -1,
      -1,    -1,    -1,     3,     4,     5,    -1,     3,     4,    49,
      10,    -1,    -1,    -1,    10,    -1,    -1,    -1,    18,    19,
      20,    61,    18,    19,    20,    -1,   245,    -1,    -1,   248,
      -1,   250,    72,    73,    74,    -1,    36,    37,    -1,    -1,
      36,    37,     5,     5,    -1,    -1,    -1,     3,     4,    12,
      12,    -1,    -1,    49,    10,    -1,    -1,    -1,    -1,    -1,
      -1,    61,    18,    19,    20,    61,    -1,    -1,    -1,    -1,
      -1,    -1,    72,    73,    74,    -1,    72,    73,    74,    -1,
      36,    37,    -1,    46,    46,    -1,    49,    49,    -1,    -1,
      57,    58,    -1,    60,    61,    62,    63,    64,    -1,    62,
      62,    68,    69,     5,    -1,    61,    -1,    70,    70,    11,
      12,    74,    74,    15,    16,    -1,    72,    73,    74,    21,
      22,    23,    24,    25,    26,    27,    28,    -1,    30,    31,
      32,    33,    34,    35,    -1,    -1,    -1,    -1,    -1,    -1,
      42,    43,    -1,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    -1,    60,    61,
      62,    63,    64,    -1,    66,    67,    68,     5,    70,    -1,
      72,    73,    74,    11,    12,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    21,    22,    23,    24,    25,    26,    27,
      28,    -1,    30,    31,    32,    33,    34,    -1,    -1,    -1,
       5,    -1,    -1,    41,    -1,    -1,    11,    12,    46,    -1,
      -1,    49,    17,    -1,    -1,    -1,    21,    22,    23,    24,
      25,    26,    27,    28,    62,    30,    31,    32,    33,    34,
      -1,     5,    70,    38,    39,    -1,    74,    11,    12,    -1,
      -1,    -1,    -1,    17,    -1,    -1,    -1,    21,    22,    23,
      24,    25,    26,    27,    28,    -1,    30,    31,    32,    33,
      34,     5,    -1,    -1,    38,    39,    -1,    72,    12,    74,
       5,    -1,    -1,    17,    -1,    -1,    -1,    12,    -1,    -1,
      -1,    -1,    17,    60,    61,    62,    63,    64,    -1,    -1,
      -1,    68,    69,    -1,    -1,    -1,    -1,    -1,    72,    -1,
      74,    -1,    46,    -1,    -1,    49,    -1,    -1,    -1,    -1,
      -1,    46,    -1,    -1,    49,    -1,    15,    16,    62,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    70,    62,    72,    -1,
      74,    -1,    -1,    -1,    -1,    70,    35,    72,    -1,    74,
      -1,    -1,    -1,    42,    43,    -1,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      -1,    60,    61,    62,    63,    64,    -1,    66,    67,    68,
       5,    70,    -1,    72,    73,    74,    11,    12,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    23,    24,
      25,    26,    27,    28,    -1,    30,    31,    32,    33,    34,
      -1,    -1,    -1,    -1,    -1,    -1,    41,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    -1,    54,    55,    56,
      57,    58,    -1,    60,    61,    62,    63,    64,     5,    -1,
      -1,    68,    69,    -1,    11,    12,    -1,    -1,    75,    -1,
      -1,    -1,    -1,    -1,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,     5,    -1,
      -1,    -1,    -1,    -1,    11,    12,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    21,    22,    23,    24,    25,    26,
      27,    28,     5,    30,    31,    32,    33,    34,    11,    12,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      23,    24,    -1,    -1,    27,    28,    -1,    -1,    31,    32,
      33,    34,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    -1,    60,    61,
      62,    63,    64,    -1,    -1,    -1,    68,    69,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    -1,    54,    55,
      56,    57,    58,    -1,    60,    61,    62,    63,    64,    -1,
      -1,    -1,    68,    69,    46,    47,    48,    49,    50,    51,
      52,    -1,    54,    55,    56,    57,    58,    -1,    60,    61,
      62,    63,    64,    -1,    -1,    -1,    68,    69,    47,    48,
      49,    50,    51,    52,    -1,    54,    55,    56,    57,    58,
      -1,    60,    61,    62,    63,    64,    -1,    -1,    -1,    68,
      69,    48,    49,    50,    51,    52,    -1,    54,    55,    56,
      57,    58,    -1,    60,    61,    62,    63,    64,    -1,    -1,
      -1,    68,    69,    49,    50,    51,    52,    -1,    54,    55,
      56,    57,    58,    -1,    60,    61,    62,    63,    64,    -1,
      -1,    -1,    68,    69,    50,    51,    52,    -1,    54,    55,
      56,    57,    58,    -1,    60,    61,    62,    63,    64,    -1,
      -1,    -1,    68,    69,    52,    -1,    54,    55,    56,    57,
      58,    -1,    60,    61,    62,    63,    64,    -1,    -1,    -1,
      68,    69
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     5,    11,    12,    17,    21,    22,    23,    24,    25,
      26,    27,    28,    30,    31,    32,    33,    34,    38,    39,
      72,    77,    78,    80,    81,    82,    83,    84,    85,    87,
      89,    90,    91,    92,    98,    99,   100,   101,   102,   105,
     109,    12,    52,    83,    87,    90,    91,    12,    15,    16,
      35,    42,    43,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    60,    61,    62,
      63,    64,    66,    67,    68,    70,    72,    73,    74,    91,
     105,    27,    78,    78,     5,     0,    52,     5,    12,    46,
      49,    62,    70,    74,    79,    91,    92,    96,   103,   104,
     107,   108,    99,    74,    96,     5,    83,    85,    92,    12,
      99,    12,    87,   102,    22,   100,    99,    12,    17,    74,
      86,    87,    88,    89,    91,    96,   103,   106,   107,   112,
     113,     3,     4,    10,    18,    19,    20,    36,    37,    49,
      61,    72,    73,    74,    80,    93,    94,   105,   114,    99,
      83,    70,    70,    62,    71,    41,    40,    93,    78,    91,
      97,    99,     3,    71,    12,    31,    41,    74,    91,    95,
     103,   105,   106,     5,    62,    92,    12,    79,    12,   108,
      96,   104,    79,    12,    12,    87,    99,    99,    87,    91,
     103,    96,   104,    79,    74,   106,   112,    96,    96,   104,
      74,    52,    52,    52,    74,    78,    72,   114,   114,   114,
      49,   105,   109,   114,   115,    42,    53,    58,   106,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    54,    55,
      56,    57,    58,    60,    61,    62,    63,    64,    68,    69,
      71,    71,    78,    53,    58,    62,    71,    91,    41,    97,
      41,    42,   106,    41,    97,    91,    12,    99,    74,    88,
     103,   110,   111,    12,    12,   109,   109,   109,   109,    78,
      74,    78,    41,    53,    41,    94,   114,   114,   114,   114,
     114,   114,   114,   114,   114,   114,   114,   114,   114,   114,
     114,   114,   114,   114,   114,     5,     5,    97,    97,    97,
      29,   105,    78,   110,   110,    41,    96,   104,    78,    78,
      41,    53,    53,    53,    41,    78,   114,   114,    75,   106,
      41,    74,    74,    74,    41,   114,   115,   115,   115,    41,
      41,    41
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    76,    77,    78,    78,    78,    79,    79,    80,    80,
      80,    80,    80,    80,    81,    81,    82,    82,    82,    82,
      82,    82,    82,    82,    82,    82,    82,    82,    82,    82,
      82,    82,    82,    82,    82,    82,    82,    82,    82,    82,
      82,    82,    82,    82,    82,    82,    82,    82,    82,    82,
      83,    84,    84,    84,    84,    85,    85,    85,    85,    86,
      86,    87,    87,    87,    87,    88,    88,    89,    89,    90,
      90,    91,    91,    91,    91,    92,    92,    93,    93,    94,
      94,    94,    94,    94,    94,    95,    95,    95,    95,    95,
      96,    96,    96,    97,    97,    98,    98,    98,    99,    99,
     100,   100,   100,   100,   100,   100,   101,   101,   102,   102,
     102,   102,   102,   102,   102,   103,   103,   103,   103,   103,
     104,   104,   105,   105,   105,   105,   105,   105,   105,   105,
     105,   105,   105,   105,   106,   106,   106,   107,   107,   107,
     107,   108,   108,   108,   108,   108,   109,   109,   110,   110,
     111,   111,   111,   111,   112,   112,   112,   112,   112,   113,
     113,   113,   113,   113,   114,   115,   115,   115,   115,   114,
     114,   114,   114,   114,   114,   114,   114,   114,   114,   114,
     114,   114,   114,   114,   114,   114,   114,   114,   114,   114,
     114,   114,   114,   114,   114,   114,   114,   114,   114,   114,
     114,   114
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     0,     2,     2,     3,
       3,     1,     2,     2,     2,     4,     2,     2,     4,     4,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     3,     2,     3,     3,
       2,     2,     1,     3,     2,     1,     4,     4,     2,     1,
       2,     2,     1,     2,     1,     1,     1,     1,     2,     2,
       1,     2,     3,     2,     3,     4,     4,     1,     3,     1,
       2,     2,     4,     1,     1,     1,     2,     3,     4,     3,
       4,     4,     3,     0,     1,     1,     1,     1,     1,     2,
       1,     1,     1,     1,     1,     1,     1,     2,     1,     1,
       1,     2,     1,     1,     1,     2,     1,     1,     3,     4,
       2,     3,     2,     1,     3,     2,     2,     1,     3,     2,
       3,     2,     4,     3,     1,     2,     1,     3,     2,     2,
       1,     1,     2,     1,     4,     2,     1,     2,     2,     1,
       3,     2,     2,     1,     2,     1,     1,     4,     4,     4,
       2,     2,     2,     2,     3,     1,     3,     2,     4,     2,
       2,     2,     4,     7,     7,     7,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     5,     1,     1,     4,
       1,     1
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
        yyerror (state, YY_("syntax error: cannot back up")); \
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
                  Kind, Value, state); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       cp_name_parser_yysymbol_kind_t yykind, cp_name_parser_YYSTYPE const * const yyvaluep, struct cpname_state *state)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (state);
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
                 cp_name_parser_yysymbol_kind_t yykind, cp_name_parser_YYSTYPE const * const yyvaluep, struct cpname_state *state)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep, state);
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
yy_reduce_print (yy_state_t *yyssp, cp_name_parser_YYSTYPE *yyvsp,
                 int yyrule, struct cpname_state *state)
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
                       &yyvsp[(yyi + 1) - (yynrhs)], state);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, state); \
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
            cp_name_parser_yysymbol_kind_t yykind, cp_name_parser_YYSTYPE *yyvaluep, struct cpname_state *state)
{
  YY_USE (yyvaluep);
  YY_USE (state);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}






/*----------.
| yyparse.  |
`----------*/

int
yyparse (struct cpname_state *state)
{
/* Lookahead token kind.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static cp_name_parser_YYSTYPE yyval_default;)
cp_name_parser_YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs = 0;

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
    cp_name_parser_YYSTYPE yyvsa[YYINITDEPTH];
    cp_name_parser_YYSTYPE *yyvs = yyvsa;
    cp_name_parser_YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  cp_name_parser_yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  cp_name_parser_YYSTYPE yyval;



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
        cp_name_parser_YYSTYPE *yyvs1 = yyvs;

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
        union cp_name_parser_yyalloc *yyptr =
          YY_CAST (union cp_name_parser_yyalloc *,
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
      yychar = yylex (&yylval, state);
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
  case 2: /* result: start  */
#line 333 "cp-name-parser.y"
                        {
			  state->global_result = (yyvsp[0].comp);

			  /* Avoid warning about "yynerrs" being unused.  */
			  (void) yynerrs;
			}
#line 1940 "cp-name-parser.c.tmp"
    break;

  case 6: /* start_opt: %empty  */
#line 350 "cp-name-parser.y"
                        { (yyval.comp) = NULL; }
#line 1946 "cp-name-parser.c.tmp"
    break;

  case 7: /* start_opt: COLONCOLON start  */
#line 352 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[0].comp); }
#line 1952 "cp-name-parser.c.tmp"
    break;

  case 8: /* function: typespec_2 declarator_1  */
#line 359 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[0].nested).comp;
			  *(yyvsp[0].nested).last = (yyvsp[-1].comp);
			}
#line 1960 "cp-name-parser.c.tmp"
    break;

  case 9: /* function: typespec_2 function_arglist start_opt  */
#line 368 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME,
					  (yyvsp[-2].comp), (yyvsp[-1].nested).comp);
			  if ((yyvsp[0].comp))
			    (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME,
						   (yyval.comp), (yyvsp[0].comp));
			}
#line 1971 "cp-name-parser.c.tmp"
    break;

  case 10: /* function: colon_ext_only function_arglist start_opt  */
#line 375 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, (yyvsp[-2].comp), (yyvsp[-1].nested).comp);
			  if ((yyvsp[0].comp)) (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME, (yyval.comp), (yyvsp[0].comp)); }
#line 1978 "cp-name-parser.c.tmp"
    break;

  case 11: /* function: colon_ext_only  */
#line 378 "cp-name-parser.y"
                        {
			  /* This production is a hack to handle
			     something like "name::operator new[]" --
			     without arguments, this ordinarily would
			     not parse, but canonicalizing it is
			     important.  So we infer the "()" and then
			     remove it when converting back to string.
			     Note that this works because this
			     production is terminal.  */
			  demangle_component *comp
			    = state->fill_comp (DEMANGLE_COMPONENT_FUNCTION_TYPE,
						nullptr, nullptr);
			  (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, (yyvsp[0].comp), comp);
			  state->demangle_info->added_parens = true;
			}
#line 1998 "cp-name-parser.c.tmp"
    break;

  case 12: /* function: conversion_op_name start_opt  */
#line 395 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[-1].nested).comp;
			  if ((yyvsp[0].comp)) (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME, (yyval.comp), (yyvsp[0].comp)); }
#line 2005 "cp-name-parser.c.tmp"
    break;

  case 13: /* function: conversion_op_name abstract_declarator_fn  */
#line 398 "cp-name-parser.y"
                        { if ((yyvsp[0].abstract).last)
			    {
			       /* First complete the abstract_declarator's type using
				  the typespec from the conversion_op_name.  */
			      *(yyvsp[0].abstract).last = *(yyvsp[-1].nested).last;
			      /* Then complete the conversion_op_name with the type.  */
			      *(yyvsp[-1].nested).last = (yyvsp[0].abstract).comp;
			    }
			  /* If we have an arglist, build a function type.  */
			  if ((yyvsp[0].abstract).fn.comp)
			    (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, (yyvsp[-1].nested).comp, (yyvsp[0].abstract).fn.comp);
			  else
			    (yyval.comp) = (yyvsp[-1].nested).comp;
			  if ((yyvsp[0].abstract).start) (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME, (yyval.comp), (yyvsp[0].abstract).start);
			}
#line 2025 "cp-name-parser.c.tmp"
    break;

  case 14: /* demangler_special: DEMANGLER_SPECIAL start  */
#line 417 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp ((enum demangle_component_type) (yyvsp[-1].lval), (yyvsp[0].comp), NULL); }
#line 2031 "cp-name-parser.c.tmp"
    break;

  case 15: /* demangler_special: CONSTRUCTION_VTABLE start CONSTRUCTION_IN start  */
#line 419 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_CONSTRUCTION_VTABLE, (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 2037 "cp-name-parser.c.tmp"
    break;

  case 16: /* oper: OPERATOR NEW  */
#line 423 "cp-name-parser.y"
                        {
			  /* Match the whitespacing of cplus_demangle_operators.
			     It would abort on unrecognized string otherwise.  */
			  (yyval.comp) = state->make_operator ("new", 3);
			}
#line 2047 "cp-name-parser.c.tmp"
    break;

  case 17: /* oper: OPERATOR DELETE  */
#line 429 "cp-name-parser.y"
                        {
			  /* Match the whitespacing of cplus_demangle_operators.
			     It would abort on unrecognized string otherwise.  */
			  (yyval.comp) = state->make_operator ("delete ", 1);
			}
#line 2057 "cp-name-parser.c.tmp"
    break;

  case 18: /* oper: OPERATOR NEW '[' ']'  */
#line 435 "cp-name-parser.y"
                        {
			  /* Match the whitespacing of cplus_demangle_operators.
			     It would abort on unrecognized string otherwise.  */
			  (yyval.comp) = state->make_operator ("new[]", 3);
			}
#line 2067 "cp-name-parser.c.tmp"
    break;

  case 19: /* oper: OPERATOR DELETE '[' ']'  */
#line 441 "cp-name-parser.y"
                        {
			  /* Match the whitespacing of cplus_demangle_operators.
			     It would abort on unrecognized string otherwise.  */
			  (yyval.comp) = state->make_operator ("delete[] ", 1);
			}
#line 2077 "cp-name-parser.c.tmp"
    break;

  case 20: /* oper: OPERATOR '+'  */
#line 447 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("+", 2); }
#line 2083 "cp-name-parser.c.tmp"
    break;

  case 21: /* oper: OPERATOR '-'  */
#line 449 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("-", 2); }
#line 2089 "cp-name-parser.c.tmp"
    break;

  case 22: /* oper: OPERATOR '*'  */
#line 451 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("*", 2); }
#line 2095 "cp-name-parser.c.tmp"
    break;

  case 23: /* oper: OPERATOR '/'  */
#line 453 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("/", 2); }
#line 2101 "cp-name-parser.c.tmp"
    break;

  case 24: /* oper: OPERATOR '%'  */
#line 455 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("%", 2); }
#line 2107 "cp-name-parser.c.tmp"
    break;

  case 25: /* oper: OPERATOR '^'  */
#line 457 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("^", 2); }
#line 2113 "cp-name-parser.c.tmp"
    break;

  case 26: /* oper: OPERATOR '&'  */
#line 459 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("&", 2); }
#line 2119 "cp-name-parser.c.tmp"
    break;

  case 27: /* oper: OPERATOR '|'  */
#line 461 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("|", 2); }
#line 2125 "cp-name-parser.c.tmp"
    break;

  case 28: /* oper: OPERATOR '~'  */
#line 463 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("~", 1); }
#line 2131 "cp-name-parser.c.tmp"
    break;

  case 29: /* oper: OPERATOR '!'  */
#line 465 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("!", 1); }
#line 2137 "cp-name-parser.c.tmp"
    break;

  case 30: /* oper: OPERATOR '='  */
#line 467 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("=", 2); }
#line 2143 "cp-name-parser.c.tmp"
    break;

  case 31: /* oper: OPERATOR '<'  */
#line 469 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("<", 2); }
#line 2149 "cp-name-parser.c.tmp"
    break;

  case 32: /* oper: OPERATOR '>'  */
#line 471 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator (">", 2); }
#line 2155 "cp-name-parser.c.tmp"
    break;

  case 33: /* oper: OPERATOR ASSIGN_MODIFY  */
#line 473 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ((yyvsp[0].opname), 2); }
#line 2161 "cp-name-parser.c.tmp"
    break;

  case 34: /* oper: OPERATOR LSH  */
#line 475 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("<<", 2); }
#line 2167 "cp-name-parser.c.tmp"
    break;

  case 35: /* oper: OPERATOR RSH  */
#line 477 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator (">>", 2); }
#line 2173 "cp-name-parser.c.tmp"
    break;

  case 36: /* oper: OPERATOR EQUAL  */
#line 479 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("==", 2); }
#line 2179 "cp-name-parser.c.tmp"
    break;

  case 37: /* oper: OPERATOR NOTEQUAL  */
#line 481 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("!=", 2); }
#line 2185 "cp-name-parser.c.tmp"
    break;

  case 38: /* oper: OPERATOR LEQ  */
#line 483 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("<=", 2); }
#line 2191 "cp-name-parser.c.tmp"
    break;

  case 39: /* oper: OPERATOR GEQ  */
#line 485 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator (">=", 2); }
#line 2197 "cp-name-parser.c.tmp"
    break;

  case 40: /* oper: OPERATOR SPACESHIP  */
#line 487 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("<=>", 2); }
#line 2203 "cp-name-parser.c.tmp"
    break;

  case 41: /* oper: OPERATOR ANDAND  */
#line 489 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("&&", 2); }
#line 2209 "cp-name-parser.c.tmp"
    break;

  case 42: /* oper: OPERATOR OROR  */
#line 491 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("||", 2); }
#line 2215 "cp-name-parser.c.tmp"
    break;

  case 43: /* oper: OPERATOR INCREMENT  */
#line 493 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("++", 1); }
#line 2221 "cp-name-parser.c.tmp"
    break;

  case 44: /* oper: OPERATOR DECREMENT  */
#line 495 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("--", 1); }
#line 2227 "cp-name-parser.c.tmp"
    break;

  case 45: /* oper: OPERATOR ','  */
#line 497 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator (",", 2); }
#line 2233 "cp-name-parser.c.tmp"
    break;

  case 46: /* oper: OPERATOR ARROW '*'  */
#line 499 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("->*", 2); }
#line 2239 "cp-name-parser.c.tmp"
    break;

  case 47: /* oper: OPERATOR ARROW  */
#line 501 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("->", 2); }
#line 2245 "cp-name-parser.c.tmp"
    break;

  case 48: /* oper: OPERATOR '(' ')'  */
#line 503 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("()", 2); }
#line 2251 "cp-name-parser.c.tmp"
    break;

  case 49: /* oper: OPERATOR '[' ']'  */
#line 505 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("[]", 2); }
#line 2257 "cp-name-parser.c.tmp"
    break;

  case 50: /* conversion_op: OPERATOR typespec_2  */
#line 513 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_CONVERSION, (yyvsp[0].comp), NULL); }
#line 2263 "cp-name-parser.c.tmp"
    break;

  case 51: /* conversion_op_name: nested_name conversion_op  */
#line 518 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-1].nested1).comp;
			  d_right ((yyvsp[-1].nested1).last) = (yyvsp[0].comp);
			  (yyval.nested).last = &d_left ((yyvsp[0].comp));
			}
#line 2272 "cp-name-parser.c.tmp"
    break;

  case 52: /* conversion_op_name: conversion_op  */
#line 523 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[0].comp);
			  (yyval.nested).last = &d_left ((yyvsp[0].comp));
			}
#line 2280 "cp-name-parser.c.tmp"
    break;

  case 53: /* conversion_op_name: COLONCOLON nested_name conversion_op  */
#line 527 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-1].nested1).comp;
			  d_right ((yyvsp[-1].nested1).last) = (yyvsp[0].comp);
			  (yyval.nested).last = &d_left ((yyvsp[0].comp));
			}
#line 2289 "cp-name-parser.c.tmp"
    break;

  case 54: /* conversion_op_name: COLONCOLON conversion_op  */
#line 532 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[0].comp);
			  (yyval.nested).last = &d_left ((yyvsp[0].comp));
			}
#line 2297 "cp-name-parser.c.tmp"
    break;

  case 56: /* unqualified_name: oper '<' template_params '>'  */
#line 541 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_TEMPLATE, (yyvsp[-3].comp), (yyvsp[-1].nested).comp); }
#line 2303 "cp-name-parser.c.tmp"
    break;

  case 57: /* unqualified_name: oper '<' template_params RSH  */
#line 543 "cp-name-parser.y"
                        {
			  (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_TEMPLATE, (yyvsp[-3].comp), (yyvsp[-1].nested).comp);
			  state->unpush ('>');
			}
#line 2312 "cp-name-parser.c.tmp"
    break;

  case 58: /* unqualified_name: '~' NAME  */
#line 548 "cp-name-parser.y"
                        { (yyval.comp) = state->make_dtor (gnu_v3_complete_object_dtor, (yyvsp[0].comp)); }
#line 2318 "cp-name-parser.c.tmp"
    break;

  case 60: /* colon_name: COLONCOLON name  */
#line 561 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[0].comp); }
#line 2324 "cp-name-parser.c.tmp"
    break;

  case 61: /* name: nested_name NAME  */
#line 567 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[-1].nested1).comp; d_right ((yyvsp[-1].nested1).last) = (yyvsp[0].comp); }
#line 2330 "cp-name-parser.c.tmp"
    break;

  case 63: /* name: nested_name templ  */
#line 570 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[-1].nested1).comp; d_right ((yyvsp[-1].nested1).last) = (yyvsp[0].comp); }
#line 2336 "cp-name-parser.c.tmp"
    break;

  case 68: /* colon_ext_only: COLONCOLON ext_only_name  */
#line 580 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[0].comp); }
#line 2342 "cp-name-parser.c.tmp"
    break;

  case 69: /* ext_only_name: nested_name unqualified_name  */
#line 584 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[-1].nested1).comp; d_right ((yyvsp[-1].nested1).last) = (yyvsp[0].comp); }
#line 2348 "cp-name-parser.c.tmp"
    break;

  case 71: /* nested_name: NAME COLONCOLON  */
#line 589 "cp-name-parser.y"
                        { (yyval.nested1).comp = state->fill_comp (DEMANGLE_COMPONENT_QUAL_NAME, (yyvsp[-1].comp), NULL);
			  (yyval.nested1).last = (yyval.nested1).comp;
			}
#line 2356 "cp-name-parser.c.tmp"
    break;

  case 72: /* nested_name: nested_name NAME COLONCOLON  */
#line 593 "cp-name-parser.y"
                        { (yyval.nested1).comp = (yyvsp[-2].nested1).comp;
			  d_right ((yyvsp[-2].nested1).last) = state->fill_comp (DEMANGLE_COMPONENT_QUAL_NAME, (yyvsp[-1].comp), NULL);
			  (yyval.nested1).last = d_right ((yyvsp[-2].nested1).last);
			}
#line 2365 "cp-name-parser.c.tmp"
    break;

  case 73: /* nested_name: templ COLONCOLON  */
#line 598 "cp-name-parser.y"
                        { (yyval.nested1).comp = state->fill_comp (DEMANGLE_COMPONENT_QUAL_NAME, (yyvsp[-1].comp), NULL);
			  (yyval.nested1).last = (yyval.nested1).comp;
			}
#line 2373 "cp-name-parser.c.tmp"
    break;

  case 74: /* nested_name: nested_name templ COLONCOLON  */
#line 602 "cp-name-parser.y"
                        { (yyval.nested1).comp = (yyvsp[-2].nested1).comp;
			  d_right ((yyvsp[-2].nested1).last) = state->fill_comp (DEMANGLE_COMPONENT_QUAL_NAME, (yyvsp[-1].comp), NULL);
			  (yyval.nested1).last = d_right ((yyvsp[-2].nested1).last);
			}
#line 2382 "cp-name-parser.c.tmp"
    break;

  case 75: /* templ: NAME '<' template_params '>'  */
#line 611 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_TEMPLATE, (yyvsp[-3].comp), (yyvsp[-1].nested).comp); }
#line 2388 "cp-name-parser.c.tmp"
    break;

  case 76: /* templ: NAME '<' template_params RSH  */
#line 613 "cp-name-parser.y"
                        {
			  (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_TEMPLATE, (yyvsp[-3].comp), (yyvsp[-1].nested).comp);
			  state->unpush ('>');
			}
#line 2397 "cp-name-parser.c.tmp"
    break;

  case 77: /* template_params: template_arg  */
#line 620 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_TEMPLATE_ARGLIST, (yyvsp[0].comp), NULL);
			(yyval.nested).last = &d_right ((yyval.nested).comp); }
#line 2404 "cp-name-parser.c.tmp"
    break;

  case 78: /* template_params: template_params ',' template_arg  */
#line 623 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-2].nested).comp;
			  *(yyvsp[-2].nested).last = state->fill_comp (DEMANGLE_COMPONENT_TEMPLATE_ARGLIST, (yyvsp[0].comp), NULL);
			  (yyval.nested).last = &d_right (*(yyvsp[-2].nested).last);
			}
#line 2413 "cp-name-parser.c.tmp"
    break;

  case 80: /* template_arg: typespec_2 abstract_declarator  */
#line 635 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[0].abstract).comp;
			  *(yyvsp[0].abstract).last = (yyvsp[-1].comp);
			}
#line 2421 "cp-name-parser.c.tmp"
    break;

  case 81: /* template_arg: '&' start  */
#line 639 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_UNARY, state->make_operator ("&", 1), (yyvsp[0].comp)); }
#line 2427 "cp-name-parser.c.tmp"
    break;

  case 82: /* template_arg: '&' '(' start ')'  */
#line 641 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_UNARY, state->make_operator ("&", 1), (yyvsp[-1].comp)); }
#line 2433 "cp-name-parser.c.tmp"
    break;

  case 85: /* function_args: typespec_2  */
#line 647 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_ARGLIST, (yyvsp[0].comp), NULL);
			  (yyval.nested).last = &d_right ((yyval.nested).comp);
			}
#line 2441 "cp-name-parser.c.tmp"
    break;

  case 86: /* function_args: typespec_2 abstract_declarator  */
#line 651 "cp-name-parser.y"
                        { *(yyvsp[0].abstract).last = (yyvsp[-1].comp);
			  (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_ARGLIST, (yyvsp[0].abstract).comp, NULL);
			  (yyval.nested).last = &d_right ((yyval.nested).comp);
			}
#line 2450 "cp-name-parser.c.tmp"
    break;

  case 87: /* function_args: function_args ',' typespec_2  */
#line 656 "cp-name-parser.y"
                        { *(yyvsp[-2].nested).last = state->fill_comp (DEMANGLE_COMPONENT_ARGLIST, (yyvsp[0].comp), NULL);
			  (yyval.nested).comp = (yyvsp[-2].nested).comp;
			  (yyval.nested).last = &d_right (*(yyvsp[-2].nested).last);
			}
#line 2459 "cp-name-parser.c.tmp"
    break;

  case 88: /* function_args: function_args ',' typespec_2 abstract_declarator  */
#line 661 "cp-name-parser.y"
                        { *(yyvsp[0].abstract).last = (yyvsp[-1].comp);
			  *(yyvsp[-3].nested).last = state->fill_comp (DEMANGLE_COMPONENT_ARGLIST, (yyvsp[0].abstract).comp, NULL);
			  (yyval.nested).comp = (yyvsp[-3].nested).comp;
			  (yyval.nested).last = &d_right (*(yyvsp[-3].nested).last);
			}
#line 2469 "cp-name-parser.c.tmp"
    break;

  case 89: /* function_args: function_args ',' ELLIPSIS  */
#line 667 "cp-name-parser.y"
                        { *(yyvsp[-2].nested).last
			    = state->fill_comp (DEMANGLE_COMPONENT_ARGLIST,
					   state->make_builtin_type ("..."),
					   NULL);
			  (yyval.nested).comp = (yyvsp[-2].nested).comp;
			  (yyval.nested).last = &d_right (*(yyvsp[-2].nested).last);
			}
#line 2481 "cp-name-parser.c.tmp"
    break;

  case 90: /* function_arglist: '(' function_args ')' qualifiers_opt  */
#line 677 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_FUNCTION_TYPE, NULL, (yyvsp[-2].nested).comp);
			  (yyval.nested).last = &d_left ((yyval.nested).comp);
			  (yyval.nested).comp = state->d_qualify ((yyval.nested).comp, (yyvsp[0].lval), 1); }
#line 2489 "cp-name-parser.c.tmp"
    break;

  case 91: /* function_arglist: '(' VOID ')' qualifiers_opt  */
#line 681 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_FUNCTION_TYPE, NULL, NULL);
			  (yyval.nested).last = &d_left ((yyval.nested).comp);
			  (yyval.nested).comp = state->d_qualify ((yyval.nested).comp, (yyvsp[0].lval), 1); }
#line 2497 "cp-name-parser.c.tmp"
    break;

  case 92: /* function_arglist: '(' ')' qualifiers_opt  */
#line 685 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_FUNCTION_TYPE, NULL, NULL);
			  (yyval.nested).last = &d_left ((yyval.nested).comp);
			  (yyval.nested).comp = state->d_qualify ((yyval.nested).comp, (yyvsp[0].lval), 1); }
#line 2505 "cp-name-parser.c.tmp"
    break;

  case 93: /* qualifiers_opt: %empty  */
#line 692 "cp-name-parser.y"
                        { (yyval.lval) = 0; }
#line 2511 "cp-name-parser.c.tmp"
    break;

  case 95: /* qualifier: RESTRICT  */
#line 697 "cp-name-parser.y"
                        { (yyval.lval) = QUAL_RESTRICT; }
#line 2517 "cp-name-parser.c.tmp"
    break;

  case 96: /* qualifier: VOLATILE_KEYWORD  */
#line 699 "cp-name-parser.y"
                        { (yyval.lval) = QUAL_VOLATILE; }
#line 2523 "cp-name-parser.c.tmp"
    break;

  case 97: /* qualifier: CONST_KEYWORD  */
#line 701 "cp-name-parser.y"
                        { (yyval.lval) = QUAL_CONST; }
#line 2529 "cp-name-parser.c.tmp"
    break;

  case 99: /* qualifiers: qualifier qualifiers  */
#line 706 "cp-name-parser.y"
                        { (yyval.lval) = (yyvsp[-1].lval) | (yyvsp[0].lval); }
#line 2535 "cp-name-parser.c.tmp"
    break;

  case 100: /* int_part: INT_KEYWORD  */
#line 713 "cp-name-parser.y"
                        { (yyval.lval) = 0; }
#line 2541 "cp-name-parser.c.tmp"
    break;

  case 101: /* int_part: SIGNED_KEYWORD  */
#line 715 "cp-name-parser.y"
                        { (yyval.lval) = INT_SIGNED; }
#line 2547 "cp-name-parser.c.tmp"
    break;

  case 102: /* int_part: UNSIGNED  */
#line 717 "cp-name-parser.y"
                        { (yyval.lval) = INT_UNSIGNED; }
#line 2553 "cp-name-parser.c.tmp"
    break;

  case 103: /* int_part: CHAR  */
#line 719 "cp-name-parser.y"
                        { (yyval.lval) = INT_CHAR; }
#line 2559 "cp-name-parser.c.tmp"
    break;

  case 104: /* int_part: LONG  */
#line 721 "cp-name-parser.y"
                        { (yyval.lval) = INT_LONG; }
#line 2565 "cp-name-parser.c.tmp"
    break;

  case 105: /* int_part: SHORT  */
#line 723 "cp-name-parser.y"
                        { (yyval.lval) = INT_SHORT; }
#line 2571 "cp-name-parser.c.tmp"
    break;

  case 107: /* int_seq: int_seq int_part  */
#line 728 "cp-name-parser.y"
                        { (yyval.lval) = (yyvsp[-1].lval) | (yyvsp[0].lval); if ((yyvsp[-1].lval) & (yyvsp[0].lval) & INT_LONG) (yyval.lval) = (yyvsp[-1].lval) | INT_LLONG; }
#line 2577 "cp-name-parser.c.tmp"
    break;

  case 108: /* builtin_type: int_seq  */
#line 732 "cp-name-parser.y"
                        { (yyval.comp) = state->d_int_type ((yyvsp[0].lval)); }
#line 2583 "cp-name-parser.c.tmp"
    break;

  case 109: /* builtin_type: FLOAT_KEYWORD  */
#line 734 "cp-name-parser.y"
                        { (yyval.comp) = state->make_builtin_type ("float"); }
#line 2589 "cp-name-parser.c.tmp"
    break;

  case 110: /* builtin_type: DOUBLE_KEYWORD  */
#line 736 "cp-name-parser.y"
                        { (yyval.comp) = state->make_builtin_type ("double"); }
#line 2595 "cp-name-parser.c.tmp"
    break;

  case 111: /* builtin_type: LONG DOUBLE_KEYWORD  */
#line 738 "cp-name-parser.y"
                        { (yyval.comp) = state->make_builtin_type ("long double"); }
#line 2601 "cp-name-parser.c.tmp"
    break;

  case 112: /* builtin_type: BOOL  */
#line 740 "cp-name-parser.y"
                        { (yyval.comp) = state->make_builtin_type ("bool"); }
#line 2607 "cp-name-parser.c.tmp"
    break;

  case 113: /* builtin_type: WCHAR_T  */
#line 742 "cp-name-parser.y"
                        { (yyval.comp) = state->make_builtin_type ("wchar_t"); }
#line 2613 "cp-name-parser.c.tmp"
    break;

  case 114: /* builtin_type: VOID  */
#line 744 "cp-name-parser.y"
                        { (yyval.comp) = state->make_builtin_type ("void"); }
#line 2619 "cp-name-parser.c.tmp"
    break;

  case 115: /* ptr_operator: '*' qualifiers_opt  */
#line 748 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_POINTER, NULL, NULL);
			  (yyval.nested).last = &d_left ((yyval.nested).comp);
			  (yyval.nested).comp = state->d_qualify ((yyval.nested).comp, (yyvsp[0].lval), 0); }
#line 2627 "cp-name-parser.c.tmp"
    break;

  case 116: /* ptr_operator: '&'  */
#line 753 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_REFERENCE, NULL, NULL);
			  (yyval.nested).last = &d_left ((yyval.nested).comp); }
#line 2634 "cp-name-parser.c.tmp"
    break;

  case 117: /* ptr_operator: ANDAND  */
#line 756 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_RVALUE_REFERENCE, NULL, NULL);
			  (yyval.nested).last = &d_left ((yyval.nested).comp); }
#line 2641 "cp-name-parser.c.tmp"
    break;

  case 118: /* ptr_operator: nested_name '*' qualifiers_opt  */
#line 759 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_PTRMEM_TYPE, (yyvsp[-2].nested1).comp, NULL);
			  /* Convert the innermost DEMANGLE_COMPONENT_QUAL_NAME to a DEMANGLE_COMPONENT_NAME.  */
			  *(yyvsp[-2].nested1).last = *d_left ((yyvsp[-2].nested1).last);
			  (yyval.nested).last = &d_right ((yyval.nested).comp);
			  (yyval.nested).comp = state->d_qualify ((yyval.nested).comp, (yyvsp[0].lval), 0); }
#line 2651 "cp-name-parser.c.tmp"
    break;

  case 119: /* ptr_operator: COLONCOLON nested_name '*' qualifiers_opt  */
#line 765 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_PTRMEM_TYPE, (yyvsp[-2].nested1).comp, NULL);
			  /* Convert the innermost DEMANGLE_COMPONENT_QUAL_NAME to a DEMANGLE_COMPONENT_NAME.  */
			  *(yyvsp[-2].nested1).last = *d_left ((yyvsp[-2].nested1).last);
			  (yyval.nested).last = &d_right ((yyval.nested).comp);
			  (yyval.nested).comp = state->d_qualify ((yyval.nested).comp, (yyvsp[0].lval), 0); }
#line 2661 "cp-name-parser.c.tmp"
    break;

  case 120: /* array_indicator: '[' ']'  */
#line 773 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_ARRAY_TYPE, NULL, NULL); }
#line 2667 "cp-name-parser.c.tmp"
    break;

  case 121: /* array_indicator: '[' INT ']'  */
#line 775 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_ARRAY_TYPE, (yyvsp[-1].comp), NULL); }
#line 2673 "cp-name-parser.c.tmp"
    break;

  case 122: /* typespec_2: builtin_type qualifiers  */
#line 789 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[-1].comp), (yyvsp[0].lval), 0); }
#line 2679 "cp-name-parser.c.tmp"
    break;

  case 124: /* typespec_2: qualifiers builtin_type qualifiers  */
#line 792 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[-1].comp), (yyvsp[-2].lval) | (yyvsp[0].lval), 0); }
#line 2685 "cp-name-parser.c.tmp"
    break;

  case 125: /* typespec_2: qualifiers builtin_type  */
#line 794 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[0].comp), (yyvsp[-1].lval), 0); }
#line 2691 "cp-name-parser.c.tmp"
    break;

  case 126: /* typespec_2: name qualifiers  */
#line 797 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[-1].comp), (yyvsp[0].lval), 0); }
#line 2697 "cp-name-parser.c.tmp"
    break;

  case 128: /* typespec_2: qualifiers name qualifiers  */
#line 800 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[-1].comp), (yyvsp[-2].lval) | (yyvsp[0].lval), 0); }
#line 2703 "cp-name-parser.c.tmp"
    break;

  case 129: /* typespec_2: qualifiers name  */
#line 802 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[0].comp), (yyvsp[-1].lval), 0); }
#line 2709 "cp-name-parser.c.tmp"
    break;

  case 130: /* typespec_2: COLONCOLON name qualifiers  */
#line 805 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[-1].comp), (yyvsp[0].lval), 0); }
#line 2715 "cp-name-parser.c.tmp"
    break;

  case 131: /* typespec_2: COLONCOLON name  */
#line 807 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[0].comp); }
#line 2721 "cp-name-parser.c.tmp"
    break;

  case 132: /* typespec_2: qualifiers COLONCOLON name qualifiers  */
#line 809 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[-1].comp), (yyvsp[-3].lval) | (yyvsp[0].lval), 0); }
#line 2727 "cp-name-parser.c.tmp"
    break;

  case 133: /* typespec_2: qualifiers COLONCOLON name  */
#line 811 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[0].comp), (yyvsp[-2].lval), 0); }
#line 2733 "cp-name-parser.c.tmp"
    break;

  case 134: /* abstract_declarator: ptr_operator  */
#line 816 "cp-name-parser.y"
                        { (yyval.abstract).comp = (yyvsp[0].nested).comp; (yyval.abstract).last = (yyvsp[0].nested).last;
			  (yyval.abstract).fn.comp = NULL; (yyval.abstract).fn.last = NULL; }
#line 2740 "cp-name-parser.c.tmp"
    break;

  case 135: /* abstract_declarator: ptr_operator abstract_declarator  */
#line 819 "cp-name-parser.y"
                        { (yyval.abstract) = (yyvsp[0].abstract); (yyval.abstract).fn.comp = NULL; (yyval.abstract).fn.last = NULL;
			  if ((yyvsp[0].abstract).fn.comp) { (yyval.abstract).last = (yyvsp[0].abstract).fn.last; *(yyvsp[0].abstract).last = (yyvsp[0].abstract).fn.comp; }
			  *(yyval.abstract).last = (yyvsp[-1].nested).comp;
			  (yyval.abstract).last = (yyvsp[-1].nested).last; }
#line 2749 "cp-name-parser.c.tmp"
    break;

  case 136: /* abstract_declarator: direct_abstract_declarator  */
#line 824 "cp-name-parser.y"
                        { (yyval.abstract).fn.comp = NULL; (yyval.abstract).fn.last = NULL;
			  if ((yyvsp[0].abstract).fn.comp) { (yyval.abstract).last = (yyvsp[0].abstract).fn.last; *(yyvsp[0].abstract).last = (yyvsp[0].abstract).fn.comp; }
			}
#line 2757 "cp-name-parser.c.tmp"
    break;

  case 137: /* direct_abstract_declarator: '(' abstract_declarator ')'  */
#line 831 "cp-name-parser.y"
                        { (yyval.abstract) = (yyvsp[-1].abstract); (yyval.abstract).fn.comp = NULL; (yyval.abstract).fn.last = NULL; (yyval.abstract).fold_flag = 1;
			  if ((yyvsp[-1].abstract).fn.comp) { (yyval.abstract).last = (yyvsp[-1].abstract).fn.last; *(yyvsp[-1].abstract).last = (yyvsp[-1].abstract).fn.comp; }
			}
#line 2765 "cp-name-parser.c.tmp"
    break;

  case 138: /* direct_abstract_declarator: direct_abstract_declarator function_arglist  */
#line 835 "cp-name-parser.y"
                        { (yyval.abstract).fold_flag = 0;
			  if ((yyvsp[-1].abstract).fn.comp) { (yyval.abstract).last = (yyvsp[-1].abstract).fn.last; *(yyvsp[-1].abstract).last = (yyvsp[-1].abstract).fn.comp; }
			  if ((yyvsp[-1].abstract).fold_flag)
			    {
			      *(yyval.abstract).last = (yyvsp[0].nested).comp;
			      (yyval.abstract).last = (yyvsp[0].nested).last;
			    }
			  else
			    (yyval.abstract).fn = (yyvsp[0].nested);
			}
#line 2780 "cp-name-parser.c.tmp"
    break;

  case 139: /* direct_abstract_declarator: direct_abstract_declarator array_indicator  */
#line 846 "cp-name-parser.y"
                        { (yyval.abstract).fn.comp = NULL; (yyval.abstract).fn.last = NULL; (yyval.abstract).fold_flag = 0;
			  if ((yyvsp[-1].abstract).fn.comp) { (yyval.abstract).last = (yyvsp[-1].abstract).fn.last; *(yyvsp[-1].abstract).last = (yyvsp[-1].abstract).fn.comp; }
			  *(yyvsp[-1].abstract).last = (yyvsp[0].comp);
			  (yyval.abstract).last = &d_right ((yyvsp[0].comp));
			}
#line 2790 "cp-name-parser.c.tmp"
    break;

  case 140: /* direct_abstract_declarator: array_indicator  */
#line 852 "cp-name-parser.y"
                        { (yyval.abstract).fn.comp = NULL; (yyval.abstract).fn.last = NULL; (yyval.abstract).fold_flag = 0;
			  (yyval.abstract).comp = (yyvsp[0].comp);
			  (yyval.abstract).last = &d_right ((yyvsp[0].comp));
			}
#line 2799 "cp-name-parser.c.tmp"
    break;

  case 141: /* abstract_declarator_fn: ptr_operator  */
#line 870 "cp-name-parser.y"
                        { (yyval.abstract).comp = (yyvsp[0].nested).comp; (yyval.abstract).last = (yyvsp[0].nested).last;
			  (yyval.abstract).fn.comp = NULL; (yyval.abstract).fn.last = NULL; (yyval.abstract).start = NULL; }
#line 2806 "cp-name-parser.c.tmp"
    break;

  case 142: /* abstract_declarator_fn: ptr_operator abstract_declarator_fn  */
#line 873 "cp-name-parser.y"
                        { (yyval.abstract) = (yyvsp[0].abstract);
			  if ((yyvsp[0].abstract).last)
			    *(yyval.abstract).last = (yyvsp[-1].nested).comp;
			  else
			    (yyval.abstract).comp = (yyvsp[-1].nested).comp;
			  (yyval.abstract).last = (yyvsp[-1].nested).last;
			}
#line 2818 "cp-name-parser.c.tmp"
    break;

  case 143: /* abstract_declarator_fn: direct_abstract_declarator  */
#line 881 "cp-name-parser.y"
                        { (yyval.abstract).comp = (yyvsp[0].abstract).comp; (yyval.abstract).last = (yyvsp[0].abstract).last; (yyval.abstract).fn = (yyvsp[0].abstract).fn; (yyval.abstract).start = NULL; }
#line 2824 "cp-name-parser.c.tmp"
    break;

  case 144: /* abstract_declarator_fn: direct_abstract_declarator function_arglist COLONCOLON start  */
#line 883 "cp-name-parser.y"
                        { (yyval.abstract).start = (yyvsp[0].comp);
			  if ((yyvsp[-3].abstract).fn.comp) { (yyval.abstract).last = (yyvsp[-3].abstract).fn.last; *(yyvsp[-3].abstract).last = (yyvsp[-3].abstract).fn.comp; }
			  if ((yyvsp[-3].abstract).fold_flag)
			    {
			      *(yyval.abstract).last = (yyvsp[-2].nested).comp;
			      (yyval.abstract).last = (yyvsp[-2].nested).last;
			    }
			  else
			    (yyval.abstract).fn = (yyvsp[-2].nested);
			}
#line 2839 "cp-name-parser.c.tmp"
    break;

  case 145: /* abstract_declarator_fn: function_arglist start_opt  */
#line 894 "cp-name-parser.y"
                        { (yyval.abstract).fn = (yyvsp[-1].nested);
			  (yyval.abstract).start = (yyvsp[0].comp);
			  (yyval.abstract).comp = NULL; (yyval.abstract).last = NULL;
			}
#line 2848 "cp-name-parser.c.tmp"
    break;

  case 147: /* type: typespec_2 abstract_declarator  */
#line 902 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[0].abstract).comp;
			  *(yyvsp[0].abstract).last = (yyvsp[-1].comp);
			}
#line 2856 "cp-name-parser.c.tmp"
    break;

  case 148: /* declarator: ptr_operator declarator  */
#line 908 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[0].nested).comp;
			  (yyval.nested).last = (yyvsp[-1].nested).last;
			  *(yyvsp[0].nested).last = (yyvsp[-1].nested).comp; }
#line 2864 "cp-name-parser.c.tmp"
    break;

  case 150: /* direct_declarator: '(' declarator ')'  */
#line 916 "cp-name-parser.y"
                        { (yyval.nested) = (yyvsp[-1].nested); }
#line 2870 "cp-name-parser.c.tmp"
    break;

  case 151: /* direct_declarator: direct_declarator function_arglist  */
#line 918 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-1].nested).comp;
			  *(yyvsp[-1].nested).last = (yyvsp[0].nested).comp;
			  (yyval.nested).last = (yyvsp[0].nested).last;
			}
#line 2879 "cp-name-parser.c.tmp"
    break;

  case 152: /* direct_declarator: direct_declarator array_indicator  */
#line 923 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-1].nested).comp;
			  *(yyvsp[-1].nested).last = (yyvsp[0].comp);
			  (yyval.nested).last = &d_right ((yyvsp[0].comp));
			}
#line 2888 "cp-name-parser.c.tmp"
    break;

  case 153: /* direct_declarator: colon_ext_name  */
#line 928 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, (yyvsp[0].comp), NULL);
			  (yyval.nested).last = &d_right ((yyval.nested).comp);
			}
#line 2896 "cp-name-parser.c.tmp"
    break;

  case 154: /* declarator_1: ptr_operator declarator_1  */
#line 940 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[0].nested).comp;
			  (yyval.nested).last = (yyvsp[-1].nested).last;
			  *(yyvsp[0].nested).last = (yyvsp[-1].nested).comp; }
#line 2904 "cp-name-parser.c.tmp"
    break;

  case 155: /* declarator_1: colon_ext_name  */
#line 944 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, (yyvsp[0].comp), NULL);
			  (yyval.nested).last = &d_right ((yyval.nested).comp);
			}
#line 2912 "cp-name-parser.c.tmp"
    break;

  case 157: /* declarator_1: colon_ext_name function_arglist COLONCOLON start  */
#line 956 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, (yyvsp[-3].comp), (yyvsp[-2].nested).comp);
			  (yyval.nested).last = (yyvsp[-2].nested).last;
			  (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME, (yyval.nested).comp, (yyvsp[0].comp));
			}
#line 2921 "cp-name-parser.c.tmp"
    break;

  case 158: /* declarator_1: direct_declarator_1 function_arglist COLONCOLON start  */
#line 961 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-3].nested).comp;
			  *(yyvsp[-3].nested).last = (yyvsp[-2].nested).comp;
			  (yyval.nested).last = (yyvsp[-2].nested).last;
			  (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME, (yyval.nested).comp, (yyvsp[0].comp));
			}
#line 2931 "cp-name-parser.c.tmp"
    break;

  case 159: /* direct_declarator_1: '(' ptr_operator declarator ')'  */
#line 970 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-1].nested).comp;
			  (yyval.nested).last = (yyvsp[-2].nested).last;
			  *(yyvsp[-1].nested).last = (yyvsp[-2].nested).comp; }
#line 2939 "cp-name-parser.c.tmp"
    break;

  case 160: /* direct_declarator_1: direct_declarator_1 function_arglist  */
#line 974 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-1].nested).comp;
			  *(yyvsp[-1].nested).last = (yyvsp[0].nested).comp;
			  (yyval.nested).last = (yyvsp[0].nested).last;
			}
#line 2948 "cp-name-parser.c.tmp"
    break;

  case 161: /* direct_declarator_1: direct_declarator_1 array_indicator  */
#line 979 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-1].nested).comp;
			  *(yyvsp[-1].nested).last = (yyvsp[0].comp);
			  (yyval.nested).last = &d_right ((yyvsp[0].comp));
			}
#line 2957 "cp-name-parser.c.tmp"
    break;

  case 162: /* direct_declarator_1: colon_ext_name function_arglist  */
#line 984 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, (yyvsp[-1].comp), (yyvsp[0].nested).comp);
			  (yyval.nested).last = (yyvsp[0].nested).last;
			}
#line 2965 "cp-name-parser.c.tmp"
    break;

  case 163: /* direct_declarator_1: colon_ext_name array_indicator  */
#line 988 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, (yyvsp[-1].comp), (yyvsp[0].comp));
			  (yyval.nested).last = &d_right ((yyvsp[0].comp));
			}
#line 2973 "cp-name-parser.c.tmp"
    break;

  case 164: /* exp: '(' exp1 ')'  */
#line 994 "cp-name-parser.y"
                { (yyval.comp) = (yyvsp[-1].comp); }
#line 2979 "cp-name-parser.c.tmp"
    break;

  case 166: /* exp1: exp '>' exp  */
#line 1003 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary (">", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 2985 "cp-name-parser.c.tmp"
    break;

  case 167: /* exp1: '&' start  */
#line 1010 "cp-name-parser.y"
                { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_UNARY, state->make_operator ("&", 1), (yyvsp[0].comp)); }
#line 2991 "cp-name-parser.c.tmp"
    break;

  case 168: /* exp1: '&' '(' start ')'  */
#line 1012 "cp-name-parser.y"
                { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_UNARY, state->make_operator ("&", 1), (yyvsp[-1].comp)); }
#line 2997 "cp-name-parser.c.tmp"
    break;

  case 169: /* exp: '-' exp  */
#line 1017 "cp-name-parser.y"
                { (yyval.comp) = state->d_unary ("-", (yyvsp[0].comp)); }
#line 3003 "cp-name-parser.c.tmp"
    break;

  case 170: /* exp: '!' exp  */
#line 1021 "cp-name-parser.y"
                { (yyval.comp) = state->d_unary ("!", (yyvsp[0].comp)); }
#line 3009 "cp-name-parser.c.tmp"
    break;

  case 171: /* exp: '~' exp  */
#line 1025 "cp-name-parser.y"
                { (yyval.comp) = state->d_unary ("~", (yyvsp[0].comp)); }
#line 3015 "cp-name-parser.c.tmp"
    break;

  case 172: /* exp: '(' type ')' exp  */
#line 1032 "cp-name-parser.y"
                { if ((yyvsp[0].comp)->type == DEMANGLE_COMPONENT_LITERAL
		      || (yyvsp[0].comp)->type == DEMANGLE_COMPONENT_LITERAL_NEG)
		    {
		      (yyval.comp) = (yyvsp[0].comp);
		      d_left ((yyvsp[0].comp)) = (yyvsp[-2].comp);
		    }
		  else
		    (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_UNARY,
				      state->fill_comp (DEMANGLE_COMPONENT_CAST, (yyvsp[-2].comp), NULL),
				      (yyvsp[0].comp));
		}
#line 3031 "cp-name-parser.c.tmp"
    break;

  case 173: /* exp: STATIC_CAST '<' type '>' '(' exp1 ')'  */
#line 1048 "cp-name-parser.y"
                { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_UNARY,
				    state->fill_comp (DEMANGLE_COMPONENT_CAST, (yyvsp[-4].comp), NULL),
				    (yyvsp[-1].comp));
		}
#line 3040 "cp-name-parser.c.tmp"
    break;

  case 174: /* exp: DYNAMIC_CAST '<' type '>' '(' exp1 ')'  */
#line 1055 "cp-name-parser.y"
                { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_UNARY,
				    state->fill_comp (DEMANGLE_COMPONENT_CAST, (yyvsp[-4].comp), NULL),
				    (yyvsp[-1].comp));
		}
#line 3049 "cp-name-parser.c.tmp"
    break;

  case 175: /* exp: REINTERPRET_CAST '<' type '>' '(' exp1 ')'  */
#line 1062 "cp-name-parser.y"
                { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_UNARY,
				    state->fill_comp (DEMANGLE_COMPONENT_CAST, (yyvsp[-4].comp), NULL),
				    (yyvsp[-1].comp));
		}
#line 3058 "cp-name-parser.c.tmp"
    break;

  case 176: /* exp: exp '*' exp  */
#line 1081 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("*", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3064 "cp-name-parser.c.tmp"
    break;

  case 177: /* exp: exp '/' exp  */
#line 1085 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("/", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3070 "cp-name-parser.c.tmp"
    break;

  case 178: /* exp: exp '%' exp  */
#line 1089 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("%", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3076 "cp-name-parser.c.tmp"
    break;

  case 179: /* exp: exp '+' exp  */
#line 1093 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("+", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3082 "cp-name-parser.c.tmp"
    break;

  case 180: /* exp: exp '-' exp  */
#line 1097 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("-", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3088 "cp-name-parser.c.tmp"
    break;

  case 181: /* exp: exp LSH exp  */
#line 1101 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("<<", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3094 "cp-name-parser.c.tmp"
    break;

  case 182: /* exp: exp RSH exp  */
#line 1105 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary (">>", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3100 "cp-name-parser.c.tmp"
    break;

  case 183: /* exp: exp EQUAL exp  */
#line 1109 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("==", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3106 "cp-name-parser.c.tmp"
    break;

  case 184: /* exp: exp NOTEQUAL exp  */
#line 1113 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("!=", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3112 "cp-name-parser.c.tmp"
    break;

  case 185: /* exp: exp LEQ exp  */
#line 1117 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("<=", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3118 "cp-name-parser.c.tmp"
    break;

  case 186: /* exp: exp GEQ exp  */
#line 1121 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary (">=", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3124 "cp-name-parser.c.tmp"
    break;

  case 187: /* exp: exp SPACESHIP exp  */
#line 1125 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("<=>", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3130 "cp-name-parser.c.tmp"
    break;

  case 188: /* exp: exp '<' exp  */
#line 1129 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("<", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3136 "cp-name-parser.c.tmp"
    break;

  case 189: /* exp: exp '&' exp  */
#line 1133 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("&", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3142 "cp-name-parser.c.tmp"
    break;

  case 190: /* exp: exp '^' exp  */
#line 1137 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("^", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3148 "cp-name-parser.c.tmp"
    break;

  case 191: /* exp: exp '|' exp  */
#line 1141 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("|", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3154 "cp-name-parser.c.tmp"
    break;

  case 192: /* exp: exp ANDAND exp  */
#line 1145 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("&&", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3160 "cp-name-parser.c.tmp"
    break;

  case 193: /* exp: exp OROR exp  */
#line 1149 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("||", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3166 "cp-name-parser.c.tmp"
    break;

  case 194: /* exp: exp ARROW NAME  */
#line 1154 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("->", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3172 "cp-name-parser.c.tmp"
    break;

  case 195: /* exp: exp '.' NAME  */
#line 1158 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary (".", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3178 "cp-name-parser.c.tmp"
    break;

  case 196: /* exp: exp '?' exp ':' exp  */
#line 1162 "cp-name-parser.y"
                { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_TRINARY, state->make_operator ("?", 3),
				    state->fill_comp (DEMANGLE_COMPONENT_TRINARY_ARG1, (yyvsp[-4].comp),
						 state->fill_comp (DEMANGLE_COMPONENT_TRINARY_ARG2, (yyvsp[-2].comp), (yyvsp[0].comp))));
		}
#line 3187 "cp-name-parser.c.tmp"
    break;

  case 199: /* exp: SIZEOF '(' type ')'  */
#line 1176 "cp-name-parser.y"
                {
		  /* Match the whitespacing of cplus_demangle_operators.
		     It would abort on unrecognized string otherwise.  */
		  (yyval.comp) = state->d_unary ("sizeof ", (yyvsp[-1].comp));
		}
#line 3197 "cp-name-parser.c.tmp"
    break;

  case 200: /* exp: TRUEKEYWORD  */
#line 1185 "cp-name-parser.y"
                { struct demangle_component *i;
		  i = state->make_name ("1", 1);
		  (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_LITERAL,
				    state->make_builtin_type ( "bool"),
				    i);
		}
#line 3208 "cp-name-parser.c.tmp"
    break;

  case 201: /* exp: FALSEKEYWORD  */
#line 1194 "cp-name-parser.y"
                { struct demangle_component *i;
		  i = state->make_name ("0", 1);
		  (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_LITERAL,
				    state->make_builtin_type ("bool"),
				    i);
		}
#line 3219 "cp-name-parser.c.tmp"
    break;


#line 3223 "cp-name-parser.c.tmp"

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
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (cp_name_parser_yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

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
      yyerror (state, YY_("syntax error"));
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
                      yytoken, &yylval, state);
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
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, state);
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
  yyerror (state, YY_("memory exhausted"));
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
                  yytoken, &yylval, state);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, state);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 1204 "cp-name-parser.y"


/* Apply QUALIFIERS to LHS and return a qualified component.  IS_METHOD
   is set if LHS is a method, in which case the qualifiers are logically
   applied to "this".  We apply qualifiers in a consistent order; LHS
   may already be qualified; duplicate qualifiers are not created.  */

struct demangle_component *
cpname_state::d_qualify (struct demangle_component *lhs, int qualifiers,
			 int is_method)
{
  struct demangle_component **inner_p;
  enum demangle_component_type type;

  /* For now the order is CONST (innermost), VOLATILE, RESTRICT.  */

#define HANDLE_QUAL(TYPE, MTYPE, QUAL)				\
  if ((qualifiers & QUAL) && (type != TYPE) && (type != MTYPE))	\
    {								\
      *inner_p = fill_comp (is_method ? MTYPE : TYPE,		\
			    *inner_p, NULL);			\
      inner_p = &d_left (*inner_p);				\
      type = (*inner_p)->type;					\
    }								\
  else if (type == TYPE || type == MTYPE)			\
    {								\
      inner_p = &d_left (*inner_p);				\
      type = (*inner_p)->type;					\
    }

  inner_p = &lhs;

  type = (*inner_p)->type;

  HANDLE_QUAL (DEMANGLE_COMPONENT_RESTRICT, DEMANGLE_COMPONENT_RESTRICT_THIS, QUAL_RESTRICT);
  HANDLE_QUAL (DEMANGLE_COMPONENT_VOLATILE, DEMANGLE_COMPONENT_VOLATILE_THIS, QUAL_VOLATILE);
  HANDLE_QUAL (DEMANGLE_COMPONENT_CONST, DEMANGLE_COMPONENT_CONST_THIS, QUAL_CONST);

  return lhs;
}

/* Return a builtin type corresponding to FLAGS.  */

struct demangle_component *
cpname_state::d_int_type (int flags)
{
  const char *name;

  switch (flags)
    {
    case INT_SIGNED | INT_CHAR:
      name = "signed char";
      break;
    case INT_CHAR:
      name = "char";
      break;
    case INT_UNSIGNED | INT_CHAR:
      name = "unsigned char";
      break;
    case 0:
    case INT_SIGNED:
      name = "int";
      break;
    case INT_UNSIGNED:
      name = "unsigned int";
      break;
    case INT_LONG:
    case INT_SIGNED | INT_LONG:
      name = "long";
      break;
    case INT_UNSIGNED | INT_LONG:
      name = "unsigned long";
      break;
    case INT_SHORT:
    case INT_SIGNED | INT_SHORT:
      name = "short";
      break;
    case INT_UNSIGNED | INT_SHORT:
      name = "unsigned short";
      break;
    case INT_LLONG | INT_LONG:
    case INT_SIGNED | INT_LLONG | INT_LONG:
      name = "long long";
      break;
    case INT_UNSIGNED | INT_LLONG | INT_LONG:
      name = "unsigned long long";
      break;
    default:
      return NULL;
    }

  return make_builtin_type (name);
}

/* Wrapper to create a unary operation.  */

struct demangle_component *
cpname_state::d_unary (const char *name, struct demangle_component *lhs)
{
  return fill_comp (DEMANGLE_COMPONENT_UNARY, make_operator (name, 1), lhs);
}

/* Wrapper to create a binary operation.  */

struct demangle_component *
cpname_state::d_binary (const char *name, struct demangle_component *lhs,
			struct demangle_component *rhs)
{
  return fill_comp (DEMANGLE_COMPONENT_BINARY, make_operator (name, 2),
		    fill_comp (DEMANGLE_COMPONENT_BINARY_ARGS, lhs, rhs));
}

/* Find the end of a symbol name starting at LEXPTR.  */

static const char *
symbol_end (const char *lexptr)
{
  const char *p = lexptr;

  while (*p && (c_ident_is_alnum (*p) || *p == '_' || *p == '$' || *p == '.'))
    p++;

  return p;
}

/* Take care of parsing a number (anything that starts with a digit).
   The number starts at P and contains LEN characters.  Store the result in
   YYLVAL.  */

int
cpname_state::parse_number (const char *p, int len, int parsed_float,
			    cp_name_parser_YYSTYPE *lvalp)
{
  int unsigned_p = 0;

  /* Number of "L" suffixes encountered.  */
  int long_p = 0;

  struct demangle_component *type, *name;
  enum demangle_component_type literal_type;

  if (p[0] == '-')
    {
      literal_type = DEMANGLE_COMPONENT_LITERAL_NEG;
      p++;
      len--;
    }
  else
    literal_type = DEMANGLE_COMPONENT_LITERAL;

  if (parsed_float)
    {
      /* It's a float since it contains a point or an exponent.  */
      char c;

      /* The GDB lexer checks the result of scanf at this point.  Not doing
	 this leaves our error checking slightly weaker but only for invalid
	 data.  */

      /* See if it has `f' or `l' suffix (float or long double).  */

      c = TOLOWER (p[len - 1]);

      if (c == 'f')
      	{
      	  len--;
      	  type = make_builtin_type ("float");
      	}
      else if (c == 'l')
	{
	  len--;
	  type = make_builtin_type ("long double");
	}
      else if (ISDIGIT (c) || c == '.')
	type = make_builtin_type ("double");
      else
	return ERROR;

      name = make_name (p, len);
      lvalp->comp = fill_comp (literal_type, type, name);

      return FLOAT;
    }

  /* Note that we do not automatically generate unsigned types.  This
     can't be done because we don't have access to the gdbarch
     here.  */

  int base = 10;
  if (len > 1 && p[0] == '0')
    {
      if (p[1] == 'x' || p[1] == 'X')
	{
	  base = 16;
	  p += 2;
	  len -= 2;
	}
      else if (p[1] == 'b' || p[1] == 'B')
	{
	  base = 2;
	  p += 2;
	  len -= 2;
	}
      else if (p[1] == 'd' || p[1] == 'D' || p[1] == 't' || p[1] == 'T')
	{
	  /* Apparently gdb extensions.  */
	  base = 10;
	  p += 2;
	  len -= 2;
	}
      else
	base = 8;
    }

  long_p = 0;
  unsigned_p = 0;
  while (len > 0)
    {
      if (p[len - 1] == 'l' || p[len - 1] == 'L')
	{
	  len--;
	  long_p++;
	  continue;
	}
      if (p[len - 1] == 'u' || p[len - 1] == 'U')
	{
	  len--;
	  unsigned_p++;
	  continue;
	}
      break;
    }

  /* Use gdb_mpz here in case a 128-bit value appears.  */
  gdb_mpz value (0);
  for (int off = 0; off < len; ++off)
    {
      int dig;
      if (ISDIGIT (p[off]))
	dig = p[off] - '0';
      else
	dig = TOLOWER (p[off]) - 'a' + 10;
      if (dig >= base)
	return ERROR;
      value *= base;
      value += dig;
    }

  std::string printed = value.str ();
  const char *copy = obstack_strdup (&demangle_info->obstack, printed);

  if (long_p == 0)
    {
      if (unsigned_p)
	type = make_builtin_type ("unsigned int");
      else
	type = make_builtin_type ("int");
    }
  else if (long_p == 1)
    {
      if (unsigned_p)
	type = make_builtin_type ("unsigned long");
      else
	type = make_builtin_type ("long");
    }
  else
    {
      if (unsigned_p)
	type = make_builtin_type ("unsigned long long");
      else
	type = make_builtin_type ("long long");
    }

  name = make_name (copy, strlen (copy));
  lvalp->comp = fill_comp (literal_type, type, name);

  return INT;
}

static const char backslashable[] = "abefnrtv";
static const char represented[] = "\a\b\e\f\n\r\t\v";

/* Translate the backslash the way we would in the host character set.  */
static int
c_parse_backslash (int host_char, int *target_char)
{
  const char *ix;
  ix = strchr (backslashable, host_char);
  if (! ix)
    return 0;
  else
    *target_char = represented[ix - backslashable];
  return 1;
}

/* Parse a C escape sequence.  STRING_PTR points to a variable
   containing a pointer to the string to parse.  That pointer
   should point to the character after the \.  That pointer
   is updated past the characters we use.  The value of the
   escape sequence is returned.

   A negative value means the sequence \ newline was seen,
   which is supposed to be equivalent to nothing at all.

   If \ is followed by a null character, we return a negative
   value and leave the string pointer pointing at the null character.

   If \ is followed by 000, we return 0 and leave the string pointer
   after the zeros.  A value of 0 does not mean end of string.  */

static int
cp_parse_escape (const char **string_ptr)
{
  int target_char;
  int c = *(*string_ptr)++;
  if (c_parse_backslash (c, &target_char))
    return target_char;
  else
    switch (c)
      {
      case '\n':
	return -2;
      case 0:
	(*string_ptr)--;
	return 0;
      case '^':
	{
	  c = *(*string_ptr)++;

	  if (c == '?')
	    return 0177;
	  else if (c == '\\')
	    target_char = cp_parse_escape (string_ptr);
	  else
	    target_char = c;

	  /* Now target_char is something like `c', and we want to find
	     its control-character equivalent.  */
	  target_char = target_char & 037;

	  return target_char;
	}

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
	{
	  int i = c - '0';
	  int count = 0;
	  while (++count < 3)
	    {
	      c = (**string_ptr);
	      if (c >= '0' && c <= '7')
		{
		  (*string_ptr)++;
		  i *= 8;
		  i += c - '0';
		}
	      else
		{
		  break;
		}
	    }
	  return i;
	}
      default:
	return c;
      }
}

#define HANDLE_SPECIAL(string, comp)				\
  if (startswith (tokstart, string))				\
    {								\
      state->lexptr = tokstart + sizeof (string) - 1;			\
      lvalp->lval = comp;					\
      return DEMANGLER_SPECIAL;					\
    }

#define HANDLE_TOKEN2(string, token)			\
  if (state->lexptr[1] == string[1])				\
    {							\
      state->lexptr += 2;					\
      lvalp->opname = string;				\
      return token;					\
    }      

#define HANDLE_TOKEN3(string, token)			\
  if (state->lexptr[1] == string[1] && state->lexptr[2] == string[2])	\
    {							\
      state->lexptr += 3;					\
      lvalp->opname = string;				\
      return token;					\
    }      

/* Read one token, getting characters through LEXPTR.  */

static int
yylex (cp_name_parser_YYSTYPE *lvalp, cpname_state *state)
{
  int c;
  int namelen;
  const char *tokstart;
  char *copy;

 retry:
  state->prev_lexptr = state->lexptr;
  tokstart = state->lexptr;

  switch (c = *tokstart)
    {
    case 0:
      return 0;

    case ' ':
    case '\t':
    case '\n':
      state->lexptr++;
      goto retry;

    case '\'':
      /* We either have a character constant ('0' or '\177' for example)
	 or we have a quoted symbol reference ('foo(int,int)' in C++
	 for example). */
      state->lexptr++;
      c = *state->lexptr++;
      if (c == '\\')
	c = cp_parse_escape (&state->lexptr);
      else if (c == '\'')
	{
	  yyerror (state, _("empty character constant"));
	  return ERROR;
	}

      /* We over-allocate here, but it doesn't really matter . */
      copy = (char *) obstack_alloc (&state->demangle_info->obstack, 30);
      xsnprintf (copy, 30, "%d", c);

      c = *state->lexptr++;
      if (c != '\'')
	{
	  yyerror (state, _("invalid character constant"));
	  return ERROR;
	}

      lvalp->comp
	= state->fill_comp (DEMANGLE_COMPONENT_LITERAL,
			    state->make_builtin_type ("char"),
			    state->make_name (copy, strlen (copy)));

      return INT;

    case '(':
      if (startswith (tokstart, "(anonymous namespace)"))
	{
	  state->lexptr += 21;
	  lvalp->comp = state->make_name ("(anonymous namespace)",
					  sizeof "(anonymous namespace)" - 1);
	  return NAME;
	}
	[[fallthrough]];

    case ')':
    case ',':
      state->lexptr++;
      return c;

    case '.':
      if (state->lexptr[1] == '.' && state->lexptr[2] == '.')
	{
	  state->lexptr += 3;
	  return ELLIPSIS;
	}

      /* Might be a floating point number.  */
      if (state->lexptr[1] < '0' || state->lexptr[1] > '9')
	goto symbol;		/* Nope, must be a symbol. */

      goto try_number;

    case '-':
      HANDLE_TOKEN2 ("-=", ASSIGN_MODIFY);
      HANDLE_TOKEN2 ("--", DECREMENT);
      HANDLE_TOKEN2 ("->", ARROW);

      /* For construction vtables.  This is kind of hokey.  */
      if (startswith (tokstart, "-in-"))
	{
	  state->lexptr += 4;
	  return CONSTRUCTION_IN;
	}

      if (state->lexptr[1] < '0' || state->lexptr[1] > '9')
	{
	  state->lexptr++;
	  return '-';
	}

    try_number:
      [[fallthrough]];
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
	int hex = 0;

	if (c == '-')
	  p++;

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

	/* If the token includes the C++14 digits separator, we make a
	   copy so that we don't have to handle the separator in
	   parse_number.  */
	std::optional<std::string> no_tick;
	for (;; ++p)
	  {
	    /* This test includes !hex because 'e' is a valid hex digit
	       and thus does not indicate a floating point number when
	       the radix is hex.  */
	    if (!hex && !got_e && (*p == 'e' || *p == 'E'))
	      got_dot = got_e = 1;
	    /* This test does not include !hex, because a '.' always indicates
	       a decimal floating point number regardless of the radix.

	       NOTE drow/2005-03-09: This comment is not accurate in C99;
	       however, it's not clear that all the floating point support
	       in this file is doing any good here.  */
	    else if (!got_dot && *p == '.')
	      got_dot = 1;
	    else if (got_e && (p[-1] == 'e' || p[-1] == 'E')
		     && (*p == '-' || *p == '+'))
	      {
		/* This is the sign of the exponent, not the end of
		   the number.  */
	      }
	    /* C++14 allows a separator.  */
	    else if (*p == '\'')
	      {
		if (!no_tick.has_value ())
		  no_tick.emplace (tokstart, p);
		continue;
	      }
	    /* We will take any letters or digits.  parse_number will
	       complain if past the radix, or if L or U are not final.  */
	    else if (! ISALNUM (*p))
	      break;
	    if (no_tick.has_value ())
	      no_tick->push_back (*p);
	  }
	if (no_tick.has_value ())
	  toktype = state->parse_number (no_tick->c_str (),
					 no_tick->length (),
					 got_dot|got_e, lvalp);
	else
	  toktype = state->parse_number (tokstart, p - tokstart,
					 got_dot|got_e, lvalp);
	if (toktype == ERROR)
	  {
	    yyerror (state, _("invalid number"));
	    return ERROR;
	  }
	state->lexptr = p;
	return toktype;
      }

    case '+':
      HANDLE_TOKEN2 ("+=", ASSIGN_MODIFY);
      HANDLE_TOKEN2 ("++", INCREMENT);
      state->lexptr++;
      return c;
    case '*':
      HANDLE_TOKEN2 ("*=", ASSIGN_MODIFY);
      state->lexptr++;
      return c;
    case '/':
      HANDLE_TOKEN2 ("/=", ASSIGN_MODIFY);
      state->lexptr++;
      return c;
    case '%':
      HANDLE_TOKEN2 ("%=", ASSIGN_MODIFY);
      state->lexptr++;
      return c;
    case '|':
      HANDLE_TOKEN2 ("|=", ASSIGN_MODIFY);
      HANDLE_TOKEN2 ("||", OROR);
      state->lexptr++;
      return c;
    case '&':
      HANDLE_TOKEN2 ("&=", ASSIGN_MODIFY);
      HANDLE_TOKEN2 ("&&", ANDAND);
      state->lexptr++;
      return c;
    case '^':
      HANDLE_TOKEN2 ("^=", ASSIGN_MODIFY);
      state->lexptr++;
      return c;
    case '!':
      HANDLE_TOKEN2 ("!=", NOTEQUAL);
      state->lexptr++;
      return c;
    case '<':
      HANDLE_TOKEN3 ("<<=", ASSIGN_MODIFY);
      HANDLE_TOKEN3 ("<=>", SPACESHIP);
      HANDLE_TOKEN2 ("<=", LEQ);
      HANDLE_TOKEN2 ("<<", LSH);
      state->lexptr++;
      return c;
    case '>':
      HANDLE_TOKEN3 (">>=", ASSIGN_MODIFY);
      HANDLE_TOKEN2 (">=", GEQ);
      HANDLE_TOKEN2 (">>", RSH);
      state->lexptr++;
      return c;
    case '=':
      HANDLE_TOKEN2 ("==", EQUAL);
      state->lexptr++;
      return c;
    case ':':
      HANDLE_TOKEN2 ("::", COLONCOLON);
      state->lexptr++;
      return c;

    case '[':
    case ']':
    case '?':
    case '@':
    case '~':
    case '{':
    case '}':
    symbol:
      state->lexptr++;
      return c;

    case '"':
      /* These can't occur in C++ names.  */
      yyerror (state, _("unexpected string literal"));
      return ERROR;
    }

  if (!(c == '_' || c == '$' || c_ident_is_alpha (c)))
    {
      /* We must have come across a bad character (e.g. ';').  */
      yyerror (state, _("invalid character"));
      return ERROR;
    }

  /* It's a name.  See how long it is.  */
  namelen = 0;
  do
    c = tokstart[++namelen];
  while (c_ident_is_alnum (c) || c == '_' || c == '$');

  state->lexptr += namelen;

  /* Catch specific keywords.  Notice that some of the keywords contain
     spaces, and are sorted by the length of the first word.  They must
     all include a trailing space in the string comparison.  */
  switch (namelen)
    {
    case 16:
      if (startswith (tokstart, "reinterpret_cast"))
	return REINTERPRET_CAST;
      break;
    case 12:
      if (startswith (tokstart, "construction vtable for "))
	{
	  state->lexptr = tokstart + 24;
	  return CONSTRUCTION_VTABLE;
	}
      if (startswith (tokstart, "dynamic_cast"))
	return DYNAMIC_CAST;
      break;
    case 11:
      if (startswith (tokstart, "static_cast"))
	return STATIC_CAST;
      break;
    case 9:
      HANDLE_SPECIAL ("covariant return thunk to ", DEMANGLE_COMPONENT_COVARIANT_THUNK);
      HANDLE_SPECIAL ("reference temporary for ", DEMANGLE_COMPONENT_REFTEMP);
      break;
    case 8:
      HANDLE_SPECIAL ("typeinfo for ", DEMANGLE_COMPONENT_TYPEINFO);
      HANDLE_SPECIAL ("typeinfo fn for ", DEMANGLE_COMPONENT_TYPEINFO_FN);
      HANDLE_SPECIAL ("typeinfo name for ", DEMANGLE_COMPONENT_TYPEINFO_NAME);
      if (startswith (tokstart, "operator"))
	return OPERATOR;
      if (startswith (tokstart, "restrict"))
	return RESTRICT;
      if (startswith (tokstart, "unsigned"))
	return UNSIGNED;
      if (startswith (tokstart, "template"))
	return TEMPLATE;
      if (startswith (tokstart, "volatile"))
	return VOLATILE_KEYWORD;
      break;
    case 7:
      HANDLE_SPECIAL ("virtual thunk to ", DEMANGLE_COMPONENT_VIRTUAL_THUNK);
      if (startswith (tokstart, "wchar_t"))
	return WCHAR_T;
      break;
    case 6:
      if (startswith (tokstart, "global constructors keyed to "))
	{
	  const char *p;
	  state->lexptr = tokstart + 29;
	  lvalp->lval = DEMANGLE_COMPONENT_GLOBAL_CONSTRUCTORS;
	  /* Find the end of the symbol.  */
	  p = symbol_end (state->lexptr);
	  lvalp->comp = state->make_name (state->lexptr, p - state->lexptr);
	  state->lexptr = p;
	  return DEMANGLER_SPECIAL;
	}
      if (startswith (tokstart, "global destructors keyed to "))
	{
	  const char *p;
	  state->lexptr = tokstart + 28;
	  lvalp->lval = DEMANGLE_COMPONENT_GLOBAL_DESTRUCTORS;
	  /* Find the end of the symbol.  */
	  p = symbol_end (state->lexptr);
	  lvalp->comp = state->make_name (state->lexptr, p - state->lexptr);
	  state->lexptr = p;
	  return DEMANGLER_SPECIAL;
	}

      HANDLE_SPECIAL ("vtable for ", DEMANGLE_COMPONENT_VTABLE);
      if (startswith (tokstart, "delete"))
	return DELETE;
      if (startswith (tokstart, "struct"))
	return STRUCT;
      if (startswith (tokstart, "signed"))
	return SIGNED_KEYWORD;
      if (startswith (tokstart, "sizeof"))
	return SIZEOF;
      if (startswith (tokstart, "double"))
	return DOUBLE_KEYWORD;
      break;
    case 5:
      HANDLE_SPECIAL ("guard variable for ", DEMANGLE_COMPONENT_GUARD);
      if (startswith (tokstart, "false"))
	return FALSEKEYWORD;
      if (startswith (tokstart, "class"))
	return CLASS;
      if (startswith (tokstart, "union"))
	return UNION;
      if (startswith (tokstart, "float"))
	return FLOAT_KEYWORD;
      if (startswith (tokstart, "short"))
	return SHORT;
      if (startswith (tokstart, "const"))
	return CONST_KEYWORD;
      break;
    case 4:
      if (startswith (tokstart, "void"))
	return VOID;
      if (startswith (tokstart, "bool"))
	return BOOL;
      if (startswith (tokstart, "char"))
	return CHAR;
      if (startswith (tokstart, "enum"))
	return ENUM;
      if (startswith (tokstart, "long"))
	return LONG;
      if (startswith (tokstart, "true"))
	return TRUEKEYWORD;
      break;
    case 3:
      HANDLE_SPECIAL ("VTT for ", DEMANGLE_COMPONENT_VTT);
      HANDLE_SPECIAL ("non-virtual thunk to ", DEMANGLE_COMPONENT_THUNK);
      if (startswith (tokstart, "new"))
	return NEW;
      if (startswith (tokstart, "int"))
	return INT_KEYWORD;
      break;
    default:
      break;
    }

  lvalp->comp = state->make_name (tokstart, namelen);
  return NAME;
}

static void
yyerror (cpname_state *state, const char *msg)
{
  if (state->global_errmsg)
    return;

  state->error_lexptr = state->prev_lexptr;
  state->global_errmsg = msg ? msg : "parse error";
}

/* See cp-support.h.  */

gdb::unique_xmalloc_ptr<char>
cp_comp_to_string (struct demangle_component *result, int estimated_len)
{
  size_t err;

  char *res = gdb_cplus_demangle_print (DMGL_PARAMS | DMGL_ANSI,
					result, estimated_len, &err);
  return gdb::unique_xmalloc_ptr<char> (res);
}

/* Merge the two parse trees given by DEST and SRC.  The parse tree
   in SRC is attached to DEST at the node represented by TARGET.

   NOTE 1: Since there is no API to merge obstacks, this function does
   even attempt to try it.  Fortunately, we do not (yet?) need this ability.
   The code will assert if SRC->obstack is not empty.

   NOTE 2: The string from which SRC was parsed must not be freed, since
   this function will place pointers to that string into DEST.  */

void
cp_merge_demangle_parse_infos (struct demangle_parse_info *dest,
			       struct demangle_component *target,
			       std::unique_ptr<demangle_parse_info> src)

{
  /* Copy the SRC's parse data into DEST.  */
  *target = *src->tree;

  /* Make sure SRC is owned by DEST.  */
  dest->infos.push_back (std::move (src));
}

/* Convert a demangled name to a demangle_component tree.  On success,
   a structure containing the root of the new tree is returned.  On
   error, NULL is returned, and an error message will be set in
   *ERRMSG.  */

struct std::unique_ptr<demangle_parse_info>
cp_demangled_name_to_comp (const char *demangled_name,
			   std::string *errmsg)
{
  auto result = std::make_unique<demangle_parse_info> ();
  cpname_state state (demangled_name, result.get ());

  /* Note that we can't set yydebug here, as is done in the other
     parsers.  Bison implements yydebug as a global, even with a pure
     parser, and this parser is run from worker threads.  So, changing
     yydebug causes TSan reports.  If you need to debug this parser,
     debug gdb and set the global from the outer gdb.  */
  if (yyparse (&state))
    {
      if (state.global_errmsg && errmsg)
	*errmsg = state.global_errmsg;
      return NULL;
    }

  result->tree = state.global_result;

  return result;
}

#if GDB_SELF_TEST

static void
should_be_the_same (const char *one, const char *two)
{
  gdb::unique_xmalloc_ptr<char> cpone = cp_canonicalize_string (one);
  gdb::unique_xmalloc_ptr<char> cptwo = cp_canonicalize_string (two);

  if (cpone != nullptr)
    one = cpone.get ();
  if (cptwo != nullptr)
    two = cptwo.get ();

  SELF_CHECK (strcmp (one, two) == 0);
}

static void
should_parse (const char *name)
{
  std::string err;
  auto parsed = cp_demangled_name_to_comp (name, &err);
  SELF_CHECK (parsed != nullptr);
}

static void
canonicalize_tests ()
{
  should_be_the_same ("short int", "short");
  should_be_the_same ("int short", "short");

  should_be_the_same ("C<(char) 1>::m()", "C<(char) '\\001'>::m()");
  should_be_the_same ("x::y::z<1>", "x::y::z<0x01>");
  should_be_the_same ("x::y::z<1>", "x::y::z<01>");
  should_be_the_same ("x::y::z<(unsigned long long) 1>", "x::y::z<01ull>");
  should_be_the_same ("x::y::z<0b111>", "x::y::z<7>");
  should_be_the_same ("x::y::z<0b111>", "x::y::z<0t7>");
  should_be_the_same ("x::y::z<0b111>", "x::y::z<0D7>");

  should_be_the_same ("x::y::z<0xff'ff>", "x::y::z<65535>");

  should_be_the_same ("something<void ()>", "something<  void()  >");
  should_be_the_same ("something<void ()>", "something<void (void)>");

  should_parse ("void whatever::operator<=><int, int>");

  should_be_the_same ("Foozle<int>::fogey<Empty<int> > (Empty<int>)",
		      "Foozle<int>::fogey<Empty<int>> (Empty<int>)");

  should_be_the_same ("something :: operator new [ ]",
		      "something::operator new[]");
  should_be_the_same ("something :: operator   new",
		      "something::operator new");
  should_be_the_same ("operator()", "operator ()");
}

#endif

INIT_GDB_FILE (cp_name_parser)
{
#if GDB_SELF_TEST
  selftests::register_test ("canonicalize", canonicalize_tests);
#endif
}
