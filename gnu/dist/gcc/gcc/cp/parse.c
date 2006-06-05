/* A Bison parser, made by GNU Bison 1.875.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     IDENTIFIER = 258,
     tTYPENAME = 259,
     SELFNAME = 260,
     PFUNCNAME = 261,
     SCSPEC = 262,
     TYPESPEC = 263,
     CV_QUALIFIER = 264,
     CONSTANT = 265,
     VAR_FUNC_NAME = 266,
     STRING = 267,
     ELLIPSIS = 268,
     SIZEOF = 269,
     ENUM = 270,
     IF = 271,
     ELSE = 272,
     WHILE = 273,
     DO = 274,
     FOR = 275,
     SWITCH = 276,
     CASE = 277,
     DEFAULT = 278,
     BREAK = 279,
     CONTINUE = 280,
     RETURN_KEYWORD = 281,
     GOTO = 282,
     ASM_KEYWORD = 283,
     TYPEOF = 284,
     ALIGNOF = 285,
     SIGOF = 286,
     ATTRIBUTE = 287,
     EXTENSION = 288,
     LABEL = 289,
     REALPART = 290,
     IMAGPART = 291,
     VA_ARG = 292,
     AGGR = 293,
     VISSPEC = 294,
     DELETE = 295,
     NEW = 296,
     THIS = 297,
     OPERATOR = 298,
     CXX_TRUE = 299,
     CXX_FALSE = 300,
     NAMESPACE = 301,
     TYPENAME_KEYWORD = 302,
     USING = 303,
     LEFT_RIGHT = 304,
     TEMPLATE = 305,
     TYPEID = 306,
     DYNAMIC_CAST = 307,
     STATIC_CAST = 308,
     REINTERPRET_CAST = 309,
     CONST_CAST = 310,
     SCOPE = 311,
     EXPORT = 312,
     EMPTY = 313,
     NSNAME = 314,
     PTYPENAME = 315,
     THROW = 316,
     ASSIGN = 317,
     OROR = 318,
     ANDAND = 319,
     MIN_MAX = 320,
     EQCOMPARE = 321,
     ARITHCOMPARE = 322,
     RSHIFT = 323,
     LSHIFT = 324,
     DOT_STAR = 325,
     POINTSAT_STAR = 326,
     MINUSMINUS = 327,
     PLUSPLUS = 328,
     UNARY = 329,
     HYPERUNARY = 330,
     POINTSAT = 331,
     CATCH = 332,
     TRY = 333,
     EXTERN_LANG_STRING = 334,
     ALL = 335,
     PRE_PARSED_CLASS_DECL = 336,
     DEFARG = 337,
     DEFARG_MARKER = 338,
     PRE_PARSED_FUNCTION_DECL = 339,
     TYPENAME_DEFN = 340,
     IDENTIFIER_DEFN = 341,
     PTYPENAME_DEFN = 342,
     END_OF_LINE = 343,
     END_OF_SAVED_INPUT = 344
   };
#endif
#define IDENTIFIER 258
#define tTYPENAME 259
#define SELFNAME 260
#define PFUNCNAME 261
#define SCSPEC 262
#define TYPESPEC 263
#define CV_QUALIFIER 264
#define CONSTANT 265
#define VAR_FUNC_NAME 266
#define STRING 267
#define ELLIPSIS 268
#define SIZEOF 269
#define ENUM 270
#define IF 271
#define ELSE 272
#define WHILE 273
#define DO 274
#define FOR 275
#define SWITCH 276
#define CASE 277
#define DEFAULT 278
#define BREAK 279
#define CONTINUE 280
#define RETURN_KEYWORD 281
#define GOTO 282
#define ASM_KEYWORD 283
#define TYPEOF 284
#define ALIGNOF 285
#define SIGOF 286
#define ATTRIBUTE 287
#define EXTENSION 288
#define LABEL 289
#define REALPART 290
#define IMAGPART 291
#define VA_ARG 292
#define AGGR 293
#define VISSPEC 294
#define DELETE 295
#define NEW 296
#define THIS 297
#define OPERATOR 298
#define CXX_TRUE 299
#define CXX_FALSE 300
#define NAMESPACE 301
#define TYPENAME_KEYWORD 302
#define USING 303
#define LEFT_RIGHT 304
#define TEMPLATE 305
#define TYPEID 306
#define DYNAMIC_CAST 307
#define STATIC_CAST 308
#define REINTERPRET_CAST 309
#define CONST_CAST 310
#define SCOPE 311
#define EXPORT 312
#define EMPTY 313
#define NSNAME 314
#define PTYPENAME 315
#define THROW 316
#define ASSIGN 317
#define OROR 318
#define ANDAND 319
#define MIN_MAX 320
#define EQCOMPARE 321
#define ARITHCOMPARE 322
#define RSHIFT 323
#define LSHIFT 324
#define DOT_STAR 325
#define POINTSAT_STAR 326
#define MINUSMINUS 327
#define PLUSPLUS 328
#define UNARY 329
#define HYPERUNARY 330
#define POINTSAT 331
#define CATCH 332
#define TRY 333
#define EXTERN_LANG_STRING 334
#define ALL 335
#define PRE_PARSED_CLASS_DECL 336
#define DEFARG 337
#define DEFARG_MARKER 338
#define PRE_PARSED_FUNCTION_DECL 339
#define TYPENAME_DEFN 340
#define IDENTIFIER_DEFN 341
#define PTYPENAME_DEFN 342
#define END_OF_LINE 343
#define END_OF_SAVED_INPUT 344




/* Copy the first part of user declarations.  */
#line 30 "parse.y"

#include "config.h"

#include "system.h"

#include "tree.h"
#include "input.h"
#include "flags.h"
#include "cp-tree.h"
#include "decl.h"
#include "lex.h"
#include "c-pragma.h"		/* For YYDEBUG definition.  */
#include "output.h"
#include "except.h"
#include "toplev.h"
#include "ggc.h"

/* Like YYERROR but do call yyerror.  */
#define YYERROR1 { yyerror ("syntax error"); YYERROR; }

/* Like the default stack expander, except (1) use realloc when possible,
   (2) impose no hard maxiumum on stack size, (3) REALLY do not use alloca.

   Irritatingly, YYSTYPE is defined after this %{ %} block, so we cannot
   give malloced_yyvs its proper type.  This is ok since all we need from
   it is to be able to free it.  */

static short *malloced_yyss;
static void *malloced_yyvs;
static int class_template_ok_as_expr;

#define yyoverflow(MSG, SS, SSSIZE, VS, VSSIZE, YYSSZ)			\
do {									\
  size_t newsize;							\
  short *newss;								\
  YYSTYPE *newvs;							\
  newsize = *(YYSSZ) *= 2;						\
  if (malloced_yyss)							\
    {									\
      newss = (short *)							\
	really_call_realloc (*(SS), newsize * sizeof (short));		\
      newvs = (YYSTYPE *)						\
	really_call_realloc (*(VS), newsize * sizeof (YYSTYPE));	\
    }									\
  else									\
    {									\
      newss = (short *) really_call_malloc (newsize * sizeof (short));	\
      newvs = (YYSTYPE *) really_call_malloc (newsize * sizeof (YYSTYPE)); \
      if (newss)							\
        memcpy (newss, *(SS), (SSSIZE));				\
      if (newvs)							\
        memcpy (newvs, *(VS), (VSSIZE));				\
    }									\
  if (!newss || !newvs)							\
    {									\
      yyerror (MSG);							\
      return 2;								\
    }									\
  *(SS) = newss;							\
  *(VS) = newvs;							\
  malloced_yyss = newss;						\
  malloced_yyvs = (void *) newvs;					\
} while (0)
#define OP0(NODE) (TREE_OPERAND (NODE, 0))
#define OP1(NODE) (TREE_OPERAND (NODE, 1))

/* Contains the statement keyword (if/while/do) to include in an
   error message if the user supplies an empty conditional expression.  */
static const char *cond_stmt_keyword;

/* List of types and structure classes of the current declaration.  */
static GTY(()) tree current_declspecs;

/* List of prefix attributes in effect.
   Prefix attributes are parsed by the reserved_declspecs and declmods
   rules.  They create a list that contains *both* declspecs and attrs.  */
/* ??? It is not clear yet that all cases where an attribute can now appear in
   a declspec list have been updated.  */
static GTY(()) tree prefix_attributes;

/* When defining an enumeration, this is the type of the enumeration.  */
static GTY(()) tree current_enum_type;

/* When parsing a conversion operator name, this is the scope of the
   operator itself.  */
static GTY(()) tree saved_scopes;

static tree empty_parms PARAMS ((void));
static tree parse_decl0 PARAMS ((tree, tree, tree, tree, int));
static tree parse_decl PARAMS ((tree, tree, int));
static void parse_end_decl PARAMS ((tree, tree, tree));
static tree parse_field0 PARAMS ((tree, tree, tree, tree, tree, tree));
static tree parse_field PARAMS ((tree, tree, tree, tree));
static tree parse_bitfield0 PARAMS ((tree, tree, tree, tree, tree));
static tree parse_bitfield PARAMS ((tree, tree, tree));
static tree parse_method PARAMS ((tree, tree, tree));
static void frob_specs PARAMS ((tree, tree));
static void check_class_key PARAMS ((tree, tree));
static tree parse_scoped_id PARAMS ((tree));
static tree parse_xref_tag (tree, tree, int);
static tree parse_handle_class_head (tree, tree, tree, int, int *);
static void parse_decl_instantiation (tree, tree, tree);
static int parse_begin_function_definition (tree, tree);
static tree parse_finish_call_expr (tree, tree, int);

/* Cons up an empty parameter list.  */
static inline tree
empty_parms ()
{
  tree parms;

#ifndef NO_IMPLICIT_EXTERN_C
  if (in_system_header && current_class_type == NULL
      && current_lang_name == lang_name_c)
    parms = NULL_TREE;
  else
#endif
  parms = void_list_node;
  return parms;
}

/* Record the decl-specifiers, attributes and type lookups from the
   decl-specifier-seq in a declaration.  */

static void
frob_specs (specs_attrs, lookups)
     tree specs_attrs, lookups;
{
  save_type_access_control (lookups);
  split_specs_attrs (specs_attrs, &current_declspecs, &prefix_attributes);
  if (current_declspecs
      && TREE_CODE (current_declspecs) != TREE_LIST)
    current_declspecs = build_tree_list (NULL_TREE, current_declspecs);
  if (have_extern_spec)
    {
      /* We have to indicate that there is an "extern", but that it
         was part of a language specifier.  For instance,

    	    extern "C" typedef int (*Ptr) ();

         is well formed.  */
      current_declspecs = tree_cons (error_mark_node,
				     get_identifier ("extern"),
				     current_declspecs);
      have_extern_spec = false;
    }
}

static tree
parse_decl (declarator, attributes, initialized)
     tree declarator, attributes;
     int initialized;
{
  return start_decl (declarator, current_declspecs, initialized,
		     attributes, prefix_attributes);
}

static tree
parse_decl0 (declarator, specs_attrs, lookups, attributes, initialized)
     tree declarator, specs_attrs, lookups, attributes;
     int initialized;
{
  frob_specs (specs_attrs, lookups);
  return parse_decl (declarator, attributes, initialized);
}

static void
parse_end_decl (decl, init, asmspec)
     tree decl, init, asmspec;
{
  /* If decl is NULL_TREE, then this was a variable declaration using
     () syntax for the initializer, so we handled it in grokdeclarator.  */
  if (decl)
    decl_type_access_control (decl);
  cp_finish_decl (decl, init, asmspec, init ? LOOKUP_ONLYCONVERTING : 0);
}

static tree
parse_field (declarator, attributes, asmspec, init)
     tree declarator, attributes, asmspec, init;
{
  tree d = grokfield (declarator, current_declspecs, init, asmspec,
		      chainon (attributes, prefix_attributes));
  decl_type_access_control (d);
  return d;
}

static tree
parse_field0 (declarator, specs_attrs, lookups, attributes, asmspec, init)
     tree declarator, specs_attrs, lookups, attributes, asmspec, init;
{
  frob_specs (specs_attrs, lookups);
  return parse_field (declarator, attributes, asmspec, init);
}

static tree
parse_bitfield (declarator, attributes, width)
     tree declarator, attributes, width;
{
  tree d = grokbitfield (declarator, current_declspecs, width);
  cplus_decl_attributes (&d, chainon (attributes, prefix_attributes), 0);
  decl_type_access_control (d);
  return d;
}

static tree
parse_bitfield0 (declarator, specs_attrs, lookups, attributes, width)
     tree declarator, specs_attrs, lookups, attributes, width;
{
  frob_specs (specs_attrs, lookups);
  return parse_bitfield (declarator, attributes, width);
}

static tree
parse_method (declarator, specs_attrs, lookups)
     tree declarator, specs_attrs, lookups;
{
  tree d;
  frob_specs (specs_attrs, lookups);
  d = start_method (current_declspecs, declarator, prefix_attributes);
  decl_type_access_control (d);
  return d;
}

static void
check_class_key (key, aggr)
     tree key;
     tree aggr;
{
  if (TREE_CODE (key) == TREE_LIST)
    key = TREE_VALUE (key);
  if ((key == union_type_node) != (TREE_CODE (aggr) == UNION_TYPE))
    pedwarn ("`%s' tag used in naming `%#T'",
	     key == union_type_node ? "union"
	     : key == record_type_node ? "struct" : "class", aggr);
}



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 271 "parse.y"
typedef union YYSTYPE { GTY(())
  long itype;
  tree ttype;
  char *strtype;
  enum tree_code code;
  flagged_type_tree ftype;
  struct unparsed_text *pi;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 501 "p19357.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */
#line 478 "parse.y"

/* Tell yyparse how to print a token's value, if yydebug is set.  */
#define YYPRINT(FILE,YYCHAR,YYLVAL) yyprint(FILE,YYCHAR,YYLVAL)
extern void yyprint			PARAMS ((FILE *, int, YYSTYPE));


/* Line 214 of yacc.c.  */
#line 518 "p19357.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  4
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   13318

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  114
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  297
/* YYNRULES -- Number of rules. */
#define YYNRULES  930
/* YYNRULES -- Number of states. */
#define YYNSTATES  1823

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   344

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   112,     2,     2,     2,    85,    73,     2,
      94,   110,    83,    81,    62,    82,    93,    84,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    65,    63,
      76,    66,    77,    68,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    95,     2,   113,    72,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    61,    71,   111,    88,     2,     2,     2,
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
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    64,    67,    69,    70,
      74,    75,    78,    79,    80,    86,    87,    89,    90,    91,
      92,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short yyprhs[] =
{
       0,     0,     3,     4,     6,     7,    10,    13,    15,    16,
      17,    18,    20,    22,    23,    26,    29,    31,    32,    36,
      38,    44,    49,    55,    60,    61,    68,    69,    75,    77,
      80,    82,    85,    86,    93,    96,   100,   104,   108,   112,
     117,   118,   124,   127,   131,   133,   135,   138,   141,   143,
     146,   147,   153,   157,   159,   161,   163,   167,   169,   170,
     173,   176,   180,   182,   186,   188,   192,   194,   198,   201,
     204,   207,   209,   211,   217,   222,   225,   228,   232,   236,
     239,   242,   246,   250,   253,   256,   259,   262,   265,   268,
     270,   272,   274,   276,   277,   279,   282,   283,   285,   286,
     293,   297,   301,   305,   306,   315,   321,   322,   332,   339,
     340,   349,   355,   356,   366,   373,   376,   379,   381,   384,
     386,   393,   402,   407,   414,   421,   426,   429,   431,   434,
     437,   439,   442,   444,   447,   450,   455,   458,   459,   463,
     464,   465,   467,   471,   474,   475,   477,   479,   481,   486,
     489,   491,   493,   495,   497,   499,   501,   503,   505,   507,
     509,   511,   513,   514,   521,   522,   529,   530,   536,   537,
     543,   544,   552,   553,   561,   562,   569,   570,   577,   578,
     579,   585,   591,   593,   595,   601,   607,   608,   610,   612,
     613,   615,   617,   621,   622,   625,   627,   629,   632,   634,
     638,   640,   642,   644,   646,   648,   650,   652,   654,   658,
     660,   664,   665,   667,   669,   670,   678,   680,   682,   686,
     691,   695,   699,   703,   707,   711,   713,   715,   717,   720,
     723,   726,   729,   732,   735,   738,   743,   746,   751,   754,
     758,   762,   767,   772,   778,   784,   791,   794,   799,   805,
     808,   811,   815,   819,   823,   825,   829,   832,   836,   841,
     843,   846,   852,   854,   858,   862,   866,   870,   874,   878,
     882,   886,   890,   894,   898,   902,   906,   910,   914,   918,
     922,   926,   930,   936,   940,   944,   946,   949,   951,   955,
     959,   963,   967,   971,   975,   979,   983,   987,   991,   995,
     999,  1003,  1007,  1011,  1015,  1019,  1023,  1029,  1033,  1037,
    1039,  1042,  1046,  1050,  1052,  1054,  1056,  1058,  1060,  1061,
    1067,  1073,  1079,  1085,  1091,  1093,  1095,  1097,  1099,  1102,
    1104,  1107,  1110,  1114,  1119,  1124,  1126,  1128,  1130,  1134,
    1136,  1138,  1140,  1142,  1144,  1148,  1152,  1156,  1157,  1162,
    1167,  1170,  1175,  1178,  1185,  1190,  1193,  1196,  1198,  1203,
    1205,  1213,  1221,  1229,  1237,  1242,  1247,  1250,  1253,  1256,
    1258,  1263,  1266,  1269,  1275,  1279,  1282,  1285,  1291,  1295,
    1301,  1305,  1310,  1317,  1320,  1322,  1325,  1327,  1330,  1332,
    1334,  1335,  1338,  1341,  1345,  1349,  1353,  1356,  1359,  1362,
    1364,  1366,  1368,  1371,  1374,  1377,  1380,  1382,  1384,  1386,
    1388,  1391,  1394,  1398,  1402,  1406,  1411,  1413,  1416,  1419,
    1421,  1423,  1426,  1429,  1432,  1434,  1437,  1440,  1444,  1446,
    1449,  1452,  1454,  1456,  1458,  1460,  1462,  1464,  1466,  1471,
    1476,  1481,  1486,  1488,  1490,  1492,  1494,  1498,  1500,  1504,
    1506,  1510,  1511,  1516,  1517,  1524,  1528,  1529,  1534,  1536,
    1540,  1544,  1545,  1550,  1554,  1555,  1557,  1559,  1562,  1569,
    1571,  1575,  1576,  1578,  1583,  1590,  1595,  1597,  1599,  1601,
    1603,  1605,  1609,  1610,  1613,  1615,  1618,  1622,  1627,  1629,
    1631,  1635,  1640,  1644,  1650,  1654,  1658,  1662,  1663,  1667,
    1671,  1675,  1676,  1679,  1682,  1683,  1690,  1691,  1697,  1700,
    1703,  1706,  1707,  1708,  1709,  1721,  1723,  1724,  1726,  1727,
    1729,  1731,  1734,  1737,  1740,  1743,  1746,  1749,  1753,  1758,
    1762,  1765,  1769,  1774,  1776,  1779,  1781,  1784,  1787,  1790,
    1793,  1797,  1801,  1804,  1805,  1808,  1812,  1814,  1819,  1821,
    1825,  1827,  1829,  1832,  1835,  1839,  1843,  1844,  1846,  1850,
    1853,  1856,  1858,  1861,  1864,  1867,  1870,  1873,  1876,  1879,
    1881,  1884,  1887,  1891,  1893,  1896,  1899,  1904,  1909,  1912,
    1914,  1920,  1925,  1927,  1928,  1930,  1934,  1935,  1937,  1941,
    1943,  1945,  1947,  1949,  1954,  1959,  1964,  1969,  1974,  1978,
    1983,  1988,  1993,  1998,  2002,  2005,  2007,  2009,  2013,  2015,
    2019,  2022,  2024,  2031,  2032,  2035,  2037,  2040,  2042,  2045,
    2049,  2053,  2055,  2059,  2061,  2064,  2068,  2072,  2075,  2078,
    2082,  2084,  2089,  2094,  2098,  2102,  2105,  2107,  2109,  2112,
    2114,  2116,  2119,  2122,  2124,  2127,  2131,  2135,  2138,  2141,
    2145,  2147,  2151,  2155,  2158,  2161,  2165,  2167,  2172,  2176,
    2181,  2185,  2187,  2190,  2193,  2196,  2199,  2202,  2205,  2208,
    2210,  2213,  2218,  2223,  2226,  2228,  2230,  2232,  2234,  2237,
    2242,  2246,  2250,  2253,  2256,  2259,  2262,  2264,  2267,  2270,
    2273,  2276,  2280,  2282,  2285,  2289,  2294,  2297,  2300,  2303,
    2306,  2309,  2312,  2317,  2320,  2322,  2325,  2328,  2332,  2334,
    2338,  2341,  2345,  2348,  2351,  2355,  2357,  2361,  2366,  2368,
    2371,  2375,  2378,  2381,  2383,  2387,  2390,  2393,  2395,  2398,
    2402,  2404,  2408,  2415,  2420,  2425,  2429,  2435,  2439,  2443,
    2447,  2450,  2452,  2454,  2457,  2460,  2463,  2464,  2466,  2468,
    2471,  2475,  2476,  2481,  2483,  2484,  2485,  2491,  2493,  2494,
    2498,  2500,  2503,  2505,  2508,  2509,  2514,  2516,  2517,  2518,
    2524,  2525,  2526,  2534,  2535,  2536,  2537,  2538,  2551,  2552,
    2553,  2561,  2562,  2568,  2569,  2577,  2578,  2583,  2586,  2589,
    2592,  2596,  2603,  2612,  2623,  2632,  2645,  2656,  2667,  2672,
    2676,  2679,  2682,  2684,  2686,  2688,  2690,  2692,  2693,  2694,
    2700,  2701,  2702,  2708,  2710,  2713,  2714,  2715,  2716,  2722,
    2724,  2726,  2730,  2734,  2737,  2740,  2743,  2746,  2749,  2751,
    2754,  2755,  2757,  2758,  2760,  2762,  2763,  2765,  2767,  2771,
    2776,  2784,  2786,  2790,  2791,  2793,  2795,  2797,  2800,  2803,
    2806,  2808,  2811,  2814,  2815,  2819,  2821,  2823,  2825,  2828,
    2831,  2834,  2839,  2842,  2845,  2848,  2851,  2854,  2857,  2859,
    2862,  2864,  2867,  2869,  2871,  2872,  2873,  2875,  2881,  2885,
    2886,  2890,  2891,  2892,  2897,  2900,  2902,  2904,  2906,  2910,
    2911,  2915,  2919,  2923,  2925,  2926,  2930,  2934,  2938,  2942,
    2946,  2950,  2954,  2958,  2962,  2966,  2970,  2974,  2978,  2982,
    2986,  2990,  2994,  2998,  3002,  3006,  3010,  3014,  3018,  3023,
    3027,  3031,  3035,  3039,  3044,  3048,  3052,  3058,  3064,  3069,
    3073
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short yyrhs[] =
{
     115,     0,    -1,    -1,   116,    -1,    -1,   117,   123,    -1,
     116,   123,    -1,   116,    -1,    -1,    -1,    -1,    33,    -1,
      28,    -1,    -1,   124,   125,    -1,   156,   153,    -1,   150,
      -1,    -1,    57,   126,   147,    -1,   147,    -1,   122,    94,
      12,   110,    63,    -1,   137,    61,   118,   111,    -1,   137,
     119,   156,   120,   153,    -1,   137,   119,   150,   120,    -1,
      -1,    46,   173,    61,   127,   118,   111,    -1,    -1,    46,
      61,   128,   118,   111,    -1,   129,    -1,   131,    63,    -1,
     133,    -1,   121,   125,    -1,    -1,    46,   173,    66,   130,
     136,    63,    -1,    48,   319,    -1,    48,   333,   319,    -1,
      48,   333,   218,    -1,    48,   135,   173,    -1,    48,   333,
     173,    -1,    48,   333,   135,   173,    -1,    -1,    48,    46,
     134,   136,    63,    -1,    59,    56,    -1,   135,    59,    56,
      -1,   218,    -1,   319,    -1,   333,   319,    -1,   333,   218,
      -1,    99,    -1,   137,    99,    -1,    -1,    50,    76,   139,
     142,    77,    -1,    50,    76,    77,    -1,   138,    -1,   140,
      -1,   146,    -1,   142,    62,   146,    -1,   173,    -1,    -1,
     279,   143,    -1,    47,   143,    -1,   138,   279,   143,    -1,
     144,    -1,   144,    66,   233,    -1,   397,    -1,   397,    66,
     213,    -1,   145,    -1,   145,    66,   194,    -1,   141,   148,
      -1,   141,     1,    -1,   156,   153,    -1,   149,    -1,   147,
      -1,   137,   119,   156,   120,   153,    -1,   137,   119,   149,
     120,    -1,   121,   148,    -1,   247,    63,    -1,   237,   246,
      63,    -1,   234,   245,    63,    -1,   271,    63,    -1,   247,
      63,    -1,   237,   246,    63,    -1,   234,   245,    63,    -1,
     237,    63,    -1,   176,    63,    -1,   234,    63,    -1,     1,
      63,    -1,     1,   111,    -1,     1,   109,    -1,    63,    -1,
     400,    -1,   228,    -1,   167,    -1,    -1,   166,    -1,   166,
      63,    -1,    -1,   109,    -1,    -1,   169,   151,   410,    61,
     155,   204,    -1,   162,   152,   154,    -1,   162,   152,   368,
      -1,   162,   152,     1,    -1,    -1,   324,     5,    94,   158,
     388,   110,   306,   403,    -1,   324,     5,    49,   306,   403,
      -1,    -1,   333,   324,     5,    94,   159,   388,   110,   306,
     403,    -1,   333,   324,     5,    49,   306,   403,    -1,    -1,
     324,   189,    94,   160,   388,   110,   306,   403,    -1,   324,
     189,    49,   306,   403,    -1,    -1,   333,   324,   189,    94,
     161,   388,   110,   306,   403,    -1,   333,   324,   189,    49,
     306,   403,    -1,   234,   231,    -1,   237,   316,    -1,   316,
      -1,   237,   157,    -1,   157,    -1,     5,    94,   388,   110,
     306,   403,    -1,    94,     5,   110,    94,   388,   110,   306,
     403,    -1,     5,    49,   306,   403,    -1,    94,     5,   110,
      49,   306,   403,    -1,   189,    94,   388,   110,   306,   403,
      -1,   189,    49,   306,   403,    -1,   237,   163,    -1,   163,
      -1,   234,   231,    -1,   237,   316,    -1,   316,    -1,   237,
     157,    -1,   157,    -1,    26,     3,    -1,   165,   264,    -1,
     165,    94,   206,   110,    -1,   165,    49,    -1,    -1,    65,
     168,   170,    -1,    -1,    -1,   172,    -1,   170,    62,   172,
      -1,   170,     1,    -1,    -1,   174,    -1,   312,    -1,   326,
      -1,   171,    94,   206,   110,    -1,   171,    49,    -1,     1,
      -1,     3,    -1,     4,    -1,     5,    -1,    60,    -1,    59,
      -1,     3,    -1,    60,    -1,    59,    -1,   106,    -1,   105,
      -1,   107,    -1,    -1,    50,   185,   243,    63,   177,   186,
      -1,    -1,    50,   185,   234,   231,   178,   186,    -1,    -1,
      50,   185,   316,   179,   186,    -1,    -1,    50,   185,   157,
     180,   186,    -1,    -1,     7,    50,   185,   243,    63,   181,
     186,    -1,    -1,     7,    50,   185,   234,   231,   182,   186,
      -1,    -1,     7,    50,   185,   316,   183,   186,    -1,    -1,
       7,    50,   185,   157,   184,   186,    -1,    -1,    -1,    60,
      76,   192,   191,   190,    -1,     4,    76,   192,   191,   190,
      -1,   189,    -1,   187,    -1,   173,    76,   192,    77,   190,
      -1,     5,    76,   192,   191,   190,    -1,    -1,    77,    -1,
      79,    -1,    -1,   193,    -1,   194,    -1,   193,    62,   194,
      -1,    -1,   195,   196,    -1,   233,    -1,    60,    -1,   333,
      60,    -1,   213,    -1,   324,    50,   173,    -1,    82,    -1,
      81,    -1,    90,    -1,    89,    -1,   112,    -1,   205,    -1,
     212,    -1,    49,    -1,    94,   198,   110,    -1,    49,    -1,
      94,   202,   110,    -1,    -1,   202,    -1,     1,    -1,    -1,
     378,   231,   248,   257,    66,   203,   265,    -1,   198,    -1,
     111,    -1,   341,   339,   111,    -1,   341,   339,     1,   111,
      -1,   341,     1,   111,    -1,   212,    62,   212,    -1,   212,
      62,     1,    -1,   205,    62,   212,    -1,   205,    62,     1,
      -1,   212,    -1,   205,    -1,   223,    -1,   121,   211,    -1,
      83,   211,    -1,    73,   211,    -1,    88,   211,    -1,   197,
     211,    -1,    70,   173,    -1,   240,   207,    -1,   240,    94,
     233,   110,    -1,   241,   207,    -1,   241,    94,   233,   110,
      -1,   225,   305,    -1,   225,   305,   209,    -1,   225,   208,
     305,    -1,   225,   208,   305,   209,    -1,   225,    94,   233,
     110,    -1,   225,    94,   233,   110,   209,    -1,   225,   208,
      94,   233,   110,    -1,   225,   208,    94,   233,   110,   209,
      -1,   226,   211,    -1,   226,    95,   113,   211,    -1,   226,
      95,   198,   113,   211,    -1,    35,   211,    -1,    36,   211,
      -1,    94,   206,   110,    -1,    61,   206,   111,    -1,    94,
     206,   110,    -1,    49,    -1,    94,   243,   110,    -1,    66,
     265,    -1,    94,   233,   110,    -1,   210,    94,   233,   110,
      -1,   207,    -1,   210,   207,    -1,   210,    61,   266,   277,
     111,    -1,   211,    -1,   212,    87,   212,    -1,   212,    86,
     212,    -1,   212,    81,   212,    -1,   212,    82,   212,    -1,
     212,    83,   212,    -1,   212,    84,   212,    -1,   212,    85,
     212,    -1,   212,    80,   212,    -1,   212,    79,   212,    -1,
     212,    78,   212,    -1,   212,    76,   212,    -1,   212,    77,
     212,    -1,   212,    75,   212,    -1,   212,    74,   212,    -1,
     212,    73,   212,    -1,   212,    71,   212,    -1,   212,    72,
     212,    -1,   212,    70,   212,    -1,   212,    69,   212,    -1,
     212,    68,   383,    65,   212,    -1,   212,    66,   212,    -1,
     212,    67,   212,    -1,    64,    -1,    64,   212,    -1,   211,
      -1,   213,    87,   213,    -1,   213,    86,   213,    -1,   213,
      81,   213,    -1,   213,    82,   213,    -1,   213,    83,   213,
      -1,   213,    84,   213,    -1,   213,    85,   213,    -1,   213,
      80,   213,    -1,   213,    79,   213,    -1,   213,    78,   213,
      -1,   213,    76,   213,    -1,   213,    75,   213,    -1,   213,
      74,   213,    -1,   213,    73,   213,    -1,   213,    71,   213,
      -1,   213,    72,   213,    -1,   213,    70,   213,    -1,   213,
      69,   213,    -1,   213,    68,   383,    65,   213,    -1,   213,
      66,   213,    -1,   213,    67,   213,    -1,    64,    -1,    64,
     213,    -1,    88,   398,   173,    -1,    88,   398,   187,    -1,
     216,    -1,   409,    -1,     3,    -1,    60,    -1,    59,    -1,
      -1,     6,    76,   215,   192,   191,    -1,   409,    76,   215,
     192,   191,    -1,    50,   173,    76,   192,   191,    -1,    50,
       6,    76,   192,   191,    -1,    50,   409,    76,   192,   191,
      -1,   214,    -1,     4,    -1,     5,    -1,   220,    -1,   258,
     220,    -1,   214,    -1,    83,   219,    -1,    73,   219,    -1,
      94,   219,   110,    -1,     3,    76,   192,   191,    -1,    59,
      76,   193,   191,    -1,   318,    -1,   214,    -1,   221,    -1,
      94,   219,   110,    -1,   214,    -1,    10,    -1,   227,    -1,
      12,    -1,    11,    -1,    94,   198,   110,    -1,    94,   219,
     110,    -1,    94,     1,   110,    -1,    -1,    94,   224,   344,
     110,    -1,   214,    94,   206,   110,    -1,   214,    49,    -1,
     223,    94,   206,   110,    -1,   223,    49,    -1,    37,    94,
     212,    62,   233,   110,    -1,   223,    95,   198,   113,    -1,
     223,    90,    -1,   223,    89,    -1,    42,    -1,     9,    94,
     206,   110,    -1,   322,    -1,    52,    76,   233,    77,    94,
     198,   110,    -1,    53,    76,   233,    77,    94,   198,   110,
      -1,    54,    76,   233,    77,    94,   198,   110,    -1,    55,
      76,   233,    77,    94,   198,   110,    -1,    51,    94,   198,
     110,    -1,    51,    94,   233,   110,    -1,   333,     3,    -1,
     333,   216,    -1,   333,   409,    -1,   321,    -1,   321,    94,
     206,   110,    -1,   321,    49,    -1,   229,   217,    -1,   229,
     217,    94,   206,   110,    -1,   229,   217,    49,    -1,   229,
     218,    -1,   229,   321,    -1,   229,   218,    94,   206,   110,
      -1,   229,   218,    49,    -1,   229,   321,    94,   206,   110,
      -1,   229,   321,    49,    -1,   229,    88,     8,    49,    -1,
     229,     8,    56,    88,     8,    49,    -1,   229,     1,    -1,
      41,    -1,   333,    41,    -1,    40,    -1,   333,   226,    -1,
      44,    -1,    45,    -1,    -1,   223,    93,    -1,   223,    96,
      -1,   243,   245,    63,    -1,   234,   245,    63,    -1,   237,
     246,    63,    -1,   234,    63,    -1,   237,    63,    -1,   121,
     230,    -1,   310,    -1,   316,    -1,    49,    -1,   232,    49,
      -1,   238,   337,    -1,   307,   337,    -1,   243,   337,    -1,
     238,    -1,   307,    -1,   238,    -1,   235,    -1,   237,   243,
      -1,   243,   236,    -1,   243,   239,   236,    -1,   237,   243,
     236,    -1,   237,   243,   239,    -1,   237,   243,   239,   236,
      -1,     7,    -1,   236,   244,    -1,   236,     7,    -1,   307,
      -1,     7,    -1,   237,     9,    -1,   237,     7,    -1,   237,
     258,    -1,   243,    -1,   307,   243,    -1,   243,   239,    -1,
     307,   243,   239,    -1,   244,    -1,   239,   244,    -1,   239,
     258,    -1,   258,    -1,    14,    -1,    30,    -1,    29,    -1,
     271,    -1,     8,    -1,   313,    -1,   242,    94,   198,   110,
      -1,   242,    94,   233,   110,    -1,    31,    94,   198,   110,
      -1,    31,    94,   233,   110,    -1,     8,    -1,     9,    -1,
     271,    -1,   253,    -1,   245,    62,   249,    -1,   254,    -1,
     246,    62,   249,    -1,   255,    -1,   247,    62,   249,    -1,
      -1,   122,    94,    12,   110,    -1,    -1,   231,   248,   257,
      66,   250,   265,    -1,   231,   248,   257,    -1,    -1,   257,
      66,   252,   265,    -1,   257,    -1,   231,   248,   251,    -1,
     316,   248,   251,    -1,    -1,   316,   248,   256,   251,    -1,
     157,   248,   257,    -1,    -1,   258,    -1,   259,    -1,   258,
     259,    -1,    32,    94,    94,   260,   110,   110,    -1,   261,
      -1,   260,    62,   261,    -1,    -1,   262,    -1,   262,    94,
       3,   110,    -1,   262,    94,     3,    62,   206,   110,    -1,
     262,    94,   206,   110,    -1,   173,    -1,     7,    -1,     8,
      -1,     9,    -1,   173,    -1,   263,    62,   173,    -1,    -1,
      66,   265,    -1,   212,    -1,    61,   111,    -1,    61,   266,
     111,    -1,    61,   266,    62,   111,    -1,     1,    -1,   265,
      -1,   266,    62,   265,    -1,    95,   212,   113,   265,    -1,
     173,    65,   265,    -1,   266,    62,   173,    65,   265,    -1,
     104,   152,   154,    -1,   104,   152,   368,    -1,   104,   152,
       1,    -1,    -1,   268,   267,   153,    -1,   103,   212,   109,
      -1,   103,     1,   109,    -1,    -1,   270,   269,    -1,   270,
       1,    -1,    -1,    15,   173,    61,   272,   302,   111,    -1,
      -1,    15,    61,   273,   302,   111,    -1,    15,   173,    -1,
      15,   331,    -1,    47,   326,    -1,    -1,    -1,    -1,   283,
     284,    61,   274,   289,   111,   257,   275,   270,   276,   268,
      -1,   282,    -1,    -1,    62,    -1,    -1,    62,    -1,    38,
      -1,   279,     7,    -1,   279,     8,    -1,   279,     9,    -1,
     279,    38,    -1,   279,   258,    -1,   279,   173,    -1,   279,
     324,   173,    -1,   279,   333,   324,   173,    -1,   279,   333,
     173,    -1,   279,   188,    -1,   279,   324,   188,    -1,   279,
     333,   324,   188,    -1,   280,    -1,   279,   175,    -1,   281,
      -1,   280,    61,    -1,   280,    65,    -1,   281,    61,    -1,
     281,    65,    -1,   279,   175,    61,    -1,   279,   175,    65,
      -1,   279,    61,    -1,    -1,    65,   398,    -1,    65,   398,
     285,    -1,   286,    -1,   285,    62,   398,   286,    -1,   287,
      -1,   288,   398,   287,    -1,   326,    -1,   312,    -1,    39,
     398,    -1,     7,   398,    -1,   288,    39,   398,    -1,   288,
       7,   398,    -1,    -1,   291,    -1,   289,   290,   291,    -1,
     289,   290,    -1,    39,    65,    -1,   292,    -1,   291,   292,
      -1,   293,    63,    -1,   293,   111,    -1,   164,    65,    -1,
     164,    98,    -1,   164,    26,    -1,   164,    61,    -1,    63,
      -1,   121,   292,    -1,   141,   292,    -1,   141,   234,    63,
      -1,   400,    -1,   234,   294,    -1,   237,   295,    -1,   316,
     248,   257,   264,    -1,   157,   248,   257,   264,    -1,    65,
     212,    -1,     1,    -1,   237,   163,   248,   257,   264,    -1,
     163,   248,   257,   264,    -1,   131,    -1,    -1,   296,    -1,
     294,    62,   297,    -1,    -1,   299,    -1,   295,    62,   301,
      -1,   298,    -1,   299,    -1,   300,    -1,   301,    -1,   310,
     248,   257,   264,    -1,     4,    65,   212,   257,    -1,   316,
     248,   257,   264,    -1,   157,   248,   257,   264,    -1,     3,
      65,   212,   257,    -1,    65,   212,   257,    -1,   310,   248,
     257,   264,    -1,     4,    65,   212,   257,    -1,   316,   248,
     257,   264,    -1,     3,    65,   212,   257,    -1,    65,   212,
     257,    -1,   303,   278,    -1,   278,    -1,   304,    -1,   303,
      62,   304,    -1,   173,    -1,   173,    66,   212,    -1,   378,
     334,    -1,   378,    -1,    94,   233,   110,    95,   198,   113,
      -1,    -1,   306,     9,    -1,     9,    -1,   307,     9,    -1,
     258,    -1,   307,   258,    -1,    94,   206,   110,    -1,    94,
     388,   110,    -1,    49,    -1,    94,     1,   110,    -1,   310,
      -1,   258,   310,    -1,    83,   307,   309,    -1,    73,   307,
     309,    -1,    83,   309,    -1,    73,   309,    -1,   332,   306,
     309,    -1,   311,    -1,   311,   308,   306,   403,    -1,   311,
      95,   198,   113,    -1,   311,    95,   113,    -1,    94,   309,
     110,    -1,   324,   323,    -1,   323,    -1,   323,    -1,   333,
     323,    -1,   312,    -1,   314,    -1,   333,   314,    -1,   324,
     323,    -1,   316,    -1,   258,   316,    -1,    83,   307,   315,
      -1,    73,   307,   315,    -1,    83,   315,    -1,    73,   315,
      -1,   332,   306,   315,    -1,   222,    -1,    83,   307,   315,
      -1,    73,   307,   315,    -1,    83,   317,    -1,    73,   317,
      -1,   332,   306,   315,    -1,   318,    -1,   222,   308,   306,
     403,    -1,    94,   317,   110,    -1,   222,    95,   198,   113,
      -1,   222,    95,   113,    -1,   320,    -1,   333,   320,    -1,
     333,   214,    -1,   324,   221,    -1,   324,   218,    -1,   324,
     217,    -1,   324,   214,    -1,   324,   217,    -1,   320,    -1,
     333,   320,    -1,   243,    94,   206,   110,    -1,   243,    94,
     219,   110,    -1,   243,   232,    -1,     4,    -1,     5,    -1,
     187,    -1,   325,    -1,   324,   325,    -1,   324,    50,   330,
      56,    -1,   324,     3,    56,    -1,   324,    60,    56,    -1,
       4,    56,    -1,     5,    56,    -1,    59,    56,    -1,   187,
      56,    -1,   327,    -1,   333,   327,    -1,   328,   173,    -1,
     328,   187,    -1,   328,   330,    -1,   328,    50,   330,    -1,
     329,    -1,   328,   329,    -1,   328,   330,    56,    -1,   328,
      50,   330,    56,    -1,     4,    56,    -1,     5,    56,    -1,
     187,    56,    -1,    60,    56,    -1,     3,    56,    -1,    59,
      56,    -1,   173,    76,   192,   191,    -1,   333,   323,    -1,
     314,    -1,   333,   314,    -1,   324,    83,    -1,   333,   324,
      83,    -1,    56,    -1,    83,   306,   334,    -1,    83,   306,
      -1,    73,   306,   334,    -1,    73,   306,    -1,   332,   306,
      -1,   332,   306,   334,    -1,   335,    -1,    95,   198,   113,
      -1,   335,    95,   198,   113,    -1,   337,    -1,   258,   337,
      -1,    83,   307,   336,    -1,    83,   336,    -1,    83,   307,
      -1,    83,    -1,    73,   307,   336,    -1,    73,   336,    -1,
      73,   307,    -1,    73,    -1,   332,   306,    -1,   332,   306,
     336,    -1,   338,    -1,    94,   336,   110,    -1,   338,    94,
     388,   110,   306,   403,    -1,   338,    49,   306,   403,    -1,
     338,    95,   198,   113,    -1,   338,    95,   113,    -1,    94,
     389,   110,   306,   403,    -1,   210,   306,   403,    -1,   232,
     306,   403,    -1,    95,   198,   113,    -1,    95,   113,    -1,
     352,    -1,   340,    -1,   339,   352,    -1,   339,   340,    -1,
       1,    63,    -1,    -1,   342,    -1,   343,    -1,   342,   343,
      -1,    34,   263,    63,    -1,    -1,   410,    61,   345,   204,
      -1,   344,    -1,    -1,    -1,    16,   348,   200,   349,   350,
      -1,   346,    -1,    -1,   351,   410,   353,    -1,   346,    -1,
     410,   353,    -1,   230,    -1,   198,    63,    -1,    -1,   347,
      17,   354,   350,    -1,   347,    -1,    -1,    -1,    18,   355,
     200,   356,   350,    -1,    -1,    -1,    19,   357,   350,    18,
     358,   199,    63,    -1,    -1,    -1,    -1,    -1,    20,   359,
      94,   381,   360,   201,    63,   361,   383,   110,   362,   350,
      -1,    -1,    -1,    21,   363,    94,   202,   110,   364,   350,
      -1,    -1,    22,   212,    65,   365,   352,    -1,    -1,    22,
     212,    13,   212,    65,   366,   352,    -1,    -1,    23,    65,
     367,   352,    -1,    24,    63,    -1,    25,    63,    -1,    26,
      63,    -1,    26,   198,    63,    -1,   122,   382,    94,    12,
     110,    63,    -1,   122,   382,    94,    12,    65,   384,   110,
      63,    -1,   122,   382,    94,    12,    65,   384,    65,   384,
     110,    63,    -1,   122,   382,    94,    12,    56,   384,   110,
      63,    -1,   122,   382,    94,    12,    65,   384,    65,   384,
      65,   387,   110,    63,    -1,   122,   382,    94,    12,    56,
     384,    65,   387,   110,    63,    -1,   122,   382,    94,    12,
      65,   384,    56,   387,   110,    63,    -1,    27,    83,   198,
      63,    -1,    27,   173,    63,    -1,   380,   352,    -1,   380,
     111,    -1,    63,    -1,   371,    -1,   133,    -1,   132,    -1,
     129,    -1,    -1,    -1,    98,   369,   154,   370,   374,    -1,
      -1,    -1,    98,   372,   346,   373,   374,    -1,   375,    -1,
     374,   375,    -1,    -1,    -1,    -1,    97,   376,   379,   377,
     346,    -1,   238,    -1,   307,    -1,    94,    13,   110,    -1,
      94,   397,   110,    -1,     3,    65,    -1,    60,    65,    -1,
       4,    65,    -1,     5,    65,    -1,   383,    63,    -1,   230,
      -1,    61,   204,    -1,    -1,     9,    -1,    -1,   198,    -1,
       1,    -1,    -1,   385,    -1,   386,    -1,   385,    62,   386,
      -1,    12,    94,   198,   110,    -1,    95,   173,   113,    12,
      94,   198,   110,    -1,    12,    -1,   387,    62,    12,    -1,
      -1,   389,    -1,   233,    -1,   393,    -1,   394,    13,    -1,
     393,    13,    -1,   233,    13,    -1,    13,    -1,   393,    65,
      -1,   233,    65,    -1,    -1,    66,   391,   392,    -1,   102,
      -1,   265,    -1,   395,    -1,   397,   390,    -1,   394,   396,
      -1,   394,   399,    -1,   394,   399,    66,   265,    -1,   393,
      62,    -1,   233,    62,    -1,   235,   231,    -1,   238,   231,
      -1,   243,   231,    -1,   235,   337,    -1,   235,    -1,   237,
     316,    -1,   397,    -1,   397,   390,    -1,   395,    -1,   233,
      -1,    -1,    -1,   316,    -1,     3,   401,     3,   402,    63,
      -1,    76,   192,   191,    -1,    -1,    94,   206,   110,    -1,
      -1,    -1,    64,    94,   405,   110,    -1,    64,    49,    -1,
     233,    -1,     1,    -1,   404,    -1,   405,    62,   404,    -1,
      -1,    83,   306,   406,    -1,    73,   306,   406,    -1,   332,
     306,   406,    -1,    43,    -1,    -1,   407,    83,   408,    -1,
     407,    84,   408,    -1,   407,    85,   408,    -1,   407,    81,
     408,    -1,   407,    82,   408,    -1,   407,    73,   408,    -1,
     407,    71,   408,    -1,   407,    72,   408,    -1,   407,    88,
     408,    -1,   407,    62,   408,    -1,   407,    78,   408,    -1,
     407,    76,   408,    -1,   407,    77,   408,    -1,   407,    75,
     408,    -1,   407,    67,   408,    -1,   407,    66,   408,    -1,
     407,    80,   408,    -1,   407,    79,   408,    -1,   407,    90,
     408,    -1,   407,    89,   408,    -1,   407,    70,   408,    -1,
     407,    69,   408,    -1,   407,   112,   408,    -1,   407,    68,
      65,   408,    -1,   407,    74,   408,    -1,   407,    96,   408,
      -1,   407,    87,   408,    -1,   407,    49,   408,    -1,   407,
      95,   113,   408,    -1,   407,    41,   408,    -1,   407,    40,
     408,    -1,   407,    41,    95,   113,   408,    -1,   407,    40,
      95,   113,   408,    -1,   407,   378,   406,   408,    -1,   407,
       1,   408,    -1,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   487,   487,   488,   497,   497,   500,   505,   506,   510,
     514,   518,   524,   528,   528,   536,   538,   542,   541,   545,
     547,   549,   551,   553,   556,   555,   560,   559,   563,   564,
     566,   567,   573,   572,   584,   586,   588,   593,   595,   597,
     603,   602,   617,   623,   633,   634,   635,   637,   642,   644,
     652,   651,   658,   664,   665,   669,   671,   676,   679,   683,
     685,   690,   702,   704,   706,   708,   710,   712,   720,   722,
     727,   729,   731,   733,   736,   739,   744,   745,   747,   749,
     760,   761,   763,   765,   767,   768,   775,   776,   777,   779,
     780,   784,   785,   788,   790,   791,   794,   796,   804,   803,
     814,   816,   818,   824,   823,   827,   832,   831,   835,   840,
     839,   843,   848,   847,   851,   858,   862,   865,   868,   871,
     880,   882,   885,   887,   889,   891,   898,   906,   909,   911,
     913,   916,   918,   924,   931,   933,   935,   940,   940,   950,
     957,   961,   966,   977,   982,   988,   991,   994,  1000,  1003,
    1006,  1012,  1013,  1014,  1015,  1016,  1020,  1021,  1022,  1026,
    1027,  1028,  1033,  1032,  1037,  1036,  1041,  1040,  1044,  1043,
    1047,  1046,  1053,  1051,  1058,  1057,  1062,  1061,  1068,  1072,
    1080,  1083,  1086,  1090,  1091,  1097,  1103,  1113,  1114,  1124,
    1125,  1129,  1131,  1136,  1136,  1145,  1147,  1153,  1159,  1160,
    1173,  1175,  1177,  1179,  1181,  1186,  1188,  1192,  1196,  1201,
    1205,  1211,  1212,  1213,  1219,  1218,  1240,  1244,  1245,  1246,
    1247,  1251,  1254,  1257,  1259,  1264,  1266,  1270,  1273,  1276,
    1278,  1280,  1282,  1285,  1287,  1290,  1294,  1297,  1304,  1307,
    1310,  1313,  1316,  1321,  1324,  1327,  1331,  1333,  1337,  1341,
    1343,  1348,  1350,  1356,  1358,  1360,  1365,  1376,  1380,  1387,
    1388,  1390,  1404,  1406,  1408,  1410,  1412,  1414,  1416,  1418,
    1420,  1422,  1424,  1426,  1428,  1430,  1432,  1434,  1436,  1438,
    1440,  1442,  1444,  1446,  1450,  1452,  1454,  1459,  1461,  1463,
    1465,  1467,  1469,  1471,  1473,  1475,  1477,  1479,  1481,  1483,
    1485,  1487,  1489,  1491,  1493,  1495,  1497,  1499,  1503,  1505,
    1507,  1512,  1514,  1516,  1517,  1518,  1519,  1520,  1524,  1537,
    1544,  1554,  1556,  1558,  1564,  1565,  1566,  1570,  1571,  1580,
    1581,  1583,  1585,  1590,  1592,  1597,  1600,  1601,  1602,  1607,
    1614,  1615,  1616,  1626,  1628,  1630,  1633,  1636,  1635,  1650,
    1652,  1654,  1656,  1658,  1661,  1663,  1665,  1668,  1670,  1681,
    1682,  1686,  1690,  1694,  1698,  1700,  1704,  1706,  1708,  1716,
    1725,  1727,  1729,  1731,  1733,  1735,  1737,  1739,  1741,  1743,
    1745,  1748,  1750,  1752,  1797,  1799,  1804,  1806,  1811,  1813,
    1819,  1826,  1828,  1836,  1841,  1845,  1847,  1852,  1854,  1862,
    1863,  1868,  1871,  1878,  1881,  1884,  1888,  1891,  1902,  1904,
    1909,  1912,  1915,  1918,  1921,  1924,  1931,  1936,  1938,  1960,
    1962,  1967,  1972,  1980,  1991,  1994,  1997,  2000,  2006,  2008,
    2010,  2012,  2017,  2021,  2025,  2033,  2035,  2037,  2039,  2043,
    2047,  2062,  2082,  2084,  2086,  2090,  2091,  2096,  2097,  2102,
    2103,  2109,  2110,  2116,  2115,  2120,  2135,  2134,  2141,  2148,
    2153,  2159,  2158,  2166,  2175,  2176,  2181,  2183,  2188,  2193,
    2195,  2201,  2202,  2204,  2206,  2208,  2216,  2217,  2218,  2219,
    2224,  2226,  2231,  2233,  2241,  2242,  2245,  2248,  2251,  2258,
    2260,  2263,  2265,  2267,  2272,  2277,  2282,  2288,  2290,  2296,
    2298,  2303,  2304,  2306,  2312,  2311,  2321,  2320,  2329,  2332,
    2335,  2342,  2367,  2385,  2341,  2394,  2402,  2404,  2407,  2409,
    2415,  2416,  2418,  2420,  2422,  2424,  2429,  2434,  2439,  2444,
    2452,  2457,  2462,  2470,  2477,  2483,  2491,  2500,  2508,  2514,
    2520,  2528,  2536,  2551,  2552,  2555,  2560,  2561,  2566,  2568,
    2573,  2576,  2581,  2582,  2586,  2597,  2611,  2612,  2613,  2614,
    2618,  2627,  2633,  2642,  2643,  2648,  2650,  2652,  2654,  2656,
    2658,  2661,  2671,  2676,  2684,  2705,  2711,  2713,  2715,  2717,
    2728,  2733,  2735,  2743,  2744,  2751,  2763,  2764,  2771,  2782,
    2783,  2787,  2788,  2792,  2795,  2801,  2804,  2807,  2810,  2816,
    2818,  2823,  2825,  2827,  2832,  2833,  2841,  2842,  2846,  2848,
    2854,  2857,  2862,  2873,  2875,  2880,  2883,  2886,  2889,  2899,
    2901,  2903,  2905,  2912,  2913,  2923,  2925,  2927,  2929,  2931,
    2935,  2939,  2941,  2943,  2945,  2947,  2951,  2955,  2965,  2976,
    2977,  2978,  2983,  2991,  2992,  3001,  3003,  3005,  3007,  3009,
    3013,  3017,  3019,  3021,  3023,  3025,  3029,  3033,  3035,  3037,
    3039,  3041,  3043,  3045,  3049,  3057,  3060,  3066,  3069,  3075,
    3076,  3081,  3083,  3085,  3090,  3091,  3092,  3096,  3097,  3099,
    3103,  3106,  3114,  3124,  3130,  3136,  3141,  3142,  3147,  3160,
    3162,  3164,  3169,  3176,  3189,  3192,  3200,  3212,  3218,  3220,
    3221,  3222,  3231,  3236,  3244,  3245,  3250,  3252,  3259,  3265,
    3267,  3269,  3271,  3273,  3277,  3281,  3286,  3288,  3293,  3294,
    3304,  3306,  3308,  3310,  3312,  3314,  3316,  3318,  3320,  3324,
    3328,  3333,  3336,  3338,  3340,  3342,  3344,  3346,  3348,  3350,
    3352,  3361,  3362,  3363,  3364,  3368,  3373,  3375,  3381,  3382,
    3386,  3398,  3397,  3405,  3411,  3414,  3410,  3421,  3423,  3423,
    3431,  3432,  3437,  3440,  3443,  3442,  3450,  3454,  3459,  3453,
    3464,  3466,  3463,  3474,  3476,  3478,  3480,  3473,  3485,  3487,
    3484,  3492,  3491,  3496,  3495,  3500,  3499,  3503,  3505,  3507,
    3509,  3511,  3516,  3519,  3522,  3525,  3528,  3531,  3534,  3540,
    3542,  3544,  3548,  3551,  3553,  3555,  3558,  3564,  3566,  3563,
    3573,  3575,  3572,  3581,  3582,  3584,  3596,  3598,  3595,  3604,
    3605,  3609,  3625,  3634,  3636,  3638,  3640,  3645,  3647,  3648,
    3658,  3659,  3664,  3665,  3666,  3674,  3675,  3679,  3680,  3685,
    3687,  3694,  3696,  3708,  3711,  3712,  3720,  3722,  3725,  3727,
    3730,  3732,  3742,  3758,  3757,  3764,  3765,  3770,  3773,  3776,
    3779,  3781,  3786,  3787,  3797,  3800,  3803,  3807,  3810,  3813,
    3819,  3822,  3828,  3829,  3833,  3838,  3843,  3860,  3868,  3870,
    3874,  3876,  3880,  3882,  3884,  3889,  3894,  3899,  3901,  3906,
    3908,  3910,  3912,  3919,  3932,  3941,  3943,  3945,  3947,  3949,
    3951,  3953,  3955,  3957,  3959,  3961,  3963,  3965,  3967,  3969,
    3971,  3973,  3975,  3977,  3979,  3981,  3983,  3985,  3987,  3989,
    3991,  3993,  3995,  3997,  3999,  4001,  4003,  4005,  4007,  4009,
    4017
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "IDENTIFIER", "tTYPENAME", "SELFNAME", 
  "PFUNCNAME", "SCSPEC", "TYPESPEC", "CV_QUALIFIER", "CONSTANT", 
  "VAR_FUNC_NAME", "STRING", "ELLIPSIS", "SIZEOF", "ENUM", "IF", "ELSE", 
  "WHILE", "DO", "FOR", "SWITCH", "CASE", "DEFAULT", "BREAK", "CONTINUE", 
  "RETURN_KEYWORD", "GOTO", "ASM_KEYWORD", "TYPEOF", "ALIGNOF", "SIGOF", 
  "ATTRIBUTE", "EXTENSION", "LABEL", "REALPART", "IMAGPART", "VA_ARG", 
  "AGGR", "VISSPEC", "DELETE", "NEW", "THIS", "OPERATOR", "CXX_TRUE", 
  "CXX_FALSE", "NAMESPACE", "TYPENAME_KEYWORD", "USING", "LEFT_RIGHT", 
  "TEMPLATE", "TYPEID", "DYNAMIC_CAST", "STATIC_CAST", "REINTERPRET_CAST", 
  "CONST_CAST", "SCOPE", "EXPORT", "EMPTY", "NSNAME", "PTYPENAME", "'{'", 
  "','", "';'", "THROW", "':'", "'='", "ASSIGN", "'?'", "OROR", "ANDAND", 
  "'|'", "'^'", "'&'", "MIN_MAX", "EQCOMPARE", "'<'", "'>'", 
  "ARITHCOMPARE", "RSHIFT", "LSHIFT", "'+'", "'-'", "'*'", "'/'", "'%'", 
  "DOT_STAR", "POINTSAT_STAR", "'~'", "MINUSMINUS", "PLUSPLUS", "UNARY", 
  "HYPERUNARY", "'.'", "'('", "'['", "POINTSAT", "CATCH", "TRY", 
  "EXTERN_LANG_STRING", "ALL", "PRE_PARSED_CLASS_DECL", "DEFARG", 
  "DEFARG_MARKER", "PRE_PARSED_FUNCTION_DECL", "TYPENAME_DEFN", 
  "IDENTIFIER_DEFN", "PTYPENAME_DEFN", "END_OF_LINE", 
  "END_OF_SAVED_INPUT", "')'", "'}'", "'!'", "']'", "$accept", "program", 
  "extdefs", "@1", "extdefs_opt", ".hush_warning", ".warning_ok", 
  "extension", "asm_keyword", "lang_extdef", "@2", "extdef", "@3", "@4", 
  "@5", "namespace_alias", "@6", "using_decl", "namespace_using_decl", 
  "using_directive", "@7", "namespace_qualifier", "any_id", 
  "extern_lang_string", "template_parm_header", "@8", 
  "template_spec_header", "template_header", "template_parm_list", 
  "maybe_identifier", "template_type_parm", "template_template_parm", 
  "template_parm", "template_def", "template_extdef", "template_datadef", 
  "datadef", "ctor_initializer_opt", "maybe_return_init", 
  "eat_saved_input", "function_body", "@9", "fndef", 
  "constructor_declarator", "@10", "@11", "@12", "@13", "fn.def1", 
  "component_constructor_declarator", "fn_def2", "return_id", 
  "return_init", "base_init", "@14", "begin_function_body_", 
  "member_init_list", "begin_member_init", "member_init", "identifier", 
  "notype_identifier", "identifier_defn", "explicit_instantiation", "@15", 
  "@16", "@17", "@18", "@19", "@20", "@21", "@22", 
  "begin_explicit_instantiation", "end_explicit_instantiation", 
  "template_type", "apparent_template_type", "self_template_type", 
  "finish_template_type_", "template_close_bracket", 
  "template_arg_list_opt", "template_arg_list", "template_arg", "@23", 
  "template_arg_1", "unop", "expr", "paren_expr_or_null", 
  "paren_cond_or_null", "xcond", "condition", "@24", "compstmtend", 
  "nontrivial_exprlist", "nonnull_exprlist", "unary_expr", 
  "new_placement", "new_initializer", "regcast_or_absdcl", "cast_expr", 
  "expr_no_commas", "expr_no_comma_rangle", "notype_unqualified_id", 
  "do_id", "template_id", "object_template_id", "unqualified_id", 
  "expr_or_declarator_intern", "expr_or_declarator", 
  "notype_template_declarator", "direct_notype_declarator", "primary", 
  "@25", "new", "delete", "boolean_literal", "nodecls", "object", "decl", 
  "declarator", "fcast_or_absdcl", "type_id", "typed_declspecs", 
  "typed_declspecs1", "reserved_declspecs", "declmods", "typed_typespecs", 
  "reserved_typespecquals", "sizeof", "alignof", "typeof", "typespec", 
  "typespecqual_reserved", "initdecls", "notype_initdecls", 
  "nomods_initdecls", "maybeasm", "initdcl", "@26", "initdcl0_innards", 
  "@27", "initdcl0", "notype_initdcl0", "nomods_initdcl0", "@28", 
  "maybe_attribute", "attributes", "attribute", "attribute_list", 
  "attrib", "any_word", "identifiers_or_typenames", "maybe_init", "init", 
  "initlist", "pending_inline", "pending_inlines", "defarg_again", 
  "pending_defargs", "structsp", "@29", "@30", "@31", "@32", "@33", 
  "maybecomma", "maybecomma_warn", "aggr", "class_head", 
  "class_head_apparent_template", "class_head_decl", "class_head_defn", 
  "maybe_base_class_list", "base_class_list", "base_class", 
  "base_class_1", "base_class_access_list", "opt.component_decl_list", 
  "access_specifier", "component_decl_list", "component_decl", 
  "component_decl_1", "components", "notype_components", 
  "component_declarator0", "component_declarator", 
  "after_type_component_declarator0", "notype_component_declarator0", 
  "after_type_component_declarator", "notype_component_declarator", 
  "enumlist_opt", "enumlist", "enumerator", "new_type_id", 
  "cv_qualifiers", "nonempty_cv_qualifiers", "maybe_parmlist", 
  "after_type_declarator_intern", "after_type_declarator", 
  "direct_after_type_declarator", "nonnested_type", "complete_type_name", 
  "nested_type", "notype_declarator_intern", "notype_declarator", 
  "complex_notype_declarator", "complex_direct_notype_declarator", 
  "qualified_id", "notype_qualified_id", "overqualified_id", 
  "functional_cast", "type_name", "nested_name_specifier", 
  "nested_name_specifier_1", "typename_sub", "typename_sub0", 
  "typename_sub1", "typename_sub2", "explicit_template_type", 
  "complex_type_name", "ptr_to_mem", "global_scope", "new_declarator", 
  "direct_new_declarator", "absdcl_intern", "absdcl", 
  "direct_abstract_declarator", "stmts", "errstmt", "maybe_label_decls", 
  "label_decls", "label_decl", "compstmt_or_stmtexpr", "@34", "compstmt", 
  "simple_if", "@35", "@36", "implicitly_scoped_stmt", "@37", "stmt", 
  "simple_stmt", "@38", "@39", "@40", "@41", "@42", "@43", "@44", "@45", 
  "@46", "@47", "@48", "@49", "@50", "@51", "function_try_block", "@52", 
  "@53", "try_block", "@54", "@55", "handler_seq", "handler", "@56", 
  "@57", "type_specifier_seq", "handler_args", "label_colon", 
  "for.init.statement", "maybe_cv_qualifier", "xexpr", "asm_operands", 
  "nonnull_asm_operands", "asm_operand", "asm_clobbers", "parmlist", 
  "complex_parmlist", "defarg", "@58", "defarg1", "parms", "parms_comma", 
  "named_parm", "full_parm", "parm", "see_typename", "bad_parm", 
  "bad_decl", "template_arg_list_ignore", "arg_list_ignore", 
  "exception_specification_opt", "ansi_raise_identifier", 
  "ansi_raise_identifiers", "conversion_declarator", "operator", 
  "unoperator", "operator_name", "save_lineno", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   123,    44,    59,   316,    58,    61,   317,    63,   318,
     319,   124,    94,    38,   320,   321,    60,    62,   322,   323,
     324,    43,    45,    42,    47,    37,   325,   326,   126,   327,
     328,   329,   330,    46,    40,    91,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
      41,   125,    33,    93
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned short yyr1[] =
{
       0,   114,   115,   115,   117,   116,   116,   118,   118,   119,
     120,   121,   122,   124,   123,   125,   125,   126,   125,   125,
     125,   125,   125,   125,   127,   125,   128,   125,   125,   125,
     125,   125,   130,   129,   131,   131,   131,   132,   132,   132,
     134,   133,   135,   135,   136,   136,   136,   136,   137,   137,
     139,   138,   140,   141,   141,   142,   142,   143,   143,   144,
     144,   145,   146,   146,   146,   146,   146,   146,   147,   147,
     148,   148,   148,   148,   148,   148,   149,   149,   149,   149,
     150,   150,   150,   150,   150,   150,   150,   150,   150,   150,
     150,   151,   151,   152,   152,   152,   153,   153,   155,   154,
     156,   156,   156,   158,   157,   157,   159,   157,   157,   160,
     157,   157,   161,   157,   157,   162,   162,   162,   162,   162,
     163,   163,   163,   163,   163,   163,   164,   164,   164,   164,
     164,   164,   164,   165,   166,   166,   166,   168,   167,   169,
     170,   170,   170,   170,   171,   171,   171,   171,   172,   172,
     172,   173,   173,   173,   173,   173,   174,   174,   174,   175,
     175,   175,   177,   176,   178,   176,   179,   176,   180,   176,
     181,   176,   182,   176,   183,   176,   184,   176,   185,   186,
     187,   187,   187,   188,   188,   189,   190,   191,   191,   192,
     192,   193,   193,   195,   194,   196,   196,   196,   196,   196,
     197,   197,   197,   197,   197,   198,   198,   199,   199,   200,
     200,   201,   201,   201,   203,   202,   202,   204,   204,   204,
     204,   205,   205,   205,   205,   206,   206,   207,   207,   207,
     207,   207,   207,   207,   207,   207,   207,   207,   207,   207,
     207,   207,   207,   207,   207,   207,   207,   207,   207,   207,
     207,   208,   208,   209,   209,   209,   209,   210,   210,   211,
     211,   211,   212,   212,   212,   212,   212,   212,   212,   212,
     212,   212,   212,   212,   212,   212,   212,   212,   212,   212,
     212,   212,   212,   212,   212,   212,   212,   213,   213,   213,
     213,   213,   213,   213,   213,   213,   213,   213,   213,   213,
     213,   213,   213,   213,   213,   213,   213,   213,   213,   213,
     213,   214,   214,   214,   214,   214,   214,   214,   215,   216,
     216,   217,   217,   217,   218,   218,   218,   219,   219,   220,
     220,   220,   220,   221,   221,   222,   222,   222,   222,   223,
     223,   223,   223,   223,   223,   223,   223,   224,   223,   223,
     223,   223,   223,   223,   223,   223,   223,   223,   223,   223,
     223,   223,   223,   223,   223,   223,   223,   223,   223,   223,
     223,   223,   223,   223,   223,   223,   223,   223,   223,   223,
     223,   223,   223,   223,   225,   225,   226,   226,   227,   227,
     228,   229,   229,   230,   230,   230,   230,   230,   230,   231,
     231,   232,   232,   233,   233,   233,   233,   233,   234,   234,
     235,   235,   235,   235,   235,   235,   236,   236,   236,   237,
     237,   237,   237,   237,   238,   238,   238,   238,   239,   239,
     239,   239,   240,   241,   242,   243,   243,   243,   243,   243,
     243,   243,   244,   244,   244,   245,   245,   246,   246,   247,
     247,   248,   248,   250,   249,   249,   252,   251,   251,   253,
     254,   256,   255,   255,   257,   257,   258,   258,   259,   260,
     260,   261,   261,   261,   261,   261,   262,   262,   262,   262,
     263,   263,   264,   264,   265,   265,   265,   265,   265,   266,
     266,   266,   266,   266,   267,   267,   267,   268,   268,   269,
     269,   270,   270,   270,   272,   271,   273,   271,   271,   271,
     271,   274,   275,   276,   271,   271,   277,   277,   278,   278,
     279,   279,   279,   279,   279,   279,   280,   280,   280,   280,
     281,   281,   281,   282,   282,   282,   283,   283,   283,   283,
     283,   283,   283,   284,   284,   284,   285,   285,   286,   286,
     287,   287,   288,   288,   288,   288,   289,   289,   289,   289,
     290,   291,   291,   292,   292,   292,   292,   292,   292,   292,
     292,   292,   292,   292,   293,   293,   293,   293,   293,   293,
     293,   293,   293,   294,   294,   294,   295,   295,   295,   296,
     296,   297,   297,   298,   298,   299,   299,   299,   299,   300,
     300,   301,   301,   301,   302,   302,   303,   303,   304,   304,
     305,   305,   305,   306,   306,   307,   307,   307,   307,   308,
     308,   308,   308,   309,   309,   310,   310,   310,   310,   310,
     310,   311,   311,   311,   311,   311,   311,   312,   312,   313,
     313,   313,   314,   315,   315,   316,   316,   316,   316,   316,
     316,   317,   317,   317,   317,   317,   317,   318,   318,   318,
     318,   318,   318,   318,   318,   319,   319,   320,   320,   321,
     321,   322,   322,   322,   323,   323,   323,   324,   324,   324,
     324,   324,   325,   325,   325,   325,   326,   326,   327,   327,
     327,   327,   328,   328,   328,   328,   329,   329,   329,   329,
     329,   329,   330,   331,   331,   331,   332,   332,   333,   334,
     334,   334,   334,   334,   334,   334,   335,   335,   336,   336,
     337,   337,   337,   337,   337,   337,   337,   337,   337,   337,
     337,   338,   338,   338,   338,   338,   338,   338,   338,   338,
     338,   339,   339,   339,   339,   340,   341,   341,   342,   342,
     343,   345,   344,   346,   348,   349,   347,   350,   351,   350,
     352,   352,   353,   353,   354,   353,   353,   355,   356,   353,
     357,   358,   353,   359,   360,   361,   362,   353,   363,   364,
     353,   365,   353,   366,   353,   367,   353,   353,   353,   353,
     353,   353,   353,   353,   353,   353,   353,   353,   353,   353,
     353,   353,   353,   353,   353,   353,   353,   369,   370,   368,
     372,   373,   371,   374,   374,   374,   376,   377,   375,   378,
     378,   379,   379,   380,   380,   380,   380,   381,   381,   381,
     382,   382,   383,   383,   383,   384,   384,   385,   385,   386,
     386,   387,   387,   388,   388,   388,   389,   389,   389,   389,
     389,   389,   389,   391,   390,   392,   392,   393,   393,   393,
     393,   393,   394,   394,   395,   395,   395,   395,   395,   395,
     396,   396,   397,   397,   398,   399,   399,   400,   401,   401,
     402,   402,   403,   403,   403,   404,   404,   405,   405,   406,
     406,   406,   406,   407,   408,   409,   409,   409,   409,   409,
     409,   409,   409,   409,   409,   409,   409,   409,   409,   409,
     409,   409,   409,   409,   409,   409,   409,   409,   409,   409,
     409,   409,   409,   409,   409,   409,   409,   409,   409,   409,
     410
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     0,     1,     0,     2,     2,     1,     0,     0,
       0,     1,     1,     0,     2,     2,     1,     0,     3,     1,
       5,     4,     5,     4,     0,     6,     0,     5,     1,     2,
       1,     2,     0,     6,     2,     3,     3,     3,     3,     4,
       0,     5,     2,     3,     1,     1,     2,     2,     1,     2,
       0,     5,     3,     1,     1,     1,     3,     1,     0,     2,
       2,     3,     1,     3,     1,     3,     1,     3,     2,     2,
       2,     1,     1,     5,     4,     2,     2,     3,     3,     2,
       2,     3,     3,     2,     2,     2,     2,     2,     2,     1,
       1,     1,     1,     0,     1,     2,     0,     1,     0,     6,
       3,     3,     3,     0,     8,     5,     0,     9,     6,     0,
       8,     5,     0,     9,     6,     2,     2,     1,     2,     1,
       6,     8,     4,     6,     6,     4,     2,     1,     2,     2,
       1,     2,     1,     2,     2,     4,     2,     0,     3,     0,
       0,     1,     3,     2,     0,     1,     1,     1,     4,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     0,     6,     0,     6,     0,     5,     0,     5,
       0,     7,     0,     7,     0,     6,     0,     6,     0,     0,
       5,     5,     1,     1,     5,     5,     0,     1,     1,     0,
       1,     1,     3,     0,     2,     1,     1,     2,     1,     3,
       1,     1,     1,     1,     1,     1,     1,     1,     3,     1,
       3,     0,     1,     1,     0,     7,     1,     1,     3,     4,
       3,     3,     3,     3,     3,     1,     1,     1,     2,     2,
       2,     2,     2,     2,     2,     4,     2,     4,     2,     3,
       3,     4,     4,     5,     5,     6,     2,     4,     5,     2,
       2,     3,     3,     3,     1,     3,     2,     3,     4,     1,
       2,     5,     1,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     5,     3,     3,     1,     2,     1,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     5,     3,     3,     1,
       2,     3,     3,     1,     1,     1,     1,     1,     0,     5,
       5,     5,     5,     5,     1,     1,     1,     1,     2,     1,
       2,     2,     3,     4,     4,     1,     1,     1,     3,     1,
       1,     1,     1,     1,     3,     3,     3,     0,     4,     4,
       2,     4,     2,     6,     4,     2,     2,     1,     4,     1,
       7,     7,     7,     7,     4,     4,     2,     2,     2,     1,
       4,     2,     2,     5,     3,     2,     2,     5,     3,     5,
       3,     4,     6,     2,     1,     2,     1,     2,     1,     1,
       0,     2,     2,     3,     3,     3,     2,     2,     2,     1,
       1,     1,     2,     2,     2,     2,     1,     1,     1,     1,
       2,     2,     3,     3,     3,     4,     1,     2,     2,     1,
       1,     2,     2,     2,     1,     2,     2,     3,     1,     2,
       2,     1,     1,     1,     1,     1,     1,     1,     4,     4,
       4,     4,     1,     1,     1,     1,     3,     1,     3,     1,
       3,     0,     4,     0,     6,     3,     0,     4,     1,     3,
       3,     0,     4,     3,     0,     1,     1,     2,     6,     1,
       3,     0,     1,     4,     6,     4,     1,     1,     1,     1,
       1,     3,     0,     2,     1,     2,     3,     4,     1,     1,
       3,     4,     3,     5,     3,     3,     3,     0,     3,     3,
       3,     0,     2,     2,     0,     6,     0,     5,     2,     2,
       2,     0,     0,     0,    11,     1,     0,     1,     0,     1,
       1,     2,     2,     2,     2,     2,     2,     3,     4,     3,
       2,     3,     4,     1,     2,     1,     2,     2,     2,     2,
       3,     3,     2,     0,     2,     3,     1,     4,     1,     3,
       1,     1,     2,     2,     3,     3,     0,     1,     3,     2,
       2,     1,     2,     2,     2,     2,     2,     2,     2,     1,
       2,     2,     3,     1,     2,     2,     4,     4,     2,     1,
       5,     4,     1,     0,     1,     3,     0,     1,     3,     1,
       1,     1,     1,     4,     4,     4,     4,     4,     3,     4,
       4,     4,     4,     3,     2,     1,     1,     3,     1,     3,
       2,     1,     6,     0,     2,     1,     2,     1,     2,     3,
       3,     1,     3,     1,     2,     3,     3,     2,     2,     3,
       1,     4,     4,     3,     3,     2,     1,     1,     2,     1,
       1,     2,     2,     1,     2,     3,     3,     2,     2,     3,
       1,     3,     3,     2,     2,     3,     1,     4,     3,     4,
       3,     1,     2,     2,     2,     2,     2,     2,     2,     1,
       2,     4,     4,     2,     1,     1,     1,     1,     2,     4,
       3,     3,     2,     2,     2,     2,     1,     2,     2,     2,
       2,     3,     1,     2,     3,     4,     2,     2,     2,     2,
       2,     2,     4,     2,     1,     2,     2,     3,     1,     3,
       2,     3,     2,     2,     3,     1,     3,     4,     1,     2,
       3,     2,     2,     1,     3,     2,     2,     1,     2,     3,
       1,     3,     6,     4,     4,     3,     5,     3,     3,     3,
       2,     1,     1,     2,     2,     2,     0,     1,     1,     2,
       3,     0,     4,     1,     0,     0,     5,     1,     0,     3,
       1,     2,     1,     2,     0,     4,     1,     0,     0,     5,
       0,     0,     7,     0,     0,     0,     0,    12,     0,     0,
       7,     0,     5,     0,     7,     0,     4,     2,     2,     2,
       3,     6,     8,    10,     8,    12,    10,    10,     4,     3,
       2,     2,     1,     1,     1,     1,     1,     0,     0,     5,
       0,     0,     5,     1,     2,     0,     0,     0,     5,     1,
       1,     3,     3,     2,     2,     2,     2,     2,     1,     2,
       0,     1,     0,     1,     1,     0,     1,     1,     3,     4,
       7,     1,     3,     0,     1,     1,     1,     2,     2,     2,
       1,     2,     2,     0,     3,     1,     1,     1,     2,     2,
       2,     4,     2,     2,     2,     2,     2,     2,     1,     2,
       1,     2,     1,     1,     0,     0,     1,     5,     3,     0,
       3,     0,     0,     4,     2,     1,     1,     1,     3,     0,
       3,     3,     3,     1,     0,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     4,     3,
       3,     3,     3,     4,     3,     3,     5,     5,     4,     3,
       0
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned short yydefact[] =
{
       4,     0,    13,    13,     1,     6,     0,     5,     0,   315,
     674,   675,     0,   420,   436,   615,     0,    12,   434,     0,
       0,    11,   520,   893,     0,     0,     0,   178,   708,    17,
     317,   316,    89,     0,     0,   874,     0,    48,     0,     0,
      14,    28,     0,    30,     9,    53,    54,     0,    19,    16,
      96,   119,    93,     0,   676,   182,   336,   313,   337,   650,
       0,   409,     0,   408,     0,   424,     0,   449,   617,   466,
     435,     0,   533,   535,   515,   543,   419,   639,   437,   640,
     117,   335,   661,   637,     0,   677,   613,     0,    90,     0,
     314,    86,    88,    87,   193,     0,   682,   193,   683,   193,
     318,   178,   151,   152,   153,   155,   154,   506,   508,     0,
     704,     0,   509,     0,     0,     0,   152,   153,   155,   154,
      26,     0,     0,     0,     0,     0,     0,     0,   510,   686,
       0,   692,     0,     0,     0,    40,     0,     0,    34,     0,
       0,    50,     0,     0,   684,   193,   193,   315,   617,     0,
     648,   643,     0,     0,     0,   647,     0,     0,     0,     0,
     336,     0,   327,     0,     0,     0,   335,   613,    31,     0,
      29,     4,    49,     0,    69,   420,     0,     0,     9,    72,
      68,    71,    96,     0,     0,     0,   435,    97,    15,     0,
     464,     0,     0,   482,    94,    84,   685,   621,     0,     0,
     613,    85,     0,     0,     0,   115,     0,   445,   399,   630,
     400,   636,     0,   613,   422,   421,    83,   118,   410,     0,
     447,   423,   116,     0,   416,   442,   443,   411,   426,   428,
     431,   444,     0,    80,   467,   521,   522,   523,   524,   542,
     160,   159,   161,   526,   534,   183,   530,   525,     0,     0,
     536,   537,   538,   539,   874,     0,   616,   425,   618,     0,
     461,   315,   675,     0,   316,   706,   182,   667,   668,   664,
     642,   678,     0,   315,   317,   663,   641,   662,   638,     0,
     894,   894,   894,   894,   894,   894,   894,     0,   894,   894,
     894,   894,   894,   894,   894,   894,   894,   894,   894,   894,
     894,   894,   894,   894,   894,   894,   894,   894,   894,     0,
     894,   894,   819,   424,   820,   889,   318,     0,   190,   191,
       0,   881,     0,     0,   193,     0,   518,   504,     0,     0,
       0,   705,   703,   615,   340,   343,   342,   432,   433,     0,
       0,     0,   386,   384,   357,   388,   389,     0,     0,     0,
       0,     0,   285,     0,     0,   201,   200,     0,     0,   203,
     202,     0,   204,     0,     0,     0,   205,   259,     0,   262,
     206,   339,   227,     0,     0,   341,     0,     0,   406,     0,
       0,   424,   407,   669,   369,   359,     0,     0,   471,     4,
      24,    32,   700,   696,   697,   701,   699,   698,   151,   152,
     153,     0,   155,   154,   688,   689,   693,   690,   687,     0,
     315,   325,   326,   324,   666,   665,    36,    35,    52,     0,
     168,     0,     0,   424,   166,    18,     0,     0,   193,   644,
     618,   646,     0,   645,   152,   153,   311,   312,   331,   617,
       0,   654,   330,     0,   653,     0,   338,   317,   316,     0,
       0,     0,   329,   328,   658,     0,     0,    13,     0,   178,
      10,    10,    75,     0,    70,     0,     0,    76,    79,     0,
     463,   465,   133,   102,   807,   100,   390,   101,   136,     0,
       0,   134,    95,     0,   850,   226,     0,   225,   845,   868,
       0,   406,   424,   407,     0,   844,   846,   875,   857,     0,
       0,   660,     0,     0,   882,   617,     0,   628,   623,     0,
     627,     0,     0,     0,     0,     0,   613,   464,     0,    82,
       0,   613,   635,     0,   413,   414,     0,    81,   464,     0,
       0,   418,   417,   412,   429,   430,   451,   450,   193,   540,
     541,   151,   154,   527,   531,   529,     0,   544,   511,   427,
     464,   680,   613,   103,     0,     0,     0,     0,   681,   613,
     109,   614,     0,   649,   675,   707,   182,   929,     0,   925,
       0,   924,   922,   904,   910,   909,   894,   916,   915,   901,
     902,   900,   919,   908,   906,   907,   905,   912,   911,   898,
     899,   895,   896,   897,   921,   903,   914,   913,   894,   920,
     917,   426,   613,   613,     0,   613,     0,   894,   193,   187,
     188,   333,   193,   316,   309,   194,   287,   198,   195,     0,
       0,     0,     0,   186,   186,     0,   176,     0,   424,   174,
     519,   608,   605,     0,   518,   606,   518,     0,     0,   249,
     250,     0,     0,     0,     0,     0,     0,   286,   233,   230,
     229,   231,     0,     0,     0,     0,     0,   339,     0,   930,
       0,   228,   232,   440,     0,     0,     0,   260,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   350,     0,   352,   356,   355,   391,     0,     0,   392,
       0,     0,     0,   238,   611,     0,   246,   383,     0,     0,
     874,   372,   375,   376,     0,     0,   441,   401,   727,   723,
       0,     0,   613,   613,   613,   403,   730,     0,   234,     0,
     236,     0,   673,   405,     0,     0,   404,   371,     0,   366,
     385,   367,   387,   670,     0,   368,   477,   478,   479,   476,
       0,   469,   472,     0,     4,     0,   691,   193,   694,     0,
      44,    45,     0,    58,     0,     0,     0,    62,    66,    55,
     873,   424,    58,   872,    64,   179,   164,   162,   179,   334,
     186,     0,   652,   651,   338,     0,   655,     0,    21,    23,
      96,    10,    10,    78,    77,     0,   139,   137,   930,    92,
      91,   488,     0,   484,   483,     0,   622,   619,   849,   863,
     852,   727,   723,     0,   864,   613,   867,   869,     0,     0,
     865,     0,   866,   620,   848,   862,   851,   847,   876,   859,
     870,   860,   853,   858,   659,     0,   673,     0,   657,   624,
     618,   626,   625,   617,     0,     0,     0,     0,     0,     0,
     613,   634,     0,   459,   458,   446,   633,     0,   882,     0,
     629,   415,   448,   460,   438,   439,   464,     0,   528,   532,
     674,   675,   874,   874,   676,   545,   546,   548,   874,   551,
     550,     0,     0,   462,   882,   843,   193,   193,   679,   193,
     882,   843,   613,   106,   613,   112,   894,   894,   918,   923,
     889,   889,   889,     0,   928,     0,   192,   310,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     197,     0,   877,   181,   185,   319,   179,   172,   170,   179,
       0,   507,   519,   604,     0,     0,     0,     0,     0,     0,
     424,     0,     0,     0,   346,     0,   344,   345,     0,     0,
     257,   224,   223,   315,   674,   675,   317,   316,     0,     0,
     489,   516,     0,   222,   221,   283,   284,   834,   833,     0,
     281,   280,   278,   279,   277,   276,   275,   273,   274,   272,
     271,   270,   265,   266,   267,   268,   269,   264,   263,     0,
       0,     0,     0,     0,     0,     0,   240,   254,     0,     0,
     239,   613,   613,     0,   613,   610,   715,     0,     0,     0,
       0,     0,   374,     0,   378,     0,   380,     0,   617,   726,
     725,   718,   722,   721,   873,     0,     0,   740,     0,     0,
     882,   402,   882,   728,   613,   843,     0,     0,     0,   727,
     723,     0,     0,   613,     0,   617,     0,     0,     0,     0,
     471,     0,     0,    27,     0,     0,   695,     0,    41,    47,
      46,    60,    57,    50,    58,     0,    51,     0,   193,    59,
     526,     0,   169,   179,   179,   167,   180,   333,   332,    20,
      22,    74,    96,   452,   808,     0,     0,   485,     0,   135,
     617,   726,   722,   727,   723,     0,   617,   637,     0,   613,
     728,     0,   727,   723,     0,   339,     0,   669,     0,   871,
       0,     0,   884,     0,     0,     0,     0,   456,   632,   631,
     455,   186,   553,   552,   874,   874,   874,     0,   579,   675,
       0,   569,     0,     0,     0,   582,     0,   132,   127,     0,
     182,   583,   586,     0,     0,   561,     0,   130,   573,   105,
       0,     0,     0,     0,   111,     0,   882,   843,   882,   843,
     927,   926,   891,   890,   892,   320,   307,   308,     0,   305,
     304,   302,   303,   301,   300,   299,   298,   297,   296,   295,
     290,   291,   292,   293,   294,   289,   288,   199,   880,   177,
     179,   179,   175,   609,   607,   505,   358,     0,   364,   365,
       0,     0,     0,     0,   345,   348,   751,     0,     0,     0,
       0,   258,     0,   349,   351,   354,   252,   251,   242,     0,
     241,   256,     0,     0,   712,   710,     0,   713,     0,   247,
       0,     0,   193,   381,     0,     0,     0,   719,   618,   724,
     720,   731,   613,   739,   737,   738,     0,   729,   882,     0,
     735,     0,   235,   237,   671,   672,   727,   723,     0,   370,
     470,   468,   315,     0,    25,    33,   702,    61,    56,    63,
      67,    65,   165,   163,    73,   815,   150,   156,   158,   157,
       0,     0,   141,   145,   146,   147,    98,     0,   486,   618,
     726,   722,   727,   723,     0,   613,   642,   728,     0,     0,
     672,   366,   367,   670,   368,   861,   855,   856,   854,   886,
     885,   887,     0,     0,     0,     0,   618,     0,     0,   453,
     184,     0,   555,   554,   549,   613,   843,   578,     0,   570,
     583,   571,   464,   464,   567,   568,   565,   566,   613,   843,
     315,   674,     0,   451,   128,   574,   584,   589,   590,   451,
     451,     0,     0,   451,   126,   575,   587,   451,     0,   464,
       0,   562,   563,   564,   464,   613,   322,   321,   323,   613,
     108,     0,   114,     0,     0,   173,   171,     0,     0,     0,
       0,     0,   746,     0,   492,     0,   490,   261,   282,     0,
     243,   244,   253,   255,   711,   709,   716,   714,     0,   248,
       0,     0,   373,   377,   379,   882,   733,   613,   734,     0,
     473,   475,   816,   809,   813,   143,     0,   149,     0,   746,
     487,   726,   722,     0,   728,   345,     0,   883,   617,   457,
       0,   547,   882,     0,     0,   572,   482,   482,   882,     0,
       0,     0,   464,   464,     0,   464,   464,     0,   464,     0,
     560,   512,     0,   482,   882,   882,   613,   613,   306,   353,
       0,     0,     0,     0,     0,   217,   752,     0,   747,   748,
     491,     0,     0,   245,   717,   382,   321,   736,   882,     0,
       0,   814,   142,     0,    99,   727,   723,     0,   618,     0,
     888,   454,   122,   613,   613,   843,   577,   581,   125,   613,
     464,   464,   598,   482,   315,   674,     0,   585,   591,   592,
     451,   451,   482,   482,     0,   482,   588,   501,   576,   104,
     110,   882,   882,   360,   361,   362,   363,   480,     0,     0,
       0,   742,   753,   760,   741,     0,   749,   493,   612,   732,
     474,     0,   817,   148,   617,   882,   882,     0,   882,   597,
     594,   596,     0,     0,   464,   464,   464,   593,   595,   580,
       0,   107,   113,     0,   750,   745,   220,     0,   218,   744,
     743,   315,   674,   675,   754,   767,   770,   773,   778,     0,
       0,     0,     0,     0,     0,     0,     0,   316,   802,   810,
       0,   830,   806,   805,   804,     0,   762,     0,     0,   424,
     766,   761,   803,   930,     0,     0,   930,   120,   123,   613,
     124,   464,   464,   603,   482,   482,   503,     0,   502,   497,
     481,   219,   823,   825,   826,     0,     0,   758,     0,     0,
       0,   785,   787,   788,   789,     0,     0,     0,     0,     0,
       0,     0,   824,   930,   398,   831,     0,   763,   396,   451,
       0,   397,     0,   451,     0,     0,   764,   801,   800,   821,
     822,   818,   882,   602,   600,   599,   601,     0,     0,   514,
     209,     0,   755,   768,   757,     0,   930,     0,     0,     0,
     781,   930,   790,     0,   799,    42,   155,    37,   155,     0,
      38,   811,     0,   394,   395,     0,     0,     0,   393,   758,
     121,   500,   499,    93,    96,   216,     0,   424,     0,   758,
     758,   771,     0,   746,   828,   774,     0,     0,     0,   930,
     786,   798,    43,    39,   815,     0,   765,     0,   498,   210,
     451,   756,   769,     0,   759,   829,     0,   827,   779,   783,
     782,   812,   835,   835,     0,   496,   494,   495,   464,   207,
       0,     0,   213,     0,   212,   758,   930,     0,     0,     0,
     836,   837,     0,   791,     0,     0,   772,   775,   780,   784,
       0,     0,     0,     0,     0,     0,   835,     0,   214,   208,
       0,     0,     0,   841,     0,   794,   838,     0,     0,   792,
       0,     0,   839,     0,     0,     0,     0,     0,     0,   215,
     776,     0,   842,   796,   797,     0,   793,   758,     0,     0,
     777,   840,   795
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,     1,   457,     3,   458,   173,   789,   363,   189,     5,
       6,    40,   143,   754,   389,    41,   755,  1145,  1603,    43,
     409,  1650,   759,    44,    45,   419,    46,  1146,   766,  1071,
     767,   768,   769,    48,   180,   181,    49,   798,   192,   188,
     475,  1429,    50,    51,   885,  1167,   891,  1169,    52,  1148,
    1149,   193,   194,   799,  1095,   476,  1290,  1291,  1292,   631,
    1293,   244,    53,  1084,  1083,   778,   775,  1201,  1200,   939,
     936,   142,  1082,    54,   246,    55,   933,   611,   317,   318,
     319,   320,   615,   364,   656,  1761,  1682,  1763,  1716,  1800,
    1476,   366,  1052,   367,   702,  1010,   368,   369,   370,   617,
     371,   324,    57,   268,   760,   438,   162,    58,    59,   372,
     659,   373,   374,   375,   800,   376,  1606,   536,   723,  1034,
    1151,   489,   227,   490,   491,   228,   379,   380,    64,   503,
     229,   206,   219,    66,   517,   537,  1440,   853,  1328,   207,
     220,    67,   550,   854,    68,    69,   750,   751,   752,  1538,
     481,   970,   971,  1714,  1679,  1628,  1570,    70,   636,   326,
     882,  1527,  1629,  1220,   632,    71,    72,    73,    74,    75,
     255,   875,   876,   877,   878,  1153,  1370,  1154,  1155,  1156,
    1355,  1365,  1356,  1517,  1357,  1358,  1518,  1519,   633,   634,
     635,   703,  1040,   493,   200,   515,   508,   209,    77,    78,
      79,   150,   151,   165,    81,   138,   383,   384,   385,    83,
     386,    85,   880,   129,   130,   131,   556,   112,    86,   387,
    1015,  1016,  1035,  1031,   726,  1540,  1541,  1477,  1478,  1479,
    1542,  1392,  1543,  1610,  1635,  1719,  1685,  1686,  1544,  1611,
    1709,  1636,  1720,  1637,  1743,  1638,  1746,  1790,  1817,  1639,
    1765,  1729,  1766,  1691,   477,   796,  1285,  1612,  1653,  1734,
    1423,  1424,  1490,  1616,  1718,  1552,  1613,  1725,  1656,   979,
    1769,  1770,  1771,  1794,   494,  1036,   833,  1121,  1318,   496,
     497,   498,   829,   499,   156,   831,  1158,    95,   622,   838,
    1321,  1322,   607,    89,   567,    90,   959
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -1665
static const short yypact[] =
{
     130,   160,   173, -1665, -1665, -1665,  5561, -1665,   241,   112,
     163,   256,   147,   196, -1665, -1665,   956, -1665, -1665,   183,
     195, -1665, -1665, -1665,  1811,   868,   804,   260, -1665, -1665,
     313,   269, -1665,  1586,  1586, -1665,  2006, -1665,  5561,   280,
   -1665, -1665,   357, -1665,    48, -1665, -1665,  5783, -1665, -1665,
     318,   958,   425,   383,   411, -1665, -1665, -1665, -1665,   484,
    2153, -1665,  7092, -1665,   376,  1692,   538, -1665,   488, -1665,
   -1665,   991,    72,   365, -1665,   458,  8376, -1665, -1665, -1665,
     905, -1665, -1665, -1665,  1085, -1665, -1665,  2141, -1665,  4136,
     465, -1665, -1665, -1665,   334,   526, -1665,   334, -1665,   334,
   -1665, -1665, -1665,   163,   256,   504,   269, -1665,   502,   411,
   -1665,  1213, -1665,   265, 11780,   472, -1665, -1665, -1665, -1665,
   -1665,   176,   529,   368,   588,   541,   604,   557, -1665, -1665,
    2028, -1665,  1153,   163,   256, -1665,   504,   269, -1665,  1021,
    2162,   546,  7936,   605, -1665, -1665,   334,   553,  4659,  2745,
   -1665, -1665,  1354,  2948,  2745, -1665,  1548,  2955,  2955,  2006,
     522,   528, -1665,   484,   533,   552,   561, -1665, -1665,   666,
   -1665,   586, -1665,  5953, -1665, -1665,   260,  3097,   614, -1665,
   -1665, -1665,   318,  3345,  8134,   833,   652, -1665, -1665,   638,
     488,   733,   161,   571,   683, -1665, -1665, -1665,  4848, 10827,
   -1665, -1665,  4595,  4595,  5201,   905,   895, -1665, -1665,   579,
   -1665, -1665,  2338, -1665, -1665, -1665, -1665, -1665,  1692,  1044,
   -1665,   488,   905, 11780, -1665, -1665, -1665,  1315,  1692, -1665,
     488, -1665,  3345, -1665, -1665, -1665, -1665, -1665, -1665, -1665,
   -1665, -1665, -1665,   678,   493,   411, -1665,   488,  2420,  1729,
   -1665, -1665, -1665, -1665, -1665,   708, -1665,  1632,   488,   265,
   -1665,   662,   775,  2342,   692, -1665,   392, -1665, -1665, -1665,
   -1665, -1665,  5659, -1665,   504, -1665, -1665, -1665, -1665,  2974,
   -1665,   680,   690, -1665, -1665, -1665, -1665,   730, -1665, -1665,
   -1665, -1665, -1665, -1665, -1665, -1665, -1665, -1665, -1665, -1665,
   -1665, -1665, -1665, -1665, -1665, -1665, -1665, -1665, -1665,   684,
   -1665, -1665, -1665,  1632,  8376,   521, -1665,   838,   764, -1665,
   11872,   741,   838,   838,   334,  7936,  1379, -1665,   746,  1870,
     692, -1665, -1665,   755, -1665, -1665, -1665, -1665, -1665, 12977,
   12977,   760, -1665, -1665, -1665, -1665, -1665,   768,   782,   790,
     792,   798, 12332,  1870, 12977, -1665, -1665, 12977, 12977, -1665,
   -1665,  9625, -1665, 12977, 12977,   776,   827, -1665, 12424, -1665,
    8880,   414,   944,  7982, 12516, -1665,  1920,   802,  1423, 13069,
   13161,  6983,  6531, -1665,   478, -1665,  1750,  3357,  2544,   586,
   -1665, -1665, -1665, -1665, -1665, -1665, -1665, -1665,   529,   368,
     588,  1870,   541,   604,   823,   557, -1665,   846, -1665,  1251,
     746,   163,   256, -1665, -1665, -1665, -1665, -1665, -1665,  6439,
   -1665,  3345,  8115,  1500, -1665, -1665,   617,   838,   334, -1665,
    4659, -1665,  3170, -1665,   850,   863, -1665, -1665, -1665,   533,
    2745, -1665, -1665,  2745, -1665,   836, -1665, -1665, -1665,   533,
     533,   533, -1665, -1665, -1665,  5659,   839,   852,   864, -1665,
   -1665, -1665, -1665,  7936, -1665,  1051,  1055, -1665, -1665,   943,
   -1665,   488, -1665, -1665, -1665, -1665,   912, -1665, -1665, 10275,
   12332, -1665, -1665,   875, -1665,   827,   887,  8880,   294,  1577,
    8134,  1577,  2426,  6889,   893, -1665,   387,  5854,   945,   964,
     755, -1665,   909,   587,   150,  8153,  6073, -1665, -1665,  6073,
   -1665,  6420,  6420,  5201,  8240,   931, -1665,   488,  3345, -1665,
   10919, -1665, -1665,  6828,  1315,  1692,  3345, -1665,   488,   933,
     936, -1665, -1665,  1315, -1665,   488,  1007, -1665,   980, -1665,
   -1665,   746,   692,   678, -1665, -1665,  2420,  2930, -1665,  1632,
     488, -1665, -1665, -1665,   994,   999,  1011,  1003, -1665, -1665,
   -1665, -1665,  4659, -1665,   844, -1665,   626, -1665,   972, -1665,
     974, -1665, -1665, -1665, -1665, -1665, -1665, -1665, -1665, -1665,
   -1665, -1665, -1665, -1665, -1665, -1665, -1665, -1665, -1665, -1665,
   -1665, -1665, -1665, -1665, -1665, -1665, -1665, -1665, -1665, -1665,
   -1665,  1632, -1665, -1665,  1175, -1665,   788, -1665,   334, -1665,
   -1665,  1090, -1665,  1050, 12609, -1665, -1665,  8551, -1665,  2664,
    3642, 12332,  1042, -1665, -1665,   838, -1665,  3345,  2849, -1665,
   -1665,  1065, -1665,  1023,  1078, -1665,  1379,   823, 12332, -1665,
   -1665, 12332, 11780,  8439,  8439,  8439,  8439, 13231, -1665, -1665,
   -1665, -1665,  1026, 12701, 12701,  9625,  1061,   232,  1064, -1665,
    1073, -1665, -1665, -1665, 10551,  9717,  9625, -1665, 10643, 12332,
   12332, 10367, 12332, 12332, 12332, 12332, 12332, 12332, 12332, 12332,
   12332, 12332, 12332, 12332, 12332, 12332, 12332, 12332, 12332, 12332,
   12332, -1665, 12332, -1665, -1665, -1665, -1665, 12332, 12332, -1665,
   12332, 11780,  6286,   636,   747, 11011, -1665, -1665,  1098,  2342,
    1186,   710,   721,   727,  2683,   788, -1665, -1665,  2354,  2354,
    5137, 11103,  1112,  1159, -1665, -1665,   663,  9625, -1665,  9625,
   -1665, 11501,   412, -1665,  1365,   265, -1665, -1665, 12332, -1665,
   -1665, -1665, -1665, -1665,   262,   465, -1665, -1665, -1665, -1665,
     156, -1665,  1125,  1113,   586,  1251,  1171,   334, -1665,  1165,
   -1665, -1665,  2162,  1339,  1154,  1193,    57,  1166,  1179, -1665,
   -1665,  3196,   991, -1665,  1180, -1665, -1665, -1665, -1665, -1665,
   -1665,   838, -1665, -1665,  1133,  1141, -1665,  1190, -1665, -1665,
     318, -1665, -1665, -1665, -1665,  1151, -1665, -1665, -1665, -1665,
   -1665, -1665,  9530, 13231, -1665,  1156, -1665, -1665, -1665, -1665,
   -1665,  3283,  3283,  5431, -1665, -1665, -1665, -1665,  2338,  2141,
   -1665, 11594, -1665, -1665, -1665, -1665, -1665, -1665, -1665, -1665,
     964,  1201, -1665, -1665, -1665, 11964,  1159,   738, -1665, -1665,
    8153, -1665, -1665,  8240,  6073,  6073,  7718,  7718,  8240,  1365,
   -1665, -1665,  6828, -1665,  1203, -1665, -1665,  1157,   150,  8153,
   -1665,  1315, -1665, -1665, -1665, -1665,   488,  1187,   678, -1665,
     368,   588, -1665, -1665,   557,  1215, -1665, -1665,   209, -1665,
   -1665,  2334,  3500, -1665,   150,  7427,   334,   334, -1665,   334,
     150,  7427, -1665, -1665, -1665, -1665, -1665, -1665, -1665, -1665,
    1980,  1980,  1980,  1517, -1665,   838, -1665,  8551, 12609, 12609,
   10367, 12609, 12609, 12609, 12609, 12609, 12609, 12609, 12609, 12609,
   12609, 12609, 12609, 12609, 12609, 12609, 12609, 12609, 12609,  2342,
     269,  1169, -1665, -1665, -1665, -1665, -1665, -1665, -1665, -1665,
   12332, -1665,  1870, -1665,  1176,  1170,  9045,  1182,  1189,  1223,
    7296,  1226,  1229,  1231, -1665,  1202, -1665, -1665,  1205,  1252,
   -1665, -1665, 13231,  1253,   763,   860,    73,   452, 12332,  1254,
   -1665,  1258,  1206, -1665, 13231, 13231, 13231, -1665, -1665,  1264,
    8437,  8489,  7007,  8567,  8716,  3441,  4706,  3970,  3970,  3970,
    2611,  2611,  1370,  1370,   976,   976,   976, -1665, -1665,  1216,
    1224,  1225,  1230,  1241,  1242,  8439,   636, -1665, 10275, 12332,
   -1665, -1665, -1665, 12332, -1665, -1665,  1245, 12977,  1243,  1248,
    1278,  1306, -1665, 12332, -1665, 12332, -1665, 12332,   718,  4224,
   -1665, -1665,  4224, -1665,   114,  1256,  1262, -1665,  1250,  8439,
     150, -1665,   150,  4746, -1665,  7427, 11195,  1263,  1268,  8990,
    8990,  6685,  1277, 12424,  1282,  2725,  3452,  3357,  1711,  1286,
    2544,  1292, 12793, -1665,  1260,  1325, -1665,   838, -1665, -1665,
   -1665, -1665, -1665, -1665,  3382,  6439, -1665,  8439, -1665, -1665,
    1314, 12609, -1665, -1665, -1665, -1665, -1665, -1665, -1665, -1665,
   -1665, -1665,   318, -1665, -1665,   453,  1344, -1665,   110, -1665,
    3089,  3566,  3566,  3691,  3691,  5431,  4458,   289,  2338, -1665,
    3722,  3716, 11687, 11687,  9343,   338,  1297,   424,  1470, -1665,
   10275,  9812, -1665,  5102,  1177,  1177,  2186, -1665, -1665, -1665,
    1343, -1665, -1665, -1665, -1665, -1665, -1665,  2435, -1665,   925,
     566, -1665, 12332,  8249,  2267, -1665,  2267,   374,   374,    87,
     740,  2820,  7204,   102,  4450, -1665,   277,   374, -1665, -1665,
    1308,   838,   838,   838, -1665,  1313,   150,  7427,   150,  7427,
   -1665, -1665, -1665, -1665, -1665, -1665,  8551,  8551,  1351,  3892,
    4179,  5970,  6556,  6290,  4077,  5416,  4391,  4391,  2677,  2677,
    1377,  1377,  1046,  1046,  1046, -1665, -1665,   999, -1665, -1665,
   -1665, -1665, -1665, 13231, -1665, -1665, -1665,  8439, -1665, -1665,
    1340,  1342,  1346,  1349,  1133, -1665, -1665,  8759, 10275,  9907,
    1324, -1665, 12332, -1665, -1665, -1665, -1665, -1665,   508,  1334,
   -1665, -1665,  1335,   401,  1009,  1009,  1336,  1009, 12332, -1665,
   12977,  1439,   334, -1665,  1356,  1357,  1359, -1665,   718, -1665,
   -1665, -1665, -1665, -1665, -1665, -1665,   718, -1665,   150,  1371,
   -1665,  1337, -1665, -1665, -1665, -1665,  3848,  3848,  6106, -1665,
   -1665, -1665,   299,  1374, -1665, -1665, -1665, -1665, -1665, -1665,
   -1665,  8551, -1665, -1665, -1665,  1383, -1665,   529,   541,   604,
      93,   773, -1665, -1665, -1665, -1665, -1665,  9999, -1665,  3089,
    3566,  3566,  4308,  4308,  6402, -1665,   559,  3722,  3089,  1376,
     524,   562,   582,   686,   520, -1665, -1665, -1665, -1665, -1665,
   -1665, -1665,   305,  2498,  2498,   848,   848,   848, 10275, -1665,
   -1665,  2930, -1665, -1665, -1665, -1665,  7427, 13231,   247, -1665,
    1962, -1665,   488,   488, -1665, -1665, -1665, -1665, -1665,  7427,
     585,  1043, 12332,  1007, -1665,  1425, -1665, -1665, -1665,   270,
     370,  1085,  2948,   377,   374,  1429, -1665,   382,  1427,   488,
    4666, -1665, -1665, -1665,   488, -1665, -1665,  1437, -1665, -1665,
   -1665,  1394, -1665,  1395, 12609, -1665, -1665,  1402, 12332, 12332,
   12332, 12332,    94, 10275, -1665,  1449, -1665, -1665, 13231, 12332,
   -1665,   508, -1665, -1665, -1665, -1665, -1665, -1665,  1403, -1665,
    1475,   838, -1665, -1665, -1665,   150, -1665, -1665, -1665, 12332,
   -1665, -1665, -1665,  1383, -1665, -1665,   539, -1665, 12332,    94,
   -1665,  5310,  5310,  1365,  5634,   688,  5102, -1665,   848, -1665,
   10275, -1665,   150,  1409,   784, -1665,  1461,  1461,   150,  1418,
   12332, 12332,  8464,   488,  3785,   488,   488,  3946,   488,  6590,
   -1665, -1665,  5049,  1461,   150,   150, -1665, -1665,  8551, -1665,
    1431,  1436,  1440,  1445,  1870, -1665, -1665,  9247,  1499, -1665,
   -1665, 10275,  1422, -1665, -1665, -1665, -1665, -1665,   150,  1446,
    1443, -1665, -1665,  1452, -1665,  6253,  6253,  6183,  1000,  1000,
   -1665, -1665, -1665, -1665, -1665,  7427, -1665, -1665, -1665, -1665,
    8464,  8464, -1665,  1461,   696,  1045, 12332, -1665, -1665, -1665,
    1007,  1007,  1461,  1461,   844,  1461, -1665, -1665, -1665, -1665,
   -1665,   150,   150, -1665, -1665, -1665, -1665, -1665,  1128,   399,
    9135, -1665, -1665, -1665, -1665, 11306, -1665, -1665, -1665, -1665,
   -1665,  8357, -1665, -1665,  1000,   150,   150,  1455,   150, -1665,
   -1665, -1665, 12332, 12332,  8464,   488,   488, -1665, -1665, -1665,
    8676, -1665, -1665,  1870, -1665, -1665, -1665,   420, -1665, -1665,
   -1665,  1474,  1074,  1096, -1665, -1665, -1665, -1665, -1665, 12332,
    1484,  1503,  1510, 12056,   205,  1870,   287,   780, -1665, -1665,
   12148,  1565, -1665, -1665, -1665,  1522, -1665,  6937,  7625,  5261,
    1569, -1665, -1665,  1476,  1478,  1486, -1665, -1665, -1665, -1665,
   -1665,  8464,  8464, -1665,  1461,  1461, -1665, 10735, -1665, -1665,
   -1665, -1665, -1665, -1665, -1665,   787,   787,  1538,  1516,  1520,
    4506, -1665, -1665, -1665, -1665,  1558, 12332,  1562,  1561,  1576,
    2456,  2485, -1665, -1665, -1665, -1665,  1544, -1665, -1665,  1007,
    1135, -1665,  1137,  1007, 12240,  1160, -1665, -1665, -1665, -1665,
   -1665, -1665,   150, -1665, -1665, -1665, -1665,  1535,  8807,  1545,
   -1665, 11780, -1665, -1665, -1665,  1630, -1665,  9438, 11780, 12332,
   -1665, -1665, -1665,  1589, -1665, -1665,  1597, -1665,  1576,  2456,
   -1665, -1665,  1644, -1665, -1665, 12885, 12885, 10091, -1665,  1538,
   -1665, -1665, -1665,   425,   318, -1665,  1547,   259,  3345,  1538,
    1538, -1665, 11405,    94, -1665, -1665,  1599,  1553, 13209, -1665,
   -1665, -1665, -1665, -1665,  1383,   180, -1665,   197, -1665, -1665,
    1007, -1665, -1665,   789, -1665, -1665, 10183, -1665, -1665, -1665,
   -1665,  1383,   104,   104,  1603, -1665, -1665, -1665,   488, -1665,
   12332,  1605, -1665,  1610, -1665,  1538, -1665,  1581,  1870,   812,
    1614, -1665,   428, -1665,  1611,  1578, -1665, -1665, -1665, -1665,
   12332,  1573,  1677,  1629,   104,  1677,   104,  1631, -1665, -1665,
   10459,  1583,  1683, -1665,   436, -1665, -1665,   439,   822, -1665,
   10275,  1587, -1665,  1615,  1693,  1650,  1655,  1677,  1657, -1665,
   -1665, 12332, -1665, -1665, -1665,   441, -1665,  1538,  1617,  1662,
   -1665, -1665, -1665
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
   -1665, -1665,  1736, -1665,  -340,  1551,  -397,    23,    -5,  1734,
   -1665,  1702, -1665, -1665, -1665, -1469, -1665,   510, -1665, -1461,
   -1665,    95,   992,    38,  -388, -1665, -1665,   106, -1665,  -710,
   -1665, -1665,   675,    51,  1574,  1294,  1592, -1665,    46,  -178,
    -787, -1665,   -10,    41, -1665, -1665, -1665, -1665, -1665,   610,
   -1665, -1665, -1665, -1665, -1665, -1665, -1665, -1665,   340,   -16,
   -1665, -1665, -1665, -1665, -1665, -1665, -1665, -1665, -1665, -1665,
   -1665,  1666,  -735,  7678,  -173,   -68,  -596,  -197,   -46,  1624,
    -582, -1665, -1665, -1665,   213, -1665,   136, -1665, -1582, -1665,
   -1369,   990,  -133,  -334, -1665,  -956,  7556,  4624,  6839,  1207,
    5249,  1460,  -369,   -56,   -62,  1261,  -152,   -63,   135, -1665,
   -1665, -1665,  -347, -1665, -1665, -1665, -1490,     8,  -368,  1537,
      19,     9,  -149,    16,    61,  -216, -1665, -1665, -1665,    -3,
    -132,  -177,  -179,    -8,   -32,  -266, -1665,  -408, -1665, -1665,
   -1665, -1665, -1665,   519,  1341,  2792, -1665,   717, -1665, -1665,
   -1345,  -453,   978, -1665, -1665, -1665, -1665,    58, -1665, -1665,
   -1665, -1665, -1665, -1665,  1144,  -361, -1665, -1665, -1665, -1665,
   -1665, -1665,   451,   650, -1665, -1665, -1665,   427, -1072, -1665,
   -1665, -1665, -1665, -1665, -1665,   643, -1665,   339,  1172, -1665,
     859,  1110,  3025,    98,  1609,  3509,  1392, -1665,  -520, -1665,
       4,  2057,   799,  -134,   171,  -104,  5763,  1447, -1665,  6767,
    2563,  1198,   -18,  -121, -1665,  1689,   -50, -1665,  6298,  3922,
    -246, -1665,  1816,  1842, -1665, -1665,   282, -1665, -1665,   346,
    1167, -1665, -1410, -1665, -1665, -1665, -1563, -1665, -1447,   105,
   -1665, -1665, -1665, -1665, -1665, -1665, -1665, -1665, -1665, -1665,
   -1665, -1665, -1665, -1665,    91, -1665, -1665, -1665, -1665, -1665,
      96, -1371, -1665, -1665,   -57, -1665, -1665, -1665, -1665,  -893,
   -1536, -1665,    45, -1664,  -804,  -160,  1002, -1665, -1665, -1665,
   -1665,  -409, -1665,  -405,  -183, -1665,    84, -1665, -1665,  1062,
     400, -1665,   153, -1665,  3186,  -156,  -756
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -931
static const short yytable[] =
{
     108,    39,   525,    65,   464,   466,   465,   128,   121,  1094,
     773,   408,   453,   732,   774,    61,   266,  1178,   741,   190,
     110,   269,    62,   441,   444,    60,   804,   879,   934,    38,
     906,   765,   315,    39,   667,    65,   417,   182,   495,   185,
     742,   549,  1096,  1085,    65,   728,   730,    61,   260,   753,
    1230,   322,  1491,   323,    62,   243,    61,    60,   772,   218,
    1494,    38,  1079,   184,   790,   486,   183,    63,   205,   524,
     177,   547,  1339,   257,  1341,   544,  1602,   415,   416,   533,
     407,  1160,  1371,   414,  1604,   178,   313,  1165,   773,   269,
      88,   276,   830,  1580,  1425,   532,   534,   601,   179,    63,
     427,  1506,  1507,   217,    76,   186,  1727,   557,    63,   171,
    1654,   381,    47,  1344,   404,  -879,  1767,   331,  1528,  1075,
     863,  1797,    88,   231,   732,   623,   624,   808,  1474,   144,
      -2,   149,   154,   250,  1076,   836,    76,   251,  -155,   423,
     436,  1368,   883,  1815,    47,    76,  1736,   172,  1345,   269,
     312,    61,  1346,    47,  -138,  1426,  1741,  1742,   422,   561,
       4,   421,   473,   461,  1764,   601,  1668,   182,  1561,   185,
      65,   163,  1297,    -3,    65,   378,   809,  1567,  1568,   810,
    1569,   218,    61,   420,  1086,  1347,    61,   314,    94,    62,
     528,   205,    60,   184,   425,   492,   183,  1724,  1755,  1768,
     177,  1199,  1778,    63,  1202,  1475,  1671,   166,   102,   116,
     117,   566,   382,  1369,   837,   178,  1135,  1772,  1060,    96,
     381,  1298,  -139,   100,   960,   217,  -139,  1684,   179,   779,
     780,   745,   543,   545,    63,   186,  1752,   390,    63,    97,
      76,  1259,   391,  1701,  1730,  1753,   101,   555,  1136,    47,
    1798,   741,   855,  1602,  1820,   440,   443,    88,  -139,   474,
     862,  1604,  -139,   276,   118,   119,  1061,   225,   226,    10,
      11,    76,  1400,   742,    16,    76,   231,   114,   625,  1675,
    1676,   691,  1750,    47,   378,   231,   231,   453,  1646,   115,
    1754,    20,   163,   163,   163,   474,  -399,    22,    17,  1684,
     506,   509,   342,    98,    91,   761,    25,   808,   717,  1684,
    1684,   257,    98,   637,   712,   231,   704,   381,    28,  1779,
     711,   382,   628,    99,   136,   137,   692,   365,   166,   166,
     166,  -399,    99,   135,    61,  -399,   141,   648,  -636,   163,
    1372,   422,  -329,    28,   627,   146,  1649,   805,  1282,  1283,
      92,   756,    93,   835,  1745,  1684,   809,  1444,   381,   810,
    -873,  1419,   453,  1381,  1277,  1383,   626,  1436,  -399,   144,
     313,   231,   749,   869,   169,   166,   861,   441,   444,   257,
    1491,   378,   781,  -636,  -636,   637,    63,  -336,  1373,   145,
    1371,   276,   532,   534,  1091,  1092,  -400,   742,    17,  -636,
     824,   532,    17,  -131,  1074,    17,  -451,  1684,  -129,  1420,
      17,  -189,   502,  -189,  1064,  1437,   771,   534,   382,   218,
     170,  -613,   378,    76,   393,  -613,   252,   187,   935,   776,
     253,  -400,  -336,  -336,   312,  -400,   529,  -451,  -131,   231,
    -451,   559,  -131,  -129,    97,  1483,   195,  -129,  -329,   825,
     717,   191,   826,   792,  1286,   185,  1287,   870,   871,   382,
      65,  1041,  1575,   691,   745,  1385,  1386,   196,  -400,   534,
     223,   314,    61,  -661,  -613,  -131,  -613,  -613,  -613,   184,
    -129,   231,   183,  1575,  1785,  -451,   560,   218,   931,  -613,
     257,  -613,   867,  1786,   771,   835,  1280,   814,  1804,   820,
     822,  1804,  -144,  1804,   866,   945,  -613,  -613,   692,    28,
    1576,  1403,  1288,  1289,  -140,  -140,    42,  -154,  -661,  -661,
      20,   186,  -613,   254,    63,   133,   134,   737,   146,   321,
     868,  1631,  1443,   197,  -661,  1330,   273,  -338,  1787,    12,
    1286,   316,  1287,   870,   871,  1449,  1805,  -144,    42,  1806,
     231,  1819,  -338,   557,   539,  1231,  -338,  1007,   540,   999,
     144,    76,   905,   327,  1000,    20,   388,  1002,  1003,  -314,
     133,   134,   738,  -338,  1008,  1294,    23,    28,   198,   199,
     136,   137,   231,   231,  1087,   392,  -338,  -338,  -144,  -338,
    -338,   231,   447,   448,   602,    28,   316,   395,  1288,  1289,
     232,   233,  1009,  1399,   603,  1059,   449,   231,  -635,   844,
     845,  -315,  1090,   397,  -314,  -314,   450,   879,  -338,  -338,
     478,    35,    28,   418,   276,   136,   137,   451,   197,   428,
    -314,  -313,  -329,  -144,  -338,   937,   717,   479,   446,   381,
     950,   950,   950,   950,   394,   704,   163,   163,   163,   969,
    1450,   761,   381,  -635,  -635,   176,  -315,  -315,  1070,   231,
     396,   428,   454,   381,    99,   480,   773,  1315,  1317,  -635,
     774,  -656,  -315,   198,   520,   894,  -313,  -313,   456,   612,
     146,   835,   166,   166,   166,  1007,   231,   765,   741,  1132,
    1133,   453,  -313,  1020,   609,  1137,   610,    -8,   381,   313,
    1069,  1557,  1008,   378,   378,   378,   378,   378,  1175,   470,
     742,  1067,  1044,   172,   772,   468,   378,   771,   551,   667,
     895,  1545,   133,   134,   381,   495,   381,   378,   492,   532,
    1009,   495,   469,   857,   601,  -662,   472,  -338,   428,   276,
     382,   382,   382,   382,   382,   128,   482,  1072,   558,  1312,
      20,   133,   134,   382,   538,   269,  1080,  1045,  1046,  1022,
     408,  1562,   378,   312,   382,  1394,  1396,   717,   146,   548,
    1024,   742,   428,   557,    28,   568,  1026,   136,   137,   822,
    -662,  -662,  -338,  -338,  1545,   570,   969,  1122,   378,  1348,
     378,   718,   133,   134,  1726,   576,  -662,   598,  -332,   382,
     314,   719,   551,    28,  1023,    80,   136,   137,   133,   134,
     771,   879,   720,   721,  1150,  1025,  1029,  1032,   492,    96,
    1011,  1027,  1427,   276,   552,   382,   612,   382,  -152,   231,
    1012,    98,  1123,  1504,  1349,   621,  1680,    80,  1759,    97,
    1161,  1162,  1013,  1163,  1396,  1652,    80,   136,   137,   638,
     135,    99,    10,    11,   641,   947,   146,  1545,   643,   210,
      28,   222,   642,   136,   137,   836,   644,  1428,   645,   553,
    1276,   122,   123,   124,   646,  1439,  1232,  1782,  1505,    65,
      20,  1681,   771,  1760,   978,   495,   663,  1807,   771,   664,
    1244,    61,  1245,   892,  1246,   232,   467,  1801,  1152,   757,
      98,   745,   758,   453,    28,  1144,  1294,   136,   137,  1101,
    1102,  1001,   716,  1197,  1284,   609,    98,   610,  1018,   231,
      99,  1323,  1783,  1147,    28,  -153,    97,   125,   126,  1273,
    1722,  1324,  1808,    17,  1038,  1545,    99,  -451,   893,    99,
    1480,   424,  1325,    63,  1124,  1125,   784,   429,   163,   787,
    1756,  1331,  1332,  1333,   453,   795,   163,   518,   519,   102,
     103,   104,  1314,    -7,  1376,  1377,  1378,  -451,  -451,   441,
     444,  -451,    80,  1545,  1335,   788,    80,   797,   441,   444,
      76,    98,   210,   222,   166,   806,    17,  1501,  1404,  1405,
    -451,  1407,   166,   693,   102,   103,   104,   807,   235,   236,
     237,    99,   950,   823,    10,    11,  1233,   495,   231,   495,
    1545,  -872,    28,   133,   134,   105,   106,   107,   561,  1336,
    -451,  -451,   834,    20,   410,   411,   412,    12,  1547,   238,
     832,   210,    20,   694,   695,    17,   950,   696,   697,   698,
     699,   851,   771,   864,   749,   269,   865,    28,   492,   717,
     105,   106,   239,  1172,  1173,  1174,    28,  -189,  1072,   136,
     137,   276,   689,   690,    23,    28,   378,   888,   136,   137,
     886,   263,   771,  1495,   950,   887,  1150,  1295,  1150,   889,
     274,   264,  1011,  1496,  1150,   896,  1150,   897,   261,    10,
     262,    12,  1012,  -878,  1497,   721,   240,   241,   242,    96,
     378,    96,   771,   382,  1013,   932,   526,   527,  1451,    35,
    1563,   492,  -196,   518,   793,  1342,  1343,   526,   794,    97,
     950,    97,   276,   978,   629,  1374,   146,  -196,    23,  -196,
      96,   940,   927,   928,   941,   263,   954,   382,   378,  1633,
     942,    65,   773,    65,    30,   264,  1615,  1029,  1032,   218,
      97,    65,    98,    61,  1019,    61,   122,   123,   124,  1354,
    1152,  1634,  1152,    61,   771,  1340,   771,  1144,   265,  1144,
    1152,   956,    99,    35,   957,   382,   495,  1144,   328,   133,
     134,    10,    11,   960,   378,  1147,   256,  1147,   485,   495,
    1573,  1574,  1353,  1363,  1021,  1147,  1411,   518,  1703,   526,
    1704,  1300,  1301,  1395,   950,    63,  1039,    63,  1041,    20,
    1300,  1301,   125,   126,  1486,    63,   328,    10,    11,  1062,
     210,   382,   518,  1708,  1063,   329,  1236,  1066,  1068,   429,
    1073,    22,  1077,    28,   136,   330,   136,   137,   163,   163,
     163,   836,    76,  -332,    76,  1078,  1081,   163,   163,   163,
    1323,  1088,    76,  1089,   273,   411,   412,    12,   265,  1261,
    1324,  1093,    80,   329,  1131,   771,  1099,  1120,   378,  1127,
    1128,  1325,   136,   330,   166,   166,   166,  1134,   163,  1198,
    1206,  1395,   271,   166,   166,   166,  1489,  1205,   210,   817,
     210,   210,  1208,   266,    23,  1493,   828,   161,   269,  1209,
    1210,   771,  1150,  1211,   429,   382,  1212,    28,  1213,   271,
     274,    31,  1214,  1216,   166,  1215,  1221,   210,  -151,  1218,
    1219,  1453,   531,   225,   226,   210,  1223,  1455,  1456,  1222,
      16,  1453,  1458,   771,  1224,  1456,  1241,   271,  1225,    35,
    1238,  1226,   398,   399,   400,   495,   771,  1809,  1354,   836,
     271,  1227,  1228,    22,  1242,  1243,  1240,   261,   133,   134,
      12,   429,    25,  1253,  1029,  1032,  1251,    65,   328,    10,
      11,  1274,  1252,  1262,   148,   148,   -57,   164,  1263,    61,
     -57,  1353,   102,   116,   117,  1130,  1152,  1264,  1275,   566,
     538,   -57,  1265,  1144,  1150,    28,  1269,    23,   402,   403,
    1431,  1432,  1271,   221,   263,  1296,   230,  1310,  1295,  1329,
     271,  1147,   247,    30,   264,   329,  1384,   258,  1375,   442,
     445,  1124,  1125,  1379,   136,   330,   210,   133,   134,  1662,
    1660,    63,  1665,   950,  1388,  1397,  1389,   265,   118,   119,
    1390,   630,    35,  1391,  1401,  1402,   271,  1410,   265,  1406,
    1418,  1408,   208,   686,   687,   688,   689,   690,  1537,    65,
     924,   925,   926,   927,   928,   161,  1412,  1413,    76,  1414,
     485,    61,   717,  1311,    10,    11,    12,   271,  1152,    28,
    1422,  1417,   136,   137,  1421,  1144,  1435,  1454,  1565,  1566,
     430,  1459,  1460,  -702,   771,   430,   718,   378,   439,   439,
     164,   601,   771,  1147,  1466,  1467,   719,   224,   225,   226,
     342,   740,  1469,    23,  1481,    16,  1484,   720,   721,  1503,
     328,   133,   134,    63,  1485,   221,    28,   479,  1509,   274,
      31,   471,    20,  1474,   382,  1548,  1738,  1551,    22,  1632,
    1601,  1533,  1609,   505,   505,   514,  1534,    25,   771,  1641,
    1535,   102,   434,   435,    61,  1536,  1550,  1630,    35,   230,
      76,  1608,  1553,   777,  1607,  1619,  1642,   329,  1600,   535,
     210,   441,   444,  1643,  1655,   208,   136,   330,  1647,  1648,
     147,    10,    11,    12,   271,  1657,  1666,  1667,  1669,   147,
     133,   134,    12,  1431,  1432,    15,  1670,  1609,   230,  -930,
     565,  1470,  1471,  1472,  1473,   218,    63,   118,   106,    61,
    1687,   485,  1482,   562,  1688,  1659,  1608,  1659,    20,  1607,
      23,  1692,   658,  1600,   208,  1694,   717,   391,   485,    23,
     271,   528,  1695,    28,  1697,  1700,    30,    31,  1702,   429,
     225,   226,    28,    76,  1711,    30,    31,    16,  1721,  1713,
     811,   377,  1731,  1732,   230,   258,  1735,  1739,   429,    33,
     812,    63,  1747,  1748,    20,    35,  1773,   231,  1776,    34,
      22,   813,   721,  1777,    35,  1780,  1784,  1788,  1717,    25,
      36,  1157,   485,  1733,  1609,  1717,  1792,   485,  1789,  1793,
     485,   485,  1795,  1802,  1799,  1803,    61,  1810,    76,   224,
     225,   226,   439,  1608,   381,  1812,  1607,    16,  1758,  1811,
    1600,   442,   785,  1813,   328,    10,    11,  1601,  1814,  1609,
    1816,   485,   230,   258,    20,  1822,  1740,  1821,   485,   463,
      22,    61,   102,   103,   104,   488,     2,     7,  1608,    25,
     168,  1607,   312,  1717,   271,  1600,  1699,  1065,    63,   312,
    1278,   462,  1781,   410,    10,    11,    12,   791,  1605,  1737,
     530,   329,  1364,   221,   230,   460,  1492,   325,   378,   426,
     136,   330,  1683,   442,   445,   231,   608,  1270,   943,   314,
    1098,   430,  1441,    63,   430,    76,   314,  1334,   105,   106,
     164,   164,   164,    23,   565,  1366,   562,  1462,  1526,   163,
     263,  1204,   271,   844,   845,   382,  1645,   312,   944,   274,
     264,   485,  1006,   208,   102,   116,   117,   271,   521,   406,
      76,   907,  1579,   713,  1546,   485,   958,  1744,  1757,  1796,
    1751,   221,  1119,   230,   258,   166,  1500,     0,    35,     0,
     163,   163,   163,     0,   314,     0,     0,   840,     0,     0,
     840,     0,   843,   843,   514,     0,     0,   618,   471,  1693,
       0,  1446,  1447,     0,   859,     0,   535,     0,     0,   471,
     118,   119,   120,   102,   116,   117,   166,   166,   166,     0,
       0,   208,     0,   208,   208,     0,     0,     0,  1461,     0,
     535,   471,     0,  1463,  1715,     0,     0,   839,   660,   429,
     978,  1715,     0,     0,     0,     0,   839,     0,     0,     0,
     208,     0,   271,     0,     0,   442,   955,     0,   208,     0,
    1129,   707,     0,   273,   411,   412,    12,   658,   708,   118,
     119,     0,   271,     0,     0,  1605,     0,     0,     0,     0,
       0,     0,   535,  1157,     0,  1157,  1159,     0,     0,     0,
    1360,  1367,  1164,  1157,     0,     0,   770,     0,     0,  1715,
       0,     0,     0,    23,     0,  1350,  1351,    11,    12,   230,
     709,  1512,  1513,  1775,  1522,  1523,    28,  1525,     0,   274,
      31,     0,     0,     0,   133,   134,     0,     0,   658,   561,
     658,     0,  1054,  1791,   164,   164,   439,     0,     0,   485,
       0,     0,     0,   978,     0,    23,     0,   439,   710,   147,
     133,   134,    12,   485,     0,   485,   271,   485,    28,   208,
       0,    30,    31,     0,  1818,  1445,     0,  1352,     0,  1559,
    1560,   398,   399,   400,   770,   202,    28,     0,    20,   136,
     137,     0,     0,     0,     0,   203,     0,   271,     0,    23,
      35,     0,   485,   602,     0,     0,   204,     0,     0,  1028,
    1028,  1028,    28,   603,     0,    30,    31,     0,   439,     0,
     439,     0,  1055,     0,   161,     0,     0,     0,   401,   157,
       0,     0,  1116,  1623,  1624,  1625,     0,   402,   403,   158,
       0,   155,     0,     0,    35,     0,  1054,     0,   429,     0,
     159,   271,  1254,     0,  1255,     0,     0,   429,   442,   785,
       0,     0,   230,   247,     0,  1176,  1177,     0,  1179,  1180,
    1181,  1182,  1183,  1184,  1185,  1186,  1187,  1188,  1189,  1190,
    1191,  1192,  1193,  1194,  1195,  1196,     0,     0,     0,  1360,
    1673,  1674,     0,     0,   273,    10,    11,    12,     0,     0,
       0,     0,  1100,  1100,  1106,     0,   147,    10,    11,    12,
       0,     0,  1106,   208,     0,   273,   411,   412,    12,  1157,
       0,     0,     0,     0,     0,     0,   164,     0,     0,   948,
     949,   951,   952,   953,    23,   840,   840,   843,   843,   514,
      10,    11,   660,   859,     0,   561,    23,     0,     0,     0,
     274,    31,     0,   972,     0,    23,   431,   471,     0,    28,
       0,   433,    30,    31,     0,     0,   201,     0,    20,     0,
     725,   274,    31,   733,   736,     0,   202,     0,  1380,    35,
    1382,     0,   839,     0,     0,   839,   203,     0,  1004,     0,
       0,    35,    28,     0,     0,   136,   137,   204,     0,     0,
      35,   839,     0,  1521,   271,     0,   271,     0,  1521,  1323,
     155,  1157,     0,     0,  1047,     0,  1048,     0,  1138,  1324,
       9,    10,  1139,    12,   175,    14,    15,  1774,     0,     0,
    1325,     0,    16,     0,     0,     0,     0,     0,  1281,     0,
       0,   230,     0,     0,     0,     0,    18,     0,    19,    20,
      21,     0,     0,     0,     0,    22,   271,     0,     0,   271,
      23,   442,   955,     0,    25,  1140,     0,   176,     0,     0,
    1416,     0,     0,    28,     0,     0,    30,    31,     0,   563,
    1141,   816,  1142,   725,   733,   736,     0,   122,   870,   871,
      33,   261,    10,    11,    12,   102,   116,   117,   554,     0,
      34,     0,     0,     0,     0,    35,     0,     0,   133,   134,
       0,  1143,     0,    15,     0,   442,   445,     0,     0,     0,
    1248,     0,     0,  1248,   442,  1309,     0,     0,     0,     0,
       0,    23,     0,     0,  1256,    23,    20,     0,   263,     0,
    1055,  1055,  1055,   125,   126,     0,     0,    30,   264,     0,
       0,   118,   119,   717,   161,     0,   210,  1663,   210,   485,
      28,     0,     0,   136,   137,   247,     0,     0,   485,     0,
       0,   265,   488,   541,   103,   104,    35,   718,   488,   147,
      10,    11,    12,   224,   225,   226,     0,   719,   122,   870,
     871,    16,  1299,  1299,  1106,  1106,  1106,     0,   720,   721,
       0,  1308,     0,  1106,  1106,  1106,     0,     0,    20,   102,
     116,   117,     0,     0,    22,  1326,  1326,  1327,     0,    23,
     329,     0,     0,    25,     0,   717,     0,  1487,     0,   105,
     542,     0,    28,     0,   164,    30,    31,     0,   102,   116,
     117,    28,   839,   221,   125,   126,     0,   782,   839,   811,
     783,     0,    10,    11,  1502,     0,     0,    15,     0,   812,
    1508,     0,   786,     0,    35,  1696,   119,   210,     0,     0,
     821,   721,     0,     0,     0,     0,  1529,  1530,   442,   785,
      20,     0,     0,     0,  1030,  1033,     0,     0,     0,     0,
       0,     0,  1229,  1359,  1698,   119,     0,   102,   116,   117,
    1549,   746,   747,   748,    28,     0,     0,   136,   137,   271,
       0,     0,     0,   431,   442,   785,   433,     0,     0,    84,
       0,  1323,     0,     0,     0,     0,   972,     0,     0,   111,
     563,  1324,   488,     0,     0,     0,     0,     0,     0,   139,
       0,  1468,  1325,  1571,  1572,     0,   152,   152,     0,   152,
       0,    84,     0,   118,   119,     0,     0,  1055,  1055,  1055,
      84,     0,   770,   733,  1279,     0,     0,  1617,  1618,     0,
    1620,     0,     0,   212,     0,    84,     0,  1030,  1033,     0,
       0,   271,     0,     0,   248,     0,     0,     0,     0,   111,
       0,  1299,  1299,  1106,  1106,  1106,     0,     0,  1308,     0,
     279,     0,   111,     0,   485,   271,     0,     0,     0,     0,
    1320,     0,     0,     0,  1438,  1438,  1327,   410,    10,    11,
      12,     0,     0,     0,     0,     0,   111,     0,     0,     0,
       0,     0,     0,   471,   471,     0,   410,   133,   134,    12,
       0,   839,   684,   685,   686,   687,   688,   689,   690,     0,
     839,     0,     0,   139,   488,    84,   488,    23,     0,     0,
     471,   152,   152,     0,   929,   471,   432,   152,   839,   839,
     152,   152,   152,   274,   264,     0,    23,     0,   273,   133,
     134,    12,  1359,   263,  1710,     0,    84,     0,     0,     0,
      84,     0,   274,   264,  1387,     0,   212,    84,   147,   133,
     134,    12,    35,     0,   256,     0,     0,    20,   922,   923,
     924,   925,   926,   927,   928,   212,   212,   212,    23,     0,
       0,    35,  1498,  1498,   717,  1499,     0,    20,     0,     0,
       0,    28,     0,     0,   274,    31,     0,     0,    23,     0,
       0,     0,   733,   471,   471,   212,   471,   471,  1266,   471,
       0,    28,     0,     0,    30,    31,     0,     0,  1267,     0,
       0,     0,   546,    35,     0,     0,     0,     0,    33,  1268,
     721,     0,   111,  1350,  1351,    11,    12,     0,    34,     0,
     839,     0,     0,    35,     0,   152,  1554,  1554,  1554,    36,
       0,     0,     0,     0,     0,  1249,  1520,     0,  1250,     0,
       0,   471,   471,     0,     0,     0,   224,   225,   226,  1257,
     234,     0,     0,    23,    16,  1030,  1033,     0,     0,   155,
    1247,     0,     0,   488,     0,     0,    28,   111,   604,    30,
      31,    20,     0,   619,     0,  1352,   488,    22,    84,     0,
     839,   839,     0,   202,     0,     0,    25,  1247,     0,     0,
       0,   782,   783,   203,     0,   471,   471,   471,    35,   786,
       0,     0,   938,     0,   204,     0,     0,  1249,  1250,  1030,
    1033,     0,     0,     0,     0,  1116,  1257,     0,  1030,  1033,
       0,     0,     0,   122,   870,   871,   111,   872,     0,   714,
     234,   604,  1247,     0,   604,   734,   839,     0,  1247,   221,
     230,   273,   133,   134,    12,     0,   234,     0,   147,   133,
     134,    12,   471,   471,    15,     0,     0,   442,  1309,   873,
       0,     0,   139,  1320,     0,     0,     0,   410,    10,   564,
      12,     0,   111,     0,   212,   111,    28,    20,     0,   125,
     126,    23,     0,   152,     0,     0,     0,     0,    23,   208,
       0,   208,     0,   152,     0,   514,   152,   274,    31,     0,
       0,    28,     0,   234,    30,    31,     0,    23,   152,     0,
       0,     0,   234,     0,   263,     0,    84,     0,   157,     0,
       0,     0,     0,   274,   264,     0,    35,     0,   158,   234,
       0,     0,   488,    35,     0,     0,   843,   843,   843,   159,
     234,     0,   212,   818,   212,   212,   734,   565,   230,     0,
     818,     0,    35,     0,     0,     0,     0,     0,   212,   212,
       0,     0,   212,     0,   212,   212,   212,   849,     0,     0,
       0,   212,  1030,  1033,     0,     0,   212,     0,   770,   212,
    1247,     0,   147,    10,    11,    12,     0,     0,  1247,   471,
     147,    10,    11,    12,   175,    14,    15,     0,     0,     0,
     208,   272,    16,     0,     0,     0,  1249,  1250,  1030,  1033,
       0,    20,     0,  1257,     0,   152,    18,     0,    19,    20,
      21,     0,    23,     0,     0,    22,     0,     0,   717,     0,
      23,  1247,     0,     0,    25,    28,     0,   176,    30,    31,
    1247,     0,     0,    28,     0,     0,    30,    31,   431,   433,
       0,     0,   811,     0,     0,     0,     0,   563,     0,   903,
      33,     0,   812,   410,   133,   134,    12,    35,     0,     0,
      34,     0,     0,   813,   721,    35,     0,     0,     0,     0,
     212,    36,   455,     0,     0,     0,    37,     0,     0,   147,
      10,    11,    12,   224,   225,   226,   111,   111,   111,   111,
       0,    16,     0,    23,     0,     0,     0,     0,     0,     0,
     263,     0,   234,     0,     0,   504,     0,     0,    20,   274,
     264,   234,     0,     0,    22,     0,     0,     0,   523,    23,
       0,     0,     0,    25,   660,   717,     0,  1249,  1250,     0,
    1257,     0,    28,   565,     0,    30,    31,     0,    35,     0,
       0,     0,     0,   234,     0,   111,     0,   604,     0,   811,
       0,     0,     0,     0,     0,     0,     0,     0,   714,   812,
       0,   604,   604,   734,    35,     0,   147,    10,    11,    12,
     813,   721,    15,     0,  1056,     0,     0,   234,  1058,     0,
       0,     0,     0,     0,     0,     0,   234,     0,     0,     0,
       0,  1030,  1033,     0,     0,    20,     0,     0,   139,     0,
       0,     0,     0,     0,     0,   139,    23,   234,     0,     0,
       0,     0,   717,     0,   212,   248,     0,     0,     0,    28,
    1247,  1247,    30,    31,     0,     0,     0,     0,   147,    10,
      11,    12,     0,     0,   234,     0,   811,   782,   783,     0,
     739,    10,    11,    12,   786,     0,   812,     0,     0,     0,
       0,    35,     0,     0,   212,   212,  1108,   813,   721,     0,
       0,     0,  1111,     0,  1108,   102,   116,   117,    23,   235,
     236,   237,     0,     0,     0,     0,  1247,   342,   740,     0,
      23,    28,     0,   212,    30,    31,   849,   212,   212,   849,
     849,   849,     0,    28,    20,   212,   136,   137,   202,     0,
     238,     0,   212,     0,     0,     0,     0,     0,   203,     0,
       0,     0,     0,    35,     0,     0,     0,     0,     0,   204,
       0,   118,   119,     0,     0,    84,     0,     0,   111,     0,
       0,     0,     0,     0,   111,   410,    10,    11,    12,     0,
       0,     0,     0,   604,   604,   604,     0,   569,   571,   572,
     573,   574,   575,     0,   577,   578,   579,   580,   581,   582,
     583,   584,   585,   586,   587,   588,   589,   590,   591,   592,
     593,   594,   595,   596,   597,    23,   599,   600,     0,     0,
       0,  1138,   263,     9,    10,  1139,    12,   175,    14,    15,
       0,   274,   264,   604,     0,    16,   678,   679,   680,   681,
     682,   683,   684,   685,   686,   687,   688,   689,   690,    18,
       0,    19,    20,    21,     0,   265,     0,     0,    22,  -556,
      35,   852,     0,    23,     0,     0,   858,    25,  1140,     0,
     176,     0,     0,     0,     0,     0,    28,     0,     0,    30,
      31,     0,     0,  1141,     0,  1142,     0,     0,   111,   147,
      10,    11,    12,    33,     0,   256,     0,   884,     0,     0,
       0,     0,     0,    34,   890,     0,     0,     0,    35,     0,
       0,   604,   604,     0,  1143,   604,     0,     0,    20,     0,
       0,     0,   111,     0,     0,     0,   604,     0,   111,    23,
       0,  -556,  1056,  1056,  1056,   717,     0,     0,   604,     0,
    1111,     0,    28,     0,     0,    30,    31,   900,   901,     0,
     902,     0,   234,     0,     0,   234,     0,     0,   111,   811,
     111,     0,     0,     0,     0,   739,    10,    11,    12,   812,
       0,   234,     0,     0,    35,     0,     0,     0,     0,     0,
     813,   721,     0,   212,   212,   212,   212,   212,  1108,   849,
       0,     0,     0,   212,     0,  1108,  1108,  1108,     0,     0,
       0,  1111,   342,   740,     0,    23,   111,   849,   849,   849,
       0,     0,     0,     0,   147,    10,    11,    12,    28,     0,
      15,   136,   930,   139,     0,     0,   152,    84,     0,    84,
       0,   507,   510,     0,  1361,    84,     0,    84,     0,   410,
      10,    11,    12,    20,     0,   147,    10,    11,    12,     0,
     111,   561,   111,     0,    23,     0,     0,     0,     0,     0,
     717,     0,     0,     0,     0,     0,     0,    28,  1042,  1043,
      30,    31,     0,     0,    20,     0,     0,  1042,     0,    23,
       0,     0,   898,     0,  1103,    23,   263,     0,     0,     0,
     111,   717,     0,     0,  1104,   274,   264,     0,    28,    35,
       0,    30,    31,     0,   899,  1105,   721,     0,  1514,  1515,
      11,    12,     0,   904,     0,   811,     0,   604,   604,   565,
     604,     0,     0,     0,    35,   812,     0,     0,     0,     0,
      35,   604,     0,     0,     0,     0,   813,   721,     0,   604,
     234,     0,     0,     0,     0,     0,     0,     0,    23,   604,
     604,   734,     0,     0,     0,     0,     0,     0,     0,     0,
    1110,    28,     0,     0,    30,    31,     0,   234,     0,     0,
    1516,   273,   133,   134,    12,     0,     0,    15,   202,     0,
       0,     0,   212,   212,   212,   849,   849,  1433,   203,     0,
     212,   212,     0,    35,     0,  1126,     0,     0,     0,   204,
      20,     0,     0,     0,     0,     0,   849,   849,   849,   849,
     849,    23,   234,     0,     0,     0,     0,   717,   234,   111,
       0,     0,     0,  1361,    28,     0,     0,   274,    31,     0,
       0,     0,   111,     0,     0,     0,     0,  1166,     0,  1168,
       0,  1266,     0,     0,     0,  1457,     0,     0,    87,     0,
       0,  1267,     0,    84,     0,     0,    35,     0,   113,     0,
       0,     0,  1268,   721,     0,     0,     0,   132,   140,   410,
     133,  1524,    12,     0,     0,   153,   153,     0,   153,     0,
      87,     0,   912,   913,   914,   915,   916,   917,   918,    87,
     919,   920,   921,   922,   923,   924,   925,   926,   927,   928,
       0,     0,   153,     0,    87,     0,     0,     0,     0,    23,
       0,     0,     0,   249,   849,   849,   263,   849,   259,   111,
       0,   849,     0,     0,     0,   274,   264,     0,     0,     0,
       0,   259,     0,     0,     0,   841,     0,   212,   842,     0,
     507,   510,   152,     0,     0,    84,     0,     0,     0,   565,
       0,     0,   860,     0,    35,     0,  1234,  1235,     0,  1237,
     234,     0,     0,     0,     0,     0,     0,     0,   234,   682,
     683,   684,   685,   686,   687,   688,   689,   690,   849,   849,
    1433,   849,   849,     0,    87,     0,     0,     0,   111,  1258,
     153,   153,     0,     0,     0,     0,   153,     0,     0,   153,
     153,   153,  1170,  1171,     0,     0,     0,     0,     0,     0,
       0,   234,     0,     0,     0,    87,     0,     0,     0,    87,
     234,     0,     0,     0,     0,   153,    87,     0,     0,     0,
       0,     0,     0,     0,   111,     0,     0,   849,   234,   234,
       0,     0,     0,     0,   153,   153,   153,     0,     0,     0,
       0,     0,     0,     0,  1307,     0,     0,   280,     0,     0,
      10,    11,     0,     0,    14,    15,     0,     0,     0,     0,
       0,    16,   917,   918,   153,   919,   920,   921,   922,   923,
     924,   925,   926,   927,   928,    18,     0,    19,    20,     0,
     212,   818,   212,     0,    22,     0,   281,   282,     0,     0,
       0,     0,     0,    25,     0,   283,     0,     0,     0,     0,
       0,     0,    28,     0,   153,   136,   137,     0,   284,     0,
       0,     0,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   298,   299,   300,   301,   302,
     303,   304,     0,   305,   306,   307,   308,  1108,   133,   134,
     234,   309,   310,   256,     0,     0,   259,   606,     0,     0,
       0,     0,   620,     0,     0,     0,     0,    87,   311,     0,
     913,   914,   915,   916,   917,   918,    20,   919,   920,   921,
     922,   923,   924,   925,   926,   927,   928,     0,  1108,  1108,
    1108,     0,     0,   717,     0,     0,     0,  1415,     0,     0,
      28,   212,     0,   136,   137,     0,     0,     0,     0,     0,
     234,   234,     0,     0,     0,   259,     0,   718,   715,     0,
     606,     0,     0,   606,   735,     0,     0,   719,     0,   744,
       0,   273,    10,    11,    12,     0,     0,    15,   720,   721,
     507,   510,     0,     0,     0,     0,     0,     0,     0,     0,
    1434,   762,     0,     0,     0,     0,     0,     0,     0,     0,
      20,   259,     0,   153,   259,     0,   234,     0,     0,     0,
       0,    23,   153,   841,   842,   507,   510,   717,     0,     0,
    1442,   860,   153,     0,    28,   153,     0,   274,    31,     0,
       0,     0,     0,  1448,     0,     0,     0,   153,     0,     0,
       0,  1302,     0,     0,     0,    87,     0,     0,     0,     0,
       0,  1303,     0,     0,     0,     0,    35,     0,     0,     0,
    1464,     0,  1304,   721,  1465,     0,     0,     0,     0,     0,
       0,   153,   819,   153,   153,   735,     0,     0,     0,   819,
       0,     0,     0,     0,     0,     0,     0,   153,   153,     0,
       0,   153,     0,   153,   153,   153,   606,     0,     0,     0,
     153,     0,  1488,     0,     0,   153,     0,     0,   153,     0,
       0,  1138,     0,     9,    10,  1139,    12,   175,    14,    15,
       0,   273,    10,    11,    12,    16,     0,     0,     0,   881,
     920,   921,   922,   923,   924,   925,   926,   927,   928,    18,
       0,    19,    20,    21,   153,     0,     0,     0,    22,  -557,
      20,  1531,  1532,    23,     0,     0,     0,    25,  1140,     0,
     176,    23,     0,     0,     0,     0,    28,   717,     0,    30,
      31,     0,     0,  1141,    28,  1142,     0,   274,    31,  1689,
       0,     0,     0,    33,     0,     0,     0,     0,  1555,  1556,
       0,  1302,     0,    34,  1558,     0,     0,     0,    35,     0,
       0,  1303,   744,     0,  1143,     0,    35,     0,     0,   153,
       0,     0,  1304,   721,     0,     0,     0,     0,     0,     0,
       0,  -557,     0,     0,     0,   259,   259,   259,   259,     0,
       0,  1690,   669,   670,   671,   672,   673,   674,   675,   676,
     677,   678,   679,   680,   681,   682,   683,   684,   685,   686,
     687,   688,   689,   690,     0,     0,     0,     0,   147,    10,
      11,    12,     0,     0,    15,     0,     0,     0,     0,     0,
     841,   842,   507,   510,     0,     0,     0,     0,     0,   860,
       0,   507,   510,     0,   259,     0,   606,    20,     0,     0,
       0,     0,     0,   841,   842,   860,     0,     0,    23,     0,
     606,   606,   735,     0,  1672,     0,     0,     0,     0,     0,
       0,    28,     0,  1057,    30,    31,     0,     0,     0,     0,
       0,     0,   147,   133,   134,    12,   744,  1138,   202,     9,
      10,  1139,    12,   175,    14,    15,     0,   762,   203,     0,
       0,    16,     0,    35,     0,   132,     0,     0,     0,   204,
       0,    20,     0,   153,   249,    18,     0,    19,    20,    21,
       0,     0,    23,     0,    22,  -559,     0,     0,     0,    23,
       0,     0,     0,    25,  1140,    28,   176,     0,    30,    31,
       0,     0,    28,     0,     0,    30,    31,     0,     0,  1141,
       0,  1142,    33,   153,   153,   819,     0,     0,     0,    33,
       0,     0,    34,  1118,     0,     0,     0,    35,     0,    34,
     133,   134,     0,    36,    35,   561,     0,     0,     0,     0,
    1143,     0,   153,     0,     0,   606,   153,   153,   606,   606,
     606,     0,     0,     0,   153,     0,     0,  -559,    20,     0,
       0,   153,   679,   680,   681,   682,   683,   684,   685,   686,
     687,   688,   689,   690,     0,   717,     0,     0,     0,     0,
       0,     0,    28,     0,    87,   136,   137,   259,     0,   841,
     842,   507,   510,   259,     0,     0,   860,     0,     0,   718,
       0,     0,   606,   606,   606,     0,     0,     0,     0,   719,
       0,     0,   507,   510,     0,     0,     0,     0,     0,     0,
     720,   721,     0,     0,     0,     0,     0,     0,     0,   483,
       0,   273,    10,    11,    12,   175,    14,   333,   334,   335,
     336,   484,   337,    16,     0,     0,     0,     0,     0,     0,
       0,     0,   606,     0,     0,     0,     0,    18,   338,    19,
      20,    21,     0,   339,   340,   341,    22,     0,   342,   343,
     344,    23,   345,   346,     0,    25,     0,     0,     0,   347,
     348,   349,   350,   351,    28,     0,     0,   274,    31,     0,
       0,     0,   352,     0,     0,     0,     0,     0,   353,     0,
       0,   354,     0,     0,     0,     0,     0,   259,     0,   355,
     356,   357,     0,     0,     0,     0,   358,   359,   360,     0,
     841,   842,   361,   860,   616,     0,     0,     0,     0,     0,
     606,   606,     0,     0,   606,     0,     0,     0,  -843,     0,
     362,   259,     0,   639,   640,   606,     0,   259,     0,     0,
       0,  1057,  1057,  1057,     0,     0,     0,   606,   649,   744,
       0,   650,   651,     0,     0,     0,     0,   661,   662,     0,
       0,     0,     0,     0,     0,     0,     0,   259,   706,   259,
       0,     0,     0,     0,   507,   510,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   881,     0,     0,
       0,     0,   153,   153,   153,   153,   153,   819,   606,     0,
       0,     0,   153,     0,  1118,  1118,  1118,     0,     0,     0,
     744,     0,     0,     0,     0,   259,   606,   606,   606,     0,
    1138,     0,     9,    10,  1139,    12,   175,    14,    15,   881,
       0,     0,   140,     0,    16,   153,    87,     0,    87,     0,
       0,     0,     0,  1362,    87,     0,    87,     0,    18,     0,
      19,    20,    21,     0,     0,     0,     0,    22,  -558,   259,
       0,   259,    23,     0,     0,     0,    25,  1140,     0,   176,
       0,     0,     0,  1319,     0,    28,    10,    11,    30,    31,
      14,    15,  1141,     0,  1142,     0,     0,    16,     0,     0,
       0,     0,    33,     0,     0,     0,     0,     0,     0,   259,
       0,    18,    34,    19,    20,     0,     0,    35,     0,     0,
      22,    10,    11,  1143,   175,    14,    15,     0,     0,    25,
     484,     0,    16,     0,     0,     0,   606,   606,    28,   606,
    -558,   136,   137,     0,     0,     0,    18,     0,    19,    20,
     606,     0,     0,     0,     0,    22,     0,     0,   606,     0,
       0,     0,     0,     0,    25,     0,   717,     0,   606,   606,
     735,     0,     0,    28,     0,     0,   136,   137,     0,     0,
       0,     0,     0,     0,   147,    10,    11,    12,     0,     0,
     718,     0,     0,     0,   507,   510,     0,     0,     0,     0,
     719,   153,   153,   153,   606,   606,   735,     0,     0,   153,
     153,   720,   721,    20,     0,     0,     0,     0,   616,     0,
       0,     0,     0,     0,    23,   606,   606,   606,   606,   606,
       0,     0,     0,   881,     0,    56,     0,    28,   259,     0,
      30,    31,  1362,     0,   147,    10,    11,    12,   224,   225,
     226,   259,     0,     0,   511,     0,    16,   649,   650,     0,
       0,     0,    56,    56,   512,   160,     0,    56,     0,    35,
       0,     0,    87,    20,     0,   513,    56,     0,     0,    22,
       0,     0,     0,     0,    23,     0,     0,     0,    25,    56,
     717,    56,     0,     0,    10,    11,     0,    28,     0,   256,
      30,    31,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   267,   202,     0,   275,     0,     0,     0,
       0,     0,    20,     0,   203,     0,     0,     0,   881,    35,
       0,     0,     0,   606,   606,  1664,   606,     0,   259,   717,
     606,     0,     0,     0,     0,     0,    28,     0,     0,   136,
     137,     0,     0,     0,     0,     0,   153,     0,     0,     0,
       0,   153,     0,  1495,    87,     0,     0,     0,   413,   413,
       0,    56,     0,  1496,     0,     0,     0,    56,    56,     0,
       0,   267,   275,    56,  1497,   721,   160,   160,   160,     0,
       0,     0,     0,   452,     0,     0,     0,   606,   606,   735,
     606,   606,    56,     0,     0,     0,    56,   259,     0,     0,
       0,     0,    56,    56,   147,    10,    11,    12,   175,    14,
      15,     0,     0,     0,   484,     0,    16,     0,     0,     0,
       0,    56,    56,   160,     0,     0,     0,     0,     0,     0,
      18,   267,    19,    20,     0,     0,     0,     0,     0,    22,
       0,     0,     0,   259,    23,     0,   606,     0,    25,     0,
     717,    56,     0,     0,     0,     0,     0,    28,     0,     0,
      30,    31,   918,     0,   919,   920,   921,   922,   923,   924,
     925,   926,   927,   928,  1103,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  1104,     0,     0,     0,  1651,    35,
       0,    56,     0,     0,     0,  1105,   721,     0,   267,   153,
     819,   153,   616,   616,     0,   616,   616,   616,   616,   616,
     616,   616,   616,   616,   616,   616,   616,   616,   616,   616,
     616,   616,   616,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     8,     0,     9,    10,    11,    12,    13,    14,
      15,     0,     0,     0,    56,     0,    16,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1118,     0,     0,    17,
      18,     0,    19,    20,    21,     0,     0,     0,     0,    22,
       0,     0,     0,     0,    23,     0,     0,    24,    25,    26,
     657,    27,     0,     0,     0,     0,     0,    28,    29,     0,
      30,    31,     0,     0,    32,   413,     0,  1118,  1118,  1118,
       0,     0,     0,     0,    33,   267,     0,     0,    10,    11,
     153,  1239,     0,   561,    34,     0,     0,     0,     0,    35,
       0,     0,     0,     0,     0,    36,     0,     0,   413,     0,
      37,     0,   147,   133,   134,    12,    20,     0,   561,     0,
      56,     0,     0,   649,   650,     0,     0,     0,     0,    56,
       0,   267,     0,   717,     0,     0,     0,     0,   452,    56,
      28,    20,    56,   136,   137,     0,     0,     0,   452,   452,
     452,     0,    23,     0,    56,   616,     0,  1495,     0,     0,
       0,     0,    56,     0,     0,    28,     0,  1496,    30,    31,
       0,     0,     0,     0,     0,     0,     0,     0,  1497,   721,
       0,     0,    33,     0,     0,     0,   649,   650,    56,    56,
      56,    56,    34,     0,     0,     0,    56,    35,     0,     0,
       0,     0,     0,    36,    56,    56,     0,     0,    56,     0,
     160,   160,   160,   452,     0,     0,     0,    56,     0,    82,
       0,     0,    56,     0,     0,    56,     0,     0,     0,     0,
       0,     0,     0,     0,   174,     0,   147,    10,    11,    12,
     175,    14,    15,     0,     0,     0,    82,    82,    16,    82,
       0,    82,     0,     0,     0,     0,     0,     0,     0,     0,
      82,    56,    18,     0,    19,    20,    21,     0,     0,     0,
       0,    22,     0,    82,     0,    82,    23,     0,     0,     0,
      25,     0,     0,   176,     0,     0,     0,     0,     0,    28,
       0,     0,    30,    31,     0,     0,     0,     0,     0,     0,
     277,     0,     0,     0,     0,     0,    33,   147,    10,    11,
      12,   175,    14,    15,  1409,     0,    34,   827,   267,    16,
       0,    35,     0,     0,     0,     0,    56,    36,     0,     0,
       0,     0,    37,    18,     0,    19,    20,     0,     0,     0,
       0,     0,    22,     0,     0,     0,     0,    23,     0,     0,
       0,    25,   657,   657,   657,    82,     0,     0,     0,     0,
      28,    82,    82,    30,    31,   657,   277,    82,     0,     0,
      82,    82,    82,     0,     0,     0,     0,    33,     0,     0,
       0,     0,     0,     0,     0,     0,    82,    34,     0,     0,
      82,     0,    35,     0,     0,     0,    82,    82,    36,     0,
       0,     0,     0,     0,     8,     0,     9,    10,    11,    12,
      13,    14,    15,   267,     0,    82,    82,    82,    16,     0,
       0,     0,     0,     0,     0,     0,   657,     0,   657,     0,
     657,     0,    18,     0,    19,    20,     0,     0,     0,     0,
       0,    22,     0,     0,     0,    82,    23,     0,     0,     0,
      25,     0,     0,   459,   413,     0,     0,     0,   616,    28,
       0,   413,    30,    31,     0,     0,    32,     0,     0,     0,
      56,     0,     0,     0,     0,     0,    33,     0,     0,     0,
       0,     0,     0,     0,     0,    82,    34,     0,     0,     0,
       0,    35,   914,   915,   916,   917,   918,    36,   919,   920,
     921,   922,   923,   924,   925,   926,   927,   928,     0,     0,
      56,    56,   160,     0,     0,     0,     0,   267,   275,     0,
    1115,     0,     0,     0,     0,     0,   147,    10,    11,    12,
       0,     0,   256,     0,   657,     0,     0,     0,    82,    56,
       0,     0,   452,    56,    56,   452,   452,   452,     0,     0,
       0,    56,     0,     0,     0,    20,     0,     0,    56,   273,
      10,    11,    12,   175,    14,    15,    23,     0,     0,   484,
       0,    16,     0,     0,     0,     0,     0,     0,     0,    28,
       0,    56,    30,    31,     0,    18,     0,    19,    20,     0,
       0,     0,     0,     0,    22,     0,   202,     0,     0,    23,
     743,     0,     0,    25,     0,   717,   203,     0,     0,     0,
       0,    35,    28,     0,     0,   274,    31,   204,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  1266,
       0,     0,     0,     0,    82,     0,     0,    10,    11,  1267,
     175,    14,    15,    82,    35,     0,   484,     0,    16,     0,
    1268,   721,     0,    82,     0,     0,    82,     0,     0,     0,
       0,     0,    18,     0,    19,    20,     0,     0,    82,     0,
       0,    22,     0,     0,   661,     0,    82,     0,     0,     0,
      25,     0,   717,     0,     0,     0,     0,     0,     0,    28,
       0,     0,   136,   137,     0,     0,     0,     0,     0,     0,
       0,     0,    82,    82,    82,    82,  1495,    10,    11,     0,
      82,     0,    15,     0,     0,     0,  1496,     0,    82,    82,
       0,     0,    82,     0,    82,    82,    82,  1497,   721,     0,
       0,    82,     0,     0,     0,    20,    82,     0,     0,    82,
      10,    11,     0,     0,    14,    15,     0,     0,   657,   657,
     657,    16,   717,     0,   452,   267,     0,     0,     0,    28,
       0,     0,   136,   137,     0,    18,     0,    19,    20,     0,
       0,     0,     0,     0,    22,    82,  1495,     0,     0,   649,
     650,     0,     0,    25,   167,     0,  1496,     0,     0,     0,
       0,     0,    28,     0,     0,   136,   137,  1497,   721,    56,
      56,    56,   160,   160,   160,   452,     0,   267,   213,    56,
     267,  1115,  1115,  1115,   916,   917,   918,   275,   919,   920,
     921,   922,   923,   924,   925,   926,   927,   928,     0,     0,
    1005,     0,     0,   743,     0,     0,     0,     0,     0,     0,
      82,     0,   160,    56,     0,    56,     0,     0,     0,     0,
      56,    56,     0,    56,     0,   273,    10,    11,    12,   175,
      14,    15,     0,     0,     0,   484,     0,    16,     0,     0,
       0,     0,     0,   147,    10,    11,    12,     0,     0,    15,
       0,    18,     0,    19,    20,     0,     0,     0,     0,     0,
      22,     0,     0,    10,    11,    23,   175,    14,    15,    25,
       0,   717,    20,     0,    16,   167,   167,   167,    28,     0,
       0,   274,    31,    23,     0,     0,     0,     0,    18,     0,
      19,    20,     0,     0,     0,  1302,    28,    22,   743,    30,
      31,   213,     0,     0,     0,  1303,   763,     0,     0,   764,
      35,     0,     0,   511,     0,    28,  1304,   721,   136,   137,
     213,   213,   516,   512,     0,     0,     0,     0,    35,     0,
       0,     0,     0,     0,   513,   452,   452,   452,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     213,     0,     0,     0,    82,    10,    11,     0,     0,    14,
     256,     0,     0,     0,     0,     0,    16,     0,    56,    56,
      56,   452,   452,   452,     0,     0,    56,    56,     0,     0,
      18,     0,    19,    20,     0,     0,     0,     0,     0,    22,
       0,     0,     0,     0,    82,    82,    82,     0,    25,     0,
     717,     0,   277,     0,  1117,     0,     0,    28,     0,    56,
     136,   137,     0,  1514,   133,   134,    12,     0,     0,     0,
       0,     0,     0,    82,   718,     0,     0,    82,    82,     0,
     267,   275,     0,   605,   719,    82,     0,     0,     0,    56,
       0,     0,    82,     0,     0,   720,   721,     0,     0,   915,
     916,   917,   918,    23,   919,   920,   921,   922,   923,   924,
     925,   926,   927,   928,     0,    82,    28,     0,     0,    30,
      31,     0,     0,     0,     0,  1516,     0,     0,     0,     0,
       0,     0,     0,    33,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    34,     0,     0,   724,     0,    35,   724,
     724,     0,     0,     0,    36,     0,   652,     0,   273,    10,
      11,    12,   175,    14,   333,   334,   335,   336,   484,   337,
      16,     0,     0,    56,     0,     0,   267,     0,    56,     0,
       0,    56,     0,     0,    18,   338,    19,    20,    21,   213,
     339,   340,   341,    22,     0,   342,   343,   344,    23,   345,
     346,     0,    25,     0,   717,     0,   347,   348,   349,   350,
     351,    28,     0,     0,   274,    31,  -347,     0,     0,   352,
       0,     0,     0,     0,     0,   353,     0,     0,  1049,     0,
       0,     0,     0,     0,     0,     0,   355,   356,  1050,     0,
       0,     0,     0,   358,   359,   360,     0,     0,     0,  1051,
     721,     0,     0,     0,     0,     0,     0,   815,     0,   815,
     815,   724,     0,     0,     0,     0,     0,   362,     0,     0,
       0,     0,     0,   213,   213,     0,     0,   213,     0,   516,
     516,   516,   850,     0,     0,     0,   213,     0,     0,     0,
     743,   213,     0,     0,   213,     0,     0,   211,     0,     0,
       0,   147,    10,    11,    12,     0,     0,   561,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   270,     0,     0,   278,     0,    56,    56,    56,     0,
      20,     0,     0,    82,    82,    82,    82,    82,    82,     0,
       0,    23,     0,    82,     0,  1117,  1117,  1117,   270,     0,
     332,  1313,     0,     0,    28,     0,     0,    30,    31,     0,
       0,     0,  -419,    10,    11,  -419,  -419,    14,   256,     0,
       0,   202,     0,     0,    16,     0,    82,    82,     0,    82,
       0,   203,     0,  1115,    82,    82,    35,    82,    18,     0,
      19,    20,   204,     0,     0,   213,     0,    22,     0,     0,
       0,     0,  -419,     0,     0,     0,    25,     0,   717,     0,
     147,    10,    11,    12,     0,    28,     0,     0,   136,   137,
     211,     0,     0,     0,  1115,  1115,  1115,     0,     0,     0,
       0,     0,   718,     0,     0,     0,     0,    56,     0,   211,
     211,   211,   719,     0,     0,     0,     0,  -419,     0,   522,
      23,     0,     0,   720,   721,     0,     0,   133,   134,     0,
       0,   225,   226,    28,     0,     0,    30,    31,    16,   211,
    1658,     0,  1014,     0,     0,     0,     0,     0,     0,     0,
     202,     0,     0,     0,     0,    20,   724,   724,   724,     0,
     203,    22,     0,     0,     0,    35,   278,     0,     0,   724,
      25,   204,   717,     0,     0,     0,     0,   487,     0,    28,
       0,     0,   136,   137,     0,     0,   270,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   718,     0,     0,     0,
       0,     0,    82,    82,    82,     0,   719,     0,     0,   815,
      82,    82,     0,     0,     0,     0,     0,   731,   721,   675,
     676,   677,   678,   679,   680,   681,   682,   683,   684,   685,
     686,   687,   688,   689,   690,   147,    10,    11,    12,   214,
      14,   215,     0,    82,     0,     0,     0,    16,     0,   815,
     815,  1109,     0,     0,     0,     0,     0,     0,     0,  1109,
       0,    18,     0,    19,    20,   277,     0,     0,     0,     0,
      22,     0,     0,    82,     0,    23,     0,     0,   213,    25,
       0,   850,   213,   213,   850,   850,   850,     0,    28,     0,
     213,    30,    31,   270,   278,   216,     0,   213,     0,     0,
       0,     0,     0,     0,     0,    33,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    34,     0,     0,     0,     0,
      35,     0,     0,     0,     0,     0,    36,     0,   211,     0,
       0,   647,     0,     0,     0,     0,     0,     0,   605,   605,
     605,     0,     0,     0,     0,     0,     0,  1350,    10,  1139,
      12,   214,    14,   215,     0,     0,     0,    82,     0,    16,
       0,     0,    82,     0,     0,    82,     0,     0,     0,     0,
       0,     0,     0,    18,     0,    19,    20,     0,     0,     0,
       0,     0,    22,     0,     0,     0,     0,    23,   724,     0,
       0,    25,     0,     0,     0,     0,   211,     0,   211,   211,
      28,     0,     0,    30,    31,     0,     0,     0,     0,  1352,
       0,     0,   211,   211,     0,     0,   211,    33,   211,   211,
     211,   211,     0,     0,     0,   211,     0,    34,     0,     0,
     211,     0,    35,   211,     0,     0,     0,     0,  1143,     0,
     133,   134,     0,     0,   225,   226,     0,     0,     0,     0,
       0,    16,     0,     0,     0,     0,     0,     0,   803,   487,
       0,     0,     0,     0,     0,     0,   724,   724,    20,     0,
     724,     0,     0,     0,    22,     0,     0,     0,     0,     0,
       0,   724,     0,    25,     0,   717,     0,   724,   724,   724,
       0,     0,    28,   724,     0,   136,   137,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   718,
      82,    82,    82,     0,     0,     0,     0,     0,     0,   719,
       0,     0,     0,     0,     0,     0,   270,   278,     0,     0,
     720,   721,     0,     0,   211,     0,     0,     0,   815,   815,
     815,  1109,  1109,  1109,  1305,     0,     0,     0,   815,     0,
    1109,  1109,  1109,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   850,   850,   850,     0,     0,  1117,     0,     0,
       0,    10,    11,     0,   175,    14,    15,     0,     0,     0,
     484,   167,    16,     0,     0,     0,     0,     0,     0,   213,
       0,     0,     0,     0,     0,     0,    18,     0,    19,    20,
     487,     0,     0,     0,     0,    22,     0,     0,  1117,  1117,
    1117,     0,     0,     0,    25,     0,     0,   487,     0,     0,
     946,    82,     0,    28,     0,     0,   136,   137,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   270,   278,   962,   803,     0,     0,   974,   975,   976,
       0,   980,   981,   982,   983,   984,   985,   986,   987,   988,
     989,   990,   991,   992,   993,   994,   995,   996,   997,   998,
       0,   487,  1014,  1014,     0,  1014,   487,     0,   211,   487,
     487,     0,     0,     0,     0,     0,   724,     0,     0,     0,
       0,     0,     0,     0,   724,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   724,   724,   724,     0,     0,     0,
     487,     0,     0,     0,     0,     0,     0,   487,   211,   211,
    1107,     0,     0,     0,     0,   270,   278,     0,  1107,     0,
       0,     0,     0,     0,     0,     0,     0,   815,   815,   815,
    1305,  1305,  1305,     0,     0,   815,   815,   211,     0,     0,
     211,   211,   211,   211,   211,   211,   522,     0,     0,   211,
       0,   850,   850,   850,   850,   850,   211,     0,   147,    10,
      11,    12,   214,    14,   215,     0,     0,     0,   213,     0,
      16,   803,     0,     0,     0,     0,     0,     0,   278,     0,
       0,     0,     0,     0,    18,     0,    19,    20,     0,     0,
     487,     0,     0,    22,     0,     0,     0,     0,    23,     0,
       0,     0,    25,     0,   487,     0,     0,     0,     0,     0,
       0,    28,     0,     0,    30,    31,     0,     0,  1661,     0,
       0,     0,     0,     0,   109,     0,     0,     0,    33,     0,
       0,     0,     0,   127,   109,     0,     0,     0,    34,     0,
       0,   109,   109,    35,   109,     0,     0,     0,     0,    36,
       0,   273,    10,    11,    12,     0,     0,    15,     0,  1305,
    1305,     0,  1305,     0,     0,     0,   850,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   245,
      20,     0,   213,     0,     0,     0,     0,     0,     0,     0,
       0,    23,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    28,     0,     0,   274,    31,  1203,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   846,     0,  1305,  1305,  1305,  1305,  1305,     0,     0,
       0,   847,     0,     0,     0,     0,    35,  1217,   405,     0,
     127,     0,   848,     0,     0,     0,     0,   109,   109,     0,
       0,     0,     0,   270,   278,   270,   109,   109,     0,     0,
     109,   109,   109,     0,   437,   109,   109,   109,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   803,   487,     0,
       0,     0,  1305,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   487,     0,   487,     0,   487,   211,   211,   211,
     211,   211,  1107,   211,     0,  1306,     0,   211,   270,  1107,
    1107,  1107,     0,     0,     0,   278,     0,     0,     0,     0,
       0,   211,   211,   211,     0,     0,     0,     0,     0,     0,
       0,   487,     0,     0,     0,   213,     0,   213,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   211,     0,
       0,     0,     0,     0,     0,     0,   245,   109,     0,     0,
       0,     0,     0,     0,   722,     0,     0,   722,   722,   147,
      10,    11,    12,   175,    14,    15,     0,     0,     0,     0,
     109,    16,     0,     0,     0,     0,     0,     0,     0,   803,
     803,     0,   516,     0,     0,    18,     0,    19,    20,     0,
       0,     0,     0,     0,    22,     0,     0,     0,     0,    23,
       0,  1337,     0,    25,     0,     0,    10,    11,     0,     0,
      14,    15,    28,   109,     0,    30,    31,    16,     0,     0,
       0,     0,     0,   516,   516,   516,     0,     0,     0,    33,
       0,    18,     0,    19,    20,     0,   213,     0,     0,    34,
      22,     0,     0,     0,    35,     0,     0,     0,     0,    25,
      36,     0,     0,     0,     0,     0,     0,     0,    28,     0,
       0,   136,   137,   700,     0,   722,     0,   722,   722,   722,
       0,     0,     0,     0,   109,     0,   109,   803,   803,   109,
       0,  1398,     0,     0,     0,     0,   211,   211,   211,   211,
     211,  1107,     0,     0,   211,   211,   701,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   109,     0,     0,
     211,   211,   211,   211,   211,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   211,   109,     0,
     109,     0,     0,     0,     0,     0,     0,     0,   109,    10,
      11,   109,   214,    14,   215,     0,     0,     0,   522,     0,
      16,     0,     0,   109,     0,     0,   803,   147,    10,    11,
      12,   214,    14,   215,    18,     0,    19,    20,     0,    16,
       0,     0,     0,    22,     0,     0,   147,    10,    11,    12,
       0,     0,    25,    18,     0,    19,    20,   803,     0,     0,
       0,    28,    22,     0,   136,   137,     0,    23,     0,     0,
       0,    25,     0,     0,     0,    20,     0,     0,     0,     0,
      28,  1452,     0,    30,    31,     0,    23,     0,   211,   211,
    1306,   211,     0,     0,     0,   211,     0,    33,     0,    28,
       0,     0,    30,    31,     0,     0,     0,    34,     0,     0,
       0,   211,    35,     0,   245,   874,   202,     0,    36,     0,
       0,     0,   803,     0,     0,     0,   203,     0,     0,     0,
     109,    35,     0,   273,    10,    11,    12,   204,     0,     0,
       0,     0,   147,   133,  1338,    12,     0,     0,   487,     0,
       0,     0,   211,   211,  1107,   211,   211,   487,     0,     0,
       0,     0,    20,     0,   722,   722,   722,     0,     0,   803,
       0,    20,   109,    23,   109,     0,     0,  1053,     0,  1510,
    1511,     0,    23,     0,     0,     0,    28,     0,     0,   274,
      31,     0,     0,     0,     0,    28,     0,     0,    30,    31,
       0,     0,     0,   846,     0,     0,     0,     0,     0,     0,
     803,   211,   157,   847,     0,     0,     0,   722,    35,     0,
       0,     0,   158,     0,   848,     0,     0,    35,     0,     0,
       0,     0,     0,   159,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  1564,     0,     0,     0,     0,
       0,    10,    11,     0,   175,    14,    15,   722,   722,   722,
    1614,     0,    16,     0,   211,     0,   211,  1053,     0,     0,
      10,    11,   109,     0,    14,   256,    18,     0,    19,    20,
       0,    16,   109,   109,     0,    22,   109,   109,     0,     0,
       0,  1621,  1622,     0,    25,    18,     0,    19,    20,     0,
       0,     0,     0,    28,    22,     0,   136,   137,     0,     0,
       0,     0,     0,    25,     0,     0,     0,     0,  1640,     0,
       0,  1107,    28,   109,     0,   136,   137,     0,     0,     0,
     109,   127,     0,    10,    11,     0,     0,    14,    15,     0,
     245,     0,     0,     0,    16,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1678,     0,    18,     0,
      19,    20,  1107,  1107,  1107,     0,     0,    22,     0,     0,
       0,     0,     0,     0,     0,   211,    25,     0,     0,     0,
       0,     0,     0,     0,     0,    28,    20,     0,   136,   137,
       0,     0,     0,   487,     0,     0,   722,   673,   674,   675,
     676,   677,   678,   679,   680,   681,   682,   683,   684,   685,
     686,   687,   688,   689,   690,     0,     0,     0,  1728,     0,
     669,   670,   671,   672,   673,   674,   675,   676,   677,   678,
     679,   680,   681,   682,   683,   684,   685,   686,   687,   688,
     689,   690,     0,     0,     0,     0,     0,     0,     0,   874,
     674,   675,   676,   677,   678,   679,   680,   681,   682,   683,
     684,   685,   686,   687,   688,   689,   690,     0,   109,   109,
     109,   109,     0,     0,   722,   722,     0,     0,   722,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   722,
       0,     0,     0,     0,     0,  1053,  1053,  1053,     0,     0,
       0,   722,     0,     0,     0,     0,     0,   908,   909,   910,
     911,   912,   913,   914,   915,   916,   917,   918,   109,   919,
     920,   921,   922,   923,   924,   925,   926,   927,   928,   803,
     676,   677,   678,   679,   680,   681,   682,   683,   684,   685,
     686,   687,   688,   689,   690,     0,   722,   722,   722,   722,
     722,   722,   722,     0,     0,     0,   722,     0,  1053,  1053,
    1053,     0,     0,     0,     0,     0,     0,  1626,     0,  -513,
    -513,  -513,  -513,  -513,  -513,  -513,     0,     0,     0,  -513,
       0,  -513,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  -513,     0,  -513,     0,   109,   109,  -513,     0,
     109,     0,     0,     0,  -513,     0,     0,     0,     0,  -513,
       0,   109,     0,  -513,     0,  -513,     0,     0,     0,     0,
       0,     0,  -513,   109,     0,  -513,  -513,  -513,  -513,  -513,
       0,  -513,  -513,  -513,  -513,  -513,  -513,  -513,  -513,  -513,
    -513,  -513,  -513,  -513,  -513,  -513,  -513,  -513,  -513,  -513,
    -513,  -513,  -513,  -513,  -513,  -513,  -513,     0,     0,  -513,
    -513,  -513,  -513,   874,  -513,     0,     0,     0,     0,  1627,
    -513,     0,     0,     0,     0,  -513,  -513,  -513,     0,  -513,
     677,   678,   679,   680,   681,   682,   683,   684,   685,   686,
     687,   688,   689,   690,   722,     0,     0,     0,     0,     0,
       0,     0,   722,     0,     0,   874,     0,     0,   109,     0,
       0,   109,   722,   722,   722,   669,   670,   671,   672,   673,
     674,   675,   676,   677,   678,   679,   680,   681,   682,   683,
     684,   685,   686,   687,   688,   689,   690,     0,     0,     0,
       0,     0,     0,     0,     0,   722,   722,   722,   722,   722,
     722,     0,     0,   722,   722,     0,     0,     0,     0,     0,
       0,     0,  1393,   669,   670,   671,   672,   673,   674,   675,
     676,   677,   678,   679,   680,   681,   682,   683,   684,   685,
     686,   687,   688,   689,   690,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   109,   109,     0,   109,  1712,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   109,     0,     0,     0,
       0,     0,     0,     0,   109,     0,     0,     0,     0,     0,
       0,     0,   668,     0,   109,   109,   669,   670,   671,   672,
     673,   674,   675,   676,   677,   678,   679,   680,   681,   682,
     683,   684,   685,   686,   687,   688,   689,   690,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   722,   722,     0,
     722,     0,     0,   273,    10,    11,    12,     0,    14,   333,
     334,   335,   336,     0,   337,    16,     0,     0,     0,   874,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    18,
     338,    19,    20,    21,     0,   339,   340,   341,    22,     0,
     342,   343,   344,    23,   345,   346,     0,    25,     0,   717,
     109,   347,   348,   349,   350,   351,    28,     0,     0,   274,
      31,   722,   722,   722,   722,   722,     0,     0,     0,     0,
     353,     0,     0,  1049,     0,     0,     0,     0,     0,     0,
       0,   355,   356,  1050,     0,     0,     0,     0,   358,   359,
     360,     0,     0,     0,  1051,   721,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   362,     0,   874,     0,     0,  1207,     0,     0,
     722,   669,   670,   671,   672,   673,   674,   675,   676,   677,
     678,   679,   680,   681,   682,   683,   684,   685,   686,   687,
     688,   689,   690,     0,     0,   109,  1577,   109,  -930,  -930,
    -930,  -930,  -930,  -930,  -930,  -930,  -930,  -930,     0,  -930,
    -930,  -930,     0,  -930,  -930,  -930,  -930,  -930,  -930,  -930,
    -930,  -930,  -930,  -930,  -930,  -930,  -930,  -930,  -930,     0,
    -930,  -930,  -930,  -930,     0,  -930,  -930,  -930,  -930,  -930,
    -930,  -930,  -930,  -930,     0,     0,  -930,  -930,  -930,  -930,
    -930,  -930,     0,     0,  -930,  -930,  -930,     0,  -930,  -930,
       0,     0,     0,     0,     0,  -930,     0,     0,  -930,     0,
       0,     0,     0,     0,     0,     0,  -930,  -930,  -930,     0,
       0,     0,     0,  -930,  -930,  -930,     0,     0,     0,  -930,
       0,     0,     0,  -930,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1578,  -930,  1539,     0,
    -930,  -930,  -930,  -930,  -930,  -930,  -930,  -930,  -930,  -930,
       0,  -930,  -930,  -930,     0,  -930,  -930,  -930,  -930,  -930,
    -930,  -930,  -930,  -930,  -930,  -930,  -930,  -930,  -930,  -930,
    -930,     0,  -930,  -930,  -930,  -930,     0,  -930,  -930,  -930,
    -930,  -930,  -930,  -930,  -930,  -930,     0,     0,  -930,  -930,
    -930,  -930,  -930,  -930,     0,     0,  -930,  -930,  -930,     0,
    -930,  -930,     0,     0,     0,     0,     0,  -930,     0,     0,
    -930,     0,     0,     0,     0,     0,     0,     0,  -930,  -930,
    -930,     0,     0,     0,     0,  -930,  -930,  -930,     0,     0,
       0,  -930,     0,     0,   652,  -930,   147,    10,    11,    12,
     175,    14,   333,   334,   335,   336,   484,   337,    16,  -930,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    18,   338,    19,    20,    21,     0,   339,   340,
     341,    22,     0,   342,   343,   344,    23,   345,   346,     0,
      25,     0,   717,     0,   347,   348,   349,   350,   351,    28,
       0,     0,    30,    31,  -347,     0,     0,   352,     0,     0,
       0,     0,     0,   353,     0,     0,  1112,     0,     0,     0,
       0,     0,     0,     0,   355,   356,  1113,     0,     0,     0,
       0,   358,   359,   360,     0,     0,     0,  1114,   721,   977,
       0,   273,    10,    11,    12,   175,    14,   333,   334,   335,
     336,     0,   337,    16,     0,   362,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    18,   338,    19,
      20,    21,     0,   339,   340,   341,    22,     0,   342,   343,
     344,    23,   345,   346,     0,    25,     0,     0,     0,   347,
     348,   349,   350,   351,    28,     0,     0,   274,    31,  1723,
       0,  -832,   352,     0,     0,     0,     0,     0,   353,     0,
       0,   354,     0,     0,     0,     0,     0,     0,     0,   355,
     356,   357,     0,     0,     0,     0,   358,   359,   360,     0,
       0,   801,   361,   963,   964,   965,    12,     0,    14,   500,
     334,   335,   336,     0,   337,    16,     0,     0,     0,     0,
     362,     0,     0,     0,     0,     0,     0,     0,     0,    18,
     338,    19,     0,    21,     0,   339,   340,   341,    22,     0,
     342,   343,   344,    23,   345,   346,     0,    25,     0,     0,
       0,   347,   348,   349,   350,   351,    28,     0,     0,   966,
     967,   802,     0,     0,   352,     0,     0,     0,     0,     0,
     353,     0,     0,   354,     0,     0,     0,     0,     0,     0,
       0,   355,   356,   357,     0,     0,     0,     0,   358,   359,
     360,     0,     0,     0,   361,   968,   652,     0,   273,    10,
      11,    12,     0,    14,   333,   334,   335,   336,     0,   337,
      16,  1097,   362,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    18,   338,    19,    20,    21,     0,
     339,   340,   341,    22,     0,   342,   343,   344,    23,   345,
     346,     0,    25,     0,     0,     0,   347,   348,   349,   350,
     351,    28,     0,     0,   274,    31,  -347,     0,     0,   352,
       0,     0,     0,     0,     0,   353,     0,     0,   653,     0,
       0,     0,     0,     0,     0,     0,   355,   356,   654,     0,
       0,     0,     0,   358,   359,   360,     0,     0,   801,   655,
     963,   964,   965,    12,     0,    14,   500,   334,   335,   336,
       0,   337,    16,     0,     0,     0,     0,   362,     0,     0,
       0,     0,     0,     0,     0,     0,    18,   338,    19,     0,
      21,     0,   339,   340,   341,    22,     0,   342,   343,   344,
      23,   345,   346,     0,    25,     0,     0,     0,   347,   348,
     349,   350,   351,    28,     0,     0,   966,   967,   802,     0,
       0,   352,     0,     0,     0,     0,     0,   353,     0,     0,
     354,     0,     0,     0,     0,     0,     0,     0,   355,   356,
     357,     0,     0,     0,     0,   358,   359,   360,     0,     0,
       0,   361,   968,   801,     0,   273,    10,    11,    12,     0,
      14,   500,   334,   335,   336,     0,   337,    16,     0,   362,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    18,   338,    19,     0,    21,     0,   339,   340,   341,
      22,     0,   342,   343,   344,    23,   345,   346,     0,    25,
       0,     0,     0,   347,   348,   349,   350,   351,    28,     0,
       0,   274,    31,   802,     0,     0,   352,     0,     0,     0,
       0,     0,   353,     0,     0,   354,     0,     0,     0,     0,
       0,     0,     0,   355,   356,   357,     0,     0,     0,     0,
     358,   359,   360,     0,     0,     0,   361,     0,   801,     0,
     963,   964,   965,    12,  1316,    14,   500,   334,   335,   336,
       0,   337,    16,     0,   362,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    18,   338,    19,     0,
      21,     0,   339,   340,   341,    22,     0,   342,   343,   344,
      23,   345,   346,     0,    25,     0,     0,     0,   347,   348,
     349,   350,   351,    28,     0,     0,   966,   967,   802,     0,
       0,   352,     0,     0,     0,     0,     0,   353,     0,     0,
     354,     0,     0,     0,     0,     0,     0,     0,   355,   356,
     357,     0,     0,     0,     0,   358,   359,   360,     0,     0,
     801,   361,   963,   964,   965,    12,     0,    14,   500,   334,
     335,   336,     0,   337,    16,     0,     0,     0,  -517,   362,
       0,     0,     0,     0,     0,     0,     0,     0,    18,   338,
      19,     0,    21,     0,   339,   340,   341,    22,     0,   342,
     343,   344,    23,   345,   346,     0,    25,     0,     0,     0,
     347,   348,   349,   350,   351,    28,     0,     0,   966,   967,
     802,     0,     0,   352,     0,     0,     0,     0,     0,   353,
       0,     0,   354,     0,     0,     0,     0,     0,     0,     0,
     355,   356,   357,     0,     0,     0,     0,   358,   359,   360,
       0,     0,   652,   361,   147,    10,    11,    12,     0,    14,
     333,   334,   335,   336,     0,   337,    16,     0,     0,     0,
    1430,   362,     0,     0,     0,     0,     0,     0,     0,     0,
      18,   338,    19,    20,    21,     0,   339,   340,   341,    22,
       0,   342,   343,   344,    23,   345,   346,     0,    25,     0,
       0,     0,   347,   348,   349,   350,   351,    28,     0,     0,
      30,    31,  -347,     0,     0,   352,     0,     0,     0,     0,
       0,   353,     0,     0,  1705,     0,     0,     0,     0,     0,
       0,     0,   355,   356,  1706,     0,     0,     0,     0,   358,
     359,   360,     0,     0,  1762,  1707,   273,    10,    11,    12,
       0,    14,   333,   334,   335,   336,     0,   337,    16,     0,
       0,     0,     0,   362,     0,     0,     0,     0,     0,     0,
       0,     0,    18,   338,    19,    20,    21,     0,   339,   340,
     341,    22,     0,   342,   343,   344,    23,   345,   346,     0,
      25,     0,     0,     0,   347,   348,   349,   350,   351,    28,
       0,     0,   274,    31,     0,     0,  -211,   352,     0,     0,
       0,     0,     0,   353,     0,     0,   354,     0,     0,     0,
       0,     0,     0,     0,   355,   356,   357,     0,     0,     0,
       0,   358,   359,   360,     0,     0,   801,   361,   273,    10,
      11,    12,     0,    14,   500,   334,   335,   336,     0,   337,
      16,     0,     0,     0,     0,   362,     0,     0,     0,     0,
       0,     0,     0,     0,    18,   338,    19,     0,    21,     0,
     339,   340,   341,    22,     0,   342,   343,   344,    23,   345,
     346,     0,    25,     0,     0,     0,   347,   348,   349,   350,
     351,    28,     0,     0,   274,    31,   802,     0,     0,   352,
       0,     0,     0,     0,     0,   353,     0,     0,   354,     0,
       0,     0,     0,     0,     0,     0,   355,   356,   357,     0,
       0,     0,     0,   358,   359,   360,     0,     0,   977,   361,
     273,    10,    11,    12,     0,    14,   500,   334,   335,   336,
       0,   337,    16,     0,     0,     0,     0,   362,     0,     0,
       0,     0,     0,     0,     0,     0,    18,   338,    19,     0,
      21,     0,   339,   340,   341,    22,     0,   342,   343,   344,
      23,   345,   346,     0,    25,     0,     0,     0,   347,   348,
     349,   350,   351,    28,     0,     0,   274,    31,     0,     0,
       0,   352,  -832,     0,     0,     0,     0,   353,     0,     0,
     354,     0,     0,     0,     0,     0,     0,     0,   355,   356,
     357,     0,     0,     0,     0,   358,   359,   360,     0,     0,
     977,   361,   273,    10,    11,    12,     0,    14,   500,   334,
     335,   336,     0,   337,    16,     0,     0,     0,     0,   362,
       0,     0,     0,     0,     0,     0,     0,     0,    18,   338,
      19,     0,    21,     0,   339,   340,   341,    22,     0,   342,
     343,   344,    23,   345,   346,     0,    25,     0,     0,     0,
     347,   348,   349,   350,   351,    28,     0,     0,   274,    31,
       0,     0,     0,   352,     0,     0,     0,     0,     0,   353,
       0,     0,   354,     0,     0,     0,     0,     0,     0,     0,
     355,   356,   357,     0,     0,     0,     0,   358,   359,   360,
       0,     0,   961,   361,   273,    10,    11,    12,     0,    14,
     500,   334,   335,   336,     0,   337,    16,     0,     0,  -832,
       0,   362,     0,     0,     0,     0,     0,     0,     0,     0,
      18,   338,    19,     0,    21,     0,   339,   340,   341,    22,
       0,   342,   343,   344,    23,   345,   346,     0,    25,     0,
       0,     0,   347,   348,   349,   350,   351,    28,     0,     0,
     274,    31,     0,     0,     0,   352,     0,     0,     0,     0,
       0,   353,     0,     0,   354,     0,     0,     0,     0,     0,
       0,     0,   355,   356,   357,     0,     0,     0,     0,   358,
     359,   360,     0,     0,   973,   361,   273,    10,    11,    12,
       0,    14,   500,   334,   335,   336,     0,   337,    16,     0,
       0,     0,     0,   362,     0,     0,     0,     0,     0,     0,
       0,     0,    18,   338,    19,     0,    21,     0,   339,   340,
     341,    22,     0,   342,   343,   344,    23,   345,   346,     0,
      25,     0,     0,     0,   347,   348,   349,   350,   351,    28,
       0,     0,   274,    31,     0,     0,     0,   352,     0,     0,
       0,     0,     0,   353,     0,     0,   354,     0,     0,     0,
       0,     0,     0,     0,   355,   356,   357,     0,     0,     0,
       0,   358,   359,   360,     0,     0,  1677,   361,   273,    10,
      11,    12,     0,    14,   500,   334,   335,   336,     0,   337,
      16,     0,     0,     0,     0,   362,     0,     0,     0,     0,
       0,     0,     0,     0,    18,   338,    19,     0,    21,     0,
     339,   340,   341,    22,     0,   342,   343,   344,    23,   345,
     346,     0,    25,     0,     0,     0,   347,   348,   349,   350,
     351,    28,     0,     0,   274,    31,     0,     0,     0,   352,
       0,     0,     0,     0,     0,   353,     0,     0,   354,     0,
       0,     0,     0,     0,     0,     0,   355,   356,   357,     0,
       0,     0,     0,   358,   359,   360,     0,     0,     0,   361,
     273,    10,    11,    12,     0,    14,   500,   334,   335,   336,
       0,   337,    16,     0,     0,     0,     0,   362,     0,     0,
       0,     0,     0,     0,     0,     0,    18,   338,    19,     0,
      21,     0,   339,   340,   341,    22,     0,   342,   343,   344,
      23,   345,   346,     0,    25,     0,     0,     0,   347,   348,
     349,   350,   351,    28,     0,     0,   274,    31,     0,     0,
       0,   352,     0,     0,     0,     0,     0,   353,     0,     0,
     354,     0,     0,     0,     0,     0,     0,     0,   355,   356,
     357,     0,     0,     0,     0,   358,   359,   360,     0,     0,
       0,   361,   273,    10,    11,    12,     0,    14,   500,   334,
     335,   336,     0,   337,    16,     0,     0,     0,     0,   362,
     501,     0,     0,     0,     0,     0,     0,     0,    18,   338,
      19,     0,    21,     0,   339,   340,   341,    22,     0,   342,
     343,   344,    23,   345,   346,     0,    25,     0,     0,     0,
     347,   348,   349,   350,   351,    28,     0,     0,   274,    31,
       0,     0,     0,   352,     0,     0,     0,     0,     0,   353,
       0,     0,   354,     0,     0,     0,     0,     0,     0,     0,
     355,   356,   357,     0,     0,     0,     0,   358,   359,   360,
       0,     0,     0,   361,   273,    10,    11,    12,     0,    14,
     500,   334,   335,   336,     0,   337,    16,     0,     0,     0,
       0,   362,   856,     0,     0,     0,     0,     0,     0,     0,
      18,   338,    19,     0,    21,     0,   339,   340,   341,    22,
       0,   342,   343,   344,    23,   345,   346,     0,    25,     0,
       0,     0,   347,   348,   349,   350,   351,    28,     0,     0,
     274,    31,     0,     0,     0,   352,     0,     0,     0,     0,
       0,   353,     0,     0,   354,     0,     0,     0,     0,     0,
       0,     0,   355,   356,   357,     0,     0,     0,     0,   358,
     359,   360,     0,     0,     0,   361,   273,    10,    11,    12,
       0,    14,   500,   334,   335,   336,     0,   337,    16,     0,
       0,     0,     0,   362,  1017,     0,     0,     0,     0,     0,
       0,     0,    18,   338,    19,     0,    21,     0,   339,   340,
     341,    22,     0,   342,   343,   344,    23,   345,   346,     0,
      25,     0,     0,     0,   347,   348,   349,   350,   351,    28,
       0,     0,   274,    31,     0,     0,     0,   352,     0,     0,
       0,     0,     0,   353,     0,     0,   354,     0,     0,     0,
       0,     0,     0,     0,   355,   356,   357,     0,     0,     0,
       0,   358,   359,   360,     0,     0,     0,   361,   273,    10,
      11,    12,     0,    14,   500,   334,   335,   336,     0,   337,
      16,     0,     0,     0,     0,   362,  1037,     0,     0,     0,
       0,     0,     0,     0,    18,   338,    19,     0,    21,     0,
     339,   340,   341,    22,     0,   342,   343,   344,    23,   345,
     346,     0,    25,     0,     0,     0,   347,   348,   349,   350,
     351,    28,     0,     0,   274,    31,     0,     0,     0,   352,
       0,     0,     0,     0,     0,   353,     0,     0,   354,     0,
       0,     0,     0,     0,     0,     0,   355,   356,   357,     0,
       0,     0,     0,   358,   359,   360,     0,     0,     0,   361,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   362,  1260,  1581,
    1582,  1583,    12,   175,    14,   333,   334,   335,   336,     0,
     337,    16,  1584,     0,  1585,  1586,  1587,  1588,  1589,  1590,
    1591,  1592,  1593,  1594,    17,    18,   338,    19,    20,    21,
       0,   339,   340,   341,    22,     0,   342,   343,   344,    23,
     345,   346,  1595,    25,  1596,     0,     0,   347,   348,   349,
     350,   351,    28,     0,     0,   274,  1597,  1216,     0,  1598,
     352,     0,     0,     0,     0,     0,   353,     0,     0,   354,
       0,     0,     0,     0,     0,     0,     0,   355,   356,   357,
       0,     0,     0,     0,   358,   359,   360,     0,     0,     0,
     361,     0,     0,     0,  1599,     0,     0,     0,  1581,  1582,
    1583,    12,   175,    14,   333,   334,   335,   336,   362,   337,
      16,  1584,     0,  1585,  1586,  1587,  1588,  1589,  1590,  1591,
    1592,  1593,  1594,    17,    18,   338,    19,    20,    21,     0,
     339,   340,   341,    22,     0,   342,   343,   344,    23,   345,
     346,  1595,    25,  1596,     0,     0,   347,   348,   349,   350,
     351,    28,     0,     0,   274,  1597,     0,     0,  1598,   352,
       0,     0,     0,     0,     0,   353,     0,     0,   354,     0,
       0,     0,     0,     0,     0,     0,   355,   356,   357,     0,
       0,     0,     0,   358,   359,   360,     0,     0,     0,   361,
       0,     0,     0,  1599,   273,    10,    11,    12,   175,    14,
     333,   334,   335,   336,   484,   337,    16,   362,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      18,   338,    19,    20,    21,     0,   339,   340,   341,    22,
       0,   342,   343,   344,    23,   345,   346,     0,    25,     0,
     717,     0,   347,   348,   349,   350,   351,    28,     0,     0,
     274,    31,     0,     0,     0,   352,     0,     0,     0,     0,
       0,   353,     0,     0,  1049,     0,     0,     0,     0,     0,
       0,     0,   355,   356,  1050,     0,     0,     0,     0,   358,
     359,   360,     0,     0,     0,  1051,   721,   147,    10,    11,
      12,   175,    14,   333,   334,   335,   336,   484,   337,    16,
       0,     0,     0,   362,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    18,   338,    19,    20,    21,     0,   339,
     340,   341,    22,     0,   342,   343,   344,    23,   345,   346,
       0,    25,     0,   717,     0,   347,   348,   349,   350,   351,
      28,     0,     0,    30,    31,     0,     0,     0,   352,     0,
       0,     0,     0,     0,   353,     0,     0,  1112,     0,     0,
       0,     0,     0,     0,     0,   355,   356,  1113,     0,     0,
       0,     0,   358,   359,   360,     0,     0,     0,  1114,   721,
     147,    10,    11,    12,     0,    14,   333,   334,   335,   336,
       0,   337,    16,     0,     0,     0,   362,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    18,   338,    19,    20,
      21,     0,   339,   340,   341,    22,     0,   342,   343,   344,
      23,   345,   346,     0,    25,     0,   717,     0,   347,   348,
     349,   350,   351,    28,     0,     0,    30,    31,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   353,     0,     0,
    1112,     0,     0,     0,     0,     0,     0,     0,   355,   356,
    1113,     0,     0,     0,     0,   358,   359,   360,     0,     0,
       0,  1114,   721,   273,    10,    11,    12,     0,    14,   333,
     334,   335,   336,     0,   337,    16,     0,     0,     0,   362,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    18,
     338,    19,    20,    21,     0,   339,   340,   341,    22,     0,
     342,   343,   344,    23,   345,   346,     0,    25,     0,     0,
       0,   347,   348,   349,   350,   351,    28,     0,     0,   274,
      31,     0,     0,     0,   352,     0,     0,     0,     0,     0,
     353,     0,     0,   354,     0,     0,     0,     0,     0,     0,
       0,   355,   356,   357,     0,     0,     0,     0,   358,   359,
     360,     0,     0,     0,   361,   273,    10,    11,    12,     0,
      14,   333,   334,   335,   336,     0,   337,    16,     0,     0,
       0,     0,   362,     0,     0,     0,     0,     0,     0,     0,
       0,    18,   338,    19,    20,    21,     0,   339,   340,   341,
      22,     0,   342,   343,   344,    23,   345,   346,     0,    25,
       0,     0,     0,   347,   348,   349,   350,   351,    28,     0,
       0,   274,   613,     0,     0,     0,   614,     0,     0,     0,
       0,     0,   353,     0,     0,   354,     0,     0,     0,     0,
       0,     0,     0,   355,   356,   357,     0,     0,     0,     0,
     358,   359,   360,     0,     0,     0,   361,   273,    10,    11,
      12,     0,    14,   500,   334,   335,   336,     0,   337,    16,
       0,     0,     0,     0,   362,     0,     0,     0,     0,     0,
       0,     0,     0,    18,   338,    19,    20,    21,     0,   339,
     340,   341,    22,     0,   342,   343,   344,    23,   345,   346,
       0,    25,     0,     0,     0,   347,   348,   349,   350,   351,
      28,     0,     0,   274,    31,     0,     0,     0,   352,     0,
       0,     0,     0,     0,   353,     0,     0,   653,     0,     0,
       0,     0,     0,     0,     0,   355,   356,   654,     0,     0,
       0,     0,   358,   359,   360,     0,     0,     0,   655,   273,
      10,    11,    12,     0,    14,   500,   334,   335,   336,     0,
     337,    16,     0,     0,     0,     0,   362,     0,     0,     0,
       0,     0,     0,     0,     0,    18,   338,    19,     0,    21,
       0,   339,   340,   341,    22,     0,   342,   343,   344,    23,
     345,   346,     0,    25,     0,     0,     0,   347,   348,   349,
     350,   351,    28,     0,     0,   274,    31,     0,     0,  1644,
     352,     0,     0,     0,     0,     0,   353,     0,     0,   354,
       0,     0,     0,     0,     0,     0,     0,   355,   356,   357,
       0,     0,     0,     0,   358,   359,   360,     0,     0,     0,
     361,   273,    10,    11,    12,   175,    14,   333,   334,   335,
     336,     0,   337,    16,     0,     0,     0,     0,   362,     0,
       0,     0,     0,     0,     0,     0,     0,    18,   338,    19,
      20,    21,     0,   339,   340,   341,    22,     0,   342,   343,
     344,    23,   345,   346,     0,    25,     0,     0,     0,   347,
     348,   349,   350,   351,    28,     0,     0,   274,    31,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   353,     0,
       0,   354,     0,     0,     0,     0,     0,     0,     0,   355,
     356,   357,     0,     0,     0,     0,   358,   359,   360,     0,
       0,     0,   361,   147,    10,    11,    12,     0,    14,   500,
     334,   335,   336,     0,   337,    16,     0,     0,     0,     0,
     362,     0,     0,     0,     0,     0,     0,     0,     0,    18,
     338,    19,    20,    21,     0,   339,   340,   341,    22,     0,
     342,   343,   344,    23,   345,   346,     0,    25,     0,     0,
       0,   347,   348,   349,   350,   351,    28,     0,     0,    30,
      31,     0,     0,     0,   352,     0,     0,     0,     0,     0,
     353,     0,     0,  1705,     0,     0,     0,     0,     0,     0,
       0,   355,   356,  1706,     0,     0,     0,     0,   358,   359,
     360,     0,     0,     0,  1707,   273,    10,    11,    12,     0,
      14,   500,   334,   335,   336,     0,   337,    16,     0,     0,
       0,     0,   362,     0,     0,     0,     0,     0,     0,     0,
       0,    18,   338,    19,     0,    21,     0,   339,   340,   341,
      22,     0,   342,   343,   344,    23,   345,   346,     0,    25,
       0,     0,     0,   347,   348,   349,   350,   351,    28,     0,
       0,   274,    31,     0,     0,     0,   352,     0,     0,     0,
       0,     0,   353,     0,     0,   354,     0,     0,     0,     0,
       0,     0,     0,   355,   356,   357,     0,     0,     0,     0,
     358,   359,   360,     0,     0,     0,   361,   273,    10,    11,
      12,     0,    14,   500,   334,   335,   336,     0,   337,    16,
       0,     0,     0,     0,   362,     0,     0,     0,     0,     0,
       0,     0,     0,    18,   338,    19,     0,    21,     0,   339,
     340,   341,    22,     0,   342,   343,   344,    23,   345,   346,
       0,    25,     0,     0,     0,   347,   348,   349,   350,   351,
      28,     0,     0,   274,    31,   665,     0,     0,     0,     0,
       0,     0,     0,     0,   353,     0,     0,   354,     0,     0,
       0,     0,     0,     0,     0,   355,   356,   357,     0,     0,
       0,     0,   358,   359,   360,     0,     0,     0,   666,   273,
      10,    11,    12,     0,    14,   500,   334,   335,   336,     0,
     337,    16,     0,     0,     0,     0,   362,     0,     0,     0,
       0,     0,     0,     0,     0,    18,   338,    19,     0,    21,
       0,   339,   340,   341,    22,     0,   342,   343,   344,    23,
     345,   346,     0,    25,     0,     0,     0,   347,   348,   349,
     350,   351,    28,     0,     0,   274,    31,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   353,     0,     0,   354,
       0,     0,     0,     0,     0,     0,     0,   355,   356,   357,
       0,     0,     0,     0,   358,   359,   360,     0,     0,     0,
     361,   705,   273,    10,    11,    12,     0,    14,   500,   334,
     335,   336,     0,   337,    16,     0,     0,     0,   362,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    18,   338,
      19,     0,    21,     0,   339,   340,   341,    22,     0,   342,
     343,   344,    23,   345,   346,     0,    25,     0,     0,     0,
     347,   348,   349,   350,   351,    28,     0,     0,   274,    31,
       0,     0,     0,   614,     0,     0,     0,     0,     0,   353,
       0,     0,   354,     0,     0,     0,     0,     0,     0,     0,
     355,   356,   357,     0,     0,     0,     0,   358,   359,   360,
       0,     0,     0,   361,   273,    10,    11,    12,     0,    14,
     500,   334,   335,   336,     0,   337,    16,     0,     0,     0,
       0,   362,     0,     0,     0,     0,     0,     0,     0,     0,
      18,   338,    19,    20,    21,     0,   339,   340,   341,    22,
       0,   342,   343,   344,    23,   345,   346,     0,    25,     0,
       0,     0,   347,   348,   349,   350,   351,    28,     0,     0,
     274,    31,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   353,     0,     0,   653,     0,     0,     0,     0,     0,
       0,     0,   355,   356,   654,     0,     0,     0,     0,   358,
     359,   360,     0,     0,     0,   655,  1272,    10,    11,    12,
       0,    14,   500,   334,   335,   336,     0,   337,    16,     0,
       0,     0,     0,   362,     0,     0,     0,     0,     0,     0,
       0,     0,    18,   338,    19,     0,    21,     0,   339,   340,
     341,    22,     0,   342,   343,   344,    23,   345,   346,     0,
      25,     0,     0,     0,   347,   348,   349,   350,   351,    28,
       0,     0,   274,    31,     0,     0,     0,   352,     0,     0,
       0,     0,     0,   353,     0,     0,   354,     0,     0,     0,
       0,     0,     0,     0,   355,   356,   357,     0,     0,     0,
       0,   358,   359,   360,     0,     0,     0,   361,   147,    10,
      11,    12,     0,    14,   333,   334,   335,   336,     0,   337,
      16,     0,     0,     0,     0,   362,     0,     0,     0,     0,
       0,     0,     0,     0,    18,   338,    19,    20,    21,     0,
     339,   340,   341,    22,     0,   342,   343,   344,    23,   345,
     346,     0,    25,     0,     0,     0,   347,   348,   349,   350,
     351,    28,     0,     0,    30,    31,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   353,     0,     0,  1705,     0,
       0,     0,     0,     0,     0,     0,   355,   356,  1706,     0,
       0,     0,     0,   358,   359,   360,     0,     0,     0,  1707,
     273,    10,    11,    12,     0,    14,   500,   334,   335,   336,
       0,   337,    16,     0,     0,     0,     0,   362,     0,     0,
       0,     0,     0,     0,     0,     0,    18,   338,    19,     0,
      21,     0,   339,   340,   341,    22,     0,   342,   343,   344,
      23,   345,   346,     0,    25,     0,     0,     0,   347,   348,
     349,   350,   351,    28,     0,     0,   274,    31,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   353,     0,     0,
     354,     0,     0,     0,     0,     0,     0,     0,   355,   356,
     357,     0,     0,     0,     0,   358,   359,   360,     0,     0,
       0,   361,   273,    10,    11,    12,     0,    14,   500,   334,
     335,   336,     0,   337,    16,     0,     0,     0,     0,   362,
       0,     0,     0,     0,     0,     0,     0,     0,    18,   338,
      19,     0,    21,     0,   339,   340,   341,    22,     0,   342,
     343,   344,    23,   345,   346,     0,    25,     0,     0,     0,
     347,   348,   349,   350,   351,    28,     0,     0,   274,    31,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   353,
       0,     0,   354,     0,     0,     0,     0,     0,     0,     0,
     355,   356,   357,     0,     0,     0,     0,   358,   359,   360,
       0,     0,     0,   727,   273,    10,    11,    12,     0,    14,
     500,   334,   335,   336,     0,   337,    16,     0,     0,     0,
       0,   362,     0,     0,     0,     0,     0,     0,     0,     0,
      18,   338,    19,     0,    21,     0,   339,   340,   341,    22,
       0,   342,   343,   344,    23,   345,   346,     0,    25,     0,
       0,     0,   347,   348,   349,   350,   351,    28,     0,     0,
     274,    31,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   353,     0,     0,   354,     0,     0,     0,     0,     0,
       0,     0,   355,   356,   357,     0,     0,     0,     0,   358,
     359,   360,     0,     0,     0,   729,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   362,  1749,   669,   670,   671,   672,   673,
     674,   675,   676,   677,   678,   679,   680,   681,   682,   683,
     684,   685,   686,   687,   688,   689,   690,   669,   670,   671,
     672,   673,   674,   675,   676,   677,   678,   679,   680,   681,
     682,   683,   684,   685,   686,   687,   688,   689,   690
};

static const short yycheck[] =
{
      16,     6,   218,     6,   182,   184,   183,    25,    24,   796,
     419,   132,   164,   381,   419,     6,    84,   910,   387,    51,
      16,    84,     6,   157,   158,     6,   479,   547,   624,     6,
     612,   419,    89,    38,   368,    38,   140,    47,   198,    47,
     387,   257,   798,   778,    47,   379,   380,    38,    80,   389,
    1006,    97,  1423,    99,    38,    71,    47,    38,   419,    62,
    1429,    38,   772,    47,   461,   198,    47,     6,    60,   218,
      47,   254,  1144,    76,  1146,   248,  1545,   139,   140,   228,
     130,   885,  1154,   139,  1545,    47,    89,   891,   497,   152,
       6,    87,   497,  1540,     1,   227,   228,   313,    47,    38,
     146,  1446,  1447,    62,     6,    47,  1688,   263,    47,    61,
    1600,   114,     6,    26,   130,     3,    12,   113,  1463,    62,
     528,  1785,    38,    65,   492,   322,   323,    13,    34,    56,
       0,    33,    34,    61,    77,   503,    38,    65,    65,   142,
     156,    39,   550,  1807,    38,    47,  1709,    99,    61,   212,
      89,   142,    65,    47,    61,    62,  1719,  1720,   142,     9,
       0,   142,     1,   173,  1746,   381,  1613,   177,  1513,   177,
     173,    36,    62,     0,   177,   114,    62,  1522,  1523,    65,
    1525,   184,   173,   142,   780,    98,   177,    89,    76,   173,
     222,   183,   173,   177,   143,   198,   177,  1687,     1,    95,
     177,   936,  1765,   142,   939,   111,  1616,    36,     3,     4,
       5,   279,   114,   111,    64,   177,     7,  1753,    62,    56,
     223,   111,    61,    76,   110,   184,    65,  1637,   177,   426,
     427,   387,   248,   249,   173,   177,    56,    61,   177,    76,
     142,  1045,    66,  1653,  1691,    65,    50,   263,    39,   143,
    1786,   620,   518,  1722,  1817,   157,   158,   173,    61,    98,
     526,  1722,    65,   259,    59,    60,   110,     8,     9,     4,
       5,   173,  1228,   620,    15,   177,   218,    94,   324,  1624,
    1625,    49,  1729,   177,   223,   227,   228,   439,    83,    94,
     110,    32,   157,   158,   159,    98,    26,    38,    28,  1709,
     202,   203,    40,    56,    63,   409,    47,    13,    49,  1719,
    1720,   314,    56,   329,   376,   257,   373,   320,    56,  1766,
     376,   223,   325,    76,    59,    60,    94,   114,   157,   158,
     159,    61,    76,    46,   325,    65,    76,   353,    49,   204,
      63,   325,   110,    56,   325,    76,    59,   480,  1083,  1084,
     109,   401,   111,    94,  1723,  1765,    62,   110,   361,    65,
      66,    62,   514,  1167,  1074,  1169,   325,    62,    98,    56,
     373,   313,   388,   546,    94,   204,   525,   511,   512,   382,
    1751,   320,   428,    94,    95,   401,   325,    49,   111,    76,
    1462,   387,   524,   525,   791,   792,    26,   744,    28,   110,
      13,   533,    28,    26,   765,    28,    32,  1817,    26,   110,
      28,    77,   199,    79,   754,   110,   419,   549,   320,   422,
      63,     9,   361,   325,    56,    13,    61,   109,   625,   421,
      65,    61,    94,    95,   373,    65,   223,    63,    61,   381,
      66,    49,    65,    61,    76,  1401,    63,    65,   110,    62,
      49,    26,    65,   463,     1,   463,     3,     4,     5,   361,
     463,    49,    63,    49,   620,  1200,  1201,    56,    98,   601,
      94,   373,   463,    49,    62,    98,    64,    65,    66,   463,
      98,   423,   463,    63,    56,   111,    94,   490,   621,    77,
     493,    79,   538,    65,   497,    94,  1078,   489,    62,   491,
     492,    62,    49,    62,   536,   638,    94,    95,    94,    56,
     111,   110,    59,    60,    61,    62,     6,    65,    94,    95,
      32,   463,   110,    65,   463,     4,     5,    49,    76,     3,
     546,   111,  1336,    49,   110,  1131,     3,    13,   110,     6,
       1,    76,     3,     4,     5,  1349,   110,    94,    38,   110,
     492,   110,    28,   709,    61,  1008,    32,    49,    65,   692,
      56,   463,   608,    61,   697,    32,    94,   700,   701,    49,
       4,     5,    94,    49,    66,  1095,    43,    56,    94,    95,
      59,    60,   524,   525,   781,    56,    62,    63,    49,    65,
      66,   533,    59,    60,    73,    56,    76,    56,    59,    60,
      62,    63,    94,    95,    83,   738,    73,   549,    49,   511,
     512,    49,   790,    56,    94,    95,    83,  1137,    94,    95,
      49,    88,    56,    77,   620,    59,    60,    94,    49,    76,
     110,    49,   110,    94,   110,   627,    49,    66,   110,   642,
     643,   644,   645,   646,    56,   702,   511,   512,   513,   665,
      65,   755,   655,    94,    95,    50,    94,    95,   762,   601,
      56,    76,   110,   666,    76,    94,  1075,  1120,  1121,   110,
    1075,   110,   110,    94,    95,    49,    94,    95,    12,    62,
      76,    94,   511,   512,   513,    49,   628,  1075,  1057,   872,
     873,   843,   110,   709,    77,   878,    79,   111,   701,   702,
     762,  1505,    66,   642,   643,   644,   645,   646,   905,   190,
    1057,   757,    49,    99,  1075,    63,   655,   720,    56,  1053,
      94,  1477,     4,     5,   727,   885,   729,   666,   731,   861,
      94,   891,    94,   520,   950,    49,     3,    49,    76,   735,
     642,   643,   644,   645,   646,   763,    63,   763,    56,  1118,
      32,     4,     5,   655,    76,   818,   772,    94,    95,    49,
     881,    65,   701,   702,   666,  1218,  1219,    49,    76,    61,
      49,  1118,    76,   929,    56,    95,    49,    59,    60,   771,
      94,    95,    94,    95,  1540,    95,   802,    49,   727,    49,
     729,    73,     4,     5,  1687,    65,   110,   113,   110,   701,
     702,    83,    56,    56,    94,     6,    59,    60,     4,     5,
     813,  1331,    94,    95,   882,    94,   718,   719,   821,    56,
      73,    94,    49,   819,    49,   727,    62,   729,    65,   771,
      83,    56,    94,    49,    94,    94,    49,    38,    49,    76,
     886,   887,    95,   889,  1297,    65,    47,    59,    60,    94,
      46,    76,     4,     5,    94,   642,    76,  1613,    76,    60,
      56,    62,    94,    59,    60,  1233,    76,    94,    76,    94,
    1067,     3,     4,     5,    76,  1328,  1009,    65,    94,   882,
      32,    94,   885,    94,   671,  1045,   110,    65,   891,    62,
    1023,   882,  1025,    49,  1027,    62,    63,  1790,   882,    76,
      56,  1057,    56,  1055,    56,   882,  1426,    59,    60,   811,
     812,   698,   110,   929,  1092,    77,    56,    79,   705,   861,
      76,    73,   110,   882,    56,    65,    76,    59,    60,  1062,
    1686,    83,   110,    28,   721,  1691,    76,    32,    94,    76,
    1393,   142,    94,   882,   846,   847,   110,   148,   813,   110,
    1737,  1134,  1135,  1136,  1106,    12,   821,    62,    63,     3,
       4,     5,  1118,   111,  1161,  1162,  1163,    62,    63,  1103,
    1104,    66,   173,  1729,    49,   111,   177,    65,  1112,  1113,
     882,    56,   183,   184,   813,   110,    28,  1440,  1234,  1235,
      32,  1237,   821,    49,     3,     4,     5,   110,     7,     8,
       9,    76,  1005,   110,     4,     5,  1009,  1167,   950,  1169,
    1766,    66,    56,     4,     5,    59,    60,    61,     9,    94,
      62,    63,   113,    32,     3,     4,     5,     6,  1481,    38,
      66,   232,    32,    89,    90,    28,  1039,    93,    94,    95,
      96,   110,  1045,   110,  1060,  1108,   110,    56,  1051,    49,
      59,    60,    61,   900,   901,   902,    56,    77,  1074,    59,
      60,  1057,    86,    87,    43,    56,  1005,    56,    59,    60,
      76,    50,  1075,    73,  1077,    76,  1144,  1095,  1146,    76,
      59,    60,    73,    83,  1152,   113,  1154,   113,     3,     4,
       5,     6,    83,     3,    94,    95,   105,   106,   107,    56,
    1039,    56,  1105,  1005,    95,    63,    62,    63,    65,    88,
      65,  1114,    62,    62,    63,  1147,  1148,    62,    63,    76,
    1123,    76,  1118,   910,   325,  1157,    76,    77,    43,    79,
      56,    66,    86,    87,   111,    50,   110,  1039,  1077,    65,
      62,  1144,  1551,  1146,    59,    60,  1551,  1049,  1050,  1152,
      76,  1154,    56,  1144,    56,  1146,     3,     4,     5,  1151,
    1144,    65,  1146,  1154,  1167,  1146,  1169,  1144,    83,  1146,
    1154,   110,    76,    88,   110,  1077,  1336,  1154,     3,     4,
       5,     4,     5,   110,  1123,  1144,     9,  1146,   198,  1349,
      62,    63,  1151,  1152,     8,  1154,  1242,    62,    63,    62,
      63,  1103,  1104,  1219,  1207,  1144,    94,  1146,    49,    32,
    1112,  1113,    59,    60,  1411,  1154,     3,     4,     5,    94,
     421,  1123,    62,    63,   111,    50,  1013,    56,    63,   430,
      76,    38,    66,    56,    59,    60,    59,    60,  1103,  1104,
    1105,  1609,  1144,   110,  1146,    66,    66,  1112,  1113,  1114,
      73,   110,  1154,    63,     3,     4,     5,     6,    83,  1046,
      83,   110,   463,    50,    77,  1268,   110,    66,  1207,    66,
     113,    94,    59,    60,  1103,  1104,  1105,    62,  1143,   110,
     110,  1297,    84,  1112,  1113,  1114,  1419,   111,   489,   490,
     491,   492,   110,  1361,    43,  1428,   497,    36,  1361,   110,
      77,  1304,  1370,    77,   505,  1207,    77,    56,    77,   111,
      59,    60,   110,    61,  1143,   110,   110,   518,    65,    65,
      62,  1353,     7,     8,     9,   526,   110,  1359,  1360,    65,
      15,  1363,  1364,  1336,   110,  1367,    88,   139,   113,    88,
      95,   111,     3,     4,     5,  1505,  1349,  1800,  1340,  1717,
     152,   110,   110,    38,    76,    49,   113,     3,     4,     5,
       6,   562,    47,   113,  1266,  1267,   110,  1370,     3,     4,
       5,   111,   110,   110,    33,    34,    62,    36,   110,  1370,
      66,  1340,     3,     4,     5,   866,  1370,   110,    63,  1457,
      76,    77,   110,  1370,  1462,    56,   110,    43,    59,    60,
    1302,  1303,   110,    62,    50,    61,    65,   110,  1426,    66,
     212,  1370,    71,    59,    60,    50,    65,    76,   110,   158,
     159,  1323,  1324,   110,    59,    60,   627,     4,     5,  1608,
    1607,  1370,  1609,  1436,    94,   111,    94,    83,    59,    60,
      94,    62,    88,    94,   110,   110,   248,     8,    83,   113,
     113,  1238,    60,    83,    84,    85,    86,    87,  1474,  1462,
      83,    84,    85,    86,    87,   204,   110,   110,  1370,   110,
     480,  1462,    49,     3,     4,     5,     6,   279,  1462,    56,
      97,   110,    59,    60,   110,  1462,   110,    62,  1520,  1521,
     149,    62,    65,    56,  1497,   154,    73,  1436,   157,   158,
     159,  1717,  1505,  1462,   110,   110,    83,     7,     8,     9,
      40,    41,   110,    43,    65,    15,   113,    94,    95,   110,
       3,     4,     5,  1462,    49,   184,    56,    66,   110,    59,
      60,   190,    32,    34,  1436,   113,  1714,    94,    38,    65,
    1545,   110,  1545,   202,   203,   204,   110,    47,  1551,    65,
     110,     3,     4,     5,  1545,   110,   110,  1573,    88,   218,
    1462,  1545,   110,    63,  1545,   110,    63,    50,  1545,   228,
     771,  1705,  1706,    63,     9,   183,    59,    60,  1594,  1595,
       3,     4,     5,     6,   386,    63,    17,   111,   110,     3,
       4,     5,     6,  1495,  1496,     9,   110,  1600,   257,    61,
      83,  1388,  1389,  1390,  1391,  1608,  1545,    59,    60,  1600,
      94,   621,  1399,   272,    94,  1607,  1600,  1609,    32,  1600,
      43,    63,   361,  1600,   232,    63,    49,    66,   638,    43,
     432,  1663,    56,    56,  1650,  1651,    59,    60,    94,   840,
       8,     9,    56,  1545,   109,    59,    60,    15,    18,   104,
      73,   114,    63,    56,   313,   314,    12,   110,   859,    73,
      83,  1600,    63,   110,    32,    88,    63,  1609,    63,    83,
      38,    94,    95,    63,    88,    94,    62,    66,  1681,    47,
      94,   882,   692,  1699,  1687,  1688,   113,   697,   110,    12,
     700,   701,    63,   110,    63,    12,  1687,   110,  1600,     7,
       8,     9,   361,  1687,  1707,    12,  1687,    15,  1740,    94,
    1687,   450,   451,    63,     3,     4,     5,  1722,    63,  1722,
      63,   731,   381,   382,    32,    63,  1718,   110,   738,   178,
      38,  1722,     3,     4,     5,   198,     0,     3,  1722,    47,
      38,  1722,  1681,  1746,   546,  1722,  1651,   755,  1687,  1688,
    1075,   177,  1768,     3,     4,     5,     6,   463,  1545,  1713,
     223,    50,  1152,   422,   423,   173,  1426,   101,  1707,   145,
      59,    60,  1636,   512,   513,  1717,   316,  1060,   634,  1681,
     802,   440,  1331,  1722,   443,  1687,  1688,  1137,    59,    60,
     449,   450,   451,    43,    83,  1152,   455,  1370,  1459,  1664,
      50,   942,   604,  1705,  1706,  1707,  1593,  1746,   636,    59,
      60,   821,   702,   421,     3,     4,     5,   619,   209,   130,
    1722,   614,  1540,   376,  1478,   835,   659,  1722,  1737,  1784,
    1734,   490,   830,   492,   493,  1664,  1436,    -1,    88,    -1,
    1705,  1706,  1707,    -1,  1746,    -1,    -1,   506,    -1,    -1,
     509,    -1,   511,   512,   513,    -1,    -1,   320,   517,  1646,
      -1,  1342,  1343,    -1,   523,    -1,   525,    -1,    -1,   528,
      59,    60,    61,     3,     4,     5,  1705,  1706,  1707,    -1,
      -1,   489,    -1,   491,   492,    -1,    -1,    -1,  1369,    -1,
     549,   550,    -1,  1374,  1681,    -1,    -1,   505,   361,  1100,
    1687,  1688,    -1,    -1,    -1,    -1,   514,    -1,    -1,    -1,
     518,    -1,   714,    -1,    -1,   654,   655,    -1,   526,    -1,
     858,     1,    -1,     3,     4,     5,     6,   666,     8,    59,
      60,    -1,   734,    -1,    -1,  1722,    -1,    -1,    -1,    -1,
      -1,    -1,   601,  1144,    -1,  1146,   884,    -1,    -1,    -1,
    1151,  1152,   890,  1154,    -1,    -1,   419,    -1,    -1,  1746,
      -1,    -1,    -1,    43,    -1,     3,     4,     5,     6,   628,
      50,  1452,  1453,  1760,  1455,  1456,    56,  1458,    -1,    59,
      60,    -1,    -1,    -1,     4,     5,    -1,    -1,   727,     9,
     729,    -1,   731,  1780,   653,   654,   655,    -1,    -1,  1009,
      -1,    -1,    -1,  1790,    -1,    43,    -1,   666,    88,     3,
       4,     5,     6,  1023,    -1,  1025,   818,  1027,    56,   627,
      -1,    59,    60,    -1,  1811,    63,    -1,    65,    -1,  1510,
    1511,     3,     4,     5,   497,    73,    56,    -1,    32,    59,
      60,    -1,    -1,    -1,    -1,    83,    -1,   849,    -1,    43,
      88,    -1,  1062,    73,    -1,    -1,    94,    -1,    -1,   718,
     719,   720,    56,    83,    -1,    59,    60,    -1,   727,    -1,
     729,    -1,   731,    -1,   813,    -1,    -1,    -1,    50,    73,
      -1,    -1,   821,  1564,  1565,  1566,    -1,    59,    60,    83,
      -1,    34,    -1,    -1,    88,    -1,   835,    -1,  1299,    -1,
      94,   903,  1040,    -1,  1042,    -1,    -1,  1308,   847,   848,
      -1,    -1,   771,   772,    -1,   908,   909,    -1,   911,   912,
     913,   914,   915,   916,   917,   918,   919,   920,   921,   922,
     923,   924,   925,   926,   927,   928,    -1,    -1,    -1,  1340,
    1621,  1622,    -1,    -1,     3,     4,     5,     6,    -1,    -1,
      -1,    -1,   811,   812,   813,    -1,     3,     4,     5,     6,
      -1,    -1,   821,   771,    -1,     3,     4,     5,     6,  1370,
      -1,    -1,    -1,    -1,    -1,    -1,   835,    -1,    -1,   642,
     643,   644,   645,   646,    43,   844,   845,   846,   847,   848,
       4,     5,   655,   852,    -1,     9,    43,    -1,    -1,    -1,
      59,    60,    -1,   666,    -1,    43,   149,   866,    -1,    56,
      -1,   154,    59,    60,    -1,    -1,    63,    -1,    32,    -1,
     378,    59,    60,   381,   382,    -1,    73,    -1,  1166,    88,
    1168,    -1,   840,    -1,    -1,   843,    83,    -1,   701,    -1,
      -1,    88,    56,    -1,    -1,    59,    60,    94,    -1,    -1,
      88,   859,    -1,  1454,  1056,    -1,  1058,    -1,  1459,    73,
     203,  1462,    -1,    -1,   727,    -1,   729,    -1,     1,    83,
       3,     4,     5,     6,     7,     8,     9,  1758,    -1,    -1,
      94,    -1,    15,    -1,    -1,    -1,    -1,    -1,  1081,    -1,
      -1,   950,    -1,    -1,    -1,    -1,    29,    -1,    31,    32,
      33,    -1,    -1,    -1,    -1,    38,  1108,    -1,    -1,  1111,
      43,  1050,  1051,    -1,    47,    48,    -1,    50,    -1,    -1,
    1258,    -1,    -1,    56,    -1,    -1,    59,    60,    -1,   272,
      63,   489,    65,   491,   492,   493,    -1,     3,     4,     5,
      73,     3,     4,     5,     6,     3,     4,     5,     6,    -1,
      83,    -1,    -1,    -1,    -1,    88,    -1,    -1,     4,     5,
      -1,    94,    -1,     9,    -1,  1104,  1105,    -1,    -1,    -1,
    1029,    -1,    -1,  1032,  1113,  1114,    -1,    -1,    -1,    -1,
      -1,    43,    -1,    -1,  1043,    43,    32,    -1,    50,    -1,
    1049,  1050,  1051,    59,    60,    -1,    -1,    59,    60,    -1,
      -1,    59,    60,    49,  1143,    -1,  1607,  1608,  1609,  1419,
      56,    -1,    -1,    59,    60,  1074,    -1,    -1,  1428,    -1,
      -1,    83,   885,     3,     4,     5,    88,    73,   891,     3,
       4,     5,     6,     7,     8,     9,    -1,    83,     3,     4,
       5,    15,  1101,  1102,  1103,  1104,  1105,    -1,    94,    95,
      -1,  1110,    -1,  1112,  1113,  1114,    -1,    -1,    32,     3,
       4,     5,    -1,    -1,    38,  1124,  1125,  1126,    -1,    43,
      50,    -1,    -1,    47,    -1,    49,    -1,  1415,    -1,    59,
      60,    -1,    56,    -1,  1143,    59,    60,    -1,     3,     4,
       5,    56,  1100,  1152,    59,    60,    -1,   440,  1106,    73,
     443,    -1,     4,     5,  1442,    -1,    -1,     9,    -1,    83,
    1448,    -1,   455,    -1,    88,    59,    60,  1718,    -1,    -1,
      94,    95,    -1,    -1,    -1,    -1,  1464,  1465,  1267,  1268,
      32,    -1,    -1,    -1,   718,   719,    -1,    -1,    -1,    -1,
      -1,    -1,  1005,  1151,    59,    60,    -1,     3,     4,     5,
    1488,     7,     8,     9,    56,    -1,    -1,    59,    60,  1361,
      -1,    -1,    -1,   506,  1303,  1304,   509,    -1,    -1,     6,
      -1,    73,    -1,    -1,    -1,    -1,  1039,    -1,    -1,    16,
     523,    83,  1045,    -1,    -1,    -1,    -1,    -1,    -1,    26,
      -1,  1384,    94,  1531,  1532,    -1,    33,    34,    -1,    36,
      -1,    38,    -1,    59,    60,    -1,    -1,  1266,  1267,  1268,
      47,    -1,  1075,   771,  1077,    -1,    -1,  1555,  1556,    -1,
    1558,    -1,    -1,    60,    -1,    62,    -1,   811,   812,    -1,
      -1,  1433,    -1,    -1,    71,    -1,    -1,    -1,    -1,    76,
      -1,  1300,  1301,  1302,  1303,  1304,    -1,    -1,  1307,    -1,
      87,    -1,    89,    -1,  1664,  1457,    -1,    -1,    -1,    -1,
    1123,    -1,    -1,    -1,  1323,  1324,  1325,     3,     4,     5,
       6,    -1,    -1,    -1,    -1,    -1,   113,    -1,    -1,    -1,
      -1,    -1,    -1,  1342,  1343,    -1,     3,     4,     5,     6,
      -1,  1299,    81,    82,    83,    84,    85,    86,    87,    -1,
    1308,    -1,    -1,   140,  1167,   142,  1169,    43,    -1,    -1,
    1369,   148,   149,    -1,    50,  1374,   153,   154,  1326,  1327,
     157,   158,   159,    59,    60,    -1,    43,    -1,     3,     4,
       5,     6,  1340,    50,  1672,    -1,   173,    -1,    -1,    -1,
     177,    -1,    59,    60,  1207,    -1,   183,   184,     3,     4,
       5,     6,    88,    -1,     9,    -1,    -1,    32,    81,    82,
      83,    84,    85,    86,    87,   202,   203,   204,    43,    -1,
      -1,    88,  1431,  1432,    49,  1434,    -1,    32,    -1,    -1,
      -1,    56,    -1,    -1,    59,    60,    -1,    -1,    43,    -1,
      -1,    -1,   950,  1452,  1453,   232,  1455,  1456,    73,  1458,
      -1,    56,    -1,    -1,    59,    60,    -1,    -1,    83,    -1,
      -1,    -1,   249,    88,    -1,    -1,    -1,    -1,    73,    94,
      95,    -1,   259,     3,     4,     5,     6,    -1,    83,    -1,
    1438,    -1,    -1,    88,    -1,   272,  1495,  1496,  1497,    94,
      -1,    -1,    -1,    -1,    -1,  1029,  1454,    -1,  1032,    -1,
      -1,  1510,  1511,    -1,    -1,    -1,     7,     8,     9,  1043,
      68,    -1,    -1,    43,    15,  1049,  1050,    -1,    -1,   812,
    1028,    -1,    -1,  1336,    -1,    -1,    56,   314,   315,    59,
      60,    32,    -1,   320,    -1,    65,  1349,    38,   325,    -1,
    1498,  1499,    -1,    73,    -1,    -1,    47,  1055,    -1,    -1,
      -1,   844,   845,    83,    -1,  1564,  1565,  1566,    88,   852,
      -1,    -1,    63,    -1,    94,    -1,    -1,  1101,  1102,  1103,
    1104,    -1,    -1,    -1,    -1,  1664,  1110,    -1,  1112,  1113,
      -1,    -1,    -1,     3,     4,     5,   373,     7,    -1,   376,
     148,   378,  1100,    -1,   381,   382,  1554,    -1,  1106,  1608,
    1609,     3,     4,     5,     6,    -1,   164,    -1,     3,     4,
       5,     6,  1621,  1622,     9,    -1,    -1,  1706,  1707,    39,
      -1,    -1,   409,  1436,    -1,    -1,    -1,     3,     4,     5,
       6,    -1,   419,    -1,   421,   422,    56,    32,    -1,    59,
      60,    43,    -1,   430,    -1,    -1,    -1,    -1,    43,  1607,
      -1,  1609,    -1,   440,    -1,  1664,   443,    59,    60,    -1,
      -1,    56,    -1,   221,    59,    60,    -1,    43,   455,    -1,
      -1,    -1,   230,    -1,    50,    -1,   463,    -1,    73,    -1,
      -1,    -1,    -1,    59,    60,    -1,    88,    -1,    83,   247,
      -1,    -1,  1505,    88,    -1,    -1,  1705,  1706,  1707,    94,
     258,    -1,   489,   490,   491,   492,   493,    83,  1717,    -1,
     497,    -1,    88,    -1,    -1,    -1,    -1,    -1,   505,   506,
      -1,    -1,   509,    -1,   511,   512,   513,   514,    -1,    -1,
      -1,   518,  1266,  1267,    -1,    -1,   523,    -1,  1551,   526,
    1248,    -1,     3,     4,     5,     6,    -1,    -1,  1256,  1758,
       3,     4,     5,     6,     7,     8,     9,    -1,    -1,    -1,
    1718,    86,    15,    -1,    -1,    -1,  1300,  1301,  1302,  1303,
      -1,    32,    -1,  1307,    -1,   562,    29,    -1,    31,    32,
      33,    -1,    43,    -1,    -1,    38,    -1,    -1,    49,    -1,
      43,  1299,    -1,    -1,    47,    56,    -1,    50,    59,    60,
    1308,    -1,    -1,    56,    -1,    -1,    59,    60,  1101,  1102,
      -1,    -1,    73,    -1,    -1,    -1,    -1,  1110,    -1,   606,
      73,    -1,    83,     3,     4,     5,     6,    88,    -1,    -1,
      83,    -1,    -1,    94,    95,    88,    -1,    -1,    -1,    -1,
     627,    94,   167,    -1,    -1,    -1,    99,    -1,    -1,     3,
       4,     5,     6,     7,     8,     9,   643,   644,   645,   646,
      -1,    15,    -1,    43,    -1,    -1,    -1,    -1,    -1,    -1,
      50,    -1,   430,    -1,    -1,   200,    -1,    -1,    32,    59,
      60,   439,    -1,    -1,    38,    -1,    -1,    -1,   213,    43,
      -1,    -1,    -1,    47,  1707,    49,    -1,  1431,  1432,    -1,
    1434,    -1,    56,    83,    -1,    59,    60,    -1,    88,    -1,
      -1,    -1,    -1,   471,    -1,   702,    -1,   704,    -1,    73,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   715,    83,
      -1,   718,   719,   720,    88,    -1,     3,     4,     5,     6,
      94,    95,     9,    -1,   731,    -1,    -1,   505,   735,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   514,    -1,    -1,    -1,
      -1,  1495,  1496,    -1,    -1,    32,    -1,    -1,   755,    -1,
      -1,    -1,    -1,    -1,    -1,   762,    43,   535,    -1,    -1,
      -1,    -1,    49,    -1,   771,   772,    -1,    -1,    -1,    56,
    1498,  1499,    59,    60,    -1,    -1,    -1,    -1,     3,     4,
       5,     6,    -1,    -1,   562,    -1,    73,  1300,  1301,    -1,
       3,     4,     5,     6,  1307,    -1,    83,    -1,    -1,    -1,
      -1,    88,    -1,    -1,   811,   812,   813,    94,    95,    -1,
      -1,    -1,   819,    -1,   821,     3,     4,     5,    43,     7,
       8,     9,    -1,    -1,    -1,    -1,  1554,    40,    41,    -1,
      43,    56,    -1,   840,    59,    60,   843,   844,   845,   846,
     847,   848,    -1,    56,    32,   852,    59,    60,    73,    -1,
      38,    -1,   859,    -1,    -1,    -1,    -1,    -1,    83,    -1,
      -1,    -1,    -1,    88,    -1,    -1,    -1,    -1,    -1,    94,
      -1,    59,    60,    -1,    -1,   882,    -1,    -1,   885,    -1,
      -1,    -1,    -1,    -1,   891,     3,     4,     5,     6,    -1,
      -1,    -1,    -1,   900,   901,   902,    -1,   281,   282,   283,
     284,   285,   286,    -1,   288,   289,   290,   291,   292,   293,
     294,   295,   296,   297,   298,   299,   300,   301,   302,   303,
     304,   305,   306,   307,   308,    43,   310,   311,    -1,    -1,
      -1,     1,    50,     3,     4,     5,     6,     7,     8,     9,
      -1,    59,    60,   950,    -1,    15,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    29,
      -1,    31,    32,    33,    -1,    83,    -1,    -1,    38,    39,
      88,   516,    -1,    43,    -1,    -1,   521,    47,    48,    -1,
      50,    -1,    -1,    -1,    -1,    -1,    56,    -1,    -1,    59,
      60,    -1,    -1,    63,    -1,    65,    -1,    -1,  1005,     3,
       4,     5,     6,    73,    -1,     9,    -1,   552,    -1,    -1,
      -1,    -1,    -1,    83,   559,    -1,    -1,    -1,    88,    -1,
      -1,  1028,  1029,    -1,    94,  1032,    -1,    -1,    32,    -1,
      -1,    -1,  1039,    -1,    -1,    -1,  1043,    -1,  1045,    43,
      -1,   111,  1049,  1050,  1051,    49,    -1,    -1,  1055,    -1,
    1057,    -1,    56,    -1,    -1,    59,    60,   602,   603,    -1,
     605,    -1,   840,    -1,    -1,   843,    -1,    -1,  1075,    73,
    1077,    -1,    -1,    -1,    -1,     3,     4,     5,     6,    83,
      -1,   859,    -1,    -1,    88,    -1,    -1,    -1,    -1,    -1,
      94,    95,    -1,  1100,  1101,  1102,  1103,  1104,  1105,  1106,
      -1,    -1,    -1,  1110,    -1,  1112,  1113,  1114,    -1,    -1,
      -1,  1118,    40,    41,    -1,    43,  1123,  1124,  1125,  1126,
      -1,    -1,    -1,    -1,     3,     4,     5,     6,    56,    -1,
       9,    59,    60,  1140,    -1,    -1,  1143,  1144,    -1,  1146,
      -1,   202,   203,    -1,  1151,  1152,    -1,  1154,    -1,     3,
       4,     5,     6,    32,    -1,     3,     4,     5,     6,    -1,
    1167,     9,  1169,    -1,    43,    -1,    -1,    -1,    -1,    -1,
      49,    -1,    -1,    -1,    -1,    -1,    -1,    56,   723,   724,
      59,    60,    -1,    -1,    32,    -1,    -1,   732,    -1,    43,
      -1,    -1,   576,    -1,    73,    43,    50,    -1,    -1,    -1,
    1207,    49,    -1,    -1,    83,    59,    60,    -1,    56,    88,
      -1,    59,    60,    -1,   598,    94,    95,    -1,     3,     4,
       5,     6,    -1,   607,    -1,    73,    -1,  1234,  1235,    83,
    1237,    -1,    -1,    -1,    88,    83,    -1,    -1,    -1,    -1,
      88,  1248,    -1,    -1,    -1,    -1,    94,    95,    -1,  1256,
    1028,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    43,  1266,
    1267,  1268,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     815,    56,    -1,    -1,    59,    60,    -1,  1055,    -1,    -1,
      65,     3,     4,     5,     6,    -1,    -1,     9,    73,    -1,
      -1,    -1,  1299,  1300,  1301,  1302,  1303,  1304,    83,    -1,
    1307,  1308,    -1,    88,    -1,   850,    -1,    -1,    -1,    94,
      32,    -1,    -1,    -1,    -1,    -1,  1323,  1324,  1325,  1326,
    1327,    43,  1100,    -1,    -1,    -1,    -1,    49,  1106,  1336,
      -1,    -1,    -1,  1340,    56,    -1,    -1,    59,    60,    -1,
      -1,    -1,  1349,    -1,    -1,    -1,    -1,   892,    -1,   894,
      -1,    73,    -1,    -1,    -1,  1362,    -1,    -1,     6,    -1,
      -1,    83,    -1,  1370,    -1,    -1,    88,    -1,    16,    -1,
      -1,    -1,    94,    95,    -1,    -1,    -1,    25,    26,     3,
       4,     5,     6,    -1,    -1,    33,    34,    -1,    36,    -1,
      38,    -1,    70,    71,    72,    73,    74,    75,    76,    47,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      -1,    -1,    60,    -1,    62,    -1,    -1,    -1,    -1,    43,
      -1,    -1,    -1,    71,  1431,  1432,    50,  1434,    76,  1436,
      -1,  1438,    -1,    -1,    -1,    59,    60,    -1,    -1,    -1,
      -1,    89,    -1,    -1,    -1,   506,    -1,  1454,   509,    -1,
     511,   512,  1459,    -1,    -1,  1462,    -1,    -1,    -1,    83,
      -1,    -1,   523,    -1,    88,    -1,  1011,  1012,    -1,  1014,
    1248,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1256,    79,
      80,    81,    82,    83,    84,    85,    86,    87,  1495,  1496,
    1497,  1498,  1499,    -1,   142,    -1,    -1,    -1,  1505,  1044,
     148,   149,    -1,    -1,    -1,    -1,   154,    -1,    -1,   157,
     158,   159,   896,   897,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1299,    -1,    -1,    -1,   173,    -1,    -1,    -1,   177,
    1308,    -1,    -1,    -1,    -1,   183,   184,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1551,    -1,    -1,  1554,  1326,  1327,
      -1,    -1,    -1,    -1,   202,   203,   204,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1109,    -1,    -1,     1,    -1,    -1,
       4,     5,    -1,    -1,     8,     9,    -1,    -1,    -1,    -1,
      -1,    15,    75,    76,   232,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    29,    -1,    31,    32,    -1,
    1607,  1608,  1609,    -1,    38,    -1,    40,    41,    -1,    -1,
      -1,    -1,    -1,    47,    -1,    49,    -1,    -1,    -1,    -1,
      -1,    -1,    56,    -1,   272,    59,    60,    -1,    62,    -1,
      -1,    -1,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    -1,    87,    88,    89,    90,  1664,     4,     5,
    1438,    95,    96,     9,    -1,    -1,   314,   315,    -1,    -1,
      -1,    -1,   320,    -1,    -1,    -1,    -1,   325,   112,    -1,
      71,    72,    73,    74,    75,    76,    32,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    -1,  1705,  1706,
    1707,    -1,    -1,    49,    -1,    -1,    -1,  1252,    -1,    -1,
      56,  1718,    -1,    59,    60,    -1,    -1,    -1,    -1,    -1,
    1498,  1499,    -1,    -1,    -1,   373,    -1,    73,   376,    -1,
     378,    -1,    -1,   381,   382,    -1,    -1,    83,    -1,   387,
      -1,     3,     4,     5,     6,    -1,    -1,     9,    94,    95,
     811,   812,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1305,   409,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      32,   419,    -1,   421,   422,    -1,  1554,    -1,    -1,    -1,
      -1,    43,   430,   844,   845,   846,   847,    49,    -1,    -1,
    1335,   852,   440,    -1,    56,   443,    -1,    59,    60,    -1,
      -1,    -1,    -1,  1348,    -1,    -1,    -1,   455,    -1,    -1,
      -1,    73,    -1,    -1,    -1,   463,    -1,    -1,    -1,    -1,
      -1,    83,    -1,    -1,    -1,    -1,    88,    -1,    -1,    -1,
    1375,    -1,    94,    95,  1379,    -1,    -1,    -1,    -1,    -1,
      -1,   489,   490,   491,   492,   493,    -1,    -1,    -1,   497,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   505,   506,    -1,
      -1,   509,    -1,   511,   512,   513,   514,    -1,    -1,    -1,
     518,    -1,  1417,    -1,    -1,   523,    -1,    -1,   526,    -1,
      -1,     1,    -1,     3,     4,     5,     6,     7,     8,     9,
      -1,     3,     4,     5,     6,    15,    -1,    -1,    -1,   547,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    29,
      -1,    31,    32,    33,   562,    -1,    -1,    -1,    38,    39,
      32,  1466,  1467,    43,    -1,    -1,    -1,    47,    48,    -1,
      50,    43,    -1,    -1,    -1,    -1,    56,    49,    -1,    59,
      60,    -1,    -1,    63,    56,    65,    -1,    59,    60,    13,
      -1,    -1,    -1,    73,    -1,    -1,    -1,    -1,  1503,  1504,
      -1,    73,    -1,    83,  1509,    -1,    -1,    -1,    88,    -1,
      -1,    83,   620,    -1,    94,    -1,    88,    -1,    -1,   627,
      -1,    -1,    94,    95,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   111,    -1,    -1,    -1,   643,   644,   645,   646,    -1,
      -1,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    -1,    -1,    -1,    -1,     3,     4,
       5,     6,    -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,
    1101,  1102,  1103,  1104,    -1,    -1,    -1,    -1,    -1,  1110,
      -1,  1112,  1113,    -1,   702,    -1,   704,    32,    -1,    -1,
      -1,    -1,    -1,  1124,  1125,  1126,    -1,    -1,    43,    -1,
     718,   719,   720,    -1,  1619,    -1,    -1,    -1,    -1,    -1,
      -1,    56,    -1,   731,    59,    60,    -1,    -1,    -1,    -1,
      -1,    -1,     3,     4,     5,     6,   744,     1,    73,     3,
       4,     5,     6,     7,     8,     9,    -1,   755,    83,    -1,
      -1,    15,    -1,    88,    -1,   763,    -1,    -1,    -1,    94,
      -1,    32,    -1,   771,   772,    29,    -1,    31,    32,    33,
      -1,    -1,    43,    -1,    38,    39,    -1,    -1,    -1,    43,
      -1,    -1,    -1,    47,    48,    56,    50,    -1,    59,    60,
      -1,    -1,    56,    -1,    -1,    59,    60,    -1,    -1,    63,
      -1,    65,    73,   811,   812,   813,    -1,    -1,    -1,    73,
      -1,    -1,    83,   821,    -1,    -1,    -1,    88,    -1,    83,
       4,     5,    -1,    94,    88,     9,    -1,    -1,    -1,    -1,
      94,    -1,   840,    -1,    -1,   843,   844,   845,   846,   847,
     848,    -1,    -1,    -1,   852,    -1,    -1,   111,    32,    -1,
      -1,   859,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    -1,    49,    -1,    -1,    -1,    -1,
      -1,    -1,    56,    -1,   882,    59,    60,   885,    -1,  1300,
    1301,  1302,  1303,   891,    -1,    -1,  1307,    -1,    -1,    73,
      -1,    -1,   900,   901,   902,    -1,    -1,    -1,    -1,    83,
      -1,    -1,  1323,  1324,    -1,    -1,    -1,    -1,    -1,    -1,
      94,    95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     1,
      -1,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   950,    -1,    -1,    -1,    -1,    29,    30,    31,
      32,    33,    -1,    35,    36,    37,    38,    -1,    40,    41,
      42,    43,    44,    45,    -1,    47,    -1,    -1,    -1,    51,
      52,    53,    54,    55,    56,    -1,    -1,    59,    60,    -1,
      -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,    70,    -1,
      -1,    73,    -1,    -1,    -1,    -1,    -1,  1005,    -1,    81,
      82,    83,    -1,    -1,    -1,    -1,    88,    89,    90,    -1,
    1431,  1432,    94,  1434,   320,    -1,    -1,    -1,    -1,    -1,
    1028,  1029,    -1,    -1,  1032,    -1,    -1,    -1,   110,    -1,
     112,  1039,    -1,   339,   340,  1043,    -1,  1045,    -1,    -1,
      -1,  1049,  1050,  1051,    -1,    -1,    -1,  1055,   354,  1057,
      -1,   357,   358,    -1,    -1,    -1,    -1,   363,   364,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1075,   374,  1077,
      -1,    -1,    -1,    -1,  1495,  1496,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1095,    -1,    -1,
      -1,    -1,  1100,  1101,  1102,  1103,  1104,  1105,  1106,    -1,
      -1,    -1,  1110,    -1,  1112,  1113,  1114,    -1,    -1,    -1,
    1118,    -1,    -1,    -1,    -1,  1123,  1124,  1125,  1126,    -1,
       1,    -1,     3,     4,     5,     6,     7,     8,     9,  1137,
      -1,    -1,  1140,    -1,    15,  1143,  1144,    -1,  1146,    -1,
      -1,    -1,    -1,  1151,  1152,    -1,  1154,    -1,    29,    -1,
      31,    32,    33,    -1,    -1,    -1,    -1,    38,    39,  1167,
      -1,  1169,    43,    -1,    -1,    -1,    47,    48,    -1,    50,
      -1,    -1,    -1,     1,    -1,    56,     4,     5,    59,    60,
       8,     9,    63,    -1,    65,    -1,    -1,    15,    -1,    -1,
      -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,  1207,
      -1,    29,    83,    31,    32,    -1,    -1,    88,    -1,    -1,
      38,     4,     5,    94,     7,     8,     9,    -1,    -1,    47,
      13,    -1,    15,    -1,    -1,    -1,  1234,  1235,    56,  1237,
     111,    59,    60,    -1,    -1,    -1,    29,    -1,    31,    32,
    1248,    -1,    -1,    -1,    -1,    38,    -1,    -1,  1256,    -1,
      -1,    -1,    -1,    -1,    47,    -1,    49,    -1,  1266,  1267,
    1268,    -1,    -1,    56,    -1,    -1,    59,    60,    -1,    -1,
      -1,    -1,    -1,    -1,     3,     4,     5,     6,    -1,    -1,
      73,    -1,    -1,    -1,  1705,  1706,    -1,    -1,    -1,    -1,
      83,  1299,  1300,  1301,  1302,  1303,  1304,    -1,    -1,  1307,
    1308,    94,    95,    32,    -1,    -1,    -1,    -1,   614,    -1,
      -1,    -1,    -1,    -1,    43,  1323,  1324,  1325,  1326,  1327,
      -1,    -1,    -1,  1331,    -1,     6,    -1,    56,  1336,    -1,
      59,    60,  1340,    -1,     3,     4,     5,     6,     7,     8,
       9,  1349,    -1,    -1,    73,    -1,    15,   653,   654,    -1,
      -1,    -1,    33,    34,    83,    36,    -1,    38,    -1,    88,
      -1,    -1,  1370,    32,    -1,    94,    47,    -1,    -1,    38,
      -1,    -1,    -1,    -1,    43,    -1,    -1,    -1,    47,    60,
      49,    62,    -1,    -1,     4,     5,    -1,    56,    -1,     9,
      59,    60,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    84,    73,    -1,    87,    -1,    -1,    -1,
      -1,    -1,    32,    -1,    83,    -1,    -1,    -1,  1426,    88,
      -1,    -1,    -1,  1431,  1432,    94,  1434,    -1,  1436,    49,
    1438,    -1,    -1,    -1,    -1,    -1,    56,    -1,    -1,    59,
      60,    -1,    -1,    -1,    -1,    -1,  1454,    -1,    -1,    -1,
      -1,  1459,    -1,    73,  1462,    -1,    -1,    -1,   139,   140,
      -1,   142,    -1,    83,    -1,    -1,    -1,   148,   149,    -1,
      -1,   152,   153,   154,    94,    95,   157,   158,   159,    -1,
      -1,    -1,    -1,   164,    -1,    -1,    -1,  1495,  1496,  1497,
    1498,  1499,   173,    -1,    -1,    -1,   177,  1505,    -1,    -1,
      -1,    -1,   183,   184,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    13,    -1,    15,    -1,    -1,    -1,
      -1,   202,   203,   204,    -1,    -1,    -1,    -1,    -1,    -1,
      29,   212,    31,    32,    -1,    -1,    -1,    -1,    -1,    38,
      -1,    -1,    -1,  1551,    43,    -1,  1554,    -1,    47,    -1,
      49,   232,    -1,    -1,    -1,    -1,    -1,    56,    -1,    -1,
      59,    60,    76,    -1,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    73,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    83,    -1,    -1,    -1,  1596,    88,
      -1,   272,    -1,    -1,    -1,    94,    95,    -1,   279,  1607,
    1608,  1609,   908,   909,    -1,   911,   912,   913,   914,   915,
     916,   917,   918,   919,   920,   921,   922,   923,   924,   925,
     926,   927,   928,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     1,    -1,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,   325,    -1,    15,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1664,    -1,    -1,    28,
      29,    -1,    31,    32,    33,    -1,    -1,    -1,    -1,    38,
      -1,    -1,    -1,    -1,    43,    -1,    -1,    46,    47,    48,
     361,    50,    -1,    -1,    -1,    -1,    -1,    56,    57,    -1,
      59,    60,    -1,    -1,    63,   376,    -1,  1705,  1706,  1707,
      -1,    -1,    -1,    -1,    73,   386,    -1,    -1,     4,     5,
    1718,  1017,    -1,     9,    83,    -1,    -1,    -1,    -1,    88,
      -1,    -1,    -1,    -1,    -1,    94,    -1,    -1,   409,    -1,
      99,    -1,     3,     4,     5,     6,    32,    -1,     9,    -1,
     421,    -1,    -1,  1049,  1050,    -1,    -1,    -1,    -1,   430,
      -1,   432,    -1,    49,    -1,    -1,    -1,    -1,   439,   440,
      56,    32,   443,    59,    60,    -1,    -1,    -1,   449,   450,
     451,    -1,    43,    -1,   455,  1081,    -1,    73,    -1,    -1,
      -1,    -1,   463,    -1,    -1,    56,    -1,    83,    59,    60,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    94,    95,
      -1,    -1,    73,    -1,    -1,    -1,  1112,  1113,   489,   490,
     491,   492,    83,    -1,    -1,    -1,   497,    88,    -1,    -1,
      -1,    -1,    -1,    94,   505,   506,    -1,    -1,   509,    -1,
     511,   512,   513,   514,    -1,    -1,    -1,   518,    -1,     6,
      -1,    -1,   523,    -1,    -1,   526,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     1,    -1,     3,     4,     5,     6,
       7,     8,     9,    -1,    -1,    -1,    33,    34,    15,    36,
      -1,    38,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      47,   562,    29,    -1,    31,    32,    33,    -1,    -1,    -1,
      -1,    38,    -1,    60,    -1,    62,    43,    -1,    -1,    -1,
      47,    -1,    -1,    50,    -1,    -1,    -1,    -1,    -1,    56,
      -1,    -1,    59,    60,    -1,    -1,    -1,    -1,    -1,    -1,
      87,    -1,    -1,    -1,    -1,    -1,    73,     3,     4,     5,
       6,     7,     8,     9,  1240,    -1,    83,    13,   619,    15,
      -1,    88,    -1,    -1,    -1,    -1,   627,    94,    -1,    -1,
      -1,    -1,    99,    29,    -1,    31,    32,    -1,    -1,    -1,
      -1,    -1,    38,    -1,    -1,    -1,    -1,    43,    -1,    -1,
      -1,    47,   653,   654,   655,   142,    -1,    -1,    -1,    -1,
      56,   148,   149,    59,    60,   666,   153,   154,    -1,    -1,
     157,   158,   159,    -1,    -1,    -1,    -1,    73,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   173,    83,    -1,    -1,
     177,    -1,    88,    -1,    -1,    -1,   183,   184,    94,    -1,
      -1,    -1,    -1,    -1,     1,    -1,     3,     4,     5,     6,
       7,     8,     9,   714,    -1,   202,   203,   204,    15,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   727,    -1,   729,    -1,
     731,    -1,    29,    -1,    31,    32,    -1,    -1,    -1,    -1,
      -1,    38,    -1,    -1,    -1,   232,    43,    -1,    -1,    -1,
      47,    -1,    -1,    50,   755,    -1,    -1,    -1,  1384,    56,
      -1,   762,    59,    60,    -1,    -1,    63,    -1,    -1,    -1,
     771,    -1,    -1,    -1,    -1,    -1,    73,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   272,    83,    -1,    -1,    -1,
      -1,    88,    72,    73,    74,    75,    76,    94,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    -1,    -1,
     811,   812,   813,    -1,    -1,    -1,    -1,   818,   819,    -1,
     821,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,     6,
      -1,    -1,     9,    -1,   835,    -1,    -1,    -1,   325,   840,
      -1,    -1,   843,   844,   845,   846,   847,   848,    -1,    -1,
      -1,   852,    -1,    -1,    -1,    32,    -1,    -1,   859,     3,
       4,     5,     6,     7,     8,     9,    43,    -1,    -1,    13,
      -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    56,
      -1,   882,    59,    60,    -1,    29,    -1,    31,    32,    -1,
      -1,    -1,    -1,    -1,    38,    -1,    73,    -1,    -1,    43,
     387,    -1,    -1,    47,    -1,    49,    83,    -1,    -1,    -1,
      -1,    88,    56,    -1,    -1,    59,    60,    94,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    73,
      -1,    -1,    -1,    -1,   421,    -1,    -1,     4,     5,    83,
       7,     8,     9,   430,    88,    -1,    13,    -1,    15,    -1,
      94,    95,    -1,   440,    -1,    -1,   443,    -1,    -1,    -1,
      -1,    -1,    29,    -1,    31,    32,    -1,    -1,   455,    -1,
      -1,    38,    -1,    -1,  1600,    -1,   463,    -1,    -1,    -1,
      47,    -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,    56,
      -1,    -1,    59,    60,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   489,   490,   491,   492,    73,     4,     5,    -1,
     497,    -1,     9,    -1,    -1,    -1,    83,    -1,   505,   506,
      -1,    -1,   509,    -1,   511,   512,   513,    94,    95,    -1,
      -1,   518,    -1,    -1,    -1,    32,   523,    -1,    -1,   526,
       4,     5,    -1,    -1,     8,     9,    -1,    -1,  1049,  1050,
    1051,    15,    49,    -1,  1055,  1056,    -1,    -1,    -1,    56,
      -1,    -1,    59,    60,    -1,    29,    -1,    31,    32,    -1,
      -1,    -1,    -1,    -1,    38,   562,    73,    -1,    -1,  1705,
    1706,    -1,    -1,    47,    36,    -1,    83,    -1,    -1,    -1,
      -1,    -1,    56,    -1,    -1,    59,    60,    94,    95,  1100,
    1101,  1102,  1103,  1104,  1105,  1106,    -1,  1108,    60,  1110,
    1111,  1112,  1113,  1114,    74,    75,    76,  1118,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    -1,    -1,
      94,    -1,    -1,   620,    -1,    -1,    -1,    -1,    -1,    -1,
     627,    -1,  1143,  1144,    -1,  1146,    -1,    -1,    -1,    -1,
    1151,  1152,    -1,  1154,    -1,     3,     4,     5,     6,     7,
       8,     9,    -1,    -1,    -1,    13,    -1,    15,    -1,    -1,
      -1,    -1,    -1,     3,     4,     5,     6,    -1,    -1,     9,
      -1,    29,    -1,    31,    32,    -1,    -1,    -1,    -1,    -1,
      38,    -1,    -1,     4,     5,    43,     7,     8,     9,    47,
      -1,    49,    32,    -1,    15,   157,   158,   159,    56,    -1,
      -1,    59,    60,    43,    -1,    -1,    -1,    -1,    29,    -1,
      31,    32,    -1,    -1,    -1,    73,    56,    38,   715,    59,
      60,   183,    -1,    -1,    -1,    83,    47,    -1,    -1,    50,
      88,    -1,    -1,    73,    -1,    56,    94,    95,    59,    60,
     202,   203,   204,    83,    -1,    -1,    -1,    -1,    88,    -1,
      -1,    -1,    -1,    -1,    94,  1266,  1267,  1268,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     232,    -1,    -1,    -1,   771,     4,     5,    -1,    -1,     8,
       9,    -1,    -1,    -1,    -1,    -1,    15,    -1,  1299,  1300,
    1301,  1302,  1303,  1304,    -1,    -1,  1307,  1308,    -1,    -1,
      29,    -1,    31,    32,    -1,    -1,    -1,    -1,    -1,    38,
      -1,    -1,    -1,    -1,   811,   812,   813,    -1,    47,    -1,
      49,    -1,   819,    -1,   821,    -1,    -1,    56,    -1,  1340,
      59,    60,    -1,     3,     4,     5,     6,    -1,    -1,    -1,
      -1,    -1,    -1,   840,    73,    -1,    -1,   844,   845,    -1,
    1361,  1362,    -1,   315,    83,   852,    -1,    -1,    -1,  1370,
      -1,    -1,   859,    -1,    -1,    94,    95,    -1,    -1,    73,
      74,    75,    76,    43,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    -1,   882,    56,    -1,    -1,    59,
      60,    -1,    -1,    -1,    -1,    65,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    83,    -1,    -1,   378,    -1,    88,   381,
     382,    -1,    -1,    -1,    94,    -1,     1,    -1,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    -1,    -1,  1454,    -1,    -1,  1457,    -1,  1459,    -1,
      -1,  1462,    -1,    -1,    29,    30,    31,    32,    33,   421,
      35,    36,    37,    38,    -1,    40,    41,    42,    43,    44,
      45,    -1,    47,    -1,    49,    -1,    51,    52,    53,    54,
      55,    56,    -1,    -1,    59,    60,    61,    -1,    -1,    64,
      -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    73,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    81,    82,    83,    -1,
      -1,    -1,    -1,    88,    89,    90,    -1,    -1,    -1,    94,
      95,    -1,    -1,    -1,    -1,    -1,    -1,   489,    -1,   491,
     492,   493,    -1,    -1,    -1,    -1,    -1,   112,    -1,    -1,
      -1,    -1,    -1,   505,   506,    -1,    -1,   509,    -1,   511,
     512,   513,   514,    -1,    -1,    -1,   518,    -1,    -1,    -1,
    1057,   523,    -1,    -1,   526,    -1,    -1,    60,    -1,    -1,
      -1,     3,     4,     5,     6,    -1,    -1,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    84,    -1,    -1,    87,    -1,  1607,  1608,  1609,    -1,
      32,    -1,    -1,  1100,  1101,  1102,  1103,  1104,  1105,    -1,
      -1,    43,    -1,  1110,    -1,  1112,  1113,  1114,   111,    -1,
     113,  1118,    -1,    -1,    56,    -1,    -1,    59,    60,    -1,
      -1,    -1,     3,     4,     5,     6,     7,     8,     9,    -1,
      -1,    73,    -1,    -1,    15,    -1,  1143,  1144,    -1,  1146,
      -1,    83,    -1,  1664,  1151,  1152,    88,  1154,    29,    -1,
      31,    32,    94,    -1,    -1,   627,    -1,    38,    -1,    -1,
      -1,    -1,    43,    -1,    -1,    -1,    47,    -1,    49,    -1,
       3,     4,     5,     6,    -1,    56,    -1,    -1,    59,    60,
     183,    -1,    -1,    -1,  1705,  1706,  1707,    -1,    -1,    -1,
      -1,    -1,    73,    -1,    -1,    -1,    -1,  1718,    -1,   202,
     203,   204,    83,    -1,    -1,    -1,    -1,    88,    -1,   212,
      43,    -1,    -1,    94,    95,    -1,    -1,     4,     5,    -1,
      -1,     8,     9,    56,    -1,    -1,    59,    60,    15,   232,
      63,    -1,   704,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      73,    -1,    -1,    -1,    -1,    32,   718,   719,   720,    -1,
      83,    38,    -1,    -1,    -1,    88,   259,    -1,    -1,   731,
      47,    94,    49,    -1,    -1,    -1,    -1,   198,    -1,    56,
      -1,    -1,    59,    60,    -1,    -1,   279,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    73,    -1,    -1,    -1,
      -1,    -1,  1299,  1300,  1301,    -1,    83,    -1,    -1,   771,
    1307,  1308,    -1,    -1,    -1,    -1,    -1,    94,    95,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,     3,     4,     5,     6,     7,
       8,     9,    -1,  1340,    -1,    -1,    -1,    15,    -1,   811,
     812,   813,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   821,
      -1,    29,    -1,    31,    32,  1362,    -1,    -1,    -1,    -1,
      38,    -1,    -1,  1370,    -1,    43,    -1,    -1,   840,    47,
      -1,   843,   844,   845,   846,   847,   848,    -1,    56,    -1,
     852,    59,    60,   386,   387,    63,    -1,   859,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    73,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    83,    -1,    -1,    -1,    -1,
      88,    -1,    -1,    -1,    -1,    -1,    94,    -1,   421,    -1,
      -1,   352,    -1,    -1,    -1,    -1,    -1,    -1,   900,   901,
     902,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,
       6,     7,     8,     9,    -1,    -1,    -1,  1454,    -1,    15,
      -1,    -1,  1459,    -1,    -1,  1462,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    29,    -1,    31,    32,    -1,    -1,    -1,
      -1,    -1,    38,    -1,    -1,    -1,    -1,    43,   950,    -1,
      -1,    47,    -1,    -1,    -1,    -1,   489,    -1,   491,   492,
      56,    -1,    -1,    59,    60,    -1,    -1,    -1,    -1,    65,
      -1,    -1,   505,   506,    -1,    -1,   509,    73,   511,   512,
     513,   514,    -1,    -1,    -1,   518,    -1,    83,    -1,    -1,
     523,    -1,    88,   526,    -1,    -1,    -1,    -1,    94,    -1,
       4,     5,    -1,    -1,     8,     9,    -1,    -1,    -1,    -1,
      -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,   479,   480,
      -1,    -1,    -1,    -1,    -1,    -1,  1028,  1029,    32,    -1,
    1032,    -1,    -1,    -1,    38,    -1,    -1,    -1,    -1,    -1,
      -1,  1043,    -1,    47,    -1,    49,    -1,  1049,  1050,  1051,
      -1,    -1,    56,  1055,    -1,    59,    60,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    73,
    1607,  1608,  1609,    -1,    -1,    -1,    -1,    -1,    -1,    83,
      -1,    -1,    -1,    -1,    -1,    -1,   619,   620,    -1,    -1,
      94,    95,    -1,    -1,   627,    -1,    -1,    -1,  1100,  1101,
    1102,  1103,  1104,  1105,  1106,    -1,    -1,    -1,  1110,    -1,
    1112,  1113,  1114,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1124,  1125,  1126,    -1,    -1,  1664,    -1,    -1,
      -1,     4,     5,    -1,     7,     8,     9,    -1,    -1,    -1,
      13,  1143,    15,    -1,    -1,    -1,    -1,    -1,    -1,  1151,
      -1,    -1,    -1,    -1,    -1,    -1,    29,    -1,    31,    32,
     621,    -1,    -1,    -1,    -1,    38,    -1,    -1,  1705,  1706,
    1707,    -1,    -1,    -1,    47,    -1,    -1,   638,    -1,    -1,
     641,  1718,    -1,    56,    -1,    -1,    59,    60,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   734,   735,   664,   665,    -1,    -1,   668,   669,   670,
      -1,   672,   673,   674,   675,   676,   677,   678,   679,   680,
     681,   682,   683,   684,   685,   686,   687,   688,   689,   690,
      -1,   692,  1234,  1235,    -1,  1237,   697,    -1,   771,   700,
     701,    -1,    -1,    -1,    -1,    -1,  1248,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1256,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1266,  1267,  1268,    -1,    -1,    -1,
     731,    -1,    -1,    -1,    -1,    -1,    -1,   738,   811,   812,
     813,    -1,    -1,    -1,    -1,   818,   819,    -1,   821,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1299,  1300,  1301,
    1302,  1303,  1304,    -1,    -1,  1307,  1308,   840,    -1,    -1,
     843,   844,   845,   846,   847,   848,   849,    -1,    -1,   852,
      -1,  1323,  1324,  1325,  1326,  1327,   859,    -1,     3,     4,
       5,     6,     7,     8,     9,    -1,    -1,    -1,  1340,    -1,
      15,   802,    -1,    -1,    -1,    -1,    -1,    -1,   881,    -1,
      -1,    -1,    -1,    -1,    29,    -1,    31,    32,    -1,    -1,
     821,    -1,    -1,    38,    -1,    -1,    -1,    -1,    43,    -1,
      -1,    -1,    47,    -1,   835,    -1,    -1,    -1,    -1,    -1,
      -1,    56,    -1,    -1,    59,    60,    -1,    -1,    63,    -1,
      -1,    -1,    -1,    -1,    16,    -1,    -1,    -1,    73,    -1,
      -1,    -1,    -1,    25,    26,    -1,    -1,    -1,    83,    -1,
      -1,    33,    34,    88,    36,    -1,    -1,    -1,    -1,    94,
      -1,     3,     4,     5,     6,    -1,    -1,     9,    -1,  1431,
    1432,    -1,  1434,    -1,    -1,    -1,  1438,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    71,
      32,    -1,  1454,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    43,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    56,    -1,    -1,    59,    60,   940,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    73,    -1,  1495,  1496,  1497,  1498,  1499,    -1,    -1,
      -1,    83,    -1,    -1,    -1,    -1,    88,   968,   130,    -1,
     132,    -1,    94,    -1,    -1,    -1,    -1,   139,   140,    -1,
      -1,    -1,    -1,  1056,  1057,  1058,   148,   149,    -1,    -1,
     152,   153,   154,    -1,   156,   157,   158,   159,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1008,  1009,    -1,
      -1,    -1,  1554,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1023,    -1,  1025,    -1,  1027,  1100,  1101,  1102,
    1103,  1104,  1105,  1106,    -1,  1108,    -1,  1110,  1111,  1112,
    1113,  1114,    -1,    -1,    -1,  1118,    -1,    -1,    -1,    -1,
      -1,  1124,  1125,  1126,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1062,    -1,    -1,    -1,  1607,    -1,  1609,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1151,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   248,   249,    -1,    -1,
      -1,    -1,    -1,    -1,   378,    -1,    -1,   381,   382,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,
     272,    15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1120,
    1121,    -1,  1664,    -1,    -1,    29,    -1,    31,    32,    -1,
      -1,    -1,    -1,    -1,    38,    -1,    -1,    -1,    -1,    43,
      -1,  1142,    -1,    47,    -1,    -1,     4,     5,    -1,    -1,
       8,     9,    56,   315,    -1,    59,    60,    15,    -1,    -1,
      -1,    -1,    -1,  1705,  1706,  1707,    -1,    -1,    -1,    73,
      -1,    29,    -1,    31,    32,    -1,  1718,    -1,    -1,    83,
      38,    -1,    -1,    -1,    88,    -1,    -1,    -1,    -1,    47,
      94,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    56,    -1,
      -1,    59,    60,    61,    -1,   489,    -1,   491,   492,   493,
      -1,    -1,    -1,    -1,   376,    -1,   378,  1218,  1219,   381,
      -1,  1222,    -1,    -1,    -1,    -1,  1299,  1300,  1301,  1302,
    1303,  1304,    -1,    -1,  1307,  1308,    94,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   409,    -1,    -1,
    1323,  1324,  1325,  1326,  1327,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1340,   430,    -1,
     432,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   440,     4,
       5,   443,     7,     8,     9,    -1,    -1,    -1,  1361,    -1,
      15,    -1,    -1,   455,    -1,    -1,  1297,     3,     4,     5,
       6,     7,     8,     9,    29,    -1,    31,    32,    -1,    15,
      -1,    -1,    -1,    38,    -1,    -1,     3,     4,     5,     6,
      -1,    -1,    47,    29,    -1,    31,    32,  1328,    -1,    -1,
      -1,    56,    38,    -1,    59,    60,    -1,    43,    -1,    -1,
      -1,    47,    -1,    -1,    -1,    32,    -1,    -1,    -1,    -1,
      56,  1352,    -1,    59,    60,    -1,    43,    -1,  1431,  1432,
    1433,  1434,    -1,    -1,    -1,  1438,    -1,    73,    -1,    56,
      -1,    -1,    59,    60,    -1,    -1,    -1,    83,    -1,    -1,
      -1,  1454,    88,    -1,   546,   547,    73,    -1,    94,    -1,
      -1,    -1,  1393,    -1,    -1,    -1,    83,    -1,    -1,    -1,
     562,    88,    -1,     3,     4,     5,     6,    94,    -1,    -1,
      -1,    -1,     3,     4,     5,     6,    -1,    -1,  1419,    -1,
      -1,    -1,  1495,  1496,  1497,  1498,  1499,  1428,    -1,    -1,
      -1,    -1,    32,    -1,   718,   719,   720,    -1,    -1,  1440,
      -1,    32,   604,    43,   606,    -1,    -1,   731,    -1,  1450,
    1451,    -1,    43,    -1,    -1,    -1,    56,    -1,    -1,    59,
      60,    -1,    -1,    -1,    -1,    56,    -1,    -1,    59,    60,
      -1,    -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,
    1481,  1554,    73,    83,    -1,    -1,    -1,   771,    88,    -1,
      -1,    -1,    83,    -1,    94,    -1,    -1,    88,    -1,    -1,
      -1,    -1,    -1,    94,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1516,    -1,    -1,    -1,    -1,
      -1,     4,     5,    -1,     7,     8,     9,   811,   812,   813,
      13,    -1,    15,    -1,  1607,    -1,  1609,   821,    -1,    -1,
       4,     5,   704,    -1,     8,     9,    29,    -1,    31,    32,
      -1,    15,   714,   715,    -1,    38,   718,   719,    -1,    -1,
      -1,  1562,  1563,    -1,    47,    29,    -1,    31,    32,    -1,
      -1,    -1,    -1,    56,    38,    -1,    59,    60,    -1,    -1,
      -1,    -1,    -1,    47,    -1,    -1,    -1,    -1,  1589,    -1,
      -1,  1664,    56,   755,    -1,    59,    60,    -1,    -1,    -1,
     762,   763,    -1,     4,     5,    -1,    -1,     8,     9,    -1,
     772,    -1,    -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1627,    -1,    29,    -1,
      31,    32,  1705,  1706,  1707,    -1,    -1,    38,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1718,    47,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    56,    32,    -1,    59,    60,
      -1,    -1,    -1,  1664,    -1,    -1,   950,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    -1,    -1,    -1,  1689,    -1,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   881,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    -1,   900,   901,
     902,   903,    -1,    -1,  1028,  1029,    -1,    -1,  1032,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1043,
      -1,    -1,    -1,    -1,    -1,  1049,  1050,  1051,    -1,    -1,
      -1,  1055,    -1,    -1,    -1,    -1,    -1,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,   950,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,  1800,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    -1,  1100,  1101,  1102,  1103,
    1104,  1105,  1106,    -1,    -1,    -1,  1110,    -1,  1112,  1113,
    1114,    -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    -1,    13,
      -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    26,    -1,    28,    -1,  1028,  1029,    32,    -1,
    1032,    -1,    -1,    -1,    38,    -1,    -1,    -1,    -1,    43,
      -1,  1043,    -1,    47,    -1,    49,    -1,    -1,    -1,    -1,
      -1,    -1,    56,  1055,    -1,    59,    60,    61,    62,    63,
      -1,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    -1,    -1,    93,
      94,    95,    96,  1095,    98,    -1,    -1,    -1,    -1,   103,
     104,    -1,    -1,    -1,    -1,   109,   110,   111,    -1,   113,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,  1248,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1256,    -1,    -1,  1137,    -1,    -1,  1140,    -1,
      -1,  1143,  1266,  1267,  1268,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1299,  1300,  1301,  1302,  1303,
    1304,    -1,    -1,  1307,  1308,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   113,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1234,  1235,    -1,  1237,   109,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1248,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1256,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    62,    -1,  1266,  1267,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1431,  1432,    -1,
    1434,    -1,    -1,     3,     4,     5,     6,    -1,     8,     9,
      10,    11,    12,    -1,    14,    15,    -1,    -1,    -1,  1331,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    29,
      30,    31,    32,    33,    -1,    35,    36,    37,    38,    -1,
      40,    41,    42,    43,    44,    45,    -1,    47,    -1,    49,
    1362,    51,    52,    53,    54,    55,    56,    -1,    -1,    59,
      60,  1495,  1496,  1497,  1498,  1499,    -1,    -1,    -1,    -1,
      70,    -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    81,    82,    83,    -1,    -1,    -1,    -1,    88,    89,
      90,    -1,    -1,    -1,    94,    95,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   112,    -1,  1426,    -1,    -1,    62,    -1,    -1,
    1554,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    -1,    -1,  1457,     1,  1459,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    -1,    14,
      15,    16,    -1,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    -1,
      35,    36,    37,    38,    -1,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    -1,    -1,    51,    52,    53,    54,
      55,    56,    -1,    -1,    59,    60,    61,    -1,    63,    64,
      -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    73,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    81,    82,    83,    -1,
      -1,    -1,    -1,    88,    89,    90,    -1,    -1,    -1,    94,
      -1,    -1,    -1,    98,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   111,   112,     1,    -1,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      -1,    14,    15,    16,    -1,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    -1,    35,    36,    37,    38,    -1,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    -1,    -1,    51,    52,
      53,    54,    55,    56,    -1,    -1,    59,    60,    61,    -1,
      63,    64,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,
      73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,    82,
      83,    -1,    -1,    -1,    -1,    88,    89,    90,    -1,    -1,
      -1,    94,    -1,    -1,     1,    98,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,   112,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    29,    30,    31,    32,    33,    -1,    35,    36,
      37,    38,    -1,    40,    41,    42,    43,    44,    45,    -1,
      47,    -1,    49,    -1,    51,    52,    53,    54,    55,    56,
      -1,    -1,    59,    60,    61,    -1,    -1,    64,    -1,    -1,
      -1,    -1,    -1,    70,    -1,    -1,    73,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    81,    82,    83,    -1,    -1,    -1,
      -1,    88,    89,    90,    -1,    -1,    -1,    94,    95,     1,
      -1,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    -1,    14,    15,    -1,   112,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    29,    30,    31,
      32,    33,    -1,    35,    36,    37,    38,    -1,    40,    41,
      42,    43,    44,    45,    -1,    47,    -1,    -1,    -1,    51,
      52,    53,    54,    55,    56,    -1,    -1,    59,    60,    61,
      -1,    63,    64,    -1,    -1,    -1,    -1,    -1,    70,    -1,
      -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,
      82,    83,    -1,    -1,    -1,    -1,    88,    89,    90,    -1,
      -1,     1,    94,     3,     4,     5,     6,    -1,     8,     9,
      10,    11,    12,    -1,    14,    15,    -1,    -1,    -1,    -1,
     112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    29,
      30,    31,    -1,    33,    -1,    35,    36,    37,    38,    -1,
      40,    41,    42,    43,    44,    45,    -1,    47,    -1,    -1,
      -1,    51,    52,    53,    54,    55,    56,    -1,    -1,    59,
      60,    61,    -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,
      70,    -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    81,    82,    83,    -1,    -1,    -1,    -1,    88,    89,
      90,    -1,    -1,    -1,    94,    95,     1,    -1,     3,     4,
       5,     6,    -1,     8,     9,    10,    11,    12,    -1,    14,
      15,   111,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    29,    30,    31,    32,    33,    -1,
      35,    36,    37,    38,    -1,    40,    41,    42,    43,    44,
      45,    -1,    47,    -1,    -1,    -1,    51,    52,    53,    54,
      55,    56,    -1,    -1,    59,    60,    61,    -1,    -1,    64,
      -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    73,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    81,    82,    83,    -1,
      -1,    -1,    -1,    88,    89,    90,    -1,    -1,     1,    94,
       3,     4,     5,     6,    -1,     8,     9,    10,    11,    12,
      -1,    14,    15,    -1,    -1,    -1,    -1,   112,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    29,    30,    31,    -1,
      33,    -1,    35,    36,    37,    38,    -1,    40,    41,    42,
      43,    44,    45,    -1,    47,    -1,    -1,    -1,    51,    52,
      53,    54,    55,    56,    -1,    -1,    59,    60,    61,    -1,
      -1,    64,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,
      73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,    82,
      83,    -1,    -1,    -1,    -1,    88,    89,    90,    -1,    -1,
      -1,    94,    95,     1,    -1,     3,     4,     5,     6,    -1,
       8,     9,    10,    11,    12,    -1,    14,    15,    -1,   112,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    29,    30,    31,    -1,    33,    -1,    35,    36,    37,
      38,    -1,    40,    41,    42,    43,    44,    45,    -1,    47,
      -1,    -1,    -1,    51,    52,    53,    54,    55,    56,    -1,
      -1,    59,    60,    61,    -1,    -1,    64,    -1,    -1,    -1,
      -1,    -1,    70,    -1,    -1,    73,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    81,    82,    83,    -1,    -1,    -1,    -1,
      88,    89,    90,    -1,    -1,    -1,    94,    -1,     1,    -1,
       3,     4,     5,     6,   102,     8,     9,    10,    11,    12,
      -1,    14,    15,    -1,   112,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    29,    30,    31,    -1,
      33,    -1,    35,    36,    37,    38,    -1,    40,    41,    42,
      43,    44,    45,    -1,    47,    -1,    -1,    -1,    51,    52,
      53,    54,    55,    56,    -1,    -1,    59,    60,    61,    -1,
      -1,    64,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,
      73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,    82,
      83,    -1,    -1,    -1,    -1,    88,    89,    90,    -1,    -1,
       1,    94,     3,     4,     5,     6,    -1,     8,     9,    10,
      11,    12,    -1,    14,    15,    -1,    -1,    -1,   111,   112,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    29,    30,
      31,    -1,    33,    -1,    35,    36,    37,    38,    -1,    40,
      41,    42,    43,    44,    45,    -1,    47,    -1,    -1,    -1,
      51,    52,    53,    54,    55,    56,    -1,    -1,    59,    60,
      61,    -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,    70,
      -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      81,    82,    83,    -1,    -1,    -1,    -1,    88,    89,    90,
      -1,    -1,     1,    94,     3,     4,     5,     6,    -1,     8,
       9,    10,    11,    12,    -1,    14,    15,    -1,    -1,    -1,
     111,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      29,    30,    31,    32,    33,    -1,    35,    36,    37,    38,
      -1,    40,    41,    42,    43,    44,    45,    -1,    47,    -1,
      -1,    -1,    51,    52,    53,    54,    55,    56,    -1,    -1,
      59,    60,    61,    -1,    -1,    64,    -1,    -1,    -1,    -1,
      -1,    70,    -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    81,    82,    83,    -1,    -1,    -1,    -1,    88,
      89,    90,    -1,    -1,     1,    94,     3,     4,     5,     6,
      -1,     8,     9,    10,    11,    12,    -1,    14,    15,    -1,
      -1,    -1,    -1,   112,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    29,    30,    31,    32,    33,    -1,    35,    36,
      37,    38,    -1,    40,    41,    42,    43,    44,    45,    -1,
      47,    -1,    -1,    -1,    51,    52,    53,    54,    55,    56,
      -1,    -1,    59,    60,    -1,    -1,    63,    64,    -1,    -1,
      -1,    -1,    -1,    70,    -1,    -1,    73,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    81,    82,    83,    -1,    -1,    -1,
      -1,    88,    89,    90,    -1,    -1,     1,    94,     3,     4,
       5,     6,    -1,     8,     9,    10,    11,    12,    -1,    14,
      15,    -1,    -1,    -1,    -1,   112,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    29,    30,    31,    -1,    33,    -1,
      35,    36,    37,    38,    -1,    40,    41,    42,    43,    44,
      45,    -1,    47,    -1,    -1,    -1,    51,    52,    53,    54,
      55,    56,    -1,    -1,    59,    60,    61,    -1,    -1,    64,
      -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    73,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    81,    82,    83,    -1,
      -1,    -1,    -1,    88,    89,    90,    -1,    -1,     1,    94,
       3,     4,     5,     6,    -1,     8,     9,    10,    11,    12,
      -1,    14,    15,    -1,    -1,    -1,    -1,   112,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    29,    30,    31,    -1,
      33,    -1,    35,    36,    37,    38,    -1,    40,    41,    42,
      43,    44,    45,    -1,    47,    -1,    -1,    -1,    51,    52,
      53,    54,    55,    56,    -1,    -1,    59,    60,    -1,    -1,
      -1,    64,    65,    -1,    -1,    -1,    -1,    70,    -1,    -1,
      73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,    82,
      83,    -1,    -1,    -1,    -1,    88,    89,    90,    -1,    -1,
       1,    94,     3,     4,     5,     6,    -1,     8,     9,    10,
      11,    12,    -1,    14,    15,    -1,    -1,    -1,    -1,   112,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    29,    30,
      31,    -1,    33,    -1,    35,    36,    37,    38,    -1,    40,
      41,    42,    43,    44,    45,    -1,    47,    -1,    -1,    -1,
      51,    52,    53,    54,    55,    56,    -1,    -1,    59,    60,
      -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,    70,
      -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      81,    82,    83,    -1,    -1,    -1,    -1,    88,    89,    90,
      -1,    -1,     1,    94,     3,     4,     5,     6,    -1,     8,
       9,    10,    11,    12,    -1,    14,    15,    -1,    -1,   110,
      -1,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      29,    30,    31,    -1,    33,    -1,    35,    36,    37,    38,
      -1,    40,    41,    42,    43,    44,    45,    -1,    47,    -1,
      -1,    -1,    51,    52,    53,    54,    55,    56,    -1,    -1,
      59,    60,    -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,
      -1,    70,    -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    81,    82,    83,    -1,    -1,    -1,    -1,    88,
      89,    90,    -1,    -1,     1,    94,     3,     4,     5,     6,
      -1,     8,     9,    10,    11,    12,    -1,    14,    15,    -1,
      -1,    -1,    -1,   112,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    29,    30,    31,    -1,    33,    -1,    35,    36,
      37,    38,    -1,    40,    41,    42,    43,    44,    45,    -1,
      47,    -1,    -1,    -1,    51,    52,    53,    54,    55,    56,
      -1,    -1,    59,    60,    -1,    -1,    -1,    64,    -1,    -1,
      -1,    -1,    -1,    70,    -1,    -1,    73,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    81,    82,    83,    -1,    -1,    -1,
      -1,    88,    89,    90,    -1,    -1,     1,    94,     3,     4,
       5,     6,    -1,     8,     9,    10,    11,    12,    -1,    14,
      15,    -1,    -1,    -1,    -1,   112,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    29,    30,    31,    -1,    33,    -1,
      35,    36,    37,    38,    -1,    40,    41,    42,    43,    44,
      45,    -1,    47,    -1,    -1,    -1,    51,    52,    53,    54,
      55,    56,    -1,    -1,    59,    60,    -1,    -1,    -1,    64,
      -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    73,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    81,    82,    83,    -1,
      -1,    -1,    -1,    88,    89,    90,    -1,    -1,    -1,    94,
       3,     4,     5,     6,    -1,     8,     9,    10,    11,    12,
      -1,    14,    15,    -1,    -1,    -1,    -1,   112,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    29,    30,    31,    -1,
      33,    -1,    35,    36,    37,    38,    -1,    40,    41,    42,
      43,    44,    45,    -1,    47,    -1,    -1,    -1,    51,    52,
      53,    54,    55,    56,    -1,    -1,    59,    60,    -1,    -1,
      -1,    64,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,
      73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,    82,
      83,    -1,    -1,    -1,    -1,    88,    89,    90,    -1,    -1,
      -1,    94,     3,     4,     5,     6,    -1,     8,     9,    10,
      11,    12,    -1,    14,    15,    -1,    -1,    -1,    -1,   112,
     113,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    29,    30,
      31,    -1,    33,    -1,    35,    36,    37,    38,    -1,    40,
      41,    42,    43,    44,    45,    -1,    47,    -1,    -1,    -1,
      51,    52,    53,    54,    55,    56,    -1,    -1,    59,    60,
      -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,    70,
      -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      81,    82,    83,    -1,    -1,    -1,    -1,    88,    89,    90,
      -1,    -1,    -1,    94,     3,     4,     5,     6,    -1,     8,
       9,    10,    11,    12,    -1,    14,    15,    -1,    -1,    -1,
      -1,   112,   113,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      29,    30,    31,    -1,    33,    -1,    35,    36,    37,    38,
      -1,    40,    41,    42,    43,    44,    45,    -1,    47,    -1,
      -1,    -1,    51,    52,    53,    54,    55,    56,    -1,    -1,
      59,    60,    -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,
      -1,    70,    -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    81,    82,    83,    -1,    -1,    -1,    -1,    88,
      89,    90,    -1,    -1,    -1,    94,     3,     4,     5,     6,
      -1,     8,     9,    10,    11,    12,    -1,    14,    15,    -1,
      -1,    -1,    -1,   112,   113,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    29,    30,    31,    -1,    33,    -1,    35,    36,
      37,    38,    -1,    40,    41,    42,    43,    44,    45,    -1,
      47,    -1,    -1,    -1,    51,    52,    53,    54,    55,    56,
      -1,    -1,    59,    60,    -1,    -1,    -1,    64,    -1,    -1,
      -1,    -1,    -1,    70,    -1,    -1,    73,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    81,    82,    83,    -1,    -1,    -1,
      -1,    88,    89,    90,    -1,    -1,    -1,    94,     3,     4,
       5,     6,    -1,     8,     9,    10,    11,    12,    -1,    14,
      15,    -1,    -1,    -1,    -1,   112,   113,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    29,    30,    31,    -1,    33,    -1,
      35,    36,    37,    38,    -1,    40,    41,    42,    43,    44,
      45,    -1,    47,    -1,    -1,    -1,    51,    52,    53,    54,
      55,    56,    -1,    -1,    59,    60,    -1,    -1,    -1,    64,
      -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    73,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    81,    82,    83,    -1,
      -1,    -1,    -1,    88,    89,    90,    -1,    -1,    -1,    94,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   112,   113,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    -1,
      14,    15,    16,    -1,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      -1,    35,    36,    37,    38,    -1,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    -1,    -1,    51,    52,    53,
      54,    55,    56,    -1,    -1,    59,    60,    61,    -1,    63,
      64,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    73,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,    82,    83,
      -1,    -1,    -1,    -1,    88,    89,    90,    -1,    -1,    -1,
      94,    -1,    -1,    -1,    98,    -1,    -1,    -1,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,   112,    14,
      15,    16,    -1,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    -1,
      35,    36,    37,    38,    -1,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    -1,    -1,    51,    52,    53,    54,
      55,    56,    -1,    -1,    59,    60,    -1,    -1,    63,    64,
      -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    73,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    81,    82,    83,    -1,
      -1,    -1,    -1,    88,    89,    90,    -1,    -1,    -1,    94,
      -1,    -1,    -1,    98,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,   112,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      29,    30,    31,    32,    33,    -1,    35,    36,    37,    38,
      -1,    40,    41,    42,    43,    44,    45,    -1,    47,    -1,
      49,    -1,    51,    52,    53,    54,    55,    56,    -1,    -1,
      59,    60,    -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,
      -1,    70,    -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    81,    82,    83,    -1,    -1,    -1,    -1,    88,
      89,    90,    -1,    -1,    -1,    94,    95,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      -1,    -1,    -1,   112,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    29,    30,    31,    32,    33,    -1,    35,
      36,    37,    38,    -1,    40,    41,    42,    43,    44,    45,
      -1,    47,    -1,    49,    -1,    51,    52,    53,    54,    55,
      56,    -1,    -1,    59,    60,    -1,    -1,    -1,    64,    -1,
      -1,    -1,    -1,    -1,    70,    -1,    -1,    73,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    81,    82,    83,    -1,    -1,
      -1,    -1,    88,    89,    90,    -1,    -1,    -1,    94,    95,
       3,     4,     5,     6,    -1,     8,     9,    10,    11,    12,
      -1,    14,    15,    -1,    -1,    -1,   112,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    29,    30,    31,    32,
      33,    -1,    35,    36,    37,    38,    -1,    40,    41,    42,
      43,    44,    45,    -1,    47,    -1,    49,    -1,    51,    52,
      53,    54,    55,    56,    -1,    -1,    59,    60,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,
      73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,    82,
      83,    -1,    -1,    -1,    -1,    88,    89,    90,    -1,    -1,
      -1,    94,    95,     3,     4,     5,     6,    -1,     8,     9,
      10,    11,    12,    -1,    14,    15,    -1,    -1,    -1,   112,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    29,
      30,    31,    32,    33,    -1,    35,    36,    37,    38,    -1,
      40,    41,    42,    43,    44,    45,    -1,    47,    -1,    -1,
      -1,    51,    52,    53,    54,    55,    56,    -1,    -1,    59,
      60,    -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,
      70,    -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    81,    82,    83,    -1,    -1,    -1,    -1,    88,    89,
      90,    -1,    -1,    -1,    94,     3,     4,     5,     6,    -1,
       8,     9,    10,    11,    12,    -1,    14,    15,    -1,    -1,
      -1,    -1,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    29,    30,    31,    32,    33,    -1,    35,    36,    37,
      38,    -1,    40,    41,    42,    43,    44,    45,    -1,    47,
      -1,    -1,    -1,    51,    52,    53,    54,    55,    56,    -1,
      -1,    59,    60,    -1,    -1,    -1,    64,    -1,    -1,    -1,
      -1,    -1,    70,    -1,    -1,    73,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    81,    82,    83,    -1,    -1,    -1,    -1,
      88,    89,    90,    -1,    -1,    -1,    94,     3,     4,     5,
       6,    -1,     8,     9,    10,    11,    12,    -1,    14,    15,
      -1,    -1,    -1,    -1,   112,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    29,    30,    31,    32,    33,    -1,    35,
      36,    37,    38,    -1,    40,    41,    42,    43,    44,    45,
      -1,    47,    -1,    -1,    -1,    51,    52,    53,    54,    55,
      56,    -1,    -1,    59,    60,    -1,    -1,    -1,    64,    -1,
      -1,    -1,    -1,    -1,    70,    -1,    -1,    73,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    81,    82,    83,    -1,    -1,
      -1,    -1,    88,    89,    90,    -1,    -1,    -1,    94,     3,
       4,     5,     6,    -1,     8,     9,    10,    11,    12,    -1,
      14,    15,    -1,    -1,    -1,    -1,   112,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    29,    30,    31,    -1,    33,
      -1,    35,    36,    37,    38,    -1,    40,    41,    42,    43,
      44,    45,    -1,    47,    -1,    -1,    -1,    51,    52,    53,
      54,    55,    56,    -1,    -1,    59,    60,    -1,    -1,    63,
      64,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    73,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,    82,    83,
      -1,    -1,    -1,    -1,    88,    89,    90,    -1,    -1,    -1,
      94,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    -1,    14,    15,    -1,    -1,    -1,    -1,   112,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    29,    30,    31,
      32,    33,    -1,    35,    36,    37,    38,    -1,    40,    41,
      42,    43,    44,    45,    -1,    47,    -1,    -1,    -1,    51,
      52,    53,    54,    55,    56,    -1,    -1,    59,    60,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,
      -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,
      82,    83,    -1,    -1,    -1,    -1,    88,    89,    90,    -1,
      -1,    -1,    94,     3,     4,     5,     6,    -1,     8,     9,
      10,    11,    12,    -1,    14,    15,    -1,    -1,    -1,    -1,
     112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    29,
      30,    31,    32,    33,    -1,    35,    36,    37,    38,    -1,
      40,    41,    42,    43,    44,    45,    -1,    47,    -1,    -1,
      -1,    51,    52,    53,    54,    55,    56,    -1,    -1,    59,
      60,    -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,
      70,    -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    81,    82,    83,    -1,    -1,    -1,    -1,    88,    89,
      90,    -1,    -1,    -1,    94,     3,     4,     5,     6,    -1,
       8,     9,    10,    11,    12,    -1,    14,    15,    -1,    -1,
      -1,    -1,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    29,    30,    31,    -1,    33,    -1,    35,    36,    37,
      38,    -1,    40,    41,    42,    43,    44,    45,    -1,    47,
      -1,    -1,    -1,    51,    52,    53,    54,    55,    56,    -1,
      -1,    59,    60,    -1,    -1,    -1,    64,    -1,    -1,    -1,
      -1,    -1,    70,    -1,    -1,    73,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    81,    82,    83,    -1,    -1,    -1,    -1,
      88,    89,    90,    -1,    -1,    -1,    94,     3,     4,     5,
       6,    -1,     8,     9,    10,    11,    12,    -1,    14,    15,
      -1,    -1,    -1,    -1,   112,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    29,    30,    31,    -1,    33,    -1,    35,
      36,    37,    38,    -1,    40,    41,    42,    43,    44,    45,
      -1,    47,    -1,    -1,    -1,    51,    52,    53,    54,    55,
      56,    -1,    -1,    59,    60,    61,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    70,    -1,    -1,    73,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    81,    82,    83,    -1,    -1,
      -1,    -1,    88,    89,    90,    -1,    -1,    -1,    94,     3,
       4,     5,     6,    -1,     8,     9,    10,    11,    12,    -1,
      14,    15,    -1,    -1,    -1,    -1,   112,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    29,    30,    31,    -1,    33,
      -1,    35,    36,    37,    38,    -1,    40,    41,    42,    43,
      44,    45,    -1,    47,    -1,    -1,    -1,    51,    52,    53,
      54,    55,    56,    -1,    -1,    59,    60,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    73,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,    82,    83,
      -1,    -1,    -1,    -1,    88,    89,    90,    -1,    -1,    -1,
      94,    95,     3,     4,     5,     6,    -1,     8,     9,    10,
      11,    12,    -1,    14,    15,    -1,    -1,    -1,   112,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    29,    30,
      31,    -1,    33,    -1,    35,    36,    37,    38,    -1,    40,
      41,    42,    43,    44,    45,    -1,    47,    -1,    -1,    -1,
      51,    52,    53,    54,    55,    56,    -1,    -1,    59,    60,
      -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,    70,
      -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      81,    82,    83,    -1,    -1,    -1,    -1,    88,    89,    90,
      -1,    -1,    -1,    94,     3,     4,     5,     6,    -1,     8,
       9,    10,    11,    12,    -1,    14,    15,    -1,    -1,    -1,
      -1,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      29,    30,    31,    32,    33,    -1,    35,    36,    37,    38,
      -1,    40,    41,    42,    43,    44,    45,    -1,    47,    -1,
      -1,    -1,    51,    52,    53,    54,    55,    56,    -1,    -1,
      59,    60,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    70,    -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    81,    82,    83,    -1,    -1,    -1,    -1,    88,
      89,    90,    -1,    -1,    -1,    94,     3,     4,     5,     6,
      -1,     8,     9,    10,    11,    12,    -1,    14,    15,    -1,
      -1,    -1,    -1,   112,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    29,    30,    31,    -1,    33,    -1,    35,    36,
      37,    38,    -1,    40,    41,    42,    43,    44,    45,    -1,
      47,    -1,    -1,    -1,    51,    52,    53,    54,    55,    56,
      -1,    -1,    59,    60,    -1,    -1,    -1,    64,    -1,    -1,
      -1,    -1,    -1,    70,    -1,    -1,    73,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    81,    82,    83,    -1,    -1,    -1,
      -1,    88,    89,    90,    -1,    -1,    -1,    94,     3,     4,
       5,     6,    -1,     8,     9,    10,    11,    12,    -1,    14,
      15,    -1,    -1,    -1,    -1,   112,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    29,    30,    31,    32,    33,    -1,
      35,    36,    37,    38,    -1,    40,    41,    42,    43,    44,
      45,    -1,    47,    -1,    -1,    -1,    51,    52,    53,    54,
      55,    56,    -1,    -1,    59,    60,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    73,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    81,    82,    83,    -1,
      -1,    -1,    -1,    88,    89,    90,    -1,    -1,    -1,    94,
       3,     4,     5,     6,    -1,     8,     9,    10,    11,    12,
      -1,    14,    15,    -1,    -1,    -1,    -1,   112,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    29,    30,    31,    -1,
      33,    -1,    35,    36,    37,    38,    -1,    40,    41,    42,
      43,    44,    45,    -1,    47,    -1,    -1,    -1,    51,    52,
      53,    54,    55,    56,    -1,    -1,    59,    60,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,
      73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,    82,
      83,    -1,    -1,    -1,    -1,    88,    89,    90,    -1,    -1,
      -1,    94,     3,     4,     5,     6,    -1,     8,     9,    10,
      11,    12,    -1,    14,    15,    -1,    -1,    -1,    -1,   112,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    29,    30,
      31,    -1,    33,    -1,    35,    36,    37,    38,    -1,    40,
      41,    42,    43,    44,    45,    -1,    47,    -1,    -1,    -1,
      51,    52,    53,    54,    55,    56,    -1,    -1,    59,    60,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,
      -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      81,    82,    83,    -1,    -1,    -1,    -1,    88,    89,    90,
      -1,    -1,    -1,    94,     3,     4,     5,     6,    -1,     8,
       9,    10,    11,    12,    -1,    14,    15,    -1,    -1,    -1,
      -1,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      29,    30,    31,    -1,    33,    -1,    35,    36,    37,    38,
      -1,    40,    41,    42,    43,    44,    45,    -1,    47,    -1,
      -1,    -1,    51,    52,    53,    54,    55,    56,    -1,    -1,
      59,    60,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    70,    -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    81,    82,    83,    -1,    -1,    -1,    -1,    88,
      89,    90,    -1,    -1,    -1,    94,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   112,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned short yystos[] =
{
       0,   115,   116,   117,     0,   123,   124,   123,     1,     3,
       4,     5,     6,     7,     8,     9,    15,    28,    29,    31,
      32,    33,    38,    43,    46,    47,    48,    50,    56,    57,
      59,    60,    63,    73,    83,    88,    94,    99,   121,   122,
     125,   129,   131,   133,   137,   138,   140,   141,   147,   150,
     156,   157,   162,   176,   187,   189,   214,   216,   221,   222,
     234,   235,   237,   238,   242,   243,   247,   255,   258,   259,
     271,   279,   280,   281,   282,   283,   307,   312,   313,   314,
     316,   318,   320,   323,   324,   325,   332,   333,   400,   407,
     409,    63,   109,   111,    76,   401,    56,    76,    56,    76,
      76,    50,     3,     4,     5,    59,    60,    61,   173,   187,
     314,   324,   331,   333,    94,    94,     4,     5,    59,    60,
      61,   173,     3,     4,     5,    59,    60,   187,   326,   327,
     328,   329,   333,     4,     5,    46,    59,    60,   319,   324,
     333,    76,   185,   126,    56,    76,    76,     3,   258,   307,
     315,   316,   324,   333,   307,   315,   398,    73,    83,    94,
     214,   219,   220,   222,   258,   317,   318,   332,   125,    94,
      63,    61,    99,   119,     1,     7,    50,   121,   137,   147,
     148,   149,   156,   234,   237,   247,   271,   109,   153,   122,
     248,    26,   152,   165,   166,    63,    56,    49,    94,    95,
     308,    63,    73,    83,    94,   231,   245,   253,   310,   311,
     316,   323,   324,   332,     7,     9,    63,   157,   243,   246,
     254,   258,   316,    94,     7,     8,     9,   236,   239,   244,
     258,   271,    62,    63,   259,     7,     8,     9,    38,    61,
     105,   106,   107,   173,   175,   187,   188,   258,   324,   333,
      61,    65,    61,    65,    65,   284,     9,   243,   258,   333,
     248,     3,     5,    50,    60,    83,   189,   214,   217,   221,
     323,   325,   306,     3,    59,   214,   314,   320,   323,   324,
       1,    40,    41,    49,    62,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    87,    88,    89,    90,    95,
      96,   112,   238,   243,   307,   378,    76,   192,   193,   194,
     195,     3,   192,   192,   215,   185,   273,    61,     3,    50,
      60,   314,   323,     9,    10,    11,    12,    14,    30,    35,
      36,    37,    40,    41,    42,    44,    45,    51,    52,    53,
      54,    55,    64,    70,    73,    81,    82,    83,    88,    89,
      90,    94,   112,   121,   197,   198,   205,   207,   210,   211,
     212,   214,   223,   225,   226,   227,   229,   233,   238,   240,
     241,   243,   307,   320,   321,   322,   324,   333,    94,   128,
      61,    66,    56,    56,    56,    56,    56,    56,     3,     4,
       5,    50,    59,    60,   173,   187,   329,   330,   327,   134,
       3,     4,     5,   214,   217,   218,   218,   319,    77,   139,
     157,   234,   237,   243,   316,   147,   193,   192,    76,   316,
     258,   315,   324,   315,     4,     5,   173,   187,   219,   258,
     307,   317,   219,   307,   317,   219,   110,    59,    60,    73,
      83,    94,   214,   220,   110,   306,    12,   116,   118,    50,
     150,   156,   148,   119,   153,   245,   246,    63,    63,    94,
     257,   258,     3,     1,    98,   154,   169,   368,    49,    66,
      94,   264,    63,     1,    13,   205,   206,   212,   233,   235,
     237,   238,   243,   307,   388,   389,   393,   394,   395,   397,
       9,   113,   198,   243,   306,   258,   307,   309,   310,   307,
     309,    73,    83,    94,   258,   309,   332,   248,    62,    63,
      95,   308,   323,   306,   236,   239,    62,    63,   248,   198,
     233,     7,   244,   236,   244,   258,   231,   249,    76,    61,
      65,     3,    60,   173,   188,   173,   324,   398,    61,   239,
     256,    56,    49,    94,     6,   173,   330,   409,    56,    49,
      94,     9,   258,   315,     5,    83,   189,   408,    95,   408,
      95,   408,   408,   408,   408,   408,    65,   408,   408,   408,
     408,   408,   408,   408,   408,   408,   408,   408,   408,   408,
     408,   408,   408,   408,   408,   408,   408,   408,   113,   408,
     408,   239,    73,    83,   324,   332,   333,   406,   215,    77,
      79,   191,    62,    60,    64,   196,   211,   213,   233,   324,
     333,    94,   402,   191,   191,   192,   157,   234,   243,   316,
      62,   173,   278,   302,   303,   304,   272,   173,    94,   211,
     211,    94,    94,    76,    76,    76,    76,   212,   173,   211,
     211,   211,     1,    73,    83,    94,   198,   214,   219,   224,
     233,   211,   211,   110,    62,    61,    94,   207,    62,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    49,    94,    49,    89,    90,    93,    94,    95,    96,
      61,    94,   208,   305,   378,    95,   211,     1,     8,    50,
      88,   217,   218,   321,   324,   333,   110,    49,    73,    83,
      94,    95,   210,   232,   332,   337,   338,    94,   207,    94,
     207,    94,   232,   337,   324,   333,   337,    49,    94,     3,
      41,   216,   226,   320,   333,   409,     7,     8,     9,   173,
     260,   261,   262,   118,   127,   130,   330,    76,    56,   136,
     218,   319,   333,    47,    50,   138,   142,   144,   145,   146,
     233,   243,   279,   395,   397,   180,   231,    63,   179,   191,
     191,   192,   315,   315,   110,   219,   315,   110,   111,   120,
     120,   149,   156,    63,    63,    12,   369,    65,   151,   167,
     228,     1,    61,   212,   265,   206,   110,   110,    13,    62,
      65,    73,    83,    94,   231,   332,   337,   316,   324,   333,
     231,    94,   231,   110,    13,    62,    65,    13,   316,   396,
     397,   399,    66,   390,   113,    94,   232,    64,   403,   310,
     258,   309,   309,   258,   307,   307,    73,    83,    94,   324,
     332,   110,   306,   251,   257,   249,   113,   198,   306,   258,
     309,   236,   249,   251,   110,   110,   248,   192,   173,   188,
       4,     5,     7,    39,   187,   285,   286,   287,   288,   312,
     326,   333,   274,   251,   306,   158,    76,    76,    56,    76,
     306,   160,    49,    94,    49,    94,   113,   113,   408,   408,
     306,   306,   306,   324,   408,   192,   194,   213,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    50,
      60,   206,    63,   190,   190,   191,   184,   231,    63,   183,
      66,   111,    62,   278,   302,   206,   212,   198,   233,   233,
     243,   233,   233,   233,   110,   219,   110,   110,   344,   410,
     110,     1,   212,     3,     4,     5,    59,    60,    95,   173,
     265,   266,   233,     1,   212,   212,   212,     1,   198,   383,
     212,   212,   212,   212,   212,   212,   212,   212,   212,   212,
     212,   212,   212,   212,   212,   212,   212,   212,   212,   206,
     206,   198,   206,   206,   233,    94,   305,    49,    66,    94,
     209,    73,    83,    95,   332,   334,   335,   113,   198,    56,
     173,     8,    49,    94,    49,    94,    49,    94,   258,   307,
     336,   337,   307,   336,   233,   336,   389,   113,   198,    94,
     306,    49,   306,   306,    49,    94,    95,   233,   233,    73,
      83,    94,   206,   210,   219,   258,   324,   333,   324,   206,
      62,   110,    94,   111,   118,   136,    56,   192,    63,   218,
     319,   143,   173,    76,   279,    62,    77,    66,    66,   143,
     173,    66,   186,   178,   177,   186,   190,   191,   110,    63,
     153,   120,   120,   110,   154,   168,   410,   111,   266,   110,
     258,   307,   307,    73,    83,    94,   258,   323,   324,   332,
     306,   324,    73,    83,    94,   214,   219,   320,   333,   390,
      66,   391,    49,    94,   307,   307,   306,    66,   113,   403,
     257,    77,   398,   398,    62,     7,    39,   398,     1,     5,
      48,    63,    65,    94,   121,   131,   141,   157,   163,   164,
     189,   234,   237,   289,   291,   292,   293,   316,   400,   403,
     388,   192,   192,   192,   403,   388,   306,   159,   306,   161,
     408,   408,   406,   406,   406,   191,   213,   213,   383,   213,
     213,   213,   213,   213,   213,   213,   213,   213,   213,   213,
     213,   213,   213,   213,   213,   213,   213,   173,   110,   186,
     182,   181,   186,   212,   304,   111,   110,    62,   110,   110,
      77,    77,    77,    77,   110,   110,    61,   212,    65,    62,
     277,   110,    65,   110,   110,   113,   111,   110,   110,   233,
     209,   265,   206,   243,   306,   306,   198,   306,    95,   211,
     113,    88,    76,    49,   206,   206,   206,   337,   258,   336,
     336,   110,   110,   113,   403,   403,   258,   336,   306,   388,
     113,   198,   110,   110,   110,   110,    73,    83,    94,   110,
     261,   110,     3,   206,   111,    63,   191,   143,   146,   233,
     194,   213,   186,   186,   153,   370,     1,     3,    59,    60,
     170,   171,   172,   174,   312,   326,    61,    62,   111,   258,
     307,   307,    73,    83,    94,   332,   323,   306,   258,   219,
     110,     3,   216,   320,   409,   265,   102,   265,   392,     1,
     233,   404,   405,    73,    83,    94,   258,   258,   252,    66,
     190,   398,   398,   398,   287,    49,    94,   212,     5,   292,
     234,   292,   248,   248,    26,    61,    65,    98,    49,    94,
       3,     4,    65,   157,   231,   294,   296,   298,   299,   310,
     316,   324,   333,   157,   163,   295,   299,   316,    39,   111,
     290,   292,    63,   111,   248,   110,   191,   191,   191,   110,
     403,   388,   403,   388,    65,   186,   186,   233,    94,    94,
      94,    94,   345,   113,   265,   173,   265,   111,   212,    95,
     209,   110,   110,   110,   334,   334,   113,   334,   198,   211,
       8,   192,   110,   110,   110,   306,   403,   110,   113,    62,
     110,   110,    97,   374,   375,     1,    62,    49,    94,   155,
     111,   307,   307,   324,   306,   110,    62,   110,   258,   265,
     250,   286,   306,   388,   110,    63,   257,   257,   306,   388,
      65,    65,   212,   248,    62,   248,   248,   324,   248,    62,
      65,   257,   291,   257,   306,   306,   110,   110,   213,   110,
     198,   198,   198,   198,    34,   111,   204,   341,   342,   343,
     265,    65,   198,   209,   113,    49,   191,   403,   306,   206,
     376,   375,   172,   206,   204,    73,    83,    94,   258,   258,
     404,   265,   403,   110,    49,    94,   264,   264,   403,   110,
     212,   212,   257,   257,     3,     4,    65,   297,   300,   301,
     310,   316,   257,   257,     5,   257,   301,   275,   264,   403,
     403,   306,   306,   110,   110,   110,   110,   173,   263,     1,
     339,   340,   344,   346,   352,   410,   343,   265,   113,   403,
     110,    94,   379,   110,   258,   306,   306,   388,   306,   257,
     257,   264,    65,    65,   212,   248,   248,   264,   264,   264,
     270,   403,   403,    62,    63,    63,   111,     1,   111,   340,
     352,     3,     4,     5,    16,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    46,    48,    60,    63,    98,
     121,   122,   129,   132,   133,   198,   230,   234,   237,   243,
     347,   353,   371,   380,    13,   397,   377,   403,   403,   110,
     403,   212,   212,   257,   257,   257,     1,   103,   269,   276,
     173,   111,    65,    65,    65,   348,   355,   357,   359,   363,
     212,    65,    63,    63,    63,   198,    83,   173,   173,    59,
     135,   333,    65,   372,   230,     9,   382,    63,    63,   231,
     245,    63,   246,   316,    94,   245,    17,   111,   352,   110,
     110,   346,   306,   257,   257,   264,   264,     1,   212,   268,
      49,    94,   200,   200,   346,   350,   351,    94,    94,    13,
      65,   367,    63,   198,    63,    56,    59,   173,    59,   135,
     173,   346,    94,    63,    63,    73,    83,    94,    63,   354,
     403,   109,   109,   104,   267,   198,   202,   243,   378,   349,
     356,    18,   410,    61,   230,   381,   383,   202,   212,   365,
     352,    63,    56,   173,   373,    12,   350,   152,   153,   110,
     231,   350,   350,   358,   353,   204,   360,    63,   110,    65,
     352,   374,    56,    65,   110,     1,   154,   368,   248,    49,
      94,   199,     1,   201,   202,   364,   366,    12,    95,   384,
     385,   386,   384,    63,   257,   198,    63,    63,   350,   352,
      94,   173,    65,   110,    62,    56,    65,   110,    66,   110,
     361,   198,   113,    12,   387,    63,   386,   387,   384,    63,
     203,   383,   110,    12,    62,   110,   110,    65,   110,   265,
     110,    94,    12,    63,    63,   387,    63,   362,   198,   110,
     350,   110,    63
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrlab1

/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (cinluded).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylineno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylineno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

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

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
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
     `$$ = $1'.

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
#line 487 "parse.y"
    { finish_translation_unit (); ;}
    break;

  case 3:
#line 489 "parse.y"
    { finish_translation_unit (); ;}
    break;

  case 4:
#line 497 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 5:
#line 499 "parse.y"
    { yyval.ttype = NULL_TREE; ggc_collect (); ;}
    break;

  case 6:
#line 501 "parse.y"
    { yyval.ttype = NULL_TREE; ggc_collect (); ;}
    break;

  case 9:
#line 510 "parse.y"
    { have_extern_spec = true;
		  yyval.ttype = NULL_TREE; ;}
    break;

  case 10:
#line 514 "parse.y"
    { have_extern_spec = false; ;}
    break;

  case 11:
#line 519 "parse.y"
    { yyval.itype = pedantic;
		  pedantic = 0; ;}
    break;

  case 13:
#line 528 "parse.y"
    { if (pending_lang_change) do_pending_lang_change();
		  type_lookups = NULL_TREE; ;}
    break;

  case 14:
#line 531 "parse.y"
    { if (! toplevel_bindings_p ())
		  pop_everything (); ;}
    break;

  case 15:
#line 537 "parse.y"
    { do_pending_inlines (); ;}
    break;

  case 16:
#line 539 "parse.y"
    { do_pending_inlines (); ;}
    break;

  case 17:
#line 542 "parse.y"
    { warning ("keyword `export' not implemented, and will be ignored"); ;}
    break;

  case 18:
#line 544 "parse.y"
    { do_pending_inlines (); ;}
    break;

  case 19:
#line 546 "parse.y"
    { do_pending_inlines (); ;}
    break;

  case 20:
#line 548 "parse.y"
    { assemble_asm (yyvsp[-2].ttype); ;}
    break;

  case 21:
#line 550 "parse.y"
    { pop_lang_context (); ;}
    break;

  case 22:
#line 552 "parse.y"
    { do_pending_inlines (); pop_lang_context (); ;}
    break;

  case 23:
#line 554 "parse.y"
    { do_pending_inlines (); pop_lang_context (); ;}
    break;

  case 24:
#line 556 "parse.y"
    { push_namespace (yyvsp[-1].ttype); ;}
    break;

  case 25:
#line 558 "parse.y"
    { pop_namespace (); ;}
    break;

  case 26:
#line 560 "parse.y"
    { push_namespace (NULL_TREE); ;}
    break;

  case 27:
#line 562 "parse.y"
    { pop_namespace (); ;}
    break;

  case 29:
#line 565 "parse.y"
    { do_toplevel_using_decl (yyvsp[-1].ttype); ;}
    break;

  case 31:
#line 568 "parse.y"
    { pedantic = yyvsp[-1].itype; ;}
    break;

  case 32:
#line 573 "parse.y"
    { begin_only_namespace_names (); ;}
    break;

  case 33:
#line 575 "parse.y"
    {
		  end_only_namespace_names ();
		  if (lastiddecl)
		    yyvsp[-1].ttype = lastiddecl;
		  do_namespace_alias (yyvsp[-4].ttype, yyvsp[-1].ttype);
		;}
    break;

  case 34:
#line 585 "parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 35:
#line 587 "parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 36:
#line 589 "parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 37:
#line 594 "parse.y"
    { yyval.ttype = build_nt (SCOPE_REF, yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 38:
#line 596 "parse.y"
    { yyval.ttype = build_nt (SCOPE_REF, global_namespace, yyvsp[0].ttype); ;}
    break;

  case 39:
#line 598 "parse.y"
    { yyval.ttype = build_nt (SCOPE_REF, yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 40:
#line 603 "parse.y"
    { begin_only_namespace_names (); ;}
    break;

  case 41:
#line 605 "parse.y"
    {
		  end_only_namespace_names ();
		  /* If no declaration was found, the using-directive is
		     invalid. Since that was not reported, we need the
		     identifier for the error message. */
		  if (TREE_CODE (yyvsp[-1].ttype) == IDENTIFIER_NODE && lastiddecl)
		    yyvsp[-1].ttype = lastiddecl;
		  do_using_directive (yyvsp[-1].ttype);
		;}
    break;

  case 42:
#line 618 "parse.y"
    {
		  if (TREE_CODE (yyval.ttype) == IDENTIFIER_NODE)
		    yyval.ttype = lastiddecl;
		  got_scope = yyval.ttype;
		;}
    break;

  case 43:
#line 624 "parse.y"
    {
		  yyval.ttype = yyvsp[-1].ttype;
		  if (TREE_CODE (yyval.ttype) == IDENTIFIER_NODE)
		    yyval.ttype = lastiddecl;
		  got_scope = yyval.ttype;
		;}
    break;

  case 46:
#line 636 "parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 47:
#line 638 "parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 48:
#line 643 "parse.y"
    { push_lang_context (yyvsp[0].ttype); ;}
    break;

  case 49:
#line 645 "parse.y"
    { if (current_lang_name != yyvsp[0].ttype)
		    error ("use of linkage spec `%D' is different from previous spec `%D'", yyvsp[0].ttype, current_lang_name);
		  pop_lang_context (); push_lang_context (yyvsp[0].ttype); ;}
    break;

  case 50:
#line 652 "parse.y"
    { begin_template_parm_list (); ;}
    break;

  case 51:
#line 654 "parse.y"
    { yyval.ttype = end_template_parm_list (yyvsp[-1].ttype); ;}
    break;

  case 52:
#line 659 "parse.y"
    { begin_specialization();
		  yyval.ttype = NULL_TREE; ;}
    break;

  case 55:
#line 670 "parse.y"
    { yyval.ttype = process_template_parm (NULL_TREE, yyvsp[0].ttype); ;}
    break;

  case 56:
#line 672 "parse.y"
    { yyval.ttype = process_template_parm (yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 57:
#line 677 "parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 58:
#line 679 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 59:
#line 684 "parse.y"
    { yyval.ttype = finish_template_type_parm (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 60:
#line 686 "parse.y"
    { yyval.ttype = finish_template_type_parm (class_type_node, yyvsp[0].ttype); ;}
    break;

  case 61:
#line 691 "parse.y"
    { yyval.ttype = finish_template_template_parm (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 62:
#line 703 "parse.y"
    { yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ttype); ;}
    break;

  case 63:
#line 705 "parse.y"
    { yyval.ttype = build_tree_list (groktypename (yyvsp[0].ftype.t), yyvsp[-2].ttype); ;}
    break;

  case 64:
#line 707 "parse.y"
    { yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ftype.t); ;}
    break;

  case 65:
#line 709 "parse.y"
    { yyval.ttype = build_tree_list (yyvsp[0].ttype, yyvsp[-2].ftype.t); ;}
    break;

  case 66:
#line 711 "parse.y"
    { yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ttype); ;}
    break;

  case 67:
#line 713 "parse.y"
    {
		  yyvsp[0].ttype = check_template_template_default_arg (yyvsp[0].ttype);
		  yyval.ttype = build_tree_list (yyvsp[0].ttype, yyvsp[-2].ttype);
		;}
    break;

  case 68:
#line 721 "parse.y"
    { finish_template_decl (yyvsp[-1].ttype); ;}
    break;

  case 69:
#line 723 "parse.y"
    { finish_template_decl (yyvsp[-1].ttype); ;}
    break;

  case 70:
#line 728 "parse.y"
    { do_pending_inlines (); ;}
    break;

  case 71:
#line 730 "parse.y"
    { do_pending_inlines (); ;}
    break;

  case 72:
#line 732 "parse.y"
    { do_pending_inlines (); ;}
    break;

  case 73:
#line 734 "parse.y"
    { do_pending_inlines ();
		  pop_lang_context (); ;}
    break;

  case 74:
#line 737 "parse.y"
    { do_pending_inlines ();
		  pop_lang_context (); ;}
    break;

  case 75:
#line 740 "parse.y"
    { pedantic = yyvsp[-1].itype; ;}
    break;

  case 77:
#line 746 "parse.y"
    {;}
    break;

  case 78:
#line 748 "parse.y"
    { note_list_got_semicolon (yyvsp[-2].ftype.t); ;}
    break;

  case 79:
#line 750 "parse.y"
    {
		  if (yyvsp[-1].ftype.t != error_mark_node)
                    {
		      maybe_process_partial_specialization (yyvsp[-1].ftype.t);
		      note_got_semicolon (yyvsp[-1].ftype.t);
	            }
                ;}
    break;

  case 81:
#line 762 "parse.y"
    {;}
    break;

  case 82:
#line 764 "parse.y"
    { note_list_got_semicolon (yyvsp[-2].ftype.t); ;}
    break;

  case 83:
#line 766 "parse.y"
    { pedwarn ("empty declaration"); ;}
    break;

  case 85:
#line 769 "parse.y"
    {
		  tree t, attrs;
		  split_specs_attrs (yyvsp[-1].ftype.t, &t, &attrs);
		  shadow_tag (t);
		  note_list_got_semicolon (yyvsp[-1].ftype.t);
		;}
    break;

  case 88:
#line 778 "parse.y"
    { end_input (); ;}
    break;

  case 98:
#line 804 "parse.y"
    { yyval.ttype = begin_compound_stmt (/*has_no_scope=*/1); ;}
    break;

  case 99:
#line 806 "parse.y"
    {
		  STMT_LINENO (yyvsp[-1].ttype) = yyvsp[-3].itype;
		  finish_compound_stmt (/*has_no_scope=*/1, yyvsp[-1].ttype);
		  finish_function_body (yyvsp[-5].ttype);
		;}
    break;

  case 100:
#line 815 "parse.y"
    { expand_body (finish_function (0)); ;}
    break;

  case 101:
#line 817 "parse.y"
    { expand_body (finish_function (0)); ;}
    break;

  case 102:
#line 819 "parse.y"
    { ;}
    break;

  case 103:
#line 824 "parse.y"
    { yyval.ttype = begin_constructor_declarator (yyvsp[-2].ttype, yyvsp[-1].ttype); ;}
    break;

  case 104:
#line 826 "parse.y"
    { yyval.ttype = make_call_declarator (yyvsp[-4].ttype, yyvsp[-3].ttype, yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 105:
#line 828 "parse.y"
    { yyval.ttype = begin_constructor_declarator (yyvsp[-4].ttype, yyvsp[-3].ttype);
		  yyval.ttype = make_call_declarator (yyval.ttype, empty_parms (), yyvsp[-1].ttype, yyvsp[0].ttype);
		;}
    break;

  case 106:
#line 832 "parse.y"
    { yyval.ttype = begin_constructor_declarator (yyvsp[-2].ttype, yyvsp[-1].ttype); ;}
    break;

  case 107:
#line 834 "parse.y"
    { yyval.ttype = make_call_declarator (yyvsp[-4].ttype, yyvsp[-3].ttype, yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 108:
#line 836 "parse.y"
    { yyval.ttype = begin_constructor_declarator (yyvsp[-4].ttype, yyvsp[-3].ttype);
		  yyval.ttype = make_call_declarator (yyval.ttype, empty_parms (), yyvsp[-1].ttype, yyvsp[0].ttype);
		;}
    break;

  case 109:
#line 840 "parse.y"
    { yyval.ttype = begin_constructor_declarator (yyvsp[-2].ttype, yyvsp[-1].ttype); ;}
    break;

  case 110:
#line 842 "parse.y"
    { yyval.ttype = make_call_declarator (yyvsp[-4].ttype, yyvsp[-3].ttype, yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 111:
#line 844 "parse.y"
    { yyval.ttype = begin_constructor_declarator (yyvsp[-4].ttype, yyvsp[-3].ttype);
		  yyval.ttype = make_call_declarator (yyval.ttype, empty_parms (), yyvsp[-1].ttype, yyvsp[0].ttype);
		;}
    break;

  case 112:
#line 848 "parse.y"
    { yyval.ttype = begin_constructor_declarator (yyvsp[-2].ttype, yyvsp[-1].ttype); ;}
    break;

  case 113:
#line 850 "parse.y"
    { yyval.ttype = make_call_declarator (yyvsp[-4].ttype, yyvsp[-3].ttype, yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 114:
#line 852 "parse.y"
    { yyval.ttype = begin_constructor_declarator (yyvsp[-4].ttype, yyvsp[-3].ttype);
		  yyval.ttype = make_call_declarator (yyval.ttype, empty_parms (), yyvsp[-1].ttype, yyvsp[0].ttype);
		;}
    break;

  case 115:
#line 859 "parse.y"
    { check_for_new_type ("return type", yyvsp[-1].ftype);
		  if (!parse_begin_function_definition (yyvsp[-1].ftype.t, yyvsp[0].ttype))
		    YYERROR1; ;}
    break;

  case 116:
#line 863 "parse.y"
    { if (!parse_begin_function_definition (yyvsp[-1].ftype.t, yyvsp[0].ttype))
		    YYERROR1; ;}
    break;

  case 117:
#line 866 "parse.y"
    { if (!parse_begin_function_definition (NULL_TREE, yyvsp[0].ttype))
		    YYERROR1; ;}
    break;

  case 118:
#line 869 "parse.y"
    { if (!parse_begin_function_definition (yyvsp[-1].ftype.t, yyvsp[0].ttype))
		    YYERROR1; ;}
    break;

  case 119:
#line 872 "parse.y"
    { if (!parse_begin_function_definition (NULL_TREE, yyvsp[0].ttype))
		    YYERROR1; ;}
    break;

  case 120:
#line 881 "parse.y"
    { yyval.ttype = make_call_declarator (yyvsp[-5].ttype, yyvsp[-3].ttype, yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 121:
#line 884 "parse.y"
    { yyval.ttype = make_call_declarator (yyvsp[-6].ttype, yyvsp[-3].ttype, yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 122:
#line 886 "parse.y"
    { yyval.ttype = make_call_declarator (yyvsp[-3].ttype, empty_parms (), yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 123:
#line 888 "parse.y"
    { yyval.ttype = make_call_declarator (yyvsp[-4].ttype, empty_parms (), yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 124:
#line 890 "parse.y"
    { yyval.ttype = make_call_declarator (yyvsp[-5].ttype, yyvsp[-3].ttype, yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 125:
#line 892 "parse.y"
    { yyval.ttype = make_call_declarator (yyvsp[-3].ttype, empty_parms (), yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 126:
#line 899 "parse.y"
    { yyval.ttype = parse_method (yyvsp[0].ttype, yyvsp[-1].ftype.t, yyvsp[-1].ftype.lookups);
		 rest_of_mdef:
		  if (! yyval.ttype)
		    YYERROR1;
		  if (yychar == YYEMPTY)
		    yychar = YYLEX;
		  snarf_method (yyval.ttype); ;}
    break;

  case 127:
#line 907 "parse.y"
    { yyval.ttype = parse_method (yyvsp[0].ttype, NULL_TREE, NULL_TREE);
		  goto rest_of_mdef; ;}
    break;

  case 128:
#line 910 "parse.y"
    { yyval.ttype = parse_method (yyvsp[0].ttype, yyvsp[-1].ftype.t, yyvsp[-1].ftype.lookups); goto rest_of_mdef;;}
    break;

  case 129:
#line 912 "parse.y"
    { yyval.ttype = parse_method (yyvsp[0].ttype, yyvsp[-1].ftype.t, yyvsp[-1].ftype.lookups); goto rest_of_mdef;;}
    break;

  case 130:
#line 914 "parse.y"
    { yyval.ttype = parse_method (yyvsp[0].ttype, NULL_TREE, NULL_TREE);
		  goto rest_of_mdef; ;}
    break;

  case 131:
#line 917 "parse.y"
    { yyval.ttype = parse_method (yyvsp[0].ttype, yyvsp[-1].ftype.t, yyvsp[-1].ftype.lookups); goto rest_of_mdef;;}
    break;

  case 132:
#line 919 "parse.y"
    { yyval.ttype = parse_method (yyvsp[0].ttype, NULL_TREE, NULL_TREE);
		  goto rest_of_mdef; ;}
    break;

  case 133:
#line 925 "parse.y"
    {
		  yyval.ttype = yyvsp[0].ttype;
		;}
    break;

  case 134:
#line 932 "parse.y"
    { finish_named_return_value (yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 135:
#line 934 "parse.y"
    { finish_named_return_value (yyval.ttype, yyvsp[-1].ttype); ;}
    break;

  case 136:
#line 936 "parse.y"
    { finish_named_return_value (yyval.ttype, NULL_TREE); ;}
    break;

  case 137:
#line 940 "parse.y"
    { begin_mem_initializers (); ;}
    break;

  case 138:
#line 941 "parse.y"
    {
		  if (yyvsp[0].ftype.new_type_flag == 0)
		    error ("no base or member initializers given following ':'");
		  finish_mem_initializers (yyvsp[0].ftype.t);
		;}
    break;

  case 139:
#line 950 "parse.y"
    {
		  yyval.ttype = begin_function_body ();
		;}
    break;

  case 140:
#line 957 "parse.y"
    {
		  yyval.ftype.new_type_flag = 0;
		  yyval.ftype.t = NULL_TREE;
		;}
    break;

  case 141:
#line 962 "parse.y"
    {
		  yyval.ftype.new_type_flag = 1;
		  yyval.ftype.t = yyvsp[0].ttype;
		;}
    break;

  case 142:
#line 967 "parse.y"
    {
		  if (yyvsp[0].ttype)
		    {
		      yyval.ftype.new_type_flag = 1;
		      TREE_CHAIN (yyvsp[0].ttype) = yyvsp[-2].ftype.t;
		      yyval.ftype.t = yyvsp[0].ttype;
		    }
		  else
		    yyval.ftype = yyvsp[-2].ftype;
		;}
    break;

  case 144:
#line 982 "parse.y"
    {
 		  if (current_class_name)
		    pedwarn ("anachronistic old style base class initializer");
		  yyval.ttype = expand_member_init (NULL_TREE);
		  in_base_initializer = yyval.ttype && !DECL_P (yyval.ttype);
		;}
    break;

  case 145:
#line 989 "parse.y"
    { yyval.ttype = expand_member_init (yyvsp[0].ttype);
		  in_base_initializer = yyval.ttype && !DECL_P (yyval.ttype); ;}
    break;

  case 146:
#line 992 "parse.y"
    { yyval.ttype = expand_member_init (yyvsp[0].ttype);
		  in_base_initializer = yyval.ttype && !DECL_P (yyval.ttype); ;}
    break;

  case 147:
#line 995 "parse.y"
    { yyval.ttype = expand_member_init (yyvsp[0].ttype);
		  in_base_initializer = yyval.ttype && !DECL_P (yyval.ttype); ;}
    break;

  case 148:
#line 1001 "parse.y"
    { in_base_initializer = 0;
		  yyval.ttype = yyvsp[-3].ttype ? build_tree_list (yyvsp[-3].ttype, yyvsp[-1].ttype) : NULL_TREE; ;}
    break;

  case 149:
#line 1004 "parse.y"
    { in_base_initializer = 0;
		  yyval.ttype = yyvsp[-1].ttype ? build_tree_list (yyvsp[-1].ttype, void_type_node) : NULL_TREE; ;}
    break;

  case 150:
#line 1007 "parse.y"
    { in_base_initializer = 0;
		  yyval.ttype = NULL_TREE; ;}
    break;

  case 162:
#line 1033 "parse.y"
    { do_type_instantiation (yyvsp[-1].ftype.t, NULL_TREE, 1);
		  yyungetc (';', 1); ;}
    break;

  case 164:
#line 1037 "parse.y"
    { tree specs = strip_attrs (yyvsp[-1].ftype.t);
		  parse_decl_instantiation (specs, yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 166:
#line 1041 "parse.y"
    { parse_decl_instantiation (NULL_TREE, yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 168:
#line 1044 "parse.y"
    { parse_decl_instantiation (NULL_TREE, yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 170:
#line 1047 "parse.y"
    { do_type_instantiation (yyvsp[-1].ftype.t, yyvsp[-4].ttype, 1);
		  yyungetc (';', 1); ;}
    break;

  case 171:
#line 1050 "parse.y"
    {;}
    break;

  case 172:
#line 1053 "parse.y"
    { tree specs = strip_attrs (yyvsp[-1].ftype.t);
		  parse_decl_instantiation (specs, yyvsp[0].ttype, yyvsp[-4].ttype); ;}
    break;

  case 173:
#line 1056 "parse.y"
    {;}
    break;

  case 174:
#line 1058 "parse.y"
    { parse_decl_instantiation (NULL_TREE, yyvsp[0].ttype, yyvsp[-3].ttype); ;}
    break;

  case 175:
#line 1060 "parse.y"
    {;}
    break;

  case 176:
#line 1062 "parse.y"
    { parse_decl_instantiation (NULL_TREE, yyvsp[0].ttype, yyvsp[-3].ttype); ;}
    break;

  case 177:
#line 1064 "parse.y"
    {;}
    break;

  case 178:
#line 1068 "parse.y"
    { begin_explicit_instantiation(); ;}
    break;

  case 179:
#line 1072 "parse.y"
    { end_explicit_instantiation(); ;}
    break;

  case 180:
#line 1082 "parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 181:
#line 1085 "parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 184:
#line 1093 "parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 185:
#line 1099 "parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 186:
#line 1103 "parse.y"
    {
		  if (yychar == YYEMPTY)
		    yychar = YYLEX;

		  yyval.ttype = finish_template_type (yyvsp[-3].ttype, yyvsp[-1].ttype,
					     yychar == SCOPE);
		;}
    break;

  case 188:
#line 1115 "parse.y"
    {
		  /* Handle `Class<Class<Type>>' without space in the `>>' */
		  pedwarn ("`>>' should be `> >' in template class name");
		  yyungetc ('>', 1);
		;}
    break;

  case 189:
#line 1124 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 191:
#line 1130 "parse.y"
    { yyval.ttype = build_tree_list (NULL_TREE, yyval.ttype); ;}
    break;

  case 192:
#line 1132 "parse.y"
    { yyval.ttype = chainon (yyval.ttype, build_tree_list (NULL_TREE, yyvsp[0].ttype)); ;}
    break;

  case 193:
#line 1136 "parse.y"
    { ++class_template_ok_as_expr; ;}
    break;

  case 194:
#line 1138 "parse.y"
    { 
		  --class_template_ok_as_expr; 
		  yyval.ttype = yyvsp[0].ttype; 
		;}
    break;

  case 195:
#line 1146 "parse.y"
    { yyval.ttype = groktypename (yyvsp[0].ftype.t); ;}
    break;

  case 196:
#line 1148 "parse.y"
    {
		  yyval.ttype = lastiddecl;
		  if (DECL_TEMPLATE_TEMPLATE_PARM_P (yyval.ttype))
		    yyval.ttype = TREE_TYPE (yyval.ttype);
		;}
    break;

  case 197:
#line 1154 "parse.y"
    {
		  yyval.ttype = lastiddecl;
		  if (DECL_TEMPLATE_TEMPLATE_PARM_P (yyval.ttype))
		    yyval.ttype = TREE_TYPE (yyval.ttype);
		;}
    break;

  case 199:
#line 1161 "parse.y"
    {
		  if (!processing_template_decl)
		    {
		      error ("use of template qualifier outside template");
		      yyval.ttype = error_mark_node;
		    }
		  else
		    yyval.ttype = make_unbound_class_template (yyvsp[-2].ttype, yyvsp[0].ttype, tf_error | tf_parsing);
		;}
    break;

  case 200:
#line 1174 "parse.y"
    { yyval.code = NEGATE_EXPR; ;}
    break;

  case 201:
#line 1176 "parse.y"
    { yyval.code = CONVERT_EXPR; ;}
    break;

  case 202:
#line 1178 "parse.y"
    { yyval.code = PREINCREMENT_EXPR; ;}
    break;

  case 203:
#line 1180 "parse.y"
    { yyval.code = PREDECREMENT_EXPR; ;}
    break;

  case 204:
#line 1182 "parse.y"
    { yyval.code = TRUTH_NOT_EXPR; ;}
    break;

  case 205:
#line 1187 "parse.y"
    { yyval.ttype = build_x_compound_expr (yyval.ttype); ;}
    break;

  case 207:
#line 1193 "parse.y"
    { error ("ISO C++ forbids an empty condition for `%s'",
			 cond_stmt_keyword);
		  yyval.ttype = integer_zero_node; ;}
    break;

  case 208:
#line 1197 "parse.y"
    { yyval.ttype = yyvsp[-1].ttype; ;}
    break;

  case 209:
#line 1202 "parse.y"
    { error ("ISO C++ forbids an empty condition for `%s'",
			 cond_stmt_keyword);
		  yyval.ttype = integer_zero_node; ;}
    break;

  case 210:
#line 1206 "parse.y"
    { yyval.ttype = yyvsp[-1].ttype; ;}
    break;

  case 211:
#line 1211 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 213:
#line 1214 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 214:
#line 1219 "parse.y"
    { {
		  tree d;
		  for (d = getdecls (); d; d = TREE_CHAIN (d))
		    if (TREE_CODE (d) == TYPE_DECL) {
		      tree s = TREE_TYPE (d);
		      if (TREE_CODE (s) == RECORD_TYPE)
			error ("definition of class `%T' in condition", s);
		      else if (TREE_CODE (s) == ENUMERAL_TYPE)
			error ("definition of enum `%T' in condition", s);
		    }
		  }
		  current_declspecs = yyvsp[-4].ftype.t;
		  yyval.ttype = parse_decl (yyvsp[-3].ttype, yyvsp[-1].ttype, 1);
		;}
    break;

  case 215:
#line 1234 "parse.y"
    {
		  parse_end_decl (yyvsp[-1].ttype, yyvsp[0].ttype, yyvsp[-3].ttype);
		  yyval.ttype = convert_from_reference (yyvsp[-1].ttype);
		  if (TREE_CODE (TREE_TYPE (yyval.ttype)) == ARRAY_TYPE)
		    error ("definition of array `%#D' in condition", yyval.ttype);
		;}
    break;

  case 221:
#line 1252 "parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyval.ttype,
		                  build_tree_list (NULL_TREE, yyvsp[0].ttype)); ;}
    break;

  case 222:
#line 1255 "parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyval.ttype,
		                  build_tree_list (NULL_TREE, error_mark_node)); ;}
    break;

  case 223:
#line 1258 "parse.y"
    { chainon (yyval.ttype, build_tree_list (NULL_TREE, yyvsp[0].ttype)); ;}
    break;

  case 224:
#line 1260 "parse.y"
    { chainon (yyval.ttype, build_tree_list (NULL_TREE, error_mark_node)); ;}
    break;

  case 225:
#line 1265 "parse.y"
    { yyval.ttype = build_tree_list (NULL_TREE, yyval.ttype); ;}
    break;

  case 227:
#line 1271 "parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 228:
#line 1274 "parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  pedantic = yyvsp[-1].itype; ;}
    break;

  case 229:
#line 1277 "parse.y"
    { yyval.ttype = build_x_indirect_ref (yyvsp[0].ttype, "unary *"); ;}
    break;

  case 230:
#line 1279 "parse.y"
    { yyval.ttype = build_x_unary_op (ADDR_EXPR, yyvsp[0].ttype); ;}
    break;

  case 231:
#line 1281 "parse.y"
    { yyval.ttype = build_x_unary_op (BIT_NOT_EXPR, yyvsp[0].ttype); ;}
    break;

  case 232:
#line 1283 "parse.y"
    { yyval.ttype = finish_unary_op_expr (yyvsp[-1].code, yyvsp[0].ttype); ;}
    break;

  case 233:
#line 1286 "parse.y"
    { yyval.ttype = finish_label_address_expr (yyvsp[0].ttype); ;}
    break;

  case 234:
#line 1288 "parse.y"
    { yyval.ttype = finish_sizeof (yyvsp[0].ttype);
		  skip_evaluation--; ;}
    break;

  case 235:
#line 1291 "parse.y"
    { yyval.ttype = finish_sizeof (groktypename (yyvsp[-1].ftype.t));
		  check_for_new_type ("sizeof", yyvsp[-1].ftype);
		  skip_evaluation--; ;}
    break;

  case 236:
#line 1295 "parse.y"
    { yyval.ttype = finish_alignof (yyvsp[0].ttype);
		  skip_evaluation--; ;}
    break;

  case 237:
#line 1298 "parse.y"
    { yyval.ttype = finish_alignof (groktypename (yyvsp[-1].ftype.t));
		  check_for_new_type ("alignof", yyvsp[-1].ftype);
		  skip_evaluation--; ;}
    break;

  case 238:
#line 1305 "parse.y"
    { yyval.ttype = build_new (NULL_TREE, yyvsp[0].ftype.t, NULL_TREE, yyvsp[-1].itype);
		  check_for_new_type ("new", yyvsp[0].ftype); ;}
    break;

  case 239:
#line 1308 "parse.y"
    { yyval.ttype = build_new (NULL_TREE, yyvsp[-1].ftype.t, yyvsp[0].ttype, yyvsp[-2].itype);
		  check_for_new_type ("new", yyvsp[-1].ftype); ;}
    break;

  case 240:
#line 1311 "parse.y"
    { yyval.ttype = build_new (yyvsp[-1].ttype, yyvsp[0].ftype.t, NULL_TREE, yyvsp[-2].itype);
		  check_for_new_type ("new", yyvsp[0].ftype); ;}
    break;

  case 241:
#line 1314 "parse.y"
    { yyval.ttype = build_new (yyvsp[-2].ttype, yyvsp[-1].ftype.t, yyvsp[0].ttype, yyvsp[-3].itype);
		  check_for_new_type ("new", yyvsp[-1].ftype); ;}
    break;

  case 242:
#line 1318 "parse.y"
    { yyval.ttype = build_new (NULL_TREE, groktypename(yyvsp[-1].ftype.t),
				  NULL_TREE, yyvsp[-3].itype);
		  check_for_new_type ("new", yyvsp[-1].ftype); ;}
    break;

  case 243:
#line 1322 "parse.y"
    { yyval.ttype = build_new (NULL_TREE, groktypename(yyvsp[-2].ftype.t), yyvsp[0].ttype, yyvsp[-4].itype);
		  check_for_new_type ("new", yyvsp[-2].ftype); ;}
    break;

  case 244:
#line 1325 "parse.y"
    { yyval.ttype = build_new (yyvsp[-3].ttype, groktypename(yyvsp[-1].ftype.t), NULL_TREE, yyvsp[-4].itype);
		  check_for_new_type ("new", yyvsp[-1].ftype); ;}
    break;

  case 245:
#line 1328 "parse.y"
    { yyval.ttype = build_new (yyvsp[-4].ttype, groktypename(yyvsp[-2].ftype.t), yyvsp[0].ttype, yyvsp[-5].itype);
		  check_for_new_type ("new", yyvsp[-2].ftype); ;}
    break;

  case 246:
#line 1332 "parse.y"
    { yyval.ttype = delete_sanity (yyvsp[0].ttype, NULL_TREE, 0, yyvsp[-1].itype); ;}
    break;

  case 247:
#line 1334 "parse.y"
    { yyval.ttype = delete_sanity (yyvsp[0].ttype, NULL_TREE, 1, yyvsp[-3].itype);
		  if (yychar == YYEMPTY)
		    yychar = YYLEX; ;}
    break;

  case 248:
#line 1338 "parse.y"
    { yyval.ttype = delete_sanity (yyvsp[0].ttype, yyvsp[-2].ttype, 2, yyvsp[-4].itype);
		  if (yychar == YYEMPTY)
		    yychar = YYLEX; ;}
    break;

  case 249:
#line 1342 "parse.y"
    { yyval.ttype = build_x_unary_op (REALPART_EXPR, yyvsp[0].ttype); ;}
    break;

  case 250:
#line 1344 "parse.y"
    { yyval.ttype = build_x_unary_op (IMAGPART_EXPR, yyvsp[0].ttype); ;}
    break;

  case 251:
#line 1349 "parse.y"
    { yyval.ttype = yyvsp[-1].ttype; ;}
    break;

  case 252:
#line 1351 "parse.y"
    { pedwarn ("old style placement syntax, use () instead");
		  yyval.ttype = yyvsp[-1].ttype; ;}
    break;

  case 253:
#line 1357 "parse.y"
    { yyval.ttype = yyvsp[-1].ttype; ;}
    break;

  case 254:
#line 1359 "parse.y"
    { yyval.ttype = void_zero_node; ;}
    break;

  case 255:
#line 1361 "parse.y"
    {
		  error ("`%T' is not a valid expression", yyvsp[-1].ftype.t);
		  yyval.ttype = error_mark_node;
		;}
    break;

  case 256:
#line 1366 "parse.y"
    {
		  /* This was previously allowed as an extension, but
		     was removed in G++ 3.3.  */
		  error ("initialization of new expression with `='");
		  yyval.ttype = error_mark_node;
		;}
    break;

  case 257:
#line 1377 "parse.y"
    { yyvsp[-1].ftype.t = finish_parmlist (build_tree_list (NULL_TREE, yyvsp[-1].ftype.t), 0);
		  yyval.ttype = make_call_declarator (NULL_TREE, yyvsp[-1].ftype.t, NULL_TREE, NULL_TREE);
		  check_for_new_type ("cast", yyvsp[-1].ftype); ;}
    break;

  case 258:
#line 1381 "parse.y"
    { yyvsp[-1].ftype.t = finish_parmlist (build_tree_list (NULL_TREE, yyvsp[-1].ftype.t), 0);
		  yyval.ttype = make_call_declarator (yyval.ttype, yyvsp[-1].ftype.t, NULL_TREE, NULL_TREE);
		  check_for_new_type ("cast", yyvsp[-1].ftype); ;}
    break;

  case 260:
#line 1389 "parse.y"
    { yyval.ttype = reparse_absdcl_as_casts (yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 261:
#line 1391 "parse.y"
    {
		  tree init = build_nt (CONSTRUCTOR, NULL_TREE,
					nreverse (yyvsp[-2].ttype));
		  if (pedantic)
		    pedwarn ("ISO C++ forbids compound literals");
		  /* Indicate that this was a C99 compound literal.  */
		  TREE_HAS_CONSTRUCTOR (init) = 1;

		  yyval.ttype = reparse_absdcl_as_casts (yyval.ttype, init);
		;}
    break;

  case 263:
#line 1407 "parse.y"
    { yyval.ttype = build_x_binary_op (MEMBER_REF, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 264:
#line 1409 "parse.y"
    { yyval.ttype = build_m_component_ref (yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 265:
#line 1411 "parse.y"
    { yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 266:
#line 1413 "parse.y"
    { yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 267:
#line 1415 "parse.y"
    { yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 268:
#line 1417 "parse.y"
    { yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 269:
#line 1419 "parse.y"
    { yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 270:
#line 1421 "parse.y"
    { yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 271:
#line 1423 "parse.y"
    { yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 272:
#line 1425 "parse.y"
    { yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 273:
#line 1427 "parse.y"
    { yyval.ttype = build_x_binary_op (LT_EXPR, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 274:
#line 1429 "parse.y"
    { yyval.ttype = build_x_binary_op (GT_EXPR, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 275:
#line 1431 "parse.y"
    { yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 276:
#line 1433 "parse.y"
    { yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 277:
#line 1435 "parse.y"
    { yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 278:
#line 1437 "parse.y"
    { yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 279:
#line 1439 "parse.y"
    { yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 280:
#line 1441 "parse.y"
    { yyval.ttype = build_x_binary_op (TRUTH_ANDIF_EXPR, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 281:
#line 1443 "parse.y"
    { yyval.ttype = build_x_binary_op (TRUTH_ORIF_EXPR, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 282:
#line 1445 "parse.y"
    { yyval.ttype = build_x_conditional_expr (yyval.ttype, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 283:
#line 1447 "parse.y"
    { yyval.ttype = build_x_modify_expr (yyval.ttype, NOP_EXPR, yyvsp[0].ttype);
		  if (yyval.ttype != error_mark_node)
                    C_SET_EXP_ORIGINAL_CODE (yyval.ttype, MODIFY_EXPR); ;}
    break;

  case 284:
#line 1451 "parse.y"
    { yyval.ttype = build_x_modify_expr (yyval.ttype, yyvsp[-1].code, yyvsp[0].ttype); ;}
    break;

  case 285:
#line 1453 "parse.y"
    { yyval.ttype = build_throw (NULL_TREE); ;}
    break;

  case 286:
#line 1455 "parse.y"
    { yyval.ttype = build_throw (yyvsp[0].ttype); ;}
    break;

  case 288:
#line 1462 "parse.y"
    { yyval.ttype = build_x_binary_op (MEMBER_REF, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 289:
#line 1464 "parse.y"
    { yyval.ttype = build_m_component_ref (yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 290:
#line 1466 "parse.y"
    { yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 291:
#line 1468 "parse.y"
    { yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 292:
#line 1470 "parse.y"
    { yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 293:
#line 1472 "parse.y"
    { yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 294:
#line 1474 "parse.y"
    { yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 295:
#line 1476 "parse.y"
    { yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 296:
#line 1478 "parse.y"
    { yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 297:
#line 1480 "parse.y"
    { yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 298:
#line 1482 "parse.y"
    { yyval.ttype = build_x_binary_op (LT_EXPR, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 299:
#line 1484 "parse.y"
    { yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 300:
#line 1486 "parse.y"
    { yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 301:
#line 1488 "parse.y"
    { yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 302:
#line 1490 "parse.y"
    { yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 303:
#line 1492 "parse.y"
    { yyval.ttype = build_x_binary_op (yyvsp[-1].code, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 304:
#line 1494 "parse.y"
    { yyval.ttype = build_x_binary_op (TRUTH_ANDIF_EXPR, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 305:
#line 1496 "parse.y"
    { yyval.ttype = build_x_binary_op (TRUTH_ORIF_EXPR, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 306:
#line 1498 "parse.y"
    { yyval.ttype = build_x_conditional_expr (yyval.ttype, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 307:
#line 1500 "parse.y"
    { yyval.ttype = build_x_modify_expr (yyval.ttype, NOP_EXPR, yyvsp[0].ttype);
		  if (yyval.ttype != error_mark_node)
                    C_SET_EXP_ORIGINAL_CODE (yyval.ttype, MODIFY_EXPR); ;}
    break;

  case 308:
#line 1504 "parse.y"
    { yyval.ttype = build_x_modify_expr (yyval.ttype, yyvsp[-1].code, yyvsp[0].ttype); ;}
    break;

  case 309:
#line 1506 "parse.y"
    { yyval.ttype = build_throw (NULL_TREE); ;}
    break;

  case 310:
#line 1508 "parse.y"
    { yyval.ttype = build_throw (yyvsp[0].ttype); ;}
    break;

  case 311:
#line 1513 "parse.y"
    { yyval.ttype = build_nt (BIT_NOT_EXPR, yyvsp[0].ttype); ;}
    break;

  case 312:
#line 1515 "parse.y"
    { yyval.ttype = build_nt (BIT_NOT_EXPR, yyvsp[0].ttype); ;}
    break;

  case 318:
#line 1524 "parse.y"
    {
		  /* If lastiddecl is a BASELINK we're in an
		     expression like S::f<int>, so don't
		     do_identifier; we only do that for unqualified
		     identifiers.  */
	          if (!lastiddecl || !BASELINK_P (lastiddecl))
		    yyval.ttype = do_identifier (yyvsp[-1].ttype, 3, NULL_TREE);
		  else
		    yyval.ttype = yyvsp[-1].ttype;
		;}
    break;

  case 319:
#line 1538 "parse.y"
    { 
		  tree template_name = yyvsp[-2].ttype;
		  if (TREE_CODE (template_name) == COMPONENT_REF)
		    template_name = TREE_OPERAND (template_name, 1);
		  yyval.ttype = lookup_template_function (template_name, yyvsp[-1].ttype); 
		;}
    break;

  case 320:
#line 1545 "parse.y"
    { 
		  tree template_name = yyvsp[-2].ttype;
		  if (TREE_CODE (template_name) == COMPONENT_REF)
		    template_name = TREE_OPERAND (template_name, 1);
		  yyval.ttype = lookup_template_function (template_name, yyvsp[-1].ttype); 
		;}
    break;

  case 321:
#line 1555 "parse.y"
    { yyval.ttype = lookup_template_function (yyvsp[-3].ttype, yyvsp[-1].ttype); ;}
    break;

  case 322:
#line 1557 "parse.y"
    { yyval.ttype = lookup_template_function (yyvsp[-3].ttype, yyvsp[-1].ttype); ;}
    break;

  case 323:
#line 1560 "parse.y"
    { yyval.ttype = lookup_template_function (yyvsp[-3].ttype, yyvsp[-1].ttype); ;}
    break;

  case 328:
#line 1572 "parse.y"
    {
		  /* Provide support for '(' attributes '*' declarator ')'
		     etc */
		  yyval.ttype = tree_cons (yyvsp[-1].ttype, yyvsp[0].ttype, NULL_TREE);
		;}
    break;

  case 330:
#line 1582 "parse.y"
    { yyval.ttype = build_nt (INDIRECT_REF, yyvsp[0].ttype); ;}
    break;

  case 331:
#line 1584 "parse.y"
    { yyval.ttype = build_nt (ADDR_EXPR, yyvsp[0].ttype); ;}
    break;

  case 332:
#line 1586 "parse.y"
    { yyval.ttype = yyvsp[-1].ttype; ;}
    break;

  case 333:
#line 1591 "parse.y"
    { yyval.ttype = lookup_template_function (yyvsp[-3].ttype, yyvsp[-1].ttype); ;}
    break;

  case 334:
#line 1593 "parse.y"
    { yyval.ttype = lookup_template_function (yyvsp[-3].ttype, yyvsp[-1].ttype); ;}
    break;

  case 338:
#line 1603 "parse.y"
    { yyval.ttype = finish_decl_parsing (yyvsp[-1].ttype); ;}
    break;

  case 339:
#line 1608 "parse.y"
    {
		  if (TREE_CODE (yyvsp[0].ttype) == BIT_NOT_EXPR)
		    yyval.ttype = build_x_unary_op (BIT_NOT_EXPR, TREE_OPERAND (yyvsp[0].ttype, 0));
		  else
		    yyval.ttype = finish_id_expr (yyvsp[0].ttype);
		;}
    break;

  case 342:
#line 1617 "parse.y"
    {
		  yyval.ttype = fix_string_type (yyval.ttype);
		  /* fix_string_type doesn't set up TYPE_MAIN_VARIANT of
		     a const array the way we want, so fix it.  */
		  if (flag_const_strings)
		    TREE_TYPE (yyval.ttype) = build_cplus_array_type
		      (TREE_TYPE (TREE_TYPE (yyval.ttype)),
		       TYPE_DOMAIN (TREE_TYPE (yyval.ttype)));
		;}
    break;

  case 343:
#line 1627 "parse.y"
    { yyval.ttype = finish_fname (yyvsp[0].ttype); ;}
    break;

  case 344:
#line 1629 "parse.y"
    { yyval.ttype = finish_parenthesized_expr (yyvsp[-1].ttype); ;}
    break;

  case 345:
#line 1631 "parse.y"
    { yyvsp[-1].ttype = reparse_decl_as_expr (NULL_TREE, yyvsp[-1].ttype);
		  yyval.ttype = finish_parenthesized_expr (yyvsp[-1].ttype); ;}
    break;

  case 346:
#line 1634 "parse.y"
    { yyval.ttype = error_mark_node; ;}
    break;

  case 347:
#line 1636 "parse.y"
    { if (!at_function_scope_p ())
		    {
		      error ("braced-group within expression allowed only inside a function");
		      YYERROR;
		    }
		  if (pedantic)
		    pedwarn ("ISO C++ forbids braced-groups within expressions");
		  yyval.ttype = begin_stmt_expr ();
		;}
    break;

  case 348:
#line 1646 "parse.y"
    { yyval.ttype = finish_stmt_expr (yyvsp[-2].ttype); ;}
    break;

  case 349:
#line 1651 "parse.y"
    { yyval.ttype = parse_finish_call_expr (yyvsp[-3].ttype, yyvsp[-1].ttype, 1); ;}
    break;

  case 350:
#line 1653 "parse.y"
    { yyval.ttype = parse_finish_call_expr (yyvsp[-1].ttype, NULL_TREE, 1); ;}
    break;

  case 351:
#line 1655 "parse.y"
    { yyval.ttype = parse_finish_call_expr (yyvsp[-3].ttype, yyvsp[-1].ttype, 0); ;}
    break;

  case 352:
#line 1657 "parse.y"
    { yyval.ttype = parse_finish_call_expr (yyvsp[-1].ttype, NULL_TREE, 0); ;}
    break;

  case 353:
#line 1659 "parse.y"
    { yyval.ttype = build_x_va_arg (yyvsp[-3].ttype, groktypename (yyvsp[-1].ftype.t));
		  check_for_new_type ("__builtin_va_arg", yyvsp[-1].ftype); ;}
    break;

  case 354:
#line 1662 "parse.y"
    { yyval.ttype = grok_array_decl (yyval.ttype, yyvsp[-1].ttype); ;}
    break;

  case 355:
#line 1664 "parse.y"
    { yyval.ttype = finish_increment_expr (yyvsp[-1].ttype, POSTINCREMENT_EXPR); ;}
    break;

  case 356:
#line 1666 "parse.y"
    { yyval.ttype = finish_increment_expr (yyvsp[-1].ttype, POSTDECREMENT_EXPR); ;}
    break;

  case 357:
#line 1669 "parse.y"
    { yyval.ttype = finish_this_expr (); ;}
    break;

  case 358:
#line 1671 "parse.y"
    {
		  /* This is a C cast in C++'s `functional' notation
		     using the "implicit int" extension so that:
		     `const (3)' is equivalent to `const int (3)'.  */
		  tree type;

		  type = hash_tree_cons (NULL_TREE, yyvsp[-3].ttype, NULL_TREE);
		  type = groktypename (build_tree_list (type, NULL_TREE));
		  yyval.ttype = build_functional_cast (type, yyvsp[-1].ttype);
		;}
    break;

  case 360:
#line 1683 "parse.y"
    { tree type = groktypename (yyvsp[-4].ftype.t);
		  check_for_new_type ("dynamic_cast", yyvsp[-4].ftype);
		  yyval.ttype = build_dynamic_cast (type, yyvsp[-1].ttype); ;}
    break;

  case 361:
#line 1687 "parse.y"
    { tree type = groktypename (yyvsp[-4].ftype.t);
		  check_for_new_type ("static_cast", yyvsp[-4].ftype);
		  yyval.ttype = build_static_cast (type, yyvsp[-1].ttype); ;}
    break;

  case 362:
#line 1691 "parse.y"
    { tree type = groktypename (yyvsp[-4].ftype.t);
		  check_for_new_type ("reinterpret_cast", yyvsp[-4].ftype);
		  yyval.ttype = build_reinterpret_cast (type, yyvsp[-1].ttype); ;}
    break;

  case 363:
#line 1695 "parse.y"
    { tree type = groktypename (yyvsp[-4].ftype.t);
		  check_for_new_type ("const_cast", yyvsp[-4].ftype);
		  yyval.ttype = build_const_cast (type, yyvsp[-1].ttype); ;}
    break;

  case 364:
#line 1699 "parse.y"
    { yyval.ttype = build_typeid (yyvsp[-1].ttype); ;}
    break;

  case 365:
#line 1701 "parse.y"
    { tree type = groktypename (yyvsp[-1].ftype.t);
		  check_for_new_type ("typeid", yyvsp[-1].ftype);
		  yyval.ttype = get_typeid (type); ;}
    break;

  case 366:
#line 1705 "parse.y"
    { yyval.ttype = parse_scoped_id (yyvsp[0].ttype); ;}
    break;

  case 367:
#line 1707 "parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 368:
#line 1709 "parse.y"
    {
		  got_scope = NULL_TREE;
		  if (TREE_CODE (yyvsp[0].ttype) == IDENTIFIER_NODE)
		    yyval.ttype = parse_scoped_id (yyvsp[0].ttype);
		  else
		    yyval.ttype = yyvsp[0].ttype;
		;}
    break;

  case 369:
#line 1717 "parse.y"
    { yyval.ttype = build_offset_ref (OP0 (yyval.ttype), OP1 (yyval.ttype));
		  if (!class_template_ok_as_expr 
		      && DECL_CLASS_TEMPLATE_P (yyval.ttype))
		    {
		      error ("invalid use of template `%D'", yyval.ttype); 
		      yyval.ttype = error_mark_node;
		    }
                ;}
    break;

  case 370:
#line 1726 "parse.y"
    { yyval.ttype = parse_finish_call_expr (yyvsp[-3].ttype, yyvsp[-1].ttype, 0); ;}
    break;

  case 371:
#line 1728 "parse.y"
    { yyval.ttype = parse_finish_call_expr (yyvsp[-1].ttype, NULL_TREE, 0); ;}
    break;

  case 372:
#line 1730 "parse.y"
    { yyval.ttype = finish_class_member_access_expr (yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 373:
#line 1732 "parse.y"
    { yyval.ttype = finish_object_call_expr (yyvsp[-3].ttype, yyvsp[-4].ttype, yyvsp[-1].ttype); ;}
    break;

  case 374:
#line 1734 "parse.y"
    { yyval.ttype = finish_object_call_expr (yyvsp[-1].ttype, yyvsp[-2].ttype, NULL_TREE); ;}
    break;

  case 375:
#line 1736 "parse.y"
    { yyval.ttype = finish_class_member_access_expr (yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 376:
#line 1738 "parse.y"
    { yyval.ttype = finish_class_member_access_expr (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 377:
#line 1740 "parse.y"
    { yyval.ttype = finish_object_call_expr (yyvsp[-3].ttype, yyvsp[-4].ttype, yyvsp[-1].ttype); ;}
    break;

  case 378:
#line 1742 "parse.y"
    { yyval.ttype = finish_object_call_expr (yyvsp[-1].ttype, yyvsp[-2].ttype, NULL_TREE); ;}
    break;

  case 379:
#line 1744 "parse.y"
    { yyval.ttype = finish_qualified_object_call_expr (yyvsp[-3].ttype, yyvsp[-4].ttype, yyvsp[-1].ttype); ;}
    break;

  case 380:
#line 1746 "parse.y"
    { yyval.ttype = finish_qualified_object_call_expr (yyvsp[-1].ttype, yyvsp[-2].ttype, NULL_TREE); ;}
    break;

  case 381:
#line 1749 "parse.y"
    { yyval.ttype = finish_pseudo_destructor_call_expr (yyvsp[-3].ttype, NULL_TREE, yyvsp[-1].ttype); ;}
    break;

  case 382:
#line 1751 "parse.y"
    { yyval.ttype = finish_pseudo_destructor_call_expr (yyvsp[-5].ttype, yyvsp[-4].ttype, yyvsp[-1].ttype); ;}
    break;

  case 383:
#line 1753 "parse.y"
    {
		  yyval.ttype = error_mark_node;
		;}
    break;

  case 384:
#line 1798 "parse.y"
    { yyval.itype = 0; ;}
    break;

  case 385:
#line 1800 "parse.y"
    { got_scope = NULL_TREE; yyval.itype = 1; ;}
    break;

  case 386:
#line 1805 "parse.y"
    { yyval.itype = 0; ;}
    break;

  case 387:
#line 1807 "parse.y"
    { got_scope = NULL_TREE; yyval.itype = 1; ;}
    break;

  case 388:
#line 1812 "parse.y"
    { yyval.ttype = boolean_true_node; ;}
    break;

  case 389:
#line 1814 "parse.y"
    { yyval.ttype = boolean_false_node; ;}
    break;

  case 390:
#line 1819 "parse.y"
    {
		  if (DECL_CONSTRUCTOR_P (current_function_decl))
		    finish_mem_initializers (NULL_TREE);
		;}
    break;

  case 391:
#line 1827 "parse.y"
    { got_object = TREE_TYPE (yyval.ttype); ;}
    break;

  case 392:
#line 1829 "parse.y"
    {
		  yyval.ttype = build_x_arrow (yyval.ttype);
		  got_object = TREE_TYPE (yyval.ttype);
		;}
    break;

  case 393:
#line 1837 "parse.y"
    {
		  if (yyvsp[-2].ftype.t && IS_AGGR_TYPE_CODE (TREE_CODE (yyvsp[-2].ftype.t)))
		    note_got_semicolon (yyvsp[-2].ftype.t);
		;}
    break;

  case 394:
#line 1842 "parse.y"
    {
		  note_list_got_semicolon (yyvsp[-2].ftype.t);
		;}
    break;

  case 395:
#line 1846 "parse.y"
    {;}
    break;

  case 396:
#line 1848 "parse.y"
    {
		  shadow_tag (yyvsp[-1].ftype.t);
		  note_list_got_semicolon (yyvsp[-1].ftype.t);
		;}
    break;

  case 397:
#line 1853 "parse.y"
    { warning ("empty declaration"); ;}
    break;

  case 398:
#line 1855 "parse.y"
    { pedantic = yyvsp[-1].itype; ;}
    break;

  case 401:
#line 1869 "parse.y"
    { yyval.ttype = make_call_declarator (NULL_TREE, empty_parms (),
					     NULL_TREE, NULL_TREE); ;}
    break;

  case 402:
#line 1872 "parse.y"
    { yyval.ttype = make_call_declarator (yyval.ttype, empty_parms (), NULL_TREE,
					     NULL_TREE); ;}
    break;

  case 403:
#line 1879 "parse.y"
    { yyval.ftype.t = build_tree_list (yyvsp[-1].ftype.t, yyvsp[0].ttype);
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;}
    break;

  case 404:
#line 1882 "parse.y"
    { yyval.ftype.t = build_tree_list (yyvsp[-1].ftype.t, yyvsp[0].ttype);
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;}
    break;

  case 405:
#line 1885 "parse.y"
    { yyval.ftype.t = build_tree_list (build_tree_list (NULL_TREE, yyvsp[-1].ftype.t),
					  yyvsp[0].ttype);
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;}
    break;

  case 406:
#line 1889 "parse.y"
    { yyval.ftype.t = build_tree_list (yyvsp[0].ftype.t, NULL_TREE);
		  yyval.ftype.new_type_flag = yyvsp[0].ftype.new_type_flag;  ;}
    break;

  case 407:
#line 1892 "parse.y"
    { yyval.ftype.t = build_tree_list (yyvsp[0].ftype.t, NULL_TREE);
		  yyval.ftype.new_type_flag = yyvsp[0].ftype.new_type_flag; ;}
    break;

  case 408:
#line 1903 "parse.y"
    { yyval.ftype.lookups = type_lookups; ;}
    break;

  case 409:
#line 1905 "parse.y"
    { yyval.ftype.lookups = type_lookups; ;}
    break;

  case 410:
#line 1910 "parse.y"
    { yyval.ftype.t = tree_cons (NULL_TREE, yyvsp[0].ftype.t, yyvsp[-1].ftype.t);
		  yyval.ftype.new_type_flag = yyvsp[0].ftype.new_type_flag; ;}
    break;

  case 411:
#line 1913 "parse.y"
    { yyval.ftype.t = tree_cons (NULL_TREE, yyvsp[-1].ftype.t, yyvsp[0].ttype);
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;}
    break;

  case 412:
#line 1916 "parse.y"
    { yyval.ftype.t = tree_cons (NULL_TREE, yyvsp[-2].ftype.t, chainon (yyvsp[-1].ttype, yyvsp[0].ttype));
		  yyval.ftype.new_type_flag = yyvsp[-2].ftype.new_type_flag; ;}
    break;

  case 413:
#line 1919 "parse.y"
    { yyval.ftype.t = tree_cons (NULL_TREE, yyvsp[-1].ftype.t, chainon (yyvsp[0].ttype, yyvsp[-2].ftype.t));
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;}
    break;

  case 414:
#line 1922 "parse.y"
    { yyval.ftype.t = tree_cons (NULL_TREE, yyvsp[-1].ftype.t, chainon (yyvsp[0].ttype, yyvsp[-2].ftype.t));
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;}
    break;

  case 415:
#line 1925 "parse.y"
    { yyval.ftype.t = tree_cons (NULL_TREE, yyvsp[-2].ftype.t,
				    chainon (yyvsp[-1].ttype, chainon (yyvsp[0].ttype, yyvsp[-3].ftype.t)));
		  yyval.ftype.new_type_flag = yyvsp[-2].ftype.new_type_flag; ;}
    break;

  case 416:
#line 1932 "parse.y"
    { if (extra_warnings)
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyval.ttype));
		  yyval.ttype = build_tree_list (NULL_TREE, yyval.ttype); ;}
    break;

  case 417:
#line 1937 "parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ftype.t, yyval.ttype); ;}
    break;

  case 418:
#line 1939 "parse.y"
    { if (extra_warnings)
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyval.ttype); ;}
    break;

  case 419:
#line 1961 "parse.y"
    { yyval.ftype.lookups = NULL_TREE; TREE_STATIC (yyval.ftype.t) = 1; ;}
    break;

  case 420:
#line 1963 "parse.y"
    {
		  yyval.ftype.t = hash_tree_cons (NULL_TREE, yyvsp[0].ttype, NULL_TREE);
		  yyval.ftype.new_type_flag = 0; yyval.ftype.lookups = NULL_TREE;
		;}
    break;

  case 421:
#line 1968 "parse.y"
    {
		  yyval.ftype.t = hash_tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ftype.t);
		  TREE_STATIC (yyval.ftype.t) = 1;
		;}
    break;

  case 422:
#line 1973 "parse.y"
    {
		  if (extra_warnings && TREE_STATIC (yyval.ftype.t))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ftype.t = hash_tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ftype.t);
		  TREE_STATIC (yyval.ftype.t) = TREE_STATIC (yyvsp[-1].ftype.t);
		;}
    break;

  case 423:
#line 1981 "parse.y"
    { yyval.ftype.t = hash_tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ftype.t); ;}
    break;

  case 424:
#line 1992 "parse.y"
    { yyval.ftype.t = build_tree_list (NULL_TREE, yyvsp[0].ftype.t);
		  yyval.ftype.new_type_flag = yyvsp[0].ftype.new_type_flag; ;}
    break;

  case 425:
#line 1995 "parse.y"
    { yyval.ftype.t = tree_cons (NULL_TREE, yyvsp[0].ftype.t, yyvsp[-1].ftype.t);
		  yyval.ftype.new_type_flag = yyvsp[0].ftype.new_type_flag; ;}
    break;

  case 426:
#line 1998 "parse.y"
    { yyval.ftype.t = tree_cons (NULL_TREE, yyvsp[-1].ftype.t, yyvsp[0].ttype);
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;}
    break;

  case 427:
#line 2001 "parse.y"
    { yyval.ftype.t = tree_cons (NULL_TREE, yyvsp[-1].ftype.t, chainon (yyvsp[0].ttype, yyvsp[-2].ftype.t));
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;}
    break;

  case 428:
#line 2007 "parse.y"
    { yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ftype.t); ;}
    break;

  case 429:
#line 2009 "parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ftype.t, yyvsp[-1].ttype); ;}
    break;

  case 430:
#line 2011 "parse.y"
    { yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ttype); ;}
    break;

  case 431:
#line 2013 "parse.y"
    { yyval.ttype = tree_cons (yyvsp[0].ttype, NULL_TREE, NULL_TREE); ;}
    break;

  case 432:
#line 2017 "parse.y"
    { skip_evaluation++; ;}
    break;

  case 433:
#line 2021 "parse.y"
    { skip_evaluation++; ;}
    break;

  case 434:
#line 2025 "parse.y"
    { skip_evaluation++; ;}
    break;

  case 435:
#line 2034 "parse.y"
    { yyval.ftype.lookups = NULL_TREE; ;}
    break;

  case 436:
#line 2036 "parse.y"
    { yyval.ftype.t = yyvsp[0].ttype; yyval.ftype.new_type_flag = 0; yyval.ftype.lookups = NULL_TREE; ;}
    break;

  case 437:
#line 2038 "parse.y"
    { yyval.ftype.t = yyvsp[0].ttype; yyval.ftype.new_type_flag = 0; yyval.ftype.lookups = NULL_TREE; ;}
    break;

  case 438:
#line 2040 "parse.y"
    { yyval.ftype.t = finish_typeof (yyvsp[-1].ttype);
		  yyval.ftype.new_type_flag = 0; yyval.ftype.lookups = NULL_TREE;
		  skip_evaluation--; ;}
    break;

  case 439:
#line 2044 "parse.y"
    { yyval.ftype.t = groktypename (yyvsp[-1].ftype.t);
		  yyval.ftype.new_type_flag = 0; yyval.ftype.lookups = NULL_TREE;
		  skip_evaluation--; ;}
    break;

  case 440:
#line 2048 "parse.y"
    { tree type = TREE_TYPE (yyvsp[-1].ttype);

                  yyval.ftype.new_type_flag = 0; yyval.ftype.lookups = NULL_TREE;
		  if (IS_AGGR_TYPE (type))
		    {
		      sorry ("sigof type specifier");
		      yyval.ftype.t = type;
		    }
		  else
		    {
		      error ("`sigof' applied to non-aggregate expression");
		      yyval.ftype.t = error_mark_node;
		    }
		;}
    break;

  case 441:
#line 2063 "parse.y"
    { tree type = groktypename (yyvsp[-1].ftype.t);

                  yyval.ftype.new_type_flag = 0; yyval.ftype.lookups = NULL_TREE;
		  if (IS_AGGR_TYPE (type))
		    {
		      sorry ("sigof type specifier");
		      yyval.ftype.t = type;
		    }
		  else
		    {
		      error("`sigof' applied to non-aggregate type");
		      yyval.ftype.t = error_mark_node;
		    }
		;}
    break;

  case 442:
#line 2083 "parse.y"
    { yyval.ftype.t = yyvsp[0].ttype; yyval.ftype.new_type_flag = 0; ;}
    break;

  case 443:
#line 2085 "parse.y"
    { yyval.ftype.t = yyvsp[0].ttype; yyval.ftype.new_type_flag = 0; ;}
    break;

  case 446:
#line 2092 "parse.y"
    { check_multiple_declarators (); ;}
    break;

  case 448:
#line 2098 "parse.y"
    { check_multiple_declarators (); ;}
    break;

  case 450:
#line 2104 "parse.y"
    { check_multiple_declarators (); ;}
    break;

  case 451:
#line 2109 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 452:
#line 2111 "parse.y"
    { yyval.ttype = yyvsp[-1].ttype; ;}
    break;

  case 453:
#line 2116 "parse.y"
    { yyval.ttype = parse_decl (yyvsp[-3].ttype, yyvsp[-1].ttype, 1); ;}
    break;

  case 454:
#line 2119 "parse.y"
    { parse_end_decl (yyvsp[-1].ttype, yyvsp[0].ttype, yyvsp[-4].ttype); ;}
    break;

  case 455:
#line 2121 "parse.y"
    {
		  yyval.ttype = parse_decl (yyvsp[-2].ttype, yyvsp[0].ttype, 0);
		  parse_end_decl (yyval.ttype, NULL_TREE, yyvsp[-1].ttype);
		;}
    break;

  case 456:
#line 2135 "parse.y"
    { yyval.ttype = parse_decl0 (yyvsp[-3].ttype, yyvsp[-4].ftype.t,
					   yyvsp[-4].ftype.lookups, yyvsp[-1].ttype, 1); ;}
    break;

  case 457:
#line 2140 "parse.y"
    { parse_end_decl (yyvsp[-1].ttype, yyvsp[0].ttype, yyvsp[-4].ttype); ;}
    break;

  case 458:
#line 2142 "parse.y"
    { tree d = parse_decl0 (yyvsp[-2].ttype, yyvsp[-3].ftype.t,
					yyvsp[-3].ftype.lookups, yyvsp[0].ttype, 0);
		  parse_end_decl (d, NULL_TREE, yyvsp[-1].ttype); ;}
    break;

  case 459:
#line 2149 "parse.y"
    {;}
    break;

  case 460:
#line 2154 "parse.y"
    {;}
    break;

  case 461:
#line 2159 "parse.y"
    { /* Set things up as initdcl0_innards expects.  */
	      yyval.ttype = yyvsp[0].ttype;
	      yyvsp[0].ttype = yyvsp[-1].ttype;
              yyvsp[-1].ftype.t = NULL_TREE;
	      yyvsp[-1].ftype.lookups = NULL_TREE; ;}
    break;

  case 462:
#line 2165 "parse.y"
    {;}
    break;

  case 463:
#line 2167 "parse.y"
    { tree d = parse_decl0 (yyvsp[-2].ttype, NULL_TREE, NULL_TREE, yyvsp[0].ttype, 0);
		  parse_end_decl (d, NULL_TREE, yyvsp[-1].ttype); ;}
    break;

  case 464:
#line 2175 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 465:
#line 2177 "parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 466:
#line 2182 "parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 467:
#line 2184 "parse.y"
    { yyval.ttype = chainon (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 468:
#line 2189 "parse.y"
    { yyval.ttype = yyvsp[-2].ttype; ;}
    break;

  case 469:
#line 2194 "parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 470:
#line 2196 "parse.y"
    { yyval.ttype = chainon (yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 471:
#line 2201 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 472:
#line 2203 "parse.y"
    { yyval.ttype = build_tree_list (yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 473:
#line 2205 "parse.y"
    { yyval.ttype = build_tree_list (yyvsp[-3].ttype, build_tree_list (NULL_TREE, yyvsp[-1].ttype)); ;}
    break;

  case 474:
#line 2207 "parse.y"
    { yyval.ttype = build_tree_list (yyvsp[-5].ttype, tree_cons (NULL_TREE, yyvsp[-3].ttype, yyvsp[-1].ttype)); ;}
    break;

  case 475:
#line 2209 "parse.y"
    { yyval.ttype = build_tree_list (yyvsp[-3].ttype, yyvsp[-1].ttype); ;}
    break;

  case 480:
#line 2225 "parse.y"
    { yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ttype); ;}
    break;

  case 481:
#line 2227 "parse.y"
    { yyval.ttype = chainon (yyvsp[-2].ttype, build_tree_list (NULL_TREE, yyvsp[0].ttype)); ;}
    break;

  case 482:
#line 2232 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 483:
#line 2234 "parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 485:
#line 2243 "parse.y"
    { yyval.ttype = build_nt (CONSTRUCTOR, NULL_TREE, NULL_TREE);
		  TREE_HAS_CONSTRUCTOR (yyval.ttype) = 1; ;}
    break;

  case 486:
#line 2246 "parse.y"
    { yyval.ttype = build_nt (CONSTRUCTOR, NULL_TREE, nreverse (yyvsp[-1].ttype));
		  TREE_HAS_CONSTRUCTOR (yyval.ttype) = 1; ;}
    break;

  case 487:
#line 2249 "parse.y"
    { yyval.ttype = build_nt (CONSTRUCTOR, NULL_TREE, nreverse (yyvsp[-2].ttype));
		  TREE_HAS_CONSTRUCTOR (yyval.ttype) = 1; ;}
    break;

  case 488:
#line 2252 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 489:
#line 2259 "parse.y"
    { yyval.ttype = build_tree_list (NULL_TREE, yyval.ttype); ;}
    break;

  case 490:
#line 2261 "parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyval.ttype); ;}
    break;

  case 491:
#line 2264 "parse.y"
    { yyval.ttype = build_tree_list (yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 492:
#line 2266 "parse.y"
    { yyval.ttype = build_tree_list (yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 493:
#line 2268 "parse.y"
    { yyval.ttype = tree_cons (yyvsp[-2].ttype, yyvsp[0].ttype, yyval.ttype); ;}
    break;

  case 494:
#line 2273 "parse.y"
    {
		  expand_body (finish_function (2));
		  process_next_inline (yyvsp[-2].pi);
		;}
    break;

  case 495:
#line 2278 "parse.y"
    {
		  expand_body (finish_function (2));
                  process_next_inline (yyvsp[-2].pi);
		;}
    break;

  case 496:
#line 2283 "parse.y"
    {
		  finish_function (2);
		  process_next_inline (yyvsp[-2].pi); ;}
    break;

  case 499:
#line 2297 "parse.y"
    { replace_defarg (yyvsp[-2].ttype, yyvsp[-1].ttype); ;}
    break;

  case 500:
#line 2299 "parse.y"
    { replace_defarg (yyvsp[-2].ttype, error_mark_node); ;}
    break;

  case 502:
#line 2305 "parse.y"
    { do_pending_defargs (); ;}
    break;

  case 503:
#line 2307 "parse.y"
    { do_pending_defargs (); ;}
    break;

  case 504:
#line 2312 "parse.y"
    { yyval.ttype = current_enum_type;
		  current_enum_type = start_enum (yyvsp[-1].ttype); ;}
    break;

  case 505:
#line 2315 "parse.y"
    { yyval.ftype.t = current_enum_type;
		  finish_enum (current_enum_type);
		  yyval.ftype.new_type_flag = 1;
		  current_enum_type = yyvsp[-2].ttype;
		  check_for_missing_semicolon (yyval.ftype.t); ;}
    break;

  case 506:
#line 2321 "parse.y"
    { yyval.ttype = current_enum_type;
		  current_enum_type = start_enum (make_anon_name ()); ;}
    break;

  case 507:
#line 2324 "parse.y"
    { yyval.ftype.t = current_enum_type;
		  finish_enum (current_enum_type);
		  yyval.ftype.new_type_flag = 1;
		  current_enum_type = yyvsp[-2].ttype;
		  check_for_missing_semicolon (yyval.ftype.t); ;}
    break;

  case 508:
#line 2330 "parse.y"
    { yyval.ftype.t = parse_xref_tag (enum_type_node, yyvsp[0].ttype, 1);
		  yyval.ftype.new_type_flag = 0; ;}
    break;

  case 509:
#line 2333 "parse.y"
    { yyval.ftype.t = parse_xref_tag (enum_type_node, yyvsp[0].ttype, 1);
		  yyval.ftype.new_type_flag = 0; ;}
    break;

  case 510:
#line 2336 "parse.y"
    { yyval.ftype.t = yyvsp[0].ttype;
		  yyval.ftype.new_type_flag = 0;
		  if (!processing_template_decl)
		    pedwarn ("using `typename' outside of template"); ;}
    break;

  case 511:
#line 2342 "parse.y"
    {
		  if (yyvsp[-1].ttype && yyvsp[-2].ftype.t != error_mark_node)
		    {
		      tree type = TREE_TYPE (yyvsp[-2].ftype.t);

		      if (TREE_CODE (type) == TYPENAME_TYPE)
			{
			  if (IMPLICIT_TYPENAME_P (type))
			    /* In a definition of a member class template,
			       we will get here with an implicit typename,
			       a TYPENAME_TYPE with a type. */
			    type = TREE_TYPE (type);
			  else
			    {
			      error ("qualified name does not name a class");
			      type = error_mark_node;
			    }
			}
		      maybe_process_partial_specialization (type);
		      xref_basetypes (type, yyvsp[-1].ttype);
		    }
		  yyvsp[-2].ftype.t = begin_class_definition (TREE_TYPE (yyvsp[-2].ftype.t));
		  check_class_key (current_aggr, yyvsp[-2].ftype.t);
                  current_aggr = NULL_TREE; ;}
    break;

  case 512:
#line 2367 "parse.y"
    {
		  int semi;
		  tree t;

		  if (yychar == YYEMPTY)
		    yychar = YYLEX;
		  semi = yychar == ';';

		  t = finish_class_definition (yyvsp[-6].ftype.t, yyvsp[0].ttype, semi, yyvsp[-6].ftype.new_type_flag);
		  yyval.ttype = t;

		  /* restore current_aggr */
		  current_aggr = TREE_CODE (t) != RECORD_TYPE
				 ? union_type_node
				 : CLASSTYPE_DECLARED_CLASS (t)
				 ? class_type_node : record_type_node;
		;}
    break;

  case 513:
#line 2385 "parse.y"
    {
		  done_pending_defargs ();
		  begin_inline_definitions ();
		;}
    break;

  case 514:
#line 2390 "parse.y"
    {
		  yyval.ftype.t = yyvsp[-3].ttype;
		  yyval.ftype.new_type_flag = 1;
		;}
    break;

  case 515:
#line 2395 "parse.y"
    {
		  yyval.ftype.t = TREE_TYPE (yyvsp[0].ftype.t);
		  yyval.ftype.new_type_flag = yyvsp[0].ftype.new_type_flag;
		  check_class_key (current_aggr, yyval.ftype.t);
		;}
    break;

  case 519:
#line 2410 "parse.y"
    { if (pedantic && !in_system_header)
		    pedwarn ("comma at end of enumerator list"); ;}
    break;

  case 521:
#line 2417 "parse.y"
    { error ("storage class specifier `%s' not allowed after struct or class", IDENTIFIER_POINTER (yyvsp[0].ttype)); ;}
    break;

  case 522:
#line 2419 "parse.y"
    { error ("type specifier `%s' not allowed after struct or class", IDENTIFIER_POINTER (yyvsp[0].ttype)); ;}
    break;

  case 523:
#line 2421 "parse.y"
    { error ("type qualifier `%s' not allowed after struct or class", IDENTIFIER_POINTER (yyvsp[0].ttype)); ;}
    break;

  case 524:
#line 2423 "parse.y"
    { error ("no body nor ';' separates two class, struct or union declarations"); ;}
    break;

  case 525:
#line 2425 "parse.y"
    { yyval.ttype = build_tree_list (yyvsp[0].ttype, yyvsp[-1].ttype); ;}
    break;

  case 526:
#line 2430 "parse.y"
    {
		  current_aggr = yyvsp[-1].ttype;
		  yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ttype);
		;}
    break;

  case 527:
#line 2435 "parse.y"
    {
		  current_aggr = yyvsp[-2].ttype;
		  yyval.ttype = build_tree_list (yyvsp[-1].ttype, yyvsp[0].ttype);
		;}
    break;

  case 528:
#line 2440 "parse.y"
    {
		  current_aggr = yyvsp[-3].ttype;
		  yyval.ttype = build_tree_list (yyvsp[-1].ttype, yyvsp[0].ttype);
		;}
    break;

  case 529:
#line 2445 "parse.y"
    {
		  current_aggr = yyvsp[-2].ttype;
		  yyval.ttype = build_tree_list (global_namespace, yyvsp[0].ttype);
		;}
    break;

  case 530:
#line 2453 "parse.y"
    {
		  current_aggr = yyvsp[-1].ttype;
		  yyval.ttype = yyvsp[0].ttype;
		;}
    break;

  case 531:
#line 2458 "parse.y"
    {
		  current_aggr = yyvsp[-2].ttype;
		  yyval.ttype = yyvsp[0].ttype;
		;}
    break;

  case 532:
#line 2463 "parse.y"
    {
		  current_aggr = yyvsp[-3].ttype;
		  yyval.ttype = yyvsp[0].ttype;
		;}
    break;

  case 533:
#line 2471 "parse.y"
    {
		  yyval.ftype.t = parse_handle_class_head (current_aggr,
						  TREE_PURPOSE (yyvsp[0].ttype), 
						  TREE_VALUE (yyvsp[0].ttype),
						  0, &yyval.ftype.new_type_flag);
		;}
    break;

  case 534:
#line 2478 "parse.y"
    {
		  current_aggr = yyvsp[-1].ttype;
		  yyval.ftype.t = TYPE_MAIN_DECL (parse_xref_tag (current_aggr, yyvsp[0].ttype, 0));
		  yyval.ftype.new_type_flag = 1;
		;}
    break;

  case 535:
#line 2484 "parse.y"
    {
		  yyval.ftype.t = yyvsp[0].ttype;
		  yyval.ftype.new_type_flag = 0;
		;}
    break;

  case 536:
#line 2492 "parse.y"
    {
		  yyungetc ('{', 1);
		  yyval.ftype.t = parse_handle_class_head (current_aggr,
						  TREE_PURPOSE (yyvsp[-1].ttype), 
						  TREE_VALUE (yyvsp[-1].ttype),
						  1, 
						  &yyval.ftype.new_type_flag);
		;}
    break;

  case 537:
#line 2501 "parse.y"
    {
		  yyungetc (':', 1);
		  yyval.ftype.t = parse_handle_class_head (current_aggr,
						  TREE_PURPOSE (yyvsp[-1].ttype), 
						  TREE_VALUE (yyvsp[-1].ttype),
						  1, &yyval.ftype.new_type_flag);
		;}
    break;

  case 538:
#line 2509 "parse.y"
    {
		  yyungetc ('{', 1);
		  yyval.ftype.t = handle_class_head_apparent_template
			   (yyvsp[-1].ttype, &yyval.ftype.new_type_flag);
		;}
    break;

  case 539:
#line 2515 "parse.y"
    {
		  yyungetc (':', 1);
		  yyval.ftype.t = handle_class_head_apparent_template
			   (yyvsp[-1].ttype, &yyval.ftype.new_type_flag);
		;}
    break;

  case 540:
#line 2521 "parse.y"
    {
		  yyungetc ('{', 1);
		  current_aggr = yyvsp[-2].ttype;
		  yyval.ftype.t = parse_handle_class_head (current_aggr,
						  NULL_TREE, yyvsp[-1].ttype,
						  1, &yyval.ftype.new_type_flag);
		;}
    break;

  case 541:
#line 2529 "parse.y"
    {
		  yyungetc (':', 1);
		  current_aggr = yyvsp[-2].ttype;
		  yyval.ftype.t = parse_handle_class_head (current_aggr,
						  NULL_TREE, yyvsp[-1].ttype,
						  1, &yyval.ftype.new_type_flag);
		;}
    break;

  case 542:
#line 2537 "parse.y"
    {
		  current_aggr = yyvsp[-1].ttype;
		  yyval.ftype.t = TYPE_MAIN_DECL (parse_xref_tag (yyvsp[-1].ttype, 
							 make_anon_name (), 
							 0));
		  yyval.ftype.new_type_flag = 0;
		  CLASSTYPE_DECLARED_CLASS (TREE_TYPE (yyval.ftype.t))
		    = yyvsp[-1].ttype == class_type_node;
		  yyungetc ('{', 1);
		;}
    break;

  case 543:
#line 2551 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 544:
#line 2553 "parse.y"
    { error ("no bases given following `:'");
		  yyval.ttype = NULL_TREE; ;}
    break;

  case 545:
#line 2556 "parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 547:
#line 2562 "parse.y"
    { yyval.ttype = chainon (yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 548:
#line 2567 "parse.y"
    { yyval.ttype = finish_base_specifier (access_default_node, yyvsp[0].ttype); ;}
    break;

  case 549:
#line 2569 "parse.y"
    { yyval.ttype = finish_base_specifier (yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 550:
#line 2574 "parse.y"
    { if (!TYPE_P (yyval.ttype))
		    yyval.ttype = error_mark_node; ;}
    break;

  case 551:
#line 2577 "parse.y"
    { yyval.ttype = TREE_TYPE (yyval.ttype); ;}
    break;

  case 553:
#line 2583 "parse.y"
    { if (yyvsp[-1].ttype != ridpointers[(int)RID_VIRTUAL])
		    error ("`%D' access", yyvsp[-1].ttype);
		  yyval.ttype = access_default_virtual_node; ;}
    break;

  case 554:
#line 2587 "parse.y"
    {
		  if (yyvsp[-2].ttype != access_default_virtual_node)
		    error ("multiple access specifiers");
		  else if (yyvsp[-1].ttype == access_public_node)
		    yyval.ttype = access_public_virtual_node;
		  else if (yyvsp[-1].ttype == access_protected_node)
		    yyval.ttype = access_protected_virtual_node;
		  else /* $2 == access_private_node */
		    yyval.ttype = access_private_virtual_node;
		;}
    break;

  case 555:
#line 2598 "parse.y"
    { if (yyvsp[-1].ttype != ridpointers[(int)RID_VIRTUAL])
		    error ("`%D' access", yyvsp[-1].ttype);
		  else if (yyval.ttype == access_public_node)
		    yyval.ttype = access_public_virtual_node;
		  else if (yyval.ttype == access_protected_node)
		    yyval.ttype = access_protected_virtual_node;
		  else if (yyval.ttype == access_private_node)
		    yyval.ttype = access_private_virtual_node;
		  else
		    error ("multiple `virtual' specifiers");
		;}
    break;

  case 560:
#line 2619 "parse.y"
    {
		  current_access_specifier = yyvsp[-1].ttype;
                ;}
    break;

  case 561:
#line 2628 "parse.y"
    {
		  finish_member_declaration (yyvsp[0].ttype);
		  current_aggr = NULL_TREE;
		  reset_type_access_control ();
		;}
    break;

  case 562:
#line 2634 "parse.y"
    {
		  finish_member_declaration (yyvsp[0].ttype);
		  current_aggr = NULL_TREE;
		  reset_type_access_control ();
		;}
    break;

  case 564:
#line 2644 "parse.y"
    { error ("missing ';' before right brace");
		  yyungetc ('}', 0); ;}
    break;

  case 565:
#line 2649 "parse.y"
    { yyval.ttype = finish_method (yyval.ttype); ;}
    break;

  case 566:
#line 2651 "parse.y"
    { yyval.ttype = finish_method (yyval.ttype); ;}
    break;

  case 567:
#line 2653 "parse.y"
    { yyval.ttype = finish_method (yyval.ttype); ;}
    break;

  case 568:
#line 2655 "parse.y"
    { yyval.ttype = finish_method (yyval.ttype); ;}
    break;

  case 569:
#line 2657 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 570:
#line 2659 "parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  pedantic = yyvsp[-1].itype; ;}
    break;

  case 571:
#line 2662 "parse.y"
    {
		  if (yyvsp[0].ttype)
		    yyval.ttype = finish_member_template_decl (yyvsp[0].ttype);
		  else
		    /* The component was already processed.  */
		    yyval.ttype = NULL_TREE;

		  finish_template_decl (yyvsp[-1].ttype);
		;}
    break;

  case 572:
#line 2672 "parse.y"
    {
		  yyval.ttype = finish_member_class_template (yyvsp[-1].ftype.t);
		  finish_template_decl (yyvsp[-2].ttype);
		;}
    break;

  case 573:
#line 2677 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 574:
#line 2685 "parse.y"
    {
		  /* Most of the productions for component_decl only
		     allow the creation of one new member, so we call
		     finish_member_declaration in component_decl_list.
		     For this rule and the next, however, there can be
		     more than one member, e.g.:

		       int i, j;

		     and we need the first member to be fully
		     registered before the second is processed.
		     Therefore, the rules for components take care of
		     this processing.  To avoid registering the
		     components more than once, we send NULL_TREE up
		     here; that lets finish_member_declaration know
		     that there is nothing to do.  */
		  if (!yyvsp[0].itype)
		    grok_x_components (yyvsp[-1].ftype.t);
		  yyval.ttype = NULL_TREE;
		;}
    break;

  case 575:
#line 2706 "parse.y"
    {
		  if (!yyvsp[0].itype)
		    grok_x_components (yyvsp[-1].ftype.t);
		  yyval.ttype = NULL_TREE;
		;}
    break;

  case 576:
#line 2712 "parse.y"
    { yyval.ttype = grokfield (yyval.ttype, NULL_TREE, yyvsp[0].ttype, yyvsp[-2].ttype, yyvsp[-1].ttype); ;}
    break;

  case 577:
#line 2714 "parse.y"
    { yyval.ttype = grokfield (yyval.ttype, NULL_TREE, yyvsp[0].ttype, yyvsp[-2].ttype, yyvsp[-1].ttype); ;}
    break;

  case 578:
#line 2716 "parse.y"
    { yyval.ttype = grokbitfield (NULL_TREE, NULL_TREE, yyvsp[0].ttype); ;}
    break;

  case 579:
#line 2718 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 580:
#line 2729 "parse.y"
    { tree specs, attrs;
		  split_specs_attrs (yyvsp[-4].ftype.t, &specs, &attrs);
		  yyval.ttype = grokfield (yyvsp[-3].ttype, specs, yyvsp[0].ttype, yyvsp[-2].ttype,
				  chainon (yyvsp[-1].ttype, attrs)); ;}
    break;

  case 581:
#line 2734 "parse.y"
    { yyval.ttype = grokfield (yyval.ttype, NULL_TREE, yyvsp[0].ttype, yyvsp[-2].ttype, yyvsp[-1].ttype); ;}
    break;

  case 582:
#line 2736 "parse.y"
    { yyval.ttype = do_class_using_decl (yyvsp[0].ttype); ;}
    break;

  case 583:
#line 2743 "parse.y"
    { yyval.itype = 0; ;}
    break;

  case 584:
#line 2745 "parse.y"
    {
		  if (PROCESSING_REAL_TEMPLATE_DECL_P ())
		    yyvsp[0].ttype = finish_member_template_decl (yyvsp[0].ttype);
		  finish_member_declaration (yyvsp[0].ttype);
		  yyval.itype = 1;
		;}
    break;

  case 585:
#line 2752 "parse.y"
    {
		  check_multiple_declarators ();
		  if (PROCESSING_REAL_TEMPLATE_DECL_P ())
		    yyvsp[0].ttype = finish_member_template_decl (yyvsp[0].ttype);
		  finish_member_declaration (yyvsp[0].ttype);
		  yyval.itype = 2;
		;}
    break;

  case 586:
#line 2763 "parse.y"
    { yyval.itype = 0; ;}
    break;

  case 587:
#line 2765 "parse.y"
    {
		  if (PROCESSING_REAL_TEMPLATE_DECL_P ())
		    yyvsp[0].ttype = finish_member_template_decl (yyvsp[0].ttype);
		  finish_member_declaration (yyvsp[0].ttype);
		  yyval.itype = 1;
		;}
    break;

  case 588:
#line 2772 "parse.y"
    {
		  check_multiple_declarators ();
		  if (PROCESSING_REAL_TEMPLATE_DECL_P ())
		    yyvsp[0].ttype = finish_member_template_decl (yyvsp[0].ttype);
		  finish_member_declaration (yyvsp[0].ttype);
		  yyval.itype = 2;
		;}
    break;

  case 593:
#line 2793 "parse.y"
    { yyval.ttype = parse_field0 (yyvsp[-3].ttype, yyvsp[-4].ftype.t, yyvsp[-4].ftype.lookups,
				     yyvsp[-1].ttype, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 594:
#line 2796 "parse.y"
    { yyval.ttype = parse_bitfield0 (yyvsp[-3].ttype, yyvsp[-4].ftype.t, yyvsp[-4].ftype.lookups,
					yyvsp[0].ttype, yyvsp[-1].ttype); ;}
    break;

  case 595:
#line 2802 "parse.y"
    { yyval.ttype = parse_field0 (yyvsp[-3].ttype, yyvsp[-4].ftype.t, yyvsp[-4].ftype.lookups,
				     yyvsp[-1].ttype, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 596:
#line 2805 "parse.y"
    { yyval.ttype = parse_field0 (yyvsp[-3].ttype, yyvsp[-4].ftype.t, yyvsp[-4].ftype.lookups,
				     yyvsp[-1].ttype, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 597:
#line 2808 "parse.y"
    { yyval.ttype = parse_bitfield0 (yyvsp[-3].ttype, yyvsp[-4].ftype.t, yyvsp[-4].ftype.lookups,
					yyvsp[0].ttype, yyvsp[-1].ttype); ;}
    break;

  case 598:
#line 2811 "parse.y"
    { yyval.ttype = parse_bitfield0 (NULL_TREE, yyvsp[-3].ftype.t,
					yyvsp[-3].ftype.lookups, yyvsp[0].ttype, yyvsp[-1].ttype); ;}
    break;

  case 599:
#line 2817 "parse.y"
    { yyval.ttype = parse_field (yyvsp[-3].ttype, yyvsp[-1].ttype, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 600:
#line 2819 "parse.y"
    { yyval.ttype = parse_bitfield (yyvsp[-3].ttype, yyvsp[0].ttype, yyvsp[-1].ttype); ;}
    break;

  case 601:
#line 2824 "parse.y"
    { yyval.ttype = parse_field (yyvsp[-3].ttype, yyvsp[-1].ttype, yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 602:
#line 2826 "parse.y"
    { yyval.ttype = parse_bitfield (yyvsp[-3].ttype, yyvsp[0].ttype, yyvsp[-1].ttype); ;}
    break;

  case 603:
#line 2828 "parse.y"
    { yyval.ttype = parse_bitfield (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype); ;}
    break;

  case 608:
#line 2847 "parse.y"
    { build_enumerator (yyvsp[0].ttype, NULL_TREE, current_enum_type); ;}
    break;

  case 609:
#line 2849 "parse.y"
    { build_enumerator (yyvsp[-2].ttype, yyvsp[0].ttype, current_enum_type); ;}
    break;

  case 610:
#line 2855 "parse.y"
    { yyval.ftype.t = build_tree_list (yyvsp[-1].ftype.t, yyvsp[0].ttype);
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;}
    break;

  case 611:
#line 2858 "parse.y"
    { yyval.ftype.t = build_tree_list (yyvsp[0].ftype.t, NULL_TREE);
		  yyval.ftype.new_type_flag = yyvsp[0].ftype.new_type_flag; ;}
    break;

  case 612:
#line 2863 "parse.y"
    {
		  if (pedantic)
		    pedwarn ("ISO C++ forbids array dimensions with parenthesized type in new");
		  yyval.ftype.t = build_nt (ARRAY_REF, TREE_VALUE (yyvsp[-4].ftype.t), yyvsp[-1].ttype);
		  yyval.ftype.t = build_tree_list (TREE_PURPOSE (yyvsp[-4].ftype.t), yyval.ftype.t);
		  yyval.ftype.new_type_flag = yyvsp[-4].ftype.new_type_flag;
		;}
    break;

  case 613:
#line 2874 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 614:
#line 2876 "parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyval.ttype); ;}
    break;

  case 615:
#line 2881 "parse.y"
    { yyval.ftype.t = hash_tree_cons (NULL_TREE, yyvsp[0].ttype, NULL_TREE);
		  yyval.ftype.new_type_flag = 0; ;}
    break;

  case 616:
#line 2884 "parse.y"
    { yyval.ftype.t = hash_tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ftype.t);
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;}
    break;

  case 617:
#line 2887 "parse.y"
    { yyval.ftype.t = hash_tree_cons (yyvsp[0].ttype, NULL_TREE, NULL_TREE);
		  yyval.ftype.new_type_flag = 0; ;}
    break;

  case 618:
#line 2890 "parse.y"
    { yyval.ftype.t = hash_tree_cons (yyvsp[0].ttype, NULL_TREE, yyvsp[-1].ftype.t);
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;}
    break;

  case 619:
#line 2900 "parse.y"
    { yyval.ttype = yyvsp[-1].ttype; ;}
    break;

  case 620:
#line 2902 "parse.y"
    { yyval.ttype = yyvsp[-1].ttype; ;}
    break;

  case 621:
#line 2904 "parse.y"
    { yyval.ttype = empty_parms (); ;}
    break;

  case 622:
#line 2906 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 624:
#line 2914 "parse.y"
    {
		  /* Provide support for '(' attributes '*' declarator ')'
		     etc */
		  yyval.ttype = tree_cons (yyvsp[-1].ttype, yyvsp[0].ttype, NULL_TREE);
		;}
    break;

  case 625:
#line 2924 "parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[-1].ftype.t, yyvsp[0].ttype); ;}
    break;

  case 626:
#line 2926 "parse.y"
    { yyval.ttype = make_reference_declarator (yyvsp[-1].ftype.t, yyvsp[0].ttype); ;}
    break;

  case 627:
#line 2928 "parse.y"
    { yyval.ttype = make_pointer_declarator (NULL_TREE, yyvsp[0].ttype); ;}
    break;

  case 628:
#line 2930 "parse.y"
    { yyval.ttype = make_reference_declarator (NULL_TREE, yyvsp[0].ttype); ;}
    break;

  case 629:
#line 2932 "parse.y"
    { tree arg = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype);
		  yyval.ttype = build_nt (SCOPE_REF, yyvsp[-2].ttype, arg);
		;}
    break;

  case 631:
#line 2940 "parse.y"
    { yyval.ttype = make_call_declarator (yyval.ttype, yyvsp[-2].ttype, yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 632:
#line 2942 "parse.y"
    { yyval.ttype = build_nt (ARRAY_REF, yyval.ttype, yyvsp[-1].ttype); ;}
    break;

  case 633:
#line 2944 "parse.y"
    { yyval.ttype = build_nt (ARRAY_REF, yyval.ttype, NULL_TREE); ;}
    break;

  case 634:
#line 2946 "parse.y"
    { yyval.ttype = yyvsp[-1].ttype; ;}
    break;

  case 635:
#line 2948 "parse.y"
    { push_nested_class (yyvsp[-1].ttype, 3);
		  yyval.ttype = build_nt (SCOPE_REF, yyval.ttype, yyvsp[0].ttype);
		  TREE_COMPLEXITY (yyval.ttype) = current_class_depth; ;}
    break;

  case 637:
#line 2956 "parse.y"
    {
		  if (TREE_CODE (yyvsp[0].ttype) == IDENTIFIER_NODE)
		    {
		      yyval.ttype = lookup_name (yyvsp[0].ttype, 1);
		      maybe_note_name_used_in_class (yyvsp[0].ttype, yyval.ttype);
		    }
		  else
		    yyval.ttype = yyvsp[0].ttype;
		;}
    break;

  case 638:
#line 2966 "parse.y"
    {
		  if (TREE_CODE (yyvsp[0].ttype) == IDENTIFIER_NODE)
		    yyval.ttype = IDENTIFIER_GLOBAL_VALUE (yyvsp[0].ttype);
		  else
		    yyval.ttype = yyvsp[0].ttype;
		  got_scope = NULL_TREE;
		;}
    break;

  case 641:
#line 2979 "parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 642:
#line 2984 "parse.y"
    { yyval.ttype = get_type_decl (yyvsp[0].ttype); ;}
    break;

  case 644:
#line 2993 "parse.y"
    {
		  /* Provide support for '(' attributes '*' declarator ')'
		     etc */
		  yyval.ttype = tree_cons (yyvsp[-1].ttype, yyvsp[0].ttype, NULL_TREE);
		;}
    break;

  case 645:
#line 3002 "parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[-1].ftype.t, yyvsp[0].ttype); ;}
    break;

  case 646:
#line 3004 "parse.y"
    { yyval.ttype = make_reference_declarator (yyvsp[-1].ftype.t, yyvsp[0].ttype); ;}
    break;

  case 647:
#line 3006 "parse.y"
    { yyval.ttype = make_pointer_declarator (NULL_TREE, yyvsp[0].ttype); ;}
    break;

  case 648:
#line 3008 "parse.y"
    { yyval.ttype = make_reference_declarator (NULL_TREE, yyvsp[0].ttype); ;}
    break;

  case 649:
#line 3010 "parse.y"
    { tree arg = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype);
		  yyval.ttype = build_nt (SCOPE_REF, yyvsp[-2].ttype, arg);
		;}
    break;

  case 651:
#line 3018 "parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[-1].ftype.t, yyvsp[0].ttype); ;}
    break;

  case 652:
#line 3020 "parse.y"
    { yyval.ttype = make_reference_declarator (yyvsp[-1].ftype.t, yyvsp[0].ttype); ;}
    break;

  case 653:
#line 3022 "parse.y"
    { yyval.ttype = make_pointer_declarator (NULL_TREE, yyvsp[0].ttype); ;}
    break;

  case 654:
#line 3024 "parse.y"
    { yyval.ttype = make_reference_declarator (NULL_TREE, yyvsp[0].ttype); ;}
    break;

  case 655:
#line 3026 "parse.y"
    { tree arg = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype);
		  yyval.ttype = build_nt (SCOPE_REF, yyvsp[-2].ttype, arg);
		;}
    break;

  case 657:
#line 3034 "parse.y"
    { yyval.ttype = make_call_declarator (yyval.ttype, yyvsp[-2].ttype, yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 658:
#line 3036 "parse.y"
    { yyval.ttype = yyvsp[-1].ttype; ;}
    break;

  case 659:
#line 3038 "parse.y"
    { yyval.ttype = build_nt (ARRAY_REF, yyval.ttype, yyvsp[-1].ttype); ;}
    break;

  case 660:
#line 3040 "parse.y"
    { yyval.ttype = build_nt (ARRAY_REF, yyval.ttype, NULL_TREE); ;}
    break;

  case 661:
#line 3042 "parse.y"
    { enter_scope_of (yyvsp[0].ttype); ;}
    break;

  case 662:
#line 3044 "parse.y"
    { enter_scope_of (yyvsp[0].ttype); yyval.ttype = yyvsp[0].ttype;;}
    break;

  case 663:
#line 3046 "parse.y"
    { yyval.ttype = build_nt (SCOPE_REF, global_namespace, yyvsp[0].ttype);
		  enter_scope_of (yyval.ttype);
		;}
    break;

  case 664:
#line 3050 "parse.y"
    { got_scope = NULL_TREE;
		  yyval.ttype = build_nt (SCOPE_REF, yyvsp[-1].ttype, yyvsp[0].ttype);
		  enter_scope_of (yyval.ttype);
		;}
    break;

  case 665:
#line 3058 "parse.y"
    { got_scope = NULL_TREE;
		  yyval.ttype = build_nt (SCOPE_REF, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 666:
#line 3061 "parse.y"
    { got_scope = NULL_TREE;
 		  yyval.ttype = build_nt (SCOPE_REF, yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 667:
#line 3067 "parse.y"
    { got_scope = NULL_TREE;
		  yyval.ttype = build_nt (SCOPE_REF, yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 668:
#line 3070 "parse.y"
    { got_scope = NULL_TREE;
		  yyval.ttype = build_nt (SCOPE_REF, yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 670:
#line 3077 "parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 671:
#line 3082 "parse.y"
    { yyval.ttype = build_functional_cast (yyvsp[-3].ftype.t, yyvsp[-1].ttype); ;}
    break;

  case 672:
#line 3084 "parse.y"
    { yyval.ttype = reparse_decl_as_expr (yyvsp[-3].ftype.t, yyvsp[-1].ttype); ;}
    break;

  case 673:
#line 3086 "parse.y"
    { yyval.ttype = reparse_absdcl_as_expr (yyvsp[-1].ftype.t, yyvsp[0].ttype); ;}
    break;

  case 678:
#line 3098 "parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 679:
#line 3100 "parse.y"
    { got_scope = yyval.ttype
		    = make_typename_type (yyvsp[-3].ttype, yyvsp[-1].ttype, tf_error | tf_parsing); ;}
    break;

  case 680:
#line 3104 "parse.y"
    { got_scope = yyval.ttype
		    = make_typename_type (yyvsp[-2].ttype, yyvsp[-1].ttype, tf_error | tf_parsing); ;}
    break;

  case 681:
#line 3107 "parse.y"
    { got_scope = yyval.ttype
		    = make_typename_type (yyvsp[-2].ttype, yyvsp[-1].ttype, tf_error | tf_parsing); ;}
    break;

  case 682:
#line 3115 "parse.y"
    {
		  if (TREE_CODE (yyvsp[-1].ttype) == IDENTIFIER_NODE)
		    {
		      yyval.ttype = lastiddecl;
		      maybe_note_name_used_in_class (yyvsp[-1].ttype, yyval.ttype);
		    }
		  got_scope = yyval.ttype =
		    complete_type (TYPE_MAIN_VARIANT (TREE_TYPE (yyval.ttype)));
		;}
    break;

  case 683:
#line 3125 "parse.y"
    {
		  if (TREE_CODE (yyvsp[-1].ttype) == IDENTIFIER_NODE)
		    yyval.ttype = lastiddecl;
		  got_scope = yyval.ttype = TREE_TYPE (yyval.ttype);
		;}
    break;

  case 684:
#line 3131 "parse.y"
    {
		  if (TREE_CODE (yyval.ttype) == IDENTIFIER_NODE)
		    yyval.ttype = lastiddecl;
		  got_scope = yyval.ttype;
		;}
    break;

  case 685:
#line 3137 "parse.y"
    { got_scope = yyval.ttype = complete_type (TREE_TYPE (yyvsp[-1].ttype)); ;}
    break;

  case 687:
#line 3143 "parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 688:
#line 3148 "parse.y"
    {
		  if (TYPE_P (yyvsp[-1].ttype))
		    yyval.ttype = make_typename_type (yyvsp[-1].ttype, yyvsp[0].ttype, tf_error | tf_parsing);
		  else if (TREE_CODE (yyvsp[0].ttype) == IDENTIFIER_NODE)
		    error ("`%T' is not a class or namespace", yyvsp[0].ttype);
		  else
		    {
		      yyval.ttype = yyvsp[0].ttype;
		      if (TREE_CODE (yyval.ttype) == TYPE_DECL)
			yyval.ttype = TREE_TYPE (yyval.ttype);
		    }
		;}
    break;

  case 689:
#line 3161 "parse.y"
    { yyval.ttype = TREE_TYPE (yyvsp[0].ttype); ;}
    break;

  case 690:
#line 3163 "parse.y"
    { yyval.ttype = make_typename_type (yyvsp[-1].ttype, yyvsp[0].ttype, tf_error | tf_parsing); ;}
    break;

  case 691:
#line 3165 "parse.y"
    { yyval.ttype = make_typename_type (yyvsp[-2].ttype, yyvsp[0].ttype, tf_error | tf_parsing); ;}
    break;

  case 692:
#line 3170 "parse.y"
    {
		  if (TREE_CODE (yyvsp[0].ttype) == IDENTIFIER_NODE)
		    error ("`%T' is not a class or namespace", yyvsp[0].ttype);
		  else if (TREE_CODE (yyvsp[0].ttype) == TYPE_DECL)
		    yyval.ttype = TREE_TYPE (yyvsp[0].ttype);
		;}
    break;

  case 693:
#line 3177 "parse.y"
    {
		  if (TYPE_P (yyvsp[-1].ttype))
		    yyval.ttype = make_typename_type (yyvsp[-1].ttype, yyvsp[0].ttype, tf_error | tf_parsing);
		  else if (TREE_CODE (yyvsp[0].ttype) == IDENTIFIER_NODE)
		    error ("`%T' is not a class or namespace", yyvsp[0].ttype);
		  else
		    {
		      yyval.ttype = yyvsp[0].ttype;
		      if (TREE_CODE (yyval.ttype) == TYPE_DECL)
			yyval.ttype = TREE_TYPE (yyval.ttype);
		    }
		;}
    break;

  case 694:
#line 3190 "parse.y"
    { got_scope = yyval.ttype
		    = make_typename_type (yyvsp[-2].ttype, yyvsp[-1].ttype, tf_error | tf_parsing); ;}
    break;

  case 695:
#line 3193 "parse.y"
    { got_scope = yyval.ttype
		    = make_typename_type (yyvsp[-3].ttype, yyvsp[-1].ttype, tf_error | tf_parsing); ;}
    break;

  case 696:
#line 3201 "parse.y"
    {
		  if (TREE_CODE (yyvsp[-1].ttype) != TYPE_DECL)
		    yyval.ttype = lastiddecl;

		  /* Retrieve the type for the identifier, which might involve
		     some computation. */
		  got_scope = complete_type (TREE_TYPE (yyval.ttype));

		  if (yyval.ttype == error_mark_node)
		    error ("`%T' is not a class or namespace", yyvsp[-1].ttype);
		;}
    break;

  case 697:
#line 3213 "parse.y"
    {
		  if (TREE_CODE (yyvsp[-1].ttype) != TYPE_DECL)
		    yyval.ttype = lastiddecl;
		  got_scope = complete_type (TREE_TYPE (yyval.ttype));
		;}
    break;

  case 698:
#line 3219 "parse.y"
    { got_scope = yyval.ttype = complete_type (TREE_TYPE (yyval.ttype)); ;}
    break;

  case 701:
#line 3223 "parse.y"
    {
		  if (TREE_CODE (yyval.ttype) == IDENTIFIER_NODE)
		    yyval.ttype = lastiddecl;
		  got_scope = yyval.ttype;
		;}
    break;

  case 702:
#line 3232 "parse.y"
    { yyval.ttype = build_min_nt (TEMPLATE_ID_EXPR, yyvsp[-3].ttype, yyvsp[-1].ttype); ;}
    break;

  case 703:
#line 3237 "parse.y"
    {
		  if (TREE_CODE (yyvsp[0].ttype) == IDENTIFIER_NODE)
		    yyval.ttype = IDENTIFIER_GLOBAL_VALUE (yyvsp[0].ttype);
		  else
		    yyval.ttype = yyvsp[0].ttype;
		  got_scope = NULL_TREE;
		;}
    break;

  case 705:
#line 3246 "parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 706:
#line 3251 "parse.y"
    { got_scope = NULL_TREE; ;}
    break;

  case 707:
#line 3253 "parse.y"
    { yyval.ttype = yyvsp[-1].ttype; got_scope = NULL_TREE; ;}
    break;

  case 708:
#line 3260 "parse.y"
    { got_scope = void_type_node; ;}
    break;

  case 709:
#line 3266 "parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 710:
#line 3268 "parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 711:
#line 3270 "parse.y"
    { yyval.ttype = make_reference_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 712:
#line 3272 "parse.y"
    { yyval.ttype = make_reference_declarator (yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 713:
#line 3274 "parse.y"
    { tree arg = make_pointer_declarator (yyvsp[0].ttype, NULL_TREE);
		  yyval.ttype = build_nt (SCOPE_REF, yyvsp[-1].ttype, arg);
		;}
    break;

  case 714:
#line 3278 "parse.y"
    { tree arg = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype);
		  yyval.ttype = build_nt (SCOPE_REF, yyvsp[-2].ttype, arg);
		;}
    break;

  case 716:
#line 3287 "parse.y"
    { yyval.ttype = build_nt (ARRAY_REF, NULL_TREE, yyvsp[-1].ttype); ;}
    break;

  case 717:
#line 3289 "parse.y"
    { yyval.ttype = build_nt (ARRAY_REF, yyval.ttype, yyvsp[-1].ttype); ;}
    break;

  case 719:
#line 3295 "parse.y"
    {
		  /* Provide support for '(' attributes '*' declarator ')'
		     etc */
		  yyval.ttype = tree_cons (yyvsp[-1].ttype, yyvsp[0].ttype, NULL_TREE);
		;}
    break;

  case 720:
#line 3305 "parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[-1].ftype.t, yyvsp[0].ttype); ;}
    break;

  case 721:
#line 3307 "parse.y"
    { yyval.ttype = make_pointer_declarator (NULL_TREE, yyvsp[0].ttype); ;}
    break;

  case 722:
#line 3309 "parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[0].ftype.t, NULL_TREE); ;}
    break;

  case 723:
#line 3311 "parse.y"
    { yyval.ttype = make_pointer_declarator (NULL_TREE, NULL_TREE); ;}
    break;

  case 724:
#line 3313 "parse.y"
    { yyval.ttype = make_reference_declarator (yyvsp[-1].ftype.t, yyvsp[0].ttype); ;}
    break;

  case 725:
#line 3315 "parse.y"
    { yyval.ttype = make_reference_declarator (NULL_TREE, yyvsp[0].ttype); ;}
    break;

  case 726:
#line 3317 "parse.y"
    { yyval.ttype = make_reference_declarator (yyvsp[0].ftype.t, NULL_TREE); ;}
    break;

  case 727:
#line 3319 "parse.y"
    { yyval.ttype = make_reference_declarator (NULL_TREE, NULL_TREE); ;}
    break;

  case 728:
#line 3321 "parse.y"
    { tree arg = make_pointer_declarator (yyvsp[0].ttype, NULL_TREE);
		  yyval.ttype = build_nt (SCOPE_REF, yyvsp[-1].ttype, arg);
		;}
    break;

  case 729:
#line 3325 "parse.y"
    { tree arg = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype);
		  yyval.ttype = build_nt (SCOPE_REF, yyvsp[-2].ttype, arg);
		;}
    break;

  case 731:
#line 3334 "parse.y"
    { yyval.ttype = yyvsp[-1].ttype; ;}
    break;

  case 732:
#line 3337 "parse.y"
    { yyval.ttype = make_call_declarator (yyval.ttype, yyvsp[-3].ttype, yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 733:
#line 3339 "parse.y"
    { yyval.ttype = make_call_declarator (yyval.ttype, empty_parms (), yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 734:
#line 3341 "parse.y"
    { yyval.ttype = build_nt (ARRAY_REF, yyval.ttype, yyvsp[-1].ttype); ;}
    break;

  case 735:
#line 3343 "parse.y"
    { yyval.ttype = build_nt (ARRAY_REF, yyval.ttype, NULL_TREE); ;}
    break;

  case 736:
#line 3345 "parse.y"
    { yyval.ttype = make_call_declarator (NULL_TREE, yyvsp[-3].ttype, yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 737:
#line 3347 "parse.y"
    { set_quals_and_spec (yyval.ttype, yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 738:
#line 3349 "parse.y"
    { set_quals_and_spec (yyval.ttype, yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 739:
#line 3351 "parse.y"
    { yyval.ttype = build_nt (ARRAY_REF, NULL_TREE, yyvsp[-1].ttype); ;}
    break;

  case 740:
#line 3353 "parse.y"
    { yyval.ttype = build_nt (ARRAY_REF, NULL_TREE, NULL_TREE); ;}
    break;

  case 747:
#line 3376 "parse.y"
    { if (pedantic)
		    pedwarn ("ISO C++ forbids label declarations"); ;}
    break;

  case 750:
#line 3387 "parse.y"
    {
		  while (yyvsp[-1].ttype)
		    {
		      finish_label_decl (TREE_VALUE (yyvsp[-1].ttype));
		      yyvsp[-1].ttype = TREE_CHAIN (yyvsp[-1].ttype);
		    }
		;}
    break;

  case 751:
#line 3398 "parse.y"
    { yyval.ttype = begin_compound_stmt (0); ;}
    break;

  case 752:
#line 3400 "parse.y"
    { STMT_LINENO (yyvsp[-1].ttype) = yyvsp[-3].itype;
		  finish_compound_stmt (0, yyvsp[-1].ttype); ;}
    break;

  case 753:
#line 3406 "parse.y"
    { last_expr_type = NULL_TREE; ;}
    break;

  case 754:
#line 3411 "parse.y"
    { yyval.ttype = begin_if_stmt ();
		  cond_stmt_keyword = "if"; ;}
    break;

  case 755:
#line 3414 "parse.y"
    { finish_if_stmt_cond (yyvsp[0].ttype, yyvsp[-1].ttype); ;}
    break;

  case 756:
#line 3416 "parse.y"
    { yyval.ttype = yyvsp[-3].ttype;
		  finish_then_clause (yyvsp[-3].ttype); ;}
    break;

  case 758:
#line 3423 "parse.y"
    { yyval.ttype = begin_compound_stmt (0); ;}
    break;

  case 759:
#line 3425 "parse.y"
    { STMT_LINENO (yyvsp[-2].ttype) = yyvsp[-1].itype;
		  if (yyvsp[0].ttype) STMT_LINENO (yyvsp[0].ttype) = yyvsp[-1].itype;
		  finish_compound_stmt (0, yyvsp[-2].ttype); ;}
    break;

  case 761:
#line 3433 "parse.y"
    { if (yyvsp[0].ttype) STMT_LINENO (yyvsp[0].ttype) = yyvsp[-1].itype; ;}
    break;

  case 762:
#line 3438 "parse.y"
    { finish_stmt ();
		  yyval.ttype = NULL_TREE; ;}
    break;

  case 763:
#line 3441 "parse.y"
    { yyval.ttype = finish_expr_stmt (yyvsp[-1].ttype); ;}
    break;

  case 764:
#line 3443 "parse.y"
    { begin_else_clause (); ;}
    break;

  case 765:
#line 3445 "parse.y"
    {
		  yyval.ttype = yyvsp[-3].ttype;
		  finish_else_clause (yyvsp[-3].ttype);
		  finish_if_stmt ();
		;}
    break;

  case 766:
#line 3451 "parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  finish_if_stmt (); ;}
    break;

  case 767:
#line 3454 "parse.y"
    {
		  yyval.ttype = begin_while_stmt ();
		  cond_stmt_keyword = "while";
		;}
    break;

  case 768:
#line 3459 "parse.y"
    { finish_while_stmt_cond (yyvsp[0].ttype, yyvsp[-1].ttype); ;}
    break;

  case 769:
#line 3461 "parse.y"
    { yyval.ttype = yyvsp[-3].ttype;
		  finish_while_stmt (yyvsp[-3].ttype); ;}
    break;

  case 770:
#line 3464 "parse.y"
    { yyval.ttype = begin_do_stmt (); ;}
    break;

  case 771:
#line 3466 "parse.y"
    {
		  finish_do_body (yyvsp[-2].ttype);
		  cond_stmt_keyword = "do";
		;}
    break;

  case 772:
#line 3471 "parse.y"
    { yyval.ttype = yyvsp[-5].ttype;
		  finish_do_stmt (yyvsp[-1].ttype, yyvsp[-5].ttype); ;}
    break;

  case 773:
#line 3474 "parse.y"
    { yyval.ttype = begin_for_stmt (); ;}
    break;

  case 774:
#line 3476 "parse.y"
    { finish_for_init_stmt (yyvsp[-2].ttype); ;}
    break;

  case 775:
#line 3478 "parse.y"
    { finish_for_cond (yyvsp[-1].ttype, yyvsp[-5].ttype); ;}
    break;

  case 776:
#line 3480 "parse.y"
    { finish_for_expr (yyvsp[-1].ttype, yyvsp[-8].ttype); ;}
    break;

  case 777:
#line 3482 "parse.y"
    { yyval.ttype = yyvsp[-10].ttype;
		  finish_for_stmt (yyvsp[-10].ttype); ;}
    break;

  case 778:
#line 3485 "parse.y"
    { yyval.ttype = begin_switch_stmt (); ;}
    break;

  case 779:
#line 3487 "parse.y"
    { finish_switch_cond (yyvsp[-1].ttype, yyvsp[-3].ttype); ;}
    break;

  case 780:
#line 3489 "parse.y"
    { yyval.ttype = yyvsp[-5].ttype;
		  finish_switch_stmt (yyvsp[-5].ttype); ;}
    break;

  case 781:
#line 3492 "parse.y"
    { yyval.ttype = finish_case_label (yyvsp[-1].ttype, NULL_TREE); ;}
    break;

  case 782:
#line 3494 "parse.y"
    { yyval.ttype = yyvsp[-1].ttype; ;}
    break;

  case 783:
#line 3496 "parse.y"
    { yyval.ttype = finish_case_label (yyvsp[-3].ttype, yyvsp[-1].ttype); ;}
    break;

  case 784:
#line 3498 "parse.y"
    { yyval.ttype = yyvsp[-1].ttype; ;}
    break;

  case 785:
#line 3500 "parse.y"
    { yyval.ttype = finish_case_label (NULL_TREE, NULL_TREE); ;}
    break;

  case 786:
#line 3502 "parse.y"
    { yyval.ttype = yyvsp[-1].ttype; ;}
    break;

  case 787:
#line 3504 "parse.y"
    { yyval.ttype = finish_break_stmt (); ;}
    break;

  case 788:
#line 3506 "parse.y"
    { yyval.ttype = finish_continue_stmt (); ;}
    break;

  case 789:
#line 3508 "parse.y"
    { yyval.ttype = finish_return_stmt (NULL_TREE); ;}
    break;

  case 790:
#line 3510 "parse.y"
    { yyval.ttype = finish_return_stmt (yyvsp[-1].ttype); ;}
    break;

  case 791:
#line 3512 "parse.y"
    { yyval.ttype = finish_asm_stmt (yyvsp[-4].ttype, yyvsp[-2].ttype, NULL_TREE, NULL_TREE,
					NULL_TREE);
		  ASM_INPUT_P (yyval.ttype) = 1; ;}
    break;

  case 792:
#line 3517 "parse.y"
    { yyval.ttype = finish_asm_stmt (yyvsp[-6].ttype, yyvsp[-4].ttype, yyvsp[-2].ttype, NULL_TREE, NULL_TREE); ;}
    break;

  case 793:
#line 3521 "parse.y"
    { yyval.ttype = finish_asm_stmt (yyvsp[-8].ttype, yyvsp[-6].ttype, yyvsp[-4].ttype, yyvsp[-2].ttype, NULL_TREE); ;}
    break;

  case 794:
#line 3523 "parse.y"
    { yyval.ttype = finish_asm_stmt (yyvsp[-6].ttype, yyvsp[-4].ttype, NULL_TREE, yyvsp[-2].ttype, NULL_TREE); ;}
    break;

  case 795:
#line 3527 "parse.y"
    { yyval.ttype = finish_asm_stmt (yyvsp[-10].ttype, yyvsp[-8].ttype, yyvsp[-6].ttype, yyvsp[-4].ttype, yyvsp[-2].ttype); ;}
    break;

  case 796:
#line 3530 "parse.y"
    { yyval.ttype = finish_asm_stmt (yyvsp[-8].ttype, yyvsp[-6].ttype, NULL_TREE, yyvsp[-4].ttype, yyvsp[-2].ttype); ;}
    break;

  case 797:
#line 3533 "parse.y"
    { yyval.ttype = finish_asm_stmt (yyvsp[-8].ttype, yyvsp[-6].ttype, yyvsp[-4].ttype, NULL_TREE, yyvsp[-2].ttype); ;}
    break;

  case 798:
#line 3535 "parse.y"
    {
		  if (pedantic)
		    pedwarn ("ISO C++ forbids computed gotos");
		  yyval.ttype = finish_goto_stmt (yyvsp[-1].ttype);
		;}
    break;

  case 799:
#line 3541 "parse.y"
    { yyval.ttype = finish_goto_stmt (yyvsp[-1].ttype); ;}
    break;

  case 800:
#line 3543 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 801:
#line 3545 "parse.y"
    { error ("label must be followed by statement");
		  yyungetc ('}', 0);
		  yyval.ttype = NULL_TREE; ;}
    break;

  case 802:
#line 3549 "parse.y"
    { finish_stmt ();
		  yyval.ttype = NULL_TREE; ;}
    break;

  case 803:
#line 3552 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 804:
#line 3554 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 805:
#line 3556 "parse.y"
    { do_local_using_decl (yyvsp[0].ttype);
		  yyval.ttype = NULL_TREE; ;}
    break;

  case 806:
#line 3559 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 807:
#line 3564 "parse.y"
    { yyval.ttype = begin_function_try_block (); ;}
    break;

  case 808:
#line 3566 "parse.y"
    { finish_function_try_block (yyvsp[-1].ttype); ;}
    break;

  case 809:
#line 3568 "parse.y"
    { finish_function_handler_sequence (yyvsp[-3].ttype); ;}
    break;

  case 810:
#line 3573 "parse.y"
    { yyval.ttype = begin_try_block (); ;}
    break;

  case 811:
#line 3575 "parse.y"
    { finish_try_block (yyvsp[-1].ttype); ;}
    break;

  case 812:
#line 3577 "parse.y"
    { finish_handler_sequence (yyvsp[-3].ttype); ;}
    break;

  case 815:
#line 3584 "parse.y"
    { /* Generate a fake handler block to avoid later aborts. */
		  tree fake_handler = begin_handler ();
		  finish_handler_parms (NULL_TREE, fake_handler);
		  finish_handler (fake_handler);
		  yyval.ttype = fake_handler;

		  error ("must have at least one catch per try block");
		;}
    break;

  case 816:
#line 3596 "parse.y"
    { yyval.ttype = begin_handler (); ;}
    break;

  case 817:
#line 3598 "parse.y"
    { finish_handler_parms (yyvsp[0].ttype, yyvsp[-1].ttype); ;}
    break;

  case 818:
#line 3600 "parse.y"
    { finish_handler (yyvsp[-3].ttype); ;}
    break;

  case 821:
#line 3610 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 822:
#line 3626 "parse.y"
    {
		  check_for_new_type ("inside exception declarations", yyvsp[-1].ftype);
		  yyval.ttype = start_handler_parms (TREE_PURPOSE (yyvsp[-1].ftype.t),
					    TREE_VALUE (yyvsp[-1].ftype.t));
		;}
    break;

  case 823:
#line 3635 "parse.y"
    { finish_label_stmt (yyvsp[-1].ttype); ;}
    break;

  case 824:
#line 3637 "parse.y"
    { finish_label_stmt (yyvsp[-1].ttype); ;}
    break;

  case 825:
#line 3639 "parse.y"
    { finish_label_stmt (yyvsp[-1].ttype); ;}
    break;

  case 826:
#line 3641 "parse.y"
    { finish_label_stmt (yyvsp[-1].ttype); ;}
    break;

  case 827:
#line 3646 "parse.y"
    { finish_expr_stmt (yyvsp[-1].ttype); ;}
    break;

  case 829:
#line 3649 "parse.y"
    { if (pedantic)
		    pedwarn ("ISO C++ forbids compound statements inside for initializations");
		;}
    break;

  case 830:
#line 3658 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 832:
#line 3664 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 834:
#line 3667 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 835:
#line 3674 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 838:
#line 3681 "parse.y"
    { yyval.ttype = chainon (yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 839:
#line 3686 "parse.y"
    { yyval.ttype = build_tree_list (build_tree_list (NULL_TREE, yyvsp[-3].ttype), yyvsp[-1].ttype); ;}
    break;

  case 840:
#line 3688 "parse.y"
    { yyvsp[-5].ttype = build_string (IDENTIFIER_LENGTH (yyvsp[-5].ttype),
				     IDENTIFIER_POINTER (yyvsp[-5].ttype));
		  yyval.ttype = build_tree_list (build_tree_list (yyvsp[-5].ttype, yyvsp[-3].ttype), yyvsp[-1].ttype); ;}
    break;

  case 841:
#line 3695 "parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, NULL_TREE);;}
    break;

  case 842:
#line 3697 "parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-2].ttype); ;}
    break;

  case 843:
#line 3708 "parse.y"
    {
		  yyval.ttype = empty_parms();
		;}
    break;

  case 845:
#line 3713 "parse.y"
    { yyval.ttype = finish_parmlist (build_tree_list (NULL_TREE, yyvsp[0].ftype.t), 0);
		  check_for_new_type ("inside parameter list", yyvsp[0].ftype); ;}
    break;

  case 846:
#line 3721 "parse.y"
    { yyval.ttype = finish_parmlist (yyval.ttype, 0); ;}
    break;

  case 847:
#line 3723 "parse.y"
    { yyval.ttype = finish_parmlist (yyvsp[-1].ttype, 1); ;}
    break;

  case 848:
#line 3726 "parse.y"
    { yyval.ttype = finish_parmlist (yyvsp[-1].ttype, 1); ;}
    break;

  case 849:
#line 3728 "parse.y"
    { yyval.ttype = finish_parmlist (build_tree_list (NULL_TREE,
							 yyvsp[-1].ftype.t), 1); ;}
    break;

  case 850:
#line 3731 "parse.y"
    { yyval.ttype = finish_parmlist (NULL_TREE, 1); ;}
    break;

  case 851:
#line 3733 "parse.y"
    {
		  /* This helps us recover from really nasty
		     parse errors, for example, a missing right
		     parenthesis.  */
		  yyerror ("possibly missing ')'");
		  yyval.ttype = finish_parmlist (yyvsp[-1].ttype, 0);
		  yyungetc (':', 0);
		  yychar = ')';
		;}
    break;

  case 852:
#line 3743 "parse.y"
    {
		  /* This helps us recover from really nasty
		     parse errors, for example, a missing right
		     parenthesis.  */
		  yyerror ("possibly missing ')'");
		  yyval.ttype = finish_parmlist (build_tree_list (NULL_TREE,
							 yyvsp[-1].ftype.t), 0);
		  yyungetc (':', 0);
		  yychar = ')';
		;}
    break;

  case 853:
#line 3758 "parse.y"
    { maybe_snarf_defarg (); ;}
    break;

  case 854:
#line 3760 "parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 857:
#line 3771 "parse.y"
    { check_for_new_type ("in a parameter list", yyvsp[0].ftype);
		  yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ftype.t); ;}
    break;

  case 858:
#line 3774 "parse.y"
    { check_for_new_type ("in a parameter list", yyvsp[-1].ftype);
		  yyval.ttype = build_tree_list (yyvsp[0].ttype, yyvsp[-1].ftype.t); ;}
    break;

  case 859:
#line 3777 "parse.y"
    { check_for_new_type ("in a parameter list", yyvsp[0].ftype);
		  yyval.ttype = chainon (yyval.ttype, yyvsp[0].ftype.t); ;}
    break;

  case 860:
#line 3780 "parse.y"
    { yyval.ttype = chainon (yyval.ttype, build_tree_list (NULL_TREE, yyvsp[0].ttype)); ;}
    break;

  case 861:
#line 3782 "parse.y"
    { yyval.ttype = chainon (yyval.ttype, build_tree_list (yyvsp[0].ttype, yyvsp[-2].ttype)); ;}
    break;

  case 863:
#line 3788 "parse.y"
    { check_for_new_type ("in a parameter list", yyvsp[-1].ftype);
		  yyval.ttype = build_tree_list (NULL_TREE, yyvsp[-1].ftype.t); ;}
    break;

  case 864:
#line 3798 "parse.y"
    { yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag;
		  yyval.ftype.t = build_tree_list (yyvsp[-1].ftype.t, yyvsp[0].ttype); ;}
    break;

  case 865:
#line 3801 "parse.y"
    { yyval.ftype.t = build_tree_list (yyvsp[-1].ftype.t, yyvsp[0].ttype);
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;}
    break;

  case 866:
#line 3804 "parse.y"
    { yyval.ftype.t = build_tree_list (build_tree_list (NULL_TREE, yyvsp[-1].ftype.t),
					  yyvsp[0].ttype);
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;}
    break;

  case 867:
#line 3808 "parse.y"
    { yyval.ftype.t = build_tree_list (yyvsp[-1].ftype.t, yyvsp[0].ttype);
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag; ;}
    break;

  case 868:
#line 3811 "parse.y"
    { yyval.ftype.t = build_tree_list (yyvsp[0].ftype.t, NULL_TREE);
		  yyval.ftype.new_type_flag = yyvsp[0].ftype.new_type_flag; ;}
    break;

  case 869:
#line 3814 "parse.y"
    { yyval.ftype.t = build_tree_list (yyvsp[-1].ftype.t, yyvsp[0].ttype);
		  yyval.ftype.new_type_flag = 0; ;}
    break;

  case 870:
#line 3820 "parse.y"
    { yyval.ftype.t = build_tree_list (NULL_TREE, yyvsp[0].ftype.t);
		  yyval.ftype.new_type_flag = yyvsp[0].ftype.new_type_flag;  ;}
    break;

  case 871:
#line 3823 "parse.y"
    { yyval.ftype.t = build_tree_list (yyvsp[0].ttype, yyvsp[-1].ftype.t);
		  yyval.ftype.new_type_flag = yyvsp[-1].ftype.new_type_flag;  ;}
    break;

  case 874:
#line 3834 "parse.y"
    { see_typename (); ;}
    break;

  case 875:
#line 3839 "parse.y"
    {
		  error ("type specifier omitted for parameter");
		  yyval.ttype = build_tree_list (integer_type_node, NULL_TREE);
		;}
    break;

  case 876:
#line 3844 "parse.y"
    {
		  if (TREE_CODE (yyval.ttype) == SCOPE_REF)
		    {
		      if (TREE_CODE (TREE_OPERAND (yyval.ttype, 0)) == TEMPLATE_TYPE_PARM
			  || TREE_CODE (TREE_OPERAND (yyval.ttype, 0)) == BOUND_TEMPLATE_TEMPLATE_PARM)
			error ("`%E' is not a type, use `typename %E' to make it one", yyval.ttype, yyval.ttype);
		      else
			error ("no type `%D' in `%T'", TREE_OPERAND (yyval.ttype, 1), TREE_OPERAND (yyval.ttype, 0));
		    }
		  else
		    error ("type specifier omitted for parameter `%E'", yyval.ttype);
		  yyval.ttype = build_tree_list (integer_type_node, yyval.ttype);
		;}
    break;

  case 877:
#line 3861 "parse.y"
    {
                  error("'%D' is used as a type, but is not defined as a type.", yyvsp[-4].ttype);
                  yyvsp[-2].ttype = error_mark_node;
		;}
    break;

  case 878:
#line 3869 "parse.y"
    { ;}
    break;

  case 880:
#line 3875 "parse.y"
    { ;}
    break;

  case 882:
#line 3881 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 883:
#line 3883 "parse.y"
    { yyval.ttype = yyvsp[-1].ttype; ;}
    break;

  case 884:
#line 3885 "parse.y"
    { yyval.ttype = empty_except_spec; ;}
    break;

  case 885:
#line 3890 "parse.y"
    {
		  check_for_new_type ("exception specifier", yyvsp[0].ftype);
		  yyval.ttype = groktypename (yyvsp[0].ftype.t);
		;}
    break;

  case 886:
#line 3895 "parse.y"
    { yyval.ttype = error_mark_node; ;}
    break;

  case 887:
#line 3900 "parse.y"
    { yyval.ttype = add_exception_specifier (NULL_TREE, yyvsp[0].ttype, 1); ;}
    break;

  case 888:
#line 3902 "parse.y"
    { yyval.ttype = add_exception_specifier (yyvsp[-2].ttype, yyvsp[0].ttype, 1); ;}
    break;

  case 889:
#line 3907 "parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 890:
#line 3909 "parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 891:
#line 3911 "parse.y"
    { yyval.ttype = make_reference_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 892:
#line 3913 "parse.y"
    { tree arg = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype);
		  yyval.ttype = build_nt (SCOPE_REF, yyvsp[-2].ttype, arg);
		;}
    break;

  case 893:
#line 3920 "parse.y"
    {
	  saved_scopes = tree_cons (got_scope, got_object, saved_scopes);
	  TREE_LANG_FLAG_0 (saved_scopes) = looking_for_typename;
	  /* We look for conversion-type-id's in both the class and current
	     scopes, just as for ID in 'ptr->ID::'.  */
	  looking_for_typename = 1;
	  got_object = got_scope;
          got_scope = NULL_TREE;
	;}
    break;

  case 894:
#line 3932 "parse.y"
    { got_scope = TREE_PURPOSE (saved_scopes);
          got_object = TREE_VALUE (saved_scopes);
	  looking_for_typename = TREE_LANG_FLAG_0 (saved_scopes);
          saved_scopes = TREE_CHAIN (saved_scopes);
	  yyval.ttype = got_scope;
	;}
    break;

  case 895:
#line 3942 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (MULT_EXPR)); ;}
    break;

  case 896:
#line 3944 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (TRUNC_DIV_EXPR)); ;}
    break;

  case 897:
#line 3946 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (TRUNC_MOD_EXPR)); ;}
    break;

  case 898:
#line 3948 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (PLUS_EXPR)); ;}
    break;

  case 899:
#line 3950 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (MINUS_EXPR)); ;}
    break;

  case 900:
#line 3952 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (BIT_AND_EXPR)); ;}
    break;

  case 901:
#line 3954 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (BIT_IOR_EXPR)); ;}
    break;

  case 902:
#line 3956 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (BIT_XOR_EXPR)); ;}
    break;

  case 903:
#line 3958 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (BIT_NOT_EXPR)); ;}
    break;

  case 904:
#line 3960 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (COMPOUND_EXPR)); ;}
    break;

  case 905:
#line 3962 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (yyvsp[-1].code)); ;}
    break;

  case 906:
#line 3964 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (LT_EXPR)); ;}
    break;

  case 907:
#line 3966 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (GT_EXPR)); ;}
    break;

  case 908:
#line 3968 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (yyvsp[-1].code)); ;}
    break;

  case 909:
#line 3970 "parse.y"
    { yyval.ttype = frob_opname (ansi_assopname (yyvsp[-1].code)); ;}
    break;

  case 910:
#line 3972 "parse.y"
    { yyval.ttype = frob_opname (ansi_assopname (NOP_EXPR)); ;}
    break;

  case 911:
#line 3974 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (yyvsp[-1].code)); ;}
    break;

  case 912:
#line 3976 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (yyvsp[-1].code)); ;}
    break;

  case 913:
#line 3978 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (POSTINCREMENT_EXPR)); ;}
    break;

  case 914:
#line 3980 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (PREDECREMENT_EXPR)); ;}
    break;

  case 915:
#line 3982 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (TRUTH_ANDIF_EXPR)); ;}
    break;

  case 916:
#line 3984 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (TRUTH_ORIF_EXPR)); ;}
    break;

  case 917:
#line 3986 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (TRUTH_NOT_EXPR)); ;}
    break;

  case 918:
#line 3988 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (COND_EXPR)); ;}
    break;

  case 919:
#line 3990 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (yyvsp[-1].code)); ;}
    break;

  case 920:
#line 3992 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (COMPONENT_REF)); ;}
    break;

  case 921:
#line 3994 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (MEMBER_REF)); ;}
    break;

  case 922:
#line 3996 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (CALL_EXPR)); ;}
    break;

  case 923:
#line 3998 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (ARRAY_REF)); ;}
    break;

  case 924:
#line 4000 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (NEW_EXPR)); ;}
    break;

  case 925:
#line 4002 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (DELETE_EXPR)); ;}
    break;

  case 926:
#line 4004 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (VEC_NEW_EXPR)); ;}
    break;

  case 927:
#line 4006 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (VEC_DELETE_EXPR)); ;}
    break;

  case 928:
#line 4008 "parse.y"
    { yyval.ttype = frob_opname (grokoptypename (yyvsp[-2].ftype.t, yyvsp[-1].ttype, yyvsp[0].ttype)); ;}
    break;

  case 929:
#line 4010 "parse.y"
    { yyval.ttype = frob_opname (ansi_opname (ERROR_MARK)); ;}
    break;

  case 930:
#line 4017 "parse.y"
    { if (yychar == YYEMPTY)
		    yychar = YYLEX;
		  yyval.itype = lineno; ;}
    break;


    }

/* Line 991 of yacc.c.  */
#line 10217 "p19357.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("syntax error, unexpected ") + 1;
	  yysize += yystrlen (yytname[yytype]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
	  /* Pop the error token.  */
          YYPOPSTACK;
	  /* Pop the rest of the stack.  */
	  while (yyss < yyssp)
	    {
	      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
	      yydestruct (yystos[*yyssp], yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab2;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
yyerrlab1:

  /* Suppress GCC warning that yyerrlab1 is unused when no action
     invokes YYERROR.  */
#if defined (__GNUC_MINOR__) && 2093 <= (__GNUC__ * 1000 + __GNUC_MINOR__) \
    && !defined __cplusplus
  __attribute__ ((__unused__))
#endif


  goto yyerrlab2;


/*---------------------------------------------------------------.
| yyerrlab2 -- pop states until the error token can be shifted.  |
`---------------------------------------------------------------*/
yyerrlab2:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
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

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      yyvsp--;
      yystate = *--yyssp;

      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


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

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 4021 "parse.y"


#ifdef SPEW_DEBUG
const char *
debug_yytranslate (value)
    int value;
{
  return yytname[YYTRANSLATE (value)];
}
#endif

/* Free malloced parser stacks if necessary.  */

void
free_parser_stacks ()
{
  if (malloced_yyss)
    {
      free (malloced_yyss);
      free (malloced_yyvs);
    }
}

/* Return the value corresponding to TOKEN in the global scope.  */

static tree
parse_scoped_id (token)
     tree token;
{
  cxx_binding binding;
 
  cxx_binding_clear (&binding);
  if (!qualified_lookup_using_namespace (token, global_namespace, &binding, 0))
    binding.value = NULL;
  if (yychar == YYEMPTY)
    yychar = yylex();

  return do_scoped_id (token, binding.value);
}

/* AGGR may be either a type node (like class_type_node) or a
   TREE_LIST whose TREE_PURPOSE is a list of attributes and whose
   TREE_VALUE is a type node.  Set *TAG_KIND and *ATTRIBUTES to
   represent the information encoded.  */

static void
parse_split_aggr (tree aggr, enum tag_types *tag_kind, tree *attributes)
{
  if (TREE_CODE (aggr) == TREE_LIST) 
    {
      *attributes = TREE_PURPOSE (aggr);
      aggr = TREE_VALUE (aggr);
    }
  else
    *attributes = NULL_TREE;
  *tag_kind = (enum tag_types) tree_low_cst (aggr, 1);
}

/* Like xref_tag, except that the AGGR may be either a type node (like
   class_type_node) or a TREE_LIST whose TREE_PURPOSE is a list of
   attributes and whose TREE_VALUE is a type node.  */

static tree
parse_xref_tag (tree aggr, tree name, int globalize)
{
  tree attributes;
  enum tag_types tag_kind;
  parse_split_aggr (aggr, &tag_kind, &attributes);
  return xref_tag (tag_kind, name, attributes, globalize);
}

/* Like handle_class_head, but AGGR may be as for parse_xref_tag.  */

static tree
parse_handle_class_head (tree aggr, tree scope, tree id, 
			 int defn_p, int *new_type_p)
{
  tree attributes;
  enum tag_types tag_kind;
  parse_split_aggr (aggr, &tag_kind, &attributes);
  return handle_class_head (tag_kind, scope, id, attributes, 
			    defn_p, new_type_p);
}

/* Like do_decl_instantiation, but the declarator has not yet been
   parsed.  */

static void
parse_decl_instantiation (tree declspecs, tree declarator, tree storage)
{
  tree decl = grokdeclarator (declarator, declspecs, NORMAL, 0, NULL);
  do_decl_instantiation (decl, storage);
}

/* Like begin_function_definition, but SPECS_ATTRS is a combined list
   containing both a decl-specifier-seq and attributes.  */

static int
parse_begin_function_definition (tree specs_attrs, tree declarator)
{
  tree specs;
  tree attrs;
  
  split_specs_attrs (specs_attrs, &specs, &attrs);
  return begin_function_definition (specs, attrs, declarator);
}

/* Like finish_call_expr, but the name for FN has not yet been
   resolved.  */

static tree
parse_finish_call_expr (tree fn, tree args, int koenig)
{
  bool disallow_virtual;

  if (TREE_CODE (fn) == OFFSET_REF)
    return build_offset_ref_call_from_tree (fn, args);

  if (TREE_CODE (fn) == SCOPE_REF)
    {
      tree scope = TREE_OPERAND (fn, 0);
      tree name = TREE_OPERAND (fn, 1);

      if (scope == error_mark_node || name == error_mark_node)
	return error_mark_node;
      if (!processing_template_decl)
	fn = resolve_scoped_fn_name (scope, name);
      disallow_virtual = true;
    }
  else
    disallow_virtual = false;

  if (koenig && TREE_CODE (fn) == IDENTIFIER_NODE)
    {
      tree f;
      
      /* Do the Koenig lookup.  */
      fn = do_identifier (fn, 2, args);
      /* If name lookup didn't find any matching declarations, we've
	 got an unbound identifier.  */
      if (TREE_CODE (fn) == IDENTIFIER_NODE)
	{
	  /* For some reason, do_identifier does not resolve
	     conversion operator names if the only matches would be
	     template conversion operators.  So, we do it here.  */
	  if (IDENTIFIER_TYPENAME_P (fn) && current_class_type)
	    {
	      f = lookup_member (current_class_type, fn,
				 /*protect=*/1, /*want_type=*/0);
	      if (f)
		return finish_call_expr (f, args,
					 /*disallow_virtual=*/false);
	    }
	  /* If the name still could not be resolved, then the program
	     is ill-formed.  */
	  if (TREE_CODE (fn) == IDENTIFIER_NODE)
	    {
	      unqualified_name_lookup_error (fn);
	      return error_mark_node;
	    }
	}
      else if (TREE_CODE (fn) == FUNCTION_DECL
	       || DECL_FUNCTION_TEMPLATE_P (fn)
	       || TREE_CODE (fn) == OVERLOAD)
	{
	  tree scope = DECL_CONTEXT (get_first_fn (fn));
	  if (scope && TYPE_P (scope))
	    {
	      tree access_scope;

	      if (DERIVED_FROM_P (scope, current_class_type)
		  && current_class_ref)
		{
		  fn = build_baselink (lookup_base (current_class_type,
						    scope,
						    ba_any,
						    NULL),
				       TYPE_BINFO (current_class_type),
				       fn,
				       /*optype=*/NULL_TREE);
		  return finish_object_call_expr (fn,
						  current_class_ref,
						  args);
		}


	      access_scope = current_class_type;
	      while (!DERIVED_FROM_P (scope, access_scope))
		{
		  access_scope = TYPE_CONTEXT (access_scope);
		  while (DECL_P (access_scope))
		    access_scope = DECL_CONTEXT (access_scope);
		}
	      
	      fn = build_baselink (NULL_TREE,
				   TYPE_BINFO (access_scope),
				   fn,
				   /*optype=*/NULL_TREE);
	    }
	}
    }

  if (TREE_CODE (fn) == COMPONENT_REF)
    /* If the parser sees `(x->y)(bar)' we get here because the
       parentheses confuse the parser.  Treat this like 
       `x->y(bar)'.  */
    return finish_object_call_expr (TREE_OPERAND (fn, 1),
				    TREE_OPERAND (fn, 0),
				    args);

  if (processing_template_decl)
    return build_nt (CALL_EXPR, fn, args, NULL_TREE);

  return build_call_from_tree (fn, args, disallow_virtual);
}

#include "gt-cp-parse.h"

