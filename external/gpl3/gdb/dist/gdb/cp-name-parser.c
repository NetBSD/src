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
  /* LEXPTR is the current pointer into our lex buffer.  PREV_LEXPTR
     is the start of the last token lexed, only used for diagnostics.
     ERROR_LEXPTR is the first place an error occurred.  GLOBAL_ERRMSG
     is the first error message encountered.  */

  const char *lexptr, *prev_lexptr, *error_lexptr, *global_errmsg;

  demangle_parse_info *demangle_info;

  /* The parse tree created by the parser is stored here after a
     successful parse.  */

  struct demangle_component *global_result;

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

#line 550 "cp-name-parser.c.tmp"


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
#define YYLAST   1115

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  76
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  40
/* YYNRULES -- Number of rules.  */
#define YYNRULES  198
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  330

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
       0,   316,   316,   325,   327,   329,   334,   335,   342,   351,
     358,   362,   365,   384,   386,   390,   396,   402,   408,   414,
     416,   418,   420,   422,   424,   426,   428,   430,   432,   434,
     436,   438,   440,   442,   444,   446,   448,   450,   452,   454,
     456,   458,   460,   462,   464,   466,   468,   470,   472,   480,
     485,   490,   494,   499,   507,   508,   510,   522,   523,   529,
     531,   532,   534,   537,   538,   541,   542,   546,   548,   551,
     555,   560,   564,   573,   577,   580,   591,   592,   596,   598,
     600,   601,   604,   608,   613,   618,   624,   634,   638,   642,
     650,   651,   654,   656,   658,   662,   663,   670,   672,   674,
     676,   678,   680,   684,   685,   689,   691,   693,   695,   697,
     699,   701,   705,   710,   713,   716,   722,   730,   732,   746,
     748,   749,   751,   754,   756,   757,   759,   762,   764,   766,
     768,   773,   776,   781,   788,   792,   803,   809,   827,   830,
     838,   840,   851,   858,   859,   865,   869,   873,   875,   880,
     885,   897,   901,   905,   913,   918,   927,   931,   936,   941,
     945,   951,   957,   960,   967,   969,   974,   978,   982,   989,
    1005,  1012,  1019,  1038,  1042,  1046,  1050,  1054,  1058,  1062,
    1066,  1070,  1074,  1078,  1082,  1086,  1090,  1094,  1098,  1102,
    1106,  1111,  1115,  1119,  1126,  1130,  1133,  1142,  1151
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

#define YYPACT_NINF (-206)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     720,    13,  -206,    52,   555,  -206,   -16,  -206,  -206,  -206,
    -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,   720,   720,
      12,    23,  -206,  -206,  -206,   -11,  -206,   110,  -206,   120,
     -36,  -206,    53,    63,   120,   924,  -206,   317,   120,    17,
    -206,  -206,   350,  -206,   120,  -206,    53,    41,    -2,    24,
    -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,
    -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,
    -206,  -206,  -206,  -206,    36,    40,  -206,  -206,    64,   125,
    -206,  -206,  -206,    93,  -206,  -206,   350,    13,   720,  -206,
    -206,   120,     6,   619,  -206,     5,    63,   127,   339,  -206,
     -31,  -206,  -206,   812,   127,    49,  -206,  -206,   136,  -206,
    -206,    41,   120,   120,  -206,  -206,  -206,    75,   748,   619,
    -206,  -206,   -31,  -206,   232,   127,    47,  -206,   -31,  -206,
     -31,  -206,  -206,    80,    97,   114,   121,  -206,  -206,   652,
     501,   470,   501,   423,  -206,    18,  -206,    17,   941,  -206,
    -206,    92,   107,  -206,  -206,  -206,   720,    89,  -206,   246,
    -206,  -206,   112,  -206,    41,   141,   120,   453,    10,    14,
     453,   453,   147,    49,   120,   136,   720,  -206,   184,  -206,
     181,  -206,  -206,  -206,  -206,   120,  -206,  -206,  -206,   251,
     257,   183,  -206,  -206,   453,  -206,  -206,  -206,   195,  -206,
     900,   900,   900,   900,   720,  -206,   501,    34,    34,    34,
     683,   453,   155,   915,   170,   350,  -206,  -206,   501,   501,
     501,   501,   501,   501,   501,   501,   501,   501,   501,   501,
     501,   501,   501,   501,   501,   501,   501,   207,   228,  -206,
    -206,  -206,  -206,   120,  -206,    11,   120,  -206,   120,   870,
    -206,  -206,  -206,    35,   720,  -206,   257,  -206,   257,   200,
     -31,   720,   720,   203,   194,   197,   201,   217,   720,  -206,
     501,   501,  -206,  -206,   810,   965,   718,   987,  1008,  1028,
    1046,  1046,   495,   495,   495,   495,   846,   846,   352,   352,
      34,    34,    34,  -206,  -206,  -206,  -206,  -206,  -206,   453,
    -206,   218,  -206,  -206,  -206,  -206,  -206,  -206,  -206,   174,
     186,   198,  -206,   234,    34,   941,   501,  -206,  -206,   473,
     473,   473,  -206,   941,   236,   244,   245,  -206,  -206,  -206
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    60,    99,     0,     0,    98,   101,   102,    97,    94,
      93,   107,   109,    92,   111,   106,   100,   110,     0,     0,
       0,     0,     2,     5,     4,    54,    51,     6,    68,   124,
       0,    65,     0,    62,    95,     0,   103,   105,   120,   143,
       3,    69,     0,    53,   128,    66,     0,     0,    15,    16,
      32,    44,    29,    41,    40,    26,    24,    25,    35,    36,
      30,    31,    37,    38,    39,    33,    34,    19,    20,    21,
      22,    23,    42,    43,    46,     0,    27,    28,     0,     0,
      49,   108,    13,     0,    56,     1,     0,     0,     0,   114,
     113,    90,     0,     0,    11,     0,     0,     6,   138,   137,
     140,    12,   123,     0,     6,    59,    50,    67,    61,    71,
      96,     0,   126,   122,   101,   104,   119,     0,     0,     0,
      63,    57,   152,    64,     0,     6,   131,   144,   133,     8,
     153,   194,   195,     0,     0,     0,     0,   197,   198,     0,
       0,     0,     0,     0,    81,     0,    74,    76,    80,   127,
      52,     0,     0,    45,    48,    47,     0,     0,     7,     0,
     112,    91,     0,   117,     0,   111,    90,     0,     0,     0,
     131,    82,     0,     0,    90,     0,     0,   142,     0,   139,
     135,   136,    10,    70,    72,   130,   125,   121,    58,     0,
     131,   159,   160,     9,     0,   132,   151,   135,   157,   158,
       0,     0,     0,     0,     0,    78,     0,   166,   168,   167,
       0,   143,     0,   162,     0,     0,    73,    77,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    17,
      18,    14,    55,    90,   118,     0,    90,    89,    90,     0,
      83,   134,   115,     0,     0,   129,     0,   150,   131,     0,
     146,     0,     0,     0,     0,     0,     0,     0,     0,   164,
       0,     0,   161,    75,     0,   190,   189,   188,   187,   186,
     180,   181,   185,   182,   183,   184,   178,   179,   176,   177,
     173,   174,   175,   191,   192,   116,    88,    87,    86,    84,
     141,     0,   145,   156,   148,   149,   154,   155,   196,     0,
       0,     0,    79,     0,   169,   163,     0,    85,   147,     0,
       0,     0,   165,   193,     0,     0,     0,   170,   172,   171
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -206,  -206,    30,    56,   -39,  -206,  -206,    -1,  -206,    -4,
    -206,    15,  -182,   -13,     1,    -3,   111,   178,    72,  -206,
     -18,  -160,  -206,   223,   253,  -206,   260,   -20,   -95,   190,
     -19,   -14,   199,   145,  -205,  -206,   173,  -206,    -5,  -117
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    21,   158,    94,    23,    24,    25,    26,    27,    28,
     120,    29,   122,    30,    31,    32,    33,   145,   146,   169,
      97,   160,    34,    35,    36,    37,    38,   170,    99,    39,
     172,   128,   101,    40,   259,   260,   129,   130,   213,   214
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      46,    79,    43,   144,    45,   181,   247,    98,   257,   162,
     173,    81,   104,   100,   252,   105,   105,    84,    44,   126,
     127,   125,     1,    85,    95,    41,   123,   192,   107,   117,
      22,   106,    79,   181,   118,   199,   124,   148,   103,    92,
     173,    86,   107,   103,    79,   150,     1,   144,    82,    83,
     112,   301,     1,   302,   121,   248,   249,     1,   105,   117,
     215,   183,    44,    89,   118,    42,    90,   174,   151,     4,
       4,   216,   174,   243,   257,   109,   257,   163,    98,    91,
       1,   148,   180,   295,   100,   159,   296,    92,   297,    20,
     168,   119,   118,    89,   152,    95,    90,   243,   153,   190,
      79,    42,   237,   238,   191,   155,   126,   195,    79,    91,
     197,   154,   198,   123,   189,    87,   168,    92,    45,    20,
     107,   194,    88,   124,    20,    20,   185,   126,   217,   125,
     105,   215,   188,   156,   123,   207,   208,   209,    96,   176,
      79,   121,   242,   108,   124,     9,    10,    20,   184,   201,
      13,   195,   250,   177,   200,   107,    89,   108,   106,    90,
     182,   245,   121,   239,    95,   305,   202,    95,    95,   205,
     258,   195,    91,   203,   190,   253,   144,   123,   240,    44,
      92,   193,   246,   244,    93,   107,   241,   124,   251,    87,
     108,    95,   127,   254,    80,   261,   270,    79,    79,    79,
      79,   208,   324,   325,   326,   121,   175,   262,    95,    96,
     148,   272,   293,   274,   275,   276,   277,   278,   279,   280,
     281,   282,   283,   284,   285,   286,   287,   288,   289,   290,
     291,   292,   147,   294,   267,   108,   258,   105,   258,   195,
     269,   303,   304,   123,   308,   123,    79,   309,   319,   118,
     310,   105,   102,   124,   311,   124,   105,   110,   312,   318,
     320,   116,     1,     4,   157,   314,   315,   149,   118,   117,
     108,   121,   321,   121,   118,   322,   147,   327,    96,   108,
     317,    96,    96,   171,   300,   328,   329,   273,   212,    96,
     115,   306,   307,   171,   174,   113,    95,   179,   313,   196,
     108,     0,     0,    89,    20,    96,    90,     0,   243,   171,
       0,   323,     0,   243,   161,     0,     0,     0,    20,    91,
       0,     0,    96,    20,     0,     0,     0,    92,     2,    20,
       0,   256,     0,   211,     0,   186,   187,     0,     5,   114,
       7,     8,     0,     0,    87,   263,   264,   265,   266,     0,
      16,   178,     0,   131,   132,     1,   108,     0,     0,     0,
     133,     2,     3,     0,   175,     0,     0,     4,   134,   135,
     136,     5,     6,     7,     8,     9,    10,    11,    12,     0,
      13,    14,    15,    16,    17,    89,   137,   138,    90,   161,
     211,   211,   211,   211,     0,     0,     0,   161,     0,   139,
       0,    91,     0,     0,     0,   147,     0,     0,   255,    92,
      96,   140,     0,    93,   234,   235,   236,     0,     0,     0,
     237,   238,   141,   142,   143,     0,   131,   132,     1,     0,
       0,     0,     0,   133,     2,    47,     0,     0,     0,   299,
       0,   134,   135,   136,     5,     6,     7,     8,     9,    10,
      11,    12,     0,    13,    14,    15,    16,    17,    87,   137,
     138,     0,     0,     0,     0,   178,   161,     0,     0,   161,
       0,   161,   210,   131,   132,    84,   131,   132,     0,     0,
     133,     0,     0,   133,   140,     0,     0,     0,   134,   135,
     136,   134,   135,   136,     0,   206,   142,   143,     0,    89,
       0,     0,    90,     0,   131,   132,   137,   138,     0,   137,
     138,   133,     0,     0,     0,    91,     0,     0,     0,   134,
     135,   136,   210,    92,     0,     0,     0,   167,     0,     0,
       0,   140,     0,     0,   140,     0,     0,   137,   138,     0,
       0,     0,   206,   142,   143,   206,   142,   143,     0,     0,
       0,     0,   230,   231,     0,   232,   233,   234,   235,   236,
       1,     0,   140,   237,   238,     0,     2,    47,     0,     0,
      48,    49,     0,   206,   142,   143,     5,     6,     7,     8,
       9,    10,    11,    12,     0,    13,    14,    15,    16,    17,
      50,     0,     0,     0,     0,     0,     0,    51,    52,     0,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,     0,    67,    68,    69,    70,    71,
       0,    72,    73,    74,     1,    75,     0,    76,    77,    78,
       2,   164,     0,     0,     0,     0,     0,     0,     0,     0,
       5,     6,     7,     8,     9,    10,    11,    12,     0,    13,
     165,    15,    16,    17,     0,     0,     0,     1,     0,     0,
     166,     0,     0,     2,     3,    89,     0,     0,    90,     4,
       0,     0,     0,     5,     6,     7,     8,     9,    10,    11,
      12,    91,    13,    14,    15,    16,    17,     0,     1,    92,
      18,    19,     0,   167,     2,     3,     0,     0,     0,     0,
       4,     0,     0,     0,     5,     6,     7,     8,     9,    10,
      11,    12,     0,    13,    14,    15,    16,    17,     0,     0,
       0,    18,    19,     0,    20,     1,   204,     0,     0,     0,
       0,     2,     3,     0,     0,     0,     0,     4,     0,     0,
       0,     5,     6,     7,     8,     9,    10,    11,    12,     0,
      13,    14,    15,    16,    17,    20,     0,   268,    18,    19,
       0,     0,     0,    48,    49,   221,   222,   223,   224,   225,
     226,     0,   227,   228,   229,   230,   231,     0,   232,   233,
     234,   235,   236,    50,     0,     0,   237,   238,     0,     0,
      51,    52,    20,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,     0,    67,    68,
      69,    70,    71,     0,    72,    73,    74,     1,    75,     0,
      76,    77,    78,     2,    47,     0,     0,     0,     0,     0,
       0,     0,     0,     5,     6,     7,     8,     9,    10,    11,
      12,     0,    13,   165,    15,    16,    17,     0,     0,     0,
       0,     0,     0,   166,   218,   219,   220,   221,   222,   223,
     224,   225,   226,     0,   227,   228,   229,   230,   231,     0,
     232,   233,   234,   235,   236,     1,     0,     0,   237,   238,
       0,     2,    47,     0,     0,   316,     0,     0,     0,     0,
       0,     5,     6,     7,     8,     9,    10,    11,    12,   298,
      13,    14,    15,    16,    17,     1,   232,   233,   234,   235,
     236,     2,    47,     0,   237,   238,     0,     0,     0,     0,
       0,     5,     6,     7,     8,     9,    10,    11,    12,     1,
      13,    14,    15,    16,    17,     2,   111,     0,     0,     0,
       0,     0,     0,     0,     0,     5,     6,     7,     8,     0,
       0,    11,    12,     0,     0,    14,    15,    16,    17,   218,
     219,   220,   221,   222,   223,   224,   225,   226,   271,   227,
     228,   229,   230,   231,     0,   232,   233,   234,   235,   236,
       0,     0,     0,   237,   238,   218,   219,   220,   221,   222,
     223,   224,   225,   226,     0,   227,   228,   229,   230,   231,
       0,   232,   233,   234,   235,   236,     0,     0,     0,   237,
     238,   220,   221,   222,   223,   224,   225,   226,     0,   227,
     228,   229,   230,   231,     0,   232,   233,   234,   235,   236,
       0,     0,     0,   237,   238,   222,   223,   224,   225,   226,
       0,   227,   228,   229,   230,   231,     0,   232,   233,   234,
     235,   236,     0,     0,     0,   237,   238,   223,   224,   225,
     226,     0,   227,   228,   229,   230,   231,     0,   232,   233,
     234,   235,   236,     0,     0,     0,   237,   238,   224,   225,
     226,     0,   227,   228,   229,   230,   231,     0,   232,   233,
     234,   235,   236,     0,     0,     0,   237,   238,   226,     0,
     227,   228,   229,   230,   231,     0,   232,   233,   234,   235,
     236,     0,     0,     0,   237,   238
};

static const yytype_int16 yycheck[] =
{
       3,     4,     3,    42,     3,   100,   166,    27,   190,     3,
       5,    27,    30,    27,   174,     5,     5,     5,     3,    39,
      39,    39,     5,     0,    27,    12,    39,   122,    32,    12,
       0,    32,    35,   128,    17,   130,    39,    42,    74,    70,
       5,    52,    46,    74,    47,    46,     5,    86,    18,    19,
      35,   256,     5,   258,    39,    41,    42,     5,     5,    12,
      42,    12,    47,    46,    17,    52,    49,    62,    70,    17,
      17,    53,    62,    62,   256,    12,   258,    71,    98,    62,
       5,    86,   100,   243,    98,    88,   246,    70,   248,    72,
      93,    74,    17,    46,    70,    98,    49,    62,    62,   119,
     103,    52,    68,    69,   122,    41,   126,   126,   111,    62,
     128,    71,   130,   126,   117,     5,   119,    70,   117,    72,
     124,    74,    12,   126,    72,    72,   111,   147,   147,   147,
       5,    42,   117,    40,   147,   140,   141,   142,    27,    12,
     143,   126,    53,    32,   147,    25,    26,    72,    12,    52,
      30,   170,   171,    97,    74,   159,    46,    46,   159,    49,
     104,   164,   147,    71,   167,   260,    52,   170,   171,   139,
     190,   190,    62,    52,   194,   178,   215,   190,    71,   164,
      70,   125,    41,    71,    74,   189,   156,   190,    41,     5,
      79,   194,   211,    12,     4,    12,    41,   200,   201,   202,
     203,   206,   319,   320,   321,   190,    95,    12,   211,    98,
     215,    41,     5,   218,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,   229,   230,   231,   232,   233,   234,
     235,   236,    42,     5,   204,   124,   256,     5,   258,   258,
     210,    41,   260,   256,    41,   258,   249,    53,    74,    17,
      53,     5,    29,   256,    53,   258,     5,    34,    41,    41,
      74,    38,     5,    17,    86,   270,   271,    44,    17,    12,
     159,   256,    74,   258,    17,    41,    86,    41,   167,   168,
     299,   170,   171,    93,   254,    41,    41,   215,   143,   178,
      37,   261,   262,   103,    62,    35,   299,    98,   268,   126,
     189,    -1,    -1,    46,    72,   194,    49,    -1,    62,   119,
      -1,   316,    -1,    62,    91,    -1,    -1,    -1,    72,    62,
      -1,    -1,   211,    72,    -1,    -1,    -1,    70,    11,    72,
      -1,    74,    -1,   143,    -1,   112,   113,    -1,    21,    22,
      23,    24,    -1,    -1,     5,   200,   201,   202,   203,    -1,
      33,    12,    -1,     3,     4,     5,   245,    -1,    -1,    -1,
      10,    11,    12,    -1,   253,    -1,    -1,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    -1,
      30,    31,    32,    33,    34,    46,    36,    37,    49,   166,
     200,   201,   202,   203,    -1,    -1,    -1,   174,    -1,    49,
      -1,    62,    -1,    -1,    -1,   215,    -1,    -1,   185,    70,
     299,    61,    -1,    74,    62,    63,    64,    -1,    -1,    -1,
      68,    69,    72,    73,    74,    -1,     3,     4,     5,    -1,
      -1,    -1,    -1,    10,    11,    12,    -1,    -1,    -1,   249,
      -1,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    -1,    30,    31,    32,    33,    34,     5,    36,
      37,    -1,    -1,    -1,    -1,    12,   243,    -1,    -1,   246,
      -1,   248,    49,     3,     4,     5,     3,     4,    -1,    -1,
      10,    -1,    -1,    10,    61,    -1,    -1,    -1,    18,    19,
      20,    18,    19,    20,    -1,    72,    73,    74,    -1,    46,
      -1,    -1,    49,    -1,     3,     4,    36,    37,    -1,    36,
      37,    10,    -1,    -1,    -1,    62,    -1,    -1,    -1,    18,
      19,    20,    49,    70,    -1,    -1,    -1,    74,    -1,    -1,
      -1,    61,    -1,    -1,    61,    -1,    -1,    36,    37,    -1,
      -1,    -1,    72,    73,    74,    72,    73,    74,    -1,    -1,
      -1,    -1,    57,    58,    -1,    60,    61,    62,    63,    64,
       5,    -1,    61,    68,    69,    -1,    11,    12,    -1,    -1,
      15,    16,    -1,    72,    73,    74,    21,    22,    23,    24,
      25,    26,    27,    28,    -1,    30,    31,    32,    33,    34,
      35,    -1,    -1,    -1,    -1,    -1,    -1,    42,    43,    -1,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    -1,    60,    61,    62,    63,    64,
      -1,    66,    67,    68,     5,    70,    -1,    72,    73,    74,
      11,    12,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      21,    22,    23,    24,    25,    26,    27,    28,    -1,    30,
      31,    32,    33,    34,    -1,    -1,    -1,     5,    -1,    -1,
      41,    -1,    -1,    11,    12,    46,    -1,    -1,    49,    17,
      -1,    -1,    -1,    21,    22,    23,    24,    25,    26,    27,
      28,    62,    30,    31,    32,    33,    34,    -1,     5,    70,
      38,    39,    -1,    74,    11,    12,    -1,    -1,    -1,    -1,
      17,    -1,    -1,    -1,    21,    22,    23,    24,    25,    26,
      27,    28,    -1,    30,    31,    32,    33,    34,    -1,    -1,
      -1,    38,    39,    -1,    72,     5,    74,    -1,    -1,    -1,
      -1,    11,    12,    -1,    -1,    -1,    -1,    17,    -1,    -1,
      -1,    21,    22,    23,    24,    25,    26,    27,    28,    -1,
      30,    31,    32,    33,    34,    72,    -1,    74,    38,    39,
      -1,    -1,    -1,    15,    16,    47,    48,    49,    50,    51,
      52,    -1,    54,    55,    56,    57,    58,    -1,    60,    61,
      62,    63,    64,    35,    -1,    -1,    68,    69,    -1,    -1,
      42,    43,    72,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    -1,    60,    61,
      62,    63,    64,    -1,    66,    67,    68,     5,    70,    -1,
      72,    73,    74,    11,    12,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    21,    22,    23,    24,    25,    26,    27,
      28,    -1,    30,    31,    32,    33,    34,    -1,    -1,    -1,
      -1,    -1,    -1,    41,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    -1,    54,    55,    56,    57,    58,    -1,
      60,    61,    62,    63,    64,     5,    -1,    -1,    68,    69,
      -1,    11,    12,    -1,    -1,    75,    -1,    -1,    -1,    -1,
      -1,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,     5,    60,    61,    62,    63,
      64,    11,    12,    -1,    68,    69,    -1,    -1,    -1,    -1,
      -1,    21,    22,    23,    24,    25,    26,    27,    28,     5,
      30,    31,    32,    33,    34,    11,    12,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    21,    22,    23,    24,    -1,
      -1,    27,    28,    -1,    -1,    31,    32,    33,    34,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    -1,    60,    61,    62,    63,    64,
      -1,    -1,    -1,    68,    69,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    -1,    54,    55,    56,    57,    58,
      -1,    60,    61,    62,    63,    64,    -1,    -1,    -1,    68,
      69,    46,    47,    48,    49,    50,    51,    52,    -1,    54,
      55,    56,    57,    58,    -1,    60,    61,    62,    63,    64,
      -1,    -1,    -1,    68,    69,    48,    49,    50,    51,    52,
      -1,    54,    55,    56,    57,    58,    -1,    60,    61,    62,
      63,    64,    -1,    -1,    -1,    68,    69,    49,    50,    51,
      52,    -1,    54,    55,    56,    57,    58,    -1,    60,    61,
      62,    63,    64,    -1,    -1,    -1,    68,    69,    50,    51,
      52,    -1,    54,    55,    56,    57,    58,    -1,    60,    61,
      62,    63,    64,    -1,    -1,    -1,    68,    69,    52,    -1,
      54,    55,    56,    57,    58,    -1,    60,    61,    62,    63,
      64,    -1,    -1,    -1,    68,    69
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
      49,   105,   109,   114,   115,    42,    53,   106,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    54,    55,    56,
      57,    58,    60,    61,    62,    63,    64,    68,    69,    71,
      71,    78,    53,    62,    71,    91,    41,    97,    41,    42,
     106,    41,    97,    91,    12,    99,    74,    88,   103,   110,
     111,    12,    12,   109,   109,   109,   109,    78,    74,    78,
      41,    53,    41,    94,   114,   114,   114,   114,   114,   114,
     114,   114,   114,   114,   114,   114,   114,   114,   114,   114,
     114,   114,   114,     5,     5,    97,    97,    97,    29,   105,
      78,   110,   110,    41,    96,   104,    78,    78,    41,    53,
      53,    53,    41,    78,   114,   114,    75,   106,    41,    74,
      74,    74,    41,   114,   115,   115,   115,    41,    41,    41
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    76,    77,    78,    78,    78,    79,    79,    80,    80,
      80,    80,    80,    81,    81,    82,    82,    82,    82,    82,
      82,    82,    82,    82,    82,    82,    82,    82,    82,    82,
      82,    82,    82,    82,    82,    82,    82,    82,    82,    82,
      82,    82,    82,    82,    82,    82,    82,    82,    82,    83,
      84,    84,    84,    84,    85,    85,    85,    86,    86,    87,
      87,    87,    87,    88,    88,    89,    89,    90,    90,    91,
      91,    91,    91,    92,    93,    93,    94,    94,    94,    94,
      94,    94,    95,    95,    95,    95,    95,    96,    96,    96,
      97,    97,    98,    98,    98,    99,    99,   100,   100,   100,
     100,   100,   100,   101,   101,   102,   102,   102,   102,   102,
     102,   102,   103,   103,   103,   103,   103,   104,   104,   105,
     105,   105,   105,   105,   105,   105,   105,   105,   105,   105,
     105,   106,   106,   106,   107,   107,   107,   107,   108,   108,
     108,   108,   108,   109,   109,   110,   110,   111,   111,   111,
     111,   112,   112,   112,   112,   112,   113,   113,   113,   113,
     113,   114,   115,   115,   115,   115,   114,   114,   114,   114,
     114,   114,   114,   114,   114,   114,   114,   114,   114,   114,
     114,   114,   114,   114,   114,   114,   114,   114,   114,   114,
     114,   114,   114,   114,   114,   114,   114,   114,   114
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     0,     2,     2,     3,
       3,     2,     2,     2,     4,     2,     2,     4,     4,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     3,     2,     3,     3,     2,
       2,     1,     3,     2,     1,     4,     2,     1,     2,     2,
       1,     2,     1,     1,     1,     1,     2,     2,     1,     2,
       3,     2,     3,     4,     1,     3,     1,     2,     2,     4,
       1,     1,     1,     2,     3,     4,     3,     4,     4,     3,
       0,     1,     1,     1,     1,     1,     2,     1,     1,     1,
       1,     1,     1,     1,     2,     1,     1,     1,     2,     1,
       1,     1,     2,     1,     1,     3,     4,     2,     3,     2,
       1,     3,     2,     2,     1,     3,     2,     3,     2,     4,
       3,     1,     2,     1,     3,     2,     2,     1,     1,     2,
       1,     4,     2,     1,     2,     2,     1,     3,     2,     2,
       1,     2,     1,     1,     4,     4,     4,     2,     2,     2,
       2,     3,     1,     3,     2,     4,     2,     2,     2,     4,
       7,     7,     7,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     5,     1,     1,     4,     1,     1
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
#line 317 "cp-name-parser.y"
                        {
			  state->global_result = (yyvsp[0].comp);

			  /* Avoid warning about "yynerrs" being unused.  */
			  (void) yynerrs;
			}
#line 1910 "cp-name-parser.c.tmp"
    break;

  case 6: /* start_opt: %empty  */
#line 334 "cp-name-parser.y"
                        { (yyval.comp) = NULL; }
#line 1916 "cp-name-parser.c.tmp"
    break;

  case 7: /* start_opt: COLONCOLON start  */
#line 336 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[0].comp); }
#line 1922 "cp-name-parser.c.tmp"
    break;

  case 8: /* function: typespec_2 declarator_1  */
#line 343 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[0].nested).comp;
			  *(yyvsp[0].nested).last = (yyvsp[-1].comp);
			}
#line 1930 "cp-name-parser.c.tmp"
    break;

  case 9: /* function: typespec_2 function_arglist start_opt  */
#line 352 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME,
					  (yyvsp[-2].comp), (yyvsp[-1].nested).comp);
			  if ((yyvsp[0].comp))
			    (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME,
						   (yyval.comp), (yyvsp[0].comp));
			}
#line 1941 "cp-name-parser.c.tmp"
    break;

  case 10: /* function: colon_ext_only function_arglist start_opt  */
#line 359 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, (yyvsp[-2].comp), (yyvsp[-1].nested).comp);
			  if ((yyvsp[0].comp)) (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME, (yyval.comp), (yyvsp[0].comp)); }
#line 1948 "cp-name-parser.c.tmp"
    break;

  case 11: /* function: conversion_op_name start_opt  */
#line 363 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[-1].nested).comp;
			  if ((yyvsp[0].comp)) (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME, (yyval.comp), (yyvsp[0].comp)); }
#line 1955 "cp-name-parser.c.tmp"
    break;

  case 12: /* function: conversion_op_name abstract_declarator_fn  */
#line 366 "cp-name-parser.y"
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
#line 1975 "cp-name-parser.c.tmp"
    break;

  case 13: /* demangler_special: DEMANGLER_SPECIAL start  */
#line 385 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp ((enum demangle_component_type) (yyvsp[-1].lval), (yyvsp[0].comp), NULL); }
#line 1981 "cp-name-parser.c.tmp"
    break;

  case 14: /* demangler_special: CONSTRUCTION_VTABLE start CONSTRUCTION_IN start  */
#line 387 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_CONSTRUCTION_VTABLE, (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 1987 "cp-name-parser.c.tmp"
    break;

  case 15: /* oper: OPERATOR NEW  */
#line 391 "cp-name-parser.y"
                        {
			  /* Match the whitespacing of cplus_demangle_operators.
			     It would abort on unrecognized string otherwise.  */
			  (yyval.comp) = state->make_operator ("new", 3);
			}
#line 1997 "cp-name-parser.c.tmp"
    break;

  case 16: /* oper: OPERATOR DELETE  */
#line 397 "cp-name-parser.y"
                        {
			  /* Match the whitespacing of cplus_demangle_operators.
			     It would abort on unrecognized string otherwise.  */
			  (yyval.comp) = state->make_operator ("delete ", 1);
			}
#line 2007 "cp-name-parser.c.tmp"
    break;

  case 17: /* oper: OPERATOR NEW '[' ']'  */
#line 403 "cp-name-parser.y"
                        {
			  /* Match the whitespacing of cplus_demangle_operators.
			     It would abort on unrecognized string otherwise.  */
			  (yyval.comp) = state->make_operator ("new[]", 3);
			}
#line 2017 "cp-name-parser.c.tmp"
    break;

  case 18: /* oper: OPERATOR DELETE '[' ']'  */
#line 409 "cp-name-parser.y"
                        {
			  /* Match the whitespacing of cplus_demangle_operators.
			     It would abort on unrecognized string otherwise.  */
			  (yyval.comp) = state->make_operator ("delete[] ", 1);
			}
#line 2027 "cp-name-parser.c.tmp"
    break;

  case 19: /* oper: OPERATOR '+'  */
#line 415 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("+", 2); }
#line 2033 "cp-name-parser.c.tmp"
    break;

  case 20: /* oper: OPERATOR '-'  */
#line 417 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("-", 2); }
#line 2039 "cp-name-parser.c.tmp"
    break;

  case 21: /* oper: OPERATOR '*'  */
#line 419 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("*", 2); }
#line 2045 "cp-name-parser.c.tmp"
    break;

  case 22: /* oper: OPERATOR '/'  */
#line 421 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("/", 2); }
#line 2051 "cp-name-parser.c.tmp"
    break;

  case 23: /* oper: OPERATOR '%'  */
#line 423 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("%", 2); }
#line 2057 "cp-name-parser.c.tmp"
    break;

  case 24: /* oper: OPERATOR '^'  */
#line 425 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("^", 2); }
#line 2063 "cp-name-parser.c.tmp"
    break;

  case 25: /* oper: OPERATOR '&'  */
#line 427 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("&", 2); }
#line 2069 "cp-name-parser.c.tmp"
    break;

  case 26: /* oper: OPERATOR '|'  */
#line 429 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("|", 2); }
#line 2075 "cp-name-parser.c.tmp"
    break;

  case 27: /* oper: OPERATOR '~'  */
#line 431 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("~", 1); }
#line 2081 "cp-name-parser.c.tmp"
    break;

  case 28: /* oper: OPERATOR '!'  */
#line 433 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("!", 1); }
#line 2087 "cp-name-parser.c.tmp"
    break;

  case 29: /* oper: OPERATOR '='  */
#line 435 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("=", 2); }
#line 2093 "cp-name-parser.c.tmp"
    break;

  case 30: /* oper: OPERATOR '<'  */
#line 437 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("<", 2); }
#line 2099 "cp-name-parser.c.tmp"
    break;

  case 31: /* oper: OPERATOR '>'  */
#line 439 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator (">", 2); }
#line 2105 "cp-name-parser.c.tmp"
    break;

  case 32: /* oper: OPERATOR ASSIGN_MODIFY  */
#line 441 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ((yyvsp[0].opname), 2); }
#line 2111 "cp-name-parser.c.tmp"
    break;

  case 33: /* oper: OPERATOR LSH  */
#line 443 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("<<", 2); }
#line 2117 "cp-name-parser.c.tmp"
    break;

  case 34: /* oper: OPERATOR RSH  */
#line 445 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator (">>", 2); }
#line 2123 "cp-name-parser.c.tmp"
    break;

  case 35: /* oper: OPERATOR EQUAL  */
#line 447 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("==", 2); }
#line 2129 "cp-name-parser.c.tmp"
    break;

  case 36: /* oper: OPERATOR NOTEQUAL  */
#line 449 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("!=", 2); }
#line 2135 "cp-name-parser.c.tmp"
    break;

  case 37: /* oper: OPERATOR LEQ  */
#line 451 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("<=", 2); }
#line 2141 "cp-name-parser.c.tmp"
    break;

  case 38: /* oper: OPERATOR GEQ  */
#line 453 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator (">=", 2); }
#line 2147 "cp-name-parser.c.tmp"
    break;

  case 39: /* oper: OPERATOR SPACESHIP  */
#line 455 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("<=>", 2); }
#line 2153 "cp-name-parser.c.tmp"
    break;

  case 40: /* oper: OPERATOR ANDAND  */
#line 457 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("&&", 2); }
#line 2159 "cp-name-parser.c.tmp"
    break;

  case 41: /* oper: OPERATOR OROR  */
#line 459 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("||", 2); }
#line 2165 "cp-name-parser.c.tmp"
    break;

  case 42: /* oper: OPERATOR INCREMENT  */
#line 461 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("++", 1); }
#line 2171 "cp-name-parser.c.tmp"
    break;

  case 43: /* oper: OPERATOR DECREMENT  */
#line 463 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("--", 1); }
#line 2177 "cp-name-parser.c.tmp"
    break;

  case 44: /* oper: OPERATOR ','  */
#line 465 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator (",", 2); }
#line 2183 "cp-name-parser.c.tmp"
    break;

  case 45: /* oper: OPERATOR ARROW '*'  */
#line 467 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("->*", 2); }
#line 2189 "cp-name-parser.c.tmp"
    break;

  case 46: /* oper: OPERATOR ARROW  */
#line 469 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("->", 2); }
#line 2195 "cp-name-parser.c.tmp"
    break;

  case 47: /* oper: OPERATOR '(' ')'  */
#line 471 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("()", 2); }
#line 2201 "cp-name-parser.c.tmp"
    break;

  case 48: /* oper: OPERATOR '[' ']'  */
#line 473 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("[]", 2); }
#line 2207 "cp-name-parser.c.tmp"
    break;

  case 49: /* conversion_op: OPERATOR typespec_2  */
#line 481 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_CONVERSION, (yyvsp[0].comp), NULL); }
#line 2213 "cp-name-parser.c.tmp"
    break;

  case 50: /* conversion_op_name: nested_name conversion_op  */
#line 486 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-1].nested1).comp;
			  d_right ((yyvsp[-1].nested1).last) = (yyvsp[0].comp);
			  (yyval.nested).last = &d_left ((yyvsp[0].comp));
			}
#line 2222 "cp-name-parser.c.tmp"
    break;

  case 51: /* conversion_op_name: conversion_op  */
#line 491 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[0].comp);
			  (yyval.nested).last = &d_left ((yyvsp[0].comp));
			}
#line 2230 "cp-name-parser.c.tmp"
    break;

  case 52: /* conversion_op_name: COLONCOLON nested_name conversion_op  */
#line 495 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-1].nested1).comp;
			  d_right ((yyvsp[-1].nested1).last) = (yyvsp[0].comp);
			  (yyval.nested).last = &d_left ((yyvsp[0].comp));
			}
#line 2239 "cp-name-parser.c.tmp"
    break;

  case 53: /* conversion_op_name: COLONCOLON conversion_op  */
#line 500 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[0].comp);
			  (yyval.nested).last = &d_left ((yyvsp[0].comp));
			}
#line 2247 "cp-name-parser.c.tmp"
    break;

  case 55: /* unqualified_name: oper '<' template_params '>'  */
#line 509 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_TEMPLATE, (yyvsp[-3].comp), (yyvsp[-1].nested).comp); }
#line 2253 "cp-name-parser.c.tmp"
    break;

  case 56: /* unqualified_name: '~' NAME  */
#line 511 "cp-name-parser.y"
                        { (yyval.comp) = state->make_dtor (gnu_v3_complete_object_dtor, (yyvsp[0].comp)); }
#line 2259 "cp-name-parser.c.tmp"
    break;

  case 58: /* colon_name: COLONCOLON name  */
#line 524 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[0].comp); }
#line 2265 "cp-name-parser.c.tmp"
    break;

  case 59: /* name: nested_name NAME  */
#line 530 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[-1].nested1).comp; d_right ((yyvsp[-1].nested1).last) = (yyvsp[0].comp); }
#line 2271 "cp-name-parser.c.tmp"
    break;

  case 61: /* name: nested_name templ  */
#line 533 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[-1].nested1).comp; d_right ((yyvsp[-1].nested1).last) = (yyvsp[0].comp); }
#line 2277 "cp-name-parser.c.tmp"
    break;

  case 66: /* colon_ext_only: COLONCOLON ext_only_name  */
#line 543 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[0].comp); }
#line 2283 "cp-name-parser.c.tmp"
    break;

  case 67: /* ext_only_name: nested_name unqualified_name  */
#line 547 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[-1].nested1).comp; d_right ((yyvsp[-1].nested1).last) = (yyvsp[0].comp); }
#line 2289 "cp-name-parser.c.tmp"
    break;

  case 69: /* nested_name: NAME COLONCOLON  */
#line 552 "cp-name-parser.y"
                        { (yyval.nested1).comp = state->fill_comp (DEMANGLE_COMPONENT_QUAL_NAME, (yyvsp[-1].comp), NULL);
			  (yyval.nested1).last = (yyval.nested1).comp;
			}
#line 2297 "cp-name-parser.c.tmp"
    break;

  case 70: /* nested_name: nested_name NAME COLONCOLON  */
#line 556 "cp-name-parser.y"
                        { (yyval.nested1).comp = (yyvsp[-2].nested1).comp;
			  d_right ((yyvsp[-2].nested1).last) = state->fill_comp (DEMANGLE_COMPONENT_QUAL_NAME, (yyvsp[-1].comp), NULL);
			  (yyval.nested1).last = d_right ((yyvsp[-2].nested1).last);
			}
#line 2306 "cp-name-parser.c.tmp"
    break;

  case 71: /* nested_name: templ COLONCOLON  */
#line 561 "cp-name-parser.y"
                        { (yyval.nested1).comp = state->fill_comp (DEMANGLE_COMPONENT_QUAL_NAME, (yyvsp[-1].comp), NULL);
			  (yyval.nested1).last = (yyval.nested1).comp;
			}
#line 2314 "cp-name-parser.c.tmp"
    break;

  case 72: /* nested_name: nested_name templ COLONCOLON  */
#line 565 "cp-name-parser.y"
                        { (yyval.nested1).comp = (yyvsp[-2].nested1).comp;
			  d_right ((yyvsp[-2].nested1).last) = state->fill_comp (DEMANGLE_COMPONENT_QUAL_NAME, (yyvsp[-1].comp), NULL);
			  (yyval.nested1).last = d_right ((yyvsp[-2].nested1).last);
			}
#line 2323 "cp-name-parser.c.tmp"
    break;

  case 73: /* templ: NAME '<' template_params '>'  */
#line 574 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_TEMPLATE, (yyvsp[-3].comp), (yyvsp[-1].nested).comp); }
#line 2329 "cp-name-parser.c.tmp"
    break;

  case 74: /* template_params: template_arg  */
#line 578 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_TEMPLATE_ARGLIST, (yyvsp[0].comp), NULL);
			(yyval.nested).last = &d_right ((yyval.nested).comp); }
#line 2336 "cp-name-parser.c.tmp"
    break;

  case 75: /* template_params: template_params ',' template_arg  */
#line 581 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-2].nested).comp;
			  *(yyvsp[-2].nested).last = state->fill_comp (DEMANGLE_COMPONENT_TEMPLATE_ARGLIST, (yyvsp[0].comp), NULL);
			  (yyval.nested).last = &d_right (*(yyvsp[-2].nested).last);
			}
#line 2345 "cp-name-parser.c.tmp"
    break;

  case 77: /* template_arg: typespec_2 abstract_declarator  */
#line 593 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[0].abstract).comp;
			  *(yyvsp[0].abstract).last = (yyvsp[-1].comp);
			}
#line 2353 "cp-name-parser.c.tmp"
    break;

  case 78: /* template_arg: '&' start  */
#line 597 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_UNARY, state->make_operator ("&", 1), (yyvsp[0].comp)); }
#line 2359 "cp-name-parser.c.tmp"
    break;

  case 79: /* template_arg: '&' '(' start ')'  */
#line 599 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_UNARY, state->make_operator ("&", 1), (yyvsp[-1].comp)); }
#line 2365 "cp-name-parser.c.tmp"
    break;

  case 82: /* function_args: typespec_2  */
#line 605 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_ARGLIST, (yyvsp[0].comp), NULL);
			  (yyval.nested).last = &d_right ((yyval.nested).comp);
			}
#line 2373 "cp-name-parser.c.tmp"
    break;

  case 83: /* function_args: typespec_2 abstract_declarator  */
#line 609 "cp-name-parser.y"
                        { *(yyvsp[0].abstract).last = (yyvsp[-1].comp);
			  (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_ARGLIST, (yyvsp[0].abstract).comp, NULL);
			  (yyval.nested).last = &d_right ((yyval.nested).comp);
			}
#line 2382 "cp-name-parser.c.tmp"
    break;

  case 84: /* function_args: function_args ',' typespec_2  */
#line 614 "cp-name-parser.y"
                        { *(yyvsp[-2].nested).last = state->fill_comp (DEMANGLE_COMPONENT_ARGLIST, (yyvsp[0].comp), NULL);
			  (yyval.nested).comp = (yyvsp[-2].nested).comp;
			  (yyval.nested).last = &d_right (*(yyvsp[-2].nested).last);
			}
#line 2391 "cp-name-parser.c.tmp"
    break;

  case 85: /* function_args: function_args ',' typespec_2 abstract_declarator  */
#line 619 "cp-name-parser.y"
                        { *(yyvsp[0].abstract).last = (yyvsp[-1].comp);
			  *(yyvsp[-3].nested).last = state->fill_comp (DEMANGLE_COMPONENT_ARGLIST, (yyvsp[0].abstract).comp, NULL);
			  (yyval.nested).comp = (yyvsp[-3].nested).comp;
			  (yyval.nested).last = &d_right (*(yyvsp[-3].nested).last);
			}
#line 2401 "cp-name-parser.c.tmp"
    break;

  case 86: /* function_args: function_args ',' ELLIPSIS  */
#line 625 "cp-name-parser.y"
                        { *(yyvsp[-2].nested).last
			    = state->fill_comp (DEMANGLE_COMPONENT_ARGLIST,
					   state->make_builtin_type ("..."),
					   NULL);
			  (yyval.nested).comp = (yyvsp[-2].nested).comp;
			  (yyval.nested).last = &d_right (*(yyvsp[-2].nested).last);
			}
#line 2413 "cp-name-parser.c.tmp"
    break;

  case 87: /* function_arglist: '(' function_args ')' qualifiers_opt  */
#line 635 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_FUNCTION_TYPE, NULL, (yyvsp[-2].nested).comp);
			  (yyval.nested).last = &d_left ((yyval.nested).comp);
			  (yyval.nested).comp = state->d_qualify ((yyval.nested).comp, (yyvsp[0].lval), 1); }
#line 2421 "cp-name-parser.c.tmp"
    break;

  case 88: /* function_arglist: '(' VOID ')' qualifiers_opt  */
#line 639 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_FUNCTION_TYPE, NULL, NULL);
			  (yyval.nested).last = &d_left ((yyval.nested).comp);
			  (yyval.nested).comp = state->d_qualify ((yyval.nested).comp, (yyvsp[0].lval), 1); }
#line 2429 "cp-name-parser.c.tmp"
    break;

  case 89: /* function_arglist: '(' ')' qualifiers_opt  */
#line 643 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_FUNCTION_TYPE, NULL, NULL);
			  (yyval.nested).last = &d_left ((yyval.nested).comp);
			  (yyval.nested).comp = state->d_qualify ((yyval.nested).comp, (yyvsp[0].lval), 1); }
#line 2437 "cp-name-parser.c.tmp"
    break;

  case 90: /* qualifiers_opt: %empty  */
#line 650 "cp-name-parser.y"
                        { (yyval.lval) = 0; }
#line 2443 "cp-name-parser.c.tmp"
    break;

  case 92: /* qualifier: RESTRICT  */
#line 655 "cp-name-parser.y"
                        { (yyval.lval) = QUAL_RESTRICT; }
#line 2449 "cp-name-parser.c.tmp"
    break;

  case 93: /* qualifier: VOLATILE_KEYWORD  */
#line 657 "cp-name-parser.y"
                        { (yyval.lval) = QUAL_VOLATILE; }
#line 2455 "cp-name-parser.c.tmp"
    break;

  case 94: /* qualifier: CONST_KEYWORD  */
#line 659 "cp-name-parser.y"
                        { (yyval.lval) = QUAL_CONST; }
#line 2461 "cp-name-parser.c.tmp"
    break;

  case 96: /* qualifiers: qualifier qualifiers  */
#line 664 "cp-name-parser.y"
                        { (yyval.lval) = (yyvsp[-1].lval) | (yyvsp[0].lval); }
#line 2467 "cp-name-parser.c.tmp"
    break;

  case 97: /* int_part: INT_KEYWORD  */
#line 671 "cp-name-parser.y"
                        { (yyval.lval) = 0; }
#line 2473 "cp-name-parser.c.tmp"
    break;

  case 98: /* int_part: SIGNED_KEYWORD  */
#line 673 "cp-name-parser.y"
                        { (yyval.lval) = INT_SIGNED; }
#line 2479 "cp-name-parser.c.tmp"
    break;

  case 99: /* int_part: UNSIGNED  */
#line 675 "cp-name-parser.y"
                        { (yyval.lval) = INT_UNSIGNED; }
#line 2485 "cp-name-parser.c.tmp"
    break;

  case 100: /* int_part: CHAR  */
#line 677 "cp-name-parser.y"
                        { (yyval.lval) = INT_CHAR; }
#line 2491 "cp-name-parser.c.tmp"
    break;

  case 101: /* int_part: LONG  */
#line 679 "cp-name-parser.y"
                        { (yyval.lval) = INT_LONG; }
#line 2497 "cp-name-parser.c.tmp"
    break;

  case 102: /* int_part: SHORT  */
#line 681 "cp-name-parser.y"
                        { (yyval.lval) = INT_SHORT; }
#line 2503 "cp-name-parser.c.tmp"
    break;

  case 104: /* int_seq: int_seq int_part  */
#line 686 "cp-name-parser.y"
                        { (yyval.lval) = (yyvsp[-1].lval) | (yyvsp[0].lval); if ((yyvsp[-1].lval) & (yyvsp[0].lval) & INT_LONG) (yyval.lval) = (yyvsp[-1].lval) | INT_LLONG; }
#line 2509 "cp-name-parser.c.tmp"
    break;

  case 105: /* builtin_type: int_seq  */
#line 690 "cp-name-parser.y"
                        { (yyval.comp) = state->d_int_type ((yyvsp[0].lval)); }
#line 2515 "cp-name-parser.c.tmp"
    break;

  case 106: /* builtin_type: FLOAT_KEYWORD  */
#line 692 "cp-name-parser.y"
                        { (yyval.comp) = state->make_builtin_type ("float"); }
#line 2521 "cp-name-parser.c.tmp"
    break;

  case 107: /* builtin_type: DOUBLE_KEYWORD  */
#line 694 "cp-name-parser.y"
                        { (yyval.comp) = state->make_builtin_type ("double"); }
#line 2527 "cp-name-parser.c.tmp"
    break;

  case 108: /* builtin_type: LONG DOUBLE_KEYWORD  */
#line 696 "cp-name-parser.y"
                        { (yyval.comp) = state->make_builtin_type ("long double"); }
#line 2533 "cp-name-parser.c.tmp"
    break;

  case 109: /* builtin_type: BOOL  */
#line 698 "cp-name-parser.y"
                        { (yyval.comp) = state->make_builtin_type ("bool"); }
#line 2539 "cp-name-parser.c.tmp"
    break;

  case 110: /* builtin_type: WCHAR_T  */
#line 700 "cp-name-parser.y"
                        { (yyval.comp) = state->make_builtin_type ("wchar_t"); }
#line 2545 "cp-name-parser.c.tmp"
    break;

  case 111: /* builtin_type: VOID  */
#line 702 "cp-name-parser.y"
                        { (yyval.comp) = state->make_builtin_type ("void"); }
#line 2551 "cp-name-parser.c.tmp"
    break;

  case 112: /* ptr_operator: '*' qualifiers_opt  */
#line 706 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_POINTER, NULL, NULL);
			  (yyval.nested).last = &d_left ((yyval.nested).comp);
			  (yyval.nested).comp = state->d_qualify ((yyval.nested).comp, (yyvsp[0].lval), 0); }
#line 2559 "cp-name-parser.c.tmp"
    break;

  case 113: /* ptr_operator: '&'  */
#line 711 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_REFERENCE, NULL, NULL);
			  (yyval.nested).last = &d_left ((yyval.nested).comp); }
#line 2566 "cp-name-parser.c.tmp"
    break;

  case 114: /* ptr_operator: ANDAND  */
#line 714 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_RVALUE_REFERENCE, NULL, NULL);
			  (yyval.nested).last = &d_left ((yyval.nested).comp); }
#line 2573 "cp-name-parser.c.tmp"
    break;

  case 115: /* ptr_operator: nested_name '*' qualifiers_opt  */
#line 717 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_PTRMEM_TYPE, (yyvsp[-2].nested1).comp, NULL);
			  /* Convert the innermost DEMANGLE_COMPONENT_QUAL_NAME to a DEMANGLE_COMPONENT_NAME.  */
			  *(yyvsp[-2].nested1).last = *d_left ((yyvsp[-2].nested1).last);
			  (yyval.nested).last = &d_right ((yyval.nested).comp);
			  (yyval.nested).comp = state->d_qualify ((yyval.nested).comp, (yyvsp[0].lval), 0); }
#line 2583 "cp-name-parser.c.tmp"
    break;

  case 116: /* ptr_operator: COLONCOLON nested_name '*' qualifiers_opt  */
#line 723 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_PTRMEM_TYPE, (yyvsp[-2].nested1).comp, NULL);
			  /* Convert the innermost DEMANGLE_COMPONENT_QUAL_NAME to a DEMANGLE_COMPONENT_NAME.  */
			  *(yyvsp[-2].nested1).last = *d_left ((yyvsp[-2].nested1).last);
			  (yyval.nested).last = &d_right ((yyval.nested).comp);
			  (yyval.nested).comp = state->d_qualify ((yyval.nested).comp, (yyvsp[0].lval), 0); }
#line 2593 "cp-name-parser.c.tmp"
    break;

  case 117: /* array_indicator: '[' ']'  */
#line 731 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_ARRAY_TYPE, NULL, NULL); }
#line 2599 "cp-name-parser.c.tmp"
    break;

  case 118: /* array_indicator: '[' INT ']'  */
#line 733 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_ARRAY_TYPE, (yyvsp[-1].comp), NULL); }
#line 2605 "cp-name-parser.c.tmp"
    break;

  case 119: /* typespec_2: builtin_type qualifiers  */
#line 747 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[-1].comp), (yyvsp[0].lval), 0); }
#line 2611 "cp-name-parser.c.tmp"
    break;

  case 121: /* typespec_2: qualifiers builtin_type qualifiers  */
#line 750 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[-1].comp), (yyvsp[-2].lval) | (yyvsp[0].lval), 0); }
#line 2617 "cp-name-parser.c.tmp"
    break;

  case 122: /* typespec_2: qualifiers builtin_type  */
#line 752 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[0].comp), (yyvsp[-1].lval), 0); }
#line 2623 "cp-name-parser.c.tmp"
    break;

  case 123: /* typespec_2: name qualifiers  */
#line 755 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[-1].comp), (yyvsp[0].lval), 0); }
#line 2629 "cp-name-parser.c.tmp"
    break;

  case 125: /* typespec_2: qualifiers name qualifiers  */
#line 758 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[-1].comp), (yyvsp[-2].lval) | (yyvsp[0].lval), 0); }
#line 2635 "cp-name-parser.c.tmp"
    break;

  case 126: /* typespec_2: qualifiers name  */
#line 760 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[0].comp), (yyvsp[-1].lval), 0); }
#line 2641 "cp-name-parser.c.tmp"
    break;

  case 127: /* typespec_2: COLONCOLON name qualifiers  */
#line 763 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[-1].comp), (yyvsp[0].lval), 0); }
#line 2647 "cp-name-parser.c.tmp"
    break;

  case 128: /* typespec_2: COLONCOLON name  */
#line 765 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[0].comp); }
#line 2653 "cp-name-parser.c.tmp"
    break;

  case 129: /* typespec_2: qualifiers COLONCOLON name qualifiers  */
#line 767 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[-1].comp), (yyvsp[-3].lval) | (yyvsp[0].lval), 0); }
#line 2659 "cp-name-parser.c.tmp"
    break;

  case 130: /* typespec_2: qualifiers COLONCOLON name  */
#line 769 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[0].comp), (yyvsp[-2].lval), 0); }
#line 2665 "cp-name-parser.c.tmp"
    break;

  case 131: /* abstract_declarator: ptr_operator  */
#line 774 "cp-name-parser.y"
                        { (yyval.abstract).comp = (yyvsp[0].nested).comp; (yyval.abstract).last = (yyvsp[0].nested).last;
			  (yyval.abstract).fn.comp = NULL; (yyval.abstract).fn.last = NULL; }
#line 2672 "cp-name-parser.c.tmp"
    break;

  case 132: /* abstract_declarator: ptr_operator abstract_declarator  */
#line 777 "cp-name-parser.y"
                        { (yyval.abstract) = (yyvsp[0].abstract); (yyval.abstract).fn.comp = NULL; (yyval.abstract).fn.last = NULL;
			  if ((yyvsp[0].abstract).fn.comp) { (yyval.abstract).last = (yyvsp[0].abstract).fn.last; *(yyvsp[0].abstract).last = (yyvsp[0].abstract).fn.comp; }
			  *(yyval.abstract).last = (yyvsp[-1].nested).comp;
			  (yyval.abstract).last = (yyvsp[-1].nested).last; }
#line 2681 "cp-name-parser.c.tmp"
    break;

  case 133: /* abstract_declarator: direct_abstract_declarator  */
#line 782 "cp-name-parser.y"
                        { (yyval.abstract).fn.comp = NULL; (yyval.abstract).fn.last = NULL;
			  if ((yyvsp[0].abstract).fn.comp) { (yyval.abstract).last = (yyvsp[0].abstract).fn.last; *(yyvsp[0].abstract).last = (yyvsp[0].abstract).fn.comp; }
			}
#line 2689 "cp-name-parser.c.tmp"
    break;

  case 134: /* direct_abstract_declarator: '(' abstract_declarator ')'  */
#line 789 "cp-name-parser.y"
                        { (yyval.abstract) = (yyvsp[-1].abstract); (yyval.abstract).fn.comp = NULL; (yyval.abstract).fn.last = NULL; (yyval.abstract).fold_flag = 1;
			  if ((yyvsp[-1].abstract).fn.comp) { (yyval.abstract).last = (yyvsp[-1].abstract).fn.last; *(yyvsp[-1].abstract).last = (yyvsp[-1].abstract).fn.comp; }
			}
#line 2697 "cp-name-parser.c.tmp"
    break;

  case 135: /* direct_abstract_declarator: direct_abstract_declarator function_arglist  */
#line 793 "cp-name-parser.y"
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
#line 2712 "cp-name-parser.c.tmp"
    break;

  case 136: /* direct_abstract_declarator: direct_abstract_declarator array_indicator  */
#line 804 "cp-name-parser.y"
                        { (yyval.abstract).fn.comp = NULL; (yyval.abstract).fn.last = NULL; (yyval.abstract).fold_flag = 0;
			  if ((yyvsp[-1].abstract).fn.comp) { (yyval.abstract).last = (yyvsp[-1].abstract).fn.last; *(yyvsp[-1].abstract).last = (yyvsp[-1].abstract).fn.comp; }
			  *(yyvsp[-1].abstract).last = (yyvsp[0].comp);
			  (yyval.abstract).last = &d_right ((yyvsp[0].comp));
			}
#line 2722 "cp-name-parser.c.tmp"
    break;

  case 137: /* direct_abstract_declarator: array_indicator  */
#line 810 "cp-name-parser.y"
                        { (yyval.abstract).fn.comp = NULL; (yyval.abstract).fn.last = NULL; (yyval.abstract).fold_flag = 0;
			  (yyval.abstract).comp = (yyvsp[0].comp);
			  (yyval.abstract).last = &d_right ((yyvsp[0].comp));
			}
#line 2731 "cp-name-parser.c.tmp"
    break;

  case 138: /* abstract_declarator_fn: ptr_operator  */
#line 828 "cp-name-parser.y"
                        { (yyval.abstract).comp = (yyvsp[0].nested).comp; (yyval.abstract).last = (yyvsp[0].nested).last;
			  (yyval.abstract).fn.comp = NULL; (yyval.abstract).fn.last = NULL; (yyval.abstract).start = NULL; }
#line 2738 "cp-name-parser.c.tmp"
    break;

  case 139: /* abstract_declarator_fn: ptr_operator abstract_declarator_fn  */
#line 831 "cp-name-parser.y"
                        { (yyval.abstract) = (yyvsp[0].abstract);
			  if ((yyvsp[0].abstract).last)
			    *(yyval.abstract).last = (yyvsp[-1].nested).comp;
			  else
			    (yyval.abstract).comp = (yyvsp[-1].nested).comp;
			  (yyval.abstract).last = (yyvsp[-1].nested).last;
			}
#line 2750 "cp-name-parser.c.tmp"
    break;

  case 140: /* abstract_declarator_fn: direct_abstract_declarator  */
#line 839 "cp-name-parser.y"
                        { (yyval.abstract).comp = (yyvsp[0].abstract).comp; (yyval.abstract).last = (yyvsp[0].abstract).last; (yyval.abstract).fn = (yyvsp[0].abstract).fn; (yyval.abstract).start = NULL; }
#line 2756 "cp-name-parser.c.tmp"
    break;

  case 141: /* abstract_declarator_fn: direct_abstract_declarator function_arglist COLONCOLON start  */
#line 841 "cp-name-parser.y"
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
#line 2771 "cp-name-parser.c.tmp"
    break;

  case 142: /* abstract_declarator_fn: function_arglist start_opt  */
#line 852 "cp-name-parser.y"
                        { (yyval.abstract).fn = (yyvsp[-1].nested);
			  (yyval.abstract).start = (yyvsp[0].comp);
			  (yyval.abstract).comp = NULL; (yyval.abstract).last = NULL;
			}
#line 2780 "cp-name-parser.c.tmp"
    break;

  case 144: /* type: typespec_2 abstract_declarator  */
#line 860 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[0].abstract).comp;
			  *(yyvsp[0].abstract).last = (yyvsp[-1].comp);
			}
#line 2788 "cp-name-parser.c.tmp"
    break;

  case 145: /* declarator: ptr_operator declarator  */
#line 866 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[0].nested).comp;
			  (yyval.nested).last = (yyvsp[-1].nested).last;
			  *(yyvsp[0].nested).last = (yyvsp[-1].nested).comp; }
#line 2796 "cp-name-parser.c.tmp"
    break;

  case 147: /* direct_declarator: '(' declarator ')'  */
#line 874 "cp-name-parser.y"
                        { (yyval.nested) = (yyvsp[-1].nested); }
#line 2802 "cp-name-parser.c.tmp"
    break;

  case 148: /* direct_declarator: direct_declarator function_arglist  */
#line 876 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-1].nested).comp;
			  *(yyvsp[-1].nested).last = (yyvsp[0].nested).comp;
			  (yyval.nested).last = (yyvsp[0].nested).last;
			}
#line 2811 "cp-name-parser.c.tmp"
    break;

  case 149: /* direct_declarator: direct_declarator array_indicator  */
#line 881 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-1].nested).comp;
			  *(yyvsp[-1].nested).last = (yyvsp[0].comp);
			  (yyval.nested).last = &d_right ((yyvsp[0].comp));
			}
#line 2820 "cp-name-parser.c.tmp"
    break;

  case 150: /* direct_declarator: colon_ext_name  */
#line 886 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, (yyvsp[0].comp), NULL);
			  (yyval.nested).last = &d_right ((yyval.nested).comp);
			}
#line 2828 "cp-name-parser.c.tmp"
    break;

  case 151: /* declarator_1: ptr_operator declarator_1  */
#line 898 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[0].nested).comp;
			  (yyval.nested).last = (yyvsp[-1].nested).last;
			  *(yyvsp[0].nested).last = (yyvsp[-1].nested).comp; }
#line 2836 "cp-name-parser.c.tmp"
    break;

  case 152: /* declarator_1: colon_ext_name  */
#line 902 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, (yyvsp[0].comp), NULL);
			  (yyval.nested).last = &d_right ((yyval.nested).comp);
			}
#line 2844 "cp-name-parser.c.tmp"
    break;

  case 154: /* declarator_1: colon_ext_name function_arglist COLONCOLON start  */
#line 914 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, (yyvsp[-3].comp), (yyvsp[-2].nested).comp);
			  (yyval.nested).last = (yyvsp[-2].nested).last;
			  (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME, (yyval.nested).comp, (yyvsp[0].comp));
			}
#line 2853 "cp-name-parser.c.tmp"
    break;

  case 155: /* declarator_1: direct_declarator_1 function_arglist COLONCOLON start  */
#line 919 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-3].nested).comp;
			  *(yyvsp[-3].nested).last = (yyvsp[-2].nested).comp;
			  (yyval.nested).last = (yyvsp[-2].nested).last;
			  (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME, (yyval.nested).comp, (yyvsp[0].comp));
			}
#line 2863 "cp-name-parser.c.tmp"
    break;

  case 156: /* direct_declarator_1: '(' ptr_operator declarator ')'  */
#line 928 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-1].nested).comp;
			  (yyval.nested).last = (yyvsp[-2].nested).last;
			  *(yyvsp[-1].nested).last = (yyvsp[-2].nested).comp; }
#line 2871 "cp-name-parser.c.tmp"
    break;

  case 157: /* direct_declarator_1: direct_declarator_1 function_arglist  */
#line 932 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-1].nested).comp;
			  *(yyvsp[-1].nested).last = (yyvsp[0].nested).comp;
			  (yyval.nested).last = (yyvsp[0].nested).last;
			}
#line 2880 "cp-name-parser.c.tmp"
    break;

  case 158: /* direct_declarator_1: direct_declarator_1 array_indicator  */
#line 937 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-1].nested).comp;
			  *(yyvsp[-1].nested).last = (yyvsp[0].comp);
			  (yyval.nested).last = &d_right ((yyvsp[0].comp));
			}
#line 2889 "cp-name-parser.c.tmp"
    break;

  case 159: /* direct_declarator_1: colon_ext_name function_arglist  */
#line 942 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, (yyvsp[-1].comp), (yyvsp[0].nested).comp);
			  (yyval.nested).last = (yyvsp[0].nested).last;
			}
#line 2897 "cp-name-parser.c.tmp"
    break;

  case 160: /* direct_declarator_1: colon_ext_name array_indicator  */
#line 946 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, (yyvsp[-1].comp), (yyvsp[0].comp));
			  (yyval.nested).last = &d_right ((yyvsp[0].comp));
			}
#line 2905 "cp-name-parser.c.tmp"
    break;

  case 161: /* exp: '(' exp1 ')'  */
#line 952 "cp-name-parser.y"
                { (yyval.comp) = (yyvsp[-1].comp); }
#line 2911 "cp-name-parser.c.tmp"
    break;

  case 163: /* exp1: exp '>' exp  */
#line 961 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary (">", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 2917 "cp-name-parser.c.tmp"
    break;

  case 164: /* exp1: '&' start  */
#line 968 "cp-name-parser.y"
                { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_UNARY, state->make_operator ("&", 1), (yyvsp[0].comp)); }
#line 2923 "cp-name-parser.c.tmp"
    break;

  case 165: /* exp1: '&' '(' start ')'  */
#line 970 "cp-name-parser.y"
                { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_UNARY, state->make_operator ("&", 1), (yyvsp[-1].comp)); }
#line 2929 "cp-name-parser.c.tmp"
    break;

  case 166: /* exp: '-' exp  */
#line 975 "cp-name-parser.y"
                { (yyval.comp) = state->d_unary ("-", (yyvsp[0].comp)); }
#line 2935 "cp-name-parser.c.tmp"
    break;

  case 167: /* exp: '!' exp  */
#line 979 "cp-name-parser.y"
                { (yyval.comp) = state->d_unary ("!", (yyvsp[0].comp)); }
#line 2941 "cp-name-parser.c.tmp"
    break;

  case 168: /* exp: '~' exp  */
#line 983 "cp-name-parser.y"
                { (yyval.comp) = state->d_unary ("~", (yyvsp[0].comp)); }
#line 2947 "cp-name-parser.c.tmp"
    break;

  case 169: /* exp: '(' type ')' exp  */
#line 990 "cp-name-parser.y"
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
#line 2963 "cp-name-parser.c.tmp"
    break;

  case 170: /* exp: STATIC_CAST '<' type '>' '(' exp1 ')'  */
#line 1006 "cp-name-parser.y"
                { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_UNARY,
				    state->fill_comp (DEMANGLE_COMPONENT_CAST, (yyvsp[-4].comp), NULL),
				    (yyvsp[-1].comp));
		}
#line 2972 "cp-name-parser.c.tmp"
    break;

  case 171: /* exp: DYNAMIC_CAST '<' type '>' '(' exp1 ')'  */
#line 1013 "cp-name-parser.y"
                { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_UNARY,
				    state->fill_comp (DEMANGLE_COMPONENT_CAST, (yyvsp[-4].comp), NULL),
				    (yyvsp[-1].comp));
		}
#line 2981 "cp-name-parser.c.tmp"
    break;

  case 172: /* exp: REINTERPRET_CAST '<' type '>' '(' exp1 ')'  */
#line 1020 "cp-name-parser.y"
                { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_UNARY,
				    state->fill_comp (DEMANGLE_COMPONENT_CAST, (yyvsp[-4].comp), NULL),
				    (yyvsp[-1].comp));
		}
#line 2990 "cp-name-parser.c.tmp"
    break;

  case 173: /* exp: exp '*' exp  */
#line 1039 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("*", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 2996 "cp-name-parser.c.tmp"
    break;

  case 174: /* exp: exp '/' exp  */
#line 1043 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("/", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3002 "cp-name-parser.c.tmp"
    break;

  case 175: /* exp: exp '%' exp  */
#line 1047 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("%", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3008 "cp-name-parser.c.tmp"
    break;

  case 176: /* exp: exp '+' exp  */
#line 1051 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("+", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3014 "cp-name-parser.c.tmp"
    break;

  case 177: /* exp: exp '-' exp  */
#line 1055 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("-", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3020 "cp-name-parser.c.tmp"
    break;

  case 178: /* exp: exp LSH exp  */
#line 1059 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("<<", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3026 "cp-name-parser.c.tmp"
    break;

  case 179: /* exp: exp RSH exp  */
#line 1063 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary (">>", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3032 "cp-name-parser.c.tmp"
    break;

  case 180: /* exp: exp EQUAL exp  */
#line 1067 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("==", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3038 "cp-name-parser.c.tmp"
    break;

  case 181: /* exp: exp NOTEQUAL exp  */
#line 1071 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("!=", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3044 "cp-name-parser.c.tmp"
    break;

  case 182: /* exp: exp LEQ exp  */
#line 1075 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("<=", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3050 "cp-name-parser.c.tmp"
    break;

  case 183: /* exp: exp GEQ exp  */
#line 1079 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary (">=", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3056 "cp-name-parser.c.tmp"
    break;

  case 184: /* exp: exp SPACESHIP exp  */
#line 1083 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("<=>", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3062 "cp-name-parser.c.tmp"
    break;

  case 185: /* exp: exp '<' exp  */
#line 1087 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("<", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3068 "cp-name-parser.c.tmp"
    break;

  case 186: /* exp: exp '&' exp  */
#line 1091 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("&", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3074 "cp-name-parser.c.tmp"
    break;

  case 187: /* exp: exp '^' exp  */
#line 1095 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("^", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3080 "cp-name-parser.c.tmp"
    break;

  case 188: /* exp: exp '|' exp  */
#line 1099 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("|", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3086 "cp-name-parser.c.tmp"
    break;

  case 189: /* exp: exp ANDAND exp  */
#line 1103 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("&&", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3092 "cp-name-parser.c.tmp"
    break;

  case 190: /* exp: exp OROR exp  */
#line 1107 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("||", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3098 "cp-name-parser.c.tmp"
    break;

  case 191: /* exp: exp ARROW NAME  */
#line 1112 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("->", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3104 "cp-name-parser.c.tmp"
    break;

  case 192: /* exp: exp '.' NAME  */
#line 1116 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary (".", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3110 "cp-name-parser.c.tmp"
    break;

  case 193: /* exp: exp '?' exp ':' exp  */
#line 1120 "cp-name-parser.y"
                { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_TRINARY, state->make_operator ("?", 3),
				    state->fill_comp (DEMANGLE_COMPONENT_TRINARY_ARG1, (yyvsp[-4].comp),
						 state->fill_comp (DEMANGLE_COMPONENT_TRINARY_ARG2, (yyvsp[-2].comp), (yyvsp[0].comp))));
		}
#line 3119 "cp-name-parser.c.tmp"
    break;

  case 196: /* exp: SIZEOF '(' type ')'  */
#line 1134 "cp-name-parser.y"
                {
		  /* Match the whitespacing of cplus_demangle_operators.
		     It would abort on unrecognized string otherwise.  */
		  (yyval.comp) = state->d_unary ("sizeof ", (yyvsp[-1].comp));
		}
#line 3129 "cp-name-parser.c.tmp"
    break;

  case 197: /* exp: TRUEKEYWORD  */
#line 1143 "cp-name-parser.y"
                { struct demangle_component *i;
		  i = state->make_name ("1", 1);
		  (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_LITERAL,
				    state->make_builtin_type ( "bool"),
				    i);
		}
#line 3140 "cp-name-parser.c.tmp"
    break;

  case 198: /* exp: FALSEKEYWORD  */
#line 1152 "cp-name-parser.y"
                { struct demangle_component *i;
		  i = state->make_name ("0", 1);
		  (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_LITERAL,
				    state->make_builtin_type ("bool"),
				    i);
		}
#line 3151 "cp-name-parser.c.tmp"
    break;


#line 3155 "cp-name-parser.c.tmp"

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

#line 1162 "cp-name-parser.y"


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
  cpname_state state;

  state.prev_lexptr = state.lexptr = demangled_name;
  state.error_lexptr = NULL;
  state.global_errmsg = NULL;

  auto result = std::make_unique<demangle_parse_info> ();
  state.demangle_info = result.get ();

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
}

#endif

void _initialize_cp_name_parser ();
void
_initialize_cp_name_parser ()
{
#if GDB_SELF_TEST
  selftests::register_test ("canonicalize", canonicalize_tests);
#endif
}
