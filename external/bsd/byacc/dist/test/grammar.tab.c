/*	$NetBSD: grammar.tab.c,v 1.1.1.1 2009/10/29 00:46:53 christos Exp $	*/

#ifndef lint
static const char yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93";
#endif

#include <stdlib.h>
#include <string.h>

#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9

#define YYEMPTY        (-1)
#define yyclearin      (yychar = YYEMPTY)
#define yyerrok        (yyerrflag = 0)
#define YYRECOVERING() (yyerrflag != 0)

/* compatibility with bison */
#ifdef YYPARSE_PARAM
/* compatibility with FreeBSD */
#ifdef YYPARSE_PARAM_TYPE
#define YYPARSE_DECL() yyparse(YYPARSE_PARAM_TYPE YYPARSE_PARAM)
#else
#define YYPARSE_DECL() yyparse(void *YYPARSE_PARAM)
#endif
#else
#define YYPARSE_DECL() yyparse(void)
#endif /* YYPARSE_PARAM */

extern int YYPARSE_DECL();

static int yygrowstack(void);
#define yyparse    grammar_parse
#define yylex      grammar_lex
#define yyerror    grammar_error
#define yychar     grammar_char
#define yyval      grammar_val
#define yylval     grammar_lval
#define yydebug    grammar_debug
#define yynerrs    grammar_nerrs
#define yyerrflag  grammar_errflag
#define yyss       grammar_ss
#define yyssp      grammar_ssp
#define yyvs       grammar_vs
#define yyvsp      grammar_vsp
#define yylhs      grammar_lhs
#define yylen      grammar_len
#define yydefred   grammar_defred
#define yydgoto    grammar_dgoto
#define yysindex   grammar_sindex
#define yyrindex   grammar_rindex
#define yygindex   grammar_gindex
#define yytable    grammar_table
#define yycheck    grammar_check
#define yyname     grammar_name
#define yyrule     grammar_rule
#define YYPREFIX "grammar_"
#line 69 "grammar.y"
#include <stdio.h>
#include <ctype.h>
#include "cproto.h"
#include "symbol.h"
#include "semantic.h"

#define YYMAXDEPTH 150

extern	int	yylex (void);

/* declaration specifier attributes for the typedef statement currently being
 * scanned
 */
static int cur_decl_spec_flags;

/* pointer to parameter list for the current function definition */
static ParameterList *func_params;

/* A parser semantic action sets this pointer to the current declarator in
 * a function parameter declaration in order to catch any comments following
 * the parameter declaration on the same line.  If the lexer scans a comment
 * and <cur_declarator> is not NULL, then the comment is attached to the
 * declarator.  To ignore subsequent comments, the lexer sets this to NULL
 * after scanning a comment or end of line.
 */
static Declarator *cur_declarator;

/* temporary string buffer */
static char buf[MAX_TEXT_SIZE];

/* table of typedef names */
static SymbolTable *typedef_names;

/* table of define names */
static SymbolTable *define_names;

/* table of type qualifiers */
static SymbolTable *type_qualifiers;

/* information about the current input file */
typedef struct {
    char *base_name;		/* base input file name */
    char *file_name;		/* current file name */
    FILE *file; 		/* input file */
    unsigned line_num;		/* current line number in input file */
    FILE *tmp_file;		/* temporary file */
    long begin_comment; 	/* tmp file offset after last written ) or ; */
    long end_comment;		/* tmp file offset after last comment */
    boolean convert;		/* if TRUE, convert function definitions */
    boolean changed;		/* TRUE if conversion done in this file */
} IncludeStack;

static IncludeStack *cur_file;	/* current input file */

#include "yyerror.c"

static int haveAnsiParam (void);


/* Flags to enable us to find if a procedure returns a value.
 */
static int return_val,	/* nonzero on BRACES iff return-expression found */
	   returned_at;	/* marker for token-number to set 'return_val' */

#if OPT_LINTLIBRARY
static char *dft_decl_spec (void);

static char *
dft_decl_spec (void)
{
    return (lintLibrary() && !return_val) ? "void" : "int";
}

#else
#define dft_decl_spec() "int"
#endif

static int
haveAnsiParam (void)
{
    Parameter *p;
    if (func_params != 0) {
	for (p = func_params->first; p != 0; p = p->next) {
	    if (p->declarator->func_def == FUNC_ANSI) {
		return TRUE;
	    }
	}
    }
    return FALSE;
}
#line 149 "grammar.tab.c"
#define T_IDENTIFIER 257
#define T_TYPEDEF_NAME 258
#define T_DEFINE_NAME 259
#define T_AUTO 260
#define T_EXTERN 261
#define T_REGISTER 262
#define T_STATIC 263
#define T_TYPEDEF 264
#define T_INLINE 265
#define T_EXTENSION 266
#define T_CHAR 267
#define T_DOUBLE 268
#define T_FLOAT 269
#define T_INT 270
#define T_VOID 271
#define T_LONG 272
#define T_SHORT 273
#define T_SIGNED 274
#define T_UNSIGNED 275
#define T_ENUM 276
#define T_STRUCT 277
#define T_UNION 278
#define T_Bool 279
#define T_Complex 280
#define T_Imaginary 281
#define T_TYPE_QUALIFIER 282
#define T_BRACKETS 283
#define T_LBRACE 284
#define T_MATCHRBRACE 285
#define T_ELLIPSIS 286
#define T_INITIALIZER 287
#define T_STRING_LITERAL 288
#define T_ASM 289
#define T_ASMARG 290
#define T_VA_DCL 291
#define YYERRCODE 256
static const short grammar_lhs[] = {                     -1,
    0,    0,   26,   26,   27,   27,   27,   27,   27,   27,
   27,   31,   30,   30,   28,   28,   34,   28,   32,   32,
   33,   33,   35,   35,   37,   38,   29,   39,   29,   36,
   36,   36,   40,   40,    1,    1,    2,    2,    2,    3,
    3,    3,    3,    3,    3,    4,    4,    4,    4,    4,
    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
    5,    5,    6,    6,    6,   19,   19,    8,    8,    9,
   41,    9,    7,    7,    7,   25,   23,   23,   10,   10,
   11,   11,   11,   11,   11,   20,   20,   21,   21,   22,
   22,   14,   14,   15,   15,   16,   16,   16,   17,   17,
   18,   18,   24,   24,   12,   12,   12,   13,   13,   13,
   13,   13,   13,   13,
};
static const short grammar_len[] = {                      2,
    0,    1,    1,    2,    1,    1,    1,    1,    3,    2,
    2,    2,    3,    3,    2,    3,    0,    5,    2,    1,
    0,    1,    1,    3,    0,    0,    7,    0,    5,    0,
    1,    1,    1,    2,    1,    2,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    3,    2,    2,    1,    1,    1,    3,    1,
    0,    4,    3,    2,    2,    1,    1,    1,    2,    1,
    1,    3,    2,    4,    4,    2,    3,    0,    1,    1,
    2,    1,    3,    1,    3,    2,    2,    1,    0,    1,
    1,    3,    1,    2,    1,    2,    1,    3,    2,    1,
    4,    3,    3,    2,
};
static const short grammar_defred[] = {                   0,
    0,    0,    0,    0,   77,    0,   62,   40,    0,   42,
   43,   20,   44,    0,   46,   47,   48,   49,   54,   50,
   51,   52,   53,   76,   66,   67,   55,   56,   57,   61,
    0,    7,    0,    0,   35,   37,   38,   39,   59,   60,
   28,    0,    0,    0,  103,   81,    0,    0,    3,    5,
    6,    8,    0,   10,   11,   78,    0,   90,    0,    0,
  104,    0,   19,    0,   41,   45,   15,   36,    0,   68,
    0,    0,    0,   83,    0,    0,   64,    0,    0,   74,
    4,   58,    0,   82,   87,   91,    0,   14,   13,    9,
   16,    0,   71,    0,   31,   33,    0,    0,    0,    0,
    0,   94,    0,    0,  101,   12,   63,   73,    0,    0,
   69,    0,    0,    0,   34,    0,  110,   96,   97,    0,
    0,   84,    0,   85,    0,   23,    0,    0,   72,   26,
   29,  114,    0,    0,    0,  109,    0,   93,   95,  102,
   18,    0,    0,  108,  113,  112,    0,   24,   27,  111,
};
static const short grammar_dgoto[] = {                   33,
   87,   35,   36,   37,   38,   39,   40,   69,   70,   41,
   42,  119,  120,  100,  101,  102,  103,  104,   43,   44,
   59,   60,   45,   46,   47,   48,   49,   50,   51,   52,
   77,   53,  127,  109,  128,   97,   94,  143,   72,   98,
  112,
};
static const short grammar_sindex[] = {                  -2,
   -3,   27, -239, -177,    0,    0,    0,    0, -274,    0,
    0,    0,    0, -246,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
 -266,    0,    0,  455,    0,    0,    0,    0,    0,    0,
    0,  -35, -245,  128,    0,    0, -245,   -2,    0,    0,
    0,    0,  642,    0,    0,    0,  -15,    0,  -12, -239,
    0,  590,    0,  -27,    0,    0,    0,    0,  -10,    0,
  -11,  534,  -72,    0, -237, -232,    0,  -35, -232,    0,
    0,    0,  642,    0,    0,    0,  455,    0,    0,    0,
    0,   27,    0,  534,    0,    0, -222,  617,  209,   34,
   39,    0,   44,   42,    0,    0,    0,    0,   27,  -11,
    0, -200, -196, -195,    0,  174,    0,    0,    0,  -33,
  243,    0,  561,    0, -177,    0,   33,   49,    0,    0,
    0,    0,   53,   55,  417,    0,  -33,    0,    0,    0,
    0,   27, -188,    0,    0,    0,   57,    0,    0,    0,
};
static const short grammar_rindex[] = {                  99,
    0,    0,  275,    0,    0,  -38,    0,    0,  481,    0,
    0,    0,    0,  509,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   30,    0,    0,    0,    0,    0,  101,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  343,  309,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   98, -182,   62,    0,    0,  133,    0,   64,  379,    0,
    0,    0,   -5,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0, -182,    0,    0,    0, -180,  -19,    0,
   65,    0,    0,   68,    0,    0,    0,    0,   51,    9,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  -13,
   19,    0,    0,    0,    0,    0,    0,   52,    0,    0,
    0,    0,    0,    0,    0,    0,   35,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
};
static const short grammar_gindex[] = {                   0,
   11,  -17,    0,    0,   13,    0,    0,    0,   20,    8,
  -43,   -1,   -8,  -89,    0,   -9,    0,    0,    0,  -44,
    0,    0,    4,    0,    0,    0,   70,  -53,    0,    0,
  -18,    0,    0,    0,    0,   22,    0,    0,    0,    0,
    0,
};
#define YYTABLESIZE 924
static const short grammar_table[] = {                   58,
   78,   58,   58,   58,   73,   58,  135,   61,   88,   57,
   34,    5,   56,   62,   85,   58,   68,   63,   96,    7,
   58,   98,   78,   64,   98,   84,  134,  107,   80,    3,
  107,   90,   17,   92,   17,    4,   17,    2,   75,    3,
   96,   71,   30,   89,  115,  147,   76,  106,   91,   93,
   79,   75,   70,   17,  121,   55,   32,  107,   34,  105,
  108,  114,  105,   83,    4,   68,    2,   70,    3,   68,
   80,  121,   86,   80,  122,  106,  105,   78,  106,    5,
   56,   68,  123,   99,  124,  125,  129,  130,   80,  131,
   80,  141,  142,  144,  110,  145,  149,  150,    1,  110,
    2,   30,   99,   32,   79,   92,  118,   79,  100,   21,
   22,  111,  137,  139,  133,  113,  126,   81,    0,    0,
    0,    0,   79,   57,   79,    0,   99,    0,  140,    0,
    0,    0,    0,   99,    0,    0,    0,    0,    0,    0,
    0,   70,    0,    0,    0,   99,    0,    0,    0,  148,
    0,    0,    0,    0,    0,    0,   70,    0,    0,    0,
    0,    0,    0,    0,    0,    4,    0,    2,    0,    0,
   65,    0,   65,   65,   65,    0,   65,    0,    0,    0,
    0,    0,    0,    0,    5,    6,    7,    8,   65,   10,
   11,   65,   13,   66,   15,   16,   17,   18,   19,   20,
   21,   22,   23,   24,   25,   26,   27,   28,   29,   30,
    0,    4,    0,  116,  132,    3,    0,    0,   58,   58,
   58,   58,   58,   58,   58,   78,   58,   58,   58,   58,
   58,   58,   58,   58,   58,   58,   58,   58,   58,   58,
   58,   58,   58,   58,   58,   78,    4,   74,  116,  136,
    3,   17,   78,    1,    5,    6,    7,    8,    9,   10,
   11,   12,   13,   14,   15,   16,   17,   18,   19,   20,
   21,   22,   23,   24,   25,   26,   27,   28,   29,   30,
    4,   54,  116,    5,   56,    0,   31,   80,   80,   80,
   80,   80,   80,   80,   80,   80,   80,   80,   80,   80,
   80,   80,   80,   80,   80,   80,   80,   80,   80,   80,
   80,   80,   88,   80,   88,   88,   88,    0,   88,    0,
   80,   79,   79,   79,   79,   79,   79,   79,   79,   79,
   79,   79,   79,   79,   79,   79,   79,   79,   79,   79,
   79,   79,   79,   79,   79,   79,   89,   79,   89,   89,
   89,    0,   89,    0,   79,   25,   25,   25,   25,   25,
   25,   25,   25,   25,   25,   25,   25,   25,   25,   25,
   25,   25,   25,   25,   25,   25,   25,   25,   25,   25,
   86,   25,   86,   86,    5,   56,   86,    0,   25,   65,
   65,   65,   65,   65,   65,   65,    0,   65,   65,   65,
   65,   65,   65,   65,   65,   65,   65,   65,   65,   65,
   65,   65,   65,   65,   65,   65,   75,    0,   75,   75,
   75,    0,   75,    0,    0,    0,    0,    0,    0,    0,
    5,    6,    7,    8,   65,   10,   11,   75,   13,   66,
   15,   16,   17,   18,   19,   20,   21,   22,   23,   24,
   25,   26,   27,   28,   29,   30,  117,  146,    0,    0,
    0,    0,    0,    0,    0,    5,    6,    7,    8,   65,
   10,   11,    0,   13,   66,   15,   16,   17,   18,   19,
   20,   21,   22,   23,   24,   25,   26,   27,   28,   29,
   30,  117,    4,    0,    2,    0,    3,    0,    0,    5,
   56,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   67,    0,    0,    0,    0,   41,    0,
   41,    0,   41,    0,    0,  117,    0,    0,    0,    0,
    0,   88,   88,    0,    0,    0,    0,    0,    0,   41,
    0,    0,    0,    0,    0,    0,   45,    0,   45,    0,
   45,    0,    0,    0,    0,    0,    0,   88,    0,    0,
    0,    0,    0,    0,    0,   89,   89,   45,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   89,    0,    0,    0,    0,    0,    0,    0,   86,
   86,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   86,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   75,   75,   75,   75,   75,
   75,   75,    0,   75,   75,   75,   75,   75,   75,   75,
   75,   75,   75,   75,   75,   75,   75,   75,   75,   75,
   75,   75,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   82,    7,    8,   65,   10,   11,
    0,   13,   66,   15,   16,   17,   18,   19,   20,   21,
   22,   23,   24,   25,   26,   27,   28,   29,   30,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    5,    6,    7,    8,   65,   10,   11,    0,   13,
   66,   15,   16,   17,   18,   19,   20,   21,   22,   23,
   24,   25,   26,   27,   28,   29,   30,   41,   41,   41,
   41,   41,   41,   41,    0,   41,   41,   41,   41,   41,
   41,   41,   41,   41,   41,   41,   41,   41,   41,   41,
   41,   41,   41,    0,    0,   45,   45,   45,   45,   45,
   45,   45,    0,   45,   45,   45,   45,   45,   45,   45,
   45,   45,   45,   45,   45,   45,   45,   45,   45,   45,
   45,   82,    7,    8,   65,   10,   11,   12,   13,   14,
   15,   16,   17,   18,   19,   20,   21,   22,   23,   24,
   25,   26,   27,   28,   29,   30,    0,    0,   82,    7,
    8,   65,   10,   11,   95,   13,   66,   15,   16,   17,
   18,   19,   20,   21,   22,   23,   24,   25,   26,   27,
   28,   29,   30,    0,    0,    0,  138,   82,    7,    8,
   65,   10,   11,   12,   13,   14,   15,   16,   17,   18,
   19,   20,   21,   22,   23,   24,   25,   26,   27,   28,
   29,   30,    0,   75,   82,    7,    8,   65,   10,   11,
   12,   13,   14,   15,   16,   17,   18,   19,   20,   21,
   22,   23,   24,   25,   26,   27,   28,   29,   30,   82,
    7,    8,   65,   10,   11,    0,   13,   66,   15,   16,
   17,   18,   19,   20,   21,   22,   23,   24,   25,   26,
   27,   28,   29,   30,
};
static const short grammar_check[] = {                   38,
   44,   40,   41,   42,   40,   44,   40,    4,   62,    2,
    0,  257,  258,  288,   59,    3,   34,  264,   72,  259,
   59,   41,   61,  290,   44,   41,  116,   41,   47,   42,
   44,   59,   38,   44,   40,   38,   42,   40,  284,   42,
   94,   34,  282,   62,   98,  135,   43,  285,   59,   61,
   47,  284,   44,   59,   99,   59,   59,   76,   48,   41,
   79,  284,   44,   53,   38,   83,   40,   59,   42,   87,
   41,  116,   60,   44,   41,   41,   73,  121,   44,  257,
  258,   99,   44,   73,   41,   44,  287,  284,   59,  285,
   61,   59,   44,   41,   87,   41,  285,   41,    0,   92,
    0,  284,   41,  284,   41,   41,   99,   44,   41,   59,
   59,   92,  121,  123,  116,   94,  109,   48,   -1,   -1,
   -1,   -1,   59,  116,   61,   -1,  116,   -1,  125,   -1,
   -1,   -1,   -1,  123,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   44,   -1,   -1,   -1,  135,   -1,   -1,   -1,  142,
   -1,   -1,   -1,   -1,   -1,   -1,   59,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   38,   -1,   40,   -1,   -1,
   38,   -1,   40,   41,   42,   -1,   44,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  257,  258,  259,  260,  261,  262,
  263,   59,  265,  266,  267,  268,  269,  270,  271,  272,
  273,  274,  275,  276,  277,  278,  279,  280,  281,  282,
   -1,   38,   -1,   40,   41,   42,   -1,   -1,  257,  258,
  259,  260,  261,  262,  263,  264,  265,  266,  267,  268,
  269,  270,  271,  272,  273,  274,  275,  276,  277,  278,
  279,  280,  281,  282,  283,  284,   38,  283,   40,  283,
   42,  257,  291,  256,  257,  258,  259,  260,  261,  262,
  263,  264,  265,  266,  267,  268,  269,  270,  271,  272,
  273,  274,  275,  276,  277,  278,  279,  280,  281,  282,
   38,  285,   40,  257,  258,   -1,  289,  258,  259,  260,
  261,  262,  263,  264,  265,  266,  267,  268,  269,  270,
  271,  272,  273,  274,  275,  276,  277,  278,  279,  280,
  281,  282,   38,  284,   40,   41,   42,   -1,   44,   -1,
  291,  258,  259,  260,  261,  262,  263,  264,  265,  266,
  267,  268,  269,  270,  271,  272,  273,  274,  275,  276,
  277,  278,  279,  280,  281,  282,   38,  284,   40,   41,
   42,   -1,   44,   -1,  291,  258,  259,  260,  261,  262,
  263,  264,  265,  266,  267,  268,  269,  270,  271,  272,
  273,  274,  275,  276,  277,  278,  279,  280,  281,  282,
   38,  284,   40,   41,  257,  258,   44,   -1,  291,  257,
  258,  259,  260,  261,  262,  263,   -1,  265,  266,  267,
  268,  269,  270,  271,  272,  273,  274,  275,  276,  277,
  278,  279,  280,  281,  282,  283,   38,   -1,   40,   41,
   42,   -1,   44,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
  257,  258,  259,  260,  261,  262,  263,   59,  265,  266,
  267,  268,  269,  270,  271,  272,  273,  274,  275,  276,
  277,  278,  279,  280,  281,  282,  283,   41,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,  257,  258,  259,  260,  261,
  262,  263,   -1,  265,  266,  267,  268,  269,  270,  271,
  272,  273,  274,  275,  276,  277,  278,  279,  280,  281,
  282,  283,   38,   -1,   40,   -1,   42,   -1,   -1,  257,
  258,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   59,   -1,   -1,   -1,   -1,   38,   -1,
   40,   -1,   42,   -1,   -1,  283,   -1,   -1,   -1,   -1,
   -1,  257,  258,   -1,   -1,   -1,   -1,   -1,   -1,   59,
   -1,   -1,   -1,   -1,   -1,   -1,   38,   -1,   40,   -1,
   42,   -1,   -1,   -1,   -1,   -1,   -1,  283,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,  257,  258,   59,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  283,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  257,
  258,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,  283,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,  257,  258,  259,  260,  261,
  262,  263,   -1,  265,  266,  267,  268,  269,  270,  271,
  272,  273,  274,  275,  276,  277,  278,  279,  280,  281,
  282,  283,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  258,  259,  260,  261,  262,  263,
   -1,  265,  266,  267,  268,  269,  270,  271,  272,  273,
  274,  275,  276,  277,  278,  279,  280,  281,  282,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  257,  258,  259,  260,  261,  262,  263,   -1,  265,
  266,  267,  268,  269,  270,  271,  272,  273,  274,  275,
  276,  277,  278,  279,  280,  281,  282,  257,  258,  259,
  260,  261,  262,  263,   -1,  265,  266,  267,  268,  269,
  270,  271,  272,  273,  274,  275,  276,  277,  278,  279,
  280,  281,  282,   -1,   -1,  257,  258,  259,  260,  261,
  262,  263,   -1,  265,  266,  267,  268,  269,  270,  271,
  272,  273,  274,  275,  276,  277,  278,  279,  280,  281,
  282,  258,  259,  260,  261,  262,  263,  264,  265,  266,
  267,  268,  269,  270,  271,  272,  273,  274,  275,  276,
  277,  278,  279,  280,  281,  282,   -1,   -1,  258,  259,
  260,  261,  262,  263,  291,  265,  266,  267,  268,  269,
  270,  271,  272,  273,  274,  275,  276,  277,  278,  279,
  280,  281,  282,   -1,   -1,   -1,  286,  258,  259,  260,
  261,  262,  263,  264,  265,  266,  267,  268,  269,  270,
  271,  272,  273,  274,  275,  276,  277,  278,  279,  280,
  281,  282,   -1,  284,  258,  259,  260,  261,  262,  263,
  264,  265,  266,  267,  268,  269,  270,  271,  272,  273,
  274,  275,  276,  277,  278,  279,  280,  281,  282,  258,
  259,  260,  261,  262,  263,   -1,  265,  266,  267,  268,
  269,  270,  271,  272,  273,  274,  275,  276,  277,  278,
  279,  280,  281,  282,
};
#define YYFINAL 33
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 291
#if YYDEBUG
static const char *grammar_name[] = {

"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,"'&'",0,"'('","')'","'*'",0,"','",0,0,0,0,0,0,0,0,0,0,0,0,0,0,"';'",0,
"'='",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
"T_IDENTIFIER","T_TYPEDEF_NAME","T_DEFINE_NAME","T_AUTO","T_EXTERN",
"T_REGISTER","T_STATIC","T_TYPEDEF","T_INLINE","T_EXTENSION","T_CHAR",
"T_DOUBLE","T_FLOAT","T_INT","T_VOID","T_LONG","T_SHORT","T_SIGNED",
"T_UNSIGNED","T_ENUM","T_STRUCT","T_UNION","T_Bool","T_Complex","T_Imaginary",
"T_TYPE_QUALIFIER","T_BRACKETS","T_LBRACE","T_MATCHRBRACE","T_ELLIPSIS",
"T_INITIALIZER","T_STRING_LITERAL","T_ASM","T_ASMARG","T_VA_DCL",
};
static const char *grammar_rule[] = {
"$accept : program",
"program :",
"program : translation_unit",
"translation_unit : external_declaration",
"translation_unit : translation_unit external_declaration",
"external_declaration : declaration",
"external_declaration : function_definition",
"external_declaration : ';'",
"external_declaration : linkage_specification",
"external_declaration : T_ASM T_ASMARG ';'",
"external_declaration : error T_MATCHRBRACE",
"external_declaration : error ';'",
"braces : T_LBRACE T_MATCHRBRACE",
"linkage_specification : T_EXTERN T_STRING_LITERAL braces",
"linkage_specification : T_EXTERN T_STRING_LITERAL declaration",
"declaration : decl_specifiers ';'",
"declaration : decl_specifiers init_declarator_list ';'",
"$$1 :",
"declaration : any_typedef decl_specifiers $$1 opt_declarator_list ';'",
"any_typedef : T_EXTENSION T_TYPEDEF",
"any_typedef : T_TYPEDEF",
"opt_declarator_list :",
"opt_declarator_list : declarator_list",
"declarator_list : declarator",
"declarator_list : declarator_list ',' declarator",
"$$2 :",
"$$3 :",
"function_definition : decl_specifiers declarator $$2 opt_declaration_list T_LBRACE $$3 T_MATCHRBRACE",
"$$4 :",
"function_definition : declarator $$4 opt_declaration_list T_LBRACE T_MATCHRBRACE",
"opt_declaration_list :",
"opt_declaration_list : T_VA_DCL",
"opt_declaration_list : declaration_list",
"declaration_list : declaration",
"declaration_list : declaration_list declaration",
"decl_specifiers : decl_specifier",
"decl_specifiers : decl_specifiers decl_specifier",
"decl_specifier : storage_class",
"decl_specifier : type_specifier",
"decl_specifier : type_qualifier",
"storage_class : T_AUTO",
"storage_class : T_EXTERN",
"storage_class : T_REGISTER",
"storage_class : T_STATIC",
"storage_class : T_INLINE",
"storage_class : T_EXTENSION",
"type_specifier : T_CHAR",
"type_specifier : T_DOUBLE",
"type_specifier : T_FLOAT",
"type_specifier : T_INT",
"type_specifier : T_LONG",
"type_specifier : T_SHORT",
"type_specifier : T_SIGNED",
"type_specifier : T_UNSIGNED",
"type_specifier : T_VOID",
"type_specifier : T_Bool",
"type_specifier : T_Complex",
"type_specifier : T_Imaginary",
"type_specifier : T_TYPEDEF_NAME",
"type_specifier : struct_or_union_specifier",
"type_specifier : enum_specifier",
"type_qualifier : T_TYPE_QUALIFIER",
"type_qualifier : T_DEFINE_NAME",
"struct_or_union_specifier : struct_or_union any_id braces",
"struct_or_union_specifier : struct_or_union braces",
"struct_or_union_specifier : struct_or_union any_id",
"struct_or_union : T_STRUCT",
"struct_or_union : T_UNION",
"init_declarator_list : init_declarator",
"init_declarator_list : init_declarator_list ',' init_declarator",
"init_declarator : declarator",
"$$5 :",
"init_declarator : declarator '=' $$5 T_INITIALIZER",
"enum_specifier : enumeration any_id braces",
"enum_specifier : enumeration braces",
"enum_specifier : enumeration any_id",
"enumeration : T_ENUM",
"any_id : T_IDENTIFIER",
"any_id : T_TYPEDEF_NAME",
"declarator : pointer direct_declarator",
"declarator : direct_declarator",
"direct_declarator : identifier_or_ref",
"direct_declarator : '(' declarator ')'",
"direct_declarator : direct_declarator T_BRACKETS",
"direct_declarator : direct_declarator '(' parameter_type_list ')'",
"direct_declarator : direct_declarator '(' opt_identifier_list ')'",
"pointer : '*' opt_type_qualifiers",
"pointer : '*' opt_type_qualifiers pointer",
"opt_type_qualifiers :",
"opt_type_qualifiers : type_qualifier_list",
"type_qualifier_list : type_qualifier",
"type_qualifier_list : type_qualifier_list type_qualifier",
"parameter_type_list : parameter_list",
"parameter_type_list : parameter_list ',' T_ELLIPSIS",
"parameter_list : parameter_declaration",
"parameter_list : parameter_list ',' parameter_declaration",
"parameter_declaration : decl_specifiers declarator",
"parameter_declaration : decl_specifiers abs_declarator",
"parameter_declaration : decl_specifiers",
"opt_identifier_list :",
"opt_identifier_list : identifier_list",
"identifier_list : any_id",
"identifier_list : identifier_list ',' any_id",
"identifier_or_ref : any_id",
"identifier_or_ref : '&' any_id",
"abs_declarator : pointer",
"abs_declarator : pointer direct_abs_declarator",
"abs_declarator : direct_abs_declarator",
"direct_abs_declarator : '(' abs_declarator ')'",
"direct_abs_declarator : direct_abs_declarator T_BRACKETS",
"direct_abs_declarator : T_BRACKETS",
"direct_abs_declarator : direct_abs_declarator '(' parameter_type_list ')'",
"direct_abs_declarator : direct_abs_declarator '(' ')'",
"direct_abs_declarator : '(' parameter_type_list ')'",
"direct_abs_declarator : '(' ')'",

};
#endif
#if YYDEBUG
#include <stdio.h>
#endif

/* define the initial stack-sizes */
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH  YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 500
#define YYMAXDEPTH  500
#endif
#endif

#define YYINITSTACKSIZE 500

int      yydebug;
int      yynerrs;
int      yyerrflag;
int      yychar;
short   *yyssp;
YYSTYPE *yyvsp;
YYSTYPE  yyval;
YYSTYPE  yylval;

/* variables for the parser stack */
static short   *yyss;
static short   *yysslim;
static YYSTYPE *yyvs;
static unsigned yystacksize;
#line 816 "grammar.y"

#if defined(__EMX__) || defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(vms)
# ifdef USE_flex
#  include "lexyy.c"
# else
#  include "lex_yy.c"
# endif
#else
# include "lex.yy.c"
#endif

static void
yaccError (char *msg)
{
    func_params = NULL;
    put_error();		/* tell what line we're on, and what file */
    fprintf(stderr, "%s at token '%s'\n", msg, yytext);
}

/* Initialize the table of type qualifier keywords recognized by the lexical
 * analyzer.
 */
void
init_parser (void)
{
    static char *keywords[] = {
	"const",
	"restrict",
	"volatile",
	"interrupt",
#ifdef vms
	"noshare",
	"readonly",
#endif
#if defined(MSDOS) || defined(OS2)
	"__cdecl",
	"__export",
	"__far",
	"__fastcall",
	"__fortran",
	"__huge",
	"__inline",
	"__interrupt",
	"__loadds",
	"__near",
	"__pascal",
	"__saveregs",
	"__segment",
	"__stdcall",
	"__syscall",
	"_cdecl",
	"_cs",
	"_ds",
	"_es",
	"_export",
	"_far",
	"_fastcall",
	"_fortran",
	"_huge",
	"_interrupt",
	"_loadds",
	"_near",
	"_pascal",
	"_saveregs",
	"_seg",
	"_segment",
	"_ss",
	"cdecl",
	"far",
	"huge",
	"near",
	"pascal",
#ifdef OS2
	"__far16",
#endif
#endif
#ifdef __GNUC__
	/* gcc aliases */
	"__builtin_va_arg",
	"__builtin_va_list",
	"__const",
	"__const__",
	"__inline",
	"__inline__",
	"__restrict",
	"__restrict__",
	"__volatile",
	"__volatile__",
#endif
    };
    unsigned i;

    /* Initialize type qualifier table. */
    type_qualifiers = new_symbol_table();
    for (i = 0; i < sizeof(keywords)/sizeof(keywords[0]); ++i) {
	new_symbol(type_qualifiers, keywords[i], NULL, DS_NONE);
    }
}

/* Process the C source file.  Write function prototypes to the standard
 * output.  Convert function definitions and write the converted source
 * code to a temporary file.
 */
void
process_file (FILE *infile, char *name)
{
    char *s;

    if (strlen(name) > 2) {
	s = name + strlen(name) - 2;
	if (*s == '.') {
	    ++s;
	    if (*s == 'l' || *s == 'y')
		BEGIN LEXYACC;
#if defined(MSDOS) || defined(OS2)
	    if (*s == 'L' || *s == 'Y')
		BEGIN LEXYACC;
#endif
	}
    }

    included_files = new_symbol_table();
    typedef_names = new_symbol_table();
    define_names = new_symbol_table();
    inc_depth = -1;
    curly = 0;
    ly_count = 0;
    func_params = NULL;
    yyin = infile;
    include_file(strcpy(base_file, name), func_style != FUNC_NONE);
    if (file_comments) {
#if OPT_LINTLIBRARY
    	if (lintLibrary()) {
	    put_blankline(stdout);
	    begin_tracking();
	}
#endif
	put_string(stdout, "/* ");
	put_string(stdout, cur_file_name());
	put_string(stdout, " */\n");
    }
    yyparse();
    free_symbol_table(define_names);
    free_symbol_table(typedef_names);
    free_symbol_table(included_files);
}

#ifdef NO_LEAKS
void
free_parser(void)
{
    free_symbol_table (type_qualifiers);
#ifdef FLEX_SCANNER
    if (yy_current_buffer != 0)
	yy_delete_buffer(yy_current_buffer);
#endif
}
#endif
#line 803 "grammar.tab.c"
/* allocate initial stack or double stack size, up to YYMAXDEPTH */
static int yygrowstack(void)
{
    int i;
    unsigned newsize;
    short *newss;
    YYSTYPE *newvs;

    if ((newsize = yystacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return -1;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;

    i = yyssp - yyss;
    newss = (yyss != 0)
          ? (short *)realloc(yyss, newsize * sizeof(*newss))
          : (short *)malloc(newsize * sizeof(*newss));
    if (newss == 0)
        return -1;

    yyss  = newss;
    yyssp = newss + i;
    newvs = (yyvs != 0)
          ? (YYSTYPE *)realloc(yyvs, newsize * sizeof(*newvs))
          : (YYSTYPE *)malloc(newsize * sizeof(*newvs));
    if (newvs == 0)
        return -1;

    yyvs = newvs;
    yyvsp = newvs + i;
    yystacksize = newsize;
    yysslim = yyss + newsize - 1;
    return 0;
}

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

    if (yyss == NULL && yygrowstack()) goto yyoverflow;
    yyssp = yyss;
    yyvsp = yyvs;
    yystate = 0;
    *yyssp = 0;

yyloop:
    if ((yyn = yydefred[yystate]) != 0) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
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
        if (yyssp >= yysslim && yygrowstack())
        {
            goto yyoverflow;
        }
        yystate = yytable[yyn];
        *++yyssp = yytable[yyn];
        *++yyvsp = yylval;
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

    yyerror("syntax error");

    goto yyerrlab;

yyerrlab:
    ++yynerrs;

yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yysslim && yygrowstack())
                {
                    goto yyoverflow;
                }
                yystate = yytable[yyn];
                *++yyssp = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
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
        yyval = yyvsp[1-yym];
    else
        memset(&yyval, 0, sizeof yyval);
    switch (yyn)
    {
case 10:
#line 179 "grammar.y"
	{
	    yyerrok;
	}
break;
case 11:
#line 183 "grammar.y"
	{
	    yyerrok;
	}
break;
case 13:
#line 194 "grammar.y"
	{
	    /* Provide an empty action here so bison will not complain about
	     * incompatible types in the default action it normally would
	     * have generated.
	     */
	}
break;
case 14:
#line 201 "grammar.y"
	{
	    /* empty */
	}
break;
case 15:
#line 208 "grammar.y"
	{
#if OPT_LINTLIBRARY
	    if (types_out && want_typedef()) {
		gen_declarations(&yyvsp[-1].decl_spec, (DeclaratorList *)0);
		flush_varargs();
	    }
#endif
	    free_decl_spec(&yyvsp[-1].decl_spec);
	    end_typedef();
	}
break;
case 16:
#line 219 "grammar.y"
	{
	    if (func_params != NULL) {
		set_param_types(func_params, &yyvsp[-2].decl_spec, &yyvsp[-1].decl_list);
	    } else {
		gen_declarations(&yyvsp[-2].decl_spec, &yyvsp[-1].decl_list);
#if OPT_LINTLIBRARY
		flush_varargs();
#endif
		free_decl_list(&yyvsp[-1].decl_list);
	    }
	    free_decl_spec(&yyvsp[-2].decl_spec);
	    end_typedef();
	}
break;
case 17:
#line 233 "grammar.y"
	{
	    cur_decl_spec_flags = yyvsp[0].decl_spec.flags;
	    free_decl_spec(&yyvsp[0].decl_spec);
	}
break;
case 18:
#line 238 "grammar.y"
	{
	    end_typedef();
	}
break;
case 19:
#line 245 "grammar.y"
	{
	    begin_typedef();
	}
break;
case 20:
#line 249 "grammar.y"
	{
	    begin_typedef();
	}
break;
case 23:
#line 261 "grammar.y"
	{
	    int flags = cur_decl_spec_flags;

	    /* If the typedef is a pointer type, then reset the short type
	     * flags so it does not get promoted.
	     */
	    if (strcmp(yyvsp[0].declarator->text, yyvsp[0].declarator->name) != 0)
		flags &= ~(DS_CHAR | DS_SHORT | DS_FLOAT);
	    new_symbol(typedef_names, yyvsp[0].declarator->name, NULL, flags);
	    free_declarator(yyvsp[0].declarator);
	}
break;
case 24:
#line 273 "grammar.y"
	{
	    int flags = cur_decl_spec_flags;

	    if (strcmp(yyvsp[0].declarator->text, yyvsp[0].declarator->name) != 0)
		flags &= ~(DS_CHAR | DS_SHORT | DS_FLOAT);
	    new_symbol(typedef_names, yyvsp[0].declarator->name, NULL, flags);
	    free_declarator(yyvsp[0].declarator);
	}
break;
case 25:
#line 285 "grammar.y"
	{
	    check_untagged(&yyvsp[-1].decl_spec);
	    if (yyvsp[0].declarator->func_def == FUNC_NONE) {
		yyerror("syntax error");
		YYERROR;
	    }
	    func_params = &(yyvsp[0].declarator->head->params);
	    func_params->begin_comment = cur_file->begin_comment;
	    func_params->end_comment = cur_file->end_comment;
	}
break;
case 26:
#line 296 "grammar.y"
	{
	    /* If we're converting to K&R and we've got a nominally K&R
	     * function which has a parameter which is ANSI (i.e., a prototyped
	     * function pointer), then we must override the deciphered value of
	     * 'func_def' so that the parameter will be converted.
	     */
	    if (func_style == FUNC_TRADITIONAL
	     && haveAnsiParam()
	     && yyvsp[-3].declarator->head->func_def == func_style) {
		yyvsp[-3].declarator->head->func_def = FUNC_BOTH;
	    }

	    func_params = NULL;

	    if (cur_file->convert)
		gen_func_definition(&yyvsp[-4].decl_spec, yyvsp[-3].declarator);
	    gen_prototype(&yyvsp[-4].decl_spec, yyvsp[-3].declarator);
#if OPT_LINTLIBRARY
	    flush_varargs();
#endif
	    free_decl_spec(&yyvsp[-4].decl_spec);
	    free_declarator(yyvsp[-3].declarator);
	}
break;
case 28:
#line 321 "grammar.y"
	{
	    if (yyvsp[0].declarator->func_def == FUNC_NONE) {
		yyerror("syntax error");
		YYERROR;
	    }
	    func_params = &(yyvsp[0].declarator->head->params);
	    func_params->begin_comment = cur_file->begin_comment;
	    func_params->end_comment = cur_file->end_comment;
	}
break;
case 29:
#line 331 "grammar.y"
	{
	    DeclSpec decl_spec;

	    func_params = NULL;

	    new_decl_spec(&decl_spec, dft_decl_spec(), yyvsp[-4].declarator->begin, DS_NONE);
	    if (cur_file->convert)
		gen_func_definition(&decl_spec, yyvsp[-4].declarator);
	    gen_prototype(&decl_spec, yyvsp[-4].declarator);
#if OPT_LINTLIBRARY
	    flush_varargs();
#endif
	    free_decl_spec(&decl_spec);
	    free_declarator(yyvsp[-4].declarator);
	}
break;
case 36:
#line 362 "grammar.y"
	{
	    join_decl_specs(&yyval.decl_spec, &yyvsp[-1].decl_spec, &yyvsp[0].decl_spec);
	    free(yyvsp[-1].decl_spec.text);
	    free(yyvsp[0].decl_spec.text);
	}
break;
case 40:
#line 377 "grammar.y"
	{
	    new_decl_spec(&yyval.decl_spec, yyvsp[0].text.text, yyvsp[0].text.begin, DS_NONE);
	}
break;
case 41:
#line 381 "grammar.y"
	{
	    new_decl_spec(&yyval.decl_spec, yyvsp[0].text.text, yyvsp[0].text.begin, DS_EXTERN);
	}
break;
case 42:
#line 385 "grammar.y"
	{
	    new_decl_spec(&yyval.decl_spec, yyvsp[0].text.text, yyvsp[0].text.begin, DS_NONE);
	}
break;
case 43:
#line 389 "grammar.y"
	{
	    new_decl_spec(&yyval.decl_spec, yyvsp[0].text.text, yyvsp[0].text.begin, DS_STATIC);
	}
break;
case 44:
#line 393 "grammar.y"
	{
	    new_decl_spec(&yyval.decl_spec, yyvsp[0].text.text, yyvsp[0].text.begin, DS_INLINE);
	}
break;
case 45:
#line 397 "grammar.y"
	{
	    new_decl_spec(&yyval.decl_spec, yyvsp[0].text.text, yyvsp[0].text.begin, DS_JUNK);
	}
break;
case 46:
#line 404 "grammar.y"
	{
	    new_decl_spec(&yyval.decl_spec, yyvsp[0].text.text, yyvsp[0].text.begin, DS_CHAR);
	}
break;
case 47:
#line 408 "grammar.y"
	{
	    new_decl_spec(&yyval.decl_spec, yyvsp[0].text.text, yyvsp[0].text.begin, DS_NONE);
	}
break;
case 48:
#line 412 "grammar.y"
	{
	    new_decl_spec(&yyval.decl_spec, yyvsp[0].text.text, yyvsp[0].text.begin, DS_FLOAT);
	}
break;
case 49:
#line 416 "grammar.y"
	{
	    new_decl_spec(&yyval.decl_spec, yyvsp[0].text.text, yyvsp[0].text.begin, DS_NONE);
	}
break;
case 50:
#line 420 "grammar.y"
	{
	    new_decl_spec(&yyval.decl_spec, yyvsp[0].text.text, yyvsp[0].text.begin, DS_NONE);
	}
break;
case 51:
#line 424 "grammar.y"
	{
	    new_decl_spec(&yyval.decl_spec, yyvsp[0].text.text, yyvsp[0].text.begin, DS_SHORT);
	}
break;
case 52:
#line 428 "grammar.y"
	{
	    new_decl_spec(&yyval.decl_spec, yyvsp[0].text.text, yyvsp[0].text.begin, DS_NONE);
	}
break;
case 53:
#line 432 "grammar.y"
	{
	    new_decl_spec(&yyval.decl_spec, yyvsp[0].text.text, yyvsp[0].text.begin, DS_NONE);
	}
break;
case 54:
#line 436 "grammar.y"
	{
	    new_decl_spec(&yyval.decl_spec, yyvsp[0].text.text, yyvsp[0].text.begin, DS_NONE);
	}
break;
case 55:
#line 440 "grammar.y"
	{
	    new_decl_spec(&yyval.decl_spec, yyvsp[0].text.text, yyvsp[0].text.begin, DS_CHAR);
	}
break;
case 56:
#line 444 "grammar.y"
	{
	    new_decl_spec(&yyval.decl_spec, yyvsp[0].text.text, yyvsp[0].text.begin, DS_NONE);
	}
break;
case 57:
#line 448 "grammar.y"
	{
	    new_decl_spec(&yyval.decl_spec, yyvsp[0].text.text, yyvsp[0].text.begin, DS_NONE);
	}
break;
case 58:
#line 452 "grammar.y"
	{
	    Symbol *s;
	    s = find_symbol(typedef_names, yyvsp[0].text.text);
	    if (s != NULL)
		new_decl_spec(&yyval.decl_spec, yyvsp[0].text.text, yyvsp[0].text.begin, s->flags);
	}
break;
case 61:
#line 464 "grammar.y"
	{
	    new_decl_spec(&yyval.decl_spec, yyvsp[0].text.text, yyvsp[0].text.begin, DS_NONE);
	}
break;
case 62:
#line 468 "grammar.y"
	{
	    /* This rule allows the <pointer> nonterminal to scan #define
	     * names as if they were type modifiers.
	     */
	    Symbol *s;
	    s = find_symbol(define_names, yyvsp[0].text.text);
	    if (s != NULL)
		new_decl_spec(&yyval.decl_spec, yyvsp[0].text.text, yyvsp[0].text.begin, s->flags);
	}
break;
case 63:
#line 481 "grammar.y"
	{
	    char *s;
	    if ((s = implied_typedef()) == 0)
	        (void)sprintf(s = buf, "%s %s", yyvsp[-2].text.text, yyvsp[-1].text.text);
	    new_decl_spec(&yyval.decl_spec, s, yyvsp[-2].text.begin, DS_NONE);
	}
break;
case 64:
#line 488 "grammar.y"
	{
	    char *s;
	    if ((s = implied_typedef()) == 0)
		(void)sprintf(s = buf, "%s {}", yyvsp[-1].text.text);
	    new_decl_spec(&yyval.decl_spec, s, yyvsp[-1].text.begin, DS_NONE);
	}
break;
case 65:
#line 495 "grammar.y"
	{
	    (void)sprintf(buf, "%s %s", yyvsp[-1].text.text, yyvsp[0].text.text);
	    new_decl_spec(&yyval.decl_spec, buf, yyvsp[-1].text.begin, DS_NONE);
	}
break;
case 66:
#line 503 "grammar.y"
	{
	    imply_typedef(yyval.text.text);
	}
break;
case 67:
#line 507 "grammar.y"
	{
	    imply_typedef(yyval.text.text);
	}
break;
case 68:
#line 514 "grammar.y"
	{
	    new_decl_list(&yyval.decl_list, yyvsp[0].declarator);
	}
break;
case 69:
#line 518 "grammar.y"
	{
	    add_decl_list(&yyval.decl_list, &yyvsp[-2].decl_list, yyvsp[0].declarator);
	}
break;
case 70:
#line 525 "grammar.y"
	{
	    if (yyvsp[0].declarator->func_def != FUNC_NONE && func_params == NULL &&
		func_style == FUNC_TRADITIONAL && cur_file->convert) {
		gen_func_declarator(yyvsp[0].declarator);
		fputs(cur_text(), cur_file->tmp_file);
	    }
	    cur_declarator = yyval.declarator;
	}
break;
case 71:
#line 534 "grammar.y"
	{
	    if (yyvsp[-1].declarator->func_def != FUNC_NONE && func_params == NULL &&
		func_style == FUNC_TRADITIONAL && cur_file->convert) {
		gen_func_declarator(yyvsp[-1].declarator);
		fputs(" =", cur_file->tmp_file);
	    }
	}
break;
case 73:
#line 546 "grammar.y"
	{
	    char *s;
	    if ((s = implied_typedef()) == 0)
		(void)sprintf(s = buf, "enum %s", yyvsp[-1].text.text);
	    new_decl_spec(&yyval.decl_spec, s, yyvsp[-2].text.begin, DS_NONE);
	}
break;
case 74:
#line 553 "grammar.y"
	{
	    char *s;
	    if ((s = implied_typedef()) == 0)
		(void)sprintf(s = buf, "%s {}", yyvsp[-1].text.text);
	    new_decl_spec(&yyval.decl_spec, s, yyvsp[-1].text.begin, DS_NONE);
	}
break;
case 75:
#line 560 "grammar.y"
	{
	    (void)sprintf(buf, "enum %s", yyvsp[0].text.text);
	    new_decl_spec(&yyval.decl_spec, buf, yyvsp[-1].text.begin, DS_NONE);
	}
break;
case 76:
#line 568 "grammar.y"
	{
	    imply_typedef("enum");
	    yyval.text = yyvsp[0].text;
	}
break;
case 79:
#line 581 "grammar.y"
	{
	    yyval.declarator = yyvsp[0].declarator;
	    (void)sprintf(buf, "%s%s", yyvsp[-1].text.text, yyval.declarator->text);
	    free(yyval.declarator->text);
	    yyval.declarator->text = xstrdup(buf);
	    yyval.declarator->begin = yyvsp[-1].text.begin;
	    yyval.declarator->pointer = TRUE;
	}
break;
case 81:
#line 594 "grammar.y"
	{
	    yyval.declarator = new_declarator(yyvsp[0].text.text, yyvsp[0].text.text, yyvsp[0].text.begin);
	}
break;
case 82:
#line 598 "grammar.y"
	{
	    yyval.declarator = yyvsp[-1].declarator;
	    (void)sprintf(buf, "(%s)", yyval.declarator->text);
	    free(yyval.declarator->text);
	    yyval.declarator->text = xstrdup(buf);
	    yyval.declarator->begin = yyvsp[-2].text.begin;
	}
break;
case 83:
#line 606 "grammar.y"
	{
	    yyval.declarator = yyvsp[-1].declarator;
	    (void)sprintf(buf, "%s%s", yyval.declarator->text, yyvsp[0].text.text);
	    free(yyval.declarator->text);
	    yyval.declarator->text = xstrdup(buf);
	}
break;
case 84:
#line 613 "grammar.y"
	{
	    yyval.declarator = new_declarator("%s()", yyvsp[-3].declarator->name, yyvsp[-3].declarator->begin);
	    yyval.declarator->params = yyvsp[-1].param_list;
	    yyval.declarator->func_stack = yyvsp[-3].declarator;
	    yyval.declarator->head = (yyvsp[-3].declarator->func_stack == NULL) ? yyval.declarator : yyvsp[-3].declarator->head;
	    yyval.declarator->func_def = FUNC_ANSI;
	}
break;
case 85:
#line 621 "grammar.y"
	{
	    yyval.declarator = new_declarator("%s()", yyvsp[-3].declarator->name, yyvsp[-3].declarator->begin);
	    yyval.declarator->params = yyvsp[-1].param_list;
	    yyval.declarator->func_stack = yyvsp[-3].declarator;
	    yyval.declarator->head = (yyvsp[-3].declarator->func_stack == NULL) ? yyval.declarator : yyvsp[-3].declarator->head;
	    yyval.declarator->func_def = FUNC_TRADITIONAL;
	}
break;
case 86:
#line 632 "grammar.y"
	{
	    (void)sprintf(yyval.text.text, "*%s", yyvsp[0].text.text);
	    yyval.text.begin = yyvsp[-1].text.begin;
	}
break;
case 87:
#line 637 "grammar.y"
	{
	    (void)sprintf(yyval.text.text, "*%s%s", yyvsp[-1].text.text, yyvsp[0].text.text);
	    yyval.text.begin = yyvsp[-2].text.begin;
	}
break;
case 88:
#line 645 "grammar.y"
	{
	    strcpy(yyval.text.text, "");
	    yyval.text.begin = 0L;
	}
break;
case 90:
#line 654 "grammar.y"
	{
	    (void)sprintf(yyval.text.text, "%s ", yyvsp[0].decl_spec.text);
	    yyval.text.begin = yyvsp[0].decl_spec.begin;
	    free(yyvsp[0].decl_spec.text);
	}
break;
case 91:
#line 660 "grammar.y"
	{
	    (void)sprintf(yyval.text.text, "%s%s ", yyvsp[-1].text.text, yyvsp[0].decl_spec.text);
	    yyval.text.begin = yyvsp[-1].text.begin;
	    free(yyvsp[0].decl_spec.text);
	}
break;
case 93:
#line 670 "grammar.y"
	{
	    add_ident_list(&yyval.param_list, &yyvsp[-2].param_list, "...");
	}
break;
case 94:
#line 677 "grammar.y"
	{
	    new_param_list(&yyval.param_list, yyvsp[0].parameter);
	}
break;
case 95:
#line 681 "grammar.y"
	{
	    add_param_list(&yyval.param_list, &yyvsp[-2].param_list, yyvsp[0].parameter);
	}
break;
case 96:
#line 688 "grammar.y"
	{
	    check_untagged(&yyvsp[-1].decl_spec);
	    yyval.parameter = new_parameter(&yyvsp[-1].decl_spec, yyvsp[0].declarator);
	}
break;
case 97:
#line 693 "grammar.y"
	{
	    check_untagged(&yyvsp[-1].decl_spec);
	    yyval.parameter = new_parameter(&yyvsp[-1].decl_spec, yyvsp[0].declarator);
	}
break;
case 98:
#line 698 "grammar.y"
	{
	    check_untagged(&yyvsp[0].decl_spec);
	    yyval.parameter = new_parameter(&yyvsp[0].decl_spec, (Declarator *)0);
	}
break;
case 99:
#line 706 "grammar.y"
	{
	    new_ident_list(&yyval.param_list);
	}
break;
case 101:
#line 714 "grammar.y"
	{
	    new_ident_list(&yyval.param_list);
	    add_ident_list(&yyval.param_list, &yyval.param_list, yyvsp[0].text.text);
	}
break;
case 102:
#line 719 "grammar.y"
	{
	    add_ident_list(&yyval.param_list, &yyvsp[-2].param_list, yyvsp[0].text.text);
	}
break;
case 103:
#line 726 "grammar.y"
	{
	    yyval.text = yyvsp[0].text;
	}
break;
case 104:
#line 730 "grammar.y"
	{
#if OPT_LINTLIBRARY
	    if (lintLibrary()) { /* Lint doesn't grok C++ ref variables */
		yyval.text = yyvsp[0].text;
	    } else
#endif
		(void)sprintf(yyval.text.text, "&%s", yyvsp[0].text.text);
	    yyval.text.begin = yyvsp[-1].text.begin;
	}
break;
case 105:
#line 743 "grammar.y"
	{
	    yyval.declarator = new_declarator(yyvsp[0].text.text, "", yyvsp[0].text.begin);
	}
break;
case 106:
#line 747 "grammar.y"
	{
	    yyval.declarator = yyvsp[0].declarator;
	    (void)sprintf(buf, "%s%s", yyvsp[-1].text.text, yyval.declarator->text);
	    free(yyval.declarator->text);
	    yyval.declarator->text = xstrdup(buf);
	    yyval.declarator->begin = yyvsp[-1].text.begin;
	}
break;
case 108:
#line 759 "grammar.y"
	{
	    yyval.declarator = yyvsp[-1].declarator;
	    (void)sprintf(buf, "(%s)", yyval.declarator->text);
	    free(yyval.declarator->text);
	    yyval.declarator->text = xstrdup(buf);
	    yyval.declarator->begin = yyvsp[-2].text.begin;
	}
break;
case 109:
#line 767 "grammar.y"
	{
	    yyval.declarator = yyvsp[-1].declarator;
	    (void)sprintf(buf, "%s%s", yyval.declarator->text, yyvsp[0].text.text);
	    free(yyval.declarator->text);
	    yyval.declarator->text = xstrdup(buf);
	}
break;
case 110:
#line 774 "grammar.y"
	{
	    yyval.declarator = new_declarator(yyvsp[0].text.text, "", yyvsp[0].text.begin);
	}
break;
case 111:
#line 778 "grammar.y"
	{
	    yyval.declarator = new_declarator("%s()", "", yyvsp[-3].declarator->begin);
	    yyval.declarator->params = yyvsp[-1].param_list;
	    yyval.declarator->func_stack = yyvsp[-3].declarator;
	    yyval.declarator->head = (yyvsp[-3].declarator->func_stack == NULL) ? yyval.declarator : yyvsp[-3].declarator->head;
	    yyval.declarator->func_def = FUNC_ANSI;
	}
break;
case 112:
#line 786 "grammar.y"
	{
	    yyval.declarator = new_declarator("%s()", "", yyvsp[-2].declarator->begin);
	    yyval.declarator->func_stack = yyvsp[-2].declarator;
	    yyval.declarator->head = (yyvsp[-2].declarator->func_stack == NULL) ? yyval.declarator : yyvsp[-2].declarator->head;
	    yyval.declarator->func_def = FUNC_ANSI;
	}
break;
case 113:
#line 793 "grammar.y"
	{
	    Declarator *d;

	    d = new_declarator("", "", yyvsp[-2].text.begin);
	    yyval.declarator = new_declarator("%s()", "", yyvsp[-2].text.begin);
	    yyval.declarator->params = yyvsp[-1].param_list;
	    yyval.declarator->func_stack = d;
	    yyval.declarator->head = yyval.declarator;
	    yyval.declarator->func_def = FUNC_ANSI;
	}
break;
case 114:
#line 804 "grammar.y"
	{
	    Declarator *d;

	    d = new_declarator("", "", yyvsp[-1].text.begin);
	    yyval.declarator = new_declarator("%s()", "", yyvsp[-1].text.begin);
	    yyval.declarator->func_stack = d;
	    yyval.declarator->head = yyval.declarator;
	    yyval.declarator->func_def = FUNC_ANSI;
	}
break;
#line 1662 "grammar.tab.c"
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
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
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if (yyssp >= yysslim && yygrowstack())
    {
        goto yyoverflow;
    }
    *++yyssp = (short) yystate;
    *++yyvsp = yyval;
    goto yyloop;

yyoverflow:
    yyerror("yacc stack overflow");

yyabort:
    return (1);

yyaccept:
    return (0);
}
