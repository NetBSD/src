/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

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

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse rx_parse
#define yylex   rx_lex
#define yyerror rx_error
#define yylval  rx_lval
#define yychar  rx_char
#define yydebug rx_debug
#define yynerrs rx_nerrs


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     REG = 258,
     FLAG = 259,
     CREG = 260,
     EXPR = 261,
     UNKNOWN_OPCODE = 262,
     IS_OPCODE = 263,
     DOT_S = 264,
     DOT_B = 265,
     DOT_W = 266,
     DOT_L = 267,
     DOT_A = 268,
     DOT_UB = 269,
     DOT_UW = 270,
     ABS = 271,
     ADC = 272,
     ADD = 273,
     AND_ = 274,
     BCLR = 275,
     BCND = 276,
     BMCND = 277,
     BNOT = 278,
     BRA = 279,
     BRK = 280,
     BSET = 281,
     BSR = 282,
     BTST = 283,
     CLRPSW = 284,
     CMP = 285,
     DBT = 286,
     DIV = 287,
     DIVU = 288,
     EDIV = 289,
     EDIVU = 290,
     EMUL = 291,
     EMULU = 292,
     FADD = 293,
     FCMP = 294,
     FDIV = 295,
     FMUL = 296,
     FREIT = 297,
     FSUB = 298,
     FTOI = 299,
     INT = 300,
     ITOF = 301,
     JMP = 302,
     JSR = 303,
     MACHI = 304,
     MACLO = 305,
     MAX = 306,
     MIN = 307,
     MOV = 308,
     MOVU = 309,
     MUL = 310,
     MULHI = 311,
     MULLO = 312,
     MULU = 313,
     MVFACHI = 314,
     MVFACMI = 315,
     MVFACLO = 316,
     MVFC = 317,
     MVTACHI = 318,
     MVTACLO = 319,
     MVTC = 320,
     MVTIPL = 321,
     NEG = 322,
     NOP = 323,
     NOT = 324,
     OR = 325,
     POP = 326,
     POPC = 327,
     POPM = 328,
     PUSH = 329,
     PUSHA = 330,
     PUSHC = 331,
     PUSHM = 332,
     RACW = 333,
     REIT = 334,
     REVL = 335,
     REVW = 336,
     RMPA = 337,
     ROLC = 338,
     RORC = 339,
     ROTL = 340,
     ROTR = 341,
     ROUND = 342,
     RTE = 343,
     RTFI = 344,
     RTS = 345,
     RTSD = 346,
     SAT = 347,
     SATR = 348,
     SBB = 349,
     SCCND = 350,
     SCMPU = 351,
     SETPSW = 352,
     SHAR = 353,
     SHLL = 354,
     SHLR = 355,
     SMOVB = 356,
     SMOVF = 357,
     SMOVU = 358,
     SSTR = 359,
     STNZ = 360,
     STOP = 361,
     STZ = 362,
     SUB = 363,
     SUNTIL = 364,
     SWHILE = 365,
     TST = 366,
     WAIT = 367,
     XCHG = 368,
     XOR = 369
   };
#endif
/* Tokens.  */
#define REG 258
#define FLAG 259
#define CREG 260
#define EXPR 261
#define UNKNOWN_OPCODE 262
#define IS_OPCODE 263
#define DOT_S 264
#define DOT_B 265
#define DOT_W 266
#define DOT_L 267
#define DOT_A 268
#define DOT_UB 269
#define DOT_UW 270
#define ABS 271
#define ADC 272
#define ADD 273
#define AND_ 274
#define BCLR 275
#define BCND 276
#define BMCND 277
#define BNOT 278
#define BRA 279
#define BRK 280
#define BSET 281
#define BSR 282
#define BTST 283
#define CLRPSW 284
#define CMP 285
#define DBT 286
#define DIV 287
#define DIVU 288
#define EDIV 289
#define EDIVU 290
#define EMUL 291
#define EMULU 292
#define FADD 293
#define FCMP 294
#define FDIV 295
#define FMUL 296
#define FREIT 297
#define FSUB 298
#define FTOI 299
#define INT 300
#define ITOF 301
#define JMP 302
#define JSR 303
#define MACHI 304
#define MACLO 305
#define MAX 306
#define MIN 307
#define MOV 308
#define MOVU 309
#define MUL 310
#define MULHI 311
#define MULLO 312
#define MULU 313
#define MVFACHI 314
#define MVFACMI 315
#define MVFACLO 316
#define MVFC 317
#define MVTACHI 318
#define MVTACLO 319
#define MVTC 320
#define MVTIPL 321
#define NEG 322
#define NOP 323
#define NOT 324
#define OR 325
#define POP 326
#define POPC 327
#define POPM 328
#define PUSH 329
#define PUSHA 330
#define PUSHC 331
#define PUSHM 332
#define RACW 333
#define REIT 334
#define REVL 335
#define REVW 336
#define RMPA 337
#define ROLC 338
#define RORC 339
#define ROTL 340
#define ROTR 341
#define ROUND 342
#define RTE 343
#define RTFI 344
#define RTS 345
#define RTSD 346
#define SAT 347
#define SATR 348
#define SBB 349
#define SCCND 350
#define SCMPU 351
#define SETPSW 352
#define SHAR 353
#define SHLL 354
#define SHLR 355
#define SMOVB 356
#define SMOVF 357
#define SMOVU 358
#define SSTR 359
#define STNZ 360
#define STOP 361
#define STZ 362
#define SUB 363
#define SUNTIL 364
#define SWHILE 365
#define TST 366
#define WAIT 367
#define XCHG 368
#define XOR 369




/* Copy the first part of user declarations.  */
#line 21 "rx-parse.y"


#include "as.h"
#include "safe-ctype.h"
#include "rx-defs.h"

static int rx_lex (void);

#define COND_EQ	0
#define COND_NE	1

#define MEMEX 0x06

#define BSIZE 0
#define WSIZE 1
#define LSIZE 2

/*                       .sb    .sw    .l     .uw   */
static int sizemap[] = { BSIZE, WSIZE, LSIZE, WSIZE };

/* Ok, here are the rules for using these macros...

   B*() is used to specify the base opcode bytes.  Fields to be filled
        in later, leave zero.  Call this first.

   F() and FE() are used to fill in fields within the base opcode bytes.  You MUST
        call B*() before any F() or FE().

   [UN]*O*(), PC*() appends operands to the end of the opcode.  You
        must call P() and B*() before any of these, so that the fixups
        have the right byte location.
        O = signed, UO = unsigned, NO = negated, PC = pcrel

   IMM() adds an immediate and fills in the field for it.
   NIMM() same, but negates the immediate.
   NBIMM() same, but negates the immediate, for sbb.
   DSP() adds a displacement, and fills in the field for it.

   Note that order is significant for the O, IMM, and DSP macros, as
   they append their data to the operand buffer in the order that you
   call them.

   Use "disp" for displacements whenever possible; this handles the
   "0" case properly.  */

#define B1(b1)             rx_base1 (b1)
#define B2(b1, b2)         rx_base2 (b1, b2)
#define B3(b1, b2, b3)     rx_base3 (b1, b2, b3)
#define B4(b1, b2, b3, b4) rx_base4 (b1, b2, b3, b4)

/* POS is bits from the MSB of the first byte to the LSB of the last byte.  */
#define F(val,pos,sz)      rx_field (val, pos, sz)
#define FE(exp,pos,sz)	   rx_field (exp_val (exp), pos, sz);

#define O1(v)              rx_op (v, 1, RXREL_SIGNED); rx_range (v, -128, 255)
#define O2(v)              rx_op (v, 2, RXREL_SIGNED); rx_range (v, -32768, 65536)
#define O3(v)              rx_op (v, 3, RXREL_SIGNED); rx_range (v, -8388608, 16777216)
#define O4(v)              rx_op (v, 4, RXREL_SIGNED)

#define UO1(v)             rx_op (v, 1, RXREL_UNSIGNED); rx_range (v, 0, 255)
#define UO2(v)             rx_op (v, 2, RXREL_UNSIGNED); rx_range (v, 0, 65536)
#define UO3(v)             rx_op (v, 3, RXREL_UNSIGNED); rx_range (v, 0, 16777216)
#define UO4(v)             rx_op (v, 4, RXREL_UNSIGNED)

#define NO1(v)             rx_op (v, 1, RXREL_NEGATIVE)
#define NO2(v)             rx_op (v, 2, RXREL_NEGATIVE)
#define NO3(v)             rx_op (v, 3, RXREL_NEGATIVE)
#define NO4(v)             rx_op (v, 4, RXREL_NEGATIVE)

#define PC1(v)             rx_op (v, 1, RXREL_PCREL)
#define PC2(v)             rx_op (v, 2, RXREL_PCREL)
#define PC3(v)             rx_op (v, 3, RXREL_PCREL)

#define IMM_(v,pos,size)   F (immediate (v, RXREL_SIGNED, pos, size), pos, 2); \
			   if (v.X_op != O_constant && v.X_op != O_big) rx_linkrelax_imm (pos)
#define IMM(v,pos)	   IMM_ (v, pos, 32)
#define IMMW(v,pos)	   IMM_ (v, pos, 16); rx_range (v, -32768, 65536)
#define IMMB(v,pos)	   IMM_ (v, pos, 8); rx_range (v, -128, 255)
#define NIMM(v,pos)	   F (immediate (v, RXREL_NEGATIVE, pos, 32), pos, 2)
#define NBIMM(v,pos)	   F (immediate (v, RXREL_NEGATIVE_BORROW, pos, 32), pos, 2)
#define DSP(v,pos,msz)	   if (!v.X_md) rx_relax (RX_RELAX_DISP, pos); \
			   else rx_linkrelax_dsp (pos); \
			   F (displacement (v, msz), pos, 2)

#define id24(a,b2,b3)	   B3 (0xfb+a, b2, b3)

static int         rx_intop (expressionS, int, int);
static int         rx_uintop (expressionS, int);
static int         rx_disp3op (expressionS);
static int         rx_disp5op (expressionS *, int);
static int         rx_disp5op0 (expressionS *, int);
static int         exp_val (expressionS exp);
static expressionS zero_expr (void);
static int         immediate (expressionS, int, int, int);
static int         displacement (expressionS, int);
static void        rtsd_immediate (expressionS);
static void	   rx_range (expressionS, int, int);

static int    need_flag = 0;
static int    rx_in_brackets = 0;
static int    rx_last_token = 0;
static char * rx_init_start;
static char * rx_last_exp_start = 0;
static int    sub_op;
static int    sub_op2;

#define YYDEBUG 1
#define YYERROR_VERBOSE 1



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

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 134 "rx-parse.y"
{
  int regno;
  expressionS exp;
}
/* Line 193 of yacc.c.  */
#line 448 "rx-parse.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 461 "rx-parse.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
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
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
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
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  216
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   612

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  121
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  59
/* YYNRULES -- Number of rules.  */
#define YYNRULES  241
/* YYNRULES -- Number of states.  */
#define YYNSTATES  607

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   369

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,   115,     2,     2,     2,     2,
       2,     2,     2,   120,   116,   119,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   117,     2,   118,     2,     2,     2,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    11,    13,    16,    20,
      24,    27,    31,    35,    39,    43,    47,    51,    55,    58,
      68,    78,    88,    96,   101,   110,   119,   125,   133,   142,
     148,   154,   160,   166,   172,   178,   185,   191,   195,   196,
     200,   201,   205,   206,   210,   215,   220,   228,   232,   238,
     244,   250,   253,   256,   259,   263,   266,   269,   272,   275,
     278,   281,   284,   288,   292,   294,   296,   298,   300,   303,
     306,   309,   312,   314,   316,   318,   320,   324,   333,   342,
     350,   361,   373,   379,   387,   397,   407,   417,   424,   425,
     429,   430,   434,   435,   439,   440,   444,   445,   449,   450,
     454,   455,   459,   460,   464,   465,   469,   470,   474,   475,
     479,   480,   484,   485,   489,   490,   494,   495,   499,   500,
     504,   505,   509,   510,   514,   515,   519,   524,   529,   534,
     539,   548,   557,   566,   575,   576,   580,   581,   585,   586,
     590,   591,   595,   596,   600,   601,   605,   606,   610,   614,
     621,   631,   641,   646,   651,   656,   661,   664,   667,   670,
     673,   676,   680,   689,   698,   707,   716,   725,   734,   735,
     739,   740,   744,   745,   749,   750,   754,   759,   764,   770,
     776,   782,   788,   794,   804,   814,   824,   825,   829,   830,
     834,   835,   839,   840,   844,   845,   849,   855,   859,   867,
     875,   881,   885,   893,   897,   905,   913,   918,   920,   922,
     924,   926,   930,   932,   936,   944,   952,   956,   961,   968,
     970,   975,   977,   981,   989,   990,   992,   993,   996,   998,
    1000,  1001,  1003,  1005,  1006,  1008,  1010,  1012,  1013,  1015,
    1017,  1018
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     122,     0,    -1,     7,    -1,    25,    -1,    31,    -1,    90,
      -1,    68,    -1,    24,     6,    -1,    24,    13,     6,    -1,
      24,     9,     6,    -1,    27,     6,    -1,    27,    13,     6,
      -1,    21,     9,     6,    -1,    21,    10,     6,    -1,    24,
      10,     6,    -1,    24,    11,     6,    -1,    27,    11,     6,
      -1,    21,    11,     6,    -1,    21,     6,    -1,    53,    10,
     115,     6,   116,   173,   117,     3,   118,    -1,    53,    11,
     115,     6,   116,   173,   117,     3,   118,    -1,    53,    12,
     115,     6,   116,   173,   117,     3,   118,    -1,    91,   115,
       6,   116,     3,   119,     3,    -1,    30,     3,   116,     3,
      -1,    30,   173,   117,     3,   118,    14,   116,     3,    -1,
      30,   173,   117,     3,   118,   176,   116,     3,    -1,    54,
     178,     3,   116,     3,    -1,    54,   178,   117,     3,   118,
     116,     3,    -1,    54,   178,     6,   117,     3,   118,   116,
       3,    -1,   108,   115,     6,   116,     3,    -1,    30,   115,
       6,   116,     3,    -1,    18,   115,     6,   116,     3,    -1,
      55,   115,     6,   116,     3,    -1,    19,   115,     6,   116,
       3,    -1,    70,   115,     6,   116,     3,    -1,    53,    12,
     115,     6,   116,     3,    -1,    53,   115,     6,   116,     3,
      -1,    91,   115,     6,    -1,    -1,   100,   123,   170,    -1,
      -1,    98,   124,   170,    -1,    -1,    99,   125,   170,    -1,
      77,     3,   119,     3,    -1,    73,     3,   119,     3,    -1,
      18,   115,     6,   116,     3,   116,     3,    -1,    45,   115,
       6,    -1,    26,   115,     6,   116,     3,    -1,    20,   115,
       6,   116,     3,    -1,    28,   115,     6,   116,     3,    -1,
      92,     3,    -1,    84,     3,    -1,    83,     3,    -1,    74,
     177,     3,    -1,    71,     3,    -1,    76,     5,    -1,    72,
       5,    -1,    97,   174,    -1,    29,   174,    -1,    47,     3,
      -1,    48,     3,    -1,    24,   179,     3,    -1,    27,   179,
       3,    -1,    96,    -1,   103,    -1,   101,    -1,   102,    -1,
     109,   177,    -1,   110,   177,    -1,   104,   177,    -1,    82,
     177,    -1,    89,    -1,    88,    -1,   112,    -1,    93,    -1,
      66,   115,     6,    -1,    53,   177,     3,   116,     6,   117,
       3,   118,    -1,    53,   177,     6,   117,     3,   118,   116,
       3,    -1,    53,   177,     3,   116,   117,     3,   118,    -1,
      53,   177,   117,     3,   118,   116,   173,   117,     3,   118,
      -1,    53,   177,     6,   117,     3,   118,   116,   173,   117,
       3,   118,    -1,    53,   177,     3,   116,     3,    -1,    53,
     177,   117,     3,   118,   116,     3,    -1,    26,   115,     6,
     116,   173,   117,     3,   118,    10,    -1,    20,   115,     6,
     116,   173,   117,     3,   118,    10,    -1,    28,   115,     6,
     116,   173,   117,     3,   118,    10,    -1,    74,   177,   173,
     117,     3,   118,    -1,    -1,    94,   126,   162,    -1,    -1,
      67,   127,   167,    -1,    -1,    17,   128,   166,    -1,    -1,
      16,   129,   167,    -1,    -1,    51,   130,   165,    -1,    -1,
      52,   131,   165,    -1,    -1,    36,   132,   164,    -1,    -1,
      37,   133,   164,    -1,    -1,    32,   134,   165,    -1,    -1,
      33,   135,   165,    -1,    -1,   111,   136,   165,    -1,    -1,
     114,   137,   165,    -1,    -1,    69,   138,   167,    -1,    -1,
     107,   139,   164,    -1,    -1,   105,   140,   164,    -1,    -1,
      36,   141,   168,    -1,    -1,    37,   142,   168,    -1,    -1,
     113,   143,   168,    -1,    -1,    46,   144,   168,    -1,    26,
       3,   116,     3,    -1,    20,     3,   116,     3,    -1,    28,
       3,   116,     3,    -1,    23,     3,   116,     3,    -1,    26,
       3,   116,   173,   117,     3,   118,    10,    -1,    20,     3,
     116,   173,   117,     3,   118,    10,    -1,    28,     3,   116,
     173,   117,     3,   118,    10,    -1,    23,     3,   116,   173,
     117,     3,   118,    10,    -1,    -1,    43,   145,   171,    -1,
      -1,    39,   146,   171,    -1,    -1,    38,   147,   171,    -1,
      -1,    41,   148,   171,    -1,    -1,    40,   149,   171,    -1,
      -1,    44,   150,   172,    -1,    -1,    87,   151,   172,    -1,
      95,    12,     3,    -1,    95,   177,   173,   117,     3,   118,
      -1,    22,   115,     6,   116,   173,   117,     3,   118,    10,
      -1,    23,   115,     6,   116,   173,   117,     3,   118,    10,
      -1,    56,     3,   116,     3,    -1,    57,     3,   116,     3,
      -1,    49,     3,   116,     3,    -1,    50,     3,   116,     3,
      -1,    63,     3,    -1,    64,     3,    -1,    59,     3,    -1,
      60,     3,    -1,    61,     3,    -1,    78,   115,     6,    -1,
      53,   177,     3,   116,   117,     3,   120,   118,    -1,    53,
     177,     3,   116,   117,   119,     3,   118,    -1,    53,   177,
     117,     3,   120,   118,   116,     3,    -1,    53,   177,   117,
     119,     3,   118,   116,     3,    -1,    54,   178,   117,     3,
     120,   118,   116,     3,    -1,    54,   178,   117,   119,     3,
     118,   116,     3,    -1,    -1,    85,   152,   169,    -1,    -1,
      86,   153,   169,    -1,    -1,    81,   154,   169,    -1,    -1,
      80,   155,   169,    -1,    65,     3,   116,     5,    -1,    62,
       5,   116,     3,    -1,    85,   115,     6,   116,     3,    -1,
      86,   115,     6,   116,     3,    -1,    65,   115,     6,   116,
       5,    -1,    22,   115,     6,   116,     3,    -1,    23,   115,
       6,   116,     3,    -1,    53,   177,     3,   116,   117,     3,
     116,     3,   118,    -1,    53,   177,   117,     3,   116,     3,
     118,   116,     3,    -1,    54,   178,   117,     3,   116,     3,
     118,   116,     3,    -1,    -1,   108,   156,   161,    -1,    -1,
      18,   157,   161,    -1,    -1,    55,   158,   161,    -1,    -1,
      19,   159,   161,    -1,    -1,    70,   160,   161,    -1,    94,
     115,     6,   116,     3,    -1,     3,   116,     3,    -1,   173,
     117,     3,   118,    14,   116,     3,    -1,   173,   117,     3,
     118,   176,   116,     3,    -1,     3,   116,     3,   116,     3,
      -1,     3,   116,     3,    -1,   173,   117,     3,   118,   179,
     116,     3,    -1,     3,   116,     3,    -1,   173,   117,     3,
     118,    14,   116,     3,    -1,   173,   117,     3,   118,   176,
     116,     3,    -1,   115,     6,   116,     3,    -1,   163,    -1,
     164,    -1,   162,    -1,   164,    -1,     3,   116,     3,    -1,
       3,    -1,     3,   116,     3,    -1,   173,   117,     3,   118,
      14,   116,     3,    -1,   173,   117,     3,   118,   176,   116,
       3,    -1,     3,   116,     3,    -1,   115,     6,   116,     3,
      -1,   115,     6,   116,     3,   116,     3,    -1,   169,    -1,
     115,     6,   116,     3,    -1,   172,    -1,     3,   116,     3,
      -1,   173,   117,     3,   118,   179,   116,     3,    -1,    -1,
       6,    -1,    -1,   175,     4,    -1,    10,    -1,    11,    -1,
      -1,    12,    -1,    15,    -1,    -1,    10,    -1,    11,    -1,
      12,    -1,    -1,    10,    -1,    11,    -1,    -1,    12,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   174,   174,   179,   182,   185,   188,   193,   208,   211,
     216,   225,   230,   238,   241,   246,   248,   250,   255,   273,
     281,   287,   295,   304,   309,   312,   317,   322,   325,   333,
     340,   348,   354,   360,   366,   372,   380,   390,   395,   395,
     396,   396,   397,   397,   401,   414,   427,   432,   437,   439,
     444,   449,   451,   453,   458,   463,   468,   476,   484,   486,
     491,   493,   495,   497,   502,   504,   506,   508,   513,   515,
     517,   522,   527,   529,   531,   533,   538,   544,   552,   566,
     571,   576,   581,   586,   591,   593,   595,   600,   605,   605,
     606,   606,   607,   607,   608,   608,   609,   609,   610,   610,
     611,   611,   612,   612,   613,   613,   614,   614,   615,   615,
     616,   616,   617,   617,   618,   618,   619,   619,   623,   623,
     624,   624,   625,   625,   626,   626,   630,   632,   634,   636,
     639,   641,   643,   645,   650,   650,   651,   651,   652,   652,
     653,   653,   654,   654,   655,   655,   656,   656,   660,   662,
     667,   673,   679,   681,   683,   685,   691,   693,   695,   697,
     699,   702,   713,   715,   720,   722,   727,   729,   734,   734,
     735,   735,   736,   736,   737,   737,   741,   747,   752,   754,
     759,   764,   770,   775,   778,   781,   786,   786,   787,   787,
     788,   788,   789,   789,   790,   790,   795,   805,   807,   809,
     811,   818,   820,   828,   830,   832,   838,   843,   844,   848,
     849,   853,   855,   861,   863,   865,   872,   876,   878,   880,
     886,   888,   891,   893,   899,   900,   903,   903,   908,   909,
     910,   911,   912,   915,   916,   917,   918,   921,   922,   923,
     926,   927
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "REG", "FLAG", "CREG", "EXPR",
  "UNKNOWN_OPCODE", "IS_OPCODE", "DOT_S", "DOT_B", "DOT_W", "DOT_L",
  "DOT_A", "DOT_UB", "DOT_UW", "ABS", "ADC", "ADD", "AND_", "BCLR", "BCND",
  "BMCND", "BNOT", "BRA", "BRK", "BSET", "BSR", "BTST", "CLRPSW", "CMP",
  "DBT", "DIV", "DIVU", "EDIV", "EDIVU", "EMUL", "EMULU", "FADD", "FCMP",
  "FDIV", "FMUL", "FREIT", "FSUB", "FTOI", "INT", "ITOF", "JMP", "JSR",
  "MACHI", "MACLO", "MAX", "MIN", "MOV", "MOVU", "MUL", "MULHI", "MULLO",
  "MULU", "MVFACHI", "MVFACMI", "MVFACLO", "MVFC", "MVTACHI", "MVTACLO",
  "MVTC", "MVTIPL", "NEG", "NOP", "NOT", "OR", "POP", "POPC", "POPM",
  "PUSH", "PUSHA", "PUSHC", "PUSHM", "RACW", "REIT", "REVL", "REVW",
  "RMPA", "ROLC", "RORC", "ROTL", "ROTR", "ROUND", "RTE", "RTFI", "RTS",
  "RTSD", "SAT", "SATR", "SBB", "SCCND", "SCMPU", "SETPSW", "SHAR", "SHLL",
  "SHLR", "SMOVB", "SMOVF", "SMOVU", "SSTR", "STNZ", "STOP", "STZ", "SUB",
  "SUNTIL", "SWHILE", "TST", "WAIT", "XCHG", "XOR", "'#'", "','", "'['",
  "']'", "'-'", "'+'", "$accept", "statement", "@1", "@2", "@3", "@4",
  "@5", "@6", "@7", "@8", "@9", "@10", "@11", "@12", "@13", "@14", "@15",
  "@16", "@17", "@18", "@19", "@20", "@21", "@22", "@23", "@24", "@25",
  "@26", "@27", "@28", "@29", "@30", "@31", "@32", "@33", "@34", "@35",
  "@36", "@37", "@38", "op_subadd", "op_dp20_rm_l", "op_dp20_rm",
  "op_dp20_i", "op_dp20_rim", "op_dp20_rim_l", "op_dp20_rr", "op_xchg",
  "op_shift_rot", "op_shift", "float2_op", "float2_op_ni", "disp", "flag",
  "@39", "memex", "bwl", "bw", "opt_l", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,    35,    44,    91,    93,    45,
      43
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   121,   122,   122,   122,   122,   122,   122,   122,   122,
     122,   122,   122,   122,   122,   122,   122,   122,   122,   122,
     122,   122,   122,   122,   122,   122,   122,   122,   122,   122,
     122,   122,   122,   122,   122,   122,   122,   122,   123,   122,
     124,   122,   125,   122,   122,   122,   122,   122,   122,   122,
     122,   122,   122,   122,   122,   122,   122,   122,   122,   122,
     122,   122,   122,   122,   122,   122,   122,   122,   122,   122,
     122,   122,   122,   122,   122,   122,   122,   122,   122,   122,
     122,   122,   122,   122,   122,   122,   122,   122,   126,   122,
     127,   122,   128,   122,   129,   122,   130,   122,   131,   122,
     132,   122,   133,   122,   134,   122,   135,   122,   136,   122,
     137,   122,   138,   122,   139,   122,   140,   122,   141,   122,
     142,   122,   143,   122,   144,   122,   122,   122,   122,   122,
     122,   122,   122,   122,   145,   122,   146,   122,   147,   122,
     148,   122,   149,   122,   150,   122,   151,   122,   122,   122,
     122,   122,   122,   122,   122,   122,   122,   122,   122,   122,
     122,   122,   122,   122,   122,   122,   122,   122,   152,   122,
     153,   122,   154,   122,   155,   122,   122,   122,   122,   122,
     122,   122,   122,   122,   122,   122,   156,   122,   157,   122,
     158,   122,   159,   122,   160,   122,   122,   161,   161,   161,
     161,   162,   162,   163,   163,   163,   164,   165,   165,   166,
     166,   167,   167,   168,   168,   168,   169,   170,   170,   170,
     171,   171,   172,   172,   173,   173,   175,   174,   176,   176,
     176,   176,   176,   177,   177,   177,   177,   178,   178,   178,
     179,   179
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     2,     3,     3,
       2,     3,     3,     3,     3,     3,     3,     3,     2,     9,
       9,     9,     7,     4,     8,     8,     5,     7,     8,     5,
       5,     5,     5,     5,     5,     6,     5,     3,     0,     3,
       0,     3,     0,     3,     4,     4,     7,     3,     5,     5,
       5,     2,     2,     2,     3,     2,     2,     2,     2,     2,
       2,     2,     3,     3,     1,     1,     1,     1,     2,     2,
       2,     2,     1,     1,     1,     1,     3,     8,     8,     7,
      10,    11,     5,     7,     9,     9,     9,     6,     0,     3,
       0,     3,     0,     3,     0,     3,     0,     3,     0,     3,
       0,     3,     0,     3,     0,     3,     0,     3,     0,     3,
       0,     3,     0,     3,     0,     3,     0,     3,     0,     3,
       0,     3,     0,     3,     0,     3,     4,     4,     4,     4,
       8,     8,     8,     8,     0,     3,     0,     3,     0,     3,
       0,     3,     0,     3,     0,     3,     0,     3,     3,     6,
       9,     9,     4,     4,     4,     4,     2,     2,     2,     2,
       2,     3,     8,     8,     8,     8,     8,     8,     0,     3,
       0,     3,     0,     3,     0,     3,     4,     4,     5,     5,
       5,     5,     5,     9,     9,     9,     0,     3,     0,     3,
       0,     3,     0,     3,     0,     3,     5,     3,     7,     7,
       5,     3,     7,     3,     7,     7,     4,     1,     1,     1,
       1,     3,     1,     3,     7,     7,     3,     4,     6,     1,
       4,     1,     3,     7,     0,     1,     0,     2,     1,     1,
       0,     1,     1,     0,     1,     1,     1,     0,     1,     1,
       0,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     2,    94,    92,   188,   192,     0,     0,     0,     0,
     240,     3,     0,   240,     0,   226,   224,     4,   104,   106,
     118,   120,   138,   136,   142,   140,   134,   144,     0,   124,
       0,     0,     0,     0,    96,    98,   233,   237,   190,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    90,
       6,   112,   194,     0,     0,     0,   233,     0,     0,     0,
     174,   172,   233,     0,     0,   168,   170,   146,    73,    72,
       5,     0,     0,    75,    88,   233,    64,   226,    40,    42,
      38,    66,    67,    65,   233,   116,   114,   186,   233,   233,
     108,    74,   122,   110,     0,     0,   224,     0,   224,     0,
     224,     0,     0,    18,     0,     0,     0,     0,     0,     0,
       7,     0,     0,     0,   241,     0,     0,     0,     0,    10,
       0,     0,     0,     0,     0,    59,     0,     0,   225,     0,
       0,   224,   224,     0,   224,     0,   224,   224,   224,   224,
     224,   224,   224,     0,   224,    60,    61,     0,     0,   224,
     224,   234,   235,   236,     0,     0,   238,   239,     0,     0,
     224,     0,     0,   158,   159,   160,     0,   156,   157,     0,
       0,     0,     0,     0,     0,   224,    55,    57,     0,   234,
     235,   236,   224,    56,     0,     0,     0,     0,    71,    53,
      52,     0,     0,     0,     0,   224,     0,    51,     0,   224,
     236,   224,    58,     0,     0,     0,    70,     0,     0,     0,
     224,    68,    69,   224,   224,   224,     1,   212,    95,     0,
       0,   209,   210,    93,     0,     0,     0,   189,     0,     0,
     193,   224,     0,    12,    13,    17,     0,   224,     0,     9,
      14,    15,     8,    62,   224,     0,    16,    11,    63,   224,
       0,   227,     0,     0,     0,     0,   207,   208,   105,     0,
     107,   101,     0,   119,     0,   103,   121,     0,     0,   139,
     221,     0,   137,   143,   141,   135,   145,    47,   125,     0,
       0,    97,    99,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   191,     0,     0,     0,     0,     0,
      76,    91,   113,     0,   195,     0,    54,     0,     0,   161,
       0,   175,   173,     0,   169,     0,   171,   147,    37,     0,
      89,   148,     0,     0,   219,    41,    43,    39,   117,   115,
       0,   187,   109,   123,   111,     0,     0,     0,     0,     0,
       0,     0,     0,   127,     0,   224,   224,   129,     0,   224,
     126,     0,   224,   128,     0,   224,    23,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   154,   155,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   152,   153,   177,   176,     0,     0,    45,     0,    44,
       0,     0,     0,     0,     0,     0,     0,     0,   211,   201,
       0,     0,    31,   197,     0,    33,     0,    49,     0,   181,
       0,     0,   182,     0,     0,    48,     0,     0,    50,     0,
      30,   230,   203,     0,   213,     0,   222,     0,     0,   224,
     224,   224,    36,    82,     0,     0,     0,     0,     0,     0,
       0,    26,     0,     0,     0,     0,     0,    32,   180,    34,
       0,   216,   178,   179,     0,   196,     0,     0,    29,   206,
     240,     0,     0,   230,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   228,   229,   231,     0,   232,     0,   230,
     230,   220,   240,     0,     0,    35,     0,     0,     0,     0,
       0,     0,   224,     0,     0,     0,     0,     0,     0,     0,
      87,     0,   149,   217,     0,    46,   200,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      79,     0,     0,   224,     0,    83,     0,     0,     0,     0,
       0,    27,     0,     0,    22,     0,     0,     0,     0,   131,
       0,     0,   133,     0,   130,     0,   132,     0,    24,    25,
       0,     0,     0,     0,     0,     0,     0,     0,    77,     0,
     162,   163,    78,     0,     0,     0,   164,   165,    28,     0,
     166,   167,   218,   202,   198,   199,    85,   150,   151,    84,
      86,   204,   205,   214,   215,   223,    19,    20,    21,   183,
       0,   184,     0,   185,     0,    80,    81
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    94,   205,   203,   204,   199,   172,    96,    95,   149,
     150,   133,   135,   131,   132,   213,   215,   173,   208,   207,
     134,   136,   214,   144,   141,   138,   137,   140,   139,   142,
     195,   192,   194,   187,   186,   210,    98,   160,   100,   175,
     227,   221,   256,   257,   258,   223,   218,   263,   324,   325,
     269,   270,   271,   125,   126,   478,   155,   158,   116
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -428
static const yytype_int16 yypact[] =
{
     475,  -428,  -428,  -428,   -73,   -59,    21,    95,   -27,    22,
      59,  -428,    24,    32,    25,  -428,    12,  -428,  -428,  -428,
     -20,    -7,  -428,  -428,  -428,  -428,  -428,  -428,    33,  -428,
      18,   109,   114,   147,  -428,  -428,    23,    -3,    41,   160,
     177,   185,   192,   193,   200,   203,   204,    26,    93,  -428,
    -428,  -428,    94,   207,   206,   209,   181,   208,   211,   101,
    -428,  -428,   181,   214,   215,   104,   105,  -428,  -428,  -428,
    -428,   107,   220,  -428,   110,   190,  -428,  -428,  -428,  -428,
    -428,  -428,  -428,  -428,   181,  -428,  -428,   111,   181,   181,
    -428,  -428,  -428,  -428,   224,   226,    14,   221,    45,   225,
      45,   116,   228,  -428,   229,   230,   231,   232,   123,   234,
    -428,   235,   236,   237,  -428,   238,   227,   129,   240,  -428,
     241,   242,   246,   134,   245,  -428,   248,   137,  -428,   249,
     139,    16,    16,   142,    55,   142,    55,    20,    20,    20,
      20,    20,   140,   252,    55,  -428,  -428,   138,   143,    16,
      16,   145,   146,   148,   256,     6,  -428,  -428,     7,   258,
      45,   149,   150,  -428,  -428,  -428,   151,  -428,  -428,   153,
     262,   264,   226,   226,   265,    45,  -428,  -428,   155,  -428,
    -428,  -428,   152,  -428,   156,   266,   270,   270,  -428,  -428,
    -428,   271,   270,   272,   270,   140,   273,  -428,   274,   154,
     278,   276,  -428,    27,    27,    27,  -428,   142,   142,   277,
      45,  -428,  -428,    16,    55,    16,  -428,   168,  -428,   169,
     280,  -428,  -428,  -428,   173,   175,   179,  -428,   180,   182,
    -428,   158,   186,  -428,  -428,  -428,   187,   159,   188,  -428,
    -428,  -428,  -428,  -428,   164,   189,  -428,  -428,  -428,   165,
     191,  -428,   284,   194,   285,   195,  -428,  -428,  -428,   196,
    -428,  -428,   198,  -428,   199,  -428,  -428,   201,   283,  -428,
    -428,   202,  -428,  -428,  -428,  -428,  -428,  -428,  -428,   289,
     290,  -428,  -428,   288,   293,   294,   205,   210,   217,     0,
     212,   218,     2,   216,  -428,   298,   303,   305,   291,   222,
    -428,  -428,  -428,   233,  -428,   306,  -428,   223,   309,  -428,
     239,  -428,  -428,   243,  -428,   244,  -428,  -428,   247,   250,
    -428,  -428,   251,   312,  -428,  -428,  -428,  -428,  -428,  -428,
     253,  -428,  -428,  -428,  -428,   317,   319,   254,   320,   321,
     322,   324,   328,  -428,   255,   166,   170,  -428,   257,   171,
    -428,   259,   172,  -428,   260,   176,  -428,   334,   197,   338,
     339,   340,   341,   342,   263,   343,  -428,  -428,   267,   268,
     269,   344,     8,   345,   -37,   347,   348,   349,    -9,   350,
     351,  -428,  -428,  -428,  -428,   352,   353,  -428,   355,  -428,
     358,   359,   361,   362,   364,   368,   275,   370,  -428,  -428,
     372,   279,   282,   286,   281,  -428,   375,  -428,   287,  -428,
     292,   377,  -428,   295,   378,  -428,   299,   379,  -428,   300,
    -428,    63,  -428,   301,  -428,   302,  -428,   383,   304,   276,
     276,   178,  -428,  -428,   307,     3,   308,   384,   311,   310,
     313,  -428,   314,   385,   318,   315,   323,  -428,  -428,  -428,
     325,  -428,  -428,  -428,   157,  -428,   326,   386,  -428,  -428,
     380,   387,   390,    75,   327,   391,   392,   329,   393,   331,
     397,   332,   398,  -428,  -428,  -428,   330,  -428,   335,    82,
      88,  -428,   380,   336,   337,  -428,   346,   400,    29,   402,
     356,   357,   183,   363,   365,   367,   360,   403,   369,   371,
    -428,   404,  -428,   373,   374,  -428,  -428,   394,   417,   401,
     366,   432,   408,   436,   411,   463,   413,   472,   405,   407,
     476,   477,   478,   479,   480,   422,   426,   427,   473,   433,
    -428,   481,   482,   184,   485,  -428,   354,   434,   435,   437,
     486,  -428,   439,   449,  -428,   452,   453,   454,   455,  -428,
     425,   429,  -428,   450,  -428,   451,  -428,   456,  -428,  -428,
     459,   461,   462,   464,   465,   487,   488,   489,  -428,   490,
    -428,  -428,  -428,   492,   466,   470,  -428,  -428,  -428,   471,
    -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,
    -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,
     474,  -428,   493,  -428,   494,  -428,  -428
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,
    -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,
    -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,
    -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,
     -84,   399,  -428,   -94,   -83,  -428,  -126,  -104,  -132,    -1,
      13,  -138,   -16,   382,  -428,  -427,   -25,  -428,   -12
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -103
static const yytype_int16 yytable[] =
{
     130,   122,   222,   374,   276,   378,   488,   156,   157,   287,
     290,   433,   288,   291,   434,   127,   230,   219,   128,   255,
     128,   145,   128,   267,   101,   108,   128,   117,   123,   169,
     310,   182,   266,   151,   152,   153,   508,   188,   119,   261,
     278,   265,    97,   120,   114,   121,   301,   302,   226,   260,
     201,   128,   521,   523,   311,   312,    99,   317,   262,   206,
     314,   128,   316,   211,   212,   110,   281,   282,   111,   112,
     113,   114,   115,   473,   474,   475,   294,   476,   477,   437,
     224,   438,   228,   439,   228,   473,   474,   475,   107,   507,
     477,   304,   473,   474,   475,  -100,   520,   477,   473,   474,
     475,   103,   522,   477,   104,   105,   106,   443,  -102,   444,
     333,   445,   146,   328,   329,   259,   259,   147,   264,   375,
     264,   379,   489,   289,   292,   435,   331,   129,   264,   220,
     332,   220,   334,   259,   259,   268,   102,   109,   154,   118,
     124,   170,   323,   267,   228,   529,   128,   530,   143,   531,
     148,   272,   273,   274,   275,   306,   159,   219,   128,   228,
     128,   343,   347,   161,   128,   128,   307,   350,   353,   407,
     128,   128,   128,   409,   412,   415,   128,   128,   128,   418,
     162,   485,   128,   224,   128,   322,   535,   572,   163,   128,
     128,   179,   180,   181,   228,   164,   165,   259,   264,   259,
     179,   180,   200,   326,   327,   166,   167,   168,   171,   174,
     176,   177,   178,   183,   184,   344,   185,   189,   190,   191,
     193,   348,   196,   197,   216,   198,   209,   225,   351,   217,
     243,   229,   231,   354,   232,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,   244,   245,   246,   247,   248,
     249,   250,   251,   252,   279,   253,   254,   220,   277,   280,
     283,   284,   286,   285,   293,   295,   296,   297,   299,   298,
     300,   303,   309,   310,   305,   308,   501,   313,   315,   318,
     319,   321,   128,   330,   335,   336,   337,   356,   358,   364,
     338,   339,   366,   367,   368,   340,   384,   341,   342,   369,
     370,   381,   345,   346,   349,   352,   382,   355,   383,   387,
     357,   359,   389,   360,   361,   421,   362,   363,   396,   365,
     398,   371,   399,   401,   402,   403,   372,   404,   376,   408,
     410,   405,   380,   413,   373,   377,   416,   420,   385,   419,
     388,   422,   423,   424,   425,   426,   428,   432,   436,   386,
     440,   441,   442,   446,   447,   390,   449,   448,   450,   391,
     392,   451,   452,   393,   453,   454,   394,   455,   395,   397,
     400,   456,   406,   458,   411,   459,   414,   417,   464,   427,
     467,   469,   471,   429,   430,   431,   481,   491,   496,   503,
     505,   457,   114,   506,   510,   511,   513,   460,   461,   463,
     515,   517,   462,   528,   465,   532,   541,   544,   558,   466,
     559,   549,   468,   483,   484,   486,   470,   472,   552,   479,
     480,   554,   482,   556,   487,   565,   490,   492,   493,   566,
     567,   494,   495,   498,   497,   586,   569,   576,   577,   587,
     578,   499,   580,   500,   502,   509,   518,   512,   504,   514,
     516,   519,   581,   525,   526,   582,   583,   584,   585,   202,
     588,   589,   591,   527,   592,   593,   590,   594,   595,   601,
     524,   575,   533,   602,   603,   534,   536,   604,   540,   537,
       0,   538,     1,   539,   550,   542,     0,   543,     0,   545,
     546,     2,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,     0,
     547,    20,    21,    22,    23,    24,    25,   573,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,   548,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
     551,    57,    58,    59,   553,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,   555,    86,    87,    88,    89,    90,    91,    92,    93,
     557,   568,   560,   561,   562,   563,   564,     0,   320,   570,
     571,   574,   579,     0,     0,   596,   597,   598,   599,   600,
       0,   605,   606
};

static const yytype_int16 yycheck[] =
{
      16,    13,    96,     3,   142,     3,     3,    10,    11,     3,
       3,     3,     6,     6,     6,     3,   100,     3,     6,     3,
       6,     3,     6,     3,     3,     3,     6,     3,     3,     3,
       3,    56,   136,    10,    11,    12,   463,    62,     6,   133,
     144,   135,   115,    11,    12,    13,   172,   173,     3,   132,
      75,     6,   479,   480,   186,   187,   115,   195,     3,    84,
     192,     6,   194,    88,    89,     6,   149,   150,     9,    10,
      11,    12,    13,    10,    11,    12,   160,    14,    15,   116,
      96,   118,    98,   120,   100,    10,    11,    12,   115,    14,
      15,   175,    10,    11,    12,   115,    14,    15,    10,    11,
      12,     6,    14,    15,     9,    10,    11,   116,   115,   118,
     214,   120,     3,   207,   208,   131,   132,     3,   134,   119,
     136,   119,   119,   117,   117,   117,   210,   115,   144,   115,
     213,   115,   215,   149,   150,   115,   115,   115,   115,   115,
     115,   115,   115,     3,   160,   116,     6,   118,   115,   120,
       3,   138,   139,   140,   141,     3,   115,     3,     6,   175,
       6,     3,     3,     3,     6,     6,   182,     3,     3,     3,
       6,     6,     6,     3,     3,     3,     6,     6,     6,     3,
       3,     3,     6,   199,     6,   201,     3,     3,     3,     6,
       6,    10,    11,    12,   210,     3,     3,   213,   214,   215,
      10,    11,    12,   204,   205,     5,     3,     3,   115,   115,
       3,     5,     3,     5,     3,   231,   115,     3,     3,   115,
     115,   237,   115,     3,     0,   115,   115,     6,   244,     3,
       3,     6,   116,   249,     6,     6,     6,     6,     6,   116,
       6,     6,     6,     6,     6,   116,     6,     6,     6,     3,
     116,     6,     4,   116,   116,     6,   117,   115,     6,   116,
     115,   115,     6,   115,     6,   116,   116,   116,     6,   116,
       6,     6,     6,     3,   119,   119,   119,     6,     6,     6,
       6,     3,     6,     6,   116,   116,     6,     3,     3,     6,
     117,   116,     3,     3,     6,   116,     5,   117,   116,     6,
       6,     3,   116,   116,   116,   116,     3,   116,     3,     3,
     116,   116,     3,   117,   116,   118,   117,   116,     6,   117,
       3,   116,     3,     3,     3,     3,   116,     3,   116,   345,
     346,     3,   116,   349,   117,   117,   352,     3,   116,   355,
     117,     3,     3,     3,     3,     3,     3,     3,     3,   116,
       3,     3,     3,     3,     3,   116,     3,     5,     3,   116,
     116,     3,     3,   116,     3,     3,   116,     3,   117,   116,
     116,     3,   117,     3,   117,     3,   117,   117,     3,   116,
       3,     3,     3,   116,   116,   116,     3,     3,     3,     3,
       3,   116,    12,     3,     3,     3,     3,   118,   116,   118,
       3,     3,   116,     3,   117,     3,     3,     3,     3,   117,
       3,    10,   117,   429,   430,   431,   117,   117,    10,   118,
     118,    10,   118,    10,   117,     3,   118,   116,   118,     3,
       3,   118,   118,   118,   116,    10,     3,     3,     3,    10,
       3,   118,     3,   118,   118,   118,   116,   118,   460,   118,
     118,   116,     3,   117,   117,     3,     3,     3,     3,    77,
      10,    10,     3,   117,     3,     3,    10,     3,     3,     3,
     482,   117,   116,     3,     3,   118,   492,     3,   118,   116,
      -1,   116,     7,   116,   118,   116,    -1,   116,    -1,   116,
     116,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    -1,
     116,    36,    37,    38,    39,    40,    41,   533,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,   116,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
     118,    76,    77,    78,   118,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   118,   107,   108,   109,   110,   111,   112,   113,   114,
     118,   118,   116,   116,   116,   116,   116,    -1,   199,   118,
     118,   116,   116,    -1,    -1,   118,   118,   118,   118,   117,
      -1,   118,   118
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     7,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      36,    37,    38,    39,    40,    41,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    76,    77,    78,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   107,   108,   109,   110,
     111,   112,   113,   114,   122,   129,   128,   115,   157,   115,
     159,     3,   115,     6,     9,    10,    11,   115,     3,   115,
       6,     9,    10,    11,    12,    13,   179,     3,   115,     6,
      11,    13,   179,     3,   115,   174,   175,     3,     6,   115,
     173,   134,   135,   132,   141,   133,   142,   147,   146,   149,
     148,   145,   150,   115,   144,     3,     3,     3,     3,   130,
     131,    10,    11,    12,   115,   177,    10,    11,   178,   115,
     158,     3,     3,     3,     3,     3,     5,     3,     3,     3,
     115,   115,   127,   138,   115,   160,     3,     5,     3,    10,
      11,    12,   177,     5,     3,   115,   155,   154,   177,     3,
       3,   115,   152,   115,   153,   151,   115,     3,   115,   126,
      12,   177,   174,   124,   125,   123,   177,   140,   139,   115,
     156,   177,   177,   136,   143,   137,     0,     3,   167,     3,
     115,   162,   164,   166,   173,     6,     3,   161,   173,     6,
     161,   116,     6,     6,     6,     6,     6,   116,     6,     6,
       6,     6,     6,     3,   116,     6,     6,     6,     3,   116,
       6,     4,   116,     6,   117,     3,   163,   164,   165,   173,
     165,   164,     3,   168,   173,   164,   168,     3,   115,   171,
     172,   173,   171,   171,   171,   171,   172,     6,   168,   116,
     116,   165,   165,   115,   115,   115,     6,     3,     6,   117,
       3,     6,   117,     6,   161,   116,   116,   116,   116,     6,
       6,   167,   167,     6,   161,   119,     3,   173,   119,     6,
       3,   169,   169,     6,   169,     6,   169,   172,     6,     6,
     162,     3,   173,   115,   169,   170,   170,   170,   164,   164,
       6,   161,   165,   168,   165,   116,   116,     6,   117,   116,
     116,   117,   116,     3,   173,   116,   116,     3,   173,   116,
       3,   173,   116,     3,   173,   116,     3,   116,     3,   116,
     117,   116,   117,   116,     6,   117,     3,     3,     6,     6,
       6,   116,   116,   117,     3,   119,   116,   117,     3,   119,
     116,     3,     3,     3,     5,   116,   116,     3,   117,     3,
     116,   116,   116,   116,   116,   117,     6,   116,     3,     3,
     116,     3,     3,     3,     3,     3,   117,     3,   173,     3,
     173,   117,     3,   173,   117,     3,   173,   117,     3,   173,
       3,   118,     3,     3,     3,     3,     3,   116,     3,   116,
     116,   116,     3,     3,     6,   117,     3,   116,   118,   120,
       3,     3,     3,   116,   118,   120,     3,     3,     5,     3,
       3,     3,     3,     3,     3,     3,     3,   116,     3,     3,
     118,   116,   116,   118,     3,   117,   117,     3,   117,     3,
     117,     3,   117,    10,    11,    12,    14,    15,   176,   118,
     118,     3,   118,   173,   173,     3,   173,   117,     3,   119,
     118,     3,   116,   118,   118,   118,     3,   116,   118,   118,
     118,   119,   118,     3,   179,     3,     3,    14,   176,   118,
       3,     3,   118,     3,   118,     3,   118,     3,   116,   116,
      14,   176,    14,   176,   179,   117,   117,   117,     3,   116,
     118,   120,     3,   116,   118,     3,   173,   116,   116,   116,
     118,     3,   116,   116,     3,   116,   116,   116,   116,    10,
     118,   118,    10,   118,    10,   118,    10,   118,     3,     3,
     116,   116,   116,   116,   116,     3,     3,     3,   118,     3,
     118,   118,     3,   173,   116,   117,     3,     3,     3,   116,
       3,     3,     3,     3,     3,     3,    10,    10,    10,    10,
      10,     3,     3,     3,     3,     3,   118,   118,   118,   118,
     117,     3,     3,     3,     3,   118,   118
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


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
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
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
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
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
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

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
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
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

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
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
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

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
#line 175 "rx-parse.y"
    { as_bad (_("Unknown opcode: %s"), rx_init_start); }
    break;

  case 3:
#line 180 "rx-parse.y"
    { B1 (0x00); }
    break;

  case 4:
#line 183 "rx-parse.y"
    { B1 (0x01); }
    break;

  case 5:
#line 186 "rx-parse.y"
    { B1 (0x02); }
    break;

  case 6:
#line 189 "rx-parse.y"
    { B1 (0x03); }
    break;

  case 7:
#line 194 "rx-parse.y"
    { if (rx_disp3op ((yyvsp[(2) - (2)].exp)))
	      { B1 (0x08); rx_disp3 ((yyvsp[(2) - (2)].exp), 5); }
	    else if (rx_intop ((yyvsp[(2) - (2)].exp), 8, 8))
	      { B1 (0x2e); PC1 ((yyvsp[(2) - (2)].exp)); }
	    else if (rx_intop ((yyvsp[(2) - (2)].exp), 16, 16))
	      { B1 (0x38); PC2 ((yyvsp[(2) - (2)].exp)); }
	    else if (rx_intop ((yyvsp[(2) - (2)].exp), 24, 24))
	      { B1 (0x04); PC3 ((yyvsp[(2) - (2)].exp)); }
	    else
	      { rx_relax (RX_RELAX_BRANCH, 0);
		rx_linkrelax_branch ();
		/* We'll convert this to a longer one later if needed.  */
		B1 (0x08); rx_disp3 ((yyvsp[(2) - (2)].exp), 5); } }
    break;

  case 8:
#line 209 "rx-parse.y"
    { B1 (0x04); PC3 ((yyvsp[(3) - (3)].exp)); }
    break;

  case 9:
#line 212 "rx-parse.y"
    { B1 (0x08); rx_disp3 ((yyvsp[(3) - (3)].exp), 5); }
    break;

  case 10:
#line 217 "rx-parse.y"
    { if (rx_intop ((yyvsp[(2) - (2)].exp), 16, 16))
	      { B1 (0x39); PC2 ((yyvsp[(2) - (2)].exp)); }
	    else if (rx_intop ((yyvsp[(2) - (2)].exp), 24, 24))
	      { B1 (0x05); PC3 ((yyvsp[(2) - (2)].exp)); }
	    else
	      { rx_relax (RX_RELAX_BRANCH, 0);
		rx_linkrelax_branch ();
		B1 (0x39); PC2 ((yyvsp[(2) - (2)].exp)); } }
    break;

  case 11:
#line 226 "rx-parse.y"
    { B1 (0x05), PC3 ((yyvsp[(3) - (3)].exp)); }
    break;

  case 12:
#line 231 "rx-parse.y"
    { if ((yyvsp[(1) - (3)].regno) == COND_EQ || (yyvsp[(1) - (3)].regno) == COND_NE)
	      { B1 ((yyvsp[(1) - (3)].regno) == COND_EQ ? 0x10 : 0x18); rx_disp3 ((yyvsp[(3) - (3)].exp), 5); }
	    else
	      as_bad (_("Only BEQ and BNE may have .S")); }
    break;

  case 13:
#line 239 "rx-parse.y"
    { B1 (0x20); F ((yyvsp[(1) - (3)].regno), 4, 4); PC1 ((yyvsp[(3) - (3)].exp)); }
    break;

  case 14:
#line 242 "rx-parse.y"
    { B1 (0x2e), PC1 ((yyvsp[(3) - (3)].exp)); }
    break;

  case 15:
#line 247 "rx-parse.y"
    { B1 (0x38), PC2 ((yyvsp[(3) - (3)].exp)); }
    break;

  case 16:
#line 249 "rx-parse.y"
    { B1 (0x39), PC2 ((yyvsp[(3) - (3)].exp)); }
    break;

  case 17:
#line 251 "rx-parse.y"
    { if ((yyvsp[(1) - (3)].regno) == COND_EQ || (yyvsp[(1) - (3)].regno) == COND_NE)
	      { B1 ((yyvsp[(1) - (3)].regno) == COND_EQ ? 0x3a : 0x3b); PC2 ((yyvsp[(3) - (3)].exp)); }
	    else
	      as_bad (_("Only BEQ and BNE may have .W")); }
    break;

  case 18:
#line 256 "rx-parse.y"
    { if ((yyvsp[(1) - (2)].regno) == COND_EQ || (yyvsp[(1) - (2)].regno) == COND_NE)
	      {
		rx_relax (RX_RELAX_BRANCH, 0);
		rx_linkrelax_branch ();
		B1 ((yyvsp[(1) - (2)].regno) == COND_EQ ? 0x10 : 0x18); rx_disp3 ((yyvsp[(2) - (2)].exp), 5);
	      }
	    else
	      {
		rx_relax (RX_RELAX_BRANCH, 0);
		/* This is because we might turn it into a
		   jump-over-jump long branch.  */
		rx_linkrelax_branch ();
	        B1 (0x20); F ((yyvsp[(1) - (2)].regno), 4, 4); PC1 ((yyvsp[(2) - (2)].exp));
	      } }
    break;

  case 19:
#line 275 "rx-parse.y"
    { if ((yyvsp[(8) - (9)].regno) <= 7 && rx_uintop ((yyvsp[(4) - (9)].exp), 8) && rx_disp5op0 (&(yyvsp[(6) - (9)].exp), BSIZE))
	      { B2 (0x3c, 0); rx_field5s2 ((yyvsp[(6) - (9)].exp)); F ((yyvsp[(8) - (9)].regno), 9, 3); O1 ((yyvsp[(4) - (9)].exp)); }
	    else
	      { B2 (0xf8, 0x04); F ((yyvsp[(8) - (9)].regno), 8, 4); DSP ((yyvsp[(6) - (9)].exp), 6, BSIZE); O1 ((yyvsp[(4) - (9)].exp));
	      if ((yyvsp[(4) - (9)].exp).X_op != O_constant && (yyvsp[(4) - (9)].exp).X_op != O_big) rx_linkrelax_imm (12); } }
    break;

  case 20:
#line 282 "rx-parse.y"
    { if ((yyvsp[(8) - (9)].regno) <= 7 && rx_uintop ((yyvsp[(4) - (9)].exp), 8) && rx_disp5op0 (&(yyvsp[(6) - (9)].exp), WSIZE))
	      { B2 (0x3d, 0); rx_field5s2 ((yyvsp[(6) - (9)].exp)); F ((yyvsp[(8) - (9)].regno), 9, 3); O1 ((yyvsp[(4) - (9)].exp)); }
	    else
	      { B2 (0xf8, 0x01); F ((yyvsp[(8) - (9)].regno), 8, 4); DSP ((yyvsp[(6) - (9)].exp), 6, WSIZE); IMMW ((yyvsp[(4) - (9)].exp), 12); } }
    break;

  case 21:
#line 288 "rx-parse.y"
    { if ((yyvsp[(8) - (9)].regno) <= 7 && rx_uintop ((yyvsp[(4) - (9)].exp), 8) && rx_disp5op0 (&(yyvsp[(6) - (9)].exp), LSIZE))
	      { B2 (0x3e, 0); rx_field5s2 ((yyvsp[(6) - (9)].exp)); F ((yyvsp[(8) - (9)].regno), 9, 3); O1 ((yyvsp[(4) - (9)].exp)); }
	    else
	      { B2 (0xf8, 0x02); F ((yyvsp[(8) - (9)].regno), 8, 4); DSP ((yyvsp[(6) - (9)].exp), 6, LSIZE); IMM ((yyvsp[(4) - (9)].exp), 12); } }
    break;

  case 22:
#line 296 "rx-parse.y"
    { B2 (0x3f, 0); F ((yyvsp[(5) - (7)].regno), 8, 4); F ((yyvsp[(7) - (7)].regno), 12, 4); rtsd_immediate ((yyvsp[(3) - (7)].exp));
	    if ((yyvsp[(5) - (7)].regno) == 0)
	      rx_error (_("RTSD cannot pop R0"));
	    if ((yyvsp[(5) - (7)].regno) > (yyvsp[(7) - (7)].regno))
	      rx_error (_("RTSD first reg must be <= second reg")); }
    break;

  case 23:
#line 305 "rx-parse.y"
    { B2 (0x47, 0); F ((yyvsp[(2) - (4)].regno), 8, 4); F ((yyvsp[(4) - (4)].regno), 12, 4); }
    break;

  case 24:
#line 310 "rx-parse.y"
    { B2 (0x44, 0); F ((yyvsp[(4) - (8)].regno), 8, 4); F ((yyvsp[(8) - (8)].regno), 12, 4); DSP ((yyvsp[(2) - (8)].exp), 6, BSIZE); }
    break;

  case 25:
#line 313 "rx-parse.y"
    { B3 (MEMEX, 0x04, 0); F ((yyvsp[(6) - (8)].regno), 8, 2);  F ((yyvsp[(4) - (8)].regno), 16, 4); F ((yyvsp[(8) - (8)].regno), 20, 4); DSP ((yyvsp[(2) - (8)].exp), 14, sizemap[(yyvsp[(6) - (8)].regno)]); }
    break;

  case 26:
#line 318 "rx-parse.y"
    { B2 (0x5b, 0x00); F ((yyvsp[(2) - (5)].regno), 5, 1); F ((yyvsp[(3) - (5)].regno), 8, 4); F ((yyvsp[(5) - (5)].regno), 12, 4); }
    break;

  case 27:
#line 323 "rx-parse.y"
    { B2 (0x58, 0x00); F ((yyvsp[(2) - (7)].regno), 5, 1); F ((yyvsp[(4) - (7)].regno), 8, 4); F ((yyvsp[(7) - (7)].regno), 12, 4); }
    break;

  case 28:
#line 326 "rx-parse.y"
    { if ((yyvsp[(5) - (8)].regno) <= 7 && (yyvsp[(8) - (8)].regno) <= 7 && rx_disp5op (&(yyvsp[(3) - (8)].exp), (yyvsp[(2) - (8)].regno)))
	      { B2 (0xb0, 0); F ((yyvsp[(2) - (8)].regno), 4, 1); F ((yyvsp[(5) - (8)].regno), 9, 3); F ((yyvsp[(8) - (8)].regno), 13, 3); rx_field5s ((yyvsp[(3) - (8)].exp)); }
	    else
	      { B2 (0x58, 0x00); F ((yyvsp[(2) - (8)].regno), 5, 1); F ((yyvsp[(5) - (8)].regno), 8, 4); F ((yyvsp[(8) - (8)].regno), 12, 4); DSP ((yyvsp[(3) - (8)].exp), 6, (yyvsp[(2) - (8)].regno)); } }
    break;

  case 29:
#line 334 "rx-parse.y"
    { if (rx_uintop ((yyvsp[(3) - (5)].exp), 4))
	      { B2 (0x60, 0); FE ((yyvsp[(3) - (5)].exp), 8, 4); F ((yyvsp[(5) - (5)].regno), 12, 4); }
	    else
	      /* This is really an add, but we negate the immediate.  */
	      { B2 (0x70, 0); F ((yyvsp[(5) - (5)].regno), 8, 4); F ((yyvsp[(5) - (5)].regno), 12, 4); NIMM ((yyvsp[(3) - (5)].exp), 6); } }
    break;

  case 30:
#line 341 "rx-parse.y"
    { if (rx_uintop ((yyvsp[(3) - (5)].exp), 4))
	      { B2 (0x61, 0); FE ((yyvsp[(3) - (5)].exp), 8, 4); F ((yyvsp[(5) - (5)].regno), 12, 4); }
	    else if (rx_uintop ((yyvsp[(3) - (5)].exp), 8))
	      { B2 (0x75, 0x50); F ((yyvsp[(5) - (5)].regno), 12, 4); UO1 ((yyvsp[(3) - (5)].exp)); }
	    else
	      { B2 (0x74, 0x00); F ((yyvsp[(5) - (5)].regno), 12, 4); IMM ((yyvsp[(3) - (5)].exp), 6); } }
    break;

  case 31:
#line 349 "rx-parse.y"
    { if (rx_uintop ((yyvsp[(3) - (5)].exp), 4))
	      { B2 (0x62, 0); FE ((yyvsp[(3) - (5)].exp), 8, 4); F ((yyvsp[(5) - (5)].regno), 12, 4); }
	    else
	      { B2 (0x70, 0); F ((yyvsp[(5) - (5)].regno), 8, 4); F ((yyvsp[(5) - (5)].regno), 12, 4); IMM ((yyvsp[(3) - (5)].exp), 6); } }
    break;

  case 32:
#line 355 "rx-parse.y"
    { if (rx_uintop ((yyvsp[(3) - (5)].exp), 4))
	      { B2 (0x63, 0); FE ((yyvsp[(3) - (5)].exp), 8, 4); F ((yyvsp[(5) - (5)].regno), 12, 4); }
	    else
	      { B2 (0x74, 0x10); F ((yyvsp[(5) - (5)].regno), 12, 4); IMM ((yyvsp[(3) - (5)].exp), 6); } }
    break;

  case 33:
#line 361 "rx-parse.y"
    { if (rx_uintop ((yyvsp[(3) - (5)].exp), 4))
	      { B2 (0x64, 0); FE ((yyvsp[(3) - (5)].exp), 8, 4); F ((yyvsp[(5) - (5)].regno), 12, 4); }
	    else
	      { B2 (0x74, 0x20); F ((yyvsp[(5) - (5)].regno), 12, 4); IMM ((yyvsp[(3) - (5)].exp), 6); } }
    break;

  case 34:
#line 367 "rx-parse.y"
    { if (rx_uintop ((yyvsp[(3) - (5)].exp), 4))
	      { B2 (0x65, 0); FE ((yyvsp[(3) - (5)].exp), 8, 4); F ((yyvsp[(5) - (5)].regno), 12, 4); }
	    else
	      { B2 (0x74, 0x30); F ((yyvsp[(5) - (5)].regno), 12, 4); IMM ((yyvsp[(3) - (5)].exp), 6); } }
    break;

  case 35:
#line 373 "rx-parse.y"
    { if (rx_uintop ((yyvsp[(4) - (6)].exp), 4))
	      { B2 (0x66, 0); FE ((yyvsp[(4) - (6)].exp), 8, 4); F ((yyvsp[(6) - (6)].regno), 12, 4); }
	    else if (rx_uintop ((yyvsp[(4) - (6)].exp), 8))
	      { B2 (0x75, 0x40); F ((yyvsp[(6) - (6)].regno), 12, 4); UO1 ((yyvsp[(4) - (6)].exp)); }
	    else
	      { B2 (0xfb, 0x02); F ((yyvsp[(6) - (6)].regno), 8, 4); IMM ((yyvsp[(4) - (6)].exp), 12); } }
    break;

  case 36:
#line 381 "rx-parse.y"
    { if (rx_uintop ((yyvsp[(3) - (5)].exp), 4))
	      { B2 (0x66, 0); FE ((yyvsp[(3) - (5)].exp), 8, 4); F ((yyvsp[(5) - (5)].regno), 12, 4); }
	    else if (rx_uintop ((yyvsp[(3) - (5)].exp), 8))
	      { B2 (0x75, 0x40); F ((yyvsp[(5) - (5)].regno), 12, 4); UO1 ((yyvsp[(3) - (5)].exp)); }
	    else
	      { B2 (0xfb, 0x02); F ((yyvsp[(5) - (5)].regno), 8, 4); IMM ((yyvsp[(3) - (5)].exp), 12); } }
    break;

  case 37:
#line 391 "rx-parse.y"
    { B1 (0x67); rtsd_immediate ((yyvsp[(3) - (3)].exp)); }
    break;

  case 38:
#line 395 "rx-parse.y"
    { sub_op = 0; }
    break;

  case 40:
#line 396 "rx-parse.y"
    { sub_op = 1; }
    break;

  case 42:
#line 397 "rx-parse.y"
    { sub_op = 2; }
    break;

  case 44:
#line 402 "rx-parse.y"
    {
	    if ((yyvsp[(2) - (4)].regno) == (yyvsp[(4) - (4)].regno))
	      { B2 (0x7e, 0x80); F (LSIZE, 10, 2); F ((yyvsp[(2) - (4)].regno), 12, 4); }
	    else
	     { B2 (0x6e, 0); F ((yyvsp[(2) - (4)].regno), 8, 4); F ((yyvsp[(4) - (4)].regno), 12, 4); }
	    if ((yyvsp[(2) - (4)].regno) == 0)
	      rx_error (_("PUSHM cannot push R0"));
	    if ((yyvsp[(2) - (4)].regno) > (yyvsp[(4) - (4)].regno))
	      rx_error (_("PUSHM first reg must be <= second reg")); }
    break;

  case 45:
#line 415 "rx-parse.y"
    {
	    if ((yyvsp[(2) - (4)].regno) == (yyvsp[(4) - (4)].regno))
	      { B2 (0x7e, 0xb0); F ((yyvsp[(2) - (4)].regno), 12, 4); }
	    else
	      { B2 (0x6f, 0); F ((yyvsp[(2) - (4)].regno), 8, 4); F ((yyvsp[(4) - (4)].regno), 12, 4); }
	    if ((yyvsp[(2) - (4)].regno) == 0)
	      rx_error (_("POPM cannot pop R0"));
	    if ((yyvsp[(2) - (4)].regno) > (yyvsp[(4) - (4)].regno))
	      rx_error (_("POPM first reg must be <= second reg")); }
    break;

  case 46:
#line 428 "rx-parse.y"
    { B2 (0x70, 0x00); F ((yyvsp[(5) - (7)].regno), 8, 4); F ((yyvsp[(7) - (7)].regno), 12, 4); IMM ((yyvsp[(3) - (7)].exp), 6); }
    break;

  case 47:
#line 433 "rx-parse.y"
    { B2(0x75, 0x60), UO1 ((yyvsp[(3) - (3)].exp)); }
    break;

  case 48:
#line 438 "rx-parse.y"
    { B2 (0x78, 0); FE ((yyvsp[(3) - (5)].exp), 7, 5); F ((yyvsp[(5) - (5)].regno), 12, 4); }
    break;

  case 49:
#line 440 "rx-parse.y"
    { B2 (0x7a, 0); FE ((yyvsp[(3) - (5)].exp), 7, 5); F ((yyvsp[(5) - (5)].regno), 12, 4); }
    break;

  case 50:
#line 445 "rx-parse.y"
    { B2 (0x7c, 0x00); FE ((yyvsp[(3) - (5)].exp), 7, 5); F ((yyvsp[(5) - (5)].regno), 12, 4); }
    break;

  case 51:
#line 450 "rx-parse.y"
    { B2 (0x7e, 0x30); F ((yyvsp[(2) - (2)].regno), 12, 4); }
    break;

  case 52:
#line 452 "rx-parse.y"
    { B2 (0x7e, 0x40); F ((yyvsp[(2) - (2)].regno), 12, 4); }
    break;

  case 53:
#line 454 "rx-parse.y"
    { B2 (0x7e, 0x50); F ((yyvsp[(2) - (2)].regno), 12, 4); }
    break;

  case 54:
#line 459 "rx-parse.y"
    { B2 (0x7e, 0x80); F ((yyvsp[(2) - (3)].regno), 10, 2); F ((yyvsp[(3) - (3)].regno), 12, 4); }
    break;

  case 55:
#line 464 "rx-parse.y"
    { B2 (0x7e, 0xb0); F ((yyvsp[(2) - (2)].regno), 12, 4); }
    break;

  case 56:
#line 469 "rx-parse.y"
    { if ((yyvsp[(2) - (2)].regno) < 16)
	      { B2 (0x7e, 0xc0); F ((yyvsp[(2) - (2)].regno), 12, 4); }
	    else
	      as_bad (_("PUSHC can only push the first 16 control registers")); }
    break;

  case 57:
#line 477 "rx-parse.y"
    { if ((yyvsp[(2) - (2)].regno) < 16)
	      { B2 (0x7e, 0xe0); F ((yyvsp[(2) - (2)].regno), 12, 4); }
	    else
	      as_bad (_("POPC can only pop the first 16 control registers")); }
    break;

  case 58:
#line 485 "rx-parse.y"
    { B2 (0x7f, 0xa0); F ((yyvsp[(2) - (2)].regno), 12, 4); }
    break;

  case 59:
#line 487 "rx-parse.y"
    { B2 (0x7f, 0xb0); F ((yyvsp[(2) - (2)].regno), 12, 4); }
    break;

  case 60:
#line 492 "rx-parse.y"
    { B2 (0x7f, 0x00); F ((yyvsp[(2) - (2)].regno), 12, 4); }
    break;

  case 61:
#line 494 "rx-parse.y"
    { B2 (0x7f, 0x10); F ((yyvsp[(2) - (2)].regno), 12, 4); }
    break;

  case 62:
#line 496 "rx-parse.y"
    { B2 (0x7f, 0x40); F ((yyvsp[(3) - (3)].regno), 12, 4); }
    break;

  case 63:
#line 498 "rx-parse.y"
    { B2 (0x7f, 0x50); F ((yyvsp[(3) - (3)].regno), 12, 4); }
    break;

  case 64:
#line 503 "rx-parse.y"
    { B2 (0x7f, 0x83); }
    break;

  case 65:
#line 505 "rx-parse.y"
    { B2 (0x7f, 0x87); }
    break;

  case 66:
#line 507 "rx-parse.y"
    { B2 (0x7f, 0x8b); }
    break;

  case 67:
#line 509 "rx-parse.y"
    { B2 (0x7f, 0x8f); }
    break;

  case 68:
#line 514 "rx-parse.y"
    { B2 (0x7f, 0x80); F ((yyvsp[(2) - (2)].regno), 14, 2); }
    break;

  case 69:
#line 516 "rx-parse.y"
    { B2 (0x7f, 0x84); F ((yyvsp[(2) - (2)].regno), 14, 2); }
    break;

  case 70:
#line 518 "rx-parse.y"
    { B2 (0x7f, 0x88); F ((yyvsp[(2) - (2)].regno), 14, 2); }
    break;

  case 71:
#line 523 "rx-parse.y"
    { B2 (0x7f, 0x8c); F ((yyvsp[(2) - (2)].regno), 14, 2); }
    break;

  case 72:
#line 528 "rx-parse.y"
    { B2 (0x7f, 0x94); }
    break;

  case 73:
#line 530 "rx-parse.y"
    { B2 (0x7f, 0x95); }
    break;

  case 74:
#line 532 "rx-parse.y"
    { B2 (0x7f, 0x96); }
    break;

  case 75:
#line 534 "rx-parse.y"
    { B2 (0x7f, 0x93); }
    break;

  case 76:
#line 539 "rx-parse.y"
    { B3 (0x75, 0x70, 0x00); FE ((yyvsp[(3) - (3)].exp), 20, 4); }
    break;

  case 77:
#line 545 "rx-parse.y"
    { if ((yyvsp[(3) - (8)].regno) <= 7 && (yyvsp[(7) - (8)].regno) <= 7 && rx_disp5op (&(yyvsp[(5) - (8)].exp), (yyvsp[(2) - (8)].regno)))
	      { B2 (0x80, 0); F ((yyvsp[(2) - (8)].regno), 2, 2); F ((yyvsp[(7) - (8)].regno), 9, 3); F ((yyvsp[(3) - (8)].regno), 13, 3); rx_field5s ((yyvsp[(5) - (8)].exp)); }
	    else
	      { B2 (0xc3, 0x00); F ((yyvsp[(2) - (8)].regno), 2, 2); F ((yyvsp[(7) - (8)].regno), 8, 4); F ((yyvsp[(3) - (8)].regno), 12, 4); DSP ((yyvsp[(5) - (8)].exp), 4, (yyvsp[(2) - (8)].regno)); }}
    break;

  case 78:
#line 553 "rx-parse.y"
    { if ((yyvsp[(5) - (8)].regno) <= 7 && (yyvsp[(8) - (8)].regno) <= 7 && rx_disp5op (&(yyvsp[(3) - (8)].exp), (yyvsp[(2) - (8)].regno)))
	      { B2 (0x88, 0); F ((yyvsp[(2) - (8)].regno), 2, 2); F ((yyvsp[(5) - (8)].regno), 9, 3); F ((yyvsp[(8) - (8)].regno), 13, 3); rx_field5s ((yyvsp[(3) - (8)].exp)); }
	    else
	      { B2 (0xcc, 0x00); F ((yyvsp[(2) - (8)].regno), 2, 2); F ((yyvsp[(5) - (8)].regno), 8, 4); F ((yyvsp[(8) - (8)].regno), 12, 4); DSP ((yyvsp[(3) - (8)].exp), 6, (yyvsp[(2) - (8)].regno)); } }
    break;

  case 79:
#line 567 "rx-parse.y"
    { B2 (0xc3, 0x00); F ((yyvsp[(2) - (7)].regno), 2, 2); F ((yyvsp[(6) - (7)].regno), 8, 4); F ((yyvsp[(3) - (7)].regno), 12, 4); }
    break;

  case 80:
#line 572 "rx-parse.y"
    { B2 (0xc0, 0); F ((yyvsp[(2) - (10)].regno), 2, 2); F ((yyvsp[(4) - (10)].regno), 8, 4); F ((yyvsp[(9) - (10)].regno), 12, 4); DSP ((yyvsp[(7) - (10)].exp), 4, (yyvsp[(2) - (10)].regno)); }
    break;

  case 81:
#line 577 "rx-parse.y"
    { B2 (0xc0, 0x00); F ((yyvsp[(2) - (11)].regno), 2, 2); F ((yyvsp[(5) - (11)].regno), 8, 4); F ((yyvsp[(10) - (11)].regno), 12, 4); DSP ((yyvsp[(3) - (11)].exp), 6, (yyvsp[(2) - (11)].regno)); DSP ((yyvsp[(8) - (11)].exp), 4, (yyvsp[(2) - (11)].regno)); }
    break;

  case 82:
#line 582 "rx-parse.y"
    { B2 (0xcf, 0x00); F ((yyvsp[(2) - (5)].regno), 2, 2); F ((yyvsp[(3) - (5)].regno), 8, 4); F ((yyvsp[(5) - (5)].regno), 12, 4); }
    break;

  case 83:
#line 587 "rx-parse.y"
    { B2 (0xcc, 0x00); F ((yyvsp[(2) - (7)].regno), 2, 2); F ((yyvsp[(4) - (7)].regno), 8, 4); F ((yyvsp[(7) - (7)].regno), 12, 4); }
    break;

  case 84:
#line 592 "rx-parse.y"
    { B2 (0xf0, 0x00); F ((yyvsp[(7) - (9)].regno), 8, 4); FE ((yyvsp[(3) - (9)].exp), 13, 3); DSP ((yyvsp[(5) - (9)].exp), 6, BSIZE); }
    break;

  case 85:
#line 594 "rx-parse.y"
    { B2 (0xf0, 0x08); F ((yyvsp[(7) - (9)].regno), 8, 4); FE ((yyvsp[(3) - (9)].exp), 13, 3); DSP ((yyvsp[(5) - (9)].exp), 6, BSIZE); }
    break;

  case 86:
#line 596 "rx-parse.y"
    { B2 (0xf4, 0x00); F ((yyvsp[(7) - (9)].regno), 8, 4); FE ((yyvsp[(3) - (9)].exp), 13, 3); DSP ((yyvsp[(5) - (9)].exp), 6, BSIZE); }
    break;

  case 87:
#line 601 "rx-parse.y"
    { B2 (0xf4, 0x08); F ((yyvsp[(2) - (6)].regno), 14, 2); F ((yyvsp[(5) - (6)].regno), 8, 4); DSP ((yyvsp[(3) - (6)].exp), 6, (yyvsp[(2) - (6)].regno)); }
    break;

  case 88:
#line 605 "rx-parse.y"
    { sub_op = 0; }
    break;

  case 90:
#line 606 "rx-parse.y"
    { sub_op = 1; sub_op2 = 1; }
    break;

  case 92:
#line 607 "rx-parse.y"
    { sub_op = 2; }
    break;

  case 94:
#line 608 "rx-parse.y"
    { sub_op = 3; sub_op2 = 2; }
    break;

  case 96:
#line 609 "rx-parse.y"
    { sub_op = 4; }
    break;

  case 98:
#line 610 "rx-parse.y"
    { sub_op = 5; }
    break;

  case 100:
#line 611 "rx-parse.y"
    { sub_op = 6; }
    break;

  case 102:
#line 612 "rx-parse.y"
    { sub_op = 7; }
    break;

  case 104:
#line 613 "rx-parse.y"
    { sub_op = 8; }
    break;

  case 106:
#line 614 "rx-parse.y"
    { sub_op = 9; }
    break;

  case 108:
#line 615 "rx-parse.y"
    { sub_op = 12; }
    break;

  case 110:
#line 616 "rx-parse.y"
    { sub_op = 13; }
    break;

  case 112:
#line 617 "rx-parse.y"
    { sub_op = 14; sub_op2 = 0; }
    break;

  case 114:
#line 618 "rx-parse.y"
    { sub_op = 14; }
    break;

  case 116:
#line 619 "rx-parse.y"
    { sub_op = 15; }
    break;

  case 118:
#line 623 "rx-parse.y"
    { sub_op = 6; }
    break;

  case 120:
#line 624 "rx-parse.y"
    { sub_op = 7; }
    break;

  case 122:
#line 625 "rx-parse.y"
    { sub_op = 16; }
    break;

  case 124:
#line 626 "rx-parse.y"
    { sub_op = 17; }
    break;

  case 126:
#line 631 "rx-parse.y"
    { id24 (1, 0x63, 0x00); F ((yyvsp[(4) - (4)].regno), 16, 4); F ((yyvsp[(2) - (4)].regno), 20, 4); }
    break;

  case 127:
#line 633 "rx-parse.y"
    { id24 (1, 0x67, 0x00); F ((yyvsp[(4) - (4)].regno), 16, 4); F ((yyvsp[(2) - (4)].regno), 20, 4); }
    break;

  case 128:
#line 635 "rx-parse.y"
    { id24 (1, 0x6b, 0x00); F ((yyvsp[(4) - (4)].regno), 16, 4); F ((yyvsp[(2) - (4)].regno), 20, 4); }
    break;

  case 129:
#line 637 "rx-parse.y"
    { id24 (1, 0x6f, 0x00); F ((yyvsp[(4) - (4)].regno), 16, 4); F ((yyvsp[(2) - (4)].regno), 20, 4); }
    break;

  case 130:
#line 640 "rx-parse.y"
    { id24 (1, 0x60, 0x00); F ((yyvsp[(6) - (8)].regno), 16, 4); F ((yyvsp[(2) - (8)].regno), 20, 4); DSP ((yyvsp[(4) - (8)].exp), 14, BSIZE); }
    break;

  case 131:
#line 642 "rx-parse.y"
    { id24 (1, 0x64, 0x00); F ((yyvsp[(6) - (8)].regno), 16, 4); F ((yyvsp[(2) - (8)].regno), 20, 4); DSP ((yyvsp[(4) - (8)].exp), 14, BSIZE); }
    break;

  case 132:
#line 644 "rx-parse.y"
    { id24 (1, 0x68, 0x00); F ((yyvsp[(6) - (8)].regno), 16, 4); F ((yyvsp[(2) - (8)].regno), 20, 4); DSP ((yyvsp[(4) - (8)].exp), 14, BSIZE); }
    break;

  case 133:
#line 646 "rx-parse.y"
    { id24 (1, 0x6c, 0x00); F ((yyvsp[(6) - (8)].regno), 16, 4); F ((yyvsp[(2) - (8)].regno), 20, 4); DSP ((yyvsp[(4) - (8)].exp), 14, BSIZE); }
    break;

  case 134:
#line 650 "rx-parse.y"
    { sub_op = 0; }
    break;

  case 136:
#line 651 "rx-parse.y"
    { sub_op = 1; }
    break;

  case 138:
#line 652 "rx-parse.y"
    { sub_op = 2; }
    break;

  case 140:
#line 653 "rx-parse.y"
    { sub_op = 3; }
    break;

  case 142:
#line 654 "rx-parse.y"
    { sub_op = 4; }
    break;

  case 144:
#line 655 "rx-parse.y"
    { sub_op = 5; }
    break;

  case 146:
#line 656 "rx-parse.y"
    { sub_op = 6; }
    break;

  case 148:
#line 661 "rx-parse.y"
    { id24 (1, 0xdb, 0x00); F ((yyvsp[(1) - (3)].regno), 20, 4); F ((yyvsp[(3) - (3)].regno), 16, 4); }
    break;

  case 149:
#line 663 "rx-parse.y"
    { id24 (1, 0xd0, 0x00); F ((yyvsp[(1) - (6)].regno), 20, 4); F ((yyvsp[(2) - (6)].regno), 12, 2); F ((yyvsp[(5) - (6)].regno), 16, 4); DSP ((yyvsp[(3) - (6)].exp), 14, (yyvsp[(2) - (6)].regno)); }
    break;

  case 150:
#line 668 "rx-parse.y"
    { id24 (1, 0xe0, 0x00); F ((yyvsp[(1) - (9)].regno), 20, 4); FE ((yyvsp[(3) - (9)].exp), 11, 3);
	      F ((yyvsp[(7) - (9)].regno), 16, 4); DSP ((yyvsp[(5) - (9)].exp), 14, BSIZE); }
    break;

  case 151:
#line 674 "rx-parse.y"
    { id24 (1, 0xe0, 0x0f); FE ((yyvsp[(3) - (9)].exp), 11, 3); F ((yyvsp[(7) - (9)].regno), 16, 4);
	      DSP ((yyvsp[(5) - (9)].exp), 14, BSIZE); }
    break;

  case 152:
#line 680 "rx-parse.y"
    { id24 (2, 0x00, 0x00); F ((yyvsp[(2) - (4)].regno), 16, 4); F ((yyvsp[(4) - (4)].regno), 20, 4); }
    break;

  case 153:
#line 682 "rx-parse.y"
    { id24 (2, 0x01, 0x00); F ((yyvsp[(2) - (4)].regno), 16, 4); F ((yyvsp[(4) - (4)].regno), 20, 4); }
    break;

  case 154:
#line 684 "rx-parse.y"
    { id24 (2, 0x04, 0x00); F ((yyvsp[(2) - (4)].regno), 16, 4); F ((yyvsp[(4) - (4)].regno), 20, 4); }
    break;

  case 155:
#line 686 "rx-parse.y"
    { id24 (2, 0x05, 0x00); F ((yyvsp[(2) - (4)].regno), 16, 4); F ((yyvsp[(4) - (4)].regno), 20, 4); }
    break;

  case 156:
#line 692 "rx-parse.y"
    { id24 (2, 0x17, 0x00); F ((yyvsp[(2) - (2)].regno), 20, 4); }
    break;

  case 157:
#line 694 "rx-parse.y"
    { id24 (2, 0x17, 0x10); F ((yyvsp[(2) - (2)].regno), 20, 4); }
    break;

  case 158:
#line 696 "rx-parse.y"
    { id24 (2, 0x1f, 0x00); F ((yyvsp[(2) - (2)].regno), 20, 4); }
    break;

  case 159:
#line 698 "rx-parse.y"
    { id24 (2, 0x1f, 0x20); F ((yyvsp[(2) - (2)].regno), 20, 4); }
    break;

  case 160:
#line 700 "rx-parse.y"
    { id24 (2, 0x1f, 0x10); F ((yyvsp[(2) - (2)].regno), 20, 4); }
    break;

  case 161:
#line 703 "rx-parse.y"
    { id24 (2, 0x18, 0x00);
	    if (rx_uintop ((yyvsp[(3) - (3)].exp), 4) && (yyvsp[(3) - (3)].exp).X_add_number == 1)
	      ;
	    else if (rx_uintop ((yyvsp[(3) - (3)].exp), 4) && (yyvsp[(3) - (3)].exp).X_add_number == 2)
	      F (1, 19, 1);
	    else
	      as_bad (_("RACW expects #1 or #2"));}
    break;

  case 162:
#line 714 "rx-parse.y"
    { id24 (2, 0x20, 0); F ((yyvsp[(2) - (8)].regno), 14, 2); F ((yyvsp[(6) - (8)].regno), 16, 4); F ((yyvsp[(3) - (8)].regno), 20, 4); }
    break;

  case 163:
#line 716 "rx-parse.y"
    { id24 (2, 0x24, 0); F ((yyvsp[(2) - (8)].regno), 14, 2); F ((yyvsp[(7) - (8)].regno), 16, 4); F ((yyvsp[(3) - (8)].regno), 20, 4); }
    break;

  case 164:
#line 721 "rx-parse.y"
    { id24 (2, 0x28, 0); F ((yyvsp[(2) - (8)].regno), 14, 2); F ((yyvsp[(4) - (8)].regno), 16, 4); F ((yyvsp[(8) - (8)].regno), 20, 4); }
    break;

  case 165:
#line 723 "rx-parse.y"
    { id24 (2, 0x2c, 0); F ((yyvsp[(2) - (8)].regno), 14, 2); F ((yyvsp[(5) - (8)].regno), 16, 4); F ((yyvsp[(8) - (8)].regno), 20, 4); }
    break;

  case 166:
#line 728 "rx-parse.y"
    { id24 (2, 0x38, 0); F ((yyvsp[(2) - (8)].regno), 15, 1); F ((yyvsp[(4) - (8)].regno), 16, 4); F ((yyvsp[(8) - (8)].regno), 20, 4); }
    break;

  case 167:
#line 730 "rx-parse.y"
    { id24 (2, 0x3c, 0); F ((yyvsp[(2) - (8)].regno), 15, 1); F ((yyvsp[(5) - (8)].regno), 16, 4); F ((yyvsp[(8) - (8)].regno), 20, 4); }
    break;

  case 168:
#line 734 "rx-parse.y"
    { sub_op = 6; }
    break;

  case 170:
#line 735 "rx-parse.y"
    { sub_op = 4; }
    break;

  case 172:
#line 736 "rx-parse.y"
    { sub_op = 5; }
    break;

  case 174:
#line 737 "rx-parse.y"
    { sub_op = 7; }
    break;

  case 176:
#line 742 "rx-parse.y"
    { id24 (2, 0x68, 0x00); F ((yyvsp[(4) - (4)].regno) % 16, 20, 4); F ((yyvsp[(4) - (4)].regno) / 16, 15, 1);
	    F ((yyvsp[(2) - (4)].regno), 16, 4); }
    break;

  case 177:
#line 748 "rx-parse.y"
    { id24 (2, 0x6a, 0); F ((yyvsp[(2) - (4)].regno), 15, 5); F ((yyvsp[(4) - (4)].regno), 20, 4); }
    break;

  case 178:
#line 753 "rx-parse.y"
    { id24 (2, 0x6e, 0); FE ((yyvsp[(3) - (5)].exp), 15, 5); F ((yyvsp[(5) - (5)].regno), 20, 4); }
    break;

  case 179:
#line 755 "rx-parse.y"
    { id24 (2, 0x6c, 0); FE ((yyvsp[(3) - (5)].exp), 15, 5); F ((yyvsp[(5) - (5)].regno), 20, 4); }
    break;

  case 180:
#line 760 "rx-parse.y"
    { id24 (2, 0x73, 0x00); F ((yyvsp[(5) - (5)].regno), 19, 5); IMM ((yyvsp[(3) - (5)].exp), 12); }
    break;

  case 181:
#line 765 "rx-parse.y"
    { id24 (2, 0xe0, 0x00); F ((yyvsp[(1) - (5)].regno), 16, 4); FE ((yyvsp[(3) - (5)].exp), 11, 5);
	      F ((yyvsp[(5) - (5)].regno), 20, 4); }
    break;

  case 182:
#line 771 "rx-parse.y"
    { id24 (2, 0xe0, 0xf0); FE ((yyvsp[(3) - (5)].exp), 11, 5); F ((yyvsp[(5) - (5)].regno), 20, 4); }
    break;

  case 183:
#line 776 "rx-parse.y"
    { id24 (3, 0x00, 0); F ((yyvsp[(2) - (9)].regno), 10, 2); F ((yyvsp[(6) - (9)].regno), 12, 4); F ((yyvsp[(8) - (9)].regno), 16, 4); F ((yyvsp[(3) - (9)].regno), 20, 4); }
    break;

  case 184:
#line 779 "rx-parse.y"
    { id24 (3, 0x40, 0); F ((yyvsp[(2) - (9)].regno), 10, 2); F ((yyvsp[(4) - (9)].regno), 12, 4); F ((yyvsp[(6) - (9)].regno), 16, 4); F ((yyvsp[(9) - (9)].regno), 20, 4); }
    break;

  case 185:
#line 782 "rx-parse.y"
    { id24 (3, 0xc0, 0); F ((yyvsp[(2) - (9)].regno), 10, 2); F ((yyvsp[(4) - (9)].regno), 12, 4); F ((yyvsp[(6) - (9)].regno), 16, 4); F ((yyvsp[(9) - (9)].regno), 20, 4); }
    break;

  case 186:
#line 786 "rx-parse.y"
    { sub_op = 0; }
    break;

  case 188:
#line 787 "rx-parse.y"
    { sub_op = 2; }
    break;

  case 190:
#line 788 "rx-parse.y"
    { sub_op = 3; }
    break;

  case 192:
#line 789 "rx-parse.y"
    { sub_op = 4; }
    break;

  case 194:
#line 790 "rx-parse.y"
    { sub_op = 5; }
    break;

  case 196:
#line 796 "rx-parse.y"
    { id24 (2, 0x70, 0x20); F ((yyvsp[(5) - (5)].regno), 20, 4); NBIMM ((yyvsp[(3) - (5)].exp), 12); }
    break;

  case 197:
#line 806 "rx-parse.y"
    { B2 (0x43 + (sub_op<<2), 0); F ((yyvsp[(1) - (3)].regno), 8, 4); F ((yyvsp[(3) - (3)].regno), 12, 4); }
    break;

  case 198:
#line 808 "rx-parse.y"
    { B2 (0x40 + (sub_op<<2), 0); F ((yyvsp[(3) - (7)].regno), 8, 4); F ((yyvsp[(7) - (7)].regno), 12, 4); DSP ((yyvsp[(1) - (7)].exp), 6, BSIZE); }
    break;

  case 199:
#line 810 "rx-parse.y"
    { B3 (MEMEX, sub_op<<2, 0); F ((yyvsp[(5) - (7)].regno), 8, 2); F ((yyvsp[(3) - (7)].regno), 16, 4); F ((yyvsp[(7) - (7)].regno), 20, 4); DSP ((yyvsp[(1) - (7)].exp), 14, sizemap[(yyvsp[(5) - (7)].regno)]); }
    break;

  case 200:
#line 812 "rx-parse.y"
    { id24 (4, sub_op<<4, 0), F ((yyvsp[(5) - (5)].regno), 12, 4), F ((yyvsp[(1) - (5)].regno), 16, 4), F ((yyvsp[(3) - (5)].regno), 20, 4); }
    break;

  case 201:
#line 819 "rx-parse.y"
    { id24 (1, 0x03 + (sub_op<<2), 0x00); F ((yyvsp[(1) - (3)].regno), 16, 4); F ((yyvsp[(3) - (3)].regno), 20, 4); }
    break;

  case 202:
#line 821 "rx-parse.y"
    { B4 (MEMEX, 0xa0, 0x00 + sub_op, 0x00);
	  F ((yyvsp[(3) - (7)].regno), 24, 4); F ((yyvsp[(7) - (7)].regno), 28, 4); DSP ((yyvsp[(1) - (7)].exp), 14, LSIZE); }
    break;

  case 203:
#line 829 "rx-parse.y"
    { id24 (1, 0x03 + (sub_op<<2), 0x00); F ((yyvsp[(1) - (3)].regno), 16, 4); F ((yyvsp[(3) - (3)].regno), 20, 4); }
    break;

  case 204:
#line 831 "rx-parse.y"
    { id24 (1, 0x00 + (sub_op<<2), 0x00); F ((yyvsp[(3) - (7)].regno), 16, 4); F ((yyvsp[(7) - (7)].regno), 20, 4); DSP ((yyvsp[(1) - (7)].exp), 14, BSIZE); }
    break;

  case 205:
#line 833 "rx-parse.y"
    { B4 (MEMEX, 0x20 + ((yyvsp[(5) - (7)].regno) << 6), 0x00 + sub_op, 0x00);
	  F ((yyvsp[(3) - (7)].regno), 24, 4); F ((yyvsp[(7) - (7)].regno), 28, 4); DSP ((yyvsp[(1) - (7)].exp), 14, sizemap[(yyvsp[(5) - (7)].regno)]); }
    break;

  case 206:
#line 839 "rx-parse.y"
    { id24 (2, 0x70, sub_op<<4); F ((yyvsp[(4) - (4)].regno), 20, 4); IMM ((yyvsp[(2) - (4)].exp), 12); }
    break;

  case 211:
#line 854 "rx-parse.y"
    { id24 (1, 0x03 + (sub_op<<2), 0x00); F ((yyvsp[(1) - (3)].regno), 16, 4); F ((yyvsp[(3) - (3)].regno), 20, 4); }
    break;

  case 212:
#line 856 "rx-parse.y"
    { B2 (0x7e, sub_op2 << 4); F ((yyvsp[(1) - (1)].regno), 12, 4); }
    break;

  case 213:
#line 862 "rx-parse.y"
    { id24 (1, 0x03 + (sub_op<<2), 0); F ((yyvsp[(1) - (3)].regno), 16, 4); F ((yyvsp[(3) - (3)].regno), 20, 4); }
    break;

  case 214:
#line 864 "rx-parse.y"
    { id24 (1, 0x00 + (sub_op<<2), 0); F ((yyvsp[(3) - (7)].regno), 16, 4); F ((yyvsp[(7) - (7)].regno), 20, 4); DSP ((yyvsp[(1) - (7)].exp), 14, BSIZE); }
    break;

  case 215:
#line 866 "rx-parse.y"
    { B4 (MEMEX, 0x20, 0x00 + sub_op, 0); F ((yyvsp[(5) - (7)].regno), 8, 2); F ((yyvsp[(3) - (7)].regno), 24, 4); F ((yyvsp[(7) - (7)].regno), 28, 4);
	    DSP ((yyvsp[(1) - (7)].exp), 14, sizemap[(yyvsp[(5) - (7)].regno)]); }
    break;

  case 216:
#line 873 "rx-parse.y"
    { id24 (2, 0x60 + sub_op, 0); F ((yyvsp[(1) - (3)].regno), 16, 4); F ((yyvsp[(3) - (3)].regno), 20, 4); }
    break;

  case 217:
#line 877 "rx-parse.y"
    { B2 (0x68 + (sub_op<<1), 0); FE ((yyvsp[(2) - (4)].exp), 7, 5); F ((yyvsp[(4) - (4)].regno), 12, 4); }
    break;

  case 218:
#line 879 "rx-parse.y"
    { id24 (2, 0x80 + (sub_op << 5), 0); FE ((yyvsp[(2) - (6)].exp), 11, 5); F ((yyvsp[(4) - (6)].regno), 16, 4); F ((yyvsp[(6) - (6)].regno), 20, 4); }
    break;

  case 220:
#line 887 "rx-parse.y"
    { id24 (2, 0x72, sub_op << 4); F ((yyvsp[(4) - (4)].regno), 20, 4); O4 ((yyvsp[(2) - (4)].exp)); }
    break;

  case 222:
#line 892 "rx-parse.y"
    { id24 (1, 0x83 + (sub_op << 2), 0); F ((yyvsp[(1) - (3)].regno), 16, 4); F ((yyvsp[(3) - (3)].regno), 20, 4); }
    break;

  case 223:
#line 894 "rx-parse.y"
    { id24 (1, 0x80 + (sub_op << 2), 0); F ((yyvsp[(3) - (7)].regno), 16, 4); F ((yyvsp[(7) - (7)].regno), 20, 4); DSP ((yyvsp[(1) - (7)].exp), 14, LSIZE); }
    break;

  case 224:
#line 899 "rx-parse.y"
    { (yyval.exp) = zero_expr (); }
    break;

  case 225:
#line 900 "rx-parse.y"
    { (yyval.exp) = (yyvsp[(1) - (1)].exp); }
    break;

  case 226:
#line 903 "rx-parse.y"
    { need_flag = 1; }
    break;

  case 227:
#line 903 "rx-parse.y"
    { need_flag = 0; (yyval.regno) = (yyvsp[(2) - (2)].regno); }
    break;

  case 228:
#line 908 "rx-parse.y"
    { (yyval.regno) = 0; }
    break;

  case 229:
#line 909 "rx-parse.y"
    { (yyval.regno) = 1; }
    break;

  case 230:
#line 910 "rx-parse.y"
    { (yyval.regno) = 2; }
    break;

  case 231:
#line 911 "rx-parse.y"
    { (yyval.regno) = 2; }
    break;

  case 232:
#line 912 "rx-parse.y"
    { (yyval.regno) = 3; }
    break;

  case 233:
#line 915 "rx-parse.y"
    { (yyval.regno) = LSIZE; }
    break;

  case 234:
#line 916 "rx-parse.y"
    { (yyval.regno) = BSIZE; }
    break;

  case 235:
#line 917 "rx-parse.y"
    { (yyval.regno) = WSIZE; }
    break;

  case 236:
#line 918 "rx-parse.y"
    { (yyval.regno) = LSIZE; }
    break;

  case 237:
#line 921 "rx-parse.y"
    { (yyval.regno) = 1; }
    break;

  case 238:
#line 922 "rx-parse.y"
    { (yyval.regno) = 0; }
    break;

  case 239:
#line 923 "rx-parse.y"
    { (yyval.regno) = 1; }
    break;

  case 240:
#line 926 "rx-parse.y"
    {}
    break;

  case 241:
#line 927 "rx-parse.y"
    {}
    break;


/* Line 1267 of yacc.c.  */
#line 3299 "rx-parse.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
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
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
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

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
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


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

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
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 930 "rx-parse.y"

/* ====================================================================== */

static struct
{
  const char * string;
  int          token;
  int          val;
}
token_table[] =
{
  { "r0", REG, 0 },
  { "r1", REG, 1 },
  { "r2", REG, 2 },
  { "r3", REG, 3 },
  { "r4", REG, 4 },
  { "r5", REG, 5 },
  { "r6", REG, 6 },
  { "r7", REG, 7 },
  { "r8", REG, 8 },
  { "r9", REG, 9 },
  { "r10", REG, 10 },
  { "r11", REG, 11 },
  { "r12", REG, 12 },
  { "r13", REG, 13 },
  { "r14", REG, 14 },
  { "r15", REG, 15 },

  { "psw", CREG, 0 },
  { "pc", CREG, 1 },
  { "usp", CREG, 2 },
  { "fpsw", CREG, 3 },
  /* reserved */
  /* reserved */
  /* reserved */
  { "wr", CREG, 7 },

  { "bpsw", CREG, 8 },
  { "bpc", CREG, 9 },
  { "isp", CREG, 10 },
  { "fintv", CREG, 11 },
  { "intb", CREG, 12 },

  { "pbp", CREG, 16 },
  { "pben", CREG, 17 },

  { "bbpsw", CREG, 24 },
  { "bbpc", CREG, 25 },

  { ".s", DOT_S, 0 },
  { ".b", DOT_B, 0 },
  { ".w", DOT_W, 0 },
  { ".l", DOT_L, 0 },
  { ".a", DOT_A , 0},
  { ".ub", DOT_UB, 0 },
  { ".uw", DOT_UW , 0},

  { "c", FLAG, 0 },
  { "z", FLAG, 1 },
  { "s", FLAG, 2 },
  { "o", FLAG, 3 },
  { "i", FLAG, 8 },
  { "u", FLAG, 9 },

#define OPC(x) { #x, x, IS_OPCODE }
  OPC(ABS),
  OPC(ADC),
  OPC(ADD),
  { "and", AND_, IS_OPCODE },
  OPC(BCLR),
  OPC(BCND),
  OPC(BMCND),
  OPC(BNOT),
  OPC(BRA),
  OPC(BRK),
  OPC(BSET),
  OPC(BSR),
  OPC(BTST),
  OPC(CLRPSW),
  OPC(CMP),
  OPC(DBT),
  OPC(DIV),
  OPC(DIVU),
  OPC(EDIV),
  OPC(EDIVU),
  OPC(EMUL),
  OPC(EMULU),
  OPC(FADD),
  OPC(FCMP),
  OPC(FDIV),
  OPC(FMUL),
  OPC(FREIT),
  OPC(FSUB),
  OPC(FTOI),
  OPC(INT),
  OPC(ITOF),
  OPC(JMP),
  OPC(JSR),
  OPC(MVFACHI),
  OPC(MVFACMI),
  OPC(MVFACLO),
  OPC(MVFC),
  OPC(MVTACHI),
  OPC(MVTACLO),
  OPC(MVTC),
  OPC(MVTIPL),
  OPC(MACHI),
  OPC(MACLO),
  OPC(MAX),
  OPC(MIN),
  OPC(MOV),
  OPC(MOVU),
  OPC(MUL),
  OPC(MULHI),
  OPC(MULLO),
  OPC(MULU),
  OPC(NEG),
  OPC(NOP),
  OPC(NOT),
  OPC(OR),
  OPC(POP),
  OPC(POPC),
  OPC(POPM),
  OPC(PUSH),
  OPC(PUSHA),
  OPC(PUSHC),
  OPC(PUSHM),
  OPC(RACW),
  OPC(REIT),
  OPC(REVL),
  OPC(REVW),
  OPC(RMPA),
  OPC(ROLC),
  OPC(RORC),
  OPC(ROTL),
  OPC(ROTR),
  OPC(ROUND),
  OPC(RTE),
  OPC(RTFI),
  OPC(RTS),
  OPC(RTSD),
  OPC(SAT),
  OPC(SATR),
  OPC(SBB),
  OPC(SCCND),
  OPC(SCMPU),
  OPC(SETPSW),
  OPC(SHAR),
  OPC(SHLL),
  OPC(SHLR),
  OPC(SMOVB),
  OPC(SMOVF),
  OPC(SMOVU),
  OPC(SSTR),
  OPC(STNZ),
  OPC(STOP),
  OPC(STZ),
  OPC(SUB),
  OPC(SUNTIL),
  OPC(SWHILE),
  OPC(TST),
  OPC(WAIT),
  OPC(XCHG),
  OPC(XOR),
};

#define NUM_TOKENS (sizeof (token_table) / sizeof (token_table[0]))

static struct
{
  char * string;
  int    token;
}
condition_opcode_table[] =
{
  { "b", BCND },
  { "bm", BMCND },
  { "sc", SCCND },
};

#define NUM_CONDITION_OPCODES (sizeof (condition_opcode_table) / sizeof (condition_opcode_table[0]))

static struct
{
  char * string;
  int    val;
}
condition_table[] =
{
  { "z", 0 },
  { "eq", 0 },
  { "geu",  2 },
  { "c",  2 },
  { "gtu", 4 },
  { "pz", 6 },
  { "ge", 8 },
  { "gt", 10 },
  { "o",  12},
  /* always = 14 */
  { "nz", 1 },
  { "ne", 1 },
  { "ltu", 3 },
  { "nc", 3 },
  { "leu", 5 },
  { "n", 7 },
  { "lt", 9 },
  { "le", 11 },
  { "no", 13 }
  /* never = 15 */
};

#define NUM_CONDITIONS (sizeof (condition_table) / sizeof (condition_table[0]))

void
rx_lex_init (char * beginning, char * ending)
{
  rx_init_start = beginning;
  rx_lex_start = beginning;
  rx_lex_end = ending;
  rx_in_brackets = 0;
  rx_last_token = 0;

  setbuf (stdout, 0);
}

static int
check_condition (char * base)
{
  char * cp;
  unsigned int i;

  if ((unsigned) (rx_lex_end - rx_lex_start) < strlen (base) + 1)
    return 0;
  if (memcmp (rx_lex_start, base, strlen (base)))
    return 0;
  cp = rx_lex_start + strlen (base);
  for (i = 0; i < NUM_CONDITIONS; i ++)
    {
      if (strcasecmp (cp, condition_table[i].string) == 0)
	{
	  rx_lval.regno = condition_table[i].val;
	  return 1;
	}
    }
  return 0;
}

static int
rx_lex (void)
{
  unsigned int ci;
  char * save_input_pointer;

  while (ISSPACE (*rx_lex_start)
	 && rx_lex_start != rx_lex_end)
    rx_lex_start ++;

  rx_last_exp_start = rx_lex_start;

  if (rx_lex_start == rx_lex_end)
    return 0;

  if (ISALPHA (*rx_lex_start)
      || (rx_pid_register != -1 && memcmp (rx_lex_start, "%pidreg", 7) == 0)
      || (rx_gp_register != -1 && memcmp (rx_lex_start, "%gpreg", 6) == 0)
      || (*rx_lex_start == '.' && ISALPHA (rx_lex_start[1])))
    {
      unsigned int i;
      char * e;
      char save;

      for (e = rx_lex_start + 1;
	   e < rx_lex_end && ISALNUM (*e);
	   e ++)
	;
      save = *e;
      *e = 0;

      if (strcmp (rx_lex_start, "%pidreg") == 0)
	{
	  {
	    rx_lval.regno = rx_pid_register;
	    *e = save;
	    rx_lex_start = e;
	    rx_last_token = REG;
	    return REG;
	  }
	}

      if (strcmp (rx_lex_start, "%gpreg") == 0)
	{
	  {
	    rx_lval.regno = rx_gp_register;
	    *e = save;
	    rx_lex_start = e;
	    rx_last_token = REG;
	    return REG;
	  }
	}

      if (rx_last_token == 0)
	for (ci = 0; ci < NUM_CONDITION_OPCODES; ci ++)
	  if (check_condition (condition_opcode_table[ci].string))
	    {
	      *e = save;
	      rx_lex_start = e;
	      rx_last_token = condition_opcode_table[ci].token;
	      return condition_opcode_table[ci].token;
	    }

      for (i = 0; i < NUM_TOKENS; i++)
	if (strcasecmp (rx_lex_start, token_table[i].string) == 0
	    && !(token_table[i].val == IS_OPCODE && rx_last_token != 0)
	    && !(token_table[i].token == FLAG && !need_flag))
	  {
	    rx_lval.regno = token_table[i].val;
	    *e = save;
	    rx_lex_start = e;
	    rx_last_token = token_table[i].token;
	    return token_table[i].token;
	  }
      *e = save;
    }

  if (rx_last_token == 0)
    {
      rx_last_token = UNKNOWN_OPCODE;
      return UNKNOWN_OPCODE;
    }

  if (rx_last_token == UNKNOWN_OPCODE)
    return 0;

  if (*rx_lex_start == '[')
    rx_in_brackets = 1;
  if (*rx_lex_start == ']')
    rx_in_brackets = 0;

  if (rx_in_brackets
      || rx_last_token == REG
      || strchr ("[],#", *rx_lex_start))
    {
      rx_last_token = *rx_lex_start;
      return *rx_lex_start ++;
    }

  save_input_pointer = input_line_pointer;
  input_line_pointer = rx_lex_start;
  rx_lval.exp.X_md = 0;
  expression (&rx_lval.exp);

  /* We parse but ignore any :<size> modifier on expressions.  */
  if (*input_line_pointer == ':')
    {
      char *cp;

      for (cp  = input_line_pointer + 1; *cp && cp < rx_lex_end; cp++)
	if (!ISDIGIT (*cp))
	  break;
      if (cp > input_line_pointer+1)
	input_line_pointer = cp;
    }

  rx_lex_start = input_line_pointer;
  input_line_pointer = save_input_pointer;
  rx_last_token = EXPR;
  return EXPR;
}

int
rx_error (const char * str)
{
  int len;

  len = rx_last_exp_start - rx_init_start;

  as_bad ("%s", rx_init_start);
  as_bad ("%*s^ %s", len, "", str);
  return 0;
}

static int
rx_intop (expressionS exp, int nbits, int opbits)
{
  long v;
  long mask, msb;

  if (exp.X_op == O_big && nbits == 32)
      return 1;
  if (exp.X_op != O_constant)
    return 0;
  v = exp.X_add_number;

  msb = 1UL << (opbits - 1);
  mask = (1UL << opbits) - 1;

  if ((v & msb) && ! (v & ~mask))
    v -= 1UL << opbits;

  switch (nbits)
    {
    case 4:
      return -0x8 <= v && v <= 0x7;
    case 5:
      return -0x10 <= v && v <= 0x17;
    case 8:
      return -0x80 <= v && v <= 0x7f;
    case 16:
      return -0x8000 <= v && v <= 0x7fff;
    case 24:
      return -0x800000 <= v && v <= 0x7fffff;
    case 32:
      return 1;
    default:
      printf ("rx_intop passed %d\n", nbits);
      abort ();
    }
  return 1;
}

static int
rx_uintop (expressionS exp, int nbits)
{
  unsigned long v;

  if (exp.X_op != O_constant)
    return 0;
  v = exp.X_add_number;

  switch (nbits)
    {
    case 4:
      return v <= 0xf;
    case 8:
      return v <= 0xff;
    case 16:
      return v <= 0xffff;
    case 24:
      return v <= 0xffffff;
    default:
      printf ("rx_uintop passed %d\n", nbits);
      abort ();
    }
  return 1;
}

static int
rx_disp3op (expressionS exp)
{
  unsigned long v;

  if (exp.X_op != O_constant)
    return 0;
  v = exp.X_add_number;
  if (v < 3 || v > 10)
    return 0;
  return 1;
}

static int
rx_disp5op (expressionS * exp, int msize)
{
  long v;

  if (exp->X_op != O_constant)
    return 0;
  v = exp->X_add_number;

  switch (msize)
    {
    case BSIZE:
      if (0 < v && v <= 31)
	return 1;
      break;
    case WSIZE:
      if (v & 1)
	return 0;
      if (0 < v && v <= 63)
	{
	  exp->X_add_number >>= 1;
	  return 1;
	}
      break;
    case LSIZE:
      if (v & 3)
	return 0;
      if (0 < v && v <= 127)
	{
	  exp->X_add_number >>= 2;
	  return 1;
	}
      break;
    }
  return 0;
}

/* Just like the above, but allows a zero displacement.  */

static int
rx_disp5op0 (expressionS * exp, int msize)
{
  if (exp->X_op != O_constant)
    return 0;
  if (exp->X_add_number == 0)
    return 1;
  return rx_disp5op (exp, msize);
}

static int
exp_val (expressionS exp)
{
  if (exp.X_op != O_constant)
  {
    rx_error (_("constant expected"));
    return 0;
  }
  return exp.X_add_number;
}

static expressionS
zero_expr (void)
{
  /* Static, so program load sets it to all zeros, which is what we want.  */
  static expressionS zero;
  zero.X_op = O_constant;
  return zero;
}

static int
immediate (expressionS exp, int type, int pos, int bits)
{
  /* We will emit constants ourself here, so negate them.  */
  if (type == RXREL_NEGATIVE && exp.X_op == O_constant)
    exp.X_add_number = - exp.X_add_number;
  if (type == RXREL_NEGATIVE_BORROW)
    {
      if (exp.X_op == O_constant)
	exp.X_add_number = - exp.X_add_number - 1;
      else
	rx_error (_("sbb cannot use symbolic immediates"));
    }

  if (rx_intop (exp, 8, bits))
    {
      rx_op (exp, 1, type);
      return 1;
    }
  else if (rx_intop (exp, 16, bits))
    {
      rx_op (exp, 2, type);
      return 2;
    }
  else if (rx_uintop (exp, 16) && bits == 16)
    {
      rx_op (exp, 2, type);
      return 2;
    }
  else if (rx_intop (exp, 24, bits))
    {
      rx_op (exp, 3, type);
      return 3;
    }
  else if (rx_intop (exp, 32, bits))
    {
      rx_op (exp, 4, type);
      return 0;
    }
  else if (type == RXREL_SIGNED)
    {
      /* This is a symbolic immediate, we will relax it later.  */
      rx_relax (RX_RELAX_IMM, pos);
      rx_op (exp, linkrelax ? 4 : 1, type);
      return 1;
    }
  else
    {
      /* Let the linker deal with it.  */
      rx_op (exp, 4, type);
      return 0;
    }
}

static int
displacement (expressionS exp, int msize)
{
  int val;
  int vshift = 0;

  if (exp.X_op == O_symbol
      && exp.X_md)
    {
      switch (exp.X_md)
	{
	case BFD_RELOC_GPREL16:
	  switch (msize)
	    {
	    case BSIZE:
	      exp.X_md = BFD_RELOC_RX_GPRELB;
	      break;
	    case WSIZE:
	      exp.X_md = BFD_RELOC_RX_GPRELW;
	      break;
	    case LSIZE:
	      exp.X_md = BFD_RELOC_RX_GPRELL;
	      break;
	    }
	  O2 (exp);
	  return 2;
	}
    }

  if (exp.X_op == O_subtract)
    {
      exp.X_md = BFD_RELOC_RX_DIFF;
      O2 (exp);
      return 2;
    }

  if (exp.X_op != O_constant)
    {
      rx_error (_("displacements must be constants"));
      return -1;
    }
  val = exp.X_add_number;

  if (val == 0)
    return 0;

  switch (msize)
    {
    case BSIZE:
      break;
    case WSIZE:
      if (val & 1)
	rx_error (_("word displacement not word-aligned"));
      vshift = 1;
      break;
    case LSIZE:
      if (val & 3)
	rx_error (_("long displacement not long-aligned"));
      vshift = 2;
      break;
    default:
      as_bad (_("displacement with unknown size (internal bug?)\n"));
      break;
    }

  val >>= vshift;
  exp.X_add_number = val;

  if (0 <= val && val <= 255 )
    {
      O1 (exp);
      return 1;
    }

  if (0 <= val && val <= 65535)
    {
      O2 (exp);
      return 2;
    }
  if (val < 0)
    rx_error (_("negative displacements not allowed"));
  else
    rx_error (_("displacement too large"));
  return -1;
}

static void
rtsd_immediate (expressionS exp)
{
  int val;

  if (exp.X_op != O_constant)
    {
      rx_error (_("rtsd size must be constant"));
      return;
    }
  val = exp.X_add_number;
  if (val & 3)
    rx_error (_("rtsd size must be multiple of 4"));

  if (val < 0 || val > 1020)
    rx_error (_("rtsd size must be 0..1020"));

  val >>= 2;
  exp.X_add_number = val;
  O1 (exp);
}

static void
rx_range (expressionS exp, int minv, int maxv)
{
  int val;

  if (exp.X_op != O_constant)
    return;

  val = exp.X_add_number;
  if (val < minv || val > maxv)
    as_warn (_("Value %d out of range %d..%d"), val, minv, maxv);
}

