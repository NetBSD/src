#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif
/* original parser id follows */
/* yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93" */
/* (use YYMAJOR/YYMINOR for ifdefs dependent on parser version) */

#define YYBYACC 1
#define YYMAJOR 2
#define YYMINOR 0

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

#line 37 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"

#include <stdio.h>
#include "defs.h"

static id_rec *cur_menu;
static optn_info *cur_optn;
#define OPT_NOMENU 0

#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#line 47 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
typedef union {
	char *s_value;
	int   i_value;
	optn_info *optn_value;
	action a_value;
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
#line 46 "parse.c"

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

#define X 257
#define Y 258
#define W 259
#define H 260
#define NO 261
#define BOX 262
#define SUB 263
#define HELP 264
#define MENU 265
#define NEXT 266
#define EXIT 267
#define ACTION 268
#define ENDWIN 269
#define OPTION 270
#define TITLE 271
#define DEFAULT 272
#define DISPLAY 273
#define ERROR 274
#define EXITSTRING 275
#define EXPAND 276
#define ALLOW 277
#define DYNAMIC 278
#define MENUS 279
#define SCROLLABLE 280
#define SHORTCUT 281
#define CLEAR 282
#define MESSAGES 283
#define ALWAYS 284
#define SCROLL 285
#define CONTINUOUS 286
#define STRING 287
#define NAME 288
#define CODE 289
#define INT_CONST 290
#define CHAR_CONST 291
#define YYERRCODE 256
typedef int YYINT;
static const YYINT yylhs[] = {                           -1,
    0,    1,    1,    9,    9,    9,    9,    9,    9,    9,
   14,   13,   15,   12,   17,   11,   20,   10,   19,   19,
   18,   18,    3,    3,   16,   16,   16,   16,   16,   16,
   16,   16,   16,   16,   16,   16,   16,   16,   16,   16,
   16,   16,   16,   16,   16,   16,   16,    5,    5,   24,
    4,   23,   23,   25,   25,   26,   26,   26,   26,    7,
    6,    6,   21,   21,   22,   22,    8,    8,    2,    2,
    2,
};
static const YYINT yylen[] = {                            2,
    2,    0,    1,    0,    2,    2,    2,    2,    2,    2,
    4,    3,    4,    3,    0,    5,    0,   10,    0,    1,
    2,    3,    1,    1,    2,    1,    2,    1,    2,    1,
    2,    1,    2,    1,    3,    2,    3,    2,    3,    2,
    2,    3,    3,    3,    3,    2,    2,    1,    2,    0,
    6,    1,    1,    1,    3,    3,    3,    1,    1,    3,
    0,    3,    0,    3,    0,    3,    0,    3,    0,    3,
    3,
};
static const YYINT yydefred[] = {                         0,
    3,    0,    4,    0,    0,   15,    0,    0,    5,    6,
    7,    8,    9,   10,   17,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   28,    0,   26,    0,
    0,    0,   30,   32,   34,    0,    0,    0,    0,    0,
   14,   12,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   27,    0,   25,    0,   29,   31,   33,    0,   40,
   24,   23,   46,   36,   47,   38,   41,    0,    0,   60,
   11,   13,   21,    0,    0,   42,   43,   44,   45,   39,
   35,   37,   16,   62,   22,    0,    0,    0,    0,    0,
   64,    0,   50,   48,    0,   66,    0,    0,   49,    0,
   53,   52,    0,    0,    0,   18,    0,   68,    0,    0,
    0,    0,   59,   58,    0,   54,   70,   71,    0,    0,
   51,    0,   57,   56,   55,
};
#if defined(YYDESTRUCT_CALL) || defined(YYSTYPE_TOSTRING)
static const YYINT yystos[] = {                           0,
  289,  293,  294,  302,  265,  272,  274,  277,  303,  304,
  305,  306,  307,  308,  288,  310,  268,  300,  276,  278,
  313,  257,  258,  259,  260,  261,  262,  263,  267,  271,
  272,  275,  280,  281,  282,  284,  286,  309,   40,  299,
   59,   59,  279,  283,   44,  311,  312,   61,   61,   61,
   61,  262,  263,  267,  272,  280,  281,  282,  284,  265,
  287,  288,  296,  267,  296,  285,  271,  311,  269,  289,
   59,   59,  309,   44,   59,  290,  290,  290,  290,  265,
  267,  285,   59,   41,  309,  276,  314,  300,  273,  315,
   59,  300,  270,  297,  298,   59,  317,  267,  297,  301,
  289,  296,  316,  300,  264,  295,   44,   59,  289,  296,
  263,  266,  267,  300,  318,  319,   59,   59,  265,  265,
   59,   44,  288,  288,  319,
};
#endif /* YYDESTRUCT_CALL || YYSTYPE_TOSTRING */
static const YYINT yydgoto[] = {                          2,
    3,  106,   63,   94,   95,   40,  114,  100,    4,    9,
   10,   11,   12,   13,   14,   38,   16,   46,   47,   21,
   87,   90,  103,   97,  115,  116,
};
static const YYINT yysindex[] = {                      -264,
    0,    0,    0, -228, -256,    0, -220, -263,    0,    0,
    0,    0,    0,    0,    0, -253,   14,  -18,   -3, -226,
   15,    5,    6,    8,    9, -246,    0, -194,    0, -245,
 -195, -245,    0,    0,    0, -212, -197,   15, -192, -214,
    0,    0,   19,   21, -253,   37,   24, -206, -205, -204,
 -203,    0, -177,    0, -178,    0,    0,    0, -193,    0,
    0,    0,    0,    0,    0,    0,    0,  -20,   49,    0,
    0,    0,    0, -253, -183,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0, -220, -179,   36, -220, -174,
    0,   38,    0,    0, -247,    0, -227, -220,    0, -166,
    0,    0,   55,   41, -224,    0, -216,    0,   42,   43,
 -162, -161,    0,    0,  -14,    0,    0,    0, -182, -181,
    0, -216,    0,    0,    0,
};
static const YYINT yyrindex[] = {                         2,
    0,    0,    0,  105,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0, -180,    0,    0,    0,
   51,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   52,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0, -215,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0, -158,    0,    0,    0,
    0,    0,    0,    0,    1,    0,    0,    0,    0,   12,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,
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
    0,    0,    0,    0,    0,
};
#endif
static const YYINT yygindex[] = {                         0,
    0,    0,  -29,   13,    0,    0,   -7,    0,    0,    0,
    0,    0,    0,    0,    0,  -34,    0,   75,    0,    0,
    0,    0,    0,    0,    0,   -8,
};
#define YYTABLESIZE 289
static const YYINT yytable[] = {                         18,
   67,    2,   65,   22,   23,   24,   25,   26,   27,   28,
   73,   69,   19,   29,   20,   52,   53,   30,   31,   98,
   54,   32,   93,   74,    1,   55,   33,   34,   35,  122,
   36,   15,   37,   56,   57,   58,    5,   59,   83,   85,
   41,   61,   62,    6,  121,    7,  111,   17,    8,  112,
  113,   17,   43,   39,   63,   42,   44,   63,   45,   61,
   62,  101,   61,   62,  109,   48,   49,  102,   50,   51,
   60,   64,   66,   67,   70,  110,   69,   71,   88,   72,
   74,   92,   75,   76,   77,   78,   79,   80,   81,   84,
  104,   82,   86,   89,   91,   93,   96,  105,  107,  108,
  117,  118,  119,  120,    1,  123,  124,   99,   61,   19,
   20,   65,   68,  125,    0,    0,    0,    0,    0,    0,
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
    0,    0,    0,    0,   67,   67,    2,    0,    0,    0,
    0,    0,   67,    2,   67,    2,   69,   67,    2,    0,
    0,    0,    0,   69,    0,   69,    0,    0,   69,
};
static const YYINT yycheck[] = {                          7,
    0,    0,   32,  257,  258,  259,  260,  261,  262,  263,
   45,    0,  276,  267,  278,  262,  263,  271,  272,  267,
  267,  275,  270,   44,  289,  272,  280,  281,  282,   44,
  284,  288,  286,  280,  281,  282,  265,  284,   59,   74,
   59,  287,  288,  272,   59,  274,  263,  268,  277,  266,
  267,  268,  279,   40,  270,   59,  283,  273,   44,  287,
  288,  289,  287,  288,  289,   61,   61,   97,   61,   61,
  265,  267,  285,  271,  289,  105,  269,   59,   86,   59,
   44,   89,   59,  290,  290,  290,  290,  265,  267,   41,
   98,  285,  276,  273,   59,  270,   59,  264,   44,   59,
   59,   59,  265,  265,    0,  288,  288,   95,  289,   59,
   59,  270,   38,  122,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  264,  265,  265,   -1,   -1,   -1,
   -1,   -1,  272,  272,  274,  274,  265,  277,  277,   -1,
   -1,   -1,   -1,  272,   -1,  274,   -1,   -1,  277,
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
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
};
#endif
#define YYFINAL 2
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 291
#define YYUNDFTOKEN 320
#define YYTRANSLATE(a) ((a) > YYMAXTOKEN ? YYUNDFTOKEN : (a))
#if YYDEBUG
static const char *const yyname[] = {

"$end",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,"'('","')'",0,0,"','",0,0,0,0,0,0,0,0,0,0,0,0,0,0,"';'",0,"'='",0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"error","X","Y","W",
"H","NO","BOX","SUB","HELP","MENU","NEXT","EXIT","ACTION","ENDWIN","OPTION",
"TITLE","DEFAULT","DISPLAY","ERROR","EXITSTRING","EXPAND","ALLOW","DYNAMIC",
"MENUS","SCROLLABLE","SHORTCUT","CLEAR","MESSAGES","ALWAYS","SCROLL",
"CONTINUOUS","STRING","NAME","CODE","INT_CONST","CHAR_CONST","$accept","system",
"init_code","helpstr","text","option","option_list","act_opt","action",
"exitact","menu_list","menu_def","default_def","initerror_def","expand_def",
"dynamic_def","msgxlat_def","opt","$$1","opt_list","opts","$$2","expaction",
"dispact","option_legend","$$3","elem_list","elem","illegal-symbol",
};
static const char *const yyrule[] = {
"$accept : system",
"system : init_code menu_list",
"init_code :",
"init_code : CODE",
"menu_list :",
"menu_list : menu_list menu_def",
"menu_list : menu_list default_def",
"menu_list : menu_list initerror_def",
"menu_list : menu_list expand_def",
"menu_list : menu_list dynamic_def",
"menu_list : menu_list msgxlat_def",
"dynamic_def : ALLOW DYNAMIC MENUS ';'",
"expand_def : ALLOW EXPAND ';'",
"msgxlat_def : ALLOW DYNAMIC MESSAGES ';'",
"initerror_def : ERROR action ';'",
"$$1 :",
"default_def : DEFAULT $$1 opt opt_list ';'",
"$$2 :",
"menu_def : MENU NAME $$2 opts ';' expaction dispact option_list exitact helpstr",
"opts :",
"opts : opt_list",
"opt_list : ',' opt",
"opt_list : opt_list ',' opt",
"text : NAME",
"text : STRING",
"opt : NO EXIT",
"opt : EXIT",
"opt : NO BOX",
"opt : BOX",
"opt : NO SCROLLABLE",
"opt : SCROLLABLE",
"opt : NO SHORTCUT",
"opt : SHORTCUT",
"opt : NO CLEAR",
"opt : CLEAR",
"opt : NO DEFAULT EXIT",
"opt : DEFAULT EXIT",
"opt : NO ALWAYS SCROLL",
"opt : ALWAYS SCROLL",
"opt : NO SUB MENU",
"opt : SUB MENU",
"opt : CONTINUOUS TITLE",
"opt : X '=' INT_CONST",
"opt : Y '=' INT_CONST",
"opt : W '=' INT_CONST",
"opt : H '=' INT_CONST",
"opt : TITLE text",
"opt : EXITSTRING text",
"option_list : option",
"option_list : option_list option",
"$$3 :",
"option : OPTION $$3 option_legend ',' elem_list ';'",
"option_legend : text",
"option_legend : CODE",
"elem_list : elem",
"elem_list : elem_list ',' elem",
"elem : NEXT MENU NAME",
"elem : SUB MENU NAME",
"elem : action",
"elem : EXIT",
"action : ACTION act_opt CODE",
"act_opt :",
"act_opt : '(' ENDWIN ')'",
"expaction :",
"expaction : EXPAND action ';'",
"dispact :",
"dispact : DISPLAY action ';'",
"exitact :",
"exitact : EXIT action ';'",
"helpstr :",
"helpstr : HELP CODE ';'",
"helpstr : HELP text ';'",

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
#line 71 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ check_defined();
	   if (!had_errors)
		   write_menu_file(yystack.l_mark[-1].s_value);
	 }
#line 1 ""
break;
case 2:
#line 77 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ yyval.s_value = ""; }
#line 1 ""
break;
case 11:
#line 91 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ do_dynamic = 1; }
#line 1 ""
break;
case 12:
#line 94 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ do_expands = 1; }
#line 1 ""
break;
case 13:
#line 97 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ do_msgxlat = 1; }
#line 1 ""
break;
case 14:
#line 100 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ error_act = yystack.l_mark[-1].a_value; }
#line 1 ""
break;
case 15:
#line 103 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu = &default_menu; }
#line 1 ""
break;
case 17:
#line 107 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu = get_menu (yystack.l_mark[0].s_value);
		  if (cur_menu->info != NULL)
			  yyerror ("Menu %s defined twice", yystack.l_mark[0].s_value);
		  else {
			  cur_menu->info = malloc (sizeof (menu_info));
			  *(cur_menu->info) = default_info;
		  }
		}
#line 1 ""
break;
case 18:
#line 116 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ optn_info *t;
		  cur_menu->info->optns = NULL;
		  while (yystack.l_mark[-2].optn_value != NULL) {
			  t = yystack.l_mark[-2].optn_value;
			  yystack.l_mark[-2].optn_value = yystack.l_mark[-2].optn_value->next;
			  t->next = cur_menu->info->optns;
			  cur_menu->info->optns = t;
			  cur_menu->info->numopt++;
		  }
		}
#line 1 ""
break;
case 25:
#line 138 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->mopt |= MC_NOEXITOPT; }
#line 1 ""
break;
case 26:
#line 139 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->mopt &= ~MC_NOEXITOPT; }
#line 1 ""
break;
case 27:
#line 140 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->mopt |= MC_NOBOX; }
#line 1 ""
break;
case 28:
#line 141 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->mopt &= ~MC_NOBOX; }
#line 1 ""
break;
case 29:
#line 142 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->mopt &= ~MC_SCROLL; }
#line 1 ""
break;
case 30:
#line 143 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->mopt |= MC_SCROLL; }
#line 1 ""
break;
case 31:
#line 144 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->mopt |= MC_NOSHORTCUT; }
#line 1 ""
break;
case 32:
#line 145 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->mopt &= ~MC_NOSHORTCUT; }
#line 1 ""
break;
case 33:
#line 146 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->mopt |= MC_NOCLEAR; }
#line 1 ""
break;
case 34:
#line 147 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->mopt &= ~MC_NOCLEAR; }
#line 1 ""
break;
case 35:
#line 148 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->mopt &= ~MC_DFLTEXIT; }
#line 1 ""
break;
case 36:
#line 149 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->mopt |= MC_DFLTEXIT; }
#line 1 ""
break;
case 37:
#line 150 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->mopt &= ~MC_ALWAYS_SCROLL; }
#line 1 ""
break;
case 38:
#line 151 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->mopt |= MC_ALWAYS_SCROLL; }
#line 1 ""
break;
case 39:
#line 152 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->mopt &= ~MC_SUBMENU; }
#line 1 ""
break;
case 40:
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->mopt |= MC_SUBMENU; }
#line 1 ""
break;
case 41:
#line 154 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->mopt |= MC_CONTINUOUS; }
#line 1 ""
break;
case 42:
#line 155 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->x = atoi(yystack.l_mark[0].s_value); }
#line 1 ""
break;
case 43:
#line 156 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->y = atoi(yystack.l_mark[0].s_value); }
#line 1 ""
break;
case 44:
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->w = atoi(yystack.l_mark[0].s_value); }
#line 1 ""
break;
case 45:
#line 158 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->h = atoi(yystack.l_mark[0].s_value); }
#line 1 ""
break;
case 46:
#line 159 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->title = yystack.l_mark[0].s_value; }
#line 1 ""
break;
case 47:
#line 160 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->exitstr = yystack.l_mark[0].s_value;
				  cur_menu->info->mopt &= ~MC_NOEXITOPT; }
#line 1 ""
break;
case 49:
#line 165 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ yystack.l_mark[0].optn_value->next = yystack.l_mark[-1].optn_value; yyval.optn_value = yystack.l_mark[0].optn_value; }
#line 1 ""
break;
case 50:
#line 169 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_optn = calloc(1, sizeof(*cur_optn));
		  cur_optn->optact.code = "";
		}
#line 1 ""
break;
case 51:
#line 174 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ yyval.optn_value = cur_optn; }
#line 1 ""
break;
case 52:
#line 177 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_optn->name = yystack.l_mark[0].s_value; }
#line 1 ""
break;
case 53:
#line 178 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_optn->name = yystack.l_mark[0].s_value; cur_optn->name_is_code = TRUE;}
#line 1 ""
break;
case 56:
#line 185 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ id_rec *t = get_menu (yystack.l_mark[0].s_value);
		  if (cur_optn->menu != OPT_NOMENU)
			  yyerror ("Double sub/next menu definition");
		  else {
			  cur_optn->menu = t->menu_no;
		  }
		}
#line 1 ""
break;
case 57:
#line 193 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ id_rec *t = get_menu (yystack.l_mark[0].s_value);
		  if (cur_optn->menu != OPT_NOMENU)
			  yyerror ("Double sub/next menu definition");
		  else {
			  cur_optn->menu = t->menu_no;
			  cur_optn->issub = TRUE;
		  }
		}
#line 1 ""
break;
case 58:
#line 201 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_optn->optact = yystack.l_mark[0].a_value; }
#line 1 ""
break;
case 59:
#line 202 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_optn->doexit = TRUE; }
#line 1 ""
break;
case 60:
#line 206 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ yyval.a_value.code = yystack.l_mark[0].s_value;
		  yyval.a_value.endwin = yystack.l_mark[-1].i_value;
		}
#line 1 ""
break;
case 61:
#line 211 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ yyval.i_value = 0; }
#line 1 ""
break;
case 62:
#line 212 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ yyval.i_value = 1; }
#line 1 ""
break;
case 63:
#line 215 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->expact.code = ""; }
#line 1 ""
break;
case 64:
#line 216 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ if (!do_expands) yyerror ("Menu expands "
	  						     "not enabled");
	  			  cur_menu->info->expact = yystack.l_mark[-1].a_value; }
#line 1 ""
break;
case 65:
#line 221 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->postact.code = ""; }
#line 1 ""
break;
case 66:
#line 222 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->postact = yystack.l_mark[-1].a_value; }
#line 1 ""
break;
case 67:
#line 226 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->exitact.code = ""; }
#line 1 ""
break;
case 68:
#line 227 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->exitact = yystack.l_mark[-1].a_value; }
#line 1 ""
break;
case 69:
#line 230 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->helpstr = NULL; }
#line 1 ""
break;
case 70:
#line 231 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ asprintf(&cur_menu->info->helpstr, "\"%s\"", yystack.l_mark[-1].s_value); }
#line 1 ""
break;
case 71:
#line 232 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/menuc/../../usr.bin/menuc/parse.y"
	{ cur_menu->info->helpstr = yystack.l_mark[-1].s_value; }
#line 1 ""
break;
#line 1528 "parse.c"
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
