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


#include "defs.h"

#include <unistd.h>
#include "safe-ctype.h"
#include "demangle.h"
#include "cp-support.h"
#include "c-support.h"
#include "parser-defs.h"

#define GDB_YY_REMAP_PREFIX cpname
#include "yy-remap.h"

/* The components built by the parser are allocated ahead of time,
   and cached in this structure.  */

#define ALLOC_CHUNK 100

struct demangle_info {
  int used;
  struct demangle_info *next;
  struct demangle_component comps[ALLOC_CHUNK];
};


#line 98 "cp-name-parser.c.tmp"

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
    LSH = 302,                     /* LSH  */
    RSH = 303,                     /* RSH  */
    UNARY = 304,                   /* UNARY  */
    INCREMENT = 305,               /* INCREMENT  */
    DECREMENT = 306,               /* DECREMENT  */
    ARROW = 307                    /* ARROW  */
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
#define LSH 302
#define RSH 303
#define UNARY 304
#define INCREMENT 305
#define DECREMENT 306
#define ARROW 307

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 66 "cp-name-parser.y"

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
  

#line 272 "cp-name-parser.c.tmp"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif




int yyparse (struct cpname_state *state);



/* Symbol kind.  */
enum yysymbol_kind_t
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
  YYSYMBOL_LSH = 56,                       /* LSH  */
  YYSYMBOL_RSH = 57,                       /* RSH  */
  YYSYMBOL_58_ = 58,                       /* '@'  */
  YYSYMBOL_59_ = 59,                       /* '+'  */
  YYSYMBOL_60_ = 60,                       /* '-'  */
  YYSYMBOL_61_ = 61,                       /* '*'  */
  YYSYMBOL_62_ = 62,                       /* '/'  */
  YYSYMBOL_63_ = 63,                       /* '%'  */
  YYSYMBOL_UNARY = 64,                     /* UNARY  */
  YYSYMBOL_INCREMENT = 65,                 /* INCREMENT  */
  YYSYMBOL_DECREMENT = 66,                 /* DECREMENT  */
  YYSYMBOL_ARROW = 67,                     /* ARROW  */
  YYSYMBOL_68_ = 68,                       /* '.'  */
  YYSYMBOL_69_ = 69,                       /* '['  */
  YYSYMBOL_70_ = 70,                       /* ']'  */
  YYSYMBOL_71_ = 71,                       /* '~'  */
  YYSYMBOL_72_ = 72,                       /* '!'  */
  YYSYMBOL_73_ = 73,                       /* '('  */
  YYSYMBOL_74_ = 74,                       /* ':'  */
  YYSYMBOL_YYACCEPT = 75,                  /* $accept  */
  YYSYMBOL_result = 76,                    /* result  */
  YYSYMBOL_start = 77,                     /* start  */
  YYSYMBOL_start_opt = 78,                 /* start_opt  */
  YYSYMBOL_function = 79,                  /* function  */
  YYSYMBOL_demangler_special = 80,         /* demangler_special  */
  YYSYMBOL_oper = 81,                      /* oper  */
  YYSYMBOL_conversion_op = 82,             /* conversion_op  */
  YYSYMBOL_conversion_op_name = 83,        /* conversion_op_name  */
  YYSYMBOL_unqualified_name = 84,          /* unqualified_name  */
  YYSYMBOL_colon_name = 85,                /* colon_name  */
  YYSYMBOL_name = 86,                      /* name  */
  YYSYMBOL_colon_ext_name = 87,            /* colon_ext_name  */
  YYSYMBOL_colon_ext_only = 88,            /* colon_ext_only  */
  YYSYMBOL_ext_only_name = 89,             /* ext_only_name  */
  YYSYMBOL_nested_name = 90,               /* nested_name  */
  YYSYMBOL_templ = 91,                     /* templ  */
  YYSYMBOL_template_params = 92,           /* template_params  */
  YYSYMBOL_template_arg = 93,              /* template_arg  */
  YYSYMBOL_function_args = 94,             /* function_args  */
  YYSYMBOL_function_arglist = 95,          /* function_arglist  */
  YYSYMBOL_qualifiers_opt = 96,            /* qualifiers_opt  */
  YYSYMBOL_qualifier = 97,                 /* qualifier  */
  YYSYMBOL_qualifiers = 98,                /* qualifiers  */
  YYSYMBOL_int_part = 99,                  /* int_part  */
  YYSYMBOL_int_seq = 100,                  /* int_seq  */
  YYSYMBOL_builtin_type = 101,             /* builtin_type  */
  YYSYMBOL_ptr_operator = 102,             /* ptr_operator  */
  YYSYMBOL_array_indicator = 103,          /* array_indicator  */
  YYSYMBOL_typespec_2 = 104,               /* typespec_2  */
  YYSYMBOL_abstract_declarator = 105,      /* abstract_declarator  */
  YYSYMBOL_direct_abstract_declarator = 106, /* direct_abstract_declarator  */
  YYSYMBOL_abstract_declarator_fn = 107,   /* abstract_declarator_fn  */
  YYSYMBOL_type = 108,                     /* type  */
  YYSYMBOL_declarator = 109,               /* declarator  */
  YYSYMBOL_direct_declarator = 110,        /* direct_declarator  */
  YYSYMBOL_declarator_1 = 111,             /* declarator_1  */
  YYSYMBOL_direct_declarator_1 = 112,      /* direct_declarator_1  */
  YYSYMBOL_exp = 113,                      /* exp  */
  YYSYMBOL_exp1 = 114                      /* exp1  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;


/* Second part of user prologue.  */
#line 85 "cp-name-parser.y"


struct cpname_state
{
  /* LEXPTR is the current pointer into our lex buffer.  PREV_LEXPTR
     is the start of the last token lexed, only used for diagnostics.
     ERROR_LEXPTR is the first place an error occurred.  GLOBAL_ERRMSG
     is the first error message encountered.  */

  const char *lexptr, *prev_lexptr, *error_lexptr, *global_errmsg;

  struct demangle_info *demangle_info;

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

  int parse_number (const char *p, int len, int parsed_float, YYSTYPE *lvalp);
};

struct demangle_component *
cpname_state::d_grab ()
{
  struct demangle_info *more;

  if (demangle_info->used >= ALLOC_CHUNK)
    {
      if (demangle_info->next == NULL)
	{
	  more = XNEW (struct demangle_info);
	  more->next = NULL;
	  demangle_info->next = more;
	}
      else
	more = demangle_info->next;

      more->used = 0;
      demangle_info = more;
    }
  return &demangle_info->comps[demangle_info->used++];
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

/* Enable yydebug for the stand-alone parser.  */
#ifdef TEST_CPNAMES
# define YYDEBUG	1
#endif

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

static int yylex (YYSTYPE *, cpname_state *);
static void yyerror (cpname_state *, const char *);

#line 579 "cp-name-parser.c.tmp"


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
#define YYFINAL  84
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1137

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  75
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  40
/* YYNRULES -- Number of rules.  */
#define YYNRULES  195
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  325

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   307


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
       2,     2,     2,    72,     2,     2,     2,    63,    49,     2,
      73,    41,    61,    59,    42,    60,    68,    62,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    74,     2,
      52,    43,    53,    44,    58,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    69,     2,    70,    48,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    47,     2,    71,     2,     2,     2,
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
      54,    55,    56,    57,    64,    65,    66,    67
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   348,   348,   357,   359,   361,   366,   367,   374,   383,
     390,   394,   397,   416,   418,   422,   428,   434,   440,   446,
     448,   450,   452,   454,   456,   458,   460,   462,   464,   466,
     468,   470,   472,   474,   476,   478,   480,   482,   484,   486,
     488,   490,   492,   494,   496,   498,   500,   502,   510,   515,
     520,   524,   529,   537,   538,   540,   552,   553,   559,   561,
     562,   564,   567,   568,   571,   572,   576,   578,   581,   585,
     590,   594,   603,   607,   610,   621,   622,   626,   628,   630,
     633,   637,   642,   647,   653,   663,   667,   671,   679,   680,
     683,   685,   687,   691,   692,   699,   701,   703,   705,   707,
     709,   713,   714,   718,   720,   722,   724,   726,   728,   730,
     734,   739,   742,   745,   751,   759,   761,   775,   777,   778,
     780,   783,   785,   786,   788,   791,   793,   795,   797,   802,
     805,   810,   817,   821,   832,   838,   856,   859,   867,   869,
     880,   887,   888,   894,   898,   902,   904,   909,   914,   926,
     930,   934,   942,   947,   956,   960,   965,   970,   974,   980,
     986,   989,   996,   998,  1003,  1007,  1011,  1018,  1034,  1041,
    1048,  1067,  1071,  1075,  1079,  1083,  1087,  1091,  1095,  1099,
    1103,  1107,  1111,  1115,  1119,  1123,  1127,  1131,  1136,  1140,
    1144,  1151,  1155,  1158,  1167,  1176
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
  "\"end of file\"", "error", "\"invalid token\"", "INT", "FLOAT", "NAME",
  "STRUCT", "CLASS", "UNION", "ENUM", "SIZEOF", "UNSIGNED", "COLONCOLON",
  "TEMPLATE", "ERROR", "NEW", "DELETE", "OPERATOR", "STATIC_CAST",
  "REINTERPRET_CAST", "DYNAMIC_CAST", "SIGNED_KEYWORD", "LONG", "SHORT",
  "INT_KEYWORD", "CONST_KEYWORD", "VOLATILE_KEYWORD", "DOUBLE_KEYWORD",
  "BOOL", "ELLIPSIS", "RESTRICT", "VOID", "FLOAT_KEYWORD", "CHAR",
  "WCHAR_T", "ASSIGN_MODIFY", "TRUEKEYWORD", "FALSEKEYWORD",
  "DEMANGLER_SPECIAL", "CONSTRUCTION_VTABLE", "CONSTRUCTION_IN", "')'",
  "','", "'='", "'?'", "OROR", "ANDAND", "'|'", "'^'", "'&'", "EQUAL",
  "NOTEQUAL", "'<'", "'>'", "LEQ", "GEQ", "LSH", "RSH", "'@'", "'+'",
  "'-'", "'*'", "'/'", "'%'", "UNARY", "INCREMENT", "DECREMENT", "ARROW",
  "'.'", "'['", "']'", "'~'", "'!'", "'('", "':'", "$accept", "result",
  "start", "start_opt", "function", "demangler_special", "oper",
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
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-187)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     833,    39,  -187,    42,   540,  -187,   -12,  -187,  -187,  -187,
    -187,  -187,  -187,  -187,  -187,  -187,  -187,  -187,   833,   833,
      27,    41,  -187,  -187,  -187,     5,  -187,   710,  -187,    36,
      -5,  -187,    43,    65,    36,   506,  -187,   120,    36,   711,
    -187,  -187,   329,  -187,    36,  -187,    43,    73,    16,    21,
    -187,  -187,  -187,  -187,  -187,  -187,  -187,  -187,  -187,  -187,
    -187,  -187,  -187,  -187,  -187,  -187,  -187,  -187,  -187,  -187,
    -187,  -187,  -187,    34,    30,  -187,  -187,    64,   115,  -187,
    -187,  -187,    83,  -187,  -187,   329,    39,   833,  -187,  -187,
      36,     6,   603,  -187,    12,    65,    98,   750,  -187,   -49,
    -187,  -187,   857,    98,    70,  -187,  -187,   124,  -187,  -187,
      73,    36,    36,  -187,  -187,  -187,    48,   860,   603,  -187,
    -187,   -49,  -187,    23,    98,   720,  -187,   -49,  -187,   -49,
    -187,  -187,    53,    80,    87,    95,  -187,  -187,   656,   476,
     476,   476,   428,    10,  -187,   780,   325,  -187,  -187,    79,
      82,  -187,  -187,  -187,   833,    22,  -187,    28,  -187,  -187,
      89,  -187,    73,   116,    36,   780,    37,   114,   780,   780,
     123,    70,    36,   124,   833,  -187,   161,  -187,   166,  -187,
    -187,  -187,  -187,    36,  -187,  -187,  -187,    50,   729,   168,
    -187,  -187,   780,  -187,  -187,  -187,   169,  -187,   979,   979,
     979,   979,   833,  -187,   105,   105,   105,   680,   780,   142,
     970,   143,   329,  -187,  -187,   476,   476,   476,   476,   476,
     476,   476,   476,   476,   476,   476,   476,   476,   476,   476,
     476,   476,   476,   180,   181,  -187,  -187,  -187,  -187,    36,
    -187,    45,    36,  -187,    36,   949,  -187,  -187,  -187,    51,
     833,  -187,   729,  -187,   729,   148,   -49,   833,   833,   152,
     138,   141,   144,   154,   833,  -187,   476,   476,  -187,  -187,
     890,   993,  1015,  1036,  1056,   359,   753,   753,  1069,  1069,
    1069,   241,   241,   178,   178,   105,   105,   105,  -187,  -187,
    -187,  -187,  -187,  -187,   780,  -187,   157,  -187,  -187,  -187,
    -187,  -187,  -187,  -187,   126,   128,   131,  -187,   164,   105,
     325,   476,  -187,  -187,   471,   471,   471,  -187,   325,   188,
     189,   192,  -187,  -187,  -187
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    59,    97,     0,     0,    96,    99,   100,    95,    92,
      91,   105,   107,    90,   109,   104,    98,   108,     0,     0,
       0,     0,     2,     5,     4,    53,    50,     6,    67,   122,
       0,    64,     0,    61,    93,     0,   101,   103,   118,   141,
       3,    68,     0,    52,   126,    65,     0,     0,    15,    16,
      32,    43,    29,    40,    39,    26,    24,    25,    35,    36,
      30,    31,    37,    38,    33,    34,    19,    20,    21,    22,
      23,    41,    42,    45,     0,    27,    28,     0,     0,    48,
     106,    13,     0,    55,     1,     0,     0,     0,   112,   111,
      88,     0,     0,    11,     0,     0,     6,   136,   135,   138,
      12,   121,     0,     6,    58,    49,    66,    60,    70,    94,
       0,   124,   120,    99,   102,   117,     0,     0,     0,    62,
      56,   150,    63,     0,     6,   129,   142,   131,     8,   151,
     191,   192,     0,     0,     0,     0,   194,   195,     0,     0,
       0,     0,     0,     0,    73,    75,    79,   125,    51,     0,
       0,    44,    47,    46,     0,     0,     7,     0,   110,    89,
       0,   115,     0,   109,    88,     0,     0,     0,   129,    80,
       0,     0,    88,     0,     0,   140,     0,   137,   133,   134,
      10,    69,    71,   128,   123,   119,    57,     0,   129,   157,
     158,     9,     0,   130,   149,   133,   155,   156,     0,     0,
       0,     0,     0,    77,   164,   166,   165,     0,   141,     0,
     160,     0,     0,    72,    76,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    17,    18,    14,    54,    88,
     116,     0,    88,    87,    88,     0,    81,   132,   113,     0,
       0,   127,     0,   148,   129,     0,   144,     0,     0,     0,
       0,     0,     0,     0,     0,   162,     0,     0,   159,    74,
       0,   187,   186,   185,   184,   183,   178,   179,   182,   180,
     181,   176,   177,   174,   175,   171,   172,   173,   188,   189,
     114,    86,    85,    84,    82,   139,     0,   143,   154,   146,
     147,   152,   153,   193,     0,     0,     0,    78,     0,   167,
     161,     0,    83,   145,     0,     0,     0,   163,   190,     0,
       0,     0,   168,   170,   169
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -187,  -187,    25,   -66,  -187,  -187,  -187,     3,  -187,   -20,
    -187,    -1,   -32,    15,     1,     0,   150,   149,    31,  -187,
     -25,  -156,  -187,   234,   205,  -187,   213,   -17,   -98,   196,
     -18,   -16,   158,  -129,  -106,  -187,   134,  -187,    -6,  -186
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    21,   156,    93,    23,    24,    25,    26,    27,    28,
     119,    29,   253,    30,    31,    78,    33,   143,   144,   167,
      96,   158,    34,    35,    36,    37,    38,   168,    98,    39,
     170,   127,   100,    40,   255,   256,   128,   129,   210,   211
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      32,   179,    44,    46,    45,   103,    43,   121,   243,   160,
      97,    99,   106,   209,   124,    80,   248,   171,    32,    32,
      91,   126,   125,   190,   102,    22,   106,    94,   104,   179,
     175,   197,    83,   104,   111,   105,   146,   180,   120,   123,
     117,    84,   104,    81,    82,     4,    44,     1,   104,   148,
     104,    41,   212,     1,   122,   104,   171,    85,   191,     4,
       4,     9,    10,   213,   212,   117,    13,   117,   102,   259,
     260,   261,   262,   172,   178,   238,   161,   108,     1,   146,
      97,    99,   181,   290,   172,   149,   291,   157,   292,   239,
     150,    42,   166,   121,    20,   151,   189,    94,   172,    20,
     152,   188,   195,   106,   196,   153,   239,   193,   125,   183,
     174,   239,   239,    20,    20,   186,   187,    45,   166,    20,
     104,    20,    42,   154,   120,   123,   198,   214,   319,   320,
     321,     2,   199,   204,   205,   206,   182,   106,    32,   200,
     122,     5,   113,     7,     8,    94,   296,   201,   297,   235,
     193,   246,   236,    16,    32,   244,   245,   242,   300,   240,
     105,    44,   241,   203,   247,    94,    86,   106,    94,    94,
     193,   254,   233,   234,    32,   188,   249,    95,   250,   237,
     257,   258,   107,   266,   268,   288,   289,   120,   123,   298,
     126,   304,    94,   303,   305,   307,   107,   306,   313,   314,
      79,   315,    32,   122,   316,   317,   146,    32,    94,   270,
     271,   272,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   282,   283,   284,   285,   286,   287,   263,   107,   322,
     323,   299,   265,   324,   155,   254,   193,   254,   145,   230,
     231,   232,   114,   269,   173,   233,   234,    95,   112,     0,
      32,   120,   123,   120,   123,   177,     0,    32,    32,   194,
     309,   310,     0,   101,    32,     0,     0,   122,   109,   122,
       0,     0,   115,   107,     0,   295,   312,     0,   147,     0,
       0,   145,   301,   302,     0,     0,     0,     0,   169,   308,
       0,     0,     0,     0,    94,    95,     0,     0,   169,     0,
     228,   229,   230,   231,   232,   318,     0,   107,   233,   234,
       0,     0,     0,     0,   169,    95,   107,     0,    95,    95,
       0,     0,     0,     0,   159,     0,    95,     0,     0,     0,
       0,     0,   130,   131,     1,     0,     0,   107,   208,   132,
       2,    47,    95,     0,     0,   184,   185,   133,   134,   135,
       5,     6,     7,     8,     9,    10,    11,    12,    95,    13,
      14,    15,    16,    17,     0,   136,   137,     0,     0,   215,
     216,   217,   218,   219,   220,   221,   222,   223,   138,   224,
     225,   226,   227,     0,   228,   229,   230,   231,   232,   139,
       0,   107,   233,   234,   208,   208,   208,   208,   159,   173,
     140,   141,   142,     0,     0,     0,   159,     0,   145,   221,
     222,   223,     0,   224,   225,   226,   227,   251,   228,   229,
     230,   231,   232,     0,     0,     0,   233,   234,     0,     0,
       0,   130,   131,     1,     0,     0,     0,     0,   132,     2,
      47,   294,     0,     0,    95,     0,   133,   134,   135,     5,
       6,     7,     8,     9,    10,    11,    12,     0,    13,    14,
      15,    16,    17,     0,   136,   137,     0,     0,     0,     0,
       0,     0,     0,   159,   130,   131,   159,   207,   159,   130,
     131,   132,     0,     0,     0,     0,   132,     0,   139,   133,
     134,   135,     0,     0,   133,   134,   135,     0,     0,   140,
     141,   142,     0,     0,     0,     0,     0,   136,   137,     0,
       0,     1,   136,   137,     0,     0,     0,     2,   110,     0,
     207,     0,     0,     0,     0,     0,     0,     5,     6,     7,
       8,   139,     0,    11,    12,     0,   139,    14,    15,    16,
      17,     0,   140,   141,   142,     1,     0,   140,   141,   142,
       0,     2,    47,     0,     0,    48,    49,     0,     0,     0,
       0,     5,     6,     7,     8,     9,    10,    11,    12,     0,
      13,    14,    15,    16,    17,    50,     0,     0,     0,     0,
       0,     0,    51,    52,     0,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,     0,    66,
      67,    68,    69,    70,     0,    71,    72,    73,     1,    74,
       0,    75,    76,    77,     2,   162,     0,     0,     0,     0,
       0,     0,     0,     0,     5,     6,     7,     8,     9,    10,
      11,    12,     0,    13,   163,    15,    16,    17,     0,     0,
       0,     0,     0,     0,   164,     0,     0,     0,     0,    88,
       0,     0,    89,     0,     0,     0,     0,     0,     0,     0,
       0,     1,     0,     0,    90,     0,     0,     2,     3,     0,
       0,     0,    91,     4,     0,     0,   165,     5,     6,     7,
       8,     9,    10,    11,    12,     1,    13,    14,    15,    16,
      17,     2,     3,     0,    18,    19,     0,     4,     0,     0,
       0,     5,     6,     7,     8,     9,    10,    11,    12,     0,
      13,    14,    15,    16,    17,    86,     1,     0,    18,    19,
       0,     0,    87,   116,     0,     1,     0,    20,   117,   202,
       0,     0,   116,     0,     1,     0,     0,   117,     0,     0,
       0,   116,     0,     0,     0,     0,   117,     0,     0,     0,
       0,    20,     0,   264,     0,    86,    88,    88,     0,    89,
      89,     0,   176,     0,     0,     0,    88,     0,     0,    89,
       0,    90,    90,     0,     0,    88,     0,     0,    89,    91,
      91,    90,    20,    92,   118,    86,     0,     0,     0,    91,
      90,    20,   176,   192,     0,     0,    88,     0,    91,    89,
      20,     0,   252,     0,     0,   223,     0,   224,   225,   226,
     227,    90,   228,   229,   230,   231,   232,     0,     0,    91,
     233,   234,     0,    92,     0,     0,    88,     0,     0,    89,
       0,     0,     0,     0,     0,     0,     0,     0,     1,     0,
       0,    90,     0,     0,     2,     3,     0,     0,     0,    91,
       4,     0,     0,   165,     5,     6,     7,     8,     9,    10,
      11,    12,     1,    13,    14,    15,    16,    17,     2,    47,
       0,    18,    19,     0,     0,    48,    49,     0,     5,     6,
       7,     8,     9,    10,    11,    12,     0,    13,   163,    15,
      16,    17,     0,     0,     0,    50,     0,     0,   164,     0,
       0,     0,    51,    52,    20,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,     0,    66,
      67,    68,    69,    70,     0,    71,    72,    73,     0,    74,
       0,    75,    76,    77,   215,   216,   217,   218,   219,   220,
     221,   222,   223,     0,   224,   225,   226,   227,     0,   228,
     229,   230,   231,   232,     1,     0,     0,   233,   234,     0,
       2,    47,     0,     0,   311,     0,     0,     0,     0,     0,
       5,     6,     7,     8,     9,    10,    11,    12,   293,    13,
      14,    15,    16,    17,     1,     0,     0,     0,     0,     0,
       2,    47,     0,     0,     0,     0,     0,     0,     0,     0,
       5,     6,     7,     8,     9,    10,    11,    12,     0,    13,
      14,    15,    16,    17,   215,   216,   217,   218,   219,   220,
     221,   222,   223,   267,   224,   225,   226,   227,     0,   228,
     229,   230,   231,   232,     0,     0,     0,   233,   234,   217,
     218,   219,   220,   221,   222,   223,     0,   224,   225,   226,
     227,     0,   228,   229,   230,   231,   232,     0,     0,     0,
     233,   234,   218,   219,   220,   221,   222,   223,     0,   224,
     225,   226,   227,     0,   228,   229,   230,   231,   232,     0,
       0,     0,   233,   234,   219,   220,   221,   222,   223,     0,
     224,   225,   226,   227,     0,   228,   229,   230,   231,   232,
       0,     0,     0,   233,   234,   220,   221,   222,   223,     0,
     224,   225,   226,   227,     0,   228,   229,   230,   231,   232,
       0,     0,     0,   233,   234,   226,   227,     0,   228,   229,
     230,   231,   232,     0,     0,     0,   233,   234
};

static const yytype_int16 yycheck[] =
{
       0,    99,     3,     3,     3,    30,     3,    39,   164,     3,
      27,    27,    32,   142,    39,    27,   172,     5,    18,    19,
      69,    39,    39,   121,    73,     0,    46,    27,     5,   127,
      96,   129,     5,     5,    35,    32,    42,   103,    39,    39,
      17,     0,     5,    18,    19,    17,    47,     5,     5,    46,
       5,    12,    42,     5,    39,     5,     5,    52,   124,    17,
      17,    25,    26,    53,    42,    17,    30,    17,    73,   198,
     199,   200,   201,    61,    99,    53,    70,    12,     5,    85,
      97,    97,    12,   239,    61,    69,   242,    87,   244,    61,
      69,    52,    92,   125,    71,    61,   121,    97,    61,    71,
      70,   118,   127,   123,   129,    41,    61,   125,   125,   110,
      12,    61,    61,    71,    71,   116,   116,   116,   118,    71,
       5,    71,    52,    40,   125,   125,    73,   145,   314,   315,
     316,    11,    52,   139,   140,   141,    12,   157,   138,    52,
     125,    21,    22,    23,    24,   145,   252,    52,   254,    70,
     168,   169,    70,    33,   154,    41,    42,    41,   256,    70,
     157,   162,   162,   138,    41,   165,     5,   187,   168,   169,
     188,   188,    67,    68,   174,   192,   176,    27,    12,   154,
      12,    12,    32,    41,    41,     5,     5,   188,   188,    41,
     208,    53,   192,    41,    53,    41,    46,    53,    41,    73,
       4,    73,   202,   188,    73,    41,   212,   207,   208,   215,
     216,   217,   218,   219,   220,   221,   222,   223,   224,   225,
     226,   227,   228,   229,   230,   231,   232,   202,    78,    41,
      41,   256,   207,    41,    85,   252,   254,   254,    42,    61,
      62,    63,    37,   212,    94,    67,    68,    97,    35,    -1,
     250,   252,   252,   254,   254,    97,    -1,   257,   258,   125,
     266,   267,    -1,    29,   264,    -1,    -1,   252,    34,   254,
      -1,    -1,    38,   123,    -1,   250,   294,    -1,    44,    -1,
      -1,    85,   257,   258,    -1,    -1,    -1,    -1,    92,   264,
      -1,    -1,    -1,    -1,   294,   145,    -1,    -1,   102,    -1,
      59,    60,    61,    62,    63,   311,    -1,   157,    67,    68,
      -1,    -1,    -1,    -1,   118,   165,   166,    -1,   168,   169,
      -1,    -1,    -1,    -1,    90,    -1,   176,    -1,    -1,    -1,
      -1,    -1,     3,     4,     5,    -1,    -1,   187,   142,    10,
      11,    12,   192,    -1,    -1,   111,   112,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,   208,    30,
      31,    32,    33,    34,    -1,    36,    37,    -1,    -1,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    49,    54,
      55,    56,    57,    -1,    59,    60,    61,    62,    63,    60,
      -1,   241,    67,    68,   198,   199,   200,   201,   164,   249,
      71,    72,    73,    -1,    -1,    -1,   172,    -1,   212,    50,
      51,    52,    -1,    54,    55,    56,    57,   183,    59,    60,
      61,    62,    63,    -1,    -1,    -1,    67,    68,    -1,    -1,
      -1,     3,     4,     5,    -1,    -1,    -1,    -1,    10,    11,
      12,   245,    -1,    -1,   294,    -1,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    -1,    30,    31,
      32,    33,    34,    -1,    36,    37,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   239,     3,     4,   242,    49,   244,     3,
       4,    10,    -1,    -1,    -1,    -1,    10,    -1,    60,    18,
      19,    20,    -1,    -1,    18,    19,    20,    -1,    -1,    71,
      72,    73,    -1,    -1,    -1,    -1,    -1,    36,    37,    -1,
      -1,     5,    36,    37,    -1,    -1,    -1,    11,    12,    -1,
      49,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    23,
      24,    60,    -1,    27,    28,    -1,    60,    31,    32,    33,
      34,    -1,    71,    72,    73,     5,    -1,    71,    72,    73,
      -1,    11,    12,    -1,    -1,    15,    16,    -1,    -1,    -1,
      -1,    21,    22,    23,    24,    25,    26,    27,    28,    -1,
      30,    31,    32,    33,    34,    35,    -1,    -1,    -1,    -1,
      -1,    -1,    42,    43,    -1,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    -1,    59,
      60,    61,    62,    63,    -1,    65,    66,    67,     5,    69,
      -1,    71,    72,    73,    11,    12,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    21,    22,    23,    24,    25,    26,
      27,    28,    -1,    30,    31,    32,    33,    34,    -1,    -1,
      -1,    -1,    -1,    -1,    41,    -1,    -1,    -1,    -1,    46,
      -1,    -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     5,    -1,    -1,    61,    -1,    -1,    11,    12,    -1,
      -1,    -1,    69,    17,    -1,    -1,    73,    21,    22,    23,
      24,    25,    26,    27,    28,     5,    30,    31,    32,    33,
      34,    11,    12,    -1,    38,    39,    -1,    17,    -1,    -1,
      -1,    21,    22,    23,    24,    25,    26,    27,    28,    -1,
      30,    31,    32,    33,    34,     5,     5,    -1,    38,    39,
      -1,    -1,    12,    12,    -1,     5,    -1,    71,    17,    73,
      -1,    -1,    12,    -1,     5,    -1,    -1,    17,    -1,    -1,
      -1,    12,    -1,    -1,    -1,    -1,    17,    -1,    -1,    -1,
      -1,    71,    -1,    73,    -1,     5,    46,    46,    -1,    49,
      49,    -1,    12,    -1,    -1,    -1,    46,    -1,    -1,    49,
      -1,    61,    61,    -1,    -1,    46,    -1,    -1,    49,    69,
      69,    61,    71,    73,    73,     5,    -1,    -1,    -1,    69,
      61,    71,    12,    73,    -1,    -1,    46,    -1,    69,    49,
      71,    -1,    73,    -1,    -1,    52,    -1,    54,    55,    56,
      57,    61,    59,    60,    61,    62,    63,    -1,    -1,    69,
      67,    68,    -1,    73,    -1,    -1,    46,    -1,    -1,    49,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     5,    -1,
      -1,    61,    -1,    -1,    11,    12,    -1,    -1,    -1,    69,
      17,    -1,    -1,    73,    21,    22,    23,    24,    25,    26,
      27,    28,     5,    30,    31,    32,    33,    34,    11,    12,
      -1,    38,    39,    -1,    -1,    15,    16,    -1,    21,    22,
      23,    24,    25,    26,    27,    28,    -1,    30,    31,    32,
      33,    34,    -1,    -1,    -1,    35,    -1,    -1,    41,    -1,
      -1,    -1,    42,    43,    71,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    -1,    59,
      60,    61,    62,    63,    -1,    65,    66,    67,    -1,    69,
      -1,    71,    72,    73,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    -1,    54,    55,    56,    57,    -1,    59,
      60,    61,    62,    63,     5,    -1,    -1,    67,    68,    -1,
      11,    12,    -1,    -1,    74,    -1,    -1,    -1,    -1,    -1,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,     5,    -1,    -1,    -1,    -1,    -1,
      11,    12,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      21,    22,    23,    24,    25,    26,    27,    28,    -1,    30,
      31,    32,    33,    34,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    -1,    59,
      60,    61,    62,    63,    -1,    -1,    -1,    67,    68,    46,
      47,    48,    49,    50,    51,    52,    -1,    54,    55,    56,
      57,    -1,    59,    60,    61,    62,    63,    -1,    -1,    -1,
      67,    68,    47,    48,    49,    50,    51,    52,    -1,    54,
      55,    56,    57,    -1,    59,    60,    61,    62,    63,    -1,
      -1,    -1,    67,    68,    48,    49,    50,    51,    52,    -1,
      54,    55,    56,    57,    -1,    59,    60,    61,    62,    63,
      -1,    -1,    -1,    67,    68,    49,    50,    51,    52,    -1,
      54,    55,    56,    57,    -1,    59,    60,    61,    62,    63,
      -1,    -1,    -1,    67,    68,    56,    57,    -1,    59,    60,
      61,    62,    63,    -1,    -1,    -1,    67,    68
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     5,    11,    12,    17,    21,    22,    23,    24,    25,
      26,    27,    28,    30,    31,    32,    33,    34,    38,    39,
      71,    76,    77,    79,    80,    81,    82,    83,    84,    86,
      88,    89,    90,    91,    97,    98,    99,   100,   101,   104,
     108,    12,    52,    82,    86,    89,    90,    12,    15,    16,
      35,    42,    43,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    59,    60,    61,    62,
      63,    65,    66,    67,    69,    71,    72,    73,    90,   104,
      27,    77,    77,     5,     0,    52,     5,    12,    46,    49,
      61,    69,    73,    78,    90,    91,    95,   102,   103,   106,
     107,    98,    73,    95,     5,    82,    84,    91,    12,    98,
      12,    86,   101,    22,    99,    98,    12,    17,    73,    85,
      86,    87,    88,    90,    95,   102,   105,   106,   111,   112,
       3,     4,    10,    18,    19,    20,    36,    37,    49,    60,
      71,    72,    73,    92,    93,   104,   113,    98,    82,    69,
      69,    61,    70,    41,    40,    92,    77,    90,    96,    98,
       3,    70,    12,    31,    41,    73,    90,    94,   102,   104,
     105,     5,    61,    91,    12,    78,    12,   107,    95,   103,
      78,    12,    12,    86,    98,    98,    86,    90,   102,    95,
     103,    78,    73,   105,   111,    95,    95,   103,    73,    52,
      52,    52,    73,    77,   113,   113,   113,    49,   104,   108,
     113,   114,    42,    53,   105,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    54,    55,    56,    57,    59,    60,
      61,    62,    63,    67,    68,    70,    70,    77,    53,    61,
      70,    90,    41,    96,    41,    42,   105,    41,    96,    90,
      12,    98,    73,    87,   102,   109,   110,    12,    12,   108,
     108,   108,   108,    77,    73,    77,    41,    53,    41,    93,
     113,   113,   113,   113,   113,   113,   113,   113,   113,   113,
     113,   113,   113,   113,   113,   113,   113,   113,     5,     5,
      96,    96,    96,    29,   104,    77,   109,   109,    41,    95,
     103,    77,    77,    41,    53,    53,    53,    41,    77,   113,
     113,    74,   105,    41,    73,    73,    73,    41,   113,   114,
     114,   114,    41,    41,    41
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    75,    76,    77,    77,    77,    78,    78,    79,    79,
      79,    79,    79,    80,    80,    81,    81,    81,    81,    81,
      81,    81,    81,    81,    81,    81,    81,    81,    81,    81,
      81,    81,    81,    81,    81,    81,    81,    81,    81,    81,
      81,    81,    81,    81,    81,    81,    81,    81,    82,    83,
      83,    83,    83,    84,    84,    84,    85,    85,    86,    86,
      86,    86,    87,    87,    88,    88,    89,    89,    90,    90,
      90,    90,    91,    92,    92,    93,    93,    93,    93,    93,
      94,    94,    94,    94,    94,    95,    95,    95,    96,    96,
      97,    97,    97,    98,    98,    99,    99,    99,    99,    99,
      99,   100,   100,   101,   101,   101,   101,   101,   101,   101,
     102,   102,   102,   102,   102,   103,   103,   104,   104,   104,
     104,   104,   104,   104,   104,   104,   104,   104,   104,   105,
     105,   105,   106,   106,   106,   106,   107,   107,   107,   107,
     107,   108,   108,   109,   109,   110,   110,   110,   110,   111,
     111,   111,   111,   111,   112,   112,   112,   112,   112,   113,
     114,   114,   114,   114,   113,   113,   113,   113,   113,   113,
     113,   113,   113,   113,   113,   113,   113,   113,   113,   113,
     113,   113,   113,   113,   113,   113,   113,   113,   113,   113,
     113,   113,   113,   113,   113,   113
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     0,     2,     2,     3,
       3,     2,     2,     2,     4,     2,     2,     4,     4,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     3,     2,     3,     3,     2,     2,
       1,     3,     2,     1,     4,     2,     1,     2,     2,     1,
       2,     1,     1,     1,     1,     2,     2,     1,     2,     3,
       2,     3,     4,     1,     3,     1,     2,     2,     4,     1,
       1,     2,     3,     4,     3,     4,     4,     3,     0,     1,
       1,     1,     1,     1,     2,     1,     1,     1,     1,     1,
       1,     1,     2,     1,     1,     1,     2,     1,     1,     1,
       2,     1,     1,     3,     4,     2,     3,     2,     1,     3,
       2,     2,     1,     3,     2,     3,     2,     4,     3,     1,
       2,     1,     3,     2,     2,     1,     1,     2,     1,     4,
       2,     1,     2,     2,     1,     3,     2,     2,     1,     2,
       1,     1,     4,     4,     4,     2,     2,     2,     2,     3,
       1,     3,     2,     4,     2,     2,     2,     4,     7,     7,
       7,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       5,     1,     1,     4,     1,     1
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
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, struct cpname_state *state)
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
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, struct cpname_state *state)
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
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
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
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, struct cpname_state *state)
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
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

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
#line 349 "cp-name-parser.y"
                        {
			  state->global_result = (yyvsp[0].comp);

			  /* Avoid warning about "yynerrs" being unused.  */
			  (void) yynerrs;
			}
#line 1943 "cp-name-parser.c.tmp"
    break;

  case 6: /* start_opt: %empty  */
#line 366 "cp-name-parser.y"
                        { (yyval.comp) = NULL; }
#line 1949 "cp-name-parser.c.tmp"
    break;

  case 7: /* start_opt: COLONCOLON start  */
#line 368 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[0].comp); }
#line 1955 "cp-name-parser.c.tmp"
    break;

  case 8: /* function: typespec_2 declarator_1  */
#line 375 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[0].nested).comp;
			  *(yyvsp[0].nested).last = (yyvsp[-1].comp);
			}
#line 1963 "cp-name-parser.c.tmp"
    break;

  case 9: /* function: typespec_2 function_arglist start_opt  */
#line 384 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME,
					  (yyvsp[-2].comp), (yyvsp[-1].nested).comp);
			  if ((yyvsp[0].comp))
			    (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME,
						   (yyval.comp), (yyvsp[0].comp));
			}
#line 1974 "cp-name-parser.c.tmp"
    break;

  case 10: /* function: colon_ext_only function_arglist start_opt  */
#line 391 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, (yyvsp[-2].comp), (yyvsp[-1].nested).comp);
			  if ((yyvsp[0].comp)) (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME, (yyval.comp), (yyvsp[0].comp)); }
#line 1981 "cp-name-parser.c.tmp"
    break;

  case 11: /* function: conversion_op_name start_opt  */
#line 395 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[-1].nested).comp;
			  if ((yyvsp[0].comp)) (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME, (yyval.comp), (yyvsp[0].comp)); }
#line 1988 "cp-name-parser.c.tmp"
    break;

  case 12: /* function: conversion_op_name abstract_declarator_fn  */
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
#line 2008 "cp-name-parser.c.tmp"
    break;

  case 13: /* demangler_special: DEMANGLER_SPECIAL start  */
#line 417 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp ((enum demangle_component_type) (yyvsp[-1].lval), (yyvsp[0].comp), NULL); }
#line 2014 "cp-name-parser.c.tmp"
    break;

  case 14: /* demangler_special: CONSTRUCTION_VTABLE start CONSTRUCTION_IN start  */
#line 419 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_CONSTRUCTION_VTABLE, (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 2020 "cp-name-parser.c.tmp"
    break;

  case 15: /* oper: OPERATOR NEW  */
#line 423 "cp-name-parser.y"
                        {
			  /* Match the whitespacing of cplus_demangle_operators.
			     It would abort on unrecognized string otherwise.  */
			  (yyval.comp) = state->make_operator ("new", 3);
			}
#line 2030 "cp-name-parser.c.tmp"
    break;

  case 16: /* oper: OPERATOR DELETE  */
#line 429 "cp-name-parser.y"
                        {
			  /* Match the whitespacing of cplus_demangle_operators.
			     It would abort on unrecognized string otherwise.  */
			  (yyval.comp) = state->make_operator ("delete ", 1);
			}
#line 2040 "cp-name-parser.c.tmp"
    break;

  case 17: /* oper: OPERATOR NEW '[' ']'  */
#line 435 "cp-name-parser.y"
                        {
			  /* Match the whitespacing of cplus_demangle_operators.
			     It would abort on unrecognized string otherwise.  */
			  (yyval.comp) = state->make_operator ("new[]", 3);
			}
#line 2050 "cp-name-parser.c.tmp"
    break;

  case 18: /* oper: OPERATOR DELETE '[' ']'  */
#line 441 "cp-name-parser.y"
                        {
			  /* Match the whitespacing of cplus_demangle_operators.
			     It would abort on unrecognized string otherwise.  */
			  (yyval.comp) = state->make_operator ("delete[] ", 1);
			}
#line 2060 "cp-name-parser.c.tmp"
    break;

  case 19: /* oper: OPERATOR '+'  */
#line 447 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("+", 2); }
#line 2066 "cp-name-parser.c.tmp"
    break;

  case 20: /* oper: OPERATOR '-'  */
#line 449 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("-", 2); }
#line 2072 "cp-name-parser.c.tmp"
    break;

  case 21: /* oper: OPERATOR '*'  */
#line 451 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("*", 2); }
#line 2078 "cp-name-parser.c.tmp"
    break;

  case 22: /* oper: OPERATOR '/'  */
#line 453 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("/", 2); }
#line 2084 "cp-name-parser.c.tmp"
    break;

  case 23: /* oper: OPERATOR '%'  */
#line 455 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("%", 2); }
#line 2090 "cp-name-parser.c.tmp"
    break;

  case 24: /* oper: OPERATOR '^'  */
#line 457 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("^", 2); }
#line 2096 "cp-name-parser.c.tmp"
    break;

  case 25: /* oper: OPERATOR '&'  */
#line 459 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("&", 2); }
#line 2102 "cp-name-parser.c.tmp"
    break;

  case 26: /* oper: OPERATOR '|'  */
#line 461 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("|", 2); }
#line 2108 "cp-name-parser.c.tmp"
    break;

  case 27: /* oper: OPERATOR '~'  */
#line 463 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("~", 1); }
#line 2114 "cp-name-parser.c.tmp"
    break;

  case 28: /* oper: OPERATOR '!'  */
#line 465 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("!", 1); }
#line 2120 "cp-name-parser.c.tmp"
    break;

  case 29: /* oper: OPERATOR '='  */
#line 467 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("=", 2); }
#line 2126 "cp-name-parser.c.tmp"
    break;

  case 30: /* oper: OPERATOR '<'  */
#line 469 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("<", 2); }
#line 2132 "cp-name-parser.c.tmp"
    break;

  case 31: /* oper: OPERATOR '>'  */
#line 471 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator (">", 2); }
#line 2138 "cp-name-parser.c.tmp"
    break;

  case 32: /* oper: OPERATOR ASSIGN_MODIFY  */
#line 473 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ((yyvsp[0].opname), 2); }
#line 2144 "cp-name-parser.c.tmp"
    break;

  case 33: /* oper: OPERATOR LSH  */
#line 475 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("<<", 2); }
#line 2150 "cp-name-parser.c.tmp"
    break;

  case 34: /* oper: OPERATOR RSH  */
#line 477 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator (">>", 2); }
#line 2156 "cp-name-parser.c.tmp"
    break;

  case 35: /* oper: OPERATOR EQUAL  */
#line 479 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("==", 2); }
#line 2162 "cp-name-parser.c.tmp"
    break;

  case 36: /* oper: OPERATOR NOTEQUAL  */
#line 481 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("!=", 2); }
#line 2168 "cp-name-parser.c.tmp"
    break;

  case 37: /* oper: OPERATOR LEQ  */
#line 483 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("<=", 2); }
#line 2174 "cp-name-parser.c.tmp"
    break;

  case 38: /* oper: OPERATOR GEQ  */
#line 485 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator (">=", 2); }
#line 2180 "cp-name-parser.c.tmp"
    break;

  case 39: /* oper: OPERATOR ANDAND  */
#line 487 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("&&", 2); }
#line 2186 "cp-name-parser.c.tmp"
    break;

  case 40: /* oper: OPERATOR OROR  */
#line 489 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("||", 2); }
#line 2192 "cp-name-parser.c.tmp"
    break;

  case 41: /* oper: OPERATOR INCREMENT  */
#line 491 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("++", 1); }
#line 2198 "cp-name-parser.c.tmp"
    break;

  case 42: /* oper: OPERATOR DECREMENT  */
#line 493 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("--", 1); }
#line 2204 "cp-name-parser.c.tmp"
    break;

  case 43: /* oper: OPERATOR ','  */
#line 495 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator (",", 2); }
#line 2210 "cp-name-parser.c.tmp"
    break;

  case 44: /* oper: OPERATOR ARROW '*'  */
#line 497 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("->*", 2); }
#line 2216 "cp-name-parser.c.tmp"
    break;

  case 45: /* oper: OPERATOR ARROW  */
#line 499 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("->", 2); }
#line 2222 "cp-name-parser.c.tmp"
    break;

  case 46: /* oper: OPERATOR '(' ')'  */
#line 501 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("()", 2); }
#line 2228 "cp-name-parser.c.tmp"
    break;

  case 47: /* oper: OPERATOR '[' ']'  */
#line 503 "cp-name-parser.y"
                        { (yyval.comp) = state->make_operator ("[]", 2); }
#line 2234 "cp-name-parser.c.tmp"
    break;

  case 48: /* conversion_op: OPERATOR typespec_2  */
#line 511 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_CONVERSION, (yyvsp[0].comp), NULL); }
#line 2240 "cp-name-parser.c.tmp"
    break;

  case 49: /* conversion_op_name: nested_name conversion_op  */
#line 516 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-1].nested1).comp;
			  d_right ((yyvsp[-1].nested1).last) = (yyvsp[0].comp);
			  (yyval.nested).last = &d_left ((yyvsp[0].comp));
			}
#line 2249 "cp-name-parser.c.tmp"
    break;

  case 50: /* conversion_op_name: conversion_op  */
#line 521 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[0].comp);
			  (yyval.nested).last = &d_left ((yyvsp[0].comp));
			}
#line 2257 "cp-name-parser.c.tmp"
    break;

  case 51: /* conversion_op_name: COLONCOLON nested_name conversion_op  */
#line 525 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-1].nested1).comp;
			  d_right ((yyvsp[-1].nested1).last) = (yyvsp[0].comp);
			  (yyval.nested).last = &d_left ((yyvsp[0].comp));
			}
#line 2266 "cp-name-parser.c.tmp"
    break;

  case 52: /* conversion_op_name: COLONCOLON conversion_op  */
#line 530 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[0].comp);
			  (yyval.nested).last = &d_left ((yyvsp[0].comp));
			}
#line 2274 "cp-name-parser.c.tmp"
    break;

  case 54: /* unqualified_name: oper '<' template_params '>'  */
#line 539 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_TEMPLATE, (yyvsp[-3].comp), (yyvsp[-1].nested).comp); }
#line 2280 "cp-name-parser.c.tmp"
    break;

  case 55: /* unqualified_name: '~' NAME  */
#line 541 "cp-name-parser.y"
                        { (yyval.comp) = state->make_dtor (gnu_v3_complete_object_dtor, (yyvsp[0].comp)); }
#line 2286 "cp-name-parser.c.tmp"
    break;

  case 57: /* colon_name: COLONCOLON name  */
#line 554 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[0].comp); }
#line 2292 "cp-name-parser.c.tmp"
    break;

  case 58: /* name: nested_name NAME  */
#line 560 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[-1].nested1).comp; d_right ((yyvsp[-1].nested1).last) = (yyvsp[0].comp); }
#line 2298 "cp-name-parser.c.tmp"
    break;

  case 60: /* name: nested_name templ  */
#line 563 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[-1].nested1).comp; d_right ((yyvsp[-1].nested1).last) = (yyvsp[0].comp); }
#line 2304 "cp-name-parser.c.tmp"
    break;

  case 65: /* colon_ext_only: COLONCOLON ext_only_name  */
#line 573 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[0].comp); }
#line 2310 "cp-name-parser.c.tmp"
    break;

  case 66: /* ext_only_name: nested_name unqualified_name  */
#line 577 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[-1].nested1).comp; d_right ((yyvsp[-1].nested1).last) = (yyvsp[0].comp); }
#line 2316 "cp-name-parser.c.tmp"
    break;

  case 68: /* nested_name: NAME COLONCOLON  */
#line 582 "cp-name-parser.y"
                        { (yyval.nested1).comp = state->fill_comp (DEMANGLE_COMPONENT_QUAL_NAME, (yyvsp[-1].comp), NULL);
			  (yyval.nested1).last = (yyval.nested1).comp;
			}
#line 2324 "cp-name-parser.c.tmp"
    break;

  case 69: /* nested_name: nested_name NAME COLONCOLON  */
#line 586 "cp-name-parser.y"
                        { (yyval.nested1).comp = (yyvsp[-2].nested1).comp;
			  d_right ((yyvsp[-2].nested1).last) = state->fill_comp (DEMANGLE_COMPONENT_QUAL_NAME, (yyvsp[-1].comp), NULL);
			  (yyval.nested1).last = d_right ((yyvsp[-2].nested1).last);
			}
#line 2333 "cp-name-parser.c.tmp"
    break;

  case 70: /* nested_name: templ COLONCOLON  */
#line 591 "cp-name-parser.y"
                        { (yyval.nested1).comp = state->fill_comp (DEMANGLE_COMPONENT_QUAL_NAME, (yyvsp[-1].comp), NULL);
			  (yyval.nested1).last = (yyval.nested1).comp;
			}
#line 2341 "cp-name-parser.c.tmp"
    break;

  case 71: /* nested_name: nested_name templ COLONCOLON  */
#line 595 "cp-name-parser.y"
                        { (yyval.nested1).comp = (yyvsp[-2].nested1).comp;
			  d_right ((yyvsp[-2].nested1).last) = state->fill_comp (DEMANGLE_COMPONENT_QUAL_NAME, (yyvsp[-1].comp), NULL);
			  (yyval.nested1).last = d_right ((yyvsp[-2].nested1).last);
			}
#line 2350 "cp-name-parser.c.tmp"
    break;

  case 72: /* templ: NAME '<' template_params '>'  */
#line 604 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_TEMPLATE, (yyvsp[-3].comp), (yyvsp[-1].nested).comp); }
#line 2356 "cp-name-parser.c.tmp"
    break;

  case 73: /* template_params: template_arg  */
#line 608 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_TEMPLATE_ARGLIST, (yyvsp[0].comp), NULL);
			(yyval.nested).last = &d_right ((yyval.nested).comp); }
#line 2363 "cp-name-parser.c.tmp"
    break;

  case 74: /* template_params: template_params ',' template_arg  */
#line 611 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-2].nested).comp;
			  *(yyvsp[-2].nested).last = state->fill_comp (DEMANGLE_COMPONENT_TEMPLATE_ARGLIST, (yyvsp[0].comp), NULL);
			  (yyval.nested).last = &d_right (*(yyvsp[-2].nested).last);
			}
#line 2372 "cp-name-parser.c.tmp"
    break;

  case 76: /* template_arg: typespec_2 abstract_declarator  */
#line 623 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[0].abstract).comp;
			  *(yyvsp[0].abstract).last = (yyvsp[-1].comp);
			}
#line 2380 "cp-name-parser.c.tmp"
    break;

  case 77: /* template_arg: '&' start  */
#line 627 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_UNARY, state->make_operator ("&", 1), (yyvsp[0].comp)); }
#line 2386 "cp-name-parser.c.tmp"
    break;

  case 78: /* template_arg: '&' '(' start ')'  */
#line 629 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_UNARY, state->make_operator ("&", 1), (yyvsp[-1].comp)); }
#line 2392 "cp-name-parser.c.tmp"
    break;

  case 80: /* function_args: typespec_2  */
#line 634 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_ARGLIST, (yyvsp[0].comp), NULL);
			  (yyval.nested).last = &d_right ((yyval.nested).comp);
			}
#line 2400 "cp-name-parser.c.tmp"
    break;

  case 81: /* function_args: typespec_2 abstract_declarator  */
#line 638 "cp-name-parser.y"
                        { *(yyvsp[0].abstract).last = (yyvsp[-1].comp);
			  (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_ARGLIST, (yyvsp[0].abstract).comp, NULL);
			  (yyval.nested).last = &d_right ((yyval.nested).comp);
			}
#line 2409 "cp-name-parser.c.tmp"
    break;

  case 82: /* function_args: function_args ',' typespec_2  */
#line 643 "cp-name-parser.y"
                        { *(yyvsp[-2].nested).last = state->fill_comp (DEMANGLE_COMPONENT_ARGLIST, (yyvsp[0].comp), NULL);
			  (yyval.nested).comp = (yyvsp[-2].nested).comp;
			  (yyval.nested).last = &d_right (*(yyvsp[-2].nested).last);
			}
#line 2418 "cp-name-parser.c.tmp"
    break;

  case 83: /* function_args: function_args ',' typespec_2 abstract_declarator  */
#line 648 "cp-name-parser.y"
                        { *(yyvsp[0].abstract).last = (yyvsp[-1].comp);
			  *(yyvsp[-3].nested).last = state->fill_comp (DEMANGLE_COMPONENT_ARGLIST, (yyvsp[0].abstract).comp, NULL);
			  (yyval.nested).comp = (yyvsp[-3].nested).comp;
			  (yyval.nested).last = &d_right (*(yyvsp[-3].nested).last);
			}
#line 2428 "cp-name-parser.c.tmp"
    break;

  case 84: /* function_args: function_args ',' ELLIPSIS  */
#line 654 "cp-name-parser.y"
                        { *(yyvsp[-2].nested).last
			    = state->fill_comp (DEMANGLE_COMPONENT_ARGLIST,
					   state->make_builtin_type ("..."),
					   NULL);
			  (yyval.nested).comp = (yyvsp[-2].nested).comp;
			  (yyval.nested).last = &d_right (*(yyvsp[-2].nested).last);
			}
#line 2440 "cp-name-parser.c.tmp"
    break;

  case 85: /* function_arglist: '(' function_args ')' qualifiers_opt  */
#line 664 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_FUNCTION_TYPE, NULL, (yyvsp[-2].nested).comp);
			  (yyval.nested).last = &d_left ((yyval.nested).comp);
			  (yyval.nested).comp = state->d_qualify ((yyval.nested).comp, (yyvsp[0].lval), 1); }
#line 2448 "cp-name-parser.c.tmp"
    break;

  case 86: /* function_arglist: '(' VOID ')' qualifiers_opt  */
#line 668 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_FUNCTION_TYPE, NULL, NULL);
			  (yyval.nested).last = &d_left ((yyval.nested).comp);
			  (yyval.nested).comp = state->d_qualify ((yyval.nested).comp, (yyvsp[0].lval), 1); }
#line 2456 "cp-name-parser.c.tmp"
    break;

  case 87: /* function_arglist: '(' ')' qualifiers_opt  */
#line 672 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_FUNCTION_TYPE, NULL, NULL);
			  (yyval.nested).last = &d_left ((yyval.nested).comp);
			  (yyval.nested).comp = state->d_qualify ((yyval.nested).comp, (yyvsp[0].lval), 1); }
#line 2464 "cp-name-parser.c.tmp"
    break;

  case 88: /* qualifiers_opt: %empty  */
#line 679 "cp-name-parser.y"
                        { (yyval.lval) = 0; }
#line 2470 "cp-name-parser.c.tmp"
    break;

  case 90: /* qualifier: RESTRICT  */
#line 684 "cp-name-parser.y"
                        { (yyval.lval) = QUAL_RESTRICT; }
#line 2476 "cp-name-parser.c.tmp"
    break;

  case 91: /* qualifier: VOLATILE_KEYWORD  */
#line 686 "cp-name-parser.y"
                        { (yyval.lval) = QUAL_VOLATILE; }
#line 2482 "cp-name-parser.c.tmp"
    break;

  case 92: /* qualifier: CONST_KEYWORD  */
#line 688 "cp-name-parser.y"
                        { (yyval.lval) = QUAL_CONST; }
#line 2488 "cp-name-parser.c.tmp"
    break;

  case 94: /* qualifiers: qualifier qualifiers  */
#line 693 "cp-name-parser.y"
                        { (yyval.lval) = (yyvsp[-1].lval) | (yyvsp[0].lval); }
#line 2494 "cp-name-parser.c.tmp"
    break;

  case 95: /* int_part: INT_KEYWORD  */
#line 700 "cp-name-parser.y"
                        { (yyval.lval) = 0; }
#line 2500 "cp-name-parser.c.tmp"
    break;

  case 96: /* int_part: SIGNED_KEYWORD  */
#line 702 "cp-name-parser.y"
                        { (yyval.lval) = INT_SIGNED; }
#line 2506 "cp-name-parser.c.tmp"
    break;

  case 97: /* int_part: UNSIGNED  */
#line 704 "cp-name-parser.y"
                        { (yyval.lval) = INT_UNSIGNED; }
#line 2512 "cp-name-parser.c.tmp"
    break;

  case 98: /* int_part: CHAR  */
#line 706 "cp-name-parser.y"
                        { (yyval.lval) = INT_CHAR; }
#line 2518 "cp-name-parser.c.tmp"
    break;

  case 99: /* int_part: LONG  */
#line 708 "cp-name-parser.y"
                        { (yyval.lval) = INT_LONG; }
#line 2524 "cp-name-parser.c.tmp"
    break;

  case 100: /* int_part: SHORT  */
#line 710 "cp-name-parser.y"
                        { (yyval.lval) = INT_SHORT; }
#line 2530 "cp-name-parser.c.tmp"
    break;

  case 102: /* int_seq: int_seq int_part  */
#line 715 "cp-name-parser.y"
                        { (yyval.lval) = (yyvsp[-1].lval) | (yyvsp[0].lval); if ((yyvsp[-1].lval) & (yyvsp[0].lval) & INT_LONG) (yyval.lval) = (yyvsp[-1].lval) | INT_LLONG; }
#line 2536 "cp-name-parser.c.tmp"
    break;

  case 103: /* builtin_type: int_seq  */
#line 719 "cp-name-parser.y"
                        { (yyval.comp) = state->d_int_type ((yyvsp[0].lval)); }
#line 2542 "cp-name-parser.c.tmp"
    break;

  case 104: /* builtin_type: FLOAT_KEYWORD  */
#line 721 "cp-name-parser.y"
                        { (yyval.comp) = state->make_builtin_type ("float"); }
#line 2548 "cp-name-parser.c.tmp"
    break;

  case 105: /* builtin_type: DOUBLE_KEYWORD  */
#line 723 "cp-name-parser.y"
                        { (yyval.comp) = state->make_builtin_type ("double"); }
#line 2554 "cp-name-parser.c.tmp"
    break;

  case 106: /* builtin_type: LONG DOUBLE_KEYWORD  */
#line 725 "cp-name-parser.y"
                        { (yyval.comp) = state->make_builtin_type ("long double"); }
#line 2560 "cp-name-parser.c.tmp"
    break;

  case 107: /* builtin_type: BOOL  */
#line 727 "cp-name-parser.y"
                        { (yyval.comp) = state->make_builtin_type ("bool"); }
#line 2566 "cp-name-parser.c.tmp"
    break;

  case 108: /* builtin_type: WCHAR_T  */
#line 729 "cp-name-parser.y"
                        { (yyval.comp) = state->make_builtin_type ("wchar_t"); }
#line 2572 "cp-name-parser.c.tmp"
    break;

  case 109: /* builtin_type: VOID  */
#line 731 "cp-name-parser.y"
                        { (yyval.comp) = state->make_builtin_type ("void"); }
#line 2578 "cp-name-parser.c.tmp"
    break;

  case 110: /* ptr_operator: '*' qualifiers_opt  */
#line 735 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_POINTER, NULL, NULL);
			  (yyval.nested).last = &d_left ((yyval.nested).comp);
			  (yyval.nested).comp = state->d_qualify ((yyval.nested).comp, (yyvsp[0].lval), 0); }
#line 2586 "cp-name-parser.c.tmp"
    break;

  case 111: /* ptr_operator: '&'  */
#line 740 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_REFERENCE, NULL, NULL);
			  (yyval.nested).last = &d_left ((yyval.nested).comp); }
#line 2593 "cp-name-parser.c.tmp"
    break;

  case 112: /* ptr_operator: ANDAND  */
#line 743 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_RVALUE_REFERENCE, NULL, NULL);
			  (yyval.nested).last = &d_left ((yyval.nested).comp); }
#line 2600 "cp-name-parser.c.tmp"
    break;

  case 113: /* ptr_operator: nested_name '*' qualifiers_opt  */
#line 746 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_PTRMEM_TYPE, (yyvsp[-2].nested1).comp, NULL);
			  /* Convert the innermost DEMANGLE_COMPONENT_QUAL_NAME to a DEMANGLE_COMPONENT_NAME.  */
			  *(yyvsp[-2].nested1).last = *d_left ((yyvsp[-2].nested1).last);
			  (yyval.nested).last = &d_right ((yyval.nested).comp);
			  (yyval.nested).comp = state->d_qualify ((yyval.nested).comp, (yyvsp[0].lval), 0); }
#line 2610 "cp-name-parser.c.tmp"
    break;

  case 114: /* ptr_operator: COLONCOLON nested_name '*' qualifiers_opt  */
#line 752 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_PTRMEM_TYPE, (yyvsp[-2].nested1).comp, NULL);
			  /* Convert the innermost DEMANGLE_COMPONENT_QUAL_NAME to a DEMANGLE_COMPONENT_NAME.  */
			  *(yyvsp[-2].nested1).last = *d_left ((yyvsp[-2].nested1).last);
			  (yyval.nested).last = &d_right ((yyval.nested).comp);
			  (yyval.nested).comp = state->d_qualify ((yyval.nested).comp, (yyvsp[0].lval), 0); }
#line 2620 "cp-name-parser.c.tmp"
    break;

  case 115: /* array_indicator: '[' ']'  */
#line 760 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_ARRAY_TYPE, NULL, NULL); }
#line 2626 "cp-name-parser.c.tmp"
    break;

  case 116: /* array_indicator: '[' INT ']'  */
#line 762 "cp-name-parser.y"
                        { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_ARRAY_TYPE, (yyvsp[-1].comp), NULL); }
#line 2632 "cp-name-parser.c.tmp"
    break;

  case 117: /* typespec_2: builtin_type qualifiers  */
#line 776 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[-1].comp), (yyvsp[0].lval), 0); }
#line 2638 "cp-name-parser.c.tmp"
    break;

  case 119: /* typespec_2: qualifiers builtin_type qualifiers  */
#line 779 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[-1].comp), (yyvsp[-2].lval) | (yyvsp[0].lval), 0); }
#line 2644 "cp-name-parser.c.tmp"
    break;

  case 120: /* typespec_2: qualifiers builtin_type  */
#line 781 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[0].comp), (yyvsp[-1].lval), 0); }
#line 2650 "cp-name-parser.c.tmp"
    break;

  case 121: /* typespec_2: name qualifiers  */
#line 784 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[-1].comp), (yyvsp[0].lval), 0); }
#line 2656 "cp-name-parser.c.tmp"
    break;

  case 123: /* typespec_2: qualifiers name qualifiers  */
#line 787 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[-1].comp), (yyvsp[-2].lval) | (yyvsp[0].lval), 0); }
#line 2662 "cp-name-parser.c.tmp"
    break;

  case 124: /* typespec_2: qualifiers name  */
#line 789 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[0].comp), (yyvsp[-1].lval), 0); }
#line 2668 "cp-name-parser.c.tmp"
    break;

  case 125: /* typespec_2: COLONCOLON name qualifiers  */
#line 792 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[-1].comp), (yyvsp[0].lval), 0); }
#line 2674 "cp-name-parser.c.tmp"
    break;

  case 126: /* typespec_2: COLONCOLON name  */
#line 794 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[0].comp); }
#line 2680 "cp-name-parser.c.tmp"
    break;

  case 127: /* typespec_2: qualifiers COLONCOLON name qualifiers  */
#line 796 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[-1].comp), (yyvsp[-3].lval) | (yyvsp[0].lval), 0); }
#line 2686 "cp-name-parser.c.tmp"
    break;

  case 128: /* typespec_2: qualifiers COLONCOLON name  */
#line 798 "cp-name-parser.y"
                        { (yyval.comp) = state->d_qualify ((yyvsp[0].comp), (yyvsp[-2].lval), 0); }
#line 2692 "cp-name-parser.c.tmp"
    break;

  case 129: /* abstract_declarator: ptr_operator  */
#line 803 "cp-name-parser.y"
                        { (yyval.abstract).comp = (yyvsp[0].nested).comp; (yyval.abstract).last = (yyvsp[0].nested).last;
			  (yyval.abstract).fn.comp = NULL; (yyval.abstract).fn.last = NULL; }
#line 2699 "cp-name-parser.c.tmp"
    break;

  case 130: /* abstract_declarator: ptr_operator abstract_declarator  */
#line 806 "cp-name-parser.y"
                        { (yyval.abstract) = (yyvsp[0].abstract); (yyval.abstract).fn.comp = NULL; (yyval.abstract).fn.last = NULL;
			  if ((yyvsp[0].abstract).fn.comp) { (yyval.abstract).last = (yyvsp[0].abstract).fn.last; *(yyvsp[0].abstract).last = (yyvsp[0].abstract).fn.comp; }
			  *(yyval.abstract).last = (yyvsp[-1].nested).comp;
			  (yyval.abstract).last = (yyvsp[-1].nested).last; }
#line 2708 "cp-name-parser.c.tmp"
    break;

  case 131: /* abstract_declarator: direct_abstract_declarator  */
#line 811 "cp-name-parser.y"
                        { (yyval.abstract).fn.comp = NULL; (yyval.abstract).fn.last = NULL;
			  if ((yyvsp[0].abstract).fn.comp) { (yyval.abstract).last = (yyvsp[0].abstract).fn.last; *(yyvsp[0].abstract).last = (yyvsp[0].abstract).fn.comp; }
			}
#line 2716 "cp-name-parser.c.tmp"
    break;

  case 132: /* direct_abstract_declarator: '(' abstract_declarator ')'  */
#line 818 "cp-name-parser.y"
                        { (yyval.abstract) = (yyvsp[-1].abstract); (yyval.abstract).fn.comp = NULL; (yyval.abstract).fn.last = NULL; (yyval.abstract).fold_flag = 1;
			  if ((yyvsp[-1].abstract).fn.comp) { (yyval.abstract).last = (yyvsp[-1].abstract).fn.last; *(yyvsp[-1].abstract).last = (yyvsp[-1].abstract).fn.comp; }
			}
#line 2724 "cp-name-parser.c.tmp"
    break;

  case 133: /* direct_abstract_declarator: direct_abstract_declarator function_arglist  */
#line 822 "cp-name-parser.y"
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
#line 2739 "cp-name-parser.c.tmp"
    break;

  case 134: /* direct_abstract_declarator: direct_abstract_declarator array_indicator  */
#line 833 "cp-name-parser.y"
                        { (yyval.abstract).fn.comp = NULL; (yyval.abstract).fn.last = NULL; (yyval.abstract).fold_flag = 0;
			  if ((yyvsp[-1].abstract).fn.comp) { (yyval.abstract).last = (yyvsp[-1].abstract).fn.last; *(yyvsp[-1].abstract).last = (yyvsp[-1].abstract).fn.comp; }
			  *(yyvsp[-1].abstract).last = (yyvsp[0].comp);
			  (yyval.abstract).last = &d_right ((yyvsp[0].comp));
			}
#line 2749 "cp-name-parser.c.tmp"
    break;

  case 135: /* direct_abstract_declarator: array_indicator  */
#line 839 "cp-name-parser.y"
                        { (yyval.abstract).fn.comp = NULL; (yyval.abstract).fn.last = NULL; (yyval.abstract).fold_flag = 0;
			  (yyval.abstract).comp = (yyvsp[0].comp);
			  (yyval.abstract).last = &d_right ((yyvsp[0].comp));
			}
#line 2758 "cp-name-parser.c.tmp"
    break;

  case 136: /* abstract_declarator_fn: ptr_operator  */
#line 857 "cp-name-parser.y"
                        { (yyval.abstract).comp = (yyvsp[0].nested).comp; (yyval.abstract).last = (yyvsp[0].nested).last;
			  (yyval.abstract).fn.comp = NULL; (yyval.abstract).fn.last = NULL; (yyval.abstract).start = NULL; }
#line 2765 "cp-name-parser.c.tmp"
    break;

  case 137: /* abstract_declarator_fn: ptr_operator abstract_declarator_fn  */
#line 860 "cp-name-parser.y"
                        { (yyval.abstract) = (yyvsp[0].abstract);
			  if ((yyvsp[0].abstract).last)
			    *(yyval.abstract).last = (yyvsp[-1].nested).comp;
			  else
			    (yyval.abstract).comp = (yyvsp[-1].nested).comp;
			  (yyval.abstract).last = (yyvsp[-1].nested).last;
			}
#line 2777 "cp-name-parser.c.tmp"
    break;

  case 138: /* abstract_declarator_fn: direct_abstract_declarator  */
#line 868 "cp-name-parser.y"
                        { (yyval.abstract).comp = (yyvsp[0].abstract).comp; (yyval.abstract).last = (yyvsp[0].abstract).last; (yyval.abstract).fn = (yyvsp[0].abstract).fn; (yyval.abstract).start = NULL; }
#line 2783 "cp-name-parser.c.tmp"
    break;

  case 139: /* abstract_declarator_fn: direct_abstract_declarator function_arglist COLONCOLON start  */
#line 870 "cp-name-parser.y"
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
#line 2798 "cp-name-parser.c.tmp"
    break;

  case 140: /* abstract_declarator_fn: function_arglist start_opt  */
#line 881 "cp-name-parser.y"
                        { (yyval.abstract).fn = (yyvsp[-1].nested);
			  (yyval.abstract).start = (yyvsp[0].comp);
			  (yyval.abstract).comp = NULL; (yyval.abstract).last = NULL;
			}
#line 2807 "cp-name-parser.c.tmp"
    break;

  case 142: /* type: typespec_2 abstract_declarator  */
#line 889 "cp-name-parser.y"
                        { (yyval.comp) = (yyvsp[0].abstract).comp;
			  *(yyvsp[0].abstract).last = (yyvsp[-1].comp);
			}
#line 2815 "cp-name-parser.c.tmp"
    break;

  case 143: /* declarator: ptr_operator declarator  */
#line 895 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[0].nested).comp;
			  (yyval.nested).last = (yyvsp[-1].nested).last;
			  *(yyvsp[0].nested).last = (yyvsp[-1].nested).comp; }
#line 2823 "cp-name-parser.c.tmp"
    break;

  case 145: /* direct_declarator: '(' declarator ')'  */
#line 903 "cp-name-parser.y"
                        { (yyval.nested) = (yyvsp[-1].nested); }
#line 2829 "cp-name-parser.c.tmp"
    break;

  case 146: /* direct_declarator: direct_declarator function_arglist  */
#line 905 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-1].nested).comp;
			  *(yyvsp[-1].nested).last = (yyvsp[0].nested).comp;
			  (yyval.nested).last = (yyvsp[0].nested).last;
			}
#line 2838 "cp-name-parser.c.tmp"
    break;

  case 147: /* direct_declarator: direct_declarator array_indicator  */
#line 910 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-1].nested).comp;
			  *(yyvsp[-1].nested).last = (yyvsp[0].comp);
			  (yyval.nested).last = &d_right ((yyvsp[0].comp));
			}
#line 2847 "cp-name-parser.c.tmp"
    break;

  case 148: /* direct_declarator: colon_ext_name  */
#line 915 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, (yyvsp[0].comp), NULL);
			  (yyval.nested).last = &d_right ((yyval.nested).comp);
			}
#line 2855 "cp-name-parser.c.tmp"
    break;

  case 149: /* declarator_1: ptr_operator declarator_1  */
#line 927 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[0].nested).comp;
			  (yyval.nested).last = (yyvsp[-1].nested).last;
			  *(yyvsp[0].nested).last = (yyvsp[-1].nested).comp; }
#line 2863 "cp-name-parser.c.tmp"
    break;

  case 150: /* declarator_1: colon_ext_name  */
#line 931 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, (yyvsp[0].comp), NULL);
			  (yyval.nested).last = &d_right ((yyval.nested).comp);
			}
#line 2871 "cp-name-parser.c.tmp"
    break;

  case 152: /* declarator_1: colon_ext_name function_arglist COLONCOLON start  */
#line 943 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, (yyvsp[-3].comp), (yyvsp[-2].nested).comp);
			  (yyval.nested).last = (yyvsp[-2].nested).last;
			  (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME, (yyval.nested).comp, (yyvsp[0].comp));
			}
#line 2880 "cp-name-parser.c.tmp"
    break;

  case 153: /* declarator_1: direct_declarator_1 function_arglist COLONCOLON start  */
#line 948 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-3].nested).comp;
			  *(yyvsp[-3].nested).last = (yyvsp[-2].nested).comp;
			  (yyval.nested).last = (yyvsp[-2].nested).last;
			  (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME, (yyval.nested).comp, (yyvsp[0].comp));
			}
#line 2890 "cp-name-parser.c.tmp"
    break;

  case 154: /* direct_declarator_1: '(' ptr_operator declarator ')'  */
#line 957 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-1].nested).comp;
			  (yyval.nested).last = (yyvsp[-2].nested).last;
			  *(yyvsp[-1].nested).last = (yyvsp[-2].nested).comp; }
#line 2898 "cp-name-parser.c.tmp"
    break;

  case 155: /* direct_declarator_1: direct_declarator_1 function_arglist  */
#line 961 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-1].nested).comp;
			  *(yyvsp[-1].nested).last = (yyvsp[0].nested).comp;
			  (yyval.nested).last = (yyvsp[0].nested).last;
			}
#line 2907 "cp-name-parser.c.tmp"
    break;

  case 156: /* direct_declarator_1: direct_declarator_1 array_indicator  */
#line 966 "cp-name-parser.y"
                        { (yyval.nested).comp = (yyvsp[-1].nested).comp;
			  *(yyvsp[-1].nested).last = (yyvsp[0].comp);
			  (yyval.nested).last = &d_right ((yyvsp[0].comp));
			}
#line 2916 "cp-name-parser.c.tmp"
    break;

  case 157: /* direct_declarator_1: colon_ext_name function_arglist  */
#line 971 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, (yyvsp[-1].comp), (yyvsp[0].nested).comp);
			  (yyval.nested).last = (yyvsp[0].nested).last;
			}
#line 2924 "cp-name-parser.c.tmp"
    break;

  case 158: /* direct_declarator_1: colon_ext_name array_indicator  */
#line 975 "cp-name-parser.y"
                        { (yyval.nested).comp = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, (yyvsp[-1].comp), (yyvsp[0].comp));
			  (yyval.nested).last = &d_right ((yyvsp[0].comp));
			}
#line 2932 "cp-name-parser.c.tmp"
    break;

  case 159: /* exp: '(' exp1 ')'  */
#line 981 "cp-name-parser.y"
                { (yyval.comp) = (yyvsp[-1].comp); }
#line 2938 "cp-name-parser.c.tmp"
    break;

  case 161: /* exp1: exp '>' exp  */
#line 990 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary (">", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 2944 "cp-name-parser.c.tmp"
    break;

  case 162: /* exp1: '&' start  */
#line 997 "cp-name-parser.y"
                { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_UNARY, state->make_operator ("&", 1), (yyvsp[0].comp)); }
#line 2950 "cp-name-parser.c.tmp"
    break;

  case 163: /* exp1: '&' '(' start ')'  */
#line 999 "cp-name-parser.y"
                { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_UNARY, state->make_operator ("&", 1), (yyvsp[-1].comp)); }
#line 2956 "cp-name-parser.c.tmp"
    break;

  case 164: /* exp: '-' exp  */
#line 1004 "cp-name-parser.y"
                { (yyval.comp) = state->d_unary ("-", (yyvsp[0].comp)); }
#line 2962 "cp-name-parser.c.tmp"
    break;

  case 165: /* exp: '!' exp  */
#line 1008 "cp-name-parser.y"
                { (yyval.comp) = state->d_unary ("!", (yyvsp[0].comp)); }
#line 2968 "cp-name-parser.c.tmp"
    break;

  case 166: /* exp: '~' exp  */
#line 1012 "cp-name-parser.y"
                { (yyval.comp) = state->d_unary ("~", (yyvsp[0].comp)); }
#line 2974 "cp-name-parser.c.tmp"
    break;

  case 167: /* exp: '(' type ')' exp  */
#line 1019 "cp-name-parser.y"
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
#line 2990 "cp-name-parser.c.tmp"
    break;

  case 168: /* exp: STATIC_CAST '<' type '>' '(' exp1 ')'  */
#line 1035 "cp-name-parser.y"
                { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_UNARY,
				    state->fill_comp (DEMANGLE_COMPONENT_CAST, (yyvsp[-4].comp), NULL),
				    (yyvsp[-1].comp));
		}
#line 2999 "cp-name-parser.c.tmp"
    break;

  case 169: /* exp: DYNAMIC_CAST '<' type '>' '(' exp1 ')'  */
#line 1042 "cp-name-parser.y"
                { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_UNARY,
				    state->fill_comp (DEMANGLE_COMPONENT_CAST, (yyvsp[-4].comp), NULL),
				    (yyvsp[-1].comp));
		}
#line 3008 "cp-name-parser.c.tmp"
    break;

  case 170: /* exp: REINTERPRET_CAST '<' type '>' '(' exp1 ')'  */
#line 1049 "cp-name-parser.y"
                { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_UNARY,
				    state->fill_comp (DEMANGLE_COMPONENT_CAST, (yyvsp[-4].comp), NULL),
				    (yyvsp[-1].comp));
		}
#line 3017 "cp-name-parser.c.tmp"
    break;

  case 171: /* exp: exp '*' exp  */
#line 1068 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("*", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3023 "cp-name-parser.c.tmp"
    break;

  case 172: /* exp: exp '/' exp  */
#line 1072 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("/", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3029 "cp-name-parser.c.tmp"
    break;

  case 173: /* exp: exp '%' exp  */
#line 1076 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("%", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3035 "cp-name-parser.c.tmp"
    break;

  case 174: /* exp: exp '+' exp  */
#line 1080 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("+", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3041 "cp-name-parser.c.tmp"
    break;

  case 175: /* exp: exp '-' exp  */
#line 1084 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("-", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3047 "cp-name-parser.c.tmp"
    break;

  case 176: /* exp: exp LSH exp  */
#line 1088 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("<<", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3053 "cp-name-parser.c.tmp"
    break;

  case 177: /* exp: exp RSH exp  */
#line 1092 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary (">>", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3059 "cp-name-parser.c.tmp"
    break;

  case 178: /* exp: exp EQUAL exp  */
#line 1096 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("==", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3065 "cp-name-parser.c.tmp"
    break;

  case 179: /* exp: exp NOTEQUAL exp  */
#line 1100 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("!=", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3071 "cp-name-parser.c.tmp"
    break;

  case 180: /* exp: exp LEQ exp  */
#line 1104 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("<=", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3077 "cp-name-parser.c.tmp"
    break;

  case 181: /* exp: exp GEQ exp  */
#line 1108 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary (">=", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3083 "cp-name-parser.c.tmp"
    break;

  case 182: /* exp: exp '<' exp  */
#line 1112 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("<", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3089 "cp-name-parser.c.tmp"
    break;

  case 183: /* exp: exp '&' exp  */
#line 1116 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("&", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3095 "cp-name-parser.c.tmp"
    break;

  case 184: /* exp: exp '^' exp  */
#line 1120 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("^", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3101 "cp-name-parser.c.tmp"
    break;

  case 185: /* exp: exp '|' exp  */
#line 1124 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("|", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3107 "cp-name-parser.c.tmp"
    break;

  case 186: /* exp: exp ANDAND exp  */
#line 1128 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("&&", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3113 "cp-name-parser.c.tmp"
    break;

  case 187: /* exp: exp OROR exp  */
#line 1132 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("||", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3119 "cp-name-parser.c.tmp"
    break;

  case 188: /* exp: exp ARROW NAME  */
#line 1137 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary ("->", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3125 "cp-name-parser.c.tmp"
    break;

  case 189: /* exp: exp '.' NAME  */
#line 1141 "cp-name-parser.y"
                { (yyval.comp) = state->d_binary (".", (yyvsp[-2].comp), (yyvsp[0].comp)); }
#line 3131 "cp-name-parser.c.tmp"
    break;

  case 190: /* exp: exp '?' exp ':' exp  */
#line 1145 "cp-name-parser.y"
                { (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_TRINARY, state->make_operator ("?", 3),
				    state->fill_comp (DEMANGLE_COMPONENT_TRINARY_ARG1, (yyvsp[-4].comp),
						 state->fill_comp (DEMANGLE_COMPONENT_TRINARY_ARG2, (yyvsp[-2].comp), (yyvsp[0].comp))));
		}
#line 3140 "cp-name-parser.c.tmp"
    break;

  case 193: /* exp: SIZEOF '(' type ')'  */
#line 1159 "cp-name-parser.y"
                {
		  /* Match the whitespacing of cplus_demangle_operators.
		     It would abort on unrecognized string otherwise.  */
		  (yyval.comp) = state->d_unary ("sizeof ", (yyvsp[-1].comp));
		}
#line 3150 "cp-name-parser.c.tmp"
    break;

  case 194: /* exp: TRUEKEYWORD  */
#line 1168 "cp-name-parser.y"
                { struct demangle_component *i;
		  i = state->make_name ("1", 1);
		  (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_LITERAL,
				    state->make_builtin_type ( "bool"),
				    i);
		}
#line 3161 "cp-name-parser.c.tmp"
    break;

  case 195: /* exp: FALSEKEYWORD  */
#line 1177 "cp-name-parser.y"
                { struct demangle_component *i;
		  i = state->make_name ("0", 1);
		  (yyval.comp) = state->fill_comp (DEMANGLE_COMPONENT_LITERAL,
				    state->make_builtin_type ("bool"),
				    i);
		}
#line 3172 "cp-name-parser.c.tmp"
    break;


#line 3176 "cp-name-parser.c.tmp"

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

#line 1187 "cp-name-parser.y"


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
			    YYSTYPE *lvalp)
{
  int unsigned_p = 0;

  /* Number of "L" suffixes encountered.  */
  int long_p = 0;

  struct demangle_component *signed_type;
  struct demangle_component *unsigned_type;
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

  /* This treats 0x1 and 1 as different literals.  We also do not
     automatically generate unsigned types.  */

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

  if (long_p == 0)
    {
      unsigned_type = make_builtin_type ("unsigned int");
      signed_type = make_builtin_type ("int");
    }
  else if (long_p == 1)
    {
      unsigned_type = make_builtin_type ("unsigned long");
      signed_type = make_builtin_type ("long");
    }
  else
    {
      unsigned_type = make_builtin_type ("unsigned long long");
      signed_type = make_builtin_type ("long long");
    }

   if (unsigned_p)
     type = unsigned_type;
   else
     type = signed_type;

   name = make_name (p, len);
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
yylex (YYSTYPE *lvalp, cpname_state *state)
{
  int c;
  int namelen;
  const char *tokstart;

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

      c = *state->lexptr++;
      if (c != '\'')
	{
	  yyerror (state, _("invalid character constant"));
	  return ERROR;
	}

      /* FIXME: We should refer to a canonical form of the character,
	 presumably the same one that appears in manglings - the decimal
	 representation.  But if that isn't in our input then we have to
	 allocate memory for it somewhere.  */
      lvalp->comp
	= state->fill_comp (DEMANGLE_COMPONENT_LITERAL,
			    state->make_builtin_type ("char"),
			    state->make_name (tokstart,
					      state->lexptr - tokstart));

      return INT;

    case '(':
      if (startswith (tokstart, "(anonymous namespace)"))
	{
	  state->lexptr += 21;
	  lvalp->comp = state->make_name ("(anonymous namespace)",
					  sizeof "(anonymous namespace)" - 1);
	  return NAME;
	}
	/* FALL THROUGH */

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
      /* FALL THRU.  */

    try_number:
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
	      /* This is the sign of the exponent, not the end of the
		 number.  */
	      continue;
	    /* We will take any letters or digits.  parse_number will
	       complain if past the radix, or if L or U are not final.  */
	    else if (! ISALNUM (*p))
	      break;
	  }
	toktype = state->parse_number (tokstart, p - tokstart, got_dot|got_e,
				       lvalp);
	if (toktype == ERROR)
	  {
	    char *err_copy = (char *) alloca (p - tokstart + 1);

	    memcpy (err_copy, tokstart, p - tokstart);
	    err_copy[p - tokstart] = 0;
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

/* Allocate a chunk of the components we'll need to build a tree.  We
   generally allocate too many components, but the extra memory usage
   doesn't hurt because the trees are temporary and the storage is
   reused.  More may be allocated later, by d_grab.  */
static struct demangle_info *
allocate_info (void)
{
  struct demangle_info *info = XNEW (struct demangle_info);

  info->next = NULL;
  info->used = 0;
  return info;
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

/* Constructor for demangle_parse_info.  */

demangle_parse_info::demangle_parse_info ()
: info (NULL),
  tree (NULL)
{
  obstack_init (&obstack);
}

/* Destructor for demangle_parse_info.  */

demangle_parse_info::~demangle_parse_info ()
{
  /* Free any allocated chunks of memory for the parse.  */
  while (info != NULL)
    {
      struct demangle_info *next = info->next;

      xfree (info);
      info = next;
    }

  /* Free any memory allocated during typedef replacement.  */
  obstack_free (&obstack, NULL);
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
			       struct demangle_parse_info *src)

{
  struct demangle_info *di;

  /* Copy the SRC's parse data into DEST.  */
  *target = *src->tree;
  di = dest->info;
  while (di->next != NULL)
    di = di->next;
  di->next = src->info;

  /* Clear the (pointer to) SRC's parse data so that it is not freed when
     cp_demangled_parse_info_free is called.  */
  src->info = NULL;
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

  state.demangle_info = allocate_info ();

  std::unique_ptr<demangle_parse_info> result (new demangle_parse_info);
  result->info = state.demangle_info;

  if (yyparse (&state))
    {
      if (state.global_errmsg && errmsg)
	*errmsg = state.global_errmsg;
      return NULL;
    }

  result->tree = state.global_result;

  return result;
}

#ifdef TEST_CPNAMES

static void
cp_print (struct demangle_component *result)
{
  char *str;
  size_t err = 0;

  str = gdb_cplus_demangle_print (DMGL_PARAMS | DMGL_ANSI, result, 64, &err);
  if (str == NULL)
    return;

  fputs (str, stdout);

  xfree (str);
}

static char
trim_chars (char *lexptr, char **extra_chars)
{
  char *p = (char *) symbol_end (lexptr);
  char c = 0;

  if (*p)
    {
      c = *p;
      *p = 0;
      *extra_chars = p + 1;
    }

  return c;
}

/* When this file is built as a standalone program, xmalloc comes from
   libiberty --- in which case we have to provide xfree ourselves.  */

void
xfree (void *ptr)
{
  if (ptr != NULL)
    {
      /* Literal `free' would get translated back to xfree again.  */
      CONCAT2 (fr,ee) (ptr);
    }
}

/* GDB normally defines internal_error itself, but when this file is built
   as a standalone program, we must also provide an implementation.  */

void
internal_error (const char *file, int line, const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  fprintf (stderr, "%s:%d: internal error: ", file, line);
  vfprintf (stderr, fmt, ap);
  exit (1);
}

int
main (int argc, char **argv)
{
  char *str2, *extra_chars, c;
  char buf[65536];
  int arg;

  arg = 1;
  if (argv[arg] && strcmp (argv[arg], "--debug") == 0)
    {
      yydebug = 1;
      arg++;
    }

  if (argv[arg] == NULL)
    while (fgets (buf, 65536, stdin) != NULL)
      {
	buf[strlen (buf) - 1] = 0;
	/* Use DMGL_VERBOSE to get expanded standard substitutions.  */
	c = trim_chars (buf, &extra_chars);
	str2 = cplus_demangle (buf, DMGL_PARAMS | DMGL_ANSI | DMGL_VERBOSE);
	if (str2 == NULL)
	  {
	    printf ("Demangling error\n");
	    if (c)
	      printf ("%s%c%s\n", buf, c, extra_chars);
	    else
	      printf ("%s\n", buf);
	    continue;
	  }

	std::string errmsg;
	std::unique_ptr<demangle_parse_info> result
	  = cp_demangled_name_to_comp (str2, &errmsg);
	if (result == NULL)
	  {
	    fputs (errmsg.c_str (), stderr);
	    fputc ('\n', stderr);
	    continue;
	  }

	cp_print (result->tree);

	xfree (str2);
	if (c)
	  {
	    putchar (c);
	    fputs (extra_chars, stdout);
	  }
	putchar ('\n');
      }
  else
    {
      std::string errmsg;
      std::unique_ptr<demangle_parse_info> result
	= cp_demangled_name_to_comp (argv[arg], &errmsg);
      if (result == NULL)
	{
	  fputs (errmsg.c_str (), stderr);
	  fputc ('\n', stderr);
	  return 0;
	}
      cp_print (result->tree);
      putchar ('\n');
    }
  return 0;
}

#endif
